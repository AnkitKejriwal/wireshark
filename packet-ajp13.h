/* packet-ajp13.h
 * Headers for AJP13 dissection
 * Copyright 2002, Christopher K. St. John <cks@distributopia.com>
 *
 * $Id: packet-ajp13.h,v 1.2 2002/06/29 00:00:48 guy Exp $
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __PACKET_AJP13_H__
#define __PACKET_AJP13_H__

void http_ajp13_add(guint32 port, dissector_handle_t handle);

#endif
