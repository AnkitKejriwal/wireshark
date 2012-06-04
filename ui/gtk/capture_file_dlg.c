/* capture_file_dlg.c
 * Dialog boxes for handling capture files
 *
 * $Id$
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
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
#include "config.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <string.h>

#include <gtk/gtk.h>

#include "packet-range.h"
#include <epan/filesystem.h>
#include <epan/addr_resolv.h>
#include <epan/prefs.h>

#include "../globals.h"
#include "../color.h"
#include "../color_filters.h"
#include "../merge.h"
#include "ui/util.h"
#include <wsutil/file_util.h>

#include "ui/alert_box.h"
#include "ui/last_open_dir.h"
#include "ui/recent.h"
#include "ui/simple_dialog.h"
#include "ui/ui_util.h"

#include "ui/gtk/gtkglobals.h"
#include "ui/gtk/keys.h"
#include "ui/gtk/filter_dlg.h"
#include "ui/gtk/gui_utils.h"
#include "ui/gtk/dlg_utils.h"
#include "ui/gtk/file_dlg.h"
#include "ui/gtk/capture_file_dlg.h"
#include "ui/gtk/drag_and_drop.h"
#include "ui/gtk/main.h"
#include "ui/gtk/color_dlg.h"
#include "ui/gtk/new_packet_list.h"
#ifdef HAVE_LIBPCAP
#include "ui/gtk/capture_dlg.h"
#endif
#include "ui/gtk/stock_icons.h"
#include "ui/gtk/range_utils.h"
#include "ui/gtk/filter_autocomplete.h"

#if _WIN32
#include <gdk/gdkwin32.h>
#include <windows.h>
#include "ui/win32/file_dlg_win32.h"
#endif


static void file_open_ok_cb(GtkWidget *w, gpointer fs);
static void file_open_destroy_cb(GtkWidget *win, gpointer user_data);
static void file_merge_ok_cb(GtkWidget *w, gpointer fs);
static void file_merge_destroy_cb(GtkWidget *win, gpointer user_data);
static void do_file_save(capture_file *cf, gboolean dont_reopen);
static void do_file_save_as(capture_file *cf);
static void file_save_as_cb(GtkWidget *fs);
static void file_save_as_select_file_type_cb(GtkWidget *w, gpointer data);
static void file_save_as_destroy_cb(GtkWidget *win, gpointer user_data);
static void file_export_specified_packets_cb(GtkWidget *w, gpointer fs);
static void file_export_specified_packets_select_file_type_cb(GtkWidget *w, gpointer data);
static void file_export_specified_packets_ok_cb(GtkWidget *w, gpointer fs);
static void file_export_specified_packets_destroy_cb(GtkWidget *win, gpointer user_data);
static void file_color_import_ok_cb(GtkWidget *w, gpointer filter_list);
static void file_color_import_destroy_cb(GtkWidget *win, gpointer user_data);
static void file_color_export_ok_cb(GtkWidget *w, gpointer filter_list);
static void file_color_export_destroy_cb(GtkWidget *win, gpointer user_data);
static void set_file_type_list(GtkWidget *combo_box, capture_file *cf);

#define E_FILE_TYPE_COMBO_BOX_KEY "file_type_combo_box"
#define E_COMPRESSED_CB_KEY       "compressed_cb"

#define E_FILE_M_RESOLVE_KEY	  "file_dlg_mac_resolve_key"
#define E_FILE_N_RESOLVE_KEY	  "file_dlg_network_resolve_key"
#define E_FILE_T_RESOLVE_KEY	  "file_dlg_transport_resolve_key"

#define E_MERGE_PREPEND_KEY 	  "merge_dlg_prepend_key"
#define E_MERGE_CHRONO_KEY 	      "merge_dlg_chrono_key"
#define E_MERGE_APPEND_KEY 	      "merge_dlg_append_key"


#define PREVIEW_TABLE_KEY       "preview_table_key"
#define PREVIEW_FILENAME_KEY    "preview_filename_key"
#define PREVIEW_FORMAT_KEY      "preview_format_key"
#define PREVIEW_SIZE_KEY        "preview_size_key"
#define PREVIEW_ELAPSED_KEY     "preview_elapsed_key"
#define PREVIEW_PACKETS_KEY     "preview_packets_key"
#define PREVIEW_FIRST_KEY       "preview_first_key"


/*
 * Keep a static pointer to the current "Save Capture File As" window, if
 * any, so that if somebody tries to do "File:Save" or "File:Save As"
 * while there's already a "Save Capture File As" window up, we just pop
 * up the existing one, rather than creating a new one.
 */
static GtkWidget *file_save_as_w;

/*
 * Keep a static pointer to the current "Export Specified Packets" window, if
 * any, so that if somebody tries to do "File:Export Specified Packets"
 * while there's already a "Export Specified Packets" window up, we just pop
 * up the existing one, rather than creating a new one.
 */
static GtkWidget *file_export_specified_packets_w;

/* XXX - can we make these not be static? */
static packet_range_t  range;
static gboolean        color_selected;
static GtkWidget      *range_tb;

#define PREVIEW_STR_MAX         200


/* set a new filename for the preview widget */
static wtap *
preview_set_filename(GtkWidget *prev, const gchar *cf_name)
{
    GtkWidget  *label;
    gchar      *display_basename;
    wtap       *wth;
    int         err = 0;
    gchar      *err_info;
    gchar       string_buff[PREVIEW_STR_MAX];
    gint64      filesize;


    /* init preview labels */
    label = (GtkWidget *)g_object_get_data(G_OBJECT(prev), PREVIEW_FILENAME_KEY);
    gtk_label_set_text(GTK_LABEL(label), "-");
    label = (GtkWidget *)g_object_get_data(G_OBJECT(prev), PREVIEW_FORMAT_KEY);
    gtk_label_set_text(GTK_LABEL(label), "-");
    label = (GtkWidget *)g_object_get_data(G_OBJECT(prev), PREVIEW_SIZE_KEY);
    gtk_label_set_text(GTK_LABEL(label), "-");
    label = (GtkWidget *)g_object_get_data(G_OBJECT(prev), PREVIEW_ELAPSED_KEY);
    gtk_label_set_text(GTK_LABEL(label), "-");
    label = (GtkWidget *)g_object_get_data(G_OBJECT(prev), PREVIEW_PACKETS_KEY);
    gtk_label_set_text(GTK_LABEL(label), "-");
    label = (GtkWidget *)g_object_get_data(G_OBJECT(prev), PREVIEW_FIRST_KEY);
    gtk_label_set_text(GTK_LABEL(label), "-");

    if(!cf_name) {
        return NULL;
    }

    label = (GtkWidget *)g_object_get_data(G_OBJECT(prev), PREVIEW_FILENAME_KEY);
    display_basename = g_filename_display_basename(cf_name);
    gtk_label_set_text(GTK_LABEL(label), display_basename);
    g_free(display_basename);

    if (test_for_directory(cf_name) == EISDIR) {
        label = (GtkWidget *)g_object_get_data(G_OBJECT(prev), PREVIEW_FORMAT_KEY);
        gtk_label_set_text(GTK_LABEL(label), "directory");
        return NULL;
    }

    wth = wtap_open_offline(cf_name, &err, &err_info, TRUE);
    if (wth == NULL) {
        label = (GtkWidget *)g_object_get_data(G_OBJECT(prev), PREVIEW_FORMAT_KEY);
        if(err == WTAP_ERR_FILE_UNKNOWN_FORMAT) {
            gtk_label_set_text(GTK_LABEL(label), "unknown file format");
        } else {
            gtk_label_set_text(GTK_LABEL(label), "error opening file");
        }
        return NULL;
    }

    /* Find the size of the file. */
    filesize = wtap_file_size(wth, &err);
    if (filesize == -1) {
        gtk_label_set_text(GTK_LABEL(label), "error getting file size");
        wtap_close(wth);
        return NULL;
    }
    g_snprintf(string_buff, PREVIEW_STR_MAX, "%" G_GINT64_MODIFIER "d bytes", filesize);
    label = (GtkWidget *)g_object_get_data(G_OBJECT(prev), PREVIEW_SIZE_KEY);
    gtk_label_set_text(GTK_LABEL(label), string_buff);

    /* type */
    g_strlcpy(string_buff, wtap_file_type_string(wtap_file_type(wth)), PREVIEW_STR_MAX);
    label = (GtkWidget *)g_object_get_data(G_OBJECT(prev), PREVIEW_FORMAT_KEY);
    gtk_label_set_text(GTK_LABEL(label), string_buff);

    return wth;
}


/* do a preview run on the currently selected capture file */
static void
preview_do(GtkWidget *prev, wtap *wth)
{
    GtkWidget  *label;
    unsigned int elapsed_time;
    time_t      time_preview;
    time_t      time_current;
    int         err = 0;
    gchar      *err_info;
    gint64      data_offset;
    const struct wtap_pkthdr *phdr;
    double      start_time = 0;	/* seconds, with nsec resolution */
    double      stop_time = 0;	/* seconds, with nsec resolution */
    double      cur_time;
    unsigned int packets = 0;
    gboolean    is_breaked = FALSE;
    gchar       string_buff[PREVIEW_STR_MAX];
    time_t      ti_time;
    struct tm  *ti_tm;


    time(&time_preview);
    while ( (wtap_read(wth, &err, &err_info, &data_offset)) ) {
        phdr = wtap_phdr(wth);
        cur_time = wtap_nstime_to_sec(&phdr->ts);
        if(packets == 0) {
            start_time 	= cur_time;
            stop_time = cur_time;
        }
        if (cur_time < start_time) {
            start_time = cur_time;
        }
        if (cur_time > stop_time){
            stop_time = cur_time;
        }

        packets++;
        if(packets%1000 == 0) {
            /* do we have a timeout? */
            time(&time_current);
            if(time_current-time_preview >= (time_t) prefs.gui_fileopen_preview) {
                is_breaked = TRUE;
                break;
            }
        }
    }

    if(err != 0) {
        g_snprintf(string_buff, PREVIEW_STR_MAX, "error after reading %u packets", packets);
        label = (GtkWidget *)g_object_get_data(G_OBJECT(prev), PREVIEW_PACKETS_KEY);
        gtk_label_set_text(GTK_LABEL(label), string_buff);
        wtap_close(wth);
        return;
    }

    /* packet count */
    if(is_breaked) {
        g_snprintf(string_buff, PREVIEW_STR_MAX, "more than %u packets (preview timeout)", packets);
    } else {
        g_snprintf(string_buff, PREVIEW_STR_MAX, "%u", packets);
    }
    label = g_object_get_data(G_OBJECT(prev), PREVIEW_PACKETS_KEY);
    gtk_label_set_text(GTK_LABEL(label), string_buff);

    /* first packet */
    ti_time = (long)start_time;
    ti_tm = localtime( &ti_time );
	if(ti_tm) {
		g_snprintf(string_buff, PREVIEW_STR_MAX,
				 "%04d-%02d-%02d %02d:%02d:%02d",
				 ti_tm->tm_year + 1900,
				 ti_tm->tm_mon + 1,
				 ti_tm->tm_mday,
				 ti_tm->tm_hour,
				 ti_tm->tm_min,
				 ti_tm->tm_sec);
	} else {
		g_snprintf(string_buff, PREVIEW_STR_MAX, "?");
	}
        label = g_object_get_data(G_OBJECT(prev), PREVIEW_FIRST_KEY);
    gtk_label_set_text(GTK_LABEL(label), string_buff);

    /* elapsed time */
    elapsed_time = (unsigned int)(stop_time-start_time);
    if(elapsed_time/86400) {
      g_snprintf(string_buff, PREVIEW_STR_MAX, "%02u days %02u:%02u:%02u",
        elapsed_time/86400, elapsed_time%86400/3600, elapsed_time%3600/60, elapsed_time%60);
    } else {
      g_snprintf(string_buff, PREVIEW_STR_MAX, "%02u:%02u:%02u",
        elapsed_time%86400/3600, elapsed_time%3600/60, elapsed_time%60);
    }
    if(is_breaked) {
      g_snprintf(string_buff, PREVIEW_STR_MAX, "unknown");
    }
    label = (GtkWidget *)g_object_get_data(G_OBJECT(prev), PREVIEW_ELAPSED_KEY);
    gtk_label_set_text(GTK_LABEL(label), string_buff);

    wtap_close(wth);
}

#if 0
/* as the dialog layout will look very ugly when using the file chooser preview mechanism,
   simply use the same layout as in GTK2.0 */
static void
update_preview_cb (GtkFileChooser *file_chooser, gpointer data)
{
    GtkWidget *prev = GTK_WIDGET (data);
    char *cf_name;
    gboolean have_preview;

    cf_name = gtk_file_chooser_get_preview_filename (file_chooser);

    have_preview = preview_set_filename(prev, cf_name);

    g_free (cf_name);

    have_preview = TRUE;
    gtk_file_chooser_set_preview_widget_active (file_chooser, have_preview);
}
#endif


/* the filename text entry changed */
static void
file_open_entry_changed(GtkWidget *w _U_, gpointer file_sel)
{
    GtkWidget *prev = (GtkWidget *)g_object_get_data(G_OBJECT(file_sel), PREVIEW_TABLE_KEY);
    gchar *cf_name;
    gboolean have_preview;
    wtap       *wth;

    /* get the filename */
    cf_name = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_sel));

    /* set the filename to the preview */
    wth = preview_set_filename(prev, cf_name);
    have_preview = (wth != NULL);

    g_free(cf_name);

    /* make the preview widget sensitive */
    gtk_widget_set_sensitive(prev, have_preview);

    /*
     * XXX - if the Open button isn't sensitive, you can't type into
     * the location bar and select the file or directory you've typed.
     * See
     *
     *	https://bugs.wireshark.org/bugzilla/show_bug.cgi?id=1791
     *
     * It's not as if allowing users to click Open when they've
     * selected a file that's not a valid capture file will cause
     * anything worse than an error dialog, so we'll leave the Open
     * button sensitive for now.  Perhaps making it sensitive if
     * cf_name is NULL would also work, although I don't know whether
     * there are any cases where it would be non-null when you've
     * typed in the location bar.
     *
     * XXX - Bug 1791 also notes that, with the line removed, Bill
     * Meier "somehow managed to get the file chooser window somewhat
     * wedged in that neither the cancel or open buttons were responsive".
     * That seems a bit odd, given that, without this line, we're not
     * monkeying with the Open button's sensitivity, but...
     */
#if 0
    /* make the open/save/... dialog button sensitive */

    gtk_dialog_set_response_sensitive(file_sel, GTK_RESPONSE_ACCEPT, have_preview);
#endif

    /* do the actual preview */
    if(have_preview)
        preview_do(prev, wth);
}


/* copied from summary_dlg.c */
static GtkWidget *
add_string_to_table_sensitive(GtkWidget *list, guint *row, const gchar *title, const gchar *value, gboolean sensitive)
{
    GtkWidget *label;
    gchar     *indent;

    if(strlen(value) != 0) {
        indent = g_strdup_printf("   %s", title);
    } else {
        indent = g_strdup(title);
    }
    label = gtk_label_new(indent);
    g_free(indent);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_widget_set_sensitive(label, sensitive);
    gtk_table_attach_defaults(GTK_TABLE(list), label, 0, 1, *row, *row+1);

    label = gtk_label_new(value);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_widget_set_sensitive(label, sensitive);
    gtk_table_attach_defaults(GTK_TABLE(list), label, 1, 2, *row, *row+1);

    *row = *row + 1;

    return label;
}

static GtkWidget *
add_string_to_table(GtkWidget *list, guint *row, const gchar *title, const gchar *value)
{
    return add_string_to_table_sensitive(list, row, title, value, TRUE);
}



static GtkWidget *
preview_new(void)
{
    GtkWidget *table, *label;
    guint         row;

    table = gtk_table_new(1, 2, FALSE);
    gtk_table_set_col_spacings(GTK_TABLE(table), 6);
    gtk_table_set_row_spacings(GTK_TABLE(table), 3);
    row = 0;

    label = add_string_to_table(table, &row, "Filename:", "-");
    gtk_widget_set_size_request(label, DEF_WIDTH/3, -1);
    g_object_set_data(G_OBJECT(table), PREVIEW_FILENAME_KEY, label);
    label = add_string_to_table(table, &row, "Format:", "-");
    g_object_set_data(G_OBJECT(table), PREVIEW_FORMAT_KEY, label);
    label = add_string_to_table(table, &row, "Size:", "-");
    g_object_set_data(G_OBJECT(table), PREVIEW_SIZE_KEY, label);
    label = add_string_to_table(table, &row, "Packets:", "-");
    g_object_set_data(G_OBJECT(table), PREVIEW_PACKETS_KEY, label);
    label = add_string_to_table(table, &row, "First Packet:", "-");
    g_object_set_data(G_OBJECT(table), PREVIEW_FIRST_KEY, label);
    label = add_string_to_table(table, &row, "Elapsed time:", "-");
    g_object_set_data(G_OBJECT(table), PREVIEW_ELAPSED_KEY, label);

    return table;
}

/*
 * Keep a static pointer to the current "Open Capture File" window, if
 * any, so that if somebody tries to do "File:Open" while there's already
 * an "Open Capture File" window up, we just pop up the existing one,
 * rather than creating a new one.
 */
static GtkWidget *file_open_w;

/* Open a file */
static void
file_open_cmd(GtkWidget *w)
{
#if _WIN32
  win32_open_file(GDK_WINDOW_HWND(gtk_widget_get_window(top_level)));
#else /* _WIN32 */
  GtkWidget	*main_hb, *main_vb, *filter_hbox, *filter_bt, *filter_te,
  		*m_resolv_cb, *n_resolv_cb, *t_resolv_cb, *prev;
  /* No Apply button, and "OK" just sets our text widget, it doesn't
     activate it (i.e., it doesn't cause us to try to open the file). */
  static construct_args_t args = {
  	"Wireshark: Read Filter",
  	FALSE,
  	FALSE,
    TRUE
  };

  if (file_open_w != NULL) {
    /* There's already an "Open Capture File" dialog box; reactivate it. */
    reactivate_window(file_open_w);
    return;
  }

  file_open_w = file_selection_new("Wireshark: Open Capture File",
                                   FILE_SELECTION_OPEN);
  /* it's annoying, that the file chooser dialog is already shown here,
     so we cannot use the correct gtk_window_set_default_size() to resize it */
  gtk_widget_set_size_request(file_open_w, DEF_WIDTH, DEF_HEIGHT);

  switch (prefs.gui_fileopen_style) {

  case FO_STYLE_LAST_OPENED:
    /* The user has specified that we should start out in the last directory
       we looked in.  If we've already opened a file, use its containing
       directory, if we could determine it, as the directory, otherwise
       use the "last opened" directory saved in the preferences file if
       there was one. */
    /* This is now the default behaviour in file_selection_new() */
    break;

  case FO_STYLE_SPECIFIED:
    /* The user has specified that we should always start out in a
       specified directory; if they've specified that directory,
       start out by showing the files in that dir. */
    if (prefs.gui_fileopen_dir[0] != '\0')
      file_selection_set_current_folder(file_open_w, prefs.gui_fileopen_dir);
    break;
  }


  main_hb = ws_gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3, FALSE);
  file_selection_set_extra_widget(file_open_w, main_hb);
  gtk_widget_show(main_hb);

  /* Container for each row of widgets */
  main_vb = ws_gtk_box_new(GTK_ORIENTATION_VERTICAL, 3, FALSE);
  gtk_container_set_border_width(GTK_CONTAINER(main_vb), 5);
  gtk_box_pack_start(GTK_BOX(main_hb), main_vb, FALSE, FALSE, 0);
  gtk_widget_show(main_vb);

  /* filter row */
  filter_hbox = ws_gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1, FALSE);
  gtk_container_set_border_width(GTK_CONTAINER(filter_hbox), 0);
  gtk_box_pack_start(GTK_BOX(main_vb), filter_hbox, FALSE, FALSE, 0);
  gtk_widget_show(filter_hbox);

  filter_bt = gtk_button_new_from_stock(WIRESHARK_STOCK_DISPLAY_FILTER_ENTRY);
  g_signal_connect(filter_bt, "clicked",
                   G_CALLBACK(display_filter_construct_cb), &args);
  g_signal_connect(filter_bt, "destroy",
                   G_CALLBACK(filter_button_destroy_cb), NULL);
  gtk_box_pack_start(GTK_BOX(filter_hbox), filter_bt, FALSE, TRUE, 0);
  gtk_widget_show(filter_bt);
  gtk_widget_set_tooltip_text(filter_bt, "Open the \"Display Filter\" dialog, to edit/apply filters");

  filter_te = gtk_entry_new();
  g_object_set_data(G_OBJECT(filter_bt), E_FILT_TE_PTR_KEY, filter_te);
  gtk_box_pack_start(GTK_BOX(filter_hbox), filter_te, TRUE, TRUE, 3);
  g_signal_connect(filter_te, "changed",
                   G_CALLBACK(filter_te_syntax_check_cb), NULL);
  g_object_set_data(G_OBJECT(filter_hbox), E_FILT_AUTOCOMP_PTR_KEY, NULL);
  g_signal_connect(filter_te, "key-press-event", G_CALLBACK (filter_string_te_key_pressed_cb), NULL);
  g_signal_connect(file_open_w, "key-press-event", G_CALLBACK (filter_parent_dlg_key_pressed_cb), NULL);
  colorize_filter_te_as_empty(filter_te);
  gtk_widget_show(filter_te);
  gtk_widget_set_tooltip_text(filter_te, "Enter a display filter.");

  g_object_set_data(G_OBJECT(file_open_w), E_RFILTER_TE_KEY, filter_te);

  /* resolve buttons */
  m_resolv_cb = gtk_check_button_new_with_mnemonic("Enable _MAC name resolution");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m_resolv_cb),
	gbl_resolv_flags & RESOLV_MAC);
  gtk_box_pack_start(GTK_BOX(main_vb), m_resolv_cb, FALSE, FALSE, 0);
  g_object_set_data(G_OBJECT(file_open_w),
                  E_FILE_M_RESOLVE_KEY, m_resolv_cb);
  gtk_widget_show(m_resolv_cb);

  n_resolv_cb = gtk_check_button_new_with_mnemonic("Enable _network name resolution");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(n_resolv_cb),
	gbl_resolv_flags & RESOLV_NETWORK);
  gtk_box_pack_start(GTK_BOX(main_vb), n_resolv_cb, FALSE, FALSE, 0);
  gtk_widget_show(n_resolv_cb);
  g_object_set_data(G_OBJECT(file_open_w), E_FILE_N_RESOLVE_KEY, n_resolv_cb);
  t_resolv_cb = gtk_check_button_new_with_mnemonic("Enable _transport name resolution");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(t_resolv_cb),
	gbl_resolv_flags & RESOLV_TRANSPORT);
  gtk_box_pack_start(GTK_BOX(main_vb), t_resolv_cb, FALSE, FALSE, 0);
  gtk_widget_show(t_resolv_cb);
  g_object_set_data(G_OBJECT(file_open_w), E_FILE_T_RESOLVE_KEY, t_resolv_cb);

  g_signal_connect(file_open_w, "destroy",
                   G_CALLBACK(file_open_destroy_cb), NULL);

  /* preview widget */
  prev = preview_new();
  g_object_set_data(G_OBJECT(file_open_w), PREVIEW_TABLE_KEY, prev);
  gtk_widget_show_all(prev);
  gtk_box_pack_start(GTK_BOX(main_hb), prev, TRUE, TRUE, 0);

  g_signal_connect(GTK_FILE_CHOOSER(file_open_w), "selection-changed",
                   G_CALLBACK(file_open_entry_changed), file_open_w);
  file_open_entry_changed(file_open_w, file_open_w);

  g_object_set_data(G_OBJECT(file_open_w), E_DFILTER_TE_KEY,
                    g_object_get_data(G_OBJECT(w), E_DFILTER_TE_KEY));
  if (gtk_dialog_run(GTK_DIALOG(file_open_w)) == GTK_RESPONSE_ACCEPT)
  {
    file_open_ok_cb(file_open_w, file_open_w);
  }
  else window_destroy(file_open_w);
#endif /* _WIN32 */
}

void
file_open_cmd_cb(GtkWidget *widget, gpointer data _U_) {
  /* If there's unsaved data, let the user save it first.
     If they cancel out of it, don't quit. */
  if (do_file_close(&cfile, FALSE, " before opening a new capture file"))
    file_open_cmd(widget);
}

/* user pressed "open" button */
static void
file_open_ok_cb(GtkWidget *w, gpointer fs) {
  gchar       *cf_name, *s;
  const gchar *rfilter;
  GtkWidget   *filter_te, *m_resolv_cb, *n_resolv_cb, *t_resolv_cb;
  dfilter_t   *rfcode = NULL;
  int          err;

  cf_name = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fs));
  filter_te = (GtkWidget *)g_object_get_data(G_OBJECT(w), E_RFILTER_TE_KEY);
  rfilter = gtk_entry_get_text(GTK_ENTRY(filter_te));
  if (!dfilter_compile(rfilter, &rfcode)) {
    bad_dfilter_alert_box(rfilter);
    g_free(cf_name);
    return;
  }

  /* Perhaps the user specified a directory instead of a file.
     Check whether they did. */
  if (test_for_directory(cf_name) == EISDIR) {
    /* It's a directory - set the file selection box to display that
       directory, don't try to open the directory as a capture file. */
    set_last_open_dir(cf_name);
    g_free(cf_name);
    file_selection_set_current_folder(fs, get_last_open_dir());
    return;
  }

  /* Try to open the capture file. */
  if (cf_open(&cfile, cf_name, FALSE, &err) != CF_OK) {
    /* We couldn't open it; don't dismiss the open dialog box,
       just leave it around so that the user can, after they
       dismiss the alert box popped up for the open error,
       try again. */
    if (rfcode != NULL)
      dfilter_free(rfcode);
    g_free(cf_name);

    /* XXX - as we cannot start a new event loop (using gtk_dialog_run()),
     * as this will prevent the user from closing the now existing error
     * message, simply close the dialog (this is the best we can do here). */
    if (file_open_w)
      window_destroy(file_open_w);

    return;
  }

  /* Attach the new read filter to "cf" ("cf_open()" succeeded, so
     it closed the previous capture file, and thus destroyed any
     previous read filter attached to "cf"). */
  cfile.rfcode = rfcode;

  /* Set the global resolving variable */
  gbl_resolv_flags = prefs.name_resolve;
  m_resolv_cb = (GtkWidget *)g_object_get_data(G_OBJECT(w), E_FILE_M_RESOLVE_KEY);
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (m_resolv_cb)))
    gbl_resolv_flags |= RESOLV_MAC;
  else
    gbl_resolv_flags &= ~RESOLV_MAC;
  n_resolv_cb = (GtkWidget *)g_object_get_data(G_OBJECT(w), E_FILE_N_RESOLVE_KEY);
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (n_resolv_cb)))
    gbl_resolv_flags |= RESOLV_NETWORK;
  else
    gbl_resolv_flags &= ~RESOLV_NETWORK;
  t_resolv_cb = (GtkWidget *)g_object_get_data(G_OBJECT(w), E_FILE_T_RESOLVE_KEY);
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (t_resolv_cb)))
    gbl_resolv_flags |= RESOLV_TRANSPORT;
  else
    gbl_resolv_flags &= ~RESOLV_TRANSPORT;

  /* We've crossed the Rubicon; get rid of the file selection box. */
  window_destroy(GTK_WIDGET (fs));

  switch (cf_read(&cfile, FALSE)) {

  case CF_READ_OK:
  case CF_READ_ERROR:
    /* Just because we got an error, that doesn't mean we were unable
       to read any of the file; we handle what we could get from the
       file. */
    break;

  case CF_READ_ABORTED:
    /* The user bailed out of re-reading the capture file; the
       capture file has been closed - just free the capture file name
       string and return (without changing the last containing
       directory). */
    g_free(cf_name);
    return;
  }

  /* Save the name of the containing directory specified in the path name,
     if any; we can write over cf_name, which is a good thing, given that
     "get_dirname()" does write over its argument. */
  s = get_dirname(cf_name);
  set_last_open_dir(s);

  g_free(cf_name);
}

static void
file_open_destroy_cb(GtkWidget *win _U_, gpointer user_data _U_)
{
  /* Note that we no longer have a "Open Capture File" dialog box. */
  file_open_w = NULL;
}

/*
 * Keep a static pointer to the current "Merge Capture File" window, if
 * any, so that if somebody tries to do "File:Merge" while there's already
 * an "Merge Capture File" window up, we just pop up the existing one,
 * rather than creating a new one.
 */
static GtkWidget *file_merge_w;

/* Merge existing with another file */
static void
file_merge_cmd(GtkWidget *w)
{
#if _WIN32
  win32_merge_file(GDK_WINDOW_HWND(gtk_widget_get_window(top_level)));
  new_packet_list_freeze();
  new_packet_list_thaw();
#else /* _WIN32 */
  GtkWidget	*main_hb, *main_vb, *ft_hb, *ft_lb, *ft_combo_box, *filter_hbox,
		*filter_bt, *filter_te, *prepend_rb, *chrono_rb,
		*append_rb, *prev;

  /* No Apply button, and "OK" just sets our text widget, it doesn't
     activate it (i.e., it doesn't cause us to try to open the file). */
  static construct_args_t args = {
    "Wireshark: Read Filter",
    FALSE,
    FALSE,
    TRUE
  };

  if (file_merge_w != NULL) {
    /* There's already an "Merge Capture File" dialog box; reactivate it. */
    reactivate_window(file_merge_w);
    return;
  }

  /* Default to saving all packets, in the file's current format. */

  file_merge_w = file_selection_new("Wireshark: Merge with Capture File",
                                   FILE_SELECTION_OPEN);
  /* it's annoying, that the file chooser dialog is already shown here,
     so we cannot use the correct gtk_window_set_default_size() to resize it */
  gtk_widget_set_size_request(file_merge_w, DEF_WIDTH, DEF_HEIGHT);

  switch (prefs.gui_fileopen_style) {

  case FO_STYLE_LAST_OPENED:
    /* The user has specified that we should start out in the last directory
       we looked in.  If we've already opened a file, use its containing
       directory, if we could determine it, as the directory, otherwise
       use the "last opened" directory saved in the preferences file if
       there was one. */
    /* This is now the default behaviour in file_selection_new() */
    break;

  case FO_STYLE_SPECIFIED:
    /* The user has specified that we should always start out in a
       specified directory; if they've specified that directory,
       start out by showing the files in that dir. */
    if (prefs.gui_fileopen_dir[0] != '\0')
      file_selection_set_current_folder(file_merge_w, prefs.gui_fileopen_dir);
    break;
  }

  main_hb = ws_gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3, FALSE);
  file_selection_set_extra_widget(file_merge_w, main_hb);
  gtk_widget_show(main_hb);

  /* Container for each row of widgets */
  main_vb = ws_gtk_box_new(GTK_ORIENTATION_VERTICAL, 3, FALSE);
  gtk_container_set_border_width(GTK_CONTAINER(main_vb), 5);
  gtk_box_pack_start(GTK_BOX(main_hb), main_vb, FALSE, FALSE, 0);
  gtk_widget_show(main_vb);

  /* File type row */
  range_tb = NULL;
  ft_hb = ws_gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3, FALSE);
  gtk_container_add(GTK_CONTAINER(main_vb), ft_hb);
  gtk_widget_show(ft_hb);

  ft_lb = gtk_label_new("Merged output file type:");
  gtk_box_pack_start(GTK_BOX(ft_hb), ft_lb, FALSE, FALSE, 0);
  gtk_widget_show(ft_lb);

  ft_combo_box = ws_combo_box_new_text_and_pointer();

  /* Generate the list of file types we can save. */
  set_file_type_list(ft_combo_box, &cfile);
  gtk_box_pack_start(GTK_BOX(ft_hb), ft_combo_box, FALSE, FALSE, 0);
  gtk_widget_show(ft_combo_box);
  g_object_set_data(G_OBJECT(file_merge_w), E_FILE_TYPE_COMBO_BOX_KEY, ft_combo_box);
  ws_combo_box_set_active(GTK_COMBO_BOX(ft_combo_box), 0); /* No callback */

  filter_hbox = ws_gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1, FALSE);
  gtk_container_set_border_width(GTK_CONTAINER(filter_hbox), 0);
  gtk_box_pack_start(GTK_BOX(main_vb), filter_hbox, FALSE, FALSE, 0);
  gtk_widget_show(filter_hbox);

  filter_bt = gtk_button_new_from_stock(WIRESHARK_STOCK_DISPLAY_FILTER_ENTRY);
  g_signal_connect(filter_bt, "clicked",
                   G_CALLBACK(display_filter_construct_cb), &args);
  g_signal_connect(filter_bt, "destroy",
                   G_CALLBACK(filter_button_destroy_cb), NULL);
  gtk_box_pack_start(GTK_BOX(filter_hbox), filter_bt, FALSE, TRUE, 0);
  gtk_widget_show(filter_bt);
  gtk_widget_set_tooltip_text(filter_bt, "Open the \"Display Filter\" dialog, to edit/apply filters");

  filter_te = gtk_entry_new();
  g_object_set_data(G_OBJECT(filter_bt), E_FILT_TE_PTR_KEY, filter_te);
  gtk_box_pack_start(GTK_BOX(filter_hbox), filter_te, TRUE, TRUE, 3);
  g_signal_connect(filter_te, "changed",
                   G_CALLBACK(filter_te_syntax_check_cb), NULL);
  g_object_set_data(G_OBJECT(filter_hbox), E_FILT_AUTOCOMP_PTR_KEY, NULL);
  g_signal_connect(filter_te, "key-press-event", G_CALLBACK (filter_string_te_key_pressed_cb), NULL);
  g_signal_connect(file_merge_w, "key-press-event", G_CALLBACK (filter_parent_dlg_key_pressed_cb), NULL);
  colorize_filter_te_as_empty(filter_te);
  gtk_widget_show(filter_te);
  gtk_widget_set_tooltip_text(filter_te, "Enter a display filter.");

  g_object_set_data(G_OBJECT(file_merge_w), E_RFILTER_TE_KEY, filter_te);

  prepend_rb = gtk_radio_button_new_with_mnemonic_from_widget(NULL,
      "Prepend packets to existing file");
  gtk_widget_set_tooltip_text(prepend_rb, "The resulting file contains the packets from the selected, followed by the packets from the currently loaded file, the packet timestamps will be ignored.");
  gtk_box_pack_start(GTK_BOX(main_vb), prepend_rb, FALSE, FALSE, 0);
  g_object_set_data(G_OBJECT(file_merge_w),
                  E_MERGE_PREPEND_KEY, prepend_rb);
  gtk_widget_show(prepend_rb);

  chrono_rb = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(prepend_rb), "Merge packets chronologically");
  gtk_widget_set_tooltip_text(chrono_rb, "The resulting file contains all the packets from the currently loaded and the selected file, sorted by the packet timestamps.");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chrono_rb), TRUE);
  gtk_box_pack_start(GTK_BOX(main_vb), chrono_rb, FALSE, FALSE, 0);
  gtk_widget_show(chrono_rb);
  g_object_set_data(G_OBJECT(file_merge_w), E_MERGE_CHRONO_KEY, chrono_rb);

  append_rb = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(prepend_rb), "Append packets to existing file");
  gtk_widget_set_tooltip_text(append_rb, "The resulting file contains the packets from the currently loaded, followed by the packets from the selected file, the packet timestamps will be ignored.");
  gtk_box_pack_start(GTK_BOX(main_vb), append_rb, FALSE, FALSE, 0);
  gtk_widget_show(append_rb);
  g_object_set_data(G_OBJECT(file_merge_w), E_MERGE_APPEND_KEY, append_rb);

  g_signal_connect(file_merge_w, "destroy",
                   G_CALLBACK(file_merge_destroy_cb), NULL);

  /* preview widget */
  prev = preview_new();
  g_object_set_data(G_OBJECT(file_merge_w), PREVIEW_TABLE_KEY, prev);
  gtk_widget_show_all(prev);
  gtk_box_pack_start(GTK_BOX(main_hb), prev, TRUE, TRUE, 0);

  g_signal_connect(GTK_FILE_CHOOSER(file_merge_w), "selection-changed",
                   G_CALLBACK(file_open_entry_changed), file_merge_w);
  file_open_entry_changed(file_merge_w, file_merge_w);

  g_object_set_data(G_OBJECT(file_merge_w), E_DFILTER_TE_KEY,
                    g_object_get_data(G_OBJECT(w), E_DFILTER_TE_KEY));
  if (gtk_dialog_run(GTK_DIALOG(file_merge_w)) == GTK_RESPONSE_ACCEPT)
  {
    file_merge_ok_cb(file_merge_w, file_merge_w);
  }
  else window_destroy(file_merge_w);
#endif /* _WIN32 */
}

void
file_merge_cmd_cb(GtkWidget *widget, gpointer data _U_) {
  /* If there's unsaved data, let the user save it first.
     If they cancel out of it, don't merge. */
  GtkWidget *msg_dialog;
  gchar     *display_basename;
  gint       response;

  if (prefs.gui_ask_unsaved) {
    if (cfile.is_tempfile || cfile.unsaved_changes) {
      /* This is a temporary capture file or has unsaved changes; ask the
         user whether to save the capture. */
      if (cfile.is_tempfile) {
        msg_dialog = gtk_message_dialog_new(GTK_WINDOW(top_level),
                                            GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
                                            GTK_MESSAGE_QUESTION,
                                            GTK_BUTTONS_NONE,
                                            "Do you want to save the captured packets before merging another capture file into it?");

        gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(msg_dialog),
             "A temporary capture file can't be merged.");
      } else {
        /*
         * Format the message.
         */
        display_basename = g_filename_display_basename(cfile.filename);
        msg_dialog = gtk_message_dialog_new(GTK_WINDOW(top_level),
                                            GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
                                            GTK_MESSAGE_QUESTION,
                                            GTK_BUTTONS_NONE,
                                            "Do you want to save the changes you've made "
                                            "to the capture file \"%s\" before merging another capture file into it?",
                                            display_basename);
        g_free(display_basename);
        gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(msg_dialog),
             "The changes must be saved before the files are merged.");
      }

#ifndef _WIN32
      gtk_dialog_add_button(GTK_DIALOG(msg_dialog),
                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
      gtk_dialog_add_button(GTK_DIALOG(msg_dialog),
                            GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT);
#else
      gtk_dialog_add_button(GTK_DIALOG(msg_dialog),
                            GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT);
      gtk_dialog_add_button(GTK_DIALOG(msg_dialog),
                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
#endif
      gtk_dialog_set_default_response(GTK_DIALOG(msg_dialog), GTK_RESPONSE_ACCEPT);

      response = gtk_dialog_run(GTK_DIALOG(msg_dialog));
      gtk_widget_destroy(msg_dialog);

      switch (response) {

      case GTK_RESPONSE_ACCEPT:
        /* Save the file but don't close it */
        do_file_save(&cfile, FALSE);
        break;

      case GTK_RESPONSE_CANCEL:
      case GTK_RESPONSE_NONE:
      case GTK_RESPONSE_DELETE_EVENT:
      default:
        /* Don't do the merge. */
        return;
      }
    }
  }

  /* Do the merge. */
  file_merge_cmd(widget);
}


static void
file_merge_ok_cb(GtkWidget *w, gpointer fs) {
  gchar       *cf_name, *s;
  const gchar *rfilter;
  GtkWidget   *ft_combo_box, *filter_te, *rb;
  dfilter_t   *rfcode = NULL;
  int          err;
  cf_status_t  merge_status;
  char        *in_filenames[2];
  char        *tmpname;
  gpointer     ptr;
  int          file_type;


  cf_name = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fs));
  filter_te = (GtkWidget *)g_object_get_data(G_OBJECT(w), E_RFILTER_TE_KEY);
  rfilter = gtk_entry_get_text(GTK_ENTRY(filter_te));
  if (!dfilter_compile(rfilter, &rfcode)) {
    bad_dfilter_alert_box(rfilter);
    g_free(cf_name);
    return;
  }

  ft_combo_box  = (GtkWidget *)g_object_get_data(G_OBJECT(w), E_FILE_TYPE_COMBO_BOX_KEY);
  if (! ws_combo_box_get_active_pointer(GTK_COMBO_BOX(ft_combo_box), &ptr)) {
      g_assert_not_reached();  /* Programming error: somehow nothing is active */
  }
  file_type = GPOINTER_TO_INT(ptr);

  /* Perhaps the user specified a directory instead of a file.
     Check whether they did. */
  if (test_for_directory(cf_name) == EISDIR) {
	/* It's a directory - set the file selection box to display that
	   directory, don't try to open the directory as a capture file. */
        set_last_open_dir(cf_name);
        g_free(cf_name);
        file_selection_set_current_folder(fs, get_last_open_dir());
    	return;
  }

  /* merge or append the two files */
  rb = (GtkWidget *)g_object_get_data(G_OBJECT(w), E_MERGE_CHRONO_KEY);
  tmpname = NULL;
  if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (rb))) {
      /* chronological order */
      in_filenames[0] = cfile.filename;
      in_filenames[1] = cf_name;
      merge_status = cf_merge_files(&tmpname, 2, in_filenames, file_type, FALSE);
  } else {
      rb = (GtkWidget *)g_object_get_data(G_OBJECT(w), E_MERGE_PREPEND_KEY);
      if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (rb))) {
          /* prepend file */
          in_filenames[0] = cf_name;
          in_filenames[1] = cfile.filename;
          merge_status = cf_merge_files(&tmpname, 2, in_filenames, file_type,
                                        TRUE);
      } else {
          /* append file */
          in_filenames[0] = cfile.filename;
          in_filenames[1] = cf_name;
          merge_status = cf_merge_files(&tmpname, 2, in_filenames, file_type,
                                        TRUE);
      }
  }

  g_free(cf_name);

  if (merge_status != CF_OK) {
    if (rfcode != NULL)
      dfilter_free(rfcode);
    g_free(tmpname);
    return;
  }

  cf_close(&cfile);

  /* We've crossed the Rubicon; get rid of the file selection box. */
  window_destroy(GTK_WIDGET (fs));

  /* Try to open the merged capture file. */
  if (cf_open(&cfile, tmpname, TRUE /* temporary file */, &err) != CF_OK) {
    /* We couldn't open it; don't dismiss the open dialog box,
       just leave it around so that the user can, after they
       dismiss the alert box popped up for the open error,
       try again. */
    if (rfcode != NULL)
      dfilter_free(rfcode);
    g_free(tmpname);
    /* XXX - as we cannot start a new event loop (using gtk_dialog_run()),
     * as this will prevent the user from closing the now existing error
     * message, simply close the dialog (this is the best we can do here). */
    if (file_open_w)
      window_destroy(file_open_w);
    return;
  }
  g_free(tmpname);

  /* Attach the new read filter to "cf" ("cf_open()" succeeded, so
     it closed the previous capture file, and thus destroyed any
     previous read filter attached to "cf"). */
  cfile.rfcode = rfcode;

  switch (cf_read(&cfile, FALSE)) {

  case CF_READ_OK:
  case CF_READ_ERROR:
    /* Just because we got an error, that doesn't mean we were unable
       to read any of the file; we handle what we could get from the
       file. */
    break;

  case CF_READ_ABORTED:
    /* The user bailed out of re-reading the capture file; the
       capture file has been closed - just free the capture file name
       string and return (without changing the last containing
       directory). */
    return;
  }

  /* Save the name of the containing directory specified in the path name,
     if any; we can write over cf_merged_name, which is a good thing, given that
     "get_dirname()" does write over its argument. */
  s = get_dirname(tmpname);
  set_last_open_dir(s);
}

static void
file_merge_destroy_cb(GtkWidget *win _U_, gpointer user_data _U_)
{
  /* Note that we no longer have a "Merge Capture File" dialog box. */
  file_merge_w = NULL;
}

gboolean
do_file_close(capture_file *cf, gboolean from_quit, const char *before_what)
{
  GtkWidget *msg_dialog;
  gchar     *display_basename;
  gint       response;
  gboolean   capture_in_progress;

  if (cf->state == FILE_CLOSED)
    return TRUE; /* already closed, nothing to do */

#ifdef HAVE_LIBPCAP
  if (cf->state == FILE_READ_IN_PROGRESS) {
    /* This is true if we're reading a capture file *or* if we're doing
       a live capture.  If we're reading a capture file, the main loop
       is busy reading packets, and only accepting input from the
       progress dialog, so we can't get here, so this means we're
       doing a capture. */
    capture_in_progress = TRUE;
  } else
#endif
    capture_in_progress = FALSE;
       
  if (prefs.gui_ask_unsaved) {
    if (cf->is_tempfile || capture_in_progress || cf->unsaved_changes) {
      /* This is a temporary capture file, or there's a capture in
         progress, or the file has unsaved changes; ask the user whether
         to save the data. */
      if (cf->is_tempfile) {
        msg_dialog = gtk_message_dialog_new(GTK_WINDOW(top_level),
                                            GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
                                            GTK_MESSAGE_QUESTION,
                                            GTK_BUTTONS_NONE,
                                            capture_in_progress ? 
                                                "Do you want to stop the capture and save the captured packets%s?" :
                                                "Do you want to save the captured packets%s?",
                                            before_what);

        gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(msg_dialog),
             "Your captured packets will be lost if you don't save them.");
      } else {
        /*
         * Format the message.
         */
        display_basename = g_filename_display_basename(cf->filename);
        if (capture_in_progress) {
          msg_dialog = gtk_message_dialog_new(GTK_WINDOW(top_level),
                                              GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
                                              GTK_MESSAGE_QUESTION,
                                              GTK_BUTTONS_NONE,
                                              "Do you want to stop the capture and save the captured packets%s?",
                                              before_what);
        } else {
          msg_dialog = gtk_message_dialog_new(GTK_WINDOW(top_level),
                                              GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
                                              GTK_MESSAGE_QUESTION,
                                              GTK_BUTTONS_NONE,
                                              "Do you want to save the changes you've made "
                                              "to the capture file \"%s\"%s?",
                                              display_basename, before_what);
        }
        g_free(display_basename);
        gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(msg_dialog),
                                                 capture_in_progress ?
             "Your captured packets will be lost if you don't save them." :
             "Your changes will be lost if you don't save them.");
      }

#ifndef _WIN32
      /* If this is from a Quit operation, use "quit and don't save"
         rather than just "don't save". */
      gtk_dialog_add_button(GTK_DIALOG(msg_dialog),
                            (from_quit ?
                                (cf->state == FILE_READ_IN_PROGRESS ?
                                    WIRESHARK_STOCK_STOP_QUIT_DONT_SAVE :
                                    WIRESHARK_STOCK_QUIT_DONT_SAVE) :
                                (capture_in_progress ?
                                    WIRESHARK_STOCK_STOP_DONT_SAVE :
                                    WIRESHARK_STOCK_DONT_SAVE)),
                            GTK_RESPONSE_REJECT);
      gtk_dialog_add_button(GTK_DIALOG(msg_dialog),
                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
      gtk_dialog_add_button(GTK_DIALOG(msg_dialog),
                            (capture_in_progress ?
                                WIRESHARK_STOCK_STOP_SAVE :
                                GTK_STOCK_SAVE),
                            GTK_RESPONSE_ACCEPT);
#else
      gtk_dialog_add_button(GTK_DIALOG(msg_dialog),
                            (capture_in_progress ?
                                WIRESHARK_STOCK_STOP_SAVE :
                                GTK_STOCK_SAVE),
                            GTK_RESPONSE_ACCEPT);
      gtk_dialog_add_button(GTK_DIALOG(msg_dialog),
                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
      gtk_dialog_add_button(GTK_DIALOG(msg_dialog),
                            (from_quit ?
                                (capture_in_progress ?
                                    WIRESHARK_STOCK_STOP_QUIT_DONT_SAVE :
                                    WIRESHARK_STOCK_QUIT_DONT_SAVE) :
                                (capture_in_progress ?
                                    WIRESHARK_STOCK_STOP_DONT_SAVE :
                                    WIRESHARK_STOCK_DONT_SAVE)),
                            GTK_RESPONSE_REJECT);
#endif
      gtk_dialog_set_default_response(GTK_DIALOG(msg_dialog), GTK_RESPONSE_ACCEPT);

      response = gtk_dialog_run(GTK_DIALOG(msg_dialog));
      gtk_widget_destroy(msg_dialog);

      switch (response) {

      case GTK_RESPONSE_ACCEPT:
#ifdef HAVE_LIBPCAP
        /* If there's a capture in progress, we have to stop the capture
           and then do the save. */
        if (capture_in_progress)
          capture_stop_cb(NULL, NULL);
#endif
        /* Save the file and close it */
        do_file_save(cf, TRUE);
        break;

      case GTK_RESPONSE_REJECT:
#ifdef HAVE_LIBPCAP
        /* If there's a capture in progress; we have to stop the capture
           and then do the close. */
        if (capture_in_progress)
          capture_stop_cb(NULL, NULL);
#endif
        /* Just close the file, discarding changes */
        cf_close(cf);
        break;

      case GTK_RESPONSE_CANCEL:
      case GTK_RESPONSE_NONE:
      case GTK_RESPONSE_DELETE_EVENT:
      default:
        /* Don't close the file (and don't stop any capture in progress). */
        return FALSE; /* file not closed */
        break;
      }
    } else {
      /* unchanged file, just close it */
      cf_close(cf);
    }
  } else {
    /* User asked not to be bothered by those prompts, just close it.
       XXX - should that apply only to saving temporary files? */
#ifdef HAVE_LIBPCAP
      /* If there's a capture in progress, we have to stop the capture
         and then do the close. */
    if (capture_in_progress)
      capture_stop_cb(NULL, NULL);
#endif
    cf_close(cf);
  }
  return TRUE; /* file closed */
}

/* Close a file */
void
file_close_cmd_cb(GtkWidget *widget _U_, gpointer data _U_) {
  do_file_close(&cfile, FALSE, "");
}

/*
 * Save the capture file in question, prompting the user for a file
 * name to save to if necessary.
 */
static void
do_file_save(capture_file *cf, gboolean dont_reopen)
{
  char *fname;

  if (cf->is_tempfile) {
    /* This is a temporary capture file, so saving it means saving
       it to a permanent file.  Prompt the user for a location
       to which to save it. */
    do_file_save_as(cf);
  } else {
    if (cf->unsaved_changes) {
      /* This is not a temporary capture file, but it has unsaved
         changes, so saving it means doing a "safe save" on top
         of the existing file, in the same format - no UI needed. */
      /* XXX - cf->filename might get freed out from under us, because
         the code path through which cf_save_packets() goes currently
	 closes the current file and then opens and reloads the saved file,
	 so make a copy and free it later. */
      fname = g_strdup(cf->filename);
      cf_save_packets(cf, fname, cf->cd_t, cf->iscompressed, dont_reopen);
      g_free(fname);
    }
    /* Otherwise just do nothing. */
  }
}

void
file_save_cmd_cb(GtkWidget *w _U_, gpointer data _U_) {
  do_file_save(&cfile, FALSE);
}

/* Attach a list of the valid 'save as' file types to a combo_box by
   checking what Wiretap supports.  Make the default type the first
   in the list.
 */
static void
set_file_type_list(GtkWidget *combo_box, capture_file *cf)
{
  GArray *savable_file_types;
  guint i;
  int ft;

  savable_file_types = wtap_get_savable_file_types(cf->cd_t, cf->lnk_t);

  if (savable_file_types != NULL) {
    /* OK, we have at least one file type we can save this file as.
       (If we didn't, we shouldn't have gotten here in the first
       place.)  Add them all to the combo box.  */
    for (i = 0; i < savable_file_types->len; i++) {
      ft = g_array_index(savable_file_types, int, i);
      ws_combo_box_append_text_and_pointer(GTK_COMBO_BOX(combo_box),
                                           wtap_file_type_string(ft),
                                           GINT_TO_POINTER(ft));
    }
    g_array_free(savable_file_types, TRUE);
  }
}

static void
file_save_as_select_file_type_cb(GtkWidget *w, gpointer data _U_)
{
  int new_file_type;
  gpointer ptr;
  GtkWidget *compressed_cb;

  if (! ws_combo_box_get_active_pointer(GTK_COMBO_BOX(w), &ptr)) {
      g_assert_not_reached();  /* Programming error: somehow nothing is active */
  }
  new_file_type = GPOINTER_TO_INT(ptr);

  compressed_cb = (GtkWidget *)g_object_get_data(G_OBJECT(file_save_as_w), E_COMPRESSED_CB_KEY);
  if (!wtap_dump_can_compress(new_file_type)) {
    /* Can't compress this file type; turn off compression and make
       the compression checkbox insensitive. */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(compressed_cb), FALSE);
    gtk_widget_set_sensitive(compressed_cb, FALSE);
  } else
    gtk_widget_set_sensitive(compressed_cb, TRUE);
}


static void
do_file_save_as(capture_file *cf)
{
#if _WIN32
  win32_save_as_file(GDK_WINDOW_HWND(gtk_widget_get_window(top_level)));
#else /* _WIN32 */
  GtkWidget     *main_vb, *ft_hb, *ft_lb, *ft_combo_box, *compressed_cb;
  GtkWidget     *msg_dialog;
  gchar         *display_basename;
  gint           response;
  char          *cf_name;
  ws_statb64     statbuf;

  if (file_save_as_w != NULL) {
    /* There's already an "Save Capture File As" dialog box; reactivate it. */
    reactivate_window(file_save_as_w);
    return;
  }

  /* Default to saving in the file's current format. */

  /* build the file selection */
  file_save_as_w = file_selection_new ("Wireshark: Save Capture File As",
                                       FILE_SELECTION_SAVE);

  /* Container for each row of widgets */

  main_vb = ws_gtk_box_new(GTK_ORIENTATION_VERTICAL, 5, FALSE);
  gtk_container_set_border_width(GTK_CONTAINER(main_vb), 5);
  file_selection_set_extra_widget(file_save_as_w, main_vb);
  gtk_widget_show(main_vb);

  /* File type row */
  range_tb = NULL;
  ft_hb = ws_gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3, FALSE);
  gtk_container_add(GTK_CONTAINER(main_vb), ft_hb);
  gtk_widget_show(ft_hb);

  ft_lb = gtk_label_new("File type:");
  gtk_box_pack_start(GTK_BOX(ft_hb), ft_lb, FALSE, FALSE, 0);
  gtk_widget_show(ft_lb);

  ft_combo_box = ws_combo_box_new_text_and_pointer();

  /* Generate the list of file types we can save. */
  set_file_type_list(ft_combo_box, cf);
  gtk_box_pack_start(GTK_BOX(ft_hb), ft_combo_box, FALSE, FALSE, 0);
  gtk_widget_show(ft_combo_box);
  g_object_set_data(G_OBJECT(file_save_as_w), E_FILE_TYPE_COMBO_BOX_KEY, ft_combo_box);

  /* compressed */
  compressed_cb = gtk_check_button_new_with_label("Compress with gzip");
  gtk_container_add(GTK_CONTAINER(ft_hb), compressed_cb);
  gtk_widget_show(compressed_cb);
  g_object_set_data(G_OBJECT(file_save_as_w), E_COMPRESSED_CB_KEY, compressed_cb);
  /* Ok: now "select" the default filetype which invokes file_save_as_select_file_type_cb */
  g_signal_connect(ft_combo_box, "changed", G_CALLBACK(file_save_as_select_file_type_cb), NULL);
  ws_combo_box_set_active(GTK_COMBO_BOX(ft_combo_box), 0);

  g_signal_connect(file_save_as_w, "destroy",
                   G_CALLBACK(file_save_as_destroy_cb), NULL);

  /*
   * Loop until the user either selects a file or gives up.
   */
  for (;;) {
    if (gtk_dialog_run(GTK_DIALOG(file_save_as_w)) != GTK_RESPONSE_ACCEPT) {
      /* They clicked "Cancel" or closed the dialog or.... */
      window_destroy(file_save_as_w);
      return;
    }

    cf_name = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_save_as_w));

    /* Perhaps the user specified a directory instead of a file.
       Check whether they did. */
    if (test_for_directory(cf_name) == EISDIR) {
          /* It's a directory - set the file selection box to display that
             directory, and go back and re-run it. */
          set_last_open_dir(cf_name);
          g_free(cf_name);
          file_selection_set_current_folder(file_save_as_w,
                                            get_last_open_dir());
          continue;
    }

    /*
     * Check that the from file is not the same as to file
     * We do it here so we catch all cases ...
     * Unfortunately, the file requester gives us an absolute file
     * name and the read file name may be relative (if supplied on
     * the command line). From Joerg Mayer.
     */
    if (files_identical(cf->filename, cf_name)) {
      simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK,
        "%sCapture file: \"%s\" identical to loaded file!%s\n\n"
        "Please choose a different filename.",
        simple_dialog_primary_start(), cf_name, simple_dialog_primary_end());
      g_free(cf_name);

      /* XXX - as we cannot start a new event loop (using gtk_dialog_run()),
       * as this will prevent the user from closing the now existing error
       * message, simply close the dialog (this is the best we can do here). */
      if (file_save_as_w)
        window_destroy(GTK_WIDGET(file_save_as_w));

      return;
    }

    /* it the file doesn't exist, simply try to save it */
    if (!file_exists(cf_name)) {
      gtk_widget_hide(GTK_WIDGET(file_save_as_w));
      g_free(cf_name);
      file_save_as_cb(file_save_as_w);
      return;
    }

    /*
     * The file exists.  Ask the user if they want to overwrite it.
     *
     * XXX - what if it's a symlink?  TextEdit just blindly replaces
     * symlinks with files if you save over them, but gedit appears
     * to replace the *target* of the symlink.  (To find the target,
     * use realpath() - it dates back to the SUSv2 and may be even
     * older.)
     */
    display_basename = g_filename_display_basename(cf_name);
    msg_dialog = gtk_message_dialog_new(GTK_WINDOW(file_save_as_w),
                                        GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_MESSAGE_QUESTION,
                                        GTK_BUTTONS_NONE,
                                        "A file named \"%s\" already exists. Do you want to replace it?",
                                        display_basename);
    g_free(display_basename);
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(msg_dialog),
        "A file or folder with the same name already exists in that folder. "
        "Replacing it will overwrite its current contents.");

    gtk_dialog_add_buttons(GTK_DIALOG(msg_dialog),
#ifndef _WIN32
                           GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                           "Replace", GTK_RESPONSE_ACCEPT,
#else
                           "Replace", GTK_RESPONSE_ACCEPT,
                           GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
#endif
                           NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(msg_dialog), GTK_RESPONSE_CANCEL);

    response = gtk_dialog_run(GTK_DIALOG(msg_dialog));
    gtk_widget_destroy(msg_dialog);

    if (response != GTK_RESPONSE_ACCEPT) {
      /* The user doesn't want to overwrite this file; let them choose
         another one */
      g_free(cf_name);
      continue;
    }

    /* Check whether the file has all the write permission bits clear
       and, on systems that have the 4.4-Lite file flags, whether it
       has the "user immutable" flag set.  Treat both of those as an
       indication that the user wants to protect the file from
       casual overwriting, and ask the user if they want to override
       that.

       (Linux's "immutable" flag, as fetched and set by the appropriate
       ioctls (FS_IOC_GETFLAGS/FS_IOC_SETFLAGS in newer kernels,
       EXT2_IOC_GETFLAGS/EXT2_IOC_SETFLAGS in older kernels - non-ext2
       file systems that support those ioctls use the same values as ext2
       does), appears to be more like the *BSD/OS X "system immutable"
       flag, as it can be set only by the superuser or by processes with
       CAP_LINUX_IMMUTABLE, so it sounds as if it's not intended for
       arbitrary users to set or clear. */
    if (ws_stat64(cf_name, &statbuf) != -1) {
      /* OK, we have the permission bits and, if HAVE_ST_FLAGS is defined,
         the flags.  (If we don't, we don't worry about it.) */
#ifdef HAVE_ST_FLAGS
      if (statbuf.st_flags & UF_IMMUTABLE) {
        display_basename = g_filename_display_basename(cf_name);
        msg_dialog = gtk_message_dialog_new(GTK_WINDOW(file_save_as_w),
                                            GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
                                            GTK_MESSAGE_QUESTION,
                                            GTK_BUTTONS_NONE,
#ifdef __APPLE__
        /* Stuff in the OS X UI calls files with the "user immutable" bit
           "locked"; pre-OS X Mac software might have had that notion and
           called it "locked". */
                                            "The file \"%s\" is locked.",
#else /* __APPLE__ */
        /* Just call it "immutable" in *BSD. */
                                            "The file \"%s\" is immutable.",
#endif /* __APPLE__ */
                                            display_basename);
        g_free(display_basename);
      } else
#endif /* HAVE_ST_FLAGS */
      if ((statbuf.st_mode & (S_IWUSR|S_IWGRP|S_IWOTH)) == 0) {
        display_basename = g_filename_display_basename(cf_name);
        msg_dialog = gtk_message_dialog_new(GTK_WINDOW(file_save_as_w),
                                            GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
                                            GTK_MESSAGE_QUESTION,
                                            GTK_BUTTONS_NONE,
                                            "The file \"%s\" is read-only.",
                                            display_basename);
        g_free(display_basename);
      } else {
        /* No problem, just drive on. */
        msg_dialog = NULL;
      }
      if (msg_dialog != NULL) {
        /* OK, ask the user if they want to overwrite the file. */
        gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(msg_dialog),
            "Do you want to overwrite it anyway?");

        gtk_dialog_add_buttons(GTK_DIALOG(msg_dialog),
                               "Overwrite", GTK_RESPONSE_ACCEPT,
                               "Don't overwrite", GTK_RESPONSE_REJECT,
                               NULL);
        gtk_dialog_set_default_response(GTK_DIALOG(msg_dialog), GTK_RESPONSE_REJECT);

        response = gtk_dialog_run(GTK_DIALOG(msg_dialog));
        gtk_widget_destroy(msg_dialog);

        if (response != GTK_RESPONSE_ACCEPT) {
          /* The user doesn't want to overwrite this file; let them choose
             another one */
          g_free(cf_name);
          continue;
        }

#ifdef HAVE_ST_FLAGS
        /* OK, they want to overwrite the file.  If it has the "user
           immutable" flag, we have to turn that off first, so we
           can move the file. */
        if (statbuf.st_flags & UF_IMMUTABLE) {
          /* If this fails, the attempt to save will fail, so just
             let that happen and pop up a "you lose" dialog. */
          chflags(cf_name, statbuf.st_flags & ~UF_IMMUTABLE);
        }
#endif
      }
    }

    /* save file */
    gtk_widget_hide(GTK_WIDGET(file_save_as_w));
    file_save_as_cb(file_save_as_w);
    return;
  }
#endif /* _WIN32 */
}

void
file_save_as_cmd_cb(GtkWidget *w _U_, gpointer data _U_)
{
  do_file_save_as(&cfile);
}

/* all tests ok, we only have to save the file */
/* (and probably continue with a pending operation) */
static void
file_save_as_cb(GtkWidget *fs) {
  GtkWidget *ft_combo_box;
  GtkWidget *compressed_cb;
  gchar	    *cf_name;
  gchar	    *dirname;
  gpointer   ptr;
  int        file_type;
  gboolean   compressed;

  cf_name = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fs));

  compressed_cb = (GtkWidget *)g_object_get_data(G_OBJECT(fs), E_COMPRESSED_CB_KEY);
  ft_combo_box  = (GtkWidget *)g_object_get_data(G_OBJECT(fs), E_FILE_TYPE_COMBO_BOX_KEY);

  if (! ws_combo_box_get_active_pointer(GTK_COMBO_BOX(ft_combo_box), &ptr)) {
      g_assert_not_reached();  /* Programming error: somehow nothing is active */
  }
  file_type = GPOINTER_TO_INT(ptr);
  compressed = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(compressed_cb));

  /* Write out all the packets to the file with the specified name. */
  if (cf_save_packets(&cfile, cf_name, file_type, compressed, FALSE) != CF_OK) {
    /* The write failed; don't dismiss the open dialog box,
       just leave it around so that the user can, after they
       dismiss the alert box popped up for the error, try again. */
    g_free(cf_name);
    /* XXX - as we cannot start a new event loop (using gtk_dialog_run()),
     * as this will prevent the user from closing the now existing error
     * message, simply close the dialog (this is the best we can do here). */
    if (file_save_as_w)
      window_destroy(GTK_WIDGET (fs));
    return;
  }

  /* The write succeeded; get rid of the file selection box. */
  /* cf_save_packets() might already closed our dialog! */
  if (file_save_as_w)
    window_destroy(GTK_WIDGET (fs));

  /* Save the directory name for future file dialogs. */
  dirname = get_dirname(cf_name);  /* Overwrites cf_name */
  set_last_open_dir(dirname);
  g_free(cf_name);
}


void
file_save_as_destroy(void)
{
  if (file_save_as_w)
    window_destroy(file_save_as_w);
}

static void
file_save_as_destroy_cb(GtkWidget *win _U_, gpointer user_data _U_)
{
  /* Note that we no longer have a "Save Capture File As" dialog box. */
  file_save_as_w = NULL;
}

/*
 * Update various dynamic parts of the range controls; called from outside
 * the file dialog code whenever the packet counts change.
 */
void
file_export_specified_packets_update_dynamics(void)
{
  if (file_export_specified_packets_w == NULL) {
    /* We don't currently have a "Export Specified Packets..." dialog box up. */
    return;
  }

  range_update_dynamics(range_tb);
}

void
file_export_specified_packets_cmd_cb(GtkWidget *widget _U_, gpointer data _U_)
{
#if _WIN32
  win32_export_specified_packets_file(GDK_WINDOW_HWND(gtk_widget_get_window(top_level)));
#else /* _WIN32 */
  GtkWidget     *main_vb, *ft_hb, *ft_lb, *ft_combo_box, *range_fr, *compressed_cb;

  if (file_export_specified_packets_w != NULL) {
    /* There's already an "Export Specified Packets" dialog box; reactivate it. */
    reactivate_window(file_export_specified_packets_w);
    return;
  }

  /* Default to writing out all displayed packets, in the file's current format. */

  /* init the packet range */
  packet_range_init(&range);
  range.process_filtered = TRUE;
  range.include_dependents = TRUE;

  /* build the file selection */
  file_export_specified_packets_w = file_selection_new ("Wireshark: Export Specified Packets",
                                                FILE_SELECTION_SAVE);

  /* Container for each row of widgets */

  main_vb = ws_gtk_box_new(GTK_ORIENTATION_VERTICAL, 5, FALSE);
  gtk_container_set_border_width(GTK_CONTAINER(main_vb), 5);
  file_selection_set_extra_widget(file_export_specified_packets_w, main_vb);
  gtk_widget_show(main_vb);

  /*** Packet Range frame ***/
  range_fr = gtk_frame_new("Packet Range");
  gtk_box_pack_start(GTK_BOX(main_vb), range_fr, FALSE, FALSE, 0);
  gtk_widget_show(range_fr);

  /* range table */
  range_tb = range_new(&range, TRUE);
  gtk_container_add(GTK_CONTAINER(range_fr), range_tb);
  gtk_widget_show(range_tb);

  /* File type row */
  ft_hb = ws_gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3, FALSE);
  gtk_container_add(GTK_CONTAINER(main_vb), ft_hb);
  gtk_widget_show(ft_hb);

  ft_lb = gtk_label_new("File type:");
  gtk_box_pack_start(GTK_BOX(ft_hb), ft_lb, FALSE, FALSE, 0);
  gtk_widget_show(ft_lb);

  ft_combo_box = ws_combo_box_new_text_and_pointer();

  /* Generate the list of file types we can save. */
  set_file_type_list(ft_combo_box, &cfile);
  gtk_box_pack_start(GTK_BOX(ft_hb), ft_combo_box, FALSE, FALSE, 0);
  gtk_widget_show(ft_combo_box);
  g_object_set_data(G_OBJECT(file_export_specified_packets_w), E_FILE_TYPE_COMBO_BOX_KEY, ft_combo_box);

  /* dynamic values in the range frame */
  range_update_dynamics(range_tb);

  /* compressed */
  compressed_cb = gtk_check_button_new_with_label("Compress with gzip");
  gtk_container_add(GTK_CONTAINER(ft_hb), compressed_cb);
  gtk_widget_show(compressed_cb);
  g_object_set_data(G_OBJECT(file_export_specified_packets_w), E_COMPRESSED_CB_KEY, compressed_cb);

  /* Ok: now "select" the default filetype which invokes file_export_specified_packets_select_file_type_cb */
  g_signal_connect(ft_combo_box, "changed", G_CALLBACK(file_export_specified_packets_select_file_type_cb), NULL);
  ws_combo_box_set_active(GTK_COMBO_BOX(ft_combo_box), 0);

  g_signal_connect(file_export_specified_packets_w, "destroy",
                   G_CALLBACK(file_export_specified_packets_destroy_cb), NULL);

  if (gtk_dialog_run(GTK_DIALOG(file_export_specified_packets_w)) == GTK_RESPONSE_ACCEPT) {
    file_export_specified_packets_ok_cb(file_export_specified_packets_w, file_export_specified_packets_w);
  } else {
    window_destroy(file_export_specified_packets_w);
  }
#endif /* _WIN32 */
}

static void
file_export_specified_packets_select_file_type_cb(GtkWidget *w, gpointer data _U_)
{
  int new_file_type;
  gpointer ptr;
  GtkWidget *compressed_cb;

  if (! ws_combo_box_get_active_pointer(GTK_COMBO_BOX(w), &ptr)) {
      g_assert_not_reached();  /* Programming error: somehow nothing is active */
  }
  new_file_type = GPOINTER_TO_INT(ptr);

  compressed_cb = (GtkWidget *)g_object_get_data(G_OBJECT(file_export_specified_packets_w), E_COMPRESSED_CB_KEY);
  if (!wtap_dump_can_compress(new_file_type)) {
    /* Can't compress this file type; turn off compression and make
       the compression checkbox insensitive. */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(compressed_cb), FALSE);
    gtk_widget_set_sensitive(compressed_cb, FALSE);
  } else
    gtk_widget_set_sensitive(compressed_cb, TRUE);
}

static void
file_export_specified_packets_exists_answered_cb(gpointer dialog _U_, gint btn, gpointer data)
{
    gchar	*cf_name;

    cf_name = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(data));

    switch(btn) {
    case(ESD_BTN_OK):
        /* save file */
        file_export_specified_packets_cb(NULL, data);
        break;
    case(ESD_BTN_CANCEL):
        /* XXX - as we cannot start a new event loop (using gtk_dialog_run()),
         * as this will prevent the user from closing the now existing error
         * message, simply close the dialog (this is the best we can do here). */
        if (file_export_specified_packets_w)
            window_destroy(file_export_specified_packets_w);
        break;
    default:
        g_assert_not_reached();
    }
    g_free(cf_name);
}

/* user pressed "Export Specified Packets" dialog "Ok" button */
static void
file_export_specified_packets_ok_cb(GtkWidget *w _U_, gpointer fs) {
  gchar	*cf_name;
  gpointer  dialog;

  cf_name = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fs));

  /* Perhaps the user specified a directory instead of a file.
     Check whether they did. */
  if (test_for_directory(cf_name) == EISDIR) {
    /* It's a directory - set the file selection box to display that
       directory, and leave the selection box displayed. */
    set_last_open_dir(cf_name);
    g_free(cf_name);
    file_selection_set_current_folder(fs, get_last_open_dir());
    return;
  }

  /* Check whether the range is valid. */
  if (!range_check_validity(&range)) {
    /* The range isn't valid; don't dismiss the open dialog box,
       just leave it around so that the user can, after they
       dismiss the alert box popped up for the error, try again. */
    g_free(cf_name);
    /* XXX - as we cannot start a new event loop (using gtk_dialog_run()),
     * as this will prevent the user from closing the now existing error
     * message, simply close the dialog (this is the best we can do here). */
    if (file_save_as_w)
      window_destroy(GTK_WIDGET (fs));

    return;
  }

  /*
   * Check that the from file is not the same as to file
   * We do it here so we catch all cases ...
   * Unfortunately, the file requester gives us an absolute file
   * name and the read file name may be relative (if supplied on
   * the command line). From Joerg Mayer.
   */
  if (files_identical(cfile.filename, cf_name)) {
    simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK,
      "%sCapture file: \"%s\" identical to loaded file!%s\n\n"
      "Please choose a different filename.",
      simple_dialog_primary_start(), cf_name, simple_dialog_primary_end());
    g_free(cf_name);
    /* XXX - as we cannot start a new event loop (using gtk_dialog_run()),
     * as this will prevent the user from closing the now existing error
     * message, simply close the dialog (this is the best we can do here). */
    if (file_export_specified_packets_w)
      window_destroy(GTK_WIDGET (fs));

    return;
  }

  /* don't show the dialog while saving (or asking) */
  gtk_widget_hide(GTK_WIDGET (fs));

  /* it the file doesn't exist, simply try to write the packets to it */
  if (!file_exists(cf_name)) {
    file_export_specified_packets_cb(NULL, fs);
    g_free(cf_name);
    return;
  }

  /* The file exists.  Ask the user if they want to overwrite it. */
  dialog = simple_dialog(ESD_TYPE_CONFIRMATION, ESD_BTNS_OK_CANCEL,
      "%s"
      "A file named \"%s\" already exists. Do you want to replace it?"
      "%s\n\n"
      "A file or folder with the same name already exists in that folder. "
      "Replacing it will overwrite its current contents.",
      simple_dialog_primary_start(), cf_name, simple_dialog_primary_end());
  simple_dialog_set_cb(dialog, file_export_specified_packets_exists_answered_cb, fs);

  g_free(cf_name);
}

/* all tests ok, we only have to write out the packets */
/* (and probably continue with a pending operation) */
static void
file_export_specified_packets_cb(GtkWidget *w _U_, gpointer fs) {
  GtkWidget *ft_combo_box;
  GtkWidget *compressed_cb;
  gchar	    *cf_name;
  gchar	    *dirname;
  gpointer   ptr;
  int        file_type;
  gboolean   compressed;

  cf_name = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fs));

  compressed_cb = (GtkWidget *)g_object_get_data(G_OBJECT(fs), E_COMPRESSED_CB_KEY);
  ft_combo_box  = (GtkWidget *)g_object_get_data(G_OBJECT(fs), E_FILE_TYPE_COMBO_BOX_KEY);

  if (! ws_combo_box_get_active_pointer(GTK_COMBO_BOX(ft_combo_box), &ptr)) {
      g_assert_not_reached();  /* Programming error: somehow nothing is active */
  }
  file_type = GPOINTER_TO_INT(ptr);
  compressed = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(compressed_cb));

  /* Write out the specified packets to the file with the specified name. */
  if (cf_export_specified_packets(&cfile, cf_name, &range, file_type,
                                  compressed) != CF_OK) {
    /* The write failed; don't dismiss the open dialog box,
       just leave it around so that the user can, after they
       dismiss the alert box popped up for the error, try again. */
    g_free(cf_name);
    /* XXX - as we cannot start a new event loop (using gtk_dialog_run()),
     * as this will prevent the user from closing the now existing error
     * message, simply close the dialog (this is the best we can do here). */
    if (file_export_specified_packets_w)
      window_destroy(GTK_WIDGET (fs));
    return;
  }

  /* The write succeeded; get rid of the file selection box. */
  /* cf_export_specified_packets() might already closed our dialog! */
  if (file_export_specified_packets_w)
    window_destroy(GTK_WIDGET (fs));

  /* Save the directory name for future file dialogs.
     XXX - should there be separate ones for "Save As" and
     "Export Specified Packets"? */
  dirname = get_dirname(cf_name);  /* Overwrites cf_name */
  set_last_open_dir(dirname);
  g_free(cf_name);
}

void
file_export_specified_packets_destroy(void)
{
  if (file_export_specified_packets_w)
    window_destroy(file_export_specified_packets_w);
}

static void
file_export_specified_packets_destroy_cb(GtkWidget *win _U_, gpointer user_data _U_)
{
  /* Note that we no longer have a "Export Specified Packets" dialog box. */
  file_export_specified_packets_w = NULL;
}

/* Reload a file using the current read and display filters */
void
file_reload_cmd_cb(GtkWidget *w _U_, gpointer data _U_) {
  cf_reload(&cfile);
}

/******************** Color Filters *********************************/
/*
 * Keep a static pointer to the current "Color Export" window, if
 * any, so that if somebody tries to do "Export"
 * while there's already a "Color Export" window up, we just pop
 * up the existing one, rather than creating a new one.
 */
static GtkWidget *file_color_import_w;

/* sets the file path to the global color filter file.
   WARNING: called by both the import and the export dialog.
*/
static void
color_global_cb(GtkWidget *widget _U_, gpointer data)
{
  GtkWidget *fs_widget = (GtkWidget *)data;
  gchar *path;

  /* decide what file to open (from dfilter code) */
  path = get_datafile_path("colorfilters");

  gtk_file_chooser_select_filename(GTK_FILE_CHOOSER(fs_widget), path);

  g_free(path);
}

/* Import color filters */
void
file_color_import_cmd_cb(GtkWidget *color_filters, gpointer filter_list _U_)
{
#if _WIN32
  win32_import_color_file(GDK_WINDOW_HWND(gtk_widget_get_window(top_level)), color_filters);
#else /* _WIN32 */
  GtkWidget	*main_vb, *cfglobal_but;

  /* No Apply button, and "OK" just sets our text widget, it doesn't
     activate it (i.e., it doesn't cause us to try to open the file). */

  if (file_color_import_w != NULL) {
    /* There's already an "Import Color Filters" dialog box; reactivate it. */
    reactivate_window(file_color_import_w);
    return;
  }

  file_color_import_w = file_selection_new("Wireshark: Import Color Filters",
                                           FILE_SELECTION_OPEN);

  /* Container for each row of widgets */
  main_vb = ws_gtk_box_new(GTK_ORIENTATION_VERTICAL, 3, FALSE);
  gtk_container_set_border_width(GTK_CONTAINER(main_vb), 5);
  file_selection_set_extra_widget(file_color_import_w, main_vb);
  gtk_widget_show(main_vb);


  cfglobal_but = gtk_button_new_with_label("Global Color Filter File");
  gtk_container_add(GTK_CONTAINER(main_vb), cfglobal_but);
  g_signal_connect(cfglobal_but, "clicked",
                   G_CALLBACK(color_global_cb), file_color_import_w);
  gtk_widget_show(cfglobal_but);

  g_signal_connect(file_color_import_w, "destroy",
                   G_CALLBACK(file_color_import_destroy_cb), NULL);


  if (gtk_dialog_run(GTK_DIALOG(file_color_import_w)) == GTK_RESPONSE_ACCEPT)
  {
      file_color_import_ok_cb(file_color_import_w, color_filters);
  }
  else window_destroy(file_color_import_w);
#endif /* _WIN32 */
}

static void
file_color_import_ok_cb(GtkWidget *w, gpointer color_filters) {
  gchar     *cf_name, *s;
  GtkWidget *fs = gtk_widget_get_toplevel(w);

  cf_name = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fs));

  /* Perhaps the user specified a directory instead of a file.
     Check whether they did. */
  if (test_for_directory(cf_name) == EISDIR) {
	/* It's a directory - set the file selection box to display that
	   directory, don't try to open the directory as a color filter file. */
        set_last_open_dir(cf_name);
        g_free(cf_name);
        file_selection_set_current_folder(fs, get_last_open_dir());
    	return;
  }

  /* Try to open the color filter file. */

  if (!color_filters_import(cf_name, color_filters)) {
    /* We couldn't open it; don't dismiss the open dialog box,
       just leave it around so that the user can, after they
       dismiss the alert box popped up for the open error,
       try again. */
    g_free(cf_name);
    /* XXX - as we cannot start a new event loop (using gtk_dialog_run()),
     * as this will prevent the user from closing the now existing error
     * message, simply close the dialog (this is the best we can do here). */
    window_destroy(GTK_WIDGET (fs));

    return;
  }

  /* We've crossed the Rubicon; get rid of the file selection box. */
  window_destroy(GTK_WIDGET (fs));

  /* Save the name of the containing directory specified in the path name,
     if any; we can write over cf_name, which is a good thing, given that
     "get_dirname()" does write over its argument. */
  s = get_dirname(cf_name);
  set_last_open_dir(s);

  g_free(cf_name);
}

static void
file_color_import_destroy_cb(GtkWidget *win _U_, gpointer user_data _U_)
{
  /* Note that we no longer have a "Open Capture File" dialog box. */
  file_color_import_w = NULL;
}

static GtkWidget *file_color_export_w;
/*
 * Set the "Export only selected filters" toggle button as appropriate for
 * the current output file type and count of selected filters.
 *
 * Called when the "Export" dialog box is created and when the selected
 * count changes.
 */
static void
color_set_export_selected_sensitive(GtkWidget * cfselect_cb)
{
  if (file_color_export_w == NULL) {
    /* We don't currently have an "Export" dialog box up. */
    return;
  }

  /* We can request that only the selected filters be saved only if
        there *are* selected filters. */
  if (color_selected_count() != 0)
    gtk_widget_set_sensitive(cfselect_cb, TRUE);
  else {
    /* Force the "Export only selected filters" toggle to "false", turn
       off the flag it controls. */
    color_selected = FALSE;
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cfselect_cb), FALSE);
    gtk_widget_set_sensitive(cfselect_cb, FALSE);
  }
}

static void
color_toggle_selected_cb(GtkWidget *widget, gpointer data _U_)
{
  color_selected = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (widget));
}

void
file_color_export_cmd_cb(GtkWidget *w _U_, gpointer filter_list)
{
#if _WIN32
  win32_export_color_file(GDK_WINDOW_HWND(gtk_widget_get_window(top_level)), filter_list);
#else /* _WIN32 */
  GtkWidget *main_vb, *cfglobal_but;
  GtkWidget *cfselect_cb;

  if (file_color_export_w != NULL) {
    /* There's already an "Color Filter Export" dialog box; reactivate it. */
    reactivate_window(file_color_export_w);
    return;
  }

  color_selected   = FALSE;

  file_color_export_w = file_selection_new("Wireshark: Export Color Filters",
                                           FILE_SELECTION_SAVE);

  /* Container for each row of widgets */
  main_vb = ws_gtk_box_new(GTK_ORIENTATION_VERTICAL, 3, FALSE);
  gtk_container_set_border_width(GTK_CONTAINER(main_vb), 5);
  file_selection_set_extra_widget(file_color_export_w, main_vb);
  gtk_widget_show(main_vb);

  cfselect_cb = gtk_check_button_new_with_label("Export only selected filters");
  gtk_container_add(GTK_CONTAINER(main_vb), cfselect_cb);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cfselect_cb), FALSE);
  g_signal_connect(cfselect_cb, "toggled",
                   G_CALLBACK(color_toggle_selected_cb), NULL);
  gtk_widget_show(cfselect_cb);
  color_set_export_selected_sensitive(cfselect_cb);

  cfglobal_but = gtk_button_new_with_label("Global Color Filter File");
  gtk_container_add(GTK_CONTAINER(main_vb), cfglobal_but);
  g_signal_connect(cfglobal_but, "clicked",
                   G_CALLBACK(color_global_cb), file_color_export_w);
  gtk_widget_show(cfglobal_but);

  g_signal_connect(file_color_export_w, "destroy",
                   G_CALLBACK(file_color_export_destroy_cb), NULL);

  if (gtk_dialog_run(GTK_DIALOG(file_color_export_w)) == GTK_RESPONSE_ACCEPT)
  {
      file_color_export_ok_cb(file_color_export_w, filter_list);
  }
  else window_destroy(file_color_export_w);
#endif /* _WIN32 */
}

static void
file_color_export_ok_cb(GtkWidget *w, gpointer filter_list) {
  gchar	*cf_name;
  gchar	*dirname;
  GtkWidget *fs = gtk_widget_get_toplevel(w);

  cf_name = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fs));

  /* Perhaps the user specified a directory instead of a file.
     Check whether they did. */
  if (test_for_directory(cf_name) == EISDIR) {
        /* It's a directory - set the file selection box to display that
           directory, and leave the selection box displayed. */
        set_last_open_dir(cf_name);
        g_free(cf_name);
        file_selection_set_current_folder(fs, get_last_open_dir());
        return;
  }

  /* Write out the filters (all, or only the ones that are currently
     displayed or selected) to the file with the specified name. */

   if (!color_filters_export(cf_name, filter_list, color_selected))
   {
    /* The write failed; don't dismiss the open dialog box,
       just leave it around so that the user can, after they
       dismiss the alert box popped up for the error, try again. */
       g_free(cf_name);

      /* XXX - as we cannot start a new event loop (using gtk_dialog_run()),
       * as this will prevent the user from closing the now existing error
       * message, simply close the dialog (this is the best we can do here). */
       window_destroy(GTK_WIDGET (fs));

       return;
   }

  /* The write succeeded; get rid of the file selection box. */
  window_destroy(GTK_WIDGET (fs));

  /* Save the directory name for future file dialogs. */
  dirname = get_dirname(cf_name);  /* Overwrites cf_name */
  set_last_open_dir(dirname);
  g_free(cf_name);
}

static void
file_color_export_destroy_cb(GtkWidget *win _U_, gpointer user_data _U_)
{
  file_color_export_w = NULL;
}
