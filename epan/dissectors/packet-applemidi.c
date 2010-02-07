/* packet-applemidi.c
 * Routines for dissection of Apple network-midi session establishment.
 * Copyright 2006-2010, Tobias Erichsen <t.erichsen@gmx.de>
 *
 * $Id$
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * Copied from packet-data.c, README.developer, and various other files.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *
 * Apple network-midi session establishment is a lightweight protocol for providing
 * a simple session establishment for MIDI-data sent in the form of RTP-MIDI (RFC 4695)
 * Peers recognize each other using the Apple Bonjour scheme with the service-name
 * "_apple-midi._udp", establish a connection using AppleMIDI (no official name, just
 * an abbreviation) and then send payload using RTP-MIDI.  The implementation of
 * this dissector is based on the Apple implementation summary from May 6th, 2005.
 *
 * Here are some links:
 *
 * http://www.cs.berkeley.edu/~lazzaro/rtpmidi/
 * http://www.faqs.org/rfcs/rfc4695.html
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glib.h>
#include <epan/packet.h>
#include <epan/prefs.h>
#include <epan/conversation.h>

/* Definitions for protocol name during dissector-register */
#define APPLEMIDI_DISSECTOR_NAME			"Apple Network-MIDI Session Protocol"
#define APPLEMIDI_DISSECTOR_SHORTNAME		"AppleMIDI"
#define APPLEMIDI_DISSECTOR_ABBREVIATION	"applemidi"

/* Signature "Magic Value" for Apple network MIDI session establishment */
#define APPLEMIDI_PROTOCOL_SIGNATURE			0xffff

/* Apple network MIDI valid commands */
#define APPLEMIDI_COMMAND_INVITATION			0x494e		/*   "IN"   */
#define APPLEMIDI_COMMAND_INVITATION_REJECTED	0x4e4f		/*   "NO"   */
#define APLLEMIDI_COMMAND_INVITATION_ACCEPTED	0x4f4b		/*   "OK"   */
#define APPLEMIDI_COMMAND_ENDSESSION			0x4259		/*   "BY"   */
#define APPLEMIDI_COMMAND_SYNCHRONIZATION		0x434b		/*   "CK"   */
#define APPLEMIDI_COMMAND_RECEIVER_FEEDBACK		0x5253		/*   "RS"   */


static int	hf_applemidi_signature			= -1;
static int	hf_applemidi_command			= -1;
static int	hf_applemidi_protocol_version		= -1;
static int	hf_applemidi_token			= -1;
static int	hf_applemidi_ssrc			= -1;
static int	hf_applemidi_name			= -1;
static int	hf_applemidi_count			= -1;
static int	hf_applemidi_padding			= -1;
static int	hf_applemidi_timestamp1			= -1;
static int	hf_applemidi_timestamp2			= -1;
static int	hf_applemidi_timestamp3			= -1;
static int	hf_applemidi_sequence_num		= -1;
static int	hf_applemidi_rtp_sequence_num		= -1;


static gint	ett_applemidi				= -1;
static gint	ett_applemidi_seq_num			= -1;


static const value_string applemidi_commands[] = {
	{ APPLEMIDI_COMMAND_INVITATION,				"Invitation" },
	{ APPLEMIDI_COMMAND_INVITATION_REJECTED,	"Invitation Rejected" },
	{ APLLEMIDI_COMMAND_INVITATION_ACCEPTED,	"Invitation Accepted" },
	{ APPLEMIDI_COMMAND_ENDSESSION,				"End Session" },
	{ APPLEMIDI_COMMAND_SYNCHRONIZATION,		"Synchronization" },
	{ APPLEMIDI_COMMAND_RECEIVER_FEEDBACK,		"Receiver Feedback" },
	{ 0,						NULL },
};


void   proto_reg_handoff_applemidi( void );

static guint			udp_port			= 0;

static int			proto_applemidi			= -1;
static dissector_handle_t	applemidi_handle		= NULL;
static dissector_handle_t	rtp_handle			= NULL;






static void
dissect_applemidi( tvbuff_t *tvb, packet_info *pinfo _U_ , proto_tree *tree ) {

	guint16		 command;
	guint16		 seq_num;
	guint8		 count;
	guint8		*name;
	unsigned int	 offset			= 0;
	proto_tree	*applemidi_tree		= NULL;
	proto_tree	*applemidi_tree_seq_num = NULL;


	col_set_str( pinfo->cinfo, COL_PROTOCOL, APPLEMIDI_DISSECTOR_SHORTNAME );

	/* Clear out stuff in the info column */
	col_clear( pinfo->cinfo, COL_INFO );

	command   = tvb_get_ntohs( tvb, offset + 2 );

	col_add_fstr( pinfo->cinfo, COL_INFO, "%s", val_to_str( command, applemidi_commands, "Unknown Command: 0x%04x" ) );

	if ( tree ) {
		proto_item *ti = NULL;
		ti = proto_tree_add_item( tree, proto_applemidi, tvb, 0, -1, FALSE );
		applemidi_tree = proto_item_add_subtree( ti, ett_applemidi );

		proto_tree_add_item( applemidi_tree, hf_applemidi_signature, tvb, offset, 2, FALSE );
		offset += 2;

		proto_tree_add_item( applemidi_tree, hf_applemidi_command, tvb, offset, 2, FALSE );
		offset += 2;

		/* the format of packets for "IN", "NO", "OK" and "BY" is identical and contains
		 * the protocol version, a random number generated by the initiator of the session
		 * the SSRC that is used by the respective sides RTP-entity and optionally the
		 * name of the participant */
		if ( ( APPLEMIDI_COMMAND_INVITATION == command ) || ( APPLEMIDI_COMMAND_INVITATION_REJECTED == command ) || ( APLLEMIDI_COMMAND_INVITATION_ACCEPTED == command ) || ( APPLEMIDI_COMMAND_ENDSESSION == command ) ) {
			int len;

			proto_tree_add_item( applemidi_tree, hf_applemidi_protocol_version, tvb, offset, 4, FALSE );
			offset += 4;

			proto_tree_add_item( applemidi_tree, hf_applemidi_token, tvb, offset, 4, FALSE );
			offset += 4;

			proto_tree_add_item( applemidi_tree, hf_applemidi_ssrc, tvb, offset, 4, FALSE );
			offset += 4;

			len = tvb->length - offset;

			/* Name is optional */
			if ( len > 0 ) {
				name = tvb_get_ephemeral_string( tvb, offset, len );
				proto_tree_add_item( applemidi_tree, hf_applemidi_name, tvb, offset, len, FALSE );
				col_append_fstr( pinfo->cinfo, COL_INFO, ": peer = \"%s\"", name );
			}

		/* the synchronization packet contains three 64bit timestamps,  and a value to define how
		 * many of the timestamps transmitted are valid */
		} else if ( APPLEMIDI_COMMAND_SYNCHRONIZATION == command ) {
			proto_tree_add_item( applemidi_tree, hf_applemidi_ssrc, tvb, offset, 4, FALSE );
			offset += 4;

			count = tvb_get_guint8( tvb, offset );
			proto_tree_add_item( applemidi_tree, hf_applemidi_count, tvb, offset, 1, FALSE );
			col_append_fstr( pinfo->cinfo, COL_INFO, ": count = %u", count );
			offset += 1;

			proto_tree_add_item( applemidi_tree, hf_applemidi_padding, tvb, offset, 3, FALSE );
			offset += 3;

			proto_tree_add_item(applemidi_tree, hf_applemidi_timestamp1, tvb, offset, 8, FALSE );
			offset += 8;

			proto_tree_add_item( applemidi_tree, hf_applemidi_timestamp2, tvb, offset, 8, FALSE );
			offset += 8;

			proto_tree_add_item( applemidi_tree, hf_applemidi_timestamp3, tvb, offset, 8, FALSE );
			offset += 8;
		/* With the receiver feedback packet, the recipient can tell the sender up to what sequence
		 * number in the RTP-stream the packets have been received, this can be used to shorten the
		 * recovery-journal-section in the RTP-session */
		} else if ( APPLEMIDI_COMMAND_RECEIVER_FEEDBACK == command ) {
			proto_tree_add_item( applemidi_tree, hf_applemidi_ssrc, tvb, offset, 4, FALSE );
			offset += 4;

			ti = proto_tree_add_item( applemidi_tree, hf_applemidi_sequence_num, tvb, offset, 4, FALSE );
			/* Apple includes a 32bit sequence-number, but the RTP-packet only specifies 16bit
			 * this subtree and subitem are added to be able to associate the sequence-number
			 * here easier with the one specified in the corresponding RTP-packet */
			applemidi_tree_seq_num = proto_item_add_subtree( ti, ett_applemidi_seq_num );
			seq_num = tvb_get_ntohs( tvb, offset );
			proto_tree_add_uint( applemidi_tree_seq_num, hf_applemidi_rtp_sequence_num, tvb, offset, 2, seq_num );
			offset += 4;

			col_append_fstr( pinfo->cinfo, COL_INFO, ": seq = %u", seq_num );
		}
	}
}


static gboolean
dissect_applemidi_heur( tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree ) {

	unsigned int	 offset	= 0;
	guint16			 signature;
	guint16			 command;
	conversation_t	*p_conv;
	gboolean		 result	= FALSE;



	/* Get the two octets making up the magic value */
	signature = tvb_get_ntohs( tvb, offset );


	/* An applemidi session protocol UDP-packet must start with the "magic value" of 0xffff ... */
	if ( APPLEMIDI_PROTOCOL_SIGNATURE == signature ) {

		command = tvb_get_ntohs( tvb, offset + 2 );

		/* ... followed by packet-command: "IN", "NO", "OK", "BY", "CK" and "RS" */
		if( APPLEMIDI_COMMAND_INVITATION == command )
		  result = TRUE;
		else if( APPLEMIDI_COMMAND_INVITATION_REJECTED == command )
		  result = TRUE;
		else if( APLLEMIDI_COMMAND_INVITATION_ACCEPTED == command )
		  result = TRUE;
		else if( APPLEMIDI_COMMAND_ENDSESSION == command )
		  result = TRUE;
		else if( APPLEMIDI_COMMAND_SYNCHRONIZATION == command )
		  result = TRUE;
		else if( APPLEMIDI_COMMAND_RECEIVER_FEEDBACK == command )
		  result = TRUE;

	}

	p_conv=find_conversation( pinfo->fd->num, &pinfo->src, &pinfo->dst, pinfo->ptype, pinfo->srcport, pinfo->destport, 0 );

	if( result ) {
		if( !p_conv ) {
			p_conv = conversation_new( 0, &pinfo->src, &pinfo->dst, pinfo->ptype, pinfo->srcport, pinfo->destport, 0 );
		}
		if( p_conv ) {
			conversation_set_dissector( p_conv, applemidi_handle );
		}

		dissect_applemidi( tvb, pinfo, tree );
	} else {
		if( p_conv )  {
			if( p_conv->dissector_handle == applemidi_handle ) {
				result = TRUE;
				/* punt packets not containing valid AppleMIDI-commands to RTP (or in
				 * case it exists, the more specialized RTP-MIDI-dissector */
				call_dissector( rtp_handle, tvb, pinfo, tree );
			}
		}
	}
	return result;
}


void
proto_register_applemidi( void )
{
	module_t *applemidi_module;

	static hf_register_info hf[] =	{
		{
			&hf_applemidi_signature,
			{
				"Signature",
				"applemidi.signature",
				FT_UINT16,
				BASE_HEX,
				NULL,
				0x0,
				NULL, HFILL
			}
		},
		{
			&hf_applemidi_command,
			{
				"Command",
				"applemidi.command",
				FT_UINT16,
				BASE_HEX,
				VALS( applemidi_commands ),
				0x0,
				NULL, HFILL
			}
		},
		{
			&hf_applemidi_protocol_version,
			{
				"Protocol Version",
				"applemidi.protocol_version",
				FT_UINT32,
				BASE_DEC,
				NULL,
				0x0,
				NULL, HFILL
			}
		},
		{
			&hf_applemidi_token,
			{
				"Initiator Token",
				"applemidi.initiator_token",
				FT_UINT32,
				BASE_HEX,
				NULL,
				0x0,
				NULL, HFILL
			}
		},
		{
			&hf_applemidi_ssrc,
			{
				"Sender SSRC",
				"applemidi.sender_ssrc",
				FT_UINT32,
				BASE_HEX,
				NULL,
				0x0,
				NULL, HFILL
			}
		},
		{
			&hf_applemidi_name,
			{
				"Name",
				"applemidi.name",
				FT_STRING,
				BASE_NONE,
				NULL,
				0x0,
				NULL, HFILL
			}
		},
		{
			&hf_applemidi_count,
			{
				"Count",
				"applemidi.count",
				FT_UINT8,
				BASE_DEC,
				NULL,
				0x0,
				NULL, HFILL
			}
		},
		{
			&hf_applemidi_padding,
			{
				"Padding",
				"applemidi.padding",
				FT_UINT24,
				BASE_HEX,
				NULL,
				0x0,
				NULL, HFILL
			}
		},
		{
			&hf_applemidi_timestamp1,
			{
				"Timestamp 1",
				"applemidi.timestamp1",
				FT_UINT64,
				BASE_HEX,
				NULL,
				0x0,
				NULL, HFILL
			}
		},
		{
			&hf_applemidi_timestamp2,
			{
				"Timestamp 2",
				"applemidi.timestamp2",
				FT_UINT64,
				BASE_HEX,
				NULL,
				0x0,
				NULL, HFILL
			}
		},
		{
			&hf_applemidi_timestamp3,
			{
				"Timestamp 3",
				"applemidi.timestamp3",
				FT_UINT64,
				BASE_HEX,
				NULL,
				0x0,
				NULL, HFILL
			}
		},
		{
			&hf_applemidi_sequence_num,
			{
				"Sequence Number",
				"applemidi.sequence_number",
				FT_UINT32,
				BASE_HEX,
				NULL,
				0x0,
				NULL, HFILL
			}
		},
		{
			&hf_applemidi_rtp_sequence_num,
			{
				"RTP Sequence Number",
				"applemidi.rtp_sequence_number",
				FT_UINT16,
				BASE_DEC,
				NULL,
				0x0,
				NULL, HFILL
			}
		},
	};


	static gint *ett[] = {
		&ett_applemidi,
		&ett_applemidi_seq_num
	};

    proto_applemidi = proto_register_protocol( APPLEMIDI_DISSECTOR_NAME, APPLEMIDI_DISSECTOR_SHORTNAME, APPLEMIDI_DISSECTOR_ABBREVIATION );
	proto_register_field_array( proto_applemidi, hf, array_length( hf ) );
	proto_register_subtree_array( ett, array_length( ett ) );

	applemidi_module = prefs_register_protocol( proto_applemidi, proto_reg_handoff_applemidi );

}

void
proto_reg_handoff_applemidi( void ) {


	if ( !applemidi_handle ) {
		applemidi_handle = create_dissector_handle( dissect_applemidi, proto_applemidi );
	}

	/* If we cannot decode the data it will be RTP-MIDI since the Apple session protocol uses
	 * two ports: the control-port and the MIDI-port.  On both ports an invitation is being sent.
	 * The second port is then used for the RTP-MIDI-data. So if we can't find valid AppleMidi
	 * packets, it will be most likely RTP-MIDI...
	 */
	rtp_handle = find_dissector( "rtp" );
	dissector_add( "udp.port" , udp_port, applemidi_handle );
	heur_dissector_add( "udp", dissect_applemidi_heur, proto_applemidi );
	applemidi_handle = find_dissector( "applemidi" );

}
