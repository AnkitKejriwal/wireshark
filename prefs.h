/* prefs.h
 * Definitions for preference handling routines
 *
 * $Id: prefs.h,v 1.40 2002/08/28 21:00:40 jmayer Exp $
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

#ifndef __PREFS_H__
#define __PREFS_H__

#include <glib.h>

#include "color.h"

#define PR_DEST_CMD  0
#define PR_DEST_FILE 1

#define DEF_WIDTH 750
#define DEF_HEIGHT 550

/*
 * Convert a string listing name resolution types to a bitmask of
 * those types.
 *
 * Set "*name_resolve" to the bitmask, and return '\0', on success;
 * return the bad character in the string on error.
 */
char string_to_name_resolve(char *string, guint32 *name_resolve);

typedef struct _e_prefs {
  gint     pr_format;
  gint     pr_dest;
  gchar   *pr_file;
  gchar   *pr_cmd;
  GList   *col_list;
  gint     num_cols;
  color_t  st_client_fg, st_client_bg, st_server_fg, st_server_bg;
  gboolean gui_scrollbar_on_right;
  gboolean gui_plist_sel_browse;
  gboolean gui_ptree_sel_browse;
  gint     gui_ptree_line_style;
  gint     gui_ptree_expander_style;
  gboolean gui_hex_dump_highlight_style;
  gchar   *gui_font_name;
  color_t  gui_marked_fg;
  color_t  gui_marked_bg;
  gboolean gui_geometry_save_position;
  gboolean gui_geometry_save_size;
  gint     gui_geometry_main_x;
  gint     gui_geometry_main_y;
  gint     gui_geometry_main_width;
  gint     gui_geometry_main_height;
  guint32  name_resolve;
  gchar   *capture_device;
  gboolean capture_prom_mode;
  gboolean capture_real_time;
  gboolean capture_auto_scroll;
} e_prefs;

extern e_prefs prefs;

/*
 * Routines to let modules that have preference settings register
 * themselves by name, and to let them register preference settings
 * by name.
 */
struct pref_module;

typedef struct pref_module module_t;

/*
 * Register a module that will have preferences.
 * Specify the name used for the module in the preferences file, the
 * title used in the tab for it in a preferences dialog box, and a
 * routine to call back when we apply the preferences.
 * Note:
 * In case of dissectors, the specified name should be the protocol
 * name specified at the proto_register_protocol() call in order to
 * make the "Protocol Properties..." menu item work.
 */
extern module_t *prefs_register_module(const char *name, const char *title,
    void (*apply_cb)(void));

typedef void (*module_cb)(module_t *module, gpointer user_data);

/*
 * Register that a protocol has preferences.
 */
extern module_t *prefs_register_protocol(int id, void (*apply_cb)(void));

/*
 * Register that a protocol used to have preferences but no longer does,
 * by creating an "obsolete" module for it.
 */
extern module_t *prefs_register_protocol_obsolete(int id);

/*
 * Call a callback function, with a specified argument, for each module.
 * Ignores "obsolete" modules; their sole purpose is to allow old
 * preferences for dissectors that no longer have preferences to be
 * silently ignored in preference files.
 */
extern void prefs_module_foreach(module_cb callback, gpointer user_data);

/*
 * Call the "apply" callback function for each module if any of its
 * preferences have changed, and then clear the flag saying its
 * preferences have changed, as the module has been notified of that
 * fact.
 */
extern void prefs_apply_all(void);

struct preference;

typedef struct preference pref_t;

/*
 * Returns TRUE if the given protocol has registered preferences.
 */
extern gboolean prefs_is_registered_protocol(char *name);

/*
 * Returns the module title of a registered protocol (or NULL if unknown).
 */
extern const char *prefs_get_title_by_name(char *name);

/*
 * Register a preference with an unsigned integral value.
 */
extern void prefs_register_uint_preference(module_t *module, const char *name,
    const char *title, const char *description, guint base, guint *var);

/*
 * Register a preference with an Boolean value.
 */
extern void prefs_register_bool_preference(module_t *module, const char *name,
    const char *title, const char *description, gboolean *var);

/*
 * Register a preference with an enumerated value.
 */
typedef struct {
	char	*name;
	gint	value;
} enum_val_t;

extern void prefs_register_enum_preference(module_t *module, const char *name,
    const char *title, const char *description, gint *var,
    const enum_val_t *enumvals, gboolean radio_buttons);

/*
 * Register a preference with a character-string value.
 */
extern void prefs_register_string_preference(module_t *module, const char *name,
    const char *title, const char *description, char **var);

/*
 * Register a preference that used to be supported but no longer is.
 */
extern void prefs_register_obsolete_preference(module_t *module,
    const char *name);

typedef void (*pref_cb)(pref_t *pref, gpointer user_data);

/*
 * Call a callback function, with a specified argument, for each preference
 * in a given module.
 */
extern void prefs_pref_foreach(module_t *module, pref_cb callback,
    gpointer user_data);

/*
 * Register all non-dissector modules' preferences.
 */
extern void prefs_register_modules(void);

/* Read the preferences file, fill in "prefs", and return a pointer to it.

   If we got an error (other than "it doesn't exist") trying to read
   the global preferences file, stuff the errno into "*gpf_errno_return"
   and a pointer to the path of the file into "*gpf_path_return", and
   return NULL.

   If we got an error (other than "it doesn't exist") trying to read
   the user's preferences file, stuff the errno into "*pf_errno_return"
   and a pointer to the path of the file into "*pf_path_return", and
   return NULL. */
extern e_prefs *read_prefs(int *, char **, int *, char **);

/* Write out "prefs" to the user's preferences file, and return 0.

   If we got an error, stuff a pointer to the path of the preferences file
   into "*pf_path_return", and return the errno. */
extern int write_prefs(char **);

/* Copy a set of preferences. */
extern void copy_prefs(e_prefs *dest, e_prefs *src);

/* Free a set of preferences. */
extern void free_prefs(e_prefs *pr);

/*
 * Given a string of the form "<pref name>:<pref value>", as might appear
 * as an argument to a "-o" option, parse it and set the preference in
 * question.  Return an indication of whether it succeeded or failed
 * in some fashion.
 *
 * XXX - should supply, for syntax errors, a detailed explanation of
 * the syntax error.
 */
#define PREFS_SET_OK		0	/* succeeded */
#define PREFS_SET_SYNTAX_ERR	1	/* syntax error in string */
#define PREFS_SET_NO_SUCH_PREF	2	/* no such preference */
#define PREFS_SET_OBSOLETE	3	/* preference used to exist but no longer does */

extern int prefs_set_pref(char *prefarg);

#endif /* prefs.h */
