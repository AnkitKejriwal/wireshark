/* packet-laplink.c
 * Routines for laplink dissection
 * Copyright 2003, Brad Hards <bradh@frogmouth.net>
 *
 * $Id: packet-laplink.c,v 1.1 2003/07/24 20:22:50 guy Exp $
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#ifdef NEED_SNPRINTF_H
# include "snprintf.h"
#endif

#include <epan/packet.h>

#define TCP_PORT_LAPLINK 1547
#define UDP_PORT_LAPLINK 1547

/* Initialize the protocol and registered fields */
static int proto_laplink = -1;
static int hf_laplink_udp_ident = -1;
static int hf_laplink_udp_name = -1;
static int hf_laplink_tcp_ident = -1;
static int hf_laplink_tcp_length = -1;
static int hf_laplink_tcp_data = -1;

/* Initialize the subtree pointers */
static gint ett_laplink = -1;

static const value_string laplink_udp_magic[] = {
	{ 0x0f010000, "Name Solicitation" },
	{ 0xf0000200, "Name Reply" },
	{ 0, NULL }
};

static const value_string laplink_tcp_magic[] = {
	{ 0x00000000, "Null bytes" },
	{ 0xff08c000, "Unknown TCP query - connection?" },
	{ 0xff08c200, "Unknown TCP query - connection?" },
	{ 0xff0bc000, "Unknown TCP query - connection?" },
	{ 0xff0bc200, "Unknown TCP query - connection?" },
	{ 0xff10c000, "Unknown TCP response - connection?" },
	{ 0xff10c200, "Unknown TCP response - connection?" },
	{ 0xff11c000, "Unknown TCP query/response - directory list or file transfer?" },
	{ 0xff11c200, "Unknown TCP query - directory list or file request?" },
	{ 0xff13c000, "Unknown TCP response - connection?" },
	{ 0xff13c200, "Unknown TCP response - connection?" },
	{ 0xff14c000, "Unknown TCP response - directory list or file transfer?" },
	{ 0, NULL }
};

/* Code to actually dissect the packets - UDP */
static void
dissect_laplink_udp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	int offset = 0;
	proto_item *ti;
	proto_tree *laplink_tree;
	guint32 udp_ident;

/* Make entries in Protocol column and Info column on summary display */
	if (check_col(pinfo->cinfo, COL_PROTOCOL)) 
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "Laplink");

	udp_ident = tvb_get_ntohl(tvb, offset);
	if (check_col(pinfo->cinfo, COL_INFO)) {
		col_add_str(pinfo->cinfo, COL_INFO,
			    val_to_str(udp_ident, laplink_udp_magic, "Unknown UDP (%u)"));
	}
    
	if (tree){
		ti = proto_tree_add_item(tree, proto_laplink, tvb, 0, -1, FALSE);
		laplink_tree = proto_item_add_subtree(ti, ett_laplink);

		proto_tree_add_item(laplink_tree, hf_laplink_udp_ident, tvb, offset, 4, FALSE);
		offset =+ 4;

		proto_tree_add_item(laplink_tree, hf_laplink_udp_name, tvb, offset, -1, FALSE);
	}
}

/* Code to actually dissect the packets - TCP aspects*/
static void
dissect_laplink_tcp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	int offset = 0;
	int length = 0;
	proto_item *ti;
	proto_tree *laplink_tree;
	guint32 tcp_ident;

/* Make entries in Protocol column and Info column on summary display */
	if (check_col(pinfo->cinfo, COL_PROTOCOL)) 
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "Laplink");

	tcp_ident = tvb_get_ntohl(tvb, offset);
	if (check_col(pinfo->cinfo, COL_INFO)) {
		col_add_str(pinfo->cinfo, COL_INFO,
			    val_to_str(tcp_ident, laplink_tcp_magic, "TCP TBA (%u)"));
	}
    
	if (tree){
		ti = proto_tree_add_item(tree, proto_laplink, tvb, 0, -1, FALSE);


		laplink_tree = proto_item_add_subtree(ti, ett_laplink);

		proto_tree_add_item(laplink_tree, hf_laplink_tcp_ident, tvb, offset, 4, FALSE);
		offset += 4;

		length = tvb_get_ntohs(tvb, offset);
		proto_tree_add_item(laplink_tree, hf_laplink_tcp_length, tvb, offset, 2, FALSE);
		offset += 2;

		proto_tree_add_item(laplink_tree, hf_laplink_tcp_data, tvb, offset, -1, FALSE);

/* Continue adding tree items to process the packet here */

	}



/* If this protocol has a sub-dissector call it here, see section 1.8 */
}


/* Register the protocol with Ethereal */

void
proto_register_laplink(void)
{                 

/* Setup list of header fields  See Section 1.6.1 for details*/
	static hf_register_info hf[] = {
		{ &hf_laplink_udp_ident,
			{ "UDP Ident", "laplink.udp_ident",
			FT_UINT32, BASE_HEX, VALS(laplink_udp_magic), 0x0,          
			"Unknown magic", HFILL }
		},
		{ &hf_laplink_udp_name,
			{ "UDP Name", "laplink.udp_name",
			FT_STRINGZ, BASE_NONE, NULL, 0x0,          
			"Machine name", HFILL }
		},
		{ &hf_laplink_tcp_ident,
			{ "TCP Ident", "laplink.tcp_ident",
			FT_UINT32, BASE_HEX, VALS(laplink_tcp_magic), 0x0,          
			"Unknown magic", HFILL }
		},
		{ &hf_laplink_tcp_length,
			{ "TCP Data payload length", "laplink.tcp_length",
			FT_UINT16, BASE_DEC, NULL, 0x0,          
			"Length of remaining payload", HFILL }
		},
		{ &hf_laplink_tcp_data,
			{ "Unknown TCP data", "laplink.tcp_data",
			FT_BYTES, BASE_HEX, NULL, 0x0,          
			"TCP data", HFILL }
		},
	};

/* Setup protocol subtree array */
	static gint *ett[] = {
		&ett_laplink,
	};

/* Register the protocol name and description */
	proto_laplink = proto_register_protocol("Laplink",
	    "Laplink", "laplink");

/* Required function calls to register the header fields and subtrees used */
	proto_register_field_array(proto_laplink, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));
}


/* If this dissector uses sub-dissector registration add a registration routine.
   This format is required because a script is used to find these routines and
   create the code that calls these routines.
*/
void
proto_reg_handoff_laplink(void)
{
	dissector_handle_t laplink_udp_handle;
	dissector_handle_t laplink_tcp_handle;

	laplink_tcp_handle = create_dissector_handle(dissect_laplink_tcp,
	    proto_laplink);
	dissector_add("tcp.port", TCP_PORT_LAPLINK, laplink_tcp_handle);

	laplink_udp_handle = create_dissector_handle(dissect_laplink_udp,
	    proto_laplink);
	dissector_add("udp.port", UDP_PORT_LAPLINK, laplink_udp_handle);
}

