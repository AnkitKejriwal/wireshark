/* ethertype.c
 * Routines for calling the right protocol for the ethertype.
 *
 * $Id: packet-ethertype.c,v 1.42 2004/01/28 03:36:37 gerald Exp $
 *
 * Gilbert Ramirez <gram@alumni.rice.edu>
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

#include <glib.h>
#include <epan/packet.h>
#include "packet-eth.h"
#include "packet-ip.h"
#include "packet-ipv6.h"
#include "packet-ipx.h"
#include "packet-vlan.h"
#include "packet-vines.h"
#include "etypes.h"
#include "ppptypes.h"

static dissector_table_t ethertype_dissector_table;

static dissector_handle_t data_handle;

const value_string etype_vals[] = {
    {ETHERTYPE_IP,		"IP"				},
    {ETHERTYPE_IPv6,		"IPv6"				},
    {ETHERTYPE_X25L3,		"X.25 Layer 3"			},
    {ETHERTYPE_ARP,		"ARP"				},
    {ETHERTYPE_REVARP,		"RARP"				},
    {ETHERTYPE_DEC_LB,		"DEC LanBridge"			},
    {ETHERTYPE_ATALK,		"Appletalk"			},
    {ETHERTYPE_SNA,		"SNA-over-Ethernet"		},
    {ETHERTYPE_AARP,		"AARP"				},
    {ETHERTYPE_IPX,		"Netware IPX/SPX"		},
    {ETHERTYPE_VINES_IP,	"Vines IP"			},
    {ETHERTYPE_VINES_ECHO,	"Vines Echo"			},
    {ETHERTYPE_TRAIN,		"Netmon Train"			},
    {ETHERTYPE_LOOP,		"Loopback"			}, /* Ethernet Loopback */
    {ETHERTYPE_WCP,		"Wellfleet Compression Protocol" },
    {ETHERTYPE_ISMP,		"Cabletron Interswitch Message Protocol" },
    {ETHERTYPE_ISMP_TBFLOOD,	"Cabletron SFVLAN 1.8 Tag-Based Flood" },
    				/* for ISMP, see RFC 2641, RFC 2642, RFC 2643 */
    {ETHERTYPE_PPPOED,		"PPPoE Discovery"		},
    {ETHERTYPE_PPPOES,		"PPPoE Session"			},
    {ETHERTYPE_INTEL_ANS,	"Intel ANS probe"		},
    {ETHERTYPE_MS_NLB_HEARTBEAT,	"MS NLB heartbeat"	},
    {ETHERTYPE_VLAN,		"802.1Q Virtual LAN"		},
    {ETHERTYPE_EAPOL,		"802.1X Authentication"         },
    {ETHERTYPE_MPLS,		"MPLS label switched packet"	},
    {ETHERTYPE_MPLS_MULTI,	"MPLS multicast label switched packet" },
    {ETHERTYPE_3C_NBP_DGRAM,	"3Com NBP Datagram"		},
    {ETHERTYPE_DEC,		"DEC proto"			},
    {ETHERTYPE_DNA_DL,		"DEC DNA Dump/Load"		},
    {ETHERTYPE_DNA_RC,		"DEC DNA Remote Console"	},
    {ETHERTYPE_DNA_RT,		"DEC DNA Routing"		},
    {ETHERTYPE_LAT,		"DEC LAT"			},
    {ETHERTYPE_DEC_DIAG,	"DEC Diagnostics"		},
    {ETHERTYPE_DEC_CUST,	"DEC Customer use"		},
    {ETHERTYPE_DEC_SCA,		"DEC LAVC/SCA"			},
    {ETHERTYPE_ETHBRIDGE,	"Transparent Ethernet bridging" },
    {ETHERTYPE_CGMP,		"Cisco Group Management Protocol" },
    {ETHERTYPE_SLOW_PROTOCOLS,	"Slow Protocols"		},
    {ETHERTYPE_RTNET,		"RTNET Protocol"                },
    {ETHERTYPE_RTCFG,		"RTCFG Protocol"                },
    {ETHERTYPE_PROFINET,	"PROFInet"                },

    /*
     * NDISWAN on Windows translates Ethernet frames from higher-level
     * protocols into PPP frames to hand to the PPP driver, and translates
     * PPP frames from the PPP driver to hand to the higher-level protocols.
     *
     * Apparently the PPP driver, on at least some versions of Windows,
     * passes frames for internal-to-PPP protocols up through NDISWAN;
     * the protocol type field appears to be passed through unchanged
     * (unlike what's done with, for example, the protocol type field
     * for IP, which is mapped from its PPP value to its Ethernet value).
     *
     * This means that we may see, on Ethernet captures, frames for
     * protocols internal to PPP, so we list as "Ethernet" protocol
     * types the PPP protocol types we've seen.
     */
    {PPP_IPCP,			"PPP IP Control Protocol" },
    {PPP_LCP,			"PPP Link Control Protocol" },
    {PPP_PAP,			"PPP Password Authentication Protocol" },
    {PPP_CCP,			"PPP Compression Control Protocol" },
    {0,				NULL				} };

static void add_dix_trailer(proto_tree *fh_tree, int trailer_id, tvbuff_t *tvb,
    tvbuff_t *next_tvb, int offset_after_etype, guint length_before,
    gint fcs_len);

void
capture_ethertype(guint16 etype, const guchar *pd, int offset, int len,
		  packet_counts *ld)
{
  switch (etype) {
    case ETHERTYPE_ARP:
      ld->arp++;
      break;
    case ETHERTYPE_IP:
      capture_ip(pd, offset, len, ld);
      break;
    case ETHERTYPE_IPv6:
      capture_ipv6(pd, offset, len, ld);
      break;
    case ETHERTYPE_IPX:
      capture_ipx(ld);
      break;
    case ETHERTYPE_VLAN:
      capture_vlan(pd, offset, len, ld);
      break;
    case ETHERTYPE_VINES_IP:
    case ETHERTYPE_VINES_ECHO:
      capture_vines(ld);
      break;
    default:
      ld->other++;
      break;
  }
}

void
ethertype(guint16 etype, tvbuff_t *tvb, int offset_after_etype,
		packet_info *pinfo, proto_tree *tree, proto_tree *fh_tree,
		int etype_id, int trailer_id, int fcs_len)
{
	char			*description;
	tvbuff_t		*next_tvb;
	guint			length_before;
	volatile gboolean	dissector_found;
	const char		*saved_proto, *exception_proto;

	/* Add the Ethernet type to the protocol tree */
	if (tree) {
		proto_tree_add_uint(fh_tree, etype_id, tvb,
		    offset_after_etype - 2, 2, etype);
	}

	/* Tvbuff for the payload after the Ethernet type. */
	next_tvb = tvb_new_subset(tvb, offset_after_etype, -1, -1);

	pinfo->ethertype = etype;

	/* Remember how much data there is in it. */
	length_before = tvb_reported_length(next_tvb);

	/* Look for sub-dissector, and call it if found.
	   Catch BoundsError and ReportedBoundsError, so that if the
	   reported length of "next_tvb" was reduced by some dissector
	   before an exception was thrown, we can still put in an item
	   for the trailer. */
	saved_proto = pinfo->current_proto;
	TRY {
		dissector_found = dissector_try_port(ethertype_dissector_table,
		    etype, next_tvb, pinfo, tree);
	}
	CATCH2(BoundsError, ReportedBoundsError) {
		/* Well, somebody threw an exception; that means that a
		   dissector was found, so we don't need to dissect
		   the payload as data or update the protocol or info
		   columns. */
		dissector_found = TRUE;

		/* Save the protocol for which the exception was
		   thrown, and restore pinfo->current_proto to the
		   one for us, so if we throw an exception adding
		   the trailer (e.g., if we get an FCS when capturing,
		   but the snapshot length cut off the FCS), the
		   exception will reflect the appropriate protocol. */
		exception_proto = pinfo->current_proto;
		pinfo->current_proto = saved_proto;

		/* Add the trailer, if appropriate. */
		add_dix_trailer(fh_tree, trailer_id, tvb, next_tvb,
		    offset_after_etype, length_before, fcs_len);

		/* Put back the protocol for which the exception is
		   thrown, and rethrow the exception, so the
		   appropriate indication for the exception can be
		   put into the tree. */
		pinfo->current_proto = exception_proto;
		RETHROW;

		/* XXX - RETHROW shouldn't return. */
		g_assert_not_reached();
	}
	ENDTRY;

	if (!dissector_found) {
		/* No sub-dissector found.
		   Label rest of packet as "Data" */
		call_dissector(data_handle,next_tvb, pinfo, tree);

		/* Label protocol */
		switch (etype) {

		case ETHERTYPE_LOOP:
			if (check_col(pinfo->cinfo, COL_PROTOCOL)) {
				col_add_fstr(pinfo->cinfo, COL_PROTOCOL, "LOOP");
			}
			break;

		default:
			if (check_col(pinfo->cinfo, COL_PROTOCOL)) {
				col_add_fstr(pinfo->cinfo, COL_PROTOCOL, "0x%04x",
				    etype);
			}
			break;
		}
		if (check_col(pinfo->cinfo, COL_INFO)) {
			description = match_strval(etype, etype_vals);
			if (description) {
				col_add_fstr(pinfo->cinfo, COL_INFO, "%s",
				    description);
			}
		}
	}

	add_dix_trailer(fh_tree, trailer_id, tvb, next_tvb, offset_after_etype,
	    length_before, fcs_len);
}

static void
add_dix_trailer(proto_tree *fh_tree, int trailer_id, tvbuff_t *tvb,
    tvbuff_t *next_tvb, int offset_after_etype, guint length_before,
    gint fcs_len)
{
	guint		length;
	tvbuff_t	*volatile trailer_tvb;

	if (fh_tree == NULL)
		return;	/* we're not building a protocol tree */

	if (trailer_id == -1)
		return;	/* our caller doesn't care about trailers */

	/* OK, how much is there in that tvbuff now? */
	length = tvb_reported_length(next_tvb);

	/* If there's less than there was before, what's left is
	   a trailer. */
	if (length < length_before) {
		/*
		 * Create a tvbuff for the padding.
		 */
		TRY {
			trailer_tvb = tvb_new_subset(tvb,
			    offset_after_etype + length, -1, -1);
		}
		CATCH2(BoundsError, ReportedBoundsError) {
			/* The packet doesn't have "length" bytes worth of
			   captured data left in it.  No trailer to display. */
			trailer_tvb = NULL;
			add_ethernet_trailer(fh_tree, trailer_id, tvb, trailer_tvb, fcs_len);
		}
		ENDTRY;
	} else
		trailer_tvb = NULL;	/* no trailer */

}

void
proto_register_ethertype(void)
{
	/* subdissector code */
	ethertype_dissector_table = register_dissector_table("ethertype",
	    "Ethertype", FT_UINT16, BASE_HEX);
}

void
proto_reg_handoff_ethertype(void)
{
	data_handle = find_dissector("data");
}
