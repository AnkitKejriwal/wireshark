/* plugins.c
 * plugin routines
 *
 * $Id$
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@ethereal.com>
 * Copyright 1999 Gerald Combs
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

#ifdef NEED_SNPRINTF_H
# include "snprintf.h"
#endif

#include "plugins.h"

#ifdef HAVE_PLUGINS

#include <time.h>

#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif

#ifdef HAVE_DIRECT_H
#include <direct.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <errno.h>

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "filesystem.h"

#ifdef PLUGINS_NEED_ADDRESS_TABLE
#include "conversation.h"
#include "reassemble.h"
#include <epan/prefs.h>
#include <epan/dissectors/packet-giop.h>
#include <epan/dissectors/packet-tpkt.h>
#include <epan/dissectors/packet-tcp.h>
#include <epan/dissectors/packet-rpc.h>
#include <epan/tap.h>
#include "asn1.h"
#include <epan/dissectors/packet-per.h>
#include <epan/dissectors/packet-ber.h>
#include <epan/dissectors/packet-rtp.h>
#include <epan/dissectors/packet-rtcp.h>
#include <epan/xdlc.h>
#include <epan/crc16.h>
#include "report_err.h"
#include "plugins/plugin_table.h"
static plugin_address_table_t	patable = {
/* file generated by plugin_gen.py */
#include "plugins/Xass-list"
};
#endif

/* linked list of all plugins */
plugin *plugin_list;

#define PLUGINS_DIR_NAME	"plugins"

/*
 * add a new plugin to the list
 * returns :
 * - 0 : OK
 * - ENOMEM : memory allocation problem
 * - EEXIST : the same plugin (i.e. name/version) was already registered.
 */
static int
add_plugin(void *handle, gchar *name, gchar *version,
	   void (*reg_handoff)(void))
{
    plugin *new_plug, *pt_plug;

    pt_plug = plugin_list;
    if (!pt_plug) /* the list is empty */
    {
	new_plug = (plugin *)g_malloc(sizeof(plugin));
	if (new_plug == NULL) return ENOMEM;
        plugin_list = new_plug;
    }
    else
    {
	while (1)
	{
	    /* check if the same name/version is already registered */
	    if (!strcmp(pt_plug->name, name) &&
		!strcmp(pt_plug->version, version))
	    {
		return EEXIST;
	    }

	    /* we found the last plugin in the list */
	    if (pt_plug->next == NULL) break;

	    pt_plug = pt_plug->next;
	}
	new_plug = (plugin *)g_malloc(sizeof(plugin));
	if (new_plug == NULL) return ENOMEM;
	pt_plug->next = new_plug;
    }

    new_plug->handle = handle;
    new_plug->name = name;
    new_plug->version = version;
    new_plug->reg_handoff = reg_handoff;
    new_plug->next = NULL;
    return 0;
}

/*
 * XXX - when we remove support for old-style plugins (which we should
 * probably do eventually, as all plugins should be written as new-style
 * ones), we may want to have "init_plugins()" merely save a pointer
 * to the plugin's "init" routine, just as we save a pointer to its
 * "reg_handoff" routine, and have a "register_all_plugins()" routine
 * to go through the list of plugins and call all of them.
 *
 * Then we'd have "epan_init()", or perhaps even something higher up
 * in the call tree, call "init_plugins()", and have "proto_init()"
 * call "register_all_plugins()" right after calling "register_all_protocols()";
 * this might be a bit cleaner.
 */
static void
plugins_scan_dir(const char *dirname)
{
#define FILENAME_LEN	1024
#if GLIB_MAJOR_VERSION < 2
    gchar         *hack_path;       /* pathname used to construct lt_lib_ext */
    gchar         *lt_lib_ext;      /* extension for loadable modules */
    DIR           *dir;             /* scanned directory */
    struct dirent *file;            /* current file */
    gchar         *name;
#else /* GLIB 2 */
    GDir          *dir;             /* scanned directory */
    GError        **dummy;
    const gchar   *name;
#endif
    gchar          filename[FILENAME_LEN];   /* current file name */
    GModule       *handle;          /* handle returned by dlopen */
    gchar         *version;
    void         (*init)(void *);
    void         (*reg_handoff)(void);
    gchar         *dot;
    int            cr;

#if GLIB_MAJOR_VERSION < 2
    /*
     * We find the extension used on this platform for loadable modules
     * by the sneaky hack of calling "g_module_build_path" to build
     * the pathname for a module with an empty directory name and
     * empty module name, and then search for the last "." and use
     * everything from the last "." on.
     */
    hack_path = g_module_build_path("", "");
    lt_lib_ext = strrchr(hack_path, '.');
    if (lt_lib_ext == NULL)
    {
	/*
	 * Does this mean there *is* no extension?  Assume so.
	 *
	 * XXX - the code below assumes that all loadable modules have
	 * an extension....
	 */
	lt_lib_ext = "";
    }

    if ((dir = opendir(dirname)) != NULL)
    {
	while ((file = readdir(dir)) != NULL)
	{
	    /* don't try to open "." and ".." */
	    if (!(strcmp(file->d_name, "..") &&
		  strcmp(file->d_name, "."))) continue;

            /* skip anything but files with lt_lib_ext */
            dot = strrchr(file->d_name, '.');
            if (dot == NULL || strcmp(dot, lt_lib_ext) != 0) continue;

	    snprintf(filename, FILENAME_LEN, "%s" G_DIR_SEPARATOR_S "%s",
	        dirname, file->d_name);
	    name = (gchar *)file->d_name;
#else /* GLIB 2 */
    /*
     * GLib 2.x defines G_MODULE_SUFFIX as the extension used on this
     * platform for loadable modules.
     */
    dummy = g_malloc(sizeof(GError *));
    *dummy = NULL;
    if ((dir = g_dir_open(dirname, 0, dummy)) != NULL)
    {
    	while ((name = g_dir_read_name(dir)) != NULL)
	{
	    /* skip anything but files with G_MODULE_SUFFIX */
            dot = strrchr(name, '.');
            if (dot == NULL || strcmp(dot+1, G_MODULE_SUFFIX) != 0) continue;

	    snprintf(filename, FILENAME_LEN, "%s" G_DIR_SEPARATOR_S "%s",
	        dirname, name);
#endif
	    if ((handle = g_module_open(filename, 0)) == NULL)
	    {
		g_warning("Couldn't load module %s: %s", filename,
			  g_module_error());
		continue;
	    }
	    if (g_module_symbol(handle, "version", (gpointer*)&version) == FALSE)
	    {
	        g_warning("The plugin %s has no version symbol", name);
		g_module_close(handle);
		continue;
	    }

	    /*
	     * Old-style dissectors don't have a "plugin_reg_handoff()"
	     * routine; we no longer support them.
	     *
	     * New-style dissectors have one, because, otherwise, there's
	     * no way for them to arrange that they ever be called.
	     */
	    if (g_module_symbol(handle, "plugin_reg_handoff",
					 (gpointer*)&reg_handoff))
	    {
		/*
		 * We require it to have a "plugin_init()" routine.
		 */
		if (!g_module_symbol(handle, "plugin_init", (gpointer*)&init))
		{
		    g_warning("The plugin %s has a plugin_reg_handoff symbol but no plugin_init routine",
			      name);
		    g_module_close(handle);
		    continue;
		}

		/*
		 * We have a "plugin_reg_handoff()" routine, so we don't
		 * need the protocol, filter string, or dissector pointer.
		 */
		if ((cr = add_plugin(handle, g_strdup(name), version,
				     reg_handoff)))
		{
		    if (cr == EEXIST)
			fprintf(stderr, "The plugin %s, version %s\n"
			    "was found in multiple directories\n", name, version);
		    else
			fprintf(stderr, "Memory allocation problem\n"
			    "when processing plugin %s, version %s\n",
			    name, version);
		    g_module_close(handle);
		    continue;
		}

		/*
		 * Call its init routine.
		 */
#ifdef PLUGINS_NEED_ADDRESS_TABLE
		init(&patable);
#else
		init(NULL);
#endif
	    }
	    else
	    {
		/*
		 * This is an old-style dissector; warn that it won't
		 * be used, as those aren't supported.
		 */
		fprintf(stderr,
		    "The plugin %s, version %s is an old-style plugin;\n"
		    "Those are no longer supported.\n", name, version);
	    }
	}
#if GLIB_MAJOR_VERSION < 2
	closedir(dir);
    }
    g_free(hack_path);
#else /* GLIB 2 */
	g_dir_close(dir);
    }
    g_clear_error(dummy);
    g_free(dummy);
#endif
}


/* get the global plugin dir */
/* Return value is malloced so the caller should g_free() it. */
const char *get_plugins_global_dir(const char *plugin_dir)
{
#ifdef _WIN32
	char *install_plugin_dir;

	/*
	 * On Windows, the data file directory is the installation
	 * directory; the plugins are stored under it.
	 *
	 * Assume we're running the installed version of Ethereal;
	 * on Windows, the data file directory is the directory
	 * in which the Ethereal binary resides.
	 */
	install_plugin_dir = g_strdup_printf("%s\\plugins\\%s", get_datafile_dir(), VERSION);

	/*
	 * Make sure that pathname refers to a directory.
	 */
	if (test_for_directory(install_plugin_dir) != EISDIR) {
		/*
		 * Either it doesn't refer to a directory or it
		 * refers to something that doesn't exist.
		 *
		 * Assume that means we're running, for example,
		 * a version of Ethereal we've built in a source
		 * directory, and fall back on the default
		 * installation directory, so you can put the plugins
		 * somewhere so they can be used with this version
		 * of Ethereal.
		 *
		 * XXX - should we, instead, have the Windows build
		 * procedure create a subdirectory of the "plugins"
		 * source directory, and copy the plugin DLLs there,
		 * so that you use the plugins from the build tree?
		 */
		g_free(install_plugin_dir);
		install_plugin_dir =
		    g_strdup("C:\\Program Files\\Ethereal\\plugins\\" VERSION);
	}

	return install_plugin_dir;
#else
	/*
	 * Scan the plugin directory.
	 */
	return strdup(plugin_dir);
#endif
}


/* get the personal plugin dir */
/* Return value is malloced so the caller should g_free() it. */
const char *get_plugins_pers_dir(void)
{
    return get_persconffile_path(PLUGINS_DIR_NAME, FALSE);
}

/*
 * init plugins
 */
void
init_plugins(const char *plugin_dir)
{
    const char *datafile_dir;

    if (plugin_list == NULL)      /* ensure init_plugins is only run once */
    {
	/*
	 * Scan the global plugin directory.
	 */
	datafile_dir = get_plugins_global_dir(plugin_dir);
	plugins_scan_dir(datafile_dir);
	g_free((char *) datafile_dir);

	/*
	 * Scan the users plugin directory.
	 */
	datafile_dir = get_plugins_pers_dir();
	plugins_scan_dir(datafile_dir);
	g_free((char *) datafile_dir);
    }
}

void
register_all_plugin_handoffs(void)
{
  plugin *pt_plug;

  /*
   * For all new-style plugins, call the register-handoff routine.
   * This is called from "proto_init()"; it must be called after
   * "register_all_protocols()" and "init_plugins()" are called,
   * in case one plugin registers itself either with a built-in
   * dissector or with another plugin; we must first register all
   * dissectors, whether built-in or plugin, so their dissector tables
   * are initialized, and only then register all handoffs.
   *
   * We treat those protocols as always being enabled; they should
   * use the standard mechanism for enabling/disabling protocols, not
   * the plugin-specific mechanism.
   */
  for (pt_plug = plugin_list; pt_plug != NULL; pt_plug = pt_plug->next)
    (pt_plug->reg_handoff)();
}
#endif
