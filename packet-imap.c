/* packet-imap.c
 * Routines for imap packet dissection
 * Copyright 1999, Richard Sharpe <rsharpe@ns.aus.com>
 *
 * $Id: packet-imap.c,v 1.19 2002/01/24 09:20:48 guy Exp $
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@ethereal.com>
 * Copyright 1998 Gerald Combs
 *
 * Copied from packet-tftp.c
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

#include <stdio.h>

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif

#include <string.h>
#include <glib.h>
#include <epan/packet.h>
#include <epan/strutil.h>

static int proto_imap = -1;
static int hf_imap_response = -1;
static int hf_imap_request = -1;

static gint ett_imap = -1;

#define TCP_PORT_IMAP			143

static void
dissect_imap(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
        gboolean        is_request;
        proto_tree      *imap_tree, *ti;
	gint		offset = 0;
	const u_char	*line;
	gint		next_offset;
	int		linelen;
	int		tokenlen;
	const u_char	*next_token;

	if (check_col(pinfo->cinfo, COL_PROTOCOL))
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "IMAP");

	/*
	 * Find the end of the first line.
	 *
	 * Note that "tvb_find_line_end()" will return a value that is
	 * not longer than what's in the buffer, so the "tvb_get_ptr()"
	 * call won't throw an exception.
	 */
	linelen = tvb_find_line_end(tvb, offset, -1, &next_offset);
	line = tvb_get_ptr(tvb, offset, linelen);

	if (pinfo->match_port == pinfo->destport)
		is_request = TRUE;
	else
		is_request = FALSE;

	if (check_col(pinfo->cinfo, COL_INFO)) {
		/*
		 * Put the first line from the buffer into the summary
		 * (but leave out the line terminator).
		 */
		col_add_fstr(pinfo->cinfo, COL_INFO, "%s: %s",
		    is_request ? "Request" : "Response",
		    format_text(line, linelen));
	}

	if (tree) {
		ti = proto_tree_add_item(tree, proto_imap, tvb, offset, -1,
		    FALSE);
		imap_tree = proto_item_add_subtree(ti, ett_imap);

		if (is_request) {
			proto_tree_add_boolean_hidden(imap_tree,
			    hf_imap_request, tvb, 0, 0, TRUE);
		} else {
			proto_tree_add_boolean_hidden(imap_tree,
			    hf_imap_response, tvb, 0, 0, TRUE);
		}

		/*
		 * Show the first line as tags + requests or replies.
		 */

		/*
		 * Extract the first token, and, if there is a first
		 * token, add it as the request or reply tag.
		 */
		tokenlen = get_token_len(line, line + linelen, &next_token);
		if (tokenlen != 0) {
			if (is_request) {
				proto_tree_add_text(imap_tree, tvb, offset,
				    tokenlen, "Request Tag: %s",
				    format_text(line, tokenlen));
			} else {
				proto_tree_add_text(imap_tree, tvb, offset,
				    tokenlen, "Response Tag: %s",
				    format_text(line, tokenlen));
			}
			offset += next_token - line;
			linelen -= next_token - line;
			line = next_token;
		}

		/*
		 * Add the rest of the line as request or reply data.
		 */
		if (linelen != 0) {
			if (is_request) {
				proto_tree_add_text(imap_tree, tvb, offset,
				    linelen, "Request: %s",
				    format_text(line, linelen));
			} else {
				proto_tree_add_text(imap_tree, tvb, offset,
				    linelen, "Response: %s",
				    format_text(line, linelen));
			}
		}

		/*
		 * XXX - show the rest of the frame; this requires that
		 * we handle literals, quoted strings, continuation
		 * responses, etc..
		 *
		 * This involves a state machine, and attaching
		 * state information to the packets.
		 */
	}
}

void
proto_register_imap(void)
{
  static hf_register_info hf[] = {
    { &hf_imap_response,
      { "Response",           "imap.response",
	FT_BOOLEAN, BASE_NONE, NULL, 0x0,
      	"TRUE if IMAP response", HFILL }},

    { &hf_imap_request,
      { "Request",            "imap.request",
	FT_BOOLEAN, BASE_NONE, NULL, 0x0,
      	"TRUE if IMAP request", HFILL }}
  };
  static gint *ett[] = {
    &ett_imap,
  };

  proto_imap = proto_register_protocol("Internet Message Access Protocol", 
				       "IMAP", "imap");
  proto_register_field_array(proto_imap, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));
}

void
proto_reg_handoff_imap(void)
{
  dissector_handle_t imap_handle;

  imap_handle = create_dissector_handle(dissect_imap, proto_imap);
  dissector_add("tcp.port", TCP_PORT_IMAP, imap_handle);
}
