/* packet-isis-lsp.h
 * Defines and such for LSP and their CLV decodes
 *
 * $Id: packet-isis-lsp.h,v 1.16 2003/03/27 19:42:33 guy Exp $
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

#ifndef _PACKET_ISIS_LSP_H
#define _PACKET_ISIS_LSP_H

/*
 * Declarations for L1/L2 LSP base header.
 */

/* P | ATT | HIPPITY | DS FIELD description */
#define ISIS_LSP_PARTITION_MASK     0x80
#define ISIS_LSP_PARTITION_SHIFT    7
#define ISIS_LSP_PARTITION(info)    (((info) & ISIS_LSP_PARTITION_MASK) >> ISIS_LSP_PARTITION_SHIFT)

#define ISIS_LSP_ATT_MASK     0x78
#define ISIS_LSP_ATT_SHIFT    3
#define ISIS_LSP_ATT(info)    (((info) & ISIS_LSP_ATT_MASK) >> ISIS_LSP_ATT_SHIFT)

#define ISIS_LSP_ATT_DEFAULT(info)    ((info) >> 3)
#define ISIS_LSP_ATT_DELAY(info)      (((info) >> 2) & 1)
#define ISIS_LSP_ATT_EXPENSE(info)    (((info) >> 1) & 1)
#define ISIS_LSP_ATT_ERROR(info)      ((info) & 1)

#define ISIS_LSP_HIPPITY_MASK     0x04
#define ISIS_LSP_HIPPITY_SHIFT    2
#define ISIS_LSP_HIPPITY(info)    (((info) & ISIS_LSP_HIPPITY_MASK) >> ISIS_LSP_HIPPITY_SHIFT)

#define ISIS_LSP_IS_TYPE_MASK     0x03
#define ISIS_LSP_IS_TYPE(info)    ((info) & ISIS_LSP_IS_TYPE_MASK)



#define ISIS_LSP_TYPE_UNUSED0		0
#define ISIS_LSP_TYPE_LEVEL_1		1
#define ISIS_LSP_TYPE_UNUSED2		2
#define ISIS_LSP_TYPE_LEVEL_2		3

#define ISIS_LSP_ATTACHED_NONE    0
#define ISIS_LSP_ATTACHED_DEFAULT 1
#define ISIS_LSP_ATTACHED_DELAY   2
#define ISIS_LSP_ATTACHED_EXPENSE 4
#define ISIS_LSP_ATTACHED_ERROR   8


#define ISIS_LSP_CLV_METRIC_SUPPORTED(x)	((x)&0x80)
#define ISIS_LSP_CLV_METRIC_IE(x)               ((x)&0x40)
#define ISIS_LSP_CLV_METRIC_RESERVED(x)		((x)&0x40)
#define ISIS_LSP_CLV_METRIC_UPDOWN(x)           ((x)&0x80)
#define ISIS_LSP_CLV_METRIC_VALUE(x)		((x)&0x3f)

/*
 * detail clv information on L1 lsp packets
 */
#define ISIS_CLV_L1_LSP_AREA_ADDRESS		1
#define ISIS_CLV_L1_LSP_IS_NEIGHBORS		2
#define ISIS_CLV_L1_LSP_ES_NEIGHBORS		3
#define ISIS_CLV_L1_LSP_EXT_IS_REACHABLE        22
#define ISIS_CLV_L1_LSP_IP_INT_REACHABLE	128
#define ISIS_CLV_L1_LSP_NLPID			129
#define ISIS_CLV_L1_LSP_IP_EXT_REACHABLE        130
#define ISIS_CLV_L1_LSP_IP_INTERFACE_ADDR	132
#define ISIS_CLV_L1_LSP_TE_ROUTER_ID            134
#define ISIS_CLV_L1_LSP_EXT_IP_REACHABLE        135
#define ISIS_CLV_L1_LSP_HOSTNAME                137
#define ISIS_CLV_L1_LSP_MT_IS_REACHABLE         222
#define ISIS_CLV_L1_LSP_MT                      229
#define ISIS_CLV_L1_LSP_IPv6_INTERFACE_ADDR     232
#define ISIS_CLV_L1_LSP_IPv6_REACHABLE          236
/*
 * Note, the spec say 133, but everyone seems to use 10. Any clue on why
 * this is would be appreciated!
 */
#define ISIS_CLV_L1_LSP_AUTHENTICATION_NS	10	/* non spec */
#define ISIS_CLV_L1_LSP_AUTHENTICATION		133

/*
 * detail clv information on L2 lsp packets
 */
#define ISIS_CLV_L2_LSP_AREA_ADDRESS		1
#define ISIS_CLV_L2_LSP_IS_NEIGHBORS		2
#define ISIS_CLV_L2_LSP_PARTITION_DIS		4
#define ISIS_CLV_L2_LSP_PREFIX_NEIGHBORS	5
#define ISIS_CLV_L2_LSP_EXT_IS_REACHABLE        22
#define ISIS_CLV_L2_LSP_IP_INT_REACHABLE	128
#define ISIS_CLV_L2_LSP_NLPID			129
#define ISIS_CLV_L2_LSP_IP_EXT_REACHABLE	130
#define ISIS_CLV_L2_LSP_IDRP_INFO		131
#define ISIS_CLV_L2_LSP_IP_INTERFACE_ADDR	132
#define ISIS_CLV_L2_LSP_TE_ROUTER_ID            134
#define ISIS_CLV_L2_LSP_EXT_IP_REACHABLE        135
#define ISIS_CLV_L2_LSP_HOSTNAME                137
#define ISIS_CLV_L2_LSP_MT_IS_REACHABLE         222
#define ISIS_CLV_L2_LSP_MT                      229
#define ISIS_CLV_L2_LSP_IPv6_INTERFACE_ADDR     232
#define ISIS_CLV_L2_LSP_IPv6_REACHABLE          236
/*
 * Note, the spec say 133, but everyone seems to use 10. Any clue on why
 * this is would be appreciated!
 */
#define ISIS_CLV_L2_LSP_AUTHENTICATION_NS	10  	/*non spec */
#define ISIS_CLV_L2_LSP_AUTHENTICATION		133

/*
 * Published API functions.  NOTE, this are "local" API functions and
 * are only valid from with isis decodes.
 */
extern void isis_dissect_isis_lsp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
	int offset, int hello_type, int header_length, int id_length);
extern void isis_register_lsp(int proto_isis);

#endif /* _PACKET_ISIS_LSP_H */
