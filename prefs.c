/* prefs.c
 * Routines for handling preferences
 *
 * $Id: prefs.c,v 1.93 2003/01/28 22:27:18 guy Exp $
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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib.h>

#include <epan/filesystem.h>
#include "globals.h"
#include <epan/resolv.h>
#include <epan/packet.h>
#include "file.h"
#include "prefs.h"
#include <epan/proto.h>
#include "column.h"
#include "print.h"

#include "prefs-int.h"

/* Internal functions */
static module_t *find_module(const char *name);
static module_t *prefs_register_module_or_subtree(module_t *parent,
    const char *name, const char *title, gboolean is_subtree,
    void (*apply_cb)(void));
static struct preference *find_preference(module_t *, const char *);
static int    set_pref(gchar*, gchar*);
static GList *get_string_list(gchar *);
static gchar *put_string_list(GList *);
static void   clear_string_list(GList *);
static void   free_col_info(e_prefs *);

#define GPF_NAME	"ethereal.conf"
#define PF_NAME		"preferences"

static gboolean init_prefs = TRUE;
static gchar *gpf_path = NULL;

/*
 * XXX - variables to allow us to attempt to interpret the first
 * "mgcp.{tcp,udp}.port" in a preferences file as
 * "mgcp.{tcp,udp}.gateway_port" and the second as
 * "mgcp.{tcp,udp}.callagent_port".
 */
static int mgcp_tcp_port_count;
static int mgcp_udp_port_count;

e_prefs prefs;

gchar	*gui_ptree_line_style_text[] =
	{ "NONE", "SOLID", "DOTTED", "TABBED", NULL };

gchar	*gui_ptree_expander_style_text[] =
	{ "NONE", "SQUARE", "TRIANGLE", "CIRCULAR", NULL };

gchar	*gui_hex_dump_highlight_style_text[] =
	{ "BOLD", "INVERSE", NULL };

/*
 * List of all modules with preference settings.
 */
static GList *modules;

/*
 * List of all modules that should show up at the top level of the
 * tree in the preference dialog box.
 */
static GList *top_level_modules;

static gint
module_compare_name(gconstpointer p1_arg, gconstpointer p2_arg)
{
	const module_t *p1 = p1_arg;
	const module_t *p2 = p2_arg;

	return g_strcasecmp(p1->name, p2->name);
}

static gint
module_compare_title(gconstpointer p1_arg, gconstpointer p2_arg)
{
	const module_t *p1 = p1_arg;
	const module_t *p2 = p2_arg;

	return g_strcasecmp(p1->title, p2->title);
}

/*
 * Register a module that will have preferences.
 * Specify the module under which to register it or NULL to register it
 * at the top level, the name used for the module in the preferences file,
 * the title used in the tab for it in a preferences dialog box, and a
 * routine to call back when we apply the preferences.
 */
module_t *
prefs_register_module(module_t *parent, const char *name, const char *title,
    void (*apply_cb)(void))
{
	return prefs_register_module_or_subtree(parent, name, title, FALSE,
	    apply_cb);
}

/*
 * Register a subtree that will have modules under it.
 * Specify the module under which to register it or NULL to register it
 * at the top level and the title used in the tab for it in a preferences
 * dialog box.
 */
module_t *
prefs_register_subtree(module_t *parent, const char *title)
{
	return prefs_register_module_or_subtree(parent, NULL, title, TRUE,
	    NULL);
}

static module_t *
prefs_register_module_or_subtree(module_t *parent, const char *name,
    const char *title, gboolean is_subtree, void (*apply_cb)(void))
{
	module_t *module;
	const guchar *p;

	module = g_malloc(sizeof (module_t));
	module->name = name;
	module->title = title;
	module->is_subtree = is_subtree;
	module->apply_cb = apply_cb;
	module->prefs = NULL;	/* no preferences, to start */
	module->numprefs = 0;
	module->prefs_changed = FALSE;
	module->obsolete = FALSE;

	/*
	 * Do we have a module name?
	 */
	if (name != NULL) {
		/*
		 * Yes.
		 * Make sure that only lower-case ASCII letters, numbers,
		 * underscores, and dots appear in the name.
		 *
		 * Crash if there is, as that's an error in the code;
		 * you can make the title a nice string with capitalization,
		 * white space, punctuation, etc., but the name can be used
		 * on the command line, and shouldn't require quoting,
		 * shifting, etc.
		 */
		for (p = name; *p != '\0'; p++)
			g_assert(isascii(*p) &&
			    (islower(*p) || isdigit(*p) || *p == '_' ||
			     *p == '.'));

		/*
		 * Make sure there's not already a module with that
		 * name.  Crash if there is, as that's an error in the
		 * code, and the code has to be fixed not to register
		 * more than one module with the same name.
		 *
		 * We search the list of all modules; the subtree stuff
		 * doesn't require preferences in subtrees to have names
		 * that reflect the subtree they're in (that would require
		 * protocol preferences to have a bogus "protocol.", or
		 * something such as that, to be added to all their names).
		 */
		g_assert(find_module(name) == NULL);

		/*
		 * Insert this module in the list of all modules.
		 */
		modules = g_list_insert_sorted(modules, module,
		    module_compare_name);
	} else {
		/*
		 * This has no name, just a title; check to make sure it's a
		 * subtree, and crash if it's not.
		 */
		g_assert(is_subtree);
	}

	/*
	 * Insert this module into the appropriate place in the display
	 * tree.
	 */
	if (parent == NULL) {
		/*
		 * It goes at the top.
		 */
		top_level_modules = g_list_insert_sorted(top_level_modules,
		    module, module_compare_title);
	} else {
		/*
		 * It goes into the list for this module.
		 */
		parent->prefs = g_list_insert_sorted(parent->prefs, module,
		    module_compare_title);
	}

	return module;
}

/*
 * Register that a protocol has preferences.
 */
module_t *protocols_module;

module_t *
prefs_register_protocol(int id, void (*apply_cb)(void))
{
	/*
	 * Have we yet created the "Protocols" subtree?
	 */
	if (protocols_module == NULL) {
		/*
		 * No.  Do so.
		 */
		protocols_module = prefs_register_subtree(NULL, "Protocols");
	}
	return prefs_register_module(protocols_module,
	    proto_get_protocol_filter_name(id),
	    proto_get_protocol_short_name(id), apply_cb);
}

/*
 * Register that a protocol used to have preferences but no longer does,
 * by creating an "obsolete" module for it.
 */
module_t *
prefs_register_protocol_obsolete(int id)
{
	module_t *module;

	/*
	 * Have we yet created the "Protocols" subtree?
	 */
	if (protocols_module == NULL) {
		/*
		 * No.  Do so.
		 */
		protocols_module = prefs_register_subtree(NULL, "Protocols");
	}
	module = prefs_register_module(protocols_module,
	    proto_get_protocol_filter_name(id),
	    proto_get_protocol_short_name(id), NULL);
	module->obsolete = TRUE;
	return module;
}

/*
 * Find a module, given its name.
 */
static gint
module_match(gconstpointer a, gconstpointer b)
{
	const module_t *module = a;
	const char *name = b;

	return strcmp(name, module->name);
}

static module_t *
find_module(const char *name)
{
	GList *list_entry;

	list_entry = g_list_find_custom(modules, (gpointer)name, module_match);
	if (list_entry == NULL)
		return NULL;	/* no such module */
	return (module_t *) list_entry->data;
}

typedef struct {
	module_cb callback;
	gpointer user_data;
} module_cb_arg_t;

static void
do_module_callback(gpointer data, gpointer user_data)
{
	module_t *module = data;
	module_cb_arg_t *arg = user_data;

	if (!module->obsolete)
		(*arg->callback)(module, arg->user_data);
}

/*
 * Call a callback function, with a specified argument, for each module
 * in a list of modules.  If the list is NULL, searches the top-level
 * list in the display tree of modules.
 *
 * Ignores "obsolete" modules; their sole purpose is to allow old
 * preferences for dissectors that no longer have preferences to be
 * silently ignored in preference files.  Does not ignore subtrees,
 * as this can be used when walking the display tree of modules.
 */
void
prefs_module_list_foreach(GList *module_list, module_cb callback,
    gpointer user_data)
{
	module_cb_arg_t arg;

	if (module_list == NULL)
		module_list = top_level_modules;

	arg.callback = callback;
	arg.user_data = user_data;
	g_list_foreach(module_list, do_module_callback, &arg);
}

/*
 * Call a callback function, with a specified argument, for each module
 * in the list of all modules.  (This list does not include subtrees.)
 *
 * Ignores "obsolete" modules; their sole purpose is to allow old
 * preferences for dissectors that no longer have preferences to be
 * silently ignored in preference files.
 */
void
prefs_modules_foreach(module_cb callback, gpointer user_data)
{
	prefs_module_list_foreach(modules, callback, user_data);
}

static void
call_apply_cb(gpointer data, gpointer user_data _U_)
{
	module_t *module = data;

	if (module->obsolete)
		return;
	if (module->prefs_changed) {
		if (module->apply_cb != NULL)
			(*module->apply_cb)();
		module->prefs_changed = FALSE;
	}
}

/*
 * Call the "apply" callback function for each module if any of its
 * preferences have changed, and then clear the flag saying its
 * preferences have changed, as the module has been notified of that
 * fact.
 */
void
prefs_apply_all(void)
{
	g_list_foreach(modules, call_apply_cb, NULL);
}

/*
 * Register a preference in a module's list of preferences.
 * If it has a title, give it an ordinal number; otherwise, it's a
 * preference that won't show up in the UI, so it shouldn't get an
 * ordinal number (the ordinal should be the ordinal in the set of
 * *visible* preferences).
 */
static pref_t *
register_preference(module_t *module, const char *name, const char *title,
    const char *description)
{
	pref_t *preference;
	const guchar *p;

	preference = g_malloc(sizeof (pref_t));
	preference->name = name;
	preference->title = title;
	preference->description = description;
	if (title != NULL)
		preference->ordinal = module->numprefs;
	else
		preference->ordinal = -1;	/* no ordinal for you */

	/*
	 * Make sure that only lower-case ASCII letters, numbers,
	 * underscores, and dots appear in the preference name.
	 *
	 * Crash if there is, as that's an error in the code;
	 * you can make the title and description nice strings
	 * with capitalization, white space, punctuation, etc.,
	 * but the name can be used on the command line,
	 * and shouldn't require quoting, shifting, etc.
	 */
	for (p = name; *p != '\0'; p++)
		g_assert(isascii(*p) &&
		    (islower(*p) || isdigit(*p) || *p == '_' || *p == '.'));

	/*
	 * Make sure there's not already a preference with that
	 * name.  Crash if there is, as that's an error in the
	 * code, and the code has to be fixed not to register
	 * more than one preference with the same name.
	 */
	g_assert(find_preference(module, name) == NULL);

	/*
	 * There isn't already one with that name, so add the
	 * preference.
	 */
	module->prefs = g_list_append(module->prefs, preference);
	if (title != NULL)
		module->numprefs++;

	return preference;
}

/*
 * Find a preference in a module's list of preferences, given the module
 * and the preference's name.
 */
static gint
preference_match(gconstpointer a, gconstpointer b)
{
	const pref_t *pref = a;
	const char *name = b;

	return strcmp(name, pref->name);
}

static struct preference *
find_preference(module_t *module, const char *name)
{
	GList *list_entry;

	list_entry = g_list_find_custom(module->prefs, (gpointer)name,
	    preference_match);
	if (list_entry == NULL)
		return NULL;	/* no such preference */
	return (struct preference *) list_entry->data;
}

/*
 * Returns TRUE if the given protocol has registered preferences
 */
gboolean
prefs_is_registered_protocol(char *name)
{
	module_t *m = find_module(name);

	return (m != NULL && !m->obsolete);
}

/*
 * Returns the module title of a registered protocol
 */
const char *
prefs_get_title_by_name(char *name)
{
	module_t *m = find_module(name);

	return (m != NULL && !m->obsolete) ? m->title : NULL;
}

/*
 * Register a preference with an unsigned integral value.
 */
void
prefs_register_uint_preference(module_t *module, const char *name,
    const char *title, const char *description, guint base, guint *var)
{
	pref_t *preference;

	preference = register_preference(module, name, title, description);
	preference->type = PREF_UINT;
	preference->varp.uint = var;
	preference->info.base = base;
}

/*
 * Register a preference with an Boolean value.
 */
void
prefs_register_bool_preference(module_t *module, const char *name,
    const char *title, const char *description, gboolean *var)
{
	pref_t *preference;

	preference = register_preference(module, name, title, description);
	preference->type = PREF_BOOL;
	preference->varp.boolp = var;
}

/*
 * Register a preference with an enumerated value.
 */
void
prefs_register_enum_preference(module_t *module, const char *name,
    const char *title, const char *description, gint *var,
    const enum_val_t *enumvals, gboolean radio_buttons)
{
	pref_t *preference;

	preference = register_preference(module, name, title, description);
	preference->type = PREF_ENUM;
	preference->varp.enump = var;
	preference->info.enum_info.enumvals = enumvals;
	preference->info.enum_info.radio_buttons = radio_buttons;
}

/*
 * Register a preference with a character-string value.
 */
void
prefs_register_string_preference(module_t *module, const char *name,
    const char *title, const char *description, char **var)
{
	pref_t *preference;

	preference = register_preference(module, name, title, description);
	preference->type = PREF_STRING;
	preference->varp.string = var;
	preference->saved_val.string = NULL;
}

/*
 * Register a preference that used to be supported but no longer is.
 */
void
prefs_register_obsolete_preference(module_t *module, const char *name)
{
	pref_t *preference;

	preference = register_preference(module, name, NULL, NULL);
	preference->type = PREF_OBSOLETE;
}

typedef struct {
	pref_cb callback;
	gpointer user_data;
} pref_cb_arg_t;

static void
do_pref_callback(gpointer data, gpointer user_data)
{
	pref_t *pref = data;
	pref_cb_arg_t *arg = user_data;

	if (pref->type == PREF_OBSOLETE) {
		/*
		 * This preference is no longer supported; it's not a
		 * real preference, so we don't call the callback for
		 * it (i.e., we treat it as if it weren't found in the
		 * list of preferences, and we weren't called in the
		 * first place).
		 */
		return;
	}

	(*arg->callback)(pref, arg->user_data);
}

/*
 * Call a callback function, with a specified argument, for each preference
 * in a given module.
 */
void
prefs_pref_foreach(module_t *module, pref_cb callback, gpointer user_data)
{
	pref_cb_arg_t arg;

	arg.callback = callback;
	arg.user_data = user_data;
	g_list_foreach(module->prefs, do_pref_callback, &arg);
}

/*
 * Register all non-dissector modules' preferences.
 */
void
prefs_register_modules(void)
{
}

/* Parse through a list of comma-separated, possibly quoted strings.
   Return a list of the string data. */
static GList *
get_string_list(gchar *str)
{
  enum { PRE_STRING, IN_QUOT, NOT_IN_QUOT };

  gint      state = PRE_STRING, i = 0, j = 0;
  gboolean  backslash = FALSE;
  guchar    cur_c;
  gchar    *slstr = NULL;
  GList    *sl = NULL;

  /* Allocate a buffer for the first string.   */
  slstr = (gchar *) g_malloc(sizeof(gchar) * COL_MAX_LEN);
  j = 0;

  for (;;) {
    cur_c = str[i];
    if (cur_c == '\0') {
      /* It's the end of the input, so it's the end of the string we
         were working on, and there's no more input. */
      if (state == IN_QUOT || backslash) {
        /* We were in the middle of a quoted string or backslash escape,
           and ran out of characters; that's an error.  */
        g_free(slstr);
        clear_string_list(sl);
        return NULL;
      }
      slstr[j] = '\0';
      sl = g_list_append(sl, slstr);
      break;
    }
    if (cur_c == '"' && ! backslash) {
      switch (state) {
        case PRE_STRING:
          /* We hadn't yet started processing a string; this starts the
             string, and we're now quoting.  */
          state = IN_QUOT;
          break;
        case IN_QUOT:
          /* We're in the middle of a quoted string, and we saw a quotation
             mark; we're no longer quoting.   */
          state = NOT_IN_QUOT;
          break;
        case NOT_IN_QUOT:
          /* We're working on a string, but haven't seen a quote; we're
             now quoting.  */
          state = IN_QUOT;
          break;
        default:
          break;
      }
    } else if (cur_c == '\\' && ! backslash) {
      /* We saw a backslash, and the previous character wasn't a
         backslash; escape the next character.

         This also means we've started a new string. */
      backslash = TRUE;
      if (state == PRE_STRING)
        state = NOT_IN_QUOT;
    } else if (cur_c == ',' && state != IN_QUOT && ! backslash) {
      /* We saw a comma, and we're not in the middle of a quoted string
         and it wasn't preceded by a backslash; it's the end of
         the string we were working on...  */
      slstr[j] = '\0';
      sl = g_list_append(sl, slstr);

      /* ...and the beginning of a new string.  */
      state = PRE_STRING;
      slstr = (gchar *) g_malloc(sizeof(gchar) * COL_MAX_LEN);
      j = 0;
    } else if (!isspace(cur_c) || state != PRE_STRING) {
      /* Either this isn't a white-space character, or we've started a
         string (i.e., already seen a non-white-space character for that
         string and put it into the string).

         The character is to be put into the string; do so if there's
         room.  */
      if (j < COL_MAX_LEN) {
        slstr[j] = cur_c;
        j++;
      }

      /* If it was backslash-escaped, we're done with the backslash escape.  */
      backslash = FALSE;
    }
    i++;
  }
  return(sl);
}

#define MAX_FMT_PREF_LEN      1024
#define MAX_FMT_PREF_LINE_LEN   60
static gchar *
put_string_list(GList *sl)
{
  static gchar  pref_str[MAX_FMT_PREF_LEN] = "";
  GList        *clp = g_list_first(sl);
  gchar        *str;
  int           cur_pos = 0, cur_len = 0;
  gchar        *quoted_str;
  int           str_len;
  gchar        *strp, *quoted_strp, c;
  int           fmt_len;

  while (clp) {
    str = clp->data;

    /* Allocate a buffer big enough to hold the entire string, with each
       character quoted (that's the worst case).  */
    str_len = strlen(str);
    quoted_str = g_malloc(str_len*2 + 1);

    /* Now quote any " or \ characters in it. */
    strp = str;
    quoted_strp = quoted_str;
    while ((c = *strp++) != '\0') {
      if (c == '"' || c == '\\') {
        /* It has to be backslash-quoted.  */
        *quoted_strp++ = '\\';
      }
      *quoted_strp++ = c;
    }
    *quoted_strp = '\0';

    fmt_len = strlen(quoted_str) + 4;
    if ((fmt_len + cur_len) < (MAX_FMT_PREF_LEN - 1)) {
      if ((fmt_len + cur_pos) > MAX_FMT_PREF_LINE_LEN) {
        /* Wrap the line.  */
        cur_len--;
        cur_pos = 0;
        pref_str[cur_len] = '\n'; cur_len++;
        pref_str[cur_len] = '\t'; cur_len++;
      }
      sprintf(&pref_str[cur_len], "\"%s\", ", quoted_str);
      cur_pos += fmt_len;
      cur_len += fmt_len;
    }
    g_free(quoted_str);
    clp = clp->next;
  }

  /* If the string is at least two characters long, the last two characters
     are ", ", and should be discarded, as there are no more items in the
     string.  */
  if (cur_len >= 2)
    pref_str[cur_len - 2] = '\0';

  return(pref_str);
}

static void
clear_string_list(GList *sl)
{
  GList *l = sl;

  while (l) {
    g_free(l->data);
    l = g_list_remove_link(l, l);
  }
}

/*
 * Takes a string, a pointer to an array of "enum_val_t"s, and a default gint
 * value.
 * The array must be terminated by an entry with a null "name" string.
 * If the string matches a "name" strings in an entry, the value from that
 * entry is returned. Otherwise, the default value that was passed as the
 * third argument is returned.
 */
gint
find_val_for_string(const char *needle, const enum_val_t *haystack,
    gint default_value)
{
	int i = 0;

	while (haystack[i].name != NULL) {
		if (strcasecmp(needle, haystack[i].name) == 0) {
			return haystack[i].value;
		}
		i++;
	}
	return default_value;
}

/* Takes an string and a pointer to an array of strings, and a default int value.
 * The array must be terminated by a NULL string. If the string is found in the array
 * of strings, the index of that string in the array is returned. Otherwise, the
 * default value that was passed as the third argument is returned.
 */
static int
find_index_from_string_array(char *needle, char **haystack, int default_value)
{
	int i = 0;

	while (haystack[i] != NULL) {
		if (strcmp(needle, haystack[i]) == 0) {
			return i;
		}
		i++;
	}
	return default_value;
}

/* Preferences file format:
 * - Configuration directives start at the beginning of the line, and
 *   are terminated with a colon.
 * - Directives can be continued on the next line by preceding them with
 *   whitespace.
 *
 * Example:

# This is a comment line
print.command: lpr
print.file: /a/very/long/path/
	to/ethereal-out.ps
 *
 */

#define MAX_VAR_LEN    48
#define MAX_VAL_LEN  1024

#define DEF_NUM_COLS    6

static void read_prefs_file(const char *pf_path, FILE *pf);

/* Read the preferences file, fill in "prefs", and return a pointer to it.

   If we got an error (other than "it doesn't exist") trying to read
   the global preferences file, stuff the errno into "*gpf_errno_return"
   and a pointer to the path of the file into "*gpf_path_return", and
   return NULL.

   If we got an error (other than "it doesn't exist") trying to read
   the user's preferences file, stuff the errno into "*pf_errno_return"
   and a pointer to the path of the file into "*pf_path_return", and
   return NULL. */
e_prefs *
read_prefs(int *gpf_errno_return, char **gpf_path_return,
	   int *pf_errno_return, char **pf_path_return)
{
  int         i;
  char       *pf_path;
  FILE       *pf;
  fmt_data   *cfmt;
  gchar      *col_fmt[] = {"No.",      "%m", "Time",        "%t",
                           "Source",   "%s", "Destination", "%d",
                           "Protocol", "%p", "Info",        "%i"};

  if (init_prefs) {
    /* Initialize preferences to wired-in default values.
       They may be overridded by the global preferences file or the
       user's preferences file. */
    init_prefs       = FALSE;
    prefs.pr_format  = PR_FMT_TEXT;
    prefs.pr_dest    = PR_DEST_CMD;
    prefs.pr_file    = g_strdup("ethereal.out");
    prefs.pr_cmd     = g_strdup("lpr");
    prefs.col_list = NULL;
    for (i = 0; i < DEF_NUM_COLS; i++) {
      cfmt = (fmt_data *) g_malloc(sizeof(fmt_data));
      cfmt->title = g_strdup(col_fmt[i * 2]);
      cfmt->fmt   = g_strdup(col_fmt[(i * 2) + 1]);
      prefs.col_list = g_list_append(prefs.col_list, cfmt);
    }
    prefs.num_cols  = DEF_NUM_COLS;
    prefs.st_client_fg.pixel =     0;
    prefs.st_client_fg.red   = 32767;
    prefs.st_client_fg.green =     0;
    prefs.st_client_fg.blue  =     0;
    prefs.st_client_bg.pixel = 65535;
    prefs.st_client_bg.red   = 65535;
    prefs.st_client_bg.green = 65535;
    prefs.st_client_bg.blue  = 65535;
    prefs.st_server_fg.pixel =     0;
    prefs.st_server_fg.red   =     0;
    prefs.st_server_fg.green =     0;
    prefs.st_server_fg.blue  = 32767;
    prefs.st_server_bg.pixel = 65535;
    prefs.st_server_bg.red   = 65535;
    prefs.st_server_bg.green = 65535;
    prefs.st_server_bg.blue  = 65535;
    prefs.gui_scrollbar_on_right = TRUE;
    prefs.gui_plist_sel_browse = FALSE;
    prefs.gui_ptree_sel_browse = FALSE;
    prefs.gui_altern_colors = FALSE;
    prefs.gui_ptree_line_style = 0;
    prefs.gui_ptree_expander_style = 1;
    prefs.gui_hex_dump_highlight_style = 1;
#ifdef WIN32
    prefs.gui_font_name = g_strdup("-*-lucida console-medium-r-*-*-*-100-*-*-*-*-*-*");
#else
    /*
     * XXX - for now, we make the initial font name a pattern that matches
     * only ISO 8859/1 fonts, so that we don't match 2-byte fonts such
     * as ISO 10646 fonts.
     *
     * Users in locales using other one-byte fonts will have to choose
     * a different font from the preferences dialog - or put the font
     * selection in the global preferences file to make that font the
     * default for all users who don't explicitly specify a different
     * font.
     *
     * Making this a font set rather than a font has two problems:
     *
     *	1) as far as I know, you can't select font sets with the
     *	   font selection dialog;
     *
     *  2) if you use a font set, the text to be drawn must be a
     *	   multi-byte string in the appropriate locale, but
     *	   Ethereal does *NOT* guarantee that's the case - in
     *	   the hex-dump window, each character in the text portion
     *	   of the display must be a *single* byte, and in the
     *	   packet-list and protocol-tree windows, text extracted
     *	   from the packet is not necessarily in the right format.
     *
     * "Doing this right" may, for the packet-list and protocol-tree
     * windows, require that dissectors know what the locale is
     * *AND* know what locale and text representation is used in
     * the packets they're dissecting, and may be impossible in
     * the hex-dump window (except by punting and displaying only
     * ASCII characters).
     *
     * GTK+ 2.0 may simplify part of the problem, as it will, as I
     * understand it, use UTF-8-encoded Unicode as its internal
     * character set; however, we'd still have to know whatever
     * character set and encoding is used in the packet (which
     * may differ for different protocols, e.g. SMB might use
     * PC code pages for some strings and Unicode for others, whilst
     * NFS might use some UNIX character set encoding, e.g. ISO 8859/x,
     * or one of the EUC character sets for Asian languages, or one
     * of the other multi-byte character sets, or UTF-8, or...).
     *
     * I.e., as far as I can tell, "internationalizing" the packet-list,
     * protocol-tree, and hex-dump windows involves a lot more than, say,
     * just using font sets rather than fonts.
     */
    prefs.gui_font_name = g_strdup("-*-fixed-medium-r-semicondensed-*-*-120-*-*-*-*-iso8859-1");
#endif
    prefs.gui_marked_fg.pixel        =     65535;
    prefs.gui_marked_fg.red          =     65535;
    prefs.gui_marked_fg.green        =     65535;
    prefs.gui_marked_fg.blue         =     65535;
    prefs.gui_marked_bg.pixel        =         0;
    prefs.gui_marked_bg.red          =         0;
    prefs.gui_marked_bg.green        =         0;
    prefs.gui_marked_bg.blue         =         0;
    prefs.gui_geometry_save_position =         0;
    prefs.gui_geometry_save_size     =         1;
    prefs.gui_geometry_main_x        =        20;
    prefs.gui_geometry_main_y        =        20;
    prefs.gui_geometry_main_width    = DEF_WIDTH;
    prefs.gui_geometry_main_height   =        -1;

/* set the default values for the capture dialog box */
    prefs.capture_device      = NULL;
    prefs.capture_prom_mode   = TRUE;
    prefs.capture_real_time   = FALSE;
    prefs.capture_auto_scroll = FALSE;
    prefs.name_resolve        = RESOLV_ALL ^ RESOLV_NETWORK;
  }

  /* Construct the pathname of the global preferences file. */
  if (! gpf_path) {
    gpf_path = (gchar *) g_malloc(strlen(get_datafile_dir()) +
      strlen(GPF_NAME) + 2);
    sprintf(gpf_path, "%s" G_DIR_SEPARATOR_S "%s",
      get_datafile_dir(), GPF_NAME);
  }

  /* Read the global preferences file, if it exists. */
  *gpf_path_return = NULL;
  if ((pf = fopen(gpf_path, "r")) != NULL) {
    /* We succeeded in opening it; read it. */
    read_prefs_file(gpf_path, pf);
    fclose(pf);
  } else {
    /* We failed to open it.  If we failed for some reason other than
       "it doesn't exist", return the errno and the pathname, so our
       caller can report the error. */
    if (errno != ENOENT) {
      *gpf_errno_return = errno;
      *gpf_path_return = gpf_path;
    }
  }

  /* Construct the pathname of the user's preferences file. */
  pf_path = get_persconffile_path(PF_NAME, FALSE);

  /* Read the user's preferences file, if it exists. */
  *pf_path_return = NULL;
  if ((pf = fopen(pf_path, "r")) != NULL) {
    /* We succeeded in opening it; read it. */
    read_prefs_file(pf_path, pf);
    fclose(pf);
    g_free(pf_path);
    pf_path = NULL;
  } else {
    /* We failed to open it.  If we failed for some reason other than
       "it doesn't exist", return the errno and the pathname, so our
       caller can report the error. */
    if (errno != ENOENT) {
      *pf_errno_return = errno;
      *pf_path_return = pf_path;
    }
  }

  return &prefs;
}

static void
read_prefs_file(const char *pf_path, FILE *pf)
{
  enum { START, IN_VAR, PRE_VAL, IN_VAL, IN_SKIP };
  gchar     cur_var[MAX_VAR_LEN], cur_val[MAX_VAL_LEN];
  int       got_c, state = START;
  gboolean  got_val = FALSE;
  gint      var_len = 0, val_len = 0, fline = 1, pline = 1;

  /*
   * Start out the counters of "mgcp.{tcp,udp}.port" entries we've
   * seen.
   */
  mgcp_tcp_port_count = 0;
  mgcp_udp_port_count = 0;

  while ((got_c = getc(pf)) != EOF) {
    if (got_c == '\n') {
      state = START;
      fline++;
      continue;
    }
    if (var_len >= MAX_VAR_LEN) {
      g_warning ("%s line %d: Variable too long", pf_path, fline);
      state = IN_SKIP;
      var_len = 0;
      continue;
    }
    if (val_len >= MAX_VAL_LEN) {
      g_warning ("%s line %d: Value too long", pf_path, fline);
      state = IN_SKIP;
      var_len = 0;
      continue;
    }

    switch (state) {
      case START:
        if (isalnum(got_c)) {
          if (var_len > 0) {
            if (got_val) {
              cur_var[var_len] = '\0';
              cur_val[val_len] = '\0';
              switch (set_pref(cur_var, cur_val)) {

	      case PREFS_SET_SYNTAX_ERR:
                g_warning ("%s line %d: Syntax error", pf_path, pline);
                break;

	      case PREFS_SET_NO_SUCH_PREF:
                g_warning ("%s line %d: No such preference \"%s\"", pf_path,
				pline, cur_var);
                break;

	      case PREFS_SET_OBSOLETE:
	        /* We silently ignore attempts to set these; it's
	           probably not the user's fault that it's in there -
	           they may have saved preferences with a release that
	           supported them. */
                break;
              }
            } else {
              g_warning ("%s line %d: Incomplete preference", pf_path, pline);
            }
          }
          state      = IN_VAR;
          got_val    = FALSE;
          cur_var[0] = got_c;
          var_len    = 1;
          pline = fline;
        } else if (isspace(got_c) && var_len > 0 && got_val) {
          state = PRE_VAL;
        } else if (got_c == '#') {
          state = IN_SKIP;
        } else {
          g_warning ("%s line %d: Malformed line", pf_path, fline);
        }
        break;
      case IN_VAR:
        if (got_c != ':') {
          cur_var[var_len] = got_c;
          var_len++;
        } else {
          state   = PRE_VAL;
          val_len = 0;
          got_val = TRUE;
        }
        break;
      case PRE_VAL:
        if (!isspace(got_c)) {
          state = IN_VAL;
          cur_val[val_len] = got_c;
          val_len++;
        }
        break;
      case IN_VAL:
        if (got_c != '#')  {
          cur_val[val_len] = got_c;
          val_len++;
        } else {
          while (isspace((guchar)cur_val[val_len]) && val_len > 0)
            val_len--;
          state = IN_SKIP;
        }
        break;
    }
  }
  if (var_len > 0) {
    if (got_val) {
      cur_var[var_len] = '\0';
      cur_val[val_len] = '\0';
      switch (set_pref(cur_var, cur_val)) {

      case PREFS_SET_SYNTAX_ERR:
        g_warning ("%s line %d: Syntax error", pf_path, pline);
        break;

      case PREFS_SET_NO_SUCH_PREF:
        g_warning ("%s line %d: No such preference \"%s\"", pf_path,
			pline, cur_var);
        break;

      case PREFS_SET_OBSOLETE:
	/* We silently ignore attempts to set these; it's probably not
	   the user's fault that it's in there - they may have saved
	   preferences with a release that supported it. */
        break;
      }
    } else {
      g_warning ("%s line %d: Incomplete preference", pf_path, pline);
    }
  }
}

/*
 * Given a string of the form "<pref name>:<pref value>", as might appear
 * as an argument to a "-o" option, parse it and set the preference in
 * question.  Return an indication of whether it succeeded or failed
 * in some fashion.
 */
int
prefs_set_pref(char *prefarg)
{
	guchar *p, *colonp;
	int ret;

	/*
	 * Set the counters of "mgcp.{tcp,udp}.port" entries we've
	 * seen to values that keep us from trying to interpret tham
	 * as "mgcp.{tcp,udp}.gateway_port" or "mgcp.{tcp,udp}.callagent_port",
	 * as, from the command line, we have no way of guessing which
	 * the user had in mind.
	 */
	mgcp_tcp_port_count = -1;
	mgcp_udp_port_count = -1;

	colonp = strchr(prefarg, ':');
	if (colonp == NULL)
		return PREFS_SET_SYNTAX_ERR;

	p = colonp;
	*p++ = '\0';

	/*
	 * Skip over any white space (there probably won't be any, but
	 * as we allow it in the preferences file, we might as well
	 * allow it here).
	 */
	while (isspace(*p))
		p++;
	if (*p == '\0') {
		/*
		 * Put the colon back, so if our caller uses, in an
		 * error message, the string they passed us, the message
		 * looks correct.
		 */
		*colonp = ':';
		return PREFS_SET_SYNTAX_ERR;
	}

	ret = set_pref(prefarg, p);
	*colonp = ':';	/* put the colon back */
	return ret;
}

#define PRS_PRINT_FMT    "print.format"
#define PRS_PRINT_DEST   "print.destination"
#define PRS_PRINT_FILE   "print.file"
#define PRS_PRINT_CMD    "print.command"
#define PRS_COL_FMT      "column.format"
#define PRS_STREAM_CL_FG "stream.client.fg"
#define PRS_STREAM_CL_BG "stream.client.bg"
#define PRS_STREAM_SR_FG "stream.server.fg"
#define PRS_STREAM_SR_BG "stream.server.bg"
#define PRS_GUI_SCROLLBAR_ON_RIGHT "gui.scrollbar_on_right"
#define PRS_GUI_PLIST_SEL_BROWSE "gui.packet_list_sel_browse"
#define PRS_GUI_PTREE_SEL_BROWSE "gui.protocol_tree_sel_browse"
#define PRS_GUI_ALTERN_COLORS "gui.tree_view_altern_colors"
#define PRS_GUI_PTREE_LINE_STYLE "gui.protocol_tree_line_style"
#define PRS_GUI_PTREE_EXPANDER_STYLE "gui.protocol_tree_expander_style"
#define PRS_GUI_HEX_DUMP_HIGHLIGHT_STYLE "gui.hex_dump_highlight_style"
#define PRS_GUI_FONT_NAME "gui.font_name"
#define PRS_GUI_MARKED_FG "gui.marked_frame.fg"
#define PRS_GUI_MARKED_BG "gui.marked_frame.bg"
#define PRS_GUI_GEOMETRY_SAVE_POSITION "gui.geometry.save.position"
#define PRS_GUI_GEOMETRY_SAVE_SIZE     "gui.geometry.save.size"
#define PRS_GUI_GEOMETRY_MAIN_X        "gui.geometry.main.x"
#define PRS_GUI_GEOMETRY_MAIN_Y        "gui.geometry.main.y"
#define PRS_GUI_GEOMETRY_MAIN_WIDTH    "gui.geometry.main.width"
#define PRS_GUI_GEOMETRY_MAIN_HEIGHT   "gui.geometry.main.height"

/*
 * This applies to more than just captures, so it's not "capture.name_resolve";
 * "capture.name_resolve" is supported on input for backwards compatibility.
 *
 * It's not a preference for a particular part of Ethereal, it's used all
 * over the place, so its name doesn't have two components.
 */
#define PRS_NAME_RESOLVE "name_resolve"
#define PRS_CAP_NAME_RESOLVE "capture.name_resolve"

/*  values for the capture dialog box */
#define PRS_CAP_DEVICE      "capture.device"
#define PRS_CAP_PROM_MODE   "capture.prom_mode"
#define PRS_CAP_REAL_TIME   "capture.real_time_update"
#define PRS_CAP_AUTO_SCROLL "capture.auto_scroll"

#define RED_COMPONENT(x)   ((((x) >> 16) & 0xff) * 65535 / 255)
#define GREEN_COMPONENT(x) ((((x) >>  8) & 0xff) * 65535 / 255)
#define BLUE_COMPONENT(x)   (((x)        & 0xff) * 65535 / 255)

static gchar *pr_formats[] = { "text", "postscript" };
static gchar *pr_dests[]   = { "command", "file" };

typedef struct {
  char    letter;
  guint32 value;
} name_resolve_opt_t;

static name_resolve_opt_t name_resolve_opt[] = {
  { 'm', RESOLV_MAC },
  { 'n', RESOLV_NETWORK },
  { 't', RESOLV_TRANSPORT },
};

#define N_NAME_RESOLVE_OPT	(sizeof name_resolve_opt / sizeof name_resolve_opt[0])

static char *
name_resolve_to_string(guint32 name_resolve)
{
  static char string[N_NAME_RESOLVE_OPT+1];
  char *p;
  unsigned int i;
  gboolean all_opts_set = TRUE;

  if (name_resolve == RESOLV_NONE)
    return "FALSE";
  p = &string[0];
  for (i = 0; i < N_NAME_RESOLVE_OPT; i++) {
    if (name_resolve & name_resolve_opt[i].value)
      *p++ =  name_resolve_opt[i].letter;
    else
      all_opts_set = FALSE;
  }
  *p = '\0';
  if (all_opts_set)
    return "TRUE";
  return string;
}

char
string_to_name_resolve(char *string, guint32 *name_resolve)
{
  char c;
  unsigned int i;

  *name_resolve = 0;
  while ((c = *string++) != '\0') {
    for (i = 0; i < N_NAME_RESOLVE_OPT; i++) {
      if (c == name_resolve_opt[i].letter) {
        *name_resolve |= name_resolve_opt[i].value;
        break;
      }
    }
    if (i == N_NAME_RESOLVE_OPT) {
      /*
       * Unrecognized letter.
       */
      return c;
    }
  }
  return '\0';
}

static int
set_pref(gchar *pref_name, gchar *value)
{
  GList    *col_l, *col_l_elt;
  gint      llen;
  fmt_data *cfmt;
  unsigned long int cval;
  guint    uval;
  gboolean bval;
  gint     enum_val;
  char     *p;
  gchar    *dotp, *last_dotp;
  module_t *module;
  pref_t   *pref;
  gboolean had_a_dot;

  if (strcmp(pref_name, PRS_PRINT_FMT) == 0) {
    if (strcmp(value, pr_formats[PR_FMT_TEXT]) == 0) {
      prefs.pr_format = PR_FMT_TEXT;
    } else if (strcmp(value, pr_formats[PR_FMT_PS]) == 0) {
      prefs.pr_format = PR_FMT_PS;
    } else {
      return PREFS_SET_SYNTAX_ERR;
    }
  } else if (strcmp(pref_name, PRS_PRINT_DEST) == 0) {
    if (strcmp(value, pr_dests[PR_DEST_CMD]) == 0) {
      prefs.pr_dest = PR_DEST_CMD;
    } else if (strcmp(value, pr_dests[PR_DEST_FILE]) == 0) {
      prefs.pr_dest = PR_DEST_FILE;
    } else {
      return PREFS_SET_SYNTAX_ERR;
    }
  } else if (strcmp(pref_name, PRS_PRINT_FILE) == 0) {
    if (prefs.pr_file) g_free(prefs.pr_file);
    prefs.pr_file = g_strdup(value);
  } else if (strcmp(pref_name, PRS_PRINT_CMD) == 0) {
    if (prefs.pr_cmd) g_free(prefs.pr_cmd);
    prefs.pr_cmd = g_strdup(value);
  } else if (strcmp(pref_name, PRS_COL_FMT) == 0) {
    col_l = get_string_list(value);
    if (col_l == NULL)
      return PREFS_SET_SYNTAX_ERR;
    if ((g_list_length(col_l) % 2) != 0) {
      /* A title didn't have a matching format.  */
      clear_string_list(col_l);
      return PREFS_SET_SYNTAX_ERR;
    }
    /* Check to make sure all column formats are valid.  */
    col_l_elt = g_list_first(col_l);
    while(col_l_elt) {
      /* Make sure the title isn't empty.  */
      if (strcmp(col_l_elt->data, "") == 0) {
      	/* It is.  */
        clear_string_list(col_l);
        return PREFS_SET_SYNTAX_ERR;
      }

      /* Go past the title.  */
      col_l_elt = col_l_elt->next;

      /* Check the format.  */
      if (get_column_format_from_str(col_l_elt->data) == -1) {
        /* It's not a valid column format.  */
        clear_string_list(col_l);
        return PREFS_SET_SYNTAX_ERR;
      }

      /* Go past the format.  */
      col_l_elt = col_l_elt->next;
    }
    free_col_info(&prefs);
    prefs.col_list = NULL;
    llen             = g_list_length(col_l);
    prefs.num_cols   = llen / 2;
    col_l_elt = g_list_first(col_l);
    while(col_l_elt) {
      cfmt = (fmt_data *) g_malloc(sizeof(fmt_data));
      cfmt->title    = g_strdup(col_l_elt->data);
      col_l_elt      = col_l_elt->next;
      cfmt->fmt      = g_strdup(col_l_elt->data);
      col_l_elt      = col_l_elt->next;
      prefs.col_list = g_list_append(prefs.col_list, cfmt);
    }
    clear_string_list(col_l);
  } else if (strcmp(pref_name, PRS_STREAM_CL_FG) == 0) {
    cval = strtoul(value, NULL, 16);
    prefs.st_client_fg.pixel = 0;
    prefs.st_client_fg.red   = RED_COMPONENT(cval);
    prefs.st_client_fg.green = GREEN_COMPONENT(cval);
    prefs.st_client_fg.blue  = BLUE_COMPONENT(cval);
  } else if (strcmp(pref_name, PRS_STREAM_CL_BG) == 0) {
    cval = strtoul(value, NULL, 16);
    prefs.st_client_bg.pixel = 0;
    prefs.st_client_bg.red   = RED_COMPONENT(cval);
    prefs.st_client_bg.green = GREEN_COMPONENT(cval);
    prefs.st_client_bg.blue  = BLUE_COMPONENT(cval);
  } else if (strcmp(pref_name, PRS_STREAM_SR_FG) == 0) {
    cval = strtoul(value, NULL, 16);
    prefs.st_server_fg.pixel = 0;
    prefs.st_server_fg.red   = RED_COMPONENT(cval);
    prefs.st_server_fg.green = GREEN_COMPONENT(cval);
    prefs.st_server_fg.blue  = BLUE_COMPONENT(cval);
  } else if (strcmp(pref_name, PRS_STREAM_SR_BG) == 0) {
    cval = strtoul(value, NULL, 16);
    prefs.st_server_bg.pixel = 0;
    prefs.st_server_bg.red   = RED_COMPONENT(cval);
    prefs.st_server_bg.green = GREEN_COMPONENT(cval);
    prefs.st_server_bg.blue  = BLUE_COMPONENT(cval);
  } else if (strcmp(pref_name, PRS_GUI_SCROLLBAR_ON_RIGHT) == 0) {
    if (strcasecmp(value, "true") == 0) {
	    prefs.gui_scrollbar_on_right = TRUE;
    }
    else {
	    prefs.gui_scrollbar_on_right = FALSE;
    }
  } else if (strcmp(pref_name, PRS_GUI_PLIST_SEL_BROWSE) == 0) {
    if (strcasecmp(value, "true") == 0) {
	    prefs.gui_plist_sel_browse = TRUE;
    }
    else {
	    prefs.gui_plist_sel_browse = FALSE;
    }
  } else if (strcmp(pref_name, PRS_GUI_PTREE_SEL_BROWSE) == 0) {
    if (strcasecmp(value, "true") == 0) {
	    prefs.gui_ptree_sel_browse = TRUE;
    }
    else {
	    prefs.gui_ptree_sel_browse = FALSE;
    }
  } else if (strcmp(pref_name, PRS_GUI_ALTERN_COLORS) == 0) {
    if (strcasecmp(value, "true") == 0) {
            prefs.gui_altern_colors = TRUE;
    }
    else {
            prefs.gui_altern_colors = FALSE;
    }
  } else if (strcmp(pref_name, PRS_GUI_PTREE_LINE_STYLE) == 0) {
	  prefs.gui_ptree_line_style =
		  find_index_from_string_array(value, gui_ptree_line_style_text, 0);
  } else if (strcmp(pref_name, PRS_GUI_PTREE_EXPANDER_STYLE) == 0) {
	  prefs.gui_ptree_expander_style =
		  find_index_from_string_array(value, gui_ptree_expander_style_text, 1);
  } else if (strcmp(pref_name, PRS_GUI_HEX_DUMP_HIGHLIGHT_STYLE) == 0) {
	  prefs.gui_hex_dump_highlight_style =
		  find_index_from_string_array(value, gui_hex_dump_highlight_style_text, 1);
  } else if (strcmp(pref_name, PRS_GUI_FONT_NAME) == 0) {
	  if (prefs.gui_font_name != NULL)
		g_free(prefs.gui_font_name);
	  prefs.gui_font_name = g_strdup(value);
  } else if (strcmp(pref_name, PRS_GUI_MARKED_FG) == 0) {
    cval = strtoul(value, NULL, 16);
    prefs.gui_marked_fg.pixel = 0;
    prefs.gui_marked_fg.red   = RED_COMPONENT(cval);
    prefs.gui_marked_fg.green = GREEN_COMPONENT(cval);
    prefs.gui_marked_fg.blue  = BLUE_COMPONENT(cval);
  } else if (strcmp(pref_name, PRS_GUI_MARKED_BG) == 0) {
    cval = strtoul(value, NULL, 16);
    prefs.gui_marked_bg.pixel = 0;
    prefs.gui_marked_bg.red   = RED_COMPONENT(cval);
    prefs.gui_marked_bg.green = GREEN_COMPONENT(cval);
    prefs.gui_marked_bg.blue  = BLUE_COMPONENT(cval);
  } else if (strcmp(pref_name, PRS_GUI_GEOMETRY_SAVE_POSITION) == 0) {
    if (strcasecmp(value, "true") == 0) {
	    prefs.gui_geometry_save_position = TRUE;
    }
    else {
	    prefs.gui_geometry_save_position = FALSE;
    }
  } else if (strcmp(pref_name, PRS_GUI_GEOMETRY_SAVE_SIZE) == 0) {
    if (strcasecmp(value, "true") == 0) {
	    prefs.gui_geometry_save_size = TRUE;
    }
    else {
	    prefs.gui_geometry_save_size = FALSE;
    }
  } else if (strcmp(pref_name, PRS_GUI_GEOMETRY_MAIN_X) == 0) {
    prefs.gui_geometry_main_x = strtol(value, NULL, 10);
  } else if (strcmp(pref_name, PRS_GUI_GEOMETRY_MAIN_Y) == 0) {
    prefs.gui_geometry_main_y = strtol(value, NULL, 10);
  } else if (strcmp(pref_name, PRS_GUI_GEOMETRY_MAIN_WIDTH) == 0) {
    prefs.gui_geometry_main_width = strtol(value, NULL, 10);
  } else if (strcmp(pref_name, PRS_GUI_GEOMETRY_MAIN_HEIGHT) == 0) {
    prefs.gui_geometry_main_height = strtol(value, NULL, 10);

/* handle the capture options */
  } else if (strcmp(pref_name, PRS_CAP_DEVICE) == 0) {
    if (prefs.capture_device != NULL)
      g_free(prefs.capture_device);
    prefs.capture_device = g_strdup(value);
  } else if (strcmp(pref_name, PRS_CAP_PROM_MODE) == 0) {
    prefs.capture_prom_mode = ((strcasecmp(value, "true") == 0)?TRUE:FALSE);
  } else if (strcmp(pref_name, PRS_CAP_REAL_TIME) == 0) {
    prefs.capture_real_time = ((strcasecmp(value, "true") == 0)?TRUE:FALSE);
  } else if (strcmp(pref_name, PRS_CAP_AUTO_SCROLL) == 0) {
    prefs.capture_auto_scroll = ((strcasecmp(value, "true") == 0)?TRUE:FALSE);

/* handle the global options */
  } else if (strcmp(pref_name, PRS_NAME_RESOLVE) == 0 ||
	     strcmp(pref_name, PRS_CAP_NAME_RESOLVE) == 0) {
    /*
     * "TRUE" and "FALSE", for backwards compatibility, are synonyms for
     * RESOLV_ALL and RESOLV_NONE.
     *
     * Otherwise, we treat it as a list of name types we want to resolve.
     */
    if (strcasecmp(value, "true") == 0)
      prefs.name_resolve = RESOLV_ALL;
    else if (strcasecmp(value, "false") == 0)
      prefs.name_resolve = RESOLV_NONE;
    else {
      prefs.name_resolve = RESOLV_NONE;	/* start out with none set */
      if (string_to_name_resolve(value, &prefs.name_resolve) != '\0')
        return PREFS_SET_SYNTAX_ERR;
    }
  } else {
    /* To which module does this preference belong? */
    module = NULL;
    last_dotp = pref_name;
    had_a_dot = FALSE;
    while (!module) {
        dotp = strchr(last_dotp, '.');
        if (dotp == NULL) {
            if (had_a_dot) {
              /* no such module */
              return PREFS_SET_NO_SUCH_PREF;
            }
            else {
              /* no ".", so no module/name separator */
              return PREFS_SET_SYNTAX_ERR;
            }
        }
        else {
            had_a_dot = TRUE;
        }
        *dotp = '\0';		/* separate module and preference name */
        module = find_module(pref_name);

        /*
         * XXX - "Diameter" rather than "diameter" was used in earlier
         * versions of Ethereal; if we didn't find the module, and its name
         * was "Diameter", look for "diameter" instead.
         *
         * In addition, the BEEP protocol used to be the BXXP protocol,
         * so if we didn't find the module, and its name was "bxxp",
         * look for "beep" instead.
         *
         * Also, the preferences for GTP v0 and v1 were combined under
         * a single "gtp" heading.
         */
        if (module == NULL) {
          if (strcmp(pref_name, "Diameter") == 0)
            module = find_module("diameter");
          else if (strcmp(pref_name, "bxxp") == 0)
            module = find_module("beep");
          else if (strcmp(pref_name, "gtpv0") == 0 ||
                   strcmp(pref_name, "gtpv1") == 0)
            module = find_module("gtp");
        }
        *dotp = '.';		/* put the preference string back */
        dotp++;			/* skip past separator to preference name */
        last_dotp = dotp;
    }

    pref = find_preference(module, dotp);

    if (pref == NULL) {
      if (strncmp(pref_name, "mgcp.", 5) == 0) {
        /*
         * XXX - "mgcp.display raw text toggle" and "mgcp.display dissect tree"
         * rather than "mgcp.display_raw_text" and "mgcp.display_dissect_tree"
         * were used in earlier versions of Ethereal; if we didn't find the
         * preference, it was an MGCP preference, and its name was
         * "display raw text toggle" or "display dissect tree", look for
         * "display_raw_text" or "display_dissect_tree" instead.
         *
         * "mgcp.tcp.port" and "mgcp.udp.port" are harder to handle, as both
         * the gateway and callagent ports were given those names; we interpret
         * the first as "mgcp.{tcp,udp}.gateway_port" and the second as
         * "mgcp.{tcp,udp}.callagent_port", as that's the order in which
         * they were registered by the MCCP dissector and thus that's the
         * order in which they were written to the preferences file.  (If
         * we're not reading the preferences file, but are handling stuff
         * from a "-o" command-line option, we have no clue which the user
         * had in mind - they should have used "mgcp.{tcp,udp}.gateway_port"
         * or "mgcp.{tcp,udp}.callagent_port" instead.)
         */
        if (strcmp(dotp, "display raw text toggle") == 0)
          pref = find_preference(module, "display_raw_text");
        else if (strcmp(dotp, "display dissect tree") == 0)
          pref = find_preference(module, "display_dissect_tree");
        else if (strcmp(dotp, "tcp.port") == 0) {
          mgcp_tcp_port_count++;
          if (mgcp_tcp_port_count == 1) {
            /* It's the first one */
            pref = find_preference(module, "tcp.gateway_port");
 	  } else if (mgcp_tcp_port_count == 2) {
            /* It's the second one */
            pref = find_preference(module, "tcp.callagent_port");
	  }
          /* Otherwise it's from the command line, and we don't bother
             mapping it. */
	} else if (strcmp(dotp, "udp.port") == 0) {
          mgcp_udp_port_count++;
          if (mgcp_udp_port_count == 1) {
            /* It's the first one */
            pref = find_preference(module, "udp.gateway_port");
	  } else if (mgcp_udp_port_count == 2) {
            /* It's the second one */
            pref = find_preference(module, "udp.callagent_port");
	  }
          /* Otherwise it's from the command line, and we don't bother
             mapping it. */
	}
      } else if (strncmp(pref_name, "smb.", 4) == 0) {
        /* Handle old names for SMB preferences. */
        if (strcmp(dotp, "smb.trans.reassembly") == 0)
          pref = find_preference(module, "trans_reassembly");
        else if (strcmp(dotp, "smb.dcerpc.reassembly") == 0)
          pref = find_preference(module, "dcerpc_reassembly");
      } else if (strncmp(pref_name, "ndmp.", 5) == 0) {
        /* Handle old names for NDMP preferences. */
        if (strcmp(dotp, "ndmp.desegment") == 0)
          pref = find_preference(module, "desegment");
      } else if (strncmp(pref_name, "diameter.", 9) == 0) {
        /* Handle old names for Diameter preferences. */
        if (strcmp(dotp, "diameter.desegment") == 0)
          pref = find_preference(module, "desegment");
      } else if (strncmp(pref_name, "pcli.", 5) == 0) {
        /* Handle old names for PCLI preferences. */
        if (strcmp(dotp, "pcli.udp_port") == 0)
          pref = find_preference(module, "udp_port");
      }
    }
    if (pref == NULL)
      return PREFS_SET_NO_SUCH_PREF;	/* no such preference */

    switch (pref->type) {

    case PREF_UINT:
      uval = strtoul(value, &p, pref->info.base);
      if (p == value || *p != '\0')
        return PREFS_SET_SYNTAX_ERR;	/* number was bad */
      if (*pref->varp.uint != uval) {
        module->prefs_changed = TRUE;
        *pref->varp.uint = uval;
      }
      break;

    case PREF_BOOL:
      /* XXX - give an error if it's neither "true" nor "false"? */
      if (strcasecmp(value, "true") == 0)
        bval = TRUE;
      else
        bval = FALSE;
      if (*pref->varp.boolp != bval) {
	module->prefs_changed = TRUE;
	*pref->varp.boolp = bval;
      }
      break;

    case PREF_ENUM:
      /* XXX - give an error if it doesn't match? */
      enum_val = find_val_for_string(value,
					pref->info.enum_info.enumvals, 1);
      if (*pref->varp.enump != enum_val) {
	module->prefs_changed = TRUE;
	*pref->varp.enump = enum_val;
      }
      break;

    case PREF_STRING:
      if (*pref->varp.string == NULL || strcmp(*pref->varp.string, value) != 0) {
        module->prefs_changed = TRUE;
        if (*pref->varp.string != NULL)
          g_free(*pref->varp.string);
        *pref->varp.string = g_strdup(value);
      }
      break;

    case PREF_OBSOLETE:
      return PREFS_SET_OBSOLETE;	/* no such preference any more */
    }
  }

  return PREFS_SET_OK;
}

typedef struct {
	module_t *module;
	FILE	*pf;
} write_pref_arg_t;

/*
 * Write out a single preference.
 */
static void
write_pref(gpointer data, gpointer user_data)
{
	pref_t *pref = data;
	write_pref_arg_t *arg = user_data;
	const enum_val_t *enum_valp;
	const char *val_string;

	if (pref->type == PREF_OBSOLETE) {
		/*
		 * This preference is no longer supported; it's not a
		 * real preference, so we don't write it out (i.e., we
		 * treat it as if it weren't found in the list of
		 * preferences, and we weren't called in the first place).
		 */
		return;
	}

	fprintf(arg->pf, "\n# %s\n", pref->description);

	switch (pref->type) {

	case PREF_UINT:
		switch (pref->info.base) {

		case 10:
			fprintf(arg->pf, "# A decimal number.\n");
			fprintf(arg->pf, "%s.%s: %u\n", arg->module->name,
			    pref->name, *pref->varp.uint);
			break;

		case 8:
			fprintf(arg->pf, "# An octal number.\n");
			fprintf(arg->pf, "%s.%s: %#o\n", arg->module->name,
			    pref->name, *pref->varp.uint);
			break;

		case 16:
			fprintf(arg->pf, "# A hexadecimal number.\n");
			fprintf(arg->pf, "%s.%s: %#x\n", arg->module->name,
			    pref->name, *pref->varp.uint);
			break;
		}
		break;

	case PREF_BOOL:
		fprintf(arg->pf, "# TRUE or FALSE (case-insensitive).\n");
		fprintf(arg->pf, "%s.%s: %s\n", arg->module->name, pref->name,
		    *pref->varp.boolp ? "TRUE" : "FALSE");
		break;

	case PREF_ENUM:
		fprintf(arg->pf, "# One of: ");
		enum_valp = pref->info.enum_info.enumvals;
		val_string = NULL;
		while (enum_valp->name != NULL) {
			if (enum_valp->value == *pref->varp.enump)
				val_string = enum_valp->name;
			fprintf(arg->pf, "%s", enum_valp->name);
			enum_valp++;
			if (enum_valp->name == NULL)
				fprintf(arg->pf, "\n");
			else
				fprintf(arg->pf, ", ");
		}
		fprintf(arg->pf, "# (case-insensitive).\n");
		fprintf(arg->pf, "%s.%s: %s\n", arg->module->name, pref->name,
		    val_string);
		break;

	case PREF_STRING:
		fprintf(arg->pf, "# A string.\n");
		fprintf(arg->pf, "%s.%s: %s\n", arg->module->name, pref->name,
		    *pref->varp.string);
		break;

	case PREF_OBSOLETE:
		g_assert_not_reached();
		break;
	}
}

static void
write_module_prefs(gpointer data, gpointer user_data)
{
	write_pref_arg_t arg;

	arg.module = data;
	arg.pf = user_data;
	g_list_foreach(arg.module->prefs, write_pref, &arg);
}

/* Write out "prefs" to the user's preferences file, and return 0.

   If we got an error, stuff a pointer to the path of the preferences file
   into "*pf_path_return", and return the errno. */
int
write_prefs(char **pf_path_return)
{
  char        *pf_path;
  FILE        *pf;
  GList       *clp, *col_l;
  fmt_data    *cfmt;

  /* To do:
   * - Split output lines longer than MAX_VAL_LEN
   * - Create a function for the preference directory check/creation
   *   so that duplication can be avoided with filter.c
   */

  pf_path = get_persconffile_path(PF_NAME, TRUE);
  if ((pf = fopen(pf_path, "w")) == NULL) {
    *pf_path_return = pf_path;
    return errno;
  }

  fputs("# Configuration file for Ethereal " VERSION ".\n"
    "#\n"
    "# This file is regenerated each time preferences are saved within\n"
    "# Ethereal.  Making manual changes should be safe, however.\n"
    "\n"
    "######## Printing ########\n"
    "\n", pf);

  fprintf (pf, "# Can be one of \"text\" or \"postscript\".\n"
    "print.format: %s\n\n", pr_formats[prefs.pr_format]);

  fprintf (pf, "# Can be one of \"command\" or \"file\".\n"
    "print.destination: %s\n\n", pr_dests[prefs.pr_dest]);

  fprintf (pf, "# This is the file that gets written to when the "
    "destination is set to \"file\"\n"
    "%s: %s\n\n", PRS_PRINT_FILE, prefs.pr_file);

  fprintf (pf, "# Output gets piped to this command when the destination "
    "is set to \"command\"\n"
    "%s: %s\n\n", PRS_PRINT_CMD, prefs.pr_cmd);

  clp = prefs.col_list;
  col_l = NULL;
  while (clp) {
    cfmt = (fmt_data *) clp->data;
    col_l = g_list_append(col_l, cfmt->title);
    col_l = g_list_append(col_l, cfmt->fmt);
    clp = clp->next;
  }
  fprintf (pf, "# Packet list column format.  Each pair of strings consists "
    "of a column title \n# and its format.\n"
    "%s: %s\n\n", PRS_COL_FMT, put_string_list(col_l));
  /* This frees the list of strings, but not the strings to which it
     refers; that's what we want, as we haven't copied those strings,
     we just referred to them.  */
  g_list_free(col_l);

  fprintf (pf, "# TCP stream window color preferences.  Each value is a six "
    "digit hexadecimal value in the form rrggbb.\n");
  fprintf (pf, "%s: %02x%02x%02x\n", PRS_STREAM_CL_FG,
    (prefs.st_client_fg.red * 255 / 65535),
    (prefs.st_client_fg.green * 255 / 65535),
    (prefs.st_client_fg.blue * 255 / 65535));
  fprintf (pf, "%s: %02x%02x%02x\n", PRS_STREAM_CL_BG,
    (prefs.st_client_bg.red * 255 / 65535),
    (prefs.st_client_bg.green * 255 / 65535),
    (prefs.st_client_bg.blue * 255 / 65535));
  fprintf (pf, "%s: %02x%02x%02x\n", PRS_STREAM_SR_FG,
    (prefs.st_server_fg.red * 255 / 65535),
    (prefs.st_server_fg.green * 255 / 65535),
    (prefs.st_server_fg.blue * 255 / 65535));
  fprintf (pf, "%s: %02x%02x%02x\n", PRS_STREAM_SR_BG,
    (prefs.st_server_bg.red * 255 / 65535),
    (prefs.st_server_bg.green * 255 / 65535),
    (prefs.st_server_bg.blue * 255 / 65535));

  fprintf(pf, "\n# Vertical scrollbars should be on right side? TRUE/FALSE\n");
  fprintf(pf, PRS_GUI_SCROLLBAR_ON_RIGHT ": %s\n",
		  prefs.gui_scrollbar_on_right == TRUE ? "TRUE" : "FALSE");

  fprintf(pf, "\n# Packet-list selection bar can be used to browse w/o selecting? TRUE/FALSE\n");
  fprintf(pf, PRS_GUI_PLIST_SEL_BROWSE ": %s\n",
		  prefs.gui_plist_sel_browse == TRUE ? "TRUE" : "FALSE");

  fprintf(pf, "\n# Protocol-tree selection bar can be used to browse w/o selecting? TRUE/FALSE\n");
  fprintf(pf, PRS_GUI_PTREE_SEL_BROWSE ": %s\n",
		  prefs.gui_ptree_sel_browse == TRUE ? "TRUE" : "FALSE");

  fprintf(pf, "\n# Alternating colors in TreeViews\n");
  fprintf(pf, PRS_GUI_ALTERN_COLORS ": %s\n",
		  prefs.gui_altern_colors == TRUE ? "TRUE" : "FALSE");

  fprintf(pf, "\n# Protocol-tree line style. One of: NONE, SOLID, DOTTED, TABBED\n");
  fprintf(pf, PRS_GUI_PTREE_LINE_STYLE ": %s\n",
          gui_ptree_line_style_text[prefs.gui_ptree_line_style]);

  fprintf(pf, "\n# Protocol-tree expander style. One of: NONE, SQUARE, TRIANGLE, CIRCULAR\n");
  fprintf(pf, PRS_GUI_PTREE_EXPANDER_STYLE ": %s\n",
		  gui_ptree_expander_style_text[prefs.gui_ptree_expander_style]);

  fprintf(pf, "\n# Hex dump highlight style. One of: BOLD, INVERSE\n");
  fprintf(pf, PRS_GUI_HEX_DUMP_HIGHLIGHT_STYLE ": %s\n",
		  gui_hex_dump_highlight_style_text[prefs.gui_hex_dump_highlight_style]);

  fprintf(pf, "\n# Font name for packet list, protocol tree, and hex dump panes.\n");
  fprintf(pf, PRS_GUI_FONT_NAME ": %s\n", prefs.gui_font_name);

  fprintf (pf, "\n# Color preferences for a marked frame.  Each value is a six "
    "digit hexadecimal value in the form rrggbb.\n");
  fprintf (pf, "%s: %02x%02x%02x\n", PRS_GUI_MARKED_FG,
    (prefs.gui_marked_fg.red * 255 / 65535),
    (prefs.gui_marked_fg.green * 255 / 65535),
    (prefs.gui_marked_fg.blue * 255 / 65535));
  fprintf (pf, "%s: %02x%02x%02x\n", PRS_GUI_MARKED_BG,
    (prefs.gui_marked_bg.red * 255 / 65535),
    (prefs.gui_marked_bg.green * 255 / 65535),
    (prefs.gui_marked_bg.blue * 255 / 65535));

  fprintf(pf, "\n# Save window position at exit? TRUE/FALSE\n");
  fprintf(pf, PRS_GUI_GEOMETRY_SAVE_POSITION ": %s\n",
		  prefs.gui_geometry_save_position == TRUE ? "TRUE" : "FALSE");

  fprintf(pf, "\n# Save window size at exit? TRUE/FALSE\n");
  fprintf(pf, PRS_GUI_GEOMETRY_SAVE_SIZE ": %s\n",
		  prefs.gui_geometry_save_size == TRUE ? "TRUE" : "FALSE");

  fprintf(pf, "\n# Main window geometry. Decimal integers.\n");
  fprintf(pf, PRS_GUI_GEOMETRY_MAIN_X ": %d\n", prefs.gui_geometry_main_x);
  fprintf(pf, PRS_GUI_GEOMETRY_MAIN_Y ": %d\n", prefs.gui_geometry_main_y);
  fprintf(pf, PRS_GUI_GEOMETRY_MAIN_WIDTH ": %d\n",
  		  prefs.gui_geometry_main_width);
  fprintf(pf, PRS_GUI_GEOMETRY_MAIN_HEIGHT ": %d\n",
  		  prefs.gui_geometry_main_height);

  fprintf(pf, "\n# Resolve addresses to names? TRUE/FALSE/{list of address types to resolve}\n");
  fprintf(pf, PRS_NAME_RESOLVE ": %s\n",
		  name_resolve_to_string(prefs.name_resolve));

/* write the capture options */
  if (prefs.capture_device != NULL) {
    fprintf(pf, "\n# Default capture device\n");
    fprintf(pf, PRS_CAP_DEVICE ": %s\n", prefs.capture_device);
  }

  fprintf(pf, "\n# Capture in promiscuous mode? TRUE/FALSE\n");
  fprintf(pf, PRS_CAP_PROM_MODE ": %s\n",
		  prefs.capture_prom_mode == TRUE ? "TRUE" : "FALSE");

  fprintf(pf, "\n# Update packet list in real time during capture? TRUE/FALSE\n");
  fprintf(pf, PRS_CAP_REAL_TIME ": %s\n",
		  prefs.capture_real_time == TRUE ? "TRUE" : "FALSE");

  fprintf(pf, "\n# scroll packet list during capture? TRUE/FALSE\n");
  fprintf(pf, PRS_CAP_AUTO_SCROLL ": %s\n",
		  prefs.capture_auto_scroll == TRUE ? "TRUE" : "FALSE");

  g_list_foreach(modules, write_module_prefs, pf);

  fclose(pf);

  /* XXX - catch I/O errors (e.g. "ran out of disk space") and return
     an error indication, or maybe write to a new preferences file and
     rename that file on top of the old one only if there are not I/O
     errors. */
  return 0;
}

/* Copy a set of preferences. */
void
copy_prefs(e_prefs *dest, e_prefs *src)
{
  fmt_data *src_cfmt, *dest_cfmt;
  GList *entry;

  dest->pr_format = src->pr_format;
  dest->pr_dest = src->pr_dest;
  dest->pr_file = g_strdup(src->pr_file);
  dest->pr_cmd = g_strdup(src->pr_cmd);
  dest->col_list = NULL;
  for (entry = src->col_list; entry != NULL; entry = g_list_next(entry)) {
    src_cfmt = entry->data;
    dest_cfmt = (fmt_data *) g_malloc(sizeof(fmt_data));
    dest_cfmt->title = g_strdup(src_cfmt->title);
    dest_cfmt->fmt = g_strdup(src_cfmt->fmt);
    dest->col_list = g_list_append(dest->col_list, dest_cfmt);
  }
  dest->num_cols = src->num_cols;
  dest->st_client_fg = src->st_client_fg;
  dest->st_client_bg = src->st_client_bg;
  dest->st_server_fg = src->st_server_fg;
  dest->st_server_bg = src->st_server_bg;
  dest->gui_scrollbar_on_right = src->gui_scrollbar_on_right;
  dest->gui_plist_sel_browse = src->gui_plist_sel_browse;
  dest->gui_ptree_sel_browse = src->gui_ptree_sel_browse;
  dest->gui_altern_colors = src->gui_altern_colors;
  dest->gui_ptree_line_style = src->gui_ptree_line_style;
  dest->gui_ptree_expander_style = src->gui_ptree_expander_style;
  dest->gui_hex_dump_highlight_style = src->gui_hex_dump_highlight_style;
  dest->gui_font_name = g_strdup(src->gui_font_name);
  dest->gui_marked_fg = src->gui_marked_fg;
  dest->gui_marked_bg = src->gui_marked_bg;
  dest->gui_geometry_save_position = src->gui_geometry_save_position;
  dest->gui_geometry_save_size = src->gui_geometry_save_size;
  dest->gui_geometry_main_x = src->gui_geometry_main_x;
  dest->gui_geometry_main_y = src->gui_geometry_main_y;
  dest->gui_geometry_main_width = src->gui_geometry_main_width;
  dest->gui_geometry_main_height = src->gui_geometry_main_height;
/*  values for the capture dialog box */
  dest->capture_device = g_strdup(src->capture_device);
  dest->capture_prom_mode = src->capture_prom_mode;
  dest->capture_real_time = src->capture_real_time;
  dest->capture_auto_scroll = src->capture_auto_scroll;
  dest->name_resolve = src->name_resolve;

}

/* Free a set of preferences. */
void
free_prefs(e_prefs *pr)
{
  if (pr->pr_file != NULL) {
    g_free(pr->pr_file);
    pr->pr_file = NULL;
  }
  if (pr->pr_cmd != NULL) {
    g_free(pr->pr_cmd);
    pr->pr_cmd = NULL;
  }
  free_col_info(pr);
  if (pr->gui_font_name != NULL) {
    g_free(pr->gui_font_name);
    pr->gui_font_name = NULL;
  }
  if (pr->capture_device != NULL) {
    g_free(pr->capture_device);
    pr->capture_device = NULL;
  }
}

static void
free_col_info(e_prefs *pr)
{
  fmt_data *cfmt;

  while (pr->col_list != NULL) {
    cfmt = pr->col_list->data;
    g_free(cfmt->title);
    g_free(cfmt->fmt);
    g_free(cfmt);
    pr->col_list = g_list_remove_link(pr->col_list, pr->col_list);
  }
  g_list_free(pr->col_list);
  pr->col_list = NULL;
}
