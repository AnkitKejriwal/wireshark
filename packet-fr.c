/* packet-fr.c
 * Routines for Frame Relay  dissection
 *
 * Copyright 2001, Paul Ionescu	<paul@acorp.ro>
 *
 * $Id: packet-fr.c,v 1.39 2003/09/02 23:37:48 guy Exp $
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
 *
 * References:
 *
 * http://www.protocols.com/pbook/frame.htm
 * http://www.frforum.com/5000/Approved/FRF.3/FRF.3.2.pdf
 * ITU Recommendations Q.922 and Q.933
 * RFC-1490
 * RFC-2427
 * Cisco encapsulation
 * http://www.trillium.com/products/legacy/frame/technical/
 * http://www.trillium.com/assets/legacyframe/white_paper/8771019.pdf
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <string.h>
#include <glib.h>
#include <epan/packet.h>
#include "packet-llc.h"
#include "packet-chdlc.h"
#include "xdlc.h"
#include "etypes.h"
#include "oui.h"
#include "nlpid.h"
#include "greproto.h"
#include <epan/conversation.h>

/*
 * Bits in the address field.
 */
#define FRELAY_EA		0x01	/* Address field extension bit */

#define FRELAY_UPPER_DLCI	0xFC	/* Upper DLCI */
#define FRELAY_CR		0x02	/* Command/response bit in first octet */

#define FRELAY_SECOND_DLCI	0xF0	/* DLCI bits in FECN/BECN/DE octet */
#define FRELAY_FECN		0x08	/* Forward Explicit Congestion Notification */
#define FRELAY_BECN		0x04	/* Backward Explicit Congestion Notification */
#define FRELAY_DE		0x02	/* Discard Eligibility */

#define FRELAY_THIRD_DLCI	0xFE	/* DLCI bits in third octet, if any */

#define FRELAY_LOWER_DLCI	0xFC	/* Lower DLCI */
#define FRELAY_DC		0x02	/* DLCI or DL-CORE control indicator in last octet */

#define FROM_DCE	0x80		/* for direction setting */

static gint proto_fr    = -1;
static gint ett_fr      = -1;
static gint ett_fr_address = -1;
static gint ett_fr_control = -1;
static gint hf_fr_ea    = -1;
static gint hf_fr_upper_dlci = -1;
static gint hf_fr_cr	= -1;
static gint hf_fr_second_dlci = -1;
static gint hf_fr_fecn  = -1;
static gint hf_fr_becn  = -1;
static gint hf_fr_de    = -1;
static gint hf_fr_third_dlci = -1;
static gint hf_fr_dlcore_control = -1;
static gint hf_fr_lower_dlci = -1;
static gint hf_fr_dc    = -1;
static gint hf_fr_dlci  = -1;
static gint hf_fr_control = -1;
static gint hf_fr_nlpid = -1;
static gint hf_fr_oui   = -1;
static gint hf_fr_pid   = -1;
static gint hf_fr_snaptype = -1;
static gint hf_fr_chdlctype = -1;

static dissector_handle_t data_handle;

static dissector_table_t osinl_subdissector_table;

static const true_false_string cmd_string = {
                "Command",
                "Response"
        };
static const true_false_string ctrl_string = {
                "DLCI Address",
                "Control"
        };
static const true_false_string ea_string = {
                "Last Octet",
                "More Follows"
        };

/*
 * This isn't the same as "nlpid_vals[]"; 0x08 is Q.933, not Q.931,
 * and 0x09 is LMI, not Q.2931, and we assume that it's an initial
 * protocol identifier, so 0x01 is T.70, not X.29.
 */
static const value_string fr_nlpid_vals[] = {
	{ NLPID_NULL,            "NULL" },
	{ NLPID_IPI_T_70,        "T.70" },	/* XXX - IPI, or SPI? */
	{ NLPID_X_633,           "X.633" },
	{ NLPID_Q_931,           "Q.933" },
	{ NLPID_LMI,             "LMI" },
	{ NLPID_Q_2119,          "Q.2119" },
	{ NLPID_SNAP,            "SNAP" },
	{ NLPID_ISO8473_CLNP,    "CLNP" },
	{ NLPID_ISO9542_ESIS,    "ESIS" },
	{ NLPID_ISO10589_ISIS,   "ISIS" },
	{ NLPID_ISO10747_IDRP,   "IDRP" },
	{ NLPID_ISO9542X25_ESIS, "ESIS (X.25)" },
	{ NLPID_ISO10030,        "ISO 10030" },
	{ NLPID_ISO11577,        "ISO 11577" },
	{ NLPID_COMPRESSED,      "Data compression protocol" },
	{ NLPID_IP,              "IP" },
	{ NLPID_IP6,             "IPv6" },
	{ NLPID_PPP,             "PPP" },
	{ 0,                     NULL },
};

static dissector_table_t fr_subdissector_table;

static void dissect_fr_nlpid(tvbuff_t *tvb, int offset, packet_info *pinfo,
			     proto_tree *tree, proto_item *ti,
			     proto_tree *fr_tree, guint8 fr_ctrl);
static void dissect_lapf(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);
static void dissect_fr_xid(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);

static void
dissect_fr_common(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
		  gboolean has_direction)
{
  int offset = 0;
  proto_item *ti = NULL;
  proto_tree *fr_tree = NULL;
  proto_item *octet_item = NULL;
  proto_tree *octet_tree = NULL;
  guint8 fr_octet;
  int is_response = FALSE;
  guint32 address;
  guint8  fr_ctrl;
  guint16 fr_type;

  if (check_col(pinfo->cinfo, COL_PROTOCOL))
      col_set_str(pinfo->cinfo, COL_PROTOCOL, "FR");
  if (check_col(pinfo->cinfo, COL_INFO))
      col_clear(pinfo->cinfo, COL_INFO);

  if (has_direction) {
    if (pinfo->pseudo_header->x25.flags & FROM_DCE) {
      if (check_col(pinfo->cinfo, COL_RES_DL_DST))
	col_set_str(pinfo->cinfo, COL_RES_DL_DST, "DTE");
      if (check_col(pinfo->cinfo, COL_RES_DL_SRC))
	col_set_str(pinfo->cinfo, COL_RES_DL_SRC, "DCE");
    } else {
      if (check_col(pinfo->cinfo, COL_RES_DL_DST))
	col_set_str(pinfo->cinfo, COL_RES_DL_DST, "DCE");
      if (check_col(pinfo->cinfo, COL_RES_DL_SRC))
	col_set_str(pinfo->cinfo, COL_RES_DL_SRC, "DTE");
    }
  }

  /*
   * OK, fetch the address field - keep going until we get an EA bit.
   */
  fr_octet = tvb_get_guint8(tvb, offset);
  if (tree) {
      ti = proto_tree_add_protocol_format(tree, proto_fr, tvb, 0, -1, "Frame Relay");
      fr_tree = proto_item_add_subtree(ti, ett_fr);
  }
  if (fr_octet & FRELAY_EA) {
    /*
     * Bogus!  There should be at least 2 octets.
     */
    address = 0;
    if (tree) {
      proto_tree_add_text(fr_tree, tvb, offset, 1,
			  "Bogus 1-octet address field");
      offset++;
    }
  } else {
    /*
     * The first octet contains the upper 6 bits of the DLCI, as well
     * as the C/R bit.
     */
    address = (fr_octet & FRELAY_UPPER_DLCI) >> 2;
    is_response = (fr_octet & FRELAY_CR);
    if (tree) {
      octet_item = proto_tree_add_text(fr_tree, tvb, offset, 1,
				       "First address octet: 0x%02x", fr_octet);
      octet_tree = proto_item_add_subtree(octet_item, ett_fr_address);
      proto_tree_add_uint(octet_tree, hf_fr_upper_dlci, tvb, offset, 1, fr_octet);
      proto_tree_add_boolean(octet_tree, hf_fr_cr, tvb, offset, 1, fr_octet);
      proto_tree_add_boolean(octet_tree, hf_fr_ea, tvb, offset, 1, fr_octet);
    }
    offset++;

    /*
     * The second octet contains 4 more bits of DLCI, as well as FECN,
     * BECN, and DE.
     */
    fr_octet = tvb_get_guint8(tvb, offset);
    address = (address << 4) | ((fr_octet & FRELAY_SECOND_DLCI) >> 4);
    if (tree) {
      octet_item = proto_tree_add_text(fr_tree, tvb, offset, 1,
				       "Second address octet: 0x%02x",
				       fr_octet);
      octet_tree = proto_item_add_subtree(octet_item, ett_fr_address);
      proto_tree_add_uint(octet_tree, hf_fr_second_dlci, tvb, offset, 1, fr_octet);
      proto_tree_add_boolean(octet_tree, hf_fr_fecn, tvb, 0, offset, fr_octet);
      proto_tree_add_boolean(octet_tree, hf_fr_becn, tvb, 0, offset, fr_octet);
      proto_tree_add_boolean(octet_tree, hf_fr_de, tvb, 0, offset, fr_octet);
      proto_tree_add_boolean(octet_tree, hf_fr_ea, tvb, offset, 1, fr_octet);
    }
    offset++;

    if (!(fr_octet & FRELAY_EA)) {
      /*
       * We have 3 or more address octets.
       *
       * The third octet contains 7 more bits of DLCI if EA isn't set,
       * and lower DLCI or DL-CORE control plus the DLCI or DL-CORE
       * control indicator flag if EA is set.
       */
      fr_octet = tvb_get_guint8(tvb, offset);
      if (!(fr_octet & FRELAY_EA)) {
	/*
	 * 7 more bits of DLCI.
	 */
	address = (address << 7) | ((fr_octet & FRELAY_THIRD_DLCI) >> 1);
	if (tree) {
	  octet_item = proto_tree_add_text(fr_tree, tvb, offset, 1,
					   "Third address octet: 0x%02x",
					   fr_octet);
	  octet_tree = proto_item_add_subtree(octet_item, ett_fr_address);
	  proto_tree_add_uint(octet_tree, hf_fr_third_dlci, tvb, offset, 1, fr_octet);
	  proto_tree_add_boolean(octet_tree, hf_fr_ea, tvb, offset, 1, fr_octet);
	}
	offset++;
	fr_octet = tvb_get_guint8(tvb, offset);
	while (!(fr_octet & FRELAY_EA)) {
	  /*
	   * Bogus!  More than 4 octets of address.
	   */
	  if (tree) {
	    proto_tree_add_text(fr_tree, tvb, offset, 1,
				"Bogus extra address octet");
	  }
	  offset++;
	  fr_octet = tvb_get_guint8(tvb, offset);
	}
      }
      if (tree) {
	octet_item = proto_tree_add_text(fr_tree, tvb, offset, 1,
					 "Final address octet: 0x%02x",
					 fr_octet);
	octet_tree = proto_item_add_subtree(octet_item, ett_fr_address);
      } 

      /*
       * Last octet - contains lower DLCI or DL-CORE control, DLCI or
       * DL-CORE control indicator flag.
       */
      if (fr_octet & FRELAY_DC) {
	/*
	 * DL-CORE.
	 */
	proto_tree_add_uint(octet_tree, hf_fr_dlcore_control, tvb, offset, 1, fr_octet);
      } else {
      	/*
      	 * Last 6 bits of DLCI.
      	 */
      	address = (address << 6) | ((fr_octet & FRELAY_LOWER_DLCI) >> 2);
	proto_tree_add_uint(octet_tree, hf_fr_lower_dlci, tvb, offset, 1, fr_octet);
      }
      proto_tree_add_boolean(octet_tree, hf_fr_dc, tvb, offset, 1, fr_octet);
      proto_tree_add_boolean(octet_tree, hf_fr_ea, tvb, offset, 1, fr_octet);
    }
  }
  if (tree) {
    /* Put the full DLCI into the protocol tree. */
    proto_tree_add_uint(fr_tree, hf_fr_dlci, tvb, 0, offset, address);
  }

  pinfo->ctype = CT_DLCI;
  pinfo->circuit_id = address;

  if (check_col(pinfo->cinfo, COL_INFO))
      col_add_fstr(pinfo->cinfo, COL_INFO, "DLCI %u", address);

  fr_ctrl = tvb_get_guint8(tvb, offset);
  if (fr_ctrl == XDLC_U) {
      dissect_xdlc_control(tvb, offset, pinfo, fr_tree, hf_fr_control,
 			   ett_fr_control, is_response, TRUE, TRUE);
      offset++;

      dissect_fr_nlpid(tvb, offset, pinfo, tree, ti, fr_tree, fr_ctrl);
  } else {
      if (address == 0) {
		/* this must be some sort of lapf on DLCI 0 for SVC */
		/* because DLCI 0 is rezerved for LMI and  SVC signaling encaplulated in lapf */
		/* and LMI is transmitted in unnumbered information (03) */
		/* so this must be lapf (guessing) */
		dissect_xdlc_control(tvb, offset, pinfo, fr_tree,
				     hf_fr_control, ett_fr_control,
				     is_response, TRUE, TRUE);
		dissect_lapf(tvb_new_subset(tvb,offset,-1,-1),pinfo,tree);
		return;
      }
      if (fr_ctrl == (XDLC_U|XDLC_XID)) {
		dissect_xdlc_control(tvb, offset, pinfo, fr_tree,
				     hf_fr_control, ett_fr_control,
				     is_response, TRUE, TRUE);
		dissect_fr_xid(tvb_new_subset(tvb,offset,-1,-1),pinfo,tree);
		return;
      }

      /*
       * If the data does not start with unnumbered information (03) and
       * the DLCI# is not 0, then there may be Cisco Frame Relay encapsulation.
       */
      fr_type  = tvb_get_ntohs(tvb, offset);
      if (ti != NULL) {
		/* Include the Cisco HDLC type in the top-level protocol
		   tree item. */
		proto_item_set_end(ti, tvb, offset+2);
      }
      chdlctype(fr_type, tvb, offset+2, pinfo, tree, fr_tree, hf_fr_chdlctype);
  }
}

static void
dissect_fr(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
  dissect_fr_common(tvb, pinfo, tree, FALSE);
}

static void
dissect_fr_phdr(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
  dissect_fr_common(tvb, pinfo, tree, TRUE);
}

static void dissect_fr_uncompressed(tvbuff_t *tvb, packet_info *pinfo,
				    proto_tree *tree)
{
  proto_item *ti = NULL;
  proto_tree *fr_tree = NULL;

  if (check_col(pinfo->cinfo, COL_PROTOCOL))
      col_set_str(pinfo->cinfo, COL_PROTOCOL, "FR");
  if (check_col(pinfo->cinfo, COL_INFO))
      col_clear(pinfo->cinfo, COL_INFO);

  if (tree) {
      ti = proto_tree_add_protocol_format(tree, proto_fr, tvb, 0, -1, "Frame Relay");
      fr_tree = proto_item_add_subtree(ti, ett_fr);
  }
  dissect_fr_nlpid(tvb, 0, pinfo, tree, ti, fr_tree, XDLC_U);
}

static void dissect_fr_nlpid(tvbuff_t *tvb, int offset, packet_info *pinfo,
			     proto_tree *tree, proto_item *ti,
			     proto_tree *fr_tree, guint8 fr_ctrl)
{
  guint8  fr_nlpid;
  tvbuff_t *next_tvb;

  /*
   * Tentatively set the Frame Relay item not to include the NLPID,
   * as OSI network layer protocols consider it to be part of
   * the OSI PDU.
   */
  proto_item_set_end(ti, tvb, offset);
  fr_nlpid = tvb_get_guint8 (tvb,offset);
  if (fr_nlpid == 0) {
	if (tree)
		proto_tree_add_text(fr_tree, tvb, offset, 1, "Padding");
	offset++;
	if (ti != NULL) {
		/* Include the padding in the top-level protocol tree item. */
		proto_item_set_end(ti, tvb, offset);
	}
	fr_nlpid=tvb_get_guint8( tvb,offset);
  }

  /*
   * OSI network layer protocols consider the NLPID to be part
   * of the frame, so we'll pass it as part of the payload and,
   * if the protocol is one of those, add it as a hidden item here.
   */
  next_tvb = tvb_new_subset(tvb,offset,-1,-1);
  if (dissector_try_port(osinl_subdissector_table, fr_nlpid, next_tvb,
			 pinfo, tree)) {
	/*
	 * Yes, we got a match.  Add the NLPID as a hidden item,
	 * so you can, at least, filter on it.
	 */
	if (tree)
		proto_tree_add_uint_hidden(fr_tree, hf_fr_nlpid,
		    tvb, offset, 1, fr_nlpid );
	return;
  }

  /*
   * All other protocols don't.
   *
   * XXX - not true for Q.933 and LMI, but we don't yet have a
   * Q.933 dissector (it'd be similar to the Q.931 dissector,
   * but I don't think it'd be identical, although it's less
   * different than is the Q.2931 dissector), and the LMI
   * dissector doesn't yet put the protocol discriminator
   * (NLPID) into the tree.
   *
   * Note that an NLPID of 0x08 for Q.933 could either be a
   * Q.933 signaling message or a message for a protocol
   * identified by a 2-octet layer 2 protocol type and a
   * 2-octet layer 3 protocol type, those protocol type
   * octets having the values from octets 6, 6a, 7, and 7a
   * of a Q.931 low layer compatibility information element
   * (section 4.5.19 of Q.931; Q.933 says they have the values
   * from a Q.933 low layer compatibility information element,
   * but Q.933 low layer compatibility information elements
   * don't have protocol values in them).
   *
   * Assuming that, as Q.933 seems to imply, that Q.933 messages
   * look just like Q.931 messages except where it explicitly
   * says they differ, then the octet after the NLPID would,
   * in a Q.933 message, have its upper 4 bits zero (that's
   * the length of the call reference value, in Q.931, and
   * is limited to 15 or fewer octets).  As appears to be the case,
   * octet 6 of a Q.931 low layer compatibility element has the
   * 0x40 bit set, so you can distinguish between a Q.933
   * message and an encapsulated packet by checking whether
   * the upper 4 bits of the octet after the NLPID are zero.
   *
   * To handle this, we'd handle Q.933's NLPID specially, which
   * we'd want to do anyway, so that we give it a tvbuff that
   * includes the NLPID.
   */
  if (tree)
	proto_tree_add_uint(fr_tree, hf_fr_nlpid, tvb, offset, 1, fr_nlpid );
  offset++;

  switch (fr_nlpid) {

  case NLPID_SNAP:
	if (ti != NULL) {
		/* Include the NLPID and SNAP header in the top-level
		   protocol tree item. */
		proto_item_set_end(ti, tvb, offset+5);
	}
	dissect_snap(tvb, offset, pinfo, tree, fr_tree, fr_ctrl,
	      hf_fr_oui, hf_fr_snaptype, hf_fr_pid, 0);
	return;

  default:
	if (ti != NULL) {
		/* Include the NLPID in the top-level protocol tree item. */
		proto_item_set_end(ti, tvb, offset);
	}
	next_tvb = tvb_new_subset(tvb,offset,-1,-1);
	if (!dissector_try_port(fr_subdissector_table,fr_nlpid,
				next_tvb, pinfo, tree))
		call_dissector(data_handle,next_tvb, pinfo, tree);
	break;
  }
}

static void dissect_lapf(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	proto_tree_add_text(tree, tvb, 0, 0, "Frame relay lapf not yet implemented");
	call_dissector(data_handle,tvb_new_subset(tvb,0,-1,-1),pinfo,tree);
}
static void dissect_fr_xid(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	proto_tree_add_text(tree, tvb, 0, 0, "Frame relay xid not yet implemented");
	call_dissector(data_handle,tvb_new_subset(tvb,0,-1,-1),pinfo,tree);
}

/* Register the protocol with Ethereal */
void proto_register_fr(void)
{
  static hf_register_info hf[] = {
        { &hf_fr_ea, {
           "EA", "fr.ea", FT_BOOLEAN, 8, TFS(&ea_string),
            FRELAY_EA, "Extended Address", HFILL }},
        { &hf_fr_upper_dlci, {
           "Upper DLCI", "fr.upper_dlci", FT_UINT8, BASE_HEX,
            NULL, FRELAY_UPPER_DLCI, "Upper bits of DLCI", HFILL }},
        { &hf_fr_cr, {
           "CR", "fr.cr", FT_BOOLEAN, 8, TFS(&cmd_string),
            FRELAY_CR, "Command/Response", HFILL }},
        { &hf_fr_second_dlci, {
           "Second DLCI", "fr.second_dlci", FT_UINT8, BASE_HEX,
            NULL, FRELAY_SECOND_DLCI, "Bits below upper bits of DLCI", HFILL }},
        { &hf_fr_fecn, {
           "FECN", "fr.fecn", FT_BOOLEAN, 8,
            NULL, FRELAY_FECN, "Forward Explicit Congestion Notification", HFILL }},
        { &hf_fr_becn, {
           "BECN", "fr.becn", FT_BOOLEAN, 8,
            NULL, FRELAY_BECN, "Backward Explicit Congestion Notification", HFILL }},
        { &hf_fr_de, {
           "DE", "fr.de", FT_BOOLEAN, 8,
            NULL, FRELAY_DE, "Discard Eligibility", HFILL }},
        { &hf_fr_third_dlci, {
           "Third DLCI", "fr.third_dlci", FT_UINT8, BASE_HEX,
            NULL, FRELAY_THIRD_DLCI, "Additional bits of DLCI", HFILL }},
        { &hf_fr_dlcore_control, {
           "DL-CORE Control", "fr.dlcore_control", FT_UINT8, BASE_HEX,
            NULL, FRELAY_LOWER_DLCI, "DL-Core control bits", HFILL }},
        { &hf_fr_lower_dlci, {
           "Lower DLCI", "fr.lower_dlci", FT_UINT8, BASE_HEX,
            NULL, FRELAY_LOWER_DLCI, "Lower bits of DLCI", HFILL }},
        { &hf_fr_dc, {
           "DC", "fr.dc", FT_BOOLEAN, 16, TFS(&ctrl_string),
            FRELAY_CR, "Address/Control", HFILL }},
        { &hf_fr_dlci, {
           "DLCI", "fr.dlci", FT_UINT32, BASE_DEC,
            NULL, 0x0, "Data-Link Connection Identifier", HFILL }},
	{ &hf_fr_control, {
          "Control Field", "fr.control", FT_UINT8, BASE_HEX,
          NULL, 0x0, "Control field", HFILL }},
        { &hf_fr_nlpid, {
           "NLPID", "fr.nlpid", FT_UINT8, BASE_HEX,
            VALS(fr_nlpid_vals), 0x0, "Frame Relay Encapsulated Protocol NLPID", HFILL }},
	{ &hf_fr_oui, {
	   "Organization Code",	"fr.snap.oui", FT_UINT24, BASE_HEX,
	   VALS(oui_vals), 0x0, "", HFILL }},
	{ &hf_fr_pid, {
	   "Protocol ID", "fr.snap.pid", FT_UINT16, BASE_HEX,
	   NULL, 0x0, "", HFILL }},
        { &hf_fr_snaptype, {
           "Type", "fr.snaptype", FT_UINT16, BASE_HEX,
            VALS(etype_vals), 0x0, "Frame Relay SNAP Encapsulated Protocol", HFILL }},
        { &hf_fr_chdlctype, {
           "Type", "fr.chdlctype", FT_UINT16, BASE_HEX,
            VALS(chdlc_vals), 0x0, "Frame Relay Cisco HDLC Encapsulated Protocol", HFILL }},
  };


  /* Setup protocol subtree array */
  static gint *ett[] = {
    &ett_fr,
    &ett_fr_address,
    &ett_fr_control,
  };

  proto_fr = proto_register_protocol("Frame Relay", "FR", "fr");
  proto_register_field_array(proto_fr, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));

  fr_subdissector_table = register_dissector_table("fr.ietf",
	"Frame Relay NLPID", FT_UINT8, BASE_HEX);

  register_dissector("fr_uncompressed", dissect_fr_uncompressed, proto_fr);
  register_dissector("fr", dissect_fr, proto_fr);
}

void proto_reg_handoff_fr(void)
{
  dissector_handle_t fr_handle, fr_phdr_handle;

  fr_handle = create_dissector_handle(dissect_fr, proto_fr);
  dissector_add("gre.proto", GRE_FR, fr_handle);
  dissector_add("wtap_encap", WTAP_ENCAP_FRELAY, fr_handle);

  fr_phdr_handle = create_dissector_handle(dissect_fr_phdr, proto_fr);
  dissector_add("wtap_encap", WTAP_ENCAP_FRELAY_WITH_PHDR, fr_phdr_handle);
  data_handle = find_dissector("data");

  osinl_subdissector_table = find_dissector_table("osinl");
}
