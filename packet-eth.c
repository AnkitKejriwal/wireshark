/* packet-eth.c
 * Routines for ethernet packet disassembly
 *
 * $Id: packet-eth.c,v 1.78 2003/01/22 01:18:03 sahlberg Exp $
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
#include "prefs.h"
#include "etypes.h"
#include <epan/resolv.h>
#include "packet-eth.h"
#include "packet-ieee8023.h"
#include "packet-ipx.h"
#include "packet-isl.h"
#include "packet-llc.h"
#include "tap.h"

/* Interpret capture file as FW1 monitor file */
static gboolean eth_interpret_as_fw1_monitor = FALSE;

/* protocols and header fields */
static int proto_eth = -1;
static int hf_eth_dst = -1;
static int hf_eth_src = -1;
static int hf_eth_len = -1;
static int hf_eth_type = -1;
static int hf_eth_addr = -1;
static int hf_eth_trailer = -1;

static gint ett_ieee8023 = -1;
static gint ett_ether2 = -1;

static dissector_handle_t isl_handle;
static dissector_handle_t fw1_handle;

static int eth_tap = -1;

#define ETH_HEADER_SIZE	14

/* These are the Netware-ish names for the different Ethernet frame types.
	EthernetII: The ethernet with a Type field instead of a length field
	Ethernet802.2: An 802.3 header followed by an 802.2 header
	Ethernet802.3: A raw 802.3 packet. IPX/SPX can be the only payload.
			There's no 802.2 hdr in this.
	EthernetSNAP: Basically 802.2, just with 802.2SNAP. For our purposes,
		there's no difference between 802.2 and 802.2SNAP, since we just
		pass it down to the LLC dissector. -- Gilbert
*/
#define ETHERNET_II 	0
#define ETHERNET_802_2	1
#define ETHERNET_802_3	2
#define ETHERNET_SNAP	3

void
capture_eth(const guchar *pd, int offset, int len, packet_counts *ld)
{
  guint16    etype, length;
  int     ethhdr_type;	/* the type of ethernet frame */

  if (!BYTES_ARE_IN_FRAME(offset, len, ETH_HEADER_SIZE)) {
    ld->other++;
    return;
  }

  etype = pntohs(&pd[offset+12]);

	/* either ethernet802.3 or ethernet802.2 */
  if (etype <= IEEE_802_3_MAX_LEN) {
    length = etype;

    /* Is there an 802.2 layer? I can tell by looking at the first 2
       bytes after the 802.3 header. If they are 0xffff, then what
       follows the 802.3 header is an IPX payload, meaning no 802.2.
       (IPX/SPX is they only thing that can be contained inside a
       straight 802.3 packet). A non-0xffff value means that there's an
       802.2 layer inside the 802.3 layer */
    if (pd[offset+14] == 0xff && pd[offset+15] == 0xff) {
      ethhdr_type = ETHERNET_802_3;
    }
    else {
      ethhdr_type = ETHERNET_802_2;
    }

    /* Oh, yuck.  Cisco ISL frames require special interpretation of the
       destination address field; fortunately, they can be recognized by
       checking the first 5 octets of the destination address, which are
       01-00-0C-00-00 for ISL frames. */
    if (pd[offset] == 0x01 && pd[offset+1] == 0x00 && pd[offset+2] == 0x0C
	&& pd[offset+3] == 0x00 && pd[offset+4] == 0x00) {
      capture_isl(pd, offset, len, ld);
      return;
    }

    /* Convert the LLC length from the 802.3 header to a total
       frame length, by adding in the size of any data that preceded
       the Ethernet header, and adding in the Ethernet header size,
       and set the payload and captured-payload lengths to the minima
       of the total length and the frame lengths. */
    length += offset + ETH_HEADER_SIZE;
    if (len > length)
      len = length;
  } else {
    ethhdr_type = ETHERNET_II;
  }
  offset += ETH_HEADER_SIZE;

  switch (ethhdr_type) {
    case ETHERNET_802_3:
      capture_ipx(ld);
      break;
    case ETHERNET_802_2:
      capture_llc(pd, offset, len, ld);
      break;
    case ETHERNET_II:
      capture_ethertype(etype, pd, offset, len, ld);
      break;
  }
}

static void
dissect_eth(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
  proto_item		*ti;
  static eth_hdr 	ehdr;
  volatile gboolean	is_802_2;
  proto_tree		*volatile fh_tree = NULL;

  if (check_col(pinfo->cinfo, COL_PROTOCOL))
    col_set_str(pinfo->cinfo, COL_PROTOCOL, "Ethernet");

  tvb_memcpy(tvb, ehdr.src, 6, 6);
  tvb_memcpy(tvb, ehdr.dst, 0, 6);
  SET_ADDRESS(&pinfo->dl_src,	AT_ETHER, 6, ehdr.src);
  SET_ADDRESS(&pinfo->src,	AT_ETHER, 6, ehdr.src);
  SET_ADDRESS(&pinfo->dl_dst,	AT_ETHER, 6, ehdr.dst);
  SET_ADDRESS(&pinfo->dst,	AT_ETHER, 6, ehdr.dst);

  ehdr.type = tvb_get_ntohs(tvb, 12);

	/* either ethernet802.3 or ethernet802.2 */
  if (ehdr.type <= IEEE_802_3_MAX_LEN) {
    /* Oh, yuck.  Cisco ISL frames require special interpretation of the
       destination address field; fortunately, they can be recognized by
       checking the first 5 octets of the destination address, which are
       01-00-0C-00-00 for ISL frames. */
    if (	tvb_get_guint8(tvb, 0) == 0x01 &&
		tvb_get_guint8(tvb, 1) == 0x00 &&
		tvb_get_guint8(tvb, 2) == 0x0C &&
		tvb_get_guint8(tvb, 3) == 0x00 &&
		tvb_get_guint8(tvb, 4) == 0x00 ) {
      call_dissector(isl_handle, tvb, pinfo, tree);
      goto end_of_eth;
    }

    /* Is there an 802.2 layer? I can tell by looking at the first 2
       bytes after the 802.3 header. If they are 0xffff, then what
       follows the 802.3 header is an IPX payload, meaning no 802.2.
       (IPX/SPX is they only thing that can be contained inside a
       straight 802.3 packet). A non-0xffff value means that there's an
       802.2 layer inside the 802.3 layer */
    is_802_2 = TRUE;
    TRY {
	    if (tvb_get_ntohs(tvb, 14) == 0xffff) {
	      is_802_2 = FALSE;
	    }
    }
    CATCH2(BoundsError, ReportedBoundsError) {
	    ; /* do nothing */

    }
    ENDTRY;

    if (check_col(pinfo->cinfo, COL_INFO)) {
      col_add_fstr(pinfo->cinfo, COL_INFO, "IEEE 802.3 Ethernet %s",
		(is_802_2 ? "" : "Raw "));
    }
    if (tree) {
      ti = proto_tree_add_protocol_format(tree, proto_eth, tvb, 0, ETH_HEADER_SIZE,
		"IEEE 802.3 Ethernet %s", (is_802_2 ? "" : "Raw "));

      fh_tree = proto_item_add_subtree(ti, ett_ieee8023);

      proto_tree_add_ether(fh_tree, hf_eth_dst, tvb, 0, 6, ehdr.dst);
      proto_tree_add_ether(fh_tree, hf_eth_src, tvb, 6, 6, ehdr.src);

/* add items for eth.addr filter */
      proto_tree_add_ether_hidden(fh_tree, hf_eth_addr, tvb, 0, 6, ehdr.dst);
      proto_tree_add_ether_hidden(fh_tree, hf_eth_addr, tvb, 6, 6, ehdr.src);
    }

    dissect_802_3(ehdr.type, is_802_2, tvb, ETH_HEADER_SIZE, pinfo, tree, fh_tree,
		  hf_eth_len, hf_eth_trailer);
  } else {
    if (eth_interpret_as_fw1_monitor) {
      call_dissector(fw1_handle, tvb, pinfo, tree);
      goto end_of_eth;
    }

    if (check_col(pinfo->cinfo, COL_INFO))
      col_set_str(pinfo->cinfo, COL_INFO, "Ethernet II");
    if (tree) {
      ti = proto_tree_add_protocol_format(tree, proto_eth, tvb, 0, ETH_HEADER_SIZE,
		"Ethernet II, Src: %s, Dst: %s",
		ether_to_str(ehdr.src), ether_to_str(ehdr.dst));

      fh_tree = proto_item_add_subtree(ti, ett_ether2);

      proto_tree_add_ether(fh_tree, hf_eth_dst, tvb, 0, 6, ehdr.dst);
      proto_tree_add_ether(fh_tree, hf_eth_src, tvb, 6, 6, ehdr.src);
/* add items for eth.addr filter */
      proto_tree_add_ether_hidden(fh_tree, hf_eth_addr, tvb, 0, 6, ehdr.dst);
      proto_tree_add_ether_hidden(fh_tree, hf_eth_addr, tvb, 6, 6, ehdr.src);
    }

    ethertype(ehdr.type, tvb, ETH_HEADER_SIZE, pinfo, tree, fh_tree, hf_eth_type,
          hf_eth_trailer);
  }

end_of_eth:
  tap_queue_packet(eth_tap, pinfo, &ehdr);
  return;
}

void
proto_register_eth(void)
{
	static hf_register_info hf[] = {

		{ &hf_eth_dst,
		{ "Destination",	"eth.dst", FT_ETHER, BASE_NONE, NULL, 0x0,
			"Destination Hardware Address", HFILL }},

		{ &hf_eth_src,
		{ "Source",		"eth.src", FT_ETHER, BASE_NONE, NULL, 0x0,
			"Source Hardware Address", HFILL }},

		{ &hf_eth_len,
		{ "Length",		"eth.len", FT_UINT16, BASE_DEC, NULL, 0x0,
			"", HFILL }},

		/* registered here but handled in ethertype.c */
		{ &hf_eth_type,
		{ "Type",		"eth.type", FT_UINT16, BASE_HEX, VALS(etype_vals), 0x0,
			"", HFILL }},
		{ &hf_eth_addr,
		{ "Source or Destination Address", "eth.addr", FT_ETHER, BASE_NONE, NULL, 0x0,
			"Source or Destination Hardware Address", HFILL }},

                { &hf_eth_trailer,
		{ "Trailer", "eth.trailer", FT_BYTES, BASE_NONE, NULL, 0x0,
			"Ethernet Trailer or Checksum", HFILL }},

	};
	static gint *ett[] = {
		&ett_ieee8023,
		&ett_ether2,
	};
	module_t *eth_module;

	proto_eth = proto_register_protocol("Ethernet", "Ethernet", "eth");
	proto_register_field_array(proto_eth, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));

	/* Register configuration preferences */
	eth_module = prefs_register_protocol(proto_eth, NULL);
	prefs_register_bool_preference(eth_module, "interpret_as_fw1_monitor",
            "Interpret as FireWall-1 monitor file",
"Whether the capture file should be interpreted as a CheckPoint FireWall-1 monitor file",
            &eth_interpret_as_fw1_monitor);

	register_dissector("eth", dissect_eth, proto_eth);
	eth_tap = register_tap("eth");
}

void
proto_reg_handoff_eth(void)
{
	dissector_handle_t eth_handle;

	/*
	 * Get a handle for the ISL dissector.
	 */
	isl_handle = find_dissector("isl");
	fw1_handle = find_dissector("fw1");

	eth_handle = find_dissector("eth");
	dissector_add("wtap_encap", WTAP_ENCAP_ETHERNET, eth_handle);
	dissector_add("ethertype", ETHERTYPE_ETHBRIDGE, eth_handle);
	dissector_add("chdlctype", ETHERTYPE_ETHBRIDGE, eth_handle);
	dissector_add("gre.proto", ETHERTYPE_ETHBRIDGE, eth_handle);
}
