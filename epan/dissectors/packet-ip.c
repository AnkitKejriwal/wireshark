/* packet-ip.c
 * Routines for IP and miscellaneous IP protocol packet disassembly
 *
 * $Id$
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * Wednesday, January 17, 2006
 * Support for the CIPSO IPv4 option
 * (http://sourceforge.net/docman/display_doc.php?docid=34650&group_id=174379)
 * by   Paul Moore <paul.moore@hp.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <string.h>
#include <glib.h>

#include <epan/packet.h>
#include <epan/addr_resolv.h>
#include <epan/ipproto.h>
#include <epan/ip_opts.h>
#include <epan/prefs.h>
#include <epan/reassemble.h>
#include <epan/etypes.h>
#include <epan/greproto.h>
#include <epan/ppptypes.h>
#include <epan/llcsaps.h>
#include <epan/aftypes.h>
#include <epan/arcnet_pids.h>
#include <epan/in_cksum.h>
#include <epan/nlpid.h>
#include <epan/ax25_pids.h>
#include <epan/tap.h>
#include <epan/emem.h>
#include <epan/expert.h>

#include "packet-ip.h"
#include "packet-ipsec.h"

#ifdef HAVE_GEOIP
#include <GeoIP.h>
#include <epan/geoip_db.h>
#endif /* HAVE_GEOIP */


static int ip_tap = -1;

/* Decode the old IPv4 TOS field as the DiffServ DS Field (RFC2474/2475) */
static gboolean g_ip_dscp_actif = TRUE;

/* Defragment fragmented IP datagrams */
static gboolean ip_defragment = TRUE;

/* Place IP summary in proto tree */
static gboolean ip_summary_in_tree = TRUE;

/* Perform IP checksum */
static gboolean ip_check_checksum = TRUE;

/* Assume TSO and correct zero-length IP packets */
static gboolean ip_tso_supported = TRUE;

#ifdef HAVE_GEOIP
/* Look up addresses in GeoIP */
static gboolean ip_use_geoip = TRUE;
#endif /* HAVE_GEOIP */

/* Interpret the reserved flag as security flag (RFC 3514) */
static gboolean ip_security_flag = FALSE;

static int proto_ip = -1;
static int hf_ip_version = -1;
static int hf_ip_hdr_len = -1;
static int hf_ip_dsfield = -1;
static int hf_ip_dsfield_dscp = -1;
static int hf_ip_dsfield_ecn = -1;
static int hf_ip_tos = -1;
static int hf_ip_tos_precedence = -1;
static int hf_ip_tos_delay = -1;
static int hf_ip_tos_throughput = -1;
static int hf_ip_tos_reliability = -1;
static int hf_ip_tos_cost = -1;
static int hf_ip_len = -1;
static int hf_ip_id = -1;
static int hf_ip_dst = -1;
static int hf_ip_dst_host = -1;
static int hf_ip_src = -1;
static int hf_ip_src_host = -1;
static int hf_ip_addr = -1;
static int hf_ip_host = -1;
static int hf_ip_flags = -1;
static int hf_ip_flags_sf = -1;
static int hf_ip_flags_rf = -1;
static int hf_ip_flags_df = -1;
static int hf_ip_flags_mf = -1;
static int hf_ip_frag_offset = -1;
static int hf_ip_ttl = -1;
static int hf_ip_proto = -1;
static int hf_ip_checksum = -1;
static int hf_ip_checksum_good = -1;
static int hf_ip_checksum_bad = -1;

/* IP option fields */
static int hf_ip_opt_type = -1;
static int hf_ip_opt_type_copy = -1;
static int hf_ip_opt_type_class = -1;
static int hf_ip_opt_type_number = -1;
static int hf_ip_opt_len = -1;
static int hf_ip_opt_ptr = -1;
static int hf_ip_opt_sid = -1;
static int hf_ip_opt_mtu = -1;
static int hf_ip_opt_id_number = -1;
static int hf_ip_opt_ohc = -1;
static int hf_ip_opt_rhc = -1;
static int hf_ip_opt_originator = -1;
static int hf_ip_opt_ra = -1;
static int hf_ip_opt_addr = -1;
static int hf_ip_opt_padding = -1;
static int hf_ip_opt_qs_func = -1;
static int hf_ip_opt_qs_rate = -1;
static int hf_ip_opt_qs_ttl = -1;
static int hf_ip_opt_qs_ttl_diff = -1;
static int hf_ip_opt_qs_unused = -1;
static int hf_ip_opt_qs_nonce = -1;
static int hf_ip_opt_qs_reserved = -1;
static int hf_ip_opt_sec_rfc791_sec = -1;
static int hf_ip_opt_sec_rfc791_comp = -1;
static int hf_ip_opt_sec_rfc791_hr = -1;
static int hf_ip_opt_sec_rfc791_tcc = -1;
static int hf_ip_opt_sec_cl = -1;
static int hf_ip_opt_sec_prot_auth_flags = -1;
static int hf_ip_opt_sec_prot_auth_genser = -1;
static int hf_ip_opt_sec_prot_auth_siop_esi = -1;
static int hf_ip_opt_sec_prot_auth_sci = -1;
static int hf_ip_opt_sec_prot_auth_nsa = -1;
static int hf_ip_opt_sec_prot_auth_doe = -1;
static int hf_ip_opt_sec_prot_auth_unassigned = -1;
static int hf_ip_opt_sec_prot_auth_unassigned2 = -1;
static int hf_ip_opt_sec_prot_auth_fti = -1;
static int hf_ip_opt_ext_sec_add_sec_info_format_code = -1;
static int hf_ip_opt_ext_sec_add_sec_info = -1;
static int hf_ip_rec_rt = -1;
static int hf_ip_rec_rt_host = -1;
static int hf_ip_cur_rt = -1;
static int hf_ip_cur_rt_host = -1;
static int hf_ip_src_rt = -1;
static int hf_ip_src_rt_host = -1;
static int hf_ip_empty_rt = -1;
static int hf_ip_empty_rt_host = -1;

static int hf_ip_fragments = -1;
static int hf_ip_fragment = -1;
static int hf_ip_fragment_overlap = -1;
static int hf_ip_fragment_overlap_conflict = -1;
static int hf_ip_fragment_multiple_tails = -1;
static int hf_ip_fragment_too_long_fragment = -1;
static int hf_ip_fragment_error = -1;
static int hf_ip_fragment_count = -1;
static int hf_ip_reassembled_in = -1;
static int hf_ip_reassembled_length = -1;
static int hf_ip_reassembled_data = -1;

#ifdef HAVE_GEOIP
static int hf_geoip_country = -1;
static int hf_geoip_city = -1;
static int hf_geoip_org = -1;
static int hf_geoip_isp = -1;
static int hf_geoip_asnum = -1;
static int hf_geoip_lat = -1;
static int hf_geoip_lon = -1;
static int hf_geoip_src_country = -1;
static int hf_geoip_src_city = -1;
static int hf_geoip_src_org = -1;
static int hf_geoip_src_isp = -1;
static int hf_geoip_src_asnum = -1;
static int hf_geoip_src_lat = -1;
static int hf_geoip_src_lon = -1;
static int hf_geoip_dst_country = -1;
static int hf_geoip_dst_city = -1;
static int hf_geoip_dst_org = -1;
static int hf_geoip_dst_isp = -1;
static int hf_geoip_dst_asnum = -1;
static int hf_geoip_dst_lat = -1;
static int hf_geoip_dst_lon = -1;
#endif /* HAVE_GEOIP */

static gint ett_ip = -1;
static gint ett_ip_dsfield = -1;
static gint ett_ip_tos = -1;
static gint ett_ip_off = -1;
static gint ett_ip_options = -1;
static gint ett_ip_option_eool = -1;
static gint ett_ip_option_nop = -1;
static gint ett_ip_option_sec = -1;
static gint ett_ip_option_route = -1;
static gint ett_ip_option_timestamp = -1;
static gint ett_ip_option_ext_security = -1;
static gint ett_ip_option_cipso = -1;
static gint ett_ip_option_sid = -1;
static gint ett_ip_option_mtu = -1;
static gint ett_ip_option_tr = -1;
static gint ett_ip_option_ra = -1;
static gint ett_ip_option_sdb = -1;
static gint ett_ip_option_qs = -1;
static gint ett_ip_option_other = -1;
static gint ett_ip_fragments = -1;
static gint ett_ip_fragment  = -1;
static gint ett_ip_checksum = -1;
static gint ett_ip_opt_type = -1;
static gint ett_ip_opt_sec_prot_auth_flags = -1;

#ifdef HAVE_GEOIP
static gint ett_geoip_info = -1;
#endif /* HAVE_GEOIP */

static const fragment_items ip_frag_items = {
  &ett_ip_fragment,
  &ett_ip_fragments,
  &hf_ip_fragments,
  &hf_ip_fragment,
  &hf_ip_fragment_overlap,
  &hf_ip_fragment_overlap_conflict,
  &hf_ip_fragment_multiple_tails,
  &hf_ip_fragment_too_long_fragment,
  &hf_ip_fragment_error,
  &hf_ip_fragment_count,
  &hf_ip_reassembled_in,
  &hf_ip_reassembled_length,
  &hf_ip_reassembled_data,
  "IPv4 fragments"
};

static dissector_table_t ip_dissector_table;

static dissector_handle_t ipv6_handle;
static dissector_handle_t data_handle;
static dissector_handle_t tapa_handle;


/* IP structs and definitions */

/* Offsets of fields within an IP header. */
#define IPH_V_HL                0
#define IPH_TOS                 1
#define IPH_LEN                 2
#define IPH_ID                  4
#define IPH_TTL                 6
#define IPH_OFF                 8
#define IPH_P                   9
#define IPH_SUM                 10
#define IPH_SRC                 12
#define IPH_DST                 16

/* Minimum IP header length. */
#define IPH_MIN_LEN             20

/* Width (in bits) of the fragment offset IP header field */
#define IP_OFFSET_WIDTH         13

/* Width (in bits) of the flags IP header field */
#define IP_FLAGS_WIDTH          3

/* IP flags. */
#define IP_RF                   0x8000      /* Flag: "Reserved bit"     */
#define IP_DF                   0x4000      /* Flag: "Don't Fragment"   */
#define IP_MF                   0x2000      /* Flag: "More Fragments"   */
#define IP_OFFSET               0x1FFF      /* "Fragment Offset" part   */

/* Differentiated Services Field. See RFCs 2474, 2597 and 2598. */
#define IPDSFIELD_DSCP_MASK     0xFC
#define IPDSFIELD_ECN_MASK      0x03
#define IPDSFIELD_DSCP_SHIFT    2

#define IPDSFIELD_DSCP(dsfield) (((dsfield)&IPDSFIELD_DSCP_MASK)>>IPDSFIELD_DSCP_SHIFT)
#define IPDSFIELD_ECN(dsfield)  ((dsfield)&IPDSFIELD_ECN_MASK)

#define IPDSFIELD_DSCP_DEFAULT  0x00
#define IPDSFIELD_DSCP_CS1      0x08
#define IPDSFIELD_DSCP_AF11     0x0A
#define IPDSFIELD_DSCP_AF12     0x0C
#define IPDSFIELD_DSCP_AF13     0x0E
#define IPDSFIELD_DSCP_CS2      0x10
#define IPDSFIELD_DSCP_AF21     0x12
#define IPDSFIELD_DSCP_AF22     0x14
#define IPDSFIELD_DSCP_AF23     0x16
#define IPDSFIELD_DSCP_CS3      0x18
#define IPDSFIELD_DSCP_AF31     0x1A
#define IPDSFIELD_DSCP_AF32     0x1C
#define IPDSFIELD_DSCP_AF33     0x1E
#define IPDSFIELD_DSCP_CS4      0x20
#define IPDSFIELD_DSCP_AF41     0x22
#define IPDSFIELD_DSCP_AF42     0x24
#define IPDSFIELD_DSCP_AF43     0x26
#define IPDSFIELD_DSCP_CS5      0x28
#define IPDSFIELD_DSCP_EF       0x2E
#define IPDSFIELD_DSCP_CS6      0x30
#define IPDSFIELD_DSCP_CS7      0x38

#define IPDSFIELD_ECT_NOT       0x00
#define IPDSFIELD_ECT_1         0x01
#define IPDSFIELD_ECT_0         0x02
#define IPDSFIELD_CE            0x03

/* IP TOS, superseded by the DS Field, RFC 2474. */
#define IPTOS_TOS_MASK          0x1E
#define IPTOS_TOS(tos)          ((tos) & IPTOS_TOS_MASK)
#define IPTOS_NONE              0x00
#define IPTOS_LOWCOST           0x02
#define IPTOS_RELIABILITY       0x04
#define IPTOS_THROUGHPUT        0x08
#define IPTOS_LOWDELAY          0x10
#define IPTOS_SECURITY          0x1E

#define IPTOS_PREC_MASK             0xE0
#define IPTOS_PREC_SHIFT            5
#define IPTOS_PREC(tos)             (((tos)&IPTOS_PREC_MASK)>>IPTOS_PREC_SHIFT)
#define IPTOS_PREC_NETCONTROL       7
#define IPTOS_PREC_INTERNETCONTROL  6
#define IPTOS_PREC_CRITIC_ECP       5
#define IPTOS_PREC_FLASHOVERRIDE    4
#define IPTOS_PREC_FLASH            3
#define IPTOS_PREC_IMMEDIATE        2
#define IPTOS_PREC_PRIORITY         1
#define IPTOS_PREC_ROUTINE          0

/* IP options */
#define IPOPT_COPY_MASK         0x80
#define IPOPT_COPY              0x80

#define IPOPT_CLASS_MASK        0x60
#define IPOPT_CONTROL           0x00
#define IPOPT_RESERVED1         0x20
#define IPOPT_MEASUREMENT       0x40
#define IPOPT_RESERVED2         0x60

#define IPOPT_NUMBER_MASK       0x1F

/* REF: http://www.iana.org/assignments/ip-parameters */
/* TODO: Not all of these are implemented. */
#define IPOPT_EOOL      (0 |IPOPT_CONTROL)
#define IPOPT_NOP       (1 |IPOPT_CONTROL)
#define IPOPT_SEC       (2 |IPOPT_COPY|IPOPT_CONTROL)       /* RFC 791/1108 */
#define IPOPT_LSR       (3 |IPOPT_COPY|IPOPT_CONTROL)
#define IPOPT_TS        (4 |IPOPT_MEASUREMENT)
#define IPOPT_ESEC      (5 |IPOPT_COPY|IPOPT_CONTROL)       /* RFC 1108 */
#define IPOPT_CIPSO     (6 |IPOPT_COPY|IPOPT_CONTROL)       /* draft-ietf-cipso-ipsecurity-01 */
#define IPOPT_RR        (7 |IPOPT_CONTROL)
#define IPOPT_SID       (8 |IPOPT_COPY|IPOPT_CONTROL)
#define IPOPT_SSR       (9 |IPOPT_COPY|IPOPT_CONTROL)
#define IPOPT_ZSU       (10|IPOPT_CONTROL)                  /* Zsu */
#define IPOPT_MTUP      (11|IPOPT_CONTROL)                  /* RFC 1063 */
#define IPOPT_MTUR      (12|IPOPT_CONTROL)                  /* RFC 1063 */
#define IPOPT_FINN      (13|IPOPT_COPY|IPOPT_MEASUREMENT)   /* Finn */
#define IPOPT_VISA      (14|IPOPT_COPY|IPOPT_CONTROL)       /* Estrin */
#define IPOPT_ENCODE    (15|IPOPT_CONTROL)                  /* VerSteeg */
#define IPOPT_IMITD     (16|IPOPT_COPY|IPOPT_CONTROL)       /* Lee */
#define IPOPT_EIP       (17|IPOPT_COPY|IPOPT_CONTROL)       /* RFC 1385 */
#define IPOPT_TR        (18|IPOPT_MEASUREMENT)              /* RFC 1393 */
#define IPOPT_ADDEXT    (19|IPOPT_COPY|IPOPT_CONTROL)       /* Ullmann IPv7 */
#define IPOPT_RTRALT    (20|IPOPT_COPY|IPOPT_CONTROL)       /* RFC 2113 */
#define IPOPT_SDB       (21|IPOPT_COPY|IPOPT_CONTROL)       /* RFC 1770 Graff */
#define IPOPT_UN        (22|IPOPT_COPY|IPOPT_CONTROL)       /* Released 18-Oct-2005 */
#define IPOPT_DPS       (23|IPOPT_COPY|IPOPT_CONTROL)       /* Malis */
#define IPOPT_UMP       (24|IPOPT_COPY|IPOPT_CONTROL)       /* Farinacci */
#define IPOPT_QS        (25|IPOPT_CONTROL)                  /* RFC 4782 */
#define IPOPT_EXP       (30|IPOPT_CONTROL)                  /* RFC 4727 */


/* IP option lengths */
#define IPOLEN_SEC_MIN          3
#define IPOLEN_LSR_MIN          3
#define IPOLEN_TS_MIN           4
#define IPOLEN_ESEC_MIN         3
#define IPOLEN_CIPSO_MIN        10
#define IPOLEN_RR_MIN           3
#define IPOLEN_SID              4
#define IPOLEN_SSR_MIN          3
#define IPOLEN_MTU              4
#define IPOLEN_TR               12
#define IPOLEN_RA               4
#define IPOLEN_SDB_MIN          6
#define IPOLEN_QS               8
#define IPOLEN_MAX              40

#define IPSEC_RFC791_UNCLASSIFIED 0x0000
#define IPSEC_RFC791_CONFIDENTIAL 0xF135
#define IPSEC_RFC791_EFTO         0x789A
#define IPSEC_RFC791_MMMM         0xBC4D
#define IPSEC_RFC791_PROG         0x5E26
#define IPSEC_RFC791_RESTRICTED   0xAF13
#define IPSEC_RFC791_SECRET       0xD788
#define IPSEC_RFC791_TOPSECRET    0x6BC5
#define IPSEC_RFC791_RESERVED1    0x35E2
#define IPSEC_RFC791_RESERVED2    0x9AF1
#define IPSEC_RFC791_RESERVED3    0x4D78
#define IPSEC_RFC791_RESERVED4    0x24BD
#define IPSEC_RFC791_RESERVED5    0x135E
#define IPSEC_RFC791_RESERVED6    0x89AF
#define IPSEC_RFC791_RESERVED7    0xC4D6
#define IPSEC_RFC791_RESERVED8    0xE26B

#define IPSEC_RESERVED4         0x01
#define IPSEC_TOPSECRET         0x3D
#define IPSEC_SECRET            0x5A
#define IPSEC_CONFIDENTIAL      0x96
#define IPSEC_RESERVED3         0x66
#define IPSEC_RESERVED2         0xCC
#define IPSEC_UNCLASSIFIED      0xAB
#define IPSEC_RESERVED1         0xF1

#define IPOPT_TS_TSONLY         0       /* timestamps only */
#define IPOPT_TS_TSANDADDR      1       /* timestamps and addresses */
#define IPOPT_TS_PRESPEC        3       /* specified modules only */

#define IPLOCAL_NETWRK_CTRL_BLK_VRRP_ADDR       0xE0000012
#define IPLOCAL_NETWRK_CTRL_BLK_VRRP_TTL        0xFF
#define IPLOCAL_NETWRK_CTRL_BLK_GLPB_ADDR       0xE0000066
#define IPLOCAL_NETWRK_CTRL_BLK_GLPB_TTL        0XFF
#define IPLOCAL_NETWRK_CTRL_BLK_MDNS_ADDR       0xE00000FB
#define IPLOCAL_NETWRK_CTRL_BLK_MDNS_TTL        0XFF
#define IPLOCAL_NETWRK_CTRL_BLK_LLMNR_ADDR      0xE00000FC

#define IPLOCAL_NETWRK_CTRL_BLK_ANY_TTL         0x1000 /* larger than max ttl */
#define IPLOCAL_NETWRK_CTRL_BLK_DEFAULT_TTL     0X01

/* Return true if the address is in the 224.0.0.0/24 network block */
#define is_a_local_network_control_block_addr(addr) \
  ((addr & 0xffffff00) == 0xe0000000)

/* Return true if the address is in the 224.0.0.0/4 network block */
#define is_a_multicast_addr(addr) \
  ((addr & 0xf0000000) == 0xe0000000)

/*
 * defragmentation of IPv4
 */
static GHashTable *ip_fragment_table = NULL;
static GHashTable *ip_reassembled_table = NULL;

static void
ip_defragment_init(void)
{
  fragment_table_init(&ip_fragment_table);
  reassembled_table_init(&ip_reassembled_table);
}

void
capture_ip(const guchar *pd, int offset, int len, packet_counts *ld) {
  if (!BYTES_ARE_IN_FRAME(offset, len, IPH_MIN_LEN)) {
    ld->other++;
    return;
  }
  switch (pd[offset + 9]) {
    case IP_PROTO_TCP:
      ld->tcp++;
      break;
    case IP_PROTO_UDP:
    case IP_PROTO_UDPLITE:
      ld->udp++;
      break;
    case IP_PROTO_ICMP:
    case IP_PROTO_ICMPV6:   /* XXX - separate counters? */
      ld->icmp++;
      break;
    case IP_PROTO_SCTP:
      ld->sctp++;
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

#ifdef HAVE_GEOIP
static void
add_geoip_info_entry(proto_item *geoip_info_item, tvbuff_t *tvb, gint offset, guint32 ip, int isdst)
{
  proto_tree *geoip_info_tree;

  guint num_dbs = geoip_db_num_dbs();
  guint item_cnt = 0;
  guint dbnum;

  geoip_info_tree = proto_item_add_subtree(geoip_info_item, ett_geoip_info);

  for (dbnum = 0; dbnum < num_dbs; dbnum++) {
    const char *geoip_str = geoip_db_lookup_ipv4(dbnum, ip, NULL);
    int db_type = geoip_db_type(dbnum);

    int geoip_hf, geoip_local_hf;

    switch (db_type) {
      case GEOIP_COUNTRY_EDITION:
        geoip_hf = hf_geoip_country;
        geoip_local_hf = (isdst) ? hf_geoip_dst_country : hf_geoip_src_country;
        break;
      case GEOIP_CITY_EDITION_REV0:
        geoip_hf = hf_geoip_city;
        geoip_local_hf = (isdst) ? hf_geoip_dst_city : hf_geoip_src_city;
        break;
      case GEOIP_CITY_EDITION_REV1:
        geoip_hf = hf_geoip_city;
        geoip_local_hf = (isdst) ? hf_geoip_dst_city : hf_geoip_src_city;
        break;
      case GEOIP_ORG_EDITION:
        geoip_hf = hf_geoip_org;
        geoip_local_hf = (isdst) ? hf_geoip_dst_org : hf_geoip_src_org;
        break;
      case GEOIP_ISP_EDITION:
        geoip_hf = hf_geoip_isp;
        geoip_local_hf = (isdst) ? hf_geoip_dst_isp : hf_geoip_src_isp;
        break;
      case GEOIP_ASNUM_EDITION:
        geoip_hf = hf_geoip_asnum;
        geoip_local_hf = (isdst) ? hf_geoip_dst_asnum : hf_geoip_src_asnum;
        break;
      case WS_LAT_FAKE_EDITION:
        geoip_hf = hf_geoip_lat;
        geoip_local_hf = (isdst) ? hf_geoip_dst_lat : hf_geoip_src_lat;
        break;
      case WS_LON_FAKE_EDITION:
        geoip_hf = hf_geoip_lon;
        geoip_local_hf = (isdst) ? hf_geoip_dst_lon : hf_geoip_src_lon;
        break;
      default:
        continue;
        break;
    }

    if (geoip_str) {
      proto_item *item;
      if (db_type == WS_LAT_FAKE_EDITION || db_type == WS_LON_FAKE_EDITION) {
        /* Convert latitude, longitude to double. Fix bug #5077 */
        item = proto_tree_add_double_format_value(geoip_info_tree, geoip_local_hf,
          tvb, offset, 4, g_ascii_strtod(geoip_str, NULL), "%s", geoip_str);
        PROTO_ITEM_SET_GENERATED(item);
        item = proto_tree_add_double_format_value(geoip_info_tree, geoip_hf,
          tvb, offset, 4, g_ascii_strtod(geoip_str, NULL), "%s", geoip_str);
        PROTO_ITEM_SET_GENERATED(item);
        PROTO_ITEM_SET_HIDDEN(item);
      } else {
        item = proto_tree_add_unicode_string(geoip_info_tree, geoip_local_hf,
          tvb, offset, 4, geoip_str);
        PROTO_ITEM_SET_GENERATED(item);
        item = proto_tree_add_unicode_string(geoip_info_tree, geoip_hf,
          tvb, offset, 4, geoip_str);
        PROTO_ITEM_SET_GENERATED(item);
        PROTO_ITEM_SET_HIDDEN(item);
      }

      item_cnt++;
      proto_item_append_text(geoip_info_item, "%s%s",
                             plurality(item_cnt, "", ", "), geoip_str);
    }
  }

  if (item_cnt == 0)
    proto_item_append_text(geoip_info_item, "Unknown");
}

static void
add_geoip_info(proto_tree *tree, tvbuff_t *tvb, gint offset, guint32 src32,
               guint32 dst32)
{
  guint num_dbs;
  proto_item *geoip_info_item;

  num_dbs = geoip_db_num_dbs();
  if (num_dbs < 1)
    return;

  geoip_info_item = proto_tree_add_text(tree, tvb, offset + IPH_SRC, 4, "Source GeoIP: ");
  PROTO_ITEM_SET_GENERATED(geoip_info_item);
  add_geoip_info_entry(geoip_info_item, tvb, offset + IPH_SRC, src32, 0);

  geoip_info_item = proto_tree_add_text(tree, tvb, offset + IPH_DST, 4,
                                        "Destination GeoIP: ");
  PROTO_ITEM_SET_GENERATED(geoip_info_item);
  add_geoip_info_entry(geoip_info_item, tvb, offset + IPH_DST, dst32, 1);
}
#endif /* HAVE_GEOIP */

static const value_string ipopt_type_class_vals[] = {
  {(IPOPT_CONTROL & IPOPT_CLASS_MASK) >> 5, "Control"},
  {(IPOPT_RESERVED1 & IPOPT_CLASS_MASK) >> 5, "Reserved for future use"},
  {(IPOPT_MEASUREMENT & IPOPT_CLASS_MASK) >> 5, "Debugging and measurement"},
  {(IPOPT_RESERVED2 & IPOPT_CLASS_MASK) >> 5, "Reserved for future use"},
  {0, NULL}
};

static const value_string ipopt_type_number_vals[] = {
  {IPOPT_EOOL & IPOPT_NUMBER_MASK, "End of Option List (EOL)"},
  {IPOPT_NOP & IPOPT_NUMBER_MASK, "No-Operation (NOP)"},
  {IPOPT_SEC & IPOPT_NUMBER_MASK, "Security"},
  {IPOPT_LSR & IPOPT_NUMBER_MASK, "Loose source route"},
  {IPOPT_TS & IPOPT_NUMBER_MASK, "Time stamp"},
  {IPOPT_ESEC & IPOPT_NUMBER_MASK, "Extended security"},
  {IPOPT_CIPSO & IPOPT_NUMBER_MASK, "Commercial IP security option"},
  {IPOPT_RR & IPOPT_NUMBER_MASK, "Record route"},
  {IPOPT_SID & IPOPT_NUMBER_MASK, "Stream identifier"},
  {IPOPT_SSR & IPOPT_NUMBER_MASK, "Strict source route"},
  {IPOPT_ZSU & IPOPT_NUMBER_MASK, "Experimental Measurement"},
  {IPOPT_MTUP & IPOPT_NUMBER_MASK, "MTU probe"},
  {IPOPT_MTUR & IPOPT_NUMBER_MASK, "MTU Reply"},
  {IPOPT_FINN & IPOPT_NUMBER_MASK, "Experimental Flow Control"},
  {IPOPT_VISA & IPOPT_NUMBER_MASK, "Experimental Access Control"},
  {IPOPT_ENCODE & IPOPT_NUMBER_MASK, "Ask Estrin"},
  {IPOPT_IMITD & IPOPT_NUMBER_MASK, "IMI Traffic Descriptor"},
  {IPOPT_EIP & IPOPT_NUMBER_MASK, "Extended Internet Protocol"},
  {IPOPT_TR & IPOPT_NUMBER_MASK, "Traceroute"},
  {IPOPT_ADDEXT & IPOPT_NUMBER_MASK, "Address Extension"},
  {IPOPT_RTRALT & IPOPT_NUMBER_MASK, "Router Alert"},
  {IPOPT_SDB & IPOPT_NUMBER_MASK, "Selective Directed Broadcast"},
  {IPOPT_UN & IPOPT_NUMBER_MASK, "Unassigned"},
  {IPOPT_DPS & IPOPT_NUMBER_MASK, "Dynamic Packet State"},
  {IPOPT_UMP & IPOPT_NUMBER_MASK, "Upstream Multicast Packet"},
  {IPOPT_QS & IPOPT_NUMBER_MASK, "Quick-Start"},
  {IPOPT_EXP & IPOPT_NUMBER_MASK, "RFC 3692-style experiment"},
  {0, NULL}
};

static void
dissect_ipopt_type(tvbuff_t *tvb, int offset, proto_tree *tree)
{
  proto_tree *type_tree;
  proto_item *ti;

  ti = proto_tree_add_item(tree, hf_ip_opt_type, tvb, offset, 1, ENC_NA);
  type_tree = proto_item_add_subtree(ti, ett_ip_opt_type);
  proto_tree_add_item(type_tree, hf_ip_opt_type_copy, tvb, offset, 1, ENC_NA);
  proto_tree_add_item(type_tree, hf_ip_opt_type_class, tvb, offset, 1, ENC_NA);
  proto_tree_add_item(type_tree, hf_ip_opt_type_number, tvb, offset, 1, ENC_NA);
}

static void
dissect_ipopt_eool(const ip_tcp_opt *optp, tvbuff_t *tvb, int offset,
                   guint optlen _U_, packet_info *pinfo _U_,
                   proto_tree *opt_tree)
{
  proto_tree *field_tree;
  proto_item *tf;

  tf = proto_tree_add_text(opt_tree, tvb, offset,  1, "%s", optp->name);
  field_tree = proto_item_add_subtree(tf, *optp->subtree_index);
  dissect_ipopt_type(tvb, offset, field_tree);
}

#define dissect_ipopt_nop   dissect_ipopt_eool

  static const value_string secl_rfc791_vals[] = {
  {IPSEC_RFC791_UNCLASSIFIED, "Unclassified"},
  {IPSEC_RFC791_CONFIDENTIAL, "Confidential"},
  {IPSEC_RFC791_EFTO,         "EFTO"        },
  {IPSEC_RFC791_MMMM,         "MMMM"        },
  {IPSEC_RFC791_PROG,         "PROG"        },
  {IPSEC_RFC791_RESTRICTED,   "Restricted"  },
  {IPSEC_RFC791_SECRET,       "Secret"      },
  {IPSEC_RFC791_TOPSECRET,    "Top secret"  },
  {IPSEC_RFC791_RESERVED1,    "Reserved"    },
  {IPSEC_RFC791_RESERVED2,    "Reserved"    },
  {IPSEC_RFC791_RESERVED3,    "Reserved"    },
  {IPSEC_RFC791_RESERVED4,    "Reserved"    },
  {IPSEC_RFC791_RESERVED5,    "Reserved"    },
  {IPSEC_RFC791_RESERVED6,    "Reserved"    },
  {IPSEC_RFC791_RESERVED7,    "Reserved"    },
  {IPSEC_RFC791_RESERVED8,    "Reserved"    },
  {0,                  NULL          }
};

static const value_string sec_cl_vals[] = {
  {IPSEC_RESERVED4,    "Reserved 4"  },
  {IPSEC_TOPSECRET,    "Top secret"  },
  {IPSEC_SECRET,       "Secret"      },
  {IPSEC_CONFIDENTIAL, "Confidential"},
  {IPSEC_RESERVED3,    "Reserved 3"  },
  {IPSEC_RESERVED2,    "Reserved 2"  },
  {IPSEC_UNCLASSIFIED, "Unclassified"},
  {IPSEC_RESERVED1,    "Reserved 1"  },
  {0,                  NULL          }
};

static const true_false_string ip_opt_sec_prot_auth_flag_tfs = {
  "Datagram protected in accordance with its rules",
  "Datagram not protected in accordance with its rules"
};

static const true_false_string ip_opt_sec_prot_auth_fti_tfs = {
  "Additional octet present",
  "Final octet"
};

static const int *ip_opt_sec_prot_auth_fields_byte_1[] = {
  &hf_ip_opt_sec_prot_auth_genser,
  &hf_ip_opt_sec_prot_auth_siop_esi,
  &hf_ip_opt_sec_prot_auth_sci,
  &hf_ip_opt_sec_prot_auth_nsa,
  &hf_ip_opt_sec_prot_auth_doe,
  &hf_ip_opt_sec_prot_auth_unassigned,
  &hf_ip_opt_sec_prot_auth_fti,
  NULL
};

static const int *ip_opt_sec_prot_auth_fields_byte_n[] = {
  &hf_ip_opt_sec_prot_auth_unassigned2,
  &hf_ip_opt_sec_prot_auth_fti,
  NULL
};
static void
dissect_ipopt_security(const ip_tcp_opt *optp, tvbuff_t *tvb, int offset,
                       guint optlen, packet_info *pinfo, proto_tree *opt_tree)
{
  proto_tree *field_tree;
  proto_item *tf;
  proto_item *tf_sub;
  guint      val;
  guint      curr_offset = offset;

  tf = proto_tree_add_text(opt_tree, tvb, curr_offset, optlen, "%s (%u bytes)",
                           optp->name, optlen);
  field_tree = proto_item_add_subtree(tf, *optp->subtree_index);
  dissect_ipopt_type(tvb, curr_offset, field_tree);
  curr_offset++;
  tf_sub = proto_tree_add_item(field_tree, hf_ip_opt_len, tvb, curr_offset, 1, ENC_NA);
  if (optlen > IPOLEN_MAX)
    expert_add_info_format(pinfo, tf_sub, PI_PROTOCOL, PI_WARN,
                           "Invalid length for option");
  curr_offset++;

  if (optlen == 11) {
  /* Analyze payload start to decide whether it should be dissected
     according to RFC 791 or RFC 1108 */
    val = tvb_get_ntohs(tvb, curr_offset);
    if (match_strval(val, secl_rfc791_vals)) {
      /* Dissect as RFC 791 */
      proto_tree_add_item(field_tree, hf_ip_opt_sec_rfc791_sec,
                          tvb, curr_offset, 2, ENC_BIG_ENDIAN);
      curr_offset += 2;
      proto_tree_add_item(field_tree, hf_ip_opt_sec_rfc791_comp,
                          tvb, curr_offset, 2, ENC_BIG_ENDIAN);
      curr_offset += 2;
      proto_tree_add_item(field_tree, hf_ip_opt_sec_rfc791_hr,
                          tvb, curr_offset, 2, ENC_ASCII|ENC_NA);
      curr_offset += 2;
      proto_tree_add_item(field_tree, hf_ip_opt_sec_rfc791_tcc,
                          tvb, curr_offset, 3, ENC_ASCII|ENC_NA);
      return;
    }
  }

  /* Dissect as RFC 108 */
  proto_tree_add_item(field_tree, hf_ip_opt_sec_cl, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
  curr_offset++;
  if ((curr_offset - offset) >= optlen) {
    return;
  }
  val = tvb_get_guint8(tvb, curr_offset);
  proto_tree_add_bitmask(field_tree, tvb, curr_offset, hf_ip_opt_sec_prot_auth_flags,
                         ett_ip_opt_sec_prot_auth_flags, ip_opt_sec_prot_auth_fields_byte_1,
                         ENC_BIG_ENDIAN);
  curr_offset++;
  while (val & 0x01) {
    if ((val & 0x01) && ((curr_offset - offset) == optlen)) {
      expert_add_info_format(pinfo, tf_sub, PI_PROTOCOL, PI_WARN,
                             "Field Termination Indicator set to 1 for last byte of option");
      break;
    }
    val = tvb_get_guint8(tvb, curr_offset);
    proto_tree_add_bitmask(field_tree, tvb, curr_offset, hf_ip_opt_sec_prot_auth_flags,
                           ett_ip_opt_sec_prot_auth_flags, ip_opt_sec_prot_auth_fields_byte_n,
                           ENC_BIG_ENDIAN);
    curr_offset++;
  }
  if ((curr_offset - offset) < optlen) {
    expert_add_info_format(pinfo, tf, PI_PROTOCOL, PI_WARN,
                           "Extraneous data in option");
  }
}

static void
dissect_ipopt_ext_security(const ip_tcp_opt *optp, tvbuff_t *tvb, int offset,
                           guint optlen, packet_info *pinfo, proto_tree *opt_tree)
{
  proto_tree *field_tree;
  proto_item *tf;
  proto_item *tf_sub;
  guint      curr_offset = offset;
  gint      remaining;

  tf = proto_tree_add_text(opt_tree, tvb, curr_offset, optlen, "%s (%u bytes)",
                           optp->name, optlen);
  field_tree = proto_item_add_subtree(tf, *optp->subtree_index);
  dissect_ipopt_type(tvb, curr_offset, field_tree);
  curr_offset++;
  tf_sub = proto_tree_add_item(field_tree, hf_ip_opt_len, tvb, curr_offset, 1, ENC_NA);
  if (optlen > IPOLEN_MAX)
    expert_add_info_format(pinfo, tf_sub, PI_PROTOCOL, PI_WARN,
                           "Invalid length for option");
  curr_offset++;
  proto_tree_add_item(field_tree, hf_ip_opt_ext_sec_add_sec_info_format_code, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
  curr_offset++;
  remaining = optlen - (curr_offset - offset);
  if (remaining > 0) {
    proto_tree_add_item(field_tree, hf_ip_opt_ext_sec_add_sec_info, tvb, curr_offset, remaining, ENC_NA);
  }
}

/* USHRT_MAX can hold at most 5 (base 10) digits (6 for the NULL byte) */
#define USHRT_MAX_STRLEN    6

/* Maximum CIPSO tag length:
 * (IP hdr max)60 - (IPv4 hdr std)20 - (CIPSO base)6 = 34 */
#define CIPSO_TAG_LEN_MAX   34

/* The Commercial IP Security Option (CIPSO) is defined in IETF draft
 * draft-ietf-cipso-ipsecurity-01.txt and FIPS 188, a copy of both documents
 * can be found at the NetLabel project page, http://netlabel.sf.net or at
 * http://tools.ietf.org/html/draft-ietf-cipso-ipsecurity-01 */
static void
dissect_ipopt_cipso(const ip_tcp_opt *optp, tvbuff_t *tvb, int offset,
                    guint optlen, packet_info *pinfo, proto_tree *opt_tree)
{
  proto_tree *field_tree;
  proto_item *tf;
  guint      tagtype, taglen;
  int        offset_max = offset + optlen;

  tf = proto_tree_add_text(opt_tree, tvb, offset, optlen, "%s (%u bytes)",
                           optp->name, optlen);
  field_tree = proto_item_add_subtree(tf, *optp->subtree_index);
  dissect_ipopt_type(tvb, offset, field_tree);
  tf = proto_tree_add_item(field_tree, hf_ip_opt_len, tvb, offset + 1, 1, ENC_NA);
  if (optlen > IPOLEN_MAX)
    expert_add_info_format(pinfo, tf, PI_PROTOCOL, PI_WARN,
                           "Invalid length for option");

  offset += 2;

  proto_tree_add_text(field_tree, tvb, offset, 4, "DOI: %u",
                      tvb_get_ntohl(tvb, offset));
  offset += 4;

  /* loop through all of the tags in the CIPSO option */
  while (offset < offset_max) {
    tagtype = tvb_get_guint8(tvb, offset);

    if ((offset + 1) < offset_max)
      taglen = tvb_get_guint8(tvb, offset + 1);
    else
      taglen = 1;

    switch (tagtype) {
    case 0:
      /* padding - skip this tag */
      offset += 1;
      continue;
    case 1:
      /* restrictive bitmap, see CIPSO draft section 3.4.2 for tag format */
      if ((taglen < 4) || (taglen > CIPSO_TAG_LEN_MAX) ||
         ((offset + (int)taglen - 1) > offset_max)) {
        proto_tree_add_text(field_tree, tvb, offset, offset_max - offset,
                            "Malformed CIPSO tag");
        return;
      }

      proto_tree_add_text(field_tree, tvb, offset, 1,
                          "Tag Type: Restrictive Category Bitmap (%u)",
                          tagtype);

      /* skip past alignment octet */
      offset += 3;

      proto_tree_add_text(field_tree, tvb, offset, 1, "Sensitivity Level: %u",
                          tvb_get_guint8(tvb, offset));
      offset += 1;

      if (taglen > 4) {
        guint bit_spot = 0;
        guint byte_spot = 0;
        unsigned char bitmask;
        char *cat_str;
        char *cat_str_tmp = ep_alloc(USHRT_MAX_STRLEN);
        size_t cat_str_len;
        const guint8 *val_ptr = tvb_get_ptr(tvb, offset, taglen - 4);

        /* this is just a guess regarding string size, but we grow it below
         * if needed */
        cat_str_len = 256;
        cat_str = ep_alloc0(cat_str_len);

        /* we checked the length above so the highest category value
         * possible here is 240 */
        while (byte_spot < (taglen - 4)) {
          bitmask = 0x80;
          bit_spot = 0;
          while (bit_spot < 8) {
            if (val_ptr[byte_spot] & bitmask) {
              g_snprintf(cat_str_tmp, USHRT_MAX_STRLEN, "%u",
                         byte_spot * 8 + bit_spot);
              if (cat_str_len < (strlen(cat_str) + 2 + USHRT_MAX_STRLEN)) {
                char *cat_str_new;

                while (cat_str_len < (strlen(cat_str) + 2 + USHRT_MAX_STRLEN))
                  cat_str_len += cat_str_len;
                cat_str_new = ep_alloc(cat_str_len);
                g_strlcpy(cat_str_new, cat_str, cat_str_len);
                cat_str_new[cat_str_len - 1] = '\0';
                cat_str = cat_str_new;
              }
              if (cat_str[0] != '\0')
                g_strlcat(cat_str, ",", cat_str_len);
              g_strlcat(cat_str, cat_str_tmp, cat_str_len);
            }
            bit_spot++;
            bitmask >>= 1;
          }
          byte_spot++;
        }

        if (cat_str)
          proto_tree_add_text(field_tree, tvb, offset, taglen - 4,
                              "Categories: %s", cat_str);
        else
          proto_tree_add_text(field_tree, tvb, offset, taglen - 4,
                              "Categories: ERROR PARSING CATEGORIES");
        offset += taglen - 4;
      }
      break;
    case 2:
      /* enumerated categories, see CIPSO draft section 3.4.3 for tag format */
      if ((taglen < 4) || (taglen > CIPSO_TAG_LEN_MAX) ||
         ((offset + (int)taglen - 1) > offset_max)) {
        proto_tree_add_text(field_tree, tvb, offset, offset_max - offset,
                            "Malformed CIPSO tag");
        return;
      }

      proto_tree_add_text(field_tree, tvb, offset, 1,
                          "Tag Type: Enumerated Categories (%u)", tagtype);

      /* skip past alignment octet */
      offset += 3;

      /* sensitivity level */
      proto_tree_add_text(field_tree, tvb, offset, 1, "Sensitivity Level: %u",
                          tvb_get_guint8(tvb, offset));
      offset += 1;

      if (taglen > 4) {
        int offset_max_cat = offset + taglen - 4;
        char *cat_str = ep_alloc0(USHRT_MAX_STRLEN * 15);
        char *cat_str_tmp = ep_alloc(USHRT_MAX_STRLEN);

        while ((offset + 2) <= offset_max_cat) {
          g_snprintf(cat_str_tmp, USHRT_MAX_STRLEN, "%u",
                     tvb_get_ntohs(tvb, offset));
          offset += 2;
          if (cat_str[0] != '\0')
            g_strlcat(cat_str, ",", USHRT_MAX_STRLEN * 15);
          g_strlcat(cat_str, cat_str_tmp, USHRT_MAX_STRLEN * 15);
        }

        proto_tree_add_text(field_tree, tvb, offset - taglen + 4, taglen - 4,
                            "Categories: %s", cat_str);
      }
      break;
    case 5:
      /* ranged categories, see CIPSO draft section 3.4.4 for tag format */
      if ((taglen < 4) || (taglen > CIPSO_TAG_LEN_MAX) ||
         ((offset + (int)taglen - 1) > offset_max)) {
        proto_tree_add_text(field_tree, tvb, offset, offset_max - offset,
                            "Malformed CIPSO tag");
        return;
      }

      proto_tree_add_text(field_tree, tvb, offset, 1,
                          "Tag Type: Ranged Categories (%u)", tagtype);

      /* skip past alignment octet */
      offset += 3;

      /* sensitivity level */
      proto_tree_add_text(field_tree, tvb, offset, 1, "Sensitivity Level: %u",
                          tvb_get_guint8(tvb, offset));
      offset += 1;

      if (taglen > 4) {
        guint16 cat_low, cat_high;
        int offset_max_cat = offset + taglen - 4;
        char *cat_str = ep_alloc0(USHRT_MAX_STRLEN * 16);
        char *cat_str_tmp = ep_alloc(USHRT_MAX_STRLEN * 2);

        while ((offset + 2) <= offset_max_cat) {
          cat_high = tvb_get_ntohs(tvb, offset);
          if ((offset + 4) <= offset_max_cat) {
            cat_low = tvb_get_ntohs(tvb, offset + 2);
            offset += 4;
          } else {
            cat_low = 0;
            offset += 2;
          }
          if (cat_low != cat_high)
            g_snprintf(cat_str_tmp, USHRT_MAX_STRLEN * 2, "%u-%u",
                       cat_high, cat_low);
          else
            g_snprintf(cat_str_tmp, USHRT_MAX_STRLEN * 2, "%u", cat_high);

          if (cat_str[0] != '\0')
            g_strlcat(cat_str, ",", USHRT_MAX_STRLEN * 16);
          g_strlcat(cat_str, cat_str_tmp, USHRT_MAX_STRLEN * 16);
        }

        proto_tree_add_text(field_tree, tvb, offset - taglen + 4, taglen - 4,
                            "Categories: %s", cat_str);
      }
      break;
    case 6:
      /* permissive categories, see FIPS 188 section 6.9 for tag format */
      if ((taglen < 4) || (taglen > CIPSO_TAG_LEN_MAX) ||
         ((offset + (int)taglen - 1) > offset_max)) {
        proto_tree_add_text(field_tree, tvb, offset, offset_max - offset,
                            "Malformed CIPSO tag");
        return;
      }

      proto_tree_add_text(field_tree, tvb, offset, 1,
                          "Tag Type: Permissive Categories (%u)", tagtype);
      proto_tree_add_text(field_tree, tvb, offset + 2, taglen - 2, "Tag data");
      offset += taglen;
      break;
    case 7:
      /* free form, see FIPS 188 section 6.10 for tag format */
      if ((taglen < 2) || (taglen > CIPSO_TAG_LEN_MAX) ||
         ((offset + (int)taglen - 1) > offset_max)) {
        proto_tree_add_text(field_tree, tvb, offset, offset_max - offset,
                            "Malformed CIPSO tag");
        return;
      }

      proto_tree_add_text(field_tree, tvb, offset, 1,
                          "Tag Type: Free Form (%u)", tagtype);
      proto_tree_add_text(field_tree, tvb, offset + 2, taglen - 2, "Tag data");
      offset += taglen;
      break;
    default:
      /* unknown tag - stop parsing this IPv4 option */
      if ((offset + 1) <= offset_max) {
        taglen = tvb_get_guint8(tvb, offset + 1);
        proto_tree_add_text(field_tree, tvb, offset, 1,
                            "Tag Type: Unknown (%u) (%u bytes)",
                            tagtype, taglen);
        return;
      }
      proto_tree_add_text(field_tree, tvb, offset, 1,
                          "Tag Type: Unknown (%u) (invalid format)", tagtype);
      return;
    }
  }
}

static void
dissect_option_route(proto_tree *tree, tvbuff_t *tvb, int offset, int hf,
                     int hf_host, gboolean next)
{
  proto_item *ti;
  guint32 route;

  route = tvb_get_ipv4(tvb, offset);
  if (next)
    proto_tree_add_ipv4_format_value(tree, hf, tvb, offset, 4, route,
                                     "%s <- (next)",
                                     ip_to_str((gchar *)&route));
  else
    proto_tree_add_ipv4(tree, hf, tvb, offset, 4, route);
  ti = proto_tree_add_string(tree, hf_host, tvb, offset, 4, get_hostname(route));
  PROTO_ITEM_SET_GENERATED(ti);
  PROTO_ITEM_SET_HIDDEN(ti);
}

static void
dissect_ipopt_route(const ip_tcp_opt *optp, tvbuff_t *tvb, int offset,
                    guint optlen, packet_info *pinfo, proto_tree *opt_tree)
{
  proto_tree *field_tree;
  proto_item *tf;
  guint8 len, ptr;
  int optoffset = 0;

  tf = proto_tree_add_text(opt_tree, tvb, offset, optlen, "%s (%u bytes)",
                           optp->name, optlen);
  field_tree = proto_item_add_subtree(tf, *optp->subtree_index);
  dissect_ipopt_type(tvb, offset, field_tree);
  tf = proto_tree_add_item(field_tree, hf_ip_opt_len, tvb, offset + 1, 1, ENC_NA);
  if (optlen > IPOLEN_MAX)
    expert_add_info_format(pinfo, tf, PI_PROTOCOL, PI_WARN,
                           "Invalid length for option");
  ptr = tvb_get_guint8(tvb, offset + 2);
  tf = proto_tree_add_item(field_tree, hf_ip_opt_ptr, tvb, offset + 2, 1, ENC_NA);
  if ((ptr < (optp->optlen + 1)) || (ptr & 3)) {
    if (ptr < (optp->optlen + 1)) {
      expert_add_info_format(pinfo, tf, PI_PROTOCOL, PI_WARN,
                             "Pointer points before first address");
    }
    else {
      expert_add_info_format(pinfo, tf, PI_PROTOCOL, PI_WARN,
                             "Pointer points to middle of address");
    }
    return;
  }

  len = optlen;
  optoffset = 3;    /* skip past type, length and pointer */
  for (optlen -= 3; optlen > 0; optlen -= 4, optoffset += 4) {
    if (optlen < 4) {
      expert_add_info_format(pinfo, tf, PI_PROTOCOL, PI_WARN,
                             "Suboption would go past end of option");
      break;
    }

    if (ptr > len) {
      /* This is a recorded route */
      dissect_option_route(field_tree, tvb, offset + optoffset, hf_ip_rec_rt,
                           hf_ip_rec_rt_host, FALSE);
    } else if (optoffset == (len - 4)) {
      /* This is the the destination */
      proto_item *item;
      guint32 addr;
      const char *dst_host;

      addr = tvb_get_ipv4(tvb, offset + optoffset);
      dst_host = get_hostname(addr);
      proto_tree_add_ipv4(field_tree, hf_ip_dst, tvb,
                          offset + optoffset, 4, addr);
      item = proto_tree_add_ipv4(field_tree, hf_ip_addr, tvb,
                                 offset + optoffset, 4, addr);
      PROTO_ITEM_SET_HIDDEN(item);
      item = proto_tree_add_string(field_tree, hf_ip_dst_host, tvb,
                                   offset + optoffset, 4, dst_host);
      PROTO_ITEM_SET_GENERATED(item);
      PROTO_ITEM_SET_HIDDEN(item);
      item = proto_tree_add_string(field_tree, hf_ip_host, tvb,
                                   offset + optoffset, 4, dst_host);
      PROTO_ITEM_SET_GENERATED(item);
      PROTO_ITEM_SET_HIDDEN(item);
    } else if ((optoffset + 1) < ptr) {
      /* This is also a recorded route */
      dissect_option_route(field_tree, tvb, offset + optoffset, hf_ip_rec_rt,
                           hf_ip_rec_rt_host, FALSE);
    } else if ((optoffset + 1) == ptr) {
      /* This is the next source route.  TODO: Should we use separate hf's
       * for this, such as hf_ip_next_rt and hf_ip_next_rt_host and avoid
       * having to pass TRUE/FALSE to dissect_option_route()? */
      dissect_option_route(field_tree, tvb, offset + optoffset, hf_ip_src_rt,
                           hf_ip_src_rt_host, TRUE);
    } else {
      /* This must be a source route */
      dissect_option_route(field_tree, tvb, offset + optoffset, hf_ip_src_rt,
                           hf_ip_src_rt_host, FALSE);
    }
  }
}

static void
dissect_ipopt_record_route(const ip_tcp_opt *optp, tvbuff_t *tvb, int offset,
                           guint optlen, packet_info *pinfo,
                           proto_tree *opt_tree)
{
  proto_tree *field_tree;
  proto_item *tf;
  guint8 len, ptr;
  int optoffset = 0;

  tf = proto_tree_add_text(opt_tree, tvb, offset, optlen, "%s (%u bytes)",
                           optp->name, optlen);
  field_tree = proto_item_add_subtree(tf, *optp->subtree_index);
  dissect_ipopt_type(tvb, offset, field_tree);
  tf = proto_tree_add_item(field_tree, hf_ip_opt_len, tvb, offset + 1, 1, ENC_NA);
  if (optlen > IPOLEN_MAX)
    expert_add_info_format(pinfo, tf, PI_PROTOCOL, PI_WARN,
                           "Invalid length for option");
  ptr = tvb_get_guint8(tvb, offset + 2);
  tf = proto_tree_add_item(field_tree, hf_ip_opt_ptr, tvb, offset + 2, 1, ENC_NA);

  if ((ptr < (optp->optlen + 1)) || (ptr & 3)) {
    if (ptr < (optp->optlen + 1)) {
      expert_add_info_format(pinfo, tf, PI_PROTOCOL, PI_WARN,
                             "Pointer points before first address");
    }
    else {
      expert_add_info_format(pinfo, tf, PI_PROTOCOL, PI_WARN,
                             "Pointer points to middle of address");
    }
    return;
  }

  len = optlen;
  optoffset = 3;    /* skip past type, length and pointer */
  for (optlen -= 3; optlen > 0; optlen -= 4, optoffset += 4) {
    if (optlen < 4) {
      expert_add_info_format(pinfo, tf, PI_PROTOCOL, PI_WARN,
                             "Suboption would go past end of option");
      break;
    }

    if (ptr > len) {
      /* The recorded route data area is full. */
      dissect_option_route(field_tree, tvb, offset + optoffset, hf_ip_rec_rt,
                           hf_ip_rec_rt_host, FALSE);
    } else if ((optoffset + 1) < ptr) {
      /* This is a recorded route */
      dissect_option_route(field_tree, tvb, offset + optoffset, hf_ip_rec_rt,
                           hf_ip_rec_rt_host, FALSE);
    } else if ((optoffset + 1) == ptr) {
      /* This is the next available slot.  TODO: Should we use separate hf's
       * for this, such as hf_ip_next_rt and hf_ip_next_rt_host and avoid
       * having to pass TRUE/FALSE to dissect_option_route()? */
      dissect_option_route(field_tree, tvb, offset + optoffset, hf_ip_empty_rt,
                           hf_ip_empty_rt_host, TRUE);
    } else {
      /* This must be an available slot too. */
      dissect_option_route(field_tree, tvb, offset + optoffset, hf_ip_empty_rt,
                           hf_ip_empty_rt_host, FALSE);
    }
  }
}

/* Stream Identifier */
static void
dissect_ipopt_sid(const ip_tcp_opt *optp, tvbuff_t *tvb, int offset,
                  guint optlen, packet_info *pinfo, proto_tree *opt_tree)
{
  proto_tree *field_tree;
  proto_item *tf;

  tf = proto_tree_add_text(opt_tree, tvb, offset, optlen, "%s (%u bytes): %u",
                           optp->name, optlen, tvb_get_ntohs(tvb, offset + 2));
  field_tree = proto_item_add_subtree(tf, *optp->subtree_index);
  dissect_ipopt_type(tvb, offset, field_tree);
  tf = proto_tree_add_item(field_tree, hf_ip_opt_len, tvb, offset + 1, 1, ENC_NA);
  if (optlen != (guint)optp->optlen)
    expert_add_info_format(pinfo, tf, PI_PROTOCOL, PI_WARN,
                           "Invalid length for option");
  proto_tree_add_item(field_tree, hf_ip_opt_sid, tvb, offset + 2, 2, ENC_BIG_ENDIAN);
}

/* RFC 1063: MTU Probe and MTU Reply */
static void
dissect_ipopt_mtu(const ip_tcp_opt *optp, tvbuff_t *tvb, int offset,
                  guint optlen, packet_info *pinfo, proto_tree *opt_tree)
{
  proto_tree *field_tree;
  proto_item *tf;

  tf = proto_tree_add_text(opt_tree, tvb, offset, optlen, "%s (%u bytes): %u",
                           optp->name, optlen, tvb_get_ntohs(tvb, offset + 2));
  field_tree = proto_item_add_subtree(tf, *optp->subtree_index);
  dissect_ipopt_type(tvb, offset, field_tree);
  tf = proto_tree_add_item(field_tree, hf_ip_opt_len, tvb, offset + 1, 1, ENC_NA);
  if (optlen != (guint)optp->optlen)
    expert_add_info_format(pinfo, tf, PI_PROTOCOL, PI_WARN,
                           "Invalid length for option");
  proto_tree_add_item(field_tree, hf_ip_opt_mtu, tvb, offset + 2, 2, ENC_BIG_ENDIAN);
}

/* RFC 1393: Traceroute */
static void
dissect_ipopt_tr(const ip_tcp_opt *optp, tvbuff_t *tvb, int offset,
                  guint optlen, packet_info *pinfo, proto_tree *opt_tree)
{
  proto_tree *field_tree;
  proto_item *tf;

  tf = proto_tree_add_text(opt_tree, tvb, offset, optlen, "%s (%u bytes)",
                           optp->name, optlen);
  field_tree = proto_item_add_subtree(tf, *optp->subtree_index);
  dissect_ipopt_type(tvb, offset, field_tree);
  tf = proto_tree_add_item(field_tree, hf_ip_opt_len, tvb, offset + 1, 1, ENC_NA);
  if (optlen != (guint)optp->optlen)
    expert_add_info_format(pinfo, tf, PI_PROTOCOL, PI_WARN,
                           "Invalid length for option");

  proto_tree_add_item(field_tree, hf_ip_opt_id_number, tvb, offset + 2, 2, ENC_BIG_ENDIAN);
  proto_tree_add_item(field_tree, hf_ip_opt_ohc, tvb, offset + 4, 2, ENC_BIG_ENDIAN);
  proto_tree_add_item(field_tree, hf_ip_opt_rhc, tvb, offset + 6, 2, ENC_BIG_ENDIAN);
  proto_tree_add_item(field_tree, hf_ip_opt_originator, tvb, offset + 8, 4, ENC_BIG_ENDIAN);
}

static void
dissect_ipopt_timestamp(const ip_tcp_opt *optp, tvbuff_t *tvb,
                        int offset, guint optlen, packet_info *pinfo,
                        proto_tree *opt_tree)
{
  proto_tree *field_tree;
  proto_item *tf;
  int        ptr;
  int        optoffset = 0;
  int        flg;
  static const value_string flag_vals[] = {
    {IPOPT_TS_TSONLY,    "Time stamps only"                      },
    {IPOPT_TS_TSANDADDR, "Time stamp and address"                },
    {IPOPT_TS_PRESPEC,   "Time stamps for prespecified addresses"},
    {0,                  NULL                                    }};
  guint32 addr;
  guint ts;

  tf = proto_tree_add_text(opt_tree, tvb, offset, optlen, "%s (%u bytes)",
                           optp->name, optlen);
  field_tree = proto_item_add_subtree(tf, *optp->subtree_index);
  dissect_ipopt_type(tvb, offset, field_tree);
  tf = proto_tree_add_item(field_tree, hf_ip_opt_len, tvb, offset + 1, 1, ENC_NA);
  if (optlen > IPOLEN_MAX)
    expert_add_info_format(pinfo, tf, PI_PROTOCOL, PI_WARN,
                           "Invalid length for option");
  optoffset += 2;   /* skip past type and length */
  optlen -= 2;      /* subtract size of type and length */

  ptr = tvb_get_guint8(tvb, offset + optoffset);
  proto_tree_add_text(field_tree, tvb, offset + optoffset, 1, "Pointer: %d%s",
                      ptr, ((ptr == 1) ? " (header is full)" :
                      (ptr < 5) ? " (points before first address)" :
                      (((ptr - 1) & 3) ? " (points to middle of field)" : "")));
  optoffset++;
  optlen--;
  ptr--;    /* ptr is 1-origin */

  flg = tvb_get_guint8(tvb, offset + optoffset);
  proto_tree_add_text(field_tree, tvb, offset + optoffset, 1, "Overflow: %u",
                      flg >> 4);
  flg &= 0xF;
  proto_tree_add_text(field_tree, tvb, offset + optoffset, 1, "Flag: %s",
                      val_to_str(flg, flag_vals, "Unknown (0x%x)"));
  optoffset++;
  optlen--;

  while (optlen > 0) {
    if (flg == IPOPT_TS_TSANDADDR || flg == IPOPT_TS_PRESPEC) {
      if (optlen < 8) {
        proto_tree_add_text(field_tree, tvb, offset + optoffset, optlen,
                            "(suboption would go past end of option)");
        break;
      }
      addr = tvb_get_ipv4(tvb, offset + optoffset);
      ts = tvb_get_ntohl(tvb, offset + optoffset + 4);
      optlen -= 8;
      proto_tree_add_text(field_tree, tvb, offset + optoffset, 8,
                          "Address = %s, time stamp = %u",
                          ((addr == 0) ? "-" :
                          get_hostname(addr)), ts);
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

/* Router Alert */
static const range_string ra_rvals[] = {
  {0, 0, "Router shall examine packet"},
  {1, 65535, "Reserved"},
  {0, 0, NULL}
};

static void
dissect_ipopt_ra(const ip_tcp_opt *optp, tvbuff_t *tvb, int offset,
                 guint optlen, packet_info *pinfo, proto_tree *opt_tree)
{
  /* Router-Alert, as defined by RFC2113 */
  proto_tree *field_tree;
  proto_item *tf;
  guint16 value = tvb_get_ntohs(tvb, offset + 2);

  tf = proto_tree_add_text(opt_tree, tvb, offset, optlen,
                           "%s (%u bytes): %s (%u)", optp->name, optlen,
                           rval_to_str(value, ra_rvals, "Unknown (%u)"),
                           value);
  field_tree = proto_item_add_subtree(tf, *optp->subtree_index);
  dissect_ipopt_type(tvb, offset, field_tree);
  tf = proto_tree_add_item(field_tree, hf_ip_opt_len, tvb, offset + 1, 1, ENC_NA);
  if (optlen != (guint)optp->optlen)
    expert_add_info_format(pinfo, tf, PI_PROTOCOL, PI_WARN,
                           "Invalid length for option");
  proto_tree_add_item(field_tree, hf_ip_opt_ra, tvb, offset + 2, 2, ENC_BIG_ENDIAN);
}

/* RFC 1770: Selective Directed Broadcast */
static void
dissect_ipopt_sdb(const ip_tcp_opt *optp, tvbuff_t *tvb, int offset,
                 guint optlen, packet_info *pinfo, proto_tree *opt_tree)
{
  proto_tree *field_tree;
  proto_item *tf;

  tf = proto_tree_add_text(opt_tree, tvb, offset, optlen, "%s (%u bytes)",
                           optp->name, optlen);
  field_tree = proto_item_add_subtree(tf, *optp->subtree_index);
  dissect_ipopt_type(tvb, offset, field_tree);
  tf = proto_tree_add_item(field_tree, hf_ip_opt_len, tvb, offset + 1, 1, ENC_NA);
  if (optlen > IPOLEN_MAX)
    expert_add_info_format(pinfo, tf, PI_PROTOCOL, PI_WARN,
                           "Invalid length for option");
  for (offset += 2, optlen -= 2; optlen >= 4; offset += 4, optlen -= 4)
    proto_tree_add_item(field_tree, hf_ip_opt_addr, tvb, offset, 4, ENC_BIG_ENDIAN);

  if (optlen > 0)
    proto_tree_add_item(field_tree, hf_ip_opt_padding, tvb, offset, optlen, ENC_NA);
}

const value_string qs_func_vals[] = {
  {QS_RATE_REQUEST, "Rate request"},
  {QS_RATE_REPORT,  "Rate report"},
  {0,               NULL}
};

static const value_string qs_rate_vals[] = {
  { 0, "0 bit/s"},
  { 1, "80 Kbit/s"},
  { 2, "160 Kbit/s"},
  { 3, "320 Kbit/s"},
  { 4, "640 Kbit/s"},
  { 5, "1.28 Mbit/s"},
  { 6, "2.56 Mbit/s"},
  { 7, "5.12 Mbit/s"},
  { 8, "10.24 Mbit/s"},
  { 9, "20.48 Mbit/s"},
  {10, "40.96 Mbit/s"},
  {11, "81.92 Mbit/s"},
  {12, "163.84 Mbit/s"},
  {13, "327.68 Mbit/s"},
  {14, "655.36 Mbit/s"},
  {15, "1.31072 Gbit/s"},
  {0, NULL}
};
value_string_ext qs_rate_vals_ext = VALUE_STRING_EXT_INIT(qs_rate_vals);

static void
dissect_ipopt_qs(const ip_tcp_opt *optp, tvbuff_t *tvb, int offset,
                 guint optlen, packet_info *pinfo, proto_tree *opt_tree)
{
  proto_tree *field_tree;
  proto_item *tf;
  proto_item *ti;

  guint8 command = tvb_get_guint8(tvb, offset + 2);
  guint8 function = command >> 4;
  guint8 rate = command & QS_RATE_MASK;
  guint8 ttl_diff;

  tf = proto_tree_add_text(opt_tree, tvb, offset, optlen,
                           "%s (%u bytes): %s (%u)", optp->name, optlen,
                           val_to_str(function, qs_func_vals, "Unknown (%u)"),
                           function);
  field_tree = proto_item_add_subtree(tf, *optp->subtree_index);
  dissect_ipopt_type(tvb, offset, field_tree);
  ti = proto_tree_add_item(field_tree, hf_ip_opt_len, tvb, offset + 1, 1, ENC_NA);
  if (optlen != (guint)optp->optlen)
    expert_add_info_format(pinfo, ti, PI_PROTOCOL, PI_WARN,
                           "Invalid length for option");
  proto_tree_add_item(field_tree, hf_ip_opt_qs_func, tvb, offset + 2, 1, ENC_NA);

  if (function == QS_RATE_REQUEST) {
    proto_tree_add_item(field_tree, hf_ip_opt_qs_rate, tvb, offset + 2, 1, ENC_NA);
    proto_tree_add_item(field_tree, hf_ip_opt_qs_ttl, tvb, offset + 3, 1, ENC_NA);
    ttl_diff = (pinfo->ip_ttl - tvb_get_guint8(tvb, offset + 3) % 256);
    ti = proto_tree_add_uint_format_value(field_tree, hf_ip_opt_qs_ttl_diff,
                                          tvb, offset + 3, 1, ttl_diff,
                                          "%u", ttl_diff);
    PROTO_ITEM_SET_GENERATED(ti);
    proto_item_append_text(tf, ", %s, QS TTL %u, QS TTL diff %u",
                           val_to_str_ext(rate, &qs_rate_vals_ext, "Unknown (%u)"),
                           tvb_get_guint8(tvb, offset + 3), ttl_diff);
    proto_tree_add_item(field_tree, hf_ip_opt_qs_nonce, tvb, offset + 4, 4, ENC_NA);
    proto_tree_add_item(field_tree, hf_ip_opt_qs_reserved, tvb, offset + 4, 4, ENC_NA);
  } else if (function == QS_RATE_REPORT) {
    proto_tree_add_item(field_tree, hf_ip_opt_qs_rate, tvb, offset + 2, 1, ENC_NA);
    proto_item_append_text(tf, ", %s",
                           val_to_str_ext(rate, &qs_rate_vals_ext, "Unknown (%u)"));
    proto_tree_add_item(field_tree, hf_ip_opt_qs_unused, tvb, offset + 3, 1, ENC_NA);
    proto_tree_add_item(field_tree, hf_ip_opt_qs_nonce, tvb, offset + 4, 4, ENC_NA);
    proto_tree_add_item(field_tree, hf_ip_opt_qs_reserved, tvb, offset + 4, 4, ENC_NA);
  }
}

static const ip_tcp_opt ipopts[] = {
  {IPOPT_EOOL, "End of Options List (EOL)", &ett_ip_option_eool,
    OPT_LEN_NO_LENGTH, 0, dissect_ipopt_eool},
  {IPOPT_NOP, "No Operation (NOP)", &ett_ip_option_nop,
    OPT_LEN_NO_LENGTH, 0, dissect_ipopt_nop},
  {IPOPT_SEC, "Security", &ett_ip_option_sec,
    OPT_LEN_VARIABLE_LENGTH, IPOLEN_SEC_MIN, dissect_ipopt_security},
  {IPOPT_LSR, "Loose Source Route", &ett_ip_option_route,
    OPT_LEN_VARIABLE_LENGTH, IPOLEN_LSR_MIN, dissect_ipopt_route},
  {IPOPT_TS, "Time Stamp", &ett_ip_option_timestamp,
    OPT_LEN_VARIABLE_LENGTH, IPOLEN_TS_MIN, dissect_ipopt_timestamp},
  {IPOPT_ESEC, "Extended Security", &ett_ip_option_ext_security,
    OPT_LEN_VARIABLE_LENGTH, IPOLEN_ESEC_MIN, dissect_ipopt_ext_security},
  {IPOPT_CIPSO, "Commercial Security", &ett_ip_option_cipso,
    OPT_LEN_VARIABLE_LENGTH, IPOLEN_CIPSO_MIN, dissect_ipopt_cipso},
  {IPOPT_RR, "Record Route", &ett_ip_option_route,
    OPT_LEN_VARIABLE_LENGTH, IPOLEN_RR_MIN, dissect_ipopt_record_route},
  {IPOPT_SID, "Stream ID", &ett_ip_option_sid,
    OPT_LEN_FIXED_LENGTH, IPOLEN_SID, dissect_ipopt_sid},
  {IPOPT_SSR, "Strict Source Route", &ett_ip_option_route,
    OPT_LEN_VARIABLE_LENGTH, IPOLEN_SSR_MIN, dissect_ipopt_route},
#if 0 /* TODO */
  {IPOPT_ZSU, "Experimental Measurement", &ett_ip_option_zsu,
    OPT_LEN_VARIABLE_LENGTH /* ? */, IPOLEN_ZSU_MIN, dissect_ipopt_zsu},
#endif
  {IPOPT_MTUP, "MTU Probe", &ett_ip_option_mtu,
    OPT_LEN_FIXED_LENGTH, IPOLEN_MTU, dissect_ipopt_mtu},
  {IPOPT_MTUR, "MTU Reply", &ett_ip_option_mtu,
    OPT_LEN_FIXED_LENGTH, IPOLEN_MTU, dissect_ipopt_mtu},
#if 0 /* TODO */
  {IPOPT_FINN, "Experimental Flow Control", &ett_ip_option_finn,
    OPT_LEN_VARIABLE_LENGTH /* ? */, IPOLEN_FINN_MIN, dissect_ipopt_finn},
  {IPOPT_VISA, "Experimental Access Control", &ett_ip_option_visa,
    OPT_LEN_VARIABLE_LENGTH /* ? */, IPOLEN_VISA_MIN, dissect_ipopt_visa},
  {IPOPT_ENCODE, "???", &ett_ip_option_encode,
    OPT_LEN_VARIABLE_LENGTH /* ? */, IPOLEN_ENCODE_MIN, dissect_ipopt_encode},
  {IPOPT_IMITD, "IMI Traffic Descriptor", &ett_ip_option_imitd,
    OPT_LEN_VARIABLE_LENGTH /* ? */, IPOLEN_IMITD_MIN, dissect_ipopt_imitd},
  {IPOPT_EIP, "Extended Internet Protocol", &ett_ip_option_eip,
    OPT_LEN_VARIABLE_LENGTH /* ? */, IPOLEN_EIP_MIN, dissect_ipopt_eip},
#endif
  {IPOPT_TR, "Traceroute", &ett_ip_option_tr,
    OPT_LEN_FIXED_LENGTH, IPOLEN_TR, dissect_ipopt_tr},
#if 0 /* TODO */
  {IPOPT_ADDEXT, "Address Extension", &ett_ip_option_addext,
    OPT_LEN_VARIABLE_LENGTH /* ? */, IPOLEN_ADDEXT_MIN, dissect_ipopt_addext},
#endif
  {IPOPT_RTRALT, "Router Alert", &ett_ip_option_ra,
    OPT_LEN_FIXED_LENGTH, IPOLEN_RA, dissect_ipopt_ra},
  {IPOPT_SDB, "Selective Directed Broadcast", &ett_ip_option_sdb,
    OPT_LEN_VARIABLE_LENGTH, IPOLEN_SDB_MIN, dissect_ipopt_sdb},
#if 0 /* TODO */
  {IPOPT_UN, "Unassigned", &ett_ip_option_un,
    OPT_LEN_VARIABLE_LENGTH /* ? */, IPOLEN_UN_MIN, dissect_ipopt_un},
  {IPOPT_DPS, "Dynamic Packet State", &ett_ip_option_dps,
    OPT_LEN_VARIABLE_LENGTH /* ? */, IPOLEN_DPS_MIN, dissect_ipopt_dps},
  {IPOPT_UMP, "Upstream Multicast Pkt.", &ett_ip_option_ump,
    OPT_LEN_VARIABLE_LENGTH /* ? */, IPOLEN_UMP_MIN, dissect_ipopt_ump},
#endif
  {IPOPT_QS, "Quick-Start", &ett_ip_option_qs,
    OPT_LEN_FIXED_LENGTH, IPOLEN_QS, dissect_ipopt_qs}
#if 0 /* TODO */
  {IPOPT_EXP, "RFC3692-style Experiment", &ett_ip_option_exp,
    OPT_LEN_VARIABLE_LENGTH /* ? */, IPOLEN_EXP_MIN, dissect_ipopt_exp}
#endif
};

#define N_IP_OPTS       array_length(ipopts)

/* Dissect the IP, TCP or various PPP protocols (IPCP, CP, LCP, VSNCP, BAP)
 * options in a packet. */
void
dissect_ip_tcp_options(tvbuff_t *tvb, int offset, guint length,
                       const ip_tcp_opt *opttab, int nopts, int eol,
                        packet_info *pinfo, proto_tree *opt_tree,
                        proto_item *opt_item)
{
  guchar            opt;
  const ip_tcp_opt *optp;
  opt_len_type      len_type;
  unsigned int      optlen;
  const char       *name;
  void            (*dissect)(const struct ip_tcp_opt *, tvbuff_t *,
                             int, guint, packet_info *, proto_tree *);
  guint             len, nop_count = 0;

  while (length > 0) {
    opt = tvb_get_guint8(tvb, offset);
    for (optp = &opttab[0]; optp < &opttab[nopts]; optp++) {
      if (optp->optcode == opt)
        break;
    }
    if (optp == &opttab[nopts]) {
      /* We assume that the only OPT_LEN_NO_LENGTH options are EOL and NOP options,
         so that we can treat unknown options as OPT_LEN_VARIABLE_LENGTH with a
         minimum of 2, and at least be able to move on to the next option
         by using the length in the option. */
      optp = NULL;  /* indicate that we don't know this option */
      len_type = OPT_LEN_VARIABLE_LENGTH;
      optlen = 2;
      name = ep_strdup_printf("Unknown (0x%02x)", opt);
      dissect = NULL;
      nop_count = 0;
    } else {
      len_type = optp->len_type;
      optlen = optp->optlen;
      name = optp->name;
      dissect = optp->dissect;
      if (opt_item && len_type == OPT_LEN_NO_LENGTH && optlen == 0 && opt == 1 &&
         (nop_count == 0 || offset % 4)) { /* opt 1 = NOP in both IP and TCP */
        /* Count number of NOP in a row within a uint32 */
        nop_count++;
      } else {
        nop_count = 0;
      }
    }
    --length;      /* account for type byte */
    if (len_type != OPT_LEN_NO_LENGTH) {
      /* Option has a length. Is it in the packet? */
      if (length == 0) {
        /* Bogus - packet must at least include option code byte and
           length byte! */
        proto_tree_add_text(opt_tree, tvb, offset, 1,
                            "%s (length byte past end of options)", name);
        return;
      }
      len = tvb_get_guint8(tvb, offset + 1);  /* total including type, len */
      --length;    /* account for length byte */
      if (len < 2) {
        /* Bogus - option length is too short to include option code and
           option length. */
        proto_tree_add_text(opt_tree, tvb, offset, 2,
                            "%s (with too-short option length = %u byte%s)",
                            name, len, plurality(len, "", "s"));
        return;
      } else if (len - 2 > length) {
        /* Bogus - option goes past the end of the header. */
        proto_tree_add_text(opt_tree, tvb, offset, length,
                            "%s (option length = %u byte%s says option goes "
                            "past end of options)",
                            name, len, plurality(len, "", "s"));
        return;
      } else if (len_type == OPT_LEN_FIXED_LENGTH && len != optlen) {
        /* Bogus - option length isn't what it's supposed to be for this
           option. */
        proto_tree_add_text(opt_tree, tvb, offset, len,
                            "%s (with option length = %u byte%s; should be %u)",
                            name, len, plurality(len, "", "s"), optlen);
        return;
      } else if (len_type == OPT_LEN_VARIABLE_LENGTH && len < optlen) {
        /* Bogus - option length is less than what it's supposed to be for
           this option. */
        proto_tree_add_text(opt_tree, tvb, offset, len,
                            "%s (with option length = %u byte%s; "
                            "should be >= %u)",
                            name, len, plurality(len, "", "s"), optlen);
        return;
      } else {
        if (optp == NULL) {
          proto_tree_add_text(opt_tree, tvb, offset, len, "%s (%u byte%s)",
                              name, len, plurality(len, "", "s"));
        } else {
          if (dissect != NULL) {
            /* Option has a dissector. */
            proto_item_append_text(proto_tree_get_parent(opt_tree), ", %s",
                                   optp->name);
            (*dissect)(optp, tvb, offset, len, pinfo, opt_tree);
          } else {
            proto_tree *field_tree;
            proto_item *tf;

            /* Option has no data, hence no dissector. */
            proto_item_append_text(proto_tree_get_parent(opt_tree), ", %s",
                                   name);
            tf = proto_tree_add_text(opt_tree, tvb, offset,  len, "%s", name);
            field_tree = proto_item_add_subtree(tf, ett_ip_option_other);
            dissect_ipopt_type(tvb, offset, field_tree);
          }
        }
        len -= 2;   /* subtract size of type and length */
        offset += 2 + len;
      }
      length -= len;
    } else {
      if (dissect != NULL) {
        proto_item_append_text(proto_tree_get_parent(opt_tree), ", %s",
                               optp->name);
        (*dissect)(optp, tvb, offset, 1, pinfo, opt_tree);
      } else {
        proto_tree *field_tree;
        proto_item *tf;

        /* Option has no data, hence no dissector. */
        proto_item_append_text(proto_tree_get_parent(opt_tree), ", %s", name);
        tf = proto_tree_add_text(opt_tree, tvb, offset,  1, "%s", name);
        field_tree = proto_item_add_subtree(tf, ett_ip_option_other);
        dissect_ipopt_type(tvb, offset, field_tree);
      }
      offset += 1;

      if (nop_count == 4 && strcmp (name, "No-Operation (NOP)") == 0) {
        expert_add_info_format(pinfo, opt_item, PI_PROTOCOL, PI_WARN,
                               "4 NOP in a row - a router may have removed "
                               "some options");
      }
    }
    if (opt == eol)
      break;
  }
}

/* This function searches the IP options for either a loose or strict source
 * route option, then returns the offset to the destination address if the
 * pointer is still valid or zero if the pointer is greater than the length.
 *
 * The guts of this function was taken from dissect_ip_tcp_options().
 */
static int
get_dst_offset(tvbuff_t *tvb, int offset, guint length,
               const ip_tcp_opt *opttab, int nopts, int eol)
{
  guchar            opt;
  const ip_tcp_opt *optp;
  opt_len_type      len_type;
  unsigned int      optlen;
  guint             len;
  int               orig_offset = offset;

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
      optp = NULL;  /* indicate that we don't know this option */
      len_type = OPT_LEN_VARIABLE_LENGTH;
      optlen = 2;
    } else {
      len_type = optp->len_type;
      optlen = optp->optlen;
    }
    --length;      /* account for type byte */
    if (len_type != OPT_LEN_NO_LENGTH) {
      /* Option has a length. Is it in the packet? */
      if (length == 0) {
        /* Bogus - packet must at least include option code byte and
           length byte! */
        return 0;
      }
      len = tvb_get_guint8(tvb, offset + 1);  /* total including type, len */
      --length;    /* account for length byte */
      if (len < 2) {
        /* Bogus - option length is too short to include option code and
           option length. */
        return 0;
      } else if (len - 2 > length) {
        /* Bogus - option goes past the end of the header. */
        return 0;
      } else if (len_type == OPT_LEN_FIXED_LENGTH && len != optlen) {
        /* Bogus - option length isn't what it's supposed to be for this
           option. */
        return 0;
      } else if (len_type == OPT_LEN_VARIABLE_LENGTH && len < optlen) {
        /* Bogus - option length is less than what it's supposed to be for
           this option. */
        return 0;
      } else {
        if (optp != NULL) {
          if (opt == IPOPT_SSR || opt == IPOPT_LSR) {
            /* Hmm, what if you have both options? */
            guint8 ptr;

            ptr = tvb_get_guint8(tvb, offset + 2);
            if (ptr < 4 || (ptr & 3) || (ptr > len)) {
              return 0;
            }
            return (offset - orig_offset) + 4 + (len - 4);
          }
        }
        len -= 2;   /* subtract size of type and length */
        offset += 2 + len;
      }
      length -= len;
    } else {
      offset += 1;
    }
    if (opt == eol)
      return 0;
  }
  return 0;
}

/* Returns the valid ttl for the group address */
static guint16
local_network_control_block_addr_valid_ttl(guint32 addr)
{
  /* An exception list, as some protocols seem to insist on
   * doing differently:
   */

  /* IETF's VRRP (rfc3768) */
  if (IPLOCAL_NETWRK_CTRL_BLK_VRRP_ADDR == addr)
    return IPLOCAL_NETWRK_CTRL_BLK_VRRP_TTL;
  /* Cisco's GLPB */
  if (IPLOCAL_NETWRK_CTRL_BLK_GLPB_ADDR == addr)
    return IPLOCAL_NETWRK_CTRL_BLK_GLPB_TTL;
  /* mDNS (draft-cheshire-dnsext-multicastdns-07) */
  if (IPLOCAL_NETWRK_CTRL_BLK_MDNS_ADDR == addr)
    return IPLOCAL_NETWRK_CTRL_BLK_MDNS_TTL;
  /* LLMNR (rfc4795) */
  if (IPLOCAL_NETWRK_CTRL_BLK_LLMNR_ADDR == addr)
    return IPLOCAL_NETWRK_CTRL_BLK_ANY_TTL;
  return IPLOCAL_NETWRK_CTRL_BLK_DEFAULT_TTL;
}

const value_string dscp_vals[] = {
  { IPDSFIELD_DSCP_DEFAULT, "Default"               },
  { IPDSFIELD_DSCP_CS1,     "Class Selector 1"      },
  { IPDSFIELD_DSCP_AF11,    "Assured Forwarding 11" },
  { IPDSFIELD_DSCP_AF12,    "Assured Forwarding 12" },
  { IPDSFIELD_DSCP_AF13,    "Assured Forwarding 13" },
  { IPDSFIELD_DSCP_CS2,     "Class Selector 2"      },
  { IPDSFIELD_DSCP_AF21,    "Assured Forwarding 21" },
  { IPDSFIELD_DSCP_AF22,    "Assured Forwarding 22" },
  { IPDSFIELD_DSCP_AF23,    "Assured Forwarding 23" },
  { IPDSFIELD_DSCP_CS3,     "Class Selector 3"      },
  { IPDSFIELD_DSCP_AF31,    "Assured Forwarding 31" },
  { IPDSFIELD_DSCP_AF32,    "Assured Forwarding 32" },
  { IPDSFIELD_DSCP_AF33,    "Assured Forwarding 33" },
  { IPDSFIELD_DSCP_CS4,     "Class Selector 4"      },
  { IPDSFIELD_DSCP_AF41,    "Assured Forwarding 41" },
  { IPDSFIELD_DSCP_AF42,    "Assured Forwarding 42" },
  { IPDSFIELD_DSCP_AF43,    "Assured Forwarding 43" },
  { IPDSFIELD_DSCP_CS5,     "Class Selector 5"      },
  { IPDSFIELD_DSCP_EF,      "Expedited Forwarding"  },
  { IPDSFIELD_DSCP_CS6,     "Class Selector 6"      },
  { IPDSFIELD_DSCP_CS7,     "Class Selector 7"      },
  { 0,                      NULL                    }};
value_string_ext dscp_vals_ext = VALUE_STRING_EXT_INIT(dscp_vals);

const value_string ecn_vals[] = {
  { IPDSFIELD_ECT_NOT, "Not-ECT (Not ECN-Capable Transport)" },
  { IPDSFIELD_ECT_1,   "ECT(1) (ECN-Capable Transport)"      },
  { IPDSFIELD_ECT_0,   "ECT(0) (ECN-Capable Transport)"      },
  { IPDSFIELD_CE,      "CE (Congestion Experienced)"         },
  { 0,                 NULL                                  }};

static const value_string precedence_vals[] = {
  { IPTOS_PREC_ROUTINE,         "routine"              },
  { IPTOS_PREC_PRIORITY,        "priority"             },
  { IPTOS_PREC_IMMEDIATE,       "immediate"            },
  { IPTOS_PREC_FLASH,           "flash"                },
  { IPTOS_PREC_FLASHOVERRIDE,   "flash override"       },
  { IPTOS_PREC_CRITIC_ECP,      "CRITIC/ECP"           },
  { IPTOS_PREC_INTERNETCONTROL, "internetwork control" },
  { IPTOS_PREC_NETCONTROL,      "network control"      },
  { 0,                          NULL                   }};

static const value_string iptos_vals[] = {
  { IPTOS_NONE,        "None" },
  { IPTOS_LOWCOST,     "Minimize cost" },
  { IPTOS_RELIABILITY, "Maximize reliability" },
  { IPTOS_THROUGHPUT,  "Maximize throughput" },
  { IPTOS_LOWDELAY,    "Minimize delay" },
  { IPTOS_SECURITY,    "Maximize security" },
  { 0,                 NULL }
};

static const true_false_string tos_set_low = {
  "Low",
  "Normal"
};

static const true_false_string tos_set_high = {
  "High",
  "Normal"
};

static const true_false_string flags_sf_set_evil = {
  "Evil",
  "Not evil"
};

guint16
ip_checksum(const guint8 *ptr, int len)
{
  vec_t cksum_vec[1];

  cksum_vec[0].ptr = ptr;
  cksum_vec[0].len = len;
  return in_cksum(&cksum_vec[0], 1);
}

static void
dissect_ip(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree)
{
  proto_tree *ip_tree = NULL, *field_tree = NULL;
  proto_item *ti = NULL, *tf;
  guint32    addr;
  int        offset = 0, dst_off;
  guint      hlen, optlen;
  guint16    flags;
  guint8     nxt;
  guint16    ipsum;
  fragment_data *ipfd_head = NULL;
  tvbuff_t   *next_tvb;
  gboolean   update_col_info = TRUE;
  gboolean   save_fragmented;
  ws_ip *iph;
  const guchar *src_addr, *dst_addr;
  guint32    src32, dst32;
  proto_tree *tree;
  proto_item *item = NULL, *ttl_item;
  proto_tree *checksum_tree;
  guint16 ttl;

  tree = parent_tree;
  iph = ep_alloc(sizeof(ws_ip));

  col_set_str(pinfo->cinfo, COL_PROTOCOL, "IPv4");
  col_clear(pinfo->cinfo, COL_INFO);

  iph->ip_v_hl = tvb_get_guint8(tvb, offset);
  if ( hi_nibble(iph->ip_v_hl) == 6) {
    call_dissector(ipv6_handle, tvb, pinfo, parent_tree);
    return;
  }

  hlen = lo_nibble(iph->ip_v_hl) * 4;   /* IP header length, in bytes */

  if (tree) {
    ti = proto_tree_add_item(tree, proto_ip, tvb, offset, hlen, ENC_NA);
    ip_tree = proto_item_add_subtree(ti, ett_ip);

    proto_tree_add_uint(ip_tree, hf_ip_version, tvb, offset, 1,
                        hi_nibble(iph->ip_v_hl));
  }

  /* if IP is not referenced from any filters we don't need to worry about
     generating any tree items.  We must do this after we created the actual
     protocol above so that proto hier stat still works though.
     XXX: Note that because of the following optimization expert items must
          not be generated inside of an 'if (tree) ...'
          so that Analyze ! Expert ...  will work.
  */
  if (!proto_field_is_referenced(parent_tree, proto_ip)) {
    tree = NULL;
  }

  if (hlen < IPH_MIN_LEN) {
    col_add_fstr(pinfo->cinfo, COL_INFO,
                 "Bogus IP header length (%u, must be at least %u)",
                 hlen, IPH_MIN_LEN);
    if (tree) {
      proto_tree_add_uint_format(ip_tree, hf_ip_hdr_len, tvb, offset, 1, hlen,
                                 "Header length: %u bytes (bogus, must be "
                                 "at least %u)", hlen, IPH_MIN_LEN);
    }
    return;
  }

  if (tree) {
    proto_tree_add_uint_format(ip_tree, hf_ip_hdr_len, tvb, offset, 1, hlen,
                               "Header length: %u bytes", hlen);
  }

  iph->ip_tos = tvb_get_guint8(tvb, offset + 1);
  if (g_ip_dscp_actif) {
    col_add_fstr(pinfo->cinfo, COL_DSCP_VALUE, "%u",
                 IPDSFIELD_DSCP(iph->ip_tos));
  }

  if (tree) {
    if (g_ip_dscp_actif) {
      tf = proto_tree_add_uint_format(ip_tree, hf_ip_dsfield, tvb, offset + 1,
        1, iph->ip_tos, "Differentiated Services Field: 0x%02x "
        "(DSCP 0x%02x: %s; ECN: 0x%02x: %s)", iph->ip_tos,
        IPDSFIELD_DSCP(iph->ip_tos), val_to_str_ext_const(IPDSFIELD_DSCP(iph->ip_tos),
                                                          &dscp_vals_ext, "Unknown DSCP"),
        IPDSFIELD_ECN(iph->ip_tos), val_to_str_const(IPDSFIELD_ECN(iph->ip_tos),
                                                     ecn_vals, "Unknown ECN"));

      field_tree = proto_item_add_subtree(tf, ett_ip_dsfield);
      proto_tree_add_item(field_tree, hf_ip_dsfield_dscp, tvb, offset + 1, 1, ENC_NA);
      proto_tree_add_item(field_tree, hf_ip_dsfield_ecn, tvb, offset + 1, 1, ENC_NA);
    } else {
      tf = proto_tree_add_uint_format(ip_tree, hf_ip_tos, tvb, offset + 1, 1,
                                      iph->ip_tos,
                                      "Type of service: 0x%02x (%s)",
                                      iph->ip_tos,
                                      val_to_str_const(IPTOS_TOS(iph->ip_tos),
                                                       iptos_vals, "Unknown"));

      field_tree = proto_item_add_subtree(tf, ett_ip_tos);
      proto_tree_add_item(field_tree, hf_ip_tos_precedence, tvb, offset + 1, 1, ENC_NA);
      proto_tree_add_item(field_tree, hf_ip_tos_delay, tvb, offset + 1, 1, ENC_NA);
      proto_tree_add_item(field_tree, hf_ip_tos_throughput, tvb, offset + 1, 1, ENC_NA);
      proto_tree_add_item(field_tree, hf_ip_tos_reliability, tvb, offset + 1, 1, ENC_NA);
      proto_tree_add_item(field_tree, hf_ip_tos_cost, tvb, offset + 1, 1, ENC_NA);
    }
  }

  /* Length of IP datagram.
     XXX - what if this is greater than the reported length of the
     tvbuff?  This could happen, for example, in an IP datagram
     inside an ICMP datagram; we need to somehow let the
     dissector we call know that, as it might want to avoid
     doing its checksumming. */
  iph->ip_len = tvb_get_ntohs(tvb, offset + 2);

  /* Correct for zero-length TSO packets
   * If ip_len is zero, assume TSO and use the reported length instead.  Note
   * that we need to use the frame/reported length instead of the
   * actually-available length, just in case a snaplen was used on capture. */
  if (ip_tso_supported && !iph->ip_len)
    iph->ip_len = tvb_reported_length(tvb);

  if (iph->ip_len < hlen) {
    col_add_fstr(pinfo->cinfo, COL_INFO,
                 "Bogus IP length (%u, less than header length %u)",
                 iph->ip_len, hlen);
    if (tree) {
      if (!iph->ip_len) {
        tf = proto_tree_add_uint_format(ip_tree, hf_ip_len, tvb, offset + 2, 2,
          iph->ip_len,
          "Total length: 0 bytes (may be caused by \"TCP segmentation offload\" (TSO)?)");
      } else {
        tf = proto_tree_add_uint_format(ip_tree, hf_ip_len, tvb, offset + 2, 2,
          iph->ip_len,
          "Total length: %u bytes (bogus, less than header length %u)",
          iph->ip_len, hlen);
      }
      expert_add_info_format(pinfo, tf, PI_PROTOCOL, PI_ERROR, "Bogus IP length");
    }
    return;
  }

  /*
   * Now that we know that the total length of this IP datagram isn't
   * obviously bogus, adjust the length of this tvbuff to include only
   * the IP datagram.
   */
  set_actual_length(tvb, iph->ip_len);

  if (tree)
    proto_tree_add_uint(ip_tree, hf_ip_len, tvb, offset + 2, 2, iph->ip_len);

  iph->ip_id  = tvb_get_ntohs(tvb, offset + 4);
  if (tree)
    proto_tree_add_uint(ip_tree, hf_ip_id, tvb, offset + 4, 2, iph->ip_id);

  iph->ip_off = tvb_get_ntohs(tvb, offset + 6);
  if (tree) {
    int bit_offset = (offset + 6) * 8;

    flags = (iph->ip_off & (IP_RF | IP_DF | IP_MF)) >> IP_OFFSET_WIDTH;
    tf = proto_tree_add_uint(ip_tree, hf_ip_flags, tvb, offset + 6, 1, flags);
    field_tree = proto_item_add_subtree(tf, ett_ip_off);
    if (ip_security_flag) {
      proto_item *sf;

      sf = proto_tree_add_bits_item(field_tree, hf_ip_flags_sf, tvb,
                                    bit_offset + 0, 1, ENC_BIG_ENDIAN);
      if (iph->ip_off & IP_RF) {
        proto_item_append_text(tf, " (Evil packet!)");
        expert_add_info_format(pinfo, sf, PI_SECURITY, PI_WARN,
                               "This is an Evil packet (RFC 3514)");
      }
    } else {
      proto_tree_add_bits_item(field_tree, hf_ip_flags_rf, tvb, bit_offset + 0,
                               1, ENC_LITTLE_ENDIAN);
    }
    if (iph->ip_off & IP_DF)
      proto_item_append_text(tf, " (Don't Fragment)");
    proto_tree_add_bits_item(field_tree, hf_ip_flags_df, tvb, bit_offset + 1,
                             1, ENC_BIG_ENDIAN);
    if (iph->ip_off & IP_MF)
      proto_item_append_text(tf, " (More Fragments)");
    proto_tree_add_bits_item(field_tree, hf_ip_flags_mf, tvb, bit_offset + 2,
                             1, ENC_BIG_ENDIAN);
    proto_tree_add_uint(ip_tree, hf_ip_frag_offset, tvb, offset + 6, 2,
                        (iph->ip_off & IP_OFFSET)*8);
  }

  iph->ip_ttl = tvb_get_guint8(tvb, offset + 8);
  pinfo->ip_ttl = iph->ip_ttl;
  if (tree) {
    ttl_item = proto_tree_add_item(ip_tree, hf_ip_ttl, tvb, offset + 8, 1, ENC_BIG_ENDIAN);
  } else {
    ttl_item = NULL;
  }

  iph->ip_p = tvb_get_guint8(tvb, offset + 9);
  if (tree) {
    proto_tree_add_item(ip_tree, hf_ip_proto, tvb, offset + 9, 1, ENC_BIG_ENDIAN);
  }

  iph->ip_sum = tvb_get_ntohs(tvb, offset + 10);

  /*
   * If we have the entire IP header available, check the checksum.
   */
  if (ip_check_checksum && tvb_bytes_exist(tvb, offset, hlen)) {
    ipsum = ip_checksum(tvb_get_ptr(tvb, offset, hlen), hlen);
    if (tree) {
      if (ipsum == 0) {
        item = proto_tree_add_uint_format(ip_tree, hf_ip_checksum, tvb,
                                          offset + 10, 2, iph->ip_sum,
                                          "Header checksum: 0x%04x [correct]",
                                          iph->ip_sum);
        checksum_tree = proto_item_add_subtree(item, ett_ip_checksum);
        item = proto_tree_add_boolean(checksum_tree, hf_ip_checksum_good, tvb,
                                      offset + 10, 2, TRUE);
        PROTO_ITEM_SET_GENERATED(item);
        item = proto_tree_add_boolean(checksum_tree, hf_ip_checksum_bad, tvb,
                                      offset + 10, 2, FALSE);
        PROTO_ITEM_SET_GENERATED(item);
      } else {
        item = proto_tree_add_uint_format(ip_tree, hf_ip_checksum, tvb,
                                          offset + 10, 2, iph->ip_sum,
                                          "Header checksum: 0x%04x "
                                          "[incorrect, should be 0x%04x "
                                          "(may be caused by \"IP checksum "
                                          "offload\"?)]", iph->ip_sum,
                                          in_cksum_shouldbe(iph->ip_sum,
                                          ipsum));
        checksum_tree = proto_item_add_subtree(item, ett_ip_checksum);
        item = proto_tree_add_boolean(checksum_tree, hf_ip_checksum_good, tvb,
                                      offset + 10, 2, FALSE);
        PROTO_ITEM_SET_GENERATED(item);
        item = proto_tree_add_boolean(checksum_tree, hf_ip_checksum_bad, tvb,
                                      offset + 10, 2, TRUE);
        PROTO_ITEM_SET_GENERATED(item);
      }
    }
    if (ipsum != 0) {
      /* Add expert item always (so tap gets called if present);
         if (tree == NULL) then item will be NULL
         else item should be from the
         add_boolean(..., hf_ip_checksum_bad, ...) above */
      expert_add_info_format(pinfo, item, PI_CHECKSUM, PI_ERROR,
                             "Bad checksum");
    }
  } else {
    ipsum = 0;
    if (tree) {
      item = proto_tree_add_uint_format(ip_tree, hf_ip_checksum, tvb,
                                        offset + 10, 2, iph->ip_sum,
                                        "Header checksum: 0x%04x [%s]",
                                        iph->ip_sum, ip_check_checksum ?
                                        "not all data available" :
                                        "validation disabled");
      checksum_tree = proto_item_add_subtree(item, ett_ip_checksum);
      item = proto_tree_add_boolean(checksum_tree, hf_ip_checksum_good, tvb,
                                    offset + 10, 2, FALSE);
      PROTO_ITEM_SET_GENERATED(item);
      item = proto_tree_add_boolean(checksum_tree, hf_ip_checksum_bad, tvb,
                                    offset + 10, 2, FALSE);
      PROTO_ITEM_SET_GENERATED(item);
    }
  }
  src_addr = tvb_get_ptr(tvb, offset + IPH_SRC, 4);
  src32 = tvb_get_ntohl(tvb, offset + IPH_SRC);
  SET_ADDRESS(&pinfo->net_src, AT_IPv4, 4, src_addr);
  SET_ADDRESS(&pinfo->src, AT_IPv4, 4, src_addr);
  SET_ADDRESS(&iph->ip_src, AT_IPv4, 4, src_addr);
  if (tree) {
    const char *src_host;

    memcpy(&addr, iph->ip_src.data, 4);
    src_host = get_hostname(addr);
    if (ip_summary_in_tree) {
      proto_item_append_text(ti, ", Src: %s (%s)", src_host,
                             ip_to_str(iph->ip_src.data));
    }
    proto_tree_add_ipv4(ip_tree, hf_ip_src, tvb, offset + 12, 4, addr);
    item = proto_tree_add_ipv4(ip_tree, hf_ip_addr, tvb, offset + 12, 4, addr);
    PROTO_ITEM_SET_HIDDEN(item);
    item = proto_tree_add_string(ip_tree, hf_ip_src_host, tvb, offset + 12, 4,
                                 src_host);
    PROTO_ITEM_SET_GENERATED(item);
    PROTO_ITEM_SET_HIDDEN(item);
    item = proto_tree_add_string(ip_tree, hf_ip_host, tvb, offset + 12, 4,
                                 src_host);
    PROTO_ITEM_SET_GENERATED(item);
    PROTO_ITEM_SET_HIDDEN(item);
  }

  /* If there's an IP strict or loose source routing option, then the final
   * L3 IP destination address will be the last entry in the routing header
   * EXCEPT when the table is exhausted (pointer is greater than the length).
   * In this case, the final L3 IP destination address is the one in the L3
   * header. (REF: http://tools.ietf.org/html/rfc791#section-3.1)
   */
  if (hlen > IPH_MIN_LEN) {
    /* There's more than just the fixed-length header.  See if we've got
     * either a strict or loose source route option and if so, return the
     * offset into the tvb to where the real destination IP address is located.
     */
    dst_off = get_dst_offset(tvb, offset + 20, hlen - IPH_MIN_LEN, ipopts,
                             N_IP_OPTS, IPOPT_EOOL);
  }
  else
    dst_off = 0;

  dst_addr = tvb_get_ptr(tvb, offset + IPH_DST + dst_off, 4);
  dst32 = tvb_get_ntohl(tvb, offset + IPH_DST + dst_off);
  SET_ADDRESS(&pinfo->net_dst, AT_IPv4, 4, dst_addr);
  SET_ADDRESS(&pinfo->dst, AT_IPv4, 4, dst_addr);
  SET_ADDRESS(&iph->ip_dst, AT_IPv4, 4, dst_addr);

  /* If an IP is destined for an IP address in the Local Network Control Block
   * (e.g. 224.0.0.0/24), the packet should never be routed and the TTL would
   * be expected to be 1.  (see RFC 3171)  Flag a TTL greater than 1.
   *
   * Flag a low TTL if the packet is not destined for a multicast address
   * (e.g. 224.0.0.0/4) ... and the payload isn't protocol 103 (PIM).
   * (see http://tools.ietf.org/html/rfc3973#section-4.7).
   */
  if (is_a_local_network_control_block_addr(dst32)) {
    ttl = local_network_control_block_addr_valid_ttl(dst32);
    if (ttl != iph->ip_ttl && ttl != IPLOCAL_NETWRK_CTRL_BLK_ANY_TTL) {
      expert_add_info_format(pinfo, ttl_item, PI_SEQUENCE, PI_NOTE,
                             "\"Time To Live\" != %d for a packet sent to the "
                             "Local Network Control Block (see RFC 3171)",
                             ttl);
    }
  } else if (!is_a_multicast_addr(dst32) && iph->ip_ttl < 5 &&
            (iph->ip_p != IP_PROTO_PIM)) {
    expert_add_info_format(pinfo, ttl_item, PI_SEQUENCE, PI_NOTE,
                           "\"Time To Live\" only %u", iph->ip_ttl);
  }

  if (tree) {
    const char *dst_host;
    guint32 cur_rt;

    memcpy(&addr, iph->ip_dst.data, 4);
    dst_host = get_hostname(addr);
    if (dst_off)
      cur_rt = tvb_get_ipv4(tvb, offset + 16);
    if (ip_summary_in_tree) {
      proto_item_append_text(ti, ", Dst: %s (%s)", dst_host,
                             ip_to_str(iph->ip_dst.data));
      if (dst_off)
        proto_item_append_text(ti, ", Via: %s (%s)", get_hostname(cur_rt),
                               ip_to_str((gchar *)&cur_rt));
    }

    if (dst_off) {
      proto_tree_add_ipv4(ip_tree, hf_ip_cur_rt, tvb, offset + 16, 4, cur_rt);
      item = proto_tree_add_string(ip_tree, hf_ip_cur_rt_host, tvb,
                                   offset + 16, 4, get_hostname(cur_rt));
      PROTO_ITEM_SET_GENERATED(item);
      PROTO_ITEM_SET_HIDDEN(item);
    }
    else {
      proto_tree_add_ipv4(ip_tree, hf_ip_dst, tvb, offset + 16, 4, addr);
      item = proto_tree_add_ipv4(ip_tree, hf_ip_addr, tvb, offset + 16, 4,
                                 addr);
      PROTO_ITEM_SET_HIDDEN(item);
      item = proto_tree_add_string(ip_tree, hf_ip_dst_host, tvb, offset + 16,
                                   4, dst_host);
      PROTO_ITEM_SET_GENERATED(item);
      PROTO_ITEM_SET_HIDDEN(item);
      item = proto_tree_add_string(ip_tree, hf_ip_host, tvb,
                                   offset + 16 + dst_off, 4, dst_host);
      PROTO_ITEM_SET_GENERATED(item);
      PROTO_ITEM_SET_HIDDEN(item);
    }
  }

#ifdef HAVE_GEOIP
  if (tree && ip_use_geoip) {
    add_geoip_info(ip_tree, tvb, offset, src32, dst32);
  }
#endif

  /* Decode IP options, if any. */
  if (hlen > IPH_MIN_LEN) {
    /* There's more than just the fixed-length header.  Decode the options. */
    optlen = hlen - IPH_MIN_LEN;  /* length of options, in bytes */
    tf = proto_tree_add_text(ip_tree, tvb, offset + 20, optlen,
                             "Options: (%u bytes)", optlen);
    field_tree = proto_item_add_subtree(tf, ett_ip_options);
    dissect_ip_tcp_options(tvb, offset + 20, optlen, ipopts, N_IP_OPTS,
                           IPOPT_EOOL, pinfo, field_tree, tf);
  }

  pinfo->ipproto = iph->ip_p;
  pinfo->iplen = iph->ip_len;
  pinfo->iphdrlen = hlen;
  tap_queue_packet(ip_tap, pinfo, iph);

  /* Skip over header + options */
  offset += hlen;
  nxt = iph->ip_p;  /* XXX - what if this isn't the same for all fragments? */

  /* If ip_defragment is on, this is a fragment, we have all the data
   * in the fragment, and the header checksum is valid, then just add
   * the fragment to the hashtable.
   */
  save_fragmented = pinfo->fragmented;
  if (ip_defragment && (iph->ip_off & (IP_MF|IP_OFFSET)) &&
      tvb_bytes_exist(tvb, offset, pinfo->iplen - pinfo->iphdrlen) &&
      ipsum == 0) {
    ipfd_head = fragment_add_check(tvb, offset, pinfo,
                                   iph->ip_p ^ iph->ip_id ^ src32 ^ dst32,
                                   ip_fragment_table, ip_reassembled_table,
                                   (iph->ip_off & IP_OFFSET) * 8,
                                   pinfo->iplen - pinfo->iphdrlen,
                                   iph->ip_off & IP_MF);

    next_tvb = process_reassembled_data(tvb, offset, pinfo, "Reassembled IPv4",
                                        ipfd_head, &ip_frag_items,
                                        &update_col_info, ip_tree);
  } else {
    /* If this is the first fragment, dissect its contents, otherwise
       just show it as a fragment.

       XXX - if we eventually don't save the reassembled contents of all
       fragmented datagrams, we may want to always reassemble. */
    if (iph->ip_off & IP_OFFSET) {
      /* Not the first fragment - don't dissect it. */
      next_tvb = NULL;
    } else {
      /* First fragment, or not fragmented.  Dissect what we have here. */

      /* Get a tvbuff for the payload. */
      next_tvb = tvb_new_subset_remaining(tvb, offset);

      /*
       * If this is the first fragment, but not the only fragment,
       * tell the next protocol that.
       */
      if (iph->ip_off & IP_MF)
        pinfo->fragmented = TRUE;
      else
        pinfo->fragmented = FALSE;
    }
  }

  if (next_tvb == NULL) {
    /* Just show this as a fragment. */
    col_add_fstr(pinfo->cinfo, COL_INFO,
                 "Fragmented IP protocol (proto=%s %u, off=%u, ID=%04x)",
                 ipprotostr(iph->ip_p), iph->ip_p,
                 (iph->ip_off & IP_OFFSET) * 8, iph->ip_id);
    if ( ipfd_head && ipfd_head->reassembled_in != pinfo->fd->num ) {
      col_append_fstr(pinfo->cinfo, COL_INFO, " [Reassembled in #%u]",
                      ipfd_head->reassembled_in);
    }

    call_dissector(data_handle, tvb_new_subset_remaining(tvb, offset), pinfo,
                   parent_tree);
    pinfo->fragmented = save_fragmented;
    return;
  }

  /* XXX This is an ugly hack because I didn't manage to make the IPIP
   * dissector a heuristic one [JMayer]
   * The TAPA protocol also uses IP protocol number 4 but it isn't really
   * IPIP, so try to detect it first and call it explicitly before calling
   * the generic ip.proto dispatcher
   */
  if (nxt == IP_PROTO_IPIP && (tvb_get_guint8(next_tvb, 0) & 0xF0) != 0x40 &&
      tvb_get_ntohs(next_tvb, 2) < 20) {
    call_dissector(tapa_handle,next_tvb, pinfo, parent_tree);

    /* Hand off to the next protocol.

     XXX - setting the columns only after trying various dissectors means
     that if one of those dissectors throws an exception, the frame won't
     even be labeled as an IP frame; ideally, if a frame being dissected
     throws an exception, it'll be labeled as a mangled frame of the
     type in question. */
  } else if (!dissector_try_uint(ip_dissector_table, nxt, next_tvb, pinfo,
                                 parent_tree)) {
    /* Unknown protocol */
    if (update_col_info) {
      col_add_fstr(pinfo->cinfo, COL_INFO, "%s (%u)",
                   ipprotostr(iph->ip_p), iph->ip_p);
    }
    call_dissector(data_handle,next_tvb, pinfo, parent_tree);
  }
  pinfo->fragmented = save_fragmented;
}

static gboolean
dissect_ip_heur(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
    int length, tot_length;
    guint8 oct, version, ihl;
        
/*
    0                   1                   2                   3   
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |Version|  IHL  |Type of Service|          Total Length         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/
    length = tvb_length(tvb);
    if(length<4){
        /* Need at least 4 bytes to make some sort of decision */
        return FALSE;
    }
    oct = tvb_get_guint8(tvb,0);
    ihl = oct & 0x0f;
    version = oct >> 4;
    if(version == 6){
        /* TODO: Add IPv6 checks here */
/*
    3.  IPv6 Header Format

         0                   1                   2                   3   
         0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |Version| Traffic Class |           Flow Label                  |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |         Payload Length        |  Next Header  |   Hop Limit   |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                                                               |
        +                                                               +
        |                                                               |
        +                         Source Address                        +
        |                                                               |
        +                                                               +
        |                                                               |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                                                               |
        +                                                               +
        |                                                               |
        +                      Destination Address                      +
        |                                                               |
        +                                                               +
        |                                                               |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

        Version              4-bit Internet Protocol version number = 6.

        Traffic Class        8-bit traffic class field.  See section 7.

        Flow Label           20-bit flow label.  See section 6.

        Payload Length       16-bit unsigned integer.  Length of the IPv6
                             payload, i.e., the rest of the packet following
                             this IPv6 header, in octets.  (Note that any
                             extension headers [section 4] present are
                             considered part of the payload, i.e., included
                             in the length count.)


*/
        if(length<8){
            /* Need at least 8 bytes to make a decision */
            return FALSE;
        }
        tot_length = tvb_get_ntohs(tvb,4);
        if(tot_length != 40 + (int)tvb_reported_length(tvb)){
            return FALSE;
        }
        call_dissector(ipv6_handle, tvb, pinfo, tree);
        return TRUE;
    }
    /* version == IPv4 , the minimum value for a correct header is 5 */
    if((version != 4)|| (ihl < 5)){
        return FALSE;
    }
    tot_length = tvb_get_ntohs(tvb,2);

    if(tot_length != 8 + (int)tvb_reported_length(tvb)){
        return FALSE;
    }

    dissect_ip(tvb, pinfo, tree);
    return TRUE;
}

void
proto_register_ip(void)
{
#define ARG_TO_STR(ARG) #ARG
#define FLAGS_OFFSET_WIDTH_MSG(WIDTH) \
  "Flags (" ARG_TO_STR(WIDTH) " bits)"
#define FRAG_OFFSET_WIDTH_MSG(WIDTH) \
  "Fragment offset (" ARG_TO_STR(WIDTH) " bits)"

  static hf_register_info hf[] = {
    { &hf_ip_version,
      { "Version", "ip.version", FT_UINT8, BASE_DEC,
        NULL, 0x0, NULL, HFILL }},

    { &hf_ip_hdr_len,
      { "Header Length", "ip.hdr_len", FT_UINT8, BASE_DEC,
        NULL, 0x0, NULL, HFILL }},

    { &hf_ip_dsfield,
      { "Differentiated Services field", "ip.dsfield", FT_UINT8, BASE_DEC,
        NULL, 0x0, NULL, HFILL }},

    { &hf_ip_dsfield_dscp,
      { "Differentiated Services Codepoint", "ip.dsfield.dscp", FT_UINT8, BASE_HEX | BASE_EXT_STRING,
        &dscp_vals_ext, IPDSFIELD_DSCP_MASK, NULL, HFILL }},

    { &hf_ip_dsfield_ecn,
      { "Explicit Congestion Notification", "ip.dsfield.ecn", FT_UINT8, BASE_HEX,
        VALS(ecn_vals), IPDSFIELD_ECN_MASK, NULL, HFILL }},

    { &hf_ip_tos,
      { "Type of Service", "ip.tos", FT_UINT8, BASE_DEC,
        NULL, 0x0, NULL, HFILL }},

    { &hf_ip_tos_precedence,
      { "Precedence", "ip.tos.precedence", FT_UINT8, BASE_DEC,
        VALS(precedence_vals), IPTOS_PREC_MASK, NULL, HFILL }},

    { &hf_ip_tos_delay,
      { "Delay", "ip.tos.delay", FT_BOOLEAN, 8,
        TFS(&tos_set_low), IPTOS_LOWDELAY, NULL, HFILL }},

    { &hf_ip_tos_throughput,
      { "Throughput", "ip.tos.throughput", FT_BOOLEAN, 8,
        TFS(&tos_set_high), IPTOS_THROUGHPUT, NULL, HFILL }},

    { &hf_ip_tos_reliability,
      { "Reliability", "ip.tos.reliability", FT_BOOLEAN, 8,
        TFS(&tos_set_high), IPTOS_RELIABILITY, NULL, HFILL }},

    { &hf_ip_tos_cost,
      { "Cost", "ip.tos.cost", FT_BOOLEAN, 8,
        TFS(&tos_set_low), IPTOS_LOWCOST, NULL, HFILL }},

    { &hf_ip_len,
      { "Total Length", "ip.len", FT_UINT16, BASE_DEC,
        NULL, 0x0, NULL, HFILL }},

    { &hf_ip_id,
      { "Identification", "ip.id", FT_UINT16, BASE_HEX_DEC,
        NULL, 0x0, NULL, HFILL }},

    { &hf_ip_dst,
      { "Destination", "ip.dst", FT_IPv4, BASE_NONE,
        NULL, 0x0, NULL, HFILL }},

    { &hf_ip_dst_host,
      { "Destination Host", "ip.dst_host", FT_STRING, BASE_NONE,
        NULL, 0x0, NULL, HFILL }},

    { &hf_ip_src,
      { "Source", "ip.src", FT_IPv4, BASE_NONE,
        NULL, 0x0, NULL, HFILL }},

    { &hf_ip_src_host,
      { "Source Host", "ip.src_host", FT_STRING, BASE_NONE,
        NULL, 0x0, NULL, HFILL }},

    { &hf_ip_addr,
      { "Source or Destination Address", "ip.addr", FT_IPv4, BASE_NONE,
        NULL, 0x0, NULL, HFILL }},

    { &hf_ip_host,
      { "Source or Destination Host", "ip.host", FT_STRING, BASE_NONE,
        NULL, 0x0, NULL, HFILL }},

#ifdef HAVE_GEOIP
    { &hf_geoip_country,
      { "Source or Destination GeoIP Country", "ip.geoip.country",
        FT_STRING, BASE_NONE, NULL, 0x0, NULL, HFILL }},
    { &hf_geoip_city,
      { "Source or Destination GeoIP City", "ip.geoip.city",
        FT_STRING, BASE_NONE, NULL, 0x0, NULL, HFILL }},
    { &hf_geoip_org,
      { "Source or Destination GeoIP Organization", "ip.geoip.org",
        FT_STRING, BASE_NONE, NULL, 0x0, NULL, HFILL }},
    { &hf_geoip_isp,
      { "Source or Destination GeoIP ISP", "ip.geoip.isp",
        FT_STRING, BASE_NONE, NULL, 0x0, NULL, HFILL }},
    { &hf_geoip_asnum,
      { "Source or Destination GeoIP AS Number", "ip.geoip.asnum",
        FT_STRING, BASE_NONE, NULL, 0x0, NULL, HFILL }},
    { &hf_geoip_lat,
      { "Source or Destination GeoIP Latitude", "ip.geoip.lat",
        FT_DOUBLE, BASE_NONE, NULL, 0x0, NULL, HFILL }},
    { &hf_geoip_lon,
      { "Source or Destination GeoIP Longitude", "ip.geoip.lon",
        FT_DOUBLE, BASE_NONE, NULL, 0x0, NULL, HFILL }},
    { &hf_geoip_src_country,
      { "Source GeoIP Country", "ip.geoip.src_country",
        FT_STRING, BASE_NONE, NULL, 0x0, NULL, HFILL }},
    { &hf_geoip_src_city,
      { "Source GeoIP City", "ip.geoip.src_city",
        FT_STRING, BASE_NONE, NULL, 0x0, NULL, HFILL }},
    { &hf_geoip_src_org,
      { "Source GeoIP Organization", "ip.geoip.src_org",
        FT_STRING, BASE_NONE, NULL, 0x0, NULL, HFILL }},
    { &hf_geoip_src_isp,
      { "Source GeoIP ISP", "ip.geoip.src_isp",
        FT_STRING, BASE_NONE, NULL, 0x0, NULL, HFILL }},
    { &hf_geoip_src_asnum,
      { "Source GeoIP AS Number", "ip.geoip.src_asnum",
        FT_STRING, BASE_NONE, NULL, 0x0, NULL, HFILL }},
    { &hf_geoip_src_lat,
      { "Source GeoIP Latitude", "ip.geoip.src_lat",
        FT_DOUBLE, BASE_NONE, NULL, 0x0, NULL, HFILL }},
    { &hf_geoip_src_lon,
      { "Source GeoIP Longitude", "ip.geoip.src_lon",
        FT_DOUBLE, BASE_NONE, NULL, 0x0, NULL, HFILL }},
    { &hf_geoip_dst_country,
      { "Destination GeoIP Country", "ip.geoip.dst_country",
        FT_STRING, BASE_NONE, NULL, 0x0, NULL, HFILL }},
    { &hf_geoip_dst_city,
      { "Destination GeoIP City", "ip.geoip.dst_city",
        FT_STRING, BASE_NONE, NULL, 0x0, NULL, HFILL }},
    { &hf_geoip_dst_org,
      { "Destination GeoIP Organization", "ip.geoip.dst_org",
        FT_STRING, BASE_NONE, NULL, 0x0, NULL, HFILL }},
    { &hf_geoip_dst_isp,
      { "Destination GeoIP ISP", "ip.geoip.dst_isp",
        FT_STRING, BASE_NONE, NULL, 0x0, NULL, HFILL }},
    { &hf_geoip_dst_asnum,
      { "Destination GeoIP AS Number", "ip.geoip.dst_asnum",
        FT_STRING, BASE_NONE, NULL, 0x0, NULL, HFILL }},
    { &hf_geoip_dst_lat,
      { "Destination GeoIP Latitude", "ip.geoip.dst_lat",
        FT_DOUBLE, BASE_NONE, NULL, 0x0, NULL, HFILL }},
    { &hf_geoip_dst_lon,
      { "Destination GeoIP Longitude", "ip.geoip.dst_lon",
        FT_DOUBLE, BASE_NONE, NULL, 0x0, NULL, HFILL }},
#endif /* HAVE_GEOIP */

    { &hf_ip_flags,
      { "Flags", "ip.flags", FT_UINT8, BASE_HEX,
        NULL, 0x0, FLAGS_OFFSET_WIDTH_MSG(IP_FLAGS_WIDTH), HFILL }},

    { &hf_ip_flags_sf,
      { "Security flag", "ip.flags.sf", FT_BOOLEAN, BASE_NONE,
        TFS(&flags_sf_set_evil), 0x0, "Security flag (RFC 3514)", HFILL }},

    { &hf_ip_flags_rf,
      { "Reserved bit", "ip.flags.rb", FT_BOOLEAN, BASE_NONE,
        TFS(&tfs_set_notset), 0x0, NULL, HFILL }},

    { &hf_ip_flags_df,
      { "Don't fragment", "ip.flags.df", FT_BOOLEAN, BASE_NONE,
        TFS(&tfs_set_notset), 0x0, NULL, HFILL }},

    { &hf_ip_flags_mf,
      { "More fragments", "ip.flags.mf", FT_BOOLEAN, BASE_NONE,
        TFS(&tfs_set_notset), 0x0, NULL, HFILL }},

    { &hf_ip_frag_offset,
      { "Fragment offset", "ip.frag_offset", FT_UINT16, BASE_DEC,
        NULL, 0x0, FRAG_OFFSET_WIDTH_MSG(IP_OFFSET_WIDTH), HFILL }},

    { &hf_ip_ttl,
      { "Time to live", "ip.ttl", FT_UINT8, BASE_DEC,
        NULL, 0x0, NULL, HFILL }},

    { &hf_ip_proto,
      { "Protocol", "ip.proto", FT_UINT8, BASE_DEC | BASE_EXT_STRING,
        &ipproto_val_ext, 0x0, NULL, HFILL }},

    { &hf_ip_checksum,
      { "Header checksum", "ip.checksum", FT_UINT16, BASE_HEX,
        NULL, 0x0, NULL, HFILL }},

    { &hf_ip_checksum_good,
      { "Good", "ip.checksum_good", FT_BOOLEAN, BASE_NONE,  NULL, 0x0,
        "True: checksum matches packet content; False: doesn't match content or not checked", HFILL }},

    { &hf_ip_checksum_bad,
      { "Bad", "ip.checksum_bad", FT_BOOLEAN, BASE_NONE, NULL, 0x0,
        "True: checksum doesn't match packet content; False: matches content or not checked", HFILL }},

    /* IP options related fields */
    { &hf_ip_opt_type,
      { "Type", "ip.opt.type", FT_UINT8, BASE_DEC,
        NULL, 0x0, NULL, HFILL }},

    { &hf_ip_opt_type_copy,
      { "Copy on fragmentation", "ip.opt.type.copy", FT_BOOLEAN, 8,
        TFS(&tfs_yes_no), IPOPT_COPY_MASK, NULL, HFILL }},

    { &hf_ip_opt_type_class,
      { "Class", "ip.opt.type.class", FT_UINT8, BASE_DEC,
        VALS(ipopt_type_class_vals), IPOPT_CLASS_MASK, NULL, HFILL }},

    { &hf_ip_opt_type_number,
      { "Number", "ip.opt.type.number", FT_UINT8, BASE_DEC,
        VALS(ipopt_type_number_vals), IPOPT_NUMBER_MASK, NULL, HFILL }},

    { &hf_ip_opt_len,
      { "Length", "ip.opt.len", FT_UINT8, BASE_DEC,
        NULL, 0x0, NULL, HFILL }},

    { &hf_ip_opt_ptr,
      { "Pointer", "ip.opt.ptr", FT_UINT8, BASE_DEC,
        NULL, 0x0, NULL, HFILL }},

    { &hf_ip_opt_sid,
      { "Stream Identifier", "ip.opt.sid", FT_UINT16, BASE_DEC,
        NULL, 0x0, "SATNET stream identifier", HFILL }},

    { &hf_ip_opt_mtu,
      { "MTU", "ip.opt.mtu", FT_UINT16, BASE_DEC,
        NULL, 0x0, NULL, HFILL }},

    { &hf_ip_opt_id_number,
      { "ID Number", "ip.opt.id_number", FT_UINT16, BASE_DEC,
        NULL, 0x0, NULL, HFILL }},

    { &hf_ip_opt_ohc,
      { "Outbound Hop Count", "ip.opt.ohc", FT_UINT16, BASE_DEC,
        NULL, 0x0, NULL, HFILL }},

    { &hf_ip_opt_rhc,
      { "Return Hop Count", "ip.opt.rhc", FT_UINT16, BASE_DEC,
        NULL, 0x0, NULL, HFILL }},

    { &hf_ip_opt_originator,
      { "Originator IP Address", "ip.opt.originator", FT_IPv4, BASE_NONE,
        NULL, 0x0, NULL, HFILL }},

    { &hf_ip_opt_ra,
      { "Router Alert", "ip.opt.ra", FT_UINT16, BASE_DEC | BASE_RANGE_STRING,
        RVALS(ra_rvals), 0x0, NULL, HFILL }},

    { &hf_ip_opt_addr,
      { "IP Address", "ip.opt.addr", FT_IPv4, BASE_NONE,
        NULL, 0x0, NULL, HFILL }},

    { &hf_ip_opt_padding,
      { "Padding", "ip.opt.padding", FT_BYTES, BASE_NONE,
        NULL, 0x0, NULL, HFILL }},

    { &hf_ip_opt_qs_func,
      { "Function", "ip.opt.qs_func", FT_UINT8, BASE_DEC,
        VALS(qs_func_vals), QS_FUNC_MASK, NULL, HFILL }},

    { &hf_ip_opt_qs_rate,
      { "Rate", "ip.opt.qs_rate", FT_UINT8, BASE_DEC | BASE_EXT_STRING,
        &qs_rate_vals_ext, QS_RATE_MASK, NULL, HFILL }},

    { &hf_ip_opt_qs_ttl,
      { "QS TTL", "ip.opt.qs_ttl", FT_UINT8, BASE_DEC,
        NULL, 0x0, NULL, HFILL }},

    { &hf_ip_opt_qs_ttl_diff,
      { "TTL Diff", "ip.opt.qs_ttl_diff", FT_UINT8, BASE_DEC,
        NULL, 0x0, NULL, HFILL }},

    { &hf_ip_opt_qs_unused,
      { "Not Used", "ip.opt.qs_unused", FT_UINT8, BASE_DEC,
        NULL, 0x0, NULL, HFILL }},

    { &hf_ip_opt_qs_nonce,
      { "QS Nonce", "ip.opt.qs_nonce", FT_UINT32, BASE_HEX,
        NULL, 0xFFFFFFFC, NULL, HFILL }},

    { &hf_ip_opt_qs_reserved,
      { "Reserved", "ip.opt.qs_reserved", FT_UINT32, BASE_HEX,
        NULL, 0x00000003, NULL, HFILL }},

    { &hf_ip_opt_sec_rfc791_sec,
      { "Security", "ip.opt.sec_rfc791_sec", FT_UINT8, BASE_HEX,
        VALS(secl_rfc791_vals), 0x0, NULL, HFILL }},

    { &hf_ip_opt_sec_rfc791_comp,
      { "Compartments", "ip.opt.sec_rfc791_comp", FT_UINT16, BASE_DEC,
        NULL, 0x0, NULL, HFILL }},

    { &hf_ip_opt_sec_rfc791_hr,
      { "Handling Restrictions", "ip.opt.sec_rfc791_hr", FT_STRING, BASE_NONE,
        NULL, 0x0, NULL, HFILL }},

    { &hf_ip_opt_sec_rfc791_tcc,
      { "Transmission Control Code", "ip.opt.sec_rfc791_tcc", FT_STRING, BASE_NONE,
        NULL, 0x0, NULL, HFILL }},

    { &hf_ip_opt_sec_cl,
      { "Classification Level", "ip.opt.sec_cl", FT_UINT8, BASE_HEX,
        VALS(sec_cl_vals), 0x0, NULL, HFILL }},

    { &hf_ip_opt_sec_prot_auth_flags,
      { "Protection Authority Flags", "ip.opt.sec_prot_auth_flags", FT_UINT8, BASE_HEX,
        NULL, 0x0, NULL, HFILL }},

    { &hf_ip_opt_sec_prot_auth_genser,
      { "GENSER", "ip.opt.sec_prot_auth_genser", FT_BOOLEAN, 8,
        TFS(&ip_opt_sec_prot_auth_flag_tfs), 0x80, NULL, HFILL }},

    { &hf_ip_opt_sec_prot_auth_siop_esi,
      { "SIOP-ESI", "ip.opt.sec_prot_auth_siop_esi", FT_BOOLEAN, 8,
        TFS(&ip_opt_sec_prot_auth_flag_tfs), 0x40, NULL, HFILL }},

    { &hf_ip_opt_sec_prot_auth_sci,
      { "SCI", "ip.opt.sec_prot_auth_sci", FT_BOOLEAN, 8,
        TFS(&ip_opt_sec_prot_auth_flag_tfs), 0x20, NULL, HFILL }},

    { &hf_ip_opt_sec_prot_auth_nsa,
      { "NSA", "ip.opt.sec_prot_auth_nsa", FT_BOOLEAN, 8,
        TFS(&ip_opt_sec_prot_auth_flag_tfs), 0x10, NULL, HFILL }},

    { &hf_ip_opt_sec_prot_auth_doe,
      { "DOE", "ip.opt.sec_prot_auth_doe", FT_BOOLEAN, 8,
        TFS(&ip_opt_sec_prot_auth_flag_tfs), 0x08, NULL, HFILL }},

    { &hf_ip_opt_sec_prot_auth_unassigned,
      { "Unassigned", "ip.opt.sec_prot_auth_unassigned", FT_UINT8, BASE_HEX,
        NULL, 0x06, NULL, HFILL }},

    { &hf_ip_opt_sec_prot_auth_unassigned2,
      { "Unassigned", "ip.opt.sec_prot_auth_unassigned", FT_UINT8, BASE_HEX,
        NULL, 0xFE, NULL, HFILL }},

    { &hf_ip_opt_sec_prot_auth_fti,
      { "Field Termination Indicator", "ip.opt.sec_prot_auth_fti", FT_BOOLEAN, 8,
        TFS(&ip_opt_sec_prot_auth_fti_tfs), 0x01, NULL, HFILL }},

    { &hf_ip_opt_ext_sec_add_sec_info_format_code,
      { "Additional Security Info Format Code", "ip.opt.ext_sec_add_sec_info_format_code", FT_UINT8, BASE_HEX,
        NULL, 0x0, NULL, HFILL }},

    { &hf_ip_opt_ext_sec_add_sec_info,
      { "Additional Security Info", "ip.opt.ext_sec_add_sec_info", FT_BYTES, BASE_NONE,
        NULL, 0x0, NULL, HFILL }},

    { &hf_ip_rec_rt,
      { "Recorded Route", "ip.rec_rt", FT_IPv4, BASE_NONE, NULL, 0x0,
        NULL, HFILL }},

    { &hf_ip_rec_rt_host,
      { "Recorded Route Host", "ip.rec_rt_host", FT_STRING, BASE_NONE,
        NULL, 0x0, NULL, HFILL }},

    { &hf_ip_cur_rt,
      { "Current Route", "ip.cur_rt", FT_IPv4, BASE_NONE, NULL, 0x0,
        NULL, HFILL }},

    { &hf_ip_cur_rt_host,
      { "Current Route Host", "ip.cur_rt_host", FT_STRING, BASE_NONE,
        NULL, 0x0, NULL, HFILL }},

    { &hf_ip_src_rt,
      { "Source Route", "ip.src_rt", FT_IPv4, BASE_NONE, NULL, 0x0,
        NULL, HFILL }},

    { &hf_ip_src_rt_host,
      { "Source Route Host", "ip.src_rt_host", FT_STRING, BASE_NONE,
        NULL, 0x0, NULL, HFILL }},

    { &hf_ip_empty_rt,
      { "Empty Route", "ip.empty_rt", FT_IPv4, BASE_NONE, NULL, 0x0,
        NULL, HFILL }},

    { &hf_ip_empty_rt_host,
      { "Empty Route Host", "ip.empty_rt_host", FT_STRING, BASE_NONE,
        NULL, 0x0, NULL, HFILL }},

    { &hf_ip_fragment_overlap,
      { "Fragment overlap", "ip.fragment.overlap", FT_BOOLEAN, BASE_NONE,
        NULL, 0x0, "Fragment overlaps with other fragments", HFILL }},

    { &hf_ip_fragment_overlap_conflict,
      { "Conflicting data in fragment overlap", "ip.fragment.overlap.conflict",
        FT_BOOLEAN, BASE_NONE, NULL, 0x0,
        "Overlapping fragments contained conflicting data", HFILL }},

    { &hf_ip_fragment_multiple_tails,
      { "Multiple tail fragments found", "ip.fragment.multipletails",
        FT_BOOLEAN, BASE_NONE, NULL, 0x0,
        "Several tails were found when defragmenting the packet", HFILL }},

    { &hf_ip_fragment_too_long_fragment,
      { "Fragment too long", "ip.fragment.toolongfragment",
        FT_BOOLEAN, BASE_NONE, NULL, 0x0,
        "Fragment contained data past end of packet", HFILL }},

    { &hf_ip_fragment_error,
      { "Defragmentation error", "ip.fragment.error", FT_FRAMENUM, BASE_NONE,
        NULL, 0x0, "Defragmentation error due to illegal fragments", HFILL }},

    { &hf_ip_fragment_count,
      { "Fragment count", "ip.fragment.count", FT_UINT32, BASE_DEC,
        NULL, 0x0, NULL, HFILL }},

    { &hf_ip_fragment,
      { "IPv4 Fragment", "ip.fragment", FT_FRAMENUM, BASE_NONE,
        NULL, 0x0, NULL, HFILL }},

    { &hf_ip_fragments,
      { "IPv4 Fragments", "ip.fragments", FT_BYTES, BASE_NONE,
        NULL, 0x0, NULL, HFILL }},

    { &hf_ip_reassembled_in,
      { "Reassembled IPv4 in frame", "ip.reassembled_in", FT_FRAMENUM, BASE_NONE,
      NULL, 0x0, "This IPv4 packet is reassembled in this frame", HFILL }},

    { &hf_ip_reassembled_length,
      { "Reassembled IPv4 length", "ip.reassembled.length", FT_UINT32, BASE_DEC,
        NULL, 0x0, "The total length of the reassembled payload", HFILL }},

    { &hf_ip_reassembled_data,
      { "Reassembled IPv4 data", "ip.reassembled.data", FT_BYTES, BASE_NONE,
        NULL, 0x0, "The reassembled payload", HFILL }}
};

  static gint *ett[] = {
    &ett_ip,
    &ett_ip_dsfield,
    &ett_ip_tos,
    &ett_ip_off,
    &ett_ip_options,
    &ett_ip_option_eool,
    &ett_ip_option_nop,
    &ett_ip_option_sec,
    &ett_ip_option_route,
    &ett_ip_option_timestamp,
    &ett_ip_option_ext_security,
    &ett_ip_option_cipso,
    &ett_ip_option_sid,
    &ett_ip_option_mtu,
    &ett_ip_option_tr,
    &ett_ip_option_ra,
    &ett_ip_option_sdb,
    &ett_ip_option_qs,
    &ett_ip_option_other,
    &ett_ip_fragments,
    &ett_ip_fragment,
    &ett_ip_checksum,
    &ett_ip_opt_type,
    &ett_ip_opt_sec_prot_auth_flags,
#ifdef HAVE_GEOIP
    &ett_geoip_info
#endif
  };
  module_t *ip_module;

  proto_ip = proto_register_protocol("Internet Protocol Version 4", "IPv4", "ip");
  proto_register_field_array(proto_ip, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));

  /* subdissector code */
  ip_dissector_table = register_dissector_table("ip.proto", "IPv4 protocol",
                                                FT_UINT8, BASE_DEC);

  /* Register configuration options */
  ip_module = prefs_register_protocol(proto_ip, NULL);
  prefs_register_bool_preference(ip_module, "decode_tos_as_diffserv",
    "Decode IPv4 TOS field as DiffServ field",
    "Whether the IPv4 type-of-service field should be decoded as a "
    "Differentiated Services field (see RFC2474/RFC2475)", &g_ip_dscp_actif);
  prefs_register_bool_preference(ip_module, "defragment",
    "Reassemble fragmented IPv4 datagrams",
    "Whether fragmented IPv4 datagrams should be reassembled", &ip_defragment);
  prefs_register_bool_preference(ip_module, "summary_in_tree",
    "Show IPv4 summary in protocol tree",
    "Whether the IPv4 summary line should be shown in the protocol tree",
    &ip_summary_in_tree);
  prefs_register_bool_preference(ip_module, "check_checksum",
  "Validate the IPv4 checksum if possible",
  "Whether to validate the IPv4 checksum", &ip_check_checksum);
  prefs_register_bool_preference(ip_module, "tso_support",
    "Support packet-capture from IP TSO-enabled hardware",
    "Whether to correct for TSO-enabled (TCP segmentation offload) hardware "
    "captures, such as spoofing the IP packet length", &ip_tso_supported);
#ifdef HAVE_GEOIP
  prefs_register_bool_preference(ip_module, "use_geoip",
    "Enable GeoIP lookups",
    "Whether to look up IP addresses in each GeoIP database we have loaded",
    &ip_use_geoip);
#endif /* HAVE_GEOIP */
  prefs_register_bool_preference(ip_module, "security_flag" ,
    "Interpret Reserved flag as Security flag (RFC 3514)",
    "Whether to interpret the originally reserved flag as security flag",
    &ip_security_flag);

  register_dissector("ip", dissect_ip, proto_ip);
  register_init_routine(ip_defragment_init);
  ip_tap = register_tap("ip");
}

void
proto_reg_handoff_ip(void)
{
  dissector_handle_t ip_handle;

  ip_handle = find_dissector("ip");
  ipv6_handle = find_dissector("ipv6");
  tapa_handle = find_dissector("tapa");
  data_handle = find_dissector("data");

  dissector_add_uint("ethertype", ETHERTYPE_IP, ip_handle);
  dissector_add_uint("ppp.protocol", PPP_IP, ip_handle);
  dissector_add_uint("ppp.protocol", ETHERTYPE_IP, ip_handle);
  dissector_add_uint("gre.proto", ETHERTYPE_IP, ip_handle);
  dissector_add_uint("gre.proto", GRE_WCCP, ip_handle);
  dissector_add_uint("llc.dsap", SAP_IP, ip_handle);
  dissector_add_uint("ip.proto", IP_PROTO_IPIP, ip_handle);
  dissector_add_uint("null.type", BSD_AF_INET, ip_handle);
  dissector_add_uint("chdlctype", ETHERTYPE_IP, ip_handle);
  dissector_add_uint("osinl.excl", NLPID_IP, ip_handle);
  dissector_add_uint("fr.ietf", NLPID_IP, ip_handle);
  dissector_add_uint("x.25.spi", NLPID_IP, ip_handle);
  dissector_add_uint("arcnet.protocol_id", ARCNET_PROTO_IP_1051, ip_handle);
  dissector_add_uint("arcnet.protocol_id", ARCNET_PROTO_IP_1201, ip_handle);
  dissector_add_uint("ax25.pid", AX25_P_IP, ip_handle);
  dissector_add_handle("udp.port", ip_handle);

  heur_dissector_add("tipc", dissect_ip_heur, proto_ip);
}

/*
 * Editor modelines  -  http://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 2
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vi: set shiftwidth=2 tabstop=8 expandtab:
 * :indentSize=2:tabSize=8:noTabs=true:
 */

