/* rtp_stream_dlg.c
 * RTP streams summary addition for ethereal
 *
 * $Id: rtp_stream_dlg.c,v 1.26 2004/06/30 21:17:55 etxrab Exp $
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
#  include <config.h>
#endif

#include "rtp_stream_dlg.h"
#include "rtp_stream.h"
#include "rtp_analysis.h"

#include "globals.h"
#include "epan/filesystem.h"

#include "tap_menu.h"
#include "dlg_utils.h"
#include "ui_util.h"
#include "compat_macros.h"
#include "gtkglobals.h"

#include "image/clist_ascend.xpm"
#include "image/clist_descend.xpm"

#include "rtp_pt.h"

#include <string.h>


typedef const guint8 * ip_addr_p;

static const gchar FWD_LABEL_TEXT[] = "Select a forward stream with left mouse button";
static const gchar REV_LABEL_TEXT[] = "Select a reverse stream with SHIFT + left mouse button";

/****************************************************************************/
/* pointer to the one and only dialog window */
static GtkWidget *rtp_stream_dlg = NULL;

/* save as dialog box */
static GtkWidget *rtpstream_save_dlg = NULL;
static GtkWidget *clist = NULL;
static GtkWidget *top_label = NULL;
static GtkWidget *label_fwd = NULL;
static GtkWidget *label_rev = NULL;

static rtp_stream_info_t* selected_stream_fwd = NULL;  /* current selection */
static rtp_stream_info_t* selected_stream_rev = NULL;  /* current selection for reversed */
static GList *last_list = NULL;

static guint32 streams_nb = 0;     /* number of displayed streams */

/****************************************************************************/
/* append a line to clist */
static void add_to_clist(rtp_stream_info_t* strinfo)
{
	gchar label_text[256];
	gint added_row;
	gchar *data[8];
	gchar field[8][30];

	data[0]=&field[0][0];
	data[1]=&field[1][0];
	data[2]=&field[2][0];
	data[3]=&field[3][0];
	data[4]=&field[4][0];
	data[5]=&field[5][0];
	data[6]=&field[6][0];
	data[7]=&field[7][0];

	g_snprintf(field[0], 20, "%s", ip_to_str((const guint8*)&(strinfo->src_addr)));
	g_snprintf(field[1], 20, "%u", strinfo->src_port);
	g_snprintf(field[2], 20, "%s", ip_to_str((const guint8*)&(strinfo->dest_addr)));
	g_snprintf(field[3], 20, "%u", strinfo->dest_port);
	g_snprintf(field[4], 20, "%u", strinfo->ssrc);
	g_snprintf(field[5], 30, "%s", val_to_str(strinfo->pt, rtp_payload_type_vals,
		"Unknown (%u)"));
	g_snprintf(field[6], 20, "%u", strinfo->npackets);
	/* XXX: Comment field is not used for the moment */
/*	g_snprintf(field[7], 20, "%s", "");*/

	added_row = gtk_clist_append(GTK_CLIST(clist), data);

	/* set data pointer of last row to point to user data for that row */
	gtk_clist_set_row_data(GTK_CLIST(clist), added_row, strinfo);

	/* Update the top label with the number of detected streams */
	sprintf(label_text,
	        "Detected %d RTP streams. Choose one for forward and reverse direction for analysis",
	        ++streams_nb);
	gtk_label_set(GTK_LABEL(top_label), label_text);
}

/****************************************************************************/
static void save_stream_destroy_cb(GtkWidget *win _U_, gpointer user_data _U_)
{
	/* Note that we no longer have a Save voice info dialog box. */
	rtpstream_save_dlg = NULL;
}

/****************************************************************************/
/* save in a file */
static void save_stream_ok_cb(GtkWidget *ok_bt _U_, gpointer user_data _U_)
{
	gchar *g_dest;

	if (!selected_stream_fwd)
		return;

	g_dest = g_strdup(gtk_file_selection_get_filename(GTK_FILE_SELECTION (rtpstream_save_dlg)));

	/* Perhaps the user specified a directory instead of a file.
	Check whether they did. */
	if (test_for_directory(g_dest) == EISDIR) {
		/* It's a directory - set the file selection box to display it. */
		set_last_open_dir(g_dest);
		g_free(g_dest);
		file_selection_set_current_folder(rtpstream_save_dlg, get_last_open_dir());
		return;
	}

	/*
	 * Don't dismiss the dialog box if the save operation fails.
	 */
	if (!rtpstream_save(selected_stream_fwd, g_dest))
		return;

	window_destroy(GTK_WIDGET(rtpstream_save_dlg));
}


/****************************************************************************/
/* CALLBACKS                                                                */
/****************************************************************************/
static void
rtpstream_on_destroy                      (GtkObject       *object _U_,
                                        gpointer         user_data _U_)
{
	/* Remove the stream tap listener */
	remove_tap_listener_rtp_stream();

	/* Is there a save voice window open? */
	if (rtpstream_save_dlg != NULL)
		window_destroy(rtpstream_save_dlg);

	/* Clean up memory used by stream tap */
	rtpstream_reset((rtpstream_tapinfo_t*) rtpstream_get_info());

	/* Note that we no longer have a "RTP Streams" dialog box. */
	rtp_stream_dlg = NULL;
}


/****************************************************************************/
static void
rtpstream_on_unselect                  (GtkButton       *button _U_,
                                        gpointer         user_data _U_)
{
	selected_stream_fwd = NULL;
	selected_stream_rev = NULL;
	gtk_clist_unselect_all(GTK_CLIST(clist));
	gtk_label_set_text(GTK_LABEL(label_fwd), FWD_LABEL_TEXT);
	gtk_label_set_text(GTK_LABEL(label_rev), REV_LABEL_TEXT);
}


/****************************************************************************/
gint rtp_stream_info_cmp_reverse(gconstpointer aa, gconstpointer bb)
{
	const struct _rtp_stream_info* a = aa;
	const struct _rtp_stream_info* b = bb;

	if (a==NULL || b==NULL)
		return 1;
	if ((a->src_addr == b->dest_addr)
		&& (a->src_port == b->dest_port)
		&& (a->dest_addr == b->src_addr)
		&& (a->dest_port == b->src_port))
		return 0;
	else
		return 1;
}

/****************************************************************************/
static void
rtpstream_on_findrev		       (GtkButton	*button _U_,
					gpointer	 user_data _U_)
{
	gint row;
	gint start_row;
	rtp_stream_info_t* pstream = NULL;

	if (selected_stream_fwd==NULL)
		return;
	if (selected_stream_rev==NULL) {
		pstream = selected_stream_fwd;
	}
	else {
		pstream = selected_stream_rev;
	}

	start_row = gtk_clist_find_row_from_data(GTK_CLIST(clist), pstream);
	row = start_row+1;

	for (row=start_row+1;
		(pstream = gtk_clist_get_row_data(GTK_CLIST(clist), row));
		row++) {
		if (rtp_stream_info_cmp_reverse(selected_stream_fwd, pstream) == 0) {
			gtk_clist_select_row(GTK_CLIST(clist), row, 0);
			gtk_clist_moveto(GTK_CLIST(clist), row, 0, 0.5, 0);
			return;
		}
	}

	/* wrap around */
	for (row=0;
		(pstream = gtk_clist_get_row_data(GTK_CLIST(clist), row)) && row<start_row;
		row++) {
		if (rtp_stream_info_cmp_reverse(selected_stream_fwd, pstream) == 0) {
			gtk_clist_select_row(GTK_CLIST(clist), row, 0);
			gtk_clist_moveto(GTK_CLIST(clist), row, 0, 0.5, 0);
			return;
		}
	}

	/* if we didnt find another stream, highlight the current reverse stream */
	if (selected_stream_rev!=NULL) {
		gtk_clist_select_row(GTK_CLIST(clist), row, 0);
		gtk_clist_moveto(GTK_CLIST(clist), row, 0, 0.5, 0);
	}
}


/****************************************************************************/
/*
static void
rtpstream_on_goto                      (GtkButton       *button _U_,
                                        gpointer         user_data _U_)
{
	if (selected_stream_fwd)
	{
		goto_frame(&cfile, selected_stream_fwd->first_frame_num);
	}
}
*/


/****************************************************************************/
static void
rtpstream_on_save                      (GtkButton       *button _U_,
                                        gpointer         data _U_)
{
	rtpstream_tapinfo_t* tapinfo = data;

	GtkWidget *vertb;
	GtkWidget *ok_bt;

	if (!selected_stream_fwd)
		return;

	if (rtpstream_save_dlg != NULL) {
		/* There's already a Save dialog box; reactivate it. */
		reactivate_window(rtpstream_save_dlg);
		return;
	}

	/* XXX - use file_selection from dlg_utils instead! */
	rtpstream_save_dlg = gtk_file_selection_new("Ethereal: Save selected stream in rtpdump ('-F dump') format");

	/* Container for each row of widgets */
	vertb = gtk_vbox_new(FALSE, 0);
	gtk_container_border_width(GTK_CONTAINER(vertb), 5);
	gtk_box_pack_start(GTK_BOX(GTK_FILE_SELECTION(rtpstream_save_dlg)->action_area),
		vertb, FALSE, FALSE, 0);
	gtk_widget_show (vertb);

	ok_bt = GTK_FILE_SELECTION(rtpstream_save_dlg)->ok_button;
	SIGNAL_CONNECT(ok_bt, "clicked", save_stream_ok_cb, tapinfo);

	window_set_cancel_button(rtpstream_save_dlg,
	    GTK_FILE_SELECTION(rtpstream_save_dlg)->cancel_button, window_cancel_button_cb);

	SIGNAL_CONNECT(rtpstream_save_dlg, "delete_event", window_delete_event_cb, NULL);
	SIGNAL_CONNECT(rtpstream_save_dlg, "destroy", save_stream_destroy_cb,
	               NULL);

	gtk_widget_show(rtpstream_save_dlg);
	window_present(rtpstream_save_dlg);
}


/****************************************************************************/
static void
rtpstream_on_mark                      (GtkButton       *button _U_,
                                        gpointer         user_data _U_)
{
	if (selected_stream_fwd==NULL && selected_stream_rev==NULL)
		return;
	rtpstream_mark(selected_stream_fwd, selected_stream_rev);
}


/****************************************************************************/
static void
rtpstream_on_filter                    (GtkButton       *button _U_,
                                        gpointer         user_data _U_)
{
	gchar *filter_string = NULL;
	gchar *filter_string_fwd = NULL;
	gchar *filter_string_rev = NULL;

	if (selected_stream_fwd==NULL && selected_stream_rev==NULL)
		return;

	if (selected_stream_fwd)
	{
		filter_string_fwd = g_strdup_printf(
			"(ip.src==%s && udp.srcport==%u && ip.dst==%s && udp.dstport==%u && rtp.ssrc==%u)",
			ip_to_str((const guint8*)&(selected_stream_fwd->src_addr)),
			selected_stream_fwd->src_port,
			ip_to_str((const guint8*)&(selected_stream_fwd->dest_addr)),
			selected_stream_fwd->dest_port,
			selected_stream_fwd->ssrc);
        filter_string = filter_string_fwd;
	}

	if (selected_stream_rev)
	{
		filter_string_rev = g_strdup_printf(
			"(ip.src==%s && udp.srcport==%u && ip.dst==%s && udp.dstport==%u && rtp.ssrc==%u)",
			ip_to_str((const guint8*)&(selected_stream_rev->src_addr)),
			selected_stream_rev->src_port,
			ip_to_str((const guint8*)&(selected_stream_rev->dest_addr)),
			selected_stream_rev->dest_port,
			selected_stream_rev->ssrc);
        filter_string = filter_string_rev;

	    if (selected_stream_fwd)
	    {
            filter_string = g_strdup_printf("%s || %s", filter_string, filter_string_rev);
            g_free(filter_string_fwd);
            g_free(filter_string_rev);
        }
    }

    gtk_entry_set_text(GTK_ENTRY(main_display_filter_widget), filter_string);
    g_free(filter_string);

/*
	main_filter_packets(&cfile, filter_string, FALSE);
	rtpstream_dlg_update(rtpstream_get_info()->strinfo_list);
*/
}


/****************************************************************************/
static void
rtpstream_on_analyse                   (GtkButton       *button _U_,
                                        gpointer         user_data _U_)
{
	guint32 ip_src_fwd = 0;
	guint16 port_src_fwd = 0;
	guint32 ip_dst_fwd = 0;
	guint16 port_dst_fwd = 0;
	guint32 ssrc_fwd = 0;
	guint32 ip_src_rev = 0;
	guint16 port_src_rev = 0;
	guint32 ip_dst_rev = 0;
	guint16 port_dst_rev = 0;
	guint32 ssrc_rev = 0;

	if (selected_stream_fwd) {
		ip_src_fwd = selected_stream_fwd->src_addr;
		port_src_fwd = selected_stream_fwd->src_port;
		ip_dst_fwd = selected_stream_fwd->dest_addr;
		port_dst_fwd = selected_stream_fwd->dest_port;
		ssrc_fwd = selected_stream_fwd->ssrc;
	}

	if (selected_stream_rev) {
		ip_src_rev = selected_stream_rev->src_addr;
		port_src_rev = selected_stream_rev->src_port;
		ip_dst_rev = selected_stream_rev->dest_addr;
		port_dst_rev = selected_stream_rev->dest_port;
		ssrc_rev = selected_stream_rev->ssrc;
	}

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


/****************************************************************************/
/* when the user selects a row in the stream list */
static void
rtpstream_on_select_row(GtkCList *clist,
                                            gint row _U_,
                                            gint column _U_,
                                            GdkEventButton *event _U_,
                                            gpointer user_data _U_)
{
	gchar label_text[80];

	/* update the labels */
	if (event==NULL || event->state & GDK_SHIFT_MASK) {
		selected_stream_rev = gtk_clist_get_row_data(GTK_CLIST(clist), row);
		g_snprintf(label_text, 80, "Reverse: %s:%u -> %s:%u, SSRC=%u",
			ip_to_str((ip_addr_p)&selected_stream_rev->src_addr),
			selected_stream_rev->src_port,
			ip_to_str((ip_addr_p)&selected_stream_rev->dest_addr),
			selected_stream_rev->dest_port,
			selected_stream_rev->ssrc
		);
		gtk_label_set_text(GTK_LABEL(label_rev), label_text);
	}
	else {
		selected_stream_fwd = gtk_clist_get_row_data(GTK_CLIST(clist), row);
		g_snprintf(label_text, 80, "Forward: %s:%u -> %s:%u, SSRC=%u",
			ip_to_str((ip_addr_p)&selected_stream_fwd->src_addr),
			selected_stream_fwd->src_port,
			ip_to_str((ip_addr_p)&selected_stream_fwd->dest_addr),
			selected_stream_fwd->dest_port,
			selected_stream_fwd->ssrc
		);
		gtk_label_set_text(GTK_LABEL(label_fwd), label_text);
	}

/*
	gtk_widget_set_sensitive(save_bt, TRUE);
	gtk_widget_set_sensitive(filter_bt, TRUE);
	gtk_widget_set_sensitive(mark_bt, TRUE);
*/
	/* TODO: activate other buttons when implemented */
}


/****************************************************************************/
#define NUM_COLS 7

typedef struct column_arrows {
	GtkWidget *table;
	GtkWidget *ascend_pm;
	GtkWidget *descend_pm;
} column_arrows;


/****************************************************************************/
static void
rtpstream_click_column_cb(GtkCList *clist, gint column, gpointer data)
{
	column_arrows *col_arrows = (column_arrows *) data;
	int i;

	gtk_clist_freeze(clist);

	for (i=0; i<NUM_COLS; i++) {
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
static gint
rtpstream_sort_column(GtkCList *clist, gconstpointer ptr1, gconstpointer ptr2)
{
	char *text1 = NULL;
	char *text2 = NULL;
	int i1, i2;

	const GtkCListRow *row1 = (const GtkCListRow *) ptr1;
	const GtkCListRow *row2 = (const GtkCListRow *) ptr2;

	text1 = GTK_CELL_TEXT (row1->cell[clist->sort_column])->text;
	text2 = GTK_CELL_TEXT (row2->cell[clist->sort_column])->text;

	switch(clist->sort_column){
	case 0:
	case 2:
	case 5:
	case 7:
		return strcmp (text1, text2);
	case 1:
	case 3:
	case 4:
	case 6:
		i1=atoi(text1);
		i2=atoi(text2);
		return i1-i2;
	}
	g_assert_not_reached();
	return 0;
}


/****************************************************************************/
/* INTERFACE                                                                */
/****************************************************************************/

static void rtpstream_dlg_create (void)
{
	GtkWidget *rtpstream_dlg_w;
	GtkWidget *main_vb;
	GtkWidget *scrolledwindow;
	GtkWidget *hbuttonbox;
/*	GtkWidget *bt_goto;*/
	GtkWidget *bt_unselect;
	GtkWidget *bt_findrev;
	GtkWidget *bt_save;
	GtkWidget *bt_frames;
	GtkWidget *bt_filter;
	GtkWidget *bt_analyse;
	GtkWidget *bt_close;

	gchar *titles[8] =  {"Src IP addr", "Src port",  "Dest IP addr", "Dest port", "SSRC", "Payload", "Packets", "Comment"};
	column_arrows *col_arrows;
	GtkWidget *column_lb;
	int i;

	rtpstream_dlg_w = dlg_window_new("Ethereal: RTP Streams");
	gtk_window_set_default_size(GTK_WINDOW(rtpstream_dlg_w), 620, 200);

	main_vb = gtk_vbox_new (FALSE, 0);
	gtk_container_add(GTK_CONTAINER(rtpstream_dlg_w), main_vb);
	gtk_container_set_border_width (GTK_CONTAINER (main_vb), 12);

	top_label = gtk_label_new ("Detected 0 RTP streams. Choose one for forward and reverse direction for analysis");
	gtk_box_pack_start (GTK_BOX (main_vb), top_label, FALSE, FALSE, 8);

	scrolledwindow = scrolled_window_new (NULL, NULL);
	gtk_box_pack_start (GTK_BOX (main_vb), scrolledwindow, TRUE, TRUE, 0);

	clist = gtk_clist_new (NUM_COLS);
	gtk_container_add (GTK_CONTAINER (scrolledwindow), clist);

	gtk_clist_set_column_width (GTK_CLIST (clist), 0, 100);
	gtk_clist_set_column_width (GTK_CLIST (clist), 1, 50);
	gtk_clist_set_column_width (GTK_CLIST (clist), 2, 100);
	gtk_clist_set_column_width (GTK_CLIST (clist), 3, 50);
	gtk_clist_set_column_width (GTK_CLIST (clist), 4, 80);
	gtk_clist_set_column_width (GTK_CLIST (clist), 5, 118);
	gtk_clist_set_column_width (GTK_CLIST (clist), 6, 40);
/*	gtk_clist_set_column_width (GTK_CLIST (clist), 7, 51);*/

	gtk_clist_set_column_justification(GTK_CLIST(clist), 0, GTK_JUSTIFY_CENTER);
	gtk_clist_set_column_justification(GTK_CLIST(clist), 1, GTK_JUSTIFY_CENTER);
	gtk_clist_set_column_justification(GTK_CLIST(clist), 2, GTK_JUSTIFY_CENTER);
	gtk_clist_set_column_justification(GTK_CLIST(clist), 3, GTK_JUSTIFY_CENTER);
	gtk_clist_set_column_justification(GTK_CLIST(clist), 4, GTK_JUSTIFY_CENTER);
	gtk_clist_set_column_justification(GTK_CLIST(clist), 5, GTK_JUSTIFY_LEFT);
	gtk_clist_set_column_justification(GTK_CLIST(clist), 6, GTK_JUSTIFY_RIGHT);
/*	gtk_clist_set_column_justification(GTK_CLIST(clist), 7, GTK_JUSTIFY_CENTER);*/

	gtk_clist_column_titles_show (GTK_CLIST (clist));

	gtk_clist_set_compare_func(GTK_CLIST(clist), rtpstream_sort_column);
	gtk_clist_set_sort_column(GTK_CLIST(clist), 0);
	gtk_clist_set_sort_type(GTK_CLIST(clist), GTK_SORT_ASCENDING);

	gtk_widget_show(rtpstream_dlg_w);

	/* sort by column feature */
	col_arrows = (column_arrows *) g_malloc(sizeof(column_arrows) * NUM_COLS);

	for (i=0; i<NUM_COLS; i++) {
		col_arrows[i].table = gtk_table_new(2, 2, FALSE);
		gtk_table_set_col_spacings(GTK_TABLE(col_arrows[i].table), 5);
		column_lb = gtk_label_new(titles[i]);
		gtk_table_attach(GTK_TABLE(col_arrows[i].table), column_lb, 0, 1, 0, 2, GTK_SHRINK, GTK_SHRINK, 0, 0);
		gtk_widget_show(column_lb);

		col_arrows[i].ascend_pm = xpm_to_widget(clist_ascend_xpm);
		gtk_table_attach(GTK_TABLE(col_arrows[i].table), col_arrows[i].ascend_pm, 1, 2, 1, 2, GTK_SHRINK, GTK_SHRINK, 0, 0);
		col_arrows[i].descend_pm = xpm_to_widget(clist_descend_xpm);
		gtk_table_attach(GTK_TABLE(col_arrows[i].table), col_arrows[i].descend_pm, 1, 2, 0, 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
		/* make src-ip be the default sort order */
		if (i == 0) {
			gtk_widget_show(col_arrows[i].ascend_pm);
		}
		gtk_clist_set_column_widget(GTK_CLIST(clist), i, col_arrows[i].table);
		gtk_widget_show(col_arrows[i].table);
	}

	SIGNAL_CONNECT(clist, "click-column", rtpstream_click_column_cb, col_arrows);

	label_fwd = gtk_label_new (FWD_LABEL_TEXT);
	gtk_box_pack_start (GTK_BOX (main_vb), label_fwd, FALSE, FALSE, 0);

	label_rev = gtk_label_new (REV_LABEL_TEXT);
	gtk_box_pack_start (GTK_BOX (main_vb), label_rev, FALSE, FALSE, 0);

    /* button row */
	hbuttonbox = gtk_hbutton_box_new ();
	gtk_box_pack_start (GTK_BOX (main_vb), hbuttonbox, FALSE, FALSE, 0);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (hbuttonbox), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing (GTK_BUTTON_BOX (hbuttonbox), 0);

	bt_unselect = gtk_button_new_with_label ("Unselect");
	gtk_container_add (GTK_CONTAINER (hbuttonbox), bt_unselect);

	bt_findrev = gtk_button_new_with_label ("Find Reverse");
	gtk_container_add (GTK_CONTAINER (hbuttonbox), bt_findrev);
/*
	bt_goto = BUTTON_NEW_FROM_STOCK(GTK_STOCK_JUMP_TO);
	gtk_container_add (GTK_CONTAINER (hbuttonbox), bt_goto);
*/
	bt_save = BUTTON_NEW_FROM_STOCK(GTK_STOCK_SAVE_AS);
	gtk_container_add (GTK_CONTAINER (hbuttonbox), bt_save);

	bt_frames = gtk_button_new_with_label ("Mark frames");
	gtk_container_add (GTK_CONTAINER (hbuttonbox), bt_frames);

	bt_filter = gtk_button_new_with_label ("Set filter");
	gtk_container_add (GTK_CONTAINER (hbuttonbox), bt_filter);

	bt_analyse = gtk_button_new_with_label ("Analyse");
	gtk_container_add (GTK_CONTAINER (hbuttonbox), bt_analyse);

	bt_close = BUTTON_NEW_FROM_STOCK(GTK_STOCK_CLOSE);
	gtk_container_add (GTK_CONTAINER (hbuttonbox), bt_close);
	GTK_WIDGET_SET_FLAGS(bt_close, GTK_CAN_DEFAULT);

	SIGNAL_CONNECT(clist, "select_row", rtpstream_on_select_row, NULL);
	SIGNAL_CONNECT(bt_unselect, "clicked", rtpstream_on_unselect, NULL);
	SIGNAL_CONNECT(bt_findrev, "clicked", rtpstream_on_findrev, NULL);
/*
	SIGNAL_CONNECT(bt_goto, "clicked", rtpstream_on_goto, NULL);
*/
	SIGNAL_CONNECT(bt_save, "clicked", rtpstream_on_save, NULL);
	SIGNAL_CONNECT(bt_frames, "clicked", rtpstream_on_mark, NULL);
	SIGNAL_CONNECT(bt_filter, "clicked", rtpstream_on_filter, NULL);
	SIGNAL_CONNECT(bt_analyse, "clicked", rtpstream_on_analyse, NULL);

	window_set_cancel_button(rtpstream_dlg_w, bt_close, window_cancel_button_cb);

	SIGNAL_CONNECT(rtpstream_dlg_w, "delete_event", window_delete_event_cb, NULL);
	SIGNAL_CONNECT(rtpstream_dlg_w, "destroy", rtpstream_on_destroy, NULL);

	gtk_widget_show_all(rtpstream_dlg_w);
	window_present(rtpstream_dlg_w);

	rtpstream_on_unselect(NULL, NULL);

	rtp_stream_dlg = rtpstream_dlg_w;
}


/****************************************************************************/
/* PUBLIC								    */
/****************************************************************************/

/****************************************************************************/
/* update the contents of the dialog box clist */
/* list: pointer to list of rtp_stream_info_t* */
void rtpstream_dlg_update(GList *list)
{
	if (rtp_stream_dlg != NULL) {
		gtk_clist_clear(GTK_CLIST(clist));
		streams_nb = 0;

		list = g_list_first(list);
		while (list)
		{
			add_to_clist((rtp_stream_info_t*)(list->data));
			list = g_list_next(list);
		}

		rtpstream_on_unselect(NULL, NULL);
	}

	last_list = list;
}


/****************************************************************************/
/* update the contents of the dialog box clist */
/* list: pointer to list of rtp_stream_info_t* */
void rtpstream_dlg_show(GList *list)
{
	if (rtp_stream_dlg != NULL) {
		/* There's already a dialog box; reactivate it. */
		reactivate_window(rtp_stream_dlg);
		/* Another list since last call? */
		if (list != last_list) {
			rtpstream_dlg_update(list);
		}
	}
	else {
		/* Create and show the dialog box */
		rtpstream_dlg_create();
		rtpstream_dlg_update(list);
	}
}


/****************************************************************************/
/* entry point when called via the GTK menu */
void rtpstream_launch(GtkWidget *w _U_, gpointer data _U_)
{
	/* Register the tap listener */
	register_tap_listener_rtp_stream();

	/* Scan for RTP streams (redissect all packets) */
	rtpstream_scan();

	/* Show the dialog box with the list of streams */
	rtpstream_dlg_show(rtpstream_get_info()->strinfo_list);

	/* Tap listener will be removed and cleaned up in rtpstream_on_destroy */
}

/****************************************************************************/
void
register_tap_listener_rtp_stream_dlg(void)
{
	register_tap_menu_item("RTP/Show All Streams...", REGISTER_TAP_GROUP_NONE,
	    rtpstream_launch, NULL, NULL, NULL);
}
