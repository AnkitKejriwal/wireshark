/* filesystem.h
 * Filesystem utility definitions
 *
 * $Id: filesystem.h,v 1.6 2001/10/22 22:59:25 guy Exp $
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

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

/*
 * Given a pathname, return a pointer to the last pathname separator
 * character in the pathname, or NULL if the pathname contains no
 * separators.
 */
char *find_last_pathname_separator(char *);

/*
 * Given a pathname, return the last component.
 */
char *get_basename(char *);

/*
 * Given a pathname, return a string containing everything but the
 * last component.  NOTE: this overwrites the pathname handed into
 * it....
 */
char *get_dirname(char *);

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
int test_for_directory(const char *);

/*
 * Get the directory in which global configuration and data files are
 * stored.
 */
const char *get_datafile_dir(void);

/*
 * Get the directory in which files that, at least on UNIX, are
 * system files (such as "/etc/ethers") are stored; on Windows,
 * there's no "/etc" directory, so we get them from the Ethereal
 * global configuration and data file directory.
 */
const char *get_systemfile_dir(void);

/* Returns the user's home directory, via the HOME environment
 * variable, or a default directory if HOME is not set */
const char* get_home_dir(void);

/*
 * Get the directory in which personal configuration files reside.
 */
const char *get_persconffile_dir(void);

#endif /* FILESYSTEM_H */
