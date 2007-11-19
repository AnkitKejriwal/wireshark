/* packet-wimaxasncp.c
 *
 * Routines for WiMAX ASN Control Plane packet dissection dissection
 *
 * Copyright 2007, Mobile Metrics - http://mobilemetrics.net/
 *
 * Author: Stephen Croll <croll@mobilemetrics.net>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <glib.h>

#include <epan/packet.h>
#include <epan/prefs.h>
#include <epan/sminmpec.h>
#include <epan/addr_resolv.h>
#include <epan/ipproto.h>
#include <epan/expert.h>
#include <epan/filesystem.h>
#include <epan/report_err.h>
#include <epan/eap.h>

#include "wimaxasncp_dict.h"

/* Forward declarations we need below */
void proto_register_wimaxasncp(void);
void proto_reg_handoff_wimaxasncp(void);

/* Initialize the protocol and registered fields */
static int proto_wimaxasncp                     = -1;
static int hf_wimaxasncp_version                = -1;
static int hf_wimaxasncp_flags                  = -1;
static int hf_wimaxasncp_function_type          = -1;
static int hf_wimaxasncp_op_id                  = -1;
static int hf_wimaxasncp_message_type           = -1;
static int hf_wimaxasncp_qos_msg                = -1;
static int hf_wimaxasncp_ho_control_msg         = -1;
static int hf_wimaxasncp_data_path_control_msg  = -1;
static int hf_wimaxasncp_context_delivery_msg   = -1;
static int hf_wimaxasncp_r3_mobility_msg        = -1;
static int hf_wimaxasncp_paging_msg             = -1;
static int hf_wimaxasncp_rrm_msg                = -1;
static int hf_wimaxasncp_authentication_msg     = -1;
static int hf_wimaxasncp_ms_state_msg           = -1;
static int hf_wimaxasncp_reauthentication_msg   = -1;
static int hf_wimaxasncp_session_msg            = -1;
static int hf_wimaxasncp_length                 = -1;
static int hf_wimaxasncp_msid                   = -1;
static int hf_wimaxasncp_reserved1              = -1;
static int hf_wimaxasncp_transaction_id         = -1;
static int hf_wimaxasncp_reserved2              = -1;
static int hf_wimaxasncp_tlv                    = -1;
static int hf_wimaxasncp_tlv_type               = -1;
static int hf_wimaxasncp_tlv_length             = -1;
static int hf_wimaxasncp_tlv_value_bytes        = -1;
static int hf_wimaxasncp_tlv_value_bitflags16   = -1;
static int hf_wimaxasncp_tlv_value_bitflags32   = -1;
static int hf_wimaxasncp_tlv_value_protocol     = -1;
static int hf_wimaxasncp_tlv_value_vendor_id    = -1;

/* Preferences */
static gboolean show_transaction_id_d_bit      = FALSE;
static gboolean debug_enabled                  = FALSE;

/* Default WiMAX ASN control protocol port */
#define WIMAXASNCP_DEF_UDP_PORT     2231
static guint global_wimaxasncp_udp_port = WIMAXASNCP_DEF_UDP_PORT;


/* Initialize the subtree pointers */
static gint ett_wimaxasncp                                       = -1;
static gint ett_wimaxasncp_flags                                 = -1;
static gint ett_wimaxasncp_tlv                                   = -1;
static gint ett_wimaxasncp_tlv_value_bitflags16                  = -1;
static gint ett_wimaxasncp_tlv_value_bitflags32                  = -1;
static gint ett_wimaxasncp_tlv_protocol_list                     = -1;
static gint ett_wimaxasncp_tlv_port_range_list                   = -1;
static gint ett_wimaxasncp_tlv_ip_address_mask_list              = -1;
static gint ett_wimaxasncp_tlv_ip_address_mask                   = -1;
static gint ett_wimaxasncp_tlv_eap                               = -1;
static gint ett_wimaxasncp_tlv_vendor_specific_information_field = -1;

/* Header size, up to, but not including, the TLV fields. */
#define WIMAXASNCP_HEADER_SIZE       20

/* Offset to end of the length field in the headder. */
#define WIMAXASNCP_HEADER_LENGTH_END 6

#define WIMAXASNCP_BIT32(n) (1 << (31 - (n)))
#define WIMAXASNCP_BIT16(n) (1 << (15 - (n)))
#define WIMAXASNCP_BIT8(n)  (1 << ( 7 - (n)))

#define WIMAXASNCP_FLAGS_T  WIMAXASNCP_BIT8(6)
#define WIMAXASNCP_FLAGS_R  WIMAXASNCP_BIT8(7)

typedef struct {
    GArray* hf;
    GArray* ett;
} wimaxasncp_build_dict_t;

static wimaxasncp_dict_t *wimaxasncp_dict = NULL;

wimaxasncp_build_dict_t wimaxasncp_build_dict;

static wimaxasncp_dict_tlv_t wimaxasncp_tlv_not_found =
{
    0, "Unknown", NULL, WIMAXASNCP_TLV_UNKNOWN,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    NULL, NULL, NULL
};

static dissector_handle_t eap_handle;

/* ------------------------------------------------------------------------- */

static const value_string wimaxasncp_flag_vals[] =
{
    { WIMAXASNCP_BIT8(0), "Reserved" },
    { WIMAXASNCP_BIT8(1), "Reserved" },
    { WIMAXASNCP_BIT8(2), "Reserved" },
    { WIMAXASNCP_BIT8(3), "Reserved" },
    { WIMAXASNCP_BIT8(4), "Reserved" },
    { WIMAXASNCP_BIT8(5), "Reserved" },
    { WIMAXASNCP_FLAGS_T, "T - Source and Destination Identifier TLVs"},
    { WIMAXASNCP_FLAGS_R, "R - Reset Next Expected Transaction ID"},
    { 0,                  NULL}
};

/* ------------------------------------------------------------------------- */

static const value_string wimaxasncp_op_id_vals[] =
{
    { 0,   "Invalid"},
    { 1,   "Request/Initiation"},
    { 2,   "Response"},
    { 3,   "Ack"},
    { 4,   "Indication"},
    { 5,   "Reserved"},
    { 6,   "Reserved"},
    { 7,   "Reserved"},
    { 0,   NULL}
};

/* ------------------------------------------------------------------------- */

#define WIMAXASNCP_FT_QOS                 1
#define WIMAXASNCP_FT_HO_CONTROL          2
#define WIMAXASNCP_FT_DATA_PATH_CONTROL   3
#define WIMAXASNCP_FT_CONTEXT_DELIVERY    4
#define WIMAXASNCP_FT_R3_MOBILITY         5
#define WIMAXASNCP_FT_PAGING              6
#define WIMAXASNCP_FT_RRM                 7
#define WIMAXASNCP_FT_AUTHENTICATION      8
#define WIMAXASNCP_FT_MS_STATE            9
#define WIMAXASNCP_FT_REAUTHENTICATION    10
#define WIMAXASNCP_FT_SESSION             11    /* Nokia recommended value */

/* ------------------------------------------------------------------------- */

static const value_string wimaxasncp_function_type_vals[] =
{
    { WIMAXASNCP_FT_QOS,               "QoS"},
    { WIMAXASNCP_FT_HO_CONTROL,        "HO Control"},
    { WIMAXASNCP_FT_DATA_PATH_CONTROL, "Data Path Control"},
    { WIMAXASNCP_FT_CONTEXT_DELIVERY,  "Context Delivery"},
    { WIMAXASNCP_FT_R3_MOBILITY,       "R3 Mobility"},
    { WIMAXASNCP_FT_PAGING,            "Paging"},
    { WIMAXASNCP_FT_RRM,               "RRM"},
    { WIMAXASNCP_FT_AUTHENTICATION,    "Authentication"},
    { WIMAXASNCP_FT_MS_STATE,          "MS State"},
    { WIMAXASNCP_FT_REAUTHENTICATION,  "Re-Authentication"},
    { WIMAXASNCP_FT_SESSION,           "Session"},
    { 0,    NULL}
};

/* ------------------------------------------------------------------------- */

static const value_string wimaxasncp_qos_msg_vals[] =
{
    { 1,  "RR_Ack"},
    { 2,  "RR_Req"},
    { 3,  "RR_Rsp"},
    { 0,   NULL}
};

/* ------------------------------------------------------------------------- */

static const value_string wimaxasncp_ho_control_msg_vals[] =
{
    { 1,  "HO_Ack"},
    { 2,  "HO_Complete"},
    { 3,  "HO_Cnf"},
    { 4,  "HO_Req"},
    { 5,  "HO_Rsp"},
    { 0,   NULL}
};

/* ------------------------------------------------------------------------- */

static const value_string wimaxasncp_data_path_control_msg_vals[] =
{
    { 1,   "Path_Dereg_Ack"},
    { 2,   "Path_Dereg_Req"},
    { 3,   "Path_Dereg_Rsp"},
    { 4,   "Path_Modification_Ack"},
    { 5,   "Path_Modification_Req"},
    { 6,   "Path_Modification_Rsp"},
    { 7,   "Path_Prereg_Ack"},
    { 8,   "Path_Prereg_Req"},
    { 9,   "Path_Prereg_Rsp"},
    { 10,  "Path_Reg_Ack"},
    { 11,  "Path_Reg_Req"},
    { 12,  "Path_Reg_Rsp"},

    /* see also wimaxasncp_ms_state_msg_vals[] */
    { 13,  "MS_Attachment_Req (DPC)"},
    { 14,  "MS_Attachment_Rsp (DPC)"},
    { 15,  "MS_Attachment_Ack (DPC)"},

    { 16,  "Key_Change_Directive"},
    { 0,   NULL}
};

/* ------------------------------------------------------------------------- */

static const value_string wimaxasncp_context_delivery_msg_vals[] =
{
    { 1,  "Context_Rpt"},
    { 2,  "Context_Req"},
    { 3,  "Context_Ack"},
    { 0,   NULL}
};

/* ------------------------------------------------------------------------- */

static const value_string wimaxasncp_r3_mobility_msg_vals[] =
{
    { 1,  "Anchor_DPF_HO_Req"},
    { 2,  "Anchor_DPF_HO_Trigger"},
    { 3,  "Anchor_DPF_HO_Rsp"},
    { 4,  "Anchor_DPF_Relocate_Req"},
    { 5,  "FA_Register_Req"},
    { 6,  "FA_Register_Rsp"},
    { 7,  "Anchor_DPF_Relocate_Rsp"},
    { 8,  "FA_Revoke_Req"},
    { 9,  "FA_Revoke_Rsp"},
    { 0,   NULL}
};

/* ------------------------------------------------------------------------- */

static const value_string wimaxasncp_paging_msg_vals[] =
{
    { 1,  "Initiate_Paging_Req"},
    { 2,  "Initiate_Paging_Rsp"},
    { 3,  "LU_Cnf"},
    { 4,  "LU_Req"},
    { 5,  "LU_Rsp"},
    { 6,  "Paging_Announce"},
    { 7,  "CMAC_Key_Count_Req"},
    { 8,  "CMAC_Key_Count_Rsp"},
    { 0,   NULL}
};

/* ------------------------------------------------------------------------- */

static const value_string wimaxasncp_rrm_msg_vals[] =
{
    { 1,  "R6 PHY_Parameters_Req"},
    { 2,  "R6 PHY_Parameters_Rpt"},
    { 3,  "R4/R6 Spare_Capacity_Req"},
    { 4,  "R4/R6 Spare_Capacity_Rpt"},
    { 5,  "R6 Neighbor_BS_Resource_Status_Update"},
    { 6,  "R4/R6 Radio_Config_Update_Req"},
    { 7,  "R4/R6 Radio_Config_Update_Rpt"},
    { 0,   NULL}
};

/* ------------------------------------------------------------------------- */

static const value_string wimaxasncp_authentication_msg_vals[] =
{
    { 1,  "AR_Authenticated_EAP_Start"},
    { 2,  "AR_Authenticated_EAP_Transfer"},
    { 3,  "AR_EAP_Start"},
    { 4,  "AR_EAP_Transfer"},
    { 5,  "AR_EAP_Complete"},
    { 6,  "CMAC_Key_Count_Update"},       /* Nokia recommended value */
    { 7,  "CMAC_Key_Count_Update_Ack"},   /* Nokia recommended value */
    { 0,   NULL}
};

/* ------------------------------------------------------------------------- */

static const value_string wimaxasncp_ms_state_msg_vals[] =
{
    { 1,  "IM_Entry_State_Change_Req"},
    { 2,  "IM_Entry_State_Change_Rsp"},
    { 3,  "IM_Exit_State_Change_Req"},
    { 4,  "IM_Exit_State_Change_Rsp"},
    { 5,  "NW_ReEntry_State_Change_Directive"},
    { 6,  "MS_PreAttachment_Req"},
    { 7,  "MS_PreAttachment_Rsp"},
    { 8,  "MS_PreAttachment_Ack"},
    { 9,  "MS_Attachment_Req"},
    { 10, "MS_Attachment_Rsp"},
    { 11, "MS_Attachment_Ack"},
    { 12, "IM_Entry_State_Change_Ack"},
    { 0,   NULL}
};

/* ------------------------------------------------------------------------- */

static const value_string wimaxasncp_reauthentication_msg_vals[] =
{
    { 1,  "AR_EAP_Start"},
    { 2,  "Key_Change_Directive"},
    { 3,  "Key_Change_Cnf"},
    { 4,  "Relocation_Cnf"},
    { 5,  "Relocation_Confirm_Ack"},
    { 6,  "Relocation_Notify"},
    { 7,  "Relocation_Notify_Ack"},
    { 8,  "Relocation_Req"},
    { 9,  "Relocation_Rsp"},
    { 10, "Key_Change_Ack"},
    { 0,   NULL}
};

/* ------------------------------------------------------------------------- */

static const value_string wimaxasncp_session_msg_vals[] =
{
    { 1,  "Session_Release_Req"},  /* Nokia recommended value */
    { 2,  "Session_Release_Rsp"},  /* Nokia recommended value */
    { 3,  "Session_Release_Ack"},  /* Nokia recommended value */
    { 4,  "Session_Failure_Rpt"},  /* Nokia recommended value */
    { 5,  "Session_Failure_Rsp"},  /* Nokia recommended value */
    { 0,   NULL}
};

/* ========================================================================= */

typedef struct {
    guint8 function_type;
    const value_string *vals;
} wimaxasncp_func_msg_t;

/* ------------------------------------------------------------------------ */

static const wimaxasncp_func_msg_t wimaxasncp_func_to_msg_vals_map[] =
{
    { WIMAXASNCP_FT_QOS,               wimaxasncp_qos_msg_vals },
    { WIMAXASNCP_FT_HO_CONTROL,        wimaxasncp_ho_control_msg_vals },
    { WIMAXASNCP_FT_DATA_PATH_CONTROL, wimaxasncp_data_path_control_msg_vals },
    { WIMAXASNCP_FT_CONTEXT_DELIVERY,  wimaxasncp_context_delivery_msg_vals },
    { WIMAXASNCP_FT_R3_MOBILITY,       wimaxasncp_r3_mobility_msg_vals },
    { WIMAXASNCP_FT_PAGING,            wimaxasncp_paging_msg_vals },
    { WIMAXASNCP_FT_RRM,               wimaxasncp_rrm_msg_vals },
    { WIMAXASNCP_FT_AUTHENTICATION,    wimaxasncp_authentication_msg_vals },
    { WIMAXASNCP_FT_MS_STATE,          wimaxasncp_ms_state_msg_vals },
    { WIMAXASNCP_FT_REAUTHENTICATION,  wimaxasncp_reauthentication_msg_vals },
    { WIMAXASNCP_FT_SESSION,           wimaxasncp_session_msg_vals }
};

/* ========================================================================= */

static const wimaxasncp_dict_tlv_t *wimaxasncp_get_tlv_info(
    guint16 type)
{
    if (wimaxasncp_dict)
    {
        wimaxasncp_dict_tlv_t *tlv;

        for (tlv = wimaxasncp_dict->tlvs; tlv; tlv = tlv->next)
        {
            if (tlv->type == type)
            {
                return tlv;
            }
        }
    }

    if (debug_enabled)
    {
        g_print("fix-me: unknown TLV type: %u\n", type);
    }

    return &wimaxasncp_tlv_not_found;
}

/* ========================================================================= */

static const gchar *wimaxasncp_get_enum_name(
    const wimaxasncp_dict_tlv_t *tlv_info,
    guint32 code)
{
    if (tlv_info->enum_vs)
    {
        return val_to_str(code, tlv_info->enum_vs, "Unknown");
    }
    else
    {
        return "Unknown";
    }
}

/* ========================================================================= */

static const value_string wimaxasncp_decode_type_vals[] =
{
  { WIMAXASNCP_TLV_UNKNOWN,             "WIMAXASNCP_TLV_UNKNOWN"},
  { WIMAXASNCP_TLV_TBD,                 "WIMAXASNCP_TLV_TBD"},
  { WIMAXASNCP_TLV_COMPOUND,            "WIMAXASNCP_TLV_COMPOUND"},
  { WIMAXASNCP_TLV_BYTES,               "WIMAXASNCP_TLV_BYTES"},
  { WIMAXASNCP_TLV_ENUM8,               "WIMAXASNCP_TLV_ENUM8"},
  { WIMAXASNCP_TLV_ENUM16,              "WIMAXASNCP_TLV_ENUM16"},
  { WIMAXASNCP_TLV_ENUM32,              "WIMAXASNCP_TLV_ENUM32"},
  { WIMAXASNCP_TLV_ETHER,               "WIMAXASNCP_TLV_ETHER"},
  { WIMAXASNCP_TLV_ASCII_STRING,        "WIMAXASNCP_TLV_ASCII_STRING"},
  { WIMAXASNCP_TLV_FLAG0,               "WIMAXASNCP_TLV_FLAG0"},
  { WIMAXASNCP_TLV_BITFLAGS16,          "WIMAXASNCP_TLV_BITFLAGS16"},
  { WIMAXASNCP_TLV_BITFLAGS32,          "WIMAXASNCP_TLV_BITFLAGS32"},
  { WIMAXASNCP_TLV_ID,                  "WIMAXASNCP_TLV_ID"},
  { WIMAXASNCP_TLV_HEX8,                "WIMAXASNCP_TLV_HEX8"},
  { WIMAXASNCP_TLV_HEX16,               "WIMAXASNCP_TLV_HEX16"},
  { WIMAXASNCP_TLV_HEX32,               "WIMAXASNCP_TLV_HEX32"},
  { WIMAXASNCP_TLV_DEC8,                "WIMAXASNCP_TLV_DEC8"},
  { WIMAXASNCP_TLV_DEC16,               "WIMAXASNCP_TLV_DEC16"},
  { WIMAXASNCP_TLV_DEC32,               "WIMAXASNCP_TLV_DEC32"},
  { WIMAXASNCP_TLV_IP_ADDRESS,          "WIMAXASNCP_TLV_IP_ADDRESS"},
  { WIMAXASNCP_TLV_IPV4_ADDRESS,        "WIMAXASNCP_TLV_IPV4_ADDRESS"},
  { WIMAXASNCP_TLV_PROTOCOL_LIST,       "WIMAXASNCP_TLV_PROTOCOL_LIST"},
  { WIMAXASNCP_TLV_PORT_RANGE_LIST,     "WIMAXASNCP_TLV_PORT_RANGE_LIST"},
  { WIMAXASNCP_TLV_IP_ADDRESS_MASK_LIST,"WIMAXASNCP_TLV_IP_ADDRESS_MASK_LIST"},
  { WIMAXASNCP_TLV_VENDOR_SPECIFIC,     "WIMAXASNCP_TLV_VENDOR_SPECIFIC"},
  { 0, NULL}
};

/* ========================================================================= */

static void wimaxasncp_proto_tree_add_tlv_ipv4_value(
    tvbuff_t *tvb,
    proto_tree *tree,
    proto_item *tlv_item,
    guint offset,
    const wimaxasncp_dict_tlv_t *tlv_info)
{
    int hf_value;
    guint32 ip;
    const gchar *hostname;
    const gchar *ip_str;

    if (tlv_info->hf_ipv4 != -1)
    {
        hf_value = tlv_info->hf_ipv4;
    }
    else
    {
        hf_value = tlv_info->hf_value;
    }

    ip = tvb_get_ipv4(tvb, offset);
    hostname = get_hostname(ip);
    ip_str = ip_to_str((guint8 *)&ip);

    proto_tree_add_ipv4_format(
        tree, hf_value,
        tvb, offset, 4, ip,
        "Value: %s (%s)", hostname, ip_str);

    proto_item_append_text(
        tlv_item, " - %s (%s)",
        hostname, ip_str);
}

/* ========================================================================= */

static void wimaxasncp_proto_tree_add_tlv_ipv6_value(
    tvbuff_t *tvb,
    proto_tree *tree,
    proto_item *tlv_item,
    guint offset,
    const wimaxasncp_dict_tlv_t *tlv_info)
{
    int hf_value;
    struct e_in6_addr ip;
    const gchar *hostname;
    const gchar *ip_str;

    if (tlv_info->hf_ipv4 != -1)
    {
        hf_value = tlv_info->hf_ipv6;
    }
    else
    {
        hf_value = tlv_info->hf_value;
    }

    tvb_get_ipv6(tvb, offset, &ip);
    hostname = get_hostname6(&ip);
    ip_str = ip6_to_str(&ip);

    proto_tree_add_ipv6_format(
        tree, hf_value,
        tvb, offset, 16, (guint8 *)&ip,
        "Value: %s (%s)", hostname, ip_str);

    proto_item_append_text(
        tlv_item, " - %s (%s)",
        hostname, ip_str);
}

/* ========================================================================= */

static void wimaxasncp_proto_tree_add_ether_value(
    tvbuff_t *tvb,
    proto_tree *tree,
    proto_item *tlv_item,
    guint offset,
    guint length,
    const wimaxasncp_dict_tlv_t *tlv_info)
{
    int hf_value;
    const guint8 *p;
    const gchar *ether_name;
    const gchar *ether_str;

    if (tlv_info->hf_bsid != -1)
    {
        hf_value = tlv_info->hf_bsid;
    }
    else
    {
        hf_value = tlv_info->hf_value;
    }

    p = tvb_get_ptr(tvb, offset, length);
    ether_name = get_ether_name(p);
    ether_str = ether_to_str(p);

    proto_tree_add_ether_format(
        tree, hf_value,
        tvb, offset, length, p,
        "Value: %s (%s)",
        ether_name, ether_str);
    
    proto_item_append_text(
        tlv_item, " - %s (%s)",
        ether_name, ether_str);
}

/* ========================================================================= */

static void wimaxasncp_dissect_tlv_value(
    tvbuff_t *tvb,
    packet_info *pinfo _U_,
    proto_tree *tree,
    proto_item *tlv_item,
    const wimaxasncp_dict_tlv_t *tlv_info)
{
    guint offset = 0;
    guint length;
    const guint max_show_bytes = 24; /* arbitrary */
    const gchar *hex_note = "[hex]";
    const gchar *s;

    length = tvb_reported_length(tvb);

    switch(tlv_info->decoder)
    {
    case WIMAXASNCP_TLV_ENUM8:
    {
        if (length != 1)
        {
            /* encoding error */
            break;
        }

        if (tlv_info->enums == NULL)
        {
            if (debug_enabled)
            {
                g_print("fix-me: enum values missing for TLV %s (%u)\n",
                        tlv_info->name, tlv_info->type);
            }
        }

        if (tree)
        {
            guint8 value;

            value = tvb_get_guint8(tvb, offset);

            s = wimaxasncp_get_enum_name(tlv_info, value);

            proto_tree_add_uint_format(
                tree, tlv_info->hf_value,
                tvb, offset, length, value,
                "Value: %s (%u)", s, value);

            proto_item_append_text(tlv_item, " - %s", s);
        }

        return;
    }
    case WIMAXASNCP_TLV_ENUM16:
    {
        if (length != 2)
        {
            /* encoding error */
            break;
        }

        if (tlv_info->enums == NULL)
        {
            if (debug_enabled)
            {
                g_print("fix-me: enum values missing for TLV %s (%d)\n",
                        tlv_info->name, tlv_info->type);
            }
        }

        if (tree)
        {
            guint16 value;

            value = tvb_get_ntohs(tvb, offset);

            s = wimaxasncp_get_enum_name(tlv_info, value);

            proto_tree_add_uint_format(
                tree, tlv_info->hf_value,
                tvb, offset, length, value,
                "Value: %s (%u)", s, value);

            proto_item_append_text(tlv_item, " - %s", s);
        }

        return;
    }
    case WIMAXASNCP_TLV_ENUM32:
    {
        if (length != 4)
        {
            /* encoding error */
            break;
        }

        if (tlv_info->enums == NULL)
        {
            if (debug_enabled)
            {
                g_print("fix-me: enum values missing for TLV %s (%d)\n",
                        tlv_info->name, tlv_info->type);
            }
        }

        if (tree)
        {
            guint32 value;

            value = tvb_get_ntohl(tvb, offset);

            s = wimaxasncp_get_enum_name(tlv_info, value);

            proto_tree_add_uint_format(
                tree, tlv_info->hf_value,
                tvb, offset, length, value,
                "Value: %s (%u)", s, value);

            proto_item_append_text(tlv_item, " - %s", s);
        }

        return;
    }
    case WIMAXASNCP_TLV_ETHER:
    {
        if (length != 6)
        {
            /* encoding error */
            break;
        }

        if (tree)
        {
            wimaxasncp_proto_tree_add_ether_value(
                tvb, tree, tlv_item, offset, length, tlv_info);
        }

        return;
    }
    case WIMAXASNCP_TLV_ASCII_STRING:
    {
        if (tree)
        {
            const guint8 *p;
            const gchar *s = tvb_get_ephemeral_string(tvb, offset, length);

            p = tvb_get_ptr(tvb, offset, length);

            proto_tree_add_string_format(
                tree, tlv_info->hf_value,
                tvb, offset, length, p,
                "Value: %s", s);

            proto_item_append_text(
                tlv_item, " - %s", s);
        }

        return;
    }
    case WIMAXASNCP_TLV_FLAG0:
    {
        if (length != 0)
        {
            /* encoding error */
            break;
        }

        return;
    }
    case WIMAXASNCP_TLV_BITFLAGS16:
    {
        if (length != 2)
        {
            /* encoding error */
            break;
        }

        if (tlv_info->enums == NULL)
        {
            /* enum values missing */
        }

        if (tree)
        {
            proto_tree *flags_tree;
            proto_item *item;
            guint16 value;
            guint i;

            value = tvb_get_ntohs(tvb, offset);

            item = proto_tree_add_uint_format(
                tree, tlv_info->hf_value,
                tvb, offset, length, value,
                "Value: %s",
                decode_numeric_bitfield(value, 0xffff, 16, "0x%04x"));

            proto_item_append_text(tlv_item, " - 0x%04x", value);

            if (value != 0)
            {
                flags_tree = proto_item_add_subtree(
                    item, ett_wimaxasncp_tlv_value_bitflags16);

                for (i = 0; i < 16; ++i)
                {
                    guint16 mask;
                    mask = 1 << (15 - i);

                    if (value & mask)
                    {
                        s = wimaxasncp_get_enum_name(tlv_info, value & mask);

                        proto_tree_add_uint_format(
                            flags_tree, hf_wimaxasncp_tlv_value_bitflags16,
                            tvb, offset, length, value,
                            "Bit #%u is set: %s", i, s);
                    }
                }
            }
        }

        return;
    }
    case WIMAXASNCP_TLV_BITFLAGS32:
    {
        if (length != 4)
        {
            /* encoding error */
            break;
        }

        if (tlv_info->enums == NULL)
        {
            /* enum values missing */
        }

        if (tree)
        {
            proto_tree *flags_tree;
            proto_item *item;
            guint32 value;
            guint i;

            value = tvb_get_ntohl(tvb, offset);

            item = proto_tree_add_uint_format(
                tree, tlv_info->hf_value,
                tvb, offset, length, value,
                "Value: %s",
                decode_numeric_bitfield(value, 0xffffffff, 32, "0x%08x"));

            proto_item_append_text(tlv_item, " - 0x%08x", value);

            if (value != 0)
            {
                flags_tree = proto_item_add_subtree(
                    item, ett_wimaxasncp_tlv_value_bitflags32);

                for (i = 0; i < 32; ++i)
                {
                    guint32 mask;
                    mask = 1 << (31 - i);

                    if (value & mask)
                    {
                        s = wimaxasncp_get_enum_name(tlv_info, value & mask);

                        proto_tree_add_uint_format(
                            flags_tree, hf_wimaxasncp_tlv_value_bitflags32,
                            tvb, offset, length, value,
                            "Bit #%u is set: %s", i, s);
                    }
                }
            }
        }

        return;
    }
    case WIMAXASNCP_TLV_ID:
    {
        if (length == 4)
        {
            if (tree)
            {
                wimaxasncp_proto_tree_add_tlv_ipv4_value(
                    tvb, tree, tlv_item, offset, tlv_info);
            }

            return;
        }
        else if (length == 6)
        {
            if (tree)
            {
                wimaxasncp_proto_tree_add_ether_value(
                    tvb, tree, tlv_item, offset, length, tlv_info);
            }

            return;
        }
        else if (length == 16)
        {
            if (tree)
            {
                wimaxasncp_proto_tree_add_tlv_ipv6_value(
                    tvb, tree, tlv_item, offset, tlv_info);
            }

            return;
        }
        else
        {
            /* encoding error */
            break;
        }
    }
    case WIMAXASNCP_TLV_BYTES:
    {
        if (tree)
        {
            const gchar *format1;
            const gchar *format2;
            const guint8 *p = tvb_get_ptr(tvb, offset, length);
            const gchar *s = 
                bytestring_to_str(p, MIN(length, max_show_bytes), 0);

            if (length <= max_show_bytes)
            {
                format1 = "Value: %s";
                format2 = " - %s";
            }
            else
            {
                format1 = "Value: %s...";
                format2 = " - %s...";
            }

            proto_tree_add_bytes_format(
                tree, tlv_info->hf_value,
                tvb, offset, length, p,
                format1, s);

            proto_item_append_text(
                tlv_item, format2, s);
        }

        return;
    }
    case WIMAXASNCP_TLV_HEX8:
    {
        if (length != 1)
        {
            /* encoding error */
            break;
        }

        if (tree)
        {
            guint8 value;

            value = tvb_get_guint8(tvb, offset);

            proto_tree_add_uint_format(
                tree, tlv_info->hf_value,
                tvb, offset, length, value,
                "Value: 0x%02x", value);

            proto_item_append_text(tlv_item, " - 0x%02x", value);
        }

        return;
    }
    case WIMAXASNCP_TLV_HEX16:
    {
        if (length != 2)
        {
            /* encoding error */
            break;
        }

        if (tree)
        {
            guint16 value;

            value = tvb_get_ntohs(tvb, offset);

            proto_tree_add_uint_format(
                tree, tlv_info->hf_value,
                tvb, offset, length, value,
                "Value: 0x%04x", value);

            proto_item_append_text(tlv_item, " - 0x%04x", value);
        }

        return;
    }
    case WIMAXASNCP_TLV_HEX32:
    {
        if (length != 4)
        {
            /* encoding error */
            break;
        }

        if (tree)
        {
            guint32 value;

            value = tvb_get_ntohl(tvb, offset);

            proto_tree_add_uint_format(
                tree, tlv_info->hf_value,
                tvb, offset, length, value,
                "Value: 0x%08x", value);

            proto_item_append_text(tlv_item, " - 0x%08x", value);
        }

        return;
    }
    case WIMAXASNCP_TLV_DEC8:
    {
        if (length != 1)
        {
            /* encoding error */
            break;
        }

        if (tree)
        {
            guint8 value;

            value = tvb_get_guint8(tvb, offset);

            proto_tree_add_uint_format(
                tree, tlv_info->hf_value,
                tvb, offset, length, value,
                "Value: %d", value);

            proto_item_append_text(tlv_item, " - %u", value);
        }

        return;
    }
    case WIMAXASNCP_TLV_DEC16:
    {
        if (length != 2)
        {
            /* encoding error */
            break;
        }

        if (tree)
        {
            guint16 value;

            value = tvb_get_ntohs(tvb, offset);

            proto_tree_add_uint_format(
                tree, tlv_info->hf_value,
                tvb, offset, length, value,
                "Value: %d", value);

            proto_item_append_text(tlv_item, " - %u", value);
        }

        return;
    }
    case WIMAXASNCP_TLV_DEC32:
    {
        if (length != 4)
        {
            /* encoding error */
            break;
        }

        if (tree)
        {
            guint32 value;

            value = tvb_get_ntohl(tvb, offset);

            proto_tree_add_uint_format(
                tree, tlv_info->hf_value,
                tvb, offset, length, value,
                "Value: %d", value);

            proto_item_append_text(tlv_item, " - %u", value);
        }

        return;
    }
    case WIMAXASNCP_TLV_TBD:
    {
        if (debug_enabled)
        {
            g_print(
                "fix-me: TBD: TLV %s (%d)\n", tlv_info->name, tlv_info->type);
        }

        if (tree)
        {
            const gchar *format;
            const guint8 *p = tvb_get_ptr(tvb, offset, length);
            const gchar *s = 
                bytestring_to_str(p, MIN(length, max_show_bytes), 0);

            if (length <= max_show_bytes)
            {
                format = "Value: %s %s";
            }
            else
            {
                format = "Value: %s %s...";
            }
            
            proto_tree_add_bytes_format(
                tree, tlv_info->hf_value,
                tvb, offset, length, p,
                format, hex_note, s);

            proto_item_append_text(tlv_item, " - TBD");
        }

        return;
    }
    case WIMAXASNCP_TLV_IP_ADDRESS:
    {
        if (length == 4)
        {
            if (tree)
            {
                wimaxasncp_proto_tree_add_tlv_ipv4_value(
                    tvb, tree, tlv_item, offset, tlv_info);
            }

            return;
        }
        else if (length == 16)
        {
            if (tree)
            {
                wimaxasncp_proto_tree_add_tlv_ipv6_value(
                    tvb, tree, tlv_item, offset, tlv_info);
            }

            return;
        }
        else
        {
            /* encoding error */
            break;
        }
    }
    case WIMAXASNCP_TLV_IPV4_ADDRESS:
    {
        if (length != 4)
        {
            /* encoding error */
            break;
        }

        if (tree)
        {
            wimaxasncp_proto_tree_add_tlv_ipv4_value(
                tvb, tree, tlv_item, offset, tlv_info);
        }

        return;
    }
    case WIMAXASNCP_TLV_PROTOCOL_LIST:
    {
        if (length % 2 != 0)
        {
            /* encoding error */
            break;
        }

        if (tree && length > 0)
        {
            proto_tree *protocol_list_tree;
            proto_item *item;
            const guint max_protocols_in_tlv_item = 8; /* arbitrary */

            item = proto_tree_add_text(
                tree, tvb, offset, length,
                "Value");

            protocol_list_tree = proto_item_add_subtree(
                item, ett_wimaxasncp_tlv_protocol_list);

            /* hidden item for filtering */
            item = proto_tree_add_item(
                protocol_list_tree, tlv_info->hf_value,
                tvb, offset, length, FALSE);

            PROTO_ITEM_SET_HIDDEN(item);

            while (offset < tvb_length(tvb))
            {
                guint16 protocol;
                const gchar *protocol_name;

                protocol = tvb_get_ntohs(tvb, offset);
                protocol_name = ipprotostr(protocol);

                proto_tree_add_uint_format(
                    protocol_list_tree, tlv_info->hf_protocol,
                    tvb, offset, 2, protocol,
                    "Protocol: %s (%u)", protocol_name, protocol);

                if (offset == 0)
                {
                    proto_item_append_text(tlv_item, " - %s", protocol_name);
                }
                else if (offset < 2 * max_protocols_in_tlv_item)
                {
                    proto_item_append_text(tlv_item, ", %s", protocol_name);
                }
                else if (offset == 2 * max_protocols_in_tlv_item)
                {
                    proto_item_append_text(tlv_item, ", ...");
                }

                offset += 2;
            }
        }

        return;
    }
    case WIMAXASNCP_TLV_PORT_RANGE_LIST:
    {
        if (length % 4 != 0)
        {
            /* encoding error */
            break;
        }

        if (tree && length > 0)
        {
            proto_tree *port_range_list_tree;
            proto_item *item;
            const guint max_port_ranges_in_tlv_item = 3; /* arbitrary */

            item = proto_tree_add_text(
                tree, tvb, offset, length,
                "Value");

            port_range_list_tree = proto_item_add_subtree(
                item, ett_wimaxasncp_tlv_port_range_list);

            /* hidden item for filtering */
            item = proto_tree_add_item(
                port_range_list_tree, tlv_info->hf_value,
                tvb, offset, length, FALSE);

            PROTO_ITEM_SET_HIDDEN(item);

            while (offset < tvb_length(tvb))
            {
                guint16 portLow;
                guint16 portHigh;

                portLow = tvb_get_ntohs(tvb, offset);
                portHigh = tvb_get_ntohs(tvb, offset + 2);

                proto_tree_add_text(
                    port_range_list_tree, tvb, offset, 4,
                    "Port Range: %d-%d", portLow, portHigh);

                /* hidden items are for filtering */

                item = proto_tree_add_item(
                    port_range_list_tree, tlv_info->hf_port_low,
                    tvb, offset, 2, FALSE);

                PROTO_ITEM_SET_HIDDEN(item);

                item = proto_tree_add_item(
                    port_range_list_tree, tlv_info->hf_port_high,
                    tvb, offset + 2, 2, FALSE);

                PROTO_ITEM_SET_HIDDEN(item);

                if (offset == 0)
                {
                    proto_item_append_text(
                        tlv_item, " - %d-%d", portLow, portHigh);
                }
                else if (offset < 4 * max_port_ranges_in_tlv_item)
                {
                    proto_item_append_text(
                        tlv_item, ", %d-%d", portLow, portHigh);
                }
                else if (offset == 4 * max_port_ranges_in_tlv_item)
                {
                    proto_item_append_text(tlv_item, ", ...");
                }

                offset += 4;
            }
        }

        return;
    }
    case WIMAXASNCP_TLV_IP_ADDRESS_MASK_LIST:
    {
        /* --------------------------------------------------------------------
         * The definion of these TLVs are ambiguous. The length in octets is
         * described as Nx8 (IPv4) or Nx32 (IPv6), but this function cannot
         * always differentiate between IPv4 and IPv6. For example, if length
         * = 32, then is it IPv4 where N=4 (4x8) or IPv6 where N=1 (1x32)?
         *
         * For now, we presume lengths that *can* indicate an IPv6 address and
         * mask list *do* denote an IPv6 address and mask list.
         * --------------------------------------------------------------------
         */

        if (length % 8 != 0)
        {
            /* encoding error */
            break;
        }

        if (tree && length > 0)
        {
            proto_tree *ip_address_mask_list_tree;
            proto_item *item;

            item = proto_tree_add_text(
                tree, tvb, offset, length,
                "Value");

            ip_address_mask_list_tree = proto_item_add_subtree(
                item, ett_wimaxasncp_tlv_ip_address_mask_list);

            /* hidden item for filtering */
            item = proto_tree_add_item(
                ip_address_mask_list_tree, tlv_info->hf_value,
                tvb, offset, length, FALSE);

            PROTO_ITEM_SET_HIDDEN(item);

            if (length % 32 == 0)
            {
                /* ------------------------------------------------------------
                 * presume IPv6
                 * ------------------------------------------------------------
                 */

                while (offset < tvb_length(tvb))
                {
                    proto_tree *ip_address_mask_tree;
                    struct e_in6_addr ip;
                    const gchar *s;

                    item = proto_tree_add_text(
                        ip_address_mask_list_tree, tvb, offset, 32,
                        "IPv6 Address and Mask");

                    ip_address_mask_tree = proto_item_add_subtree(
                        item, ett_wimaxasncp_tlv_ip_address_mask);

                    /* --------------------------------------------------------
                     * address
                     * --------------------------------------------------------
                     */

                    tvb_get_ipv6(tvb, offset, &ip);

                    proto_tree_add_item(
                        ip_address_mask_tree,
                        tlv_info->hf_ipv6,
                        tvb, offset, 16, FALSE);

                    /* too long to display ?
                    proto_item_append_text(
                        item, " - %s (%s)",
                        get_hostname6(&ip), ip6_to_str(&ip));
                    */

                    offset += 16;

                    /* --------------------------------------------------------
                     * mask
                     * --------------------------------------------------------
                     */

                    tvb_get_ipv6(tvb, offset, &ip);

                    s = ip6_to_str(&ip);

                    proto_tree_add_ipv6_format_value(
                        ip_address_mask_tree,
                        tlv_info->hf_ipv6_mask,
                        tvb, offset, 16, (const guint8*)&ip,
                        "%s", s);

                    /* too long to display ?
                    proto_item_append_text(
                        item, " / %s", s);
                    */

                    offset += 16;
                }
            }
            else
            {
                /* ------------------------------------------------------------
                 * IPv4
                 * ------------------------------------------------------------
                 */

                while (offset < tvb_length(tvb))
                {
                    proto_tree *ip_address_mask_tree;
                    guint32 ip;
                    const gchar *s;

                    item = proto_tree_add_text(
                        ip_address_mask_list_tree, tvb, offset, 8,
                        "IPv4 Address and Mask");

                    ip_address_mask_tree = proto_item_add_subtree(
                        item, ett_wimaxasncp_tlv_ip_address_mask);

                    /* --------------------------------------------------------
                     * address
                     * --------------------------------------------------------
                     */

                    ip = tvb_get_ipv4(tvb, offset);

                    proto_tree_add_item(
                        ip_address_mask_tree,
                        tlv_info->hf_ipv4,
                        tvb, offset, 4, FALSE);

                    proto_item_append_text(
                        item, " - %s (%s)",
                        get_hostname(ip), ip_to_str((guint8 *)&ip));

                    offset += 4;

                    /* --------------------------------------------------------
                     * mask
                     * --------------------------------------------------------
                     */

                    ip = tvb_get_ipv4(tvb, offset);

                    s = ip_to_str((guint8 *)&ip);

                    proto_tree_add_ipv4_format_value(
                        ip_address_mask_tree,
                        tlv_info->hf_ipv4_mask,
                        tvb, offset, 4, ip,
                        "%s", s);

                    proto_item_append_text(
                        item, " / %s", s);

                    offset += 4;
                }
            }
        }

        return;
    }
    case WIMAXASNCP_TLV_EAP:
    {
        /*
         *   EAP payload, call eap dissector to dissect eap payload
         */
        guint8 eap_code;
        guint8 eap_type = 0;

        /* Get code */
        eap_code = tvb_get_guint8(tvb, offset);
        if (eap_code == EAP_REQUEST || eap_code == EAP_RESPONSE)
        {
            /* Get type */
            eap_type = tvb_get_guint8(tvb, offset + 4);
        }

        /* Add code and type to info column */
        if (check_col(pinfo->cinfo, COL_INFO))
        {
            col_append_fstr(pinfo->cinfo, COL_INFO, " [");
            col_append_fstr(pinfo->cinfo, COL_INFO,
                            val_to_str(eap_code, eap_code_vals, "Unknown code (0x%02X)"));

            if (eap_code == EAP_REQUEST || eap_code == EAP_RESPONSE)
            {
                col_append_fstr(pinfo->cinfo, COL_INFO, ", ");
                col_append_fstr(pinfo->cinfo, COL_INFO,
                                val_to_str(eap_type, eap_type_vals, "Unknown type (0x%02X)"));
            }

            col_append_fstr(pinfo->cinfo, COL_INFO, "]");
        }


        if (tree)
        {
            proto_tree *eap_tree;
            proto_item *item;
            gboolean save_writable;
            tvbuff_t *eap_tvb;

            /* Create EAP subtree */
            item = proto_tree_add_item(tree, tlv_info->hf_value, tvb,
                                       offset, length, FALSE);
            proto_item_set_text(item, "Value");
            eap_tree = proto_item_add_subtree(item, ett_wimaxasncp_tlv_eap);

            /* Also show high-level details in this root item */
            proto_item_append_text(item, " (%s",
                                   val_to_str(eap_code, eap_code_vals,
                                              "Unknown code (0x%02X)"));
            if (eap_code == EAP_REQUEST || eap_code == EAP_RESPONSE)
            {
                proto_item_append_text(item, ", %s",
                                       val_to_str(eap_type, eap_type_vals,
                                       "Unknown type (0x%02X)"));
            }
            proto_item_append_text(item, ")");


            /* Extract remaining bytes into new tvb */
            eap_tvb = tvb_new_subset(tvb, offset, length,
                                     tvb_length_remaining(tvb, offset));

            /* Disable writing to info column while calling eap dissector */
            save_writable = col_get_writable(pinfo->cinfo);
            col_set_writable(pinfo->cinfo, FALSE);

            /* Call the EAP dissector. */
            call_dissector(eap_handle, eap_tvb, pinfo, eap_tree);

            /* Restore previous writable state of info column */
            col_set_writable(pinfo->cinfo, save_writable);
        }

        return;
    }

    case WIMAXASNCP_TLV_VENDOR_SPECIFIC:
    {
        /* --------------------------------------------------------------------
         *  The format of the vendor specific information field (VSIF) is not
         *  clearly defined.  It appears to be compound as the spec states
         *  that the vendor ID field shall be the first TLV embedded inside
         *  the VSIF.  However, the vendor ID is shown as a 24-bit value. Does
         *  this mean the field is 24-bits?  If so, how is alignment/padding
         *  handled?
         *
         * For now, we decode the vendor ID as a non-padded 24-bit value and
         * dump the rest as hex.
         * --------------------------------------------------------------------
         */

        if (length < 3)
        {
            /* encoding error */
            break;
        }

        if (tree)
        {
            proto_tree *vsif_tree;
            proto_item *item;
            guint32 vendorId;
            const gchar *vendorName;

            item = proto_tree_add_text(
                tree, tvb, offset, length,
                "Value");

            vsif_tree = proto_item_add_subtree(
                item, ett_wimaxasncp_tlv_vendor_specific_information_field);

            /* hidden item for filtering */
            item = proto_tree_add_item(
                vsif_tree, tlv_info->hf_value,
                tvb, offset, length, FALSE);

            PROTO_ITEM_SET_HIDDEN(item);

            /* ----------------------------------------------------------------
             * vendor ID (24-bit)
             * ----------------------------------------------------------------
             */

            vendorId = tvb_get_ntoh24(tvb, offset);

            vendorName = val_to_str(vendorId, sminmpec_values, "Unknown");
            proto_tree_add_uint_format(
                vsif_tree, tlv_info->hf_vendor_id,
                tvb, offset, 3, vendorId,
                "Vendor ID: %s (%u)", vendorName, vendorId);

            proto_item_append_text(tlv_item, " - %s", vendorName);

            offset += 3;

            /* ----------------------------------------------------------------
             * hex dump the rest
             * ----------------------------------------------------------------
             */

            if (offset < tvb_length(tvb))
            {
                proto_tree_add_item(
                    vsif_tree, tlv_info->hf_vendor_rest_of_info,
                    tvb, offset, length - offset, FALSE);
            }
        }

        return;
    }
    case WIMAXASNCP_TLV_UNKNOWN:
    {
        if (tree)
        {
            const gchar *format1;
            const gchar *format2;
            const guint8 *p = tvb_get_ptr(tvb, offset, length);
            const gchar *s = 
                bytestring_to_str(p, MIN(length, max_show_bytes), 0);

            if (length <= max_show_bytes)
            {
                format1 = "Value: %s %s";
                format2 = " - %s %s";
            }
            else
            {
                format1 = "Value: %s %s...";
                format2 = " - %s %s...";
            }

            proto_tree_add_bytes_format(
                tree, tlv_info->hf_value,
                tvb, offset, length, p,
                format1, hex_note, s);

            proto_item_append_text(
                tlv_item, format2, hex_note, s);

        }

        return;
    }
    default:
        if (debug_enabled)
        {
            g_print(
                "fix-me: unknown decoder: %d\n", tlv_info->decoder);
        }
        break;
    }

    /* default is hex dump */

    if (tree)
    {
        const gchar *format;
        const guint8 *p = tvb_get_ptr(tvb, offset, length);
        const gchar *s = bytestring_to_str(p, MIN(length, max_show_bytes), 0);

        if (length <= max_show_bytes)
        {
            format = "Value: %s %s";
        }
        else
        {
            format = "Value: %s %s...";
        }
        
        proto_tree_add_bytes_format(
            tree, hf_wimaxasncp_tlv_value_bytes,
            tvb, offset, length, p,
            format, hex_note, s);
    }
}

/* ========================================================================= */

static guint dissect_wimaxasncp_tlvs(
    tvbuff_t *tvb,
    packet_info *pinfo,
    proto_tree *tree)
{
    guint offset;

    offset = 0;
    while (offset < tvb_reported_length(tvb))
    {
        proto_tree *tlv_tree = NULL;
        proto_item *tlv_item = NULL;
        const wimaxasncp_dict_tlv_t *tlv_info;

        guint16 type;
        guint16 length;
        guint pad;

        /* --------------------------------------------------------------------
         * type and length
         * --------------------------------------------------------------------
         */

        type = tvb_get_ntohs(tvb, offset);
        tlv_info = wimaxasncp_get_tlv_info(type);

        length = tvb_get_ntohs(tvb, offset + 2);
        pad = 4 - (length % 4);
        if (pad == 4)
        {
            pad = 0;
        }

        if (tree)
        {
            proto_item *type_item;

            gint tree_length = MIN(
                (gint)(4 + length + pad), tvb_length_remaining(tvb, offset));

            tlv_item = proto_tree_add_item(
                tree, tlv_info->hf_root,
                tvb, offset, tree_length, FALSE);

            /* Set label for tlv item */
            proto_item_set_text(tlv_item, "TLV: %s", tlv_info->name);

            /* Show code number if unknown */
            if (tlv_info->decoder == WIMAXASNCP_TLV_UNKNOWN)
            {
                proto_item_append_text(tlv_item, " (%u)", type);
            }

            /* Indicate if a compound tlv */
            if (tlv_info->decoder == WIMAXASNCP_TLV_COMPOUND)
            {
                proto_item_append_text(tlv_item, " [Compound]");
            }

            /* Create TLV subtree */
            tlv_tree = proto_item_add_subtree(
                tlv_item, ett_wimaxasncp_tlv);

            /* Type (expert item if unknown) */
            type_item = proto_tree_add_uint_format(
                tlv_tree, hf_wimaxasncp_tlv_type,
                tvb, offset, 2, type,
                "Type: %s (%u)", tlv_info->name, type);

            if (tlv_info->decoder == WIMAXASNCP_TLV_UNKNOWN)
            {
                expert_add_info_format(pinfo, type_item,
                                       PI_UNDECODED, PI_WARN,
                                       "Unknown TLV type (%u)",
                                       type);
            }

            /* Length */
            proto_tree_add_uint(
                tlv_tree, hf_wimaxasncp_tlv_length,
                tvb, offset + 2, 2, length);

        }

        offset += 4;

        /* --------------------------------------------------------------------
         * value
         * --------------------------------------------------------------------
         */

        if (tlv_info->decoder == WIMAXASNCP_TLV_COMPOUND)
        {
            if (length == 0)
            {
                /* error? compound, but no TLVs inside */
            }
            else if (tvb_length_remaining(tvb, offset) > 0)
            {
                tvbuff_t *tlv_tvb;

                /* N.B.  Not padding out tvb length */
                tlv_tvb = tvb_new_subset(
                    tvb, offset,
                    MIN(length, tvb_length_remaining(tvb, offset)),
                    length);

                /* N.B.  This is a recursive call... */
                dissect_wimaxasncp_tlvs(tlv_tvb, pinfo, tlv_tree);
            }
            else
            {
                /* this should throw */
                tvb_ensure_bytes_exist(tvb, offset, length + pad);
            }
        }
        else
        {
            tvbuff_t *tlv_tvb;

            tvb_ensure_bytes_exist(tvb, offset, length + pad);

            tlv_tvb = tvb_new_subset(
                tvb, offset,
                MIN(length, tvb_length_remaining(tvb, offset)),
                length);

            wimaxasncp_dissect_tlv_value(
                tlv_tvb, pinfo, tlv_tree, tlv_item, tlv_info);
        }

        offset += length + pad;
    }

    return offset;
}

/* ========================================================================= */

static guint dissect_wimaxasncp_backend(
    tvbuff_t *tvb,
    packet_info *pinfo,
    proto_tree *tree)
{
    guint offset = 0;
    guint16 ui16;
    guint32 ui32;
    const guint8 *p;
    guint8 *pmsid = NULL;
    guint16 tid = 0;
    gboolean dbit_show;


    /* ------------------------------------------------------------------------
     * MSID
     * ------------------------------------------------------------------------
     */

    p = tvb_get_ptr(tvb, offset, 6);

    if (tree)
    {
        proto_tree_add_ether(
            tree, hf_wimaxasncp_msid,
            tvb, offset, 6, p);

        pmsid = ether_to_str(p);
    }

    offset += 6;

    /* ------------------------------------------------------------------------
     * reserved
     * ------------------------------------------------------------------------
     */

    ui32 = tvb_get_ntohl(tvb, offset);

    if (tree)
    {
        proto_tree_add_uint(
            tree, hf_wimaxasncp_reserved1,
            tvb, offset, 4, ui32);
    }

    offset += 4;

    /* ------------------------------------------------------------------------
     * transaction ID
     * ------------------------------------------------------------------------
     */

    dbit_show = FALSE;
    ui16 = tvb_get_ntohs(tvb, offset);

    if (tree)
    {
        if (show_transaction_id_d_bit)
        {
            const guint16 mask = 0x7fff;

            if (ui16 & 0x8000)
            {
                proto_tree_add_uint_format(
                    tree, hf_wimaxasncp_transaction_id,
                    tvb, offset, 2, ui16,
                    "Transaction ID: D + 0x%04x (0x%04x)", mask & ui16, ui16);

                tid = ui16 & mask;
                dbit_show = TRUE;
            }
            else
            {
                proto_tree_add_uint_format(
                    tree, hf_wimaxasncp_transaction_id,
                    tvb, offset, 2, ui16,
                    "Transaction ID: 0x%04x", ui16);

                tid = ui16;
            }
        }
        else
        {
            proto_tree_add_uint(
                tree, hf_wimaxasncp_transaction_id,
                tvb, offset, 2, ui16);

            tid = ui16;
        }
    }

    offset += 2;

    /* ------------------------------------------------------------------------
     * reserved
     * ------------------------------------------------------------------------
     */

    ui16 = tvb_get_ntohs(tvb, offset);

    if (tree)
    {
        proto_tree_add_uint(
            tree, hf_wimaxasncp_reserved2,
            tvb, offset, 2, ui16);
    }

    offset += 2;

    /* ------------------------------------------------------------------------
     * TLVs
     * ------------------------------------------------------------------------
     */

    if (offset < tvb_length(tvb))
    {
        tvbuff_t *tlv_tvb;

        tlv_tvb = tvb_new_subset(
            tvb, offset,
            tvb_length(tvb) - offset,
            tvb_length(tvb) - offset);

        offset += dissect_wimaxasncp_tlvs(tlv_tvb, pinfo, tree);
    }

    if (check_col(pinfo->cinfo, COL_INFO))
    {
        col_append_fstr(pinfo->cinfo, COL_INFO, " - MSID:%s", pmsid);
        if (dbit_show)
        {
            col_append_fstr(pinfo->cinfo, COL_INFO, ", TID:D+0x%04x", tid);
        }
        else
        {
            col_append_fstr(pinfo->cinfo, COL_INFO, ", TID:0x%04x", tid);
        }
    }

    return offset;
}

/* ========================================================================= */

static int
dissect_wimaxasncp(
    tvbuff_t *tvb,
    packet_info *pinfo,
    proto_tree *tree)
{
    const gchar *unknown = "Unknown";

    /* Set up structures needed to add the protocol subtree and manage it */
    proto_item *packet_item = NULL;
    proto_item *item = NULL;
    proto_tree *wimaxasncp_tree = NULL;
    tvbuff_t *subtree;

    guint offset;
    guint8 ui8;

    guint8 function_type;
    guint16 length;

    /* ------------------------------------------------------------------------
     * First, we do some heuristics to check if the packet cannot be our
     * protocol.
     * ------------------------------------------------------------------------
     */

    /* Should we check a minimum size?  If so, uncomment out the following
     * code. */
    /*
    if (tvb_reported_length(tvb) < WIMAXASNCP_HEADER_SIZE)
    {
        return 0;
    }
    */

    /* We currently only support version 1. */
    if (tvb_bytes_exist(tvb, 0, 1) && tvb_get_guint8(tvb, 0) != 1)
    {
        return 0;
    }

    /* ------------------------------------------------------------------------
     * Initialize the protocol and info column.
     * ------------------------------------------------------------------------
     */

    /* Make entries in Protocol column and Info column on summary display */
    if (check_col(pinfo->cinfo, COL_PROTOCOL))
    {
        col_set_str(pinfo->cinfo, COL_PROTOCOL, "WiMAX");
    }

    /* We'll fill in the "Info" column after fetch data, so we clear the
       column first in case calls to fetch data from the packet throw an
       exception. */
    if (check_col(pinfo->cinfo, COL_INFO))
    {
        col_clear(pinfo->cinfo, COL_INFO);
    }

    /* ========================================================================
     * Disesction starts here
     * ========================================================================
     */

    /* ------------------------------------------------------------------------
     * total packet, we'll adjust after we read the length field
     * ------------------------------------------------------------------------
     */

    offset = 0;

    if (tree)
    {
        packet_item = proto_tree_add_item(
            tree, proto_wimaxasncp,
            tvb, 0, MIN(WIMAXASNCP_HEADER_LENGTH_END, tvb_length(tvb)), FALSE);

        wimaxasncp_tree = proto_item_add_subtree(
            packet_item, ett_wimaxasncp);
    }

    /* ------------------------------------------------------------------------
     * version
     * ------------------------------------------------------------------------
     */

    ui8 = tvb_get_guint8(tvb, offset);

    if (tree)
    {
        proto_tree_add_uint(
            wimaxasncp_tree, hf_wimaxasncp_version,
            tvb, offset, 1, ui8);
    }

    offset += 1;

    /* ------------------------------------------------------------------------
     * flags
     * ------------------------------------------------------------------------
     */

    ui8 = tvb_get_guint8(tvb, offset);

    if (tree)
    {
        proto_tree *flags_tree;
        guint i;

        if (ui8 == 0)
        {
            item = proto_tree_add_uint_format(
                wimaxasncp_tree, hf_wimaxasncp_flags,
                tvb, offset, 1, ui8,
                "Flags: 0x%02x", ui8);
        }
        else
        {
            item = proto_tree_add_uint_format(
                wimaxasncp_tree, hf_wimaxasncp_flags,
                tvb, offset, 1, ui8,
                "Flags: ");

            if (ui8 & (WIMAXASNCP_FLAGS_T | WIMAXASNCP_FLAGS_R))
            {
                if (ui8 & WIMAXASNCP_FLAGS_T)
                {
                    proto_item_append_text(item, "T");
                }

                if (ui8 & WIMAXASNCP_FLAGS_R)
                {
                    proto_item_append_text(item, "R");
                }

                proto_item_append_text(item, " - ");
            }

            proto_item_append_text(
                item, "%s", decode_numeric_bitfield(ui8, 0xff, 8, "0x%02x"));

            flags_tree = proto_item_add_subtree(
                item, ett_wimaxasncp_flags);

            for (i = 0; i < 8; ++i)
            {
                guint8 mask;
                mask = 1 << (7 - i);

                if (ui8 & mask)
                {
                    proto_tree_add_uint_format(
                        flags_tree, hf_wimaxasncp_flags,
                        tvb, offset, 1, ui8,
                        "Bit #%u is set: %s",
                        i,
                        val_to_str(
                            ui8 & mask, wimaxasncp_flag_vals, "Unknown"));
                }
            }
        }
    }

    offset += 1;

    /* ------------------------------------------------------------------------
     * function type
     * ------------------------------------------------------------------------
     */

    function_type = tvb_get_guint8(tvb, offset);

    if (tree)
    {
        proto_item *function_type_item;
        function_type_item = proto_tree_add_uint(
            wimaxasncp_tree, hf_wimaxasncp_function_type,
            tvb, offset, 1, function_type);

        /* Add expert item if not matched */
        if (strcmp(val_to_str(function_type,
                              wimaxasncp_function_type_vals,
                              unknown),
                   unknown) == 0)
        {
                expert_add_info_format(pinfo, function_type_item,
                                       PI_UNDECODED, PI_WARN,
                                       "Unknown function type (%u)",
                                       function_type);
        }
    }

    offset += 1;

    /* ------------------------------------------------------------------------
     * OP ID and message type
     * ------------------------------------------------------------------------
     */

    ui8 = tvb_get_guint8(tvb, offset);

    if (tree)
    {
        const gchar *message_name;
        const wimaxasncp_func_msg_t *p = NULL;
        gsize i;

        /* --------------------------------------------------------------------
         * OP ID
         * --------------------------------------------------------------------
         */

        item = proto_tree_add_uint_format(
            wimaxasncp_tree, hf_wimaxasncp_op_id,
             tvb, offset, 1, ui8,
            "OP ID: %s", val_to_str(ui8 >> 5, wimaxasncp_op_id_vals, unknown));

        proto_item_append_text(
            item, " (%s)", decode_numeric_bitfield(ui8, 0xe0, 8, "%u"));


        /* use the function type to find the message vals */
        for (i = 0; i < array_length(wimaxasncp_func_to_msg_vals_map); ++i)
        {
            p = &wimaxasncp_func_to_msg_vals_map[i];

            if (function_type == p->function_type)
            {
                break;
            }
        }

        /* --------------------------------------------------------------------
         * message type
         * --------------------------------------------------------------------
         */

        message_name = p ? val_to_str(0x1f & ui8, p->vals, unknown) : unknown;

        item = proto_tree_add_uint_format(
            wimaxasncp_tree, hf_wimaxasncp_op_id,
            tvb, offset, 1, ui8,
            "Message Type: %s", message_name);

        proto_item_append_text(
            item, " (%s)", decode_numeric_bitfield(ui8, 0x1f, 8, "%u"));

        /* Add expert item if not matched */
        if (strcmp(message_name, unknown) == 0)
        {
                expert_add_info_format(pinfo, item,
                                       PI_UNDECODED, PI_WARN,
                                       "Unknown message op (%u)",
                                       0x1f & ui8);
        }

        if (check_col(pinfo->cinfo, COL_INFO))
        {
            col_add_str(pinfo->cinfo, COL_INFO, message_name);
        }
    }

    offset += 1;

    /* ------------------------------------------------------------------------
     * length
     * ------------------------------------------------------------------------
     */

    length = tvb_get_ntohs(tvb, offset);

    if (tree)
    {
        proto_item_set_len(
            packet_item, MAX(WIMAXASNCP_HEADER_LENGTH_END, length));

        item = proto_tree_add_uint(
            wimaxasncp_tree, hf_wimaxasncp_length,
            tvb, offset, 2, length);
    }

    offset += 2;

    if (length < WIMAXASNCP_HEADER_SIZE)
    {
        expert_add_info_format(
            pinfo, item, PI_MALFORMED, PI_ERROR, "Bad length");

        if (tree)
        {
            proto_item_append_text(
                item, " [error: specified length less than header size (20)]");
        }

        if (length <= WIMAXASNCP_HEADER_LENGTH_END)
        {
            return offset;
        }
    }

    /* ------------------------------------------------------------------------
     * remaining header fields and TLVs
     * ------------------------------------------------------------------------
     */

    subtree = tvb_new_subset(
        tvb, offset,
        MIN(length, tvb_length(tvb) - offset),
        length - WIMAXASNCP_HEADER_LENGTH_END);

    offset += dissect_wimaxasncp_backend(
        subtree, pinfo, wimaxasncp_tree);

    /* ------------------------------------------------------------------------
     * done, return the amount of data this dissector was able to dissect
     * ------------------------------------------------------------------------
     */

    return offset;
}

/* ========================================================================= */

static char *alnumerize(
    char *name)
{
    char *r = name;
    char *w = name;
    char c;
    
    for ( ; (c = *r); ++r) 
    {
        if (isalnum((unsigned char)c) || c == '_' || c == '.') 
        {
            *(w++) = c;
        }
        else if (c == ' ' || c == '-' || c == '/')
        {
            /* skip if at start of string */
            if (w == name)
            {
                continue;
            }

            /* skip if we would produce multiple adjacent '_'s */
            if (*(w - 1) == '_')
            {
                continue;
            }

            *(w++) = '_';
        }
    }
    
    *w = '\0';
    
    return name;
}

/* ========================================================================= */

static void add_reg_info(
    int *hf_ptr,
    const char *name,
    const char *abbrev,
    enum ftenum type,
    int  display,
    const char *blurb)
{
    hf_register_info hf = { 
        hf_ptr, { name, abbrev, type, display, NULL, 0x0, blurb, HFILL } };

    g_array_append_val(wimaxasncp_build_dict.hf, hf);
}

/* ========================================================================= */

static void add_tlv_reg_info(
    wimaxasncp_dict_tlv_t *tlv)
{
    const char *name;
    const char *abbrev;
    const char *blurb;

    /* ------------------------------------------------------------------------
     * add root reg info
     * ------------------------------------------------------------------------
     */

    name = g_strdup(tlv->name);
    abbrev = alnumerize(g_strdup_printf("wimaxasncp.tlv.%s", tlv->name));

    switch (tlv->decoder)
    {
    case WIMAXASNCP_TLV_UNKNOWN:
        blurb = "type=Unknown";
        break;
    case WIMAXASNCP_TLV_TBD:
        blurb = g_strdup_printf("type=%u, TBD", tlv->type);
        break;
    case WIMAXASNCP_TLV_COMPOUND:
        blurb = g_strdup_printf("type=%u, Compound", tlv->type);
        break;
    case WIMAXASNCP_TLV_FLAG0:
        blurb = g_strdup_printf("type=%u, Value = Null", tlv->type);
        break;
    default:
        blurb = g_strdup_printf("type=%u", tlv->type);
        break;
    }

    add_reg_info(
        &tlv->hf_root, name, abbrev, FT_BYTES, BASE_NONE, blurb);

    /* ------------------------------------------------------------------------
     * add value(s) reg info
     * ------------------------------------------------------------------------
     */

    name = g_strdup("Value");
    abbrev = alnumerize(g_strdup_printf("wimaxasncp.tlv.%s.value", tlv->name));
    blurb = g_strdup_printf("value for type=%u", tlv->type);

    switch (tlv->decoder)
    {
    case WIMAXASNCP_TLV_UNKNOWN:
        g_free((gpointer*)blurb);

        add_reg_info(
            &tlv->hf_value, name, abbrev, FT_BYTES, BASE_HEX, 
            "value for unknown type");
        break;

    case WIMAXASNCP_TLV_TBD:
        add_reg_info(
            &tlv->hf_value, name, abbrev, FT_BYTES, BASE_HEX, blurb);
        break;

    case WIMAXASNCP_TLV_COMPOUND:
    case WIMAXASNCP_TLV_FLAG0:
        g_free((gpointer*)name);
        g_free((gpointer*)abbrev);
        g_free((gpointer*)blurb);
        break;

    case WIMAXASNCP_TLV_BYTES:
        add_reg_info(
            &tlv->hf_value, name, abbrev, FT_BYTES, BASE_HEX, blurb);
        break;

    case WIMAXASNCP_TLV_ENUM8:
        add_reg_info(
            &tlv->hf_value, name, abbrev, FT_UINT8, BASE_DEC, blurb);
        break;

    case WIMAXASNCP_TLV_ENUM16:
        add_reg_info(
            &tlv->hf_value, name, abbrev, FT_UINT16, BASE_DEC, blurb);
        break;

    case WIMAXASNCP_TLV_ENUM32:
        add_reg_info(
            &tlv->hf_value, name, abbrev, FT_UINT32, BASE_DEC, blurb);
        break;

    case WIMAXASNCP_TLV_ETHER:
        add_reg_info(
            &tlv->hf_value, name, abbrev, FT_ETHER, BASE_NONE, blurb);
        break;

    case WIMAXASNCP_TLV_ASCII_STRING:
        add_reg_info(
            &tlv->hf_value, name, abbrev, FT_STRING, BASE_NONE, blurb);
        break;

    case WIMAXASNCP_TLV_BITFLAGS16:
        add_reg_info(
            &tlv->hf_value, name, abbrev, FT_UINT16, BASE_HEX, blurb);
        break;

    case WIMAXASNCP_TLV_BITFLAGS32:
        add_reg_info(
            &tlv->hf_value, name, abbrev, FT_UINT32, BASE_HEX, blurb);
        break;

    case WIMAXASNCP_TLV_ID:
        g_free((gpointer*)name);
        g_free((gpointer*)abbrev);

        name = "IPv4 Address";

        abbrev = alnumerize(
            g_strdup_printf("wimaxasncp.tlv.%s.ipv4_value", tlv->name));

        add_reg_info(
            &tlv->hf_ipv4, name, abbrev, FT_IPv4, BASE_NONE, blurb);

        name = "IPv6 Address";

        abbrev = alnumerize(
            g_strdup_printf("wimaxasncp.tlv.%s.ipv6_value", tlv->name));

        add_reg_info(
            &tlv->hf_ipv6, name, abbrev, FT_IPv6, BASE_NONE, blurb);

        name = "BS ID";

        abbrev = alnumerize(
            g_strdup_printf("wimaxasncp.tlv.%s.bsid_value", tlv->name));

        add_reg_info(
            &tlv->hf_bsid, name, abbrev, FT_ETHER, BASE_NONE, blurb);

        break;

    case WIMAXASNCP_TLV_HEX8:
        add_reg_info(
            &tlv->hf_value, name, abbrev, FT_UINT8, BASE_HEX, blurb);
        break;

    case WIMAXASNCP_TLV_HEX16:
        add_reg_info(
            &tlv->hf_value, name, abbrev, FT_UINT16, BASE_HEX, blurb);
        break;

    case WIMAXASNCP_TLV_HEX32:
        add_reg_info(
            &tlv->hf_value, name, abbrev, FT_UINT32, BASE_HEX, blurb);
        break;

    case WIMAXASNCP_TLV_DEC8:
        add_reg_info(
            &tlv->hf_value, name, abbrev, FT_UINT8, BASE_DEC, blurb);
        break;

    case WIMAXASNCP_TLV_DEC16:
        add_reg_info(
            &tlv->hf_value, name, abbrev, FT_UINT16, BASE_DEC, blurb);
        break;

    case WIMAXASNCP_TLV_DEC32:
        add_reg_info(
            &tlv->hf_value, name, abbrev, FT_UINT32, BASE_DEC, blurb);
        break;

    case WIMAXASNCP_TLV_IP_ADDRESS:
        g_free((gpointer*)name);
        g_free((gpointer*)abbrev);

        name = "IPv4 Address";

        abbrev = alnumerize(
            g_strdup_printf("wimaxasncp.tlv.%s.ipv4_value", tlv->name));

        add_reg_info(
            &tlv->hf_ipv4, name, abbrev, FT_IPv4, BASE_NONE, blurb);

        name = "IPv6 Address";

        abbrev = alnumerize(
            g_strdup_printf("wimaxasncp.tlv.%s.ipv6_value", tlv->name));

        add_reg_info(
            &tlv->hf_ipv6, name, abbrev, FT_IPv6, BASE_NONE, blurb);

        break;

    case WIMAXASNCP_TLV_IPV4_ADDRESS:
        add_reg_info(
            &tlv->hf_value, name, abbrev, FT_IPv4, BASE_NONE, blurb);
        break;

    case WIMAXASNCP_TLV_PROTOCOL_LIST:
        add_reg_info(
            &tlv->hf_value, name, abbrev, FT_BYTES, BASE_HEX, blurb);

        blurb = g_strdup_printf("value component for type=%u", tlv->type);

        name = "Protocol";
        
        abbrev = alnumerize(
            g_strdup_printf("wimaxasncp.tlv.%s.value.protocol", tlv->name));

        add_reg_info(
            &tlv->hf_protocol, name, abbrev, FT_UINT16, BASE_DEC, blurb);

        break;

    case WIMAXASNCP_TLV_PORT_RANGE_LIST:
        add_reg_info(
            &tlv->hf_value, name, abbrev, FT_BYTES, BASE_HEX, blurb);

        blurb = g_strdup_printf("value component for type=%u", tlv->type);

        name = "Port Low";

        abbrev = alnumerize(
            g_strdup_printf("wimaxasncp.tlv.%s.value.port_low", tlv->name));

        add_reg_info(
            &tlv->hf_port_low, name, abbrev, FT_UINT16, BASE_DEC, blurb);

        name = "Port High";

        abbrev = alnumerize(
            g_strdup_printf("wimaxasncp.tlv.%s.value.port_high", tlv->name));

        add_reg_info(
            &tlv->hf_port_high, name, abbrev, FT_UINT16, BASE_DEC, blurb);

        break;

    case WIMAXASNCP_TLV_IP_ADDRESS_MASK_LIST:
        add_reg_info(
            &tlv->hf_value, name, abbrev, FT_BYTES, BASE_HEX, blurb);

        blurb = g_strdup_printf("value component for type=%u", tlv->type);

        name = "IPv4 Address";

        abbrev = alnumerize(
            g_strdup_printf("wimaxasncp.tlv.%s.value.ipv4", tlv->name));

        add_reg_info(
            &tlv->hf_ipv4, name, abbrev, FT_IPv4, BASE_NONE, blurb);

        name = "IPv4 Mask";

        abbrev = alnumerize(
            g_strdup_printf("wimaxasncp.tlv.%s.value.ipv4_mask", tlv->name));

        add_reg_info(
            &tlv->hf_ipv4_mask, name, abbrev, FT_IPv4, BASE_NONE, blurb);

        name = "IPv6 Address";

        abbrev = alnumerize(
            g_strdup_printf("wimaxasncp.tlv.%s.value.ipv6", tlv->name));

        add_reg_info(
            &tlv->hf_ipv6, name, abbrev, FT_IPv6, BASE_NONE, blurb);

        name = "IPv6 Mask";

        abbrev = alnumerize(
            g_strdup_printf("wimaxasncp.tlv.%s.value.ipv6_mask", tlv->name));

        add_reg_info(
            &tlv->hf_ipv6_mask, name, abbrev, FT_IPv6, BASE_NONE, blurb);

        break;

    case WIMAXASNCP_TLV_VENDOR_SPECIFIC:
        add_reg_info(
            &tlv->hf_value, name, abbrev, FT_BYTES, BASE_HEX, blurb);

        blurb = g_strdup_printf("value component for type=%u", tlv->type);

        name = "Vendor ID";

        abbrev = alnumerize(
            g_strdup_printf("wimaxasncp.tlv.%s.value.vendor_id", tlv->name));

        add_reg_info(
            &tlv->hf_vendor_id, name, abbrev, FT_UINT24, BASE_DEC, blurb);

        name = "Rest of Info";

        abbrev = alnumerize(
            g_strdup_printf(
                "wimaxasncp.tlv.%s.value.vendor_rest_of_info", tlv->name));

        add_reg_info(
            &tlv->hf_vendor_rest_of_info, name, abbrev, FT_BYTES, BASE_HEX, 
            blurb);

        break;

    case WIMAXASNCP_TLV_EAP:
        blurb = g_strdup_printf("EAP payload embedded in %s", name);

        add_reg_info(
            &tlv->hf_value, name, abbrev, FT_BYTES, BASE_HEX, blurb);
        break;


    default:
        add_reg_info(
            &tlv->hf_value, name, abbrev, FT_BYTES, BASE_HEX, blurb);

        if (debug_enabled)
        {
            g_print(
                "fix-me: unknown decoder: %d\n", tlv->decoder);
        }

        break;
    }
}

/* ========================================================================= */
/* Register the protocol with Wireshark */

/* this format is require because a script is used to build the C function
   that calls all the protocol registration.
*/

void
proto_register_wimaxasncp(void)
{
    module_t *wimaxasncp_module;
    gboolean debug_parser;
    gboolean dump_dict;
    gchar *dir;
    gchar* dict_error;
 
    /* ------------------------------------------------------------------------
     * List of header fields
     * ------------------------------------------------------------------------
     */

    static hf_register_info hf_base[] = {
            {
                &hf_wimaxasncp_version,      /* ID */
                {
                    "Version",               /* FIELDNAME */
                    "wimaxasncp.version",    /* PROTOABBREV.FIELDABBRE */
                    FT_UINT8,                /* FIELDTYPE */
                    BASE_DEC,                /* FIELDBASE */
                    NULL,                    /* FIELDCONVERT */
                    0x0,                     /* BITMASK */
                    "",                      /* FIELDDESCR */
                    HFILL                    /* HFILL */
                }
            },
            {
                &hf_wimaxasncp_flags,
                {
                    "Flags",
                    "wimaxasncp.flags",
                    FT_UINT8,
                    BASE_HEX,
                    NULL,
                    0xff,
                    "",
                    HFILL
                }
            },
            {
                &hf_wimaxasncp_function_type,
                {
                    "Function Type",
                    "wimaxasncp.function_type",
                    FT_UINT8,
                    BASE_DEC,
                    VALS(wimaxasncp_function_type_vals),
                    0x0,
                    "",
                    HFILL
                }
            },
            {
                &hf_wimaxasncp_op_id,
                {
                    "OP ID",
                    "wimaxasncp.opid",
                    FT_UINT8,
                    BASE_HEX,
                    VALS(wimaxasncp_op_id_vals),
                    0xE0,
                    "",
                    HFILL
                }
            },
            {
                &hf_wimaxasncp_message_type,
                {
                    "Message Type",
                    "wimaxasncp.message_type",
                    FT_UINT8,
                    BASE_HEX,
                    NULL,
                    0x1F,
                    "",
                    HFILL
                }
            },
            {
                &hf_wimaxasncp_qos_msg,
                {
                    "Message Type",
                    "wimaxasncp.qos_msg",
                    FT_UINT8,
                    BASE_HEX,
                    VALS(wimaxasncp_qos_msg_vals),
                    0x1F,
                    "",
                    HFILL
                }
            },
            {
                &hf_wimaxasncp_ho_control_msg,
                {
                    "Message Type",
                    "wimaxasncp.ho_control_msg",
                    FT_UINT8,
                    BASE_HEX,
                    VALS(wimaxasncp_ho_control_msg_vals),
                    0x1F,
                    "",
                    HFILL
                }
            },
            {
                &hf_wimaxasncp_data_path_control_msg,
                {
                    "Message Type",
                    "wimaxasncp.data_path_control_msg",
                    FT_UINT8,
                    BASE_HEX,
                    VALS(wimaxasncp_data_path_control_msg_vals),
                    0x1F,
                    "",
                    HFILL
                }
            },
            {
                &hf_wimaxasncp_context_delivery_msg,
                {
                    "Message Type",
                    "wimaxasncp.context_delivery_msg",
                    FT_UINT8,
                    BASE_HEX,
                    VALS(wimaxasncp_context_delivery_msg_vals),
                    0x1F,
                    "",
                    HFILL
                }
            },
            {
                &hf_wimaxasncp_r3_mobility_msg,
                {
                    "Message Type",
                    "wimaxasncp.r3_mobility_msg",
                    FT_UINT8,
                    BASE_HEX,
                    VALS(wimaxasncp_r3_mobility_msg_vals),
                    0x1F,
                    "",
                    HFILL
                }
            },
            {
                &hf_wimaxasncp_paging_msg,
                {
                    "Message Type",
                    "wimaxasncp.paging_msg",
                    FT_UINT8,
                    BASE_HEX,
                    VALS(wimaxasncp_paging_msg_vals),
                    0x1F,
                    "",
                    HFILL
                }
            },
            {
                &hf_wimaxasncp_rrm_msg,
                {
                    "Message Type",
                    "wimaxasncp.rrm_msg",
                    FT_UINT8,
                    BASE_HEX,
                    VALS(wimaxasncp_rrm_msg_vals),
                    0x1F,
                    "",
                    HFILL
                }
            },
            {
                &hf_wimaxasncp_authentication_msg,
                {
                    "Message Type",
                    "wimaxasncp.authentication_msg",
                    FT_UINT8,
                    BASE_HEX,
                    VALS(wimaxasncp_authentication_msg_vals),
                    0x1F,
                    "",
                    HFILL
                }
            },
            {
                &hf_wimaxasncp_ms_state_msg,
                {
                    "Message Type",
                    "wimaxasncp.ms_state_msg",
                    FT_UINT8,
                    BASE_HEX,
                    VALS(wimaxasncp_ms_state_msg_vals),
                    0x1F,
                    "",
                    HFILL
                }
            },
            {
                &hf_wimaxasncp_reauthentication_msg,
                {
                    "Message Type",
                    "wimaxasncp.reauthentication_msg",
                    FT_UINT8,
                    BASE_HEX,
                    VALS(wimaxasncp_reauthentication_msg_vals),
                    0x1F,
                    "",
                    HFILL
                }
            },
            {
                &hf_wimaxasncp_session_msg,
                {
                    "Message Type",
                    "wimaxasncp.session_msg",
                    FT_UINT8,
                    BASE_HEX,
                    VALS(wimaxasncp_session_msg_vals),
                    0x1F,
                    "",
                    HFILL
                }
            },
            {
                &hf_wimaxasncp_length,
                {
                    "Length",
                    "wimaxasncp.length",
                    FT_UINT16,
                    BASE_DEC,
                    NULL,
                    0x0,
                    "",
                    HFILL
                }
            },
            {
                &hf_wimaxasncp_msid,
                {
                    "MSID",
                    "wimaxasncp.msid",
                    FT_ETHER,
                    BASE_NONE,
                    NULL,
                    0x0,
                    "",
                    HFILL
                }
            },
            {
                &hf_wimaxasncp_reserved1,
                {
                    "Reserved",
                    "wimaxasncp.reserved1",
                    FT_UINT32,
                    BASE_HEX,
                    NULL,
                    0x0,
                    "",
                    HFILL
                }
            },
            {
                &hf_wimaxasncp_transaction_id,
                {
                    "Transaction ID",
                    "wimaxasncp.transaction_id",
                    FT_UINT16,
                    BASE_HEX,
                    NULL,
                    0x0,
                    "",
                    HFILL
                }
            },
            {
                &hf_wimaxasncp_reserved2,
                {
                    "Reserved",
                    "wimaxasncp.reserved2",
                    FT_UINT16,
                    BASE_HEX,
                    NULL,
                    0x0,
                    "",
                    HFILL
                }
            },
            {
                &hf_wimaxasncp_tlv,
                {
                    "TLV",
                    "wimaxasncp.tlv",
                    FT_BYTES,
                    BASE_HEX,
                    NULL,
                    0x0,
                    "",
                    HFILL
                }
            },
            {
                &hf_wimaxasncp_tlv_type,
                {
                    "Type",
                    "wimaxasncp.tlv.type",
                    FT_UINT16,
                    BASE_DEC,
                    NULL,
                    0x0,
                    "",
                    HFILL
                }
            },
            {
                &hf_wimaxasncp_tlv_length,
                {
                    "Length",
                    "wimaxasncp.tlv.length",
                    FT_UINT16,
                    BASE_DEC,
                    NULL,
                    0x0,
                    "",
                    HFILL
                }
            },
            {
                &hf_wimaxasncp_tlv_value_bytes,
                {
                    "Value",
                    "wimaxasncp.tlv_value_bytes",
                    FT_BYTES,
                    BASE_HEX,
                    NULL,
                    0x0,
                    "",
                    HFILL
                }
            },
            {
                &hf_wimaxasncp_tlv_value_bitflags16,
                {
                    "Value",
                    "wimaxasncp.tlv_value_bitflags16",
                    FT_UINT16,
                    BASE_HEX,
                    NULL,
                    0xffff,
                    "",
                    HFILL
                }
            },
            {
                &hf_wimaxasncp_tlv_value_bitflags32,
                {
                    "Value",
                    "wimaxasncp.tlv_value_bitflags32",
                    FT_UINT32,
                    BASE_HEX,
                    NULL,
                    0xffffffff,
                    "",
                    HFILL
                }
            },
            {
                &hf_wimaxasncp_tlv_value_protocol,
                {
                    "Value",
                    "wimaxasncp.tlv_value_protocol",
                    FT_UINT16,
                    BASE_DEC,
                    NULL,
                    0x0,
                    "",
                    HFILL
                }
            },
            {
                &hf_wimaxasncp_tlv_value_vendor_id,
                {
                    "Vendor ID",
                    "wimaxasncp.tlv_value_vendor_id",
                    FT_UINT24,
                    BASE_DEC,
                    NULL,
                    0x0,
                    "",
                    HFILL
                }
            }
        };

    /* ------------------------------------------------------------------------
     * Protocol subtree array
     * ------------------------------------------------------------------------
     */

    static gint *ett_base[] = {
            &ett_wimaxasncp,
            &ett_wimaxasncp_flags,
            &ett_wimaxasncp_tlv,
            &ett_wimaxasncp_tlv_value_bitflags16,
            &ett_wimaxasncp_tlv_value_bitflags32,
            &ett_wimaxasncp_tlv_protocol_list,
            &ett_wimaxasncp_tlv_port_range_list,
            &ett_wimaxasncp_tlv_ip_address_mask_list,
            &ett_wimaxasncp_tlv_ip_address_mask,
            &ett_wimaxasncp_tlv_eap,
            &ett_wimaxasncp_tlv_vendor_specific_information_field
    };

    /* ------------------------------------------------------------------------
     * load the XML dictionary
     * ------------------------------------------------------------------------
     */

    debug_parser = getenv("WIRESHARK_DEBUG_WIMAXASNCP_DICT_PARSER") != NULL;
    dump_dict    = getenv("WIRESHARK_DUMP_WIMAXASNCP_DICT") != NULL;

    dir = ep_strdup_printf(
        "%s" G_DIR_SEPARATOR_S "wimaxasncp",
        get_datafile_dir());

    wimaxasncp_dict = 
        wimaxasncp_dict_scan(dir, "dictionary.xml", debug_parser, &dict_error);

    if (dict_error)
    {
        report_failure("wimaxasncp - %s", dict_error);
        g_free(dict_error);
    }

    if (wimaxasncp_dict && dump_dict)
    {
        wimaxasncp_dict_print(stdout, wimaxasncp_dict);
    }

    /* ------------------------------------------------------------------------
     * build the hf and ett dictionary entries
     * ------------------------------------------------------------------------
     */

    wimaxasncp_build_dict.hf = 
        g_array_new(FALSE, TRUE, sizeof(hf_register_info));

    g_array_append_vals(
        wimaxasncp_build_dict.hf, hf_base, array_length(hf_base));

    wimaxasncp_build_dict.ett = 
        g_array_new(FALSE, TRUE, sizeof(gint*));

    g_array_append_vals(
        wimaxasncp_build_dict.ett, ett_base, array_length(ett_base));

    if (wimaxasncp_dict)
    {
        wimaxasncp_dict_tlv_t *tlv;

        /* For each TLV found in XML file */
        for (tlv = wimaxasncp_dict->tlvs; tlv; tlv = tlv->next)
        {
            if (tlv->enums)
            {
                /* Create array for enums */
                wimaxasncp_dict_enum_t *e;
                GArray* array = g_array_new(TRUE, TRUE, sizeof(value_string));

                /* Copy each entry into value_string array */
                for (e = tlv->enums; e; e = e->next)
                {
                    value_string item = { e->code, e->name };
                    g_array_append_val(array, item);
                }

                /* Set enums to use with this TLV */
                tlv->enum_vs = (value_string*)array->data;
            }

            add_tlv_reg_info(tlv);
        }
    }

    /* add an entry for unknown TLVs */
    add_tlv_reg_info(&wimaxasncp_tlv_not_found);

    /* ------------------------------------------------------------------------
     * complete registration
     * ------------------------------------------------------------------------
     */

        /* Register the protocol name and description */
    proto_wimaxasncp = proto_register_protocol(
            "WiMAX ASN Control Plane Protocol",
            "WiMAX ASN CP",
            "wimaxasncp");

        /* Required function calls to register the header fields and subtrees
         * used */
    proto_register_field_array(
        proto_wimaxasncp,
        (hf_register_info*)wimaxasncp_build_dict.hf->data, 
        wimaxasncp_build_dict.hf->len);

    proto_register_subtree_array(
        (gint**)wimaxasncp_build_dict.ett->data,
        wimaxasncp_build_dict.ett->len);

    /* The following debug will only be printed if the debug_enabled variable
     * is set programmatically.  Setting the value via preferences will not
     * work as it will be set too late to affect this code path.
     */
    if (debug_enabled)
    {
        if (wimaxasncp_dict)
        {
            wimaxasncp_dict_tlv_t *tlv;

            for (tlv = wimaxasncp_dict->tlvs; tlv; tlv = tlv->next)
            {
                printf(
                    "%s\n"
                    "  type                   = %d\n"
                    "  description            = %s\n"
                    "  decoder                = %s\n"
                    "  hf_root                = %d\n"
                    "  hf_value               = %d\n"
                    "  hf_ipv4                = %d\n"
                    "  hf_ipv6                = %d\n"
                    "  hf_bsid                = %d\n"
                    "  hf_protocol            = %d\n"
                    "  hf_port_low            = %d\n"
                    "  hf_port_high           = %d\n"
                    "  hf_ipv4_mask           = %d\n"
                    "  hf_ipv6_mask           = %d\n"
                    "  hf_vendor_id           = %d\n"
                    "  hf_vendor_rest_of_info = %d\n",
                    tlv->name,
                    tlv->type,
                    tlv->description,
                    val_to_str(
                        tlv->decoder, wimaxasncp_decode_type_vals, "Unknown"),
                    tlv->hf_root,
                    tlv->hf_value,
                    tlv->hf_ipv4,
                    tlv->hf_ipv6,
                    tlv->hf_bsid,
                    tlv->hf_protocol,
                    tlv->hf_port_low,
                    tlv->hf_port_high,
                    tlv->hf_ipv4_mask,
                    tlv->hf_ipv6_mask,
                    tlv->hf_vendor_id,
                    tlv->hf_vendor_rest_of_info);
            }
        }
    }

        /* Register this dissector by name */
    new_register_dissector("wimaxasncp", dissect_wimaxasncp, proto_wimaxasncp);

        /* Register preferences module (See Section 2.6 for more on
         * preferences) */
    wimaxasncp_module = prefs_register_protocol(
            proto_wimaxasncp,
            proto_reg_handoff_wimaxasncp);

        /* Register preferences */
    prefs_register_bool_preference(
            wimaxasncp_module,
            "show_transaction_id_d_bit",
            "Show transaction ID direction bit",
            "Show transaction ID direction bit separately from the rest of "
            "the transaction ID field.",
            &show_transaction_id_d_bit);

    prefs_register_bool_preference(
            wimaxasncp_module,
            "debug_enabled",
            "Enable debug output",
            "Print debug output to the console.",
            &debug_enabled);

    prefs_register_uint_preference(
        wimaxasncp_module, 
        "udp.wimax_port",
        "UDP Port for WiMAX ASN Control Plane Protocol",
        "Set UDP port for WiMAX ASN Control Plane Protocol",
        10, &global_wimaxasncp_udp_port);

}

/* ========================================================================= */
/* If this dissector uses sub-dissector registration add a registration
   routine.  This exact format is required because a script is used to find
   these routines and create the code that calls these routines.

   This function is also called by preferences whenever "Apply" is pressed
   (see prefs_register_protocol above) so it should accommodate being called
   more than once.
*/
void
proto_reg_handoff_wimaxasncp(void)
{
    static gboolean inited = FALSE;
    dissector_handle_t wimaxasncp_handle;
    static int currentPort = -1;

    memset(&wimaxasncp_handle, 0, sizeof(dissector_handle_t));

    if (!inited)
    {

        /*  Use new_create_dissector_handle() to indicate that
         *  dissect_wimaxasncp() returns the number of bytes it dissected (or
         *  0 if it thinks the packet does not belong to WiMAX ASN Control
         *  Plane).
         */
        wimaxasncp_handle = new_create_dissector_handle(
             dissect_wimaxasncp,
             proto_wimaxasncp);


         inited = TRUE;
    }

    if (currentPort != -1)
    {
        /* Remove any previous registered port */
        dissector_delete("udp.port", currentPort, wimaxasncp_handle);
    }

    /* Add the new one from preferences */
    currentPort = global_wimaxasncp_udp_port;
    dissector_add("udp.port", currentPort, wimaxasncp_handle);

    /* Find the EAP dissector */
    eap_handle = find_dissector("eap");
}
