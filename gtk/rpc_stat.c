/* rpc_stat.c
 * rpc_stat   2002 Ronnie Sahlberg
 *
 * $Id: rpc_stat.c,v 1.14 2003/08/25 11:06:32 sahlberg Exp $
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

/* This module provides rpc call/reply SRT (Server Response Time) statistics 
 * to ethereal.
 *
 * It serves as an example on how to use the tap api.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <gtk/gtk.h>
#include "menu.h"
#include "epan/packet_info.h"
#include "simple_dialog.h"
#include "tap.h"
#include "../register.h"
#include "packet-rpc.h"
#include "../globals.h"
#include "compat_macros.h"
#include "service_response_time_table.h"

extern GtkWidget   *main_display_filter_widget;

/* used to keep track of the statistics for an entire program interface */
typedef struct _rpcstat_t {
	GtkWidget *win;
	srt_stat_table srt_table;
	char *prog;
	guint32 program;
	guint32 version;
	guint32 num_procedures;
} rpcstat_t;




static void
rpcstat_reset(rpcstat_t *rs)
{
	char title_string[256];

	reset_srt_table_data(&rs->srt_table);

	snprintf(title_string, 256, "ONC-RPC Service Response Time statistics for %s version %d : %s", rs->prog, rs->version, cfile.filename);
	gtk_window_set_title(GTK_WINDOW(rs->win), title_string);
}


static int
rpcstat_packet(rpcstat_t *rs, packet_info *pinfo, epan_dissect_t *edt _U_, rpc_call_info_value *ri)
{
	if(ri->proc>=rs->num_procedures){
		/* dont handle this since its outside of known table */
		return 0;
	}
	/* we are only interested in reply packets */
	if(ri->request){
		return 0;
	}
	/* we are only interested in certain program/versions */
	if( (ri->prog!=rs->program) || (ri->vers!=rs->version) ){
		return 0;
	}

	add_srt_table_data(&rs->srt_table, ri->proc, &ri->req_time, pinfo);

	return 1;
}

static void
rpcstat_draw(rpcstat_t *rs)
{
	draw_srt_table_data(&rs->srt_table);
}



static guint32 rpc_program=0;
static guint32 rpc_version=0;
static gint32 rpc_min_vers=-1;
static gint32 rpc_max_vers=-1;
static gint32 rpc_min_proc=-1;
static gint32 rpc_max_proc=-1;

static void *
rpcstat_find_procs(gpointer *key, gpointer *value _U_, gpointer *user_data _U_)
{
	rpc_proc_info_key *k=(rpc_proc_info_key *)key;

	if(k->prog!=rpc_program){
		return NULL;
	}
	if(k->vers!=rpc_version){
		return NULL;
	}
	if(rpc_min_proc==-1){
		rpc_min_proc=k->proc;
		rpc_max_proc=k->proc;
	}
	if((gint32)k->proc<rpc_min_proc){
		rpc_min_proc=k->proc;
	}
	if((gint32)k->proc>rpc_max_proc){
		rpc_max_proc=k->proc;
	}

	return NULL;
}

static void *
rpcstat_find_vers(gpointer *key, gpointer *value _U_, gpointer *user_data _U_)
{
	rpc_proc_info_key *k=(rpc_proc_info_key *)key;

	if(k->prog!=rpc_program){
		return NULL;
	}
	if(rpc_min_vers==-1){
		rpc_min_vers=k->vers;
		rpc_max_vers=k->vers;
	}
	if((gint32)k->vers<rpc_min_vers){
		rpc_min_vers=k->vers;
	}
	if((gint32)k->vers>rpc_max_vers){
		rpc_max_vers=k->vers;
	}

	return NULL;
}

/* since the gtk2 implementation of tap is multithreaded we must protect
 * remove_tap_listener() from modifying the list while draw_tap_listener()
 * is running.  the other protected block is in main.c
 *
 * there should not be any other critical regions in gtk2
 */
void protect_thread_critical_region(void);
void unprotect_thread_critical_region(void);
static void
win_destroy_cb(GtkWindow *win _U_, gpointer data)
{
	rpcstat_t *rs=(rpcstat_t *)data;

	protect_thread_critical_region();
	remove_tap_listener(rs);
	unprotect_thread_critical_region();

	free_srt_table_data(&rs->srt_table);
	g_free(rs);
}

/* When called, this function will create a new instance of gtk2-rpcstat.
 */
static void
gtk_rpcstat_init(char *optarg)
{
	rpcstat_t *rs;
	guint32 i;
	char title_string[256];
	char filter_string[256];
	GtkWidget *vbox;
	GtkWidget *stat_label;
	GtkWidget *filter_label;
	int program, version, pos;
	char *filter=NULL;
	GString *error_string;

	pos=0;
	if(sscanf(optarg,"rpc,srt,%d,%d,%n",&program,&version,&pos)==2){
		if(pos){
			filter=optarg+pos;
		} else {
			filter=NULL;
		}
	} else {
		fprintf(stderr, "ethereal: invalid \"-z rpc,srt,<program>,<version>[,<filter>]\" argument\n");
		exit(1);
	}

	rpc_program=program;
	rpc_version=version;
	rs=g_malloc(sizeof(rpcstat_t));
	rs->prog=rpc_prog_name(rpc_program);
	rs->program=rpc_program;
	rs->version=rpc_version;

	rs->win=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(rs->win), 550, 400);
	snprintf(title_string, 256, "ONC-RPC Service Response Time statistics for %s version %d : %s", rs->prog, rs->version, cfile.filename);
	gtk_window_set_title(GTK_WINDOW(rs->win), title_string);
	SIGNAL_CONNECT(rs->win, "destroy", win_destroy_cb, rs);

	vbox=gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(rs->win), vbox);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
	gtk_widget_show(vbox);

	stat_label=gtk_label_new(title_string);
	gtk_box_pack_start(GTK_BOX(vbox), stat_label, FALSE, FALSE, 0);
	gtk_widget_show(stat_label);

	snprintf(filter_string,255,"Filter:%s",filter?filter:"");
	filter_label=gtk_label_new(filter_string);
	gtk_box_pack_start(GTK_BOX(vbox), filter_label, FALSE, FALSE, 0);
	gtk_widget_show(filter_label);

	rpc_min_proc=-1;
	rpc_max_proc=-1;
	g_hash_table_foreach(rpc_procs, (GHFunc)rpcstat_find_procs, NULL);
	rs->num_procedures=rpc_max_proc+1;

	/* We must display TOP LEVEL Widget before calling init_srt_table() */
	gtk_widget_show(rs->win);

	init_srt_table(&rs->srt_table, rpc_max_proc+1, vbox);

	for(i=0;i<rs->num_procedures;i++){
		init_srt_table_row(&rs->srt_table, i, rpc_proc_name(rpc_program, rpc_version, i));
	}


	error_string=register_tap_listener("rpc", rs, filter, (void*)rpcstat_reset, (void*)rpcstat_packet, (void*)rpcstat_draw);
	if(error_string){
		simple_dialog(ESD_TYPE_WARN, NULL, error_string->str);
		g_string_free(error_string, TRUE);
		free_srt_table_data(&rs->srt_table);
		g_free(rs);
		return;
	}


	gtk_widget_show_all(rs->win);
	redissect_packets(&cfile);
}




static GtkWidget *dlg=NULL, *dlg_box;
static GtkWidget *prog_box;
static GtkWidget *prog_label, *prog_opt, *prog_menu;
static GtkWidget *vers_label, *vers_opt, *vers_menu;
static GtkWidget *filter_box;
static GtkWidget *filter_label, *filter_entry;
static GtkWidget *start_button;


static void
rpcstat_start_button_clicked(GtkWidget *item _U_, gpointer data _U_)
{
	char *filter;
	char str[256];

	filter=(char *)gtk_entry_get_text(GTK_ENTRY(filter_entry));
	if(filter[0]==0){
		sprintf(str, "rpc,srt,%d,%d", rpc_program, rpc_version);
		filter="";
	} else {
		sprintf(str, "rpc,srt,%d,%d,%s", rpc_program, rpc_version, filter);
	}
	gtk_rpcstat_init(str);
}


static void
rpcstat_version_select(GtkWidget *item _U_, gpointer key)
{
	int vers=(int)key;

	rpc_version=vers;
}



static void
rpcstat_program_select(GtkWidget *item _U_, gpointer key)
{
	rpc_prog_info_key *k=(rpc_prog_info_key *)key;
	int i;

	rpc_program=k->prog;

	/* change version menu */
	rpc_version=0;
	gtk_object_destroy(GTK_OBJECT(vers_menu));
	vers_menu=gtk_menu_new();
	rpc_min_vers=-1;
	rpc_max_vers=-1;
	g_hash_table_foreach(rpc_procs, (GHFunc)rpcstat_find_vers, NULL);
	rpc_version=rpc_min_vers;
	for(i=rpc_min_vers;i<=rpc_max_vers;i++){
		GtkWidget *menu_item;
		char vs[5];
		sprintf(vs,"%d",i);
		menu_item=gtk_menu_item_new_with_label(vs);
		SIGNAL_CONNECT(menu_item, "activate", rpcstat_version_select,
                               i);

		gtk_widget_show(menu_item);
		gtk_menu_append(GTK_MENU(vers_menu), menu_item);
	}
	gtk_option_menu_set_menu(GTK_OPTION_MENU(vers_opt), vers_menu);
}

static void *
rpcstat_list_programs(gpointer *key, gpointer *value, gpointer *user_data _U_)
{
	rpc_prog_info_key *k=(rpc_prog_info_key *)key;
	rpc_prog_info_value *v=(rpc_prog_info_value *)value;
	GtkWidget *menu_item;

	menu_item=gtk_menu_item_new_with_label(v->progname);
	SIGNAL_CONNECT(menu_item, "activate", rpcstat_program_select, k);

	gtk_widget_show(menu_item);
	gtk_menu_append(GTK_MENU(prog_menu), menu_item);

	if(!rpc_program){
		rpc_program=k->prog;
	}

	return NULL;
}

static void
dlg_destroy_cb(void)
{
	dlg=NULL;
}

static void
gtk_rpcstat_cb(GtkWidget *w _U_, gpointer d _U_)
{
	int i;
	char *filter;

	/* if the window is already open, bring it to front */
	if(dlg){
		gdk_window_raise(dlg->window);
		return;
	}

	dlg=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(dlg), "ONC-RPC Service Response Time statistics");
	SIGNAL_CONNECT(dlg, "destroy", dlg_destroy_cb, NULL);
	dlg_box=gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(dlg), dlg_box);
	gtk_widget_show(dlg_box);


	prog_box=gtk_hbox_new(FALSE, 10);
	/* Program label */
	gtk_container_set_border_width(GTK_CONTAINER(prog_box), 10);
	prog_label=gtk_label_new("Program:");
	gtk_box_pack_start(GTK_BOX(prog_box), prog_label, FALSE, FALSE, 0);
	gtk_widget_show(prog_label);

	/* Program menu */
	prog_opt=gtk_option_menu_new();
	prog_menu=gtk_menu_new();
	g_hash_table_foreach(rpc_progs, (GHFunc)rpcstat_list_programs, NULL);
	gtk_option_menu_set_menu(GTK_OPTION_MENU(prog_opt), prog_menu);
	gtk_box_pack_start(GTK_BOX(prog_box), prog_opt, TRUE, TRUE, 0);
	gtk_widget_show(prog_opt);

	/* Version label */
	gtk_container_set_border_width(GTK_CONTAINER(prog_box), 10);
	vers_label=gtk_label_new("Version:");
	gtk_box_pack_start(GTK_BOX(prog_box), vers_label, FALSE, FALSE, 0);
	gtk_widget_show(vers_label);

	/* Version menu */
	vers_opt=gtk_option_menu_new();
	vers_menu=gtk_menu_new();
	rpc_min_vers=-1;
	rpc_max_vers=-1;
	g_hash_table_foreach(rpc_procs, (GHFunc)rpcstat_find_vers, NULL);
	rpc_version=rpc_min_vers;
	for(i=rpc_min_vers;i<=rpc_max_vers;i++){
		GtkWidget *menu_item;
		char vs[5];
		sprintf(vs,"%d",i);
		menu_item=gtk_menu_item_new_with_label(vs);
		SIGNAL_CONNECT(menu_item, "activate", rpcstat_version_select,
                               i);

		gtk_widget_show(menu_item);
		gtk_menu_append(GTK_MENU(vers_menu), menu_item);
	}

	gtk_option_menu_set_menu(GTK_OPTION_MENU(vers_opt), vers_menu);
	gtk_box_pack_start(GTK_BOX(prog_box), vers_opt, TRUE, TRUE, 0);
	gtk_widget_show(vers_opt);

	gtk_box_pack_start(GTK_BOX(dlg_box), prog_box, TRUE, TRUE, 0);
	gtk_widget_show(prog_box);


	/* filter box */
	filter_box=gtk_hbox_new(FALSE, 10);
	/* Filter label */
	gtk_container_set_border_width(GTK_CONTAINER(filter_box), 10);
	filter_label=gtk_label_new("Filter:");
	gtk_box_pack_start(GTK_BOX(filter_box), filter_label, FALSE, FALSE, 0);
	gtk_widget_show(filter_label);

	filter_entry=gtk_entry_new_with_max_length(250);
	gtk_box_pack_start(GTK_BOX(filter_box), filter_entry, FALSE, FALSE, 0);
	filter=gtk_entry_get_text(GTK_ENTRY(main_display_filter_widget));
	if(filter){
		gtk_entry_set_text(GTK_ENTRY(filter_entry), filter);
	}
	gtk_widget_show(filter_entry);

	gtk_box_pack_start(GTK_BOX(dlg_box), filter_box, TRUE, TRUE, 0);
	gtk_widget_show(filter_box);


	/* the start button */
	start_button=gtk_button_new_with_label("Create Stat");
        SIGNAL_CONNECT_OBJECT(start_button, "clicked",
                              rpcstat_start_button_clicked, NULL);
	gtk_box_pack_start(GTK_BOX(dlg_box), start_button, TRUE, TRUE, 0);
	gtk_widget_show(start_button);

	gtk_widget_show_all(dlg);
}


void
register_tap_listener_gtkrpcstat(void)
{
	register_ethereal_tap("rpc,srt,", gtk_rpcstat_init);
}

void
register_tap_menu_gtkrpcstat(void)
{
	register_tap_menu_item("Service Response Time/ONC-RPC", gtk_rpcstat_cb);
}
