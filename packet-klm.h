/* packet-klm.h   2001 Ronnie Sahlberg <See AUTHORS for email>
 *
 * $Id: packet-klm.h,v 1.4 2002/08/28 21:00:19 jmayer Exp $
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

#ifndef PACKET_KLM_H
#define PACKET_KLM_H

#define KLMPROC_TEST   1
#define KLMPROC_LOCK   2
#define KLMPROC_CANCEL 3
#define KLMPROC_UNLOCK 4

#define KLM_PROGRAM 100020

#endif
