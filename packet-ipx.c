/* packet-ipx.c
 * Routines for NetWare's IPX
 * Gilbert Ramirez <gram@verdict.uthscsa.edu>
 *
 * $Id: packet-ipx.c,v 1.20 1999/05/10 20:51:36 gram Exp $
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@unicom.net>
 * Copyright 1998 Gerald Combs
 *
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

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#include <stdio.h>
#include <glib.h>
#include "packet.h"
#include "packet-ipx.h"
#include "packet-ncp.h"

/* The information in this module (IPX, SPX, NCP) comes from:
	NetWare LAN Analysis, Second Edition
	Laura A. Chappell and Dan E. Hakes
	(c) 1994 Novell, Inc.
	Novell Press, San Jose.
	ISBN: 0-7821-1362-1

  And from the ncpfs source code by Volker Lendecke

*/

static void
dissect_spx(const u_char *pd, int offset, frame_data *fd, proto_tree *tree, int max_data);

static void
dissect_ipxrip(const u_char *pd, int offset, frame_data *fd, proto_tree *tree, int
max_data);

static void
dissect_sap(const u_char *pd, int offset, frame_data *fd, proto_tree *tree, int
max_data);

struct port_info {
	guint16	port;
	void	(*func) (const u_char *, int, frame_data *, proto_tree *, int);
	char	*text;
};

struct conn_info {
	guint8	ctrl;
	char	*text;
};

struct server_info {
	guint16	type;
	char	*text;
};

/* ================================================================= */
/* IPX                                                               */
/* ================================================================= */
static struct port_info	ports[] = {
	{ 0x0451, dissect_ncp,		"NCP" },
	{ 0x0452, dissect_sap,		"SAP" },
	{ 0x0453, dissect_ipxrip, 	"RIP" },
	{ 0x0455, NULL,			"NetBIOS" },
	{ 0x0456, NULL,			"Diagnostic" },
	{ 0x0457, NULL,			"Serialization" },
	{ 0x0551, NULL,			"NWLink SMB Name Query" },
	{ 0x0553, dissect_nwlink_dg,	"NWLink SMB Datagram" },
	{ 0x055d, NULL,			"Attachmate Gateway" },
	{ 0x4001, NULL,			"IPX Message" },
	{ 0x0000, NULL,			NULL }
};

static char*
port_text(guint16 port) {
	int i=0;

	while (ports[i].text != NULL) {
		if (ports[i].port == port) {
			return ports[i].text;
		}
		i++;
	}
	return "Unknown";
}

static void*
port_func(guint16 port) {
	int i=0;

	while (ports[i].text != NULL) {
		if (ports[i].port == port) {
			return ports[i].func;
		}
		i++;
	}
	return NULL;
}

char *
ipx_packet_type(u_char val)
{
	if (val == 0) {
		return "IPX";
	}
	else if (val == 5) {
		return "SPX";
	}
	else if (val == 17) {
		return "NCP";
	}
	else if (val == 20) {
		return "NetBIOS Broadcast";
	}
	else if (val >= 16 && val <= 31) {
		return "Experimental Protocol";
	}
	else {
		return "Unknown";
	}
}

gchar*
ipxnet_to_string(const guint8 *ad)
{
	static gchar	str[3][12];
	static gchar	*cur;

	if (cur == &str[0][0]) {
		cur = &str[1][0];
	} else if (cur == &str[1][0]) {
		cur = &str[2][0];
	} else {
		cur = &str[0][0];
	}

	sprintf(cur, "%02X %02X %02X %02X", ad[0], ad[1], ad[2], ad[3]);
	return cur;
}

gchar*
ipx_addr_to_str(guint32 net, const guint8 *ad)
{
	static gchar	str[3][22];
	static gchar	*cur;

	if (cur == &str[0][0]) {
		cur = &str[1][0];
	} else if (cur == &str[1][0]) {
		cur = &str[2][0];
	} else {
		cur = &str[0][0];
	}

	sprintf(cur, "%X.%02x%02x%02x%02x%02x%02x", net, 
		ad[0], ad[1], ad[2], ad[3], ad[4], ad[5]);
	return cur;
}

void
dissect_ipx(const u_char *pd, int offset, frame_data *fd, proto_tree *tree) {

	proto_tree	*ipx_tree;
	proto_item	*ti;
	guint8		ipx_type, ipx_hops;
	guint16		ipx_checksum, ipx_length;
	guint8		*ipx_snode, *ipx_dnode, *ipx_snet, *ipx_dnet;

	gchar		*str_dnet, *str_snet;
	guint16		ipx_dsocket, ipx_ssocket;
	void		(*dissect) (const u_char *, int, frame_data *, proto_tree *, int);
	int		max_data;

	/* Calculate here for use in pinfo and in tree */
	ipx_dnet = (guint8*)&pd[offset+6];
	ipx_snet = (guint8*)&pd[offset+18];
	str_dnet = ipxnet_to_string(ipx_dnet);
	str_snet = ipxnet_to_string(ipx_snet);
	ipx_dsocket = pntohs(&pd[offset+16]);
	ipx_ssocket = pntohs(&pd[offset+28]);
	ipx_dnode = (guint8*)&pd[offset+10];
	ipx_snode = (guint8*)&pd[offset+22];
	ipx_type = pd[offset+5];
	ipx_length = pntohs(&pd[offset+2]);
	max_data = ipx_length - 30;

	if (check_col(fd, COL_RES_DL_DST))
		col_add_str(fd, COL_RES_DL_DST,
				ipx_addr_to_str(pntohl(ipx_dnet), ipx_dnode));
	if (check_col(fd, COL_RES_DL_SRC))
		col_add_str(fd, COL_RES_DL_SRC,
				ipx_addr_to_str(pntohl(ipx_snet), ipx_snode));

	if (check_col(fd, COL_PROTOCOL))
		col_add_str(fd, COL_PROTOCOL, "IPX");
	if (check_col(fd, COL_INFO))
		col_add_fstr(fd, COL_INFO, "%s (0x%04X)", port_text(ipx_dsocket),
				ipx_dsocket);

	if (tree) {
		ipx_checksum = pntohs(&pd[offset]);
		ipx_hops = pd[offset+4];

		ti = proto_tree_add_item(tree, offset, 30,
			"Internetwork Packet Exchange");
		ipx_tree = proto_tree_new();
		proto_item_add_subtree(ti, ipx_tree, ETT_IPX);
		proto_tree_add_item(ipx_tree, offset,      2, "Checksum: 0x%04x",
				ipx_checksum);
		proto_tree_add_item(ipx_tree, offset+2,    2, "Length: %d bytes",
				ipx_length);
		proto_tree_add_item(ipx_tree, offset+4,    1, "Transport Control: %d hops",
				ipx_hops);
		proto_tree_add_item(ipx_tree, offset+5,    1, "Packet Type: %s",
			ipx_packet_type(ipx_type));
		proto_tree_add_item(ipx_tree, offset+6,    4, "Destination Network: %s",
			str_dnet);
		proto_tree_add_item(ipx_tree, offset+10,   6, "Destination Node: %s",
			ether_to_str(ipx_dnode));
		proto_tree_add_item(ipx_tree, offset+16,   2,
			"Destination Socket: %s (0x%04X)", port_text(ipx_dsocket),
			ipx_dsocket);
		proto_tree_add_item(ipx_tree, offset+18,   4, "Source Network: %s",
			str_snet);
		proto_tree_add_item(ipx_tree, offset+22,   6, "Source Node: %s",
			ether_to_str(ipx_snode));
		proto_tree_add_item(ipx_tree, offset+28,   2,
			"Source Socket: %s (0x%04X)", port_text(ipx_ssocket), ipx_ssocket);
	}
	offset += 30;

	switch (ipx_type) {
		case 5: /* SPX */
			dissect_spx(pd, offset, fd, tree, max_data);
			break;

		case 17: /* NCP */
			if (pntohl(ipx_dnode) == 0 && pntohs(ipx_dnode + 4) == 1)
				nw_server_address = pntohl(ipx_dnet);
			else if (pntohl(ipx_snode) == 0 && pntohs(ipx_snode + 4) == 1)
				nw_server_address = pntohl(ipx_snet);
			else
				nw_server_address = 0;

			dissect_ncp(pd, offset, fd, tree, max_data);
			break;

		case 20: /* NetBIOS */
			if (ipx_dsocket == 0x0455) {
				dissect_nbipx_ns(pd, offset, fd, tree, max_data);
				break;
			}
			/* else fall through */

		case 0: /* IPX, fall through to default */
		default:
			dissect = port_func(ipx_dsocket);
			if (dissect) {
				dissect(pd, offset, fd, tree, max_data);
			}
			else {
				dissect = port_func(ipx_ssocket);
				if (dissect) {
					dissect(pd, offset, fd, tree, max_data);
				}
				else {
					dissect_data(pd, offset, fd, tree);
				}
			}
			break;
	}
}


/* ================================================================= */
/* SPX                                                               */
/* ================================================================= */
static char*
spx_conn_ctrl(u_char ctrl)
{
	int i=0;

	static struct conn_info	conns[] = {
		{ 0x10, "End-of-Message" },
		{ 0x20, "Attention" },
		{ 0x40, "Acknowledgment Required"},
		{ 0x80, "System Packet"}
	};

	while (conns[i].text != NULL) {
		if (conns[i].ctrl == ctrl) {
			return conns[i].text;
		}
		i++;
	}
	return "Unknown";
}

static char*
spx_datastream(u_char type)
{
	switch (type) {
		case 0xfe:
			return "End-of-Connection";
		case 0xff:
			return "End-of-Connection Acknowledgment";
		default:
			return "Client-Defined";
	}
}

static void
dissect_spx(const u_char *pd, int offset, frame_data *fd, proto_tree *tree, int
	max_data) {

	proto_tree	*spx_tree;
	proto_item	*ti;

	if (check_col(fd, COL_PROTOCOL))
		col_add_str(fd, COL_PROTOCOL, "SPX");
	if (check_col(fd, COL_INFO))
		col_add_str(fd, COL_INFO, "SPX");

	if (tree) {
		ti = proto_tree_add_item(tree, offset, 12, "Sequenced Packet Exchange");
		spx_tree = proto_tree_new();
		proto_item_add_subtree(ti, spx_tree, ETT_SPX);

		proto_tree_add_item(spx_tree, offset,      1,
			"Connection Control: %s (0x%02X)",
			spx_conn_ctrl(pd[offset]), pd[offset]);

		proto_tree_add_item(spx_tree, offset+1,     1,
			"Datastream Type: %s (0x%02X)",
			spx_datastream(pd[offset+1]), pd[offset+1]);

		proto_tree_add_item(spx_tree, offset+2,     2,
			"Source Connection ID: %d", pntohs( &pd[offset+2] ) );

		proto_tree_add_item(spx_tree, offset+4,     2,
			"Destination Connection ID: %d", pntohs( &pd[offset+4] ) );

		proto_tree_add_item(spx_tree, offset+6,     2,
			"Sequence Number: %d", pntohs( &pd[offset+6] ) );

		proto_tree_add_item(spx_tree, offset+8,     2,
			"Acknowledgment Number: %d", pntohs( &pd[offset+8] ) );

		proto_tree_add_item(spx_tree, offset+10,     2,
			"Allocation Number: %d", pntohs( &pd[offset+10] ) );

		offset += 12;
		dissect_data(pd, offset, fd, tree);
	}
}

/* ================================================================= */
/* IPX RIP                                                           */
/* ================================================================= */
static void
dissect_ipxrip(const u_char *pd, int offset, frame_data *fd, proto_tree *tree,
	int max_data) {

	proto_tree	*rip_tree;
	proto_item	*ti;
	guint16		operation;
	struct ipx_rt_def route;
	int			cursor;

	char		*rip_type[2] = { "Request", "Response" };

	operation = pntohs(&pd[offset]) - 1;

	if (check_col(fd, COL_PROTOCOL))
	 col_add_str(fd, COL_PROTOCOL, "IPX RIP");
	if (check_col(fd, COL_PROTOCOL)) {
	 if (operation < 2) {
		 col_add_str(fd, COL_INFO, rip_type[operation]);
	 }
	 else {
		 col_add_str(fd, COL_INFO, "Unknown Packet Type");
	 }
	}

	if (tree) {
		ti = proto_tree_add_item(tree, offset, END_OF_FRAME,
			"IPX Routing Information Protocol");
		rip_tree = proto_tree_new();
		proto_item_add_subtree(ti, rip_tree, ETT_IPXRIP);

		if (operation < 2) {
			proto_tree_add_item(rip_tree, offset, 2,
			"RIP packet type: %s", rip_type[operation]);
		}
		else {
			proto_tree_add_item(rip_tree, offset, 2, "Unknown RIP packet type");
		}

		for (cursor = offset + 2; cursor < fd->cap_len; cursor += 8) {
			memcpy(&route.network, &pd[cursor], 4);
			route.hops = pntohs(&pd[cursor+4]);
			route.ticks = pntohs(&pd[cursor+6]);

			if (operation == IPX_RIP_REQUEST - 1) {
				proto_tree_add_item(rip_tree, cursor,      8,
					"Route Vector: %s, %d hop%s, %d tick%s",
					ipxnet_to_string((guint8*)&route.network),
					route.hops,  route.hops  == 1 ? "" : "s",
					route.ticks, route.ticks == 1 ? "" : "s");
			}
			else {
				proto_tree_add_item(rip_tree, cursor,      8,
					"Route Vector: %s, %d hop%s, %d tick%s (%d ms)",
					ipxnet_to_string((guint8*)&route.network),
					route.hops,  route.hops  == 1 ? "" : "s",
					route.ticks, route.ticks == 1 ? "" : "s",
					route.ticks * 1000 / 18);
			}
		}
	}
}



/* ================================================================= */
/* SAP																 */
/* ================================================================= */
static char*
server_type(guint16 type)
{
	int i=0;

	/* some of these are from ncpfs, others are from the book */
	static struct server_info	servers[] = {
		{ 0x0001,	"User" },
		{ 0x0002,	"User Group" },
		{ 0x0003,	"Print Queue" },
		{ 0x0004,	"File server" },
		{ 0x0005,	"Job server" },
		{ 0x0007,	"Print server" },
		{ 0x0008,	"Archive server" },
		{ 0x0009,	"Archive server" },
		{ 0x000a,	"Job queue" },
		{ 0x000b,	"Administration" },
		{ 0x0021,	"NAS SNA gateway" },
		{ 0x0024,	"Remote bridge" },
		{ 0x0026,	"Bridge server" },
		{ 0x0027,	"TCP/IP gateway" },
		{ 0x002d,	"Time Synchronization VAP" },
		{ 0x002e,	"Archive Server Dynamic SAP" },
		{ 0x0047,	"Advertising print server" },
		{ 0x004b,	"Btrieve VAP 5.0" },
		{ 0x004c,	"SQL VAP" },
		{ 0x0050,	"Btrieve VAP" },
		{ 0x0053,	"Print Queue VAP" },
		{ 0x007a,	"TES NetWare for VMS" },
		{ 0x0098,	"NetWare access server" },
		{ 0x009a,	"Named Pipes server" },
		{ 0x009e,	"Portable NetWare Unix" },
		{ 0x0107,	"NetWare 386" },
		{ 0x0111,	"Test server" },
		{ 0x0133,	"NetWare Name Service" },
		{ 0x0166,	"NetWare management" },
		{ 0x026a,	"NetWare management" },
		{ 0x026b,	"Time synchronization" },
		{ 0x0278,	"NetWare Directory server" },
		{ 0x055d,	"Attachmate SNA gateway" },
		{ 0x0000,	NULL }
	};

	while (servers[i].text != NULL) {
		if (servers[i].type == type) {
			return servers[i].text;
		}
		i++;
	}
	return "Unknown";
}

static void
dissect_sap(const u_char *pd, int offset, frame_data *fd, proto_tree *tree,
	int max_data) {

	proto_tree	*sap_tree, *s_tree;
	proto_item	*ti;
	int			cursor;
	struct sap_query query;
	struct sap_server_ident server;

	char		*sap_type[4] = { "General Query", "General Response",
		"Nearest Query", "Nearest Response" };

	query.query_type = pntohs(&pd[offset]);
	query.server_type = pntohs(&pd[offset+2]);

	if (check_col(fd, COL_PROTOCOL))
		col_add_str(fd, COL_PROTOCOL, "SAP");
	if (check_col(fd, COL_INFO)) {
		if (query.query_type < 4) {
			col_add_str(fd, COL_INFO, sap_type[query.query_type - 1]);
		}
		else {
			col_add_str(fd, COL_INFO, "Unknown Packet Type");
		}
	}

	if (tree) {
		ti = proto_tree_add_item(tree, offset, END_OF_FRAME,
			"Service Advertising Protocol");
		sap_tree = proto_tree_new();
		proto_item_add_subtree(ti, sap_tree, ETT_IPXSAP);

		if (query.query_type < 4) {
			proto_tree_add_item(sap_tree, offset, 2, sap_type[query.query_type - 1]);
		}
		else {
			proto_tree_add_item(sap_tree, offset, 2,
					"Unknown SAP Packet Type %d", query.query_type);
		}

		if (query.query_type == IPX_SAP_GENERAL_RESPONSE ||
				query.query_type == IPX_SAP_NEAREST_RESPONSE) { /* responses */

			for (cursor = offset + 2; (cursor + 64) <= fd->cap_len; cursor += 64) {
				server.server_type = pntohs(&pd[cursor]);
				memcpy(server.server_name, &pd[cursor+2], 48);
				memcpy(&server.server_network, &pd[cursor+50], 4);
				memcpy(&server.server_node, &pd[cursor+54], 6);
				server.server_port = pntohs(&pd[cursor+60]);
				server.intermediate_network = pntohs(&pd[cursor+62]);

				ti = proto_tree_add_item(sap_tree, cursor+2, 48,
					"Server Name: %s", server.server_name);
				s_tree = proto_tree_new();
				proto_item_add_subtree(ti, s_tree, ETT_IPXSAP_SERVER);

				proto_tree_add_item(s_tree, cursor, 2, "Server Type: %s (0x%04X)",
						server_type(server.server_type), server.server_type);
				proto_tree_add_item(s_tree, cursor+50, 4, "Network: %s",
						ipxnet_to_string((guint8*)&pd[cursor+50]));
				proto_tree_add_item(s_tree, cursor+54, 6, "Node: %s",
						ether_to_str((guint8*)&pd[cursor+54]));
				proto_tree_add_item(s_tree, cursor+60, 2, "Socket: %s (0x%04X)",
						port_text(server.server_port), server.server_port);
				proto_tree_add_item(s_tree, cursor+62, 2,
						"Intermediate Networks: %d",
						server.intermediate_network);
			}
		}
		else {  /* queries */
			proto_tree_add_item(sap_tree, offset+2, 2, "Server Type: %s (0x%04X)",
					server_type(query.server_type), query.server_type);
		}
	}
}

