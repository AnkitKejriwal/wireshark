/*
 * packet-mp4ves.c
 * Routines for MPEG4 dissection
 * Copyright 2007-2008, Anders Broman <anders.broman[at]ericsson.com>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * References:
 * http://www.ietf.org/rfc/rfc3016.txt?number=3016
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include <epan/packet.h>
#include <epan/proto.h>
#include <epan/asn1.h>

#include "prefs.h"

/* Initialize the protocol and registered fields */
static int proto_mp4ves								= -1;

static int hf_mp4ves_config = -1;
static int hf_mp4ves_start_code_prefix = -1;
static int hf_mp4ves_start_code = -1;
static int hf_mp4ves_vop_coding_type = -1;

/* Initialize the subtree pointers */
static int ett_mp4ves = -1;
static int ett_mp4ves_config = -1;

/* The dynamic payload type which will be dissected as MP4V-ES */

static guint global_dynamic_payload_type = 0;


/*
14496-2, Annex G, Table G-1.
Table G-1 FLC table for profile_and_level_indication Profile/Level Code
*/
static const value_string mp4ves_level_indication_vals[] =
{
  { 0,    "Reserved" },
  { 1,    "Simple Profile/Level 1" },
  { 2,    "Simple Profile/Level 2" },
  { 3,    "Reserved" },
  { 4,    "Reserved" },
  { 5,    "Reserved" },
  { 6,    "Reserved" },
  { 7,    "Reserved" },
  { 8,    "Simple Profile/Level 0" },
  { 9,    "Simple Profile/Level 0b" },
  /* Reserved 00001001 - 00010000 */
  { 0x11, "Simple Scalable Profile/Level 1" },
  { 0x12, "Simple Scalable Profile/Level 2" },
  /* Reserved 00010011 - 00100000 */
  { 0x21, "Core Profile/Level 1" },
  { 0x22, "Core Profile/Level 2" },
  /* Reserved 00100011 - 00110001 */
  { 0x32, "Main Profile/Level 2" },
  { 0x33, "Main Profile/Level 3" },
  { 0x34, "Main Profile/Level 4" },
  /* Reserved 00110101 - 01000001  */
  { 0x42, "N-bit Profile/Level 2" },
  /* Reserved 01000011 - 01010000  */
  { 0x51, "Scalable Texture Profile/Level 1" },
  /* Reserved 01010010 - 01100000 */
  { 0x61, "Simple Face Animation Profile/Level 1" },
  { 0x62, "Simple Face Animation Profile/Level 2" },
  { 0x63, "Simple FBA Profile/Level 1" },
  { 0x64, "Simple FBA Profile/Level 2" },
  /* Reserved 01100101 - 01110000 */
  { 0x71, "Basic Animated Texture Profile/Level 1" },
  { 0x72, "Basic Animated Texture Profile/Level 2" },
  /* Reserved 01110011 - 10000000 */
  { 0x81, "Hybrid Profile/Level 1" },
  { 0x82, "Hybrid Profile/Level 2" },
  /* Reserved 10000011 - 10010000 */
  { 0x91, "Advanced Real Time Simple Profile/Level 1" },
  { 0x92, "Advanced Real Time Simple Profile/Level 2" },
  { 0x93, "Advanced Real Time Simple Profile/Level 3" },
  { 0x94, "Advanced Real Time Simple Profile/Level 4" },
  /* Reserved 10010101 - 10100000 */
  { 0xa1, "Core Scalable Profile/Level 1" },
  { 0xa2, "Core Scalable Profile/Level 2" },
  { 0xa3, "Core Scalable Profile/Level 3" },
  /* Reserved 10100100 - 10110000  */
  { 0xb1, "Advanced Coding Efficiency Profile/Level 1" },
  { 0xb2, "Advanced Coding Efficiency Profile/Level 2" },
  { 0xb3, "Advanced Coding Efficiency Profile/Level 3" },
  { 0xb4, "Advanced Coding Efficiency Profile/Level 4" },
  /* Reserved 10110101 - 11000000 */
  { 0xc1, "Advanced Core Profile/Level 1" },
  { 0xc2, "Advanced Core Profile/Level 2" },
  /* Reserved 11000011 - 11010000 */
  { 0xd1, "Advanced Scalable Texture/Level 1" },
  { 0xd2, "Advanced Scalable Texture/Level 2" },
  { 0xd3, "Advanced Scalable Texture/Level 3" },
  /* Reserved 11010100 - 11100000 */
  { 0xe1, "Simple Studio Profile/Level 1" },
  { 0xe2, "Simple Studio Profile/Level 2" },
  { 0xe3, "Simple Studio Profile/Level 3" },
  { 0xe4, "Simple Studio Profile/Level 4" },
  { 0xe5, "Core Studio Profile/Level 1" },
  { 0xe6, "Core Studio Profile/Level 2" },
  { 0xe7, "Core Studio Profile/Level 3" },
  { 0xe8, "Core Studio Profile/Level 4" },
  /* Reserved 11101001 - 11101111 */
  { 0xf0, "Advanced Simple Profile/Level 0" },
  { 0xf1, "Advanced Simple Profile/Level 1" },
  { 0xf2, "Advanced Simple Profile/Level 2" },
  { 0xf3, "Advanced Simple Profile/Level 3" },
  { 0xf4, "Advanced Simple Profile/Level 4" },
  { 0xf5, "Advanced Simple Profile/Level 5" },
  /* Reserved 11110110 - 11110111 */
  { 0xf8, "Fine Granularity Scalable Profile/Level 0" },
  { 0xf9, "Fine Granularity Scalable Profile/Level 1" },
  { 0xfa, "Fine Granularity Scalable Profile/Level 2" },
  { 0xfb, "Fine Granularity Scalable Profile/Level 3" },
  { 0xfc, "Fine Granularity Scalable Profile/Level 4" },
  { 0xfd, "Fine Granularity Scalable Profile/Level 5" },
  { 0xfe, "Reserved" },
  { 0xff, "Reserved for Escape" },
  { 0, NULL },
};
static const range_string mp4ves_startcode_vals[] = {
	{ 0,	0x1f, "video_object_start_code" },
	{ 0x20, 0x2f, "video_object_layer_start_code" },
	{ 0x30, 0xaf, "reserved" },
	{ 0xb0, 0xb0, "visual_object_sequence_start_code" },
	{ 0xb1, 0xb1, "visual_object_sequence_end_code" },
	{ 0xb2, 0xb2, "user_data_start_code" },
	{ 0xb3, 0xb3, "group_of_vop_start_code" },
	{ 0xb4, 0xb4, "video_session_error_code" },
	{ 0xb5, 0xb5, "visual_object_start_code" },
	{ 0xb6, 0xb6, "vop_start_code" },
	{ 0xb7, 0xb9, "reserved" },
	{ 0xba, 0xba, "fba_object_start_code" },
	{ 0xbb, 0xbb, "fba_object_plane_start_code" },
	{ 0xbc, 0xbc, "mesh_object_start_code" },
	{ 0xbd, 0xbd, "mesh_object_plane_start_code" },
	{ 0xbe, 0xbe, "still_texture_object_start_code" },
	{ 0xbf, 0xbf, "texture_spatial_layer_start_code" },
	{ 0xc0, 0xc0, "texture_snr_layer_start_code" },
	{ 0xc1, 0xc1, "texture_tile_start_code" },
	{ 0xc2, 0xc2, "texture_shape_layer_start_code" },
	{ 0xc3, 0xc3, "stuffing_start_code" },
	{ 0xc4, 0xc5, "reserved" },
	{ 0xc6, 0xcf, "System start codes" }, /* NOTE System start codes are defined in ISO/IEC 14496-1:1999 */
	{ 0,     0, NULL }
};

static const value_string mp4ves_vop_coding_type_vals[] = {
	{ 0,	"intra-coded (I)" },
	{ 1,	"predictive-coded (P)" },
	{ 2,	"bidirectionally-predictive-coded (B)" },
	{ 3,	"sprite (S)" },
	{ 0,	NULL }
};

/* 
 * FLC table for video_object_type indication
 */
static const value_string mp4ves_video_object_type_vals[] = {
	{ 0x0,	"Reserved" },
	{ 0x1,	"Simple Object Type" },
	{ 0x2,	"Simple Scalable Object Type" },
	{ 0x3,	"Core Object Type" },
	{ 0x4,	"Main Object Type" },
	{ 0x5,	"N-bit Object Type" },
	{ 0x6,	"Basic Anim. 2D Texture" },
	{ 0x7,	"Anim. 2D Mesh" },
	{ 0x8,	"Simple Face" },
	{ 0x9,	"Still Scalable Texture" },
	{ 0xa,	"Advanced Real Time Simple" },
	{ 0xb,	"Core Scalable" },
	{ 0xc,	"Advanced Coding Efficiency" },
	{ 0xd,	"Advanced Scalable Texture" },
	{ 0xe,	"Simple FBA" },
	{ 0,	NULL }
};

#if 0
To be called from packet-sdp.c 
void 
dissect_mp4ves_config(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	proto_item *item;
	proto_tree *mp4ves_tree;
	int bit_offset = 0;
	guint32 dword;

	item = proto_tree_add_item(tree, hf_mp4ves_config, tvb, 0, -1, FALSE);
	mp4ves_tree = proto_item_add_subtree(item, ett_mp4ves_config);

	/* Get start code prefix */
	dword = tvb_get_bits32(tvb,bit_offset, 24, FALSE);
	if (dword == 1){

	}else{
		/* No start code prefix */
		return;
	}

	proto_tree_add_bits_item(tree, hf_mp4ves_start_code_prefix, tvb, bit_offset, 24, FALSE);
	bit_offset = bit_offset+24;

	/* We are byte aligned no stuffing */
	dword = tvb_get_bits8(tvb,bit_offset, 8);
	proto_tree_add_bits_item(tree, hf_mp4ves_start_code, tvb, bit_offset, 8, FALSE);


}
#endif

void
dissect_mp4ves(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	int bit_offset = 0;
	proto_item *item;
	proto_tree *mp4ves_tree;
	guint32 dword;

	/* Make entries in Protocol column and Info column on summary display */
	if (check_col(pinfo->cinfo, COL_PROTOCOL)) 
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "MP4V-ES");
	if (tree) {
		item = proto_tree_add_item(tree, proto_mp4ves, tvb, 0, -1, FALSE);
		mp4ves_tree = proto_item_add_subtree(item, ett_mp4ves);
	/*
	    +------+------+------+------+
	(a) | RTP  |  VS  |  VO  | VOL  |
	    |header|header|header|header|
	    +------+------+------+------+

	    +------+------+------+------+------------+
	(b) | RTP  |  VS  |  VO  | VOL  |Video Packet|
	    |header|header|header|header|            |
	    +------+------+------+------+------------+

	    +------+-----+------------------+
	(c) | RTP  | GOV |Video Object Plane|
	    |header|     |                  |
	    +------+-----+------------------+

	    +------+------+------------+  +------+------+------------+
	(d) | RTP  | VOP  |Video Packet|  | RTP  |  VP  |Video Packet|
	    |header|header|    (1)     |  |header|header|    (2)     |
	    +------+------+------------+  +------+------+------------+

	    +------+------+------------+------+------------+------+------------+
	(e) | RTP  |  VP  |Video Packet|  VP  |Video Packet|  VP  |Video Packet|
	    |header|header|     (1)    |header|    (2)     |header|    (3)     |
	    +------+------+------------+------+------------+------+------------+

	   +------+------+------------+  +------+------------+
	(f) | RTP  | VOP  |VOP fragment|  | RTP  |VOP fragment|
	    |header|header|    (1)     |  |header|    (2)     | ___
	    +------+------+------------+  +------+------------+

	     Figure 2 - Examples of RTP packetized MPEG-4 Visual bitstream

	So a valid packet should start with
	VS	- Visual Object Sequence Header
	GOV	- Group_of_VideoObjectPlane
	VOP	- Video Object Plane 
	VP	- Video Plane
	Otherwies it's a VOP fragment.

	visual_object_sequence_start_code: The visual_object_sequence_start_code is 
	the bit string '000001B0' in hexadecimal. It initiates a visual session.

	group_of_vop_start_code: The group_of_vop_start_code is the bit string '000001B3' in hexadecimal. It identifies 
	the beginning of a GOV header.

	vop_start_code: This is the bit string '000001B6' in hexadecimal.


	*/
		dword = tvb_get_bits32(tvb,bit_offset, 24, FALSE);
		if (dword != 1){
			/* if it's not 23 zeros followed by 1 it isn't a start code */
			proto_tree_add_text(tree, tvb, bit_offset>>3, -1, "Data");
			return;
		}
		proto_tree_add_bits_item(tree, hf_mp4ves_start_code_prefix, tvb, bit_offset, 24, FALSE);
		bit_offset = bit_offset+24;
		dword = tvb_get_bits8(tvb,bit_offset, 8);
		proto_tree_add_bits_item(tree, hf_mp4ves_start_code, tvb, bit_offset, 8, FALSE);
		bit_offset = bit_offset+8;
		switch(dword){
		/* vop_start_code */
		case 0xb6:
			/* vop_coding_type 2 bits */
			proto_tree_add_bits_item(tree, hf_mp4ves_vop_coding_type, tvb, bit_offset, 2, FALSE);
			break;
		default:
			break;
		}
	}

}
/*
 * Parameter name profileAndLevel
 * Parameter description This is a nonCollapsing GenericParameter
 * Parameter identifier value 0
 * Parameter status Mandatory
 * Parameter type unsignedMax. Shall be in the range 0..255.
 * H245:
 * unsignedMax       INTEGER(0..65535), -- Look for max 
 */
static int
dissect_mp4ves_par_profile(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree _U_)
{
  int offset = 0;
  guint16 lvl;
  const gchar *p = NULL;
  asn1_ctx_t *actx;

  actx = get_asn1_ctx(pinfo->private_data);
  DISSECTOR_ASSERT(actx);

  lvl = tvb_get_ntohs(tvb, offset);
  p = match_strval(lvl, VALS(mp4ves_level_indication_vals));
  if (p) {
    proto_item_append_text(actx->created_item, " - profileAndLevel %s", p);
  }
  offset += 2;
  return offset;
}
static int
dissect_mp4ves_par_video_object_type(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree _U_)
{
  int offset = 0;
  guint16 lvl;
  const gchar *p = NULL;
  asn1_ctx_t *actx;

  actx = get_asn1_ctx(pinfo->private_data);
  DISSECTOR_ASSERT(actx);

  lvl = tvb_get_ntohs(tvb, offset);
  p = match_strval(lvl, VALS(mp4ves_video_object_type_vals));
  if (p) {
    proto_item_append_text(actx->created_item, " - video_object_type %s", p);
  }
  offset += 2;
  return offset;
}

typedef struct _mp4ves_capability_t {
  const gchar *id;
  const gchar *name;
  new_dissector_t content_pdu;
} mp4ves_capability_t;

static mp4ves_capability_t mp4ves_capability_tab[] = {
  /* ITU-T H.245  capabilities ISO/IEC 14496-2(m*/
  { "GenericCapability/0.0.8.245.1.0.0/nonCollapsing/0", "profileAndLevel", dissect_mp4ves_par_profile },
  { "GenericCapability/0.0.8.245.1.0.0/nonCollapsing/1", "object", dissect_mp4ves_par_video_object_type },
  { NULL, NULL, NULL },
};                                 

static mp4ves_capability_t *find_cap(const gchar *id) {
  mp4ves_capability_t *ftr = NULL;
  mp4ves_capability_t *f;

  for (f=mp4ves_capability_tab; f->id; f++) {
    if (!strcmp(id, f->id)) { ftr = f; break; }
  }
  return ftr;
}

static void
dissect_mp4ves_name(tvbuff_t *tvb _U_, packet_info *pinfo, proto_tree *tree) 
{
  asn1_ctx_t *actx;
  mp4ves_capability_t *ftr = NULL;

  actx = get_asn1_ctx(pinfo->private_data);
  DISSECTOR_ASSERT(actx);
  if (tree) {
    ftr = find_cap(pinfo->match_string);
    if (ftr) {
      proto_item_append_text(actx->created_item, " - %s", ftr->name);
      proto_item_append_text(proto_item_get_parent(proto_tree_get_parent(tree)), ": %s", ftr->name);
    } else {
      proto_item_append_text(actx->created_item, " - unknown(%s)", pinfo->match_string);
    }
  }
}

void
proto_reg_handoff_mp4ves(void)
{
	static dissector_handle_t mp4ves_handle;
	static guint dynamic_payload_type;
	static gboolean mp4ves_prefs_initialized = FALSE;

	if (!mp4ves_prefs_initialized) {
		dissector_handle_t mp4ves_name_handle;
		mp4ves_capability_t *ftr;

		mp4ves_handle = find_dissector("mp4ves");
		dissector_add_string("rtp_dyn_payload_type","MP4V-ES", mp4ves_handle);
		mp4ves_prefs_initialized = TRUE;

		mp4ves_name_handle = create_dissector_handle(dissect_mp4ves_name, proto_mp4ves);
		for (ftr=mp4ves_capability_tab; ftr->id; ftr++) {
		    if (ftr->name) 
				dissector_add_string("h245.gef.name", ftr->id, mp4ves_name_handle);
			if (ftr->content_pdu)
				dissector_add_string("h245.gef.content", ftr->id, new_create_dissector_handle(ftr->content_pdu, proto_mp4ves));
		}
	}else{
		if ( dynamic_payload_type > 95 )
			dissector_delete("rtp.pt", dynamic_payload_type, mp4ves_handle);
	}
	dynamic_payload_type = global_dynamic_payload_type;

	if ( dynamic_payload_type > 95 ){
		dissector_add("rtp.pt", dynamic_payload_type, mp4ves_handle);
	}
}


void
proto_register_mp4ves(void)
{                 


/* Setup list of header fields  See Section 1.6.1 for details*/
	static hf_register_info hf[] = {
		{ &hf_mp4ves_config,
			{ "Configuration",        "mp4ves.configuration", 
			FT_BYTES, BASE_NONE, NULL, 0x0,
			"Configuration", HFILL }
		},
		{ &hf_mp4ves_start_code_prefix,
			{ "start code prefix",		"mp4ves.start_code_prefix", 
			FT_UINT32, BASE_HEX, NULL, 0x0,
			"start code prefix", HFILL }
		},
		{ &hf_mp4ves_start_code,
			{ "Start code",		"mp4ves.start_code", 
			FT_UINT32, BASE_HEX, RVALS(&mp4ves_startcode_vals), 0x0,
			"Start code", HFILL }
		},
		{ &hf_mp4ves_vop_coding_type,
			{ "vop_coding_type",		"mp4ves.vop_coding_type", 
			FT_UINT8, BASE_DEC, VALS(mp4ves_vop_coding_type_vals), 0x0,
			"Start code", HFILL }
		},
	};

/* Setup protocol subtree array */
	static gint *ett[] = {
		&ett_mp4ves,
		&ett_mp4ves_config,
	};

	module_t *mp4ves_module;

/* Register the protocol name and description */
	proto_mp4ves = proto_register_protocol("MP4V-ES","MP4V-ES", "mp4v-es");

/* Required function calls to register the header fields and subtrees used */
	proto_register_field_array(proto_mp4ves, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));
	/* Register a configuration option for port */

	register_dissector("mp4ves", dissect_mp4ves, proto_mp4ves);

	/* Register a configuration option for port */	
	mp4ves_module = prefs_register_protocol(proto_mp4ves, proto_reg_handoff_mp4ves);

	prefs_register_uint_preference(mp4ves_module, "dynamic.payload.type",
								   "MP4V-ES dynamic payload type",
								   "The dynamic payload type which will be interpreted as MP4V-ES",
								   10,
								   &global_dynamic_payload_type);

}
