/* proto_hier_stats.c
 * Routines for calculating statistics based on protocol.
 *
 * $Id: proto_hier_stats.c,v 1.18 2003/11/24 22:11:53 guy Exp $
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

#include "globals.h"
#include "proto_hier_stats.h"
#include "progress_dlg.h"
#include "simple_dialog.h"
#include <epan/epan_dissect.h>
#include <wtap.h>

#include <stdio.h>
#include <glib.h>

/* Update the progress bar this many times when scanning the packet list. */
#define N_PROGBAR_UPDATES	100

#define STAT_NODE_STATS(n)   ((ph_stats_node_t*)(n)->data)
#define STAT_NODE_HFINFO(n)  (STAT_NODE_STATS(n)->hfinfo)

static GNode*
find_stat_node(GNode *parent_stat_node, header_field_info *needle_hfinfo)
{
	GNode			*needle_stat_node;
	header_field_info	*hfinfo;
	ph_stats_node_t         *stats;

	needle_stat_node = g_node_first_child(parent_stat_node);

	while (needle_stat_node) {
		hfinfo = STAT_NODE_HFINFO(needle_stat_node);
		if (hfinfo &&  hfinfo->id == needle_hfinfo->id) {
			return needle_stat_node;
		}
		needle_stat_node = g_node_next_sibling(needle_stat_node);
	}

	/* None found. Create one. */
	stats = g_new(ph_stats_node_t, 1);

	/* Intialize counters */
	stats->hfinfo = needle_hfinfo;
	stats->num_pkts_total = 0;
	stats->num_pkts_last = 0;
	stats->num_bytes_total = 0;
	stats->num_bytes_last = 0;

	needle_stat_node = g_node_new(stats);
	g_node_append(parent_stat_node, needle_stat_node);
	return needle_stat_node;
}


static void
process_node(proto_item *ptree_node, GNode *parent_stat_node, ph_stats_t *ps, guint pkt_len)
{
	field_info		*finfo;
	ph_stats_node_t	*stats;
	proto_item		*proto_sibling_node;
	GNode			*stat_node;

	finfo = PITEM_FINFO(ptree_node);
	g_assert(finfo);

	stat_node = find_stat_node(parent_stat_node, finfo->ptr_u.hfinfo);

	/* Assert that the finfo is related to a protocol, not a field. */
	g_assert(finfo->ptr_u.hfinfo->parent == -1);

	stats = STAT_NODE_STATS(stat_node);
	stats->num_pkts_total++;
	stats->num_bytes_total += pkt_len;

	proto_sibling_node = g_node_next_sibling(ptree_node);

	if (proto_sibling_node) {
		process_node(proto_sibling_node, stat_node, ps, pkt_len);
	}
	else {
		stats->num_pkts_last++;
		stats->num_bytes_last += pkt_len;
	}
}



static void
process_tree(proto_tree *protocol_tree, ph_stats_t* ps, guint pkt_len)
{
	proto_item	*ptree_node;

	ptree_node = g_node_first_child(protocol_tree);
	if (!ptree_node) {
		return;
	}

	process_node(ptree_node, ps->stats_tree, ps, pkt_len);
}

static gboolean
process_frame(frame_data *frame, column_info *cinfo, ph_stats_t* ps)
{
	epan_dissect_t			*edt;
	union wtap_pseudo_header	phdr;
	guint8				pd[WTAP_MAX_PACKET_SIZE];
	int				err;

	/* Load the frame from the capture file */
	if (!wtap_seek_read(cfile.wth, frame->file_off, &phdr, pd,
	    frame->cap_len, &err)) {
		simple_dialog(ESD_TYPE_CRIT, NULL,
		    file_read_error_message(err), cfile.filename);
		return FALSE;	/* failure */
	}

	/* Dissect the frame */
	edt = epan_dissect_new(TRUE, FALSE);
	epan_dissect_run(edt, &phdr, pd, frame, cinfo);

	/* Get stats from this protocol tree */
	process_tree(edt->tree, ps, frame->pkt_len);

	/* Free our memory. */
	epan_dissect_free(edt);

	return TRUE;	/* success */
}



ph_stats_t*
ph_stats_new(void)
{
	ph_stats_t	*ps;
	frame_data	*frame;
	guint		tot_packets, tot_bytes;
	progdlg_t	*progbar = NULL;
	gboolean	stop_flag;
	int		count;
	float		prog_val;
	GTimeVal	start_time;
	gchar		status_str[100];

	/* Initialize the data */
	ps = g_new(ph_stats_t, 1);
	ps->tot_packets = 0;
	ps->tot_bytes = 0;
	ps->stats_tree = g_node_new(NULL);

	/* Update the progress bar when it gets to this value. */
	cfile.progbar_nextstep = 0;
	/* When we reach the value that triggers a progress bar update,
	   bump that value by this amount. */
	cfile.progbar_quantum = cfile.count/N_PROGBAR_UPDATES;
	/* Count of packets at which we've looked. */
	count = 0;

	stop_flag = FALSE;
	g_get_current_time(&start_time);

	tot_packets = 0;
	tot_bytes = 0;

	for (frame = cfile.plist; frame != NULL; frame = frame->next) {
		/* Update the progress bar, but do it only N_PROGBAR_UPDATES
		   times; when we update it, we have to run the GTK+ main
		   loop to get it to repaint what's pending, and doing so
		   may involve an "ioctl()" to see if there's any pending
		   input from an X server, and doing that for every packet
		   can be costly, especially on a big file. */
		if (count >= cfile.progbar_nextstep) {
			/* let's not divide by zero. I should never be started
			 * with count == 0, so let's assert that
			 */
			g_assert(cfile.count > 0);

			prog_val = (gfloat) count / cfile.count;

			if (progbar == NULL)
				/* Create the progress bar if necessary */
				progbar = delayed_create_progress_dlg(
				    "Computing", "protocol hierarchy statistics", "Stop",
				    &stop_flag, &start_time, prog_val);

			if (progbar != NULL) {
				g_snprintf(status_str, sizeof(status_str),
					"%4u of %u frames", count, cfile.count);
				update_progress_dlg(progbar, prog_val, status_str);
			}

			cfile.progbar_nextstep += cfile.progbar_quantum;
		}

		if (stop_flag) {
			/* Well, the user decided to abort the statistics.
			   computation process  Just stop. */
			break;
		}

		/* Skip frames that are hidden due to the display filter.
		   XXX - should the progress bar count only packets that
		   passed the display filter?  If so, it should
		   probably do so for other loops (see "file.c") that
		   look only at those packets. */
		if (frame->flags.passed_dfilter) {
			if (!process_frame(frame, &cfile.cinfo, ps)) {
				/*
				 * Give up, and set "stop_flag" so we
				 * just abort rather than popping up
				 * the statistics window.
				 */
				stop_flag = TRUE;
				break;
			}

			tot_packets++;
			tot_bytes += frame->pkt_len;
		}

		count++;
	}

	/* We're done calculating the statistics; destroy the progress bar
           if it was created. */
	if (progbar != NULL)
		destroy_progress_dlg(progbar);

	if (stop_flag) {
		/*
		 * We quit in the middle; throw away the statistics
		 * and return NULL, so our caller doesn't pop up a
		 * window with the incomplete statistics.
		 */
		ph_stats_free(ps);
		return NULL;
	}

	ps->tot_packets = tot_packets;
	ps->tot_bytes = tot_bytes;

	return ps;
}

static gboolean
stat_node_free(GNode *node, gpointer data _U_)
{
	ph_stats_node_t	*stats = node->data;

	if (stats) {
		g_free(stats);
	}
	return FALSE;
}

void
ph_stats_free(ph_stats_t *ps)
{

	if (ps->stats_tree) {
		g_node_traverse(ps->stats_tree, G_IN_ORDER,
				G_TRAVERSE_ALL, -1,
				stat_node_free, NULL);
		g_node_destroy(ps->stats_tree);
	}

	g_free(ps);
}
