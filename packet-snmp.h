/* packet-snmp.h
 * Exported routines for SNMP (simple network management protocol)
 * D.Jorand (c) 1998
 *
 * $Id: packet-snmp.h,v 1.9 2003/09/06 01:21:00 guy Exp $
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@ethereal.com>
 * Copyright 1998 Didier Jorand
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

#ifndef __PACKET_SNMP_H__
#define __PACKET_SNMP_H__

/*
 * Guts of the SNMP dissector - exported for use by protocols such as
 * ILMI.
 */
extern guint dissect_snmp_pdu(tvbuff_t *, int, packet_info *, proto_tree *tree,
    int, gint, gboolean);

#endif
