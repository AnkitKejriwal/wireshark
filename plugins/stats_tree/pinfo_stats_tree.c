/* pinfo_stats_tree.c
* Stats tree for ethernet frames
*
*  (c) 2005, Luis E. G. Ontanon <luis.ontanon@gmail.com>
*
* $Id$
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
#include "config.h"
#endif

#include <epan/stats_tree.h>

/* XXX: this belongs to to_str.c */
static const gchar* port_type_to_str (port_type type) {
	switch (type) {
		case PT_NONE:   return NULL;
		case PT_SCTP:   return "SCTP";
		case PT_TCP:	return "TCP";
		case PT_UDP:	return "UDP";
		case PT_IPX:	return "IPX";
		case PT_NCP:	return "NCP";
		case PT_EXCHG: return "FC EXCHG";
		case PT_DDP: return "DDP";
		case PT_SBCCS: return "FICON SBCCS";
	}
	
	g_assert_not_reached();
	
	return NULL;
}

/* ip host stats_tree -- basic test */
static int st_node_ip = -1;
static gchar* st_str_ip = "IP address";

extern void ip_hosts_stats_tree_init(stats_tree* st) {
	st_node_ip = create_node(st, st_str_ip, 0, TRUE);	
}

extern int ip_hosts_stats_tree_packet(stats_tree *st  , packet_info *pinfo, epan_dissect_t *edt _U_, const void *p _U_) {
	static guint8 str[128];
	
	tick_stat_node(st, st_str_ip, 0, FALSE);
	
	g_snprintf(str, sizeof(str),"%s",address_to_str(&pinfo->net_src));
	tick_stat_node(st, str, st_node_ip, FALSE);

	g_snprintf(str, sizeof(str),"%s",address_to_str(&pinfo->net_dst));
	tick_stat_node(st, str, st_node_ip, FALSE);
	
}


/* packet type stats_tree -- test pivot node */
static int st_node_ptype = -1;
static gchar* st_str_ptype = "Port Type";

extern void ptype_stats_tree_init(stats_tree* st) {
	st_node_ptype = create_pivot_node(st, st_str_ptype, 0);
}

extern void ptype_stats_tree_packet(stats_tree* st, packet_info* pinfo, epan_dissect_t *edt _U_, const void *p _U_) {
	const gchar* ptype;
	
	ptype = port_type_to_str(pinfo->ptype);

	tick_pivot(st,st_node_ptype,ptype);
}

/* packet length stats_tree -- test range node */ 
static int st_node_plen = -1;
static gchar* st_str_plen = "Packet Lenght";

extern void plen_stats_tree_init(stats_tree* st) {
	st_node_plen = create_range_node(st, st_str_plen, 0, "0-19","20-39","40-79","80-159","160-319","320-639","640-1279","1280-",NULL);
}

extern void plen_stats_tree_packet(stats_tree* st, packet_info* pinfo, epan_dissect_t *edt _U_, const void *p _U_) {
	tick_stat_node(st, st_str_plen, 0, FALSE);
	tick_range(st, st_str_plen, 0, pinfo->fd->pkt_len);
}

/* register all pinfo trees */
extern void register_pinfo_stat_trees(void) {
	register_stats_tree("ip","ip_hosts",st_str_ip, ip_hosts_stats_tree_packet, ip_hosts_stats_tree_init );
	register_stats_tree("ip","ptype",st_str_ptype, ptype_stats_tree_packet, ptype_stats_tree_init );
	register_stats_tree("frame","plen",st_str_plen, plen_stats_tree_packet, plen_stats_tree_init );
}
 
