/* ui_util.h
 * Definitions for UI utility routines
 *
 * $Id: ui_util.h,v 1.9 2004/05/20 18:18:12 ulfl Exp $
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

#ifndef __GTKGUIUI_UTIL_H__
#define __GTKGUIUI_UTIL_H__

/* Create a new window, of the specified type, with the specified title
   (if any) and the Ethereal icon. */
GtkWidget *window_new(GtkWindowType, const gchar *);

/* Given a pointer to a GtkWidget for a top-level window, raise it and
   de-iconify it.  This routine is used if the user has done something to
   ask that a window of a certain type be popped up when there can be only
   one such window and such a window has already been popped up - we
   pop up the existing one rather than creating a new one. */
void reactivate_window(GtkWidget *);

/* Create a GtkScrolledWindow, set its scrollbar placement appropriately,
   and remember it. */
GtkWidget *scrolled_window_new(GtkAdjustment *hadjustment,
			       GtkAdjustment *vadjustment);

/* Set the scrollbar placement of all scrolled windows based on user
   preference. */
void set_scrollbar_placement_all(void);

#if GTK_MAJOR_VERSION < 2
/* Create a GtkCTree, give it the right styles, and remember it. */
GtkWidget *ctree_new(gint columns, gint tree_column);
GtkWidget *ctree_new_with_titles(gint columns, gint tree_column,
				 gchar *titles[]);
#else
GtkWidget *tree_view_new(GtkTreeModel *model);
#endif

/* create a simple list widget */
extern GtkWidget *simple_list_new(gint cols, gchar **titles);
/* append a row to the simple list */
/* use it like: simple_list_append(list, 0, "first", 1, "second", -1) */
extern void simple_list_append(GtkWidget *list, ...);



/* Set the styles of all Trees based upon user preferences. */
void set_tree_styles_all(void);

/* convert an xpm picture into a GtkWidget showing it (top_level must already be visible!) */
GtkWidget *xpm_to_widget(const char ** xpm);

#endif /* __GTKGUIUI_UTIL_H__ */
