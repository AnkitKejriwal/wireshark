/* webbrowser.h
 * Web browser activation functions
 *
 * $Id: webbrowser.h,v 1.2 2004/06/29 17:10:53 ulfl Exp $
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

#ifndef __WEBBROWSER_H__
#define __WEBBROWSER_H__

extern gboolean browser_needs_pref();

extern gboolean browser_open_url (const gchar *url);

/* browse a file relative to the data dir */
extern void browser_open_data_file (const gchar *filename);

#endif /* __WEBBROWSER_H__ */
