/* packet-irc.c
 * Routines for MSX irc packet dissection
 *
 * $Id: packet-irc.c,v 1.6 2000/05/31 05:07:11 guy Exp $
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@zing.org>
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
#include "packet.h"

static int proto_irc = -1;
static int hf_irc_request = -1;
static int hf_irc_response = -1;
static int hf_irc_command = -1;

static gint ett_irc = -1;

#define TCP_PORT_IRC			6667
	/* good candidate for dynamic port specification */

static void
dissect_irc_request(proto_tree *tree, char *line, int offset, int len)
{
	proto_tree_add_boolean_hidden(tree, hf_irc_request, NullTVB,
		offset, len, TRUE);
	proto_tree_add_text(tree, NullTVB, offset, 
		len, "Request Line: %s", line);
}

static void
dissect_irc_response(proto_tree *tree, char *line, int offset, int len)
{
	proto_tree_add_boolean_hidden(tree, hf_irc_response, NullTVB,
		offset, len, TRUE);
	proto_tree_add_text(tree, NullTVB, offset, 
		len, "Response Line: %s", line);
}

static void
dissect_irc(const u_char *pd, int offset, frame_data *fd, proto_tree *tree)
{
	proto_tree      *irc_tree, *ti;
	char *tmpline;
	int start, cur, len;
	const u_char *i;

	if (check_col(fd, COL_PROTOCOL))
	col_add_str(fd, COL_PROTOCOL, "IRC");

	if (check_col(fd, COL_INFO))
	{
		col_add_fstr(fd, COL_INFO, "%s", 
			(pi.match_port == pi.destport) ? "Request" : "Response");	  
	}

	if (tree) 
	{
		ti = proto_tree_add_item(tree, proto_irc, NullTVB, offset, END_OF_FRAME, FALSE);
		irc_tree = proto_item_add_subtree(ti, ett_irc);

		tmpline = (char *)g_malloc( pi.captured_len );
		i = pd+offset;
		while ( i < pd + pi.captured_len )
		{
			start = i - pd;
			cur = 0;
			len = 0;
			tmpline[cur] = 0;

			/* copy up to end or cr/nl */
			while ( i < pd + pi.captured_len && *i != '\r' && *i != '\n' )
			{
				tmpline[cur++] = *(i++);
				len++;
			}
			tmpline[cur] = 0;

			/* skip any CR/NL */
			while ( i < pd + pi.captured_len && 
				(*i == '\r' || *i == '\n') )
			{
				i++;
				len++;
			}

			if ( strlen(tmpline) > 0 )
			{
				if (pi.match_port == pi.destport)
				{
					dissect_irc_request(irc_tree, tmpline, start, len);
				}
				else
				{
					dissect_irc_response(irc_tree, tmpline, start, len);
				}
			}
		}
		g_free(tmpline);
		tmpline = 0;
	}
}

void
proto_register_irc(void)
{
	static hf_register_info hf[] = {
	  { &hf_irc_response,
	    { "Response",           "irc.response",
	      FT_BOOLEAN, BASE_NONE, NULL, 0x0,
	      "TRUE if IRC response" }},
	  
	  { &hf_irc_request,
	    { "Request",            "irc.request",
	      FT_BOOLEAN, BASE_NONE, NULL, 0x0,
	      "TRUE if IRC request" }},

	  { &hf_irc_command,
	    { "Command",            "irc.command",
	      FT_STRING, BASE_NONE, NULL, 0x0,
	      "Command associated with request" }}
	};

	static gint *ett[] = {
		&ett_irc,
	};
	proto_irc = proto_register_protocol("Internet Relay Chat", "irc");
	proto_register_field_array(proto_irc, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));
}

void
proto_reg_handoff_irc(void)
{
	dissector_add("tcp.port", TCP_PORT_IRC, dissect_irc);
}

