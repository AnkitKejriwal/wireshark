/* packet-vines.c
 * Routines for Banyan VINES protocol packet disassembly
 *
 * $Id: packet-vines.c,v 1.5 1998/12/29 04:05:36 gerald Exp $
 *
 * Don Lafontaine <lafont02@cn.ca>
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@zing.org>
 * Copyright 1998 Gerald Combs
 * Joerg Mayer <jmayer@telemation.de>
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

#include "config.h"

#include <gtk/gtk.h>

#include <stdio.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include "ethereal.h"
#include "packet.h"
#include "etypes.h"
#include "packet-vines.h"


/* AFAIK Vines FRP (Fragmentation Protocol) is used on all media except Ethernet
 * and TR (and probably FDDI) - Fragmentation on these media types is not possible
 * FIXME: Do we need to use this header with PPP too?
 */

void
dissect_vines_frp(const u_char *pd, int offset, frame_data *fd, GtkTree *tree) {
  guint8   vines_frp_ctrl, vines_frp_seqno; 
  GtkWidget *vines_frp_tree, *ti;
  gchar	frp_flags_str[32];

  /* To do: Check for {cap len,pkt len} < struct len */
  /* Avoids alignment problems on many architectures. */
  vines_frp_ctrl = pd[offset];
  vines_frp_seqno = pd[offset+1];
  
  if (check_col(fd, COL_PROTOCOL))
    col_add_str(fd, COL_PROTOCOL, "Vines FRP");
  /*
   * 1: first fragment of vines packet
   * 2: last fragment of vines packet
   * 4 ... 80: unused
   */
  switch (vines_frp_ctrl) {
  case 0:
    strcpy(frp_flags_str, "middle");
    break;
  case 1:
    strcpy(frp_flags_str, "first");
    break;
  case 2:
    strcpy(frp_flags_str, "last");
    break;
  case 3:
    strcpy(frp_flags_str, "only");
    break;
  default:
    strcpy(frp_flags_str, "please report: unknown");
    break;
  }
  
  if (tree) {
    ti = add_item_to_tree(GTK_WIDGET(tree), offset, 2,
      "Vines Fragmentation Protocol");
    vines_frp_tree = gtk_tree_new();
    add_subtree(ti, vines_frp_tree, ETT_VINES_FRP);
    add_item_to_tree(vines_frp_tree, offset,     1, "Control Flags: 0x%02x = %s fragment", vines_frp_ctrl, frp_flags_str);
    add_item_to_tree(vines_frp_tree, offset + 1, 1, "Sequence Number: 0x%02x", vines_frp_seqno);
  }

  /* Skip over header */
  offset += 2;

  /* Decode the "real" Vines now */
  dissect_vines(pd, offset, fd, tree);
}

void
dissect_vines(const u_char *pd, int offset, frame_data *fd, GtkTree *tree) 
	{
  	e_vip       viph;
  	GtkWidget *vip_tree, *ti;
/*  	gchar      tos_str[32]; */
	int  is_broadcast = 0;
	int  hops = 0;

  /* To do: check for runts, errs, etc. */
  /* Avoids alignment problems on many architectures. */
  	memcpy(&viph, &pd[offset], sizeof(e_vip));

  	viph.vip_chksum = pntohs(&pd[offset]);
  	viph.vip_pktlen = pntohs(&pd[offset+2]);
  	viph.vip_dnet = pntohl(&pd[offset+6]);
  	viph.vip_snet = pntohl(&pd[offset+12]);
  	viph.vip_dsub = pntohs(&pd[offset+10]);
  	viph.vip_ssub = pntohs(&pd[offset+16]);

    	switch (viph.vip_proto) {
       	case VIP_PROTO_IPC:
		if (check_col(fd, COL_PROTOCOL))
			col_add_str(fd, COL_PROTOCOL, "Vines IPC");
		if (check_col(fd, COL_INFO))
			col_add_fstr(fd, COL_INFO, "IPC (%02x)", viph.vip_proto);
 		break;
       	case VIP_PROTO_SPP:      
		if (check_col(fd, COL_PROTOCOL))
        		col_add_str(fd, COL_PROTOCOL, "Vines SPP");
		if (check_col(fd, COL_INFO))
			col_add_fstr(fd, COL_INFO, "SPP (%02x)", viph.vip_proto);
		break;
	case VIP_PROTO_ARP:
		if (check_col(fd, COL_PROTOCOL))
			col_add_str(fd, COL_PROTOCOL, "Vines ARP");
		if (check_col(fd, COL_INFO))
			col_add_fstr(fd, COL_INFO, "ARP (%02x)", viph.vip_proto);
		break;
	case VIP_PROTO_RTP:
		if (check_col(fd, COL_PROTOCOL))
			col_add_str(fd, COL_PROTOCOL, "Vines RTP");
		if (check_col(fd, COL_INFO))
			col_add_fstr(fd, COL_INFO, "RTP (%02x)", viph.vip_proto);
		break;
	case VIP_PROTO_ICP:
		if (check_col(fd, COL_PROTOCOL))
			col_add_str(fd, COL_PROTOCOL, "Vines ICP");
		if (check_col(fd, COL_INFO))
			col_add_fstr(fd, COL_INFO, "ICP (%02x)", viph.vip_proto);
		break;
	default:
		if (check_col(fd, COL_PROTOCOL))
			col_add_str(fd, COL_PROTOCOL, "Vines IP");
		if (check_col(fd, COL_INFO))
			col_add_fstr(fd, COL_INFO, "Unknown VIP protocol (%02x)", viph.vip_proto);
	}

			if (check_col(fd, COL_RES_NET_SRC))
  	  	col_add_fstr(fd, COL_RES_NET_SRC, "%08x.%04x", viph.vip_snet, viph.vip_ssub);
			if (check_col(fd, COL_RES_NET_DST))
    		col_add_fstr(fd, COL_RES_NET_DST, "%08x.%04x", viph.vip_dnet, viph.vip_dsub);

 	/* helpers to decode flags */
	/* FIXME: Not used yet */
 	if ((viph.vip_dnet == 0xffffffff) && (viph.vip_dsub == 0xffff)) {
 		is_broadcast = 1;
 	}
 	hops = viph.vip_tctl & 0xf; 
 
  /*
  	viph.ip_tos = IPTOS_TOS(viph.ip_tos);
  	switch (viph.ip_tos) 
  		{
    	case IPTOS_NONE:
    	  	strcpy(tos_str, "None");
    	  	break;
    	case IPTOS_LOWDELAY:
    	  	strcpy(tos_str, "Minimize delay");
    	  	break;
    	case IPTOS_THROUGHPUT:
    	  	strcpy(tos_str, "Maximize throughput");
    	  	break;
    	case IPTOS_RELIABILITY:
    	  	strcpy(tos_str, "Maximize reliability");
    	  	break;
    	case IPTOS_LOWCOST:
    	  	strcpy(tos_str, "Minimize cost");
    	  	break;
    	default:
    	  	strcpy(tos_str, "Unknon.  Malformed?");
    	  	break;
  		}
  */
  	if (tree) 
  		{
    	ti = add_item_to_tree(GTK_WIDGET(tree), offset, (viph.vip_pktlen),
      		"Vines IP");
    	vip_tree = gtk_tree_new();
    	add_subtree(ti, vip_tree, ETT_VINES);
    	add_item_to_tree(vip_tree, offset,      2, "Packet checksum: 0x%04x", viph.vip_chksum);
    	add_item_to_tree(vip_tree, offset +  2, 2, "Packet length: 0x%04x (%d)", viph.vip_pktlen, viph.vip_pktlen); 
    	add_item_to_tree(vip_tree, offset +  4, 1, "Transport control: 0x%02x",
      		viph.vip_tctl);
    	add_item_to_tree(vip_tree, offset +  5, 1, "Protocol: 0x%02x", viph.vip_proto);
  		}


  	offset += 18;
	switch (viph.vip_proto) 
		{
    	case VIP_PROTO_SPP:
		dissect_vines_spp(pd, offset, fd, tree);
    		break;
  		}
	}
#define VINES_VSPP_DATA 1
#define VINES_VSPP_ACK 5
void dissect_vines_spp(const u_char *pd, int offset, frame_data *fd, GtkTree *tree) 
	{
  	e_vspp       viph;
  	GtkWidget *vspp_tree, *ti;

  /* To do: check for runts, errs, etc. */
  /* Avoids alignment problems on many architectures. */
  	memcpy(&viph, &pd[offset], sizeof(e_vspp));

  	viph.vspp_sport = ntohs(viph.vspp_sport);
  	viph.vspp_dport = ntohs(viph.vspp_dport);
  	viph.vspp_lclid = ntohs(viph.vspp_lclid);
  	viph.vspp_rmtid = ntohs(viph.vspp_rmtid);

    switch (viph.vspp_pkttype) 
    	{
      	case VSPP_PKTTYPE_DATA:      
					if (check_col(fd, COL_PROTOCOL))
			col_add_str(fd, COL_PROTOCOL, "VSPP Data");
        	break;
      	case VSPP_PKTTYPE_DISC:      
					if (check_col(fd, COL_PROTOCOL))
			col_add_str(fd, COL_PROTOCOL, "VSPP Disconnect");
        	break;
      	case VSPP_PKTTYPE_PROBE:      
					if (check_col(fd, COL_PROTOCOL))
			col_add_str(fd, COL_PROTOCOL, "VSPP Probe");
        	break;
      	case VSPP_PKTTYPE_ACK:
					if (check_col(fd, COL_PROTOCOL))
   	    		col_add_str(fd, COL_PROTOCOL, "VSPP Ack");
		break;
      	default:
					if (check_col(fd, COL_PROTOCOL))
   	    		col_add_str(fd, COL_PROTOCOL, "VSPP Unknown");
    	}
	if (check_col(fd, COL_INFO))
       		col_add_fstr(fd, COL_INFO, "NS=%04x NR=%04x Window=%04x RID=%04x LID=%04x D=%04x S=%04x", 
			viph.vspp_seqno, viph.vspp_ack, viph.vspp_win, viph.vspp_rmtid,
       			viph.vspp_lclid, viph.vspp_dport, viph.vspp_sport);
  /*
	iph.ip_tos = IPTOS_TOS(iph.ip_tos);
	switch (iph.ip_tos)
  		{
    	case IPTOS_NONE:
    	  	strcpy(tos_str, "None");
    	  	break;
    	case IPTOS_LOWDELAY:
    	  	strcpy(tos_str, "Minimize delay");
    	  	break;
    	case IPTOS_THROUGHPUT:
    	  	strcpy(tos_str, "Maximize throughput");
    	  	break;
    	case IPTOS_RELIABILITY:
    	  	strcpy(tos_str, "Maximize reliability");
    	  	break;
    	case IPTOS_LOWCOST:
    	  	strcpy(tos_str, "Minimize cost");
    	  	break;
    	default:
    	  	strcpy(tos_str, "Unknon.  Malformed?");
    	  	break;
  		}
*/ 
  	if (tree) 
  		{
    	ti = add_item_to_tree(GTK_WIDGET(tree), offset, sizeof(viph),
      		"Vines SPP");
    	vspp_tree = gtk_tree_new();
    	add_subtree(ti, vspp_tree, ETT_VINES_SPP);
    	add_item_to_tree(vspp_tree, offset,      2, "Source port: 0x%04x", viph.vspp_sport);
    	add_item_to_tree(vspp_tree, offset+2,    2, "Destination port: 0x%04x", viph.vspp_dport); 
    	add_item_to_tree(vspp_tree, offset+4,    1, "Packet type: 0x%02x", viph.vspp_pkttype);
    	add_item_to_tree(vspp_tree, offset+5,    1, "Control: 0x%02x", viph.vspp_control);
    	add_item_to_tree(vspp_tree, offset+6,    2, "Local Connection ID: 0x%04x", viph.vspp_lclid);
    	add_item_to_tree(vspp_tree, offset+8,    2, "Remote Connection ID: 0x%04x", viph.vspp_rmtid);
    	add_item_to_tree(vspp_tree, offset+10,   2, "Sequence number: 0x%04x", viph.vspp_seqno);
    	add_item_to_tree(vspp_tree, offset+12,   2, "Ack number: 0x%04x", viph.vspp_ack);
    	add_item_to_tree(vspp_tree, offset+14,   2, "Window: 0x%04x", viph.vspp_win);
  		}

	}
