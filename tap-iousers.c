/* tap-iousers.c
 * iostat   2003 Ronnie Sahlberg
 *
 * $Id: tap-iousers.c,v 1.15 2003/09/05 01:33:40 sahlberg Exp $
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

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#include <string.h>
#include <epan/packet_info.h>
#include <epan/packet.h>
#include <epan/resolv.h>
#include "tap.h"
#include "register.h"
#include "packet-ip.h"
#include "packet-ipx.h"
#include "packet-tcp.h"
#include "packet-udp.h"
#include "packet-eth.h"
#include "packet-tr.h"
#include "packet-fc.h"
#include "packet-fddi.h"
#include <string.h>

typedef struct _io_users_t {
	char *type;
	char *filter;
	struct _io_users_item_t *items;
} io_users_t;

typedef struct _io_users_item_t {
	struct _io_users_item_t *next;
	char *name1;
	char *name2;
	address addr1;
	address addr2; 
	guint32 frames1;
	guint32 frames2;
	guint32 bytes1;
	guint32 bytes2;
} io_users_item_t;


static int
iousers_udpip_packet(io_users_t *iu, packet_info *pinfo, epan_dissect_t *edt _U_, void *vudph)
{
	e_udphdr *udph=vudph;
	char name1[256],name2[256];
	io_users_item_t *iui;
	int direction=0;

	if(udph->uh_sport>udph->uh_dport){
		direction=0;
		snprintf(name1,256,"%s:%s",address_to_str(&udph->ip_src),get_udp_port(udph->uh_sport));
		snprintf(name2,256,"%s:%s",address_to_str(&udph->ip_dst),get_udp_port(udph->uh_dport));
	} else if(udph->uh_sport<udph->uh_dport){
		direction=1;
		snprintf(name2,256,"%s:%s",address_to_str(&udph->ip_src),get_udp_port(udph->uh_sport));
		snprintf(name1,256,"%s:%s",address_to_str(&udph->ip_dst),get_udp_port(udph->uh_dport));
	} else if(CMP_ADDRESS(&udph->ip_src, &udph->ip_dst)>0){
		direction=0;
		snprintf(name1,256,"%s:%s",address_to_str(&udph->ip_src),get_udp_port(udph->uh_sport));
		snprintf(name2,256,"%s:%s",address_to_str(&udph->ip_dst),get_udp_port(udph->uh_dport));
	} else {
		direction=1;
		snprintf(name2,256,"%s:%s",address_to_str(&udph->ip_src),get_udp_port(udph->uh_sport));
		snprintf(name1,256,"%s:%s",address_to_str(&udph->ip_dst),get_udp_port(udph->uh_dport));
	}

	for(iui=iu->items;iui;iui=iui->next){
		if((!strcmp(iui->name1, name1))
		&& (!strcmp(iui->name2, name2)) ){
			break;
		}
	}

	if(!iui){
		iui=g_malloc(sizeof(io_users_item_t));
		iui->next=iu->items;
		iu->items=iui;
/*		iui->addr1=NULL;*/
		iui->name1=strdup(name1);
/*		iui->addr2=NULL;*/
		iui->name2=strdup(name2);
		iui->frames1=0;
		iui->frames2=0;
		iui->bytes1=0;
		iui->bytes2=0;
	}

	if(direction){
		iui->frames1++;
		iui->bytes1+=pinfo->fd->pkt_len;
	} else {
		iui->frames2++;
		iui->bytes2+=pinfo->fd->pkt_len;
	}

	return 1;
}


static int
iousers_tcpip_packet(io_users_t *iu, packet_info *pinfo, epan_dissect_t *edt _U_, void *vtcph)
{
	struct tcpheader *tcph=vtcph;
	char name1[256],name2[256];
	io_users_item_t *iui;
	int direction=0;

	if(tcph->th_sport>tcph->th_dport){
		direction=0;
		snprintf(name1,256,"%s:%s",address_to_str(&tcph->ip_src),get_tcp_port(tcph->th_sport));
		snprintf(name2,256,"%s:%s",address_to_str(&tcph->ip_dst),get_tcp_port(tcph->th_dport));
	} else if(tcph->th_sport<tcph->th_dport){
		direction=1;
		snprintf(name2,256,"%s:%s",address_to_str(&tcph->ip_src),get_tcp_port(tcph->th_sport));
		snprintf(name1,256,"%s:%s",address_to_str(&tcph->ip_dst),get_tcp_port(tcph->th_dport));
	} else if(CMP_ADDRESS(&tcph->ip_src, &tcph->ip_dst)>0){
		direction=0;
		snprintf(name1,256,"%s:%s",address_to_str(&tcph->ip_src),get_tcp_port(tcph->th_sport));
		snprintf(name2,256,"%s:%s",address_to_str(&tcph->ip_dst),get_tcp_port(tcph->th_dport));
	} else {
		direction=1;
		snprintf(name2,256,"%s:%s",address_to_str(&tcph->ip_src),get_tcp_port(tcph->th_sport));
		snprintf(name1,256,"%s:%s",address_to_str(&tcph->ip_dst),get_tcp_port(tcph->th_dport));
	}

	for(iui=iu->items;iui;iui=iui->next){
		if((!strcmp(iui->name1, name1))
		&& (!strcmp(iui->name2, name2)) ){
			break;
		}
	}

	if(!iui){
		iui=g_malloc(sizeof(io_users_item_t));
		iui->next=iu->items;
		iu->items=iui;
/*		iui->addr1=NULL;*/
		iui->name1=strdup(name1);
/*		iui->addr2=NULL;*/
		iui->name2=strdup(name2);
		iui->frames1=0;
		iui->frames2=0;
		iui->bytes1=0;
		iui->bytes2=0;
	}

	if(direction){
		iui->frames1++;
		iui->bytes1+=pinfo->fd->pkt_len;
	} else {
		iui->frames2++;
		iui->bytes2+=pinfo->fd->pkt_len;
	}

	return 1;
}


static int
iousers_ip_packet(io_users_t *iu, packet_info *pinfo, epan_dissect_t *edt _U_, void *vip)
{
	e_ip *iph=vip;
	address *addr1, *addr2;
	io_users_item_t *iui;

	if(CMP_ADDRESS(&iph->ip_src, &iph->ip_dst)>0){
		addr1=&iph->ip_src;
		addr2=&iph->ip_dst;
	} else {
		addr2=&iph->ip_src;
		addr1=&iph->ip_dst;
	}

	for(iui=iu->items;iui;iui=iui->next){
		if((!CMP_ADDRESS(&iui->addr1, addr1))
		&&(!CMP_ADDRESS(&iui->addr2, addr2)) ){
			break;
		}
	}

	if(!iui){
		iui=g_malloc(sizeof(io_users_item_t));
		iui->next=iu->items;
		iu->items=iui;
		COPY_ADDRESS(&iui->addr1, addr1);
		iui->name1=strdup(address_to_str(addr1));
		COPY_ADDRESS(&iui->addr2, addr2);
		iui->name2=strdup(address_to_str(addr2));
		iui->frames1=0;
		iui->frames2=0;
		iui->bytes1=0;
		iui->bytes2=0;
	}

	if(!CMP_ADDRESS(&iph->ip_dst, &iui->addr1)){
		iui->frames1++;
		iui->bytes1+=pinfo->fd->pkt_len;
	} else {
		iui->frames2++;
		iui->bytes2+=pinfo->fd->pkt_len;
	}

	return 1;
}

static int
iousers_ipx_packet(io_users_t *iu, packet_info *pinfo, epan_dissect_t *edt _U_, void *vipx)
{
	ipxhdr_t *ipxh=vipx;
	address *addr1, *addr2;
	io_users_item_t *iui;

	if(CMP_ADDRESS(&ipxh->ipx_src, &ipxh->ipx_dst)>0){
		addr1=&ipxh->ipx_src;
		addr2=&ipxh->ipx_dst;
	} else {
		addr2=&ipxh->ipx_src;
		addr1=&ipxh->ipx_dst;
	}

	for(iui=iu->items;iui;iui=iui->next){
		if((!CMP_ADDRESS(&iui->addr1, addr1))
		&&(!CMP_ADDRESS(&iui->addr2, addr2)) ){
			break;
		}
	}

	if(!iui){
		iui=g_malloc(sizeof(io_users_item_t));
		iui->next=iu->items;
		iu->items=iui;
		COPY_ADDRESS(&iui->addr1, addr1);
		iui->name1=strdup(address_to_str(addr1));
		COPY_ADDRESS(&iui->addr2, addr2);
		iui->name2=strdup(address_to_str(addr2));
		iui->frames1=0;
		iui->frames2=0;
		iui->bytes1=0;
		iui->bytes2=0;
	}

	if(!CMP_ADDRESS(&ipxh->ipx_dst, &iui->addr1)){
		iui->frames1++;
		iui->bytes1+=pinfo->fd->pkt_len;
	} else {
		iui->frames2++;
		iui->bytes2+=pinfo->fd->pkt_len;
	}

	return 1;
}

static int
iousers_fc_packet(io_users_t *iu, packet_info *pinfo, epan_dissect_t *edt _U_, void *vfc)
{
	fc_hdr *fchdr=vfc;
	address *addr1, *addr2;
	io_users_item_t *iui;

	if(CMP_ADDRESS(&fchdr->s_id, &fchdr->d_id)<0){
		addr1=&fchdr->s_id;
		addr2=&fchdr->d_id;
	} else {
		addr2=&fchdr->s_id;
		addr1=&fchdr->d_id;
	}

	for(iui=iu->items;iui;iui=iui->next){
		if((!CMP_ADDRESS(&iui->addr1, addr1))
		&&(!CMP_ADDRESS(&iui->addr2, addr2)) ){
			break;
		}
	}

	if(!iui){
		iui=g_malloc(sizeof(io_users_item_t));
		iui->next=iu->items;
		iu->items=iui;
		COPY_ADDRESS(&iui->addr1, addr1);
		iui->name1=strdup(address_to_str(addr1));
		COPY_ADDRESS(&iui->addr2, addr2);
		iui->name2=strdup(address_to_str(addr2));
		iui->frames1=0;
		iui->frames2=0;
		iui->bytes1=0;
		iui->bytes2=0;
	}

	if(!CMP_ADDRESS(&fchdr->d_id,&iui->addr1)){
		iui->frames1++;
		iui->bytes1+=pinfo->fd->pkt_len;
	} else {
		iui->frames2++;
		iui->bytes2+=pinfo->fd->pkt_len;
	}

	return 1;
}

static int
iousers_eth_packet(io_users_t *iu, packet_info *pinfo, epan_dissect_t *edt _U_, void *veth)
{
	eth_hdr *ehdr=veth;
	address *addr1, *addr2;
	io_users_item_t *iui;

	if(CMP_ADDRESS(&ehdr->src, &ehdr->dst)<0){
		addr1=&ehdr->src;
		addr2=&ehdr->dst;
	} else {
		addr2=&ehdr->src;
		addr1=&ehdr->dst;
	}

	for(iui=iu->items;iui;iui=iui->next){
		if((!CMP_ADDRESS(&iui->addr1, addr1))
		&&(!CMP_ADDRESS(&iui->addr2, addr2)) ){
			break;
		}
	}

	if(!iui){
		iui=g_malloc(sizeof(io_users_item_t));
		iui->next=iu->items;
		iu->items=iui;
		COPY_ADDRESS(&iui->addr1, addr1);
		iui->name1=strdup(address_to_str(addr1));
		COPY_ADDRESS(&iui->addr2, addr2);
		iui->name2=strdup(address_to_str(addr2));
		iui->frames1=0;
		iui->frames2=0;
		iui->bytes1=0;
		iui->bytes2=0;
	}

	if(!CMP_ADDRESS(&ehdr->dst,&iui->addr1)){
		iui->frames1++;
		iui->bytes1+=pinfo->fd->pkt_len;
	} else {
		iui->frames2++;
		iui->bytes2+=pinfo->fd->pkt_len;
	}

	return 1;
}

static int
iousers_fddi_packet(io_users_t *iu, packet_info *pinfo, epan_dissect_t *edt _U_, void *veth)
{
	fddi_hdr *ehdr=veth;
	address *addr1, *addr2;
	io_users_item_t *iui;

	if(CMP_ADDRESS(&ehdr->src, &ehdr->dst)<0){
		addr1=&ehdr->src;
		addr2=&ehdr->dst;
	} else {
		addr2=&ehdr->src;
		addr1=&ehdr->dst;
	}

	for(iui=iu->items;iui;iui=iui->next){
		if((!CMP_ADDRESS(&iui->addr1, addr1))
		&&(!CMP_ADDRESS(&iui->addr2, addr2)) ){
			break;
		}
	}

	if(!iui){
		iui=g_malloc(sizeof(io_users_item_t));
		iui->next=iu->items;
		iu->items=iui;
		COPY_ADDRESS(&iui->addr1, addr1);
		iui->name1=strdup(address_to_str(addr1));
		COPY_ADDRESS(&iui->addr2, addr2);
		iui->name2=strdup(address_to_str(addr2));
		iui->frames1=0;
		iui->frames2=0;
		iui->bytes1=0;
		iui->bytes2=0;
	}

	if(!CMP_ADDRESS(&ehdr->dst,&iui->addr1)){
		iui->frames1++;
		iui->bytes1+=pinfo->fd->pkt_len;
	} else {
		iui->frames2++;
		iui->bytes2+=pinfo->fd->pkt_len;
	}

	return 1;
}

static int
iousers_tr_packet(io_users_t *iu, packet_info *pinfo, epan_dissect_t *edt _U_, void *vtr)
{
	tr_hdr *trhdr=vtr;
	address *addr1, *addr2;
	io_users_item_t *iui;

	if(CMP_ADDRESS(&trhdr->src, &trhdr->dst)<0){
		addr1=&trhdr->src;
		addr2=&trhdr->dst;
	} else {
		addr2=&trhdr->src;
		addr1=&trhdr->dst;
	}

	for(iui=iu->items;iui;iui=iui->next){
		if((!CMP_ADDRESS(&iui->addr1, addr1))
		&&(!CMP_ADDRESS(&iui->addr2, addr2)) ){
			break;
		}
	}

	if(!iui){
		iui=g_malloc(sizeof(io_users_item_t));
		iui->next=iu->items;
		iu->items=iui;
		COPY_ADDRESS(&iui->addr1, addr1);
		iui->name1=strdup(address_to_str(addr1));
		COPY_ADDRESS(&iui->addr2, addr2);
		iui->name2=strdup(address_to_str(addr2));
		iui->frames1=0;
		iui->frames2=0;
		iui->bytes1=0;
		iui->bytes2=0;
	}

	if(!CMP_ADDRESS(&trhdr->dst,&iui->addr1)){
		iui->frames1++;
		iui->bytes1+=pinfo->fd->pkt_len;
	} else {
		iui->frames2++;
		iui->bytes2+=pinfo->fd->pkt_len;
	}

	return 1;
}

static void
iousers_draw(io_users_t *iu)
{
	io_users_item_t *iui;
	guint32 last_frames, max_frames;

	printf("================================================================================\n");
	printf("%s Conversations\n",iu->type);
	printf("Filter:%s\n",iu->filter?iu->filter:"<No Filter>");
	printf("                                               |       <-      | |       ->      | |     Total     |\n");
	printf("                                               | Frames  Bytes | | Frames  Bytes | | Frames  Bytes |\n");
	max_frames=0xffffffff;
	do {
		last_frames=0;
		for(iui=iu->items;iui;iui=iui->next){
			guint32 tot_frames;
			tot_frames=iui->frames1+iui->frames2;

			if((tot_frames>last_frames)
			&&(tot_frames<max_frames)){
				last_frames=tot_frames;
			}
		}
		for(iui=iu->items;iui;iui=iui->next){
			guint32 tot_frames;
			tot_frames=iui->frames1+iui->frames2;

			if(tot_frames==last_frames){
				printf("%-20s <-> %-20s  %6d %9d  %6d %9d  %6d %9d\n",
					iui->name1, iui->name2,
					iui->frames1, iui->bytes1,
					iui->frames2, iui->bytes2,
					iui->frames1+iui->frames2,
					iui->bytes1+iui->bytes2
				);
			}
		}
		max_frames=last_frames;
	} while(last_frames);
	printf("================================================================================\n");
}

void
iousers_init(char *optarg)
{
	char *filter=NULL;
	char *tap_type, *tap_type_name;
	static int (*packet_func)(io_users_t *, packet_info *, epan_dissect_t *, void *);
	io_users_t *iu=NULL;
	GString *error_string;

	if(!strncmp(optarg,"conv,eth",8)){
		if(optarg[8]==','){
			filter=optarg+9;
		} else {
			filter=NULL;
		}
		tap_type="eth";
		tap_type_name="Ethernet";
		packet_func=iousers_eth_packet;
	} else if(!strncmp(optarg,"conv,fc",7)){
		if(optarg[7]==','){
			filter=optarg+8;
		} else {
			filter=NULL;
		}
		tap_type="fc";
		tap_type_name="Fibre Channel";
		packet_func=iousers_fc_packet;
	} else if(!strncmp(optarg,"conv,fddi",9)){
		if(optarg[9]==','){
			filter=optarg+10;
		} else {
			filter=NULL;
		}
		tap_type="fddi";
		tap_type_name="FDDI";
		packet_func=iousers_fddi_packet;
	} else if(!strncmp(optarg,"conv,tcp",8)){
		if(optarg[8]==','){
			filter=optarg+9;
		} else {
			filter=NULL;
		}
		tap_type="tcp";
		tap_type_name="TCP";
		packet_func=iousers_tcpip_packet;
	} else if(!strncmp(optarg,"conv,udp",8)){
		if(optarg[8]==','){
			filter=optarg+9;
		} else {
			filter=NULL;
		}
		tap_type="udp";
		tap_type_name="UDP";
		packet_func=iousers_udpip_packet;
	} else if(!strncmp(optarg,"conv,tr",7)){
		if(optarg[7]==','){
			filter=optarg+8;
		} else {
			filter=NULL;
		}
		tap_type="tr";
		tap_type_name="Token Ring";
		packet_func=iousers_tr_packet;
	} else if(!strncmp(optarg,"conv,ipx",8)){
		if(optarg[8]==','){
			filter=optarg+9;
		} else {
			filter=NULL;
		}
		tap_type="ipx";
		tap_type_name="IPX";
		packet_func=iousers_ipx_packet;
	} else if(!strncmp(optarg,"conv,ip",7)){
		if(optarg[7]==','){
			filter=optarg+8;
		} else {
			filter=NULL;
		}
		tap_type="ip";
		tap_type_name="IPv4";
		packet_func=iousers_ip_packet;
	} else {
		fprintf(stderr, "tethereal: invalid \"-z conv,<type>[,<filter>]\" argument\n");
		fprintf(stderr,"   <type> must be one of\n");
		fprintf(stderr,"      \"eth\"\n");
		fprintf(stderr,"      \"fc\"\n");
		fprintf(stderr,"      \"fddi\"\n");
		fprintf(stderr,"      \"ip\"\n");
		fprintf(stderr,"      \"ipx\"\n");
		fprintf(stderr,"      \"tcp\"\n");
		fprintf(stderr,"      \"tr\"\n");
		fprintf(stderr,"      \"udp\"\n");
		exit(1);
	}


	iu=g_malloc(sizeof(io_users_t));
	iu->items=NULL;
	iu->type=tap_type_name;
	if(filter){
		iu->filter=strdup(filter);
	} else {
		iu->filter=NULL;
	}

	error_string=register_tap_listener(tap_type, iu, filter, NULL, (void*)packet_func, (void*)iousers_draw);
	if(error_string){
		if(iu->items){
			g_free(iu->items);
		}
		g_free(iu);
		fprintf(stderr, "tethereal: Couldn't register conversations tap: %s\n",
		    error_string->str);
		g_string_free(error_string, TRUE);
		exit(1);
	}

}

void
register_tap_listener_iousers(void)
{
	register_ethereal_tap("conv,", iousers_init);
}

