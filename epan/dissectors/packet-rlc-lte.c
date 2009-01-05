/* Routines for LTE RLC disassembly
 *
 * Martin Mathieson
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <epan/packet.h>
#include <epan/expert.h>

#include "packet-rlc-lte.h"


/* Described in:
 * 3GPP TS 36.322 Evolved Universal Terrestial Radio Access (E-UTRA)
 * Radio Link Control (RLC) Protocol specification
 */

/* Initialize the protocol and registered fields. */
int proto_rlc_lte = -1;

/* Decoding context */
static int hf_rlc_lte_context_mode = -1;
static int hf_rlc_lte_context_direction = -1;
static int hf_rlc_lte_context_priority = -1;
static int hf_rlc_lte_context_ueid = -1;
static int hf_rlc_lte_context_channel_type = -1;
static int hf_rlc_lte_context_channel_id = -1;
static int hf_rlc_lte_context_pdu_length = -1;
static int hf_rlc_lte_context_um_sn_length = -1;

/* Transparent mode fields */
static int hf_rlc_lte_tm_data = -1;

/* Unacknowledged mode fields */
static int hf_rlc_lte_um_header = -1;
static int hf_rlc_lte_um_fi = -1;
static int hf_rlc_lte_um_fixed_e = -1;
static int hf_rlc_lte_um_sn = -1;
static int hf_rlc_lte_um_fixed_reserved = -1;
static int hf_rlc_lte_um_data = -1;
static int hf_rlc_lte_extension_part = -1;

/* Extended header (common to UM and AM) */
static int hf_rlc_lte_extension_e = -1;
static int hf_rlc_lte_extension_li = -1;
static int hf_rlc_lte_extension_padding = -1;


/* Acknowledged mode fields */
static int hf_rlc_lte_am_header = -1;
static int hf_rlc_lte_am_data_control = -1;
static int hf_rlc_lte_am_rf = -1;
static int hf_rlc_lte_am_p = -1;
static int hf_rlc_lte_am_fi = -1;
static int hf_rlc_lte_am_fixed_e = -1;
static int hf_rlc_lte_am_fixed_sn = -1;
static int hf_rlc_lte_am_segment_lsf = -1;
static int hf_rlc_lte_am_segment_so = -1;
static int hf_rlc_lte_am_data = -1;

/* Control fields */
static int hf_rlc_lte_am_cpt = -1;
static int hf_rlc_lte_am_ack_sn = -1;
static int hf_rlc_lte_am_e1 = -1;
static int hf_rlc_lte_am_e2 = -1;
static int hf_rlc_lte_am_nack_sn = -1;
static int hf_rlc_lte_am_so_start = -1;
static int hf_rlc_lte_am_so_end = -1;

/* Subtrees. */
static int ett_rlc_lte = -1;
static int ett_rlc_lte_um_header = -1;
static int ett_rlc_lte_am_header = -1;
static int ett_rlc_lte_extension_part = -1;


static const value_string direction_vals[] =
{
    { 0,      "Uplink"},
    { 1,      "Downlink"},
    { 0, NULL }
};


#define RLC_TM_MODE 1
#define RLC_UM_MODE 2
#define RLC_AM_MODE 3


static const value_string rlc_mode_short_vals[] =
{
    { RLC_TM_MODE,      "TM"},
    { RLC_UM_MODE,      "UM"},
    { RLC_AM_MODE,      "AM"},
    { 0, NULL }
};

static const value_string rlc_mode_vals[] =
{
    { RLC_TM_MODE,      "Transparent Mode"},
    { RLC_UM_MODE,      "Unacknowledged Mode"},
    { RLC_AM_MODE,      "Acknowledged Mode"},
    { 0, NULL }
};


static const value_string rlc_channel_type_vals[] =
{
    { 1,      "CCCH"},
    { 2,      "BCCH"},
    { 3,      "PCCH"},
    { 4,      "SRB"},
    { 5,      "DRB"},
    { 0, NULL }
};


static const value_string framing_info_vals[] =
{
    { 0,      "First byte begins an RLC SDU and last byte ends an RLC SDU"},
    { 1,      "First byte begins an RLC SDU and last byte does not end an RLC SDU"},
    { 2,      "First byte does not begin an RLC SDU and last byte ends an RLC SDU"},
    { 3,      "First byte does not begin an RLC SDU and last byte does not end an RLC SDU"},
    { 0, NULL }
};

static const value_string fixed_extension_vals[] =
{
    { 0,      "Data field follows from the octet following the fixed part of the header"},
    { 1,      "A set of E field and LI field follows from the octet following the fixed part of the header"},
    { 0, NULL }
};

static const value_string extension_extension_vals[] =
{
    { 0,      "Data field follows from the octet following the LI field following this E field"},
    { 1,      "A set of E field and LI field follows from the bit following the LI field following this E field"},
    { 0, NULL }
};

static const value_string data_or_control_vals[] =
{
    { 0,      "Control PDU"},
    { 1,      "Data PDU"},
    { 0, NULL }
};

static const value_string resegmentation_flag_vals[] =
{
    { 0,      "AMD PDU"},
    { 1,      "AND PDU segment"},
    { 0, NULL }
};

static const value_string polling_bit_vals[] =
{
    { 0,      "Status report not requested"},
    { 1,      "Status report is requested"},
    { 0, NULL }
};


static const value_string lsf_vals[] =
{
    { 0,      "Last byte of the AMD PDU segment does not correspond to the last byte of an AMD PDU"},
    { 1,      "Last byte of the AMD PDU segment corresponds to the last byte of an AND PDU"},
    { 0, NULL }
};


static const value_string control_pdu_type_vals[] =
{
    { 0,      "STATUS PDU"},
    { 0, NULL }
};

static const value_string am_e1_vals[] =
{
    { 0,      "A set of NACK_SN, E1 and E2 does not follow"},
    { 1,      "A set of NACK_SN, E1 and E2 follows"},
    { 0, NULL }
};

static const value_string am_e2_vals[] =
{
    { 0,      "A set of SOstart and SOend does not follow for this NACK_SN"},
    { 1,      "A set of SOstart and SOend follows for this NACK_SN"},
    { 0, NULL }
};

/* These are for keeping track of UM/AM extension headers, and the lengths found
   in them */
guint8  s_number_of_extensions = 0;
#define MAX_RLC_SDUS 64
guint16 s_lengths[MAX_RLC_SDUS];


/* Dissect extension headers (common to both UM and AM) */
static int dissect_rlc_lte_extension_header(tvbuff_t *tvb, packet_info *pinfo,
                                            proto_tree *tree,
                                            int offset)
{
    guint8  isOdd;
    guint64 extension = 1;
    guint64 length;

    /* Reset this count */
    s_number_of_extensions = 0;

    while (extension && (s_number_of_extensions < MAX_RLC_SDUS)) {
        proto_tree *extension_part_tree;
        proto_item *extension_part_ti;

        isOdd = (s_number_of_extensions % 2);

        /* Extension part subtree */
        extension_part_ti = proto_tree_add_string_format(tree,
                                                         hf_rlc_lte_extension_part,
                                                         tvb, offset, 2,
                                                         "",
                                                         "Extension Part");
        extension_part_tree = proto_item_add_subtree(extension_part_ti,
                                                     ett_rlc_lte_extension_part);

        /* Read next extension */
        proto_tree_add_bits_ret_val(extension_part_tree, hf_rlc_lte_extension_e, tvb,
                                   (offset*8) + ((isOdd) ? 4 : 0),
                                    1,
                                    &extension, FALSE);

        /* Read length field */
        proto_tree_add_bits_ret_val(extension_part_tree, hf_rlc_lte_extension_li, tvb,
                                   (offset*8) + ((isOdd) ? 5 : 1),
                                    11,
                                    &length, FALSE);

        proto_item_append_text(extension_part_tree, " (length=%u)", (guint16)length);

        /* Move on to byte of next extension */
        if (isOdd) {
            offset += 2;
        } else {
            offset++;
        }

        s_lengths[s_number_of_extensions++] = (guint16)length;
    }

    /* May need to skip padding after last extension part */
    isOdd = (s_number_of_extensions % 2);
    if (isOdd) {
        guint8 padding;
        proto_item *ti;

        padding = tvb_get_guint8(tvb, offset) & 0x0f;
        ti = proto_tree_add_item(tree, hf_rlc_lte_extension_padding,
                                 tvb, offset, 1, FALSE);
        if (padding != 0) {
            expert_add_info_format(pinfo, ti, PI_MALFORMED, PI_ERROR,
                      "Extension Header padding not zero (found 0x%x)", padding);
        }
        offset++;
    }

    return offset;
}


/* Show in the info column how many bytes are in the UM/AM PDU, and indicate
   whether or not the beginning and end are included in this packet */
static void show_PDU_in_info(packet_info *pinfo,
                             guint16 length,
                             guint8 first_includes_start,
                             guint8 last_includes_end)
{
    /* Reflect this PDU in the info column */
    if (check_col(pinfo->cinfo, COL_INFO)) {
        col_append_fstr(pinfo->cinfo, COL_INFO, "  %s%u-bytes%s",
                        (first_includes_start) ? "[" : "..",
                        length,
                        (last_includes_end) ? "]" : "..");
    }
}


/***************************************************/
/* Unacknowledged mode PDU                         */
static void dissect_rlc_lte_um(tvbuff_t *tvb, packet_info *pinfo,
                               proto_tree *tree,
                               int offset,
                               rlc_lte_info *p_rlc_lte_info)
{
    guint64 framing_info;
    guint8  first_includes_start;
    guint8  last_includes_end;
    guint64 fixed_extension;
    guint64 sn;
    gint    start_offset = offset;
    proto_tree *um_header_tree;
    proto_item *um_header_ti;

    /* Add UM header subtree */
    um_header_ti = proto_tree_add_string_format(tree,
                                                hf_rlc_lte_um_header,
                                                tvb, offset, 0,
                                                "",
                                                "UM header");
    um_header_tree = proto_item_add_subtree(um_header_ti,
                                            ett_rlc_lte_um_header);


    /*******************************/
    /* Fixed UM header             */
    if (p_rlc_lte_info->UMSequenceNumberLength == 5) {
        /* Framing info (2 bits) */
        proto_tree_add_bits_ret_val(um_header_tree, hf_rlc_lte_um_fi,
                                    tvb, offset*8, 2,
                                    &framing_info, FALSE);

        /* Extension (1 bit) */
        proto_tree_add_bits_ret_val(um_header_tree, hf_rlc_lte_um_fixed_e, tvb,
                                    (offset*8) + 2, 1,
                                    &fixed_extension, FALSE);

        /* Sequence Number (5 bit) */
        proto_tree_add_bits_ret_val(um_header_tree, hf_rlc_lte_um_sn, tvb,
                                    (offset*8) + 3, 5,
                                    &sn, FALSE);
        offset++;
    }
    else if (p_rlc_lte_info->UMSequenceNumberLength == 10) {
        guint8 reserved;
        proto_item *ti;

        /* Check 3 Reserved bits */
        reserved = (tvb_get_guint8(tvb, offset) & 0xe0) >> 5;
        ti = proto_tree_add_item(um_header_tree, hf_rlc_lte_um_fixed_reserved, tvb, offset, 1, FALSE);
        if (reserved != 0) {
            expert_add_info_format(pinfo, ti, PI_MALFORMED, PI_ERROR,
                      "RLC UM Fixed header Reserved bits not zero (found 0x%x)", reserved);
        }

        /* Framing info (2 bits) */
        proto_tree_add_bits_ret_val(um_header_tree, hf_rlc_lte_um_fi,
                                    tvb, (offset*8)+3, 2,
                                    &framing_info, FALSE);

        /* Extension (1 bit) */
        proto_tree_add_bits_ret_val(um_header_tree, hf_rlc_lte_um_fixed_e, tvb,
                                    (offset*8) + 5, 1,
                                    &fixed_extension, FALSE);

        /* Sequence Number (10 bits) */
        proto_tree_add_bits_ret_val(um_header_tree, hf_rlc_lte_um_sn, tvb,
                                    (offset*8) + 6, 10,
                                    &sn, FALSE);
        offset += 2;
    }
    else {
        /* Invalid length of sequence number */
        proto_item *ti;
        ti = proto_tree_add_text(um_header_tree, tvb, 0, 0, "Invalid sequence number length (%u bits)",
                                 p_rlc_lte_info->UMSequenceNumberLength);
        expert_add_info_format(pinfo, ti, PI_MALFORMED, PI_ERROR,
                               "Invalid sequence number length (%u bits)",
                               p_rlc_lte_info->UMSequenceNumberLength);
        return;
    }

    /* Show SN in info column */
    if (check_col(pinfo->cinfo, COL_INFO)) {
        col_append_fstr(pinfo->cinfo, COL_INFO, "  SN=%04u",
                        (guint16)sn);
    }

    /* Show SN in UM header root */
    proto_item_append_text(um_header_ti, " (SN=%u)", (guint16)sn);
    proto_item_set_len(um_header_ti, offset-start_offset);


    /*************************************/
    /* UM header extension               */
    if (fixed_extension) {
        offset = dissect_rlc_lte_extension_header(tvb, pinfo, tree, offset);
    }


    /* Extract these 2 flags from framing_info */
    first_includes_start = ((guint8)framing_info & 0x02) == 0;
    last_includes_end =    ((guint8)framing_info & 0x01) == 0;


    /*************************************/
    /* Data                              */
    if (s_number_of_extensions > 0) {
        /* Show each data segment separately */
        int n;
        for (n=0; n < s_number_of_extensions; n++) {
            proto_tree_add_item(tree, hf_rlc_lte_um_data, tvb, offset, s_lengths[n], FALSE);
            show_PDU_in_info(pinfo, s_lengths[n],
                             (n==0) ? first_includes_start : TRUE,
                             TRUE);
            offset += s_lengths[n];
        }
    }

    /* Final data element */
    proto_tree_add_item(tree, hf_rlc_lte_um_data, tvb, offset, -1, FALSE);
    show_PDU_in_info(pinfo, tvb_length_remaining(tvb, offset),
                     (s_number_of_extensions == 0) ? first_includes_start : TRUE,
                     last_includes_end);
}




/* Dissect an AM STATUS PDU */
static void dissect_rlc_lte_am_status_pdu(tvbuff_t *tvb, packet_info *pinfo,
                                          proto_tree *tree,
                                          int offset)
{
    guint8     cpt;
    guint64    ack_sn, nack_sn;
    guint64    e1 = 0, e2 = 0;
    guint64    so_start, so_end;
    int        bit_offset = offset * 8;
    proto_item *ti;

    /****************************************************************/
    /* Part of RLC control PDU header                               */

    /* Control PDU Type (CPT) */
    cpt = (tvb_get_guint8(tvb, offset) & 0xf0) >> 4;
    ti = proto_tree_add_item(tree, hf_rlc_lte_am_cpt, tvb, offset, 1, FALSE);
    if (cpt != 0) {
        /* Protest and stop - only know about STATUS PDUs */
        expert_add_info_format(pinfo, ti, PI_MALFORMED, PI_ERROR,
                               "RLC Control frame type %u not handled", cpt);
        return;
    }


    /*****************************************************************/
    /* STATUS PDU                                                    */

    /* The PDU itself starts 4 bits into the byte */
    bit_offset += 4;

    /* ACK SN */
    proto_tree_add_bits_ret_val(tree, hf_rlc_lte_am_ack_sn, tvb,
                                bit_offset, 10, &ack_sn, FALSE);
    bit_offset += 10;

    /* E1 */
    proto_tree_add_bits_ret_val(tree, hf_rlc_lte_am_e1, tvb,
                                bit_offset, 1, &e1, FALSE);

    /* Skip another bit to byte-align the next bit... */
    bit_offset++;

    /* Optional, extra fields */
    do {
        if (e1) {
            /****************************/
            /* Read NACK_SN, E1, E2     */

            /* ACK_SN */
            proto_tree_add_bits_ret_val(tree, hf_rlc_lte_am_nack_sn, tvb,
                                        bit_offset, 10, &nack_sn, FALSE);
            bit_offset += 10;

            /* E1 */
            proto_tree_add_bits_ret_val(tree, hf_rlc_lte_am_e1, tvb,
                                        bit_offset, 1, &e1, FALSE);
            bit_offset++;

            /* E2 */
            proto_tree_add_bits_ret_val(tree, hf_rlc_lte_am_e2, tvb,
                                        bit_offset, 1, &e2, FALSE);
            bit_offset++;

            /* Reset this flag here */
            e1 = 0;
        }
        if (e2) {
            /* Read SOstart, SOend */
            proto_tree_add_bits_ret_val(tree, hf_rlc_lte_am_so_start, tvb,
                                        bit_offset, 15, &so_start, FALSE);
            bit_offset += 15;

            proto_tree_add_bits_ret_val(tree, hf_rlc_lte_am_so_end, tvb,
                                        bit_offset, 15, &so_end, FALSE);
            bit_offset += 15;

            /* Reset this flag here */
            e2 = 0;
        }
    } while (e1 || e2);

}


/***************************************************/
/* Acknowledged mode PDU                           */
static void dissect_rlc_lte_am(tvbuff_t *tvb, packet_info *pinfo,
                               proto_tree *tree,
                               int offset)
{
    guint8 is_data;
    guint8 is_segment;
    guint8 fixed_extension;
    guint8 framing_info;
    guint8 first_includes_start;
    guint8 last_includes_end;
    proto_tree *am_header_tree;
    proto_item *am_header_ti;
    gint   start_offset = offset;
    guint16    sn;

    /* Add UM header subtree */
    am_header_ti = proto_tree_add_string_format(tree,
                                                hf_rlc_lte_am_header,
                                                tvb, offset, 0,
                                                "",
                                                "AM header");
    am_header_tree = proto_item_add_subtree(am_header_ti,
                                            ett_rlc_lte_am_header);


    /*******************************************/
    /* First bit is Data/Control flag           */
    is_data = (tvb_get_guint8(tvb, offset) & 0x80) >> 7;
    proto_tree_add_item(am_header_tree, hf_rlc_lte_am_data_control, tvb, offset, 1, FALSE);
    if (check_col(pinfo->cinfo, COL_INFO)) {
        col_append_str(pinfo->cinfo, COL_INFO, (is_data) ? " [DATA]" : " [CONTROL]");
    }


    /**************************************************/
    /* Control PDUs are a completely separate format  */
    if (!is_data) {
        dissect_rlc_lte_am_status_pdu(tvb, pinfo, am_header_tree, offset);
        return;
    }


    /******************************/
    /* Data PDU fixed header      */

    /* Re-segmentation Flag (RF) field */
    is_segment = (tvb_get_guint8(tvb, offset) & 0x40) >> 6;
    proto_tree_add_item(am_header_tree, hf_rlc_lte_am_rf, tvb, offset, 1, FALSE);

    /* Polling bit */
    proto_tree_add_item(am_header_tree, hf_rlc_lte_am_p, tvb, offset, 1, FALSE);

    /* Framing Info */
    framing_info = (tvb_get_guint8(tvb, offset) & 0x18) >> 3;
    proto_tree_add_item(am_header_tree, hf_rlc_lte_am_fi, tvb, offset, 1, FALSE);

    /* Extension bit */
    fixed_extension = (tvb_get_guint8(tvb, offset) & 0x04) >> 2;
    proto_tree_add_item(am_header_tree, hf_rlc_lte_am_fixed_e, tvb, offset, 1, FALSE);

    /* Sequence Number */
    sn = tvb_get_ntohs(tvb, offset) & 0x03ff;
    proto_tree_add_item(am_header_tree, hf_rlc_lte_am_fixed_sn, tvb, offset, 2, FALSE);

    offset += 2;

    /* Show SN in AM header root */
    proto_item_append_text(am_header_ti, " (SN=%u)", sn);
    proto_item_set_len(am_header_ti, offset-start_offset);

    /***************************************/
    /* Dissect extra segment header fields */
    if (is_segment) {
        /* Last Segment Field (LSF) */
        proto_tree_add_item(am_header_tree, hf_rlc_lte_am_segment_lsf, tvb, offset, 1, FALSE);

        /* SO */
        proto_tree_add_item(am_header_tree, hf_rlc_lte_am_segment_so, tvb, offset, 2, FALSE);

        offset += 2;
    }

    /*************************************/
    /* AM header extension               */
    if (fixed_extension) {
        offset = dissect_rlc_lte_extension_header(tvb, pinfo, tree, offset);
    }


    /* Extract these 2 flags from framing_info */
    first_includes_start = (framing_info & 0x02) == 0;
    last_includes_end =    (framing_info & 0x01) == 0;


    /*************************************/
    /* Data                        */
    if (s_number_of_extensions > 0) {
        /* Show each data segment separately */
        int n;
        for (n=0; n < s_number_of_extensions; n++) {
            proto_tree_add_item(tree, hf_rlc_lte_am_data, tvb, offset, s_lengths[n], FALSE);
            show_PDU_in_info(pinfo, s_lengths[n],
                             (n==0) ? first_includes_start : TRUE,
                             TRUE);
            offset += s_lengths[n];
        }
    }

    /* Final data element */
    proto_tree_add_item(tree, hf_rlc_lte_am_data, tvb, offset, -1, FALSE);
    show_PDU_in_info(pinfo, tvb_length_remaining(tvb, offset),
                     (s_number_of_extensions == 0) ? first_includes_start : TRUE,
                     last_includes_end);
}



/*****************************/
/* Main dissection function. */
/*****************************/

void dissect_rlc_lte(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
    proto_tree             *rlc_lte_tree;
    proto_item             *ti;
    proto_item             *mode_ti;
    gint                   offset = 0;
    struct rlc_lte_info    *p_rlc_lte_info = NULL;

    /* Set protocol name */
    if (check_col(pinfo->cinfo, COL_PROTOCOL)) {
        col_set_str(pinfo->cinfo, COL_PROTOCOL, "RLC-LTE");
    }

    /* Create protocol tree. */
    ti = proto_tree_add_item(tree, proto_rlc_lte, tvb, offset, -1, FALSE);
    rlc_lte_tree = proto_item_add_subtree(ti, ett_rlc_lte);


    /* Look for packet info! */
    p_rlc_lte_info = p_get_proto_data(pinfo->fd, proto_rlc_lte);

    /* Can't dissect anything without it... */
    if (p_rlc_lte_info == NULL) {
        proto_item *ti =
            proto_tree_add_text(rlc_lte_tree, tvb, offset, -1,
                                "Can't dissect LTE RLC frame because no per-frame info was attached!");
        PROTO_ITEM_SET_GENERATED(ti);
        return;
    }

    /*****************************************/
    /* Show context information              */
    /* TODO: hide inside own tree?           */

    ti = proto_tree_add_uint(rlc_lte_tree, hf_rlc_lte_context_direction,
                             tvb, 0, 0, p_rlc_lte_info->direction);
    PROTO_ITEM_SET_GENERATED(ti);

    mode_ti = proto_tree_add_uint(rlc_lte_tree, hf_rlc_lte_context_mode,
                                  tvb, 0, 0, p_rlc_lte_info->rlcMode);
    PROTO_ITEM_SET_GENERATED(mode_ti);

    ti = proto_tree_add_uint(rlc_lte_tree, hf_rlc_lte_context_ueid,
                             tvb, 0, 0, p_rlc_lte_info->ueid);
    PROTO_ITEM_SET_GENERATED(ti);

    ti = proto_tree_add_uint(rlc_lte_tree, hf_rlc_lte_context_priority,
                             tvb, 0, 0, p_rlc_lte_info->priority);
    PROTO_ITEM_SET_GENERATED(ti);

    ti = proto_tree_add_uint(rlc_lte_tree, hf_rlc_lte_context_channel_type,
                             tvb, 0, 0, p_rlc_lte_info->channelType);
    PROTO_ITEM_SET_GENERATED(ti);

    ti = proto_tree_add_uint(rlc_lte_tree, hf_rlc_lte_context_channel_id,
                             tvb, 0, 0, p_rlc_lte_info->channelId);
    PROTO_ITEM_SET_GENERATED(ti);

    ti = proto_tree_add_uint(rlc_lte_tree, hf_rlc_lte_context_pdu_length,
                             tvb, 0, 0, p_rlc_lte_info->pduLength);
    PROTO_ITEM_SET_GENERATED(ti);

    if (p_rlc_lte_info->rlcMode == RLC_UM_MODE) {
        ti = proto_tree_add_uint(rlc_lte_tree, hf_rlc_lte_context_um_sn_length,
                                 tvb, 0, 0, p_rlc_lte_info->UMSequenceNumberLength);
        PROTO_ITEM_SET_GENERATED(ti);
    }


    /* Append context highlights to info column */
    if (check_col(pinfo->cinfo, COL_INFO)) {
        col_add_fstr(pinfo->cinfo, COL_INFO,
                     "[%s] [%s] UEId=%u %s:%u",
                     (p_rlc_lte_info->direction == 0) ? "UL" : "DL",
                     val_to_str(p_rlc_lte_info->rlcMode, rlc_mode_short_vals, "Unknown"),
                     p_rlc_lte_info->ueid,
                     val_to_str(p_rlc_lte_info->channelType, rlc_channel_type_vals, "Unknown"),
                     p_rlc_lte_info->channelId);
    }

    /* Reset this count */
    s_number_of_extensions = 0;

    /* Dissect the RLC PDU itself. Format depends upon mode... */
    switch (p_rlc_lte_info->rlcMode) {

        case RLC_TM_MODE:
            /* Remaining bytes are all data */
            proto_tree_add_item(rlc_lte_tree, hf_rlc_lte_tm_data, tvb, offset, -1, FALSE);
            if (check_col(pinfo->cinfo, COL_INFO)) {
                col_append_fstr(pinfo->cinfo, COL_INFO, "   [%u-bytes]",
                               tvb_length_remaining(tvb, offset));
            }
            break;

        case RLC_UM_MODE:
            dissect_rlc_lte_um(tvb, pinfo, rlc_lte_tree, offset, p_rlc_lte_info);
            break;

        case RLC_AM_MODE:
            dissect_rlc_lte_am(tvb, pinfo, rlc_lte_tree, offset);
            break;

        default:
            /* Error - unrecognised mode */
            expert_add_info_format(pinfo, mode_ti, PI_MALFORMED, PI_ERROR,
                                   "Unrecognised RLC Mode set (%u)", p_rlc_lte_info->rlcMode);
            break;
    }
}


void proto_register_rlc_lte(void)
{
    static hf_register_info hf[] =
    {
        /**********************************/
        /* Items for decoding context     */
        { &hf_rlc_lte_context_mode,
            { "RLC Mode",
              "rlc-lte.mode", FT_UINT8, BASE_DEC, VALS(rlc_mode_vals), 0x0,
              "RLC Mode", HFILL
            }
        },
        { &hf_rlc_lte_context_direction,
            { "Direction",
              "rlc-lte.direction", FT_UINT8, BASE_DEC, VALS(direction_vals), 0x0,
              "Direction of message", HFILL
            }
        },
        { &hf_rlc_lte_context_priority,
            { "Priority",
              "rlc-lte.priority", FT_UINT8, BASE_DEC, 0, 0x0,
              "Priority", HFILL
            }
        },
        { &hf_rlc_lte_context_ueid,
            { "UEId",
              "rlc-lte.ueid", FT_UINT16, BASE_DEC, 0, 0x0,
              "User Equipment Identifier associated with message", HFILL
            }
        },
        { &hf_rlc_lte_context_channel_type,
            { "Channel Type",
              "rlc-lte.channel-type", FT_UINT16, BASE_DEC, VALS(rlc_channel_type_vals), 0x0,
              "Channel Type associated with message", HFILL
            }
        },
        { &hf_rlc_lte_context_channel_id,
            { "Channel ID",
              "rlc-lte.channel-id", FT_UINT16, BASE_DEC, 0, 0x0,
              "Channel ID associated with message", HFILL
            }
        },
        { &hf_rlc_lte_context_pdu_length,
            { "PDU Length",
              "rlc-lte.pdu_length", FT_UINT16, BASE_DEC, 0, 0x0,
              "Length of PDU (in bytes)", HFILL
            }
        },
        { &hf_rlc_lte_context_um_sn_length,
            { "UM Sequence number length",
              "rlc-lte.um-seqnum-length", FT_UINT8, BASE_DEC, 0, 0x0,
              "Length of UM sequence number in bits", HFILL
            }
        },


        /* Transparent mode fields */
        { &hf_rlc_lte_tm_data,
            { "TM Data",
              "rlc-lte.tm.data", FT_BYTES, BASE_HEX, 0, 0x0,
              "Transparent Mode Data", HFILL
            }
        },

        /* Unacknowledged mode fields */
        { &hf_rlc_lte_um_header,
            { "UM Header",
              "rlc-lte.um.header", FT_STRING, BASE_NONE, NULL, 0x0,
              "Unackowledged Mode Header", HFILL
            }
        },
        { &hf_rlc_lte_um_fi,
            { "Framing Info",
              "rlc-lte.um.fi", FT_UINT8, BASE_HEX, VALS(framing_info_vals), 0x0,
              "Framing Info", HFILL
            }
        },
        { &hf_rlc_lte_um_fixed_e,
            { "Extension",
              "rlc-lte.um.fixed.e", FT_UINT8, BASE_HEX, VALS(fixed_extension_vals), 0x0,
              "Extension in fixed part of UM header", HFILL
            }
        },
        { &hf_rlc_lte_um_sn,
            { "Sequence number",
              "rlc-lte.um.sn", FT_UINT8, BASE_DEC, 0, 0x0,
              "Unacknowledged Mode Sequence Number", HFILL
            }
        },
        { &hf_rlc_lte_um_fixed_reserved,
            { "Reserved",
              "rlc-lte.um.reserved", FT_UINT8, BASE_DEC, 0, 0xe0,
              "Unacknowledged Mode Fixed header reserved bits", HFILL
            }
        },
        { &hf_rlc_lte_um_data,
            { "UM Data",
              "rlc-lte.um.data", FT_BYTES, BASE_HEX, 0, 0x0,
              "Unacknowledged Mode Data", HFILL
            }
        },
        { &hf_rlc_lte_extension_part,
            { "Extension Part",
              "rlc-lte.extension-part", FT_STRING, BASE_NONE, 0, 0x0,
              "Extension Part", HFILL
            }
        },


        { &hf_rlc_lte_extension_e,
            { "Extension",
              "rlc-lte.extension.e", FT_UINT8, BASE_HEX, VALS(extension_extension_vals), 0x0,
              "Extension in extended part of the header", HFILL
            }
        },
        { &hf_rlc_lte_extension_li,
            { "Length Indicator",
              "rlc-lte.extension.li", FT_UINT16, BASE_DEC, 0, 0x0,
              "Length Indicator", HFILL
            }
        },
        { &hf_rlc_lte_extension_padding,
            { "Padding",
              "rlc-lte.extension.padding", FT_UINT8, BASE_HEX, 0, 0x0f,
              "Extension header padding", HFILL
            }
        },


        { &hf_rlc_lte_am_header,
            { "UM Header",
              "rlc-lte.am.header", FT_STRING, BASE_NONE, NULL, 0x0,
              "Ackowledged Mode Header", HFILL
            }
        },
        { &hf_rlc_lte_am_data_control,
            { "Frame type",
              "rlc-lte.am.frame_type", FT_UINT8, BASE_HEX, VALS(data_or_control_vals), 0x80,
              "AM Frame Type (Control or Data)", HFILL
            }
        },
        { &hf_rlc_lte_am_rf,
            { "Re-segmentation Flag",
              "rlc-lte.am.rf", FT_UINT8, BASE_HEX, VALS(resegmentation_flag_vals), 0x40,
              "AM Re-segmentation Flag", HFILL
            }
        },
        { &hf_rlc_lte_am_p,
            { "Polling Bit",
              "rlc-lte.am.p", FT_UINT8, BASE_HEX, VALS(polling_bit_vals), 0x20,
              "Polling Bit", HFILL
            }
        },
        { &hf_rlc_lte_am_fi,
            { "Framing Info",
              "rlc-lte.am.fi", FT_UINT8, BASE_HEX, VALS(framing_info_vals), 0x18,
              "AM Framing Info", HFILL
            }
        },
        { &hf_rlc_lte_am_fixed_e,
            { "Extension",
              "rlc-lte.am.fixed.e", FT_UINT8, BASE_HEX, VALS(fixed_extension_vals), 0x04,
              "Fixed Extension Bit", HFILL
            }
        },
        { &hf_rlc_lte_am_fixed_sn,
            { "Sequence Number",
              "rlc-lte.am.fixed.sn", FT_UINT16, BASE_HEX, 0, 0x03ff,
              "AM Fixed Sequence Number", HFILL
            }
        },
        { &hf_rlc_lte_am_segment_lsf,
            { "Last Segment Flag",
              "rlc-lte.am.segment.lsf", FT_UINT8, BASE_HEX, VALS(lsf_vals), 0x80,
              "Last Segment Flag", HFILL
            }
        },
        { &hf_rlc_lte_am_segment_so,
            { "Segment Offset",
              "rlc-lte.am.segment.so", FT_UINT16, BASE_HEX, 0, 0x7fff,
              "Segment Offset", HFILL
            }
        },
        { &hf_rlc_lte_am_data,
            { "AM Data",
              "rlc-lte.am.data", FT_BYTES, BASE_HEX, 0, 0x0,
              "Acknowledged Mode Data", HFILL
            }
        },


        { &hf_rlc_lte_am_cpt,
            { "Control PDU Type",
              "rlc-lte.am.cpt", FT_UINT8, BASE_HEX, VALS(control_pdu_type_vals), 0x70,
              "AM Control PDU Type", HFILL
            }
        },
        { &hf_rlc_lte_am_ack_sn,
            { "ACK Sequence Number",
              "rlc-lte.am.ack-sn", FT_UINT16, BASE_DEC, 0, 0x0ffc,
              "Sequence Number we're next expecting to receive", HFILL
            }
        },
        { &hf_rlc_lte_am_e1,
            { "Extension bit 1",
              "rlc-lte.am.e1", FT_UINT8, BASE_HEX, VALS(am_e1_vals), 0x0,
              "Extension bit 1", HFILL
            }
        },
        { &hf_rlc_lte_am_e2,
            { "Extension bit 2",
              "rlc-lte.am.e2", FT_UINT8, BASE_HEX, VALS(am_e2_vals), 0x0,
              "Extension bit 2", HFILL
            }
        },
        { &hf_rlc_lte_am_nack_sn,
            { "NACK Sequence Number",
              "rlc-lte.am.nack-sn", FT_UINT16, BASE_DEC, 0, 0x0,
              "Negative Acknowledgement Sequence Number", HFILL
            }
        },
        { &hf_rlc_lte_am_so_start,
            { "SO Start",
              "rlc-lte.am.so-start", FT_UINT16, BASE_DEC, 0, 0x0,
              "SO Start", HFILL
            }
        },
        { &hf_rlc_lte_am_so_end,
            { "SO End",
              "rlc-lte.am.so-end", FT_UINT16, BASE_DEC, 0, 0x0,
              "SO End", HFILL
            }
        },

    };

    static gint *ett[] =
    {
        &ett_rlc_lte,
        &ett_rlc_lte_um_header,
        &ett_rlc_lte_am_header,
        &ett_rlc_lte_extension_part,
    };

    /* Register protocol. */
    proto_rlc_lte = proto_register_protocol("RLC-LTE", "RLC-LTE", "rlc-lte");
    proto_register_field_array(proto_rlc_lte, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));

    /* Allow other dissectors to find this one by name. */
    register_dissector("rlc-lte", dissect_rlc_lte, proto_rlc_lte);
}


