/* packet-tpkt.h
 *
 * Routines for TPKT dissection
 *
 * Copyright 2000, Philips Electronics N.V.
 * Andreas Sikkema <andreas.sikkema@philips.com>
 *
 * $Id: packet-tpkt.h,v 1.6 2002/02/22 08:56:46 guy Exp $
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

/*
 * Check whether this could be a TPKT-encapsulated PDU.
 * Returns -1 if it's not.
 * Sets "*offset" to the offset of the first byte past the TPKT header,
 * and returns the length from the TPKT header, if it is.
 */
extern int is_tpkt(tvbuff_t *tvb, int *offset);

/*
 * Dissect TPKT-encapsulated data in a TCP stream.
 */
extern void dissect_tpkt_encap(tvbuff_t *tvb, packet_info *pinfo,
    proto_tree *tree, gboolean desegment,
    dissector_handle_t subdissector_handle);
