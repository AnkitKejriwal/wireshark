/* packet-gsm_a_common.h
 *
 * $Id$
 *
 *   Reference [3]
 *   Mobile radio interface Layer 3 specification;
 *   Core network protocols;
 *   Stage 3
 *   (3GPP TS 24.008 version 4.7.0 Release 4)
 *   (ETSI TS 124 008 V6.8.0 (2005-03))
 *
 *   Reference [5]
 *   Point-to-Point (PP) Short Message Service (SMS)
 *   support on mobile radio interface
 *   (3GPP TS 24.011 version 4.1.1 Release 4)
 *
 *   Reference [7]
 *   Mobile radio interface Layer 3 specification;
 *   Core network protocols;
 *   Stage 3
 *   (3GPP TS 24.008 version 5.9.0 Release 5)
 *
 *   Reference [8]
 *   Mobile radio interface Layer 3 specification;
 *   Core network protocols;
 *   Stage 3
 *   (3GPP TS 24.008 version 6.7.0 Release 6)
 *	 (3GPP TS 24.008 version 6.8.0 Release 6)
 *
 * Copyright 2003, Michael Lum <mlum [AT] telostech.com>,
 * In association with Telos Technology Inc.
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
#ifndef __PACKET_GSM_A_COMMON_H__
#define __PACKET_GSM_A_COMMON_H__

#include "packet-sccp.h"

/* PROTOTYPES/FORWARDS */
typedef guint16 (*elem_fcn)(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len);
typedef void (*msg_fcn)(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len);
#if 0
/* XXX moved to tvbuff.h, clean up later */
typedef struct dgt_set_t
{
	unsigned char out[15];
}
dgt_set_t;
#endif
int my_dgt_tbcd_unpack( 
	char	*out,		/* ASCII pattern out */
	guchar	*in,		/* packed pattern in */
	int		num_octs,	/* Number of octets to unpack */
	dgt_set_t	*dgt		/* Digit definitions */
	);

/* globals needed as a result of spltting the packet-gsm_a.c into several files
 * until further restructuring can take place to make them more modular
 */

/* common PD values */
extern const value_string protocol_discriminator_vals[];
extern const value_string gsm_a_pd_short_str_vals[];

extern guint16 de_cld_party_bcd_num(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len);

/* Needed to share the packet-gsm_a_common.c functions */
extern const value_string gsm_bssmap_elem_strings[];
extern gint ett_gsm_bssmap_elem[];
extern elem_fcn bssmap_elem_fcn[];
extern int hf_gsm_a_bssmap_elem_id;
extern int hf_gsm_a_bssmap_cell_ci;

extern const value_string gsm_dtap_elem_strings[];
extern gint ett_gsm_dtap_elem[];
extern elem_fcn dtap_elem_fcn[];
extern int hf_gsm_a_dtap_elem_id;

extern const value_string gsm_rp_elem_strings[];
extern gint ett_gsm_rp_elem[];
extern elem_fcn rp_elem_fcn[];
extern int hf_gsm_a_rp_elem_id;

extern const value_string gsm_rr_elem_strings[];
extern gint ett_gsm_rr_elem[];
extern elem_fcn rr_elem_fcn[];
extern int hf_gsm_a_rr_elem_id;
extern void get_rr_msg_params(guint8 oct, const gchar **msg_str, int *ett_tree, int *hf_idx, msg_fcn *msg_fcn);

extern const value_string gsm_common_elem_strings[];
extern gint ett_gsm_common_elem[];
extern elem_fcn common_elem_fcn[];
extern int hf_gsm_a_common_elem_id;

extern const value_string gsm_gm_elem_strings[];
extern gint ett_gsm_gm_elem[];
extern elem_fcn gm_elem_fcn[];
extern int hf_gsm_a_gm_elem_id;
extern void get_gmm_msg_params(guint8 oct, const gchar **msg_str, int *ett_tree, int *hf_idx, msg_fcn *msg_fcn);
extern void get_sm_msg_params(guint8 oct, const gchar **msg_str, int *ett_tree, int *hf_idx, msg_fcn *msg_fcn);

extern const value_string gsm_bsslap_elem_strings[];
extern gint ett_gsm_bsslap_elem[];
extern elem_fcn bsslap_elem_fcn[];
extern int hf_gsm_a_bsslap_elem_id;

extern const value_string gsm_bssmap_le_elem_strings[];
extern gint ett_gsm_bssmap_le_elem[];
extern elem_fcn bssmap_le_elem_fcn[];
extern int hf_gsm_bssmap_le_elem_id;

extern const value_string nas_eps_common_elem_strings[];
extern gint ett_nas_eps_common_elem[];
extern elem_fcn nas_eps_common_elem_fcn[];
extern int hf_nas_eps_common_elem_id;

extern const value_string nas_emm_elem_strings[];
extern gint ett_nas_eps_emm_elem[];
extern elem_fcn emm_elem_fcn[];
extern int hf_nas_eps_emm_elem_id;

extern const value_string nas_esm_elem_strings[];
extern gint ett_nas_eps_esm_elem[];
extern elem_fcn esm_elem_fcn[];
extern int hf_nas_eps_esm_elem_id;

extern const value_string sgsap_elem_strings[];
extern gint ett_sgsap_elem[];
extern elem_fcn sgsap_elem_fcn[];
extern int hf_sgsap_elem_id;

extern const value_string bssgp_elem_strings[];
extern gint ett_bssgp_elem[];
extern elem_fcn bssgp_elem_fcn[];
extern int hf_bssgp_elem_id;

extern sccp_msg_info_t* sccp_msg;
extern sccp_assoc_info_t* sccp_assoc;

extern int gsm_a_tap;
extern gboolean lower_nibble;
extern packet_info *gsm_a_dtap_pinfo;

/* TS 23 032 */
void dissect_geographical_description(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);

/* common field values */
extern int hf_gsm_a_length;
extern int hf_gsm_a_extension;
extern int hf_gsm_a_tmsi;
extern int hf_gsm_a_L3_protocol_discriminator;
extern int hf_gsm_a_call_prio;
extern int hf_gsm_a_b8spare;
extern int hf_gsm_a_skip_ind;
extern int hf_gsm_a_rr_chnl_needed_ch1;
extern int hf_gsm_a_spare_bits;
extern int hf_gsm_a_lac;

/* for the nasty hack below */
#define GSM_BSSMAP_APDU_IE	0x49

/* flags for the packet-gsm_a_common routines */
#define GSM_A_PDU_TYPE_BSSMAP		0  /* BSSAP_PDU_TYPE_BSSMAP i.e. 0 - until split complete at least! */
#define GSM_A_PDU_TYPE_DTAP			1  /* BSSAP_PDU_TYPE_DTAP i.e. 1   - until split complete at least! */
#define GSM_A_PDU_TYPE_RP			2
#define GSM_A_PDU_TYPE_RR			3
#define GSM_A_PDU_TYPE_COMMON		4
#define GSM_A_PDU_TYPE_GM			5
#define GSM_A_PDU_TYPE_BSSLAP		6
#define GSM_A_PDU_TYPE_SACCH		7
#define GSM_PDU_TYPE_BSSMAP_LE		8
#define NAS_PDU_TYPE_COMMON			9
#define NAS_PDU_TYPE_EMM			10
#define NAS_PDU_TYPE_ESM			11
#define SGSAP_PDU_TYPE				12
#define BSSGP_PDU_TYPE				13

extern const char* get_gsm_a_msg_string(int pdu_type, int idx);

/*
 * this should be set on a per message basis, if possible
 */
#define	IS_UPLINK_FALSE		0
#define	IS_UPLINK_TRUE		1
#define	IS_UPLINK_UNKNOWN	2

/* Defines and nasty static for handling half octet mandatory V IEs 
 * TODO: Note originally UPPER_NIBBLE was -2 and LOWER_NIBBLE was -1
 * changed here to unsigned integer as it wouldn't compile (Warnings on Ubuntu)
 * ugly hack...
 */
#define UPPER_NIBBLE	(2)
#define LOWER_NIBBLE	(1)

/* FUNCTIONS */

/* ELEMENT FUNCTIONS */

#define	EXTRANEOUS_DATA_CHECK(edc_len, edc_max_len) \
	if (((edc_len) > (edc_max_len))||lower_nibble) \
	{ \
		proto_tree_add_text(tree, tvb, \
			curr_offset, (edc_len) - (edc_max_len), "Extraneous Data"); \
		curr_offset += ((edc_len) - (edc_max_len)); \
	}

#define	SHORT_DATA_CHECK(sdc_len, sdc_min_len) \
	if ((sdc_len) < (sdc_min_len)) \
	{ \
		proto_tree_add_text(tree, tvb, \
			curr_offset, (sdc_len), "Short Data (?)"); \
		curr_offset += (sdc_len); \
		return(curr_offset - offset); \
	}

#define	EXACT_DATA_CHECK(edc_len, edc_eq_len) \
	if ((edc_len) != (edc_eq_len)) \
	{ \
		proto_tree_add_text(tree, tvb, \
			curr_offset, (edc_len), "Unexpected Data Length"); \
		curr_offset += (edc_len); \
		return(curr_offset - offset); \
	}

#define	NO_MORE_DATA_CHECK(nmdc_len) \
	if ((nmdc_len) == (curr_offset - offset)) return(nmdc_len);

#define	SET_ELEM_VARS(SEV_pdu_type, SEV_elem_names, SEV_elem_ett, SEV_elem_funcs) \
	switch (SEV_pdu_type) \
	{ \
	case GSM_A_PDU_TYPE_BSSMAP: \
		SEV_elem_names = gsm_bssmap_elem_strings; \
		SEV_elem_ett = ett_gsm_bssmap_elem; \
		SEV_elem_funcs = bssmap_elem_fcn; \
		break; \
	case GSM_A_PDU_TYPE_DTAP: \
		SEV_elem_names = gsm_dtap_elem_strings; \
		SEV_elem_ett = ett_gsm_dtap_elem; \
		SEV_elem_funcs = dtap_elem_fcn; \
		break; \
	case GSM_A_PDU_TYPE_RP: \
		SEV_elem_names = gsm_rp_elem_strings; \
		SEV_elem_ett = ett_gsm_rp_elem; \
		SEV_elem_funcs = rp_elem_fcn; \
		break; \
	case GSM_A_PDU_TYPE_RR: \
		SEV_elem_names = gsm_rr_elem_strings; \
		SEV_elem_ett = ett_gsm_rr_elem; \
		SEV_elem_funcs = rr_elem_fcn; \
		break; \
	case GSM_A_PDU_TYPE_COMMON: \
		SEV_elem_names = gsm_common_elem_strings; \
		SEV_elem_ett = ett_gsm_common_elem; \
		SEV_elem_funcs = common_elem_fcn; \
		break; \
	case GSM_A_PDU_TYPE_GM: \
		SEV_elem_names = gsm_gm_elem_strings; \
		SEV_elem_ett = ett_gsm_gm_elem; \
		SEV_elem_funcs = gm_elem_fcn; \
		break; \
	case GSM_A_PDU_TYPE_BSSLAP: \
		SEV_elem_names = gsm_bsslap_elem_strings; \
		SEV_elem_ett = ett_gsm_bsslap_elem; \
		SEV_elem_funcs = bsslap_elem_fcn; \
		break; \
	case GSM_PDU_TYPE_BSSMAP_LE: \
		SEV_elem_names = gsm_bssmap_le_elem_strings; \
		SEV_elem_ett = ett_gsm_bssmap_le_elem; \
		SEV_elem_funcs = bssmap_le_elem_fcn; \
		break; \
	case NAS_PDU_TYPE_COMMON: \
		SEV_elem_names = nas_eps_common_elem_strings; \
		SEV_elem_ett = ett_nas_eps_common_elem; \
		SEV_elem_funcs = nas_eps_common_elem_fcn; \
		break; \
	case NAS_PDU_TYPE_EMM: \
		SEV_elem_names = nas_emm_elem_strings; \
		SEV_elem_ett = ett_nas_eps_emm_elem; \
		SEV_elem_funcs = emm_elem_fcn; \
		break; \
	case NAS_PDU_TYPE_ESM: \
		SEV_elem_names = nas_esm_elem_strings; \
		SEV_elem_ett = ett_nas_eps_esm_elem; \
		SEV_elem_funcs = esm_elem_fcn; \
		break; \
	case SGSAP_PDU_TYPE: \
		SEV_elem_names = sgsap_elem_strings; \
		SEV_elem_ett = ett_sgsap_elem; \
		SEV_elem_funcs = sgsap_elem_fcn; \
		break; \
	case BSSGP_PDU_TYPE: \
		SEV_elem_names = bssgp_elem_strings; \
		SEV_elem_ett = ett_bssgp_elem; \
		SEV_elem_funcs = bssgp_elem_fcn; \
		break; \
	default: \
		proto_tree_add_text(tree, \
			tvb, curr_offset, -1, \
			"Unknown PDU type (%u) gsm_a_common", SEV_pdu_type); \
		return(consumed); \
	}

/*
 * Type Length Value (TLV) element dissector
 */
extern guint16 elem_tlv(tvbuff_t *tvb, proto_tree *tree, guint8 iei, gint pdu_type, int idx, guint32 offset, guint len, const gchar *name_add);

/*
 * Type Extendable Length Value (TLVE) element dissector
 */
extern guint16 elem_telv(tvbuff_t *tvb, proto_tree *tree, guint8 iei, gint pdu_type, int idx, guint32 offset, guint len, const gchar *name_add);

/*
 * Type Length Value (TLV-E) element dissector
 */
extern guint16 elem_tlv_e(tvbuff_t *tvb, proto_tree *tree, guint8 iei, gint pdu_type, int idx, guint32 offset, guint len, const gchar *name_add);

/*
 * Type Value (TV) element dissector
 *
 * Length cannot be used in these functions, big problem if a element dissector
 * is not defined for these.
 */
extern guint16 elem_tv(tvbuff_t *tvb, proto_tree *tree, guint8 iei, gint pdu_type, int idx, guint32 offset, const gchar *name_add);

/*
 * Type Value (TV) element dissector
 * Where top half nibble is IEI and bottom half nibble is value.
 *
 * Length cannot be used in these functions, big problem if a element dissector
 * is not defined for these.
 */
extern guint16 elem_tv_short(tvbuff_t *tvb, proto_tree *tree, guint8 iei, gint pdu_type, int idx, guint32 offset, const gchar *name_add);

/*
 * Type (T) element dissector
 */
extern guint16 elem_t(tvbuff_t *tvb, proto_tree *tree, guint8 iei, gint pdu_type, int idx, guint32 offset, const gchar *name_add);

/*
 * Length Value (LV) element dissector
 */
extern guint16 elem_lv(tvbuff_t *tvb, proto_tree *tree, gint pdu_type, int idx, guint32 offset, guint len, const gchar *name_add);

/*
 * Length Value (LV-E) element dissector
 */
extern guint16 elem_lv_e(tvbuff_t *tvb, proto_tree *tree, gint pdu_type, int idx, guint32 offset, guint len, const gchar *name_add);

/*
 * Value (V) element dissector
 *
 * Length cannot be used in these functions, big problem if a element dissector
 * is not defined for these.
 */
extern guint16 elem_v(tvbuff_t *tvb, proto_tree *tree, gint pdu_type, int idx, guint32 offset);

/*
 * Short Value (V_SHORT) element dissector
 *
 * Length is (ab)used in these functions to indicate upper nibble of the octet (-2) or lower nibble (-1)
 * noting that the tv_short dissector always sets the length to -1, as the upper nibble is the IEI.
 * This is expected to be used upper nibble first, as the tables of 24.008.
 */

extern guint16 elem_v_short(tvbuff_t *tvb, proto_tree *tree, gint pdu_type, int idx, guint32 offset);


/* XXX: Most (if not all) the functions which make use of the following macros have the variables 'consumed',
 *      'curr_offset', and 'cur_len' declared as *unsigned*.  This means that the 'if (curr_len <= 0)' statement
 *      originally at the end of each of the macros would always be FALSE since an unsigned cannot be less than 0.
 *      I've chosen to change the statement to 'if ((signed)curr_len <= 0)'. (Although this may be a bit of a
 *      hack, it seems simpler than changing declarations to signed in all the places these macros are used).
 *      Is there a better approach ?
 */

#define ELEM_MAND_TLV(EMT_iei, EMT_pdu_type, EMT_elem_idx, EMT_elem_name_addition) \
{\
	if ((consumed = elem_tlv(tvb, tree, (guint8) EMT_iei, EMT_pdu_type, EMT_elem_idx, curr_offset, curr_len, EMT_elem_name_addition)) > 0) \
	{ \
		curr_offset += consumed; \
		curr_len -= consumed; \
	} \
	else \
	{ \
		proto_tree_add_text(tree, \
			tvb, curr_offset, 0, \
			"Missing Mandatory element (0x%02x) %s%s, rest of dissection is suspect", \
			EMT_iei, \
			get_gsm_a_msg_string(EMT_pdu_type, EMT_elem_idx), \
			(EMT_elem_name_addition == NULL) ? "" : EMT_elem_name_addition \
			); \
	} \
	if ((signed)curr_len <= 0) return;      \
}
/* This is a version where the length field can be one or two octets depending 
 * if the extension bit is set or not (TS 48.016 p 10.1.2).
 *         8        7 6 5 4 3 2 1
 * octet 2 0/1 ext  length
 * octet 2a length
 */
#define ELEM_MAND_TELV(EMT_iei, EMT_pdu_type, EMT_elem_idx, EMT_elem_name_addition) \
{\
	if ((consumed = elem_telv(tvb, tree, (guint8) EMT_iei, EMT_pdu_type, EMT_elem_idx, curr_offset, curr_len, EMT_elem_name_addition)) > 0) \
	{ \
		curr_offset += consumed; \
		curr_len -= consumed; \
	} \
	else \
	{ \
		proto_tree_add_text(tree, \
			tvb, curr_offset, 0, \
			"Missing Mandatory element (0x%02x) %s%s, rest of dissection is suspect", \
			EMT_iei, \
			get_gsm_a_msg_string(EMT_pdu_type, EMT_elem_idx), \
			(EMT_elem_name_addition == NULL) ? "" : EMT_elem_name_addition \
			); \
	} \
	if ((signed)curr_len <= 0) return;      \
}

#define ELEM_MAND_TLV_E(EMT_iei, EMT_pdu_type, EMT_elem_idx, EMT_elem_name_addition) \
{\
	if ((consumed = elem_tlv_e(tvb, tree, (guint8) EMT_iei, EMT_pdu_type, EMT_elem_idx, curr_offset, curr_len, EMT_elem_name_addition)) > 0) \
	{ \
		curr_offset += consumed; \
		curr_len -= consumed; \
	} \
	else \
	{ \
		proto_tree_add_text(tree, \
			tvb, curr_offset, 0, \
			"Missing Mandatory element (0x%02x) %s%s, rest of dissection is suspect", \
			EMT_iei, \
			get_gsm_a_msg_string(EMT_pdu_type, EMT_elem_idx), \
			(EMT_elem_name_addition == NULL) ? "" : EMT_elem_name_addition \
			); \
	} \
	if ((signed)curr_len <= 0) return;      \
}
#define ELEM_OPT_TLV(EOT_iei, EOT_pdu_type, EOT_elem_idx, EOT_elem_name_addition) \
{\
	if ((consumed = elem_tlv(tvb, tree, (guint8) EOT_iei, EOT_pdu_type, EOT_elem_idx, curr_offset, curr_len, EOT_elem_name_addition)) > 0) \
	{ \
		curr_offset += consumed; \
		curr_len -= consumed; \
	} \
        if ((signed)curr_len <= 0) return;      \
}

#define ELEM_OPT_TELV(EOT_iei, EOT_pdu_type, EOT_elem_idx, EOT_elem_name_addition) \
{\
	if ((consumed = elem_telv(tvb, tree, (guint8) EOT_iei, EOT_pdu_type, EOT_elem_idx, curr_offset, curr_len, EOT_elem_name_addition)) > 0) \
	{ \
		curr_offset += consumed; \
		curr_len -= consumed; \
	} \
        if ((signed)curr_len <= 0) return;      \
}

#define ELEM_OPT_TLV_E(EOT_iei, EOT_pdu_type, EOT_elem_idx, EOT_elem_name_addition) \
{\
	if ((consumed = elem_tlv_e(tvb, tree, (guint8) EOT_iei, EOT_pdu_type, EOT_elem_idx, curr_offset, curr_len, EOT_elem_name_addition)) > 0) \
	{ \
		curr_offset += consumed; \
		curr_len -= consumed; \
	} \
	if ((signed)curr_len <= 0) return;      \
}

#define ELEM_MAND_TV(EMT_iei, EMT_pdu_type, EMT_elem_idx, EMT_elem_name_addition) \
{\
	if ((consumed = elem_tv(tvb, tree, (guint8) EMT_iei, EMT_pdu_type, EMT_elem_idx, curr_offset, EMT_elem_name_addition)) > 0) \
	{ \
		curr_offset += consumed; \
		curr_len -= consumed; \
	} \
	else \
	{ \
		proto_tree_add_text(tree, \
			tvb, curr_offset, 0, \
			"Missing Mandatory element (0x%02x) %s%s, rest of dissection is suspect", \
			EMT_iei, \
			get_gsm_a_msg_string(EMT_pdu_type, EMT_elem_idx), \
			(EMT_elem_name_addition == NULL) ? "" : EMT_elem_name_addition \
		); \
	} \
	if ((signed)curr_len <= 0) return;      \
}

#define ELEM_OPT_TV(EOT_iei, EOT_pdu_type, EOT_elem_idx, EOT_elem_name_addition) \
{\
	if ((consumed = elem_tv(tvb, tree, (guint8) EOT_iei, EOT_pdu_type, EOT_elem_idx, curr_offset, EOT_elem_name_addition)) > 0) \
	{ \
		curr_offset += consumed; \
		curr_len -= consumed; \
	} \
        if ((signed)curr_len <= 0) return;      \
}

#define ELEM_OPT_TV_SHORT(EOT_iei, EOT_pdu_type, EOT_elem_idx, EOT_elem_name_addition) \
{\
	if ((consumed = elem_tv_short(tvb, tree, EOT_iei, EOT_pdu_type, EOT_elem_idx, curr_offset, EOT_elem_name_addition)) > 0) \
	{ \
		curr_offset += consumed; \
		curr_len -= consumed; \
	} \
	if ((signed)curr_len <= 0) return;      \
}

#define ELEM_OPT_T(EOT_iei, EOT_pdu_type, EOT_elem_idx, EOT_elem_name_addition) \
{\
	if ((consumed = elem_t(tvb, tree, (guint8) EOT_iei, EOT_pdu_type, EOT_elem_idx, curr_offset, EOT_elem_name_addition)) > 0) \
	{ \
		curr_offset += consumed; \
		curr_len -= consumed; \
	} \
	if ((signed)curr_len <= 0) return;      \
}

#define ELEM_MAND_LV(EML_pdu_type, EML_elem_idx, EML_elem_name_addition) \
{\
	if ((consumed = elem_lv(tvb, tree, EML_pdu_type, EML_elem_idx, curr_offset, curr_len, EML_elem_name_addition)) > 0) \
	{ \
		curr_offset += consumed; \
		curr_len -= consumed; \
	} \
	else \
	{ \
		/* Mandatory, but nothing we can do */ \
	} \
	if ((signed)curr_len <= 0) return;      \
}

#define ELEM_MAND_LV_E(EML_pdu_type, EML_elem_idx, EML_elem_name_addition) \
{\
	if ((consumed = elem_lv_e(tvb, tree, EML_pdu_type, EML_elem_idx, curr_offset, curr_len, EML_elem_name_addition)) > 0) \
	{ \
		curr_offset += consumed; \
		curr_len -= consumed; \
	} \
	else \
	{ \
		/* Mandatory, but nothing we can do */ \
	} \
	if ((signed)curr_len <= 0) return;      \
}

#define ELEM_MAND_V(EMV_pdu_type, EMV_elem_idx) \
{\
	if ((consumed = elem_v(tvb, tree, EMV_pdu_type, EMV_elem_idx, curr_offset)) > 0) \
	{ \
		curr_offset += consumed; \
		curr_len -= consumed; \
	} \
	else \
	{ \
		/* Mandatory, but nothing we can do */ \
	} \
	if ((signed)curr_len <= 0) return;      \
}

#define ELEM_MAND_V_SHORT(EMV_pdu_type, EMV_elem_idx) \
{\
	if ((consumed = elem_v_short(tvb, tree, EMV_pdu_type, EMV_elem_idx, curr_offset)) > 0) \
	{ \
		curr_offset += consumed; \
		curr_len -= consumed; \
	} \
	else \
	{ \
		/* Mandatory, but nothing we can do */ \
	} \
	if ((signed)curr_len <= 0) return;      \
}

/*
 * this enum must be kept in-sync with 'gsm_a_pd_str'
 * it is used as an index into the array
 */
typedef enum
{
	PD_GCC = 0,
	PD_BCC,
	PD_RSVD_1,
	PD_CC,
	PD_GTTP,
	PD_MM,
	PD_RR,
	PD_UNK_1,
	PD_GMM,
	PD_SMS,
	PD_SM,
	PD_SS,
	PD_LCS,
	PD_UNK_2,
	PD_RSVD_EXT,
	PD_TP
}
gsm_a_pd_str_e;

typedef struct _gsm_a_tap_rec_t {
	/*
	 * value from packet-bssap.h
	 */
	guint8		pdu_type;
	guint8		message_type;
	gsm_a_pd_str_e	protocol_disc;
} gsm_a_tap_rec_t;

void dissect_bssmap(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);

void dissect_bssmap_le(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);

void bssmap_old_bss_to_new_bss_info(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo);
void bssmap_new_bss_to_old_bss_info(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo);

void dtap_mm_mm_info(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len);

guint16 be_cell_id_aux(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len, guint8 disc);
guint16 be_cell_id_list(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len);
guint16 be_chan_type(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len);

guint16 de_lai(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len);
guint16 de_mid(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len);
guint16 de_cell_id(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len);
guint16 de_bearer_cap(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len);
guint16 de_bearer_cap_uplink(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len);
guint16 be_emlpp_prio(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len);
guint16 be_ganss_loc_type(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len);
guint16 be_ganss_pos_dta(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len);
guint16 be_ganss_ass_dta(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len);
guint16 de_plmn_list(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len);
guint16 de_ms_cm_1(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string _U_, int string_len _U_);
guint16 de_ms_cm_2(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len);
guint16 de_ms_cm_3(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len);
guint16 de_serv_cat(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len);
guint16 de_sm_apn(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len);
guint16 de_sm_pco(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string _U_, int string_len _U_);
guint16 de_sm_qos(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len);
guint16 de_sm_pflow_id(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len);
guint16 de_sm_tflow_temp(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len);
guint16 de_time_zone(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_);
guint16 de_gmm_drx_param(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len);
guint16 de_gmm_ms_net_cap(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len);
guint16 de_gmm_rai(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len);
guint16 de_gmm_ms_radio_acc_cap(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len);

guint16 de_rr_cause(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len);
guint16 de_rr_cell_dsc(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len);
guint16 de_rr_ch_dsc(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string _U_, int string_len _U_);
guint16 de_rr_ch_mode(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len);
guint16 de_rr_chnl_needed(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string _U_, int string_len _U_);
guint16 de_rr_cip_mode_set(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len);
guint16 de_rr_cm_enq_mask(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len);
guint16 de_rr_meas_res(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string _U_, int string_len _U_);
guint16 de_rr_multirate_conf(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string _U_, int string_len _U_);
guint16 de_rr_sus_cau(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len);
guint16 de_rr_tlli(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string, int string_len);

guint16 de_rej_cause(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string _U_, int string_len _U_);
guint16 de_d_gb_call_ref(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string _U_, int string_len _U_);

guint16 de_emm_ue_net_cap(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len, gchar *add_string _U_, int string_len _U_);

void dtap_rr_ho_cmd(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len);
void dtap_rr_cip_mode_cpte(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len);

void bssmap_perf_loc_abort(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len);
void bssmap_reset(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len);
void bssmap_conn_oriented(tvbuff_t *tvb, proto_tree *tree, guint32 offset, guint len);

/*
 * the following allows TAP code access to the messages
 * without having to duplicate it. With MSVC and a 
 * libwireshark.dll, we need a special declaration.
 */
WS_VAR_IMPORT const value_string gsm_a_bssmap_msg_strings[];
WS_VAR_IMPORT const value_string gsm_a_dtap_msg_mm_strings[];
WS_VAR_IMPORT const value_string gsm_a_dtap_msg_rr_strings[];
WS_VAR_IMPORT const value_string gsm_a_dtap_msg_cc_strings[];
WS_VAR_IMPORT const value_string gsm_a_dtap_msg_gmm_strings[];
WS_VAR_IMPORT const value_string gsm_a_dtap_msg_sms_strings[];
WS_VAR_IMPORT const value_string gsm_a_dtap_msg_sm_strings[];
WS_VAR_IMPORT const value_string gsm_a_dtap_msg_ss_strings[];
WS_VAR_IMPORT const value_string gsm_a_dtap_msg_tp_strings[];
WS_VAR_IMPORT const value_string gsm_a_sacch_msg_rr_strings[];
WS_VAR_IMPORT const gchar *gsm_a_pd_str[];

extern const value_string gsm_a_qos_del_of_err_sdu_vals[];
extern const value_string gsm_a_qos_del_order_vals[];
extern const value_string gsm_a_qos_traffic_cls_vals[];
extern const value_string gsm_a_qos_ber_vals[];
extern const value_string gsm_a_qos_sdu_err_rat_vals[];
extern const value_string gsm_a_qos_traff_hdl_pri_vals[];

extern const value_string gsm_a_type_of_number_values[];
extern const value_string gsm_a_numbering_plan_id_values[]; 
extern const value_string gsm_a_sms_vals[];
extern value_string_ext gsm_a_rr_rxlev_vals_ext;

typedef enum
{
	/* Common Information Elements [3] 10.5.1 */
	DE_CELL_ID,				/* Cell Identity */
	DE_CIPH_KEY_SEQ_NUM,	/* Ciphering Key Sequence Number */
	DE_LAI,					/* Location Area Identification */
	DE_MID,					/* Mobile Identity */
	DE_MS_CM_1,				/* Mobile Station Classmark 1 */
	DE_MS_CM_2,				/* Mobile Station Classmark 2 */
	DE_MS_CM_3,				/* Mobile Station Classmark 3 */
	DE_SPARE_NIBBLE,		/* Spare Half Octet */
	DE_D_GB_CALL_REF,		/* Descriptive group or broadcast call reference */
	DE_G_CIPH_KEY_NUM,		/* Group Cipher Key Number */
	DE_PD_SAPI,				/* PD and SAPI $(CCBS)$ */
	DE_PRIO,				/* Priority Level */
	DE_PLMN_LIST,			/* PLMN List */

	DE_COMMON_NONE							/* NONE */
}
common_elem_idx_t;

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
	BE_LCS_CAUSE,	 /* 3.2.2.66 LCS Cause */
	BE_LCS_CLIENT,	 /* 10.14 LCS Client Type */
	BE_APDU,	 /* APDU */
	BE_NE_ID,	 /* Network Element Identity */
	BE_GPS_ASSIST_DATA,	 /* GPS Assistance Data */
	BE_DECIPH_KEYS,	 /* Deciphering Keys */
	BE_RET_ERR_REQ,	 /* Return Error Request */
	BE_RET_ERR_CAUSE,	 /* Return Error Cause */
	BE_SEG,	 /* Segmentation */
	BE_SERV_HO,	/* Service Handover */
	BE_SRC_RNC_TO_TAR_RNC_UMTS,	/* Source RNC to target RNC transparent information (UMTS) */
	BE_SRC_RNC_TO_TAR_RNC_CDMA,	/* Source RNC to target RNC transparent information (cdma2000) */
	BE_GERAN_CLS_M,	/* GERAN Classmark */
	BE_GRAN_BSC_CONT,	/* GERAN BSC Container */
	BE_NEW_BSS_TO_OLD_BSS_INF,	/* New BSS to Old BSS Information */
	BE_INTER_SYS_INF,	/*	Inter-System Information */
	BE_SNA_ACC_INF,		/* SNA Access Information */
	BE_VSTK_RAND_INF,	/* VSTK_RAND Information */
	BE_VSTK_INF,		/* VSTK Information */
	BE_PAGING_INF,		/* Paging Information */
	BE_IMEI,			/* IMEI */
	BE_VEL_EST,			/* Velocity Estimate */
	BE_VGCS_FEAT_FLG,	/* VGCS Feature Flags */
	BE_TALKER_PRI,		/* Talker Priority */
	BE_EMRG_SET_IND,	/* Emergency Set Indication */
	BE_TALKER_ID,		/* Talker Identity */
	BE_CELL_ID_LIST_SEG,/* Cell Identifier List Segment */
	BE_SMS_TO_VGCS,		/* SMS to VGCS */
	BE_VGCS_TALKER_MOD, /*	VGCS Talker Mode */
	BE_VGS_VBS_CELL_STAT, /*	VGCS/VBS Cell Status */
	BE_CELL_ID_LST_SEG_F_EST_CELLS, /*	Cell Identifier List Segment for established cells */
	BE_CELL_ID_LST_SEG_F_CELL_TB_EST,	/* Cell Identifier List Segment for cells to be established */
	BE_CELL_ID_LST_SEG_F_REL_CELL,	/* Cell Identifier List Segment for released cells - no user present */
	BE_CELL_ID_LST_SEG_F_NOT_EST_CELL,	/* Cell Identifier List Segment for not established cells - no establishment possible */
	BE_GANSS_ASS_DTA,	/*	GANSS Assistance Data */
	BE_GANSS_POS_DTA,	/*	GANSS Positioning Data */
	BE_GANSS_LOC_TYP,	/* GANSS Location Type */
	BE_APP_DATA,		/* Application Data */
	BE_DATA_ID,			/* Data Identity */
	BE_APP_DATA_INF,	/*  Application Data Information */
	BE_MSISDN,			/* MSISDN */
	BE_AOIP_TRANS_LAY_ADD,	/*	AoIP Transport Layer Address */
	BE_SPEECH_CODEC_LST,	/* Speech Codec List */
	BE_SPEECH_CODEC,		/* Speech Codec */
	BE_CALL_ID,				/* Call Identifier */
	BE_CALL_ID_LST,			/* Call Identifier List */
	BE_NONE	/* NONE */
}
bssmap_elem_idx_t;

typedef enum
{
	/* BSSMAP LE Elements */
	DE_BMAPLE_LCSQOS,			/* LCS QOS */
	DE_BMAPLE_LCS_PRIO,			/* LCS Priority */
	DE_BMAPLE_LOC_TYPE,			/* Location Type */
	DE_BMAPLE_GANSS_LOC_TYPE,	/* GANSS Location Type */	
	DE_BMAPLE_GEO_LOC,			/* 10.9 Geographic Location */	
	DE_BMAPLE_POS_DATA,			/* Positioning Data */			
	DE_BMAPLE_GANSS_POS_DATA,	/* GANSS Positioning Data */
	DE_BMAPLE_VELOC_DATA,		/* Velocity Data */
	DE_BMAPLE_LCS_CAUSE,		/* LCS Cause */	
	DE_BMAPLE_LCS_CLIENT_TYPE,	/* LCS Client Type */
	DE_BMAPLE_APDU,				/* 10.3 APDU */		
	DE_BMAPLE_NETWORK_ELEM_ID,	/* Network Element Identity */
	DE_BMAPLE_REQ_GPS_ASSIST_D, /* 10.10 Requested GPS Assistance Data */	
	DE_BMAPLE_REQ_GNSS_ASSIST_D,/* Requested GANSS Assistance Data */
	DE_BMAPLE_DECIPH_KEYS,		/* 10.8 Deciphering Keys */
	DE_BMAPLE_RETURN_ERROR_REQ,	/* Return Error Request */
	DE_BMAPLE_RETURN_ERROR_CAUSE,	/* Return Error Cause */
	DE_BMAPLE_SEGMENTATION,		/* Segmentation */
	DE_BMAPLE_CLASSMARK_TYPE_3,	/* Classmark Information Type 3 */
	DE_BMAPLE_CAUSE,			/* 10.4 Cause */
	DE_BMAPLE_CELL_IDENTIFIER,	/* 10.5 Cell Identifier */
	DE_BMAPLE_CHOSEN_CHANNEL,	/* 10.6 Chosen Channel */
	DE_BMAPLE_IMSI,				/* 10.11 IMSI */
	DE_BMAPLE_RES1,				/* Reserved */
	DE_BMAPLE_RES2,				/* Reserved */
	DE_BMAPLE_RES3,				/* Reserved */
	DE_BMAPLE_LCS_CAPABILITY,	/* LCS Capability */
	DE_BMAPLE_PACKET_MEAS_REP,	/* Packet Measurement Report */
	DE_BMAPLE_MEAS_CELL_ID,		/* Measured Cell Identity */
	DE_BMAPLE_IMEI,				/* IMEI */
	BMAPLE_NONE					/* NONE */
}
bssmap_le_elem_idx_t;

typedef enum
{
	/* BSS LAP Elements 5 */
	DE_BLAP_RES1,			/* Reserved */
	DE_BLAP_TA,				/* Timing Advance */
	DE_BLAP_RES3,			/* Reserved */			/* (note) */
	DE_BLAP_RES4,			/* Cell Identity */	
	DE_BLAP_RES5,			/* Reserved */			/* (note) */
	DE_BLAP_RES6,			/* Reserved */			/* (note) */
	DE_BLAP_RES7,			/* Reserved */			/* (note) */
	DE_BLAP_CH_DESC,		/* Channel Description */
	DE_BLAP_RES9,			/* Reserved */			/* (note) */
	DE_BLAP_RES10,			/* Reserved */			/* (note) */
	DE_BLAP_RES11,			/* Reserved */			/* (note) */
	DE_BLAP_MEAS_REP,		/* Measurement Report */
	DE_BLAP_RES13,			/* Reserved */			/* (note) */
	DE_BLAP_CAUSE,			/* Cause */
	DE_BLAP_RRLP_FLG,		/* RRLP Flag */
	DE_BLAP_RRLP_IE,		/* RRLP IE */
	DE_BLAP_CELL_ID_LIST,	/* Cell Identity List */
	DE_BLAP_ENH_MEAS_REP,	/* Enhanced Measurement Report */
	DE_BLAP_LAC,			/* Location Area Code */
	DE_BLAP_FREQ_LIST,		/* Frequency List */
	DE_BLAP_MS_POW,			/* MS Power */
	DE_BLAP_DELTA_TIME,		/* Delta Timer */
	DE_BLAP_SERV_CELL_ID,	/* Serving Cell Identifier */
	DE_BLAP_ENC_KEY,		/* Encryption Key (Kc) */
	DE_BLAP_CIP_M_SET,		/* Cipher Mode Setting */
	DE_BLAP_CH_MODE,		/* Channel Mode */
	DE_BLAP_POLL_REP,		/* Polling Repetition */
	DE_BLAP_PKT_CH_DESC,	/* Packet Channel Description */
	DE_BLAP_TLLI,			/* TLLI */
	DE_BLAP_TFI,			/* TFI */
	DE_BLAP_START_TIME,		/* Starting Time */
	BSSLAP_NONE				/* NONE */
}
bsslap_elem_idx_t;

typedef enum
{
	/* Mobility Management Information Elements [3] 10.5.3 */
	DE_AUTH_PARAM_RAND,				/* Authentication Parameter RAND */
	DE_AUTH_PARAM_AUTN,				/* Authentication Parameter AUTN (UMTS authentication challenge only) */
	DE_AUTH_RESP_PARAM,				/* Authentication Response Parameter */
	DE_AUTH_RESP_PARAM_EXT,			/* Authentication Response Parameter (extension) (UMTS authentication challenge only) */
	DE_AUTH_FAIL_PARAM,				/* Authentication Failure Parameter (UMTS authentication challenge only) */
	DE_CM_SRVC_TYPE,				/* CM Service Type */
	DE_ID_TYPE,						/* Identity Type */
	DE_LOC_UPD_TYPE,				/* Location Updating Type */
	DE_NETWORK_NAME,				/* Network Name */
	DE_REJ_CAUSE,					/* Reject Cause */
	DE_FOP,							/* Follow-on Proceed */
	DE_TIME_ZONE,					/* Time Zone */
	DE_TIME_ZONE_TIME,				/* Time Zone and Time */
	DE_CTS_PERM,					/* CTS Permission */
	DE_LSA_ID,						/* LSA Identifier */
	DE_DAY_SAVING_TIME,				/* Daylight Saving Time */
	DE_EMERGENCY_NUM_LIST,			/* Emergency Number List */
	/* Call Control Information Elements 10.5.4 */
	DE_AUX_STATES,					/* Auxiliary States */
	DE_BEARER_CAP,					/* Bearer Capability */
	DE_CC_CAP,						/* Call Control Capabilities */
	DE_CALL_STATE,					/* Call State */
	DE_CLD_PARTY_BCD_NUM,			/* Called Party BCD Number */
	DE_CLD_PARTY_SUB_ADDR,			/* Called Party Subaddress */
	DE_CLG_PARTY_BCD_NUM,			/* Calling Party BCD Number */
	DE_CLG_PARTY_SUB_ADDR,			/* Calling Party Subaddress */
	DE_CAUSE,						/* Cause */
	DE_CLIR_SUP,					/* CLIR Suppression */
	DE_CLIR_INV,					/* CLIR Invocation */
	DE_CONGESTION,					/* Congestion Level */
	DE_CONN_NUM,					/* Connected Number */
	DE_CONN_SUB_ADDR,				/* Connected Subaddress */
	DE_FACILITY,					/* Facility */
	DE_HLC,							/* High Layer Compatibility */
	DE_KEYPAD_FACILITY,				/* Keypad Facility */
	DE_LLC,							/* Low Layer Compatibility */
	DE_MORE_DATA,					/* More Data */
	DE_NOT_IND,						/* Notification Indicator */
	DE_PROG_IND,					/* Progress Indicator */
	DE_RECALL_TYPE,					/* Recall type $(CCBS)$ */
	DE_RED_PARTY_BCD_NUM,			/* Redirecting Party BCD Number */
	DE_RED_PARTY_SUB_ADDR,			/* Redirecting Party Subaddress */
	DE_REPEAT_IND,					/* Repeat Indicator */
	DE_REV_CALL_SETUP_DIR,			/* Reverse Call Setup Direction */
	DE_SETUP_CONTAINER,				/* SETUP Container $(CCBS)$ */
	DE_SIGNAL,						/* Signal */
	DE_SS_VER_IND,					/* SS Version Indicator */
	DE_USER_USER,					/* User-user */
	DE_ALERT_PATTERN,				/* Alerting Pattern $(NIA)$ */
	DE_ALLOWED_ACTIONS,				/* Allowed Actions $(CCBS)$ */
	DE_SI,							/* Stream Identifier */
	DE_NET_CC_CAP,					/* Network Call Control Capabilities */
	DE_CAUSE_NO_CLI,				/* Cause of No CLI */
	DE_SUP_CODEC_LIST,				/* Supported Codec List */
	DE_SERV_CAT,					/* Service Category */
	DE_REDIAL,						/* 10.5.4.34 Redial */
	DE_NET_INIT_SERV_UPG,			/* 10.5.4.35 Network-initiated Service Upgrade ind */
	/* Short Message Service Information Elements [5] 8.1.4 */
	DE_CP_USER_DATA,				/* CP-User Data */
	DE_CP_CAUSE,					/* CP-Cause */
	/* Tests procedures information elements 3GPP TS 44.014 6.4.0 and 3GPP TS 34.109 6.4.0 */
	DE_TP_SUB_CHANNEL,			/* Close TCH Loop Cmd Sub-channel */
	DE_TP_ACK,			/* Open Loop Cmd Ack */
	DE_TP_LOOP_TYPE,			/* Close Multi-slot Loop Cmd Loop type*/
	DE_TP_LOOP_ACK,			/* Close Multi-slot Loop Ack Result */
	DE_TP_TESTED_DEVICE,			/* Test Interface Tested device */
	DE_TP_PDU_DESCRIPTION,			/* GPRS Test Mode Cmd PDU description */
	DE_TP_MODE_FLAG,			/* GPRS Test Mode Cmd Mode flag */
	DE_TP_EGPRS_MODE_FLAG,			/* EGPRS Start Radio Block Loopback Cmd Mode flag */
	DE_TP_UE_TEST_LOOP_MODE,			/* Close UE Test Loop Mode */
	DE_TP_UE_POSITIONING_TECHNOLOGY,			/* UE Positioning Technology */
	DE_TP_RLC_SDU_COUNTER_VALUE,			/* RLC SDU Counter Value */
	DE_TP_EPC_UE_TEST_LOOP_MODE,	/* UE Test Loop Mode */
	DE_TP_EPC_UE_TL_A_LB_SETUP,		/* UE Test Loop Mode A LB Setup */
	DE_TP_EPC_UE_TL_B_LB_SETUP,		/* UE Test Loop Mode B LB Setup */
	DE_NONE							/* NONE */
}
dtap_elem_idx_t;

typedef enum
{
	/* GPRS Mobility Management Information Elements [3] 10.5.5 */
	DE_ATTACH_RES,					/* [7] 10.5.1 Attach Result*/
	DE_ATTACH_TYPE,					/* [7] 10.5.2 Attach Type */
	DE_CIPH_ALG,					/* [7] 10.5.3 Cipher Algorithm */
	DE_TMSI_STAT,					/* [7] 10.5.4 TMSI Status */
	DE_DETACH_TYPE,					/* [7] 10.5.5 Detach Type */
	DE_DRX_PARAM,					/* [7] 10.5.6 DRX Parameter */
	DE_FORCE_TO_STAND,				/* [7] 10.5.7 Force to Standby */
	DE_FORCE_TO_STAND_H,			/* [7] 10.5.8 Force to Standby - Info is in the high nibble */
	DE_P_TMSI_SIG,					/* [7] 10.5.9 P-TMSI Signature */
	DE_P_TMSI_SIG_2,				/* [7] 10.5.10 P-TMSI Signature 2 */
	DE_ID_TYPE_2,					/* [7] 10.5.11 Identity Type 2 */
	DE_IMEISV_REQ,					/* [7] 10.5.12 IMEISV Request */
	DE_REC_N_PDU_NUM_LIST,			/* [7] 10.5.13 Receive N-PDU Numbers List */
	DE_MS_NET_CAP,					/* [7] 10.5.14 MS Network Capability */
	DE_MS_RAD_ACC_CAP,				/* [7] 10.5.15 MS Radio Access Capability */
	DE_GMM_CAUSE,					/* [7] 10.5.16 GMM Cause */
	DE_RAI,							/* [7] 10.5.17 Routing Area Identification */
	DE_UPD_RES,						/* [7] 10.5.18 Update Result */
	DE_UPD_TYPE,					/* [7] 10.5.19 Update Type */
	DE_AC_REF_NUM,					/* [7] 10.5.20 A&C Reference Number */
	DE_AC_REF_NUM_H,				/* A&C Reference Number - Info is in the high nibble */
	DE_SRVC_TYPE,					/* [7] 10.5.20 Service Type */
	DE_CELL_NOT,					/* [7] 10.5.21 Cell Notification */
	DE_PS_LCS_CAP,					/* [7] 10.5.22 PS LCS Capability */
	DE_NET_FEAT_SUP,				/* [7] 10.5.23 Network Feature Support */
	DE_RAT_INFO_CONTAINER,			/* [7] 10.5.24 Inter RAT information container */
	/* [7] 10.5.25 Requested MS information */
	/* Session Management Information Elements [3] 10.5.6 */
	DE_ACC_POINT_NAME,				/* Access Point Name */
	DE_NET_SAPI,					/* Network Service Access Point Identifier */
	DE_PRO_CONF_OPT,				/* Protocol Configuration Options */
	DE_PD_PRO_ADDR,					/* Packet Data Protocol Address */
	DE_QOS,							/* Quality Of Service */
	DE_SM_CAUSE,					/* SM Cause */
	DE_SM_CAUSE_2,					/* SM Cause 2 */
	DE_LINKED_TI,					/* Linked TI */
	DE_LLC_SAPI,					/* LLC Service Access Point Identifier */
	DE_TEAR_DOWN_IND,				/* Tear Down Indicator */
	DE_PACKET_FLOW_ID,				/* Packet Flow Identifier */
	DE_TRAFFIC_FLOW_TEMPLATE,		/* Traffic Flow Template */
	DE_TMGI,						/* Temporary Mobile Group Identity (TMGI) */
	DE_MBMS_BEARER_CAP,				/* MBMS bearer capabilities */
	DE_MBMS_PROT_CONF_OPT,			/* MBMS protocol configuration options */
	DE_ENH_NSAPI,					/* Enhanced network service access point identifier */
	DE_REQ_TYPE,					/* Request type */
	/* GPRS Common Information Elements [8] 10.5.7 */
	DE_PDP_CONTEXT_STAT,			/* [8] 10.5.7.1		PDP Context Status */
	DE_RAD_PRIO,					/* [8] 10.5.7.2		Radio Priority */
	DE_GPRS_TIMER,					/* [8] 10.5.7.3		GPRS Timer */
	DE_GPRS_TIMER_2,				/* [8] 10.5.7.4		GPRS Timer 2 */
	DE_RAD_PRIO_2,					/* [8] 10.5.7.5		Radio Priority 2 */
	DE_MBMS_CTX_STATUS,				/* [8] 10.5.7.6		MBMS context status */
	DE_GM_NONE							/* NONE */
}
gm_elem_idx_t;

typedef enum
{
	/* Radio Resource Management Information Elements 10.5.2, most are from 10.5.1	*/
	DE_RR_BA_RANGE,				   /* [3]  10.5.2.1a	BA Range */
	DE_RR_CELL_CH_DSC,			   /* [3]  10.5.2.1b	Cell Channel Description	*/
	DE_RR_BA_LIST_PREF,			   /* [3]  10.5.2.1c	BA List Pref */
	DE_RR_UTRAN_FREQ_LIST,		   /* [3]  10.5.2.1d	UTRAN Frequency List */
	DE_RR_CELL_SELECT_INDIC,	   /* [3]  10.5.2.1e	Cell selection indicator after release of all TCH and SDCCH IE */
	DE_RR_CELL_DSC,					/* 10.5.2.2   RR Cell Description				*/
	DE_RR_CELL_OPT_BCCH,				/* [3]  10.5.2.3	Cell Options (BCCH)		*/
	DE_RR_CELL_OPT_SACCH,				/* [3]  10.5.2.3a	Cell Options (SACCH)		*/
	DE_RR_CELL_SEL_PARAM,				/* [3]  10.5.2.4	Cell Selection Parameters		*/
/*
 * [3]  10.5.2.4a	(void)
 */
	DE_RR_CH_DSC,					/* [3]  10.5.2.5	Channel Description			*/
	DE_RR_CH_DSC2,					/* [3]  10.5.2.5a   Channel Description 2 		*/
	DE_RR_CH_DSC3,					/* [3]  10.5.2.5c   Channel Description 3 		*/
	DE_RR_CH_MODE,					/* [3]  10.5.2.6	Channel Mode				*/
	DE_RR_CH_MODE2,					/* [3]  10.5.2.7	Channel Mode 2				*/
	DE_RR_UTRAN_CM,					/* [3]  10.5.2.7a	UTRAN Classmark */
/* [3]  10.5.2.7b	(void) */
	DE_RR_CM_ENQ_MASK,				/* [3]  10.5.2.7c	Classmark Enquiry Mask		*/
/* [3]  10.5.2.7d	GERAN Iu Mode Classmark information element						*/
	DE_RR_CHNL_NEEDED,				/* [3]  10.5.2.8	Channel Needed
 * [3]  10.5.2.8a	(void)
 * [3]  10.5.2.8b	Channel Request Description 2 */
	DE_RR_CIP_MODE_SET,				/* [3]  10.5.2.9	Cipher Mode Setting			*/
	DE_RR_CIP_MODE_RESP,			/* [3]  10.5.2.10	Cipher Response			 */
	DE_RR_CTRL_CH_DESC,				/* [3]  10.5.2.11	Control Channel Description	*/
/* [3]  10.5.2.11a	DTM Information Details */
	DE_RR_DYN_ARFCN_MAP,			/* [3]  10.5.2.11b	Dynamic ARFCN Mapping		*/
	DE_RR_FREQ_CH_SEQ,				/* [3]  10.5.2.12	Frequency Channel Sequence	*/
	DE_RR_FREQ_LIST,				/* [3]  10.5.2.13	Frequency List				*/
	DE_RR_FREQ_SHORT_LIST,			/* [3]  10.5.2.14	Frequency Short List		*/
	DE_RR_FREQ_SHORT_LIST2,			/* [3]  10.5.2.14a	Frequency Short List 2		*/
/* [3]  10.5.2.14b	Group Channel Description */
	DE_RR_GPRS_RESUMPTION,			/* [3]  10.5.2.14c	GPRS Resumption */
	DE_RR_GPRS_BROADCAST_INFORMATION,			/* [3]  10.5.2.14d	GPRS broadcast information */
/* [3]  10.5.2.14e	Enhanced DTM CS Release Indication*/

	DE_RR_HO_REF,					/* 10.5.2.15  Handover Reference				*/

	DE_RR_IA_REST_OCT,				/* [3] 10.5.2.16 IA Rest Octets				*/
	DE_RR_IAR_REST_OCT,				/* [3] 10.5.2.17 IAR Rest Octets				*/
	DE_RR_IAX_REST_OCT,				/* [3] 10.5.2.18 IAX Rest Octets				*/
	DE_RR_L2_PSEUDO_LEN,			/* [3] 10.5.2.19 L2 Pseudo Length				*/
	DE_RR_MEAS_RES,					/* [3] 10.5.2.20 Measurement Results		*/
 /* [3] 10.5.2.20a GPRS Measurement Results */
	DE_RR_MOB_ALL,					/* [3] 10.5.2.21 Mobile Allocation				*/
	DE_RR_MOB_TIME_DIFF,			/* [3] 10.5.2.21a Mobile Time Difference		*/
	DE_RR_MULTIRATE_CONF,			/* [3] 10.5.2.21aa MultiRate configuration		*/
	DE_RR_MULT_ALL,					/* [3] 10.5.2.21b Multislot Allocation			*/
/*
 * [3] 10.5.2.21c NC mode
 */
	DE_RR_NEIGH_CELL_DESC,				/* [3] 10.5.2.22 Neighbour Cell Description	*/
	DE_RR_NEIGH_CELL_DESC2,				/* [3] 10.5.2.22a Neighbour Cell Description 2	*/
/*
 * [3] 10.5.2.22b (void)
 * [3] 10.5.2.22c NT/N Rest Octets */
	DE_RR_P1_REST_OCT,					/* [3] 10.5.2.23 P1 Rest Octets */
	DE_RR_P2_REST_OCT,					/* [3] 10.5.2.24 P2 Rest Octets */
	DE_RR_P3_REST_OCT,					/* [3] 10.5.2.25 P3 Rest Octets */
	DE_RR_PACKET_CH_DESC,				/* [3] 10.5.2.25a Packet Channel Description	*/
	DE_RR_DED_MOD_OR_TBF,			/* [3] 10.5.2.25b Dedicated mode or TBF			*/
/* [3] 10.5.2.25c RR Packet Uplink Assignment
 * [3] 10.5.2.25d RR Packet Downlink Assignment */
	DE_RR_PAGE_MODE,				/* [3] 10.5.2.26 Page Mode						*/
/* [3] 10.5.2.26a (void)
 * [3] 10.5.2.26b (void)
 * [3] 10.5.2.26c (void)
 * [3] 10.5.2.26d (void)
 */
	DE_RR_NCC_PERM,					/* [3] 10.5.2.27 NCC Permitted */
	DE_RR_POW_CMD,					/* 10.5.2.28  Power Command						*/
	DE_RR_POW_CMD_AND_ACC_TYPE,		/* 10.5.2.28a Power Command and access type		*/
	DE_RR_RACH_CTRL_PARAM,			/* [3] 10.5.2.29 RACH Control Parameters */
	DE_RR_REQ_REF,					/* [3] 10.5.2.30 Request Reference				*/
	DE_RR_CAUSE,					/* 10.5.2.31  RR Cause							*/
	DE_RR_SYNC_IND,					/* 10.5.2.39  Synchronization Indication		*/
	DE_RR_SI1_REST_OCT,				/* [3] 10.5.2.32 SI1 Rest Octets */
/* [3] 10.5.2.33 SI 2bis Rest Octets */
	DE_RR_SI2TER_REST_OCT,			/* [3] 10.5.2.33a SI 2ter Rest Octets */
	DE_RR_SI2QUATER_REST_OCT,		/* [3] 10.5.2.33b SI 2quater Rest Octets */
	DE_RR_SI3_REST_OCT,				/* [3] 10.5.2.34 SI3 Rest Octets */
	DE_RR_SI4_REST_OCT,				/* [3] 10.5.2.35 SI4 Rest Octets */
	DE_RR_SI6_REST_OCT,				/* [3] 10.5.2.35a SI6 Rest Octets */
/* [3] 10.5.2.36 SI 7 Rest Octets
 * [3] 10.5.2.37 SI 8 Rest Octets
 * [3] 10.5.2.37a SI 9 Rest Octets
 */
	DE_RR_SI13_REST_OCT,				/* [3] 10.5.2.37b SI13 Rest Octets */
/* [3] 10.5.2.37c (void)
 * [3] 10.5.2.37d (void)
 * [3] 10.5.2.37e SI 16 Rest Octets
 * [3] 10.5.2.37f SI 17 Rest Octets
 * [3] 10.5.2.37g SI 19 Rest Octets
 * [3] 10.5.2.37h SI 18 Rest Octets
 * [3] 10.5.2.37i SI 20 Rest Octets */
	DE_RR_STARTING_TIME,			/* [3] 10.5.2.38 Starting Time					*/
	DE_RR_TIMING_ADV,				/* [3] 10.5.2.40 Timing Advance					*/
	DE_RR_TIME_DIFF,				/* [3] 10.5.2.41 Time Difference				*/
	DE_RR_TLLI,						/* [3] 10.5.2.41a TLLI							*/
	DE_RR_TMSI_PTMSI,				/* [3] 10.5.2.42 TMSI/P-TMSI */
	DE_RR_VGCS_TAR_MODE_IND,		/* [3] 10.5.2.42a VGCS target mode Indication	*/
	DE_RR_VGCS_CIP_PAR,				/* [3] 10.5.2.42b	VGCS Ciphering Parameters	*/

	DE_RR_WAIT_IND,					/* [3] 10.5.2.43 Wait Indication */
/* [3] 10.5.2.44 SI10 rest octets $(ASCI)$ */
	DE_RR_EXT_MEAS_RESULT,     	/* [3] 10.5.2.45 Extended Measurement Results */
	DE_RR_EXT_MEAS_FREQ_LIST,		/* [3] 10.5.2.46 Extended Measurement Frequency List */
	DE_RR_SUS_CAU,					/* [3] 10.5.2.47 Suspension Cause				*/
	DE_RR_APDU_ID,					/* [3] 10.5.2.48 APDU ID */
	DE_RR_APDU_FLAGS,				/* [3] 10.5.2.49 APDU Flags */
	DE_RR_APDU_DATA,				/* [3] 10.5.2.50 APDU Data */
	DE_RR_HO_TO_UTRAN_CMD,			/* [3] 10.5.2.51 Handover To UTRAN Command */
/* [3] 10.5.2.52 Handover To cdma2000 Command
 * [3] 10.5.2.53 (void)
 * [3] 10.5.2.54 (void)
 * [3] 10.5.2.55 (void)
 * [3] 10.5.2.56 3G Target Cell */
	DE_RR_SERV_SUP,					/* 10.5.2.57 Service Support						*/
/* 10.5.2.58 MBMS p-t-m Channel Description
 */

	DE_RR_DED_SERV_INF,				/* [3] 10.5.2.59	Dedicated Service Information */

/*
 * 10.5.2.60 MPRACH Description
 * 10.5.2.61 Restriction Timer
 * 10.5.2.62 MBMS Session Identity
 * 10.5.2.63 Reduced group or broadcast call reference
 * 10.5.2.64 Talker Priority status
 * 10.5.2.65 Talker Identity
 * 10.5.2.66 Token
 * 10.5.2.67 PS Cause
 * 10.5.2.68 VGCS AMR Configuration
 */
	DE_RR_CARRIER_IND,				/* 10.5.2.69 Carrier Indication */
	DE_RR_NONE							/* NONE */
}
rr_elem_idx_t;

typedef enum
{
    /* 9.9.3    EPS Mobility Management (EMM) information elements */
    DE_EMM_ADD_UPD_RES,         /* 9.9.3.0A Additional update result */
    DE_EMM_ADD_UPD_TYPE,        /* 9.9.3.0B Additional update type */
    DE_EMM_AUTH_FAIL_PAR,       /* 9.9.3.1  Authentication failure parameter (dissected in packet-gsm_a_dtap.c)*/
    DE_EMM_AUTN,                /* 9.9.3.2  Authentication parameter AUTN */
    DE_EMM_AUTH_PAR_RAND,       /* 9.9.3.3  Authentication parameter RAND */
    DE_EMM_AUTH_RESP_PAR,       /* 9.9.3.4  Authentication response parameter */
    DE_EMM_CSFB_RESP,           /* 9.9.3.5  CSFB response */
    DE_EMM_DAYL_SAV_T,          /* 9.9.3.6  Daylight saving time */
    DE_EMM_DET_TYPE,            /* 9.9.3.7  Detach type */
    DE_EMM_DRX_PAR,             /* 9.9.3.8  DRX parameter (dissected in packet-gsm_a_gm.c)*/
    DE_EMM_CAUSE,               /* 9.9.3.9  EMM cause */
    DE_EMM_ATT_RES,             /* 9.9.3.10 EPS attach result (Coded inline */
    DE_EMM_ATT_TYPE,            /* 9.9.3.11 EPS attach type (Coded Inline)*/
    DE_EMM_EPS_MID,             /* 9.9.3.12 EPS mobile identity */
    DE_EMM_EPS_NET_FEATURE_SUP, /* 9.9.3.12A EPS network feature support */
    DE_EMM_EPS_UPD_RES,         /* 9.9.3.13 EPS update result ( Coded inline)*/
    DE_EMM_EPS_UPD_TYPE,        /* 9.9.3.14 EPS update type */
    DE_EMM_ESM_MSG_CONT,        /* 9.9.3.15 ESM message conta */
    DE_EMM_GPRS_TIMER,          /* 9.9.3.16 GPRS timer ,See subclause 10.5.7.3 in 3GPP TS 24.008 [6]. */
    DE_EMM_ID_TYPE_2,           /* 9.9.3.17 Identity type 2 ,See subclause 10.5.5.9 in 3GPP TS 24.008 [6]. */
    DE_EMM_IMEISV_REQ,          /* 9.9.3.18 IMEISV request ,See subclause 10.5.5.10 in 3GPP TS 24.008 [6]. */
    DE_EMM_KSI_AND_SEQ_NO,      /* 9.9.3.19 KSI and sequence number */
    DE_EMM_MS_NET_CAP,          /* 9.9.3.20 MS network capability ,See subclause 10.5.5.12 in 3GPP TS 24.008 [6]. */
    DE_EMM_NAS_KEY_SET_ID,      /* 9.9.3.21 NAS key set identifier (coded inline)*/
    DE_EMM_NAS_MSG_CONT,        /* 9.9.3.22 NAS message container */
    DE_EMM_NAS_SEC_ALGS,        /* 9.9.3.23 NAS security algorithms */
    DE_EMM_NET_NAME,            /* 9.9.3.24 Network name, See subclause 10.5.3.5a in 3GPP TS 24.008 [6]. */
    DE_EMM_NONCE,               /* 9.9.3.25 Nonce */
    DE_EMM_PAGING_ID,           /* 9.9.3.25A Paging identity */
    DE_EMM_P_TMSI_SIGN,         /* 9.9.3.26 P-TMSI signature, See subclause 10.5.5.8 in 3GPP TS 24.008 [6]. */
    DE_EMM_SERV_TYPE,           /* 9.9.3.27 Service type */
    DE_EMM_SHORT_MAC,           /* 9.9.3.28 Short MAC */
    DE_EMM_TZ,                  /* 9.9.3.29 Time zone, See subclause 10.5.3.8 in 3GPP TS 24.008 [6]. */
    DE_EMM_TZ_AND_T,            /* 9.9.3.30 Time zone and time, See subclause 10.5.3.9 in 3GPP TS 24.008 [6]. */
    DE_EMM_TMSI_STAT,           /* 9.9.3.31 TMSI status, See subclause 10.5.5.4 in 3GPP TS 24.008 [6]. */
    DE_EMM_TRAC_AREA_ID,        /* 9.9.3.32 Tracking area identity */
    DE_EMM_TRAC_AREA_ID_LST,    /* 9.9.3.33 Tracking area identity list */
    DE_EMM_UE_NET_CAP,          /* 9.9.3.34 UE network capability */
    DE_EMM_UE_RA_CAP_INF_UPD_NEED,  /* 9.9.3.35 UE radio capability information update needed */
    DE_EMM_UE_SEC_CAP,          /* 9.9.3.36 UE security capability */
    DE_EMM_EMERG_NUM_LST,       /* 9.9.3.37 Emergency Number List */
    DE_EMM_CLI,                 /* 9.9.3.38 CLI */
    DE_EMM_SS_CODE,             /* 9.9.3.39 SS Code */
    DE_EMM_LCS_IND,             /* 9.9.3.40 LCS indicator */
    DE_EMM_LCS_CLIENT_ID,       /* 9.9.3.41 LCS client identity */
    DE_EMM_GEN_MSG_CONT_TYPE,   /* 9.9.3.42 Generic message container type */
    DE_EMM_GEN_MSG_CONT,        /* 9.9.3.43 Generic message container */
    DE_EMM_VOICE_DMN_PREF,      /* 9.9.3.44 Voice domain preference and UE's usage setting */
    DE_EMM_NONE                 /* NONE */

}
nas_emm_elem_idx_t;

#endif /* __PACKET_GSM_A_COMMON_H__ */
