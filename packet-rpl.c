/* packet-rpl.c
 * Routines for RPL
 * Jochen Friedrich <jochen@scram.de>
 *
 * $Id: packet-rpl.c,v 1.3 2002/11/05 22:50:42 guy Exp $
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glib.h>
#include <epan/packet.h>

#include "llcsaps.h"
#include "packet-llc.h"

static int proto_rpl          = -1;

static int hf_rpl_type        = -1;
static int hf_rpl_corrval     = -1;
static int hf_rpl_respval     = -1;
static int hf_rpl_maxframe    = -1;
static int hf_rpl_connclass   = -1;
static int hf_rpl_lmac        = -1;
static int hf_rpl_smac        = -1;
static int hf_rpl_sap         = -1;
static int hf_rpl_equipment   = -1;
static int hf_rpl_memsize     = -1;
static int hf_rpl_bsmversion  = -1;
static int hf_rpl_adapterid   = -1;
static int hf_rpl_shortname   = -1;
static int hf_rpl_laddress    = -1;
static int hf_rpl_xaddress    = -1;
static int hf_rpl_sequence    = -1;
static int hf_rpl_config      = -1;
static int hf_rpl_flags       = -1;
static int hf_rpl_data        = -1;
static int hf_rpl_ec          = -1;

static gint ett_rpl           = -1;
static gint ett_rpl_0004      = -1;
static gint ett_rpl_0008      = -1;
static gint ett_rpl_4003      = -1;
static gint ett_rpl_4006      = -1;
static gint ett_rpl_4007      = -1;
static gint ett_rpl_4009      = -1;
static gint ett_rpl_400a      = -1;
static gint ett_rpl_400b      = -1;
static gint ett_rpl_400c      = -1;
static gint ett_rpl_4011      = -1;
static gint ett_rpl_4018      = -1;
static gint ett_rpl_c005      = -1;
static gint ett_rpl_c014      = -1;
static gint ett_rpl_unkn      = -1;

static const value_string rpl_type_vals[] = {
	{ 1,		"FIND Command" },
	{ 2,	 	"FOUND Frame" },
	{ 4,		"Search Vector" },
	{ 8,		"Connect Info Vector" },
	{ 0x10,		"Send File Request" },
	{ 0x20,		"File Data Response" },
	{ 0x4003,	"Correlator Vector" },
	{ 0x4006,	"Loader Address Vector" },
	{ 0x4007,	"Loader SAP Vector" },
	{ 0x4009,	"Frame Size Sub-Vector" },
	{ 0x400a,	"Connect Class Sub-Vector" },
	{ 0x400b,	"Response Correlator" },
	{ 0x400c,	"Set Address Vector" },
	{ 0x4011,	"Sequence Header" },
	{ 0x4018,	"File Data Vector" },
	{ 0xc005,	"Loader Info Sub-Vector" },
	{ 0xc014,	"Loader Header" },
	{ 0x0,	NULL }
};

static dissector_handle_t data_handle;

static void
dissect_rpl_container(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	guint16 len, type, sublen, subtyp;
	proto_item *ti;
	proto_tree *rpl_container_tree;
	guint16 offset;
	gint ett_type;

	len = tvb_get_ntohs(tvb, 0);
	proto_tree_add_text(tree, tvb, 0, 2, "Length: %u", len);

	type = tvb_get_ntohs(tvb, 2);
	proto_tree_add_text(tree, tvb, 2, 2, "Type: %s", 
		val_to_str(type, rpl_type_vals, "Unknown Type"));
	offset = 4;

	switch (type) {
		case 1:
		case 2:
		case 4:
		case 8:
		case 0x10:
		case 0x20:
			while (len >= offset+4) {
				sublen = tvb_get_ntohs(tvb, offset);
				subtyp = tvb_get_ntohs(tvb, offset+2);
				ett_type = ett_rpl_unkn;
				if(subtyp == 0x0004) ett_type = ett_rpl_0004;
				if(subtyp == 0x0008) ett_type = ett_rpl_0008;
				if(subtyp == 0x4003) ett_type = ett_rpl_4003;
				if(subtyp == 0x4006) ett_type = ett_rpl_4006;
				if(subtyp == 0x4007) ett_type = ett_rpl_4007;
				if(subtyp == 0x4009) ett_type = ett_rpl_4009;
				if(subtyp == 0x400a) ett_type = ett_rpl_400a;
				if(subtyp == 0x400b) ett_type = ett_rpl_400b;
				if(subtyp == 0x400c) ett_type = ett_rpl_400c;
				if(subtyp == 0x4011) ett_type = ett_rpl_4011;
				if(subtyp == 0x4018) ett_type = ett_rpl_4018;
				if(subtyp == 0xc005) ett_type = ett_rpl_c005;
				if(subtyp == 0xc014) ett_type = ett_rpl_c014;
				ti = proto_tree_add_text(tree, tvb,
					offset, sublen, val_to_str(subtyp, 
					rpl_type_vals, "Unknown Type"));
				rpl_container_tree = proto_item_add_subtree(ti, 
					ett_type);
				dissect_rpl_container(tvb_new_subset(tvb, 
					offset, sublen, -1), pinfo, 
					rpl_container_tree);

				offset += sublen;
			}	
			break;

		case 0x4003:
			proto_tree_add_item(tree, hf_rpl_corrval, 
				tvb, offset, 4, FALSE);
			offset += 4;
			break;

		case 0x4006:
			proto_tree_add_item(tree, hf_rpl_lmac, 
				tvb, offset, 6, FALSE);
			offset += 6;
			break;

		case 0x4007:
			proto_tree_add_item(tree, hf_rpl_sap, 
				tvb, offset, 1, FALSE);
			offset ++;
			break;

		case 0x4009:
			proto_tree_add_item(tree, hf_rpl_maxframe, 
				tvb, offset, 2, FALSE);
			offset += 2;
			break;

		case 0x400a:
			proto_tree_add_item(tree, hf_rpl_connclass, 
				tvb, offset, 2, FALSE);
			offset += 2;
			break;

		case 0x400b:
			proto_tree_add_item(tree, hf_rpl_respval, 
				tvb, offset, 1, FALSE);
			offset ++;
			break;

		case 0x400c:
			proto_tree_add_item(tree, hf_rpl_smac, 
				tvb, offset, 6, FALSE);
			offset += 6;
			break;

		case 0x4011:
			proto_tree_add_item(tree, hf_rpl_sequence, 
				tvb, offset, 4, FALSE);
			offset += 4;
			break;

		case 0x4018:
			proto_tree_add_item(tree, hf_rpl_data, 
				tvb, offset, len-4, FALSE);
			offset += len - 4;
			break;

		case 0xc005:
			proto_tree_add_item(tree, hf_rpl_config, 
				tvb, offset, 8, FALSE);
			offset += 8;
			proto_tree_add_item(tree, hf_rpl_equipment, 
				tvb, offset, 2, FALSE);
			offset += 2;
			proto_tree_add_item(tree, hf_rpl_memsize, 
				tvb, offset, 2, FALSE);
			offset += 2;
			proto_tree_add_item(tree, hf_rpl_bsmversion, 
				tvb, offset, 2, FALSE);
			offset += 2;
			proto_tree_add_item(tree, hf_rpl_ec, 
				tvb, offset, 6, FALSE);
			offset += 6;
			proto_tree_add_item(tree, hf_rpl_adapterid, 
				tvb, offset, 2, FALSE);
			offset += 2;
			proto_tree_add_item(tree, hf_rpl_shortname, 
				tvb, offset, 10, FALSE);
			offset += 10;
			break;

		case 0xc014:
			proto_tree_add_item(tree, hf_rpl_laddress, 
				tvb, offset, 4, FALSE);
			offset += 4;
			proto_tree_add_item(tree, hf_rpl_xaddress, 
				tvb, offset, 4, FALSE);
			offset += 4;
			proto_tree_add_item(tree, hf_rpl_flags, 
				tvb, offset, 1, FALSE);
			offset ++;
			break;

		default:
			call_dissector(data_handle, 
				tvb_new_subset(tvb, 4, -1, -1), pinfo,
				tree);
			break;
	}
	if (tvb_reported_length(tvb) > offset)
		call_dissector(data_handle, 
			tvb_new_subset(tvb, offset, -1, -1), pinfo, tree);
}

static void
dissect_rpl(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	guint16 rpl_len, rpl_type;
	proto_item *ti;
	proto_tree *rpl_tree;

	if (check_col(pinfo->cinfo, COL_PROTOCOL))
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "RPL");

	rpl_len  = tvb_get_ntohs(tvb, 0);
	rpl_type = tvb_get_ntohs(tvb, 2);

	if (check_col(pinfo->cinfo, COL_INFO)) {
		col_set_str(pinfo->cinfo, COL_INFO,
		val_to_str(rpl_type, rpl_type_vals, "Unknown Type"));
	}
	if (tree) {
		proto_tree_add_uint_hidden(tree, hf_rpl_type, tvb, 2, 2,
			rpl_type);
		ti = proto_tree_add_item(tree, proto_rpl, tvb, 0, 
			rpl_len, FALSE);
		rpl_tree = proto_item_add_subtree(ti, ett_rpl);
		dissect_rpl_container(tvb_new_subset(tvb, 0, rpl_len, -1),
			pinfo, rpl_tree);
	
		if (tvb_reported_length(tvb) > rpl_len)
			call_dissector(data_handle, 
				tvb_new_subset(tvb, rpl_len, -1, -1), pinfo,
				tree);
	}
}

void
proto_register_rpl(void)
{
	static hf_register_info hf[] = {
		{ &hf_rpl_type,
			{ "Type", "rpl.type", 
				FT_UINT16, BASE_DEC, NULL, 0x0,
				"RPL Packet Type", HFILL }},
		{ &hf_rpl_corrval,
			{ "Correlator Value", "rpl.corrval", 
				FT_UINT32, BASE_HEX, NULL, 0x0,
				"RPL Correlator Value", HFILL }},
		{ &hf_rpl_respval,
			{ "Response Code", "rpl.respval", 
				FT_UINT8, BASE_DEC, NULL, 0x0,
				"RPL Response Code", HFILL }},
		{ &hf_rpl_maxframe,
			{ "Maximum Frame Size", "rpl.maxframe", 
				FT_UINT16, BASE_DEC, NULL, 0x0,
				"RPL Maximum Frame Size", HFILL }},
		{ &hf_rpl_connclass,
			{ "Connection Class", "rpl.connclass", 
				FT_UINT16, BASE_HEX, NULL, 0x0,
				"RPL Connection Class", HFILL }},
		{ &hf_rpl_lmac,
			{ "Loader MAC Address", "rpl.lmac", 
				FT_ETHER, BASE_NONE, NULL, 0x0,
				"RPL Loader MAC Address", HFILL }},
		{ &hf_rpl_smac,
			{ "Set MAC Address", "rpl.smac", 
				FT_ETHER, BASE_NONE, NULL, 0x0,
				"RPL Set MAC Address", HFILL }},
		{ &hf_rpl_sap,
			{ "SAP", "rpl.sap", 
				FT_UINT8, BASE_HEX, VALS(sap_vals), 0x0,
				"RPL SAP", HFILL }},
		{ &hf_rpl_equipment,
			{ "Equipment", "rpl.equipment", 
				FT_UINT16, BASE_HEX, NULL, 0x0,
				"RPL Equipment - AX from INT 11h", HFILL }},
		{ &hf_rpl_memsize,
			{ "Memory Size", "rpl.memsize", 
				FT_UINT16, BASE_DEC, NULL, 0x0,
				"RPL Memory Size - AX from INT 12h MINUS 32k"
				" MINUS the Boot ROM Size", HFILL }},
		{ &hf_rpl_bsmversion,
			{ "BSM Version", "rpl.bsmversion", 
				FT_UINT16, BASE_HEX, NULL, 0x0,
				"RPL Version of BSM.obj", HFILL }},
		{ &hf_rpl_adapterid,
			{ "Adapter ID", "rpl.adapterid", 
				FT_UINT16, BASE_HEX, NULL, 0x0,
				"RPL Adapter ID", HFILL }},
		{ &hf_rpl_shortname,
			{ "Short Name", "rpl.shortname", 
				FT_BYTES, BASE_HEX, NULL, 0x0,
				"RPL BSM Short Name", HFILL }},
		{ &hf_rpl_laddress,
			{ "Locate Address", "rpl.laddress", 
				FT_UINT32, BASE_HEX, NULL, 0x0,
				"RPL Locate Address", HFILL }},
		{ &hf_rpl_xaddress,
			{ "XFER Address", "rpl.xaddress", 
				FT_UINT32, BASE_HEX, NULL, 0x0,
				"RPL Transfer Control Address", HFILL }},
		{ &hf_rpl_sequence,
			{ "Sequence Number", "rpl.sequence", 
				FT_UINT32, BASE_HEX, NULL, 0x0,
				"RPL Sequence Number", HFILL }},
		{ &hf_rpl_config,
			{ "Configuration", "rpl.config", 
				FT_BYTES, BASE_HEX, NULL, 0x0,
				 "RPL Configuration", HFILL }},
		{ &hf_rpl_flags,
			{ "Flags", "rpl.flags", 
				FT_UINT8, BASE_HEX, NULL, 0x0,
				"RPL Bit Significant Option Flags", HFILL }},
		{ &hf_rpl_data,
			{ "Data", "rpl.data", 
				FT_BYTES, BASE_HEX, NULL, 0x0,
				"RPL Binary File Data", HFILL }},
		{ &hf_rpl_ec,
			{ "EC", "rpl.ec",
				FT_BYTES, BASE_HEX, NULL, 0x0,
				"RPL EC", HFILL }},
	};

	static gint *ett[] = {
		&ett_rpl,
		&ett_rpl_0004,
		&ett_rpl_0008,
		&ett_rpl_4003,
		&ett_rpl_4006,
		&ett_rpl_4007,
		&ett_rpl_4009,
		&ett_rpl_400a,
		&ett_rpl_400b,
		&ett_rpl_400c,
		&ett_rpl_4011,
		&ett_rpl_4018,
		&ett_rpl_c005,
		&ett_rpl_c014,
		&ett_rpl_unkn
	};

        proto_rpl = proto_register_protocol("Remote Program Load",
	    "RPL", "rpl");
	proto_register_field_array(proto_rpl, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));
	register_dissector("rpl", dissect_rpl, proto_rpl);
}

void
proto_reg_handoff_rpl(void)
{
	dissector_handle_t rpl_handle;

	data_handle = find_dissector("data");
	rpl_handle = find_dissector("rpl");
	dissector_add("llc.dsap", SAP_RPL2, rpl_handle);
}
