/* gtkglobals.h
 * GTK-related Global defines, etc.
 *
 * $Id: gtkglobals.h,v 1.25 2004/06/01 17:33:36 ulfl Exp $
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

/** @file
 *  GTK globals like the main application window.
 */

/** application window */
extern GtkWidget *top_level;

/** packet list pane */
extern GtkWidget *packet_list;

/** tree view (packet details) pane */
extern GtkWidget *tree_view;

/** byte notebook (packet bytes) pane */
extern GtkWidget *byte_nb_ptr;

#if GTK_MAJOR_VERSION < 2
/** normal font */
extern GdkFont   *m_r_font;
/** bold font */
extern GdkFont   *m_b_font;
/** font height */
extern guint      m_font_height;
/** font width */
extern guint      m_font_width;
/** ??? 
 * @todo what's this?
 */
extern GtkStyle  *item_style;
#else
/** normal font */
extern PangoFontDescription *m_r_font;
/** bold font */
extern PangoFontDescription *m_b_font;
#endif

#if GTK_MAJOR_VERSION >= 2 || GTK_MINOR_VERSION >= 3
/**
 * XXX - "isprint()" can return "true" for non-ASCII characters, but
 * those don't work with GTK+ 1.3 or later, as they take UTF-8 strings
 * as input.  Until we fix up Ethereal to properly handle non-ASCII
 * characters in all output (both GUI displays and text printouts)
 * in those versions of GTK+, we work around the problem by escaping
 * all characters that aren't printable ASCII.
 */
#undef isprint
#define isprint(c) (c >= 0x20 && c < 0x7f)
#endif

#endif
