/* packet-ppp.c
 * Routines for ppp packet disassembly
 *
 * $Id: packet-ppp.c,v 1.44 2000/11/19 08:54:01 guy Exp $
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@zing.org>
 *
 * This file created and by Mike Hall <mlh@io.com>
 * Copyright 1998
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

#include <glib.h>
#include "packet.h"
#include "packet-ppp.h"
#include "ppptypes.h"
#include "packet-atalk.h"
#include "packet-ip.h"
#include "packet-ipv6.h"
#include "packet-ipx.h"
#include "packet-vines.h"

static int proto_ppp = -1;

static gint ett_ppp = -1;
static gint ett_ipcp = -1;
static gint ett_ipcp_options = -1;
static gint ett_ipcp_ipaddrs_opt = -1;
static gint ett_ipcp_compressprot_opt = -1;
static gint ett_lcp = -1;
static gint ett_lcp_options = -1;
static gint ett_lcp_mru_opt = -1;
static gint ett_lcp_async_map_opt = -1;
static gint ett_lcp_authprot_opt = -1;
static gint ett_lcp_qualprot_opt = -1;
static gint ett_lcp_magicnum_opt = -1;
static gint ett_lcp_fcs_alternatives_opt = -1;
static gint ett_lcp_numbered_mode_opt = -1;
static gint ett_lcp_callback_opt = -1;
static gint ett_lcp_multilink_ep_disc_opt = -1;
static gint ett_lcp_internationalization_opt = -1;

static dissector_table_t subdissector_table;

static int proto_mp = -1;
static int hf_mp_frag_first = -1;
static int hf_mp_frag_last = -1;
static int hf_mp_sequence_num = -1;

static int ett_mp = -1;
static int ett_mp_flags = -1;

/* PPP structs and definitions */

typedef struct _e_ppphdr {
  guint8  ppp_addr;
  guint8  ppp_ctl;
  guint16 ppp_prot;
} e_ppphdr;

static const value_string ppp_vals[] = {
	{PPP_IP,        "IP"             },
	{PPP_AT,        "Appletalk"      },
	{PPP_IPX,       "Netware IPX/SPX"},
	{PPP_VJC_COMP,	"VJ compressed TCP"},
	{PPP_VJC_UNCOMP,"VJ uncompressed TCP"}, 
	{PPP_VINES,     "Vines"          },
        {PPP_MP,	"Multilink"},
	{PPP_IPV6,      "IPv6"           },
	{PPP_COMP,	"compressed packet" },
	{PPP_IPCP,	"IP Control Protocol" },
	{PPP_ATCP,	"AppleTalk Control Protocol" },
	{PPP_IPXCP,	"IPX Control Protocol" },
	{PPP_CCP,	"Compression Control Protocol" },
	{PPP_LCP,	"Link Control Protocol" },
	{PPP_PAP,	"Password Authentication Protocol"  },
	{PPP_LQR,	"Link Quality Report protocol" },
	{PPP_CHAP,	"Cryptographic Handshake Auth. Protocol" },
	{PPP_CBCP,	"Callback Control Protocol" },
	{0,             NULL            }
};

/* CP (LCP, IPCP, etc.) codes.
 * from pppd fsm.h 
 */
#define CONFREQ    1  /* Configuration Request */
#define CONFACK    2  /* Configuration Ack */
#define CONFNAK    3  /* Configuration Nak */
#define CONFREJ    4  /* Configuration Reject */
#define TERMREQ    5  /* Termination Request */
#define TERMACK    6  /* Termination Ack */
#define CODEREJ    7  /* Code Reject */

static const value_string cp_vals[] = {
	{CONFREQ,    "Configuration Request" },
	{CONFACK,    "Configuration Ack" },
	{CONFNAK,    "Configuration Nak" },
	{CONFREJ,    "Configuration Reject" },
	{TERMREQ,    "Termination Request" },
	{TERMACK,    "Termination Ack" },
	{CODEREJ,    "Code Reject" },
	{0,          NULL            } };

/*
 * LCP-specific packet types.
 */
#define PROTREJ    8  /* Protocol Reject */
#define ECHOREQ    9  /* Echo Request */
#define ECHOREP    10 /* Echo Reply */
#define DISCREQ    11 /* Discard Request */
#define IDENT      12 /* Identification */
#define TIMEREMAIN 13 /* Time remaining */

#define CBCP_OPT  6 /* Use callback control protocol */

static const value_string lcp_vals[] = {
	{CONFREQ,    "Configuration Request" },
	{CONFACK,    "Configuration Ack" },
	{CONFNAK,    "Configuration Nak" },
	{CONFREJ,    "Configuration Reject" },
	{TERMREQ,    "Termination Request" },
	{TERMACK,    "Termination Ack" },
	{CODEREJ,    "Code Reject" },
	{PROTREJ,    "Protocol Reject" },
	{ECHOREQ,    "Echo Request" },
	{ECHOREP,    "Echo Reply" },
	{DISCREQ,    "Discard Request" },
	{IDENT,      "Identification" },
	{TIMEREMAIN, "Time Remaining" },
	{0,          NULL }
};

/*
 * Options.  (LCP)
 */
#define CI_MRU			1	/* Maximum Receive Unit */
#define CI_ASYNCMAP		2	/* Async Control Character Map */
#define CI_AUTHTYPE		3	/* Authentication Type */
#define CI_QUALITY		4	/* Quality Protocol */
#define CI_MAGICNUMBER		5	/* Magic Number */
#define CI_PCOMPRESSION		7	/* Protocol Field Compression */
#define CI_ACCOMPRESSION	8	/* Address/Control Field Compression */
#define CI_FCS_ALTERNATIVES	9	/* FCS Alternatives (RFC 1570) */
#define CI_SELF_DESCRIBING_PAD	10	/* Self-Describing Pad (RFC 1570) */
#define CI_NUMBERED_MODE	11	/* Numbered Mode (RFC 1663) */
#define CI_CALLBACK		13	/* Callback (RFC 1570) */
#define CI_COMPOUND_FRAMES	15	/* Compound frames (RFC 1570) */
#define CI_MULTILINK_MRRU	17	/* Multilink MRRU (RFC 1717) */
#define CI_MULTILINK_SSNH	18	/* Multilink Short Sequence Number
					   Header (RFC 1717) */
#define CI_MULTILINK_EP_DISC	19	/* Multilink Endpoint Discriminator
					   (RFC 1717) */
#define CI_DCE_IDENTIFIER	21	/* DCE Identifier */
#define CI_MULTILINK_PLUS_PROC	22	/* Multilink Plus Procedure */
#define CI_LINK_DISC_FOR_BACP	23	/* Link Discriminator for BACP
					   (RFC 2125) */
#define CI_LCP_AUTHENTICATION	24	/* LCP Authentication Option */
#define CI_COBS			25	/* Consistent Overhead Byte
					   Stuffing */
#define CI_PREFIX_ELISION	26	/* Prefix elision */
#define CI_MULTILINK_HDR_FMT	27	/* Multilink header format */
#define CI_INTERNATIONALIZATION	28	/* Internationalization (RFC 2484) */
#define	CI_SDL_ON_SONET_SDH	29	/* Simple Data Link on SONET/SDH */

static void dissect_lcp_mru_opt(const ip_tcp_opt *optp, tvbuff_t *tvb,
			int offset, guint length, proto_tree *tree);
static void dissect_lcp_async_map_opt(const ip_tcp_opt *optp, tvbuff_t *tvb,
			int offset, guint length, proto_tree *tree);
static void dissect_lcp_protocol_opt(const ip_tcp_opt *optp, tvbuff_t *tvb,
			int offset, guint length, proto_tree *tree);
static void dissect_lcp_magicnumber_opt(const ip_tcp_opt *optp,
			tvbuff_t *tvb, int offset, guint length,
			proto_tree *tree);
static void dissect_lcp_fcs_alternatives_opt(const ip_tcp_opt *optp,
			tvbuff_t *tvb, int offset, guint length,
			proto_tree *tree);
static void dissect_lcp_numbered_mode_opt(const ip_tcp_opt *optp,
			tvbuff_t *tvb, int offset, guint length,
			proto_tree *tree);
static void dissect_lcp_self_describing_pad_opt(const ip_tcp_opt *optp,
			tvbuff_t *tvb, int offset, guint length,
			proto_tree *tree);
static void dissect_lcp_callback_opt(const ip_tcp_opt *optp, tvbuff_t *tvb,
			int offset, guint length, proto_tree *tree);
static void dissect_lcp_multilink_mrru_opt(const ip_tcp_opt *optp,
			tvbuff_t *tvb, int offset, guint length,
			proto_tree *tree);
static void dissect_lcp_multilink_ep_disc_opt(const ip_tcp_opt *optp,
			tvbuff_t *tvb, int offset, guint length,
			proto_tree *tree);
static void dissect_lcp_bap_link_discriminator_opt(const ip_tcp_opt *optp,
			tvbuff_t *tvb, int offset, guint length,
			proto_tree *tree);
static void dissect_lcp_internationalization_opt(const ip_tcp_opt *optp,
			tvbuff_t *tvb, int offset, guint length,
			proto_tree *tree);
static void dissect_mp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);

static const ip_tcp_opt lcp_opts[] = {
	{
		CI_MRU,
		"Maximum Receive Unit",
		&ett_lcp_mru_opt,
		FIXED_LENGTH,
		4,
		dissect_lcp_mru_opt
	},
	{
		CI_ASYNCMAP,
		"Async Control Character Map",
		&ett_lcp_async_map_opt,
		FIXED_LENGTH,
		6,
		dissect_lcp_async_map_opt
	},
	{
		CI_AUTHTYPE,
		"Authentication protocol",
		&ett_lcp_authprot_opt,
		VARIABLE_LENGTH,
		4,
		dissect_lcp_protocol_opt
	},
	{
		CI_QUALITY,
		"Quality protocol",
		&ett_lcp_qualprot_opt,
		VARIABLE_LENGTH,
		4,
		dissect_lcp_protocol_opt
	},
	{
		CI_MAGICNUMBER,
		NULL,
		&ett_lcp_magicnum_opt,
		FIXED_LENGTH,
		6,
		dissect_lcp_magicnumber_opt
	},
	{
		CI_PCOMPRESSION,
		"Protocol field compression",
		NULL,
		FIXED_LENGTH,
		2,
		NULL
	},
	{
		CI_ACCOMPRESSION,
		"Address/control field compression",
		NULL,
		FIXED_LENGTH,
		2,
		NULL
	},
	{
		CI_FCS_ALTERNATIVES,
		NULL,
		&ett_lcp_fcs_alternatives_opt,
		FIXED_LENGTH,
		3,
		dissect_lcp_fcs_alternatives_opt
	},
	{
		CI_SELF_DESCRIBING_PAD,
		NULL,
		NULL,
		FIXED_LENGTH,
		3,
		dissect_lcp_self_describing_pad_opt
	},
	{
		CI_NUMBERED_MODE,
		"Numbered mode",
		&ett_lcp_numbered_mode_opt,
		VARIABLE_LENGTH,
		4,
		dissect_lcp_numbered_mode_opt
	},
	{
		CI_CALLBACK,
		"Callback",
		&ett_lcp_callback_opt,
		VARIABLE_LENGTH,
		3,
		dissect_lcp_callback_opt,
	},
	{
		CI_COMPOUND_FRAMES,
		"Compound frames",
		NULL,
		FIXED_LENGTH,
		2,
		NULL
	},
	{
		CI_MULTILINK_MRRU,
		NULL,
		NULL,
		FIXED_LENGTH,
		4,
		dissect_lcp_multilink_mrru_opt
	},
	{
		CI_MULTILINK_SSNH,
		"Use short sequence number headers",
		NULL,
		FIXED_LENGTH,
		2,
		NULL
	},
	{
		CI_MULTILINK_EP_DISC,
		"Multilink endpoint discriminator",
		&ett_lcp_multilink_ep_disc_opt,
		VARIABLE_LENGTH,
		3,
		dissect_lcp_multilink_ep_disc_opt,
	},
	{
		CI_DCE_IDENTIFIER,
		"DCE identifier",
		NULL,
		VARIABLE_LENGTH,
		2,
		NULL
	},
	{
		CI_MULTILINK_PLUS_PROC,
		"Multilink Plus Procedure",
		NULL,
		VARIABLE_LENGTH,
		2,
		NULL
	},
	{
		CI_LINK_DISC_FOR_BACP,
		NULL,
		NULL,
		FIXED_LENGTH,
		4,
		dissect_lcp_bap_link_discriminator_opt
	},
	{
		CI_LCP_AUTHENTICATION,
		"LCP authentication",
		NULL,
		VARIABLE_LENGTH,
		2,
		NULL
	},
	{
		CI_COBS,
		"Consistent Overhead Byte Stuffing",
		NULL,
		VARIABLE_LENGTH,
		2,
		NULL
	},
	{
		CI_PREFIX_ELISION,
		"Prefix elision",
		NULL,
		VARIABLE_LENGTH,
		2,
		NULL
	},
	{
		CI_MULTILINK_HDR_FMT,
		"Multilink header format",
		NULL,
		VARIABLE_LENGTH,
		2,
		NULL
	},
	{
		CI_INTERNATIONALIZATION,
		"Internationalization",
		&ett_lcp_internationalization_opt,
		VARIABLE_LENGTH,
		7,
		dissect_lcp_internationalization_opt
	},
	{
		CI_SDL_ON_SONET_SDH,
		"Simple data link on SONET/SDH",
		NULL,
		VARIABLE_LENGTH,
		2,
		NULL
	}
};

#define N_LCP_OPTS	(sizeof lcp_opts / sizeof lcp_opts[0])

/*
 * Options.  (IPCP)
 */
#define CI_ADDRS	1	/* IP Addresses (deprecated) (RFC 1172) */
#define CI_COMPRESSTYPE	2	/* Compression Type (RFC 1332) */
#define CI_ADDR		3	/* IP Address (RFC 1332) */
#define CI_MOBILE_IPv4	4	/* Mobile IPv4 (RFC 2290) */
#define CI_MS_DNS1	129	/* Primary DNS value (RFC 1877) */
#define CI_MS_WINS1	130	/* Primary WINS value (RFC 1877) */
#define CI_MS_DNS2	131	/* Secondary DNS value (RFC 1877) */
#define CI_MS_WINS2	132	/* Secondary WINS value (RFC 1877) */

static void dissect_ipcp_addrs_opt(const ip_tcp_opt *optp, tvbuff_t *tvb,
			int offset, guint length, proto_tree *tree);
static void dissect_ipcp_addr_opt(const ip_tcp_opt *optp, tvbuff_t *tvb,
			int offset, guint length, proto_tree *tree);

static const ip_tcp_opt ipcp_opts[] = {
	{
		CI_ADDRS,
		"IP addresses (deprecated)",
		&ett_ipcp_ipaddrs_opt,
		FIXED_LENGTH,
		10,
		dissect_ipcp_addrs_opt
	},
	{
		CI_COMPRESSTYPE,
		"IP compression protocol",
		&ett_ipcp_compressprot_opt,
		VARIABLE_LENGTH,
		4,
		dissect_lcp_protocol_opt
	},
	{
		CI_ADDR,
		"IP address",
		NULL,
		FIXED_LENGTH,
		6,
		dissect_ipcp_addr_opt
	},
	{
		CI_MOBILE_IPv4,
		"Mobile node's home IP address",
		NULL,
		FIXED_LENGTH,
		6,
		dissect_ipcp_addr_opt
	},
	{
		CI_MS_DNS1,
		"Primary DNS server IP address",
		NULL,
		FIXED_LENGTH,
		6,
		dissect_ipcp_addr_opt
	},
	{
		CI_MS_WINS1,
		"Primary WINS server IP address",
		NULL,
		FIXED_LENGTH,
		6,
		dissect_ipcp_addr_opt
	},
	{
		CI_MS_DNS2,
		"Secondary DNS server IP address",
		NULL,
		FIXED_LENGTH,
		6,
		dissect_ipcp_addr_opt
	},
	{
		CI_MS_WINS2,
		"Secondary WINS server IP address",
		NULL,
		FIXED_LENGTH,
		6,
		dissect_ipcp_addr_opt
	}
};

#define N_IPCP_OPTS	(sizeof ipcp_opts / sizeof ipcp_opts[0])

void
capture_ppp( const u_char *pd, int offset, packet_counts *ld ) {
  switch (pntohs(&pd[offset + 2])) {
    case PPP_IP:
      capture_ip(pd, offset + 4, ld);
      break;
    case PPP_IPX:
      capture_ipx(pd, offset + 4, ld);
      break;
    case PPP_VINES:
      capture_vines(pd, offset + 4, ld);
      break;
    default:
      ld->other++;
      break;
  }
}

static void
dissect_lcp_mru_opt(const ip_tcp_opt *optp, tvbuff_t *tvb, int offset,
			guint length, proto_tree *tree)
{
  proto_tree_add_text(tree, tvb, offset, length, "MRU: %u",
			tvb_get_ntohs(tvb, offset + 2));
}

static void
dissect_lcp_async_map_opt(const ip_tcp_opt *optp, tvbuff_t *tvb, int offset,
			guint length, proto_tree *tree)
{
  proto_tree_add_text(tree, tvb, offset, length, "Async characters to map: 0x%08x",
			tvb_get_ntohl(tvb, offset + 2));
}

static void
dissect_lcp_protocol_opt(const ip_tcp_opt *optp, tvbuff_t *tvb, int offset,
			guint length, proto_tree *tree)
{
  guint16 protocol;
  proto_item *tf;
  proto_tree *field_tree = NULL;
  
  tf = proto_tree_add_text(tree, tvb, offset, length, "%s: %u byte%s",
	  optp->name, length, plurality(length, "", "s"));
  field_tree = proto_item_add_subtree(tf, *optp->subtree_index);
  offset += 2;
  length -= 2;
  protocol = tvb_get_ntohs(tvb, offset);
  proto_tree_add_text(field_tree, tvb, offset, 2, "%s: %s (0x%02x)", optp->name,
		val_to_str(protocol, ppp_vals, "Unknown"), protocol);
  offset += 2;
  length -= 2;
  if (length > 0)
    proto_tree_add_text(field_tree, tvb, offset, length, "Data (%d byte%s)", length,
    			plurality(length, "", "s"));
}

static void
dissect_lcp_magicnumber_opt(const ip_tcp_opt *optp, tvbuff_t *tvb,
			int offset, guint length, proto_tree *tree)
{
  proto_tree_add_text(tree, tvb, offset, length, "Magic number: 0x%08x",
			tvb_get_ntohl(tvb, offset + 2));
}

static void
dissect_lcp_fcs_alternatives_opt(const ip_tcp_opt *optp, tvbuff_t *tvb,
			int offset, guint length, proto_tree *tree)
{
  proto_item *tf;
  proto_tree *field_tree = NULL;
  guint8 alternatives;
  
  alternatives = tvb_get_guint8(tvb, offset + 2);
  tf = proto_tree_add_text(tree, tvb, offset, length, "%s: 0x%02x",
	  optp->name, alternatives);
  field_tree = proto_item_add_subtree(tf, *optp->subtree_index);
  offset += 2;
  if (alternatives & 0x1)
    proto_tree_add_text(field_tree, tvb, offset + 2, 1, "%s",
       decode_boolean_bitfield(alternatives, 0x1, 8, "Null FCS", NULL));
  if (alternatives & 0x2)
    proto_tree_add_text(field_tree, tvb, offset + 2, 1, "%s",
       decode_boolean_bitfield(alternatives, 0x2, 8, "CCITT 16-bit FCS", NULL));
  if (alternatives & 0x4)
    proto_tree_add_text(field_tree, tvb, offset + 2, 1, "%s",
       decode_boolean_bitfield(alternatives, 0x4, 8, "CCITT 32-bit FCS", NULL));
}

static void
dissect_lcp_self_describing_pad_opt(const ip_tcp_opt *optp, tvbuff_t *tvb,
			int offset, guint length, proto_tree *tree)
{
  proto_tree_add_text(tree, tvb, offset, length,
			"Maximum octets of self-describing padding: %u",
			tvb_get_guint8(tvb, offset + 2));
}

static void
dissect_lcp_numbered_mode_opt(const ip_tcp_opt *optp, tvbuff_t *tvb,
			int offset, guint length, proto_tree *tree)
{
  proto_item *tf;
  proto_tree *field_tree = NULL;
  
  tf = proto_tree_add_text(tree, tvb, offset, length, "%s: %u byte%s",
	  optp->name, length, plurality(length, "", "s"));
  field_tree = proto_item_add_subtree(tf, *optp->subtree_index);
  offset += 2;
  length -= 2;
  proto_tree_add_text(field_tree, tvb, offset, 1, "Window: %u",
			tvb_get_guint8(tvb, offset));
  offset += 1;
  length -= 1;
  if (length > 0)
    proto_tree_add_text(field_tree, tvb, offset, length, "Address (%d byte%s)",
			length, plurality(length, "", "s"));
}

static const value_string callback_op_vals[] = {
	{0, "Location is determined by user authentication" },
	{1, "Message is dialing string" },
	{2, "Message is location identifier" },
	{3, "Message is E.164" },
	{4, "Message is distinguished name" },
	{0, NULL }
};

static void
dissect_lcp_callback_opt(const ip_tcp_opt *optp, tvbuff_t *tvb, int offset,
			guint length, proto_tree *tree)
{
  proto_item *tf;
  proto_tree *field_tree = NULL;
  guint8 operation;
  
  tf = proto_tree_add_text(tree, tvb, offset, length, "%s: %u byte%s",
	  optp->name, length, plurality(length, "", "s"));
  field_tree = proto_item_add_subtree(tf, *optp->subtree_index);
  offset += 2;
  length -= 2;
  operation = tvb_get_guint8(tvb, offset);
  proto_tree_add_text(field_tree, tvb, offset, 1, "Operation: %s (0x%02x)",
		val_to_str(operation, callback_op_vals, "Unknown"),
		operation);
  offset += 1;
  length -= 1;
  if (length > 0)
    proto_tree_add_text(field_tree, tvb, offset, length, "Message (%d byte%s)",
			length, plurality(length, "", "s"));
}

static void
dissect_lcp_multilink_mrru_opt(const ip_tcp_opt *optp, tvbuff_t *tvb,
			int offset, guint length, proto_tree *tree)
{
  proto_tree_add_text(tree, tvb, offset, length, "Multilink MRRU: %u",
			tvb_get_ntohs(tvb, offset + 2));
}

#define CLASS_NULL			0
#define CLASS_LOCAL			1
#define CLASS_IP			2
#define CLASS_IEEE_802_1		3
#define CLASS_PPP_MAGIC_NUMBER		4
#define CLASS_PSDN_DIRECTORY_NUMBER	5

static const value_string multilink_ep_disc_class_vals[] = {
	{CLASS_NULL,                  "Null" },
	{CLASS_LOCAL,                 "Locally assigned address" },
	{CLASS_IP,                    "IP address" },
	{CLASS_IEEE_802_1,            "IEEE 802.1 globally assigned MAC address" },
	{CLASS_PPP_MAGIC_NUMBER,      "PPP magic-number block" },
	{CLASS_PSDN_DIRECTORY_NUMBER, "Public switched network directory number" },
	{0,                           NULL }
};

static void
dissect_lcp_multilink_ep_disc_opt(const ip_tcp_opt *optp, tvbuff_t *tvb,
			int offset, guint length, proto_tree *tree)
{
  proto_item *tf;
  proto_tree *field_tree = NULL;
  guint8 ep_disc_class;

  tf = proto_tree_add_text(tree, tvb, offset, length, "%s: %u byte%s",
	  optp->name, length, plurality(length, "", "s"));
  field_tree = proto_item_add_subtree(tf, *optp->subtree_index);
  offset += 2;
  length -= 2;
  ep_disc_class = tvb_get_guint8(tvb, offset);
  proto_tree_add_text(field_tree, tvb, offset, 1, "Class: %s (%u)",
		val_to_str(ep_disc_class, multilink_ep_disc_class_vals, "Unknown"),
		ep_disc_class);
  offset += 1;
  length -= 1;
  if (length > 0) {
    switch (ep_disc_class) {

    case CLASS_NULL:
      proto_tree_add_text(field_tree, tvb, offset, length,
			"Address (%d byte%s), should have been empty",
			length, plurality(length, "", "s"));
      break;

    case CLASS_LOCAL:
      if (length > 20) {
        proto_tree_add_text(field_tree, tvb, offset, length,
			"Address (%d byte%s), should have been <20",
			length, plurality(length, "", "s"));
      } else {
        proto_tree_add_text(field_tree, tvb, offset, length,
			"Address (%d byte%s)",
			length, plurality(length, "", "s"));
      }
      break;

    case CLASS_IP:
      if (length != 4) {
        proto_tree_add_text(field_tree, tvb, offset, length,
			"Address (%d byte%s), should have been 4",
			length, plurality(length, "", "s"));
      } else {
        proto_tree_add_text(field_tree, tvb, offset, length,
			"Address: %s", ip_to_str(tvb_get_ptr(tvb, offset, 4)));
      }
      break;

    case CLASS_IEEE_802_1:
      if (length != 6) {
        proto_tree_add_text(field_tree, tvb, offset, length,
			"Address (%d byte%s), should have been 6",
			length, plurality(length, "", "s"));
      } else {
        proto_tree_add_text(field_tree, tvb, offset, length,
			"Address: %s", ether_to_str(tvb_get_ptr(tvb, offset, 6)));
      }
      break;

    case CLASS_PPP_MAGIC_NUMBER:
      /* XXX - dissect as 32-bit magic numbers */
      if (length > 20) {
        proto_tree_add_text(field_tree, tvb, offset, length,
			"Address (%d byte%s), should have been <20",
			length, plurality(length, "", "s"));
      } else {
        proto_tree_add_text(field_tree, tvb, offset, length,
			"Address (%d byte%s)",
			length, plurality(length, "", "s"));
      }
      break;

    case CLASS_PSDN_DIRECTORY_NUMBER:
      if (length > 15) {
        proto_tree_add_text(field_tree, tvb, offset, length,
			"Address (%d byte%s), should have been <20",
			length, plurality(length, "", "s"));
      } else {
        proto_tree_add_text(field_tree, tvb, offset, length,
			"Address (%d byte%s)",
			length, plurality(length, "", "s"));
      }
      break;

    default:
      proto_tree_add_text(field_tree, tvb, offset, length,
			"Address (%d byte%s)",
			length, plurality(length, "", "s"));
      break;
    }
  }
}

static void
dissect_lcp_bap_link_discriminator_opt(const ip_tcp_opt *optp, tvbuff_t *tvb,
			int offset, guint length, proto_tree *tree)
{
  proto_tree_add_text(tree, tvb, offset, length,
			"Link discriminator for BAP: 0x%04x",
			tvb_get_ntohs(tvb, offset + 2));
}

/* Character set numbers from the IANA charset registry. */
static const value_string charset_num_vals[] = {
	{105, "UTF-8" },
	{0,   NULL }
};

static void
dissect_lcp_internationalization_opt(const ip_tcp_opt *optp, tvbuff_t *tvb,
			int offset, guint length, proto_tree *tree)
{
  proto_item *tf;
  proto_tree *field_tree = NULL;
  guint32 charset;
  
  tf = proto_tree_add_text(tree, tvb, offset, length, "%s: %u byte%s",
	  optp->name, length, plurality(length, "", "s"));
  field_tree = proto_item_add_subtree(tf, *optp->subtree_index);
  offset += 2;
  length -= 2;
  charset = tvb_get_ntohl(tvb, offset);
  proto_tree_add_text(field_tree, tvb, offset, 4, "Character set: %s (0x%04x)",
		val_to_str(charset, charset_num_vals, "Unknown"),
		charset);
  offset += 4;
  length -= 4;
  if (length > 0) {
    /* XXX - should be displayed as an ASCII string */
    proto_tree_add_text(field_tree, tvb, offset, length, "Language tag (%d byte%s)",
			length, plurality(length, "", "s"));
  }
}

static void
dissect_ipcp_addrs_opt(const ip_tcp_opt *optp, tvbuff_t *tvb,
			int offset, guint length, proto_tree *tree)
{
  proto_item *tf;
  proto_tree *field_tree = NULL;
  
  tf = proto_tree_add_text(tree, tvb, offset, length, "%s: %u byte%s",
	  optp->name, length, plurality(length, "", "s"));
  field_tree = proto_item_add_subtree(tf, *optp->subtree_index);
  offset += 2;
  length -= 2;
  proto_tree_add_text(field_tree, tvb, offset, 4,
			"Source IP address: %s",
			ip_to_str(tvb_get_ptr(tvb, offset, 4)));
  offset += 4;
  length -= 4;
  proto_tree_add_text(field_tree, tvb, offset, 4,
			"Destination IP address: %s",
			ip_to_str(tvb_get_ptr(tvb, offset, 4)));
}

static void dissect_ipcp_addr_opt(const ip_tcp_opt *optp, tvbuff_t *tvb,
			int offset, guint length, proto_tree *tree)
{
  proto_tree_add_text(tree, tvb, offset, length, "%s: %s", optp->name,
			ip_to_str(tvb_get_ptr(tvb, offset + 2, 4)));
}

static void
dissect_cp( tvbuff_t *tvb, const char *proto_short_name,
	const char *proto_long_name, int proto_subtree_index,
	const value_string *proto_vals, int options_subtree_index,
	const ip_tcp_opt *opts, int nopts, packet_info *pinfo, proto_tree *tree ) {
  proto_item *ti;
  proto_tree *fh_tree = NULL;
  proto_item *tf;
  proto_tree *field_tree;

  guint8 code;
  guint8 id;
  int length, offset;
  guint16 protocol;

  code = tvb_get_guint8(tvb, 0);
  id = tvb_get_guint8(tvb, 1);
  length = tvb_get_ntohs(tvb, 2);

  if(check_col(pinfo->fd, COL_INFO))
	col_add_fstr(pinfo->fd, COL_INFO, "%sCP %s", proto_short_name,
		val_to_str(code, proto_vals, "Unknown"));

  if(tree) {
    ti = proto_tree_add_text(tree, tvb, 0, 4, "%s Control Protocol",
				proto_long_name);
    fh_tree = proto_item_add_subtree(ti, proto_subtree_index);
    proto_tree_add_text(fh_tree, tvb, 0, 1, "Code: %s (0x%02x)",
      val_to_str(code, proto_vals, "Unknown"), code);
    proto_tree_add_text(fh_tree, tvb, 1, 1, "Identifier: 0x%02x",
			id);
    proto_tree_add_text(fh_tree, tvb, 2, 2, "Length: %u",
			length);
  }
  offset = 4;
  length -= 4;

  switch (code) {
    case CONFREQ:
    case CONFACK:
    case CONFNAK:
    case CONFREJ:
      if(tree) {
        if (length > 0) {
          tf = proto_tree_add_text(fh_tree, tvb, offset, length,
            "Options: (%d byte%s)", length, plurality(length, "", "s"));
          field_tree = proto_item_add_subtree(tf, options_subtree_index);
          dissect_ip_tcp_options(tvb, offset, length, opts, nopts, -1,
				 field_tree);
        }
      }
      break;

    case ECHOREQ:
    case ECHOREP:
    case DISCREQ:
    case IDENT:
      if(tree) {
	proto_tree_add_text(fh_tree, tvb, offset, 4, "Magic number: 0x%08x",
			tvb_get_ntohl(tvb, offset));
	offset += 4;
	length -= 4;
	if (length > 0)
          proto_tree_add_text(fh_tree, tvb, offset, length, "Message (%d byte%s)",
				length, plurality(length, "", "s"));
      }
      break;

    case TIMEREMAIN:
      if(tree) {
	proto_tree_add_text(fh_tree, tvb, offset, 4, "Magic number: 0x%08x",
			tvb_get_ntohl(tvb, offset));
	offset += 4;
	length -= 4;
	proto_tree_add_text(fh_tree, tvb, offset, 4, "Seconds remaining: %u",
			tvb_get_ntohl(tvb, offset));
	offset += 4;
	length -= 4;
	if (length > 0)
          proto_tree_add_text(fh_tree, tvb, offset, length, "Message (%d byte%s)",
				length, plurality(length, "", "s"));
      }
      break;

    case PROTREJ:
      if(tree) {
      	protocol = tvb_get_ntohs(tvb, offset);
	proto_tree_add_text(fh_tree, tvb, offset, 2, "Rejected protocol: %s (0x%04x)",
		val_to_str(protocol, ppp_vals, "Unknown"), protocol);
	offset += 2;
	length -= 2;
	if (length > 0)
          proto_tree_add_text(fh_tree, tvb, offset, length, "Rejected packet (%d byte%s)",
				length, plurality(length, "", "s"));
		/* XXX - should be dissected as a PPP packet */
      }
      break;

    case CODEREJ:
		/* decode the rejected LCP packet here. */
      if (length > 0)
        proto_tree_add_text(fh_tree, tvb, offset, length, "Rejected packet (%d byte%s)",
				length, plurality(length, "", "s"));
      break;

    case TERMREQ:
    case TERMACK:
      if (length > 0)
        proto_tree_add_text(fh_tree, tvb, offset, length, "Data (%d byte%s)",
				length, plurality(length, "", "s"));
      break;

    default:
      if (length > 0)
        proto_tree_add_text(fh_tree, tvb, offset, length, "Stuff (%d byte%s)",
				length, plurality(length, "", "s"));
      break;
  }
}

/* Protocol field compression */
#define PFC_BIT 0x01

static gboolean
dissect_ppp_stuff( tvbuff_t *tvb, packet_info *pinfo,
		proto_tree *tree, proto_tree *fh_tree ) {
  guint16 ppp_prot;
  int     proto_len;
  tvbuff_t	*next_tvb;

  if (tvb_get_guint8(tvb, 0) & PFC_BIT) {
    ppp_prot = tvb_get_guint8(tvb, 0);
    proto_len = 1;
  } else {
    ppp_prot = tvb_get_ntohs(tvb, 0);
    proto_len = 2;
  }

  if (tree) {
    proto_tree_add_text(fh_tree, tvb, 0, proto_len, "Protocol: %s (0x%04x)",
      val_to_str(ppp_prot, ppp_vals, "Unknown"), ppp_prot);
  }

  next_tvb = tvb_new_subset(tvb, proto_len, -1, -1);

  /* do lookup with the subdissector table */
  if (dissector_try_port(subdissector_table, ppp_prot, next_tvb, pinfo, tree))
      return TRUE;

  /* XXX - make "dissect_lcp()" and "dissect_ipcp()", have them just
     call "dissect_cp()", and register them as well?

     We can do that for "dissect_mp()", too. */
  switch (ppp_prot) {
    case PPP_MP:
      dissect_mp(next_tvb, pinfo, tree);
      return TRUE;
    case PPP_LCP:
      dissect_cp(next_tvb, "L", "Link", ett_lcp, lcp_vals, ett_lcp_options,
		lcp_opts, N_LCP_OPTS, pinfo, tree);
      return TRUE;
    case PPP_IPCP:
      dissect_cp(next_tvb, "IP", "IP", ett_ipcp, cp_vals, ett_ipcp_options,
		ipcp_opts, N_IPCP_OPTS, pinfo, tree);
      return TRUE;
    default:
      if (check_col(pinfo->fd, COL_INFO))
        col_add_fstr(pinfo->fd, COL_INFO, "PPP %s (0x%04x)",
		val_to_str(ppp_prot, ppp_vals, "Unknown"), ppp_prot);
      dissect_data(next_tvb, 0, pinfo, tree);
      return FALSE;
  }
}

#define MP_FRAG_MASK     0xC0
#define MP_FRAG(bits)    ((bits) & MP_FRAG_MASK)
#define MP_FRAG_FIRST    0x80
#define MP_FRAG_LAST     0x40
#define MP_FRAG_RESERVED 0x3f

/* According to RFC 1717, the length the MP header isn't indicated anywhere
   in the header itself.  It starts out at four bytes and can be
   negotiated down to two using LCP.  We currently assume that all
   headers are four bytes.  - gcc
 */
static void
dissect_mp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
  proto_tree *mp_tree, *hdr_tree, *fh_tree = NULL;
  proto_item *ti;
  guint8      flags;
  guint32     seq;
  gchar       *flag_str;
  int         first, last;
  tvbuff_t	*next_tvb;

  CHECK_DISPLAY_AS_DATA(proto_mp, tvb, pinfo, tree);

  flags = tvb_get_guint8(tvb, 0);
  first = flags && MP_FRAG_FIRST;
  last  = flags && MP_FRAG_LAST;
  seq   = tvb_get_ntoh24(tvb, 1);

  if (check_col(pinfo->fd, COL_INFO))
    col_add_fstr(pinfo->fd, COL_INFO, "PPP Multilink");

  if (tree) {
    switch (flags) {
      case MP_FRAG_FIRST:
        flag_str = "First";
        break;
      case MP_FRAG_LAST:
        flag_str = "Last";
        break;
      case MP_FRAG_FIRST|MP_FRAG_LAST:
        flag_str = "First, Last";
        break;
      default:
        flag_str = "Unknown";
        break;
    }
    ti = proto_tree_add_item(tree, proto_mp, tvb, 0, 4, FALSE);
    mp_tree = proto_item_add_subtree(ti, ett_mp);
    ti = proto_tree_add_text(mp_tree, tvb, 0, 1, "Fragment: 0x%2X (%s)",
      flags, flag_str);
    hdr_tree = proto_item_add_subtree(ti, ett_mp_flags);
    proto_tree_add_boolean_format(hdr_tree, hf_mp_frag_first, tvb, 0, 1, first,
      "%s", decode_boolean_bitfield(flags, MP_FRAG_FIRST, sizeof(flags) * 8,
        "first", "not first"));
    proto_tree_add_boolean_format(hdr_tree, hf_mp_frag_last, tvb, 0, 1, last,
      "%s", decode_boolean_bitfield(flags, MP_FRAG_LAST, sizeof(flags) * 8,
        "last", "not last"));
    proto_tree_add_text(hdr_tree, tvb, 0, 1, "%s",
      decode_boolean_bitfield(flags, MP_FRAG_RESERVED, sizeof(flags) * 8,
        "reserved", "reserved"));
    proto_tree_add_uint(mp_tree, hf_mp_sequence_num, tvb,  1, 3, seq);
 }

  next_tvb = tvb_new_subset(tvb, 4, -1, -1);

  if (tvb_length(next_tvb) > 0) {
	  if (tree) {
	    ti = proto_tree_add_item(tree, proto_ppp, next_tvb, 0, 1, FALSE);
	    fh_tree = proto_item_add_subtree(ti, ett_ppp);
	  }
	  dissect_ppp_stuff(next_tvb, pinfo, tree, fh_tree);
  }
}

static void
dissect_payload_ppp( tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree ) {
  proto_item *ti;
  proto_tree *fh_tree = NULL;

  /* XXX - the length shouldn't be 2, it should be based on the length
     of the protocol field. */
  if(tree) {
    ti = proto_tree_add_item(tree, proto_ppp, tvb, 0, 2, FALSE);
    fh_tree = proto_item_add_subtree(ti, ett_ppp);
  }

  dissect_ppp_stuff(tvb, pinfo, tree, fh_tree);
}

void
dissect_ppp( tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree ) {
  e_ppphdr   ph;
  proto_item *ti;
  proto_tree *fh_tree = NULL;
  int        proto_offset;
  tvbuff_t   *next_tvb;
  guint8     byte0;

  CHECK_DISPLAY_AS_DATA(proto_ppp, tvb, pinfo, tree);

  pinfo->current_proto = "PPP";
  byte0 = tvb_get_guint8(tvb, 0);

  if (byte0 == 0xff) {
    ph.ppp_addr = tvb_get_guint8(tvb, 0);
    ph.ppp_ctl  = tvb_get_guint8(tvb, 1);
    ph.ppp_prot = tvb_get_ntohs(tvb, 2);
    proto_offset =  2;
  }
  else {
    /* address and control are compressed (NULL) */
    ph.ppp_prot = tvb_get_ntohs(tvb, 0);
    proto_offset = 0;
  }

  /* load the top pane info. This should be overwritten by
     the next protocol in the stack */

  if(check_col(pinfo->fd, COL_RES_DL_SRC))
    col_set_str(pinfo->fd, COL_RES_DL_SRC, "N/A" );
  if(check_col(pinfo->fd, COL_RES_DL_DST))
    col_set_str(pinfo->fd, COL_RES_DL_DST, "N/A" );
  if(check_col(pinfo->fd, COL_PROTOCOL))
    col_set_str(pinfo->fd, COL_PROTOCOL, "PPP" );

  if(tree) {
    ti = proto_tree_add_item(tree, proto_ppp, tvb, 0, 4, FALSE);
    fh_tree = proto_item_add_subtree(ti, ett_ppp);
    if (byte0 == 0xff) {
      proto_tree_add_text(fh_tree, tvb, 0, 1, "Address: %02x", ph.ppp_addr);
      proto_tree_add_text(fh_tree, tvb, 1, 1, "Control: %02x", ph.ppp_ctl);
    }
  }

  next_tvb = tvb_new_subset(tvb, proto_offset, -1, -1);

  if (!dissect_ppp_stuff(next_tvb, pinfo, tree, fh_tree)) {
    if (check_col(pinfo->fd, COL_PROTOCOL))
      col_add_fstr(pinfo->fd, COL_PROTOCOL, "0x%04x", ph.ppp_prot);
  }
}

void
proto_register_ppp(void)
{
/*        static hf_register_info hf[] = {
                { &variable,
                { "Name",           "ppp.abbreviation", TYPE, VALS_POINTER }},
        };*/
	static gint *ett[] = {
		&ett_ppp,
		&ett_ipcp,
		&ett_ipcp_options,
		&ett_ipcp_ipaddrs_opt,
		&ett_ipcp_compressprot_opt,
		&ett_lcp,
		&ett_lcp_options,
		&ett_lcp_mru_opt,
		&ett_lcp_async_map_opt,
		&ett_lcp_authprot_opt,
		&ett_lcp_qualprot_opt,
		&ett_lcp_magicnum_opt,
		&ett_lcp_fcs_alternatives_opt,
		&ett_lcp_numbered_mode_opt,
		&ett_lcp_callback_opt,
		&ett_lcp_multilink_ep_disc_opt,
		&ett_lcp_internationalization_opt,
	};

        proto_ppp = proto_register_protocol("Point-to-Point Protocol", "ppp");
 /*       proto_register_field_array(proto_ppp, hf, array_length(hf));*/
	proto_register_subtree_array(ett, array_length(ett));

/* subdissector code */
	subdissector_table = register_dissector_table("ppp.protocol");

	register_dissector("ppp", dissect_ppp);
	register_dissector("payload_ppp", dissect_payload_ppp);
}

void
proto_register_mp(void)
{
  static hf_register_info hf[] = {
    { &hf_mp_frag_first,
    { "First fragment",		"mp.first",	FT_BOOLEAN, BASE_NONE, NULL, 0x0,
    	"" }},

    { &hf_mp_frag_last,
    { "Last fragment",		"mp.last",	FT_BOOLEAN, BASE_NONE, NULL, 0x0,
    	"" }},

    { &hf_mp_sequence_num,
    { "Sequence number",	"mp.seq",	FT_UINT24, BASE_DEC, NULL, 0x0,
    	"" }}
  };
  static gint *ett[] = {
    &ett_mp,
    &ett_mp_flags,
  };

  proto_mp = proto_register_protocol("PPP Multilink Protocol", "mp");
  proto_register_field_array(proto_mp, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));
}
