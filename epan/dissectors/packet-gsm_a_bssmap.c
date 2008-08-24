/* packet-gsm_a_bssmap.c
 * Routines for GSM A Interface BSSMAP dissection
 *
 * Copyright 2003, Michael Lum <mlum [AT] telostech.com>
 * In association with Telos Technology Inc.
 *
 * Title		3GPP			Other
 *
 *   Reference [2]
 *   Mobile-services Switching Centre - Base Station System
 *   (MSC - BSS) interface;
 *   Layer 3 specification
 *   (GSM 08.08 version 7.7.0 Release 1998)	TS 100 590 v7.7.0
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#include <epan/packet.h>
#include <epan/prefs.h>
#include <epan/tap.h>
#include <epan/emem.h>

#include "packet-bssap.h"
#include "packet-sccp.h"
#include "packet-gsm_a_common.h"
#include "packet-e212.h"

/* PROTOTYPES/FORWARDS */

const value_string gsm_a_bssmap_msg_strings[] = {
	{ 0x01,	"Assignment Request" },
	{ 0x02,	"Assignment Complete" },
	{ 0x03,	"Assignment Failure" },
	{ 0x10,	"Handover Request" },
	{ 0x11,	"Handover Required" },
	{ 0x12,	"Handover Request Acknowledge" },
	{ 0x13,	"Handover Command" },
	{ 0x14,	"Handover Complete" },
	{ 0x15,	"Handover Succeeded" },
	{ 0x16,	"Handover Failure" },
	{ 0x17,	"Handover Performed" },
	{ 0x18,	"Handover Candidate Enquire" },
	{ 0x19,	"Handover Candidate Response" },
	{ 0x1a,	"Handover Required Reject" },
	{ 0x1b,	"Handover Detect" },
	{ 0x20,	"Clear Command" },
	{ 0x21,	"Clear Complete" },
	{ 0x22,	"Clear Request" },
	{ 0x23,	"Reserved" },
	{ 0x24,	"Reserved" },
	{ 0x25,	"SAPI 'n' Reject" },
	{ 0x26,	"Confusion" },
	{ 0x28,	"Suspend" },
	{ 0x29,	"Resume" },
	{ 0x2a,	"Connection Oriented Information" },
	{ 0x2b,	"Perform Location Request" },
	{ 0x2c,	"LSA Information" },
	{ 0x2d,	"Perform Location Response" },
	{ 0x2e,	"Perform Location Abort" },
	{ 0x2f,	"Common Id" },
	{ 0x30,	"Reset" },
	{ 0x31,	"Reset Acknowledge" },
	{ 0x32,	"Overload" },
	{ 0x33,	"Reserved" },
	{ 0x34,	"Reset Circuit" },
	{ 0x35,	"Reset Circuit Acknowledge" },
	{ 0x36,	"MSC Invoke Trace" },
	{ 0x37,	"BSS Invoke Trace" },
	{ 0x3a,	"Connectionless Information" },
	{ 0x40,	"Block" },
	{ 0x41,	"Blocking Acknowledge" },
	{ 0x42,	"Unblock" },
	{ 0x43,	"Unblocking Acknowledge" },
	{ 0x44,	"Circuit Group Block" },
	{ 0x45,	"Circuit Group Blocking Acknowledge" },
	{ 0x46,	"Circuit Group Unblock" },
	{ 0x47,	"Circuit Group Unblocking Acknowledge" },
	{ 0x48,	"Unequipped Circuit" },
	{ 0x4e,	"Change Circuit" },
	{ 0x4f,	"Change Circuit Acknowledge" },
	{ 0x50,	"Resource Request" },
	{ 0x51,	"Resource Indication" },
	{ 0x52,	"Paging" },
	{ 0x53,	"Cipher Mode Command" },
	{ 0x54,	"Classmark Update" },
	{ 0x55,	"Cipher Mode Complete" },
	{ 0x56,	"Queuing Indication" },
	{ 0x57,	"Complete Layer 3 Information" },
	{ 0x58,	"Classmark Request" },
	{ 0x59,	"Cipher Mode Reject" },
	{ 0x5a,	"Load Indication" },
	{ 0x04,	"VGCS/VBS Setup" },
	{ 0x05,	"VGCS/VBS Setup Ack" },
	{ 0x06,	"VGCS/VBS Setup Refuse" },
	{ 0x07,	"VGCS/VBS Assignment Request" },
	{ 0x1c,	"VGCS/VBS Assignment Result" },
	{ 0x1d,	"VGCS/VBS Assignment Failure" },
	{ 0x1e,	"VGCS/VBS Queuing Indication" },
	{ 0x1f,	"Uplink Request" },
	{ 0x27,	"Uplink Request Acknowledge" },
	{ 0x49,	"Uplink Request Confirmation" },
	{ 0x4a,	"Uplink Release Indication" },
	{ 0x4b,	"Uplink Reject Command" },
	{ 0x4c,	"Uplink Release Command" },
	{ 0x4d,	"Uplink Seized Command" },
	{ 0, NULL }
};

const value_string gsm_bssmap_elem_strings[] = {
	{ 0x01,	"Circuit Identity Code" },
	{ 0x02,	"Reserved" },
	{ 0x03,	"Resource Available" },
	{ 0x04,	"Cause" },
	{ 0x05,	"Cell Identifier" },
	{ 0x06,	"Priority" },
	{ 0x07,	"Layer 3 Header Information" },
	{ 0x08,	"IMSI" },
	{ 0x09,	"TMSI" },
	{ 0x0a,	"Encryption Information" },
	{ 0x0b,	"Channel Type" },
	{ 0x0c,	"Periodicity" },
	{ 0x0d,	"Extended Resource Indicator" },
	{ 0x0e,	"Number Of MSs" },
	{ 0x0f,	"Reserved" },
	{ 0x10,	"Reserved" },
	{ 0x11,	"Reserved" },
	{ 0x12,	"Classmark Information Type 2" },
	{ 0x13,	"Classmark Information Type 3" },
	{ 0x14,	"Interference Band To Be Used" },
	{ 0x15,	"RR Cause" },
	{ 0x16,	"Reserved" },
	{ 0x17,	"Layer 3 Information" },
	{ 0x18,	"DLCI" },
	{ 0x19,	"Downlink DTX Flag" },
	{ 0x1a,	"Cell Identifier List" },
	{ 0x1b,	"Response Request" },
	{ 0x1c,	"Resource Indication Method" },
	{ 0x1d,	"Classmark Information Type 1" },
	{ 0x1e,	"Circuit Identity Code List" },
	{ 0x1f,	"Diagnostic" },
	{ 0x20,	"Layer 3 Message Contents" },
	{ 0x21,	"Chosen Channel" },
	{ 0x22,	"Total Resource Accessible" },
	{ 0x23,	"Cipher Response Mode" },
	{ 0x24,	"Channel Needed" },
	{ 0x25,	"Trace Type" },
	{ 0x26,	"TriggerID" },
	{ 0x27,	"Trace Reference" },
	{ 0x28,	"TransactionID" },
	{ 0x29,	"Mobile Identity" },
	{ 0x2a,	"OMCID" },
	{ 0x2b,	"Forward Indicator" },
	{ 0x2c,	"Chosen Encryption Algorithm" },
	{ 0x2d,	"Circuit Pool" },
	{ 0x2e,	"Circuit Pool List" },
	{ 0x2f,	"Time Indication" },
	{ 0x30,	"Resource Situation" },
	{ 0x31,	"Current Channel Type 1" },
	{ 0x32,	"Queuing Indicator" },
	{ 0x40,	"Speech Version" },
	{ 0x33,	"Assignment Requirement" },
	{ 0x35,	"Talker Flag" },
	{ 0x36,	"Connection Release Requested" },
	{ 0x37,	"Group Call Reference" },
	{ 0x38,	"eMLPP Priority" },
	{ 0x39,	"Configuration Evolution Indication" },
	{ 0x3a,	"Old BSS to New BSS Information" },
	{ 0x3b,	"LSA Identifier" },
	{ 0x3c,	"LSA Identifier List" },
	{ 0x3d,	"LSA Information" },
	{ 0x3e,	"LCS QoS" },
	{ 0x3f,	"LSA access control suppression" },
	{ 0x43,	"LCS Priority" },
	{ 0x44,	"Location Type" },
	{ 0x45,	"Location Estimate" },
	{ 0x46,	"Positioning Data" },
	{ 0x47,	"LCS Cause" },
	{ 0x48,	"LCS Client Type" },
	{ GSM_BSSMAP_APDU_IE,	"APDU" },
	{ 0x4a,	"Network Element Identity" },
	{ 0x4b,	"GPS Assistance Data" },
	{ 0x4c,	"Deciphering Keys" },
	{ 0x4d,	"Return Error Request" },
	{ 0x4e,	"Return Error Cause" },
	{ 0x4f,	"Segmentation" },
	{ 0, NULL }
};

static const value_string bssap_cc_values[] = {
	{ 0x00,		"not further specified" },
	{ 0x80,		"FACCH or SDCCH" },
	{ 0xc0,		"SACCH" },
	{ 0,		NULL } };

static const value_string bssap_sapi_values[] = {
	{ 0x00,		"RR/MM/CC" },
	{ 0x03,		"SMS" },
	{ 0,		NULL } };

static const value_string gsm_a_be_cell_id_disc_vals[] = {
	{ 0,		"The whole Cell Global Identification, CGI, is used to identify the cells."},
	{ 1,		"Location Area Code, LAC, and Cell Identify, CI, is used to identify the cells."},
	{ 2,		"Cell Identity, CI, is used to identify the cells."},
	{ 3,		"No cell is associated with the transaction."},
	{ 4,		"Location Area Identification, LAI, is used to identify all cells within a Location Area."},
	{ 5,		"Location Area Code, LAC, is used to identify all cells within a location area."},
	{ 6,		"All cells on the BSS are identified."},
	{ 8,		"Intersystem Handover to UTRAN or cdma2000. PLMN-ID, LAC, and RNC-ID, are encoded to identify the target RNC."},
	{ 9,		"Intersystem Handover to UTRAN or cdma2000. The RNC-ID is coded to identify the target RNC."},
	{ 10,		"Intersystem Handover to UTRAN or cdma2000. LAC and RNC-ID are encoded to identify the target RNC."},
	{ 0,	NULL }
};

static const value_string gsm_a_rr_channel_needed_vals[] = {
{ 0x00,		"Any channel"},
{ 0x01,		"SDCCH"},
{ 0x02,		"TCH/F (Full rate)"},
{ 0x03,		"TCH/H or TCH/F (Dual rate)"},
	{ 0,	NULL }
};

/* Initialize the protocol and registered fields */
static int proto_a_bssmap = -1;

static int hf_gsm_a_bssmap_msg_type = -1;
int hf_gsm_a_length = -1;
int hf_gsm_a_bssmap_elem_id = -1;
static int hf_gsm_a_cell_ci = -1;
static int hf_gsm_a_cell_lac = -1;
static int hf_gsm_a_dlci_cc = -1;
static int hf_gsm_a_dlci_spare = -1;
static int hf_gsm_a_dlci_sapi = -1;
static int hf_gsm_a_bssmap_cause = -1;
static int hf_gsm_a_be_cell_id_disc = -1;
static int hf_gsm_a_be_rnc_id = -1;
static int hf_gsm_a_apdu_protocol_id = -1;

/* Initialize the subtree pointers */
static gint ett_bssmap_msg = -1;
static gint ett_cell_list = -1;
static gint ett_dlci = -1;

static char a_bigbuf[1024];

static dissector_handle_t data_handle;
static dissector_handle_t gsm_bsslap_handle = NULL;
static dissector_handle_t bssmap_handle;
static dissector_handle_t dtap_handle;

static packet_info *g_pinfo;
static proto_tree *g_tree;

typedef enum
{
	BE_CIC,	 /* Circuit Identity Code */
	BE_RSVD_1,	 /* Reserved */
	BE_RES_AVAIL,	 /* Resource Available */
	BE_CAUSE,	 /* Cause */
	BE_CELL_ID,	 /* Cell Identifier */
	BE_PRIO,	 /* Priority */
	BE_L3_HEADER_INFO,	 /* Layer 3 Header Information */
	BE_IMSI,	 /* IMSI */
	BE_TMSI,	 /* TMSI */
	BE_ENC_INFO,	 /* Encryption Information */
	BE_CHAN_TYPE,	 /* Channel Type */
	BE_PERIODICITY,	 /* Periodicity */
	BE_EXT_RES_IND,	 /* Extended Resource Indicator */
	BE_NUM_MS,	 /* Number Of MSs */
	BE_RSVD_2,	 /* Reserved */
	BE_RSVD_3,	 /* Reserved */
	BE_RSVD_4,	 /* Reserved */
	BE_CM_INFO_2,	 /* Classmark Information Type 2 */
	BE_CM_INFO_3,	 /* Classmark Information Type 3 */
	BE_INT_BAND,	 /* Interference Band To Be Used */
	BE_RR_CAUSE,	 /* RR Cause */
	BE_RSVD_5,	 /* Reserved */
	BE_L3_INFO,	 /* Layer 3 Information */
	BE_DLCI,	 /* DLCI */
	BE_DOWN_DTX_FLAG,	 /* Downlink DTX Flag */
	BE_CELL_ID_LIST,	 /* Cell Identifier List */
	BE_RESP_REQ,	 /* Response Request */
	BE_RES_IND_METHOD,	 /* Resource Indication Method */
	BE_CM_INFO_1,	 /* Classmark Information Type 1 */
	BE_CIC_LIST,	 /* Circuit Identity Code List */
	BE_DIAG,	 /* Diagnostic */
	BE_L3_MSG,	 /* Layer 3 Message Contents */
	BE_CHOSEN_CHAN,	 /* Chosen Channel */
	BE_TOT_RES_ACC,	 /* Total Resource Accessible */
	BE_CIPH_RESP_MODE,	 /* Cipher Response Mode */
	BE_CHAN_NEEDED,	 /* Channel Needed */
	BE_TRACE_TYPE,	 /* Trace Type */
	BE_TRIGGERID,	 /* TriggerID */
	BE_TRACE_REF,	 /* Trace Reference */
	BE_TRANSID,	 /* TransactionID */
	BE_MID,	 /* Mobile Identity */
	BE_OMCID,	 /* OMCID */
	BE_FOR_IND,	 /* Forward Indicator */
	BE_CHOSEN_ENC_ALG,	 /* Chosen Encryption Algorithm */
	BE_CCT_POOL,	 /* Circuit Pool */
	BE_CCT_POOL_LIST,	 /* Circuit Pool List */
	BE_TIME_IND,	 /* Time Indication */
	BE_RES_SIT,	 /* Resource Situation */
	BE_CURR_CHAN_1,	 /* Current Channel Type 1 */
	BE_QUE_IND,	 /* Queueing Indicator */
	BE_SPEECH_VER,	 /* Speech Version */
	BE_ASS_REQ,	 /* Assignment Requirement */
	BE_TALKER_FLAG,	 /* Talker Flag */
	BE_CONN_REL_REQ,	 /* Connection Release Requested */
	BE_GROUP_CALL_REF,	 /* Group Call Reference */
	BE_EMLPP_PRIO,	 /* eMLPP Priority */
	BE_CONF_EVO_IND,	 /* Configuration Evolution Indication */
	BE_OLD2NEW_INFO,	 /* Old BSS to New BSS Information */
	BE_LSA_ID,	 /* LSA Identifier */
	BE_LSA_ID_LIST,	 /* LSA Identifier List */
	BE_LSA_INFO,	 /* LSA Information */
	BE_LCS_QOS,	 /* LCS QoS */
	BE_LSA_ACC_CTRL,	 /* LSA access control suppression */
	BE_LCS_PRIO,	 /* LCS Priority */
	BE_LOC_TYPE,	 /* Location Type */
	BE_LOC_EST,	 /* Location Estimate */
	BE_POS_DATA,	 /* Positioning Data */
	BE_LCS_CAUSE,	 /* LCS Cause */
	BE_LCS_CLIENT,	 /* LCS Client Type */
	BE_APDU,	 /* APDU */
	BE_NE_ID,	 /* Network Element Identity */
	BE_GSP_ASSIST_DATA,	 /* GPS Assistance Data */
	BE_DECIPH_KEYS,	 /* Deciphering Keys */
	BE_RET_ERR_REQ,	 /* Return Error Request */
	BE_RET_ERR_CAUSE,	 /* Return Error Cause */
	BE_SEG,	 /* Segmentation */
	BE_NONE	/* NONE */
}
bssmap_elem_idx_t;

#define	NUM_GSM_BSSMAP_ELEM (sizeof(gsm_bssmap_elem_strings)/sizeof(value_string))
gint ett_gsm_bssmap_elem[NUM_GSM_BSSMAP_ELEM];

/*
 * [2] 3.2.2.2
 */
static guint8
be_cic(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len)
{
	guint32	curr_offset;
	guint32	value;

	len = len;
	curr_offset = offset;

	value = tvb_get_ntohs(tvb, curr_offset);

	other_decode_bitfield_value(a_bigbuf, value, 0xffe0, 16);
	proto_tree_add_text(tree,
	tvb, curr_offset, 2,
	"%s :  PCM Multiplexer: %u",
	a_bigbuf,
	(value & 0xffe0) >> 5);

	other_decode_bitfield_value(a_bigbuf, value, 0x001f, 16);
	proto_tree_add_text(tree,
	tvb, curr_offset, 2,
	"%s :  Timeslot: %u",
	a_bigbuf,
	value & 0x001f);

	curr_offset += 2;

	if (add_string)
	g_snprintf(add_string, string_len, " - (%u) (0x%04x)", value, value);

	/* no length check possible */

	return(curr_offset - offset);
}

/*
 * [2] 3.2.2.5
 */
static guint8
be_cause(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len)
{
	guint8	oct;
	guint32	value;
	guint32	curr_offset;
	const gchar	*str = NULL;

	curr_offset = offset;

	oct = tvb_get_guint8(tvb, curr_offset);

	other_decode_bitfield_value(a_bigbuf, oct, 0x80, 8);
	proto_tree_add_text(tree,
	tvb, curr_offset, 1,
	"%s :  Extension: %s",
	a_bigbuf,
	(oct & 0x80) ? "extended" : "not extended");

	if (oct & 0x80)
	{
	/* 2 octet cause */

	if ((oct & 0x0f) == 0x00)
	{
	    /* national cause */
	    switch ((oct & 0x70) >> 4)
	    {
	    case 0: str = "Normal Event"; break;
	    case 1: str = "Normal Event"; break;
	    case 2: str = "Resource Unavailable"; break;
	    case 3: str = "Service or option not available"; break;
	    case 4: str = "Service or option not implemented"; break;
	    case 5: str = "Invalid message (e.g., parameter out of range)"; break;
	    case 6: str = "Protocol error"; break;
	    default:
		str = "Interworking";
		break;
	    }

	    other_decode_bitfield_value(a_bigbuf, oct, 0x70, 8);
	    proto_tree_add_text(tree,
		tvb, curr_offset, 1,
		"%s :  Cause Class: %s",
		a_bigbuf,
		str);

	    other_decode_bitfield_value(a_bigbuf, oct, 0x0f, 8);
	    proto_tree_add_text(tree,
		tvb, curr_offset, 1,
		"%s :  National Cause",
		a_bigbuf);

	    curr_offset++;

	    proto_tree_add_text(tree, tvb, curr_offset, 1,
		"Cause Value");

	    curr_offset++;

	    if (add_string)
		g_snprintf(add_string, string_len, " - (National Cause)");
	}
	else
	{
	    value = tvb_get_guint8(tvb, curr_offset + 1);

	    other_decode_bitfield_value(a_bigbuf, oct, 0x7f, 8);
	    proto_tree_add_text(tree,
		tvb, curr_offset, 1,
		"%s :  Cause (MSB): %u",
		a_bigbuf,
		((oct & 0x7f) << 8) | value);

	    curr_offset++;

	    other_decode_bitfield_value(a_bigbuf, value, 0xff, 8);
	    proto_tree_add_text(tree, tvb, curr_offset, 1,
		"%s :  Cause (LSB)",
		a_bigbuf);

	    curr_offset++;
	}
	}
	else
	{
	switch (oct)
	{
	case 0x00: str = "Radio interface message failure"; break;
	case 0x01: str = "Radio interface failure"; break;
	case 0x02: str = "Uplink quality"; break;
	case 0x03: str = "Uplink strength"; break;
	case 0x04: str = "Downlink quality"; break;
	case 0x05: str = "Downlink strength"; break;
	case 0x06: str = "Distance"; break;
	case 0x07: str = "O and M intervention"; break;
	case 0x08: str = "Response to MSC invocation"; break;
	case 0x09: str = "Call control"; break;
	case 0x0a: str = "Radio interface failure, reversion to old channel"; break;
	case 0x0b: str = "Handover successful"; break;
	case 0x0c: str = "Better Cell"; break;
	case 0x0d: str = "Directed Retry"; break;
	case 0x0e: str = "Joined group call channel"; break;
	case 0x0f: str = "Traffic"; break;

	case 0x20: str = "Equipment failure"; break;
	case 0x21: str = "No radio resource available"; break;
	case 0x22: str = "Requested terrestrial resource unavailable"; break;
	case 0x23: str = "CCCH overload"; break;
	case 0x24: str = "Processor overload"; break;
	case 0x25: str = "BSS not equipped"; break;
	case 0x26: str = "MS not equipped"; break;
	case 0x27: str = "Invalid cell"; break;
	case 0x28: str = "Traffic Load"; break;
	case 0x29: str = "Preemption"; break;

	case 0x30: str = "Requested transcoding/rate adaption unavailable"; break;
	case 0x31: str = "Circuit pool mismatch"; break;
	case 0x32: str = "Switch circuit pool"; break;
	case 0x33: str = "Requested speech version unavailable"; break;
	case 0x34: str = "LSA not allowed"; break;

	case 0x40: str = "Ciphering algorithm not supported"; break;

	case 0x50: str = "Terrestrial circuit already allocated"; break;
	case 0x51: str = "Invalid message contents"; break;
	case 0x52: str = "Information element or field missing"; break;
	case 0x53: str = "Incorrect value"; break;
	case 0x54: str = "Unknown Message type"; break;
	case 0x55: str = "Unknown Information Element"; break;

	case 0x60: str = "Protocol Error between BSS and MSC"; break;
	case 0x61: str = "VGCS/VBS call non existent"; break;

	default:
	    if ((oct >= 0x10) && (oct <= 0x17)) { str = "Reserved for international use"; }
	    else if ((oct >= 0x18) && (oct <= 0x1f)) { str = "Reserved for national use"; }
	    else if ((oct >= 0x2a) && (oct <= 0x2f)) { str = "Reserved for national use"; }
	    else if ((oct >= 0x35) && (oct <= 0x3f)) { str = "Reserved for international use"; }
	    else if ((oct >= 0x41) && (oct <= 0x47)) { str = "Reserved for international use"; }
	    else if ((oct >= 0x48) && (oct <= 0x4f)) { str = "Reserved for national use"; }
	    else if ((oct >= 0x56) && (oct <= 0x57)) { str = "Reserved for international use"; }
	    else if ((oct >= 0x58) && (oct <= 0x5f)) { str = "Reserved for national use"; }
	    else if ((oct >= 0x62) && (oct <= 0x67)) { str = "Reserved for international use"; }
	    else if ((oct >= 0x68) && (oct <= 0x6f)) { str = "Reserved for national use"; }
	    else if ((oct >= 0x70) && (oct <= 0x77)) { str = "Reserved for international use"; }
	    else if ((oct >= 0x78) && (oct <= 0x7f)) { str = "Reserved for national use"; }
	    break;
	}

	other_decode_bitfield_value(a_bigbuf, oct, 0x7f, 8);
	proto_tree_add_uint_format(tree, hf_gsm_a_bssmap_cause,
	    tvb, curr_offset, 1, oct & 0x7f,
	    "%s :  Cause: (%u) %s",
	    a_bigbuf,
	    oct & 0x7f,
	    str);

	curr_offset++;

	if (add_string)
	    g_snprintf(add_string, string_len, " - (%u) %s", oct & 0x7f, str);
	}

	EXTRANEOUS_DATA_CHECK(len, curr_offset - offset);

	return(curr_offset - offset);
}

/*
 * [2] 3.2.2.7
 */
static guint8
be_tmsi(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len)
{
	guint32	curr_offset;
	guint32	value;

	curr_offset = offset;

	value = tvb_get_ntohl(tvb, curr_offset);

	proto_tree_add_uint(tree, hf_gsm_a_tmsi,
	tvb, curr_offset, 4,
	value);

	if (add_string)
	g_snprintf(add_string, string_len, " - (0x%04x)", value);

	curr_offset += 4;

	EXTRANEOUS_DATA_CHECK(len, curr_offset - offset);

	return(curr_offset - offset);
}

/*
 * [2] 3.2.2.9
 */
static guint8
be_l3_header_info(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string _U_, int string_len _U_)
{
	guint8	oct;
	guint32	curr_offset;

	curr_offset = offset;

	oct = tvb_get_guint8(tvb, curr_offset);

	other_decode_bitfield_value(a_bigbuf, oct, 0xf0, 8);
	proto_tree_add_text(tree,
	tvb, curr_offset, 1,
	"%s :  Spare",
	a_bigbuf);

	proto_tree_add_item(tree, hf_gsm_a_L3_protocol_discriminator, tvb, curr_offset, 1, FALSE);


	curr_offset++;

	NO_MORE_DATA_CHECK(len);

	oct = tvb_get_guint8(tvb, curr_offset);

	other_decode_bitfield_value(a_bigbuf, oct, 0xf0, 8);
	proto_tree_add_text(tree,
	tvb, curr_offset, 1,
	"%s :  Spare",
	a_bigbuf);

	other_decode_bitfield_value(a_bigbuf, oct, 0x08, 8);
	proto_tree_add_text(tree,
	tvb, curr_offset, 1,
	"%s :  TI flag: %s",
	a_bigbuf,
	((oct & 0x08) ?  "allocated by receiver" : "allocated by sender"));

	other_decode_bitfield_value(a_bigbuf, oct, 0x07, 8);
	proto_tree_add_text(tree,
	tvb, curr_offset, 1,
	"%s :  TIO: %u",
	a_bigbuf,
	oct & 0x07);

	curr_offset++;

	EXTRANEOUS_DATA_CHECK(len, curr_offset - offset);

	return(curr_offset - offset);
}

/*
 * [2] 3.2.2.10
 */
static guint8
be_enc_info(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string _U_, int string_len _U_)
{
	guint8	oct;
	guint8	mask;
	guint8	alg_id;
	guint32	curr_offset;

	curr_offset = offset;

	oct = tvb_get_guint8(tvb, curr_offset);

	mask = 0x80;
	alg_id = 7;

	do
	{
	other_decode_bitfield_value(a_bigbuf, oct, mask, 8);
	proto_tree_add_text(tree,
	    tvb, curr_offset, 1,
	    "%s :  GSM A5/%u: %spermitted",
	    a_bigbuf,
	    alg_id,
	    (mask & oct) ? "" : "not ");

	mask >>= 1;
	alg_id--;
	}
	while (mask != 0x01);

	other_decode_bitfield_value(a_bigbuf, oct, mask, 8);
	proto_tree_add_text(tree,
	tvb, curr_offset, 1,
	"%s :  No encryption: %spermitted",
	a_bigbuf,
	(mask & oct) ? "" : "not ");

	curr_offset++;

	NO_MORE_DATA_CHECK(len);

	proto_tree_add_text(tree,
	tvb, curr_offset, len - (curr_offset - offset),
			"Key: %s",
			tvb_bytes_to_str(tvb, curr_offset, len-(curr_offset-offset) ));

	curr_offset += len - (curr_offset - offset);

	EXTRANEOUS_DATA_CHECK(len, curr_offset - offset);

	return(curr_offset - offset);
}

/*
 * [2] 3.2.2.11
 */
guint8
be_chan_type(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len)
{
	guint8	oct;
	guint8	sdi;
	guint8	num_chan;
	guint32	curr_offset;
	const gchar *str;

	curr_offset = offset;

	oct = tvb_get_guint8(tvb, curr_offset);

	other_decode_bitfield_value(a_bigbuf, oct, 0xf0, 8);
	proto_tree_add_text(tree,
	tvb, curr_offset, 1,
	"%s :  Spare",
	a_bigbuf);

	sdi = oct & 0x0f;
	switch (sdi)
	{
	case 1: str = "Speech"; break;
	case 2: str = "Data"; break;
	case 3: str = "Signalling"; break;
	default:
	str = "Reserved";
	break;
	}

	other_decode_bitfield_value(a_bigbuf, oct, 0x0f, 8);
	proto_tree_add_text(tree,
	tvb, curr_offset, 1,
	"%s :  Speech/Data Indicator: %s",
	a_bigbuf,
	str);

	if (add_string)
	g_snprintf(add_string, string_len, " - (%s)", str);

	curr_offset++;

	NO_MORE_DATA_CHECK(len);

	oct = tvb_get_guint8(tvb, curr_offset);

	if (sdi == 0x01)
	{
	/* speech */

	switch (oct)
	{
	case 0x08: str = "Full rate TCH channel Bm.  Prefer full rate TCH"; break;
	case 0x09: str = "Half rate TCH channel Lm.  Prefer half rate TCH"; break;
	case 0x0a: str = "Full or Half rate channel, Full rate preferred changes allowed after first allocation"; break;
	case 0x0b: str = "Full or Half rate channel, Half rate preferred changes allowed after first allocation"; break;
	case 0x1a: str = "Full or Half rate channel, Full rate preferred changes between full and half rate not allowed after first allocation"; break;
	case 0x1b: str = "Full or Half rate channel, Half rate preferred changes between full and half rate not allowed after first allocation"; break;
	case 0x0f: str = "Full or Half rate channel, changes allowed after first allocation"; break;
	case 0x1f: str = "Full or Half rate channel, changes between full and half rate not allowed after first allocation"; break;
	default:
	    str = "Reserved";
	    break;
	}

	proto_tree_add_text(tree,
	    tvb, curr_offset, 1,
	    "Channel Rate and Type: %s",
	    str);

	curr_offset++;

	NO_MORE_DATA_CHECK(len);

	do
	{
	    oct = tvb_get_guint8(tvb, curr_offset);

	    other_decode_bitfield_value(a_bigbuf, oct, 0x80, 8);
	    proto_tree_add_text(tree,
		tvb, curr_offset, 1,
		"%s :  Extension: %s",
		a_bigbuf,
		(oct & 0x80) ? "extended" : "not extended");

	    switch (oct & 0x7f)
	    {
	    case 0x01: str = "GSM speech full rate version 1"; break;
	    case 0x11: str = "GSM speech full rate version 2"; break;
	    case 0x21: str = "GSM speech full rate version 3 (AMR)"; break;

	    case 0x05: str = "GSM speech half rate version 1"; break;
	    case 0x15: str = "GSM speech half rate version 2"; break;
	    case 0x25: str = "GSM speech half rate version 3 (AMR)"; break;

	    default:
		str = "Reserved";
		break;
	    }

	    other_decode_bitfield_value(a_bigbuf, oct, 0x7f, 8);
	    proto_tree_add_text(tree,
		tvb, curr_offset, 1,
		"%s :  Speech version identifier: %s",
		a_bigbuf,
		str);

	    curr_offset++;
	}
	while ((len - (curr_offset - offset)) > 0);
	}
	else if (sdi == 0x02)
	{
	/* data */

	num_chan = 0;

	switch (oct)
	{
	case 0x08: str = "Full rate TCH channel Bm"; break;
	case 0x09: str = "Half rate TCH channel Lm"; break;
	case 0x0a: str = "Full or Half rate TCH channel, Full rate preferred, changes allowed also after first channel allocation as a result of the request"; break;
	case 0x0b: str = "Full or Half rate TCH channel, Half rate preferred, changes allowed also after first channel allocation as a result of the request"; break;
	case 0x1a: str = "Full or Half rate TCH channel, Full rate preferred, changes not allowed after first channel allocation as a result of the request"; break;
	case 0x1b: str = "Full or Half rate TCH channel. Half rate preferred, changes not allowed after first channel allocation as a result of the request"; break;
	default:
	    if ((oct >= 0x20) && (oct <= 0x27))
	    {
		str = "Full rate TCH channels in a multislot configuration, changes by the BSS of the the number of TCHs and if applicable the used radio interface rate per channel allowed after first channel allocation as a result of the request";

		num_chan = (oct - 0x20) + 1;
	    }
	    else if ((oct >= 0x30) && (oct <= 0x37))
	    {
		str = "Full rate TCH channels in a multislot configuration, changes by the BSS of the number of TCHs or the used radio interface rate per channel not allowed after first channel allocation as a result of the request";

		num_chan = (oct - 0x30) + 1;
	    }
	    else
	    {
		str = "Reserved";
	    }
	    break;
	}

	if (num_chan > 0)
	{
	    proto_tree_add_text(tree,
		tvb, curr_offset, 1,
		"Channel Rate and Type: Max channels %u, %s",
		num_chan,
		str);
	}
	else
	{
	    proto_tree_add_text(tree,
		tvb, curr_offset, 1,
		"Channel Rate and Type: %s",
		str);
	}

	curr_offset++;

	NO_MORE_DATA_CHECK(len);

	oct = tvb_get_guint8(tvb, curr_offset);

	other_decode_bitfield_value(a_bigbuf, oct, 0x80, 8);
	proto_tree_add_text(tree,
	    tvb, curr_offset, 1,
	    "%s :  Extension: %s",
	    a_bigbuf,
	    (oct & 0x80) ? "extended" : "not extended");

	other_decode_bitfield_value(a_bigbuf, oct, 0x40, 8);
	proto_tree_add_text(tree,
	    tvb, curr_offset, 1,
	    "%s :  %sTransparent service",
	    a_bigbuf,
	    (oct & 0x40) ? "Non-" : "");

	if (num_chan == 0)
	{
	    if (oct & 0x40)
	    {
		/* non-transparent */

		switch (oct & 0x3f)
		{
		case 0x00: str = "12 kbit/s if the channel is a full rate TCH, or 6 kbit/s if the channel is a half rate TCH"; break;
		case 0x18: str = "14.5 kbit/s"; break;
		case 0x10: str = "12 kbits/s"; break;
		case 0x11: str = "6 kbits/s"; break;
		default:
		    str = "Reserved";
		    break;
		}
	    }
	    else
	    {
		switch (oct & 0x3f)
		{
		case 0x18: str = "14.4 kbit/s"; break;
		case 0x10: str = "9.6kbit/s"; break;
		case 0x11: str = "4.8kbit/s"; break;
		case 0x12: str = "2.4kbit/s"; break;
		case 0x13: str = "1.2Kbit/s"; break;
		case 0x14: str = "600 bit/s"; break;
		case 0x15: str = "1200/75 bit/s (1200 network-to-MS / 75 MS-to-network)"; break;
		default:
		    str = "Reserved";
		    break;
		}
	    }
	}
	else
	{
	    if (oct & 0x40)
	    {
		/* non-transparent */

		switch (oct & 0x3f)
		{
		case 0x16: str = "58 kbit/s (4x14.5 kbit/s)"; break;
		case 0x14: str = "48.0 / 43.5 kbit/s (4x12 kbit/s or 3x14.5 kbit/s)"; break;
		case 0x13: str = "36.0 / 29.0 kbit/s (3x12 kbit/s or 2x14.5 kbit/s)"; break;
		case 0x12: str = "24.0 / 24.0 (4x6 kbit/s or 2x12 kbit/s)"; break;
		case 0x11: str = "18.0 / 14.5 kbit/s (3x6 kbit/s or 1x14.5 kbit/s)"; break;
		case 0x10: str = "12.0 / 12.0 kbit/s (2x6 kbit/s or 1x12 kbit/s)"; break;
		default:
		    str = "Reserved";
		    break;
		}
	    }
	    else
	    {
		switch (oct & 0x3f)
		{
		case 0x1f: str = "64 kbit/s, bit transparent"; break;
		case 0x1e: str = "56 kbit/s, bit transparent"; break;
		case 0x1d: str = "56 kbit/s"; break;
		case 0x1c: str = "48 kbit/s"; break;
		case 0x1b: str = "38.4 kbit/s"; break;
		case 0x1a: str = "28.8 kbit/s"; break;
		case 0x19: str = "19.2 kbit/s"; break;
		case 0x18: str = "14.4 kbit/s"; break;
		case 0x10: str = "9.6 kbit/s"; break;
		default:
		    str = "Reserved";
		    break;
		}
	    }
	}

	other_decode_bitfield_value(a_bigbuf, oct, 0x3f, 8);
	proto_tree_add_text(tree,
	    tvb, curr_offset, 1,
	    "%s :  Rate: %s",
	    a_bigbuf,
	    str);

	curr_offset++;

	NO_MORE_DATA_CHECK(len);

	oct = tvb_get_guint8(tvb, curr_offset);

	other_decode_bitfield_value(a_bigbuf, oct, 0x80, 8);
	proto_tree_add_text(tree,
	    tvb, curr_offset, 1,
	    "%s :  Extension: %s",
	    a_bigbuf,
	    (oct & 0x80) ? "extended" : "not extended");

	other_decode_bitfield_value(a_bigbuf, oct, 0x70, 8);
	proto_tree_add_text(tree,
	    tvb, curr_offset, 1,
	    "%s :  Spare",
	    a_bigbuf);

	if (num_chan == 0)
	{
	    other_decode_bitfield_value(a_bigbuf, oct, 0x08, 8);
	    proto_tree_add_text(tree,
		tvb, curr_offset, 1,
		"%s :  14.5 kbit/s (TCH/F14.4) %sallowed",
		a_bigbuf,
		(oct & 0x08) ? "" : "not ");

	    other_decode_bitfield_value(a_bigbuf, oct, 0x04, 8);
	    proto_tree_add_text(tree,
		tvb, curr_offset, 1,
		"%s :  Spare",
		a_bigbuf);

	    other_decode_bitfield_value(a_bigbuf, oct, 0x02, 8);
	    proto_tree_add_text(tree,
		tvb, curr_offset, 1,
		"%s :  12.0 kbit/s (TCH F/9.6) %sallowed",
		a_bigbuf,
		(oct & 0x02) ? "" : "not ");

	    other_decode_bitfield_value(a_bigbuf, oct, 0x01, 8);
	    proto_tree_add_text(tree,
		tvb, curr_offset, 1,
		"%s :  6.0 kbit/s (TCH F/4.8) %sallowed",
		a_bigbuf,
		(oct & 0x01) ? "" : "not ");
	}
	else
	{
	    other_decode_bitfield_value(a_bigbuf, oct, 0x08, 8);
	    proto_tree_add_text(tree,
		tvb, curr_offset, 1,
		"%s :  14.5/14.4 kbit/s (TCH/F14.4) %sallowed",
		a_bigbuf,
		(oct & 0x08) ? "" : "not ");

	    other_decode_bitfield_value(a_bigbuf, oct, 0x04, 8);
	    proto_tree_add_text(tree,
		tvb, curr_offset, 1,
		"%s :  Spare",
		a_bigbuf);

	    other_decode_bitfield_value(a_bigbuf, oct, 0x02, 8);
	    proto_tree_add_text(tree,
		tvb, curr_offset, 1,
		"%s :  12.0/9.6 kbit/s (TCH F/9.6) %sallowed",
		a_bigbuf,
		(oct & 0x02) ? "" : "not ");

	    other_decode_bitfield_value(a_bigbuf, oct, 0x01, 8);
	    proto_tree_add_text(tree,
		tvb, curr_offset, 1,
		"%s :  6.0/4.8 kbit/s (TCH F/4.8) %sallowed",
		a_bigbuf,
		(oct & 0x01) ? "" : "not ");
	}

	curr_offset++;
	}
	else if (sdi == 0x03)
	{
	/* signalling */

	switch (oct)
	{
	case 0x00: str = "SDCCH or Full rate TCH channel Bm or Half rate TCH channel Lm"; break;
	case 0x01: str = "SDCCH"; break;
	case 0x02: str = "SDCCH or Full rate TCH channel Bm"; break;
	case 0x03: str = "SDCCH or Half rate TCH channel Lm"; break;
	case 0x08: str = "Full rate TCH channel Bm"; break;
	case 0x09: str = "Half rate TCH channel Lm"; break;
	case 0x0a: str = "Full or Half rate TCH channel, Full rate preferred, changes allowed also after first channel allocation as a result of the request"; break;
	case 0x0b: str = "Full or Half rate TCH channel, Half rate preferred, changes allowed also after first channel allocation as a result of the request"; break;
	case 0x1a: str = "Full or Half rate TCH channel, Full rate preferred, changes not allowed after first channel allocation as a result of the request"; break;
	case 0x1b: str = "Full or Half rate TCH channel. Half rate preferred, changes not allowed after first channel allocation as a result of the request"; break;
	default:
	    str = "Reserved";
	    break;
	}

	proto_tree_add_text(tree,
	    tvb, curr_offset, 1,
	    "Channel Rate and Type: %s",
	    str);

	curr_offset++;

	NO_MORE_DATA_CHECK(len);

	proto_tree_add_text(tree,
	    tvb, curr_offset, len - (curr_offset - offset),
	    "Spare");

	curr_offset += len - (curr_offset - offset);
	}
	else
	{
	/* unknown format */

	proto_tree_add_text(tree,
	    tvb, curr_offset, len - (curr_offset - offset),
	    "Unknown format");

	curr_offset += len - (curr_offset - offset);
	}

	EXTRANEOUS_DATA_CHECK(len, curr_offset - offset);

	return(curr_offset - offset);
}

/*
 * [2] 3.2.2.17
 * Formats everything after the discriminator, shared function
 */
guint8
be_cell_id_aux(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len, guint8 disc)
{
	guint32	value;
	guint32	curr_offset;

	if (add_string)
	add_string[0] = '\0';
	curr_offset = offset;

	switch (disc)
	{
	case 0x00:
	/* FALLTHRU */

	case 0x04:
	/* FALLTHRU */

	case 0x08:  /* For intersystem handover from GSM to UMTS or cdma2000: */

	curr_offset = dissect_e212_mcc_mnc(tvb, tree, curr_offset);

	/* FALLTHRU */

	case 0x01:
	case 0x05:
	case 0x0a: /*For intersystem handover from GSM to UMTS or cdma2000: */

	/* LAC */

	value = tvb_get_ntohs(tvb, curr_offset);

	proto_tree_add_item(tree, hf_gsm_a_cell_lac, tvb, curr_offset, 2, FALSE);
	
	curr_offset += 2;

	if (add_string)
	    g_snprintf(add_string, string_len, " - LAC (0x%04x)", value);

	/* FALLTHRU */

	case 0x09: /* For intersystem handover from GSM to UMTS or cdma2000: */

	if ((disc == 0x08) ||(disc == 0x09) || (disc == 0x0a)){
		/* RNC-ID */
		value = tvb_get_ntohs(tvb, curr_offset);
		proto_tree_add_item(tree, hf_gsm_a_be_rnc_id, tvb, curr_offset, 2, FALSE);

		if (add_string)
		{
		    if (add_string[0] == '\0')
		    {
			g_snprintf(add_string, string_len, " - RNC-ID (%u)", value);
		    }
		    else
		    {
			g_snprintf(add_string, string_len, "%s/RNC-ID (%u)", add_string, value);
		    }
		}
		break;
	}

	if ((disc == 0x04) || (disc == 0x05) || (disc == 0x08)) break;

	/* FALLTHRU */

	case 0x02:

	/* CI */

	value = tvb_get_ntohs(tvb, curr_offset);

	proto_tree_add_uint(tree, hf_gsm_a_cell_ci, tvb,
	    curr_offset, 2, value);

	curr_offset += 2;

	if (add_string)
	{
	    if (add_string[0] == '\0')
	    {
		g_snprintf(add_string, string_len, " - CI (%u)", value);
	    }
	    else
	    {
		g_snprintf(add_string, string_len, "%s/CI (%u)", add_string, value);
	    }
	}
	break;

	default:
	proto_tree_add_text(tree, tvb, curr_offset, len,
	    "Cell ID - Unknown format");

	curr_offset += (len);
	break;
	}

	return(curr_offset - offset);
}

static guint8
be_cell_id(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len)
{
	guint8	oct;
	guint8	disc;
	guint32	curr_offset;

	len = len;
	curr_offset = offset;

	oct = tvb_get_guint8(tvb, curr_offset);

	other_decode_bitfield_value(a_bigbuf, oct, 0xf0, 8);
	proto_tree_add_text(tree,
	tvb, curr_offset, 1,
	"%s :  Spare",
	a_bigbuf);

	proto_tree_add_item(tree, hf_gsm_a_be_cell_id_disc, tvb, curr_offset, 1, FALSE);
	disc = oct&0x0f;
	curr_offset++;

	NO_MORE_DATA_CHECK(len);

	curr_offset +=
	be_cell_id_aux(tvb, tree, curr_offset, len - (curr_offset - offset), add_string, string_len, disc);

	/* no length check possible */

	return(curr_offset - offset);
}

/*
 * [2] 3.2.2.18
 */
static guint8
be_prio(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len)
{
	guint8	oct;
	guint32	curr_offset;
	const gchar *str;

	len = len;
	curr_offset = offset;

	oct = tvb_get_guint8(tvb, curr_offset);

	proto_tree_add_item(tree, hf_gsm_a_b8spare, tvb, curr_offset, 1, FALSE);

	other_decode_bitfield_value(a_bigbuf, oct, 0x40, 8);
	proto_tree_add_text(tree,
	tvb, curr_offset, 1,
	"%s :  Preemption Capability Indicator (PCI): this allocation request %s preempt an existing connection",
	a_bigbuf,
	(oct & 0x40) ? "may" : "shall not");

	switch ((oct & 0x3c) >> 2)
	{
	case 0x00: str = "Spare"; break;
	case 0x0f: str = "priority not used"; break;
	default:
	str = "1 is highest";
	break;
	}

	other_decode_bitfield_value(a_bigbuf, oct, 0x3c, 8);
	proto_tree_add_text(tree,
	tvb, curr_offset, 1,
	"%s :  Priority Level: (%u) %s",
	a_bigbuf,
	(oct & 0x3c) >> 2,
	str);

	if (add_string)
	g_snprintf(add_string, string_len, " - (%u)", (oct & 0x3c) >> 2);

	other_decode_bitfield_value(a_bigbuf, oct, 0x02, 8);
	proto_tree_add_text(tree,
	tvb, curr_offset, 1,
	"%s :  Queuing Allowed Indicator (QA): queuing %sallowed",
	a_bigbuf,
	(oct & 0x02) ? "" : "not ");

	other_decode_bitfield_value(a_bigbuf, oct, 0x01, 8);
	proto_tree_add_text(tree,
	tvb, curr_offset, 1,
	"%s :  Preemption Vulnerability Indicator (PVI): this connection %s be preempted by another allocation request",
	a_bigbuf,
	(oct & 0x01) ? "might" : "shall not");

	curr_offset++;

	/* no length check possible */

	return(curr_offset - offset);
}

/*
 * [2] 3.2.2.24
 */
static guint8
be_l3_info(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string _U_, int string_len _U_)
{
	guint32	curr_offset;
	tvbuff_t	*l3_tvb;

	curr_offset = offset;

	proto_tree_add_text(tree, tvb, curr_offset, len,
	"Layer 3 Information value");

	/*
	 * dissect the embedded DTAP message
	 */
	l3_tvb = tvb_new_subset(tvb, curr_offset, len, len);

	call_dissector(dtap_handle, l3_tvb, g_pinfo, g_tree);

	curr_offset += len;

	EXTRANEOUS_DATA_CHECK(len, curr_offset - offset);

	return(curr_offset - offset);
}

/*
 * [2] 3.2.2.25
 */
static guint8
be_dlci(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string _U_, int string_len _U_)
{
	guint8	oct;
	guint32	curr_offset;
	proto_item	*item = NULL;
	proto_tree	*subtree = NULL;

	len = len;
	curr_offset = offset;

	item =
	proto_tree_add_text(tree, tvb, curr_offset, 1,
	    "Data Link Connection Identifier");

	subtree = proto_item_add_subtree(item, ett_dlci);

	oct = tvb_get_guint8(tvb, curr_offset);

	proto_tree_add_uint(subtree, hf_gsm_a_dlci_cc, tvb, curr_offset, 1, oct);
	proto_tree_add_uint(subtree, hf_gsm_a_dlci_spare, tvb, curr_offset, 1, oct);
	proto_tree_add_uint(subtree, hf_gsm_a_dlci_sapi, tvb, curr_offset, 1, oct);

	curr_offset++;

	/* no length check possible */

	return(curr_offset - offset);
}

/*
 * [2] 3.2.2.26
 */
static guint8
be_down_dtx_flag(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string _U_, int string_len _U_)
{
	guint	oct;
	guint32	curr_offset;

	len = len;
	curr_offset = offset;

	oct = tvb_get_guint8(tvb, curr_offset);

	other_decode_bitfield_value(a_bigbuf, oct, 0xfe, 8);
	proto_tree_add_text(tree,
	tvb, curr_offset, 1,
	"%s :  Spare",
	a_bigbuf);

	other_decode_bitfield_value(a_bigbuf, oct, 0x01, 8);
	proto_tree_add_text(tree,
	tvb, curr_offset, 1,
	"%s :  BSS is %s to activate DTX in the downlink direction",
	a_bigbuf,
	(oct & 0x01) ? "forbidden" : "allowed");

	curr_offset++;

	/* no length check possible */

	return(curr_offset - offset);
}

/*
 * [2] 3.2.2.27
 */
guint8
be_cell_id_list(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len)
{
	guint8	oct;
	guint8	consumed;
	guint8	disc;
	guint8	num_cells;
	guint32	curr_offset;
	proto_item	*item = NULL;
	proto_tree	*subtree = NULL;

	curr_offset = offset;

	oct = tvb_get_guint8(tvb, curr_offset);

	other_decode_bitfield_value(a_bigbuf, oct, 0xf0, 8);
	proto_tree_add_text(tree,
	tvb, curr_offset, 1,
	"%s :  Spare",
	a_bigbuf);

	disc = oct & 0x0f;
	proto_tree_add_item(tree, hf_gsm_a_be_cell_id_disc, tvb, curr_offset, 1, FALSE);
	curr_offset++;

	NO_MORE_DATA_CHECK(len);

	num_cells = 0;
	do
	{
	item =
	    proto_tree_add_text(tree,
		tvb, curr_offset, -1,
		"Cell %u",
		num_cells + 1);

	subtree = proto_item_add_subtree(item, ett_cell_list);

	if (add_string)
	    add_string[0] = '\0';
	consumed =
	    be_cell_id_aux(tvb, subtree, curr_offset, len - (curr_offset - offset), add_string, string_len, disc);

	if (add_string && add_string[0] != '\0')
	{
	    proto_item_append_text(item, "%s", add_string ? add_string : "");
	}

	proto_item_set_len(item, consumed);

	curr_offset += consumed;

	num_cells++;
	}
	while ((len - (curr_offset - offset)) > 0 && consumed > 0);

	if (add_string) {
	g_snprintf(add_string, string_len, " - %u cell%s",
	    num_cells, plurality(num_cells, "", "s"));
	}

	EXTRANEOUS_DATA_CHECK(len, curr_offset - offset);

	return(curr_offset - offset);
}

/*
 * [2] 3.2.2.33
 */
static guint8
be_chosen_chan(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string _U_, int string_len _U_)
{
	guint8	oct;
	guint32	curr_offset;
	const gchar *str = NULL;

	len = len;
	curr_offset = offset;

	oct = tvb_get_guint8(tvb, curr_offset);

	switch ((oct & 0xf0) >> 4)
	{
	case 0: str = "No channel mode indication"; break;
	case 9: str = "Speech (full rate or half rate)"; break;
	case 14: str = "Data, 14.5 kbit/s radio interface rate"; break;
	case 11: str = "Data, 12.0 kbit/s radio interface rate"; break;
	case 12: str = "Data, 6.0 kbit/s radio interface rate"; break;
	case 13: str = "Data, 3.6 kbit/s radio interface rate"; break;
	case 8: str = "Signalling only"; break;
	default:
	str = "Reserved";
	break;
	}

	other_decode_bitfield_value(a_bigbuf, oct, 0xf0, 8);
	proto_tree_add_text(tree,
	tvb, curr_offset, 1,
	"%s :  Channel mode: %s",
	a_bigbuf,
	str);

	switch (oct & 0x0f)
	{
	case 0: str = "None"; break;
	case 1: str = "SDCCH"; break;
	case 8: str = "1 Full rate TCH"; break;
	case 9: str = "1 Half rate TCH"; break;
	case 10: str = "2 Full Rate TCHs"; break;
	case 11: str = "3 Full Rate TCHs"; break;
	case 12: str = "4 Full Rate TCHs"; break;
	case 13: str = "5 Full Rate TCHs"; break;
	case 14: str = "6 Full Rate TCHs"; break;
	case 15: str = "7 Full Rate TCHs"; break;
	case 4: str = "8 Full Rate TCHs"; break;
	default:
	str = "Reserved";
	break;
	}

	other_decode_bitfield_value(a_bigbuf, oct, 0x0f, 8);
	proto_tree_add_text(tree,
	tvb, curr_offset, 1,
	"%s :  Channel: %s",
	a_bigbuf,
	str);

	curr_offset++;

	/* no length check possible */

	return(curr_offset - offset);
}

/*
 * [2] 3.2.2.34
 */
static guint8
be_ciph_resp_mode(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string _U_, int string_len _U_)
{
	guint8	oct;
	guint32	curr_offset;

	len = len;
	curr_offset = offset;

	oct = tvb_get_guint8(tvb, curr_offset);

	other_decode_bitfield_value(a_bigbuf, oct, 0xfe, 8);
	proto_tree_add_text(tree,
	tvb, curr_offset, 1,
	"%s :  Spare",
	a_bigbuf);

	other_decode_bitfield_value(a_bigbuf, oct, 0x01, 8);
	proto_tree_add_text(tree,
	tvb, curr_offset, 1,
	"%s :  IMEISV must %sbe included by the mobile station",
	a_bigbuf,
	(oct & 0x01) ? "" : "not ");

	curr_offset++;

	/* no length check possible */

	return(curr_offset - offset);
}


/*
 * [2] 3.2.2.35
 */
static guint8
be_l3_msg(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string _U_, int string_len _U_)
{
	guint32	curr_offset;
	tvbuff_t	*l3_tvb;

	curr_offset = offset;

	proto_tree_add_text(tree, tvb, curr_offset, len,
	"Layer 3 Message Contents");

	/*
	 * dissect the embedded DTAP message
	 */
	l3_tvb = tvb_new_subset(tvb, curr_offset, len, len);

	call_dissector(dtap_handle, l3_tvb, g_pinfo, g_tree);

	curr_offset += len;

	EXTRANEOUS_DATA_CHECK(len, curr_offset - offset);

	return(curr_offset - offset);
}

/*
 * [2] 3.2.2.36 Channel Needed
 */
static guint8
be_cha_needed(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string _U_, int string_len _U_)
{
	guint32	curr_offset;

	len = len;
	curr_offset = offset;

	/* no length check possible */
   	proto_tree_add_item(tree, hf_gsm_a_rr_chnl_needed_ch1, tvb, curr_offset, 1, FALSE);
		curr_offset++;
	return(curr_offset - offset);
}


/*
 * [2] 3.2.2.43
 */
static guint8
be_for_ind(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string _U_, int string_len _U_)
{
	guint8	oct;
	guint32	curr_offset;
	const gchar *str = NULL;

	len = len;
	curr_offset = offset;

	oct = tvb_get_guint8(tvb, curr_offset);

	other_decode_bitfield_value(a_bigbuf, oct, 0xf0, 8);
	proto_tree_add_text(tree,
	tvb, curr_offset, 1,
	"%s :  Spare",
	a_bigbuf);

	switch (oct & 0x0f)
	{
	case 1: str = "forward to subsequent BSS, no trace at MSC"; break;
	case 2: str = "forward to subsequent BSS, and trace at MSC"; break;
	default:
	str = "Reserved";
	break;
	}

	other_decode_bitfield_value(a_bigbuf, oct, 0x0f, 8);
	proto_tree_add_text(tree,
	tvb, curr_offset, 1,
	"%s :  %s",
	a_bigbuf,
	str);

	curr_offset++;

	/* no length check possible */

	return(curr_offset - offset);
}

/*
 * [2] 3.2.2.44
 */
static guint8
be_chosen_enc_alg(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len)
{
	guint8	oct;
	guint32	curr_offset;
	const gchar *str = NULL;

	len = len;
	curr_offset = offset;

	oct = tvb_get_guint8(tvb, curr_offset);

	switch (oct)
	{
	case 0x01: str = "No encryption used"; break;
	case 0x02: str = "GSM A5/1"; break;
	case 0x03: str = "GSM A5/2"; break;
	case 0x04: str = "GSM A5/3"; break;
	case 0x05: str = "GSM A5/4"; break;
	case 0x06: str = "GSM A5/5"; break;
	case 0x07: str = "GSM A5/6"; break;
	case 0x08: str = "GSM A5/7"; break;
	default:
	str = "Reserved";
	break;
	}

	proto_tree_add_text(tree,
	tvb, curr_offset, 1,
	"Algorithm Identifier: %s",
	str);

	curr_offset++;

	if (add_string)
	g_snprintf(add_string, string_len, " - %s", str);

	/* no length check possible */

	return(curr_offset - offset);
}

/*
 * [2] 3.2.2.45
 */
static guint8
be_cct_pool(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len)
{
	guint8	oct;
	guint32	curr_offset;
	const gchar *str = NULL;

	len = len;
	curr_offset = offset;

	oct = tvb_get_guint8(tvb, curr_offset);

	if (oct <= 32)
	{
	str = "";
	}
	else if ((oct >= 0x80) && (oct <= 0x8f))
	{
	str = ", for national/local use";
	}
	else
	{
	str = ", reserved for future international use";
	}

	proto_tree_add_text(tree,
	tvb, curr_offset, 1,
	"Circuit pool number: %u%s",
	oct,
	str);

	curr_offset++;

	if (add_string)
	g_snprintf(add_string, string_len, " - (%u)", oct);

	/* no length check possible */

	return(curr_offset - offset);
}

/*
 * [2] 3.2.2.49
 */
static guint8
be_curr_chan_1(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string _U_, int string_len _U_)
{
	guint8	oct;
	guint32	curr_offset;
	const gchar *str;

	len = len;
	curr_offset = offset;

	oct = tvb_get_guint8(tvb, curr_offset);

	switch ((oct & 0xf0) >> 4)
	{
	case 0x00: str = "Signalling only"; break;
	case 0x01: str = "Speech (full rate or half rate)"; break;
	case 0x06: str = "Data, 14.5 kbit/s radio interface rate"; break;
	case 0x03: str = "Data, 12.0 kbit/s radio interface rate"; break;
	case 0x04: str = "Data, 6.0 kbit/s radio interface rate"; break;
	case 0x05: str = "Data, 3.6 kbit/s radio interface rate"; break;
	default:
	str = "Reserved";
	break;
	}

	other_decode_bitfield_value(a_bigbuf, oct, 0xf0, 8);
	proto_tree_add_text(tree,
	tvb, curr_offset, 1,
	"%s :  Channel Mode: %s",
	a_bigbuf,
	str);

	switch (oct & 0x0f)
	{
	case 0x01: str = "SDCCH"; break;
	case 0x08: str = "1 Full rate TCH"; break;
	case 0x09: str = "1 Half rate TCH"; break;
	case 0x0a: str = "2 Full Rate TCHs"; break;
	case 0x0b: str = "3 Full Rate TCHs"; break;
	case 0x0c: str = "4 Full Rate TCHs"; break;
	case 0x0d: str = "5 Full Rate TCHs"; break;
	case 0x0e: str = "6 Full Rate TCHs"; break;
	case 0x0f: str = "7 Full Rate TCHs"; break;
	case 0x04: str = "8 Full Rate TCHs"; break;
	default:
	str = "Reserved";
	break;
	}

	other_decode_bitfield_value(a_bigbuf, oct, 0x0f, 8);
	proto_tree_add_text(tree,
	tvb, curr_offset, 1,
	"%s :  Channel: (%u) %s",
	a_bigbuf,
	oct & 0x0f,
	str);

	curr_offset++;

	/* no length check possible */

	return(curr_offset - offset);
}

/*
 * [2] 3.2.2.50
 */
static guint8
be_que_ind(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string _U_, int string_len _U_)
{
	guint8	oct;
	guint32	curr_offset;

	len = len;
	curr_offset = offset;

	oct = tvb_get_guint8(tvb, curr_offset);

	other_decode_bitfield_value(a_bigbuf, oct, 0xfc, 8);
	proto_tree_add_text(tree,
	tvb, curr_offset, 1,
	"%s :  Spare",
	a_bigbuf);

	other_decode_bitfield_value(a_bigbuf, oct, 0x02, 8);
	proto_tree_add_text(tree,
	tvb, curr_offset, 1,
	"%s :  qri: it is recommended %sto allow queuing",
	a_bigbuf,
	(oct & 0x02) ? "" : "not ");

	other_decode_bitfield_value(a_bigbuf, oct, 0x01, 8);
	proto_tree_add_text(tree,
	tvb, curr_offset, 1,
	"%s :  Spare",
	a_bigbuf);

	curr_offset++;

	/* no length check possible */

	return(curr_offset - offset);
}

/*
 * [2] 3.2.2.51
 */
static guint8
be_speech_ver(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len)
{
	guint8	oct;
	guint32	curr_offset;
	const gchar *str = NULL;
	const gchar	*short_str = NULL;

	len = len;
	curr_offset = offset;

	oct = tvb_get_guint8(tvb, curr_offset);

	proto_tree_add_item(tree, hf_gsm_a_b8spare, tvb, curr_offset, 1, FALSE);

	switch (oct & 0x7f)
	{
	case 0x01: str = "GSM speech full rate version 1"; short_str = "FR1"; break;
	case 0x11: str = "GSM speech full rate version 2"; short_str = "FR2"; break;
	case 0x21: str = "GSM speech full rate version 3 (AMR)"; short_str = "FR3 (AMR)"; break;

	case 0x05: str = "GSM speech half rate version 1"; short_str = "HR1"; break;
	case 0x15: str = "GSM speech half rate version 2"; short_str = "HR2"; break;
	case 0x25: str = "GSM speech half rate version 3 (AMR)"; short_str = "HR3 (AMR)"; break;

	default:
	str = "Reserved";
	short_str = str;
	break;
	}

	other_decode_bitfield_value(a_bigbuf, oct, 0x7f, 8);
	proto_tree_add_text(tree,
	tvb, curr_offset, 1,
	"%s :  Speech version identifier: %s",
	a_bigbuf,
	str);

	curr_offset++;

	if (add_string)
	g_snprintf(add_string, string_len, " - (%s)", short_str);

	/* no length check possible */

	return(curr_offset - offset);
}

/*
 * 3.2.2.68 3GPP TS 48.008 version 6.9.0 Release 6
 */

/* BSSLAP the embedded message is as defined in 3GPP TS 48.071
 * LLP the embedded message contains a Facility Information Element as defined in 3GPP TS 44.071
 *		excluding the Facility IEI and length of Facility IEI octets defined in 3GPP TS 44.071.
 * SMLCPP the embedded message is as defined in 3GPP TS 48.031
 */
static const value_string gsm_a_apdu_protocol_id_strings[] = {
	{ 0,	"reserved" },
	{ 1,	"BSSLAP" },
	{ 2,	"LLP" },
	{ 3,	"SMLCPP" },
	{ 0, NULL },
};

static guint8
be_apdu(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string _U_, int string_len _U_)
{
	guint32	curr_offset;
	guint8	apdu_protocol_id;
	tvbuff_t *APDU_tvb;

	curr_offset = offset;

	/* curr_offset + 1 is a hack, the length part here is 2 octets and we are off by one */
    proto_tree_add_text(tree, tvb, curr_offset+1, len,
	"APDU");

	/*
	 * dissect the embedded APDU message
	 * if someone writes a TS 09.31 dissector
	 *
	 * The APDU octets 4 to n are coded in the same way as the
	 * equivalent octet in the APDU element of 3GPP TS 49.031.
	 */

	apdu_protocol_id = tvb_get_guint8(tvb,curr_offset+1);
	proto_tree_add_item(tree, hf_gsm_a_apdu_protocol_id, tvb, curr_offset+1, 1, FALSE);

	switch(apdu_protocol_id){
	case 1:
		/* BSSLAP
		 * the embedded message is as defined in 3GPP TS 08.71(3GPP TS 48.071 version 7.2.0 Release 7)
		 */
		APDU_tvb = tvb_new_subset(tvb, curr_offset+2, len-1, len-1);
		if(gsm_bsslap_handle)
			call_dissector(gsm_bsslap_handle, APDU_tvb, g_pinfo, g_tree);
		break;
	case 2:
		/* LLP
		 * The embedded message contains a Facility Information Element as defined in 3GPP TS 04.71
		 * excluding the Facility IEI and length of Facility IEI octets defined in 3GPP TS 04.71.
		 */
		break;
	case 3:
		/* SMLCPP
		 * The embedded message is as defined in 3GPP TS 08.31
		 */
		break;
	default:
		break;
	}

	curr_offset += len;

	EXTRANEOUS_DATA_CHECK(len, curr_offset - offset);

	return(curr_offset - offset);
}

guint8 (*bssmap_elem_fcn[])(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len) = {
	be_cic,	/* Circuit Identity Code */
	NULL,	/* Reserved */
	NULL,	/* Resource Available */
	be_cause,	/* Cause */
	be_cell_id,	/* Cell Identifier */
	be_prio,	/* Priority */
	be_l3_header_info,	/* Layer 3 Header Information */
	de_mid,	/* IMSI */
	be_tmsi,	/* TMSI */
	be_enc_info,	/* Encryption Information */
	be_chan_type,	/* Channel Type */
	NULL,	/* Periodicity */
	NULL,	/* Extended Resource Indicator */
	NULL,	/* Number Of MSs */
	NULL,	/* Reserved */
	NULL,	/* Reserved */
	NULL,	/* Reserved */
	de_ms_cm_2,	/* Classmark Information Type 2 */
	NULL,	/* Classmark Information Type 3 */
	NULL,	/* Interference Band To Be Used */
	de_rr_cause,	/* RR Cause */
	NULL,	/* Reserved */
	be_l3_info,	/* Layer 3 Information */
	be_dlci,	/* DLCI */
	be_down_dtx_flag,	/* Downlink DTX Flag */
	be_cell_id_list,	/* Cell Identifier List */
	NULL /* no associated data */,	/* Response Request */
	NULL,	/* Resource Indication Method */
	de_ms_cm_1,	/* Classmark Information Type 1 */
	NULL,	/* Circuit Identity Code List */
	NULL,	/* Diagnostic */
	be_l3_msg,	/* Layer 3 Message Contents */
	be_chosen_chan,	/* Chosen Channel */
	NULL,	/* Total Resource Accessible */
	be_ciph_resp_mode,	/* Cipher Response Mode */
	be_cha_needed,	/* Channel Needed */
	NULL,	/* Trace Type */
	NULL,	/* TriggerID */
	NULL,	/* Trace Reference */
	NULL,	/* TransactionID */
	de_mid,	/* Mobile Identity */
	NULL,	/* OMCID */
	be_for_ind,	/* Forward Indicator */
	be_chosen_enc_alg,	/* Chosen Encryption Algorithm */
	be_cct_pool,	/* Circuit Pool */
	NULL,	/* Circuit Pool List */
	NULL,	/* Time Indication */
	NULL,	/* Resource Situation */
	be_curr_chan_1,	/* Current Channel Type 1 */
	be_que_ind,	/* Queueing Indicator */
	be_speech_ver,	/* Speech Version */
	NULL,	/* Assignment Requirement */
	NULL /* no associated data */,	/* Talker Flag */
	NULL /* no associated data */,	/* Connection Release Requested */
	NULL,	/* Group Call Reference */
	NULL,	/* eMLPP Priority */
	NULL,	/* Configuration Evolution Indication */
	NULL /* no decode required */,	/* Old BSS to New BSS Information */
	NULL,	/* LSA Identifier */
	NULL,	/* LSA Identifier List */
	NULL,	/* LSA Information */
	NULL,	/* LCS QoS */
	NULL,	/* LSA access control suppression */
	NULL,	/* LCS Priority */
	NULL,	/* Location Type */
	NULL,	/* Location Estimate */
	NULL,	/* Positioning Data */
	NULL,	/* LCS Cause */
	NULL,	/* LCS Client Type */
	be_apdu,	/* APDU */
	NULL,	/* Network Element Identity */
	NULL,	/* GPS Assistance Data */
	NULL,	/* Deciphering Keys */
	NULL,	/* Return Error Request */
	NULL,	/* Return Error Cause */
	NULL,	/* Segmentation */
	NULL,	/* NONE */
};

/* MESSAGE FUNCTIONS */

/*
 *  [2] 3.2.1.1
 */
static void
bssmap_ass_req(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len)
{
	guint32	curr_offset;
	guint32	consumed;
	guint	curr_len;

	curr_offset = offset;
	curr_len = len;

	ELEM_MAND_TLV(gsm_bssmap_elem_strings[BE_CHAN_TYPE].value, BSSAP_PDU_TYPE_BSSMAP, BE_CHAN_TYPE, "");

	ELEM_OPT_TLV(gsm_bssmap_elem_strings[BE_L3_HEADER_INFO].value, BSSAP_PDU_TYPE_BSSMAP, BE_L3_HEADER_INFO, "");

	ELEM_OPT_TLV(gsm_bssmap_elem_strings[BE_PRIO].value, BSSAP_PDU_TYPE_BSSMAP, BE_PRIO, "");

	ELEM_OPT_TV(gsm_bssmap_elem_strings[BE_CIC].value, BSSAP_PDU_TYPE_BSSMAP, BE_CIC, "");

	ELEM_OPT_TV(gsm_bssmap_elem_strings[BE_DOWN_DTX_FLAG].value, BSSAP_PDU_TYPE_BSSMAP, BE_DOWN_DTX_FLAG, "");

	ELEM_OPT_TV(gsm_bssmap_elem_strings[BE_INT_BAND].value, BSSAP_PDU_TYPE_BSSMAP, BE_INT_BAND, "");

	ELEM_OPT_TLV(gsm_bssmap_elem_strings[BE_CM_INFO_2].value, BSSAP_PDU_TYPE_BSSMAP, BE_CM_INFO_2, "");

	ELEM_OPT_TLV(gsm_bssmap_elem_strings[BE_GROUP_CALL_REF].value, BSSAP_PDU_TYPE_BSSMAP, BE_GROUP_CALL_REF, "");

	ELEM_OPT_T(gsm_bssmap_elem_strings[BE_TALKER_FLAG].value, BSSAP_PDU_TYPE_BSSMAP, BE_TALKER_FLAG, "");

	ELEM_OPT_TV(gsm_bssmap_elem_strings[BE_LSA_ACC_CTRL].value, BSSAP_PDU_TYPE_BSSMAP, BE_LSA_ACC_CTRL, "");

	EXTRANEOUS_DATA_CHECK(curr_len, 0);
}

/*
 *  [2] 3.2.1.2
 */
static void
bssmap_ass_complete(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len)
{
	guint32	curr_offset;
	guint32	consumed;
	guint	curr_len;

	curr_offset = offset;
	curr_len = len;

	ELEM_OPT_TV(gsm_bssmap_elem_strings[BE_RR_CAUSE].value, BSSAP_PDU_TYPE_BSSMAP, BE_RR_CAUSE, "");

	ELEM_OPT_TV(gsm_bssmap_elem_strings[BE_CIC].value, BSSAP_PDU_TYPE_BSSMAP, BE_CIC, "");

	ELEM_OPT_TLV(gsm_bssmap_elem_strings[BE_CELL_ID].value, BSSAP_PDU_TYPE_BSSMAP, BE_CELL_ID, "");

	ELEM_OPT_TV(gsm_bssmap_elem_strings[BE_CHOSEN_CHAN].value, BSSAP_PDU_TYPE_BSSMAP, BE_CHOSEN_CHAN, "");

	ELEM_OPT_TV(gsm_bssmap_elem_strings[BE_CHOSEN_ENC_ALG].value, BSSAP_PDU_TYPE_BSSMAP, BE_CHOSEN_ENC_ALG, "");

	ELEM_OPT_TV(gsm_bssmap_elem_strings[BE_CCT_POOL].value, BSSAP_PDU_TYPE_BSSMAP, BE_CCT_POOL, "");

	ELEM_OPT_TV(gsm_bssmap_elem_strings[BE_SPEECH_VER].value, BSSAP_PDU_TYPE_BSSMAP, BE_SPEECH_VER, " (Chosen)");

	ELEM_OPT_TLV(gsm_bssmap_elem_strings[BE_LSA_ID].value, BSSAP_PDU_TYPE_BSSMAP, BE_LSA_ID, "");

	EXTRANEOUS_DATA_CHECK(curr_len, 0);
}

/*
 *  [2] 3.2.1.3
 */
static void
bssmap_ass_failure(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len)
{
	guint32	curr_offset;
	guint32	consumed;
	guint	curr_len;

	curr_offset = offset;
	curr_len = len;

	ELEM_MAND_TLV(gsm_bssmap_elem_strings[BE_CAUSE].value, BSSAP_PDU_TYPE_BSSMAP, BE_CAUSE, "");

	ELEM_OPT_TV(gsm_bssmap_elem_strings[BE_RR_CAUSE].value, BSSAP_PDU_TYPE_BSSMAP, BE_RR_CAUSE, "");

	ELEM_OPT_TV(gsm_bssmap_elem_strings[BE_CCT_POOL].value, BSSAP_PDU_TYPE_BSSMAP, BE_CCT_POOL, "");

	ELEM_OPT_TLV(gsm_bssmap_elem_strings[BE_CCT_POOL_LIST].value, BSSAP_PDU_TYPE_BSSMAP, BE_CCT_POOL_LIST, "");

	EXTRANEOUS_DATA_CHECK(curr_len, 0);
}

/*
 *  [2] 3.2.1.4
 */
static void
bssmap_block(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len)
{
	guint32	curr_offset;
	guint32	consumed;
	guint	curr_len;

	curr_offset = offset;
	curr_len = len;

	ELEM_MAND_TLV(gsm_bssmap_elem_strings[BE_CAUSE].value, BSSAP_PDU_TYPE_BSSMAP, BE_CAUSE, "");

	ELEM_MAND_TV(gsm_bssmap_elem_strings[BE_CIC].value, BSSAP_PDU_TYPE_BSSMAP, BE_CIC, "");

	ELEM_OPT_T(gsm_bssmap_elem_strings[BE_CONN_REL_REQ].value, BSSAP_PDU_TYPE_BSSMAP, BE_CONN_REL_REQ, "");

	EXTRANEOUS_DATA_CHECK(curr_len, 0);
}

/*
 *  [2] 3.2.1.5
 */
static void
bssmap_block_ack(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len)
{
	guint32	curr_offset;
	guint32	consumed;
	guint	curr_len;

	curr_offset = offset;
	curr_len = len;

	ELEM_MAND_TV(gsm_bssmap_elem_strings[BE_CIC].value, BSSAP_PDU_TYPE_BSSMAP, BE_CIC, "");

	EXTRANEOUS_DATA_CHECK(curr_len, 0);
}

/*
 *  [2] 3.2.1.6
 */
static void
bssmap_unblock(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len)
{
	guint32	curr_offset;
	guint32	consumed;
	guint	curr_len;

	curr_offset = offset;
	curr_len = len;

	ELEM_MAND_TV(gsm_bssmap_elem_strings[BE_CIC].value, BSSAP_PDU_TYPE_BSSMAP, BE_CIC, "");

	ELEM_OPT_T(gsm_bssmap_elem_strings[BE_CONN_REL_REQ].value, BSSAP_PDU_TYPE_BSSMAP, BE_CONN_REL_REQ, "");

	EXTRANEOUS_DATA_CHECK(curr_len, 0);
}

/*
 *  [2] 3.2.1.7
 */
static void
bssmap_unblock_ack(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len)
{
	guint32	curr_offset;
	guint32	consumed;
	guint	curr_len;

	curr_offset = offset;
	curr_len = len;

	ELEM_MAND_TV(gsm_bssmap_elem_strings[BE_CIC].value, BSSAP_PDU_TYPE_BSSMAP, BE_CIC, "");

	EXTRANEOUS_DATA_CHECK(curr_len, 0);
}

/*
 *  [2] 3.2.1.8
 */
static void
bssmap_ho_req(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len)
{
	guint32	curr_offset;
	guint32	consumed;
	guint	curr_len;

	curr_offset = offset;
	curr_len = len;

	ELEM_MAND_TLV(gsm_bssmap_elem_strings[BE_CHAN_TYPE].value, BSSAP_PDU_TYPE_BSSMAP, BE_CHAN_TYPE, "");

	ELEM_MAND_TLV(gsm_bssmap_elem_strings[BE_ENC_INFO].value, BSSAP_PDU_TYPE_BSSMAP, BE_ENC_INFO, "");

	ELEM_OPT_TV(gsm_bssmap_elem_strings[BE_CM_INFO_1].value, BSSAP_PDU_TYPE_BSSMAP, BE_CM_INFO_1, "");

	ELEM_OPT_TLV(gsm_bssmap_elem_strings[BE_CM_INFO_2].value, BSSAP_PDU_TYPE_BSSMAP, BE_CM_INFO_2, "");

	ELEM_MAND_TLV(gsm_bssmap_elem_strings[BE_CELL_ID].value, BSSAP_PDU_TYPE_BSSMAP, BE_CELL_ID, " (Serving)");

	ELEM_OPT_TLV(gsm_bssmap_elem_strings[BE_PRIO].value, BSSAP_PDU_TYPE_BSSMAP, BE_PRIO, "");

	ELEM_OPT_TV(gsm_bssmap_elem_strings[BE_CIC].value, BSSAP_PDU_TYPE_BSSMAP, BE_CIC, "");

	ELEM_OPT_TV(gsm_bssmap_elem_strings[BE_DOWN_DTX_FLAG].value, BSSAP_PDU_TYPE_BSSMAP, BE_DOWN_DTX_FLAG, "");

	ELEM_MAND_TLV(gsm_bssmap_elem_strings[BE_CELL_ID].value, BSSAP_PDU_TYPE_BSSMAP, BE_CELL_ID, " (Target)");

	ELEM_OPT_TV(gsm_bssmap_elem_strings[BE_INT_BAND].value, BSSAP_PDU_TYPE_BSSMAP, BE_INT_BAND, "");

	ELEM_OPT_TLV(gsm_bssmap_elem_strings[BE_CAUSE].value, BSSAP_PDU_TYPE_BSSMAP, BE_CAUSE, "");

	ELEM_OPT_TLV(gsm_bssmap_elem_strings[BE_CM_INFO_3].value, BSSAP_PDU_TYPE_BSSMAP, BE_CM_INFO_3, "");

	ELEM_OPT_TV(gsm_bssmap_elem_strings[BE_CURR_CHAN_1].value, BSSAP_PDU_TYPE_BSSMAP, BE_CURR_CHAN_1, "");

	ELEM_OPT_TV(gsm_bssmap_elem_strings[BE_SPEECH_VER].value, BSSAP_PDU_TYPE_BSSMAP, BE_SPEECH_VER, " (Used)");

	ELEM_OPT_TLV(gsm_bssmap_elem_strings[BE_GROUP_CALL_REF].value, BSSAP_PDU_TYPE_BSSMAP, BE_GROUP_CALL_REF, "");

	ELEM_OPT_T(gsm_bssmap_elem_strings[BE_TALKER_FLAG].value, BSSAP_PDU_TYPE_BSSMAP, BE_TALKER_FLAG, "");

	ELEM_OPT_TV(gsm_bssmap_elem_strings[BE_CONF_EVO_IND].value, BSSAP_PDU_TYPE_BSSMAP, BE_CONF_EVO_IND, "");

	ELEM_OPT_TV(gsm_bssmap_elem_strings[BE_CHOSEN_ENC_ALG].value, BSSAP_PDU_TYPE_BSSMAP, BE_CHOSEN_ENC_ALG, " (Serving)");

	ELEM_OPT_TLV(gsm_bssmap_elem_strings[BE_OLD2NEW_INFO].value, BSSAP_PDU_TYPE_BSSMAP, BE_OLD2NEW_INFO, "");

	ELEM_OPT_TLV(gsm_bssmap_elem_strings[BE_LSA_INFO].value, BSSAP_PDU_TYPE_BSSMAP, BE_LSA_INFO, "");

	ELEM_OPT_TV(gsm_bssmap_elem_strings[BE_LSA_ACC_CTRL].value, BSSAP_PDU_TYPE_BSSMAP, BE_LSA_ACC_CTRL, "");

	EXTRANEOUS_DATA_CHECK(curr_len, 0);
}

/*
 *  [2] 3.2.1.9
 */
static void
bssmap_ho_reqd(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len)
{
	guint32	curr_offset;
	guint32	consumed;
	guint	curr_len;

	curr_offset = offset;
	curr_len = len;

	ELEM_MAND_TLV(gsm_bssmap_elem_strings[BE_CAUSE].value, BSSAP_PDU_TYPE_BSSMAP, BE_CAUSE, "");

	ELEM_OPT_T(gsm_bssmap_elem_strings[BE_RESP_REQ].value, BSSAP_PDU_TYPE_BSSMAP, BE_RESP_REQ, "");

	ELEM_MAND_TLV(gsm_bssmap_elem_strings[BE_CELL_ID_LIST].value, BSSAP_PDU_TYPE_BSSMAP, BE_CELL_ID_LIST, " (Preferred)");

	ELEM_OPT_TLV(gsm_bssmap_elem_strings[BE_CCT_POOL_LIST].value, BSSAP_PDU_TYPE_BSSMAP, BE_CCT_POOL_LIST, "");

	ELEM_OPT_TV(gsm_bssmap_elem_strings[BE_CURR_CHAN_1].value, BSSAP_PDU_TYPE_BSSMAP, BE_CURR_CHAN_1, "");

	ELEM_OPT_TV(gsm_bssmap_elem_strings[BE_SPEECH_VER].value, BSSAP_PDU_TYPE_BSSMAP, BE_SPEECH_VER, " (Used)");

	ELEM_OPT_TV(gsm_bssmap_elem_strings[BE_QUE_IND].value, BSSAP_PDU_TYPE_BSSMAP, BE_QUE_IND, "");

	ELEM_OPT_TLV(gsm_bssmap_elem_strings[BE_OLD2NEW_INFO].value, BSSAP_PDU_TYPE_BSSMAP, BE_OLD2NEW_INFO, "");

	EXTRANEOUS_DATA_CHECK(curr_len, 0);
}

/*
 *  [2] 3.2.1.10
 */
static void
bssmap_ho_req_ack(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len)
{
	guint32	curr_offset;
	guint32	consumed;
	guint	curr_len;

	curr_offset = offset;
	curr_len = len;

	ELEM_MAND_TLV(gsm_bssmap_elem_strings[BE_L3_INFO].value, BSSAP_PDU_TYPE_BSSMAP, BE_L3_INFO, "");

	ELEM_OPT_TV(gsm_bssmap_elem_strings[BE_CHOSEN_CHAN].value, BSSAP_PDU_TYPE_BSSMAP, BE_CHOSEN_CHAN, "");

	ELEM_OPT_TV(gsm_bssmap_elem_strings[BE_CHOSEN_ENC_ALG].value, BSSAP_PDU_TYPE_BSSMAP, BE_CHOSEN_ENC_ALG, "");

	ELEM_OPT_TV(gsm_bssmap_elem_strings[BE_CCT_POOL].value, BSSAP_PDU_TYPE_BSSMAP, BE_CCT_POOL, "");

	ELEM_OPT_TV(gsm_bssmap_elem_strings[BE_SPEECH_VER].value, BSSAP_PDU_TYPE_BSSMAP, BE_SPEECH_VER, " (Chosen)");

	ELEM_OPT_TV(gsm_bssmap_elem_strings[BE_CIC].value, BSSAP_PDU_TYPE_BSSMAP, BE_CIC, "");

	ELEM_OPT_TLV(gsm_bssmap_elem_strings[BE_LSA_ID].value, BSSAP_PDU_TYPE_BSSMAP, BE_LSA_ID, "");

	EXTRANEOUS_DATA_CHECK(curr_len, 0);
}

/*
 *  [2] 3.2.1.11
 */
static void
bssmap_ho_cmd(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len)
{
	guint32	curr_offset;
	guint32	consumed;
	guint	curr_len;

	curr_offset = offset;
	curr_len = len;

	ELEM_MAND_TLV(gsm_bssmap_elem_strings[BE_L3_INFO].value, BSSAP_PDU_TYPE_BSSMAP, BE_L3_INFO, "");

	ELEM_OPT_TLV(gsm_bssmap_elem_strings[BE_CELL_ID].value, BSSAP_PDU_TYPE_BSSMAP, BE_CELL_ID, "");

	EXTRANEOUS_DATA_CHECK(curr_len, 0);
}

/*
 *  [2] 3.2.1.12
 */
static void
bssmap_ho_complete(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len)
{
	guint32	curr_offset;
	guint32	consumed;
	guint	curr_len;

	curr_offset = offset;
	curr_len = len;

	ELEM_OPT_TV(gsm_bssmap_elem_strings[BE_RR_CAUSE].value, BSSAP_PDU_TYPE_BSSMAP, BE_RR_CAUSE, "");

	EXTRANEOUS_DATA_CHECK(curr_len, 0);
}

/*
 *  [2] 3.2.1.14
 */
static void
bssmap_ho_cand_enq(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len)
{
	guint32	curr_offset;
	guint32	consumed;
	guint	curr_len;

	curr_offset = offset;
	curr_len = len;

	ELEM_MAND_TV(gsm_bssmap_elem_strings[BE_NUM_MS].value, BSSAP_PDU_TYPE_BSSMAP, BE_NUM_MS, "");

	ELEM_MAND_TLV(gsm_bssmap_elem_strings[BE_CELL_ID_LIST].value, BSSAP_PDU_TYPE_BSSMAP, BE_CELL_ID_LIST, "");

	ELEM_MAND_TLV(gsm_bssmap_elem_strings[BE_CELL_ID].value, BSSAP_PDU_TYPE_BSSMAP, BE_CELL_ID, "");

	EXTRANEOUS_DATA_CHECK(curr_len, 0);
}

/*
 *  [2] 3.2.1.15
 */
static void
bssmap_ho_cand_resp(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len)
{
	guint32	curr_offset;
	guint32	consumed;
	guint	curr_len;

	curr_offset = offset;
	curr_len = len;

	ELEM_MAND_TV(gsm_bssmap_elem_strings[BE_NUM_MS].value, BSSAP_PDU_TYPE_BSSMAP, BE_NUM_MS, "");

	ELEM_MAND_TLV(gsm_bssmap_elem_strings[BE_CELL_ID].value, BSSAP_PDU_TYPE_BSSMAP, BE_CELL_ID, "");

	EXTRANEOUS_DATA_CHECK(curr_len, 0);
}

/*
 *  [2] 3.2.1.16
 */
static void
bssmap_ho_failure(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len)
{
	guint32	curr_offset;
	guint32	consumed;
	guint	curr_len;

	curr_offset = offset;
	curr_len = len;

	ELEM_MAND_TLV(gsm_bssmap_elem_strings[BE_CAUSE].value, BSSAP_PDU_TYPE_BSSMAP, BE_CAUSE, "");

	ELEM_OPT_TV(gsm_bssmap_elem_strings[BE_RR_CAUSE].value, BSSAP_PDU_TYPE_BSSMAP, BE_RR_CAUSE, "");

	ELEM_OPT_TV(gsm_bssmap_elem_strings[BE_CCT_POOL].value, BSSAP_PDU_TYPE_BSSMAP, BE_CCT_POOL, "");

	ELEM_OPT_TLV(gsm_bssmap_elem_strings[BE_CCT_POOL_LIST].value, BSSAP_PDU_TYPE_BSSMAP, BE_CCT_POOL_LIST, "");

	EXTRANEOUS_DATA_CHECK(curr_len, 0);
}

/*
 *  [2] 3.2.1.19
 */
static void
bssmap_paging(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len)
{
	guint32	curr_offset;
	guint32	consumed;
	guint	curr_len;

	curr_offset = offset;
	curr_len = len;

	ELEM_MAND_TLV(gsm_bssmap_elem_strings[BE_IMSI].value, BSSAP_PDU_TYPE_BSSMAP, BE_IMSI, "");

	ELEM_OPT_TLV(gsm_bssmap_elem_strings[BE_TMSI].value, BSSAP_PDU_TYPE_BSSMAP, BE_TMSI, "");

	ELEM_MAND_TLV(gsm_bssmap_elem_strings[BE_CELL_ID_LIST].value, BSSAP_PDU_TYPE_BSSMAP, BE_CELL_ID_LIST, "");

	ELEM_OPT_TV(gsm_bssmap_elem_strings[BE_CHAN_NEEDED].value, BSSAP_PDU_TYPE_BSSMAP, BE_CHAN_NEEDED, "");

	ELEM_OPT_TV(gsm_bssmap_elem_strings[BE_EMLPP_PRIO].value, BSSAP_PDU_TYPE_BSSMAP, BE_EMLPP_PRIO, "");

	EXTRANEOUS_DATA_CHECK(curr_len, 0);
}

/*
 *  [2] 3.2.1.20
 */
static void
bssmap_clear_req(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len)
{
	guint32	curr_offset;
	guint32	consumed;
	guint	curr_len;

	curr_offset = offset;
	curr_len = len;

	ELEM_MAND_TLV(gsm_bssmap_elem_strings[BE_CAUSE].value, BSSAP_PDU_TYPE_BSSMAP, BE_CAUSE, "");

	EXTRANEOUS_DATA_CHECK(curr_len, 0);
}

/*
 *  [2] 3.2.1.21
 */
static void
bssmap_clear_cmd(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len)
{
	guint32	curr_offset;
	guint32	consumed;
	guint	curr_len;

	curr_offset = offset;
	curr_len = len;

	ELEM_OPT_TLV(gsm_bssmap_elem_strings[BE_L3_HEADER_INFO].value, BSSAP_PDU_TYPE_BSSMAP, BE_L3_HEADER_INFO, "");

	ELEM_MAND_TLV(gsm_bssmap_elem_strings[BE_CAUSE].value, BSSAP_PDU_TYPE_BSSMAP, BE_CAUSE, "");

	EXTRANEOUS_DATA_CHECK(curr_len, 0);
}

/*
 *  [2] 3.2.1.23
 */
static void
bssmap_reset(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len)
{
	guint32	curr_offset;
	guint32	consumed;
	guint	curr_len;

	curr_offset = offset;
	curr_len = len;

	ELEM_MAND_TLV(gsm_bssmap_elem_strings[BE_CAUSE].value, BSSAP_PDU_TYPE_BSSMAP, BE_CAUSE, "");

	EXTRANEOUS_DATA_CHECK(curr_len, 0);
}

/*
 *  [2] 3.2.1.25
 */
static void
bssmap_ho_performed(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len)
{
	guint32	curr_offset;
	guint32	consumed;
	guint	curr_len;

	curr_offset = offset;
	curr_len = len;

	ELEM_MAND_TLV(gsm_bssmap_elem_strings[BE_CAUSE].value, BSSAP_PDU_TYPE_BSSMAP, BE_CAUSE, "");

	ELEM_MAND_TLV(gsm_bssmap_elem_strings[BE_CELL_ID].value, BSSAP_PDU_TYPE_BSSMAP, BE_CELL_ID, "");

	ELEM_OPT_TV(gsm_bssmap_elem_strings[BE_CHOSEN_CHAN].value, BSSAP_PDU_TYPE_BSSMAP, BE_CHOSEN_CHAN, "");

	ELEM_OPT_TV(gsm_bssmap_elem_strings[BE_CHOSEN_ENC_ALG].value, BSSAP_PDU_TYPE_BSSMAP, BE_CHOSEN_ENC_ALG, "");

	ELEM_OPT_TV(gsm_bssmap_elem_strings[BE_SPEECH_VER].value, BSSAP_PDU_TYPE_BSSMAP, BE_SPEECH_VER, " (Chosen)");

	ELEM_OPT_TLV(gsm_bssmap_elem_strings[BE_LSA_ID].value, BSSAP_PDU_TYPE_BSSMAP, BE_LSA_ID, "");

	EXTRANEOUS_DATA_CHECK(curr_len, 0);
}

/*
 *  [2] 3.2.1.26
 */
static void
bssmap_overload(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len)
{
	guint32	curr_offset;
	guint32	consumed;
	guint	curr_len;

	curr_offset = offset;
	curr_len = len;

	ELEM_MAND_TLV(gsm_bssmap_elem_strings[BE_CAUSE].value, BSSAP_PDU_TYPE_BSSMAP, BE_CAUSE, "");

	ELEM_OPT_TLV(gsm_bssmap_elem_strings[BE_CELL_ID].value, BSSAP_PDU_TYPE_BSSMAP, BE_CELL_ID, "");

	EXTRANEOUS_DATA_CHECK(curr_len, 0);
}

/*
 *  [2] 3.2.1.29
 */
static void
bssmap_cm_upd(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len)
{
	guint32	curr_offset;
	guint32	consumed;
	guint	curr_len;

	curr_offset = offset;
	curr_len = len;

	ELEM_MAND_TLV(gsm_bssmap_elem_strings[BE_CM_INFO_2].value, BSSAP_PDU_TYPE_BSSMAP, BE_CM_INFO_2, "");

	ELEM_OPT_TLV(gsm_bssmap_elem_strings[BE_CM_INFO_3].value, BSSAP_PDU_TYPE_BSSMAP, BE_CM_INFO_3, "");

	EXTRANEOUS_DATA_CHECK(curr_len, 0);
}

/*
 *  [2] 3.2.1.30
 */
static void
bssmap_ciph_mode_cmd(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len)
{
	guint32	curr_offset;
	guint32	consumed;
	guint	curr_len;

	curr_offset = offset;
	curr_len = len;

	ELEM_OPT_TLV(gsm_bssmap_elem_strings[BE_L3_HEADER_INFO].value, BSSAP_PDU_TYPE_BSSMAP, BE_L3_HEADER_INFO, "");

	ELEM_MAND_TLV(gsm_bssmap_elem_strings[BE_ENC_INFO].value, BSSAP_PDU_TYPE_BSSMAP, BE_ENC_INFO, "");

	ELEM_OPT_TV(gsm_bssmap_elem_strings[BE_CIPH_RESP_MODE].value, BSSAP_PDU_TYPE_BSSMAP, BE_CIPH_RESP_MODE, "");

	EXTRANEOUS_DATA_CHECK(curr_len, 0);
}

/*
 *  [2] 3.2.1.31
 */
static void
bssmap_ciph_mode_complete(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len)
{
	guint32	curr_offset;
	guint32	consumed;
	guint	curr_len;

	curr_offset = offset;
	curr_len = len;

	ELEM_OPT_TLV(gsm_bssmap_elem_strings[BE_L3_MSG].value, BSSAP_PDU_TYPE_BSSMAP, BE_L3_MSG, "");

	ELEM_OPT_TV(gsm_bssmap_elem_strings[BE_CHOSEN_ENC_ALG].value, BSSAP_PDU_TYPE_BSSMAP, BE_CHOSEN_ENC_ALG, "");

	EXTRANEOUS_DATA_CHECK(curr_len, 0);
}

/*
 * [2] 3.2.1.32
 */
static void
bssmap_cl3_info(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len)
{
	guint8	consumed;
	guint32	curr_offset;
	guint	curr_len;

	curr_offset = offset;
	curr_len = len;

	ELEM_MAND_TLV(gsm_bssmap_elem_strings[BE_CELL_ID].value, BSSAP_PDU_TYPE_BSSMAP, BE_CELL_ID, "");

	ELEM_MAND_TLV(gsm_bssmap_elem_strings[BE_L3_INFO].value, BSSAP_PDU_TYPE_BSSMAP, BE_L3_INFO, "");

	ELEM_OPT_TV(gsm_bssmap_elem_strings[BE_CHOSEN_CHAN].value, BSSAP_PDU_TYPE_BSSMAP, BE_CHOSEN_CHAN, "");

	ELEM_OPT_TLV(gsm_bssmap_elem_strings[BE_LSA_ID_LIST].value, BSSAP_PDU_TYPE_BSSMAP, BE_LSA_ID_LIST, "");

	ELEM_OPT_TLV(gsm_bssmap_elem_strings[BE_APDU].value, BSSAP_PDU_TYPE_BSSMAP, BE_APDU, "");

	EXTRANEOUS_DATA_CHECK(curr_len, 0);
}

/*
 * [2] 3.2.1.34
 */
static void
bssmap_sapi_rej(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len)
{
	guint8	consumed;
	guint32	curr_offset;
	guint	curr_len;

	curr_offset = offset;
	curr_len = len;

	ELEM_MAND_TV(gsm_bssmap_elem_strings[BE_DLCI].value, BSSAP_PDU_TYPE_BSSMAP, BE_DLCI, "");

	ELEM_MAND_TLV(gsm_bssmap_elem_strings[BE_CAUSE].value, BSSAP_PDU_TYPE_BSSMAP, BE_CAUSE, "");

	EXTRANEOUS_DATA_CHECK(curr_len, 0);
}

/*
 *  [2] 3.2.1.37
 */
static void
bssmap_ho_reqd_rej(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len)
{
	guint32	curr_offset;
	guint32	consumed;
	guint	curr_len;

	curr_offset = offset;
	curr_len = len;

	ELEM_MAND_TLV(gsm_bssmap_elem_strings[BE_CAUSE].value, BSSAP_PDU_TYPE_BSSMAP, BE_CAUSE, "");

	EXTRANEOUS_DATA_CHECK(curr_len, 0);
}

/*
 *  [2] 3.2.1.38
 */
static void
bssmap_reset_cct(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len)
{
	guint32	curr_offset;
	guint32	consumed;
	guint	curr_len;

	curr_offset = offset;
	curr_len = len;

	ELEM_MAND_TV(gsm_bssmap_elem_strings[BE_CIC].value, BSSAP_PDU_TYPE_BSSMAP, BE_CIC, "");

	ELEM_MAND_TLV(gsm_bssmap_elem_strings[BE_CAUSE].value, BSSAP_PDU_TYPE_BSSMAP, BE_CAUSE, "");

	EXTRANEOUS_DATA_CHECK(curr_len, 0);
}

/*
 *  [2] 3.2.1.39
 */
static void
bssmap_reset_cct_ack(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len)
{
	guint32	curr_offset;
	guint32	consumed;
	guint	curr_len;

	curr_offset = offset;
	curr_len = len;

	ELEM_MAND_TV(gsm_bssmap_elem_strings[BE_CIC].value, BSSAP_PDU_TYPE_BSSMAP, BE_CIC, "");

	EXTRANEOUS_DATA_CHECK(curr_len, 0);
}

/*
 *  [2] 3.2.1.41
 */
static void
bssmap_cct_group_block(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len)
{
	guint32	curr_offset;
	guint32	consumed;
	guint	curr_len;

	curr_offset = offset;
	curr_len = len;

	ELEM_MAND_TLV(gsm_bssmap_elem_strings[BE_CAUSE].value, BSSAP_PDU_TYPE_BSSMAP, BE_CAUSE, "");

	ELEM_MAND_TV(gsm_bssmap_elem_strings[BE_CIC].value, BSSAP_PDU_TYPE_BSSMAP, BE_CIC, "");

	ELEM_MAND_TV(gsm_bssmap_elem_strings[BE_CIC_LIST].value, BSSAP_PDU_TYPE_BSSMAP, BE_CIC_LIST, "");

	EXTRANEOUS_DATA_CHECK(curr_len, 0);
}

/*
 *  [2] 3.2.1.42
 */
static void
bssmap_cct_group_block_ack(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len)
{
	guint32	curr_offset;
	guint32	consumed;
	guint	curr_len;

	curr_offset = offset;
	curr_len = len;

	ELEM_MAND_TV(gsm_bssmap_elem_strings[BE_CIC].value, BSSAP_PDU_TYPE_BSSMAP, BE_CIC, "");

	ELEM_MAND_TV(gsm_bssmap_elem_strings[BE_CIC_LIST].value, BSSAP_PDU_TYPE_BSSMAP, BE_CIC_LIST, "");

	EXTRANEOUS_DATA_CHECK(curr_len, 0);
}

/*
 *  [2] 3.2.1.43
 */
static void
bssmap_cct_group_unblock(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len)
{
	guint32	curr_offset;
	guint32	consumed;
	guint	curr_len;

	curr_offset = offset;
	curr_len = len;

	ELEM_MAND_TV(gsm_bssmap_elem_strings[BE_CIC].value, BSSAP_PDU_TYPE_BSSMAP, BE_CIC, "");

	ELEM_MAND_TV(gsm_bssmap_elem_strings[BE_CIC_LIST].value, BSSAP_PDU_TYPE_BSSMAP, BE_CIC_LIST, "");

	EXTRANEOUS_DATA_CHECK(curr_len, 0);
}

/*
 *  [2] 3.2.1.44
 */
static void
bssmap_cct_group_unblock_ack(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len)
{
	guint32	curr_offset;
	guint32	consumed;
	guint	curr_len;

	curr_offset = offset;
	curr_len = len;

	ELEM_MAND_TV(gsm_bssmap_elem_strings[BE_CIC].value, BSSAP_PDU_TYPE_BSSMAP, BE_CIC, "");

	ELEM_MAND_TV(gsm_bssmap_elem_strings[BE_CIC_LIST].value, BSSAP_PDU_TYPE_BSSMAP, BE_CIC_LIST, "");

	EXTRANEOUS_DATA_CHECK(curr_len, 0);
}

/*
 *  [2] 3.2.1.45
 */
static void
bssmap_confusion(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len)
{
	guint32	curr_offset;
	guint32	consumed;
	guint	curr_len;

	curr_offset = offset;
	curr_len = len;

	ELEM_MAND_TLV(gsm_bssmap_elem_strings[BE_CAUSE].value, BSSAP_PDU_TYPE_BSSMAP, BE_CAUSE, "");

	ELEM_MAND_TLV(gsm_bssmap_elem_strings[BE_DIAG].value, BSSAP_PDU_TYPE_BSSMAP, BE_DIAG, "");

	EXTRANEOUS_DATA_CHECK(curr_len, 0);
}

/*
 *  [2] 3.2.1.47
 */
static void
bssmap_unequipped_cct(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len)
{
	guint32	curr_offset;
	guint32	consumed;
	guint	curr_len;

	curr_offset = offset;
	curr_len = len;

	ELEM_MAND_TV(gsm_bssmap_elem_strings[BE_CIC].value, BSSAP_PDU_TYPE_BSSMAP, BE_CIC, "");

	ELEM_OPT_TV(gsm_bssmap_elem_strings[BE_CIC_LIST].value, BSSAP_PDU_TYPE_BSSMAP, BE_CIC_LIST, "");

	EXTRANEOUS_DATA_CHECK(curr_len, 0);
}

/*
 *  [2] 3.2.1.48
 */
static void
bssmap_ciph_mode_rej(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len)
{
	guint32	curr_offset;
	guint32	consumed;
	guint	curr_len;

	curr_offset = offset;
	curr_len = len;

	ELEM_MAND_TLV(gsm_bssmap_elem_strings[BE_CAUSE].value, BSSAP_PDU_TYPE_BSSMAP, BE_CAUSE, "");

	EXTRANEOUS_DATA_CHECK(curr_len, 0);
}

/*
 *  [2] 3.2.1.49
 */
static void
bssmap_load_ind(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len)
{
	guint32	curr_offset;
	guint32	consumed;
	guint	curr_len;

	curr_offset = offset;
	curr_len = len;

	ELEM_MAND_TV(gsm_bssmap_elem_strings[BE_TIME_IND].value, BSSAP_PDU_TYPE_BSSMAP, BE_TIME_IND, "");

	ELEM_MAND_TLV(gsm_bssmap_elem_strings[BE_CELL_ID].value, BSSAP_PDU_TYPE_BSSMAP, BE_CELL_ID, "");

	ELEM_MAND_TLV(gsm_bssmap_elem_strings[BE_CELL_ID_LIST].value, BSSAP_PDU_TYPE_BSSMAP, BE_CELL_ID_LIST, " (Target)");

	ELEM_OPT_TLV(gsm_bssmap_elem_strings[BE_RES_SIT].value, BSSAP_PDU_TYPE_BSSMAP, BE_RES_SIT, "");

	ELEM_OPT_TLV(gsm_bssmap_elem_strings[BE_CAUSE].value, BSSAP_PDU_TYPE_BSSMAP, BE_CAUSE, "");

	EXTRANEOUS_DATA_CHECK(curr_len, 0);
}

/*
 *  [2] 3.2.1.66
 */
static void
bssmap_change_cct(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len)
{
	guint32	curr_offset;
	guint32	consumed;
	guint	curr_len;

	curr_offset = offset;
	curr_len = len;

	ELEM_MAND_TLV(gsm_bssmap_elem_strings[BE_CAUSE].value, BSSAP_PDU_TYPE_BSSMAP, BE_CAUSE, "");

	EXTRANEOUS_DATA_CHECK(curr_len, 0);
}

/*
 *  [2] 3.2.1.67
 */
static void
bssmap_change_cct_ack(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len)
{
	guint32	curr_offset;
	guint32	consumed;
	guint	curr_len;

	curr_offset = offset;
	curr_len = len;

	ELEM_MAND_TV(gsm_bssmap_elem_strings[BE_CIC].value, BSSAP_PDU_TYPE_BSSMAP, BE_CIC, "");

	EXTRANEOUS_DATA_CHECK(curr_len, 0);
}

/*
 *  [2] 3.2.1.68
 */
static void
bssmap_common_id(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len)
{
	guint32	curr_offset;
	guint32	consumed;
	guint	curr_len;

	curr_offset = offset;
	curr_len = len;

	ELEM_MAND_TLV(gsm_bssmap_elem_strings[BE_IMSI].value, BSSAP_PDU_TYPE_BSSMAP, BE_IMSI, "");

	EXTRANEOUS_DATA_CHECK(curr_len, 0);
}

/*
 *  [2] 3.2.1.69
 */
static void
bssmap_lsa_info(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len)
{
	guint32	curr_offset;
	guint32	consumed;
	guint	curr_len;

	curr_offset = offset;
	curr_len = len;

	ELEM_MAND_TLV(gsm_bssmap_elem_strings[BE_LSA_INFO].value, BSSAP_PDU_TYPE_BSSMAP, BE_LSA_INFO, "");

	EXTRANEOUS_DATA_CHECK(curr_len, 0);
}

/*
 *  [2] 3.2.1.70
 */
static void
bssmap_conn_oriented(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len)
{
	guint32	curr_offset;
	guint32	consumed;
	guint	curr_len;

	curr_offset = offset;
	curr_len = len;

	ELEM_MAND_TLV(gsm_bssmap_elem_strings[BE_APDU].value, BSSAP_PDU_TYPE_BSSMAP, BE_APDU, "");

	ELEM_OPT_TLV(gsm_bssmap_elem_strings[BE_SEG].value, BSSAP_PDU_TYPE_BSSMAP, BE_SEG, "");

	EXTRANEOUS_DATA_CHECK(curr_len, 0);
}


#define	NUM_GSM_BSSMAP_MSG (sizeof(gsm_a_bssmap_msg_strings)/sizeof(value_string))
static gint ett_gsm_bssmap_msg[NUM_GSM_BSSMAP_MSG];

static void (*bssmap_msg_fcn[])(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len) = {
	bssmap_ass_req,	/* Assignment Request */
	bssmap_ass_complete,	/* Assignment Complete */
	bssmap_ass_failure,	/* Assignment Failure */
	bssmap_ho_req,	/* Handover Request */
	bssmap_ho_reqd,	/* Handover Required */
	bssmap_ho_req_ack,	/* Handover Request Acknowledge */
	bssmap_ho_cmd,	/* Handover Command */
	bssmap_ho_complete,	/* Handover Complete */
	NULL /* no associated data */,	/* Handover Succeeded */
	bssmap_ho_failure,	/* Handover Failure */
	bssmap_ho_performed,	/* Handover Performed */
	bssmap_ho_cand_enq,	/* Handover Candidate Enquire */
	bssmap_ho_cand_resp,	/* Handover Candidate Response */
	bssmap_ho_reqd_rej,	/* Handover Required Reject */
	NULL /* no associated data */,	/* Handover Detect */
	bssmap_clear_cmd,	/* Clear Command */
	NULL /* no associated data */,	/* Clear Complete */
	bssmap_clear_req,	/* Clear Request */
	NULL,	/* Reserved */
	NULL,	/* Reserved */
	bssmap_sapi_rej,	/* SAPI 'n' Reject */
	bssmap_confusion,	/* Confusion */
	NULL,	/* Suspend */
	NULL,	/* Resume */
	bssmap_conn_oriented,	/* Connection Oriented Information */
	NULL,	/* Perform Location Request */
	bssmap_lsa_info,	/* LSA Information */
	NULL,	/* Perform Location Response */
	NULL,	/* Perform Location Abort */
	bssmap_common_id,	/* Common Id */
	bssmap_reset,	/* Reset */
	NULL /* no associated data */,	/* Reset Acknowledge */
	bssmap_overload,	/* Overload */
	NULL,	/* Reserved */
	bssmap_reset_cct,	/* Reset Circuit */
	bssmap_reset_cct_ack,	/* Reset Circuit Acknowledge */
	NULL,	/* MSC Invoke Trace */
	NULL,	/* BSS Invoke Trace */
	NULL,	/* Connectionless Information */
	bssmap_block,	/* Block */
	bssmap_block_ack,	/* Blocking Acknowledge */
	bssmap_unblock,	/* Unblock */
	bssmap_unblock_ack,	/* Unblocking Acknowledge */
	bssmap_cct_group_block,	/* Circuit Group Block */
	bssmap_cct_group_block_ack,	/* Circuit Group Blocking Acknowledge */
	bssmap_cct_group_unblock,	/* Circuit Group Unblock */
	bssmap_cct_group_unblock_ack,	/* Circuit Group Unblocking Acknowledge */
	bssmap_unequipped_cct,	/* Unequipped Circuit */
	bssmap_change_cct,	/* Change Circuit */
	bssmap_change_cct_ack,	/* Change Circuit Acknowledge */
	NULL,	/* Resource Request */
	NULL,	/* Resource Indication */
	bssmap_paging,	/* Paging */
	bssmap_ciph_mode_cmd,	/* Cipher Mode Command */
	bssmap_cm_upd,	/* Classmark Update */
	bssmap_ciph_mode_complete,	/* Cipher Mode Complete */
	NULL /* no associated data */,	/* Queuing Indication */
	bssmap_cl3_info,	/* Complete Layer 3 Information */
	NULL /* no associated data */,	/* Classmark Request */
	bssmap_ciph_mode_rej,	/* Cipher Mode Reject */
	bssmap_load_ind,	/* Load Indication */
	NULL,	/* VGCS/VBS Setup */
	NULL,	/* VGCS/VBS Setup Ack */
	NULL,	/* VGCS/VBS Setup Refuse */
	NULL,	/* VGCS/VBS Assignment Request */
	NULL,	/* VGCS/VBS Assignment Result */
	NULL,	/* VGCS/VBS Assignment Failure */
	NULL,	/* VGCS/VBS Queuing Indication */
	NULL,	/* Uplink Request */
	NULL,	/* Uplink Request Acknowledge */
	NULL,	/* Uplink Request Confirmation */
	NULL,	/* Uplink Release Indication */
	NULL,	/* Uplink Reject Command */
	NULL,	/* Uplink Release Command */
	NULL,	/* Uplink Seized Command */
	NULL,	/* NONE */
};

void
dissect_bssmap(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	static gsm_a_tap_rec_t	tap_rec[4];
	static gsm_a_tap_rec_t	*tap_p;
	static guint			tap_current=0;
	guint8	oct;
	guint32	offset, saved_offset;
	guint32	len;
	gint	idx;
	proto_item	*bssmap_item = NULL;
	proto_tree	*bssmap_tree = NULL;
	const gchar	*str;
	sccp_msg_info_t* sccp_msg;

	sccp_msg = pinfo->sccp_info;

	if (!(sccp_msg && sccp_msg->data.co.assoc)) {
		sccp_msg = NULL;
	}

	if (check_col(pinfo->cinfo, COL_INFO))
	{
	col_append_str(pinfo->cinfo, COL_INFO, "(BSSMAP) ");
	}

	/*
	 * set tap record pointer
	 */
	tap_current++;
	if (tap_current >= 4)
	{
	tap_current = 0;
	}
	tap_p = &tap_rec[tap_current];


	offset = 0;
	saved_offset = offset;

	g_pinfo = pinfo;
	g_tree = tree;

	len = tvb_length(tvb);

	/*
	 * add BSSMAP message name
	 */
	oct = tvb_get_guint8(tvb, offset++);

	str = match_strval_idx((guint32) oct, gsm_a_bssmap_msg_strings, &idx);

	if (sccp_msg && !sccp_msg->data.co.label) {
		sccp_msg->data.co.label = se_strdup(val_to_str((guint32) oct, gsm_a_bssmap_msg_strings, "BSSMAP (0x%02x)"));
	}

	/*
	 * create the protocol tree
	 */
	if (str == NULL)
	{
	bssmap_item =
	    proto_tree_add_protocol_format(tree, proto_a_bssmap, tvb, 0, len,
		"GSM A-I/F BSSMAP - Unknown BSSMAP Message Type (0x%02x)",
		oct);

	bssmap_tree = proto_item_add_subtree(bssmap_item, ett_bssmap_msg);
	}
	else
	{
	bssmap_item =
	    proto_tree_add_protocol_format(tree, proto_a_bssmap, tvb, 0, -1,
		"GSM A-I/F BSSMAP - %s",
		str);

	bssmap_tree = proto_item_add_subtree(bssmap_item, ett_gsm_bssmap_msg[idx]);

	if (check_col(pinfo->cinfo, COL_INFO))
	{
	    col_append_fstr(pinfo->cinfo, COL_INFO, "%s ", str);
	}

	/*
	 * add BSSMAP message name
	 */
	proto_tree_add_uint_format(bssmap_tree, hf_gsm_a_bssmap_msg_type,
	tvb, saved_offset, 1, oct, "Message Type %s",str);
	}

	tap_p->pdu_type = BSSAP_PDU_TYPE_BSSMAP;
	tap_p->message_type = oct;

	tap_queue_packet(gsm_a_tap, pinfo, tap_p);

	if (str == NULL) return;

	if ((len - offset) <= 0) return;

	/*
	 * decode elements
	 */
	if (bssmap_msg_fcn[idx] == NULL)
	{
	proto_tree_add_text(bssmap_tree,
	    tvb, offset, len - offset,
	    "Message Elements");
	}
	else
	{
	(*bssmap_msg_fcn[idx])(tvb, bssmap_tree, offset, len - offset);
	}
}

/* Register the protocol with Wireshark */
void
proto_register_gsm_a_bssmap(void)
{
	guint		i;
	guint		last_offset;

	/* Setup list of header fields */

	static hf_register_info hf[] =
	{
	{ &hf_gsm_a_bssmap_msg_type,
	    { "BSSMAP Message Type",	"gsm_a.bssmap_msgtype",
	    FT_UINT8, BASE_HEX, VALS(gsm_a_bssmap_msg_strings), 0x0,
	    "", HFILL }
	},
	{ &hf_gsm_a_bssmap_elem_id,
	    { "Element ID",	"gsm_a_bssmap.elem_id",
	    FT_UINT8, BASE_DEC, NULL, 0,
	    "", HFILL }
	},
	{ &hf_gsm_a_length,
	    { "Length",		"gsm_a.len",
	    FT_UINT16, BASE_DEC, NULL, 0,
	    "", HFILL }
	},
	{ &hf_gsm_a_cell_ci,
	    { "Cell CI",	"gsm_a.cell_ci",
	    FT_UINT16, BASE_HEX_DEC, 0, 0x0,
	    "", HFILL }
	},
	{ &hf_gsm_a_cell_lac,
	    { "Cell LAC",	"gsm_a.cell_lac",
	    FT_UINT16, BASE_HEX_DEC, 0, 0x0,
	    "", HFILL }
	},
	{ &hf_gsm_a_dlci_cc,
	    { "Control Channel", "bssap.dlci.cc",
	    FT_UINT8, BASE_HEX, VALS(bssap_cc_values), 0xc0,
	    "", HFILL}
	},
	{ &hf_gsm_a_dlci_spare,
	    { "Spare", "bssap.dlci.spare",
	    FT_UINT8, BASE_HEX, NULL, 0x38,
	    "", HFILL}
	},
	{ &hf_gsm_a_dlci_sapi,
	    { "SAPI", "bssap.dlci.sapi",
	    FT_UINT8, BASE_HEX, VALS(bssap_sapi_values), 0x07,
	    "", HFILL}
	},
	{ &hf_gsm_a_bssmap_cause,
	    { "BSSMAP Cause",	"gsm_a_bssmap.cause",
	    FT_UINT8, BASE_HEX, 0, 0x0,
	    "", HFILL }
	},
	{ &hf_gsm_a_be_cell_id_disc,
		{ "Cell identification discriminator","gsm_a.be.cell_id_disc",
		FT_UINT8,BASE_DEC,  VALS(gsm_a_be_cell_id_disc_vals), 0x0f,
		"Cell identification discriminator", HFILL }
	},
	{ &hf_gsm_a_be_rnc_id,
		{ "RNC-ID","gsm_a.be.rnc_id",
		FT_UINT16,BASE_DEC,  NULL, 0x0,
		"RNC-ID", HFILL }
	},
	{ &hf_gsm_a_apdu_protocol_id,
		{ "Protocol ID", "gsm_a.apdu_protocol_id",
		FT_UINT8, BASE_DEC, VALS(gsm_a_apdu_protocol_id_strings), 0x0,
		"APDU embedded protocol id", HFILL }
	},
	};

	/* Setup protocol subtree array */
#define	NUM_INDIVIDUAL_ELEMS	3
	static gint *ett[NUM_INDIVIDUAL_ELEMS + NUM_GSM_BSSMAP_MSG +
			NUM_GSM_BSSMAP_ELEM];

	ett[0] = &ett_bssmap_msg;
	ett[1] = &ett_cell_list;
	ett[2] = &ett_dlci;

	last_offset = NUM_INDIVIDUAL_ELEMS;

	for (i=0; i < NUM_GSM_BSSMAP_MSG; i++, last_offset++)
	{
	ett_gsm_bssmap_msg[i] = -1;
	ett[last_offset] = &ett_gsm_bssmap_msg[i];
	}

	for (i=0; i < NUM_GSM_BSSMAP_ELEM; i++, last_offset++)
	{
	ett_gsm_bssmap_elem[i] = -1;
	ett[last_offset] = &ett_gsm_bssmap_elem[i];
	}

	/* Register the protocol name and description */

	proto_a_bssmap =
	proto_register_protocol("GSM A-I/F BSSMAP", "GSM BSSMAP", "gsm_a_bssmap");

	proto_register_field_array(proto_a_bssmap, hf, array_length(hf));

	proto_register_subtree_array(ett, array_length(ett));

	register_dissector("gsm_a_bssmap", dissect_bssmap, proto_a_bssmap);
}


void
proto_reg_handoff_gsm_a_bssmap(void)
{

	bssmap_handle = create_dissector_handle(dissect_bssmap, proto_a_bssmap);
	dtap_handle = find_dissector("gsm_a_dtap");

	dissector_add("bssap.pdu_type",  BSSAP_PDU_TYPE_BSSMAP, bssmap_handle);
	data_handle = find_dissector("data");
	gsm_bsslap_handle = find_dissector("gsm_bsslap");
}

