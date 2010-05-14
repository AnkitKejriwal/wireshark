/* capture-pcap-util.c
 * Utility routines for packet capture
 *
 * $Id$
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
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

#ifdef HAVE_LIBPCAP

#include <glib.h>

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include <wtap.h>
#include <libpcap.h>

#include "capture_ifinfo.h"
#include "capture-pcap-util.h"
#include "capture-pcap-util-int.h"

#ifndef _WIN32
#include <netinet/in.h>
#endif


/*
 * Get the data-link type for a libpcap device.
 * This works around AIX 5.x's non-standard and incompatible-with-the-
 * rest-of-the-universe libpcap.
 */
int
get_pcap_linktype(pcap_t *pch, const char *devname
#ifndef _AIX
	_U_
#endif
)
{
	int linktype;
#ifdef _AIX
	const char *ifacename;
#endif

	linktype = pcap_datalink(pch);
#ifdef _AIX

	/*
	 * The libpcap that comes with AIX 5.x uses RFC 1573 ifType values
	 * rather than DLT_ values for link-layer types; the ifType values
	 * for LAN devices are:
	 *
	 *	Ethernet	6
	 *	802.3		7
	 *	Token Ring	9
	 *	FDDI		15
	 *
	 * and the ifType value for a loopback device is 24.
	 *
	 * The AIX names for LAN devices begin with:
	 *
	 *	Ethernet		en
	 *	802.3			et
	 *	Token Ring		tr
	 *	FDDI			fi
	 *
	 * and the AIX names for loopback devices begin with "lo".
	 *
	 * (The difference between "Ethernet" and "802.3" is presumably
	 * whether packets have an Ethernet header, with a packet type,
	 * or an 802.3 header, with a packet length, followed by an 802.2
	 * header and possibly a SNAP header.)
	 *
	 * If the device name matches "linktype" interpreted as an ifType
	 * value, rather than as a DLT_ value, we will assume this is AIX's
	 * non-standard, incompatible libpcap, rather than a standard libpcap,
	 * and will map the link-layer type to the standard DLT_ value for
	 * that link-layer type, as that's what the rest of Wireshark expects.
	 *
	 * (This means the capture files won't be readable by a tcpdump
	 * linked with AIX's non-standard libpcap, but so it goes.  They
	 * *will* be readable by standard versions of tcpdump, Wireshark,
	 * and so on.)
	 *
	 * XXX - if we conclude we're using AIX libpcap, should we also
	 * set a flag to cause us to assume the time stamps are in
	 * seconds-and-nanoseconds form, and to convert them to
	 * seconds-and-microseconds form before processing them and
	 * writing them out?
	 */

	/*
	 * Find the last component of the device name, which is the
	 * interface name.
	 */
	ifacename = strchr(devname, '/');
	if (ifacename == NULL)
		ifacename = devname;

	/* See if it matches any of the LAN device names. */
	if (strncmp(ifacename, "en", 2) == 0) {
		if (linktype == 6) {
			/*
			 * That's the RFC 1573 value for Ethernet; map it
			 * to DLT_EN10MB.
			 */
			linktype = 1;
		}
	} else if (strncmp(ifacename, "et", 2) == 0) {
		if (linktype == 7) {
			/*
			 * That's the RFC 1573 value for 802.3; map it to
			 * DLT_EN10MB.
			 * (libpcap, tcpdump, Wireshark, etc. don't care if
			 * it's Ethernet or 802.3.)
			 */
			linktype = 1;
		}
	} else if (strncmp(ifacename, "tr", 2) == 0) {
		if (linktype == 9) {
			/*
			 * That's the RFC 1573 value for 802.5 (Token Ring);
			 * map it to DLT_IEEE802, which is what's used for
			 * Token Ring.
			 */
			linktype = 6;
		}
	} else if (strncmp(ifacename, "fi", 2) == 0) {
		if (linktype == 15) {
			/*
			 * That's the RFC 1573 value for FDDI; map it to
			 * DLT_FDDI.
			 */
			linktype = 10;
		}
	} else if (strncmp(ifacename, "lo", 2) == 0) {
		if (linktype == 24) {
			/*
			 * That's the RFC 1573 value for "software loopback"
			 * devices; map it to DLT_NULL, which is what's used
			 * for loopback devices on BSD.
			 */
			linktype = 0;
		}
	}
#endif

	return linktype;
}

if_info_t *
if_info_new(char *name, char *description)
{
	if_info_t *if_info;

	if_info = (if_info_t *)g_malloc(sizeof (if_info_t));
	if_info->name = g_strdup(name);
	if (description == NULL)
		if_info->description = NULL;
	else
		if_info->description = g_strdup(description);
	if_info->addrs = NULL;
	if_info->loopback = FALSE;
	return if_info;
}

void
if_info_add_address(if_info_t *if_info, struct sockaddr *addr)
{
	if_addr_t *if_addr;
	struct sockaddr_in *ai;
#ifdef INET6
	struct sockaddr_in6 *ai6;
#endif

	switch (addr->sa_family) {

	case AF_INET:
		ai = (struct sockaddr_in *)addr;
		if_addr = (if_addr_t *)g_malloc(sizeof(*if_addr));
		if_addr->ifat_type = IF_AT_IPv4;
		if_addr->addr.ip4_addr =
		    *((guint32 *)&(ai->sin_addr.s_addr));
		if_info->addrs = g_slist_append(if_info->addrs, if_addr);
		break;

#ifdef INET6
	case AF_INET6:
		ai6 = (struct sockaddr_in6 *)addr;
		if_addr = (if_addr_t *)g_malloc(sizeof(*if_addr));
		if_addr->ifat_type = IF_AT_IPv6;
		memcpy((void *)&if_addr->addr.ip6_addr,
		    (void *)&ai6->sin6_addr.s6_addr,
		    sizeof if_addr->addr.ip6_addr);
		if_info->addrs = g_slist_append(if_info->addrs, if_addr);
		break;
#endif
	}
}

#ifdef HAVE_PCAP_FINDALLDEVS
/*
 * Get all IP address information, and the loopback flag, for the given
 * interface.
 */
static void
if_info_ip(if_info_t *if_info, pcap_if_t *d)
{
	pcap_addr_t *a;

	/* Loopback flag */
	if_info->loopback = (d->flags & PCAP_IF_LOOPBACK) ? TRUE : FALSE;

	/* All addresses */
	for (a = d->addresses; a != NULL; a = a->next) {
		if (a->addr != NULL)
			if_info_add_address(if_info, a->addr);
	}
}

#ifdef HAVE_PCAP_REMOTE
GList *
get_interface_list_findalldevs_ex(const char *source,
                                  struct pcap_rmtauth *auth,
                                  int *err, char **err_str)
{
	GList  *il = NULL;
	pcap_if_t *alldevs, *dev;
	if_info_t *if_info;
	char errbuf[PCAP_ERRBUF_SIZE];

        if (pcap_findalldevs_ex((char *)source, auth, &alldevs, errbuf) == -1) {
		*err = CANT_GET_INTERFACE_LIST;
		if (err_str != NULL)
			*err_str = cant_get_if_list_error_message(errbuf);
		return NULL;
	}

	if (alldevs == NULL) {
		/*
		 * No interfaces found.
		 */
		*err = NO_INTERFACES_FOUND;
		if (err_str != NULL)
			*err_str = NULL;
		return NULL;
	}

	for (dev = alldevs; dev != NULL; dev = dev->next) {
		if_info = if_info_new(dev->name, dev->description);
		il = g_list_append(il, if_info);
		if_info_ip(if_info, dev);
	}
	pcap_freealldevs(alldevs);

	return il;
}
#endif

GList *
get_interface_list_findalldevs(int *err, char **err_str)
{
	GList  *il = NULL;
	pcap_if_t *alldevs, *dev;
	if_info_t *if_info;
	char errbuf[PCAP_ERRBUF_SIZE];

	if (pcap_findalldevs(&alldevs, errbuf) == -1) {
		*err = CANT_GET_INTERFACE_LIST;
		if (err_str != NULL)
			*err_str = cant_get_if_list_error_message(errbuf);
		return NULL;
	}

	if (alldevs == NULL) {
		/*
		 * No interfaces found.
		 */
		*err = NO_INTERFACES_FOUND;
		if (err_str != NULL)
			*err_str = NULL;
		return NULL;
	}

	for (dev = alldevs; dev != NULL; dev = dev->next) {
		if_info = if_info_new(dev->name, dev->description);
		il = g_list_append(il, if_info);
		if_info_ip(if_info, dev);
	}
	pcap_freealldevs(alldevs);

	return il;
}
#endif /* HAVE_PCAP_FINDALLDEVS */

static void
free_if_info_addr_cb(gpointer addr, gpointer user_data _U_)
{
	g_free(addr);
}

static void
free_if_cb(gpointer data, gpointer user_data _U_)
{
	if_info_t *if_info = (if_info_t *)data;

	g_free(if_info->name);
	g_free(if_info->description);

	g_slist_foreach(if_info->addrs, free_if_info_addr_cb, NULL);
	g_slist_free(if_info->addrs);
	g_free(if_info);
}

void
free_interface_list(GList *if_list)
{
	g_list_foreach(if_list, free_if_cb, NULL);
	g_list_free(if_list);
}

#if !defined(HAVE_PCAP_DATALINK_NAME_TO_VAL) || !defined(HAVE_PCAP_DATALINK_VAL_TO_NAME) || !defined(HAVE_PCAP_DATALINK_VAL_TO_DESCRIPTION)
struct dlt_choice {
	const char *name;
	const char *description;
	int	dlt;
};

#define DLT_CHOICE(code, description) { #code, description, code }
#define DLT_CHOICE_SENTINEL { NULL, NULL, 0 }

static struct dlt_choice dlt_choices[] = {
	DLT_CHOICE(DLT_NULL, "BSD loopback"),
	DLT_CHOICE(DLT_EN10MB, "Ethernet"),
	DLT_CHOICE(DLT_IEEE802, "Token ring"),
	DLT_CHOICE(DLT_ARCNET, "ARCNET"),
	DLT_CHOICE(DLT_SLIP, "SLIP"),
	DLT_CHOICE(DLT_PPP, "PPP"),
	DLT_CHOICE(DLT_FDDI, "FDDI"),
	DLT_CHOICE(DLT_ATM_RFC1483, "RFC 1483 IP-over-ATM"),
	DLT_CHOICE(DLT_RAW, "Raw IP"),
	DLT_CHOICE(DLT_SLIP_BSDOS, "BSD/OS SLIP"),
	DLT_CHOICE(DLT_PPP_BSDOS, "BSD/OS PPP"),
	DLT_CHOICE(DLT_ATM_CLIP, "Linux Classical IP-over-ATM"),
	DLT_CHOICE(DLT_PPP_SERIAL, "PPP over serial"),
	DLT_CHOICE(DLT_PPP_ETHER, "PPPoE"),
	DLT_CHOICE(DLT_C_HDLC, "Cisco HDLC"),
	DLT_CHOICE(DLT_IEEE802_11, "802.11"),
	DLT_CHOICE(DLT_FRELAY, "Frame Relay"),
	DLT_CHOICE(DLT_LOOP, "OpenBSD loopback"),
	DLT_CHOICE(DLT_ENC, "OpenBSD encapsulated IP"),
	DLT_CHOICE(DLT_LINUX_SLL, "Linux cooked"),
	DLT_CHOICE(DLT_LTALK, "Localtalk"),
	DLT_CHOICE(DLT_PFLOG, "OpenBSD pflog file"),
	DLT_CHOICE(DLT_PRISM_HEADER, "802.11 plus Prism header"),
	DLT_CHOICE(DLT_IP_OVER_FC, "RFC 2625 IP-over-Fibre Channel"),
	DLT_CHOICE(DLT_SUNATM, "Sun raw ATM"),
	DLT_CHOICE(DLT_IEEE802_11_RADIO, "802.11 plus BSD radio information header"),
	DLT_CHOICE(DLT_APPLE_IP_OVER_IEEE1394, "Apple IP-over-IEEE 1394"),
	DLT_CHOICE(DLT_ARCNET_LINUX, "Linux ARCNET"),
	DLT_CHOICE(DLT_LINUX_IRDA, "Linux IrDA"),
	DLT_CHOICE(DLT_IEEE802_11_RADIO_AVS, "802.11 plus AVS radio information header"),
	DLT_CHOICE_SENTINEL
};

#if !defined(HAVE_PCAP_DATALINK_NAME_TO_VAL)
static int
pcap_datalink_name_to_val(const char *name)
{
	int i;

	for (i = 0; dlt_choices[i].name != NULL; i++) {
		if (g_ascii_strcasecmp(dlt_choices[i].name + sizeof("DLT_") - 1,
		    name) == 0)
			return (dlt_choices[i].dlt);
	}
	return (-1);
}
#endif /* defined(HAVE_PCAP_DATALINK_NAME_TO_VAL) */

#if !defined(HAVE_PCAP_DATALINK_VAL_TO_NAME)
static const char *
pcap_datalink_val_to_name(int dlt)
{
	int i;

	for (i = 0; dlt_choices[i].name != NULL; i++) {
		if (dlt_choices[i].dlt == dlt)
			return (dlt_choices[i].name + sizeof("DLT_") - 1);
	}
	return (NULL);
}
#endif /* defined(HAVE_PCAP_DATALINK_VAL_TO_NAME) */

#if !defined(HAVE_PCAP_DATALINK_VAL_TO_DESCRIPTION)
const char *
pcap_datalink_val_to_description(int dlt)
{
	int i;

	for (i = 0; dlt_choices[i].name != NULL; i++) {
		if (dlt_choices[i].dlt == dlt)
			return (dlt_choices[i].description);
	}
	return (NULL);
}
#endif /* defined(HAVE_PCAP_DATALINK_VAL_TO_DESCRIPTION) */

#endif /* !defined(HAVE_PCAP_DATALINK_VAL_TO_NAME) || !defined(HAVE_PCAP_DATALINK_VAL_TO_DESCRIPTION) */

static void
free_linktype_cb(gpointer data, gpointer user_data _U_)
{
	data_link_info_t *linktype_info = (data_link_info_t *)data;

	g_free(linktype_info->name);
	g_free(linktype_info->description);
}

void
free_if_capabilities(if_capabilities_t *caps)
{
	g_list_foreach(caps->data_link_types, free_linktype_cb, NULL);
	g_list_free(caps->data_link_types);
	g_free(caps);
}

const char *
linktype_val_to_name(int dlt)
{
    return pcap_datalink_val_to_name(dlt);
}

int linktype_name_to_val(const char *linktype)
{
    return pcap_datalink_name_to_val(linktype);
}

#endif /* HAVE_LIBPCAP */
