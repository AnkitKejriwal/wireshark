/* packet-q931.h
 * Declarations of exported routines and tables for Q.931 and Q.2931 frame
 * disassembly
 * Guy Harris <guy@alum.mit.edu>
 *
 * $Id$
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@ethereal.com>
 * Copyright 1998
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

#ifndef __PACKET_Q931_H__
#define __PACKET_Q931_H__

extern void dissect_q931_bearer_capability_ie(tvbuff_t *, int, int,
    proto_tree *);

extern void dissect_q931_cause_ie(tvbuff_t *, int, int,
    proto_tree *, int);

extern void dissect_q931_progress_indicator_ie(tvbuff_t *, int, int,
    proto_tree *);

extern void dissect_q931_high_layer_compat_ie(tvbuff_t *, int, int,
    proto_tree *);

extern void dissect_q931_user_user_ie(tvbuff_t *tvb, int offset, int len,
    proto_tree *tree);

extern const value_string q931_cause_location_vals[];

extern const value_string q931_cause_code_vals[];

extern const value_string q931_protocol_discriminator_vals[];

#endif
