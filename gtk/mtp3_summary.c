/* mtp3_summary.c
 * Routines for MTP3 Statictics summary window
 *
 * Copyright 2004, Michael Lum <mlum [AT] telostech.com>
 * In association with Telos Technology Inc.
 *
 * Modified from gsm_map_summary.c
 *
 * $Id: mtp3_summary.c,v 1.2 2004/05/22 19:56:19 ulfl Exp $
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

#include <gtk/gtk.h>
#include <string.h>

#include <wtap.h>

#include "epan/packet_info.h"
#include "epan/epan.h"
#include "epan/value_string.h"
#include "tap_menu.h"
#include "summary.h"
#include "image/clist_ascend.xpm"
#include "image/clist_descend.xpm"
#include "dlg_utils.h"
#include "ui_util.h"
#include "compat_macros.h"
#include "tap.h"

#include "packet-mtp3.h"
#include "mtp3_stat.h"

#define SUM_STR_MAX 1024

typedef struct column_arrows {
    GtkWidget		*table;
    GtkWidget		*ascend_pm;
    GtkWidget		*descend_pm;
} column_arrows;

#define	MTP3_SUM_INIT_TABLE_NUM_COLUMNS		6

typedef struct _my_columns_t {
    guint32		value;
    gchar		*strptr;
    GtkJustification	just;
} my_columns_t;

static my_columns_t columns[MTP3_SUM_INIT_TABLE_NUM_COLUMNS] = {
    { 110,	"SI",			GTK_JUSTIFY_LEFT },
    { 100,	"Num MSUs",		GTK_JUSTIFY_RIGHT },
    { 100,	"MSUs/sec",		GTK_JUSTIFY_RIGHT },
    { 100,	"Num Bytes",		GTK_JUSTIFY_RIGHT },
    { 100,	"Bytes/MSU",		GTK_JUSTIFY_RIGHT },
    { 100,	"Bytes/sec",		GTK_JUSTIFY_RIGHT }
};


static void
add_string_to_box(gchar *str, GtkWidget *box)
{
  GtkWidget *lb;
  lb = gtk_label_new(str);
  gtk_misc_set_alignment(GTK_MISC(lb), 0.0, 0.5);
  gtk_box_pack_start(GTK_BOX(box), lb,FALSE,FALSE, 0);
  gtk_widget_show(lb);
}


static void
mtp3_sum_gtk_click_column_cb(
    GtkCList		*clist,
    gint		column,
    gpointer		data)
{
    column_arrows	*col_arrows = (column_arrows *) data;
    int			i;


    gtk_clist_freeze(clist);

    for (i=0; i < MTP3_SUM_INIT_TABLE_NUM_COLUMNS; i++)
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
	 * Columns 0 sorted in descending order by default
	 */
	if (column == 0)
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
mtp3_sum_gtk_sort_column(
    GtkCList		*clist,
    gconstpointer	ptr1,
    gconstpointer	ptr2)
{
    GtkCListRow		*row1 = (GtkCListRow *) ptr1;
    GtkCListRow		*row2 = (GtkCListRow *) ptr2;
    char		*text1 = NULL;
    char		*text2 = NULL;
    int			i1, i2;

    text1 = GTK_CELL_TEXT(row1->cell[clist->sort_column])->text;
    text2 = GTK_CELL_TEXT(row2->cell[clist->sort_column])->text;

    switch (clist->sort_column)
    {
    case 0:
	/* text columns */
	return(strcmp(text1, text2));

    default:
	/* number columns */
	i1 = strtol(text1, NULL, 0);
	i2 = strtol(text2, NULL, 0);
	return(i1 - i2);
    }

    g_assert_not_reached();

    return(0);
}

static void
mtp3_sum_draw(
    GtkWidget		*table,
    double		seconds,
    int			*tot_num_msus_p,
    double		*tot_num_bytes_p)
{
    char		*entries[MTP3_SUM_INIT_TABLE_NUM_COLUMNS];
    int			i, j;
    int			num_msus;
    double		num_bytes;

    if (table == NULL)
    {
	return;
    }

    *tot_num_msus_p = 0;
    *tot_num_bytes_p = 0;

    for (i=0; i < MTP3_NUM_SI_CODE; i++)
    {
	entries[0] = g_strdup(mtp3_service_indicator_code_short_vals[i].strptr);

	j = 0;
	num_msus = 0;
	num_bytes = 0;

	while (j < mtp3_num_used)
	{
	    num_msus += mtp3_stat[j].si_code[i].num_msus;
	    num_bytes += mtp3_stat[j].si_code[i].size;

	    j++;
	}

	*tot_num_msus_p += num_msus;
	*tot_num_bytes_p += num_bytes;

	entries[1] = g_strdup_printf("%u", num_msus);

	entries[2] = g_strdup_printf("%.2f", num_msus/seconds);

	entries[3] = g_strdup_printf("%.0f", num_bytes);

	entries[4] = g_strdup_printf("%.2f", num_bytes/num_msus);

	entries[5] = g_strdup_printf("%.2f", num_bytes/seconds);

	gtk_clist_insert(GTK_CLIST(table), i, entries);
    }

    gtk_clist_sort(GTK_CLIST(table));
}


void
mtp3_sum_gtk_sum_cb(GtkWidget *w _U_, gpointer d _U_)
{
  summary_tally summary;
  GtkWidget     *sum_open_w,
                *main_vb, *file_fr, *data_fr, *file_box,
		*data_box, *bbox, *close_bt,
		*tot_fr, *tot_box,
		*table, *column_lb, *table_fr;
  column_arrows	*col_arrows;
  GdkBitmap	*ascend_bm, *descend_bm;
  GdkPixmap	*ascend_pm, *descend_pm;
  GtkStyle	*win_style;

  gchar         string_buff[SUM_STR_MAX];
  double        seconds;
  int		tot_num_msus;
  double        tot_num_bytes;
  int		i;

  /* initialize the tally */
  summary_fill_in(&summary);

  /* initial compututations */
  seconds = summary.stop_time - summary.start_time;

  sum_open_w = dlg_window_new("MTP3 Statistics: Summary");

  /* Container for each row of widgets */
  main_vb = gtk_vbox_new(FALSE, 3);
  gtk_container_border_width(GTK_CONTAINER(main_vb), 5);
  gtk_container_add(GTK_CONTAINER(sum_open_w), main_vb);
  gtk_widget_show(main_vb);

  /* File frame */
  file_fr = gtk_frame_new("File");
  gtk_container_add(GTK_CONTAINER(main_vb), file_fr);
  gtk_widget_show(file_fr);

  file_box = gtk_vbox_new(FALSE, 3);
  gtk_container_add(GTK_CONTAINER(file_fr), file_box);
  gtk_widget_show(file_box);

  /* filename */
  g_snprintf(string_buff, SUM_STR_MAX, "Name: %s", summary.filename);
  add_string_to_box(string_buff, file_box);

  /* length */
  g_snprintf(string_buff, SUM_STR_MAX, "Length: %lu", summary.file_length);
  add_string_to_box(string_buff, file_box);

  /* format */
  g_snprintf(string_buff, SUM_STR_MAX, "Format: %s", wtap_file_type_string(summary.encap_type));
  add_string_to_box(string_buff, file_box);

  if (summary.has_snap) {
    /* snapshot length */
    g_snprintf(string_buff, SUM_STR_MAX, "Snapshot length: %u", summary.snap);
    add_string_to_box(string_buff, file_box);
  }

  /* Data frame */
  data_fr = gtk_frame_new("Data");
  gtk_container_add(GTK_CONTAINER(main_vb), data_fr);
  gtk_widget_show(data_fr);

  data_box = gtk_vbox_new(FALSE, 3);
  gtk_container_add(GTK_CONTAINER(data_fr), data_box);
  gtk_widget_show(data_box);

  /* seconds */
  g_snprintf(string_buff, SUM_STR_MAX, "Elapsed time: %.3f seconds", summary.elapsed_time);
  add_string_to_box(string_buff, data_box);

  g_snprintf(string_buff, SUM_STR_MAX, "Between first and last packet: %.3f seconds", seconds);
  add_string_to_box(string_buff, data_box);

  /* Packet count */
  g_snprintf(string_buff, SUM_STR_MAX, "Packet count: %i", summary.packet_count);
  add_string_to_box(string_buff, data_box);

  /* MTP3 SPECIFIC */
  table_fr = gtk_frame_new("Service Indicator (SI) Totals");
  gtk_container_add(GTK_CONTAINER(main_vb), table_fr);
  gtk_widget_show(table_fr);

  table = gtk_clist_new(MTP3_SUM_INIT_TABLE_NUM_COLUMNS);
  gtk_container_add(GTK_CONTAINER(table_fr), table);
  gtk_widget_show(table);

  col_arrows =
      (column_arrows *) g_malloc(sizeof(column_arrows) * MTP3_SUM_INIT_TABLE_NUM_COLUMNS);

  win_style =
      gtk_widget_get_style(sum_open_w);

  /* We must display dialog widget before calling gdk_pixmap_create_from_xpm_d() */
  gtk_widget_show_all(sum_open_w);

  ascend_pm =
      gdk_pixmap_create_from_xpm_d(sum_open_w->window,
	  &ascend_bm,
	  &win_style->bg[GTK_STATE_NORMAL],
	  (gchar **) clist_ascend_xpm);

  descend_pm =
      gdk_pixmap_create_from_xpm_d(sum_open_w->window,
	  &descend_bm,
	  &win_style->bg[GTK_STATE_NORMAL],
	  (gchar **)clist_descend_xpm);

  for (i = 0; i < MTP3_SUM_INIT_TABLE_NUM_COLUMNS; i++)
  {
      col_arrows[i].table = gtk_table_new(2, 2, FALSE);

      gtk_table_set_col_spacings(GTK_TABLE(col_arrows[i].table), 5);

      column_lb = gtk_label_new(columns[i].strptr);

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

      gtk_clist_set_column_justification(GTK_CLIST(table), i, columns[i].just);

      gtk_clist_set_column_widget(GTK_CLIST(table), i, col_arrows[i].table);
      gtk_widget_show(col_arrows[i].table);
  }
  gtk_clist_column_titles_show(GTK_CLIST(table));

  gtk_clist_set_compare_func(GTK_CLIST(table), mtp3_sum_gtk_sort_column);
  gtk_clist_set_sort_column(GTK_CLIST(table), 0);
  gtk_clist_set_sort_type(GTK_CLIST(table), GTK_SORT_ASCENDING);

  for (i = 0; i < MTP3_SUM_INIT_TABLE_NUM_COLUMNS; i++)
  {
      gtk_clist_set_column_width(GTK_CLIST(table), i, columns[i].value);
  }

  gtk_clist_set_shadow_type(GTK_CLIST(table), GTK_SHADOW_IN);
  gtk_clist_column_titles_show(GTK_CLIST(table));

  SIGNAL_CONNECT(table, "click-column", mtp3_sum_gtk_click_column_cb, col_arrows);

  mtp3_sum_draw(table, seconds, &tot_num_msus, &tot_num_bytes);

  /* Totals frame */
  tot_fr = gtk_frame_new("Totals");
  gtk_container_add(GTK_CONTAINER(main_vb), tot_fr);
  gtk_widget_show(tot_fr);

  tot_box = gtk_vbox_new(FALSE, 3);
  gtk_container_add(GTK_CONTAINER(tot_fr), tot_box);
  gtk_widget_show(tot_box);

  g_snprintf(string_buff, SUM_STR_MAX, "Total MSUs: %u", tot_num_msus);
  add_string_to_box(string_buff, tot_box);

  g_snprintf(string_buff, SUM_STR_MAX, "MSUs/second: %.2f", tot_num_msus/seconds);
  add_string_to_box(string_buff, tot_box);

  g_snprintf(string_buff, SUM_STR_MAX, "Total Bytes: %.0f", tot_num_bytes);
  add_string_to_box(string_buff, tot_box);

  g_snprintf(string_buff, SUM_STR_MAX, "Average Bytes/MSU: %.2f", tot_num_bytes/tot_num_msus);
  add_string_to_box(string_buff, tot_box);

  g_snprintf(string_buff, SUM_STR_MAX, "Bytes/second: %.2f", tot_num_bytes/seconds);
  add_string_to_box(string_buff, tot_box);

  /* Button row. */
  bbox = dlg_button_row_new(GTK_STOCK_CLOSE, NULL);
  gtk_container_add(GTK_CONTAINER(main_vb), bbox);
  gtk_widget_show(bbox);

  close_bt = OBJECT_GET_DATA(bbox, GTK_STOCK_CLOSE);
  SIGNAL_CONNECT_OBJECT(close_bt, "clicked", gtk_widget_destroy, sum_open_w);
  gtk_widget_grab_default(close_bt);

  /* Catch the "key_press_event" signal in the window, so that we can catch
     the ESC key being pressed and act as if the "Close" button had
     been selected. */
  dlg_set_cancel(sum_open_w, close_bt);

  gtk_widget_show_all(sum_open_w);
}


void
register_tap_listener_gtkmtp3_summary(void)
{
    register_tap_menu_item("MTP3/MSU Summary",  REGISTER_TAP_GROUP_NONE,
        mtp3_sum_gtk_sum_cb, NULL, NULL, NULL);
}
