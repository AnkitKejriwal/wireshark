/* column.h
 * Definitions for column handling routines
 *
 * $Id: column.h,v 1.1 1998/11/17 04:28:41 gerald Exp $
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@zing.org>
 * Copyright 1998 Gerald Combs
 *
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

#ifndef __COLUMN_H__
#define __COLUMN_H__

typedef struct _fmt_data {
  gchar *title;
  gchar *fmt;
} fmt_data;

gint       get_column_format(gint);
gchar     *get_column_title(gint);
gchar     *col_format_to_pref_str();
void       get_column_format_matches(gboolean *, gint);
gint       get_column_width(gint format, GdkFont *font);
GtkWidget *column_prefs_show();
void       column_prefs_ok(GtkWidget *);
void       column_prefs_save(GtkWidget *);
void       column_prefs_cancel(GtkWidget *);

#endif /* column.h */
