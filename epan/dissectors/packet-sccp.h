/* packet-sccp.h
 * Definitions for Signalling Connection Control Part (SCCP) dissection
 *
 * $Id$
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#ifndef __PACKET_SCCP_H 
#define __PACKET_SCCP_H

#define SCCP_MSG_TYPE_CR    0x01
#define SCCP_MSG_TYPE_CC    0x02
#define SCCP_MSG_TYPE_CREF  0x03
#define SCCP_MSG_TYPE_RLSD  0x04
#define SCCP_MSG_TYPE_RLC   0x05
#define SCCP_MSG_TYPE_DT1   0x06
#define SCCP_MSG_TYPE_DT2   0x07
#define SCCP_MSG_TYPE_AK    0x08
#define SCCP_MSG_TYPE_UDT   0x09
#define SCCP_MSG_TYPE_UDTS  0x0a
#define SCCP_MSG_TYPE_ED    0x0b
#define SCCP_MSG_TYPE_EA    0x0c
#define SCCP_MSG_TYPE_RSR   0x0d
#define SCCP_MSG_TYPE_RSC   0x0e
#define SCCP_MSG_TYPE_ERR   0x0f
#define SCCP_MSG_TYPE_IT    0x10
#define SCCP_MSG_TYPE_XUDT  0x11
#define SCCP_MSG_TYPE_XUDTS 0x12

/* The below 2 are ITU only */
#define SCCP_MSG_TYPE_LUDT  0x13
#define SCCP_MSG_TYPE_LUDTS 0x14

WS_VAR_IMPORT const value_string sccp_message_type_acro_values[];

typedef enum _sccp_payload_t {
    SCCP_PLOAD_NONE,
    SCCP_PLOAD_BSSAP,
    SCCP_PLOAD_RANAP,
    SCCP_PLOAD_NUM_PLOADS
} sccp_payload_t;

typedef struct _sccp_msg_info_t {
	guint framenum;
	guint offset;
	guint type;
	struct _sccp_assoc_info_t* assoc;
	struct _sccp_msg_info_t* next;
	
	gchar* label;
	gchar* comment;
} sccp_msg_info_t;

typedef struct _sccp_assoc_info_t {
    guint32 id;
    guint32 calling_dpc;
    guint32 called_dpc;
    guint8 calling_ssn;
    guint8 called_ssn;
    gboolean has_fw_key;
    gboolean has_bw_key;
    sccp_msg_info_t* msgs;
    sccp_msg_info_t* curr_msg;
	
    sccp_payload_t proto;
    gchar* calling_party;
    gchar* called_party;
    gchar* extra_info;

} sccp_assoc_info_t;

#endif
