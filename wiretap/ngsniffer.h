/* ngsniffer.h
 *
 * $Id: ngsniffer.h,v 1.2 1998/11/12 06:01:25 gram Exp $
 *
 * Wiretap Library
 * Copyright (c) 1998 by Gilbert Ramirez <gram@verdict.uthscsa.edu>
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
struct ngsniffer_hdr {
	guint32	junk1;
	guint32	junk2;
	guint32	junk3;
	guint16	bytes;
	guint16	junk4;
	guint32	junk5;
};


int ngsniffer_open(wtap *wth);
int ngsniffer_read(wtap *wth);
