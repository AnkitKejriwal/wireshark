/* packet-dcm.h
 *
 * $Id$
 * Routines for DICOM packet dissection
 * Copyright 2008, David Aggeler <david_aggeler@hispeed.ch>
 * This file is generated automatically from part 6 of the standard
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
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

#ifndef PACKET_DCM_H
#define PACKET_DCM_H

/* Used for DICOM Export Object feature */
typedef struct _dicom_eo_t {
	guint32  pkt_num;
	gchar   *hostname;
	gchar   *filename;
	gchar   *content_type;
	guint32  payload_len;
	const guint8 *payload_data;
} dicom_eo_t;

/* ---------------------------------------------------------------------
 * DICOM UID Definitions

 * Part 6 lists following different UID Types (2006-2008)

 * Application Context Name
 * Coding Scheme
 * DICOM UIDs as a Coding Scheme
 * LDAP OID
 * Meta SOP Class
 * SOP Class
 * Service Class
 * Transfer Syntax
 * Well-known Print Queue SOP Instance
 * Well-known Printer SOP Instance
 * Well-known SOP Instance
 * Well-known frame of reference
 */

struct dcm_uid {
    const char *value;
    const char *name;
    const char *type;
};

typedef struct dcm_uid dcm_uid_t;

#endif  /* PACKET_DCM_H */
