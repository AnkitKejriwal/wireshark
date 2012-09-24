/* packet-foundry.c
 * Routines for the disassembly of Foundry LLC messages (currently
 * Foundry Discovery Protocol - FDP only)
 *
 * $Id$
 *
 * Copyright 2012 Joerg Mayer (see AUTHORS file)
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#include <glib.h>
#include <epan/packet.h>
#include <epan/strutil.h>
#include <epan/in_cksum.h>
#include "packet-llc.h"
#include <epan/oui.h>

static int hf_llc_foundry_pid = -1;

static int proto_fdp = -1;
/* FDP header */
static int hf_fdp_version = -1;
static int hf_fdp_holdtime = -1;
static int hf_fdp_checksum = -1;
/* TLV header */
static int hf_fdp_tlv_type = -1;
static int hf_fdp_tlv_length = -1;
/* Unknown element */
static int hf_fdp_unknown = -1;
static int hf_fdp_unknown_data = -1;
/* String element */
static int hf_fdp_string = -1;
static int hf_fdp_string_data = -1;
static int hf_fdp_string_text = -1;
/* Net? element */
static int hf_fdp_net = -1;
static int hf_fdp_net_unknown = -1;
static int hf_fdp_net_ip = -1;
static int hf_fdp_net_iplength = -1;

static gint ett_fdp = -1;
static gint ett_fdp_tlv_header = -1;
static gint ett_fdp_unknown = -1;
static gint ett_fdp_string = -1;
static gint ett_fdp_net = -1;

#define PROTO_SHORT_NAME "FDP"
#define PROTO_LONG_NAME "Foundry Discovery Protocol"

static const value_string foundry_pid_vals[] = {
	{ 0x2000,	"FDP" },

	{ 0,		NULL }
};

typedef enum {
	FDP_TYPE_NAME = 1,
	FDP_TYPE_NET = 2, /* Binary, Network information? */
	FDP_TYPE_PORT = 3,
	FDP_TYPE_CAPABILITIES = 4,
	FDP_TYPE_VERSION = 5,
	FDP_TYPE_MODEL = 6,
	FDP_TYPE_UNK0101 = 0x0101,
	FDP_TYPE_UNK0102 = 0x0102,
} fdp_type_t;

static const value_string fdp_type_vals[] = {
	{ FDP_TYPE_NAME,		"DeviceID"},
	{ FDP_TYPE_NET,			"Net?"},
	{ FDP_TYPE_PORT,		"Interface"},
	{ FDP_TYPE_CAPABILITIES,	"Capabilities"},
	{ FDP_TYPE_VERSION,		"Version"},
	{ FDP_TYPE_MODEL,		"Platform"},
	{ FDP_TYPE_UNK0101,		"Unknown-0101"},
	{ FDP_TYPE_UNK0102,		"Unknown-0102"},

	{ 0,    NULL }
};

static int
dissect_tlv_header(tvbuff_t *tvb, packet_info *pinfo _U_, int offset, int length _U_, proto_tree *tree)
{
	proto_item	*tlv_item;
	proto_tree	*tlv_tree;
	guint16		tlv_type;
	guint16		tlv_length;

	tlv_type = tvb_get_ntohs(tvb, offset);
	tlv_length = tvb_get_ntohs(tvb, offset + 2);

	tlv_item = proto_tree_add_text(tree, tvb, offset, 4,
		"Length %d, type %d = %s",
		tlv_length, tlv_type,
		val_to_str(tlv_type, fdp_type_vals, "Unknown (%d)"));

	tlv_tree = proto_item_add_subtree(tlv_item, ett_fdp_tlv_header);
	proto_tree_add_uint(tlv_tree, hf_fdp_tlv_type, tvb, offset, 2, tlv_type);
	offset += 2;

	proto_tree_add_uint(tlv_tree, hf_fdp_tlv_length, tvb, offset, 2, tlv_length);
	offset += 2;

	return offset;
}

static int
dissect_string_tlv(tvbuff_t *tvb, packet_info *pinfo, int offset, int length, proto_tree *tree, const char* type_string)
{
	proto_item	*string_item;
	proto_tree	*string_tree;
	guint8		*string_value;

	string_item = proto_tree_add_protocol_format(tree, hf_fdp_string,
		tvb, offset, length, "%s", type_string);

	string_tree = proto_item_add_subtree(string_item, ett_fdp_string);

	dissect_tlv_header(tvb, pinfo, offset, 4, string_tree);
	offset += 4;
	length -= 4;

	string_value = tvb_get_ephemeral_string(tvb, offset, length);
	proto_item_append_text(string_item, ": \"%s\"",
		format_text(string_value, strlen(string_value)));

	proto_tree_add_item(string_tree, hf_fdp_string_data, tvb, offset, length, ENC_NA);
	proto_tree_add_item(string_tree, hf_fdp_string_text, tvb, offset, length, ENC_ASCII);

	return offset;
}

static void
dissect_net_tlv(tvbuff_t *tvb, packet_info *pinfo, int offset, int length, proto_tree *tree)
{
	proto_item	*net_item;
	proto_tree	*net_tree;

	net_item = proto_tree_add_protocol_format(tree, hf_fdp_net,
		tvb, offset, length, "Net?");

	net_tree = proto_item_add_subtree(net_item, ett_fdp_net);

	dissect_tlv_header(tvb, pinfo, offset, 4, net_tree);
	offset += 4;
	length -= 4;

	proto_tree_add_item(net_tree, hf_fdp_net_unknown, tvb, offset, 7, ENC_NA);
	offset += 7;
	length -= 7;

	/* Length of IP address block in bytes */
	proto_tree_add_item(net_tree, hf_fdp_net_iplength, tvb, offset, 2, ENC_BIG_ENDIAN);
	offset += 2;
	length -= 2;

	while (length >= 4) {
		proto_tree_add_item(net_tree, hf_fdp_net_ip, tvb, offset, 4, ENC_NA);
		offset += 4;
		length -= 4;
	}
}

static void
dissect_unknown_tlv(tvbuff_t *tvb, packet_info *pinfo, int offset, int length, proto_tree *tree)
{
	proto_item	*unknown_item;
	proto_tree	*unknown_tree;
	guint16		tlv_type;

	tlv_type = tvb_get_ntohs(tvb, offset);

	unknown_item = proto_tree_add_protocol_format(tree, hf_fdp_unknown,
		tvb, offset, length, "Unknown element [%u]", tlv_type);

	unknown_tree = proto_item_add_subtree(unknown_item, ett_fdp_unknown);

	dissect_tlv_header(tvb, pinfo, offset, 4, unknown_tree);
	offset += 4;
	length -= 4;

	proto_tree_add_item(unknown_tree, hf_fdp_unknown_data, tvb, offset, length, ENC_NA);
}

static void
dissect_fdp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	proto_item *ti;
	proto_tree *fdp_tree = NULL;
	guint32 offset = 0;
	guint16 tlv_type;
	guint16 tlv_length;
	guint16 data_length;
	const char *type_string;

	col_set_str(pinfo->cinfo, COL_PROTOCOL, PROTO_SHORT_NAME);
	col_set_str(pinfo->cinfo, COL_INFO, PROTO_SHORT_NAME ":");

	if (tree) {
		data_length = tvb_reported_length_remaining(tvb, offset);

		ti = proto_tree_add_item(tree, proto_fdp, tvb, offset, -1, ENC_NA);
		fdp_tree = proto_item_add_subtree(ti, ett_fdp);

		proto_tree_add_item(fdp_tree, hf_fdp_version, tvb, offset, 1, ENC_NA);
		offset += 1;
		proto_tree_add_item(fdp_tree, hf_fdp_holdtime, tvb, offset, 1, ENC_NA);
		offset += 1;
		proto_tree_add_item(fdp_tree, hf_fdp_checksum, tvb, offset, 2, ENC_BIG_ENDIAN);
		offset += 2;

		/* Decode the individual TLVs */
		while (offset < data_length) {
			if (data_length - offset < 4) {
				proto_tree_add_text(fdp_tree, tvb, offset, 4,
					"Too few bytes left for TLV: %u (< 4)",
					data_length - offset);
				break;
			}
			tlv_type = tvb_get_ntohs(tvb, offset);
			tlv_length = tvb_get_ntohs(tvb, offset + 2);

			if ((tlv_length < 4) || (tlv_length > (data_length - offset))) {
				proto_tree_add_text(fdp_tree, tvb, offset, 0,
					"TLV with invalid length: %u", tlv_length);
				break;
			}
			type_string = val_to_str(tlv_type, fdp_type_vals, "[%u]");
			if (check_col(pinfo->cinfo, COL_INFO))
				col_append_fstr(pinfo->cinfo, COL_INFO, " %s", type_string);

			switch (tlv_type) {
			case FDP_TYPE_NAME:
			case FDP_TYPE_PORT:
			case FDP_TYPE_CAPABILITIES:
			case FDP_TYPE_VERSION:
			case FDP_TYPE_MODEL:
				dissect_string_tlv(tvb, pinfo, offset, tlv_length, fdp_tree, type_string);
				break;
			case FDP_TYPE_NET:
				dissect_net_tlv(tvb, pinfo, offset, tlv_length, fdp_tree);
				break;
			case FDP_TYPE_UNK0101:
			case FDP_TYPE_UNK0102:
			default:
				dissect_unknown_tlv(tvb, pinfo, offset, tlv_length, fdp_tree);
				break;
			}
			offset += tlv_length;
		}

	}
}

void
proto_register_fdp(void)
{
	static hf_register_info hf[] = {

	/* FDP header */
		{ &hf_fdp_version,
		{ "Version?",	"fdp.version", FT_UINT8, BASE_DEC, NULL,
			0x0, NULL, HFILL }},

		{ &hf_fdp_holdtime,
		{ "Holdtime",	"fdp.holdtime", FT_UINT16, BASE_DEC, NULL,
			0x0, NULL, HFILL }},

		{ &hf_fdp_checksum,
		{ "Checksum?",	"fdp.checksum", FT_UINT16, BASE_HEX, NULL,
			0x0, NULL, HFILL }},

	/* TLV header */
		{ &hf_fdp_tlv_type,
		{ "TLV type",	"fdp.tlv.type", FT_UINT16, BASE_DEC, VALS(fdp_type_vals),
			0x0, NULL, HFILL }},

		{ &hf_fdp_tlv_length,
		{ "TLV length",	"fdp.tlv.length", FT_UINT16, BASE_DEC, NULL,
			0x0, NULL, HFILL }},

	/* Unknown element */
		{ &hf_fdp_unknown,
		{ "Unknown",	"fdp.unknown", FT_PROTOCOL, BASE_NONE, NULL,
			0x0, NULL, HFILL }},

		{ &hf_fdp_unknown_data,
		{ "Unknown",	"fdp.unknown.data", FT_BYTES, BASE_NONE, NULL,
			0x0, NULL, HFILL }},

	/* String element */
		{ &hf_fdp_string,
		{ "DeviceID",	"fdp.deviceid", FT_PROTOCOL, BASE_NONE, NULL,
			0x0, NULL, HFILL }},

		{ &hf_fdp_string_data,
		{ "Data",	"fdp.string.data", FT_BYTES, BASE_NONE, NULL,
			0x0, NULL, HFILL }},

		{ &hf_fdp_string_text,
		{ "Text",	"fdp.string.text", FT_STRING, BASE_NONE, NULL,
			0x0, NULL, HFILL }},

	/* Net? element */
		{ &hf_fdp_net,
		{ "Net?",	"fdp.net", FT_PROTOCOL, BASE_NONE, NULL,
			0x0, NULL, HFILL }},

		{ &hf_fdp_net_unknown,
		{ "Net Unknown?",	"fdp.net.unknown", FT_BYTES, BASE_NONE, NULL,
			0x0, NULL, HFILL }},

		{ &hf_fdp_net_iplength,
		{ "Net IP Bytes?",	"fdp.net.iplength", FT_UINT16, BASE_DEC, NULL,
			0x0, "Number of bytes carrying IP addresses", HFILL }},

		{ &hf_fdp_net_ip,
		{ "Net IP Address?",	"fdp.net.ip", FT_IPv4, BASE_NONE, NULL,
			0x0, NULL, HFILL }},

        };
	static gint *ett[] = {
		&ett_fdp,
		&ett_fdp_tlv_header,
		&ett_fdp_unknown,
		&ett_fdp_net,
		&ett_fdp_string,
	};

        proto_fdp = proto_register_protocol(PROTO_LONG_NAME,
	    PROTO_SHORT_NAME, "fdp");
        proto_register_field_array(proto_fdp, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));
}

void
proto_reg_handoff_fdp(void)
{
	dissector_handle_t fdp_handle;

	fdp_handle = create_dissector_handle(dissect_fdp, proto_fdp);
	dissector_add_uint("llc.foundry_pid", 0x2000, fdp_handle);
}

void
proto_register_foundry_oui(void)
{
	static hf_register_info hf[] = {
	  { &hf_llc_foundry_pid,
		{ "PID",	"llc.foundry_pid",  FT_UINT16, BASE_HEX,
		  VALS(foundry_pid_vals), 0x0, NULL, HFILL }
	  }
	};

	llc_add_oui(OUI_FOUNDRY, "llc.foundry_pid", "Foundry OUI PID", hf);
}
