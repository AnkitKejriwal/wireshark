/* packet-loop.c
 * Routines for Ethernet loopback/Configuration Test Protocol dissection
 *
 * See
 *
 *	http://stuff.mit.edu/people/jhawk/ctp.html
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
#include "etypes.h"

static int proto_loop = -1;
static int hf_loop_skipcount = -1;
static int hf_loop_function = -1;
static int hf_loop_receipt_number = -1;
static int hf_loop_forwarding_address = -1;

static gint ett_loop = -1;

static dissector_handle_t data_handle;

#define FUNC_REPLY		1
#define FUNC_FORWARD_DATA	2

static const value_string function_vals[] = {
  { FUNC_REPLY, "Reply" },
  { FUNC_FORWARD_DATA, "Forward Data" },
  { 0, NULL }
};

static void
dissect_loop(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
  proto_tree  *loop_tree = NULL;
  proto_item  *ti;
  guint16     function;
  int         offset = 0;
  gboolean    set_info = FALSE;
  tvbuff_t    *next_tvb;

  if (check_col(pinfo->cinfo, COL_PROTOCOL))
    col_set_str(pinfo->cinfo, COL_PROTOCOL, "LOOP");
  if (check_col(pinfo->cinfo, COL_INFO))
    col_clear(pinfo->cinfo, COL_INFO);

  if (tree) {
    ti = proto_tree_add_item(tree, proto_loop, tvb, 0, -1, FALSE);
    loop_tree = proto_item_add_subtree(ti, ett_loop);
  }

  for (;;) {
    if (tree)
      proto_tree_add_item(loop_tree, hf_loop_skipcount, tvb, offset, 2, FALSE);
    offset += 2;
    function = tvb_get_ntohs(tvb, 2);
    if (!set_info) {
      if (check_col(pinfo->cinfo, COL_INFO)) {
        col_add_str(pinfo->cinfo, COL_INFO,
                    val_to_str(function, function_vals, "Unknown function (%u)"));
      }
      set_info = TRUE;
    }
    if (tree)
      proto_tree_add_uint(loop_tree, hf_loop_function, tvb, offset, 2, function);
    offset += 2;
    switch (function) {

    case FUNC_REPLY:
      if (tree)
        proto_tree_add_item(loop_tree, hf_loop_receipt_number, tvb, offset, 2,
                            FALSE);
      offset += 2;
      next_tvb = tvb_new_subset(tvb, offset, -1, -1);
      call_dissector(data_handle, next_tvb, pinfo, tree);
      return;

    case FUNC_FORWARD_DATA:
      if (tree)
        proto_tree_add_item(loop_tree, hf_loop_forwarding_address, tvb, offset,
                            6, FALSE);
      offset += 6;
      break;

    default:
      next_tvb = tvb_new_subset(tvb, offset, -1, -1);
      call_dissector(data_handle, next_tvb, pinfo, tree);
      return;
    }
  }
}

void
proto_register_loop(void)
{
  static hf_register_info hf[] = {
    { &hf_loop_skipcount,
      { "skipCount",		"loop.skipcount",
	FT_UINT16,	BASE_DEC,	NULL,	0x0,
      	"", HFILL }},

    { &hf_loop_function,
      { "Function",		"loop.function",
	FT_UINT16,	BASE_DEC,	VALS(function_vals),	0x0,
      	"", HFILL }},

    { &hf_loop_receipt_number,
      { "Receipt number",	"loop.receipt_number",
	FT_UINT16,	BASE_DEC,	NULL,	0x0,
      	"", HFILL }},

    { &hf_loop_forwarding_address,
      { "Forwarding address",	"loop.forwarding_address",
	FT_ETHER,	BASE_NONE,	NULL,	0x0,
      	"", HFILL }},
  };
  static gint *ett[] = {
    &ett_loop,
  };

  proto_loop = proto_register_protocol("Configuration Test Protocol (loopback)",
				      "LOOP", "loop");
  proto_register_field_array(proto_loop, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));
}

void
proto_reg_handoff_loop(void)
{
  dissector_handle_t loop_handle;

  loop_handle = create_dissector_handle(dissect_loop, proto_loop);

  dissector_add("ethertype", ETHERTYPE_LOOP, loop_handle);
}
