/* rtp_stream.c
 * RTP streams summary addition for ethereal
 *
 * $Id: rtp_stream.c,v 1.14 2004/02/12 22:24:28 guy Exp $
 *
 * Copyright 2003, Alcatel Business Systems
 * By Lars Ruoff <lars.ruoff@gmx.net>
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
# include "config.h"
#endif

#include "rtp_stream.h"
#include "rtp_stream_dlg.h"

#include "globals.h"

#include "tap.h"
#include "register.h"
#include "packet-rtp.h"

#include "alert_box.h"
#include "simple_dialog.h"

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#include <string.h>

#ifndef O_BINARY
#define O_BINARY 0
#endif


/****************************************************************************/
/* the one and only global rtpstream_tapinfo_t structure */
static rtpstream_tapinfo_t the_tapinfo_struct =
	{0, NULL, 0, TAP_ANALYSE, NULL, NULL, NULL, 0, FALSE};


/****************************************************************************/
/* GCompareFunc style comparison function for _rtp_stream_info */
gint rtp_stream_info_cmp(gconstpointer aa, gconstpointer bb)
{
	const struct _rtp_stream_info* a = aa;
	const struct _rtp_stream_info* b = bb;

	if (a==b)
		return 0;
	if (a==NULL || b==NULL)
		return 1;
	if ((a->src_addr == b->src_addr)
		&& (a->src_port == b->src_port)
		&& (a->dest_addr == b->dest_addr)
		&& (a->dest_port == b->dest_port)
		&& (a->ssrc == b->ssrc))
		return 0;
	else
		return 1;
}


/****************************************************************************/
/* when there is a [re]reading of packet's */
void rtpstream_reset(rtpstream_tapinfo_t *tapinfo)
{
	GList* list;

	if (tapinfo->mode == TAP_ANALYSE) {
		/* free the data items first */
		list = g_list_first(tapinfo->strinfo_list);
		while (list)
		{
			g_free(list->data);
			list = g_list_next(list);
		}
		g_list_free(tapinfo->strinfo_list);
		tapinfo->strinfo_list = NULL;
		tapinfo->nstreams = 0;
		tapinfo->npackets = 0;
	}

	++(tapinfo->launch_count);

	return;
}

/****************************************************************************/
/* redraw the output */
void rtpstream_draw(rtpstream_tapinfo_t *tapinfo _U_)
{
/* XXX: see rtpstream_on_update in rtp_streams_dlg.c for comments
	gtk_signal_emit_by_name(top_level, "signal_rtpstream_update");
*/
	rtpstream_dlg_update(the_tapinfo_struct.strinfo_list);
	return;
}


/*
* rtpdump file format
*
* The file starts with the tool to be used for playing this file,
* the multicast/unicast receive address and the port.
*
* #!rtpplay1.0 224.2.0.1/3456\n
*
* This is followed by one binary header (RD_hdr_t) and one RD_packet_t
* structure for each received packet.  All fields are in network byte
* order.  We don't need the source IP address since we can do mapping
* based on SSRC.  This saves (a little) space, avoids non-IPv4
* problems and privacy/security concerns. The header is followed by
* the RTP/RTCP header and (optionally) the actual payload.
*/

#define RTPFILE_VERSION "1.0"

/*
* Write a header to the current output file.
* The header consists of an identifying string, followed
* by a binary structure.
*/
static void rtp_write_header(rtp_stream_info_t *strinfo, FILE *file)
{
	guint32 start_sec;     /* start of recording (GMT) (seconds) */
	guint32 start_usec;    /* start of recording (GMT) (microseconds)*/
	guint32 source;        /* network source (multicast address) */
	guint16 port;          /* UDP port */
	guint16 padding;       /* 2 padding bytes */
	
	fprintf(file, "#!rtpplay%s %s/%u\n", RTPFILE_VERSION,
		ip_to_str((guint8*) &strinfo->dest_addr),
		strinfo->dest_port);

	start_sec = g_htonl(strinfo->start_sec);
	start_usec = g_htonl(strinfo->start_usec);
	source = strinfo->src_addr; /* already is in network order */
	port = g_htons(strinfo->src_port);
	padding = 0;

	fwrite(&start_sec, 4, 1, file);
	fwrite(&start_usec, 4, 1, file);
	fwrite(&source, 4, 1, file);
	fwrite(&port, 2, 1, file);
	fwrite(&padding, 2, 1, file);
}

/* utility function for writing a sample to file in rtpdump -F dump format (.rtp)*/
static void rtp_write_sample(rtp_sample_t* sample, FILE* file)
{
	guint16 length;    /* length of packet, including this header (may
	                     be smaller than plen if not whole packet recorded) */
	guint16 plen;      /* actual header+payload length for RTP, 0 for RTCP */
	guint32 offset;    /* milliseconds since the start of recording */

	length = g_htons(sample->header.frame_length + 8);
	plen = g_htons(sample->header.frame_length);
	offset = g_htonl(sample->header.rec_time);

	fwrite(&length, 2, 1, file);
	fwrite(&plen, 2, 1, file);
	fwrite(&offset, 4, 1, file);
	fwrite(sample->frame, sample->header.frame_length, 1, file);
}


/****************************************************************************/
/* whenever a RTP packet is seen by the tap listener */
int rtpstream_packet(rtpstream_tapinfo_t *tapinfo _U_, packet_info *pinfo, epan_dissect_t *edt _U_, struct _rtp_info *rtpinfo _U_)
{
	rtp_stream_info_t tmp_strinfo;
	rtp_stream_info_t *strinfo = NULL;
	GList* list;

	rtp_sample_t sample;

	/* gather infos on the stream this packet is part of */
	g_memmove(&(tmp_strinfo.src_addr), pinfo->src.data, 4);
	tmp_strinfo.src_port = pinfo->srcport;
	g_memmove(&(tmp_strinfo.dest_addr), pinfo->dst.data, 4);
	tmp_strinfo.dest_port = pinfo->destport;
	tmp_strinfo.ssrc = rtpinfo->info_sync_src;
	tmp_strinfo.pt = rtpinfo->info_payload_type;

	if (tapinfo->mode == TAP_ANALYSE) {
		/* check wether we already have a stream with these parameters in the list */
		list = g_list_first(tapinfo->strinfo_list);
		while (list)
		{
			if (rtp_stream_info_cmp(&tmp_strinfo, (rtp_stream_info_t*)(list->data))==0)
			{
				strinfo = (rtp_stream_info_t*)(list->data);  /*found!*/
				break;
			}
			list = g_list_next(list);
		}

		/* not in the list? then create a new entry */
		if (!strinfo) {
			tmp_strinfo.npackets = 0;
			tmp_strinfo.first_frame_num = pinfo->fd->num;
			tmp_strinfo.start_sec = pinfo->fd->abs_secs;
			tmp_strinfo.start_usec = pinfo->fd->abs_usecs;
			tmp_strinfo.tag_vlan_error = 0;
			tmp_strinfo.tag_diffserv_error = 0;
			tmp_strinfo.vlan_id = 0;
			strinfo = g_malloc(sizeof(rtp_stream_info_t));
			*strinfo = tmp_strinfo;  /* memberwise copy of struct */
			tapinfo->strinfo_list = g_list_append(tapinfo->strinfo_list, strinfo);
		}

		/* increment the packets counter for this stream */
		++(strinfo->npackets);

		/* increment the packets counter of all streams */
		++(tapinfo->npackets);
		
		return 1;  /* refresh output */
	}
	else if (tapinfo->mode == TAP_SAVE) {
		if (rtp_stream_info_cmp(&tmp_strinfo, tapinfo->filter_stream_fwd)==0) {
			/* XXX - what if rtpinfo->info_all_data_present is
			   FALSE, so that we don't *have* all the data? */
			sample.header.rec_time = 
				(pinfo->fd->abs_usecs + 1000000 - tapinfo->filter_stream_fwd->start_usec)/1000
				+ (pinfo->fd->abs_secs - tapinfo->filter_stream_fwd->start_sec - 1)*1000;
			sample.header.frame_length = rtpinfo->info_data_len;
			sample.frame = rtpinfo->info_data;
			rtp_write_sample(&sample, tapinfo->save_file);
		}
	}
	else if (tapinfo->mode == TAP_MARK) {
		if (rtp_stream_info_cmp(&tmp_strinfo, tapinfo->filter_stream_fwd)==0
			|| rtp_stream_info_cmp(&tmp_strinfo, tapinfo->filter_stream_rev)==0)
		{
			mark_frame(&cfile, pinfo->fd);
		}
	}

	return 0;
}

/****************************************************************************/
/* scan for RTP streams */
void rtpstream_scan(void)
{
	gboolean was_registered = the_tapinfo_struct.is_registered;
	if (!the_tapinfo_struct.is_registered)
		register_tap_listener_rtp_stream();

	the_tapinfo_struct.mode = TAP_ANALYSE;
	retap_packets(&cfile);

	if (!was_registered)
		remove_tap_listener_rtp_stream();
}


/****************************************************************************/
/* save rtp dump of stream_fwd */
gboolean rtpstream_save(rtp_stream_info_t* stream, const gchar *filename)
{
	gboolean was_registered = the_tapinfo_struct.is_registered;
	/* open file for saving */
	the_tapinfo_struct.save_file = fopen(filename, "wb");
	if (the_tapinfo_struct.save_file==NULL) {
		open_failure_alert_box(filename, errno, TRUE);
		return FALSE;
	}

	rtp_write_header(stream, the_tapinfo_struct.save_file);
	if (ferror(the_tapinfo_struct.save_file)) {
		write_failure_alert_box(filename, errno);
		fclose(the_tapinfo_struct.save_file);
		return FALSE;
	}

	if (!the_tapinfo_struct.is_registered)
		register_tap_listener_rtp_stream();

	the_tapinfo_struct.mode = TAP_SAVE;
	the_tapinfo_struct.filter_stream_fwd = stream;
	retap_packets(&cfile);
	the_tapinfo_struct.mode = TAP_ANALYSE;

	if (!was_registered)
		remove_tap_listener_rtp_stream();

	if (ferror(the_tapinfo_struct.save_file)) {
		write_failure_alert_box(filename, errno);
		fclose(the_tapinfo_struct.save_file);
		return FALSE;
	}

	if (fclose(the_tapinfo_struct.save_file) == EOF) {
		write_failure_alert_box(filename, errno);
		return FALSE;
	}
	return TRUE;
}


/****************************************************************************/
/* mark packets in stream_fwd or stream_rev */
void rtpstream_mark(rtp_stream_info_t* stream_fwd, rtp_stream_info_t* stream_rev)
{
	gboolean was_registered = the_tapinfo_struct.is_registered;
	if (!the_tapinfo_struct.is_registered)
		register_tap_listener_rtp_stream();

	the_tapinfo_struct.mode = TAP_MARK;
	the_tapinfo_struct.filter_stream_fwd = stream_fwd;
	the_tapinfo_struct.filter_stream_rev = stream_rev;
	retap_packets(&cfile);
	the_tapinfo_struct.mode = TAP_ANALYSE;

	if (!was_registered)
		remove_tap_listener_rtp_stream();
}


/****************************************************************************/
const rtpstream_tapinfo_t* rtpstream_get_info(void)
{
	return &the_tapinfo_struct;
}


/****************************************************************************/
/* TAP INTERFACE */
/****************************************************************************/

/****************************************************************************/
static void
rtpstream_init_tap(char *dummy _U_)
{
	/* XXX: never called? */
}


/* XXX just copied from gtk/rpc_stat.c */
void protect_thread_critical_region(void);
void unprotect_thread_critical_region(void);

/****************************************************************************/
void
remove_tap_listener_rtp_stream(void)
{
	if (the_tapinfo_struct.is_registered) {
		protect_thread_critical_region();
		remove_tap_listener(&the_tapinfo_struct);
		unprotect_thread_critical_region();

		the_tapinfo_struct.is_registered = FALSE;
	}
}


/****************************************************************************/
void
register_tap_listener_rtp_stream(void)
{
	GString *error_string;

	if (!the_tapinfo_struct.is_registered) {
		register_ethereal_tap("rtp", rtpstream_init_tap);

		error_string = register_tap_listener("rtp", &the_tapinfo_struct,
			NULL,
			(void*)rtpstream_reset, (void*)rtpstream_packet, (void*)rtpstream_draw);

		if (error_string != NULL) {
			simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK,
				      error_string->str);
			g_string_free(error_string, TRUE);
			exit(1);
		}

		the_tapinfo_struct.is_registered = TRUE;
	}
}
