/* packet-gsm_map.c
 * Routines for GSM Mobile Application Part dissection
 *
 * Copyright 2000, Felix Fei <felix.fei [AT] utstar.com>
 *
 * Michael Lum <mlum [AT] telostech.com>,
 * Changed to run on new version of TCAP, many changes for
 * EOC matching, and parameter separation.  (2003)
 *
 * $Id: packet-gsm_map.c,v 1.6 2004/02/20 10:49:12 guy Exp $
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@ethereal.com>
 * Copyright 1998 Gerald Combs
 *
 * Copied from packet-tcap.c (where "WHATEVER_FILE_YOU_USED"
 * is a dissector file; if you just copied this from README.developer,
 * don't bother with the "Copied from" - you don't even need to put
 * in a "Copied from" if you copied an existing dissector, especially
 * if the bulk of the code in the new dissector is your code)
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

#include "epan/packet.h"
#include "tap.h"
#include "asn1.h"

#include "packet-gsm_map.h"


/* OPERATION CODE DEFINITION */

/* LOCATION MANAGEMENT */
#define MAP_UPD_LOC			2	/* Update Location */
#define MAP_CANCEL_LOC			3	/* Cancel Location */
#define MAP_PURGE			67	/* Purge MS */
#define MAP_SEND_ID			5	/* Send Identification */
#define MAP_GPRS_UPD_LOC		23	/* GPRS Update Location */
#define MAP_DET_IMSI			5	/* Detach IMSI */
#define MAP_NOTE_MM_EVT			89	/* Note MM Event */

/* HANDOVER MANAGEMENT */
#define MAP_PREP_HO			68	/* Prepare Handover */
#define MAP_PREP_SUBS_HO		69	/* Prepare Subsequent Handover */
#define MAP_PERF_HO			28	/* Perform Handover */
#define MAP_PERF_SUBS_HO		30	/* Perform Subsequent Handover */
#define MAP_SEND_END_SIG		29	/* Send End Signal */
#define MAP_PROC_ACC_SIG		33	/* Process Access Signalling */
#define MAP_FWD_ACC_SIG			34	/* Forward Access Signalling */

/* AUTHENTICATION MANAGEMENT */
#define MAP_AUTH_INFO			56	/* Send Authintication Info */
#define MAP_AUTH_FAIL_RPT		15	/* Authentication Failure Report */

/*  IDENTIFICATION MANAGEMENT */
#define MAP_CHK_IMEI			43	/* Check IMEI */

/* FAULT & RECOVERY MANAGEMENT */
#define MAP_RESET			37	/* Reset */
#define MAP_RESTORE_DATA		57	/* Restore Data */
#define MAP_FWD_CHK_SS_IND		38	/* Forward Check SS Indication */

/* OAM MANAGEMENT */
#define MAP_ACT_TRACE			50	/* Activate Trace */
#define MAP_DEACT_TRACE			51	/* Deactivate Trace Mode */
#define MAP_SEND_IMSI			58	/* Send IMSI */
#define MAP_TRACE_SUBS_ACTV		52	/* Trace Subscriber Activity */
#define MAP_NOTE_INTER_HO		35	/* Not Internal Handover */

/* CALL MANAGEMENT */
#define MAP_ROUTE_INFO			22	/* Send Routing Info */
#define MAP_PROV_ROAM_NUM		4	/* Provide Roaming Number */
#define MAP_PROV_SIWFS_NUM		31	/* Provide SIWFS Number */
#define MAP_SIWFS_SIG_MOD		32	/* SIWFS Signalling Modify */
#define MAP_RES_CALL_HAND		6	/* Resume Call Handling */
#define MAP_SET_RPT_STATE		73	/* Set Reporting State */
#define MAP_STAT_RPT			74	/* Status Report */
#define MAP_REM_USR_FREE		75	/* Remote user free */
#define MAP_PREP_GRP_CALL		39	/* Prepare Group Call */
#define MAP_SND_GRP_CALL_END_SIG	40	/* Send Group Call End Signalling */
#define MAP_PRO_GRP_CALL_SIG		41	/* Process Group Call Signalling  */
#define MAP_FWD_GRP_CALL_SIG		42	/* Forward Group Call Signalling  */
#define MAP_IST_ALERT			87	/* IST Alert */
#define MAP_IST_COMMAND			88	/* IST Command */

/* SS MANAGEMENT */
#define MAP_REG_SS			10	/* Register SS */
#define MAP_ERASE_SS			11	/* Erase SS */
#define MAP_ACT_SS			12	/* Activate SS */
#define MAP_DEACT_SS			13	/* Deactivate SS */
#define MAP_INTER_SS			14	/* Interogate SS */
#define MAP_PROC_U_SS_REQ		59	/* Process Unstructured SS Req */
#define MAP_U_SS_REQ			60	/* Unstructured SS Request */
#define MAP_U_SS_NOTIFY			61	/* Unstructured SS Notify */
#define MAP_REG_PASSWD			17	/* Register Password */
#define MAP_GET_PASSWD			18	/* Get Password */
#define MAP_REG_CC_ENT			76	/* Register CC Entry */
#define MAP_ERASE_CC_ENT		77	/* Erase CC Entry */
#define MAP_BEGIN_SUBS_ACTV		54	/* Begin Subscriber Activity */
#define MAP_PROC_U_SS_DATA		19	/* Process Unstructured SS Data */
#define MAP_SS_INV_NOTIFY		72	/* SS Invocation Notify */

/* SMS MANAGEMENT */
#define MAP_MO_FWD_SM			46	/* Mobile Originated Forward Short Message */
#define MAP_MT_FWD_SM			44	/* Mobile Terminated Forward Short Message */
#define MAP_ROUTE_INFO_SM		45	/* Routing Info for SM */
#define MAP_SM_DEL_STAT			47	/* Report SM Delivery Status */
#define MAP_INFORM_SC			63	/* Inform Service Center */
#define MAP_ALERT_SC			64	/* Alert Service Center */
#define MAP_SM_READY			66	/* Ready for Short Message */
#define MAP_NOTE_SUB_PRES		48	/* Note Subscriber Present */
#define MAP_ALERT_SC_W_RES		49	/* Alert SC Without Result */

/* SUBSCRIBER MANAGEMENT */
#define MAP_INS_SUB_DATA		7	/* Insert Subscriber Data */
#define MAP_DEL_SUB_DATA		8	/* Delete Subscriber Data */
#define MAP_PROV_SUB_INFO		70	/* Provide Subscriber Info */
#define MAP_ANY_TIME_INTER		71	/* Any Time Interrogation */
#define MAP_SEND_PARAM			9	/* Send Parameters */
#define MAP_ANY_TIME_SUB_DATA_INTER	62	/* Any Time Subscriber Info Interrogation */
#define MAP_ANY_TIME_MOD		65	/* Any Time Modification */
#define MAP_NOTE_SUB_DATA_MOD		5	/* Note Subscriber Data Modified */

/* PDP ACTIVE MANAGEMENT */
#define MAP_GPRS_ROUTE_INFO		24	/* Rout Info for GPRS */
#define MAP_FAIL_REP			25	/* Failure Report */
#define MAP_GPRS_NOTE_MS_PRES		26	/* GPRS NoteMs Present */

/* LOCATION SERVICE */
#define MAP_PROV_SUB_LOC		83	/* Provide Subscriber Location */
#define MAP_SEND_ROUTE_INFO_FOR_LCS	85	/* Send Routing Info For LCS */
#define MAP_SUB_LOC_REP			86	/* Subscriber Location Report */


#define MAP_OPR_CODE_TAG	0x02
#define MAP_INVOKE_ID_TAG	0x02
#define MAP_LINK_ID_TAG		0x80
#define MAP_SEQ_TAG		0x30
#define MAP_GE_PROBLEM_TAG	0x80
#define MAP_IN_PROBLEM_TAG	0x81
#define MAP_RR_PROBLEM_TAG	0x82
#define MAP_RE_PROBLEM_TAG	0x83
#define MAP_INVALID_TAG		0x00

#define MAP_OK			0x0
#define MAP_FAIL		0x1

const value_string gsm_map_opr_code_strings[] = {

/* LOCATION MANAGEMENT */
    { MAP_UPD_LOC,			"Update Location"},
    { MAP_CANCEL_LOC,			"Cancel Location"},
    { MAP_PURGE,			"Purge MS"},
    { MAP_SEND_ID,			"Send Identification"},
    { MAP_GPRS_UPD_LOC,			"Update GPRS Location"},
    { MAP_DET_IMSI,			"Detach IMSI"},
    { MAP_NOTE_MM_EVT,			"Note MM Event"},

/* HANDOVER MANAGEMENT */
    { MAP_PREP_HO,			"Prepare Handover"},
    { MAP_PREP_SUBS_HO,			"Prepare Subsequent Handover"},
    { MAP_PERF_HO,			"Perform Handover"},
    { MAP_PERF_SUBS_HO,			"Perform Subsequent Handover"},
    { MAP_SEND_END_SIG,			"Send End Signal"},
    { MAP_PROC_ACC_SIG,			"Process Access Signalling"},
    { MAP_FWD_ACC_SIG,			"Forward Access Signalling"},

/* AUTHENTICATION MANAGEMENT */
    { MAP_AUTH_INFO,			"Send Authentication Info"},
    { MAP_AUTH_FAIL_RPT,		"Authentication Failure Report"},

/* IDENTIFICATION MANAGEMENT */
    { MAP_CHK_IMEI,			"Check IMEI"},

/* FAULT & RECOVERY MANAGEMENT */
    { MAP_RESET,			"Reset"},
    { MAP_RESTORE_DATA,			"Restore Data"},
    { MAP_FWD_CHK_SS_IND,		"Forward Check SS Indication"},

/* OAM MANAGEMENT */
    { MAP_ACT_TRACE,			"Activate Trace Mode"},
    { MAP_DEACT_TRACE,			"Deactivate Trace Mode"},
    { MAP_SEND_IMSI,			"Send IMSI"},
    { MAP_TRACE_SUBS_ACTV,		"Trace Subscriber Activity"},
    { MAP_NOTE_INTER_HO,		"Note Internal Handover"},

/*  CALL MANAGEMENT */
    { MAP_ROUTE_INFO,			"Send Routing Info"},
    { MAP_PROV_ROAM_NUM,		"Provide Roaming Number"},
    { MAP_PROV_SIWFS_NUM,		"Provide SIWFS Number"},
    { MAP_SIWFS_SIG_MOD,		"SIWFS Signalling Modify"},
    { MAP_RES_CALL_HAND,		"Resume Call Handling"},
    { MAP_SET_RPT_STATE,		"Set Reporting State"},
    { MAP_STAT_RPT,			"Status Report"},
    { MAP_REM_USR_FREE,			"Remote User Free"},
    { MAP_PREP_GRP_CALL,		"Prepare Group Call"},
    { MAP_SND_GRP_CALL_END_SIG,		"Send Group Call End Signalling"},
    { MAP_PRO_GRP_CALL_SIG,		"Process Group Call Signalling"},
    { MAP_FWD_GRP_CALL_SIG,		"Forward Group Call Signalling"},
    { MAP_IST_ALERT,			"IST Alert"},
    { MAP_IST_COMMAND,			"IST Command"},

/* SS MANAGEMENT */
    { MAP_REG_SS,			"Register SS"},
    { MAP_ERASE_SS,			"Erase SS"},
    { MAP_ACT_SS,			"Activate SS"},
    { MAP_DEACT_SS,			"Deactivate SS"},
    { MAP_INTER_SS,			"Interogate SS"},
    { MAP_PROC_U_SS_REQ,		"Process Unstructured SS Request"},
    { MAP_U_SS_REQ,			"Unstructured SS Request"},
    { MAP_U_SS_NOTIFY,			"Unstructured SS Notify"},
    { MAP_REG_PASSWD,			"Register Password"},
    { MAP_GET_PASSWD,			"Get Password"},
    { MAP_REG_CC_ENT,			"Register CC Entry"},
    { MAP_ERASE_CC_ENT,			"Erase CC Entry"},
    { MAP_BEGIN_SUBS_ACTV,		"Begin Subscriber Activity"},
    { MAP_PROC_U_SS_DATA,		"Process Unstructured SS Data"},
    { MAP_SS_INV_NOTIFY,		"SS Invocation Notification"},

/* SMS MANAGEMENT */
    { MAP_MO_FWD_SM,			"MO Forward SM"},
    { MAP_MT_FWD_SM,			"MT Forward SM"},
    { MAP_ROUTE_INFO_SM,		"Send Routing Info For SM"},
    { MAP_SM_DEL_STAT,			"Report SM Delivery Status"},
    { MAP_INFORM_SC,			"Inform Service Center"},
    { MAP_ALERT_SC,			"Alert Service Center"},
    { MAP_SM_READY,			"Ready For SM"},
    { MAP_NOTE_SUB_PRES,		"Note Subscriber Present"},
    { MAP_ALERT_SC_W_RES,		"Alert SC Without Result"},

/* SUBSCRIBER MANAGEMENT */
    { MAP_INS_SUB_DATA,			"Insert Subscriber Data"},
    { MAP_DEL_SUB_DATA,			"Delete Subscriber Data"},
    { MAP_PROV_SUB_INFO,		"Provide Subscriber Info"},
    { MAP_ANY_TIME_INTER,		"Any Time Interrogation"},
    { MAP_SEND_PARAM,			"Send Parameters"},
    { MAP_ANY_TIME_SUB_DATA_INTER,	"Any Time Subscription Interrogation"},
    { MAP_ANY_TIME_MOD,			"Any Time Modification"},
    { MAP_NOTE_SUB_DATA_MOD,		"Note Subscriber Data Modified"},

/* PDP ACTIVE MANAGEMENT */
    { MAP_GPRS_ROUTE_INFO,		"Send Routing Info For GPRS"},
    { MAP_FAIL_REP,			"Failure Report"},
    { MAP_GPRS_NOTE_MS_PRES,		"Note MS Present For GPRS"},

/* LOCATION SERVICE */
    { MAP_PROV_SUB_LOC,			"Provide Subscriber Location"},
    { MAP_SEND_ROUTE_INFO_FOR_LCS,	"Send Routing Info For LCS"},
    { MAP_SUB_LOC_REP,			"Subscriber Location Report"},

    { 0,				NULL}
};

/* TCAP component type */
#define MAP_TC_INVOKE		0xa1
#define MAP_TC_RRL		0xa2
#define MAP_TC_RE		0xa3
#define MAP_TC_REJECT		0xa4
#define MAP_TC_RRN		0xa7

static const value_string tag_strings[] = {
    { MAP_TC_INVOKE,		"Invoke" },
    { MAP_TC_RRL,		"RetRes(Last)" },
    { MAP_TC_RE,		"RetErr" },
    { MAP_TC_REJECT,		"Reject" },
    { MAP_TC_RRN,		"RetRes(Not Last)" },
    { 0,			NULL }
};

#define MAP_EOC_LEN	2 /* 0x00 0x00 */

/* Initialize the protocol and registered fields */
static int proto_map = -1;

static int gsm_map_tap = -1;

static dissector_table_t sms_dissector_table;	/* SMS TPDU */

static packet_info *g_pinfo;
static proto_tree *g_tree;

static int hf_map_tag = -1;
static int hf_map_length = -1;
static int hf_map_opr_code = -1;
static int hf_map_int = -1;

/* Initialize the subtree pointers */
static gint ett_map = -1;
static gint ett_component = -1;
static gint ett_components = -1;
static gint ett_param = -1;
static gint ett_params = -1;
static gint ett_problem = -1;
static gint ett_opr_code = -1;

typedef struct dgt_set_t
{
    unsigned char out[15];
}
dgt_set_t;

#ifdef MLUM
static dgt_set_t Dgt_tbcd = {
    {
  /*  0   1   2   3   4   5   6   7   8   9   a   b   c   d   e */
     '0','1','2','3','4','5','6','7','8','9','?','B','C','*','#'
    }
};
#endif

static dgt_set_t Dgt_msid = {
    {
  /*  0   1   2   3   4   5   6   7   8   9   a   b   c   d   e */
     '0','1','2','3','4','5','6','7','8','9','?','?','?','?','?'
    }
};


/* FORWARD DECLARATIONS */

static int dissect_map_eoc(ASN1_SCK *asn1, proto_tree *tree);

/* FUNCTIONS */

static int
find_eoc(ASN1_SCK *asn1)
{
    guint	saved_offset;
    guint	tag;
    guint	len;
    gboolean	def_len;

    saved_offset = asn1->offset;

    while (!asn1_eoc(asn1, -1))
    {
	asn1_id_decode1(asn1, &tag);
	asn1_length_decode(asn1, &def_len, &len);

	if (def_len)
	{
	    asn1->offset += len;
	}
	else
	{
	    asn1->offset += find_eoc(asn1);
	    asn1_eoc_decode(asn1, -1);
	}
    }

    len = asn1->offset - saved_offset;
    asn1->offset = saved_offset;

    return(len);
}

/*
 * Unpack BCD input pattern into output ASCII pattern
 *
 * Input Pattern is supplied using the same format as the digits
 *
 * Returns: length of unpacked pattern
 */
static int
my_dgt_tbcd_unpack(
    char	*out,		/* ASCII pattern out */
    guchar	*in,		/* packed pattern in */
    int		num_octs,	/* Number of octets to unpack */
    dgt_set_t	*dgt		/* Digit definitions */
    )
{
    int cnt = 0;
    unsigned char i;

    while (num_octs)
    {
	/*
	 * unpack first value in byte
	 */
	i = *in++;
	*out++ = dgt->out[i & 0x0f];
	cnt++;

	/*
	 * unpack second value in byte
	 */
	i >>= 4;

	if (i == 0x0f)	/* odd number bytes - hit filler */
	    break;

	*out++ = dgt->out[i];
	cnt++;
	num_octs--;
    }

    *out = '\0';

    return(cnt);
}

static gchar *
my_match_strval(guint32 val, const value_string *vs, gint *idx)
{
    gint	i = 0;

    while (vs[i].strptr) {
	if (vs[i].value == val)
	{
	    *idx = i;
	    return(vs[i].strptr);
	}

	i++;
    }

    *idx = -1;
    return(NULL);
}

static gboolean
check_map_tag(ASN1_SCK *asn1, guint tag)
{
    guint	saved_offset, real_tag;

    if (tvb_length_remaining(asn1->tvb, asn1->offset) <= 0)
    {
	return(FALSE);
    }

    saved_offset = asn1->offset;
    asn1_id_decode1(asn1, &real_tag);
    asn1->offset = saved_offset;

    return(tag == real_tag);
}

static int
dissect_map_params(ASN1_SCK *asn1, proto_tree *tree, guint exp_len)
{
    guint	orig_offset, saved_offset, len_offset;
    guint	tag, len;
    gboolean	def_len = FALSE;
    proto_item	*item;
    proto_tree	*subtree;

    orig_offset = asn1->offset;

    while ((tvb_length_remaining(asn1->tvb, asn1->offset) > 0) &&
	(!check_map_tag(asn1, 0)))
    {
	if ((exp_len != 0) &&
	    ((asn1->offset - orig_offset) >= exp_len))
	{
	    break;
	}

	saved_offset = asn1->offset;
	asn1_id_decode1(asn1, &tag);
	len_offset = asn1->offset;
	asn1_length_decode(asn1, &def_len, &len);

	if (tag == MAP_SEQ_TAG)
	{
	    item =
		proto_tree_add_text(tree, asn1->tvb,
		    saved_offset, -1, "Sequence");

	    subtree = proto_item_add_subtree(item, ett_params);

	    proto_tree_add_uint_format(subtree, hf_map_tag, asn1->tvb,
		saved_offset, len_offset - saved_offset, tag, "Sequence Tag");

	    if (def_len)
	    {
		proto_tree_add_uint(subtree, hf_map_length, asn1->tvb,
		    len_offset, asn1->offset - len_offset, len);
	    }
	    else
	    {
		proto_tree_add_text(subtree, asn1->tvb,
		    len_offset, asn1->offset - len_offset, "Length: Indefinite");

		len = find_eoc(asn1);
	    }

	    proto_item_set_len(item, (asn1->offset - saved_offset) + len +
		(def_len ? 0 : MAP_EOC_LEN));

	    dissect_map_params(asn1, subtree, len);

	    if (!def_len)
	    {
		dissect_map_eoc(asn1, subtree);
	    }
	    continue;
	}

	if (!def_len)
	{
	    proto_tree_add_uint_format(tree, hf_map_tag, asn1->tvb,
		saved_offset, len_offset - saved_offset, tag, "Parameter Tag");

	    proto_tree_add_text(tree, asn1->tvb,
		len_offset, asn1->offset - len_offset, "Length: Indefinite");

	    len = find_eoc(asn1);

	    dissect_map_params(asn1, tree, len);

	    dissect_map_eoc(asn1, tree);
	    continue;
	}

	item =
	    proto_tree_add_text(tree, asn1->tvb,
		saved_offset, (asn1->offset - saved_offset) + len, "Parameter");

	subtree = proto_item_add_subtree(item, ett_param);

	proto_tree_add_uint_format(subtree, hf_map_tag, asn1->tvb,
	    saved_offset, len_offset - saved_offset, tag, "Parameter Tag");

	proto_tree_add_uint(subtree, hf_map_length, asn1->tvb,
	    len_offset, asn1->offset - len_offset, len);

	proto_tree_add_text(subtree, asn1->tvb,
	    asn1->offset, len, "Parameter Data");

	asn1->offset += len;
    }

    return(MAP_OK);
}


/* PARAMETERS */

static void
param_imsi(ASN1_SCK *asn1, proto_tree *tree, guint len)
{
    guint	saved_offset;
    guchar	*poctets;
    char	bigbuf[1024];

    saved_offset = asn1->offset;
    asn1_string_value_decode(asn1, len, &poctets);

    my_dgt_tbcd_unpack(bigbuf, poctets, len, &Dgt_msid);

    proto_tree_add_text(tree, asn1->tvb,
	saved_offset, len, "IMSI %s", bigbuf);
}

static void
param_lmsi(ASN1_SCK *asn1, proto_tree *tree, guint len)
{
    guint	saved_offset;
    gint32	value;

    saved_offset = asn1->offset;
    asn1_int32_value_decode(asn1, len, &value);

    proto_tree_add_text(tree, asn1->tvb,
	saved_offset, len, "LMSI 0x%04x", value);
}

static void
param_boolean(ASN1_SCK *asn1, proto_tree *tree, guint len)
{
    guint	saved_offset;
    gint32	value;

    saved_offset = asn1->offset;
    asn1_int32_value_decode(asn1, len, &value);

    proto_tree_add_text(tree, asn1->tvb,
	saved_offset, len, "%s (0x%x)", value ? "TRUE" : "FALSE", value);
}

static void
param_AddressString(ASN1_SCK *asn1, proto_tree *tree, guint len)
{
    guint	saved_offset;
    gint32	value;
    guchar	*poctets;
    gchar	*str = NULL;
    char	bigbuf[1024];

    saved_offset = asn1->offset;
    asn1_int32_value_decode(asn1, 1, &value);

    other_decode_bitfield_value(bigbuf, value, 0x80, 8);
    proto_tree_add_text(tree, asn1->tvb,
	saved_offset, 1,
	"%s :  %sxtension",
	bigbuf, (value & 0x80) ? "No E" : "E");

    switch ((value & 0x70) >> 4)
    {
    case 0x00: str = "unknown"; break;
    case 0x01: str = "International Number"; break;
    case 0x02: str = "National Significant Number"; break;
    case 0x03: str = "Network Specific Number"; break;
    case 0x04: str = "Subscriber Number"; break;
    case 0x05: str = "Reserved"; break;
    case 0x06: str = "Abbreviated Number"; break;
    case 0x07: str = "Reserved for extension"; break;
    }

    other_decode_bitfield_value(bigbuf, value, 0x70, 8);
    proto_tree_add_text(tree, asn1->tvb,
	saved_offset, asn1->offset - saved_offset,
	"%s :  %s",
	bigbuf, str);

    switch (value & 0x0f)
    {
    case 0x00: str = "unknown"; break;
    case 0x01: str = "ISDN/Telephony Numbering (Rec ITU-T E.164)"; break;
    case 0x02: str = "spare"; break;
    case 0x03: str = "Data Numbering (ITU-T Rec. X.121)"; break;
    case 0x04: str = "Telex Numbering (ITU-T Rec. F.69)"; break;
    case 0x05: str = "spare"; break;
    case 0x06: str = "Land Mobile Numbering (ITU-T Rec. E.212)"; break;
    case 0x07: str = "spare"; break;
    case 0x08: str = "National Numbering"; break;
    case 0x09: str = "Private Numbering"; break;
    case 0x0f: str = "Reserved for extension"; break;
    default:
	str = "Reserved";
	break;
    }

    other_decode_bitfield_value(bigbuf, value, 0x0f, 8);
    proto_tree_add_text(tree, asn1->tvb,
	saved_offset, asn1->offset - saved_offset,
	"%s :  %s",
	bigbuf, str);

    saved_offset = asn1->offset;
    asn1_string_value_decode(asn1, len - 1, &poctets);

    my_dgt_tbcd_unpack(bigbuf, poctets, len - 1, &Dgt_msid);

    proto_tree_add_text(tree, asn1->tvb, saved_offset, len - 1,
	"BCD Digits %s", bigbuf);
}

static gchar *param_1_strings[] = {
    "imsi",
    "lmsi",
    "msisdn",
    "serviceCentreAddressDA",
    "serviceCentreAddressOA",
    "serviceCentreAddress",
    "msc-Number",
    "SignalInfo",
    "Boolean",
    "locationInfoWithLMSI",
    "networkNode-Number"
};

typedef enum
{
    GSM_MAP_P_IMSI,			/* IMSI */
    GSM_MAP_P_LMSI,			/* LMSI */
    GSM_MAP_P_MSISDN,			/* MSISDN */
    GSM_MAP_P_SC_ADDR_DA,		/* Service Centre Address DA */
    GSM_MAP_P_SC_ADDR_OA,		/* Service Centre Address OA */
    GSM_MAP_P_SC_ADDR,			/* Service Centre Address */
    GSM_MAP_P_MSC_NUMBER,		/* MSC Number */
    GSM_MAP_P_SIG_INFO,			/* Signal Info */
    GSM_MAP_P_BOOL,			/* Boolean */
    GSM_MAP_P_LIWLMSI,			/* Location Information with LMSI */
    GSM_MAP_P_NETNODE_NUM,		/* Network Node Number */
    GSM_MAP_P_NONE			/* NONE */
}
param_idx_t;

#define	NUM_PARAM_1 (sizeof(param_1_strings)/sizeof(value_string))
static gint ett_param_1[NUM_PARAM_1];
static void (*param_1_fcn[])(ASN1_SCK *asn1, proto_tree *tree, guint len) = {
    param_imsi,				/* IMSI */
    param_lmsi,				/* LMSI */
    param_AddressString,		/* MSISDN */
    param_AddressString,		/* Service Centre Address DA */
    param_AddressString,		/* Service Centre Address OA */
    param_AddressString,		/* Service Centre Address */
    param_AddressString,		/* MSC Number */
    NULL,				/* Signal Info */
    param_boolean,			/* Boolean */
    NULL,				/* Location Information with LMSI */
    param_AddressString,		/* Network Node Number */
    NULL				/* NONE */
};


#define	GSM_MAP_PARAM_DISPLAY(Gtree, Goffset, Gtag, Ga1, Ga2) \
    { \
	gchar		*_str = NULL; \
	gint		_ett_param_idx; \
	guint		_len_offset; \
	guint		_len; \
	void		(*_param_fcn)(ASN1_SCK *asn1, proto_tree *tree, guint len) = NULL; \
	proto_tree	*_subtree; \
	proto_item	*_item; \
	gboolean	_def_len; \
 \
	_len_offset = asn1->offset; \
	asn1_length_decode(asn1, &_def_len, &_len); \
\
	if (!_def_len) \
	{ \
	    _len = find_eoc(asn1);\
	} \
\
	if (Ga1 == GSM_MAP_P_NONE) \
	{ \
	    _str = "Parameter Tag"; \
	    _ett_param_idx = ett_param; \
	    _param_fcn = NULL; \
	} \
	else \
	{ \
	    _str = param_1_strings[Ga1]; \
	    _ett_param_idx = ett_param_1[Ga1]; \
	    _param_fcn = param_1_fcn[Ga1]; \
	} \
 \
	_item = \
	    proto_tree_add_text(Gtree, asn1->tvb, Goffset, \
		(asn1->offset - Goffset) + _len + \
		(_def_len ? 0 : MAP_EOC_LEN), Ga2); \
 \
	_subtree = proto_item_add_subtree(_item, _ett_param_idx); \
 \
	proto_tree_add_uint_format(_subtree, hf_map_tag, asn1->tvb, \
	    Goffset, _len_offset - Goffset, Gtag, _str); \
 \
	if (_def_len) \
	{ \
	    proto_tree_add_uint(_subtree, hf_map_length, asn1->tvb, \
		_len_offset, asn1->offset - _len_offset, _len); \
	} \
	else \
	{ \
	    proto_tree_add_text(_subtree, asn1->tvb, \
		_len_offset, asn1->offset - _len_offset, "Length: Indefinite"); \
	} \
 \
	if (_param_fcn == NULL) \
	{ \
	    proto_tree_add_text(_subtree, asn1->tvb, \
		asn1->offset, _len, "Parameter Data"); \
 \
	    asn1->offset += _len; \
	} \
	else \
	{ \
	    (*_param_fcn)(asn1, _subtree, _len); \
	} \
\
	if (!_def_len) \
	{ \
	    dissect_map_eoc(asn1, Gtree); \
	} \
    }


static void
param_SM_RP_DA(ASN1_SCK *asn1, proto_tree *tree)
{
    guint	saved_offset;
    guint	tag;
    gint	idx;

    saved_offset = asn1->offset;
    asn1_id_decode1(asn1, &tag);

    switch (tag)
    {
    case 0x80:	/* IMSI */
	idx = GSM_MAP_P_IMSI;
	break;

    case 0x81:	/* LMSI */
	idx = GSM_MAP_P_LMSI;
	break;

    case 0x84:	/* AddressString */
	idx = GSM_MAP_P_SC_ADDR_DA;
	break;

    default:
	idx = GSM_MAP_P_NONE;
	break;
    }

    GSM_MAP_PARAM_DISPLAY(tree, saved_offset, tag, idx, "SM-RP-DA");
}


static void
param_SM_RP_OA(ASN1_SCK *asn1, proto_tree *tree)
{
    guint	saved_offset;
    guint	tag;
    gint	idx;

    saved_offset = asn1->offset;
    asn1_id_decode1(asn1, &tag);

    switch (tag)
    {
    case 0x82:	/* MSISDN */
	idx = GSM_MAP_P_MSISDN;
	break;

    case 0x84:	/* AddressString */
	idx = GSM_MAP_P_SC_ADDR_DA;
	break;

    default:
	idx = GSM_MAP_P_NONE;
	break;
    }

    GSM_MAP_PARAM_DISPLAY(tree, saved_offset, tag, idx, "SM-RP-OA");
}


static void
param_SM_RP_UI(ASN1_SCK *asn1, proto_tree *tree)
{
    guint	saved_offset, len_offset;
    guint	tag, len;
    gboolean	def_len = FALSE;
    gchar	*str = NULL;
    proto_item	*item;
    proto_tree	*subtree;
    gint	ett_param_idx;
    tvbuff_t	*tpdu_tvb;

    saved_offset = asn1->offset;
    asn1_id_decode1(asn1, &tag);
    len_offset = asn1->offset;
    asn1_length_decode(asn1, &def_len, &len);

    str = param_1_strings[GSM_MAP_P_SIG_INFO];
    ett_param_idx = ett_param_1[GSM_MAP_P_SIG_INFO];

    item =
	proto_tree_add_text(tree, asn1->tvb, saved_offset, -1, "SM-RP-UI");

    subtree = proto_item_add_subtree(item, ett_param_idx);

    proto_tree_add_uint_format(subtree, hf_map_tag, asn1->tvb,
	saved_offset, len_offset - saved_offset, tag, str);

    if (def_len)
    {
	proto_tree_add_uint(subtree, hf_map_length, asn1->tvb,
	    len_offset, asn1->offset - len_offset, len);
    }
    else
    {
	proto_tree_add_text(subtree, asn1->tvb,
	    len_offset, asn1->offset - len_offset, "Length: Indefinite");

	len = find_eoc(asn1);
    }

    proto_item_set_len(item, (asn1->offset - saved_offset) + len +
	(def_len ? 0 : MAP_EOC_LEN));

    proto_tree_add_text(subtree, asn1->tvb, asn1->offset, len, "TPDU");

    /*
     * dissect the embedded TPDU message
     */
    tpdu_tvb = tvb_new_subset(asn1->tvb, asn1->offset, len, len);

    dissector_try_port(sms_dissector_table, 0, tpdu_tvb, g_pinfo, g_tree);

    asn1->offset += len;

    if (!def_len)
    {
	dissect_map_eoc(asn1, subtree);
    }
}



static void
param_LWI_LMSI(ASN1_SCK *asn1, proto_tree *tree)
{
    guint	saved_offset, len_offset;
    guint	tag, len;
    gboolean	def_len = FALSE;
    gchar	*str = NULL;
    proto_item	*item;
    proto_tree	*subtree;
    gint	ett_param_idx;

    saved_offset = asn1->offset;
    asn1_id_decode1(asn1, &tag);
    len_offset = asn1->offset;
    asn1_length_decode(asn1, &def_len, &len);

    str = param_1_strings[GSM_MAP_P_LIWLMSI];
    ett_param_idx = ett_param_1[GSM_MAP_P_LIWLMSI];

    item =
	proto_tree_add_text(tree, asn1->tvb, saved_offset, -1,
	    "LOCATION INFO WITH LMSI");

    subtree = proto_item_add_subtree(item, ett_param_idx);

    proto_tree_add_uint_format(subtree, hf_map_tag, asn1->tvb,
	saved_offset, len_offset - saved_offset, tag, str);

    if (def_len)
    {
	proto_tree_add_uint(subtree, hf_map_length, asn1->tvb,
	    len_offset, asn1->offset - len_offset, len);
    }
    else
    {
	proto_tree_add_text(subtree, asn1->tvb,
	    len_offset, asn1->offset - len_offset, "Length: Indefinite");

	len = find_eoc(asn1);
    }

    proto_item_set_len(item, (asn1->offset - saved_offset) + len +
	(def_len ? 0 : MAP_EOC_LEN));

    saved_offset = asn1->offset;
    asn1_id_decode1(asn1, &tag);

    GSM_MAP_PARAM_DISPLAY(subtree, saved_offset, tag, GSM_MAP_P_NETNODE_NUM, "NETWORK NODE NUMBER");

    dissect_map_params(asn1, subtree, len - (asn1->offset - saved_offset));

    if (!def_len)
    {
	dissect_map_eoc(asn1, subtree);
    }
}


/* MESSAGES */

static void
op_send_auth_info(ASN1_SCK *asn1, proto_tree *tree)
{
    guint	saved_offset, len_offset;
    guint	tag, len;
    gboolean	def_len = FALSE;
    proto_item	*item;
    proto_tree	*subtree;

    if (tvb_length_remaining(asn1->tvb, asn1->offset) <= 0) return;

    saved_offset = asn1->offset;
    asn1_id_decode1(asn1, &tag);

    if (tag != MAP_SEQ_TAG)
    {
	GSM_MAP_PARAM_DISPLAY(tree, saved_offset, tag, GSM_MAP_P_IMSI, "IMSI");

	return;
    }

    len_offset = asn1->offset;
    asn1_length_decode(asn1, &def_len, &len);

    item =
	proto_tree_add_text(tree, asn1->tvb, saved_offset, -1, "Sequence");

    subtree = proto_item_add_subtree(item, ett_params);

    proto_tree_add_uint_format(subtree, hf_map_tag, asn1->tvb,
	saved_offset, len_offset - saved_offset, tag, "Sequence Tag");

    if (def_len)
    {
	proto_tree_add_uint(subtree, hf_map_length, asn1->tvb,
	    len_offset, asn1->offset - len_offset, len);
    }
    else
    {
	proto_tree_add_text(subtree, asn1->tvb,
	    len_offset, asn1->offset - len_offset, "Length: Indefinite");

	len = find_eoc(asn1);
    }

    proto_item_set_len(item, (asn1->offset - saved_offset) + len +
	(def_len ? 0 : MAP_EOC_LEN));

    saved_offset = asn1->offset;
    asn1_id_decode1(asn1, &tag);

    GSM_MAP_PARAM_DISPLAY(subtree, saved_offset, tag, GSM_MAP_P_IMSI, "IMSI");

    dissect_map_params(asn1, subtree, len - (asn1->offset - saved_offset));

    if (!def_len)
    {
	dissect_map_eoc(asn1, subtree);
    }
}

static void
op_mo_forward_sm(ASN1_SCK *asn1, proto_tree *tree)
{
    guint	saved_offset, len_offset;
    guint	tag, len;
    gboolean	def_len = FALSE;
    proto_item	*item;
    proto_tree	*subtree;

    if (tvb_length_remaining(asn1->tvb, asn1->offset) <= 0) return;

    saved_offset = asn1->offset;
    asn1_id_decode1(asn1, &tag);
    len_offset = asn1->offset;
    asn1_length_decode(asn1, &def_len, &len);

    if (tag != MAP_SEQ_TAG)
    {
	/*
	 * Hmmm, unexpected
	 */
	return;
    }

    item =
	proto_tree_add_text(tree, asn1->tvb, saved_offset, -1, "Sequence");

    subtree = proto_item_add_subtree(item, ett_params);

    proto_tree_add_uint_format(subtree, hf_map_tag, asn1->tvb,
	saved_offset, len_offset - saved_offset, tag, "Sequence Tag");

    if (def_len)
    {
	proto_tree_add_uint(subtree, hf_map_length, asn1->tvb,
	    len_offset, asn1->offset - len_offset, len);
    }
    else
    {
	proto_tree_add_text(subtree, asn1->tvb,
	    len_offset, asn1->offset - len_offset, "Length: Indefinite");

	len = find_eoc(asn1);
    }

    proto_item_set_len(item, (asn1->offset - saved_offset) + len +
	(def_len ? 0 : MAP_EOC_LEN));

    saved_offset = asn1->offset;

    param_SM_RP_DA(asn1, subtree);

    param_SM_RP_OA(asn1, subtree);

    param_SM_RP_UI(asn1, subtree);

    dissect_map_params(asn1, subtree, len - (asn1->offset - saved_offset));

    if (!def_len)
    {
	dissect_map_eoc(asn1, subtree);
    }
}

static void
op_mt_forward_sm(ASN1_SCK *asn1, proto_tree *tree)
{
    guint	saved_offset, len_offset;
    guint	tag, len;
    gboolean	def_len = FALSE;
    proto_item	*item;
    proto_tree	*subtree;

    if (tvb_length_remaining(asn1->tvb, asn1->offset) <= 0) return;

    saved_offset = asn1->offset;
    asn1_id_decode1(asn1, &tag);
    len_offset = asn1->offset;
    asn1_length_decode(asn1, &def_len, &len);

    if (tag != MAP_SEQ_TAG)
    {
	/*
	 * Hmmm, unexpected
	 */
	return;
    }

    item =
	proto_tree_add_text(tree, asn1->tvb, saved_offset, -1, "Sequence");

    subtree = proto_item_add_subtree(item, ett_params);

    proto_tree_add_uint_format(subtree, hf_map_tag, asn1->tvb,
	saved_offset, len_offset - saved_offset, tag, "Sequence Tag");

    if (def_len)
    {
	proto_tree_add_uint(subtree, hf_map_length, asn1->tvb,
	    len_offset, asn1->offset - len_offset, len);
    }
    else
    {
	proto_tree_add_text(subtree, asn1->tvb,
	    len_offset, asn1->offset - len_offset, "Length: Indefinite");

	len = find_eoc(asn1);
    }

    proto_item_set_len(item, (asn1->offset - saved_offset) + len +
	(def_len ? 0 : MAP_EOC_LEN));

    saved_offset = asn1->offset;

    param_SM_RP_DA(asn1, subtree);

    param_SM_RP_OA(asn1, subtree);

    param_SM_RP_UI(asn1, subtree);

    dissect_map_params(asn1, subtree, len - (asn1->offset - saved_offset));

    if (!def_len)
    {
	dissect_map_eoc(asn1, subtree);
    }
}

static void
op_forward_sm_rr(ASN1_SCK *asn1, proto_tree *tree)
{
    guint	saved_offset, len_offset;
    guint	tag, len;
    gboolean	def_len = FALSE;
    proto_item	*item;
    proto_tree	*subtree;

    if (tvb_length_remaining(asn1->tvb, asn1->offset) <= 0) return;

    saved_offset = asn1->offset;
    asn1_id_decode1(asn1, &tag);
    len_offset = asn1->offset;
    asn1_length_decode(asn1, &def_len, &len);

    if (tag != MAP_SEQ_TAG)
    {
	/*
	 * Hmmm, unexpected
	 */
	return;
    }

    item =
	proto_tree_add_text(tree, asn1->tvb, saved_offset, -1, "Sequence");

    subtree = proto_item_add_subtree(item, ett_params);

    proto_tree_add_uint_format(subtree, hf_map_tag, asn1->tvb,
	saved_offset, len_offset - saved_offset, tag, "Sequence Tag");

    if (def_len)
    {
	proto_tree_add_uint(subtree, hf_map_length, asn1->tvb,
	    len_offset, asn1->offset - len_offset, len);
    }
    else
    {
	proto_tree_add_text(subtree, asn1->tvb,
	    len_offset, asn1->offset - len_offset, "Length: Indefinite");

	len = find_eoc(asn1);
    }

    proto_item_set_len(item, (asn1->offset - saved_offset) + len +
	(def_len ? 0 : MAP_EOC_LEN));

    saved_offset = asn1->offset;

    param_SM_RP_UI(asn1, subtree);

    dissect_map_params(asn1, subtree, len - (asn1->offset - saved_offset));

    if (!def_len)
    {
	dissect_map_eoc(asn1, subtree);
    }
}

static void
op_send_rti_sm(ASN1_SCK *asn1, proto_tree *tree)
{
    guint	saved_offset, len_offset;
    guint	tag, len;
    gboolean	def_len = FALSE;
    proto_item	*item;
    proto_tree	*subtree;

    if (tvb_length_remaining(asn1->tvb, asn1->offset) <= 0) return;

    saved_offset = asn1->offset;
    asn1_id_decode1(asn1, &tag);
    len_offset = asn1->offset;
    asn1_length_decode(asn1, &def_len, &len);

    if (tag != MAP_SEQ_TAG)
    {
	/*
	 * Hmmm, unexpected
	 */
	return;
    }

    item =
	proto_tree_add_text(tree, asn1->tvb,
	    saved_offset, -1, "Sequence");

    subtree = proto_item_add_subtree(item, ett_params);

    proto_tree_add_uint_format(subtree, hf_map_tag, asn1->tvb,
	saved_offset, len_offset - saved_offset, tag, "Sequence Tag");

    if (def_len)
    {
	proto_tree_add_uint(subtree, hf_map_length, asn1->tvb,
	    len_offset, asn1->offset - len_offset, len);
    }
    else
    {
	proto_tree_add_text(subtree, asn1->tvb,
	    len_offset, asn1->offset - len_offset, "Length: Indefinite");

	len = find_eoc(asn1);
    }

    proto_item_set_len(item, (asn1->offset - saved_offset) + len +
	(def_len ? 0 : MAP_EOC_LEN));

    len_offset = asn1->offset;

    saved_offset = asn1->offset;
    asn1_id_decode1(asn1, &tag);

    GSM_MAP_PARAM_DISPLAY(subtree, saved_offset, tag, GSM_MAP_P_MSISDN, "MSISDN");

    saved_offset = asn1->offset;
    asn1_id_decode1(asn1, &tag);

    GSM_MAP_PARAM_DISPLAY(subtree, saved_offset, tag, GSM_MAP_P_BOOL, "SM-RP-PRI");

    saved_offset = asn1->offset;
    asn1_id_decode1(asn1, &tag);

    GSM_MAP_PARAM_DISPLAY(subtree, saved_offset, tag, GSM_MAP_P_SC_ADDR, "Service Centre Address");

    dissect_map_params(asn1, subtree, len - (asn1->offset - len_offset));

    if (!def_len)
    {
	dissect_map_eoc(asn1, subtree);
    }
}

static void
op_send_rti_sm_rr(ASN1_SCK *asn1, proto_tree *tree)
{
    guint	saved_offset, len_offset;
    guint	tag, len;
    gboolean	def_len = FALSE;
    proto_item	*item;
    proto_tree	*subtree;

    if (tvb_length_remaining(asn1->tvb, asn1->offset) <= 0) return;

    saved_offset = asn1->offset;
    asn1_id_decode1(asn1, &tag);
    len_offset = asn1->offset;
    asn1_length_decode(asn1, &def_len, &len);

    if (tag != MAP_SEQ_TAG)
    {
	/*
	 * Hmmm, unexpected
	 */
	return;
    }

    item =
	proto_tree_add_text(tree, asn1->tvb, saved_offset, -1, "Sequence");

    subtree = proto_item_add_subtree(item, ett_params);

    proto_tree_add_uint_format(subtree, hf_map_tag, asn1->tvb,
	saved_offset, len_offset - saved_offset, tag, "Sequence Tag");

    if (def_len)
    {
	proto_tree_add_uint(subtree, hf_map_length, asn1->tvb,
	    len_offset, asn1->offset - len_offset, len);
    }
    else
    {
	proto_tree_add_text(subtree, asn1->tvb,
	    len_offset, asn1->offset - len_offset, "Length: Indefinite");

	len = find_eoc(asn1);
    }

    proto_item_set_len(item, (asn1->offset - saved_offset) + len +
	(def_len ? 0 : MAP_EOC_LEN));

    saved_offset = asn1->offset;
    asn1_id_decode1(asn1, &tag);

    GSM_MAP_PARAM_DISPLAY(subtree, saved_offset, tag, GSM_MAP_P_IMSI, "IMSI");

    param_LWI_LMSI(asn1, subtree);

    dissect_map_params(asn1, subtree, len - (asn1->offset - saved_offset));

    if (!def_len)
    {
	dissect_map_eoc(asn1, subtree);
    }
}

#define	GSM_MAP_NUM_OP (sizeof(gsm_map_opr_code_strings)/sizeof(value_string))
static gint ett_op[GSM_MAP_NUM_OP];
static void (*op_fcn[])(ASN1_SCK *asn1, proto_tree *tree) = {
    NULL,	/* Update Location */
    NULL,	/* Cancel Location */
    NULL,	/* Purge MS */
    NULL,	/* Send Identification */
    NULL,	/* Update GPRS Location */
    NULL,	/* Detach IMSI */
    NULL,	/* Note MM Event */
    NULL,	/* Prepare Handover */
    NULL,	/* Prepare Subsequent Handover */
    NULL,	/* Perform Handover */
    NULL,	/* Perform Subsequent Handover */
    NULL,	/* Send End Signal */
    NULL,	/* Process Access Signalling */
    NULL,	/* Forward Access Signalling */
    op_send_auth_info,	/* Send Authentication Info */
    NULL,	/* Authentication Failure Report */
    NULL,	/* Check IMEI */
    NULL,	/* Reset */
    NULL,	/* Restore Data */
    NULL,	/* Forward Check SS Indication */
    NULL,	/* Activate Trace Mode */
    NULL,	/* Deactivate Trace Mode */
    NULL,	/* Send IMSI */
    NULL,	/* Trace Subscriber Activity */
    NULL,	/* Note Internal Handover */
    NULL,	/* Send Routing Info */
    NULL,	/* Provide Roaming Number */
    NULL,	/* Provide SIWFS Number */
    NULL,	/* SIWFS Signalling Modify */
    NULL,	/* Resume Call Handling */
    NULL,	/* Set Reporting State */
    NULL,	/* Status Report */
    NULL,	/* Remote User Free */
    NULL,	/* Prepare Group Call */
    NULL,	/* Send Group Call End Signalling */
    NULL,	/* Process Group Call Signalling */
    NULL,	/* Forward Group Call Signalling */
    NULL,	/* IST Alert */
    NULL,	/* IST Command */
    NULL,	/* Register SS */
    NULL,	/* Erase SS */
    NULL,	/* Activate SS */
    NULL,	/* Deactivate SS */
    NULL,	/* Interogate SS */
    NULL,	/* Process Unstructured SS Request */
    NULL,	/* Unstructured SS Request */
    NULL,	/* Unstructured SS Notify */
    NULL,	/* Register Password */
    NULL,	/* Get Password */
    NULL,	/* Register CC Entry */
    NULL,	/* Erase CC Entry */
    NULL,	/* Begin Subscriber Activity */
    NULL,	/* Process Unstructured SS Data */
    NULL,	/* SS Invocation Notification */
    op_mo_forward_sm,	/* MO Forward SM */
    op_mt_forward_sm,	/* MT Forward SM */
    op_send_rti_sm,	/* Send Routing Info For SM */
    NULL,	/* Report SM Delivery Status */
    NULL,	/* Inform Service Center */
    NULL,	/* Alert Service Center */
    NULL,	/* Ready For SM */
    NULL,	/* Note Subscriber Present */
    NULL,	/* Alert SC Without Result */
    NULL,	/* Insert Subscriber Data */
    NULL,	/* Delete Subscriber Data */
    NULL,	/* Provide Subscriber Info */
    NULL,	/* Any Time Interrogation */
    NULL,	/* Send Parameters */
    NULL,	/* Any Time Subscription Interrogation */
    NULL,	/* Any Time Modification */
    NULL,	/* Note Subscriber Data Modified */
    NULL,	/* Send Routing Info For GPRS */
    NULL,	/* Failure Report */
    NULL,	/* Note MS Present For GPRS */
    NULL,	/* Provide Subscriber Location */
    NULL,	/* Send Routing Info For LCS */
    NULL,	/* Subscriber Location Report */

    NULL	/* NONE */
};

static gint ett_op_rr[GSM_MAP_NUM_OP];
static void (*op_fcn_rr[])(ASN1_SCK *asn1, proto_tree *tree) = {
    NULL,	/* Update Location */
    NULL,	/* Cancel Location */
    NULL,	/* Purge MS */
    NULL,	/* Send Identification */
    NULL,	/* Update GPRS Location */
    NULL,	/* Detach IMSI */
    NULL,	/* Note MM Event */
    NULL,	/* Prepare Handover */
    NULL,	/* Prepare Subsequent Handover */
    NULL,	/* Perform Handover */
    NULL,	/* Perform Subsequent Handover */
    NULL,	/* Send End Signal */
    NULL,	/* Process Access Signalling */
    NULL,	/* Forward Access Signalling */
    NULL,	/* Send Authentication Info */
    NULL,	/* Authentication Failure Report */
    NULL,	/* Check IMEI */
    NULL,	/* Reset */
    NULL,	/* Restore Data */
    NULL,	/* Forward Check SS Indication */
    NULL,	/* Activate Trace Mode */
    NULL,	/* Deactivate Trace Mode */
    NULL,	/* Send IMSI */
    NULL,	/* Trace Subscriber Activity */
    NULL,	/* Note Internal Handover */
    NULL,	/* Send Routing Info */
    NULL,	/* Provide Roaming Number */
    NULL,	/* Provide SIWFS Number */
    NULL,	/* SIWFS Signalling Modify */
    NULL,	/* Resume Call Handling */
    NULL,	/* Set Reporting State */
    NULL,	/* Status Report */
    NULL,	/* Remote User Free */
    NULL,	/* Prepare Group Call */
    NULL,	/* Send Group Call End Signalling */
    NULL,	/* Process Group Call Signalling */
    NULL,	/* Forward Group Call Signalling */
    NULL,	/* IST Alert */
    NULL,	/* IST Command */
    NULL,	/* Register SS */
    NULL,	/* Erase SS */
    NULL,	/* Activate SS */
    NULL,	/* Deactivate SS */
    NULL,	/* Interogate SS */
    NULL,	/* Process Unstructured SS Request */
    NULL,	/* Unstructured SS Request */
    NULL,	/* Unstructured SS Notify */
    NULL,	/* Register Password */
    NULL,	/* Get Password */
    NULL,	/* Register CC Entry */
    NULL,	/* Erase CC Entry */
    NULL,	/* Begin Subscriber Activity */
    NULL,	/* Process Unstructured SS Data */
    NULL,	/* SS Invocation Notification */
    op_forward_sm_rr,	/* MO Forward SM */
    op_forward_sm_rr,	/* MT Forward SM */
    op_send_rti_sm_rr,	/* Send Routing Info For SM */
    NULL,	/* Report SM Delivery Status */
    NULL,	/* Inform Service Center */
    NULL,	/* Alert Service Center */
    NULL,	/* Ready For SM */
    NULL,	/* Note Subscriber Present */
    NULL,	/* Alert SC Without Result */
    NULL,	/* Insert Subscriber Data */
    NULL,	/* Delete Subscriber Data */
    NULL,	/* Provide Subscriber Info */
    NULL,	/* Any Time Interrogation */
    NULL,	/* Send Parameters */
    NULL,	/* Any Time Subscription Interrogation */
    NULL,	/* Any Time Modification */
    NULL,	/* Note Subscriber Data Modified */
    NULL,	/* Send Routing Info For GPRS */
    NULL,	/* Failure Report */
    NULL,	/* Note MS Present For GPRS */
    NULL,	/* Provide Subscriber Location */
    NULL,	/* Send Routing Info For LCS */
    NULL,	/* Subscriber Location Report */

    NULL	/* NONE */
};


/* GENERIC MAP DISSECTOR FUNCTIONS */

static int
dissect_map_tag(ASN1_SCK *asn1, proto_tree *tree, guint *tag, guchar * str,
    proto_item **item_p)
{
    guint	saved_offset, real_tag;

    saved_offset = asn1->offset;
    asn1_id_decode1(asn1, &real_tag);
    if ((*tag != (guint) -1) && (real_tag != *tag))
    {
	asn1->offset = saved_offset;
	return(MAP_FAIL);
    }

    *item_p =
	proto_tree_add_uint_format(tree, hf_map_tag, asn1->tvb,
	    saved_offset, asn1->offset - saved_offset,
	    real_tag, str);

    return(MAP_OK);
}


static int
dissect_map_len(ASN1_SCK *asn1, proto_tree *tree, gboolean *def_len, guint *len)
{
    guint	saved_offset;

    saved_offset = asn1->offset;
    *len = 0;
    *def_len = FALSE;
    asn1_length_decode(asn1, def_len, len);

    if (*def_len)
    {
	proto_tree_add_uint(tree, hf_map_length, asn1->tvb, saved_offset,
	    asn1->offset - saved_offset, *len);
    }
    else
    {
	proto_tree_add_text(tree, asn1->tvb,
	    saved_offset, asn1->offset - saved_offset, "Length: Indefinite");
    }

    return(MAP_OK);
}


static int
dissect_map_integer(ASN1_SCK *asn1, proto_tree *tree, guint len, guchar * str)
{
    guint	saved_offset;
    gint32	invokeId;

    saved_offset = asn1->offset;
    asn1_int32_value_decode(asn1, len, &invokeId);

    proto_tree_add_int_format(tree, hf_map_int, asn1->tvb,
	saved_offset, asn1->offset - saved_offset,
	invokeId, "%s %d", str, invokeId);

    return(MAP_OK);
}


static int
dissect_map_invokeId(ASN1_SCK *asn1, proto_tree *tree)
{
    guint	saved_offset = 0;
    guint	len;
    guint	tag;
    proto_item	*item, *null_item;
    proto_tree	*subtree;
    gboolean	def_len;

    if (check_map_tag(asn1, MAP_INVOKE_ID_TAG))
    {
	saved_offset = asn1->offset;
	item =
	    proto_tree_add_text(tree, asn1->tvb,
		saved_offset, -1, "Invoke Id");

	subtree = proto_item_add_subtree(item, ett_component);

	tag = -1;
	dissect_map_tag(asn1, subtree, &tag, "Invoke Id Tag", &null_item);
	dissect_map_len(asn1, subtree, &def_len, &len);
	dissect_map_integer(asn1, subtree, len, "Invoke Id:");

	proto_item_set_len(item, asn1->offset - saved_offset);
    }

    return(MAP_OK);
}


static void
dissect_map_problem(ASN1_SCK *asn1, proto_tree *tree)
{
    guint	orig_offset, saved_offset, len_offset;
    guint	len;
    guint	tag;
    proto_tree	*subtree;
    proto_item	*item = NULL;
    gchar	*str = NULL;
    gchar	*type_str = NULL;
    gint32	spec;
    gboolean	def_len;

    orig_offset = asn1->offset;
    saved_offset = asn1->offset;
    asn1_id_decode1(asn1, &tag);

    len_offset = asn1->offset;
    asn1_length_decode(asn1, &def_len, &len);

    item =
	proto_tree_add_text(tree, asn1->tvb, saved_offset, -1, "Problem Code");

    subtree = proto_item_add_subtree(item, ett_problem);

    if (!def_len)
    {
	len = find_eoc(asn1);
    }

    proto_item_set_len(item, (asn1->offset - saved_offset) + len +
	(def_len ? 0 : MAP_EOC_LEN));

    if (len != 1)
    {
	proto_tree_add_text(subtree, asn1->tvb,
	    asn1->offset, len, "Unknown encoding of Problem Code");

	asn1->offset += len;

	if (!def_len)
	{
	    asn1_eoc_decode(asn1, -1);
	}

	return;
    }

    saved_offset = asn1->offset;
    asn1_int32_value_decode(asn1, 1, &spec);

    switch (tag)
    {
    case MAP_GE_PROBLEM_TAG:
	type_str = "General Problem";
	switch (spec)
	{
	case 0: str = "Unrecognized Component"; break;
	case 1: str = "Mistyped Component"; break;
	case 2: str = "Badly Structured Component"; break;
	default:
	    str = "Undefined";
	    break;
	}
	break;

    case MAP_IN_PROBLEM_TAG:
	type_str = "Invoke";
	switch (spec)
	{
	case 0: str = "Duplicate Invoke ID"; break;
	case 1: str = "Unrecognized Operation"; break;
	case 2: str = "Mistyped Parameter"; break;
	case 3: str = "Resource Limitation"; break;
	case 4: str = "Initiating Release"; break;
	case 5: str = "Unrecognized Linked ID"; break;
	case 6: str = "Linked Response Unexpected"; break;
	case 7: str = "Unexpected Linked Operation"; break;
	default:
	    str = "Undefined";
	    break;
	}
	break;

    case MAP_RR_PROBLEM_TAG:
	type_str = "Return Result";
	switch (spec)
	{
	case 0: str = "Unrecognized Invoke ID"; break;
	case 1: str = "Return Result Unexpected"; break;
	case 2: str = "Mistyped Parameter"; break;
	default:
	    str = "Undefined";
	    break;
	}
	break;

    case MAP_RE_PROBLEM_TAG:
	type_str = "Return Error";
	switch (spec)
	{
	case 0: str = "Unrecognized Invoke ID"; break;
	case 1: str = "Return Error Unexpected"; break;
	case 2: str = "Unrecognized Error"; break;
	case 3: str = "Unexpected Error"; break;
	case 4: str = "Mistyped Parameter"; break;
	default:
	    str = "Undefined";
	    break;
	}
	break;

    default:
	type_str = "Undefined";
	break;
    }

    proto_tree_add_uint_format(subtree, hf_map_tag, asn1->tvb,
	orig_offset, len_offset - orig_offset, tag, type_str);

    if (def_len)
    {
	proto_tree_add_uint(subtree, hf_map_length, asn1->tvb,
	    len_offset, saved_offset - len_offset, len);
    }
    else
    {
	proto_tree_add_text(subtree, asn1->tvb,
	    len_offset, saved_offset - len_offset, "Length: Indefinite");
    }

    proto_tree_add_text(subtree, asn1->tvb, saved_offset, 1,
	"Problem Specifier %s", str);
}


static int
dissect_map_lnkId(ASN1_SCK *asn1, proto_tree *tree)
{
    guint	saved_offset = 0;
    guint	len;
    guint	tag;
    proto_item	*item, *null_item;
    proto_tree	*subtree;
    gboolean	def_len;

    if (check_map_tag(asn1, MAP_LINK_ID_TAG))
    {
	saved_offset = asn1->offset;
	item =
	    proto_tree_add_text(tree, asn1->tvb,
		saved_offset, -1, "Linked Id");

	subtree = proto_item_add_subtree(item, ett_component);

	tag = -1;
	dissect_map_tag(asn1, tree, &tag, "Linked Id Tag", &null_item);
	dissect_map_len(asn1, tree, &def_len, &len);
	dissect_map_integer(asn1, tree, len, "Linked Id:");

	proto_item_set_len(item, asn1->offset - saved_offset);
    }

    return(MAP_OK);
}


static int
dissect_map_opr_code(ASN1_SCK *asn1, packet_info *pinfo, proto_tree *tree, gint *op_idx_p)
{
    guint			opr_offset = 0, saved_offset = 0;
    guint			len;
    guint			tag;
    gint32			val;
    gchar			*str = NULL;
    proto_item			*item;
    proto_tree			*subtree;
    gboolean			def_len;
    static gsm_map_tap_rec_t	tap_rec[4];
    static gsm_map_tap_rec_t	*tap_p;
    static int			tap_current=0;

    /*
     * set tap record pointer
     */
    tap_current++;
    if (tap_current == 4)
    {
	tap_current = 0;
    }
    tap_p = &tap_rec[tap_current];

    if (check_map_tag(asn1, MAP_OPR_CODE_TAG))
    {
	opr_offset = asn1->offset;
	item =
	    proto_tree_add_text(tree, asn1->tvb, opr_offset, -1,
		"Operation Code");
	subtree = proto_item_add_subtree(item, ett_opr_code);

	tag = -1;
	asn1_id_decode1(asn1, &tag);
	proto_tree_add_uint_format(subtree, hf_map_tag, asn1->tvb,
	    opr_offset, asn1->offset - opr_offset, tag, "Operation Code Tag");

	dissect_map_len(asn1, subtree, &def_len, &len);

	saved_offset = asn1->offset;
	asn1_int32_value_decode(asn1, len, &val);
	proto_tree_add_int(subtree, hf_map_opr_code, asn1->tvb, saved_offset,
	    asn1->offset - saved_offset, val);

	proto_item_set_len(item, asn1->offset - opr_offset);

	str = my_match_strval(val, gsm_map_opr_code_strings, op_idx_p);

	if (NULL == str) return(MAP_FAIL);

	if (check_col(pinfo->cinfo, COL_INFO))
	{
	    col_append_fstr(pinfo->cinfo, COL_INFO,  "%s ", str);
	}

	tap_p->opr_code_idx = *op_idx_p;

	tap_queue_packet(gsm_map_tap, pinfo, tap_p);
    }

    return(MAP_OK);
}


static int
dissect_map_eoc(ASN1_SCK *asn1, proto_tree *tree)
{
    guint	saved_offset;

    saved_offset = asn1->offset;

    if (tvb_length_remaining(asn1->tvb, saved_offset) <= 0)
    {
	return(MAP_FAIL);
    }

    if (!asn1_eoc(asn1, -1))
    {
	return(MAP_FAIL);
    }

    asn1_eoc_decode(asn1, -1);

    proto_tree_add_text(tree, asn1->tvb,
	saved_offset, asn1->offset - saved_offset, "End of Contents");

    return(MAP_OK);
}


static void
dissect_map_invoke(ASN1_SCK *asn1, packet_info *pinfo, proto_tree *tree)
{
    proto_tree	*subtree;
    guint	orig_offset, saved_offset;
    guint	len;
    guint	tag;
    proto_item	*item;
    gint	op_idx;
    gboolean	def_len;
    int		ret;

    orig_offset = asn1->offset;
    saved_offset = asn1->offset;
    ret = asn1_id_decode1(asn1, &tag);

    item =
	proto_tree_add_text(tree, asn1->tvb, saved_offset, -1, "Component");

    subtree =
	proto_item_add_subtree(item, ett_components);

    proto_tree_add_uint_format(subtree, hf_map_tag, asn1->tvb, saved_offset,
	asn1->offset - saved_offset, tag, "Invoke Type Tag");

    dissect_map_len(asn1, subtree, &def_len, &len);

    saved_offset = asn1->offset;

    dissect_map_invokeId(asn1, subtree);

    dissect_map_lnkId(asn1, subtree);

    if (dissect_map_opr_code(asn1, pinfo, subtree, &op_idx) == MAP_OK)
    {
	if (def_len)
	{
	    len -= asn1->offset - saved_offset;
	}
	else
	{
	    len = find_eoc(asn1);
	}

	/*
	 * decode elements
	 */
	if (op_fcn[op_idx] == NULL)
	{
	    dissect_map_params(asn1, subtree, len);
	}
	else
	{
	    (*op_fcn[op_idx])(asn1, subtree);
	}
    }

    if (!def_len)
    {
	dissect_map_eoc(asn1, subtree);
    }

    proto_item_set_len(item, asn1->offset - orig_offset);
}


static void
dissect_map_rr(ASN1_SCK *asn1, packet_info *pinfo, proto_tree *tree, gchar *str)
{
    guint	tag, len, comp_len;
    gint	op_idx;
    guint	orig_offset, saved_offset, len_offset;
    proto_item	*seq_item, *item;
    proto_tree	*seq_subtree, *subtree;
    gboolean	def_len;
    gboolean	comp_def_len;

    tag = -1;
    orig_offset = asn1->offset;
    saved_offset = asn1->offset;
    asn1_id_decode1(asn1, &tag);

    item =
	proto_tree_add_text(tree, asn1->tvb, saved_offset, -1, "Component");

    subtree =
	proto_item_add_subtree(item, ett_components);

    proto_tree_add_uint_format(subtree, hf_map_tag, asn1->tvb,
	saved_offset, asn1->offset - saved_offset, tag, str);

    dissect_map_len(asn1, subtree, &comp_def_len, &comp_len);

    saved_offset = asn1->offset;

    dissect_map_invokeId(asn1, subtree);

    if (tvb_length_remaining(asn1->tvb, asn1->offset) <= 0)
    {
	proto_item_set_len(item, asn1->offset - orig_offset);

	return;
    }

    saved_offset = asn1->offset;

    tag = -1;
    asn1_id_decode1(asn1, &tag);

    if (tag == MAP_SEQ_TAG)
    {
	len_offset = asn1->offset;
	asn1_length_decode(asn1, &def_len, &len);

	seq_item =
	    proto_tree_add_text(subtree, asn1->tvb, saved_offset, -1, "Sequence");

	seq_subtree = proto_item_add_subtree(seq_item, ett_params);

	proto_tree_add_uint_format(seq_subtree, hf_map_tag, asn1->tvb,
	    saved_offset, len_offset - saved_offset, tag, "Sequence Tag");

	if (def_len)
	{
	    proto_tree_add_uint(seq_subtree, hf_map_length, asn1->tvb,
		len_offset, asn1->offset - len_offset, len);
	}
	else
	{
	    proto_tree_add_text(seq_subtree, asn1->tvb,
		len_offset, asn1->offset - len_offset, "Length: Indefinite");

	    len = find_eoc(asn1);
	}

	proto_item_set_len(seq_item,
	    (asn1->offset - saved_offset) + len +
	    (def_len ? 0 : MAP_EOC_LEN));

	saved_offset = asn1->offset;

	if (dissect_map_opr_code(asn1, pinfo, seq_subtree, &op_idx) == MAP_OK)
	{
	    len -= asn1->offset - saved_offset;

	    /*
	     * decode elements
	     */
	    if (op_fcn_rr[op_idx] == NULL)
	    {
		dissect_map_params(asn1, seq_subtree, len);
	    }
	    else
	    {
		(*op_fcn_rr[op_idx])(asn1, seq_subtree);
	    }
	}

	if (!def_len)
	{
	    dissect_map_eoc(asn1, seq_subtree);
	}
    }

    if (!comp_def_len)
    {
	dissect_map_eoc(asn1, subtree);
    }

    proto_item_set_len(item, asn1->offset - orig_offset);
}


static int
dissect_map_re(ASN1_SCK *asn1, proto_tree *tree)
{
    guint	tag, len, comp_len;
    guint	orig_offset, saved_offset;
    proto_item	*item, *null_item;
    proto_tree	*subtree;
    gboolean	comp_def_len, def_len;

    tag = -1;
    orig_offset = asn1->offset;
    saved_offset = asn1->offset;
    asn1_id_decode1(asn1, &tag);

    item = proto_tree_add_text(tree, asn1->tvb, saved_offset, -1, "Component");

    subtree = proto_item_add_subtree(item, ett_components);

    proto_tree_add_uint_format(subtree, hf_map_tag, asn1->tvb, saved_offset, asn1->offset - saved_offset,
	tag, "Return Error Type Tag");

    dissect_map_len(asn1, subtree, &comp_def_len, &comp_len);

    if (!comp_def_len)
    {
	comp_len = find_eoc(asn1);
    }

    saved_offset = asn1->offset;

    dissect_map_invokeId(asn1, subtree);

#define MAP_LOCAL_ERR_CODE_TAG 0x2
#define MAP_GBL_ERR_CODE_TAG 0x6
    if (check_map_tag(asn1, MAP_LOCAL_ERR_CODE_TAG))
    {
	tag = -1;
	dissect_map_tag(asn1, subtree, &tag, "Local Error Code Tag", &null_item);
    }
    else if (check_map_tag(asn1, MAP_GBL_ERR_CODE_TAG))
    {
	tag = -1;
	dissect_map_tag(asn1, subtree, &tag, "Global Error Code Tag", &null_item);
    }
    else
    {
	proto_tree_add_text(subtree, asn1->tvb, asn1->offset, comp_len,
	    "Unknown Error Code");

	asn1->offset += comp_len;

	if (!comp_def_len)
	{
	    dissect_map_eoc(asn1, subtree);
	}

	proto_item_set_len(item, asn1->offset - orig_offset);

	return(MAP_OK);
    }

    dissect_map_len(asn1, subtree, &def_len, &len);
    dissect_map_integer(asn1, subtree, len, "Error Code:");

    dissect_map_params(asn1, subtree, comp_len - (asn1->offset - saved_offset));

    if (!def_len)
    {
	dissect_map_eoc(asn1, subtree);
    }

    proto_item_set_len(item, asn1->offset - orig_offset);

    return(MAP_OK);
}


static void
dissect_map_reject(ASN1_SCK *asn1, proto_tree *tree)
{
    guint	tag, len;
    guint	saved_offset;
    proto_item	*item;
    proto_tree	*subtree;
    gboolean	def_len;

    tag = -1;

    saved_offset = asn1->offset;
    asn1_id_decode1(asn1, &tag);

    item = proto_tree_add_text(tree, asn1->tvb, saved_offset, -1, "Component");

    subtree = proto_item_add_subtree(item, ett_components);

    proto_tree_add_uint_format(subtree, hf_map_tag, asn1->tvb, saved_offset, asn1->offset - saved_offset,
	tag, "Reject Type Tag");

    dissect_map_len(asn1, subtree, &def_len, &len);

    dissect_map_invokeId(asn1, subtree);
    dissect_map_problem(asn1, subtree);

    if (!def_len)
    {
	dissect_map_eoc(asn1, subtree);
    }

    proto_item_set_len(item, asn1->offset - saved_offset);
}


static void
dissect_map_message(packet_info *pinfo, proto_tree *map_tree, ASN1_SCK *asn1)
{
    guint	tag;
    guint	saved_offset;
    gchar	*str = NULL;
    static int	i = 0;

    saved_offset = asn1->offset;
    asn1_id_decode1(asn1, &tag);
    asn1->offset = saved_offset;

    str = match_strval(tag, tag_strings);

    if (NULL == str) return;

    if (check_col(pinfo->cinfo, COL_INFO))
    {
        if (0 == i)
	{
	    col_append_fstr(pinfo->cinfo, COL_INFO,  "%s ", str);
	}
        else
        {
            col_append_fstr(pinfo->cinfo, COL_INFO,  "& %s ", str);
        }
    }

    switch(tag)
    {
    case MAP_TC_INVOKE :
	dissect_map_invoke(asn1, pinfo, map_tree);
	break;

    case MAP_TC_RRL :
	dissect_map_rr(asn1, pinfo, map_tree, "Return Result(Last) Type Tag");
	break;

    case MAP_TC_RE :
	dissect_map_re(asn1, map_tree);
	break;

    case MAP_TC_REJECT :
	dissect_map_reject(asn1, map_tree);
	break;

    case MAP_TC_RRN :
	dissect_map_rr(asn1, pinfo, map_tree, "Return Result(Not Last) Type Tag");
	break;

    default:
	proto_tree_add_text(map_tree, asn1->tvb, saved_offset, -1,
	    "Message type not handled, ignoring");
	break;
    }
}


static void
dissect_map(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
    proto_item	*ti;
    proto_tree	*map_tree;
    ASN1_SCK	asn1;
    int		offset = 0;

    /*
     * Make entries in Protocol column on summary display
     */
    if (check_col(pinfo->cinfo, COL_PROTOCOL))
    {
	col_set_str(pinfo->cinfo, COL_PROTOCOL, "GSM MAP");
    }


    /* In the interest of speed, if "tree" is NULL, don't do any work not
     * necessary to generate protocol tree items.
     */
    if (tree)
    {
	g_pinfo = pinfo;
	g_tree = tree;

	/* create display subtree for the protocol */
	ti = proto_tree_add_item(tree, proto_map, tvb, 0, -1, FALSE);

	map_tree = proto_item_add_subtree(ti, ett_map);

	asn1_open(&asn1, tvb, offset);

	dissect_map_message(pinfo, map_tree, &asn1);

	asn1_close(&asn1, &offset);
    }
}


/* Register the protocol with Ethereal */

void
proto_register_map(void)
{
    guint		i;
    gint		last_offset;

    /* Setup list of header fields  See Section 1.6.1 for details*/
    static hf_register_info hf[] =
    {
	{ &hf_map_tag,
	    { "Tag",		"map.msgtype",
	    FT_UINT8, BASE_HEX, NULL, 0,
	    "", HFILL }
	},
	{ &hf_map_length,
	    { "Length",		"map.len",
	    FT_UINT8, BASE_HEX, NULL, 0,
	    "", HFILL }
	},
	{ &hf_map_opr_code,
	    { "Operation Code",	"map.oprcode",
	    FT_INT32, BASE_DEC, VALS(gsm_map_opr_code_strings), 0,
	    "", HFILL }
	},
	{ &hf_map_int,
	    { "Integer Data",	"map.data",
	    FT_INT32, BASE_DEC, 0, 0,
	    "", HFILL }
	},
    };

    /* Setup protocol subtree array */
#define	NUM_INDIVIDUAL_PARAMS	7
    static gint *ett[NUM_INDIVIDUAL_PARAMS+(GSM_MAP_NUM_OP*2)+NUM_PARAM_1];

    memset((void *) ett, 0, sizeof(ett));

    ett[0] = &ett_map;
    ett[1] = &ett_opr_code;
    ett[2] = &ett_component;
    ett[3] = &ett_components;
    ett[4] = &ett_param;
    ett[5] = &ett_params;
    ett[6] = &ett_problem;

    last_offset = NUM_INDIVIDUAL_PARAMS;

    for (i=0; i < GSM_MAP_NUM_OP; i++, last_offset++)
    {
	ett_op[i] = -1;
	ett[last_offset] = &ett_op[i];
    }

    for (i=0; i < GSM_MAP_NUM_OP; i++, last_offset++)
    {
	ett_op_rr[i] = -1;
	ett[last_offset] = &ett_op_rr[i];
    }

    for (i=0; i < NUM_PARAM_1; i++, last_offset++)
    {
	ett_param_1[i] = -1;
	ett[last_offset] = &ett_param_1[i];
    }

    /* Register the protocol name and description */
    proto_map =
	proto_register_protocol("GSM Mobile Application Part",
	    "GSM MAP", "gsm_map");

    /* Required function calls to register the header fields and subtrees used */
    proto_register_field_array(proto_map, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));

    sms_dissector_table =
	register_dissector_table("gsm_map.sms_tpdu", "GSM SMS TPDU",
	FT_UINT8, BASE_DEC);

    gsm_map_tap = register_tap("gsm_map");
}

void
proto_reg_handoff_map(void)
{
    dissector_handle_t	map_handle;

    map_handle = create_dissector_handle(dissect_map, proto_map);
    dissector_add("tcap.itu_ssn", 6, map_handle);
    dissector_add("tcap.itu_ssn", 7, map_handle);
    dissector_add("tcap.itu_ssn", 8, map_handle);
    dissector_add("tcap.itu_ssn", 9, map_handle);
}
