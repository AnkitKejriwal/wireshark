/* packet-frame.c
 *
 * Top-most dissector. Decides dissector based on Wiretap Encapsulation Type.
 *
 * $Id$
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@ethereal.com>
 * Copyright 2000 Gerald Combs
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
#include <epan/timestamp.h>
#include <epan/tvbuff.h>
#include "packet-frame.h"
#include <epan/prefs.h>
#include <epan/tap.h>

int proto_frame = -1;
int hf_frame_arrival_time = -1;
static int hf_frame_time_delta = -1;
static int hf_frame_time_relative = -1;
int hf_frame_number = -1;
int hf_frame_packet_len = -1;
int hf_frame_capture_len = -1;
static int hf_frame_p2p_dir = -1;
static int hf_frame_file_off = -1;
static int hf_frame_marked = -1;
static int hf_frame_ref_time = -1;
static int hf_frame_protocols = -1;

static int proto_short = -1;
int proto_malformed = -1;
static int proto_unreassembled = -1;

static gint ett_frame = -1;

static int frame_tap = -1;

static dissector_handle_t data_handle;
static dissector_handle_t docsis_handle;
static dissector_handle_t mate_handle = NULL;

/* Preferences */
static gboolean show_file_off = FALSE;
static gboolean force_docsis_encap;

static const value_string p2p_dirs[] = {
	{ P2P_DIR_SENT,	"Sent" },
	{ P2P_DIR_RECV, "Received" },
	{ 0, NULL }
};

static dissector_table_t wtap_encap_dissector_table;

static void
dissect_frame(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree)
{
	proto_tree	*fh_tree=NULL;
	proto_item	*volatile ti = NULL;
	nstime_t	ts;
	int		cap_len, pkt_len;
	proto_tree	*tree;

	tree=parent_tree;

	pinfo->current_proto = "Frame";

	if (pinfo->pseudo_header != NULL) {
		switch (pinfo->fd->lnk_t) {

		case WTAP_ENCAP_WFLEET_HDLC:
		case WTAP_ENCAP_CHDLC_WITH_PHDR:
		case WTAP_ENCAP_PPP_WITH_PHDR:
		case WTAP_ENCAP_SDLC:
			pinfo->p2p_dir = pinfo->pseudo_header->p2p.sent ?
			    P2P_DIR_SENT : P2P_DIR_RECV;
			break;

		case WTAP_ENCAP_LAPB:
		case WTAP_ENCAP_FRELAY_WITH_PHDR:
			pinfo->p2p_dir =
			    (pinfo->pseudo_header->x25.flags & FROM_DCE) ?
			    P2P_DIR_RECV : P2P_DIR_SENT;
			break;

		case WTAP_ENCAP_ISDN:
			pinfo->p2p_dir = pinfo->pseudo_header->isdn.uton ?
			    P2P_DIR_SENT : P2P_DIR_RECV;
			break;
		}
	}

	if ((force_docsis_encap) && (docsis_handle)) {
		/*
		 * XXX - setting it here makes it impossible to
		 * turn the "Treat all frames as DOCSIS frames"
		 * option off.
		 *
		 * The TCP Graph code currently uses "fd->lnk_t";
		 * it should eventually just get the information
		 * it needs from a full-blown dissection, so that
		 * can handle any link-layer type.
		 */
		pinfo->fd->lnk_t = WTAP_ENCAP_DOCSIS;
	}

	/* Put in frame header information. */
	if (tree) {
	  cap_len = tvb_length(tvb);
	  pkt_len = tvb_reported_length(tvb);

	  ti = proto_tree_add_protocol_format(tree, proto_frame, tvb, 0, -1,
	    "Frame %u (%u bytes on wire, %u bytes captured)", pinfo->fd->num, pkt_len, cap_len);

	  fh_tree = proto_item_add_subtree(ti, ett_frame);
	}

	/* if IP is not referenced from any filters we dont need to worry about
	   generating any tree items.  We must do this after we created the actual
	   protocol above so that proto hier stat still works though.
	*/
	if(!proto_field_is_referenced(tree, proto_frame)){
		tree=NULL;
		fh_tree = NULL;
	}

       
	if (fh_tree) {
	  proto_tree_add_boolean_hidden(fh_tree, hf_frame_marked, tvb, 0, 0,pinfo->fd->flags.marked);

	  if(pinfo->fd->flags.ref_time){
		proto_tree_add_item(fh_tree, hf_frame_ref_time, tvb, 0, 0, FALSE);
	  }

	  ts.secs = pinfo->fd->abs_secs;
	  ts.nsecs = pinfo->fd->abs_usecs*1000;

	  proto_tree_add_time(fh_tree, hf_frame_arrival_time, tvb,
		0, 0, &ts);

	  ts.secs = pinfo->fd->del_secs;
	  ts.nsecs = pinfo->fd->del_usecs*1000;

	  proto_tree_add_time(fh_tree, hf_frame_time_delta, tvb,
		0, 0, &ts);

	  ts.secs = pinfo->fd->rel_secs;
	  ts.nsecs = pinfo->fd->rel_usecs*1000;

	  proto_tree_add_time(fh_tree, hf_frame_time_relative, tvb,
		0, 0, &ts);

	  proto_tree_add_uint(fh_tree, hf_frame_number, tvb,
		0, 0, pinfo->fd->num);

	  proto_tree_add_uint_format(fh_tree, hf_frame_packet_len, tvb,
		0, 0, pkt_len, "Packet Length: %d byte%s", pkt_len,
		plurality(pkt_len, "", "s"));

	  proto_tree_add_uint_format(fh_tree, hf_frame_capture_len, tvb,
		0, 0, cap_len, "Capture Length: %d byte%s", cap_len,
		plurality(cap_len, "", "s"));

	  ti = proto_tree_add_string(fh_tree, hf_frame_protocols, tvb,
	  	0, 0, "");
	  pinfo->layer_names = g_string_new("");

	  /* Check for existences of P2P pseudo header */
	  if (pinfo->p2p_dir != P2P_DIR_UNKNOWN) {
		  proto_tree_add_uint(fh_tree, hf_frame_p2p_dir, tvb,
				  0, 0, pinfo->p2p_dir);
	  }

	  if (show_file_off) {
		  proto_tree_add_int_format(fh_tree, hf_frame_file_off, tvb,
				  0, 0, pinfo->fd->file_off,
				  "File Offset: %ld (0x%lx)",
				  pinfo->fd->file_off, pinfo->fd->file_off);
	  }
	}

	TRY {
		if (!dissector_try_port(wtap_encap_dissector_table, pinfo->fd->lnk_t,
					tvb, pinfo, parent_tree)) {

			if (check_col(pinfo->cinfo, COL_PROTOCOL))
				col_set_str(pinfo->cinfo, COL_PROTOCOL, "UNKNOWN");
			if (check_col(pinfo->cinfo, COL_INFO))
				col_add_fstr(pinfo->cinfo, COL_INFO, "WTAP_ENCAP = %u",
				    pinfo->fd->lnk_t);
			call_dissector(data_handle,tvb, pinfo, parent_tree);
		}
	}
	CATCH_ALL {
		show_exception(tvb, pinfo, parent_tree, EXCEPT_CODE, GET_MESSAGE);
	}
	ENDTRY;

	if (tree) {
		proto_item_append_string(ti, pinfo->layer_names->str);
		g_string_free(pinfo->layer_names, TRUE);
		pinfo->layer_names = NULL;
	}

	tap_queue_packet(frame_tap, pinfo, NULL);

	if (mate_handle) call_dissector(mate_handle,tvb, pinfo, parent_tree);

}

void
show_exception(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
    unsigned long exception, const char *exception_message)
{
	switch (exception) {

	case BoundsError:
		if (check_col(pinfo->cinfo, COL_INFO))
			col_append_str(pinfo->cinfo, COL_INFO, "[Short Frame]");
		proto_tree_add_protocol_format(tree, proto_short, tvb, 0, 0,
				"[Short Frame: %s]", pinfo->current_proto);
		break;

	case ReportedBoundsError:
		show_reported_bounds_error(tvb, pinfo, tree);
		break;

	case DissectorError:
		if (check_col(pinfo->cinfo, COL_INFO))
			col_append_fstr(pinfo->cinfo, COL_INFO,
			    "[Dissector bug, protocol %s: %s]",
			    pinfo->current_proto, exception_message);
		proto_tree_add_protocol_format(tree, proto_malformed, tvb, 0, 0,
		    "[Dissector bug, protocol %s: %s]",
		    pinfo->current_proto, exception_message);
		g_warning("Dissector bug, protocol %s, in packet %u: %s",
		    pinfo->current_proto, pinfo->fd->num, exception_message);
		g_free((void *)exception_message);
		break;

	default:
		/* XXX - we want to know, if an unknown exception passed until here, don't we? */
		g_assert_not_reached();
	}
}

void
show_reported_bounds_error(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	if (pinfo->fragmented) {
		/*
		 * We were dissecting an unreassembled fragmented
		 * packet when the exception was thrown, so the
		 * problem isn't that the dissector expected
		 * something but it wasn't in the packet, the
		 * problem is that the dissector expected something
		 * but it wasn't in the fragment we dissected.
		 */
		if (check_col(pinfo->cinfo, COL_INFO))
			col_append_fstr(pinfo->cinfo, COL_INFO,
			    "[Unreassembled Packet%s]",
			    pinfo->noreassembly_reason);
		proto_tree_add_protocol_format(tree, proto_unreassembled,
		    tvb, 0, 0, "[Unreassembled Packet%s: %s]",
		    pinfo->noreassembly_reason, pinfo->current_proto);
	} else {
		if (check_col(pinfo->cinfo, COL_INFO))
			col_append_str(pinfo->cinfo, COL_INFO,
			    "[Malformed Packet]");
		proto_tree_add_protocol_format(tree, proto_malformed,
		    tvb, 0, 0, "[Malformed Packet: %s]", pinfo->current_proto);
	}
}

void
proto_register_frame(void)
{
	static hf_register_info hf[] = {
		{ &hf_frame_arrival_time,
		{ "Arrival Time",		"frame.time", FT_ABSOLUTE_TIME, BASE_NONE, NULL, 0x0,
			"Absolute time when this frame was captured", HFILL }},

		{ &hf_frame_time_delta,
		{ "Time delta from previous packet",	"frame.time_delta", FT_RELATIVE_TIME, BASE_NONE, NULL,
			0x0,
			"Time delta since previous diplayed frame", HFILL }},

		{ &hf_frame_time_relative,
		{ "Time since reference or first frame",	"frame.time_relative", FT_RELATIVE_TIME, BASE_NONE, NULL,
			0x0,
			"Time relative reference or first frame", HFILL }},

		{ &hf_frame_number,
		{ "Frame Number",		"frame.number", FT_UINT32, BASE_DEC, NULL, 0x0,
			"", HFILL }},

		{ &hf_frame_packet_len,
		{ "Total Frame Length",		"frame.pkt_len", FT_UINT32, BASE_DEC, NULL, 0x0,
			"", HFILL }},

		{ &hf_frame_capture_len,
		{ "Capture Frame Length",	"frame.cap_len", FT_UINT32, BASE_DEC, NULL, 0x0,
			"", HFILL }},

		{ &hf_frame_p2p_dir,
		{ "Point-to-Point Direction",	"frame.p2p_dir", FT_UINT8, BASE_DEC, VALS(p2p_dirs), 0x0,
			"", HFILL }},

		{ &hf_frame_file_off,
		{ "File Offset",	"frame.file_off", FT_INT32, BASE_DEC, NULL, 0x0,
			"", HFILL }},

		{ &hf_frame_marked,
		{ "Frame is marked",	"frame.marked", FT_BOOLEAN, 8, NULL, 0x0,
			"Frame is marked in the GUI", HFILL }},

		{ &hf_frame_ref_time,
		{ "This is a Ref Time frame",	"frame.ref_time", FT_NONE, 0, NULL, 0x0,
			"This frame is a Reference Time frame", HFILL }},

		{ &hf_frame_protocols,
		{ "Protocols in frame",	"frame.protocols", FT_STRING, 0, NULL, 0x0,
			"Protocols carried by this frame", HFILL }},
	};
	static gint *ett[] = {
		&ett_frame,
	};
	module_t *frame_module;

	wtap_encap_dissector_table = register_dissector_table("wtap_encap",
	    "Wiretap encapsulation type", FT_UINT32, BASE_DEC);

	proto_frame = proto_register_protocol("Frame", "Frame", "frame");
	proto_register_field_array(proto_frame, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));
	register_dissector("frame",dissect_frame,proto_frame);

	/* You can't disable dissection of "Frame", as that would be
	   tantamount to not doing any dissection whatsoever. */
	proto_set_cant_toggle(proto_frame);

	proto_short = proto_register_protocol("Short Frame", "Short frame", "short");
	proto_malformed = proto_register_protocol("Malformed Packet",
	    "Malformed packet", "malformed");
	proto_unreassembled = proto_register_protocol(
	    "Unreassembled Fragmented Packet",
	    "Unreassembled fragmented packet", "unreassembled");

	/* "Short Frame", "Malformed Packet", and "Unreassembled Fragmented
	   Packet" aren't really protocols, they're error indications;
	   disabling them makes no sense. */
	proto_set_cant_toggle(proto_short);
	proto_set_cant_toggle(proto_malformed);
	proto_set_cant_toggle(proto_unreassembled);

	/* Our preferences */
	frame_module = prefs_register_protocol(proto_frame, NULL);
	prefs_register_bool_preference(frame_module, "show_file_off",
	    "Show File Offset", "Show File Offset", &show_file_off);
	prefs_register_bool_preference(frame_module, "force_docsis_encap",
	    "Treat all frames as DOCSIS frames", "Treat all frames as DOCSIS Frames", &force_docsis_encap);

	frame_tap=register_tap("frame");
}

void
proto_reg_handoff_frame(void)
{
	data_handle = find_dissector("data");
	docsis_handle = find_dissector("docsis");
	mate_handle = find_dissector("mate");
}
