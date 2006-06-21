/* packet-pn-mrrt.c
 * Routines for PN-MRRT (PROFINET Media Redundancy for cyclic realtime data) 
 * packet dissection.
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
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include <string.h>

#include <glib.h>
#include <epan/packet.h>
#include <epan/dissectors/packet-dcerpc.h>
#include <epan/oui.h>
#include <epan/expert.h>

static int proto_pn_mrrt = -1;

static int hf_pn_mrrt_sequence_id = -1;
static int hf_pn_mrrt_domain_uuid = -1;
static int hf_pn_mrrt_type = -1;
static int hf_pn_mrrt_length = -1;
static int hf_pn_mrrt_version = -1;
static int hf_pn_mrrt_sa = -1;

static int hf_pn_mrrt_data = -1;


static gint ett_pn_mrrt = -1;



static const value_string pn_mrrt_block_type_vals[] = {
	{ 0x00, "End" },
	{ 0x01, "Common" },
	{ 0x02, "Test" },
    /*0x03 - 0x7E Reserved */
	{ 0x7F, "Organizationally Specific"},
	{ 0, NULL },
};

static const value_string pn_mrrt_oui_vals[] = {
	{ OUI_PROFINET,         "PROFINET" },

	{ 0, NULL }
};


/* XXX - use include file instead for these helpers */
extern int dissect_pn_uint8(tvbuff_t *tvb, gint offset, packet_info *pinfo,
                  proto_tree *tree, int hfindex, guint8 *pdata);

extern int dissect_pn_uint16(tvbuff_t *tvb, gint offset, packet_info *pinfo,
                       proto_tree *tree, int hfindex, guint16 *pdata);

extern int dissect_pn_uint32(tvbuff_t *tvb, gint offset, packet_info *pinfo,
                       proto_tree *tree, int hfindex, guint32 *pdata);

extern int dissect_pn_int16(tvbuff_t *tvb, gint offset, packet_info *pinfo _U_,
                       proto_tree *tree, int hfindex, gint16 *pdata);

extern int dissect_pn_oid(tvbuff_t *tvb, int offset, packet_info *pinfo _U_,
                    proto_tree *tree, int hfindex, guint32 *pdata);

extern int dissect_pn_mac(tvbuff_t *tvb, int offset, packet_info *pinfo _U_,
                    proto_tree *tree, int hfindex, guint8 *pdata);

extern int dissect_pn_uuid(tvbuff_t *tvb, int offset, packet_info *pinfo _U_,
                    proto_tree *tree, int hfindex, e_uuid_t *uuid);



static int
dissect_PNMRRT_Common(tvbuff_t *tvb, int offset, 
	packet_info *pinfo, proto_tree *tree, proto_item *item, guint8 length)
{
    guint16 sequence_id;
    e_uuid_t uuid;


    /* MRRT_SequenceID */
    offset = dissect_pn_uint16(tvb, offset, pinfo, tree, hf_pn_mrrt_sequence_id, &sequence_id);

    /* MRRT_DomainUUID */
    offset = dissect_pn_uuid(tvb, offset, pinfo, tree, hf_pn_mrrt_domain_uuid, &uuid);

    if (check_col(pinfo->cinfo, COL_INFO))
      col_append_fstr(pinfo->cinfo, COL_INFO, "Common");

    proto_item_append_text(item, "Common");

    return offset;
}


static int
dissect_PNMRRT_Test(tvbuff_t *tvb, int offset, 
	packet_info *pinfo, proto_tree *tree, proto_item *item, guint8 length)
{
    guint8 mac[6];


    /* MRRT_SA */
    offset = dissect_pn_mac(tvb, offset, pinfo, tree, hf_pn_mrrt_sa, mac);

    /* Padding */
    if (offset % 4) {
        offset += 4 - (offset % 4);
    }

    if (check_col(pinfo->cinfo, COL_INFO))
      col_append_fstr(pinfo->cinfo, COL_INFO, "Test");

    proto_item_append_text(item, "Test");

    return offset;
}

static int
dissect_PNMRRT_PDU(tvbuff_t *tvb, int offset, 
	packet_info *pinfo, proto_tree *tree, proto_item *item)
{
    guint16 version;
    guint8 type;
    guint8 length;
    gint    i =0;
    proto_item *unknown_item;


    /* MRRT_Version */
    offset = dissect_pn_uint16(tvb, offset, pinfo, tree, hf_pn_mrrt_version, &version);

    while(tvb_length_remaining(tvb, offset) > 0) {
        /* MRRT_TLVHeader.Type */
        offset = dissect_pn_uint8(tvb, offset, pinfo, tree, hf_pn_mrrt_type, &type);

        /* MRRT_TLVHeader.Length */
        offset = dissect_pn_uint8(tvb, offset, pinfo, tree, hf_pn_mrrt_length, &length);


        if(i != 0) {
            if (check_col(pinfo->cinfo, COL_INFO))
              col_append_fstr(pinfo->cinfo, COL_INFO, ", ");

            proto_item_append_text(item, ", ");
        }

        i++;

        switch(type) {
        case(0x00):
            /* no content */
            if (check_col(pinfo->cinfo, COL_INFO))
              col_append_fstr(pinfo->cinfo, COL_INFO, "End");
            proto_item_append_text(item, "End");
            return offset;
            break;
        case(0x01):
            offset = dissect_PNMRRT_Common(tvb, offset, pinfo, tree, item, length);
            break;
        case(0x02):
            offset = dissect_PNMRRT_Test(tvb, offset, pinfo, tree, item, length);
            break;
        default:
            unknown_item = proto_tree_add_string_format(tree, hf_pn_mrrt_data, tvb, offset, length, "data", 
                "PN-MRRT Unknown TLVType 0x%x, Data: %d bytes", type, length);
            expert_add_info_format(pinfo, unknown_item, PI_UNDECODED, PI_WARN,
			    "Unknown TLVType 0x%x, %u bytes",
			    type, length);
	        if (check_col(pinfo->cinfo, COL_INFO))
		        col_append_fstr(pinfo->cinfo, COL_INFO, "Unknown TLVType 0x%x", type);

	        proto_item_append_text(item, "Unknown TLVType 0x%x", type);

            offset += length;
        }
    }

    /* will never be reached */
    return offset;
}


/* possibly dissect a PN-RT packet (frame ID must be in the appropriate range) */
static gboolean
dissect_PNMRRT_Data_heur(tvbuff_t *tvb, 
	packet_info *pinfo, proto_tree *tree)
{
    guint16 u16FrameID;
    proto_item *item = NULL;
    proto_tree *mrrt_tree = NULL;
    int offset = 0;
	guint32 u32SubStart;


    /* the tvb will NOT contain the frame_id here, so get it from our private data! */
    u16FrameID = GPOINTER_TO_UINT(pinfo->private_data);

	/* frame id must be in valid range (MRRT) */
	if (u16FrameID != 0xFF60) {
        /* we are not interested in this packet */
        return FALSE;
    }

	if (check_col(pinfo->cinfo, COL_PROTOCOL))
	    col_add_str(pinfo->cinfo, COL_PROTOCOL, "PN-MRRT");
    if (check_col(pinfo->cinfo, COL_INFO))
      col_add_str(pinfo->cinfo, COL_INFO, "");

    /* subtree for MRRT */
	item = proto_tree_add_protocol_format(tree, proto_pn_mrrt, tvb, 0, 0, "PROFINET MRRT, ");
	mrrt_tree = proto_item_add_subtree(item, ett_pn_mrrt);
    u32SubStart = offset;

    offset = dissect_PNMRRT_PDU(tvb, offset, pinfo, mrrt_tree, item);

	proto_item_set_len(item, offset - u32SubStart);

    return TRUE;
}


void
proto_register_pn_mrrt (void)
{
	static hf_register_info hf[] = {
    { &hf_pn_mrrt_data,
        { "Undecoded Data", "pn_mrrt.data", FT_STRING, BASE_DEC, NULL, 0x0, "", HFILL }},

	{ &hf_pn_mrrt_type,
		{ "Type", "pn_mrrt.type", FT_UINT8, BASE_HEX, VALS(pn_mrrt_block_type_vals), 0x0, "", HFILL }},
	{ &hf_pn_mrrt_length,
		{ "Length", "pn_mrrt.length", FT_UINT8, BASE_DEC, NULL, 0x0, "", HFILL }},
	{ &hf_pn_mrrt_version,
		{ "Version", "pn_mrrt.version", FT_UINT16, BASE_DEC, NULL, 0x0, "", HFILL }},
	{ &hf_pn_mrrt_sequence_id,
		{ "SequenceID", "pn_mrrt.sequence_id", FT_UINT16, BASE_HEX, NULL, 0x0, "Unique sequence number to each outstanding service request", HFILL }},
	{ &hf_pn_mrrt_sa,
        { "SA", "pn_mrrt.sa", FT_ETHER, BASE_HEX, 0x0, 0x0, "", HFILL }},
	{ &hf_pn_mrrt_domain_uuid,
		{ "DomainUUID", "pn_mrrt.domain_uuid", FT_GUID, BASE_NONE, NULL, 0x0, "", HFILL }},
    };


	static gint *ett[] = {
		&ett_pn_mrrt
    };

	proto_pn_mrrt = proto_register_protocol ("PROFINET MRRT", "PN-MRRT", "pn_mrrt");
	proto_register_field_array (proto_pn_mrrt, hf, array_length (hf));
	proto_register_subtree_array (ett, array_length (ett));
}


void
proto_reg_handoff_pn_mrrt (void)
{

    /* register ourself as an heuristic pn-rt payload dissector */
	heur_dissector_add("pn_rt", dissect_PNMRRT_Data_heur, proto_pn_mrrt);
}
