/* packet-icmpv6.c
 * Routines for ICMPv6 packet disassembly
 *
 * $Id: packet-icmpv6.c,v 1.59 2002/01/10 11:21:21 guy Exp $
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@ethereal.com>
 * Copyright 1998 Gerald Combs
 *
 * MobileIPv6 support added by Tomislav Borosa <tomislav.borosa@siemens.hr>
 *
 * HMIPv6 support added by Martti Kuparinen <martti.kuparinen@iki.fi>
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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <stdlib.h>
#include <string.h>

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif

#include <glib.h>

#ifdef NEED_SNPRINTF_H
# include "snprintf.h"
#endif

#include "packet.h"
#include "packet-ipv6.h"
#include "packet-dns.h"
#include "in_cksum.h"
#include "resolv.h"
#include "ipproto.h"

#ifndef offsetof
#define	offsetof(type, member)	((size_t)(&((type *)0)->member))
#endif

static int proto_icmpv6 = -1;
static int hf_icmpv6_type = -1;
static int hf_icmpv6_code = -1;
static int hf_icmpv6_checksum = -1;
static int hf_icmpv6_checksum_bad = -1;

static gint ett_icmpv6 = -1;
static gint ett_icmpv6opt = -1;
static gint ett_icmpv6flag = -1;
static gint ett_nodeinfo_flag = -1;
static gint ett_nodeinfo_subject4 = -1;
static gint ett_nodeinfo_subject6 = -1;
static gint ett_nodeinfo_node4 = -1;
static gint ett_nodeinfo_node6 = -1;
static gint ett_nodeinfo_nodebitmap = -1;
static gint ett_nodeinfo_nodedns = -1;

static dissector_handle_t ipv6_handle;
static dissector_handle_t data_handle;

static const value_string names_nodeinfo_qtype[] = {
    { NI_QTYPE_NOOP,		"NOOP" },
    { NI_QTYPE_SUPTYPES,	"Supported query types" },
    { NI_QTYPE_DNSNAME,		"DNS name" },
    { NI_QTYPE_NODEADDR,	"Node addresses" },
    { NI_QTYPE_IPV4ADDR, 	"IPv4 node addresses" },
    { 0,			NULL }
};

static const value_string names_rrenum_matchcode[] = {
    { RPM_PCO_ADD,		"Add" },
    { RPM_PCO_CHANGE,		"Change" },
    { RPM_PCO_SETGLOBAL,	"Set Global" },
    { 0,			NULL }
};

static const value_string names_router_pref[] = {
        { ND_RA_FLAG_RTPREF_HIGH,	"High" }, 
        { ND_RA_FLAG_RTPREF_MEDIUM,	"Medium" }, 
        { ND_RA_FLAG_RTPREF_LOW,	"Low" }, 
        { ND_RA_FLAG_RTPREF_RSV,	"Reserved" }, 
};

static void
dissect_contained_icmpv6(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree)
{
    tvbuff_t *next_tvb;
    address save_dl_src;
    address save_dl_dst;
    address save_net_src;
    address save_net_dst;
    address save_src;
    address save_dst;
    gboolean save_in_error_pkt;

    next_tvb = tvb_new_subset(tvb, offset, -1, -1);

    /* tiny sanity check */
    if ((tvb_get_guint8(tvb, offset) & 0xf0) == 0x60) {
	/* The contained packet is an IPv6 datagram; dissect it.

	   Set the columns non-writable, so that the packet list
	   shows this as an ICMPv6 packet, not as the type of packet
	   for which the ICMPv6 packet was generated. */
	col_set_writable(pinfo->cinfo, FALSE);

	/* Also, save the current values of the addresses, and restore
	   them when we're finished dissecting the contained packet, so
	   that the address columns in the summary don't reflect the
	   contained packet, but reflect this packet instead. */
	save_dl_src = pinfo->dl_src;
	save_dl_dst = pinfo->dl_dst;
	save_net_src = pinfo->net_src;
	save_net_dst = pinfo->net_dst;
	save_src = pinfo->src;
	save_dst = pinfo->dst;

	/* Save the current value of the "we're inside an error packet"
	   flag, and set that flag; subdissectors may treat packets
	   that are the payload of error packets differently from
	   "real" packets. */
	save_in_error_pkt = pinfo->in_error_pkt;
	pinfo->in_error_pkt = TRUE;

	/* Dissect the contained packet.
	   Catch ReportedBoundsError, and do nothing if we see it,
	   because it's not an error if the contained packet is short;
	   there's no guarantee that all of it was included.

	   XXX - should catch BoundsError, and re-throw it after cleaning
	   up. */
	TRY {
	    call_dissector(ipv6_handle, next_tvb, pinfo, tree);
	}
	CATCH(ReportedBoundsError) {
	    ; /* do nothing */
	}
	ENDTRY;

	/* Restore the "we're inside an error packet" flag. */
	pinfo->in_error_pkt = save_in_error_pkt;

	/* Restore the addresses. */
	pinfo->dl_src = save_dl_src;
	pinfo->dl_dst = save_dl_dst;
	pinfo->net_src = save_net_src;
	pinfo->net_dst = save_net_dst;
	pinfo->src = save_src;
	pinfo->dst = save_dst;
    } else
	call_dissector(data_handle,next_tvb, pinfo, tree);
}

static void
dissect_icmpv6opt(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree)
{
    proto_tree *icmp6opt_tree, *field_tree;
    proto_item *ti, *tf;
    struct nd_opt_hdr nd_opt_hdr, *opt;
    int len;
    char *typename;

    if (!tree)
	return;

again:
    if ((int)tvb_reported_length(tvb) <= offset)
	return; /* No more options left */

    opt = &nd_opt_hdr;
    tvb_memcpy(tvb, (guint8 *)opt, offset, sizeof *opt);
    len = opt->nd_opt_len << 3;

    /* !!! specify length */
    ti = proto_tree_add_text(tree, tvb, offset, len, "ICMPv6 options");
    icmp6opt_tree = proto_item_add_subtree(ti, ett_icmpv6opt);

    if (len == 0) {
	proto_tree_add_text(icmp6opt_tree, tvb,
			    offset + offsetof(struct nd_opt_hdr, nd_opt_len), 1,
			    "Invalid option length: %u",
			    opt->nd_opt_len);
	return; /* we must not try to decode this */
    }

    switch (opt->nd_opt_type) {
    case ND_OPT_SOURCE_LINKADDR:
	typename = "Source link-layer address";
	break;
    case ND_OPT_TARGET_LINKADDR:
	typename = "Target link-layer address";
	break;
    case ND_OPT_PREFIX_INFORMATION:
	typename = "Prefix information";
	break;
    case ND_OPT_REDIRECTED_HEADER:
	typename = "Redirected header";
	break;
    case ND_OPT_MTU:
	typename = "MTU";
	break;
    case ND_OPT_ADVINTERVAL:
	typename = "Advertisement Interval";
	break;
    case ND_OPT_HOMEAGENT_INFO:
	typename = "Home Agent Information";
	break;
    case ND_OPT_MAP:
	typename = "HMIPv6 MAP option";
	break;
    default:
	typename = "Unknown";
	break;
    }

    proto_tree_add_text(icmp6opt_tree, tvb,
	offset + offsetof(struct nd_opt_hdr, nd_opt_type), 1,
	"Type: %u (%s)", opt->nd_opt_type, typename);
    proto_tree_add_text(icmp6opt_tree, tvb,
	offset + offsetof(struct nd_opt_hdr, nd_opt_len), 1,
	"Length: %u bytes (%u)", opt->nd_opt_len << 3, opt->nd_opt_len);

    /* decode... */
    switch (opt->nd_opt_type) {
    case ND_OPT_SOURCE_LINKADDR:
    case ND_OPT_TARGET_LINKADDR:
      {
	char *t;
	int len, i, p;
	len = (opt->nd_opt_len << 3) - sizeof(*opt);
	t = (char *)malloc(len * 3);
	memset(t, 0, len * 3);
	p = offset + sizeof(*opt);
	for (i = 0; i < len; i++) {
	    if (i)
		t[i * 3 - 1] = ':';
	    sprintf(&t[i * 3], "%02x", tvb_get_guint8(tvb, p + i) & 0xff);
	}
	proto_tree_add_text(icmp6opt_tree, tvb,
	    offset + sizeof(*opt), len, "Link-layer address: %s", t);
	free(t);
	break;
      }
    case ND_OPT_PREFIX_INFORMATION:
      {
	struct nd_opt_prefix_info nd_opt_prefix_info, *pi;
	int flagoff;

	pi = &nd_opt_prefix_info;
	tvb_memcpy(tvb, (guint8 *)pi, offset, sizeof *pi);
	proto_tree_add_text(icmp6opt_tree, tvb,
	    offset + offsetof(struct nd_opt_prefix_info, nd_opt_pi_prefix_len),
	    1, "Prefix length: %u", pi->nd_opt_pi_prefix_len);

	flagoff = offset + offsetof(struct nd_opt_prefix_info, nd_opt_pi_flags_reserved);
	tf = proto_tree_add_text(icmp6opt_tree, tvb, flagoff, 1, "Flags: 0x%02x",
	    tvb_get_guint8(tvb, offset + offsetof(struct nd_opt_prefix_info, nd_opt_pi_flags_reserved)));
	field_tree = proto_item_add_subtree(tf, ett_icmpv6flag);
	proto_tree_add_text(field_tree, tvb, flagoff, 1, "%s",
	    decode_boolean_bitfield(pi->nd_opt_pi_flags_reserved,
		    ND_OPT_PI_FLAG_ONLINK, 8, "Onlink", "Not onlink"));
	proto_tree_add_text(field_tree, tvb, flagoff, 1, "%s",
	    decode_boolean_bitfield(pi->nd_opt_pi_flags_reserved,
		    ND_OPT_PI_FLAG_AUTO, 8, "Auto", "Not auto"));
	proto_tree_add_text(field_tree, tvb, flagoff, 1, "%s",
	    decode_boolean_bitfield(pi->nd_opt_pi_flags_reserved,
		    ND_OPT_PI_FLAG_ROUTER, 8,
		    "Router Address", "Not router address"));
	proto_tree_add_text(field_tree, tvb, flagoff, 1, "%s",
	    decode_boolean_bitfield(pi->nd_opt_pi_flags_reserved,
		    ND_OPT_PI_FLAG_SITEPREF, 8,
		    "Site prefix", "Not site prefix"));
	proto_tree_add_text(icmp6opt_tree, tvb,
	    offset + offsetof(struct nd_opt_prefix_info, nd_opt_pi_valid_time),
	    4, "Valid lifetime: 0x%08x",
	    pntohl(&pi->nd_opt_pi_valid_time));
	proto_tree_add_text(icmp6opt_tree, tvb,
	    offset + offsetof(struct nd_opt_prefix_info, nd_opt_pi_preferred_time),
	    4, "Preferred lifetime: 0x%08x",
	    pntohl(&pi->nd_opt_pi_preferred_time));
	proto_tree_add_text(icmp6opt_tree, tvb,
	    offset + offsetof(struct nd_opt_prefix_info, nd_opt_pi_prefix),
	    16, "Prefix: %s", ip6_to_str(&pi->nd_opt_pi_prefix));
	break;
      }
    case ND_OPT_REDIRECTED_HEADER:
	proto_tree_add_text(icmp6opt_tree, tvb,
	    offset + 8, (opt->nd_opt_len << 3) - 8, "Redirected packet");
	dissect_contained_icmpv6(tvb, offset + 8, pinfo, icmp6opt_tree);
	break;
    case ND_OPT_MTU:
	proto_tree_add_text(icmp6opt_tree, tvb,
	    offset + offsetof(struct nd_opt_mtu, nd_opt_mtu_mtu), 4,
	    "MTU: %u", tvb_get_ntohl(tvb, offset + offsetof(struct nd_opt_mtu, nd_opt_mtu_mtu)));
	break;
    case ND_OPT_ADVINTERVAL:
	proto_tree_add_text(icmp6opt_tree, tvb,
	    offset + offsetof(struct nd_opt_adv_int, nd_opt_adv_int_advint), 4,
	    "Advertisement Interval: %d",
	    tvb_get_ntohl(tvb, offset + offsetof(struct nd_opt_adv_int, nd_opt_adv_int_advint)));
	break;
    case ND_OPT_HOMEAGENT_INFO:
      {
	struct nd_opt_ha_info pibuf, *pi;

	pi = &pibuf;
	tvb_memcpy(tvb, (guint8 *)pi, offset, sizeof *pi);
	proto_tree_add_text(icmp6opt_tree, tvb,
	    offset + offsetof(struct nd_opt_ha_info, nd_opt_ha_info_ha_pref),
	    2, "Home Agent Preference: %d",
	    pntohs(&pi->nd_opt_ha_info_ha_pref));
	proto_tree_add_text(icmp6opt_tree, tvb,
	    offset + offsetof(struct nd_opt_ha_info, nd_opt_ha_info_ha_life),
	    2, "Home Agent Lifetime: %d",
	    pntohs(&pi->nd_opt_ha_info_ha_life));
	break;
      }
    case ND_OPT_MAP:
      {
	struct nd_opt_map_info mapbuf, *map;
	int flagoff;

	map = &mapbuf;
	tvb_memcpy(tvb, (guint8 *)map, offset, sizeof *map);
	proto_tree_add_text(icmp6opt_tree, tvb,
	    offset + offsetof(struct nd_opt_map_info, nd_opt_map_distance),
	    1, "Distance: %u", map->nd_opt_map_distance);
	proto_tree_add_text(icmp6opt_tree, tvb,
	    offset + offsetof(struct nd_opt_map_info, nd_opt_map_preference),
	    1, "Preference: %u", map->nd_opt_map_preference);
	proto_tree_add_text(icmp6opt_tree, tvb,
	    offset + offsetof(struct nd_opt_map_info, nd_opt_map_prefixlen),
	    1, "Prefix Length: %u", map->nd_opt_map_prefixlen);
	flagoff = offset + offsetof(struct nd_opt_map_info,
	    nd_opt_map_flags);
	tf = proto_tree_add_text(icmp6opt_tree, tvb, flagoff, 1,
	    "Flags: 0x%02x",
	    tvb_get_guint8(tvb, offset + offsetof(struct nd_opt_map_info,
	    nd_opt_map_flags)));
	field_tree = proto_item_add_subtree(tf, ett_icmpv6flag);
	proto_tree_add_text(field_tree, tvb, flagoff, 1, "%s",
	    decode_boolean_bitfield(map->nd_opt_map_flags,
		ND_OPT_MAP_FLAG_R, 8, "R", "No R"));
	proto_tree_add_text(field_tree, tvb, flagoff, 1, "%s",
	    decode_boolean_bitfield(map->nd_opt_map_flags,
		ND_OPT_MAP_FLAG_M, 8, "M", "No M"));
	proto_tree_add_text(field_tree, tvb, flagoff, 1, "%s",
	    decode_boolean_bitfield(map->nd_opt_map_flags,
		ND_OPT_MAP_FLAG_I, 8, "I", "No I"));
	proto_tree_add_text(field_tree, tvb, flagoff, 1, "%s",
	    decode_boolean_bitfield(map->nd_opt_map_flags,
		ND_OPT_MAP_FLAG_T, 8, "T", "No T"));
	proto_tree_add_text(field_tree, tvb, flagoff, 1, "%s",
	    decode_boolean_bitfield(map->nd_opt_map_flags,
		ND_OPT_MAP_FLAG_P, 8, "P", "No P"));
	proto_tree_add_text(icmp6opt_tree, tvb,
	    offset + offsetof(struct nd_opt_map_info, nd_opt_map_lifetime),
	    4, "Lifetime: %u", pntohl(&map->nd_opt_map_lifetime));

	proto_tree_add_text(icmp6opt_tree, tvb,
	    offset + offsetof(struct nd_opt_map_info, nd_opt_map_address), 16,
#ifdef INET6
	    "Address of MAP: %s (%s)",
	    get_hostname6(&map->nd_opt_map_address),
#else
	    "Address of MAP: %s",
#endif
	    ip6_to_str(&map->nd_opt_map_address));
	break;
      }
    case ND_OPT_ROUTE_INFO:
      {
	struct nd_opt_route_info ribuf, *ri;
	struct e_in6_addr in6;
	int l;
	guint32 lifetime;

	ri = &ribuf;
	tvb_memcpy(tvb, (guint8 *)ri, offset, sizeof *ri);
	memset(&in6, 0, sizeof(in6));
	switch (ri->nd_opt_rti_len) {
	case 1:
	    l = 0;
	    break;
	case 2:
	    tvb_memcpy(tvb, (guint8 *)&in6, offset + sizeof(*ri), l = 8);
	    break;
	case 3:
	    tvb_memcpy(tvb, (guint8 *)&in6, offset + sizeof(*ri), l = 16);
	    break;
	default:
	    l = -1;
	    break;
	}
	if (l >= 0) {
	    proto_tree_add_text(icmp6opt_tree, tvb,
		offset + offsetof(struct nd_opt_route_info, nd_opt_rti_prefixlen),
		1, "Prefix length: %u", ri->nd_opt_rti_prefixlen);
	    tf = proto_tree_add_text(icmp6opt_tree, tvb,
		offset + offsetof(struct nd_opt_route_info, nd_opt_rti_flags),
		1, "Flags: 0x%02x", ri->nd_opt_rti_flags);
	    field_tree = proto_item_add_subtree(tf, ett_icmpv6flag);
	    proto_tree_add_text(field_tree, tvb,
		offset + offsetof(struct nd_opt_route_info, nd_opt_rti_flags),
		1, "%s",
		decode_enumerated_bitfield(ri->nd_opt_rti_flags,
		    ND_RA_FLAG_RTPREF_MASK, 8, names_router_pref,
		    "Router preference: %s"));
	    lifetime = pntohl(&ri->nd_opt_rti_lifetime);
	    if (lifetime == 0xffffffff)
		proto_tree_add_text(icmp6opt_tree, tvb,
		    offset + offsetof(struct nd_opt_route_info, nd_opt_rti_lifetime),
		    sizeof(ri->nd_opt_rti_lifetime), "Lifetime: infinity");
	    else
		proto_tree_add_text(icmp6opt_tree, tvb,
		    offset + offsetof(struct nd_opt_route_info, nd_opt_rti_lifetime),
		    sizeof(ri->nd_opt_rti_lifetime), "Lifetime: %u", lifetime);
	    proto_tree_add_text(icmp6opt_tree, tvb,
		offset + sizeof(*ri), l, "Prefix: %s", ip6_to_str(&in6));
	} else {
	    proto_tree_add_text(icmp6opt_tree, tvb,
		offset + offsetof(struct nd_opt_hdr, nd_opt_len), 1,
		"Invalid option length: %u", opt->nd_opt_len);
	}
	break;
      }
    }

    offset += (opt->nd_opt_len << 3);
    goto again;
}

/*
 * draft-ietf-ipngwg-icmp-name-lookups-07.txt
 * Note that the packet format was changed several times in the past.
 */

static const char *
bitrange0(v, s, buf, buflen)
	guint32 v;
	int s;
	char *buf;
	int buflen;
{
	guint32 v0;
	char *p, *ep;
	int off;
	int i, l;

	if (buflen < 1)
		return NULL;
	if (buflen == 1) {
		buf[0] = '\0';
		return NULL;
	}

	v0 = v;
	p = buf;
	ep = buf + buflen - 1;
	memset(buf, 0, buflen);
	off = 0;
	while (off < 32) {
		/* shift till we have 0x01 */
		if ((v & 0x01) == 0) {
			switch (v & 0x0f) {
			case 0x00:
				v >>= 4; off += 4; continue;
			case 0x08:
				v >>= 3; off += 3; continue;
			case 0x04: case 0x0c:
				v >>= 2; off += 2; continue;
			default:
				v >>= 1; off += 1; continue;
			}
		}

		/* we have 0x01 with us */
		for (i = 0; i < 32 - off; i++) {
			if ((v & (0x01 << i)) == 0)
				break;
		}
		if (i == 1)
			l = snprintf(p, ep - p, ",%d", s + off);
		else {
			l = snprintf(p, ep - p, ",%d-%d", s + off,
			    s + off + i - 1);
		}
		if (l == -1 || l > ep - p) {
			buf[0] = '\0';
			return NULL;
		}
		v >>= i; off += i;
	}

	return buf;
}

static const char *
bitrange(tvbuff_t *tvb, int offset, int l, int s)
{
    static char buf[1024];
    char *q, *eq;
    int i;

    memset(buf, 0, sizeof(buf));
    q = buf;
    eq = buf + sizeof(buf) - 1;
    for (i = 0; i < l; i++) {
	if (bitrange0(tvb_get_ntohl(tvb, offset + i * 4), s + i * 4, q, eq - q) == NULL) {
	    if (q != buf && q + 5 < buf + sizeof(buf))
		strncpy(q, ",...", 5);
	    return buf;
	}
    }

    return buf + 1;
}

static void
dissect_nodeinfo(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree)
{
    proto_tree *field_tree;
	proto_item *tf;
    struct icmp6_nodeinfo icmp6_nodeinfo, *ni;
    int off;
    unsigned int j;
    int i, n, l, p;
    guint16 flags;
    char dname[MAXDNAME];
    guint8 ipaddr[4];

    ni = &icmp6_nodeinfo;
    tvb_memcpy(tvb, (guint8 *)ni, offset, sizeof *ni);
    /* flags */
    flags = pntohs(&ni->ni_flags);
    tf = proto_tree_add_text(tree, tvb,
	offset + offsetof(struct icmp6_nodeinfo, ni_flags),
	sizeof(ni->ni_flags), "Flags: 0x%04x", flags);
    field_tree = proto_item_add_subtree(tf, ett_nodeinfo_flag);
    switch (pntohs(&ni->ni_qtype)) {
    case NI_QTYPE_SUPTYPES:
	if (ni->ni_type == ICMP6_NI_QUERY) {
	    proto_tree_add_text(field_tree, tvb,
		offset + offsetof(struct icmp6_nodeinfo, ni_flags),
		sizeof(ni->ni_flags), "%s",
		decode_boolean_bitfield(flags, NI_SUPTYPE_FLAG_COMPRESS, sizeof(flags) * 8,
		    "Compressed reply supported",
		    "No compressed reply support"));
	} else {
	    proto_tree_add_text(field_tree, tvb,
		offset + offsetof(struct icmp6_nodeinfo, ni_flags),
		sizeof(ni->ni_flags), "%s",
		decode_boolean_bitfield(flags, NI_SUPTYPE_FLAG_COMPRESS, sizeof(flags) * 8,
		    "Compressed", "Not compressed"));
	}
	break;
    case NI_QTYPE_DNSNAME:
	if (ni->ni_type == ICMP6_NI_REPLY) {
	    proto_tree_add_text(field_tree, tvb,
		offset + offsetof(struct icmp6_nodeinfo, ni_flags),
		sizeof(ni->ni_flags), "%s",
		decode_boolean_bitfield(flags, NI_FQDN_FLAG_VALIDTTL, sizeof(flags) * 8,
		    "Valid TTL field", "Meaningless TTL field"));
	}
	break;
    case NI_QTYPE_NODEADDR:
	proto_tree_add_text(field_tree, tvb,
	    offset + offsetof(struct icmp6_nodeinfo, ni_flags),
	    sizeof(ni->ni_flags), "%s",
	    decode_boolean_bitfield(flags, NI_NODEADDR_FLAG_GLOBAL, sizeof(flags) * 8,
		"Global address",
		"Not global address"));
	proto_tree_add_text(field_tree, tvb,
	    offset + offsetof(struct icmp6_nodeinfo, ni_flags),
	    sizeof(ni->ni_flags), "%s",
	    decode_boolean_bitfield(flags, NI_NODEADDR_FLAG_SITELOCAL, sizeof(flags) * 8,
		"Site-local address",
		"Not site-local address"));
	proto_tree_add_text(field_tree, tvb,
	    offset + offsetof(struct icmp6_nodeinfo, ni_flags),
	    sizeof(ni->ni_flags), "%s",
	    decode_boolean_bitfield(flags, NI_NODEADDR_FLAG_LINKLOCAL, sizeof(flags) * 8,
		"Link-local address",
		"Not link-local address"));
	proto_tree_add_text(field_tree, tvb,
	    offset + offsetof(struct icmp6_nodeinfo, ni_flags),
	    sizeof(ni->ni_flags), "%s",
	    decode_boolean_bitfield(flags, NI_NODEADDR_FLAG_COMPAT, sizeof(flags) * 8,
		"IPv4 compatible/mapped address",
		"Not IPv4 compatible/mapped address"));
	/* fall through */
    case NI_QTYPE_IPV4ADDR:
	proto_tree_add_text(field_tree, tvb,
	    offset + offsetof(struct icmp6_nodeinfo, ni_flags),
	    sizeof(ni->ni_flags), "%s",
	    decode_boolean_bitfield(flags, NI_NODEADDR_FLAG_ALL, sizeof(flags) * 8,
		"All unicast address",
		"Unicast addresses on the queried interface"));
	proto_tree_add_text(field_tree, tvb,
	    offset + offsetof(struct icmp6_nodeinfo, ni_flags),
	    sizeof(ni->ni_flags), "%s",
	    decode_boolean_bitfield(flags, NI_NODEADDR_FLAG_TRUNCATE, sizeof(flags) * 8,
		"Truncated", "Not truncated"));
	break;
    }

    /* nonce */
    proto_tree_add_text(tree, tvb,
	offset + offsetof(struct icmp6_nodeinfo, icmp6_ni_nonce[0]),
	sizeof(ni->icmp6_ni_nonce), "Nonce: 0x%08x%08x",
	pntohl(&ni->icmp6_ni_nonce[0]), pntohl(&ni->icmp6_ni_nonce[4]));

    /* offset for "the rest of data" */
    off = sizeof(*ni);

    /* rest of data */
    if (!tvb_bytes_exist(tvb, offset, sizeof(*ni)))
	goto nodata;
    if (ni->ni_type == ICMP6_NI_QUERY) {
	switch (ni->ni_code) {
	case ICMP6_NI_SUBJ_IPV6:
	    n = tvb_length_remaining(tvb, offset + sizeof(*ni));
	    n /= sizeof(struct e_in6_addr);
	    tf = proto_tree_add_text(tree, tvb,
		offset + sizeof(*ni), tvb_length_remaining(tvb, offset), "IPv6 subject addresses");
	    field_tree = proto_item_add_subtree(tf, ett_nodeinfo_subject6);
	    p = offset + sizeof *ni;
	    for (i = 0; i < n; i++) {
		struct e_in6_addr e_in6_addr;
		tvb_memcpy(tvb, (guint8 *)&e_in6_addr, p, sizeof e_in6_addr);
		proto_tree_add_text(field_tree, tvb,
		    p, sizeof(struct e_in6_addr),
		    "%s", ip6_to_str(&e_in6_addr));
		p += sizeof(struct e_in6_addr);
	    }
	    off = tvb_length_remaining(tvb, offset);
	    break;
	case ICMP6_NI_SUBJ_FQDN:
	    l = get_dns_name(tvb, offset + sizeof(*ni),
	    	offset + sizeof(*ni), dname, sizeof(dname));
	    if (tvb_bytes_exist(tvb, offset + sizeof(*ni) + l, 1) &&
	        tvb_get_guint8(tvb, offset + sizeof(*ni) + l) == 0) {
		l++;
		proto_tree_add_text(tree, tvb, offset + sizeof(*ni), l,
		    "DNS label: %s (truncated)", dname);
	    } else {
		proto_tree_add_text(tree, tvb, offset + sizeof(*ni), l,
		    "DNS label: %s", dname);
	    }
	    off = tvb_length_remaining(tvb, offset + sizeof(*ni) + l);
	    break;
	case ICMP6_NI_SUBJ_IPV4:
	    n = tvb_length_remaining(tvb, offset + sizeof(*ni));
	    n /= sizeof(guint32);
	    tf = proto_tree_add_text(tree, tvb,
		offset + sizeof(*ni), tvb_length_remaining(tvb, offset), "IPv4 subject addresses");
	    field_tree = proto_item_add_subtree(tf, ett_nodeinfo_subject4);
	    p = offset + sizeof *ni;
	    for (i = 0; i < n; i++) {
		tvb_memcpy(tvb, ipaddr, p, 4);                    
		proto_tree_add_text(field_tree, tvb,
		    p, sizeof(guint32), "%s", ip_to_str(ipaddr));
		p += sizeof(guint32);
	    }
	    off = tvb_length_remaining(tvb, offset);
	    break;
	}
    } else {
	switch (pntohs(&ni->ni_qtype)) {
	case NI_QTYPE_NOOP:
	    break;
	case NI_QTYPE_SUPTYPES:
	    p = offset + sizeof *ni;
	    tf = proto_tree_add_text(tree, tvb,
		offset + sizeof(*ni), tvb_length_remaining(tvb, p),
		"Supported type bitmap%s",
		(flags & 0x0001) ? ", compressed" : "");
	    field_tree = proto_item_add_subtree(tf,
		ett_nodeinfo_nodebitmap);
	    n = 0;
	    while (tvb_bytes_exist(tvb, p, sizeof(guint32))) { /* XXXX Check what? */
		if ((flags & 0x0001) == 0) {
		    l = tvb_length_remaining(tvb, offset + sizeof(*ni));
		    l /= sizeof(guint32);
		    i = 0;
		} else {
		    l = tvb_get_ntohs(tvb, p);
		    i = tvb_get_ntohs(tvb, p + sizeof(guint16));	/*skip*/
		}
		if (n + l * 32 > (1 << 16))
		    break;
		if (n + (l + i) * 32 > (1 << 16))
		    break;
		if ((flags & 0x0001) == 0) {
		    proto_tree_add_text(field_tree, tvb, p,
			l * 4, "Bitmap (%d to %d): %s", n, n + l * 32 - 1,
			bitrange(tvb, p, l, n));
		    p += l * 4;
		} else {
		    proto_tree_add_text(field_tree, tvb, p,
			4 + l * 4, "Bitmap (%d to %d): %s", n, n + l * 32 - 1,
			bitrange(tvb, p + 4, l, n));
		    p += (4 + l * 4);
		}
		n += l * 32 + i * 32;
	    }
	    off = tvb_length_remaining(tvb, offset);
	    break;
	case NI_QTYPE_DNSNAME:
	    proto_tree_add_text(tree, tvb, offset + sizeof(*ni),
		sizeof(gint32), "TTL: %d", (gint32)tvb_get_ntohl(tvb, offset + sizeof *ni));
	    tf = proto_tree_add_text(tree, tvb,
		offset + sizeof(*ni) + sizeof(guint32),
		tvb_length_remaining(tvb, offset),
		"DNS labels");
	    field_tree = proto_item_add_subtree(tf, ett_nodeinfo_nodedns);
	    j = offset + sizeof (*ni) + sizeof(guint32);
	    while (j < tvb_length(tvb)) {
		l = get_dns_name(tvb, j,
		   offset + sizeof (*ni) + sizeof(guint32),
		   dname,sizeof(dname));
		if (tvb_bytes_exist(tvb, j + l, 1) &&
		    tvb_get_guint8(tvb, j + l) == 0) {
		    l++;
		    proto_tree_add_text(field_tree, tvb, j, l,
			"DNS label: %s (truncated)", dname);
		} else {
		    proto_tree_add_text(field_tree, tvb, j, l,
			"DNS label: %s", dname);
		}
		j += l;
	    }
	    off = tvb_length_remaining(tvb, offset);
	    break;
	case NI_QTYPE_NODEADDR:
	    n = tvb_length_remaining(tvb, offset + sizeof(*ni));
	    n /= sizeof(gint32) + sizeof(struct e_in6_addr);
	    tf = proto_tree_add_text(tree, tvb,
		offset + sizeof(*ni), tvb_length_remaining(tvb, offset), "IPv6 node addresses");
	    field_tree = proto_item_add_subtree(tf, ett_nodeinfo_node6);
	    p = offset + sizeof (*ni);
	    for (i = 0; i < n; i++) {
		struct e_in6_addr e_in6_addr;
		gint32 ttl;
		ttl = (gint32)tvb_get_ntohl(tvb, p);
		tvb_memcpy(tvb, (guint8 *)&e_in6_addr, p + sizeof ttl, sizeof e_in6_addr);
		proto_tree_add_text(field_tree, tvb,
		    p, sizeof(struct e_in6_addr) + sizeof(gint32),
		    "%s (TTL %d)", ip6_to_str(&e_in6_addr), ttl);
		p += sizeof(struct e_in6_addr) + sizeof(gint32);
	    }
	    off = tvb_length_remaining(tvb, offset);
	    break;
	case NI_QTYPE_IPV4ADDR:
	    n = tvb_length_remaining(tvb, offset + sizeof(*ni));
	    n /= sizeof(gint32) + sizeof(guint32);
	    tf = proto_tree_add_text(tree, tvb,
		offset + sizeof(*ni), tvb_length_remaining(tvb, offset), "IPv4 node addresses");
	    field_tree = proto_item_add_subtree(tf, ett_nodeinfo_node4);
	    p = offset + sizeof *ni;
	    for (i = 0; i < n; i++) {
		tvb_memcpy(tvb, ipaddr, sizeof(gint32) + p, 4);
		proto_tree_add_text(field_tree, tvb,
		    p, sizeof(guint32), "%s (TTL %d)", ip_to_str(ipaddr), tvb_get_ntohl(tvb, p));
		p += sizeof(gint32) + sizeof(guint32);
	    }
	    off = tvb_length_remaining(tvb, offset);
	    break;
	}
    }
nodata:;

    /* the rest of data */
    call_dissector(data_handle,tvb_new_subset(tvb, offset + off, -1, -1), pinfo, tree);
}

static void
dissect_rrenum(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree)
{
    proto_tree *field_tree, *opt_tree;
	proto_item *tf;
    struct icmp6_router_renum icmp6_router_renum, *rr;
    struct rr_pco_match rr_pco_match, *match;
    struct rr_pco_use rr_pco_use, *use;
    int flagoff, off;
    unsigned int l;
    guint8 flags;

    rr = &icmp6_router_renum;
    tvb_memcpy(tvb, (guint8 *)rr, offset, sizeof *rr);
    proto_tree_add_text(tree, tvb,
	offset + offsetof(struct icmp6_router_renum, rr_seqnum), 4,
	"Sequence number: 0x%08x", pntohl(&rr->rr_seqnum));
    proto_tree_add_text(tree, tvb,
	offset + offsetof(struct icmp6_router_renum, rr_segnum), 1,
	"Segment number: 0x%02x", rr->rr_segnum);

    flagoff = offset + offsetof(struct icmp6_router_renum, rr_flags);
    flags = tvb_get_guint8(tvb, flagoff);
    tf = proto_tree_add_text(tree, tvb, flagoff, 1,
	"Flags: 0x%02x", flags);
    field_tree = proto_item_add_subtree(tf, ett_icmpv6flag);
    proto_tree_add_text(field_tree, tvb, flagoff, 1, "%s",
	decode_boolean_bitfield(flags, 0x80, 8,
	    "Test command", "Not test command"));
    proto_tree_add_text(field_tree, tvb, flagoff, 1, "%s",
	decode_boolean_bitfield(flags, 0x40, 8,
	    "Result requested", "Result not requested"));
    proto_tree_add_text(field_tree, tvb, flagoff, 1, "%s",
	decode_boolean_bitfield(flags, 0x20, 8,
	    "All interfaces", "Not all interfaces"));
    proto_tree_add_text(field_tree, tvb, flagoff, 1, "%s",
	decode_boolean_bitfield(flags, 0x10, 8,
	    "Site specific", "Not site specific"));
    proto_tree_add_text(field_tree, tvb, flagoff, 1, "%s",
	decode_boolean_bitfield(flags, 0x08, 8,
	    "Processed previously", "Complete result"));

    proto_tree_add_text(tree, tvb,
	offset + offsetof(struct icmp6_router_renum, rr_maxdelay), 2,
	"Max delay: 0x%04x", pntohs(&rr->rr_maxdelay));
    call_dissector(data_handle,tvb_new_subset(tvb, offset + sizeof(*rr), -1, -1), pinfo, tree);	/*XXX*/

    if (rr->rr_code == ICMP6_ROUTER_RENUMBERING_COMMAND) {
	off = offset + sizeof(*rr);
	match = &rr_pco_match;
	tvb_memcpy(tvb, (guint8 *)match, off, sizeof *match);
	tf = proto_tree_add_text(tree, tvb, off, sizeof(*match),
	    "Match-Prefix: %s/%u (%u-%u)", ip6_to_str(&match->rpm_prefix),
	    match->rpm_matchlen, match->rpm_minlen, match->rpm_maxlen);
	opt_tree = proto_item_add_subtree(tf, ett_icmpv6opt);
	proto_tree_add_text(opt_tree, tvb,
	    off + offsetof(struct rr_pco_match, rpm_code),
	    sizeof(match->rpm_code), "OpCode: %s (%u)",
	    val_to_str(match->rpm_code, names_rrenum_matchcode, "Unknown"), 
	    match->rpm_code);
	proto_tree_add_text(opt_tree, tvb,
	    off + offsetof(struct rr_pco_match, rpm_len),
	    sizeof(match->rpm_len), "OpLength: %u (%u octets)",
	    match->rpm_len, match->rpm_len * 8);
	proto_tree_add_text(opt_tree, tvb,
	    off + offsetof(struct rr_pco_match, rpm_ordinal),
	    sizeof(match->rpm_ordinal), "Ordinal: %u", match->rpm_ordinal);
	proto_tree_add_text(opt_tree, tvb,
	    off + offsetof(struct rr_pco_match, rpm_matchlen),
	    sizeof(match->rpm_matchlen), "MatchLen: %u", match->rpm_matchlen);
	proto_tree_add_text(opt_tree, tvb,
	    off + offsetof(struct rr_pco_match, rpm_minlen),
	    sizeof(match->rpm_minlen), "MinLen: %u", match->rpm_minlen);
	proto_tree_add_text(opt_tree, tvb,
	    off + offsetof(struct rr_pco_match, rpm_maxlen),
	    sizeof(match->rpm_maxlen), "MaxLen: %u", match->rpm_maxlen);
	proto_tree_add_text(opt_tree, tvb,
	    off + offsetof(struct rr_pco_match, rpm_prefix),
	    sizeof(match->rpm_prefix), "MatchPrefix: %s",
	    ip6_to_str(&match->rpm_prefix));

	off += sizeof(*match);
	use = &rr_pco_use;
	for (l = match->rpm_len * 8 - sizeof(*match);
	     l >= sizeof(*use); l -= sizeof(*use), off += sizeof(*use)) {
	    tvb_memcpy(tvb, (guint8 *)use, off, sizeof *use);
	    tf = proto_tree_add_text(tree, tvb, off, sizeof(*use),
		"Use-Prefix: %s/%u (keep %u)", ip6_to_str(&use->rpu_prefix),
		use->rpu_uselen, use->rpu_keeplen);
	    opt_tree = proto_item_add_subtree(tf, ett_icmpv6opt);
	    proto_tree_add_text(opt_tree, tvb,
		off + offsetof(struct rr_pco_use, rpu_uselen),
		sizeof(use->rpu_uselen), "UseLen: %u", use->rpu_uselen);
	    proto_tree_add_text(opt_tree, tvb,
		off + offsetof(struct rr_pco_use, rpu_keeplen),
		sizeof(use->rpu_keeplen), "KeepLen: %u", use->rpu_keeplen);
	    tf = proto_tree_add_text(opt_tree, tvb,
		flagoff = off + offsetof(struct rr_pco_use, rpu_ramask),
		sizeof(use->rpu_ramask), "FlagMask: 0x%x", use->rpu_ramask);
	    field_tree = proto_item_add_subtree(tf, ett_icmpv6flag);
	    flags = tvb_get_guint8(tvb, flagoff);
	    proto_tree_add_text(field_tree, tvb, flagoff, 1, "%s",
		decode_boolean_bitfield(flags,
		    ICMP6_RR_PCOUSE_RAFLAGS_ONLINK, 8,
		    "Onlink", "Not onlink"));
	    proto_tree_add_text(field_tree, tvb, flagoff, 1, "%s",
		decode_boolean_bitfield(flags,
		    ICMP6_RR_PCOUSE_RAFLAGS_AUTO, 8,
		    "Auto", "Not auto"));
	    tf = proto_tree_add_text(opt_tree, tvb,
		flagoff = off + offsetof(struct rr_pco_use, rpu_raflags),
		sizeof(use->rpu_raflags), "RAFlags: 0x%x", use->rpu_raflags);
	    field_tree = proto_item_add_subtree(tf, ett_icmpv6flag);
	    flags = tvb_get_guint8(tvb, flagoff);
	    proto_tree_add_text(field_tree, tvb, flagoff, 1, "%s",
		decode_boolean_bitfield(flags,
		    ICMP6_RR_PCOUSE_RAFLAGS_ONLINK, 8,
		    "Onlink", "Not onlink"));
	    proto_tree_add_text(field_tree, tvb, flagoff, 1, "%s",
		decode_boolean_bitfield(flags,
		    ICMP6_RR_PCOUSE_RAFLAGS_AUTO, 8, "Auto", "Not auto"));
	    if (pntohl(&use->rpu_vltime) == 0xffffffff)
		proto_tree_add_text(opt_tree, tvb,
		    off + offsetof(struct rr_pco_use, rpu_vltime),
		    sizeof(use->rpu_vltime), "Valid Lifetime: infinity");
	    else
		proto_tree_add_text(opt_tree, tvb,
		    off + offsetof(struct rr_pco_use, rpu_vltime),
		    sizeof(use->rpu_vltime), "Valid Lifetime: %u",
		    pntohl(&use->rpu_vltime));
	    if (pntohl(&use->rpu_pltime) == 0xffffffff)
		proto_tree_add_text(opt_tree, tvb,
		    off + offsetof(struct rr_pco_use, rpu_pltime),
		    sizeof(use->rpu_pltime), "Preferred Lifetime: infinity");
	    else
		proto_tree_add_text(opt_tree, tvb,
		    off + offsetof(struct rr_pco_use, rpu_pltime),
		    sizeof(use->rpu_pltime), "Preferred Lifetime: %u",
		    pntohl(&use->rpu_pltime));
	    tf = proto_tree_add_text(opt_tree, tvb,
		flagoff = off + offsetof(struct rr_pco_use, rpu_flags),
		sizeof(use->rpu_flags), "Flags: 0x%08x",
		pntohl(&use->rpu_flags));
	    field_tree = proto_item_add_subtree(tf, ett_icmpv6flag);
	    flags = tvb_get_guint8(tvb, flagoff);
	    proto_tree_add_text(field_tree, tvb, flagoff, 4, "%s",
		decode_boolean_bitfield(flags,
		    ICMP6_RR_PCOUSE_FLAGS_DECRVLTIME, 32,
		    "Decrement valid lifetime", "No decrement valid lifetime"));
	    proto_tree_add_text(field_tree, tvb, flagoff, 4, "%s",
		decode_boolean_bitfield(flags,
		    ICMP6_RR_PCOUSE_FLAGS_DECRPLTIME, 32,
		    "Decrement preferred lifetime",
		    "No decrement preferred lifetime"));
	    proto_tree_add_text(opt_tree, tvb,
		off + offsetof(struct rr_pco_use, rpu_prefix),
		sizeof(use->rpu_prefix), "UsePrefix: %s",
		ip6_to_str(&use->rpu_prefix));
	}

    }
}

static void
dissect_icmpv6(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
    proto_tree *icmp6_tree, *field_tree;
    proto_item *ti, *tf = NULL;
    struct icmp6_hdr icmp6_hdr, *dp;
    struct icmp6_nodeinfo *ni = NULL;
    char *codename, *typename;
    char *colcodename, *coltypename;
    int len;
    guint length, reported_length;
    vec_t cksum_vec[4];
    guint32 phdr[2];
    guint16 cksum, computed_cksum;
    int offset;
    tvbuff_t *next_tvb;

    if (check_col(pinfo->cinfo, COL_PROTOCOL))
	col_set_str(pinfo->cinfo, COL_PROTOCOL, "ICMPv6");
    if (check_col(pinfo->cinfo, COL_INFO))
	col_clear(pinfo->cinfo, COL_INFO);

    offset = 0;
    tvb_memcpy(tvb, (guint8 *)&icmp6_hdr, offset, sizeof icmp6_hdr);
    dp = &icmp6_hdr;
    codename = typename = colcodename = coltypename = "Unknown";
    len = sizeof(*dp);
    switch (dp->icmp6_type) {
    case ICMP6_DST_UNREACH:
	typename = coltypename = "Unreachable";
	switch (dp->icmp6_code) {
	case ICMP6_DST_UNREACH_NOROUTE:
	    codename = colcodename = "Route unreachable";
	    break;
	case ICMP6_DST_UNREACH_ADMIN:
	    codename = colcodename = "Administratively prohibited";
	    break;
	case ICMP6_DST_UNREACH_NOTNEIGHBOR:
	    codename = colcodename = "Not a neighbor";
	    break;
	case ICMP6_DST_UNREACH_ADDR:
	    codename = colcodename = "Address unreachable";
	    break;
	case ICMP6_DST_UNREACH_NOPORT:
	    codename = colcodename = "Port unreachable";
	    break;
	}
	break;
    case ICMP6_PACKET_TOO_BIG:
	typename = coltypename = "Too big";
	codename = colcodename = NULL;
	break;
    case ICMP6_TIME_EXCEEDED:
	typename = coltypename = "Time exceeded";
	switch (dp->icmp6_code) {
	case ICMP6_TIME_EXCEED_TRANSIT:
	    codename = colcodename = "In-transit";
	    break;
	case ICMP6_TIME_EXCEED_REASSEMBLY:
	    codename = colcodename = "Reassembly";
	    break;
	}
        break;
    case ICMP6_PARAM_PROB:
	typename = coltypename = "Parameter problem";
	switch (dp->icmp6_code) {
	case ICMP6_PARAMPROB_HEADER:
	    codename = colcodename = "Header";
	    break;
	case ICMP6_PARAMPROB_NEXTHEADER:
	    codename = colcodename = "Next header";
	    break;
	case ICMP6_PARAMPROB_OPTION:
	    codename = colcodename = "Option";
	    break;
	}
        break;
    case ICMP6_ECHO_REQUEST:
	typename = coltypename = "Echo request";
	codename = colcodename = NULL;
	break;
    case ICMP6_ECHO_REPLY:
	typename = coltypename = "Echo reply";
	codename = colcodename = NULL;
	break;
    case ICMP6_MEMBERSHIP_QUERY:
	typename = coltypename = "Multicast listener query";
	codename = colcodename = NULL;
	break;
    case ICMP6_MEMBERSHIP_REPORT:
	typename = coltypename = "Multicast listener report";
	codename = colcodename = NULL;
	break;
    case ICMP6_MEMBERSHIP_REDUCTION:
	typename = coltypename = "Multicast listener done";
	codename = colcodename = NULL;
	break;
    case ND_ROUTER_SOLICIT:
	typename = coltypename = "Router solicitation";
	codename = colcodename = NULL;
	len = sizeof(struct nd_router_solicit);
	break;
    case ND_ROUTER_ADVERT:
	typename = coltypename = "Router advertisement";
	codename = colcodename = NULL;
	len = sizeof(struct nd_router_advert);
	break;
    case ND_NEIGHBOR_SOLICIT:
	typename = coltypename = "Neighbor solicitation";
	codename = colcodename = NULL;
	len = sizeof(struct nd_neighbor_solicit);
	break;
    case ND_NEIGHBOR_ADVERT:
	typename = coltypename = "Neighbor advertisement";
	codename = colcodename = NULL;
	len = sizeof(struct nd_neighbor_advert);
	break;
    case ND_REDIRECT:
	typename = coltypename = "Redirect";
	codename = colcodename = NULL;
	len = sizeof(struct nd_redirect);
	break;
    case ICMP6_ROUTER_RENUMBERING:
	typename = coltypename = "Router renumbering";
	switch (dp->icmp6_code) {
	case ICMP6_ROUTER_RENUMBERING_COMMAND:
	    codename = colcodename = "Command";
	    break;
	case ICMP6_ROUTER_RENUMBERING_RESULT:
	    codename = colcodename = "Result";
	    break;
	case ICMP6_ROUTER_RENUMBERING_SEQNUM_RESET:
	    codename = colcodename = "Sequence number reset";
	    break;
	}
	len = sizeof(struct icmp6_router_renum);
	break;
    case ICMP6_NI_QUERY:
    case ICMP6_NI_REPLY:
	ni = (struct icmp6_nodeinfo *)dp;
	if (ni->ni_type == ICMP6_NI_QUERY) {
	    typename = coltypename = "Node information query";
	    switch (ni->ni_code) {
	    case ICMP6_NI_SUBJ_IPV6:
		codename = "Query subject = IPv6 addresses";
		break;
	    case ICMP6_NI_SUBJ_FQDN:
		if (tvb_bytes_exist(tvb, offset, sizeof(*ni)))
		    codename = "Query subject = DNS name";
		else
		    codename = "Query subject = empty";
		break;
	    case ICMP6_NI_SUBJ_IPV4:
		codename = "Query subject = IPv4 addresses";
		break;
	    }
	} else {
	    typename = coltypename = "Node information reply";
	    switch (ni->ni_code) {
	    case ICMP6_NI_SUCCESS:
		codename = "Successful";
		break;
	    case ICMP6_NI_REFUSED:
		codename = "Refused";
		break;
	    case ICMP6_NI_UNKNOWN:
		codename = "Unknown query type";
		break;
	    }
	}
	colcodename = val_to_str(pntohs(&ni->ni_qtype), names_nodeinfo_qtype,
	    "Unknown");
	len = sizeof(struct icmp6_nodeinfo);
	break;
    }

    if (check_col(pinfo->cinfo, COL_INFO)) {
	char typebuf[256], codebuf[256];

	if (coltypename && strcmp(coltypename, "Unknown") == 0) {
	    snprintf(typebuf, sizeof(typebuf), "Unknown (0x%02x)",
		dp->icmp6_type);
	    coltypename = typebuf;
	}
	if (colcodename && strcmp(colcodename, "Unknown") == 0) {
	    snprintf(codebuf, sizeof(codebuf), "Unknown (0x%02x)",
		dp->icmp6_code);
	    colcodename = codebuf;
	}
	if (colcodename) {
	    col_add_fstr(pinfo->cinfo, COL_INFO, "%s (%s)", coltypename, colcodename);
	} else {
	    col_add_fstr(pinfo->cinfo, COL_INFO, "%s", coltypename);
	}
    }

    if (tree) {
	/* !!! specify length */
	ti = proto_tree_add_item(tree, proto_icmpv6, tvb, offset, len, FALSE);
	icmp6_tree = proto_item_add_subtree(ti, ett_icmpv6);

	proto_tree_add_uint_format(icmp6_tree, hf_icmpv6_type, tvb,
	    offset + offsetof(struct icmp6_hdr, icmp6_type), 1,
	    dp->icmp6_type,
	    "Type: %u (%s)", dp->icmp6_type, typename);
	if (codename) {
	    proto_tree_add_uint_format(icmp6_tree, hf_icmpv6_code, tvb,
		offset + offsetof(struct icmp6_hdr, icmp6_code), 1,
		dp->icmp6_code,
		"Code: %u (%s)", dp->icmp6_code, codename);
	} else {
	    proto_tree_add_uint_format(icmp6_tree, hf_icmpv6_code, tvb,
		offset + offsetof(struct icmp6_hdr, icmp6_code), 1,
		dp->icmp6_code,
		"Code: %u", dp->icmp6_code);
	}
	cksum = (guint16)htons(dp->icmp6_cksum);
	length = tvb_length(tvb);
	reported_length = tvb_reported_length(tvb);
	if (!pinfo->fragmented && length >= reported_length) {
	    /* The packet isn't part of a fragmented datagram and isn't
	       truncated, so we can checksum it. */

	    /* Set up the fields of the pseudo-header. */
	    cksum_vec[0].ptr = pinfo->src.data;
	    cksum_vec[0].len = pinfo->src.len;
	    cksum_vec[1].ptr = pinfo->dst.data;
	    cksum_vec[1].len = pinfo->dst.len;
	    cksum_vec[2].ptr = (const guint8 *)&phdr;
	    phdr[0] = htonl(tvb_reported_length(tvb));
	    phdr[1] = htonl(IP_PROTO_ICMPV6);
	    cksum_vec[2].len = 8;
	    cksum_vec[3].len = tvb_reported_length(tvb);
	    cksum_vec[3].ptr = tvb_get_ptr(tvb, offset, cksum_vec[3].len);
	    computed_cksum = in_cksum(cksum_vec, 4);
	    if (computed_cksum == 0) {
		proto_tree_add_uint_format(icmp6_tree, hf_icmpv6_checksum,
			tvb,
			offset + offsetof(struct icmp6_hdr, icmp6_cksum), 2,
			cksum,
			"Checksum: 0x%04x (correct)", cksum);
	    } else {
		proto_tree_add_boolean_hidden(icmp6_tree, hf_icmpv6_checksum_bad,
			tvb,
			offset + offsetof(struct icmp6_hdr, icmp6_cksum), 2,
			TRUE);
		proto_tree_add_uint_format(icmp6_tree, hf_icmpv6_checksum,
			tvb,
			offset + offsetof(struct icmp6_hdr, icmp6_cksum), 2,
			cksum,
			"Checksum: 0x%04x (incorrect, should be 0x%04x)",
			cksum, in_cksum_shouldbe(cksum, computed_cksum));
	    }
	} else {
	    proto_tree_add_uint(icmp6_tree, hf_icmpv6_checksum, tvb,
		offset + offsetof(struct icmp6_hdr, icmp6_cksum), 2,
		cksum);
	}

	/* decode... */
	switch (dp->icmp6_type) {
	case ICMP6_DST_UNREACH:
	case ICMP6_TIME_EXCEEDED:
	    dissect_contained_icmpv6(tvb, offset + sizeof(*dp), pinfo,
		icmp6_tree);
	    break;
	case ICMP6_PACKET_TOO_BIG:
	    proto_tree_add_text(icmp6_tree, tvb,
		offset + offsetof(struct icmp6_hdr, icmp6_mtu), 4,
		"MTU: %u", pntohl(&dp->icmp6_mtu));
	    dissect_contained_icmpv6(tvb, offset + sizeof(*dp), pinfo,
		icmp6_tree);
	    break;
	case ICMP6_PARAM_PROB:
	    proto_tree_add_text(icmp6_tree, tvb,
		offset + offsetof(struct icmp6_hdr, icmp6_pptr), 4,
		"Problem pointer: 0x%04x", pntohl(&dp->icmp6_pptr));
	    dissect_contained_icmpv6(tvb, offset + sizeof(*dp), pinfo,
		icmp6_tree);
	    break;
	case ICMP6_ECHO_REQUEST:
	case ICMP6_ECHO_REPLY:
	    proto_tree_add_text(icmp6_tree, tvb,
		offset + offsetof(struct icmp6_hdr, icmp6_id), 2,
		"ID: 0x%04x", (guint16)ntohs(dp->icmp6_id));
	    proto_tree_add_text(icmp6_tree, tvb,
		offset + offsetof(struct icmp6_hdr, icmp6_seq), 2,
		"Sequence: 0x%04x", (guint16)ntohs(dp->icmp6_seq));
	    next_tvb = tvb_new_subset(tvb, offset + sizeof(*dp), -1, -1);
	    call_dissector(data_handle,next_tvb, pinfo, icmp6_tree);
	    break;
	case ICMP6_MEMBERSHIP_QUERY:
	case ICMP6_MEMBERSHIP_REPORT:
	case ICMP6_MEMBERSHIP_REDUCTION:
	    proto_tree_add_text(icmp6_tree, tvb,
		offset + offsetof(struct icmp6_hdr, icmp6_maxdelay), 2,
		"Maximum response delay: %u",
		(guint16)ntohs(dp->icmp6_maxdelay));
	    proto_tree_add_text(icmp6_tree, tvb, offset + sizeof(*dp), 16,
		"Multicast Address: %s",
		ip6_to_str((struct e_in6_addr *)(tvb_get_ptr(tvb, offset + sizeof *dp, sizeof (struct e_in6_addr)))));
	    break;
	case ND_ROUTER_SOLICIT:
	    dissect_icmpv6opt(tvb, offset + sizeof(*dp), pinfo, icmp6_tree);
	    break;
	case ND_ROUTER_ADVERT:
	  {
	    struct nd_router_advert nd_router_advert, *ra;
	    int flagoff;
	    guint32 ra_flags;

	    ra = &nd_router_advert;
	    tvb_memcpy(tvb, (guint8 *)ra, offset, sizeof *ra);
	    proto_tree_add_text(icmp6_tree, tvb,
		offset + offsetof(struct nd_router_advert, nd_ra_curhoplimit),
		1, "Cur hop limit: %u", ra->nd_ra_curhoplimit);

	    flagoff = offset + offsetof(struct nd_router_advert, nd_ra_flags_reserved);
	    ra_flags = tvb_get_guint8(tvb, flagoff);
	    tf = proto_tree_add_text(icmp6_tree, tvb, flagoff, 1, "Flags: 0x%02x", ra_flags);
	    field_tree = proto_item_add_subtree(tf, ett_icmpv6flag);
	    proto_tree_add_text(field_tree, tvb, flagoff, 1, "%s",
		decode_boolean_bitfield(ra_flags,
			ND_RA_FLAG_MANAGED, 8, "Managed", "Not managed"));
	    proto_tree_add_text(field_tree, tvb, flagoff, 1, "%s",
		decode_boolean_bitfield(ra_flags,
			ND_RA_FLAG_OTHER, 8, "Other", "Not other"));
	    proto_tree_add_text(field_tree, tvb, flagoff, 1, "%s",
		decode_boolean_bitfield(ra_flags,
			ND_RA_FLAG_HOME_AGENT, 8,
			"Home Agent", "Not Home Agent"));		
	    proto_tree_add_text(field_tree, tvb, flagoff, 1, "%s",
		decode_enumerated_bitfield(ra_flags, ND_RA_FLAG_RTPREF_MASK, 8,
		names_router_pref, "Router preference: %s"));
	    proto_tree_add_text(icmp6_tree, tvb,
		offset + offsetof(struct nd_router_advert, nd_ra_router_lifetime),
		2, "Router lifetime: %u",
		(guint16)ntohs(ra->nd_ra_router_lifetime));
	    proto_tree_add_text(icmp6_tree, tvb,
		offset + offsetof(struct nd_router_advert, nd_ra_reachable), 4,
		"Reachable time: %u", pntohl(&ra->nd_ra_reachable));
	    proto_tree_add_text(icmp6_tree, tvb,
		offset + offsetof(struct nd_router_advert, nd_ra_retransmit), 4,
		"Retrans time: %u", pntohl(&ra->nd_ra_retransmit));
	    dissect_icmpv6opt(tvb, offset + sizeof(struct nd_router_advert), pinfo, icmp6_tree);
	    break;
	  }
	case ND_NEIGHBOR_SOLICIT:
	  {
	    struct nd_neighbor_solicit nd_neighbor_solicit, *ns;

	    ns = &nd_neighbor_solicit;
	    tvb_memcpy(tvb, (guint8 *)ns, offset, sizeof *ns);
	    proto_tree_add_text(icmp6_tree, tvb,
			offset + offsetof(struct nd_neighbor_solicit, nd_ns_target), 16,
#ifdef INET6
			"Target: %s (%s)",
			get_hostname6(&ns->nd_ns_target),
#else
			"Target: %s",
#endif
			ip6_to_str(&ns->nd_ns_target));

	    dissect_icmpv6opt(tvb, offset + sizeof(*ns), pinfo, icmp6_tree);
	    break;
	  }
	case ND_NEIGHBOR_ADVERT:
	  {
	    int flagoff, targetoff;
	    guint32 na_flags;
	    struct e_in6_addr na_target;

	    flagoff = offset + offsetof(struct nd_neighbor_advert, nd_na_flags_reserved);
	    na_flags = tvb_get_ntohl(tvb, flagoff);

	    tf = proto_tree_add_text(icmp6_tree, tvb, flagoff, 4, "Flags: 0x%08x", na_flags);
	    field_tree = proto_item_add_subtree(tf, ett_icmpv6flag);
	    proto_tree_add_text(field_tree, tvb, flagoff, 4, "%s",
		decode_boolean_bitfield(na_flags,
			ND_NA_FLAG_ROUTER, 32, "Router", "Not router"));
	    proto_tree_add_text(field_tree, tvb, flagoff, 4, "%s",
		decode_boolean_bitfield(na_flags,
			ND_NA_FLAG_SOLICITED, 32, "Solicited", "Not adverted"));
	    proto_tree_add_text(field_tree, tvb, flagoff, 4, "%s",
		decode_boolean_bitfield(na_flags,
			ND_NA_FLAG_OVERRIDE, 32, "Override", "Not override"));

	    targetoff = offset + offsetof(struct nd_neighbor_advert, nd_na_target);
	    tvb_memcpy(tvb, (guint8 *)&na_target, targetoff, sizeof na_target);
	    proto_tree_add_text(icmp6_tree, tvb, targetoff, 16,
#ifdef INET6
			"Target: %s (%s)",
			get_hostname6(&na_target),
#else
			"Target: %s",
#endif
			ip6_to_str(&na_target));

	    dissect_icmpv6opt(tvb, offset + sizeof(struct nd_neighbor_advert), pinfo, icmp6_tree);
	    break;
	  }
	case ND_REDIRECT:
	  {
	    struct nd_redirect nd_redirect, *rd;

	    rd = &nd_redirect;
	    tvb_memcpy(tvb, (guint8 *)rd, offset, sizeof *rd);
	    proto_tree_add_text(icmp6_tree, tvb,
			offset + offsetof(struct nd_redirect, nd_rd_target), 16,
#ifdef INET6
			"Target: %s (%s)",
			get_hostname6(&rd->nd_rd_target),
#else
			"Target: %s",
#endif
			ip6_to_str(&rd->nd_rd_target));

	    proto_tree_add_text(icmp6_tree, tvb,
			offset + offsetof(struct nd_redirect, nd_rd_dst), 16,
#ifdef INET6
			"Destination: %s (%s)",
			get_hostname6(&rd->nd_rd_dst),
#else
			"Destination: %s",
#endif
			ip6_to_str(&rd->nd_rd_dst));

	    dissect_icmpv6opt(tvb, offset + sizeof(*rd), pinfo, icmp6_tree);
	    break;
	  }
	case ICMP6_ROUTER_RENUMBERING:
	    dissect_rrenum(tvb, offset, pinfo, icmp6_tree);
	    break;
	case ICMP6_NI_QUERY:
	case ICMP6_NI_REPLY:
	    ni = (struct icmp6_nodeinfo *)dp;
	    proto_tree_add_text(icmp6_tree, tvb,
		offset + offsetof(struct icmp6_nodeinfo, ni_qtype),
		sizeof(ni->ni_qtype),
		"Query type: 0x%04x (%s)", pntohs(&ni->ni_qtype),
		val_to_str(pntohs(&ni->ni_qtype), names_nodeinfo_qtype,
		"Unknown"));
	    dissect_nodeinfo(tvb, offset, pinfo, icmp6_tree);
	    break;
	default:
	    next_tvb = tvb_new_subset(tvb, offset + sizeof(*dp), -1, -1);
	    call_dissector(data_handle,next_tvb, pinfo, tree);
	    break;
	}
    }
}

void
proto_register_icmpv6(void)
{
  static hf_register_info hf[] = {
    { &hf_icmpv6_type,
      { "Type",           "icmpv6.type",	FT_UINT8,  BASE_DEC, NULL, 0x0,
      	"", HFILL }},
    { &hf_icmpv6_code,
      { "Code",           "icmpv6.code",	FT_UINT8,  BASE_DEC, NULL, 0x0,
      	"", HFILL }},
    { &hf_icmpv6_checksum,
      { "Checksum",       "icmpv6.checksum",	FT_UINT16, BASE_HEX, NULL, 0x0,
      	"", HFILL }},
    { &hf_icmpv6_checksum_bad,
      { "Bad Checksum",   "icmpv6.checksum_bad", FT_BOOLEAN, BASE_NONE,	NULL, 0x0,
	"", HFILL }},
  };
  static gint *ett[] = {
    &ett_icmpv6,
    &ett_icmpv6opt,
    &ett_icmpv6flag,
    &ett_nodeinfo_flag,
    &ett_nodeinfo_subject4,
    &ett_nodeinfo_subject6,
    &ett_nodeinfo_node4,
    &ett_nodeinfo_node6,
    &ett_nodeinfo_nodebitmap,
    &ett_nodeinfo_nodedns,
  };

  proto_icmpv6 = proto_register_protocol("Internet Control Message Protocol v6",
					 "ICMPv6", "icmpv6");
  proto_register_field_array(proto_icmpv6, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));
}

void
proto_reg_handoff_icmpv6(void)
{
  dissector_handle_t icmpv6_handle;

  icmpv6_handle = create_dissector_handle(dissect_icmpv6, proto_icmpv6);
  dissector_add("ip.proto", IP_PROTO_ICMPV6, icmpv6_handle);

  /*
   * Get a handle for the IPv6 dissector.
   */
  ipv6_handle = find_dissector("ipv6");
  data_handle = find_dissector("data");
}
