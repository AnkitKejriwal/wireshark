/* packet-icmpv6.c
 * Routines for ICMPv6 packet disassembly
 *
 * $Id$
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * MobileIPv6 support added by Tomislav Borosa <tomislav.borosa@siemens.hr>
 * Copyright 2006, Nicolas DICHTEL - 6WIND - <nicolas.dichtel@6wind.com>
 *
 * HMIPv6 support added by Martti Kuparinen <martti.kuparinen@iki.fi>
 *
 * FMIPv6 support added by Martin Andre <andre@clarinet.u-strasbg.fr>
 *
 * RPL support added by Colin O'Flynn & Owen Kirby.
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

#include <string.h>

#include <glib.h>

#include <epan/packet.h>
#include <epan/in_cksum.h>
#include <epan/addr_resolv.h>
#include <epan/ipproto.h>
#include <epan/asn1.h>
#include <epan/strutil.h>
#include <epan/expert.h>

#include "packet-ber.h"
#include "packet-ipv6.h"
#include "packet-dns.h"
#include "packet-x509af.h"
#include "packet-x509if.h"


#ifndef offsetof
#define offsetof(type, member)  ((size_t)(&((type *)0)->member))
#endif

/*
 * See, under http://www.ietf.org/internet-drafts/
 *
 *      draft-ietf-mobileip-ipv6-15.txt
 *
 * and
 *
 *      draft-ietf-ipngwg-icmp-name-lookups-08.txt
 *
 * and
 *
 *      draft-ietf-mobileip-hmipv6-05.txt
 *
 * and
 *
 *      draft-ieft-roll-rpl-12.txt
 *
 * and
 *
 *      rfc4068.txt
 */

static int proto_icmpv6 = -1;
static int hf_icmpv6_type = -1;
static int hf_icmpv6_code = -1;
static int hf_icmpv6_checksum = -1;
static int hf_icmpv6_checksum_bad = -1;
static int hf_icmpv6_nor = -1;
static int hf_icmpv6_haad_ha_addrs = -1;
static int hf_icmpv6_ra_cur_hop_limit = -1;
static int hf_icmpv6_ra_router_lifetime = -1;
static int hf_icmpv6_ra_reachable_time = -1;
static int hf_icmpv6_ra_retrans_timer = -1;

/* RPL: draft-ietf-roll-rpl-12.txt: Routing over Low-Power and Lossy Networks. */
static int hf_icmpv6_dis_reserved = -1;
static int hf_icmpv6_dio_grounded = -1;
static int hf_icmpv6_dio_preference = -1;
static int hf_icmpv6_dio_rank = -1;
static int hf_icmpv6_dio_instance = -1;
static int hf_icmpv6_dio_version = -1;
static int hf_icmpv6_dio_zero = -1;
static int hf_icmpv6_dio_trigger_seqnum = -1;
static int hf_icmpv6_dio_dagid = -1;
static int hf_icmpv6_dio_mop = -1;
static int hf_icmpv6_dao_instance = -1;
static int hf_icmpv6_dao_seqnum = -1;
static int hf_icmpv6_dao_reserved = -1;
static int hf_icmpv6_dao_flag_k = -1;
static int hf_icmpv6_dao_flag_d = -1;
static int hf_icmpv6_dao_flag_rsv = -1;
static int hf_icmpv6_dao_dodagid = -1;
static int hf_icmpv6_daoack_instance = -1;
static int hf_icmpv6_daoack_seqnum = -1;
static int hf_icmpv6_daoack_status = -1;
static int hf_icmpv6_daoack_flag_d = -1;
static int hf_icmpv6_daoack_flag_rsv = -1;
static int hf_icmpv6_daoack_dodagid = -1;
static int hf_icmpv6_rpl_opt_route = -1;
static int hf_icmpv6_rpl_opt_route_pref = -1;
static int hf_icmpv6_rpl_opt_route_lifetime = -1;
static int hf_icmpv6_rpl_opt_route_length = -1;
static int hf_icmpv6_rpl_opt_config_auth = -1;
static int hf_icmpv6_rpl_opt_config_pcs = -1;
static int hf_icmpv6_rpl_opt_config_doublings = -1;
static int hf_icmpv6_rpl_opt_config_min_interval = -1;
static int hf_icmpv6_rpl_opt_config_redundancy = -1;
static int hf_icmpv6_rpl_opt_config_rank_incr = -1;
static int hf_icmpv6_rpl_opt_config_hop_rank_inc = -1;
static int hf_icmpv6_rpl_opt_config_ocp = -1;
static int hf_icmpv6_rpl_opt_config_rsv = -1;
static int hf_icmpv6_rpl_opt_config_def_lifetime = -1;
static int hf_icmpv6_rpl_opt_config_lifetime_unit = -1;
static int hf_icmpv6_rpl_opt_target = -1;
static int hf_icmpv6_rpl_opt_target_length = -1;
static int hf_icmpv6_rpl_opt_target_reserved = -1;
static int hf_icmpv6_rpl_opt_transit_pathseq = -1;
static int hf_icmpv6_rpl_opt_transit_pathctl = -1;
static int hf_icmpv6_rpl_opt_transit_pathlifetime = -1;
static int hf_icmpv6_rpl_opt_transit_parent = -1;
static int hf_icmpv6_rpl_opt_transit_e = -1;
static int hf_icmpv6_rpl_opt_transit_flags = -1;
static int hf_icmpv6_rpl_opt_solicited_instance = -1;
static int hf_icmpv6_rpl_opt_solicited_flag_v = -1;
static int hf_icmpv6_rpl_opt_solicited_flag_i = -1;
static int hf_icmpv6_rpl_opt_solicited_flag_d = -1;
static int hf_icmpv6_rpl_opt_solicited_dodagid = -1;
static int hf_icmpv6_rpl_opt_solicited_version = -1;
static int hf_icmpv6_rpl_opt_prefix = -1;
static int hf_icmpv6_rpl_opt_prefix_flag_l = -1;
static int hf_icmpv6_rpl_opt_prefix_flag_a = -1;
static int hf_icmpv6_rpl_opt_prefix_flag_r = -1;
static int hf_icmpv6_rpl_opt_prefix_vlifetime = -1;
static int hf_icmpv6_rpl_opt_prefix_plifetime = -1;
static int hf_icmpv6_rpl_opt_prefix_length = -1;
static int hf_icmpv6_rpl_opt_targetdesc = -1;

static int hf_icmpv6_option = -1;
static int hf_icmpv6_option_type = -1;
static int hf_icmpv6_option_length = -1;
static int hf_icmpv6_opt_cga_pad_len = -1;
static int hf_icmpv6_opt_cga = -1;
static int hf_icmpv6_opt_cga_modifier = -1;
static int hf_icmpv6_opt_cga_subnet_prefix = -1;
static int hf_icmpv6_opt_cga_count = -1;
static int hf_icmpv6_opt_cga_ext_type = -1;
static int hf_icmpv6_opt_cga_ext_length = -1;
static int hf_icmpv6_opt_rsa_key_hash = -1;
static int hf_icmpv6_opt_name_type = -1;
static int hf_icmpv6_opt_name_x501 = -1;
static int hf_icmpv6_opt_name_fqdn = -1;
static int hf_icmpv6_opt_cert_type = -1;
static int hf_icmpv6_identifier = -1;
static int hf_icmpv6_all_comp = -1;
static int hf_icmpv6_comp = -1;
static int hf_icmpv6_x509if_Name = -1;
static int hf_icmpv6_x509af_Certificate = -1;
static int hf_icmpv6_recursive_dns_serv = -1;
static int hf_icmpv6_opt_prefix_len = -1;
static int hf_icmpv6_opt_flag_l = -1;
static int hf_icmpv6_opt_flag_a = -1;
static int hf_icmpv6_opt_flag_reserved = -1;

static int hf_icmpv6_opt_aro_lifetime = -1;
static int hf_icmpv6_opt_aro_eui64 = -1;
static int hf_icmpv6_opt_aro_addr_reg = -1;
static int hf_icmpv6_opt_6co_lifetime = -1;
static int hf_icmpv6_opt_6co_context_length = -1;
static int hf_icmpv6_opt_abro_addr = -1;
static int hf_icmpv6_opt_abro_version = -1;

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
static gint ett_multicastRR = -1;
static gint ett_icmpv6opt_name = -1;
static gint ett_cga_param_name = -1;
static gint ett_dao_rr_stack = -1;

static dissector_handle_t ipv6_handle;
static dissector_handle_t data_handle;

static const value_string icmpv6_type_str[] = {
    { ICMP6_DST_UNREACH,           "Unreachable" },
    { ICMP6_PACKET_TOO_BIG,        "Too big" },
    { ICMP6_TIME_EXCEEDED,         "Time exceeded" },
    { ICMP6_PARAM_PROB,            "Parameter problem" },
    { 100,                         "Private experimentation" },
    { 101,                         "Private experimentation" },
    { 127,                         "Reserved for expansion of ICMPv6 error messages" },
    { ICMP6_ECHO_REQUEST,          "Echo (ping) request" },
    { ICMP6_ECHO_REPLY,            "Echo (ping) reply" },
    { ICMP6_MEMBERSHIP_QUERY,      "Multicast listener query" },
    { ICMP6_MEMBERSHIP_REPORT,     "Multicast listener report" },
    { ICMP6_MEMBERSHIP_REDUCTION,  "Multicast listener done" },
    { ND_ROUTER_SOLICIT,           "Router solicitation" },
    { ND_ROUTER_ADVERT,            "Router advertisement" },
    { ND_NEIGHBOR_SOLICIT,         "Neighbor solicitation" },
    { ND_NEIGHBOR_ADVERT,          "Neighbor advertisement" },
    { ND_REDIRECT,                 "Redirect" },
    { ICMP6_ROUTER_RENUMBERING,    "Router renumbering" },
    { ICMP6_NI_QUERY,              "Node information query" },
    { ICMP6_NI_REPLY,              "Node information reply" },
    { ICMP6_IND_SOLICIT,           "Inverse neighbor discovery solicitation" },
    { ICMP6_IND_ADVERT,            "Inverse neighbor discovery advertisement" },
    { ICMP6_MLDV2_REPORT,          "Multicast Listener Report Message v2" },
    { ICMP6_MIP6_DHAAD_REQUEST,    "Dynamic Home Agent Address Discovery Request" },
    { ICMP6_MIP6_DHAAD_REPLY,      "Dynamic Home Agent Address Discovery Reply" },
    { ICMP6_MIP6_MPS,              "Mobile Prefix Solicitation" },
    { ICMP6_MIP6_MPA,              "Mobile Prefix Advertisement" },
    { ICMP6_CERT_PATH_SOL,         "Certification Path Solicitation" },
    { ICMP6_CERT_PATH_AD,          "Certification Path Advertisement" },
    { ICMP6_EXPERIMENTAL_MOBILITY, "Experimental Mobility" },
    { ICMP6_MCAST_ROUTER_ADVERT,   "Multicast Router Advertisement" },
    { ICMP6_MCAST_ROUTER_SOLICIT,  "Multicast Router Solicitation" },
    { ICMP6_MCAST_ROUTER_TERM,     "Multicast Router Termination" },
    { ICMP6_FMIPV6_MESSAGES,       "FMIPv6 Messages" },
    { ICMP6_RPL_CONTROL,           "RPL Control Message" },
    { 200,                         "Private experimentation" },
    { 201,                         "Private experimentation" },
    { 255,                         "Reserved for expansion of ICMPv6 informational messages" },
    { 0, NULL }
};

static const value_string icmpv6_unreach_code_str[] = {
    { ICMP6_DST_UNREACH_NOROUTE,     "Route unreachable" },
    { ICMP6_DST_UNREACH_ADMIN,       "Administratively prohibited" },
    { ICMP6_DST_UNREACH_BEYONDSCOPE, "Beyond scope of source address" },
    { ICMP6_DST_UNREACH_ADDR,        "Address unreachable" },
    { ICMP6_DST_UNREACH_NOPORT,      "Port unreachable" },
    { ICMP6_DST_UNREACH_INGR_EGR,    "Source address failed ingress/egress policy" },
    { ICMP6_DST_UNREACH_REJECT,      "Reject route to destination" },
    { 0, NULL }
};

static const value_string icmpv6_timeex_code_str[] = {
    { ICMP6_TIME_EXCEED_TRANSIT,    "In-transit" },
    { ICMP6_TIME_EXCEED_REASSEMBLY, "Reassembly" },
    { 0, NULL }
};

static const value_string icmpv6_paramprob_code_str[] = {
    { ICMP6_PARAMPROB_HEADER,     "Header" },
    { ICMP6_PARAMPROB_NEXTHEADER, "Next header" },
    { ICMP6_PARAMPROB_OPTION,     "Option" },
    { 0, NULL }
};

static const value_string icmpv6_router_renum_code_str[] = {
    { ICMP6_ROUTER_RENUMBERING_COMMAND,      "Command" },
    { ICMP6_ROUTER_RENUMBERING_RESULT,       "Result" },
    { ICMP6_ROUTER_RENUMBERING_SEQNUM_RESET, "Sequence number reset" },
    { 0, NULL }
};

static const value_string names_nodeinfo_qtype[] = {
    { NI_QTYPE_NOOP,            "NOOP" },
    { NI_QTYPE_SUPTYPES,        "Supported query types" },
    { NI_QTYPE_DNSNAME,         "DNS name" },
    { NI_QTYPE_NODEADDR,        "Node addresses" },
    { NI_QTYPE_IPV4ADDR,        "IPv4 node addresses" },
    { 0,                        NULL }
};

static const value_string names_rrenum_matchcode[] = {
    { RPM_PCO_ADD,              "Add" },
    { RPM_PCO_CHANGE,           "Change" },
    { RPM_PCO_SETGLOBAL,        "Set Global" },
    { 0,                        NULL }
};

static const value_string names_router_pref[] = {
    { ND_RA_FLAG_RTPREF_HIGH,   "High" },
    { ND_RA_FLAG_RTPREF_MEDIUM, "Medium" },
    { ND_RA_FLAG_RTPREF_LOW,    "Low" },
    { ND_RA_FLAG_RTPREF_RSV,    "Reserved" },
    { 0, NULL}
};

static const value_string names_fmip6_prrtadv_code[] = {
    { FMIP6_PRRTADV_MNTUP,      "MN should use AP-ID, AR-info tuple" },
    { FMIP6_PRRTADV_NI_HOVER,   "Network Initiated Handover trigger" },
    { FMIP6_PRRTADV_NORTINFO,   "No new router information" },
    { FMIP6_PRRTADV_LIMRTINFO,  "Limited new router information" },
    { FMIP6_PRRTADV_UNSOL,      "Unsolicited" },
    { 0,                        NULL }
};

static const value_string names_fmip6_hi_code[] = {
    { FMIP6_HI_PCOA,    "FBU sent from previous link" },
    { FMIP6_HI_NOTPCOA, "FBU sent from new link" },
    { 0,                NULL }
};

static const value_string names_fmip6_hack_code[] = {
    { FMIP6_HACK_VALID,         "Handover Accepted, NCoA valid" },
    { FMIP6_HACK_INVALID,       "Handover Accepted, NCoA not valid" },
    { FMIP6_HACK_INUSE,         "Handover Accepted, NCoA in use" },
    { FMIP6_HACK_ASSIGNED,      "Handover Accepted, NCoA assigned" },
    { FMIP6_HACK_NOTASSIGNED,   "Handover Accepted, NCoA not assigned" },
    { FMIP6_HACK_NOTACCEPTED,   "Handover Not Accepted, reason unspecified" },
    { FMIP6_HACK_PROHIBITED,    "Administratively prohibited" },
    { FMIP6_HACK_INSUFFICIENT,  "Insufficient resources" },
    { 0,                        NULL }
};

static const value_string names_fmip6_ip_addr_opt_code[] = {
    { FMIP6_OPT_IP_ADDRESS_OPTCODE_PCOA, "Old Care-of Address" },
    { FMIP6_OPT_IP_ADDRESS_OPTCODE_NCOA, "New Care-of Address" },
    { FMIP6_OPT_IP_ADDRESS_OPTCODE_NAR,  "NAR's IP address" },
    { 0,                        NULL }
};

static const value_string names_fmip6_lla_opt_code[] = {
    { FMIP6_OPT_LINK_LAYER_ADDRESS_OPTCODE_WILDCARD,  "Wildcard" },
    { FMIP6_OPT_LINK_LAYER_ADDRESS_OPTCODE_NAP,       "Link-layer Address of the New Access Point" },
    { FMIP6_OPT_LINK_LAYER_ADDRESS_OPTCODE_MN,        "Link-layer Address of the MN" },
    { FMIP6_OPT_LINK_LAYER_ADDRESS_OPTCODE_NAR,       "Link-layer Address of the NAR" },
    { FMIP6_OPT_LINK_LAYER_ADDRESS_OPTCODE_SRC,       "Link-layer Address of the source" },
    { FMIP6_OPT_LINK_LAYER_ADDRESS_OPTCODE_CURROUTER, "The AP belongs to the current interface of the router" },
    { FMIP6_OPT_LINK_LAYER_ADDRESS_OPTCODE_NOPREFIX,  "No prefix information available" },
    { FMIP6_OPT_LINK_LAYER_ADDRESS_OPTCODE_NOSUPPORT, "No fast handovers support available" },
    { 0,                        NULL }
};

static const value_string names_fmip6_naack_opt_status[] = {
    { FMIP6_OPT_NEIGHBOR_ADV_ACK_STATUS_INVALID,        "New CoA is invalid" },
    { FMIP6_OPT_NEIGHBOR_ADV_ACK_STATUS_INVALID_NEW,    "New CoA is invalid, use the supplied CoA" },
    { FMIP6_OPT_NEIGHBOR_ADV_ACK_STATUS_UNRECOGNIZED,   "LLA is unrecognized" },
    { 0,                        NULL }
};

static const value_string fmip6_opt_type_str[] = {
    { FMIP6_OPT_IP_ADDRESS,             "IP Address" },
    { FMIP6_OPT_NEW_ROUTER_PREFIX_INFO, "New Router Prefix Information" },
    { FMIP6_OPT_LINK_LAYER_ADDRESS,     "Link-layer Address" },
    { 0, NULL }
};

static const value_string names_6lowpannd_aro_status_str[] = {
    { 0,                                "Success" },
    { 1,                                "Duplicate Exists" },
    { 2,                                "Neighbor Cache Full" },
    { 0, NULL }
};

/* RPL: draft-ietf-roll-rpl-12.txt: Routing over Low-Power and Lossy Networks. */
static const value_string names_rpl_code[] = {
    { ICMP6_RPL_DIS,    "DODAG Information Solicitation" },
    { ICMP6_RPL_DIO,    "DODAG Information Object" },
    { ICMP6_RPL_DAO,    "Destination Advertisement Object" },
    { ICMP6_RPL_DAOACK, "Destination Advertisement Object Acknowledgement" },
    { 0, NULL }
};

/* RPL: draft-ietf-roll-rpl-12.txt: Routing over Low-Power and Lossy Networks. */
static const value_string names_rpl_option[] = {
    { RPL_OPT_PAD1,     "1-byte padding" },
    { RPL_OPT_PADN,     "n-byte padding" },
    { RPL_OPT_METRIC,   "Metric container" },
    { RPL_OPT_ROUTING,  "Routing"},
    { RPL_OPT_CONFIG,   "DODAG configuration" },
    { RPL_OPT_TARGET,   "RPL Target" },
    { RPL_OPT_TRANSIT,  "Transit Information" },
    { RPL_OPT_SOLICITED,"Solicited Information"},
    { RPL_OPT_PREFIX,   "Prefix Information"},
    { RPL_OPT_TARGETDESC, "RPL Target Descriptor"},
    { 0, NULL }
};

/* http://www.iana.org/assignments/icmpv6-parameters */
static const value_string option_vals[] = {
    { ND_OPT_SOURCE_LINKADDR,           "Source link-layer address" },
    { ND_OPT_TARGET_LINKADDR,           "Target link-layer address" },
    { ND_OPT_PREFIX_INFORMATION,        "Prefix information" },
    { ND_OPT_REDIRECTED_HEADER,         "Redirected header" },
    { ND_OPT_MTU,                       "MTU" },
    { ND_OPT_ADVINTERVAL,               "Advertisement Interval" },
    { ND_OPT_HOMEAGENT_INFO,            "Home Agent Information" },
    { ND_OPT_CGA,                       "CGA" },                                    /* [RFC3971] */
    { ND_OPT_RSA,                       "RSA Signature" },                          /* [RFC3971] */
    { ND_OPT_TIMESTAMP,                 "Timestamp" },                              /* [RFC3971] */
    { ND_OPT_NONCE,                     "Nonce" },                                  /* [RFC3971] */
    { ND_OPT_TRUST_ANCHOR,              "Trust Anchor" },                           /* [RFC3971] */
    { ND_OPT_CERTIFICATE,               "Certificate" },                            /* [RFC3971] */
    { FMIP6_OPT_IP_ADDRESS,             "IP Address Option" },                      /* [RFC4068] */
    { FMIP6_OPT_NEW_ROUTER_PREFIX_INFO, "New Router Prefix Information" },          /* [RFC4068] */
    { FMIP6_OPT_LINK_LAYER_ADDRESS,     "Link-layer Address" },                     /* [RFC4068] */
    { FMIP6_OPT_NEIGHBOR_ADV_ACK,       "Neighbor Advertisement Acknowledgment" },  /* [RFC4068] */
	/* 21-22   Unassigned */
    { 23,                               "MAP" },                                    /* [RFC4140] */
    { ND_OPT_ROUTE_INFO,                "Route Information" },                      /* [RFC4191] */
    { ND_OPT_RECURSIVE_DNS_SERVER,      "Recursive DNS Server" },                   /* [RFC5006] */
    { 26,                               "RA Flags Extension" },                     /* [RFC5075] */
    { 27,                               "Handover Key Request" },                   /* [RFC5269] */
    { 28,                               "Handover Key Reply" },                     /* [RFC5269] */
    { 29,                               "Handover Assist Information" },            /* [RFC5271] */
    { 30,                               "Mobile Node Identifier Option" },          /* [RFC5271] */
    { ND_OPT_ADDR_RESOLUTION,           "Address Resolution Option" },              /* 6LoWPAN-ND */
    { ND_OPT_6LOWPAN_CONTEXT,           "6LoWPAN Context Option" },                 /* 6LoWPAN-ND */
    { ND_OPT_AUTH_BORDER_ROUTER,        "Authorative Border Router" },              /* 6LoWPAN-ND */
	/* 31-137  Unassigned */
    { 138,                              "CARD Request" },                           /* [RFC4065] */
    { 139,                              "CARD Reply" },                             /* [RFC4065] */
	/* 140-252 Unassigned */
    { ND_OPT_MAP,                       "HMIPv6 MAP option" },                      /* unassigned */
    { 253,                              "RFC3692-style Experiment 1" },             /* [RFC4727] */
    { 254,                              "RFC3692-style Experiment 2" },             /* [RFC4727] */
    { 0,                                NULL }
};

static const value_string icmpv6_option_name_type_vals[] = {
    { 1,    "DER Encoded X.501 Name" },
    { 2,    "FQDN" },
    { 3,    "SHA-1 Subject Key Identifier (SKI)" },
    { 4,    "SHA-224 Subject Key Identifier (SKI)" },
    { 5,    "SHA-256 Subject Key Identifier (SKI)" },
    { 6,    "SHA-384 Subject Key Identifier (SKI)" },
    { 7,    "SHA-512 Subject Key Identifier (SKI)" },
    { 253,  "Reserved for Experimental Use" },
    { 254,  "Reserved for Experimental Use" },
    { 255,  "Reserved" },
    { 0,    NULL }
};

static const value_string icmpv6_option_cert_type_vals[] = {
    { 1,    "X.509v3 Certificate" },
    { 0,    NULL }
};

static void
dissect_contained_icmpv6(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree)
{
    gboolean save_in_error_pkt;
    tvbuff_t *next_tvb;

    /* Save the current value of the "we're inside an error packet"
       flag, and set that flag; subdissectors may treat packets
       that are the payload of error packets differently from
       "real" packets. */
    save_in_error_pkt = pinfo->in_error_pkt;
    pinfo->in_error_pkt = TRUE;
    pinfo->in_error_pkt = TRUE;

    next_tvb = tvb_new_subset_remaining(tvb, offset);

    /* tiny sanity check */
    if ((tvb_get_guint8(tvb, offset) & 0xf0) == 0x60) {
        /* The contained packet is an IPv6 datagram; dissect it. */
        call_dissector(ipv6_handle, next_tvb, pinfo, tree);
    } else
        call_dissector(data_handle,next_tvb, pinfo, tree);

    /* Restore the "we're inside an error packet" flag. */
    pinfo->in_error_pkt = save_in_error_pkt;
}

static void
dissect_icmpv6ndopt(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree)
{
    proto_tree *icmp6opt_tree, *field_tree, *name_tree, *cga_tree;
    proto_item *ti, *tf, *name_item, *cga_item;
    struct nd_opt_hdr nd_opt_hdr, *opt;
    int len;
    const char *typename;
    static const guint8 nd_redirect_reserved[6] = {0, 0, 0, 0, 0, 0};
    guint8 nd_redirect_res[6];
    int opt_offset;
    guint8 padd_length = 0;
    int par_len;
    guint8 name_type = 0;
    guint8 cert_type = 0;
    asn1_ctx_t asn1_ctx;
    guint32 lifetime;
    guint32 no_of_pars;
    guint32 i;

    while ((int)tvb_reported_length(tvb) > offset) {
	/* there are more options */

	opt = &nd_opt_hdr;
	tvb_memcpy(tvb, (guint8 *)opt, offset, sizeof *opt);
	len = opt->nd_opt_len << 3;

	/* !!! specify length */
	ti = proto_tree_add_item(tree, hf_icmpv6_option, tvb, offset, len, FALSE);
	icmp6opt_tree = proto_item_add_subtree(ti, ett_icmpv6opt);

	if (len == 0) {
	    proto_tree_add_text(icmp6opt_tree, tvb,
				offset + offsetof(struct nd_opt_hdr, nd_opt_len), 1,
				"Invalid option length: %u",
				opt->nd_opt_len);
	    return; /* we must not try to decode this */
	}

	typename = val_to_str(opt->nd_opt_type, option_vals, "Unknown");

	/* Add option name to option root label */
	proto_item_append_text(ti, " (%s)", typename);

	/* Option type */
	proto_tree_add_item(icmp6opt_tree, hf_icmpv6_option_type, tvb,
			    offset + offsetof(struct nd_opt_hdr, nd_opt_type), 1, FALSE);
	/* Option length */
	proto_tree_add_uint(icmp6opt_tree, hf_icmpv6_option_length, tvb,
			    offset + offsetof(struct nd_opt_hdr, nd_opt_len), 1,
			    opt->nd_opt_len << 3);

	/* decode... */
	switch (opt->nd_opt_type) {
	case ND_OPT_SOURCE_LINKADDR:
	case ND_OPT_TARGET_LINKADDR:
	{
	    int len_local, p;
	    gchar *llstr;

	    p = offset + sizeof(*opt);
	    len_local = (opt->nd_opt_len << 3) - sizeof(*opt);
	    llstr = tvb_bytes_to_str_punct(tvb, p, len_local, ':');
	    proto_tree_add_text(icmp6opt_tree, tvb,
				offset + sizeof(*opt), len_local, "Link-layer address: %s",
				llstr);
	    col_append_fstr(pinfo->cinfo, COL_INFO,
			    " %s %s",
			    opt->nd_opt_type == ND_OPT_SOURCE_LINKADDR ? "from" : "is at",
			    llstr);
	    break;
	}
	case ND_OPT_PREFIX_INFORMATION:
	{
		    /* RFC 4861 */
	    struct nd_opt_prefix_info nd_opt_prefix_info, *pi;
	    int flagoff;

	    pi = &nd_opt_prefix_info;
	    tvb_memcpy(tvb, (guint8 *)pi, offset, sizeof *pi);
		    proto_tree_add_item(icmp6opt_tree, hf_icmpv6_opt_prefix_len, tvb, offset + 2, 1, FALSE);

	    flagoff = offset + 3;
	    tf = proto_tree_add_text(icmp6opt_tree, tvb, flagoff, 1, "Flags: 0x%02x",
				     tvb_get_guint8(tvb, flagoff));
	    field_tree = proto_item_add_subtree(tf, ett_icmpv6flag);
		    proto_tree_add_item(field_tree, hf_icmpv6_opt_flag_l, tvb, offset + 3, 1, FALSE);
		    proto_tree_add_item(field_tree, hf_icmpv6_opt_flag_a, tvb, offset + 3, 1, FALSE);
		    proto_tree_add_item(field_tree, hf_icmpv6_opt_flag_reserved, tvb, offset + 3, 1, FALSE);
    #if 0
		    /* This bits are not in the RFC, removed the code Anders Broman 2010-06-02
			    if no one complains for a while delete it. */
	    proto_tree_add_text(field_tree, tvb, flagoff, 1, "%s",
				decode_boolean_bitfield(pi->nd_opt_pi_flags_reserved,
							ND_OPT_PI_FLAG_ROUTER, 8,
							"Router Address", "Not router address"));
	    proto_tree_add_text(field_tree, tvb, flagoff, 1, "%s",
				decode_boolean_bitfield(pi->nd_opt_pi_flags_reserved,
							ND_OPT_PI_FLAG_SITEPREF, 8,
							"Site prefix", "Not site prefix"));
    #endif
	    if (pntohl(&pi->nd_opt_pi_valid_time) == 0xffffffff)
		proto_tree_add_text(icmp6opt_tree, tvb,
				    offset+4,
				    4, "Valid lifetime: infinity");
	    else
		proto_tree_add_text(icmp6opt_tree, tvb,
				    offset+4,
				    4, "Valid lifetime: %u",
				    pntohl(&pi->nd_opt_pi_valid_time));
	    if (pntohl(&pi->nd_opt_pi_preferred_time) == 0xffffffff)
		proto_tree_add_text(icmp6opt_tree, tvb,
				    offset+8,
				    4, "Preferred lifetime: infinity");
	    else
		proto_tree_add_text(icmp6opt_tree, tvb,
				    offset+8,
				    4, "Preferred lifetime: %u",
				    pntohl(&pi->nd_opt_pi_preferred_time));
	    proto_tree_add_text(icmp6opt_tree, tvb,
				offset + 12,
				4, "Reserved");
	    proto_tree_add_text(icmp6opt_tree, tvb,
				offset + 16,
				16, "Prefix: %s", ip6_to_str(&pi->nd_opt_pi_prefix));
	    break;
	}
	case ND_OPT_REDIRECTED_HEADER:
	    tvb_memcpy(tvb, (guint8 *)&nd_redirect_res, offset + 2, 6);
	    if (memcmp(nd_redirect_res, nd_redirect_reserved, 6) == 0)
		proto_tree_add_text(icmp6opt_tree, tvb,
				    offset + 2, 6, "Reserved: 0 (correct)");
	    else
		proto_tree_add_text(icmp6opt_tree, tvb,
				    offset +2, 6, "Reserved: MUST be 0 (incorrect!)");
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
				"Advertisement Interval: %u",
				tvb_get_ntohl(tvb, offset + offsetof(struct nd_opt_adv_int, nd_opt_adv_int_advint)));
	    break;
	case ND_OPT_HOMEAGENT_INFO: /* 8 */
	{
	    struct nd_opt_ha_info pibuf, *pi;

	    pi = &pibuf;
	    tvb_memcpy(tvb, (guint8 *)pi, offset, sizeof *pi);
	    proto_tree_add_text(icmp6opt_tree, tvb,
				offset + offsetof(struct nd_opt_ha_info, nd_opt_ha_info_ha_pref),
				2, "Home Agent Preference: %d",
				(gint16)pntohs(&pi->nd_opt_ha_info_ha_pref));
	    proto_tree_add_text(icmp6opt_tree, tvb,
				offset + offsetof(struct nd_opt_ha_info, nd_opt_ha_info_ha_life),
				2, "Home Agent Lifetime: %u",
				pntohs(&pi->nd_opt_ha_info_ha_life));
	    break;
	}
	case ND_OPT_CGA: /* 11 */ {
	    guint16 ext_data_len;
	    /* RFC 3971 5.1.  CGA Option */
	    /* Pad Length */
	    opt_offset = offset +2;
	    padd_length = tvb_get_guint8(tvb,opt_offset);
	    proto_tree_add_item(icmp6opt_tree, hf_icmpv6_opt_cga_pad_len, tvb, opt_offset, 1, FALSE);
	    opt_offset++;
	    /* Reserved 8 bits */
	    proto_tree_add_text(icmp6opt_tree, tvb,opt_offset,1,"Reserved");
	    opt_offset++;
	    /* CGA Parameters A variable-length field containing the CGA Parameters data
	     * structure described in Section 4 of
	     * "Cryptographically Generated Addresses (CGA)", RFC3972.
	     */
	    par_len = len-4-padd_length;
	    cga_item = proto_tree_add_item(icmp6opt_tree, hf_icmpv6_opt_cga, tvb, opt_offset, par_len, FALSE);
	    par_len += opt_offset;
	    cga_tree = proto_item_add_subtree(cga_item, ett_cga_param_name);
	    proto_tree_add_item(cga_tree,hf_icmpv6_opt_cga_modifier,tvb, opt_offset,16,FALSE);
	    opt_offset += 16;
	    proto_tree_add_item(cga_tree,hf_icmpv6_opt_cga_subnet_prefix,tvb,opt_offset,8,FALSE);
	    opt_offset += 8;
	    proto_tree_add_item(cga_tree,hf_icmpv6_opt_cga_count,tvb,opt_offset,1,FALSE);
	    opt_offset++;
	    asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
	    opt_offset = dissect_x509af_SubjectPublicKeyInfo(FALSE, tvb, opt_offset, &asn1_ctx, cga_tree, -1);
	    /* Process RFC 4581*/
	    while (opt_offset < par_len) {
		proto_tree_add_item(cga_tree,hf_icmpv6_opt_cga_ext_type,tvb,opt_offset,2,FALSE);
		opt_offset += 2;
		ext_data_len = tvb_get_ntohs(tvb,opt_offset);
		proto_tree_add_item(cga_tree,hf_icmpv6_opt_cga_ext_length,tvb,opt_offset,2,FALSE);
		opt_offset += 2;
		proto_tree_add_text(cga_tree,tvb,opt_offset,ext_data_len,"Ext Data");
		opt_offset += ext_data_len;
	    }

	    /* Padding */
	    proto_tree_add_text(icmp6opt_tree, tvb,opt_offset,padd_length,"Padding");
	    break;
	}
	case ND_OPT_RSA: /* 12 */
	    /*5.2.  RSA Signature Option */
	    opt_offset = offset +2;
	    /* Reserved, A 16-bit field reserved for future use. */
	    proto_tree_add_text(icmp6opt_tree, tvb,opt_offset,2,"Reserved");
	    opt_offset = opt_offset + 2;
	    /* Key Hash
	     * A 128-bit field containing the most significant (leftmost) 128
	     * bits of a SHA-1 [14] hash of the public key used for constructing
	     * the signature.
	     */
	    proto_tree_add_item(icmp6opt_tree, hf_icmpv6_opt_rsa_key_hash, tvb, opt_offset, 16, FALSE);
	    opt_offset = opt_offset + 16;
	    /* Digital Signature */
	    par_len = len - 20;
	    proto_tree_add_text(icmp6opt_tree, tvb,opt_offset,par_len,"Digital Signature + Padding");
	    /* Padding */
	    /* TODO: Calculate padding length and exlude from the signature */
	    break;
	case ND_OPT_TIMESTAMP: /* 13 */
	    opt_offset = offset +2;
	    /* Reserved A 48-bit field reserved for future use. */
	    proto_tree_add_text(icmp6opt_tree, tvb,opt_offset,6,"Reserved");
	    opt_offset = opt_offset + 6;
	    /* Timestamp
	     * A 64-bit unsigned integer field containing a timestamp.  The value
	     * indicates the number of seconds since January 1, 1970, 00:00 UTC,
	     * by using a fixed point format.  In this format, the integer number
	     * of seconds is contained in the first 48 bits of the field, and the
	     * remaining 16 bits indicate the number of 1/64K fractions of a
	     * second.
	     */
	    proto_tree_add_text(icmp6opt_tree, tvb,opt_offset,6,"Timestamp(number of seconds since January 1, 1970, 00:00 UTC)");
	    opt_offset = opt_offset + 6;
	    proto_tree_add_text(icmp6opt_tree, tvb,opt_offset,2,"Timestamp(1/64K fractions of a second)");
	    break;
	case ND_OPT_NONCE:
	    /* 5.3.2.  Nonce Option */
	    opt_offset = offset +2;
	    proto_tree_add_text(icmp6opt_tree, tvb,opt_offset,len-2,"Nonce");
	    /* Nonce */
	    break;
	case ND_OPT_TRUST_ANCHOR:
	    opt_offset = offset +2;
	    /* Name Type */
	    name_type = tvb_get_guint8(tvb,opt_offset);
	    proto_tree_add_item(icmp6opt_tree, hf_icmpv6_opt_name_type, tvb, opt_offset, 1, FALSE);
	    opt_offset++;
	    /* Pad Length */
	    padd_length = tvb_get_guint8(tvb,opt_offset);
	    proto_tree_add_item(icmp6opt_tree, hf_icmpv6_opt_cga_pad_len, tvb, opt_offset, 1, FALSE);
	    opt_offset++;
	    par_len = len - 4 - padd_length;
	    switch (name_type){
	    case 1:
		/* DER Encoded X.501 Name */
		name_item =proto_tree_add_item(icmp6opt_tree, hf_icmpv6_opt_name_x501, tvb, opt_offset, par_len, FALSE);
		name_tree = proto_item_add_subtree(name_item, ett_icmpv6opt_name);
		asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
		dissect_x509if_Name(FALSE, tvb, opt_offset, &asn1_ctx, name_tree, hf_icmpv6_x509if_Name);
		break;
	    case 2:
		/* FQDN */
		proto_tree_add_item(icmp6opt_tree, hf_icmpv6_opt_name_fqdn, tvb, opt_offset, par_len, FALSE);
		break;
	    default:
		proto_tree_add_text(icmp6opt_tree, tvb,opt_offset, par_len,"Unknown name type");
		break;
	    }
	    opt_offset = opt_offset + par_len;
	    /* Padding */
	    proto_tree_add_text(icmp6opt_tree, tvb,opt_offset,padd_length,"Padding");
	    opt_offset = opt_offset + padd_length;
	    break;
	case ND_OPT_CERTIFICATE:
	    opt_offset = offset +2;
	    /* Cert Type */
	    cert_type = tvb_get_guint8(tvb,opt_offset);
	    proto_tree_add_item(icmp6opt_tree, hf_icmpv6_opt_cert_type, tvb, opt_offset, 1, FALSE);
	    opt_offset++;
	    /* Reserved */
	    proto_tree_add_text(icmp6opt_tree, tvb,opt_offset,1,"Reserved");
	    opt_offset++;
	    /* Certificate */

	    if(cert_type == 1){
		asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
		opt_offset = dissect_x509af_Certificate(FALSE, tvb, opt_offset, &asn1_ctx, icmp6opt_tree, hf_icmpv6_x509af_Certificate);
		par_len = len - (opt_offset - offset);
		/* Padding */
		proto_tree_add_text(icmp6opt_tree, tvb,opt_offset,par_len,"Padding");
	    }else{
		par_len = len - 4;
		proto_tree_add_text(icmp6opt_tree, tvb,opt_offset,par_len,"Unknown Certificate + padding");
	    }
	    break;
	case ND_OPT_MAP:
	{
	    struct nd_opt_map_info mapbuf, *map;
	    int flagoff;

	    map = &mapbuf;
	    tvb_memcpy(tvb, (guint8 *)map, offset, sizeof *map);
	    proto_tree_add_text(icmp6opt_tree, tvb,
				offset + offsetof(struct nd_opt_map_info, nd_opt_map_dist_and_pref),
				1, "Distance: %u", (map->nd_opt_map_dist_and_pref >> 4));
	    proto_tree_add_text(icmp6opt_tree, tvb,
				offset + offsetof(struct nd_opt_map_info, nd_opt_map_dist_and_pref),
				1, "Preference: %u", (map->nd_opt_map_dist_and_pref & 0x0F));
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
	    proto_tree_add_text(field_tree, tvb, flagoff, 1, "%s",
				decode_boolean_bitfield(map->nd_opt_map_flags,
							ND_OPT_MAP_FLAG_V, 8, "V", "No V"));
	    proto_tree_add_text(icmp6opt_tree, tvb,
				offset + offsetof(struct nd_opt_map_info, nd_opt_map_lifetime),
				4, "Lifetime: %u", pntohl(&map->nd_opt_map_lifetime));

	    proto_tree_add_text(icmp6opt_tree, tvb,
				offset + offsetof(struct nd_opt_map_info, nd_opt_map_address), 16,
				"Address of MAP: %s (%s)",
				get_hostname6(&map->nd_opt_map_address),
				ip6_to_str(&map->nd_opt_map_address));
	    break;
	}
	case ND_OPT_ROUTE_INFO:
	{
	    struct nd_opt_route_info ribuf, *ri;
	    struct e_in6_addr in6;
	    int l;
	    guint32 lifetime_local;

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
		lifetime_local = pntohl(&ri->nd_opt_rti_lifetime);
		if (lifetime_local == 0xffffffff)
		    proto_tree_add_text(icmp6opt_tree, tvb,
					offset + offsetof(struct nd_opt_route_info, nd_opt_rti_lifetime),
					sizeof(ri->nd_opt_rti_lifetime), "Lifetime: infinity");
		else
		    proto_tree_add_text(icmp6opt_tree, tvb,
					offset + offsetof(struct nd_opt_route_info, nd_opt_rti_lifetime),
					sizeof(ri->nd_opt_rti_lifetime), "Lifetime: %u", lifetime_local);
		proto_tree_add_text(icmp6opt_tree, tvb,
				    offset + sizeof(*ri), l, "Prefix: %s", ip6_to_str(&in6));
	    } else {
		proto_tree_add_text(icmp6opt_tree, tvb,
				    offset + offsetof(struct nd_opt_hdr, nd_opt_len), 1,
				    "Invalid option length: %u", opt->nd_opt_len);
	    }
	    break;
	}
	case FMIP6_OPT_NEIGHBOR_ADV_ACK:
	{
	    struct fmip6_opt_neighbor_advertisement_ack fmip6_opt_neighbor_advertisement_ack, *opt_naack;
	    struct e_in6_addr in6;

	    opt_naack = &fmip6_opt_neighbor_advertisement_ack;
	    tvb_memcpy(tvb, (guint8 *)opt_naack, offset, sizeof *opt_naack);

	    proto_tree_add_text(icmp6opt_tree, tvb,
				offset + offsetof(struct fmip6_opt_neighbor_advertisement_ack, fmip6_opt_optcode),
				1, "Option-Code: %u",
				opt_naack->fmip6_opt_optcode);

	    proto_tree_add_text(icmp6opt_tree, tvb,
				offset + offsetof(struct fmip6_opt_neighbor_advertisement_ack, fmip6_opt_status),
				1, "Status: %s",
				val_to_str(opt_naack->fmip6_opt_status, names_fmip6_naack_opt_status, "Unknown"));

	    if (opt_naack->fmip6_opt_len == 3){
		tvb_memcpy(tvb, (guint8 *)&in6, offset + sizeof(*opt_naack), 16);
		proto_tree_add_text(icmp6opt_tree, tvb,
				    offset + sizeof(*opt_naack),
				    16, "New Care-of Address: %s",
				    ip6_to_str(&in6));
	    }

	    break;
	}
	case ND_OPT_RECURSIVE_DNS_SERVER:

	    opt_offset = offset + 2;
	    proto_tree_add_text(icmp6opt_tree, tvb, opt_offset ,2 ,"Reserved");
	    opt_offset = opt_offset + 2;
	    /* A value of all one bits (0xffffffff) represents infinity.  A value of
	     * zero means that the RDNSS address MUST no longer be used.
	     */
	    lifetime = tvb_get_ntohl(tvb, opt_offset);
	    if (lifetime == 0xffffffff){
		proto_tree_add_text(icmp6opt_tree, tvb, opt_offset ,4 ,"Lifetime: infinity");
	    }else{
		if(lifetime==0){
		    proto_tree_add_text(icmp6opt_tree, tvb, opt_offset ,4 ,"Lifetime: RDNSS address MUST no longer be used");
		}else{
		    proto_tree_add_text(icmp6opt_tree, tvb, opt_offset ,4 ,"Lifetime: %u", lifetime);
		}
	    }
	    opt_offset = opt_offset+4;
	    /* Addresses of IPv6 Recursive DNS Servers */
	    no_of_pars = opt->nd_opt_len - 1;
	    no_of_pars = no_of_pars >> 2;

	    for (i = 0; i <= no_of_pars; i++) {
		proto_tree_add_item(icmp6opt_tree, hf_icmpv6_recursive_dns_serv, tvb, opt_offset, 16, FALSE);
		opt_offset = opt_offset+16;
	    }
	    break;
	case ND_OPT_6LOWPAN_CONTEXT:
	{
	    /* 6lowpan-ND */
	    struct nd_opt_6lowpan_context nd_opt_6lowpan_context, *sco;
	    struct e_in6_addr in6;
	    int flagoff;

	    sco = &nd_opt_6lowpan_context;
	    tvb_memcpy(tvb, (guint8 *)sco, offset, sizeof *sco);
	    /* Length */
	    proto_tree_add_uint(icmp6opt_tree, hf_icmpv6_opt_6co_context_length, tvb,
				    offset + offsetof(struct nd_opt_6lowpan_context, nd_opt_6co_context_length),
				    1, sco->nd_opt_6co_context_length);

	    /* Lifetime */
	    proto_tree_add_uint(icmp6opt_tree, hf_icmpv6_opt_6co_lifetime, tvb,
				    offset + offsetof(struct nd_opt_6lowpan_context, nd_opt_6co_lifetime),
				    2, pntohs(&sco->nd_opt_6co_lifetime));

	    /* CID & Flags */
	    flagoff = offset + offsetof(struct nd_opt_6lowpan_context,nd_opt_6co_CID);
	    tf = proto_tree_add_text(icmp6opt_tree, tvb, flagoff, 1,
				    "Flags & CID: 0x%02x", tvb_get_guint8(tvb, offset +
				    offsetof(struct nd_opt_6lowpan_context, nd_opt_6co_CID)));
	    field_tree = proto_item_add_subtree(tf, ett_icmpv6flag);
	    proto_tree_add_text(field_tree, tvb, flagoff, 1, "%s",
				decode_boolean_bitfield(sco->nd_opt_6co_CID,
							ND_OPT_6CO_CFLAG, 8, "Valid for Compression", "Not Valid for Compression"));

	    proto_tree_add_text(field_tree, tvb, flagoff, 1, "%s",
				decode_numeric_bitfield(sco->nd_opt_6co_CID,
							ND_OPT_6CO_CIDMASK, 8, "CID of %d"));

	    /* Context */
	    if(sco->nd_opt_6co_context_length > 64){
		tvb_memcpy(tvb, (guint8 *)&in6, offset + sizeof *sco, 16);
	    } else {
		tvb_memcpy(tvb, (guint8 *)&in6, offset + sizeof *sco, 8);
		in6.bytes[8] = 0;
		in6.bytes[9] = 0;
		in6.bytes[10] = 0;
		in6.bytes[11] = 0;
		in6.bytes[12] = 0;
		in6.bytes[13] = 0;
		in6.bytes[14] = 0;
		in6.bytes[15] = 0;
	    }

	    proto_tree_add_text(icmp6opt_tree, tvb, offset + sizeof *sco,
				    16, "Context: %s", ip6_to_str(&in6));
	}
	break;
	case ND_OPT_ADDR_RESOLUTION:
	{
	    /* 6lowpan-ND */
	    struct nd_opt_addr_resolution nd_opt_addr_resolution, *aro;

	    aro = &nd_opt_addr_resolution;
	    tvb_memcpy(tvb, (guint8 *)aro, offset, sizeof *aro);
	    /* Status */
	    proto_tree_add_text(icmp6opt_tree, tvb,
				offset + offsetof(struct nd_opt_addr_resolution, nd_opt_aro_status),
				1, "Status: %s",
				val_to_str(aro->nd_opt_aro_status, names_6lowpannd_aro_status_str, "Unknown"));

	    /* Lifetime */
	    proto_tree_add_uint(icmp6opt_tree, hf_icmpv6_opt_aro_lifetime, tvb,
				    offset + offsetof(struct nd_opt_addr_resolution, nd_opt_aro_lifetime),
				    2, pntohs(&aro->nd_opt_aro_lifetime));

	    /* EUI-64 */
	    proto_tree_add_item(icmp6opt_tree, hf_icmpv6_opt_aro_eui64, tvb, offset + offsetof(struct nd_opt_addr_resolution, nd_opt_aro_eui64), 8, FALSE);

	    /* Registered Address (if present only) */
	    if(aro->nd_opt_aro_len == 4)
	    {
		proto_tree_add_item(icmp6opt_tree, hf_icmpv6_opt_aro_addr_reg, tvb, offset + sizeof *aro, 16, FALSE);
	    }

	}
	break;
	case ND_OPT_AUTH_BORDER_ROUTER:
	{
	    /* Version at offset 2, Address at offset 8 */
	    guint8 version[2];
	    tvb_memcpy(tvb, version, offset+2, 2);

	    proto_tree_add_uint(icmp6opt_tree, hf_icmpv6_opt_abro_version, tvb,
				    offset + 2, 2, pntohs(version));

	    proto_tree_add_item(icmp6opt_tree, hf_icmpv6_opt_abro_addr, tvb,
				offset + 8, 16, FALSE);

	}
	    break;
	} /* switch (opt->nd_opt_type) */

	offset += (opt->nd_opt_len << 3);

	/* Set length of option tree */
	proto_item_set_len(ti, opt->nd_opt_len << 3);
    }
}

static void
dissect_icmpv6fmip6opt(tvbuff_t *tvb, int offset, proto_tree *tree)
{
    proto_tree *icmp6opt_tree;
    proto_item *ti;
    struct fmip6_opt_hdr fmip6_opt_hdr, *opt;
    int len;
    const char *typename;

    if (!tree)
        return;

    while ((int)tvb_reported_length(tvb) > offset) {
        /* there are more options */

	opt = &fmip6_opt_hdr;
	tvb_memcpy(tvb, (guint8 *)opt, offset, sizeof *opt);
	len = opt->fmip6_opt_len << 3;

	/* !!! specify length */
	ti = proto_tree_add_text(tree, tvb, offset, len, "ICMPv6 options");
	icmp6opt_tree = proto_item_add_subtree(ti, ett_icmpv6opt);

	if (len == 0) {
	    proto_tree_add_text(icmp6opt_tree, tvb,
				offset + offsetof(struct fmip6_opt_hdr, fmip6_opt_len), 1,
				"Invalid option length: %u",
				opt->fmip6_opt_len);
	    return; /* we must not try to decode this */
	}

	typename = val_to_str (opt->fmip6_opt_type, fmip6_opt_type_str, "Unknown");

	proto_tree_add_text(icmp6opt_tree, tvb,
			    offset + offsetof(struct fmip6_opt_hdr, fmip6_opt_type), 1,
			    "Type: %u (%s)", opt->fmip6_opt_type, typename);
	proto_tree_add_text(icmp6opt_tree, tvb,
			    offset + offsetof(struct fmip6_opt_hdr, fmip6_opt_len), 1,
			    "Length: %u bytes (%u)", opt->fmip6_opt_len << 3, opt->fmip6_opt_len);

	/* decode... */
	switch (opt->fmip6_opt_type) {
	case FMIP6_OPT_IP_ADDRESS:
	{
	    struct fmip6_opt_ip_address fmip6_opt_ip_address, *opt_ip;

	    opt_ip = &fmip6_opt_ip_address;
	    tvb_memcpy(tvb, (guint8 *)opt_ip, offset, sizeof *opt_ip);

	    proto_tree_add_text(icmp6opt_tree, tvb,
				offset + offsetof(struct fmip6_opt_hdr, fmip6_opt_optcode), 1, "Option-Code: %s",
				val_to_str(opt->fmip6_opt_optcode, names_fmip6_ip_addr_opt_code, "Unknown"));

	    proto_tree_add_text(icmp6opt_tree, tvb,
				offset + offsetof(struct fmip6_opt_ip_address, fmip6_opt_prefix_len),
				1, "Prefix length: %u", opt_ip->fmip6_opt_prefix_len);

	    proto_tree_add_text(icmp6opt_tree, tvb,
				offset + offsetof(struct fmip6_opt_ip_address, fmip6_opt_ip6_address),
				16, "IPv6 Address: %s",
				ip6_to_str(&opt_ip->fmip6_opt_ip6_address));
	    break;
	}
	case FMIP6_OPT_NEW_ROUTER_PREFIX_INFO:
	{
	    struct fmip6_opt_new_router_prefix_info fmip6_opt_new_router_prefix_info, *opt_nr;

	    opt_nr = &fmip6_opt_new_router_prefix_info;
	    tvb_memcpy(tvb, (guint8 *)opt_nr, offset, sizeof *opt_nr);

	    proto_tree_add_text(icmp6opt_tree, tvb,
				offset + offsetof(struct fmip6_opt_hdr, fmip6_opt_optcode), 1, "Option-Code: %u",
				opt->fmip6_opt_optcode);

	    proto_tree_add_text(icmp6opt_tree, tvb,
				offset + offsetof(struct fmip6_opt_new_router_prefix_info, fmip6_opt_prefix_len),
				1, "Prefix length: %u", opt_nr->fmip6_opt_prefix_len);

	    proto_tree_add_text(icmp6opt_tree, tvb,
				offset + offsetof(struct fmip6_opt_new_router_prefix_info, fmip6_opt_prefix),
				16, "Prefix: %s",
				ip6_to_str(&opt_nr->fmip6_opt_prefix));
	    break;
	}
	case FMIP6_OPT_LINK_LAYER_ADDRESS:
	{
	    int len_local, p;

	    p = offset + sizeof(*opt);
	    proto_tree_add_text(icmp6opt_tree, tvb,
				offset + offsetof(struct fmip6_opt_hdr, fmip6_opt_optcode), 1, "Option-Code: %s",
				val_to_str(opt->fmip6_opt_optcode, names_fmip6_lla_opt_code, "Unknown"));
	    len_local = (opt->fmip6_opt_len << 3) - sizeof(*opt);
	    proto_tree_add_text(icmp6opt_tree, tvb,
				offset + sizeof(*opt), len_local, "Link-layer address: %s",
				tvb_bytes_to_str_punct(tvb, p, len_local, ':'));
	    break;
	}
	} /* switch (opt->fmip6_opt_type) */

	offset += (opt->fmip6_opt_len << 3);
    }
}

/* RPL: draft-ietf-roll-rpl-12.txt: Routing over Low-Power and Lossy Networks. */
static void
dissect_icmpv6rplopt(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree)
{
    proto_tree *icmp6opt_tree;
    proto_item *ti;
    guint8      type;
    guint16     optlen;
    int         optoffset;
    const char *type_name;
    proto_item *ti_plen = NULL;

    if (!tree)
        return;

    while ((int)tvb_reported_length(tvb) > offset) {
	/* there are more options */

	/* Make a subtree for the option. */
	ti = proto_tree_add_item(tree, hf_icmpv6_option, tvb, offset, 1, FALSE);
	icmp6opt_tree = proto_item_add_subtree(ti, ett_icmpv6opt);

	/* Get the option type. */
	type = tvb_get_guint8(tvb, offset);
	type_name = val_to_str(type, names_rpl_option, "Unknown");
	proto_item_append_text(ti, " (%s)", type_name);
	proto_tree_add_text(icmp6opt_tree, tvb, offset, 1, "Type: %u (%s)", type, type_name);

	/* The Pad1 option is a special case, and contains no data. */
	if (type == RPL_OPT_PAD1) {
	    offset++;
	    continue;
	}
	optlen = tvb_get_guint8(tvb, offset + 1);
	proto_item_set_len(ti, optlen + 2);
	proto_tree_add_uint(icmp6opt_tree, hf_icmpv6_option_length, tvb, offset + 1, 1, optlen);

	/* Display the option contents. */
	offset += 2;
	optoffset = offset;
	switch(type) {
	    case RPL_OPT_PADN:
		/* n-byte padding */
		proto_tree_add_text(icmp6opt_tree, tvb, offset, optlen, "Padding length is %d", optlen+2);
		break;

	    case RPL_OPT_METRIC:
		/* DAG metric container */
		/* See draft-ietf-roll-routing-metrics for formatting. */
		break;

	    case RPL_OPT_ROUTING: {
		guint8              route_len;
		struct e_in6_addr   route;

		/* Route length */
		route_len = tvb_get_guint8(tvb, optoffset);
		ti_plen = proto_tree_add_uint(icmp6opt_tree, hf_icmpv6_rpl_opt_route_length, tvb, optoffset, 1, route_len);
		optoffset++;

		if (route_len > (sizeof(route) << 3)) {
		    /* Illegal prefix length! Must prevent evil buffer overflows >:@ */
		    route_len = sizeof(route) << 3;
		    expert_add_info_format(pinfo, ti_plen, PI_MALFORMED, PI_ERROR, "Route length invalid, greather than 128 bits");
		}

		/* Flags - only preference is used anymore. */
		proto_tree_add_item(icmp6opt_tree, hf_icmpv6_rpl_opt_route_pref, tvb, optoffset, 1, FALSE);
		optoffset++;

		/* Prefix lifetime. */
		proto_tree_add_item(icmp6opt_tree, hf_icmpv6_rpl_opt_route_lifetime, tvb, optoffset, 4, FALSE);
		optoffset += 4;

		/* Prefix */
		memset(&route, 0, sizeof(route));
		tvb_memcpy(tvb, &route, optoffset, (route_len + 7) >> 3);         /* Round up to the nearest octet. */
		if (route_len & 0x7)
		    route.bytes[route_len >> 3] &= 0xff << (route_len & 0x7);    /* Clear unused bits in the last octet. */
		proto_tree_add_ipv6(icmp6opt_tree, hf_icmpv6_rpl_opt_route, tvb, optoffset, ((route_len + 7) >> 3), route.bytes);
		optoffset += ((route_len + 7) >> 3);

		break;
	    }

	    case RPL_OPT_CONFIG: {
		/* flags */
		guint8 flags = tvb_get_guint8(tvb, optoffset);
		proto_tree_add_boolean(icmp6opt_tree, hf_icmpv6_rpl_opt_config_auth, tvb, optoffset, 1, flags & RPL_OPT_CONFIG_FLAG_AUTH);
		proto_tree_add_uint(icmp6opt_tree, hf_icmpv6_rpl_opt_config_pcs, tvb, optoffset, 1, flags & RPL_OPT_CONFIG_FLAG_PCS);
		optoffset += 1;
		/* DAG configuration */
		proto_tree_add_item(icmp6opt_tree, hf_icmpv6_rpl_opt_config_doublings, tvb, optoffset, 1, FALSE);
		optoffset++;
		proto_tree_add_item(icmp6opt_tree, hf_icmpv6_rpl_opt_config_min_interval, tvb, optoffset, 1, FALSE);
		optoffset++;
		proto_tree_add_item(icmp6opt_tree, hf_icmpv6_rpl_opt_config_redundancy, tvb, optoffset, 1, FALSE);
		optoffset++;
		proto_tree_add_item(icmp6opt_tree, hf_icmpv6_rpl_opt_config_rank_incr, tvb, optoffset, 2, FALSE);
		optoffset += 2;
		proto_tree_add_item(icmp6opt_tree, hf_icmpv6_rpl_opt_config_hop_rank_inc, tvb, optoffset, 2, FALSE);
		optoffset += 2;
		proto_tree_add_item(icmp6opt_tree, hf_icmpv6_rpl_opt_config_ocp, tvb, optoffset, 2, FALSE);
		optoffset += 2;
		proto_tree_add_item(icmp6opt_tree, hf_icmpv6_rpl_opt_config_rsv, tvb, optoffset, 1, FALSE);
		optoffset++;
		proto_tree_add_item(icmp6opt_tree, hf_icmpv6_rpl_opt_config_def_lifetime, tvb, optoffset, 1, FALSE);
		optoffset++;
		proto_tree_add_item(icmp6opt_tree, hf_icmpv6_rpl_opt_config_lifetime_unit, tvb, optoffset, 2, FALSE);
		optoffset += 2;
		break;
	    }

	    case RPL_OPT_TARGET: {
		guint8              target_len;
		struct e_in6_addr   target;

		/* Target */
		proto_tree_add_item(icmp6opt_tree, hf_icmpv6_rpl_opt_target_reserved, tvb, optoffset, 1, FALSE);
		optoffset++;

		/* Prefix length */
		target_len = tvb_get_guint8(tvb, optoffset);
		ti_plen = proto_tree_add_uint(icmp6opt_tree, hf_icmpv6_rpl_opt_target_length, tvb, optoffset, 1, target_len);
		optoffset++;
		if (target_len > (sizeof(target) << 3)) {
		    /* Illegal prefix length! Must prevent evil buffer overflows >:@ */
		    target_len = sizeof(target) << 3;
		    expert_add_info_format(pinfo, ti_plen, PI_MALFORMED, PI_ERROR, "Target length invalid, greather than 128 bits");
		}

		/* Prefix */
		memset(&target, 0, sizeof(target));
		tvb_memcpy(tvb, &target, optoffset, (target_len + 7) >> 3);         /* Round up to the nearest octet. */
		if (target_len & 0x7)
		    target.bytes[target_len >> 3] &= 0xff << (target_len & 0x7);    /* Clear unused bits in the last octet. */
		proto_tree_add_ipv6(icmp6opt_tree, hf_icmpv6_rpl_opt_target, tvb, optoffset, ((target_len + 7) >> 3), target.bytes);
		optoffset += ((target_len + 7) >> 3);
		break;
	    }

	    case RPL_OPT_TRANSIT: {

		/* flags */
		guint8 flags = tvb_get_guint8(tvb, optoffset);
		proto_tree_add_boolean(icmp6opt_tree, hf_icmpv6_rpl_opt_transit_e, tvb, optoffset, 1, flags & RPL_OPT_TRANSIT_E);
		proto_tree_add_uint(icmp6opt_tree, hf_icmpv6_rpl_opt_transit_flags, tvb, optoffset, 1, flags & RPL_OPT_TRANSIT_FLAGS);
		optoffset++;

		/* Path Control */
		proto_tree_add_item(icmp6opt_tree, hf_icmpv6_rpl_opt_transit_pathctl, tvb, optoffset, 1, FALSE);
		optoffset++;

		/* Path Sequence */
		proto_tree_add_item(icmp6opt_tree, hf_icmpv6_rpl_opt_transit_pathseq, tvb, optoffset, 1, FALSE);
		optoffset++;

		/* Path Lifetime */
		proto_tree_add_item(icmp6opt_tree, hf_icmpv6_rpl_opt_transit_pathlifetime, tvb, optoffset, 1, FALSE);
		optoffset++;

		/* Option contains parent */
		if(optlen > 4)
		{
		   proto_tree_add_item(icmp6opt_tree, hf_icmpv6_rpl_opt_transit_parent, tvb, optoffset, 16, FALSE);
		   optoffset += 16;
		}

		break;
	    }

	    case RPL_OPT_SOLICITED: {
		guint8 flags;

		/* Instance ID */
		proto_tree_add_item(icmp6opt_tree, hf_icmpv6_rpl_opt_solicited_instance, tvb, optoffset, 1, FALSE);
		optoffset++;

		/* flags */
		flags = tvb_get_guint8(tvb, optoffset);
		proto_tree_add_boolean(icmp6opt_tree, hf_icmpv6_rpl_opt_solicited_flag_v, tvb, optoffset, 1, flags & RPL_OPT_SOLICITED_V);
		proto_tree_add_boolean(icmp6opt_tree, hf_icmpv6_rpl_opt_solicited_flag_i, tvb, optoffset, 1, flags & RPL_OPT_SOLICITED_I);
		proto_tree_add_boolean(icmp6opt_tree, hf_icmpv6_rpl_opt_solicited_flag_d, tvb, optoffset, 1, flags & RPL_OPT_SOLICITED_D);
		optoffset++;

		/* DODAG ID */
		proto_tree_add_item(icmp6opt_tree, hf_icmpv6_rpl_opt_solicited_dodagid, tvb, optoffset, 16, FALSE);
		optoffset += 16;

		/* Version Number */
		proto_tree_add_item(icmp6opt_tree, hf_icmpv6_rpl_opt_solicited_version, tvb, optoffset, 1, FALSE);
		optoffset++;

		break;
	    }

	    case RPL_OPT_PREFIX: {
		/* Destination prefix option. */
		guint8              prefix_len;
		struct e_in6_addr   prefix;

		/* Prefix length */
		prefix_len = tvb_get_guint8(tvb, optoffset);
		ti_plen = proto_tree_add_uint(icmp6opt_tree, hf_icmpv6_rpl_opt_prefix_length, tvb, optoffset, 1, prefix_len);
		optoffset++;
		if (prefix_len > (sizeof(prefix) << 3)) {
		    /* Illegal prefix length! Must prevent evil buffer overflows >:@ */
		    prefix_len = sizeof(prefix) << 3;
		    expert_add_info_format(pinfo, ti_plen, PI_MALFORMED, PI_ERROR, "Prefix length invalid, greather than 128 bits");
		}

		/* Flags. */
		proto_tree_add_item(icmp6opt_tree, hf_icmpv6_rpl_opt_prefix_flag_l, tvb, optoffset, 1, FALSE);
		proto_tree_add_item(icmp6opt_tree, hf_icmpv6_rpl_opt_prefix_flag_a, tvb, optoffset, 1, FALSE);
		proto_tree_add_item(icmp6opt_tree, hf_icmpv6_rpl_opt_prefix_flag_r, tvb, optoffset, 1, FALSE);
		optoffset++;

		/* Valid lifetime. */
		proto_tree_add_item(icmp6opt_tree, hf_icmpv6_rpl_opt_prefix_vlifetime, tvb, optoffset, 4, FALSE);
		optoffset += 4;

		/* Preferrred Lifetime */
		proto_tree_add_item(icmp6opt_tree, hf_icmpv6_rpl_opt_prefix_plifetime, tvb, optoffset, 4, FALSE);
		optoffset += 4;

		/* 4 reserved bytes. */
		optoffset += 4;

		/* Prefix */
		memset(&prefix, 0, sizeof(prefix));
		tvb_memcpy(tvb, &prefix, optoffset, (prefix_len + 7) >> 3);         /* Round up to the nearest octet. */
		if (prefix_len & 0x7)
		    prefix.bytes[prefix_len >> 3] &= 0xff << (prefix_len & 0x7);    /* Clear unused bits in the last octet. */
		proto_tree_add_ipv6(icmp6opt_tree, hf_icmpv6_rpl_opt_prefix, tvb, optoffset, ((prefix_len + 7) >> 3), prefix.bytes);
		optoffset += ((prefix_len + 7) >> 3); /* Round up to the nearest 8 bytes. */
		break;
	    }

	    case RPL_OPT_TARGETDESC: {

		/* Descriptor */
		proto_tree_add_item(icmp6opt_tree, hf_icmpv6_rpl_opt_targetdesc, tvb, optoffset, 4, FALSE);
		optoffset += 4;
		break;

	    }

	    default:
		break;
	} /* switch */

	/* Get the next option. */
	offset += optlen;
    }
}

/*
 * draft-ietf-ipngwg-icmp-name-lookups-07.txt
 * Note that the packet format was changed several times in the past.
 */

static void
bitrange0(guint32 v, int s, emem_strbuf_t *strbuf)
{
    guint32 v0;
    int off;
    int i;

    v0 = v;
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
            ep_strbuf_append_printf(strbuf, ",%d", s + off);
        else {
            ep_strbuf_append_printf(strbuf, ",%d-%d", s + off,
                                    s + off + i - 1);
        }
        v >>= i; off += i;
    }
}

static const char *
bitrange(tvbuff_t *tvb, int offset, int l, int s)
{
    emem_strbuf_t *strbuf;
    int i;

    strbuf = ep_strbuf_new_label(NULL);
    for (i = 0; i < l; i++)
        bitrange0(tvb_get_ntohl(tvb, offset + i * 4), s + i * 4, strbuf);

    return strbuf->str + 1;     /* skip initial "," */
}

#define NI_SIZE 16
#define NI_FLAGS_SIZE 2
#define NI_FLAGS_OFFSET 6
#define NI_NONCE_SIZE 8
#define NI_NONCE_OFFSET 8
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
    const guchar *dname;
    guint32 ipaddr;

    ni = &icmp6_nodeinfo;
    tvb_memcpy(tvb, (guint8 *)ni, offset, sizeof *ni);
    /* flags */
    flags = pntohs(&ni->ni_flags);
    tf = proto_tree_add_text(tree, tvb,
                             offset + NI_FLAGS_OFFSET,
                             NI_FLAGS_SIZE, "Flags: 0x%04x", flags);
    field_tree = proto_item_add_subtree(tf, ett_nodeinfo_flag);
    switch (pntohs(&ni->ni_qtype)) {
    case NI_QTYPE_SUPTYPES:
        if (ni->ni_type == ICMP6_NI_QUERY) {
            proto_tree_add_text(field_tree, tvb,
                                offset + NI_FLAGS_OFFSET,
                                NI_FLAGS_SIZE, "%s",
                                decode_boolean_bitfield(flags, NI_SUPTYPE_FLAG_COMPRESS, sizeof(flags) * 8,
                                                        "Compressed reply supported",
                                                        "No compressed reply support"));
        } else {
            proto_tree_add_text(field_tree, tvb,
                                offset + NI_FLAGS_OFFSET,
                                NI_FLAGS_SIZE, "%s",
                                decode_boolean_bitfield(flags, NI_SUPTYPE_FLAG_COMPRESS, sizeof(flags) * 8,
                                                        "Compressed", "Not compressed"));
        }
        break;
    case NI_QTYPE_DNSNAME:
        if (ni->ni_type == ICMP6_NI_REPLY) {
            proto_tree_add_text(field_tree, tvb,
                                offset + NI_FLAGS_OFFSET,
                                NI_FLAGS_SIZE, "%s",
                                decode_boolean_bitfield(flags, NI_FQDN_FLAG_VALIDTTL, sizeof(flags) * 8,
                                                        "Valid TTL field", "Meaningless TTL field"));
        }
        break;
    case NI_QTYPE_NODEADDR:
        proto_tree_add_text(field_tree, tvb,
                            offset + NI_FLAGS_OFFSET,
                            NI_FLAGS_SIZE, "%s",
                            decode_boolean_bitfield(flags, NI_NODEADDR_FLAG_GLOBAL, sizeof(flags) * 8,
                                                    "Global address",
                                                    "Not global address"));
        proto_tree_add_text(field_tree, tvb,
                            offset + NI_FLAGS_OFFSET,
                            NI_FLAGS_SIZE, "%s",
                            decode_boolean_bitfield(flags, NI_NODEADDR_FLAG_SITELOCAL, sizeof(flags) * 8,
                                                    "Site-local address",
                                                    "Not site-local address"));
        proto_tree_add_text(field_tree, tvb,
                            offset + NI_FLAGS_OFFSET,
                            NI_FLAGS_SIZE, "%s",
                            decode_boolean_bitfield(flags, NI_NODEADDR_FLAG_LINKLOCAL, sizeof(flags) * 8,
                                                    "Link-local address",
                                                    "Not link-local address"));
        proto_tree_add_text(field_tree, tvb,
                            offset + NI_FLAGS_OFFSET,
                            NI_FLAGS_SIZE, "%s",
                            decode_boolean_bitfield(flags, NI_NODEADDR_FLAG_COMPAT, sizeof(flags) * 8,
                                                    "IPv4 compatible/mapped address",
                                                    "Not IPv4 compatible/mapped address"));
        /* fall through */
    case NI_QTYPE_IPV4ADDR:
        proto_tree_add_text(field_tree, tvb,
                            offset + NI_FLAGS_OFFSET,
                            NI_FLAGS_SIZE, "%s",
                            decode_boolean_bitfield(flags, NI_NODEADDR_FLAG_ALL, sizeof(flags) * 8,
                                                    "All unicast address",
                                                    "Unicast addresses on the queried interface"));
        proto_tree_add_text(field_tree, tvb,
                            offset + NI_FLAGS_OFFSET,
                            NI_FLAGS_SIZE, "%s",
                            decode_boolean_bitfield(flags, NI_NODEADDR_FLAG_TRUNCATE, sizeof(flags) * 8,
                                                    "Truncated", "Not truncated"));
        break;
    } /* switch (pntohs(&ni->ni_qtype)) */

    /* nonce */
    proto_tree_add_text(tree, tvb,
                        offset + NI_NONCE_OFFSET,
                        NI_NONCE_SIZE, "Nonce: 0x%08x%08x",
                        pntohl(&ni->icmp6_ni_nonce[0]), pntohl(&ni->icmp6_ni_nonce[4]));

    /* offset for "the rest of data" */
    off = NI_SIZE;

    /* rest of data */
    if (!tvb_bytes_exist(tvb, offset, sizeof(*ni)))
        goto nodata;
    if (ni->ni_type == ICMP6_NI_QUERY) {
        switch (ni->ni_code) {
        case ICMP6_NI_SUBJ_IPV6:
            n = tvb_reported_length_remaining(tvb, offset + sizeof(*ni));
            n /= sizeof(struct e_in6_addr);
            tf = proto_tree_add_text(tree, tvb,
                                     offset + sizeof(*ni), -1, "IPv6 subject addresses");
            field_tree = proto_item_add_subtree(tf, ett_nodeinfo_subject6);
            p = offset + sizeof *ni;
            for (i = 0; i < n; i++) {
                struct e_in6_addr e_in6_addr;
                tvb_get_ipv6(tvb, p, &e_in6_addr);
                proto_tree_add_text(field_tree, tvb,
                                    p, sizeof(struct e_in6_addr),
                                    "%s", ip6_to_str(&e_in6_addr));
                p += sizeof(struct e_in6_addr);
            }
            off = tvb_length_remaining(tvb, offset);
            break;
        case ICMP6_NI_SUBJ_FQDN:
            /* XXX Fix data length */
            l = get_dns_name(tvb, offset + sizeof(*ni), 0,
                             offset + sizeof(*ni), &dname);
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
            n = tvb_reported_length_remaining(tvb, offset + sizeof(*ni));
            n /= sizeof(guint32);
            tf = proto_tree_add_text(tree, tvb,
                                     offset + sizeof(*ni), -1, "IPv4 subject addresses");
            field_tree = proto_item_add_subtree(tf, ett_nodeinfo_subject4);
            p = offset + sizeof *ni;
            for (i = 0; i < n; i++) {
                ipaddr = tvb_get_ipv4(tvb, p);
                proto_tree_add_text(field_tree, tvb,
                                    p, sizeof(guint32), "%s", ip_to_str((guint8 *)&ipaddr));
                p += sizeof(guint32);
            }
            off = tvb_length_remaining(tvb, offset);
            break;
        } /* switch (ni->ni_code) */
    } else {
        switch (pntohs(&ni->ni_qtype)) {
        case NI_QTYPE_NOOP:
            break;
        case NI_QTYPE_SUPTYPES:
            p = offset + sizeof *ni;
            tf = proto_tree_add_text(tree, tvb,
                                     offset + sizeof(*ni), -1,
                                     "Supported type bitmap%s",
                                     (flags & 0x0001) ? ", compressed" : "");
            field_tree = proto_item_add_subtree(tf,
                                                ett_nodeinfo_nodebitmap);
            n = 0;
            while (tvb_bytes_exist(tvb, p, sizeof(guint32))) { /* XXXX Check what? */
                if ((flags & 0x0001) == 0) {
                    l = tvb_reported_length_remaining(tvb, offset + sizeof(*ni));
                    l /= sizeof(guint32);
                    i = 0;
                } else {
                    l = tvb_get_ntohs(tvb, p);
                    i = tvb_get_ntohs(tvb, p + sizeof(guint16));        /*skip*/
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
                                     offset + sizeof(*ni) + sizeof(guint32),    -1,
                                     "DNS labels");
            field_tree = proto_item_add_subtree(tf, ett_nodeinfo_nodedns);
            j = offset + sizeof (*ni) + sizeof(guint32);
            while (j < tvb_reported_length(tvb)) {
                /* XXX Fix data length */
                l = get_dns_name(tvb, j, 0,
                                 offset + sizeof (*ni) + sizeof(guint32),
                                 &dname);
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
            n = tvb_reported_length_remaining(tvb, offset + sizeof(*ni));
            n /= sizeof(gint32) + sizeof(struct e_in6_addr);
            tf = proto_tree_add_text(tree, tvb,
                                     offset + sizeof(*ni), -1, "IPv6 node addresses");
            field_tree = proto_item_add_subtree(tf, ett_nodeinfo_node6);
            p = offset + sizeof (*ni);
            for (i = 0; i < n; i++) {
                struct e_in6_addr e_in6_addr;
                gint32 ttl;
                ttl = (gint32)tvb_get_ntohl(tvb, p);
                tvb_get_ipv6(tvb, p + sizeof ttl, &e_in6_addr);
                proto_tree_add_text(field_tree, tvb,
                                    p, sizeof(struct e_in6_addr) + sizeof(gint32),
                                    "%s (TTL %d)", ip6_to_str(&e_in6_addr), ttl);
                p += sizeof(struct e_in6_addr) + sizeof(gint32);
            }
            off = tvb_length_remaining(tvb, offset);
            break;
        case NI_QTYPE_IPV4ADDR:
            n = tvb_reported_length_remaining(tvb, offset + sizeof(*ni));
            n /= sizeof(gint32) + sizeof(guint32);
            tf = proto_tree_add_text(tree, tvb,
                                     offset + sizeof(*ni), -1, "IPv4 node addresses");
            field_tree = proto_item_add_subtree(tf, ett_nodeinfo_node4);
            p = offset + sizeof *ni;
            for (i = 0; i < n; i++) {
                ipaddr = tvb_get_ipv4(tvb, sizeof(gint32) + p);
                proto_tree_add_text(field_tree, tvb,
                                    p, sizeof(guint32), "%s (TTL %d)",
                                    ip_to_str((guint8 *)&ipaddr), tvb_get_ntohl(tvb, p));
                p += sizeof(gint32) + sizeof(guint32);
            }
            off = tvb_length_remaining(tvb, offset);
            break;
        } /* switch (pntohs(&ni->ni_qtype)) */
    }
nodata:;

    /* the rest of data */
    call_dissector(data_handle,tvb_new_subset_remaining(tvb, offset + off), pinfo, tree);
}

#define RR_SIZE 16
#define RR_SEQNUM_SIZE 4
#define RR_SEQNUM_OFFSET 4
#define RR_SEGNUM_SIZE 1
#define RR_SEGNUM_OFFSET 8
#define RR_FLAGS_SIZE 1
#define RR_FLAGS_OFFSET 9
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
                        offset + RR_SEQNUM_OFFSET, RR_SEQNUM_SIZE,
                        "Sequence number: 0x%08x", pntohl(&rr->rr_seqnum));
    proto_tree_add_text(tree, tvb,
                        offset + RR_SEGNUM_OFFSET, RR_SEGNUM_SIZE,
                        "Segment number: 0x%02x", rr->rr_segnum);

    flagoff = offset + RR_FLAGS_OFFSET;
    flags = tvb_get_guint8(tvb, flagoff);
    tf = proto_tree_add_text(tree, tvb, flagoff, RR_FLAGS_SIZE,
                             "Flags: 0x%02x", flags);
    field_tree = proto_item_add_subtree(tf, ett_icmpv6flag);
    proto_tree_add_text(field_tree, tvb, flagoff, RR_FLAGS_SIZE, "%s",
                        decode_boolean_bitfield(flags, 0x80, 8,
                                                "Test command", "Not test command"));
    proto_tree_add_text(field_tree, tvb, flagoff, RR_FLAGS_SIZE, "%s",
                        decode_boolean_bitfield(flags, 0x40, 8,
                                                "Result requested", "Result not requested"));
    proto_tree_add_text(field_tree, tvb, flagoff, RR_FLAGS_SIZE, "%s",
                        decode_boolean_bitfield(flags, 0x20, 8,
                                                "All interfaces", "Not all interfaces"));
    proto_tree_add_text(field_tree, tvb, flagoff, RR_FLAGS_SIZE, "%s",
                        decode_boolean_bitfield(flags, 0x10, 8,
                                                "Site specific", "Not site specific"));
    proto_tree_add_text(field_tree, tvb, flagoff, RR_FLAGS_SIZE, "%s",
                        decode_boolean_bitfield(flags, 0x08, 8,
                                                "Processed previously", "Complete result"));

    proto_tree_add_text(tree, tvb,
                        offset + offsetof(struct icmp6_router_renum, rr_maxdelay), 2,
                        "Max delay: 0x%04x", pntohs(&rr->rr_maxdelay));
    call_dissector(data_handle,tvb_new_subset_remaining(tvb, offset + RR_SIZE), pinfo, tree);   /*XXX*/

    if (rr->rr_code == ICMP6_ROUTER_RENUMBERING_COMMAND) {
        off = offset + RR_SIZE;
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

/*
 * See I-D draft-vida-mld-v2-08
 */
static const value_string mldrv2ModesNames[] = {
    { 1, "Include" },
    { 2, "Exclude" },
    { 3, "Changed to include" },
    { 4, "Changed to exclude" },
    { 5, "Allow new sources" },
    { 6, "Block old sources" },
    { 0, NULL }
};

static void
dissect_mldrv2( tvbuff_t *tvb, guint32 offset, guint16 count, proto_tree *tree )
{
    proto_tree *sub_tree;
    proto_item *tf;

    guint8 recordType, auxDataLen;
    guint32 sourceNb, recordSize, localOffset;
    struct e_in6_addr addr;

    for( ; count; count--, offset += recordSize ) {
        localOffset = offset;
        recordType = tvb_get_guint8( tvb, localOffset );
        localOffset += 1;
        auxDataLen = tvb_get_guint8( tvb, localOffset );
        localOffset += 1;
        sourceNb = tvb_get_ntohs( tvb, localOffset );
        localOffset += 2;
        recordSize = 4 + 16 + (16 * sourceNb) + (auxDataLen * 4);

        tvb_get_ipv6(tvb, localOffset, &addr);
        tf = proto_tree_add_text( tree, tvb, offset, recordSize,
                                  "%s: %s (%s)", val_to_str(recordType, mldrv2ModesNames,"Unknown mode"),
                                  get_hostname6(&addr), ip6_to_str(&addr)
            );
        sub_tree = proto_item_add_subtree(tf, ett_multicastRR);

        proto_tree_add_text( sub_tree, tvb, offset,   1, "Mode: %s (%u)",
                             val_to_str(recordType, mldrv2ModesNames,"Unknown mode"), recordType );
        proto_tree_add_text( sub_tree, tvb, offset+1, 1, "Aux data len: %u", auxDataLen * 4);
        proto_tree_add_text( sub_tree, tvb, offset+2, 2, "Number of Sources: %u", sourceNb);
        proto_tree_add_text( sub_tree, tvb, localOffset, 16, "Multicast Address: %s", ip6_to_str(&addr) );
        localOffset += 16;

        for( ; sourceNb; sourceNb--, localOffset += 16 ) {
            tvb_get_ipv6(tvb, localOffset, &addr);
            proto_tree_add_text( sub_tree, tvb, localOffset, 16,
                                 "Source Address: %s (%s)", get_hostname6(&addr), ip6_to_str(&addr)
                );
        }
    }
}

static void
dissect_mldqv2(tvbuff_t *tvb, guint32 offset, guint16 count, proto_tree *tree)
{
    struct e_in6_addr addr;

    for ( ; count; count--, offset += 16) {
        tvb_get_ipv6(tvb, offset, &addr);
        proto_tree_add_text(tree, tvb, offset, 16,
                            "Source Address: %s (%s)", get_hostname6(&addr), ip6_to_str(&addr));
    }
}

static void
dissect_icmpv6(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
    proto_tree *icmp6_tree, *field_tree;
    proto_item *ti, *hidden_item, *tf = NULL;
    struct icmp6_hdr icmp6_hdr, *dp;
    const char *codename, *typename;
    const char *colcodename, *coltypename;
    int len;
    guint length, reported_length;
    vec_t cksum_vec[4];
    guint32 phdr[2];
    guint16 cksum, computed_cksum;
    int offset;
    tvbuff_t *next_tvb;

    col_set_str(pinfo->cinfo, COL_PROTOCOL, "ICMPv6");
    col_clear(pinfo->cinfo, COL_INFO);

    offset = 0;
    tvb_memcpy(tvb, (guint8 *)&icmp6_hdr, offset, sizeof icmp6_hdr);
    dp = &icmp6_hdr;
    typename = coltypename = val_to_str (dp->icmp6_type, icmpv6_type_str, "Unknown");
    codename = colcodename = NULL;

    len = sizeof(*dp);
    switch (dp->icmp6_type) {
    case ICMP6_DST_UNREACH:
        codename = colcodename = val_to_str (dp->icmp6_code, icmpv6_unreach_code_str, "Unknown");
        break;
    case ICMP6_TIME_EXCEEDED:
        codename = colcodename = val_to_str (dp->icmp6_code, icmpv6_timeex_code_str, "Unknown");
        break;
    case ICMP6_PARAM_PROB:
        codename = colcodename = val_to_str (dp->icmp6_code, icmpv6_paramprob_code_str, "Unknown");
        break;
    case ND_ROUTER_SOLICIT:
        len = sizeof(struct nd_router_solicit);
        break;
    case ND_ROUTER_ADVERT:
        len = sizeof(struct nd_router_advert);
        break;
    case ND_NEIGHBOR_SOLICIT:
        len = sizeof(struct nd_neighbor_solicit);
        break;
    case ND_NEIGHBOR_ADVERT:
        len = sizeof(struct nd_neighbor_advert);
        break;
    case ND_REDIRECT:
        len = sizeof(struct nd_redirect);
        break;
    case ICMP6_ROUTER_RENUMBERING:
        codename = colcodename = val_to_str (dp->icmp6_code, icmpv6_router_renum_code_str, "Unknown");
        len = sizeof(struct icmp6_router_renum);
        break;
    case ICMP6_NI_QUERY:
    case ICMP6_NI_REPLY:
    {
        struct icmp6_nodeinfo icmp6_nodeinfo, *ni;

        ni = &icmp6_nodeinfo;
        tvb_memcpy(tvb, (guint8 *)ni, offset, sizeof *ni);

        if (ni->ni_type == ICMP6_NI_QUERY) {
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
            default:
                codename = "Unknown";
                break;
            }
        } else {
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
            default:
                codename = "Unknown";
                break;
            }
        }
        colcodename = val_to_str(pntohs(&ni->ni_qtype), names_nodeinfo_qtype, "Unknown");
        len = sizeof(struct icmp6_nodeinfo);
        break;
    }
    case ICMP6_ECHO_REQUEST:
    case ICMP6_ECHO_REPLY:
    case ICMP6_MEMBERSHIP_REPORT:
    case ICMP6_MIP6_DHAAD_REQUEST:
    case ICMP6_MIP6_DHAAD_REPLY:
    case ICMP6_MIP6_MPS:
    case ICMP6_MIP6_MPA:
    case ICMP6_CERT_PATH_SOL:
    case ICMP6_CERT_PATH_AD:
    case ICMP6_MLDV2_REPORT:
        codename = "Should always be zero";
        break;
    case ICMP6_EXPERIMENTAL_MOBILITY:
        switch (dp->icmp6_data8[0]) {
        case FMIP6_SUBTYPE_RTSOLPR:
            typename = coltypename = "RtSolPr (ICMPv6 Experimental Mobility)";
            codename = "Should always be zero";
            break;
        case FMIP6_SUBTYPE_PRRTADV:
            typename = coltypename = "PrRtAdv (ICMPv6 Experimental Mobility)";
            codename = val_to_str(dp->icmp6_code, names_fmip6_prrtadv_code, "Unknown");
            break;
        case FMIP6_SUBTYPE_HI:
            typename = coltypename = "HI (ICMPv6 Experimental Mobility)";
            codename = val_to_str(dp->icmp6_code, names_fmip6_hi_code, "Unknown");
            break;
        case FMIP6_SUBTYPE_HACK:
            typename = coltypename = "HAck (ICMPv6 Experimental Mobility)";
            codename = val_to_str(dp->icmp6_code, names_fmip6_hack_code, "Unknown");
            break;
        default:
            codename = colcodename = "Unknown";
            break;
        } /* switch (dp->icmp6_data8[0]) */
        break;
    case ICMP6_RPL_CONTROL:
        codename = colcodename = val_to_str (dp->icmp6_code, names_rpl_code, "Unknown");
        break;
    default:
        codename = colcodename = "Unknown";
        break;
    } /* switch (dp->icmp6_type) */

    if (check_col(pinfo->cinfo, COL_INFO)) {
        char typebuf[256], codebuf[256];


        if (pinfo->destport == 0x0dd8 && dp->icmp6_type == ICMP6_ECHO_REQUEST) {
            /* RFC 4380
             * 5.2.9. Direct IPv6 Connectivity Test
             */
            col_set_str(pinfo->cinfo, COL_PROTOCOL, "Teredo");
            col_set_str(pinfo->cinfo, COL_INFO, "Direct IPv6 Connectivity Test");
        } else {
            if (coltypename && strcmp(coltypename, "Unknown") == 0) {
                g_snprintf(typebuf, sizeof(typebuf), "Unknown (0x%02x)",
                           dp->icmp6_type);
                coltypename = typebuf;
            }
            if (colcodename && strcmp(colcodename, "Unknown") == 0) {
                g_snprintf(codebuf, sizeof(codebuf), "Unknown (0x%02x)",
                           dp->icmp6_code);
                colcodename = codebuf;
            }
            if (colcodename) {
                col_add_fstr(pinfo->cinfo, COL_INFO, "%s (%s)", coltypename, colcodename);
            } else {
                col_add_str(pinfo->cinfo, COL_INFO, coltypename);
            }
        }
    }

    if (tree) {
        /* !!! specify length */
        ti = proto_tree_add_item(tree, proto_icmpv6, tvb, offset, -1, FALSE);
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
        cksum = (guint16)g_htons(dp->icmp6_cksum);
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
            phdr[0] = g_htonl(tvb_reported_length(tvb));
            phdr[1] = g_htonl(IP_PROTO_ICMPV6);
            cksum_vec[2].len = 8;
            cksum_vec[3].len = tvb_reported_length(tvb);
            cksum_vec[3].ptr = tvb_get_ptr(tvb, offset, cksum_vec[3].len);
            computed_cksum = in_cksum(cksum_vec, 4);
            if (computed_cksum == 0) {
                proto_tree_add_uint_format(icmp6_tree, hf_icmpv6_checksum,
                                           tvb,
                                           offset + offsetof(struct icmp6_hdr, icmp6_cksum), 2,
                                           cksum,
                                           "Checksum: 0x%04x [correct]", cksum);
            } else {
                hidden_item = proto_tree_add_boolean(icmp6_tree, hf_icmpv6_checksum_bad,
                                                     tvb,
                                                     offset + offsetof(struct icmp6_hdr, icmp6_cksum), 2,
                                                     TRUE);
                PROTO_ITEM_SET_HIDDEN(hidden_item);
                proto_tree_add_uint_format(icmp6_tree, hf_icmpv6_checksum,
                                           tvb,
                                           offset + offsetof(struct icmp6_hdr, icmp6_cksum), 2,
                                           cksum,
                                           "Checksum: 0x%04x [incorrect, should be 0x%04x]",
                                           cksum, in_cksum_shouldbe(cksum, computed_cksum));
            }
        } else {
            proto_tree_add_uint(icmp6_tree, hf_icmpv6_checksum, tvb,
                                offset + offsetof(struct icmp6_hdr, icmp6_cksum), 2,
                                cksum);
        }

#define ICMP6_DATA_OFFSET 4
#define ICMP6_SEQ_OFFSET 6
        /* decode... */
        switch (dp->icmp6_type) {
        case ICMP6_DST_UNREACH:
        case ICMP6_TIME_EXCEEDED:
            dissect_contained_icmpv6(tvb, offset + sizeof(*dp), pinfo,
                                     icmp6_tree);
            break;
        case ICMP6_PACKET_TOO_BIG:
            proto_tree_add_text(icmp6_tree, tvb,
                                offset + ICMP6_DATA_OFFSET, 4,
                                "MTU: %u", pntohl(&dp->icmp6_mtu));
            dissect_contained_icmpv6(tvb, offset + sizeof(*dp), pinfo,
                                     icmp6_tree);
            break;
        case ICMP6_PARAM_PROB:
            proto_tree_add_text(icmp6_tree, tvb,
                                offset + ICMP6_DATA_OFFSET, 4,
                                "Problem pointer: 0x%04x", pntohl(&dp->icmp6_pptr));
            dissect_contained_icmpv6(tvb, offset + sizeof(*dp), pinfo,
                                     icmp6_tree);
            break;
        case ICMP6_ECHO_REQUEST:
        case ICMP6_ECHO_REPLY:
            proto_tree_add_text(icmp6_tree, tvb,
                                offset + ICMP6_DATA_OFFSET, 2,
                                "ID: 0x%04x", (guint16)g_ntohs(dp->icmp6_id));
            proto_tree_add_text(icmp6_tree, tvb,
                                offset + ICMP6_SEQ_OFFSET, 2,
                                "Sequence: %u", (guint16)g_ntohs(dp->icmp6_seq));
            col_append_fstr(pinfo->cinfo, COL_INFO,
                            " id=0x%04x, seq=%u",
                            g_ntohs(dp->icmp6_id), g_ntohs(dp->icmp6_seq));

            if (pinfo->destport == 0x0dd8 && dp->icmp6_type == ICMP6_ECHO_REQUEST) {
                /* RFC 4380
                 * 5.2.9. Direct IPv6 Connectivity Test
                 */
                proto_tree_add_text(icmp6_tree, tvb, offset + ICMP6_SEQ_OFFSET + 2, 4,
                                    "Nonce: 0x%08x", tvb_get_ntohl(tvb, offset + ICMP6_SEQ_OFFSET + 2));
            } else {
                next_tvb = tvb_new_subset(tvb, offset + sizeof(*dp), -1, -1);
                call_dissector(data_handle,next_tvb, pinfo, icmp6_tree);
            }
            break;
        case ICMP6_MEMBERSHIP_QUERY:
        case ICMP6_MEMBERSHIP_REPORT:
        case ICMP6_MEMBERSHIP_REDUCTION:
#define MLDV2_MINLEN 28
#define MLDV1_MINLEN 24
            if (dp->icmp6_type == ICMP6_MEMBERSHIP_QUERY) {
                if (length >= MLDV2_MINLEN) {
                    guint32 mrc;
                    guint16 qqi;
                    guint8 flag;
                    guint16 nsrcs;

                    mrc = g_ntohs(dp->icmp6_maxdelay);
                    flag = tvb_get_guint8(tvb, offset + sizeof(*dp) + 16);
                    qqi = tvb_get_guint8(tvb, offset + sizeof(*dp) + 16 + 1);
                    nsrcs = tvb_get_ntohs(tvb, offset + sizeof(*dp) + 16 + 2);

                    if (mrc >= 32768)
                        mrc = ((mrc & 0x0fff) | 0x1000) <<
                            (((mrc & 0x7000) >> 12) + 3);
                    proto_tree_add_text(icmp6_tree, tvb,
                                        offset + ICMP6_DATA_OFFSET, 2,
                                        "Maximum response delay[ms]: %u", mrc);

                    proto_tree_add_text(icmp6_tree, tvb, offset + sizeof(*dp),
                                        16, "Multicast Address: %s",
                                        ip6_to_str((const struct e_in6_addr *)(tvb_get_ptr(tvb,
                                                                                           offset + sizeof *dp, sizeof (struct e_in6_addr)))));

                    proto_tree_add_text(icmp6_tree, tvb,
                                        offset + sizeof(*dp) + 16, 1, "S Flag: %s",
                                        flag & 0x08 ? "ON" : "OFF");
                    proto_tree_add_text(icmp6_tree, tvb,
                                        offset + sizeof(*dp) + 16, 1, "Robustness: %d",
                                        flag & 0x07);
                    if (qqi >= 128)
                        qqi = ((qqi & 0x0f) | 0x10) << (((qqi & 0x70) >> 4) + 3);
                    proto_tree_add_text(icmp6_tree, tvb,
                                        offset + sizeof(*dp) + 17, 1, "QQI: %d", qqi);

                    dissect_mldqv2(tvb, offset + sizeof(*dp) + 20, nsrcs,
                                   icmp6_tree);
                    break;
                } else if (length > MLDV1_MINLEN) {
                    next_tvb = tvb_new_subset(tvb, offset + sizeof(*dp), -1, -1);
                    call_dissector(data_handle,next_tvb, pinfo, tree);
                    break;
                }
                /* MLDv1 Query -> FALLTHOUGH */
            } /* if (dp->icmp6_type == ICMP6_MEMBERSHIP_QUERY) */
#undef MLDV2_MINLEN
#undef MLDV1_MINLEN
            proto_tree_add_text(icmp6_tree, tvb,
                                offset + ICMP6_DATA_OFFSET, 2,
                                "Maximum response delay: %u",
                                (guint16)g_ntohs(dp->icmp6_maxdelay));
            proto_tree_add_text(icmp6_tree, tvb, offset + sizeof(*dp), 16,
                                "Multicast Address: %s",
                                ip6_to_str((const struct e_in6_addr *)(tvb_get_ptr(tvb, offset + sizeof *dp, sizeof (struct e_in6_addr)))));
            break;
        case ND_ROUTER_SOLICIT:
            dissect_icmpv6ndopt(tvb, offset + sizeof(*dp), pinfo, icmp6_tree);
            break;
        case ICMP6_MLDV2_REPORT:
        {
            guint16 nbRecords;

            nbRecords = tvb_get_ntohs( tvb, offset+4+2 );
            proto_tree_add_text( icmp6_tree, tvb, offset+4, 2, "Reserved: %d (Should always be zero)",
                                 tvb_get_ntohs (tvb, offset+4) );
            proto_tree_add_item( icmp6_tree, hf_icmpv6_nor, tvb, offset+4+2, 2, FALSE );
            dissect_mldrv2( tvb, offset+4+2+2, nbRecords, icmp6_tree );
            break;
        }
#define ND_RA_CURHOPLIMIT_OFFSET 4
#define ND_RA_FLAGS_RESERVED_OFFSET 5
#define ND_RA_ROUTER_LIFETIME_OFFSET 6
        case ND_ROUTER_ADVERT:
        {
            struct nd_router_advert nd_router_advert, *ra;
            int flagoff;
            guint32 ra_flags;

            ra = &nd_router_advert;
            tvb_memcpy(tvb, (guint8 *)ra, offset, sizeof *ra);

            /* Current hop limit */
            proto_tree_add_uint(icmp6_tree, hf_icmpv6_ra_cur_hop_limit, tvb,
                                offset + ND_RA_CURHOPLIMIT_OFFSET,
                                1, ra->nd_ra_curhoplimit);

            /* Flags */
            flagoff = offset + ND_RA_FLAGS_RESERVED_OFFSET;
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
            proto_tree_add_text(field_tree, tvb, flagoff, 1, "%s",
                                decode_boolean_bitfield(ra_flags,
                                                        ND_RA_FLAG_ND_PROXY, 8,
                                                        "Proxied", "Not Proxied"));

            /* Router lifetime */
            proto_tree_add_uint(icmp6_tree, hf_icmpv6_ra_router_lifetime, tvb,
                                offset + ND_RA_ROUTER_LIFETIME_OFFSET,
                                2, (guint16)g_ntohs(ra->nd_ra_router_lifetime));

            /* Reachable time */
            proto_tree_add_uint(icmp6_tree, hf_icmpv6_ra_reachable_time, tvb,
                                offset + offsetof(struct nd_router_advert, nd_ra_reachable), 4,
                                pntohl(&ra->nd_ra_reachable));

            /* Retrans timer */
            proto_tree_add_uint(icmp6_tree, hf_icmpv6_ra_retrans_timer, tvb,
                                offset + offsetof(struct nd_router_advert, nd_ra_retransmit), 4,
                                pntohl(&ra->nd_ra_retransmit));

            dissect_icmpv6ndopt(tvb, offset + sizeof(struct nd_router_advert), pinfo, icmp6_tree);
            break;
        }
        case ND_NEIGHBOR_SOLICIT:
        {
            struct nd_neighbor_solicit nd_neighbor_solicit, *ns;

            ns = &nd_neighbor_solicit;
            tvb_memcpy(tvb, (guint8 *)ns, offset, sizeof *ns);
            proto_tree_add_text(icmp6_tree, tvb, offset+4, 4, "Reserved: %d (Should always be zero)",
                                tvb_get_ntohl (tvb, offset+4));
            proto_tree_add_text(icmp6_tree, tvb,
                                offset + offsetof(struct nd_neighbor_solicit, nd_ns_target), 16,
                                "Target: %s (%s)",
                                get_hostname6(&ns->nd_ns_target),
                                ip6_to_str(&ns->nd_ns_target));
            col_append_fstr(pinfo->cinfo, COL_INFO,
                            " for %s", ip6_to_str(&ns->nd_ns_target));

            dissect_icmpv6ndopt(tvb, offset + sizeof(*ns), pinfo, icmp6_tree);
            break;
        }
#define ND_NA_FLAGS_RESERVED_OFFSET 4
        case ND_NEIGHBOR_ADVERT:
        {
            int flagoff, targetoff;
            guint32 na_flags;
            struct e_in6_addr na_target;
            emem_strbuf_t *flags_strbuf = ep_strbuf_new_label("");

            flagoff = offset + ND_NA_FLAGS_RESERVED_OFFSET;
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
                                "Target: %s (%s)",
                                get_hostname6(&na_target),
                                ip6_to_str(&na_target));
            if (na_flags & ND_NA_FLAG_ROUTER) {
                ep_strbuf_append(flags_strbuf, "rtr, ");
            }
            if (na_flags & ND_NA_FLAG_SOLICITED) {
                ep_strbuf_append(flags_strbuf, "sol, ");
            }
            if (na_flags & ND_NA_FLAG_OVERRIDE) {
                ep_strbuf_append(flags_strbuf, "ovr, ");
            }
            if (flags_strbuf->len > 2) {
                ep_strbuf_truncate(flags_strbuf, flags_strbuf->len - 2);
            } else {
                ep_strbuf_printf(flags_strbuf, "none");
            }
            col_append_fstr(pinfo->cinfo, COL_INFO,
                            " %s (%s)", ip6_to_str(&na_target), flags_strbuf->str);

            dissect_icmpv6ndopt(tvb, offset + sizeof(struct nd_neighbor_advert), pinfo, icmp6_tree);
            break;
        }
        case ND_REDIRECT:
        {
            struct nd_redirect nd_redirect, *rd;

            rd = &nd_redirect;
            tvb_memcpy(tvb, (guint8 *)rd, offset, sizeof *rd);
            proto_tree_add_text(icmp6_tree, tvb, offset+4, 4, "Reserved: %d (Should always be zero)",
                                tvb_get_ntohs (tvb, offset+4));
            proto_tree_add_text(icmp6_tree, tvb,
                                offset + offsetof(struct nd_redirect, nd_rd_target), 16,
                                "Target: %s (%s)",
                                get_hostname6(&rd->nd_rd_target),
                                ip6_to_str(&rd->nd_rd_target));

            proto_tree_add_text(icmp6_tree, tvb,
                                offset + offsetof(struct nd_redirect, nd_rd_dst), 16,
                                "Destination: %s (%s)",
                                get_hostname6(&rd->nd_rd_dst),
                                ip6_to_str(&rd->nd_rd_dst));

            dissect_icmpv6ndopt(tvb, offset + sizeof(*rd), pinfo, icmp6_tree);
            break;
        }
        case ICMP6_ROUTER_RENUMBERING:
            dissect_rrenum(tvb, offset, pinfo, icmp6_tree);
            break;
#define NI_QTYPE_OFFSET 4
        case ICMP6_NI_QUERY:
        case ICMP6_NI_REPLY:
        {
            struct icmp6_nodeinfo *ni = (struct icmp6_nodeinfo *)dp;
            proto_tree_add_text(icmp6_tree, tvb,
                                offset + NI_QTYPE_OFFSET,
                                sizeof(ni->ni_qtype),
                                "Query type: 0x%04x (%s)", pntohs(&ni->ni_qtype),
                                val_to_str(pntohs(&ni->ni_qtype), names_nodeinfo_qtype,
                                           "Unknown"));
            dissect_nodeinfo(tvb, offset, pinfo, icmp6_tree);
            break;
        }
        case ICMP6_MIP6_DHAAD_REQUEST:
            proto_tree_add_text(icmp6_tree, tvb,
                                offset + 4, 2, "Identifier: %d (0x%02x)",
                                tvb_get_ntohs(tvb, offset + 4),
                                tvb_get_ntohs(tvb, offset + 4));
            proto_tree_add_text(icmp6_tree, tvb,
                                offset + 6, 2, "Reserved: %d",
                                tvb_get_ntohs(tvb, offset + 6));
            break;
        case ICMP6_MIP6_DHAAD_REPLY:
            proto_tree_add_text(icmp6_tree, tvb,
                                offset + 4, 2, "Identifier: %d (0x%02x)",
                                tvb_get_ntohs(tvb, offset + 4),
                                tvb_get_ntohs(tvb, offset + 4));
            proto_tree_add_text(icmp6_tree, tvb,
                                offset + 6, 2, "Reserved: %d",
                                tvb_get_ntohs(tvb, offset + 6));
            /* Show all Home Agent Addresses */
            {
                int i, suboffset;
                int ha_num = (length - 8)/16;

                for (i = 0; i < ha_num; i++) {
                    suboffset = 16 * i;
                    proto_tree_add_ipv6(icmp6_tree, hf_icmpv6_haad_ha_addrs,
                                        tvb, offset + 8 + suboffset, 16,
                                        tvb_get_ptr(tvb, offset + 8 + suboffset, 16));
                }
            }
            break;
        case ICMP6_MIP6_MPS:
            proto_tree_add_text(icmp6_tree, tvb,
                                offset + 4, 2, "Identifier: %d (0x%02x)",
                                tvb_get_ntohs(tvb, offset + 4),
                                tvb_get_ntohs(tvb, offset + 4));
            proto_tree_add_text(icmp6_tree, tvb,
                                offset + 6, 2, "Reserved: %d",
                                tvb_get_ntohs(tvb, offset + 6));
            break;
        case ICMP6_MIP6_MPA:
            proto_tree_add_text(icmp6_tree, tvb,
                                offset + 4, 2, "Identifier: %d (0x%02x)",
                                tvb_get_ntohs(tvb, offset + 4),
                                tvb_get_ntohs(tvb, offset + 4));
            proto_tree_add_text(icmp6_tree, tvb,
                                offset + 6, 1, "%s",
                                decode_boolean_bitfield(tvb_get_guint8(tvb, offset + 6),
                                                        0x80, 8,
                                                        "Managed Address Configuration",
                                                        "No Managed Address Configuration"));
            proto_tree_add_text(icmp6_tree, tvb,
                                offset + 6, 1, "%s",
                                decode_boolean_bitfield(tvb_get_guint8(tvb, offset + 6),
                                                        0x40, 8,
                                                        "Other Stateful Configuration",
                                                        "No Other Stateful Configuration"));
            proto_tree_add_text(icmp6_tree, tvb,
                                offset + 7, 1, "Reserved: %d",
                                tvb_get_guint8(tvb, offset + 7));
            /* Show all options */
            dissect_icmpv6ndopt(tvb, offset + 8, pinfo, icmp6_tree);
            break;
        case ICMP6_EXPERIMENTAL_MOBILITY:
            switch (dp->icmp6_data8[0]) {
            case FMIP6_SUBTYPE_RTSOLPR:
            {
                struct fmip6_rtsolpr *rtsolpr;
                rtsolpr = (struct fmip6_rtsolpr*) dp;
                proto_tree_add_text(icmp6_tree, tvb,
                                    offset + 4, 1,
                                    "Subtype: Router Solicitation for Proxy Advertisement");
                proto_tree_add_text(icmp6_tree, tvb,
                                    offset + 6, 2,
                                    "Identifier: %d", pntohs(&rtsolpr->fmip6_rtsolpr_id));
                dissect_icmpv6fmip6opt(tvb, offset + sizeof(*dp), icmp6_tree);
                break;
            }
            case FMIP6_SUBTYPE_PRRTADV:
            {
                struct fmip6_prrtadv *prrtadv;
                prrtadv = (struct fmip6_prrtadv*) dp;
                proto_tree_add_text(icmp6_tree, tvb,
                                    offset + 4, 1,
                                    "Subtype: Proxy Router Advertisement");
                proto_tree_add_text(icmp6_tree, tvb,
                                    offset + 6, 2,
                                    "Identifier: %d", pntohs(&prrtadv->fmip6_prrtadv_id));
                dissect_icmpv6fmip6opt(tvb, offset + sizeof(*dp), icmp6_tree);
                break;
            }
            case FMIP6_SUBTYPE_HI:
            {
                struct fmip6_hi *hi;
                int flagoff;
                guint8 hi_flags;
                hi = (struct fmip6_hi*) dp;
                proto_tree_add_text(icmp6_tree, tvb,
                                    offset + 4, 1,
                                    "Subtype: Handover Initiate");

                flagoff = offset + 5;
                hi_flags = tvb_get_guint8(tvb, flagoff);
                tf = proto_tree_add_text(icmp6_tree, tvb, flagoff, 1, "Flags: 0x%02x", hi_flags);
                field_tree = proto_item_add_subtree(tf, ett_icmpv6flag);
                proto_tree_add_text(field_tree, tvb, flagoff, 1, "%s",
                                    decode_boolean_bitfield(hi_flags,
                                                            FMIP_HI_FLAG_ASSIGNED, 8, "Assigned", "Not assigned"));
                proto_tree_add_text(field_tree, tvb, flagoff, 1, "%s",
                                    decode_boolean_bitfield(hi_flags,
                                                            FMIP_HI_FLAG_BUFFER, 8, "Buffered", "Not buffered"));
                proto_tree_add_text(icmp6_tree, tvb,
                                    offset + 6, 2,
                                    "Identifier: %d", pntohs(&hi->fmip6_hi_id));
                dissect_icmpv6fmip6opt(tvb, offset + sizeof(*dp), icmp6_tree);
                break;
            }
            case FMIP6_SUBTYPE_HACK:
            {
                struct fmip6_hack *hack;
                hack = (struct fmip6_hack*) dp;
                proto_tree_add_text(icmp6_tree, tvb,
                                    offset + 4, 1,
                                    "Subtype: Handover Acknowledge");
                proto_tree_add_text(icmp6_tree, tvb,
                                    offset + 6, 2,
                                    "Identifier: %d", pntohs(&hack->fmip6_hack_id));
                dissect_icmpv6fmip6opt(tvb, offset + sizeof(*dp), icmp6_tree);
                break;
            }
            } /* switch (dp->icmp6_data8[0]) */
            break;
        case ICMP6_CERT_PATH_SOL:
            /*RFC 3971 6.4.1.  Certification Path Solicitation Message Format */
            offset += 4;
            proto_tree_add_text(icmp6_tree, tvb, offset, -1,
                                "Certification Path Solicitation Message");
            /* Identifier A 16-bit unsigned integer field */
            proto_tree_add_item(icmp6_tree, hf_icmpv6_identifier, tvb, offset,
                                2, FALSE);
            offset += 2;
            /* Component A 16-bit unsigned integer field,
             * 65,535 if the sender seeks to retrieve all certificates.
             * Otherwise, set to the identifier that the receiver wants.
             */
            proto_tree_add_item(icmp6_tree, hf_icmpv6_comp, tvb, offset, 2,
                                FALSE);
            offset += 2;
            dissect_icmpv6ndopt(tvb, offset, pinfo, icmp6_tree);
            break;
        case ICMP6_CERT_PATH_AD:
            /*RFC 3971 6.4.2.  Certification Path Advertisement Message Format */
            offset = offset +4;
            proto_tree_add_text(icmp6_tree, tvb, offset, -1,"Certification Path Advertisement Message");

            /* Identifier A 16-bit unsigned integer field */
            proto_tree_add_item(icmp6_tree, hf_icmpv6_identifier, tvb, offset, 2, FALSE);
            offset = offset + 2;
            /* All Components  A 16-bit unsigned integer field*/
            proto_tree_add_item(icmp6_tree, hf_icmpv6_all_comp, tvb, offset, 2, FALSE);
            offset = offset + 2;

            /* Component A 16-bit unsigned integer field, used to inform the receiver
             * which certificate is being sent.
             */
            proto_tree_add_item(icmp6_tree, hf_icmpv6_comp, tvb, offset, 2, FALSE);
            offset = offset + 2;

            /* Reserved */
            proto_tree_add_text(icmp6_tree, tvb, offset, 2,"Reserved");
            offset = offset + 2;

            dissect_icmpv6ndopt(tvb, offset, pinfo, icmp6_tree);
            break;
        case ICMP6_RPL_CONTROL:
            /* RPL: draft-ietf-roll-rpl-12.txt: Routing over Low-Power and Lossy Networks. */
            offset += 4; /* Skip the type, code and checksum. */
            if (dp->icmp6_code == ICMP6_RPL_DIS) {

                /* Reserved */
                proto_tree_add_item(icmp6_tree, hf_icmpv6_dis_reserved, tvb, offset, 2, FALSE);
                offset += 2;

            }
            else if (dp->icmp6_code == ICMP6_RPL_DIO) {
                /* DODAG Information Object */
                guint8  flags;

                /* RPLInstanceID */
                proto_tree_add_item(icmp6_tree, hf_icmpv6_dio_instance, tvb, offset, 1, FALSE);
                offset += 1;

                /* RPLInstanceID */
                proto_tree_add_item(icmp6_tree, hf_icmpv6_dio_version, tvb, offset, 1, FALSE);
                offset += 1;

                /* Rank */
                proto_tree_add_item(icmp6_tree, hf_icmpv6_dio_rank, tvb, offset, 2, FALSE);
                offset += 2;

                /* flags */
                flags = tvb_get_guint8(tvb, offset);
                proto_tree_add_boolean(icmp6_tree, hf_icmpv6_dio_grounded, tvb, offset, 1, flags & RPL_DIO_FLAG_GROUNDED);
                proto_tree_add_boolean(icmp6_tree, hf_icmpv6_dio_zero, tvb, offset, 1, flags & RPL_DIO_FLAG_ZERO);
                proto_tree_add_uint(icmp6_tree, hf_icmpv6_dio_mop, tvb, offset, 1, flags & RPL_DIO_FLAG_MOP);
                proto_tree_add_uint(icmp6_tree, hf_icmpv6_dio_preference, tvb, offset, 1, flags & RPL_DIO_FLAG_PREFERENCE);
                offset += 1;

                /* Destination Advertisement Trigger Sequence Number (DTSN) */
                proto_tree_add_item(icmp6_tree, hf_icmpv6_dio_trigger_seqnum, tvb, offset, 1, FALSE);
                offset += 1;
                offset += 2;

                /* DODAGID */
                proto_tree_add_item(icmp6_tree, hf_icmpv6_dio_dagid, tvb, offset, 16, FALSE);
                offset += 16;
            }
            else if (dp->icmp6_code == ICMP6_RPL_DAO) {
                /* Destination Advertisement Object */
                struct e_in6_addr   addr6;
                guint8 flags;

                /* DAO Instance */
                proto_tree_add_item(icmp6_tree, hf_icmpv6_dao_instance, tvb, offset, 1, FALSE);
                offset += 1;

                /* flags */
                flags = tvb_get_guint8(tvb, offset);
                proto_tree_add_boolean(icmp6_tree, hf_icmpv6_dao_flag_k, tvb, offset, 1, flags & RPL_DAO_FLAG_K);
                proto_tree_add_boolean(icmp6_tree, hf_icmpv6_dao_flag_d, tvb, offset, 1, flags & RPL_DAO_FLAG_D);
                proto_tree_add_uint(icmp6_tree, hf_icmpv6_dao_flag_rsv, tvb, offset, 1, flags & RPL_DAO_FLAG_RESERVED);
                offset += 1;

                /* DAO Reserved */
                proto_tree_add_item(icmp6_tree, hf_icmpv6_dao_reserved, tvb, offset, 1, FALSE);
                offset += 1;

                /* DAO Reserved */
                proto_tree_add_item(icmp6_tree, hf_icmpv6_dao_seqnum, tvb, offset, 1, FALSE);
                offset += 1;

                if(flags & RPL_DAO_FLAG_D)
                {
                    tvb_memcpy(tvb, addr6.bytes, offset, 16);
                    proto_tree_add_ipv6(icmp6_tree, hf_icmpv6_dao_dodagid, tvb, offset, 16, addr6.bytes);
                    offset += 16;
                }
            }
            else if (dp->icmp6_code == ICMP6_RPL_DAOACK) {
                /* Destination Advertisement Object */
                struct e_in6_addr   addr6;
                guint8 flags;

                /* DAO Instance */
                proto_tree_add_item(icmp6_tree, hf_icmpv6_daoack_instance, tvb, offset, 1, FALSE);
                offset += 1;

                /* flags */
                flags = tvb_get_guint8(tvb, offset);
                proto_tree_add_boolean(icmp6_tree, hf_icmpv6_daoack_flag_d, tvb, offset, 1, flags & RPL_DAOACK_FLAG_D);
                proto_tree_add_uint(icmp6_tree, hf_icmpv6_daoack_flag_rsv, tvb, offset, 1, flags & RPL_DAOACK_FLAG_RESERVED);
                offset += 1;

                /* DAO Sequence */
                proto_tree_add_item(icmp6_tree, hf_icmpv6_daoack_seqnum, tvb, offset, 1, FALSE);
                offset += 1;

                /* DAO Status */
                proto_tree_add_item(icmp6_tree, hf_icmpv6_daoack_status, tvb, offset, 1, FALSE);
                offset += 1;

                if(flags & RPL_DAOACK_FLAG_D)
                {
                    tvb_memcpy(tvb, addr6.bytes, offset, 16);
                    proto_tree_add_ipv6(icmp6_tree, hf_icmpv6_daoack_dodagid, tvb, offset, 16, addr6.bytes);
                    offset += 16;
                }
                return;
            }
            /* Options */
            dissect_icmpv6rplopt(tvb, offset, pinfo, icmp6_tree);
            break;
        default:
            next_tvb = tvb_new_subset(tvb, offset + sizeof(*dp), -1, -1);
            call_dissector(data_handle,next_tvb, pinfo, tree);
            break;
        } /* switch (dp->icmp6_type) */
    } /* if (tree) */
}

void
proto_register_icmpv6(void)
{
    static hf_register_info hf[] = {
        { &hf_icmpv6_type,
          { "Type",           "icmpv6.type",    FT_UINT8,  BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
        { &hf_icmpv6_code,
          { "Code",           "icmpv6.code",    FT_UINT8,  BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
        { &hf_icmpv6_checksum,
          { "Checksum",       "icmpv6.checksum", FT_UINT16, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
        { &hf_icmpv6_checksum_bad,
          { "Bad Checksum",   "icmpv6.checksum_bad", FT_BOOLEAN, BASE_NONE,  NULL, 0x0,
            NULL, HFILL }},
        { &hf_icmpv6_nor,
          { "Number of records", "icmpv6.nor", FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
        { &hf_icmpv6_haad_ha_addrs,
          { "Home Agent Addresses", "icmpv6.haad.ha_addrs", FT_IPv6, BASE_NONE, NULL, 0x0,
            NULL, HFILL }},
        { &hf_icmpv6_ra_cur_hop_limit,
          { "Cur hop limit",           "icmpv6.ra.cur_hop_limit", FT_UINT8,  BASE_DEC, NULL, 0x0,
            "Current hop limit", HFILL }},
        { &hf_icmpv6_ra_router_lifetime,
          { "Router lifetime",         "icmpv6.ra.router_lifetime", FT_UINT16,  BASE_DEC, NULL, 0x0,
            "Router lifetime (s)", HFILL }},
        { &hf_icmpv6_ra_reachable_time,
          { "Reachable time",          "icmpv6.ra.reachable_time", FT_UINT32,  BASE_DEC, NULL, 0x0,
            "Reachable time (ms)", HFILL }},
        { &hf_icmpv6_ra_retrans_timer,
          { "Retrans timer",           "icmpv6.ra.retrans_timer", FT_UINT32,  BASE_DEC, NULL, 0x0,
            "Retrans timer (ms)", HFILL }},
        { &hf_icmpv6_option,
          { "ICMPv6 Option",           "icmpv6.option", FT_NONE,  BASE_NONE, NULL, 0x0,
            "Option", HFILL }},
        { &hf_icmpv6_option_type,
          { "Type",           "icmpv6.option.type", FT_UINT8,  BASE_DEC, VALS(option_vals), 0x0,
            "Options type", HFILL }},
        { &hf_icmpv6_option_length,
          { "Length",         "icmpv6.option.length", FT_UINT8,  BASE_DEC, NULL, 0x0,
            "Options length (in bytes)", HFILL }},
        { &hf_icmpv6_opt_cga_pad_len,
          { "Pad Length",     "icmpv6.option.cga.pad_length", FT_UINT8,  BASE_DEC, NULL, 0x0,
            "Pad Length (in bytes)", HFILL }},
        { &hf_icmpv6_opt_cga,
          { "CGA",            "icmpv6.option.cga", FT_BYTES,  BASE_NONE, NULL, 0x0,
            NULL, HFILL }},
        { &hf_icmpv6_opt_cga_modifier,
          { "Modifier",       "icmpv6.option.cga.modifier", FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }},
        { &hf_icmpv6_opt_cga_subnet_prefix,
          { "Subnet Prefix",  "icmpv6.option.cga.subnet_prefix", FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }},
        { &hf_icmpv6_opt_cga_count,
          { "Count",          "icmpv6.option.cga.count", FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }},
        { &hf_icmpv6_opt_cga_ext_type,
          { "Ext Type",       "icmpv6.option.cga.ext_type", FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }},
        { &hf_icmpv6_opt_cga_ext_length,
          { "Ext Length",     "icmpv6.option.cga.ext_length", FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }},
        { &hf_icmpv6_opt_rsa_key_hash,
          { "Key Hash",       "icmpv6.option.rsa.key_hash", FT_BYTES,  BASE_NONE, NULL, 0x0,
            NULL, HFILL }},
        { &hf_icmpv6_opt_name_type,
          { "Name Type",      "icmpv6.option.name_type", FT_UINT8,  BASE_DEC, VALS(icmpv6_option_name_type_vals), 0x0,
            NULL, HFILL }},
        { &hf_icmpv6_opt_name_x501,
          { "DER Encoded X.501 Name", "icmpv6.option.name_x501", FT_BYTES,  BASE_NONE, NULL, 0x0,
            NULL, HFILL }},
        { &hf_icmpv6_opt_name_fqdn,
          { "FQDN",           "icmpv6.option.name_type.fqdn", FT_STRING,  BASE_NONE, NULL, 0x0,
            NULL, HFILL }},
        { &hf_icmpv6_opt_cert_type,
          { "Cert Type",       "icmpv6.option.name_type", FT_UINT8,  BASE_DEC, VALS(icmpv6_option_cert_type_vals), 0x0,
            NULL, HFILL }},
        { &hf_icmpv6_identifier,
          { "Identifier",      "icmpv6.identifier", FT_UINT16,  BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
        { &hf_icmpv6_all_comp,
          { "All Components",  "icmpv6.all_comp", FT_UINT16,  BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
        { &hf_icmpv6_comp,
          { "Component",       "icmpv6.comp", FT_UINT16,  BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
        { &hf_icmpv6_x509if_Name,
          { "Name",            "icmpv6.x509_Name", FT_UINT32, BASE_DEC, VALS(x509if_Name_vals), 0x0,
            NULL, HFILL }},
        { &hf_icmpv6_x509af_Certificate,
          { "Certificate",     "icmpv6.x509_Certificate", FT_NONE, BASE_NONE, NULL, 0x0,
            NULL, HFILL }},
        { &hf_icmpv6_recursive_dns_serv,
          { "Recursive DNS Servers", "icmpv6.recursive_dns_serv", FT_IPv6, BASE_NONE, NULL, 0x0,
            NULL, HFILL }},
		{ &hf_icmpv6_opt_prefix_len,
          { "Prefix Length", "icmpv6.opt_prefix.length", FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
		{ &hf_icmpv6_opt_flag_l,
          { "On-link flag(L)", "icmpv6.opt_prefix.flag.l", FT_BOOLEAN, 8, TFS(&tfs_set_notset), 0x80,
            NULL, HFILL }},
		{ &hf_icmpv6_opt_flag_a,
          { "Autonomous address-configuration flag(A)", "icmpv6.opt_prefix.flag.a", FT_BOOLEAN, 8, TFS(&tfs_set_notset), 0x40,
            NULL, HFILL }},
        { &hf_icmpv6_opt_aro_lifetime,
          { "Address Lifetime", "icmpv6.opt_aro.lifetime", FT_UINT16, BASE_DEC, NULL, 0x00,
            NULL, HFILL }},
        { &hf_icmpv6_opt_aro_eui64,
          { "EUI-64", "icmpv6.opt_aro.eui64", FT_BYTES, BASE_NONE, NULL, 0x00,
            NULL, HFILL }},
        { &hf_icmpv6_opt_aro_addr_reg,
          { "Registered Address", "icmpv6.opt_aro.addr_reg", FT_IPv6, BASE_NONE, NULL, 0x00,
            NULL, HFILL }},
        { &hf_icmpv6_opt_6co_context_length,
          { "Context Length", "icmpv6.opt_6co.context_length", FT_UINT8, BASE_DEC, NULL, 0x00,
            NULL, HFILL }},
        { &hf_icmpv6_opt_6co_lifetime,
          { "Lifetime (x 10s)", "icmpv6.opt_6co.lifetime", FT_UINT16, BASE_DEC, NULL, 0x00,
            NULL, HFILL }},
        { &hf_icmpv6_opt_abro_version,
          { "Version", "icmpv6.opt_abro.version", FT_UINT16, BASE_DEC, NULL, 0x00,
            NULL, HFILL }},
        { &hf_icmpv6_opt_abro_addr,
          { "Address", "icmpv6.opt_abro.addr", FT_IPv6, BASE_NONE, NULL, 0x00,
            NULL, HFILL }},
		{ &hf_icmpv6_opt_flag_reserved,
          { "Reserved", "icmpv6.opt_prefix.flag.reserved", FT_UINT8, BASE_DEC, NULL, 0x3f,
            NULL, HFILL }},
        /* RPL: draft-ietf-roll-rpl-12.txt: Routing over Low-Power and Lossy Networks. */
        { &hf_icmpv6_dis_reserved,
           { "Grounded",          "icmpv6.rpl.dis.reserved", FT_UINT16, BASE_DEC, NULL, 0x0,
             NULL, HFILL }},
        { &hf_icmpv6_dio_grounded,
           { "Grounded",          "icmpv6.rpl.dio.grounded", FT_BOOLEAN, 8, NULL, RPL_DIO_FLAG_GROUNDED,
             NULL, HFILL }},
        { &hf_icmpv6_dio_zero,
           { "Zero",              "icmpv6.rpl.dio.zero", FT_BOOLEAN, 8, NULL, RPL_DIO_FLAG_ZERO,
             NULL, HFILL }},
        { &hf_icmpv6_dio_mop,
           { "MOP",               "icmpv6.rpl.dio.mop", FT_UINT8, BASE_HEX, NULL, RPL_DIO_FLAG_MOP,
             NULL, HFILL }},
        { &hf_icmpv6_dio_preference,
           { "DAG Preference",    "icmpv6.rpl.dio.preference", FT_UINT8, BASE_DEC, NULL, RPL_DIO_FLAG_PREFERENCE,
             NULL, HFILL }},
        { &hf_icmpv6_dio_rank,
           { "Rank",              "icmpv6.rpl.dio.rank", FT_UINT16, BASE_DEC, NULL, 0x0,
             NULL, HFILL }},
        { &hf_icmpv6_dio_instance,
           { "RPLInstanceID",     "icmpv6.rpl.dio.instance", FT_UINT8, BASE_DEC, NULL, 0x0,
             NULL, HFILL }},
        { &hf_icmpv6_dio_version,
           { "version",           "icmpv6.rpl.dio.version", FT_UINT8, BASE_DEC, NULL, 0x0,
             NULL, HFILL }},
        { &hf_icmpv6_dio_trigger_seqnum,
           { "DTSN",              "icmpv6.rpl.dio.dtsn", FT_UINT8, BASE_DEC, NULL, 0x0,
             "Destination advertisement trigger sequence number", HFILL }},
        { &hf_icmpv6_dao_instance,
           { "DAO Instance",      "icmpv6.rpl.dao.instance", FT_UINT8, BASE_DEC, NULL, 0x0,
             NULL, HFILL }},
        { &hf_icmpv6_dio_dagid,
           { "DODAGID",           "icmpv6.rpl.dio.dagid", FT_IPv6, BASE_NONE, NULL, 0x0,
             NULL, HFILL }},
        { &hf_icmpv6_dao_seqnum,
           { "DAO Sequence",      "icmpv6.rpl.dao.sequence", FT_UINT8, BASE_DEC, NULL, 0x0,
             NULL, HFILL }},
        { &hf_icmpv6_dao_reserved,
           { "Reserved",          "icmpv6.rpl.dao.reserved", FT_UINT8, BASE_DEC, NULL, 0x0,
             NULL, HFILL }},
        { &hf_icmpv6_dao_flag_k,
           { "DAO-ACK Request",   "icmpv6.rpl.dao.flag_k", FT_BOOLEAN, 8, NULL, RPL_DAO_FLAG_K,
             NULL, HFILL }},
        { &hf_icmpv6_dao_flag_d,
           { "DODAGID Present",   "icmpv6.rpl.dao.flag_d", FT_BOOLEAN, 8, NULL, RPL_DAO_FLAG_D,
             NULL, HFILL }},
        { &hf_icmpv6_dao_flag_rsv,
           { "Reserved",          "icmpv6.rpl.dao.flag_rsv", FT_UINT8, BASE_DEC, NULL, RPL_DAO_FLAG_RESERVED,
             NULL, HFILL }},
        { &hf_icmpv6_dao_dodagid,
           { "DODAGID",           "icmpv6.rpl.dao.dodagid", FT_IPv6, BASE_NONE, NULL, 0x0,
             NULL, HFILL }},
        { &hf_icmpv6_daoack_instance,
           { "Instance",          "icmpv6.rpl.daoack.instance", FT_UINT8, BASE_DEC, NULL, 0x0,
             NULL, HFILL }},
        { &hf_icmpv6_daoack_seqnum,
           { "DAO-ACK Sequence",  "icmpv6.rpl.daoack.sequence", FT_UINT8, BASE_DEC, NULL, 0x0,
             NULL, HFILL }},
        { &hf_icmpv6_daoack_status,
           { "Status",            "icmpv6.rpl.daoack.status", FT_UINT8, BASE_DEC, NULL, 0x0,
             NULL, HFILL }},
        { &hf_icmpv6_daoack_flag_d,
           { "DODAGID Present",   "icmpv6.rpl.daoack.flag_d", FT_BOOLEAN, 8, NULL, RPL_DAOACK_FLAG_D,
             NULL, HFILL }},
        { &hf_icmpv6_daoack_flag_rsv,
           { "Reserved",          "icmpv6.rpl.daoack.flag_rsv", FT_UINT8, BASE_DEC, NULL, RPL_DAOACK_FLAG_RESERVED,
             NULL, HFILL }},
        { &hf_icmpv6_daoack_dodagid,
           { "DODAGID",           "icmpv6.rpl.daoack.dodagid", FT_IPv6, BASE_NONE, NULL, 0x0,
             NULL, HFILL }},
        { &hf_icmpv6_rpl_opt_route,
           { "Route",             "icmpv6.rpl.opt.route", FT_IPv6, BASE_NONE, NULL, 0x0,
             NULL, HFILL }},
        { &hf_icmpv6_rpl_opt_route_pref,
           { "Routing Preference","icmpv6.rpl.opt.route.pref", FT_UINT8, BASE_DEC, NULL, RPL_OPT_ROUTE_PREFERENCE,
             NULL, HFILL }},
        { &hf_icmpv6_rpl_opt_route_lifetime,
           { "Route Lifetime",    "icmpv6.rpl.opt.route.lifetime", FT_UINT32, BASE_DEC, NULL, 0x0,
             NULL, HFILL }},
        { &hf_icmpv6_rpl_opt_route_length,
           { "Route Length",      "icmpv6.rpl.opt.route.length", FT_UINT8, BASE_DEC, NULL, 0x0,
             NULL, HFILL }},
        { &hf_icmpv6_rpl_opt_prefix_flag_a,
           { "Auto Address Config","icmpv6.rpl.opt.config.A", FT_BOOLEAN, 8, NULL, RPL_OPT_PREFIX_A,
             NULL, HFILL }},
        { &hf_icmpv6_rpl_opt_prefix_flag_r,
           { "Router Address",     "icmpv6.rpl.opt.config.R", FT_BOOLEAN, 8, NULL, RPL_OPT_PREFIX_R,
             NULL, HFILL }},
        { &hf_icmpv6_rpl_opt_config_auth,
           { "Authentication Enabled","icmpv6.rpl.opt.config.auth", FT_BOOLEAN, 8, NULL, RPL_OPT_CONFIG_FLAG_AUTH,
             NULL, HFILL }},
        { &hf_icmpv6_rpl_opt_config_pcs,
           { "Path Control Size",  "icmpv6.rpl.opt.config.pcs", FT_UINT8, BASE_DEC, NULL, RPL_OPT_CONFIG_FLAG_PCS,
             NULL, HFILL }},
        { &hf_icmpv6_rpl_opt_config_doublings,
           { "DIOIntervalDoublings","icmpv6.rpl.opt.config.interval_double", FT_UINT8, BASE_DEC, NULL, 0x0,
             NULL, HFILL }},
        { &hf_icmpv6_rpl_opt_config_min_interval,
           { "DIOIntervalMin",     "icmpv6.rpl.opt.config.interval_min", FT_UINT16, BASE_DEC, NULL, 0x0,
             NULL, HFILL }},
        { &hf_icmpv6_rpl_opt_config_redundancy,
           { "DIORedundancyConstant", "icmpv6.rpl.opt.config.redundancy", FT_UINT8, BASE_DEC, NULL, 0x0,
             NULL, HFILL }},
        { &hf_icmpv6_rpl_opt_config_rank_incr,
           { "MaxRankInc",         "icmpv6.rpl.opt.config.max_rank_inc", FT_UINT16, BASE_DEC, NULL, 0x0,
             NULL, HFILL }},
        { &hf_icmpv6_rpl_opt_config_hop_rank_inc,
           { "MinHopRankInc",      "icmpv6.rpl.opt.config.min_hop_rank_inc", FT_UINT16, BASE_DEC, NULL, 0x0,
              NULL, HFILL }},
        { &hf_icmpv6_rpl_opt_config_ocp,
           { "Objective Code Point","icmpv6.rpl.opt.config.ocp", FT_UINT16, BASE_DEC, NULL, 0x0,
              NULL, HFILL }},
        { &hf_icmpv6_rpl_opt_config_rsv,
           { "Reserved",           "icmpv6.rpl.opt.config.rsv", FT_UINT8, BASE_DEC, NULL, 0x0,
              NULL, HFILL }},
        { &hf_icmpv6_rpl_opt_config_def_lifetime,
           { "Default Lifetime",   "icmpv6.rpl.opt.config.def_lifetime", FT_UINT8, BASE_DEC, NULL, 0x0,
              NULL, HFILL }},
        { &hf_icmpv6_rpl_opt_config_lifetime_unit,
           { "Lifetime Unit",      "icmpv6.rpl.opt.config.lifetime_unit", FT_UINT16, BASE_DEC, NULL, 0x0,
              NULL, HFILL }},
        { &hf_icmpv6_rpl_opt_target,
           { "Target",             "icmpv6.rpl.opt.target", FT_IPv6, BASE_NONE, NULL, 0x0,
              NULL, HFILL }},
        { &hf_icmpv6_rpl_opt_target_length,
           { "Target Length",      "icmpv6.rpl.opt.target.length", FT_UINT8, BASE_DEC, NULL, 0x0,
              NULL, HFILL }},
        { &hf_icmpv6_rpl_opt_target_reserved,
           { "Reserved",           "icmpv6.rpl.opt.target.reserved", FT_UINT8, BASE_DEC, NULL, 0x0,
              NULL, HFILL }},
         { &hf_icmpv6_rpl_opt_transit_e,
           { "External",           "icmpv6.rpl.opt.transit.e", FT_BOOLEAN, 8, NULL, RPL_OPT_TRANSIT_E,
             NULL, HFILL }},
        { &hf_icmpv6_rpl_opt_transit_flags,
           { "Flags",              "icmpv6.rpl.opt.transit.flags", FT_UINT8, BASE_DEC, NULL, RPL_OPT_TRANSIT_FLAGS,
             NULL, HFILL }},
        { &hf_icmpv6_rpl_opt_transit_pathseq,
           { "Path Sequence",      "icmpv6.rpl.opt.transit.pathseq", FT_UINT8, BASE_DEC, NULL, 0x0,
              NULL, HFILL }},
        { &hf_icmpv6_rpl_opt_transit_pathctl,
           { "Path Control",       "icmpv6.rpl.opt.transit.pathctl", FT_UINT8, BASE_DEC, NULL, 0x0,
              NULL, HFILL }},
        { &hf_icmpv6_rpl_opt_transit_pathlifetime,
           { "Path Lifetime",      "icmpv6.rpl.opt.transit.pathlifetime", FT_UINT8, BASE_DEC, NULL, 0x0,
              NULL, HFILL }},
        { &hf_icmpv6_rpl_opt_transit_parent,
           { "Parent Address",     "icmpv6.rpl.opt.transit.parent", FT_IPv6, BASE_NONE, NULL, 0x0,
              NULL, HFILL }},
        { &hf_icmpv6_rpl_opt_solicited_instance,
           { "Instance",           "icmpv6.rpl.opt.solicited.instance", FT_UINT8, BASE_DEC, NULL, 0x0,
              NULL, HFILL }},
        { &hf_icmpv6_rpl_opt_solicited_flag_v,
           { "Version predicate",  "icmpv6.rpl.opt.solicited.versionflag", FT_BOOLEAN, 8, NULL, RPL_OPT_SOLICITED_V,
              NULL, HFILL }},
        { &hf_icmpv6_rpl_opt_solicited_flag_i,
           { "InstanceID predicate","icmpv6.rpl.opt.solicited.instanceflag", FT_BOOLEAN, 8, NULL, RPL_OPT_SOLICITED_I,
              NULL, HFILL }},
        { &hf_icmpv6_rpl_opt_solicited_flag_d,
           { "DODAGID predicate",  "icmpv6.rpl.opt.solicited.dodagidflag", FT_BOOLEAN, 8, NULL, RPL_OPT_SOLICITED_D,
              NULL, HFILL }},
        { &hf_icmpv6_rpl_opt_solicited_dodagid,
           { "DODAGID",            "icmpv6.rpl.opt.solicited.dodagid", FT_IPv6, BASE_NONE, NULL, 0x0,
              NULL, HFILL }},
        { &hf_icmpv6_rpl_opt_solicited_version,
           { "Version",            "icmpv6.rpl.opt.solicited.version", FT_UINT8, BASE_DEC, NULL, 0x0,
              NULL, HFILL }},
        { &hf_icmpv6_rpl_opt_prefix_vlifetime,
           { "Valid Lifetime",    "icmpv6.rpl.opt.prefix.valid_lifetime", FT_UINT32, BASE_DEC, NULL, 0x0,
             NULL, HFILL }},
        { &hf_icmpv6_rpl_opt_prefix_plifetime,
           { "Preferred Lifetime", "icmpv6.rpl.opt.prefix.preferred_lifetime", FT_UINT32, BASE_DEC, NULL, 0x0,
             NULL, HFILL }},
        { &hf_icmpv6_rpl_opt_prefix_length,
           { "Prefix Length",      "icmpv6.rpl.opt.prefix.length", FT_UINT8, BASE_DEC, NULL, 0x0,
             NULL, HFILL }},
        { &hf_icmpv6_rpl_opt_prefix,
           { "Destination Prefix", "icmpv6.rpl.opt.prefix", FT_IPv6, BASE_NONE, NULL, 0x0,
             NULL, HFILL }},
        { &hf_icmpv6_rpl_opt_prefix_flag_l,
           { "On Link",            "icmpv6.rpl.opt.prefix.L", FT_BOOLEAN, 8, NULL, RPL_OPT_PREFIX_L,
             NULL, HFILL }},
        { &hf_icmpv6_rpl_opt_targetdesc,
           { "Descriptor",         "icmpv6.rpl.opt.targetdesc.descriptor", FT_UINT32, BASE_HEX, NULL, 0x0,
             NULL, HFILL }},

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
        &ett_multicastRR,
        &ett_icmpv6opt_name,
        &ett_cga_param_name,
        &ett_dao_rr_stack
    };

    proto_icmpv6 = proto_register_protocol("Internet Control Message Protocol v6",
                                           "ICMPv6", "icmpv6");
    proto_register_field_array(proto_icmpv6, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));

    register_dissector("icmpv6", dissect_icmpv6, proto_icmpv6);
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

/*
 * Editor modelines  -  http://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 4
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vi: set shiftwidth=4 tabstop=8 expandtab
 * :indentSize=4:tabSize=8:noTabs=true:
 */
