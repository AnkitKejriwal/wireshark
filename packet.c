/* packet.c
 * Routines for packet disassembly
 *
 * $Id: packet.c,v 1.55 1999/11/17 21:58:32 guy Exp $
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@zing.org>
 * Copyright 1998 Gerald Combs
 *
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

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_WINSOCK_H
#include <winsock.h>
#endif

#include <glib.h>

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#ifdef NEED_SNPRINTF_H
# include "snprintf.h"
#endif

#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif

#include <arpa/inet.h>

#ifdef NEED_INET_V6DEFS_H
# include "inet_v6defs.h"
#endif

#include "packet.h"
#include "print.h"
#include "file.h"

extern capture_file  cf;

static int proto_frame = -1;
static int hf_frame_arrival_time = -1;
static int hf_frame_time_delta = -1;
static int hf_frame_number = -1;
static int hf_frame_packet_len = -1;
static int hf_frame_capture_len = -1;

static gint ett_frame = -1;

gchar *
ether_to_str(const guint8 *ad) {
  static gchar  str[3][18];
  static gchar *cur;
  gchar        *p;
  int          i;
  guint32      octet;
  static const gchar hex_digits[16] = "0123456789abcdef";

  if (cur == &str[0][0]) {
    cur = &str[1][0];
  } else if (cur == &str[1][0]) {  
    cur = &str[2][0];
  } else {  
    cur = &str[0][0];
  }
  p = &cur[18];
  *--p = '\0';
  i = 5;
  for (;;) {
    octet = ad[i];
    *--p = hex_digits[octet&0xF];
    octet >>= 4;
    *--p = hex_digits[octet&0xF];
    if (i == 0)
      break;
    *--p = ':';
    i--;
  }
  return p;
}

gchar *
ip_to_str(const guint8 *ad) {
  static gchar  str[3][16];
  static gchar *cur;
  gchar        *p;
  int           i;
  guint32       octet;
  guint32       digit;

  if (cur == &str[0][0]) {
    cur = &str[1][0];
  } else if (cur == &str[1][0]) {  
    cur = &str[2][0];
  } else {  
    cur = &str[0][0];
  }
  p = &cur[16];
  *--p = '\0';
  i = 3;
  for (;;) {
    octet = ad[i];
    *--p = (octet%10) + '0';
    octet /= 10;
    digit = octet%10;
    octet /= 10;
    if (digit != 0 || octet != 0)
      *--p = digit + '0';
    if (octet != 0)
      *--p = octet + '0';
    if (i == 0)
      break;
    *--p = '.';
    i--;
  }
  return p;
}

gchar *
ip6_to_str(struct e_in6_addr *ad) {
#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46
#endif
  static gchar buf[INET6_ADDRSTRLEN];

  inet_ntop(AF_INET6, (u_char*)ad, (gchar*)buf, sizeof(buf));
  return buf;
}


#define	PLURALIZE(n)	(((n) > 1) ? "s" : "")
#define	COMMA(do_it)	((do_it) ? ", " : "")

gchar *
time_secs_to_str(guint32 time)
{
  static gchar  str[3][8+1+4+2+2+5+2+2+7+2+2+7+1];
  static gchar *cur, *p;
  int hours, mins, secs;
  int do_comma;

  if (cur == &str[0][0]) {
    cur = &str[1][0];
  } else if (cur == &str[1][0]) {  
    cur = &str[2][0];
  } else {  
    cur = &str[0][0];
  }

  secs = time % 60;
  time /= 60;
  mins = time % 60;
  time /= 60;
  hours = time % 24;
  time /= 24;

  p = cur;
  if (time != 0) {
    sprintf(p, "%u day%s", time, PLURALIZE(time));
    p += strlen(p);
    do_comma = 1;
  } else
    do_comma = 0;
  if (hours != 0) {
    sprintf(p, "%s%u hour%s", COMMA(do_comma), hours, PLURALIZE(hours));
    p += strlen(p);
    do_comma = 1;
  } else
    do_comma = 0;
  if (mins != 0) {
    sprintf(p, "%s%u minute%s", COMMA(do_comma), mins, PLURALIZE(mins));
    p += strlen(p);
    do_comma = 1;
  } else
    do_comma = 0;
  if (secs != 0)
    sprintf(p, "%s%u second%s", COMMA(do_comma), secs, PLURALIZE(secs));
  return cur;
}

/* Max string length for displaying byte string.  */
#define	MAX_BYTE_STR_LEN	20

/* Turn an array of bytes into a string showing the bytes in hex. */
gchar *
bytes_to_str(const guint8 *bd, int bd_len) {
  static gchar  str[3][MAX_BYTE_STR_LEN+3+1];
  static gchar *cur;
  gchar        *p;
  int           len;
  static const char hex[16] = { '0', '1', '2', '3', '4', '5', '6', '7',
                                '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

  if (cur == &str[0][0]) {
    cur = &str[1][0];
  } else if (cur == &str[1][0]) {  
    cur = &str[2][0];
  } else {  
    cur = &str[0][0];
  }
  p = cur;
  len = MAX_BYTE_STR_LEN;
  while (bd_len > 0 && len > 0) {
    *p++ = hex[(*bd) >> 4];
    *p++ = hex[(*bd) & 0xF];
    len -= 2;
    bd++;
    bd_len--;
  }
  if (bd_len != 0) {
    /* Note that we're not showing the full string.  */
    *p++ = '.';
    *p++ = '.';
    *p++ = '.';
  }
  *p = '\0';
  return cur;
}

static const char *mon_names[12] = {
	"Jan",
	"Feb",
	"Mar",
	"Apr",
	"May",
	"Jun",
	"Jul",
	"Aug",
	"Sep",
	"Oct",
	"Nov",
	"Dec"
};

gchar *
abs_time_to_str(struct timeval *abs_time)
{
        struct tm *tmp;
        static gchar *cur;
        static char str[3][3+1+2+2+4+1+2+1+2+1+2+1+4+1 + 5 /* extra */];

        if (cur == &str[0][0]) {
                cur = &str[1][0];
        } else if (cur == &str[1][0]) {
                cur = &str[2][0];
        } else {
                cur = &str[0][0];
        }

        tmp = localtime(&abs_time->tv_sec);
        sprintf(cur, "%s %2d, %d %02d:%02d:%02d.%04ld",
            mon_names[tmp->tm_mon],
            tmp->tm_mday,
            tmp->tm_year + 1900,
            tmp->tm_hour,
            tmp->tm_min,
            tmp->tm_sec,
            (long)abs_time->tv_usec/100);

        return cur;
}

gchar *
rel_time_to_str(struct timeval *rel_time)
{
        static gchar *cur;
        static char str[3][10+1+6+1];

        if (cur == &str[0][0]) {
                cur = &str[1][0];
        } else if (cur == &str[1][0]) {
                cur = &str[2][0];
        } else {
                cur = &str[0][0];
        }

	sprintf(cur, "%ld.%06ld", (long)rel_time->tv_sec,
		(long)rel_time->tv_usec);

        return cur;
}

/*
 * Given a pointer into a data buffer, and to the end of the buffer,
 * find the end of the (putative) line at that position in the data
 * buffer.
 * Return a pointer to the EOL character(s) in "*eol".
 */
const u_char *
find_line_end(const u_char *data, const u_char *dataend, const u_char **eol)
{
  const u_char *lineend;

  lineend = memchr(data, '\n', dataend - data);
  if (lineend == NULL) {
    /*
     * No LF - line is probably continued in next TCP segment.
     */
    lineend = dataend;
    *eol = dataend;
  } else {
    /*
     * Is the LF at the beginning of the line?
     */
    if (lineend > data) {
      /*
       * No - is it preceded by a carriage return?
       * (Perhaps it's supposed to be, but that's not guaranteed....)
       */
      if (*(lineend - 1) == '\r') {
        /*
	 * Yes.  The EOL starts with the CR.
	 */
        *eol = lineend - 1;
      } else {
        /*
         * No.  The EOL starts with the LF.
         */
        *eol = lineend;

        /*
         * I seem to remember that we once saw lines ending with LF-CR
         * in an HTTP request or response, so check if it's *followed*
         * by a carriage return.
         */
        if (lineend < (dataend - 1) && *(lineend + 1) == '\r') {
          /*
           * It's <non-LF><LF><CR>; say it ends with the CR.
           */
          lineend++;
        }
      }
    }

    /*
     * Point to the character after the last character.
     */
    lineend++;
  }
  return lineend;
}

#define	MAX_COLUMNS_LINE_DETAIL	62

/*
 * Get the length of the next token in a line, and the beginning of the
 * next token after that (if any).
 * Return 0 if there is no next token.
 */
int
get_token_len(const u_char *linep, const u_char *lineend,
	      const u_char **next_token)
{
  const u_char *tokenp;
  int token_len;

  tokenp = linep;
  
  /*
   * Search for a blank, a CR or an LF, or the end of the buffer.
   */
  while (linep < lineend && *linep != ' ' && *linep != '\r' && *linep != '\n')
      linep++;
  token_len = linep - tokenp;

  /*
   * Skip trailing blanks.
   */
  while (linep < lineend && *linep == ' ')
    linep++;

  *next_token = linep;

  return token_len;
}

/*
 * Given a string, generate a string from it that shows non-printable
 * characters as C-style escapes, and return a pointer to it.
 */
gchar *
format_text(const u_char *string, int len)
{
  static gchar fmtbuf[MAX_COLUMNS_LINE_DETAIL + 3 + 4 + 1];
  gchar *fmtbufp;
  int column;
  const u_char *stringend = string + len;
  u_char c;
  int i;

  column = 0;
  fmtbufp = &fmtbuf[0];
  while (string < stringend) {
    if (column >= MAX_COLUMNS_LINE_DETAIL) {
      /*
       * Put "..." and quit.
       */
      strcpy(fmtbufp, " ...");
      break;
    }
    c = *string++;
    if (isprint(c)) {
      *fmtbufp++ = c;
      column++;
    } else {
      *fmtbufp++ =  '\\';
      column++;
      switch (c) {

      case '\\':
	*fmtbufp++ = '\\';
	column++;
	break;

      case '\a':
	*fmtbufp++ = 'a';
	column++;
	break;

      case '\b':
	*fmtbufp++ = 'b';
	column++;
	break;

      case '\f':
	*fmtbufp++ = 'f';
	column++;
	break;

      case '\n':
	*fmtbufp++ = 'n';
	column++;
	break;

      case '\r':
	*fmtbufp++ = 'r';
	column++;
	break;

      case '\t':
	*fmtbufp++ = 't';
	column++;
	break;

      case '\v':
	*fmtbufp++ = 'v';
	column++;
	break;

      default:
	i = (c>>6)&03;
	*fmtbufp++ = i + '0';
	column++;
	i = (c>>3)&07;
	*fmtbufp++ = i + '0';
	column++;
	i = (c>>0)&07;
	*fmtbufp++ = i + '0';
	column++;
	break;
      }
    }
  }
  *fmtbufp = '\0';
  return fmtbuf;
}


/* Tries to match val against each element in the value_string array vs.
   Returns the associated string ptr on a match.
   Formats val with fmt, and returns the resulting string, on failure. */
gchar*
val_to_str(guint32 val, const value_string *vs, const char *fmt) {
  gchar *ret;
  static gchar  str[3][64];
  static gchar *cur;

  ret = match_strval(val, vs);
  if (ret != NULL)
    return ret;
  if (cur == &str[0][0]) {
    cur = &str[1][0];
  } else if (cur == &str[1][0]) {  
    cur = &str[2][0];
  } else {  
    cur = &str[0][0];
  }
  snprintf(cur, 64, fmt, val);
  return cur;
}

/* Tries to match val against each element in the value_string array vs.
   Returns the associated string ptr on a match, or NULL on failure. */
gchar*
match_strval(guint32 val, const value_string *vs) {
  gint i = 0;
  
  while (vs[i].strptr) {
    if (vs[i].value == val)
      return(vs[i].strptr);
    i++;
  }

  return(NULL);
}

/* Generate, into "buf", a string showing the bits of a bitfield.
   Return a pointer to the character after that string. */
char *
decode_bitfield_value(char *buf, guint32 val, guint32 mask, int width)
{
  int i;
  guint32 bit;
  char *p;

  i = 0;
  p = buf;
  bit = 1 << (width - 1);
  for (;;) {
    if (mask & bit) {
      /* This bit is part of the field.  Show its value. */
      if (val & bit)
        *p++ = '1';
      else
        *p++ = '0';
    } else {
      /* This bit is not part of the field. */
      *p++ = '.';
    }
    bit >>= 1;
    i++;
    if (i >= width)
      break;
    if (i % 4 == 0)
      *p++ = ' ';
  }
  strcpy(p, " = ");
  p += 3;
  return p;
}

/* Generate a string describing a Boolean bitfield (a one-bit field that
   says something is either true of false). */
const char *
decode_boolean_bitfield(guint32 val, guint32 mask, int width,
    const char *truedesc, const char *falsedesc)
{
  static char buf[1025];
  char *p;

  p = decode_bitfield_value(buf, val, mask, width);
  if (val & mask)
    strcpy(p, truedesc);
  else
    strcpy(p, falsedesc);
  return buf;
}

/* Generate a string describing an enumerated bitfield (an N-bit field
   with various specific values having particular names). */
const char *
decode_enumerated_bitfield(guint32 val, guint32 mask, int width,
    const value_string *tab, const char *fmt)
{
  static char buf[1025];
  char *p;

  p = decode_bitfield_value(buf, val, mask, width);
  sprintf(p, fmt, val_to_str(val & mask, tab, "Unknown"));
  return buf;
}

/* Generate a string describing a numeric bitfield (an N-bit field whose
   value is just a number). */
const char *
decode_numeric_bitfield(guint32 val, guint32 mask, int width,
    const char *fmt)
{
  static char buf[1025];
  char *p;
  int shift = 0;

  /* Compute the number of bits we have to shift the bitfield right
     to extract its value. */
  while ((mask & (1<<shift)) == 0)
    shift++;

  p = decode_bitfield_value(buf, val, mask, width);
  sprintf(p, fmt, (val & mask) >> shift);
  return buf;
}

/* Checks to see if a particular packet information element is needed for
   the packet list */
gint
check_col(frame_data *fd, gint el) {
  int i;

  if (fd->cinfo) {
    for (i = 0; i < fd->cinfo->num_cols; i++) {
      if (fd->cinfo->fmt_matx[i][el])
        return TRUE;
    }
  }
  return FALSE;
}

/* Adds a vararg list to a packet info string. */
void
col_add_fstr(frame_data *fd, gint el, gchar *format, ...) {
  va_list ap;
  int     i;
  size_t  max_len;
  
  va_start(ap, format);
  for (i = 0; i < fd->cinfo->num_cols; i++) {
    if (fd->cinfo->fmt_matx[i][el]) {
      if (el == COL_INFO)
	max_len = COL_MAX_INFO_LEN;
      else
	max_len = COL_MAX_LEN;
      vsnprintf(fd->cinfo->col_data[i], max_len, format, ap);
    }
  }
}

void
col_add_str(frame_data *fd, gint el, const gchar* str) {
  int    i;
  size_t max_len;

  for (i = 0; i < fd->cinfo->num_cols; i++) {
    if (fd->cinfo->fmt_matx[i][el]) {
      if (el == COL_INFO)
	max_len = COL_MAX_INFO_LEN;
      else
	max_len = COL_MAX_LEN;
      strncpy(fd->cinfo->col_data[i], str, max_len);
      fd->cinfo->col_data[i][max_len - 1] = 0;
    }
  }
}

/* Appends a vararg list to a packet info string. */
void
col_append_fstr(frame_data *fd, gint el, gchar *format, ...) {
  va_list ap;
  int     i;
  size_t  len, max_len;
  
  va_start(ap, format);
  for (i = 0; i < fd->cinfo->num_cols; i++) {
    if (fd->cinfo->fmt_matx[i][el]) {
      len = strlen(fd->cinfo->col_data[i]);
      if (el == COL_INFO)
	max_len = COL_MAX_INFO_LEN;
      else
	max_len = COL_MAX_LEN;
      vsnprintf(&fd->cinfo->col_data[i][len], max_len - len, format, ap);
    }
  }
}

void
col_append_str(frame_data *fd, gint el, gchar* str) {
  int    i;
  size_t len, max_len;

  for (i = 0; i < fd->cinfo->num_cols; i++) {
    if (fd->cinfo->fmt_matx[i][el]) {
      len = strlen(fd->cinfo->col_data[i]);
      if (el == COL_INFO)
	max_len = COL_MAX_LEN;
      else
	max_len = COL_MAX_INFO_LEN;
      strncat(fd->cinfo->col_data[i], str, max_len - len);
      fd->cinfo->col_data[i][max_len - 1] = 0;
    }
  }
}
	
void blank_packetinfo(void)
{
  pi.dl_src.type = AT_NONE;
  pi.dl_dst.type = AT_NONE;
  pi.net_src.type = AT_NONE;
  pi.net_dst.type = AT_NONE;
  pi.src.type = AT_NONE;
  pi.dst.type = AT_NONE;
  pi.ipproto  = 0;
  pi.ptype = PT_NONE;
  pi.srcport  = 0;
  pi.destport = 0;
}

/* Allow protocols to register "init" routines, which are called before
   we make a pass through a capture file and dissect all its packets
   (e.g., when we read in a new capture file, or run a "filter packets"
   or "colorize packets" pass over the current capture file). */
static GSList *init_routines;

void
register_init_routine(void (*func)(void))
{
	init_routines = g_slist_append(init_routines, func);
}

/* Call all the registered "init" routines. */
static void
call_init_routine(gpointer routine, gpointer dummy)
{
	void (*func)(void) = routine;

	(*func)();
}

void
init_all_protocols(void)
{
	g_slist_foreach(init_routines, &call_init_routine, NULL);
}

/* this routine checks the frame type from the cf structure */
void
dissect_packet(const u_char *pd, frame_data *fd, proto_tree *tree)
{
	proto_tree *fh_tree;
	proto_item *ti;
	struct timeval tv;

	/* Put in frame header information. */
	if (tree) {
	  ti = proto_tree_add_item_format(tree, proto_frame, 0, fd->cap_len,
	    NULL, "Frame (%d on wire, %d captured)", fd->pkt_len, fd->cap_len);

	  fh_tree = proto_item_add_subtree(ti, ett_frame);

	  tv.tv_sec = fd->abs_secs;
	  tv.tv_usec = fd->abs_usecs;

	  proto_tree_add_item(fh_tree, hf_frame_arrival_time,
		0, 0, &tv);

	  tv.tv_sec = fd->del_secs;
	  tv.tv_usec = fd->del_usecs;

	  proto_tree_add_item(fh_tree, hf_frame_time_delta,
		0, 0, &tv);

	  proto_tree_add_item(fh_tree, hf_frame_number,
		0, 0, fd->num);

	  proto_tree_add_item_format(fh_tree, hf_frame_packet_len,
		0, 0, fd->pkt_len, "Packet Length: %d byte%s", fd->pkt_len,
		plurality(fd->pkt_len, "", "s"));
		
	  proto_tree_add_item_format(fh_tree, hf_frame_capture_len,
		0, 0, fd->cap_len, "Capture Length: %d byte%s", fd->cap_len,
		plurality(fd->cap_len, "", "s"));
	}

	blank_packetinfo();

	/* Set the initial payload to the packet length, and the initial
	   captured payload to the capture length (other protocols may
	   reduce them if their headers say they're less). */
	pi.len = fd->pkt_len;
	pi.captured_len = fd->cap_len;

	switch (fd->lnk_t) {
		case WTAP_ENCAP_ETHERNET :
			dissect_eth(pd, 0, fd, tree);
			break;
		case WTAP_ENCAP_FDDI :
			dissect_fddi(pd, fd, tree, FALSE);
			break;
		case WTAP_ENCAP_FDDI_BITSWAPPED :
			dissect_fddi(pd, fd, tree, TRUE);
			break;
		case WTAP_ENCAP_TR :
			dissect_tr(pd, 0, fd, tree);
			break;
		case WTAP_ENCAP_NULL :
			dissect_null(pd, fd, tree);
			break;
		case WTAP_ENCAP_PPP :
			dissect_ppp(pd, fd, tree);
			break;
		case WTAP_ENCAP_LAPB :
			dissect_lapb(pd, fd, tree);
			break;
		case WTAP_ENCAP_RAW_IP :
			dissect_raw(pd, fd, tree);
			break;
		case WTAP_ENCAP_LINUX_ATM_CLIP :
			dissect_clip(pd, fd, tree);
			break;
		case WTAP_ENCAP_ATM_SNIFFER :
			dissect_atm(pd, fd, tree);
			break;
		case WTAP_ENCAP_ASCEND :
			dissect_ascend(pd, fd, tree);
			break;
		case WTAP_ENCAP_LAPD :
			dissect_lapd(pd, fd, tree);
			break;
	}
}

void
proto_register_frame(void)
{
	static hf_register_info hf[] = {
		{ &hf_frame_arrival_time,
		{ "Arrival Time",		"frame.time", FT_ABSOLUTE_TIME, BASE_NONE, NULL, 0x0,
			""}},

		{ &hf_frame_time_delta,
		{ "Time delta from previous packet",	"frame.time_delta", FT_RELATIVE_TIME, BASE_NONE, NULL,
			0x0,
			"" }},

		{ &hf_frame_number,
		{ "Frame Number",		"frame.number", FT_UINT32, BASE_DEC, NULL, 0x0,
			"" }},

		{ &hf_frame_packet_len,
		{ "Total Frame Length",		"frame.pkt_len", FT_UINT32, BASE_DEC, NULL, 0x0,
			"" }},

		{ &hf_frame_capture_len,
		{ "Capture Frame Length",	"frame.cap_len", FT_UINT32, BASE_DEC, NULL, 0x0,
			"" }},
	};
	static gint *ett[] = {
		&ett_frame,
	};

	proto_frame = proto_register_protocol("Frame", "frame");
	proto_register_field_array(proto_frame, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));
}
