/* packet-rtsp.c
 * Routines for RTSP packet disassembly (RFC 2326)
 *
 * Jason Lango <jal@netapp.com>
 * Liberally copied from packet-http.c, by Guy Harris <guy@alum.mit.edu>
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
 *
 * References:
 * RTSP is defined in RFC 2326, http://www.ietf.org/rfc/rfc2326.txt?number=2326
 * http://www.iana.org/assignments/rsvp-parameters
 */

#include "config.h"

#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include <epan/prefs.h>

#include <glib.h>
#include <epan/packet.h>
#include <epan/req_resp_hdrs.h>
#include "packet-rtp.h"
#include "packet-rtcp.h"
#include <epan/conversation.h>
#include <epan/strutil.h>
#include "packet-e164.h"

static int proto_rtsp		= -1;

static gint ett_rtsp		= -1;
static gint ett_rtspframe	= -1;
static gint ett_rtsp_method 	= -1;

static int hf_rtsp_method	= -1;
static int hf_rtsp_url		= -1;
static int hf_rtsp_status	= -1;
static int hf_rtsp_session	= -1;
static int hf_rtsp_X_Vig_Msisdn	= -1;

static dissector_handle_t sdp_handle;
static dissector_handle_t rtp_handle;
static dissector_handle_t rtcp_handle;

void proto_reg_handoff_rtsp(void);

static GMemChunk *rtsp_vals = NULL;
#define rtsp_hash_init_count 20

/*
 * desegmentation of RTSP headers
 * (when we are over TCP or another protocol providing the desegmentation API)
 */
static gboolean rtsp_desegment_headers = FALSE;

/*
 * desegmentation of RTSP bodies
 * (when we are over TCP or another protocol providing the desegmentation API)
 * TODO let the user filter on content-type the bodies he wants desegmented
 */
static gboolean rtsp_desegment_body = FALSE;

/* http://www.iana.org/assignments/port-numberslists two rtsp ports */
#define TCP_PORT_RTSP			554
#define TCP_ALTERNATE_PORT_RTSP		8554
static guint global_rtsp_tcp_port = TCP_PORT_RTSP;
static guint global_rtsp_tcp_alternate_port = TCP_ALTERNATE_PORT_RTSP;
/*
* Variables to allow for proper deletion of dissector registration when
* the user changes port from the gui.
*/
static guint tcp_port = 0;
static guint tcp_alternate_port = 0;

/*
 * Takes an array of bytes, assumed to contain a null-terminated
 * string, as an argument, and returns the length of the string -
 * i.e., the size of the array, minus 1 for the null terminator.
 */
#define STRLEN_CONST(str)	(sizeof (str) - 1)

#define RTSP_FRAMEHDR	('$')

typedef struct {
	dissector_handle_t		dissector;
} rtsp_interleaved_t;

#define RTSP_MAX_INTERLEAVED		(8)

/*
 * Careful about dynamically allocating memory in this structure (say
 * for dynamically increasing the size of the 'interleaved' array) -
 * the containing structure is garbage collected and contained
 * pointers will not be freed.
 */
typedef struct {
	rtsp_interleaved_t		interleaved[RTSP_MAX_INTERLEAVED];
} rtsp_conversation_data_t;

static int
dissect_rtspinterleaved(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree)
{
	guint		length_remaining;
	proto_item	*ti;
	proto_tree	*rtspframe_tree = NULL;
	int		orig_offset;
	guint8		rf_start;		/* always RTSP_FRAMEHDR */
	guint8		rf_chan;        /* interleaved channel id */
	guint16		rf_len;         /* packet length */
	tvbuff_t	*next_tvb;
	conversation_t	*conv;
	rtsp_conversation_data_t	*data;
	dissector_handle_t		dissector;

	/*
	 * This will throw an exception if we don't have any data left.
	 * That's what we want.  (See "tcp_dissect_pdus()", which is
	 * similar.)
	 */
	length_remaining = tvb_ensure_length_remaining(tvb, offset);

	/*
	 * Can we do reassembly?
	 */
	if (rtsp_desegment_headers && pinfo->can_desegment) {
		/*
		 * Yes - would an RTSP multiplexed header starting at
		 * this offset be split across segment boundaries?
		 */
		if (length_remaining < 4) {
			/*
			 * Yes.  Tell the TCP dissector where the data
			 * for this message starts in the data it handed
			 * us, and how many more bytes we need, and return.
			 */
			pinfo->desegment_offset = offset;
			pinfo->desegment_len = 4 - length_remaining;
			return -1;
		}
	}

	/*
	 * Get the "$", channel, and length from the header.
	 */
	orig_offset = offset;
	rf_start = tvb_get_guint8(tvb, offset);
	rf_chan = tvb_get_guint8(tvb, offset+1);
	rf_len = tvb_get_ntohs(tvb, offset+2);

	/*
	 * Can we do reassembly?
	 */
	if (rtsp_desegment_body && pinfo->can_desegment) {
		/*
		 * Yes - is the header + encapsulated packet split
		 * across segment boundaries?
		 */
		if (length_remaining < 4U + rf_len) {
			/*
			 * Yes.  Tell the TCP dissector where the data
			 * for this message starts in the data it handed
			 * us, and how many more bytes we need, and return.
			 */
			pinfo->desegment_offset = offset;
			pinfo->desegment_len = 4U + rf_len - length_remaining;
			return -1;
		}
	}

	if (check_col(pinfo->cinfo, COL_INFO))
		col_add_fstr(pinfo->cinfo, COL_INFO,
			"Interleaved channel 0x%02x, %u bytes",
			rf_chan, rf_len);

	if (tree != NULL) {
		ti = proto_tree_add_protocol_format(tree, proto_rtsp, tvb,
		    offset, 4,
		    "RTSP Interleaved Frame, Channel: 0x%02x, %u bytes",
		    rf_chan, rf_len);
		rtspframe_tree = proto_item_add_subtree(ti, ett_rtspframe);

		proto_tree_add_text(rtspframe_tree, tvb, offset, 1,
		    "Magic: 0x%02x",
		    rf_start);
	}
	offset += 1;

	if (tree != NULL) {
		proto_tree_add_text(rtspframe_tree, tvb, offset, 1,
		    "Channel: 0x%02x",
		    rf_chan);
	}
	offset += 1;

	if (tree != NULL) {
		proto_tree_add_text(rtspframe_tree, tvb, offset, 2,
		    "Length: %u bytes",
		    rf_len);
	}
	offset += 2;

	/*
	 * We set the actual length of the tvbuff for the interleaved
	 * stuff to the minimum of what's left in the tvbuff and the
	 * length in the header.
	 *
	 * XXX - what if there's nothing left in the tvbuff?
	 * We'd want a BoundsError exception to be thrown, so
	 * that a Short Frame would be reported.
	 */
	if (length_remaining > rf_len)
		length_remaining = rf_len;
	next_tvb = tvb_new_subset(tvb, offset, length_remaining, rf_len);

	conv = find_conversation(&pinfo->src, &pinfo->dst, pinfo->ptype,
		pinfo->srcport, pinfo->destport, 0);

	if (conv &&
	    (data = conversation_get_proto_data(conv, proto_rtsp)) &&
	    rf_chan < RTSP_MAX_INTERLEAVED &&
	    (dissector = data->interleaved[rf_chan].dissector)) {
		call_dissector(dissector, next_tvb, pinfo, tree);
	} else {
		proto_tree_add_text(rtspframe_tree, tvb, offset, rf_len,
			"Data (%u bytes)", rf_len);
	}

	offset += rf_len;

	return offset - orig_offset;
}

static void process_rtsp_request(tvbuff_t *tvb, int offset, const guchar *data,
	size_t linelen, proto_tree *tree);

static void process_rtsp_reply(tvbuff_t *tvb, int offset, const guchar *data,
	size_t linelen, proto_tree *tree);

typedef enum {
	RTSP_REQUEST,
	RTSP_REPLY,
	NOT_RTSP
} rtsp_type_t;

static const char *rtsp_methods[] = {
	"DESCRIBE",
	"ANNOUNCE",
	"GET_PARAMETER",
	"OPTIONS",
	"PAUSE",
	"PLAY",
	"RECORD",
	"REDIRECT",
	"SETUP",
	"SET_PARAMETER",
	"TEARDOWN"
};

#define RTSP_NMETHODS	(sizeof rtsp_methods / sizeof rtsp_methods[0])

static gboolean
is_rtsp_request_or_reply(const guchar *line, size_t linelen, rtsp_type_t *type)
{
	unsigned	ii;

	/* Is this an RTSP reply? */
	if (linelen >= 5 && strncasecmp("RTSP/", line, 5) == 0) {
		/*
		 * Yes.
		 */
		*type = RTSP_REPLY;
		return TRUE;
	}

	/*
	 * Is this an RTSP request?
	 * Check whether the line begins with one of the RTSP request
	 * methods.
	 */
	for (ii = 0; ii < RTSP_NMETHODS; ii++) {
		size_t len = strlen(rtsp_methods[ii]);
		if (linelen >= len &&
		    strncasecmp(rtsp_methods[ii], line, len) == 0 &&
		    (len == linelen || isspace(line[len]))) {
			*type = RTSP_REQUEST;
			return TRUE;
		}
	}
	*type = NOT_RTSP;
	return FALSE;
}

static const char rtsp_content_type[] = "Content-Type:";

static int
is_content_sdp(const guchar *line, size_t linelen)
{
	static const char type[] = "application/sdp";
	size_t		typelen = STRLEN_CONST(type);

	line += STRLEN_CONST(rtsp_content_type);
	linelen -= STRLEN_CONST(rtsp_content_type);
	while (linelen > 0 && (*line == ' ' || *line == '\t')) {
		line++;
		linelen--;
	}

	if (linelen < typelen || strncasecmp(type, line, typelen))
		return FALSE;

	line += typelen;
	linelen -= typelen;
	if (linelen > 0 && !isspace(*line))
		return FALSE;

	return TRUE;
}

static const char rtsp_transport[] = "Transport:";
static const char rtsp_sps[] = "server_port=";
static const char rtsp_cps[] = "client_port=";
static const char rtsp_rtp[] = "rtp/";
static const char rtsp_inter[] = "interleaved=";

static void
rtsp_create_conversation(packet_info *pinfo, const guchar *line_begin,
	size_t line_len)
{
	conversation_t	*conv;
	guchar		buf[256];
	guchar		*tmp;
	guint		c_data_port, c_mon_port;
	guint		s_data_port, s_mon_port;

	if (line_len > sizeof(buf) - 1) {
		/*
		 * Don't overflow the buffer.
		 */
		line_len = sizeof(buf) - 1;
	}
	memcpy(buf, line_begin, line_len);
	buf[line_len] = '\0';

	tmp = buf + STRLEN_CONST(rtsp_transport);
	while (*tmp && isspace(*tmp))
		tmp++;
	if (strncasecmp(tmp, rtsp_rtp, strlen(rtsp_rtp)) != 0) {
		g_warning("Frame %u: rtsp: unknown transport", pinfo->fd->num);
		return;
	}

	c_data_port = c_mon_port = 0;
	s_data_port = s_mon_port = 0;
	if ((tmp = strstr(buf, rtsp_sps))) {
		tmp += strlen(rtsp_sps);
		if (sscanf(tmp, "%u-%u", &s_data_port, &s_mon_port) < 1) {
			g_warning("Frame %u: rtsp: bad server_port",
				pinfo->fd->num);
			return;
		}
	}
	if ((tmp = strstr(buf, rtsp_cps))) {
		tmp += strlen(rtsp_cps);
		if (sscanf(tmp, "%u-%u", &c_data_port, &c_mon_port) < 1) {
			g_warning("Frame %u: rtsp: bad client_port",
				pinfo->fd->num);
			return;
		}
	}
	if (!c_data_port) {
		rtsp_conversation_data_t	*data;
		guint				s_data_chan, s_mon_chan;
		int				i;

		/*
		 * Deal with RTSP TCP-interleaved conversations.
		 */
		if ((tmp = strstr(buf, rtsp_inter)) == NULL) {
			/*
			 * No interleaved or server_port - probably a
			 * SETUP request, rather than reply.
			 */
			return;
		}
		tmp += strlen(rtsp_inter);
		i = sscanf(tmp, "%u-%u", &s_data_chan, &s_mon_chan);
		if (i < 1) {
			g_warning("Frame %u: rtsp: bad interleaved",
				pinfo->fd->num);
			return;
		}
		conv = find_conversation(&pinfo->src, &pinfo->dst, pinfo->ptype,
			pinfo->srcport, pinfo->destport, 0);
		if (!conv) {
			conv = conversation_new(&pinfo->src, &pinfo->dst,
				pinfo->ptype, pinfo->srcport, pinfo->destport,
				0);
		}
		data = conversation_get_proto_data(conv, proto_rtsp);
		if (!data) {
			data = g_mem_chunk_alloc(rtsp_vals);
			conversation_add_proto_data(conv, proto_rtsp, data);
		}
		if (s_data_chan < RTSP_MAX_INTERLEAVED) {
			data->interleaved[s_data_chan].dissector =
				rtp_handle;
		}
		if (i > 1 && s_mon_chan < RTSP_MAX_INTERLEAVED) {
			data->interleaved[s_mon_chan].dissector =
				rtcp_handle;
		}
		return;
	}

	/*
	 * We only want to match on the destination address, not the
	 * source address, because the server might send back a packet
	 * from an address other than the address to which its client
	 * sent the packet, so we construct a conversation with no
	 * second address.
	 */
	rtp_add_address(pinfo, &pinfo->dst, c_data_port, s_data_port,
                    "RTSP", pinfo->fd->num);

	if (!c_mon_port)
		return;

	rtcp_add_address(pinfo, &pinfo->dst, c_mon_port, s_mon_port,
                     "RTSP", pinfo->fd->num);
}

static const char rtsp_content_length[] = "Content-Length:";

static int
rtsp_get_content_length(const guchar *line_begin, size_t line_len)
{
	guchar		buf[256];
	guchar		*tmp;
	long		content_length;
	char		*p;
	guchar		*up;

	if (line_len > sizeof(buf) - 1) {
		/*
		 * Don't overflow the buffer.
		 */
		line_len = sizeof(buf) - 1;
	}
	memcpy(buf, line_begin, line_len);
	buf[line_len] = '\0';

	tmp = buf + STRLEN_CONST(rtsp_content_length);
	while (*tmp && isspace(*tmp))
		tmp++;
	content_length = strtol(tmp, &p, 10);
	up = p;
	if (up == tmp || (*up != '\0' && !isspace(*up)))
		return -1;	/* not a valid number */
	return content_length;
}

static const char rtsp_Session[] = "Session:";
static const char rtsp_X_Vig_Msisdn[] = "X-Vig-Msisdn";

static int
dissect_rtspmessage(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree)
{
	proto_tree		*rtsp_tree = NULL;
	proto_tree		*sub_tree = NULL;
	proto_item		*ti = NULL;
	const guchar		*line;
	gint			next_offset;
	const guchar		*linep, *lineend;
	int			orig_offset;
	int			first_linelen, linelen;
	int			line_end_offset;
	int			colon_offset;
	gboolean		is_request_or_reply;
	gboolean		body_requires_content_len;
	gboolean		saw_req_resp_or_header;
	guchar			c;
	rtsp_type_t		rtsp_type;
	gboolean		is_header;
	int			is_sdp = FALSE;
	int			datalen;
	int			content_length;
	int			reported_datalen;
	int			value_offset;
	int			value_len;
	e164_info_t		e164_info;
	/*
	 * Is this a request or response?
	 *
	 * Note that "tvb_find_line_end()" will return a value that
	 * is not longer than what's in the buffer, so the
	 * "tvb_get_ptr()" call won't throw an exception.
	 */
	first_linelen = tvb_find_line_end(tvb, offset,
	    tvb_ensure_length_remaining(tvb, offset), &next_offset,
	    FALSE);
	/*
	 * Is the first line a request or response?
	 */
	line = tvb_get_ptr(tvb, offset, first_linelen);
	is_request_or_reply = is_rtsp_request_or_reply(line, first_linelen,
	    &rtsp_type);
	if (is_request_or_reply) {
                /*
		 * Yes, it's a request or response.
		 * Do header desegmentation if we've been told to,
		 * and do body desegmentation if we've been told to and
		 * we find a Content-Length header.
		 */
		if (!req_resp_hdrs_do_reassembly(tvb, pinfo,
		    rtsp_desegment_headers, rtsp_desegment_body)) {
			/*
			 * More data needed for desegmentation.
			 */
			return -1;
		}
	}

	/*
	 * RFC 2326 says that a content length must be specified
	 * in requests that have a body, although section 4.4 speaks
	 * of a server closing the connection indicating the end of
	 * a reply body.
	 *
	 * We assume that an absent content length in a request means
	 * that we don't have a body, and that an absent content length
	 * in a reply means that the reply body runs to the end of
	 * the connection.  If the first line is neither, we assume
	 * that whatever follows a blank line should be treated as a
	 * body; there's not much else we can do, as we're jumping
	 * into the message in the middle.
	 *
	 * XXX - if there was no Content-Length entity header, we should
	 * accumulate all data until the end of the connection.
	 * That'd require that the TCP dissector call subdissectors
	 * for all frames with FIN, even if they contain no data,
	 * which would require subdissectors to deal intelligently
	 * with empty segments.
	 */
	if (rtsp_type == RTSP_REQUEST)
		body_requires_content_len = TRUE;
	else
		body_requires_content_len = FALSE;

	if (check_col(pinfo->cinfo, COL_PROTOCOL))
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "RTSP");
	if (check_col(pinfo->cinfo, COL_INFO)) {
		/*
		 * Put the first line from the buffer into the summary
 		 * if it's an RTSP request or reply (but leave out the
		 * line terminator).
		 * Otherwise, just call it a continuation.
		 *
		 * Note that "tvb_find_line_end()" will return a value that
		 * is not longer than what's in the buffer, so the
		 * "tvb_get_ptr()" call won't throw an exception.
		 */
		line = tvb_get_ptr(tvb, offset, first_linelen);
		if (is_request_or_reply)
			if ( rtsp_type == RTSP_REPLY ) {
				col_add_str(pinfo->cinfo, COL_INFO, "Reply: ");
				col_append_str(pinfo->cinfo, COL_INFO,
					format_text(line, first_linelen));
			}
			else
				col_add_str(pinfo->cinfo, COL_INFO,
					format_text(line, first_linelen));

		else
			col_set_str(pinfo->cinfo, COL_INFO, "Continuation");

	}

	orig_offset = offset;
	if (tree) {
		ti = proto_tree_add_item(tree, proto_rtsp, tvb, offset,	-1,
		    FALSE);
		rtsp_tree = proto_item_add_subtree(ti, ett_rtsp);
	}

	/*
	 * We haven't yet seen a Content-Length header.
	 */
	content_length = -1;

	/*
	 * Process the packet data, a line at a time.
	 */
	saw_req_resp_or_header = FALSE;	/* haven't seen anything yet */
	while (tvb_reported_length_remaining(tvb, offset) != 0) {
		/*
		 * We haven't yet concluded that this is a header.
		 */
		is_header = FALSE;

		/*
		 * Find the end of the line.
		 */
		linelen = tvb_find_line_end(tvb, offset,
		    tvb_ensure_length_remaining(tvb, offset), &next_offset,
		    FALSE);
		if (linelen < 0)
			return -1;
		line_end_offset = offset + linelen;
		/*
		 * colon_offset may be -1
		 */
		colon_offset = tvb_find_guint8(tvb, offset, linelen, ':');


		/*
		 * Get a buffer that refers to the line.
		 */
		line = tvb_get_ptr(tvb, offset, linelen);
		lineend = line + linelen;

		/*
		 * OK, does it look like an RTSP request or response?
		 */
		is_request_or_reply = is_rtsp_request_or_reply(line, linelen,
		    &rtsp_type);
		if (is_request_or_reply)
			goto is_rtsp;

		/*
		 * No.  Does it look like a blank line (as would appear
		 * at the end of an RTSP request)?
		 */
		if (linelen == 0)
			goto is_rtsp;	/* Yes. */

		/*
		 * No.  Does it look like a header?
		 */
		linep = line;
		while (linep < lineend) {
			c = *linep++;

			/*
			 * This must be a CHAR to be part of a token; that
			 * means it must be ASCII.
			 */
			if (!isascii(c))
				break;	/* not ASCII, thus not a CHAR */

			/*
			 * This mustn't be a CTL to be part of a token.
			 *
			 * XXX - what about leading LWS on continuation
			 * lines of a header?
			 */
			if (iscntrl(c))
				break;	/* CTL, not part of a header */

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
				/*
				 * It's a tspecial, so it's not
				 * part of a token, so it's not
				 * a field name for the beginning
				 * of a header.
				 */
				goto not_rtsp;

			case ':':
				/*
				 * This ends the token; we consider
				 * this to be a header.
				 */
				is_header = TRUE;
				goto is_rtsp;

			case ' ':
			case '\t':
				/*
				 * LWS (RFC-2616, 4.2); continue the previous
				 * header.
				 */
				goto is_rtsp;
			}
		}

		/*
		 * We haven't seen the colon, but everything else looks
		 * OK for a header line.
		 *
		 * If we've already seen an RTSP request or response
		 * line, or a header line, and we're at the end of
		 * the tvbuff, we assume this is an incomplete header
		 * line.  (We quit this loop after seeing a blank line,
		 * so if we've seen a request or response line, or a
		 * header line, this is probably more of the request
		 * or response we're presumably seeing.  There is some
		 * risk of false positives, but the same applies for
		 * full request or response lines or header lines,
		 * although that's less likely.)
		 *
		 * We throw an exception in that case, by checking for
		 * the existence of the next byte after the last one
		 * in the line.  If it exists, "tvb_ensure_bytes_exist()"
		 * throws no exception, and we fall through to the
		 * "not RTSP" case.  If it doesn't exist,
		 * "tvb_ensure_bytes_exist()" will throw the appropriate
		 * exception.
		 */
		if (saw_req_resp_or_header)
			tvb_ensure_bytes_exist(tvb, offset, linelen + 1);

	not_rtsp:
		/*
		 * We don't consider this part of an RTSP request or
		 * reply, so we don't display it.
		 */
		break;

	is_rtsp:
		/*
		 * Process this line.
		 */
		if (linelen == 0) {
			/*
			 * This is a blank line, which means that
			 * whatever follows it isn't part of this
			 * request or reply.
			 */
			proto_tree_add_text(rtsp_tree, tvb, offset,
			    next_offset - offset, "%s",
			    tvb_format_text(tvb, offset, next_offset - offset));
			offset = next_offset;
			break;
		}

		/*
		 * Not a blank line - either a request, a reply, or a header
		 * line.
		 */
		saw_req_resp_or_header = TRUE;
		if (rtsp_tree) {
			ti = proto_tree_add_text(rtsp_tree, tvb, offset,
				next_offset - offset, "%s",
				tvb_format_text(tvb, offset, next_offset - offset));

			sub_tree = proto_item_add_subtree(ti, ett_rtsp_method);

			switch (rtsp_type) {

			case RTSP_REQUEST:
				process_rtsp_request(tvb, offset, line, linelen,
				    sub_tree);
				break;

			case RTSP_REPLY:
				process_rtsp_reply(tvb, offset, line, linelen,
				    sub_tree);
				break;

			case NOT_RTSP:
				break;
			}
		}
		if (is_header) {
			/*
			 * Process some headers specially.
			 */
#define HDR_MATCHES(header) \
	(linelen > STRLEN_CONST(header) && \
	 strncasecmp(line, (header), STRLEN_CONST(header)) == 0)

			if (HDR_MATCHES(rtsp_transport)) {
				/*
				 * Based on the port numbers specified
				 * in the Transport: header, set up
				 * a conversation that will be dissected
				 * with the appropriate dissector.
				 */
				rtsp_create_conversation(pinfo, line, linelen);
			} else if (HDR_MATCHES(rtsp_content_type)) {
				/*
				 * If the Content-Type: header says this
				 * is SDP, dissect the payload as SDP.
				 *
				 * XXX - we should just do the same
				 * sort of header processing
				 * that HTTP does, and use the
				 * "media_type" dissector table on
				 * the content type.
				 *
				 * We should use those for Transport:
				 * and Content-Length: as well (and
				 * should process Content-Length: in
				 * HTTP).
				 */
				if (is_content_sdp(line, linelen))
					is_sdp = TRUE;
			} else if (HDR_MATCHES(rtsp_content_length)) {
				/*
				 * Only the amount specified by the
				 * Content-Length: header should be treated
				 * as payload.
				 */
				content_length = rtsp_get_content_length(line,
				    linelen);

			} else if (HDR_MATCHES(rtsp_Session)) {
				/*
				 * Extract the session string

				 */

				if ( colon_offset != -1 ){
					/*
					* Skip whitespace after the colon.
					* (Code from SIP dissector )
					*/
					value_offset = colon_offset + 1;
					while (value_offset < line_end_offset
						&& ((c = tvb_get_guint8(tvb,
							value_offset)) == ' '
						|| c == '\t'))
						value_offset++;
					/*
					 * Put the value into the protocol tree
					*/
					value_len = line_end_offset - value_offset;
					proto_tree_add_string(sub_tree, hf_rtsp_session,tvb,
						value_offset, value_len ,
						tvb_format_text(tvb, value_offset, value_len));
				}

			} else if (HDR_MATCHES(rtsp_X_Vig_Msisdn)) {
				/*
				 * Extract the X_Vig_Msisdn string
				 */
				if ( colon_offset != -1 ){
					/*
					 * Skip whitespace after the colon.
					 * (Code from SIP dissector )
					 */
					value_offset = colon_offset + 1;
					while (value_offset < line_end_offset
						&& ((c = tvb_get_guint8(tvb,
						    value_offset)) == ' '
						  || c == '\t'))
						value_offset++;
					/*
					 * Put the value into the protocol tree
					 */
					value_len = line_end_offset - value_offset;
					proto_tree_add_string(sub_tree, hf_rtsp_X_Vig_Msisdn,tvb,
						value_offset, value_len ,
						tvb_format_text(tvb, value_offset, value_len));

					e164_info.e164_number_type = CALLING_PARTY_NUMBER;
					e164_info.nature_of_address = 0;

					e164_info.E164_number_str = tvb_get_string(tvb, value_offset,
						value_len);
					e164_info.E164_number_length = value_len;
					dissect_e164_number(tvb, sub_tree, value_offset,
						value_len, e164_info);
					g_free(e164_info.E164_number_str);


				}
			}

		}
		offset = next_offset;
	}

	/*
	 * If a content length was supplied, the amount of data to be
	 * processed as RTSP payload is the minimum of the content
	 * length and the amount of data remaining in the frame.
	 *
	 * If no content length was supplied (or if a bad content length
	 * was supplied), the amount of data to be processed is the amount
	 * of data remaining in the frame.
	 */
	datalen = tvb_length_remaining(tvb, offset);
	reported_datalen = tvb_reported_length_remaining(tvb, offset);
	if (content_length != -1) {
		/*
		 * Content length specified; display only that amount
		 * as payload.
		 */
		if (datalen > content_length)
			datalen = content_length;

		/*
		 * XXX - limit the reported length in the tvbuff we'll
		 * hand to a subdissector to be no greater than the
		 * content length.
		 *
		 * We really need both unreassembled and "how long it'd
		 * be if it were reassembled" lengths for tvbuffs, so
		 * that we throw the appropriate exceptions for
		 * "not enough data captured" (running past the length),
		 * "packet needed reassembly" (within the length but
		 * running past the unreassembled length), and
		 * "packet is malformed" (running past the reassembled
		 * length).
		 */
		if (reported_datalen > content_length)
			reported_datalen = content_length;
	} else {
		/*
		 * No content length specified; if this message doesn't
		 * have a body if no content length is specified, process
		 * nothing as payload.
		 */
		if (body_requires_content_len)
			datalen = 0;
	}

	if (datalen > 0) {
		/*
		 * There's stuff left over; process it.
		 */
		if (is_sdp) {
			tvbuff_t *new_tvb;

			/*
			 * Fix up the top-level item so that it doesn't
			 * include the SDP stuff.
			 */
			if (ti != NULL)
				proto_item_set_len(ti, offset);

			/*
			 * Now create a tvbuff for the SDP stuff and
			 * dissect it.
			 *
			 * The amount of data to be processed that's
			 * available in the tvbuff is "datalen", which
			 * is the minimum of the amount of data left in
			 * the tvbuff and any specified content length.
			 *
			 * The amount of data to be processed that's in
			 * this frame, regardless of whether it was
			 * captured or not, is "reported_datalen",
			 * which, if no content length was specified,
			 * is -1, i.e. "to the end of the frame.
			 */
			new_tvb = tvb_new_subset(tvb, offset, datalen,
			    reported_datalen);
			call_dissector(sdp_handle, new_tvb, pinfo, tree);
		} else {
			if (tvb_get_guint8(tvb, offset) == RTSP_FRAMEHDR) {
				/*
				 * This is interleaved stuff; don't
				 * treat it as raw data - set "datalen"
				 * to 0, so we won't skip the offset
				 * past it, which will cause our
				 * caller to process that stuff itself.
				 */
				datalen = 0;
			} else {
				proto_tree_add_text(rtsp_tree, tvb, offset,
				    datalen, "Data (%d bytes)",
				    reported_datalen);
			}
		}

		/*
		 * We've processed "datalen" bytes worth of data
		 * (which may be no data at all); advance the
		 * offset past whatever data we've processed.
		 */
		offset += datalen;
	}
	return offset - orig_offset;
}

static void
process_rtsp_request(tvbuff_t *tvb, int offset, const guchar *data,
	size_t linelen, proto_tree *tree)
{
	const guchar	*lineend = data + linelen;
	unsigned		ii;
	const guchar	*url;
	const guchar	*url_start;
	guchar			*tmp_url;

	/* Request Methods */
	for (ii = 0; ii < RTSP_NMETHODS; ii++) {
		size_t len = strlen(rtsp_methods[ii]);
		if (linelen >= len &&
		    strncasecmp(rtsp_methods[ii], data, len) == 0 &&
		    (len == linelen || isspace(data[len])))
			break;
	}
	if (ii == RTSP_NMETHODS) {
		/*
		 * We got here because "is_rtsp_request_or_reply()" returned
		 * RTSP_REQUEST, so we know one of the request methods
		 * matched, so we "can't get here".
		 */
		g_assert_not_reached();
	}

	/* Method name */
	proto_tree_add_string(tree, hf_rtsp_method, tvb, offset,
		strlen(rtsp_methods[ii]), rtsp_methods[ii]);


	/* URL */
	url = data;
	while (url < lineend && !isspace(*url))
		url++;
	while (url < lineend && isspace(*url))
		url++;
	url_start = url;
	while (url < lineend && !isspace(*url))
		url++;
	tmp_url = g_malloc(url - url_start + 1);
	memcpy(tmp_url, url_start, url - url_start);
	tmp_url[url - url_start] = 0;
	proto_tree_add_string(tree, hf_rtsp_url, tvb,
		offset + (url_start - data), url - url_start, tmp_url);
	g_free(tmp_url);
}

static void
process_rtsp_reply(tvbuff_t *tvb, int offset, const guchar *data,
	size_t linelen, proto_tree *tree)
{
	const guchar	*lineend = data + linelen;
	const guchar	*status = data;
	const guchar	*status_start;
	unsigned int	status_i;

	/* status code */
	while (status < lineend && !isspace(*status))
		status++;
	while (status < lineend && isspace(*status))
		status++;
	status_start = status;
	status_i = 0;
	while (status < lineend && isdigit(*status))
		status_i = status_i * 10 + *status++ - '0';
	proto_tree_add_uint(tree, hf_rtsp_status, tvb,
		offset + (status_start - data),
		status - status_start, status_i);
}

static void
dissect_rtsp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	int		offset = 0;
	int		len;

	while (tvb_reported_length_remaining(tvb, offset) != 0) {
		len = (tvb_get_guint8(tvb, offset) == RTSP_FRAMEHDR)
			? dissect_rtspinterleaved(tvb, offset, pinfo, tree)
			: dissect_rtspmessage(tvb, offset, pinfo, tree);
		if (len == -1)
			break;
		offset += len;

		/*
		 * OK, we've set the Protocol and Info columns for the
		 * first RTSP message; make the columns non-writable,
		 * so that we don't change it for subsequent RTSP messages.
		 */
		col_set_writable(pinfo->cinfo, FALSE);
	}
}

static void
rtsp_init(void)
{
/* Routine to initialize rtsp protocol before each capture or filter pass. */
/* Release any memory if needed.  Then setup the memory chunks.		*/

  	if (rtsp_vals)
   		g_mem_chunk_destroy(rtsp_vals);

  	rtsp_vals = g_mem_chunk_new("rtsp_vals",
		sizeof(rtsp_conversation_data_t),
		rtsp_hash_init_count * sizeof(rtsp_conversation_data_t),
		G_ALLOC_AND_FREE);
}

void
proto_register_rtsp(void)
{
	static gint *ett[] = {
		&ett_rtspframe,
		&ett_rtsp,
		&ett_rtsp_method,
	};
	static hf_register_info hf[] = {
		{ &hf_rtsp_method,
			{ "Method", "rtsp.method", FT_STRING, BASE_NONE, NULL, 0,
			"", HFILL }},
		{ &hf_rtsp_url,
			{ "URL", "rtsp.url", FT_STRING, BASE_NONE, NULL, 0,
			"", HFILL }},
		{ &hf_rtsp_status,
			{ "Status", "rtsp.status", FT_UINT32, BASE_DEC, NULL, 0,
			"", HFILL }},
		{ &hf_rtsp_session,
			{ "Session", "rtsp.session", FT_STRING, BASE_NONE, NULL, 0,
			"", HFILL }},
		{ &hf_rtsp_X_Vig_Msisdn,
			{ "X-Vig-Msisdn", "X_Vig_Msisdn", FT_STRING, BASE_NONE, NULL, 0,
			"", HFILL }},


	};
	module_t *rtsp_module;

        proto_rtsp = proto_register_protocol("Real Time Streaming Protocol",
		"RTSP", "rtsp");
	proto_register_field_array(proto_rtsp, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));

	/* Register our configuration options, particularly our ports */

	rtsp_module = prefs_register_protocol(proto_rtsp, proto_reg_handoff_rtsp);
	prefs_register_uint_preference(rtsp_module, "tcp.port",
		"RTSP TCP Port",
		"Set the TCP port for RTSP messages",
		10, &global_rtsp_tcp_port);
	prefs_register_uint_preference(rtsp_module, "tcp.alternate_port",
		"Alternate RTSP TCP Port",
		"Set the alternate TCP port for RTSP messages",
		10, &global_rtsp_tcp_alternate_port);
	prefs_register_bool_preference(rtsp_module, "desegment_headers",
	    "Reassemble RTSP headers spanning multiple TCP segments",
	    "Whether the RTSP dissector should reassemble headers "
	    "of a request spanning multiple TCP segments. "
	    " To use this option, you must also enable \"Allow subdissectors to reassemble TCP streams\" in the TCP protocol settings.",
	    &rtsp_desegment_headers);
	prefs_register_bool_preference(rtsp_module, "desegment_body",
	    "Trust the \"Content-length:\" header and\ndesegment RTSP "
	    "bodies\nspanning multiple TCP segments",
	    "Whether the RTSP dissector should use the "
	    "\"Content-length:\" value to desegment the body "
	    "of a request spanning multiple TCP segments",
	    &rtsp_desegment_body);

	register_init_routine(rtsp_init);	/* register re-init routine */
}

void
proto_reg_handoff_rtsp(void)
{
	dissector_handle_t rtsp_handle;
	static int rtsp_prefs_initialized = FALSE;

	rtsp_handle = create_dissector_handle(dissect_rtsp, proto_rtsp);

	if (!rtsp_prefs_initialized) {
		rtsp_prefs_initialized = TRUE;
	}
	else {
		dissector_delete("tcp.port", tcp_port, rtsp_handle);
		dissector_delete("tcp.port", tcp_alternate_port, rtsp_handle);
	}
	/* Set our port number for future use */

	tcp_port = global_rtsp_tcp_port;
	tcp_alternate_port = global_rtsp_tcp_alternate_port;

	dissector_add("tcp.port", tcp_port, rtsp_handle);
	dissector_add("tcp.port", tcp_alternate_port, rtsp_handle);

	sdp_handle = find_dissector("sdp");
	rtp_handle = find_dissector("rtp");
	rtcp_handle = find_dissector("rtcp");
}
