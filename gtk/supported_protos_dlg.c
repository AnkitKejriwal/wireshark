/* supported_protos_dlg.c
 *
 * Laurent Deniel <laurent.deniel@free.fr>
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <gtk/gtk.h>
#include <string.h>

#ifdef NEED_SNPRINTF_H
# include "snprintf.h"
#endif

#include "supported_protos_dlg.h"
#include "prefs.h"
#include "globals.h"
#include "gtkglobals.h"
#include "ui_util.h"
#include "compat_macros.h"
#include "dlg_utils.h"



static const char *proto_supported =
"The following %d protocols (and packet types) are currently\n"
"supported by Ethereal:\n\n";

static const char *dfilter_supported =
"The following per-protocol fields are currently supported by\n"
"Ethereal and can be used in display filters:\n";



typedef enum {
  PROTOCOL_SUPPORTED,
  DFILTER_SUPPORTED,
} supported_type_t;

static void supported_close_cb(GtkWidget *w, gpointer data);
static void supported_destroy_cb(GtkWidget *w, gpointer data);
static void insert_text(GtkWidget *w, const char *buffer, int nchars);
static void set_supported_text(GtkWidget *w, supported_type_t type);

/*
 * Keep a static pointer to the current "Supported" window, if any, so that
 * if somebody tries to do "Help->Supported" while there's already a
 * "Supported" window up, we just pop up the existing one, rather than
 * creating a new one.
*/
static GtkWidget *supported_w = NULL;

/*
 * Keep static pointers to the text widgets as well (for text format changes).
 */
static GtkWidget *proto_text, *dfilter_text;



void supported_cb(GtkWidget *w _U_, gpointer data _U_)
{

  GtkWidget *main_vb, *bbox, *supported_nb, *close_bt, *label, *txt_scrollw,
    *proto_vb,
#if GTK_MAJOR_VERSION < 2
    *dfilter_tb, *dfilter_vsb;
#else
    *dfilter_vb;
#endif

  if (supported_w != NULL) {
    /* There's already a "Supported" dialog box; reactivate it. */
    reactivate_window(supported_w);
    return;
  }

  supported_w = dlg_window_new("Ethereal: Supported Protocols");
  SIGNAL_CONNECT(supported_w, "destroy", supported_destroy_cb, NULL);
  WIDGET_SET_SIZE(supported_w, DEF_WIDTH * 2/3, DEF_HEIGHT * 2/3);
  gtk_container_border_width(GTK_CONTAINER(supported_w), 2);

  /* Container for each row of widgets */
  main_vb = gtk_vbox_new(FALSE, 1);
  gtk_container_border_width(GTK_CONTAINER(main_vb), 1);
  gtk_container_add(GTK_CONTAINER(supported_w), main_vb);
  gtk_widget_show(main_vb);

  /* supported topics container */
  supported_nb = gtk_notebook_new();
  gtk_container_add(GTK_CONTAINER(main_vb), supported_nb);


  /* humm, gtk 1.2 does not support horizontal scrollbar for text widgets */

  /* protocol list */
  proto_vb = gtk_vbox_new(FALSE, 0);
  gtk_container_border_width(GTK_CONTAINER(proto_vb), 1);

  txt_scrollw = scrolled_window_new(NULL, NULL);
  gtk_box_pack_start(GTK_BOX(proto_vb), txt_scrollw, TRUE, TRUE, 0);
#if GTK_MAJOR_VERSION < 2
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(txt_scrollw),
				 GTK_POLICY_ALWAYS,
				 GTK_POLICY_ALWAYS);
  proto_text = gtk_text_new(NULL, NULL);
  gtk_text_set_editable(GTK_TEXT(proto_text), FALSE);
  gtk_text_set_line_wrap(GTK_TEXT(proto_text), FALSE);
  set_supported_text(proto_text, PROTOCOL_SUPPORTED);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(txt_scrollw),
					proto_text);
#else
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(txt_scrollw),
				 GTK_POLICY_AUTOMATIC,
				 GTK_POLICY_AUTOMATIC);
  proto_text = gtk_text_view_new();
  gtk_text_view_set_editable(GTK_TEXT_VIEW(proto_text), FALSE);
  set_supported_text(proto_text, PROTOCOL_SUPPORTED);
  gtk_container_add(GTK_CONTAINER(txt_scrollw), proto_text);
#endif
  gtk_widget_show(txt_scrollw);
  gtk_widget_show(proto_text);
  gtk_widget_show(proto_vb);
  label = gtk_label_new("Protocols");
  gtk_notebook_append_page(GTK_NOTEBOOK(supported_nb), proto_vb, label);

  /* display filter fields */
#if GTK_MAJOR_VERSION < 2
  /* X windows have a maximum size of 32767.  Since the height can easily
     exceed this, we have to jump through some hoops to have a functional
     vertical scroll bar. */

  dfilter_tb = gtk_table_new(2, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (dfilter_tb), 0, 3);
  gtk_table_set_row_spacing (GTK_TABLE (dfilter_tb), 0, 3);
  gtk_container_border_width(GTK_CONTAINER(dfilter_tb), 1);

  txt_scrollw = scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(txt_scrollw),
				 GTK_POLICY_ALWAYS,
				 GTK_POLICY_NEVER);
  dfilter_text = gtk_text_new(NULL, NULL);
  dfilter_vsb = gtk_vscrollbar_new(GTK_TEXT(dfilter_text)->vadj);
  if (prefs.gui_scrollbar_on_right) {
    gtk_table_attach (GTK_TABLE (dfilter_tb), txt_scrollw, 0, 1, 0, 1,
                            GTK_EXPAND | GTK_SHRINK | GTK_FILL,
                            GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
    gtk_table_attach (GTK_TABLE (dfilter_tb), dfilter_vsb, 1, 2, 0, 1,
                            GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  } else {
    gtk_table_attach (GTK_TABLE (dfilter_tb), txt_scrollw, 1, 2, 0, 1,
                            GTK_EXPAND | GTK_SHRINK | GTK_FILL,
                            GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
    gtk_table_attach (GTK_TABLE (dfilter_tb), dfilter_vsb, 0, 1, 0, 1,
                            GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  }
  gtk_text_set_editable(GTK_TEXT(dfilter_text), FALSE);
  gtk_text_set_line_wrap(GTK_TEXT(dfilter_text), FALSE);
  set_supported_text(dfilter_text, DFILTER_SUPPORTED);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(txt_scrollw),
					dfilter_text);
#else
  dfilter_vb = gtk_vbox_new(FALSE, 0);
  gtk_container_border_width(GTK_CONTAINER(dfilter_vb), 1);

  txt_scrollw = scrolled_window_new(NULL, NULL);
  gtk_box_pack_start(GTK_BOX(dfilter_vb), txt_scrollw, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(txt_scrollw),
				 GTK_POLICY_AUTOMATIC,
				 GTK_POLICY_AUTOMATIC);
  dfilter_text = gtk_text_view_new();
  if (prefs.gui_scrollbar_on_right) {
    gtk_scrolled_window_set_placement(GTK_SCROLLED_WINDOW(txt_scrollw),
                                      GTK_CORNER_TOP_LEFT);
  }
  else {
    gtk_scrolled_window_set_placement(GTK_SCROLLED_WINDOW(txt_scrollw),
                                      GTK_CORNER_TOP_RIGHT);
  }
  gtk_text_view_set_editable(GTK_TEXT_VIEW(dfilter_text), FALSE);
  set_supported_text(dfilter_text, DFILTER_SUPPORTED);
  gtk_container_add(GTK_CONTAINER(txt_scrollw), dfilter_text);
#endif
  gtk_widget_show(txt_scrollw);
  gtk_widget_show(dfilter_text);
#if GTK_MAJOR_VERSION < 2
  gtk_widget_show(dfilter_tb);
  gtk_widget_show(dfilter_vsb);
#else
  gtk_widget_show(dfilter_vb);
#endif
  label = gtk_label_new("Display Filter Fields");
#if GTK_MAJOR_VERSION < 2
  gtk_notebook_append_page(GTK_NOTEBOOK(supported_nb), dfilter_tb, label);
#else
  gtk_notebook_append_page(GTK_NOTEBOOK(supported_nb), dfilter_vb, label);
#endif

  /* XXX add other panels here ... */

  gtk_widget_show(supported_nb);

  /* Buttons ("Close" only one for now) */
  bbox = gtk_hbutton_box_new();
  /*bbox = gtk_hbox_new(FALSE, 1);*/
  gtk_box_pack_end(GTK_BOX(main_vb), bbox, FALSE, FALSE, 0);
  gtk_widget_show(bbox);
#if GTK_MAJOR_VERSION < 2
  close_bt = gtk_button_new_with_label("OK");
#else
  close_bt = gtk_button_new_from_stock(GTK_STOCK_OK);
#endif
  SIGNAL_CONNECT(close_bt, "clicked", supported_close_cb, supported_w);
  GTK_WIDGET_SET_FLAGS(close_bt, GTK_CAN_DEFAULT);
  gtk_container_add(GTK_CONTAINER(bbox), close_bt);
  gtk_widget_grab_default(close_bt);
  gtk_widget_show(close_bt);

  gtk_quit_add_destroy(gtk_main_level(), GTK_OBJECT(supported_w));

  /* Catch the "key_press_event" signal in the window, so that we can catch
     the ESC key being pressed and act as if the "Cancel" button had
     been selected. */
  dlg_set_cancel(supported_w, close_bt);

  gtk_widget_show(supported_w);

} /* supported_cb */

static void supported_close_cb(GtkWidget *w _U_, gpointer data)
{
  gtk_widget_destroy(GTK_WIDGET(data));
}

static void supported_destroy_cb(GtkWidget *w _U_, gpointer data _U_)
{
  /* Note that we no longer have a Help window. */
  supported_w = NULL;
}

static void insert_text(GtkWidget *w, const char *buffer, int nchars)
{
#if GTK_MAJOR_VERSION < 2
    gtk_text_insert(GTK_TEXT(w), m_r_font, NULL, NULL, buffer, nchars);
#else
    GtkTextBuffer *buf= gtk_text_view_get_buffer(GTK_TEXT_VIEW(w));
    GtkTextIter    iter;

    gtk_text_buffer_get_end_iter(buf, &iter);
    gtk_widget_modify_font(w, m_r_font);
    if (!g_utf8_validate(buffer, -1, NULL))
        printf(buffer);
    gtk_text_buffer_insert(buf, &iter, buffer, nchars);
#endif
}


static void set_supported_text(GtkWidget *w, supported_type_t type)
{

#define BUFF_LEN 4096
#define B_LEN    256
  char buffer[BUFF_LEN];
  header_field_info *hfinfo;
  int i, len, maxlen = 0, maxlen2 = 0, maxlen4 = 0;
#if GTK_MAJOR_VERSION < 2
  int maxlen3 = 0, nb_lines = 0;
  int width, height;
#endif
  const char *type_name;
  void *cookie, *cookie2;
  protocol_t *protocol;
  char *name, *short_name, *filter_name;
  int namel = 0, short_namel = 0, filter_namel = 0;
  int count, fcount;


  /*
   * XXX quick hack:
   * the width and height computations are performed to make the
   * horizontal scrollbar work in gtk1.2. This is only necessary for the
   * PROTOCOL_SUPPORTED and DFILTER_SUPPORTED windows since all others should
   * not have any horizontal scrollbar (line wrapping enabled).
   */


#if GTK_MAJOR_VERSION < 2
  gtk_text_freeze(GTK_TEXT(w));
#endif

  switch(type) {

  case PROTOCOL_SUPPORTED :
    /* first pass to know the maximum length of first field */
    count = 0;
    for (i = proto_get_first_protocol(&cookie); i != -1;
         i = proto_get_next_protocol(&cookie)) {
	    count++;
        protocol = find_protocol_by_id(i);
	    name = proto_get_protocol_name(i);
	    short_name = proto_get_protocol_short_name(protocol);
	    filter_name = proto_get_protocol_filter_name(i);
	    if ((len = strlen(name)) > namel)
		    namel = len;
	    if ((len = strlen(short_name)) > short_namel)
		    short_namel = len;
	    if ((len = strlen(filter_name)) > filter_namel)
		    filter_namel = len;
    }
    maxlen = namel + short_namel + filter_namel;

    len = snprintf(buffer, BUFF_LEN, proto_supported, count);
#if GTK_MAJOR_VERSION < 2
    maxlen2 = len;
    width = gdk_string_width(m_r_font, buffer);
    insert_text(w, buffer, maxlen2);
#else
    insert_text(w, buffer, len);
#endif

    /* ok, display the correctly aligned strings */
    for (i = proto_get_first_protocol(&cookie); i != -1;
         i = proto_get_next_protocol(&cookie)) {
        protocol = find_protocol_by_id(i);
	    name = proto_get_protocol_name(i);
	    short_name = proto_get_protocol_short_name(protocol);
	    filter_name = proto_get_protocol_filter_name(i);
 
	    /* the name used for sorting in the left column */
	    len = snprintf(buffer, BUFF_LEN, "%*s %*s %*s\n",
			   -short_namel,  short_name,
			   -namel,	  name,
			   -filter_namel, filter_name);
#if GTK_MAJOR_VERSION < 2
	    if (len > maxlen2) {
		    maxlen2 = len;
		    if ((len = gdk_string_width(m_r_font, buffer)) > width)
			    width = len;
	    }
	    insert_text(w, buffer, strlen(buffer));
	    nb_lines++;
#else
	    insert_text(w, buffer, strlen(buffer));
#endif
    }

#if GTK_MAJOR_VERSION < 2
    height = (3 + nb_lines) * m_font_height;
    WIDGET_SET_SIZE(w, 20 + width, 20 + height);
#endif
    break;

  case DFILTER_SUPPORTED  :

    /* XXX we should display hinfo->blurb instead of name (if not empty) */

    /* first pass to know the maximum length of first and second fields */
    for (i = proto_get_first_protocol(&cookie); i != -1;
         i = proto_get_next_protocol(&cookie)) {

	    for (hfinfo = proto_get_first_protocol_field(i, &cookie2); hfinfo != NULL;
		 hfinfo = proto_get_next_protocol_field(&cookie2)) {

		    if (hfinfo->same_name_prev != NULL) /* ignore duplicate names */
			    continue;

		    if ((len = strlen(hfinfo->abbrev)) > maxlen)
			    maxlen = len;
		    if ((len = strlen(hfinfo->name)) > maxlen2)
			    maxlen2 = len;
		    if ((len = strlen(hfinfo->blurb)) > maxlen4)
			    maxlen4 = len;
	    }
    }

#if GTK_MAJOR_VERSION < 2
    maxlen3 = strlen(dfilter_supported);
    width = gdk_string_width(m_r_font, dfilter_supported);
    insert_text(w, dfilter_supported, maxlen3);
#else
    insert_text(w, dfilter_supported, strlen(dfilter_supported));
#endif

    fcount = 0;
    for (i = proto_get_first_protocol(&cookie); i != -1;
         i = proto_get_next_protocol(&cookie)) {
        protocol = find_protocol_by_id(i);
	    name = proto_get_protocol_name(i);
	    short_name = proto_get_protocol_short_name(protocol);
	    filter_name = proto_get_protocol_filter_name(i);

	    count = 0;
	    for (hfinfo = proto_get_first_protocol_field(i, &cookie2); hfinfo != NULL;
		 hfinfo = proto_get_next_protocol_field(&cookie2)) {

		    if (hfinfo->same_name_prev != NULL) /* ignore duplicate names */
			    continue;
		    count++;
	    }
	    fcount += count;

	    len = snprintf(buffer, BUFF_LEN, "\n%s - %s (%s) [%d fields]:\n",
			   short_name, name, filter_name, count);
	    insert_text(w, buffer, len);

	    for (hfinfo = proto_get_first_protocol_field(i, &cookie2); hfinfo != NULL;
		 hfinfo = proto_get_next_protocol_field(&cookie2)) {

		    if (hfinfo->same_name_prev != NULL) /* ignore duplicate names */
			    continue;

		    type_name = ftype_pretty_name(hfinfo->type);
		    len = snprintf(buffer, BUFF_LEN, "%*s %*s %*s (%s)\n",
				   -maxlen,  hfinfo->abbrev,
				   -maxlen2, hfinfo->name,
				   -maxlen4, hfinfo->blurb,
				   type_name);
#if GTK_MAJOR_VERSION < 2
		    if (len > maxlen3) {
			    maxlen3 = len;
			    if ((len = gdk_string_width(m_r_font, buffer)) > width)
				    width = len;
		    }
		    insert_text(w, buffer, strlen(buffer));
		    nb_lines ++;
#else
		    insert_text(w, buffer, strlen(buffer));
#endif
	    }
    }
    len = snprintf(buffer, BUFF_LEN, "\n-- Total %d fields\n", fcount);
    insert_text(w, buffer, len);

#if GTK_MAJOR_VERSION < 2
    height = (5 + nb_lines) * m_font_height;
    WIDGET_SET_SIZE(w, 20 + width, 20 + height);
#endif
    break;
  default :
    g_assert_not_reached();
    break;
  } /* switch(type) */
#if GTK_MAJOR_VERSION < 2
  gtk_text_thaw(GTK_TEXT(w));
#endif
} /* set_supported_text */


static void clear_supported_text(GtkWidget *w)
{
#if GTK_MAJOR_VERSION < 2
  GtkText *txt = GTK_TEXT(w);

  gtk_text_set_point(txt, 0);
  /* Keep GTK+ 1.2.3 through 1.2.6 from dumping core - see
     http://www.ethereal.com/lists/ethereal-dev/199912/msg00312.html and
     http://www.gnome.org/mailing-lists/archives/gtk-devel-list/1999-October/0051.shtml
     for more information */
  gtk_adjustment_set_value(txt->vadj, 0.0);
  gtk_text_forward_delete(txt, gtk_text_get_length(txt));
#else
  GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(w));

  gtk_text_buffer_set_text(buf, "", 0);
#endif
}


/* Redraw all the text widgets, to use a new font. */
void supported_redraw(void)
{
  if (supported_w != NULL) {
#if GTK_MAJOR_VERSION < 2
    gtk_text_freeze(GTK_TEXT(proto_text));
#endif
    clear_supported_text(proto_text);
    set_supported_text(proto_text, PROTOCOL_SUPPORTED);
#if GTK_MAJOR_VERSION < 2
    gtk_text_thaw(GTK_TEXT(proto_text));

    gtk_text_freeze(GTK_TEXT(dfilter_text));
#endif
    clear_supported_text(dfilter_text);
    set_supported_text(dfilter_text, DFILTER_SUPPORTED);
#if GTK_MAJOR_VERSION < 2
    gtk_text_thaw(GTK_TEXT(dfilter_text));
#endif
  }
}
