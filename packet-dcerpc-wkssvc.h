/* packet-dcerpc-wkssvc.h
 * Routines for SMB \PIPE\wkssvc packet disassembly
 * Copyright 2001, Tim Potter <tpot@samba.org>
 * Copyright 2002, Richard Sharpe <rsharpe@richardsharpe.org>
 *
 * $Id: packet-dcerpc-wkssvc.h,v 1.7 2003/05/01 17:53:22 sharpe Exp $
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

#ifndef __PACKET_DCERPC_WKSSVC_H
#define __PACKET_DCERPC_WKSSVC_H

/* Functions available on the WKSSVC pipe.  From Samba, include/rpc_wkssvc.h */

#define WKS_NetWkstaGetInfo       0x00
#define WKS_NetWkstaSetInfo       0x01
#define WKS_NetWkstaEnumUsers     0x02
#define WKS_NetWkstaUnkn_003      0x03
#define WKS_NetWkstaUnkn_004      0x04
#define WKS_NetWkstaTransportEnum 0x05
#endif /* packet-dcerpc-wkssvc.h */
