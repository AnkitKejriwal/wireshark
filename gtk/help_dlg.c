/* help_dlg.c
 *
 * $Id$
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
#include <stdio.h>
#include <errno.h>

#include "epan/filesystem.h"
#include "help_dlg.h"
#include "text_page.h"
#include "prefs.h"
#include "gtkglobals.h"
#include "ui_util.h"
#include "compat_macros.h"
#include "dlg_utils.h"
#include "simple_dialog.h"
#include "webbrowser.h"

#define HELP_DIR	"help"


#define NOTEBOOK_KEY    "notebook_key"

static void help_destroy_cb(GtkWidget *w, gpointer data);

/*
 * Keep a static pointer to the current "Help" window, if any, so that
 * if somebody tries to do "Help->Help" while there's already a
 * "Help" window up, we just pop up the existing one, rather than
 * creating a new one.
*/
static GtkWidget *help_w = NULL;

/*
 * Keep a list of text widgets and corresponding file names as well
 * (for text format changes).
 */
typedef struct {
  char *topic;
  char *pathname;
  GtkWidget *page;
} help_page_t;

static GSList *help_text_pages = NULL;


/*
 * Helper function to show a simple help text page.
 */
static GtkWidget * help_page(const char *topic, const char *filename)
{
  GtkWidget *text_page;
  char *relative_path, *absolute_path;
  help_page_t *page;

  relative_path = g_strconcat(HELP_DIR, G_DIR_SEPARATOR_S, filename, NULL);
  absolute_path = get_datafile_path(relative_path);
  text_page = text_page_new(absolute_path);
  g_free(relative_path);
  gtk_widget_show(text_page);

  page = g_malloc(sizeof (help_page_t));
  page->topic = g_strdup(topic);
  page->pathname = absolute_path;
  page->page = text_page;
  help_text_pages = g_slist_append(help_text_pages, page);

  return text_page;
}


/*
 * Create and show help dialog.
 */
void help_cb(GtkWidget *w _U_, gpointer data _U_)
{
  GtkWidget *main_vb, *bbox, *help_nb, *close_bt, *label, *topic_vb;
  char line[4096+1];	/* XXX - size? */
  char *p;
  char *filename;
  char *help_toc_file_path;
  FILE *help_toc_file;

  if (help_w != NULL) {
    /* There's already a "Help" dialog box; reactivate it. */
    reactivate_window(help_w);
    return;
  }

  help_toc_file_path = get_datafile_path(HELP_DIR G_DIR_SEPARATOR_S "toc");
  help_toc_file = fopen(help_toc_file_path, "r");
  if (help_toc_file == NULL) {
    simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK, "Could not open file \"%s\": %s",
                  help_toc_file_path, strerror(errno));
    g_free(help_toc_file_path);
    return;
  }

  help_w = window_new_with_geom(GTK_WINDOW_TOPLEVEL, "Ethereal: Help", "help");
  gtk_window_set_default_size(GTK_WINDOW(help_w), DEF_WIDTH, DEF_HEIGHT);
  gtk_container_border_width(GTK_CONTAINER(help_w), 2);

  /* Container for each row of widgets */
  main_vb = gtk_vbox_new(FALSE, 1);
  gtk_container_border_width(GTK_CONTAINER(main_vb), 1);
  gtk_container_add(GTK_CONTAINER(help_w), main_vb);

  /* help topics container */
  help_nb = gtk_notebook_new();
  gtk_container_add(GTK_CONTAINER(main_vb), help_nb);
  OBJECT_SET_DATA(help_w, NOTEBOOK_KEY, help_nb);

  /* help topics */
  while (fgets(line, sizeof line, help_toc_file) != NULL) {
    /* Strip off line ending. */
    p = strpbrk(line, "\r\n");
    if (p == NULL)
      break;		/* last line has no line ending */
    *p = '\0';
    /* {Topic title}:{filename of help file} */
    p = strchr(line, ':');
    if (p != NULL) {
      *p++ = '\0';
      filename = p;

      /*
       * "line" refers to the topic now, and "filename" refers to the
       * file name.
       */
      topic_vb = help_page(line, filename);
      label = gtk_label_new(line);
      gtk_notebook_append_page(GTK_NOTEBOOK(help_nb), topic_vb, label);
    }
  }
  if(ferror(help_toc_file)) {
    simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK, "Error reading file \"%s\": %s",
                  help_toc_file_path, strerror(errno));
  }
  fclose(help_toc_file);


  /* Button row */
  bbox = dlg_button_row_new(GTK_STOCK_OK, NULL);
  gtk_box_pack_end(GTK_BOX(main_vb), bbox, FALSE, FALSE, 5);

  close_bt = OBJECT_GET_DATA(bbox, GTK_STOCK_OK);
  window_set_cancel_button(help_w, close_bt, window_cancel_button_cb);

  SIGNAL_CONNECT(help_w, "delete_event", window_delete_event_cb, NULL);
  SIGNAL_CONNECT(help_w, "destroy", help_destroy_cb, NULL);

  gtk_quit_add_destroy(gtk_main_level(), GTK_OBJECT(help_w));

  gtk_widget_show_all(help_w);
  window_present(help_w);
} /* help_cb */


/*
 * Open the help dialog and show a specific help page.
 */

void help_topic_cb(GtkWidget *w _U_, gpointer data) {
    gchar       *topic = data;
    gchar       *page_topic;
    GtkWidget   *help_nb;
    GSList      *help_page_ent;
    gint        page_num = 0;
    help_page_t *page;

    /* show help dialog, if not already opened */
    help_cb(NULL, NULL);

    help_nb = OBJECT_GET_DATA(help_w, NOTEBOOK_KEY);

    /* find page to display */
    for (help_page_ent = help_text_pages; help_page_ent != NULL;
         help_page_ent = g_slist_next(help_page_ent))
    {
        page = (help_page_t *)help_page_ent->data;
        page_topic = page->topic;
        if (strcmp (page_topic, topic) == 0) {
            /* topic page found, switch to notebook page */
            gtk_notebook_set_page(GTK_NOTEBOOK(help_nb), page_num);
            return;
        }
        page_num++;
    }

    /* topic page not found, default (first page) will be shown */
}


/*
 * Help dialog is closed now.
 */
static void help_destroy_cb(GtkWidget *w _U_, gpointer data _U_)
{
  GSList *help_page_ent;
  help_page_t *page;

  /* Free up the list of help pages. */
  for (help_page_ent = help_text_pages; help_page_ent != NULL;
       help_page_ent = g_slist_next(help_page_ent)) {
    page = (help_page_t *)help_page_ent->data;
    g_free(page->topic);
    g_free(page->pathname);
    g_free(page);
  }
  g_slist_free(help_text_pages);
  help_text_pages = NULL;

  /* Note that we no longer have a Help window. */
  help_w = NULL;
}


/**
 * Redraw all help pages, to use a new font.
 */
void help_redraw(void)
{
  GSList *help_page_ent;
  help_page_t *help_page;

  if (help_w != NULL) {
    for (help_page_ent = help_text_pages; help_page_ent != NULL;
        help_page_ent = g_slist_next(help_page_ent))
    {
      help_page = (help_page_t *)help_page_ent->data;
      text_page_redraw(help_page->page, help_page->pathname);
    }
  }
}


void
url_page_action(url_page_action_e action)
{
    /* pages online at www.ethereal.com */
    switch(action) {
    case(ONLINEPAGE_HOME):
        browser_open_url ("http://www.ethereal.com");
        break;
    case(ONLINEPAGE_DOWNLOAD):
        browser_open_url ("http://www.ethereal.com/download.html");
        break;
    case(ONLINEPAGE_USERGUIDE):
        browser_open_url ("http://www.ethereal.com/docs/user-guide");
        break;
    case(ONLINEPAGE_FAQ):
        browser_open_url ("http://www.ethereal.com/faq.html");
        break;
    case(ONLINEPAGE_SAMPLE_FILES):
        browser_open_url ("http://www.ethereal.com/sample");
        break;

    /* local manual pages */
    case(LOCALPAGE_MAN_ETHEREAL):
        browser_open_data_file("ethereal.html");
        break;
    case(LOCALPAGE_MAN_ETHEREAL_FILTER):
        browser_open_data_file("ethereal-filter.html");
        break;
    case(LOCALPAGE_MAN_TETHEREAL):
        browser_open_data_file("tethereal.html");
        break;
    case(LOCALPAGE_MAN_MERGECAP):
        browser_open_data_file("mergecap.html");
        break;
    case(LOCALPAGE_MAN_EDITCAP):
        browser_open_data_file("editcap.html");
        break;
    case(LOCALPAGE_MAN_TEXT2PCAP):
        browser_open_data_file("text2pcap.html");
        break;

#ifdef ETHEREAL_EUG_DIR
    /* local help pages (User's Guide) */
    case(HELP_CONTENT):
        browser_open_data_file("eug_html_chunked/index.html");
        break;
    case(HELP_CAPTURE_OPTIONS_DIALOG):
        browser_open_data_file("eug_html_chunked/ChCapCaptureOptions.html");
        break;
    case(HELP_CAPTURE_FILTERS_DIALOG):
        browser_open_data_file("eug_html_chunked/ChWorkDefineFilterSection.html");
        break;
    case(HELP_DISPLAY_FILTERS_DIALOG):
        browser_open_data_file("eug_html_chunked/ChWorkDefineFilterSection.html");
        break;
#endif

    default:
        g_assert_not_reached();
    }
}


void
url_page_cb(GtkWidget *w _U_, url_page_action_e action)
{
    url_page_action(action);
}


void
url_page_menu_cb( GtkWidget *w _U_, gpointer data _U_, url_page_action_e action)
{
    url_page_action(action);
}

