/* packet-dcerpc-browser.h
 * Routines for DCERPC Browser packet disassembly
 * Copyright 2002, Ronnie Sahlberg
 *
 * $Id: packet-dcerpc-browser.h,v 1.3 2003/10/02 21:48:35 guy Exp $
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

#ifndef __PACKET_DCERPC_BROWSER_H
#define __PACKET_DCERPC_BROWSER_H

#define BROWSER_BROWSERR_SERVER_ENUM 		0x00
#define BROWSER_BROWSERR_DEBUG_CALL 		0x01
#define BROWSER_BROWSERR_QUERY_OTHER_DOMAINS	0x02
#define BROWSER_BROWSERR_RESET_NETLOGON_STATE	0x03
#define BROWSER_BROWSERR_DEBUG_TRACE		0x04
#define BROWSER_BROWSERR_QUERY_STATISTICS	0x05
#define BROWSER_BROWSERR_RESET_STATISTICS	0x06
#define BROWSER_NETR_BROWSER_STATISTICS_CLEAR	0x07
#define BROWSER_NETR_BROWSER_STATISTICS_GET	0x08
#define BROWSER_BROWSERR_SET_NETLOGON_STATE	0x09
#define BROWSER_BROWSERR_QUERY_EMULATED_DOMAINS 0x0a
#define BROWSER_BROWSERR_SERVER_ENUM_EX		0x0b

#endif
