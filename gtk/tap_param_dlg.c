/* tap_param_dlg.c
 * Routines for parameter dialog used by gui taps
 * Copyright 2003 Lars Roland
 *
 * $Id$
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
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

#include <stdio.h>
#include <string.h>

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#include <gtk/gtk.h>

#include <epan/stat_cmd_args.h>

#include "../simple_dialog.h"
#include "../file.h"
#include "../globals.h"
#include "../stat_menu.h"

#include "gtk/stock_icons.h"
#include "gtk/dlg_utils.h"
#include "gtk/filter_dlg.h"
#include "gtk/gui_stat_menu.h"
#include "gtk/tap_param_dlg.h"
#include "gtk/gui_utils.h"
#include "gtk/gtkglobals.h"
#include "gtk/filter_autocomplete.h"

#include "gtk/old-gtk-compat.h"

typedef struct _tap_param_dlg_list_item {
    GtkWidget *dlg;
    tap_param_dlg cont;
    construct_args_t args;
    GtkWidget **param_items;    /* items for params */
    struct _tap_param_dlg_list_item *next;
} tap_param_dlg_list_item;

static tap_param_dlg_list_item *start_dlg_list=NULL;
static tap_param_dlg_list_item *end_dlg_list=NULL;
static tap_param_dlg_list_item *current_dlg = NULL;

static void
tap_param_dlg_cb(GtkWidget *w, gpointer data);

/*
 * Register a stat that has a parameter dialog.
 * We register it both as a command-line stat and a menu item stat.
 */
void
register_dfilter_stat(tap_param_dlg *info, const char *name,
    register_stat_group_t group)
{
    char *full_name;

    register_stat_cmd_arg(info->init_string, info->tap_init_cb, NULL);

    /*
     * This menu item will pop up a dialog box, so append "..."
     * to it.
     */
    full_name = g_strdup_printf("%s...", name);
    register_stat_menu_item(full_name, group, tap_param_dlg_cb, NULL,
                            NULL, info);
    g_free(full_name);
}              

void tap_param_dlg_update (void)
{
    tap_param_dlg_list_item *dialog = start_dlg_list;
    char *title;
    
    while(dialog != NULL) {
        if(dialog->dlg) {
            title = g_strdup_printf("Wireshark: %s: %s", dialog->cont.win_title , cf_get_display_name(&cfile));
            gtk_window_set_title(GTK_WINDOW(dialog->dlg), title);
            g_free(title);
        }
        dialog = dialog->next;
    }
}

static void
dlg_destroy_cb(GtkWidget *item _U_, gpointer dialog_data)
{
    tap_param_dlg_list_item *dlg_data = (tap_param_dlg_list_item *) dialog_data;
    dlg_data->dlg = NULL;
}

static void
tap_param_dlg_start_button_clicked(GtkWidget *item _U_, gpointer dialog_data)
{
    GString *params;
    size_t i;
    gint j;
    
    tap_param_dlg_list_item *dlg_data = (tap_param_dlg_list_item *) dialog_data;

    params = g_string_new(dlg_data->cont.init_string);
    for(i=0;i<dlg_data->cont.nparams;i++) {
        g_string_append_c(params, ',');
        switch (dlg_data->cont.params[i].type) {

        case PARAM_ENUM:
            j = gtk_combo_box_get_active(GTK_COMBO_BOX(dlg_data->param_items[i]));
            g_string_append_printf(params,"%d",
                                   dlg_data->cont.params[i].enum_vals[j].value);
            break;

        case PARAM_UINT:
        case PARAM_STRING:
        case PARAM_FILTER:
            g_string_append(params,
                            gtk_entry_get_text(GTK_ENTRY(dlg_data->param_items[i])));
            break;
        }
    }
    (dlg_data->cont.tap_init_cb)(params->str,NULL);
    g_string_free(params, TRUE);
}


static void
tap_param_dlg_cb(GtkWidget *w _U_, gpointer data)
{
    const char *filter;
    char *title;
    GtkWidget *dlg_box;
    GtkWidget *item_box, *item, *label, *filter_bt;
    GtkWidget *bbox, *start_button, *cancel_button;
    size_t i, j;
    char *label_with_colon;
    
    tap_param_dlg *dlg_data = (tap_param_dlg *) data;

    if(dlg_data==NULL)
        return;
        
    if(dlg_data->index==-1) {
        /* Dialog is not registered */
        if(start_dlg_list==NULL) {
            start_dlg_list = (tap_param_dlg_list_item *) g_malloc(sizeof (tap_param_dlg_list_item));
            end_dlg_list = start_dlg_list;
            end_dlg_list->cont.index = 0; /* first entry in list -> index = 0 */
        } else {
            end_dlg_list->next = (tap_param_dlg_list_item *) g_malloc(sizeof (tap_param_dlg_list_item));
            end_dlg_list->next->cont.index = end_dlg_list->cont.index + 1;
            end_dlg_list = end_dlg_list->next;
        }
        end_dlg_list->dlg = NULL;
        end_dlg_list->param_items = g_malloc(dlg_data->nparams * sizeof (GtkWidget *));
        end_dlg_list->cont.win_title = dlg_data->win_title;
        end_dlg_list->cont.init_string = dlg_data->init_string;
        end_dlg_list->cont.tap_init_cb = dlg_data->tap_init_cb;
        end_dlg_list->cont.nparams = dlg_data->nparams;
        end_dlg_list->cont.params = dlg_data->params;
        end_dlg_list->args.title = g_strdup_printf("%s Filter", dlg_data->win_title);
        end_dlg_list->args.wants_apply_button = TRUE;
        end_dlg_list->args.activate_on_ok = FALSE;
        end_dlg_list->args.modal_and_transient = FALSE;
        end_dlg_list->next = NULL;
        dlg_data->index = end_dlg_list->cont.index;
        current_dlg = end_dlg_list;
    } else {
        /* Dialog is registered, find it */
        current_dlg = start_dlg_list;
        while(dlg_data->index != current_dlg->cont.index)
        {
            if(current_dlg->next == NULL) {
                /* could not find any dialog */
                return;
            }
            current_dlg = current_dlg->next;
        }
    }

    /* if the window is already open, bring it to front */
    if(current_dlg->dlg){
        gdk_window_raise(gtk_widget_get_window(current_dlg->dlg));
        return;
    }

    title = g_strdup_printf("Wireshark: %s: %s", current_dlg->cont.win_title , cf_get_display_name(&cfile));

    current_dlg->dlg=dlg_window_new(title);
    gtk_window_set_default_size(GTK_WINDOW(current_dlg->dlg), 300, -1);
    g_free(title);

    dlg_box=gtk_vbox_new(FALSE, 10);
    gtk_container_set_border_width(GTK_CONTAINER(dlg_box), 10);
    gtk_container_add(GTK_CONTAINER(current_dlg->dlg), dlg_box);
    gtk_widget_show(dlg_box);

    /* Parameter items */
    for(i=0;i<current_dlg->cont.nparams;i++) {
        /* Item box */
        item_box=gtk_hbox_new(FALSE, 3);

        switch (current_dlg->cont.params[i].type) {

        case PARAM_UINT:
        case PARAM_STRING:
            /* Label */
            label_with_colon=g_strdup_printf("%s:", current_dlg->cont.params[i].title);
            label=gtk_label_new(label_with_colon);
            g_free(label_with_colon);
            gtk_box_pack_start(GTK_BOX(item_box), label, FALSE, TRUE, 0);
            gtk_widget_show(label);

            /* Entry */
            item=gtk_entry_new();
            break;

        case PARAM_ENUM:
            /* Label */
            label_with_colon=g_strdup_printf("%s:", current_dlg->cont.params[i].title);
            label=gtk_label_new(label_with_colon);
            g_free(label_with_colon);
            gtk_box_pack_start(GTK_BOX(item_box), label, FALSE, TRUE, 0);
            gtk_widget_show(label);

            /* Combo box */
            item=gtk_combo_box_new_text();
            for (j = 0; current_dlg->cont.params[i].enum_vals[j].name != NULL;
                 j++)
                gtk_combo_box_append_text(GTK_COMBO_BOX(item),
                                          current_dlg->cont.params[i].enum_vals[j].description);
            gtk_combo_box_set_active(GTK_COMBO_BOX(item), 0);
            break;

        case PARAM_FILTER:
            /* Filter button */
            filter_bt=gtk_button_new_from_stock(WIRESHARK_STOCK_DISPLAY_FILTER_ENTRY);
            g_signal_connect(filter_bt, "clicked", G_CALLBACK(display_filter_construct_cb), &(current_dlg->args));
            gtk_box_pack_start(GTK_BOX(item_box), filter_bt, FALSE, TRUE, 0);
            gtk_widget_show(filter_bt);

            /* Entry */
            item=gtk_entry_new();
            filter=gtk_entry_get_text(GTK_ENTRY(main_display_filter_widget));
            if(filter){
                gtk_entry_set_text(GTK_ENTRY(item), filter);
            } else {
                colorize_filter_te_as_empty(item);
            }
            g_signal_connect(item, "changed", G_CALLBACK(filter_te_syntax_check_cb), NULL);
            g_object_set_data(G_OBJECT(item_box), E_FILT_AUTOCOMP_PTR_KEY, NULL);
            g_signal_connect(item, "key-press-event", G_CALLBACK (filter_string_te_key_pressed_cb), NULL);
            g_signal_connect(current_dlg->dlg, "key-press-event", G_CALLBACK (filter_parent_dlg_key_pressed_cb), NULL);
            g_object_set_data(G_OBJECT(filter_bt), E_FILT_TE_PTR_KEY, item);
            break;

        default:
            g_assert_not_reached();
            item=NULL;
            break;
        }
    
        gtk_box_pack_start(GTK_BOX(item_box), item, TRUE, TRUE, 0);
        current_dlg->param_items[i]=item;
        gtk_widget_show(item);

        gtk_box_pack_start(GTK_BOX(dlg_box), item_box, TRUE, TRUE, 0);
        gtk_widget_show(item_box);
    }

    /* button box */
    bbox = dlg_button_row_new(WIRESHARK_STOCK_CREATE_STAT, GTK_STOCK_CANCEL, NULL);
    gtk_box_pack_start(GTK_BOX(dlg_box), bbox, FALSE, FALSE, 0);
    gtk_widget_show(bbox);

    start_button = g_object_get_data(G_OBJECT(bbox), WIRESHARK_STOCK_CREATE_STAT);
    g_signal_connect(start_button, "clicked",
                     G_CALLBACK(tap_param_dlg_start_button_clicked), current_dlg);

    cancel_button = g_object_get_data(G_OBJECT(bbox), GTK_STOCK_CANCEL);
    window_set_cancel_button(current_dlg->dlg, cancel_button, window_cancel_button_cb);

    /* Catch the "activate" signal on all the text entries, so that
       if the user types Return there, we act as if the "Create Stat"
       button had been selected, as happens if Return is typed if
       some widget that *doesn't* handle the Return key has the input
       focus. */
    for(i=0;i<current_dlg->cont.nparams;i++){
        switch (current_dlg->cont.params[i].type) {

        case PARAM_ENUM:
            break;

        case PARAM_UINT:
        case PARAM_STRING:
        case PARAM_FILTER:
            dlg_set_activate(current_dlg->param_items[i], start_button);
            break;
        }
    }

    /* Give the initial focus to the first entry box. */
    if(current_dlg->cont.nparams>0){
        gtk_widget_grab_focus(current_dlg->param_items[0]);
    }

    gtk_widget_grab_default(start_button );

    g_signal_connect(current_dlg->dlg, "delete_event", G_CALLBACK(window_delete_event_cb), NULL);
    g_signal_connect(current_dlg->dlg, "destroy", G_CALLBACK(dlg_destroy_cb), current_dlg);

    gtk_widget_show_all(current_dlg->dlg);
    window_present(current_dlg->dlg);
}
