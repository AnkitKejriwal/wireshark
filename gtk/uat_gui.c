/*
 *  uat_gui.c
 *
 *  $Id$
 *
 *  User Accessible Tables GUI
 *  Mantain an array of user accessible data strucures
 *  
 * (c) 2007, Luis E. Garcia Ontanon <luis.ontanon@gmail.com>
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 2001 Gerald Combs
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
 * TO DO:
 * + improvements
 *   - field value check (red/green editbox)
 *   - tooltips (add field descriptions)
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <gtk/gtk.h>
#include <epan/dfilter/dfilter-macro.h>
#include <epan/emem.h>
#include <epan/report_err.h>
#include <gdk/gdkkeysyms.h>
#include "gtkglobals.h"
#include "gui_utils.h"
#include "dlg_utils.h"
#include "help_dlg.h"
#include "menu.h"
#include "compat_macros.h"
#include <epan/proto.h>
#include <epan/packet.h>
#include "../stat_menu.h"
#include "gui_stat_menu.h"

#include <epan/uat-int.h>
#include <epan/value_string.h>
#include "uat_gui.h"

#if GTK_MAJOR_VERSION >= 2
# undef GTK_MAJOR_VERSION
# define GTK_MAJOR_VERSION 1
# define BUTTON_SIZE_X -1
# define BUTTON_SIZE_Y -1
#else
# define BUTTON_SIZE_X 50
# define BUTTON_SIZE_Y 20
#endif

struct _uat_rep_t {
	GtkWidget* window;
	GtkWidget* vbox;
    GtkWidget* scrolledwindow;
    GtkWidget* clist;
	GtkWidget* bbox;
	GtkWidget* bt_close;
	GtkWidget* bt_new;
	GtkWidget* bt_edit;
	GtkWidget* bt_delete;
	GtkWidget* bt_up;
	GtkWidget* bt_down;
	GtkWidget* bt_save;
	GtkWidget* unsaved_window;
	
	gint selected;
	gboolean dont_save;
#if GTK_MAJOR_VERSION >= 2
	GtkTreeSelection  *selection;
#endif
	
};

struct _str_pair {
	const char* ptr;
	unsigned len;
};

struct _uat_dlg_data {
    GtkWidget* win;
    GPtrArray* entries;
	uat_t* uat;
	void* rec;
	gboolean is_new;
	gint row;
	GPtrArray* tobe_freed;
};


static gboolean unsaved_dialog(GtkWindow *w, GdkEvent* e, gpointer u);
static gboolean uat_window_delete_event_cb(GtkWindow *w, GdkEvent* e, gpointer u);
static gboolean uat_close_unchanged_cb(GtkWidget *w, void* u);
static gboolean uat_close_changed_cb(GtkWidget *w, void* u);

static void set_buttons(uat_t* uat, gint row) {
	
	if (row > 0) {
		gtk_widget_set_sensitive (uat->rep->bt_up, TRUE);
	} else {
		gtk_widget_set_sensitive (uat->rep->bt_up, FALSE);
	}
	
	if (row < (gint)(*uat->nrows_p - 1) && row >= 0) {
		gtk_widget_set_sensitive (uat->rep->bt_down, TRUE);
	} else {
		gtk_widget_set_sensitive (uat->rep->bt_down, FALSE);
	}
	
	if (uat->changed) {
		gtk_widget_set_sensitive (uat->rep->bt_save, TRUE);
		
		SIGNAL_DISCONNECT_BY_FUNC(uat->rep->bt_close, uat_close_unchanged_cb, uat);
		SIGNAL_CONNECT(uat->rep->bt_close, "clicked", uat_close_changed_cb, uat);
		
		SIGNAL_DISCONNECT_BY_FUNC(uat->rep->window, uat_window_delete_event_cb, uat);
		SIGNAL_CONNECT(uat->rep->window, "delete_event", unsaved_dialog, uat);
		SIGNAL_CONNECT(uat->rep->window, "destroy", unsaved_dialog, uat);
	} else {
		gtk_widget_set_sensitive (uat->rep->bt_save, FALSE);
		
		SIGNAL_DISCONNECT_BY_FUNC(uat->rep->bt_close, uat_close_changed_cb, uat);
		SIGNAL_CONNECT(uat->rep->bt_close, "clicked", uat_close_unchanged_cb, uat);
		
		SIGNAL_DISCONNECT_BY_FUNC(uat->rep->window, unsaved_dialog, uat);
		SIGNAL_CONNECT(GTK_WINDOW(uat->rep->window), "delete_event", uat_window_delete_event_cb, uat);
		SIGNAL_CONNECT(GTK_WINDOW(uat->rep->window), "destroy", uat_window_delete_event_cb, uat);
	}
}

static char* fld_tostr(void* rec, uat_field_t* f) {
	guint len;
	char* ptr;
	char* out;
	
	f->cb.tostr(rec,&ptr,&len,f->cbdata.tostr,f->fld_data);
		
	switch(f->mode) {
		case PT_TXTMOD_STRING:
		case PT_TXTMOD_ENUM:
			out = ep_strndup(ptr,len);
			break;
		case PT_TXTMOD_HEXBYTES: {
			GString* s = g_string_sized_new( len*2 + 1 );
			guint i;
			
			for (i=0; i<len;i++) g_string_sprintfa(s,"%.2X",((guint8*)ptr)[i]);
			
			out = ep_strdup_printf(s->str);
			
			g_string_free(s,TRUE);
			break;
		} 
		default:
			g_assert_not_reached();
			break;
	}
	
	return out;
}



static void append_row(uat_t* uat, guint idx) {
	GPtrArray* a = g_ptr_array_new();
	void* rec = UAT_INDEX_PTR(uat,idx);
	uat_field_t* f = uat->fields;
	guint rownum;
	guint colnum;
	
	gtk_clist_freeze(GTK_CLIST(uat->rep->clist));

	for ( colnum = 0; colnum < uat->ncols; colnum++ )
		g_ptr_array_add(a,fld_tostr(rec,&(f[colnum])));
	
	rownum = gtk_clist_append(GTK_CLIST(uat->rep->clist), (gchar**)a->pdata);
	gtk_clist_set_row_data(GTK_CLIST(uat->rep->clist), rownum, rec);

	gtk_clist_thaw(GTK_CLIST(uat->rep->clist));

	g_ptr_array_free(a,TRUE);
}

static void reset_row(uat_t* uat, guint idx) {
	void* rec = UAT_INDEX_PTR(uat,idx);
	uat_field_t* f = uat->fields;
	guint colnum;
	
	gtk_clist_freeze(GTK_CLIST(uat->rep->clist));
	
	for ( colnum = 0; colnum < uat->ncols; colnum++ ) {
		gtk_clist_set_text(GTK_CLIST(uat->rep->clist), idx, colnum, fld_tostr(rec,&(f[colnum])));
	}
	
	gtk_clist_thaw(GTK_CLIST(uat->rep->clist));

}

static guint8* unhexbytes(const char* si, guint len, guint* len_p, char** err) {
	guint8* buf;
	guint8* p;
	const guint8* s = (void*)si;
	unsigned i;
	
	if (len % 2) {
		*err = "Uneven number of chars hex string";
		return NULL;
	}
	
	buf = ep_alloc(len/2+1);
	p = buf;
	
	for (i = 0; i<len ; i += 2) {
		guint8 lo = s[i+1];
		guint8 hi = s[i];
		
		if (hi >= '0' && hi <= '9') {
			hi -= '0';
		} else if (hi >= 'a' && hi <= 'f') {
			hi -=  'a';
			hi += 0xa;
		} else if (hi >= 'A' && hi <= 'F') {
			hi -=  'A';
			hi += 0xa;
		} else {
			goto on_error;
		}
		
		if (lo >= '0' && lo <= '9') {
			lo -= '0';
		} else if (lo >= 'a' && lo <= 'f') {
			lo -=  'a';
			lo += 0xa;
		} else if (lo >= 'A' && lo <= 'F') {
			lo -=  'A';
			lo += 0xa;
		} else {
			goto on_error;
		}
		
		*(p++) = (hi*0x10) + lo;
	}
	
	len /= 2;
	
	if (len_p) *len_p = len;

	buf[len] = '\0';
	
	*err = NULL;
	return buf;
	
on_error:
	*err = "Error parsing hex string";
	return NULL;
}


static gboolean uat_dlg_cb(GtkWidget *win _U_, gpointer user_data) {
	struct _uat_dlg_data* dd = user_data;
	guint ncols = dd->uat->ncols;
	uat_field_t* f = dd->uat->fields;
	char* err = NULL;
	guint colnum;

	for ( colnum = 0; colnum < ncols; colnum++ ) {
		void* e = g_ptr_array_index(dd->entries,colnum);
		const char* text;
		unsigned len = 0;
		
		switch(f[colnum].mode) {
			case PT_TXTMOD_STRING:
				text = gtk_entry_get_text(GTK_ENTRY(e));
				len = strlen(text);
				break;
			case PT_TXTMOD_HEXBYTES: {
				text = gtk_entry_get_text(GTK_ENTRY(e));
				
				text = (void*) unhexbytes(text, strlen(text), &len, &err);
				
				if (err) {
					err = ep_strdup_printf("error in field '%s': %s",f[colnum].name,err);
					goto on_failure;
				}
				
				break;
			} 
			case PT_TXTMOD_ENUM: {
				text = *(char**)e;
				text = text ? text : "";
				len = strlen(text);
				g_ptr_array_add(dd->tobe_freed,e);
			}
				break;
			default:
				g_assert_not_reached();
				return FALSE;
		}
		
		
		if (f[colnum].cb.chk) {
			if (! f[colnum].cb.chk(dd->rec, text, len, f[colnum].cbdata.chk, f[colnum].fld_data, &err)) {
				err = ep_strdup_printf("error in field '%s': %s",f[colnum].name,err);
				goto on_failure;
			}
		}
		
		f[colnum].cb.set(dd->rec,text,len, f[colnum].cbdata.set, f[colnum].fld_data);
	}

	if (dd->uat->update_cb) {
		dd->uat->update_cb(dd->rec,&err);
		
		if (err) {
			err = ep_strdup_printf("error updating record: %s",err);
			goto on_failure;
		}
	}
	
	if (dd->is_new) {
		void* rec_tmp = dd->rec;
		dd->rec = uat_add_record(dd->uat, dd->rec);
	
		if (dd->uat->free_cb) {
			dd->uat->free_cb(rec_tmp);
		}
		
		g_free(rec_tmp);
	}
	
	dd->uat->changed = TRUE;
	
	set_buttons(dd->uat,-1);

	if (dd->is_new) {
		append_row(dd->uat, (*dd->uat->nrows_p) - 1 );
	} else {
		reset_row(dd->uat,dd->row);
	}
		
    g_ptr_array_free(dd->entries,TRUE);
    window_destroy(GTK_WIDGET(dd->win));
	
	window_present(GTK_WIDGET(dd->uat->rep->window));
	 
    return TRUE;
on_failure:
		
	report_failure("%s",err);
	return FALSE;
}

static gboolean uat_cancel_dlg_cb(GtkWidget *win _U_, gpointer user_data) {
	struct _uat_dlg_data* dd = user_data;

	window_present(GTK_WIDGET(dd->uat->rep->window));

	if (dd->is_new) g_free(dd->rec);
    g_ptr_array_free(dd->entries,TRUE);
    window_destroy(GTK_WIDGET(dd->win));
	g_free(dd);

	while (dd->tobe_freed->len) g_free( g_ptr_array_remove_index_fast(dd->tobe_freed, dd->tobe_freed->len - 1 ) );
	
    return TRUE;
}

struct _fld_menu_item_data_t {
	const char* text;
	char const** valptr;
};

static void fld_menu_item_cb(GtkMenuItem *menuitem _U_, gpointer user_data) {
	struct _fld_menu_item_data_t* md = user_data;
	
	*(md->valptr) = md->text;
}

static void fld_menu_item_destroy_cb(GtkMenuItem *menuitem _U_, gpointer user_data) {
	g_free(user_data);
}

static void uat_edit_dialog(uat_t* uat, gint row) {
    GtkWidget *win, *main_tb, *main_vb, *bbox, *bt_cancel, *bt_ok;
    struct _uat_dlg_data* dd = g_malloc(sizeof(struct _uat_dlg_data));
	uat_field_t* f = uat->fields;
	guint colnum;
	
    dd->entries = g_ptr_array_new();
    dd->win = dlg_window_new(uat->name);
    dd->uat = uat;
	dd->rec = row < 0 ? g_malloc0(uat->record_size) : UAT_INDEX_PTR(uat,row);
	dd->is_new = row < 0 ? TRUE : FALSE;
	dd->row = row;
    dd->tobe_freed = g_ptr_array_new();
	
    win = dd->win;
		
#if GTK_MAJOR_VERSION >= 2
	gtk_window_set_resizable(GTK_WINDOW(win),FALSE);
    gtk_window_resize(GTK_WINDOW(win),400, 30*(uat->ncols+2));
#else
  	gtk_window_set_policy(GTK_WINDOW(win), FALSE, FALSE, TRUE);
	gtk_window_set_default_size(GTK_WINDOW(win), 400, 30*(uat->ncols+2));
    gtk_widget_set_usize(win, 400, 30*(uat->ncols+2));
#endif
    
    main_vb = gtk_vbox_new(FALSE,5);
    gtk_container_add(GTK_CONTAINER(win), main_vb);
	gtk_container_border_width(GTK_CONTAINER(main_vb), 6);
	
    main_tb = gtk_table_new(uat->ncols+1, 2, FALSE);
    gtk_box_pack_start(GTK_BOX(main_vb), main_tb, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(main_tb), 5);
    gtk_table_set_col_spacings(GTK_TABLE(main_tb), 10);
    
	for ( colnum = 0; colnum < uat->ncols; colnum++ ) {
        GtkWidget *entry, *label;
        char* text = fld_tostr(dd->rec,&(f[colnum]));
							   
        label = gtk_label_new(f[colnum].name);
        gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
        gtk_table_attach_defaults(GTK_TABLE(main_tb), label, 0, 1, colnum+1, colnum + 2);
        gtk_widget_show(label);
		
		
		switch(f[colnum].mode) {
			case PT_TXTMOD_STRING:
			case PT_TXTMOD_HEXBYTES: {
				entry = gtk_entry_new();
				g_ptr_array_add(dd->entries,entry);
				gtk_table_attach_defaults(GTK_TABLE(main_tb), entry, 1, 2, colnum+1, colnum + 2);
				gtk_widget_show(entry);
				if (! dd->is_new) {
					gtk_entry_set_text(GTK_ENTRY(entry),text);
				}
				break;
			}
			case PT_TXTMOD_ENUM: {
				GtkWidget *menu, *option_menu;
				int menu_index, index;
				const value_string* enum_vals = f[colnum].fld_data;
				void* valptr = g_malloc0(sizeof(void*));

				menu = gtk_menu_new();
				menu_index = -1;
				for (index = 0; enum_vals[index].strptr != NULL; index++) {
					struct _fld_menu_item_data_t* md = g_malloc(sizeof(struct _fld_menu_item_data_t)); /* XXX: leaked */
					const char* str = enum_vals[index].strptr;
					GtkWidget* menu_item = gtk_menu_item_new_with_label(str);
					
					md->text = str;
					md->valptr = valptr;
					
					gtk_menu_append(GTK_MENU(menu), menu_item);
					
					if ( g_str_equal(str, text) ) {
						menu_index = index;
						*((char const**)valptr) = str;
					}
					
					gtk_widget_show(menu_item);
					SIGNAL_CONNECT(menu_item, "activate", fld_menu_item_cb, md);
					SIGNAL_CONNECT(menu_item, "destroy", fld_menu_item_destroy_cb, md);
				}
				
				g_ptr_array_add(dd->entries,valptr);
				g_ptr_array_add(dd->tobe_freed,valptr);

				/* Create the option menu from the menu */
				option_menu = gtk_option_menu_new();
				gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
				gtk_widget_show(option_menu);

				/* Set its current value to the variable's current value */
				if (menu_index != -1)
					gtk_option_menu_set_history(GTK_OPTION_MENU(option_menu), menu_index);
								
				gtk_table_attach_defaults(GTK_TABLE(main_tb), option_menu, 1, 2, colnum+1, colnum + 2);

				break;
			}
			default:
				g_assert_not_reached();
				return;
		}
    }
	
    bbox = dlg_button_row_new(GTK_STOCK_CANCEL,GTK_STOCK_OK, NULL);
	gtk_box_pack_end(GTK_BOX(main_vb), bbox, FALSE, FALSE, 0);
    
    bt_ok = OBJECT_GET_DATA(bbox, GTK_STOCK_OK);
    SIGNAL_CONNECT(bt_ok, "clicked", uat_dlg_cb, dd);
    gtk_widget_grab_default(bt_ok);
    
    bt_cancel = OBJECT_GET_DATA(bbox, GTK_STOCK_CANCEL);
    SIGNAL_CONNECT(bt_cancel, "clicked", uat_cancel_dlg_cb, dd);
    gtk_widget_grab_default(bt_cancel);
    
    gtk_widget_show(main_tb);
    gtk_widget_show(main_vb);
    gtk_widget_show(win);
}

struct _uat_del {
	GtkWidget *win;
	uat_t* uat;
	gint idx;
};

static void uat_del_cb(GtkButton *button _U_, gpointer u) {
	struct _uat_del* ud = u;
	
	uat_remove_record_idx(ud->uat, ud->idx);
	gtk_clist_remove(GTK_CLIST(ud->uat->rep->clist),ud->idx);
	
	ud->uat->changed = TRUE;
	set_buttons(ud->uat,-1);
	
    window_destroy(GTK_WIDGET(ud->win));
	window_present(GTK_WIDGET(ud->uat->rep->window));
	g_free(ud);
}

static void uat_cancel_del_cb(GtkButton *button _U_, gpointer u) {
	struct _uat_del* ud = u;
    window_destroy(GTK_WIDGET(ud->win));
	window_present(GTK_WIDGET(ud->uat->rep->window));
	g_free(ud);
}

static void uat_del_dlg(uat_t* uat, int idx) {
    GtkWidget *win, *main_tb, *main_vb, *bbox, *bt_cancel, *bt_ok;
	uat_field_t* f = uat->fields;
	guint colnum;
	void* rec = UAT_INDEX_PTR(uat,idx);
	struct _uat_del* ud = g_malloc(sizeof(struct _uat_del));

	ud->uat = uat;
	ud->idx = idx;
    ud->win = win = dlg_window_new(ep_strdup_printf("Confirm Delete"));
	
	
#if GTK_MAJOR_VERSION >= 2
	gtk_window_set_resizable(GTK_WINDOW(win),FALSE);
    gtk_window_resize(GTK_WINDOW(win),400,25*(uat->ncols+2));
#else
	gtk_window_set_policy(GTK_WINDOW(win), FALSE, FALSE, TRUE);
	gtk_window_set_default_size(GTK_WINDOW(win), 400, 25*(uat->ncols+2));
    gtk_widget_set_usize(win, 400, 25*(uat->ncols+2));
#endif
    
    main_vb = gtk_vbox_new(FALSE,5);
    gtk_container_add(GTK_CONTAINER(win), main_vb);
	gtk_container_border_width(GTK_CONTAINER(main_vb), 6);
	
    main_tb = gtk_table_new(uat->ncols+1, 2, FALSE);
    gtk_box_pack_start(GTK_BOX(main_vb), main_tb, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(main_tb), 10);
    gtk_table_set_col_spacings(GTK_TABLE(main_tb), 15);
    
	for ( colnum = 0; colnum < uat->ncols; colnum++ ) {
        GtkWidget *label;
        char* text = fld_tostr(rec,&(f[colnum]));
				
        label = gtk_label_new(f[colnum].name);
        gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
        gtk_table_attach_defaults(GTK_TABLE(main_tb), label, 0, 1, colnum+1, colnum + 2);
        gtk_widget_show(label);
		
        label = gtk_label_new(text);
        gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
        gtk_table_attach_defaults(GTK_TABLE(main_tb), label, 1, 2, colnum+1, colnum + 2);
        gtk_widget_show(label);
	}
	
    bbox = dlg_button_row_new(GTK_STOCK_CANCEL,GTK_STOCK_DELETE, NULL);
	gtk_box_pack_start(GTK_BOX(main_vb), bbox, FALSE, FALSE, 0);
    
    bt_ok = OBJECT_GET_DATA(bbox,GTK_STOCK_DELETE);
    SIGNAL_CONNECT(bt_ok, "clicked", uat_del_cb, ud);
    gtk_widget_grab_default(bt_ok);
    
    bt_cancel = OBJECT_GET_DATA(bbox, GTK_STOCK_CANCEL);
    SIGNAL_CONNECT(bt_cancel, "clicked", uat_cancel_del_cb, ud);
    gtk_widget_grab_default(bt_cancel);
    
    gtk_widget_show(main_tb);
    gtk_widget_show(main_vb);
    gtk_widget_show(win);
}

static void uat_new_cb(GtkButton *button _U_, gpointer u) {
	uat_t* uat = u;
	uat_edit_dialog(uat, -1);
}

static void uat_edit_cb(GtkButton *button _U_, gpointer u) {
	uat_t* uat = u;
	uat_edit_dialog(uat, uat->rep->selected);
}

static void uat_delete_cb(GtkButton *button _U_, gpointer u) {
	uat_t* uat = u;
	uat_del_dlg(uat,uat->rep->selected);
}

static gboolean uat_window_delete_event_cb(GtkWindow *w _U_, GdkEvent* e _U_, gpointer u) {
	uat_t* uat = u;
	uat_rep_t* rep = uat->rep;
		
	SIGNAL_DISCONNECT_BY_FUNC(uat->rep->window, uat_window_delete_event_cb, uat);
	gtk_widget_destroy(uat->rep->window);
	
	uat->rep = NULL;
	if (rep) g_free(rep);
	return TRUE;
}

static gboolean uat_close_unchanged_cb(GtkWidget *w _U_, void* u) {
	return uat_window_delete_event_cb(NULL,NULL,u);
}

static gboolean uat_close_changed_cb(GtkWidget *w _U_, void* u) {
	unsaved_dialog(NULL,NULL,u);
	return TRUE;
}


static void uat_up_cb(GtkButton *button _U_, gpointer u) {
	uat_t* uat = u;
	guint row = uat->rep->selected;
	
	g_assert(row > 0);
	
	uat_swap(uat,row,row-1);
	gtk_clist_swap_rows(GTK_CLIST(uat->rep->clist),row,row-1);

	uat->changed = TRUE;
	
	row -= 1;
	uat->rep->selected = row;
	set_buttons(uat,row);
}

static void uat_down_cb(GtkButton *button _U_, gpointer u) {
	uat_t* uat = u;
	guint row = uat->rep->selected;
	
	g_assert(row < *uat->nrows_p - 1);
	
	uat_swap(uat,row,row+1);
	gtk_clist_swap_rows(GTK_CLIST(uat->rep->clist),row,row+1);
	
	uat->changed = TRUE;
	
	row += 1;
	uat->rep->selected = row;
	set_buttons(uat,row);
}

static void uat_save_cb(GtkButton *button _U_, gpointer u) {
	uat_t* uat = u;
	gchar* err = NULL;

	uat_save(uat,&err);
	
	if (err) {
		report_failure("Error while saving %s: %s",uat->name,err);
	}
	
	set_buttons(uat,-1);
}

static void remember_selected_row(GtkCList *clist _U_, gint row, gint column _U_, GdkEvent *event _U_, gpointer u) {
    uat_t* uat = u;
	
    uat->rep->selected = row;
    
    gtk_widget_set_sensitive (uat->rep->bt_edit, TRUE);
    gtk_widget_set_sensitive(uat->rep->bt_delete, TRUE);    
	
	set_buttons(uat,row);
}

static void unremember_selected_row(GtkCList *clist _U_, gint row _U_, gint column _U_, GdkEvent *event _U_, gpointer u)
{
    uat_t* uat = u;

	uat->rep->selected = -1;
	gtk_widget_set_sensitive (uat->rep->bt_edit, FALSE);
	gtk_widget_set_sensitive(uat->rep->bt_delete, FALSE);
	gtk_widget_set_sensitive (uat->rep->bt_down, FALSE);
	gtk_widget_set_sensitive (uat->rep->bt_up, FALSE);
}

static void uat_yesclose_cb(GtkWindow *w _U_, void* u) {
	uat_t* uat = u;
	uat->rep->dont_save = TRUE;
	
	window_delete_event_cb(uat->rep->unsaved_window,NULL,NULL);
	
	SIGNAL_DISCONNECT_BY_FUNC(uat->rep->window, uat_window_delete_event_cb, uat);
	SIGNAL_DISCONNECT_BY_FUNC(uat->rep->window, unsaved_dialog, uat);
	window_destroy(uat->rep->window);
	
	uat->rep->unsaved_window = NULL;
	g_free(uat->rep);
	uat->rep = NULL;
}


static void uat_saveandclose_cb(GtkWindow *w, void* u) {
	uat_save_cb(NULL,(uat_t*)u);
	uat_yesclose_cb(w,u);
}

static gboolean unsaved_dialog(GtkWindow *w _U_, GdkEvent* e _U_, gpointer u) {
	GtkWidget *win, *vbox, *label, *bbox;
	GtkWidget *save_bt, *close_bt;
	gchar* message;
	uat_t* uat  = u;
	
	if (uat->rep->unsaved_window) {
		window_present(uat->rep->unsaved_window);
		return TRUE;
	}
	
	uat->rep->unsaved_window = win = window_new(GTK_WINDOW_TOPLEVEL, "Discard Changes?");
	gtk_window_set_default_size(GTK_WINDOW(win), 360, 140);

#if GTK_MAJOR_VERSION >= 2
	gtk_window_set_position(GTK_WINDOW(win), GTK_WIN_POS_CENTER_ON_PARENT);
#else
	gtk_window_set_position(GTK_WINDOW(win), GTK_WIN_POS_CENTER);
#endif
	vbox = gtk_vbox_new(FALSE, 12);
	gtk_container_border_width(GTK_CONTAINER(vbox), 6);
	gtk_container_add(GTK_CONTAINER(win), vbox);
	
	message  = ep_strdup_printf("Changes to '%s' are not going to be saved to disk!\n"
								"Current changes will be active until wireshark is closed.\n"
								"You may open '%s' again to save changes before closing wireshark.\n"
								"Are you sure you want to close '%s' without saving?", uat->name, uat->name, uat->name);
	
	label = gtk_label_new(message);
	gtk_widget_show(label);

	bbox = dlg_button_row_new(GTK_STOCK_SAVE,GTK_STOCK_CLOSE, NULL);
	
	save_bt = OBJECT_GET_DATA(bbox, GTK_STOCK_SAVE);
	close_bt = OBJECT_GET_DATA(bbox, GTK_STOCK_CLOSE);
	
	SIGNAL_CONNECT(save_bt, "clicked", uat_saveandclose_cb, uat);
	SIGNAL_CONNECT(close_bt, "clicked", uat_yesclose_cb, uat);
	
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(vbox), bbox, FALSE, FALSE, 0);

	gtk_widget_show_all(win);
	window_present(win);

	return TRUE;
}

#if (GLIB_MAJOR_VERSION >= 2)
static void uat_help_cb(GtkWidget* w _U_, gpointer u) {
	help_topic_html(ep_strdup_printf("%s.html",((uat_t*)u)->help));
}
#endif

static GtkWidget* uat_window(void* u) {
	uat_t* uat = u;
	uat_field_t* f = uat->fields;
	uat_rep_t* rep;
	guint i;
	guint colnum;
	GtkWidget *l_hbox, *c_hbox, *r_hbox;
	
	if (uat->rep) {
		window_present(uat->rep->window);
		return uat->rep->window;
	} else {
		uat->rep = rep = g_malloc0(sizeof(uat_rep_t));
	}
	
	rep->window = window_new(GTK_WINDOW_TOPLEVEL, uat->name);
	gtk_window_set_default_size(GTK_WINDOW(rep->window), 480, 320);
	
#if GTK_MAJOR_VERSION >= 2
	gtk_window_set_position(GTK_WINDOW(rep->window), GTK_WIN_POS_CENTER_ON_PARENT);
#else
	gtk_window_set_position(GTK_WINDOW(rep->window), GTK_WIN_POS_CENTER);
#endif
	
	gtk_container_border_width(GTK_CONTAINER(rep->window), 6);
	
	rep->vbox = gtk_vbox_new(FALSE, 12);
	gtk_container_border_width(GTK_CONTAINER(rep->vbox), 6);
	gtk_container_add(GTK_CONTAINER(rep->window), rep->vbox);

	rep->scrolledwindow = scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(rep->vbox), rep->scrolledwindow);

#if GTK_MAJOR_VERSION >= 2
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(rep->scrolledwindow), GTK_SHADOW_IN);
#endif
	
	rep->clist = gtk_clist_new(uat->ncols);
		
	for ( colnum = 0; colnum < uat->ncols; colnum++ ) {
		gtk_clist_set_column_title(GTK_CLIST(rep->clist), colnum, f[colnum].name);
		gtk_clist_set_column_auto_resize(GTK_CLIST(rep->clist), colnum, TRUE);
	}
	
	gtk_clist_column_titles_show(GTK_CLIST(rep->clist));
	gtk_clist_freeze(GTK_CLIST(rep->clist));
	
	for ( i = 0 ; i < *(uat->nrows_p); i++ ) {
		append_row(uat, i);
	}
	
	gtk_clist_thaw(GTK_CLIST(rep->clist));
	
#if GTK_MAJOR_VERSION < 2
	gtk_clist_set_selection_mode(GTK_CLIST(rep->clist),GTK_SELECTION_SINGLE);
#else
	rep->selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(rep->clist));
	gtk_tree_selection_set_mode(rep->selection, GTK_SELECTION_SINGLE);
#endif
	
	gtk_container_add(GTK_CONTAINER(rep->scrolledwindow), rep->clist);

	rep->bbox = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(rep->bbox);
	
	
    l_hbox = gtk_hbutton_box_new();
    gtk_box_pack_start(GTK_BOX(rep->bbox), l_hbox, TRUE, TRUE, 0);
    gtk_widget_show(l_hbox);

	c_hbox = gtk_hbutton_box_new();
    gtk_box_pack_start(GTK_BOX(rep->bbox), c_hbox, TRUE, TRUE, 40);
    gtk_widget_show(c_hbox);

	r_hbox = gtk_hbutton_box_new();
    gtk_box_pack_start(GTK_BOX(rep->bbox), r_hbox, TRUE, TRUE, 0);
    gtk_widget_show(r_hbox);
	
	
	
	rep->bt_down = BUTTON_NEW_FROM_STOCK(GTK_STOCK_GO_DOWN);
	gtk_box_pack_end(GTK_BOX(l_hbox), rep->bt_down, TRUE, TRUE, 0);
    gtk_widget_show(rep->bt_down);
	
	rep->bt_up = BUTTON_NEW_FROM_STOCK(GTK_STOCK_GO_UP);
	gtk_box_pack_end(GTK_BOX(l_hbox), rep->bt_up, TRUE, TRUE, 0);
    gtk_widget_show(rep->bt_up);
	
	
	rep->bt_new = BUTTON_NEW_FROM_STOCK(GTK_STOCK_NEW);
	gtk_box_pack_start(GTK_BOX(c_hbox), rep->bt_new, TRUE, TRUE, 0);
    gtk_widget_show(rep->bt_new);
	
	rep->bt_edit = BUTTON_NEW_FROM_STOCK(WIRESHARK_STOCK_EDIT);
	gtk_box_pack_start(GTK_BOX(c_hbox), rep->bt_edit, TRUE, TRUE, 0);
    gtk_widget_show(rep->bt_edit);

	rep->bt_delete = BUTTON_NEW_FROM_STOCK(GTK_STOCK_DELETE);
	gtk_box_pack_end(GTK_BOX(c_hbox), rep->bt_delete, TRUE, TRUE, 0);
    gtk_widget_show(rep->bt_delete);

#if (GLIB_MAJOR_VERSION >= 2)
	if(uat->help) {
		GtkWidget* help_btn;
		help_btn = BUTTON_NEW_FROM_STOCK(GTK_STOCK_HELP);
		gtk_box_pack_start(GTK_BOX(r_hbox), help_btn, TRUE, TRUE, 0);
		gtk_widget_show(help_btn);
		SIGNAL_CONNECT(help_btn, "clicked", uat_help_cb, uat);
	}
#endif

	rep->bt_save = BUTTON_NEW_FROM_STOCK(GTK_STOCK_SAVE);
	gtk_box_pack_end(GTK_BOX(r_hbox), rep->bt_save, TRUE, TRUE, 0);
    gtk_widget_show(rep->bt_save);
	
	rep->bt_close = BUTTON_NEW_FROM_STOCK(GTK_STOCK_OK);
	gtk_box_pack_end(GTK_BOX(r_hbox), rep->bt_close, TRUE, TRUE, 0);
    gtk_widget_show(rep->bt_close);

	
	gtk_widget_show(l_hbox);
    gtk_widget_show(c_hbox);
    gtk_widget_show(r_hbox);

	gtk_box_pack_start(GTK_BOX(rep->vbox), rep->bbox, FALSE, FALSE, 0);

	gtk_widget_set_sensitive (rep->bt_up, FALSE);
	gtk_widget_set_sensitive (rep->bt_down, FALSE);
	gtk_widget_set_sensitive (rep->bt_edit, FALSE);
	gtk_widget_set_sensitive (rep->bt_delete, FALSE);
	gtk_widget_set_sensitive (rep->bt_save, uat->changed);
	

#if GTK_MAJOR_VERSION < 2
	SIGNAL_CONNECT(rep->clist, "select_row", remember_selected_row, uat);
	SIGNAL_CONNECT(rep->clist, "unselect_row", unremember_selected_row, uat);
#else
	SIGNAL_CONNECT(selection, "changed", remember_selected_row, uat);
#endif
	
	SIGNAL_CONNECT(rep->bt_new, "clicked", uat_new_cb, uat);
	SIGNAL_CONNECT(rep->bt_edit, "clicked", uat_edit_cb, uat);
	SIGNAL_CONNECT(rep->bt_delete, "clicked", uat_delete_cb, uat);
	SIGNAL_CONNECT(rep->bt_up, "clicked", uat_up_cb, uat);
	SIGNAL_CONNECT(rep->bt_down, "clicked", uat_down_cb, uat);
	SIGNAL_CONNECT(rep->bt_save, "clicked", uat_save_cb, uat);

	if (uat->changed) {
		gtk_widget_set_sensitive (uat->rep->bt_save, TRUE);
		SIGNAL_CONNECT(rep->bt_close, "clicked", uat_close_changed_cb, uat);
		SIGNAL_CONNECT(GTK_WINDOW(rep->window), "delete_event", unsaved_dialog, uat);
		SIGNAL_CONNECT(GTK_WINDOW(rep->window), "destroy", unsaved_dialog, uat);		
	} else {
		gtk_widget_set_sensitive (uat->rep->bt_save, FALSE);
		SIGNAL_CONNECT(rep->bt_close, "clicked", uat_close_unchanged_cb, uat);
		SIGNAL_CONNECT(GTK_WINDOW(rep->window), "delete_event", uat_window_delete_event_cb, uat);
		SIGNAL_CONNECT(GTK_WINDOW(rep->window), "destroy", uat_window_delete_event_cb, uat);				
	}
	
	gtk_widget_grab_focus(rep->clist);

	gtk_widget_show_all(rep->window);
	window_present(rep->window);

	return rep->window;
}

void uat_window_cb(GtkWidget* u _U_, void* uat) {
	uat_window(uat);
}


/*
 Add an UAT to the menu
 */
static void add_uat_to_menu(void* u, void* user_data _U_) {
	uat_t* uat = u;
	register_stat_menu_item(uat->category ? ep_strdup_printf("%s/%s",uat->category,uat->name) : uat->name, 
							REGISTER_USER_TABLES,
							uat_window_cb,
							NULL,
							NULL,
							uat);
}


void uat_init_menus(void) {
	uat_foreach_table(add_uat_to_menu,NULL);

}

