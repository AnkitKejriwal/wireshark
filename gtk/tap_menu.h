/* tap_menu.h
 * Menu definitions for use by taps
 *
 * $Id: tap_menu.h,v 1.2 2004/02/22 18:44:03 ulfl Exp $
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

#ifndef __GTKGUITAPMENU_H__
#define __GTKGUITAPMENU_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Add a new menu item for a tap.
 * This must be called after we've created the main menu, so it can't
 * be called from the routine that registers taps - we have to introduce
 * another per-tap registration routine.
 *
 * "callback" gets called when the menu item is selected; it should do
 * the work of creating the tap window.
 *
 * "selected_packet_enabled" gets called by "set_menus_for_selected_packet()";
 * it's passed a pointer to the "frame_data" structure for the current frame,
 * if any, and to the "epan_dissect_t" structure for that frame, if any, and
 * should return TRUE if the tap will work now (which might depend on whether
 * a frame is selected and, if one is, on the frame) and FALSE if not.
 *
 * "selected_tree_row_enabled" gets called by
 * "set_menus_for_selected_tree_row()"; it's passed a pointer to the
 * "field_info" structure for the currently selected field, if any,
 * and should return TRUE if the tap will work now (which might depend on
 * whether a tree row is selected and, if one is, on the tree row) and
 * FALSE if not.
 */
extern void register_tap_menu_item(
    char *name, 
    gint layer,
    GtkItemFactoryCallback callback,
    gboolean (*selected_packet_enabled)(frame_data *, epan_dissect_t *),
    gboolean (*selected_tree_row_enabled)(field_info *),
    gpointer callback_data);

/* XXX: would it better to use an enum here? */
#define REGISTER_TAP_LAYER_GENERIC      0
#define REGISTER_TAP_LAYER_PHYSICAL     1   /* currently unused */
#define REGISTER_TAP_LAYER_DATA_LINK    2
#define REGISTER_TAP_LAYER_NETWORK      3
#define REGISTER_TAP_LAYER_TRANSPORT    4
#define REGISTER_TAP_LAYER_SESSION      5   /* currently unused */
#define REGISTER_TAP_LAYER_PRESENTATION 6   /* currently unused */
#define REGISTER_TAP_LAYER_APPLICATION  7

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GTKGUITAPMENU_H__ */
