/* rtp_analysis.h
 * RTP analysis addition for ethereal
 *
 * $Id$
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

#ifndef RTP_ANALYSIS_H_INCLUDED
#define RTP_ANALYSIS_H_INCLUDED

#include <glib.h>
#include <epan/address.h>
#include <epan/packet_info.h>

/** @file
 *  ??? 
 *  @todo what's this?
 */

void rtp_analysis(
		address *ip_src_fwd,
		guint16 port_src_fwd,
		address *ip_dst_fwd,
		guint16 port_dst_fwd,
		guint32 ssrc_fwd,
		address *ip_src_rev,
		guint16 port_src_rev,
		address *ip_dst_rev,
		guint16 port_dst_rev,
		guint32 ssrc_rev
		);

/****************************************************************************/
/* structure that holds the information about the forward and reversed direction */
typedef struct _bw_history_item {
        double time;
        guint32 bytes;
} bw_history_item;

#define BUFF_BW 300 

typedef struct _tap_rtp_stat_t {
	gboolean first_packet;     /* do not use in code that is called after rtp_packet_analyse */
	                           /* use (flags & STAT_FLAG_FIRST) instead */
	/* all of the following fields will be initialized after
	 rtp_packet_analyse has been called */
	guint32 flags;             /* see STAT_FLAG-defines below */
	guint16 seq_num;
	guint32 timestamp;
	guint32 delta_timestamp;
	double bandwidth;
	bw_history_item bw_history[BUFF_BW];
	guint16 bw_start_index;
	guint16 bw_index;
	guint32 total_bytes;
	double delta;
	double jitter;
	double diff;
	double time;
	double start_time;
	double max_delta;
	double max_jitter;
	double mean_jitter;
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

#define PT_UNDEFINED -1

/* status flags for the flags parameter in tap_rtp_stat_t */
#define STAT_FLAG_FIRST       0x01
#define STAT_FLAG_MARKER      0x02
#define STAT_FLAG_WRONG_SEQ   0x04
#define STAT_FLAG_PT_CHANGE   0x08
#define STAT_FLAG_PT_CN       0x10
#define STAT_FLAG_FOLLOW_PT_CN  0x20
#define STAT_FLAG_REG_PT_CHANGE  0x40
#define STAT_FLAG_WRONG_TIMESTAMP  0x80

/* forward */
struct _rtp_info;

/* function for analysing an RTP packet. Called from rtp_analysis and rtp_streams */
extern int rtp_packet_analyse(tap_rtp_stat_t *statinfo,
        packet_info *pinfo,
        const struct _rtp_info *rtpinfo);


#endif /*RTP_ANALYSIS_H_INCLUDED*/
