/* pcap-util-int.h
 * Definitions of routines internal to the libpcap/WinPcap utilities
 *
 * $Id: pcap-util-int.h,v 1.1 2003/10/10 03:04:38 guy Exp $
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

#ifndef __PCAP_UTIL_INT_H__
#define __PCAP_UTIL_INT_H__

#ifdef HAVE_LIBPCAP

extern if_info_t *if_info_new(char *name, char *description);
#ifdef HAVE_PCAP_FINDALLDEVS
extern GList *get_interface_list_findalldevs(int *err, char *err_str);
#endif

#endif /* HAVE_LIBPCAP */

#endif /* __PCAP_UTIL_H__ */
