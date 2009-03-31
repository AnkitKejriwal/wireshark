/* packet-wassp.c
 * Routines for the disassembly of the Chantry/HiPath AP-Controller
 * tunneling protocol.
 *
 * $Id$
 *
 * Copyright 2009 Joerg Mayer (see AUTHORS file)
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
  TODO:
  - Improve heuristics!!!
  - Add many more TLV descriptions
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>
#include <epan/packet.h>
#include <epan/emem.h>
#include <epan/expert.h>

/* protocol handles */
static int proto_wassp = -1;

static dissector_handle_t snmp_handle;

/* ett handles */
static int ett_wassp = -1;
static int ett_wassp_tlv_header = -1;

/* hf elements */
/* tlv generic */
static int hf_wassp_tlv_type = -1;
static int hf_wassp_tlv_length = -1;
static int hf_wassp_tlv_data = -1;
/* tunnel header */
static int hf_wassp_version = -1;
static int hf_wassp_type = -1;
static int hf_wassp_seqno = -1;
static int hf_wassp_flags = -1;
static int hf_wassp_sessionid = -1;
static int hf_wassp_length = -1;
/* tunnel tlvs */
static int hf_status = -1;
static int hf_ru_soft_version = -1;
static int hf_ru_serial_number = -1;
static int hf_ru_challenge = -1;
static int hf_ru_response = -1;
static int hf_ac_ipaddr = -1;
static int hf_ru_vns_id = -1;
static int hf_tftp_server = -1;
static int hf_image_path = -1;
static int hf_ru_config = -1;
static int hf_ru_state = -1;
static int hf_ru_session_key = -1;
static int hf_message_type = -1;
static int hf_random_number = -1;
static int hf_standby_timeout = -1;
static int hf_ru_challenge_id = -1;
static int hf_ru_model = -1;
static int hf_ru_scan_mode = -1;
static int hf_ru_scan_type = -1;
static int hf_ru_scan_interval = -1;
static int hf_ru_radio_type = -1;
static int hf_ru_channel_dwell_time = -1;
static int hf_ru_channel_list = -1;
static int hf_ru_trap = -1;
static int hf_ru_scan_times = -1;
static int hf_ru_scan_delay = -1;
static int hf_ru_scan_req_id = -1;
static int hf_static_config = -1;
static int hf_local_bridging = -1;
static int hf_static_bp_ipaddr = -1;
static int hf_static_bp_netmask = -1;
static int hf_static_bp_gateway = -1;
static int hf_static_bm_ipaddr = -1;
static int hf_ru_alarm = -1;
static int hf_bp_request_id = -1;
static int hf_snmp_error_status = -1;
static int hf_snmp_error_index = -1;
static int hf_ap_img_to_ram = -1;
static int hf_ap_img_role = -1;
static int hf_unknown_65 = -1;
static int hf_unknown_65_62 = -1;
static int hf_unknown_65_63 = -1;
static int hf_unknown_65_64 = -1;
static int hf_unknown_65_70 = -1;
static int hf_unknown_69 = -1;
static int hf_unknown_69_1 = -1;
static int hf_unknown_69_2 = -1;
static int hf_wassp_vlan_tag = -1;
static int hf_wassp_tunnel_type = -1;
static int hf_ap_dhcp_mode = -1;
static int hf_ap_ipaddr = -1;
static int hf_ap_netmask = -1;
static int hf_ap_gateway = -1;
/* discover header */
static int hf_wassp_discover1 = -1;
/* static int hf_wassp_length = -1; */
static int hf_wassp_discover2 = -1;
static int hf_wassp_subtype = -1;
static int hf_wassp_ether = -1;
static int hf_wassp_discover3 = -1;
/* discover tlvs */
/* peer header */
/* peer tlvs */

#define PROTO_SHORT_NAME "WASSP"
#define PROTO_LONG_NAME "Wireless Access Station Session Protocol"

#define PORT_WASSP_DISCOVER	13907
#define PORT_WASSP_TUNNEL	13910
/* #define PORT_WASSP_PEER		13913?? */

/* ============= copy/paste/modify from value_string.[hc] ============== */
typedef struct _ext_value_string {
  guint32  value;
  const gchar   *strptr;
  int* hf_element;
  int (*specialfunction)(tvbuff_t *, packet_info *, proto_tree *, guint32,
	guint32, const struct _ext_value_string *);
  const struct _ext_value_string *evs;
} ext_value_string;


static const gchar*
match_strextval_idx(guint32 val, const ext_value_string *vs, gint *idx) {
  gint i = 0;

  if(vs) {
    while (vs[i].strptr) {
      if (vs[i].value == val) {
	if (idx)
	  *idx = i;
	return(vs[i].strptr);
      }
      i++;
    }
  }

  if (idx)
    *idx = -1;
  return NULL;
}

static const gchar*
extval_to_str_idx(guint32 val, const ext_value_string *vs, gint *idx, const char *fmt) {
  const gchar *ret;

  if (!fmt)
    fmt="Unknown";

  ret = match_strextval_idx(val, vs, idx);
  if (ret != NULL)
    return ret;

  return ep_strdup_printf(fmt, val);
}
/* ============= end copy/paste/modify  ============== */

/* Forward decls needed by wassp_tunnel_tlv_vals et al */
static int dissect_snmp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *wassp_tree,
	guint32 offset, guint32 length, const ext_value_string *value_array);
static int dissect_tlv(tvbuff_t *tvb, packet_info *pinfo, proto_tree *wassp_tree,
	guint32 offset, guint32 length, const ext_value_string *value_array);

static const ext_value_string wassp_tunnel_unknown69_2_tlv_vals[] = {

	{ 0,	NULL, NULL, NULL, NULL }
};

static const ext_value_string wassp_tunnel_unknown69_1_tlv_vals[] = {

	{ 0,	NULL, NULL, NULL, NULL }
};

static const ext_value_string wassp_tunnel_unknown69_tlv_vals[] = {
	{ 1, "Unknown69_1", NULL, dissect_tlv, wassp_tunnel_unknown69_1_tlv_vals },
	{ 2, "Unknown69_2", NULL, dissect_tlv, wassp_tunnel_unknown69_2_tlv_vals },

	{ 0,	NULL, NULL, NULL, NULL }
};

static const ext_value_string wassp_tunnel_unknown65_70_tlv_vals[] = {

	{ 0,	NULL, NULL, NULL, NULL }
};

static const ext_value_string wassp_tunnel_unknown65_64_tlv_vals[] = {

	{ 0,	NULL, NULL, NULL, NULL }
};

static const ext_value_string wassp_tunnel_unknown65_63_tlv_vals[] = {

	{ 0,	NULL, NULL, NULL, NULL }
};

static const ext_value_string wassp_tunnel_unknown65_62_tlv_vals[] = {

	{ 0,	NULL, NULL, NULL, NULL }
};

static const ext_value_string wassp_tunnel_unknown65_tlv_vals[] = {
	{ 62, "Unknown65_62", NULL, dissect_tlv, wassp_tunnel_unknown65_62_tlv_vals },
	{ 63, "Unknown65_63", NULL, dissect_tlv, wassp_tunnel_unknown65_63_tlv_vals },
	{ 64, "Unknown65_64", NULL, dissect_tlv, wassp_tunnel_unknown65_64_tlv_vals },
	{ 70, "Unknown65_70", NULL, dissect_tlv, wassp_tunnel_unknown65_70_tlv_vals },

	{ 0,	NULL, NULL, NULL, NULL }
};

static const ext_value_string wassp_tunnel_tlv_vals[] = {
	{ 1, "STATUS", &hf_status, NULL, NULL },
	{ 2, "RU-SOFT-VERSION", &hf_ru_soft_version, NULL, NULL },
	{ 3, "RU-SERIAL-NUMBER", &hf_ru_serial_number, NULL, NULL },
	{ 4, "RU-CHALLENGE", &hf_ru_challenge, NULL, NULL },
	{ 5, "RU-RESPONSE", &hf_ru_response, NULL, NULL },
	{ 6, "AC-IPADDR", &hf_ac_ipaddr, NULL, NULL },
	{ 7, "RU-VNS-ID", &hf_ru_vns_id, NULL, NULL },
	{ 8, "TFTP-SERVER", &hf_tftp_server, NULL, NULL },
	{ 9, "IMAGE-PATH", &hf_image_path, NULL, NULL },
	{ 10, "RU-CONFIG", &hf_ru_config, dissect_snmp, NULL },
	{ 11, "RU-STATE", &hf_ru_state, NULL, NULL },
	{ 12, "RU-SESSION-KEY", &hf_ru_session_key, NULL, NULL },
	{ 13, "MESSAGE-TYPE", &hf_message_type, NULL, NULL },
	{ 14, "RANDOM-NUMBER", &hf_random_number, NULL, NULL },
	{ 15, "STANDBY-TIMEOUT", &hf_standby_timeout, NULL, NULL },
	{ 16, "RU-CHALLENGE-ID", &hf_ru_challenge_id, NULL, NULL },
	{ 17, "RU-MODEL", &hf_ru_model, NULL, NULL },
	{ 18, "RU-SCAN-MODE", &hf_ru_scan_mode, NULL, NULL },
	{ 19, "RU-SCAN-TYPE", &hf_ru_scan_type, NULL, NULL },
	{ 20, "RU-SCAN-INTERVAL", &hf_ru_scan_interval, NULL, NULL },
	{ 21, "RU-RADIO-TYPE", &hf_ru_radio_type, NULL, NULL },
	{ 22, "RU-CHANNEL-DWELL-TIME", &hf_ru_channel_dwell_time, NULL, NULL },
	{ 23, "RU-CHANNEL-LIST", &hf_ru_channel_list, NULL, NULL },
	{ 24, "RU-TRAP", &hf_ru_trap, NULL, NULL },
	{ 25, "RU-SCAN-TIMES", &hf_ru_scan_times, NULL, NULL },
	{ 26, "RU-SCAN-DELAY", &hf_ru_scan_delay, NULL, NULL },
	{ 27, "RU-SCAN-REQ-ID", &hf_ru_scan_req_id, NULL, NULL },
	{ 28, "STATIC-CONFIG", &hf_static_config, NULL, NULL },
	{ 29, "LOCAL-BRIDGING", &hf_local_bridging, NULL, NULL },
	{ 30, "STATIC-BP-IPADDR", &hf_static_bp_ipaddr, NULL, NULL },
	{ 31, "STATIC-BP-NETMASK", &hf_static_bp_netmask, NULL, NULL },
	{ 32, "STATIC-BP-GATEWAY", &hf_static_bp_gateway, NULL, NULL },
	{ 33, "STATIC-BM-IPADDR", &hf_static_bm_ipaddr, NULL, NULL },
	{ 38, "RU-ALARM", &hf_ru_alarm, dissect_snmp, NULL },
	{ 46, "BP-REQUEST-ID", &hf_bp_request_id, NULL, NULL },
	{ 60, "SNMP-ERROR-STATUS", &hf_snmp_error_status, NULL, NULL },
	{ 61, "SNMP-ERROR-INDEX", &hf_snmp_error_index, NULL, NULL },
	{ 63, "AP-IMG-TO-RAM", &hf_ap_img_to_ram, NULL, NULL },
	{ 64, "AP-IMG-ROLE", &hf_ap_img_role, NULL, NULL },
	{ 65, "UNKNOWN-65", &hf_unknown_65, dissect_tlv, wassp_tunnel_unknown65_tlv_vals },
	{ 69, "UNKNOWN-69", NULL, dissect_tlv, wassp_tunnel_unknown69_tlv_vals },
	{ 76, "WASSP-VLAN-TAG", &hf_wassp_vlan_tag, NULL, NULL },
	{ 81, "WASSP-TUNNEL-TYPE", &hf_wassp_tunnel_type, NULL, NULL },
	{ 88, "AP-DHCP-MODE", &hf_ap_dhcp_mode, NULL, NULL },
	{ 89, "AP-IPADDR", &hf_ap_ipaddr, NULL, NULL },
	{ 90, "AP-NETMASK", &hf_ap_netmask, NULL, NULL },
	{ 91, "AP-GATEWAY", &hf_ap_gateway, NULL, NULL },
	{ 91, "AP-GATEWAY", &hf_ap_gateway, NULL, NULL },

	{ 0,	NULL, NULL, NULL, NULL }
};

static const value_string wassp_tunnel_pdu_type[] = {
	{ 1, "?Discover?" },
	{ 2, "RU Registration Request" },
	{ 3, "RU Registration Response" },
	{ 4, "RU Authentication Request" },
	{ 5, "RU Authentication Response" },
	{ 6, "RU Software Version Validate Request" },
	{ 7, "RU Software Version Validate Response" },
	{ 8, "RU Configuration Request" },
	{ 9, "RU Configuration Response" },
	{ 10, "RU Acknowledge" },
	{ 11, "RU Configuration Status Notify" },
	{ 12, "RU Set State Request" },
	{ 13, "RU Set State Response" },
	{ 14, "RU Statistics Notify" },
	{ 15, "Data" },
	{ 16, "Poll" },
	{ 17, "SNMP Request" },
	{ 18, "SNMP Response" },
	{ 19, "BP Trap Notify" },
	{ 20, "BP Scan Request" },
	{ 21, "RFM Notify" },
	{ 22, "RU SNMP Alarm Notify" },
	{ 28, "RU Stats Req" },
	{ 29, "RU Stats Resp" },
	{ 30, "MU Stats Req" },

	{ 0,	NULL }
};

#if 0
static const value_string wassp_setresult_vals[] = {
	{ 0,	"Success" },
	{ 1,	"Failauth" },

	{ 0,	NULL }
};
#endif

static int
dissect_snmp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *wassp_tree,
	volatile guint32 offset, guint32 length, const ext_value_string *value_array _U_)
{
	tvbuff_t *snmp_tvb;

	/* Don't add SNMP stuff to the info column */
	if (check_col(pinfo->cinfo, COL_INFO)) 
		col_set_writable(pinfo->cinfo, FALSE);

	snmp_tvb = tvb_new_subset(tvb, offset, length, length);

	/* Continue after SNMP dissection errors */
	TRY {
		call_dissector(snmp_handle, snmp_tvb, pinfo, wassp_tree);
	} CATCH2(BoundsError, ReportedBoundsError) {
		expert_add_info_format(pinfo, NULL,
			PI_MALFORMED, PI_ERROR,
			"Malformed or short SNMP subpacket");

		if (check_col(pinfo->cinfo, COL_INFO))
			col_append_str(pinfo->cinfo, COL_INFO,
				" [Malformed or short SNMP subpacket] " );
	} ENDTRY;

	if (check_col(pinfo->cinfo, COL_INFO)) 
		col_set_writable(pinfo->cinfo, TRUE);

	offset += length;

	return offset;
}

static int
dissect_tlv(tvbuff_t *tvb, packet_info *pinfo, proto_tree *wassp_tree,
	guint32 offset, guint32 length _U_, const ext_value_string *value_array)
{
	guint32 tlv_type;
	guint32 tlv_length;
	proto_item *tlv_item;
	proto_item *tlv_tree;
	proto_item *type_item;
	int type_index;
	guint32 tlv_end;

	tlv_type = tvb_get_ntohs(tvb, offset);
	tlv_length = tvb_get_ntohs(tvb, offset + 2);
	DISSECTOR_ASSERT(tlv_length >= 4);
	tlv_item = proto_tree_add_text(wassp_tree, tvb,
		offset, tlv_length,
		"T %d, L %d: %s",
		tlv_type,
		tlv_length,
		extval_to_str_idx(tlv_type, value_array, NULL, "Unknown"));
	tlv_tree = proto_item_add_subtree(tlv_item,
		ett_wassp_tlv_header);
	type_item = proto_tree_add_item(tlv_tree, hf_wassp_tlv_type,
		tvb, offset, 2, FALSE);
	proto_item_append_text(type_item, " = %s",
		extval_to_str_idx(tlv_type, value_array,
			&type_index, "Unknown"));
	offset += 2;
	proto_tree_add_item(tlv_tree, hf_wassp_tlv_length,
		tvb, offset, 2, FALSE);
	offset += 2;

	tlv_length -= 4;

	if (tlv_length == 0)
		return offset;

	tlv_end = offset + tlv_length;

	/* Make hf_ handling independent of specialfuncion */
	if ( type_index != -1 && value_array[type_index].hf_element) {
		proto_tree_add_item(tlv_tree,
			*(value_array[type_index].hf_element),
			tvb, offset, tlv_length, FALSE);
	} else {
		proto_tree_add_item(tlv_tree, hf_wassp_tlv_data,
			tvb, offset, tlv_length, FALSE);
	}
	if ( type_index != -1 && value_array[type_index].specialfunction ) {
		guint32 newoffset;

		while (offset < tlv_end) {
			newoffset = value_array[type_index].specialfunction (
				tvb, pinfo, tlv_tree, offset, tlv_length,
				value_array[type_index].evs);
			DISSECTOR_ASSERT(newoffset > offset);
			offset = newoffset;
		}
	}
	return tlv_end;
}

static int
dissect_wassp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	proto_item *ti;
	proto_tree *wassp_tree = NULL;
	guint32 offset = 0;
	guint32 packet_length;
	guint8 packet_type;
	guint32 subtype;

	packet_type = tvb_get_guint8(tvb, 1);
	if (check_col(pinfo->cinfo, COL_PROTOCOL))
		col_set_str(pinfo->cinfo, COL_PROTOCOL, PROTO_SHORT_NAME);
	if (check_col(pinfo->cinfo, COL_INFO))
		col_add_str(pinfo->cinfo, COL_INFO, val_to_str(packet_type,
			wassp_tunnel_pdu_type, "Type 0x%02x"));

	if (tree) {
		ti = proto_tree_add_item(tree, proto_wassp, tvb, offset, -1,
		    FALSE);
		wassp_tree = proto_item_add_subtree(ti, ett_wassp);

		proto_tree_add_item(wassp_tree, hf_wassp_version, tvb, offset, 1,
			FALSE);
		offset += 1;

		proto_tree_add_item(wassp_tree, hf_wassp_type, tvb, offset, 1,
			FALSE);
		offset += 1;

		switch (packet_type) {
		case 1: /* Discover ??? */
			proto_tree_add_item(wassp_tree, hf_wassp_discover1, tvb, offset, 2,
				FALSE);
			offset += 2;
			packet_length = tvb_get_ntohs(tvb, offset);
			proto_tree_add_item(wassp_tree, hf_wassp_length, tvb, offset, 2,
				FALSE);
			offset += 2;
			proto_tree_add_item(wassp_tree, hf_wassp_discover2, tvb, offset, 2,
				FALSE);
			offset += 2;
			subtype = tvb_get_ntohs(tvb, offset);
			proto_tree_add_item(wassp_tree, hf_wassp_subtype, tvb, offset, 2,
				FALSE);
			offset += 2;
			switch (subtype) {
			case 1:
				proto_tree_add_item(wassp_tree, hf_wassp_ether, tvb, offset, 6,
					FALSE);
				offset += 6;
				break;
			case 2:
				proto_tree_add_item(wassp_tree, hf_wassp_discover3, tvb, offset, 2,
					FALSE);
				offset += 2;
				break;
			}
			break;
		default:
			proto_tree_add_item(wassp_tree, hf_wassp_seqno, tvb, offset, 1,
				FALSE);
			offset += 1;
	
			proto_tree_add_item(wassp_tree, hf_wassp_flags, tvb, offset, 1,
				FALSE);
			offset += 1;
	
			proto_tree_add_item(wassp_tree, hf_wassp_sessionid, tvb, offset, 2,
				FALSE);
			offset += 2;

			packet_length = tvb_get_ntohs(tvb, offset);
			proto_tree_add_item(wassp_tree, hf_wassp_length, tvb, offset, 2,
				FALSE);
			offset += 2;

			break;
		}
		while (offset < packet_length)
			offset = dissect_tlv(tvb, pinfo, wassp_tree,
				offset, 0, wassp_tunnel_tlv_vals);
	}
	return offset;
}

static gboolean
test_wassp(tvbuff_t *tvb)
{
	/* Minimum of 8 bytes, first byte (version) has value of 3 */
	if ( tvb_length(tvb) < 8
		    || tvb_get_guint8(tvb, 0) != 3
		    /* || tvb_get_guint8(tvb, 2) != 0
		    || tvb_get_ntohs(tvb, 6) > tvb_reported_length(tvb) */
	) {
		return FALSE;
	}
	return TRUE;
}

#if 0
static gboolean
dissect_wassp_heur(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	if ( !test_wassp(tvb) ) {
		return FALSE;
	}
	dissect_wassp(tvb, pinfo, tree);
	return TRUE;
}
#endif

static int
dissect_wassp_static(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	if ( !test_wassp(tvb) ) {
		return 0;
	}
	return dissect_wassp(tvb, pinfo, tree);
}

void
proto_register_wassp(void)
{
	static hf_register_info hf[] = {

	/* TLV fields */
		{ &hf_wassp_tlv_type,
		{ "TlvType",	"wassp.tlv.type", FT_UINT8, BASE_DEC, NULL,
			0x0, NULL, HFILL }},

		{ &hf_wassp_tlv_length,
		{ "TlvLength",	"wassp.tlv.length", FT_UINT8, BASE_DEC, NULL,
			0x0, NULL, HFILL }},

		{ &hf_wassp_tlv_data,
		{ "TlvData",   "wassp.tlv.data", FT_BYTES, BASE_NONE, NULL,
			0x0, NULL, HFILL }},

	/* WASSP tunnel header */
		{ &hf_wassp_version,
		{ "Protocol Version",	"wassp.version", FT_UINT8, BASE_DEC, NULL,
			0x0, NULL, HFILL }},

		{ &hf_wassp_type,
		{ "PDU Type",	"wassp.type", FT_UINT8, BASE_DEC, VALS(wassp_tunnel_pdu_type),
			0x0, NULL, HFILL }},

		{ &hf_wassp_seqno,
		{ "Sequence No",	"wassp.seqno", FT_UINT8, BASE_DEC, NULL,
			0x0, NULL, HFILL }},

		{ &hf_wassp_flags,
		{ "Flags",	"wassp.flags", FT_UINT8, BASE_HEX, NULL,
			0x0, NULL, HFILL }},

		{ &hf_wassp_sessionid,
		{ "Session ID",	"wassp.sessionid", FT_UINT8, BASE_DEC, NULL,
			0x0, NULL, HFILL }},

		{ &hf_wassp_length,
		{ "PDU Length",	"wassp.length", FT_UINT8, BASE_HEX, NULL,
			0x0, NULL, HFILL }},

	/* WASSP tunnel data */
		{ &hf_status,
		{ "STATUS", "wassp.status", FT_UINT32, BASE_DEC, NULL,
				0x0, NULL, HFILL }},

		{ &hf_ru_soft_version,
		{ "RU-SOFT-VERSION", "wassp.ru.soft.version", FT_STRING, BASE_DEC, NULL,
				0x0, NULL, HFILL }},

		{ &hf_ru_serial_number,
		{ "RU-SERIAL-NUMBER", "wassp.ru.serial.number", FT_STRING, BASE_DEC, NULL,
				0x0, NULL, HFILL }},

		{ &hf_ru_challenge,
		{ "RU-CHALLENGE", "wassp.ru.challenge", FT_BYTES, BASE_NONE, NULL,
				0x0, NULL, HFILL }},

		{ &hf_ru_response,
		{ "RU-RESPONSE", "wassp.ru.response", FT_BYTES, BASE_NONE, NULL,
				0x0, NULL, HFILL }},

		{ &hf_ac_ipaddr,
		{ "AC-IPADDR", "wassp.ac.ipaddr", FT_IPv4, BASE_NONE, NULL,
				0x0, NULL, HFILL }},

		{ &hf_ru_vns_id,
		{ "RU-VNS-ID", "wassp.ru.vns.id", FT_UINT32, BASE_DEC, NULL,
				0x0, NULL, HFILL }},

		{ &hf_tftp_server,
		{ "TFTP-SERVER", "wassp.tftp.server", FT_IPv4, BASE_NONE, NULL,
				0x0, NULL, HFILL }},

		{ &hf_image_path,
		{ "IMAGE-PATH", "wassp.image.path", FT_STRING, BASE_DEC, NULL,
				0x0, NULL, HFILL }},

		{ &hf_ru_config,
		{ "RU-CONFIG", "wassp.ru.config", FT_NONE, BASE_NONE, NULL,
				0x0, NULL, HFILL }},

		{ &hf_ru_state,
		{ "RU-STATE", "wassp.ru.state", FT_UINT32, BASE_DEC, NULL,
				0x0, NULL, HFILL }},

		{ &hf_ru_session_key,
		{ "RU-SESSION-KEY", "wassp.ru.session.key", FT_STRING, BASE_DEC, NULL,
				0x0, NULL, HFILL }},

		{ &hf_message_type,
		{ "MESSAGE-TYPE", "wassp.message.type", FT_UINT32, BASE_DEC, NULL,
				0x0, NULL, HFILL }},

		{ &hf_random_number,
		{ "RANDOM-NUMBER", "wassp.random.number", FT_BYTES, BASE_NONE, NULL,
				0x0, NULL, HFILL }},

		{ &hf_standby_timeout,
		{ "STANDBY-TIMEOUT", "wassp.standby.timeout", FT_UINT32, BASE_DEC, NULL,
				0x0, NULL, HFILL }},

		{ &hf_ru_challenge_id,
		{ "RU-CHALLENGE-ID", "wassp.ru.challenge.id", FT_UINT32, BASE_DEC, NULL,
				0x0, NULL, HFILL }},

		{ &hf_ru_model,
		{ "RU-MODEL", "wassp.ru.model", FT_STRING, BASE_DEC, NULL,
				0x0, NULL, HFILL }},

		{ &hf_ru_scan_mode,
		{ "RU-SCAN-MODE", "wassp.ru.scan.mode", FT_UINT32, BASE_DEC, NULL,
				0x0, NULL, HFILL }},

		{ &hf_ru_scan_type,
		{ "RU-SCAN-TYPE", "wassp.ru.scan.type", FT_UINT32, BASE_DEC, NULL,
				0x0, NULL, HFILL }},

		{ &hf_ru_scan_interval,
		{ "RU-SCAN-INTERVAL", "wassp.ru.scan.interval", FT_UINT32, BASE_DEC, NULL,
				0x0, NULL, HFILL }},

		{ &hf_ru_radio_type,
		{ "RU-RADIO-TYPE", "wassp.ru.radio.type", FT_UINT32, BASE_DEC, NULL,
				0x0, NULL, HFILL }},

		{ &hf_ru_channel_dwell_time,
		{ "RU-CHANNEL-DWELL-TIME", "wassp.ru.channel.dwell.time", FT_UINT32, BASE_DEC, NULL,
				0x0, NULL, HFILL }},

		{ &hf_ru_channel_list,
		{ "RU-CHANNEL-LIST", "wassp.ru.channel.list", FT_UINT32, BASE_DEC, NULL,
				0x0, NULL, HFILL }},

		{ &hf_ru_trap,
		{ "RU-TRAP", "wassp.ru.trap", FT_STRING, BASE_DEC, NULL,
				0x0, NULL, HFILL }},

		{ &hf_ru_scan_times,
		{ "RU-SCAN-TIMES", "wassp.ru.scan.times", FT_UINT32, BASE_DEC, NULL,
				0x0, NULL, HFILL }},

		{ &hf_ru_scan_delay,
		{ "RU-SCAN-DELAY", "wassp.ru.scan.delay", FT_UINT32, BASE_DEC, NULL,
				0x0, NULL, HFILL }},

		{ &hf_ru_scan_req_id,
		{ "RU-SCAN-REQ-ID", "wassp.ru.scan.req.id", FT_UINT32, BASE_DEC, NULL,
				0x0, NULL, HFILL }},

		{ &hf_static_config,
		{ "STATIC-CONFIG", "wassp.static.config", FT_UINT32, BASE_DEC, NULL,
				0x0, NULL, HFILL }},

		{ &hf_local_bridging,
		{ "LOCAL-BRIDGING", "wassp.local.bridging", FT_UINT32, BASE_DEC, NULL,
				0x0, NULL, HFILL }},

		{ &hf_static_bp_ipaddr,
		{ "STATIC-BP-IPADDR", "wassp.static.bp.ipaddr", FT_IPv4, BASE_NONE, NULL,
				0x0, NULL, HFILL }},

		{ &hf_static_bp_netmask,
		{ "STATIC-BP-NETMASK", "wassp.static.bp.netmask", FT_IPv4, BASE_NONE, NULL,
				0x0, NULL, HFILL }},

		{ &hf_static_bp_gateway,
		{ "STATIC-BP-GATEWAY", "wassp.static.bp.gateway", FT_IPv4, BASE_NONE, NULL,
				0x0, NULL, HFILL }},

		{ &hf_static_bm_ipaddr,
		{ "STATIC-BM-IPADDR", "wassp.static.bm.ipaddr", FT_IPv4, BASE_NONE, NULL,
				0x0, NULL, HFILL }},

		{ &hf_ru_alarm,
		{ "RU-ALARM", "wassp.ru.alarm", FT_NONE, BASE_NONE, NULL,
				0x0, NULL, HFILL }},

		{ &hf_bp_request_id,
		{ "BP-REQUEST-ID", "wassp.bp.request.id", FT_UINT32, BASE_DEC, NULL,
				0x0, NULL, HFILL }},

		{ &hf_snmp_error_status,
		{ "SNMP-ERROR-STATUS", "wassp.snmp.error.status", FT_UINT32, BASE_DEC, NULL,
				0x0, NULL, HFILL }},

		{ &hf_snmp_error_index,
		{ "SNMP-ERROR-INDEX", "wassp.snmp.error.index", FT_UINT32, BASE_DEC, NULL,
				0x0, NULL, HFILL }},

		{ &hf_ap_img_to_ram,
		{ "AP-IMG-TO-RAM", "wassp.ap.img.to.ram", FT_UINT32, BASE_DEC, NULL,
				0x0, NULL, HFILL }},

		{ &hf_ap_img_role,
		{ "AP-IMG-ROLE", "wassp.ap.img.role", FT_UINT32, BASE_DEC, NULL,
				0x0, NULL, HFILL }},

		{ &hf_unknown_65,
		{ "UNKNOWN-65", "wassp.unknown65", FT_NONE, BASE_NONE, NULL,
				0x0, NULL, HFILL }},

		{ &hf_unknown_69,
		{ "UNKNOWN-65", "wassp.unknown69", FT_NONE, BASE_NONE, NULL,
				0x0, NULL, HFILL }},

		{ &hf_wassp_vlan_tag,
		{ "WASSP-VLAN-TAG", "wassp.wassp.vlan.tag", FT_INT32, BASE_DEC, NULL,
				0x0, NULL, HFILL }},

		{ &hf_wassp_tunnel_type,
		{ "WASSP-TUNNEL-TYPE", "wassp.wassp.tunnel.type", FT_UINT32, BASE_DEC, NULL,
				0x0, NULL, HFILL }},

		{ &hf_ap_dhcp_mode,
		{ "AP-DHCP-MODE", "wassp.ap.dhcp.mode", FT_UINT32, BASE_DEC, NULL,
				0x0, NULL, HFILL }},

		{ &hf_ap_ipaddr,
		{ "AP-IPADDR", "wassp.ap.ipaddr", FT_IPv4, BASE_NONE, NULL,
				0x0, NULL, HFILL }},

		{ &hf_ap_netmask,
		{ "AP-NETMASK", "wassp.ap.netmask", FT_IPv4, BASE_NONE, NULL,
				0x0, NULL, HFILL }},

		{ &hf_ap_gateway,
		{ "AP-GATEWAY", "wassp.ap.gateway", FT_IPv4, BASE_NONE, NULL,
				0x0, NULL, HFILL }},

	/* WASSP tunnel subtypes unknown 65 */
		{ &hf_unknown_65_62,
		{ "UNKNOWN-65-62", "wassp.unknown65.62", FT_BYTES, BASE_NONE, NULL,
				0x0, NULL, HFILL }},

		{ &hf_unknown_65_63,
		{ "UNKNOWN-65-63", "wassp.unknown65.63", FT_BYTES, BASE_NONE, NULL,
				0x0, NULL, HFILL }},

		{ &hf_unknown_65_64,
		{ "UNKNOWN-65-64", "wassp.unknown65.64", FT_BYTES, BASE_NONE, NULL,
				0x0, NULL, HFILL }},

		{ &hf_unknown_65_70,
		{ "UNKNOWN-65-70", "wassp.unknown65.70", FT_BYTES, BASE_NONE, NULL,
				0x0, NULL, HFILL }},

	/* WASSP tunnel subtypes unknown 69 */
		{ &hf_unknown_69_1,
		{ "UNKNOWN-69-1", "wassp.unknown69.1", FT_BYTES, BASE_NONE, NULL,
				0x0, NULL, HFILL }},

		{ &hf_unknown_69_2,
		{ "UNKNOWN-69-2", "wassp.unknown69.2", FT_BYTES, BASE_NONE, NULL,
				0x0, NULL, HFILL }},

	/* WASSP discover header */
		{ &hf_wassp_discover1,
		{ "Discover Header1",	"wassp.discover1", FT_UINT8, BASE_HEX, NULL,
			0x0, NULL, HFILL }},

		/* { &hf_wassp_length, */ /* see tunnel header */

		{ &hf_wassp_discover2,
		{ "Discover Header2",	"wassp.discover2", FT_UINT8, BASE_HEX, NULL,
			0x0, NULL, HFILL }},

		{ &hf_wassp_subtype,
		{ "Discover Subtype",	"wassp.subtype", FT_UINT8, BASE_DEC, NULL,
			0x0, NULL, HFILL }},

		{ &hf_wassp_ether,
		{ "Discover Ether",	"wassp.ether", FT_ETHER, BASE_NONE, NULL,
			0x0, NULL, HFILL }},

		{ &hf_wassp_discover3,
		{ "Discover Header3",	"wassp.discover3", FT_UINT8, BASE_HEX, NULL,
			0x0, NULL, HFILL }},

	};
	static gint *ett[] = {
		&ett_wassp,
		&ett_wassp_tlv_header,
	};

	proto_wassp = proto_register_protocol(PROTO_LONG_NAME, PROTO_SHORT_NAME, "wassp");
	proto_register_field_array(proto_wassp, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));
}

void
proto_reg_handoff_wassp(void)
{
	dissector_handle_t wassp_handle;


	wassp_handle = new_create_dissector_handle(dissect_wassp_static, proto_wassp);
	dissector_add("udp.port", PORT_WASSP_DISCOVER, wassp_handle);
	dissector_add("udp.port", PORT_WASSP_TUNNEL, wassp_handle);
	/* dissector_add("udp.port", PORT_WASSP_PEER, wassp_handle); */
#if 0
	heur_dissector_add("udp", dissect_wassp_heur, proto_wassp);
#endif

	snmp_handle = find_dissector("snmp");
}

