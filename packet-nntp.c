/* packet-nntp.c
 * Routines for nntp packet dissection
 * Copyright 1999, Richard Sharpe <rsharpe@ns.aus.com>
 *
 * $Id: packet-nntp.c,v 1.1 1999/04/06 02:02:11 guy Exp $
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@unicom.net>
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

#include <stdio.h>

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif

#include <string.h>
#include <glib.h>
#include "packet.h"

extern packet_info pi;

void
dissect_nntp(const u_char *pd, int offset, frame_data *fd, proto_tree *tree, int max_data)
{
        gchar           *type;
        proto_tree      *nntp_tree, *ti;
	const u_char	*data, *dataend;
	const u_char	*lineend, *eol;
	int		linelen;

	data = &pd[offset];
	dataend = data + END_OF_FRAME;
	if (dataend > data + max_data)
		dataend = data + max_data;

        if (pi.match_port == pi.destport)
        	type = "Request";
        else
        	type = "Response";

	if (check_col(fd, COL_PROTOCOL))
		col_add_str(fd, COL_PROTOCOL, "NNTP");

	if (check_col(fd, COL_INFO)) {
		/*
		 * Put the first line from the buffer into the summary.
		 */
		lineend = find_line_end(data, dataend, &eol);
		linelen = eol - data;
		col_add_fstr(fd, COL_INFO, "%s: %s", type,
		    format_text(data, linelen));
	}

	if (tree) {

	  ti = proto_tree_add_item(tree, offset, END_OF_FRAME,
				"Network News Transfer Protocol");
	  nntp_tree = proto_tree_new();
	  proto_item_add_subtree(ti, nntp_tree, ETT_NNTP);

	  /*
	   * Show the request or response as text, a line at a time.
	   * XXX - for requests, we could display the stuff after the
	   * first line, if any, based on what the request was, and
	   * for responses, we could display it based on what the
	   * matching request was, although the latter requires us to
	   * know what the matching request was....
	   */
	  while (data < dataend) {
		/*
		 * Find the end of the line.
		 */
		lineend = find_line_end(data, dataend, &eol);
		linelen = lineend - data;

		/*
		 * Put this line.
		 */
		proto_tree_add_item(nntp_tree, offset, linelen, "%s",
		    format_text(data, linelen));
		offset += linelen;
		data = lineend;
	  }
	}
}
