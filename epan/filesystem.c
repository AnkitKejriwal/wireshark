/* filesystem.c
 * Filesystem utility routines
 *
 * $Id$
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <glib.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#ifdef HAVE_DIRECT_H
#include <direct.h>		/* to declare "mkdir()" on Windows */
#endif

#ifndef _WIN32
#include <pwd.h>
#endif

#include "filesystem.h"

/*
 * Given a pathname, return a pointer to the last pathname separator
 * character in the pathname, or NULL if the pathname contains no
 * separators.
 */
char *
find_last_pathname_separator(char *path)
{
	char *separator;

#ifdef _WIN32
	char c;

	/*
	 * We have to scan for '\' or '/'.
	 * Get to the end of the string.
	 */
	separator = path + strlen(path);	/* points to ending '\0' */
	while (separator > path) {
		c = *--separator;
		if (c == '\\' || c == '/')
			return separator;	/* found it */
	}

	/*
	 * OK, we didn't find any, so no directories - but there might
	 * be a drive letter....
	 */
	return strchr(path, ':');
#else
	separator = strrchr(path, '/');
#endif
	return separator;
}

/*
 * Given a pathname, return the last component.
 */
char *
get_basename(char *path)
{
	char *filename;

	g_assert(path != NULL);
	filename = find_last_pathname_separator(path);
	if (filename == NULL) {
		/*
		 * There're no directories, drive letters, etc. in the
		 * name; the pathname *is* the file name.
		 */
		filename = path;
	} else {
		/*
		 * Skip past the pathname or drive letter separator.
		 */
		filename++;
	}
	return filename;
}

/*
 * Given a pathname, return a string containing everything but the
 * last component.  NOTE: this overwrites the pathname handed into
 * it....
 */
char *
get_dirname(char *path)
{
	char *separator;

	g_assert(path != NULL);
	separator = find_last_pathname_separator(path);
	if (separator == NULL) {
		/*
		 * There're no directories, drive letters, etc. in the
		 * name; there is no directory path to return.
		 */
		return NULL;
	}

	/*
	 * Get rid of the last pathname separator and the final file
	 * name following it.
	 */
	*separator = '\0';

	/*
	 * "path" now contains the pathname of the directory containing
	 * the file/directory to which it referred.
	 */
	return path;
}

/*
 * Given a pathname, return:
 *
 *	the errno, if an attempt to "stat()" the file fails;
 *
 *	EISDIR, if the attempt succeeded and the file turned out
 *	to be a directory;
 *
 *	0, if the attempt succeeded and the file turned out not
 *	to be a directory.
 */

/*
 * Visual C++ on Win32 systems doesn't define these.  (Old UNIX systems don't
 * define them either.)
 *
 * Visual C++ on Win32 systems doesn't define S_IFIFO, it defines _S_IFIFO.
 */
#ifndef S_ISREG
#define S_ISREG(mode)   (((mode) & S_IFMT) == S_IFREG)
#endif
#ifndef S_IFIFO
#define S_IFIFO	_S_IFIFO
#endif
#ifndef S_ISFIFO
#define S_ISFIFO(mode)  (((mode) & S_IFMT) == S_IFIFO)
#endif
#ifndef S_ISDIR
#define S_ISDIR(mode)   (((mode) & S_IFMT) == S_IFDIR)
#endif

int
test_for_directory(const char *path)
{
	struct stat statb;

	if (stat(path, &statb) < 0)
		return errno;

	if (S_ISDIR(statb.st_mode))
		return EISDIR;
	else
		return 0;
}

int
test_for_fifo(const char *path)
{
	struct stat statb;

	if (stat(path, &statb) < 0)
		return errno;

	if (S_ISFIFO(statb.st_mode))
		return ESPIPE;
	else
		return 0;
}

/*
 * Get the directory in which Ethereal's global configuration and data
 * files are stored.
 *
 * XXX - if we ever make libethereal a real library, used by multiple
 * applications (more than just Tethereal and versions of Ethereal with
 * various UIs), should the configuration files belong to the library
 * (and be shared by all those applications) or to the applications?
 *
 * If they belong to the library, that could be done on UNIX by the
 * configure script, but it's trickier on Windows, as you can't just
 * use the pathname of the executable.
 *
 * If they belong to the application, that could be done on Windows
 * by using the pathname of the executable, but we'd have to have it
 * passed in as an argument, in some call, on UNIX.
 *
 * Note that some of those configuration files might be used by code in
 * libethereal, some of them might be used by dissectors (would they
 * belong to libethereal, the application, or a separate library?),
 * and some of them might be used by other code (the Ethereal preferences
 * file includes resolver preferences that control the behavior of code
 * in libethereal, dissector preferences, and UI preferences, for
 * example).
 */
const char *
get_datafile_dir(void)
{
#ifdef _WIN32
	char prog_pathname[_MAX_PATH+2];
	char *dir_end;
	size_t datafile_dir_len;
	static char *datafile_dir;

	/*
	 * Have we already gotten the pathname?
	 * If so, just return it.
	 */
	if (datafile_dir != NULL)
		return datafile_dir;

	/*
	 * No, we haven't.
	 * Start out by assuming it's the default installation directory.
	 */
	datafile_dir = "C:\\Program Files\\Ethereal\\";

	/*
	 * Now we attempt to get the full pathname of the currently running
	 * program, under the assumption that we're running an installed
	 * version of the program.  If we fail, we don't change "datafile_dir",
	 * and thus end up using the default.
	 *
	 * XXX - does NSIS put the installation directory into
	 * "\HKEY_LOCAL_MACHINE\SOFTWARE\Ethereal\InstallDir"?
	 * If so, perhaps we should read that from the registry,
	 * instead.
	 */
	if (GetModuleFileName(NULL, prog_pathname, sizeof prog_pathname) != 0) {
		/*
		 * If the program is an installed version, the full pathname
		 * includes the pathname of the directory in which it was
		 * installed; get that directory's pathname, and construct
		 * from it the pathname of the directory in which the
		 * plugins were installed.
		 *
		 * First, find the last "\\" in the directory, as that
		 * marks the end of the directory pathname.
		 *
		 * XXX - Can the pathname be something such as
		 * "C:ethereal.exe"?  Or is it always a full pathname
		 * beginning with "\\" after the drive letter?
		 */
		dir_end = strrchr(prog_pathname, '\\');
		if (dir_end != NULL) {
			/*
			 * Found it - now figure out how long the datafile
			 * directory pathname will be.
			 */
			datafile_dir_len = (dir_end - prog_pathname);

			/*
			 * Allocate a buffer for the plugin directory
			 * pathname, and construct it.
			 */
			datafile_dir = g_malloc(datafile_dir_len + 1);
			strncpy(datafile_dir, prog_pathname, datafile_dir_len);
			datafile_dir[datafile_dir_len] = '\0';
		}
	}
	return datafile_dir;
#else
	/*
	 * Just use DATAFILE_DIR, as that's what the configure script
	 * set it to be.
	 */
	return DATAFILE_DIR;
#endif
}

/*
 * Get the directory in which files that, at least on UNIX, are
 * system files (such as "/etc/ethers") are stored; on Windows,
 * there's no "/etc" directory, so we get them from the Ethereal
 * global configuration and data file directory.
 */
const char *
get_systemfile_dir(void)
{
#ifdef _WIN32
	return get_datafile_dir();
#else
	return "/etc";
#endif
}

/*
 * Name of directory, under the user's home directory, in which
 * personal configuration files are stored.
 */
#ifdef _WIN32
#define PF_DIR "Ethereal"
#else
/*
 * XXX - should this be ".libepan"? For backwards-compatibility, I'll keep
 * it ".ethereal" for now.
 */
#define PF_DIR ".ethereal"
#endif

/*
 * Get the directory in which personal configuration files reside;
 * in UNIX-compatible systems, it's ".ethereal", under the user's home
 * directory, and on Windows systems, it's "Ethereal", under %APPDATA%
 * or, if %APPDATA% isn't set, it's "%USERPROFILE%\Application Data"
 * (which is what %APPDATA% normally is on Windows 2000).
 */
static const char *
get_persconffile_dir(void)
{
#ifdef _WIN32
	char *appdatadir;
	char *userprofiledir;
#else
	char *homedir;
	struct passwd *pwd;
#endif
	static char *pf_dir = NULL;

	/* Return the cached value, if available */
	if (pf_dir != NULL)
		return pf_dir;

#ifdef _WIN32
	/*
	 * Use %APPDATA% or %USERPROFILE%, so that configuration files are
	 * stored in the user profile, rather than in the home directory.
	 * The Windows convention is to store configuration information
	 * in the user profile, and doing so means you can use
	 * Ethereal even if the home directory is an inaccessible
	 * network drive.
	 */
	appdatadir = getenv("APPDATA");
	if (appdatadir != NULL) {
		/*
		 * Concatenate %APPDATA% with "\Ethereal".
		 */
		pf_dir = g_malloc(strlen(appdatadir) + strlen(PF_DIR) + 2);
		sprintf(pf_dir, "%s" G_DIR_SEPARATOR_S "%s", appdatadir,
		    PF_DIR);
	} else {
		/*
		 * OK, %APPDATA% wasn't set, so use
		 * %USERPROFILE%\Application Data.
		 */
		userprofiledir = getenv("USERPROFILE");
		if (userprofiledir != NULL) {
			pf_dir = g_malloc(strlen(userprofiledir) +
			    strlen("Application Data") + strlen(PF_DIR) + 3);
			sprintf(pf_dir,
			    "%s" G_DIR_SEPARATOR_S "Application Data" G_DIR_SEPARATOR_S "%s",
			    userprofiledir, PF_DIR);
		} else {
			/*
			 * Give up and use "C:".
			 */
			pf_dir = g_malloc(strlen("C:") + strlen(PF_DIR) + 2);
			sprintf(pf_dir, "C:" G_DIR_SEPARATOR_S "%s", PF_DIR);
		}
	}
#else
	/*
	 * If $HOME is set, use that.
	 */
	homedir = getenv("HOME");
	if (homedir == NULL) {
		/*
		 * Get their home directory from the password file.
		 * If we can't even find a password file entry for them,
		 * use "/tmp".
		 */
		pwd = getpwuid(getuid());
		if (pwd != NULL) {
			/*
			 * This is cached, so we don't need to worry
			 * about allocating multiple ones of them.
			 */
			homedir = g_strdup(pwd->pw_dir);
		} else
			homedir = "/tmp";
	}
	pf_dir = g_malloc(strlen(homedir) + strlen(PF_DIR) + 2);
	sprintf(pf_dir, "%s" G_DIR_SEPARATOR_S "%s", homedir, PF_DIR);
#endif

	return pf_dir;
}

/*
 * Create the directory that holds personal configuration files, if
 * necessary.  If we attempted to create it, and failed, return -1 and
 * set "*pf_dir_path_return" to the pathname of the directory we failed
 * to create (it's g_mallocated, so our caller should free it); otherwise,
 * return 0.
 */
int
create_persconffile_dir(char **pf_dir_path_return)
{
	const char *pf_dir_path;
#ifdef _WIN32
	char *pf_dir_path_copy, *pf_dir_parent_path;
	size_t pf_dir_parent_path_len;
#endif
	struct stat s_buf;
	int ret;

	pf_dir_path = get_persconffile_dir();
	if (stat(pf_dir_path, &s_buf) != 0 && errno == ENOENT) {
#ifdef _WIN32
		/*
		 * Does the parent directory of that directory
		 * exist?  %APPDATA% may not exist even though
		 * %USERPROFILE% does.
		 *
		 * We check for the existence of the directory
		 * by first checking whether the parent directory
		 * is just a drive letter and, if it's not, by
		 * doing a "stat()" on it.  If it's a drive letter,
		 * or if the "stat()" succeeds, we assume it exists.
		 */
		pf_dir_path_copy = g_strdup(pf_dir_path);
		pf_dir_parent_path = get_dirname(pf_dir_path_copy);
		pf_dir_parent_path_len = strlen(pf_dir_parent_path);
		if (pf_dir_parent_path_len > 0
		    && pf_dir_parent_path[pf_dir_parent_path_len - 1] != ':'
		    && stat(pf_dir_parent_path, &s_buf) != 0) {
			/*
			 * No, it doesn't exist - make it first.
			 */
			ret = mkdir(pf_dir_parent_path);
			if (ret == -1) {
				*pf_dir_path_return = pf_dir_parent_path;
				return -1;
			}
		}
		g_free(pf_dir_path_copy);
		ret = mkdir(pf_dir_path);
#else
		ret = mkdir(pf_dir_path, 0755);
#endif
	} else {
		/*
		 * Something with that pathname exists; if it's not
		 * a directory, we'll get an error if we try to put
		 * something in it, so we don't fail here, we wait
		 * for that attempt fo fail.
		 */
		ret = 0;
	}
	if (ret == -1)
		*pf_dir_path_return = g_strdup(pf_dir_path);
	return ret;
}

#ifdef _WIN32
/*
 * Returns the user's home directory on Win32.
 */
static const char *
get_home_dir(void)
{
	static const char *home = NULL;
	char *homedrive, *homepath;
	char *homestring;
	char *lastsep;

	/* Return the cached value, if available */
	if (home)
		return home;

	/*
	 * XXX - should we use USERPROFILE anywhere in this process?
	 * Is there a chance that it might be set but one or more of
	 * HOMEDRIVE or HOMEPATH isn't set?
	 */
	homedrive = getenv("HOMEDRIVE");
	if (homedrive != NULL) {
		homepath = getenv("HOMEPATH");
		if (homepath != NULL) {
			/*
			 * This is cached, so we don't need to worry about
			 * allocating multiple ones of them.
			 */
			homestring =
			    g_malloc(strlen(homedrive) + strlen(homepath) + 1);
			strcpy(homestring, homedrive);
			strcat(homestring, homepath);

			/*
			 * Trim off any trailing slash or backslash.
			 */
			lastsep = find_last_pathname_separator(homestring);
			if (lastsep != NULL && *(lastsep + 1) == '\0') {
				/*
				 * Last separator is the last character
				 * in the string.  Nuke it.
				 */
				*lastsep = '\0';
			}
			home = homestring;
		} else
			home = homedrive;
	} else {
		/*
		 * Give up and use C:.
		 */
		home = "C:";
	}

	return home;
}
#endif

/*
 * Construct the path name of a personal configuration file, given the
 * file name.
 *
 * On Win32, if "for_writing" is FALSE, we check whether the file exists
 * and, if not, construct a path name relative to the ".ethereal"
 * subdirectory of the user's home directory, and check whether that
 * exists; if it does, we return that, so that configuration files
 * from earlier versions can be read.
 */
char *
get_persconffile_path(const char *filename, gboolean for_writing
#ifndef _WIN32
	_U_
#endif
)
{
	char *path;
#ifdef _WIN32
	struct stat s_buf;
	char *old_path;
#endif

	path = (gchar *) g_malloc(strlen(get_persconffile_dir()) +
	    strlen(filename) + 2);
	sprintf(path, "%s" G_DIR_SEPARATOR_S "%s", get_persconffile_dir(),
	    filename);
#ifdef _WIN32
	if (!for_writing) {
		if (stat(path, &s_buf) != 0 && errno == ENOENT) {
			/*
			 * OK, it's not in the personal configuration file
			 * directory; is it in the ".ethereal" subdirectory
			 * of their home directory?
			 */
			old_path = (gchar *) g_malloc(strlen(get_home_dir()) +
			    strlen(".ethereal") + strlen(filename) + 3);
			sprintf(old_path,
			    "%s" G_DIR_SEPARATOR_S ".ethereal" G_DIR_SEPARATOR_S "%s",
			    get_home_dir(), filename);
			if (stat(old_path, &s_buf) == 0) {
				/*
				 * OK, it exists; return it instead.
				 */
				g_free(path);
				path = old_path;
			}
		}
	}
#endif

	return path;
}

/*
 * Construct the path name of a global configuration file, given the
 * file name.
 */
char *
get_datafile_path(const char *filename)
{
	char *path;

	path = (gchar *) g_malloc(strlen(get_datafile_dir()) +
	    strlen(filename) + 2);
	sprintf(path, "%s" G_DIR_SEPARATOR_S "%s", get_datafile_dir(),
	    filename);

	return path;
}

/* Delete a file */
gboolean
deletefile(const char *path)
{
	return unlink(path) == 0;
}

/*
 * Construct and return the path name of a file in the
 * appropriate temporary file directory.
 */
char *get_tempfile_path(const char *filename)
{
	char *path;

	path = (gchar *) g_malloc(strlen(g_get_tmp_dir()) +
	    strlen(filename) + 2);
	sprintf(path, "%s" G_DIR_SEPARATOR_S "%s", g_get_tmp_dir(), filename);

	return path;
}

/*
 * Return an error message for UNIX-style errno indications on open or
 * create operations.
 */
char *
file_open_error_message(int err, gboolean for_writing)
{
	char *errmsg;
	static char errmsg_errno[1024+1];

	switch (err) {

	case ENOENT:
		if (for_writing)
			errmsg = "The path to the file \"%s\" doesn't exist.";
		else
			errmsg = "The file \"%s\" doesn't exist.";
		break;

	case EACCES:
		if (for_writing)
			errmsg = "You don't have permission to create or write to the file \"%s\".";
		else
			errmsg = "You don't have permission to read the file \"%s\".";
		break;

	case EISDIR:
		errmsg = "\"%s\" is a directory (folder), not a file.";
		break;

	case ENOSPC:
		errmsg = "The file \"%s\" could not be created because there is no space left on the file system.";
		break;

#ifdef EDQUOT
	case EDQUOT:
		errmsg = "The file \"%s\" could not be created because you are too close to, or over, your disk quota.";
		break;
#endif

	default:
		snprintf(errmsg_errno, sizeof(errmsg_errno),
				"The file \"%%s\" could not be %s: %s.",
				for_writing ? "created" : "opened",
				strerror(err));
		errmsg = errmsg_errno;
		break;
	}
	return errmsg;
}

/*
 * Return an error message for UNIX-style errno indications on write
 * operations.
 */
char *
file_write_error_message(int err)
{
	char *errmsg;
	static char errmsg_errno[1024+1];

	switch (err) {

	case ENOSPC:
		errmsg = "The file \"%s\" could not be saved because there is no space left on the file system.";
		break;

#ifdef EDQUOT
	case EDQUOT:
		errmsg = "The file \"%s\" could not be saved because you are too close to, or over, your disk quota.";
		break;
#endif

	default:
		snprintf(errmsg_errno, sizeof(errmsg_errno),
		    "An error occurred while writing to the file \"%%s\": %s.",
		    strerror(err));
		errmsg = errmsg_errno;
		break;
	}
	return errmsg;
}

