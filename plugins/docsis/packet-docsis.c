/* packet-docsis.c
 * Routines for docsis dissection
 * Copyright 2002, Anand V. Narwani <anarwani@cisco.com>
 *
 * $Id: packet-docsis.c,v 1.10 2002/09/10 19:07:38 guy Exp $
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


/* This code is based on the DOCSIS 1.1 specification available at:
 * http://www.cablemodem.com/specifications.html
 *
 * DOCSIS Captures can be facilitated using the Cable Monitor Feature
 * available on Cisco Cable Modem Termination Systems :
 * http://www.cisco.com/univercd/cc/td/doc/product/cable/cab_rout/cmtsfg/ufg_cmon.htm
 *
 * This dissector depends on the presence of a DOCSIS enapsulation type.
 * There is no simple way to distinguish DOCSIS Frames from Ethernet frames,
 * since the frames are copied from the RF interface on the CMTS to
 * a Fast Ethernet interface; Thus a preference was needed to enable
 * the DOCSIS encapsulation type.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "plugins/plugin_api.h"
#include "plugins/plugin_api_defs.h"
#include "moduleinfo.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gmodule.h>

#include <epan/packet.h>


#ifndef __ETHEREAL_STATIC__
G_MODULE_EXPORT const gchar version[] = VERSION;
#endif


#include "packet-docsis.h"

#define FCTYPE_PACKET 0x00
#define FCTYPE_ATMPDU 0x01
#define FCTYPE_RESRVD 0x02
#define FCTYPE_MACSPC 0x03

#define EH_NULL_CONFIG 0
#define EH_REQUEST 1
#define EH_ACK_REQ 2
#define EH_BP_UP 3
#define EH_BP_DOWN 4
#define EH_SFLOW_HDR_DOWN 5
#define EH_SFLOW_HDR_UP 6
#define EH_RESERVED_7 7
#define EH_RESERVED_8 8
#define EH_RESERVED_9 9
#define EH_RESERVED_10 10
#define EH_RESERVED_11 11
#define EH_RESERVED_12 12
#define EH_RESERVED_13 13
#define EH_RESERVED_14 14
#define EH_EXTENDED 15

/* Initialize the protocol and registered fields */
static int proto_docsis = -1;
static int hf_docsis_fctype = -1;
static int hf_docsis_machdr_fcparm = -1;
static int hf_docsis_fcparm = -1;
static int hf_docsis_ehdron = -1;
static int hf_docsis_concat_cnt = -1;
static int hf_docsis_macparm = -1;
static int hf_docsis_ehdrlen = -1;
static int hf_docsis_lensid = -1;
static int hf_docsis_eh_type = -1;
static int hf_docsis_eh_len = -1;
static int hf_docsis_eh_val = -1;
static int hf_docsis_frag_rsvd = -1;
static int hf_docsis_frag_first = -1;
static int hf_docsis_frag_last = -1;
static int hf_docsis_frag_seq = -1;
static int hf_docsis_sid = -1;
static int hf_docsis_mini_slots = -1;
static int hf_docsis_hcs = -1;
static int hf_docsis_bpi_en = -1;
static int hf_docsis_toggle_bit = -1;
static int hf_docsis_key_seq = -1;
static int hf_docsis_ehdr_ver = -1;
static int hf_docsis_said = -1;
static int hf_docsis_ehdr_phsi = -1;
static int hf_docsis_ehdr_qind = -1;
static int hf_docsis_ehdr_grants = -1;
static int hf_docsis_reserved = -1;

static dissector_handle_t docsis_handle;
static dissector_handle_t eth_handle;
static dissector_handle_t data_handle;
static dissector_handle_t docsis_mgmt_handle;
static dissector_table_t docsis_dissector_table;

/* Initialize the subtree pointers */
static gint ett_docsis = -1;
static gint ett_ehdr = -1;

static const value_string fctype_vals[] = {
  {FCTYPE_PACKET, "Packet PDU"},
  {FCTYPE_ATMPDU, "ATM PDU"},
  {FCTYPE_RESRVD, "Reserved"},
  {FCTYPE_MACSPC, "MAC Specific"},
  {0, NULL}
};

static const value_string eh_type_vals[] = {
  {0, "NULL Configuration Parameter"},
  {EH_REQUEST, "Request"},
  {EH_ACK_REQ, "Acknowledgement Requested"},
  {EH_BP_UP, "Upstream Privacy Element"},
  {EH_BP_DOWN, "Downstream  Privacy Element"},
  {EH_SFLOW_HDR_UP, "Service Flow EH; PHS Header Upstream"},
  {EH_SFLOW_HDR_DOWN, "Service Flow EH; PHS Header Downstream"},
  {EH_RESERVED_7, "Reserved"},
  {EH_RESERVED_8, "Reserved"},
  {EH_RESERVED_9, "Reserved"},
  {EH_RESERVED_10, "Reserved"},
  {EH_RESERVED_10, "Reserved"},
  {EH_RESERVED_11, "Reserved"},
  {EH_RESERVED_12, "Reserved"},
  {EH_RESERVED_13, "Reserved"},
  {EH_RESERVED_14, "Reserved"},
  {EH_EXTENDED, "Extended"},
  {0, NULL}
};

static const value_string fcparm_vals[] = {
  {0x0, "Timing Header"},
  {0x1, "Mac Management Message"},
  {0x2, "Request Frame"},
  {0x3, "Fragmentation Header"},
  {0x1C, "Concatenation Header"},
  {0, NULL}
};
static const value_string ehdron_vals[] = {
  {0x00, "Extended Header Absent"},
  {0x01, "Extended Header Present"},
  {0, NULL}
};

static const true_false_string ena_dis_tfs = {
  "Enabled",
  "Disabled"
};

static const value_string true_false_vals[] = {
  { 0x00 , "False"},
  { 0x01 , "True" },
  { 0x00 , NULL }
};

static const value_string on_off_vals[] = {
  { 0x00 , "On"},
  { 0x01 , "Off" },
  { 0x00 , NULL }
};

static const value_string ena_dis_vals[] = {
  {0, "Disabled"},
  {1, "Enabled"},
  {0, NULL}
};

static const value_string odd_even_vals[] = {
  {1, "Odd Key"},
  {0, "Even Key"},
  {0, NULL}
};
/* Code to actually dissect the packets */
/* Code to Dissect the extended header */
static void
dissect_ehdr (tvbuff_t * tvb, proto_tree * tree, gboolean isfrag)
{
  proto_tree *ehdr_tree;
  proto_item *it;
  guint8 ehdrlen;
  int pos;
  guint8 type;
  guint8 len;
  guint8 val;
  guint8 mini_slots;
  guint16 sid;

  ehdrlen = tvb_get_guint8 (tvb, 1);
  pos = 4;

  it = proto_tree_add_text (tree, tvb, pos, ehdrlen, "Extended Header");
  ehdr_tree = proto_item_add_subtree (it, ett_ehdr);
  while (pos < (int)(ehdrlen + 4))
    {
      type = (tvb_get_guint8 (tvb, pos) & 0xF0);
      len = (tvb_get_guint8 (tvb, pos) & 0x0F);
      if ((((type >> 4) & 0x0F)== 6) && (len == 2)) 
        {
          proto_tree_add_item_hidden (ehdr_tree, hf_docsis_eh_type, tvb, pos, 1, FALSE);
	  proto_tree_add_text(ehdr_tree, tvb, pos, 1, "0110 ....  = Unsolicited Grant Sync EHDR Sub-Element" );
        }
      else
        proto_tree_add_item (ehdr_tree, hf_docsis_eh_type, tvb, pos, 1, FALSE);
      proto_tree_add_item (ehdr_tree, hf_docsis_eh_len, tvb, pos, 1, FALSE);
      switch ((type >> 4) & 0x0F)
	{
	case EH_REQUEST:
	  if (len == 3)
	    {
	      mini_slots = tvb_get_guint8 (tvb, pos + 1);
	      sid = tvb_get_ntohs (tvb, pos + 2);
	      proto_tree_add_uint (ehdr_tree, hf_docsis_mini_slots, tvb,
				   pos + 1, 1, mini_slots);
	      proto_tree_add_uint (ehdr_tree, hf_docsis_sid, tvb, pos + 2, 2,
				   sid);
	    }
	  else
	    {
	      THROW (ReportedBoundsError);
	    }
	  break;
	case EH_ACK_REQ:
	  if (len == 2)
	    {
	      sid = tvb_get_ntohs (tvb, pos + 1);
	      proto_tree_add_uint (ehdr_tree, hf_docsis_sid, tvb, pos + 2, 2,
				   sid);
	    }
	  else
	    {
	      THROW (ReportedBoundsError);
	    }
	case EH_BP_UP:
	  proto_tree_add_item (ehdr_tree, hf_docsis_key_seq, tvb, pos + 1, 1,
			       FALSE);
	  proto_tree_add_item (ehdr_tree, hf_docsis_ehdr_ver, tvb, pos + 1, 1,
			       FALSE);
	  proto_tree_add_item (ehdr_tree, hf_docsis_bpi_en, tvb, pos + 2, 1,
			       FALSE);
	  proto_tree_add_item (ehdr_tree, hf_docsis_toggle_bit, tvb, pos + 2,
			       1, FALSE);
	  proto_tree_add_item (ehdr_tree, hf_docsis_sid, tvb, pos + 2, 2,
			       FALSE);
	  proto_tree_add_item (ehdr_tree, hf_docsis_mini_slots, tvb, pos + 4,
			       1, FALSE);
	  if (isfrag)
	    {
	      proto_tree_add_item (ehdr_tree, hf_docsis_frag_rsvd, tvb, pos+5,
	                          1, FALSE);
	      proto_tree_add_item (ehdr_tree, hf_docsis_frag_first, tvb, pos+5,
			          1, FALSE);
	      proto_tree_add_item (ehdr_tree, hf_docsis_frag_last, tvb, pos+5,
			          1, FALSE);
	      proto_tree_add_item (ehdr_tree, hf_docsis_frag_seq, tvb, pos+5,
			          1, FALSE);
	    }
	  break;
	case EH_BP_DOWN:
	  proto_tree_add_item (ehdr_tree, hf_docsis_key_seq, tvb, pos + 1, 1,
			       FALSE);
	  proto_tree_add_item (ehdr_tree, hf_docsis_ehdr_ver, tvb, pos + 1, 1,
			       FALSE);
	  proto_tree_add_item (ehdr_tree, hf_docsis_bpi_en, tvb, pos + 2, 1,
			       FALSE);
	  proto_tree_add_item (ehdr_tree, hf_docsis_toggle_bit, tvb, pos + 2,
			       1, FALSE);
	  proto_tree_add_item (ehdr_tree, hf_docsis_said, tvb, pos + 2, 2,
			       FALSE);
	  proto_tree_add_item (ehdr_tree, hf_docsis_reserved, tvb, pos + 4, 1,
			       FALSE);
	  break;
	case EH_SFLOW_HDR_DOWN:
	case EH_SFLOW_HDR_UP:
	  val = tvb_get_guint8 (tvb, pos+1);
	  if (val == 0)
	  {
	    proto_tree_add_item_hidden (ehdr_tree, hf_docsis_ehdr_phsi, tvb, pos+1, 1, FALSE);
	    proto_tree_add_text (ehdr_tree, tvb, pos+1, 1, "0000 0000 = No PHS on current packet" );
	  }
	  else
	    proto_tree_add_item(ehdr_tree, hf_docsis_ehdr_phsi, tvb, pos+1, 1, FALSE);

	  if (len == 2) 
	  {
	    proto_tree_add_item (ehdr_tree, hf_docsis_ehdr_qind, tvb, pos+2, 1, FALSE);
	    proto_tree_add_item (ehdr_tree, hf_docsis_ehdr_grants, tvb, pos+2, 1, FALSE);
	  }
	  break;
	default:
	  if (len > 0)
	    proto_tree_add_item (ehdr_tree, hf_docsis_eh_val, tvb, pos + 1,
				  len, FALSE);
	}
      pos += len + 1;
    }

  return;
}


static void
dissect_docsis (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  guint8 fc;
  guint8 fctype;
  guint8 fcparm;
  guint8 ehdron;
  guint8 mac_parm;
  guint8 hdrlen;
  guint16 len_sid;
  tvbuff_t *next_tvb, *mgt_tvb;
  guint16 pdulen, captured_length, framelen;
  gboolean isfrag = FALSE;

/* Set up structures needed to add the protocol subtree and manage it */
  proto_item *ti;
  proto_tree *docsis_tree;
/* concatlen and concatpos are declared static to allow for recursive calls to
 * the dissect_docsis routine when dissecting Concatenated frames
 */
  static guint16 concatlen;
  static guint16 concatpos;

/* Extract important fields */
  fc = tvb_get_guint8 (tvb, 0);	/* Frame Control Byte */
  fctype = (fc >> 6) & 0x03;	/* Frame Control Type:  2 MSB Bits */
  fcparm = (fc >> 1) & 0x1F;	/* Frame Control Parameter: Next 5 Bits */
  ehdron = (fc & 0x01);		/* Extended Header Bit: LSB */

  mac_parm = tvb_get_guint8 (tvb, 1);	/* Mac Parm */
  len_sid = tvb_get_ntohs (tvb, 2);	/* Length Or SID */

/* set Header length based on presence of Extended header */
  if (ehdron == 0x00)
    hdrlen = 6;
  else
    hdrlen = 6 + mac_parm;

/* Captured PDU Length is based on the length of the header */
  captured_length = tvb_length_remaining (tvb, hdrlen);

/* If this is a Request Frame, then pdulen is 0 and framelen is 6 */
  if ((fctype == FCTYPE_MACSPC) && fcparm == 0x02)
    {
      pdulen = 0;
      framelen = 6;
    }
  else
    {
      framelen = 6 + len_sid;
      pdulen = len_sid - ((guint16) (mac_parm & 0xFF));
    }

/* if this is a concatenated frame setup the length of the concatenated
 * frame and set the position to the first byte of the first frame */
  if ((fctype == FCTYPE_MACSPC) && (fcparm == 0x1c))
    {
      concatlen = len_sid;
      concatpos = 6;
    }

/* Make entries in Protocol column and Info column on summary display */
  if (check_col (pinfo->cinfo, COL_PROTOCOL))
    col_set_str (pinfo->cinfo, COL_PROTOCOL, "DOCSIS");

  if (check_col (pinfo->cinfo, COL_INFO))
    {
      col_clear (pinfo->cinfo, COL_INFO);
      switch (fctype)
	{
	case FCTYPE_PACKET:
	  col_set_str (pinfo->cinfo, COL_INFO, "Packet PDU");
	  break;
	case FCTYPE_ATMPDU:
	  col_set_str (pinfo->cinfo, COL_INFO, "ATM PDU");
	  break;
	case FCTYPE_RESRVD:
	  col_set_str (pinfo->cinfo, COL_INFO, "Reserved PDU");
	  break;
	case FCTYPE_MACSPC:
	  if (fcparm == 0x02)
	    col_add_fstr (pinfo->cinfo, COL_INFO,
			  "Request Frame SID = %u Mini Slots = %u", len_sid,
			  mac_parm);
	  else if (fcparm == 0x03)
	    col_set_str (pinfo->cinfo, COL_INFO, "Fragmented Frame");
	  else
	    col_set_str (pinfo->cinfo, COL_INFO, "Mac Specific");
	  break;
	}			/* switch */
    }				/* if */

/* In the interest of speed, if "tree" is NULL, don't do any work not
   necessary to generate protocol tree items. */
  if (tree)
    {
      ti = proto_tree_add_protocol_format (tree, proto_docsis, tvb, 0,
					   hdrlen, "DOCSIS");
      docsis_tree = proto_item_add_subtree (ti, ett_docsis);

/* add an item to the subtree, see section 1.6 for more information */
      proto_tree_add_item (docsis_tree, hf_docsis_fctype, tvb, 0, 1, FALSE);
      switch (fctype)
	{
	case FCTYPE_PACKET:
	case FCTYPE_ATMPDU:
	case FCTYPE_RESRVD:
	  proto_tree_add_item (docsis_tree, hf_docsis_fcparm, tvb, 0, 1,
			       FALSE);
	  proto_tree_add_item (docsis_tree, hf_docsis_ehdron, tvb, 0, 1,
			       FALSE);
	  if (ehdron == 0x01)
	    {
	      proto_tree_add_item (docsis_tree, hf_docsis_ehdrlen, tvb, 1, 1,
				   FALSE);
	      proto_tree_add_item (docsis_tree, hf_docsis_lensid, tvb, 2, 2,
				   FALSE);
	      dissect_ehdr (tvb, docsis_tree, isfrag);
	      proto_tree_add_item (docsis_tree, hf_docsis_hcs, tvb,
				   4 + mac_parm, 2, FALSE);
	    }
	  else
	    {
	      proto_tree_add_item (docsis_tree, hf_docsis_macparm, tvb, 1, 1,
				   FALSE);
	      proto_tree_add_item (docsis_tree, hf_docsis_lensid, tvb, 2, 2,
				   FALSE);
	      proto_tree_add_item (docsis_tree, hf_docsis_hcs, tvb, 4, 2,
				   FALSE);
	    }
	  break;
	case FCTYPE_MACSPC:
	  proto_tree_add_item (docsis_tree, hf_docsis_machdr_fcparm, tvb, 0,
			       1, FALSE);
	  proto_tree_add_item (docsis_tree, hf_docsis_ehdron, tvb, 0, 1,
			       FALSE);
	  /* Decode for a Request Frame.  No extended header */
	  if (fcparm == 0x02)
	    {
	      proto_tree_add_uint (docsis_tree, hf_docsis_mini_slots, tvb, 1,
				   1, mac_parm);
	      proto_tree_add_uint (docsis_tree, hf_docsis_sid, tvb, 2, 2,
				   len_sid);
	      proto_tree_add_item (docsis_tree, hf_docsis_hcs, tvb, 4, 2,
				   FALSE);
	      break;
	    }
	  /* Check if this is a fragmentation header */
	  if (fcparm == 0x03)
	    {
	      isfrag = TRUE;
	    }
	  /* Decode for a Concatenated Header.  No Extended Header */
	  if (fcparm == 0x1c)
	    {
	      proto_item_append_text (ti, " (Concatenated Header)");
	      proto_tree_add_item (docsis_tree, hf_docsis_concat_cnt, tvb, 1,
				   1, FALSE);
	      proto_tree_add_item (docsis_tree, hf_docsis_lensid, tvb, 2, 2,
				   FALSE);
	      proto_tree_add_item (docsis_tree, hf_docsis_hcs, tvb, 4, 2,
				   FALSE);
	      break;
	    }
	  /* If Extended header is present then decode it */
	  if (ehdron == 0x01)
	    {
	      proto_tree_add_item (docsis_tree, hf_docsis_ehdrlen, tvb, 1, 1,
				   FALSE);
	      proto_tree_add_item (docsis_tree, hf_docsis_lensid, tvb, 2, 2,
				   FALSE);
	      dissect_ehdr (tvb, docsis_tree, isfrag);
	      proto_tree_add_item (docsis_tree, hf_docsis_hcs, tvb,
				   4 + mac_parm, 2, FALSE);
	      break;
	    }
	  /* default case for all other Mac Frame Types */
	  proto_tree_add_item (docsis_tree, hf_docsis_macparm, tvb, 1, 1,
			       FALSE);
	  proto_tree_add_item (docsis_tree, hf_docsis_lensid, tvb, 2, 2,
			       FALSE);
	  proto_tree_add_item (docsis_tree, hf_docsis_hcs, tvb, 4, 2, FALSE);
	  break;
	}
    }

/* If this protocol has a sub-dissector call it here, see section 1.8 */
  if ((fctype == FCTYPE_PACKET) && (len_sid > mac_parm))
    {
      next_tvb = tvb_new_subset (tvb, hdrlen, captured_length, pdulen);
      call_dissector (eth_handle, next_tvb, pinfo, tree);
      if (concatlen > 0)
	{
	  concatlen = concatlen - framelen;
	  concatpos += framelen;
	}
    }

  if ((fctype == FCTYPE_PACKET) && (pdulen == 0))
    {
      if (concatlen > 0)
        {
          concatlen = concatlen - framelen;
          concatpos += framelen;
        }
    }
  if (fctype == FCTYPE_MACSPC)
    {
      switch (fcparm)
	{
	case 0x00:
	case 0x01:
	  mgt_tvb = tvb_new_subset (tvb, hdrlen, captured_length, pdulen);
	  call_dissector (docsis_mgmt_handle, mgt_tvb, pinfo, tree);
	  if (concatlen > 0)
	    {
	      concatlen = concatlen - framelen;
	      concatpos += framelen;
	    }
	  break;
	case 0x02:
	  /* Don't do anything for a Request Frame */
	  break;
	case 0x03:
	  /* For Fragmentation Frames simply dissect using the data
	   * dissector as we don't handle them yet
	   */
	  mgt_tvb = tvb_new_subset (tvb, hdrlen, captured_length, pdulen);
	  call_dissector (data_handle, mgt_tvb, pinfo, tree);
	  if (concatlen > 0)
	    {
	      concatlen = concatlen - framelen;
	      concatpos += framelen;
	    }
	  break;
	case 0x1c:
	  /* call the docsis dissector on the same frame
	   * to dissect DOCSIS frames within the concatenated
	   * frame.  concatpos and concatlen are declared
	   * static and are decremented and incremented
	   * respectively when the inner
	   * docsis frames are dissected. */
	  while (concatlen > 0)
	    {
	      next_tvb = tvb_new_subset (tvb, concatpos, -1, concatlen);
	      call_dissector (docsis_handle, next_tvb, pinfo, tree);
	    }
	  concatlen = 0;
	  concatpos = 0;
	  if (check_col (pinfo->cinfo, COL_INFO))
	    col_set_str (pinfo->cinfo, COL_INFO, "Concatenated Frame");
	  break;
	}
    }
}


/* Register the protocol with Ethereal */

/* this format is require because a script is used to build the C function
   that calls all the protocol registration.
*/


void
proto_register_docsis (void)
{

/* Setup list of header fields  See Section 1.6.1 for details*/
  static hf_register_info hf[] = {
    {&hf_docsis_fctype,
     {"FCType", "docsis.fctype",
      FT_UINT8, BASE_BIN, VALS (fctype_vals), 0xC0,
      "Frame Control Type", HFILL}
     },
    {&hf_docsis_fcparm,
     {"FCParm", "docsis.fcparm",
      FT_UINT8, BASE_BIN, NULL, 0x3E,
      "Parameter Field", HFILL}
     },
    {&hf_docsis_machdr_fcparm,
     {"FCParm", "docsis.fcparm",
      FT_UINT8, BASE_BIN, VALS (fcparm_vals), 0x3E,
      "Parameter Field", HFILL}
     },
    {&hf_docsis_ehdron,
     {"EHDRON", "docsis.ehdron",
      FT_UINT8, BASE_BIN, VALS (ehdron_vals), 0x01,
      "Extended Header Presence", HFILL}
     },
    {&hf_docsis_macparm,
     {"MacParm", "docsis.macparm",
      FT_UINT8, BASE_HEX, NULL, 0x0,
      "Mac Parameter Field", HFILL}
     },
    {&hf_docsis_concat_cnt,
     {"Number of Concatenated Frames", "docsis.macparm",
      FT_UINT8, BASE_DEC, NULL, 0x0,
      "Number of Concatenated Frames", HFILL}
     },
    {&hf_docsis_ehdrlen,
     {"Extended Header Length (bytes)", "docsis.macparm",
      FT_UINT8, BASE_DEC, NULL, 0x0,
      "Mac Parameter Field", HFILL}
     },
    {&hf_docsis_lensid,
     {"Length after HCS (bytes)", "docsis.lensid",
      FT_UINT16, BASE_DEC, NULL, 0x0,
      "Length or SID", HFILL}
     },
    {&hf_docsis_eh_type,
     {"Type", "docsis.ehdr.type",
      FT_UINT8, BASE_DEC, VALS (eh_type_vals), 0xF0,
      "TLV Type", HFILL}
     },
    {&hf_docsis_eh_len,
     {"Length", "docsis.ehdr.len",
      FT_UINT8, BASE_DEC, NULL, 0x0F,
      "TLV Len", HFILL}
     },
    {&hf_docsis_eh_val,
     {"Value", "docsis.ehdr.value",
      FT_BYTES, BASE_HEX, NULL, 0x0,
      "TLV Value", HFILL}
     },
    {&hf_docsis_frag_rsvd,
     {"Reserved", "docsis.frag_rsvd",
      FT_UINT8, BASE_DEC, NULL, 0xC0,
      "Reserved", HFILL}
     },
    {&hf_docsis_frag_first,
     {"First Frame", "docsis.frag_first",
      FT_UINT8, BASE_DEC, VALS(true_false_vals), 0x20,
      "First Frame", HFILL}
     },
    {&hf_docsis_frag_last,
     {"Last Frame", "docsis.frag_last",
      FT_UINT8, BASE_DEC, VALS(true_false_vals), 0x10,
      "Last Frame", HFILL}
     },
    {&hf_docsis_frag_seq,
     {"Fragmentation Sequence #", "docsis.frag_seq",
      FT_UINT8, BASE_DEC, NULL, 0x0F,
      "Fragmentation Sequence Number", HFILL}
     },
    {&hf_docsis_sid,
     {"SID", "docsis.ehdr.sid",
      FT_UINT16, BASE_DEC, NULL, 0x3FFF,
      "Service Identifier", HFILL}
     },
    {&hf_docsis_said,
     {"SAID", "docsis.ehdr.said",
      FT_UINT16, BASE_DEC, NULL, 0x3FFF,
      "Security Association Identifier", HFILL}
     },
    {&hf_docsis_reserved,
     {"Reserved", "docsis.ehdr.rsvd",
      FT_UINT8, BASE_HEX, NULL, 0x3FFF,
      "Reserved Byte", HFILL}
     },
    {&hf_docsis_mini_slots,
     {"MiniSlots", "docsis.ehdr.minislots",
      FT_UINT8, BASE_DEC, NULL, 0x0,
      "Mini Slots Requested", HFILL}
     },
    {&hf_docsis_key_seq,
     {"Key Sequence", "docsis.ehdr.keyseq",
      FT_UINT8, BASE_DEC, NULL, 0xF0,
      "Key Sequence", HFILL}
     },
    {&hf_docsis_ehdr_ver,
     {"Version", "docsis.ehdr.ver",
      FT_UINT8, BASE_DEC, NULL, 0x0F,
      "Version", HFILL}
     },
    {&hf_docsis_ehdr_phsi,
     {"Payload Header Supression Index", "docsis.ehdr.phsi",
      FT_UINT8, BASE_DEC, NULL, 0x0,
      "Payload Header Supression Index", HFILL}
     },
    {&hf_docsis_ehdr_qind,
     {"Queue Indicator", "docsis.ehdr.qind",
      FT_UINT8, BASE_DEC, VALS(on_off_vals), 0x80,
      "Queue Indicator", HFILL}
     },
    {&hf_docsis_ehdr_grants,
     {"Active Grants", "docsis.ehdr.act_grants",
      FT_UINT8, BASE_DEC, NULL, 0x0,
      "Active Grants", HFILL}
     },
    {&hf_docsis_hcs,
     {"Header check sequence", "docsis.hcs",
      FT_UINT16, BASE_HEX, NULL, 0x0,
      "Header check sequence", HFILL},
     },
    {&hf_docsis_bpi_en,
     {"Encryption", "docsis.bpi_en",
      FT_UINT8, BASE_HEX, VALS (ena_dis_vals), 0x80,
      "BPI Enable", HFILL},
     },
    {&hf_docsis_toggle_bit,
     {"Toggle", "docsis.toggle_bit",
      FT_UINT8, BASE_HEX, VALS (odd_even_vals), 0x40,
      "Toggle", HFILL},
     },

  };

/* Setup protocol subtree array */
  static gint *ett[] = {
    &ett_docsis,
    &ett_ehdr,
  };

  docsis_dissector_table = register_dissector_table ("docsis",
						     "DOCSIS Encapsulation Type",
						     FT_UINT8, BASE_DEC);

/* Register the protocol name and description */
  proto_docsis = proto_register_protocol ("DOCSIS 1.1", "DOCSIS", "docsis");

/* Required function calls to register the header fields and subtrees used */
  proto_register_field_array (proto_docsis, hf, array_length (hf));
  proto_register_subtree_array (ett, array_length (ett));

  register_dissector ("docsis", dissect_docsis, proto_docsis);
}


/* If this dissector uses sub-dissector registration add a registration routine.
   This format is required because a script is used to find these routines and
   create the code that calls these routines.
*/
void
proto_reg_handoff_docsis (void)
{


  docsis_handle = find_dissector ("docsis");
  data_handle = find_dissector ("data");
  dissector_add ("wtap_encap", WTAP_ENCAP_DOCSIS, docsis_handle);

  docsis_mgmt_handle = find_dissector ("docsis_mgmt");
  eth_handle = find_dissector ("eth");
}

/* Start the functions we need for the plugin stuff */

#ifndef __ETHEREAL_STATIC__

G_MODULE_EXPORT void
plugin_reg_handoff (void)
{
  proto_reg_handoff_docsis ();
  proto_reg_handoff_docsis_bpkmattr ();
  proto_reg_handoff_docsis_dsarsp ();
  proto_reg_handoff_docsis_mgmt ();
  proto_reg_handoff_docsis_rngreq ();
  proto_reg_handoff_docsis_rngrsp ();
  proto_reg_handoff_docsis_bpkmreq ();
  proto_reg_handoff_docsis_dscack ();
  proto_reg_handoff_docsis_map ();
  proto_reg_handoff_docsis_tlv ();
  proto_reg_handoff_docsis_bpkmrsp ();
  proto_reg_handoff_docsis_dscreq ();
  proto_reg_handoff_docsis_regack ();
  proto_reg_handoff_docsis_uccreq ();
  proto_reg_handoff_docsis_dscrsp ();
  proto_reg_handoff_docsis_regreq ();
  proto_reg_handoff_docsis_uccrsp ();
  proto_reg_handoff_docsis_dsaack ();
  proto_reg_handoff_docsis_dsdreq ();
  proto_reg_handoff_docsis_regrsp ();
  proto_reg_handoff_docsis_ucd ();
  proto_reg_handoff_docsis_dsareq ();
  proto_reg_handoff_docsis_dsdrsp ();
  proto_reg_handoff_docsis_vsif ();
}

G_MODULE_EXPORT void
plugin_init (plugin_address_table_t * pat
#ifndef PLUGINS_NEED_ADDRESS_TABLE
	     _U_
#endif
  )
{
  /* initialise the table of pointers needed in Win32 DLLs */
  plugin_address_table_init (pat);
  /* register the new protocol, protocol fields, and subtrees */
  if (proto_docsis == -1)
    {				/* execute protocol initialization only once */
      proto_register_docsis ();
      proto_register_docsis_bpkmattr ();
      proto_register_docsis_dsarsp ();
      proto_register_docsis_mgmt ();
      proto_register_docsis_rngrsp ();
      proto_register_docsis_bpkmreq ();
      proto_register_docsis_dscack ();
      proto_register_docsis_map ();
      proto_register_docsis_tlv ();
      proto_register_docsis_bpkmrsp ();
      proto_register_docsis_dscreq ();
      proto_register_docsis_regack ();
      proto_register_docsis_uccreq ();
      proto_register_docsis_dscrsp ();
      proto_register_docsis_regreq ();
      proto_register_docsis_uccrsp ();
      proto_register_docsis_dsaack ();
      proto_register_docsis_dsdreq ();
      proto_register_docsis_regrsp ();
      proto_register_docsis_ucd ();
      proto_register_docsis_dsareq ();
      proto_register_docsis_dsdrsp ();
      proto_register_docsis_rngreq ();
      proto_register_docsis_vsif ();

    }
}

#endif
