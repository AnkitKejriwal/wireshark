/* nettl.h
 *
 * $Id$
 *
 * Wiretap Library
 * Copyright (c) 1998 by Gilbert Ramirez <gram@alumni.rice.edu>
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
 *
 */

#ifndef __NETTL_H__
#define __NETTL_H__

/* nettl subsystems are defined in /etc/nettlgen.conf */

#define NETTL_SUBSYS_NS_LS_LOGGING	0
#define NETTL_SUBSYS_NS_LS_NFT		1
#define NETTL_SUBSYS_NS_LS_LOOPBACK	2
#define NETTL_SUBSYS_NS_LS_NI		3
#define NETTL_SUBSYS_NS_LS_IPC		4
#define NETTL_SUBSYS_NS_LS_SOCKREGD	5
#define NETTL_SUBSYS_NS_LS_TCP		6
#define NETTL_SUBSYS_NS_LS_PXP		7
#define NETTL_SUBSYS_NS_LS_UDP		8
#define NETTL_SUBSYS_NS_LS_IP		9
#define NETTL_SUBSYS_NS_LS_PROBE	10
#define NETTL_SUBSYS_NS_LS_DRIVER	11
#define NETTL_SUBSYS_NS_LS_RLBD		12
#define NETTL_SUBSYS_NS_LS_BUFS		13
#define NETTL_SUBSYS_NS_LS_CASE21	14
#define NETTL_SUBSYS_NS_LS_ROUTER21	15
#define NETTL_SUBSYS_NS_LS_NFS		16
#define NETTL_SUBSYS_NS_LS_NETISR	17
#define NETTL_SUBSYS_NS_LS_NSE		18
#define NETTL_SUBSYS_NS_LS_STRLOG	19
#define NETTL_SUBSYS_NS_LS_TIRDWR	21
#define NETTL_SUBSYS_NS_LS_TIMOD	22
#define NETTL_SUBSYS_NS_LS_ICMP		23
#define NETTL_SUBSYS_FILTER		26
#define NETTL_SUBSYS_NAME		27
#define NETTL_SUBSYS_IGMP		29
#define NETTL_SUBSYS_SX25L2		34
#define NETTL_SUBSYS_SX25L3		35
#define NETTL_SUBSYS_FTAM_INIT		64
#define NETTL_SUBSYS_FTAM_RESP		65
#define NETTL_SUBSYS_FTAM_VFS		70
#define NETTL_SUBSYS_FTAM_USER		72
#define NETTL_SUBSYS_OTS		90
#define NETTL_SUBSYS_NETWORK		91
#define NETTL_SUBSYS_TRANSPORT		92
#define NETTL_SUBSYS_SESSION		93
#define NETTL_SUBSYS_ACSE_PRES		94
#define NETTL_SUBSYS_SHM		116
#define NETTL_SUBSYS_ACSE_US		119
#define NETTL_SUBSYS_HPS		121
#define NETTL_SUBSYS_CM			122
#define NETTL_SUBSYS_ULA_UTILS		123
#define NETTL_SUBSYS_EM			124
#define NETTL_SUBSYS_HP_APAPORT		189
#define NETTL_SUBSYS_HP_APALACP		190
#define NETTL_SUBSYS_NS_LS_IPV6		244
#define NETTL_SUBSYS_NS_LS_ICMPV6	245

/* Ethernet cards */
#define NETTL_SUBSYS_LAN100		164
#define NETTL_SUBSYS_BASE100		173
#define NETTL_SUBSYS_GSC100BT		178
#define NETTL_SUBSYS_PCI100BT		179
#define NETTL_SUBSYS_SPP100BT		180
#define NETTL_SUBSYS_GELAN		185
#define NETTL_SUBSYS_BTLAN		210
#define NETTL_SUBSYS_INTL100		233
#define NETTL_SUBSYS_IGELAN		252
#define NETTL_SUBSYS_IETHER		253

/* FDDI cards */
#define NETTL_SUBSYS_HPPB_FDDI		95
#define NETTL_SUBSYS_PCI_FDDI		176

/* Token Ring cards */
#define NETTL_SUBSYS_TOKEN		31
#define NETTL_SUBSYS_PCI_TR		187

/* from /usr/include/sys/subsys_id.h */

#define NETTL_HDR_PDUIN			0x20
#define NETTL_HDR_PDUOUT		0x10

int nettl_open(wtap *wth, int *err, gchar **err_info);

#endif
