/* packet-ip.c
 * Routines for IP and miscellaneous IP protocol packet disassembly
 *
 * $Id: packet-ip.c,v 1.127 2001/03/15 09:11:00 guy Exp $
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@zing.org>
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

#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif

#include <stdio.h>
#include <string.h>
#include <glib.h>

#ifdef NEED_SNPRINTF_H
# include "snprintf.h"
#endif

#include "packet.h"
#include "resolv.h"
#include "prefs.h"
#include "etypes.h"
#include "greproto.h"
#include "ppptypes.h"
#include "llcsaps.h"
#include "aftypes.h"
#include "packet-ip.h"
#include "packet-ipsec.h"
#include "in_cksum.h"
#include "nlpid.h"

static void dissect_icmp(tvbuff_t *, packet_info *, proto_tree *);
static void dissect_igmp(tvbuff_t *, packet_info *, proto_tree *);

/* Decode the old IPv4 TOS field as the DiffServ DS Field */
static gboolean g_ip_dscp_actif = TRUE;

static int proto_ip = -1;
static int hf_ip_version = -1;
static int hf_ip_hdr_len = -1;
static int hf_ip_dsfield = -1;
static int hf_ip_dsfield_dscp = -1;
static int hf_ip_dsfield_ect = -1;
static int hf_ip_dsfield_ce = -1;
static int hf_ip_tos = -1;
static int hf_ip_tos_precedence = -1;
static int hf_ip_tos_delay = -1;
static int hf_ip_tos_throughput = -1;
static int hf_ip_tos_reliability = -1;
static int hf_ip_tos_cost = -1;
static int hf_ip_len = -1;
static int hf_ip_id = -1;
static int hf_ip_dst = -1;
static int hf_ip_src = -1;
static int hf_ip_addr = -1;
static int hf_ip_flags = -1;
static int hf_ip_flags_df = -1;
static int hf_ip_flags_mf = -1;
static int hf_ip_frag_offset = -1;
static int hf_ip_ttl = -1;
static int hf_ip_proto = -1;
static int hf_ip_checksum = -1;
static int hf_ip_checksum_bad = -1;

static gint ett_ip = -1;
static gint ett_ip_dsfield = -1;
static gint ett_ip_tos = -1;
static gint ett_ip_off = -1;
static gint ett_ip_options = -1;
static gint ett_ip_option_sec = -1;
static gint ett_ip_option_route = -1;
static gint ett_ip_option_timestamp = -1;

/* Used by IPv6 as well, so not static */
dissector_table_t ip_dissector_table;

static int proto_igmp = -1;
static int hf_igmp_version = -1;
static int hf_igmp_type = -1;
static int hf_igmp_unused = -1;
static int hf_igmp_checksum = -1;
static int hf_igmp_group = -1;

static gint ett_igmp = -1;

static int proto_icmp = -1;
static int hf_icmp_type = -1;
static int hf_icmp_code = -1;
static int hf_icmp_checksum = -1;
static int hf_icmp_checksum_bad = -1;

static gint ett_icmp = -1;

/* ICMP definitions */

#define ICMP_ECHOREPLY     0
#define ICMP_UNREACH       3
#define ICMP_SOURCEQUENCH  4
#define ICMP_REDIRECT      5
#define ICMP_ECHO          8
#define ICMP_RTRADVERT     9
#define ICMP_RTRSOLICIT   10
#define ICMP_TIMXCEED     11
#define ICMP_PARAMPROB    12
#define ICMP_TSTAMP       13
#define ICMP_TSTAMPREPLY  14
#define ICMP_IREQ         15
#define ICMP_IREQREPLY    16
#define ICMP_MASKREQ      17
#define ICMP_MASKREPLY    18

/* ICMP UNREACHABLE */

#define ICMP_NET_UNREACH        0       /* Network Unreachable */
#define ICMP_HOST_UNREACH       1       /* Host Unreachable */
#define ICMP_PROT_UNREACH       2       /* Protocol Unreachable */
#define ICMP_PORT_UNREACH       3       /* Port Unreachable */
#define ICMP_FRAG_NEEDED        4       /* Fragmentation Needed/DF set */
#define ICMP_SR_FAILED          5       /* Source Route failed */
#define ICMP_NET_UNKNOWN        6
#define ICMP_HOST_UNKNOWN       7
#define ICMP_HOST_ISOLATED      8
#define ICMP_NET_ANO            9
#define ICMP_HOST_ANO           10
#define ICMP_NET_UNR_TOS        11
#define ICMP_HOST_UNR_TOS       12
#define ICMP_PKT_FILTERED       13      /* Packet filtered */
#define ICMP_PREC_VIOLATION     14      /* Precedence violation */
#define ICMP_PREC_CUTOFF        15      /* Precedence cut off */


/* IGMP structs and definitions */
typedef struct _e_igmp {
  guint8  igmp_v_t; /* combines igmp_v and igmp_t */
  guint8  igmp_unused;
  guint16 igmp_cksum;
  guint32 igmp_gaddr;
} e_igmp;

#define IGMP_M_QRY     0x01
#define IGMP_V1_M_RPT  0x02
#define IGMP_V2_LV_GRP 0x07
#define IGMP_DVMRP     0x03
#define IGMP_PIM       0x04
#define IGMP_V2_M_RPT  0x06
#define IGMP_MTRC_RESP 0x1e
#define IGMP_MTRC      0x1f

/* IP structs and definitions */

typedef struct _e_ip 
   {
   guint8  ip_v_hl; /* combines ip_v and ip_hl */
   guint8  ip_tos;
   guint16 ip_len;
   guint16 ip_id;
   guint16 ip_off;
   guint8  ip_ttl;
   guint8  ip_p;
   guint16 ip_sum;
   guint32 ip_src;
   guint32 ip_dst;
   } e_ip;

/* Offsets of fields within an IP header. */
#define	IPH_V_HL	0
#define	IPH_TOS		1
#define	IPH_LEN		2
#define	IPH_ID		4
#define	IPH_TTL		6
#define	IPH_OFF		8
#define	IPH_P		9
#define	IPH_SUM		10
#define	IPH_SRC		12
#define	IPH_DST		16

/* Minimum IP header length. */
#define	IPH_MIN_LEN	20

/* IP flags. */
#define IP_CE		0x8000		/* Flag: "Congestion"		*/
#define IP_DF		0x4000		/* Flag: "Don't Fragment"	*/
#define IP_MF		0x2000		/* Flag: "More Fragments"	*/
#define IP_OFFSET	0x1FFF		/* "Fragment Offset" part	*/

/* Differentiated Services Field. See RFCs 2474, 2597 and 2598. */
#define IPDSFIELD_DSCP_MASK     0xFC
#define IPDSFIELD_ECN_MASK     0x03
#define IPDSFIELD_DSCP_SHIFT	2
#define IPDSFIELD_DSCP(dsfield)	(((dsfield)&IPDSFIELD_DSCP_MASK)>>IPDSFIELD_DSCP_SHIFT)
#define IPDSFIELD_ECN(dsfield)	((dsfield)&IPDSFIELD_ECN_MASK)
#define IPDSFIELD_DSCP_DEFAULT  0x00
#define IPDSFIELD_DSCP_CS1      0x08
#define IPDSFIELD_DSCP_CS2      0x10
#define IPDSFIELD_DSCP_CS3      0x18
#define IPDSFIELD_DSCP_CS4      0x20
#define IPDSFIELD_DSCP_CS5      0x28
#define IPDSFIELD_DSCP_CS6      0x30
#define IPDSFIELD_DSCP_CS7      0x38
#define IPDSFIELD_DSCP_AF11     0x0A
#define IPDSFIELD_DSCP_AF12     0x0C
#define IPDSFIELD_DSCP_AF13     0x0E
#define IPDSFIELD_DSCP_AF21     0x12
#define IPDSFIELD_DSCP_AF22     0x14
#define IPDSFIELD_DSCP_AF23     0x16
#define IPDSFIELD_DSCP_AF31     0x1A
#define IPDSFIELD_DSCP_AF32     0x1C
#define IPDSFIELD_DSCP_AF33     0x1E
#define IPDSFIELD_DSCP_AF41     0x22
#define IPDSFIELD_DSCP_AF42     0x24
#define IPDSFIELD_DSCP_AF43     0x26
#define IPDSFIELD_DSCP_EF       0x2E
#define IPDSFIELD_ECT_MASK	0x02
#define IPDSFIELD_CE_MASK	0x01

/* IP TOS, superseded by the DS Field, RFC 2474. */
#define IPTOS_TOS_MASK    0x1E
#define IPTOS_TOS(tos)    ((tos) & IPTOS_TOS_MASK)
#define IPTOS_NONE        0x00
#define IPTOS_LOWCOST     0x02
#define IPTOS_RELIABILITY 0x04
#define IPTOS_THROUGHPUT  0x08
#define IPTOS_LOWDELAY    0x10
#define IPTOS_SECURITY    0x1E

#define IPTOS_PREC_MASK		0xE0
#define IPTOS_PREC_SHIFT	5
#define IPTOS_PREC(tos)		(((tos)&IPTOS_PREC_MASK)>>IPTOS_PREC_SHIFT)
#define IPTOS_PREC_NETCONTROL           7
#define IPTOS_PREC_INTERNETCONTROL      6
#define IPTOS_PREC_CRITIC_ECP           5
#define IPTOS_PREC_FLASHOVERRIDE        4
#define IPTOS_PREC_FLASH                3
#define IPTOS_PREC_IMMEDIATE            2
#define IPTOS_PREC_PRIORITY             1
#define IPTOS_PREC_ROUTINE              0

/* IP options */
#define IPOPT_COPY		0x80

#define	IPOPT_CONTROL		0x00
#define	IPOPT_RESERVED1		0x20
#define	IPOPT_MEASUREMENT	0x40
#define	IPOPT_RESERVED2		0x60

#define IPOPT_END	(0 |IPOPT_CONTROL)
#define IPOPT_NOOP	(1 |IPOPT_CONTROL)
#define IPOPT_SEC	(2 |IPOPT_CONTROL|IPOPT_COPY)
#define IPOPT_LSRR	(3 |IPOPT_CONTROL|IPOPT_COPY)
#define IPOPT_TIMESTAMP	(4 |IPOPT_MEASUREMENT)
#define IPOPT_RR	(7 |IPOPT_CONTROL)
#define IPOPT_SID	(8 |IPOPT_CONTROL|IPOPT_COPY)
#define IPOPT_SSRR	(9 |IPOPT_CONTROL|IPOPT_COPY)
#define IPOPT_RA	(20|IPOPT_CONTROL|IPOPT_COPY)

/* IP option lengths */
#define IPOLEN_SEC      11
#define IPOLEN_LSRR_MIN 3
#define IPOLEN_TIMESTAMP_MIN 5
#define IPOLEN_RR_MIN   3
#define IPOLEN_SID      4
#define IPOLEN_SSRR_MIN 3
#define IPOLEN_RA       4

#define IPSEC_UNCLASSIFIED	0x0000
#define	IPSEC_CONFIDENTIAL	0xF135
#define	IPSEC_EFTO		0x789A
#define	IPSEC_MMMM		0xBC4D
#define	IPSEC_RESTRICTED	0xAF13
#define	IPSEC_SECRET		0xD788
#define	IPSEC_TOPSECRET		0x6BC5
#define	IPSEC_RESERVED1		0x35E2
#define	IPSEC_RESERVED2		0x9AF1
#define	IPSEC_RESERVED3		0x4D78
#define	IPSEC_RESERVED4		0x24BD
#define	IPSEC_RESERVED5		0x135E
#define	IPSEC_RESERVED6		0x89AF
#define	IPSEC_RESERVED7		0xC4D6
#define	IPSEC_RESERVED8		0xE26B

#define	IPOPT_TS_TSONLY		0		/* timestamps only */
#define	IPOPT_TS_TSANDADDR	1		/* timestamps and addresses */
#define	IPOPT_TS_PRESPEC	3		/* specified modules only */


void
capture_ip(const u_char *pd, int offset, packet_counts *ld) {
  if (!BYTES_ARE_IN_FRAME(offset, IPH_MIN_LEN)) {
    ld->other++;
    return;
  }
  switch (pd[offset + 9]) {
    case IP_PROTO_SCTP:
      ld->sctp++;
      break;
    case IP_PROTO_TCP:
      ld->tcp++;
      break;
    case IP_PROTO_UDP:
      ld->udp++;
      break;
    case IP_PROTO_ICMP:
      ld->icmp++;
      break;
    case IP_PROTO_OSPF:
      ld->ospf++;
      break;
    case IP_PROTO_GRE:
      ld->gre++;
      break;
    case IP_PROTO_VINES:
      ld->vines++;
      break;
    default:
      ld->other++;
  }
}

static void
dissect_ipopt_security(const ip_tcp_opt *optp, tvbuff_t *tvb, int offset,
			guint optlen, frame_data *fd, proto_tree *opt_tree)
{
  proto_tree *field_tree = NULL;
  proto_item *tf;
  guint      val;
  static const value_string secl_vals[] = {
    {IPSEC_UNCLASSIFIED, "Unclassified"},
    {IPSEC_CONFIDENTIAL, "Confidential"},
    {IPSEC_EFTO,         "EFTO"        },
    {IPSEC_MMMM,         "MMMM"        },
    {IPSEC_RESTRICTED,   "Restricted"  },
    {IPSEC_SECRET,       "Secret"      },
    {IPSEC_TOPSECRET,    "Top secret"  },
    {IPSEC_RESERVED1,    "Reserved"    },
    {IPSEC_RESERVED2,    "Reserved"    },
    {IPSEC_RESERVED3,    "Reserved"    },
    {IPSEC_RESERVED4,    "Reserved"    },
    {IPSEC_RESERVED5,    "Reserved"    },
    {IPSEC_RESERVED6,    "Reserved"    },
    {IPSEC_RESERVED7,    "Reserved"    },
    {IPSEC_RESERVED8,    "Reserved"    },
    {0,                  NULL          } };

  tf = proto_tree_add_text(opt_tree, tvb, offset,      optlen, "%s:", optp->name);
  field_tree = proto_item_add_subtree(tf, *optp->subtree_index);
  offset += 2;

  val = tvb_get_ntohs(tvb, offset);
  proto_tree_add_text(field_tree, tvb, offset,       2,
              "Security: %s", val_to_str(val, secl_vals, "Unknown (0x%x)"));
  offset += 2;

  val = tvb_get_ntohs(tvb, offset);
  proto_tree_add_text(field_tree, tvb, offset,         2,
              "Compartments: %u", val);
  offset += 2;

  proto_tree_add_text(field_tree, tvb, offset,         2,
              "Handling restrictions: %c%c",
	      tvb_get_guint8(tvb, offset),
	      tvb_get_guint8(tvb, offset + 1));
  offset += 2;

  proto_tree_add_text(field_tree, tvb, offset,         3,
              "Transmission control code: %c%c%c",
	      tvb_get_guint8(tvb, offset),
	      tvb_get_guint8(tvb, offset + 1),
	      tvb_get_guint8(tvb, offset + 2));
}

static void
dissect_ipopt_route(const ip_tcp_opt *optp, tvbuff_t *tvb, int offset,
			guint optlen, frame_data *fd, proto_tree *opt_tree)
{
  proto_tree *field_tree = NULL;
  proto_item *tf;
  int ptr;
  int optoffset = 0;
  struct in_addr addr;

  tf = proto_tree_add_text(opt_tree, tvb, offset,      optlen, "%s (%u bytes)",
				optp->name, optlen);
  field_tree = proto_item_add_subtree(tf, *optp->subtree_index);

  optoffset += 2;	/* skip past type and length */
  optlen -= 2;		/* subtract size of type and length */

  ptr = tvb_get_guint8(tvb, offset + optoffset);
  proto_tree_add_text(field_tree, tvb, offset + optoffset, 1,
              "Pointer: %d%s", ptr,
              ((ptr < 4) ? " (points before first address)" :
               ((ptr & 3) ? " (points to middle of address)" : "")));
  optoffset++;
  optlen--;
  ptr--;	/* ptr is 1-origin */

  while (optlen > 0) {
    if (optlen < 4) {
      proto_tree_add_text(field_tree, tvb, offset,      optlen,
        "(suboption would go past end of option)");
      break;
    }

    /* Avoids alignment problems on many architectures. */
    tvb_memcpy(tvb, (guint8 *)&addr, offset + optoffset, sizeof(addr));

    proto_tree_add_text(field_tree, tvb, offset + optoffset, 4,
              "%s%s",
              ((addr.s_addr == 0) ? "-" : (char *)get_hostname(addr.s_addr)),
              ((optoffset == ptr) ? " <- (current)" : ""));
    optoffset += 4;
    optlen -= 4;
  }
}

static void
dissect_ipopt_sid(const ip_tcp_opt *optp, tvbuff_t *tvb, int offset,
			guint optlen, frame_data *fd, proto_tree *opt_tree)
{
  proto_tree_add_text(opt_tree, tvb, offset,      optlen,
    "%s: %u", optp->name, tvb_get_ntohs(tvb, offset + 2));
  return;
}

static void
dissect_ipopt_timestamp(const ip_tcp_opt *optp, tvbuff_t *tvb,
    int offset, guint optlen, frame_data *fd, proto_tree *opt_tree)
{
  proto_tree *field_tree = NULL;
  proto_item *tf;
  int        ptr;
  int        optoffset = 0;
  int        flg;
  static const value_string flag_vals[] = {
    {IPOPT_TS_TSONLY,    "Time stamps only"                      },
    {IPOPT_TS_TSANDADDR, "Time stamp and address"                },
    {IPOPT_TS_PRESPEC,   "Time stamps for prespecified addresses"},
    {0,                  NULL                                    } };
  struct in_addr addr;
  guint ts;

  tf = proto_tree_add_text(opt_tree, tvb, offset,      optlen, "%s:", optp->name);
  field_tree = proto_item_add_subtree(tf, *optp->subtree_index);

  optoffset += 2;	/* skip past type and length */
  optlen -= 2;		/* subtract size of type and length */

  ptr = tvb_get_guint8(tvb, offset + optoffset);
  proto_tree_add_text(field_tree, tvb, offset + optoffset, 1,
              "Pointer: %d%s", ptr,
              ((ptr < 5) ? " (points before first address)" :
               (((ptr - 1) & 3) ? " (points to middle of address)" : "")));
  optoffset++;
  optlen--;
  ptr--;	/* ptr is 1-origin */

  flg = tvb_get_guint8(tvb, offset + optoffset);
  proto_tree_add_text(field_tree, tvb, offset + optoffset,   1,
        "Overflow: %u", flg >> 4);
  flg &= 0xF;
  proto_tree_add_text(field_tree, tvb, offset + optoffset, 1,
        "Flag: %s", val_to_str(flg, flag_vals, "Unknown (0x%x)"));
  optoffset++;
  optlen--;

  while (optlen > 0) {
    if (flg == IPOPT_TS_TSANDADDR) {
      if (optlen < 8) {
        proto_tree_add_text(field_tree, tvb, offset + optoffset, optlen,
          "(suboption would go past end of option)");
        break;
      }
      tvb_memcpy(tvb, (char *)&addr, offset + optoffset, sizeof(addr));
      ts = tvb_get_ntohl(tvb, offset + optoffset + 4);
      optlen -= 8;
      proto_tree_add_text(field_tree, tvb, offset + optoffset,      8,
          "Address = %s, time stamp = %u",
          ((addr.s_addr == 0) ? "-" :  (char *)get_hostname(addr.s_addr)),
          ts);
      optoffset += 8;
    } else {
      if (optlen < 4) {
        proto_tree_add_text(field_tree, tvb, offset + optoffset, optlen,
          "(suboption would go past end of option)");
        break;
      }
      ts = tvb_get_ntohl(tvb, offset + optoffset);
      optlen -= 4;
      proto_tree_add_text(field_tree, tvb, offset + optoffset, 4,
          "Time stamp = %u", ts);
      optoffset += 4;
    }
  }
}

static void
dissect_ipopt_ra(const ip_tcp_opt *optp, tvbuff_t *tvb, int offset,
		guint optlen, frame_data *fd, proto_tree *opt_tree)
{
  /* Router-Alert, as defined by RFC2113 */
  int opt = tvb_get_ntohs(tvb, offset + 2);
  static const value_string ra_opts[] = { 
	{0, "Every router examines packet"},
	{0, NULL}
  };
  
  proto_tree_add_text(opt_tree, tvb, offset,      optlen,
    "%s: %s", optp->name, val_to_str(opt, ra_opts, "Unknown (%d)"));
  return;
}

static const ip_tcp_opt ipopts[] = {
  {
    IPOPT_END,
    "EOL",
    NULL,
    NO_LENGTH,
    0,
    NULL,
  },
  {
    IPOPT_NOOP,
    "NOP",
    NULL,
    NO_LENGTH,
    0,
    NULL,
  },
  {
    IPOPT_SEC,
    "Security",
    &ett_ip_option_sec,
    FIXED_LENGTH,
    IPOLEN_SEC,
    dissect_ipopt_security
  },
  {
    IPOPT_SSRR,
    "Strict source route",
    &ett_ip_option_route,
    VARIABLE_LENGTH,
    IPOLEN_SSRR_MIN,
    dissect_ipopt_route
  },
  {
    IPOPT_LSRR,
    "Loose source route",
    &ett_ip_option_route,
    VARIABLE_LENGTH,
    IPOLEN_LSRR_MIN,
    dissect_ipopt_route
  },
  {
    IPOPT_RR,
    "Record route",
    &ett_ip_option_route,
    VARIABLE_LENGTH,
    IPOLEN_RR_MIN,
    dissect_ipopt_route
  },
  {
    IPOPT_SID,
    "Stream identifier",
    NULL,
    FIXED_LENGTH,
    IPOLEN_SID,
    dissect_ipopt_sid
  },
  {
    IPOPT_TIMESTAMP,
    "Time stamp",
    &ett_ip_option_timestamp,
    VARIABLE_LENGTH,
    IPOLEN_TIMESTAMP_MIN,
    dissect_ipopt_timestamp
  },
  {
    IPOPT_RA,
    "Router Alert",
    NULL,
    FIXED_LENGTH,
    IPOLEN_RA,
    dissect_ipopt_ra
  },
};

#define N_IP_OPTS	(sizeof ipopts / sizeof ipopts[0])

/* Dissect the IP or TCP options in a packet. */
void
dissect_ip_tcp_options(tvbuff_t *tvb, int offset, guint length,
			const ip_tcp_opt *opttab, int nopts, int eol,
			frame_data *fd, proto_tree *opt_tree)
{
  u_char            opt;
  const ip_tcp_opt *optp;
  opt_len_type      len_type;
  int               optlen;
  char             *name;
  char              name_str[7+1+1+2+2+1+1];	/* "Unknown (0x%02x)" */
  void            (*dissect)(const struct ip_tcp_opt *, tvbuff_t *,
				int, guint, frame_data *, proto_tree *);
  guint             len;

  while (length > 0) {
    opt = tvb_get_guint8(tvb, offset);
    for (optp = &opttab[0]; optp < &opttab[nopts]; optp++) {
      if (optp->optcode == opt)
        break;
    }
    if (optp == &opttab[nopts]) {
      /* We assume that the only NO_LENGTH options are EOL and NOP options,
         so that we can treat unknown options as VARIABLE_LENGTH with a
	 minimum of 2, and at least be able to move on to the next option
	 by using the length in the option. */
      optp = NULL;	/* indicate that we don't know this option */
      len_type = VARIABLE_LENGTH;
      optlen = 2;
      snprintf(name_str, sizeof name_str, "Unknown (0x%02x)", opt);
      name = name_str;
      dissect = NULL;
    } else {
      len_type = optp->len_type;
      optlen = optp->optlen;
      name = optp->name;
      dissect = optp->dissect;
    }
    --length;      /* account for type byte */
    if (len_type != NO_LENGTH) {
      /* Option has a length. Is it in the packet? */
      if (length == 0) {
        /* Bogus - packet must at least include option code byte and
           length byte! */
        proto_tree_add_text(opt_tree, tvb, offset,      1,
              "%s (length byte past end of options)", name);
        return;
      }
      len = tvb_get_guint8(tvb, offset + 1);  /* total including type, len */
      --length;    /* account for length byte */
      if (len < 2) {
        /* Bogus - option length is too short to include option code and
           option length. */
        proto_tree_add_text(opt_tree, tvb, offset,      2,
              "%s (with too-short option length = %u byte%s)", name,
              len, plurality(len, "", "s"));
        return;
      } else if (len - 2 > length) {
        /* Bogus - option goes past the end of the header. */
        proto_tree_add_text(opt_tree, tvb, offset,      length,
              "%s (option length = %u byte%s says option goes past end of options)",
	      name, len, plurality(len, "", "s"));
        return;
      } else if (len_type == FIXED_LENGTH && len != optlen) {
        /* Bogus - option length isn't what it's supposed to be for this
           option. */
        proto_tree_add_text(opt_tree, tvb, offset,      len,
              "%s (with option length = %u byte%s; should be %u)", name,
              len, plurality(len, "", "s"), optlen);
        return;
      } else if (len_type == VARIABLE_LENGTH && len < optlen) {
        /* Bogus - option length is less than what it's supposed to be for
           this option. */
        proto_tree_add_text(opt_tree, tvb, offset,      len,
              "%s (with option length = %u byte%s; should be >= %u)", name,
              len, plurality(len, "", "s"), optlen);
        return;
      } else {
        if (optp == NULL) {
          proto_tree_add_text(opt_tree, tvb, offset,    len, "%s (%u byte%s)",
				name, len, plurality(len, "", "s"));
        } else {
          if (dissect != NULL) {
            /* Option has a dissector. */
            (*dissect)(optp, tvb, offset,          len, fd, opt_tree);
          } else {
            /* Option has no data, hence no dissector. */
            proto_tree_add_text(opt_tree, tvb, offset,  len, "%s", name);
          }
        }
        len -= 2;	/* subtract size of type and length */
        offset += 2 + len;
      }
      length -= len;
    } else {
      proto_tree_add_text(opt_tree, tvb, offset,      1, "%s", name);
      offset += 1;
    }
    if (opt == eol)
      break;
  }
}

static const value_string dscp_vals[] = {
		  { IPDSFIELD_DSCP_DEFAULT, "Default"               },
		  { IPDSFIELD_DSCP_CS1,     "Class Selector 1"      },
		  { IPDSFIELD_DSCP_CS2,     "Class Selector 2"      },
		  { IPDSFIELD_DSCP_CS3,     "Class Selector 3"      },
		  { IPDSFIELD_DSCP_CS4,     "Class Selector 4"      },
		  { IPDSFIELD_DSCP_CS5,     "Class Selector 5"      },
		  { IPDSFIELD_DSCP_CS6,     "Class Selector 6"      },
		  { IPDSFIELD_DSCP_CS7,     "Class Selector 7"      },
		  { IPDSFIELD_DSCP_AF11,    "Assured Forwarding 11" },
		  { IPDSFIELD_DSCP_AF12,    "Assured Forwarding 12" },
		  { IPDSFIELD_DSCP_AF13,    "Assured Forwarding 13" },
		  { IPDSFIELD_DSCP_AF21,    "Assured Forwarding 21" },
		  { IPDSFIELD_DSCP_AF22,    "Assured Forwarding 22" },
		  { IPDSFIELD_DSCP_AF23,    "Assured Forwarding 23" },
		  { IPDSFIELD_DSCP_AF31,    "Assured Forwarding 31" },
		  { IPDSFIELD_DSCP_AF32,    "Assured Forwarding 32" },
		  { IPDSFIELD_DSCP_AF33,    "Assured Forwarding 33" },
		  { IPDSFIELD_DSCP_AF41,    "Assured Forwarding 41" },
		  { IPDSFIELD_DSCP_AF42,    "Assured Forwarding 42" },
		  { IPDSFIELD_DSCP_AF43,    "Assured Forwarding 43" },
		  { IPDSFIELD_DSCP_EF,      "Expedited Forwarding"  },
		  { 0,                      NULL                    } };

static const value_string precedence_vals[] = {
		  { IPTOS_PREC_ROUTINE,         "routine"              },
		  { IPTOS_PREC_PRIORITY,        "priority"             },
		  { IPTOS_PREC_IMMEDIATE,       "immediate"            },
		  { IPTOS_PREC_FLASH,           "flash"                },
		  { IPTOS_PREC_FLASHOVERRIDE,   "flash override"       },
		  { IPTOS_PREC_CRITIC_ECP,      "CRITIC/ECP"           },
		  { IPTOS_PREC_INTERNETCONTROL, "internetwork control" },
		  { IPTOS_PREC_NETCONTROL,      "network control"      },
		  { 0,                          NULL                   } };

static const value_string iptos_vals[] = {
	{ IPTOS_NONE,		"None" },
	{ IPTOS_LOWCOST,	"Minimize cost" },
	{ IPTOS_RELIABILITY,	"Maximize reliability" },
	{ IPTOS_THROUGHPUT,	"Maximize throughput" },
	{ IPTOS_LOWDELAY,	"Minimize delay" },
	{ IPTOS_SECURITY,	"Maximize security" },
	{ 0,			NULL }
};

static const true_false_string tos_set_low = {
  "Low",
  "Normal"
};

static const true_false_string tos_set_high = {
  "High",
  "Normal"
};

static const true_false_string flags_set_truth = {
  "Set",
  "Not set"
};

static guint16 ip_checksum(const guint8 *ptr, int len)
{
	vec_t cksum_vec[1];

	cksum_vec[0].ptr = ptr;
	cksum_vec[0].len = len;
	return in_cksum(&cksum_vec[0], 1);
}

static void
dissect_ip(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
  e_ip       iph;
  proto_tree *ip_tree, *field_tree;
  proto_item *ti, *tf;
  int        offset = 0;
  guint      hlen, optlen, len, payload_len, reported_payload_len, padding;
  guint16    flags;
  guint8     nxt;
  guint16    ipsum;
  tvbuff_t   *next_tvb;

  if (check_col(pinfo->fd, COL_PROTOCOL))
    col_set_str(pinfo->fd, COL_PROTOCOL, "IP");
  if (check_col(pinfo->fd, COL_INFO))
    col_clear(pinfo->fd, COL_INFO);

  /* Avoids alignment problems on many architectures. */
  tvb_memcpy(tvb, (guint8 *)&iph, offset, sizeof(e_ip));
  iph.ip_len = ntohs(iph.ip_len);
  iph.ip_id  = ntohs(iph.ip_id);
  iph.ip_off = ntohs(iph.ip_off);
  iph.ip_sum = ntohs(iph.ip_sum);

  /* Length of payload handed to us. */
  reported_payload_len = tvb_reported_length(tvb);
  payload_len = tvb_length(tvb);

  /* Length of IP datagram. */
  len = iph.ip_len;

  if (len < reported_payload_len) {
    /* Adjust the length of this tvbuff to include only the IP datagram.
       Our caller may use that to determine how much of its packet
       was padding. */
    tvb_set_reported_length(tvb, len);

    /* Shrink the total payload by the amount of padding. */
    padding = reported_payload_len - len;
    if (pinfo->len >= padding)
      pinfo->len -= padding;

    /* Shrink the captured payload by the amount of padding in the
       captured payload (which may be less than the amount of padding,
       as the padding may not have been captured). */
    if (len < payload_len) {
      padding = payload_len - len;
      if (pinfo->captured_len >= padding)
        pinfo->captured_len -= padding;
    }
  }

  /* XXX - check to make sure this is at least IPH_MIN_LEN. */
  hlen = lo_nibble(iph.ip_v_hl) * 4;	/* IP header length, in bytes */
  
  if (tree) {
    ti = proto_tree_add_item(tree, proto_ip, tvb, offset, hlen, FALSE);
    ip_tree = proto_item_add_subtree(ti, ett_ip);

    proto_tree_add_uint(ip_tree, hf_ip_version, tvb, offset, 1, hi_nibble(iph.ip_v_hl));
    proto_tree_add_uint_format(ip_tree, hf_ip_hdr_len, tvb, offset, 1, hlen,
	"Header length: %u bytes", hlen);

    if (g_ip_dscp_actif) {
      tf = proto_tree_add_uint_format(ip_tree, hf_ip_dsfield, tvb, offset + 1, 1, iph.ip_tos,
	   "Differentiated Services Field: 0x%02x (DSCP 0x%02x: %s; ECN: 0x%02x)", iph.ip_tos,
	   IPDSFIELD_DSCP(iph.ip_tos), val_to_str(IPDSFIELD_DSCP(iph.ip_tos), dscp_vals,
	   "Unknown DSCP"),IPDSFIELD_ECN(iph.ip_tos));

      field_tree = proto_item_add_subtree(tf, ett_ip_dsfield);
      proto_tree_add_uint(field_tree, hf_ip_dsfield_dscp, tvb, offset + 1, 1, iph.ip_tos);
      proto_tree_add_uint(field_tree, hf_ip_dsfield_ect, tvb, offset + 1, 1, iph.ip_tos);
      proto_tree_add_uint(field_tree, hf_ip_dsfield_ce, tvb, offset + 1, 1, iph.ip_tos);
    } else {
      tf = proto_tree_add_uint_format(ip_tree, hf_ip_tos, tvb, offset + 1, 1, iph.ip_tos,
	  "Type of service: 0x%02x (%s)", iph.ip_tos,
	  val_to_str( IPTOS_TOS(iph.ip_tos), iptos_vals, "Unknown") );

      field_tree = proto_item_add_subtree(tf, ett_ip_tos);
      proto_tree_add_uint(field_tree, hf_ip_tos_precedence, tvb, offset + 1, 1, iph.ip_tos);
      proto_tree_add_boolean(field_tree, hf_ip_tos_delay, tvb, offset + 1, 1, iph.ip_tos);
      proto_tree_add_boolean(field_tree, hf_ip_tos_throughput, tvb, offset + 1, 1, iph.ip_tos);
      proto_tree_add_boolean(field_tree, hf_ip_tos_reliability, tvb, offset + 1, 1, iph.ip_tos);
      proto_tree_add_boolean(field_tree, hf_ip_tos_cost, tvb, offset + 1, 1, iph.ip_tos);
    }
    proto_tree_add_uint(ip_tree, hf_ip_len, tvb, offset +  2, 2, iph.ip_len);
    proto_tree_add_uint(ip_tree, hf_ip_id, tvb, offset +  4, 2, iph.ip_id);

    flags = (iph.ip_off & (IP_DF|IP_MF)) >> 12;
    tf = proto_tree_add_uint(ip_tree, hf_ip_flags, tvb, offset +  6, 1, flags);
    field_tree = proto_item_add_subtree(tf, ett_ip_off);
    proto_tree_add_boolean(field_tree, hf_ip_flags_df, tvb, offset + 6, 1, flags),
    proto_tree_add_boolean(field_tree, hf_ip_flags_mf, tvb, offset + 6, 1, flags),

    proto_tree_add_uint(ip_tree, hf_ip_frag_offset, tvb, offset +  6, 2,
      (iph.ip_off & IP_OFFSET)*8);
    proto_tree_add_uint(ip_tree, hf_ip_ttl, tvb, offset +  8, 1, iph.ip_ttl);
    proto_tree_add_uint_format(ip_tree, hf_ip_proto, tvb, offset +  9, 1, iph.ip_p,
	"Protocol: %s (0x%02x)", ipprotostr(iph.ip_p), iph.ip_p);

    ipsum = ip_checksum(tvb_get_ptr(tvb, offset, hlen), hlen);
    if (ipsum == 0) {
	proto_tree_add_uint_format(ip_tree, hf_ip_checksum, tvb, offset + 10, 2, iph.ip_sum,
            "Header checksum: 0x%04x (correct)", iph.ip_sum);
    }
    else {
	proto_tree_add_item_hidden(ip_tree, hf_ip_checksum_bad, tvb, offset + 10, 2, TRUE);
	proto_tree_add_uint_format(ip_tree, hf_ip_checksum, tvb, offset + 10, 2, iph.ip_sum,
            "Header checksum: 0x%04x (incorrect, should be 0x%04x)", iph.ip_sum,
	    in_cksum_shouldbe(iph.ip_sum, ipsum));
    }

    proto_tree_add_ipv4(ip_tree, hf_ip_src, tvb, offset + 12, 4, iph.ip_src);
    proto_tree_add_ipv4(ip_tree, hf_ip_dst, tvb, offset + 16, 4, iph.ip_dst);
    proto_tree_add_ipv4_hidden(ip_tree, hf_ip_addr, tvb, offset + 12, 4, iph.ip_src);
    proto_tree_add_ipv4_hidden(ip_tree, hf_ip_addr, tvb, offset + 16, 4, iph.ip_dst);

    /* Decode IP options, if any. */
    if (hlen > sizeof (e_ip)) {
      /* There's more than just the fixed-length header.  Decode the
         options. */
      optlen = hlen - sizeof (e_ip);	/* length of options, in bytes */
      tf = proto_tree_add_text(ip_tree, tvb, offset +  20, optlen,
        "Options: (%u bytes)", optlen);
      field_tree = proto_item_add_subtree(tf, ett_ip_options);
      dissect_ip_tcp_options(tvb, offset + 20, optlen,
         ipopts, N_IP_OPTS, IPOPT_END, pinfo->fd, field_tree);
    }
  }

  pinfo->ipproto = iph.ip_p;
  pinfo->iplen = iph.ip_len;
  pinfo->iphdrlen = lo_nibble(iph.ip_v_hl);
  SET_ADDRESS(&pinfo->net_src, AT_IPv4, 4, tvb_get_ptr(tvb, offset + IPH_SRC, 4));
  SET_ADDRESS(&pinfo->src, AT_IPv4, 4, tvb_get_ptr(tvb, offset + IPH_SRC, 4));
  SET_ADDRESS(&pinfo->net_dst, AT_IPv4, 4, tvb_get_ptr(tvb, offset + IPH_DST, 4));
  SET_ADDRESS(&pinfo->dst, AT_IPv4, 4, tvb_get_ptr(tvb, offset + IPH_DST, 4));

  /* Skip over header + options */
  offset += hlen;
  nxt = iph.ip_p;
  if (iph.ip_off & IP_OFFSET) {
    /* fragmented */
    if (check_col(pinfo->fd, COL_INFO))
      col_add_fstr(pinfo->fd, COL_INFO, "Fragmented IP protocol (proto=%s 0x%02x, off=%u)",
	ipprotostr(iph.ip_p), iph.ip_p, (iph.ip_off & IP_OFFSET) * 8);
    dissect_data(tvb, offset, pinfo, tree);
    return;
  }

  /*
   * If this is the first fragment, but not the only fragment,
   * tell the next protocol that.
   */
  if (iph.ip_off & IP_MF)
    pinfo->fragmented = TRUE;
  else
    pinfo->fragmented = FALSE;

  /* Hand off to the next protocol.

     XXX - setting the columns only after trying various dissectors means
     that if one of those dissectors throws an exception, the frame won't
     even be labelled as an IP frame; ideally, if a frame being dissected
     throws an exception, it'll be labelled as a mangled frame of the
     type in question. */
  next_tvb = tvb_new_subset(tvb, offset, -1, -1);
  if (!dissector_try_port(ip_dissector_table, nxt, next_tvb, pinfo, tree)) {
    /* Unknown protocol */
    if (check_col(pinfo->fd, COL_INFO))
      col_add_fstr(pinfo->fd, COL_INFO, "%s (0x%02x)", ipprotostr(iph.ip_p), iph.ip_p);
    dissect_data(next_tvb, 0, pinfo, tree);
  }
}


static const gchar *unreach_str[] = {"Network unreachable",
                                     "Host unreachable",
                                     "Protocol unreachable",
                                     "Port unreachable",
                                     "Fragmentation needed",
                                     "Source route failed",
                                     "Destination network unknown",
                                     "Destination host unknown",
                                     "Source host isolated",
                                     "Network administratively prohibited",
                                     "Host administratively prohibited",
                                     "Network unreachable for TOS",
                                     "Host unreachable for TOS",
                                     "Communication administratively filtered",
                                     "Host precedence violation",
                                     "Precedence cutoff in effect"};
                                     
#define	N_UNREACH	(sizeof unreach_str / sizeof unreach_str[0])

static const gchar *redir_str[] = {"Redirect for network",
                                   "Redirect for host",
                                   "Redirect for TOS and network",
                                   "Redirect for TOS and host"};

#define	N_REDIRECT	(sizeof redir_str / sizeof redir_str[0])

static const gchar *ttl_str[] = {"TTL equals 0 during transit",
                                 "TTL equals 0 during reassembly"};
                                 
#define	N_TIMXCEED	(sizeof ttl_str / sizeof ttl_str[0])

static const gchar *par_str[] = {"IP header bad", "Required option missing"};

#define	N_PARAMPROB	(sizeof par_str / sizeof par_str[0])

static void
dissect_icmp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
  proto_tree *icmp_tree;
  proto_item *ti;
  guint8     icmp_type;
  guint8     icmp_code;
  guint      length, reported_length;
  guint16    cksum, computed_cksum;
  gchar      type_str[64], code_str[64] = "";
  guint8     num_addrs = 0;
  guint8     addr_entry_size = 0;
  int        i;

  if (check_col(pinfo->fd, COL_PROTOCOL))
    col_set_str(pinfo->fd, COL_PROTOCOL, "ICMP");
  if (check_col(pinfo->fd, COL_INFO))
    col_clear(pinfo->fd, COL_INFO);

  /* To do: check for runts, errs, etc. */
  icmp_type = tvb_get_guint8(tvb, 0);
  icmp_code = tvb_get_guint8(tvb, 1);
  cksum = tvb_get_ntohs(tvb, 2);

  switch (icmp_type) {
    case ICMP_ECHOREPLY:
      strcpy(type_str, "Echo (ping) reply");
      break;
    case ICMP_UNREACH:
      strcpy(type_str, "Destination unreachable");
      if (icmp_code < N_UNREACH) {
        sprintf(code_str, "(%s)", unreach_str[icmp_code]);
      } else {
        strcpy(code_str, "(Unknown - error?)");
      }
      break;
    case ICMP_SOURCEQUENCH:
      strcpy(type_str, "Source quench (flow control)");
      break;
    case ICMP_REDIRECT:
      strcpy(type_str, "Redirect");
      if (icmp_code < N_REDIRECT) {
        sprintf(code_str, "(%s)", redir_str[icmp_code]);
      } else {
        strcpy(code_str, "(Unknown - error?)");
      }
      break;
    case ICMP_ECHO:
      strcpy(type_str, "Echo (ping) request");
      break;
    case ICMP_RTRADVERT:
      strcpy(type_str, "Router advertisement");
      break;
    case ICMP_RTRSOLICIT:
      strcpy(type_str, "Router solicitation");
      break;
    case ICMP_TIMXCEED:
      strcpy(type_str, "Time-to-live exceeded");
      if (icmp_code < N_TIMXCEED) {
        sprintf(code_str, "(%s)", ttl_str[icmp_code]);
      } else {
        strcpy(code_str, "(Unknown - error?)");
      }
      break;
    case ICMP_PARAMPROB:
      strcpy(type_str, "Parameter problem");
      if (icmp_code < N_PARAMPROB) {
        sprintf(code_str, "(%s)", par_str[icmp_code]);
      } else {
        strcpy(code_str, "(Unknown - error?)");
      }
      break;
    case ICMP_TSTAMP:
      strcpy(type_str, "Timestamp request");
      break;
    case ICMP_TSTAMPREPLY:
      strcpy(type_str, "Timestamp reply");
      break;
    case ICMP_IREQ:
      strcpy(type_str, "Information request");
      break;
    case ICMP_IREQREPLY:
      strcpy(type_str, "Information reply");
      break;
    case ICMP_MASKREQ:
      strcpy(type_str, "Address mask request");
      break;
    case ICMP_MASKREPLY:
      strcpy(type_str, "Address mask reply");
      break;
    default:
      strcpy(type_str, "Unknown ICMP (obsolete or malformed?)");
  }

  if (check_col(pinfo->fd, COL_INFO))
    col_add_str(pinfo->fd, COL_INFO, type_str);

  if (tree) {
    length = tvb_length(tvb);
    reported_length = tvb_reported_length(tvb);
    ti = proto_tree_add_item(tree, proto_icmp, tvb, 0, length, FALSE);
    icmp_tree = proto_item_add_subtree(ti, ett_icmp);
    proto_tree_add_uint_format(icmp_tree, hf_icmp_type, tvb, 0, 1, 
			       icmp_type,
			       "Type: %u (%s)",
			       icmp_type, type_str);
    proto_tree_add_uint_format(icmp_tree, hf_icmp_code, tvb, 1, 1, 
			       icmp_code,
			       "Code: %u %s",
			       icmp_code, code_str);

    if (!pinfo->fragmented && length >= reported_length) {
      /* The packet isn't part of a fragmented datagram and isn't
         truncated, so we can checksum it. */

      computed_cksum = ip_checksum(tvb_get_ptr(tvb, 0, reported_length),
				   reported_length);
      if (computed_cksum == 0) {
        proto_tree_add_uint_format(icmp_tree, hf_icmp_checksum, tvb, 2, 2,
			cksum,
			"Checksum: 0x%04x (correct)", cksum);
      } else {
        proto_tree_add_item_hidden(icmp_tree, hf_icmp_checksum_bad,
			tvb, 2, 2, TRUE);
        proto_tree_add_uint_format(icmp_tree, hf_icmp_checksum, tvb, 2, 2,
			cksum,
			"Checksum: 0x%04x (incorrect, should be 0x%04x)",
			cksum, in_cksum_shouldbe(cksum, computed_cksum));
      }
    } else {
      proto_tree_add_uint(icmp_tree, hf_icmp_checksum, tvb, 2, 2, cksum);
    }

    /* Decode the second 4 bytes of the packet. */
    switch (icmp_type) {
      case ICMP_ECHOREPLY:
      case ICMP_ECHO:
      case ICMP_TSTAMP:
      case ICMP_TSTAMPREPLY:
      case ICMP_IREQ:
      case ICMP_IREQREPLY:
      case ICMP_MASKREQ:
      case ICMP_MASKREPLY:
	proto_tree_add_text(icmp_tree, tvb, 4, 2, "Identifier: 0x%04x",
	  tvb_get_ntohs(tvb, 4));
	proto_tree_add_text(icmp_tree, tvb, 6, 2, "Sequence number: %02x:%02x",
	  tvb_get_guint8(tvb, 6), tvb_get_guint8(tvb, 7));
	break;

       case ICMP_UNREACH:
         switch (icmp_code) {
           case ICMP_FRAG_NEEDED:
                 proto_tree_add_text(icmp_tree, tvb, 6, 2, "MTU of next hop: %u",
                   tvb_get_ntohs(tvb, 6));
                 break;
           }
         break;

      case ICMP_RTRADVERT:
        num_addrs = tvb_get_guint8(tvb, 4);
	proto_tree_add_text(icmp_tree, tvb, 4, 1, "Number of addresses: %u",
	  num_addrs);
	addr_entry_size = tvb_get_guint8(tvb, 5);
	proto_tree_add_text(icmp_tree, tvb, 5, 1, "Address entry size: %u",
	  addr_entry_size);
	proto_tree_add_text(icmp_tree, tvb, 6, 2, "Lifetime: %s",
	  time_secs_to_str(tvb_get_ntohs(tvb, 6)));
	break;

      case ICMP_PARAMPROB:
	proto_tree_add_text(icmp_tree, tvb, 4, 1, "Pointer: %u",
	  tvb_get_guint8(tvb, 4));
	break;

      case ICMP_REDIRECT:
	proto_tree_add_text(icmp_tree, tvb, 4, 4, "Gateway address: %s",
	  ip_to_str(tvb_get_ptr(tvb, 4, 4)));
	break;
    }

    /* Decode the additional information in the packet.  */
    switch (icmp_type) {
      case ICMP_UNREACH:
      case ICMP_TIMXCEED:
      case ICMP_PARAMPROB:
      case ICMP_SOURCEQUENCH:
      case ICMP_REDIRECT:
	/* Decode the IP header and first 64 bits of data from the
	   original datagram.

	   XXX - for now, just display it as data; not all dissection
	   routines can handle a short packet without exploding. */
	dissect_data(tvb, 8, pinfo, icmp_tree);
	break;

      case ICMP_ECHOREPLY:
      case ICMP_ECHO:
	dissect_data(tvb, 8, pinfo, icmp_tree);
	break;

      case ICMP_RTRADVERT:
        if (addr_entry_size == 2) {
	  for (i = 0; i < num_addrs; i++) {
	    proto_tree_add_text(icmp_tree, tvb, 8 + (i*8), 4,
	      "Router address: %s",
	      ip_to_str(tvb_get_ptr(tvb, 8 + (i*8), 4)));
	    proto_tree_add_text(icmp_tree, tvb, 12 + (i*8), 4,
	      "Preference level: %u", tvb_get_ntohl(tvb, 12 + (i*8)));
	  }
	} else
	  dissect_data(tvb, 8, pinfo, icmp_tree);
	break;

      case ICMP_TSTAMP:
      case ICMP_TSTAMPREPLY:
	proto_tree_add_text(icmp_tree, tvb, 8, 4, "Originate timestamp: %u",
	  tvb_get_ntohl(tvb, 8));
	proto_tree_add_text(icmp_tree, tvb, 12, 4, "Receive timestamp: %u",
	  tvb_get_ntohl(tvb, 12));
	proto_tree_add_text(icmp_tree, tvb, 16, 4, "Transmit timestamp: %u",
	  tvb_get_ntohl(tvb, 16));
	break;

    case ICMP_MASKREQ:
    case ICMP_MASKREPLY:
	proto_tree_add_text(icmp_tree, tvb, 8, 4, "Address mask: %s (0x%08x)",
	  ip_to_str(tvb_get_ptr(tvb, 8, 4)), tvb_get_ntohl(tvb, 8));
	break;
    }
  }
}

static void
dissect_igmp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
  e_igmp     ih;
  proto_tree *igmp_tree;
  proto_item *ti;
  gchar      *type_str;

  if (check_col(pinfo->fd, COL_PROTOCOL))
    col_set_str(pinfo->fd, COL_PROTOCOL, "IGMP");
  if (check_col(pinfo->fd, COL_INFO))
    col_clear(pinfo->fd, COL_INFO);

  /* Avoids alignment problems on many architectures. */
  memcpy(&ih, tvb_get_ptr(tvb, 0, sizeof(e_igmp)), sizeof(e_igmp));
  
  switch (lo_nibble(ih.igmp_v_t)) {
    case IGMP_M_QRY:
      type_str = "Router query";
      break;
    case IGMP_V1_M_RPT:
      type_str = "Host response (v1)";
      break;
    case IGMP_V2_LV_GRP:
      type_str = "Leave group (v2)";
      break;
    case IGMP_DVMRP:
      type_str = "DVMRP";
      break;
    case IGMP_PIM:
      type_str = "PIM";
      break;
    case IGMP_V2_M_RPT:
      type_str = "Host response (v2)";
      break;
    case IGMP_MTRC_RESP:
      type_str = "Traceroute response";
      break;
    case IGMP_MTRC:
      type_str = "Traceroute message";
      break;
    default:
      type_str = "Unknown IGMP";
  }

  if (check_col(pinfo->fd, COL_INFO))
    col_add_str(pinfo->fd, COL_INFO, type_str);
  if (tree) {
    ti = proto_tree_add_item(tree, proto_igmp, tvb, 0, 8, FALSE);
    igmp_tree = proto_item_add_subtree(ti, ett_igmp);
    proto_tree_add_uint(igmp_tree, hf_igmp_version, tvb, 0,     1, 
			hi_nibble(ih.igmp_v_t));
    proto_tree_add_uint_format(igmp_tree, hf_igmp_type, tvb,  0    , 1, 
			       lo_nibble(ih.igmp_v_t),
			       "Type: %u (%s)",
			       lo_nibble(ih.igmp_v_t), type_str);
    proto_tree_add_item(igmp_tree, hf_igmp_unused, tvb,  1, 1, FALSE);
    proto_tree_add_item(igmp_tree, hf_igmp_checksum, tvb,  2, 2, FALSE);
    proto_tree_add_ipv4(igmp_tree, hf_igmp_group, tvb,  4, 4, ih.igmp_gaddr);
  }
}

void
proto_register_igmp(void)
{
	static hf_register_info hf[] = {

		{ &hf_igmp_version,
		{ "Version",		"igmp.version", FT_UINT8, BASE_DEC, NULL, 0x0,
			"" }},

		{ &hf_igmp_type,
		{ "Type",		"igmp.type", FT_UINT8, BASE_DEC, NULL, 0x0,
			"" }},

		{ &hf_igmp_unused,
		{ "Unused",		"igmp.unused", FT_UINT8, BASE_HEX, NULL, 0x0,
			"" }},

		{ &hf_igmp_checksum,
		{ "Checksum",		"igmp.checksum", FT_UINT16, BASE_HEX, NULL, 0x0,
			"" }},

		{ &hf_igmp_group,
		{ "Group address",	"igmp.group", FT_IPv4, BASE_NONE, NULL, 0x0,
			"" }},
	};
	static gint *ett[] = {
		&ett_igmp,
	};

	proto_igmp = proto_register_protocol("Internet Group Management Protocol",
	    "IGMP", "igmp");
	proto_register_field_array(proto_igmp, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));
}

void
proto_reg_handoff_igmp(void)
{
	dissector_add("ip.proto", IP_PROTO_IGMP, dissect_igmp, proto_igmp);
}

void
proto_register_ip(void)
{
	static hf_register_info hf[] = {

		{ &hf_ip_version,
		{ "Version",		"ip.version", FT_UINT8, BASE_DEC, NULL, 0x0,
			"" }},

		{ &hf_ip_hdr_len,
		{ "Header Length",	"ip.hdr_len", FT_UINT8, BASE_DEC, NULL, 0x0,
			"" }},

		{ &hf_ip_dsfield,
		{ "Differentiated Services field",	"ip.dsfield", FT_UINT8, BASE_DEC, NULL, 0x0,
			"" }},

		{ &hf_ip_dsfield_dscp,
		{ "Differentiated Services Codepoint",	"ip.dsfield.dscp", FT_UINT8, BASE_HEX,
			VALS(dscp_vals), IPDSFIELD_DSCP_MASK,
			"" }},

		{ &hf_ip_dsfield_ect,
		{ "ECN-Capable Transport (ECT)",	"ip.dsfield.ect", FT_UINT8, BASE_DEC, NULL,
			IPDSFIELD_ECT_MASK,
			"" }},

		{ &hf_ip_dsfield_ce,
		{ "ECN-CE",	"ip.dsfield.ce", FT_UINT8, BASE_DEC, NULL,
			IPDSFIELD_CE_MASK,
			"" }},

		{ &hf_ip_tos,
		{ "Type of Service",	"ip.tos", FT_UINT8, BASE_DEC, NULL, 0x0,
			"" }},

		{ &hf_ip_tos_precedence,
		{ "Precedence",		"ip.tos.precedence", FT_UINT8, BASE_DEC, VALS(precedence_vals),
			IPTOS_PREC_MASK,
			"" }},

		{ &hf_ip_tos_delay,
		{ "Delay",		"ip.tos.delay", FT_BOOLEAN, 8, TFS(&tos_set_low),
			IPTOS_LOWDELAY,
			"" }},

		{ &hf_ip_tos_throughput,
		{ "Throughput",		"ip.tos.throughput", FT_BOOLEAN, 8, TFS(&tos_set_high),
			IPTOS_THROUGHPUT,
			"" }},

		{ &hf_ip_tos_reliability,
		{ "Reliability",	"ip.tos.reliability", FT_BOOLEAN, 8, TFS(&tos_set_high),
			IPTOS_RELIABILITY,
			"" }},

		{ &hf_ip_tos_cost,
		{ "Cost",		"ip.tos.cost", FT_BOOLEAN, 8, TFS(&tos_set_low),
			IPTOS_LOWCOST,
			"" }},

		{ &hf_ip_len,
		{ "Total Length",	"ip.len", FT_UINT16, BASE_DEC, NULL, 0x0,
			"" }},

		{ &hf_ip_id,
		{ "Identification",	"ip.id", FT_UINT16, BASE_HEX, NULL, 0x0,
			"" }},

		{ &hf_ip_dst,
		{ "Destination",	"ip.dst", FT_IPv4, BASE_NONE, NULL, 0x0,
			"" }},

		{ &hf_ip_src,
		{ "Source",		"ip.src", FT_IPv4, BASE_NONE, NULL, 0x0,
			"" }},

		{ &hf_ip_addr,
		{ "Source or Destination Address", "ip.addr", FT_IPv4, BASE_NONE, NULL, 0x0,
			"" }},

		{ &hf_ip_flags,
		{ "Flags",		"ip.flags", FT_UINT8, BASE_HEX, NULL, 0x0,
			"" }},

		{ &hf_ip_flags_df,
		{ "Don't fragment",	"ip.flags.df", FT_BOOLEAN, 4, TFS(&flags_set_truth), IP_DF>>12,
			"" }},

		{ &hf_ip_flags_mf,
		{ "More fragments",	"ip.flags.mf", FT_BOOLEAN, 4, TFS(&flags_set_truth), IP_MF>>12,
			"" }},

		{ &hf_ip_frag_offset,
		{ "Fragment offset",	"ip.frag_offset", FT_UINT16, BASE_DEC, NULL, 0x0,
			"" }},

		{ &hf_ip_ttl,
		{ "Time to live",	"ip.ttl", FT_UINT8, BASE_DEC, NULL, 0x0,
			"" }},

		{ &hf_ip_proto,
		{ "Protocol",		"ip.proto", FT_UINT8, BASE_HEX, NULL, 0x0,
			"" }},

		{ &hf_ip_checksum,
		{ "Header checksum",	"ip.checksum", FT_UINT16, BASE_HEX, NULL, 0x0,
			"" }},

		{ &hf_ip_checksum_bad,
		{ "Bad Header checksum",	"ip.checksum_bad", FT_BOOLEAN, BASE_NONE, NULL, 0x0,
			"" }},
	};
	static gint *ett[] = {
		&ett_ip,
		&ett_ip_dsfield,
		&ett_ip_tos,
		&ett_ip_off,
		&ett_ip_options,
		&ett_ip_option_sec,
		&ett_ip_option_route,
		&ett_ip_option_timestamp,
	};
	module_t *ip_module;

	proto_ip = proto_register_protocol("Internet Protocol", "IP", "ip");
	proto_register_field_array(proto_ip, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));

	/* subdissector code */
	ip_dissector_table = register_dissector_table("ip.proto");

	/* Register a configuration option for decoding TOS as DSCP */
	ip_module = prefs_register_protocol(proto_ip, NULL);
	prefs_register_bool_preference(ip_module, "decode_tos_as_diffserv",
	    "Decode IPv4 TOS field as DiffServ field",
"Whether the IPv4 type-of-service field should be decoded as a Differentiated Services field",
	    &g_ip_dscp_actif);

	register_dissector("ip", dissect_ip, proto_ip);
}

void
proto_reg_handoff_ip(void)
{
	dissector_add("ethertype", ETHERTYPE_IP, dissect_ip, proto_ip);
	dissector_add("ppp.protocol", PPP_IP, dissect_ip, proto_ip);
	dissector_add("ppp.protocol", ETHERTYPE_IP, dissect_ip, proto_ip);
	dissector_add("gre.proto", ETHERTYPE_IP, dissect_ip, proto_ip);
	dissector_add("gre.proto", GRE_WCCP, dissect_ip, proto_ip);
	dissector_add("llc.dsap", SAP_IP, dissect_ip, proto_ip);
	dissector_add("ip.proto", IP_PROTO_IPIP, dissect_ip, proto_ip);
	dissector_add("null.type", BSD_AF_INET, dissect_ip, proto_ip);
	dissector_add("chdlctype", ETHERTYPE_IP, dissect_ip, proto_ip);
	dissector_add("fr.ietf", NLPID_IP, dissect_ip, proto_ip);
}

void
proto_register_icmp(void)
{
  static hf_register_info hf[] = {
    
    { &hf_icmp_type,
      { "Type",		"icmp.type",		FT_UINT8, BASE_DEC,	NULL, 0x0,
      	"" }},

    { &hf_icmp_code,
      { "Code",		"icmp.code",		FT_UINT8, BASE_HEX,	NULL, 0x0,
      	"" }},    

    { &hf_icmp_checksum,
      { "Checksum",	"icmp.checksum",	FT_UINT16, BASE_HEX,	NULL, 0x0,
      	"" }},

    { &hf_icmp_checksum_bad,
      { "Bad Checksum",	"icmp.checksum_bad",	FT_BOOLEAN, BASE_NONE,	NULL, 0x0,
	"" }},
  };
  static gint *ett[] = {
    &ett_icmp,
  };
  
  proto_icmp = proto_register_protocol("Internet Control Message Protocol", 
				       "ICMP", "icmp");
  proto_register_field_array(proto_icmp, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));
}

void
proto_reg_handoff_icmp(void)
{
  dissector_add("ip.proto", IP_PROTO_ICMP, dissect_icmp, proto_icmp);
}
