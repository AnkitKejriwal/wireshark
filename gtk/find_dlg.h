/* find_dlg.h
 * Definitions for "find frame" window
 *
 * $Id: find_dlg.h,v 1.2 2002/05/03 21:55:15 guy Exp $
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

#ifndef __FIND_DLG_H__
#define __FIND_DLG_H__

void   find_frame_cb(GtkWidget *, gpointer);
void   find_next_cb(GtkWidget *, gpointer);
void   find_previous_cb(GtkWidget *, gpointer);

#endif /* find_dlg.h */
