/* recent.c
 * Recent "preference" handling routines
 * Copyright 2004, Ulf Lamping <ulf.lamping@web.de>
 *
 * $Id: recent.c,v 1.2 2004/01/19 01:00:58 guy Exp $
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
#include <stdio.h>
#include <string.h>

#include "recent.h"
#include <epan/epan.h>
#include <epan/filesystem.h>
#include "menu.h"
#include "main.h"
#include "prefs.h"
#include "prefs-int.h"

#define RECENT_FILE_NAME "recent"

/* #include "../menu.h" */
extern void add_menu_recent_capture_file(gchar *file);
extern void menu_recent_read_finished(void);


recent_settings_t recent;


/* Write out "recent" to the user's recent file, and return 0.
   If we got an error, stuff a pointer to the path of the recent file
   into "*pf_path_return", and return the errno. */
int
write_recent(char **rf_path_return)
{
  char        *rf_path;
  FILE        *rf;

  /* To do:
   * - Split output lines longer than MAX_VAL_LEN
   * - Create a function for the preference directory check/creation
   *   so that duplication can be avoided with filter.c
   */

  rf_path = get_persconffile_path(RECENT_FILE_NAME, TRUE);
  if ((rf = fopen(rf_path, "w")) == NULL) {
    *rf_path_return = rf_path;
    return errno;
  }

  fputs("# Recent settings file for Ethereal " VERSION ".\n"
    "#\n"
    "# This file is regenerated each time Ethereal is quit.\n"
    "# So be careful, if you want to make manual changes here.\n"
    "\n"
    "######## Recent capture files (latest first) ########\n"
    "\n", rf);

  menu_recent_file_write_all(rf);

  fputs("\n"
    "######## Recent display filters (latest last) ########\n"
    "\n", rf);

  dfilter_recent_combo_write_all(rf);

  fprintf(rf, "\n# Main Toolbar show (hide).\n");
  fprintf(rf, "# TRUE or FALSE (case-insensitive).\n");
  fprintf(rf, RECENT_KEY_MAIN_TOOLBAR_SHOW ": %s\n",
		  recent.main_toolbar_show == TRUE ? "TRUE" : "FALSE");

  fprintf(rf, "\n# Filter Toolbar show (hide).\n");
  fprintf(rf, "# TRUE or FALSE (case-insensitive).\n");
  fprintf(rf, RECENT_KEY_FILTER_TOOLBAR_SHOW ": %s\n",
		  recent.filter_toolbar_show == TRUE ? "TRUE" : "FALSE");

  fprintf(rf, "\n# Packet list show (hide).\n");
  fprintf(rf, "# TRUE or FALSE (case-insensitive).\n");
  fprintf(rf, RECENT_KEY_PACKET_LIST_SHOW ": %s\n",
		  recent.packet_list_show == TRUE ? "TRUE" : "FALSE");

  fprintf(rf, "\n# Tree view show (hide).\n");
  fprintf(rf, "# TRUE or FALSE (case-insensitive).\n");
  fprintf(rf, RECENT_KEY_TREE_VIEW_SHOW ": %s\n",
		  recent.tree_view_show == TRUE ? "TRUE" : "FALSE");

  fprintf(rf, "\n# Byte view show (hide).\n");
  fprintf(rf, "# TRUE or FALSE (case-insensitive).\n");
  fprintf(rf, RECENT_KEY_BYTE_VIEW_SHOW ": %s\n",
		  recent.byte_view_show == TRUE ? "TRUE" : "FALSE");

  fprintf(rf, "\n# Statusbar show (hide).\n");
  fprintf(rf, "# TRUE or FALSE (case-insensitive).\n");
  fprintf(rf, RECENT_KEY_STATUSBAR_SHOW ": %s\n",
		  recent.statusbar_show == TRUE ? "TRUE" : "FALSE");

  fclose(rf);

  /* XXX - catch I/O errors (e.g. "ran out of disk space") and return
     an error indication, or maybe write to a new preferences file and
     rename that file on top of the old one only if there are not I/O
     errors. */
  return 0;
}


/* set one user's recent file key/value pair */
static int
read_set_recent_pair(gchar *key, gchar *value)
{

  if (strcmp(key, RECENT_KEY_CAPTURE_FILE) == 0) {
	add_menu_recent_capture_file(value);
  } else if (strcmp(key, RECENT_KEY_DISPLAY_FILTER) == 0) {
	dfilter_combo_add_recent(value);
  } else if (strcmp(key, RECENT_KEY_MAIN_TOOLBAR_SHOW) == 0) {
    if (strcasecmp(value, "true") == 0) {
        recent.main_toolbar_show = TRUE;
    }
    else {
        recent.main_toolbar_show = FALSE;
    }
  } else if (strcmp(key, RECENT_KEY_FILTER_TOOLBAR_SHOW) == 0) {
    if (strcasecmp(value, "true") == 0) {
        recent.filter_toolbar_show = TRUE;
    }
    else {
        recent.filter_toolbar_show = FALSE;
    }
  } else if (strcmp(key, RECENT_KEY_PACKET_LIST_SHOW) == 0) {
    if (strcasecmp(value, "true") == 0) {
        recent.packet_list_show = TRUE;
    }
    else {
        recent.packet_list_show = FALSE;
    }
  } else if (strcmp(key, RECENT_KEY_TREE_VIEW_SHOW) == 0) {
    if (strcasecmp(value, "true") == 0) {
        recent.tree_view_show = TRUE;
    }
    else {
        recent.tree_view_show = FALSE;
    }
  } else if (strcmp(key, RECENT_KEY_BYTE_VIEW_SHOW) == 0) {
    if (strcasecmp(value, "true") == 0) {
        recent.byte_view_show = TRUE;
    }
    else {
        recent.byte_view_show = FALSE;
    }
  } else if (strcmp(key, RECENT_KEY_STATUSBAR_SHOW) == 0) {
    if (strcasecmp(value, "true") == 0) {
        recent.statusbar_show = TRUE;
    }
    else {
        recent.statusbar_show = FALSE;
    }
  }

  return PREFS_SET_OK;
}


/* opens the user's recent file and read it out */
void
read_recent(char **rf_path_return, int *rf_errno_return)
{
  char       *rf_path;
  FILE       *rf;


  /* set defaults */
  recent.main_toolbar_show  = TRUE;
  recent.filter_toolbar_show= TRUE;
  recent.packet_list_show   = TRUE;
  recent.tree_view_show     = TRUE;
  recent.byte_view_show     = TRUE;
  recent.statusbar_show     = TRUE;

  /* Construct the pathname of the user's recent file. */
  rf_path = get_persconffile_path(RECENT_FILE_NAME, FALSE);

  /* Read the user's recent file, if it exists. */
  *rf_path_return = NULL;
  if ((rf = fopen(rf_path, "r")) != NULL) {
    /* We succeeded in opening it; read it. */
    read_prefs_file(rf_path, rf, read_set_recent_pair);
	/* set dfilter combobox to have one empty line at the current position */
	dfilter_combo_add_recent("");
    fclose(rf);
    g_free(rf_path);
    rf_path = NULL;
  } else {
    /* We failed to open it.  If we failed for some reason other than
       "it doesn't exist", return the errno and the pathname, so our
       caller can report the error. */
    if (errno != ENOENT) {
      *rf_errno_return = errno;
      *rf_path_return = rf_path;
    }
  }

  menu_recent_read_finished();
}


