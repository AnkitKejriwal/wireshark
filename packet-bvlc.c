/* packet-bvlc.c
 * Routines for BACnet/IP (BVLL, BVLC) dissection
 * Copyright 2001, Hartmut Mueller <hartmut@abmlinux.org>, FH Dortmund
 *
 * $Id: packet-bvlc.c,v 1.7 2001/12/08 06:41:41 guy Exp $
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@ethereal.com>
 * Copyright 1998 Gerald Combs
 *
 * Copied from README.developer,v 1.23
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
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "prefs.h"
#include "strutil.h"

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif

#include <glib.h>

#ifdef NEED_SNPRINTF_H
# include "snprintf.h"
#endif

#include "packet.h"

static int proto_bvlc = -1;
static int hf_bvlc_type = -1;
static int hf_bvlc_function = -1;
static int hf_bvlc_length = -1;
static int hf_bvlc_result = -1;
static int hf_bvlc_bdt_ip = -1;
static int hf_bvlc_bdt_mask = -1;
static int hf_bvlc_bdt_port = -1;
static int hf_bvlc_reg_ttl = -1;
static int hf_bvlc_fdt_ip = -1;
static int hf_bvlc_fdt_port = -1;
static int hf_bvlc_fdt_ttl = -1;
static int hf_bvlc_fdt_timeout = -1;
static int hf_bvlc_fwd_ip = -1;
static int hf_bvlc_fwd_port = -1;

static dissector_handle_t data_handle;

static dissector_table_t bvlc_dissector_table;

static const char*
bvlc_function_name (guint8 bvlc_function){
  static const char *type_names[] = {
	"BVLC-Result",
	"Write-Broadcast-Distribution-Table",
	"Read-Broadcast-Distribution-Table",
	"Read-Broadcast-Distribution-Table-Ack",
	"Forwarded-NPDU",
	"Register-Foreign-Device",
	"Read-Foreign-Device-Table",
	"Read-Foreign-Device-Table-Ack",
	"Delete-Foreign-Device-Table-Entry",
	"Distribute-Broadcast-To-Network",
	"Original-Unicast-NPDU",
	"Original-Broadcast-NPDU"
  };
  return (bvlc_function > 0xb)? "unknown" : type_names[bvlc_function];
}

static const char*
bvlc_result_name (guint16 bvlc_result){
  static const char *result_names[] = {
	"Successful completion",
	"Write-Broadcast-Distribution-Table NAK",
	"Read-Broadcast-Distribution-Table NAK",
	"Register-Foreign-Device NAK",
	"Read-Foreign-Device-Table NAK",
	"Delete-Foreign-Device-Table-Entry NAK",
	"Distribute-Broadcast-To-Network NAK"
  };
  return (bvlc_result > 0x0060)? "unknown" : result_names[bvlc_result];
}

static gint ett_bvlc = -1;
static gint ett_bdt = -1;
static gint ett_fdt = -1;

static void
dissect_bvlc(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{

	proto_item *ti;
	proto_item *ti_bdt;
	proto_item *ti_fdt;
	proto_tree *bvlc_tree;
	proto_tree *bdt_tree; /* Broadcast Distribution Table */
	proto_tree *fdt_tree; /* Foreign Device Table */
	
	gint offset;
	guint8 bvlc_type;
	guint8 bvlc_function;
	guint16 bvlc_length;
	guint16 packet_length;
	guint8 npdu_length;
	guint16 bvlc_result;
	tvbuff_t *next_tvb;

	if (check_col(pinfo->fd, COL_PROTOCOL))
		col_set_str(pinfo->fd, COL_PROTOCOL, "BVLC");

	if (check_col(pinfo->fd, COL_INFO))
		col_set_str(pinfo->fd, COL_INFO, "BACnet Virtual Link Control");

	offset = 0;

	bvlc_type =  tvb_get_guint8(tvb, offset);
	bvlc_function = tvb_get_guint8(tvb, offset+1);
	packet_length = tvb_get_ntohs(tvb, offset+2);
	if (bvlc_function > 0x08) {
		/*  We have a constant header length of BVLC of 4 in every
		 *  BVLC-packet forewarding an NPDU. Beware: Changes in the 
		 *  BACnet-IP-standard may break this. 
		 *  At the moment, no functions above 0x0b
		 *  exist (Addendum 135a to ANSI/ASHRAE 135-1995 - BACnet)
		 */
		bvlc_length = 4;
	} else if(bvlc_function == 0x04) {
		/* 4 Bytes + 6 Bytes for B/IP Address of Originating Device */
		bvlc_length = 10; 
	} else {
		/*  BVLC-packets with function below 0x09 contain 
		 *  routing-level data (e.g. Broadcast Distribution)
		 *  but no NPDU for BACnet, so bvlc_length goes up to the end
		 *  of the captured frame.
		 */
		bvlc_length = packet_length;
	}
	
	if (tree) {
		ti = proto_tree_add_item(tree, proto_bvlc, tvb, 0, 
			bvlc_length, FALSE);
		bvlc_tree = proto_item_add_subtree(ti, ett_bvlc);
		proto_tree_add_uint_format(bvlc_tree, hf_bvlc_type, tvb, offset, 1, 
			bvlc_type,"Type: 0x%x (Version %s)",bvlc_type,
			(bvlc_type == 0x81)?"BACnet/IP (Annex J)":"unknown");
		offset ++;
		proto_tree_add_uint_format(bvlc_tree, hf_bvlc_function, tvb, 
			offset, 1, bvlc_function,"Function: 0x%02x (%s)", 
			bvlc_function, bvlc_function_name(bvlc_function));
		offset ++;
		proto_tree_add_uint_format(bvlc_tree, hf_bvlc_length, tvb, offset, 
			2, bvlc_length, "BVLC-Length: %d of %d bytes BACnet packet length", 
			bvlc_length, packet_length);
		offset += 2;
		switch (bvlc_function) {
		case 0x00: /* BVLC-Result */
			bvlc_result = tvb_get_ntohs(tvb, offset);
			/* I dont know why the result code is encoded in 4 nibbles,
			 * but only using one: 0x00r0. Shifting left 4 bits.
			 */
			/* We should bitmask the result correctly when we have a
		 	* packet to dissect, see README.developer, 1.6.2, FID */
			proto_tree_add_uint_format(bvlc_tree, hf_bvlc_result, tvb, 
				offset, 2, bvlc_result,"Result: 0x%04x (%s)", 
				bvlc_result, bvlc_result_name(bvlc_result << 4));
			offset += 2;
			break;
		case 0x01: /* Write-Broadcast-Distribution-Table */
		case 0x03: /* Read-Broadcast-Distribution-Table-Ack */
			/* List of BDT Entries:	N*10-octet */
			ti_bdt = proto_tree_add_item(bvlc_tree, proto_bvlc, tvb,
				offset, bvlc_length-4, FALSE);
			bdt_tree = proto_item_add_subtree(ti_bdt, ett_bdt);
			/* List of BDT Entries:	N*10-octet */
			while ((bvlc_length - offset) > 9) {
				proto_tree_add_item(bdt_tree, hf_bvlc_bdt_ip,
					tvb, offset, 4, FALSE);
				offset += 4;
				proto_tree_add_item(bdt_tree, hf_bvlc_bdt_port,
					tvb, offset, 2, FALSE);
				offset += 2;
				proto_tree_add_item(bdt_tree, 
					hf_bvlc_bdt_mask, tvb, offset, 4,
					FALSE);
				offset += 4;
			} 
			/* We check this if we get a BDT-packet somewhere */
			break;
		case 0x02: /* Read-Broadcast-Distribution-Table */
			/* nothing to do here */
			break;
		case 0x05: /* Register-Foreign-Device */
			/* Time-to-Live	2-octets T, Time-to-Live T, in seconds */
			proto_tree_add_item(bvlc_tree, hf_bvlc_reg_ttl,
				tvb, offset, 2, FALSE);
			offset += 2;
			break;
		case 0x06: /* Read-Foreign-Device-Table */
			/* nothing to do here */
			break;
		case 0x07: /* Read-Foreign-Device-Table-Ack */
			/* List of FDT Entries:	N*10-octet */
			/* N indicates the number of entries in the FDT whose 
			 * contents are being returned. Each returned entry 
			 * consists of the 6-octet B/IP address of the registrant; 
			 * the 2-octet Time-to-Live value supplied at the time of
			 * registration; and a 2-octet value representing the 
			 * number of seconds remaining before the BBMD will purge 
			 * the registrant's FDT entry if no re-registration occurs.
			 */
			ti_fdt = proto_tree_add_item(bvlc_tree, proto_bvlc, tvb,
				offset, bvlc_length -4, FALSE);
			fdt_tree = proto_item_add_subtree(ti_fdt, ett_fdt);
			/* List of FDT Entries:	N*10-octet */
			while ((bvlc_length - offset) > 9) {
				proto_tree_add_item(fdt_tree, hf_bvlc_fdt_ip,
					tvb, offset, 4, FALSE);
				offset += 4;
				proto_tree_add_item(fdt_tree, hf_bvlc_fdt_port,
					tvb, offset, 2, FALSE);
				offset += 2;
				proto_tree_add_item(fdt_tree, 
					hf_bvlc_fdt_ttl, tvb, offset, 2,
					FALSE);
				offset += 2;
				proto_tree_add_item(fdt_tree, 
					hf_bvlc_fdt_timeout, tvb, offset, 2,
					FALSE);
				offset += 2;
			} 
			/* We check this if we get a FDT-packet somewhere */
			break;
		case 0x08: /* Delete-Foreign-Device-Table-Entry */
			/* FDT Entry:	6-octets */
			proto_tree_add_item(bvlc_tree, hf_bvlc_fdt_ip,
				tvb, offset, 4, FALSE);
			offset += 4;
			proto_tree_add_item(bvlc_tree, hf_bvlc_fdt_port,
				tvb, offset, 2, FALSE);
			offset += 2;
			break;
			/* We check this if we get a FDT-packet somewhere */
		case 0x04:	/* Forwarded-NPDU
				 * Why is this 0x04? It would have been a better
				 * idea to append all forewarded NPDUs at the
				 * end of the function table in the B/IP-standard!
				 */
			/* proto_tree_add_bytes_format(); */
			proto_tree_add_item(bvlc_tree, hf_bvlc_fwd_ip,
				tvb, offset, 4, FALSE);
			offset += 4;
			proto_tree_add_item(bvlc_tree, hf_bvlc_fwd_port,
				tvb, offset, 2, FALSE);
			offset += 2;
		default:/* Distribute-Broadcast-To-Network
			 * Original-Unicast-NPDU
			 * Original-Broadcast-NPDU
			 * Going to the next dissector...
			 */
			break;
		}

	}
/* Ok, no routing information BVLC packet. Dissect as
 * BACnet NPDU
 */
	npdu_length = packet_length - bvlc_length;
	next_tvb = tvb_new_subset(tvb,bvlc_length,-1,npdu_length);
	/* Code from Guy Harris */
	if (!dissector_try_port(bvlc_dissector_table, 
	bvlc_function, next_tvb, pinfo, tree)) {
		/* Unknown function - dissect the paylod as data */
		call_dissector(data_handle,next_tvb, pinfo, tree);
	}
}

void
proto_register_bvlc(void)
{
	static hf_register_info hf[] = {
		{ &hf_bvlc_type,
			{ "Type",           "bvlc.type",
			FT_UINT8, BASE_HEX, NULL, 0,
			"Type", HFILL }
		},
		{ &hf_bvlc_function,
			{ "Function",           "bvlc.function",
			FT_UINT8, BASE_HEX, NULL, 0,
			"BLVC Function", HFILL }
		},
		{ &hf_bvlc_length,
			{ "Length",           "bvlc.length",
			FT_UINT16, BASE_DEC, NULL, 0,
			"Length of BVLC", HFILL }
		},
		/* We should bitmask the result correctly when we have a
		 * packet to dissect */
		{ &hf_bvlc_result,
			{ "Result",           "bvlc.result",
			FT_UINT16, BASE_HEX, NULL, 0xffff,
			"Result Code", HFILL }
		},
		{ &hf_bvlc_bdt_ip,
			{ "IP",           "bvlc.bdt_ip",
			FT_IPv4, BASE_NONE, NULL, 0,
			"BDT IP", HFILL }
		},
		{ &hf_bvlc_bdt_port,
			{ "Port",           "bvlc.bdt_port",
			FT_UINT16, BASE_DEC, NULL, 0,
			"BDT Port", HFILL }
		},
		{ &hf_bvlc_bdt_mask,
			{ "Mask",           "bvlc.bdt_mask",
			FT_BYTES, BASE_HEX, NULL, 0,
			"BDT Broadcast Distribution Mask", HFILL }
		},
		{ &hf_bvlc_reg_ttl,
			{ "TTL",           "bvlc.reg_ttl",
			FT_UINT16, BASE_DEC, NULL, 0,
			"Foreign Device Time To Live", HFILL }
		},
		{ &hf_bvlc_fdt_ip,
			{ "IP",           "bvlc.fdt_ip",
			FT_IPv4, BASE_NONE, NULL, 0,
			"FDT IP", HFILL }
		},
		{ &hf_bvlc_fdt_port,
			{ "Port",           "bvlc.fdt_port",
			FT_UINT16, BASE_DEC, NULL, 0,
			"FDT Port", HFILL }
		},
		{ &hf_bvlc_fdt_ttl,
			{ "TTL",           "bvlc.fdt_ttl",
			FT_UINT16, BASE_DEC, NULL, 0,
			"Foreign Device Time To Live", HFILL }
		},
		{ &hf_bvlc_fdt_timeout,
			{ "Timeout",           "bvlc.fdt_timeout",
			FT_UINT16, BASE_DEC, NULL, 0,
			"Foreign Device Timeout (seconds)", HFILL }
		},
		{ &hf_bvlc_fwd_ip,
			{ "IP",           "bvlc.fwd_ip",
			FT_IPv4, BASE_NONE, NULL, 0,
			"FWD IP", HFILL }
		},
		{ &hf_bvlc_fwd_port,
			{ "Port",           "bvlc.fwd_port",
			FT_UINT16, BASE_DEC, NULL, 0,
			"FWD Port", HFILL }
		},
	};

	static gint *ett[] = {
		&ett_bvlc,
		&ett_bdt,
		&ett_fdt,
	};

	proto_bvlc = proto_register_protocol("BACnet Virtual Link Control",
	    "BVLC", "bvlc");

	proto_register_field_array(proto_bvlc, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));

	register_dissector("bvlc", dissect_bvlc, proto_bvlc);

	bvlc_dissector_table = register_dissector_table("bvlc.function",
	    "BVLC Function", FT_UINT8, BASE_HEX);
}

void
proto_reg_handoff_bvlc(void)
{
	dissector_handle_t bvlc_handle;

	bvlc_handle = find_dissector("bvlc");
	dissector_add("udp.port", 0xBAC0, bvlc_handle);
	data_handle = find_dissector("data");
}
/* Taken from add-135a (BACnet-IP-standard paper):
 *
 * The default UDP port for both directed messages and broadcasts shall 
 * be X'BAC0' and all B/IP devices shall support it. In some cases, 
 * e.g., a situation where it is desirable for two groups of BACnet devices 
 * to coexist independently on the same IP subnet, the UDP port may be 
 * configured locally to a different value without it being considered 
 * a violation of this protocol.
 *
 * This dissector does not analyse UDP packets other than on port 0xBAC0.
 * If you changed your BACnet port locally, use the ethereal feature
 * "Decode As".
 */
