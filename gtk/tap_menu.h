/* tap_menu.h
 * Menu definitions for use by taps
 *
 * $Id$
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

/** @file
 * Add a new menu item for a tap.
 */

/** The menu group this tap should be registered at. */
typedef enum {
    REGISTER_TAP_GROUP_NONE,
    REGISTER_TAP_GROUP_GENERIC,
    REGISTER_TAP_GROUP_CONVERSATION_LIST,
    REGISTER_TAP_GROUP_ENDPOINT_LIST,
    REGISTER_TAP_GROUP_RESPONSE_TIME
} REGISTER_TAP_GROUP_E;

/**
 * Add a new menu item for a tap.
 * This must be called after we've created the main menu, so it can't
 * be called from the routine that registers taps - we have to introduce
 * another per-tap registration routine.
 *
 * @param name the menu label
 *
 * @param group the menu group, this tap should be registered to
 *
 * @param callback gets called when the menu item is selected; it should do
 * the work of creating the tap window.
 *
 * @param selected_packet_enabled gets called by set_menus_for_selected_packet();
 * it's passed a pointer to the "frame_data" structure for the current frame,
 * if any, and to the "epan_dissect_t" structure for that frame, if any, and
 * should return TRUE if the tap will work now (which might depend on whether
 * a frame is selected and, if one is, on the frame) and FALSE if not.
 *
 * @param selected_tree_row_enabled gets called by
 * set_menus_for_selected_tree_row(); it's passed a pointer to the
 * "field_info" structure for the currently selected field, if any,
 * and should return TRUE if the tap will work now (which might depend on
 * whether a tree row is selected and, if one is, on the tree row) and
 * FALSE if not.
 *
 * @param callback_data data for callback function
 */    
extern void register_tap_menu_item(
    char *name, 
    REGISTER_TAP_GROUP_E group,
    GtkItemFactoryCallback callback,
    gboolean (*selected_packet_enabled)(frame_data *, epan_dissect_t *),
    gboolean (*selected_tree_row_enabled)(field_info *),
    gpointer callback_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GTKGUITAPMENU_H__ */
