/* fc_stat.c
 * fc_stat   2003 Ronnie Sahlberg
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
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#include <string.h>

#include <gtk/gtk.h>

#include <epan/packet_info.h>
#include <epan/epan.h>
#include <epan/value_string.h>

#include "tap_menu.h"
#include <epan/tap.h>
#include <epan/dissectors/packet-fc.h>
#include "../register.h"
#include "../timestats.h"
#include "compat_macros.h"
#include "../simple_dialog.h"
#include "ui_util.h"
#include "dlg_utils.h"
#include "../file.h"
#include "../globals.h"
#include "filter_dlg.h"
#include "service_response_time_table.h"
#include "gtkglobals.h"


/* used to keep track of the statistics for an entire program interface */
typedef struct _fcstat_t {
	GtkWidget *win;
	srt_stat_table fc_srt_table;
} fcstat_t;

static void
fcstat_set_title(fcstat_t *fc)
{
	char		*title;

	title = g_strdup_printf("Fibre Channel Service Response Time statistics: %s",
	    cf_get_display_name(&cfile));
	gtk_window_set_title(GTK_WINDOW(fc->win), title);
	g_free(title);
}

static void
fcstat_reset(void *pfc)
{
	fcstat_t *fc=(fcstat_t *)pfc;

	reset_srt_table_data(&fc->fc_srt_table);
	fcstat_set_title(fc);
}

static int
fcstat_packet(void *pfc, packet_info *pinfo, epan_dissect_t *edt _U_, const void *psi)
{
	const fc_hdr *fc=psi;
	fcstat_t *fs=(fcstat_t *)pfc;

	/* we are only interested in reply packets */
	if(!(fc->fctl&FC_FCTL_EXCHANGE_RESPONDER)){
		return 0;
	}
	/* if we havnt seen the request, just ignore it */
	if( (!fc->fced) || (fc->fced->first_exchange_frame==0) ){
		return 0;
	}

	add_srt_table_data(&fs->fc_srt_table, fc->type, &fc->fced->fc_time, pinfo);

	return 1;
}



static void
fcstat_draw(void *pfc)
{
	fcstat_t *fc=(fcstat_t *)pfc;

	draw_srt_table_data(&fc->fc_srt_table);
}


void protect_thread_critical_region(void);
void unprotect_thread_critical_region(void);
static void
win_destroy_cb(GtkWindow *win _U_, gpointer data)
{
	fcstat_t *fc=(fcstat_t *)data;

	protect_thread_critical_region();
	remove_tap_listener(fc);
	unprotect_thread_critical_region();

	free_srt_table_data(&fc->fc_srt_table);
	g_free(fc);
}


static void
gtk_fcstat_init(char *optarg)
{
	fcstat_t *fc;
	char *filter=NULL;
	GtkWidget *label;
	char filter_string[256];
	GString *error_string;
	int i;
	GtkWidget *vbox;
    GtkWidget *bbox;
    GtkWidget *close_bt;

	if(!strncmp(optarg,"fc,srt,",7)){
		filter=optarg+7;
	} else {
		filter=NULL;
	}

	fc=g_malloc(sizeof(fcstat_t));

	fc->win=window_new(GTK_WINDOW_TOPLEVEL, "fc-stat");
	gtk_window_set_default_size(GTK_WINDOW(fc->win), 550, 400);
	fcstat_set_title(fc);

	vbox=gtk_vbox_new(FALSE, 3);
	gtk_container_add(GTK_CONTAINER(fc->win), vbox);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);

	label=gtk_label_new("Fibre Channel Service Response Time statistics");
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	g_snprintf(filter_string,255,"Filter:%s",filter?filter:"");
	label=gtk_label_new(filter_string);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	label=gtk_label_new("Fibre Channel Types");
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	/* We must display TOP LEVEL Widget before calling init_srt_table() */
	gtk_widget_show_all(fc->win);

	init_srt_table(&fc->fc_srt_table, 256, vbox, NULL);
	for(i=0;i<256;i++){
		init_srt_table_row(&fc->fc_srt_table, i, val_to_str(i, fc_fc4_val, "Unknown(0x%02x)"));
	}


	error_string=register_tap_listener("fc", fc, filter, fcstat_reset, fcstat_packet, fcstat_draw);
	if(error_string){
		simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK, error_string->str);
		g_string_free(error_string, TRUE);
		g_free(fc);
		return;
	}

	/* Button row. */
	bbox = dlg_button_row_new(GTK_STOCK_CLOSE, NULL);
	gtk_box_pack_end(GTK_BOX(vbox), bbox, FALSE, FALSE, 0);

	close_bt = OBJECT_GET_DATA(bbox, GTK_STOCK_CLOSE);
    window_set_cancel_button(fc->win, close_bt, window_cancel_button_cb);

    SIGNAL_CONNECT(fc->win, "delete_event", window_delete_event_cb, NULL);
    SIGNAL_CONNECT(fc->win, "destroy", win_destroy_cb, fc);

	gtk_widget_show_all(fc->win);
    window_present(fc->win);

    retap_packets(&cfile);
}



static GtkWidget *dlg=NULL;
static GtkWidget *filter_entry;

static void
dlg_destroy_cb(void)
{
	dlg=NULL;
}

static void
fcstat_start_button_clicked(GtkWidget *item _U_, gpointer data _U_)
{
	GString *str;
	const char *filter;

	str = g_string_new("fc,srt");
	filter=gtk_entry_get_text(GTK_ENTRY(filter_entry));
	if(filter[0]!=0){
		g_string_sprintfa(str,",%s", filter);
	}
	gtk_fcstat_init(str->str);
	g_string_free(str, TRUE);
}

static void
gtk_fcstat_cb(GtkWidget *w _U_, gpointer d _U_)
{
	GtkWidget *dlg_box;
	GtkWidget *filter_box, *filter_bt;
	GtkWidget *bbox, *start_button, *cancel_button;
	const char *filter;
	static construct_args_t args = {
	  "Service Response Time Statistics Filter",
	  TRUE,
	  FALSE
	};

	/* if the window is already open, bring it to front */
	if(dlg){
		gdk_window_raise(dlg->window);
		return;
	}

	dlg=dlg_window_new("Ethereal: Compute Fibre Channel Service Response Time statistics");
	gtk_window_set_default_size(GTK_WINDOW(dlg), 400, -1);

	dlg_box=gtk_vbox_new(FALSE, 10);
	gtk_container_border_width(GTK_CONTAINER(dlg_box), 10);
	gtk_container_add(GTK_CONTAINER(dlg), dlg_box);
	gtk_widget_show(dlg_box);

	/* Filter box */
	filter_box=gtk_hbox_new(FALSE, 3);

	/* Filter button */
	filter_bt=BUTTON_NEW_FROM_STOCK(ETHEREAL_STOCK_DISPLAY_FILTER_ENTRY);
	SIGNAL_CONNECT(filter_bt, "clicked", display_filter_construct_cb, &args);
	gtk_box_pack_start(GTK_BOX(filter_box), filter_bt, FALSE, FALSE, 0);
	gtk_widget_show(filter_bt);

	/* Filter entry */
	filter_entry=gtk_entry_new();
    SIGNAL_CONNECT(filter_entry, "changed", filter_te_syntax_check_cb, NULL);

	/* filter prefs dialog */
	OBJECT_SET_DATA(filter_bt, E_FILT_TE_PTR_KEY, filter_entry);
	/* filter prefs dialog */

	gtk_box_pack_start(GTK_BOX(filter_box), filter_entry, TRUE, TRUE, 0);
	filter=gtk_entry_get_text(GTK_ENTRY(main_display_filter_widget));
	if(filter){
		gtk_entry_set_text(GTK_ENTRY(filter_entry), filter);
	}
	gtk_widget_show(filter_entry);

	gtk_box_pack_start(GTK_BOX(dlg_box), filter_box, TRUE, TRUE, 0);
	gtk_widget_show(filter_box);

	/* button box */
    bbox = dlg_button_row_new(ETHEREAL_STOCK_CREATE_STAT, GTK_STOCK_CANCEL, NULL);
	gtk_box_pack_start(GTK_BOX(dlg_box), bbox, FALSE, FALSE, 0);
    gtk_widget_show(bbox);

    start_button = OBJECT_GET_DATA(bbox, ETHEREAL_STOCK_CREATE_STAT);
    gtk_widget_grab_default(start_button );
    SIGNAL_CONNECT_OBJECT(start_button, "clicked",
                              fcstat_start_button_clicked, NULL);

    cancel_button = OBJECT_GET_DATA(bbox, GTK_STOCK_CANCEL);
    window_set_cancel_button(dlg, cancel_button, window_cancel_button_cb);

	/* Catch the "activate" signal on the filter text entry, so that
	   if the user types Return there, we act as if the "Create Stat"
	   button had been selected, as happens if Return is typed if some
	   widget that *doesn't* handle the Return key has the input
	   focus. */
	dlg_set_activate(filter_entry, start_button);

	/* Give the initial focus to the "Filter" entry box. */
	gtk_widget_grab_focus(filter_entry);

    SIGNAL_CONNECT(dlg, "delete_event", window_delete_event_cb, NULL);
	SIGNAL_CONNECT(dlg, "destroy", dlg_destroy_cb, NULL);

	gtk_widget_show_all(dlg);
    window_present(dlg);
}

void
register_tap_listener_gtkfcstat(void)
{
	register_ethereal_tap("fc,srt", gtk_fcstat_init);

	register_tap_menu_item("Fibre Channel...", REGISTER_TAP_GROUP_RESPONSE_TIME,
	    gtk_fcstat_cb, NULL, NULL, NULL);
}
