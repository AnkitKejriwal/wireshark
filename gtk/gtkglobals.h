/* gtkglobals.h
 * GTK-related Global defines, etc.
 *
 * $Id: gtkglobals.h,v 1.20 2002/11/03 17:38:33 oabad Exp $
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

#ifndef __GTKGLOBALS_H__
#define __GTKGLOBALS_H__

extern GtkWidget *top_level, *packet_list, *tree_view, *byte_nb_ptr;
#if GTK_MAJOR_VERSION < 2
extern GdkFont   *m_r_font, *m_b_font;
extern guint      m_font_height, m_font_width;

extern GtkStyle  *item_style;

void set_plist_font(GdkFont *font);
#else
extern PangoFontDescription *m_r_font, *m_b_font;

void set_plist_font(PangoFontDescription *font);
#endif
void set_plist_sel_browse(gboolean);

#ifdef _WIN32
/* It appears that isprint() is not working well
 * with gtk+'s text widget. By narrowing down what
 * we print, the ascii portion of the hex display works.
 * MSVCRT's isprint() returns true on values like 0xd2,
 * which cause the GtkTextWidget to go wacko.
 *
 * (I.e., whilst non-ASCII characters are considered printable
 * in the locale in which Ethereal is running - which they might
 * well be, if, for example, the locale supports ISO Latin 1 -
 * GTK+'s text widget on Windows doesn't seem to handle them
 * correctly.)
 *
 * This is a quick fix for the symptom, not the
 * underlying problem.
 */
#undef isprint
#define isprint(c) (c >= 0x20 && c <= 0x7f)
#endif

#endif
