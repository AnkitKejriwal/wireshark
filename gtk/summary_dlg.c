/* summary_dlg.c
 * Routines for capture file summary window
 *
 * $Id: summary_dlg.c,v 1.26 2004/02/02 22:51:30 guy Exp $
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

#include <wtap.h>

#ifdef NEED_SNPRINTF_H
# include "snprintf.h"
#endif

#include "summary.h"
#include "summary_dlg.h"
#include "dlg_utils.h"
#include "ui_util.h"
#include "compat_macros.h"

#define SUM_STR_MAX 1024


static void
add_string_to_box(gchar *str, GtkWidget *box)
{
  GtkWidget *lb;
  lb = gtk_label_new(str);
  gtk_misc_set_alignment(GTK_MISC(lb), 0.0, 0.5);
  gtk_box_pack_start(GTK_BOX(box), lb,FALSE,FALSE, 0);
  gtk_widget_show(lb);
}


void
summary_open_cb(GtkWidget *w _U_, gpointer d _U_)
{
  summary_tally summary;
  GtkWidget     *sum_open_w,
                *main_vb, *file_fr, *data_fr, *capture_fr, *file_box,
		*filter_box, *filter_fr,
		*data_box, *capture_box, *bbox, *close_bt;

  gchar         string_buff[SUM_STR_MAX];

  double        seconds;
  guint         offset;
  gchar        *str_dup;
  gchar        *str_work;

 /* initialize the tally */
  summary_fill_in(&summary);

  /* initial computations */
  seconds = summary.stop_time - summary.start_time;
  sum_open_w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(sum_open_w), "Ethereal: Summary");
  SIGNAL_CONNECT(sum_open_w, "realize", window_icon_realize_cb, NULL);

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
  gtk_container_border_width(GTK_CONTAINER(file_box), 5);
  gtk_container_add(GTK_CONTAINER(file_fr), file_box);
  gtk_widget_show(file_box);

  /* filename */
  snprintf(string_buff, SUM_STR_MAX, "Name: %s", summary.filename);
  add_string_to_box(string_buff, file_box);

  /* length */
  snprintf(string_buff, SUM_STR_MAX, "Length: %lu", summary.file_length);
  add_string_to_box(string_buff, file_box);

  /* format */
  snprintf(string_buff, SUM_STR_MAX, "Format: %s", wtap_file_type_string(summary.encap_type));
  add_string_to_box(string_buff, file_box);

  if (summary.has_snap) {
    /* snapshot length */
    snprintf(string_buff, SUM_STR_MAX, "Snapshot length: %u", summary.snap);
    add_string_to_box(string_buff, file_box);
  }

  /* Data frame */
  data_fr = gtk_frame_new("Data");
  gtk_container_add(GTK_CONTAINER(main_vb), data_fr);
  gtk_widget_show(data_fr);

  data_box = gtk_vbox_new(FALSE, 3);
  gtk_container_border_width(GTK_CONTAINER(data_box), 5);
  gtk_container_add(GTK_CONTAINER(data_fr), data_box);
  gtk_widget_show(data_box);

  /* seconds */
  snprintf(string_buff, SUM_STR_MAX, "Elapsed time: %.3f seconds", summary.elapsed_time);
  add_string_to_box(string_buff, data_box);

  snprintf(string_buff, SUM_STR_MAX, "Between first and last packet: %.3f seconds", seconds);
  add_string_to_box(string_buff, data_box);

  /* Packet count */
  snprintf(string_buff, SUM_STR_MAX, "Packet count: %i", summary.packet_count);
  add_string_to_box(string_buff, data_box);

  /* Filtered Packet count */
  /* Unless there is none filter, we move informations about filtered packets in a separate frame */
  if (!summary.dfilter)
	add_string_to_box("Filtered packet count: 0", data_box);

  /* Marked Packet count */
  snprintf(string_buff, SUM_STR_MAX, "Marked packet count: %i", summary.marked_count);
  add_string_to_box(string_buff, data_box);

  /* Packets per second */
  if (seconds > 0){
    snprintf(string_buff, SUM_STR_MAX, "Avg. packets/sec: %.3f", summary.packet_count/seconds);
    add_string_to_box(string_buff, data_box);
  }

  /* Packet size */
  if (summary.packet_count > 0){
    snprintf(string_buff, SUM_STR_MAX, "Avg. packet size: %.3f bytes",
      (float)summary.bytes/summary.packet_count);
    add_string_to_box(string_buff, data_box);
  }

  /* Dropped count */
  if (summary.drops_known) {
    snprintf(string_buff, SUM_STR_MAX, "Dropped packets: %u", summary.drops);
    add_string_to_box(string_buff, data_box);
  }

  /* Byte count */
  snprintf(string_buff, SUM_STR_MAX, "Bytes of traffic: %d", summary.bytes);
  add_string_to_box(string_buff, data_box);

  /* Bytes per second */
  if (seconds > 0){
    snprintf(string_buff, SUM_STR_MAX, "Avg. bytes/sec: %.3f", summary.bytes/seconds);
    add_string_to_box(string_buff, data_box);

    /* MBit per second */
    snprintf(string_buff, SUM_STR_MAX, "Avg. Mbit/sec: %.3f",
             summary.bytes * 8.0 / (seconds * 1000.0 * 1000.0));
    add_string_to_box(string_buff, data_box);
  }

  /* Filtered packets frame */
  filter_fr = gtk_frame_new("Data in filtered packets");
  gtk_container_add(GTK_CONTAINER(main_vb), filter_fr);
  gtk_widget_show(filter_fr);

  filter_box = gtk_vbox_new( FALSE, 3);
  gtk_container_border_width(GTK_CONTAINER(filter_box), 5);
  gtk_container_add(GTK_CONTAINER(filter_fr), filter_box);
  gtk_widget_show(filter_box);

  if (summary.dfilter) {
    double seconds;

    /* Display filter */
    /* limit each row to some reasonable length */
    str_dup = g_strdup_printf("Display filter: %s", summary.dfilter);
    str_work = g_strdup(str_dup);
    offset = 0;
    while(strlen(str_work) > 100) {
        str_work[100] = '\0';
        add_string_to_box(str_work, filter_box);
        g_free(str_work);
        offset+=100;
        str_work = g_strdup(&str_dup[offset]);
    }
    
    add_string_to_box(str_work, filter_box);
    g_free(str_work);
    g_free(str_dup);

    /* seconds */
    seconds = (summary.filtered_stop - summary.filtered_start);
    snprintf(string_buff, SUM_STR_MAX, "Between first and last packet: %.3f seconds", seconds);
    add_string_to_box(string_buff, filter_box);

    /* Packet count */
    snprintf(string_buff, SUM_STR_MAX, "Packet count: %i", summary.filtered_count);
    add_string_to_box(string_buff, filter_box);

    /* Packets per second */
    if (seconds > 0){
      snprintf(string_buff, SUM_STR_MAX, "Avg. packets/sec: %.3f", summary.filtered_count/seconds);
      add_string_to_box(string_buff, filter_box);
    }

    /* Packet size */
    if (summary.filtered_count > 0){
      snprintf(string_buff, SUM_STR_MAX, "Avg. packet size: %.3f bytes",
          (float) summary.filtered_bytes/summary.filtered_count);
      add_string_to_box(string_buff, filter_box);
    }

    /* Byte count */
    snprintf(string_buff, SUM_STR_MAX, "Bytes of traffic: %d", summary.filtered_bytes);
    add_string_to_box(string_buff, filter_box);

    /* Bytes per second */
    if (seconds > 0){
      snprintf(string_buff, SUM_STR_MAX, "Avg. bytes/sec: %.3f", summary.filtered_bytes/seconds);
      add_string_to_box(string_buff, filter_box);

      /* MBit per second */
      snprintf(string_buff, SUM_STR_MAX, "Avg. Mbit/sec: %.3f",
	       summary.filtered_bytes * 8.0 / (seconds * 1000.0 * 1000.0));
      add_string_to_box(string_buff, filter_box);
    }
  } else {
    /* Display filter */
    snprintf(string_buff, SUM_STR_MAX, "Display filter: none");
    add_string_to_box(string_buff, filter_box);
  }

  /* Capture Frame */
  capture_fr = gtk_frame_new("Capture");
  gtk_container_add(GTK_CONTAINER(main_vb), capture_fr);
  gtk_widget_show(capture_fr);

  capture_box = gtk_vbox_new(FALSE, 3);
  gtk_container_border_width(GTK_CONTAINER(capture_box), 5);
  gtk_container_add(GTK_CONTAINER(capture_fr), capture_box);
  gtk_widget_show(capture_box);

  /* interface */
  if (summary.iface) {
    snprintf(string_buff, SUM_STR_MAX, "Interface: %s", summary.iface);
  } else {
    sprintf(string_buff, "Interface: unknown");
  }
  add_string_to_box(string_buff, capture_box);

#ifdef HAVE_LIBPCAP
  /* Capture filter */
  if (summary.cfilter && summary.cfilter[0] != '\0') {
    snprintf(string_buff, SUM_STR_MAX, "Capture filter: %s", summary.cfilter);
  } else {
    sprintf(string_buff, "Capture filter: none");
  }
  add_string_to_box(string_buff, capture_box);
#endif

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

  gtk_window_set_position(GTK_WINDOW(sum_open_w), GTK_WIN_POS_MOUSE);
  gtk_widget_show(sum_open_w);
}
