/* color_dlg.h
 * Definitions for dialog boxes for color filters
 *
 * $Id: color_dlg.h,v 1.4 2003/10/07 10:07:47 sahlberg Exp $
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

#ifndef __COLOR_DLG_H__
#define __COLOR_DLG_H__

void color_display_with_filter(char *filter);
void color_display_cb(GtkWidget *w, gpointer d);
int color_marked_count(void);
void color_add_filter_cb (color_filter_t *colorf, gpointer arg);
#endif /* color_dlg.h */
