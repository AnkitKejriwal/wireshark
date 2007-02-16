/* packet-rtp.c
 *
 * Routines for RTP dissection
 * RTP = Real time Transport Protocol
 *
 * Copyright 2000, Philips Electronics N.V.
 * Written by Andreas Sikkema <h323@ramdyne.nl>
 *
 * $Id$
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/*
 * This dissector tries to dissect the RTP protocol according to Annex A
 * of ITU-T Recommendation H.225.0 (02/98) or RFC 1889
 *
 * RTP traffic is handled by an even UDP portnumber. This can be any
 * port number, but there is a registered port available, port 5004
 * See Annex B of ITU-T Recommendation H.225.0, section B.7
 *
 * This doesn't dissect older versions of RTP, such as:
 *
 *    the vat protocol ("version 0") - see
 *
 *	ftp://ftp.ee.lbl.gov/conferencing/vat/alpha-test/vatsrc-4.0b2.tar.gz
 *
 *    and look in "session-vat.cc" if you want to write a dissector
 *    (have fun - there aren't any nice header files showing the packet
 *    format);
 *
 *    version 1, as documented in
 *
 *	ftp://gaia.cs.umass.edu/pub/hgschulz/rtp/draft-ietf-avt-rtp-04.txt
 *
 * It also dissects PacketCable CCC-encapsulated RTP data, as described in
 * chapter 5 of the PacketCable Electronic Surveillance Specification:
 *
 *   http://www.packetcable.com/downloads/specs/PKT-SP-ESP1.5-I01-050128.pdf
 */


#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glib.h>
#include <epan/packet.h>

#include <stdio.h>
#include <string.h>

#include "packet-rtp.h"
#include <epan/rtp_pt.h>
#include "packet-ntp.h"
#include <epan/conversation.h>
#include <epan/tap.h>

#include <epan/prefs.h>
#include <epan/emem.h>

typedef struct _rfc2198_hdr {
  guint8 pt;
  int offset;
  int len;
  struct _rfc2198_hdr *next;
} rfc2198_hdr;

static dissector_handle_t rtp_handle;
static dissector_handle_t rtp_rfc2198_handle;
static dissector_handle_t stun_handle;
static dissector_handle_t t38_handle;

static dissector_handle_t pkt_ccc_handle;

static int rtp_tap = -1;

static dissector_table_t rtp_pt_dissector_table;
static dissector_table_t rtp_dyn_pt_dissector_table;

/* RTP header fields             */
static int proto_rtp           = -1;
static int hf_rtp_version      = -1;
static int hf_rtp_padding      = -1;
static int hf_rtp_extension    = -1;
static int hf_rtp_csrc_count   = -1;
static int hf_rtp_marker       = -1;
static int hf_rtp_payload_type = -1;
static int hf_rtp_seq_nr       = -1;
static int hf_rtp_timestamp    = -1;
static int hf_rtp_ssrc         = -1;
static int hf_rtp_csrc_item    = -1;
static int hf_rtp_data         = -1;
static int hf_rtp_padding_data = -1;
static int hf_rtp_padding_count= -1;
static int hf_rtp_rfc2198_follow= -1;
static int hf_rtp_rfc2198_tm_off= -1;
static int hf_rtp_rfc2198_bl_len= -1;

/* RTP header extension fields   */
static int hf_rtp_prof_define  = -1;
static int hf_rtp_length       = -1;
static int hf_rtp_hdr_ext      = -1;

/* RTP setup fields */
static int hf_rtp_setup        = -1;
static int hf_rtp_setup_frame  = -1;
static int hf_rtp_setup_method = -1;

/* RTP fields defining a sub tree */
static gint ett_rtp       = -1;
static gint ett_csrc_list = -1;
static gint ett_hdr_ext   = -1;
static gint ett_rtp_setup = -1;
static gint ett_rtp_rfc2198 = -1;
static gint ett_rtp_rfc2198_hdr = -1;


/* PacketCable CCC header fields */
static int proto_pkt_ccc       = -1;
static int hf_pkt_ccc_id       = -1;
static int hf_pkt_ccc_ts       = -1;

/* PacketCable CCC field defining a sub tree */
static gint ett_pkt_ccc = -1;

/* PacketCable CCC port preference */
static gboolean global_pkt_ccc_udp_port = 0;


#define RTP0_INVALID 0
#define RTP0_STUN    1
#define RTP0_T38     2

static enum_val_t rtp_version0_types[] = {
	{ "invalid", "Invalid RTP packets", RTP0_INVALID },
	{ "stun", "STUN packets", RTP0_STUN },
	{ "t38", "T.38 packets", RTP0_T38 },
	{ NULL, NULL, 0 }
};
static guint global_rtp_version0_type = 0;

static dissector_handle_t data_handle;

/* Forward declaration we need below */
void proto_reg_handoff_rtp(void);

static gboolean dissect_rtp_heur( tvbuff_t *tvb, packet_info *pinfo,
    proto_tree *tree );
static void dissect_rtp( tvbuff_t *tvb, packet_info *pinfo,
    proto_tree *tree );
static void show_setup_info(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);
static void get_conv_info(packet_info *pinfo, struct _rtp_info *rtp_info);

/* Preferences bool to control whether or not setup info should be shown */
static gboolean global_rtp_show_setup_info = TRUE;

/* Try heuristic RTP decode */
static gboolean global_rtp_heur = FALSE;

/* RFC2198 Redundant Audio Data */
static guint rtp_rfc2198_pt = 99;
static guint rtp_saved_rfc2198_pt = 0;

/*
 * Fields in the first octet of the RTP header.
 */

/* Version is the first 2 bits of the first octet*/
#define RTP_VERSION(octet)	((octet) >> 6)

/* Padding is the third bit; No need to shift, because true is any value
   other than 0! */
#define RTP_PADDING(octet)	((octet) & 0x20)

/* Extension bit is the fourth bit */
#define RTP_EXTENSION(octet)	((octet) & 0x10)

/* CSRC count is the last four bits */
#define RTP_CSRC_COUNT(octet)	((octet) & 0xF)

static const value_string rtp_version_vals[] =
{
	{ 0, "Old VAT Version" },
	{ 1, "First Draft Version" },
	{ 2, "RFC 1889 Version" },
	{ 0, NULL },
};

/*
 * Fields in the second octet of the RTP header.
 */

/* Marker is the first bit of the second octet */
#define RTP_MARKER(octet)	((octet) & 0x80)

/* Payload type is the last 7 bits */
#define RTP_PAYLOAD_TYPE(octet)	((octet) & 0x7F)

const value_string rtp_payload_type_vals[] =
{
	{ PT_PCMU,	"ITU-T G.711 PCMU" },
	{ PT_1016,	"USA Federal Standard FS-1016" },
	{ PT_G721,	"ITU-T G.721" },
	{ PT_GSM,	"GSM 06.10" },
	{ PT_G723,	"ITU-T G.723" },
	{ PT_DVI4_8000,	"DVI4 8000 samples/s" },
	{ PT_DVI4_16000, "DVI4 16000 samples/s" },
	{ PT_LPC,	"Experimental linear predictive encoding from Xerox PARC" },
	{ PT_PCMA,	"ITU-T G.711 PCMA" },
	{ PT_G722,	"ITU-T G.722" },
	{ PT_L16_STEREO, "16-bit uncompressed audio, stereo" },
	{ PT_L16_MONO,	"16-bit uncompressed audio, monaural" },
	{ PT_QCELP,	"Qualcomm Code Excited Linear Predictive coding" },
	{ PT_CN,	"Comfort noise" },
	{ PT_MPA,	"MPEG-I/II Audio"},
	{ PT_G728,	"ITU-T G.728" },
	{ PT_DVI4_11025, "DVI4 11025 samples/s" },
	{ PT_DVI4_22050, "DVI4 22050 samples/s" },
	{ PT_G729,	"ITU-T G.729" },
	{ PT_CN_OLD,	"Comfort noise (old)" },
	{ PT_CELB,	"Sun CellB video encoding" },
	{ PT_JPEG,	"JPEG-compressed video" },
	{ PT_NV,	"'nv' program" },
	{ PT_H261,	"ITU-T H.261" },
	{ PT_MPV,	"MPEG-I/II Video"},
	{ PT_MP2T,	"MPEG-II transport streams"},
	{ PT_H263,	"ITU-T H.263" },
	{ 0,		NULL },
};

const value_string rtp_payload_type_short_vals[] =
{
       { PT_PCMU,      "g711U" },
       { PT_1016,      "fs-1016" },
       { PT_G721,      "g721" },
       { PT_GSM,       "GSM" },
       { PT_G723,      "g723" },
       { PT_DVI4_8000, "DVI4 8k" },
       { PT_DVI4_16000, "DVI4 16k" },
       { PT_LPC,       "Exp. from Xerox PARC" },
       { PT_PCMA,      "g711A" },
       { PT_G722,      "g722" },
       { PT_L16_STEREO, "16-bit audio, stereo" },
       { PT_L16_MONO,  "16-bit audio, monaural" },
       { PT_QCELP,     "Qualcomm" },
       { PT_CN,        "CN" },
       { PT_MPA,       "MPEG-I/II Audio"},
       { PT_G728,      "g728" },
       { PT_DVI4_11025, "DVI4 11k" },
       { PT_DVI4_22050, "DVI4 22k" },
       { PT_G729,      "g729" },
       { PT_CN_OLD,    "CN(old)" },
       { PT_CELB,      "CellB" },
       { PT_JPEG,      "JPEG" },
       { PT_NV,        "NV" },
       { PT_H261,      "h261" },
       { PT_MPV,       "MPEG-I/II Video"},
       { PT_MP2T,      "MPEG-II streams"},
       { PT_H263,      "h263" },
       { 0,            NULL },
};

void
rtp_free_hash_dyn_payload(GHashTable *rtp_dyn_payload)
{
	if (rtp_dyn_payload == NULL) return;
	g_hash_table_destroy(rtp_dyn_payload);
	rtp_dyn_payload = NULL;
}

/* Set up an RTP conversation */
void rtp_add_address(packet_info *pinfo,
                     address *addr, int port,
                     int other_port,
                     const gchar *setup_method, guint32 setup_frame_number, GHashTable *rtp_dyn_payload)
{
	address null_addr;
	conversation_t* p_conv;
	struct _rtp_conversation_info *p_conv_data = NULL;

	/*
	 * If this isn't the first time this packet has been processed,
	 * we've already done this work, so we don't need to do it
	 * again.
	 */
	if (pinfo->fd->flags.visited)
	{
		return;
	}

	SET_ADDRESS(&null_addr, AT_NONE, 0, NULL);

	/*
	 * Check if the ip address and port combination is not
	 * already registered as a conversation.
	 */
	p_conv = find_conversation( setup_frame_number, addr, &null_addr, PT_UDP, port, other_port,
                                NO_ADDR_B | (!other_port ? NO_PORT_B : 0));

	/*
	 * If not, create a new conversation.
	 */
	if ( !p_conv || p_conv->setup_frame != setup_frame_number) {
		p_conv = conversation_new( setup_frame_number, addr, &null_addr, PT_UDP,
		                           (guint32)port, (guint32)other_port,
								   NO_ADDR2 | (!other_port ? NO_PORT2 : 0));
	}

	/* Set dissector */
	conversation_set_dissector(p_conv, rtp_handle);

	/*
	 * Check if the conversation has data associated with it.
	 */
	p_conv_data = conversation_get_proto_data(p_conv, proto_rtp);

	/*
	 * If not, add a new data item.
	 */
	if ( ! p_conv_data ) {
		/* Create conversation data */
		p_conv_data = se_alloc(sizeof(struct _rtp_conversation_info));
		p_conv_data->rtp_dyn_payload = NULL;

		conversation_add_proto_data(p_conv, proto_rtp, p_conv_data);
	}

	/*
	 * Update the conversation data.
	 */
	/* Free the hash if already exists */
	rtp_free_hash_dyn_payload(p_conv_data->rtp_dyn_payload);

	strncpy(p_conv_data->method, setup_method, MAX_RTP_SETUP_METHOD_SIZE);
	p_conv_data->method[MAX_RTP_SETUP_METHOD_SIZE] = '\0';
	p_conv_data->frame_number = setup_frame_number;
	p_conv_data->rtp_dyn_payload = rtp_dyn_payload;
}

static gboolean
dissect_rtp_heur( tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree )
{
	guint8      octet1, octet2;
 	unsigned int version;
	unsigned int payload_type;
 	unsigned int offset = 0;

	/* This is a heuristic dissector, which means we get all the UDP
	 * traffic not sent to a known dissector and not claimed by
	 * a heuristic dissector called before us!
	 */

	if (! global_rtp_heur)
		return FALSE;

	/* Get the fields in the first octet */
	octet1 = tvb_get_guint8( tvb, offset );
	version = RTP_VERSION( octet1 );

	if (version == 0) {
		switch (global_rtp_version0_type) {
		case RTP0_STUN:
			call_dissector(stun_handle, tvb, pinfo, tree);
			return TRUE;

		case RTP0_T38:
			call_dissector(t38_handle, tvb, pinfo, tree);
			return TRUE;

		case RTP0_INVALID:
		default:
			return FALSE; /* Unknown or unsupported version */
		}
	} else if (version != 2) {
		/* Unknown or unsupported version */
		return FALSE;
	}

	/* Get the fields in the second octet */
	octet2 = tvb_get_guint8( tvb, offset + 1 );
	payload_type = RTP_PAYLOAD_TYPE( octet2 );
	/*      if (payload_type == PT_PCMU ||
	 *		     payload_type == PT_PCMA)
	 *	     payload_type == PT_G729)
	 *	 */
	if (payload_type <= PT_H263) {
 		dissect_rtp( tvb, pinfo, tree );
		return TRUE;
	}
	else {
 		return FALSE;
	}
}

static void
dissect_rtp_data( tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
    proto_tree *rtp_tree, int offset, unsigned int data_len,
    unsigned int data_reported_len, unsigned int payload_type )
{
	tvbuff_t *newtvb;
	struct _rtp_conversation_info *p_conv_data = NULL;
	gboolean found_match = FALSE;

	newtvb = tvb_new_subset( tvb, offset, data_len, data_reported_len );

	/* if the payload type is dynamic (96 to 127), we check if the conv is set and we look for the pt definition */
	if ( (payload_type >=96) && (payload_type <=127) ) {
		p_conv_data = p_get_proto_data(pinfo->fd, proto_rtp);
		if (p_conv_data && p_conv_data->rtp_dyn_payload) {
			gchar *payload_type_str = NULL;
			payload_type_str = g_hash_table_lookup(p_conv_data->rtp_dyn_payload, &payload_type);
			if (payload_type_str){
				found_match = dissector_try_string(rtp_dyn_pt_dissector_table,
													payload_type_str, newtvb, pinfo, tree);
				/* If payload type string set from conversation and
				 * no matching dissector found it's probably because no subdissector
				 * exists. Don't call the dissectors based on payload number
				 * as that'd probably be the wrong dissector in this case.
				 * Just add it as data.
				 */
				if(found_match==FALSE)
					proto_tree_add_item( rtp_tree, hf_rtp_data, newtvb, 0, -1, FALSE );
				return;
			}

		}
	}

	/* if we don't found, it is static OR could be set static from the preferences */
	if (!dissector_try_port(rtp_pt_dissector_table, payload_type, newtvb, pinfo, tree))
		proto_tree_add_item( rtp_tree, hf_rtp_data, newtvb, 0, -1, FALSE );

}

static void
dissect_rtp_rfc2198(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree )
{
	int offset = 0;
	guint8 octet1;
	int cnt;
	gboolean hdr_follow = TRUE;
	proto_item *ti = NULL;
	proto_tree *rfc2198_tree = NULL;
	proto_tree *rfc2198_hdr_tree = NULL;
	rfc2198_hdr *hdr_last, *hdr_new;
	rfc2198_hdr *hdr_chain = NULL;

	/* Add try to RFC2198 data */
	ti = proto_tree_add_text(tree, tvb, offset, -1, "RFC2198: Redundant Audio Data");
	rfc2198_tree = proto_item_add_subtree(ti, ett_rtp_rfc2198);

	hdr_last = NULL;
	cnt = 0;
	while (hdr_follow) {
		cnt++;

		/* Allocate and fill in header */
		hdr_new = ep_alloc(sizeof(rfc2198_hdr));
		hdr_new->next = NULL;
		octet1 = tvb_get_guint8(tvb, offset);
		hdr_new->pt = RTP_PAYLOAD_TYPE(octet1);
		hdr_follow = (octet1 & 0x80);

		/* Add a subtree for this header and add items */
		ti = proto_tree_add_text(rfc2198_tree, tvb, offset, (hdr_follow)?4:1, "Header %u", cnt);
		rfc2198_hdr_tree = proto_item_add_subtree(ti, ett_rtp_rfc2198_hdr);
		proto_tree_add_item(rfc2198_hdr_tree, hf_rtp_rfc2198_follow, tvb, offset, 1, FALSE );
		proto_tree_add_item(rfc2198_hdr_tree, hf_rtp_payload_type, tvb, offset, 1, FALSE );
		proto_item_append_text(ti, ": PT=%s", val_to_str(hdr_new->pt, rtp_payload_type_vals, "Unknown (%u)"));
		offset += 1;

		/* Timestamp offset and block length don't apply to last header */
		if (hdr_follow) {
			proto_tree_add_item(rfc2198_hdr_tree, hf_rtp_rfc2198_tm_off, tvb, offset, 2, FALSE );
			proto_tree_add_item(rfc2198_hdr_tree, hf_rtp_rfc2198_bl_len, tvb, offset + 1, 2, FALSE );
			hdr_new->len = tvb_get_ntohs(tvb, offset + 1) & 0x03FF;
			proto_item_append_text(ti, ", len=%u", hdr_new->len);
			offset += 3;
		} else {
			hdr_new->len = -1;
			hdr_follow = FALSE;
		}

		if (hdr_last) {
			hdr_last->next = hdr_new;
		} else {
			hdr_chain = hdr_new;
		}
		hdr_last = hdr_new;
	}

	/* Dissect each data block according to the header info */
	hdr_last = hdr_chain;
	while (hdr_last) {
		hdr_last->offset = offset;
		if (!hdr_last->next) {
			hdr_last->len = tvb_reported_length_remaining(tvb, offset);
		}
		dissect_rtp_data(tvb, pinfo, tree, rfc2198_tree, hdr_last->offset, hdr_last->len, hdr_last->len, hdr_last->pt);
		offset += hdr_last->len;
		hdr_last = hdr_last->next;
	}
}

static void
dissect_rtp( tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree )
{
	proto_item *ti            = NULL;
	proto_tree *rtp_tree      = NULL;
	proto_tree *rtp_csrc_tree = NULL;
	guint8      octet1, octet2;
	unsigned int version;
	gboolean    padding_set;
	gboolean    extension_set;
	unsigned int csrc_count;
	gboolean    marker_set;
	unsigned int payload_type;
	gchar *payload_type_str = NULL;
	unsigned int i            = 0;
	unsigned int hdr_extension= 0;
	unsigned int padding_count;
	gint        length, reported_length;
	int         data_len;
	unsigned int offset = 0;
	guint16     seq_num;
	guint32     timestamp;
	guint32     sync_src;
	guint32     csrc_item;
	struct _rtp_conversation_info *p_conv_data = NULL;

	/* Can tap up to 4 RTP packets within same packet */
	static struct _rtp_info rtp_info_arr[4];
	static int rtp_info_current=0;
	struct _rtp_info *rtp_info;

	rtp_info_current++;
	if (rtp_info_current==4) {
		rtp_info_current=0;
	}
	rtp_info = &rtp_info_arr[rtp_info_current];

	/* Get the fields in the first octet */
	octet1 = tvb_get_guint8( tvb, offset );
	version = RTP_VERSION( octet1 );

	if (version == 0) {
		switch (global_rtp_version0_type) {
		case RTP0_STUN:
			call_dissector(stun_handle, tvb, pinfo, tree);
			return;

		case RTP0_T38:
			call_dissector(t38_handle, tvb, pinfo, tree);
			return;

		case RTP0_INVALID:
		default:
			; /* Unknown or unsupported version (let it fall through */
		}
	}

	/* fill in the rtp_info structure */
	rtp_info->info_version = version;
	if (version != 2) {
		/*
		 * Unknown or unsupported version.
		 */
		if ( check_col( pinfo->cinfo, COL_PROTOCOL ) )   {
			col_set_str( pinfo->cinfo, COL_PROTOCOL, "RTP" );
		}

		if ( check_col( pinfo->cinfo, COL_INFO) ) {
			col_add_fstr( pinfo->cinfo, COL_INFO,
			    "Unknown RTP version %u", version);
		}

		if ( tree ) {
			ti = proto_tree_add_item( tree, proto_rtp, tvb, offset, -1, FALSE );
			rtp_tree = proto_item_add_subtree( ti, ett_rtp );

			proto_tree_add_uint( rtp_tree, hf_rtp_version, tvb,
			    offset, 1, octet1);
		}
		return;
	}

	padding_set = RTP_PADDING( octet1 );
	extension_set = RTP_EXTENSION( octet1 );
	csrc_count = RTP_CSRC_COUNT( octet1 );

	/* Get the fields in the second octet */
	octet2 = tvb_get_guint8( tvb, offset + 1 );
	marker_set = RTP_MARKER( octet2 );
	payload_type = RTP_PAYLOAD_TYPE( octet2 );

	/* Get the subsequent fields */
	seq_num = tvb_get_ntohs( tvb, offset + 2 );
	timestamp = tvb_get_ntohl( tvb, offset + 4 );
	sync_src = tvb_get_ntohl( tvb, offset + 8 );

	/* fill in the rtp_info structure */
	rtp_info->info_padding_set = padding_set;
	rtp_info->info_padding_count = 0;
	rtp_info->info_marker_set = marker_set;
	rtp_info->info_payload_type = payload_type;
	rtp_info->info_seq_num = seq_num;
	rtp_info->info_timestamp = timestamp;
	rtp_info->info_sync_src = sync_src;
	rtp_info->info_setup_frame_num = 0;
	rtp_info->info_payload_type_str = NULL;

	/*
	 * Do we have all the data?
	 */
	length = tvb_length_remaining(tvb, offset);
	reported_length = tvb_reported_length_remaining(tvb, offset);
	if (reported_length >= 0 && length >= reported_length) {
		/*
		 * Yes.
		 */
		rtp_info->info_all_data_present = TRUE;
		rtp_info->info_data_len = reported_length;

		/*
		 * Save the pointer to raw rtp data (header + payload incl.
		 * padding).
		 * That should be safe because the "epan_dissect_t"
		 * constructed for the packet has not yet been freed when
		 * the taps are called.
		 * (Destroying the "epan_dissect_t" will end up freeing
		 * all the tvbuffs and hence invalidating pointers to
		 * their data.)
		 * See "add_packet_to_packet_list()" for details.
		 */
		rtp_info->info_data = tvb_get_ptr(tvb, 0, -1);
	} else {
		/*
		 * No - packet was cut short at capture time.
		 */
		rtp_info->info_all_data_present = FALSE;
		rtp_info->info_data_len = 0;
		rtp_info->info_data = NULL;
	}

	/* Look for conv and add to the frame if found */
	get_conv_info(pinfo, rtp_info);

	if ( check_col( pinfo->cinfo, COL_PROTOCOL ) )   {
		col_set_str( pinfo->cinfo, COL_PROTOCOL, "RTP" );
	}

	/* if it is dynamic payload, let use the conv data to see if it is defined */
	if ( (payload_type>95) && (payload_type<128) ) {
		/* Use existing packet info if available */
		p_conv_data = p_get_proto_data(pinfo->fd, proto_rtp);
		if (p_conv_data && p_conv_data->rtp_dyn_payload){
			payload_type_str = g_hash_table_lookup(p_conv_data->rtp_dyn_payload, &payload_type);
			rtp_info->info_payload_type_str = payload_type_str;
		}
	}

	if ( check_col( pinfo->cinfo, COL_INFO) ) {
		col_add_fstr( pinfo->cinfo, COL_INFO,
		    "Payload type=%s, SSRC=%u, Seq=%u, Time=%u%s",
			payload_type_str ? payload_type_str : val_to_str( payload_type, rtp_payload_type_vals,"Unknown (%u)" ),
		    sync_src,
		    seq_num,
		    timestamp,
		    marker_set ? ", Mark" : "");
	}


	if ( tree ) {
		proto_tree *item;
		/* Create RTP protocol tree */
		ti = proto_tree_add_item(tree, proto_rtp, tvb, offset, -1, FALSE );
		rtp_tree = proto_item_add_subtree(ti, ett_rtp );

		/* Conversation setup info */
		if (global_rtp_show_setup_info)
		{
			show_setup_info(tvb, pinfo, rtp_tree);
		}

		proto_tree_add_uint( rtp_tree, hf_rtp_version, tvb,
		    offset, 1, octet1 );
		proto_tree_add_boolean( rtp_tree, hf_rtp_padding, tvb,
		    offset, 1, octet1 );
		proto_tree_add_boolean( rtp_tree, hf_rtp_extension, tvb,
		    offset, 1, octet1 );
		proto_tree_add_uint( rtp_tree, hf_rtp_csrc_count, tvb,
		    offset, 1, octet1 );
		offset++;

		proto_tree_add_boolean( rtp_tree, hf_rtp_marker, tvb, offset,
		    1, octet2 );

		item = proto_tree_add_uint_format( rtp_tree, hf_rtp_payload_type, tvb,
		    offset, 1, octet2, "Payload type: %s (%u)",
			payload_type_str ? payload_type_str : val_to_str( payload_type, rtp_payload_type_vals,"Unknown"),
			payload_type);

		offset++;

		/* Sequence number 16 bits (2 octets) */
		proto_tree_add_uint( rtp_tree, hf_rtp_seq_nr, tvb, offset, 2, seq_num );
		offset += 2;

		/* Timestamp 32 bits (4 octets) */
		proto_tree_add_uint( rtp_tree, hf_rtp_timestamp, tvb, offset, 4, timestamp );
		offset += 4;

		/* Synchronization source identifier 32 bits (4 octets) */
		proto_tree_add_uint( rtp_tree, hf_rtp_ssrc, tvb, offset, 4, sync_src );
		offset += 4;
	} else {
		offset += 12;
	}
	/* CSRC list*/
	if ( csrc_count > 0 ) {
		if ( tree ) {
			ti = proto_tree_add_text(rtp_tree, tvb, offset, csrc_count * 4, "Contributing Source identifiers");
			rtp_csrc_tree = proto_item_add_subtree( ti, ett_csrc_list );
		}
		for (i = 0; i < csrc_count; i++ ) {
			csrc_item = tvb_get_ntohl( tvb, offset );
			if ( tree ) proto_tree_add_uint_format( rtp_csrc_tree,
			    hf_rtp_csrc_item, tvb, offset, 4,
			    csrc_item,
			    "CSRC item %d: %u",
			    i, csrc_item );
			offset += 4;
		}
	}

	/* Optional RTP header extension */
	if ( extension_set ) {
		/* Defined by profile field is 16 bits (2 octets) */
		if ( tree ) proto_tree_add_uint( rtp_tree, hf_rtp_prof_define, tvb, offset, 2, tvb_get_ntohs( tvb, offset ) );
		offset += 2;

		hdr_extension = tvb_get_ntohs( tvb, offset );
		if ( tree ) proto_tree_add_uint( rtp_tree, hf_rtp_length, tvb,
		    offset, 2, hdr_extension);
		offset += 2;
		if ( hdr_extension > 0 ) {
			if ( tree ) {
				ti = proto_tree_add_text(rtp_tree, tvb, offset, hdr_extension * 4, "Header extensions");
				/* I'm re-using the old tree variable here
				   from the CSRC list!*/
				rtp_csrc_tree = proto_item_add_subtree( ti,
				    ett_hdr_ext );
			}
			for (i = 0; i < hdr_extension; i++ ) {
				if ( tree ) proto_tree_add_uint( rtp_csrc_tree, hf_rtp_hdr_ext, tvb, offset, 4, tvb_get_ntohl( tvb, offset ) );
				offset += 4;
			}
		}
	}

	if ( padding_set ) {
		/*
		 * This RTP frame has padding - find it.
		 *
		 * The padding count is found in the LAST octet of
		 * the packet; it contains the number of octets
		 * that can be ignored at the end of the packet.
		 */
		if (tvb_length(tvb) < tvb_reported_length(tvb)) {
			/*
			 * We don't *have* the last octet of the
			 * packet, so we can't get the padding
			 * count.
			 *
			 * Put an indication of that into the
			 * tree, and just put in a raw data
			 * item.
			 */
			if ( tree ) proto_tree_add_text(rtp_tree, tvb, 0, 0,
			    "Frame has padding, but not all the frame data was captured");
			call_dissector(data_handle,
			    tvb_new_subset(tvb, offset, -1, -1),
			    pinfo, rtp_tree);
			return;
		}

		padding_count = tvb_get_guint8( tvb,
		    tvb_reported_length( tvb ) - 1 );
		data_len =
		    tvb_reported_length_remaining( tvb, offset ) - padding_count;

		rtp_info->info_payload_offset = offset;
		rtp_info->info_payload_len = tvb_length_remaining(tvb, offset);
		rtp_info->info_padding_count = padding_count;

		if (data_len > 0) {
			/*
			 * There's data left over when you take out
			 * the padding; dissect it.
			 */
			dissect_rtp_data( tvb, pinfo, tree, rtp_tree,
			    offset,
			    data_len,
			    data_len,
			    payload_type );
			offset += data_len;
		} else if (data_len < 0) {
			/*
			 * The padding count is bigger than the
			 * amount of RTP payload in the packet!
			 * Clip the padding count.
			 *
			 * XXX - put an item in the tree to indicate
			 * that the padding count is bogus?
			 */
			padding_count =
			    tvb_reported_length_remaining(tvb, offset);
		}
		if (padding_count > 1) {
			/*
			 * There's more than one byte of padding;
			 * show all but the last byte as padding
			 * data.
			 */
			if ( tree ) proto_tree_add_item( rtp_tree, hf_rtp_padding_data,
			    tvb, offset, padding_count - 1, FALSE );
			offset += padding_count - 1;
		}
		/*
		 * Show the last byte in the PDU as the padding
		 * count.
		 */
		if ( tree ) proto_tree_add_item( rtp_tree, hf_rtp_padding_count,
		    tvb, offset, 1, FALSE );
	}
	else {
		/*
		 * No padding.
		 */
		dissect_rtp_data( tvb, pinfo, tree, rtp_tree, offset,
		    tvb_length_remaining( tvb, offset ),
		    tvb_reported_length_remaining( tvb, offset ),
		    payload_type );
		rtp_info->info_payload_offset = offset;
		rtp_info->info_payload_len = tvb_length_remaining(tvb, offset);
	}
	if (!pinfo->in_error_pkt)
		tap_queue_packet(rtp_tap, pinfo, rtp_info);
}

/* Look for conversation info */
static void get_conv_info(packet_info *pinfo, struct _rtp_info *rtp_info)
{
	/* Conversation and current data */
	conversation_t *p_conv = NULL;
	struct _rtp_conversation_info *p_conv_data = NULL;

	/* Use existing packet info if available */
	p_conv_data = p_get_proto_data(pinfo->fd, proto_rtp);

	if (!p_conv_data)
	{
		/* First time, get info from conversation */
		p_conv = find_conversation(pinfo->fd->num, &pinfo->net_dst, &pinfo->net_src,
		                           pinfo->ptype,
		                           pinfo->destport, pinfo->srcport, NO_ADDR_B);
		if (p_conv)
		{
			/* Create space for packet info */
			struct _rtp_conversation_info *p_conv_packet_data;
			p_conv_data = conversation_get_proto_data(p_conv, proto_rtp);

			if (p_conv_data) {
				/* Save this conversation info into packet info */
				p_conv_packet_data = se_alloc(sizeof(struct _rtp_conversation_info));
				g_snprintf(p_conv_packet_data->method, MAX_RTP_SETUP_METHOD_SIZE, "%s", p_conv_data->method);
				p_conv_packet_data->method[MAX_RTP_SETUP_METHOD_SIZE]=0;
				p_conv_packet_data->frame_number = p_conv_data->frame_number;
				p_conv_packet_data->rtp_dyn_payload = p_conv_data->rtp_dyn_payload;
				p_add_proto_data(pinfo->fd, proto_rtp, p_conv_packet_data);
			}
		}
	}
	if (p_conv_data) rtp_info->info_setup_frame_num = p_conv_data->frame_number;
}


/* Display setup info */
static void show_setup_info(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	/* Conversation and current data */
	struct _rtp_conversation_info *p_conv_data = NULL;
		proto_tree *rtp_setup_tree;
	proto_item *ti;

	/* Use existing packet info if available */
	p_conv_data = p_get_proto_data(pinfo->fd, proto_rtp);

	if (!p_conv_data) return;

	/* Create setup info subtree with summary info. */
	ti =  proto_tree_add_string_format(tree, hf_rtp_setup, tvb, 0, 0,
		                                               "",
		                                               "Stream setup by %s (frame %u)",
		                                               p_conv_data->method,
		                                               p_conv_data->frame_number);
		PROTO_ITEM_SET_GENERATED(ti);
		rtp_setup_tree = proto_item_add_subtree(ti, ett_rtp_setup);
		if (rtp_setup_tree)
		{
			/* Add details into subtree */
			proto_item* item = proto_tree_add_uint(rtp_setup_tree, hf_rtp_setup_frame,
			                                       tvb, 0, 0, p_conv_data->frame_number);
			PROTO_ITEM_SET_GENERATED(item);
			item = proto_tree_add_string(rtp_setup_tree, hf_rtp_setup_method,
			                             tvb, 0, 0, p_conv_data->method);
			PROTO_ITEM_SET_GENERATED(item);
		}
}

/* Dissect PacketCable CCC header */

static void
dissect_pkt_ccc(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	proto_item *ti            = NULL;
	proto_tree *pkt_ccc_tree      = NULL;
 	const guint8 *ptime = tvb_get_ptr(tvb, 4, 8);

	if ( tree ) {
		ti = proto_tree_add_item(tree, proto_pkt_ccc, tvb, 0, 12, FALSE);
		pkt_ccc_tree = proto_item_add_subtree(ti, ett_pkt_ccc);

		proto_tree_add_item(pkt_ccc_tree, hf_pkt_ccc_id, tvb, 0, 4, FALSE);
		proto_tree_add_bytes_format(pkt_ccc_tree, hf_pkt_ccc_ts, tvb,
		    4, 8, "NTP timestamp: %s", ntp_fmt_ts(ptime));
	}

	dissect_rtp(tvb, pinfo, tree);
}


/* Register PacketCable CCC */

void
proto_register_pkt_ccc(void)
{
	static hf_register_info hf[] =
	{
		{
			&hf_pkt_ccc_id,
			{
				"PacketCable CCC Identifier",
				"pkt_ccc.ccc_id",
				FT_UINT32,
				BASE_DEC,
				NULL,
				0x0,
				"CCC_ID", HFILL
			}
		},
		{
			&hf_pkt_ccc_ts,
			{
				"PacketCable CCC Timestamp",
				"pkt_ccc.ts",
				FT_BYTES,
				BASE_NONE,
				NULL,
				0x0,
				"Timestamp", HFILL
			}
		},

	};

	static gint *ett[] =
	{
		&ett_pkt_ccc,
	};

	module_t *pkt_ccc_module;


	proto_pkt_ccc = proto_register_protocol("PacketCable Call Content Connection",
	    "PKT CCC", "pkt_ccc");
	proto_register_field_array(proto_pkt_ccc, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));

	register_dissector("pkt_ccc", dissect_pkt_ccc, proto_pkt_ccc);

	pkt_ccc_module = prefs_register_protocol(proto_pkt_ccc, NULL);

	prefs_register_uint_preference(pkt_ccc_module, "udp_port",
	                               "UDP port",
	                               "Decode packets on this UDP port as PacketCable CCC",
	                               10, &global_pkt_ccc_udp_port);
}

void
proto_reg_handoff_pkt_ccc(void)
{
	/*
	 * Register this dissector as one that can be selected by a
	 * UDP port number.
	 */
	pkt_ccc_handle = find_dissector("pkt_ccc");
	dissector_add_handle("udp.port", pkt_ccc_handle);
}

/* Register RTP */

void
proto_register_rtp(void)
{
	static hf_register_info hf[] =
	{
		{
			&hf_rtp_version,
			{
				"Version",
				"rtp.version",
				FT_UINT8,
				BASE_DEC,
				VALS(rtp_version_vals),
				0xC0,
				"", HFILL
			}
		},
		{
			&hf_rtp_padding,
			{
				"Padding",
				"rtp.padding",
				FT_BOOLEAN,
				8,
				NULL,
				0x20,
				"", HFILL
			}
		},
		{
			&hf_rtp_extension,
			{
				"Extension",
				"rtp.ext",
				FT_BOOLEAN,
				8,
				NULL,
				0x10,
				"", HFILL
			}
		},
		{
			&hf_rtp_csrc_count,
			{
				"Contributing source identifiers count",
				"rtp.cc",
				FT_UINT8,
				BASE_DEC,
				NULL,
				0x0F,
				"", HFILL
			}
		},
		{
			&hf_rtp_marker,
			{
				"Marker",
				"rtp.marker",
				FT_BOOLEAN,
				8,
				NULL,
				0x80,
				"", HFILL
			}
		},
		{
			&hf_rtp_payload_type,
			{
				"Payload type",
				"rtp.p_type",
				FT_UINT8,
				BASE_DEC,
				NULL,
				0x7F,
				"", HFILL
			}
		},
		{
			&hf_rtp_seq_nr,
			{
				"Sequence number",
				"rtp.seq",
				FT_UINT16,
				BASE_DEC,
				NULL,
				0x0,
				"", HFILL
			}
		},
		{
			&hf_rtp_timestamp,
			{
				"Timestamp",
				"rtp.timestamp",
				FT_UINT32,
				BASE_DEC,
				NULL,
				0x0,
				"", HFILL
			}
		},
		{
			&hf_rtp_ssrc,
			{
				"Synchronization Source identifier",
				"rtp.ssrc",
				FT_UINT32,
				BASE_DEC,
				NULL,
				0x0,
				"", HFILL
			}
		},
		{
			&hf_rtp_prof_define,
			{
				"Defined by profile",
				"rtp.ext.profile",
				FT_UINT16,
				BASE_DEC,
				NULL,
				0x0,
				"", HFILL
			}
		},
		{
			&hf_rtp_length,
			{
				"Extension length",
				"rtp.ext.len",
				FT_UINT16,
				BASE_DEC,
				NULL,
				0x0,
				"", HFILL
			}
		},
		{
			&hf_rtp_csrc_item,
			{
				"CSRC item",
				"rtp.csrc.item",
				FT_UINT32,
				BASE_DEC,
				NULL,
				0x0,
				"", HFILL
			}
		},
		{
			&hf_rtp_hdr_ext,
			{
				"Header extension",
				"rtp.hdr_ext",
				FT_UINT32,
				BASE_DEC,
				NULL,
				0x0,
				"", HFILL
			}
		},
		{
			&hf_rtp_data,
			{
				"Payload",
				"rtp.payload",
				FT_BYTES,
				BASE_HEX,
				NULL,
				0x0,
				"", HFILL
			}
		},
		{
			&hf_rtp_padding_data,
			{
				"Padding data",
				"rtp.padding.data",
				FT_BYTES,
				BASE_HEX,
				NULL,
				0x0,
				"", HFILL
			}
		},
		{
			&hf_rtp_padding_count,
			{
				"Padding count",
				"rtp.padding.count",
				FT_UINT8,
				BASE_DEC,
				NULL,
				0x0,
				"", HFILL
			}
		},
		{
			&hf_rtp_setup,
			{
				"Stream setup",
				"rtp.setup",
				FT_STRING,
				BASE_NONE,
				NULL,
				0x0,
				"Stream setup, method and frame number", HFILL
			}
		},
		{
			&hf_rtp_setup_frame,
			{
				"Setup frame",
				"rtp.setup-frame",
				FT_FRAMENUM,
				BASE_NONE,
				NULL,
				0x0,
				"Frame that set up this stream", HFILL
			}
		},
		{
			&hf_rtp_setup_method,
			{
				"Setup Method",
				"rtp.setup-method",
				FT_STRING,
				BASE_NONE,
				NULL,
				0x0,
				"Method used to set up this stream", HFILL
			}
		},
		{
			&hf_rtp_rfc2198_follow,
			{
				"Follow",
				"rtp.follow",
				FT_BOOLEAN,
				8,
				TFS(&flags_set_truth),
				0x80,
				"Next header follows", HFILL
			}
		},
		{
			&hf_rtp_rfc2198_tm_off,
			{
				"Timestamp offset",
				"rtp.timestamp-offset",
				FT_UINT16,
				BASE_DEC,
				NULL,
				0xFFFC,
				"Timestamp Offset", HFILL
			}
		},
		{
			&hf_rtp_rfc2198_bl_len,
			{
				"Block length",
				"rtp.block-length",
				FT_UINT16,
				BASE_DEC,
				NULL,
				0x03FF,
				"Block Length", HFILL
			}
		}

	};

	static gint *ett[] =
	{
		&ett_rtp,
		&ett_csrc_list,
		&ett_hdr_ext,
		&ett_rtp_setup,
		&ett_rtp_rfc2198,
		&ett_rtp_rfc2198_hdr
	};

	module_t *rtp_module;


	proto_rtp = proto_register_protocol("Real-Time Transport Protocol",
	    "RTP", "rtp");
	proto_register_field_array(proto_rtp, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));

	register_dissector("rtp", dissect_rtp, proto_rtp);
	register_dissector("rtp.rfc2198", dissect_rtp_rfc2198, proto_rtp);

	rtp_tap = register_tap("rtp");

	rtp_pt_dissector_table = register_dissector_table("rtp.pt",
	                                                  "RTP payload type", FT_UINT8, BASE_DEC);
	rtp_dyn_pt_dissector_table = register_dissector_table("rtp_dyn_payload_type",
												    "Dynamic RTP payload type", FT_STRING, BASE_NONE);


	rtp_module = prefs_register_protocol(proto_rtp, proto_reg_handoff_rtp);

	prefs_register_bool_preference(rtp_module, "show_setup_info",
	                               "Show stream setup information",
	                               "Where available, show which protocol and frame caused "
	                               "this RTP stream to be created",
	                               &global_rtp_show_setup_info);

	prefs_register_bool_preference(rtp_module, "heuristic_rtp",
	                               "Try to decode RTP outside of conversations",
	                               "If call control SIP/H323/RTSP/.. messages are missing in the trace, "
	                               "RTP isn't decoded without this",
	                               &global_rtp_heur);

	prefs_register_enum_preference(rtp_module, "version0_type",
	                               "Treat RTP version 0 packets as",
	                               "If an RTP version 0 packet is encountered, it can be treated as an invalid packet, a STUN packet, or a T.38 packet",
	                               &global_rtp_version0_type,
	                               rtp_version0_types, FALSE);
    prefs_register_uint_preference(rtp_module,
                                   "rfc2198_payload_type", "Payload Type for RFC2198",
                                   "Payload Type for RFC2198 Redundant Audio Data",
                                   10,
                                   &rtp_rfc2198_pt);
}

void
proto_reg_handoff_rtp(void)
{
	static gboolean rtp_prefs_initialized = FALSE;

	data_handle = find_dissector("data");
	stun_handle = find_dissector("stun");
	t38_handle = find_dissector("t38");
	/*
	 * Register this dissector as one that can be selected by a
	 * UDP port number.
	 */
	rtp_handle = find_dissector("rtp");
	rtp_rfc2198_handle = find_dissector("rtp.rfc2198");

	dissector_add_handle("udp.port", rtp_handle);

  	if (rtp_prefs_initialized) {
		dissector_delete("rtp.pt", rtp_saved_rfc2198_pt, rtp_rfc2198_handle);
	} else {
		rtp_prefs_initialized = TRUE;
	}
	rtp_saved_rfc2198_pt = rtp_rfc2198_pt;
	dissector_add("rtp.pt", rtp_saved_rfc2198_pt, rtp_rfc2198_handle);

	heur_dissector_add( "udp", dissect_rtp_heur, proto_rtp);
}
