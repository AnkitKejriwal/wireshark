/* follow.h
 *
 * $Id: follow.h,v 1.9 2000/08/09 05:18:38 gram Exp $
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

typedef struct _tcp_stream_chunk {
  guint32     src_addr;
  guint16     src_port;
  guint32     dlen;
} tcp_stream_chunk;

char* build_follow_filter( packet_info * );
void reassemble_tcp( u_long, u_long, const char*, u_long, int, 
		     address *, address *, u_int, u_int );
void  reset_tcp_reassembly( void );

typedef struct {
	guint32		ip_address[2];
	guint32		tcp_port[2];
	unsigned int	bytes_written[2];
} follow_tcp_stats_t;

void follow_tcp_stats(follow_tcp_stats_t* stats);

#endif
