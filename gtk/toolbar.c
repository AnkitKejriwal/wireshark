/* toolbar.c
 * The main toolbar
 * Copyright 2003, Ulf Lamping <ulf.lamping@web.de>
 *
 * $Id: toolbar.c,v 1.10 2003/10/20 19:07:18 guy Exp $
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

/*
 * This file implements a "main" toolbar for Ethereal (suitable for gtk1 and
 * gtk2).
 *
 * As it is desirable to have the same toolbar implementation for gtk1 and gtk2 
 * in Ethereal, only those library calls available in the gtk1 libraries 
 * are used inside this file.
 *
 * Hint: gtk2 in comparison to gtk1 has a better way to handle with "common"
 * icons; gtk2 calls this kind of icons "stock-icons"
 * (stock-icons including: icons for "open", "save", "print", ...).
 * The gtk2 version of this code uses them.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtk/gtk.h>

#ifdef HAVE_LIBPCAP
#include "capture_dlg.h"
#endif /* HAVE_LIBPCAP */
#include "file_dlg.h"
#include "find_dlg.h"
#include "goto_dlg.h"
#include "color.h"
#include "color_dlg.h"
#include "filter_prefs.h"
#include "prefs.h"
#include "prefs_dlg.h"
#include "main.h"
#include "help_dlg.h"
#include "gtkglobals.h"
#include "toolbar.h"
#include "keys.h"
#include "compat_macros.h"

/* All of the icons used here are coming (or are derived) from GTK2 stock icons.
 * They were converted using "The Gimp" with standard conversion from png to xpm.
 * All stock icons can be (currently) found at: 
 * "ftp://ftp.gtk.org/pub/gtk/v2.0/gtk+-2.0.6.tar.bz2"
 * in the directory "gtk+-2.0.6\gtk\stock-icons" */
#if GTK_MAJOR_VERSION < 2
#include "../image/toolbar/stock_stop_24.xpm"
#include "../image/toolbar/stock_open_24.xpm"
#include "../image/toolbar/stock_save_24.xpm"
#include "../image/toolbar/stock_close_24.xpm"
#include "../image/toolbar/stock_refresh_24.xpm"
#include "../image/toolbar/stock_print_24.xpm"
#include "../image/toolbar/stock_search_24.xpm"
#include "../image/toolbar/stock_right_arrow_24.xpm"
#include "../image/toolbar/stock_jump_to_24.xpm"
#include "../image/toolbar/stock_colorselector_24.xpm"
#include "../image/toolbar/stock_preferences_24.xpm"
#include "../image/toolbar/stock_help_24.xpm"
/* this icons are derived from the original stock icons */
#include "../image/toolbar/capture_24.xpm"
#endif /* GTK_MAJOR_VERSION */

#ifdef HAVE_LIBPCAP
#include "../image/toolbar/cfilter_24.xpm"
#endif /* HAVE_LIBPCAP */
#include "../image/toolbar/dfilter_24.xpm"


/* XXX: add this key to some .h file, as it adds a key to the top level Widget? */
#define E_TB_MAIN_KEY             "toolbar_main"
#define E_TB_MAIN_HB_KEY          "toolbar_main_handlebox"


static gboolean toolbar_init = FALSE;

#ifdef HAVE_LIBPCAP
static GtkWidget *new_button, *stop_button;
static GtkWidget *capture_filter_button;
#endif /* HAVE_LIBPCAP */
static GtkWidget *open_button, *save_button, *close_button, *reload_button;
static GtkWidget *print_button, *find_button, *find_next_button, *go_to_button;
static GtkWidget *display_filter_button;
static GtkWidget *color_display_button, *prefs_button, *help_button;

static void get_main_toolbar(GtkWidget *window, GtkWidget **toolbar);


/*
 * Create all toolbars (currently only the main toolbar)
 */
void
create_toolbar(GtkWidget *main_vbox)
{
    GtkWidget *main_tb, *main_tb_hb;

    /* Main Toolbar */
    get_main_toolbar(top_level, &main_tb);
#if GTK_MAJOR_VERSION < 2
    gtk_toolbar_set_space_size(GTK_TOOLBAR(main_tb), 3);
#endif

    /* To make it nice we'll put the toolbar into a handle box, 
     * so that it can be detached from the main window */
    /* XXX - this is coming from gtk examples (is this really helpful for
     * someone?) */
    main_tb_hb = gtk_handle_box_new();
    gtk_container_add(GTK_CONTAINER(main_tb_hb) , main_tb);
    gtk_container_set_border_width(GTK_CONTAINER(main_tb_hb), 3);
    gtk_box_pack_start(GTK_BOX(main_vbox), main_tb_hb, FALSE, TRUE, 0);
    gtk_widget_show(main_tb_hb);

    OBJECT_SET_DATA(top_level, E_TB_MAIN_KEY, main_tb);
    OBJECT_SET_DATA(top_level, E_TB_MAIN_HB_KEY, main_tb_hb);
    /* make current preferences effective */
    toolbar_redraw_all();
}

/*
 * Redraw all toolbars (currently only the main toolbar)
 */
void
toolbar_redraw_all(void)
{
    GtkWidget     *main_tb, *main_tb_hb;
    gboolean      gui_toolbar_main_show;
    static const GtkToolbarStyle styles[] = {
	GTK_TOOLBAR_ICONS,
	GTK_TOOLBAR_TEXT,
	GTK_TOOLBAR_BOTH
    };

    /* default: show toolbar */
    /* XXX: get this info from a preference setting */
    gui_toolbar_main_show = TRUE;
    /* gui_toolbar_main_style = prefs.gui_toolbar_main_show; */

    main_tb_hb = OBJECT_GET_DATA(top_level, E_TB_MAIN_HB_KEY);

    /* does the user want the toolbar? */
    if (gui_toolbar_main_show) {
        /* yes, set the style he/she prefers (texts, icons, both) */
        main_tb = OBJECT_GET_DATA(top_level, E_TB_MAIN_KEY);
        gtk_toolbar_set_style(GTK_TOOLBAR(main_tb),
                          styles[prefs.gui_toolbar_main_style]);
        gtk_widget_show(main_tb_hb);
    } else {
        /* no */
        gtk_widget_hide(main_tb_hb);
    }

#if GTK_MAJOR_VERSION < 2
    /* In GTK+ 1.2[.x], the toolbar takes the maximum vertical size it ever
     * had, even if you change the style in such a way as to reduce its
     * height, unless we queue a resize (which resizes ALL elements in the
     * top-level container).
     *
     * In GTK+ 2.x, this isn't necessary - it does the right thing. */
    gtk_container_queue_resize(GTK_CONTAINER(top_level));
#endif /* GTK_MAJOR_VERSION */
}


/* set toolbar state "have a capture file" */
void set_toolbar_for_capture_file(gboolean have_capture_file) {
    if (toolbar_init) {
        gtk_widget_set_sensitive(save_button, have_capture_file);
        gtk_widget_set_sensitive(close_button, have_capture_file);
        gtk_widget_set_sensitive(reload_button, have_capture_file);
    }
}


#ifdef HAVE_LIBPCAP

/* set toolbar state "have a capture in progress" */
void set_toolbar_for_capture_in_progress(gboolean capture_in_progress) {

    if (toolbar_init) {
        gtk_widget_set_sensitive(new_button, !capture_in_progress);
        gtk_widget_set_sensitive(open_button, !capture_in_progress);
        /*
         * XXX - this doesn't yet work in Win32, as in the menus :-(
         */
#ifndef _WIN32
        if (capture_in_progress) {
            gtk_widget_hide(new_button);
            gtk_widget_show(stop_button);
        } else {
            gtk_widget_show(new_button);
            gtk_widget_hide(stop_button);
        }
#else
        gtk_widget_set_sensitive(new_button, !capture_in_progress);
#endif /* _WIN32 */
    }
}

#endif /* HAVE_LIBPCAP */

/* set toolbar state "have packets captured" */
void set_toolbar_for_captured_packets(gboolean have_captured_packets) {

    if (toolbar_init) {
        gtk_widget_set_sensitive(print_button, have_captured_packets);
        gtk_widget_set_sensitive(find_button, have_captured_packets);
        gtk_widget_set_sensitive(find_next_button, have_captured_packets);
        gtk_widget_set_sensitive(go_to_button, have_captured_packets);
        /* XXX - I don't see a reason why this should be done (as it is in the
         * menus) */
        /* gtk_widget_set_sensitive(color_display_button, have_captured_packets);*/
    }
}


/* helper function: add a separator to the toolbar */
static void toolbar_append_separator(GtkWidget *toolbar) {
#if GTK_MAJOR_VERSION < 2
    /* XXX - the usage of a gtk_separator doesn't seem to work for a toolbar.
     * (at least in the win32 port of gtk 1.3)
     * So simply add a few spaces */
    gtk_toolbar_append_space(GTK_TOOLBAR(toolbar)); /* space after item */
    gtk_toolbar_append_space(GTK_TOOLBAR(toolbar)); /* space after item */
#else
    /* GTK 2 uses (as it should be) a seperator when adding this space */
    gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
#endif /* GTK_MAJOR_VERSION */
}


/* get the main toolbar (remember: call this only once!) */
static void get_main_toolbar(GtkWidget *window, GtkWidget **toolbar)
{
    GdkPixmap *icon;
    GtkWidget *iconw;
    GdkBitmap * mask;

    /* Display filter construct dialog has only a "Save" and a "Close" button.
     * XXX - Adding the "Ok" and "Apply" buttons would need some more work here
     */	
    static construct_args_t args = {
        "Ethereal: Edit Display Filter",
        FALSE,
        FALSE
    };


    /* this function should be only called once! */
    g_assert(!toolbar_init);

    /* we need to realize the window because we use pixmaps for 
     * items on the toolbar in the context of it */
    /* (coming from the gtk example, please don't ask me why ;-) */
    gtk_widget_realize(window);

    /* toolbar will be horizontal, with both icons and text (as default here) */
    /* (this will usually be overwritten by the preferences setting) */
#if GTK_MAJOR_VERSION < 2
    *toolbar = gtk_toolbar_new(GTK_ORIENTATION_HORIZONTAL,
                               GTK_TOOLBAR_BOTH);
#else
    *toolbar = gtk_toolbar_new();
    gtk_toolbar_set_orientation(GTK_TOOLBAR(*toolbar),
                                GTK_ORIENTATION_HORIZONTAL);
#endif /* GTK_MAJOR_VERSION */

#ifdef HAVE_LIBPCAP

    /* start capture button */
#if GTK_MAJOR_VERSION < 2
    icon = gdk_pixmap_create_from_xpm_d(window->window, &mask,
                                        &window->style->white, capture_24_xpm);
    iconw = gtk_pixmap_new(icon, mask); /* icon widget */

    new_button = 
        gtk_toolbar_append_item(GTK_TOOLBAR (*toolbar),/* our toolbar */
                                "New",                 /* button label */
                                "Start new capture...",/* button's tooltip */
                                "Private",             /* tooltip private info */
                                iconw,                 /* icon widget */
                                GTK_SIGNAL_FUNC(capture_prep_cb), /* callback */
                                NULL );
#else
    new_button = gtk_toolbar_insert_stock(GTK_TOOLBAR(*toolbar),
                                          GTK_STOCK_EXECUTE,
                                          "Start new capture...", "Private",
                                          G_CALLBACK(capture_prep_cb), NULL,
                                          -1);
#endif /* GTK_MAJOR_VERSION */

    /* either start OR stop button can be valid at a time, so no space needed
     * here */

    /* stop capture button (hidden by default) */
#ifndef _WIN32
#if GTK_MAJOR_VERSION < 2
    icon = gdk_pixmap_create_from_xpm_d(window->window, &mask,
                                        &window->style->white,
                                        stock_stop_24_xpm);
    iconw = gtk_pixmap_new(icon, mask);

    stop_button = gtk_toolbar_append_item(GTK_TOOLBAR(*toolbar),
                                          "Stop", "Stop running capture",
                                          "Private", iconw,
                                          GTK_SIGNAL_FUNC(capture_stop_cb),
                                          NULL);
#else
    stop_button = gtk_toolbar_insert_stock(GTK_TOOLBAR(*toolbar),
                                           GTK_STOCK_STOP,
                                           "Stop running capture...", "Private",
                                           G_CALLBACK(capture_stop_cb), NULL,
                                           -1);
#endif /* GTK_MAJOR_VERSION */
#endif /* _WIN32 */
    toolbar_append_separator(*toolbar);
#endif /* HAVE_LIBPCAP */

    /* open capture button */
#if GTK_MAJOR_VERSION < 2
    icon = gdk_pixmap_create_from_xpm_d(window->window, &mask,
                                        &window->style->white,
                                        stock_open_24_xpm);
    iconw = gtk_pixmap_new(icon, mask);

    open_button = gtk_toolbar_append_item(GTK_TOOLBAR(*toolbar),
                                          "Open", "Open capture file...",
                                          "Private", iconw,
                                          GTK_SIGNAL_FUNC(file_open_cmd_cb),
                                          NULL);
#else
    open_button = gtk_toolbar_insert_stock(GTK_TOOLBAR(*toolbar),
                                           GTK_STOCK_OPEN,
                                           "Open capture file...", "Private",
                                           G_CALLBACK(file_open_cmd_cb), NULL,
                                           -1);
#endif /* GTK_MAJOR_VERSION */

    /* save capture button */
#if GTK_MAJOR_VERSION < 2
    icon = gdk_pixmap_create_from_xpm_d(window->window, &mask,
                                        &window->style->white,
                                        stock_save_24_xpm);
    iconw = gtk_pixmap_new(icon, mask);

    save_button = gtk_toolbar_append_item(GTK_TOOLBAR(*toolbar),
                                          "Save", "Save capture file",
                                          "Private", iconw,
                                          GTK_SIGNAL_FUNC(file_save_cmd_cb),
                                          NULL);
#else
    save_button = gtk_toolbar_insert_stock(GTK_TOOLBAR(*toolbar),
                                           GTK_STOCK_SAVE,
                                           "Save capture file...", "Private",
                                           G_CALLBACK(file_save_cmd_cb), NULL,
                                           -1);
#endif /* GTK_MAJOR_VERSION */

    /* close capture button */
#if GTK_MAJOR_VERSION < 2
    icon = gdk_pixmap_create_from_xpm_d(window->window, &mask,
                                        &window->style->white,
                                        stock_close_24_xpm);
    iconw = gtk_pixmap_new(icon, mask);

    close_button = gtk_toolbar_append_item(GTK_TOOLBAR(*toolbar),
                                           "Close", "Close capture file",
                                           "Private", iconw,
                                           GTK_SIGNAL_FUNC(file_close_cmd_cb),
                                           NULL);
#else
    close_button = gtk_toolbar_insert_stock(GTK_TOOLBAR(*toolbar),
                                            GTK_STOCK_CLOSE,
                                            "Close capture file...", "Private",
                                            G_CALLBACK(file_close_cmd_cb), NULL,
                                            -1);
#endif /* GTK_MAJOR_VERSION */

    /* reload capture file button */
#if GTK_MAJOR_VERSION < 2
    icon = gdk_pixmap_create_from_xpm_d(window->window, &mask,
                                        &window->style->white,
                                        stock_refresh_24_xpm);
    iconw = gtk_pixmap_new(icon, mask);

    reload_button = gtk_toolbar_append_item(GTK_TOOLBAR(*toolbar),
                                            "Reload", "Reload capture file",
                                            "Private", iconw,
                                            GTK_SIGNAL_FUNC(file_reload_cmd_cb),
                                            NULL);
#else
    reload_button = gtk_toolbar_insert_stock(GTK_TOOLBAR(*toolbar),
                                             GTK_STOCK_REFRESH,
                                             "Reload capture file...",
                                             "Private",
                                             G_CALLBACK(file_reload_cmd_cb),
                                             NULL, -1);
#endif /* GTK_MAJOR_VERSION */
    toolbar_append_separator(*toolbar);

    /* print frame(s) button */
#if GTK_MAJOR_VERSION < 2
    icon = gdk_pixmap_create_from_xpm_d(window->window, &mask,
                                        &window->style->white,
                                        stock_print_24_xpm);
    iconw = gtk_pixmap_new(icon, mask);

    print_button = gtk_toolbar_append_item(GTK_TOOLBAR(*toolbar),
                                           "Print", "Print frame(s)",
                                           "Private", iconw,
                                           GTK_SIGNAL_FUNC(file_print_cmd_cb),
                                           NULL);
#else
    print_button = gtk_toolbar_insert_stock(GTK_TOOLBAR(*toolbar),
                                            GTK_STOCK_PRINT, "Print frame(s)",
                                            "Private",
                                            G_CALLBACK(file_print_cmd_cb),
                                            NULL, -1);
#endif /* GTK_MAJOR_VERSION */

    /* find frame button */
#if GTK_MAJOR_VERSION < 2
    icon = gdk_pixmap_create_from_xpm_d(window->window, &mask,
                                        &window->style->white,
                                        stock_search_24_xpm);
    iconw = gtk_pixmap_new(icon, mask);

    find_button = gtk_toolbar_append_item(GTK_TOOLBAR (*toolbar), "Find",
                                          "Find frame...",
                                          "Private", iconw,
                                          GTK_SIGNAL_FUNC(find_frame_cb), NULL);
#else
    find_button = gtk_toolbar_insert_stock(GTK_TOOLBAR(*toolbar),
                                           GTK_STOCK_FIND,
                                           "Find frame...",
                                           "Private", G_CALLBACK(find_frame_cb),
                                           NULL, -1);
#endif /* GTK_MAJOR_VERSION */

    /* find next frame button */
#if GTK_MAJOR_VERSION < 2
    icon = gdk_pixmap_create_from_xpm_d(window->window, &mask,
                                        &window->style->white,
                                        stock_right_arrow_24_xpm);
    iconw = gtk_pixmap_new(icon, mask);

    find_next_button = gtk_toolbar_append_item(GTK_TOOLBAR(*toolbar), "Next",
                                               "Find next frame", "Private",
                                               iconw,
                                               GTK_SIGNAL_FUNC(find_next_cb),
                                               NULL);
#else
    find_next_button = gtk_toolbar_insert_stock(GTK_TOOLBAR(*toolbar),
                                                GTK_STOCK_GO_FORWARD,
                                                "Find next frame", "Private",
                                                G_CALLBACK(find_next_cb), NULL,
                                                -1);
#endif /* GTK_MAJOR_VERSION */

    /* go to frame button */
#if GTK_MAJOR_VERSION < 2
    icon = gdk_pixmap_create_from_xpm_d(window->window, &mask,
                                        &window->style->white,
                                        stock_jump_to_24_xpm);
    iconw = gtk_pixmap_new(icon, mask);

    go_to_button = gtk_toolbar_append_item(GTK_TOOLBAR(*toolbar), "GoTo",
                                           "Go to frame number...", "Private",
                                           iconw,
                                           GTK_SIGNAL_FUNC(goto_frame_cb),
                                           NULL);
#else
    go_to_button = gtk_toolbar_insert_stock(GTK_TOOLBAR(*toolbar),
                                            GTK_STOCK_JUMP_TO,
                                            "Go to frame number...", "Private",
                                            G_CALLBACK(goto_frame_cb), NULL,
                                            -1);
#endif /* GTK_MAJOR_VERSION */
    toolbar_append_separator(*toolbar);
    
#ifdef HAVE_LIBPCAP

    /* capture filter button */
    icon = gdk_pixmap_create_from_xpm_d(window->window, &mask,
                                        &window->style->white, cfilter_24_xpm);
    iconw = gtk_pixmap_new(icon, mask);

    capture_filter_button =
        gtk_toolbar_append_item(GTK_TOOLBAR(*toolbar), "CFilter",
                                "Edit Capture Filters...", "Private", iconw,
                                GTK_SIGNAL_FUNC(cfilter_dialog_cb), NULL);
#endif /* HAVE_LIBPCAP */

    /* display filter button */
    icon = gdk_pixmap_create_from_xpm_d(window->window, &mask,
                                        &window->style->white, dfilter_24_xpm);
    iconw = gtk_pixmap_new(icon, mask);

    display_filter_button =
        gtk_toolbar_append_item(GTK_TOOLBAR(*toolbar), "DFilter",
                                "Edit Display Filters...", "Private", iconw,
                                GTK_SIGNAL_FUNC(display_filter_construct_cb),
                                &args);

    /* color filter button */
#if GTK_MAJOR_VERSION < 2
    icon = gdk_pixmap_create_from_xpm_d(window->window, &mask,
                                        &window->style->white,
                                        stock_colorselector_24_xpm);
    iconw = gtk_pixmap_new(icon, mask);

    color_display_button =
        gtk_toolbar_append_item(GTK_TOOLBAR(*toolbar), "Color",
                                "Apply Color Filters...", "Private", iconw,
                                GTK_SIGNAL_FUNC(color_display_cb), NULL);
#else
    color_display_button =
        gtk_toolbar_insert_stock(GTK_TOOLBAR(*toolbar), GTK_STOCK_SELECT_COLOR,
                                 "Apply color filters...", "Private",
                                 G_CALLBACK(color_display_cb), NULL, -1);
#endif /* GTK_MAJOR_VERSION */

    /* preferences button */
#if GTK_MAJOR_VERSION < 2
    icon = gdk_pixmap_create_from_xpm_d(window->window, &mask,
                                        &window->style->white,
                                        stock_preferences_24_xpm);
    iconw = gtk_pixmap_new(icon, mask);

    prefs_button = gtk_toolbar_append_item(GTK_TOOLBAR(*toolbar), "Prefs",
                                           "Edit Preferences...", "Private",
                                           iconw, GTK_SIGNAL_FUNC(prefs_cb),
                                           NULL);
#else
    prefs_button = gtk_toolbar_insert_stock(GTK_TOOLBAR(*toolbar),
                                            GTK_STOCK_PREFERENCES,
                                            "Edit preferences...", "Private",
                                            G_CALLBACK(prefs_cb), NULL, -1);
#endif /* GTK_MAJOR_VERSION */
    toolbar_append_separator(*toolbar);

    /* help button */
#if GTK_MAJOR_VERSION < 2
    icon = gdk_pixmap_create_from_xpm_d(window->window, &mask,
                                        &window->style->white,
                                        stock_help_24_xpm);
    iconw = gtk_pixmap_new(icon, mask);

    help_button = gtk_toolbar_append_item(GTK_TOOLBAR(*toolbar), "Help",
                                          "Show Help Dialog...", "Private",
                                          iconw, GTK_SIGNAL_FUNC(help_cb),
                                          NULL);
#else
    help_button = gtk_toolbar_insert_stock(GTK_TOOLBAR(*toolbar),
                                           GTK_STOCK_HELP,
                                           "Show help dialog...", "Private",
                                           G_CALLBACK(help_cb), NULL, -1);
#endif /* GTK_MAJOR_VERSION */

    /* disable all "sensitive" items by default */
    toolbar_init = TRUE;
    set_toolbar_for_captured_packets(FALSE);
    set_toolbar_for_capture_file(FALSE);
#ifdef HAVE_LIBPCAP
    set_toolbar_for_capture_in_progress(FALSE);
#endif /* HAVE_LIBPCAP */
    /* everything is well done here :-) */
    gtk_widget_show (*toolbar);
}

void
set_toolbar_object_data(gchar *key, gpointer data)
{
    OBJECT_SET_DATA(open_button, key, data);
    OBJECT_SET_DATA(reload_button, key, data);
}
