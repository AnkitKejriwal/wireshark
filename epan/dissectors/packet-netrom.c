/* packet-netrom.c
 *
 * Routines for Amateur Packet Radio protocol dissection
 * Copyright 2005,2006,2007,2008,2009,2010,2012 R.W. Stearn <richard@rns-stearn.demon.co.uk>
 *
 * $Id$
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@ethereal.com>
 * Copyright 1998 Gerald Combs
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 * Information on the protocol drawn from:
 *
 * Inspiration on how to build the dissector drawn from
 *   packet-sdlc.c
 *   packet-x25.c
 *   packet-lapb.c
 *   paket-gprs-llc.c
 *   xdlc.c
 * with the base file built from README.developers.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include <epan/strutil.h>
#include <epan/packet.h>
#include <epan/emem.h>
#include <epan/xdlc.h>
#include <packet-ip.h>

#include "packet-netrom.h"

#define STRLEN 80

#define AX25_ADDR_LEN		7	/* length of an AX.25 address */

#define NETROM_MIN_SIZE		7	/* minumum payload for a routing packet */
#define NETROM_HEADER_SIZE	20	/* minumum payload for a normal packet */

#define	NETROM_PROTOEXT		0x00
#define	NETROM_CONNREQ		0x01
#define	NETROM_CONNACK		0x02
#define	NETROM_DISCREQ		0x03
#define	NETROM_DISCACK		0x04
#define	NETROM_INFO		0x05
#define	NETROM_INFOACK		0x06

#define	NETROM_MORE_FLAG	0x20
#define	NETROM_NAK_FLAG		0x40
#define	NETROM_CHOKE_FLAG	0x80

#define NETROM_PROTO_IP		0x0C

/* Forward declaration we need below */
void proto_reg_handoff_netrom(void);

/* Dissector handles - all the possibles are listed */
static dissector_handle_t ip_handle;
static dissector_handle_t default_handle;

/* Initialize the protocol and registered fields */
static int proto_netrom			= -1;
static int hf_netrom_src		= -1;
static int hf_netrom_dst		= -1;
static int hf_netrom_ttl		= -1;
static int hf_netrom_my_cct_index	= -1;
static int hf_netrom_my_cct_id		= -1;
static int hf_netrom_your_cct_index	= -1;
static int hf_netrom_your_cct_id	= -1;
static int hf_netrom_n_r		= -1;
static int hf_netrom_n_s		= -1;
static int hf_netrom_type		= -1;
static int hf_netrom_op			= -1;
static int hf_netrom_more		= -1;
static int hf_netrom_nak		= -1;
static int hf_netrom_choke		= -1;

static int hf_netrom_user		= -1;
static int hf_netrom_node		= -1;
static int hf_netrom_pwindow		= -1;
static int hf_netrom_awindow		= -1;

static int hf_netrom_mnemonic		= -1;

/*
 * Structure containing pointers to hf_ values for various subfields of
 * the type field.
 */
typedef struct {
	int	*hf_tf_op;
	int	*hf_tf_more;
	int	*hf_tf_nak;
	int	*hf_tf_choke;
} netrom_tf_items;

static const netrom_tf_items netrom_type_items = {
	&hf_netrom_op,
	&hf_netrom_more,
	&hf_netrom_nak,
	&hf_netrom_choke
};


const value_string op_copde_vals[] = {
	{ NETROM_PROTOEXT	, "PROTOEXT"},
	{ NETROM_CONNREQ	, "CONNREQ"},
	{ NETROM_CONNACK	, "CONNACK"},
	{ NETROM_DISCREQ	, "DISCREQ"},
	{ NETROM_DISCACK	, "DISCACK"},
	{ NETROM_INFO		, "INFO"},
	{ NETROM_INFOACK	, "INFOACK"},
	{ 0			, NULL }
};

/* Initialize the subtree pointers */
static gint ett_netrom = -1;
static gint ett_netrom_type = -1;

static void
dissect_netrom_type(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree,
			int hf_netrom_type, gint ett_netrom_type, const netrom_tf_items *type_items )
{
	proto_tree *tc;
	proto_tree *type_tree;
	char *info_buffer;
	char *op_text_ptr;
	guint8 type;
	guint8 op_code;

	info_buffer = ep_alloc( STRLEN );
	info_buffer[0] = '\0';

	type =  tvb_get_guint8( tvb, offset );
	op_code = type &0x0f;

	switch ( op_code )
		{
		case NETROM_PROTOEXT	: op_text_ptr = "Protocol extension"		; break;
		case NETROM_CONNREQ	: op_text_ptr = "Connect request"		; break;
		case NETROM_CONNACK	: op_text_ptr = "Connect acknowledge"		; break;
		case NETROM_DISCREQ	: op_text_ptr = "Disconnect request"		; break;
		case NETROM_DISCACK	: op_text_ptr = "Disconnect acknowledge"	; break;
		case NETROM_INFO	: op_text_ptr = "Information"			; break;
		case NETROM_INFOACK	: op_text_ptr = "Information acknowledge"	; break;
		default			: op_text_ptr = "Unknown"			; break;
		}
	g_snprintf( info_buffer, STRLEN, "%s%s%s%s (0x%02x)",
						op_text_ptr,
						( type & NETROM_MORE_FLAG  ) ? ", More"  : "",
						( type & NETROM_NAK_FLAG   ) ? ", NAK"   : "",
						( type & NETROM_CHOKE_FLAG ) ? ", Choke" : "",
						type );
	col_add_str( pinfo->cinfo, COL_INFO, info_buffer );

	if ( tree )
		{
		tc = proto_tree_add_uint_format( tree,
						hf_netrom_type,
						tvb,
						offset,
						1,
						type,
						"Type field: %s",
						info_buffer
						);
		type_tree = proto_item_add_subtree( tc, ett_netrom_type );

		proto_tree_add_item( type_tree, *type_items->hf_tf_op, tvb, offset, 1, FALSE );
		proto_tree_add_item( type_tree, *type_items->hf_tf_choke, tvb, offset, 1, FALSE );
		proto_tree_add_item( type_tree, *type_items->hf_tf_nak, tvb, offset, 1, FALSE );
		proto_tree_add_item( type_tree, *type_items->hf_tf_more, tvb, offset, 1, FALSE );
		}
}

static void
dissect_netrom_proto(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	proto_item *ti;
	proto_tree *netrom_tree;
	int offset;
	char *op_text_ptr;
	const guint8 *src_addr;
	const guint8 *dst_addr;
	const guint8 *user_addr;
	const guint8 *node_addr;
	/* guint8 src_ssid;
	guint8 dst_ssid; */
	guint8 op_code;
	guint8 cct_index;
	guint8 cct_id;
	void *saved_private_data;
	tvbuff_t *next_tvb = NULL;

	col_set_str( pinfo->cinfo, COL_PROTOCOL, "NetROM" );

	col_clear( pinfo->cinfo, COL_INFO );

	offset = 0;

	/* source */
	src_addr = tvb_get_ptr( tvb,  offset, AX25_ADDR_LEN );
	SET_ADDRESS(&pinfo->dl_src,	AT_AX25, AX25_ADDR_LEN, src_addr);
	SET_ADDRESS(&pinfo->src,	AT_AX25, AX25_ADDR_LEN, src_addr);
	/* src_ssid = *(src_addr + 6); */
	offset += AX25_ADDR_LEN; /* step over src addr */

	/* destination */
	dst_addr = tvb_get_ptr( tvb,  offset, AX25_ADDR_LEN );
	SET_ADDRESS(&pinfo->dl_dst,	AT_AX25, AX25_ADDR_LEN, dst_addr);
	SET_ADDRESS(&pinfo->dst,	AT_AX25, AX25_ADDR_LEN, dst_addr);
	/* dst_ssid = *(dst_addr + 6); */
	offset += AX25_ADDR_LEN; /* step over dst addr */

	offset += 1; /* step over ttl */
	cct_index =  tvb_get_guint8( tvb, offset );
	offset += 1; /* step over cct index*/
	cct_id =  tvb_get_guint8( tvb, offset );
	offset += 1; /* step over cct id */
	offset += 1; /* step over n_s */
	offset += 1; /* step over n_r */

	/* frame type */
	op_code =  tvb_get_guint8( tvb, offset ) & 0x0f;
	offset += 1; /* step over op_code */

	switch ( op_code )
		{
		case NETROM_PROTOEXT	: op_text_ptr = "Protocol extension"		; break;
		case NETROM_CONNREQ	: op_text_ptr = "Connect request"		; break;
		case NETROM_CONNACK	: op_text_ptr = "Connect acknowledge"		; break;
		case NETROM_DISCREQ	: op_text_ptr = "Disconnect request"		; break;
		case NETROM_DISCACK	: op_text_ptr = "Disconnect acknowledge"	; break;
		case NETROM_INFO	: op_text_ptr = "Information"			; break;
		case NETROM_INFOACK	: op_text_ptr = "Information acknowledge"	; break;
		default			: op_text_ptr = "Unknown"			; break;
		}

	col_add_fstr( pinfo->cinfo, COL_INFO, "%s", op_text_ptr );

	if ( tree )
		{
		/* create display subtree for the protocol */

		ti = proto_tree_add_protocol_format( tree, proto_netrom, tvb, 0, NETROM_HEADER_SIZE,
			"Net/ROM, Src: %s (%s), Dst: %s (%s)",
			get_ax25_name( src_addr ),
			ax25_to_str( src_addr ),
			get_ax25_name( dst_addr ),
			ax25_to_str( dst_addr ) );

		netrom_tree = proto_item_add_subtree( ti, ett_netrom );

		offset = 0;

		/* source */
		proto_tree_add_ax25( netrom_tree, hf_netrom_src, tvb, offset, AX25_ADDR_LEN, src_addr );
		offset += AX25_ADDR_LEN;

		/* destination */
		proto_tree_add_ax25( netrom_tree, hf_netrom_dst, tvb, offset, AX25_ADDR_LEN, dst_addr );
		offset += AX25_ADDR_LEN;

		/* ttl */
		proto_tree_add_item( netrom_tree, hf_netrom_ttl, tvb, offset, 1, FALSE );
		offset += 1;

		switch ( op_code )
			{
			case NETROM_PROTOEXT	:
						/* cct index */
						proto_tree_add_item( netrom_tree, hf_netrom_my_cct_index, tvb, offset, 1, FALSE );
						offset += 1;

						/* cct id */
						proto_tree_add_item( netrom_tree, hf_netrom_my_cct_id, tvb, offset, 1, FALSE );
						offset += 1;

						/* unused */
						offset += 1;

						/* unused */
						offset += 1;
						break;
			case NETROM_CONNREQ	:
						/* cct index */
						proto_tree_add_item( netrom_tree, hf_netrom_my_cct_index, tvb, offset, 1, FALSE );
						offset += 1;

						/* cct id */
						proto_tree_add_item( netrom_tree, hf_netrom_my_cct_id, tvb, offset, 1, FALSE );
						offset += 1;

						/* unused */
						offset += 1;

						/* unused */
						offset += 1;

						break;
			case NETROM_CONNACK	:
						/* your cct index */
						proto_tree_add_item( netrom_tree, hf_netrom_your_cct_index, tvb, offset, 1, FALSE );
						offset += 1;

						/* your cct id */
						proto_tree_add_item( netrom_tree, hf_netrom_your_cct_id, tvb, offset, 1, FALSE );
						offset += 1;

						/* my cct index */
						proto_tree_add_item( netrom_tree, hf_netrom_my_cct_index, tvb, offset, 1, FALSE );
						offset += 1;

						/* my cct id */
						proto_tree_add_item( netrom_tree, hf_netrom_my_cct_id, tvb, offset, 1, FALSE );
						offset += 1;

						break;
			case NETROM_DISCREQ	:
						/* your cct index */
						proto_tree_add_item( netrom_tree, hf_netrom_your_cct_index, tvb, offset, 1, FALSE );
						offset += 1;

						/* your cct id */
						proto_tree_add_item( netrom_tree, hf_netrom_your_cct_id, tvb, offset, 1, FALSE );
						offset += 1;

						/* unused */
						offset += 1;

						/* unused */
						offset += 1;

						break;
			case NETROM_DISCACK	:
						/* your cct index */
						proto_tree_add_item( netrom_tree, hf_netrom_your_cct_index, tvb, offset, 1, FALSE );
						offset += 1;

						/* your cct id */
						proto_tree_add_item( netrom_tree, hf_netrom_your_cct_id, tvb, offset, 1, FALSE );
						offset += 1;

						/* unused */
						offset += 1;

						/* unused */
						offset += 1;

						break;
			case NETROM_INFO	:
						/* your cct index */
						proto_tree_add_item( netrom_tree, hf_netrom_your_cct_index, tvb, offset, 1, FALSE );
						offset += 1;

						/* your cct id */
						proto_tree_add_item( netrom_tree, hf_netrom_your_cct_id, tvb, offset, 1, FALSE );
						offset += 1;

						/* n_s */
						proto_tree_add_item( netrom_tree, hf_netrom_n_s, tvb, offset, 1, FALSE );
						offset += 1;

						/* n_r */
						proto_tree_add_item( netrom_tree, hf_netrom_n_r, tvb, offset, 1, FALSE );
						offset += 1;

						break;
			case NETROM_INFOACK	:
						/* your cct index */
						proto_tree_add_item( netrom_tree, hf_netrom_your_cct_index, tvb, offset, 1, FALSE );
						offset += 1;

						/* your cct id */
						proto_tree_add_item( netrom_tree, hf_netrom_your_cct_id, tvb, offset, 1, FALSE );
						offset += 1;

						/* unused */
						offset += 1;

						/* n_r */
						proto_tree_add_item( netrom_tree, hf_netrom_n_r, tvb, offset, 1, FALSE );
						offset += 1;

						break;
			default			:
						offset += 1;
						offset += 1;
						offset += 1;
						offset += 1;

						break;
			}

		/* type */
		dissect_netrom_type(	tvb,
					offset,
					pinfo,
					netrom_tree,
					hf_netrom_type,
					ett_netrom_type,
					&netrom_type_items
					);
		offset += 1;

		switch ( op_code )
			{
			case NETROM_PROTOEXT	:
						break;
			case NETROM_CONNREQ	:
						/* proposed window size */
						proto_tree_add_item( netrom_tree, hf_netrom_pwindow, tvb, offset, 1, FALSE );
						offset += 1;

						user_addr = tvb_get_ptr( tvb,  offset, AX25_ADDR_LEN );
						proto_tree_add_ax25( netrom_tree, hf_netrom_user, tvb, offset, AX25_ADDR_LEN, user_addr );
						offset += AX25_ADDR_LEN;

						node_addr = tvb_get_ptr( tvb,  offset, AX25_ADDR_LEN );
						proto_tree_add_ax25( netrom_tree, hf_netrom_node, tvb, offset, AX25_ADDR_LEN, node_addr );
						offset += AX25_ADDR_LEN;

						break;
			case NETROM_CONNACK	:
						/* accepted window size */
						proto_tree_add_item( netrom_tree, hf_netrom_awindow, tvb, offset, 1, FALSE );
						offset += 1;

						break;
			case NETROM_DISCREQ	:
						break;
			case NETROM_DISCACK	:
						break;
			case NETROM_INFO	:
						break;
			case NETROM_INFOACK	:
						break;
			default			:
						break;
			}
		}

	/* Call sub-dissectors here */

	saved_private_data = pinfo->private_data;
	next_tvb = tvb_new_subset(tvb, offset, -1, -1);

	switch ( op_code )
		{
		case NETROM_PROTOEXT	:
					if ( cct_index == NETROM_PROTO_IP && cct_id == NETROM_PROTO_IP )
						call_dissector( ip_handle , next_tvb, pinfo, tree );
					else
						call_dissector( default_handle , next_tvb, pinfo, tree );

					break;
		case NETROM_INFO	:
		default			:
					call_dissector( default_handle , next_tvb, pinfo, tree );
					break;
		}

	pinfo->private_data = saved_private_data;
}

static void
dissect_netrom_routing(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	proto_item *ti;
	proto_tree *netrom_tree;
	int offset;
	void *saved_private_data;
	tvbuff_t *next_tvb = NULL;

	if ( check_col( pinfo->cinfo, COL_PROTOCOL ) )
		col_set_str( pinfo->cinfo, COL_PROTOCOL, "NetROM");

	if ( check_col( pinfo->cinfo, COL_INFO ) )
		col_clear( pinfo->cinfo, COL_INFO );

	if ( check_col( pinfo->cinfo, COL_INFO ) )
		col_set_str( pinfo->cinfo, COL_INFO, "routing table frame");

	if (tree)
		{
		ti = proto_tree_add_protocol_format( tree, proto_netrom, tvb, 0, -1,
			"Net/ROM, routing table frame, Node: %.6s",
			tvb_get_ptr( tvb,  1, 6 )
			 );

		netrom_tree = proto_item_add_subtree( ti, ett_netrom );

		offset = 1;

		proto_tree_add_item( netrom_tree, hf_netrom_mnemonic, tvb, offset, 6, FALSE );
		offset += 6;

		saved_private_data = pinfo->private_data;
		next_tvb = tvb_new_subset(tvb, offset, -1, -1);

		call_dissector( default_handle , next_tvb, pinfo, tree );

		pinfo->private_data = saved_private_data;
		}

}

/* Code to actually dissect the packets */
static void
dissect_netrom(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	if ( tvb_get_guint8( tvb, 0 ) == 0xff )
		dissect_netrom_routing( tvb, pinfo, tree );
	else
		dissect_netrom_proto( tvb, pinfo, tree );
}

void
proto_register_netrom(void)
{
	static const true_false_string flags_set_truth =
		{
		"Set",
		"Not set"
		};


	/* Setup list of header fields */
	static hf_register_info hf[] = {
		{ &hf_netrom_src,
			{ "Source",			"netrom.src",
			FT_AX25, BASE_NONE, NULL, 0x0,
			"Source callsign", HFILL }
		},
		{ &hf_netrom_dst,
			{ "Destination",		"netrom.dst",
			FT_AX25, BASE_NONE, NULL, 0x0,
			"Destination callsign", HFILL }
		},
		{ &hf_netrom_ttl,
			{ "TTL",			"netrom.ttl",
			FT_UINT8, BASE_HEX, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_netrom_my_cct_index,
			{ "My circuit index",		"netrom.my.cct.index",
			FT_UINT8, BASE_HEX, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_netrom_my_cct_id,
			{ "My circuit ID",		"netrom.my.cct.id",
			FT_UINT8, BASE_HEX, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_netrom_your_cct_index,
			{ "Your circuit index",		"netrom.your.cct.index",
			FT_UINT8, BASE_HEX, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_netrom_your_cct_id,
			{ "Your circuit ID",		"netrom.your.cct.id",
			FT_UINT8, BASE_HEX, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_netrom_n_r,
			{ "N(r)",			"netrom.n_r",
			FT_UINT8, BASE_DEC, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_netrom_n_s,
			{ "N(s)",			"netrom.n_s",
			FT_UINT8, BASE_DEC, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_netrom_type,
			{ "Type",			"netrom.type",
			FT_UINT8, BASE_HEX, NULL, 0x0,
			"Packet type field", HFILL }
		},
		{ &hf_netrom_op,
			{ "OP code",			"netrom.op",
			FT_UINT8, BASE_HEX, VALS( op_copde_vals ), 0x0f,
			"Protocol operation code", HFILL }
		},
		{ &hf_netrom_more,
			{ "More",			"netrom.flag.more",
			FT_BOOLEAN, 8, TFS(&flags_set_truth), NETROM_MORE_FLAG,
			"More flag", HFILL }
		},
		{ &hf_netrom_nak,
			{ "NAK",			"netrom.flag.nak",
			FT_BOOLEAN, 8, TFS(&flags_set_truth), NETROM_NAK_FLAG,
			"NAK flag", HFILL }
		},
		{ &hf_netrom_choke,
			{ "Choke",			"netrom.flag.choke",
			FT_BOOLEAN, 8, TFS(&flags_set_truth), NETROM_CHOKE_FLAG,
			"Choke flag", HFILL }
		},
		{ &hf_netrom_user,
			{ "User",			"netrom.user",
			FT_AX25, BASE_NONE, NULL, 0x0,
			"User callsign", HFILL }
		},
		{ &hf_netrom_node,
			{ "Node",			"netrom.node",
			FT_AX25, BASE_NONE, NULL, 0x0,
			"Node callsign", HFILL }
		},
		{ &hf_netrom_pwindow,
			{ "Window",			"netrom.pwindow",
			FT_UINT8, BASE_DEC, NULL, 0x0,
			"Proposed window", HFILL }
		},
		{ &hf_netrom_awindow,
			{ "Window",			"netrom.awindow",
			FT_UINT8, BASE_DEC, NULL, 0x0,
			"Accepted window", HFILL }
		},
		{ &hf_netrom_mnemonic,
			{ "Node name",			"netrom.name",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
	};

	/* Setup protocol subtree array */
	static gint *ett[] = {
		&ett_netrom,
		&ett_netrom_type,
	};

	/* Register the protocol name and description */
	proto_netrom = proto_register_protocol( "Amateur Radio Net/ROM", "Net/ROM", "netrom" );

	/* Register the dissector */
	register_dissector( "netrom", dissect_netrom, proto_netrom );

	/* Required function calls to register the header fields and subtrees used */
	proto_register_field_array( proto_netrom, hf, array_length(hf ) );
	proto_register_subtree_array( ett, array_length( ett ) );
}

void
proto_reg_handoff_netrom(void)
{
	static gboolean inited = FALSE;

	if( !inited ) {

		ip_handle  = find_dissector( "ip" );
		default_handle  = find_dissector( "data" );

		inited = TRUE;
	}
}

void
capture_netrom( const guchar *pd _U_, int offset, int len, packet_counts *ld)
{
	if ( ! BYTES_ARE_IN_FRAME( offset, len, NETROM_MIN_SIZE ) )
		{
		ld->other++;
		return;
		}
	/* XXX - check fr IP-over-NetROM here! */
	ld->other++;
}
