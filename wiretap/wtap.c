/* wtap.c
 *
 * $Id$
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

#include <string.h>
#include <errno.h>

#include "wtap-int.h"
#include "file_wrappers.h"
#include "buffer.h"

int
wtap_fd(wtap *wth)
{
	return wth->fd;
}

int
wtap_file_type(wtap *wth)
{
	return wth->file_type;
}

int
wtap_snapshot_length(wtap *wth)
{
	return wth->snapshot_length;
}

int
wtap_file_encap(wtap *wth)
{
	return wth->file_encap;
}

/* Table of the encapsulation types we know about. */
static const struct encap_type_info {
	const char *name;
	const char *short_name;
} encap_table[WTAP_NUM_ENCAP_TYPES] = {
	/* WTAP_ENCAP_UNKNOWN */
	{ "Unknown", NULL },

	/* WTAP_ENCAP_ETHERNET */
	{ "Ethernet", "ether" },

	/* WTAP_ENCAP_TOKEN_RING */
	{ "Token Ring", "tr" },

	/* WTAP_ENCAP_SLIP */
	{ "SLIP", "slip" },

	/* WTAP_ENCAP_PPP */
	{ "PPP", "ppp" },

	/* WTAP_ENCAP_FDDI */
	{ "FDDI", "fddi" },

	/* WTAP_ENCAP_FDDI_BITSWAPPED */
	{ "FDDI with bit-swapped MAC addresses", "fddi-swapped" },

	/* WTAP_ENCAP_RAW_IP */
	{ "Raw IP", "rawip" },

	/* WTAP_ENCAP_ARCNET */
	{ "ARCNET", "arcnet" },

	/* WTAP_ENCAP_ARCNET_LINUX */
	{ "Linux ARCNET", "arcnet_linux" },

	/* WTAP_ENCAP_ATM_RFC1483 */
	{ "RFC 1483 ATM", "atm-rfc1483" },

	/* WTAP_ENCAP_LINUX_ATM_CLIP */
	{ "Linux ATM CLIP", "linux-atm-clip" },

	/* WTAP_ENCAP_LAPB */
	{ "LAPB", "lapb" },

	/* WTAP_ENCAP_ATM_PDUS */
	{ "ATM PDUs", "atm-pdus" },

	/* WTAP_ENCAP_ATM_PDUS_UNTRUNCATED */
	{ "ATM PDUs - untruncated", "atm-pdus-untruncated" },

	/* WTAP_ENCAP_NULL */
	{ "NULL", "null" },

	/* WTAP_ENCAP_ASCEND */
	{ "Lucent/Ascend access equipment", "ascend" },

	/* WTAP_ENCAP_ISDN */
	{ "ISDN", "isdn" },

	/* WTAP_ENCAP_IP_OVER_FC */
	{ "RFC 2625 IP-over-Fibre Channel", "ip-over-fc" },

	/* WTAP_ENCAP_PPP_WITH_PHDR */
	{ "PPP with Directional Info", "ppp-with-direction" },

	/* WTAP_ENCAP_IEEE_802_11 */
	{ "IEEE 802.11 Wireless LAN", "ieee-802-11" },

	/* WTAP_ENCAP_PRISM_HEADER */
	{ "IEEE 802.11 plus Prism II monitor mode header", "prism" },

	/* WTAP_ENCAP_IEEE_802_11_WITH_RADIO */
	{ "IEEE 802.11 Wireless LAN with radio information", "ieee-802-11-radio" },

	/* WTAP_ENCAP_IEEE_802_11_WLAN_RADIOTAP */
	{ "IEEE 802.11 plus radiotap WLAN header", "ieee-802-11-radiotap" },

	/* WTAP_ENCAP_IEEE_802_11_WLAN_AVS */
	{ "IEEE 802.11 plus AVS WLAN header", "ieee-802-11-avs" },

	/* WTAP_ENCAP_SLL */
	{ "Linux cooked-mode capture", "linux-sll" },

	/* WTAP_ENCAP_FRELAY */
	{ "Frame Relay", "frelay" },

	/* WTAP_ENCAP_FRELAY_WITH_PHDR */
	{ "Frame Relay with Directional Info", "frelay-with-direction" },

	/* WTAP_ENCAP_CHDLC */
	{ "Cisco HDLC", "chdlc" },

	/* WTAP_ENCAP_CISCO_IOS */
	{ "Cisco IOS internal", "ios" },

	/* WTAP_ENCAP_LOCALTALK */
	{ "Localtalk", "ltalk" },

	/* WTAP_ENCAP_OLD_PFLOG  */
	{ "OpenBSD PF Firewall logs, pre-3.4", "pflog-old" },

	/* WTAP_ENCAP_HHDLC */
	{ "HiPath HDLC", "hhdlc" },

	/* WTAP_ENCAP_DOCSIS */
	{ "Data Over Cable Service Interface Specification", "docsis" },

	/* WTAP_ENCAP_COSINE */
	{ "CoSine L2 debug log", "cosine" },

	/* WTAP_ENCAP_WFLEET_HDLC */
	{ "Wellfleet HDLC", "whdlc" },

	/* WTAP_ENCAP_SDLC */
	{ "SDLC", "sdlc" },

	/* WTAP_ENCAP_TZSP */
	{ "Tazmen sniffer protocol", "tzsp" },

	/* WTAP_ENCAP_ENC */
	{ "OpenBSD enc(4) encapsulating interface", "enc" },

	/* WTAP_ENCAP_PFLOG  */
	{ "OpenBSD PF Firewall logs", "pflog" },

	/* WTAP_ENCAP_CHDLC_WITH_PHDR */
	{ "Cisco HDLC with Directional Info", "chdlc-with-direction" },

	/* WTAP_ENCAP_BLUETOOTH_H4 */
	{ "Bluetooth H4", "bluetooth-h4" },

	/* WTAP_ENCAP_MTP2 */
	{ "SS7 MTP2", "mtp2" },

	/* WTAP_ENCAP_MTP3 */
	{ "SS7 MTP3", "mtp3" },

	/* WTAP_ENCAP_IRDA */
	{ "IrDA", "irda" },

	/* WTAP_ENCAP_USER0 */
	{ "USER 0", "user0" },

	/* WTAP_ENCAP_USER1 */
	{ "USER 1", "user1" },

	/* WTAP_ENCAP_USER2 */
	{ "USER 2", "user2" },

	/* WTAP_ENCAP_USER3 */
	{ "USER 3", "user3" },

	/* WTAP_ENCAP_USER4 */
	{ "USER 4", "user4" },

	/* WTAP_ENCAP_USER5 */
	{ "USER 5", "user5" },

	/* WTAP_ENCAP_USER6 */
	{ "USER 6", "user6" },

	/* WTAP_ENCAP_USER7 */
	{ "USER 7", "user7" },

	/* WTAP_ENCAP_USER8 */
	{ "USER 8", "user8" },

	/* WTAP_ENCAP_USER9 */
	{ "USER 9", "user9" },

	/* WTAP_ENCAP_USER10 */
	{ "USER 10", "user10" },

	/* WTAP_ENCAP_USER11 */
	{ "USER 11", "user11" },

	/* WTAP_ENCAP_USER12 */
	{ "USER 12", "user12" },

	/* WTAP_ENCAP_USER13 */
	{ "USER 13", "user13" },

	/* WTAP_ENCAP_USER14 */
	{ "USER 14", "user14" },

	/* WTAP_ENCAP_USER15 */
	{ "USER 15", "user15" },

	/* WTAP_ENCAP_SYMANTEC */
	{ "Symantec Enterprise Firewall", "symantec" },

	/* WTAP_ENCAP_APPLE_IP_OVER_IEEE1394 */
	{ "Apple IP-over-IEEE 1394", "ap1394" },

	/* WTAP_ENCAP_BACNET_MS_TP */
	{ "BACnet MS/TP", "bacnet-ms-tp" },

	/* WTAP_ENCAP_NETTL_RAW_ICMP */
	{ "Raw ICMP with nettl headers", "raw-icmp-nettl" },

	/* WTAP_ENCAP_NETTL_RAW_ICMPV6 */
	{ "Raw ICMPv6 with nettl headers", "raw-icmpv6-nettl" },

	/* WTAP_ENCAP_GPRS_LLC */
	{ "GPRS LLC", "gprs-llc" },

	/* WTAP_ENCAP_JUNIPER_ATM1 */
	{ "Juniper ATM1", "juniper-atm1" },

	/* WTAP_ENCAP_JUNIPER_ATM2 */
	{ "Juniper ATM2", "juniper-atm2" },

	/* WTAP_ENCAP_REDBACK */
	{ "Redback SmartEdge", "redback" },

	/* WTAP_ENCAP_NETTL_RAW_IP */
	{ "Raw IP with nettl headers", "rawip-nettl" },

	/* WTAP_ENCAP_NETTL_ETHERNET */
	{ "Ethernet with nettl headers", "ether-nettl" },

	/* WTAP_ENCAP_NETTL_TOKEN_RING */
	{ "Token Ring with nettl headers", "tr-nettl" },

	/* WTAP_ENCAP_NETTL_FDDI */
	{ "FDDI with nettl headers", "fddi-nettl" },

	/* WTAP_ENCAP_NETTL_UNKNOWN */
	{ "Unknown link-layer type with nettl headers", "unknown-nettl" },
};

/* Name that should be somewhat descriptive. */
const char
*wtap_encap_string(int encap)
{
	if (encap < 0 || encap >= WTAP_NUM_ENCAP_TYPES)
		return NULL;
	else
		return encap_table[encap].name;
}

/* Name to use in, say, a command-line flag specifying the type. */
const char
*wtap_encap_short_string(int encap)
{
	if (encap < 0 || encap >= WTAP_NUM_ENCAP_TYPES)
		return NULL;
	else
		return encap_table[encap].short_name;
}

/* Translate a short name to a capture file type. */
int
wtap_short_string_to_encap(const char *short_name)
{
	int encap;

	for (encap = 0; encap < WTAP_NUM_ENCAP_TYPES; encap++) {
		if (encap_table[encap].short_name != NULL &&
		    strcmp(short_name, encap_table[encap].short_name) == 0)
			return encap;
	}
	return -1;	/* no such encapsulation type */
}

static const char *wtap_errlist[] = {
	"The file isn't a plain file or pipe",
	"The file is being opened for random access but is a pipe",
	"The file isn't a capture file in a known format",
	"File contains record data we don't support",
	"That file format cannot be written to a pipe",
	NULL,
	"Files can't be saved in that format",
	"Files from that network type can't be saved in that format",
	"That file format doesn't support per-packet encapsulations",
	NULL,
	NULL,
	"Less data was read than was expected",
	"File contains a record that's not valid",
	"Less data was written than was requested",
	"Uncompression error: data oddly truncated",
	"Uncompression error: data would overflow buffer",
	"Uncompression error: bad LZ77 offset",
	"The standard input cannot be opened for random access",
};
#define	WTAP_ERRLIST_SIZE	(sizeof wtap_errlist / sizeof wtap_errlist[0])

const char
*wtap_strerror(int err)
{
	static char errbuf[128];
	unsigned int wtap_errlist_index;

	if (err < 0) {
#ifdef HAVE_LIBZ
		if (err >= WTAP_ERR_ZLIB_MIN && err <= WTAP_ERR_ZLIB_MAX) {
			/* Assume it's a zlib error. */
			sprintf(errbuf, "Uncompression error: %s",
			    zError(err - WTAP_ERR_ZLIB));
			return errbuf;
		}
#endif
		wtap_errlist_index = -1 - err;
		if (wtap_errlist_index >= WTAP_ERRLIST_SIZE) {
			sprintf(errbuf, "Error %d", err);
			return errbuf;
		}
		if (wtap_errlist[wtap_errlist_index] == NULL)
			return "Unknown reason";
		return wtap_errlist[wtap_errlist_index];
	} else
		return strerror(err);
}

/* Close only the sequential side, freeing up memory it uses.

   Note that we do *not* want to call the subtype's close function,
   as it would free any per-subtype data, and that data may be
   needed by the random-access side.

   Instead, if the subtype has a "sequential close" function, we call it,
   to free up stuff used only by the sequential side. */
void
wtap_sequential_close(wtap *wth)
{
	if (wth->subtype_sequential_close != NULL)
		(*wth->subtype_sequential_close)(wth);

	if (wth->fh != NULL) {
		file_close(wth->fh);
		wth->fh = NULL;
	}

	if (wth->frame_buffer) {
		buffer_free(wth->frame_buffer);
		g_free(wth->frame_buffer);
		wth->frame_buffer = NULL;
	}
}

void
wtap_close(wtap *wth)
{
	wtap_sequential_close(wth);

	if (wth->subtype_close != NULL)
		(*wth->subtype_close)(wth);

	if (wth->random_fh != NULL)
		file_close(wth->random_fh);

	g_free(wth);
}

gboolean
wtap_read(wtap *wth, int *err, gchar **err_info, long *data_offset)
{
	/*
	 * Set the packet encapsulation to the file's encapsulation
	 * value; if that's not WTAP_ENCAP_PER_PACKET, it's the
	 * right answer (and means that the read routine for this
	 * capture file type doesn't have to set it), and if it
	 * *is* WTAP_ENCAP_PER_PACKET, the caller needs to set it
	 * anyway.
	 */
	wth->phdr.pkt_encap = wth->file_encap;

	if (!wth->subtype_read(wth, err, err_info, data_offset))
		return FALSE;	/* failure */

	/*
	 * It makes no sense for the captured data length to be bigger
	 * than the actual data length.
	 */
	if (wth->phdr.caplen > wth->phdr.len)
		wth->phdr.caplen = wth->phdr.len;

	/*
	 * Make sure that it's not WTAP_ENCAP_PER_PACKET, as that
	 * probably means the file has that encapsulation type
	 * but the read routine didn't set this packet's
	 * encapsulation type.
	 */
	g_assert(wth->phdr.pkt_encap != WTAP_ENCAP_PER_PACKET);

	return TRUE;	/* success */
}

struct wtap_pkthdr*
wtap_phdr(wtap *wth)
{
	return &wth->phdr;
}

union wtap_pseudo_header*
wtap_pseudoheader(wtap *wth)
{
	return &wth->pseudo_header;
}

guint8*
wtap_buf_ptr(wtap *wth)
{
	return buffer_start_ptr(wth->frame_buffer);
}

gboolean
wtap_seek_read(wtap *wth, long seek_off,
	union wtap_pseudo_header *pseudo_header, guint8 *pd, int len,
	int *err, gchar **err_info)
{
	return wth->subtype_seek_read(wth, seek_off, pseudo_header, pd, len,
		err, err_info);
}
