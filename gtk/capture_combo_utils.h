/* capture_combo_utils.c
 * Declarations of tilities for combo box of interface names
 *
 * $Id: capture_combo_utils.h,v 1.2 2004/06/01 20:28:04 ulfl Exp $
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

#ifndef __CAPTURE_COMBO_UTILS_H__
#define __CAPTURE_COMBO_UTILS_H__

/** @file
 *  GList of available capture interfaces.
 */

/** Build the GList of available capture interfaces.
 *
 * @param if_list the interface list from get_interface_list()
 * @param do_hide hide the "hidden" interfaces
 */
GList *build_capture_combo_list(GList *if_list, gboolean do_hide);

/** Free the GList from build_capture_combo_list().
 *
 * @param combo_list the interface list from build_capture_combo_list()
 */
void free_capture_combo_list(GList *combo_list);

#endif
