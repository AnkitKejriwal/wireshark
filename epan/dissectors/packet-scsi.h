/* packet-scsi.h
 * Author: Dinesh G Dutt (ddutt@cisco.com)
 *
 * $Id$
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@ethereal.com>
 * Copyright 2002 Gerald Combs
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

#ifndef __PACKET_SCSI_H_
#define __PACKET_SCSI_H_

extern const value_string scsi_status_val[];

/*
 * SCSI Device Types.
 *
 * These can be supplied to the dissection routines if the caller happens
 * to know the device type (e.g., NDMP assumes that a "jukebox" is a
 * media changer, SCSI_DEV_SMC, and a "tape" is a sequential access device,
 * SCSI_DEV_SSC).
 *
 * If the caller doesn't know the device type, it supplies SCSI_DEV_UNKNOWN.
 */
#define SCSI_DEV_UNKNOWN   -1
#define SCSI_DEV_SBC       0x0
#define SCSI_DEV_SSC       0x1
#define SCSI_DEV_PRNT      0x2
#define SCSI_DEV_PROC      0x3
#define SCSI_DEV_WORM      0x4
#define SCSI_DEV_CDROM     0x5
#define SCSI_DEV_SCAN      0x6
#define SCSI_DEV_OPTMEM    0x7
#define SCSI_DEV_SMC       0x8
#define SCSI_DEV_COMM      0x9
#define SCSI_DEV_RAID      0xC
#define SCSI_DEV_SES       0xD
#define SCSI_DEV_RBC       0xE
#define SCSI_DEV_OCRW      0xF
#define SCSI_DEV_OSD       0x11
#define SCSI_DEV_ADC       0x12
#define SCSI_DEV_NOLUN     0x1F

#define SCSI_DEV_BITS      0x1F /* the lower 5 bits indicate device type */
#define SCSI_MS_PCODE_BITS 0x3F /* Page code bits in Mode Sense */

/* Function Decls; functions invoked by SAM-2 transport protocols such as
 * FCP/iSCSI
 */
void dissect_scsi_cdb (tvbuff_t *, packet_info *, proto_tree *,
                       gint, guint16);
void dissect_scsi_rsp (tvbuff_t *, packet_info *, proto_tree *, guint16, guint8);
void dissect_scsi_payload (tvbuff_t *, packet_info *, proto_tree *,
                           gboolean, guint16);
void dissect_scsi_snsinfo (tvbuff_t *, packet_info *, proto_tree *, guint, guint, guint16);

/*
 * Private data to be supplied to those functions via "pinfo->private_data";
 * the structure contains a 32-bit conversation ID and a 32-bit task
 * ID, where the former identifies a conversation between initiator and
 * target and the latter identifies a SCSI task within that conversation.
 */
typedef struct {
	guint32	conv_id;
	guint32 task_id;
} scsi_task_id_t;

#endif
