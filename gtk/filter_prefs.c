/* filter_prefs.c
 * Dialog boxes for filter editing
 * (This used to be a notebook page under "Preferences", hence the
 * "prefs" in the file name.)
 *
 * $Id: filter_prefs.c,v 1.20 2001/01/02 01:32:21 guy Exp $
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <gtk/gtk.h>

#include <stdlib.h>
#include <string.h>

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <ctype.h>
#ifdef HAVE_DIRECT_H
#include <direct.h>
#endif

#include <epan.h>
#include "gtk/main.h"
#include "filter_prefs.h"
#include "packet.h"
#include "file.h"
#include "util.h"
#include "dlg_utils.h"
#include "ui_util.h"
#include "prefs_dlg.h"
#include "dfilter_expr_dlg.h"

#define E_FILT_NAME_KEY "filter_name"
#define E_FILT_LBL_KEY  "filter_label"
#define E_FILT_CM_KEY   "in_cancel_mode"
#define E_FILTER_WIDGET_KEY "filter_widget"
#define E_FILT_DBLFUNC_KEY "filter_dblfunc"
#define E_FILT_DBLARG_KEY "filter_dblarg"

typedef struct _filter_def {
  char *name;
  char *strval;
} filter_def;

typedef struct _filter_cb_data {
  GList     *fl;
  GtkWidget *win;
} filter_cb_data;


static GtkWidget   *filter_l, *chg_bt, *copy_bt, *del_bt, *name_te, *filter_te;
static GList       *fl = NULL;

static void get_filter_list(void);
static GtkWidget *filter_dialog_new(GtkWidget *caller, GtkWidget *filter_te,
    gboolean wants_apply_button, gboolean wants_add_expression_button);
static void filter_dlg_dclick(GtkWidget *dummy, gpointer main_w_arg);
static void filter_dlg_ok_cb(GtkWidget *ok_bt, gpointer main_w_arg);
static void filter_dlg_apply_cb(GtkWidget *apply_bt, gpointer main_w_arg);
static void filter_dlg_save_cb(GtkWidget *save_bt, gpointer parent_w);
static void filter_dlg_cancel_cb(GtkWidget *cancel_bt, gpointer parent_w);
static void filter_dlg_destroy(GtkWidget *win, gpointer data);

static gint       filter_sel_list_button_cb(GtkWidget *, GdkEventButton *,
                           gpointer);
static void       filter_sel_list_cb(GtkWidget *, gpointer);
static void       filter_sel_new_cb(GtkWidget *, gpointer);
static void       filter_sel_chg_cb(GtkWidget *, gpointer);
static void       filter_sel_copy_cb(GtkWidget *, gpointer);
static void       filter_sel_del_cb(GtkWidget *, gpointer);
static void       filter_expr_cb(GtkWidget *, gpointer);
static void       filter_prefs_save(GtkWidget *);
static void       filter_prefs_delete(GtkWidget *);

#define	FILTER_LINE_SIZE	2048

static void
get_filter_list(void)
{
  filter_def *filt;
  FILE       *ff;
  gchar      *ff_path, *ff_name = PF_DIR "/filters", f_buf[FILTER_LINE_SIZE];
  gchar      *name_begin, *name_end, *filt_begin;
  int         len, line = 0;

  if (fl) return;
  
  /* To do: generalize this */
  ff_path = (gchar *) g_malloc(strlen(get_home_dir()) + strlen(ff_name) +  4);
  sprintf(ff_path, "%s/%s", get_home_dir(), ff_name);

  if ((ff = fopen(ff_path, "r")) == NULL) {
    g_free(ff_path);
    return;
  }

  while (fgets(f_buf, FILTER_LINE_SIZE, ff)) {
    line++;
    len = strlen(f_buf);
    if (f_buf[len - 1] == '\n') {
      len--;
      f_buf[len] = '\0';
    }
    name_begin = strchr(f_buf, '"');
    /* Empty line */
    if (name_begin == NULL)
      continue;
    name_end = strchr(name_begin + 1, '"');
    /* No terminating quote */
    if (name_end == NULL) {
      g_warning("Malformed filter in '%s' line %d.", ff_path, line);
      continue;
    }
    name_begin++;
    name_end[0] = '\0';
    filt_begin  = name_end + 1;
    while(isspace((guchar)filt_begin[0])) filt_begin++;
    /* No filter string */
    if (filt_begin[0] == '\0') {
      g_warning("Malformed filter in '%s' line %d.", ff_path, line);
      continue;
    }
    filt         = (filter_def *) g_malloc(sizeof(filter_def));
    filt->name   = g_strdup(name_begin);
    filt->strval = g_strdup(filt_begin);
    fl = g_list_append(fl, filt);
  }
  fclose(ff);
  g_free(ff_path);
}

/* XXX - we can have one global dialog box for editing, and a bunch
   of dialog boxes associated with browse buttons; we want the dialog
   boxes associated with browse buttons to at least let you save the
   current filter, so they have to allow editing; however, how do we
   arrange that if a change is made to the filter list, other dialog
   boxes get updated appropriately? */

/* Create a filter dialog for constructing a capture filter.

   This is to be used as a callback for a button next to a text entry box,
   which, when clicked, pops up this dialog to allow you to construct a
   display filter by browsing the list of saved filters (the dialog
   for constructing expressions assumes display filter syntax, not
   capture filter syntax).  The "OK" button sets the text entry box to the
   constructed filter and activates that text entry box (which should have
   no effect in the main capture dialog); this dialog is then dismissed.

   XXX - we probably want to have separate capture and display filter
   lists, but we don't yet have that, so the list of filters this
   shows is a list of all filters. */
void
capture_filter_construct_cb(GtkWidget *w, gpointer user_data)
{
	GtkWidget *caller = gtk_widget_get_toplevel(w);
	GtkWidget *filter_browse_w;
	GtkWidget *filter_te;

	/* Has a filter dialog box already been opened for that top-level
	   widget? */
	filter_browse_w = gtk_object_get_data(GTK_OBJECT(caller),
	    E_FILT_DIALOG_PTR_KEY);

	if (filter_browse_w != NULL) {
		/* Yes.  Just re-activate that dialog box. */
		reactivate_window(filter_browse_w);
		return;
	}

	/* No.  Get the text entry attached to the button. */
	filter_te = gtk_object_get_data(GTK_OBJECT(w), E_FILT_TE_PTR_KEY);

	/* Now create a new dialog, without either an "Apply" or "Add
	   Expression..." button. */
	filter_browse_w = filter_dialog_new(caller, filter_te, FALSE, FALSE);

	/* Set the E_FILT_CALLER_PTR_KEY for the new dialog to point to
	   our caller. */
	gtk_object_set_data(GTK_OBJECT(filter_browse_w), E_FILT_CALLER_PTR_KEY,
	    caller);

	/* Set the E_FILT_DIALOG_PTR_KEY for the caller to point to us */
	gtk_object_set_data(GTK_OBJECT(caller), E_FILT_DIALOG_PTR_KEY,
	    filter_browse_w);
}

/* Create a filter dialog for constructing a display filter.

   This is to be used as a callback for a button next to a text entry box,
   which, when clicked, pops up this dialog to allow you to construct a
   display filter by browsing the list of saved filters and/or by adding
   test expressions constructed with another dialog.  The "OK" button
   sets the text entry box to the constructed filter and activates that
   text entry box, causing the filter to be used; this dialog is then
   dismissed.

   If "wants_apply_button" is non-null, we add an "Apply" button that
   acts like "OK" but doesn't dismiss this dialog.

   XXX - we probably want to have separate capture and display filter
   lists, but we don't yet have that, so the list of filters this
   shows is a list of all filters. */
void
display_filter_construct_cb(GtkWidget *w, gpointer wants_apply_button)
{
	GtkWidget *caller = gtk_widget_get_toplevel(w);
	GtkWidget *filter_browse_w;
	GtkWidget *filter_te;

	/* Has a filter dialog box already been opened for that top-level
	   widget? */
	filter_browse_w = gtk_object_get_data(GTK_OBJECT(caller),
	    E_FILT_DIALOG_PTR_KEY);

	if (filter_browse_w != NULL) {
		/* Yes.  Just re-activate that dialog box. */
		reactivate_window(filter_browse_w);
		return;
	}

	/* No.  Get the text entry attached to the button. */
	filter_te = gtk_object_get_data(GTK_OBJECT(w), E_FILT_TE_PTR_KEY);

	/* Now create a new dialog, possibly with an "Apply" button, and
	   definitely with an "Add Expression..." button. */
	filter_browse_w = filter_dialog_new(caller, filter_te,
	    (wants_apply_button != NULL), TRUE);

	/* Set the E_FILT_CALLER_PTR_KEY for the new dialog to point to
	   our caller. */
	gtk_object_set_data(GTK_OBJECT(filter_browse_w), E_FILT_CALLER_PTR_KEY,
	    caller);

	/* Set the E_FILT_DIALOG_PTR_KEY for the caller to point to us */
	gtk_object_set_data(GTK_OBJECT(caller), E_FILT_DIALOG_PTR_KEY,
	    filter_browse_w);
}

static GtkWidget *global_filter_w;

/* Create a filter dialog for editing; this is to be used as a callback
   for menu items, toolbars, etc.. */
void
filter_dialog_cb(GtkWidget *w)
{
	/* Has a filter dialog box already been opened for editing? */
	if (global_filter_w != NULL) {
		/* Yes.  Just reactivate it. */
		reactivate_window(global_filter_w);
		return;
	}

	/*
	 * No.  Create one; we didn't pop this up as a result of pressing
	 * a button next to some text entry field, so don't associate it
	 * with a text entry field.
	 */
	global_filter_w = filter_dialog_new(NULL, NULL, FALSE, TRUE);
}

static GtkWidget *
filter_dialog_new(GtkWidget *caller, GtkWidget *parent_filter_te,
    gboolean wants_apply_button, gboolean wants_add_expression_button)
{
	GtkWidget	*main_w,		/* main window */
			*main_vb,		/* main container */
			*bbox,			/* button container */
			*ok_bt,			/* "OK" button */
			*apply_bt,		/* "Apply" button */
			*save_bt,		/* "Save" button */
			*cancel_bt;		/* "Cancel" button */ 
	GtkWidget	*filter_pg = NULL;	/* filter settings box */
	GtkWidget	*top_hb,
			*list_bb,
			*new_bt,
			*filter_sc,
			*nl_item,
			*nl_lb,
			*middle_hb,
			*name_lb,
			*bottom_hb,
			*filter_lb,
			*add_expression_bt;
	GtkWidget	*l_select = NULL;
	GList		*flp = NULL;
	filter_def	*filt;
	gchar		*filter_te_str = NULL;

	main_w = dlg_window_new("Ethereal: Filters");

	/* Call a handler when we're destroyed, so we can inform
	   our caller, if any, that we've been destroyed. */
	gtk_signal_connect(GTK_OBJECT(main_w), "destroy",
	    GTK_SIGNAL_FUNC(filter_dlg_destroy), NULL);

	main_vb = gtk_vbox_new(FALSE, 5);
	gtk_container_border_width(GTK_CONTAINER(main_vb), 5);
	gtk_container_add(GTK_CONTAINER(main_w), main_vb);
	gtk_widget_show(main_vb);

	/* Make sure everything is set up */  
	get_filter_list();
	if (parent_filter_te)
		filter_te_str = gtk_entry_get_text(GTK_ENTRY(parent_filter_te));

	/* Container for each row of widgets */
	filter_pg = gtk_vbox_new(FALSE, 5);
	gtk_container_border_width(GTK_CONTAINER(filter_pg), 5);
	gtk_widget_show(filter_pg);
	gtk_object_set_data(GTK_OBJECT(filter_pg), E_FILT_CM_KEY, (gpointer)FALSE);

	/* Top row: Filter list and buttons */
	top_hb = gtk_hbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(filter_pg), top_hb);
	gtk_widget_show(top_hb);

	list_bb = gtk_vbutton_box_new();
	gtk_button_box_set_layout (GTK_BUTTON_BOX (list_bb), GTK_BUTTONBOX_START);
	gtk_container_add(GTK_CONTAINER(top_hb), list_bb);
	gtk_widget_show(list_bb);

	new_bt = gtk_button_new_with_label ("New");
	gtk_signal_connect(GTK_OBJECT(new_bt), "clicked",
	    GTK_SIGNAL_FUNC(filter_sel_new_cb), NULL);
	gtk_container_add(GTK_CONTAINER(list_bb), new_bt);
	gtk_widget_show(new_bt);

	chg_bt = gtk_button_new_with_label ("Change");
	gtk_widget_set_sensitive(chg_bt, FALSE);
	gtk_signal_connect(GTK_OBJECT(chg_bt), "clicked",
	    GTK_SIGNAL_FUNC(filter_sel_chg_cb), NULL);
	gtk_container_add(GTK_CONTAINER(list_bb), chg_bt);
	gtk_widget_show(chg_bt);

	copy_bt = gtk_button_new_with_label ("Copy");
	gtk_widget_set_sensitive(copy_bt, FALSE);
	gtk_signal_connect(GTK_OBJECT(copy_bt), "clicked",
	    GTK_SIGNAL_FUNC(filter_sel_copy_cb), NULL);
	gtk_container_add(GTK_CONTAINER(list_bb), copy_bt);
	gtk_widget_show(copy_bt);

	del_bt = gtk_button_new_with_label ("Delete");
	gtk_widget_set_sensitive(del_bt, FALSE);
	gtk_signal_connect(GTK_OBJECT(del_bt), "clicked",
	    GTK_SIGNAL_FUNC(filter_sel_del_cb), NULL);
	gtk_container_add(GTK_CONTAINER(list_bb), del_bt);
	gtk_widget_show(del_bt);

	if (wants_add_expression_button) {
		/* Create the "Add Expression..." button, to pop up a dialog
		   for constructing filter comparison expressions. */
		add_expression_bt = gtk_button_new_with_label("Add Expression...");
		gtk_signal_connect(GTK_OBJECT(add_expression_bt), "clicked",
		    GTK_SIGNAL_FUNC(filter_expr_cb), main_w);
		gtk_container_add(GTK_CONTAINER(list_bb), add_expression_bt);
		gtk_widget_show(add_expression_bt);
	}

	filter_sc = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(filter_sc),
	    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_set_usize(filter_sc, 250, 150);
	gtk_container_add(GTK_CONTAINER(top_hb), filter_sc);
	gtk_widget_show(filter_sc);

	filter_l = gtk_list_new();
	gtk_list_set_selection_mode(GTK_LIST(filter_l), GTK_SELECTION_SINGLE);
	gtk_signal_connect(GTK_OBJECT(filter_l), "selection_changed",
	    GTK_SIGNAL_FUNC(filter_sel_list_cb), filter_pg);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(filter_sc),
	    filter_l);
	gtk_widget_show(filter_l);

	gtk_object_set_data(GTK_OBJECT(filter_l), E_FILT_DBLFUNC_KEY, filter_dlg_dclick);
	gtk_object_set_data(GTK_OBJECT(filter_l), E_FILT_DBLARG_KEY, main_w);

	flp = g_list_first(fl);
	while (flp) {
		filt    = (filter_def *) flp->data;
		nl_lb   = gtk_label_new(filt->name);
		nl_item = gtk_list_item_new();

		gtk_signal_connect(GTK_OBJECT(nl_item), "button_press_event",
		    GTK_SIGNAL_FUNC(filter_sel_list_button_cb), filter_l);

		gtk_misc_set_alignment (GTK_MISC (nl_lb), 0.0, 0.5);
		gtk_container_add(GTK_CONTAINER(nl_item), nl_lb);
		gtk_widget_show(nl_lb);
		gtk_container_add(GTK_CONTAINER(filter_l), nl_item);
		gtk_widget_show(nl_item);
		gtk_object_set_data(GTK_OBJECT(nl_item), E_FILT_LBL_KEY, nl_lb);
		gtk_object_set_data(GTK_OBJECT(nl_item), E_FILT_NAME_KEY, flp);

		if (filter_te_str && filt->strval) {
			if (strcmp(filter_te_str, filt->strval) == 0)
				l_select = nl_item;
		}

		flp = flp->next;
	}

	/* Middle row: Filter name entry */
	middle_hb = gtk_hbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(filter_pg), middle_hb);
	gtk_widget_show(middle_hb);
  
	name_lb = gtk_label_new("Filter name:");
	gtk_box_pack_start(GTK_BOX(middle_hb), name_lb, FALSE, FALSE, 3);
	gtk_widget_show(name_lb);
  
	name_te = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(middle_hb), name_te, TRUE, TRUE, 3);
	gtk_widget_show(name_te);

	/* Bottom row: Filter text entry */
	bottom_hb = gtk_hbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(filter_pg), bottom_hb);
	gtk_widget_show(bottom_hb);
  
	filter_lb = gtk_label_new("Filter string:");
	gtk_box_pack_start(GTK_BOX(bottom_hb), filter_lb, FALSE, FALSE, 3);
	gtk_widget_show(filter_lb);
  
	filter_te = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(bottom_hb), filter_te, TRUE, TRUE, 3);
	gtk_widget_show(filter_te);

	if (l_select) {
		gtk_list_select_child(GTK_LIST(filter_l), l_select);
	} else if (filter_te_str && filter_te_str[0]) {
		gtk_entry_set_text(GTK_ENTRY(name_te), "New filter");
		gtk_entry_set_text(GTK_ENTRY(filter_te), filter_te_str);
	}

	gtk_box_pack_start(GTK_BOX(main_vb), filter_pg, TRUE, TRUE, 0);
	gtk_object_set_data(GTK_OBJECT(filter_pg), E_FILT_TE_PTR_KEY, parent_filter_te);
	gtk_object_set_data(GTK_OBJECT(main_w), E_FILTER_WIDGET_KEY, filter_pg);

	bbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);
	gtk_container_add(GTK_CONTAINER(main_vb), bbox);
	gtk_widget_show(bbox);

	ok_bt = gtk_button_new_with_label ("OK");
	gtk_signal_connect(GTK_OBJECT(ok_bt), "clicked",
		GTK_SIGNAL_FUNC(filter_dlg_ok_cb), main_w);
	GTK_WIDGET_SET_FLAGS(ok_bt, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(bbox), ok_bt, TRUE, TRUE, 0);
	gtk_object_set_data(GTK_OBJECT(main_w), E_FILT_TE_KEY, filter_te);
	gtk_widget_grab_default(ok_bt);
	gtk_widget_show(ok_bt);

	if (wants_apply_button) {
		apply_bt = gtk_button_new_with_label ("Apply");
		gtk_signal_connect(GTK_OBJECT(apply_bt), "clicked",
		    GTK_SIGNAL_FUNC(filter_dlg_apply_cb), main_w);
		GTK_WIDGET_SET_FLAGS(apply_bt, GTK_CAN_DEFAULT);
		gtk_box_pack_start(GTK_BOX(bbox), apply_bt, TRUE, TRUE, 0);
		gtk_widget_show(apply_bt);
	}

	save_bt = gtk_button_new_with_label ("Save");
	gtk_signal_connect(GTK_OBJECT(save_bt), "clicked",
		GTK_SIGNAL_FUNC(filter_dlg_save_cb), GTK_OBJECT(main_w));
	GTK_WIDGET_SET_FLAGS(save_bt, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(bbox), save_bt, TRUE, TRUE, 0);
	gtk_widget_show(save_bt);

	cancel_bt = gtk_button_new_with_label ("Cancel");
	gtk_signal_connect(GTK_OBJECT(cancel_bt), "clicked",
		GTK_SIGNAL_FUNC(filter_dlg_cancel_cb), GTK_OBJECT(main_w));
	GTK_WIDGET_SET_FLAGS(cancel_bt, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(bbox), cancel_bt, TRUE, TRUE, 0);
	gtk_widget_show(cancel_bt);

	dlg_set_cancel(main_w, cancel_bt);

	gtk_widget_show(main_w);

	return main_w;
}

static void
filter_dlg_dclick(GtkWidget *dummy, gpointer main_w_arg)
{
	GtkWidget  *main_w = GTK_WIDGET(main_w_arg);
	GtkWidget  *w = gtk_object_get_data(GTK_OBJECT(main_w), E_FILTER_WIDGET_KEY);
	GList      *flp, *sl;
	GtkObject  *l_item;
	filter_def *filt;
	GtkWidget  *mw_filt = gtk_object_get_data(GTK_OBJECT(w), E_FILT_TE_PTR_KEY);

	if (mw_filt != NULL) {
		/*
		 * We have a text entry widget associated with this dialog
		 * box; is one of the filters in the list selected?
		 */
		sl = GTK_LIST(filter_l)->selection;
		if (sl != NULL) {
			/*
			 * Yes.  Put it in the text entry widget, and then
			 * activate that widget to cause the filter we
			 * put there to be applied.
			 */
			l_item = GTK_OBJECT(sl->data);
			flp    = (GList *) gtk_object_get_data(l_item, E_FILT_NAME_KEY);
			if (flp) {
				filt = (filter_def *) flp->data;
				gtk_entry_set_text(GTK_ENTRY(mw_filt),
				    filt->strval);
				gtk_signal_emit_by_name(GTK_OBJECT(mw_filt),
				    "activate");
			}
		}
	}

	filter_prefs_delete(w);

	gtk_widget_destroy(main_w);
}

static void
filter_dlg_ok_cb(GtkWidget *ok_bt, gpointer main_w_arg)
{
	GtkWidget  *main_w = GTK_WIDGET(main_w_arg);
	GtkWidget  *w = gtk_object_get_data(GTK_OBJECT(main_w), E_FILTER_WIDGET_KEY);

	/*
	 * Apply the filter.
	 */
	filter_dlg_apply_cb(NULL, main_w_arg);

	/*
	 * Now dismiss the dialog box.
	 */
	filter_prefs_delete(w);
	gtk_widget_destroy(main_w);
}

static void
filter_dlg_apply_cb(GtkWidget *apply_bt, gpointer main_w_arg)
{
	GtkWidget  *main_w = GTK_WIDGET(main_w_arg);
	GtkWidget  *w = gtk_object_get_data(GTK_OBJECT(main_w), E_FILTER_WIDGET_KEY);
	GtkWidget  *mw_filt = gtk_object_get_data(GTK_OBJECT(w), E_FILT_TE_PTR_KEY);
	GtkWidget  *filter_te;
	gchar      *filter_string;
	
	if (mw_filt != NULL) {
		/*
		 * We have a text entry widget associated with this dialog
		 * box; put the filter in our text entry widget into that
		 * text entry widget, and then activate that widget to
		 * cause the filter we put there to be applied.
		 */
		filter_te = gtk_object_get_data(GTK_OBJECT(main_w),
		    E_FILT_TE_KEY);
		filter_string = gtk_entry_get_text(GTK_ENTRY(filter_te));
		gtk_entry_set_text(GTK_ENTRY(mw_filt), filter_string);
		gtk_signal_emit_by_name(GTK_OBJECT(mw_filt), "activate");
	}
}

static void
filter_dlg_save_cb(GtkWidget *save_bt, gpointer parent_w)
{
	filter_prefs_save(gtk_object_get_data(GTK_OBJECT(parent_w), E_FILTER_WIDGET_KEY));
}

static void
filter_dlg_cancel_cb(GtkWidget *cancel_bt, gpointer parent_w)
{
	filter_prefs_delete(gtk_object_get_data(GTK_OBJECT(parent_w),  E_FILTER_WIDGET_KEY));
	gtk_widget_destroy(GTK_WIDGET(parent_w));
}

static void
filter_dlg_destroy(GtkWidget *win, gpointer data)
{
	GtkWidget *caller;

	/* Get the widget that requested that we be popped up, if any.
	   (It should arrange to destroy us if it's destroyed, so
	   that we don't get a pointer to a non-existent window here.) */
	caller = gtk_object_get_data(GTK_OBJECT(win), E_FILT_CALLER_PTR_KEY);

	if (caller != NULL) {
		/* Tell it we no longer exist. */
		gtk_object_set_data(GTK_OBJECT(caller), E_FILT_DIALOG_PTR_KEY,
		    NULL);
	} else {
		/* This is an editing dialog popped up from, for example,
		   a menu item; note that we no longer have one. */
		g_assert(win == global_filter_w);
		global_filter_w = NULL;
	}

	/* Now nuke this window. */
	gtk_grab_remove(GTK_WIDGET(win));
	gtk_widget_destroy(GTK_WIDGET(win));
}

static gint
filter_sel_list_button_cb (GtkWidget *widget, GdkEventButton *event,
                           gpointer func_data)
{
    GtkWidget *parent = func_data;
    GtkSignalFunc func;
    gpointer func_arg;

    if (GTK_IS_LIST_ITEM(widget) && event->type == GDK_2BUTTON_PRESS) {
        func = gtk_object_get_data(GTK_OBJECT(parent), E_FILT_DBLFUNC_KEY);
        func_arg = gtk_object_get_data(GTK_OBJECT(parent), E_FILT_DBLARG_KEY);

        if (func)
            (*func)(NULL, func_arg);
    }

    return FALSE;
}

static void
filter_sel_list_cb(GtkWidget *l, gpointer data) {
  filter_def *filt;
  gchar      *name = "", *strval = "";
  GList      *sl, *flp;
  GtkObject  *l_item;
  gint        sensitivity = FALSE;

  if (l)
	  sl = GTK_LIST(l)->selection;
  else
	  sl = NULL;
          
  if (sl) {  /* Something was selected */
    l_item = GTK_OBJECT(sl->data);
    flp    = (GList *) gtk_object_get_data(l_item, E_FILT_NAME_KEY);
    if (flp) {
      filt   = (filter_def *) flp->data;
      name   = filt->name;
      strval = filt->strval;
      sensitivity = TRUE;
    }
  }

  /* Did you know that this function is called when the window is destroyed? */
  /* Funny, that. */
  if (!gtk_object_get_data(GTK_OBJECT(data), E_FILT_CM_KEY)) {
    gtk_entry_set_text(GTK_ENTRY(name_te), name);
    gtk_entry_set_text(GTK_ENTRY(filter_te), strval);
    gtk_widget_set_sensitive(chg_bt, sensitivity);
    gtk_widget_set_sensitive(copy_bt, sensitivity);
    gtk_widget_set_sensitive(del_bt, sensitivity);
  }
}

/* To do: add input checking to each of these callbacks */
 
static void
filter_sel_new_cb(GtkWidget *w, gpointer data) {
  filter_def *filt;
  gchar      *name, *strval;
  GtkWidget  *nl_item, *nl_lb;
  
  name   = gtk_entry_get_text(GTK_ENTRY(name_te));
  strval = gtk_entry_get_text(GTK_ENTRY(filter_te));
  
  if (strlen(name) > 0 && strlen(strval) > 0) {
    filt         = (filter_def *) g_malloc(sizeof(filter_def));
    filt->name   = g_strdup(name);
    filt->strval = g_strdup(strval);
    fl           = g_list_append(fl, filt);
    nl_lb        = gtk_label_new(filt->name);
    nl_item      = gtk_list_item_new();
    gtk_misc_set_alignment (GTK_MISC (nl_lb), 0.0, 0.5);
    gtk_container_add(GTK_CONTAINER(nl_item), nl_lb);
    gtk_widget_show(nl_lb);
    gtk_container_add(GTK_CONTAINER(filter_l), nl_item);
    gtk_widget_show(nl_item);
    gtk_object_set_data(GTK_OBJECT(nl_item), E_FILT_LBL_KEY, nl_lb);
    gtk_object_set_data(GTK_OBJECT(nl_item), E_FILT_NAME_KEY, g_list_last(fl));
    gtk_list_select_child(GTK_LIST(filter_l), nl_item);
  }
}

static void
filter_sel_chg_cb(GtkWidget *w, gpointer data) {
  filter_def *filt;
  gchar      *name = "", *strval = "";
  GList      *sl, *flp;
  GtkObject  *l_item;
  GtkLabel   *nl_lb;

  sl     = GTK_LIST(filter_l)->selection;
  name   = gtk_entry_get_text(GTK_ENTRY(name_te));
  strval = gtk_entry_get_text(GTK_ENTRY(filter_te));

  if (sl) {  /* Something was selected */
    l_item = GTK_OBJECT(sl->data);
    flp    = (GList *) gtk_object_get_data(l_item, E_FILT_NAME_KEY);
    nl_lb  = (GtkLabel *) gtk_object_get_data(l_item, E_FILT_LBL_KEY);
    if (flp && nl_lb) {
      filt = (filter_def *) flp->data;
      
      if (strlen(name) > 0 && strlen(strval) > 0 && filt) {
        g_free(filt->name);
        g_free(filt->strval);
        filt->name   = g_strdup(name);
        filt->strval = g_strdup(strval);
        gtk_label_set(nl_lb, filt->name);
      }
    }
  }
}

static void
filter_sel_copy_cb(GtkWidget *w, gpointer data) {
  GList      *sl, *flp;
  filter_def *filt, *nfilt;
  gchar      *prefix = "Copy of ";
  GtkObject  *l_item;
  GtkWidget  *nl_item, *nl_lb;
  
  sl     = GTK_LIST(filter_l)->selection;
  if (sl) {  /* Something was selected */
    l_item = GTK_OBJECT(sl->data);
    flp    = (GList *) gtk_object_get_data(l_item, E_FILT_NAME_KEY);
    if (flp) {
      filt          = (filter_def *) flp->data;
      nfilt         = (filter_def *) g_malloc(sizeof(filter_def));
      nfilt->name   = g_malloc(strlen(prefix) + strlen(filt->name) + 1);
      sprintf(nfilt->name, "%s%s", prefix, filt->name);
      nfilt->strval = g_strdup(filt->strval);
      fl            = g_list_append(fl, nfilt);
      nl_lb         = gtk_label_new(nfilt->name);
      nl_item       = gtk_list_item_new();
      gtk_misc_set_alignment (GTK_MISC (nl_lb), 0.0, 0.5);
      gtk_container_add(GTK_CONTAINER(nl_item), nl_lb);
      gtk_widget_show(nl_lb);
      gtk_container_add(GTK_CONTAINER(filter_l), nl_item);
      gtk_widget_show(nl_item);
      gtk_object_set_data(GTK_OBJECT(nl_item), E_FILT_LBL_KEY, nl_lb);
      gtk_object_set_data(GTK_OBJECT(nl_item), E_FILT_NAME_KEY, g_list_last(fl));
      gtk_list_select_child(GTK_LIST(filter_l), nl_item);
    }
  }
}

static void
filter_sel_del_cb(GtkWidget *w, gpointer data) {
  GList      *sl, *flp;
  filter_def *filt;
  GtkObject  *l_item;
  gint        pos;
  
  sl = GTK_LIST(filter_l)->selection;
  if (sl) {  /* Something was selected */
    l_item = GTK_OBJECT(sl->data);
    pos    = gtk_list_child_position(GTK_LIST(filter_l),
      GTK_WIDGET(l_item));
    flp    = (GList *) gtk_object_get_data(l_item, E_FILT_NAME_KEY);
    if (flp) {
      filt = (filter_def *) flp->data;
      g_free(filt->name);
      g_free(filt->strval);
      g_free(filt);
      fl = g_list_remove_link(fl, flp);
      gtk_list_clear_items(GTK_LIST(filter_l), pos, pos + 1);
    } 
  }
}

static void
filter_expr_cb(GtkWidget *w, gpointer main_w_arg)
{
	GtkWidget  *main_w = GTK_WIDGET(main_w_arg);
	GtkWidget  *filter_te;

	filter_te = gtk_object_get_data(GTK_OBJECT(main_w), E_FILT_TE_KEY);
	dfilter_expr_dlg_new(filter_te);
}

static void
filter_prefs_save(GtkWidget *w) {
  GList       *flp;
  filter_def  *filt;
  gchar       *ff_path, *ff_dir = PF_DIR, *ff_name = "filters";
  FILE        *ff;
  struct stat  s_buf;
  
  ff_path = (gchar *) g_malloc(strlen(get_home_dir()) + strlen(ff_dir) +  
    strlen(ff_name) + 4);
  sprintf(ff_path, "%s/%s", get_home_dir(), ff_dir);

  if (stat(ff_path, &s_buf) != 0)
#ifdef WIN32
    mkdir(ff_path);
#else
    mkdir(ff_path, 0755);
#endif
    
  sprintf(ff_path, "%s/%s/%s", get_home_dir(), ff_dir, ff_name);

  if ((ff = fopen(ff_path, "w")) != NULL) {
    flp = g_list_first(fl);
    while (flp) {
      filt = (filter_def *) flp->data;
      fprintf(ff, "\"%s\" %s\n", filt->name, filt->strval);
      flp = flp->next;
    }
    fclose(ff);
  }

  g_free(ff_path);
}

static void
filter_prefs_delete(GtkWidget *w) {
 
  /* Let the list cb know we're about to destroy the widget tree, so it */
  /* doesn't operate on widgets that don't exist. */  
  gtk_object_set_data(GTK_OBJECT(w), E_FILT_CM_KEY, (gpointer)TRUE);
  gtk_widget_destroy(GTK_WIDGET(w));
} 
