/* menu.c
 * Menu routines
 *
 * $Id: menu.c,v 1.156 2004/02/03 00:16:59 ulfl Exp $
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <gtk/gtk.h>

#include <string.h>
#include <stdio.h>

#include "main.h"
#include "menu.h"
#include <epan/packet.h>
#include <epan/resolv.h>
#include "prefs.h"
#include "capture_dlg.h"
#include "color_dlg.h"
#include "filter_prefs.h"
#include "file_dlg.h"
#include "find_dlg.h"
#include "goto_dlg.h"
#include "summary_dlg.h"
#include "prefs_dlg.h"
#include "packet_win.h"
#include "print.h"
#include "follow_dlg.h"
#include "decode_as_dlg.h"
#include "help_dlg.h"
#include "supported_protos_dlg.h"
#include "proto_dlg.h"
#include "proto_hier_stats_dlg.h"
#include "keys.h"
#include <epan/plugins.h>
#include "tcp_graph.h"
#include <epan/epan_dissect.h>
#include "compat_macros.h"
#include "toolbar.h"
#include "gtkglobals.h"
#include "register.h"
#include "../tap.h"
#include "../menu.h"
#include "../ipproto.h"
#include "packet_list.h"
#include "ethclist.h"
#include "recent.h"
#include "../ui_util.h"
#include "proto_draw.h"
#include "simple_dialog.h"

GtkWidget *popup_menu_object;

extern void savehex_cb(GtkWidget * w, gpointer data _U_);

static void
clear_menu_recent_capture_file_cmd_cb(GtkWidget *w, gpointer unused _U_);

#define GTK_MENU_FUNC(a) ((GtkItemFactoryCallback)(a))

static void menus_init(void);
static void set_menu_sensitivity (GtkItemFactory *, const gchar *, gint);
static void main_toolbar_show_cb(GtkWidget *w _U_, gpointer d _U_);
static void filter_toolbar_show_cb(GtkWidget *w _U_, gpointer d _U_);
static void packet_list_show_cb(GtkWidget *w _U_, gpointer d _U_);
static void tree_view_show_cb(GtkWidget *w _U_, gpointer d _U_);
static void byte_view_show_cb(GtkWidget *w _U_, gpointer d _U_);
static void statusbar_show_cb(GtkWidget *w _U_, gpointer d _U_);
static void timestamp_absolute_cb(GtkWidget *w _U_, gpointer d _U_);
static void timestamp_absolute_date_cb(GtkWidget *w _U_, gpointer d _U_);
static void timestamp_relative_cb(GtkWidget *w _U_, gpointer d _U_);
static void timestamp_delta_cb(GtkWidget *w _U_, gpointer d _U_);
static void name_resolution_mac_cb(GtkWidget *w _U_, gpointer d _U_);
static void name_resolution_network_cb(GtkWidget *w _U_, gpointer d _U_);
static void name_resolution_transport_cb(GtkWidget *w _U_, gpointer d _U_);
static void auto_scroll_live_cb(GtkWidget *w _U_, gpointer d _U_);

/* This is the GtkItemFactoryEntry structure used to generate new menus.
       Item 1: The menu path. The letter after the underscore indicates an
               accelerator key once the menu is open.
       Item 2: The accelerator key for the entry
       Item 3: The callback function.
       Item 4: The callback action.  This changes the parameters with
               which the function is called.  The default is 0.
       Item 5: The item type, used to define what kind of an item it is.
               Here are the possible values:

               NULL               -> "<Item>"
               ""                 -> "<Item>"
               "<Title>"          -> create a title item
               "<Item>"           -> create a simple item
               "<ImageItem>"      -> create an item holding an image (gtk2)
               "<StockItem>"	  -> create an item holding a stock image (gtk2)
               "<CheckItem>"      -> create a check item
               "<ToggleItem>"     -> create a toggle item
               "<RadioItem>"      -> create a radio item
               <path>             -> path of a radio item to link against
               "<Separator>"      -> create a separator
               "<Tearoff>"        -> create a tearoff separator (gtk2)
               "<Branch>"         -> create an item to hold sub items (optional)
               "<LastBranch>"     -> create a right justified branch
       Item 6: extra data needed for ImageItem and StockItem (gtk2)
    */

/* main menu */
static GtkItemFactoryEntry menu_items[] =
{
    ITEM_FACTORY_ENTRY("/_File", NULL, NULL, 0, "<Branch>", NULL),
    ITEM_FACTORY_STOCK_ENTRY("/File/_Open...", "<control>O", file_open_cmd_cb,
                             0, GTK_STOCK_OPEN),
    ITEM_FACTORY_ENTRY("/File/Open _Recent", NULL, NULL, 0, "<Branch>", NULL),
    ITEM_FACTORY_STOCK_ENTRY("/File/_Close", "<control>W", file_close_cmd_cb,
                             0, GTK_STOCK_CLOSE),
    ITEM_FACTORY_ENTRY("/File/<separator>", NULL, NULL, 0, "<Separator>", NULL),
    ITEM_FACTORY_STOCK_ENTRY("/File/_Save", "<control>S", file_save_cmd_cb,
                             0, GTK_STOCK_SAVE),
    ITEM_FACTORY_STOCK_ENTRY("/File/Save _As...", "<shift><control>S", file_save_as_cmd_cb,
                             0, GTK_STOCK_SAVE_AS),
    ITEM_FACTORY_ENTRY("/File/<separator>", NULL, NULL, 0, "<Separator>", NULL),
    ITEM_FACTORY_ENTRY("/File/_Export", NULL, NULL, 0, "<Branch>", NULL),
    ITEM_FACTORY_ENTRY("/File/_Export/_Selected Packet Bytes...", "<control>H", savehex_cb,
                             0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/File/<separator>", NULL, NULL, 0, "<Separator>", NULL),
    ITEM_FACTORY_STOCK_ENTRY("/File/_Print...", "<control>P", file_print_cmd_cb,
                             0, GTK_STOCK_PRINT),
    ITEM_FACTORY_ENTRY("/File/<separator>", NULL, NULL, 0, "<Separator>", NULL),
    ITEM_FACTORY_STOCK_ENTRY("/File/_Quit", "<control>Q", file_quit_cmd_cb,
                             0, GTK_STOCK_QUIT),
    ITEM_FACTORY_ENTRY("/_Edit", NULL, NULL, 0, "<Branch>", NULL),
#if 0
    /* Un-#if this when we actually implement Cut/Copy/Paste. */
    ITEM_FACTORY_STOCK_ENTRY("/Edit/Cut", "<control>X", NULL,
                             0, GTK_STOCK_CUT),
    ITEM_FACTORY_STOCK_ENTRY("/Edit/Copy", "<control>C", NULL,
                             0, GTK_STOCK_COPY),
    ITEM_FACTORY_STOCK_ENTRY("/Edit/Paste", "<control>V", NULL,
                             0, GTK_STOCK_PASTE),
    ITEM_FACTORY_ENTRY("/Edit/<separator>", NULL, NULL, 0, "<Separator>"),
#endif
    ITEM_FACTORY_STOCK_ENTRY("/Edit/_Find Packet...", "<control>F",
                             find_frame_cb, 0, GTK_STOCK_FIND),
    ITEM_FACTORY_STOCK_ENTRY("/Edit/Find Ne_xt", "<control>N", find_next_cb,
                             0, GTK_STOCK_GO_FORWARD),
    ITEM_FACTORY_STOCK_ENTRY("/Edit/Find Pre_vious", "<control>B",
                             find_previous_cb, 0, GTK_STOCK_GO_BACK),
    ITEM_FACTORY_ENTRY("/Edit/<separator>", NULL, NULL, 0, "<Separator>", NULL),
    ITEM_FACTORY_STOCK_ENTRY("/Edit/Go To Firs_t Packet", NULL,
                             goto_top_frame_cb, 0, GTK_STOCK_GOTO_TOP),
    ITEM_FACTORY_STOCK_ENTRY("/Edit/Go To _Last Packet", NULL,
                             goto_bottom_frame_cb, 0, GTK_STOCK_GOTO_BOTTOM),
    ITEM_FACTORY_STOCK_ENTRY("/Edit/_Go To Packet...", "<control>G",
                             goto_frame_cb, 0, GTK_STOCK_JUMP_TO),
    ITEM_FACTORY_ENTRY("/Edit/Go To _Corresponding Packet", NULL, goto_framenum_cb,
                       0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Edit/<separator>", NULL, NULL, 0, "<Separator>", NULL),
    ITEM_FACTORY_ENTRY("/Edit/Time _Reference", NULL, NULL, 0, "<Branch>", NULL),
    ITEM_FACTORY_ENTRY("/Edit/Time Reference/Set Time Reference (toggle)", "<control>T", reftime_frame_cb, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Edit/Time Reference/Find Next", NULL, reftime_frame_cb, 1, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Edit/Time Reference/Find Previous", NULL, reftime_frame_cb, 2, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Edit/_Mark Packet", "<control>M", mark_frame_cb,
                       0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Edit/Mark _All Packets", NULL, mark_all_frames_cb,
                       0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Edit/_Unmark All Packets", NULL, unmark_all_frames_cb,
                       0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Edit/<separator>", NULL, NULL, 0, "<Separator>", NULL),
    ITEM_FACTORY_STOCK_ENTRY("/Edit/_Preferences...", "<shift><control>P", prefs_cb,
                             0, GTK_STOCK_PREFERENCES),
    ITEM_FACTORY_ENTRY("/_View", NULL, NULL, 0, "<Branch>", NULL),
    ITEM_FACTORY_ENTRY("/View/_Show", NULL, NULL, 0, "<Branch>", NULL),
    ITEM_FACTORY_ENTRY("/View/Show/Main Toolbar", NULL, main_toolbar_show_cb, 0, "<CheckItem>", NULL),
    ITEM_FACTORY_ENTRY("/View/Show/Filter Toolbar", NULL, filter_toolbar_show_cb, 0, "<CheckItem>", NULL),
    ITEM_FACTORY_ENTRY("/View/Show/<separator>", NULL, NULL, 0, "<Separator>", NULL),
    ITEM_FACTORY_ENTRY("/View/Show/Packet List", NULL, packet_list_show_cb, 0, "<CheckItem>", NULL),
    ITEM_FACTORY_ENTRY("/View/Show/Packet Details", NULL, tree_view_show_cb, 0, "<CheckItem>", NULL),
    ITEM_FACTORY_ENTRY("/View/Show/Packet Bytes", NULL, byte_view_show_cb, 0, "<CheckItem>", NULL),
    ITEM_FACTORY_ENTRY("/View/Show/<separator>", NULL, NULL, 0, "<Separator>", NULL),
    ITEM_FACTORY_ENTRY("/View/Show/Status Bar", NULL, statusbar_show_cb, 0, "<CheckItem>", NULL),
    ITEM_FACTORY_ENTRY("/View/_Time Display Format", NULL, NULL, 0, "<Branch>", NULL),
    ITEM_FACTORY_ENTRY("/View/Time Display Format/Time of Day", NULL, timestamp_absolute_cb, 
                        0, "<RadioItem>", NULL),
    ITEM_FACTORY_ENTRY("/View/Time Display Format/Date and Time of Day", NULL, timestamp_absolute_date_cb, 
                        0, "/View/Time Display Format/Time of Day", NULL),
    ITEM_FACTORY_ENTRY("/View/Time Display Format/Seconds Since Beginning of Capture", NULL, timestamp_relative_cb, 
                        0, "/View/Time Display Format/Time of Day", NULL),
    ITEM_FACTORY_ENTRY("/View/Time Display Format/Seconds Since Previous Packet", NULL, timestamp_delta_cb, 
                        0, "/View/Time Display Format/Time of Day", NULL),
    ITEM_FACTORY_ENTRY("/View/_Name Resolution", NULL, NULL, 0, "<Branch>", NULL),
    ITEM_FACTORY_ENTRY("/View/Name Resolution/Enable for _MAC Layer", NULL, name_resolution_mac_cb, 0, "<CheckItem>", NULL),
    ITEM_FACTORY_ENTRY("/View/Name Resolution/Enable for _Network Layer", NULL, name_resolution_mac_cb, 0, "<CheckItem>", NULL),
    ITEM_FACTORY_ENTRY("/View/Name Resolution/Enable for _Transport Layer", NULL, name_resolution_transport_cb, 0, "<CheckItem>", NULL),
    ITEM_FACTORY_ENTRY("/View/Auto Scroll in Live Capture", NULL, auto_scroll_live_cb, 0, "<CheckItem>", NULL),
    ITEM_FACTORY_ENTRY("/View/<separator>", NULL, NULL, 0, "<Separator>", NULL),
    ITEM_FACTORY_STOCK_ENTRY("/View/Zoom In", "<control>plus", view_zoom_in_cb,
                             0, GTK_STOCK_ZOOM_IN),
    ITEM_FACTORY_STOCK_ENTRY("/View/Zoom Out", "<control>minus", view_zoom_out_cb,
                             0, GTK_STOCK_ZOOM_OUT),
    ITEM_FACTORY_STOCK_ENTRY("/View/Normal Size", "<control>equal", view_zoom_100_cb,
                             0, GTK_STOCK_ZOOM_100),
    ITEM_FACTORY_ENTRY("/View/<separator>", NULL, NULL, 0, "<Separator>", NULL),
    ITEM_FACTORY_ENTRY("/View/Collapse _All", NULL, collapse_all_cb,
                       0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/View/_Expand All", NULL, expand_all_cb,
                       0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/View/<separator>", NULL, NULL, 0, "<Separator>", NULL),
    ITEM_FACTORY_STOCK_ENTRY("/View/_Coloring Rules...", NULL, color_display_cb,
                       0, GTK_STOCK_SELECT_COLOR),
    ITEM_FACTORY_ENTRY("/View/<separator>", NULL, NULL, 0, "<Separator>", NULL),
    ITEM_FACTORY_ENTRY("/View/_Show Packet In New Window", NULL,
                       new_window_cb, 0, NULL, NULL),
    ITEM_FACTORY_STOCK_ENTRY("/View/_Reload", "<control>R", file_reload_cmd_cb,
                             0, GTK_STOCK_REFRESH),
#ifdef HAVE_LIBPCAP
    ITEM_FACTORY_ENTRY("/_Capture", NULL, NULL, 0, "<Branch>", NULL),
    ITEM_FACTORY_STOCK_ENTRY("/Capture/_Start...", "<control>K",
                             capture_prep_cb, 0, ETHEREAL_STOCK_CAPTURE_START),
    ITEM_FACTORY_STOCK_ENTRY("/Capture/S_top", "<control>E", capture_stop_cb,
                             0, GTK_STOCK_STOP),
    ITEM_FACTORY_STOCK_ENTRY("/Capture/_Capture Filters...", NULL, cfilter_dialog_cb,
                       0, ETHEREAL_STOCK_CAPTURE_FILTER),
#endif /* HAVE_LIBPCAP */
    ITEM_FACTORY_ENTRY("/_Analyze", NULL, NULL, 0, "<Branch>", NULL),
    ITEM_FACTORY_STOCK_ENTRY("/Analyze/_Display Filters...", NULL, dfilter_dialog_cb,
                       0, ETHEREAL_STOCK_DISPLAY_FILTER),
    ITEM_FACTORY_ENTRY("/Analyze/_Match", NULL, NULL, 0, "<Branch>", NULL),
    ITEM_FACTORY_ENTRY("/Analyze/Match/_Selected", NULL,
                       match_selected_cb_replace_ptree, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Analyze/Match/_Not Selected", NULL,
                       match_selected_cb_not_ptree, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Analyze/Match/_And Selected", NULL,
                       match_selected_cb_and_ptree, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Analyze/Match/_Or Selected", NULL,
                       match_selected_cb_or_ptree, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Analyze/Match/A_nd Not Selected", NULL,
                       match_selected_cb_and_ptree_not, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Analyze/Match/O_r Not Selected", NULL,
                       match_selected_cb_or_ptree_not, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Analyze/_Prepare", NULL, NULL, 0, "<Branch>", NULL),
    ITEM_FACTORY_ENTRY("/Analyze/Prepare/_Selected", NULL,
                       prepare_selected_cb_replace_ptree, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Analyze/Prepare/_Not Selected", NULL,
                       prepare_selected_cb_not_ptree, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Analyze/Prepare/_And Selected", NULL,
                       prepare_selected_cb_and_ptree, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Analyze/Prepare/_Or Selected", NULL,
                       prepare_selected_cb_or_ptree, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Analyze/Prepare/A_nd Not Selected", NULL,
                       prepare_selected_cb_and_ptree_not, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Analyze/Prepare/O_r Not Selected", NULL,
                       prepare_selected_cb_or_ptree_not, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Analyze/<separator>", NULL, NULL, 0, "<Separator>", NULL),
    ITEM_FACTORY_ENTRY("/Analyze/_Enabled Protocols...", "<shift><control>R", proto_cb, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Analyze/Decode _As...", NULL, decode_as_cb,
                       0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Analyze/_User Specified Decodes...", NULL,
                       decode_show_cb, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Analyze/<separator>", NULL, NULL, 0, "<Separator>", NULL),
    ITEM_FACTORY_ENTRY("/Analyze/_Follow TCP Stream", NULL, follow_stream_cb,
                       0, NULL, NULL),
/*  {"/Analyze/Graph", NULL, NULL, 0, NULL}, future use */
    ITEM_FACTORY_ENTRY("/Analyze/_TCP Stream Analysis", NULL, NULL,
                       0, "<Branch>", NULL),
    ITEM_FACTORY_ENTRY("/Analyze/TCP Stream Analysis/Time-Sequence Graph (Stevens)",
                       NULL, tcp_graph_cb, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Analyze/TCP Stream Analysis/Time-Sequence Graph (tcptrace)",
                       NULL, tcp_graph_cb, 1, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Analyze/TCP Stream Analysis/Throughput Graph", NULL,
                       tcp_graph_cb, 2, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Analyze/TCP Stream Analysis/Round Trip Time Graph", NULL,
                       tcp_graph_cb, 3, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Analyze/<separator>", NULL, NULL, 0, "<Separator>", NULL),
    ITEM_FACTORY_ENTRY("/Analyze/Summar_y", NULL, summary_open_cb, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Analyze/Protocol _Hierarchy Statistics", NULL,
                       proto_hier_stats_cb, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/_Help", NULL, NULL, 0, "<Branch>", NULL),
    ITEM_FACTORY_STOCK_ENTRY("/Help/_Contents", "F1", help_cb, 0, GTK_STOCK_HELP),
    ITEM_FACTORY_ENTRY("/Help/_Supported Protocols", NULL, supported_cb, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Help/<separator>", NULL, NULL, 0, "<Separator>", NULL),
#ifdef HAVE_PLUGINS
    ITEM_FACTORY_ENTRY("/Help/About _Plugins", NULL, tools_plugins_cmd_cb,
                       0, NULL, NULL),
#endif /* HAVE_PLUGINS */
    ITEM_FACTORY_ENTRY("/Help/_About Ethereal", NULL, about_ethereal,
                       0, NULL, NULL)
};


/* calculate the number of menu_items */
static int nmenu_items = sizeof(menu_items) / sizeof(menu_items[0]);

/* packet list popup */
static GtkItemFactoryEntry packet_list_menu_items[] =
{
    ITEM_FACTORY_ENTRY("/Follow TCP Stream", NULL, follow_stream_cb,
                       0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Decode As...", NULL, decode_as_cb, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Display Filters...", NULL, dfilter_dialog_cb,
                       0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/<separator>", NULL, NULL, 0, "<Separator>", NULL),
    ITEM_FACTORY_ENTRY("/Mark Packet", NULL, mark_frame_cb, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Time Reference", NULL, NULL, 0, "<Branch>", NULL),
    ITEM_FACTORY_ENTRY("/Time Reference/Set Time Reference (toggle)", NULL, reftime_frame_cb, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Time Reference/Find Next", NULL, reftime_frame_cb, 1, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Time Reference/Find Previous", NULL, reftime_frame_cb, 2, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Match", NULL, NULL, 0, "<Branch>", NULL),
    ITEM_FACTORY_ENTRY("/Match/_Selected", NULL,
                       match_selected_cb_replace_plist, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Match/_Not Selected", NULL,
                       match_selected_cb_not_plist, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Match/_And Selected", NULL,
                       match_selected_cb_and_plist, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Match/_Or Selected", NULL, match_selected_cb_or_plist,
                       0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Match/A_nd Not Selected", NULL,
                       match_selected_cb_and_plist_not, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Match/O_r Not Selected", NULL,
                       match_selected_cb_or_plist_not, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Prepare", NULL, NULL, 0, "<Branch>", NULL),
    ITEM_FACTORY_ENTRY("/Prepare/_Selected", NULL,
                       prepare_selected_cb_replace_plist, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Prepare/_Not Selected", NULL,
                       prepare_selected_cb_not_plist, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Prepare/_And Selected", NULL,
                       prepare_selected_cb_and_plist, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Prepare/_Or Selected", NULL,
                       prepare_selected_cb_or_plist, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Prepare/A_nd Not Selected", NULL,
                       prepare_selected_cb_and_plist_not, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Prepare/O_r Not Selected", NULL,
                       prepare_selected_cb_or_plist_not, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/<separator>", NULL, NULL, 0, "<Separator>", NULL),
    ITEM_FACTORY_ENTRY("/Coloring Rules...", NULL, color_display_cb,
                       0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Print...", NULL, file_print_cmd_cb, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Show Packet In New Window", NULL, new_window_cb,
                       0, NULL, NULL),
};

static GtkItemFactoryEntry tree_view_menu_items[] =
{
    ITEM_FACTORY_ENTRY("/Follow TCP Stream", NULL, follow_stream_cb,
                       0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Decode As...", NULL, decode_as_cb, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Display Filters...", NULL, dfilter_dialog_cb,
                       0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/<separator>", NULL, NULL, 0, "<Separator>", NULL),
    ITEM_FACTORY_ENTRY("/_Resolve Name", NULL, resolve_name_cb, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/_Go To Corresponding Packet", NULL, goto_framenum_cb, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/_Export Selected Packet Bytes...", NULL, savehex_cb,
                       0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Protocol Properties...", NULL, properties_cb,
                       0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Match", NULL, NULL, 0, "<Branch>", NULL),
    ITEM_FACTORY_ENTRY("/Match/_Selected", NULL,
                       match_selected_cb_replace_ptree, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Match/_Not Selected", NULL,
                       match_selected_cb_not_ptree, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Match/_And Selected", NULL,
                       match_selected_cb_and_ptree, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Match/_Or Selected", NULL, match_selected_cb_or_ptree,
                       0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Match/A_nd Not Selected", NULL,
                       match_selected_cb_and_ptree_not, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Match/O_r Not Selected", NULL,
                       match_selected_cb_or_ptree_not, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Prepare", NULL, NULL, 0, "<Branch>", NULL),
    ITEM_FACTORY_ENTRY("/Prepare/_Selected", NULL,
                       prepare_selected_cb_replace_ptree, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Prepare/_Not Selected", NULL,
                       prepare_selected_cb_not_ptree, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Prepare/_And Selected", NULL,
                       prepare_selected_cb_and_ptree, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Prepare/_Or Selected", NULL,
                       prepare_selected_cb_or_ptree, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Prepare/A_nd Not Selected", NULL,
                       prepare_selected_cb_and_ptree_not, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Prepare/O_r Not Selected", NULL,
                       prepare_selected_cb_or_ptree_not, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/<separator>", NULL, NULL, 0, "<Separator>", NULL),
    ITEM_FACTORY_ENTRY("/Collapse All", NULL, collapse_all_cb, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Expand All", NULL, expand_all_cb, 0, NULL, NULL)
};

static GtkItemFactoryEntry hexdump_menu_items[] =
{
    ITEM_FACTORY_ENTRY("/Follow TCP Stream", NULL, follow_stream_cb,
                       0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Decode As...", NULL, decode_as_cb, 0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Display Filters...", NULL, dfilter_dialog_cb,
                       0, NULL, NULL),
    ITEM_FACTORY_ENTRY("/Export Selected Packet Bytes...", NULL, savehex_cb,
                       0, NULL, NULL)
};

static int initialize = TRUE;
static GtkItemFactory *main_menu_factory = NULL;
static GtkItemFactory *packet_list_menu_factory = NULL;
static GtkItemFactory *tree_view_menu_factory = NULL;
static GtkItemFactory *hexdump_menu_factory = NULL;

static GSList *popup_menu_list = NULL;

static GtkAccelGroup *grp;

GtkWidget *
main_menu_new(GtkAccelGroup ** table) {
  GtkWidget *menubar;

  grp = gtk_accel_group_new();

  if (initialize)
    menus_init();

  menubar = main_menu_factory->widget;

  if (table)
    *table = grp;

  return menubar;
}

static void
menus_init(void) {

  if (initialize) {
    initialize = FALSE;

    /* popup */
    packet_list_menu_factory = gtk_item_factory_new(GTK_TYPE_MENU, "<main>", NULL);
    popup_menu_object = gtk_menu_new();
    gtk_item_factory_create_items_ac(packet_list_menu_factory, sizeof(packet_list_menu_items)/sizeof(packet_list_menu_items[0]), packet_list_menu_items, popup_menu_object, 2);
    OBJECT_SET_DATA(popup_menu_object, PM_PACKET_LIST_KEY,
                    packet_list_menu_factory->widget);
    popup_menu_list = g_slist_append((GSList *)popup_menu_list, packet_list_menu_factory);

    tree_view_menu_factory = gtk_item_factory_new(GTK_TYPE_MENU, "<main>", NULL);
    gtk_item_factory_create_items_ac(tree_view_menu_factory, sizeof(tree_view_menu_items)/sizeof(tree_view_menu_items[0]), tree_view_menu_items, popup_menu_object, 2);
    OBJECT_SET_DATA(popup_menu_object, PM_TREE_VIEW_KEY,
                    tree_view_menu_factory->widget);
    popup_menu_list = g_slist_append((GSList *)popup_menu_list, tree_view_menu_factory);

    hexdump_menu_factory = gtk_item_factory_new(GTK_TYPE_MENU, "<main>", NULL);
    gtk_item_factory_create_items_ac(hexdump_menu_factory, sizeof(hexdump_menu_items)/sizeof(hexdump_menu_items[0]), hexdump_menu_items, popup_menu_object, 2);
    OBJECT_SET_DATA(popup_menu_object, PM_HEXDUMP_KEY,
                    hexdump_menu_factory->widget);
    popup_menu_list = g_slist_append((GSList *)popup_menu_list, hexdump_menu_factory);

    /* main */
    main_menu_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", grp);
    gtk_item_factory_create_items_ac(main_menu_factory, nmenu_items, menu_items, NULL, 2);
    register_all_tap_menus();	/* must be done after creating the main menu */

    /* Initialize enabled/disabled state of menu items */
    set_menus_for_unsaved_capture_file(FALSE);
    set_menus_for_capture_file(FALSE);
#if 0
    /* Un-#if this when we actually implement Cut/Copy/Paste.
       Then make sure you enable them when they can be done. */
    set_menu_sensitivity(main_menu_factory, "/Edit/Cut", FALSE);
    set_menu_sensitivity(main_menu_factory, "/Edit/Copy", FALSE);
    set_menu_sensitivity(main_menu_factory, "/Edit/Paste", FALSE);
#endif

    set_menus_for_captured_packets(FALSE);
    set_menus_for_selected_packet(&cfile);
    set_menus_for_selected_tree_row(&cfile);

    /* init with an empty recent files list */
    clear_menu_recent_capture_file_cmd_cb(NULL, NULL);
  }
}

typedef struct _menu_item {
	char	*name;
	gboolean enabled;
	gboolean (*selected_packet_enabled)(frame_data *, epan_dissect_t *);
	gboolean (*selected_tree_row_enabled)(field_info *);
	struct _menu_item *parent;
	struct _menu_item *children;
	struct _menu_item *next;
} menu_item_t;

static menu_item_t tap_menu_tree_root;

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
 * it's passed a Boolean that's TRUE if a packet is selected and FALSE
 * otherwise, and should return TRUE if the tap will work now (which
 * might depend on whether a packet is selected and, if one is, on the
 * packet) and FALSE if not.
 *
 * "selected_tree_row_enabled" gets called by
 * "set_menus_for_selected_tree_row()"; it's passed a Boolean that's TRUE if
 * a protocol tree row is selected and FALSE otherwise, and should return
 * TRUE if the tap will work now (which might depend on whether a tree row
 * is selected and, if one is, on the tree row) and FALSE if not.
 */
void
register_tap_menu_item(char *name, GtkItemFactoryCallback callback,
    gboolean (*selected_packet_enabled)(frame_data *, epan_dissect_t *),
    gboolean (*selected_tree_row_enabled)(field_info *),
    gpointer callback_data)
{
	static const char toolspath[] = "/Analyze/";
	char *p;
	char *menupath;
	size_t menupathlen;
	GtkItemFactoryEntry *entry;
	menu_item_t *curnode, *child;

	/*
	 * The menu path must be relative.
	 */
	g_assert(*name != '/');

	/*
	 * Create any submenus required.
	 */
	curnode = &tap_menu_tree_root;
	p = name;
	while ((p = strchr(p, '/')) != NULL) {
		/*
		 * OK, everything between "name" and "p" is
		 * a menu relative subtree into which the menu item
		 * will be placed.
		 *
		 * Construct the absolute path name of that subtree.
		 */
		menupathlen = sizeof toolspath + (p - name);
		menupath = g_malloc(menupathlen);
		strcpy(menupath, toolspath);
		strncat(menupath, name, p - name);

		/*
		 * Does there exist an entry with that path at this
		 * level of the Analyze menu tree?
		 */
		for (child = curnode->children; child != NULL;
		    child = child->next) {
			if (strcmp(child->name, menupath) == 0)
				break;
		}
		if (child == NULL) {
			/*
			 * No.  Create such an item as a subtree, and
			 * add it to the Tools menu tree.
			 */
			entry = g_malloc0(sizeof (GtkItemFactoryEntry));
			entry->path = menupath;
			entry->item_type = "<Branch>";
			gtk_item_factory_create_item(main_menu_factory, entry,
			    NULL, 2);
			set_menu_sensitivity(main_menu_factory, menupath,
			    FALSE);	/* no children yet */
			child = g_malloc(sizeof (menu_item_t));
			child->name = menupath;
			child->selected_packet_enabled = NULL;
			child->selected_tree_row_enabled = NULL;
			child->enabled = FALSE;	/* no children yet */
			child->parent = curnode;
			child->children = NULL;
			child->next = curnode->children;
			curnode->children = child;
		} else {
			/*
			 * Yes.  We don't need "menupath" any more.
			 */
			g_free(menupath);
		}
		curnode = child;

		/*
		 * Skip over the '/' we found.
		 */
		p++;
	}

	/*
	 * Construct the main menu path for the menu item.
	 *
	 * "sizeof toolspath" includes the trailing '\0', so the sum
	 * of that and the length of "name" is enough to hold a string
	 * containing their concatenation.
	 */
	menupathlen = sizeof toolspath + strlen(name);
	menupath = g_malloc(menupathlen);
	strcpy(menupath, toolspath);
	strcat(menupath, name);

	/*
	 * Construct an item factory entry for the item, and add it to
	 * the main menu.
	 */
	entry = g_malloc0(sizeof (GtkItemFactoryEntry));
	entry->path = menupath;
	entry->callback = callback;
	gtk_item_factory_create_item(main_menu_factory, entry, callback_data, 2);
	set_menu_sensitivity(main_menu_factory, menupath, FALSE); /* no capture file yet */
	child = g_malloc(sizeof (menu_item_t));
	child->name = menupath;
	child->enabled = FALSE;	/* no capture file yet, hence no taps yet */
	child->selected_packet_enabled = selected_packet_enabled;
	child->selected_tree_row_enabled = selected_tree_row_enabled;
	child->parent = curnode;
	child->children = NULL;
	child->next = curnode->children;
	curnode->children = child;
}

/*
 * Enable/disable menu sensitivity.
 */
static void
set_menu_sensitivity(GtkItemFactory *ifactory, const gchar *path, gint val)
{
  GSList *menu_list;
  GtkWidget *menu_item;
  gchar *dup;
  gchar *dest;


  /* the underscore character regularly confuses things, as it will prevent finding 
   * the menu_item, so it has to be removed first */
  dup = g_strdup(path);
  dest = dup;
  while(*path) {
      if (*path != '_') {
        *dest = *path;
        dest++;
      }
      path++;
  }
  *dest = '\0';

  if (ifactory == NULL) {
    /*
     * Do it for all pop-up menus.
     */
    for (menu_list = popup_menu_list; menu_list != NULL;
         menu_list = g_slist_next(menu_list))
      set_menu_sensitivity(menu_list->data, dup, val);
  } else {
    /*
     * Do it for that particular menu.
     */
    if ((menu_item = gtk_item_factory_get_widget(ifactory, dup)) != NULL) {
      if (GTK_IS_MENU(menu_item)) {
        /*
         * "dup" refers to a submenu; "gtk_item_factory_get_widget()"
         * gets the menu, not the item that, when selected, pops up
         * the submenu.
         *
         * We have to change the latter item's sensitivity, so that
         * it shows up normally if sensitive and grayed-out if
         * insensitive.
         */
        menu_item = gtk_menu_get_attach_widget(GTK_MENU(menu_item));
      }
      gtk_widget_set_sensitive(menu_item, val);
    } else{
      /* be sure this menu item *is* existing */
      g_assert_not_reached();
    }
  }

  g_free(dup);
}

void
set_menu_object_data_meat(GtkItemFactory *ifactory, gchar *path, gchar *key, gpointer data)
{
	GtkWidget *menu = NULL;

	if ((menu = gtk_item_factory_get_widget(ifactory, path)) != NULL)
		OBJECT_SET_DATA(menu, key, data);
}

void
set_menu_object_data (gchar *path, gchar *key, gpointer data) {
  GSList *menu_list = popup_menu_list;
  gchar *shortpath = strrchr(path, '/');

  set_menu_object_data_meat(main_menu_factory, path, key, data);
  while (menu_list != NULL) {
  	set_menu_object_data_meat(menu_list->data, shortpath, key, data);
	menu_list = g_slist_next(menu_list);
  }
}


/* Recently used capture files submenu: 
 * Submenu containing the recently used capture files.
 * The capture filenames are always kept with the absolute path, to be independant
 * of the current path. 
 * They are only stored inside the labels of the submenu (no separate list). */

#define MENU_RECENT_FILES_PATH "/File/Open Recent"
#define MENU_RECENT_FILES_KEY "Recent File Name"

void
update_menu_recent_capture_file1(GtkWidget *widget, gpointer cnt) {
    gchar *widget_cf_name;

    widget_cf_name = OBJECT_GET_DATA(widget, MENU_RECENT_FILES_KEY);

    /* if this menu item is a file, count it */
    if (widget_cf_name) {
        (*(guint *)cnt)++;
    }
}


/* update the menu */
void
update_menu_recent_capture_file(GtkWidget *submenu_recent_files) {
    guint cnt = 0;

    gtk_container_foreach(GTK_CONTAINER(submenu_recent_files), 
		update_menu_recent_capture_file1, &cnt);

    /* make parent menu item sensitive only, if we have any valid files in the list */
    set_menu_sensitivity(main_menu_factory, MENU_RECENT_FILES_PATH, cnt);
}


/* remove the capture filename from the "Recent Files" menu */
void
remove_menu_recent_capture_file(GtkWidget *widget, gpointer unused _U_) {
    GtkWidget *submenu_recent_files;
    gchar *widget_cf_name;


    widget_cf_name = OBJECT_GET_DATA(widget, MENU_RECENT_FILES_KEY);
    g_free(widget_cf_name);

    /* get the submenu container item */
    submenu_recent_files = gtk_item_factory_get_widget(main_menu_factory, MENU_RECENT_FILES_PATH);

    /* XXX: is this all we need to do, to free the menu item and its label?
       The reference count of widget will go to 0, so it'll be freed;
       will that free the label? */
    gtk_container_remove(GTK_CONTAINER(submenu_recent_files), widget);
}


/* callback, if the user pushed the <clear file list> item */
static void
clear_menu_recent_capture_file_cmd_cb(GtkWidget *w _U_, gpointer unused _U_) {
    GtkWidget *submenu_recent_files;


    submenu_recent_files = gtk_item_factory_get_widget(main_menu_factory, MENU_RECENT_FILES_PATH);

    gtk_container_foreach(GTK_CONTAINER(submenu_recent_files), 
		remove_menu_recent_capture_file, NULL);

    update_menu_recent_capture_file(submenu_recent_files);
}


/* callback, if the user pushed a recent file submenu item */
void
menu_open_recent_file_cmd(GtkWidget *w)
{
	GtkWidget *submenu_recent_files;
	GtkWidget *menu_item_child;
	gchar     *cf_name;
	int       err;

	submenu_recent_files = gtk_item_factory_get_widget(main_menu_factory, MENU_RECENT_FILES_PATH);

	/* get capture filename from the menu item label */
	menu_item_child = (GTK_BIN(w))->child;
	gtk_label_get(GTK_LABEL(menu_item_child), &cf_name);

	/* open and read the capture file (this will close an existing file) */
	if ((err = cf_open(cf_name, FALSE, &cfile)) == 0) {
		cf_read(&cfile);
	} else {
		/* the capture file isn't existing any longer, remove menu item */
		/* XXX: ask user to remove item, it's maybe only a temporary problem */
		remove_menu_recent_capture_file(w, NULL);
	}

	update_menu_recent_capture_file(submenu_recent_files);
}

static void menu_open_recent_file_answered_cb(gpointer dialog _U_, gint btn, gpointer data _U_)
{
    switch(btn) {
    case(ESD_BTN_YES):
        /* save file first */
        file_save_as_cmd(after_save_open_recent_file, data);
        break;
    case(ESD_BTN_NO):
        menu_open_recent_file_cmd(data);
        break;
    case(ESD_BTN_CANCEL):
        break;
    default:
        g_assert_not_reached();
    }
}

void
menu_open_recent_file_cmd_cb(GtkWidget *widget, gpointer data _U_) {
  gpointer  dialog;


  if((cfile.state != FILE_CLOSED) && !cfile.user_saved) {
    /* user didn't saved his current file, ask him */
    dialog = simple_dialog(ESD_TYPE_WARN | ESD_TYPE_MODAL, 
                ESD_BTN_YES | ESD_BTN_NO | ESD_BTN_CANCEL, 
                PRIMARY_TEXT_START "Save capture file before opening a new one?" PRIMARY_TEXT_END "\n\n"
                "If you open a new capture file without saving, your current capture data will be discarded.");
    simple_dialog_set_cb(dialog, menu_open_recent_file_answered_cb, widget);
  } else {
    /* unchanged file */
    menu_open_recent_file_cmd(widget);
  }
}

/* add the capture filename (with an absolute path) to the "Recent Files" menu */
void
add_menu_recent_capture_file_absolute(gchar *cf_name) {
	GtkWidget *submenu_recent_files;
	GList *menu_item_list;
	GList *li;
	gchar *widget_cf_name;
	GtkWidget *menu_item;
	guint cnt;


	/* get the submenu container item */
	submenu_recent_files = gtk_item_factory_get_widget(main_menu_factory, MENU_RECENT_FILES_PATH);

	/* convert container to a GList */
	menu_item_list = gtk_container_children(GTK_CONTAINER(submenu_recent_files));

	/* iterate through list items of menu_item_list, 
	 * removing special items, a maybe duplicate entry and every item above count_max */
	cnt = 1;
	for (li = g_list_first(menu_item_list); li; li = li->next, cnt++) {
		/* get capture filename */
		menu_item = GTK_WIDGET(li->data);
		widget_cf_name = OBJECT_GET_DATA(menu_item, MENU_RECENT_FILES_KEY);

		/* if this element string is one of our special items or
		 * already in the list or 
		 * this element is above maximum count (too old), remove it */
		if (!widget_cf_name ||
		    strncmp(widget_cf_name, cf_name, 1000) == 0 ||
		    cnt >= prefs.gui_recent_files_count_max) {
			remove_menu_recent_capture_file(li->data, NULL);
			cnt--;
		}
	}

	g_list_free(menu_item_list);

	/* add new item at latest position */
	menu_item = gtk_menu_item_new_with_label(cf_name);
	widget_cf_name = g_strdup(cf_name);
	OBJECT_SET_DATA(menu_item, MENU_RECENT_FILES_KEY, widget_cf_name);
	gtk_menu_prepend (GTK_MENU(submenu_recent_files), menu_item);
	SIGNAL_CONNECT_OBJECT(GTK_OBJECT(menu_item), "activate", 
		menu_open_recent_file_cmd_cb, (GtkObject *) menu_item);
	gtk_widget_show (menu_item);

	/* add seperator at last position */
	menu_item = gtk_menu_item_new();
	gtk_menu_append (GTK_MENU(submenu_recent_files), menu_item);
	gtk_widget_show (menu_item);

	/* add new "clear list" item at last position */
	menu_item = gtk_menu_item_new_with_label("<clear file list>");
	gtk_menu_append (GTK_MENU(submenu_recent_files), menu_item);
	SIGNAL_CONNECT_OBJECT(GTK_OBJECT(menu_item), "activate", 
		clear_menu_recent_capture_file_cmd_cb, (GtkObject *) menu_item);
	gtk_widget_show (menu_item);

	update_menu_recent_capture_file(submenu_recent_files);
}


/* add the capture filename to the "Recent Files" menu */
/* (will change nothing, if this filename is already in the menu) */
void
add_menu_recent_capture_file(gchar *cf_name) {
	gchar *curr;
	gchar *absolute;
	
	
	/* if this filename is an absolute path, we can use it directly */
	if (g_path_is_absolute(cf_name)) {
		add_menu_recent_capture_file_absolute(cf_name);
		return;
	}

	/* this filename is not an absolute path, prepend the current dir */
	curr = g_get_current_dir();
	absolute = g_strdup_printf("%s%s%s", curr, G_DIR_SEPARATOR_S, cf_name);
	add_menu_recent_capture_file_absolute(absolute);
	g_free(curr);
	g_free(absolute);
}



/* write a single menu item widget label to the user's recent file */
/* helper, for menu_recent_file_write_all() */
void menu_recent_file_write(GtkWidget *widget, gpointer data) {
	gchar     *cf_name;
	FILE      *rf = (FILE *) data;


	/* get capture filename from the menu item label */
	cf_name = OBJECT_GET_DATA(widget, MENU_RECENT_FILES_KEY);

	if (cf_name) {
	    fprintf (rf, RECENT_KEY_CAPTURE_FILE ": %s\n", cf_name);
	}
}


/* write all capture filenames of the menu to the user's recent file */
void
menu_recent_file_write_all(FILE *rf) {
	GtkWidget *submenu_recent_files;


	submenu_recent_files = gtk_item_factory_get_widget(main_menu_factory, MENU_RECENT_FILES_PATH);

	gtk_container_foreach(GTK_CONTAINER(submenu_recent_files), 
		menu_recent_file_write, rf);
}


static void
main_toolbar_show_cb(GtkWidget *w _U_, gpointer d _U_)
{

    /* save current setting in recent */
    recent.main_toolbar_show = GTK_CHECK_MENU_ITEM(w)->active;

    main_widgets_rearrange();
}


static void
filter_toolbar_show_cb(GtkWidget *w _U_, gpointer d _U_)
{

    /* save current setting in recent */
    recent.filter_toolbar_show = GTK_CHECK_MENU_ITEM(w)->active;

    main_widgets_rearrange();
}


static void
packet_list_show_cb(GtkWidget *w _U_, gpointer d _U_)
{

    /* save current setting in recent */
    recent.packet_list_show = GTK_CHECK_MENU_ITEM(w)->active;

    main_widgets_rearrange();
}


static void
tree_view_show_cb(GtkWidget *w _U_, gpointer d _U_)
{

    /* save current setting in recent */
    recent.tree_view_show = GTK_CHECK_MENU_ITEM(w)->active;

    main_widgets_rearrange();
}


static void
byte_view_show_cb(GtkWidget *w _U_, gpointer d _U_)
{

    /* save current setting in recent */
    recent.byte_view_show = GTK_CHECK_MENU_ITEM(w)->active;

    main_widgets_rearrange();
}


static void
statusbar_show_cb(GtkWidget *w _U_, gpointer d _U_)
{

    /* save current setting in recent */
    recent.statusbar_show = GTK_CHECK_MENU_ITEM(w)->active;

    main_widgets_rearrange();
}


static void 
timestamp_absolute_cb(GtkWidget *w _U_, gpointer d _U_)
{
    if (recent.gui_time_format != TS_ABSOLUTE) {
        timestamp_type          = TS_ABSOLUTE;
        recent.gui_time_format  = timestamp_type;
        change_time_formats(&cfile);
    }
}

static void 
timestamp_absolute_date_cb(GtkWidget *w _U_, gpointer d _U_)
{
    if (recent.gui_time_format != TS_ABSOLUTE_WITH_DATE) {
        timestamp_type          = TS_ABSOLUTE_WITH_DATE;
        recent.gui_time_format  = timestamp_type;
        change_time_formats(&cfile);
    }
}

static void 
timestamp_relative_cb(GtkWidget *w _U_, gpointer d _U_)
{
    if (recent.gui_time_format != TS_RELATIVE) {
        timestamp_type          = TS_RELATIVE;
        recent.gui_time_format  = timestamp_type;
        change_time_formats(&cfile);
    }
}

static void 
timestamp_delta_cb(GtkWidget *w _U_, gpointer d _U_)
{
    if (recent.gui_time_format != TS_DELTA) {
        timestamp_type          = TS_DELTA;
        recent.gui_time_format  = timestamp_type;
        change_time_formats(&cfile);
    }
}

static void 
name_resolution_mac_cb(GtkWidget *w _U_, gpointer d _U_)
{
    if (GTK_CHECK_MENU_ITEM(w)->active) {
        g_resolv_flags |= RESOLV_MAC;
    } else {
        g_resolv_flags &= ~RESOLV_MAC;
    }
}

static void 
name_resolution_network_cb(GtkWidget *w _U_, gpointer d _U_)
{
    if (GTK_CHECK_MENU_ITEM(w)->active) {
        g_resolv_flags |= RESOLV_NETWORK;
    } else {
        g_resolv_flags &= ~RESOLV_NETWORK;
    }
}

static void 
name_resolution_transport_cb(GtkWidget *w _U_, gpointer d _U_)
{
    if (GTK_CHECK_MENU_ITEM(w)->active) {
        g_resolv_flags |= RESOLV_TRANSPORT;
    } else {
        g_resolv_flags &= ~RESOLV_TRANSPORT;
    }
}

static void 
auto_scroll_live_cb(GtkWidget *w _U_, gpointer d _U_)
{
    auto_scroll_live = GTK_CHECK_MENU_ITEM(w)->active;
}


/* the recent file read has finished, update the menu corresponding */
void
menu_recent_read_finished(void) {
    GtkWidget *menu = NULL;

    menu = gtk_item_factory_get_widget(main_menu_factory, "/View/Show/Main Toolbar");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), recent.main_toolbar_show);

    menu = gtk_item_factory_get_widget(main_menu_factory, "/View/Show/Filter Toolbar");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), recent.filter_toolbar_show);

    menu = gtk_item_factory_get_widget(main_menu_factory, "/View/Show/Packet List");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), recent.packet_list_show);

    menu = gtk_item_factory_get_widget(main_menu_factory, "/View/Show/Packet Details");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), recent.tree_view_show);

    menu = gtk_item_factory_get_widget(main_menu_factory, "/View/Show/Packet Bytes");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), recent.byte_view_show);

    menu = gtk_item_factory_get_widget(main_menu_factory, "/View/Show/Status Bar");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), recent.statusbar_show);

    menu = gtk_item_factory_get_widget(main_menu_factory, "/View/Name Resolution/Enable for MAC Layer");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), g_resolv_flags & RESOLV_MAC);

    menu = gtk_item_factory_get_widget(main_menu_factory, "/View/Name Resolution/Enable for Network Layer");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), g_resolv_flags & RESOLV_NETWORK);

    menu = gtk_item_factory_get_widget(main_menu_factory, "/View/Name Resolution/Enable for Transport Layer");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), g_resolv_flags & RESOLV_TRANSPORT);

    menu = gtk_item_factory_get_widget(main_menu_factory, "/View/Auto Scroll in Live Capture");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), auto_scroll_live);

    main_widgets_rearrange();

    /* don't change the time format, if we had a command line value */
    if (timestamp_type != TS_NOT_SET) {
        recent.gui_time_format = timestamp_type;
    }

    switch(recent.gui_time_format) {
    case(TS_ABSOLUTE):
        menu = gtk_item_factory_get_widget(main_menu_factory, 
            "/View/Time Display Format/Time of Day");
        /* set_active will not trigger the callback when activating an active item! */
        recent.gui_time_format = -1;
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), FALSE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), TRUE);
        break;
    case(TS_ABSOLUTE_WITH_DATE):
        menu = gtk_item_factory_get_widget(main_menu_factory, 
            "/View/Time Display Format/Date and Time of Day");
        recent.gui_time_format = -1;
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), TRUE);
        break;
    case(TS_RELATIVE):
        menu = gtk_item_factory_get_widget(main_menu_factory, 
            "/View/Time Display Format/Seconds Since Beginning of Capture");
        recent.gui_time_format = -1;
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), TRUE);
        break;
    case(TS_DELTA):
        menu = gtk_item_factory_get_widget(main_menu_factory, 
            "/View/Time Display Format/Seconds Since Previous Packet");
        recent.gui_time_format = -1;
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), TRUE);
        break;
    default:
        g_assert_not_reached();
    }
}


gint
popup_menu_handler(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    GtkWidget *menu = (GtkWidget *)data;
    GdkEventButton *event_button = NULL;
    gint row, column;

    if(widget == NULL || event == NULL || data == NULL) {
        return FALSE;
    }

    /*
     * If we ever want to make the menu differ based on what row
     * and/or column we're above, we'd use "eth_clist_get_selection_info()"
     * to find the row and column number for the coordinates; a CTree is,
     * I guess, like a CList with one column(?) and the expander widget
     * as a pixmap.
     */
    /* Check if we are on packet_list object */
    if (widget == OBJECT_GET_DATA(popup_menu_object, E_MPACKET_LIST_KEY)) {
        if (packet_list_get_event_row_column(widget, (GdkEventButton *)event,
                                             &row, &column)) {
            OBJECT_SET_DATA(popup_menu_object, E_MPACKET_LIST_ROW_KEY,
                            GINT_TO_POINTER(row));
            OBJECT_SET_DATA(popup_menu_object, E_MPACKET_LIST_COL_KEY,
                            GINT_TO_POINTER(column));
            packet_list_set_selected_row(row);
        }
    }

    /* Check if we are on tree_view object */
    if (widget == tree_view) {
        tree_view_select(widget, (GdkEventButton *) event);
    }

    /* Check if we are on byte_view object */
    if(widget == get_notebook_bv_ptr(byte_nb_ptr)) {
        byte_view_select(widget, (GdkEventButton *) event);
    }

    if(event->type == GDK_BUTTON_PRESS) {
        event_button = (GdkEventButton *) event;

        /* To qoute the "Gdk Event Structures" doc:
         * "Normally button 1 is the left mouse button, 2 is the middle button, and 3 is the right button" */
        if(event_button->button == 3) {
            gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
                           event_button->button,
                           event_button->time);
            SIGNAL_EMIT_STOP_BY_NAME(widget, "button_press_event");
            return TRUE;
        }
    }
#if GTK_MAJOR_VERSION >= 2
    /* GDK_2BUTTON_PRESS is a doubleclick -> expand/collapse tree row */
    /* GTK version 1 seems to be doing this automatically */
    if (widget == tree_view && event->type == GDK_2BUTTON_PRESS) {
        GtkTreePath      *path;

        if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget),
                                          (gint) (((GdkEventButton *)event)->x),
                                          (gint) (((GdkEventButton *)event)->y),
                                          &path, NULL, NULL, NULL))
        {
            if (gtk_tree_view_row_expanded(GTK_TREE_VIEW(widget), path))
                gtk_tree_view_collapse_row(GTK_TREE_VIEW(widget), path);
            else
                gtk_tree_view_expand_row(GTK_TREE_VIEW(widget), path,
                                         FALSE);
            gtk_tree_path_free(path);
        }
    }
#endif
    return FALSE;
}

/* Enable or disable menu items based on whether you have a capture file
   you've finished reading. */
void
set_menus_for_capture_file(gboolean have_capture_file)
{
  set_menu_sensitivity(main_menu_factory, "/File/Open...", have_capture_file);
  set_menu_sensitivity(main_menu_factory, "/File/Open Recent", have_capture_file);
  set_menu_sensitivity(main_menu_factory, "/File/Close", have_capture_file);
  set_menu_sensitivity(main_menu_factory, "/File/Save As...",
      have_capture_file);
  set_menu_sensitivity(main_menu_factory, "/File/Export", have_capture_file);
  set_menu_sensitivity(main_menu_factory, "/View/Reload", have_capture_file);
  set_toolbar_for_capture_file(have_capture_file);
  packets_bar_update();
}

/* Enable or disable menu items based on whether you have an unsaved
   capture file you've finished reading. */
void
set_menus_for_unsaved_capture_file(gboolean have_unsaved_capture_file)
{
  set_menu_sensitivity(main_menu_factory, "/File/Save",
      have_unsaved_capture_file);
  set_toolbar_for_unsaved_capture_file(have_unsaved_capture_file);
}

/* Enable or disable menu items based on whether there's a capture in
   progress. */
void
set_menus_for_capture_in_progress(gboolean capture_in_progress)
{
  set_menu_sensitivity(main_menu_factory, "/File/Open...",
      !capture_in_progress);
  set_menu_sensitivity(main_menu_factory, "/File/Open Recent", 
      !capture_in_progress);
#ifdef HAVE_LIBPCAP
  set_menu_sensitivity(main_menu_factory, "/Capture/Start...",
      !capture_in_progress);
  set_menu_sensitivity(main_menu_factory, "/Capture/Stop",
      capture_in_progress);
#endif /* HAVE_LIBPCAP */
  set_toolbar_for_capture_in_progress(capture_in_progress);
}

/* Enable or disable menu items based on whether you have some captured
   packets. */
static gboolean
walk_menu_tree_for_captured_packets(menu_item_t *node,
    gboolean have_captured_packets)
{
	gboolean is_enabled;
	menu_item_t *child;

	/*
	 * Is this a leaf node or an interior node?
	 */
	if (node->children == NULL) {
		/*
		 * It's a leaf node.
		 *
		 * If it has no "selected_packet_enabled()" or
		 * "selected_tree_row_enabled()" routines, we enable
		 * it.  This allows tap windows to be popped up even
		 * if you have no capture file; this is done to let
		 * the user pop up multiple tap windows before reading
		 * in a capture file, so that they can be processed in
		 * parallel while the capture file is being read rather
		 * than one at at time as you pop up the windows, and to
		 * let the user pop up tap windows before starting an
		 * "Update list of packets in real time" capture, so that
		 * the statistics can be displayed while the capture is
		 * in progress.
		 *
		 * If it has either of those routines, we disable it for
		 * now - as long as, when a capture is first available,
		 * we don't get called after a packet or tree row is
		 * selected, that's OK.
		 * XXX - that should be done better.
		 */
		if (node->selected_packet_enabled == NULL &&
		    node->selected_tree_row_enabled == NULL)
			node->enabled = TRUE;
		else
			node->enabled = FALSE;
	} else {
		/*
		 * It's an interior node; call
		 * "walk_menu_tree_for_captured_packets()" on all its
		 * children and, if any of them are enabled, enable
		 * this node, otherwise disable it.
		 *
		 * XXX - should we just leave all interior nodes enabled?
		 * Which is a better UI choice?
		 */
		is_enabled = FALSE;
		for (child = node->children; child != NULL; child =
		    child->next) {
			if (walk_menu_tree_for_captured_packets(child,
			    have_captured_packets))
				is_enabled = TRUE;
		}
		node->enabled = is_enabled;
	}

	/*
	 * The root node doesn't correspond to a menu tree item; it
	 * has a null name pointer.
	 */
	if (node->name != NULL) {
		set_menu_sensitivity(main_menu_factory, node->name,
		    node->enabled);
	}
	return node->enabled;
}

void
set_menus_for_captured_packets(gboolean have_captured_packets)
{
  set_menu_sensitivity(main_menu_factory, "/File/Print...",
      have_captured_packets);
  set_menu_sensitivity(packet_list_menu_factory, "/Print...",
      have_captured_packets);
  set_menu_sensitivity(main_menu_factory, "/Edit/Find Packet...",
      have_captured_packets);
  set_menu_sensitivity(main_menu_factory, "/Edit/Find Next",
      have_captured_packets);
  set_menu_sensitivity(main_menu_factory, "/Edit/Find Previous",
      have_captured_packets);
  set_menu_sensitivity(main_menu_factory, "/Edit/Go To First Packet",
      have_captured_packets);
  set_menu_sensitivity(main_menu_factory, "/Edit/Go To Last Packet",
      have_captured_packets);
  set_menu_sensitivity(main_menu_factory, "/Edit/Go To Packet...",
      have_captured_packets);
  set_menu_sensitivity(main_menu_factory, "/View/Zoom In",
      have_captured_packets);
  set_menu_sensitivity(main_menu_factory, "/View/Zoom Out",
      have_captured_packets);
  set_menu_sensitivity(main_menu_factory, "/View/Normal Size",
      have_captured_packets);
  set_menu_sensitivity(main_menu_factory, "/View/Coloring Rules...",
      have_captured_packets);
  set_menu_sensitivity(packet_list_menu_factory, "/Coloring Rules...",
      have_captured_packets);
  set_menu_sensitivity(main_menu_factory, "/Analyze/Summary",
      have_captured_packets);
  set_menu_sensitivity(main_menu_factory, "/Analyze/Protocol Hierarchy Statistics", 
      have_captured_packets);
      
  walk_menu_tree_for_captured_packets(&tap_menu_tree_root,
      have_captured_packets);
  set_toolbar_for_captured_packets(have_captured_packets);
  packets_bar_update();
}

/* Enable or disable menu items based on whether a packet is selected and,
   if so, on the properties of the packet. */
static gboolean
walk_menu_tree_for_selected_packet(menu_item_t *node, frame_data *fd,
    epan_dissect_t *edt)
{
	gboolean is_enabled;
	menu_item_t *child;

	/*
	 * Is this a leaf node or an interior node?
	 */
	if (node->children == NULL) {
		/*
		 * It's a leaf node.
		 *
		 * If it has no "selected_packet_enabled()" routine,
		 * leave its enabled/disabled status alone - it
		 * doesn't depend on whether we have a packet selected
		 * or not or on the selected packet.
		 *
		 * If it has a "selected_packet_enabled()" routine,
		 * call it and set the item's enabled/disabled status
		 * based on its return value.
		 */
		if (node->selected_packet_enabled != NULL)
			node->enabled = node->selected_packet_enabled(fd, edt);
	} else {
		/*
		 * It's an interior node; call
		 * "walk_menu_tree_for_selected_packet()" on all its
		 * children and, if any of them are enabled, enable
		 * this node, otherwise disable it.
		 *
		 * XXX - should we just leave all interior nodes enabled?
		 * Which is a better UI choice?
		 */
		is_enabled = FALSE;
		for (child = node->children; child != NULL; child =
		    child->next) {
			if (walk_menu_tree_for_selected_packet(child, fd, edt))
				is_enabled = TRUE;
		}
		node->enabled = is_enabled;
	}

	/*
	 * The root node doesn't correspond to a menu tree item; it
	 * has a null name pointer.
	 */
	if (node->name != NULL) {
		set_menu_sensitivity(main_menu_factory, node->name,
		    node->enabled);
	}
	return node->enabled;
}

void
set_menus_for_selected_packet(capture_file *cf)
{
  set_menu_sensitivity(main_menu_factory, "/Edit/Mark Packet",
      cf->current_frame != NULL);
  set_menu_sensitivity(packet_list_menu_factory, "/Mark Packet",
      cf->current_frame != NULL);
  set_menu_sensitivity(main_menu_factory, "/Edit/Time Reference",
      cf->current_frame != NULL);
  set_menu_sensitivity(packet_list_menu_factory, "/Time Reference",
      cf->current_frame != NULL);
  set_menu_sensitivity(main_menu_factory, "/Edit/Mark All Packets",
      cf->current_frame != NULL);
  set_menu_sensitivity(main_menu_factory, "/Edit/Unmark All Packets",
      cf->current_frame != NULL);
  set_menu_sensitivity(main_menu_factory, "/View/Collapse All",
      cf->current_frame != NULL);
  set_menu_sensitivity(tree_view_menu_factory, "/Collapse All",
      cf->current_frame != NULL);
  set_menu_sensitivity(main_menu_factory, "/View/Expand All",
      cf->current_frame != NULL);
  set_menu_sensitivity(tree_view_menu_factory, "/Expand All",
      cf->current_frame != NULL);
  set_menu_sensitivity(main_menu_factory, "/View/Show Packet In New Window",
      cf->current_frame != NULL);
  set_menu_sensitivity(packet_list_menu_factory, "/Show Packet In New Window",
      cf->current_frame != NULL);
  set_menu_sensitivity(main_menu_factory, "/Analyze/Follow TCP Stream",
      cf->current_frame != NULL ? (cf->edt->pi.ipproto == IP_PROTO_TCP) : FALSE);
  set_menu_sensitivity(NULL, "/Follow TCP Stream",
      cf->current_frame != NULL ? (cf->edt->pi.ipproto == IP_PROTO_TCP) : FALSE);
  set_menu_sensitivity(main_menu_factory, "/Analyze/Decode As...",
      cf->current_frame != NULL && decode_as_ok());
  set_menu_sensitivity(NULL, "/Decode As...",
      cf->current_frame != NULL && decode_as_ok());
  set_menu_sensitivity(tree_view_menu_factory, "/Resolve Name",
      cf->current_frame != NULL && g_resolv_flags == 0);
  set_menu_sensitivity(main_menu_factory, "/Analyze/TCP Stream Analysis",
      cf->current_frame != NULL ? (cf->edt->pi.ipproto == IP_PROTO_TCP) : FALSE);
  set_menu_sensitivity(packet_list_menu_factory, "/Match",
      cf->current_frame != NULL);
  set_menu_sensitivity(packet_list_menu_factory, "/Prepare",
      cf->current_frame != NULL);

  walk_menu_tree_for_selected_packet(&tap_menu_tree_root, cf->current_frame,
      cf->edt);
  packets_bar_update();
}

/* Enable or disable menu items based on whether a tree row is selected
   and, if so, on the properties of the tree row. */
static gboolean
walk_menu_tree_for_selected_tree_row(menu_item_t *node, field_info *fi)
{
	gboolean is_enabled;
	menu_item_t *child;

	/*
	 * Is this a leaf node or an interior node?
	 */
	if (node->children == NULL) {
		/*
		 * It's a leaf node.
		 *
		 * If it has no "selected_tree_row_enabled()" routine,
		 * leave its enabled/disabled status alone - it
		 * doesn't depend on whether we have a tree row selected
		 * or not or on the selected tree row.
		 *
		 * If it has a "selected_tree_row_enabled()" routine,
		 * call it and set the item's enabled/disabled status
		 * based on its return value.
		 */
		if (node->selected_tree_row_enabled != NULL)
			node->enabled = node->selected_tree_row_enabled(fi);
	} else {
		/*
		 * It's an interior node; call
		 * "walk_menu_tree_for_selected_tree_row()" on all its
		 * children and, if any of them are enabled, enable
		 * this node, otherwise disable it.
		 *
		 * XXX - should we just leave all interior nodes enabled?
		 * Which is a better UI choice?
		 */
		is_enabled = FALSE;
		for (child = node->children; child != NULL; child =
		    child->next) {
			if (walk_menu_tree_for_selected_tree_row(child, fi))
				is_enabled = TRUE;
		}
		node->enabled = is_enabled;
	}

	/*
	 * The root node doesn't correspond to a menu tree item; it
	 * has a null name pointer.
	 */
	if (node->name != NULL) {
		set_menu_sensitivity(main_menu_factory, node->name,
		    node->enabled);
	}
	return node->enabled;
}

void
set_menus_for_selected_tree_row(capture_file *cf)
{
  gboolean properties;


  set_menu_sensitivity(main_menu_factory, "/File/Export/Selected Packet Bytes...", 
      cf->finfo_selected != NULL);  
  set_menu_sensitivity(tree_view_menu_factory, "/Export Selected Packet Bytes...", 
      cf->finfo_selected != NULL);
  set_menu_sensitivity(hexdump_menu_factory, "/Export Selected Packet Bytes...", 
      cf->finfo_selected != NULL);
  
  if (cf->finfo_selected != NULL) {
	header_field_info *hfinfo = cf->finfo_selected->hfinfo;
	if (hfinfo->parent == -1) {
	  properties = prefs_is_registered_protocol(hfinfo->abbrev);
	} else {
	  properties = prefs_is_registered_protocol(proto_registrar_get_abbrev(hfinfo->parent));
	}
	set_menu_sensitivity(main_menu_factory,
	  "/Edit/Go To Corresponding Packet", hfinfo->type == FT_FRAMENUM);
	set_menu_sensitivity(tree_view_menu_factory,
	  "/Go To Corresponding Packet", hfinfo->type == FT_FRAMENUM);
	set_menu_sensitivity(main_menu_factory, "/Analyze/Match",
	  proto_can_match_selected(cf->finfo_selected, cf->edt));
	set_menu_sensitivity(tree_view_menu_factory, "/Match",
	  proto_can_match_selected(cf->finfo_selected, cf->edt));
	set_menu_sensitivity(main_menu_factory, "/Analyze/Prepare",
	  proto_can_match_selected(cf->finfo_selected, cf->edt));
	set_menu_sensitivity(tree_view_menu_factory, "/Prepare",
	  proto_can_match_selected(cf->finfo_selected, cf->edt));
	set_menu_sensitivity(tree_view_menu_factory, "/Protocol Properties...",
	  properties);
  } else {
	set_menu_sensitivity(main_menu_factory,
	    "/Edit/Go To Corresponding Packet", FALSE);
	set_menu_sensitivity(tree_view_menu_factory,
	    "/Go To Corresponding Packet", FALSE);
	set_menu_sensitivity(main_menu_factory, "/Analyze/Match", FALSE);
	set_menu_sensitivity(tree_view_menu_factory, "/Match", FALSE);
	set_menu_sensitivity(main_menu_factory, "/Analyze/Prepare", FALSE);
	set_menu_sensitivity(tree_view_menu_factory, "/Prepare", FALSE);
	set_menu_sensitivity(tree_view_menu_factory, "/Protocol Properties...",
	  FALSE);
  }

  walk_menu_tree_for_selected_tree_row(&tap_menu_tree_root, cf->finfo_selected);
}
