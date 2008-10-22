/* packet-gsm_a_common.c
 * Common routines for GSM A Interface dissection
 *
 * Copyright 2003, Michael Lum <mlum [AT] telostech.com>
 * In association with Telos Technology Inc.
 *
 * Split from packet-gsm_a.c by Neil Piercy <Neil [AT] littlebriars.co.uk>
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
#include <epan/tap.h>

#include "packet-bssap.h"
#include "packet-sccp.h"
#include "packet-gsm_a_common.h"

/* nasty globals as a result of the split of packet-gsm_a.c in need of further restructure */
/* nasty static for handling half octet mandatory V IEs */
gboolean lower_nibble=FALSE;

const value_string gsm_common_elem_strings[] = {
	/* Common Information Elements 10.5.1 */
	{ 0x00,	"Cell Identity" },
	{ 0x00,	"Ciphering Key Sequence Number" },
	{ 0x00,	"Location Area Identification" },
	{ 0x00,	"Mobile Identity" },
	{ 0x00,	"Mobile Station Classmark 1" },
	{ 0x00,	"Mobile Station Classmark 2" },
	{ 0x00,	"Mobile Station Classmark 3" },
	{ 0x00,	"Spare Half Octet" },
	{ 0x00,	"Descriptive group or broadcast call reference" },
	{ 0x00,	"Group Cipher Key Number" },
	{ 0x00,	"PD and SAPI $(CCBS)$" },
	{ 0x00,	"Priority Level" },
	{ 0x00,	"PLMN List" },
	{ 0, NULL }
};

/* Mobile Station Classmark Value strings
 */

/* Mobile Station Classmark
 * Revision level
 */
static const value_string gsm_a_msc_rev_vals[] = {
	{ 0,	"Reserved for GSM phase 1"},
	{ 1,	"Used by GSM phase 2 mobile stations"},
	{ 2,	"Used by mobile stations supporting R99 or later versions of the protocol"},
	{ 3,	"Reserved for future use"},
	{ 0,	NULL }
};

/* ES IND (octet 3, bit 5) "Controlled Early Classmark Sending" option implementation */
static const value_string ES_IND_vals[] = {
	{ 0,	"Controlled Early Classmark Sending option is not implemented in the MS"},
	{ 1,	"Controlled Early Classmark Sending option is implemented in the MS"},
	{ 0,	NULL }
};
/* A5/1 algorithm supported (octet 3, bit 4 */
static const value_string A5_1_algorithm_sup_vals[] = {
	{ 0,	"encryption algorithm A5/1 available"},
	{ 1,	"encryption algorithm A5/1 not available"},
	{ 0,	NULL }
};
/* RF Power Capability (Octet 3) */
static const value_string RF_power_capability_vals[] = {
	{ 0,	"class 1"},
	{ 1,	"class 2"},
	{ 2,	"class 3"},
	{ 3,	"class 4"},
	{ 4,	"class 5"},
	{ 7,	"RF Power capability is irrelevant in this information element"},
	{ 0,	NULL }
};
/* PS capability (pseudo-synchronization capability) (octet 4) */
static const value_string ps_sup_cap_vals[] = {
	{ 0,	"PS capability not present"},
	{ 1,	"PS capability present"},
	{ 0,	NULL }
};
/* SS Screening Indicator (octet 4)defined in 3GPP TS 24.080 */
static const value_string SS_screening_indicator_vals[] = {
	{ 0,	"Default value of phase 1"},
	{ 1,	"Capability of handling of ellipsis notation and phase 2 error handling "},
	{ 2,	"For future use"},
	{ 3,	"For future use"},
	{ 0,	NULL }
};
/* SM capability (MT SMS pt to pt capability) (octet 4)*/
static const value_string SM_capability_vals[] = {
	{ 0,	"Mobile station does not support mobile terminated point to point SMS"},
	{ 1,	"Mobile station supports mobile terminated point to point SMS"},
	{ 0,	NULL }
};
/* VBS notification reception (octet 4) */
static const value_string VBS_notification_rec_vals[] = {
	{ 0,	"no VBS capability or no notifications wanted"},
	{ 1,	"VBS capability and notifications wanted"},
	{ 0,	NULL }
};
/* VGCS notification reception (octet 4) */
static const value_string VGCS_notification_rec_vals[] = {
	{ 0,	"no VGCS capability or no notifications wanted"},
	{ 1,	"VGCS capability and notifications wanted"},
	{ 0,	NULL }
};
/* FC Frequency Capability (octet 4 ) */
static const value_string FC_frequency_cap_vals[] = {
	{ 0,	"The MS does not support the E-GSM or R-GSM band"},
	{ 1,	"The MS does support the E-GSM or R-GSM "},
	{ 0,	NULL }
};
/* CM3 (octet 5, bit 8) */
static const value_string CM3_vals[] = {
	{ 0,	"The MS does not support any options that are indicated in CM3"},
	{ 1,	"The MS supports options that are indicated in classmark 3 IE"},
	{ 0,	NULL }
};
/* LCS VA capability (LCS value added location request notification capability) (octet 5,bit 6) */
static const value_string LCS_VA_cap_vals[] = {
	{ 0,	"LCS value added location request notification capability not supported"},
	{ 1,	"LCS value added location request notification capability supported"},
	{ 0,	NULL }
};
/* UCS2 treatment (octet 5, bit 5) */
static const value_string UCS2_treatment_vals[] = {
	{ 0,	"the ME has a preference for the default alphabet"},
	{ 1,	"the ME has no preference between the use of the default alphabet and the use of UCS2"},
	{ 0,	NULL }
};
/* SoLSA (octet 5, bit 4) */
static const value_string SoLSA_vals[] = {
	{ 0,	"The ME does not support SoLSA"},
	{ 1,	"The ME supports SoLSA"},
	{ 0,	NULL }
};
/* CMSP: CM Service Prompt (octet 5, bit 3) */
static const value_string CMSP_vals[] = {
	{ 0,	"Network initiated MO CM connection request not supported"},
	{ 1,	"Network initiated MO CM connection request supported for at least one CM protocol"},
	{ 0,	NULL }
};
/* A5/3 algorithm supported (octet 5, bit 2) */
static const value_string A5_3_algorithm_sup_vals[] = {
	{ 0,	"encryption algorithm A5/3 not available"},
	{ 1,	"encryption algorithm A5/3 available"},
	{ 0,	NULL }
};

/* A5/2 algorithm supported (octet 5, bit 1) */
static const value_string A5_2_algorithm_sup_vals[] = {
	{ 0,	"encryption algorithm A5/2 not available"},
	{ 1,	"encryption algorithm A5/2 available"},
	{ 0,	NULL }
};

static const value_string mobile_identity_type_vals[] = {
	{ 1,	"IMSI"},
	{ 2,	"IMEI"},
	{ 3,	"IMEISV"},
	{ 4,	"TMSI/P-TMSI"},
	{ 5,	"TMGI and optional MBMS Session Identity"}, /* ETSI TS 124 008 V6.8.0 (2005-03) p326 */
	{ 0,	"No Identity"},
	{ 0,	NULL }
};

static const value_string oddevenind_vals[] = {
	{ 0,	"Even number of identity digits"},
	{ 1,	"Odd number of identity digits"},
	{ 0,	NULL }
};

/* Initialize the protocol and registered fields */
static int proto_a_common = -1;

int gsm_a_tap = -1;

int hf_gsm_a_common_elem_id = -1;
static int hf_gsm_a_imsi = -1;
int hf_gsm_a_tmsi = -1;
static int hf_gsm_a_imei = -1;
static int hf_gsm_a_imeisv = -1;

static int hf_gsm_a_MSC_rev = -1;
static int hf_gsm_a_ES_IND			= -1;
static int hf_gsm_a_A5_1_algorithm_sup = -1;
static int hf_gsm_a_RF_power_capability = -1;
static int hf_gsm_a_ps_sup_cap		= -1;
static int hf_gsm_a_SS_screening_indicator = -1;
static int hf_gsm_a_SM_capability		 = -1;
static int hf_gsm_a_VBS_notification_rec = -1;
static int hf_gsm_a_VGCS_notification_rec = -1;
static int hf_gsm_a_FC_frequency_cap	= -1;
static int hf_gsm_a_CM3				= -1;
static int hf_gsm_a_LCS_VA_cap		= -1;
static int hf_gsm_a_UCS2_treatment	= -1;
static int hf_gsm_a_SoLSA				= -1;
static int hf_gsm_a_CMSP				= -1;
static int hf_gsm_a_A5_3_algorithm_sup= -1;
static int hf_gsm_a_A5_2_algorithm_sup = -1;

static int hf_gsm_a_odd_even_ind = -1;
static int hf_gsm_a_mobile_identity_type = -1;
static int hf_gsm_a_tmgi_mcc_mnc_ind = -1;
static int hf_gsm_a_mbs_ses_id_ind = -1;
static int hf_gsm_a_mbs_service_id = -1;
int hf_gsm_a_L3_protocol_discriminator = -1;
static int hf_gsm_a_call_prio = -1;
int hf_gsm_a_skip_ind = -1;

static int hf_gsm_a_b7spare = -1;
int hf_gsm_a_b8spare = -1;

static char a_bigbuf[1024];

sccp_msg_info_t* sccp_msg;
sccp_assoc_info_t* sccp_assoc;

#define	NUM_GSM_COMMON_ELEM (sizeof(gsm_common_elem_strings)/sizeof(value_string))
gint ett_gsm_common_elem[NUM_GSM_COMMON_ELEM];

const char* get_gsm_a_msg_string(int pdu_type, int idx)
{
	const char *msg_string=NULL;

	switch (pdu_type) {
		case GSM_A_PDU_TYPE_BSSMAP:
			msg_string = gsm_bssmap_elem_strings[idx].strptr;
			break;
		case GSM_A_PDU_TYPE_DTAP:
			msg_string = gsm_dtap_elem_strings[idx].strptr;
			break;
		case GSM_A_PDU_TYPE_RP:
			msg_string = gsm_rp_elem_strings[idx].strptr;
			break;
		case GSM_A_PDU_TYPE_RR:
			msg_string = gsm_rr_elem_strings[idx].strptr;
			break;
		case GSM_A_PDU_TYPE_COMMON:
			msg_string = gsm_common_elem_strings[idx].strptr;
			break;
		case GSM_A_PDU_TYPE_GM:
			msg_string = gsm_gm_elem_strings[idx].strptr;
			break;
		case GSM_A_PDU_TYPE_BSSLAP:
			msg_string = gsm_bsslap_elem_strings[idx].strptr;
			break;
		default:
			DISSECTOR_ASSERT_NOT_REACHED();
	}

	return msg_string;
}

static int get_hf_elem_id(int pdu_type)
{
	int			hf_elem_id = 0;

	switch (pdu_type) {
		case GSM_A_PDU_TYPE_BSSMAP:
			hf_elem_id = hf_gsm_a_bssmap_elem_id;
			break;
		case GSM_A_PDU_TYPE_DTAP:
			hf_elem_id = hf_gsm_a_dtap_elem_id;
			break;
		case GSM_A_PDU_TYPE_RP:
			hf_elem_id = hf_gsm_a_rp_elem_id;
			break;
		case GSM_A_PDU_TYPE_RR:
			hf_elem_id = hf_gsm_a_rr_elem_id;
			break;
		case GSM_A_PDU_TYPE_COMMON:
			hf_elem_id = hf_gsm_a_common_elem_id;
			break;
		case GSM_A_PDU_TYPE_GM:
			hf_elem_id = hf_gsm_a_gm_elem_id;
			break;
		case GSM_A_PDU_TYPE_BSSLAP:
			hf_elem_id = hf_gsm_a_bsslap_elem_id;
			break;
		default:
			DISSECTOR_ASSERT_NOT_REACHED();
	}

	return hf_elem_id;
}

/*
 * Type Length Value (TLV) element dissector
 */
guint8 elem_tlv(tvbuff_t *tvb, proto_tree *tree, guint8 iei, gint pdu_type, int idx, guint32 offset, guint len _U_, const gchar *name_add)
{
	guint8		oct;
	guint16		parm_len;
	guint8		lengt_length = 1;
	guint8		consumed;
	guint32		curr_offset;
	proto_tree		*subtree;
	proto_item		*item;
	const value_string	*elem_names;
	gint		*elem_ett;
	guint8 (**elem_funcs)(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len);

	curr_offset = offset;
	consumed = 0;

	SET_ELEM_VARS(pdu_type, elem_names, elem_ett, elem_funcs);

	oct = tvb_get_guint8(tvb, curr_offset);

	if (oct == iei){
		if (oct == GSM_BSSMAP_APDU_IE){
			/* This elements length is in two octets (a bit of a hack here)*/
			lengt_length = 2;
			parm_len = tvb_get_ntohs(tvb, curr_offset + 1);
			lengt_length = 2;
			if(parm_len > 255){
				/* The rest of the logic can't handle length > 255 */
				DISSECTOR_ASSERT_NOT_REACHED();
			}
		}else{
			parm_len = tvb_get_guint8(tvb, curr_offset + 1);
		}

		item =
		proto_tree_add_text(tree,
			tvb, curr_offset, parm_len + 1 + lengt_length,
			"%s%s",
			elem_names[idx].strptr,
			(name_add == NULL) || (name_add[0] == '\0') ? "" : name_add);

		subtree = proto_item_add_subtree(item, elem_ett[idx]);

		proto_tree_add_uint(subtree,
			get_hf_elem_id(pdu_type), tvb,
			curr_offset, 1, oct);

		proto_tree_add_uint(subtree, hf_gsm_a_length, tvb,
			curr_offset + 1, lengt_length, parm_len);

		if (parm_len > 0)
		{
			if (elem_funcs[idx] == NULL)
			{
				proto_tree_add_text(subtree,
					tvb, curr_offset + 1 + lengt_length, parm_len,
					"Element Value");
				/* See ASSERT above */
				consumed = (guint8)parm_len;
			}
			else
			{
				gchar *a_add_string;

				a_add_string=ep_alloc(1024);
				a_add_string[0] = '\0';
				consumed =
				(*elem_funcs[idx])(tvb, subtree, curr_offset + 2,
					parm_len, a_add_string, 1024);

				if (a_add_string[0] != '\0')
				{
					proto_item_append_text(item, "%s", a_add_string);
				}
			}
		}

		consumed += 1 + lengt_length;
	}

	return(consumed);
}

/*
 * Type Value (TV) element dissector
 *
 * Length cannot be used in these functions, big problem if a element dissector
 * is not defined for these.
 */
guint8 elem_tv(tvbuff_t *tvb, proto_tree *tree, guint8 iei, gint pdu_type, int idx, guint32 offset, const gchar *name_add)
{
	guint8		oct;
	guint8		consumed;
	guint32		curr_offset;
	proto_tree		*subtree;
	proto_item		*item;
	const value_string	*elem_names;
	gint		*elem_ett;
	guint8 (**elem_funcs)(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len);

	curr_offset = offset;
	consumed = 0;

	SET_ELEM_VARS(pdu_type, elem_names, elem_ett, elem_funcs);

	oct = tvb_get_guint8(tvb, curr_offset);

	if (oct == iei)
	{
		item =
			proto_tree_add_text(tree,
			tvb, curr_offset, -1,
			"%s%s",
			elem_names[idx].strptr,
				(name_add == NULL) || (name_add[0] == '\0') ? "" : name_add);

		subtree = proto_item_add_subtree(item, elem_ett[idx]);

		proto_tree_add_uint(subtree,
			get_hf_elem_id(pdu_type), tvb,
			curr_offset, 1, oct);

		if (elem_funcs[idx] == NULL)
		{
			/* BAD THING, CANNOT DETERMINE LENGTH */

			proto_tree_add_text(subtree,
				tvb, curr_offset + 1, 1,
				"No element dissector, rest of dissection may be incorrect");

			consumed = 1;
		}
		else
		{
			gchar *a_add_string;

			a_add_string=ep_alloc(1024);
			a_add_string[0] = '\0';
			consumed = (*elem_funcs[idx])(tvb, subtree, curr_offset + 1, -1, a_add_string, 1024);

			if (a_add_string[0] != '\0')
			{
				proto_item_append_text(item, "%s", a_add_string);
			}
		}

		consumed++;

		proto_item_set_len(item, consumed);
	}

	return(consumed);
}

/*
 * Type Value (TV) element dissector
 * Where top half nibble is IEI and bottom half nibble is value.
 *
 * Length cannot be used in these functions, big problem if a element dissector
 * is not defined for these.
 */
guint8 elem_tv_short(tvbuff_t *tvb, proto_tree *tree, guint8 iei, gint pdu_type, int idx, guint32 offset, const gchar *name_add)
{
	guint8		oct;
	guint8		consumed;
	guint32		curr_offset;
	proto_tree		*subtree;
	proto_item		*item;
	const value_string	*elem_names;
	gint		*elem_ett;
	guint8 (**elem_funcs)(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len);
	char buf[10+1];

	curr_offset = offset;
	consumed = 0;

	SET_ELEM_VARS(pdu_type, elem_names, elem_ett, elem_funcs);

	oct = tvb_get_guint8(tvb, curr_offset);

	if ((oct & 0xf0) == (iei & 0xf0))
	{
		item =
			proto_tree_add_text(tree,
				tvb, curr_offset, -1,
				"%s%s",
				elem_names[idx].strptr,
				(name_add == NULL) || (name_add[0] == '\0') ? "" : name_add);

		subtree = proto_item_add_subtree(item, elem_ett[idx]);

		other_decode_bitfield_value(buf, oct, 0xf0, 8);
		proto_tree_add_text(subtree,
			tvb, curr_offset, 1,
			"%s :  Element ID",
			buf);

		if (elem_funcs[idx] == NULL)
		{
			/* BAD THING, CANNOT DETERMINE LENGTH */

			proto_tree_add_text(subtree,
				tvb, curr_offset, 1,
				"No element dissector, rest of dissection may be incorrect");

			consumed++;
		}
		else
		{
			gchar *a_add_string;

			a_add_string=ep_alloc(1024);
			a_add_string[0] = '\0';
			consumed = (*elem_funcs[idx])(tvb, subtree, curr_offset, -1, a_add_string, 1024);

			if (a_add_string[0] != '\0')
			{
				proto_item_append_text(item, "%s", a_add_string);
			}
		}

		proto_item_set_len(item, consumed);
	}

	return(consumed);
}

/*
 * Type (T) element dissector
 */
guint8 elem_t(tvbuff_t *tvb, proto_tree *tree, guint8 iei, gint pdu_type, int idx, guint32 offset, const gchar *name_add)
{
	guint8		oct;
	guint32		curr_offset;
	guint8		consumed;
	const value_string	*elem_names;
	gint		*elem_ett;
	guint8 (**elem_funcs)(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len);

	curr_offset = offset;
	consumed = 0;

	SET_ELEM_VARS(pdu_type, elem_names, elem_ett, elem_funcs);

	oct = tvb_get_guint8(tvb, curr_offset);

	if (oct == iei)
	{
		proto_tree_add_uint_format(tree,
			get_hf_elem_id(pdu_type), tvb,
			curr_offset, 1, oct,
			"%s%s",
			elem_names[idx].strptr,
			(name_add == NULL) || (name_add[0] == '\0') ? "" : name_add);

		consumed = 1;
	}

	return(consumed);
}

/*
 * Length Value (LV) element dissector
 */
guint8 elem_lv(tvbuff_t *tvb, proto_tree *tree, gint pdu_type, int idx, guint32 offset, guint len _U_, const gchar *name_add)
{
	guint8		parm_len;
	guint8		consumed;
	guint32		curr_offset;
	proto_tree		*subtree;
	proto_item		*item;
	const value_string	*elem_names;
	gint		*elem_ett;
	guint8 (**elem_funcs)(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len);

	curr_offset = offset;
	consumed = 0;

	SET_ELEM_VARS(pdu_type, elem_names, elem_ett, elem_funcs);

	parm_len = tvb_get_guint8(tvb, curr_offset);

	item =
		proto_tree_add_text(tree,
			tvb, curr_offset, parm_len + 1,
			"%s%s",
			elem_names[idx].strptr,
			(name_add == NULL) || (name_add[0] == '\0') ? "" : name_add);

	subtree = proto_item_add_subtree(item, elem_ett[idx]);

	proto_tree_add_uint(subtree, hf_gsm_a_length, tvb,
		curr_offset, 1, parm_len);

	if (parm_len > 0)
	{
		if (elem_funcs[idx] == NULL)
		{
			proto_tree_add_text(subtree,
				tvb, curr_offset + 1, parm_len,
				"Element Value");

			consumed = parm_len;
		}
		else
		{
			gchar *a_add_string;

			a_add_string=ep_alloc(1024);
			a_add_string[0] = '\0';
			consumed =
				(*elem_funcs[idx])(tvb, subtree, curr_offset + 1,
					parm_len, a_add_string, 1024);

			if (a_add_string[0] != '\0')
			{
				proto_item_append_text(item, "%s", a_add_string);
			}
		}
	}

	return(consumed + 1);
}

/*
 * Value (V) element dissector
 *
 * Length cannot be used in these functions, big problem if a element dissector
 * is not defined for these.
 */
guint8 elem_v(tvbuff_t *tvb, proto_tree *tree, gint pdu_type, int idx, guint32 offset)
{
	guint8		consumed;
	guint32		curr_offset;
	const value_string	*elem_names;
	gint		*elem_ett;
	guint8 (**elem_funcs)(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len);

	curr_offset = offset;
	consumed = 0;

	SET_ELEM_VARS(pdu_type, elem_names, elem_ett, elem_funcs);

	if (elem_funcs[idx] == NULL)
	{
		/* BAD THING, CANNOT DETERMINE LENGTH */

		proto_tree_add_text(tree,
			tvb, curr_offset, 1,
			"No element dissector, rest of dissection may be incorrect");

		consumed = 1;
	}
	else
	{
		gchar *a_add_string;

		a_add_string=ep_alloc(1024);
		a_add_string[0] = '\0';
		consumed = (*elem_funcs[idx])(tvb, tree, curr_offset, -1, a_add_string, 1024);
	}

	return(consumed);
}

/*
 * Short Value (V_SHORT) element dissector
 *
 * Length is (ab)used in these functions to indicate upper nibble of the octet (-2) or lower nibble (-1)
 * noting that the tv_short dissector always sets the length to -1, as the upper nibble is the IEI.
 * This is expected to be used upper nibble first, as the tables of 24.008.
 */

guint8 elem_v_short(tvbuff_t *tvb, proto_tree *tree, gint pdu_type, int idx, guint32 offset)
{
	guint8		consumed;
	guint32		curr_offset;
	const value_string	*elem_names;
	gint		*elem_ett;
	guint8 (**elem_funcs)(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len);

	curr_offset = offset;
	consumed = 0;

	SET_ELEM_VARS(pdu_type, elem_names, elem_ett, elem_funcs);

	if (elem_funcs[idx] == NULL)
	{
		/* NOT A BAD THING - LENGTH IS HALF NIBBLE */

		proto_tree_add_text(tree,
			tvb, curr_offset, 1,
			"No element dissector");

		consumed = 1;
	}
	else
	{
		gchar *a_add_string;

		a_add_string=ep_alloc(1024);
		a_add_string[0] = '\0';
		consumed = (*elem_funcs[idx])(tvb, tree, curr_offset, (lower_nibble?LOWER_NIBBLE:UPPER_NIBBLE), a_add_string, 1024);
	}
	if (!lower_nibble)	/* is this the first (upper) nibble ? */
	{
		consumed--; /* only half a nibble has been consumed, but all ie dissectors assume they consume 1 octet */
		lower_nibble = TRUE;
	}
	else	/* if it is the second (lower) nibble, move on... */
		lower_nibble = FALSE;

	return(consumed);
}


static dgt_set_t Dgt_tbcd = {
	{
  /*  0   1   2   3   4   5   6   7   8   9   a   b   c   d   e */
	 '0','1','2','3','4','5','6','7','8','9','?','B','C','*','#'
	}
};

static dgt_set_t Dgt1_9_bcd = {
	{
  /*  0   1   2   3   4   5   6   7   8   9   a   b   c   d   e */
	 '0','1','2','3','4','5','6','7','8','9','?','?','?','?','?'
	}
};

/* FUNCTIONS */

/*
 * Unpack BCD input pattern into output ASCII pattern
 *
 * Input Pattern is supplied using the same format as the digits
 *
 * Returns: length of unpacked pattern
 */
int
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

/*
 * Decode the MCC/MNC from 3 octets in 'octs'
 */
static void
mcc_mnc_aux(guint8 *octs, gchar *mcc, gchar *mnc)
{
	if ((octs[0] & 0x0f) <= 9)
	{
		mcc[0] = Dgt_tbcd.out[octs[0] & 0x0f];
	}
	else
	{
		mcc[0] = (octs[0] & 0x0f) + 55;
	}

	if (((octs[0] & 0xf0) >> 4) <= 9)
	{
		mcc[1] = Dgt_tbcd.out[(octs[0] & 0xf0) >> 4];
	}
	else
	{
		mcc[1] = ((octs[0] & 0xf0) >> 4) + 55;
	}

	if ((octs[1] & 0x0f) <= 9)
	{
		mcc[2] = Dgt_tbcd.out[octs[1] & 0x0f];
	}
	else
	{
		mcc[2] = (octs[1] & 0x0f) + 55;
	}

	mcc[3] = '\0';

	if (((octs[1] & 0xf0) >> 4) <= 9)
	{
		mnc[2] = Dgt_tbcd.out[(octs[1] & 0xf0) >> 4];
	}
	else
	{
		mnc[2] = ((octs[1] & 0xf0) >> 4) + 55;
	}

	if ((octs[2] & 0x0f) <= 9)
	{
		mnc[0] = Dgt_tbcd.out[octs[2] & 0x0f];
	}
	else
	{
		mnc[0] = (octs[2] & 0x0f) + 55;
	}

	if (((octs[2] & 0xf0) >> 4) <= 9)
	{
		mnc[1] = Dgt_tbcd.out[(octs[2] & 0xf0) >> 4];
	}
	else
	{
		mnc[1] = ((octs[2] & 0xf0) >> 4) + 55;
	}

	if (mnc[1] == 'F')
	{
		/*
		 * only a 1 digit MNC (very old)
		 */
		mnc[1] = '\0';
	}
	else if (mnc[2] == 'F')
	{
		/*
		 * only a 2 digit MNC
		 */
		mnc[2] = '\0';
	}
	else
	{
		mnc[3] = '\0';
	}
}

/* 3GPP TS 24.008
 * [3] 10.5.1.1 Cell Identity
 */
guint8
de_cell_id(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len)
{
	guint32	curr_offset;

	curr_offset = offset;

	curr_offset +=
	/* 0x02 CI */
	be_cell_id_aux(tvb, tree, offset, len, add_string, string_len, 0x02);

	/* no length check possible */

	return(curr_offset - offset);
}

/*
 * [3] 10.5.1.3
 */
guint8
de_lai(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
	guint8	octs[3];
	guint16	value;
	guint32	curr_offset;
	proto_tree	*subtree;
	proto_item	*item;
	gchar	mcc[4];
	gchar	mnc[4];

	curr_offset = offset;

	item =
		proto_tree_add_text(tree,
			tvb, curr_offset, 5,
			gsm_common_elem_strings[DE_LAI].strptr);

	subtree = proto_item_add_subtree(item, ett_gsm_common_elem[DE_LAI]);

	octs[0] = tvb_get_guint8(tvb, curr_offset);
	octs[1] = tvb_get_guint8(tvb, curr_offset + 1);
	octs[2] = tvb_get_guint8(tvb, curr_offset + 2);

	mcc_mnc_aux(octs, mcc, mnc);


	proto_tree_add_text(subtree,
		tvb, curr_offset, 3,
		"Mobile Country Code (MCC): %s, Mobile Network Code (MNC): %s",
		mcc,
		mnc);

	curr_offset += 3;

	value = tvb_get_ntohs(tvb, curr_offset);

	proto_tree_add_text(subtree,
		tvb, curr_offset, 2,
		"Location Area Code (LAC): 0x%04x (%u)",
		value,
		value);

	proto_item_append_text(item, " - LAC (0x%04x)", value);

	curr_offset += 2;

	/* no length check possible */

	return(curr_offset - offset);
}

/*
 * [3] 10.5.1.4 Mobile Identity
 * 3GPP TS 24.008 version 7.8.0 Release 7
 */
static const true_false_string gsm_a_present_vals = {
	"Present" ,
	"Not present"
};

guint8
de_mid(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len)
{
	guint8	oct;
	guint32	curr_offset;
	guint8	*poctets;
	guint32	value;
	gboolean	odd;

	curr_offset = offset;
	odd = FALSE;

	oct = tvb_get_guint8(tvb, curr_offset);

	switch (oct & 0x07)
	{
	case 0:	/* No Identity */
		other_decode_bitfield_value(a_bigbuf, oct, 0xf0, 8);
		proto_tree_add_text(tree,
			tvb, curr_offset, 1,
			"%s :  Unused",
			a_bigbuf);

		proto_tree_add_item(tree, hf_gsm_a_odd_even_ind, tvb, curr_offset, 1, FALSE);

		proto_tree_add_item(tree, hf_gsm_a_mobile_identity_type, tvb, curr_offset, 1, FALSE);

		if (add_string)
			g_snprintf(add_string, string_len, " - No Identity Code");

		curr_offset++;

		if (len > 1)
		{
			proto_tree_add_text(tree, tvb, curr_offset, len - 1,
				"Format not supported");
		}

		curr_offset += len - 1;
		break;

	case 3:	/* IMEISV */
		/* FALLTHRU */

	case 1:	/* IMSI */
		other_decode_bitfield_value(a_bigbuf, oct, 0xf0, 8);
		proto_tree_add_text(tree,
			tvb, curr_offset, 1,
			"%s :  Identity Digit 1: %c",
			a_bigbuf,
			Dgt1_9_bcd.out[(oct & 0xf0) >> 4]);

		odd = oct & 0x08;

		proto_tree_add_item(tree, hf_gsm_a_odd_even_ind, tvb, curr_offset, 1, FALSE);

		proto_tree_add_item(tree, hf_gsm_a_mobile_identity_type, tvb, curr_offset, 1, FALSE);

		a_bigbuf[0] = Dgt1_9_bcd.out[(oct & 0xf0) >> 4];
		curr_offset++;

		poctets = tvb_get_ephemeral_string(tvb, curr_offset, len - (curr_offset - offset));

		my_dgt_tbcd_unpack(&a_bigbuf[1], poctets, len - (curr_offset - offset),
			&Dgt1_9_bcd);

		proto_tree_add_string_format(tree,
			((oct & 0x07) == 3) ? hf_gsm_a_imeisv : hf_gsm_a_imsi,
			tvb, curr_offset, len - (curr_offset - offset),
			a_bigbuf,
			"BCD Digits: %s",
			a_bigbuf);

		if (sccp_assoc && ! sccp_assoc->calling_party) {
			sccp_assoc->calling_party = se_strdup_printf(
				((oct & 0x07) == 3) ? "IMEISV: %s" : "IMSI: %s",
				a_bigbuf );
		}

		if (add_string)
			g_snprintf(add_string, string_len, " - %s (%s)",
				((oct & 0x07) == 3) ? "IMEISV" : "IMSI",
				a_bigbuf);

		curr_offset += len - (curr_offset - offset);

		if (!odd)
		{
			oct = tvb_get_guint8(tvb, curr_offset - 1);

			other_decode_bitfield_value(a_bigbuf, oct, 0xf0, 8);
			proto_tree_add_text(tree,
				tvb, curr_offset - 1, 1,
				"%s :  Filler",
				a_bigbuf);
		}
		break;

	case 2:	/* IMEI */
		other_decode_bitfield_value(a_bigbuf, oct, 0xf0, 8);
		proto_tree_add_text(tree,
			tvb, curr_offset, 1,
			"%s :  Identity Digit 1: %c",
			a_bigbuf,
			Dgt1_9_bcd.out[(oct & 0xf0) >> 4]);

		proto_tree_add_item(tree, hf_gsm_a_odd_even_ind, tvb, curr_offset, 1, FALSE);

		proto_tree_add_item(tree, hf_gsm_a_mobile_identity_type, tvb, curr_offset, 1, FALSE);

		a_bigbuf[0] = Dgt1_9_bcd.out[(oct & 0xf0) >> 4];
		curr_offset++;

		poctets = tvb_get_ephemeral_string(tvb, curr_offset, len - (curr_offset - offset));

		my_dgt_tbcd_unpack(&a_bigbuf[1], poctets, len - (curr_offset - offset),
			&Dgt1_9_bcd);

		proto_tree_add_string_format(tree,
			hf_gsm_a_imei,
			tvb, curr_offset, len - (curr_offset - offset),
			a_bigbuf,
			"BCD Digits: %s",
			a_bigbuf);

		if (add_string)
			g_snprintf(add_string, string_len, " - IMEI (%s)", a_bigbuf);

		curr_offset += len - (curr_offset - offset);
		break;

	case 4:	/* TMSI/P-TMSI */
		other_decode_bitfield_value(a_bigbuf, oct, 0xf0, 8);
		proto_tree_add_text(tree,
			tvb, curr_offset, 1,
			"%s :  Unused",
			a_bigbuf);

		proto_tree_add_item(tree, hf_gsm_a_odd_even_ind, tvb, curr_offset, 1, FALSE);

		proto_tree_add_item(tree, hf_gsm_a_mobile_identity_type, tvb, curr_offset, 1, FALSE);

		curr_offset++;

		value = tvb_get_ntohl(tvb, curr_offset);

		proto_tree_add_uint(tree, hf_gsm_a_tmsi,
			tvb, curr_offset, 4,
			value);

		if (add_string)
			g_snprintf(add_string, string_len, " - TMSI/P-TMSI (0x%04x)", value);

		curr_offset += 4;
		break;

	case 5: /* TMGI and optional MBMS Session Identity */
		/* MBMS Session Identity indication (octet 3) Bit 6 */
		proto_tree_add_item(tree, hf_gsm_a_mbs_ses_id_ind, tvb, offset, 1, FALSE);
		/* MCC/MNC indication (octet 3) Bit 5 */
		proto_tree_add_item(tree, hf_gsm_a_tmgi_mcc_mnc_ind, tvb, offset, 1, FALSE);
		/* Odd/even indication (octet 3) Bit 4 */
		proto_tree_add_item(tree, hf_gsm_a_odd_even_ind, tvb, curr_offset, 1, FALSE);
		curr_offset++;
		/* MBMS Service ID (octet 4, 5 and 6) */
		proto_tree_add_item(tree, hf_gsm_a_mbs_service_id, tvb, offset, 1, FALSE);
		curr_offset += 3;
		if((oct&0x10)==0x10){
			/* MCC/MNC*/
			/* MCC, Mobile country code (octet 6a, octet 6b bits 1 to 4)*/
			/* MNC, Mobile network code (octet 6b bits 5 to 8, octet 6c) */
			curr_offset += 3;
		}
		if((oct&0x20)==0x20){
			/* MBMS Session Identity (octet 7)
			 * The MBMS Session Identity field is encoded as the value part
			 * of the MBMS Session Identity IE as specified in 3GPP TS 48.018 [86].
			 */
			curr_offset++;
		}
		break;

	default:	/* Reserved */
		proto_tree_add_item(tree, hf_gsm_a_odd_even_ind, tvb, curr_offset, 1, FALSE);
		proto_tree_add_item(tree, hf_gsm_a_mobile_identity_type, tvb, curr_offset, 1, FALSE);
		proto_tree_add_text(tree, tvb, curr_offset, len,
			"Mobile station identity Format %u, Format Unknown",(oct & 0x07));

		if (add_string)
			g_snprintf(add_string, string_len, " - Format Unknown");

		curr_offset += len;
		break;
	}

	EXTRANEOUS_DATA_CHECK(len, curr_offset - offset);

	return(curr_offset - offset);
}

/*
 * [3] 10.5.1.5
 */
guint8
de_ms_cm_1(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
	guint8	oct;
	guint32	curr_offset;
	proto_tree	*subtree;
	proto_item	*item;

	curr_offset = offset;

	oct = tvb_get_guint8(tvb, curr_offset);

	item =
	proto_tree_add_text(tree,
		tvb, curr_offset, 1,
		gsm_common_elem_strings[DE_MS_CM_1].strptr);

	subtree = proto_item_add_subtree(item, ett_gsm_common_elem[DE_MS_CM_1]);

	proto_tree_add_item(subtree, hf_gsm_a_b8spare, tvb, curr_offset, 1, FALSE);

	proto_tree_add_item(subtree, hf_gsm_a_MSC_rev, tvb, curr_offset, 1, FALSE);

	proto_tree_add_item(subtree, hf_gsm_a_ES_IND, tvb, curr_offset, 1, FALSE);

	proto_tree_add_item(subtree, hf_gsm_a_A5_1_algorithm_sup, tvb, curr_offset, 1, FALSE);

	proto_tree_add_item(subtree, hf_gsm_a_RF_power_capability, tvb, curr_offset, 1, FALSE);

	curr_offset++;

	/* no length check possible */

	return(curr_offset - offset);
}

/*
 * [3] 10.5.1.6
 */
guint8
de_ms_cm_2(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string _U_, int string_len _U_)
{
	guint32	curr_offset;
	curr_offset = offset;

	proto_tree_add_item(tree, hf_gsm_a_b8spare, tvb, curr_offset, 1, FALSE);

	proto_tree_add_item(tree, hf_gsm_a_MSC_rev, tvb, curr_offset, 1, FALSE);

	proto_tree_add_item(tree, hf_gsm_a_ES_IND, tvb, curr_offset, 1, FALSE);

	proto_tree_add_item(tree, hf_gsm_a_A5_1_algorithm_sup, tvb, curr_offset, 1, FALSE);

	proto_tree_add_item(tree, hf_gsm_a_RF_power_capability, tvb, curr_offset, 1, FALSE);

	curr_offset++;

	NO_MORE_DATA_CHECK(len);

	proto_tree_add_item(tree, hf_gsm_a_b8spare, tvb, curr_offset, 1, FALSE);

	proto_tree_add_item(tree, hf_gsm_a_ps_sup_cap, tvb, curr_offset, 1, FALSE);

	proto_tree_add_item(tree, hf_gsm_a_SS_screening_indicator, tvb, curr_offset, 1, FALSE);

	/* SM capability (MT SMS pt to pt capability) (octet 4)*/
	proto_tree_add_item(tree, hf_gsm_a_SM_capability, tvb, curr_offset, 1, FALSE);
	/* VBS notification reception (octet 4) */
	proto_tree_add_item(tree, hf_gsm_a_VBS_notification_rec, tvb, curr_offset, 1, FALSE);
	/*VGCS notification reception (octet 4)*/
	proto_tree_add_item(tree, hf_gsm_a_VGCS_notification_rec, tvb, curr_offset, 1, FALSE);
	/* FC Frequency Capability (octet 4 ) */
	proto_tree_add_item(tree, hf_gsm_a_FC_frequency_cap, tvb, curr_offset, 1, FALSE);

	curr_offset++;

	NO_MORE_DATA_CHECK(len);

	/* CM3 (octet 5, bit 8) */
	proto_tree_add_item(tree, hf_gsm_a_CM3, tvb, curr_offset, 1, FALSE);
	/* spare bit 7 */
		proto_tree_add_item(tree, hf_gsm_a_b7spare, tvb, curr_offset, 1, FALSE);
	/* LCS VA capability (LCS value added location request notification capability) (octet 5,bit 6) */
	proto_tree_add_item(tree, hf_gsm_a_LCS_VA_cap, tvb, curr_offset, 1, FALSE);
	/* UCS2 treatment (octet 5, bit 5) */
	proto_tree_add_item(tree, hf_gsm_a_UCS2_treatment, tvb, curr_offset, 1, FALSE);
	/* SoLSA (octet 5, bit 4) */
	proto_tree_add_item(tree, hf_gsm_a_SoLSA, tvb, curr_offset, 1, FALSE);
	/* CMSP: CM Service Prompt (octet 5, bit 3) */
	proto_tree_add_item(tree, hf_gsm_a_CMSP, tvb, curr_offset, 1, FALSE);
	/* A5/3 algorithm supported (octet 5, bit 2) */
	proto_tree_add_item(tree, hf_gsm_a_A5_3_algorithm_sup, tvb, curr_offset, 1, FALSE);
	/* A5/2 algorithm supported (octet 5, bit 1) */
	proto_tree_add_item(tree, hf_gsm_a_A5_2_algorithm_sup, tvb, curr_offset, 1, FALSE);

	curr_offset++;

	EXTRANEOUS_DATA_CHECK(len, curr_offset - offset);

	return(curr_offset - offset);
}
/*
 * [3] 10.5.1.8
 */
static guint8
de_spare_nibble(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
	guint32	curr_offset;

	curr_offset = offset;

	proto_tree_add_text(tree,
		tvb, curr_offset, 1,
		"Spare Nibble");

	curr_offset++;

	/* no length check possible */

	return(curr_offset - offset);
}

/*
 * [3] 10.5.1.9
 */
guint8
de_d_gb_call_ref(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
	guint8	oct;
	guint32	value;
	guint32	curr_offset;
	const gchar *str;

	curr_offset = offset;

	value = tvb_get_ntohl(tvb, curr_offset);

	other_decode_bitfield_value(a_bigbuf, value, 0xffffffe0, 32);
	proto_tree_add_text(tree, tvb, curr_offset, 4,
		"%s :  Group or Broadcast call reference: %u (0x%04x)",
		a_bigbuf,
		(value & 0xffffffe0) >> 5,
		(value & 0xffffffe0) >> 5);

	other_decode_bitfield_value(a_bigbuf, value, 0x00000010, 32);
	proto_tree_add_text(tree, tvb, curr_offset, 4,
		"%s :  SF Service Flag: %s",
		a_bigbuf,
		(value & 0x00000010) ?
		"VGCS (Group call reference)" : "VBS (Broadcast call reference)");

	other_decode_bitfield_value(a_bigbuf, value, 0x00000008, 32);
	proto_tree_add_text(tree, tvb, curr_offset, 4,
		"%s :  AF Acknowledgement Flag: acknowledgment is %srequired",
		a_bigbuf,
		(value & 0x00000008) ? "" : "not ");

	switch (value & 0x00000007)
	{
	case 1: str = "call priority level 4"; break;
	case 2: str = "call priority level 3"; break;
	case 3: str = "call priority level 2"; break;
	case 4: str = "call priority level 1"; break;
	case 5: str = "call priority level 0"; break;
	case 6: str = "call priority level B"; break;
	case 7: str = "call priority level A"; break;
	default:
	str = "no priority applied";
	break;
	}

	other_decode_bitfield_value(a_bigbuf, value, 0x00000007, 32);
	proto_tree_add_text(tree, tvb, curr_offset, 4,
		"%s :  Call Priority: %s",
		a_bigbuf,
		str);

	curr_offset += 4;

	oct = tvb_get_guint8(tvb, curr_offset);

	other_decode_bitfield_value(a_bigbuf, oct, 0xf0, 8);
	proto_tree_add_text(tree, tvb, curr_offset, 1,
		"%s :  Ciphering Information",
		a_bigbuf);

	other_decode_bitfield_value(a_bigbuf, oct, 0x0f, 8);
	proto_tree_add_text(tree, tvb, curr_offset, 1,
		"%s :  Spare",
		a_bigbuf);

	curr_offset++;

	/* no length check possible */

	return(curr_offset - offset);
}

/*
 * [3] 10.5.1.10a
 */
static guint8
de_pd_sapi(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
	guint8	oct;
	guint32	curr_offset;
	proto_tree	*subtree;
	proto_item	*item;
	const gchar *str;

	curr_offset = offset;

	oct = tvb_get_guint8(tvb, curr_offset);

	item =
	proto_tree_add_text(tree,
		tvb, curr_offset, 1,
		gsm_dtap_elem_strings[DE_PD_SAPI].strptr);

	subtree = proto_item_add_subtree(item, ett_gsm_dtap_elem[DE_PD_SAPI]);

	other_decode_bitfield_value(a_bigbuf, oct, 0xc0, 8);
	proto_tree_add_text(subtree, tvb, curr_offset, 1,
		"%s :  Spare",
		a_bigbuf);

	switch ((oct & 0x30) >> 4)
	{
	case 0: str = "SAPI 0"; break;
	case 3: str = "SAPI 3"; break;
	default:
	str = "Reserved";
	break;
	}

	other_decode_bitfield_value(a_bigbuf, oct, 0x30, 8);
	proto_tree_add_text(subtree, tvb, curr_offset, 1,
		"%s :  SAPI (Sevice Access Point Identifier): %s",
		a_bigbuf,
		str);

	proto_tree_add_item(tree, hf_gsm_a_L3_protocol_discriminator, tvb, curr_offset, 1, FALSE);

	curr_offset++;

	/* no length check possible */

	return(curr_offset - offset);
}

/*
 * [3] 10.5.1.11
 */
static const value_string gsm_a_call_prio_vals[] = {
	{ 0x00,	"no priority applied" },
	{ 0x01,	"call priority level 4" },
	{ 0x02,	"call priority level 3" },
	{ 0x03,	"call priority level 2" },
	{ 0x04,	"call priority level 1" },
	{ 0x05,	"call priority level 0" },
	{ 0x06,	"call priority level B" },
	{ 0x07,	"call priority level A" },
	{ 0,			NULL }
};

static guint8
de_prio(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
	guint32	curr_offset;

	curr_offset = offset;

	proto_tree_add_item(tree, hf_gsm_a_b8spare, tvb, curr_offset, 1, FALSE);
	proto_tree_add_item(tree, hf_gsm_a_call_prio, tvb, curr_offset, 1, FALSE);
	curr_offset++;

	/* no length check possible */

	return(curr_offset - offset);
}

/*
 * [3] 10.5.1.13
 */
static guint8
de_plmn_list(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len)
{
	guint8	octs[3];
	guint32	curr_offset;
	gchar	mcc[4];
	gchar	mnc[4];
	guint8	num_plmn;

	curr_offset = offset;

	num_plmn = 0;
	while ((len - (curr_offset - offset)) >= 3)
	{
	octs[0] = tvb_get_guint8(tvb, curr_offset);
	octs[1] = tvb_get_guint8(tvb, curr_offset + 1);
	octs[2] = tvb_get_guint8(tvb, curr_offset + 2);

	mcc_mnc_aux(octs, mcc, mnc);

	proto_tree_add_text(tree,
		tvb, curr_offset, 3,
		"PLMN[%u]  Mobile Country Code (MCC): %s, Mobile Network Code (MNC): %s",
		num_plmn + 1,
		mcc,
		mnc);

	curr_offset += 3;

	num_plmn++;
	}

	if (add_string)
	g_snprintf(add_string, string_len, " - %u PLMN%s",
		num_plmn, plurality(num_plmn, "", "s"));

	EXTRANEOUS_DATA_CHECK(len, curr_offset - offset);

	return(curr_offset - offset);
}

guint8 (*common_elem_fcn[])(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len) = {
	/* Common Information Elements 10.5.1 */
	de_cell_id,	/* Cell Identity */
	NULL /* handled inline */,	/* Ciphering Key Sequence Number */
	de_lai,	/* Location Area Identification */
	de_mid,	/* Mobile Identity */
	de_ms_cm_1,	/* Mobile Station Classmark 1 */
	de_ms_cm_2,	/* Mobile Station Classmark 2 */
	NULL,		/* Mobile Station Classmark 3 */
	de_spare_nibble,	/* Spare Half Octet */
	de_d_gb_call_ref,	/* Descriptive group or broadcast call reference */
	NULL /* handled inline */,	/* Group Cipher Key Number */
	de_pd_sapi,	/* PD and SAPI $(CCBS)$ */
	/* Pos 10 */
	de_prio /* handled inline */,	/* Priority Level */
	de_plmn_list,	/* PLMN List */
	NULL,	/* NONE */
};

/* Register the protocol with Wireshark */
void
proto_register_gsm_a_common(void)
{
	guint	i;
	guint	last_offset;

	/* Setup list of header fields */
	static hf_register_info hf[] =
	{
	{ &hf_gsm_a_common_elem_id,
		{ "Element ID",	"gsm_a_common.elem_id",
		FT_UINT8, BASE_DEC, NULL, 0,
		"", HFILL }
	},
	{ &hf_gsm_a_imsi,
		{ "IMSI",	"gsm_a.imsi",
		FT_STRING, BASE_DEC, 0, 0,
		"", HFILL }
	},
	{ &hf_gsm_a_tmsi,
		{ "TMSI/P-TMSI",	"gsm_a.tmsi",
		FT_UINT32, BASE_HEX, 0, 0x0,
		"", HFILL }
	},
	{ &hf_gsm_a_imei,
		{ "IMEI",	"gsm_a.imei",
		FT_STRING, BASE_DEC, 0, 0,
		"", HFILL }
	},
	{ &hf_gsm_a_imeisv,
		{ "IMEISV",	"gsm_a.imeisv",
		FT_STRING, BASE_DEC, 0, 0,
		"", HFILL }
	},
	{ &hf_gsm_a_MSC_rev,
		{ "Revision Level","gsm_a.MSC2_rev",
		FT_UINT8,BASE_DEC, VALS(gsm_a_msc_rev_vals), 0x60,
		"Revision level", HFILL }
	},
	{ &hf_gsm_a_ES_IND,
		{ "ES IND","gsm_a.MSC2_rev",
		FT_UINT8,BASE_DEC, VALS(ES_IND_vals), 0x10,
			"ES IND", HFILL }
	},
	{ &hf_gsm_a_A5_1_algorithm_sup,
		{ "A5/1 algorithm supported","gsm_a.MSC2_rev",
		FT_UINT8,BASE_DEC, VALS(A5_1_algorithm_sup_vals), 0x08,
		"A5/1 algorithm supported ", HFILL }
	},
	{ &hf_gsm_a_RF_power_capability,
		{ "RF Power Capability","gsm_a.MSC2_rev",
		FT_UINT8,BASE_DEC, VALS(RF_power_capability_vals), 0x07,
		"RF Power Capability", HFILL }
	},
	{ &hf_gsm_a_ps_sup_cap,
		{ "PS capability (pseudo-synchronization capability)","gsm_a.ps_sup_cap",
		FT_UINT8,BASE_DEC, VALS(ps_sup_cap_vals), 0x40,
		"PS capability (pseudo-synchronization capability)", HFILL }
	},
	{ &hf_gsm_a_SS_screening_indicator,
		{ "SS Screening Indicator","gsm_a.SS_screening_indicator",
		FT_UINT8,BASE_DEC, VALS(SS_screening_indicator_vals), 0x30,
		"SS Screening Indicator", HFILL }
	},
	{ &hf_gsm_a_SM_capability,
		{ "SM capability (MT SMS pt to pt capability)","gsm_a.SM_cap",
		FT_UINT8,BASE_DEC, VALS(SM_capability_vals), 0x08,
		"SM capability (MT SMS pt to pt capability)", HFILL }
	},
	{ &hf_gsm_a_VBS_notification_rec,
		{ "VBS notification reception ","gsm_a.VBS_notification_rec",
		FT_UINT8,BASE_DEC, VALS(VBS_notification_rec_vals), 0x04,
		"VBS notification reception ", HFILL }
	},
	{ &hf_gsm_a_VGCS_notification_rec,
		{ "VGCS notification reception ","gsm_a.VGCS_notification_rec",
		FT_UINT8,BASE_DEC, VALS(VGCS_notification_rec_vals), 0x02,
		"VGCS notification reception", HFILL }
	},
	{ &hf_gsm_a_FC_frequency_cap,
		{ "FC Frequency Capability","gsm_a.FC_frequency_cap",
		FT_UINT8,BASE_DEC, VALS(FC_frequency_cap_vals), 0x01,
		"FC Frequency Capability", HFILL }
	},
	{ &hf_gsm_a_CM3,
		{ "CM3","gsm_a.CM3",
		FT_UINT8,BASE_DEC, VALS(CM3_vals), 0x80,
		"CM3", HFILL }
	},
	{ &hf_gsm_a_LCS_VA_cap,
		{ "LCS VA capability (LCS value added location request notification capability) ","gsm_a.LCS_VA_cap",
		FT_UINT8,BASE_DEC, VALS(LCS_VA_cap_vals), 0x20,
		"LCS VA capability (LCS value added location request notification capability) ", HFILL }
	},
	{ &hf_gsm_a_UCS2_treatment,
		{ "UCS2 treatment ","gsm_a.UCS2_treatment",
		FT_UINT8,BASE_DEC, VALS(UCS2_treatment_vals), 0x10,
		"UCS2 treatment ", HFILL }
	},
	{ &hf_gsm_a_SoLSA,
		{ "SoLSA","gsm_a.SoLSA",
		FT_UINT8,BASE_DEC, VALS(SoLSA_vals), 0x08,
		"SoLSA", HFILL }
	},
	{ &hf_gsm_a_CMSP,
		{ "CMSP: CM Service Prompt","gsm_a.CMSP",
		FT_UINT8,BASE_DEC, VALS(CMSP_vals), 0x04,
		"CMSP: CM Service Prompt", HFILL }
	},
	{ &hf_gsm_a_A5_3_algorithm_sup,
		{ "A5/3 algorithm supported","gsm_a.A5_3_algorithm_sup",
		FT_UINT8,BASE_DEC, VALS(A5_3_algorithm_sup_vals), 0x02,
		"A5/3 algorithm supported", HFILL }
	},
	{ &hf_gsm_a_A5_2_algorithm_sup,
		{ "A5/2 algorithm supported","gsm_a.A5_2_algorithm_sup",
		FT_UINT8,BASE_DEC, VALS(A5_2_algorithm_sup_vals), 0x01,
		"A5/2 algorithm supported", HFILL }
	},
	{ &hf_gsm_a_mobile_identity_type,
		{ "Mobile Identity Type","gsm_a.ie.mobileid.type",
		FT_UINT8, BASE_DEC, VALS(mobile_identity_type_vals), 0x07,
		"Mobile Identity Type", HFILL }
	},
	{ &hf_gsm_a_odd_even_ind,
		{ "Odd/even indication","gsm_a.oddevenind",
		FT_UINT8, BASE_DEC, oddevenind_vals, 0x08,
		"Mobile Identity", HFILL }
	},
	{ &hf_gsm_a_tmgi_mcc_mnc_ind,
		{ "MCC/MNC indication", "gsm_a.tmgi_mcc_mnc_ind",
		FT_BOOLEAN, 8, TFS(&gsm_a_present_vals), 0x10,
		"MCC/MNC indication", HFILL}
	},
	{ &hf_gsm_a_mbs_ses_id_ind,
		{ "MBMS Session Identity indication", "gsm_a.tmgi_mcc_mnc_ind",
		FT_BOOLEAN, 8, TFS(&gsm_a_present_vals), 0x20,
		"MBMS Session Identity indication", HFILL}
	},
	{ &hf_gsm_a_mbs_service_id,
		{ "MBMS Service ID", "gsm_a.mbs_service_id",
		FT_BYTES, BASE_HEX, NULL, 0x0,
		"MBMS Service ID", HFILL }
	},
	{ &hf_gsm_a_L3_protocol_discriminator,
		{ "Protocol discriminator","gsm_a.L3_protocol_discriminator",
		FT_UINT8,BASE_DEC, VALS(protocol_discriminator_vals), 0x0f,
		"Protocol discriminator", HFILL }
	},
	{ &hf_gsm_a_call_prio,
		{ "Call priority", "gsm_a.call_prio",
		FT_UINT8, BASE_DEC, VALS(gsm_a_call_prio_vals), 0x07,
		"Call priority", HFILL }
	},
	{ &hf_gsm_a_skip_ind,
		{ "Skip Indicator", "gsm_a.skip.ind",
		FT_UINT8, BASE_DEC, NULL, 0xf0,
		"Skip Indicator", HFILL }
	},
	{ &hf_gsm_a_b7spare,
		{ "Spare","gsm_a.spareb7",
		FT_UINT8,BASE_DEC, NULL, 0x40,
		"Spare", HFILL }
	},
	{ &hf_gsm_a_b8spare,
		{ "Spare","gsm_a.spareb8",
		FT_UINT8,BASE_DEC, NULL, 0x80,
		"Spare", HFILL }
	},
	};

	/* Setup protocol subtree array */
#define	NUM_INDIVIDUAL_ELEMS	0
	static gint *ett[NUM_INDIVIDUAL_ELEMS +
			NUM_GSM_COMMON_ELEM];

	last_offset = NUM_INDIVIDUAL_ELEMS;

	for (i=0; i < NUM_GSM_COMMON_ELEM; i++, last_offset++)
	{
		ett_gsm_common_elem[i] = -1;
		ett[last_offset] = &ett_gsm_common_elem[i];
	}

	/* Register the protocol name and description */

	proto_a_common =
	proto_register_protocol("GSM A-I/F COMMON", "GSM COMMON", "gsm_a_common");

	proto_register_field_array(proto_a_common, hf, array_length(hf));

	proto_register_subtree_array(ett, array_length(ett));

	gsm_a_tap = register_tap("gsm_a");
}


void
proto_reg_handoff_gsm_a_common(void)
{
}
