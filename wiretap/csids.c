/* csids.c
 *
 * $Id: csids.c,v 1.3 2000/08/31 16:44:47 gram Exp $
 *
 * Copyright (c) 2000 by Mike Hall <mlh@io.com>
 * Copyright (c) 2000 by Cisco Systems
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "wtap-int.h"
#include "buffer.h"
#include "csids.h"
#include "file_wrappers.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 
 * This module reads the output from the Cisco Secure Intrustion Detection
 * System iplogging facility. The term iplogging is misleading since this 
 * logger will only output TCP. There is no link layer information. 
 * Packet format is 4 byte timestamp (seconds since epoch), and a 4 byte size
 * of data following for that packet.
 *
 * For a time there was an error in iplogging and the ip length, flags, and id
 * were byteswapped. We will check for this and handle it before handing to ethereal.
 *
 */

static int csids_read(wtap *wth, int *err);
static int csids_seek_read(wtap *wth, int seek_off,
	union wtap_pseudo_header *pseudo_header, guint8 *pd, int len);

struct csids_header {
  guint32 seconds; /* seconds since epoch */
  guint16 zeropad; /* 2 byte zero'ed pads */
  guint16 caplen;  /* the capture length  */
};

/* XXX - return -1 on I/O error and actually do something with 'err'. */
int csids_open(wtap *wth, int *err)
{
  /* There is no file header. There is only a header for each packet 
   * so we read a packet header and compare the caplen with iplen. They 
   * should always be equal except with the wierd byteswap version.
   * 
   * THIS IS BROKEN-- anytime the caplen is 0x0101 or 0x0202 up to 0x0505 
   * this will byteswap it. I need to fix this. XXX --mlh 
   */

  int tmp,iplen,bytesRead;
  
  gboolean byteswap = FALSE;
  struct csids_header hdr;
  bytesRead=0;

  file_seek(wth->fh, 0, SEEK_SET); 
 
  /* check the file to make sure it is a csids file. */ 
  bytesRead = file_read( &hdr, 1, sizeof( struct csids_header), wth->fh );
  if( bytesRead != sizeof( struct csids_header) ) {
    *err = file_error( wth->fh );
    if( *err != 0 ) {
      return -1;
    } else {
      return 0;
    }
  }
  if( hdr.zeropad != 0 ) {
	return 0;
  }
  hdr.seconds = pntohl( &hdr.seconds );
  hdr.caplen = pntohs( &hdr.caplen );
  bytesRead = file_read( &tmp, 1, 2, wth->fh );
  if( bytesRead != 2 ) {
    *err = file_error( wth->fh );
    if( *err != 0 ) {
      return -1;
    } else {
      return 0;
    }
  }
  bytesRead = file_read( &iplen, 1, 2, wth->fh );
  if( bytesRead != 2 ) {
    *err = file_error( wth->fh );
    if( *err != 0 ) {
      return -1;
    } else {
      return 0;
    }
  }
  iplen = pntohs(&iplen);
  /* if iplen and hdr.caplen are equal, default to no byteswap. */
  if( iplen > hdr.caplen ) {
    /* maybe this is just a byteswapped version. the iplen ipflags */
    /* and ipid are swapped. We cannot use the normal swaps because */
    /* we don't know the host */
    iplen = BSWAP16(iplen);
    if( iplen <= hdr.caplen ) {
      /* we know this format */
      byteswap = TRUE;
    } else {
      /* don't know this one */
      return 0;
    }
  } else {
    byteswap = FALSE;
  } 

  wth->data_offset = 0; 
  wth->capture.csids = g_malloc(sizeof(csids_t));
  wth->capture.csids->byteswapped = byteswap;
  wth->file_encap = WTAP_ENCAP_RAW_IP; 
  wth->file_type = WTAP_FILE_CSIDS; 
  wth->snapshot_length = 16384; /* just guessing */ 
  wth->subtype_read = csids_read; 
  wth->subtype_seek_read = csids_seek_read; 

  /* no file header. So reset the fh to 0 so we can read the first packet */
  file_seek(wth->fh, 0, SEEK_SET); 

  return 1;
}

/* Find the next packet and parse it; called from wtap_loop(). */
static int csids_read(wtap *wth, int *err)
{
  guint8 *buf;
  int bytesRead = 0;
  struct csids_header hdr;
  int packet_offset = wth->data_offset;

  bytesRead = file_read( &hdr, 1, sizeof( struct csids_header) , wth->fh );
  if( bytesRead != sizeof( struct csids_header) ) {
    *err = file_error( wth->fh );
    if( *err != 0 ) {
      return -1;
    } else {
      return 0;
    }
  }
  hdr.seconds = pntohl(&hdr.seconds);
  hdr.caplen = pntohs(&hdr.caplen);

  wth->data_offset += sizeof( struct csids_header );

  /* Make sure we have enough room for the packet */
  buffer_assure_space(wth->frame_buffer, hdr.caplen);
  buf = buffer_start_ptr(wth->frame_buffer);
  
  bytesRead = file_read( buf, 1, hdr.caplen, wth->fh );
  if( bytesRead != hdr.caplen ) {
    *err = file_error( wth->fh );
    if( *err != 0 ) {
      return -1;
    }       
  }
  
  wth->data_offset += hdr.caplen;

  wth->phdr.len = hdr.caplen;
  wth->phdr.caplen = hdr.caplen;
  wth->phdr.ts.tv_sec = hdr.seconds;
  wth->phdr.ts.tv_usec = 0;
  wth->phdr.pkt_encap = WTAP_ENCAP_RAW_IP;

  if( wth->capture.csids->byteswapped == TRUE ) {
    guint16* swap = (guint16*)buf;
    *(++swap) = BSWAP16(*swap); /* the ip len */
    *(++swap) = BSWAP16(*swap); /* ip id */
    *(++swap) = BSWAP16(*swap); /* ip flags and fragoff */
  }

  /* This is a hack to fix the fact that have to atleast return 1 
   * or we stop processing. csids has no file header. We recover from 
   * this hack in csids_seek_read by checking the seek_off == 1 and 
   * setting it back to 0.
   */
  return packet_offset ? packet_offset : 1;
}

/* Used to read packets in random-access fashion */
static int
csids_seek_read (wtap *wth,
		 int seek_off,
		 union wtap_pseudo_header *pseudo_header,
		 guint8 *pd,
		 int len)
{
  int err = 0;
  int bytesRead = 0;
  struct csids_header hdr;

  /* hack to fix a problem with the way error checking is done. If the 
   * the return value from csids_read is 0 for the first packet, then
   * we stop there. So I return 1. But that messes up the offset for 
   * the seek_off on this call. So if seek_off is 1 then make it 0 and 
   * if it is not 1 leave it alone. --mlh 
   */
  int real_seek_off = seek_off;
  if( real_seek_off == 1 ) {
    real_seek_off = 0;
  }

  file_seek(wth->random_fh, real_seek_off , SEEK_SET);

  bytesRead = file_read( &hdr, 1, sizeof( struct csids_header) , wth->random_fh );
  if( bytesRead != sizeof( struct csids_header) ) {
    err = file_error( wth->fh );
    if( err != 0 ) {
      return -1;
    } else {
      return 0;
    }
  }
  hdr.seconds = pntohl(&hdr.seconds);
  hdr.caplen = pntohs(&hdr.caplen);
  
  if( len != hdr.caplen ) {
    return -1;
  }

  bytesRead = file_read( pd, 1, hdr.caplen, wth->random_fh );
  if( bytesRead != hdr.caplen ) {
    err = file_error( wth->fh );
    if( err != 0 ) {
      return -1;
    }       
  }

  if( wth->capture.csids->byteswapped == TRUE ) {
    guint16* swap = (guint16*)pd;
    *(++swap) = BSWAP16(*swap); /* the ip len */
    *(++swap) = BSWAP16(*swap); /* ip id */
    *(++swap) = BSWAP16(*swap); /* ip flags and fragoff */
  }
  
  return 0;
}



