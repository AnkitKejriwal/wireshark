/* stream_prefs.c
 * Dialog boxes for preferences for the stream window
 *
 * $Id: stream_prefs.c,v 1.1 2002/08/31 09:55:22 oabad Exp $
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
#include "config.h"
#endif

#include <errno.h>
#include <gtk/gtk.h>

#include "color.h"
#include "color_utils.h"
#include "globals.h"
#include "stream_prefs.h"
#include "keys.h"
#include "print.h"
#include "prefs.h"

static void update_text_color(GtkWidget *, gpointer);
static void update_current_color(GtkWidget *, gpointer);

static GdkColor tcolors[4], *curcolor = NULL;

#define SAMPLE_CLIENT_TEXT "Sample client text\n"
#define SAMPLE_SERVER_TEXT "Sample server text\n"
#define CFG_IDX 0
#define CBG_IDX 1
#define SFG_IDX 2
#define SBG_IDX 3
#define STREAM_SAMPLE_KEY "stream_entry"
#define STREAM_CS_KEY "stream_colorselection"
#define CS_RED 0
#define CS_GREEN 1
#define CS_BLUE 2
#define CS_OPACITY 3

GtkWidget *
stream_prefs_show()
{
  GtkWidget *main_vb, *main_tb, *label, *optmenu, *menu, *menuitem;
  GtkWidget *sample, *colorsel;
  int        width, height, i;
  gchar     *mt[] = { "Client foreground", "Client background",
                      "Server foreground", "Server background" };
  int mcount = sizeof(mt) / sizeof (gchar *);
  gdouble scolor[4];
  GtkTextBuffer *buf;
  GtkTextIter    iter;

  color_t_to_gdkcolor(&tcolors[CFG_IDX], &prefs.st_client_fg);
  color_t_to_gdkcolor(&tcolors[CBG_IDX], &prefs.st_client_bg);
  color_t_to_gdkcolor(&tcolors[SFG_IDX], &prefs.st_server_fg);
  color_t_to_gdkcolor(&tcolors[SBG_IDX], &prefs.st_server_bg);

  curcolor = &tcolors[CFG_IDX];

  scolor[CS_RED]     = (gdouble) (curcolor->red)   / 65535.0;
  scolor[CS_GREEN]   = (gdouble) (curcolor->green) / 65535.0;
  scolor[CS_BLUE]    = (gdouble) (curcolor->blue)  / 65535.0;
  scolor[CS_OPACITY] = 1.0;

  /* Enclosing containers for each row of widgets */
  main_vb = gtk_vbox_new(FALSE, 5);
  gtk_container_border_width(GTK_CONTAINER(main_vb), 5);

  main_tb = gtk_table_new(3, 3, FALSE);
  gtk_box_pack_start(GTK_BOX(main_vb), main_tb, FALSE, FALSE, 0);
  gtk_table_set_row_spacings(GTK_TABLE(main_tb), 10);
  gtk_table_set_col_spacings(GTK_TABLE(main_tb), 15);
  gtk_widget_show(main_tb);

  label = gtk_label_new("Set:");
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach_defaults(GTK_TABLE(main_tb), label, 0, 1, 0, 1);
  gtk_widget_show(label);

  /* We have to create this now, and configure it below. */
  colorsel = gtk_color_selection_new();

  optmenu = gtk_option_menu_new ();
  menu = gtk_menu_new ();
  for (i = 0; i < mcount; i++){
    menuitem = gtk_menu_item_new_with_label (mt[i]);
    gtk_object_set_data(GTK_OBJECT(menuitem), STREAM_CS_KEY,
      (gpointer) colorsel);
    g_signal_connect(G_OBJECT(menuitem), "activate",
                     G_CALLBACK(update_current_color), &tcolors[i]);
    gtk_widget_show (menuitem);
    gtk_menu_append (GTK_MENU (menu), menuitem);
  }
  gtk_option_menu_set_menu (GTK_OPTION_MENU (optmenu), menu);
  gtk_table_attach_defaults(GTK_TABLE(main_tb), optmenu, 1, 2, 0, 1);
  gtk_widget_show(optmenu);

  sample = gtk_text_view_new();
  height = 2 * (gtk_style_get_font(sample->style)->ascent +
                gtk_style_get_font(sample->style)->descent);
  width = gdk_string_width(gtk_style_get_font(sample->style),
                           "Sample server text");
  gtk_widget_set_size_request(GTK_WIDGET(sample), width, height);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(sample), FALSE);
  buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(sample));
  gtk_text_buffer_get_start_iter(buf, &iter);
  gtk_text_buffer_create_tag(buf, "client",
                             "foreground-gdk", &tcolors[CFG_IDX],
                             "background-gdk", &tcolors[CBG_IDX], NULL);
  gtk_text_buffer_create_tag(buf, "server",
                             "foreground-gdk", &tcolors[SFG_IDX],
                             "background-gdk", &tcolors[SBG_IDX], NULL);
  gtk_text_buffer_insert_with_tags_by_name(buf, &iter, SAMPLE_CLIENT_TEXT, -1,
                                           "client", NULL);
  gtk_text_buffer_insert_with_tags_by_name(buf, &iter, SAMPLE_SERVER_TEXT, -1,
                                           "server", NULL);
  gtk_table_attach_defaults(GTK_TABLE(main_tb), sample, 2, 3, 0, 2);
  gtk_widget_show(sample);

  gtk_color_selection_set_color(GTK_COLOR_SELECTION(colorsel), &scolor[CS_RED]);
  gtk_table_attach(GTK_TABLE(main_tb), colorsel, 0, 3, 2, 3,
		  GTK_SHRINK, GTK_SHRINK, 0, 0);

  gtk_object_set_data(GTK_OBJECT(colorsel), STREAM_SAMPLE_KEY,
    (gpointer) sample);
  g_signal_connect(G_OBJECT(colorsel), "color-changed",
                   G_CALLBACK(update_text_color), NULL);
  gtk_widget_show(colorsel);

  gtk_widget_show(main_vb);
  return(main_vb);
}

static void
update_text_color(GtkWidget *w, gpointer data _U_) {
  GtkTextView *sample = gtk_object_get_data(GTK_OBJECT(w), STREAM_SAMPLE_KEY);
  gdouble   scolor[4];
  GtkTextBuffer *buf;
  GtkTextTag    *tag;


  gtk_color_selection_get_color(GTK_COLOR_SELECTION(w), &scolor[CS_RED]);

  curcolor->red   = (gushort) (scolor[CS_RED]   * 65535.0);
  curcolor->green = (gushort) (scolor[CS_GREEN] * 65535.0);
  curcolor->blue  = (gushort) (scolor[CS_BLUE]  * 65535.0);

  buf = gtk_text_view_get_buffer(sample);
  tag = gtk_text_tag_table_lookup(gtk_text_buffer_get_tag_table(buf), "client");
  g_object_set(tag, "foreground-gdk", &tcolors[CFG_IDX], "background-gdk",
               &tcolors[CBG_IDX], NULL);
  tag = gtk_text_tag_table_lookup(gtk_text_buffer_get_tag_table(buf), "server");
  g_object_set(tag, "foreground-gdk", &tcolors[SFG_IDX], "background-gdk",
               &tcolors[SBG_IDX], NULL);
}

static void
update_current_color(GtkWidget *w, gpointer data)
{
  GtkColorSelection *colorsel = GTK_COLOR_SELECTION(gtk_object_get_data(GTK_OBJECT(w),
    STREAM_CS_KEY));
  gdouble            scolor[4];

  curcolor = (GdkColor *) data;

  scolor[CS_RED]     = (gdouble) (curcolor->red)   / 65535.0;
  scolor[CS_GREEN]   = (gdouble) (curcolor->green) / 65535.0;
  scolor[CS_BLUE]    = (gdouble) (curcolor->blue)  / 65535.0;
  scolor[CS_OPACITY] = 1.0;

  gtk_color_selection_set_color(colorsel, &scolor[CS_RED]);
}

void
stream_prefs_fetch(GtkWidget *w _U_)
{
  gdkcolor_to_color_t(&prefs.st_client_fg, &tcolors[CFG_IDX]);
  gdkcolor_to_color_t(&prefs.st_client_bg, &tcolors[CBG_IDX]);
  gdkcolor_to_color_t(&prefs.st_server_fg, &tcolors[SFG_IDX]);
  gdkcolor_to_color_t(&prefs.st_server_bg, &tcolors[SBG_IDX]);
}

/* XXX - "gui_prefs_apply()" handles this, as the "Follow TCP Stream"
   windows may have to be redrawn due to a font change; this means
   that calling "stream_prefs_apply()" without calling "gui_prefs_apply()"
   won't work. */
void
stream_prefs_apply(GtkWidget *w _U_)
{
}

void
stream_prefs_destroy(GtkWidget *w _U_)
{
}
