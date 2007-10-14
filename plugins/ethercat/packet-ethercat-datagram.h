/* packet-ethercat-datagram.h
 *
 * $Id$
 *
 * Copyright (c) 2007 by Beckhoff Automation GmbH
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
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
#ifndef _PACKET_ETHERCAT_DATAGRAM_
#define _PACKET_ETHERCAT_DATAGRAM_

/* structure for decoding the header -----------------------------------------*/

/* Ensure the same data layout for all platforms */
#pragma pack(push, 1)

typedef struct _EcParser
{
   guint8  cmd;
   guint8  idx;
   guint16 adp;
   guint16 ado;
   guint16 len;
   guint16 intr;
} EcParserHDR, *PEcParserHDR;

#define EcParserHDR_Len sizeof(EcParserHDR)

#pragma pack(pop)

#endif /* _PACKET_ETHERCAT_DATAGRAM_ */
