/* dlg_utils.h
 * Declarations of utilities to use when constructing dialogs
 *
 * $Id: dlg_utils.h,v 1.13 2004/05/22 19:56:18 ulfl Exp $
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

#ifndef __DLG_UTILS_H__
#define __DLG_UTILS_H__

/* Create a dialog box window that belongs to Ethereal's main window. */
extern GtkWidget *dlg_window_new(const gchar *);

/* Show the created dialog box window. */
/* use  GTK_WIN_POS_CENTER or GTK_WIN_POS_MOUSE only! */
extern void dlg_window_present(GtkWidget *win, GtkWindowPosition pos);

/* Create a file selection dialog box window that belongs to Ethereal's
   main window. */
typedef enum {
	FILE_SELECTION_OPEN,
	FILE_SELECTION_SAVE
} file_selection_action_t;
extern GtkWidget *file_selection_new(const gchar *, file_selection_action_t);

/* Set the current folder for a file selection dialog. */
extern gboolean file_selection_set_current_folder(GtkWidget *, const gchar *);

/* Set the "extra" widget for a file selection dialog, with user-supplied
   options. */
extern void file_selection_set_extra_widget(GtkWidget *, GtkWidget *);

/* Create a button row for a dialog */
/* the button widgets will be available by OBJECT_GET_DATA(stock_id) */
extern GtkWidget *dlg_button_row_new(gchar *stock_id_first, ...);

/* Set the "activate" signal for a widget to call a routine to
   activate the "OK" button for a dialog box. */
extern void dlg_set_activate(GtkWidget *widget, GtkWidget *ok_button);

/* Set the "key_press_event" signal for a top-level dialog window to
   call a routine to activate the "Cancel" button for a dialog box if
   the key being pressed is the <Esc> key. */
extern void dlg_set_cancel(GtkWidget *widget, GtkWidget *cancel_button);

extern GtkWidget *dlg_radio_button_new_with_label_with_mnemonic(GSList *group,
    const gchar *label, GtkAccelGroup *accel_group);
extern GtkWidget *dlg_check_button_new_with_label_with_mnemonic(const gchar *label,
    GtkAccelGroup *accel_group);
extern GtkWidget *dlg_toggle_button_new_with_label_with_mnemonic(const gchar *label,
			GtkAccelGroup *accel_group);

#endif
