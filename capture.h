/* capture.h
 * Definitions for packet capture windows
 *
 * $Id: capture.h,v 1.17 1999/09/30 06:49:54 guy Exp $
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@zing.org>
 * Copyright 1998 Gerald Combs
 *
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

#ifndef __CAPTURE_H__
#define __CAPTURE_H__

#ifdef HAVE_LIBPCAP

#ifndef lib_pcap_h
#include <pcap.h>
#endif

/* The version of pcap.h that comes with some systems is missing these
 * #defines.
 */

#ifndef DLT_RAW
#define DLT_RAW 12
#endif

#ifndef DLT_SLIP_BSDOS
#define DLT_SLIP_BSDOS 13
#endif

#ifndef DLT_PPP_BSDOS
#define DLT_PPP_BSDOS 14
#endif

/* Name we give to the child process when doing a "-S" or "-F" capture. */
#define	CHILD_NAME	"ethereal-capture"

/* Create a temporary file and start a capture to it. */
void   do_capture(void);

/* Start a capture to a file we've opened; "cf.save_file" is the
   pathname of the file, and "cf.save_file_fd" is the file descriptor
   we got when we opened it. */
void   run_capture(void);

/* Do the low-level work of a capture. */
void   capture(void);

#endif /* HAVE_LIBPCAP */
#endif /* capture.h */
