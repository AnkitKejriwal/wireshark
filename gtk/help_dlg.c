/* help_dlg.c
 *
 * $Id$
 *
 * Laurent Deniel <laurent.deniel@free.fr>
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
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
#include <epan/prefs.h>
#include "gtkglobals.h"
#include "gui_utils.h"
#include "compat_macros.h"
#include "dlg_utils.h"
#include "simple_dialog.h"
#include "webbrowser.h"
#include "file_util.h"

#ifdef HHC_DIR
#include <windows.h>
#include <htmlhelp.h>
#include "epan/unicode-utils.h"
#endif


#define HELP_DIR	"help"


#define NOTEBOOK_KEY    "notebook_key"

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



gboolean topic_available(topic_action_e action) {

    if(action == HELP_CAPTURE_INTERFACES_DETAILS_DIALOG) {
        /* XXX - add the page HELP_CAPTURE_INTERFACES_DETAILS_DIALOG and remove this if */
        /* page currently not existing in user's guide */
        return FALSE;
    }
    /* online: we have almost all possible pages available */
    return TRUE;
}


/*
 * Open the help dialog and show a specific HTML help page.
 */
void help_topic_html(const gchar *topic) {
    GString *url;


    /* try to open local .chm file */
#ifdef HHC_DIR
    HWND hw;

    url = g_string_new("");

    g_string_append_printf(url, "%s\\user-guide.chm::/wsug_chm/%s>Wireshark Help",
        get_datafile_dir(), topic);

    hw = HtmlHelpW(NULL,
        utf_8to16(url->str),
        HH_DISPLAY_TOPIC, 0);

    g_string_free(url, TRUE /* free_segment */);

    /* if the .chm file could be opened, stop here */
    if(hw != NULL) {
        return;
    }
#endif /* HHC_DIR */

    url = g_string_new("");

    /* try to open the HTML page from wireshark.org instead */
    g_string_append_printf(url, "http://www.wireshark.org/docs/wsug_html_chunked/%s", topic);

    browser_open_url(url->str);

    g_string_free(url, TRUE /* free_segment */);
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


static void
topic_action(topic_action_e action)
{
    /* pages online at www.wireshark.org */
    switch(action) {
    case(ONLINEPAGE_HOME):
        browser_open_url ("http://www.wireshark.org");
        break;
    case(ONLINEPAGE_WIKI):
        browser_open_url ("http://wiki.wireshark.org");
        break;
    case(ONLINEPAGE_DOWNLOAD):
        browser_open_url ("http://www.wireshark.org/download/");
        break;
    case(ONLINEPAGE_USERGUIDE):
        browser_open_url ("http://www.wireshark.org/docs/wsug_html_chunked/");
        break;
    case(ONLINEPAGE_FAQ):
        browser_open_url ("http://www.wireshark.org/faq.html");
        break;
    case(ONLINEPAGE_SAMPLE_FILES):
        browser_open_url ("http://wiki.wireshark.org/SampleCaptures");
        break;

    /* local manual pages */
    case(LOCALPAGE_MAN_WIRESHARK):
        browser_open_data_file("wireshark.html");
        break;
    case(LOCALPAGE_MAN_WIRESHARK_FILTER):
        browser_open_data_file("wireshark-filter.html");
        break;
    case(LOCALPAGE_MAN_TSHARK):
        browser_open_data_file("tshark.html");
        break;
    case(LOCALPAGE_MAN_RAWSHARK):
        browser_open_data_file("rawshark.html");
        break;
    case(LOCALPAGE_MAN_DUMPCAP):
        browser_open_data_file("dumpcap.html");
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

    /* local help pages (User's Guide) */
    case(HELP_CONTENT):
        help_topic_html( "index.html");
        break;
    case(HELP_CAPTURE_OPTIONS_DIALOG):
        help_topic_html("ChCapCaptureOptions.html");
        break;
    case(HELP_CAPTURE_FILTERS_DIALOG):
        help_topic_html("ChWorkDefineFilterSection.html");
        break;
    case(HELP_DISPLAY_FILTERS_DIALOG):
        help_topic_html("ChWorkDefineFilterSection.html");
        break;
    case(HELP_COLORING_RULES_DIALOG):
        help_topic_html("ChCustColorizationSection.html");
        break;
    case(HELP_CONFIG_PROFILES_DIALOG):
        help_topic_html("ChCustConfigProfilesSection.html");
        break;
    case(HELP_PRINT_DIALOG):
        help_topic_html("ChIOPrintSection.html");
        break;
    case(HELP_FIND_DIALOG):
        help_topic_html("ChWorkFindPacketSection.html");
        break;
    case(HELP_GOTO_DIALOG):
        help_topic_html("ChWorkGoToPacketSection.html");
        break;
    case(HELP_CAPTURE_INTERFACES_DIALOG):
        help_topic_html("ChCapInterfaceSection.html");
        break;
    case(HELP_CAPTURE_INFO_DIALOG):
        help_topic_html("ChCapRunningSection.html");
        break;
    case(HELP_ENABLED_PROTOCOLS_DIALOG):
        help_topic_html("ChCustProtocolDissectionSection.html");
        break;
    case(HELP_DECODE_AS_DIALOG):
        help_topic_html("ChCustProtocolDissectionSection.html");
        break;
    case(HELP_DECODE_AS_SHOW_DIALOG):
        help_topic_html("ChCustProtocolDissectionSection.html");
        break;
    case(HELP_FOLLOW_TCP_STREAM_DIALOG):
        help_topic_html("ChAdvFollowTCPSection.html");
        break;
    case(HELP_EXPERT_INFO_DIALOG):
        help_topic_html("ChAdvExpert.html");
        break;
    case(HELP_STATS_SUMMARY_DIALOG):
        help_topic_html("ChStatSummary.html");
        break;
    case(HELP_STATS_PROTO_HIERARCHY_DIALOG):
        help_topic_html("ChStatHierarchy.html");
        break;
    case(HELP_STATS_ENDPOINTS_DIALOG):
        help_topic_html("ChStatEndpoints.html");
        break;
    case(HELP_STATS_CONVERSATIONS_DIALOG):
        help_topic_html("ChStatConversations.html");
        break;
    case(HELP_STATS_IO_GRAPH_DIALOG):
        help_topic_html("ChStatIOGraphs.html");
        break;
    case(HELP_STATS_WLAN_TRAFFIC_DIALOG):
        help_topic_html("ChStatWLANTraffic.html");
        break;
    case(HELP_FILESET_DIALOG):
        help_topic_html("ChIOFileSetSection.html");
        break;
    case(HELP_CAPTURE_INTERFACES_DETAILS_DIALOG):
        help_topic_html("ChCapInterfaceDetailsSection.html");
        break;
    case(HELP_PREFERENCES_DIALOG):
        help_topic_html("ChCustPreferencesSection.html");
        break;
    case(HELP_EXPORT_FILE_DIALOG):
    case(HELP_EXPORT_FILE_WIN32_DIALOG):
        help_topic_html("ChIOExportSection.html");
        break;
    case(HELP_EXPORT_BYTES_DIALOG):
    case(HELP_EXPORT_BYTES_WIN32_DIALOG):
        help_topic_html("ChIOExportSection.html#ChIOExportSelectedDialog");
        break;
    case(HELP_EXPORT_OBJECT_LIST):
	help_topic_html("ChIOExportSection.html#ChIOExportObjectsDialog");
	break;
    case(HELP_OPEN_DIALOG):
    case(HELP_OPEN_WIN32_DIALOG):
        help_topic_html("ChIOOpenSection.html");
        break;
    case(HELP_MERGE_DIALOG):
    case(HELP_MERGE_WIN32_DIALOG):
        help_topic_html("ChIOMergeSection.html");
        break;
    case(HELP_SAVE_DIALOG):
    case(HELP_SAVE_WIN32_DIALOG):
        help_topic_html("ChIOSaveSection.html");
        break;

    default:
        g_assert_not_reached();
    }
}


void
topic_cb(GtkWidget *w _U_, topic_action_e action)
{
    topic_action(action);
}

void
topic_menu_cb(GtkWidget *w _U_, gpointer data _U_, topic_action_e action) {
    topic_action(action);
}

