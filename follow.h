/* follow.h
 *
 * $Id: follow.h,v 1.4 1999/07/07 01:41:15 guy Exp $
 *
 * Copyright 1998 Mike Hall <mlh@io.com>
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@zing.org>
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
 *
 */

#ifndef __FOLLOW_H__
#define __FOLLOW_H__

#include "packet.h"

extern gboolean incomplete_tcp_stream;

typedef struct _tcp_frag {
  u_long              seq;
  u_long              len;
  u_long              data_len;
  u_char             *data;
  struct _tcp_frag   *next;
} tcp_frag;

char* build_follow_filter( packet_info * );
void reassemble_tcp( u_long, u_long, const char*, u_long, int, u_long );
void  reset_tcp_reassembly( void );

#endif
