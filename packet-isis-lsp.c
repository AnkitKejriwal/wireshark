/* packet-isis-lsp.c
 * Routines for decoding isis lsp packets and their CLVs
 *
 * $Id: packet-isis-lsp.c,v 1.41 2003/04/03 05:22:08 guy Exp $
 * Stuart Stanley <stuarts@mxmail.net>
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

#include <stdio.h>
#include <string.h>
#include <glib.h>

#include "epan/ipv4.h"
#include <epan/packet.h>
#include "packet-osi.h"
#include "packet-ipv6.h"
#include "packet-isis.h"
#include "packet-isis-clv.h"
#include "packet-isis-lsp.h"
#include <epan/resolv.h>

/* lsp packets */
static int hf_isis_lsp_pdu_length = -1;
static int hf_isis_lsp_remaining_life = -1;
static int hf_isis_lsp_sequence_number = -1;
static int hf_isis_lsp_checksum = -1;
static int hf_isis_lsp_clv_ipv4_int_addr = -1;
static int hf_isis_lsp_clv_ipv6_int_addr = -1;
static int hf_isis_lsp_clv_te_router_id = -1;
static int hf_isis_lsp_clv_mt = -1;
static int hf_isis_lsp_p = -1;
static int hf_isis_lsp_att = -1;
static int hf_isis_lsp_hippity = -1;
static int hf_isis_lsp_is_type = -1;

static gint ett_isis_lsp = -1;
static gint ett_isis_lsp_info = -1;
static gint ett_isis_lsp_att = -1;
static gint ett_isis_lsp_clv_area_addr = -1;
static gint ett_isis_lsp_clv_is_neighbors = -1;
static gint ett_isis_lsp_clv_ext_is_reachability = -1; /* CLV 22 */
	static gint ett_isis_lsp_part_of_clv_ext_is_reachability = -1;
	static gint ett_isis_lsp_subclv_admin_group = -1;
	static gint ett_isis_lsp_subclv_unrsv_bw = -1;
static gint ett_isis_lsp_clv_unknown = -1;
static gint ett_isis_lsp_clv_partition_dis = -1;
static gint ett_isis_lsp_clv_prefix_neighbors = -1;
static gint ett_isis_lsp_clv_nlpid = -1;
static gint ett_isis_lsp_clv_hostname = -1;
static gint ett_isis_lsp_clv_te_router_id = -1;
static gint ett_isis_lsp_clv_auth = -1;
static gint ett_isis_lsp_clv_ipv4_int_addr = -1;
static gint ett_isis_lsp_clv_ipv6_int_addr = -1; /* CLV 232 */
static gint ett_isis_lsp_clv_ip_reachability = -1;
static gint ett_isis_lsp_clv_ip_reach_subclv = -1;
static gint ett_isis_lsp_clv_ext_ip_reachability = -1; /* CLV 135 */
	static gint ett_isis_lsp_part_of_clv_ext_ip_reachability = -1;
static gint ett_isis_lsp_clv_ipv6_reachability = -1; /* CLV 236 */
	static gint ett_isis_lsp_part_of_clv_ipv6_reachability = -1;
static gint ett_isis_lsp_clv_mt = -1;
static gint ett_isis_lsp_clv_mt_is = -1;
static gint ett_isis_lsp_part_of_clv_mt_is = -1;
static gint ett_isis_lsp_clv_mt_reachable_IPv4_prefx = -1;  /* CLV 235 */
static gint ett_isis_lsp_clv_mt_reachable_IPv6_prefx = -1;  /* CLV 237 */

static const value_string isis_lsp_istype_vals[] = {
	{ ISIS_LSP_TYPE_UNUSED0,	"Unused 0x0 (invalid)"},
	{ ISIS_LSP_TYPE_LEVEL_1,	"Level 1"},
	{ ISIS_LSP_TYPE_UNUSED2,	"Unused 0x2 (invalid)"},
	{ ISIS_LSP_TYPE_LEVEL_2,	"Level 2"},
	{ 0, NULL } };

static const true_false_string supported_string = {
		"Supported",
		"Unsupported"
	};

static const true_false_string hippity_string = {
		"Set",
		"Unset"
	};


/*
 * Predclare dissectors for use in clv dissection.
 */
static void dissect_lsp_prefix_neighbors_clv(tvbuff_t *tvb,
	proto_tree *tree, int offset, int id_length, int length);
static void dissect_lsp_partition_dis_clv(tvbuff_t *tvb,
	proto_tree *tree, int offset, int id_length, int length);
static void dissect_lsp_mt_is_reachability_clv(tvbuff_t *tvb,
	proto_tree *tree, int offset, int id_length, int length);
static void dissect_lsp_ext_is_reachability_clv(tvbuff_t *tvb,
	proto_tree *tree, int offset, int id_length, int length);
static void dissect_lsp_l2_is_neighbors_clv(tvbuff_t *tvb,
	proto_tree *tree, int offset, int id_length, int length);
static void dissect_lsp_l1_es_neighbors_clv(tvbuff_t *tvb,
	proto_tree *tree, int offset, int id_length, int length);
static void dissect_lsp_l1_is_neighbors_clv(tvbuff_t *tvb,
	proto_tree *tree, int offset, int id_length, int length);
static void dissect_lsp_area_address_clv(tvbuff_t *tvb,
	proto_tree *tree, int offset, int id_length, int length);
static void dissect_lsp_l2_auth_clv(tvbuff_t *tvb,
	proto_tree *tree, int offset, int id_length, int length);
static void dissect_lsp_l1_auth_clv(tvbuff_t *tvb,
	proto_tree *tree, int offset, int id_length, int length);
static void dissect_lsp_ipv6_int_addr_clv(tvbuff_t *tvb,
	proto_tree *tree, int offset, int id_length, int length);
static void dissect_lsp_ip_int_addr_clv(tvbuff_t *tvb,
	proto_tree *tree, int offset, int id_length, int length);
static void dissect_lsp_te_router_id_clv(tvbuff_t *tvb,
	proto_tree *tree, int offset, int id_length, int length);
static void dissect_lsp_hostname_clv(tvbuff_t *tvb,
	proto_tree *tree, int offset, int id_length, int length);
static void dissect_lsp_mt_clv(tvbuff_t *tvb,
	proto_tree *tree, int offset, int id_length, int length);
static void dissect_lsp_nlpid_clv(tvbuff_t *tvb,
	proto_tree *tree, int offset, int id_length, int length);
static void dissect_lsp_ipv6_reachability_clv(tvbuff_t *tvb,
	proto_tree *tree, int offset, int id_length, int length);
static void dissect_lsp_ext_ip_reachability_clv(tvbuff_t *tvb,
	proto_tree *tree, int offset, int id_length, int length);
static void dissect_lsp_ip_reachability_clv(tvbuff_t *tvb,
	proto_tree *tree, int offset, int id_length, int length);
static void dissect_ipreach_subclv(tvbuff_t *tvb,
        proto_tree *tree, int offset, int clv_code, int clv_len);
static void dissect_lsp_mt_reachable_IPv4_prefx_clv(tvbuff_t *tvb,
        proto_tree *tree, int offset, int id_length, int length);
static void dissect_lsp_mt_reachable_IPv6_prefx_clv(tvbuff_t *tvb,
        proto_tree *tree, int offset, int id_length, int length);


static const isis_clv_handle_t clv_l1_lsp_opts[] = {
	{
		ISIS_CLV_L1_LSP_AREA_ADDRESS,
		"Area address(es)",
		&ett_isis_lsp_clv_area_addr,
		dissect_lsp_area_address_clv
	},
	{
		ISIS_CLV_L1_LSP_IS_NEIGHBORS,
		"IS Reachability",
		&ett_isis_lsp_clv_is_neighbors,
		dissect_lsp_l1_is_neighbors_clv
	},
	{
		ISIS_CLV_L1_LSP_ES_NEIGHBORS,
		"ES Neighbor(s)",
		&ett_isis_lsp_clv_is_neighbors,
		dissect_lsp_l1_es_neighbors_clv
	},
	{
		ISIS_CLV_L1_LSP_EXT_IS_REACHABLE,
		"Extended IS reachability",
		&ett_isis_lsp_clv_ext_is_reachability,
		dissect_lsp_ext_is_reachability_clv
	},
	{
		ISIS_CLV_L1_LSP_IP_INT_REACHABLE,
		"IP Internal reachability",
		&ett_isis_lsp_clv_ip_reachability,
		dissect_lsp_ip_reachability_clv
	},
	{
		ISIS_CLV_L1_LSP_IP_EXT_REACHABLE,
		"IP External reachability",
		&ett_isis_lsp_clv_ip_reachability,
		dissect_lsp_ip_reachability_clv
	},
	{
		ISIS_CLV_L1_LSP_EXT_IP_REACHABLE,
		"Extended IP Reachability",
		&ett_isis_lsp_clv_ext_ip_reachability,
		dissect_lsp_ext_ip_reachability_clv
	},
	{
		ISIS_CLV_L1_LSP_IPv6_REACHABLE,
		"IPv6 reachability",
		&ett_isis_lsp_clv_ipv6_reachability,
		dissect_lsp_ipv6_reachability_clv
	},
	{
		ISIS_CLV_L1_LSP_NLPID,
		"Protocols supported",
		&ett_isis_lsp_clv_nlpid,
		dissect_lsp_nlpid_clv
	},
        {
                ISIS_CLV_L1_LSP_HOSTNAME,
                "Hostname",
                &ett_isis_lsp_clv_hostname,
                dissect_lsp_hostname_clv
        },
        {
                ISIS_CLV_L1_LSP_TE_ROUTER_ID,
                "Traffic Engineering Router ID",
                &ett_isis_lsp_clv_te_router_id,
                dissect_lsp_te_router_id_clv
        },
	{
		ISIS_CLV_L1_LSP_IP_INTERFACE_ADDR,
		"IP Interface address(es)",
		&ett_isis_lsp_clv_ipv4_int_addr,
		dissect_lsp_ip_int_addr_clv
	},
	{
		ISIS_CLV_L1_LSP_IPv6_INTERFACE_ADDR,
		"IPv6 Interface address(es)",
		&ett_isis_lsp_clv_ipv6_int_addr,
		dissect_lsp_ipv6_int_addr_clv
	},
	{
		ISIS_CLV_L1_LSP_AUTHENTICATION_NS,
		"Authentication(non-spec)",
		&ett_isis_lsp_clv_auth,
		dissect_lsp_l1_auth_clv
	},
	{
		ISIS_CLV_L1_LSP_AUTHENTICATION,
		"Authentication",
		&ett_isis_lsp_clv_auth,
		dissect_lsp_l1_auth_clv
	},
	{
		ISIS_CLV_L1_LSP_MT,
		"Multi Topology",
		&ett_isis_lsp_clv_mt,
		dissect_lsp_mt_clv
	},
	{
		ISIS_CLV_L1_LSP_MT_IS_REACHABLE,
		"Multi Topology IS Reachability",
		&ett_isis_lsp_clv_mt_is,
		dissect_lsp_mt_is_reachability_clv
	},
	{
		ISIS_CLV_L1_LSP_MT_REACHABLE_IPv4_PREFX,
		"Multi Topology Reachable IPv4 Prefixes",
		&ett_isis_lsp_clv_mt_reachable_IPv4_prefx,
		dissect_lsp_mt_reachable_IPv4_prefx_clv
	},
	{
		ISIS_CLV_L1_LSP_MT_REACHABLE_IPv6_PREFX,
		"Multi Topology Reachable IPv6 Prefixes",
		&ett_isis_lsp_clv_mt_reachable_IPv6_prefx,
		dissect_lsp_mt_reachable_IPv6_prefx_clv
	},
	{
		0,
		"",
		NULL,
		NULL
	}
};

static const isis_clv_handle_t clv_l2_lsp_opts[] = {
	{
		ISIS_CLV_L1_LSP_AREA_ADDRESS,
		"Area address(es)",
		&ett_isis_lsp_clv_area_addr,
		dissect_lsp_area_address_clv
	},
	{
		ISIS_CLV_L2_LSP_IS_NEIGHBORS,
		"IS Reachability",
		&ett_isis_lsp_clv_is_neighbors,
		dissect_lsp_l2_is_neighbors_clv
	},
	{
		ISIS_CLV_L2_LSP_EXT_IS_REACHABLE,
		"Extended IS reachability",
		&ett_isis_lsp_clv_ext_is_reachability,
		dissect_lsp_ext_is_reachability_clv
	},
	{
		ISIS_CLV_L2_LSP_PARTITION_DIS,
		"Parition Designated Level 2 IS",
		&ett_isis_lsp_clv_partition_dis,
		dissect_lsp_partition_dis_clv
	},
	{
		ISIS_CLV_L2_LSP_PREFIX_NEIGHBORS,
		"Prefix neighbors",
		&ett_isis_lsp_clv_prefix_neighbors,
		dissect_lsp_prefix_neighbors_clv
	},
	{
		ISIS_CLV_L2_LSP_IP_INT_REACHABLE,
		"IP Internal reachability",
		&ett_isis_lsp_clv_ip_reachability,
		dissect_lsp_ip_reachability_clv
	},
	{
		ISIS_CLV_L2_LSP_IP_EXT_REACHABLE,
		"IP External reachability",
		&ett_isis_lsp_clv_ip_reachability,
		dissect_lsp_ip_reachability_clv
	},
	{
		ISIS_CLV_L2_LSP_NLPID,
		"Protocols supported",
		&ett_isis_lsp_clv_nlpid,
		dissect_lsp_nlpid_clv
	},
        {
                ISIS_CLV_L2_LSP_HOSTNAME,
                "Hostname",
                &ett_isis_lsp_clv_hostname,
                dissect_lsp_hostname_clv
        },
        {
                ISIS_CLV_L2_LSP_TE_ROUTER_ID,
                "Traffic Engineering Router ID",
                &ett_isis_lsp_clv_te_router_id,
                dissect_lsp_te_router_id_clv
        },
	{
		ISIS_CLV_L2_LSP_EXT_IP_REACHABLE,
		"Extended IP Reachability",
		&ett_isis_lsp_clv_ext_ip_reachability,
		dissect_lsp_ext_ip_reachability_clv
	},
	{
		ISIS_CLV_L2_LSP_IPv6_REACHABLE,
		"IPv6 reachability",
		&ett_isis_lsp_clv_ipv6_reachability,
		dissect_lsp_ipv6_reachability_clv
	},
	{
		ISIS_CLV_L2_LSP_IP_INTERFACE_ADDR,
		"IP Interface address(es)",
		&ett_isis_lsp_clv_ipv4_int_addr,
		dissect_lsp_ip_int_addr_clv
	},
	{
		ISIS_CLV_L2_LSP_IPv6_INTERFACE_ADDR,
		"IPv6 Interface address(es)",
		&ett_isis_lsp_clv_ipv6_int_addr,
		dissect_lsp_ipv6_int_addr_clv
	},
	{
		ISIS_CLV_L2_LSP_AUTHENTICATION_NS,
		"Authentication(non spec)",
		&ett_isis_lsp_clv_auth,
		dissect_lsp_l2_auth_clv
	},
	{
		ISIS_CLV_L2_LSP_AUTHENTICATION,
		"Authentication",
		&ett_isis_lsp_clv_auth,
		dissect_lsp_l2_auth_clv
	},
	{
		ISIS_CLV_L2_LSP_MT,
		"Multi Topology",
		&ett_isis_lsp_clv_mt,
		dissect_lsp_mt_clv
	},
	{
		ISIS_CLV_L2_LSP_MT_IS_REACHABLE,
		"Multi Topology IS Reachability",
		&ett_isis_lsp_clv_mt_is,
		dissect_lsp_mt_is_reachability_clv
	},
	{
		ISIS_CLV_L2_LSP_MT_REACHABLE_IPv4_PREFX,
		"Multi Topology Reachable IPv4 Prefixes",
		&ett_isis_lsp_clv_mt_reachable_IPv4_prefx,
		dissect_lsp_mt_reachable_IPv4_prefx_clv
	},
	{
		ISIS_CLV_L2_LSP_MT_REACHABLE_IPv6_PREFX,
		"Multi Topology Reachable IPv6 Prefixes",
		&ett_isis_lsp_clv_mt_reachable_IPv6_prefx,
		dissect_lsp_mt_reachable_IPv6_prefx_clv
	},
	{
		0,
		"",
		NULL,
		NULL
	}
};

/*
 * Name: dissect_lsp_mt_id()
 *
 * Description:
 *	dissect and display the multi-topology ID value
 *
 * Input:
 *	tvbuff_t * : tvbuffer for packet data
 *	proto_tree * : protocol display tree to fill out.  CAN'T BE NULL
 *	int : offset into packet data where we are.
 *
 * Output:
 *	void, but we will add to proto tree.
 */
static void
dissect_lsp_mt_id(tvbuff_t *tvb, proto_tree *tree, int offset)
{
	int  mt_block, mt_id;
	char mt_desc[60];

	/* fetch two bytes */
	mt_block = tvb_get_ntohs(tvb, offset);

	proto_tree_add_text ( tree, tvb, offset, 1 ,
                        "4 most significant bits reserved, should be set to 0 (%d)", ISIS_LSP_MT_MSHIP_RES(mt_block));

	mt_id = ISIS_LSP_MT_MSHIP_ID(mt_block);
	/*mask out the lower 12 bits */
	switch(mt_id) {
	case 0:
		strcpy(mt_desc,"'standard' topology");
		break;
	case 1:
		strcpy(mt_desc,"IPv4 In-Band Management purposes");
		break;
	case 2:
		strcpy(mt_desc,"IPv6 routing topology");
		break;
	case 3:
		strcpy(mt_desc,"IPv4 multicast routing topology");
		break;
	case 4:
		strcpy(mt_desc,"IPv6 multicast routing topology");
		break;
	default:
		strcpy(mt_desc,((mt_block & 0x0fff) < 3996) ? "Reserved for IETF Consensus" : "Development, Experimental and Proprietary features");
	}

	proto_tree_add_text ( tree, tvb, offset, 2 ,
                        "%s Topology (%d)", mt_desc, mt_id);

}

/*
 * Name: dissect_metric()
 *
 * Description:
 *	Display a metric prefix portion.  ISIS has the concept of multple
 *	metric per prefix (default, delay, expense, and error).  This
 *	routine assists other dissectors by adding a single one of
 *	these to the display tree..
 *
 *	The 8th(msbit) bit in the metric octet is the "supported" bit.  The
 *		"default" support is required, so we support a "force_supported"
 *		flag that tells us that it MUST be zero (zero==supported,
 *		so it really should be a "not supported" in the boolean sense)
 *		and to display a protocol failure accordingly.  Notably,
 *		Cisco IOS 12(6) blows this!
 *	The 7th bit must be zero (reserved).
 *
 * Input:
 *	tvbuff_t * : tvbuffer for packet data
 *	proto_tree * : protocol display tree to fill out.  May be NULL
 *	int : offset into packet data where we are.
 *	guint8 : value of the metric.
 *	char * : string giving type of the metric.
 *	int : force supported.  True is the supported bit MUST be zero.
 *
 * Output:
 *	void, but we will add to proto tree if !NULL.
 */
static void
dissect_metric(tvbuff_t *tvb, proto_tree *tree,	int offset, guint8 value,
	char *pstr, int force_supported )
{
	int s;

	if ( !tree ) return;

	s = ISIS_LSP_CLV_METRIC_SUPPORTED(value);
	proto_tree_add_text(tree, tvb, offset, 1,
		"%s Metric: %s%s %s%d:%d", pstr,
		s ? "Not supported" : "Supported",
		(s && force_supported) ? "(but is required to be)":"",
		ISIS_LSP_CLV_METRIC_RESERVED(value) ? "(reserved bit != 0)":"",
		ISIS_LSP_CLV_METRIC_VALUE(value), value );
}

/*
 * Name: dissect_lsp_ip_reachability_clv()
 *
 * Description:
 *	Decode an IP reachability CLV.  This can be either internal or
 *	external (the clv format does not change and which type we are
 *	displaying is put there by the dispatcher).  All of these
 *	are a metric block followed by an IP addr and mask.
 *
 * Input:
 *	tvbuff_t * : tvbuffer for packet data
 *	proto_tree * : proto tree to build on (may be null)
 *	int : current offset into packet data
 *	int : length of IDs in packet.
 *	int : length of this clv
 *
 * Output:
 *	void, will modify proto_tree if not null.
 */
static void
dissect_lsp_ip_reachability_clv(tvbuff_t *tvb, proto_tree *tree, int offset,
	int id_length _U_, int length)
{
	proto_item 	*ti;
	proto_tree	*ntree = NULL;
	guint32		src, mask, prefix_len;

        guint32 bitmasks[33] = {
	  0x00000000,
	  0x00000008, 0x0000000c, 0x0000000e, 0x0000000f,
	  0x000000f8, 0x000000fc, 0x000000fe, 0x000000ff,
	  0x000008ff, 0x00000cff, 0x00000eff, 0x00000fff,
	  0x0000f8ff, 0x0000fcff, 0x0000feff, 0x0000ffff,
	  0x0008ffff, 0x000cffff, 0x000effff, 0x000fffff,
	  0x00f8ffff, 0x00fcffff, 0x00feffff, 0x00ffffff,
	  0x08ffffff, 0x0cffffff, 0x0effffff, 0x0fffffff,
	  0xf8ffffff, 0xfcffffff, 0xfeffffff, 0xffffffff
	};


	while ( length > 0 ) {
		if (length<12) {
			isis_dissect_unknown(tvb, tree, offset,
				"short IP reachability (%d vs 12)", length );
			return;
		}
		/*
		 * Gotta build a sub-tree for all our pieces
		 */
		if ( tree ) {
			tvb_memcpy(tvb, (guint8 *)&src, offset+4, 4);
			tvb_memcpy(tvb, (guint8 *)&mask, offset+8, 4);

			/* find out if the mask matches one of 33 possible prefix lengths */

			prefix_len=0;

			while(prefix_len<=33) {
			  if (bitmasks[prefix_len++]==mask) {
			    prefix_len--;
			    break;
			  }
			}

			/* 34 indicates no match -> must be a discontiguous netmask
			   lets dump the mask, otherwise print the prefix_len */

			if(prefix_len==34) {
			  ti = proto_tree_add_text ( tree, tvb, offset, 12,
				"IPv4 prefix: %s mask %s",
				ip_to_str((guint8*)&src),
				ip_to_str((guint8*)&mask));
			} else {
			  ti = proto_tree_add_text ( tree, tvb, offset, 12,
				"IPv4 prefix: %s/%d",
				ip_to_str((guint8*)&src),
				prefix_len );
			};

			ntree = proto_item_add_subtree(ti,
				ett_isis_lsp_clv_ip_reachability);

			proto_tree_add_text (ntree, tvb, offset, 1,
				"Default Metric: %d, %s, Distribution: %s",
				ISIS_LSP_CLV_METRIC_VALUE(tvb_get_guint8(tvb, offset)),
				ISIS_LSP_CLV_METRIC_IE(tvb_get_guint8(tvb, offset)) ? "External" : "Internal",
				ISIS_LSP_CLV_METRIC_UPDOWN(tvb_get_guint8(tvb, offset)) ? "down" : "up");


			if (ISIS_LSP_CLV_METRIC_SUPPORTED(tvb_get_guint8(tvb, offset+1))) {
			  proto_tree_add_text (ntree, tvb, offset+1, 1, "Delay Metric:   Not supported");
					       } else {
                          proto_tree_add_text (ntree, tvb, offset+1, 1, "Delay Metric:   %d, %s",
					       ISIS_LSP_CLV_METRIC_VALUE(tvb_get_guint8(tvb, offset+1)),
					       ISIS_LSP_CLV_METRIC_IE(tvb_get_guint8(tvb, offset+1)) ? "External" : "Internal");
					       }

                        if (ISIS_LSP_CLV_METRIC_SUPPORTED(tvb_get_guint8(tvb, offset+2))) {
                          proto_tree_add_text (ntree, tvb, offset+2, 1, "Expense Metric: Not supported");
			} else {
                          proto_tree_add_text (ntree, tvb, offset+2, 1, "Exense Metric:  %d, %s",
                                               ISIS_LSP_CLV_METRIC_VALUE(tvb_get_guint8(tvb, offset+2)),
					       ISIS_LSP_CLV_METRIC_IE(tvb_get_guint8(tvb, offset+2)) ? "External" : "Internal");
			}

                        if (ISIS_LSP_CLV_METRIC_SUPPORTED(tvb_get_guint8(tvb, offset+3))) {
                          proto_tree_add_text (ntree, tvb, offset+3, 1, "Error Metric:   Not supported");
			} else {
                          proto_tree_add_text (ntree, tvb, offset+3, 1, "Error Metric:   %d, %s",
                                               ISIS_LSP_CLV_METRIC_VALUE(tvb_get_guint8(tvb, offset+3)),
					       ISIS_LSP_CLV_METRIC_IE(tvb_get_guint8(tvb, offset+3)) ? "External" : "Internal");
			}
		}
		offset += 12;
		length -= 12;
	}
}

/*
 * Name: dissect_ipreach_subclv ()
 *
 * Description: parses IP reach subTLVs
 *              Called by various IP Reachability dissectors.
 *
 * Input:
 *   tvbuff_t * : tvbuffer for packet data
 *   proto_tree * : protocol display tree to fill out.
 *   int : offset into packet data where we are (beginning of the sub_clv value).
 *
 * Output:
 *   void
 */
static void
dissect_ipreach_subclv(tvbuff_t *tvb, proto_tree *tree, int offset, int clv_code, int clv_len)
{

        switch (clv_code) {
        case 1:
                while (clv_len >= 4) {
                        proto_tree_add_text(tree, tvb, offset, 4,
                                    "32-Bit Administrative tag: 0x%08x (=%u)",
                                    tvb_get_ntohl(tvb, offset),
                                    tvb_get_ntohl(tvb, offset));                
                        offset+=4;
                        clv_len-=4;
                }
                break;
        case 2:
                while (clv_len >= 8) {
                        proto_tree_add_text(tree, tvb, offset, 8,
                                    "64-Bit Administrative tag: 0x%08x%08x",
                                    tvb_get_ntohl(tvb, offset),
                                    tvb_get_ntohl(tvb, offset+4));
                        offset+=8;
                        clv_len-=8;
                }
                break;

        default :
                proto_tree_add_text (tree, tvb, offset, clv_len+2,
                                     "Unknown sub-TLV: code %u, length %u",
                                     clv_code, clv_len );
                break;
        }
}


/*
 * Name: dissect_lsp_ext_ip_reachability_clv()
 *
 * Description: Decode an Extended IP Reachability CLV - code 135.
 *
 *   The extended IP reachability TLV is an extended version
 *   of the IP reachability TLVs (codes 128 and 130). It encodes
 *   the metric as a 32-bit unsigned interger and allows to add
 *   sub-CLV(s).
 *
 *   CALLED BY TLV 235 DISSECTOR
 *
 *
 * Input:
 *   tvbuff_t * : tvbuffer for packet data
 *   proto_tree * : proto tree to build on (may be null)
 *   int : current offset into packet data
 *   int : length of IDs in packet.
 *   int : length of this clv
 *
 * Output:
 *   void, will modify proto_tree if not null.
 */
static void
dissect_lsp_ext_ip_reachability_clv(tvbuff_t *tvb, proto_tree *tree,
	int offset, int id_length _U_, int length)
{
	proto_item *pi = NULL;
	proto_tree *subtree = NULL;
	proto_tree *subtree2 = NULL;
	guint8     ctrl_info;
	guint8     bit_length, byte_length;
	guint8     prefix [4];
	guint32    metric;
	guint8     len,i;
	guint8     subclvs_len;
	guint8     clv_code, clv_len;

	if (!tree) return;

	while (length > 0) {
		memset (prefix, 0, 4);
		ctrl_info = tvb_get_guint8(tvb, offset+4);
		bit_length = ctrl_info & 0x3f;
		byte_length = (bit_length + 7) / 8;
		tvb_memcpy (tvb, prefix, offset+5, byte_length);
		metric = tvb_get_ntohl(tvb, offset);
		subclvs_len = 0;
		if ((ctrl_info & 0x40) != 0)
                        subclvs_len = 1+tvb_get_guint8(tvb, offset+5+byte_length);

		pi = proto_tree_add_text (tree, tvb, offset, 5+byte_length+subclvs_len,
                                          "IPv4 prefix: %s/%d, Metric: %u, Distribution: %s, %ssub-TLVs present",
                                          ip_to_str (prefix),
                                          bit_length,
                                          metric,
                                          ((ctrl_info & 0x80) == 0) ? "up" : "down",
                                          ((ctrl_info & 0x40) == 0) ? "no " : "" );

                /* open up a new tree per prefix */
		subtree = proto_item_add_subtree (pi, ett_isis_lsp_part_of_clv_ext_ip_reachability);

		proto_tree_add_text (subtree, tvb, offset+5, byte_length, "IPv4 prefix: %s/%u",
                                     ip_to_str (prefix),
                                     bit_length);

		proto_tree_add_text (subtree, tvb, offset, 4, "Metric: %u", metric);

		proto_tree_add_text (subtree, tvb, offset+4, 1, "Distribution: %s",
                                     ((ctrl_info & 0x80) == 0) ? "up" : "down");

		len = 5 + byte_length;
		if ((ctrl_info & 0x40) != 0) {
                        subclvs_len = tvb_get_guint8(tvb, offset+len);
                        pi = proto_tree_add_text (subtree, tvb, offset+len, 1, "sub-TLVs present, total length: %u bytes",
                                             subclvs_len);
                        proto_item_set_len (pi, subclvs_len+1);
                        /* open up a new tree for the subTLVs */
                        subtree2 = proto_item_add_subtree (pi, ett_isis_lsp_clv_ip_reach_subclv);

                        i =0;
                        while (i < subclvs_len) {
				clv_code = tvb_get_guint8(tvb, offset+len+1); /* skip the total subtlv len indicator */
				clv_len  = tvb_get_guint8(tvb, offset+len+2);
                                
                                /*
                                 * we pass on now the raw data to the ipreach_subtlv dissector
                                 * therefore we need to skip 3 bytes
                                 * (total subtlv len, subtlv type, subtlv len)
                                 */
                                dissect_ipreach_subclv(tvb, subtree2, offset+len+3, clv_code, clv_len);
                                i += clv_len + 2;
                        }
                        len += 1 + subclvs_len;
                } else {
                        proto_tree_add_text (subtree, tvb, offset+4, 1, "no sub-TLVs present");
                        proto_item_set_len (pi, len);
                }

		offset += len;
		length -= len;
	}
}

/*
 * Name: dissect_lsp_ipv6_reachability_clv()
 *
 * Description: Decode an IPv6 reachability CLV - code 236.
 *
 *   CALLED BY TLV 237 DISSECTOR
 *
 * Input:
 *   tvbuff_t * : tvbuffer for packet data
 *   proto_tree * : proto tree to build on (may be null)
 *   int : current offset into packet data
 *   int : length of IDs in packet.
 *   int : length of this clv
 *
 * Output:
 *   void, will modify proto_tree if not null.
 */
static void
dissect_lsp_ipv6_reachability_clv(tvbuff_t *tvb, proto_tree *tree, int offset,
	int id_length _U_, int length)
{
	proto_item        *pi;
	proto_tree        *subtree = NULL;
	proto_tree        *subtree2 = NULL;
	guint8            ctrl_info;
	guint8            bit_length, byte_length;
	struct e_in6_addr prefix;
	guint32           metric;
	guint8            len,i;
	guint8            subclvs_len;
	guint8            clv_code, clv_len;

	if (!tree) return;

	memset (prefix.s6_addr, 0, 16);

	while (length > 0) {
		ctrl_info = tvb_get_guint8(tvb, offset+4);
		bit_length = tvb_get_guint8(tvb, offset+5);
		byte_length = (bit_length + 7) / 8;
		tvb_memcpy (tvb, prefix.s6_addr, offset+6, byte_length);
		metric = tvb_get_ntohl(tvb, offset);
		subclvs_len = 0;
		if ((ctrl_info & 0x20) != 0)
                        subclvs_len = 1+tvb_get_guint8(tvb, offset+6+byte_length);

		pi = proto_tree_add_text (tree, tvb, offset, 6+byte_length+subclvs_len,
                                          "IPv6 prefix: %s/%u, Metric: %u, Distribution: %s, %s, %ssub-TLVs present",
                                          ip6_to_str (&prefix),
                                          bit_length,
                                          metric,
                                          ((ctrl_info & 0x80) == 0) ? "up" : "down",
                                          ((ctrl_info & 0x40) == 0) ? "internal" : "external",
                                          ((ctrl_info & 0x20) == 0) ? "no " : "" );

		subtree = proto_item_add_subtree (pi, ett_isis_lsp_part_of_clv_ipv6_reachability);

		proto_tree_add_text (subtree, tvb, offset+6, byte_length, "IPv6 prefix: %s/%u",
                                     ip6_to_str (&prefix),
                                     bit_length);

		proto_tree_add_text (subtree, tvb, offset, 4,
			"Metric: %u", metric);

		proto_tree_add_text (subtree, tvb, offset+4, 1,
			"Distribution: %s, %s",
			((ctrl_info & 0x80) == 0) ? "up" : "down",
			((ctrl_info & 0x40) == 0) ? "internal" : "external" );

		if ((ctrl_info & 0x1f) != 0) {
			proto_tree_add_text (subtree, tvb, offset+4, 1,
					     "Reserved bits: 0x%x",
					     (ctrl_info & 0x1f) );
                }

		len = 6 + byte_length;
		if ((ctrl_info & 0x20) != 0) {
                        subclvs_len = tvb_get_guint8(tvb, offset+len);
                        pi = proto_tree_add_text (subtree, tvb, offset+len, 1, "sub-TLVs present, total length: %u bytes",
                                             subclvs_len);
                        proto_item_set_len (pi, subclvs_len+1);
                        /* open up a new tree for the subTLVs */
                        subtree2 = proto_item_add_subtree (pi, ett_isis_lsp_clv_ip_reach_subclv);

                        i =0;
                        while (i < subclvs_len) {
				clv_code = tvb_get_guint8(tvb, offset+len+1); /* skip the total subtlv len indicator */
				clv_len  = tvb_get_guint8(tvb, offset+len+2);
                                dissect_ipreach_subclv(tvb, subtree2, offset+len+3, clv_code, clv_len);
                                i += clv_len + 2;
                        }
                        len += 1 + subclvs_len;
                } else {
                        proto_tree_add_text (subtree, tvb, offset+4, 1, "no sub-TLVs present");
                        proto_item_set_len (pi, len);
                }
		offset += len;
		length -= len;
	}
}

/*
 * Name: dissect_lsp_nlpid_clv()
 *
 * Description:
 *	Decode for a lsp packets NLPID clv.  Calls into the
 *	clv common one.
 *
 * Input:
 *	tvbuff_t * : tvbuffer for packet data
 *	proto_tree * : proto tree to build on (may be null)
 *	int : current offset into packet data
 *	int : length of IDs in packet.
 *	int : length of this clv
 *
 * Output:
 *	void, will modify proto_tree if not null.
 */
static void
dissect_lsp_nlpid_clv(tvbuff_t *tvb, proto_tree *tree, int offset,
	int id_length _U_, int length)
{
	isis_dissect_nlpid_clv(tvb, tree, offset, length);
}

/*
 * Name: dissect_lsp_mt_clv()
 *
 * Description: - code 229
 *	Decode for a lsp packets Multi Topology clv.  Calls into the
 *	clv common one.
 *
 * Input:
 *      tvbuff_t * : tvbuffer for packet data
 *      proto_tree * : proto tree to build on (may be null)
 *	int : current offset into packet data
 *	guint : length of this clv
 *	int : length of IDs in packet.
 *
 * Output:
 *	void, will modify proto_tree if not null.
 */
static void
dissect_lsp_mt_clv(tvbuff_t *tvb, proto_tree *tree, int offset,
	int id_length _U_, int length)
{
	isis_dissect_mt_clv(tvb, tree, offset, length, hf_isis_lsp_clv_mt );
}

/*
 * Name: dissect_lsp_hostname_clv()
 *
 * Description:
 *      Decode for a lsp packets hostname clv.  Calls into the
 *      clv common one.
 *
 * Input:
 *      tvbuff_t * : tvbuffer for packet data
 *      proto_tree * : proto tree to build on (may be null)
 *      int : current offset into packet data
 *      int : length of IDs in packet.
 *      int : length of this clv
 *
 * Output:
 *      void, will modify proto_tree if not null.
 */
static void
dissect_lsp_hostname_clv(tvbuff_t *tvb, proto_tree *tree, int offset,
	int id_length _U_, int length)
{
        isis_dissect_hostname_clv(tvb, tree, offset, length);
}


/*
 * Name: dissect_lsp_te_router_id_clv()
 *
 * Description:
 *      Decode for a lsp packets Traffic Engineering ID clv.  Calls into the
 *      clv common one.
 *
 * Input:
 *      tvbuff_t * : tvbuffer for packet data
 *      proto_tree * : proto tree to build on (may be null)
 *      int : current offset into packet data
 *      int : length of IDs in packet.
 *      int : length of this clv
 *
 * Output:
 *      void, will modify proto_tree if not null.
 */
static void
dissect_lsp_te_router_id_clv(tvbuff_t *tvb, proto_tree *tree, int offset,
	int id_length _U_, int length)
{
        isis_dissect_te_router_id_clv(tvb, tree, offset, length,
                hf_isis_lsp_clv_te_router_id );
}


/*
 * Name: dissect_lsp_ip_int_addr_clv()
 *
 * Description:
 *	Decode for a lsp packets ip interface addr clv.  Calls into the
 *	clv common one.
 *
 * Input:
 *	tvbuff_t * : tvbuffer for packet data
 *	proto_tree * : proto tree to build on (may be null)
 *	int : current offset into packet data
 *	int : length of IDs in packet.
 *	int : length of this clv
 *
 * Output:
 *	void, will modify proto_tree if not null.
 */
static void
dissect_lsp_ip_int_addr_clv(tvbuff_t *tvb, proto_tree *tree, int offset,
	int id_length _U_, int length)
{
	isis_dissect_ip_int_clv(tvb, tree, offset, length,
		hf_isis_lsp_clv_ipv4_int_addr );
}

/*
 * Name: dissect_lsp_ipv6_int_addr_clv()
 *
 * Description: Decode an IPv6 interface addr CLV - code 232.
 *
 *   Calls into the clv common one.
 *
 * Input:
 *   tvbuff_t * : tvbuffer for packet data
 *   proto_tree * : proto tree to build on (may be null)
 *   int : current offset into packet data
 *   int : length of IDs in packet.
 *   int : length of this clv
 *
 * Output:
 *   void, will modify proto_tree if not null.
 */
static void
dissect_lsp_ipv6_int_addr_clv(tvbuff_t *tvb, proto_tree *tree, int offset,
	int id_length _U_, int length)
{
	isis_dissect_ipv6_int_clv(tvb, tree, offset, length,
		hf_isis_lsp_clv_ipv6_int_addr );
}

/*
 * Name: dissect_lsp_L1_auth_clv()
 *
 * Description:
 *	Decode for a lsp packets authenticaion clv.  Calls into the
 *	clv common one.  An auth inside a L1 LSP is a per area password
 *
 * Input:
 *	tvbuff_t * : tvbuffer for packet data
 *	proto_tree * : proto tree to build on (may be null)
 *	int : current offset into packet data
 *	int : length of IDs in packet.
 *	int : length of this clv
 *
 * Output:
 *	void, will modify proto_tree if not null.
 */
static void
dissect_lsp_l1_auth_clv(tvbuff_t *tvb, proto_tree *tree, int offset,
	int id_length _U_, int length)
{
	isis_dissect_authentication_clv(tvb, tree, offset, length,
		"Per area authentication" );
}

/*
 * Name: dissect_lsp_L2_auth_clv()
 *
 * Description:
 *	Decode for a lsp packets authenticaion clv.  Calls into the
 *	clv common one.  An auth inside a L2 LSP is a per domain password
 *
 * Input:
 *	tvbuff_t * : tvbuffer for packet data
 *	proto_tree * : proto tree to build on (may be null)
 *	int : current offset into packet data
 *	int : length of IDs in packet.
 *	int : length of this clv
 *
 * Output:
 *	void, will modify proto_tree if not null.
 */
static void
dissect_lsp_l2_auth_clv(tvbuff_t *tvb, proto_tree *tree, int offset,
	int id_length _U_, int length)
{
	isis_dissect_authentication_clv(tvb, tree, offset, length,
		"Per domain authentication" );
}

/*
 * Name: dissect_lsp_area_address_clv()
 *
 * Description:
 *	Decode for a lsp packet's area address clv.  Call into clv common
 *	one.
 *
 * Input:
 *	tvbuff_t * : tvbuffer for packet data
 *	proto_tree * : protocol display tree to fill out.  May be NULL
 *	int : offset into packet data where we are.
 *	int : length of IDs in packet.
 *	int : length of clv we are decoding
 *
 * Output:
 *      void, but we will add to proto tree if !NULL.
 */
static void
dissect_lsp_area_address_clv(tvbuff_t *tvb, proto_tree *tree, int offset,
	int id_length _U_, int length)
{
	isis_dissect_area_address_clv(tvb, tree, offset, length);
}

/*
 * Name: dissect_lsp_eis_neighbors_clv_inner()
 *
 * Description:
 *	Real work horse for showing neighbors.  This means we decode the
 *	first octet as either virtual/!virtual (if show_virtual param is
 *	set), or as a must == 0 reserved value.
 *
 *	Once past that, we decode n neighbor elements.  Each neighbor
 *	is comprised of a metric block (is dissect_metric) and the
 *	addresses.
 *
 * Input:
 *	tvbuff_t * : tvbuffer for packet data
 *	proto_tree * : protocol display tree to fill out.  May be NULL
 *	int : offset into packet data where we are.
 *	int : length of IDs in packet.
 *	int : length of clv we are decoding
 *	int : set to decode first octet as virtual vs reserved == 0
 *	int : set to indicate EIS instead of IS (6 octet per addr instead of 7)
 *
 * Output:
 *      void, but we will add to proto tree if !NULL.
 */
static void
dissect_lsp_eis_neighbors_clv_inner(tvbuff_t *tvb, proto_tree *tree,
	int offset, int length, int id_length, int show_virtual, int is_eis)
{
	proto_item 	*ti;
	proto_tree	*ntree = NULL;
	int		tlen;

	if (!is_eis) {
		id_length++;	/* IDs are one octet longer in IS neighbours */
		if ( tree ) {
			if ( show_virtual ) {
				/* virtual path flag */
				proto_tree_add_text ( tree, tvb, offset, 1,
				   tvb_get_guint8(tvb, offset) ? "IsVirtual" : "IsNotVirtual" );
			} else {
				proto_tree_add_text ( tree, tvb, offset, 1,
					"Reserved value 0x%02x, must == 0",
					tvb_get_guint8(tvb, offset)  );
			}
		}
		offset++;
		length--;
	}
	tlen = 4 + id_length;

	while ( length > 0 ) {
		if (length<tlen) {
			isis_dissect_unknown(tvb, tree, offset,
				"short E/IS reachability (%d vs %d)", length,
				tlen );
			return;
		}
		/*
		 * Gotta build a sub-tree for all our pieces
		 */
		if ( tree ) {
			if ( is_eis ) {
				ti = proto_tree_add_text(tree, tvb, offset, tlen,
					"ES Neighbor: %s",
				print_system_id( tvb_get_ptr(tvb, offset+4, id_length), id_length ) );
			} else {
				ti = proto_tree_add_text(tree, tvb, offset, tlen,
					"IS Neighbor:  %s",
				print_system_id(tvb_get_ptr(tvb, offset+4, id_length), id_length ) );
			}
			ntree = proto_item_add_subtree(ti,
				ett_isis_lsp_clv_is_neighbors);



                        proto_tree_add_text (ntree, tvb, offset, 1,
					     "Default Metric: %d, %s",
					     ISIS_LSP_CLV_METRIC_VALUE(tvb_get_guint8(tvb, offset)),
					     ISIS_LSP_CLV_METRIC_IE(tvb_get_guint8(tvb, offset)) ? "External" : "Internal");

			if (ISIS_LSP_CLV_METRIC_SUPPORTED(tvb_get_guint8(tvb, offset+1))) {
                          proto_tree_add_text (ntree, tvb, offset+1, 1, "Delay Metric:   Not supported");
			} else {
                          proto_tree_add_text (ntree, tvb, offset+1, 1, "Delay Metric:   %d, %s",
                                             ISIS_LSP_CLV_METRIC_VALUE(tvb_get_guint8(tvb, offset+1)),
					     ISIS_LSP_CLV_METRIC_IE(tvb_get_guint8(tvb, offset+1)) ? "External" : "Internal");
			}


                        if (ISIS_LSP_CLV_METRIC_SUPPORTED(tvb_get_guint8(tvb, offset+2))) {
                          proto_tree_add_text (ntree, tvb, offset+2, 1, "Expense Metric: Not supported");
                        } else {
                          proto_tree_add_text (ntree, tvb, offset+2, 1, "Expense Metric: %d, %s",
					       ISIS_LSP_CLV_METRIC_VALUE(tvb_get_guint8(tvb, offset+2)),
					       ISIS_LSP_CLV_METRIC_IE(tvb_get_guint8(tvb, offset+2)) ? "External" : "Internal");
                        }

                        if (ISIS_LSP_CLV_METRIC_SUPPORTED(tvb_get_guint8(tvb, offset+3))) {
                          proto_tree_add_text (ntree, tvb, offset+3, 1, "Error Metric:   Not supported");
                        } else {
                          proto_tree_add_text (ntree, tvb, offset+3, 1, "Error Metric:   %d, %s",
					       ISIS_LSP_CLV_METRIC_VALUE(tvb_get_guint8(tvb, offset+3)),
					       ISIS_LSP_CLV_METRIC_IE(tvb_get_guint8(tvb, offset+3)) ? "External" : "Internal");
                        }

		}
		offset += tlen;
		length -= tlen;
	}
}

/*
 * Name: dissect_lsp_l1_is_neighbors_clv()
 *
 * Description:
 *	Dispatch a l1 intermediate system neighbor by calling
 *	the inner function with show virtual set to TRUE and is es set to FALSE.
 *
 * Input:
 *	tvbuff_t * : tvbuffer for packet data
 *	proto_tree * : protocol display tree to fill out.  May be NULL
 *	int : offset into packet data where we are.
 *	int : length of IDs in packet.
 *	int : length of clv we are decoding
 *
 * Output:
 *      void, but we will add to proto tree if !NULL.
 */
static void
dissect_lsp_l1_is_neighbors_clv(tvbuff_t *tvb, proto_tree *tree, int offset,
	int id_length, int length)
{
	dissect_lsp_eis_neighbors_clv_inner(tvb, tree, offset,
		length, id_length, TRUE, FALSE);
}

/*
 * Name: dissect_lsp_l1_es_neighbors_clv()
 *
 * Description:
 *	Dispatch a l1 end or intermediate system neighbor by calling
 *	the inner function with show virtual set to TRUE and es set to TRUE.
 *
 * Input:
 *	tvbuff_t * : tvbuffer for packet data
 *	proto_tree * : protocol display tree to fill out.  May be NULL
 *	int : offset into packet data where we are.
 *	int : length of IDs in packet.
 *	int : length of clv we are decoding
 *
 * Output:
 *      void, but we will add to proto tree if !NULL.
 */
static void
dissect_lsp_l1_es_neighbors_clv(tvbuff_t *tvb, proto_tree *tree, int offset,
	int id_length, int length)
{
	dissect_lsp_eis_neighbors_clv_inner(tvb, tree, offset,
		length, id_length, TRUE, TRUE);
}

/*
 * Name: dissect_lsp_l2_is_neighbors_clv()
 *
 * Description:
 *	Dispatch a l2 intermediate system neighbor by calling
 *	the inner function with show virtual set to FALSE, and is es set
 *	to FALSE
 *
 * Input:
 *	tvbuff_t * : tvbuffer for packet data
 *	proto_tree * : protocol display tree to fill out.  May be NULL
 *	int : offset into packet data where we are.
 *	int : length of IDs in packet.
 *	int : length of clv we are decoding
 *
 * Output:
 *      void, but we will add to proto tree if !NULL.
 */
static void
dissect_lsp_l2_is_neighbors_clv(tvbuff_t *tvb, proto_tree *tree, int offset,
	int id_length, int length)
{
	dissect_lsp_eis_neighbors_clv_inner(tvb, tree, offset,
		length, id_length, FALSE, FALSE);
}


/*
 * Name: dissect_subclv_admin_group ()
 *
 * Description: Called by function dissect_lsp_ext_is_reachability_clv().
 *
 *   This function is called by dissect_lsp_ext_is_reachability_clv()
 *   for dissect the administrive group sub-CLV (code 3).
 *
 * Input:
 *   tvbuff_t * : tvbuffer for packet data
 *   proto_tree * : protocol display tree to fill out.
 *   int : offset into packet data where we are (beginning of the sub_clv value).
 *
 * Output:
 *   void
 */
static void
dissect_subclv_admin_group (tvbuff_t *tvb, proto_tree *tree, int offset) {
	proto_item *ti;
	proto_tree *ntree;
	guint32    clv_value;
	guint32    mask;
	int        i;

	ti = proto_tree_add_text(tree, tvb, offset-2, 6, "Administrative group(s):");
	ntree = proto_item_add_subtree (ti, ett_isis_lsp_subclv_admin_group);

	clv_value = tvb_get_ntohl(tvb, offset);
	mask = 1;
	for (i = 0 ; i < 32 ; i++) {
		if ( (clv_value & mask) != 0 ) {
			proto_tree_add_text (ntree, tvb, offset, 4, "group %d", i);
		}
		mask <<= 1;
	}
}

/*
 * Name: dissect_subclv_max_bw ()
 *
 * Description: Called by function dissect_lsp_ext_is_reachability_clv().
 *
 *   This function is called by dissect_lsp_ext_is_reachability_clv()
 *   for dissect the maximum link bandwidth sub-CLV (code 9).
 *
 * Input:
 *   tvbuff_t * : tvbuffer for packet data
 *   proto_tree * : protocol display tree to fill out.
 *   int : offset into packet data where we are (beginning of the sub_clv value).
 *
 * Output:
 *   void
 */
static void
dissect_subclv_max_bw(tvbuff_t *tvb, proto_tree *tree, int offset)
{
	gfloat  bw;

	bw = tvb_get_ntohieee_float(tvb, offset);
	proto_tree_add_text (tree, tvb, offset-2, 6,
		"Maximum link bandwidth : %.2f Mbps", bw*8/1000000 );
}

/*
 * Name: dissect_subclv_rsv_bw ()
 *
 * Description: Called by function dissect_lsp_ext_is_reachability_clv().
 *
 *   This function is called by dissect_lsp_ext_is_reachability_clv()
 *   for dissect the reservable link bandwidth sub-CLV (code 10).
 *
 * Input:
 *   tvbuff_t * : tvbuffer for packet data
 *   proto_tree * : protocol display tree to fill out.
 *   int : offset into packet data where we are (beginning of the sub_clv value).
 *
 * Output:
 *   void
 */
static void
dissect_subclv_rsv_bw(tvbuff_t *tvb, proto_tree *tree, int offset)
{
	gfloat  bw;

	bw = tvb_get_ntohieee_float(tvb, offset);
	proto_tree_add_text (tree, tvb, offset-2, 6,
		"Reservable link bandwidth: %.2f Mbps", bw*8/1000000 );
}

/*
 * Name: dissect_subclv_unrsv_bw ()
 *
 * Description: Called by function dissect_lsp_ext_is_reachability_clv().
 *
 *   This function is called by dissect_lsp_ext_is_reachability_clv()
 *   for dissect the unreserved bandwidth sub-CLV (code 11).
 *
 * Input:
 *   tvbuff_t * : tvbuffer for packet data
 *   proto_tree * : protocol display tree to fill out.
 *   int : offset into packet data where we are (beginning of the sub_clv value).
 *
 * Output:
 *   void
 */
static void
dissect_subclv_unrsv_bw(tvbuff_t *tvb, proto_tree *tree, int offset)
{
	proto_item *ti;
	proto_tree *ntree;
	gfloat     bw;
	int        i;

	ti = proto_tree_add_text (tree, tvb, offset-2, 34, "Unreserved bandwidth:");
	ntree = proto_item_add_subtree (ti, ett_isis_lsp_subclv_unrsv_bw);

	for (i = 0 ; i < 8 ; i++) {
		bw = tvb_get_ntohieee_float(tvb, offset+4*i);
		proto_tree_add_text (ntree, tvb, offset+4*i, 4,
			"priority level %d: %.2f Mbps", i, bw*8/1000000 );
	}
}

/*
 * Name: dissect_lsp_ext_is_reachability_clv()
 *
 * Description: Decode a Extended IS Reachability CLV - code 22
 *
 *   The extended IS reachability TLV is an extended version
 *   of the IS reachability TLV (code 2). It encodes the metric
 *   as a 24-bit unsigned interger and allows to add sub-CLV(s).
 *
 *   CALLED BY TLV 222 DISSECTOR
 *
 * Input:
 *   tvbuff_t * : tvbuffer for packet data
 *   proto_tree * : protocol display tree to fill out.  May be NULL
 *   int : offset into packet data where we are.
 *   int : length of IDs in packet.
 *   int : length of clv we are decoding
 *
 * Output:
 *   void, but we will add to proto tree if !NULL.
 */
static void
dissect_lsp_ext_is_reachability_clv(tvbuff_t *tvb, proto_tree *tree,
	int offset, int id_length _U_, int length)
{
	proto_item *ti;
	proto_tree *ntree = NULL;
	guint8     subclvs_len;
	guint8     len, i;
	guint8     clv_code, clv_len;

	if (!tree) return;

	while (length > 0) {
		ti = proto_tree_add_text (tree, tvb, offset, -1,
			"IS neighbor: %s",
			print_system_id (tvb_get_ptr(tvb, offset, 7), 7) );
		ntree = proto_item_add_subtree (ti,
			ett_isis_lsp_part_of_clv_ext_is_reachability );

		proto_tree_add_text (ntree, tvb, offset+7, 3,
			"Metric: %d", tvb_get_ntoh24(tvb, offset+7) );

		subclvs_len = tvb_get_guint8(tvb, offset+10);
		if (subclvs_len == 0) {
			proto_tree_add_text (ntree, tvb, offset+10, 1, "no sub-TLVs present");
		}
		else {
			i = 0;
			while (i < subclvs_len) {
				clv_code = tvb_get_guint8(tvb, offset+11+i);
				clv_len  = tvb_get_guint8(tvb, offset+12+i);
				switch (clv_code) {
				case 3 :
					dissect_subclv_admin_group(tvb, ntree, offset+13+i);
					break;
				case 6 :
					proto_tree_add_text (ntree, tvb, offset+11+i, 6,
						"IPv4 interface address: %s", ip_to_str (tvb_get_ptr(tvb, offset+13+i, 4)) );
					break;
				case 8 :
					proto_tree_add_text (ntree, tvb, offset+11+i, 6,
						"IPv4 neighbor address: %s", ip_to_str (tvb_get_ptr(tvb, offset+13+i, 4)) );
					break;
				case 9 :
					dissect_subclv_max_bw (tvb, ntree, offset+13+i);
					break;
				case 10:
					dissect_subclv_rsv_bw (tvb, ntree, offset+13+i);
					break;
				case 11:
					dissect_subclv_unrsv_bw (tvb, ntree, offset+13+i);
					break;
				case 18:
					proto_tree_add_text (ntree, tvb, offset+11+i, 5,
						"Traffic engineering default metric: %d",
						tvb_get_ntoh24(tvb, offset+13+i) );
					break;
				case 250:
				case 251:
				case 252:
				case 253:
				case 254:
					proto_tree_add_text (ntree, tvb, offset+11+i, clv_len+2,
						"Unknown Cisco specific extensions: code %d, length %d",
						clv_code, clv_len );
					break;
				default :
					proto_tree_add_text (ntree, tvb, offset+11+i, clv_len+2,
						"Unknown sub-CLV: code %d, length %d", clv_code, clv_len );
					break;
				}
				i += clv_len + 2;
			}
		}

		len = 11 + subclvs_len;
		proto_item_set_len (ti, len);
		offset += len;
		length -= len;
	}
}

/*
 * Name: dissect_lsp_mt_reachable_IPv4_prefx_clv()
 *
 * Description: Decode Multi-Topology IPv4 Prefixes - code 235
 *
 *
 * Input:
 *   tvbuff_t * : tvbuffer for packet data
 *   proto_tree * : protocol display tree to fill out.  May be NULL
 *   int : offset into packet data where we are.
 *   int : length of IDs in packet.
 *   int : length of clv we are decoding
 *
 * Output:
 *   void, but we will add to proto tree if !NULL.
 */
static void
dissect_lsp_mt_reachable_IPv4_prefx_clv(tvbuff_t *tvb,
        proto_tree *tree, int offset, int id_length _U_, int length)
{
	if (!tree) return;
	if (length < 2) {
		isis_dissect_unknown(tvb, tree, offset,
				"short lsp multi-topology reachable IPv4 prefixes(%d vs %d)", length,
				2 );
		return;
	}
	dissect_lsp_mt_id(tvb, tree, offset);
	dissect_lsp_ext_ip_reachability_clv(tvb, tree, offset+2, 0, length-2);
}

/*
 * Name: dissect_lsp_mt_reachable_IPv6_prefx_clv()
 *
 * Description: Decode Multi-Topology IPv6 Prefixes - code 237
 *
 *
 * Input:
 *   tvbuff_t * : tvbuffer for packet data
 *   proto_tree * : protocol display tree to fill out.  May be NULL
 *   int : offset into packet data where we are.
 *   int : length of IDs in packet.
 *   int : length of clv we are decoding
 *
 * Output:
 *   void, but we will add to proto tree if !NULL.
 */
static void
dissect_lsp_mt_reachable_IPv6_prefx_clv(tvbuff_t *tvb,
        proto_tree *tree, int offset, int id_length _U_, int length)
{
	if (!tree) return;
	if (length < 2) {
		isis_dissect_unknown(tvb, tree, offset,
				"short lsp multi-topology reachable IPv6 prefixes(%d vs %d)", length,
				2 );
		return;
	}
	dissect_lsp_mt_id(tvb, tree, offset);
	dissect_lsp_ipv6_reachability_clv(tvb, tree, offset+2, 0, length-2);
}


/*
 * Name: dissect_lsp_mt_is_reachability_clv()
 *
 * Description: Decode Multi-Topology Intermediate Systems - code 222
 *
 *
 * Input:
 *   tvbuff_t * : tvbuffer for packet data
 *   proto_tree * : protocol display tree to fill out.  May be NULL
 *   int : offset into packet data where we are.
 *   int : unused
 *   int : length of clv we are decoding
 *
 * Output:
 *   void, but we will add to proto tree if !NULL.
 */

static void
dissect_lsp_mt_is_reachability_clv(tvbuff_t *tvb, proto_tree *tree, int offset,
	int id_length _U_, int length)
{
	if (!tree) return;
	if (length < 2) {
		isis_dissect_unknown(tvb, tree, offset,
				"short lsp reachability(%d vs %d)", length,
				2 );
		return;
	}

	/*
	 * the MT ID value dissection is used in other LSPs so we push it
	 * in a function
	 */
	dissect_lsp_mt_id(tvb, tree, offset);
	/*
	 * fix here. No need to parse TLV 22 (with bugs) while it is
	 * already done correctly!!
	 */
	dissect_lsp_ext_is_reachability_clv(tvb, tree, offset+2, 0, length-2);
}

/*
 * Name: dissect_lsp_partition_dis_clv()
 *
 * Description:
 *	This CLV is used to indicate which system is the designated
 *	IS for partition repair.  This means just putting out the
 *	"id_length"-octet IS.
 *
 * Input:
 *	tvbuff_t * : tvbuffer for packet data
 *	proto_tree * : protocol display tree to fill out.  May be NULL
 *	int : offset into packet data where we are.
 *	int : length of IDs in packet.
 *	int : length of clv we are decoding
 *
 * Output:
 *      void, but we will add to proto tree if !NULL.
 */
static void
dissect_lsp_partition_dis_clv(tvbuff_t *tvb, proto_tree *tree, int offset,
	int id_length, int length)
{
	if ( length < id_length ) {
		isis_dissect_unknown(tvb, tree, offset,
				"short lsp partition DIS(%d vs %d)", length,
				id_length );
		return;
	}
	/*
	 * Gotta build a sub-tree for all our pieces
	 */
	if ( tree ) {
		proto_tree_add_text ( tree, tvb, offset, id_length,
			"Partition designated L2 IS: %s",
			print_system_id( tvb_get_ptr(tvb, offset, id_length), id_length ) );
	}
	length -= id_length;
	offset += id_length;
	if ( length > 0 ){
		isis_dissect_unknown(tvb, tree, offset,
				"Long lsp partition DIS, %d left over", length );
		return;
	}
}

/*
 * Name: dissect_lsp_prefix_neighbors_clv()
 *
 * Description:
 *	The prefix CLV describes what other (OSI) networks we can reach
 *	and what their cost is.  It is built from a metric block
 *	(see dissect_metric) followed by n addresses.
 *
 * Input:
 *	tvbuff_t * : tvbuffer for packet data
 *	proto_tree * : protocol display tree to fill out.  May be NULL
 *	int : offset into packet data where we are.
 *	int : length of IDs in packet.
 *	int : length of clv we are decoding
 *
 * Output:
 *      void, but we will add to proto tree if !NULL.
 */
static void
dissect_lsp_prefix_neighbors_clv(tvbuff_t *tvb, proto_tree *tree, int offset,
	int id_length _U_, int length)
{
	char *sbuf;
	int mylen;

	if ( length < 4 ) {
		isis_dissect_unknown(tvb, tree, offset,
			"Short lsp prefix neighbors (%d vs 4)", length );
		return;
	}
	if ( tree ) {
		dissect_metric (tvb, tree, offset,
			tvb_get_guint8(tvb, offset), "Default", TRUE );
		dissect_metric (tvb, tree, offset+1,
			tvb_get_guint8(tvb, offset+1), "Delay", FALSE );
		dissect_metric (tvb, tree, offset+2,
			tvb_get_guint8(tvb, offset+2), "Expense", FALSE );
		dissect_metric (tvb, tree, offset+3,
			tvb_get_guint8(tvb, offset+3), "Error", FALSE );
	}
	offset += 4;
	length -= 4;
	while ( length > 0 ) {
		mylen = tvb_get_guint8(tvb, offset);
		length--;
		if (length<=0) {
			isis_dissect_unknown(tvb, tree, offset,
				"Zero payload space after length in prefix neighbor" );
			return;
		}
		if ( mylen > length) {
			isis_dissect_unknown(tvb, tree, offset,
				"Interal length of prefix neighbor too long (%d vs %d)",
				mylen, length );
			return;
		}

		/*
		 * Lets turn the area address into "standard" 0000.0000.etc
		 * format string.
		 */
		sbuf =  print_area( tvb_get_ptr(tvb, offset+1, mylen), mylen );
		/* and spit it out */
		if ( tree ) {
			proto_tree_add_text ( tree, tvb, offset, mylen + 1,
				"Area address (%d): %s", mylen, sbuf );
		}
		offset += mylen + 1;
		length -= mylen;	/* length already adjusted for len fld*/
	}
}

/*
 * Name: isis_dissect_isis_lsp()
 *
 * Description:
 *	Print out the LSP part of the main header and then call the CLV
 *	de-mangler with the right list of valid CLVs.
 *
 * Input:
 *	tvbuff_t * : tvbuffer for packet data
 *	proto_tree * : protocol display tree to add to.  May be NULL.
 *	int offset : our offset into packet data.
 *	int : LSP type, a la packet-isis.h ISIS_TYPE_* values
 *	int : header length of packet.
 *	int : length of IDs in packet.
 *
 * Output:
 *      void, but we will add to proto tree if !NULL.
 */
void
isis_dissect_isis_lsp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset,
	int lsp_type, int header_length, int id_length)
{
	proto_item	*ti, *to, *ta;
	proto_tree	*lsp_tree = NULL, *info_tree, *att_tree;
	guint16		pdu_length;
	guint8		lsp_info, lsp_att;
	int		len;

	if (tree) {
		ti = proto_tree_add_text(tree, tvb, offset, -1,
		    PROTO_STRING_LSP);
		lsp_tree = proto_item_add_subtree(ti, ett_isis_lsp);
	}

	pdu_length = tvb_get_ntohs(tvb, offset);
	if (tree) {
		proto_tree_add_uint(lsp_tree, hf_isis_lsp_pdu_length, tvb,
			offset, 2, pdu_length);
	}
	offset += 2;

	if (tree) {
		proto_tree_add_text(lsp_tree, tvb, offset, 2,
                                    "Remaining Lifetime: %us",
                                    tvb_get_ntohs(tvb, offset));
	}
	offset += 2;

	if (tree) {
		proto_tree_add_text(lsp_tree, tvb, offset, id_length + 2,
                                    "LSP-ID: %s",
                                    print_system_id( tvb_get_ptr(tvb, offset, id_length+2), id_length+2 ) );                
	}

	if (check_col(pinfo->cinfo, COL_INFO)) {
	    col_append_fstr(pinfo->cinfo, COL_INFO, ", LSP-ID: %s",
			print_system_id( tvb_get_ptr(tvb, offset, id_length+2), id_length+2 ) );
	}
	offset += id_length + 2;

	if (tree) {
		proto_tree_add_uint(lsp_tree, hf_isis_lsp_sequence_number, tvb,
			offset, 4,
			tvb_get_ntohl(tvb, offset));
	}
	if (check_col(pinfo->cinfo, COL_INFO)) {
		col_append_fstr(pinfo->cinfo, COL_INFO, ", Sequence: 0x%08x, Lifetime: %5us",
			tvb_get_ntohl(tvb, offset),
			tvb_get_ntohs(tvb, offset - (id_length+2+2)));
	}
	offset += 4;

	if (tree) {
		/* XXX -> we could validate the cksum here! */
		proto_tree_add_uint(lsp_tree, hf_isis_lsp_checksum, tvb,
			offset, 2, tvb_get_ntohs(tvb, offset));
	}
	offset += 2;

	if (tree) {
		/*
		 * P | ATT | HIPPITY | IS TYPE description.
		 */
		lsp_info = tvb_get_guint8(tvb, offset);
		to = proto_tree_add_text(lsp_tree, tvb, offset, 1,
			"Type block(0x%02x): Partition Repair:%d, Attached bits:%d, Overload bit:%d, IS type:%d",
			lsp_info,
			ISIS_LSP_PARTITION(lsp_info),
			ISIS_LSP_ATT(lsp_info),
			ISIS_LSP_HIPPITY(lsp_info),
			ISIS_LSP_IS_TYPE(lsp_info)
			);

		info_tree = proto_item_add_subtree(to, ett_isis_lsp_info);
		proto_tree_add_boolean(info_tree, hf_isis_lsp_p, tvb, offset, 1, lsp_info);
		ta = proto_tree_add_uint(info_tree, hf_isis_lsp_att, tvb, offset, 1, lsp_info);
		att_tree = proto_item_add_subtree(ta, ett_isis_lsp_att);
		lsp_att = ISIS_LSP_ATT(lsp_info);
		proto_tree_add_text(att_tree, tvb, offset, 1,
			  "%d... = Default metric: %s", ISIS_LSP_ATT_DEFAULT(lsp_att), ISIS_LSP_ATT_DEFAULT(lsp_att) ? "Set" : "Unset");
		proto_tree_add_text(att_tree, tvb, offset, 1,
			  ".%d.. = Delay metric: %s", ISIS_LSP_ATT_DELAY(lsp_att), ISIS_LSP_ATT_DELAY(lsp_att) ? "Set" : "Unset");
		proto_tree_add_text(att_tree, tvb, offset, 1,
			  "..%d. = Expense metric: %s", ISIS_LSP_ATT_EXPENSE(lsp_att), ISIS_LSP_ATT_EXPENSE(lsp_att) ? "Set" : "Unset");
		proto_tree_add_text(att_tree, tvb, offset, 1,
			  "...%d = Error metric: %s", ISIS_LSP_ATT_ERROR(lsp_att), ISIS_LSP_ATT_ERROR(lsp_att) ? "Set" : "Unset");
		proto_tree_add_boolean(info_tree, hf_isis_lsp_hippity, tvb, offset, 1, lsp_info);
		proto_tree_add_uint(info_tree, hf_isis_lsp_is_type, tvb, offset, 1, lsp_info);
	}
	offset += 1;

	len = pdu_length - header_length;
	if (len < 0) {
		isis_dissect_unknown(tvb, tree, offset,
			"packet header length %d went beyond packet",
			 header_length );
		return;
	}
	/*
	 * Now, we need to decode our CLVs.  We need to pass in
	 * our list of valid ones!
	 */
	if (lsp_type == ISIS_TYPE_L1_LSP){
		isis_dissect_clvs(tvb, lsp_tree, offset,
			clv_l1_lsp_opts, len, id_length,
			ett_isis_lsp_clv_unknown );
	} else {
		isis_dissect_clvs(tvb, lsp_tree, offset,
			clv_l2_lsp_opts, len, id_length,
			ett_isis_lsp_clv_unknown );
	}
}
/*
 * Name: isis_register_lsp()
 *
 * Description:
 *	Register our protocol sub-sets with protocol manager.
 *
 * Input:
 *	int : protocol index for the ISIS protocol
 *
 * Output:
 *	void
 */
void
isis_register_lsp(int proto_isis) {
	static hf_register_info hf[] = {
		{ &hf_isis_lsp_pdu_length,
		{ "PDU length",		"isis.lsp.pdu_length", FT_UINT16,
		  BASE_DEC, NULL, 0x0, "", HFILL }},

		{ &hf_isis_lsp_remaining_life,
		{ "Remaining lifetime",	"isis.lsp.remaining_life", FT_UINT16,
		  BASE_DEC, NULL, 0x0, "", HFILL }},

		{ &hf_isis_lsp_sequence_number,
		{ "Sequence number",           "isis.lsp.sequence_number",
		  FT_UINT32, BASE_HEX, NULL, 0x0, "", HFILL }},

		{ &hf_isis_lsp_checksum,
		{ "Checksum",		"isis.lsp.checksum",FT_UINT16,
		  BASE_HEX, NULL, 0x0, "", HFILL }},

		{ &hf_isis_lsp_clv_ipv4_int_addr,
		{ "IPv4 interface address", "isis.lsp.clv_ipv4_int_addr", FT_IPv4,
		   BASE_NONE, NULL, 0x0, "", HFILL }},

		{ &hf_isis_lsp_clv_ipv6_int_addr,
		{ "IPv6 interface address", "isis.lsp.clv_ipv6_int_addr", FT_IPv6,
		   BASE_NONE, NULL, 0x0, "", HFILL }},

		{ &hf_isis_lsp_clv_te_router_id,
		{ "Traffic Engineering Router ID", "isis.lsp.clv_te_router_id", FT_IPv4,
		   BASE_NONE, NULL, 0x0, "", HFILL }},

		{ &hf_isis_lsp_clv_mt,
		{ "MT-ID                     ", "isis.lsp.clv_mt",
			FT_UINT16, BASE_HEX, NULL, 0x0, "", HFILL }},

		{ &hf_isis_lsp_p,
		{ "Partition Repair",	"isis.lsp.partition_repair", FT_BOOLEAN, 8,
			TFS(&supported_string), ISIS_LSP_PARTITION_MASK,
			"If set, this router supports the optional Partition Repair function", HFILL }},

		{ &hf_isis_lsp_att,
		{ "Attachment",	"isis.lsp.att", FT_UINT8, BASE_DEC,
			NULL, ISIS_LSP_ATT_MASK,
			"", HFILL }},

		{ &hf_isis_lsp_hippity,
		{ "Overload bit",	"isis.lsp.overload", FT_BOOLEAN, 8,
			TFS(&hippity_string), ISIS_LSP_HIPPITY_MASK,
			"If set, this router will not be used by any decision process to calculate routes", HFILL }},

		{ &hf_isis_lsp_is_type,
		{ "Type of Intermediate System",	"isis.lsp.is_type", FT_UINT8, BASE_DEC,
			VALS(isis_lsp_istype_vals), ISIS_LSP_IS_TYPE_MASK,
			"", HFILL }},
	};
	static gint *ett[] = {
		&ett_isis_lsp,
		&ett_isis_lsp_info,
		&ett_isis_lsp_att,
		&ett_isis_lsp_clv_area_addr,
		&ett_isis_lsp_clv_is_neighbors,
		&ett_isis_lsp_clv_ext_is_reachability, /* CLV 22 */
		&ett_isis_lsp_part_of_clv_ext_is_reachability,
		&ett_isis_lsp_subclv_admin_group,
		&ett_isis_lsp_subclv_unrsv_bw,
		&ett_isis_lsp_clv_unknown,
		&ett_isis_lsp_clv_partition_dis,
		&ett_isis_lsp_clv_prefix_neighbors,
		&ett_isis_lsp_clv_auth,
		&ett_isis_lsp_clv_nlpid,
                &ett_isis_lsp_clv_hostname,
		&ett_isis_lsp_clv_ipv4_int_addr,
		&ett_isis_lsp_clv_ipv6_int_addr, /* CLV 232 */
		&ett_isis_lsp_clv_te_router_id,
		&ett_isis_lsp_clv_ip_reachability,
                &ett_isis_lsp_clv_ip_reach_subclv,
		&ett_isis_lsp_clv_ext_ip_reachability, /* CLV 135 */
		&ett_isis_lsp_part_of_clv_ext_ip_reachability,
		&ett_isis_lsp_clv_ipv6_reachability, /* CLV 236 */
		&ett_isis_lsp_part_of_clv_ipv6_reachability,
		&ett_isis_lsp_clv_mt,
		&ett_isis_lsp_clv_mt_is,
		&ett_isis_lsp_part_of_clv_mt_is,
    &ett_isis_lsp_clv_mt_reachable_IPv4_prefx,
    &ett_isis_lsp_clv_mt_reachable_IPv6_prefx,
	};

	proto_register_field_array(proto_isis, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));
}
