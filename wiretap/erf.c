/*
 *
 * Copyright (c) 2003 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This software and documentation has been developed by Endace Technology Ltd.
 * along with the DAG PCI network capture cards. For further information please
 * visit http://www.endace.com/.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright
 *  notice, this list of conditions and the following disclaimer in the
 *  documentation and/or other materials provided with the distribution.
 *
 *  3. The name of Endace Technology Ltd may not be used to endorse or promote
 *  products derived from this software without specific prior written
 *  permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ENDACE TECHNOLOGY LTD ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL ENDACE TECHNOLOGY LTD BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id$
 */

/* 
 * erf - Endace ERF (Extensible Record Format)
 *
 * See
 *
 *	http://www.endace.com/support/EndaceRecordFormat.pdf
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "wtap-int.h"
#include "file_wrappers.h"
#include "buffer.h"
#include "atm.h"
#include "erf.h"

static int erf_read_header(
			   FILE_T fh,
			   struct wtap_pkthdr *phdr,
			   union wtap_pseudo_header *pseudo_header,
			   erf_header_t *erf_header,
			   int *err,
			   gchar **err_info,
			   guint32 *bytes_read,
			   guint32 *packet_size);
static gboolean erf_read(wtap *wth, int *err, gchar **err_info,
			 gint64 *data_offset);
static gboolean erf_seek_read(wtap *wth, gint64 seek_off,
			      union wtap_pseudo_header *pseudo_header, guchar *pd,
			      int length, int *err, gchar **err_info);
static void erf_close(wtap *wth);

		  
int erf_open(wtap *wth, int *err, gchar **err_info _U_)
{
  guint32 i, n;
  char *s;
  guint32 records_for_erf_check = RECORDS_FOR_ERF_CHECK;
  int common_type = 0;
  erf_timestamp_t prevts;
  guint32 mc_hdr;
  guint16 eth_hdr;

  memset(&prevts, 0, sizeof(prevts));

  /* number of records to scan before deciding if this really is ERF (dflt=3) */
  if ((s = getenv("ERF_RECORDS_TO_CHECK")) != NULL) {
    if ((n = atoi(s)) > 0 && n < 101) {
      records_for_erf_check = n;
    }
  }

  /* ERF is a little hard because there's no magic number */

  for (i = 0; i < records_for_erf_check; i++) {  /* records_for_erf_check */

    erf_header_t header;
    guint32 packet_size;
    erf_timestamp_t ts;

    int r = file_read(&header,1,sizeof(header),wth->fh);

    if (r == 0 ) break;
    if (r != sizeof(header)) {
      if ((*err = file_error(wth->fh)) != 0) {
	return -1;
      } else {
	/* ERF header too short */
	/* Accept the file, only if the very first records have been successfully checked */
	if (i < MIN_RECORDS_FOR_ERF_CHECK) {
	  return 0;
	} else {
	  /* BREAK, the last record is too short, and will be ignored */
	  break;
	}
      }
    }
    
    packet_size = g_ntohs(header.rlen) - sizeof(header);

    /* fail on invalid record type, decreasing timestamps or non-zero pad-bits */
    /* Not all types within this range are decoded, but it is a first filter */
    if (header.type == 0 || header.type > ERF_TYPE_MAX ) {
      return 0;
    }

    /* Skip PAD records, timestamps may not be set */
    if (header.type == ERF_TYPE_PAD) {
      if (file_seek(wth->fh, packet_size, SEEK_CUR, err) == -1) {
	return -1;
      }
      continue;
    }

    if ((ts = pletohll(&header.ts)) < prevts) {
      /* reassembled AAL5 records may not be in time order, so allow 1 sec fudge */
      if (header.type == ERF_TYPE_AAL5) {
	if ( ((prevts-ts)>>32) > 1 ) {
	  return 0;
	}
      } else {
	/* For other records, allow 1/256 sec fudge */
	if ( (prevts-ts)>>24 > 1) { 
	  return 0;
	}
      }
    }
    memcpy(&prevts, &ts, sizeof(prevts));

    /* Check for multiple encapsulation in the same file */
    if (common_type == 0) {
      common_type = header.type;
    } else {
      if (common_type > 0 && common_type != header.type) {
	common_type = -1;
      }
    }
    
    /* Read over MC or ETH subheader */
    switch(header.type) {
    case ERF_TYPE_MC_HDLC:
    case ERF_TYPE_MC_RAW:
    case ERF_TYPE_MC_ATM:
    case ERF_TYPE_MC_RAW_CHANNEL:
    case ERF_TYPE_MC_AAL5:
    case ERF_TYPE_MC_AAL2:
    case ERF_TYPE_COLOR_MC_HDLC_POS:
      if (file_read(&mc_hdr,1,sizeof(mc_hdr),wth->fh) != sizeof(mc_hdr)) {
	*err = file_error(wth->fh);
	return -1;
      }
      packet_size -= sizeof(mc_hdr);
      break;
    case ERF_TYPE_ETH:
    case ERF_TYPE_COLOR_ETH:
    case ERF_TYPE_DSM_COLOR_ETH:
      if (file_read(&eth_hdr,1,sizeof(eth_hdr),wth->fh) != sizeof(eth_hdr)) {
	*err = file_error(wth->fh);
	return -1;
      }
      packet_size -= sizeof(eth_hdr);
      break;
    default:
      break;
    }
    
    if (file_seek(wth->fh, packet_size, SEEK_CUR, err) == -1) {	
      return -1;
    }
  } /* records_for_erf_check */
  
  if (file_seek(wth->fh, 0L, SEEK_SET, err) == -1) {	/* rewind */
    return -1;
  }
  
  wth->data_offset = 0;
  
  /* This is an ERF file */
  wth->file_type = WTAP_FILE_ERF;
  wth->snapshot_length = 0;	/* not available in header, only in frame */

  /*
   * Really want WTAP_ENCAP_PER_PACKET here but that severely limits
   * the number of output formats we can write to. If all the records
   * tested in the loop above were the same encap then use that one,
   * otherwise use WTAP_ENCAP_PER_PACKET.
   */
  wth->file_encap = (common_type < 0 ? WTAP_ENCAP_PER_PACKET : WTAP_ENCAP_ERF);

  wth->subtype_read = erf_read;
  wth->subtype_seek_read = erf_seek_read;
  wth->subtype_close = erf_close;
  wth->tsprecision = WTAP_FILE_TSPREC_NSEC;

  return 1;
}

/* Read the next packet */
static gboolean erf_read(wtap *wth, int *err, gchar **err_info,
			 gint64 *data_offset)
{
  erf_header_t erf_header;
  guint32 packet_size, bytes_read;

  *data_offset = wth->data_offset;
  
  do {
    if (!erf_read_header(wth->fh,
			 &wth->phdr, &wth->pseudo_header, &erf_header,
			 err, err_info, &bytes_read, &packet_size)) {
      return FALSE;
    }
    wth->data_offset += bytes_read;
  } while ( erf_header.type == ERF_TYPE_PAD );
  
  buffer_assure_space(wth->frame_buffer, packet_size);
  
  wtap_file_read_expected_bytes(buffer_start_ptr(wth->frame_buffer),
				(gint32)(packet_size), wth->fh, err );
  wth->data_offset += packet_size;

  return TRUE;
}

static gboolean erf_seek_read(wtap *wth, gint64 seek_off,
			      union wtap_pseudo_header *pseudo_header, guchar *pd,
			      int length, int *err, gchar **err_info)
{
  erf_header_t erf_header;
  guint32 packet_size;

  if (length) {};

  if (file_seek(wth->random_fh, seek_off, SEEK_SET, err) == -1)
    return FALSE;
	
  if (!erf_read_header(wth->random_fh, NULL, pseudo_header, &erf_header,
		       err, err_info, NULL, &packet_size))
    return FALSE;

  wtap_file_read_expected_bytes(pd, (int)packet_size, wth->random_fh, err);
  
  return TRUE;
}

static void erf_close(wtap *wth)
{
  if (wth) { }
}

static int erf_read_header(
			   FILE_T fh,
			   struct wtap_pkthdr *phdr,
			   union wtap_pseudo_header *pseudo_header,
			   erf_header_t *erf_header,
			   int *err,
			   gchar **err_info,
			   guint32 *bytes_read,
			   guint32 *packet_size)
{
  guint32 rec_size;
  guint32 mc_hdr;
  guint16 eth_hdr;

  wtap_file_read_expected_bytes(erf_header, sizeof(*erf_header), fh, err);
  if (bytes_read != NULL) {
    *bytes_read = sizeof(*erf_header);
  }
  
  rec_size = g_ntohs(erf_header->rlen);
  *packet_size = rec_size - sizeof(*erf_header);
  
  if (*packet_size > WTAP_MAX_PACKET_SIZE) {
    /*
     * Probably a corrupt capture file; don't blow up trying
     * to allocate space for an immensely-large packet.
     */
    *err = WTAP_ERR_BAD_RECORD;
    *err_info = g_strdup_printf("erf: File has %u-byte packet, bigger than maximum of %u",
				*packet_size, WTAP_MAX_PACKET_SIZE);
    return FALSE;
  }
  
  if (phdr != NULL) {
    guint64 ts = pletohll(&erf_header->ts);
    
    phdr->ts.secs = (long) (ts >> 32);
    ts = ((ts & 0xffffffff) * 1000 * 1000 * 1000);
    ts += (ts & 0x80000000) << 1; /* rounding */
    phdr->ts.nsecs = ((long) (ts >> 32));
    if (phdr->ts.nsecs >= 1000000000) {
      phdr->ts.nsecs -= 1000000000;
      phdr->ts.secs += 1;
    }
  }
  
  /* Copy the ERF pseudo header */
  pseudo_header->erf.phdr.ts = pletohll(&erf_header->ts);
  pseudo_header->erf.phdr.type = erf_header->type;
  pseudo_header->erf.phdr.flags = erf_header->flags;
  pseudo_header->erf.phdr.rlen = g_ntohs(erf_header->rlen);
  pseudo_header->erf.phdr.lctr = g_ntohs(erf_header->lctr);
  pseudo_header->erf.phdr.wlen = g_ntohs(erf_header->wlen);

  switch (erf_header->type) {

  case ERF_TYPE_HDLC_POS:
  case ERF_TYPE_COLOR_HDLC_POS:
  case ERF_TYPE_DSM_COLOR_HDLC_POS:
  case ERF_TYPE_ATM:
  case ERF_TYPE_AAL5:
  case ERF_TYPE_AAL2:
    if (phdr != NULL) {
      phdr->len =  g_htons(erf_header->wlen);
      phdr->caplen = g_htons(erf_header->wlen); /* g_htons(erf_header->rlen);  */
    }  
    break;
    
  case ERF_TYPE_ETH:
  case ERF_TYPE_COLOR_ETH:
  case ERF_TYPE_DSM_COLOR_ETH:
    wtap_file_read_expected_bytes(&eth_hdr, sizeof(eth_hdr), fh, err); 
    if (bytes_read != NULL)
      *bytes_read += sizeof(eth_hdr); 
    *packet_size -=  sizeof(eth_hdr); 
    pseudo_header->erf.subhdr.eth_hdr = g_htons(eth_hdr);
    if (phdr != NULL) {  
      phdr->len =  g_htons(erf_header->wlen);
      phdr->caplen = g_htons(erf_header->wlen);
    }  
    break;
    
  case ERF_TYPE_MC_HDLC:
  case ERF_TYPE_MC_RAW:
  case ERF_TYPE_MC_ATM:
  case ERF_TYPE_MC_RAW_CHANNEL:
  case ERF_TYPE_MC_AAL5:
  case ERF_TYPE_MC_AAL2:
  case ERF_TYPE_COLOR_MC_HDLC_POS:
    wtap_file_read_expected_bytes(&mc_hdr, sizeof(mc_hdr), fh, err); 
    if (bytes_read != NULL)
      *bytes_read += sizeof(mc_hdr); 
    *packet_size -=  sizeof(mc_hdr); 
    pseudo_header->erf.subhdr.mc_hdr = g_htonl(mc_hdr);
    if (phdr != NULL) {  
      phdr->len =  g_htons(erf_header->wlen);
      phdr->caplen = g_htons(erf_header->wlen);
    }  
    break;
	 
  case ERF_TYPE_IP_COUNTER:
  case ERF_TYPE_TCP_FLOW_COUNTER:
    /* unsupported, continue with default: */
  default:
    *err = WTAP_ERR_UNSUPPORTED_ENCAP;
    *err_info = g_strdup_printf("erf: unknown record encapsulation %u",
				erf_header->type);
    return FALSE;
  }
  
  if (phdr != NULL) {
    phdr->pkt_encap = WTAP_ENCAP_ERF;
  }
  
  return TRUE;
}


