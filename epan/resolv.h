/* resolv.h
 * Definitions for network object lookup
 *
 * $Id: resolv.h,v 1.10 2003/01/26 19:35:29 deniel Exp $
 *
 * Laurent Deniel <laurent.deniel@free.fr>
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

#ifndef __RESOLV_H__
#define __RESOLV_H__

#ifndef MAXNAMELEN
#define MAXNAMELEN  	64	/* max name length (hostname and port name) */
#endif

/*
 * Flag controlling what names to resolve.
 */
extern guint32 g_resolv_flags;

/* 32 types are sufficient (as are 640k of RAM) */
/* FIXME: Maybe MANUF/m, IP/i, IP6/6, IPX/x, UDP+TCP/t etc would be
   more useful/consistent */
#define RESOLV_NONE		0x0
#define RESOLV_MAC		0x1
#define RESOLV_NETWORK		0x2
#define RESOLV_TRANSPORT	0x4
#define RESOLV_ALL		0xFFFFFFFF

/* global variables */

extern gchar *g_ethers_path;
extern gchar *g_ipxnets_path;
extern gchar *g_pethers_path;
extern gchar *g_pipxnets_path;

/* Functions in resolv.c */

/* Set the flags controlling what names to resolve */
extern void resolv_set_flags(guint32 flags);

/* get_tcp_port returns the UDP port name or "%u" if not found */
extern guchar *get_udp_port(guint port);

/* get_tcp_port returns the TCP port name or "%u" if not found */
extern guchar *get_tcp_port(guint port);

/* get_sctp_port returns the SCTP port name or "%u" if not found */
extern guchar *get_sctp_port(guint port);

/* get_hostname returns the host name or "%d.%d.%d.%d" if not found */
extern guchar *get_hostname(guint addr);

/* get_hostname returns the host name, or numeric addr if not found */
struct e_in6_addr;
const guchar* get_hostname6(struct e_in6_addr *ad);

/* get_ether_name returns the logical name if found in ethers files else
   "<vendor>_%02x:%02x:%02x" if the vendor code is known else
   "%02x:%02x:%02x:%02x:%02x:%02x" */
extern guchar *get_ether_name(const guint8 *addr);

/* get_ether_name returns the logical name if found in ethers files else NULL */
extern guchar *get_ether_name_if_known(const guint8 *addr);

/* get_manuf_name returns the vendor name or "%02x:%02x:%02x" if not known */
extern const guchar *get_manuf_name(const guint8 *addr);

/* get_ipxnet_name returns the logical name if found in an ipxnets file,
 * or a string formatted with "%X" if not */
extern const guchar *get_ipxnet_name(const guint32 addr);

/* returns the ethernet address corresponding to name or NULL if not known */
extern guint8 *get_ether_addr(const guchar *name);

/* returns the ipx network corresponding to name. If name is unknown,
 * 0 is returned and 'known' is set to FALSE. On success, 'known'
 * is set to TRUE. */
guint32 get_ipxnet_addr(const guchar *name, gboolean *known);

/* adds a hostname/IP in the hash table */
extern void add_host_name(guint addr, const guchar *name);

/* add ethernet address / name corresponding to IP address  */
extern void add_ether_byip(guint ip, const guint8 *eth);

/* Translates a string representing the hostname or dotted-decimal IP address
 * into a numeric IP address value, returning TRUE if it succeeds and
 * FALSE if it fails. */
gboolean get_host_ipaddr(const char *host, guint32 *addrp);

/*
 * Translate IPv6 numeric address or FQDN hostname, into binary IPv6 address.
 * Return TRUE if we succeed and set "*addrp" to that numeric IP address;
 * return FALSE if we fail.
 */
gboolean get_host_ipaddr6(const char *host, struct e_in6_addr *addrp);

#endif /* __RESOLV_H__ */
