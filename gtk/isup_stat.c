/* isup_stat.c
 *
 * Copyright 2003, Michael Lum <mlum [AT] telostech.com>
 * In association with Telos Technology Inc.
 *
 * MUCH code modified from service_response_time_table.c.
 *
 * $Id: isup_stat.c,v 1.17 2004/02/18 04:11:42 jmayer Exp $
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

/*
 * This TAP provides statistics for ISUP:
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <gtk/gtk.h>
#include <string.h>

#include "epan/packet_info.h"
#include "epan/epan.h"
#include "epan/value_string.h"
#include "tap_menu.h"
#include "image/clist_ascend.xpm"
#include "image/clist_descend.xpm"
#include "simple_dialog.h"
#include "dlg_utils.h"
#include "tap.h"
#include "../register.h"
#include "../globals.h"
#include "filter_prefs.h"
#include "compat_macros.h"
#include "packet-isup.h"
#include "ui_util.h"


typedef struct column_arrows {
    GtkWidget		*table;
    GtkWidget		*ascend_pm;
    GtkWidget		*descend_pm;
} column_arrows;

typedef struct _isup_stat_dlg_t {
    GtkWidget		*win;
    GtkWidget		*scrolled_win;
    GtkWidget		*table;
    char		*entries[4];
} isup_stat_dlg_t;

#define N_MESSAGE_TYPES	256

typedef struct _isup_stat_t {
    int			message_type[N_MESSAGE_TYPES];
} isup_stat_t;


static isup_stat_dlg_t	dlg;
static isup_stat_t	stat;


static void
isup_stat_reset(
    void		*tapdata)
{
    tapdata = tapdata;

    memset((void *) &stat, 0, sizeof(isup_stat_t));
}


static int
isup_stat_packet(
    void		*tapdata,
    packet_info		*pinfo,
    epan_dissect_t	*edt _U_,
    void		*data)
{
    isup_tap_rec_t	*data_p = data;


    tapdata = tapdata;
    pinfo = pinfo;

#if 0	/* always false because message_type is 8 bit value */
    if (data_p->message_type >= N_MESSAGE_TYPES)
    {
	/*
	 * unknown message type !!!
	 */
	return(0);
    }
#endif

    stat.message_type[data_p->message_type]++;

    return(1);
}


static void
isup_stat_draw(
    void		*tapdata)
{
    int			i, j;
    char		str[256], *strp;


    tapdata = tapdata;

    if (dlg.win != NULL)
    {
	i = 0;

	while (isup_message_type_value[i].strptr)
	{
	    j = gtk_clist_find_row_from_data(GTK_CLIST(dlg.table), (gpointer) i);

	    sprintf(str, "%d",
		stat.message_type[isup_message_type_value[i].value]);
	    strp = g_strdup(str);
	    gtk_clist_set_text(GTK_CLIST(dlg.table), j, 3, strp);
	    g_free(strp);

	    i++;
	}

	gtk_clist_sort(GTK_CLIST(dlg.table));
    }
}


static void
isup_stat_gtk_click_column_cb(
    GtkCList		*clist,
    gint		column,
    gpointer		data)
{
    column_arrows	*col_arrows = (column_arrows *) data;
    int			i;


    gtk_clist_freeze(clist);

    for (i=0; i < 4; i++)
    {
	gtk_widget_hide(col_arrows[i].ascend_pm);
	gtk_widget_hide(col_arrows[i].descend_pm);
    }

    if (column == clist->sort_column)
    {
	if (clist->sort_type == GTK_SORT_ASCENDING)
	{
	    clist->sort_type = GTK_SORT_DESCENDING;
	    gtk_widget_show(col_arrows[column].descend_pm);
	}
	else
	{
	    clist->sort_type = GTK_SORT_ASCENDING;
	    gtk_widget_show(col_arrows[column].ascend_pm);
	}
    }
    else
    {
	/*
	 * Columns 0-2 sorted in descending order by default
	 * Columns 3 sorted in ascending order by default
	 */
	if (column <= 2)
	{
	    clist->sort_type = GTK_SORT_ASCENDING;
	    gtk_widget_show(col_arrows[column].ascend_pm);
	}
	else
	{
	    clist->sort_type = GTK_SORT_DESCENDING;
	    gtk_widget_show(col_arrows[column].descend_pm);
	}

	gtk_clist_set_sort_column(clist, column);
    }

    gtk_clist_thaw(clist);
    gtk_clist_sort(clist);
}


static gint
isup_stat_gtk_sort_column(
    GtkCList		*clist,
    gconstpointer	ptr1,
    gconstpointer	ptr2)
{
    const GtkCListRow		*row1 = (const GtkCListRow *) ptr1;
    const GtkCListRow		*row2 = (const GtkCListRow *) ptr2;
    char		*text1 = NULL;
    char		*text2 = NULL;
    int			i1, i2;

    text1 = GTK_CELL_TEXT(row1->cell[clist->sort_column])->text;
    text2 = GTK_CELL_TEXT(row2->cell[clist->sort_column])->text;

    switch (clist->sort_column)
    {
    case 0:
	/* FALLTHRU */

    case 3:
	i1 = strtol(text1, NULL, 0);
	i2 = strtol(text2, NULL, 0);
	return(i1 - i2);

    case 2:
    case 1:
	return(strcmp(text1, text2));
    }

    g_assert_not_reached();

    return(0);
}


static void
isup_stat_gtk_dlg_close_cb(
    GtkButton		*button _U_,
    gpointer		user_data _U_)
{
    isup_stat_dlg_t	*dlg_p = user_data;

    gtk_grab_remove(GTK_WIDGET(dlg_p->win));
    gtk_widget_destroy(GTK_WIDGET(dlg_p->win));
}


static void
isup_stat_gtk_win_destroy_cb(
    GtkWindow		*win _U_,
    gpointer		user_data _U_)
{
    memset((void *) user_data, 0, sizeof(isup_stat_dlg_t));
}


static void
isup_stat_gtk_win_create(
    isup_stat_dlg_t	*dlg_p,
    char		*title)
{
#define	INIT_TABLE_NUM_COLUMNS	4
    char		*default_titles[] = { "ID", "Acronym", "Message Name", "Count" };
    int			i;
    column_arrows	*col_arrows;
    GdkBitmap		*ascend_bm, *descend_bm;
    GdkPixmap		*ascend_pm, *descend_pm;
    GtkStyle		*win_style;
    GtkWidget		*column_lb;
    GtkWidget		*vbox;
    GtkWidget		*bt_close;
    GtkWidget		*bbox;
    GtkWidget		*dialog_vbox;
    GtkWidget		*dialog_action_area;


    dlg_p->win = gtk_dialog_new();
    gtk_window_set_default_size(GTK_WINDOW(dlg_p->win), 560, 450);
    gtk_window_set_title(GTK_WINDOW(dlg_p->win), title);
    SIGNAL_CONNECT(dlg_p->win, "destroy", isup_stat_gtk_win_destroy_cb, dlg_p);

    dialog_vbox = GTK_DIALOG(dlg_p->win)->vbox;
    gtk_widget_show(dialog_vbox);

    dialog_action_area = GTK_DIALOG(dlg_p->win)->action_area;
    gtk_widget_show(dialog_action_area);
    gtk_container_set_border_width(GTK_CONTAINER(dialog_action_area), 10);

    bbox = dlg_button_row_new(GTK_STOCK_CLOSE, NULL);
    gtk_box_pack_start(GTK_BOX(dialog_action_area), bbox, FALSE, FALSE, 0);
    gtk_widget_show(bbox);

    bt_close = OBJECT_GET_DATA(bbox, GTK_STOCK_CLOSE);
    SIGNAL_CONNECT(bt_close, "clicked", isup_stat_gtk_dlg_close_cb, dlg_p);
    gtk_widget_grab_default(bt_close);

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_widget_ref(vbox);
    OBJECT_SET_DATA_FULL(dlg_p->win, "vbox", vbox, gtk_widget_unref);
    gtk_widget_show(vbox);
    gtk_box_pack_start(GTK_BOX(dialog_vbox), vbox, TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 8);

    dlg_p->scrolled_win = scrolled_window_new(NULL, NULL);
    gtk_widget_ref(dlg_p->scrolled_win);
    OBJECT_SET_DATA_FULL(dlg_p->win, "scrolled_win", dlg_p->scrolled_win,
                         gtk_widget_unref);
    gtk_widget_show(dlg_p->scrolled_win);
    gtk_box_pack_start(GTK_BOX(vbox), dlg_p->scrolled_win, TRUE, TRUE, 0);

    dlg_p->table = gtk_clist_new(INIT_TABLE_NUM_COLUMNS);
    gtk_widget_ref(dlg_p->table);
    OBJECT_SET_DATA_FULL(dlg_p->win, "table", dlg_p->table, gtk_widget_unref);
    gtk_widget_show(dlg_p->table);

    gtk_widget_show(dlg_p->win);

    col_arrows =
	(column_arrows *) g_malloc(sizeof(column_arrows) * INIT_TABLE_NUM_COLUMNS);

    win_style =
	gtk_widget_get_style(dlg_p->scrolled_win);

    ascend_pm =
	gdk_pixmap_create_from_xpm_d(dlg_p->scrolled_win->window,
	    &ascend_bm,
	    &win_style->bg[GTK_STATE_NORMAL],
	    (gchar **) clist_ascend_xpm);

    descend_pm =
	gdk_pixmap_create_from_xpm_d(dlg_p->scrolled_win->window,
	    &descend_bm,
	    &win_style->bg[GTK_STATE_NORMAL],
	    (gchar **)clist_descend_xpm);

    for (i = 0; i < INIT_TABLE_NUM_COLUMNS; i++)
    {
	col_arrows[i].table = gtk_table_new(2, 2, FALSE);

	gtk_table_set_col_spacings(GTK_TABLE(col_arrows[i].table), 5);

	column_lb = gtk_label_new(default_titles[i]);

	gtk_table_attach(GTK_TABLE(col_arrows[i].table), column_lb,
	    0, 1, 0, 2, GTK_SHRINK, GTK_SHRINK, 0, 0);

	gtk_widget_show(column_lb);

	col_arrows[i].ascend_pm =
	    gtk_pixmap_new(ascend_pm, ascend_bm);

	gtk_table_attach(GTK_TABLE(col_arrows[i].table), col_arrows[i].ascend_pm,
	    1, 2, 1, 2, GTK_SHRINK, GTK_SHRINK, 0, 0);

	col_arrows[i].descend_pm =
	    gtk_pixmap_new(descend_pm, descend_bm);

	gtk_table_attach(GTK_TABLE(col_arrows[i].table), col_arrows[i].descend_pm,
	    1, 2, 0, 1, GTK_SHRINK, GTK_SHRINK, 0, 0);

	if (i == 0)
	{
	    /* default column sorting */
	    gtk_widget_show(col_arrows[i].ascend_pm);
	}

	gtk_clist_set_column_widget(GTK_CLIST(dlg_p->table), i, col_arrows[i].table);
	gtk_widget_show(col_arrows[i].table);
    }
    gtk_clist_column_titles_show(GTK_CLIST(dlg_p->table));

    gtk_clist_set_compare_func(GTK_CLIST(dlg_p->table), isup_stat_gtk_sort_column);
    gtk_clist_set_sort_column(GTK_CLIST(dlg_p->table), 0);
    gtk_clist_set_sort_type(GTK_CLIST(dlg_p->table), GTK_SORT_ASCENDING);

    gtk_clist_set_column_width(GTK_CLIST(dlg_p->table), 0, 60);
    gtk_clist_set_column_width(GTK_CLIST(dlg_p->table), 1, 60);
    gtk_clist_set_column_width(GTK_CLIST(dlg_p->table), 2, 290);
    gtk_clist_set_column_width(GTK_CLIST(dlg_p->table), 3, 50);

    gtk_clist_set_shadow_type(GTK_CLIST(dlg_p->table), GTK_SHADOW_IN);
    gtk_clist_column_titles_show(GTK_CLIST(dlg_p->table));
    gtk_container_add(GTK_CONTAINER(dlg_p->scrolled_win), dlg_p->table);

    SIGNAL_CONNECT(dlg_p->table, "click-column", isup_stat_gtk_click_column_cb, col_arrows);
}


/*
 * Never gets called ?
 */
static void
isup_stat_gtk_init(
    char		*optarg)
{
    /* does not appear to be called */

    optarg = optarg;
}


static void
isup_stat_gtk_cb(
    GtkWidget		*w _U_,
    gpointer		d _U_)
{
    int			i;
    char		str[100];


    /*
     * if the window is already open, bring it to front
     */
    if (dlg.win)
    {
	gdk_window_raise(dlg.win->window);
	return;
    }

    isup_stat_gtk_win_create(&dlg, "ISUP Message Type Statistics");

    i = 0;
    while (isup_message_type_value[i].strptr)
    {
	sprintf(str, "%u", isup_message_type_value[i].value);
	dlg.entries[0] = g_strdup(str);

	dlg.entries[1] = g_strdup(isup_message_type_value_acro[i].strptr);

	dlg.entries[2] = g_strdup(isup_message_type_value[i].strptr);

	dlg.entries[3] = g_strdup("0");

	gtk_clist_insert(GTK_CLIST(dlg.table), i, dlg.entries);
	gtk_clist_set_row_data(GTK_CLIST(dlg.table), i, (gpointer) i);

	i++;
    }

    isup_stat_draw(NULL);
}


void
register_tap_listener_gtkisup_stat(void)
{
    GString		*err_p;


    register_ethereal_tap("isup,", isup_stat_gtk_init);

    memset((void *) &stat, 0, sizeof(isup_stat_t));

    err_p =
	register_tap_listener("isup", NULL, NULL,
	    isup_stat_reset,
	    isup_stat_packet,
	    isup_stat_draw);

    if (err_p != NULL)
    {
	simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK, err_p->str);
	g_string_free(err_p, TRUE);

	exit(1);
    }
}


void
register_tap_menu_gtkisup_stat(void)
{
    register_tap_menu_item("_Statistics/ISUP Message Type", isup_stat_gtk_cb, NULL, NULL, NULL);
}
