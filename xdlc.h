/* xdlc.h
 * Define *DLC frame types, and routine to dissect the control field of
 * a *DLC frame.
 *
 * $Id: xdlc.h,v 1.3 1999/08/23 23:24:36 guy Exp $
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@unicom.net>
 * Copyright 1998 Gerald Combs
 *
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
 * Low-order bits of first (extended) or only (basic) octet of control
 * field, specifying the frame type.
 */
#define XDLC_I		0x00	/* Information frames */
#define XDLC_S		0x01	/* Supervisory frames */
#define XDLC_U		0x03	/* Unnumbered frames */

int get_xdlc_control(const u_char *pd, int offset, int is_response,
  int extended);

int dissect_xdlc_control(const u_char *pd, int offset, frame_data *fd,
  proto_tree *xdlc_tree, int hf_xdlc_control, int is_response, int extended);
