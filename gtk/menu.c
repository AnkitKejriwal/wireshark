/* menu.c
 * Menu routines
 *
 * $Id: menu.c,v 1.67 2002/09/05 18:47:46 jmayer Exp $
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

#include "../menu.h"

#include "main.h"
#include "menu.h"
#include <epan/packet.h>
#include <epan/resolv.h>
#include "prefs.h"
#include "capture_dlg.h"
#include "color_dlg.h"
#include "file_dlg.h"
#include "filter_prefs.h"
#include "find_dlg.h"
#include "goto_dlg.h"
#include "summary_dlg.h"
#include "display_opts.h"
#include "prefs_dlg.h"
#include "packet_win.h"
#include "print.h"
#include "follow_dlg.h"
#include "decode_as_dlg.h"
#include "help_dlg.h"
#include "proto_dlg.h"
#include "proto_hier_stats_dlg.h"
#include "keys.h"
#include <epan/plugins.h>
#include "tcp_graph.h"
#include <epan/epan_dissect.h>

GtkWidget *popup_menu_object;

#define GTK_MENU_FUNC(a) ((GtkItemFactoryCallback)(a))

static void menus_init(void);
static void set_menu_sensitivity (gchar *, gint);

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
               "<CheckItem>"      -> create a check item
               "<ToggleItem>"     -> create a toggle item
               "<RadioItem>"      -> create a radio item
               <path>             -> path of a radio item to link against
               "<Separator>"      -> create a separator
               "<Branch>"         -> create an item to hold sub items (optional)
               "<LastBranch>"     -> create a right justified branch
    */

/* main menu */
static GtkItemFactoryEntry menu_items[] =
{
  {"/_File", NULL, NULL, 0, "<Branch>" },
  {"/File/_Open...", "<control>O", GTK_MENU_FUNC(file_open_cmd_cb), 0, NULL},
  {"/File/_Close", "<control>W", GTK_MENU_FUNC(file_close_cmd_cb), 0, NULL},
  {"/File/_Save", "<control>S", GTK_MENU_FUNC(file_save_cmd_cb), 0, NULL},
  {"/File/Save _As...", NULL, GTK_MENU_FUNC(file_save_as_cmd_cb), 0, NULL},
  {"/File/_Reload", "<control>R", GTK_MENU_FUNC(file_reload_cmd_cb), 0, NULL},
  {"/File/<separator>", NULL, NULL, 0, "<Separator>"},
  {"/File/_Print...", NULL, GTK_MENU_FUNC(file_print_cmd_cb), 0, NULL},
  {"/File/Print Pac_ket", "<control>P", GTK_MENU_FUNC(file_print_packet_cmd_cb), 0, NULL},
  {"/File/<separator>", NULL, NULL, 0, "<Separator>"},
  {"/File/_Quit", "<control>Q", GTK_MENU_FUNC(file_quit_cmd_cb), 0, NULL},
  {"/_Edit", NULL, NULL, 0, "<Branch>" },
#if 0
  /* Un-#if this when we actually implement Cut/Copy/Paste. */
  {"/Edit/Cut", "<control>X", NULL, 0, NULL},
  {"/Edit/Copy", "<control>C", NULL, 0, NULL},
  {"/Edit/Paste", "<control>V", NULL, 0, NULL},
  {"/Edit/<separator>", NULL, NULL, 0, "<Separator>"},
#endif
  {"/Edit/_Find Frame...", "<control>F", GTK_MENU_FUNC(find_frame_cb), 0, NULL},
  {"/Edit/Find _Next", "<control>N", GTK_MENU_FUNC(find_next_cb), 0, NULL},
  {"/Edit/Find _Previous", "<control>B", GTK_MENU_FUNC(find_previous_cb), 0, NULL},
  {"/Edit/_Go To Frame...", "<control>G", GTK_MENU_FUNC(goto_frame_cb), 0, NULL},
  {"/Edit/<separator>", NULL, NULL, 0, "<Separator>"},
  {"/Edit/_Mark Frame", "<control>M", GTK_MENU_FUNC(mark_frame_cb), 0, NULL},
  {"/Edit/Mark _All Frames", NULL, GTK_MENU_FUNC(mark_all_frames_cb), 0, NULL},
  {"/Edit/_Unmark All Frames", NULL, GTK_MENU_FUNC(unmark_all_frames_cb), 0, NULL},
  {"/Edit/<separator>", NULL, NULL, 0, "<Separator>"},
  {"/Edit/_Preferences...", NULL, GTK_MENU_FUNC(prefs_cb), 0, NULL},
#ifdef HAVE_LIBPCAP
  {"/Edit/_Capture Filters...", NULL, GTK_MENU_FUNC(cfilter_dialog_cb), 0, NULL},
#endif
  {"/Edit/_Display Filters...", NULL, GTK_MENU_FUNC(dfilter_dialog_cb), 0, NULL},
  {"/Edit/P_rotocols...", NULL, GTK_MENU_FUNC(proto_cb), 0, NULL},
#ifdef HAVE_LIBPCAP
  {"/_Capture", NULL, NULL, 0, "<Branch>" },
  {"/Capture/_Start...", "<control>K", GTK_MENU_FUNC(capture_prep_cb), 0, NULL},
  /*
   * XXX - this doesn't yet work in Win32.
   */
#ifndef _WIN32
  {"/Capture/S_top", "<control>E", GTK_MENU_FUNC(capture_stop_cb), 0, NULL},
#endif /* _WIN32 */
#endif /* HAVE_LIBPCAP */
  {"/_Display", NULL, NULL, 0, "<Branch>" },
  {"/Display/_Options...", NULL, GTK_MENU_FUNC(display_opt_cb), 0, NULL},
  {"/Display/_Match", NULL, NULL, 0, "<Branch>" },
  {"/Display/Match/_Selected", NULL, GTK_MENU_FUNC(match_selected_cb_replace_ptree), 0, NULL},
  {"/Display/Match/_Not Selected", NULL, GTK_MENU_FUNC(match_selected_cb_not_ptree), 0, NULL},
  {"/Display/Match/_And Selected", NULL, GTK_MENU_FUNC(match_selected_cb_and_ptree), 0, NULL},
  {"/Display/Match/_Or Selected", NULL, GTK_MENU_FUNC(match_selected_cb_or_ptree), 0, NULL},
  {"/Display/Match/A_nd Not Selected", NULL, GTK_MENU_FUNC(match_selected_cb_and_ptree_not), 0, NULL},
  {"/Display/Match/O_r Not Selected", NULL, GTK_MENU_FUNC(match_selected_cb_or_ptree_not), 0, NULL},
  {"/Display/_Prepare", NULL, NULL, 0, "<Branch>" },
  {"/Display/Prepare/_Selected", NULL, GTK_MENU_FUNC(prepare_selected_cb_replace_ptree), 0, NULL},
  {"/Display/Prepare/_Not Selected", NULL, GTK_MENU_FUNC(prepare_selected_cb_not_ptree), 0, NULL},
  {"/Display/Prepare/_And Selected", NULL, GTK_MENU_FUNC(prepare_selected_cb_and_ptree), 0, NULL},
  {"/Display/Prepare/_Or Selected", NULL, GTK_MENU_FUNC(prepare_selected_cb_or_ptree), 0, NULL},
  {"/Display/Prepare/A_nd Not Selected", NULL, GTK_MENU_FUNC(prepare_selected_cb_and_ptree_not), 0, NULL},
  {"/Display/Prepare/O_r Not Selected", NULL, GTK_MENU_FUNC(prepare_selected_cb_or_ptree_not), 0, NULL},
  {"/Display/_Colorize Display...", NULL, GTK_MENU_FUNC(color_display_cb), 0, NULL},
  {"/Display/Collapse _All", NULL, GTK_MENU_FUNC(collapse_all_cb), 0, NULL},
  {"/Display/_Expand All", NULL, GTK_MENU_FUNC(expand_all_cb), 0, NULL},
  {"/Display/_Show Packet In New Window", NULL, GTK_MENU_FUNC(new_window_cb), 0, NULL},
  {"/Display/User Specified Decodes...", NULL, GTK_MENU_FUNC(decode_show_cb), 0, NULL},
  {"/_Tools", NULL, NULL, 0, "<Branch>" },
#ifdef HAVE_PLUGINS
  {"/Tools/_Plugins...", NULL, GTK_MENU_FUNC(tools_plugins_cmd_cb), 0, NULL},
#endif
  {"/Tools/_Follow TCP Stream", NULL, GTK_MENU_FUNC(follow_stream_cb), 0, NULL},
  {"/Tools/_Decode As...", NULL, GTK_MENU_FUNC(decode_as_cb), 0, NULL},
/*  {"/Tools/Graph", NULL, NULL, 0, NULL}, future use */
  {"/_Tools/TCP Stream Analysis", NULL, NULL, 0, "<Branch>" },
  {"/_Tools/TCP Stream Analysis/Time-Sequence Graph (Stevens)", NULL, GTK_MENU_FUNC (tcp_graph_cb), 0, NULL},
  {"/_Tools/TCP Stream Analysis/Time-Sequence Graph (tcptrace)", NULL, GTK_MENU_FUNC (tcp_graph_cb), 1, NULL},
  {"/_Tools/TCP Stream Analysis/Throughput Graph", NULL, GTK_MENU_FUNC (tcp_graph_cb), 2, NULL},
  {"/_Tools/TCP Stream Analysis/RTT Graph", NULL, GTK_MENU_FUNC (tcp_graph_cb), 3, NULL},
  {"/Tools/_Summary", NULL, GTK_MENU_FUNC(summary_open_cb), 0, NULL},
  {"/Tools/Protocol Hierarchy Statistics", NULL, GTK_MENU_FUNC(proto_hier_stats_cb), 0, NULL},
  {"/_Help", NULL, NULL, 0, "<LastBranch>" },
  {"/Help/_Help", NULL, GTK_MENU_FUNC(help_cb), 0, NULL},
  {"/Help/<separator>", NULL, NULL, 0, "<Separator>"},
  {"/Help/_About Ethereal...", NULL, GTK_MENU_FUNC(about_ethereal), 0, NULL}
};

/* calculate the number of menu_items */
static int nmenu_items = sizeof(menu_items) / sizeof(menu_items[0]);

/* packet list popup */
static GtkItemFactoryEntry packet_list_menu_items[] =
{
	{"/Follow TCP Stream", NULL, GTK_MENU_FUNC(follow_stream_cb), 0, NULL},
	{"/Decode As...", NULL, GTK_MENU_FUNC(decode_as_cb), 0, NULL},
	{"/Display Filters...", NULL, GTK_MENU_FUNC(dfilter_dialog_cb), 0, NULL},
	{"/<separator>", NULL, NULL, 0, "<Separator>"},
	{"/Mark Frame", NULL, GTK_MENU_FUNC(mark_frame_cb), 0, NULL},
        {"/Match", NULL, NULL, 0, "<Branch>" },
        {"/Match/_Selected", NULL, GTK_MENU_FUNC(match_selected_cb_replace_plist), 0, NULL},
        {"/Match/_Not Selected", NULL, GTK_MENU_FUNC(match_selected_cb_not_plist), 0, NULL},
        {"/Match/_And Selected", NULL, GTK_MENU_FUNC(match_selected_cb_and_plist), 0, NULL},
        {"/Match/_Or Selected", NULL, GTK_MENU_FUNC(match_selected_cb_or_plist), 0, NULL},
        {"/Match/A_nd Not Selected", NULL, GTK_MENU_FUNC(match_selected_cb_and_plist_not), 0, NULL},
        {"/Match/O_r Not Selected", NULL, GTK_MENU_FUNC(match_selected_cb_or_plist_not), 0, NULL},
        {"/Prepare", NULL, NULL, 0, "<Branch>" },
        {"/Prepare/_Selected", NULL, GTK_MENU_FUNC(prepare_selected_cb_replace_plist), 0, NULL},
        {"/Prepare/_Not Selected", NULL, GTK_MENU_FUNC(prepare_selected_cb_not_plist), 0, NULL},
        {"/Prepare/_And Selected", NULL, GTK_MENU_FUNC(prepare_selected_cb_and_plist), 0, NULL},
        {"/Prepare/_Or Selected", NULL, GTK_MENU_FUNC(prepare_selected_cb_or_plist), 0, NULL},
        {"/Prepare/A_nd Not Selected", NULL, GTK_MENU_FUNC(prepare_selected_cb_and_plist_not), 0, NULL},
        {"/Prepare/O_r Not Selected", NULL, GTK_MENU_FUNC(prepare_selected_cb_or_plist_not), 0, NULL},
	{"/<separator>", NULL, NULL, 0, "<Separator>"},
	{"/Colorize Display...", NULL, GTK_MENU_FUNC(color_display_cb), 0, NULL},
	{"/Print...", NULL, GTK_MENU_FUNC(file_print_cmd_cb), 0, NULL},
  	{"/Print Packet", NULL, GTK_MENU_FUNC(file_print_packet_cmd_cb), 0, NULL},
  	{"/Show Packet In New Window", NULL, GTK_MENU_FUNC(new_window_cb), 0, NULL},
};

static GtkItemFactoryEntry tree_view_menu_items[] =
{
	{"/Follow TCP Stream", NULL, GTK_MENU_FUNC(follow_stream_cb), 0, NULL},
	{"/Decode As...", NULL, GTK_MENU_FUNC(decode_as_cb), 0, NULL},
	{"/Display Filters...", NULL, GTK_MENU_FUNC(dfilter_dialog_cb), 0, NULL},
	{"/<separator>", NULL, NULL, 0, "<Separator>"},
	{"/Resolve Name", NULL, GTK_MENU_FUNC(resolve_name_cb), 0, NULL},
	{"/Protocol Properties...", NULL, GTK_MENU_FUNC(properties_cb), 0, NULL},
        {"/Match", NULL, NULL, 0, "<Branch>" },
        {"/Match/_Selected", NULL, GTK_MENU_FUNC(match_selected_cb_replace_ptree), 0, NULL},
        {"/Match/_Not Selected", NULL, GTK_MENU_FUNC(match_selected_cb_not_ptree), 0, NULL},
        {"/Match/_And Selected", NULL, GTK_MENU_FUNC(match_selected_cb_and_ptree), 0, NULL},
        {"/Match/_Or Selected", NULL, GTK_MENU_FUNC(match_selected_cb_or_ptree), 0, NULL},
        {"/Match/A_nd Not Selected", NULL, GTK_MENU_FUNC(match_selected_cb_and_ptree_not), 0, NULL},
        {"/Match/O_r Not Selected", NULL, GTK_MENU_FUNC(match_selected_cb_or_ptree_not), 0, NULL},
        {"/Prepare", NULL, NULL, 0, "<Branch>" },
        {"/Prepare/_Selected", NULL, GTK_MENU_FUNC(prepare_selected_cb_replace_ptree), 0, NULL},
        {"/Prepare/_Not Selected", NULL, GTK_MENU_FUNC(prepare_selected_cb_not_ptree), 0, NULL},
        {"/Prepare/_And Selected", NULL, GTK_MENU_FUNC(prepare_selected_cb_and_ptree), 0, NULL},
        {"/Prepare/_Or Selected", NULL, GTK_MENU_FUNC(prepare_selected_cb_or_ptree), 0, NULL},
        {"/Prepare/A_nd Not Selected", NULL, GTK_MENU_FUNC(prepare_selected_cb_and_ptree_not), 0, NULL},
        {"/Prepare/O_r Not Selected", NULL, GTK_MENU_FUNC(prepare_selected_cb_or_ptree_not), 0, NULL},
	{"/<separator>", NULL, NULL, 0, "<Separator>"},
	{"/Collapse All", NULL, GTK_MENU_FUNC(collapse_all_cb), 0, NULL},
	{"/Expand All", NULL, GTK_MENU_FUNC(expand_all_cb), 0, NULL}
};

static GtkItemFactoryEntry hexdump_menu_items[] =
{
	{"/Follow TCP Stream", NULL, GTK_MENU_FUNC(follow_stream_cb), 0, NULL},
	{"/Decode As...", NULL, GTK_MENU_FUNC(decode_as_cb), 0, NULL},
	{"/Display Filters...", NULL, GTK_MENU_FUNC(dfilter_dialog_cb), 0, NULL}
};

static int initialize = TRUE;
static GtkItemFactory *factory = NULL;
static GtkItemFactory *packet_list_menu_factory = NULL;
static GtkItemFactory *tree_view_menu_factory = NULL;
static GtkItemFactory *hexdump_menu_factory = NULL;

static GSList *popup_menu_list = NULL;

static GtkAccelGroup *grp;

void
get_main_menu(GtkWidget ** menubar, GtkAccelGroup ** table) {

  grp = gtk_accel_group_new();

  if (initialize) {
    popup_menu_object = gtk_widget_new(GTK_TYPE_WIDGET, NULL);
    menus_init();
  }

  if (menubar)
    *menubar = factory->widget;

  if (table)
    *table = grp;
}

static void
menus_init(void) {

  if (initialize) {
    initialize = FALSE;

    /* popup */

    packet_list_menu_factory = gtk_item_factory_new(GTK_TYPE_MENU, "<main>", NULL);
    gtk_item_factory_create_items_ac(packet_list_menu_factory, sizeof(packet_list_menu_items)/sizeof(packet_list_menu_items[0]), packet_list_menu_items, popup_menu_object, 2);
    gtk_object_set_data(GTK_OBJECT(popup_menu_object), PM_PACKET_LIST_KEY, packet_list_menu_factory->widget);
    popup_menu_list = g_slist_append((GSList *)popup_menu_list, packet_list_menu_factory);

    tree_view_menu_factory = gtk_item_factory_new(GTK_TYPE_MENU, "<main>", NULL);
    gtk_item_factory_create_items_ac(tree_view_menu_factory, sizeof(tree_view_menu_items)/sizeof(tree_view_menu_items[0]), tree_view_menu_items, popup_menu_object, 2);
    gtk_object_set_data(GTK_OBJECT(popup_menu_object), PM_TREE_VIEW_KEY, tree_view_menu_factory->widget);
    popup_menu_list = g_slist_append((GSList *)popup_menu_list, tree_view_menu_factory);

    hexdump_menu_factory = gtk_item_factory_new(GTK_TYPE_MENU, "<main>", NULL);
    gtk_item_factory_create_items_ac(hexdump_menu_factory, sizeof(hexdump_menu_items)/sizeof(hexdump_menu_items[0]), hexdump_menu_items, popup_menu_object, 2);
    gtk_object_set_data(GTK_OBJECT(popup_menu_object), PM_HEXDUMP_KEY, hexdump_menu_factory->widget);
    popup_menu_list = g_slist_append((GSList *)popup_menu_list, hexdump_menu_factory);

    factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", grp);
    gtk_item_factory_create_items_ac(factory, nmenu_items, menu_items, NULL,2);
    set_menus_for_unsaved_capture_file(FALSE);
    set_menus_for_capture_file(FALSE);
#if 0
    /* Un-#if this when we actually implement Cut/Copy/Paste.
       Then make sure you enable them when they can be done. */
    set_menu_sensitivity("/Edit/Cut", FALSE);
    set_menu_sensitivity("/Edit/Copy", FALSE);
    set_menu_sensitivity("/Edit/Paste", FALSE);
#endif
    set_menus_for_captured_packets(FALSE);
    set_menus_for_selected_packet(FALSE);
    set_menus_for_selected_tree_row(FALSE);
  }
}

void
set_menu_sensitivity_meat(GtkItemFactory *ifactory, gchar *path, gint val) {
	GtkWidget *menu = NULL;

	if((menu = gtk_item_factory_get_widget(ifactory, path)) != NULL) {
		gtk_widget_set_sensitive(menu,val);
	}
}

/* Enable/disable menu sensitivity                       */
/* /menu/path            - old functionality             */
/* <MenuName>/menu/path  - new functionality             */
/* MenuName: <Main>, <PacketList>, <TreeView>, <HexDump> */
static void
set_menu_sensitivity (gchar *path, gint val) {
  GSList *menu_list = popup_menu_list;
  gchar *prefix;
  gchar *shortpath;

  if ('<' == *path) {
    /* New functionality => selective enable/disable per menu */
    prefix=strchr(path, '/');
    shortpath=strrchr(prefix, '/');

    if (0 == strncmp(path, "<Main>", 6))
      set_menu_sensitivity_meat(factory, prefix, val);
    else if (0 == strncmp(path, "<PacketList>", 12))
      set_menu_sensitivity_meat(packet_list_menu_factory, shortpath, val);
    else if (0 == strncmp(path, "<TreeView>", 10))
      set_menu_sensitivity_meat(tree_view_menu_factory, shortpath, val);
    else if (0 == strncmp(path, "<HexDump>", 9))
      set_menu_sensitivity_meat(hexdump_menu_factory, shortpath, val);
  } else {
    /* Old functionality => enable/disable all menus with same shortpath */
    shortpath = strrchr(path, '/');

    set_menu_sensitivity_meat(factory, path, val);

    while (menu_list != NULL) {
  	  set_menu_sensitivity_meat(menu_list->data, shortpath, val);
	  menu_list = g_slist_next(menu_list);
    }
  }
}

void
set_menu_object_data_meat(GtkItemFactory *ifactory, gchar *path, gchar *key, gpointer data)
{
	GtkWidget *menu = NULL;

	if ((menu = gtk_item_factory_get_widget(ifactory, path)) != NULL)
		gtk_object_set_data(GTK_OBJECT(menu), key, data);
}

void
set_menu_object_data (gchar *path, gchar *key, gpointer data) {
  GSList *menu_list = popup_menu_list;
  gchar *shortpath = strrchr(path, '/');

  set_menu_object_data_meat(factory, path, key, data);
  while (menu_list != NULL) {
  	set_menu_object_data_meat(menu_list->data, shortpath, key, data);
	menu_list = g_slist_next(menu_list);
  }
}

gint
popup_menu_handler(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	GtkWidget *menu = NULL;
	GdkEventButton *event_button = NULL;
	GtkCList *packet_list = NULL;
	gint row, column;

	if(widget == NULL || event == NULL || data == NULL) {
		return FALSE;
	}

	/*
	 * If we ever want to make the menu differ based on what row
	 * and/or column we're above, we'd use "gtk_clist_get_selection_info()"
	 * to find the row and column number for the coordinates; a CTree is,
	 * I guess, like a CList with one column(?) and the expander widget
	 * as a pixmap.
	 */
	/* Check if we are on packet_list object */
	if (widget == gtk_object_get_data(GTK_OBJECT(popup_menu_object),
		E_MPACKET_LIST_KEY)) {
	  packet_list=GTK_CLIST(widget);
	  if (gtk_clist_get_selection_info(GTK_CLIST(packet_list),
	      ((GdkEventButton *)event)->x,
	      ((GdkEventButton *)event)->y,&row,&column)) {
	    gtk_object_set_data(GTK_OBJECT(popup_menu_object),
		E_MPACKET_LIST_ROW_KEY, (gpointer *)row);
	    gtk_object_set_data(GTK_OBJECT(popup_menu_object),
		E_MPACKET_LIST_COL_KEY, (gpointer *)column);
	  }
	}
	menu = (GtkWidget *)data;
	if(event->type == GDK_BUTTON_PRESS) {
		event_button = (GdkEventButton *) event;

		if(event_button->button == 3) {
			gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, event_button->button, event_button->time);
			gtk_signal_emit_stop_by_name(GTK_OBJECT(widget), "button_press_event");
			return TRUE;
		}
	}
	return FALSE;
}

/* Enable or disable menu items based on whether you have a capture file
   you've finished reading. */
void
set_menus_for_capture_file(gboolean have_capture_file)
{
  set_menu_sensitivity("/File/Open...", have_capture_file);
  set_menu_sensitivity("/File/Save As...", have_capture_file);
  set_menu_sensitivity("/File/Close", have_capture_file);
  set_menu_sensitivity("/File/Reload", have_capture_file);
}

/* Enable or disable menu items based on whether you have an unsaved
   capture file you've finished reading. */
void
set_menus_for_unsaved_capture_file(gboolean have_unsaved_capture_file)
{
  set_menu_sensitivity("/File/Save", have_unsaved_capture_file);
}

/* Enable or disable menu items based on whether there's a capture in
   progress. */
void
set_menus_for_capture_in_progress(gboolean capture_in_progress)
{
  set_menu_sensitivity("/File/Open...", !capture_in_progress);
  set_menu_sensitivity("/Capture/Start...", !capture_in_progress);
  /*
   * XXX - this doesn't yet work in Win32.
   */
#ifndef _WIN32
  set_menu_sensitivity("/Capture/Stop", capture_in_progress);
#endif
}

/* Enable or disable menu items based on whether you have some captured
   packets. */
void
set_menus_for_captured_packets(gboolean have_captured_packets)
{
  set_menu_sensitivity("/File/Print...", have_captured_packets);
  set_menu_sensitivity("/Edit/Find Frame...", have_captured_packets);
  set_menu_sensitivity("/Edit/Find Next", have_captured_packets);
  set_menu_sensitivity("/Edit/Find Previous", have_captured_packets);
  set_menu_sensitivity("/Edit/Go To Frame...", have_captured_packets);
  set_menu_sensitivity("/Display/Colorize Display...", have_captured_packets);
  set_menu_sensitivity("/Tools/Summary", have_captured_packets);
  set_menu_sensitivity("/Tools/Protocol Hierarchy Statistics", have_captured_packets);
  set_menu_sensitivity("<PacketList>/Display/Match", have_captured_packets);
  set_menu_sensitivity("<PacketList>/Display/Prepare", have_captured_packets);
}

/* Enable or disable menu items based on whether a packet is selected. */
void
set_menus_for_selected_packet(gboolean have_selected_packet)
{
  set_menu_sensitivity("/File/Print Packet", have_selected_packet);
  set_menu_sensitivity("/Edit/Mark Frame", have_selected_packet);
  set_menu_sensitivity("/Edit/Mark All Frames", have_selected_packet);
  set_menu_sensitivity("/Edit/Unmark All Frames", have_selected_packet);
  set_menu_sensitivity("/Display/Collapse All", have_selected_packet);
  set_menu_sensitivity("/Display/Expand All", have_selected_packet);
  set_menu_sensitivity("/Display/Show Packet In New Window", have_selected_packet);
  set_menu_sensitivity("/Tools/Follow TCP Stream",
      have_selected_packet ? (cfile.edt->pi.ipproto == 6) : FALSE);
  set_menu_sensitivity("/Tools/Decode As...",
      have_selected_packet && decode_as_ok());
  set_menu_sensitivity("/Resolve Name",
      have_selected_packet && g_resolv_flags == 0);
  set_menu_sensitivity("/Tools/TCP Stream Analysis",
            have_selected_packet ? (cfile.edt->pi.ipproto == 6) : FALSE);
}

/* Enable or disable menu items based on whether a tree row is selected
   and and on whether a "Match" can be done. */
void
set_menus_for_selected_tree_row(gboolean have_selected_tree)
{
  gboolean properties = FALSE;

  if (finfo_selected) {
	header_field_info *hfinfo = finfo_selected->hfinfo;
	if (hfinfo->parent == -1) {
	  properties = prefs_is_registered_protocol(hfinfo->abbrev);
	} else {
	  properties = prefs_is_registered_protocol(proto_registrar_get_abbrev(hfinfo->parent));
	}
	set_menu_sensitivity("<Main>/Display/Match",
	  proto_can_match_selected(finfo_selected));
	set_menu_sensitivity("<TreeView>/Display/Match",
	  proto_can_match_selected(finfo_selected));
	set_menu_sensitivity("<Main>/Display/Prepare",
	  proto_can_match_selected(finfo_selected));
	set_menu_sensitivity("<TreeView>/Display/Prepare",
	  proto_can_match_selected(finfo_selected));
  } else {
	set_menu_sensitivity("<Main>/Display/Match", FALSE);
	set_menu_sensitivity("<TreeView>/Display/Match", FALSE);
	set_menu_sensitivity("<Main>/Display/Prepare", FALSE);
	set_menu_sensitivity("<TreeView>/Display/Prepare", FALSE);
  }

  set_menu_sensitivity("/Protocol Properties...", have_selected_tree && properties);
}
