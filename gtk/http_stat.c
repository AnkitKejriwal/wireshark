/* http_stat.c
 * http_stat   2003 Jean-Michel FAYARD
 *
 * $Id: http_stat.c,v 1.30 2004/05/23 23:24:06 ulfl Exp $
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

#include <gtk/gtk.h>

#include <epan/packet_info.h>
#include <epan/epan.h>

#include "tap_menu.h"
#include "simple_dialog.h"
#include "ui_util.h"
#include "dlg_utils.h"
#include "tap.h"
#include "../register.h"
#include "../packet-http.h"
#include "../globals.h"
#include "compat_macros.h"
#include "../tap_dfilter_dlg.h"
#include "tap_dfilter_dlg.h"

	
/* used to keep track of the statictics for an entire program interface */
typedef struct _http_stats_t {
	char 		*filter;
	GtkWidget 	*win;
	GHashTable	*hash_responses;
	GHashTable	*hash_requests;
	guint32		 packets;		/* number of http packets, including HTTP continuation */
	GtkWidget	*packets_label;

	GtkWidget	*request_box;		/* container for GET, ... */

	GtkWidget	*informational_table;	/* Status code between 	100 and 199 */
	GtkWidget	*success_table;		/*			200 and 299 */
	GtkWidget	*redirection_table;	/*			300 and 399 */
	GtkWidget	*client_error_table;	/*			400 and 499 */
	GtkWidget	*server_errors_table;	/*			500 and 599 */
} httpstat_t;

/* used to keep track of the stats for a specific response code
 * for example it can be { 3, 404, "Not Found" ,...}
 * which means we captured 3 reply http/1.1 404 Not Found */
typedef struct _http_response_code_t {
	guint32 	 packets;		/* 3 */
	guint	 	 response_code;		/* 404 */
	gchar		*name;			/* Not Found */
	GtkWidget	*widget;		/* Label where we display it */
	GtkWidget	*table;			/* Table in which we put it, e.g. client_error_box */
	httpstat_t	*sp;
} http_response_code_t;

/* used to keep track of the stats for a specific request string */
typedef struct _http_request_methode_t {
	gchar		*response;	/* eg. : GET */
	guint32		 packets;
	GtkWidget	*widget;
	httpstat_t	*sp;
} http_request_methode_t;

static const value_string vals_status_code[] = {
	{ 100, "Continue" },
	{ 101, "Switching Protocols" },
	{ 199, "Informational - Others" },

	{ 200, "OK"},
	{ 201, "Created"},
	{ 202, "Accepted"},
	{ 203, "Non-authoritative Information"},
	{ 204, "No Content"},
	{ 205, "Reset Content"},
	{ 206, "Partial Content"},
	{ 299, "Success - Others"},	/* used to keep track of others Success packets */

	{ 300, "Multiple Choices"},
	{ 301, "Moved Permanently"},
	{ 302, "Moved Temporarily"},
	{ 303, "See Other"},
        { 304, "Not Modified"},
        { 305, "Use Proxy"},
	{ 399, "Redirection - Others"},

        { 400, "Bad Request"},
        { 401, "Unauthorized"},
        { 402, "Payment Required"},
        { 403, "Forbidden"},
        { 404, "Not Found"},
        { 405, "Method Not Allowed"},
        { 406, "Not Acceptable"},
        { 407, "Proxy Authentication Required"},
        { 408, "Request Time-out"},
        { 409, "Conflict"},
        { 410, "Gone"},
        { 411, "Length Required"},
        { 412, "Precondition Failed"},
        { 413, "Request Entity Too Large"},
        { 414, "Request-URI Too Large"},
        { 415, "Unsupported Media Type"},
	{ 499, "Client Error - Others"},
	
        { 500, "Internal Server Error"},
        { 501, "Not Implemented"},
        { 502, "Bad Gateway"},
        { 503, "Service Unavailable"},
        { 504, "Gateway Time-out"},
        { 505, "HTTP Version not supported"},
	{ 599, "Server Error - Others"},

	{ 0, 	NULL}
} ;

/* insert some entries */
static void
http_init_hash( httpstat_t *sp)
{
	int i;

	sp->hash_responses = g_hash_table_new( g_int_hash, g_int_equal);		
			
	for (i=0 ; vals_status_code[i].strptr ; i++ )
	{
		gint *key = g_malloc (sizeof(gint));
		http_response_code_t *sc = g_malloc (sizeof(http_response_code_t));
		*key = vals_status_code[i].value;
		sc->packets=0;
		sc->response_code =  *key;
		sc->name=vals_status_code[i].strptr;
		sc->widget=NULL;
		sc->table=NULL;
		sc->sp = sp;
		g_hash_table_insert( sc->sp->hash_responses, key, sc);
	}
	sp->hash_requests = g_hash_table_new( g_str_hash, g_str_equal);
}

#define SUM_STR_MAX 1024

static void
http_draw_hash_requests( gchar *key _U_ , http_request_methode_t *data, gchar * unused _U_)
{
    gchar	string_buff[SUM_STR_MAX];

	if (data->packets==0)
		return;
	g_snprintf(string_buff, sizeof(string_buff), 
        "     %-11s : %3d packets", data->response, data->packets); 
	if (data->widget==NULL){
		data->widget=gtk_label_new( string_buff );
		gtk_misc_set_alignment(GTK_MISC(data->widget), 0.0, 0.5);
		gtk_box_pack_start(GTK_BOX(data->sp->request_box), data->widget,FALSE,FALSE, 0);
		gtk_widget_show(data->widget);
	} else {
		gtk_label_set( GTK_LABEL(data->widget), string_buff);
	}
}

static void
http_draw_hash_responses( gint * key _U_ , http_response_code_t *data, gchar * unused _U_)
{
    gchar	string_buff[SUM_STR_MAX];

	if (data==NULL) {
		g_warning("C'est quoi ce borderl key=%d\n", *key);
	}
	if (data->packets==0)
		return;
	/*g_snprintf(string_buff, sizeof(string_buff),
        "%d packets %d:%s", data->packets, data->response_code, data->name); */
	if (data->widget==NULL){	/* create an entry in the relevant box of the window */
		guint16 x;
		GtkWidget *tmp;
		guint i = data->response_code;

		if ( (i<100)||(i>=600) ) 
			return;
		if (i<200)
			data->table = data->sp->informational_table;
		else if (i<300)
			data->table = data->sp->success_table;
		else if (i<400)
			data->table = data->sp->redirection_table;
		else if (i<500)
			data->table = data->sp->client_error_table;
		else
			data->table = data->sp->server_errors_table;
		x=GTK_TABLE( data->table)->nrows;
		
		g_snprintf(string_buff, sizeof(string_buff),
            "HTTP %3d %s ", data->response_code, data->name ); 
		tmp = gtk_label_new( string_buff );
		
		gtk_table_attach_defaults( GTK_TABLE(data->table), tmp,  0,1, x, x+1);
		gtk_label_set_justify( GTK_LABEL(tmp), GTK_JUSTIFY_LEFT);
		gtk_widget_show( tmp );

		g_snprintf(string_buff, sizeof(string_buff),
            "%9d", data->packets);
		data->widget=gtk_label_new( string_buff);

		gtk_table_attach_defaults( GTK_TABLE(data->table), data->widget, 1, 2,x,x+1);
		gtk_label_set_justify( GTK_LABEL(data->widget), GTK_JUSTIFY_RIGHT);
		gtk_widget_show( data->widget );
		
		gtk_table_resize( GTK_TABLE(data->table), x+1, 4);
		
	} else {
		/* Just update the label string */
		g_snprintf(string_buff, sizeof(string_buff),
            "%9d", data->packets );
		gtk_label_set( GTK_LABEL(data->widget), string_buff);
	}
}
		

		
static void
http_free_hash( gpointer key, gpointer value, gpointer user_data _U_ )
{
	g_free(key);
	g_free(value);
}
static void
http_reset_hash_responses(gchar *key _U_ , http_response_code_t *data, gpointer ptr _U_ ) 
{	
	data->packets = 0;
}
static void
http_reset_hash_requests(gchar *key _U_ , http_request_methode_t *data, gpointer ptr _U_ ) 
{	
	data->packets = 0;
}

static void
httpstat_reset(void *psp  )
{
 	httpstat_t *sp=psp;
	if (sp) {
        sp->packets = 0;
		g_hash_table_foreach( sp->hash_responses, (GHFunc)http_reset_hash_responses, NULL);
		g_hash_table_foreach( sp->hash_requests, (GHFunc)http_reset_hash_requests, NULL);
	}
}

static int
httpstat_packet(void *psp , packet_info *pinfo _U_, epan_dissect_t *edt _U_, void *pri)
{
	http_info_value_t *value=pri;
	httpstat_t *sp=(httpstat_t *) psp;

	/* total number of packets, including HTTP continuation packets */
	sp->packets++;

	/* We are only interested in reply packets with a status code */
	/* Request or reply packets ? */
	if (value->response_code!=0) {
		guint *key=g_malloc( sizeof(guint) );
		http_response_code_t *sc;
		
		*key=value->response_code;
		sc =  g_hash_table_lookup( 
				sp->hash_responses, 
				key);
		if (sc==NULL){
			/* non standard status code ; we classify it as others
			 * in the relevant category (Informational,Success,Redirection,Client Error,Server Error)
			 */
			int i = value->response_code;
			if ((i<100) || (i>=600)) {
				return 0;
			}
			else if (i<200){
				*key=199;	/* Hopefully, this status code will never be used */
			}
			else if (i<300){
				*key=299;
			}
			else if (i<400){
				*key=399;
			}
			else if (i<500){
				*key=499;
			}
			else{
				*key=599;
			}
			sc =  g_hash_table_lookup( 
				sp->hash_responses, 
				key);
			if (sc==NULL)
				return 0;
		}
		sc->packets++;
	} 
	else if (value->request_method){
		http_request_methode_t *sc;

		sc =  g_hash_table_lookup( 
				sp->hash_requests, 
				value->request_method);
		if (sc==NULL){
			sc=g_malloc( sizeof(http_request_methode_t) );
			sc->response=g_strdup( value->request_method );
			sc->packets=1;
			sc->widget=NULL;
			sc->sp = sp;
			g_hash_table_insert( sp->hash_requests, sc->response, sc);
		} else {
			sc->packets++;
		}
		/* g_free( value->request_method ); */
	} else {
		return 0;
	}
	return 1;
}


static void
httpstat_draw(void *psp  )
{
    gchar	string_buff[SUM_STR_MAX];
	httpstat_t *sp=psp;

	g_snprintf( string_buff, sizeof(string_buff), "HTTP stats (%d packets)", sp->packets);
	gtk_label_set( GTK_LABEL(sp->packets_label), string_buff);
	g_hash_table_foreach( sp->hash_responses, (GHFunc)http_draw_hash_responses, NULL);
	g_hash_table_foreach( sp->hash_requests,  (GHFunc)http_draw_hash_requests, NULL);
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
	httpstat_t *sp=(httpstat_t *)data;

	protect_thread_critical_region();
	remove_tap_listener(sp);
	unprotect_thread_critical_region();

	g_hash_table_foreach( sp->hash_responses, (GHFunc)http_free_hash, NULL);
	g_hash_table_destroy( sp->hash_responses);
	g_hash_table_foreach( sp->hash_requests, (GHFunc)http_free_hash, NULL);
	g_hash_table_destroy( sp->hash_requests);
	g_free(sp->filter);
	g_free(sp);
}


/* When called, this function will create a new instance of gtk_httpstat.
 */
static void
gtk_httpstat_init(char *optarg)
{
	httpstat_t *sp;
	char *filter=NULL;
	GString	*error_string;
	char *title=NULL;
	GtkWidget  *main_vb, *separator,
		*informational_fr, *success_fr, *redirection_fr, 
		*client_errors_fr, *server_errors_fr, *request_fr;
    GtkWidget	*bt_close;
    GtkWidget	*bbox;
	
	if (strncmp (optarg, "http,stat,", 10) == 0){
		filter=optarg+10;
	} else {
		filter=NULL;
	}
	
	/* top level window */
    sp = g_malloc( sizeof(httpstat_t) );
	sp->win = window_new(GTK_WINDOW_TOPLEVEL, "http-stat");

	if(filter){
		sp->filter=g_strdup(filter);
		title=g_strdup_printf("HTTP statistics with filter: %s", filter);
	} else {
		sp->filter=NULL;
		title=g_strdup("HTTP statistics");
	}

    gtk_window_set_title(GTK_WINDOW(sp->win), title);
	g_free(title);

	/* container for each group of status code */
	main_vb = gtk_vbox_new(FALSE, 12);
	gtk_container_border_width(GTK_CONTAINER(main_vb), 12);
	gtk_container_add(GTK_CONTAINER(sp->win), main_vb);
	
	/* number of packets */
	sp->packets=0;
	sp->packets_label = gtk_label_new("HTTP stats (0 HTTP packets)");
	gtk_container_add( GTK_CONTAINER(main_vb), sp->packets_label);

	/* Informational response frame */
	informational_fr = gtk_frame_new("Informational  HTTP 1xx");
  	gtk_container_add(GTK_CONTAINER(main_vb), informational_fr);
	
	sp->informational_table = gtk_table_new( 0, 2, FALSE);
	gtk_container_add(GTK_CONTAINER(informational_fr), sp->informational_table);

	/* success response frame */
	success_fr = gtk_frame_new	("Success         HTTP 2xx");
  	gtk_container_add(GTK_CONTAINER(main_vb), success_fr);
	
	sp->success_table = gtk_table_new( 0, 2, FALSE);
	gtk_container_add(GTK_CONTAINER(success_fr), sp->success_table);

	/* redirection response frame */
	redirection_fr = gtk_frame_new	("Redirection     HTTP 3xx");
  	gtk_container_add(GTK_CONTAINER(main_vb), redirection_fr);
	
	sp->redirection_table = gtk_table_new( 0, 2, FALSE);
	gtk_container_add(GTK_CONTAINER(redirection_fr), sp->redirection_table);

	/* client_errors response frame */
	client_errors_fr = gtk_frame_new("Client errors  HTTP 4xx");
  	gtk_container_add(GTK_CONTAINER(main_vb), client_errors_fr);
	
	sp->client_error_table = gtk_table_new( 0, 2, FALSE);
	gtk_container_add(GTK_CONTAINER(client_errors_fr), sp->client_error_table);

	/* server_errors response frame */
	server_errors_fr = gtk_frame_new("Server errors  HTTP 5xx");
  	gtk_container_add(GTK_CONTAINER(main_vb), server_errors_fr);
	
	sp->server_errors_table = gtk_table_new( 0, 2, FALSE);
	gtk_container_add(GTK_CONTAINER(server_errors_fr), sp->server_errors_table);

	separator = gtk_hseparator_new();
	gtk_container_add(GTK_CONTAINER(main_vb), separator );

	/* request response frame */
	request_fr = gtk_frame_new("List of request methods");
  	gtk_container_add(GTK_CONTAINER(main_vb), request_fr);
	gtk_container_border_width(GTK_CONTAINER(request_fr), 0);
	
	sp->request_box = gtk_vbox_new(FALSE, 10);
	gtk_container_add(GTK_CONTAINER(request_fr), sp->request_box);

	error_string = register_tap_listener( 
			"http",
			sp,
			filter,
			httpstat_reset,
			httpstat_packet,
			httpstat_draw);
	if (error_string){
		/* error, we failed to attach to the tap. clean up */
		simple_dialog( ESD_TYPE_ERROR, ESD_BTN_OK, error_string->str );
		g_free(sp->filter);
		g_free(sp);
		g_string_free(error_string, TRUE);
		return ;
	}

	/* Button row. */
    bbox = dlg_button_row_new(GTK_STOCK_CLOSE, NULL);
    gtk_box_pack_start(GTK_BOX(main_vb), bbox, FALSE, FALSE, 0);

    bt_close = OBJECT_GET_DATA(bbox, GTK_STOCK_CLOSE);
    window_set_cancel_button(sp->win, bt_close, window_cancel_button_cb);

    SIGNAL_CONNECT(sp->win, "delete_event", window_delete_event_cb, NULL);
	SIGNAL_CONNECT(sp->win, "destroy", win_destroy_cb, sp);

    gtk_widget_show_all(sp->win);
    window_present(sp->win);

	http_init_hash(sp);
	retap_packets(&cfile);
}

static tap_dfilter_dlg http_stat_dlg = {
	"HTTP Packet Counter",
	"http,stat",
	gtk_httpstat_init,
	-1
};

void
register_tap_listener_gtkhttpstat(void)
{
	register_ethereal_tap("http,stat", gtk_httpstat_init);

	register_tap_menu_item("HTTP", REGISTER_TAP_GROUP_NONE,
	    gtk_tap_dfilter_dlg_cb, NULL, NULL, &(http_stat_dlg));
}
