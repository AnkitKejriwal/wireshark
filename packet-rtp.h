/* packet-rtp.h
 *
 * Routines for RTP dissection
 * RTP = Real time Transport Protocol
 *
 * $Id: packet-rtp.h,v 1.9 2003/05/20 21:22:58 guy Exp $
 *
 * Copyright 2000, Philips Electronics N.V.
 * Written by Andreas Sikkema <andreas.sikkema@philips.com>
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

struct _rtp_info {
	gboolean	info_padding_set;
	gboolean	info_marker_set;
	unsigned int	info_payload_type;
	unsigned int	info_padding_count;
	guint16		info_seq_num;
	guint32		info_timestamp;
	guint32		info_sync_src;
	guint		info_data_len;
	guint		info_payload_offset;
        guint		info_payload_len;
};

void     rtp_add_address   ( packet_info *pinfo, const unsigned char* ip_addr, int prt );
void     proto_register_rtp( void );
