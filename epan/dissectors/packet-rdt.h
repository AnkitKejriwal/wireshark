/* packet-rdt.h
 *
 * Routines for RDT dissection
 * RDT = Real Data Transport
 *
 * $Id: packet-rdt.h 13300 2005-02-05 11:08:24Z etxrab $
 *
 * Written by Martin Mathieson
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

/* Info to save in RDT conversation / packet-info */
#define MAX_RDT_SETUP_METHOD_SIZE 7
struct _rdt_conversation_info
{
    gchar   method[MAX_RDT_SETUP_METHOD_SIZE + 1];
    guint32 frame_number;
};

/* Add an RDT conversation with the given details */
void rdt_add_address(packet_info *pinfo,
                     address *addr, int port,
                     int other_port,
                     gchar *setup_method, guint32 setup_frame_number);

