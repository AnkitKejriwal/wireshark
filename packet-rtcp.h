/* packet-rtcp.h
 *
 * $Id: packet-rtcp.h,v 1.5 2001/06/12 06:31:14 guy Exp $
 *
 * Routines for RTCP dissection
 * RTCP = Real-time Transport Control Protocol
 * 
 * Copyright 2000, Philips Electronics N.V.
 * Written by Andreas Sikkema <andreas.sikkema@philips.com>
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

void     rtcp_add_address   ( const unsigned char* ip_addr, int prt );
void     dissect_rtcp       ( tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree );
void     proto_register_rtcp( void );
