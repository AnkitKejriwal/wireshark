/* packet-prp.c
 * Routines for PRP (Parallel Redundancy Protocol; IEC62439 Chapter 6) dissection
 * Copyright 2007, Sven Meier <msv[AT]zhwin.ch>
 *
 * $Id$
 *
 * Revisions:
 * -
 *
 * A plugin for:
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald[AT]wireshark.org>
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <epan/packet.h>
#include <epan/etypes.h>

/**********************************************************/
/* Offsets of fields within a PRP packet.		  */
/**********************************************************/
#define	PRP_VERSION_OFFSET				0
#define	PRP_TYPE_OFFSET					2
#define	PRP_LENGTH_OFFSET				3
#define	PRP_SOURCEMACADDRESSA_OFFSET	4
#define	PRP_SOURCEMACADDRESSB_OFFSET	10

/**********************************************************/
/* Lengths of fields within a PRP packet.		  */
/**********************************************************/
#define PRP_VERSION_LENGTH 2
#define PRP_TYPE_LENGTH 1
#define PRP_LENGTH_LENGTH 1
#define PRP_SOURCE_LENGTH 6
#define PRP_TOTAL_LENGTH 16

/**********************************************************/
/* Channel values for the PRP_TYPE field		  */
/**********************************************************/
#define	PRP_TYPE_DUPLICATE_ACCEPT	21
#define	PRP_TYPE_DUPLICATE_DISCARD	20

static const value_string prp_type_vals[] = {
  {PRP_TYPE_DUPLICATE_ACCEPT,	"Duplicate Accept"},
  {PRP_TYPE_DUPLICATE_DISCARD,	"Duplicate Discard"},
  {0,				NULL          } };


#define	PRP_LAN_A					10
#define	PRP_LAN_B					11

static const value_string prp_lan_vals[] = {
  {PRP_LAN_A,	"LAN A"},
  {PRP_LAN_B,	"LAN B"},
  {0,		NULL } };

/**********************************************************/
/* Initialize the protocol and registered fields	  */
/**********************************************************/

static int proto_prp = -1;

/* Initialize supervision frame fields */
static int hf_prp_supervision_frame_version = -1;
static int hf_prp_supervision_frame_type = -1;
static int hf_prp_supervision_frame_length = -1;
static int hf_prp_supervision_frame_source_mac_address_A = -1;
static int hf_prp_supervision_frame_source_mac_address_B = -1;

/* Initialize trailer fields */
static int hf_prp_redundancy_control_trailer_sequence_nr = -1;
static int hf_prp_redundancy_control_trailer_lan = -1;
static int hf_prp_redundancy_control_trailer_size = -1;


/* Initialize the subtree pointers */
static gint ett_prp_supervision_frame = -1;
static gint ett_prp_redundancy_control_trailer = -1;

/* Code to actually dissect the packets */
static void
dissect_prp_supervision_frame(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	proto_item *ti;
	proto_tree *prp_tree;

	if (check_col(pinfo->cinfo, COL_PROTOCOL))
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "PRP");

	if (check_col(pinfo->cinfo, COL_INFO))
		col_set_str(pinfo->cinfo, COL_INFO, "Supervision Frame");

	if (!tree)
		return;

	/* create display subtree for the protocol */
	ti = proto_tree_add_item(tree, proto_prp, tvb, 0, PRP_TOTAL_LENGTH,
				 FALSE);

	prp_tree = proto_item_add_subtree(ti, ett_prp_supervision_frame);

	proto_tree_add_item(prp_tree, hf_prp_supervision_frame_version,
			    tvb, PRP_VERSION_OFFSET, PRP_VERSION_LENGTH, FALSE);

	proto_tree_add_item(prp_tree, hf_prp_supervision_frame_type,
			    tvb, PRP_TYPE_OFFSET, PRP_TYPE_LENGTH, FALSE);

	proto_tree_add_item(prp_tree, hf_prp_supervision_frame_length,
			    tvb, PRP_LENGTH_OFFSET, PRP_LENGTH_LENGTH, FALSE);

	proto_tree_add_item(prp_tree, hf_prp_supervision_frame_source_mac_address_A,
			    tvb, PRP_SOURCEMACADDRESSA_OFFSET, PRP_SOURCE_LENGTH,
			    FALSE);

	proto_tree_add_item(prp_tree, hf_prp_supervision_frame_source_mac_address_B,
			    tvb, PRP_SOURCEMACADDRESSB_OFFSET, PRP_SOURCE_LENGTH,
			    FALSE);
}

static void
dissect_prp_redundancy_control_trailer(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree)
{
	proto_item *ti;
	proto_tree *prp_tree;
	guint i;
	guint length;
	guint offset;
	guint16 lan_size;
	guint trailer_offset;

	if (!tree)
		return;

	trailer_offset = 0;
	length = tvb_reported_length(tvb);

	if(length < 14)
	{
		return;
	}

	if(0x8000 == tvb_get_ntohs(tvb, 12)) /* taged frame */
	{
		offset = 18;
	}
	else /* untaged */
	{
		offset = 14;
	}

	if(length <= 64)
	{
		for(i=length; i>=(offset+4); i--)  /* search trailer */
		{
			lan_size = tvb_get_ntohs(tvb, (i-2));
			if((lan_size == (0xa000 | ((i-offset) & 0x0fff)))
			   || (lan_size == (0xb000 | ((i-offset) & 0x0fff))))
			{
				trailer_offset = i;
			}
		}
	}
	else if(length > 64)
	{
		lan_size = tvb_get_ntohs(tvb, (length-2));
		if((lan_size == (0xa000 | ((length-offset) & 0x0fff)))
		    || (lan_size == (0xb000 | ((length-offset) & 0x0fff))))
		{
			trailer_offset = length;
		}
	}

	if(trailer_offset != 0)
	{
		/* create display subtree for the protocol */
		ti = proto_tree_add_item(tree, proto_prp, tvb, trailer_offset - 4,
					 trailer_offset, FALSE);

		prp_tree = proto_item_add_subtree(ti, ett_prp_redundancy_control_trailer);

		proto_tree_add_item(prp_tree, hf_prp_redundancy_control_trailer_sequence_nr,
				    tvb, (trailer_offset-4), 2, FALSE);

		proto_tree_add_item(prp_tree, hf_prp_redundancy_control_trailer_lan,
				    tvb, (trailer_offset-2), 2, FALSE);

		proto_tree_add_item(prp_tree, hf_prp_redundancy_control_trailer_size,
				    tvb, (trailer_offset-2), 2, FALSE);
	}
}

/* Register the protocol with Wireshark */
void proto_register_prp(void)
{
	static hf_register_info hf[] = {

		/*supervision frame*/
		{ &hf_prp_supervision_frame_version,
			{ "version", "prp.supervision_frame.version",
			FT_UINT16, BASE_DEC, NULL, 0x00,
			"", HFILL }
		},
		{ &hf_prp_supervision_frame_type,
			{ "type", "prp.supervision_frame.type",
			FT_UINT8, BASE_DEC, VALS(prp_type_vals), 0x00,
			"", HFILL }
		},
		{ &hf_prp_supervision_frame_length,
			{ "length", "prp.supervision_frame.length",
			FT_UINT8, BASE_DEC, NULL, 0x00,
			"", HFILL }
		},
		{ &hf_prp_supervision_frame_source_mac_address_A,
			{ "sourceMacAddressA", "prp.supervision_frame.prp_source_mac_address_A",
			FT_ETHER, BASE_DEC, NULL, 0x00,
			"", HFILL }
		},
		{ &hf_prp_supervision_frame_source_mac_address_B,
			{ "sourceMacAddressB", "prp.supervision_frame.prp_source_mac_address_B",
			FT_ETHER, BASE_DEC, NULL, 0x00,
			"", HFILL }
		},

		/*trailer*/
		{ &hf_prp_redundancy_control_trailer_sequence_nr,
			{ "sequenceNr", "prp.trailer.prp_sequence_nr",
			FT_UINT16, BASE_DEC, NULL, 0x00,
			"", HFILL }
		},

		{ &hf_prp_redundancy_control_trailer_lan,
			{ "lan", "prp.trailer.prp_lan",
			FT_UINT16, BASE_DEC, VALS(prp_lan_vals), 0xf000,
			"", HFILL }
		},

		{ &hf_prp_redundancy_control_trailer_size,
			{ "size", "prp.trailer.prp_size",
			FT_UINT16, BASE_DEC, NULL, 0x0fff,
			"", HFILL }
		},

	};

	static gint *ett[] = {
		&ett_prp_supervision_frame,
		&ett_prp_redundancy_control_trailer,
	};


	/* Register the protocol name and description */
	proto_prp = proto_register_protocol("Parallel Redundancy Protocol (IEC62439 Chapter 6)",
					    "PRP", "prp");

	/*  Post dissectors (such as the trailer dissector for this protocol)
	 *  get called for every single frame anyone loads into Wireshark.
	 *  Since this protocol is not of general interest we disable this
	 *  protocol by default (really every time Wireshark starts).
	 *
	 *  XXX This should be improved to only disable the protocol by
	 *  default.
	 */
  	proto_set_decoding(proto_prp, FALSE);

	/* Required function calls to register the header fields and subtree used */
	proto_register_field_array(proto_prp, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));

}

void proto_reg_handoff_prp(void)
{
	dissector_handle_t prp_supervision_frame_handle;
	dissector_handle_t prp_redundancy_control_trailer_handle;

	prp_supervision_frame_handle = create_dissector_handle(dissect_prp_supervision_frame, proto_prp);
	dissector_add("ethertype", ETHERTYPE_PRP, prp_supervision_frame_handle);

	prp_redundancy_control_trailer_handle = create_dissector_handle(dissect_prp_redundancy_control_trailer, proto_prp);
	register_postdissector(prp_redundancy_control_trailer_handle);
}
