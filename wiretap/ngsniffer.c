/* ngsniffer.c
 *
 * $Id: ngsniffer.c,v 1.14 1999/08/02 02:04:37 guy Exp $
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

/* The code in ngsniffer.c that decodes the time fields for each packet in the
 * Sniffer trace originally came from code from TCPVIEW:
 *
 * TCPVIEW
 *
 * Author:	Martin Hunt
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: martinh@cac.washington.edu
 *
 *
 * Copyright 1992 by the University of Washington
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appears in all copies and that both the
 * above copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the University of Washington not be
 * used in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  This software is made
 * available "as is", and
 * THE UNIVERSITY OF WASHINGTON DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED,
 * WITH REGARD TO THIS SOFTWARE, INCLUDING WITHOUT LIMITATION ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, AND IN
 * NO EVENT SHALL THE UNIVERSITY OF WASHINGTON BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, TORT
 * (INCLUDING NEGLIGENCE) OR STRICT LIABILITY, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <time.h>
#include "wtap.h"
#include "buffer.h"
#include "ngsniffer.h"

/*
 * Sniffer record types.
 */
#define REC_VERS	1	/* Version record (f_vers) */
#define REC_FRAME2	4	/* Frame data (f_frame2) */
#define	REC_FRAME4	8	/* Frame data (f_frame4) */
#define REC_EOF		3	/* End-of-file record (no data follows) */

/*
 * Sniffer version record format.
 *
 * XXX - the Sniffer documentation doesn't say what the compression stuff
 * means.  The manual says "IMPORTANT: You must save the file uncompressed
 * to use this format specification."
 */
struct vers_rec {
	gint16	maj_vers;	/* major version number */
	gint16	min_vers;	/* minor version number */
	gint16	time;		/* DOS-format time */
	gint16	date;		/* DOS-format date */
	gint8	type;		/* what type of records follow */
	gint8	network;	/* network type */
	gint8	format;		/* format version (we only support version 1!) */
	guint8	timeunit;	/* timestamp units */
	gint8	cmprs_vers;	/* compression version */
	gint8	cmprs_level;	/* compression level */
	gint16	rsvd[2];	/* reserved */
};

/*
 * Sniffer type 2 data record format - followed by frame data.
 */
struct frame2_rec {
	guint16	time_low;	/* low part of time stamp */
	guint16	time_med;	/* middle part of time stamp */
	guint16	time_high;	/* high part of time stamp */
	gint16	size;		/* number of bytes of data */
	guint8	fs;		/* frame error status bits */
	guint8	flags;		/* buffer flags */
	gint16	true_size;	/* size of original frame, in bytes */
	gint16	rsvd;		/* reserved */
};

/*
 * Sniffer type 4 data record format - followed by frame data.
 *
 * XXX - the manual says that the "flags" field holds "buffer flags;
 * BF_xxxx", but doesn't say what the BF_xxxx flags are.
 *
 * XXX - the manual also says there's an 8-byte "ATMTimeStamp" driver
 * time stamp at the end of "ATMSaveInfo", but, from an ATM Sniffer capture
 * file I've looked at, that appears not to be the case.
 */

/*
 * Fields from the AAL5 trailer for the frame, if it's an AAL5 frame
 * rather than a cell.
 */
typedef struct _ATM_AAL5Trailer {
	guint16	aal5t_u2u;	/* user-to-user indicator */
	guint16	aal5t_len;	/* length of the packet */
	guint32	aal5t_chksum; /* checksum for AAL5 packet */
} ATM_AAL5Trailer;

typedef struct _ATMTimeStamp {
	guint32	msw;	/* most significant word */
	guint32	lsw;	/* least significant word */
} ATMTimeStamp;

typedef struct _ATMSaveInfo {
	guint32 StatusWord;	/* status word from driver */
	ATM_AAL5Trailer Trailer; /* AAL5 trailer */
	guint8	AppTrafType;	/* traffic type */
	guint8	AppHLType;	/* protocol type */
	guint16	AppReserved;	/* reserved */
	guint16	Vpi;		/* virtual path identifier */
	guint16	Vci;		/* virtual circuit identifier */
	guint16	channel;	/* link: 0 for DCE, 1 for DTE */
	guint16	cells;		/* number of cells */
	guint32	AppVal1;	/* type-dependent */
	guint32	AppVal2;	/* type-dependent */
} ATMSaveInfo;

/*
 * Bits in StatusWord.
 */
#define	SW_ERRMASK		0x0F	/* Error mask: */
#define	SW_RX_FIFO_UNDERRUN	0x01	/* Receive FIFO underrun */
#define	SW_RX_FIFO_OVERRUN	0x02	/* Receive FIFO overrun */
#define	SW_RX_PKT_TOO_LONG	0x03	/* Received packet > max size */
#define	SW_CRC_ERROR		0x04	/* CRC error */
#define	SW_USER_ABORTED_RX	0x05	/* User aborted receive */
#define	SW_BUF_LEN_TOO_LONG	0x06	/* buffer len > max buf */
#define	SW_INTERNAL_T1_ERROR	0x07	/* Internal T1 error */
#define	SW_RX_CHANNEL_DEACTIV8	0x08	/* Rx channel deactivate */

#define	SW_ERROR		0x80	/* Error indicator */
#define	SW_CONGESTION		0x40	/* Congestion indicator */
#define	SW_CLP			0x20	/* Cell loss priority indicator */
#define	SW_RAW_CELL		0x100	/* RAW cell indicator */
#define	SW_OAM_CELL		0x200	/* OAM cell indicator */

/*
 * Bits in AppTrafType.
 *
 * For AAL types other than AAL5, the packet data is presumably for a
 * single cell, not a reassembled frame, as the ATM Sniffer manual says
 * it dosn't reassemble cells other than AAL5 cells.
 */
#define	ATT_AALTYPE		0x0F	/* AAL type: */
#define	ATT_AAL_UNKNOWN		0x00	/* Unknown AAL */
#define	ATT_AAL1		0x01	/* AAL1 */
#define	ATT_AAL3_4		0x02	/* AAL3/4 */
#define	ATT_AAL5		0x03	/* AAL5 */
#define	ATT_AAL_USER		0x04	/* User AAL */
#define	ATT_AAL_SIGNALLING	0x05	/* Signaling AAL */
#define	ATT_OAMCELL		0x06	/* OAM cell */

#define	ATT_HLTYPE		0xF0	/* Higher-layer type: */
#define	ATT_HL_UNKNOWN		0x00	/* unknown */
#define	ATT_HL_LLCMX		0x10	/* LLC multiplexed (probably RFC 1483) */
#define	ATT_HL_VCMX		0x20	/* VC multiplexed (probably RFC 1483) */
#define	ATT_HL_LANE		0x30	/* LAN Emulation */
#define	ATT_HL_ILMI		0x40	/* ILMI */
#define	ATT_HL_FRMR		0x50	/* Frame Relay */
#define	ATT_HL_SPANS		0x60	/* FORE SPANS */
#define	ATT_HL_IPSILON		0x70	/* Ipsilon */

/*
 * Values for AppHLType; the interpretation depends on the ATT_HLTYPE
 * bits in AppTrafType.
 */
#define	AHLT_UNKNOWN		0x0
#define	AHLT_VCMX_802_3_FCS	0x1	/* VCMX: 802.3 FCS */
#define	AHLT_LANE_LE_CTRL	0x1	/* LANE: LE Ctrl */
#define	AHLT_IPSILON_FT0	0x1	/* Ipsilon: Flow Type 0 */
#define	AHLT_VCMX_802_4_FCS	0x2	/* VCMX: 802.4 FCS */
#define	AHLT_LANE_802_3		0x2	/* LANE: 802.3 */
#define	AHLT_IPSILON_FT1	0x2	/* Ipsilon: Flow Type 1 */
#define	AHLT_VCMX_802_5_FCS	0x3	/* VCMX: 802.5 FCS */
#define	AHLT_LANE_802_5		0x3	/* LANE: 802.5 */
#define	AHLT_IPSILON_FT2	0x3	/* Ipsilon: Flow Type 2 */
#define	AHLT_VCMX_FDDI_FCS	0x4	/* VCMX: FDDI FCS */
#define	AHLT_LANE_802_3_MC	0x4	/* LANE: 802.3 multicast */
#define	AHLT_VCMX_802_6_FCS	0x5	/* VCMX: 802.6 FCS */
#define	AHLT_LANE_802_5_MC	0x5	/* LANE: 802.5 multicast */
#define	AHLT_VCMX_802_3		0x7	/* VCMX: 802.3 */
#define	AHLT_VCMX_802_4		0x8	/* VCMX: 802.4 */
#define	AHLT_VCMX_802_5		0x9	/* VCMX: 802.5 */
#define	AHLT_VCMX_FDDI		0xa	/* VCMX: FDDI */
#define	AHLT_VCMX_802_6		0xb	/* VCMX: 802.6 */
#define	AHLT_VCMX_FRAGMENTS	0xc	/* VCMX: Fragments */
#define	AHLT_VCMX_BPDU		0xe	/* VCMX: BPDU */

struct frame4_rec {
	guint16	time_low;	/* low part of time stamp */
	guint16	time_med;	/* middle part of time stamp */
	gint8	time_high;	/* high part of time stamp */
	gint8	time_day;	/* time in days since start of capture */
	gint16	size;		/* number of bytes of data */
	gint8	fs;		/* frame error status bits */
	gint8	flags;		/* buffer flags */
	gint16	true_size;	/* size of original frame, in bytes */
	gint16	rsvd3;		/* reserved */
	gint16	atm_pad;	/* pad to 4-byte boundary */
	ATMSaveInfo atm_info;	/* ATM-specific stuff */
};

/*
 * Size of ATM LANE header - we have to strip it off as our caller expects
 * a MAC header at the beginning of the frame data.
 */
#define	ATM_LANE_HEADER_LEN	2

/* values for V.timeunit */
#define NUM_NGSNIFF_TIMEUNITS 7
static double Usec[] = { 15.0, 0.838096, 15.0, 0.5, 2.0, 0.0, 0.1 };

#define NGSNIFF_ENCAP_ATM 10
#define NUM_NGSNIFF_ENCAPS 11
static int sniffer_encap[] = {
		WTAP_ENCAP_TR,
		WTAP_ENCAP_ETHERNET,
		WTAP_ENCAP_ARCNET,
		WTAP_ENCAP_NONE,	/* StarLAN */
		WTAP_ENCAP_NONE,	/* PC Network broadband */
		WTAP_ENCAP_NONE,	/* LocalTalk */
		WTAP_ENCAP_NONE,	/* Znet */
		WTAP_ENCAP_LAPB,	/* Internetwork analyzer */
		WTAP_ENCAP_NONE,	/* type 8 not defined in Sniffer */
		WTAP_ENCAP_FDDI,
		WTAP_ENCAP_NONE		/* ATM */
};

static int get_atm_linktype(wtap *wth);
static int linktype_for_packet(u_int app_traf_type, u_int app_hl_type);

/* Returns WTAP_FILE_NGSNIFFER on success, WTAP_FILE_UNKNOWN on failure */
int ngsniffer_open(wtap *wth)
{
	int bytes_read;
	char magic[18];
	char record_type[2];
	char record_length[4]; /* only the first 2 bytes are length,
							  the last 2 are "reserved" and are thrown away */
	guint16 type, length = 0;
	struct vers_rec version;
	guint16	start_date;
	guint16	start_time;
	struct tm tm;

	/* Read in the string that should be at the start of a Sniffer file */
	fseek(wth->fh, 0, SEEK_SET);
	bytes_read = fread(magic, 1, 17, wth->fh);

	if (bytes_read != 17) {
		return WTAP_FILE_UNKNOWN;
	}

	magic[17] = 0;

	if (strcmp(magic, "TRSNIFF data    \x1a")) {
		return WTAP_FILE_UNKNOWN;
	}

	/* This is a ngsniffer file */
	wth->capture.ngsniffer = g_malloc(sizeof(ngsniffer_t));
	wth->subtype_read = ngsniffer_read;
	wth->snapshot_length = 16384;	/* not available in header, only in frame */

	/*
	 * Read the first record, which the manual says is a version
	 * record.
	 */
	bytes_read = fread(record_type, 1, 2, wth->fh);
	bytes_read += fread(record_length, 1, 4, wth->fh);
	if (bytes_read != 6) {
		free(wth->capture.ngsniffer);
		return WTAP_FILE_UNKNOWN;
	}

	type = pletohs(record_type);
	length = pletohs(record_length);

	if (type != REC_VERS) {
		g_message("ngsniffer: Sniffer file doesn't start with a version record");
		free(wth->capture.ngsniffer);
		return WTAP_FILE_UNKNOWN;
	}

	fread(&version, 1, sizeof version, wth->fh);

	/* Make sure this is an uncompressed Sniffer file */
	if (version.format != 1) {
		g_message("ngsniffer: Compressed Sniffer files are not supported");
		free(wth->capture.ngsniffer);
		return WTAP_FILE_UNKNOWN;
	}

	/* Get data link type */
	if (version.network >= NUM_NGSNIFF_ENCAPS) {
		g_message("ngsniffer: network type %d unknown", version.network);
		free(wth->capture.ngsniffer);
		return WTAP_FILE_UNKNOWN;
	}
	else {
		wth->file_encap = sniffer_encap[version.network];
	}

	/* Get time unit */
	if (version.timeunit >= NUM_NGSNIFF_TIMEUNITS) {
		g_message("ngsniffer: Unknown timeunit %d", version.timeunit);
		free(wth->capture.ngsniffer);
		return WTAP_FILE_UNKNOWN;
	}
	else {
		wth->capture.ngsniffer->timeunit = Usec[version.timeunit];
	}

	/* Get capture start time */
	start_time = pletohs(&version.time);
	start_date = pletohs(&version.date);
	tm.tm_year = ((start_date&0xfe00)>>9) + 1980 - 1900;
	tm.tm_mon = ((start_date&0x1e0)>>5) - 1;
	tm.tm_mday = (start_date&0x1f);
	/* The time does not appear to act as an offset; only the date
	tm.tm_hour = (start_time&0xf800)>>11;
	tm.tm_min = (start_time&0x7e0)>>5;
	tm.tm_sec = (start_time&0x1f)<<1;*/
	tm.tm_hour = 0;
	tm.tm_min = 0;
	tm.tm_sec = 0;
	tm.tm_isdst = -1;
	wth->capture.ngsniffer->start = mktime(&tm);
	/*
	 * XXX - what if "secs" is -1?  Unlikely,
	 * but if the capture was done in a time
	 * zone that switches between standard and
	 * summer time sometime other than when we
	 * do, and thus the time was one that doesn't
	 * exist here because a switch from standard
	 * to summer time zips over it, it could
	 * happen.
	 *
	 * On the other hand, if the capture was done
	 * in a different time zone, this won't work
	 * right anyway; unfortunately, the time zone
	 * isn't stored in the capture file.
	 */

	/*
	 * Unless this is an ATM capture file, we won't have to read
	 * any data records to figure out the network type; zero out
	 * "pkt_len" to indicate that we've not read the first packet's
	 * header.
	 */
	wth->capture.ngsniffer->pkt_len = 0;
	wth->capture.ngsniffer->is_atm = 0;

	if (version.network == NGSNIFF_ENCAP_ATM) {
		/*
		 * Gak.  There's no link-layer header; however, the
		 * frame is probably the data that AAL5 handed up,
		 * which contains RFC 1483 stuff or LANE stuff or SPANS
		 * stuff or whatever, or, if the "frame" is just an
		 * ATM cell, it's probably just the cell.
		 *
		 * I.e., there *is* no appropriate "link type".
		 *
		 * So what we do is scan forward, looking for the first
		 * LANE or RFC 1483 LLC-multiplexed frame, use that to
		 * determine the link type, and arrange to discard all
		 * frames other than those of that type.  That denies
		 * "libpcap" applications access to anything other
		 * than that, but I *suspect* you don't get a mix of
		 * RFC 1483 LLC-multiplexed and LANE traffic, and
		 * the applications probably would have no idea how
		 * to cope with raw cells or other types of frames
		 * anyway; the only place you lose is with FORE
		 * SPANS.
		 */
		wth->file_encap = get_atm_linktype(wth);
		if (wth->file_encap == -1) {
			/*
			 * Oops, we couldn't find a link type we can
			 * handle.
			 */
			g_message("ngsniffer: no LANE or RFC 1483 LLC-multiplexed frames found");
			free(wth->capture.ngsniffer);
			return WTAP_FILE_UNKNOWN;
		}
		wth->capture.ngsniffer->is_atm = 1;
	}
	return WTAP_FILE_NGSNIFFER;
}

static int get_atm_linktype(wtap *wth)
{
	int	bytes_read;
	char record_type[2];
	char record_length[4]; /* only 1st 2 bytes are length */
	guint16 type, length;
	struct frame4_rec frame4;
	int linktype;
	guint16 time_low, time_med, time_high, true_size, size;

	for (;;) {
		/*
		 * Read the record header.
		 */
		bytes_read = fread(record_type, 1, 2, wth->fh);
		bytes_read += fread(record_length, 1, 4, wth->fh);
		if (bytes_read != 6) {
			/*
			 * End of file or error.  Probably means there *are*
			 * no LANE or RFC 1483 LLC-multiplexed frames,
			 * which means we can't handle this in
			 * "wiretap".  Return an error.
			 */
			return -1;
		}

		type = pletohs(record_type);
		length = pletohs(record_length);

		if (type == REC_EOF) {
			/*
			 * End of file.  Probably means there *are*
			 * no LANE or RFC 1483 LLC-multiplexed frames,
			 * which means we can't handle this in
			 * "wiretap".  Return an error.
			 */
			return -1;
		}

		switch (type) {

		case REC_FRAME2:
			/*
			 * Umm, we're not supposed to get these in an
			 * ATM Sniffer file; return an error.
			 */
			return -1;

		case REC_FRAME4:
			/* Read the f_frame4_struct */
			bytes_read = fread(&frame4, 1, sizeof frame4, wth->fh);
			if (bytes_read != sizeof frame4) {
				/*
				 * Read error or short record.  Return
				 * an error.
				 */
				return -1;
			}
			time_low = pletohs(&frame4.time_low);
			time_med = pletohs(&frame4.time_med);
			time_high = frame4.time_high;
			size = pletohs(&frame4.size);
			true_size = pletohs(&frame4.true_size);

			length -= sizeof frame4;	/* we already read that much */

			/*
			 * Skip it if it's not an AAL5 frame.
			 */
			if ((frame4.atm_info.AppTrafType & ATT_AALTYPE)
			    != ATT_AAL5)
				break;

			/*
			 * OK, can we determine a network type from it?
			 */
			linktype = linktype_for_packet(
			    frame4.atm_info.AppTrafType,
			    frame4.atm_info.AppHLType);
			if (linktype == -1) {
				/*
				 * Nope - keep looking.
				 */
				break;
			}

			/*
			 * XXX - use the "time_day" field?  Is that for captures
			 * that take a *really* long time?
			 */
			wth->capture.ngsniffer->t = (double)time_low+(double)(time_med)*65536.0 +
			    (double)time_high*4294967296.0;

			wth->capture.ngsniffer->true_size = true_size;
			wth->capture.ngsniffer->size = size;
			goto found;

		default:
			break;	/* unknown type, skip it */
		}

		/*
		 * Well, we don't know what it is, or we know what
		 * it is but can't handle it.  Skip past the data
		 * portion, and keep looping.
		 */
		fseek(wth->fh, length, SEEK_CUR);
	}

found:
	/*
	 * Note that we've already read the header, so that we don't do
	 * it in "ngsniffer_read()"; we've saved the stuff we need
	 * from the header, so we use it instead - and we save the
	 * length of the record, so we know how much to read.
	 */
	wth->capture.ngsniffer->pkt_len = length;
	return (linktype);
}

static int linktype_for_packet(u_int app_traf_type, u_int app_hl_type)
{
	switch (app_traf_type & ATT_HLTYPE) {

	case ATT_HL_LLCMX:
		return (WTAP_ENCAP_ATM_RFC1483);

	case ATT_HL_LANE:
		/*
		 * OK, is it 802.3, 802.5, or "none of the above"?
		 */
		switch (app_hl_type) {

		case AHLT_LANE_802_3:
		case AHLT_LANE_802_3_MC:
			return (WTAP_ENCAP_ETHERNET);

		case AHLT_LANE_802_5:
		case AHLT_LANE_802_5_MC:
			return (WTAP_ENCAP_TR);
		}
	}

	/*
	 * Keep looking until we find one of the above or run out of
	 * capture file.
	 */
	return (-1);
}

/* Read the next packet */
int ngsniffer_read(wtap *wth)
{
	int	packet_size = wth->capture.ngsniffer->pkt_len;
	int	bytes_read;
	char record_type[2];
	char record_length[4]; /* only 1st 2 bytes are length */
	guint16 type, length;
	struct frame2_rec frame2;
	struct frame4_rec frame4;
	int linktype;
	double t;
	guint16 time_low, time_med, time_high, true_size, size;
	int	data_offset;

	/*
	 * If we've already read the first packet's header, get the
	 * information we saved from it, and skip ahead and read
	 * the data.
	 */
	if (wth->capture.ngsniffer->pkt_len != 0) {
		length = wth->capture.ngsniffer->pkt_len;
		true_size = wth->capture.ngsniffer->true_size;
		size = wth->capture.ngsniffer->size;
		t = wth->capture.ngsniffer->t;
		wth->capture.ngsniffer->pkt_len = 0;	/* won't have read header for next packet */
		goto found;
	}

	for (;;) {
		/*
		 * Read the record header.
		 */
		bytes_read = fread(record_type, 1, 2, wth->fh);
		bytes_read += fread(record_length, 1, 4, wth->fh);
		if (bytes_read != 6) {
			/*
			 * End of file or error.
			 */
			return 0;
		}

		type = pletohs(record_type);
		length = pletohs(record_length);

		switch (type) {

		case REC_FRAME2:
			/* Read the f_frame2_struct */
			bytes_read = fread(&frame2, 1, sizeof frame2, wth->fh);
			if (bytes_read != sizeof frame2) {
				/*
				 * Read error or short record.  Return
				 * an error.
				 */
				g_message("ngsniffer_read: not enough frame2 data (%d bytes)",
					bytes_read);
				return 0;
			}
			time_low = pletohs(&frame2.time_low);
			time_med = pletohs(&frame2.time_med);
			time_high = pletohs(&frame2.time_high);
			size = pletohs(&frame2.size);
			true_size = pletohs(&frame2.true_size);

			length -= sizeof frame2;	/* we already read that much */

			t = (double)time_low+(double)(time_med)*65536.0 +
			    (double)time_high*4294967296.0;

			wth->phdr.flags = frame2.fs & 0x80;

			goto found;

		case REC_FRAME4:
			/* Read the f_frame4_struct */
			bytes_read = fread(&frame4, 1, sizeof frame4, wth->fh);
			if (bytes_read != sizeof frame4) {
				/*
				 * Read error or short record.  Return
				 * an error.
				 */
				g_message("ngsniffer_read: not enough frame4 data (%d bytes)",
					bytes_read);
				return 0;
			}
			time_low = pletohs(&frame4.time_low);
			time_med = pletohs(&frame4.time_med);
			time_high = frame4.time_high;
			size = pletohs(&frame4.size);
			true_size = pletohs(&frame4.true_size);

			length -= sizeof frame4;	/* we already read that much */

			/*
			 * Skip it if it's not an AAL5 frame.
			 */
			if ((frame4.atm_info.AppTrafType & ATT_AALTYPE)
			    != ATT_AAL5)
				break;

			/*
			 * Skip it if it's not a frame of the type we're
			 * handling.
			 */
			linktype = linktype_for_packet(
			    frame4.atm_info.AppTrafType,
			    frame4.atm_info.AppHLType);
			if (wth->file_encap != linktype)
				break;

			/*
			 * XXX - use the "time_day" field?  Is that for captures
			 * that take a *really* long time?
			 */
			t = (double)time_low+(double)(time_med)*65536.0 +
			    (double)time_high*4294967296.0;

			goto found;

		case REC_EOF:
			/*
			 * End of file.  Return an EOF indication.
			 */
			return 0;

		default:
			break;	/* unknown type, skip it */
		}

		/*
		 * Well, we don't know what it is, or we know what
		 * it is but can't handle it.  Skip past the data
		 * portion, and keep looping.
		 */
		fseek(wth->fh, length, SEEK_CUR);
	}

found:
	wth->phdr.len = true_size ? true_size : size;
	wth->phdr.caplen = size;

	/*
	 * For ATM LANE, skip the 2-byte LAN Emulation header.  That
	 * leaves an Ethernet/802.3 header for Ethernet/802.3, and
	 * leaves a pad byte and an 802.5 frame control byte for 802.5.
	 */
	if (wth->capture.ngsniffer->is_atm && wth->file_encap != WTAP_ENCAP_ATM_RFC1483) {
		if (length <= ATM_LANE_HEADER_LEN)
			return -1;
		fseek(wth->fh, ATM_LANE_HEADER_LEN, SEEK_CUR);
		wth->phdr.caplen -= ATM_LANE_HEADER_LEN;
		wth->phdr.len -= ATM_LANE_HEADER_LEN;
		length -= ATM_LANE_HEADER_LEN;
	}

	/*
	 * Read the packet data.
	 */
	buffer_assure_space(wth->frame_buffer, length);
	data_offset = ftell(wth->fh);
	bytes_read = fread(buffer_start_ptr(wth->frame_buffer), 1,
			length, wth->fh);

	if (bytes_read != length) {
		if (ferror(wth->fh)) {
			g_message("ngsniffer_read: fread for data: read error\n");
		} else {
			g_message("ngsniffer_read: fread for data: %d bytes out of %d",
				bytes_read, packet_size);
		}
		return -1;
	}

	t = t/1000000.0 * wth->capture.ngsniffer->timeunit; /* t = # of secs */
	t += wth->capture.ngsniffer->start;
	wth->phdr.ts.tv_sec = (long)t;
	wth->phdr.ts.tv_usec = (unsigned long)((t-(double)(wth->phdr.ts.tv_sec))
			*1.0e6);
	wth->phdr.pkt_encap = wth->file_encap;
	return data_offset;
}
