/* prefs-int.h
 * Definitions for implementation of preference handling routines;
 * used by "friends" of the preferences type.
 *
 * $Id: prefs-int.h,v 1.10 2002/12/20 01:48:54 guy Exp $
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

#ifndef __PREFS_INT_H__
#define __PREFS_INT_H__

struct pref_module {
	const char *name;	/* name of module */
	const char *title;	/* title of module (displayed in preferences notebook) */
	gboolean is_subtree;	/* if TRUE, this has other modules, not preferences, under it */
	void (*apply_cb)(void);	/* routine to call when preferences applied */
	GList	*prefs;		/* list of its preferences or submodules */
	int	numprefs;	/* number of non-obsolete preferences */
	gboolean prefs_changed;	/* if TRUE, a preference has changed since we last checked */
	gboolean obsolete;	/* if TRUE, this is a module that used to
				   exist but no longer does */
};

/*
 * Module used for protocol preferences.
 */
extern module_t *protocols_module;

/*
 * PREF_OBSOLETE is used for preferences that a module used to support
 * but no longer supports; we give different error messages for them.
 */
typedef enum {
	PREF_UINT,
	PREF_BOOL,
	PREF_ENUM,
	PREF_STRING,
	PREF_OBSOLETE
} pref_type_t;

struct preference {
	const char *name;	/* name of preference */
	const char *title;	/* title to use in GUI */
	const char *description; /* human-readable description of preference */
	int	ordinal;	/* ordinal number of this preference */
	pref_type_t type;	/* type of that preference */
	union {
		guint *uint;
		gboolean *boolp;
		gint *enump;
		char **string;
	} varp;			/* pointer to variable storing the value */
	union {
		guint uint;
		gboolean boolval;
		gint enumval;
		char *string;
	} saved_val;		/* original value, when editing from the GUI */
	union {
	  guint base;			/* input/output base, for PREF_UINT */
	  struct {
	    const enum_val_t *enumvals;	/* list of name & values */
	    gboolean radio_buttons;	/* TRUE if it should be shown as
					   radio buttons rather than as an
					   option menu or combo box in
					   the preferences tab */
	  } enum_info;			/* for PREF_ENUM */
	} info;			/* display/text file information */
	void	*control;	/* handle for GUI control for this preference */
};

gint find_val_for_string(const char *needle, const enum_val_t *haystack,
    gint default_value);

#endif /* prefs-int.h */
