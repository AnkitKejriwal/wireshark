/* packet-http.c
 * Routines for HTTP packet disassembly
 *
 * Guy Harris <guy@alum.mit.edu>
 *
 * Copyright 2002, Tim Potter <tpot@samba.org>
 * Copyright 1999, Andrew Tridgell <tridge@samba.org>
 *
 * $Id: packet-http.c,v 1.74 2003/12/07 03:17:42 guy Exp $
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
#include "config.h"
#endif

#include <string.h>
#include <ctype.h>

#include <glib.h>
#include <epan/packet.h>
#include <epan/strutil.h>

#include "util.h"
#include "packet-http.h"
#include "prefs.h"

typedef enum _http_type {
	HTTP_REQUEST,
	HTTP_RESPONSE,
	HTTP_NOTIFICATION,
	HTTP_OTHERS
} http_type_t;

#include "tap.h"

static int http_tap = -1;

static int proto_http = -1;
static int hf_http_notification = -1;
static int hf_http_response = -1;
static int hf_http_request = -1;
static int hf_http_basic = -1;
static int hf_http_request_method = -1;
static int hf_http_response_code = -1;
static int hf_http_authorization = -1;
static int hf_http_proxy_authenticate = -1;
static int hf_http_proxy_authorization = -1;
static int hf_http_www_authenticate = -1;
static int hf_http_content_type = -1;

static gint ett_http = -1;
static gint ett_http_ntlmssp = -1;
static gint ett_http_request = -1;

static dissector_handle_t data_handle;
static dissector_handle_t http_handle;

/*
 * desegmentation of HTTP headers
 * (when we are over TCP or another protocol providing the desegmentation API)
 */
static gboolean http_desegment_headers = FALSE;

/*
 * desegmentation of HTTP bodies
 * (when we are over TCP or another protocol providing the desegmentation API)
 * TODO let the user filter on content-type the bodies he wants desegmented
 */
static gboolean http_desegment_body = FALSE;

#define TCP_PORT_HTTP			80
#define TCP_PORT_PROXY_HTTP		3128
#define TCP_PORT_PROXY_ADMIN_HTTP	3132
#define TCP_ALT_PORT_HTTP		8080

/*
 * SSDP is implemented atop HTTP (yes, it really *does* run over UDP).
 */
#define TCP_PORT_SSDP			1900
#define UDP_PORT_SSDP			1900

/*
 * Protocols implemented atop HTTP.
 */
typedef enum {
	PROTO_HTTP,		/* just HTTP */
	PROTO_SSDP		/* Simple Service Discovery Protocol */
} http_proto_t;

typedef void (*RequestDissector)(tvbuff_t*, proto_tree*, int);

/*
 * Structure holding information from entity headers needed by main
 * HTTP dissector code.
 */
typedef struct {
	char	*content_type;
} entity_headers_t;

static int is_http_request_or_reply(const guchar *data, int linelen, http_type_t *type,
		RequestDissector *req_dissector, int *req_strlen);
static void process_entity_header(tvbuff_t *tvb, int offset, int next_offset,
    const guchar *line, int linelen, int colon_offset,
    packet_info *pinfo, proto_tree *tree, entity_headers_t *eh_ptr);
static gint find_header_hf_value(tvbuff_t *tvb, int offset, guint header_len);
static gboolean check_auth_ntlmssp(proto_item *hdr_item, tvbuff_t *tvb,
    packet_info *pinfo, gchar *value);
static gboolean check_auth_basic(proto_item *hdr_item, tvbuff_t *tvb,
    gchar *value);

static dissector_table_t port_subdissector_table;
static dissector_table_t content_type_subdissector_table;
static heur_dissector_list_t heur_subdissector_list;

static dissector_handle_t ntlmssp_handle=NULL;


/* Return a tvb that contains the binary representation of a base64
   string */

static tvbuff_t *
base64_to_tvb(const char *base64)
{
	tvbuff_t *tvb;
	char *data = g_strdup(base64);
	size_t len;

	len = base64_decode(data);
	tvb = tvb_new_real_data(data, len, len);

	tvb_set_free_cb(tvb, g_free);

	return tvb;
}

static void
dissect_http_ntlmssp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
    const char *line)
{
	tvbuff_t *ntlmssp_tvb;

	ntlmssp_tvb = base64_to_tvb(line);
	tvb_set_child_real_data_tvbuff(tvb, ntlmssp_tvb);
	add_new_data_source(pinfo, ntlmssp_tvb, "NTLMSSP Data");

	call_dissector(ntlmssp_handle, ntlmssp_tvb, pinfo, tree);
}

/* TODO: remove this ugly global variable */
http_info_value_t	*stat_info;
static void
dissect_http(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	http_proto_t	proto;
	char		*proto_tag;
	proto_tree	*http_tree = NULL;
	proto_item	*ti = NULL;
	gint		offset = 0;
	const guchar	*line;
	gint		next_offset;
	gint		next_offset_sav;
	const guchar	*linep, *lineend;
	int		first_linelen, linelen;
	gboolean	is_request_or_reply;
	guchar		c;
	http_type_t     http_type;
	int		datalen;
	proto_item	*hdr_item;
	RequestDissector req_dissector;
	int		req_strlen;
	proto_tree	*req_tree;
	int		colon_offset;
	long int	content_length;
	gboolean	content_length_found = FALSE;
	entity_headers_t entity_headers;
	dissector_handle_t handle;
	gboolean	dissected;

	/*
	 * Is this a request or response?
	 *
	 * Note that "tvb_find_line_end()" will return a value that
	 * is not longer than what's in the buffer, so the
	 * "tvb_get_ptr()" call won't throw an exception.
	 */
	first_linelen = tvb_find_line_end(tvb, offset, -1, &next_offset,
	    FALSE);
	line = tvb_get_ptr(tvb, offset, first_linelen);
	http_type = HTTP_OTHERS;	/* type not known yet */
	is_request_or_reply = is_http_request_or_reply(line, first_linelen,
	    &http_type, NULL, NULL);
	if (is_request_or_reply) {
		/*
		 * Yes, it's a request or response.
		 * Do header desegmentation if we've been told to.
		 *
		 * RFC 2616 defines HTTP messages as being either of the
		 * Request or the Response type
		 * (HTTP-message = Request | Response).
		 * Request and Response are defined as:
		 *     Request = Request-Line
		 *         *(( general-header
		 *         | request-header
		 *         | entity-header ) CRLF)
		 *         CRLF
		 *         [ message-body ]
		 *     Response = Status-Line
		 *         *(( general-header
		 *         | response-header
		 *         | entity-header ) CRLF)
		 *         CRLF
		 *         [ message-body ]
		 * that's why we can always assume two consecutive line
		 * endings (we allow CR, LF, or CRLF, as some clients
		 * or servers might not use a full CRLF) to mark the end
		 * of the headers.  The worst thing that would happen
		 * otherwise would be the packet not being desegmented
		 * or being interpreted as only headers.
		 */

		/*
		 * If header desegmentation is activated, check that all
		 * headers are in this tvbuff (search for an empty line
		 * marking end of headers) or request one more byte (we
		 * don't know how many bytes we'll need, so we just ask
		 * for one).
		 */
		if (http_desegment_headers && pinfo->can_desegment) {
			next_offset = offset;
			for (;;) {
				next_offset_sav = next_offset;

				/*
				 * Request one more byte if there're no
				 * bytes left.
				 */
				if (tvb_offset_exists(tvb, next_offset) == FALSE) {
					pinfo->desegment_offset = offset;
					pinfo->desegment_len = 1;
					return;
				}

				/*
				 * Request one more byte if we cannot find a
				 * header (i.e. a line end).
				 */
				linelen = tvb_find_line_end(tvb, next_offset,
				    -1, &next_offset, TRUE);
				if (linelen == -1) {
					/*
					 * Not enough data; ask for one more
					 * byte.
					 */
					pinfo->desegment_offset = offset;
					pinfo->desegment_len = 1;
					return;
				} else if (linelen == 0) {
					/*
					 * We found the end of the headers.
					 */
					break;
				}

				/*
				 * Is this a Content-Length header?
				 * If not, it either means that we are in
				 * a different header line, or that we are
				 * at the end of the headers, or that there
				 * isn't enough data; the two latter cases
				 * have already been handled above.
				 */
				if (http_desegment_body) {
					/*
					 * Check if we've found Content-Length.
					 */
					if (tvb_strneql(tvb, next_offset_sav,
					    "Content-Length:", 15) == 0) {
						if (sscanf(
						    tvb_get_string(tvb,
						        next_offset_sav + 15,
						        linelen - 15),
						    "%li", &content_length)
						    == 1)
							content_length_found = TRUE;
					}
				}
			}
		}
	}

	/*
	 * The above loop ends when we reached the end of the headers, so
	 * there should be content_length byte after the 4 terminating bytes
	 * and next_offset points to after the end of the headers.
	 */
	if (http_desegment_body && content_length_found) {
		/* next_offset has been set because content-length was found */
		if (!tvb_bytes_exist(tvb, next_offset, content_length)) {
			gint length = tvb_length_remaining(tvb, next_offset);
			if (length == -1)
				length = 0;
			pinfo->desegment_offset = offset;
			pinfo->desegment_len = content_length - length;
			return;
		}
	}

	stat_info = g_malloc(sizeof(http_info_value_t));
	stat_info->response_code = 0;
	stat_info->request_method = NULL;

	switch (pinfo->match_port) {

	case TCP_PORT_SSDP:	/* TCP_PORT_SSDP = UDP_PORT_SSDP */
		proto = PROTO_SSDP;
		proto_tag = "SSDP";
		break;

	default:
		proto = PROTO_HTTP;
		proto_tag = "HTTP";
		break;
	}

	if (check_col(pinfo->cinfo, COL_PROTOCOL))
		col_set_str(pinfo->cinfo, COL_PROTOCOL, proto_tag);
	if (check_col(pinfo->cinfo, COL_INFO)) {
		/*
		 * Put the first line from the buffer into the summary
		 * if it's an HTTP request or reply (but leave out the
		 * line terminator).
		 * Otherwise, just call it a continuation.
		 *
		 * Note that "tvb_find_line_end()" will return a value that
		 * is not longer than what's in the buffer, so the
		 * "tvb_get_ptr()" call won't throw an exception.
		 */
		line = tvb_get_ptr(tvb, offset, first_linelen);
		if (is_request_or_reply)
			col_add_str(pinfo->cinfo, COL_INFO,
			    format_text(line, first_linelen));
		else
			col_set_str(pinfo->cinfo, COL_INFO, "Continuation");
	}

	if (tree) {
		ti = proto_tree_add_item(tree, proto_http, tvb, offset, -1,
		    FALSE);
		http_tree = proto_item_add_subtree(ti, ett_http);
	}

	/*
	 * Process the packet data, a line at a time.
	 */
	http_type = HTTP_OTHERS;	/* type not known yet */
	entity_headers.content_type = NULL;	/* content type not known yet */
	while (tvb_offset_exists(tvb, offset)) {
		/*
		 * Find the end of the line.
		 */
		linelen = tvb_find_line_end(tvb, offset, -1, &next_offset,
		    FALSE);

		/*
		 * Get a buffer that refers to the line.
		 */
		line = tvb_get_ptr(tvb, offset, linelen);
		lineend = line + linelen;
		colon_offset = -1;

		/*
		 * OK, does it look like an HTTP request or response?
		 */
		req_dissector = NULL;
		is_request_or_reply = is_http_request_or_reply(line, linelen,
		    &http_type, &req_dissector, &req_strlen);
		if (is_request_or_reply)
			goto is_http;

		/*
		 * No.  Does it look like a blank line (as would appear
		 * at the end of an HTTP request)?
		 */
		if (linelen == 0)
			goto is_http;

		/*
		 * No.  Does it look like a MIME header?
		 */
		linep = line;
		colon_offset = offset;
		while (linep < lineend) {
			c = *linep++;

			/*
			 * This must be a CHAR to be part of a token; that
			 * means it must be ASCII.
			 */
			if (!isascii(c))
				break;	/* not ASCII, thus not a CHAR */

			/*
			 * This mustn't be a CTL to be part of a token;
			 * that means it must be printable.
			 */
			if (!isprint(c))
				break;	/* not printable, not a MIME header */

			/*
			 * This mustn't be a SEP to be part of a token;
			 * a ':' ends the token, everything else is an
			 * indication that this isn't a header.
			 */
			switch (c) {

			case '(':
			case ')':
			case '<':
			case '>':
			case '@':
			case ',':
			case ';':
			case '\\':
			case '"':
			case '/':
			case '[':
			case ']':
			case '?':
			case '=':
			case '{':
			case '}':
			case ' ':
				/*
				 * It's a separator, so it's not part of a
				 * token, so it's not a field name for the
				 * beginning of a MIME header.
				 *
				 * (We don't have to check for HT; that's
				 * already been ruled out by "isprint()".)
				 */
				goto not_http;

			case ':':
				/*
				 * This ends the token; we consider this
				 * to be a MIME header.
				 */
				goto is_http;

			default:
				colon_offset++;
				break;
			}
		}

	not_http:
		/*
		 * We don't consider this part of an HTTP request or
		 * reply, so we don't display it.
		 * (Yeah, that means we don't display, say, a text/http
		 * page, but you can get that from the data pane.)
		 */
		break;

	is_http:
		/*
		 * Process this line.
		 */
		if (is_request_or_reply) {
			if (tree) {
				hdr_item = proto_tree_add_text(http_tree, tvb,
				    offset, next_offset - offset, "%s",
				    tvb_format_text(tvb, offset,
				      next_offset - offset));
				if (req_dissector) {
					req_tree = proto_item_add_subtree(
					    hdr_item, ett_http_request);
					req_dissector(tvb, req_tree,
					    req_strlen);
				}
			}
		} else if (linelen != 0) {
			/*
			 * Entity header.
			 */
			process_entity_header(tvb, offset, next_offset,
			    line, linelen, colon_offset, pinfo, http_tree,
			    &entity_headers);
		} else {
			/*
			 * Blank line.
			 */
			proto_tree_add_text(http_tree, tvb,
			    offset, next_offset - offset, "%s",
			    tvb_format_text(tvb, offset,
			      next_offset - offset));
		}
		offset = next_offset;
	}

	if (tree) {
		switch (http_type) {

		case HTTP_NOTIFICATION:
			proto_tree_add_boolean_hidden(http_tree,
			    hf_http_notification, tvb, 0, 0, 1);
			break;

		case HTTP_RESPONSE:
			proto_tree_add_boolean_hidden(http_tree,
			    hf_http_response, tvb, 0, 0, 1);
			break;

		case HTTP_REQUEST:
			proto_tree_add_boolean_hidden(http_tree,
			    hf_http_request, tvb, 0, 0, 1);
			break;

		case HTTP_OTHERS:
		default:
			break;
		}
	}

	datalen = tvb_length_remaining(tvb, offset);
	if (datalen > 0) {
		tvbuff_t *next_tvb = tvb_new_subset(tvb, offset, -1, -1);

		/*
		 * Do subdissector checks.
		 *
		 * First, check whether some subdissector asked that they
		 * be called if something was on some particular port.
		 */
		handle = dissector_get_port_handle(port_subdissector_table,
		    pinfo->match_port);
		if (handle == NULL && entity_headers.content_type != NULL) {
			/*
			 * We didn't find any subdissector that
			 * registered for the port, and we have a
			 * Content-Type value.  Is there any subdissector
			 * for that content type?
			 */
			handle = dissector_get_string_handle(
			    content_type_subdissector_table,
			    entity_headers.content_type);
		}
		g_free(entity_headers.content_type);
		if (handle != NULL) {
			/*
			 * We have a subdissector - call it.
			 */
			dissected = call_dissector(handle, next_tvb, pinfo,
			    tree);
		} else {
			/*
			 * We don't have a subdissector - try the heuristic
			 * subdissectors.
			 */
			dissected = dissector_try_heuristic(
			    heur_subdissector_list, next_tvb, pinfo, tree);
		}
		if (dissected) {
			/*
			 * The subdissector dissected the body.
			 * Fix up the top-level item so that it doesn't
			 * include the stuff for that protocol.
			 */
			if (ti != NULL)
				proto_item_set_len(ti, offset);
		} else {
			call_dissector(data_handle, next_tvb, pinfo,
			    http_tree);
		}
	} else {
		/*
		 * Free up content type - there's no content.
		 */
		g_free(entity_headers.content_type);
	}

	tap_queue_packet(http_tap, pinfo, stat_info);
}

/* This can be used to dissect an HTTP request until such time
 * that a more complete dissector is written for that HTTP request.
 * This simple dissectory only puts http.request_method into a sub-tree.
 */
static void
basic_request_dissector(tvbuff_t *tvb, proto_tree *tree, int req_strlen)
{
	proto_tree_add_item(tree, hf_http_request_method, tvb, 0, req_strlen, FALSE);
}

static void
basic_response_dissector(tvbuff_t *tvb, proto_tree *tree, int req_strlen _U_)
{
	const guchar *data;
	int minor, major, status_code;

	data = tvb_get_ptr(tvb, 5, 12);
	if (sscanf(data, "%d.%d %d", &minor, &major, &status_code) == 3) {
		proto_tree_add_uint(tree, hf_http_response_code, tvb, 9, 3, status_code);
		stat_info->response_code = status_code;
	}
}

/*
 * XXX - this won't handle HTTP 0.9 replies, but they're all data
 * anyway.
 */
static int
is_http_request_or_reply(const guchar *data, int linelen, http_type_t *type,
		RequestDissector *req_dissector, int *req_strlen)
{
	int isHttpRequestOrReply = FALSE;
	int prefix_len = 0;

	/*
	 * From RFC 2774 - An HTTP Extension Framework
	 *
	 * Support the command prefix that identifies the presence of
	 * a "mandatory" header.
	 */
	if (linelen >= 2 && strncmp(data, "M-", 2) == 0) {
		data += 2;
		linelen -= 2;
		prefix_len = 2;
	}

	/*
	 * From draft-cohen-gena-client-01.txt, available from the uPnP forum:
	 *	NOTIFY, SUBSCRIBE, UNSUBSCRIBE
	 *
	 * From draft-ietf-dasl-protocol-00.txt, a now vanished Microsoft draft:
	 *	SEARCH
	 */
	if (linelen >= 5 && strncmp(data, "HTTP/", 5) == 0) {
		*type = HTTP_RESPONSE;
		isHttpRequestOrReply = TRUE;	/* response */
		if (req_dissector) {
			*req_dissector = basic_response_dissector;
			*req_strlen = linelen - 5;
		}
	} else {
		const guchar * ptr = (const guchar *)data;
		int		 index = 0;

		/* Look for the space following the Method */
		while (index < linelen) {
			if (*ptr == ' ')
				break;
			else {
				ptr++;
				index++;
			}
		}

		/* Check the methods that have same length */
		switch (index) {

		case 3:
			if (strncmp(data, "GET", index) == 0 ||
			    strncmp(data, "PUT", index) == 0) {
				*type = HTTP_REQUEST;
				isHttpRequestOrReply = TRUE;
			}
			break;

		case 4:
			if (strncmp(data, "COPY", index) == 0 ||
			    strncmp(data, "HEAD", index) == 0 ||
			    strncmp(data, "LOCK", index) == 0 ||
			    strncmp(data, "MOVE", index) == 0 ||
			    strncmp(data, "POLL", index) == 0 ||
			    strncmp(data, "POST", index) == 0) {
				*type = HTTP_REQUEST;
				isHttpRequestOrReply = TRUE;
			}
			break;

		case 5:
			if (strncmp(data, "BCOPY", index) == 0 ||
				strncmp(data, "BMOVE", index) == 0 ||
				strncmp(data, "MKCOL", index) == 0 ||
				strncmp(data, "TRACE", index) == 0) {
				*type = HTTP_REQUEST;
				isHttpRequestOrReply = TRUE;
			}
			break;

		case 6:
			if (strncmp(data, "DELETE", index) == 0 ||
				strncmp(data, "SEARCH", index) == 0 ||
				strncmp(data, "UNLOCK", index) == 0) {
				*type = HTTP_REQUEST;
				isHttpRequestOrReply = TRUE;
			}
			else if (strncmp(data, "NOTIFY", index) == 0) {
				*type = HTTP_NOTIFICATION;
				isHttpRequestOrReply = TRUE;
			}
			break;

		case 7:
			if (strncmp(data, "BDELETE", index) == 0 ||
			    strncmp(data, "CONNECT", index) == 0 ||
			    strncmp(data, "OPTIONS", index) == 0) {
				*type = HTTP_REQUEST;
				isHttpRequestOrReply = TRUE;
			}
			break;

		case 8:
			if (strncmp(data, "PROPFIND", index) == 0) {
				*type = HTTP_REQUEST;
				isHttpRequestOrReply = TRUE;
			}
			break;

		case 9:
			if (strncmp(data, "SUBSCRIBE", index) == 0) {
				*type = HTTP_NOTIFICATION;
				isHttpRequestOrReply = TRUE;
			} else if (strncmp(data, "PROPPATCH", index) == 0 ||
			    strncmp(data, "BPROPFIND", index) == 0) {
				*type = HTTP_REQUEST;
				isHttpRequestOrReply = TRUE;
			}
			break;

		case 10:
			if (strncmp(data, "BPROPPATCH", index) == 0) {
				*type = HTTP_REQUEST;
				isHttpRequestOrReply = TRUE;
			}
			break;

		case 11:
			if (strncmp(data, "UNSUBSCRIBE", index) == 0) {
				*type = HTTP_NOTIFICATION;
				isHttpRequestOrReply = TRUE;
			}
			break;

		default:
			break;
		}

		if (isHttpRequestOrReply && req_dissector) {
			*req_dissector = basic_request_dissector;
			*req_strlen = index + prefix_len;
		}
		if (isHttpRequestOrReply && req_dissector) {
			if (!stat_info->request_method)
				stat_info->request_method = g_malloc( index+1 );
				strncpy( stat_info->request_method, data, index);
				stat_info->request_method[index] = '\0';
		}
	}

	return isHttpRequestOrReply;
}

/*
 * Process entity-headers.
 */
typedef struct {
	char	*name;
	gint	*hf;
	int	special;
} entity_header_info;

#define EH_NO_SPECIAL		0
#define EH_AUTHORIZATION	1
#define EH_AUTHENTICATE		2
#define EH_CONTENT_TYPE		3

static const entity_header_info headers[] = {
	{ "Authorization", &hf_http_authorization, EH_AUTHORIZATION },
	{ "Proxy-Authorization", &hf_http_proxy_authorization, EH_AUTHORIZATION },
	{ "Proxy-Authenticate", &hf_http_proxy_authenticate, EH_AUTHENTICATE },
	{ "WWW-Authenticate", &hf_http_www_authenticate, EH_AUTHENTICATE },
	{ "Content-Type", &hf_http_content_type, EH_CONTENT_TYPE },
};

static void
process_entity_header(tvbuff_t *tvb, int offset, int next_offset,
    const guchar *line, int linelen, int colon_offset,
    packet_info *pinfo, proto_tree *tree, entity_headers_t *eh_ptr)
{
	int len;
	int line_end_offset;
	int header_len;
	gint hf_index;
	guchar c;
	int value_offset;
	int value_len;
	char *value;
	proto_item *hdr_item;
	int i;

	len = next_offset - offset;
	line_end_offset = offset + linelen;
	header_len = colon_offset - offset;
	hf_index = find_header_hf_value(tvb, offset, header_len);

	if (hf_index == -1) {
		/*
		 * Not a header we know anything about.  Just put it into
		 * the tree as text.
		 */
		if (tree) {
			proto_tree_add_text(tree, tvb, offset, len,
			    "%s", format_text(line, len));
		}
	} else {
		/*
		 * Skip whitespace after the colon.
		 */
		value_offset = colon_offset + 1;
		while (value_offset < line_end_offset
		    && ((c = line[value_offset - offset]) == ' ' || c == '\t'))
			value_offset++;

		/*
		 * Fetch the value.
		 */
		value_len = line_end_offset - value_offset;
		value = g_malloc(value_len + 1);
		memcpy(value, &line[value_offset - offset], value_len);
		value[value_len] = '\0';
		CLEANUP_PUSH(g_free, value);

		/*
		 * Add it to the protocol tree as a particular field,
		 * but display the line as is.
		 */
		if (tree) {
			hdr_item = proto_tree_add_string_format(tree,
			    *headers[hf_index].hf, tvb, offset, len,
			    value, "%s", format_text(line, len));
		} else
			hdr_item = NULL;

		/*
		 * Do any special processing that particular headers
		 * require.
		 */
		switch (headers[hf_index].special) {

		case EH_AUTHORIZATION:
			if (check_auth_ntlmssp(hdr_item, tvb, pinfo, value))
				break;	/* dissected NTLMSSP */
			check_auth_basic(hdr_item, tvb, value);
			break;

		case EH_AUTHENTICATE:
			check_auth_ntlmssp(hdr_item, tvb, pinfo, value);
			break;

		case EH_CONTENT_TYPE:
			if (eh_ptr->content_type != NULL)
				g_free(eh_ptr->content_type);
			eh_ptr->content_type = g_malloc(value_len + 1);
			for (i = 0; i < value_len; i++) {
				c = value[i];
				if (c == ';' || isspace(c)) {
					/*
					 * End of subtype - either
					 * white space or a ";"
					 * separating the subtype from
					 * a parameter.
					 */
					break;
				}

				/*
				 * Map the character to lower case;
				 * content types are case-insensitive.
				 */
				eh_ptr->content_type[i] = tolower(c);
			}
			eh_ptr->content_type[i] = '\0';
			break;
		}

		/*
		 * Free the value, by calling and popping the cleanup
		 * handler for it.
		 */
		CLEANUP_CALL_AND_POP;
	}
}

/* Returns index of entity-header tag in headers */
static gint
find_header_hf_value(tvbuff_t *tvb, int offset, guint header_len)
{
        guint i;

        for (i = 0; i < array_length(headers); i++) {
                if (header_len == strlen(headers[i].name) &&
                    tvb_strncaseeql(tvb, offset, headers[i].name, header_len) == 0)
                        return i;
        }

        return -1;
}

/*
 * Dissect Microsoft's abomination called NTLMSSP over HTTP.
 */
static gboolean
check_auth_ntlmssp(proto_item *hdr_item, tvbuff_t *tvb, packet_info *pinfo,
    gchar *value)
{
	static const char *ntlm_headers[] = {
		"NTLM ",
		"Negotiate ",
		NULL
	};
	const char **header;
	size_t hdrlen;
	proto_tree *hdr_tree;

	/*
	 * Check for NTLM credentials and challenge; those can
	 * occur with WWW-Authenticate.
	 */
	for (header = &ntlm_headers[0]; *header != NULL; header++) {
		hdrlen = strlen(*header);
		if (strncmp(value, *header, hdrlen) == 0) {
			if (hdr_item != NULL) {
				hdr_tree = proto_item_add_subtree(hdr_item,
				    ett_http_ntlmssp);
			} else
				hdr_tree = NULL;
			value += hdrlen;
			dissect_http_ntlmssp(tvb, pinfo, hdr_tree, value);
			return TRUE;
		}
	}
	return FALSE;
}

/*
 * Dissect HTTP Basic authorization.
 */
static gboolean
check_auth_basic(proto_item *hdr_item, tvbuff_t *tvb, gchar *value)
{
	static const char *basic_headers[] = {
		"Basic ",
		NULL
	};
	const char **header;
	size_t hdrlen;
	proto_tree *hdr_tree;
	size_t len;

	for (header = &basic_headers[0]; *header != NULL; header++) {
		hdrlen = strlen(*header);
		if (strncmp(value, *header, hdrlen) == 0) {
			if (hdr_item != NULL) {
				hdr_tree = proto_item_add_subtree(hdr_item,
				    ett_http_ntlmssp);
			} else
				hdr_tree = NULL;
			value += hdrlen;

			len = base64_decode(value);
			value[len] = '\0';
			proto_tree_add_string(hdr_tree, hf_http_basic, tvb,
			    0, 0, value);

			return TRUE;
		}
	}
	return FALSE;
}

void
proto_register_http(void)
{
	static hf_register_info hf[] = {
	    { &hf_http_notification,
	      { "Notification",		"http.notification",
		FT_BOOLEAN, BASE_NONE, NULL, 0x0,
		"TRUE if HTTP notification", HFILL }},
	    { &hf_http_response,
	      { "Response",		"http.response",
		FT_BOOLEAN, BASE_NONE, NULL, 0x0,
		"TRUE if HTTP response", HFILL }},
	    { &hf_http_request,
	      { "Request",		"http.request",
		FT_BOOLEAN, BASE_NONE, NULL, 0x0,
		"TRUE if HTTP request", HFILL }},
	    { &hf_http_basic,
	      { "Credentials",		"http.authbasic",
		FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL }},
	    { &hf_http_request_method,
	      { "Request Method",	"http.request.method",
		FT_STRING, BASE_NONE, NULL, 0x0,
		"HTTP Request Method", HFILL }},
	    { &hf_http_response_code,
	      { "Response Code",	"http.response.code",
		FT_UINT16, BASE_DEC, NULL, 0x0,
		"HTTP Response Code", HFILL }},
	    { &hf_http_authorization,
	      { "Authorization",	"http.authorization",
		FT_STRING, BASE_NONE, NULL, 0x0,
		"HTTP Authorization header", HFILL }},
	    { &hf_http_proxy_authenticate,
	      { "Proxy-Authenticate",	"http.proxy_authenticate",
		FT_STRING, BASE_NONE, NULL, 0x0,
		"HTTP Proxy-Authenticate header", HFILL }},
	    { &hf_http_proxy_authorization,
	      { "Proxy-Authorization",	"http.proxy_authorization",
		FT_STRING, BASE_NONE, NULL, 0x0,
		"HTTP Proxy-Authorization header", HFILL }},
	    { &hf_http_www_authenticate,
	      { "WWW-Authenticate",	"http.www_authenticate",
		FT_STRING, BASE_NONE, NULL, 0x0,
		"HTTP WWW-Authenticate header", HFILL }},
	    { &hf_http_content_type,
	      { "Content-Type",	"http.content_type",
		FT_STRING, BASE_NONE, NULL, 0x0,
		"HTTP Content-Type header", HFILL }},
	};
	static gint *ett[] = {
		&ett_http,
		&ett_http_ntlmssp,
		&ett_http_request,
	};
	module_t *http_module;

	proto_http = proto_register_protocol("Hypertext Transfer Protocol",
	    "HTTP", "http");
	proto_register_field_array(proto_http, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));
	http_module = prefs_register_protocol(proto_http, NULL);
	prefs_register_bool_preference(http_module, "desegment_http_headers",
	    "Desegment all HTTP headers spanning multiple TCP segments",
	    "Whether the HTTP dissector should desegment all headers "
	    "of a request spanning multiple TCP segments",
	    &http_desegment_headers);
	prefs_register_bool_preference(http_module, "desegment_http_body",
	    "Trust the \"Content-length:\" header and desegment HTTP "
	    "bodies spanning multiple TCP segments",
	    "Whether the HTTP dissector should use the "
	    "\"Content-length:\" value to desegment the body "
	    "of a request spanning multiple TCP segments",
	    &http_desegment_body);

	register_dissector("http", dissect_http, proto_http);
	http_handle = find_dissector("http");

	/*
	 * Dissectors shouldn't register themselves in this table;
	 * instead, they should call "http_dissector_add()", and
	 * we'll register the port number they specify as a port
	 * for HTTP, and register them in our subdissector table.
	 *
	 * This only works for protocols such as IPP that run over
	 * HTTP on a specific non-HTTP port.
	 */
	port_subdissector_table = register_dissector_table("http.port",
	    "TCP port for protocols using HTTP", FT_UINT16, BASE_DEC);

	/*
	 * Dissectors can register themselves in this table.
	 * It's just "media_type", not "http.content_type", because
	 * it's an Internet media type, usable by other protocols as well.
	 */
	content_type_subdissector_table =
	    register_dissector_table("media_type",
		"Internet media type", FT_STRING, BASE_NONE);

	/*
	 * Heuristic dissectors SHOULD register themselves in
	 * this table using the standard heur_dissector_add()
	 * function.
	 */
	register_heur_dissector_list("http", &heur_subdissector_list);

	/*
	 * Register for tapping
	 */
	http_tap = register_tap("http");
}

/*
 * Called by dissectors for protocols that run atop HTTP/TCP.
 */
void
http_dissector_add(guint32 port, dissector_handle_t handle)
{
	/*
	 * Register ourselves as the handler for that port number
	 * over TCP.
	 */
	dissector_add("tcp.port", port, http_handle);

	/*
	 * And register them in *our* table for that port.
	 */
	dissector_add("http.port", port, handle);
}

void
proto_reg_handoff_http(void)
{
	data_handle = find_dissector("data");

	dissector_add("tcp.port", TCP_PORT_HTTP, http_handle);
	dissector_add("tcp.port", TCP_ALT_PORT_HTTP, http_handle);
	dissector_add("tcp.port", TCP_PORT_PROXY_HTTP, http_handle);
	dissector_add("tcp.port", TCP_PORT_PROXY_ADMIN_HTTP, http_handle);

	/*
	 * XXX - is there anything to dissect in the body of an SSDP
	 * request or reply?  I.e., should there be an SSDP dissector?
	 */
	dissector_add("tcp.port", TCP_PORT_SSDP, http_handle);
	dissector_add("udp.port", UDP_PORT_SSDP, http_handle);

	ntlmssp_handle = find_dissector("ntlmssp");
}
