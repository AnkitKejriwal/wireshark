/* packet-x509sat.h
 * Routines for X.509 Selected Attribute Types packet dissection
 *
 * $Id: packet-x509sat-template.h,v 1.1 2004/05/24 08:42:29 sahlberg Exp $
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

#ifndef PACKET_X509SAT_H
#define PACKET_X509SAT_H

#include "packet-x509sat-exp.h"

int dissect_x509sat_DirectoryString(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);

#endif  /* PACKET_X509SAT_H */

