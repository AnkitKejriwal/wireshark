/* util.c
 * Utility routines
 *
 * $Id: util.c,v 1.67 2003/07/11 06:45:59 guy Exp $
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

#include <glib.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef NEED_SNPRINTF_H
# include "snprintf.h"
#endif

#ifdef NEED_MKSTEMP
#include "mkstemp.h"
#endif

#ifdef HAVE_IO_H
#include <io.h>
typedef int mode_t;	/* for win32 */
#endif

#ifdef HAVE_LIBZ
#include <zlib.h>	/* to get the libz version number */
#endif

#ifdef HAVE_LIBPCAP
#include <pcap.h>
#ifdef WIN32
#include "capture-wpcap.h"
#endif /* WIN32 */
#endif /* HAVE_LIBPCAP */

#ifdef HAVE_SOME_SNMP

#ifdef HAVE_NET_SNMP
#include <net-snmp/version.h>
#endif /* HAVE_NET_SNMP */

#ifdef HAVE_UCD_SNMP
#include <ucd-snmp/version.h>
#endif /* HAVE_UCD_SNMP */

#endif /* HAVE_SOME_SNMP */

#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif

#include "util.h"

/*
 * See whether the last line in the string goes past column 80; if so,
 * replace the blank at the specified point with a newline.
 */
static void
do_word_wrap(GString *str, gint point)
{
	char *line_begin;

	line_begin = strrchr(str->str, '\n');
	if (line_begin == NULL)
		line_begin = str->str;
	else
		line_begin++;
	if (strlen(line_begin) > 80) {
		g_assert(str->str[point] == ' ');
		str->str[point] = '\n';
	}
}	

/*
 * Get various library compile-time versions and append them to
 * the specified GString.
 */
void
get_compiled_version_info(GString *str)
{
#ifdef HAVE_LIBPCAP
#ifdef HAVE_PCAP_VERSION
	extern char pcap_version[];
#endif /* HAVE_PCAP_VERSION */
#endif /* HAVE_LIBPCAP */
	gint break_point;

	g_string_append(str, "with ");
	g_string_sprintfa(str,
#ifdef GLIB_MAJOR_VERSION
	    "GLib %d.%d.%d,", GLIB_MAJOR_VERSION, GLIB_MINOR_VERSION,
	    GLIB_MICRO_VERSION);
#else
	    "GLib (version unknown),");
#endif

#ifdef HAVE_LIBPCAP
	g_string_append(str, " ");
	break_point = str->len - 1;
#ifdef WIN32
	g_string_append(str, "with WinPcap (version unknown)");
#else /* WIN32 */
#ifdef HAVE_PCAP_VERSION
	g_string_sprintfa(str, "with libpcap %s,", pcap_version);
#else /* HAVE_PCAP_VERSION */
	g_string_append(str, "with libpcap (version unknown)");
#endif /* HAVE_PCAP_VERSION */
	do_word_wrap(str, break_point);
#endif /* WIN32 */
#else /* HAVE_LIBPCAP */
	g_string_append(str, " ");
	break_point = str->len - 1;
#ifdef WIN32
	g_string_append(str, "without WinPcap,");
#else /* WIN32 */
	g_string_append(str, "without libpcap,");
#endif /* WIN32 */
	do_word_wrap(str, break_point);
#endif /* HAVE_LIBPCAP */

	g_string_append(str, " ");
	break_point = str->len - 1;
#ifdef HAVE_LIBZ
	g_string_append(str, "with libz ");
#ifdef ZLIB_VERSION
	g_string_append(str, ZLIB_VERSION);
#else /* ZLIB_VERSION */
	g_string_append(str, "(version unknown)");
#endif /* ZLIB_VERSION */
#else /* HAVE_LIBZ */
	g_string_append(str, "without libz");
#endif /* HAVE_LIBZ */
	g_string_append(str, ",");
	do_word_wrap(str, break_point);

/* Oh, this is pretty. */
/* Oh, ha.  you think that was pretty.  Try this:! --Wes */
	g_string_append(str, " ");
	break_point = str->len - 1;
#ifdef HAVE_SOME_SNMP

#ifdef HAVE_UCD_SNMP
	g_string_append(str, "with UCD-SNMP ");
	g_string_append(str, VersionInfo);
#endif /* HAVE_UCD_SNMP */

#ifdef HAVE_NET_SNMP
	g_string_append(str, "with Net-SNMP ");
	g_string_append(str, netsnmp_get_version());
#endif /* HAVE_NET_SNMP */

#else /* no SNMP library */
	g_string_append(str, "without UCD-SNMP or Net-SNMP");
#endif /* HAVE_SOME_SNMP */
	g_string_append(str, ",");
	do_word_wrap(str, break_point);

	g_string_append(str, " ");
	break_point = str->len - 1;
#ifdef HAVE_GNU_ADNS
	g_string_append(str, "with ADNS");
#else
	g_string_append(str, "without ADNS");
#endif /* HAVE_GNU_ADNS */
	do_word_wrap(str, break_point);
}

/*
 * Get various library run-time versions, and the OS version, and append
 * them to the specified GString.
 */
void
get_runtime_version_info(GString *str)
{
#if defined(WIN32)
	OSVERSIONINFO info;
#elif defined(HAVE_SYS_UTSNAME_H)
	struct utsname name;
#endif

#ifdef HAVE_LIBPCAP
#ifdef WIN32
	/*
	 * On Windows, we might have been compiled with WinPcap but
	 * might not have it loaded; indicate whether we have it or
	 * not.
	 *
	 * XXX - when versions of libcap and WinPcap with
	 * "pcap_lib_version()" are released, we should use it
	 * here if available.
	 */
	if (has_wpcap)
		g_string_sprintfa(str, "with WinPcap ");
	else
		g_string_append(str, "without WinPcap ");
#endif /* WIN32 */
#endif /* HAVE_LIBPCAP */

	g_string_append(str, "on ");
#if defined(WIN32)
	info.dwOSVersionInfoSize = sizeof info;
	if (!GetVersionEx(&info)) {
		/*
		 * XXX - get the failure reason.
		 */
		g_string_append(str, "unknown Windows version");
		return;
	}
	switch (info.dwPlatformId) {

	case VER_PLATFORM_WIN32s:
		/* Shyeah, right. */
		g_string_sprintfa(str, "Windows 3.1 with Win32s");
		break;

	case VER_PLATFORM_WIN32_WINDOWS:
		/* Windows OT */
		switch (info.dwMajorVersion) {

		case 4:
			/* 3 cheers for Microsoft marketing! */
			switch (info.dwMinorVersion) {

			case 0:
				g_string_sprintfa(str, "Windows 95");
				break;

			case 10:
				g_string_sprintfa(str, "Windows 98");
				break;

			case 90:
				g_string_sprintfa(str, "Windows Me");
				break;

			default:
				g_string_sprintfa(str, "Windows OT, unknown version %u.%u",
				    info.dwMajorVersion, info.dwMinorVersion);
				break;
			}
			break;

		default:
			g_string_sprintfa(str, "Windows OT, unknown version %u.%u",
			    info.dwMajorVersion, info.dwMinorVersion);
			break;
		}
		break;

	case VER_PLATFORM_WIN32_NT:
		/* Windows NT */
		switch (info.dwMajorVersion) {

		case 3:
		case 4:
			g_string_sprintfa(str, "Windows NT %u.%u",
			    info.dwMajorVersion, info.dwMinorVersion);
			break;

		case 5:
			/* 3 cheers for Microsoft marketing! */
			switch (info.dwMinorVersion) {

			case 0:
				g_string_sprintfa(str, "Windows 2000");
				break;

			case 1:
				g_string_sprintfa(str, "Windows XP");
				break;

			case 2:
				g_string_sprintfa(str, "Windows Server 2003");
				break;

			default:
				g_string_sprintfa(str, "Windows NT, unknown version %u.%u",
				    info.dwMajorVersion, info.dwMinorVersion);
				break;
			}
			break;

		default:
			g_string_sprintfa(str, "Windows NT, unknown version %u.%u",
			    info.dwMajorVersion, info.dwMinorVersion);
			break;
		}
		break;

	default:
		g_string_sprintfa(str, "Unknown Windows platform %u version %u.%u",
		    info.dwPlatformId, info.dwMajorVersion, info.dwMinorVersion);
		break;
	}
	if (info.szCSDVersion[0] != '\0')
		g_string_sprintfa(str, " %s", info.szCSDVersion);
	g_string_sprintfa(str, ", build %u", info.dwBuildNumber);
#elif defined(HAVE_SYS_UTSNAME_H)
	/*
	 * We have <sys/utsname.h>, so we assume we have "uname()".
	 */
	if (uname(&name) < 0) {
		g_string_sprintfa(str, "unknown OS version (uname failed - %s)",
		    strerror(errno));
		return;
	}

	if (strcmp(name.sysname, "AIX") == 0) {
		/*
		 * Yay, IBM!  Thanks for doing something different
		 * from most of the other UNIXes out there, and
		 * making "name.version" apparently be the major
		 * version number and "name.release" be the minor
		 * version number.
		 */
		g_string_sprintfa(str, "%s %s.%s", name.sysname, name.version,
		    name.release);
	} else {
		/*
		 * XXX - get "version" on any other platforms?
		 *
		 * On Digital/Tru65 UNIX, it's something unknown.
		 * On Solaris, it's some kind of build information.
		 * On HP-UX, it appears to be some sort of subrevision
		 * thing.
		 */
		g_string_sprintfa(str, "%s %s", name.sysname, name.release);
	}
#else
	g_string_append(str, "an unknown OS");
#endif
}

/*
 * Collect command-line arguments as a string consisting of the arguments,
 * separated by spaces.
 */
char *
get_args_as_string(int argc, char **argv, int optind)
{
	int len;
	int i;
	char *argstring;

	/*
	 * Find out how long the string will be.
	 */
	len = 0;
	for (i = optind; i < argc; i++) {
		len += strlen(argv[i]);
		len++;	/* space, or '\0' if this is the last argument */
	}

	/*
	 * Allocate the buffer for the string.
	 */
	argstring = g_malloc(len);

	/*
	 * Now construct the string.
	 */
	strcpy(argstring, "");
	i = optind;
	for (;;) {
		strcat(argstring, argv[i]);
		i++;
		if (i == argc)
			break;
		strcat(argstring, " ");
	}
	return argstring;
}

static char *
setup_tmpdir(char *dir)
{
	int len = strlen(dir);
	char *newdir;

	/* Append slash if necessary */
	if (dir[len - 1] == '/') {
		newdir = dir;
	}
	else {
		newdir = g_malloc(len + 2);
		strcpy(newdir, dir);
		strcat(newdir, "/");
	}
	return newdir;
}

static int
try_tempfile(char *namebuf, int namebuflen, const char *dir, const char *pfx)
{
	static const char suffix[] = "XXXXXXXXXX";
	int namelen = strlen(dir) + strlen(pfx) + sizeof suffix;
	mode_t old_umask;
	int tmp_fd;

	if (namebuflen < namelen) {
		/* Stick in a truncated name, so that if this error is
		   reported with the file name, you at least get
		   something. */
		snprintf(namebuf, namebuflen, "%s%s%s", dir, pfx, suffix);
		errno = ENAMETOOLONG;
		return -1;
	}
	strcpy(namebuf, dir);
	strcat(namebuf, pfx);
	strcat(namebuf, suffix);

	/* The Single UNIX Specification doesn't say that "mkstemp()"
	   creates the temporary file with mode rw-------, so we
	   won't assume that all UNIXes will do so; instead, we set
	   the umask to 0077 to take away all group and other
	   permissions, attempt to create the file, and then put
	   the umask back. */
	old_umask = umask(0077);
	tmp_fd = mkstemp(namebuf);
	umask(old_umask);
	return tmp_fd;
}

static char *tmpdir = NULL;
#ifdef WIN32
static char *temp = NULL;
#endif
static char *E_tmpdir;

#ifndef P_tmpdir
#define P_tmpdir "/var/tmp"
#endif

int
create_tempfile(char *namebuf, int namebuflen, const char *pfx)
{
	char *dir;
	int fd;
	static gboolean initialized;

	if (!initialized) {
		if ((dir = getenv("TMPDIR")) != NULL)
			tmpdir = setup_tmpdir(dir);
#ifdef WIN32
		if ((dir = getenv("TEMP")) != NULL)
			temp = setup_tmpdir(dir);
#endif

		E_tmpdir = setup_tmpdir(P_tmpdir);
		initialized = TRUE;
	}

	if (tmpdir != NULL) {
		fd = try_tempfile(namebuf, namebuflen, tmpdir, pfx);
		if (fd != -1)
			return fd;
	}

#ifdef WIN32
	if (temp != NULL) {
		fd = try_tempfile(namebuf, namebuflen, temp, pfx);
		if (fd != -1)
			return fd;
	}
#endif

	fd = try_tempfile(namebuf, namebuflen, E_tmpdir, pfx);
	if (fd != -1)
		return fd;

	return try_tempfile(namebuf, namebuflen, "/tmp", pfx);
}

/* ASCII/EBCDIC conversion tables from
 * http://www.room42.com/store/computer_center/code_tables.shtml
 */
#if 0
static guint8 ASCII_translate_EBCDIC [ 256 ] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
    0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
    0x40, 0x5A, 0x7F, 0x7B, 0x5B, 0x6C, 0x50, 0x7D, 0x4D,
    0x5D, 0x5C, 0x4E, 0x6B, 0x60, 0x4B, 0x61,
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8,
    0xF9, 0x7A, 0x5E, 0x4C, 0x7E, 0x6E, 0x6F,
    0x7C, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8,
    0xC9, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6,
    0xD7, 0xD8, 0xD9, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7,
    0xE8, 0xE9, 0xAD, 0xE0, 0xBD, 0x5F, 0x6D,
    0x7D, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88,
    0x89, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96,
    0x97, 0x98, 0x99, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
    0xA8, 0xA9, 0xC0, 0x6A, 0xD0, 0xA1, 0x4B,
    0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B,
    0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B,
    0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B,
    0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B,
    0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B,
    0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B,
    0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B,
    0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B,
    0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B,
    0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B,
    0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B,
    0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B,
    0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B,
    0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B,
    0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B,
    0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B
};

void
ASCII_to_EBCDIC(guint8 *buf, guint bytes)
{
	guint	i;
	guint8	*bufptr;

	bufptr = buf;

	for (i = 0; i < bytes; i++, bufptr++) {
		*bufptr = ASCII_translate_EBCDIC[*bufptr];
	}
}

guint8
ASCII_to_EBCDIC1(guint8 c)
{
	return ASCII_translate_EBCDIC[c];
}
#endif

static guint8 EBCDIC_translate_ASCII [ 256 ] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
    0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
    0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
    0x2E, 0x2E, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38,
    0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x2E, 0x3F,
    0x20, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E,
    0x2E, 0x2E, 0x2E, 0x3C, 0x28, 0x2B, 0x7C,
    0x26, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E,
    0x2E, 0x21, 0x24, 0x2A, 0x29, 0x3B, 0x5E,
    0x2D, 0x2F, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E,
    0x2E, 0x7C, 0x2C, 0x25, 0x5F, 0x3E, 0x3F,
    0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E,
    0x2E, 0x3A, 0x23, 0x40, 0x27, 0x3D, 0x22,
    0x2E, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E,
    0x2E, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70, 0x71,
    0x72, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E,
    0x2E, 0x7E, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
    0x7A, 0x2E, 0x2E, 0x2E, 0x5B, 0x2E, 0x2E,
    0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E,
    0x2E, 0x2E, 0x2E, 0x2E, 0x5D, 0x2E, 0x2E,
    0x7B, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
    0x49, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E,
    0x7D, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51,
    0x52, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E,
    0x5C, 0x2E, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
    0x5A, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38,
    0x39, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E
};

void
EBCDIC_to_ASCII(guint8 *buf, guint bytes)
{
	guint	i;
	guint8	*bufptr;

	bufptr = buf;

	for (i = 0; i < bytes; i++, bufptr++) {
		*bufptr = EBCDIC_translate_ASCII[*bufptr];
	}
}

guint8
EBCDIC_to_ASCII1(guint8 c)
{
	return EBCDIC_translate_ASCII[c];
}

/* Compute the difference between two seconds/microseconds time stamps. */
void
compute_timestamp_diff(gint *diffsec, gint *diffusec,
	guint32 sec1, guint32 usec1, guint32 sec2, guint32 usec2)
{
  if (sec1 == sec2) {
    /* The seconds part of the first time is the same as the seconds
       part of the second time, so if the microseconds part of the first
       time is less than the microseconds part of the second time, the
       first time is before the second time.  The microseconds part of
       the delta should just be the difference between the microseconds
       part of the first time and the microseconds part of the second
       time; don't adjust the seconds part of the delta, as it's OK if
       the microseconds part is negative. */

    *diffsec = sec1 - sec2;
    *diffusec = usec1 - usec2;
  } else if (sec1 <= sec2) {
    /* The seconds part of the first time is less than the seconds part
       of the second time, so the first time is before the second time.

       Both the "seconds" and "microseconds" value of the delta
       should have the same sign, so if the difference between the
       microseconds values would be *positive*, subtract 1,000,000
       from it, and add one to the seconds value. */
    *diffsec = sec1 - sec2;
    if (usec2 >= usec1) {
      *diffusec = usec1 - usec2;
    } else {
      *diffusec = (usec1 - 1000000) - usec2;
      (*diffsec)++;
    }
  } else {
    /* Oh, good, we're not caught in a chronosynclastic infindibulum. */
    *diffsec = sec1 - sec2;
    if (usec2 <= usec1) {
      *diffusec = usec1 - usec2;
    } else {
      *diffusec = (usec1 + 1000000) - usec2;
      (*diffsec)--;
    }
  }
}

/* Decode a base64 string in-place - simple and slow algorithm.
   Return length of result. Taken from rproxy/librsync/base64.c by
   Andrew Tridgell. */

size_t base64_decode(char *s)
{
	static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	int bit_offset, byte_offset, idx, i, n;
	unsigned char *d = (unsigned char *)s;
	char *p;

	n=i=0;

	while (*s && (p=strchr(b64, *s))) {
		idx = (int)(p - b64);
		byte_offset = (i*6)/8;
		bit_offset = (i*6)%8;
		d[byte_offset] &= ~((1<<(8-bit_offset))-1);
		if (bit_offset < 3) {
			d[byte_offset] |= (idx << (2-bit_offset));
			n = byte_offset+1;
		} else {
			d[byte_offset] |= (idx >> (bit_offset-2));
			d[byte_offset+1] = 0;
			d[byte_offset+1] |= (idx << (8-(bit_offset-2))) & 0xFF;
			n = byte_offset+2;
		}
		s++; i++;
	}

	return n;
}
