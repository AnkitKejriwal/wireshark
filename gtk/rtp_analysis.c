/* rtp_analysis.c
 * RTP analysis addition for ethereal
 *
 * $Id: rtp_analysis.c,v 1.42 2004/05/23 23:24:06 ulfl Exp $
 *
 * Copyright 2003, Alcatel Business Systems
 * By Lars Ruoff <lars.ruoff@gmx.net>
 *
 * based on tap_rtp.c
 * Copyright 2003, Iskratel, Ltd, Kranj
 * By Miha Jemec <m.jemec@iskratel.si>
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
 * Foundation,  Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/*do not define this symbol. will be added soon*/
/*#define USE_CONVERSATION_GRAPH 1*/

#include "rtp_analysis.h"
#include "rtp_stream.h"
#include "rtp_stream_dlg.h"

#ifdef USE_CONVERSATION_GRAPH
#include "../graph/graph.h"
#endif

#include <epan/epan_dissect.h>
#include <epan/filesystem.h>

#include "util.h"
#include "tap.h"
#include "register.h"
#include "packet-rtp.h"
#include "g711.h"
#include "rtp_pt.h"

#ifdef NEED_MKSTEMP
#include "mkstemp.h"
#endif

/* in /gtk ... */
#include "dlg_utils.h"
#include "ui_util.h"
#include "alert_box.h"
#include "simple_dialog.h"
#include "tap_menu.h"
#include "main.h"
#include "progress_dlg.h"
#include "compat_macros.h"

#include "image/clist_ascend.xpm"
#include "image/clist_descend.xpm"

#include <math.h>
#include <fcntl.h>
#include <string.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_IO_H
#include <io.h> /* open/close on win32 */
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

/****************************************************************************/

typedef struct column_arrows {
	GtkWidget *table;
	GtkWidget *ascend_pm;
	GtkWidget *descend_pm;
} column_arrows;

typedef struct _dialog_data_t {
	GtkWidget *window;
	GtkCList *clist_fwd;
	GtkCList *clist_rev;
	GtkWidget *label_stats_fwd;
	GtkWidget *label_stats_rev;
	column_arrows *col_arrows_fwd;
	column_arrows *col_arrows_rev;
	GtkWidget *notebook;
	GtkCList *selected_clist;
	GtkWidget *save_voice_as_w;
	GtkWidget *save_csv_as_w;
	gint selected_row;
#ifdef USE_CONVERSATION_GRAPH
	GtkWidget *graph_window;
#endif
} dialog_data_t;

#define OK_TEXT "[ Ok ]"
#define PT_UNDEFINED -1

/* type of error when saving voice in a file didn't succeed */
typedef enum {
	TAP_RTP_WRONG_CODEC,
	TAP_RTP_WRONG_LENGTH,
	TAP_RTP_PADDING_ERROR,
	TAP_RTP_SHORT_FRAME,
	TAP_RTP_FILE_OPEN_ERROR,
	TAP_RTP_NO_DATA
} error_type_t; 


/****************************************************************************/
/* structure that holds the information about the forward and reversed direction */
typedef struct _tap_rtp_stat_t {
	gboolean first_packet;     /* do not use in code that is called after rtp_packet_analyse */
	                           /* use (flags & STAT_FLAG_FIRST) instead */
	/* all of the following fields will be initialized after
	 rtp_packet_analyse has been called */
	guint32 flags;             /* see STAT_FLAG-defines below */
	guint16 seq_num;
	guint32 timestamp;
	guint32 delta_timestamp;
	double delay;
	double jitter;
	double time;
	double start_time;
	double max_delay;
	guint32 max_nr;
	guint16 start_seq_nr;
	guint16 stop_seq_nr;
	guint32 total_nr;
	guint32 sequence;
	gboolean under;
	gint cycles;
	guint16 pt;
	int reg_pt;
} tap_rtp_stat_t;

/* status flags for the flags parameter in tap_rtp_stat_t */
#define STAT_FLAG_FIRST       0x01
#define STAT_FLAG_MARKER      0x02
#define STAT_FLAG_WRONG_SEQ   0x04
#define STAT_FLAG_PT_CHANGE   0x08
#define STAT_FLAG_PT_CN       0x10
#define STAT_FLAG_FOLLOW_PT_CN  0x20
#define STAT_FLAG_REG_PT_CHANGE  0x40

typedef struct _tap_rtp_save_info_t {
	FILE *fp;
	guint32 count;
	error_type_t error_type;
	gboolean saved;
} tap_rtp_save_info_t;


/* structure that holds the information about the forward and reversed direction */
struct _info_direction {
	tap_rtp_stat_t statinfo;
	tap_rtp_save_info_t saveinfo;
};

#define TMPNAMSIZE 100

/* structure that holds general information about the connection 
* and structures for both directions */
typedef struct _user_data_t {
	/* tap associated data*/
	guint32 ip_src_fwd;
	guint16 port_src_fwd;
	guint32 ip_dst_fwd;
	guint16 port_dst_fwd;
	guint32 ssrc_fwd;
	guint32 ip_src_rev;
	guint16 port_src_rev;
	guint32 ip_dst_rev;
	guint16 port_dst_rev;
	guint32 ssrc_rev;

	struct _info_direction forward;
	struct _info_direction reversed;

	char f_tempname[TMPNAMSIZE];
	char r_tempname[TMPNAMSIZE];

	/* dialog associated data */
	dialog_data_t dlg;

#ifdef USE_CONVERSATION_GRAPH
	time_series_t series_fwd;
	time_series_t series_rev;
#endif
} user_data_t;


/* Column titles. */
static gchar *titles[8] =  {
	"Packet",
	"Sequence",
	"Delay (s)",
	"Jitter (s)",
	"Marker",
	"Status",
	"Date",
	"Length"
};


typedef const guint8 * ip_addr_p;


/****************************************************************************/
/* TAP FUNCTIONS */

/****************************************************************************/
/* when there is a [re]reading of packet's */
static void
rtp_reset(void *user_data_arg)
{
	user_data_t *user_data = user_data_arg;

	user_data->forward.statinfo.first_packet = TRUE;
	user_data->reversed.statinfo.first_packet = TRUE;
	user_data->forward.statinfo.max_delay = 0;
	user_data->reversed.statinfo.max_delay = 0;
	user_data->forward.statinfo.delay = 0;
	user_data->reversed.statinfo.delay = 0;
	user_data->forward.statinfo.jitter = 0;
	user_data->reversed.statinfo.jitter = 0;
	user_data->forward.statinfo.timestamp = 0;
	user_data->reversed.statinfo.timestamp = 0;
	user_data->forward.statinfo.max_nr = 0;
	user_data->reversed.statinfo.max_nr = 0;
	user_data->forward.statinfo.total_nr = 0;
	user_data->reversed.statinfo.total_nr = 0;
	user_data->forward.statinfo.sequence = 0;
	user_data->reversed.statinfo.sequence = 0;
	user_data->forward.statinfo.start_seq_nr = 0;
	user_data->reversed.statinfo.start_seq_nr = 1; /* 1 is ok (for statistics in reversed direction) */
	user_data->forward.statinfo.stop_seq_nr = 0;
	user_data->reversed.statinfo.stop_seq_nr = 0;
	user_data->forward.statinfo.cycles = 0;
	user_data->reversed.statinfo.cycles = 0;
	user_data->forward.statinfo.under = FALSE;
	user_data->reversed.statinfo.under = FALSE;
	user_data->forward.statinfo.start_time = 0;
	user_data->reversed.statinfo.start_time = 0;
	user_data->forward.statinfo.time = 0;
	user_data->reversed.statinfo.time = 0;
	user_data->forward.statinfo.reg_pt = PT_UNDEFINED;
	user_data->reversed.statinfo.reg_pt = PT_UNDEFINED;

	user_data->forward.saveinfo.count = 0;
	user_data->reversed.saveinfo.count = 0;
	user_data->forward.saveinfo.saved = FALSE;
	user_data->reversed.saveinfo.saved = FALSE;

#ifdef USE_CONVERSATION_GRAPH
	if (user_data->dlg.graph_window != NULL)
		gtk_widget_destroy(user_data->dlg.graph_window);
	
	g_array_free(user_data->series_fwd.value_pairs, TRUE);
	user_data->series_fwd.value_pairs = g_array_new(FALSE, FALSE, sizeof(value_pair_t));

	g_array_free(user_data->series_rev.value_pairs, TRUE);
	user_data->series_rev.value_pairs = g_array_new(FALSE, FALSE, sizeof(value_pair_t));
#endif

	/* XXX check for error at fclose? */
	if (user_data->forward.saveinfo.fp != NULL)
		fclose(user_data->forward.saveinfo.fp); 
	if (user_data->reversed.saveinfo.fp != NULL)
		fclose(user_data->reversed.saveinfo.fp); 
	user_data->forward.saveinfo.fp = fopen(user_data->f_tempname, "wb"); 
	if (user_data->forward.saveinfo.fp == NULL)
		user_data->forward.saveinfo.error_type = TAP_RTP_FILE_OPEN_ERROR;
	user_data->reversed.saveinfo.fp = fopen(user_data->r_tempname, "wb");
	if (user_data->reversed.saveinfo.fp == NULL)
		user_data->reversed.saveinfo.error_type = TAP_RTP_FILE_OPEN_ERROR;
	return;
}

/****************************************************************************/
/* here we can redraw the output */
/* not used yet */
static void rtp_draw(void *prs _U_)
{
	return;
}

/* forward declarations */
static void add_to_clist(GtkCList *clist, guint32 number, guint16 seq_num,
                         double delay, double jitter, gchar *status, gboolean marker,
                         gchar *timeStr, guint32 pkt_len, GdkColor *color);

static int rtp_packet_analyse(tap_rtp_stat_t *statinfo,
							  packet_info *pinfo, struct _rtp_info *rtpinfo);
static int rtp_packet_add_info(GtkCList *clist,
	tap_rtp_stat_t *statinfo, packet_info *pinfo, struct _rtp_info *rtpinfo);
static int rtp_packet_save_payload(tap_rtp_save_info_t *saveinfo, 
								   tap_rtp_stat_t *statinfo,
								   packet_info *pinfo, struct _rtp_info *rtpinfo);


/****************************************************************************/
/* whenever a RTP packet is seen by the tap listener */
static int rtp_packet(void *user_data_arg, packet_info *pinfo, epan_dissect_t *edt _U_, void *rtpinfo_arg)
{
	user_data_t *user_data = user_data_arg;
	struct _rtp_info *rtpinfo = rtpinfo_arg;
#ifdef USE_CONVERSATION_GRAPH
	value_pair_t vp;
#endif

	/* we ignore packets that are not displayed */
	if (pinfo->fd->flags.passed_dfilter == 0)
		return 0;

	/* is it the forward direction?  */
	else if (user_data->ssrc_fwd == rtpinfo->info_sync_src)  {
#ifdef USE_CONVERSATION_GRAPH
		vp.time = ((double)pinfo->fd->rel_secs + (double)pinfo->fd->rel_usecs/1000000);
		vp.fnumber = pinfo->fd->num;
		g_array_append_val(user_data->series_fwd.value_pairs, vp);
#endif
		rtp_packet_analyse(&(user_data->forward.statinfo), pinfo, rtpinfo);
		rtp_packet_add_info(user_data->dlg.clist_fwd,
			&(user_data->forward.statinfo), pinfo, rtpinfo);
		rtp_packet_save_payload(&(user_data->forward.saveinfo),
			&(user_data->forward.statinfo), pinfo, rtpinfo);
	}
	/* is it the reversed direction? */
	else if (user_data->ssrc_rev == rtpinfo->info_sync_src) {
#ifdef USE_CONVERSATION_GRAPH
		vp.time = ((double)pinfo->fd->rel_secs + (double)pinfo->fd->rel_usecs/1000000);
		vp.fnumber = pinfo->fd->num;
		g_array_append_val(user_data->series_rev.value_pairs, vp);
#endif
		rtp_packet_analyse(&(user_data->reversed.statinfo), pinfo, rtpinfo);
		rtp_packet_add_info(user_data->dlg.clist_rev,
			&(user_data->reversed.statinfo), pinfo, rtpinfo);
		rtp_packet_save_payload(&(user_data->reversed.saveinfo),
			&(user_data->reversed.statinfo), pinfo, rtpinfo);
	}

	return 0;
}


/****************************************************************************/
static int rtp_packet_analyse(tap_rtp_stat_t *statinfo,
							  packet_info *pinfo, struct _rtp_info *rtpinfo)
{
	double current_time;
	double current_jitter;

	statinfo->flags = 0;

	/* check payload type */
	if (rtpinfo->info_payload_type == PT_CN
		|| rtpinfo->info_payload_type == PT_CN_OLD)
		statinfo->flags |= STAT_FLAG_PT_CN;
	if (statinfo->pt == PT_CN
		|| statinfo->pt == PT_CN_OLD)
		statinfo->flags |= STAT_FLAG_FOLLOW_PT_CN;
	if (rtpinfo->info_payload_type != statinfo->pt)
		statinfo->flags |= STAT_FLAG_PT_CHANGE;

	statinfo->pt = rtpinfo->info_payload_type;

	/* store the current time and calculate the current jitter */
	current_time = (double)pinfo->fd->rel_secs + (double) pinfo->fd->rel_usecs/1000000;
	current_jitter = statinfo->jitter + ( fabs (current_time - (statinfo->time) -
		((double)(rtpinfo->info_timestamp)-(double)(statinfo->timestamp))/8000)- statinfo->jitter)/16;
	statinfo->delay = current_time-(statinfo->time);
	statinfo->jitter = current_jitter;

	/*  is this the first packet we got in this direction? */
	if (statinfo->first_packet) {
		statinfo->start_seq_nr = rtpinfo->info_seq_num;
		statinfo->start_time = current_time;
		statinfo->delay = 0;
		statinfo->jitter = 0;
		statinfo->flags |= STAT_FLAG_FIRST;
		statinfo->first_packet = FALSE;
	}
	/* is it a packet with the mark bit set? */
	if (rtpinfo->info_marker_set) {
		statinfo->delta_timestamp = rtpinfo->info_timestamp - statinfo->timestamp;
		statinfo->flags |= STAT_FLAG_MARKER;
	}
	/* is it a regular packet? */
	if (!(statinfo->flags & STAT_FLAG_FIRST)
		&& !(statinfo->flags & STAT_FLAG_MARKER)
		&& !(statinfo->flags & STAT_FLAG_PT_CN)
		&& !(statinfo->flags & STAT_FLAG_FOLLOW_PT_CN)) {
		/* include it in maximum delay calculation */
		if (statinfo->delay > statinfo->max_delay) {
			statinfo->max_delay = statinfo->delay;
			statinfo->max_nr = pinfo->fd->num;
		}
	}
	/* regular payload change? (CN ignored) */
	if (!(statinfo->flags & STAT_FLAG_FIRST)
		&& !(statinfo->flags & STAT_FLAG_PT_CN)) {
		if ((statinfo->pt != statinfo->reg_pt)
			&& (statinfo->reg_pt != PT_UNDEFINED)) {
			statinfo->flags |= STAT_FLAG_REG_PT_CHANGE;
		}
	}

	/* set regular payload*/
	if (!(statinfo->flags & STAT_FLAG_PT_CN)) {
		statinfo->reg_pt = statinfo->pt;
	}


	/* When calculating expected rtp packets the seq number can wrap around
	* so we have to count the number of cycles
	* Variable cycles counts the wraps around in forwarding connection and
	* under is flag that indicates where we are
	*
	* XXX how to determine number of cycles with all possible lost, late
	* and duplicated packets without any doubt? It seems to me, that
	* because of all possible combination of late, duplicated or lost
	* packets, this can only be more or less good approximation
	*
	* There are some combinations (rare but theoretically possible),
	* where below code won't work correctly - statistic may be wrong then.
	*/

	/* so if the current sequence number is less than the start one
	* we assume, that there is another cycle running */
	if ((rtpinfo->info_seq_num < statinfo->start_seq_nr) && (statinfo->under == FALSE)){
		statinfo->cycles++;
		statinfo->under = TRUE;
	}
	/* what if the start seq nr was 0? Then the above condition will never
	* be true, so we add another condition. XXX The problem would arise
	* if one of the packets with seq nr 0 or 65535 would be lost or late */
	else if ((rtpinfo->info_seq_num == 0) && (statinfo->stop_seq_nr == 65535) &&
		(statinfo->under == FALSE)){
		statinfo->cycles++;
		statinfo->under = TRUE;
	}
	/* the whole round is over, so reset the flag */
	else if ((rtpinfo->info_seq_num > statinfo->start_seq_nr) && (statinfo->under != FALSE)) {
		statinfo->under = FALSE;
	}

	/* Since it is difficult to count lost, duplicate or late packets separately,
	* we would like to know at least how many times the sequence number was not ok */

	/* if the current seq number equals the last one or if we are here for
	* the first time, then it is ok, we just store the current one as the last one */
	if ( (statinfo->seq_num+1 == rtpinfo->info_seq_num) || (statinfo->flags & STAT_FLAG_FIRST) )
		statinfo->seq_num = rtpinfo->info_seq_num;
	/* if the first one is 65535. XXX same problem as above: if seq 65535 or 0 is lost... */
	else if ( (statinfo->seq_num == 65535) && (rtpinfo->info_seq_num == 0) )
		statinfo->seq_num = rtpinfo->info_seq_num;
	/* lost packets */
	else if (statinfo->seq_num+1 < rtpinfo->info_seq_num) {
		statinfo->seq_num = rtpinfo->info_seq_num;
		statinfo->sequence++;
		statinfo->flags |= STAT_FLAG_WRONG_SEQ;
	}
	/* late or duplicated */
	else if (statinfo->seq_num+1 > rtpinfo->info_seq_num) {
		statinfo->sequence++;
		statinfo->flags |= STAT_FLAG_WRONG_SEQ;
	}

	statinfo->time = current_time;
	statinfo->timestamp = rtpinfo->info_timestamp;
	statinfo->stop_seq_nr = rtpinfo->info_seq_num;
	statinfo->total_nr++;

	return 0;
}


static const GdkColor COLOR_DEFAULT = {0, 0xffff, 0xffff, 0xffff};
static const GdkColor COLOR_ERROR = {0, 0xffff, 0xbfff, 0xbfff};
static const GdkColor COLOR_WARNING = {0, 0xffff, 0xdfff, 0xbfff};
static const GdkColor COLOR_CN = {0, 0xbfff, 0xbfff, 0xffff};

/****************************************************************************/
/* adds statistics information from the packet to the clist */
static int rtp_packet_add_info(GtkCList *clist,
	tap_rtp_stat_t *statinfo, packet_info *pinfo, struct _rtp_info *rtpinfo)
{
	guint16 msecs;
	gchar timeStr[32];
	struct tm *tm_tmp;
	time_t then;
	gchar status[40];
	GdkColor color = COLOR_DEFAULT;

	then = pinfo->fd->abs_secs;
	msecs = (guint16)(pinfo->fd->abs_usecs/1000);
	tm_tmp = localtime(&then);
	g_snprintf(timeStr,sizeof(timeStr),"%02d/%02d/%04d %02d:%02d:%02d.%03d",
		tm_tmp->tm_mon + 1,
		tm_tmp->tm_mday,
		tm_tmp->tm_year + 1900,
		tm_tmp->tm_hour,
		tm_tmp->tm_min,
		tm_tmp->tm_sec,
		msecs);

	if (statinfo->pt == PT_CN) {
		g_snprintf(status,sizeof(status),"Comfort noise (PT=13, RFC 3389)");
		color = COLOR_CN;
	}
	else if (statinfo->pt == PT_CN_OLD) {
		g_snprintf(status,sizeof(status),"Comfort noise (PT=19, reserved)");
		color = COLOR_CN;
	}
	else if (statinfo->flags & STAT_FLAG_WRONG_SEQ) {
		g_snprintf(status,sizeof(status),"Wrong sequence nr.");
		color = COLOR_ERROR;
	}
	else if (statinfo->flags & STAT_FLAG_REG_PT_CHANGE) {
		g_snprintf(status,sizeof(status),"Payload changed to PT=%u", statinfo->pt);
		color = COLOR_WARNING;
	}
	else if ((statinfo->flags & STAT_FLAG_PT_CHANGE)
		&&  !(statinfo->flags & STAT_FLAG_FIRST)
		&&  !(statinfo->flags & STAT_FLAG_PT_CN)
		&&  (statinfo->flags & STAT_FLAG_FOLLOW_PT_CN)
		&&  !(statinfo->flags & STAT_FLAG_MARKER)) {
		g_snprintf(status,sizeof(status),"Marker missing?");
		color = COLOR_WARNING;
	}
	else {
		if (statinfo->flags & STAT_FLAG_MARKER) {
			color = COLOR_WARNING;
		}
		g_snprintf(status,sizeof(status),OK_TEXT);
	}

	/*  is this the first packet we got in this direction? */
	if (statinfo->flags & STAT_FLAG_FIRST) {
		add_to_clist(clist,
			pinfo->fd->num, rtpinfo->info_seq_num,
			0,
			0,
			status,
			rtpinfo->info_marker_set,
			timeStr, pinfo->fd->pkt_len,
			&color);
	}
	else {
		add_to_clist(clist,
			pinfo->fd->num, rtpinfo->info_seq_num,
			statinfo->delay,
			statinfo->jitter,
			status,
			rtpinfo->info_marker_set,
			timeStr, pinfo->fd->pkt_len,
			&color);
	}

	return 0;
}


/****************************************************************************/
static int rtp_packet_save_payload(tap_rtp_save_info_t *saveinfo, 
								   tap_rtp_stat_t *statinfo,
								   packet_info *pinfo, struct _rtp_info *rtpinfo)
{
	guint i;
	const guint8 *data;
	gint16 tmp;

	/*  is this the first packet we got in this direction? */
	if (statinfo->flags & STAT_FLAG_FIRST) {
		if (saveinfo->fp == NULL) {
			saveinfo->saved = FALSE;
			saveinfo->error_type = TAP_RTP_FILE_OPEN_ERROR;
		}
		else
			saveinfo->saved = TRUE;
	}

	/* save the voice information */
	/* if there was already an error, we quit */
	if (saveinfo->saved == FALSE)
		return 0;

	/* if the captured length and packet length aren't equal, we quit
	* because there is some information missing */
	if (pinfo->fd->pkt_len != pinfo->fd->cap_len) {
		saveinfo->saved = FALSE;
		saveinfo->error_type = TAP_RTP_WRONG_LENGTH;
		return 0;
	}

	/* if padding bit is set, but the padding count is bigger
	* then the whole RTP data - error with padding count */
	if ( (rtpinfo->info_padding_set != FALSE) &&
		(rtpinfo->info_padding_count > rtpinfo->info_payload_len) ) {
		saveinfo->saved = FALSE;
		saveinfo->error_type = TAP_RTP_PADDING_ERROR;
		return 0;
	}

	/* do we need to insert some silence? */
	if ((rtpinfo->info_marker_set) &&
		!(statinfo->flags & STAT_FLAG_FIRST) &&
		(statinfo->delta_timestamp > (rtpinfo->info_payload_len - rtpinfo->info_padding_count)) )  {
		/* the amount of silence should be the difference between
		* the last timestamp and the current one minus x
		* x should equal the amount of information in the last frame
		* XXX not done yet */
		for(i=0; i < (statinfo->delta_timestamp - rtpinfo->info_payload_len -
			rtpinfo->info_padding_count); i++) {
			tmp = (gint16 )ulaw2linear((unsigned char)(0x55));
			fwrite(&tmp, 2, 1, saveinfo->fp);
			saveinfo->count++;
		}
		fflush(saveinfo->fp);
	}

	/* ulaw? */
	if (rtpinfo->info_payload_type == PT_PCMU) {
		if (!rtpinfo->info_all_data_present) {
			/* Not all the data was captured. */
			saveinfo->saved = FALSE;
			saveinfo->error_type = TAP_RTP_SHORT_FRAME;
			return 0;
		}

		/* we put the pointer at the beginning of the RTP
		* payload, that is, at the beginning of the RTP data
		* plus the offset of the payload from the beginning
		* of the RTP data */
		data = rtpinfo->info_data + rtpinfo->info_payload_offset;
		for(i=0; i < (rtpinfo->info_payload_len - rtpinfo->info_padding_count); i++, data++) {
			tmp = (gint16 )ulaw2linear((unsigned char)*data);
			fwrite(&tmp, 2, 1, saveinfo->fp);
			saveinfo->count++;
		}
		fflush(saveinfo->fp);
		saveinfo->saved = TRUE;
		return 0;
	}

	/* alaw? */
	else if (rtpinfo->info_payload_type == PT_PCMA) {
		if (!rtpinfo->info_all_data_present) {
			/* Not all the data was captured. */
			saveinfo->saved = FALSE;
			saveinfo->error_type = TAP_RTP_SHORT_FRAME;
			return 0;
		}

		/* we put the pointer at the beginning of the RTP
		* payload, that is, at the beginning of the RTP data
		* plus the offset of the payload from the beginning
		* of the RTP data */
		data = rtpinfo->info_data + rtpinfo->info_payload_offset;
		for(i=0; i < (rtpinfo->info_payload_len - rtpinfo->info_padding_count); i++, data++) {
			tmp = (gint16 )alaw2linear((unsigned char)*data);
			fwrite(&tmp, 2, 1, saveinfo->fp);
			saveinfo->count++;
		}
		fflush(saveinfo->fp);
		saveinfo->saved = TRUE;
		return 0;
	}
	/* comfort noise? - do nothing */
	else if (rtpinfo->info_payload_type == PT_CN
		|| rtpinfo->info_payload_type == PT_CN_OLD) {
	}
	/* unsupported codec or XXX other error */
	else {
		saveinfo->saved = FALSE;
		saveinfo->error_type = TAP_RTP_WRONG_CODEC;
		return 0;
	}

	return 0;
}


/****************************************************************************/
/* CALLBACKS */

/****************************************************************************/
/* XXX just copied from gtk/rpc_stat.c */
void protect_thread_critical_region(void);
void unprotect_thread_critical_region(void);


/****************************************************************************/
/* close the dialog window and remove the tap listener */
static void on_destroy(GtkWidget *win _U_, user_data_t *user_data _U_)
{
	/* close and remove temporary files */
	if (user_data->forward.saveinfo.fp != NULL)
		fclose(user_data->forward.saveinfo.fp);
	if (user_data->reversed.saveinfo.fp != NULL)
		fclose(user_data->reversed.saveinfo.fp);
	/*XXX: test for error **/
	remove(user_data->f_tempname);
	remove(user_data->r_tempname);

	/* destroy save_voice_as window if open */
	if (user_data->dlg.save_voice_as_w != NULL)
		gtk_widget_destroy(user_data->dlg.save_voice_as_w);

#ifdef USE_CONVERSATION_GRAPH
	/* destroy graph window if open */
	if (user_data->dlg.graph_window != NULL)
		gtk_widget_destroy(user_data->dlg.graph_window);
#endif

	g_free(user_data->dlg.col_arrows_fwd);
	g_free(user_data->dlg.col_arrows_rev);
	g_free(user_data);
}


/****************************************************************************/
static void on_notebook_switch_page(GtkNotebook *notebook _U_,
                                    GtkNotebookPage *page _U_,
                                    gint page_num _U_,
                                    user_data_t *user_data _U_)
{
	user_data->dlg.selected_clist =
		(page_num==0) ? user_data->dlg.clist_fwd : user_data->dlg.clist_rev ;
	user_data->dlg.selected_row = 0;
}

/****************************************************************************/
static void on_clist_select_row(GtkCList        *clist _U_,
                                gint             row _U_,
                                gint             column _U_,
                                GdkEvent        *event _U_,
                                user_data_t     *user_data _U_)
{
	user_data->dlg.selected_clist = clist;
	user_data->dlg.selected_row = row;
}


#ifdef USE_CONVERSATION_GRAPH
/****************************************************************************/
/* when the graph window gets destroyed */
static void on_destroy_graph(GtkWidget *win _U_, user_data_t *user_data _U_)
{
	/* note that graph window has been destroyed */
	user_data->dlg.graph_window = NULL;
}

/****************************************************************************/
static void graph_selection_callback(value_pair_t vp, user_data_t *user_data)
{
	guint row;
	GtkCList *clist = NULL;
	if (vp.fnumber != 0) {
		clist = GTK_CLIST(user_data->dlg.clist_fwd);
		row = gtk_clist_find_row_from_data(clist,
				GUINT_TO_POINTER(vp.fnumber));
		if (row==-1) {
			clist = GTK_CLIST(user_data->dlg.clist_rev);
			row = gtk_clist_find_row_from_data(clist,
					GUINT_TO_POINTER(vp.fnumber));
		}
		if (row!=-1) {
			gtk_notebook_set_page(GTK_NOTEBOOK(user_data->dlg.notebook),
				(clist == GTK_CLIST(user_data->dlg.clist_fwd)) ? 0 : 1);
			gtk_clist_select_row(clist, row, 0);
			gtk_clist_moveto(clist, row, 0, 0.5, 0);
		}
	}
}


/****************************************************************************/
static void on_graph_bt_clicked(GtkWidget *bt _U_, user_data_t *user_data _U_)
{
	gchar title1[80];
	gchar title2[80];
	GList *list = NULL;
	
	if (user_data->dlg.graph_window != NULL) {
		/* There's already a graph window; reactivate it. */
		reactivate_window(user_data->dlg.graph_window);
		return;
	}
		
	list = g_list_append(list, &(user_data->series_fwd));
	list = g_list_append(list, &(user_data->series_rev));

	user_data->series_fwd.color.pixel = 0;
	user_data->series_fwd.color.red = 0x80ff;
	user_data->series_fwd.color.green = 0xe0ff;
	user_data->series_fwd.color.blue = 0xffff;
	user_data->series_fwd.yvalue = 0.5;

	user_data->series_rev.color.pixel = 0;
	user_data->series_rev.color.red = 0x60ff;
	user_data->series_rev.color.green = 0xc0ff;
	user_data->series_rev.color.blue = 0xffff;
	user_data->series_rev.yvalue = -0.5;

	g_snprintf(title1, 80, "Forward: %s:%u to %s:%u (SSRC=%u)",
		ip_to_str((ip_addr_p)&(user_data->ip_src_fwd)),
		user_data->port_src_fwd,
		ip_to_str((ip_addr_p)&(user_data->ip_dst_fwd)),
		user_data->port_dst_fwd,
		user_data->ssrc_fwd);

	g_snprintf(title2, 80, "Reverse: %s:%u to %s:%u (SSRC=%u)",
		ip_to_str((ip_addr_p)&(user_data->ip_src_rev)),
		user_data->port_src_rev,
		ip_to_str((ip_addr_p)&(user_data->ip_dst_rev)),
		user_data->port_dst_rev,
		user_data->ssrc_rev);

	user_data->dlg.graph_window = show_conversation_graph(list, title1, title2,
		&graph_selection_callback, user_data);
	SIGNAL_CONNECT(user_data->dlg.graph_window, "destroy",
                       on_destroy_graph, user_data);
}
#endif /*USE_CONVERSATION_GRAPH*/


/****************************************************************************/
static void on_goto_bt_clicked(GtkWidget *bt _U_, user_data_t *user_data _U_)
{
	guint fnumber;

	if (user_data->dlg.selected_clist!=NULL) {
		fnumber = GPOINTER_TO_UINT(gtk_clist_get_row_data(
			GTK_CLIST(user_data->dlg.selected_clist), user_data->dlg.selected_row) );
		goto_frame(&cfile, fnumber);
	}
}


static void draw_stat(user_data_t *user_data);

/****************************************************************************/
/* re-dissects all packets */
static void on_refresh_bt_clicked(GtkWidget *bt _U_, user_data_t *user_data _U_)
{
	gchar filter_text[256];
	dfilter_t *sfcode;
	GString *error_string;

	/* clear the dialog box clists */
	gtk_clist_clear(GTK_CLIST(user_data->dlg.clist_fwd));
	gtk_clist_clear(GTK_CLIST(user_data->dlg.clist_rev));

	/* try to compile the filter. */
	strcpy(filter_text,"rtp && ip");
	if (!dfilter_compile(filter_text, &sfcode)) {
		simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK, dfilter_error_msg);
		return;
	}

	g_snprintf(filter_text,sizeof(filter_text),
        "rtp && (( ip.src==%s && udp.srcport==%u && ip.dst==%s && udp.dstport==%u ) || ( ip.src==%s && udp.srcport==%u && ip.dst==%s && udp.dstport==%u ))",
		ip_to_str((ip_addr_p)&(user_data->ip_src_fwd)),
		user_data->port_src_fwd,
		ip_to_str((ip_addr_p)&(user_data->ip_dst_fwd)),
		user_data->port_dst_fwd,
		ip_to_str((ip_addr_p)&(user_data->ip_src_rev)),
		user_data->port_src_rev,
		ip_to_str((ip_addr_p)&(user_data->ip_dst_rev)),
		user_data->port_dst_rev
		);

	/* register tap listener */
	error_string = register_tap_listener("rtp", user_data, filter_text,
		rtp_reset, rtp_packet, rtp_draw);
	if (error_string != NULL) {
		simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK, error_string->str);
			g_string_free(error_string, TRUE);
		return;
	}

	/* retap all packets */
	retap_packets(&cfile);

	/* remove tap listener again */
	protect_thread_critical_region();
	remove_tap_listener(user_data);
	unprotect_thread_critical_region();

	/* draw statistics info */
	draw_stat(user_data);

	gtk_clist_sort(user_data->dlg.clist_fwd);
	gtk_clist_sort(user_data->dlg.clist_rev);
}

/****************************************************************************/
static void on_next_bt_clicked(GtkWidget *bt _U_, user_data_t *user_data _U_)
{
	GtkCList *clist;
	gchar *text;
	gint row;
	if (user_data->dlg.selected_clist==NULL)
		return;

	clist = user_data->dlg.selected_clist;
	row = user_data->dlg.selected_row + 1;

	while (gtk_clist_get_text(clist,row,5,&text)) {
		if (strcmp(text, OK_TEXT) != 0) {
			gtk_clist_select_row(clist, row, 0);
			gtk_clist_moveto(clist, row, 0, 0.5, 0);
			return;
		}
		++row;
	}

	/* wrap around */
	row = 0;
	while (gtk_clist_get_text(clist,row,5,&text) && row<user_data->dlg.selected_row) {
		if (strcmp(text, OK_TEXT) != 0) {
			gtk_clist_select_row(clist, row, 0);
			gtk_clist_moveto(clist, row, 0, 0.5, 0);
			return;
		}
		++row;
	}
}

/****************************************************************************/
/* when we want to save the information */
static void save_csv_as_ok_cb(GtkWidget *bt _U_, gpointer fs /*user_data_t *user_data*/ _U_)
{
	gchar *g_dest;
	GtkWidget *rev, *forw, *both;
	user_data_t *user_data;
	
	FILE *fp;
	char *columnText;
	int i,j;
	
	g_dest = g_strdup(gtk_file_selection_get_filename(GTK_FILE_SELECTION (fs)));
	
	/* Perhaps the user specified a directory instead of a file.
	Check whether they did. */
	if (test_for_directory(g_dest) == EISDIR) {
		/* It's a directory - set the file selection box to display it. */
		set_last_open_dir(g_dest);
		g_free(g_dest);
		gtk_file_selection_set_filename(GTK_FILE_SELECTION(fs), last_open_dir);
		return;
	}
	
	rev = (GtkWidget*)OBJECT_GET_DATA(bt, "reversed_rb");
	forw = (GtkWidget*)OBJECT_GET_DATA(bt, "forward_rb");
	both = (GtkWidget*)OBJECT_GET_DATA(bt, "both_rb");
	user_data = (user_data_t*)OBJECT_GET_DATA(bt, "user_data");
	
	if (GTK_TOGGLE_BUTTON(forw)->active || GTK_TOGGLE_BUTTON(both)->active) {
		fp = fopen(g_dest, "w");
		if (fp == NULL) {
			open_failure_alert_box(g_dest, errno, TRUE);
			return;
		}
		
		if (GTK_TOGGLE_BUTTON(both)->active) {
			fprintf(fp, "Forward\n");
			if (ferror(fp)) {
				write_failure_alert_box(g_dest, errno);
				fclose(fp);
				return;
			}
		}
		
		for(j = 0; j < 8; j++) {
			if (j == 0) {
				fprintf(fp,"%s",titles[j]);
			} else {
				fprintf(fp,",%s",titles[j]);
			}
		}
		fprintf(fp,"\n");
		if (ferror(fp)) {
			write_failure_alert_box(g_dest, errno);
			fclose(fp);
			return;
		}
		for (i = 0; i < GTK_CLIST(user_data->dlg.clist_fwd)->rows; i++) {
			for(j = 0; j < GTK_CLIST(user_data->dlg.clist_fwd)->columns; j++) {
				gtk_clist_get_text(GTK_CLIST(user_data->dlg.clist_fwd),i,j,&columnText);
				if (j == 0) {
					fprintf(fp,"%s",columnText);
				} else {
					fprintf(fp,",%s",columnText);
				}
			}
			fprintf(fp,"\n");
			if (ferror(fp)) {
				write_failure_alert_box(g_dest, errno);
				fclose(fp);
				return;
			}
		}
		
		if (fclose(fp) == EOF) {
			write_failure_alert_box(g_dest, errno);
			return;
		}
	}
	
	if (GTK_TOGGLE_BUTTON(rev)->active || GTK_TOGGLE_BUTTON(both)->active) {
		
		if (GTK_TOGGLE_BUTTON(both)->active) {
			fp = fopen(g_dest, "a");
			if (fp == NULL) {
				open_failure_alert_box(g_dest, errno, TRUE);
				return;
			}
			fprintf(fp, "\nReverse\n");
			if (ferror(fp)) {
				write_failure_alert_box(g_dest, errno);
				fclose(fp);
				return;
			}
		} else {
			fp = fopen(g_dest, "w");
			if (fp == NULL) {
				open_failure_alert_box(g_dest, errno, TRUE);
				return;
			}
		}
		for(j = 0; j < 8; j++) {
			if (j == 0) {
				fprintf(fp,"%s",titles[j]);
			} else {
				fprintf(fp,",%s",titles[j]);
			}
		}
		fprintf(fp,"\n");
		if (ferror(fp)) {
			write_failure_alert_box(g_dest, errno);
			fclose(fp);
			return;
		}
		for (i = 0; i < GTK_CLIST(user_data->dlg.clist_rev)->rows; i++) {
			for(j = 0; j < GTK_CLIST(user_data->dlg.clist_rev)->columns; j++) {
				gtk_clist_get_text(GTK_CLIST(user_data->dlg.clist_rev),i,j,&columnText);
				if (j == 0) {
					fprintf(fp,"%s",columnText);
				} else {
					fprintf(fp,",%s",columnText);
				}
			}
			fprintf(fp,"\n");
			if (ferror(fp)) {
				write_failure_alert_box(g_dest, errno);
				fclose(fp);
				return;
			}
		}
		if (fclose(fp) == EOF) {
			write_failure_alert_box(g_dest, errno);
			return;
		}
	}

	gtk_widget_destroy(GTK_WIDGET(user_data->dlg.save_csv_as_w));
}

static void save_csv_as_destroy_cb(GtkWidget *win _U_, user_data_t *user_data _U_)
{
	user_data->dlg.save_csv_as_w = NULL;
}

/* when the user wants to save the csv information in a file */
static void save_csv_as_cb(GtkWidget *bt _U_, user_data_t *user_data _U_)
{
	GtkWidget *vertb;
	GtkWidget *table1;
	GtkWidget *label_format;
	GtkWidget *channels_label;
	GSList *channels_group = NULL;
	GtkWidget *forward_rb;
	GtkWidget *reversed_rb;
	GtkWidget *both_rb;
	GtkWidget *ok_bt;
	
	if (user_data->dlg.save_csv_as_w != NULL) {
		/* There's already a Save CSV info dialog box; reactivate it. */
		reactivate_window(user_data->dlg.save_csv_as_w);
		return;
	}
	
	user_data->dlg.save_csv_as_w = gtk_file_selection_new("Ethereal: Save Data As CSV");
	SIGNAL_CONNECT(user_data->dlg.save_csv_as_w, "destroy",
                       save_csv_as_destroy_cb, user_data);
	
	/* Container for each row of widgets */
	vertb = gtk_vbox_new(FALSE, 0);
	gtk_container_border_width(GTK_CONTAINER(vertb), 5);
	gtk_box_pack_start(GTK_BOX(GTK_FILE_SELECTION(user_data->dlg.save_csv_as_w)->action_area),
		vertb, FALSE, FALSE, 0);
	gtk_widget_show (vertb);
	
	table1 = gtk_table_new (2, 4, FALSE);
	gtk_widget_show (table1);
	gtk_box_pack_start (GTK_BOX (vertb), table1, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (table1), 10);
	gtk_table_set_row_spacings (GTK_TABLE (table1), 20);
	
	label_format = gtk_label_new ("Format: Comma Separated Values");
	gtk_widget_show (label_format);
	gtk_table_attach (GTK_TABLE (table1), label_format, 0, 3, 0, 1,
		(GtkAttachOptions) (GTK_FILL),
		(GtkAttachOptions) (0), 0, 0);
	
	
	channels_label = gtk_label_new ("Channels:");
	gtk_widget_show (channels_label);
	gtk_table_attach (GTK_TABLE (table1), channels_label, 0, 1, 1, 2,
		(GtkAttachOptions) (GTK_FILL),
		(GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (channels_label), 0, 0.5);
	
	forward_rb = gtk_radio_button_new_with_label (channels_group, "forward  ");
	channels_group = gtk_radio_button_group (GTK_RADIO_BUTTON (forward_rb));
	gtk_widget_show (forward_rb);
	gtk_table_attach (GTK_TABLE (table1), forward_rb, 1, 2, 1, 2,
		(GtkAttachOptions) (GTK_FILL),
		(GtkAttachOptions) (0), 0, 0);
	
	reversed_rb = gtk_radio_button_new_with_label (channels_group, "reversed");
	channels_group = gtk_radio_button_group (GTK_RADIO_BUTTON (reversed_rb));
	gtk_widget_show (reversed_rb);
	gtk_table_attach (GTK_TABLE (table1), reversed_rb, 2, 3, 1, 2,
		(GtkAttachOptions) (GTK_FILL),
		(GtkAttachOptions) (0), 0, 0);
	
	both_rb = gtk_radio_button_new_with_label (channels_group, "both");
	channels_group = gtk_radio_button_group (GTK_RADIO_BUTTON (both_rb));
	gtk_widget_show (both_rb);
	gtk_table_attach (GTK_TABLE (table1), both_rb, 3, 4, 1, 2,
		(GtkAttachOptions) (GTK_FILL),
		(GtkAttachOptions) (0), 0, 0);
	
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(both_rb), TRUE);
	
	ok_bt = GTK_FILE_SELECTION(user_data->dlg.save_csv_as_w)->ok_button;
	OBJECT_SET_DATA(ok_bt, "forward_rb", forward_rb);
	OBJECT_SET_DATA(ok_bt, "reversed_rb", reversed_rb);
	OBJECT_SET_DATA(ok_bt, "both_rb", both_rb);
	OBJECT_SET_DATA(ok_bt, "user_data", user_data);
	
	/* Connect the cancel_button to destroy the widget */
	SIGNAL_CONNECT_OBJECT(GTK_FILE_SELECTION(user_data->dlg.save_csv_as_w)->cancel_button,
		"clicked", (GtkSignalFunc)gtk_widget_destroy,
		user_data->dlg.save_csv_as_w);
	
	/* Catch the "key_press_event" signal in the window, so that we can catch
	the ESC key being pressed and act as if the "Cancel" button had
	been selected. */
	dlg_set_cancel(user_data->dlg.save_csv_as_w, GTK_FILE_SELECTION(user_data->dlg.save_csv_as_w)->cancel_button);
	
	SIGNAL_CONNECT(ok_bt, "clicked", save_csv_as_ok_cb,
                       user_data->dlg.save_csv_as_w);
	
	gtk_widget_show(user_data->dlg.save_csv_as_w);
}


/****************************************************************************/
static void save_voice_as_destroy_cb(GtkWidget *win _U_, user_data_t *user_data _U_)
{
	/* Note that we no longer have a Save voice info dialog box. */
	user_data->dlg.save_voice_as_w = NULL;
}

/****************************************************************************/
/* here we save it into a file that user specified */
/* XXX what about endians here? could go something wrong? */
static gboolean copy_file(gchar *dest, gint channels, /*gint format,*/ user_data_t *user_data)
{
	int to_fd, forw_fd, rev_fd, fread = 0, rread = 0, fwritten, rwritten;
	gint16 f_pd;
	gint16 r_pd;
	gchar pd[1];
	guint32 f_write_silence = 0;
	guint32 r_write_silence = 0;
	progdlg_t *progbar;
	guint32 progbar_count, progbar_quantum, progbar_nextstep = 0, count = 0;
	gboolean stop_flag = FALSE;

	forw_fd = open(user_data->f_tempname, O_RDONLY | O_BINARY);
	if (forw_fd < 0) 
		return FALSE;
	rev_fd = open(user_data->r_tempname, O_RDONLY | O_BINARY);
	if (rev_fd < 0) {
		close(forw_fd); 
		return FALSE;
	}

	/* open file for saving */
	to_fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0644);
	if (to_fd < 0) {
		close(forw_fd);
		close(rev_fd);
		return FALSE;
	}

	progbar = create_progress_dlg("Saving voice in a file", dest, &stop_flag);

	/* First we write the .au header. XXX Hope this is endian independant */
	/* the magic word 0x2e736e64 == .snd */
	*pd = (unsigned char)0x2e; write(to_fd, pd, 1);
	*pd = (unsigned char)0x73; write(to_fd, pd, 1);
	*pd = (unsigned char)0x6e; write(to_fd, pd, 1);
	*pd = (unsigned char)0x64; write(to_fd, pd, 1);
	/* header offset == 24 bytes */
	*pd = (unsigned char)0x00; write(to_fd, pd, 1);
	write(to_fd, pd, 1);
	write(to_fd, pd, 1);
	*pd = (unsigned char)0x18; write(to_fd, pd, 1);
	/* total length, it is permited to set this to 0xffffffff */
	*pd = (unsigned char)0xff; write(to_fd, pd, 1); 
	write(to_fd, pd, 1); 
	write(to_fd, pd, 1); 
	write(to_fd, pd, 1);
	/* encoding format == 8 bit ulaw */
	*pd = (unsigned char)0x00; write(to_fd, pd, 1);
	write(to_fd, pd, 1);
	write(to_fd, pd, 1);
	*pd = (unsigned char)0x01; write(to_fd, pd, 1);
	/* sample rate == 8000 Hz */
	*pd = (unsigned char)0x00; write(to_fd, pd, 1);
	write(to_fd, pd, 1);
	*pd = (unsigned char)0x1f; write(to_fd, pd, 1);
	*pd = (unsigned char)0x40; write(to_fd, pd, 1);
	/* channels == 1 */
	*pd = (unsigned char)0x00; write(to_fd, pd, 1);
	write(to_fd, pd, 1);
	write(to_fd, pd, 1);
	*pd = (unsigned char)0x01; write(to_fd, pd, 1);
	
	switch (channels) {
		/* only forward direction */
		case 1: {
			progbar_count = user_data->forward.saveinfo.count;
			progbar_quantum = user_data->forward.saveinfo.count/100;
			while ((fread = read(forw_fd, &f_pd, 2)) > 0) {
				if(stop_flag) 
					break;
				if((count > progbar_nextstep) && (count <= progbar_count)) {
					update_progress_dlg(progbar, 
						(gfloat) count/progbar_count, "Saving");
					progbar_nextstep = progbar_nextstep + progbar_quantum;
				}
				count++;
				*pd = (unsigned char)linear2ulaw(f_pd);
				fwritten = write(to_fd, pd, 1);
				if ((fwritten*2 < fread) || (fwritten < 0) || (fread < 0)) {
					close(forw_fd);
					close(rev_fd);
					close(to_fd);
					destroy_progress_dlg(progbar);
					return FALSE;
				}
			}
			break;
		}
		/* only reversed direction */
		case 2: {
			progbar_count = user_data->reversed.saveinfo.count;
			progbar_quantum = user_data->reversed.saveinfo.count/100;
			while ((rread = read(rev_fd, &r_pd, 2)) > 0) {
				if(stop_flag) 
					break;
				if((count > progbar_nextstep) && (count <= progbar_count)) {
					update_progress_dlg(progbar, 
						(gfloat) count/progbar_count, "Saving");
					progbar_nextstep = progbar_nextstep + progbar_quantum;
				}
				count++;
				*pd = (unsigned char)linear2ulaw(r_pd);
				rwritten = write(to_fd, pd, 1);
				if ((rwritten*2 < rread) || (rwritten < 0) || (rread < 0)) {
					close(forw_fd);
					close(rev_fd);
					close(to_fd);
					destroy_progress_dlg(progbar);
					return FALSE;
				}
			}
			break;
		}
		/* both directions */
		default: {
			(user_data->forward.saveinfo.count > user_data->reversed.saveinfo.count) ? 
					(progbar_count = user_data->forward.saveinfo.count) : 
						(progbar_count = user_data->reversed.saveinfo.count);
			progbar_quantum = progbar_count/100;
			/* since conversation in one way can start later than in the other one, 
			 * we have to write some silence information for one channel */
			if (user_data->forward.statinfo.start_time > user_data->reversed.statinfo.start_time) {
				f_write_silence = (guint32)
					((user_data->forward.statinfo.start_time-user_data->reversed.statinfo.start_time)*8000);
			}
			else if (user_data->forward.statinfo.start_time < user_data->reversed.statinfo.start_time) {
				r_write_silence = (guint32)
					((user_data->reversed.statinfo.start_time-user_data->forward.statinfo.start_time)*8000);
			}
			for(;;) {
				if(stop_flag) 
					break;
				if((count > progbar_nextstep) && (count <= progbar_count)) {
					update_progress_dlg(progbar, 
						(gfloat) count/progbar_count, "Saving");
					progbar_nextstep = progbar_nextstep + progbar_quantum;
				}
				count++;
				if(f_write_silence > 0) {
					rread = read(rev_fd, &r_pd, 2);
					f_pd = 0;
					fread = 1;
					f_write_silence--;
				}
				else if(r_write_silence > 0) {
					fread = read(forw_fd, &f_pd, 2);
					r_pd = 0;
					rread = 1;
					r_write_silence--;
				}
				else {
					fread = read(forw_fd, &f_pd, 2); 
					rread = read(rev_fd, &r_pd, 2);
				}
				if ((rread == 0) && (fread == 0)) 
					break;
				*pd = (unsigned char)linear2ulaw( (f_pd + r_pd)/2 );
				rwritten = write(to_fd, pd, 1);
				if ((rwritten < 0) || (rread < 0) || (fread < 0)) {
					close(forw_fd);
					close(rev_fd);
					close(to_fd);
					destroy_progress_dlg(progbar);
					return FALSE;
				}
			}
		}
	}
	destroy_progress_dlg(progbar);
	close(forw_fd);
	close(rev_fd);
	close(to_fd);
	return TRUE;
}


/****************************************************************************/
/* the user wants to save in a file */
/* XXX support for different formats is currently commented out */
static void save_voice_as_ok_cb(GtkWidget *ok_bt _U_, gpointer fs _U_)
{
	gchar *g_dest;
	/*GtkWidget *wav, *au, *sw;*/
	GtkWidget *rev, *forw, *both;
	user_data_t *user_data;
	gint channels /*, format*/;
	
	g_dest = g_strdup(gtk_file_selection_get_filename(GTK_FILE_SELECTION (fs)));
	
	/* Perhaps the user specified a directory instead of a file.
	Check whether they did. */
	if (test_for_directory(g_dest) == EISDIR) {
		/* It's a directory - set the file selection box to display it. */
		set_last_open_dir(g_dest);
		g_free(g_dest);
		gtk_file_selection_set_filename(GTK_FILE_SELECTION(fs), last_open_dir);
		return;
	}
	
	/*wav = (GtkWidget *)OBJECT_GET_DATA(ok_bt, "wav_rb");
	au = (GtkWidget *)OBJECT_GET_DATA(ok_bt, "au_rb");
	sw = (GtkWidget *)OBJECT_GET_DATA(ok_bt, "sw_rb");*/
	rev = (GtkWidget *)OBJECT_GET_DATA(ok_bt, "reversed_rb");
	forw = (GtkWidget *)OBJECT_GET_DATA(ok_bt, "forward_rb");
	both = (GtkWidget *)OBJECT_GET_DATA(ok_bt, "both_rb");
	user_data = (user_data_t *)OBJECT_GET_DATA(ok_bt, "user_data");
	
	/* XXX user clicks the ok button, but we know we can't save the voice info because f.e.
	* we don't support that codec. So we pop up a warning. Maybe it would be better to
	* disable the ok button or disable the buttons for direction if only one is not ok. The
	* problem is if we open the save voice dialog and then click the refresh button and maybe 
	* the state changes, so we can't save anymore. In this case we should be able to update
	* the buttons. For now it is easier if we put the warning when the ok button is pressed.
	*/
	
	/* we can not save in both dirctions */
	if ((user_data->forward.saveinfo.saved == FALSE) && (user_data->reversed.saveinfo.saved == FALSE) && (GTK_TOGGLE_BUTTON (both)->active)) {
		/* there are many combinations here, we just exit when first matches */
		if ((user_data->forward.saveinfo.error_type == TAP_RTP_WRONG_CODEC) || 
			(user_data->reversed.saveinfo.error_type == TAP_RTP_WRONG_CODEC))
			simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK,
			"Can't save in a file: Unsupported codec!");
		else if ((user_data->forward.saveinfo.error_type == TAP_RTP_WRONG_LENGTH) || 
			(user_data->reversed.saveinfo.error_type == TAP_RTP_WRONG_LENGTH))
			simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK,
			"Can't save in a file: Wrong length of captured packets!");
		else if ((user_data->forward.saveinfo.error_type == TAP_RTP_PADDING_ERROR) || 
			(user_data->reversed.saveinfo.error_type == TAP_RTP_PADDING_ERROR))
			simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK,
			"Can't save in a file: RTP data with padding!");
		else if ((user_data->forward.saveinfo.error_type == TAP_RTP_SHORT_FRAME) || 
			(user_data->reversed.saveinfo.error_type == TAP_RTP_SHORT_FRAME))
			simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK,
			"Can't save in a file: Not all data in all packets was captured!");
		else  
			simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK,
			"Can't save in a file: File I/O problem!");
		return;
	}
	/* we can not save forward direction */
	else if ((user_data->forward.saveinfo.saved == FALSE) && ((GTK_TOGGLE_BUTTON (forw)->active) ||
		(GTK_TOGGLE_BUTTON (both)->active))) {	
		if (user_data->forward.saveinfo.error_type == TAP_RTP_WRONG_CODEC)
			simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK,
			"Can't save forward direction in a file: Unsupported codec!");
		else if (user_data->forward.saveinfo.error_type == TAP_RTP_WRONG_LENGTH)
			simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK,
			"Can't save forward direction in a file: Wrong length of captured packets!");
		else if (user_data->forward.saveinfo.error_type == TAP_RTP_PADDING_ERROR)
			simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK,
			"Can't save forward direction in a file: RTP data with padding!");
		else if (user_data->forward.saveinfo.error_type == TAP_RTP_SHORT_FRAME)
			simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK,
			"Can't save forward direction in a file: Not all data in all packets was captured!");
		else
			simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK,
			"Can't save forward direction in a file: File I/O problem!");
		return;
	}
	/* we can not save reversed direction */
	else if ((user_data->reversed.saveinfo.saved == FALSE) && ((GTK_TOGGLE_BUTTON (rev)->active) ||
		(GTK_TOGGLE_BUTTON (both)->active))) {	
		if (user_data->reversed.saveinfo.error_type == TAP_RTP_WRONG_CODEC)
			simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK,
			"Can't save reversed direction in a file: Unsupported codec!");
		else if (user_data->reversed.saveinfo.error_type == TAP_RTP_WRONG_LENGTH)
			simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK,
			"Can't save reversed direction in a file: Wrong length of captured packets!");
		else if (user_data->reversed.saveinfo.error_type == TAP_RTP_PADDING_ERROR)
			simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK,
			"Can't save reversed direction in a file: RTP data with padding!");
		else if (user_data->forward.saveinfo.error_type == TAP_RTP_SHORT_FRAME)
			simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK,
			"Can't save reversed direction in a file: Not all data in all packets was captured!");
		else if (user_data->reversed.saveinfo.error_type == TAP_RTP_NO_DATA)
			simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK,
			"Can't save reversed direction in a file: No RTP data!");
		else
			simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK,
			"Can't save reversed direction in a file: File I/O problem!");
		return;
	}
	
	/*if (GTK_TOGGLE_BUTTON (wav)->active)
	format = 1;
	else if (GTK_TOGGLE_BUTTON (au)->active)
	format = 2;
	else if (GTK_TOGGLE_BUTTON (sw)->active)
	format = 3;*/
	
	if (GTK_TOGGLE_BUTTON (rev)->active)
		channels = 2;
	else if (GTK_TOGGLE_BUTTON (both)->active)
		channels = 3;
	else 
		channels = 1;
	
	if(!copy_file(g_dest, channels/*, format*/, user_data)) {
		/* XXX - report the error type! */
		simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK,
			"An error occured while saving voice in a file!");
		return;
	}
	
	gtk_widget_destroy(GTK_WIDGET(user_data->dlg.save_voice_as_w));
}

/****************************************************************************/
/* when the user wants to save the voice information in a file */
/* XXX support for different formats is currently commented out */
static void on_save_bt_clicked(GtkWidget *bt _U_, user_data_t *user_data _U_)
{
	GtkWidget *vertb;
	GtkWidget *table1;
	GtkWidget *label_format;
	GtkWidget *channels_label;
	/*GSList *format_group = NULL;*/
	GSList *channels_group = NULL;
	GtkWidget *forward_rb;
	GtkWidget *reversed_rb;
	GtkWidget *both_rb;
	/*GtkWidget *wav_rb; GtkWidget *au_rb; GtkWidget *sw_rb;*/
	GtkWidget *ok_bt;
	
	/* if we can't save in a file: wrong codec, cut packets or other errors */
	/* shold the error arise here or later when you click ok button ? 
	* if we do it here, then we must disable the refresh button, so we don't do it here */
	
	if (user_data->dlg.save_voice_as_w != NULL) {
		/* There's already a Save voice info dialog box; reactivate it. */
		reactivate_window(user_data->dlg.save_voice_as_w);
		return;
	}
	
	user_data->dlg.save_voice_as_w = gtk_file_selection_new("Ethereal: Save Payload As ...");
	SIGNAL_CONNECT(user_data->dlg.save_voice_as_w, "destroy",
                       save_voice_as_destroy_cb, user_data);
	
	/* Container for each row of widgets */
	vertb = gtk_vbox_new(FALSE, 0);
	gtk_container_border_width(GTK_CONTAINER(vertb), 5);
	gtk_box_pack_start(GTK_BOX(GTK_FILE_SELECTION(user_data->dlg.save_voice_as_w)->action_area),
		vertb, FALSE, FALSE, 0);
	gtk_widget_show (vertb);
	
	table1 = gtk_table_new (2, 4, FALSE);
	gtk_widget_show (table1);
	gtk_box_pack_start (GTK_BOX (vertb), table1, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (table1), 10);
	gtk_table_set_row_spacings (GTK_TABLE (table1), 20);
	
	label_format = gtk_label_new ("Format: .au (ulaw, 8 bit, 8000 Hz, mono) ");
	gtk_widget_show (label_format);
	gtk_table_attach (GTK_TABLE (table1), label_format, 0, 3, 0, 1,
		(GtkAttachOptions) (GTK_FILL),
		(GtkAttachOptions) (0), 0, 0);
	
	/* we support .au - ulaw*/ 
	/*	wav_rb = gtk_radio_button_new_with_label (format_group, ".wav");
	format_group = gtk_radio_button_group (GTK_RADIO_BUTTON (wav_rb));
	gtk_widget_show (wav_rb);
	gtk_table_attach (GTK_TABLE (table1), wav_rb, 1, 2, 0, 1,
	(GtkAttachOptions) (GTK_FILL),
	(GtkAttachOptions) (0), 0, 0);
	
	  sw_rb = gtk_radio_button_new_with_label (format_group, "8 kHz, 16 bit  ");
	  format_group = gtk_radio_button_group (GTK_RADIO_BUTTON (sw_rb));
	  gtk_widget_show (sw_rb);
	  gtk_table_attach (GTK_TABLE (table1), sw_rb, 2, 3, 0, 1,
	  (GtkAttachOptions) (GTK_FILL),
	  (GtkAttachOptions) (0), 0, 0);
	  au_rb = gtk_radio_button_new_with_label (format_group, ".au");
	  format_group = gtk_radio_button_group (GTK_RADIO_BUTTON (au_rb));
	  gtk_widget_show (au_rb);
	  gtk_table_attach (GTK_TABLE (table1), au_rb, 3, 4, 0, 1,
	  (GtkAttachOptions) (GTK_FILL),
	  (GtkAttachOptions) (0), 0, 0);
	*/ 
	
	channels_label = gtk_label_new ("Channels:");
	gtk_widget_show (channels_label);
	gtk_table_attach (GTK_TABLE (table1), channels_label, 0, 1, 1, 2,
		(GtkAttachOptions) (GTK_FILL),
		(GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (channels_label), 0, 0.5);
	
	forward_rb = gtk_radio_button_new_with_label (channels_group, "forward  ");
	channels_group = gtk_radio_button_group (GTK_RADIO_BUTTON (forward_rb));
	gtk_widget_show (forward_rb);
	gtk_table_attach (GTK_TABLE (table1), forward_rb, 1, 2, 1, 2,
		(GtkAttachOptions) (GTK_FILL),
		(GtkAttachOptions) (0), 0, 0);
	
	reversed_rb = gtk_radio_button_new_with_label (channels_group, "reversed");
	channels_group = gtk_radio_button_group (GTK_RADIO_BUTTON (reversed_rb));
	gtk_widget_show (reversed_rb);
	gtk_table_attach (GTK_TABLE (table1), reversed_rb, 2, 3, 1, 2,
		(GtkAttachOptions) (GTK_FILL),
		(GtkAttachOptions) (0), 0, 0);
	
	both_rb = gtk_radio_button_new_with_label (channels_group, "both");
	channels_group = gtk_radio_button_group (GTK_RADIO_BUTTON (both_rb));
	gtk_widget_show (both_rb);
	gtk_table_attach (GTK_TABLE (table1), both_rb, 3, 4, 1, 2,
		(GtkAttachOptions) (GTK_FILL),
		(GtkAttachOptions) (0), 0, 0);
	
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(both_rb), TRUE);
	
	/* if one direction is nok we don't allow saving 
	XXX this is not ok since the user can click the refresh button and cause changes
	but we can not update this window. So we move all the decision on the time the ok
	button is clicked
	if (user_data->forward.saved == FALSE) {
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(reversed_rb), TRUE);
	gtk_widget_set_sensitive(forward_rb, FALSE);
	gtk_widget_set_sensitive(both_rb, FALSE);
	}
	else if (user_data->reversed.saved == FALSE) {
	gtk_widget_set_sensitive(reversed_rb, FALSE);
	gtk_widget_set_sensitive(both_rb, FALSE);
	}
	*/
	
	ok_bt = GTK_FILE_SELECTION(user_data->dlg.save_voice_as_w)->ok_button;
	/*OBJECT_SET_DATA(ok_bt, "wav_rb", wav_rb);
	OBJECT_SET_DATA(ok_bt, "au_rb", au_rb);
	OBJECT_SET_DATA(ok_bt, "sw_rb", sw_rb);*/
	OBJECT_SET_DATA(ok_bt, "forward_rb", forward_rb);
	OBJECT_SET_DATA(ok_bt, "reversed_rb", reversed_rb);
	OBJECT_SET_DATA(ok_bt, "both_rb", both_rb);
	OBJECT_SET_DATA(ok_bt, "user_data", user_data);
	
	/* Connect the cancel_button to destroy the widget */
	SIGNAL_CONNECT_OBJECT(GTK_FILE_SELECTION(user_data->dlg.save_voice_as_w)->cancel_button,
		"clicked", (GtkSignalFunc)gtk_widget_destroy,
		user_data->dlg.save_voice_as_w);
	
		/* Catch the "key_press_event" signal in the window, so that we can catch
		the ESC key being pressed and act as if the "Cancel" button had
	been selected. */
	dlg_set_cancel(user_data->dlg.save_voice_as_w, GTK_FILE_SELECTION(user_data->dlg.save_voice_as_w)->cancel_button);
	
	SIGNAL_CONNECT(ok_bt, "clicked", save_voice_as_ok_cb,
                       user_data->dlg.save_voice_as_w);
	
	gtk_widget_show(user_data->dlg.save_voice_as_w);
}


/****************************************************************************/
/* when we are finished with redisection, we add the label for the statistic */
static void draw_stat(user_data_t *user_data)
{
	gchar label_max[200];
	guint32 f_expected = (user_data->forward.statinfo.stop_seq_nr + user_data->forward.statinfo.cycles*65536)
		- user_data->forward.statinfo.start_seq_nr + 1;
	guint32 r_expected = (user_data->reversed.statinfo.stop_seq_nr + user_data->reversed.statinfo.cycles*65536)
		- user_data->reversed.statinfo.start_seq_nr + 1;
	gint32 f_lost = f_expected - user_data->forward.statinfo.total_nr;
	gint32 r_lost = r_expected - user_data->reversed.statinfo.total_nr;

	g_snprintf(label_max, 199, "Max delay = %f sec at packet no. %u \n"
		"Total RTP packets = %u   (expected %u)   Lost RTP packets = %d"
		"   Sequence errors = %u",
		user_data->forward.statinfo.max_delay, user_data->forward.statinfo.max_nr,
		user_data->forward.statinfo.total_nr,
		f_expected, f_lost, user_data->forward.statinfo.sequence);

	gtk_label_set_text(GTK_LABEL(user_data->dlg.label_stats_fwd), label_max);

	g_snprintf(label_max, 199, "Max delay = %f sec at packet no. %u \n"
		"Total RTP packets = %u   (expected %u)   Lost RTP packets = %d"
		"   Sequence errors = %u",
		user_data->reversed.statinfo.max_delay, user_data->reversed.statinfo.max_nr,
		user_data->reversed.statinfo.total_nr,
		r_expected, r_lost, user_data->reversed.statinfo.sequence);

	gtk_label_set_text(GTK_LABEL(user_data->dlg.label_stats_rev), label_max);

	return ;
}


#define NUM_COLS 8

/****************************************************************************/
/* append a line to clist */
static void add_to_clist(GtkCList *clist, guint32 number, guint16 seq_num,
                         double delay, double jitter, gchar *status, gboolean marker,
                         gchar *timeStr, guint32 pkt_len, GdkColor *color)
{
	guint added_row;
	gchar *data[8];
	gchar field[8][32];

	data[0]=&field[0][0];
	data[1]=&field[1][0];
	data[2]=&field[2][0];
	data[3]=&field[3][0];
	data[4]=&field[4][0];
	data[5]=&field[5][0];
	data[6]=&field[6][0];
	data[7]=&field[7][0];

	g_snprintf(field[0], 20, "%u", number);
	g_snprintf(field[1], 20, "%u", seq_num);
	g_snprintf(field[2], 20, "%f", delay);
	g_snprintf(field[3], 20, "%f", jitter);
	g_snprintf(field[4], 20, "%s", marker? "SET" : "");
	g_snprintf(field[5], 40, "%s", status);
	g_snprintf(field[6], 32, "%s", timeStr);
	g_snprintf(field[7], 20, "%u", pkt_len);

	added_row = gtk_clist_append(GTK_CLIST(clist), data);
	gtk_clist_set_row_data(GTK_CLIST(clist), added_row, GUINT_TO_POINTER(number));
	gtk_clist_set_background(GTK_CLIST(clist), added_row, color);
}


/****************************************************************************/
/* callback for sorting columns of clist */
static gint rtp_sort_column(GtkCList *clist, gconstpointer ptr1, gconstpointer ptr2)
{
	char *text1 = NULL;
	char *text2 = NULL;
	int i1, i2;
	double f1, f2;

	const GtkCListRow *row1 = (GtkCListRow *) ptr1;
	const GtkCListRow *row2 = (GtkCListRow *) ptr2;

	text1 = GTK_CELL_TEXT (row1->cell[clist->sort_column])->text;
	text2 = GTK_CELL_TEXT (row2->cell[clist->sort_column])->text;

	switch(clist->sort_column){
	/* columns representing strings */
	case 4:
	case 5:
	case 6:
		return strcmp (text1, text2);
	/* columns representing ints */
	case 0:
	case 1:
	case 7:
		i1=atoi(text1);
		i2=atoi(text2);
		return i1-i2;
	/* columns representing floats */
	case 2:
	case 3:
		f1=atof(text1);
		f2=atof(text2);
		if (fabs(f1-f2)<0.0000005)
			return 0;
		if (f1<f2)
			return -1;
		return 1;
	}
	g_assert_not_reached();
	return 0;
}


/****************************************************************************/
static void
click_column_cb(GtkCList *clist, gint column, gpointer data)
{
	column_arrows *col_arrows = (column_arrows *) data;
	int i;

	gtk_clist_freeze(clist);

	for (i = 0; i < NUM_COLS; i++) {
		gtk_widget_hide(col_arrows[i].ascend_pm);
		gtk_widget_hide(col_arrows[i].descend_pm);
	}

	if (column == clist->sort_column) {
		if (clist->sort_type == GTK_SORT_ASCENDING) {
			clist->sort_type = GTK_SORT_DESCENDING;
			gtk_widget_show(col_arrows[column].descend_pm);
		} else {
			clist->sort_type = GTK_SORT_ASCENDING;
			gtk_widget_show(col_arrows[column].ascend_pm);
		}
	} else {
		clist->sort_type = GTK_SORT_ASCENDING;
		gtk_widget_show(col_arrows[column].ascend_pm);
		gtk_clist_set_sort_column(clist, column);
	}
	gtk_clist_thaw(clist);

	gtk_clist_sort(clist);
}


/****************************************************************************/
/* Add the packet list */
static
GtkWidget* create_clist(user_data_t* user_data)
{
	GtkWidget* clist_fwd;

	/* clist for the information */
	clist_fwd = gtk_clist_new(8);
	gtk_widget_show(clist_fwd);
	SIGNAL_CONNECT(clist_fwd, "select_row", on_clist_select_row, user_data);

	gtk_clist_column_titles_show(GTK_CLIST(clist_fwd));
	gtk_clist_set_compare_func(GTK_CLIST(clist_fwd), rtp_sort_column);
	gtk_clist_set_sort_column(GTK_CLIST(clist_fwd), 0);
	gtk_clist_set_sort_type(GTK_CLIST(clist_fwd), GTK_SORT_ASCENDING);

	/* hide date and length column */
	gtk_clist_set_column_visibility(GTK_CLIST(clist_fwd), 6, FALSE);
	gtk_clist_set_column_visibility(GTK_CLIST(clist_fwd), 7, FALSE);

	/* column widths and justification */
	gtk_clist_set_column_width(GTK_CLIST(clist_fwd), 0, 60);
	gtk_clist_set_column_width(GTK_CLIST(clist_fwd), 1, 75);
	gtk_clist_set_column_width(GTK_CLIST(clist_fwd), 2, 75);
	gtk_clist_set_column_width(GTK_CLIST(clist_fwd), 3, 75);
	gtk_clist_set_column_width(GTK_CLIST(clist_fwd), 4, 50);
	gtk_clist_set_column_justification(GTK_CLIST(clist_fwd), 0, GTK_JUSTIFY_RIGHT);
	gtk_clist_set_column_justification(GTK_CLIST(clist_fwd), 1, GTK_JUSTIFY_RIGHT);
	gtk_clist_set_column_justification(GTK_CLIST(clist_fwd), 2, GTK_JUSTIFY_CENTER);
	gtk_clist_set_column_justification(GTK_CLIST(clist_fwd), 3, GTK_JUSTIFY_CENTER);
	gtk_clist_set_column_justification(GTK_CLIST(clist_fwd), 4, GTK_JUSTIFY_CENTER);
	gtk_clist_set_column_justification(GTK_CLIST(clist_fwd), 5, GTK_JUSTIFY_CENTER);

	return clist_fwd;
}


/****************************************************************************/
/* Add the sort by column feature for a packet clist */
static
column_arrows* add_sort_by_column(GtkWidget* window, GtkWidget* clist,
								  user_data_t* user_data _U_)
{
	column_arrows *col_arrows;
	GdkBitmap *ascend_bm, *descend_bm;
	GdkPixmap *ascend_pm, *descend_pm;
	GtkStyle *win_style;
	GtkWidget *column_lb;
	int i;

	col_arrows = (column_arrows *) g_malloc(sizeof(column_arrows) * NUM_COLS);
	win_style = gtk_widget_get_style(window);
	ascend_pm = gdk_pixmap_create_from_xpm_d(window->window,
			&ascend_bm,
			&win_style->bg[GTK_STATE_NORMAL],
			(gchar **)clist_ascend_xpm);
	descend_pm = gdk_pixmap_create_from_xpm_d(window->window,
			&descend_bm,
			&win_style->bg[GTK_STATE_NORMAL],
			(gchar **)clist_descend_xpm);

	for (i=0; i<NUM_COLS; i++) {
		col_arrows[i].table = gtk_table_new(2, 2, FALSE);
		gtk_table_set_col_spacings(GTK_TABLE(col_arrows[i].table), 5);
		column_lb = gtk_label_new(titles[i]);
		gtk_table_attach(GTK_TABLE(col_arrows[i].table), column_lb, 0, 1, 0, 2, GTK_SHRINK, GTK_SHRINK, 0, 0);
		gtk_widget_show(column_lb);

		col_arrows[i].ascend_pm = gtk_pixmap_new(ascend_pm, ascend_bm);
		gtk_table_attach(GTK_TABLE(col_arrows[i].table), col_arrows[i].ascend_pm, 1, 2, 1, 2, GTK_SHRINK, GTK_SHRINK, 0, 0);
		col_arrows[i].descend_pm = gtk_pixmap_new(descend_pm, descend_bm);
		gtk_table_attach(GTK_TABLE(col_arrows[i].table), col_arrows[i].descend_pm, 1, 2, 0, 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
		/* make packet-nr be the default sort order */
		if (i == 0) {
			gtk_widget_show(col_arrows[i].ascend_pm);
		}
		gtk_clist_set_column_widget(GTK_CLIST(clist), i, col_arrows[i].table);
		gtk_widget_show(col_arrows[i].table);
	}

	SIGNAL_CONNECT(clist, "click-column", click_column_cb, col_arrows);

	return col_arrows;
}

/****************************************************************************/
/* Create the dialog box with all widgets */
void create_rtp_dialog(user_data_t* user_data)
{
	GtkWidget *window = NULL;
	GtkWidget *clist_fwd;
	GtkWidget *clist_rev;
	GtkWidget *label_stats_fwd;
	GtkWidget *label_stats_rev;
	GtkWidget *notebook;

	GtkWidget *main_vb, *page, *page_r;
	GtkWidget *label;
	GtkWidget *scrolled_window, *scrolled_window_r/*, *frame, *text, *label4, *page_help*/;
	GtkWidget *box4, *voice_bt, *refresh_bt, *goto_bt, *close_bt, *csv_bt, *next_bt;
#ifdef USE_CONVERSATION_GRAPH
	GtkWidget *graph_bt;
#endif

	gchar label_forward[150];
	gchar label_reverse[150];

	gchar str_ip_src[16];
	gchar str_ip_dst[16];
	column_arrows *col_arrows_fwd;
	column_arrows *col_arrows_rev;
	

	window = window_new(GTK_WINDOW_TOPLEVEL, "Ethereal: RTP Stream Analysis");

	/* Container for each row of widgets */
	main_vb = gtk_vbox_new(FALSE, 2);
	gtk_container_border_width(GTK_CONTAINER(main_vb), 2);
	gtk_container_add(GTK_CONTAINER(window), main_vb);
	gtk_widget_show(main_vb);

	/* Notebooks... */
	strcpy(str_ip_src, ip_to_str((ip_addr_p)&user_data->ip_src_fwd));
	strcpy(str_ip_dst, ip_to_str((ip_addr_p)&user_data->ip_dst_fwd));

	g_snprintf(label_forward, 149, 
		"Analysing stream from  %s port %u  to  %s port %u   SSRC = %u", 
		str_ip_src, user_data->port_src_fwd, str_ip_dst, user_data->port_dst_fwd, user_data->ssrc_fwd);

	strcpy(str_ip_src, ip_to_str((ip_addr_p)&user_data->ip_src_rev));
	strcpy(str_ip_dst, ip_to_str((ip_addr_p)&user_data->ip_dst_rev));

	g_snprintf(label_reverse, 149,
		"Analysing stream from  %s port %u  to  %s port %u   SSRC = %u", 
		str_ip_src, user_data->port_src_rev, str_ip_dst, user_data->port_dst_rev, user_data->ssrc_rev);

	/* Start a notebook for flipping between sets of changes */
	notebook = gtk_notebook_new();
	gtk_container_add(GTK_CONTAINER(main_vb), notebook);
	OBJECT_SET_DATA(window, "notebook", notebook);
	SIGNAL_CONNECT(notebook, "switch_page", on_notebook_switch_page,
                       user_data);

	/* page for forward connection */
	page = gtk_vbox_new(FALSE, 8);
	gtk_container_set_border_width(GTK_CONTAINER(page), 8);

	/* direction label */
	label = gtk_label_new(label_forward);
	gtk_box_pack_start(GTK_BOX(page), label, FALSE, FALSE, 0);

	/* place for some statistics */
	label_stats_fwd = gtk_label_new("\n");
	gtk_box_pack_end(GTK_BOX(page), label_stats_fwd, FALSE, FALSE, 0);

	/* scrolled window */
	scrolled_window = scrolled_window_new(NULL, NULL);
	WIDGET_SET_SIZE(scrolled_window, 560, 200);

	/* packet clist */
	clist_fwd = create_clist(user_data);
	gtk_widget_show(clist_fwd);
	gtk_container_add(GTK_CONTAINER(scrolled_window), clist_fwd);
	gtk_box_pack_start(GTK_BOX(page), scrolled_window, TRUE, TRUE, 0);
	gtk_widget_show(scrolled_window);

	/* tab */
	label = gtk_label_new("  Forward Direction  ");
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page, label);

	/* same page for reversed connection */
	page_r = gtk_vbox_new(FALSE, 8);
	gtk_container_set_border_width(GTK_CONTAINER(page_r), 8);
	label = gtk_label_new(label_reverse);
	gtk_box_pack_start(GTK_BOX(page_r), label, FALSE, FALSE, 0);
	label_stats_rev = gtk_label_new("\n");
	gtk_box_pack_end(GTK_BOX(page_r), label_stats_rev, FALSE, FALSE, 0);

	scrolled_window_r = scrolled_window_new(NULL, NULL);
	WIDGET_SET_SIZE(scrolled_window_r, 560, 200);

	clist_rev = create_clist(user_data);
	gtk_widget_show(clist_rev);
	gtk_container_add(GTK_CONTAINER(scrolled_window_r), clist_rev);
	gtk_box_pack_start(GTK_BOX(page_r), scrolled_window_r, TRUE, TRUE, 0);
	gtk_widget_show(scrolled_window_r);

	label = gtk_label_new("  Reversed Direction  ");
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page_r, label);

	/* page for help&about or future
	page_help = gtk_hbox_new(FALSE, 5);
	label = gtk_label_new("     Future    ");
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page_help, label);
	frame = gtk_frame_new("");
	text = gtk_label_new("\n\nMaybe some more statistics: delay and jitter distribution,...");
	gtk_label_set_justify(GTK_LABEL(text), GTK_JUSTIFY_LEFT);
	gtk_container_add(GTK_CONTAINER(frame), text);
	gtk_container_set_border_width(GTK_CONTAINER(frame), 20);
	gtk_box_pack_start(GTK_BOX(page_help), frame, TRUE, TRUE, 0);
	*/

	/* show all notebooks */
	gtk_widget_show_all(notebook);

	/* buttons */
	box4 = gtk_hbutton_box_new();
	gtk_box_pack_start(GTK_BOX(main_vb), box4, FALSE, FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(box4), 10);
	gtk_button_box_set_layout(GTK_BUTTON_BOX (box4), GTK_BUTTONBOX_EDGE);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX (box4), 0);
	gtk_button_box_set_child_ipadding(GTK_BUTTON_BOX (box4), 4, 0);
	gtk_widget_show(box4);

	voice_bt = gtk_button_new_with_label("Save payload...");
	gtk_container_add(GTK_CONTAINER(box4), voice_bt);
	gtk_widget_show(voice_bt);
	SIGNAL_CONNECT(voice_bt, "clicked", on_save_bt_clicked, user_data);

	csv_bt = gtk_button_new_with_label("Save as CSV...");
	gtk_container_add(GTK_CONTAINER(box4), csv_bt);
	gtk_widget_show(csv_bt);
	SIGNAL_CONNECT(csv_bt, "clicked", save_csv_as_cb, user_data);

	refresh_bt = BUTTON_NEW_FROM_STOCK(GTK_STOCK_REFRESH);
	gtk_container_add(GTK_CONTAINER(box4), refresh_bt);
	gtk_widget_show(refresh_bt);
	SIGNAL_CONNECT(refresh_bt, "clicked", on_refresh_bt_clicked, user_data);

	goto_bt = BUTTON_NEW_FROM_STOCK(GTK_STOCK_JUMP_TO);
	gtk_container_add(GTK_CONTAINER(box4), goto_bt);
	gtk_widget_show(goto_bt);
	SIGNAL_CONNECT(goto_bt, "clicked", on_goto_bt_clicked, user_data);

#ifdef USE_CONVERSATION_GRAPH
	graph_bt = gtk_button_new_with_label("Graph");
	gtk_container_add(GTK_CONTAINER(box4), graph_bt);
	gtk_widget_show(graph_bt);
	SIGNAL_CONNECT(graph_bt, "clicked", on_graph_bt_clicked, user_data);
#endif

	next_bt = gtk_button_new_with_label("Next non-Ok");
	gtk_container_add(GTK_CONTAINER(box4), next_bt);
	gtk_widget_show(next_bt);
	SIGNAL_CONNECT(next_bt, "clicked", on_next_bt_clicked, user_data);

	close_bt = BUTTON_NEW_FROM_STOCK(GTK_STOCK_CLOSE);
	gtk_container_add(GTK_CONTAINER(box4), close_bt);
	gtk_widget_show(close_bt);
    window_set_cancel_button(window, close_bt, window_cancel_button_cb);

    SIGNAL_CONNECT(window, "delete_event", window_delete_event_cb, NULL);
	SIGNAL_CONNECT(window, "destroy", on_destroy, user_data);

    gtk_widget_show(window);
    window_present(window);

	/* sort by column feature */
	col_arrows_fwd = add_sort_by_column(window, clist_fwd, user_data);
	col_arrows_rev = add_sort_by_column(window, clist_rev, user_data);

	/* some widget references need to be saved for outside use */
	user_data->dlg.window = window;
	user_data->dlg.clist_fwd = GTK_CLIST(clist_fwd);
	user_data->dlg.clist_rev = GTK_CLIST(clist_rev);
	user_data->dlg.label_stats_fwd = label_stats_fwd;
	user_data->dlg.label_stats_rev = label_stats_rev;
	user_data->dlg.notebook = notebook;
	user_data->dlg.selected_clist = GTK_CLIST(clist_fwd);
	user_data->dlg.selected_row = 0;
	user_data->dlg.col_arrows_fwd = col_arrows_fwd;
	user_data->dlg.col_arrows_rev = col_arrows_rev;
}


/****************************************************************************/
static gboolean process_node(proto_node *ptree_node, header_field_info *hfinformation,
							gchar* proto_field, guint32* p_result)
{
	field_info            *finfo;
	proto_node            *proto_sibling_node;
	header_field_info     *hfssrc;
	ipv4_addr             *ipv4;

	finfo = PITEM_FINFO(ptree_node);

	if (hfinformation==(finfo->hfinfo)) {
		hfssrc = proto_registrar_get_byname(proto_field);
		if (hfssrc == NULL)
			return FALSE;
		for(ptree_node=ptree_node->first_child; ptree_node!=NULL; 
					ptree_node=ptree_node->next) {
			finfo=PITEM_FINFO(ptree_node);
			if (hfssrc==finfo->hfinfo) {
				if (hfinformation->type==FT_IPv4) {
					ipv4 = fvalue_get(&finfo->value);
					*p_result = ipv4_get_net_order_addr(ipv4);
				}
				else {
					*p_result = fvalue_get_integer(&finfo->value);
				}
				return TRUE;
			}
		}
	}

	proto_sibling_node = ptree_node->next;

	if (proto_sibling_node) {
		return process_node(proto_sibling_node, hfinformation, proto_field, p_result);
	}
	else
	return FALSE;
}

/****************************************************************************/
static gboolean get_int_value_from_proto_tree(proto_tree *protocol_tree,
											 gchar* proto_name,
											 gchar* proto_field,
											 guint32* p_result)
{
	proto_node      *ptree_node;
	header_field_info     *hfinformation;

	hfinformation = proto_registrar_get_byname(proto_name);
	if (hfinformation == NULL)
		return FALSE;

	ptree_node = ((proto_node *)protocol_tree)->first_child;
	if (!ptree_node)
		return FALSE;

	return process_node(ptree_node, hfinformation, proto_field, p_result);
}


/* XXX just copied from gtk/rpc_stat.c */
void protect_thread_critical_region(void);
void unprotect_thread_critical_region(void);

/****************************************************************************/
/* XXX only handles RTP over IPv4, should add IPv6 support */
void rtp_analysis(
		guint32 ip_src_fwd,
		guint16 port_src_fwd,
		guint32 ip_dst_fwd,
		guint16 port_dst_fwd,
		guint32 ssrc_fwd,
		guint32 ip_src_rev,
		guint16 port_src_rev,
		guint32 ip_dst_rev,
		guint16 port_dst_rev,
		guint32 ssrc_rev
		)
{
	user_data_t *user_data;
	int fd;

	/* init */
	user_data = g_malloc(sizeof(user_data_t));

	user_data->ip_src_fwd = ip_src_fwd;
	user_data->port_src_fwd = port_src_fwd;
	user_data->ip_dst_fwd = ip_dst_fwd;
	user_data->port_dst_fwd = port_dst_fwd;
	user_data->ssrc_fwd = ssrc_fwd;
	user_data->ip_src_rev = ip_src_rev;
	user_data->port_src_rev = port_src_rev;
	user_data->ip_dst_rev = ip_dst_rev;
	user_data->port_dst_rev = port_dst_rev;
	user_data->ssrc_rev = ssrc_rev;

	/* file names for storing sound data */
	/*XXX: check for errors*/
	fd = create_tempfile(user_data->f_tempname, sizeof(user_data->f_tempname),
		"ether_rtp_f");
	close(fd);
	fd = create_tempfile(user_data->r_tempname, sizeof(user_data->r_tempname),
		"ether_rtp_r");
	close(fd);
	user_data->forward.saveinfo.fp = NULL;
	user_data->reversed.saveinfo.fp = NULL;
	user_data->dlg.save_voice_as_w = NULL;
	user_data->dlg.save_csv_as_w = NULL;
#ifdef USE_CONVERSATION_GRAPH
	user_data->dlg.graph_window = NULL;
	user_data->series_fwd.value_pairs = NULL;
	user_data->series_rev.value_pairs = NULL;
#endif

	/* create the dialog box */
	create_rtp_dialog(user_data);

	/* proceed as if the Refresh button would have been pressed */
	on_refresh_bt_clicked(NULL, user_data);
}

/****************************************************************************/
/* entry point from main menu */
void rtp_analysis_cb(GtkWidget *w _U_, gpointer data _U_) 
{
	guint32 ip_src_fwd;
	guint16 port_src_fwd;
	guint32 ip_dst_fwd;
	guint16 port_dst_fwd;
	guint32 ssrc_fwd = 0;
	guint32 ip_src_rev;
	guint16 port_src_rev;
	guint32 ip_dst_rev;
	guint16 port_dst_rev;
	guint32 ssrc_rev = 0;

	gchar filter_text[256];
	dfilter_t *sfcode;
	capture_file *cf;
	epan_dissect_t *edt;
	gint err;
	gchar *err_info;
	gboolean frame_matched;
	frame_data *fdata;
	GList *strinfo_list;
	GList *filtered_list = NULL;
	rtp_stream_info_t *strinfo;
	guint nfound;

	/* Try to compile the filter. */
	strcpy(filter_text,"rtp && ip");
	if (!dfilter_compile(filter_text, &sfcode)) {
		simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK, dfilter_error_msg);
		return;
	}
	/* we load the current file into cf variable */
	cf = &cfile;
	fdata = cf->current_frame;
	
	/* we are on the selected frame now */
	if (fdata == NULL)
		return; /* if we exit here it's an error */

	/* dissect the current frame */
	if (!wtap_seek_read(cf->wth, fdata->file_off, &cf->pseudo_header,
	    cf->pd, fdata->cap_len, &err, &err_info)) {
		simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK,
			cf_read_error_message(err, err_info), cf->filename);
		return;
	}
	edt = epan_dissect_new(TRUE, FALSE);
	epan_dissect_prime_dfilter(edt, sfcode);
	epan_dissect_run(edt, &cf->pseudo_header, cf->pd, fdata, &cf->cinfo);
	frame_matched = dfilter_apply_edt(sfcode, edt);
	
	/* if it is not an rtp frame, show the rtpstream dialog */
	frame_matched = dfilter_apply_edt(sfcode, edt);
	if (frame_matched != 1) {
		epan_dissect_free(edt);
		simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK,
		    "You didn't choose a RTP packet!");
		return;
	}

	/* ok, it is a RTP frame, so let's get the ip and port values */
	g_memmove(&ip_src_fwd, edt->pi.src.data, 4);
	g_memmove(&ip_dst_fwd, edt->pi.dst.data, 4);
	port_src_fwd = edt->pi.srcport;
	port_dst_fwd = edt->pi.destport;

	/* assume the inverse ip/port combination for the reverse direction */
	g_memmove(&ip_src_rev, edt->pi.dst.data, 4);
	g_memmove(&ip_dst_rev, edt->pi.src.data, 4);
	port_src_rev = edt->pi.destport;
	port_dst_rev = edt->pi.srcport;
	
	/* now we need the SSRC value of the current frame */
	if (!get_int_value_from_proto_tree(edt->tree, "rtp", "rtp.ssrc", &ssrc_fwd)) {
		simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK,
		    "SSRC value couldn't be found!");
		return;
	}

	/* search for reversed direction in the global rtp streams list */
	nfound = 0;
	strinfo_list = g_list_first(rtpstream_get_info()->strinfo_list);
	while (strinfo_list)
	{
		strinfo = (rtp_stream_info_t*)(strinfo_list->data);
		if (strinfo->src_addr==ip_src_fwd
			&& strinfo->src_port==port_src_fwd
			&& strinfo->dest_addr==ip_dst_fwd
			&& strinfo->dest_port==port_dst_fwd)
		{
			filtered_list = g_list_prepend(filtered_list, strinfo);
		}

		if (strinfo->src_addr==ip_src_rev
			&& strinfo->src_port==port_src_rev
			&& strinfo->dest_addr==ip_dst_rev
			&& strinfo->dest_port==port_dst_rev)
		{
			++nfound;
			filtered_list = g_list_append(filtered_list, strinfo);
			if (ssrc_rev==0)
				ssrc_rev = strinfo->ssrc;
		}

		strinfo_list = g_list_next(strinfo_list);
	}

	/* if more than one reverse streams found, we let the user choose the right one */
	if (nfound>1) {
		rtpstream_dlg_show(filtered_list);
		return;
	}
	else {
		rtp_analysis(
			ip_src_fwd,
			port_src_fwd,
			ip_dst_fwd,
			port_dst_fwd,
			ssrc_fwd,
			ip_src_rev,
			port_src_rev,
			ip_dst_rev,
			port_dst_rev,
			ssrc_rev
			);
	}
}

/****************************************************************************/
static void
rtp_analysis_init(char *dummy _U_)
{
	rtp_analysis_cb(NULL, NULL);
}

/****************************************************************************/
void
register_tap_listener_rtp_analysis(void)
{
	register_ethereal_tap("rtp", rtp_analysis_init);

	register_tap_menu_item("RTP/Stream Analysis...", REGISTER_TAP_GROUP_NONE,
	    rtp_analysis_cb, NULL, NULL, NULL);
}
