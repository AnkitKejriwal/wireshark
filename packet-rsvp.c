/* packet-rsvp.c
 * Routines for RSVP packet disassembly
 *
 * (c) Copyright Ashok Narayanan <ashokn@cisco.com>
 *
 * $Id: packet-rsvp.c,v 1.90 2004/06/19 00:07:22 guy Exp $
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

/*
 * NOTES
 *
 * This module defines routines to disassemble RSVP packets, as defined in
 * RFC 2205. All objects from RFC2205 are supported, in IPv4 and IPv6 mode.
 * In addition, the Integrated Services traffic specification objects
 * defined in RFC2210 are also supported.
 *
 * IPv6 support is not completely tested
 *
 * Mar 3, 2000: Added support for MPLS/TE objects, as defined in
 * <draft-ietf-mpls-rsvp-lsp-tunnel-04.txt>
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

#include <glib.h>

#ifdef NEED_SNPRINTF_H
# include "snprintf.h"
#endif

#include <epan/tvbuff.h>
#include <epan/packet.h>
#include <prefs.h>
#include "in_cksum.h"
#include "etypes.h"
#include "ipproto.h"

#include "packet-rsvp.h"
#include "packet-ip.h"
#include "packet-frame.h"
#include "packet-diffserv-mpls-common.h"

static int proto_rsvp = -1;

/*
 * The list of tree types
 */
enum {
    TT_RSVP,
    TT_HDR,
    TT_SESSION,
    TT_HOP,
    TT_HOP_SUBOBJ,
    TT_TIME_VALUES,
    TT_ERROR,
    TT_ERROR_SUBOBJ,
    TT_SCOPE,
    TT_STYLE,
    TT_CONFIRM,
    TT_SENDER_TEMPLATE,
    TT_FILTER_SPEC,
    TT_TSPEC,
    TT_TSPEC_SUBTREE,
    TT_FLOWSPEC,
    TT_FLOWSPEC_SUBTREE,
    TT_ADSPEC,
    TT_ADSPEC_SUBTREE,
    TT_INTEGRITY,
    TT_INTEGRITY_FLAGS,
    TT_DCLASS,
    TT_LSP_TUNNEL_IF_ID,
    TT_POLICY,
    TT_MESSAGE_ID,
    TT_MESSAGE_ID_ACK,
    TT_MESSAGE_ID_LIST,
    TT_LABEL,
    TT_LABEL_REQUEST,
    TT_SESSION_ATTRIBUTE,
    TT_SESSION_ATTRIBUTE_FLAGS,
    TT_HELLO_OBJ,
    TT_EXPLICIT_ROUTE,
    TT_EXPLICIT_ROUTE_SUBOBJ,
    TT_RECORD_ROUTE,
    TT_RECORD_ROUTE_SUBOBJ,
    TT_RECORD_ROUTE_SUBOBJ_FLAGS,
    TT_ADMIN_STATUS,
    TT_ADMIN_STATUS_FLAGS,
    TT_GEN_UNI,
    TT_GEN_UNI_SUBOBJ,
    TT_BUNDLE_COMPMSG,
    TT_RESTART_CAP,
    TT_PROTECTION_INFO,
    TT_FAST_REROUTE,
    TT_FAST_REROUTE_FLAGS,
    TT_DETOUR,
    TT_DIFFSERV,
    TT_DIFFSERV_MAP,
    TT_DIFFSERV_MAP_PHBID,
    TT_UNKNOWN_CLASS,

    TT_MAX
};
static gint ett_treelist[TT_MAX];
static gint *ett_tree[TT_MAX];
#define TREE(X) ett_treelist[(X)]

/* Should we dissect bundle messages? */
static gboolean rsvp_bundle_dissect = TRUE;

/*
 * RSVP message types.
 * See
 *
 *	http://www.iana.org/assignments/rsvp-parameters
 */
typedef enum {
    RSVP_MSG_PATH=1,			/* RFC 2205 */
    RSVP_MSG_RESV,			/* RFC 2205 */
    RSVP_MSG_PERR,			/* RFC 2205 */
    RSVP_MSG_RERR,			/* RFC 2205 */
    RSVP_MSG_PTEAR,			/* RFC 2205 */
    RSVP_MSG_RTEAR,			/* RFC 2205 */
    RSVP_MSG_CONFIRM,			/* XXX - DREQ, RFC 2745? */
    					/* 9 is DREP, RFC 2745 */
    RSVP_MSG_RTEAR_CONFIRM=10,		/* from Fred Baker at Cisco */
    					/* 11 is unassigned */
    RSVP_MSG_BUNDLE = 12,		/* RFC 2961 */
    RSVP_MSG_ACK,			/* RFC 2961 */
    					/* 14 is reserved */
    RSVP_MSG_SREFRESH = 15,		/* RFC 2961 */
    					/* 16, 17, 18, 19 not listed */
    RSVP_MSG_HELLO = 20			/* RFC 3209 */
    					/* 25 is Integrity Challenge
    					   RFC 2747, RFC 3097 */
    					/* 26 is Integrity Response
    					   RFC 2747, RFC 3097 */
    					/* 66 is DSBM_willing [SBM] */
    					/* 67 is I_AM_DSBM [SBM] */
    					/* [SBM] is Subnet Bandwidth
    					   Manager ID from July 1997 */
} rsvp_message_types;

static value_string message_type_vals[] = {
    {RSVP_MSG_PATH, "PATH Message. "},
    {RSVP_MSG_RESV, "RESV Message. "},
    {RSVP_MSG_PERR, "PATH ERROR Message. "},
    {RSVP_MSG_RERR, "RESV ERROR Message. "},
    {RSVP_MSG_PTEAR, "PATH TEAR Message. "},
    {RSVP_MSG_RTEAR, "RESV TEAR Message. "},
    {RSVP_MSG_CONFIRM, "CONFIRM Message. "},
    {RSVP_MSG_RTEAR_CONFIRM, "RESV TEAR CONFIRM Message. "},
    {RSVP_MSG_BUNDLE, "BUNDLE Message. "},
    {RSVP_MSG_ACK, "ACK Message. "},
    {RSVP_MSG_SREFRESH, "SREFRESH Message. "},
    {RSVP_MSG_HELLO, "HELLO Message. "},
    {0, NULL}
};

/*
 * RSVP classes
 */
#define MAX_RSVP_CLASS 15

enum rsvp_classes {
    RSVP_CLASS_NULL=0,
    RSVP_CLASS_SESSION,

    RSVP_CLASS_HOP=3,
    RSVP_CLASS_INTEGRITY,
    RSVP_CLASS_TIME_VALUES,
    RSVP_CLASS_ERROR,
    RSVP_CLASS_SCOPE,
    RSVP_CLASS_STYLE,
    RSVP_CLASS_FLOWSPEC,
    RSVP_CLASS_FILTER_SPEC,
    RSVP_CLASS_SENDER_TEMPLATE,
    RSVP_CLASS_SENDER_TSPEC,
    RSVP_CLASS_ADSPEC,
    RSVP_CLASS_POLICY,
    RSVP_CLASS_CONFIRM,
    RSVP_CLASS_LABEL,

    RSVP_CLASS_LABEL_REQUEST=19,
    RSVP_CLASS_EXPLICIT_ROUTE,
    RSVP_CLASS_RECORD_ROUTE,

    RSVP_CLASS_HELLO,

    RSVP_CLASS_MESSAGE_ID,
    RSVP_CLASS_MESSAGE_ID_ACK,
    RSVP_CLASS_MESSAGE_ID_LIST,

    RSVP_CLASS_RECOVERY_LABEL = 34,
    RSVP_CLASS_UPSTREAM_LABEL,
    RSVP_CLASS_LABEL_SET,
    RSVP_CLASS_PROTECTION,

    RSVP_CLASS_DETOUR = 63,

    RSVP_CLASS_DIFFSERV = 65,

    RSVP_CLASS_SUGGESTED_LABEL = 129,
    RSVP_CLASS_ACCEPTABLE_LABEL_SET,
    RSVP_CLASS_RESTART_CAP,

    RSVP_CLASS_NOTIFY_REQUEST = 195,
    RSVP_CLASS_ADMIN_STATUS,

    RSVP_CLASS_FAST_REROUTE = 205,
    RSVP_CLASS_SESSION_ATTRIBUTE = 207,
    RSVP_CLASS_DCLASS = 225,
    RSVP_CLASS_LSP_TUNNEL_IF_ID = 227,
    RSVP_CLASS_GENERALIZED_UNI = 229

};

static value_string rsvp_class_vals[] = {
    {RSVP_CLASS_NULL, "NULL object"},
    {RSVP_CLASS_SESSION, "SESSION object"},
    {RSVP_CLASS_HOP, "HOP object"},
    {RSVP_CLASS_INTEGRITY, "INTEGRITY object"},
    {RSVP_CLASS_TIME_VALUES, "TIME VALUES object"},
    {RSVP_CLASS_ERROR, "ERROR object"},
    {RSVP_CLASS_SCOPE, "SCOPE object"},
    {RSVP_CLASS_STYLE, "STYLE object"},
    {RSVP_CLASS_FLOWSPEC, "FLOWSPEC object"},
    {RSVP_CLASS_FILTER_SPEC, "FILTER SPEC object"},
    {RSVP_CLASS_SENDER_TEMPLATE, "SENDER TEMPLATE object"},
    {RSVP_CLASS_SENDER_TSPEC, "SENDER TSPEC object"},
    {RSVP_CLASS_ADSPEC, "ADSPEC object"},
    {RSVP_CLASS_POLICY, "POLICY object"},
    {RSVP_CLASS_CONFIRM, "CONFIRM object"},
    {RSVP_CLASS_LABEL, "LABEL object"},
    {RSVP_CLASS_LABEL_REQUEST, "LABEL REQUEST object"},
    {RSVP_CLASS_EXPLICIT_ROUTE, "EXPLICIT ROUTE object"},
    {RSVP_CLASS_RECORD_ROUTE, "RECORD ROUTE object"},
    {RSVP_CLASS_SESSION_ATTRIBUTE, "SESSION ATTRIBUTE object"},
    {RSVP_CLASS_MESSAGE_ID, "MESSAGE-ID object"},
    {RSVP_CLASS_MESSAGE_ID_ACK, "MESSAGE-ID ACK/NACK object"},
    {RSVP_CLASS_MESSAGE_ID_LIST, "MESSAGE-ID LIST object"},
    {RSVP_CLASS_HELLO, "HELLO object"},
    {RSVP_CLASS_RECOVERY_LABEL, "RECOVERY-LABEL object"},
    {RSVP_CLASS_UPSTREAM_LABEL, "UPSTREAM-LABEL object"},
    {RSVP_CLASS_LABEL_SET, "LABEL-SET object"},
    {RSVP_CLASS_PROTECTION, "PROTECTION object"},
    {RSVP_CLASS_DIFFSERV, "DIFFSERV object"},
    {RSVP_CLASS_SUGGESTED_LABEL, "SUGGESTED-LABEL object"},
    {RSVP_CLASS_ACCEPTABLE_LABEL_SET, "ACCEPTABLE-LABEL-SET object"},
    {RSVP_CLASS_RESTART_CAP, "RESTART-CAPABILITY object"},
    {RSVP_CLASS_DCLASS, "DCLASS object"},
    {RSVP_CLASS_LSP_TUNNEL_IF_ID, "LSP-TUNNEL INTERFACE-ID object"},
    {RSVP_CLASS_NOTIFY_REQUEST, "NOTIFY-REQUEST object"},
    {RSVP_CLASS_ADMIN_STATUS, "ADMIN-STATUS object"},
    {RSVP_CLASS_GENERALIZED_UNI, "GENERALIZED-UNI object"},
    {RSVP_CLASS_DETOUR, "DETOUR object"},
    {RSVP_CLASS_FAST_REROUTE, "FAST-REROUTE object"},
    {0, NULL}
};

/*
 * RSVP error values
 */
enum rsvp_error_types {
    RSVP_ERROR_CONFIRM = 0,
    RSVP_ERROR_ADMISSION,
    RSVP_ERROR_POLICY,
    RSVP_ERROR_NO_PATH,
    RSVP_ERROR_NO_SENDER,
    RSVP_ERROR_CONFLICT_RESV_STYLE,
    RSVP_ERROR_UNKNOWN_RESV_STYLE,
    RSVP_ERROR_CONFLICT_DEST_PORTS,
    RSVP_ERROR_CONFLICT_SRC_PORTS,
    RSVP_ERROR_PREEMPTED=12,
    RSVP_ERROR_UNKNOWN_CLASS,
    RSVP_ERROR_UNKNOWN_C_TYPE,
    RSVP_ERROR_TRAFFIC = 21,
    RSVP_ERROR_TRAFFIC_SYSTEM,
    RSVP_ERROR_SYSTEM,
    RSVP_ERROR_ROUTING,
    RSVP_ERROR_NOTIFY,
    RSVP_ERROR_DIFFSERV = 27
};

enum {
    RSVP_AC_ERROR_DELAY_BOUND_ERROR = 1,
    RSVP_AC_ERROR_BANDWITH_UNAVAILABLE,
    RSVP_AC_ERROR_LARGE_MTU
};

enum {
    RSVP_TRAFFIC_CONTROL_ERROR_SERVICE_CONFLICT = 1,
    RSVP_TRAFFIC_CONTROL_ERROR_SERVIEC_UNSUPPORTED,
    RSVP_TRAFFIC_CONTROL_ERROR_BAD_FLOWSPEC,
    RSVP_TRAFFIC_CONTROL_ERROR_BAD_TSPEC,
    RSVP_TRAFFIC_CONTROL_ERROR_BAD_ADSPEC
};

enum {
    RSVP_ROUTING_ERROR_BAD_ERO = 1,
    RSVP_ROUTING_ERROR_BAD_STRICT,
    RSVP_ROUTING_ERROR_BAD_LOOSE,
    RSVP_ROUTING_ERROR_BAD_INITIAL_SUBOBJ,
    RSVP_ROUTING_ERROR_NO_ROUTE,
    RSVP_ROUTING_ERROR_UNACCEPTABLE_LABEL,
    RSVP_ROUTING_ERROR_RRO_LOOP,
    RSVP_ROUTING_ERROR_NON_RSVP_CAPABLE_ROUTER,
    RSVP_ROUTING_ERROR_LABEL_ALLOC_FAIL,
    RSVP_ROUTING_ERROR_UNSUPPORTED_L3PID
};

enum {
    RSVP_NOTIFY_ERROR_RRO_TOO_LARGE = 1,
    RSVP_NOTIFY_ERROR_RRO_NOTIFICATION,
    RSVP_NOTIFY_ERROR_RRO_TUNNEL_LOCAL_REPAIRED
};

enum {
    RSVP_DIFFSERV_ERROR_UNEXPECTED_DIFFSERVOBJ = 1,
    RSVP_DIFFSERV_ERROR_UNSUPPORTED_PHB,
    RSVP_DIFFSERV_ERROR_INVALID_EXP_PHB_MAPPING,
    RSVP_DIFFSERV_ERROR_UNSUPPORTED_PSC,
    RSVP_DIFFSERV_ERROR_PERLSP_CONTEXT_ALLOC_FAIL
};

static value_string rsvp_error_codes[] = {
    {RSVP_ERROR_CONFIRM, "Confirmation"},
    {RSVP_ERROR_ADMISSION, "Admission Control Failure "},
    {RSVP_ERROR_POLICY, "Policy Control Failure"},
    {RSVP_ERROR_NO_PATH, "No PATH information for this RESV message"},
    {RSVP_ERROR_NO_SENDER, "No sender information for this RESV message"},
    {RSVP_ERROR_CONFLICT_RESV_STYLE, "Conflicting reservation styles"},
    {RSVP_ERROR_UNKNOWN_RESV_STYLE, "Unknown reservation style"},
    {RSVP_ERROR_CONFLICT_DEST_PORTS, "Conflicting destination ports"},
    {RSVP_ERROR_CONFLICT_SRC_PORTS, "Conflicting source ports"},
    {RSVP_ERROR_PREEMPTED, "Service preempted"},
    {RSVP_ERROR_UNKNOWN_CLASS, "Unknown object class"},
    {RSVP_ERROR_UNKNOWN_C_TYPE, "Unknown object C-type"},
    {RSVP_ERROR_TRAFFIC, "Traffic Control Error"},
    {RSVP_ERROR_TRAFFIC_SYSTEM, "Traffic Control System Error"},
    {RSVP_ERROR_SYSTEM, "RSVP System Error"},
    {RSVP_ERROR_ROUTING, "Routing Error"},
    {RSVP_ERROR_NOTIFY, "RSVP Notify Error"},
    {RSVP_ERROR_DIFFSERV, "RSVP Diff-Serv Error"},
    {0, NULL}
};

static value_string rsvp_admission_control_error_vals[] = {
    {RSVP_AC_ERROR_DELAY_BOUND_ERROR, "Delay bound cannot be met"},
    {RSVP_AC_ERROR_BANDWITH_UNAVAILABLE, "Requested bandwidth unavailable"},
    {RSVP_AC_ERROR_LARGE_MTU, "MTU in flowspec larger than interface MTU"},
    {0, NULL}
};

static value_string rsvp_traffic_control_error_vals[] = { 
    {RSVP_TRAFFIC_CONTROL_ERROR_SERVICE_CONFLICT, "Service conflict"},
    {RSVP_TRAFFIC_CONTROL_ERROR_SERVIEC_UNSUPPORTED, "Service unsupported"},
    {RSVP_TRAFFIC_CONTROL_ERROR_BAD_FLOWSPEC, "Bad Flowspec value"},
    {RSVP_TRAFFIC_CONTROL_ERROR_BAD_TSPEC, "Bad Tspec value"},
    {RSVP_TRAFFIC_CONTROL_ERROR_BAD_ADSPEC, "Bad Adspec value"},
    {0, NULL}
};

static value_string rsvp_routing_error_vals[] = {
    {RSVP_ROUTING_ERROR_BAD_ERO, "Bad EXPLICIT_ROUTE object"},
    {RSVP_ROUTING_ERROR_BAD_STRICT, "Bad strict node"},
    {RSVP_ROUTING_ERROR_BAD_LOOSE, "Bad loose node"},
    {RSVP_ROUTING_ERROR_BAD_INITIAL_SUBOBJ, "Bad initial subobject"},
    {RSVP_ROUTING_ERROR_NO_ROUTE, "No route available toward destination"},
    {RSVP_ROUTING_ERROR_UNACCEPTABLE_LABEL, "Unacceptable label value"},
    {RSVP_ROUTING_ERROR_RRO_LOOP, "RRO indicated routing loops"},
    {RSVP_ROUTING_ERROR_NON_RSVP_CAPABLE_ROUTER, "non-RSVP-capable router stands in the path"},
    {RSVP_ROUTING_ERROR_LABEL_ALLOC_FAIL, "MPLS label allocation failure"},
    {RSVP_ROUTING_ERROR_UNSUPPORTED_L3PID, "Unsupported L3PID"},
    {0, NULL}
};

static value_string rsvp_notify_error_vals[] = {
    {RSVP_NOTIFY_ERROR_RRO_TOO_LARGE, "RRO too large for MTU"},
    {RSVP_NOTIFY_ERROR_RRO_NOTIFICATION, "RRO Notification"},
    {RSVP_NOTIFY_ERROR_RRO_TUNNEL_LOCAL_REPAIRED, "Tunnel locally repaired"},
    {0, NULL}
};

static value_string rsvp_diffserv_error_vals[] = {
    {RSVP_DIFFSERV_ERROR_UNEXPECTED_DIFFSERVOBJ, "Unexpected DIFFSERV object"},
    {RSVP_DIFFSERV_ERROR_UNSUPPORTED_PHB, "Unsupported PHB"},
    {RSVP_DIFFSERV_ERROR_INVALID_EXP_PHB_MAPPING, "Invalid `EXP<->PHB mapping'"},
    {RSVP_DIFFSERV_ERROR_UNSUPPORTED_PSC, "Unsupported PSC"},
    {RSVP_DIFFSERV_ERROR_PERLSP_CONTEXT_ALLOC_FAIL, "Per-LSP context allocation failure"},
    {0, NULL}
};

/*
 * Defines the reservation style plus style-specific information that
 * is not a FLOWSPEC or FILTER_SPEC object, in a RESV message.
 */
#define RSVP_DISTINCT (1 << 3)
#define RSVP_SHARED (2 << 3)
#define RSVP_SHARING_MASK (RSVP_DISTINCT | RSVP_SHARED)

#define RSVP_SCOPE_WILD 1
#define RSVP_SCOPE_EXPLICIT 2
#define RSVP_SCOPE_MASK 0x07

#define RSVP_WF (RSVP_SHARED | RSVP_SCOPE_WILD)
#define RSVP_FF (RSVP_DISTINCT | RSVP_SCOPE_EXPLICIT)
#define RSVP_SE (RSVP_SHARED | RSVP_SCOPE_EXPLICIT)

static value_string style_vals[] = {
    { RSVP_WF, "Wildcard Filter" },
    { RSVP_FF, "Fixed Filter" },
    { RSVP_SE, "Shared-Explicit" },
    { 0,       NULL }
};

enum {
    RSVP_SESSION_TYPE_IPV4 = 1,
    RSVP_SESSION_TYPE_IPV6,

    RSVP_SESSION_TYPE_IPV4_LSP = 7,
    RSVP_SESSION_TYPE_IPV6_LSP,

    RSVP_SESSION_TYPE_IPV4_UNI = 9
};

/*
 * Defines a desired QoS, in a RESV message.
 */
enum    qos_service_type {
    QOS_QUALITATIVE =     128,          /* Qualitative service */
    QOS_NULL =              6,          /* Null service (RFC2997) */
    QOS_CONTROLLED_LOAD=    5,		/* Controlled Load Service */
    QOS_GUARANTEED =        2,		/* Guaranteed service */
    QOS_TSPEC =             1		/* Traffic specification */
    };

static value_string qos_vals[] = {
    { QOS_QUALITATIVE, "Qualitative QoS" },
    { QOS_NULL, "Null-Service QoS" },
    { QOS_CONTROLLED_LOAD, "Controlled-load QoS" },
    { QOS_GUARANTEED, "Guaranteed rate QoS" },
    { QOS_TSPEC, "Traffic specification" },
    { 0, NULL }
};

static value_string svc_vals[] = {
    { 126, "Compression Hint" },
    { 127, "Token bucket" },
    { 128, "Null Service" },
    { 130, "Guaranteed-rate RSpec" },
    { 0, NULL }
};

enum rsvp_spec_types { INTSRV = 2 };

enum intsrv_services {
	INTSRV_GENERAL = 1,
	INTSRV_GTD = 2,
	INTSRV_CLOAD = 5,
	INTSRV_NULL = 6,
	INTSRV_QUALITATIVE = 128
};

static value_string intsrv_services_str[] = {
    {INTSRV_GENERAL, "Default General Parameters"},
    {INTSRV_GTD, "Guaranteed Rate"},
    {INTSRV_CLOAD, "Controlled Load"},
    {INTSRV_NULL, "Null Service"},
    {INTSRV_QUALITATIVE, "Null Service"},
    { 0, NULL }
};

#if 0
enum intsrv_field_name {
	INTSRV_NON_IS_HOPS = 1, INTSRV_COMPOSED_NON_IS_HOPS,
	INTSRV_IS_HOPS, INTSRV_COMPOSED_IS_HOPS,
	INTSRV_PATH_BANDWIDTH, INTSRV_MIN_PATH_BANDWIDTH,
	INTSRV_IF_LATENCY, INTSRV_PATH_LATENCY,
	INTSRV_MTU, INTSRV_COMPOSED_MTU,

	INTSRV_TOKEN_BUCKET_TSPEC = 127,
	INTSRV_QUALITATIVE_TSPEC = 128,
	INTSRV_GTD_RSPEC = 130,

    	INTSRV_DELAY = 131,	/* Gtd Parameter C - Max Delay Bound - bytes */
	INTSRV_MAX_JITTER,	/* Gtd Parameter D - Max Jitter */
    	INTSRV_E2E_DELAY,	/* Gtd Parameter Ctot */
	INTSRV_E2E_MAX_JITTER,	/* Gtd Parameter Dtot */
    	INTSRV_SHP_DELAY,	/* Gtd Parameter Csum */
	INTSRV_SHP_MAX_JITTER	/* Gtd Parameter Dsum */
};
#endif

static value_string adspec_params[] = {
    {4, "IS Hop Count"},
    {6, "Path b/w estimate"},
    {8, "Minimum path latency"},
    {10, "Composed MTU"},
    {133, "End-to-end composed value for C"},
    {134, "End-to-end composed value for D"},
    {135, "Since-last-reshaping point composed C"},
    {136, "Since-last-reshaping point composed D"},
    { 0, NULL }
};

const value_string gmpls_lsp_enc_str[] = {
    { 1, "Packet"},
    { 2, "Ethernet v2/DIX"},
    { 3, "ANSI PDH"},
    { 5, "SONET/SDH"},
    { 7, "Digital Wrapper"},
    { 8, "Lambda (photonic)"},
    { 9, "Fiber"},
    {11, "FiberChannel"},
    { 0, NULL }
};

const value_string gmpls_switching_type_str[] = {
    {  1, "Packet-Switch Capable-1 (PSC-1)"},
    {  2, "Packet-Switch Capable-2 (PSC-2)"},
    {  3, "Packet-Switch Capable-3 (PSC-3)"},
    {  4, "Packet-Switch Capable-4 (PSC-4)"},
    { 51, "Layer-2 Switch Capable (L2SC)"},
    {100, "Time-Division-Multiplex Capable (TDM)"},
    {150, "Lambda-Switch Capable (LSC)"},
    {200, "Fiber-Switch Capable (FSC)"},
    { 0, NULL }
};

static const value_string gmpls_gpid_str[] = {
    { 5, "Asynchronous mapping of E3 (SDH)"},
    { 8, "Bit synchronous mapping of E3 (SDH)"},
    { 9, "Byte synchronous mapping of E3 (SDH)"},
    {10, "Asynchronous mapping of DS2/T2 (SDH)"},
    {11, "Bit synchronous mapping of DS2/T2 (SONET, SDH)"},
    {13, "Asynchronous mapping of E1 (SONET, SDH)"},
    {14, "Byte synchronous mapping of E1 (SONET, SDH)"},
    {15, "Byte synchronous mapping of 31 * DS0 (SONET, SDH)"},
    {16, "Asynchronous mapping of DS1/T1 (SONET, SDH)"},
    {17, "Bit synchronous mapping of DS1/T1 (SONET, SDH)"},
    {18, "Byte synchronous mapping of DS1/T1 (SONET, SDH)"},
    {19, "VC-11 in VC-12 (SDH)"},
    {22, "DS1 SF Asynchronous (SONET)"},
    {23, "DS1 ESF Asynchronous (SONET)"},
    {24, "DS3 M23 Asynchronous (SONET)"},
    {25, "DS3 C-Bit Parity Asynchronous (SONET)"},
    {26, "VT/LOVC (SONET, SDH)"},
    {27, "STS SPE/HOVC (SONET, SDH)"},
    {28, "POS - No Scrambling, 16 bit CRC (SONET, SDH)"},
    {29, "POS - No Scrambling, 32 bit CRC (SONET, SDH)"},
    {30, "POS - Scrambling, 16 bit CRC (SONET, SDH)"},
    {31, "POS - Scrambling, 32 bit CRC (SONET, SDH)"},
    {32, "ATM Mapping (SONET, SDH)"},
    {33, "Ethernet (SDH, Lambda, Fiber)"},
    {34, "SDH (Lambda, Fiber)"},
    {35, "SONET (Lambda, Fiber)"},
    {36, "Digital Wrapper (Lambda, Fiber)"},
    {37, "Lambda (Fiber)"},
    {38, "ETSI PDH (SDH)"},
    {39, "ANSI PDH (SONET, SDH)"},
    {40, "Link Access Protocol SDH: LAPS - X.85 and X.86 (SONET, SDH)"},
    {41, "FDDI (SONET, SDH, Lambda, Fiber)"},
    {42, "DQDB: ETSI ETS 300 216 (SONET, SDH)"},
    {43, "FiberChannel-3 Services (FiberChannel)"},
    {44, "HDLC"},
    {45, "Ethernet V2/DIX (only)"},
    {46, "Ethernet 802.3 (only)"},
    { 0, NULL },
};

static const value_string gmpls_sonet_signal_type_str[] = {
    { 1, "VT1.5 SPE / VC-11"},
    { 2, "VT2 SPE / VC-12"},
    { 3, "VT3 SPE"},
    { 4, "VT6 SPE / VC-2"},
    { 5, "STS-1 SPE / VC-3"},
    { 6, "STS-3c SPE / VC-4"},
    { 7, "STS-1 / STM-0 (transp)"},
    { 8, "STS-3 / STM-1 (transp)"},
    { 9, "STS-12 / STM-4 (transp)"},
    {10, "STS-48 / STM-16 (transp)"},
    {11, "STS-192 / STM-64 (transp)"},
    {12, "STS-768 / STM-256 (transp)"},

    /* Extended non-SONET signal types */
    {13, "VTG / TUG-2"},
    {14, "TUG-3"},
    {15, "STSG-3 / AUG-1"},
    {16, "STSG-12  / AUG-4"},
    {17, "STSG-48  / AUG-16"},
    {18, "STSG-192 / AUG-64"},
    {19, "STSG-768 / AUG-256"},
};

static const value_string ouni_guni_diversity_str[] = {
    {1, "Node Diverse"},
    {2, "Link Diverse"},
    {3, "Shared-Risk Link Group Diverse"},
    {4, "Shared Path"},
};

/* -------------------- Stuff for MPLS/TE objects -------------------- */

static const value_string proto_vals[] = { {IP_PROTO_ICMP, "ICMP"},
                                           {IP_PROTO_IGMP, "IGMP"},
                                           {IP_PROTO_TCP,  "TCP" },
                                           {IP_PROTO_UDP,  "UDP" },
                                           {IP_PROTO_OSPF, "OSPF"},
                                           {0,             NULL  } };

/* Filter keys */
enum rsvp_filter_keys {

    /* Message types */
    RSVPF_MSG,          /* Message type */
    /* Shorthand for message types */
    RSVPF_PATH,
    RSVPF_RESV,
    RSVPF_PATHERR,
    RSVPF_RESVERR,
    RSVPF_PATHTEAR,
    RSVPF_RESVTEAR,
    RSVPF_RCONFIRM,
    RSVPF_JUNK_MSG8,
    RSVPF_JUNK_MSG9,
    RSVPF_RTEARCONFIRM,
    RSVPF_JUNK11,
    RSVPF_BUNDLE,
    RSVPF_ACK,
    RSVPF_JUNK14,
    RSVPF_SREFRESH,
    RSVPF_JUNK16,
    RSVPF_JUNK17,
    RSVPF_JUNK18,
    RSVPF_JUNK19,
    RSVPF_HELLO,
    /* Does the message contain an object of this type? */
    RSVPF_OBJECT,
    /* Object present shorthands */
    RSVPF_SESSION,
    RSVPF_DUMMY_1,
    RSVPF_HOP,
    RSVPF_INTEGRITY,
    RSVPF_TIME_VALUES,
    RSVPF_ERROR,
    RSVPF_SCOPE,
    RSVPF_STYLE,
    RSVPF_FLOWSPEC,
    RSVPF_FILTER_SPEC,
    RSVPF_SENDER,
    RSVPF_TSPEC,
    RSVPF_ADSPEC,
    RSVPF_POLICY,
    RSVPF_CONFIRM,
    RSVPF_LABEL,
    RSVPF_DUMMY_2,
    RSVPF_DUMMY_3,
    RSVPF_LABEL_REQUEST,
    RSVPF_EXPLICIT_ROUTE,
    RSVPF_RECORD_ROUTE,
    RSVPF_HELLO_OBJ,
    RSVPF_MESSAGE_ID,
    RSVPF_MESSAGE_ID_ACK,
    RSVPF_MESSAGE_ID_LIST,
    RSVPF_RECOVERY_LABEL,
    RSVPF_UPSTREAM_LABEL,
    RSVPF_LABEL_SET,
    RSVPF_PROTECTION,
    RSVPF_DIFFSERV,

    RSVPF_SUGGESTED_LABEL,
    RSVPF_ACCEPTABLE_LABEL_SET,
    RSVPF_RESTART_CAP,

    RSVPF_SESSION_ATTRIBUTE,
    RSVPF_DCLASS,
    RSVPF_LSP_TUNNEL_IF_ID,
    RSVPF_NOTIFY_REQUEST,
    RSVPF_ADMIN_STATUS,
    RSVPF_GENERALIZED_UNI,
    RSVPF_UNKNOWN_OBJ,

    /* Session object */
    RSVPF_SESSION_IP,
    RSVPF_SESSION_PROTO,
    RSVPF_SESSION_PORT,
    RSVPF_SESSION_TUNNEL_ID,
    RSVPF_SESSION_EXT_TUNNEL_ID,

    /* Sender template */
    RSVPF_SENDER_IP,
    RSVPF_SENDER_PORT,
    RSVPF_SENDER_LSP_ID,

    /* Diffserv object */
    RSVPF_DIFFSERV_MAPNB,
    RSVPF_DIFFSERV_MAP,
    RSVPF_DIFFSERV_MAP_EXP,
    RSVPF_DIFFSERV_PHBID,
    RSVPF_DIFFSERV_PHBID_DSCP,
    RSVPF_DIFFSERV_PHBID_CODE,
    RSVPF_DIFFSERV_PHBID_BIT14,
    RSVPF_DIFFSERV_PHBID_BIT15,

    /* Sentinel */
    RSVPF_MAX
};

static int rsvp_filter[RSVPF_MAX];

static hf_register_info rsvpf_info[] = {

    /* Message type number */
    {&rsvp_filter[RSVPF_MSG],
     { "Message Type", "rsvp.msg", FT_UINT8, BASE_DEC, VALS(message_type_vals), 0x0,
     	"", HFILL }},

    /* Message type shorthands */
    {&rsvp_filter[RSVPF_PATH],
     { "Path Message", "rsvp.path", FT_BOOLEAN, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_RESV],
     { "Resv Message", "rsvp.resv", FT_BOOLEAN, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_PATHERR],
     { "Path Error Message", "rsvp.perr", FT_BOOLEAN, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_RESVERR],
     { "Resv Error Message", "rsvp.rerr", FT_BOOLEAN, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_PATHTEAR],
     { "Path Tear Message", "rsvp.ptear", FT_BOOLEAN, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_RESVTEAR],
     { "Resv Tear Message", "rsvp.rtear", FT_BOOLEAN, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_RCONFIRM],
     { "Resv Confirm Message", "rsvp.resvconf", FT_BOOLEAN, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_RTEARCONFIRM],
     { "Resv Tear Confirm Message", "rsvp.rtearconf", FT_BOOLEAN, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_BUNDLE],
     { "Bundle Message", "rsvp.bundle", FT_BOOLEAN, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_ACK],
     { "Ack Message", "rsvp.ack", FT_BOOLEAN, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_SREFRESH],
     { "Srefresh Message", "rsvp.srefresh", FT_BOOLEAN, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_HELLO],
     { "HELLO Message", "rsvp.hello", FT_BOOLEAN, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    /* Object class */
    {&rsvp_filter[RSVPF_OBJECT],
     { "Object class", "rsvp.object", FT_UINT8, BASE_DEC, VALS(rsvp_class_vals), 0x0,
     	"", HFILL }},

    /* Object present shorthands */
    {&rsvp_filter[RSVPF_SESSION],
     { "SESSION", "rsvp.session", FT_NONE, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_HOP],
     { "HOP", "rsvp.hop", FT_NONE, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_HELLO_OBJ],
     { "HELLO Request/Ack", "rsvp.hello_obj", FT_NONE, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_INTEGRITY],
     { "INTEGRITY", "rsvp.integrity", FT_NONE, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_TIME_VALUES],
     { "TIME VALUES", "rsvp.time", FT_NONE, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_ERROR],
     { "ERROR", "rsvp.error", FT_NONE, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_SCOPE],
     { "SCOPE", "rsvp.scope", FT_NONE, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_STYLE],
     { "STYLE", "rsvp.style", FT_NONE, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_FLOWSPEC],
     { "FLOWSPEC", "rsvp.flowspec", FT_NONE, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_FILTER_SPEC],
     { "FILTERSPEC", "rsvp.filter", FT_NONE, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_SENDER],
     { "SENDER TEMPLATE", "rsvp.sender", FT_NONE, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_TSPEC],
     { "SENDER TSPEC", "rsvp.tspec", FT_NONE, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_ADSPEC],
     { "ADSPEC", "rsvp.adspec", FT_NONE, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_POLICY],
     { "POLICY", "rsvp.policy", FT_NONE, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_CONFIRM],
     { "CONFIRM", "rsvp.confirm", FT_NONE, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_LABEL],
     { "LABEL", "rsvp.label", FT_NONE, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_RECOVERY_LABEL],
     { "RECOVERY LABEL", "rsvp.recovery_label", FT_NONE, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_UPSTREAM_LABEL],
     { "UPSTREAM LABEL", "rsvp.upstream_label", FT_NONE, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_SUGGESTED_LABEL],
     { "SUGGESTED LABEL", "rsvp.suggested_label", FT_NONE, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_LABEL_SET],
     { "RESTRICTED LABEL SET", "rsvp.label_set", FT_NONE, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_ACCEPTABLE_LABEL_SET],
     { "ACCEPTABLE LABEL SET", "rsvp.acceptable_label_set", FT_NONE, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_PROTECTION],
     { "PROTECTION", "rsvp.protection", FT_NONE, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_DIFFSERV],
     { "DIFFSERV", "rsvp.diffserv", FT_NONE, BASE_NONE, NULL, 0x0,
        "", HFILL }},

    {&rsvp_filter[RSVPF_RESTART_CAP],
     { "RESTART CAPABILITY", "rsvp.restart", FT_NONE, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_LABEL_REQUEST],
     { "LABEL REQUEST", "rsvp.label_request", FT_NONE, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_SESSION_ATTRIBUTE],
     { "SESSION ATTRIBUTE", "rsvp.session_attribute", FT_NONE, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_EXPLICIT_ROUTE],
     { "EXPLICIT ROUTE", "rsvp.explicit_route", FT_NONE, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_RECORD_ROUTE],
     { "RECORD ROUTE", "rsvp.record_route", FT_NONE, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_MESSAGE_ID],
     { "MESSAGE-ID", "rsvp.msgid", FT_NONE, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_MESSAGE_ID_ACK],
     { "MESSAGE-ID ACK", "rsvp.ack", FT_NONE, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_MESSAGE_ID_LIST],
     { "MESSAGE-ID LIST", "rsvp.msgid_list", FT_NONE, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_DCLASS],
     { "DCLASS", "rsvp.dclass", FT_NONE, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_LSP_TUNNEL_IF_ID],
     { "LSP INTERFACE-ID", "rsvp.lsp_tunnel_if_id", FT_NONE, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_ADMIN_STATUS],
     { "ADMIN STATUS", "rsvp.admin_status", FT_NONE, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_NOTIFY_REQUEST],
     { "NOTIFY REQUEST", "rsvp.notify_request", FT_NONE, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_GENERALIZED_UNI],
     { "GENERALIZED UNI", "rsvp.generalized_uni", FT_NONE, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_UNKNOWN_OBJ],
     { "Unknown object", "rsvp.obj_unknown", FT_NONE, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    /* Session fields */
    {&rsvp_filter[RSVPF_SESSION_IP],
     { "Destination address", "rsvp.session.ip", FT_IPv4, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_SESSION_PORT],
     { "Port number", "rsvp.session.port", FT_UINT16, BASE_DEC, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_SESSION_PROTO],
     { "Protocol", "rsvp.session.proto", FT_UINT8, BASE_DEC, VALS(proto_vals), 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_SESSION_TUNNEL_ID],
     { "Tunnel ID", "rsvp.session.tunnel_id", FT_UINT16, BASE_DEC, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_SESSION_EXT_TUNNEL_ID],
     { "Extended tunnel ID", "rsvp.session.ext_tunnel_id", FT_UINT32, BASE_DEC, NULL, 0x0,
     	"", HFILL }},

    /* Sender template/Filterspec fields */
    {&rsvp_filter[RSVPF_SENDER_IP],
     { "Sender IPv4 address", "rsvp.sender.ip", FT_IPv4, BASE_NONE, NULL, 0x0,
     	"", HFILL }},

    {&rsvp_filter[RSVPF_SENDER_PORT],
     { "Sender port number", "rsvp.sender.port", FT_UINT16, BASE_DEC, NULL, 0x0,
       "", HFILL }},

    {&rsvp_filter[RSVPF_SENDER_LSP_ID],
     { "Sender LSP ID", "rsvp.sender.lsp_id", FT_UINT16, BASE_DEC, NULL, 0x0,
     	"", HFILL }},

    /* Diffserv object fields */
    {&rsvp_filter[RSVPF_DIFFSERV_MAPNB],
     { "MAPnb", "rsvp.diffserv.mapnb", FT_UINT8, BASE_DEC, NULL, 0x0,
       MAPNB_DESCRIPTION, HFILL }},

    {&rsvp_filter[RSVPF_DIFFSERV_MAP],
     { "MAP", "rsvp.diffserv.map", FT_NONE, BASE_NONE, NULL, 0x0,
       MAP_DESCRIPTION, HFILL }},

    {&rsvp_filter[RSVPF_DIFFSERV_MAP_EXP],
     { "EXP", "rsvp.diffserv.map.exp", FT_UINT8, BASE_DEC, NULL, 0x0,
       EXP_DESCRIPTION, HFILL }},

    {&rsvp_filter[RSVPF_DIFFSERV_PHBID],
     { "PHBID", "rsvp.diffserv.phbid", FT_NONE, BASE_NONE, NULL, 0x0,
       PHBID_DESCRIPTION, HFILL }},

    {&rsvp_filter[RSVPF_DIFFSERV_PHBID_DSCP],
     { PHBID_DSCP_DESCRIPTION, "rsvp.diffserv.phbid.dscp", FT_UINT16,
       BASE_DEC, NULL, PHBID_DSCP_MASK, "DSCP", HFILL }},

    {&rsvp_filter[RSVPF_DIFFSERV_PHBID_CODE],
     { PHBID_CODE_DESCRIPTION, "rsvp.diffserv.phbid.code", FT_UINT16,
       BASE_DEC, NULL, PHBID_CODE_MASK, "PHB id code", HFILL }},

    {&rsvp_filter[RSVPF_DIFFSERV_PHBID_BIT14],
     { PHBID_BIT14_DESCRIPTION, "rsvp.diffserv.phbid.bit14", FT_UINT16,
       BASE_DEC, VALS(phbid_bit14_vals), PHBID_BIT14_MASK, "Bit 14", HFILL }},

    {&rsvp_filter[RSVPF_DIFFSERV_PHBID_BIT15],
     { PHBID_BIT15_DESCRIPTION, "rsvp.diffserv.phbid.bit15", FT_UINT16,
       BASE_DEC, VALS(phbid_bit15_vals), PHBID_BIT15_MASK, "Bit 15", HFILL }}

};

static inline int rsvp_class_to_filter_num(int classnum)
{
    switch(classnum) {
    case RSVP_CLASS_SESSION :
    case RSVP_CLASS_HOP :
    case RSVP_CLASS_INTEGRITY :
    case RSVP_CLASS_TIME_VALUES :
    case RSVP_CLASS_ERROR :
    case RSVP_CLASS_SCOPE :
    case RSVP_CLASS_STYLE :
    case RSVP_CLASS_FLOWSPEC :
    case RSVP_CLASS_FILTER_SPEC :
    case RSVP_CLASS_SENDER_TEMPLATE :
    case RSVP_CLASS_SENDER_TSPEC :
    case RSVP_CLASS_ADSPEC :
    case RSVP_CLASS_POLICY :
    case RSVP_CLASS_CONFIRM :
    case RSVP_CLASS_LABEL :
    case RSVP_CLASS_LABEL_REQUEST :
    case RSVP_CLASS_HELLO :
    case RSVP_CLASS_EXPLICIT_ROUTE :
    case RSVP_CLASS_RECORD_ROUTE :
    case RSVP_CLASS_MESSAGE_ID :
    case RSVP_CLASS_MESSAGE_ID_ACK :
    case RSVP_CLASS_MESSAGE_ID_LIST :
	return classnum + RSVPF_OBJECT;
	break;

    case RSVP_CLASS_RECOVERY_LABEL :
    case RSVP_CLASS_UPSTREAM_LABEL :
    case RSVP_CLASS_LABEL_SET :
    case RSVP_CLASS_PROTECTION :
	return RSVPF_RECOVERY_LABEL + (classnum - RSVP_CLASS_RECOVERY_LABEL);

    case RSVP_CLASS_SUGGESTED_LABEL :
    case RSVP_CLASS_ACCEPTABLE_LABEL_SET :
    case RSVP_CLASS_RESTART_CAP :
	return RSVPF_SUGGESTED_LABEL + (classnum - RSVP_CLASS_SUGGESTED_LABEL);

    case RSVP_CLASS_DIFFSERV :
	return RSVPF_DIFFSERV;

    case RSVP_CLASS_NOTIFY_REQUEST :
	return RSVPF_NOTIFY_REQUEST;
    case RSVP_CLASS_ADMIN_STATUS :
	return RSVPF_ADMIN_STATUS;

    case RSVP_CLASS_SESSION_ATTRIBUTE :
	return RSVPF_SESSION_ATTRIBUTE;
    case RSVP_CLASS_GENERALIZED_UNI :
	return RSVPF_GENERALIZED_UNI;
    case RSVP_CLASS_DCLASS :
	return RSVPF_DCLASS;
    case RSVP_CLASS_LSP_TUNNEL_IF_ID :
	return RSVPF_LSP_TUNNEL_IF_ID;

    default:
	return RSVPF_UNKNOWN_OBJ;
    }
}

static void
find_rsvp_session_tempfilt(tvbuff_t *tvb, int hdr_offset, int *session_offp, int *tempfilt_offp)
{
    int s_off = 0, t_off = 0;
    int len, off;
    guint16 obj_length;

    if (!tvb_bytes_exist(tvb, hdr_offset+6, 2))
	goto done;

    len = tvb_get_ntohs(tvb, hdr_offset+6) + hdr_offset;
    off = hdr_offset + 8;
    for (off = hdr_offset + 8; off < len && tvb_bytes_exist(tvb, off, 3);
    	 off += obj_length) {
	obj_length = tvb_get_ntohs(tvb, off);
	if (obj_length == 0)
	    break;
	switch(tvb_get_guint8(tvb, off+2)) {
	case RSVP_CLASS_SESSION:
	    s_off = off;
	    break;
	case RSVP_CLASS_SENDER_TEMPLATE:
	case RSVP_CLASS_FILTER_SPEC:
	    t_off = off;
	    break;
	default:
	    break;
	}
    }

 done:
    if (session_offp) *session_offp = s_off;
    if (tempfilt_offp) *tempfilt_offp = t_off;
}

static char *summary_session (tvbuff_t *tvb, int offset)
{
    static char buf[80];

    switch(tvb_get_guint8(tvb, offset+3)) {
    case RSVP_SESSION_TYPE_IPV4:
	snprintf(buf, 80, "SESSION: IPv4, Destination %s, Protocol %d, Port %d. ",
		 ip_to_str(tvb_get_ptr(tvb, offset+4, 4)),
		 tvb_get_guint8(tvb, offset+8),
		 tvb_get_ntohs(tvb, offset+10));
	break;
    case RSVP_SESSION_TYPE_IPV4_LSP:
	snprintf(buf, 80, "SESSION: IPv4-LSP, Destination %s, Tunnel ID %d, Ext ID %0x. ",
		 ip_to_str(tvb_get_ptr(tvb, offset+4, 4)),
		 tvb_get_ntohs(tvb, offset+10),
		 tvb_get_ntohl(tvb, offset+12));
	break;
    case RSVP_SESSION_TYPE_IPV4_UNI:
	snprintf(buf, 80, "SESSION: IPv4-UNI, Destination %s, Tunnel ID %d, Ext Address %s. ",
		 ip_to_str(tvb_get_ptr(tvb, offset+4, 4)),
		 tvb_get_ntohs(tvb, offset+10),
		 ip_to_str(tvb_get_ptr(tvb, offset+12, 4)));
	break;
    default:
	snprintf(buf, 80, "SESSION: Type %d. ", tvb_get_guint8(tvb, offset+3));
    }

    return buf;
}

static char *summary_template (tvbuff_t *tvb, int offset)
{
    static char buf[80];
    char *objtype;

    if (tvb_get_guint8(tvb, offset+2) == RSVP_CLASS_FILTER_SPEC)
	objtype = "FILTERSPEC";
    else
	objtype = "SENDER TEMPLATE";

    switch(tvb_get_guint8(tvb, offset+3)) {
    case 1:
	snprintf(buf, 80, "%s: IPv4, Sender %s, Port %d. ", objtype,
		 ip_to_str(tvb_get_ptr(tvb, offset+4, 4)),
		 tvb_get_ntohs(tvb, offset+10));
	break;
    case 7:
	snprintf(buf, 80, "%s: IPv4-LSP, Tunnel Source: %s, LSP ID: %d. ", objtype,
		 ip_to_str(tvb_get_ptr(tvb, offset+4, 4)),
		 tvb_get_ntohs(tvb, offset+10));
	break;
    default:
	snprintf(buf, 80, "%s: Type %d. ", objtype, tvb_get_guint8(tvb, offset+3));
    }

    return buf;
}

/*------------------------------------------------------------------------------
 * SESSION
 *------------------------------------------------------------------------------*/
static void
dissect_rsvp_session (proto_tree *ti, tvbuff_t *tvb,
		      int offset, int obj_length,
		      int class, int type,
		      char *type_str)
{
    int offset2 = offset + 4;
    proto_tree *rsvp_object_tree;

    rsvp_object_tree = proto_item_add_subtree(ti, TREE(TT_SESSION));
    proto_tree_add_text(rsvp_object_tree, tvb, offset, 2,
			"Length: %u", obj_length);
    proto_tree_add_text(rsvp_object_tree, tvb, offset+2, 1,
			"Class number: %u - %s",
			class, type_str);
    proto_item_set_text(ti, summary_session(tvb, offset));

    switch(type) {
    case RSVP_SESSION_TYPE_IPV4:
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: 1 - IPv4");
	proto_tree_add_item(rsvp_object_tree,
			    rsvp_filter[RSVPF_SESSION_IP],
			    tvb, offset2, 4, FALSE);

	proto_tree_add_item(rsvp_object_tree,
			    rsvp_filter[RSVPF_SESSION_PROTO], tvb,
			    offset2+4, 1, FALSE);
	proto_tree_add_text(rsvp_object_tree, tvb, offset2+5, 1,
			    "Flags: %x",
			    tvb_get_guint8(tvb, offset2+5));
	proto_tree_add_item(rsvp_object_tree,
			    rsvp_filter[RSVPF_SESSION_PORT], tvb,
			    offset2+6, 2, FALSE);
	break;

    case RSVP_SESSION_TYPE_IPV6:
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: 2 - IPv6");
	proto_tree_add_text(rsvp_object_tree, tvb, offset2, 16,
			    "Destination address: %s",
			    ip6_to_str((const struct e_in6_addr *)tvb_get_ptr(tvb, offset2, 16)));
	proto_tree_add_text(rsvp_object_tree, tvb, offset2+16, 1,
			    "Protocol: %u",
			    tvb_get_guint8(tvb, offset2+16));
	proto_tree_add_text(rsvp_object_tree, tvb, offset2+17, 1,
			    "Flags: %x",
			    tvb_get_guint8(tvb, offset2+17));
	proto_tree_add_text(rsvp_object_tree, tvb, offset2+18, 2,
			    "Destination port: %u",
			    tvb_get_ntohs(tvb, offset2+18));
	break;

    case RSVP_SESSION_TYPE_IPV4_LSP:
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: 7 - IPv4 LSP");
	proto_tree_add_item(rsvp_object_tree,
			    rsvp_filter[RSVPF_SESSION_IP],
			    tvb, offset2, 4, FALSE);

	proto_tree_add_item(rsvp_object_tree,
			    rsvp_filter[RSVPF_SESSION_TUNNEL_ID],
			    tvb, offset2+6, 2, FALSE);

	proto_tree_add_text(rsvp_object_tree, tvb, offset2+8, 4,
			    "Extended Tunnel ID: %u (%s)",
			    tvb_get_ntohl(tvb, offset2+8),
			    ip_to_str(tvb_get_ptr(tvb, offset2+8, 4)));
	proto_tree_add_item_hidden(rsvp_object_tree,
				   rsvp_filter[RSVPF_SESSION_EXT_TUNNEL_ID],
				   tvb, offset2+8, 4, FALSE);
	break;

    case RSVP_SESSION_TYPE_IPV4_UNI:
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: 9 - IPv4 UNI");
	proto_tree_add_item(rsvp_object_tree,
			    rsvp_filter[RSVPF_SESSION_IP],
			    tvb, offset2, 4, FALSE);

	proto_tree_add_item(rsvp_object_tree,
			    rsvp_filter[RSVPF_SESSION_TUNNEL_ID],
			    tvb, offset2+6, 2, FALSE);

	proto_tree_add_text(rsvp_object_tree, tvb, offset2+8, 4,
			    "Extended IPv4 Address: %s",
			    ip_to_str(tvb_get_ptr(tvb, offset2+8, 4)));
	proto_tree_add_item_hidden(rsvp_object_tree,
				   rsvp_filter[RSVPF_SESSION_EXT_TUNNEL_ID],
				   tvb, offset2+8, 4, FALSE);
	break;

    default:
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: Unknown (%u)",
			    type);
	proto_tree_add_text(rsvp_object_tree, tvb, offset2, obj_length-4,
			    "Data (%d bytes)", obj_length-4);
    }
}


/*------------------------------------------------------------------------------
 * TLVs for HOP, ERROR and other IF_ID extended objects
 *------------------------------------------------------------------------------*/
static void
dissect_rsvp_ifid_tlv (proto_tree *ti, proto_tree *rsvp_object_tree, 
		       tvbuff_t *tvb, int offset, int obj_length, 
		       int subtree_type)
{
    guint16   tlv_off;
    guint16   tlv_type;
    guint16   tlv_len;
    char     *ifindex_name;
    proto_tree *rsvp_ifid_subtree, *ti2;
    int       offset2 = offset + 4;

    for (tlv_off = 0; tlv_off < obj_length - 12; ) {
	tlv_type = tvb_get_ntohs(tvb, offset+tlv_off);
	tlv_len = tvb_get_ntohs(tvb, offset+tlv_off+2);
	switch(tlv_type) {
	case 1:
	    ti2 = proto_tree_add_text(rsvp_object_tree, tvb,
				      offset+tlv_off, 8,
				      "IPv4 TLV - %s",
				      ip_to_str(tvb_get_ptr(tvb, offset+tlv_off+4, 4)));

	    rsvp_ifid_subtree = proto_item_add_subtree(ti2, subtree_type);
	    proto_tree_add_text(rsvp_ifid_subtree, tvb, offset+tlv_off, 2,
				"Type: 1 (IPv4)");
	    proto_tree_add_text(rsvp_ifid_subtree, tvb, offset+tlv_off+2, 2,
				"Length: %u",
				tvb_get_ntohs(tvb, offset+tlv_off+2));
	    proto_tree_add_text(rsvp_ifid_subtree, tvb, offset+tlv_off+4, 4,
				"IPv4 address: %s",
				ip_to_str(tvb_get_ptr(tvb, offset+tlv_off+4, 4)));
	    proto_item_append_text(ti, "Data IPv4: %s. ",
				   ip_to_str(tvb_get_ptr(tvb, offset+tlv_off+4, 4)));
	    break;
	    
	case 3:
	    ifindex_name = "";
	    goto ifid_ifindex;
	case 4:
	    ifindex_name = " Forward";
	    goto ifid_ifindex;
	case 5:
	    ifindex_name = " Reverse";
	ifid_ifindex:
	    ti2 = proto_tree_add_text(rsvp_object_tree, tvb,
				      offset+tlv_off, 12,
				      "Interface-Index%s TLV - %s, %d",
				      ifindex_name,
				      ip_to_str(tvb_get_ptr(tvb, offset+tlv_off+4, 4)),
				      tvb_get_ntohl(tvb, offset+tlv_off+8));
	    rsvp_ifid_subtree =	proto_item_add_subtree(ti2, subtree_type);
	    proto_tree_add_text(rsvp_ifid_subtree, tvb, offset+tlv_off, 2,
				"Type: %d (Interface Index%s)", tlv_type, ifindex_name);
	    proto_tree_add_text(rsvp_ifid_subtree, tvb, offset+tlv_off+2, 2,
				"Length: %u",
				tvb_get_ntohs(tvb, offset+tlv_off+2));
	    proto_tree_add_text(rsvp_ifid_subtree, tvb, offset+tlv_off+4, 4,
				"IPv4 address: %s",
				ip_to_str(tvb_get_ptr(tvb, offset+tlv_off+4, 4)));
	    proto_tree_add_text(rsvp_ifid_subtree, tvb, offset+tlv_off+8, 4,
				"Interface-ID: %d",
				tvb_get_ntohl(tvb, offset+tlv_off+8));
	    proto_item_append_text(ti, "Data If-Index%s: %s, %d. ", ifindex_name,
				   ip_to_str(tvb_get_ptr(tvb, offset+tlv_off+4, 4)),
				   tvb_get_ntohl(tvb, offset+tlv_off+8));
	    break;
	    
	default:
	    proto_tree_add_text(rsvp_object_tree, tvb, offset2+4, 4,
				"Logical interface: %u",
				tvb_get_ntohl(tvb, offset2+4));
	}
	tlv_off += tlv_len;
    }
}

/*------------------------------------------------------------------------------
 * HOP
 *------------------------------------------------------------------------------*/
static void
dissect_rsvp_hop (proto_tree *ti, tvbuff_t *tvb,
		  int offset, int obj_length,
		  int class, int type,
		  char *type_str)
{
    int offset2 = offset + 4;
    proto_tree *rsvp_object_tree;

    rsvp_object_tree= proto_item_add_subtree(ti, TREE(TT_HOP));

    proto_tree_add_text(rsvp_object_tree, tvb, offset, 2,
			"Length: %u", obj_length);
    proto_tree_add_text(rsvp_object_tree, tvb, offset+2, 1,
			"Class number: %u - %s",
			class, type_str);
    switch(type) {
    case 1:
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: 1 - IPv4");
	proto_tree_add_text(rsvp_object_tree, tvb, offset2, 4,
			    "Neighbor address: %s",
			    ip_to_str(tvb_get_ptr(tvb, offset2, 4)));
	proto_tree_add_text(rsvp_object_tree, tvb, offset2+4, 4,
			    "Logical interface: %u",
			    tvb_get_ntohl(tvb, offset2+4));
	proto_item_set_text(ti, "HOP: IPv4, %s",
			    ip_to_str(tvb_get_ptr(tvb, offset2, 4)));
	break;

    case 2:
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: 2 - IPv6");
	proto_tree_add_text(rsvp_object_tree, tvb, offset2, 16,
			    "Neighbor address: %s",
			    ip6_to_str((const struct e_in6_addr *)tvb_get_ptr(tvb, offset2, 16)));
	proto_tree_add_text(rsvp_object_tree, tvb, offset2+16, 4,
			    "Logical interface: 0x%08x",
			    tvb_get_ntohl(tvb, offset2+16));
	break;

    case 3:
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: 3 - IPv4 IF-ID");
	proto_tree_add_text(rsvp_object_tree, tvb, offset2, 4,
			    "Neighbor address: %s",
			    ip_to_str(tvb_get_ptr(tvb, offset2, 4)));
	proto_tree_add_text(rsvp_object_tree, tvb, offset2+4, 4,
			    "Logical interface: %u",
			    tvb_get_ntohl(tvb, offset2+4));

	proto_item_set_text(ti, "HOP: IPv4 IF-ID. Control IPv4: %s. ",
			    ip_to_str(tvb_get_ptr(tvb, offset2, 4)));

	dissect_rsvp_ifid_tlv(ti, rsvp_object_tree, tvb, offset+12, obj_length, 
			      TREE(TT_HOP_SUBOBJ));
			      
	break;

    default:
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: Unknown (%u)",
			    type);
	proto_tree_add_text(rsvp_object_tree, tvb, offset2, obj_length - 4,
			    "Data (%d bytes)", obj_length - 4);
    }
}

/*------------------------------------------------------------------------------
 * TIME VALUES
 *------------------------------------------------------------------------------*/
static void
dissect_rsvp_time_values (proto_tree *ti, tvbuff_t *tvb,
			  int offset, int obj_length,
			  int class, int type,
			  char *type_str)
{
    int offset2 = offset + 4;
    proto_tree *rsvp_object_tree;

    rsvp_object_tree = proto_item_add_subtree(ti, TREE(TT_TIME_VALUES));
    proto_tree_add_text(rsvp_object_tree, tvb, offset, 2,
			"Length: %u", obj_length);
    proto_tree_add_text(rsvp_object_tree, tvb, offset+2, 1,
			"Class number: %u - %s",
			class, type_str);
    switch(type) {
    case 1:
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: 1");
	proto_tree_add_text(rsvp_object_tree, tvb, offset2, 4,
			    "Refresh interval: %u ms (%u seconds)",
			    tvb_get_ntohl(tvb, offset2),
			    tvb_get_ntohl(tvb, offset2)/1000);
	proto_item_set_text(ti, "TIME VALUES: %d ms",
			    tvb_get_ntohl(tvb, offset2));
	break;

    default:
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: Unknown (%u)",
			    type);
	proto_tree_add_text(rsvp_object_tree, tvb, offset2, obj_length - 4,
			    "Data (%d bytes)", obj_length - 4);
	break;
    }
}

/*------------------------------------------------------------------------------
 * Error value field in ERROR object
 *------------------------------------------------------------------------------*/
static guint16
dissect_rsvp_error_value (proto_tree *ti, tvbuff_t *tvb,
                          int offset, guint8 error_code)
{
    guint16 error_val;
    value_string *rsvp_error_vals = NULL;

    error_val = tvb_get_ntohs(tvb, offset);
    switch (error_code) {
    case RSVP_ERROR_ADMISSION:
	rsvp_error_vals = rsvp_admission_control_error_vals;
	break;
    case RSVP_ERROR_TRAFFIC:
	rsvp_error_vals = rsvp_traffic_control_error_vals;
	break;
    case RSVP_ERROR_ROUTING:
	rsvp_error_vals = rsvp_routing_error_vals;
	break;
    case RSVP_ERROR_NOTIFY:
	rsvp_error_vals = rsvp_notify_error_vals;
	break;
    case RSVP_ERROR_DIFFSERV:
	rsvp_error_vals = rsvp_diffserv_error_vals;
    }
    switch (error_code) {
    case RSVP_ERROR_ADMISSION:
    case RSVP_ERROR_TRAFFIC:
    case RSVP_ERROR_NOTIFY:
    case RSVP_ERROR_ROUTING:
    case RSVP_ERROR_DIFFSERV:
	if ((error_val & 0xc0) == 0) {
	    proto_tree_add_text(ti, tvb, offset, 2,
		"Error value: %u - %s", error_val,
		val_to_str(error_val, rsvp_error_vals, "Unknown (%d)"));
	    break;
	}
    default:
	proto_tree_add_text(ti, tvb, offset, 2,
	    "Error value: %u", error_val);
    }
    return error_val;
}

/*------------------------------------------------------------------------------
 * ERROR
 *------------------------------------------------------------------------------*/
static void
dissect_rsvp_error (proto_tree *ti, tvbuff_t *tvb,
		    int offset, int obj_length,
		    int class, int type,
		    char *type_str)
{
    int offset2 = offset + 4;
    proto_tree *rsvp_object_tree;

    rsvp_object_tree = proto_item_add_subtree(ti, TREE(TT_ERROR));
    proto_tree_add_text(rsvp_object_tree, tvb, offset, 2,
			"Length: %u", obj_length);
    proto_tree_add_text(rsvp_object_tree, tvb, offset+2, 1,
			"Class number: %u - %s",
			class, type_str);
    switch(type) {
    case 1: {
	guint8 error_code;
	guint16 error_val;

	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: 1 - IPv4");
	proto_tree_add_text(rsvp_object_tree, tvb, offset2, 4,
			    "Error node: %s",
			    ip_to_str(tvb_get_ptr(tvb, offset2, 4)));
	proto_tree_add_text(rsvp_object_tree, tvb, offset2+4, 1,
			    "Flags: 0x%02x",
			    tvb_get_guint8(tvb, offset2+4));
	error_code = tvb_get_guint8(tvb, offset2+5);
	proto_tree_add_text(rsvp_object_tree, tvb, offset2+5, 1,
			    "Error code: %u - %s", error_code,
			    val_to_str(error_code, rsvp_error_codes, "Unknown (%d)"));
	error_val = dissect_rsvp_error_value(rsvp_object_tree, tvb, offset2+6, error_code);
	proto_item_set_text(ti, "ERROR: IPv4, Error code: %s, Value: %d, Error Node: %s",
			    val_to_str(error_code, rsvp_error_codes, "Unknown (%d)"),
			    error_val, ip_to_str(tvb_get_ptr(tvb, offset2, 4)));
	break;
    }

    case 2: {
	guint8 error_code;
	guint16 error_val;

	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: 2 - IPv6");
	proto_tree_add_text(rsvp_object_tree, tvb, offset2, 16,
			    "Error node: %s",
			    ip6_to_str((const struct e_in6_addr *)tvb_get_ptr(tvb, offset2, 16)));
	proto_tree_add_text(rsvp_object_tree, tvb, offset2+16, 1,
			    "Flags: 0x%02x",
			    tvb_get_guint8(tvb, offset2+16));
	error_code = tvb_get_guint8(tvb, offset2+17);
	proto_tree_add_text(rsvp_object_tree, tvb, offset2+17, 1,
			    "Error code: %u - %s",
			    error_code,
			    val_to_str(error_code, rsvp_error_codes, "Unknown"));
	error_val = dissect_rsvp_error_value(ti, tvb, offset2+18, error_code);
	break;
    }

    case 3: {
	guint8 error_code;
	guint16 error_val;

	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: 3 - IPv4 IF-ID");
	proto_tree_add_text(rsvp_object_tree, tvb, offset2, 4,
			    "Error node: %s",
			    ip_to_str(tvb_get_ptr(tvb, offset2, 4)));
	proto_tree_add_text(rsvp_object_tree, tvb, offset2+4, 1,
			    "Flags: 0x%02x",
			    tvb_get_guint8(tvb, offset2+4));
	error_code = tvb_get_guint8(tvb, offset2+5);
	proto_tree_add_text(rsvp_object_tree, tvb, offset2+5, 1,
			    "Error code: %u - %s", error_code,
			    val_to_str(error_code, rsvp_error_codes, "Unknown (%d)"));
	error_val = dissect_rsvp_error_value(ti, tvb, offset2+6, error_code);
	proto_item_set_text(ti, "ERROR: IPv4 IF-ID, Error code: %s, Value: %d, Control Node: %s. ",
			    val_to_str(error_code, rsvp_error_codes, "Unknown (%d)"),
			    error_val, ip_to_str(tvb_get_ptr(tvb, offset2, 4)));
	dissect_rsvp_ifid_tlv(ti, rsvp_object_tree, tvb, offset+12, obj_length, 
			      TREE(TT_ERROR_SUBOBJ));
	break;
    }

    default:
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: Unknown (%u)",
			    type);
	proto_tree_add_text(rsvp_object_tree, tvb, offset2, obj_length - 4,
			    "Data (%d bytes)", obj_length - 4);
    }
}

/*------------------------------------------------------------------------------
 * SCOPE
 *------------------------------------------------------------------------------*/
static void
dissect_rsvp_scope (proto_tree *ti, tvbuff_t *tvb,
		    int offset, int obj_length,
		    int class, int type,
		    char *type_str)
{
    int offset2 = offset + 4;
    proto_tree *rsvp_object_tree;
    int mylen;

    rsvp_object_tree = proto_item_add_subtree(ti, TREE(TT_SCOPE));
    proto_tree_add_text(rsvp_object_tree, tvb, offset, 2,
			"Length: %u", obj_length);
    proto_tree_add_text(rsvp_object_tree, tvb, offset+2, 1,
			"Class number: %u - %s",
			class, type_str);
    mylen = obj_length - 4;
    switch(type) {
    case 1: {
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: 1 - IPv4");
	while (mylen > 0) {
	    proto_tree_add_text(rsvp_object_tree, tvb, offset2, 4,
				"IPv4 Address: %s",
				ip_to_str(tvb_get_ptr(tvb, offset2, 4)));
	    offset2 += 4;
	    mylen -= 4;
	}
	break;
    }

    case 2: {
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: 2 - IPv6");
	while (mylen > 0) {
	    proto_tree_add_text(rsvp_object_tree, tvb, offset2, 16,
				"IPv6 Address: %s",
				ip6_to_str((const struct e_in6_addr *)tvb_get_ptr(tvb, offset2, 16)));
	    offset2 += 16;
	    mylen -= 16;
	}
	break;
    }

    default:
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: Unknown (%u)",
			    type);
	proto_tree_add_text(rsvp_object_tree, tvb, offset2, mylen,
			    "Data (%d bytes)", mylen);
    }
}

/*------------------------------------------------------------------------------
 * STYLE
 *------------------------------------------------------------------------------*/
static void
dissect_rsvp_style (proto_tree *ti, tvbuff_t *tvb,
		    int offset, int obj_length,
		    int class, int type,
		    char *type_str)
{
    int offset2 = offset + 4;
    proto_tree *rsvp_object_tree;

    rsvp_object_tree = proto_item_add_subtree(ti, TREE(TT_STYLE));
    proto_tree_add_text(rsvp_object_tree, tvb, offset, 2,
			"Length: %u", obj_length);
    proto_tree_add_text(rsvp_object_tree, tvb, offset+2, 1,
			"Class number: %u - %s",
			class, type_str);
    switch(type) {
    case 1: {
	guint32 style;

	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: 1");
	proto_tree_add_text(rsvp_object_tree, tvb, offset2, 1,
			    "Flags: 0x%02x",
			    tvb_get_guint8(tvb, offset2));
	style = tvb_get_ntoh24(tvb, offset2+1);
	proto_tree_add_text(rsvp_object_tree, tvb, offset2+1,
			    3, "Style: 0x%06X - %s", style,
			    val_to_str(style, style_vals, "Unknown"));
	proto_item_set_text(ti, "STYLE: %s (%d)",
			    val_to_str(style, style_vals, "Unknown"),
			    style);
	break;
    }

    default:
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: Unknown (%u)",
			    type);
	proto_tree_add_text(rsvp_object_tree, tvb, offset2, obj_length - 4,
			    "Data (%d bytes)", obj_length - 4);
	break;
    }
}

/*------------------------------------------------------------------------------
 * CONFIRM
 *------------------------------------------------------------------------------*/
static void
dissect_rsvp_confirm (proto_tree *ti, tvbuff_t *tvb,
		      int offset, int obj_length,
		      int class, int type,
		      char *type_str)
{
    int offset2 = offset + 4;
    proto_tree *rsvp_object_tree;

    rsvp_object_tree = proto_item_add_subtree(ti, TREE(TT_CONFIRM));
    proto_tree_add_text(rsvp_object_tree, tvb, offset, 2,
			"Length: %u", obj_length);
    proto_tree_add_text(rsvp_object_tree, tvb, offset+2, 1,
			"Class number: %u - %s",
			class, type_str);
    switch(type) {
    case 1: {
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: 1 - IPv4");
	proto_tree_add_text(rsvp_object_tree, tvb, offset2, 4,
			    "Receiver address: %s",
			    ip_to_str(tvb_get_ptr(tvb, offset2, 4)));
	proto_item_set_text(ti, "CONFIRM: Receiver %s",
			    ip_to_str(tvb_get_ptr(tvb, offset2, 4)));
	break;
    }

    case 2: {
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: 2 - IPv6");
	proto_tree_add_text(rsvp_object_tree, tvb, offset2, 16,
			    "Receiver address: %s",
			    ip6_to_str((const struct e_in6_addr *)tvb_get_ptr(tvb, offset2, 16)));
	break;
    }

    default:
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: Unknown (%u)",
			    type);
	proto_tree_add_text(rsvp_object_tree, tvb, offset2, obj_length - 4,
			    "Data (%d bytes)", obj_length - 4);
    }
}

/*------------------------------------------------------------------------------
 * SENDER TEMPLATE and FILTERSPEC
 *------------------------------------------------------------------------------*/
static void
dissect_rsvp_template_filter (proto_tree *ti, tvbuff_t *tvb,
			      int offset, int obj_length,
			      int class, int type,
			      char *type_str)
{
    int offset2 = offset + 4;
    proto_tree *rsvp_object_tree;

    switch(class) {
    case RSVP_CLASS_SENDER_TEMPLATE :
	rsvp_object_tree = proto_item_add_subtree(ti, TREE(TT_SENDER_TEMPLATE));
	proto_tree_add_text(rsvp_object_tree, tvb, offset, 2,
			    "Length: %u", obj_length);
	proto_tree_add_text(rsvp_object_tree, tvb, offset+2, 1,
			    "Class number: %u - %s",
			    class, type_str);
	break;

    default:
    case RSVP_CLASS_FILTER_SPEC :
	rsvp_object_tree = proto_item_add_subtree(ti, TREE(TT_FILTER_SPEC));
	proto_tree_add_text(rsvp_object_tree, tvb, offset, 2,
			    "Length: %u", obj_length);
	proto_tree_add_text(rsvp_object_tree, tvb, offset+2, 1,
			    "Class number: %u - %s",
			    class, type_str);
	break;
    }

     proto_item_set_text(ti, summary_template(tvb, offset));
     switch(type) {
     case 1:
	 proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			     "C-type: 1 - IPv4");
	 proto_tree_add_item(rsvp_object_tree,
			     rsvp_filter[RSVPF_SENDER_IP],
			     tvb, offset2, 4, FALSE);
	 proto_tree_add_item(rsvp_object_tree,
			     rsvp_filter[RSVPF_SENDER_PORT],
			     tvb, offset2+6, 2, FALSE);
	 break;

     case 2:
	 proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			     "C-type: 2 - IPv6");
	 proto_tree_add_text(rsvp_object_tree, tvb, offset2, 16,
			     "Source address: %s",
			     ip6_to_str((const struct e_in6_addr *)tvb_get_ptr(tvb, offset2, 16)));
	 proto_tree_add_text(rsvp_object_tree, tvb, offset2+18, 2,
			     "Source port: %u",
			     tvb_get_ntohs(tvb, offset2+18));
	 break;

     case 7:
	 proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			     "C-type: 7 - IPv4 LSP");
	 proto_tree_add_item(rsvp_object_tree,
			     rsvp_filter[RSVPF_SENDER_IP],
			     tvb, offset2, 4, FALSE);
	 proto_tree_add_item(rsvp_object_tree,
			     rsvp_filter[RSVPF_SENDER_LSP_ID],
			     tvb, offset2+6, 2, FALSE);
	 break;

     default:
	 proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			     "C-type: Unknown (%u)",
			     type);
	 proto_tree_add_text(rsvp_object_tree, tvb, offset2, obj_length - 4,
			     "Data (%d bytes)", obj_length - 4);
     }
}

/*------------------------------------------------------------------------------
 * SENDER TSPEC
 *------------------------------------------------------------------------------*/
static void
dissect_rsvp_tspec (proto_tree *ti, tvbuff_t *tvb,
		    int offset, int obj_length,
		    int class, int type,
		    char *type_str)
{
    int offset2 = offset + 4;
    proto_tree *rsvp_object_tree;
    int mylen;
    proto_tree *tspec_tree, *ti2;
    guint8 signal_type;

    rsvp_object_tree = proto_item_add_subtree(ti, TREE(TT_TSPEC));
    proto_tree_add_text(rsvp_object_tree, tvb, offset, 2,
			"Length: %u", obj_length);
    proto_tree_add_text(rsvp_object_tree, tvb, offset+2, 1,
			"Class number: %u - %s",
			class, type_str);
    mylen = obj_length - 4;

    switch(type) {
    case 2:
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: 1 - Integrated Services");
	proto_tree_add_text(rsvp_object_tree, tvb, offset2, 1,
			    "Message format version: %u",
			    tvb_get_guint8(tvb, offset2)>>4);
	proto_tree_add_text(rsvp_object_tree, tvb, offset2+2, 2,
			    "Data length: %u words, not including header",
			    tvb_get_ntohs(tvb, offset2+2));

	mylen -= 4;
	offset2 += 4;

	proto_item_set_text(ti, "SENDER TSPEC: IntServ: ");

	while (mylen > 0) {
	    guint8 service_num;
	    guint8 param_id;
	    guint16 param_len;
	    guint16 param_len_processed;
	    guint16 length;

	    service_num = tvb_get_guint8(tvb, offset2);
	    proto_tree_add_text(rsvp_object_tree, tvb, offset2, 1,
				"Service header: %u - %s",
				service_num,
				val_to_str(service_num, qos_vals, "Unknown"));
	    length = tvb_get_ntohs(tvb, offset2+2);
	    proto_tree_add_text(rsvp_object_tree, tvb, offset2+2, 2,
				"Length of service %u data: %u words, "
				"not including header",
				service_num, length);

	    mylen -= 4;
	    offset2 += 4;

	    /* Process all known service headers as a set of parameters */
	    param_len_processed = 0;
	    while (param_len_processed < length) {
		param_id = tvb_get_guint8(tvb, offset2);
		param_len = tvb_get_ntohs(tvb, offset2+2) + 1;
		switch(param_id) {
		case 127:
		    /* Token Bucket */
		    ti2 = proto_tree_add_text(rsvp_object_tree, tvb,
					      offset2, param_len*4,
					      "Token Bucket TSpec: ");
		    tspec_tree = proto_item_add_subtree(ti2, TREE(TT_TSPEC_SUBTREE));

		    proto_tree_add_text(tspec_tree, tvb, offset2, 1,
					"Parameter %u - %s",
					param_id,
					val_to_str(param_id, svc_vals, "Unknown"));
		    proto_tree_add_text(tspec_tree, tvb, offset2+1, 1,
					"Parameter %u flags: 0x%02x",
					param_id,
					tvb_get_guint8(tvb, offset2+1));
		    proto_tree_add_text(tspec_tree, tvb, offset2+2, 2,
					"Parameter %u data length: %u words, "
					"not including header",
					param_id,
					tvb_get_ntohs(tvb, offset2+2));
		    proto_tree_add_text(tspec_tree, tvb, offset2+4, 4,
					"Token bucket rate: %.10g",
					tvb_get_ntohieee_float(tvb, offset2+4));
		    proto_tree_add_text(tspec_tree, tvb, offset2+8, 4,
					"Token bucket size: %.10g",
					tvb_get_ntohieee_float(tvb, offset2+8));
		    proto_tree_add_text(tspec_tree, tvb, offset2+12, 4,
					"Peak data rate: %.10g",
					tvb_get_ntohieee_float(tvb, offset2+12));
		    proto_tree_add_text(tspec_tree, tvb, offset2+16, 4,
					"Minimum policed unit [m]: %u",
					tvb_get_ntohl(tvb, offset2+16));
		    proto_tree_add_text(tspec_tree, tvb, offset2+20, 4,
					"Maximum packet size [M]: %u",
					tvb_get_ntohl(tvb, offset2+20));
		    proto_item_append_text(ti, "Token Bucket, %.10g bytes/sec. ",
					   tvb_get_ntohieee_float(tvb, offset2+4));
		    proto_item_append_text(ti2, "Rate=%.10g Burst=%.10g Peak=%.10g m=%u M=%u",
					   tvb_get_ntohieee_float(tvb, offset2+4),
					   tvb_get_ntohieee_float(tvb, offset2+8),
					   tvb_get_ntohieee_float(tvb, offset2+12),
					   tvb_get_ntohl(tvb, offset2+16),
					   tvb_get_ntohl(tvb, offset2+20));
		    break;

		case 128:
		    /* Null Service (RFC2997) */
		    ti2 = proto_tree_add_text(rsvp_object_tree, tvb,
					      offset2, param_len*4,
					      "Null Service TSpec: ");
		    tspec_tree = proto_item_add_subtree(ti2, TREE(TT_TSPEC_SUBTREE));

		    proto_tree_add_text(tspec_tree, tvb, offset2, 1,
					"Parameter %u - %s",
					param_id,
					val_to_str(param_id, svc_vals, "Unknown"));
		    proto_tree_add_text(tspec_tree, tvb, offset2+1, 1,
					"Parameter %u flags: %x",
					param_id,
					tvb_get_guint8(tvb, offset2+1));
		    proto_tree_add_text(tspec_tree, tvb, offset2+2, 2,
					"Parameter %u data length: %u words, "
					"not including header",
					param_id,
					tvb_get_ntohs(tvb, offset2+2));
		    proto_tree_add_text(tspec_tree, tvb, offset2+4, 4,
					"Maximum packet size [M]: %u",
					tvb_get_ntohl(tvb, offset2+4));
		    proto_item_append_text(ti, "Null Service. M=%u",
					   tvb_get_ntohl(tvb, offset2+4));
		    proto_item_append_text(ti2, "Max pkt size=%u",
					   tvb_get_ntohl(tvb, offset2+4));
		    break;

		case 126:
		    /* Compression hint (RFC3006) */
		    ti2 = proto_tree_add_text(rsvp_object_tree, tvb,
					      offset2, param_len*4,
					      "Compression Hint: ");
		    tspec_tree = proto_item_add_subtree(ti2, TREE(TT_TSPEC_SUBTREE));

		    proto_tree_add_text(tspec_tree, tvb, offset2, 1,
					"Parameter %u - %s",
					param_id,
					val_to_str(param_id, svc_vals, "Unknown"));
		    proto_tree_add_text(tspec_tree, tvb, offset2+1, 1,
					"Parameter %u flags: %x",
					param_id,
					tvb_get_guint8(tvb, offset2+1));
		    proto_tree_add_text(tspec_tree, tvb, offset2+2, 2,
					"Parameter %u data length: %u words, "
					"not including header",
					param_id,
					tvb_get_ntohs(tvb, offset2+2));
		    proto_tree_add_text(tspec_tree, tvb, offset2+4, 4,
					"Hint: %u",
					tvb_get_ntohl(tvb, offset2+4));
		    proto_tree_add_text(tspec_tree, tvb, offset2+4, 4,
					"Compression Factor: %u",
					tvb_get_ntohl(tvb, offset2+8));
		    proto_item_append_text(ti, "Compression Hint. Hint=%u, Factor=%u",
					   tvb_get_ntohl(tvb, offset2+4),
					   tvb_get_ntohl(tvb, offset2+8));
		    proto_item_append_text(ti2, "Hint=%u, Factor=%u",
					   tvb_get_ntohl(tvb, offset2+4),
					   tvb_get_ntohl(tvb, offset2+8));
		    break;

		default:
		    proto_tree_add_text(rsvp_object_tree, tvb, offset2, param_len*4,
					"Unknown parameter %d, %d words",
					param_id, param_len);
		    break;
		}
		param_len_processed += param_len;
		offset2 += param_len*4;
	    }
	    mylen -= length*4;
	}
	break;

    case 4: /* SONET/SDH Tspec */
	proto_item_set_text(ti, "SENDER TSPEC: SONET/SDH: ");

	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: 4 - SONET/SDH");
	signal_type = tvb_get_guint8(tvb, offset2);
	proto_tree_add_text(rsvp_object_tree, tvb, offset2, 1,
			    "Signal Type: %d - %s", signal_type,
			    val_to_str(signal_type,
				       gmpls_sonet_signal_type_str, "Unknown"));
	ti2 = proto_tree_add_text(rsvp_object_tree, tvb, offset2+1, 1,
			    "Requested Concatenation (RCC): %d", tvb_get_guint8(tvb, offset2+1));
	tspec_tree = proto_item_add_subtree(ti2, TREE(TT_TSPEC_SUBTREE));
	proto_tree_add_text(tspec_tree, tvb, offset2+1, 1,
			    decode_boolean_bitfield(tvb_get_guint8(tvb, offset2+1), 0x01, 8,
						    "Standard contiguous concatenation",
						    "No standard contiguous concatenation"));
	proto_tree_add_text(tspec_tree, tvb, offset2+1, 1,
			    decode_boolean_bitfield(tvb_get_guint8(tvb, offset2+1), 0x02, 8,
						    "Arbitrary contiguous concatenation",
						    "No arbitrary contiguous concatenation"));
	proto_tree_add_text(rsvp_object_tree, tvb, offset2+2, 2,
			    "Number of Contiguous Components (NCC): %d", tvb_get_ntohs(tvb, offset2+2));
	proto_tree_add_text(rsvp_object_tree, tvb, offset2+4, 2,
			    "Number of Virtual Components (NVC): %d", tvb_get_ntohs(tvb, offset2+4));
	proto_tree_add_text(rsvp_object_tree, tvb, offset2+6, 2,
			    "Multiplier (MT): %d", tvb_get_ntohs(tvb, offset2+6));
	ti2 = proto_tree_add_text(rsvp_object_tree, tvb, offset2+8, 4,
				  "Transparency (T): 0x%0x", tvb_get_ntohl(tvb, offset2+8));
	tspec_tree = proto_item_add_subtree(ti2, TREE(TT_TSPEC_SUBTREE));
	proto_tree_add_text(tspec_tree, tvb, offset2+8, 4,
			    decode_boolean_bitfield(tvb_get_ntohl(tvb, offset2+8), 0x0001, 32,
						    "Section/Regenerator Section layer transparency",
						    "No Section/Regenerator Section layer transparency"));
	proto_tree_add_text(tspec_tree, tvb, offset2+8, 4,
			    decode_boolean_bitfield(tvb_get_guint8(tvb, offset2+8), 0x0002, 32,
						    "Line/Multiplex Section layer transparency",
						    "No Line/Multiplex Section layer transparency"));
	proto_tree_add_text(tspec_tree, tvb, offset2+8, 4,
			    decode_boolean_bitfield(tvb_get_guint8(tvb, offset2+8), 0x0004, 32,
						    "J0 transparency",
						    "No J0 transparency"));
	proto_tree_add_text(tspec_tree, tvb, offset2+8, 4,
			    decode_boolean_bitfield(tvb_get_guint8(tvb, offset2+8), 0x0008, 32,
						    "SOH/RSOH DCC transparency",
						    "No SOH/RSOH DCC transparency"));
	proto_tree_add_text(tspec_tree, tvb, offset2+8, 4,
			    decode_boolean_bitfield(tvb_get_guint8(tvb, offset2+8), 0x0010, 32,
						    "LOH/MSOH DCC transparency",
						    "No LOH/MSOH DCC transparency"));
	proto_tree_add_text(tspec_tree, tvb, offset2+8, 4,
			    decode_boolean_bitfield(tvb_get_guint8(tvb, offset2+8), 0x0020, 32,
						    "LOH/MSOH Extended DCC transparency",
						    "No LOH/MSOH Extended DCC transparency"));
	proto_tree_add_text(tspec_tree, tvb, offset2+8, 4,
			    decode_boolean_bitfield(tvb_get_guint8(tvb, offset2+8), 0x0040, 32,
						    "K1/K2 transparency",
						    "No K1/K2 transparency"));
	proto_tree_add_text(tspec_tree, tvb, offset2+8, 4,
			    decode_boolean_bitfield(tvb_get_guint8(tvb, offset2+8), 0x0080, 32,
						    "E1 transparency",
						    "No E1 transparency"));
	proto_tree_add_text(tspec_tree, tvb, offset2+8, 4,
			    decode_boolean_bitfield(tvb_get_guint8(tvb, offset2+8), 0x0100, 32,
						    "F1 transparency",
						    "No F1 transparency"));
	proto_tree_add_text(tspec_tree, tvb, offset2+8, 4,
			    decode_boolean_bitfield(tvb_get_guint8(tvb, offset2+8), 0x0200, 32,
						    "E2 transparency",
						    "No E2 transparency"));
	proto_tree_add_text(tspec_tree, tvb, offset2+8, 4,
			    decode_boolean_bitfield(tvb_get_guint8(tvb, offset2+8), 0x0400, 32,
						    "B1 transparency",
						    "No B1 transparency"));
	proto_tree_add_text(tspec_tree, tvb, offset2+8, 4,
			    decode_boolean_bitfield(tvb_get_guint8(tvb, offset2+8), 0x0800, 32,
						    "B2 transparency",
						    "No B2 transparency"));
	proto_tree_add_text(tspec_tree, tvb, offset2+8, 4,
			    decode_boolean_bitfield(tvb_get_guint8(tvb, offset2+8), 0x1000, 32,
						    "M0 transparency",
						    "No M0 transparency"));
	proto_tree_add_text(tspec_tree, tvb, offset2+8, 4,
			    decode_boolean_bitfield(tvb_get_guint8(tvb, offset2+8), 0x2000, 32,
						    "M1 transparency",
						    "No M1 transparency"));
	proto_tree_add_text(rsvp_object_tree, tvb, offset2+12, 4,
			    "Profile (P): %d", tvb_get_ntohl(tvb, offset2+12));

	proto_item_append_text(ti, "Signal [%s], RCC %d, NCC %d, NVC %d, MT %d, Transparency %d, Profile %d",
			       val_to_str(signal_type, gmpls_sonet_signal_type_str, "Unknown"),
			       tvb_get_guint8(tvb, offset2+1), tvb_get_ntohs(tvb, offset2+2),
			       tvb_get_ntohs(tvb, offset2+4), tvb_get_ntohs(tvb, offset2+6),
			       tvb_get_ntohl(tvb, offset2+8), tvb_get_ntohl(tvb, offset2+12));
	break;

    default: /* Unknown TSpec */
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: Unknown (%u)",
			    type);
	proto_tree_add_text(rsvp_object_tree, tvb, offset2, obj_length - 4,
			    "Data (%d bytes)", obj_length - 4);
	break;

    }
}

/*------------------------------------------------------------------------------
 * FLOWSPEC
 *------------------------------------------------------------------------------*/
static void
dissect_rsvp_flowspec (proto_tree *ti, tvbuff_t *tvb,
		       int offset, int obj_length,
		       int class, int type,
		       char *type_str)
{
    int offset2 = offset + 4;
    proto_tree *rsvp_object_tree;
    int mylen, signal_type;
    proto_tree *flowspec_tree, *ti2;

    rsvp_object_tree = proto_item_add_subtree(ti, TREE(TT_FLOWSPEC));
    proto_tree_add_text(rsvp_object_tree, tvb, offset, 2,
			"Length: %u", obj_length);
    proto_tree_add_text(rsvp_object_tree, tvb, offset+2, 1,
			"Class number: %u - %s",
			class, type_str);
    proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			"C-type: %u", type);
    mylen = obj_length - 4;

    switch(type) {

    case 2:
	proto_tree_add_text(rsvp_object_tree, tvb, offset2, 1,
			    "Message format version: %u",
			    tvb_get_guint8(tvb, offset2)>>4);
	proto_tree_add_text(rsvp_object_tree, tvb, offset2+2, 2,
			    "Data length: %u words, not including header",
			    tvb_get_ntohs(tvb, offset2+2));

	proto_item_set_text(ti, "FLOWSPEC: ");

	mylen -= 4;
	offset2+= 4;
	while (mylen > 0) {
	    guint8 service_num;
	    guint16 length;
	    guint8 param_id;
	    guint16 param_len;
	    guint16 param_len_processed;

	    service_num = tvb_get_guint8(tvb, offset2);
	    proto_tree_add_text(rsvp_object_tree, tvb, offset2, 1,
				"Service header: %u - %s",
				service_num,
				val_to_str(service_num, intsrv_services_str, "Unknown"));
	    length = tvb_get_ntohs(tvb, offset2+2);
	    proto_tree_add_text(rsvp_object_tree, tvb, offset2+2, 2,
				"Length of service %u data: %u words, "
				"not including header",
				service_num,
				length);

	    mylen -= 4;
	    offset2 += 4;

	    proto_item_append_text(ti, "%s: ",
				   val_to_str(service_num, intsrv_services_str,
					      "Unknown (%d)"));

	    /* Process all known service headers as a set of parameters */
	    param_len_processed = 0;
	    while (param_len_processed < length) {
		param_id = tvb_get_guint8(tvb, offset2);
		param_len = tvb_get_ntohs(tvb, offset2+2) + 1;
		switch(param_id) {
		case 127:
		    /* Token Bucket */
		    ti2 = proto_tree_add_text(rsvp_object_tree, tvb,
					      offset2, param_len*4,
					      "Token Bucket: ");
		    flowspec_tree = proto_item_add_subtree(ti2, TREE(TT_FLOWSPEC_SUBTREE));

		    proto_tree_add_text(flowspec_tree, tvb, offset2, 1,
					"Parameter %u - %s",
					param_id,
					val_to_str(param_id, svc_vals, "Unknown"));
		    proto_tree_add_text(flowspec_tree, tvb, offset2+1, 1,
					"Parameter %u flags: 0x%02x",
					param_id,
					tvb_get_guint8(tvb, offset2+1));
		    proto_tree_add_text(flowspec_tree, tvb, offset2+2, 2,
					"Parameter %u data length: %u words, "
					"not including header",
					param_id,
					tvb_get_ntohs(tvb, offset2+2));
		    proto_tree_add_text(flowspec_tree, tvb, offset2+4, 4,
					"Token bucket rate: %.10g",
					tvb_get_ntohieee_float(tvb, offset2+4));
		    proto_tree_add_text(flowspec_tree, tvb, offset2+8, 4,
					"Token bucket size: %.10g",
					tvb_get_ntohieee_float(tvb, offset2+8));
		    proto_tree_add_text(flowspec_tree, tvb, offset2+12, 4,
					"Peak data rate: %.10g",
					tvb_get_ntohieee_float(tvb, offset2+12));
		    proto_tree_add_text(flowspec_tree, tvb, offset2+16, 4,
					"Minimum policed unit [m]: %u",
					tvb_get_ntohl(tvb, offset2+16));
		    proto_tree_add_text(flowspec_tree, tvb, offset2+20, 4,
					"Maximum packet size [M]: %u",
					tvb_get_ntohl(tvb, offset2+20));
		    proto_item_append_text(ti, "Token Bucket, %.10g bytes/sec. ",
					   tvb_get_ntohieee_float(tvb, offset2+4));
		    proto_item_append_text(ti2, "Rate=%.10g Burst=%.10g Peak=%.10g m=%u M=%u",
					   tvb_get_ntohieee_float(tvb, offset2+4),
					   tvb_get_ntohieee_float(tvb, offset2+8),
					   tvb_get_ntohieee_float(tvb, offset2+12),
					   tvb_get_ntohl(tvb, offset2+16),
					   tvb_get_ntohl(tvb, offset2+20));
		    break;

		case 130:
		    /* Guaranteed-rate RSpec */
		    ti2 = proto_tree_add_text(rsvp_object_tree, tvb,
					      offset2, param_len*4,
					      "Guaranteed-Rate RSpec: ");
		    flowspec_tree = proto_item_add_subtree(ti2, TREE(TT_FLOWSPEC_SUBTREE));
		    proto_tree_add_text(flowspec_tree, tvb, offset2, 1,
					"Parameter %u - %s",
					param_id,
					val_to_str(param_id, svc_vals, "Unknown"));
		    proto_tree_add_text(flowspec_tree, tvb, offset2+1, 1,
					"Parameter %u flags: %x",
					param_id,
					tvb_get_guint8(tvb, offset2+1));
		    proto_tree_add_text(flowspec_tree, tvb, offset2+2, 2,
					"Parameter %u data length: %u words, "
					"not including header",
					param_id,
					tvb_get_ntohs(tvb, offset2+2));

		    proto_tree_add_text(flowspec_tree, tvb, offset2+4, 4,
					"Rate: %.10g",
					tvb_get_ntohieee_float(tvb, offset2+4));
		    proto_tree_add_text(flowspec_tree, tvb, offset2+8, 4,
					"Slack term: %u",
					tvb_get_ntohl(tvb, offset2+8));
		    proto_item_append_text(ti, "RSpec, %.10g bytes/sec. ",
					   tvb_get_ntohieee_float(tvb, offset2+4));
		    proto_item_append_text(ti2, "R=%.10g, s=%u",
					   tvb_get_ntohieee_float(tvb, offset2+4),
					   tvb_get_ntohl(tvb, offset2+8));
		    break;

		case 128:
		    /* Null Service (RFC2997) */
		    ti2 = proto_tree_add_text(rsvp_object_tree, tvb,
					      offset2, param_len*4,
					      "Null Service Flowspec: ");
		    flowspec_tree = proto_item_add_subtree(ti2, TREE(TT_FLOWSPEC_SUBTREE));

		    proto_tree_add_text(flowspec_tree, tvb, offset2, 1,
					"Parameter %u - %s",
					param_id,
					val_to_str(param_id, svc_vals, "Unknown"));
		    proto_tree_add_text(flowspec_tree, tvb, offset2+1, 1,
					"Parameter %u flags: %x",
					param_id,
					tvb_get_guint8(tvb, offset2+1));
		    proto_tree_add_text(flowspec_tree, tvb, offset2+2, 2,
					"Parameter %u data length: %u words, "
					"not including header",
					param_id,
					tvb_get_ntohs(tvb, offset2+2));
		    proto_tree_add_text(flowspec_tree, tvb, offset2+4, 4,
					"Maximum packet size [M]: %u",
					tvb_get_ntohl(tvb, offset2+4));
		    proto_item_append_text(ti, "Null Service. M=%u",
					   tvb_get_ntohl(tvb, offset2+4));
		    proto_item_append_text(ti2, "Max pkt size=%u",
					   tvb_get_ntohl(tvb, offset2+4));
		    break;

		default:
		    proto_tree_add_text(rsvp_object_tree, tvb, offset2, param_len*4,
					"Unknown parameter %d, %d words",
					param_id, param_len);
		    break;
		}
		param_len_processed += param_len;
		offset2 += param_len * 4;
	    }

	    /* offset2 += length*4; */
	    mylen -= length*4;
	}
	break;

    case 4:
	proto_item_set_text(ti, "FLOWSPEC: SONET/SDH: ");

	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: 4 - SONET/SDH");
	signal_type = tvb_get_guint8(tvb, offset2);
	proto_tree_add_text(rsvp_object_tree, tvb, offset2, 1,
			    "Signal Type: %d - %s", signal_type,
			    val_to_str(signal_type,
				       gmpls_sonet_signal_type_str, "Unknown"));
	ti2 = proto_tree_add_text(rsvp_object_tree, tvb, offset2+1, 1,
				  "Requested Concatenation (RCC): %d", tvb_get_guint8(tvb, offset2+1));
	flowspec_tree = proto_item_add_subtree(ti2, TREE(TT_FLOWSPEC_SUBTREE));
	proto_tree_add_text(flowspec_tree, tvb, offset2+1, 1,
			    decode_boolean_bitfield(tvb_get_guint8(tvb, offset2+1), 0x01, 8,
						    "Standard contiguous concatenation",
						    "No standard contiguous concatenation"));
	proto_tree_add_text(flowspec_tree, tvb, offset2+1, 1,
			    decode_boolean_bitfield(tvb_get_guint8(tvb, offset2+1), 0x02, 8,
						    "Arbitrary contiguous concatenation",
						    "No arbitrary contiguous concatenation"));
	proto_tree_add_text(rsvp_object_tree, tvb, offset2+2, 2,
			    "Number of Contiguous Components (NCC): %d", tvb_get_ntohs(tvb, offset2+2));
	proto_tree_add_text(rsvp_object_tree, tvb, offset2+4, 2,
			    "Number of Virtual Components (NVC): %d", tvb_get_ntohs(tvb, offset2+4));
	proto_tree_add_text(rsvp_object_tree, tvb, offset2+6, 2,
			    "Multiplier (MT): %d", tvb_get_ntohs(tvb, offset2+6));
	ti2 = proto_tree_add_text(rsvp_object_tree, tvb, offset2+8, 4,
				  "Transparency (T): 0x%0x", tvb_get_ntohl(tvb, offset2+8));
	flowspec_tree = proto_item_add_subtree(ti2, TREE(TT_FLOWSPEC_SUBTREE));
	proto_tree_add_text(flowspec_tree, tvb, offset2+8, 4,
			    decode_boolean_bitfield(tvb_get_ntohl(tvb, offset2+8), 0x0001, 32,
						    "Section/Regenerator Section layer transparency",
						    "No Section/Regenerator Section layer transparency"));
	proto_tree_add_text(flowspec_tree, tvb, offset2+8, 4,
			    decode_boolean_bitfield(tvb_get_guint8(tvb, offset2+8), 0x0002, 32,
						    "Line/Multiplex Section layer transparency",
						    "No Line/Multiplex Section layer transparency"));
	proto_tree_add_text(flowspec_tree, tvb, offset2+8, 4,
			    decode_boolean_bitfield(tvb_get_guint8(tvb, offset2+8), 0x0004, 32,
						    "J0 transparency",
						    "No J0 transparency"));
	proto_tree_add_text(flowspec_tree, tvb, offset2+8, 4,
			    decode_boolean_bitfield(tvb_get_guint8(tvb, offset2+8), 0x0008, 32,
						    "SOH/RSOH DCC transparency",
						    "No SOH/RSOH DCC transparency"));
	proto_tree_add_text(flowspec_tree, tvb, offset2+8, 4,
			    decode_boolean_bitfield(tvb_get_guint8(tvb, offset2+8), 0x0010, 32,
						    "LOH/MSOH DCC transparency",
						    "No LOH/MSOH DCC transparency"));
	proto_tree_add_text(flowspec_tree, tvb, offset2+8, 4,
			    decode_boolean_bitfield(tvb_get_guint8(tvb, offset2+8), 0x0020, 32,
						    "LOH/MSOH Extended DCC transparency",
						    "No LOH/MSOH Extended DCC transparency"));
	proto_tree_add_text(flowspec_tree, tvb, offset2+8, 4,
			    decode_boolean_bitfield(tvb_get_guint8(tvb, offset2+8), 0x0040, 32,
						    "K1/K2 transparency",
						    "No K1/K2 transparency"));
	proto_tree_add_text(flowspec_tree, tvb, offset2+8, 4,
			    decode_boolean_bitfield(tvb_get_guint8(tvb, offset2+8), 0x0080, 32,
						    "E1 transparency",
						    "No E1 transparency"));
	proto_tree_add_text(flowspec_tree, tvb, offset2+8, 4,
			    decode_boolean_bitfield(tvb_get_guint8(tvb, offset2+8), 0x0100, 32,
						    "F1 transparency",
						    "No F1 transparency"));
	proto_tree_add_text(flowspec_tree, tvb, offset2+8, 4,
			    decode_boolean_bitfield(tvb_get_guint8(tvb, offset2+8), 0x0200, 32,
						    "E2 transparency",
						    "No E2 transparency"));
	proto_tree_add_text(flowspec_tree, tvb, offset2+8, 4,
			    decode_boolean_bitfield(tvb_get_guint8(tvb, offset2+8), 0x0400, 32,
						    "B1 transparency",
						    "No B1 transparency"));
	proto_tree_add_text(flowspec_tree, tvb, offset2+8, 4,
			    decode_boolean_bitfield(tvb_get_guint8(tvb, offset2+8), 0x0800, 32,
						    "B2 transparency",
						    "No B2 transparency"));
	proto_tree_add_text(flowspec_tree, tvb, offset2+8, 4,
			    decode_boolean_bitfield(tvb_get_guint8(tvb, offset2+8), 0x1000, 32,
						    "M0 transparency",
						    "No M0 transparency"));
	proto_tree_add_text(flowspec_tree, tvb, offset2+8, 4,
			    decode_boolean_bitfield(tvb_get_guint8(tvb, offset2+8), 0x2000, 32,
						    "M1 transparency",
						    "No M1 transparency"));
	proto_tree_add_text(rsvp_object_tree, tvb, offset2+12, 4,
			    "Profile (P): %d", tvb_get_ntohl(tvb, offset2+12));

	proto_item_append_text(ti, "Signal [%s], RCC %d, NCC %d, NVC %d, MT %d, Transparency %d, Profile %d",
			       val_to_str(signal_type, gmpls_sonet_signal_type_str, "Unknown"),
			       tvb_get_guint8(tvb, offset2+1), tvb_get_ntohs(tvb, offset2+2),
			       tvb_get_ntohs(tvb, offset2+4), tvb_get_ntohs(tvb, offset2+6),
			       tvb_get_ntohl(tvb, offset2+8), tvb_get_ntohl(tvb, offset2+12));
	break;

    default:
	break;
    }


}

/*------------------------------------------------------------------------------
 * ADSPEC
 *------------------------------------------------------------------------------*/
static void
dissect_rsvp_adspec (proto_tree *ti, tvbuff_t *tvb,
		     int offset, int obj_length,
		     int class, int type,
		     char *type_str)
{
    int offset2 = offset + 4;
    proto_tree *rsvp_object_tree;
    int mylen, i;
    proto_tree *adspec_tree;

    rsvp_object_tree = proto_item_add_subtree(ti, TREE(TT_ADSPEC));
    proto_tree_add_text(rsvp_object_tree, tvb, offset, 2,
			"Length: %u", obj_length);
    proto_tree_add_text(rsvp_object_tree, tvb, offset+2, 1,
			"Class number: %u - %s",
			class, type_str);
    proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			"C-type: %u", type);
    mylen = obj_length - 4;

    proto_tree_add_text(rsvp_object_tree, tvb, offset2, 1,
			"Message format version: %u",
			tvb_get_guint8(tvb, offset2)>>4);
    proto_tree_add_text(rsvp_object_tree, tvb, offset2+2, 2,
			"Data length: %u words, not including header",
			tvb_get_ntohs(tvb, offset2+2));
    mylen -= 4;
    offset2 += 4;
    while (mylen > 0) {
	guint8 service_num;
	guint8 break_bit;
	guint16 length;
	char *str;

	service_num = tvb_get_guint8(tvb, offset2);
	str = val_to_str(service_num, intsrv_services_str, "Unknown");
	break_bit = tvb_get_guint8(tvb, offset2+1);
	length = tvb_get_ntohs(tvb, offset2+2);
	ti = proto_tree_add_text(rsvp_object_tree, tvb, offset2,
				 (length+1)*4,
				 str);
	adspec_tree = proto_item_add_subtree(ti,
					     TREE(TT_ADSPEC_SUBTREE));
	proto_tree_add_text(adspec_tree, tvb, offset2, 1,
			    "Service header %u - %s",
			    service_num, str);
	proto_tree_add_text(adspec_tree, tvb, offset2+1, 1,
			    (break_bit&0x80)?
			    "Break bit set":"Break bit not set");
	proto_tree_add_text(adspec_tree, tvb, offset2+2, 2,
			    "Data length: %u words, not including header",
			    length);
	mylen -= 4;
	offset2 += 4;
	i = length*4;
	while (i > 0) {
	    guint8 id;
	    guint16 phdr_length;

	    id = tvb_get_guint8(tvb, offset2);
	    phdr_length = tvb_get_ntohs(tvb, offset2+2);
	    str = match_strval(id, adspec_params);
	    if (str) {
		switch(id) {
		case 4:
		case 8:
		case 10:
		case 133:
		case 134:
		case 135:
		case 136:
		    /* 32-bit unsigned integer */
		    proto_tree_add_text(adspec_tree, tvb, offset2,
					(phdr_length+1)<<2,
					"%s - %u (type %u, length %u)",
					str,
					tvb_get_ntohl(tvb, offset2+4),
					id, phdr_length);
		    break;

		case 6:
		    /* IEEE float */
		    proto_tree_add_text(adspec_tree, tvb, offset2,
					(phdr_length+1)<<2,
					"%s - %.10g (type %u, length %u)",
					str,
					tvb_get_ntohieee_float(tvb, offset2+4),
					id, phdr_length);
		    break;
		default:
		    proto_tree_add_text(adspec_tree, tvb, offset2,
					(phdr_length+1)<<2,
					"%s (type %u, length %u)",
					str,
					id, phdr_length);
		}
	    } else {
		proto_tree_add_text(adspec_tree, tvb, offset2,
				    (phdr_length+1)<<2,
				    "Unknown (type %u, length %u)",
				    id, phdr_length);
	    }
	    offset2 += (phdr_length+1)<<2;
	    i -= (phdr_length+1)<<2;
	    mylen -= (phdr_length+1)<<2;
	}
    }
}

/*------------------------------------------------------------------------------
 * INTEGRITY
 *------------------------------------------------------------------------------*/
static void
dissect_rsvp_integrity (proto_tree *ti, tvbuff_t *tvb,
			int offset, int obj_length,
			int class, int type,
			char *type_str)
{
    int offset2 = offset + 4;
    proto_tree *rsvp_object_tree;
    proto_tree *ti2, *rsvp_integ_flags_tree;
    int flags;
    guint64 sequence_number;

    rsvp_object_tree = proto_item_add_subtree(ti, TREE(TT_INTEGRITY));
    proto_tree_add_text(rsvp_object_tree, tvb, offset, 2,
			"Length: %u", obj_length);
    proto_tree_add_text(rsvp_object_tree, tvb, offset+2, 1,
			"Class number: %u - %s",
			class, type_str);
    proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			"C-type: %u", type);
    flags = tvb_get_guint8(tvb, offset2);
    ti2 = proto_tree_add_text(rsvp_object_tree, tvb, offset2, 1,
			      "Flags: 0x%02x", flags);
    rsvp_integ_flags_tree = proto_item_add_subtree(ti2, TREE(TT_INTEGRITY_FLAGS));
    proto_tree_add_text(rsvp_integ_flags_tree, tvb, offset2, 1,
	decode_boolean_bitfield(flags, 0x01, 8, "Handshake capable", "Handshake not capable"));
    proto_tree_add_text(rsvp_object_tree, tvb, offset2+2, 6,
			"Key Identifier: %s", tvb_bytes_to_str(tvb, offset2+2, 6));
    sequence_number = tvb_get_ntohl(tvb, offset2+8);
    sequence_number <<= 32;
    sequence_number = tvb_get_ntohl(tvb, offset2+12);
    proto_tree_add_text(rsvp_object_tree, tvb, offset2+8, 8,
			"Sequence Number: %" PRIu64, sequence_number);
    proto_tree_add_text(rsvp_object_tree, tvb, offset2+16, obj_length - 20,
			"Hash: %s", tvb_bytes_to_str(tvb, offset2+16, obj_length - 20));
}

/*------------------------------------------------------------------------------
 * POLICY
 *------------------------------------------------------------------------------*/
static void
dissect_rsvp_policy (proto_tree *ti, tvbuff_t *tvb,
		     int offset, int obj_length,
		     int class, int type,
		     char *type_str)
{
    int offset2 = offset + 4;
    proto_tree *rsvp_object_tree;

    rsvp_object_tree = proto_item_add_subtree(ti, TREE(TT_POLICY));
    proto_tree_add_text(rsvp_object_tree, tvb, offset, 2,
			"Length: %u", obj_length);
    proto_tree_add_text(rsvp_object_tree, tvb, offset+2, 1,
			"Class number: %u - %s",
			class, type_str);
    proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			"C-type: %u", type);
    proto_tree_add_text(rsvp_object_tree, tvb, offset2, obj_length - 4,
			"Data (%d bytes)", obj_length - 4);
}

/*------------------------------------------------------------------------------
 * LABEL_REQUEST
 *------------------------------------------------------------------------------*/
static void
dissect_rsvp_label_request (proto_tree *ti, tvbuff_t *tvb,
			    int offset, int obj_length,
			    int class, int type,
			    char *type_str)
{
    int offset2 = offset + 4;
    proto_tree *rsvp_object_tree;

    rsvp_object_tree = proto_item_add_subtree(ti, TREE(TT_LABEL_REQUEST));
    proto_tree_add_text(rsvp_object_tree, tvb, offset, 2,
			"Length: %u", obj_length);
    proto_tree_add_text(rsvp_object_tree, tvb, offset+2, 1,
			"Class number: %u - %s",
			class, type_str);
    switch(type) {
    case 1: {
	unsigned short l3pid = tvb_get_ntohs(tvb, offset2+2);
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: 1");
	proto_tree_add_text(rsvp_object_tree, tvb, offset2+2, 2,
			    "L3PID: %s (0x%04x)",
			    val_to_str(l3pid, etype_vals, "Unknown"),
			    l3pid);
	proto_item_set_text(ti, "LABEL REQUEST: Basic: L3PID: %s (0x%04x)",
			    val_to_str(l3pid, etype_vals, "Unknown"),
			    l3pid);
	break;
    }

    case 2: {
	unsigned short l3pid = tvb_get_ntohs(tvb, offset2+2);
	unsigned short min_vpi, min_vci, max_vpi, max_vci;
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: 2 (Label Request with ATM label Range)");
	proto_tree_add_text(rsvp_object_tree, tvb, offset2+2, 2,
			    "L3PID: %s (0x%04x)",
			    val_to_str(l3pid, etype_vals, "Unknown"),
			    l3pid);
	proto_tree_add_text(rsvp_object_tree, tvb, offset2+4, 1, 
			    "M: %s Merge in Data Plane",
			    (tvb_get_guint8(tvb, offset2+4) & 0x80) ? 
			    "1: Can" : "0: Cannot");
	min_vpi = tvb_get_ntohs(tvb, offset2+4) & 0x7f;
	min_vci = tvb_get_ntohs(tvb, offset2+6);
	max_vpi = tvb_get_ntohs(tvb, offset2+8) & 0x7f;
	max_vci = tvb_get_ntohs(tvb, offset2+10);
	proto_tree_add_text(rsvp_object_tree, tvb, offset2+4, 2,
			    "Min VPI: %d", min_vpi);
	proto_tree_add_text(rsvp_object_tree, tvb, offset2+6, 2,
			    "Min VCI: %d", min_vci);
	proto_tree_add_text(rsvp_object_tree, tvb, offset2+8, 2,
			    "Max VPI: %d", max_vpi);
	proto_tree_add_text(rsvp_object_tree, tvb, offset2+10, 2,
			    "Max VCI: %d", max_vci);
	proto_item_set_text(ti, "LABEL REQUEST: ATM: L3PID: %s (0x%04x). VPI/VCI: Min: %d/%d, Max: %d/%d. %s Merge. ",
			    val_to_str(l3pid, etype_vals, "Unknown"), l3pid,
			    min_vpi, min_vci, max_vpi, max_vci, 
			    (tvb_get_guint8(tvb, offset2+4) & 0x80) ? "Can" : "Cannot");
	break;
    }

    case 3: {
	guint16 l3pid = tvb_get_ntohs(tvb, offset2+2);
	guint32 min_dlci, max_dlci, dlci_len, dlci_len_code;
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: 2 (Label Request with ATM label Range)");
	proto_tree_add_text(rsvp_object_tree, tvb, offset2+2, 2,
			    "L3PID: %s (0x%04x)",
			    val_to_str(l3pid, etype_vals, "Unknown"),
			    l3pid);
	dlci_len_code = (tvb_get_ntohs(tvb, offset2+4) & 0x0180) >> 7; 
	min_dlci = tvb_get_ntohl(tvb, offset2+4) & 0x7fffff;
	max_dlci = tvb_get_ntohl(tvb, offset2+8) & 0x7fffff;
	switch(dlci_len_code) {
	case 0: 
	    /* 10-bit DLCIs */
	    dlci_len = 10;
	    min_dlci &= 0x3ff;
	    max_dlci &= 0x3ff;
	case 2:
	    dlci_len = 23;
	default:
	    dlci_len = 0;
	    min_dlci = 0;
	    max_dlci = 0;
	}
	proto_tree_add_text(rsvp_object_tree, tvb, offset2+4, 2, 
			    "DLCI Length: %s (%d)", 
			    dlci_len==10 ? "10 bits" : 
			    dlci_len==23 ? "23 bits" : 
			    "INVALID", dlci_len_code);
	proto_tree_add_text(rsvp_object_tree, tvb, offset2+5, 3,
			    "Min DLCI: %d", min_dlci);
	proto_tree_add_text(rsvp_object_tree, tvb, offset2+8, 2,
			    "Max DLCI: %d", max_dlci);
	proto_item_set_text(ti, "LABEL REQUEST: Frame: L3PID: %s (0x%04x). DLCI Len: %s. Min DLCI: %d. Max DLCI: %d",
			    val_to_str(l3pid, etype_vals, "Unknown"), l3pid,
			    dlci_len==10 ? "10 bits" : 
			    dlci_len==23 ? "23 bits" : 
			    "INVALID", min_dlci, max_dlci);
	break;
    }
    case 4: {
	unsigned short l3pid = tvb_get_ntohs(tvb, offset2+2);
	unsigned char  lsp_enc = tvb_get_guint8(tvb,offset2);
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: 4 (Generalized Label Request)");
	proto_tree_add_text(rsvp_object_tree, tvb, offset2, 1,
			    "LSP Encoding Type: %s",
			    val_to_str(lsp_enc, gmpls_lsp_enc_str, "Unknown (%d)"));
	proto_tree_add_text(rsvp_object_tree, tvb, offset2+1, 1,
			    "Switching Type: %s",
			    val_to_str(tvb_get_guint8(tvb,offset2+1),
				       gmpls_switching_type_str, "Unknown (%d)"));
	proto_tree_add_text(rsvp_object_tree, tvb, offset2+2, 2,
			    "G-PID: %s (0x%0x)",
			    val_to_str(l3pid, gmpls_gpid_str,
				       val_to_str(l3pid, etype_vals,
						  "Unknown G-PID(0x%04x)")),
			    l3pid);
	proto_item_set_text(ti, "LABEL REQUEST: Generalized: LSP Encoding=%s, "
			    "Switching Type=%s, G-PID=%s ",
			    val_to_str(lsp_enc, gmpls_lsp_enc_str, "Unknown (%d)"),
			    val_to_str(tvb_get_guint8(tvb,offset2+1),
				       gmpls_switching_type_str, "Unknown (%d)"),
			    val_to_str(l3pid, gmpls_gpid_str,
				       val_to_str(l3pid, etype_vals,
						  "Unknown (0x%04x)")));
	break;
    }

    default: {
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: Unknown (%u)",
			    type);
	proto_tree_add_text(rsvp_object_tree, tvb, offset2, obj_length - 4,
			    "Data (%d bytes)", obj_length - 4);
	break;
    }
    }
}

/*------------------------------------------------------------------------------
 * LABEL
 *------------------------------------------------------------------------------*/
static void
dissect_rsvp_label (proto_tree *ti, tvbuff_t *tvb,
		    int offset, int obj_length,
		    int class, int type,
		    char *type_str)
{
    int offset2 = offset + 4;
    proto_tree *rsvp_object_tree;
    int mylen, i;
    char *name;

    name = (class==RSVP_CLASS_SUGGESTED_LABEL ? "SUGGESTED LABEL":
	    (class==RSVP_CLASS_UPSTREAM_LABEL ? "UPSTREAM LABEL":
	     "LABEL"));
    rsvp_object_tree = proto_item_add_subtree(ti, TREE(TT_LABEL));
    proto_tree_add_text(rsvp_object_tree, tvb, offset, 2,
			"Length: %u", obj_length);
    proto_tree_add_text(rsvp_object_tree, tvb, offset+2, 1,
			"Class number: %u - %s",
			class, type_str);
    mylen = obj_length - 4;
    switch(type) {
    case 1:
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: 1 (Packet Label)");
	proto_tree_add_text(rsvp_object_tree, tvb, offset2, 4,
			    "Label: %u",
			    tvb_get_ntohl(tvb, offset2));
	proto_item_set_text(ti, "%s: %d", name,
			    tvb_get_ntohl(tvb, offset2));
	break;

    case 2:
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: 2 (Generalized Label)");
	proto_item_set_text(ti, "%s: Generalized: ", name);
	for (i = 0; i < mylen; i += 4) {
	    proto_tree_add_text(rsvp_object_tree, tvb, offset2+i, 4,
				"Generalized Label: %u",
				tvb_get_ntohl(tvb, offset2+i));
	    if (i < 16) {
		proto_item_append_text(ti, "%d%s",
				       tvb_get_ntohl(tvb, offset2+i),
				       i+4<mylen?", ":"");
	    } else if (i == 16) {
		proto_item_append_text(ti, "...");
	    }
	}
	break;

    default:
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: Unknown (%u)",
			    type);
	proto_tree_add_text(rsvp_object_tree, tvb, offset2, mylen,
			    "Data (%d bytes)", mylen);
	break;
    }
}

/*------------------------------------------------------------------------------
 * SESSION ATTRIBUTE
 *------------------------------------------------------------------------------*/
static void
dissect_rsvp_session_attribute (proto_tree *ti, tvbuff_t *tvb,
				int offset, int obj_length,
				int class, int type,
				char *type_str)
{
    int offset2 = offset + 4;
    proto_tree *rsvp_object_tree;
    guint8 flags;
    guint8 name_len;
    proto_tree *ti2, *rsvp_sa_flags_tree;

    rsvp_object_tree = proto_item_add_subtree(ti, TREE(TT_SESSION_ATTRIBUTE));
    proto_tree_add_text(rsvp_object_tree, tvb, offset, 2,
			"Length: %u", obj_length);
    proto_tree_add_text(rsvp_object_tree, tvb, offset+2, 1,
			"Class number: %u - %s",
			class, type_str);
    switch(type) {
    case 1:
    case 7:

	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: %u - IPv4 LSP (%sResource Affinities)",
                            type, (type == 1) ? "" : "No ");
        
        if (type == 1) {
            proto_tree_add_text(rsvp_object_tree, tvb, offset2, 4, 
			    "Exclude-Any: 0x%0x", tvb_get_ntohl(tvb, offset2));
            proto_tree_add_text(rsvp_object_tree, tvb, offset2+4, 4, 
			    "Include-Any: 0x%0x", tvb_get_ntohl(tvb, offset2+4));
            proto_tree_add_text(rsvp_object_tree, tvb, offset2+8, 4, 
			    "Include-All: 0x%0x", tvb_get_ntohl(tvb, offset2+8));
            offset2 = offset2+12;
        }

	proto_tree_add_text(rsvp_object_tree, tvb, offset2, 1,
			    "Setup priority: %u",
			    tvb_get_guint8(tvb, offset2));
	proto_tree_add_text(rsvp_object_tree, tvb, offset2+1, 1,
			    "Hold priority: %u",
			    tvb_get_guint8(tvb, offset2+1));
	flags = tvb_get_guint8(tvb, offset2+2);
	ti2 = proto_tree_add_text(rsvp_object_tree, tvb, offset2+2, 1,
				  "Flags: 0x%02x", flags);
	rsvp_sa_flags_tree = proto_item_add_subtree(ti2,
						    TREE(TT_SESSION_ATTRIBUTE_FLAGS));
	proto_tree_add_text(rsvp_sa_flags_tree, tvb, offset2+2, 1,
			    decode_boolean_bitfield(flags, 0x01, 8,
						    "Local protection desired",
						    "Local protection not desired"));
	proto_tree_add_text(rsvp_sa_flags_tree, tvb, offset2+2, 1,
			    decode_boolean_bitfield(flags, 0x02, 8,
						    "Label recording desired",
						    "Label recording not desired"));
	proto_tree_add_text(rsvp_sa_flags_tree, tvb, offset2+2, 1,
			    decode_boolean_bitfield(flags, 0x04, 8,
						    "SE style desired",
						    "SE style not desired"));
	proto_tree_add_text(rsvp_sa_flags_tree, tvb, offset2+2, 1,
			    decode_boolean_bitfield(flags, 0x08, 8,
						    "Bandwidth protection desired",
						    "Bandwidth protection not desired"));
	proto_tree_add_text(rsvp_sa_flags_tree, tvb, offset2+2, 1,
			    decode_boolean_bitfield(flags, 0x10, 8,
						    "Node protection desired",
						    "Node protection not desired"));

	name_len = tvb_get_guint8(tvb, offset2+3);
	proto_tree_add_text(rsvp_object_tree, tvb, offset2+3, 1,
			    "Name length: %u", name_len);
	proto_tree_add_text(rsvp_object_tree, tvb, offset2+4, name_len,
			    "Name: %s",
			    tvb_format_text(tvb, offset2+4, name_len));

	proto_item_set_text(ti, "SESSION ATTRIBUTE: SetupPrio %d, HoldPrio %d, %s%s%s%s%s [%s]",
			    tvb_get_guint8(tvb, offset2),
			    tvb_get_guint8(tvb, offset2+1),
			    flags &0x01 ? "Local Protection, " : "",
			    flags &0x02 ? "Label Recording, " : "",
			    flags &0x04 ? "SE Style, " : "",
			    flags &0x08 ? "Bandwidth Protection, " : "",
			    flags &0x10 ? "Node Protection, " : "",
			    name_len ? tvb_format_text(tvb, offset2+4, name_len) : "");
	break;

    default:
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: Unknown (%u)",
			    type);
	proto_tree_add_text(rsvp_object_tree, tvb, offset2, obj_length - 4,
			    "Data (%d bytes)", obj_length - 4);
	break;
    }
}

/*------------------------------------------------------------------------------
 * EXPLICIT ROUTE AND RECORD ROUTE SUBOBJECTS
 *------------------------------------------------------------------------------*/
static void
dissect_rsvp_ero_rro_subobjects (proto_tree *ti, proto_tree *rsvp_object_tree, 
				 tvbuff_t *tvb,
				 int offset, int obj_length, int class)
{
    int i, j, k, l, flags;
    proto_tree *ti2, *rsvp_ro_subtree, *rsvp_rro_flags_subtree;
    int tree_type;

    switch(class) {
    case RSVP_CLASS_EXPLICIT_ROUTE:
	tree_type = TREE(TT_EXPLICIT_ROUTE_SUBOBJ); 
	break;
    case RSVP_CLASS_RECORD_ROUTE:
	tree_type = TREE(TT_RECORD_ROUTE_SUBOBJ); 
	break;
    default:
	/* Bail out */
	return;
    }

    for (i=1, l = 0; l < obj_length - 4; i++) {
	j = tvb_get_guint8(tvb, offset+l) & 0x7f;
	switch(j) {
	case 1: /* IPv4 */
	    k = tvb_get_guint8(tvb, offset+l) & 0x80;
	    ti2 = proto_tree_add_text(rsvp_object_tree, tvb,
				      offset+l, 8,
				      "IPv4 Subobject - %s%s",
				      ip_to_str(tvb_get_ptr(tvb, offset+l+2, 4)),
				      class == RSVP_CLASS_EXPLICIT_ROUTE ? 
				      (k ? ", Loose" : ", Strict") : "");
	    rsvp_ro_subtree =
		proto_item_add_subtree(ti2, tree_type);
	    if (class == RSVP_CLASS_EXPLICIT_ROUTE) 
		proto_tree_add_text(rsvp_ro_subtree, tvb, offset+l, 1,
				    k ? "Loose Hop " : "Strict Hop");
	    proto_tree_add_text(rsvp_ro_subtree, tvb, offset+l, 1,
				"Type: 1 (IPv4)");
	    proto_tree_add_text(rsvp_ro_subtree, tvb, offset+l+1, 1,
				"Length: %u",
				tvb_get_guint8(tvb, offset+l+1));
	    proto_tree_add_text(rsvp_ro_subtree, tvb, offset+l+2, 4,
				"IPv4 hop: %s",
				ip_to_str(tvb_get_ptr(tvb, offset+l+2, 4)));
	    proto_tree_add_text(rsvp_ro_subtree, tvb, offset+l+6, 1,
				"Prefix length: %u",
				tvb_get_guint8(tvb, offset+l+6));
	    if (i < 4) {
		proto_item_append_text(ti, "IPv4 %s%s",
				       ip_to_str(tvb_get_ptr(tvb, offset+l+2, 4)),
				       k ? " [L]" : "");
	    }
	    if (class == RSVP_CLASS_RECORD_ROUTE) {
		flags = tvb_get_guint8(tvb, offset+l+7);
		if (flags&0x10) {
		    proto_item_append_text(ti,  " (Node-id)");
		    proto_item_append_text(ti2, " (Node-id)");
		}
		if (flags&0x01) proto_item_append_text(ti2, ", Local Protection Available");
		if (flags&0x02) proto_item_append_text(ti2, ", Local Protection In Use");
		if (flags&0x04) proto_item_append_text(ti2, ", Backup BW Avail");
		if (flags&0x08) proto_item_append_text(ti2, ", Backup is Next-Next-Hop");
		ti2 = proto_tree_add_text(rsvp_ro_subtree, tvb, offset+l+7, 1,
					  "Flags: 0x%02x", flags);
		rsvp_rro_flags_subtree = 
		    proto_item_add_subtree(ti2, TREE(TT_RECORD_ROUTE_SUBOBJ_FLAGS)); 
		proto_tree_add_text(rsvp_rro_flags_subtree, tvb, offset+l+7, 1,
				    decode_boolean_bitfield(flags, 0x01, 8, 
							    "Local Protection Available",
							    "Local Protection Not Available"));
		proto_tree_add_text(rsvp_rro_flags_subtree, tvb, offset+l+7, 1,
				    decode_boolean_bitfield(flags, 0x02, 8, 
							    "Local Protection In Use",
							    "Local Protection Not In Use"));
		proto_tree_add_text(rsvp_rro_flags_subtree, tvb, offset+l+7, 1,
				    decode_boolean_bitfield(flags, 0x04, 8, 
							    "Bandwidth Protection Available",
							    "Bandwidth Protection Not Available"));
		proto_tree_add_text(rsvp_rro_flags_subtree, tvb, offset+l+7, 1,
				    decode_boolean_bitfield(flags, 0x08, 8, 
							    "Node Protection Available",
							    "Node Protection Not Available"));
		proto_tree_add_text(rsvp_rro_flags_subtree, tvb, offset+l+7, 1,
				    decode_boolean_bitfield(flags, 0x10, 8,
							    "Address Specifies a Node-id Address",
							    "Address Doesn't Specify a Node-id Address"));
	    }

	    break;

	case 2: /* IPv6 */
	    ti2 = proto_tree_add_text(rsvp_object_tree, tvb,
				      offset+l, 20,
				      "IPv6 Subobject");
	    rsvp_ro_subtree =
		proto_item_add_subtree(ti2, tree_type);
	    k = tvb_get_guint8(tvb, offset+l) & 0x80;
	    if (class == RSVP_CLASS_EXPLICIT_ROUTE) 
		proto_tree_add_text(rsvp_ro_subtree, tvb, offset+l, 1,
				    k ? "Loose Hop " : "Strict Hop");
	    proto_tree_add_text(rsvp_ro_subtree, tvb, offset+l, 1,
				"Type: 2 (IPv6)");
	    proto_tree_add_text(rsvp_ro_subtree, tvb, offset+l+1, 1,
				"Length: %u",
				tvb_get_guint8(tvb, offset+l+1));
	    proto_tree_add_text(rsvp_ro_subtree, tvb, offset+l+2, 16,
				"IPv6 hop: %s",
				ip6_to_str((const struct e_in6_addr *)tvb_get_ptr(tvb, offset+l+2, 16)));
	    proto_tree_add_text(rsvp_ro_subtree, tvb, offset+l+18, 1,
				"Prefix length: %u",
				tvb_get_guint8(tvb, offset+l+18));
	    if (i < 4) {
		proto_item_append_text(ti, "IPv6 [...]%s", k ? " [L]":"");
	    }
	    if (class == RSVP_CLASS_RECORD_ROUTE) {
		flags = tvb_get_guint8(tvb, offset+l+19);
		if (flags&0x10) {
		    proto_item_append_text(ti,  " (Node-id)");
		    proto_item_append_text(ti2, " (Node-id)");
		}
		if (flags&0x01) proto_item_append_text(ti2, ", Local Protection Available");
		if (flags&0x02) proto_item_append_text(ti2, ", Local Protection In Use");
		if (flags&0x04) proto_item_append_text(ti2, ", Backup BW Avail");
		if (flags&0x08) proto_item_append_text(ti2, ", Backup is Next-Next-Hop");
		ti2 = proto_tree_add_text(rsvp_ro_subtree, tvb, offset+l+19, 1,
					  "Flags: 0x%02x", flags);
		rsvp_rro_flags_subtree = 
		    proto_item_add_subtree(ti2, TREE(TT_RECORD_ROUTE_SUBOBJ_FLAGS)); 
		proto_tree_add_text(rsvp_rro_flags_subtree, tvb, offset+l+19, 1,
				    decode_boolean_bitfield(flags, 0x01, 8, 
							    "Local Protection Available",
							    "Local Protection Not Available"));
		proto_tree_add_text(rsvp_rro_flags_subtree, tvb, offset+l+19, 1,
				    decode_boolean_bitfield(flags, 0x02, 8, 
							    "Local Protection In Use",
							    "Local Protection Not In Use"));
		proto_tree_add_text(rsvp_rro_flags_subtree, tvb, offset+l+19, 1,
				    decode_boolean_bitfield(flags, 0x04, 8, 
							    "Backup Tunnel Has Bandwidth",
							    "Backup Tunnel Does Not Have Bandwidth"));
		proto_tree_add_text(rsvp_rro_flags_subtree, tvb, offset+l+19, 1,
				    decode_boolean_bitfield(flags, 0x08, 8, 
							    "Backup Tunnel Goes To Next-Next-Hop",
							    "Backup Tunnel Goes To Next-Hop"));
		proto_tree_add_text(rsvp_rro_flags_subtree, tvb, offset+l+19, 1,
				    decode_boolean_bitfield(flags, 0x10, 8,
							    "Address Specifies a Node-id Address",
							    "Address Doesn't Specify a Node-id Address"));
	    }

	    break;

	case 3: /* Label */
	    k = tvb_get_guint8(tvb, offset+l) & 0x80;
	    ti2 = proto_tree_add_text(rsvp_object_tree, tvb,
				      offset+l, 8,
				      "Label Subobject - %d, %s",
				      tvb_get_ntohl(tvb, offset+l+4),
				      class == RSVP_CLASS_EXPLICIT_ROUTE ? 
				      (k ? "Loose" : "Strict") : "");
	    rsvp_ro_subtree =
		proto_item_add_subtree(ti2, tree_type);
	    if (class == RSVP_CLASS_EXPLICIT_ROUTE) 
		proto_tree_add_text(rsvp_ro_subtree, tvb, offset+l, 1,
				    k ? "Loose Hop " : "Strict Hop");
	    proto_tree_add_text(rsvp_ro_subtree, tvb, offset+l, 1,
				"Type: 3 (Label)");
	    proto_tree_add_text(rsvp_ro_subtree, tvb, offset+l+1, 1,
				"Length: %u",
				tvb_get_guint8(tvb, offset+l+1));
	    if (class == RSVP_CLASS_RECORD_ROUTE) {
		flags = tvb_get_guint8(tvb, offset+l+2);
		if (flags&0x01) proto_item_append_text(ti2, ", Local Protection Available");
		if (flags&0x02) proto_item_append_text(ti2, ", Local Protection In Use");
		if (flags&0x04) proto_item_append_text(ti2, ", Backup BW Avail");
		if (flags&0x08) proto_item_append_text(ti2, ", Backup is Next-Next-Hop");
		ti2 = proto_tree_add_text(rsvp_ro_subtree, tvb, offset+l+2, 1,
					  "Flags: 0x%02x", flags);
		rsvp_rro_flags_subtree = 
		    proto_item_add_subtree(ti2, TREE(TT_RECORD_ROUTE_SUBOBJ_FLAGS)); 
		proto_tree_add_text(rsvp_rro_flags_subtree, tvb, offset+l+2, 1,
				    decode_boolean_bitfield(flags, 0x01, 8, 
							    "Local Protection Available",
							    "Local Protection Not Available"));
		proto_tree_add_text(rsvp_rro_flags_subtree, tvb, offset+l+2, 1,
				    decode_boolean_bitfield(flags, 0x02, 8, 
							    "Local Protection In Use",
							    "Local Protection Not In Use"));
		proto_tree_add_text(rsvp_rro_flags_subtree, tvb, offset+l+2, 1,
				    decode_boolean_bitfield(flags, 0x04, 8, 
							    "Backup Tunnel Has Bandwidth",
							    "Backup Tunnel Does Not Have Bandwidth"));
		proto_tree_add_text(rsvp_rro_flags_subtree, tvb, offset+l+2, 1,
				    decode_boolean_bitfield(flags, 0x08, 8, 
							    "Backup Tunnel Goes To Next-Next-Hop",
							    "Backup Tunnel Goes To Next-Hop"));
	    }
	    proto_tree_add_text(rsvp_ro_subtree, tvb, offset+l+3, 1,
				"C-Type: %u",
				tvb_get_guint8(tvb, offset+l+3));
	    proto_tree_add_text(rsvp_ro_subtree, tvb, offset+l+4, 4,
				"Label: %d",
				tvb_get_ntohl(tvb, offset+l+4));
	    if (i < 4) {
		proto_item_append_text(ti, "Label %d%s",
				       tvb_get_ntohl(tvb, offset+l+4),
				       k ? " [L]":"");
	    }
	    break;

	case 4: /* Unnumbered Interface-ID */
	    k = tvb_get_guint8(tvb, offset+l) & 0x80;
	    ti2 = proto_tree_add_text(rsvp_object_tree, tvb,
				      offset+l, 8,
				      "Unnumbered Interface-ID - %s, %d, %s",
				      ip_to_str(tvb_get_ptr(tvb, offset+l+4, 4)),
				      tvb_get_ntohl(tvb, offset+l+8),
				      class == RSVP_CLASS_EXPLICIT_ROUTE ? 
				      (k ? "Loose" : "Strict") : "");
	    rsvp_ro_subtree =
		proto_item_add_subtree(ti2, tree_type);
	    if (class == RSVP_CLASS_EXPLICIT_ROUTE) 
		proto_tree_add_text(rsvp_ro_subtree, tvb, offset+l, 1,
				    k ? "Loose Hop " : "Strict Hop");
	    proto_tree_add_text(rsvp_ro_subtree, tvb, offset+l, 1,
				"Type: 4 (Unnumbered Interface-ID)");
	    proto_tree_add_text(rsvp_ro_subtree, tvb, offset+l+1, 1,
				"Length: %u",
				tvb_get_guint8(tvb, offset+l+1));
	    if (class == RSVP_CLASS_RECORD_ROUTE) {
		flags = tvb_get_guint8(tvb, offset+l+2);
		if (flags&0x01) proto_item_append_text(ti2, ", Local Protection Available");
		if (flags&0x02) proto_item_append_text(ti2, ", Local Protection In Use");
		if (flags&0x04) proto_item_append_text(ti2, ", Backup BW Avail");
		if (flags&0x08) proto_item_append_text(ti2, ", Backup is Next-Next-Hop");
		ti2 = proto_tree_add_text(rsvp_ro_subtree, tvb, offset+l+2, 1,
					  "Flags: 0x%02x", flags);
		rsvp_rro_flags_subtree = 
		    proto_item_add_subtree(ti2, TREE(TT_RECORD_ROUTE_SUBOBJ_FLAGS)); 
		proto_tree_add_text(rsvp_rro_flags_subtree, tvb, offset+l+2, 1,
				    decode_boolean_bitfield(flags, 0x01, 8, 
							    "Local Protection Available",
							    "Local Protection Not Available"));
		proto_tree_add_text(rsvp_rro_flags_subtree, tvb, offset+l+2, 1,
				    decode_boolean_bitfield(flags, 0x02, 8, 
							    "Local Protection In Use",
							    "Local Protection Not In Use"));
		proto_tree_add_text(rsvp_rro_flags_subtree, tvb, offset+l+2, 1,
				    decode_boolean_bitfield(flags, 0x04, 8, 
							    "Backup Tunnel Has Bandwidth",
							    "Backup Tunnel Does Not Have Bandwidth"));
		proto_tree_add_text(rsvp_rro_flags_subtree, tvb, offset+l+2, 1,
				    decode_boolean_bitfield(flags, 0x08, 8, 
							    "Backup Tunnel Goes To Next-Next-Hop",
							    "Backup Tunnel Goes To Next-Hop"));
	    }
	    proto_tree_add_text(rsvp_ro_subtree, tvb, offset+l+4, 4,
				"Router-ID: %s",
				ip_to_str(tvb_get_ptr(tvb, offset+l+4, 4)));
	    proto_tree_add_text(rsvp_ro_subtree, tvb, offset+l+8, 4,
				"Interface-ID: %d",
				tvb_get_ntohl(tvb, offset+l+8));
	    if (i < 4) {
		proto_item_append_text(ti, "Unnum %s/%d%s",
				       ip_to_str(tvb_get_ptr(tvb, offset+l+4, 4)),
				       tvb_get_ntohl(tvb, offset+l+8),
				       k ? " [L]":"");
	    }

	    break;

	case 32: /* AS */
	    if (class == RSVP_CLASS_RECORD_ROUTE) goto defaultsub;
	    k = tvb_get_ntohs(tvb, offset+l+2);
	    ti2 = proto_tree_add_text(rsvp_object_tree, tvb,
				      offset+l, 4,
				      "Autonomous System %u",
				      k);
	    rsvp_ro_subtree =
		proto_item_add_subtree(ti2, tree_type);
	    proto_tree_add_text(rsvp_ro_subtree, tvb, offset+l, 1,
				"Type: 32 (Autonomous System Number)");
	    proto_tree_add_text(rsvp_ro_subtree, tvb, offset+l+1, 1,
				"Length: %u",
				tvb_get_guint8(tvb, offset+l+1));
	    proto_tree_add_text(rsvp_ro_subtree, tvb, offset+l+2, 2,
				"Autonomous System %u", k);
	    if (i < 4) {
		proto_item_append_text(ti, "AS %d",
				       tvb_get_ntohs(tvb, offset+l+2));
	    }

	    break;

	default: /* Unknown subobject */
	defaultsub:
	    k = tvb_get_guint8(tvb, offset+l) & 0x80;
	    ti2 = proto_tree_add_text(rsvp_object_tree, tvb,
				      offset+l,
				      tvb_get_guint8(tvb, offset+l+1),
				      "Unknown subobject: %d", j);
	    rsvp_ro_subtree =
		proto_item_add_subtree(ti2, tree_type);
	    proto_tree_add_text(rsvp_ro_subtree, tvb, offset+l, 1,
				k ? "Loose Hop " : "Strict Hop");
	    proto_tree_add_text(rsvp_ro_subtree, tvb, offset+l, 1,
				"Type: %u (Unknown)", j);
	    proto_tree_add_text(rsvp_ro_subtree, tvb, offset+l+1, 1,
				"Length: %u",
				tvb_get_guint8(tvb, offset+l+1));

	}

	l += tvb_get_guint8(tvb, offset+l+1);
	if (l < obj_length - 4) {
	    if (i < 4)
		proto_item_append_text(ti, ", ");
	    else if (i==4)
		proto_item_append_text(ti, "...");
	}
    }
}
    
/*------------------------------------------------------------------------------
 * EXPLICIT ROUTE OBJECT
 *------------------------------------------------------------------------------*/
static void
dissect_rsvp_explicit_route (proto_tree *ti, tvbuff_t *tvb,
			     int offset, int obj_length,
			     int class, int type,
			     char *type_str)
{
    /* int offset2 = offset + 4; */
    proto_tree *rsvp_object_tree;
    /* int mylen, i, j, k, l; */
    /* proto_tree *ti2, *rsvp_ero_subtree; */

    rsvp_object_tree = proto_item_add_subtree(ti, TREE(TT_EXPLICIT_ROUTE));
    proto_tree_add_text(rsvp_object_tree, tvb, offset, 2,
			"Length: %u", obj_length);
    proto_tree_add_text(rsvp_object_tree, tvb, offset+2, 1,
			"Class number: %u - %s",
			class, type_str);
    /* mylen = obj_length - 4; */
    switch(type) {
    case 1: 
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: 1");
	proto_item_set_text(ti, "EXPLICIT ROUTE: ");

	dissect_rsvp_ero_rro_subobjects(ti, rsvp_object_tree, tvb,
					offset + 4, obj_length, class);
	break;

    default:
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: Unknown (%u)",
			    type);
	proto_tree_add_text(rsvp_object_tree, tvb, offset+4, obj_length - 4,
			    "Data (%d bytes)", obj_length - 4);
	break;
    }
}

/*------------------------------------------------------------------------------
 * RECORD ROUTE OBJECT
 *------------------------------------------------------------------------------*/
static void
dissect_rsvp_record_route (proto_tree *ti, tvbuff_t *tvb,
			   int offset, int obj_length,
			   int class, int type,
			   char *type_str)
{
    /* int offset2 = offset + 4; */
    proto_tree *rsvp_object_tree;
    /* int mylen, i, j, l; */
    /* proto_tree *ti2, *rsvp_rro_subtree; */

    rsvp_object_tree = proto_item_add_subtree(ti, TREE(TT_RECORD_ROUTE));
    proto_tree_add_text(rsvp_object_tree, tvb, offset, 2,
			"Length: %u", obj_length);
    proto_tree_add_text(rsvp_object_tree, tvb, offset+2, 1,
			"Class number: %u - %s",
			class, type_str);
    proto_item_set_text(ti, "RECORD ROUTE: ");
    /* mylen = obj_length - 4; */
    switch(type) {
    case 1: 
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: 1");

	dissect_rsvp_ero_rro_subobjects(ti, rsvp_object_tree, tvb,
					offset + 4, obj_length, class);
	break;

    default: 
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: Unknown (%u)",
			    type);
	proto_tree_add_text(rsvp_object_tree, tvb, offset+4, obj_length - 4,
			    "Data (%d bytes)", obj_length - 4);
	break;
    }
}

/*------------------------------------------------------------------------------
 * MESSAGE ID
 *------------------------------------------------------------------------------*/
static void
dissect_rsvp_message_id (proto_tree *ti, tvbuff_t *tvb,
			 int offset, int obj_length,
			 int class, int type,
			 char *type_str)
{
    int offset2 = offset + 4;
    proto_tree *rsvp_object_tree;

    rsvp_object_tree = proto_item_add_subtree(ti, TREE(TT_MESSAGE_ID));
    proto_tree_add_text(rsvp_object_tree, tvb, offset, 2,
			"Length: %u", obj_length);
    proto_tree_add_text(rsvp_object_tree, tvb, offset+2, 1,
			"Class number: %u - %s",
			class, type_str);
    switch(type) {
    case 1:
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: 1");
	proto_tree_add_text(rsvp_object_tree, tvb, offset+4, 1,
			    "Flags: %d", tvb_get_guint8(tvb, offset+4));
	proto_tree_add_text(rsvp_object_tree, tvb, offset+5, 3,
			    "Epoch: %d", tvb_get_ntoh24(tvb, offset+5));
	proto_tree_add_text(rsvp_object_tree, tvb, offset+8, 4,
			    "Message-ID: %d", tvb_get_ntohl(tvb, offset+8));
	proto_item_set_text(ti, "MESSAGE-ID: %d %s",
			    tvb_get_ntohl(tvb, offset+8),
			    tvb_get_guint8(tvb, offset+4) & 1 ? "(Ack Desired)" : "");
	break;

    default:
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: Unknown (%u)",
			    type);
	proto_tree_add_text(rsvp_object_tree, tvb, offset2, obj_length - 4,
			    "Data (%d bytes)", obj_length - 4);
	break;
    }
}

/*------------------------------------------------------------------------------
 * MESSAGE ID ACK
 *------------------------------------------------------------------------------*/
static void
dissect_rsvp_message_id_ack (proto_tree *ti, tvbuff_t *tvb,
			     int offset, int obj_length,
			     int class, int type,
			     char *type_str)
{
    int offset2 = offset + 4;
    proto_tree *rsvp_object_tree;

    rsvp_object_tree = proto_item_add_subtree(ti, TREE(TT_MESSAGE_ID_ACK));
    proto_tree_add_text(rsvp_object_tree, tvb, offset, 2,
			"Length: %u", obj_length);
    proto_tree_add_text(rsvp_object_tree, tvb, offset+2, 1,
			"Class number: %u - %s",
			class, type_str);
    switch(type) {
    case 1:
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: 1");
	proto_tree_add_text(rsvp_object_tree, tvb, offset+4, 1,
			    "Flags: %d", tvb_get_guint8(tvb, offset+4));
	proto_tree_add_text(rsvp_object_tree, tvb, offset+5, 3,
			    "Epoch: %d", tvb_get_ntoh24(tvb, offset+5));
	proto_tree_add_text(rsvp_object_tree, tvb, offset+8, 4,
			    "Message-ID: %d", tvb_get_ntohl(tvb, offset+8));
	proto_item_set_text(ti, "MESSAGE-ID ACK: %d", tvb_get_ntohl(tvb, offset+8));
	break;

    case 2:
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: 2");
	proto_tree_add_text(rsvp_object_tree, tvb, offset+4, 1,
			    "Flags: %d", tvb_get_guint8(tvb, offset+4));
	proto_tree_add_text(rsvp_object_tree, tvb, offset+5, 3,
			    "Epoch: %d", tvb_get_ntoh24(tvb, offset+5));
	proto_tree_add_text(rsvp_object_tree, tvb, offset+8, 4,
			    "Message-ID: %d", tvb_get_ntohl(tvb, offset+8));
	proto_item_set_text(ti, "MESSAGE-ID NACK: %d", tvb_get_ntohl(tvb, offset+8));
	break;

    default:
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: Unknown (%u)",
			    type);
	proto_tree_add_text(rsvp_object_tree, tvb, offset2, obj_length - 4,
			    "Data (%d bytes)", obj_length - 4);
	break;
    }
}

/*------------------------------------------------------------------------------
 * MESSAGE ID LIST
 *------------------------------------------------------------------------------*/
static void
dissect_rsvp_message_id_list (proto_tree *ti, tvbuff_t *tvb,
			      int offset, int obj_length,
			      int class, int type,
			      char *type_str)
{
    int offset2 = offset + 4;
    proto_tree *rsvp_object_tree;
    int mylen;

    rsvp_object_tree = proto_item_add_subtree(ti, TREE(TT_MESSAGE_ID_LIST));
    proto_tree_add_text(rsvp_object_tree, tvb, offset, 2,
			"Length: %u", obj_length);
    proto_tree_add_text(rsvp_object_tree, tvb, offset+2, 1,
			"Class number: %u - %s",
			class, type_str);
    switch(type) {
    case 1:
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: 1");
	proto_tree_add_text(rsvp_object_tree, tvb, offset+4, 1,
			    "Flags: %d", tvb_get_guint8(tvb, offset+4));
	proto_tree_add_text(rsvp_object_tree, tvb, offset+5, 3,
			    "Epoch: %d", tvb_get_ntoh24(tvb, offset+5));
	for (mylen = 8; mylen < obj_length; mylen += 4)
	    proto_tree_add_text(rsvp_object_tree, tvb, offset+mylen, 4,
				"Message-ID: %d", tvb_get_ntohl(tvb, offset+mylen));
	proto_item_set_text(ti, "MESSAGE-ID LIST: %d IDs",
			    (obj_length - 8)/4);
	break;

    default:
	mylen = obj_length - 4;
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: Unknown (%u)",
			    type);
	proto_tree_add_text(rsvp_object_tree, tvb, offset2, obj_length - 4,
			    "Data (%d bytes)", obj_length - 4);
	break;
    }
}

/*------------------------------------------------------------------------------
 * HELLO
 *------------------------------------------------------------------------------*/
static void
dissect_rsvp_hello (proto_tree *ti, tvbuff_t *tvb,
		    int offset, int obj_length,
		    int class, int type,
		    char *type_str)
{
    proto_tree *rsvp_object_tree;

    rsvp_object_tree = proto_item_add_subtree(ti, TREE(TT_HELLO_OBJ));
    proto_tree_add_text(rsvp_object_tree, tvb, offset, 2,
			"Length: %u", obj_length);
    proto_tree_add_text(rsvp_object_tree, tvb, offset+2, 1,
			"Class number: %u - %s",
			class, type_str);
    switch(type) {
    case 1:
    case 2:
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-Type: 1 - HELLO %s object",
			    type==1 ? "REQUEST" : "ACK");
	proto_tree_add_text(rsvp_object_tree, tvb, offset+4, 4,
			    "Source Instance: 0x%x",tvb_get_ntohl(tvb, offset+4));
	proto_tree_add_text(rsvp_object_tree, tvb, offset+8, 4,
			    "Destination Instance: 0x%x",tvb_get_ntohl(tvb, offset+8));
	proto_item_append_text(ti, ": %s. Src Instance: 0x%0x. Dest Instance: 0x%0x. ", 
			       type==1 ? "REQUEST" : "ACK", 
			       tvb_get_ntohl(tvb, offset+4),
			       tvb_get_ntohl(tvb, offset+8));
	break;
    default:
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-Type: %d - UNKNOWN", type);
	break;
    };
}

/*------------------------------------------------------------------------------
 * DCLASS
 *------------------------------------------------------------------------------*/
static void
dissect_rsvp_dclass (proto_tree *ti, tvbuff_t *tvb,
		     int offset, int obj_length,
		     int class, int type,
		     char *type_str)
{
    int offset2 = offset + 4;
    proto_tree *rsvp_object_tree;
    int mylen;

    rsvp_object_tree = proto_item_add_subtree(ti, TREE(TT_DCLASS));
    proto_tree_add_text(rsvp_object_tree, tvb, offset, 2,
			"Length: %u", obj_length);
    proto_tree_add_text(rsvp_object_tree, tvb, offset+2, 1,
			"Class number: %u - %s",
			class, type_str);
    proto_item_set_text(ti, "DCLASS: ");
    switch(type) {
    case 1:
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: 1");
	for (mylen = 4; mylen < obj_length; mylen += 4) {
	    proto_tree_add_text(rsvp_object_tree, tvb, offset+mylen+3, 1,
				"DSCP: %s",
				val_to_str(tvb_get_guint8(tvb, offset+mylen+3),
					   dscp_vals, "Unknown (%d)"));
	    proto_item_append_text(ti, "%d%s",
				   tvb_get_guint8(tvb, offset+mylen+3)>>2,
				   mylen==obj_length-4 ? "":
				   mylen<16 ? ", ":
				   mylen==16 ? ", ..." : "");
	}
	break;

    default:
	mylen = obj_length - 4;
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: Unknown (%u)",
			    type);
	proto_tree_add_text(rsvp_object_tree, tvb, offset2, mylen,
			    "Data (%d bytes)", mylen);
	break;
    }
}

/*------------------------------------------------------------------------------
 * ADMINISTRATIVE STATUS
 *------------------------------------------------------------------------------*/
static void
dissect_rsvp_admin_status (proto_tree *ti, tvbuff_t *tvb,
			   int offset, int obj_length,
			   int class, int type,
			   char *type_str)
{
    int offset2 = offset + 4;
    proto_tree *rsvp_object_tree;
    proto_tree *ti2, *rsvp_admin_subtree;
    int mylen;
    guint32 status;
    rsvp_object_tree = proto_item_add_subtree(ti, TREE(TT_ADMIN_STATUS));
    proto_tree_add_text(rsvp_object_tree, tvb, offset, 2,
			"Length: %u", obj_length);
    proto_tree_add_text(rsvp_object_tree, tvb, offset+2, 1,
			"Class number: %u - %s",
			class, type_str);
    proto_item_set_text(ti, "ADMIN STATUS: ");
    switch(type) {
    case 1:
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: 1");
	status = tvb_get_ntohl(tvb, offset2);
	ti2 = proto_tree_add_text(rsvp_object_tree, tvb, offset2, 4,
				  "Admin Status: 0x%08x", status);
	rsvp_admin_subtree =
	    proto_item_add_subtree(ti2, TREE(TT_ADMIN_STATUS_FLAGS));
	proto_tree_add_text(rsvp_admin_subtree, tvb, offset2, 4,
			    decode_boolean_bitfield(status, 0x80000000, 32,
						    "R: Reflect",
						    "R: Do not reflect"));
	proto_tree_add_text(rsvp_admin_subtree, tvb, offset2, 4,
			    decode_boolean_bitfield(status, 0x04, 32,
						    "T: Testing",
						    "T: "));
	proto_tree_add_text(rsvp_admin_subtree, tvb, offset2, 4,
			    decode_boolean_bitfield(status, 0x02, 32,
						    "A: Administratively Down",
						    "A: "));
	proto_tree_add_text(rsvp_admin_subtree, tvb, offset2, 4,
			    decode_boolean_bitfield(status, 0x01, 32,
						    "D: Delete In Progress",
						    "D: "));
	proto_item_set_text(ti, "ADMIN-STATUS: %s %s %s %s",
			    (status & (1<<31)) ? "Reflect" : "",
			    (status & (1<<2))  ? "Testing" : "",
			    (status & (1<<1))  ? "Admin-Down" : "",
			    (status & (1<<0))  ? "Deleting" : "");
	break;

    default:
	mylen = obj_length - 4;
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: Unknown (%u)",
			    type);
	proto_tree_add_text(rsvp_object_tree, tvb, offset2, obj_length - 4,
			    "Data (%d bytes)", obj_length - 4);
	break;
    }
}

/*------------------------------------------------------------------------------
 * LSP TUNNEL INTERFACE ID
 *------------------------------------------------------------------------------*/
static void
dissect_rsvp_lsp_tunnel_if_id (proto_tree *ti, tvbuff_t *tvb,
			       int offset, int obj_length,
			       int class, int type,
			       char *type_str)
{
    int offset2 = offset + 4;
    proto_tree *rsvp_object_tree;

    rsvp_object_tree = proto_item_add_subtree(ti, TREE(TT_LSP_TUNNEL_IF_ID));
    proto_tree_add_text(rsvp_object_tree, tvb, offset, 2,
			"Length: %u", obj_length);
    proto_tree_add_text(rsvp_object_tree, tvb, offset+2, 1,
			"Class number: %u - %s",
			class, type_str);
    proto_item_set_text(ti, "LSP INTERFACE-ID: ");
    switch(type) {
    case 1:
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: 1 - IPv4");
	proto_tree_add_text(rsvp_object_tree, tvb, offset2, 4,
			    "Router ID: %s",
			    ip_to_str(tvb_get_ptr(tvb, offset2, 4)));
	proto_tree_add_text(rsvp_object_tree, tvb, offset2+4, 4,
			    "Interface ID: %u", tvb_get_ntohl(tvb, offset2+4));
	proto_item_set_text(ti, "LSP INTERFACE-ID: IPv4, Router-ID %s, Interface-ID %d",
			    ip_to_str(tvb_get_ptr(tvb, offset2, 4)),
			    tvb_get_ntohl(tvb, offset2+4));
	break;

    default:
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: Unknown (%u)",
			    type);
	proto_tree_add_text(rsvp_object_tree, tvb, offset2, obj_length - 4,
			    "Data (%d bytes)", obj_length - 4);
	break;
    }
}

/*------------------------------------------------------------------------------
 * GENERALIZED UNI
 *------------------------------------------------------------------------------*/
static void
dissect_rsvp_gen_uni (proto_tree *ti, tvbuff_t *tvb,
		      int offset, int obj_length,
		      int class, int type,
		      char *type_str)
{
    int offset2 = offset + 4;
    proto_tree *rsvp_object_tree;
    int mylen, i, j, k, l, m;
    proto_tree *ti2, *rsvp_gen_uni_subtree;
    int s_len, s_class, s_type;

    rsvp_object_tree = proto_item_add_subtree(ti, TREE(TT_GEN_UNI));
    proto_tree_add_text(rsvp_object_tree, tvb, offset, 2,
			"Length: %u", obj_length);
    proto_tree_add_text(rsvp_object_tree, tvb, offset+2, 1,
			"Class number: %u - %s",
			class, type_str);
    proto_item_set_text(ti, "GENERALIZED UNI: ");

    mylen = obj_length - 4;
    switch(type) {
    case 1: {
	char *c;
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: 1");
	for (i=1, l = 0; l < mylen; i++) {
	    j = tvb_get_guint8(tvb, offset2+l+2);
	    switch(j) {
	    case 1:
	    case 2: /* We do source and destination TNA together */
		c = (j==1) ? "Source" : "Destination";
		k = tvb_get_guint8(tvb, offset2+l+3);
		switch(k) {
		case 1:
		    ti2 = proto_tree_add_text(rsvp_object_tree, tvb,
					      offset2+l, 8,
					      "%s IPv4 TNA - %s", c,
					      ip_to_str(tvb_get_ptr(tvb, offset2+l+4, 4)));
		    rsvp_gen_uni_subtree =
			proto_item_add_subtree(ti2, TREE(TT_GEN_UNI_SUBOBJ));
		    proto_tree_add_text(rsvp_gen_uni_subtree, tvb, offset2+l+2, 1,
					"Class: %d (%s)", j, c);
		    proto_tree_add_text(rsvp_gen_uni_subtree, tvb, offset2+l+3, 1,
					"Type: 1 (IPv4)");
		    proto_tree_add_text(rsvp_gen_uni_subtree, tvb, offset2+l, 2,
					"Length: %u",
					tvb_get_ntohs(tvb, offset2+l));
		    proto_tree_add_text(rsvp_gen_uni_subtree, tvb, offset2+l+4, 4,
					"IPv4 hop: %s",
					ip_to_str(tvb_get_ptr(tvb, offset2+l+4, 4)));
		    if (i < 4) {
			proto_item_append_text(ti, "%s IPv4 %s", c,
					       ip_to_str(tvb_get_ptr(tvb, offset2+l+2, 4)));
		    }
		    break;

		case 2:
		    ti2 = proto_tree_add_text(rsvp_object_tree, tvb,
					      offset2+l, 20,
					      "%s IPv6 TNA", c);
		    rsvp_gen_uni_subtree =
			proto_item_add_subtree(ti2, TREE(TT_GEN_UNI_SUBOBJ));
		    proto_tree_add_text(rsvp_gen_uni_subtree, tvb, offset2+l+2, 1,
					"Class: %d (%s)", j, c);
		    proto_tree_add_text(rsvp_gen_uni_subtree, tvb, offset2+l+3, 1,
					"Type: 2 (IPv6)");
		    proto_tree_add_text(rsvp_gen_uni_subtree, tvb, offset2+l, 2,
					"Length: %u",
					tvb_get_ntohs(tvb, offset2+l));
		    proto_tree_add_text(rsvp_gen_uni_subtree, tvb, offset2+l+4, 4,
					"Data");
		    if (i < 4) {
			proto_item_append_text(ti, "%s IPv6", c);
		    }
		    break;

		case 3:
		    ti2 = proto_tree_add_text(rsvp_object_tree, tvb,
					      offset2+l, tvb_get_ntohs(tvb, offset2+l),
					      "%s NSAP TNA", c);
		    rsvp_gen_uni_subtree =
			proto_item_add_subtree(ti2, TREE(TT_GEN_UNI_SUBOBJ));
		    proto_tree_add_text(rsvp_gen_uni_subtree, tvb, offset2+l+2, 1,
					"Class: %d (%s)", j, c);
		    proto_tree_add_text(rsvp_gen_uni_subtree, tvb, offset2+l+3, 1,
					"Type: 3 (NSAP)");
		    proto_tree_add_text(rsvp_gen_uni_subtree, tvb, offset2+l, 2,
					"Length: %u",
					tvb_get_ntohs(tvb, offset2+l));
		    proto_tree_add_text(rsvp_gen_uni_subtree, tvb, offset2+l+4,
					tvb_get_ntohs(tvb, offset2+l)-4,
					"Data");
		    if (i < 4) {
			proto_item_append_text(ti, "%s NSAP", c);
		    }
		    break;

		default:
		    ti2 = proto_tree_add_text(rsvp_object_tree, tvb,
					      offset2+l, tvb_get_ntohs(tvb, offset2+l),
					      "%s UNKNOWN TNA", c);
		    rsvp_gen_uni_subtree =
			proto_item_add_subtree(ti2, TREE(TT_GEN_UNI_SUBOBJ));
		    proto_tree_add_text(rsvp_gen_uni_subtree, tvb, offset2+l+2, 1,
					"Class: %d (%s)", j, c);
		    proto_tree_add_text(rsvp_gen_uni_subtree, tvb, offset2+l+3, 1,
					"Type: %d (UNKNOWN)", j);
		    proto_tree_add_text(rsvp_gen_uni_subtree, tvb, offset2+l, 2,
					"Length: %u",
					tvb_get_ntohs(tvb, offset2+l));
		    proto_tree_add_text(rsvp_gen_uni_subtree, tvb, offset2+l+4,
					tvb_get_ntohs(tvb, offset2+l)-4,
					"Data");
		    if (i < 4) {
			proto_item_append_text(ti, "%s UNKNOWN", c);
		    }
		    break;
		}
		break;

	    case 3: /* Diversity subobject */
		k = tvb_get_guint8(tvb, offset2+l+3);
		switch(k) {
		default:
		case 1:
		    ti2 = proto_tree_add_text(rsvp_object_tree, tvb,
					      offset2+l, tvb_get_ntohs(tvb, offset2+l),
					      "Diversity Subobject");
		    rsvp_gen_uni_subtree =
			proto_item_add_subtree(ti2, TREE(TT_GEN_UNI_SUBOBJ));
		    proto_tree_add_text(rsvp_gen_uni_subtree, tvb, offset2+l+2, 1,
					"Class: %d (Diversity)", j);
		    proto_tree_add_text(rsvp_gen_uni_subtree, tvb, offset2+l+3, 1,
					"Type: %d", tvb_get_guint8(tvb, offset2+l+3));
		    proto_tree_add_text(rsvp_gen_uni_subtree, tvb, offset2+l, 2,
					"Length: %u",
					tvb_get_ntohs(tvb, offset2+l));
		    m = tvb_get_guint8(tvb, offset2+l+4) >> 4;
		    proto_tree_add_text(rsvp_gen_uni_subtree, tvb, offset2+l+4, 1,
					"Diversity: %d - %s", m,
					val_to_str(m, ouni_guni_diversity_str, "Unknown"));
		    s_len = tvb_get_ntohs(tvb, offset2+l+8);
		    s_class = tvb_get_guint8(tvb, offset2+l+10);
		    s_type = tvb_get_guint8(tvb, offset2+l+11);
		    ti2 = proto_tree_add_text(rsvp_gen_uni_subtree, tvb, offset2+l+8,
					      s_len, "Session");
		    dissect_rsvp_session(ti2, tvb, offset2+l+8, s_len, s_class, s_type,
					 val_to_str(s_class, rsvp_class_vals, "Unknown"));
		    offset2 += s_len;
		    s_len = tvb_get_ntohs(tvb, offset2+l+8);
		    s_class = tvb_get_guint8(tvb, offset2+l+10);
		    s_type = tvb_get_guint8(tvb, offset2+l+11);
		    ti2 = proto_tree_add_text(rsvp_gen_uni_subtree, tvb, offset2+l+8,
					      s_len, "Template");
		    dissect_rsvp_template_filter(ti2, tvb, offset2+l+8, s_len, s_class, s_type,
						 val_to_str(s_class, rsvp_class_vals, "Unknown"));

		    if (i < 4) {
			proto_item_append_text(ti, "Diversity");
		    }
		    break;

		}
		break;

	    case 4: /* Egress Label */
		k = tvb_get_guint8(tvb, offset2+l+3);
		switch(k) {
		default:
		case 1:
		    ti2 = proto_tree_add_text(rsvp_object_tree, tvb,
					      offset2+l, tvb_get_ntohs(tvb, offset2+l),
					      "Egress Label Subobject");
		    rsvp_gen_uni_subtree =
			proto_item_add_subtree(ti2, TREE(TT_GEN_UNI_SUBOBJ));
		    proto_tree_add_text(rsvp_gen_uni_subtree, tvb, offset2+l+2, 1,
					"Class: %d (Egress Label)", j);
		    proto_tree_add_text(rsvp_gen_uni_subtree, tvb, offset2+l+3, 1,
					"Type: %d", tvb_get_guint8(tvb, offset2+l+3));
		    proto_tree_add_text(rsvp_gen_uni_subtree, tvb, offset2+l, 2,
					"Length: %u",
					tvb_get_ntohs(tvb, offset2+l));
		    proto_tree_add_text(rsvp_gen_uni_subtree, tvb, offset2+l+4, 1,
					"Direction: %s",
					decode_boolean_bitfield(
					    tvb_get_guint8(tvb, offset2+l+4), 0x80, 8,
					    "U: 1 - Upstream label/port ID",
					    "U: 0 - Downstream label/port ID"));
		    proto_tree_add_text(rsvp_gen_uni_subtree, tvb, offset2+l+7, 1,
					"Label type: %u", tvb_get_guint8(tvb, offset2+l+7));
		    proto_tree_add_text(rsvp_gen_uni_subtree, tvb, offset2+l+8, 4,
					"Logical Port ID: %u", tvb_get_ntohl(tvb, offset2+l+8));
		    proto_item_append_text(ti2, ": %s, Label type %d, Port ID %d, Label ",
					   tvb_get_guint8(tvb, offset2+l+4) & 0x80 ?
					   "Upstream" : "Downstream",
					   tvb_get_guint8(tvb, offset2+l+7),
					   tvb_get_ntohl(tvb, offset2+l+8));
		    for (j=12; j < tvb_get_ntohs(tvb, offset2+l); j+=4) {
			proto_tree_add_text(rsvp_gen_uni_subtree, tvb, offset2+l+8, 4,
					    "Label: %u", tvb_get_ntohl(tvb, offset2+l+j));
			proto_item_append_text(ti2, "%u ", tvb_get_ntohl(tvb, offset2+l+j));
		    }
		    if (i < 4) {
			proto_item_append_text(ti, "Egress Label");
		    }
		    break;
		}
		break;

	    case 5: /* Service Level */
		k = tvb_get_guint8(tvb, offset2+l+3);
		switch(k) {
		default:
		case 1:
		    ti2 = proto_tree_add_text(rsvp_object_tree, tvb,
					      offset2+l, tvb_get_ntohs(tvb, offset2+l),
					      "Service Level Subobject");
		    rsvp_gen_uni_subtree =
			proto_item_add_subtree(ti2, TREE(TT_GEN_UNI_SUBOBJ));
		    proto_tree_add_text(rsvp_gen_uni_subtree, tvb, offset2+l+2, 1,
					"Class: %d (Egress Label)", j);
		    proto_tree_add_text(rsvp_gen_uni_subtree, tvb, offset2+l+3, 1,
					"Type: %d", tvb_get_guint8(tvb, offset2+l+3));
		    proto_tree_add_text(rsvp_gen_uni_subtree, tvb, offset2+l, 2,
					"Length: %u",
					tvb_get_ntohs(tvb, offset2+l));
		    proto_tree_add_text(rsvp_gen_uni_subtree, tvb, offset2+l+4, 1,
					"Service Level: %u", tvb_get_guint8(tvb, offset2+l+4));
		    proto_item_append_text(ti2, ": %u", tvb_get_guint8(tvb, offset2+l+4));
		    if (i < 4) {
			proto_item_append_text(ti, "Service Level %d", tvb_get_guint8(tvb, offset2+l+4));
		    }
		    break;
		}
		break;

	    default: /* Unknown subobject */
		ti2 = proto_tree_add_text(rsvp_object_tree, tvb,
					  offset2+l,
					  tvb_get_ntohs(tvb, offset2+l),
					  "Unknown subobject: %u",
					  j);
		rsvp_gen_uni_subtree =
		    proto_item_add_subtree(ti2, TREE(TT_GEN_UNI_SUBOBJ));
		proto_tree_add_text(rsvp_gen_uni_subtree, tvb, offset2+l, 1,
				    "Type: %u (Unknown)", j);
		proto_tree_add_text(rsvp_gen_uni_subtree, tvb, offset2+l+1, 1,
				    "Length: %u",
				    tvb_get_guint8(tvb, offset2+l+1));

	    }

	    l += tvb_get_guint8(tvb, offset2+l+1);
	    if (l < mylen) {
		if (i < 4)
		    proto_item_append_text(ti, ", ");
		else if (i==4)
		    proto_item_append_text(ti, "...");
	    }
	}
	break;
    }

    default:
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: Unknown (%u)",
			    type);
	proto_tree_add_text(rsvp_object_tree, tvb, offset2, mylen,
			    "Data (%d bytes)", mylen);
	break;
    }
}

/*------------------------------------------------------------------------------
 * RESTART CAPABILITY
 *------------------------------------------------------------------------------*/
static void
dissect_rsvp_restart_cap (proto_tree *ti, tvbuff_t *tvb,
			  int offset, int obj_length,
			  int class, int type,
			  char *type_str)
{
    int offset2 = offset + 4;
    proto_tree *rsvp_object_tree;

    rsvp_object_tree = proto_item_add_subtree(ti, TREE(TT_RESTART_CAP));
    proto_tree_add_text(rsvp_object_tree, tvb, offset, 2,
			"Length: %u", obj_length);
    proto_tree_add_text(rsvp_object_tree, tvb, offset+2, 1,
			"Class number: %u - %s",
			class, type_str);
    proto_item_set_text(ti, "RESTART CAPABILITY: ");
    switch(type) {
    case 1:
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: 1");
	proto_tree_add_text(rsvp_object_tree, tvb, offset2, 4,
			    "Restart Time: %d ms", 
			    tvb_get_ntohl(tvb, offset2));
	proto_tree_add_text(rsvp_object_tree, tvb, offset2+4, 4,
			    "Recovery Time: %d ms", 
			    tvb_get_ntohl(tvb, offset2+4));
	proto_item_append_text(ti, "Restart Time: %d ms. Recovery Time: %d ms.",
			    tvb_get_ntohl(tvb, offset2), tvb_get_ntohl(tvb, offset2+4));
	break;

    default:
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: Unknown (%u)",
			    type);
	proto_tree_add_text(rsvp_object_tree, tvb, offset2, obj_length - 4,
			    "Data (%d bytes)", obj_length - 4);
	break;
    }
}

/*------------------------------------------------------------------------------
 * PROTECTION INFORMATION
 *------------------------------------------------------------------------------*/
static void
dissect_rsvp_protection_info (proto_tree *ti, tvbuff_t *tvb,
			      int offset, int obj_length,
			      int class, int type,
			      char *type_str)
{
    int offset2 = offset + 4;
    proto_tree *rsvp_object_tree;

    rsvp_object_tree = proto_item_add_subtree(ti, TREE(TT_PROTECTION_INFO));
    proto_tree_add_text(rsvp_object_tree, tvb, offset, 2,
			"Length: %u", obj_length);
    proto_tree_add_text(rsvp_object_tree, tvb, offset+2, 1,
			"Class number: %u - %s",
			class, type_str);
    proto_item_set_text(ti, "PROTECTION_INFO: ");
    switch(type) {
    case 1:
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: 1 - IPv4");
	proto_tree_add_text(rsvp_object_tree, tvb, offset2, 4,
			    "Router ID: %s",
			    ip_to_str(tvb_get_ptr(tvb, offset2, 4)));
	proto_tree_add_text(rsvp_object_tree, tvb, offset2+4, 4,
			    "Interface ID: %u", tvb_get_ntohl(tvb, offset2+4));
	proto_item_append_text(ti, "Router-ID %s, Interface-ID %d",
			    ip_to_str(tvb_get_ptr(tvb, offset2, 4)),
			    tvb_get_ntohl(tvb, offset2+4));
	break;

    default:
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: Unknown (%u)",
			    type);
	proto_tree_add_text(rsvp_object_tree, tvb, offset2, obj_length - 4,
			    "Data (%d bytes)", obj_length - 4);
	break;
    }
}

/*------------------------------------------------------------------------------
 * FAST REROUTE
 *------------------------------------------------------------------------------*/
static void
dissect_rsvp_fast_reroute (proto_tree *ti, tvbuff_t *tvb,
			   int offset, int obj_length,
			   int class, int type,
			   char *type_str)
{
    proto_tree *rsvp_object_tree;
    guint8 flags;
    proto_tree *ti2, *rsvp_frr_flags_tree;

    rsvp_object_tree = proto_item_add_subtree(ti, TREE(TT_FAST_REROUTE));
    proto_tree_add_text(rsvp_object_tree, tvb, offset, 2,
			"Length: %u", obj_length);
    proto_tree_add_text(rsvp_object_tree, tvb, offset+2, 1,
			"Class number: %u - %s",
			class, type_str);
    proto_item_set_text(ti, "FAST_REROUTE: ");
    switch(type) {
    case 1:
    case 7:
	if ((type==1 && obj_length!=24) || (type==7 && obj_length!=20)) {
	    proto_tree_add_text(rsvp_object_tree, tvb, offset, obj_length, 
				"<<<Invalid length: cannot decode>>>");
	    proto_item_append_text(ti, "Invalid length");
	    break;
	}
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: %u", type);
	proto_tree_add_text(rsvp_object_tree, tvb, offset+4, 1, 
			    "Setup Priority: %d", tvb_get_guint8(tvb, offset+4));
	proto_tree_add_text(rsvp_object_tree, tvb, offset+5, 1, 
			    "Hold Priority: %d", tvb_get_guint8(tvb, offset+5));
	proto_tree_add_text(rsvp_object_tree, tvb, offset+6, 1, 
			    "Hop Limit: %d", tvb_get_guint8(tvb, offset+6));

        flags = tvb_get_guint8(tvb, offset+7);
	ti2 = proto_tree_add_text(rsvp_object_tree, tvb, offset+7, 1, 
                                  "Flags: 0x%02x", flags);
        rsvp_frr_flags_tree = proto_item_add_subtree(ti2,
                                                     TREE(TT_FAST_REROUTE_FLAGS));
	proto_tree_add_text(rsvp_frr_flags_tree, tvb, offset+7, 1,
			    decode_boolean_bitfield(flags, 0x01, 8,
						    "One-to-One Backup desired",
						    "One-to-One Backup not desired"));
	proto_tree_add_text(rsvp_frr_flags_tree, tvb, offset+7, 1,
			    decode_boolean_bitfield(flags, 0x02, 8,
						    "Facility Backup desired",
						    "Facility Backup not desired"));
	proto_tree_add_text(rsvp_object_tree, tvb, offset+8, 4, 
			    "Bandwidth: %.10g", tvb_get_ntohieee_float(tvb, offset+8));
	proto_tree_add_text(rsvp_object_tree, tvb, offset+12, 4, 
			    "Exclude-Any: 0x%0x", tvb_get_ntohl(tvb, offset+12));
	proto_tree_add_text(rsvp_object_tree, tvb, offset+16, 4, 
			    "Include-Any: 0x%0x", tvb_get_ntohl(tvb, offset+16));
	if (type==1) {
	    proto_tree_add_text(rsvp_object_tree, tvb, offset+20, 4, 
				"Include-All: 0x%0x", tvb_get_ntohl(tvb, offset+20));
	}

        proto_item_append_text(ti, "%s%s",
                               flags &0x01 ? "One-to-One Backup, " : "",
                               flags &0x02 ? "Facility Backup" : "");
	break;

    default:
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: Unknown (%u)",
			    type);
	proto_tree_add_text(rsvp_object_tree, tvb, offset+4, obj_length - 4,
			    "Data (%d bytes)", obj_length - 4);
	break;
    }
}

/*------------------------------------------------------------------------------
 * DETOUR
 *------------------------------------------------------------------------------*/
static void
dissect_rsvp_detour (proto_tree *ti, tvbuff_t *tvb,
		     int offset, int obj_length,
		     int class, int type,
		     char *type_str)
{
    proto_tree *rsvp_object_tree;
    int remaining_length, count;
    int iter;

    rsvp_object_tree = proto_item_add_subtree(ti, TREE(TT_DETOUR));
    proto_tree_add_text(rsvp_object_tree, tvb, offset, 2,
			"Length: %u", obj_length);
    proto_tree_add_text(rsvp_object_tree, tvb, offset+2, 1,
			"Class number: %u - %s",
			class, type_str);
    proto_item_set_text(ti, "DETOUR: ");
    switch(type) {
    case 7:
        iter = 0;
        proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: %u", type);
	for (remaining_length = obj_length - 4, count = 1;
	     remaining_length > 0; remaining_length -= 8, count++) {
	    if (remaining_length < 8) {
		proto_tree_add_text(rsvp_object_tree, tvb, offset+remaining_length, 
				    obj_length-remaining_length, 
				    "<<<Invalid length: cannot decode>>>");
		proto_item_append_text(ti, "Invalid length");
		break;
	    }
	    iter++;
	    proto_tree_add_text(rsvp_object_tree, tvb, offset+(4*iter), 4,
				"PLR ID %d: %s", count, 
				ip_to_str(tvb_get_ptr(tvb, offset+(4*iter), 4)));
	    iter++;
	    proto_tree_add_text(rsvp_object_tree, tvb, offset+(4*iter), 4,
				"Avoid Node ID %d: %s", count, 
				ip_to_str(tvb_get_ptr(tvb, offset+(4*iter), 4)));
	}
	break;

    default:
	proto_tree_add_text(rsvp_object_tree, tvb, offset+3, 1,
			    "C-type: Unknown (%u)",
			    type);
	proto_tree_add_text(rsvp_object_tree, tvb, offset+4, obj_length - 4,
			    "Data (%d bytes)", obj_length - 4);
	break;
    }
}

/*------------------------------------------------------------------------------
 * DIFFSERV
 *------------------------------------------------------------------------------*/
static void
dissect_rsvp_diffserv (proto_tree *ti, tvbuff_t *tvb,
		       int offset, int obj_length,
		       int class, int type,
		       char *type_str
		       )
{
    proto_tree *rsvp_object_tree;
    int mapnb, count;
    int *hfindexes[] = {
	&rsvp_filter[RSVPF_DIFFSERV_MAP],
	&rsvp_filter[RSVPF_DIFFSERV_MAP_EXP],
	&rsvp_filter[RSVPF_DIFFSERV_PHBID],
	&rsvp_filter[RSVPF_DIFFSERV_PHBID_DSCP],
	&rsvp_filter[RSVPF_DIFFSERV_PHBID_CODE],
	&rsvp_filter[RSVPF_DIFFSERV_PHBID_BIT14],
	&rsvp_filter[RSVPF_DIFFSERV_PHBID_BIT15]
    };
    gint *etts[] = {
	&TREE(TT_DIFFSERV_MAP),
	&TREE(TT_DIFFSERV_MAP_PHBID)
    };

    rsvp_object_tree = proto_item_add_subtree(ti, TREE(TT_DIFFSERV));
    proto_tree_add_text(rsvp_object_tree, tvb, offset, 2,
			"Length: %u", obj_length);
    proto_tree_add_text(rsvp_object_tree, tvb, offset+2, 1,
			"Class number: %u - %s",
			class, type_str);
    proto_item_set_text(ti, "DIFFSERV: ");
    offset += 3;
    switch (type) {
    case 1:
	proto_tree_add_text(rsvp_object_tree, tvb, offset, 1,
			    "C-type: 1 - E-LSP");
	proto_tree_add_uint(rsvp_object_tree, rsvp_filter[RSVPF_DIFFSERV_MAPNB],
			    tvb, offset + 4, 1,
			    mapnb = tvb_get_guint8(tvb, offset + 4) & 15);
	proto_item_append_text(ti, "E-LSP, %u MAP%s", mapnb,
			       (mapnb == 0) ? "" : "s");
	offset += 5;

	for (count = 0; count < mapnb; count++) {
	    dissect_diffserv_mpls_common(tvb, rsvp_object_tree, type,
					 offset, hfindexes, etts);
	    offset += 4;
	}
	break;
    case 2:
	proto_item_append_text(ti, "L-LSP");
	proto_tree_add_text(rsvp_object_tree, tvb, offset, 1,
			    "C-type: 2 - L-LSP");
	dissect_diffserv_mpls_common(tvb, rsvp_object_tree, type,
				     offset + 3, hfindexes, etts);
	break;
    default:
	proto_tree_add_text(rsvp_object_tree, tvb, offset, 1,
			    "C-type: Unknown (%u)", type);
	proto_tree_add_text(rsvp_object_tree, tvb, offset + 1, obj_length - 4,
			    "Data (%d bytes)", obj_length - 4);
    }
}

/*------------------------------------------------------------------------------
 * Dissect a single RSVP message in a tree
 *------------------------------------------------------------------------------*/
static void
dissect_rsvp_msg_tree(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
		      int tree_mode)
{
    proto_tree *rsvp_tree = NULL;
    proto_tree *rsvp_header_tree;
    proto_tree *rsvp_object_tree;
    proto_tree *ti;
    guint16 cksum, computed_cksum;
    vec_t cksum_vec[1];
    int offset = 0;
    int len;
    guint8 ver_flags;
    guint8 message_type;
    int session_off, tempfilt_off;
    int msg_length;
    int obj_length;
    int offset2;

    offset = 0;
    len = 0;
    ver_flags = tvb_get_guint8(tvb, 0);
    msg_length = tvb_get_ntohs(tvb, 6);
    message_type = tvb_get_guint8(tvb, 1);

    ti = proto_tree_add_item(tree, proto_rsvp, tvb, offset, msg_length,
			     FALSE);
    rsvp_tree = proto_item_add_subtree(ti, tree_mode);
    proto_item_append_text(rsvp_tree, ": ");
    proto_item_append_text(rsvp_tree, val_to_str(message_type, message_type_vals,
						 "Unknown (%u). "));
    find_rsvp_session_tempfilt(tvb, 0, &session_off, &tempfilt_off);
    if (session_off)
	proto_item_append_text(rsvp_tree, summary_session(tvb, session_off));
    if (tempfilt_off)
	proto_item_append_text(rsvp_tree, summary_template(tvb, tempfilt_off));

    ti = proto_tree_add_text(rsvp_tree, tvb, offset, 8, "RSVP Header. %s",
			     val_to_str(message_type, message_type_vals,
					"Unknown Message (%u). "));
    rsvp_header_tree = proto_item_add_subtree(ti, TREE(TT_HDR));

    proto_tree_add_text(rsvp_header_tree, tvb, offset, 1, "RSVP Version: %u",
			(ver_flags & 0xf0)>>4);
    proto_tree_add_text(rsvp_header_tree, tvb, offset, 1, "Flags: %02x",
			ver_flags & 0xf);
    proto_tree_add_uint(rsvp_header_tree, rsvp_filter[RSVPF_MSG], tvb,
			offset+1, 1, message_type);
    switch (RSVPF_MSG + message_type) {

    case RSVPF_PATH:
    case RSVPF_RESV:
    case RSVPF_PATHERR:
    case RSVPF_RESVERR:
    case RSVPF_PATHTEAR:
    case RSVPF_RESVTEAR:
    case RSVPF_RCONFIRM:
    case RSVPF_RTEARCONFIRM:
    case RSVPF_BUNDLE:
    case RSVPF_ACK:
    case RSVPF_SREFRESH:
    case RSVPF_HELLO:
	proto_tree_add_boolean_hidden(rsvp_header_tree, rsvp_filter[RSVPF_MSG + message_type], tvb,
				      offset+1, 1, 1);
	break;

    default:
	proto_tree_add_protocol_format(rsvp_header_tree, proto_malformed, tvb, offset+1, 1,
				       "Invalid message type: %u", message_type);
	return;
    }

    cksum = tvb_get_ntohs(tvb, offset+2);
    if (!pinfo->fragmented && (int) tvb_length(tvb) >= msg_length) {
	/* The packet isn't part of a fragmented datagram and isn't
	   truncated, so we can checksum it. */
	cksum_vec[0].ptr = tvb_get_ptr(tvb, 0, msg_length);
	cksum_vec[0].len = msg_length;
	computed_cksum = in_cksum(&cksum_vec[0], 1);
	if (computed_cksum == 0) {
	    proto_tree_add_text(rsvp_header_tree, tvb, offset+2, 2,
				"Message Checksum: 0x%04x (correct)",
				cksum);
	} else {
	    proto_tree_add_text(rsvp_header_tree, tvb, offset+2, 2,
				"Message Checksum: 0x%04x (incorrect, should be 0x%04x)",
				cksum,
				in_cksum_shouldbe(cksum, computed_cksum));
	}
    } else {
	proto_tree_add_text(rsvp_header_tree, tvb, offset+2, 2,
			    "Message Checksum: 0x%04x",
			    cksum);
    }
    proto_tree_add_text(rsvp_header_tree, tvb, offset+4, 1,
			"Sending TTL: %u",
			tvb_get_guint8(tvb, offset+4));
    proto_tree_add_text(rsvp_header_tree, tvb, offset+6, 2,
			"Message length: %u", msg_length);

    offset = 8;
    len = 8;

    if (message_type == RSVP_MSG_BUNDLE) {
	/* Bundle message. Dissect component messages */
	if (rsvp_bundle_dissect) {
	    int len = 8;
	    while (len < msg_length) {
		gint sub_len;
		tvbuff_t *tvb_sub;
		sub_len = tvb_get_ntohs(tvb, len+6);
		tvb_sub = tvb_new_subset(tvb, len, sub_len, sub_len);
		dissect_rsvp_msg_tree(tvb_sub, pinfo, rsvp_tree, TREE(TT_BUNDLE_COMPMSG));
		len += sub_len;
	    }
	} else {
	    proto_tree_add_text(rsvp_tree, tvb, offset, msg_length - len,
				"Bundle Component Messages Not Dissected");
	}
	return;
    }

    while (len < msg_length) {
	guint8 class;
	guint8 type;
	char *type_str;

	obj_length = tvb_get_ntohs(tvb, offset);
	class = tvb_get_guint8(tvb, offset+2);
	type = tvb_get_guint8(tvb, offset+3);
	type_str = val_to_str(class, rsvp_class_vals, "Unknown");
	proto_tree_add_uint_hidden(rsvp_tree, rsvp_filter[RSVPF_OBJECT], tvb,
				   offset, obj_length, class);
	ti = proto_tree_add_item(rsvp_tree, rsvp_filter[rsvp_class_to_filter_num(class)],
				 tvb, offset, obj_length, FALSE);

	offset2 = offset+4;

	switch(class) {

	case RSVP_CLASS_SESSION:
	    dissect_rsvp_session(ti, tvb, offset, obj_length, class, type, type_str);
	    break;

	case RSVP_CLASS_HOP:
	    dissect_rsvp_hop(ti, tvb, offset, obj_length, class, type, type_str);
	    break;

	case RSVP_CLASS_TIME_VALUES:
	    dissect_rsvp_time_values(ti, tvb, offset, obj_length, class, type, type_str);
	    break;

	case RSVP_CLASS_ERROR:
	    dissect_rsvp_error(ti, tvb, offset, obj_length, class, type, type_str);
	    break;

	case RSVP_CLASS_SCOPE:
	    dissect_rsvp_scope(ti, tvb, offset, obj_length, class, type, type_str);
	    break;

	case RSVP_CLASS_STYLE:
	    dissect_rsvp_style(ti, tvb, offset, obj_length, class, type, type_str);
	    break;

	case RSVP_CLASS_CONFIRM:
	    dissect_rsvp_confirm(ti, tvb, offset, obj_length, class, type, type_str);
	    break;

	case RSVP_CLASS_SENDER_TEMPLATE:
	case RSVP_CLASS_FILTER_SPEC:
	    dissect_rsvp_template_filter(ti, tvb, offset, obj_length, class, type, type_str);
	    break;

	case RSVP_CLASS_SENDER_TSPEC:
	    dissect_rsvp_tspec(ti, tvb, offset, obj_length, class, type, type_str);
	    break;

	case RSVP_CLASS_FLOWSPEC:
	    dissect_rsvp_flowspec(ti, tvb, offset, obj_length, class, type, type_str);
	    break;

	case RSVP_CLASS_ADSPEC:
	    dissect_rsvp_adspec(ti, tvb, offset, obj_length, class, type, type_str);
	    break;

	case RSVP_CLASS_INTEGRITY:
	    dissect_rsvp_integrity(ti, tvb, offset, obj_length, class, type, type_str);
	    break;

	case RSVP_CLASS_POLICY:
	    dissect_rsvp_policy(ti, tvb, offset, obj_length, class, type, type_str);
	    break;

	case RSVP_CLASS_LABEL_REQUEST:
	    dissect_rsvp_label_request(ti, tvb, offset, obj_length, class, type, type_str);
	    break;

	case RSVP_CLASS_UPSTREAM_LABEL:
	case RSVP_CLASS_SUGGESTED_LABEL:
	case RSVP_CLASS_LABEL:
	    dissect_rsvp_label(ti, tvb, offset, obj_length, class, type, type_str);
	    break;

	case RSVP_CLASS_SESSION_ATTRIBUTE:
	    dissect_rsvp_session_attribute(ti, tvb, offset, obj_length, class, type, type_str);
	    break;

	case RSVP_CLASS_EXPLICIT_ROUTE:
	    dissect_rsvp_explicit_route(ti, tvb, offset, obj_length, class, type, type_str);
	    break;

	case RSVP_CLASS_RECORD_ROUTE:
	    dissect_rsvp_record_route(ti, tvb, offset, obj_length, class, type, type_str);
	    break;

	case RSVP_CLASS_MESSAGE_ID:
	    dissect_rsvp_message_id(ti, tvb, offset, obj_length, class, type, type_str);
	    break;

	case RSVP_CLASS_MESSAGE_ID_ACK:
	    dissect_rsvp_message_id_ack(ti, tvb, offset, obj_length, class, type, type_str);
	    break;

	case RSVP_CLASS_MESSAGE_ID_LIST:
	    dissect_rsvp_message_id_list(ti, tvb, offset, obj_length, class, type, type_str);
	    break;

	case RSVP_CLASS_HELLO:
	    dissect_rsvp_hello(ti, tvb, offset, obj_length, class, type, type_str);
	    break;

	case RSVP_CLASS_DCLASS:
	    dissect_rsvp_dclass(ti, tvb, offset, obj_length, class, type, type_str);
	    break;

	case RSVP_CLASS_ADMIN_STATUS:
	    dissect_rsvp_admin_status(ti, tvb, offset, obj_length, class, type, type_str);
	    break;

	case RSVP_CLASS_LSP_TUNNEL_IF_ID:
	    dissect_rsvp_lsp_tunnel_if_id(ti, tvb, offset, obj_length, class, type, type_str);
	    break;

	case RSVP_CLASS_GENERALIZED_UNI:
	    dissect_rsvp_gen_uni(ti, tvb, offset, obj_length, class, type, type_str);
	    break;

	case RSVP_CLASS_RESTART_CAP:
	    dissect_rsvp_restart_cap(ti, tvb, offset, obj_length, class, type, type_str);
	    break;

	case RSVP_CLASS_PROTECTION:
	    dissect_rsvp_protection_info(ti, tvb, offset, obj_length, class, type, type_str);
	    break;

	case RSVP_CLASS_FAST_REROUTE:
	    dissect_rsvp_fast_reroute(ti, tvb, offset, obj_length, class, type, type_str);
	    break;

	case RSVP_CLASS_DETOUR:
	    dissect_rsvp_detour(ti, tvb, offset, obj_length, class, type, type_str);
	    break;

	case RSVP_CLASS_DIFFSERV:
	    dissect_rsvp_diffserv(ti, tvb, offset, obj_length, class, type, type_str);
	    break;

	case RSVP_CLASS_NULL:
	default:
	    rsvp_object_tree = proto_item_add_subtree(ti, TREE(TT_UNKNOWN_CLASS));
	    proto_tree_add_text(rsvp_object_tree, tvb, offset, 2,
				"Length: %u", obj_length);
	    proto_tree_add_text(rsvp_object_tree, tvb, offset+2, 1,
				"Class number: %u - %s",
				class, type_str);
	    proto_tree_add_text(rsvp_object_tree, tvb, offset2, obj_length - 4,
				"Data (%d bytes)", obj_length - 4);
	    break;
	}

	if (obj_length == 0)
	    break;
	offset += obj_length;
	len += obj_length;
    }
}

/*------------------------------------------------------------------------------
 * The main loop
 *------------------------------------------------------------------------------*/
static void
dissect_rsvp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
    guint8 ver_flags;
    guint8 message_type;
    int msg_length;
    int session_off, tempfilt_off;

    if (check_col(pinfo->cinfo, COL_PROTOCOL))
        col_set_str(pinfo->cinfo, COL_PROTOCOL, "RSVP");
    if (check_col(pinfo->cinfo, COL_INFO))
        col_clear(pinfo->cinfo, COL_INFO);

    ver_flags = tvb_get_guint8(tvb, 0);
    message_type = tvb_get_guint8(tvb, 1);
    msg_length = tvb_get_ntohs(tvb, 6);

    if (check_col(pinfo->cinfo, COL_INFO)) {
        col_add_str(pinfo->cinfo, COL_INFO,
            val_to_str(message_type, message_type_vals, "Unknown (%u). "));
	find_rsvp_session_tempfilt(tvb, 0, &session_off, &tempfilt_off);
	if (session_off)
	    col_append_str(pinfo->cinfo, COL_INFO, summary_session(tvb, session_off));
	if (tempfilt_off)
	    col_append_str(pinfo->cinfo, COL_INFO, summary_template(tvb, tempfilt_off));
    }

    if (check_col(pinfo->cinfo, COL_INFO)) {
	col_add_str(pinfo->cinfo, COL_INFO,
		    val_to_str(message_type, message_type_vals, "Unknown (%u). "));
	if (message_type == RSVP_MSG_BUNDLE) {
	    col_add_str(pinfo->cinfo, COL_INFO,
			rsvp_bundle_dissect ?
			"Component Messages Dissected" :
			"Component Messages Not Dissected");
	} else {
	    find_rsvp_session_tempfilt(tvb, 0, &session_off, &tempfilt_off);
	    if (session_off)
		col_append_str(pinfo->cinfo, COL_INFO, summary_session(tvb, session_off));
	    if (tempfilt_off)
		col_append_str(pinfo->cinfo, COL_INFO, summary_template(tvb, tempfilt_off));
	}
    }

    if (tree) {
	dissect_rsvp_msg_tree(tvb, pinfo, tree, TREE(TT_RSVP));
    }
}

static void
register_rsvp_prefs (void)
{
    module_t *rsvp_module;

    rsvp_module = prefs_register_protocol(proto_rsvp, NULL);
    prefs_register_bool_preference(
	rsvp_module, "process_bundle",
	"Dissect sub-messages in BUNDLE message",
	"Specifies whether Ethereal should decode and display sub-messages within BUNDLE messages",
	&rsvp_bundle_dissect);
}

void
proto_register_rsvp(void)
{
    gint i;

    /* Build the tree array */
    for (i=0; i<TT_MAX; i++)
	ett_tree[i] = &(ett_treelist[i]);

    proto_rsvp = proto_register_protocol("Resource ReserVation Protocol (RSVP)",
					 "RSVP", "rsvp");
    proto_register_field_array(proto_rsvp, rsvpf_info, array_length(rsvpf_info));
    proto_register_subtree_array(ett_tree, array_length(ett_tree));
    register_rsvp_prefs();
}

void
proto_reg_handoff_rsvp(void)
{
	dissector_handle_t rsvp_handle;

	rsvp_handle = create_dissector_handle(dissect_rsvp, proto_rsvp);
	dissector_add("ip.proto", IP_PROTO_RSVP, rsvp_handle);
}
