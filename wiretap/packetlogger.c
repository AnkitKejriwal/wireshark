/* packetlogger.c
 * Routines for opening Apple's (Bluetooth) PacketLogger file format captures
 * Copyright 2008-2009, Stephen Fisher <stephentfisher@yahoo.com>
 *
 * $Id$
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * Based on commview.c, Linux's BlueZ-Gnome Analyzer program and hexdumps of
 * the output files from Apple's PacketLogger tool.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "wtap.h"
#include "wtap-int.h"
#include "buffer.h"
#include "file_wrappers.h"
#include "packetlogger.h"

typedef struct packetlogger_header {
	guint32 len;
	guint64 ts;
	guint8 type;
} packetlogger_header_t;

#define PACKETLOGGER_HEADER_SIZE 13

static gboolean packetlogger_read(wtap *wth, int *err, gchar **err_info _U_,
				  gint64 *data_offset);
static gboolean packetlogger_seek_read(wtap *wth, gint64 seek_off,
				       union wtap_pseudo_header *pseudo_header,
				       guchar *pd, int length, int *err,
				       gchar **err_info _U_);
static gboolean packetlogger_read_header(packetlogger_header_t *pl_hdr,
					 FILE_T fh, int *err);


int packetlogger_open(wtap *wth, int *err, gchar **err_info _U_)
{
	packetlogger_header_t pl_hdr;

	if(!packetlogger_read_header(&pl_hdr, wth->fh, err))
		return -1;

	/* Verify this file belongs to us */
	if(!((pl_hdr.len & 0xFFFF0000) == 0 && (pl_hdr.type < 0x04 ||
						pl_hdr.type == 0xFE ||
						pl_hdr.type == 0xFF)))
		return 0;

	/* No file header. Reset the fh to 0 so we can read the first packet */
	if (file_seek(wth->fh, 0, SEEK_SET, err) == -1)
		return -1;

	/* Set up the pointers to the handlers for this file type */
	wth->subtype_read = packetlogger_read;
	wth->subtype_seek_read = packetlogger_seek_read;

	wth->data_offset = 0;
	wth->file_type = WTAP_FILE_PACKETLOGGER;
	wth->file_encap = WTAP_ENCAP_BLUETOOTH_HCI;
	wth->tsprecision = WTAP_FILE_TSPREC_USEC;

	return 1; /* Our kind of file */
}

static gboolean
packetlogger_read(wtap *wth, int *err, gchar **err_info _U_, gint64 *data_offset)
{
	packetlogger_header_t pl_hdr;
	guint bytes_read;

	*data_offset = wth->data_offset;

	if(!packetlogger_read_header(&pl_hdr, wth->fh, err))
		return FALSE;

	buffer_assure_space(wth->frame_buffer, pl_hdr.len - 9);
	bytes_read = file_read(buffer_start_ptr(wth->frame_buffer), 1,
			       pl_hdr.len - 9,
			       wth->fh);
	if(bytes_read != pl_hdr.len - 9) {
		*err = file_error(wth->fh);
		if(*err == 0)
			*err = WTAP_ERR_SHORT_READ;

		return FALSE;
	}

	wth->data_offset += (pl_hdr.len + 4);

	wth->phdr.len = pl_hdr.len - 9;
	wth->phdr.caplen = pl_hdr.len - 9;

	wth->phdr.ts.secs = (time_t) (pl_hdr.ts >> 32);
	wth->phdr.ts.nsecs = (int)((pl_hdr.ts & 0xFFFFFFFF) * 1000);

	switch(pl_hdr.type) {

	case 0 :
		wth->pseudo_header.bthci.channel = BTHCI_CHANNEL_COMMAND;
		break;
	case 1 :
		wth->pseudo_header.bthci.channel = BTHCI_CHANNEL_EVENT;
		break;

	default :
		wth->pseudo_header.bthci.channel = pl_hdr.type;
		break;
	}

	wth->pseudo_header.bthci.sent = P2P_DIR_UNKNOWN;

	return TRUE;
}

static gboolean
packetlogger_seek_read(wtap *wth, gint64 seek_off, union wtap_pseudo_header
		       *pseudo_header, guchar *pd, int length, int *err,
		       gchar **err_info _U_)
{
	packetlogger_header_t pl_hdr;
	guint bytes_read;

	if(file_seek(wth->random_fh, seek_off, SEEK_SET, err) == -1)
		return FALSE;

	if(!packetlogger_read_header(&pl_hdr, wth->random_fh, err)) {
		if(*err == 0)
			*err = WTAP_ERR_SHORT_READ;

		return FALSE;
	}

	if(length != (int)pl_hdr.len - 9) {
		*err = WTAP_ERR_BAD_RECORD;
		*err_info = g_strdup_printf("packetlogger: record length %u doesn't match requested length %d", pl_hdr.len, length);
		return FALSE;
	}

	bytes_read = file_read(pd, 1, pl_hdr.len - 9, wth->random_fh);
	if(bytes_read != (pl_hdr.len - 9)) {
		*err = file_error(wth->random_fh);
		if(*err == 0)
			*err = WTAP_ERR_SHORT_READ;

		return FALSE;
	}

	switch(pl_hdr.type) {

	case 0 :
		pseudo_header->bthci.channel = BTHCI_CHANNEL_COMMAND;
		break;
	case 1 :
		pseudo_header->bthci.channel = BTHCI_CHANNEL_EVENT;
		break;

	default :
		pseudo_header->bthci.channel = pl_hdr.type;
		break;
	}

	pseudo_header->bthci.sent = P2P_DIR_UNKNOWN;

	return TRUE;
}

static gboolean
packetlogger_read_header(packetlogger_header_t *pl_hdr, FILE_T fh, int *err)
{
	guint bytes_read = 0;

	bytes_read += file_read(&pl_hdr->len, 4, 1, fh);
	bytes_read += file_read(&pl_hdr->ts, 8, 1, fh);
	bytes_read += file_read(&pl_hdr->type, 1, 1, fh);

	/* Convert multi-byte values from big endian to host endian */
	pl_hdr->len = GUINT32_FROM_BE(pl_hdr->len);
	pl_hdr->ts = GUINT64_FROM_BE(pl_hdr->ts);

	if(bytes_read < PACKETLOGGER_HEADER_SIZE) {
		*err = file_error(fh);
		if(*err == 0 && bytes_read > 0)
			*err = WTAP_ERR_SHORT_READ;

		return FALSE;
	}

	return TRUE;
}
