/* packet-dvmrp.c   2001 Ronnie Sahlberg <rsahlber@bigpond.net.au>
 * Routines for IGMP/DVMRP packet disassembly
 *
 * $Id: packet-dvmrp.c,v 1.1 2001/06/12 06:21:55 guy Exp $
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
/*


			DVMRP	DVMRP
	code		v1	v3

	0x01		*	*
	0x02		*	*
	0x03		x
	0x04		x
	0x07			x
	0x08			x
	0x09			x


	* V3 has len>=8 and byte[6]==0xff and byte[7]==0x03


	DVMRP is defined in the following RFCs
	RFC1075 Version 1
	draft-ietf-idmr-dvmrp-v3-10.txt Version 3

	V1 and V3 can be distinguished by looking at bytes 6 and 7 in the 
	IGMP/DVMRP header.
	If header[6]==0xff and header[7]==0x03 we have version 3.


	RFC1075 has typos in 3.12.2 and 3.12.4, see if you can spot them.
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#include <stdio.h>
#include <string.h>
#include <glib.h>

#include "packet.h"
#include "ipproto.h"
#include "in_cksum.h"
#include "packet-dvmrp.h"

static int proto_dvmrp = -1;
static int hf_version = -1;
static int hf_type = -1;
static int hf_code_v1 = -1;
static int hf_checksum = -1;
static int hf_checksum_bad = -1;
static int hf_commands = -1;
static int hf_command = -1;
static int hf_count = -1;
static int hf_afi = -1;
static int hf_netmask = -1;
static int hf_metric = -1;
static int hf_dest_unr = -1;
static int hf_split_horiz = -1;
static int hf_infinity = -1;
static int hf_daddr = -1;
static int hf_maddr = -1;
static int hf_hold = -1;
static int hf_code_v3 = -1;
static int hf_capabilities = -1;
static int hf_cap_leaf = -1;
static int hf_cap_prune = -1;
static int hf_cap_genid = -1;
static int hf_cap_mtrace = -1;
static int hf_cap_snmp = -1;
static int hf_cap_netmask = -1;
static int hf_min_ver = -1;
static int hf_maj_ver = -1;
static int hf_genid = -1;
static int hf_naddr = -1;
static int hf_route = -1;
static int hf_saddr = -1;
static int hf_life = -1;
static int hf_neighbor = -1;

static int ett_dvmrp = -1;
static int ett_commands = -1;
static int ett_capabilities = -1;
static int ett_route = -1;


#define DVMRP_TYPE				0x13
static const value_string dvmrp_type[] = {
	{DVMRP_TYPE,	"DVMRP"	},
	{0,		NULL}
};

#define DVMRP_V1_RESPONSE			1
#define DVMRP_V1_REQUEST			2
#define DVMRP_V1_NON_MEMBERSHIP_REPORT		3
#define DVMRP_V1_NON_MEMBERSHIP_CANCELLATION	4
static const value_string code_v1[] = {
	{DVMRP_V1_RESPONSE,			"Response"			},
	{DVMRP_V1_REQUEST,			"Request"			},
	{DVMRP_V1_NON_MEMBERSHIP_REPORT,	"Non-membership report"		},
	{DVMRP_V1_NON_MEMBERSHIP_CANCELLATION,	"Non-membership cancellation"	},	
	{0,					NULL}
};

#define DVMRP_V3_PROBE				1
#define DVMRP_V3_REPORT				2
#define DVMRP_V3_PRUNE				7
#define DVMRP_V3_GRAFT				8
#define DVMRP_V3_GRAFT_ACK			9
static const value_string code_v3[] = {
	{DVMRP_V3_PROBE,	"Probe"},
	{DVMRP_V3_REPORT,	"Report"},
	{DVMRP_V3_PRUNE,	"Prune"},
	{DVMRP_V3_GRAFT,	"Graft"},
	{DVMRP_V3_GRAFT_ACK,	"Graft ACK"},
	{0,			NULL}
};

#define DVMRP_V3_CAP_LEAF	0x01
#define DVMRP_V3_CAP_PRUNE	0x02
#define DVMRP_V3_CAP_GENID	0x04
#define DVMRP_V3_CAP_MTRACE	0x08
#define DVMRP_V3_CAP_SNMP	0x10
#define DVMRP_V3_CAP_NETMASK	0x20
	

#define V1_COMMAND_NULL		0
#define V1_COMMAND_AFI		2
#define V1_COMMAND_SUBNETMASK	3
#define V1_COMMAND_METRIC	4
#define V1_COMMAND_FLAGS0	5
#define V1_COMMAND_INFINITY	6
#define V1_COMMAND_DA		7
#define V1_COMMAND_RDA		8
#define V1_COMMAND_NMR		9
#define V1_COMMAND_NMR_CANCEL	10
static const value_string command[] = {
	{V1_COMMAND_NULL,	"NULL"	},
	{V1_COMMAND_AFI,	"Address Family Indicator"},
	{V1_COMMAND_SUBNETMASK,	"Subnetmask"},
	{V1_COMMAND_METRIC,	"Metric"},
	{V1_COMMAND_FLAGS0,	"Flags0"},
	{V1_COMMAND_INFINITY,	"Infinity"},
	{V1_COMMAND_DA,		"Destination Address"},
	{V1_COMMAND_RDA,	"Requested Destination Address"},
	{V1_COMMAND_NMR,	"Non-Membership Report"},
	{V1_COMMAND_NMR_CANCEL,	"Non-Membership Report Cancel"},
	{0,			NULL}
};

#define V1_AFI_IP		2
static const value_string afi[] = {
	{V1_AFI_IP,	"IP v4 Family"},
	{0,		NULL}
};

static const true_false_string tfs_dest_unreach = {
	"Destination Unreachable",
	"NOT Destination Unreachable"
};

static const true_false_string tfs_split_horiz = {
	"Split Horizon concealed route",
	"NOT Split Horizon concealed route"
};

static const true_false_string tfs_cap_leaf = {
	"Leaf",
	"NOT Leaf"
};
static const true_false_string tfs_cap_prune = {
	"Prune capable",
	"NOT Prune capable"
};
static const true_false_string tfs_cap_genid = {
	"Genid capable",
	"NOT Genid capable"
};
static const true_false_string tfs_cap_mtrace = {
	"Multicast Traceroute capable",
	"NOT Multicast Traceroute capable"
};
static const true_false_string tfs_cap_snmp = {
	"SNMP capable",
	"NOT SNMP capable"
};
static const true_false_string tfs_cap_netmask = {
	"Netmask capable",
	"NOT Netmask capable"
};

static void dvmrp_checksum(proto_tree *tree,tvbuff_t *tvb, int len)
{
	guint16 cksum,hdrcksum;
	vec_t cksum_vec[1];

	cksum_vec[0].ptr = tvb_get_ptr(tvb, 0, len);
	cksum_vec[0].len = len;

	hdrcksum = tvb_get_ntohs(tvb, 2);
	cksum = in_cksum(&cksum_vec[0],1);

	if (cksum==0) {
		proto_tree_add_uint_format(tree, hf_checksum, tvb, 2, 2, hdrcksum, "Header checksum: 0x%04x (correct)", hdrcksum);
	} else {
		proto_tree_add_item_hidden(tree, hf_checksum_bad, tvb, 2, 2, TRUE);
		proto_tree_add_uint_format(tree, hf_checksum, tvb, 2, 2, hdrcksum, "Header checksum: 0x%04x (incorrect, should be 0x%04x)", hdrcksum,in_cksum_shouldbe(hdrcksum,cksum));
	}

	return;
}



int
dissect_v3_report(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree, int offset)
{
	guint8 m0,m1,m2,m3;
	guint8 s0,s1,s2,s3;
	guint8 metric;
	guint32 ip;

	while (tvb_length_remaining(tvb, offset)) {
		proto_tree *tree;
		proto_item *item;
		int old_offset = offset;

		item = proto_tree_add_item(parent_tree, hf_route, 
				tvb, offset, 0, FALSE);
		tree = proto_item_add_subtree(item, ett_route);

		m0 = 0xff;
		/* read the mask */
		m1 = tvb_get_guint8(tvb, offset);
		m2 = tvb_get_guint8(tvb, offset+1);
		m3 = tvb_get_guint8(tvb, offset+2);

		ip = m3;
		ip = (ip<<8)|m2;
		ip = (ip<<8)|m1;
		ip = (ip<<8)|m0;
		proto_tree_add_ipv4(tree, hf_netmask, tvb, offset, 3, ip);

		offset += 3;

		/* read every srcnet, metric  pairs */
		do {
			int old_offset = offset;
			m0 = 0xff;

			s0 = 0;
			s1 = 0;
			s2 = 0;
			s3 = 0;

			s0 = tvb_get_guint8(tvb, offset);
			offset += 1;
			if (m1) {
				s1 = tvb_get_guint8(tvb, offset);
				offset += 1;
			}
			if (m2) {
				s2 = tvb_get_guint8(tvb, offset);
				offset += 1;
			}
			if (m3) {
				s3 = tvb_get_guint8(tvb, offset);
				offset += 1;
			}

			/* handle special case for default route V3/3.4.3 */
			if ((!m1)&&(!m2)&&(!m3)&&(!s0)) {
				m0 = 0;
			}

			ip = s3;
			ip = (ip<<8)|s2;
			ip = (ip<<8)|s1;
			ip = (ip<<8)|s0;
			proto_tree_add_ipv4_format(tree, hf_saddr, tvb, 
				old_offset, offset-old_offset, ip,
				"%s %d.%d.%d.%d (netmask %d.%d.%d.%d)",
				m0?"Source Network":"Default Route",
				s0,s1,s2,s3,m0,m1,m2,m3);
			
			metric = tvb_get_guint8(tvb, offset);
			proto_tree_add_uint(tree, hf_metric, tvb,
				offset, 1, metric&0x7f);
			offset += 1;


		} while (!(metric&0x80));

		proto_item_set_len(item, offset-old_offset);
	}

	return offset;
}

int
dissect_dvmrp_v3(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree, int offset)
{
	guint8 code,count;

	/* version */
	proto_tree_add_uint(parent_tree, hf_version, tvb, 0, 0, 3);

	/* type of command */
	proto_tree_add_uint(parent_tree, hf_type, tvb, offset, 1, 0x13);
	offset += 1;

	/* code */
	code = tvb_get_guint8(tvb, offset);
	proto_tree_add_uint(parent_tree, hf_code_v3, tvb, offset, 1, code);
	offset += 1;
	if (check_col(pinfo->fd, COL_INFO)) {
		col_add_fstr(pinfo->fd, COL_INFO,
			"V%d %s",3 ,val_to_str(code, code_v3, 
				"Unknown Type:0x%02x"));
	}

	/* checksum */
	dvmrp_checksum(parent_tree, tvb, tvb_length_remaining(tvb, 0));
	offset += 2;

	/* skip unused byte */
	offset += 1;

	/* PROBE packets have capabilities flags, unused for other packets */
	if (code==DVMRP_V3_PROBE) {
		proto_tree *tree;
		proto_item *item;

		item = proto_tree_add_item(parent_tree, hf_capabilities, 
				tvb, offset, 1, FALSE);
		tree = proto_item_add_subtree(item, ett_capabilities);

		count = tvb_get_guint8(tvb, offset);
		proto_tree_add_boolean(tree, hf_cap_netmask, tvb, offset, 1, count);
		proto_tree_add_boolean(tree, hf_cap_snmp, tvb, offset, 1, count);
		proto_tree_add_boolean(tree, hf_cap_mtrace, tvb, offset, 1, count);
		proto_tree_add_boolean(tree, hf_cap_genid, tvb, offset, 1, count);
		proto_tree_add_boolean(tree, hf_cap_prune, tvb, offset, 1, count);
		proto_tree_add_boolean(tree, hf_cap_leaf, tvb, offset, 1, count);
	}
	offset += 1;

	/* minor version */
	proto_tree_add_uint(parent_tree, hf_min_ver, tvb, offset, 1, tvb_get_guint8(tvb, offset));
	offset += 1;

	/* major version */
	proto_tree_add_uint(parent_tree, hf_maj_ver, tvb, offset, 1, tvb_get_guint8(tvb, offset));
	offset += 1;

	switch (code) {
	case DVMRP_V3_PROBE:
		/* generation id */
		proto_tree_add_uint(parent_tree, hf_genid, tvb, 
			offset, 4, tvb_get_ntohl(tvb, offset));
		offset += 4;
		while (tvb_length_remaining(tvb, offset)) {
			proto_tree_add_ipv4(parent_tree, hf_neighbor, 
				tvb, offset, 4, 
				tvb_get_letohl(tvb, offset));
			offset += 4;
		}
		break;
	case DVMRP_V3_REPORT:
		offset = dissect_v3_report(tvb, pinfo, parent_tree, offset);
		break;
	case DVMRP_V3_PRUNE:
		/* source address */
		proto_tree_add_ipv4(parent_tree, hf_saddr, 
			tvb, offset, 4, 
			tvb_get_letohl(tvb, offset));
		offset += 4;
		/* group address */
		proto_tree_add_ipv4(parent_tree, hf_maddr, 
			tvb, offset, 4, 
			tvb_get_letohl(tvb, offset));
		offset += 4;
		/* prune lifetime */
		proto_tree_add_uint(parent_tree, hf_life, 
			tvb, offset, 4, 
			tvb_get_ntohl(tvb, offset));
		offset += 4;
		/* source netmask */
		if (tvb_length_remaining(tvb, offset)>=4) {
			proto_tree_add_ipv4(parent_tree, hf_netmask, 
				tvb, offset, 4, 
				tvb_get_letohl(tvb, offset));
			offset += 4;
		}
		break;
	case DVMRP_V3_GRAFT:
		/* source address */
		proto_tree_add_ipv4(parent_tree, hf_saddr, 
			tvb, offset, 4, 
			tvb_get_letohl(tvb, offset));
		offset += 4;
		/* group address */
		proto_tree_add_ipv4(parent_tree, hf_maddr, 
			tvb, offset, 4, 
			tvb_get_letohl(tvb, offset));
		offset += 4;
		/* source netmask */
		if (tvb_length_remaining(tvb, offset)>=4) {
			proto_tree_add_ipv4(parent_tree, hf_netmask, 
				tvb, offset, 4, 
				tvb_get_letohl(tvb, offset));
			offset += 4;
		}
		break;
	case DVMRP_V3_GRAFT_ACK:
		/* source address */
		proto_tree_add_ipv4(parent_tree, hf_saddr, 
			tvb, offset, 4, 
			tvb_get_letohl(tvb, offset));
		offset += 4;
		/* group address */
		proto_tree_add_ipv4(parent_tree, hf_maddr, 
			tvb, offset, 4, 
			tvb_get_letohl(tvb, offset));
		offset += 4;
		/* source netmask */
		if (tvb_length_remaining(tvb, offset)>=4) {
			proto_tree_add_ipv4(parent_tree, hf_netmask, 
				tvb, offset, 4, 
				tvb_get_letohl(tvb, offset));
			offset += 4;
		}
		break;
	}

	return offset;
}
	

int
dissect_dvmrp_v1(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree, int offset)
{
	guint8 code;
	guint8 af=2; /* default */

	/* version */
	proto_tree_add_uint(parent_tree, hf_version, tvb, 0, 0, 1);

	/* type of command */
	proto_tree_add_uint(parent_tree, hf_type, tvb, offset, 1, 0x13);
	offset += 1;

	/* code */
	code = tvb_get_guint8(tvb, offset);
	proto_tree_add_uint(parent_tree, hf_code_v1, tvb, offset, 1, code);
	offset += 1;
	if (check_col(pinfo->fd, COL_INFO)) {
		col_add_fstr(pinfo->fd, COL_INFO,
			"V%d %s",1 ,val_to_str(code, code_v1, 
				"Unknown Type:0x%02x"));
	}

	/* checksum */
	dvmrp_checksum(parent_tree, tvb, tvb_length_remaining(tvb, 0));
	offset += 2;

	/* decode all the v1 commands */
	while (tvb_length_remaining(tvb, offset)) {
		proto_tree *tree;
		proto_item *item;
		guint8 cmd,count;
		int old_offset = offset;

		item = proto_tree_add_item(parent_tree, hf_commands, 
				tvb, offset, 0, FALSE);
		tree = proto_item_add_subtree(item, ett_commands);

		cmd = tvb_get_guint8(tvb, offset);
		proto_tree_add_uint(tree, hf_command, tvb,
			offset, 1, cmd);
		offset += 1;

		switch (cmd){
		case V1_COMMAND_NULL:
			offset += 1; /* skip ignored/pad byte*/
			if (item) {
				proto_item_set_text(item, "Command: NULL");
			}
			break;
		case V1_COMMAND_AFI:
			af = tvb_get_guint8(tvb, offset);
			proto_tree_add_uint(tree, hf_afi, tvb,
				offset, 1, af);
			offset += 1;
			if (item) {
				proto_item_set_text(item, "%s: %s",
					val_to_str(cmd, command, "Unknown Command:0x%02x"),
					val_to_str(af, afi, "Unknown Family:0x%02x")
				);
			}
			break;		
		case V1_COMMAND_SUBNETMASK:
			count = tvb_get_guint8(tvb, offset);
			proto_tree_add_uint(tree, hf_count, tvb,
				offset, 1, count);
			offset += 1;
			if (count) { /* must be 0 or 1 */
				proto_tree_add_ipv4(tree, hf_netmask, 
					tvb, offset, 4, 
					tvb_get_letohl(tvb, offset));
				if (item) {
					proto_item_set_text(item, "%s: %d.%d.%d.%d",
						val_to_str(cmd, command, "Unknown Command:0x%02x"), 
						tvb_get_guint8(tvb, offset),
						tvb_get_guint8(tvb, offset+1),
						tvb_get_guint8(tvb, offset+2),
						tvb_get_guint8(tvb, offset+3));
				}
				offset += 4;
			} else {
				if (item) {
					proto_item_set_text(item, "%s: <no mask supplied>",
						val_to_str(cmd, command, "Unknown Command:0x%02x"));
				}
			}
			break;	
		case V1_COMMAND_METRIC:
			proto_tree_add_uint(tree, hf_metric, tvb,
				offset, 1, tvb_get_guint8(tvb, offset));
			if (item) {
				proto_item_set_text(item, "%s: %d",
					val_to_str(cmd, command, "Unknown Command:0x%02x"), 
					tvb_get_guint8(tvb, offset));
			}
			offset += 1;
			break;		
		case V1_COMMAND_FLAGS0:
			count = tvb_get_guint8(tvb, offset);
			proto_tree_add_boolean(tree, hf_dest_unr, tvb, offset, 1, count);
			proto_tree_add_boolean(tree, hf_split_horiz, tvb, offset, 1, count);
			if (item) {
				proto_item_set_text(item, "%s: 0x%02x",
					val_to_str(cmd, command, "Unknown Command:0x%02x"), count);
			}
			offset += 1;
			break;
		case V1_COMMAND_INFINITY:
			proto_tree_add_uint(tree, hf_infinity, tvb,
				offset, 1, tvb_get_guint8(tvb, offset));
			if (item) {
				proto_item_set_text(item, "%s: %d",
					val_to_str(cmd, command, "Unknown Command:0x%02x"), tvb_get_guint8(tvb, offset));
			}
			offset += 1;
			break;		
		case V1_COMMAND_DA:
		case V1_COMMAND_RDA: /* same as DA */
			count = tvb_get_guint8(tvb, offset);
			proto_tree_add_uint(tree, hf_count, tvb,
				offset, 1, count);
			offset += 1;
			while (count--) {
				proto_tree_add_ipv4(tree, hf_daddr, 
					tvb, offset, 4, 
					tvb_get_letohl(tvb, offset));
				offset += 4;
			}
			if (item) {
				proto_item_set_text(item, "%s",
					val_to_str(cmd, command, "Unknown Command:0x%02x"));
			}
			break;	
		case V1_COMMAND_NMR:
			count = tvb_get_guint8(tvb, offset);
			proto_tree_add_uint(tree, hf_count, tvb,
				offset, 1, count);
			offset += 1;
			while (count--) {
				proto_tree_add_ipv4(tree, hf_maddr, 
					tvb, offset, 4, 
					tvb_get_letohl(tvb, offset));
				offset += 4;
				proto_tree_add_uint(tree, hf_hold, tvb,
					offset, 4, tvb_get_ntohl(tvb, offset));
				offset += 4;
			}
			if (item) {
				proto_item_set_text(item, "%s",
					val_to_str(cmd, command, "Unknown Command:0x%02x"));
			}
			break;	
		case V1_COMMAND_NMR_CANCEL:
			count = tvb_get_guint8(tvb, offset);
			proto_tree_add_uint(tree, hf_count, tvb,
				offset, 1, count);
			offset += 1;
			while (count--) {
				proto_tree_add_ipv4(tree, hf_maddr, 
					tvb, offset, 4, 
					tvb_get_letohl(tvb, offset));
				offset += 4;
			}
			if (item) {
				proto_item_set_text(item, "%s",
					val_to_str(cmd, command, "Unknown Command:0x%02x"));
			}
			break;	
		}

		proto_item_set_len(item, offset-old_offset);
	}

	return offset;
}

/* This function is only called from the IGMP dissector */
int
dissect_dvmrp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree, int offset)
{
	proto_tree *tree;
	proto_item *item;

	if (!proto_is_protocol_enabled(proto_dvmrp)) {
		/* we are not enabled, skip entire packet to be nice
		   to the igmp layer. (so clicking on IGMP will display the data)
		 */
		return offset+tvb_length_remaining(tvb, offset);
	}

	item = proto_tree_add_item(parent_tree, proto_dvmrp, tvb, offset, 0, FALSE);
	tree = proto_item_add_subtree(item, ett_dvmrp);


	if (check_col(pinfo->fd, COL_PROTOCOL)) {
		col_set_str(pinfo->fd, COL_PROTOCOL, "DVMRP");
	}
	if (check_col(pinfo->fd, COL_INFO)) {
		col_clear(pinfo->fd, COL_INFO);
	}


	if ((tvb_length_remaining(tvb, offset)>=8)
	 && (tvb_get_guint8(tvb, 6)==0xff)
	 && (tvb_get_guint8(tvb, 7)==0x03)) {
		offset = dissect_dvmrp_v3(tvb, pinfo, tree, offset);
		proto_item_set_len(item, offset);
		return offset;
	}

	
	offset = dissect_dvmrp_v1(tvb, pinfo, tree, offset);
	proto_item_set_len(item, offset);
	return offset;
}

void
proto_register_dvmrp(void)
{
	static hf_register_info hf[] = {
		{ &hf_version,
			{ "DVMRP Version", "dvmrp.version", FT_UINT8, BASE_DEC,
			  NULL, 0, "DVMRP Version" }},

		{ &hf_type,
			{ "Type", "dvmrp.type", FT_UINT8, BASE_HEX,
			  VALS(dvmrp_type), 0, "DVMRP Packet Type" }},

		{ &hf_code_v1,
			{ "Code", "dvmrp.v1.code", FT_UINT8, BASE_HEX,
			  VALS(code_v1), 0, "DVMRP Packet Code" }},

		{ &hf_checksum,
			{ "Checksum", "dvmrp.checksum", FT_UINT16, BASE_HEX,
			  NULL, 0, "DVMRP Checksum" }},

		{ &hf_checksum_bad,
			{ "Bad Checksum", "dvmrp.checksum_bad", FT_BOOLEAN, BASE_NONE,
			  NULL, 0, "Bad DVMRP Checksum" }},

		{ &hf_commands,
			{ "Commands", "dvmrp.commands", FT_NONE, BASE_NONE,
			  NULL, 0, "DVMRP V1 Commands" }},

		{ &hf_command,
			{ "Command", "dvmrp.command", FT_UINT8, BASE_HEX,
			  VALS(command), 0, "DVMRP V1 Command" }},

		{ &hf_afi,
			{ "Address Family", "dvmrp.afi", FT_UINT8, BASE_HEX,
			  VALS(afi), 0, "DVMRP Address Family Indicator" }},

		{ &hf_count,
			{ "Count", "dvmrp.count", FT_UINT8, BASE_HEX,
			  NULL, 0, "Count" }},

		{ &hf_netmask,
			{ "Netmask", "igmp.netmask", FT_IPv4, BASE_NONE,
			  NULL, 0, "DVMRP Netmask" }},

		{ &hf_metric,
			{ "Metric", "dvmrp.metric", FT_UINT8, BASE_DEC,
			  NULL, 0, "DVMRP Metric" }},

		{&hf_dest_unr,
			{ "Destination Unreachable", "dvmrp.dest_unreach", FT_BOOLEAN, 8,
			TFS(&tfs_dest_unreach), 0x01, "Destination Unreachable" }},

		{&hf_split_horiz,
			{ "Split Horizon", "dvmrp.split_horiz", FT_BOOLEAN, 8,
			TFS(&tfs_split_horiz), 0x02, "Split Horizon concealed route" }},

		{ &hf_infinity,
			{ "Infinity", "dvmrp.infinity", FT_UINT8, BASE_DEC,
			  NULL, 0, "DVMRP Infinity" }},

		{ &hf_daddr,
			{ "Dest Addr", "igmp.daddr", FT_IPv4, BASE_NONE,
			  NULL, 0, "DVMRP Destination Address" }},

		{ &hf_maddr,
			{ "Multicast Addr", "igmp.maddr", FT_IPv4, BASE_NONE,
			  NULL, 0, "DVMRP Multicast Address" }},

		{ &hf_hold,
			{ "Hold Time", "dvmrp.hold", FT_UINT32, BASE_DEC,
			  NULL, 0, "DVMRP Hold Time in seconds" }},

		{ &hf_code_v3,
			{ "Code", "dvmrp.v3.code", FT_UINT8, BASE_HEX,
			  VALS(code_v3), 0, "DVMRP Packet Code" }},

		{ &hf_capabilities,
			{ "Capabilities", "dvmrp.capabilities", FT_NONE, BASE_NONE,
			  NULL, 0, "DVMRP V3 Capabilities" }},

		{&hf_cap_leaf,
			{ "Leaf", "dvmrp.cap.leaf", FT_BOOLEAN, 8,
			TFS(&tfs_cap_leaf), DVMRP_V3_CAP_LEAF, "Leaf" }},

		{&hf_cap_prune,
			{ "Prune", "dvmrp.cap.prune", FT_BOOLEAN, 8,
			TFS(&tfs_cap_prune), DVMRP_V3_CAP_PRUNE, "Prune capability" }},

		{&hf_cap_genid,
			{ "Genid", "dvmrp.cap.genid", FT_BOOLEAN, 8,
			TFS(&tfs_cap_genid), DVMRP_V3_CAP_GENID, "Genid capability" }},

		{&hf_cap_mtrace,
			{ "Mtrace", "dvmrp.cap.mtrace", FT_BOOLEAN, 8,
			TFS(&tfs_cap_mtrace), DVMRP_V3_CAP_MTRACE, "Mtrace capability" }},

		{&hf_cap_snmp,
			{ "SNMP", "dvmrp.cap.snmp", FT_BOOLEAN, 8,
			TFS(&tfs_cap_snmp), DVMRP_V3_CAP_SNMP, "SNMP capability" }},

		{&hf_cap_netmask,
			{ "Netmask", "dvmrp.cap.netmask", FT_BOOLEAN, 8,
			TFS(&tfs_cap_netmask), DVMRP_V3_CAP_NETMASK, "Netmask capability" }},

		{ &hf_min_ver,
			{ "Minor Version", "dvmrp.min_ver", FT_UINT8, BASE_HEX,
			  NULL, 0, "DVMRP Minor Version" }},

		{ &hf_maj_ver,
			{ "Major Version", "dvmrp.maj_ver", FT_UINT8, BASE_HEX,
			  NULL, 0, "DVMRP Major Version" }},

		{ &hf_genid,
			{ "Generation ID", "dvmrp.genid", FT_UINT32, BASE_DEC,
			  NULL, 0, "DVMRP Generation ID" }},

		{ &hf_naddr,
			{ "Neighbor Addr", "igmp.naddr", FT_IPv4, BASE_NONE,
			  NULL, 0, "DVMRP Neighbor Address" }},

		{ &hf_route,
			{ "Route", "dvmrp.route", FT_NONE, BASE_NONE,
			  NULL, 0, "DVMRP V3 Route Report" }},

		{ &hf_saddr,
			{ "Source Addr", "igmp.saddr", FT_IPv4, BASE_NONE,
			  NULL, 0, "DVMRP Source Address" }},

		{ &hf_life,
			{ "Prune lifetime", "dvmrp.lifetime", FT_UINT32, BASE_DEC,
			  NULL, 0, "DVMRP Prune Lifetime" }},

		{ &hf_neighbor,
			{ "Neighbor Addr", "igmp.neighbor", FT_IPv4, BASE_NONE,
			  NULL, 0, "DVMRP Neighbor Address" }},

	};
	static gint *ett[] = {
		&ett_dvmrp,
		&ett_commands,
		&ett_capabilities,
		&ett_route,
	};

	proto_dvmrp = proto_register_protocol("Distance Vector Multicast Routing Protocol",
	    "DVMRP", "dvmrp");
	proto_register_field_array(proto_dvmrp, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));
}

