/* simple_dialog.c
 * Simple message dialog box routines.
 *
 * $Id: simple_dialog.c,v 1.28 2004/02/23 00:05:50 guy Exp $
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

#include <stdio.h>

#ifdef NEED_SNPRINTF_H
# include "snprintf.h"
#endif

#include "gtkglobals.h"
#include "simple_dialog.h"
#include "dlg_utils.h"
#include "compat_macros.h"

#include "image/stock_dialog_error_48.xpm"
#include "image/stock_dialog_info_48.xpm"
#include "image/stock_dialog_warning_48.xpm"

static void simple_dialog_cancel_cb(GtkWidget *, gpointer);

#define CALLBACK_FCT_KEY    "ESD_Callback_Fct"
#define CALLBACK_BTN_KEY    "ESD_Callback_Btn"
#define CALLBACK_DATA_KEY   "ESD_Callback_Data"

/* Simple dialog function - Displays a dialog box with the supplied message
 * text.
 *
 * Args:
 * type       : One of ESD_TYPE_*.
 * btn_mask   : The value passed in determines which buttons are displayed.
 * msg_format : Sprintf-style format of the text displayed in the dialog.
 * ...        : Argument list for msg_format
 *
 *
 * XXX - if we haven't yet put up the main window, we should just
 * queue up the message, etc., and wait until the main window pops up
 * (or until we figure out that we're in a capture child and aren't
 * going to pop up a main window) and pop up the alert boxes then, so
 * that even stuff popped up before we put up the main window (such
 * as file-open or file-read errors from dissectors' init routines)
 * shows up on top.
 */

gpointer
simple_dialog(gint type, gint btn_mask, gchar *msg_format, ...) {
  GtkWidget   *win, *main_vb, *top_hb, *type_pm, *msg_label,
              *bbox, *ok_bt, *bt;
  GdkPixmap   *pixmap;
  GdkBitmap   *mask;
  GtkStyle    *style;
  GdkColormap *cmap;
  va_list      ap;
  gchar        message[2048];
  gchar      **icon;

  /* Main window */
  switch (type) {
  case ESD_TYPE_WARN :
    icon = stock_dialog_warning_48_xpm;
    break;
  case ESD_TYPE_CONFIRMATION:
    icon = stock_dialog_warning_48_xpm;
    break;
  case ESD_TYPE_ERROR:
    icon = stock_dialog_error_48_xpm;
    break;
  case ESD_TYPE_INFO :
  default :
    icon = stock_dialog_info_48_xpm;
    break;
  }

  /*
   * The GNOME HIG:
   *
   *	http://developer.gnome.org/projects/gup/hig/1.0/windows.html#alert-windows
   *
   * says that the title should be empty for alert boxes, so there's "less
   * visual noise and confounding text."
   *
   * The Windows HIG:
   *
   *	http://msdn.microsoft.com/library/default.asp?url=/library/en-us/dnwue/html/ch09f.asp
   *
   * says it should
   *
   *	...appropriately identify the source of the message -- usually
   *	the name of the object.  For example, if the message results
   *	from editing a document, the title text is the name of the
   *	document, optionally followed by the application name.  If the
   *	message results from a non-document object, then use the
   *	application name."
   *
   * and notes that the title is important "because message boxes might
   * not always the the result of current user interaction" (e.g., some
   * app might randomly pop something up, e.g. some browser letting you
   * know that it couldn't fetch something because of a timeout).
   *
   * It also says not to use "warning" or "caution", as there's already
   * an icon that tells you what type of alert it is, and that you
   * shouldn't say "error", as that provides no useful information.
   *
   * So we give it a title on Win32, and don't give it one on UN*X.
   * For now, we give it a Win32 title of just "Ethereal"; we should
   * arguably take an argument for the title.
   */
#ifdef _WIN32
  win = dlg_window_new("Ethereal");
#else
  win = dlg_window_new("");
#endif

  gtk_window_set_modal(GTK_WINDOW(win), TRUE);
  gtk_container_border_width(GTK_CONTAINER(win), 6);

  /* Container for our rows */
  main_vb = gtk_vbox_new(FALSE, 12);
  gtk_container_add(GTK_CONTAINER(win), main_vb);
  gtk_widget_show(main_vb);

  /* Top row: Icon and message text */
  top_hb = gtk_hbox_new(FALSE, 12);
  gtk_container_border_width(GTK_CONTAINER(main_vb), 6);
  gtk_container_add(GTK_CONTAINER(main_vb), top_hb);
  gtk_widget_show(top_hb);

  style = gtk_widget_get_style(win);
  cmap  = gdk_colormap_get_system();
  pixmap = gdk_pixmap_colormap_create_from_xpm_d(NULL, cmap,  &mask,
    &style->bg[GTK_STATE_NORMAL], icon);
  type_pm = gtk_pixmap_new(pixmap, mask);
  gtk_misc_set_alignment (GTK_MISC (type_pm), 0.5, 0.0);
  gtk_container_add(GTK_CONTAINER(top_hb), type_pm);
  gtk_widget_show(type_pm);

  /* Load our vararg list into the message string */
  va_start(ap, msg_format);
  vsnprintf(message, sizeof(message), msg_format, ap);
  va_end(ap);

  msg_label = gtk_label_new(message);

#if GTK_MAJOR_VERSION >= 2
  gtk_label_set_markup(GTK_LABEL(msg_label), message);
  gtk_label_set_selectable(GTK_LABEL(msg_label), TRUE);
#endif

  gtk_label_set_justify(GTK_LABEL(msg_label), GTK_JUSTIFY_FILL);
  gtk_misc_set_alignment (GTK_MISC (type_pm), 0.5, 0.0);
  gtk_container_add(GTK_CONTAINER(top_hb), msg_label);
  gtk_widget_show(msg_label);

  /* Button row */
  switch(btn_mask) {
  case(ESD_BTN_OK):
    bbox = dlg_button_row_new(GTK_STOCK_OK, NULL);
    break;
  case(ESD_BTN_CLEAR | ESD_BTN_CANCEL):
    bbox = dlg_button_row_new(GTK_STOCK_CLEAR, GTK_STOCK_CANCEL, NULL);
    break;
  case(ESD_BTNS_YES_NO_CANCEL):
    bbox = dlg_button_row_new(GTK_STOCK_YES, GTK_STOCK_NO, GTK_STOCK_CANCEL, NULL);
    break;
  default:
    g_assert_not_reached();
    bbox = NULL;
    break;
  }
  gtk_container_add(GTK_CONTAINER(main_vb), bbox);
  gtk_widget_show(bbox);

  ok_bt = OBJECT_GET_DATA(bbox, GTK_STOCK_OK);
  if(ok_bt) {
      OBJECT_SET_DATA(ok_bt, CALLBACK_BTN_KEY, GINT_TO_POINTER(ESD_BTN_OK));
      SIGNAL_CONNECT(ok_bt, "clicked", simple_dialog_cancel_cb, win);
  }

  bt = OBJECT_GET_DATA(bbox, GTK_STOCK_CLEAR);
  if(bt) {
      OBJECT_SET_DATA(bt, CALLBACK_BTN_KEY, GINT_TO_POINTER(ESD_BTN_CLEAR));
      SIGNAL_CONNECT(bt, "clicked", simple_dialog_cancel_cb, win);
  }

  bt = OBJECT_GET_DATA(bbox, GTK_STOCK_YES);
  if(bt) {
      OBJECT_SET_DATA(bt, CALLBACK_BTN_KEY, GINT_TO_POINTER(ESD_BTN_YES));
      SIGNAL_CONNECT(bt, "clicked", simple_dialog_cancel_cb, win);
  }

  bt = OBJECT_GET_DATA(bbox, GTK_STOCK_NO);
  if(bt) {
      OBJECT_SET_DATA(bt, CALLBACK_BTN_KEY, GINT_TO_POINTER(ESD_BTN_NO));
      SIGNAL_CONNECT(bt, "clicked", simple_dialog_cancel_cb, win);
  }

  bt = OBJECT_GET_DATA(bbox, GTK_STOCK_CANCEL);
  if(bt) {
      OBJECT_SET_DATA(bt, CALLBACK_BTN_KEY, GINT_TO_POINTER(ESD_BTN_CANCEL));
      SIGNAL_CONNECT(bt, "clicked", simple_dialog_cancel_cb, win);
    /* Catch the "key_press_event" signal in the window, so that we can catch
       the ESC key being pressed and act as if the "OK" button had
       been selected. */
      dlg_set_cancel(win, bt);
      gtk_widget_grab_default(bt);
  }

  if(!bt) {
      /* Catch the "key_press_event" signal in the window, so that we can catch
       the ESC key being pressed and act as if the "OK" button had
       been selected. */
    dlg_set_cancel(win, ok_bt);
    gtk_widget_grab_default(ok_bt);
  }

  gtk_widget_show(win);

  return win;
}

static void
simple_dialog_cancel_cb(GtkWidget *w, gpointer win) {
  gint button       = GPOINTER_TO_INT(    OBJECT_GET_DATA(w,   CALLBACK_BTN_KEY));
  simple_dialog_cb_t    callback_fct    = OBJECT_GET_DATA(win, CALLBACK_FCT_KEY);
  gpointer              data            = OBJECT_GET_DATA(win, CALLBACK_DATA_KEY);

  gtk_widget_destroy(GTK_WIDGET(win));

  if (callback_fct)
    (callback_fct) (win, button, data);
}

void simple_dialog_set_cb(gpointer dialog, simple_dialog_cb_t callback_fct, gpointer data)
{

    OBJECT_SET_DATA(GTK_WIDGET(dialog), CALLBACK_FCT_KEY, callback_fct);
    OBJECT_SET_DATA(GTK_WIDGET(dialog), CALLBACK_DATA_KEY, data);
}

char *
simple_dialog_primary_start(void) {
    return PRIMARY_TEXT_START;
}

char *
simple_dialog_primary_end(void) {
    return PRIMARY_TEXT_END;
}


