/* column-utils.c
 * Routines for column utilities.
 *
 * $Id: column-utils.c,v 1.42 2004/01/19 03:46:41 ulfl Exp $
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

#include <string.h>
#include <time.h>

#ifdef NEED_SNPRINTF_H
# include "snprintf.h"
#endif

#include "column-utils.h"
#include "timestamp.h"
#include "sna-utils.h"
#include "atalk-utils.h"
#include "to_str.h"
#include "packet_info.h"
#include "pint.h"
#include "resolv.h"
#include "ipv6-utils.h"
#include "osi-utils.h"
#include "value_string.h"

/* Allocate all the data structures for constructing column data, given
   the number of columns. */
void
col_setup(column_info *col_info, gint num_cols)
{
  int i;

  col_info->num_cols	= num_cols;
  col_info->col_fmt	= (gint *) g_malloc(sizeof(gint) * num_cols);
  col_info->fmt_matx	= (gboolean **) g_malloc(sizeof(gboolean *) * num_cols);
  col_info->col_first	= (int *) g_malloc(sizeof(int) * (NUM_COL_FMTS));
  col_info->col_last 	= (int *) g_malloc(sizeof(int) * (NUM_COL_FMTS));
  col_info->col_title	= (gchar **) g_malloc(sizeof(gchar *) * num_cols);
  col_info->col_data	= (gchar **) g_malloc(sizeof(gchar *) * num_cols);
  col_info->col_buf	= (gchar **) g_malloc(sizeof(gchar *) * num_cols);
  col_info->col_fence	= (int *) g_malloc(sizeof(int) * num_cols);
  col_info->col_expr	= (gchar **) g_malloc(sizeof(gchar *) * num_cols);
  col_info->col_expr_val = (gchar **) g_malloc(sizeof(gchar *) * num_cols);

  for (i = 0; i < NUM_COL_FMTS; i++) {
    col_info->col_first[i] = -1;
    col_info->col_last[i] = -1;
  }
}

/* Initialize the data structures for constructing column data. */
void
col_init(column_info *cinfo)
{
  int i;

  for (i = 0; i < cinfo->num_cols; i++) {
    cinfo->col_buf[i][0] = '\0';
    cinfo->col_data[i] = cinfo->col_buf[i];
    cinfo->col_fence[i] = 0;
    cinfo->col_expr[i][0] = '\0';
    cinfo->col_expr_val[i][0] = '\0';
  }
  cinfo->writable = TRUE;
}

gboolean
col_get_writable(column_info *cinfo)
{
	return (cinfo ? cinfo->writable : FALSE);
}

void
col_set_writable(column_info *cinfo, gboolean writable)
{
	if (cinfo)
		cinfo->writable = writable;
}

/* Checks to see if a particular packet information element is needed for
   the packet list */
gint
check_col(column_info *cinfo, gint el) {

  if (cinfo && cinfo->writable) {
    /* We are constructing columns, and they're writable */
    if (cinfo->col_first[el] >= 0) {
      /* There is at least one column in that format */
      return TRUE;
    }
  }
  return FALSE;
}

/* Sets the fence for a column to be at the end of the column. */
void
col_set_fence(column_info *cinfo, gint el)
{
  int i;

  if (cinfo && cinfo->writable) {
    /* We are constructing columns, and they're writable */
    if (cinfo->col_first[el] >= 0) {
      /* There is at least one column in that format */
      for (i = cinfo->col_first[el]; i <= cinfo->col_last[el]; i++) {
        if (cinfo->fmt_matx[i][el]) {
          cinfo->col_fence[i] = strlen(cinfo->col_data[i]);
        }
      }
    }
  }
}

/* Use this to clear out a column, especially if you're going to be
   appending to it later; at least on some platforms, it's more
   efficient than using "col_add_str()" with a null string, and
   more efficient than "col_set_str()" with a null string if you
   later append to it, as the later append will cause a string
   copy to be done. */
void
col_clear(column_info *cinfo, gint el)
{
  int    i;
  int    fence;

  g_assert(cinfo->col_first[el] >= 0);
  for (i = cinfo->col_first[el]; i <= cinfo->col_last[el]; i++) {
    if (cinfo->fmt_matx[i][el]) {
      /*
       * At this point, either
       *
       *   1) col_data[i] is equal to col_buf[i], in which case we
       *      don't have to worry about copying col_data[i] to
       *      col_buf[i];
       *
       *   2) col_data[i] isn't equal to col_buf[i], in which case
       *      the only thing that's been done to the column is
       *      "col_set_str()" calls and possibly "col_set_fence()"
       *      calls, in which case the fence is either unset and
       *      at the beginning of the string or set and at the end
       *      of the string - if it's at the beginning, we're just
       *      going to clear the column, and if it's at the end,
       *      we don't do anything.
       */
      fence = cinfo->col_fence[i];
      if (fence == 0 || cinfo->col_buf[i] == cinfo->col_data[i]) {
        /*
         * The fence isn't at the end of the column, or the column wasn't
         * last set with "col_set_str()", so clear the column out.
         */
        cinfo->col_buf[i][fence] = '\0';
        cinfo->col_data[i] = cinfo->col_buf[i];
      }
      cinfo->col_expr[i][0] = '\0';
      cinfo->col_expr_val[i][0] = '\0';
    }
  }
}

#define COL_CHECK_APPEND(cinfo, i, max_len) \
  if (cinfo->col_data[i] != cinfo->col_buf[i]) {		\
    /* This was set with "col_set_str()"; copy the string they	\
       set it to into the buffer, so we can append to it. */	\
    strncpy(cinfo->col_buf[i], cinfo->col_data[i], max_len);	\
    cinfo->col_buf[i][max_len - 1] = '\0';			\
    cinfo->col_data[i] = cinfo->col_buf[i];			\
  }

/* Use this if "str" points to something that will stay around (and thus
   needn't be copied). */
void
col_set_str(column_info *cinfo, gint el, gchar* str)
{
  int i;
  int fence;
  size_t len, max_len;

  if (el == COL_INFO)
	max_len = COL_MAX_INFO_LEN;
  else
	max_len = COL_MAX_LEN;

  g_assert(cinfo->col_first[el] >= 0);
  for (i = cinfo->col_first[el]; i <= cinfo->col_last[el]; i++) {
    if (cinfo->fmt_matx[i][el]) {
      fence = cinfo->col_fence[i];
      if (fence != 0) {
        /*
         * We will append the string after the fence.
         * First arrange that we can append, if necessary.
         */
        COL_CHECK_APPEND(cinfo, i, max_len);

        len = strlen(cinfo->col_buf[i]);
        strncat(cinfo->col_buf[i], str, max_len - len);
        cinfo->col_buf[i][max_len - 1] = 0;
      } else {
        /*
         * There's no fence, so we can just set the column to point
         * to the string.
         */
        cinfo->col_data[i] = str;
      }
    }
  }
}

/* Adds a vararg list to a packet info string. */
void
col_add_fstr(column_info *cinfo, gint el, const gchar *format, ...) {
  va_list ap;
  int     i;
  int     fence;
  size_t  max_len;

  g_assert(cinfo->col_first[el] >= 0);
  if (el == COL_INFO)
	max_len = COL_MAX_INFO_LEN;
  else
	max_len = COL_MAX_LEN;

  va_start(ap, format);
  for (i = cinfo->col_first[el]; i <= cinfo->col_last[el]; i++) {
    if (cinfo->fmt_matx[i][el]) {
      fence = cinfo->col_fence[i];
      if (fence != 0) {
        /*
         * We will append the string after the fence.
         * First arrange that we can append, if necessary.
         */
        COL_CHECK_APPEND(cinfo, i, max_len);
      } else {
        /*
         * There's no fence, so we can just write to the string.
         */
        cinfo->col_data[i] = cinfo->col_buf[i];
      }
      vsnprintf(&cinfo->col_buf[i][fence], max_len - fence, format, ap);
    }
  }
  va_end(ap);
}

/* Appends a vararg list to a packet info string. */
void
col_append_fstr(column_info *cinfo, gint el, const gchar *format, ...)
{
  va_list ap;
  int     i;
  size_t  len, max_len;

  g_assert(cinfo->col_first[el] >= 0);
  if (el == COL_INFO)
	max_len = COL_MAX_INFO_LEN;
  else
	max_len = COL_MAX_LEN;

  va_start(ap, format);
  for (i = cinfo->col_first[el]; i <= cinfo->col_last[el]; i++) {
    if (cinfo->fmt_matx[i][el]) {
      /*
       * First arrange that we can append, if necessary.
       */
      COL_CHECK_APPEND(cinfo, i, max_len);
      len = strlen(cinfo->col_buf[i]);
      vsnprintf(&cinfo->col_buf[i][len], max_len - len, format, ap);
    }
  }
  va_end(ap);
}

/* Prepends a vararg list to a packet info string. */
#define COL_BUF_MAX_LEN (((COL_MAX_INFO_LEN) > (COL_MAX_LEN)) ? \
	(COL_MAX_INFO_LEN) : (COL_MAX_LEN))
void
col_prepend_fstr(column_info *cinfo, gint el, const gchar *format, ...)
{
  va_list ap;
  int     i;
  char    orig_buf[COL_BUF_MAX_LEN];
  char   *orig;
  size_t  max_len;

  g_assert(cinfo->col_first[el] >= 0);
  if (el == COL_INFO)
	max_len = COL_MAX_INFO_LEN;
  else
	max_len = COL_MAX_LEN;

  va_start(ap, format);
  for (i = cinfo->col_first[el]; i <= cinfo->col_last[el]; i++) {
    if (cinfo->fmt_matx[i][el]) {
      if (cinfo->col_data[i] != cinfo->col_buf[i]) {
      	/* This was set with "col_set_str()"; which is effectively const */
        orig = cinfo->col_data[i];
      } else {
        orig = orig_buf;
	strncpy(orig, cinfo->col_buf[i], max_len);
	orig[max_len - 1] = '\0';
      }
      vsnprintf(cinfo->col_buf[i], max_len, format, ap);
      cinfo->col_buf[i][max_len - 1] = '\0';

      /*
       * Move the fence, unless it's at the beginning of the string.
       */
      if (cinfo->col_fence[i] > 0)
        cinfo->col_fence[i] += strlen(cinfo->col_buf[i]);

      strncat(cinfo->col_buf[i], orig, max_len);
      cinfo->col_buf[i][max_len - 1] = '\0';
      cinfo->col_data[i] = cinfo->col_buf[i];
    }
  }
  va_end(ap);
}

/* Use this if "str" points to something that won't stay around (and
   must thus be copied). */
void
col_add_str(column_info *cinfo, gint el, const gchar* str)
{
  int    i;
  int    fence;
  size_t max_len;

  g_assert(cinfo->col_first[el] >= 0);
  if (el == COL_INFO)
	max_len = COL_MAX_INFO_LEN;
  else
	max_len = COL_MAX_LEN;

  for (i = cinfo->col_first[el]; i <= cinfo->col_last[el]; i++) {
    if (cinfo->fmt_matx[i][el]) {
      fence = cinfo->col_fence[i];
      if (fence != 0) {
        /*
         * We will append the string after the fence.
         * First arrange that we can append, if necessary.
         */
        COL_CHECK_APPEND(cinfo, i, max_len);
      } else {
        /*
         * There's no fence, so we can just write to the string.
         */
        cinfo->col_data[i] = cinfo->col_buf[i];
      }
      strncpy(&cinfo->col_buf[i][fence], str, max_len - fence);
      cinfo->col_buf[i][max_len - 1] = 0;
    }
  }
}

void
col_append_str(column_info *cinfo, gint el, const gchar* str)
{
  int    i;
  size_t len, max_len;

  g_assert(cinfo->col_first[el] >= 0);
  if (el == COL_INFO)
	max_len = COL_MAX_INFO_LEN;
  else
	max_len = COL_MAX_LEN;

  for (i = cinfo->col_first[el]; i <= cinfo->col_last[el]; i++) {
    if (cinfo->fmt_matx[i][el]) {
      /*
       * First arrange that we can append, if necessary.
       */
      COL_CHECK_APPEND(cinfo, i, max_len);
      len = strlen(cinfo->col_buf[i]);
      strncat(cinfo->col_buf[i], str, max_len - len);
      cinfo->col_buf[i][max_len - 1] = 0;
    }
  }
}

static void
col_set_abs_date_time(frame_data *fd, column_info *cinfo, int col)
{
  struct tm *tmp;
  time_t then;

  then = fd->abs_secs;
  tmp = localtime(&then);
  if (tmp != NULL) {
    snprintf(cinfo->col_buf[col], COL_MAX_LEN,
             "%04d-%02d-%02d %02d:%02d:%02d.%06ld",
             tmp->tm_year + 1900,
             tmp->tm_mon + 1,
             tmp->tm_mday,
             tmp->tm_hour,
             tmp->tm_min,
             tmp->tm_sec,
             (long)fd->abs_usecs);
  } else {
    cinfo->col_buf[col][0] = '\0';
  }
  cinfo->col_data[col] = cinfo->col_buf[col];
  strcpy(cinfo->col_expr[col],"frame.time");
  strcpy(cinfo->col_expr_val[col],cinfo->col_buf[col]);
}

static void
col_set_rel_time(frame_data *fd, column_info *cinfo, int col)
{
  display_signed_time(cinfo->col_buf[col], COL_MAX_LEN,
	fd->rel_secs, fd->rel_usecs, USECS);
  cinfo->col_data[col] = cinfo->col_buf[col];
  strcpy(cinfo->col_expr[col],"frame.time_relative");
  strcpy(cinfo->col_expr_val[col],cinfo->col_buf[col]);
}

static void
col_set_delta_time(frame_data *fd, column_info *cinfo, int col)
{
  display_signed_time(cinfo->col_buf[col], COL_MAX_LEN,
	fd->del_secs, fd->del_usecs, USECS);
  cinfo->col_data[col] = cinfo->col_buf[col];
  strcpy(cinfo->col_expr[col],"frame.time_delta");
  strcpy(cinfo->col_expr_val[col],cinfo->col_buf[col]);
}

/* To do: Add check_col checks to the col_add* routines */

static void
col_set_abs_time(frame_data *fd, column_info *cinfo, int col)
{
  struct tm *tmp;
  time_t then;

  then = fd->abs_secs;
  tmp = localtime(&then);
  if (tmp != NULL) {
    snprintf(cinfo->col_buf[col], COL_MAX_LEN, "%02d:%02d:%02d.%06ld",
             tmp->tm_hour,
             tmp->tm_min,
             tmp->tm_sec,
             (long)fd->abs_usecs);
  } else {
    cinfo->col_buf[col][0] = '\0';
  }
  cinfo->col_data[col] = cinfo->col_buf[col];
  strcpy(cinfo->col_expr[col],"frame.time");
  strcpy(cinfo->col_expr_val[col],cinfo->col_buf[col]);
}

/* Add "command-line-specified" time.
   XXX - this is called from "file.c" when the user changes the time
   format they want for "command-line-specified" time; it's a bit ugly
   that we have to export it, but if we go to a CList-like widget that
   invokes callbacks to get the text for the columns rather than
   requiring us to stuff the text into the widget from outside, we
   might be able to clean this up. */
void
col_set_cls_time(frame_data *fd, column_info *cinfo, int col)
{
  switch (timestamp_type) {
    case TS_ABSOLUTE:
      col_set_abs_time(fd, cinfo, col);
      break;

    case TS_ABSOLUTE_WITH_DATE:
      col_set_abs_date_time(fd, cinfo, col);
      break;

    case TS_RELATIVE:
      col_set_rel_time(fd, cinfo, col);
      break;

    case TS_DELTA:
      col_set_delta_time(fd, cinfo, col);
      break;
  }
}

static void
col_set_addr(packet_info *pinfo, int col, address *addr, gboolean is_res,
	     gboolean is_src)
{
  guint32 ipv4_addr;
  struct e_in6_addr ipv6_addr;

  pinfo->cinfo->col_expr[col][0] = '\0';
  pinfo->cinfo->col_expr_val[col][0] = '\0';
  if (addr->type == AT_NONE)
    return;	/* no address, nothing to do */
  if (is_res) {
    switch (addr->type) {

    case AT_ETHER:
      strncpy(pinfo->cinfo->col_buf[col], get_ether_name(addr->data), COL_MAX_LEN);
      pinfo->cinfo->col_buf[col][COL_MAX_LEN - 1] = '\0';
      pinfo->cinfo->col_data[col] = pinfo->cinfo->col_buf[col];
      break;

    case AT_IPv4:
      memcpy(&ipv4_addr, addr->data, sizeof ipv4_addr);
      strncpy(pinfo->cinfo->col_buf[col], get_hostname(ipv4_addr), COL_MAX_LEN);
      pinfo->cinfo->col_buf[col][COL_MAX_LEN - 1] = '\0';
      break;

    case AT_IPv6:
      memcpy(&ipv6_addr.s6_addr, addr->data, sizeof ipv6_addr.s6_addr);
      strncpy(pinfo->cinfo->col_buf[col], get_hostname6(&ipv6_addr), COL_MAX_LEN);
      pinfo->cinfo->col_buf[col][COL_MAX_LEN - 1] = '\0';
      break;

    default:
      address_to_str_buf(addr, pinfo->cinfo->col_buf[col]);
      break;
    }
  } else
    address_to_str_buf(addr, pinfo->cinfo->col_buf[col]);
  pinfo->cinfo->col_data[col] = pinfo->cinfo->col_buf[col];

  switch (addr->type) {

  case AT_ETHER:
    if (is_src)
      strcpy(pinfo->cinfo->col_expr[col], "eth.src");
    else
      strcpy(pinfo->cinfo->col_expr[col], "eth.dst");
    strncpy(pinfo->cinfo->col_expr_val[col], ether_to_str(addr->data), COL_MAX_LEN);
    pinfo->cinfo->col_expr_val[col][COL_MAX_LEN - 1] = '\0';
    break;

  case AT_IPv4:
    if (is_src)
      strcpy(pinfo->cinfo->col_expr[col], "ip.src");
    else
      strcpy(pinfo->cinfo->col_expr[col], "ip.dst");
    strncpy(pinfo->cinfo->col_expr_val[col], ip_to_str(addr->data), COL_MAX_LEN);
    pinfo->cinfo->col_expr_val[col][COL_MAX_LEN - 1] = '\0';
    break;

  case AT_IPv6:
    if (is_src)
      strcpy(pinfo->cinfo->col_expr[col], "ipv6.src");
    else
      strcpy(pinfo->cinfo->col_expr[col], "ipv6.dst");
    strncpy(pinfo->cinfo->col_expr_val[col], ip6_to_str(&ipv6_addr), COL_MAX_LEN);
    pinfo->cinfo->col_expr_val[col][COL_MAX_LEN - 1] = '\0';
    break;

  case AT_ATALK:
    if (is_src)
      strcpy(pinfo->cinfo->col_expr[col], "ddp.src");
    else
      strcpy(pinfo->cinfo->col_expr[col], "ddp.dst");
    strcpy(pinfo->cinfo->col_expr_val[col], pinfo->cinfo->col_buf[col]);
    break;

  case AT_ARCNET:
    if (is_src)
      strcpy(pinfo->cinfo->col_expr[col], "arcnet.src");
    else
      strcpy(pinfo->cinfo->col_expr[col], "arcnet.dst");
    strcpy(pinfo->cinfo->col_expr_val[col], pinfo->cinfo->col_buf[col]);
    break;

  default:
    break;
  }
}

static void
col_set_port(packet_info *pinfo, int col, gboolean is_res, gboolean is_src)
{
  guint32 port;

  if (is_src)
    port = pinfo->srcport;
  else
    port = pinfo->destport;
  pinfo->cinfo->col_expr[col][0] = '\0';
  pinfo->cinfo->col_expr_val[col][0] = '\0';
  switch (pinfo->ptype) {

  case PT_SCTP:
    if (is_res)
      strncpy(pinfo->cinfo->col_buf[col], get_sctp_port(port), COL_MAX_LEN);
    else
      snprintf(pinfo->cinfo->col_buf[col], COL_MAX_LEN, "%u", port);
    break;

  case PT_TCP:
    if (is_res)
      strncpy(pinfo->cinfo->col_buf[col], get_tcp_port(port), COL_MAX_LEN);
    else
      snprintf(pinfo->cinfo->col_buf[col], COL_MAX_LEN, "%u", port);
    if (is_src)
      strcpy(pinfo->cinfo->col_expr[col], "tcp.srcport");
    else
      strcpy(pinfo->cinfo->col_expr[col], "tcp.dstport");
    snprintf(pinfo->cinfo->col_expr_val[col], COL_MAX_LEN, "%u", port);
    pinfo->cinfo->col_expr_val[col][COL_MAX_LEN - 1] = '\0';
    break;

  case PT_UDP:
    if (is_res)
      strncpy(pinfo->cinfo->col_buf[col], get_udp_port(port), COL_MAX_LEN);
    else
      snprintf(pinfo->cinfo->col_buf[col], COL_MAX_LEN, "%u", port);
    if (is_src)
      strcpy(pinfo->cinfo->col_expr[col], "udp.srcport");
    else
      strcpy(pinfo->cinfo->col_expr[col], "udp.dstport");
    snprintf(pinfo->cinfo->col_expr_val[col], COL_MAX_LEN, "%u", port);
    pinfo->cinfo->col_expr_val[col][COL_MAX_LEN - 1] = '\0';
    break;

  case PT_DDP:
    if (is_src)
      strcpy(pinfo->cinfo->col_expr[col], "ddp.src_socket");
    else
      strcpy(pinfo->cinfo->col_expr[col], "ddp.dst_socket");
    snprintf(pinfo->cinfo->col_buf[col], COL_MAX_LEN, "%u", port);
    snprintf(pinfo->cinfo->col_expr_val[col], COL_MAX_LEN, "%u", port);
    pinfo->cinfo->col_expr_val[col][COL_MAX_LEN - 1] = '\0';
    break;

  case PT_IPX:
    /* XXX - resolve IPX socket numbers */
    snprintf(pinfo->cinfo->col_buf[col], COL_MAX_LEN, "0x%04x", port);
    if (is_src)
      strcpy(pinfo->cinfo->col_expr[col], "ipx.src.socket");
    else
      strcpy(pinfo->cinfo->col_expr[col], "ipx.dst.socket");
    snprintf(pinfo->cinfo->col_expr_val[col], COL_MAX_LEN, "0x%04x", port);
    pinfo->cinfo->col_expr_val[col][COL_MAX_LEN - 1] = '\0';
    break;

  default:
    break;
  }
  pinfo->cinfo->col_buf[col][COL_MAX_LEN - 1] = '\0';
  pinfo->cinfo->col_data[col] = pinfo->cinfo->col_buf[col];
}

/*
 * XXX - this should be in some common code in the epan directory, shared
 * by this code and packet-isdn.c.
 */
static const value_string channel_vals[] = {
	{ 0,	"D" },
	{ 1,	"B1" },
	{ 2,	"B2" },
	{ 3,	"B3" },
	{ 4,	"B4" },
	{ 5,	"B5" },
	{ 6,	"B6" },
	{ 7,	"B7" },
	{ 8,	"B8" },
	{ 9,	"B9" },
	{ 10,	"B10" },
	{ 11,	"B11" },
	{ 12,	"B12" },
	{ 13,	"B13" },
	{ 14,	"B14" },
	{ 15,	"B15" },
	{ 16,	"B16" },
	{ 17,	"B17" },
	{ 18,	"B19" },
	{ 19,	"B19" },
	{ 20,	"B20" },
	{ 21,	"B21" },
	{ 22,	"B22" },
	{ 23,	"B23" },
	{ 24,	"B24" },
	{ 25,	"B25" },
	{ 26,	"B26" },
	{ 27,	"B27" },
	{ 28,	"B29" },
	{ 29,	"B29" },
	{ 30,	"B30" },
	{ 0,	NULL }
};

static void
col_set_circuit_id(packet_info *pinfo, int col)
{
  pinfo->cinfo->col_expr[col][0] = '\0';
  pinfo->cinfo->col_expr_val[col][0] = '\0';
  switch (pinfo->ctype) {

  case CT_DLCI:
    snprintf(pinfo->cinfo->col_buf[col], COL_MAX_LEN, "%u", pinfo->circuit_id);
    strcpy(pinfo->cinfo->col_expr[col], "fr.dlci");
    snprintf(pinfo->cinfo->col_expr_val[col], COL_MAX_LEN, "%u", pinfo->circuit_id);
    pinfo->cinfo->col_expr_val[col][COL_MAX_LEN - 1] = '\0';
    break;

  case CT_ISDN:
    snprintf(pinfo->cinfo->col_buf[col], COL_MAX_LEN, "%s",
	     val_to_str(pinfo->circuit_id, channel_vals, "Unknown (%u)"));
    strcpy(pinfo->cinfo->col_expr[col], "isdn.channel");
    snprintf(pinfo->cinfo->col_expr_val[col], COL_MAX_LEN, "%u", pinfo->circuit_id);
    pinfo->cinfo->col_expr_val[col][COL_MAX_LEN - 1] = '\0';
    break;

  case CT_X25:
    snprintf(pinfo->cinfo->col_buf[col], COL_MAX_LEN, "%u", pinfo->circuit_id);
    break;

  default:
    break;
  }
  pinfo->cinfo->col_buf[col][COL_MAX_LEN - 1] = '\0';
  pinfo->cinfo->col_data[col] = pinfo->cinfo->col_buf[col];
}

void
fill_in_columns(packet_info *pinfo)
{
  int i;

  for (i = 0; i < pinfo->cinfo->num_cols; i++) {
    switch (pinfo->cinfo->col_fmt[i]) {

    case COL_NUMBER:
      snprintf(pinfo->cinfo->col_buf[i], COL_MAX_LEN, "%u", pinfo->fd->num);
      pinfo->cinfo->col_data[i] = pinfo->cinfo->col_buf[i];
      strcpy(pinfo->cinfo->col_expr[i], "frame.number");
      strcpy(pinfo->cinfo->col_expr_val[i], pinfo->cinfo->col_buf[i]);
      break;

    case COL_CLS_TIME:
      if(pinfo->fd->flags.ref_time){
         snprintf(pinfo->cinfo->col_buf[i], COL_MAX_LEN, "*REF*");
         pinfo->cinfo->col_data[i] = pinfo->cinfo->col_buf[i];
      } else {
         col_set_cls_time(pinfo->fd, pinfo->cinfo, i);
      }
      break;

    case COL_ABS_TIME:
      if(pinfo->fd->flags.ref_time){
         snprintf(pinfo->cinfo->col_buf[i], COL_MAX_LEN, "*REF*");
         pinfo->cinfo->col_data[i] = pinfo->cinfo->col_buf[i];
      } else {
         col_set_abs_time(pinfo->fd, pinfo->cinfo, i);
      }
      break;

    case COL_ABS_DATE_TIME:
      if(pinfo->fd->flags.ref_time){
         snprintf(pinfo->cinfo->col_buf[i], COL_MAX_LEN, "*REF*");
         pinfo->cinfo->col_data[i] = pinfo->cinfo->col_buf[i];
      } else {
         col_set_abs_date_time(pinfo->fd, pinfo->cinfo, i);
      }
      break;

    case COL_REL_TIME:
      if(pinfo->fd->flags.ref_time){
         snprintf(pinfo->cinfo->col_buf[i], COL_MAX_LEN, "*REF*");
         pinfo->cinfo->col_data[i] = pinfo->cinfo->col_buf[i];
      } else {
         col_set_rel_time(pinfo->fd, pinfo->cinfo, i);
      }
      break;

    case COL_DELTA_TIME:
      if(pinfo->fd->flags.ref_time){
         snprintf(pinfo->cinfo->col_buf[i], COL_MAX_LEN, "*REF*");
         pinfo->cinfo->col_data[i] = pinfo->cinfo->col_buf[i];
      } else {
         col_set_delta_time(pinfo->fd, pinfo->cinfo, i);
      }
      break;

    case COL_DEF_SRC:
    case COL_RES_SRC:	/* COL_DEF_SRC is currently just like COL_RES_SRC */
      col_set_addr(pinfo, i, &pinfo->src, TRUE, TRUE);
      break;

    case COL_UNRES_SRC:
      col_set_addr(pinfo, i, &pinfo->src, FALSE, TRUE);
      break;

    case COL_DEF_DL_SRC:
    case COL_RES_DL_SRC:
      col_set_addr(pinfo, i, &pinfo->dl_src, TRUE, TRUE);
      break;

    case COL_UNRES_DL_SRC:
      col_set_addr(pinfo, i, &pinfo->dl_src, FALSE, TRUE);
      break;

    case COL_DEF_NET_SRC:
    case COL_RES_NET_SRC:
      col_set_addr(pinfo, i, &pinfo->net_src, TRUE, TRUE);
      break;

    case COL_UNRES_NET_SRC:
      col_set_addr(pinfo, i, &pinfo->net_src, FALSE, TRUE);
      break;

    case COL_DEF_DST:
    case COL_RES_DST:	/* COL_DEF_DST is currently just like COL_RES_DST */
      col_set_addr(pinfo, i, &pinfo->dst, TRUE, FALSE);
      break;

    case COL_UNRES_DST:
      col_set_addr(pinfo, i, &pinfo->dst, FALSE, FALSE);
      break;

    case COL_DEF_DL_DST:
    case COL_RES_DL_DST:
      col_set_addr(pinfo, i, &pinfo->dl_dst, TRUE, FALSE);
      break;

    case COL_UNRES_DL_DST:
      col_set_addr(pinfo, i, &pinfo->dl_dst, FALSE, FALSE);
      break;

    case COL_DEF_NET_DST:
    case COL_RES_NET_DST:
      col_set_addr(pinfo, i, &pinfo->net_dst, TRUE, FALSE);
      break;

    case COL_UNRES_NET_DST:
      col_set_addr(pinfo, i, &pinfo->net_dst, FALSE, FALSE);
      break;

    case COL_DEF_SRC_PORT:
    case COL_RES_SRC_PORT:	/* COL_DEF_SRC_PORT is currently just like COL_RES_SRC_PORT */
      col_set_port(pinfo, i, TRUE, TRUE);
      break;

    case COL_UNRES_SRC_PORT:
      col_set_port(pinfo, i, FALSE, TRUE);
      break;

    case COL_DEF_DST_PORT:
    case COL_RES_DST_PORT:	/* COL_DEF_DST_PORT is currently just like COL_RES_DST_PORT */
      col_set_port(pinfo, i, TRUE, FALSE);
      break;

    case COL_UNRES_DST_PORT:
      col_set_port(pinfo, i, FALSE, FALSE);
      break;

    case COL_PROTOCOL:	/* currently done by dissectors */
    case COL_INFO:	/* currently done by dissectors */
      break;

    case COL_PACKET_LENGTH:
      snprintf(pinfo->cinfo->col_buf[i], COL_MAX_LEN, "%u", pinfo->fd->pkt_len);
      pinfo->cinfo->col_data[i] = pinfo->cinfo->col_buf[i];
      strcpy(pinfo->cinfo->col_expr[i], "frame.pkt_len");
      strcpy(pinfo->cinfo->col_expr_val[i], pinfo->cinfo->col_buf[i]);
      break;
    case COL_CULMULATIVE_BYTES:
      snprintf(pinfo->cinfo->col_buf[i], COL_MAX_LEN, "%u", pinfo->fd->cul_bytes);
      pinfo->cinfo->col_data[i] = pinfo->cinfo->col_buf[i];
      break;

    case COL_OXID:
      snprintf(pinfo->cinfo->col_buf[i], COL_MAX_LEN, "0x%x", pinfo->oxid);
      pinfo->cinfo->col_buf[i][COL_MAX_LEN - 1] = '\0';
      pinfo->cinfo->col_data[i] = pinfo->cinfo->col_buf[i];
      break;

    case COL_RXID:
      snprintf(pinfo->cinfo->col_buf[i], COL_MAX_LEN, "0x%x", pinfo->rxid);
      pinfo->cinfo->col_buf[i][COL_MAX_LEN - 1] = '\0';
      pinfo->cinfo->col_data[i] = pinfo->cinfo->col_buf[i];
      break;

    case COL_IF_DIR:	/* currently done by dissectors */
      break;

    case COL_CIRCUIT_ID:
      col_set_circuit_id(pinfo, i);
      break;

    case COL_SRCIDX:
        snprintf (pinfo->cinfo->col_buf[i], COL_MAX_LEN, "0x%x",
                  pinfo->src_idx);
        pinfo->cinfo->col_buf[i][COL_MAX_LEN - 1] = '\0';
        pinfo->cinfo->col_data[i] = pinfo->cinfo->col_buf[i];
        break;

    case COL_DSTIDX:
        snprintf (pinfo->cinfo->col_buf[i], COL_MAX_LEN, "0x%x",
                  pinfo->dst_idx);
        pinfo->cinfo->col_buf[i][COL_MAX_LEN - 1] = '\0';
        pinfo->cinfo->col_data[i] = pinfo->cinfo->col_buf[i];
        break;

    case COL_VSAN:
        snprintf (pinfo->cinfo->col_buf[i], COL_MAX_LEN, "%d", pinfo->vsan);
        pinfo->cinfo->col_buf[i][COL_MAX_LEN - 1] = '\0';
        pinfo->cinfo->col_data[i] = pinfo->cinfo->col_buf[i];
        break;
        
    case NUM_COL_FMTS:	/* keep compiler happy - shouldn't get here */
      g_assert_not_reached();
      break;
    }
  }
}
