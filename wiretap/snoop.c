/* snoop.c
 *
 * $Id: snoop.c,v 1.51 2002/05/26 21:32:39 guy Exp $
 *
 * Wiretap Library
 * Copyright (c) 1998 by Gilbert Ramirez <gram@alumni.rice.edu>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <errno.h>
#include <string.h>
#include "wtap-int.h"
#include "file_wrappers.h"
#include "buffer.h"
#include "atm.h"
#include "snoop.h"
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

/* See RFC 1761 for a description of the "snoop" file format. */

/* Magic number in "snoop" files. */
static const char snoop_magic[] = {
	's', 'n', 'o', 'o', 'p', '\0', '\0', '\0'
};

/* "snoop" file header (minus magic number). */
struct snoop_hdr {
	guint32	version;	/* version number (should be 2) */
	guint32	network;	/* network type */
};

/* "snoop" record header. */
struct snooprec_hdr {
	guint32	orig_len;	/* actual length of packet */
	guint32	incl_len;	/* number of octets captured in file */
	guint32	rec_len;	/* length of record */
	guint32	cum_drops;	/* cumulative number of dropped packets */
	guint32	ts_sec;		/* timestamp seconds */
	guint32	ts_usec;	/* timestamp microseconds */
};

/*
 * The link-layer header on ATM packets.
 */
struct snoop_atm_hdr {
	guint8	flags;		/* destination and traffic type */
	guint8	vpi;		/* VPI */
	guint16	vci;		/* VCI */
};

static gboolean snoop_read(wtap *wth, int *err, long *data_offset);
static gboolean snoop_seek_read(wtap *wth, long seek_off,
    union wtap_pseudo_header *pseudo_header, u_char *pd, int length, int *err);
static gboolean snoop_read_atm_pseudoheader(FILE_T fh,
    union wtap_pseudo_header *pseudo_header, int *err);
static gboolean snoop_read_rec_data(FILE_T fh, u_char *pd, int length,
    int *err);
static gboolean snoop_dump(wtap_dumper *wdh, const struct wtap_pkthdr *phdr,
    const union wtap_pseudo_header *pseudo_header, const u_char *pd, int *err);

/*
 * See
 * 
 *	http://www.opengroup.org/onlinepubs/9638599/apdxf.htm
 *
 * for the "dlpi.h" header file specified by The Open Group, which lists
 * the DL_ values for various protocols; Solaris 7 uses the same values.
 *
 * The page at
 *
 *	http://mrpink.lerc.nasa.gov/118x/support.html
 *
 * has links to modified versions of "tcpdump" and "libpcap" for SUNatm
 * DLPI support; they suggest the 3.0 verson of SUNatm uses those
 * values.
 *
 * It also has a link to "convert.c", which is a program to convert files
 * from the format written by the "atmsnoop" program that comes with the
 * SunATM package to regular "snoop" format, claims that "SunATM 2.1 claimed
 * to be DL_FDDI (don't ask why).  SunATM 3.0 claims to be DL_IPATM, which
 * is 0x12".
 *
 * It also says that "ATM Mac header is 12 bytes long.", and seems to imply
 * that in an "atmsnoop" file, the header contains 2 bytes (direction and
 * VPI?), 2 bytes of VCI, 6 bytes of something, and 2 bytes of Ethernet
 * type; if those 6 bytes are 2 bytes of DSAP, 2 bytes of LSAP, 1 byte
 * of LLC control, and 3 bytes of SNAP OUI, that'd mean that an ATM
 * pseudo-header in an "atmsnoop" file is probably 1 byte of direction,
 * 1 byte of VPI, and 2 bytes of VCI.
 *
 * The aforementioned page also has a link to some capture files from
 * "atmsnoop"; this version of "snoop.c" appears to be able to read them.
 *
 * Source to an "atmdump" package, which includes a modified version of
 * "libpcap" to handle SunATM DLPI and an ATM driver for FreeBSD, and
 * also includes "atmdump", which is a modified "tcpdump", says that an
 * ATM packet handed up from the Sun driver for the Sun SBus ATM card on
 * Solaris 2.5.1 has 1 byte of direction, 1 byte of VPI, 2 bytes of VCI,
 * and then the ATM PDU, and suggests that the direction byte is 0x80 for
 * "transmitted" (presumably meaning DTE->DCE) and presumably not 0x80 for
 * "received" (presumably meaning DCE->DTE).
 *
 * In fact, the "direction" byte appears to have some other stuff, perhaps
 * a traffic type, in the lower 7 bits, with the 8th bit indicating the
 * direction.
 *
 * I don't know what the encapsulation of any of the other types is, so I
 * leave them all as WTAP_ENCAP_UNKNOWN.  I also don't know whether "snoop"
 * can handle any of them (it presumably can't handle ATM, otherwise Sun
 * wouldn't have supplied "atmsnoop"; even if it can't, this may be useful
 * reference information for anybody doing code to use DLPI to do raw packet
 * captures on those network types.
 *
 * See
 *
 *	http://www.shomiti.com/support/TNCapFileFormat.htm
 *
 * for information on Shomiti's mutant flavor of snoop.  For some unknown
 * unknown reason, they decided not to just Go With The DLPI Flow, and
 * instead used the types unspecified in RFC 1461 for their own nefarious
 * purposes, such as distinguishing 10MB from 100MB from 1000MB Ethernet
 * and distinguishing 4MB from 16MB Token Ring, and distinguishing both
 * of them from the "Shomiti" versions of same.
 */
int snoop_open(wtap *wth, int *err)
{
	int bytes_read;
	char magic[sizeof snoop_magic];
	struct snoop_hdr hdr;
	static const int snoop_encap[] = {
		WTAP_ENCAP_ETHERNET,	/* IEEE 802.3 */
		WTAP_ENCAP_UNKNOWN,	/* IEEE 802.4 Token Bus */
		WTAP_ENCAP_TOKEN_RING,
		WTAP_ENCAP_UNKNOWN,	/* IEEE 802.6 Metro Net */
		WTAP_ENCAP_ETHERNET,
		WTAP_ENCAP_UNKNOWN,	/* HDLC */
		WTAP_ENCAP_UNKNOWN,	/* Character Synchronous, e.g. bisync */
		WTAP_ENCAP_UNKNOWN,	/* IBM Channel-to-Channel */
		WTAP_ENCAP_FDDI_BITSWAPPED,
		WTAP_ENCAP_UNKNOWN,	/* Other */
		WTAP_ENCAP_UNKNOWN,	/* Frame Relay LAPF */
		WTAP_ENCAP_UNKNOWN,	/* Multi-protocol over Frame Relay */
		WTAP_ENCAP_UNKNOWN,	/* Character Async (e.g., SLIP and PPP?) */
		WTAP_ENCAP_UNKNOWN,	/* X.25 Classical IP */
		WTAP_ENCAP_UNKNOWN,	/* software loopback */
		WTAP_ENCAP_UNKNOWN,	/* not defined in "dlpi.h" */
		WTAP_ENCAP_UNKNOWN,	/* Fibre Channel */
		WTAP_ENCAP_UNKNOWN,	/* ATM */
		WTAP_ENCAP_ATM_SNIFFER,	/* ATM Classical IP */
		WTAP_ENCAP_UNKNOWN,	/* X.25 LAPB */
		WTAP_ENCAP_UNKNOWN,	/* ISDN */
		WTAP_ENCAP_UNKNOWN,	/* HIPPI */
		WTAP_ENCAP_UNKNOWN,	/* 100VG-AnyLAN Ethernet */
		WTAP_ENCAP_UNKNOWN,	/* 100VG-AnyLAN Token Ring */
		WTAP_ENCAP_UNKNOWN,	/* "ISO 8802/3 and Ethernet" */
		WTAP_ENCAP_UNKNOWN,	/* 100BaseT (but that's just Ethernet) */
	};
	#define NUM_SNOOP_ENCAPS (sizeof snoop_encap / sizeof snoop_encap[0])
	static const int shomiti_encap[] = {
		WTAP_ENCAP_ETHERNET,	/* IEEE 802.3 */
		WTAP_ENCAP_UNKNOWN,	/* IEEE 802.4 Token Bus */
		WTAP_ENCAP_TOKEN_RING,
		WTAP_ENCAP_UNKNOWN,	/* IEEE 802.6 Metro Net */
		WTAP_ENCAP_ETHERNET,
		WTAP_ENCAP_UNKNOWN,	/* HDLC */
		WTAP_ENCAP_UNKNOWN,	/* Character Synchronous, e.g. bisync */
		WTAP_ENCAP_UNKNOWN,	/* IBM Channel-to-Channel */
		WTAP_ENCAP_FDDI_BITSWAPPED,
		WTAP_ENCAP_UNKNOWN,	/* Other */
		WTAP_ENCAP_ETHERNET,	/* Fast Ethernet */
		WTAP_ENCAP_TOKEN_RING,	/* 4MB 802.5 token ring */
		WTAP_ENCAP_ETHERNET,	/* Gigabit Ethernet */
		WTAP_ENCAP_TOKEN_RING,	/* "IEEE 802.5 Shomiti" */
		WTAP_ENCAP_TOKEN_RING,	/* "4MB IEEE 802.5 Shomiti" */
	};
	#define NUM_SHOMITI_ENCAPS (sizeof shomiti_encap / sizeof shomiti_encap[0])
	int file_encap;

	/* Read in the string that should be at the start of a "snoop" file */
	errno = WTAP_ERR_CANT_READ;
	bytes_read = file_read(magic, 1, sizeof magic, wth->fh);
	if (bytes_read != sizeof magic) {
		*err = file_error(wth->fh);
		if (*err != 0)
			return -1;
		return 0;
	}
	wth->data_offset += sizeof magic;

	if (memcmp(magic, snoop_magic, sizeof snoop_magic) != 0) {
		return 0;
	}

	/* Read the rest of the header. */
	errno = WTAP_ERR_CANT_READ;
	bytes_read = file_read(&hdr, 1, sizeof hdr, wth->fh);
	if (bytes_read != sizeof hdr) {
		*err = file_error(wth->fh);
		if (*err != 0)
			return -1;
		return 0;
	}
	wth->data_offset += sizeof hdr;

	/*
	 * Oh, this is lovely.
	 *
	 * I suppose Shomiti could give a bunch of lawyerly noise about
	 * how "well, RFC 1761 said they were unassigned, and that's
	 * the standard, not the DLPI header file, so it's perfectly OK
	 * for us to use them, blah blah blah", but it's still irritating
	 * as hell that they used the unassigned-in-RFC-1761 values for
	 * their own purposes - especially given that Sun also used
	 * one of them in atmsnoop.
	 *
	 * For now, we treat a version number of 2 as indicating that
	 * this is a Sun snoop file, and version numbers of 3, 4, and 5
	 * as indicating that this is a Shomiti file, even though
	 * their capture file format documentation claims that they
	 * use 2 if the data "was captured using an NDIS card", which
	 * presumably means "captured with an ordinary boring network
	 * card via NDIS" as opposed to "captured with our whizzo
	 * special capture hardware".
	 *
	 * This runs the risk that we may misinterpret the network
	 * type of Shomiti captures not done using their hardware.
	 * Currently, the only not-in-RFC-1761 type we interpret in
	 * Sun snoop files is 18, for atmsnoop, and that's not used
	 * by Shomiti, but if any of the types used by Shomiti are
	 * also used by Snoop or a variant thereof - e.g.:
	 *
	 *	value	snoop			Shomiti
	 *	10	Frame Relay		100MB Ethernet
	 *	11	MP over Frame Relay	4MB 802.5
	 *	12	"Character Async"	1000MB Ethernet
	 *	13	X.25 Classical IP	"IEEE 802.5 Shomiti"
	 *	14	"software loopback"	"4MB IEEE 802.5 Shomiti"
	 *
	 * then we have a problem that may be resolvable only by checking
	 * how much padding there is in the first packet - if there're 3
	 * bytes or less, it's probably Sun snoop, which uses the padding
	 * only for padding, but if there's more, it's probably a Shomiti
	 * tool, which uses the padding for additional information.
	 */
	hdr.version = ntohl(hdr.version);
	hdr.network = ntohl(hdr.network);
	switch (hdr.version) {

	case 2:		/* Solaris 2.x and later snoop, and Shomiti
			   Surveyor prior to 3.0 (or 3.x with NDIS card?) */
		if (hdr.network >= NUM_SNOOP_ENCAPS
		    || snoop_encap[hdr.network] == WTAP_ENCAP_UNKNOWN) {
			g_message("snoop: network type %u unknown or unsupported",
			    hdr.network);
			*err = WTAP_ERR_UNSUPPORTED_ENCAP;
			return -1;
		}
		file_encap = snoop_encap[hdr.network];
		break;

	case 3:		/* Surveyor 3.0 and later, with Shomiti CMM2 hardware */
	case 4:		/* Surveyor 3.0 and later, with Shomiti GAM hardware */
	case 5:		/* Surveyor 3.0 and later, with Shomiti THG hardware */
		if (hdr.network >= NUM_SHOMITI_ENCAPS
		    || shomiti_encap[hdr.network] == WTAP_ENCAP_UNKNOWN) {
			g_message("snoop: Shomiti network type %u unknown or unsupported",
			    hdr.network);
			*err = WTAP_ERR_UNSUPPORTED_ENCAP;
			return -1;
		}
		file_encap = shomiti_encap[hdr.network];
		break;

	default:
		g_message("snoop: version %u unsupported", hdr.version);
		*err = WTAP_ERR_UNSUPPORTED;
		return -1;
	}

	/* This is a snoop file */
	wth->file_type = WTAP_FILE_SNOOP;
	wth->subtype_read = snoop_read;
	wth->subtype_seek_read = snoop_seek_read;
	wth->file_encap = file_encap;
	wth->snapshot_length = 0;	/* not available in header */
	return 1;
}

/* Read the next packet */
static gboolean snoop_read(wtap *wth, int *err, long *data_offset)
{
	guint32 rec_size;
	guint32	packet_size;
	guint32 orig_size;
	int	bytes_read;
	struct snooprec_hdr hdr;
	char	padbuf[4];
	int	padbytes;
	int	bytes_to_read;

	/* Read record header. */
	errno = WTAP_ERR_CANT_READ;
	bytes_read = file_read(&hdr, 1, sizeof hdr, wth->fh);
	if (bytes_read != sizeof hdr) {
		*err = file_error(wth->fh);
		if (*err == 0 && bytes_read != 0) {
			*err = WTAP_ERR_SHORT_READ;
		}
		return FALSE;
	}
	wth->data_offset += sizeof hdr;

	rec_size = ntohl(hdr.rec_len);
	orig_size = ntohl(hdr.orig_len);
	packet_size = ntohl(hdr.incl_len);
	if (packet_size > WTAP_MAX_PACKET_SIZE) {
		/*
		 * Probably a corrupt capture file; don't blow up trying
		 * to allocate space for an immensely-large packet.
		 */
		g_message("snoop: File has %u-byte packet, bigger than maximum of %u",
		    packet_size, WTAP_MAX_PACKET_SIZE);
		*err = WTAP_ERR_BAD_RECORD;
		return FALSE;
	}

	*data_offset = wth->data_offset;

	/*
	 * If this is an ATM packet, the first four bytes are the
	 * direction of the packet (transmit/receive), the VPI, and
	 * the VCI; read them and generate the pseudo-header from
	 * them.
	 */
	if (wth->file_encap == WTAP_ENCAP_ATM_SNIFFER) {
		if (packet_size < sizeof (struct snoop_atm_hdr)) {
			/*
			 * Uh-oh, the packet isn't big enough to even
			 * have a pseudo-header.
			 */
			g_message("snoop: atmsnoop file has a %u-byte packet, too small to have even an ATM pseudo-header\n",
			    packet_size);
			*err = WTAP_ERR_BAD_RECORD;
			return FALSE;
		}
		if (!snoop_read_atm_pseudoheader(wth->fh, &wth->pseudo_header,
		    err))
			return FALSE;	/* Read error */

		/*
		 * Don't count the pseudo-header as part of the packet.
		 */
		rec_size -= sizeof (struct snoop_atm_hdr);
		orig_size -= sizeof (struct snoop_atm_hdr);
		packet_size -= sizeof (struct snoop_atm_hdr);
		wth->data_offset += sizeof (struct snoop_atm_hdr);
	}

	buffer_assure_space(wth->frame_buffer, packet_size);
	if (!snoop_read_rec_data(wth->fh, buffer_start_ptr(wth->frame_buffer),
	    packet_size, err))
		return FALSE;	/* Read error */
	wth->data_offset += packet_size;

	wth->phdr.ts.tv_sec = ntohl(hdr.ts_sec);
	wth->phdr.ts.tv_usec = ntohl(hdr.ts_usec);
	wth->phdr.caplen = packet_size;
	wth->phdr.len = orig_size;
	wth->phdr.pkt_encap = wth->file_encap;

	/*
	 * If this is ATM LANE traffic, try to guess what type of LANE
	 * traffic it is based on the packet contents.
	 */
	if (wth->file_encap == WTAP_ENCAP_ATM_SNIFFER &&
	    wth->pseudo_header.atm.type == TRAF_LANE) {
		atm_guess_lane_type(buffer_start_ptr(wth->frame_buffer),
		    wth->phdr.caplen, &wth->pseudo_header);
	}

	/*
	 * Skip over the padding (don't "fseek()", as the standard
	 * I/O library on some platforms discards buffered data if
	 * you do that, which means it does a lot more reads).
	 * There's probably not much padding (it's probably padded only
	 * to a 4-byte boundary), so we probably need only do one read.
	 */
	padbytes = rec_size - (sizeof hdr + packet_size);
	while (padbytes != 0) {
		bytes_to_read = padbytes;
		if ((unsigned)bytes_to_read > sizeof padbuf)
			bytes_to_read = sizeof padbuf;
		errno = WTAP_ERR_CANT_READ;
		bytes_read = file_read(padbuf, 1, bytes_to_read, wth->fh);
		if (bytes_read != bytes_to_read) {
			*err = file_error(wth->fh);
			if (*err == 0)
				*err = WTAP_ERR_SHORT_READ;
			return FALSE;
		}
		wth->data_offset += bytes_read;
		padbytes -= bytes_read;
	}

	return TRUE;
}

static gboolean
snoop_seek_read(wtap *wth, long seek_off,
    union wtap_pseudo_header *pseudo_header, u_char *pd, int length, int *err)
{
	if (file_seek(wth->random_fh, seek_off, SEEK_SET) == -1) {
		*err = file_error(wth->random_fh);
		return FALSE;
	}

	if (wth->file_encap == WTAP_ENCAP_ATM_SNIFFER) {
		if (!snoop_read_atm_pseudoheader(wth->random_fh, pseudo_header,
		    err)) {
			/* Read error */
			return FALSE;
		}
	}

	/*
	 * Read the packet data.
	 */
	if (!snoop_read_rec_data(wth->random_fh, pd, length, err))
		return FALSE;	/* failed */

	/*
	 * If this is ATM LANE traffic, try to guess what type of LANE
	 * traffic it is based on the packet contents.
	 */
	if (wth->file_encap == WTAP_ENCAP_ATM_SNIFFER &&
	    pseudo_header->atm.type == TRAF_LANE)
		atm_guess_lane_type(pd, length, pseudo_header);
	return TRUE;
}

static gboolean
snoop_read_atm_pseudoheader(FILE_T fh, union wtap_pseudo_header *pseudo_header,
    int *err)
{
	struct snoop_atm_hdr atm_phdr;
	int	bytes_read;
	guint8	vpi;
	guint16	vci;

	errno = WTAP_ERR_CANT_READ;
	bytes_read = file_read(&atm_phdr, 1, sizeof (struct snoop_atm_hdr), fh);
	if (bytes_read != sizeof (struct snoop_atm_hdr)) {
		*err = file_error(fh);
		if (*err == 0)
			*err = WTAP_ERR_SHORT_READ;
		return FALSE;
	}

	vpi = atm_phdr.vpi;
	vci = pntohs(&atm_phdr.vci);

	/*
	 * The lower 4 bits of the first byte of the header indicate
	 * the type of traffic, as per the "atmioctl.h" header in
	 * SunATM.
	 */
	switch (atm_phdr.flags & 0x0F) {

	case 0x01:	/* LANE */
		pseudo_header->atm.aal = AAL_5;
		pseudo_header->atm.type = TRAF_LANE;
		break;

	case 0x02:	/* RFC 1483 LLC multiplexed traffic */
		pseudo_header->atm.aal = AAL_5;
		pseudo_header->atm.type = TRAF_LLCMX;
		break;

	case 0x05:	/* ILMI */
		pseudo_header->atm.aal = AAL_5;
		pseudo_header->atm.type = TRAF_ILMI;
		break;

	case 0x06:	/* Signalling AAL */
		pseudo_header->atm.aal = AAL_SIGNALLING;
		pseudo_header->atm.type = TRAF_UNKNOWN;
		break;

	case 0x03:	/* MARS (RFC 2022) */
		pseudo_header->atm.aal = AAL_5;
		pseudo_header->atm.type = TRAF_UNKNOWN;
		break;

	case 0x04:	/* IFMP (Ipsilon Flow Management Protocol; see RFC 1954) */
		pseudo_header->atm.aal = AAL_5;
		pseudo_header->atm.type = TRAF_UNKNOWN;	/* XXX - TRAF_IPSILON? */
		break;

	default:
		/*
		 * Assume it's AAL5, unless it's VPI 0 and VCI 5, in which
		 * case assume it's AAL_SIGNALLING; we know nothing more
		 * about it.
		 *
		 * XXX - is this necessary?  Or are we guaranteed that
		 * all signalling traffic has a type of 0x06?
		 *
		 * XXX - is this guaranteed to be AAL5?  Or, if the type is
		 * 0x00 ("raw"), might it be non-AAL5 traffic?
		 */
		if (vpi == 0 && vci == 5)
			pseudo_header->atm.aal = AAL_SIGNALLING;
		else
			pseudo_header->atm.aal = AAL_5;
		pseudo_header->atm.type = TRAF_UNKNOWN;
		break;
	}
	pseudo_header->atm.subtype = TRAF_ST_UNKNOWN;

	pseudo_header->atm.vpi = vpi;
	pseudo_header->atm.vci = vci;
	pseudo_header->atm.channel = (atm_phdr.flags & 0x80) ? 1 : 0;

	/* We don't have this information */
	pseudo_header->atm.cells = 0;
	pseudo_header->atm.aal5t_u2u = 0;
	pseudo_header->atm.aal5t_len = 0;
	pseudo_header->atm.aal5t_chksum = 0;

	return TRUE;
}

static gboolean
snoop_read_rec_data(FILE_T fh, u_char *pd, int length, int *err)
{
	int	bytes_read;

	errno = WTAP_ERR_CANT_READ;
	bytes_read = file_read(pd, 1, length, fh);

	if (bytes_read != length) {
		*err = file_error(fh);
		if (*err == 0)
			*err = WTAP_ERR_SHORT_READ;
		return FALSE;
	}
	return TRUE;
}

static const int wtap_encap[] = {
	-1,		/* WTAP_ENCAP_UNKNOWN -> unsupported */
	0x04,		/* WTAP_ENCAP_ETHERNET -> DL_ETHER */
	0x02,		/* WTAP_ENCAP_TOKEN_RING -> DL_TPR */
	-1,		/* WTAP_ENCAP_SLIP -> unsupported */
	-1,		/* WTAP_ENCAP_PPP -> unsupported */
	0x08,		/* WTAP_ENCAP_FDDI -> DL_FDDI */
	0x08,		/* WTAP_ENCAP_FDDI_BITSWAPPED -> DL_FDDI */
	-1,		/* WTAP_ENCAP_RAW_IP -> unsupported */
	-1,		/* WTAP_ENCAP_ARCNET -> unsupported */
	-1,		/* WTAP_ENCAP_ATM_RFC1483 -> unsupported */
	-1,		/* WTAP_ENCAP_LINUX_ATM_CLIP -> unsupported */
	-1,		/* WTAP_ENCAP_LAPB -> unsupported*/
	0x12,		/* WTAP_ENCAP_ATM_SNIFFER -> DL_IPATM */
	-1		/* WTAP_ENCAP_NULL -> unsupported */
};
#define NUM_WTAP_ENCAPS (sizeof wtap_encap / sizeof wtap_encap[0])

/* Returns 0 if we could write the specified encapsulation type,
   an error indication otherwise. */
int snoop_dump_can_write_encap(int encap)
{
	/* Per-packet encapsulations aren't supported. */
	if (encap == WTAP_ENCAP_PER_PACKET)
		return WTAP_ERR_ENCAP_PER_PACKET_UNSUPPORTED;

	if (encap < 0 || (unsigned)encap >= NUM_WTAP_ENCAPS || wtap_encap[encap] == -1)
		return WTAP_ERR_UNSUPPORTED_ENCAP;

	return 0;
}

/* Returns TRUE on success, FALSE on failure; sets "*err" to an error code on
   failure */
gboolean snoop_dump_open(wtap_dumper *wdh, int *err)
{
	struct snoop_hdr file_hdr;
	size_t nwritten;

	/* This is a snoop file */
	wdh->subtype_write = snoop_dump;
	wdh->subtype_close = NULL;

	/* Write the file header. */
	nwritten = fwrite(&snoop_magic, 1, sizeof snoop_magic, wdh->fh);
	if (nwritten != sizeof snoop_magic) {
		if (nwritten == 0 && ferror(wdh->fh))
			*err = errno;
		else
			*err = WTAP_ERR_SHORT_WRITE;
		return FALSE;
	}

	/* current "snoop" format is 2 */
	file_hdr.version = htonl(2);
	file_hdr.network = htonl(wtap_encap[wdh->encap]);
	nwritten = fwrite(&file_hdr, 1, sizeof file_hdr, wdh->fh);
	if (nwritten != sizeof file_hdr) {
		if (nwritten == 0 && ferror(wdh->fh))
			*err = errno;
		else
			*err = WTAP_ERR_SHORT_WRITE;
		return FALSE;
	}

	return TRUE;
}

/* Write a record for a packet to a dump file.
   Returns TRUE on success, FALSE on failure. */
static gboolean snoop_dump(wtap_dumper *wdh,
	const struct wtap_pkthdr *phdr,
	const union wtap_pseudo_header *pseudo_header _U_,
	const u_char *pd, int *err)
{
	struct snooprec_hdr rec_hdr;
	size_t nwritten;
	int reclen;
	guint padlen;
	static char zeroes[4];
	struct snoop_atm_hdr atm_hdr;
	int atm_hdrsize;

	if (wdh->encap == WTAP_ENCAP_ATM_SNIFFER)
		atm_hdrsize = sizeof (struct snoop_atm_hdr);
	else
		atm_hdrsize = 0;

	/* Record length = header length plus data length... */
	reclen = sizeof rec_hdr + phdr->caplen + atm_hdrsize;

	/* ... plus enough bytes to pad it to a 4-byte boundary. */
	padlen = ((reclen + 3) & ~3) - reclen;
	reclen += padlen;

	rec_hdr.orig_len = htonl(phdr->len + atm_hdrsize);
	rec_hdr.incl_len = htonl(phdr->caplen + atm_hdrsize);
	rec_hdr.rec_len = htonl(reclen);
	rec_hdr.cum_drops = 0;
	rec_hdr.ts_sec = htonl(phdr->ts.tv_sec);
	rec_hdr.ts_usec = htonl(phdr->ts.tv_usec);
	nwritten = fwrite(&rec_hdr, 1, sizeof rec_hdr, wdh->fh);
	if (nwritten != sizeof rec_hdr) {
		if (nwritten == 0 && ferror(wdh->fh))
			*err = errno;
		else
			*err = WTAP_ERR_SHORT_WRITE;
		return FALSE;
	}

	if (wdh->encap == WTAP_ENCAP_ATM_SNIFFER) {
		/*
		 * Write the ATM header.
		 */
		atm_hdr.flags =
		    (pseudo_header->atm.channel != 0) ? 0x80 : 0x00;
		switch (pseudo_header->atm.aal) {

		case AAL_SIGNALLING:
			/* Signalling AAL */
			atm_hdr.flags |= 0x06;
			break;

		case AAL_5:
			switch (pseudo_header->atm.type) {

			case TRAF_LANE:
				/* LANE */
				atm_hdr.flags |= 0x01;
				break;

			case TRAF_LLCMX:
				/* RFC 1483 LLC multiplexed traffic */
				atm_hdr.flags |= 0x02;
				break;

			case TRAF_ILMI:
				/* ILMI */
				atm_hdr.flags |= 0x05;
				break;
			}
			break;
		}
		atm_hdr.vpi = pseudo_header->atm.vpi;
		atm_hdr.vci = htons(pseudo_header->atm.vci);
		nwritten = fwrite(&atm_hdr, 1, sizeof atm_hdr, wdh->fh);
		if (nwritten != sizeof atm_hdr) {
			if (nwritten == 0 && ferror(wdh->fh))
				*err = errno;
			else
				*err = WTAP_ERR_SHORT_WRITE;
			return FALSE;
		}
	}

	nwritten = fwrite(pd, 1, phdr->caplen, wdh->fh);
	if (nwritten != phdr->caplen) {
		if (nwritten == 0 && ferror(wdh->fh))
			*err = errno;
		else
			*err = WTAP_ERR_SHORT_WRITE;
		return FALSE;
	}

	/* Now write the padding. */
	nwritten = fwrite(zeroes, 1, padlen, wdh->fh);
	if (nwritten != padlen) {
		if (nwritten == 0 && ferror(wdh->fh))
			*err = errno;
		else
			*err = WTAP_ERR_SHORT_WRITE;
		return FALSE;
	}
	return TRUE;
}
