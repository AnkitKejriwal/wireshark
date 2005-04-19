/* packet-isl.c
 * Routines for Cisco ISL Ethernet header disassembly
 *
 * $Id$
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
#include "packet-isl.h"
#include "packet-eth.h"
#include "packet-tr.h"
#include "packet-frame.h"
#include "etypes.h"

/*
 * See
 *
 *	http://www.cisco.com/warp/public/473/741_4.html
 *
 * and
 *
 *	http://www.cisco.com/univercd/cc/td/doc/product/lan/trsrb/frames.htm
 *
 * and
 *
 *	http://www.cisco.com/en/US/tech/tk389/tk390/technologies_tech_note09186a0080094665.shtml
 *
 * for information on ISL.
 */
static int proto_isl = -1;
static int hf_isl_dst = -1;
static int hf_isl_type = -1;
static int hf_isl_user_eth = -1;
static int hf_isl_user = -1;
static int hf_isl_src = -1;
static int hf_isl_addr = -1;
static int hf_isl_len = -1;
static int hf_isl_hsa = -1;
static int hf_isl_vlan_id = -1;
static int hf_isl_bpdu = -1;
static int hf_isl_index = -1;
static int hf_isl_crc = -1;
static int hf_isl_src_vlan_id = -1;
static int hf_isl_explorer = -1;
static int hf_isl_dst_route_descriptor = -1;
static int hf_isl_src_route_descriptor = -1;
static int hf_isl_fcs_not_incl = -1;
static int hf_isl_esize = -1;
static int hf_isl_trailer = -1;

static gint ett_isl = -1;

#define	ISL_HEADER_SIZE	26

#define	TYPE_ETHER	0x0
#define	TYPE_TR		0x1
#define	TYPE_FDDI	0x2
#define	TYPE_ATM	0x3

static dissector_handle_t eth_withfcs_handle;
static dissector_handle_t tr_handle;
static dissector_handle_t data_handle;

void
capture_isl(const guchar *pd, int offset, int len, packet_counts *ld)
{
  guint8 type;

  if (!BYTES_ARE_IN_FRAME(offset, len, ISL_HEADER_SIZE)) {
    ld->other++;
    return;
  }

  type = (pd[offset+5] >> 4)&0x0F;

  switch (type) {

  case TYPE_ETHER:
    offset += 14+12;	/* skip the header */
    capture_eth(pd, offset, len, ld);
    break;

  case TYPE_TR:
    offset += 14+17;	/* skip the header */
    capture_tr(pd, offset, len, ld);
    break;

  default:
    ld->other++;
    break;
  }
}

static const value_string type_vals[] = {
	{TYPE_ETHER, "Ethernet"},
	{TYPE_TR,    "Token-Ring"},
	{TYPE_FDDI,  "FDDI"},
	{TYPE_ATM,   "ATM"},
	{0,          NULL}
};

static const value_string ether_user_vals[] = {
	/* User values are defined by IEEE 802.1D-2004, annex G */
	{0x0, "Best effort (default priority)"},
	{0x1, "Background"},
	{0x2, "[spare priority]"},
	{0x3, "Excellent effort"},
	{0x4, "Controlled load"},
	{0x5, "\"Video\", < 100ms latency and jitter"},
	{0x6, "\"Voice\", < 10ms latency and jitter"},
	{0x7, "Network control"},
	{0,   NULL}
};

static const true_false_string bpdu_tfs = {
	"Yes",
	"No"
};

static const true_false_string explorer_tfs = {
	"Explorer frame",
	"Data frame"
};

void
dissect_isl(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int fcs_len)
{
  proto_tree *volatile fh_tree = NULL;
  proto_item *ti;
  guint8 type;
  volatile guint16 length;
  gint captured_length;
  tvbuff_t *volatile payload_tvb;
  tvbuff_t *volatile next_tvb;
  tvbuff_t *volatile trailer_tvb;
  const char *saved_proto;

  if (check_col(pinfo->cinfo, COL_PROTOCOL))
    col_set_str(pinfo->cinfo, COL_PROTOCOL, "ISL");
  if (check_col(pinfo->cinfo, COL_INFO))
    col_clear(pinfo->cinfo, COL_INFO);

  type = (tvb_get_guint8(tvb, 5) >> 4)&0x0F;

  if (tree) {
    ti = proto_tree_add_protocol_format(tree, proto_isl, tvb, 0, ISL_HEADER_SIZE,
		"ISL");
    fh_tree = proto_item_add_subtree(ti, ett_isl);
    proto_tree_add_item(fh_tree, hf_isl_dst, tvb, 0, 5, FALSE);
    proto_tree_add_item_hidden(fh_tree, hf_isl_addr, tvb, 0, 6, FALSE);
    proto_tree_add_item(fh_tree, hf_isl_type, tvb, 5, 1, FALSE);
    switch (type) {

    case TYPE_ETHER:
      proto_tree_add_item(fh_tree, hf_isl_user_eth, tvb, 5, 1, FALSE);
      break;

    default:
      /* XXX - the spec appears to indicate that the "User" field is
         used for TYPE_TR to distinguish between types of packets. */
      proto_tree_add_item(fh_tree, hf_isl_user, tvb, 5, 1, FALSE);
      break;
    }
    proto_tree_add_item(fh_tree, hf_isl_src, tvb, 6, 6, FALSE);
    proto_tree_add_item_hidden(fh_tree, hf_isl_addr, tvb, 6, 6, FALSE);
  }
  length = tvb_get_ntohs(tvb, 12);
  if (tree)
    proto_tree_add_uint(fh_tree, hf_isl_len, tvb, 12, 2, length);

  if (length != 0) {
    /* The length field was set; it's like an 802.3 length field, so
       treat it similarly, by constructing a tvbuff containing only
       the data specified by the length field. */

    TRY {
      payload_tvb = tvb_new_subset(tvb, 14, length, length);
      trailer_tvb = tvb_new_subset(tvb, 14 + length, -1, -1);
    }
    CATCH2(BoundsError, ReportedBoundsError) {
      /* Either:

	  the packet doesn't have "length" bytes worth of
	  captured data left in it - or it may not even have
	  "length" bytes worth of data in it, period -
	  so the "tvb_new_subset()" creating "payload_tvb"
	  threw an exception

         or

	  the packet has exactly "length" bytes worth of
	  captured data left in it, so the "tvb_new_subset()"
	  creating "trailer_tvb" threw an exception.

         In either case, this means that all the data in the frame
         is within the length value, so we give all the data to the
         next protocol and have no trailer. */
      payload_tvb = tvb_new_subset(tvb, 14, -1, length);
      trailer_tvb = NULL;
    }
    ENDTRY;
  } else {
    /* The length field is 0; make it the length remaining in the packet
       after the first 14 bytes. */
    length = tvb_reported_length_remaining(tvb, 14);
    payload_tvb = tvb_new_subset(tvb, 14, -1, -1);
    trailer_tvb = NULL;
  }

  if (tree) {
    /* This part looks sort of like a SNAP-encapsulated LLC header... */
    proto_tree_add_text(fh_tree, payload_tvb, 0, 1, "DSAP: 0x%X", tvb_get_guint8(tvb, 14));
    proto_tree_add_text(fh_tree, payload_tvb, 1, 1, "SSAP: 0x%X", tvb_get_guint8(tvb, 15));
    proto_tree_add_text(fh_tree, payload_tvb, 2, 1, "Control: 0x%X", tvb_get_guint8(tvb, 16));

    /* ...but this is the manufacturer's ID portion of the source address
       field (which is, admittedly, an OUI). */
    proto_tree_add_item(fh_tree, hf_isl_hsa, payload_tvb, 3, 3, FALSE);
  }
  if (check_col(pinfo->cinfo, COL_INFO))
    col_add_fstr(pinfo->cinfo, COL_INFO, "VLAN ID: 0x%04X",
		 tvb_get_ntohs(tvb, 20) >> 1);
  if (tree) {
    proto_tree_add_item(fh_tree, hf_isl_vlan_id, payload_tvb, 6, 2, FALSE);
    proto_tree_add_item(fh_tree, hf_isl_bpdu, payload_tvb, 6, 2, FALSE);
    proto_tree_add_item(fh_tree, hf_isl_index, payload_tvb, 8, 2, FALSE);
  }

  switch (type) {

  case TYPE_ETHER:
    /* The length of the encapsulated frame is the length from the
       header, minus 12 bytes for the part of the ISL header that
       follows the length. */
    if (length >= 12) {
      /* Well, we at least had that much data in the frame.  Try
         dissecting what's left as an Ethernet frame. */
      length -= 12;

      /* Trim the captured length. */
      captured_length = tvb_length_remaining(payload_tvb, 12);

      /* Make sure it's not bigger than the actual length. */
      if (captured_length > length)
        captured_length = length;

      next_tvb = tvb_new_subset(payload_tvb, 12, captured_length, length);

      /* Dissect the payload as an Etherner frame.

        Catch BoundsError and ReportedBoundsError, so that if the
        reported length of "next_tvb" was reduced by some dissector
        before an exception was thrown, we can still put in an item
        for the trailer. */
      saved_proto = pinfo->current_proto;
      TRY {
        /* Frames encapsulated in ISL include an FCS. */
        call_dissector(eth_withfcs_handle, next_tvb, pinfo, tree);
      }
      CATCH(BoundsError) {
       /* Somebody threw BoundsError, which means that dissecting the payload
          found that the packet was cut off by a snapshot length before the
          end of the payload.  The trailer comes after the payload, so *all*
          of the trailer is cut off - don't bother adding the trailer, just
          rethrow the exception so it gets reported. */
       RETHROW;
      }
      CATCH_ALL {
        /* Well, somebody threw an exception other than BoundsError.
           Show the exception, and then drive on to show the trailer,
           restoring the protocol value that was in effect before we
           called the subdissector. */
        show_exception(next_tvb, pinfo, tree, EXCEPT_CODE, GET_MESSAGE);
        pinfo->current_proto = saved_proto;
      }
      ENDTRY;

      /* Now add the Ethernet trailer and FCS.
         XXX - do this only if we're encapsulated in Ethernet? */
      add_ethernet_trailer(fh_tree, hf_isl_trailer, tvb, trailer_tvb, fcs_len);
    }
    break;

  case TYPE_TR:
    if (tree) {
      proto_tree_add_item(fh_tree, hf_isl_src_vlan_id, payload_tvb, 10, 2, FALSE);
      proto_tree_add_item(fh_tree, hf_isl_explorer, payload_tvb, 10, 2, FALSE);
      proto_tree_add_item(fh_tree, hf_isl_dst_route_descriptor, payload_tvb, 12, 2, FALSE);
      proto_tree_add_item(fh_tree, hf_isl_src_route_descriptor, payload_tvb, 14, 2, FALSE);
      /* This doesn't appear to be present in at least one capture I've seen. */
      proto_tree_add_item(fh_tree, hf_isl_fcs_not_incl, payload_tvb, 16, 1, FALSE);
      proto_tree_add_item(fh_tree, hf_isl_esize, payload_tvb, 16, 1, FALSE);
    }
    next_tvb = tvb_new_subset(payload_tvb, 17, -1, -1);
    call_dissector(tr_handle, next_tvb, pinfo, tree);
    break;

  default:
    next_tvb = tvb_new_subset(payload_tvb, 12, -1, -1);
    call_dissector(data_handle, next_tvb, pinfo, tree);
    break;
  }
}

void
proto_register_isl(void)
{
  static hf_register_info hf[] = {
	{ &hf_isl_dst,
	{ "Destination",	"isl.dst", FT_BYTES, BASE_HEX, NULL, 0x0,
		"Destination Address", HFILL }},
	{ &hf_isl_type,
	{ "Type",		"isl.type", FT_UINT8, BASE_DEC,
		VALS(type_vals), 0xF0, "Type", HFILL }},
	{ &hf_isl_user_eth,
	{ "User",		"isl.user_eth", FT_UINT8, BASE_DEC,
		VALS(ether_user_vals), 0x0F, "Priority (for Ethernet)", HFILL }},
	{ &hf_isl_user,
	{ "User",		"isl.user", FT_UINT8, BASE_HEX, NULL, 0x0F,
		"User-defined bits", HFILL }},
	{ &hf_isl_src,
	{ "Source",		"isl.src", FT_ETHER, BASE_NONE, NULL, 0x0,
		"Source Hardware Address", HFILL }},
	{ &hf_isl_addr,
	{ "Source or Destination Address", "isl.addr", FT_ETHER, BASE_NONE, NULL, 0x0,
		"Source or Destination Hardware Address", HFILL }},
	{ &hf_isl_len,
	{ "Length",		"isl.len", FT_UINT16, BASE_DEC, NULL, 0x0,
		"", HFILL }},
	{ &hf_isl_hsa,
	{ "HSA",		"isl.hsa", FT_UINT24, BASE_HEX, NULL, 0x0,
		"High bits of source address", HFILL }},
	{ &hf_isl_vlan_id,
	{ "VLAN ID",		"isl.vlan_id", FT_UINT16, BASE_HEX, NULL,
		0xFFFE, "Virtual LAN ID", HFILL }},
	{ &hf_isl_bpdu,
	{ "BPDU",		"isl.bpdu", FT_BOOLEAN, 16,
		TFS(&bpdu_tfs), 0x0001, "BPDU indicator", HFILL }},
	{ &hf_isl_index,
	{ "Index",		"isl.index", FT_UINT16, BASE_DEC, NULL, 0x0,
		"Port index of packet source", HFILL }},
	{ &hf_isl_crc,
	{ "CRC",		"isl.crc", FT_UINT32, BASE_HEX, NULL, 0x0,
		"CRC field of encapsulated frame", HFILL }},
	{ &hf_isl_src_vlan_id,
	{ "Source VLAN ID",	"isl.src_vlan_id", FT_UINT16, BASE_HEX, NULL,
		0xFFFE, "Source Virtual LAN ID", HFILL }},
	{ &hf_isl_explorer,
	{ "Explorer",		"isl.explorer", FT_BOOLEAN, 16,
		TFS(&explorer_tfs), 0x0001, "Explorer", HFILL }},
	{ &hf_isl_dst_route_descriptor,
	{ "Destination route descriptor",	"isl.dst_route_desc",
		FT_UINT16, BASE_HEX, NULL, 0x0,
		"Route descriptor to be used for forwarding", HFILL }},
	{ &hf_isl_src_route_descriptor,
	{ "Source-route descriptor",	"isl.src_route_desc",
		FT_UINT16, BASE_HEX, NULL, 0x0,
		"Route descriptor to be used for source learning", HFILL }},
	{ &hf_isl_fcs_not_incl,
	{ "FCS Not Included",	"isl.fcs_not_incl", FT_BOOLEAN, 9,
		NULL, 0x40, "FCS not included", HFILL }},
	{ &hf_isl_esize,
	{ "Esize",	"isl.esize", FT_UINT8, BASE_DEC, NULL,
		0x3F, "Frame size for frames less than 64 bytes", HFILL }},
	{ &hf_isl_trailer,
	{ "Trailer",	"isl.trailer", FT_BYTES, BASE_NONE, NULL, 0x0,
		"Ethernet Trailer or Checksum", HFILL }},

  };
  static gint *ett[] = {
	&ett_isl,
  };

  proto_isl = proto_register_protocol("Cisco ISL", "ISL", "isl");
  proto_register_field_array(proto_isl, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));
}

void
proto_reg_handoff_isl(void)
{
  /*
   * Get handles for the Ethernet and Token Ring dissectors.
   */
  eth_withfcs_handle = find_dissector("eth_withfcs");
  tr_handle = find_dissector("tr");
  data_handle = find_dissector("data");
}
