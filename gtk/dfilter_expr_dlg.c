/* dfilter_expr_dlg.c
 *
 * Allow the user to construct a subexpression of a display filter
 * expression, testing a particular field; display the tree of fields
 * and the relations and values with which it can be compared.
 *
 * Copyright 2000, Jeffrey C. Foster <jfoste@woodward.com> and
 * Guy Harris <guy@alum.mit.edu>
 *
 * $Id: dfilter_expr_dlg.c,v 1.39 2003/09/05 06:59:38 guy Exp $
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@ethereal.com>
 * Copyright 2000 Gerald Combs
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

/* Todo -
 * may want to check the enable field to decide if protocol should be in tree
 * improve speed of dialog box creation
 *	- I believe this is slow because of tree widget creation.
 *		1) could improve the widget
 *		2) keep a copy in memory after the first time.
 * user can pop multiple tree dialogs by pressing the "Tree" button multiple
 *	time.  not a good thing.
 * Sort the protocols and children
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <gtk/gtk.h>
#include <ctype.h>
#include <string.h>

#ifdef NEED_SNPRINTF_H
# include "snprintf.h"
#endif

#include "globals.h"
#include "main.h"
#include "ui_util.h"
#include "simple_dialog.h"
#include "dlg_utils.h"
#include "proto_dlg.h"
#include "filter_prefs.h"
#include "dfilter_expr_dlg.h"
#include "compat_macros.h"

#define E_DFILTER_EXPR_TREE_KEY			"dfilter_expr_tree"
#define E_DFILTER_EXPR_CURRENT_VAR_KEY		"dfilter_expr_current_var"
#define E_DFILTER_EXPR_RELATION_LABEL_KEY	"dfilter_expr_relation_label"
#define E_DFILTER_EXPR_RELATION_LIST_KEY	"dfilter_expr_relation_list"
#define E_DFILTER_EXPR_RANGE_LABEL_KEY		"dfilter_expr_range_label"
#define E_DFILTER_EXPR_RANGE_ENTRY_KEY		"dfilter_expr_range_entry"
#define E_DFILTER_EXPR_VALUE_LABEL_KEY		"dfilter_expr_value_label"
#define E_DFILTER_EXPR_VALUE_ENTRY_KEY		"dfilter_expr_value_entry"
#define E_DFILTER_EXPR_VALUE_LIST_KEY		"dfilter_expr_value_list"
#define E_DFILTER_EXPR_VALUE_LIST_SW_KEY	"dfilter_expr_value_list_sw"
#define E_DFILTER_EXPR_ACCEPT_BT_KEY		"dfilter_expr_accept_bt"
#define E_DFILTER_EXPR_VALUE_KEY		"dfilter_expr_value"

typedef struct protocol_data {
  char 	*abbrev;
  int  	hfinfo_index;
} protocol_data_t;

static void show_relations(GtkWidget *relation_label, GtkWidget *relation_list,
                           ftenum_t ftype);
static gboolean relation_is_presence_test(const char *string);
static void add_relation_list(GtkWidget *relation_list, char *relation);
static void build_boolean_values(GtkWidget *value_list_scrolled_win,
                                 GtkWidget *value_list,
                                 const true_false_string *values);
static void build_enum_values(GtkWidget *value_list_scrolled_win,
                              GtkWidget *value_list,
                              const value_string *values);
static void add_value_list_item(GtkWidget *value_list, gchar *string,
                                gpointer data);
static void display_value_fields(header_field_info *hfinfo,
                                 gboolean is_comparison, GtkWidget *value_label,
                                 GtkWidget *value_entry, GtkWidget *value_list,
                                 GtkWidget *value_list_scrolled_win,
                                 GtkWidget *range_label,
                                 GtkWidget *range_entry);

/*
 * Note that this is called every time the user clicks on an item,
 * whether it is already selected or not.
 */
#if GTK_MAJOR_VERSION < 2
static void
field_select_row_cb(GtkWidget *tree, GList *node, gint column _U_,
                    gpointer user_data _U_)
#else
static void
field_select_row_cb(GtkTreeSelection *sel, gpointer tree)
#endif
{
    GtkWidget *window = gtk_widget_get_toplevel(tree);
    GtkWidget *relation_label = OBJECT_GET_DATA(window,
                                                E_DFILTER_EXPR_RELATION_LABEL_KEY);
    GtkWidget *relation_list = OBJECT_GET_DATA(window,
                                               E_DFILTER_EXPR_RELATION_LIST_KEY);
    GtkWidget *range_label = OBJECT_GET_DATA(window,
                                             E_DFILTER_EXPR_RANGE_LABEL_KEY);
    GtkWidget *range_entry = OBJECT_GET_DATA(window,
                                             E_DFILTER_EXPR_RANGE_ENTRY_KEY);
    GtkWidget *value_label = OBJECT_GET_DATA(window,
                                             E_DFILTER_EXPR_VALUE_LABEL_KEY);
    GtkWidget *value_entry = OBJECT_GET_DATA(window,
                                             E_DFILTER_EXPR_VALUE_ENTRY_KEY);
    GtkWidget *value_list = OBJECT_GET_DATA(window,
                                            E_DFILTER_EXPR_VALUE_LIST_KEY);
    GtkWidget *value_list_scrolled_win = OBJECT_GET_DATA(window,
                                                         E_DFILTER_EXPR_VALUE_LIST_SW_KEY);
    GtkWidget *accept_bt = OBJECT_GET_DATA(window,
                                           E_DFILTER_EXPR_ACCEPT_BT_KEY);
    header_field_info *hfinfo, *cur_hfinfo;
    const char *value_type;
    char value_label_string[1024+1];   /* XXX - should be large enough */
#if GTK_MAJOR_VERSION >= 2
    GtkTreeModel *model;
    GtkTreeIter   iter;

    if (!gtk_tree_selection_get_selected(sel, &model, &iter))
        return;
    gtk_tree_model_get(model, &iter, 1, &hfinfo, -1);
#else

    hfinfo = gtk_ctree_node_get_row_data(GTK_CTREE(tree),
                                         GTK_CTREE_NODE(node));
#endif

    /*
     * What was the item that was last selected?
     */
    cur_hfinfo = OBJECT_GET_DATA(window, E_DFILTER_EXPR_CURRENT_VAR_KEY);
    if (cur_hfinfo == hfinfo) {
        /*
         * It's still selected; no need to change anything.
         */
        return;
    }

    /*
     * Mark it as currently selected.
     */
    OBJECT_SET_DATA(window, E_DFILTER_EXPR_CURRENT_VAR_KEY, hfinfo);

    show_relations(relation_label, relation_list, hfinfo->type);

    /*
     * Set the label for the value to indicate what type of value
     * it is.
     */
    value_type = ftype_pretty_name(hfinfo->type);
    if (value_type != NULL) {
        /*
         * Indicate what type of value it is.
         */
        snprintf(value_label_string, sizeof value_label_string,
                 "Value (%s)", value_type);
        gtk_label_set_text(GTK_LABEL(value_label), value_label_string);
    }

    /*
     * Clear the entry widget for the value, as whatever
     * was there before doesn't apply.
     */
    gtk_entry_set_text(GTK_ENTRY(value_entry), "");

    switch (hfinfo->type) {

    case FT_BOOLEAN:
        /*
         * The list of values should be the strings for "true"
         * and "false"; show them in the value list.
         */
        build_boolean_values(value_list_scrolled_win, value_list,
                             hfinfo->strings);
        break;

    case FT_UINT8:
    case FT_UINT16:
    case FT_UINT24:
    case FT_UINT32:
    case FT_INT8:
    case FT_INT16:
    case FT_INT24:
    case FT_INT32:
        /*
         * If this has a value_string table associated with it,
         * fill up the list of values, otherwise clear the list
         * of values.
         */
        if (hfinfo->strings != NULL) {
            build_enum_values(value_list_scrolled_win, value_list,
                              hfinfo->strings);
        } else
#if GTK_MAJOR_VERSION < 2
            gtk_list_clear_items(GTK_LIST(value_list), 0, -1);
#else
            gtk_list_store_clear(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(value_list))));
#endif
        break;

    default:
        /*
         * Clear the list of values.
         */
#if GTK_MAJOR_VERSION < 2
        gtk_list_clear_items(GTK_LIST(value_list), 0, -1);
#else
        gtk_list_store_clear(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(value_list))));
#endif
        break;
    }

    /*
     * Display various items for the value, as appropriate.
     * The relation we start out with is never a comparison.
     */
    display_value_fields(hfinfo, FALSE, value_label, value_entry,
                         value_list, value_list_scrolled_win, range_label, range_entry);

    /*
     * XXX - in browse mode, there always has to be something
     * selected, so this should always be sensitive.
     */
    gtk_widget_set_sensitive(accept_bt, TRUE);
}

static void
show_relations(GtkWidget *relation_label, GtkWidget *relation_list,
               ftenum_t ftype)
{
	/*
	 * Clear out the currently displayed list of relations.
	 */
#if GTK_MAJOR_VERSION < 2
	gtk_list_clear_items(GTK_LIST(relation_list), 0, -1);
#else
        gtk_list_store_clear(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(relation_list))));
#endif

	/*
	 * Add the supported relations.
	 */
	add_relation_list(relation_list, "is present");
	if (ftype_can_eq(ftype) ||
	    (ftype_can_slice(ftype) && ftype_can_eq(FT_BYTES)))
		add_relation_list(relation_list, "==");
	if (ftype_can_ne(ftype) ||
	    (ftype_can_slice(ftype) && ftype_can_ne(FT_BYTES)))
		add_relation_list(relation_list, "!=");
	if (ftype_can_gt(ftype) ||
	    (ftype_can_slice(ftype) && ftype_can_gt(FT_BYTES)))
		add_relation_list(relation_list, ">");
	if (ftype_can_lt(ftype) ||
	    (ftype_can_slice(ftype) && ftype_can_lt(FT_BYTES)))
		add_relation_list(relation_list, "<");
	if (ftype_can_ge(ftype) ||
	    (ftype_can_slice(ftype) && ftype_can_ge(FT_BYTES)))
		add_relation_list(relation_list, ">=");
	if (ftype_can_le(ftype) ||
	    (ftype_can_slice(ftype) && ftype_can_le(FT_BYTES)))
		add_relation_list(relation_list, "<=");
	if (ftype_can_contains(ftype) ||
	    (ftype_can_slice(ftype) && ftype_can_contains(FT_BYTES)))
		add_relation_list(relation_list, "contains");

	/*
	 * And show the list.
	 */
	gtk_widget_show(relation_label);
	gtk_widget_show(relation_list);
}

/*
 * Given a string that represents a test to be made on a field, returns
 * TRUE if it tests for the field's presence, FALSE otherwise.
 */
static gboolean
relation_is_presence_test(const char *string)
{
	return (strcmp(string, "is present") == 0);
}

static void
add_relation_list(GtkWidget *relation_list, char *relation)
{
#if GTK_MAJOR_VERSION < 2
    GtkWidget *label, *item;

    label = gtk_label_new(relation);
    item = gtk_list_item_new();

    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_container_add(GTK_CONTAINER(item), label);
    gtk_widget_show(label);
    gtk_container_add(GTK_CONTAINER(relation_list), item);
    gtk_widget_show(item);
#else
    GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(relation_list)));
    GtkTreeIter   iter;

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, relation, -1);
#endif
}

#if GTK_MAJOR_VERSION < 2
static void
relation_list_sel_cb(GtkList *relation_list, GtkWidget *child _U_,
                     gpointer user_data _U_)
#else
static void
relation_list_sel_cb(GtkTreeSelection *sel, gpointer user_data _U_)
#endif
{
#if GTK_MAJOR_VERSION < 2
    GtkWidget *window = gtk_widget_get_toplevel(GTK_WIDGET(relation_list));
#else
    GtkWidget *window = gtk_widget_get_toplevel(GTK_WIDGET(gtk_tree_selection_get_tree_view(sel)));
#endif
    GtkWidget *range_label =
        OBJECT_GET_DATA(window, E_DFILTER_EXPR_RANGE_LABEL_KEY);
    GtkWidget *range_entry =
        OBJECT_GET_DATA(window, E_DFILTER_EXPR_RANGE_ENTRY_KEY);
    GtkWidget *value_label =
        OBJECT_GET_DATA(window, E_DFILTER_EXPR_VALUE_LABEL_KEY);
    GtkWidget *value_entry =
        OBJECT_GET_DATA(window, E_DFILTER_EXPR_VALUE_ENTRY_KEY);
    GtkWidget *value_list =
        OBJECT_GET_DATA(window, E_DFILTER_EXPR_VALUE_LIST_KEY);
    GtkWidget *value_list_scrolled_win =
        OBJECT_GET_DATA(window, E_DFILTER_EXPR_VALUE_LIST_SW_KEY);
    header_field_info *hfinfo =
        OBJECT_GET_DATA(window, E_DFILTER_EXPR_CURRENT_VAR_KEY);
    gchar *item_str;
#if GTK_MAJOR_VERSION < 2
    GList *sl;
    GtkWidget *item, *item_label;

    /*
     * What's the relation?
     */
    sl = GTK_LIST(relation_list)->selection;
    item = GTK_WIDGET(sl->data);
    item_label = GTK_BIN(item)->child;
    gtk_label_get(GTK_LABEL(item_label), &item_str);
#else
    GtkTreeModel *model;
    GtkTreeIter   iter;

    /*
     * What's the relation?
     */
    if (!gtk_tree_selection_get_selected(sel, &model, &iter))
        return;
    gtk_tree_model_get(model, &iter, 0, &item_str, -1);
#endif

    /*
     * Update the display of various items for the value, as appropriate.
     */
    display_value_fields(hfinfo,
                         !relation_is_presence_test(item_str),
                         value_label, value_entry, value_list,
                         value_list_scrolled_win, range_label, range_entry);
#if GTK_MAJOR_VERSION >= 2
    g_free(item_str);
#endif
}

static void
build_boolean_values(GtkWidget *value_list_scrolled_win, GtkWidget *value_list,
                     const true_false_string *values)
{
    static const true_false_string true_false = { "True", "False" };
#if GTK_MAJOR_VERSION >= 2
    GtkTreeSelection *sel;
    GtkTreeIter       iter;

    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(value_list));
#endif

    /*
     * Clear out the items for the list, and put in the names
     * from the value_string list.
     */
#if GTK_MAJOR_VERSION < 2
    gtk_list_clear_items(GTK_LIST(value_list), 0, -1);
#else
    gtk_list_store_clear(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(value_list))));
#endif

    /*
     * Put the list in single mode, so we don't get any selection
     * events while we're building it (i.e., so we don't get any
     * on a list item BEFORE WE GET TO SET THE DATA FOR THE LIST
     * ITEM SO THAT THE HANDLER CAN HANDLE IT).
     */
#if GTK_MAJOR_VERSION < 2
    gtk_list_set_selection_mode(GTK_LIST(value_list), GTK_SELECTION_SINGLE);
#else
    gtk_tree_selection_set_mode(sel, GTK_SELECTION_SINGLE);
#endif

    /*
     * Build the list.
     */
    if (values == NULL)
        values = &true_false;
    add_value_list_item(value_list, values->true_string, (gpointer)values);
    add_value_list_item(value_list, values->false_string, NULL);

    /*
     * OK, we're done, so we can finally put it in browse mode.
     * Select the first item, so that the user doesn't have to, under
     * the assumption that they're most likely to test if something
     * is true, not false.
     */
#if GTK_MAJOR_VERSION < 2
    gtk_list_set_selection_mode(GTK_LIST(value_list), GTK_SELECTION_BROWSE);
    gtk_list_select_item(GTK_LIST(value_list), 0);
#else
    gtk_tree_selection_set_mode(sel, GTK_SELECTION_BROWSE);
    gtk_tree_model_get_iter_first(gtk_tree_view_get_model(GTK_TREE_VIEW(value_list)), &iter);
    gtk_tree_selection_select_iter(sel, &iter);
#endif

    gtk_widget_show_all(value_list_scrolled_win);
}

static void
build_enum_values(GtkWidget *value_list_scrolled_win _U_, GtkWidget *value_list,
                  const value_string *values)
{
#if GTK_MAJOR_VERSION >= 2
    GtkTreeSelection *sel;

    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(value_list));
#endif
    /*
     * Clear out the items for the list, and put in the names
     * from the value_string list.
     */
#if GTK_MAJOR_VERSION < 2
    gtk_list_clear_items(GTK_LIST(value_list), 0, -1);
#else
    gtk_list_store_clear(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(value_list))));
#endif

    /*
     * Put the list in single mode, so we don't get any selection
     * events while we're building it (i.e., so we don't get any
     * on a list item BEFORE WE GET TO SET THE DATA FOR THE LIST
     * ITEM SO THAT THE HANDLER CAN HANDLE IT).
     */
#if GTK_MAJOR_VERSION < 2
    gtk_list_set_selection_mode(GTK_LIST(value_list), GTK_SELECTION_SINGLE);
#else
    gtk_tree_selection_set_mode(sel, GTK_SELECTION_SINGLE);
#endif

    /*
     * Build the list.
     */
    while (values->strptr != NULL) {
        add_value_list_item(value_list, values->strptr,
                            (gpointer)values);
        values++;
    }

    /*
     * OK, we're done, so we can finally put it in browse mode.
     */
#if GTK_MAJOR_VERSION < 2
    gtk_list_set_selection_mode(GTK_LIST(value_list), GTK_SELECTION_BROWSE);
#else
    gtk_tree_selection_set_mode(sel, GTK_SELECTION_BROWSE);
#endif
}

static void
add_value_list_item(GtkWidget *value_list, gchar *string, gpointer data)
{
#if GTK_MAJOR_VERSION < 2
    GtkWidget *label, *item;

    label = gtk_label_new(string);
    item = gtk_list_item_new();

    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_container_add(GTK_CONTAINER(item), label);
    gtk_widget_show(label);
    gtk_container_add(GTK_CONTAINER(value_list), item);
    OBJECT_SET_DATA(item, E_DFILTER_EXPR_VALUE_KEY, data);
    gtk_widget_show(item);
#else
    GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(value_list)));
    GtkTreeIter       iter;

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, string, 1, data, -1);
#endif
}

/*
 * Show or hide the various values fields as appropriate for the field
 * and currently-selected relation.
 */
static void
display_value_fields(header_field_info *hfinfo, gboolean is_comparison,
                     GtkWidget *value_label, GtkWidget *value_entry,
                     GtkWidget *value_list _U_,
                     GtkWidget *value_list_scrolled_win, GtkWidget *range_label,
                     GtkWidget *range_entry)
{
	gboolean show_value_label = FALSE;

	/*
	 * Either:
	 *
	 *	this is an FT_NONE variable, in which case you can
	 *	only check whether it's present or absent in the
	 *	protocol tree
	 *
	 * or
	 *
	 *	this is a Boolean variable, in which case you
	 *	can't specify a value to compare with, you can
	 *	only specify whether to test for the Boolean
	 *	being true or to test for it being false
	 *
	 * or
	 *
	 *	this isn't a Boolean variable, in which case you
	 *	can test for its presence in the protocol tree,
	 *	and the default relation is such a test, in
	 *	which case you don't compare with a value
	 *
	 * so we hide the value entry.
	 */
	if (is_comparison) {
		/*
		 * The relation is a comparison; display the entry for
		 * the value with which to compare.
		 */
		gtk_widget_show(value_entry);

		/*
		 * We're showing the entry; show the label as well.
		 */
		show_value_label = TRUE;
	} else {
		/*
		 * The relation isn't a comparison; there's no value with
		 * which to compare, so don't show the entry for it.
		 */
		gtk_widget_hide(value_entry);
	}

	switch (hfinfo->type) {

	case FT_BOOLEAN:
		if (is_comparison) {
			/*
			 * The relation is a comparison, so we're showing
			 * an entry for the value with which to compare;
			 * show the list of names for values as well.
			 * (The list of values contains the strings for
			 * "true" and "false".)
			 */
			gtk_widget_show_all(value_list_scrolled_win);

			/*
			 * We're showing the value list; show the label as
			 * well.
			 */
			show_value_label = TRUE;
		} else {
			/*
			 * It's not a comparison, so we're not showing
			 * the entry for the value; don't show the
			 * list of names for values, either.
			 */
			gtk_widget_hide_all(value_list_scrolled_win);
		}
		break;

	case FT_UINT8:
	case FT_UINT16:
	case FT_UINT24:
	case FT_UINT32:
	case FT_INT8:
	case FT_INT16:
	case FT_INT24:
	case FT_INT32:
		if (hfinfo->strings != NULL) {
			/*
			 * We have a list of values to show.
			 */
			if (is_comparison) {
				/*
				 * The relation is a comparison, so we're
				 * showing an entry for the value with
				 * which to compare; show the list of
				 * names for values as well.
				 */
				gtk_widget_show_all(value_list_scrolled_win);

				/*
				 * We're showing the entry; show the label
				 * as well.
				 */
				show_value_label = TRUE;
			} else {
				/*
				 * It's not a comparison, so we're not showing
				 * the entry for the value; don't show the
				 * list of names for values, either.
				 */
				gtk_widget_hide_all(value_list_scrolled_win);
			}
		} else {
			/*
			 * There is no list of names for values, so don't
			 * show it.
			 */
			gtk_widget_hide_all(value_list_scrolled_win);
		}
		break;

	default:
		/*
		 * There is no list of names for values; hide the list.
		 */
		gtk_widget_hide_all(value_list_scrolled_win);
		break;
	}

	if (show_value_label)
		gtk_widget_show(value_label);
	else
		gtk_widget_hide(value_label);

	/*
	 * Is this a comparison, and are ranges supported by this type?
	 * If both are true, show the range stuff, otherwise hide it.
	 */
	if (is_comparison && ftype_can_slice(hfinfo->type)) {
		gtk_widget_show(range_label);
		gtk_widget_show(range_entry);
	} else {
		gtk_widget_hide(range_label);
		gtk_widget_hide(range_entry);
	}
}

#if GTK_MAJOR_VERSION < 2
static void
value_list_sel_cb(GtkList *value_list, GtkWidget *child,
                  gpointer value_entry_arg)
#else
static void
value_list_sel_cb(GtkTreeSelection *sel, gpointer value_entry_arg)
#endif
{
    GtkWidget *value_entry = value_entry_arg;
#if GTK_MAJOR_VERSION < 2
    GtkWidget *window = gtk_widget_get_toplevel(GTK_WIDGET(value_list));
#else
    GtkWidget *window = gtk_widget_get_toplevel(GTK_WIDGET(gtk_tree_selection_get_tree_view(sel)));
    GtkTreeModel *model;
    GtkTreeIter   iter;
#endif
    header_field_info *hfinfo = OBJECT_GET_DATA(window,
                                                E_DFILTER_EXPR_CURRENT_VAR_KEY);
    const value_string *value = NULL;
    char value_string[11+1];	/* long enough for 32-bit octal value */

#if GTK_MAJOR_VERSION < 2
    value = OBJECT_GET_DATA(child, E_DFILTER_EXPR_VALUE_KEY);
#else
    if (!gtk_tree_selection_get_selected(sel, &model, &iter))
        return;
    gtk_tree_model_get(model, &iter, 1, &value, -1);
#endif

    /*
     * This should either be a numeric type or a Boolean type.
     */
    if (hfinfo->type == FT_BOOLEAN) {
        /*
         * Boolean type; if the value key for the selected item
         * is non-null, it's the item for "true", otherwise it's
         * the item for "false".  Compare with 1 if we're
         * testing for "true", and compare with 0 if we're
         * testing for "false".
         */
        if (value != NULL)
            strcpy(value_string, "1");
        else
            strcpy(value_string, "0");
    } else {
        /*
         * Numeric type; get the value corresponding to the
         * selected item, and display it in the base for this
         * field.
         */
        switch (hfinfo->display) {

        case BASE_DEC:
            switch (hfinfo->type) {

            case FT_UINT8:
            case FT_UINT16:
            case FT_UINT32:
                snprintf(value_string, sizeof value_string,
                         "%u", value->value);
                break;

            case FT_INT8:
            case FT_INT16:
            case FT_INT32:
                snprintf(value_string, sizeof value_string,
                         "%d", value->value);
                break;

            default:
                g_assert_not_reached();
            }
            break;

        case BASE_HEX:
            snprintf(value_string, sizeof value_string, "0x%x",
                     value->value);
            break;

        case BASE_OCT:
            snprintf(value_string, sizeof value_string, "%#o",
                     value->value);
            break;

        default:
            g_assert_not_reached();
        }
    }
    gtk_entry_set_text(GTK_ENTRY(value_entry), value_string);
}

static void
dfilter_report_bad_value(char *format, ...)
{
	char error_msg_buf[1024];
	va_list args;

	va_start(args, format);
	vsnprintf(error_msg_buf, sizeof error_msg_buf, format, args);
	va_end(args);

	simple_dialog(ESD_TYPE_CRIT | ESD_TYPE_MODAL, NULL,
	    "%s", error_msg_buf);
}

static void
dfilter_expr_dlg_accept_cb(GtkWidget *w, gpointer filter_te_arg)
{
    GtkWidget *filter_te = filter_te_arg;
    GtkWidget *window = gtk_widget_get_toplevel(w);
    GtkWidget *relation_list =
        OBJECT_GET_DATA(window, E_DFILTER_EXPR_RELATION_LIST_KEY);
    GtkWidget *range_entry =
        OBJECT_GET_DATA(window, E_DFILTER_EXPR_RANGE_ENTRY_KEY);
    GtkWidget *value_entry =
        OBJECT_GET_DATA(window, E_DFILTER_EXPR_VALUE_ENTRY_KEY);
    header_field_info *hfinfo;
    gchar        *item_str;
    gchar        *range_str, *stripped_range_str;
    gchar        *value_str, *stripped_value_str;
    int           pos;
    gchar        *chars;
    ftenum_t      ftype;
    gboolean      can_compare;
    fvalue_t     *fvalue;
#if GTK_MAJOR_VERSION < 2
    GList        *sl;
    GtkWidget    *item, *item_label;
#else
    GtkTreeModel *model;
    GtkTreeIter   iter;
#endif

    /*
     * Get the variable to be tested.
     */
    hfinfo = OBJECT_GET_DATA(window, E_DFILTER_EXPR_CURRENT_VAR_KEY);

    /*
     * Get the relation to use, if any.
     */
    if (GTK_WIDGET_VISIBLE(relation_list)) {
        /*
         * The list of relations is visible, so we can get a
         * relation operator from it.
         */
#if GTK_MAJOR_VERSION < 2
        sl = GTK_LIST(relation_list)->selection;
        item = GTK_WIDGET(sl->data);
        item_label = GTK_BIN(item)->child;
        gtk_label_get(GTK_LABEL(item_label), &item_str);
#else
        if (gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(relation_list)),
                                            &model, &iter))
            gtk_tree_model_get(model, &iter, 0, &item_str, -1);
        else
            item_str = NULL;
#endif
    } else
        item_str = NULL;	/* no relation operator */

    /*
     * Get the range to use, if any.
     */
    if (GTK_WIDGET_VISIBLE(range_entry)) {
        range_str = g_strdup(gtk_entry_get_text(GTK_ENTRY(range_entry)));
        stripped_range_str = g_strstrip(range_str);
        if (strcmp(stripped_range_str, "") == 0) {
            /*
             * No range was specified.
             */
            g_free(range_str);
            range_str = NULL;
            stripped_range_str = NULL;
        }

        /*
         * XXX - check it for validity?
         */
    } else {
        range_str = NULL;
        stripped_range_str = NULL;
    }

    /*
     * If a range was specified, the type of the LHS of the
     * comparison is FT_BYTES; otherwise, it's the type of the field.
     */
    if (range_str == NULL)
        ftype = hfinfo->type;
    else
        ftype = FT_BYTES;

    /*
     * Make sure the relation is valid for the type in question.
     * We may be offering relations that the type of the field
     * can't support, because the field's type supports slicing,
     * and the relation *is* supported on byte strings.
     */
    if (strcmp(item_str, "==") == 0)
        can_compare = ftype_can_eq(ftype);
    else if (strcmp(item_str, "!=") == 0)
        can_compare = ftype_can_ne(ftype);
    else if (strcmp(item_str, ">") == 0)
        can_compare = ftype_can_gt(ftype);
    else if (strcmp(item_str, "<") == 0)
        can_compare = ftype_can_lt(ftype);
    else if (strcmp(item_str, ">=") == 0)
        can_compare = ftype_can_ge(ftype);
    else if (strcmp(item_str, "<=") == 0)
        can_compare = ftype_can_le(ftype);
    else if (strcmp(item_str, "contains") == 0)
        can_compare = ftype_can_contains(ftype);
    else
        can_compare = TRUE;	/* not a comparison */
    if (!can_compare) {
        if (range_str == NULL) {
            simple_dialog(ESD_TYPE_CRIT | ESD_TYPE_MODAL, NULL,
                          "That field cannot be tested with \"%s\".",
                          item_str);
        } else {
            simple_dialog(ESD_TYPE_CRIT | ESD_TYPE_MODAL, NULL,
                          "Ranges of that field cannot be tested with \"%s\".",
                          item_str);
        }
        if (range_str != NULL)
            g_free(range_str);
        return;
    }

    /*
     * Get the value to use, if any.
     */
    if (GTK_WIDGET_VISIBLE(value_entry)) {
        value_str = g_strdup(gtk_entry_get_text(GTK_ENTRY(value_entry)));
        stripped_value_str = g_strstrip(value_str);
        if (strcmp(stripped_value_str, "") == 0) {
            /*
             * This field takes a value, but they didn't supply
             * one.
             */
            simple_dialog(ESD_TYPE_CRIT | ESD_TYPE_MODAL, NULL,
                          "That field must be compared with a value, "
                          "but you didn't specify a value with which to "
                          "compare it.");
            if (range_str != NULL)
                g_free(range_str);
            g_free(value_str);
            return;
        }

        /*
         * Make sure the value is valid.
         *
         * If no range string was specified, it must be valid
         * for the type of the field; if a range string was
         * specified, must be valid for FT_BYTES.
         */
    	if (strcmp(item_str, "contains") == 0) {
		fvalue = fvalue_from_unparsed(ftype, stripped_value_str, TRUE,
					    dfilter_report_bad_value);
	}
	else {
		fvalue = fvalue_from_unparsed(ftype, stripped_value_str, FALSE,
					    dfilter_report_bad_value);
	}
        if (fvalue == NULL) {
            /*
             * It's not valid.
             *
             * The dialog box was already popped up by
             * "dfilter_report_bad_value()".
             */
            if (range_str != NULL)
                g_free(range_str);
            g_free(value_str);
            return;
        }
        fvalue_free(fvalue);
    } else {
        value_str = NULL;
        stripped_value_str = NULL;
    }

    /*
     * Insert the expression at the current cursor position.
     * If there's a non-whitespace character to the left of it,
     * insert a blank first; if there's a non-whitespace character
     * to the right of it, insert a blank after it.
     */
    pos = gtk_editable_get_position(GTK_EDITABLE(filter_te));
    chars = gtk_editable_get_chars(GTK_EDITABLE(filter_te), pos, pos + 1);
    if (strcmp(chars, "") != 0 && !isspace((unsigned char)chars[0]))
        gtk_editable_insert_text(GTK_EDITABLE(filter_te), " ", 1, &pos);
    g_free(chars);

    gtk_editable_insert_text(GTK_EDITABLE(filter_te), hfinfo->abbrev,
                             strlen(hfinfo->abbrev), &pos);
    if (range_str != NULL) {
        gtk_editable_insert_text(GTK_EDITABLE(filter_te), "[", 1, &pos);
        gtk_editable_insert_text(GTK_EDITABLE(filter_te),
                                 stripped_range_str, strlen(stripped_range_str), &pos);
        gtk_editable_insert_text(GTK_EDITABLE(filter_te), "]", 1, &pos);
        g_free(range_str);
    }
    if (item_str != NULL && !relation_is_presence_test(item_str)) {
        gtk_editable_insert_text(GTK_EDITABLE(filter_te), " ", 1, &pos);
        gtk_editable_insert_text(GTK_EDITABLE(filter_te), item_str,
                                 strlen(item_str), &pos);
    }
    if (value_str != NULL) {
        gtk_editable_insert_text(GTK_EDITABLE(filter_te), " ", 1, &pos);
        switch (hfinfo->type) {

        case FT_STRING:
        case FT_STRINGZ:
        case FT_UINT_STRING:
            /*
             * Put quotes around the string.
             */
            gtk_editable_insert_text(GTK_EDITABLE(filter_te), "\"",
                                     1, &pos);

        default:
            break;
        }
        gtk_editable_insert_text(GTK_EDITABLE(filter_te),
                                 stripped_value_str, strlen(stripped_value_str), &pos);
        switch (hfinfo->type) {

        case FT_STRING:
        case FT_STRINGZ:
        case FT_UINT_STRING:
            /*
             * Put quotes around the string.
             */
            gtk_editable_insert_text(GTK_EDITABLE(filter_te), "\"",
                                     1, &pos);

        default:
            break;
        }
        g_free(value_str);
    }
    chars = gtk_editable_get_chars(GTK_EDITABLE(filter_te), pos + 1, pos + 2);
    if (strcmp(chars, "") != 0 && !isspace((unsigned char)chars[0]))
        gtk_editable_insert_text(GTK_EDITABLE(filter_te), " ", 1, &pos);
    g_free(chars);

    /*
     * Put the cursor after the expression we just entered into
     * the text entry widget.
     */
    gtk_editable_set_position(GTK_EDITABLE(filter_te), pos);

    /*
     * We're done; destroy the dialog box (which is the top-level
     * widget for the "Accept" button).
     */
    gtk_widget_destroy(window);
#if GTK_MAJOR_VERSION >= 2
    g_free(item_str);
#endif
}

static void
dfilter_expr_dlg_cancel_cb(GtkWidget *w _U_, gpointer parent_w)
{
	/*
	 * User pressed the cancel button; close the dialog box.
	 */
	gtk_widget_destroy(GTK_WIDGET(parent_w));
}

static void
dfilter_expr_dlg_destroy_cb(GtkWidget *w, gpointer filter_te)
{
	/*
	 * The dialog box is being destroyed; disconnect from the
	 * "destroy" signal on the text entry box to which we're
	 * attached, as the handler for that signal is supposed
	 * to destroy us, but we're already gone.
	 */
	SIGNAL_DISCONNECT_BY_FUNC(filter_te, dfilter_expr_dlg_cancel_cb, w);
}

/*
 * Length of string used for protocol fields.
 */
#define TAG_STRING_LEN	256

void
dfilter_expr_dlg_new(GtkWidget *filter_te)
{
    GtkWidget *window;
    GtkWidget *main_vb;
    GtkWidget *hb;
    GtkWidget *col1_vb;
    GtkWidget *tree_label, *tree, *tree_scrolled_win;
    GtkWidget *col2_vb;
    GtkWidget *relation_label, *relation_list;
    GtkWidget *range_label, *range_entry;
    GtkWidget *value_vb;
    GtkWidget *value_label, *value_entry, *value_list_scrolled_win, *value_list;
    GtkWidget *list_bb, *alignment, *accept_bt, *close_bt;
    header_field_info       *hfinfo;
    int i, len;
#if GTK_MAJOR_VERSION < 2
    void *cookie;
    gchar *name;
    GHashTable *proto_array;
    GtkCTreeNode *protocol_node, *item_node;
#else
    GtkTreeStore *store;
    GtkTreeSelection *selection;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeIter iter;
    GtkListStore      *l_store;
    GtkTreeSelection  *l_sel;
#endif

    window = dlg_window_new("Ethereal: Filter Expression");
    gtk_container_set_border_width(GTK_CONTAINER(window), 5);

    main_vb = gtk_vbox_new(FALSE, 5);
    gtk_container_border_width(GTK_CONTAINER(main_vb), 5);
    gtk_container_add(GTK_CONTAINER(window), main_vb);
    gtk_widget_show(main_vb);

    hb = gtk_hbox_new(FALSE, 5);
    gtk_container_border_width(GTK_CONTAINER(hb), 5);
    gtk_container_add(GTK_CONTAINER(main_vb), hb);
    gtk_widget_show(hb);

    col1_vb = gtk_vbox_new(FALSE, 5);
    gtk_container_border_width(GTK_CONTAINER(col1_vb), 5);
    gtk_container_add(GTK_CONTAINER(hb), col1_vb);
    gtk_widget_show(col1_vb);

    tree_label = gtk_label_new("Field name");
    gtk_misc_set_alignment(GTK_MISC(tree_label), 0.0, 0.0);
    gtk_box_pack_start(GTK_BOX(col1_vb), tree_label, FALSE, FALSE, 0);
    gtk_widget_show(tree_label);

    tree_scrolled_win = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(tree_scrolled_win),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    WIDGET_SET_SIZE(tree_scrolled_win, 300, 400);
    gtk_box_pack_start(GTK_BOX(col1_vb), tree_scrolled_win, FALSE, FALSE, 0);
    gtk_widget_show(tree_scrolled_win);

#if GTK_MAJOR_VERSION < 2
    tree = ctree_new(1, 0);
    SIGNAL_CONNECT(tree, "tree-select-row", field_select_row_cb, tree);
#else
    store = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
    tree = tree_view_new(GTK_TREE_MODEL(store));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), FALSE);
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_BROWSE);
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Field name", renderer,
                                                      "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_set_sort_column_id(column, 0);
    SIGNAL_CONNECT(selection, "changed", field_select_row_cb, tree);
#endif
    gtk_container_add(GTK_CONTAINER(tree_scrolled_win), tree);

#if GTK_MAJOR_VERSION < 2
    /*
     * GTK's annoying CTree widget will deliver a selection event
     * the instant you add an item to the tree, *the fact that you
     * haven't even had time to set the item's row data nonwithstanding*.
     *
     * We'll put the widget into GTK_SELECTION_SINGLE mode in the
     * hopes that it's *STOP DOING THAT*.
     */
    gtk_clist_set_selection_mode(GTK_CLIST(tree),
                                 GTK_SELECTION_SINGLE);
#endif

    col2_vb = gtk_vbox_new(FALSE, 5);
    gtk_container_border_width(GTK_CONTAINER(col2_vb), 5);
    gtk_container_add(GTK_CONTAINER(hb), col2_vb);
    gtk_widget_show(col2_vb);

    relation_label = gtk_label_new("Relation");
    gtk_misc_set_alignment(GTK_MISC(relation_label), 0.0, 0.0);
    gtk_box_pack_start(GTK_BOX(col2_vb), relation_label, FALSE, FALSE, 0);

#if GTK_MAJOR_VERSION < 2
    relation_list = gtk_list_new();
    gtk_list_set_selection_mode(GTK_LIST(relation_list),
                                GTK_SELECTION_BROWSE);
#else
    l_store = gtk_list_store_new(1, G_TYPE_STRING);
    relation_list = tree_view_new(GTK_TREE_MODEL(l_store));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(relation_list), FALSE);
    g_object_unref(G_OBJECT(l_store));
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("relation", renderer,
                                                      "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(relation_list), column);
    l_sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(relation_list));
    gtk_tree_selection_set_mode(l_sel, GTK_SELECTION_BROWSE);
#endif
    gtk_box_pack_start(GTK_BOX(col2_vb), relation_list, TRUE, TRUE, 0);

    range_label = gtk_label_new("Range (offset:length)");
    gtk_misc_set_alignment(GTK_MISC(range_label), 0.0, 0.0);
    gtk_box_pack_start(GTK_BOX(col2_vb), range_label, FALSE, FALSE, 0);

    range_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(col2_vb), range_entry, FALSE, FALSE, 0);

    /*
     * OK, show the relation label and range stuff as it would be
     * with everything turned on, so it'll request as much space
     * as it'll ever need, so the dialog box and widgets start out
     * with the right sizes.
     *
     * XXX - this doesn't work.  It *doesn't* request as much space
     * as it'll ever need.
     *
     * XXX - FT_UINT8 doesn't support ranges, so even if it did work,
     * it wouldn't work right.
     *
     * XXX - this no longer affects the range stuff, as that's
     * controlled both by the type and by the relational operator
     * selected.
     */
    show_relations(relation_label, relation_list, FT_UINT8);

    value_vb = gtk_vbox_new(FALSE, 5);
    gtk_container_border_width(GTK_CONTAINER(value_vb), 5);
    gtk_container_add(GTK_CONTAINER(hb), value_vb);
    gtk_widget_show(value_vb);

    value_label = gtk_label_new("Value");
    gtk_misc_set_alignment(GTK_MISC(value_label), 0.0, 0.0);
    gtk_box_pack_start(GTK_BOX(value_vb), value_label, FALSE, FALSE, 0);
    gtk_widget_show(value_label);

    value_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(value_vb), value_entry, FALSE, FALSE, 0);
    gtk_widget_show(value_entry);

    value_list_scrolled_win = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(value_list_scrolled_win),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(value_vb), value_list_scrolled_win, TRUE,
                       TRUE, 0);
    gtk_widget_show(value_list_scrolled_win);

#if GTK_MAJOR_VERSION < 2
    value_list = gtk_list_new();
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(value_list_scrolled_win),
                                          value_list);
    SIGNAL_CONNECT(value_list, "select-child", value_list_sel_cb, value_entry);
    gtk_list_set_selection_mode(GTK_LIST(value_list), GTK_SELECTION_SINGLE);
    /* This remains hidden until an enumerated field is selected */

    /*
     * The value stuff may be hidden or shown depending on what
     * relation was selected; connect to the "select-child" signal
     * for the relation list, so we can make that happen.
     */
    SIGNAL_CONNECT(relation_list, "select-child", relation_list_sel_cb, NULL);
#else
    l_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
    value_list = tree_view_new(GTK_TREE_MODEL(l_store));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(value_list), FALSE);
    g_object_unref(G_OBJECT(l_store));
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("value", renderer,
                                                      "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(value_list), column);
    SIGNAL_CONNECT(gtk_tree_view_get_selection(GTK_TREE_VIEW(value_list)),
                   "changed", value_list_sel_cb, value_entry);

    /*
     * The value stuff may be hidden or shown depending on what
     * relation was selected; connect to the "changed" signal
     * for the relation list, so we can make that happen.
     */
    SIGNAL_CONNECT(gtk_tree_view_get_selection(GTK_TREE_VIEW(relation_list)),
                   "changed", relation_list_sel_cb, NULL);
    l_sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(value_list));
    gtk_tree_selection_set_mode(l_sel, GTK_SELECTION_SINGLE);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(value_list_scrolled_win),
                                          value_list);
    /* This remains hidden until an enumerated field is selected */
#endif

    /*
     * Put the items in the Tree; we don't want to do that until
     * we've constructed the value list and set the tree's
     * E_DFILTER_EXPR_VALUE_LIST_KEY data to point to it, and
     * constructed the "Accept" button and set the tree's
     * E_DFILTER_EXPR_ACCEPT_BT_KEY data to point to it, so that
     * when the list item is "helpfully" automatically selected for us
     * we're ready to cope with the selection signal.
     */

#if GTK_MAJOR_VERSION < 2
    /* a hash table seems excessive, but I don't see support for a
       sparse array in glib */
    proto_array = g_hash_table_new(g_direct_hash, g_direct_equal);
    for (i = proto_get_first_protocol(&cookie); i != -1;
         i = proto_get_next_protocol(&cookie)) {
        hfinfo = proto_registrar_get_nth(i);
        /* Create a node for the protocol, and remember it for
           later use. */
        name = proto_get_protocol_short_name(i); /* name, short_name or filter name ? */
        protocol_node = gtk_ctree_insert_node(GTK_CTREE(tree),
                                              NULL, NULL,
                                              &name, 5,
                                              NULL, NULL, NULL, NULL,
                                              FALSE, FALSE);
        gtk_ctree_node_set_row_data(GTK_CTREE(tree), protocol_node,
                                    hfinfo);
        g_hash_table_insert(proto_array, (gpointer)i, protocol_node);
    }

    len = proto_registrar_n();
    for (i = 0; i < len; i++) {
        char *strp, str[TAG_STRING_LEN+1];

        /*
         * If this field is a protocol, skip it - we already put
         * it in above.
         */
        if (proto_registrar_is_protocol(i))
            continue;

        hfinfo = proto_registrar_get_nth(i);

        /*
         * If this field isn't at the head of the list of
         * fields with this name, skip this field - all
         * fields with the same name are really just versions
         * of the same field stored in different bits, and
         * should have the same type/radix/value list, and
         * just differ in their bit masks.  (If a field isn't
         * a bitfield, but can be, say, 1 or 2 bytes long,
         * it can just be made FT_UINT16, meaning the
         * *maximum* length is 2 bytes, and be used
         * for all lengths.)
         */
        if (hfinfo->same_name_prev != NULL)
            continue;

        /* Create a node for the item, and put it
           under its parent protocol. */
        protocol_node = g_hash_table_lookup(proto_array,
                                            GINT_TO_POINTER(proto_registrar_get_parent(i)));
        if (hfinfo->blurb != NULL && hfinfo->blurb[0] != '\0') {
            snprintf(str, TAG_STRING_LEN, "%s - %s (%s)", hfinfo->abbrev,
                     hfinfo->name, hfinfo->blurb);
        } else {
            snprintf(str, TAG_STRING_LEN, "%s - %s", hfinfo->abbrev,
                     hfinfo->name);
        }
        str[TAG_STRING_LEN]=0;
        strp=str;
        item_node = gtk_ctree_insert_node(GTK_CTREE(tree),
                                          protocol_node, NULL,
                                          &strp, 5,
                                          NULL, NULL, NULL, NULL,
                                          TRUE, FALSE);
        gtk_ctree_node_set_row_data(GTK_CTREE(tree),
                                    item_node, hfinfo);
    }
    g_hash_table_destroy(proto_array);

#else /* GTK_MAJOR_VERSION < 2 */
{
    /* GTK2 code using two levels iterator to enumerate all protocol fields */

    GtkTreeIter iter, child_iter;
    void *cookie, *cookie2;
    gchar *name;

    for (i = proto_get_first_protocol(&cookie); i != -1;
         i = proto_get_next_protocol(&cookie)) {
	char *strp, str[TAG_STRING_LEN+1];

	hfinfo = proto_registrar_get_nth(i);
	name = proto_get_protocol_short_name(i); /* name, short_name or filter name ? */

	gtk_tree_store_append(store, &iter, NULL);
	gtk_tree_store_set(store, &iter, 0, name, 1, hfinfo, -1);

	for (hfinfo = proto_get_first_protocol_field(i, &cookie2); hfinfo != NULL;
	     hfinfo = proto_get_next_protocol_field(&cookie2)) {

		if (hfinfo->same_name_prev != NULL) /* ignore duplicate names */
			continue;

		if (hfinfo->blurb != NULL && hfinfo->blurb[0] != '\0') {
			snprintf(str, TAG_STRING_LEN, "%s - %s (%s)",
			    hfinfo->abbrev, hfinfo->name, hfinfo->blurb);
		} else {
			snprintf(str, TAG_STRING_LEN, "%s - %s", hfinfo->abbrev,
			    hfinfo->name);
		}
		str[TAG_STRING_LEN]=0;
		strp=str;
		gtk_tree_store_append(store, &child_iter, &iter);
		gtk_tree_store_set(store, &child_iter, 0, strp, 1, hfinfo, -1);
	}
    }
    g_object_unref(G_OBJECT(store));
}
#endif /* GTK_MAJOR_VERSION < 2 */

    gtk_widget_show_all(tree);

    list_bb = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(list_bb), GTK_BUTTONBOX_START);
    gtk_button_box_set_spacing(GTK_BUTTON_BOX(list_bb), 5);
    gtk_container_add(GTK_CONTAINER(main_vb), list_bb);
    gtk_widget_show(list_bb);

#if GTK_MAJOR_VERSION < 2
    accept_bt = gtk_button_new_with_label("Accept");
#else
    accept_bt = gtk_button_new_from_stock(GTK_STOCK_OK);
#endif
    gtk_widget_set_sensitive(accept_bt, FALSE);
    SIGNAL_CONNECT(accept_bt, "clicked", dfilter_expr_dlg_accept_cb, filter_te);
    GTK_WIDGET_SET_FLAGS(accept_bt, GTK_CAN_DEFAULT);
    alignment = gtk_alignment_new(0.0, 0.5, 1.0, 0.0);
    gtk_container_add(GTK_CONTAINER(alignment), accept_bt);
    gtk_box_pack_start(GTK_BOX(list_bb), alignment, TRUE, TRUE, 0);
    gtk_widget_grab_default(accept_bt);
    gtk_widget_show(accept_bt);
    gtk_widget_show(alignment);

#if GTK_MAJOR_VERSION < 2
    close_bt = gtk_button_new_with_label("Close");
#else
    close_bt = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
#endif
    SIGNAL_CONNECT(close_bt, "clicked", dfilter_expr_dlg_cancel_cb, window);
    alignment = gtk_alignment_new(0.0, 0.5, 1.0, 0.0);
    gtk_container_add(GTK_CONTAINER(alignment), close_bt);
    gtk_box_pack_start(GTK_BOX(list_bb), alignment, TRUE, TRUE, 0);
    gtk_widget_show(close_bt);
    gtk_widget_show(alignment);

    /* Catch the "activate" signal on the range and value text entries,
       so that if the user types Return there, we act as if the "Accept"
       button had been selected, as happens if Return is typed if some
       widget that *doesn't* handle the Return key has the input focus. */
    dlg_set_activate(range_entry, accept_bt);
    dlg_set_activate(value_entry, accept_bt);

    /*
     * Catch the "key_press_event" signal in the window, so that we can
     * catch the ESC key being pressed and act as if the "Close" button
     * had been selected.
     */
    dlg_set_cancel(window, close_bt);

    OBJECT_SET_DATA(window, E_DFILTER_EXPR_RELATION_LABEL_KEY, relation_label);
    OBJECT_SET_DATA(window, E_DFILTER_EXPR_RELATION_LIST_KEY, relation_list);
    OBJECT_SET_DATA(window, E_DFILTER_EXPR_RANGE_LABEL_KEY, range_label);
    OBJECT_SET_DATA(window, E_DFILTER_EXPR_RANGE_ENTRY_KEY, range_entry);
    OBJECT_SET_DATA(window, E_DFILTER_EXPR_VALUE_LABEL_KEY, value_label);
    OBJECT_SET_DATA(window, E_DFILTER_EXPR_VALUE_ENTRY_KEY, value_entry);
    OBJECT_SET_DATA(window, E_DFILTER_EXPR_VALUE_LIST_KEY, value_list);
    OBJECT_SET_DATA(window, E_DFILTER_EXPR_VALUE_LIST_SW_KEY,
                    value_list_scrolled_win);
    OBJECT_SET_DATA(window, E_DFILTER_EXPR_ACCEPT_BT_KEY, accept_bt);

#if GTK_MAJOR_VERSION < 2
    /*
     * OK, we've finally built the entire list, complete with the row data,
     * and attached to the top-level widget pointers to the relevant
     * subwidgets, so it's safe to put the list in browse mode.
     */
    gtk_clist_set_selection_mode (GTK_CLIST(tree),
                                  GTK_SELECTION_BROWSE);
#endif

    /*
     * Catch the "destroy" signal on our top-level window, and,
     * when it's destroyed, disconnect the signal we'll be
     * connecting below.
     */
    SIGNAL_CONNECT(window, "destroy", dfilter_expr_dlg_destroy_cb, filter_te);

    /*
     * Catch the "destroy" signal on the text entry widget to which
     * we're attached; if it's destroyed, we should destroy ourselves
     * as well.
     */
    SIGNAL_CONNECT(filter_te, "destroy", dfilter_expr_dlg_cancel_cb, window);

    gtk_widget_show(window);
}
