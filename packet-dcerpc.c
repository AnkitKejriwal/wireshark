/* packet-dcerpc.c
 * Routines for DCERPC packet disassembly
 * Copyright 2001, Todd Sabin <tas@webspan.net>
 * Copyright 2003, Tim Potter <tpot@samba.org>
 *
 * $Id: packet-dcerpc.c,v 1.168 2004/05/07 11:07:53 ulfl Exp $
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@ethereal.com>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <ctype.h>

#include <glib.h>
#include <epan/packet.h>
#include "packet-dcerpc.h"
#include <epan/conversation.h>
#include "prefs.h"
#include "reassemble.h"
#include "tap.h"
#include "packet-frame.h"
#include "packet-dcerpc-nt.h"

static int dcerpc_tap = -1;


static const value_string pckt_vals[] = {
    { PDU_REQ,        "Request"},
    { PDU_PING,       "Ping"},
    { PDU_RESP,       "Response"},
    { PDU_FAULT,      "Fault"},
    { PDU_WORKING,    "Working"},
    { PDU_NOCALL,     "Nocall"},
    { PDU_REJECT,     "Reject"},
    { PDU_ACK,        "Ack"},
    { PDU_CL_CANCEL,  "Cl_cancel"},
    { PDU_FACK,       "Fack"},
    { PDU_CANCEL_ACK, "Cancel_ack"},
    { PDU_BIND,       "Bind"},
    { PDU_BIND_ACK,   "Bind_ack"},
    { PDU_BIND_NAK,   "Bind_nak"},
    { PDU_ALTER,      "Alter_context"},
    { PDU_ALTER_ACK,  "Alter_context_resp"},
    { PDU_AUTH3,      "AUTH3"},
    { PDU_SHUTDOWN,   "Shutdown"},
    { PDU_CO_CANCEL,  "Co_cancel"},
    { PDU_ORPHANED,   "Orphaned"},
    { 0,              NULL }
};

static const value_string drep_byteorder_vals[] = {
    { 0, "Big-endian" },
    { 1, "Little-endian" },
    { 0,  NULL }
};

static const value_string drep_character_vals[] = {
    { 0, "ASCII" },
    { 1, "EBCDIC" },
    { 0,  NULL }
};

#define DCE_RPC_DREP_FP_IEEE 0
#define DCE_RPC_DREP_FP_VAX  1
#define DCE_RPC_DREP_FP_CRAY 2
#define DCE_RPC_DREP_FP_IBM  3

static const value_string drep_fp_vals[] = {
    { DCE_RPC_DREP_FP_IEEE, "IEEE" },
    { DCE_RPC_DREP_FP_VAX,  "VAX"  },
    { DCE_RPC_DREP_FP_CRAY, "Cray" },
    { DCE_RPC_DREP_FP_IBM,  "IBM"  },
    { 0,  NULL }
};

/*
 * Authentication services.
 */
static const value_string authn_protocol_vals[] = {
	{ DCE_C_RPC_AUTHN_PROTOCOL_NONE,    "None" },
	{ DCE_C_RPC_AUTHN_PROTOCOL_KRB5,    "Kerberos 5" },
	{ DCE_C_RPC_AUTHN_PROTOCOL_SPNEGO,  "SPNEGO" },
	{ DCE_C_RPC_AUTHN_PROTOCOL_NTLMSSP, "NTLMSSP" },
	{ DCE_C_RPC_AUTHN_PROTOCOL_SEC_CHAN,"NETLOGON Secure Channel" },
	{ 0, NULL }
};

/*
 * Protection levels.
 */
static const value_string authn_level_vals[] = {
	{ DCE_C_AUTHN_LEVEL_NONE,          "None" },
	{ DCE_C_AUTHN_LEVEL_CONNECT,       "Connect" },
	{ DCE_C_AUTHN_LEVEL_CALL,          "Call" },
	{ DCE_C_AUTHN_LEVEL_PKT,           "Packet" },
	{ DCE_C_AUTHN_LEVEL_PKT_INTEGRITY, "Packet integrity" },
	{ DCE_C_AUTHN_LEVEL_PKT_PRIVACY,   "Packet privacy" },
	{ 0,                               NULL }
};

/*
 * Flag bits in first flag field in connectionless PDU header.
 */
#define PFCL1_RESERVED_01	0x01	/* Reserved for use by implementations */
#define PFCL1_LASTFRAG		0x02	/* If set, the PDU is the last
					 * fragment of a multi-PDU
					 * transmission */
#define PFCL1_FRAG		0x04	/* If set, the PDU is a fragment of
					   a multi-PDU transmission */
#define PFCL1_NOFACK		0x08	/* If set, the receiver is not
					 * requested to send a `fack' PDU
					 * for the fragment */
#define PFCL1_MAYBE		0x10	/* If set, the PDU is for a `maybe'
					 * request */
#define PFCL1_IDEMPOTENT	0x20	/* If set, the PDU is for an idempotent
					 * request */
#define PFCL1_BROADCAST		0x40	/* If set, the PDU is for a broadcast
					 * request */
#define PFCL1_RESERVED_80	0x80	/* Reserved for use by implementations */

/*
 * Flag bits in second flag field in connectionless PDU header.
 */
#define PFCL2_RESERVED_01	0x01	/* Reserved for use by implementations */
#define PFCL2_CANCEL_PENDING	0x02	/* Cancel pending at the call end */
#define PFCL2_RESERVED_04	0x04	/* Reserved for future use */
#define PFCL2_RESERVED_08	0x08	/* Reserved for future use */
#define PFCL2_RESERVED_10	0x10	/* Reserved for future use */
#define PFCL2_RESERVED_20	0x20	/* Reserved for future use */
#define PFCL2_RESERVED_40	0x40	/* Reserved for future use */
#define PFCL2_RESERVED_80	0x80	/* Reserved for future use */

/*
 * Flag bits in connection-oriented PDU header.
 */
#define PFC_FIRST_FRAG		0x01	/* First fragment */
#define PFC_LAST_FRAG		0x02	/* Last fragment */
#define PFC_PENDING_CANCEL	0x04	/* Cancel was pending at sender */
#define PFC_RESERVED_1		0x08
#define PFC_CONC_MPX		0x10	/* suports concurrent multiplexing
					 * of a single connection. */
#define PFC_DID_NOT_EXECUTE	0x20	/* only meaningful on `fault' packet;
					 * if true, guaranteed call did not
					 * execute. */
#define PFC_MAYBE		0x40	/* `maybe' call semantics requested */
#define PFC_OBJECT_UUID		0x80	/* if true, a non-nil object UUID
					 * was specified in the handle, and
					 * is present in the optional object
					 * field. If false, the object field
					 * is omitted. */

/*
 * Tests whether a connection-oriented PDU is fragmented; returns TRUE if
 * it's not fragmented (i.e., this is both the first *and* last fragment),
 * and FALSE otherwise.
 */
#define PFC_NOT_FRAGMENTED(hdr) \
  ((hdr->flags&(PFC_FIRST_FRAG|PFC_LAST_FRAG))==(PFC_FIRST_FRAG|PFC_LAST_FRAG))

/*
 * Presentation context negotiation result.
 */
static const value_string p_cont_result_vals[] = {
	{ 0, "Acceptance" },
	{ 1, "User rejection" },
	{ 2, "Provider rejection" },
	{ 0, NULL }
};

/*
 * Presentation context negotiation rejection reasons.
 */
static const value_string p_provider_reason_vals[] = {
	{ 0, "Reason not specified" },
	{ 1, "Abstract syntax not supported" },
	{ 2, "Proposed transfer syntaxes not supported" },
	{ 3, "Local limit exceeded" },
	{ 0, NULL }
};

/*
 * Reject reasons.
 */
#define REASON_NOT_SPECIFIED		0
#define TEMPORARY_CONGESTION		1
#define LOCAL_LIMIT_EXCEEDED		2
#define CALLED_PADDR_UNKNOWN		3 /* not used */
#define PROTOCOL_VERSION_NOT_SUPPORTED	4
#define DEFAULT_CONTEXT_NOT_SUPPORTED	5 /* not used */
#define USER_DATA_NOT_READABLE		6 /* not used */
#define NO_PSAP_AVAILABLE		7 /* not used */

static const value_string reject_reason_vals[] = {
	{ REASON_NOT_SPECIFIED,           "Reason not specified" },
	{ TEMPORARY_CONGESTION,           "Temporary congestion" },
	{ LOCAL_LIMIT_EXCEEDED,           "Local limit exceeded" },
	{ CALLED_PADDR_UNKNOWN,           "Called paddr unknown" },
	{ PROTOCOL_VERSION_NOT_SUPPORTED, "Protocol version not supported" },
	{ DEFAULT_CONTEXT_NOT_SUPPORTED,  "Default context not supported" },
	{ USER_DATA_NOT_READABLE,         "User data not readable" },
	{ NO_PSAP_AVAILABLE,              "No PSAP available" },
	{ 0,                              NULL }
};

/*
 * Reject status codes.
 */
static const value_string reject_status_vals[] = {
	{ 0,          "Stub-defined exception" },
	{ 0x1c000001, "nca_s_fault_int_div_by_zero" },
	{ 0x1c000002, "nca_s_fault_addr_error" },
	{ 0x1c000003, "nca_s_fault_fp_div_zero" },
	{ 0x1c000004, "nca_s_fault_fp_underflow" },
	{ 0x1c000005, "nca_s_fault_fp_overflow" },
	{ 0x1c000006, "nca_s_fault_invalid_tag" },
	{ 0x1c000007, "nca_s_fault_invalid_bound" },
	{ 0x1c000008, "nca_rpc_version_mismatch" },
	{ 0x1c000009, "nca_unspec_reject" },
	{ 0x1c00000a, "nca_s_bad_actid" },
	{ 0x1c00000b, "nca_who_are_you_failed" },
	{ 0x1c00000c, "nca_manager_not_entered" },
	{ 0x1c00000d, "nca_s_fault_cancel" },
	{ 0x1c00000e, "nca_s_fault_ill_inst" },
	{ 0x1c00000f, "nca_s_fault_fp_error" },
	{ 0x1c000010, "nca_s_fault_int_overflow" },
	{ 0x1c000014, "nca_s_fault_pipe_empty" },
	{ 0x1c000015, "nca_s_fault_pipe_closed" },
	{ 0x1c000016, "nca_s_fault_pipe_order" },
	{ 0x1c000017, "nca_s_fault_pipe_discipline" },
	{ 0x1c000018, "nca_s_fault_pipe_comm_error" },
	{ 0x1c000019, "nca_s_fault_pipe_memory" },
	{ 0x1c00001a, "nca_s_fault_context_mismatch" },
	{ 0x1c00001b, "nca_s_fault_remote_no_memory" },
	{ 0x1c00001c, "nca_invalid_pres_context_id" },
	{ 0x1c00001d, "nca_unsupported_authn_level" },
	{ 0x1c00001f, "nca_invalid_checksum" },
	{ 0x1c000020, "nca_invalid_crc" },
	{ 0x1c000021, "ncs_s_fault_user_defined" },
	{ 0x1c000022, "nca_s_fault_tx_open_failed" },
	{ 0x1c000023, "nca_s_fault_codeset_conv_error" },
	{ 0x1c000024, "nca_s_fault_object_not_found" },
	{ 0x1c000025, "nca_s_fault_no_client_stub" },
	{ 0x1c010002, "nca_op_rng_error" },
	{ 0x1c010003, "nca_unk_if"},
	{ 0x1c010006, "nca_wrong_boot_time" },
	{ 0x1c010009, "nca_s_you_crashed" },
	{ 0x1c01000b, "nca_proto_error" },
	{ 0x1c010013, "nca_out_args_too_big" },
	{ 0x1c010014, "nca_server_too_busy" },
	{ 0x1c010017, "nca_unsupported_type" },
	{ 0,          NULL }
};

static int proto_dcerpc = -1;

/* field defines */
static int hf_dcerpc_request_in = -1;
static int hf_dcerpc_time = -1;
static int hf_dcerpc_response_in = -1;
static int hf_dcerpc_ver = -1;
static int hf_dcerpc_ver_minor = -1;
static int hf_dcerpc_packet_type = -1;
static int hf_dcerpc_cn_flags = -1;
static int hf_dcerpc_cn_flags_first_frag = -1;
static int hf_dcerpc_cn_flags_last_frag = -1;
static int hf_dcerpc_cn_flags_cancel_pending = -1;
static int hf_dcerpc_cn_flags_reserved = -1;
static int hf_dcerpc_cn_flags_mpx = -1;
static int hf_dcerpc_cn_flags_dne = -1;
static int hf_dcerpc_cn_flags_maybe = -1;
static int hf_dcerpc_cn_flags_object = -1;
static int hf_dcerpc_drep = -1;
static int hf_dcerpc_drep_byteorder = -1;
static int hf_dcerpc_drep_character = -1;
static int hf_dcerpc_drep_fp = -1;
static int hf_dcerpc_cn_frag_len = -1;
static int hf_dcerpc_cn_auth_len = -1;
static int hf_dcerpc_cn_call_id = -1;
static int hf_dcerpc_cn_max_xmit = -1;
static int hf_dcerpc_cn_max_recv = -1;
static int hf_dcerpc_cn_assoc_group = -1;
static int hf_dcerpc_cn_num_ctx_items = -1;
static int hf_dcerpc_cn_ctx_id = -1;
static int hf_dcerpc_cn_num_trans_items = -1;
static int hf_dcerpc_cn_bind_if_id = -1;
static int hf_dcerpc_cn_bind_if_ver = -1;
static int hf_dcerpc_cn_bind_if_ver_minor = -1;
static int hf_dcerpc_cn_bind_trans_id = -1;
static int hf_dcerpc_cn_bind_trans_ver = -1;
static int hf_dcerpc_cn_alloc_hint = -1;
static int hf_dcerpc_cn_sec_addr_len = -1;
static int hf_dcerpc_cn_sec_addr = -1;
static int hf_dcerpc_cn_num_results = -1;
static int hf_dcerpc_cn_ack_result = -1;
static int hf_dcerpc_cn_ack_reason = -1;
static int hf_dcerpc_cn_ack_trans_id = -1;
static int hf_dcerpc_cn_ack_trans_ver = -1;
static int hf_dcerpc_cn_reject_reason = -1;
static int hf_dcerpc_cn_num_protocols = -1;
static int hf_dcerpc_cn_protocol_ver_major = -1;
static int hf_dcerpc_cn_protocol_ver_minor = -1;
static int hf_dcerpc_cn_cancel_count = -1;
static int hf_dcerpc_cn_status = -1;
static int hf_dcerpc_auth_type = -1;
static int hf_dcerpc_auth_level = -1;
static int hf_dcerpc_auth_pad_len = -1;
static int hf_dcerpc_auth_rsrvd = -1;
static int hf_dcerpc_auth_ctx_id = -1;
static int hf_dcerpc_dg_flags1 = -1;
static int hf_dcerpc_dg_flags1_rsrvd_01 = -1;
static int hf_dcerpc_dg_flags1_last_frag = -1;
static int hf_dcerpc_dg_flags1_frag = -1;
static int hf_dcerpc_dg_flags1_nofack = -1;
static int hf_dcerpc_dg_flags1_maybe = -1;
static int hf_dcerpc_dg_flags1_idempotent = -1;
static int hf_dcerpc_dg_flags1_broadcast = -1;
static int hf_dcerpc_dg_flags1_rsrvd_80 = -1;
static int hf_dcerpc_dg_flags2 = -1;
static int hf_dcerpc_dg_flags2_rsrvd_01 = -1;
static int hf_dcerpc_dg_flags2_cancel_pending = -1;
static int hf_dcerpc_dg_flags2_rsrvd_04 = -1;
static int hf_dcerpc_dg_flags2_rsrvd_08 = -1;
static int hf_dcerpc_dg_flags2_rsrvd_10 = -1;
static int hf_dcerpc_dg_flags2_rsrvd_20 = -1;
static int hf_dcerpc_dg_flags2_rsrvd_40 = -1;
static int hf_dcerpc_dg_flags2_rsrvd_80 = -1;
static int hf_dcerpc_dg_serial_hi = -1;
static int hf_dcerpc_obj_id = -1;
static int hf_dcerpc_dg_if_id = -1;
static int hf_dcerpc_dg_act_id = -1;
static int hf_dcerpc_dg_serial_lo = -1;
static int hf_dcerpc_dg_ahint = -1;
static int hf_dcerpc_dg_ihint = -1;
static int hf_dcerpc_dg_frag_len = -1;
static int hf_dcerpc_dg_frag_num = -1;
static int hf_dcerpc_dg_auth_proto = -1;
static int hf_dcerpc_opnum = -1;
static int hf_dcerpc_dg_seqnum = -1;
static int hf_dcerpc_dg_server_boot = -1;
static int hf_dcerpc_dg_if_ver = -1;
static int hf_dcerpc_krb5_av_prot_level = -1;
static int hf_dcerpc_krb5_av_key_vers_num = -1;
static int hf_dcerpc_krb5_av_key_auth_verifier = -1;
static int hf_dcerpc_dg_cancel_vers = -1;
static int hf_dcerpc_dg_cancel_id = -1;
static int hf_dcerpc_dg_server_accepting_cancels = -1;
static int hf_dcerpc_dg_fack_vers = -1;
static int hf_dcerpc_dg_fack_window_size = -1;
static int hf_dcerpc_dg_fack_max_tsdu = -1;
static int hf_dcerpc_dg_fack_max_frag_size = -1;
static int hf_dcerpc_dg_fack_serial_num = -1;
static int hf_dcerpc_dg_fack_selack_len = -1;
static int hf_dcerpc_dg_fack_selack = -1;
static int hf_dcerpc_dg_status = -1;
static int hf_dcerpc_array_max_count = -1;
static int hf_dcerpc_array_offset = -1;
static int hf_dcerpc_array_actual_count = -1;
static int hf_dcerpc_array_buffer = -1;
static int hf_dcerpc_op = -1;
static int hf_dcerpc_referent_id = -1;
static int hf_dcerpc_fragments = -1;
static int hf_dcerpc_fragment = -1;
static int hf_dcerpc_fragment_overlap = -1;
static int hf_dcerpc_fragment_overlap_conflict = -1;
static int hf_dcerpc_fragment_multiple_tails = -1;
static int hf_dcerpc_fragment_too_long_fragment = -1;
static int hf_dcerpc_fragment_error = -1;
static int hf_dcerpc_reassembled_in = -1;
static int hf_dcerpc_unknown_if_id = -1;

static gint ett_dcerpc = -1;
static gint ett_dcerpc_cn_flags = -1;
static gint ett_dcerpc_cn_ctx = -1;
static gint ett_dcerpc_cn_iface = -1;
static gint ett_dcerpc_drep = -1;
static gint ett_dcerpc_dg_flags1 = -1;
static gint ett_dcerpc_dg_flags2 = -1;
static gint ett_dcerpc_pointer_data = -1;
static gint ett_dcerpc_string = -1;
static gint ett_dcerpc_fragments = -1;
static gint ett_dcerpc_fragment = -1;
static gint ett_dcerpc_krb5_auth_verf = -1;

static const fragment_items dcerpc_frag_items = {
	&ett_dcerpc_fragments,
	&ett_dcerpc_fragment,

	&hf_dcerpc_fragments,
	&hf_dcerpc_fragment,
	&hf_dcerpc_fragment_overlap,
	&hf_dcerpc_fragment_overlap_conflict,
	&hf_dcerpc_fragment_multiple_tails,
	&hf_dcerpc_fragment_too_long_fragment,
	&hf_dcerpc_fragment_error,
	NULL,

	"fragments"
};



#ifdef WIN32
int ResolveWin32UUID(e_uuid_t if_id, char *UUID_NAME, int UUID_NAME_MAX_LEN)
{
	char REG_UUID_NAME[MAX_PATH];
	HKEY hKey = NULL;
	DWORD UUID_MAX_SIZE = MAX_PATH;
	char REG_UUID_STR[MAX_PATH];
	
	if(UUID_NAME_MAX_LEN < 2)
		return 0;
	REG_UUID_NAME[0] = '\0';
	snprintf(REG_UUID_STR, MAX_PATH, "SOFTWARE\\Classes\\Interface\\{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
			if_id.Data1, if_id.Data2, if_id.Data3,
			if_id.Data4[0], if_id.Data4[1],
			if_id.Data4[2], if_id.Data4[3],
			if_id.Data4[4], if_id.Data4[5],
			if_id.Data4[6], if_id.Data4[7]);
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, (LPCSTR)REG_UUID_STR, 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
	{
		if (RegQueryValueEx(hKey, NULL, NULL, NULL, (LPBYTE)REG_UUID_NAME, &UUID_MAX_SIZE) == ERROR_SUCCESS && UUID_MAX_SIZE <= MAX_PATH)
			{
			snprintf(UUID_NAME, UUID_NAME_MAX_LEN, "%s", REG_UUID_NAME);
			RegCloseKey(hKey);
			return strlen(REG_UUID_NAME);
		}
		RegCloseKey(hKey);
	}
	return 0; /* we didn't find anything anyhow. Please don't use the string! */
	
}
#endif

static dcerpc_info *
get_next_di(void)
{
	static dcerpc_info di[20];
	static int di_counter=0;

	di_counter++;
	if(di_counter>=20){
		di_counter=0;
	}
	return &di[di_counter];
}

/* try to desegment big DCE/RPC packets over TCP? */
static gboolean dcerpc_cn_desegment = TRUE;

/* reassemble DCE/RPC fragments */
/* reassembly of dcerpc fragments will not work for the case where ONE frame
   might contain multiple dcerpc fragments for different PDUs.
   this case would be so unusual/weird so if you got captures like that:
	too bad
*/
static gboolean dcerpc_reassemble = FALSE;
static GHashTable *dcerpc_co_reassemble_table = NULL;
static GHashTable *dcerpc_cl_reassemble_table = NULL;

static void
dcerpc_reassemble_init(void)
{
  fragment_table_init(&dcerpc_co_reassemble_table);
  fragment_table_init(&dcerpc_cl_reassemble_table);
}

/*
 * Authentication subdissectors.  Used to dissect authentication blobs in
 * DCERPC binds, requests and responses.
 */

typedef struct _dcerpc_auth_subdissector {
	guint8 auth_level;
	guint8 auth_type;
	dcerpc_auth_subdissector_fns auth_fns;
} dcerpc_auth_subdissector;

static GSList *dcerpc_auth_subdissector_list;

static dcerpc_auth_subdissector_fns *get_auth_subdissector_fns(
	guint8 auth_level, guint8 auth_type)
{
	gpointer data;
	int i;

	for (i = 0; (data = g_slist_nth_data(dcerpc_auth_subdissector_list, i)); i++) {
		dcerpc_auth_subdissector *asd = (dcerpc_auth_subdissector *)data;

		if (asd->auth_level == auth_level && 
		    asd->auth_type == auth_type)
			return &asd->auth_fns;
	}

	return NULL;
}

void register_dcerpc_auth_subdissector(guint8 auth_level, guint8 auth_type,
				       dcerpc_auth_subdissector_fns *fns)
{
	dcerpc_auth_subdissector *d;

	if (get_auth_subdissector_fns(auth_level, auth_type))
		return;

	d = (dcerpc_auth_subdissector *)g_malloc(sizeof(dcerpc_auth_subdissector));

	d->auth_level = auth_level;
	d->auth_type = auth_type;
	memcpy(&d->auth_fns, fns, sizeof(dcerpc_auth_subdissector_fns));

	dcerpc_auth_subdissector_list = g_slist_append(dcerpc_auth_subdissector_list, d);
}

/* Hand off verifier data to a registered dissector */

static void dissect_auth_verf(tvbuff_t *auth_tvb, packet_info *pinfo,
			      proto_tree *tree, 
			      dcerpc_auth_subdissector_fns *auth_fns,
			      e_dce_cn_common_hdr_t *hdr, 
			      dcerpc_auth_info *auth_info)
{
	dcerpc_dissect_fnct_t *volatile fn = NULL;

	switch (hdr->ptype) {
	case PDU_BIND:
	case PDU_ALTER:
		fn = auth_fns->bind_fn;
		break;
	case PDU_BIND_ACK:
	case PDU_ALTER_ACK:
		fn = auth_fns->bind_ack_fn;
		break;
	case PDU_AUTH3:
		fn = auth_fns->auth3_fn;
		break;
	case PDU_REQ:
		fn = auth_fns->req_verf_fn;
		break;
	case PDU_RESP:
		fn = auth_fns->resp_verf_fn;
		break;

		/* Don't know how to handle authentication data in this 
		   pdu type. */

	default:
		g_warning("attempt to dissect %s pdu authentication data",
			  val_to_str(hdr->ptype, pckt_vals, "Unknown (%u)"));
		break;
	}

	if (fn)
		fn(auth_tvb, 0, pinfo, tree, hdr->drep);
	else
		proto_tree_add_text(tree, auth_tvb, 0, hdr->auth_len,
				    "%s Verifier", 
				    val_to_str(auth_info->auth_type, 
					       authn_protocol_vals,
					       "Unknown (%u)"));
}

/* Hand off payload data to a registered dissector */

static tvbuff_t *decode_encrypted_data(tvbuff_t *enc_tvb, 
				       packet_info *pinfo,
				       dcerpc_auth_subdissector_fns *auth_fns,
				       gboolean is_request, 
				       dcerpc_auth_info *auth_info)
{
	dcerpc_decode_data_fnct_t *fn;

	if (is_request)
		fn = auth_fns->req_data_fn;
	else
		fn = auth_fns->resp_data_fn;

	if (fn)
		return fn(enc_tvb, 0, pinfo, auth_info);

	return NULL;
}

/*
 * Subdissectors
 */

/* the registered subdissectors */
GHashTable *dcerpc_uuids=NULL;

static gint
dcerpc_uuid_equal (gconstpointer k1, gconstpointer k2)
{
    const dcerpc_uuid_key *key1 = (const dcerpc_uuid_key *)k1;
    const dcerpc_uuid_key *key2 = (const dcerpc_uuid_key *)k2;
    return ((memcmp (&key1->uuid, &key2->uuid, sizeof (e_uuid_t)) == 0)
            && (key1->ver == key2->ver));
}

static guint
dcerpc_uuid_hash (gconstpointer k)
{
    const dcerpc_uuid_key *key = (const dcerpc_uuid_key *)k;
    /* This isn't perfect, but the Data1 part of these is almost always
       unique. */
    return key->uuid.Data1;
}

void
dcerpc_init_uuid (int proto, int ett, e_uuid_t *uuid, guint16 ver,
                  dcerpc_sub_dissector *procs, int opnum_hf)
{
    dcerpc_uuid_key *key = g_malloc (sizeof (*key));
    dcerpc_uuid_value *value = g_malloc (sizeof (*value));
    header_field_info *hf_info;

    key->uuid = *uuid;
    key->ver = ver;

    value->proto = find_protocol_by_id(proto);
    value->proto_id = proto;
    value->ett = ett;
    value->name = proto_get_protocol_short_name (value->proto);
    value->procs = procs;
    value->opnum_hf = opnum_hf;

    g_hash_table_insert (dcerpc_uuids, key, value);

    hf_info = proto_registrar_get_nth(opnum_hf);
    hf_info->strings = value_string_from_subdissectors(procs);
}

/* Function to find the name of a registered protocol
 * or NULL if the protocol/version is not known to ethereal.
 */
char *
dcerpc_get_proto_name(e_uuid_t *uuid, guint16 ver)
{
    dcerpc_uuid_key key;
    dcerpc_uuid_value *sub_proto;

    key.uuid = *uuid;
    key.ver = ver;
    if(!(sub_proto = g_hash_table_lookup (dcerpc_uuids, &key))){
        return NULL;
    }
    return sub_proto->name;
}

/* Function to find the opnum hf-field of a registered protocol
 * or -1 if the protocol/version is not known to ethereal.
 */
int
dcerpc_get_proto_hf_opnum(e_uuid_t *uuid, guint16 ver)
{
    dcerpc_uuid_key key;
    dcerpc_uuid_value *sub_proto;

    key.uuid = *uuid;
    key.ver = ver;
    if(!(sub_proto = g_hash_table_lookup (dcerpc_uuids, &key))){
        return -1;
    }
    return sub_proto->opnum_hf;
}

/* Create a value_string consisting of DCERPC opnum and name from a
   subdissector array. */

value_string *value_string_from_subdissectors(dcerpc_sub_dissector *sd)
{
	value_string *vs = NULL;
	int i, num_sd = 0;

 again:
	for (i = 0; sd[i].name; i++) {
		if (vs) {
			vs[i].value = sd[i].num;
			vs[i].strptr = sd[i].name;
		} else
			num_sd++;
	}

	if (!vs) {
		vs = g_malloc((num_sd + 1) * sizeof(value_string));
		goto again;
	}

	vs[num_sd].value = 0;
	vs[num_sd].strptr = NULL;

	return vs;
}

/* Function to find the subdissector table of a registered protocol
 * or NULL if the protocol/version is not known to ethereal.
 */
dcerpc_sub_dissector *
dcerpc_get_proto_sub_dissector(e_uuid_t *uuid, guint16 ver)
{
    dcerpc_uuid_key key;
    dcerpc_uuid_value *sub_proto;

    key.uuid = *uuid;
    key.ver = ver;
    if(!(sub_proto = g_hash_table_lookup (dcerpc_uuids, &key))){
        return NULL;
    }
    return sub_proto->procs;
}


/*
 * To keep track of ctx_id mappings.
 *
 * Everytime we see a bind call we update this table.
 * Note that we always specify a SMB FID. For non-SMB transports this
 * value is 0.
 */
static GHashTable *dcerpc_binds=NULL;

typedef struct _dcerpc_bind_key {
    conversation_t *conv;
    guint16 ctx_id;
    guint16 smb_fid;
} dcerpc_bind_key;

typedef struct _dcerpc_bind_value {
	e_uuid_t uuid;
	guint16 ver;
} dcerpc_bind_value;

static GMemChunk *dcerpc_bind_key_chunk=NULL;
static GMemChunk *dcerpc_bind_value_chunk=NULL;

static gint
dcerpc_bind_equal (gconstpointer k1, gconstpointer k2)
{
    const dcerpc_bind_key *key1 = (const dcerpc_bind_key *)k1;
    const dcerpc_bind_key *key2 = (const dcerpc_bind_key *)k2;
    return (key1->conv == key2->conv
            && key1->ctx_id == key2->ctx_id
            && key1->smb_fid == key2->smb_fid);
}

static guint
dcerpc_bind_hash (gconstpointer k)
{
    const dcerpc_bind_key *key = (const dcerpc_bind_key *)k;
    return GPOINTER_TO_UINT(key->conv) + key->ctx_id + key->smb_fid;

}

/*
 * To keep track of callid mappings.  Should really use some generic
 * conversation support instead.
 */
static GHashTable *dcerpc_calls=NULL;

typedef struct _dcerpc_call_key {
    conversation_t *conv;
    guint32 call_id;
    guint16 smb_fid;
} dcerpc_call_key;

static GMemChunk *dcerpc_call_key_chunk=NULL;

static GMemChunk *dcerpc_call_value_chunk=NULL;

static gint
dcerpc_call_equal (gconstpointer k1, gconstpointer k2)
{
    const dcerpc_call_key *key1 = (const dcerpc_call_key *)k1;
    const dcerpc_call_key *key2 = (const dcerpc_call_key *)k2;
    return (key1->conv == key2->conv
            && key1->call_id == key2->call_id
            && key1->smb_fid == key2->smb_fid);
}

static guint
dcerpc_call_hash (gconstpointer k)
{
    const dcerpc_call_key *key = (const dcerpc_call_key *)k;
    return ((guint32)key->conv) + key->call_id + key->smb_fid;
}


/* to keep track of matched calls/responses
   this one uses the same value struct as calls, but the key is the frame id
   and call id; there can be more than one call in a frame.

   XXX - why not just use the same keys as are used for calls?
*/

static GHashTable *dcerpc_matched=NULL;

typedef struct _dcerpc_matched_key {
    guint32 frame;
    guint32 call_id;
} dcerpc_matched_key;

static GMemChunk *dcerpc_matched_key_chunk=NULL;

static gint
dcerpc_matched_equal (gconstpointer k1, gconstpointer k2)
{
    const dcerpc_matched_key *key1 = (const dcerpc_matched_key *)k1;
    const dcerpc_matched_key *key2 = (const dcerpc_matched_key *)k2;
    return (key1->frame == key2->frame
            && key1->call_id == key2->call_id);
}

static guint
dcerpc_matched_hash (gconstpointer k)
{
    const dcerpc_matched_key *key = (const dcerpc_matched_key *)k;
    return key->frame;
}



/*
 * Utility functions.  Modeled after packet-rpc.c
 */

int
dissect_dcerpc_uint8 (tvbuff_t *tvb, gint offset, packet_info *pinfo _U_,
                      proto_tree *tree, guint8 *drep,
                      int hfindex, guint8 *pdata)
{
    guint8 data;

    data = tvb_get_guint8 (tvb, offset);
    if (tree) {
        proto_tree_add_item (tree, hfindex, tvb, offset, 1, (drep[0] & 0x10));
    }
    if (pdata)
        *pdata = data;
    return offset + 1;
}

int
dissect_dcerpc_uint16 (tvbuff_t *tvb, gint offset, packet_info *pinfo _U_,
                       proto_tree *tree, guint8 *drep,
                       int hfindex, guint16 *pdata)
{
    guint16 data;

    data = ((drep[0] & 0x10)
            ? tvb_get_letohs (tvb, offset)
            : tvb_get_ntohs (tvb, offset));

    if (tree) {
        proto_tree_add_item (tree, hfindex, tvb, offset, 2, (drep[0] & 0x10));
    }
    if (pdata)
        *pdata = data;
    return offset + 2;
}

int
dissect_dcerpc_uint32 (tvbuff_t *tvb, gint offset, packet_info *pinfo _U_,
                       proto_tree *tree, guint8 *drep,
                       int hfindex, guint32 *pdata)
{
    guint32 data;

    data = ((drep[0] & 0x10)
            ? tvb_get_letohl (tvb, offset)
            : tvb_get_ntohl (tvb, offset));

    if (tree) {
        proto_tree_add_item (tree, hfindex, tvb, offset, 4, (drep[0] & 0x10));
    }
    if (pdata)
        *pdata = data;
    return offset+4;
}

/* handles 32 bit unix time_t */
int
dissect_dcerpc_time_t (tvbuff_t *tvb, gint offset, packet_info *pinfo _U_,
                       proto_tree *tree, guint8 *drep,
                       int hfindex, guint32 *pdata)
{
    guint32 data;
    nstime_t tv;

    data = ((drep[0] & 0x10)
            ? tvb_get_letohl (tvb, offset)
            : tvb_get_ntohl (tvb, offset));

    tv.secs=data;
    tv.nsecs=0;
    if (tree) {
        if(data==0xffffffff){
            /* special case,   no time specified */
            proto_tree_add_time_format(tree, hfindex, tvb, offset, 4, &tv, "%s: No time specified", proto_registrar_get_nth(hfindex)->name);
        } else {
            proto_tree_add_time (tree, hfindex, tvb, offset, 4, &tv);
        }
    }
    if (pdata)
        *pdata = data;

    return offset+4;
}

int
dissect_dcerpc_uint64 (tvbuff_t *tvb, gint offset, packet_info *pinfo _U_,
                       proto_tree *tree, guint8 *drep,
                       int hfindex, unsigned char *pdata)
{
    if(pdata){
      tvb_memcpy(tvb, pdata, offset, 8);
      if(drep[0] & 0x10){/* XXX this might be the wrong way around */
	unsigned char data;
	data=pdata[0];pdata[0]=pdata[7];pdata[7]=data;
	data=pdata[1];pdata[1]=pdata[6];pdata[6]=data;
	data=pdata[2];pdata[2]=pdata[5];pdata[5]=data;
	data=pdata[3];pdata[3]=pdata[4];pdata[4]=data;
      }
    }

    if (tree) {
        proto_tree_add_item(tree, hfindex, tvb, offset, 8, (drep[0] & 0x10));
    }

    return offset+8;
}


int
dissect_dcerpc_float(tvbuff_t *tvb, gint offset, packet_info *pinfo _U_,
                    proto_tree *tree, guint8 *drep, 
                    int hfindex, gfloat *pdata)
{
	gfloat data;


	switch(drep[1]) {
		case(DCE_RPC_DREP_FP_IEEE):
			data = ((drep[0] & 0x10)
					? tvb_get_letohieee_float(tvb, offset)
					: tvb_get_ntohieee_float(tvb, offset));
			if (tree) {
				proto_tree_add_float(tree, hfindex, tvb, offset, 4, data);
			}
			break;
		case(DCE_RPC_DREP_FP_VAX):  /* (fall trough) */
		case(DCE_RPC_DREP_FP_CRAY): /* (fall trough) */
		case(DCE_RPC_DREP_FP_IBM):  /* (fall trough) */
		default:
			/* ToBeDone: non IEEE floating formats */
			/* Set data to a negative infinity value */
			data = -G_MAXFLOAT;
			if (tree) {
				proto_tree_add_debug_text(tree, "DCE RPC: dissection of non IEEE floating formats currently not implemented (drep=%u)!", drep[1]);
			}
	}
    if (pdata)
        *pdata = data;
    return offset + 4;
}


int
dissect_dcerpc_double(tvbuff_t *tvb, gint offset, packet_info *pinfo _U_,
                    proto_tree *tree, guint8 *drep, 
                    int hfindex, gdouble *pdata)
{
    gdouble data;


	switch(drep[1]) {
		case(DCE_RPC_DREP_FP_IEEE):
			data = ((drep[0] & 0x10)
					? tvb_get_letohieee_double(tvb, offset)
					: tvb_get_ntohieee_double(tvb, offset));
			if (tree) {
				proto_tree_add_double(tree, hfindex, tvb, offset, 8, data);
			}
			break;
		case(DCE_RPC_DREP_FP_VAX):  /* (fall trough) */
		case(DCE_RPC_DREP_FP_CRAY): /* (fall trough) */
		case(DCE_RPC_DREP_FP_IBM):  /* (fall trough) */
		default:
			/* ToBeDone: non IEEE double formats */
			/* Set data to a negative infinity value */
			data = -G_MAXDOUBLE;
			if (tree) {
				proto_tree_add_debug_text(tree, "DCE RPC: dissection of non IEEE double formats currently not implemented (drep=%u)!", drep[1]);
			}
	}
    if (pdata)
        *pdata = data;
    return offset + 8;
}


/*
 * a couple simpler things
 */
guint16
dcerpc_tvb_get_ntohs (tvbuff_t *tvb, gint offset, guint8 *drep)
{
    if (drep[0] & 0x10) {
        return tvb_get_letohs (tvb, offset);
    } else {
        return tvb_get_ntohs (tvb, offset);
    }
}

guint32
dcerpc_tvb_get_ntohl (tvbuff_t *tvb, gint offset, guint8 *drep)
{
    if (drep[0] & 0x10) {
        return tvb_get_letohl (tvb, offset);
    } else {
        return tvb_get_ntohl (tvb, offset);
    }
}

void
dcerpc_tvb_get_uuid (tvbuff_t *tvb, gint offset, guint8 *drep, e_uuid_t *uuid)
{
    unsigned int i;
    uuid->Data1 = dcerpc_tvb_get_ntohl (tvb, offset, drep);
    uuid->Data2 = dcerpc_tvb_get_ntohs (tvb, offset+4, drep);
    uuid->Data3 = dcerpc_tvb_get_ntohs (tvb, offset+6, drep);

    for (i=0; i<sizeof (uuid->Data4); i++) {
        uuid->Data4[i] = tvb_get_guint8 (tvb, offset+8+i);
    }
}



/* NDR arrays */
/* function to dissect a unidimensional conformant array */
int
dissect_ndr_ucarray(tvbuff_t *tvb, gint offset, packet_info *pinfo,
		proto_tree *tree, guint8 *drep,
		dcerpc_dissect_fnct_t *fnct)
{
	guint32 i;
	dcerpc_info *di;
	int old_offset;

	di=pinfo->private_data;
	if(di->conformant_run){
		/* conformant run, just dissect the max_count header */
		old_offset=offset;
		di->conformant_run=0;
		offset = dissect_ndr_uint32 (tvb, offset, pinfo, tree, drep,
				hf_dcerpc_array_max_count, &di->array_max_count);
		di->array_max_count_offset=offset-4;
		di->conformant_run=1;
		di->conformant_eaten=offset-old_offset;
	} else {
		/* we don't remember where in the bytestream this field was */
		proto_tree_add_uint(tree, hf_dcerpc_array_max_count, tvb, di->array_max_count_offset, 4, di->array_max_count);

		/* real run, dissect the elements */
		for(i=0;i<di->array_max_count;i++){
			offset = (*fnct)(tvb, offset, pinfo, tree, drep);
		}
	}

	return offset;
}
/* function to dissect a unidimensional conformant and varying array */
int
dissect_ndr_ucvarray(tvbuff_t *tvb, gint offset, packet_info *pinfo,
		proto_tree *tree, guint8 *drep,
		dcerpc_dissect_fnct_t *fnct)
{
	guint32 i;
	dcerpc_info *di;
	int old_offset;

	di=pinfo->private_data;
	if(di->conformant_run){
		/* conformant run, just dissect the max_count header */
		old_offset=offset;
		di->conformant_run=0;
		offset = dissect_ndr_uint32 (tvb, offset, pinfo, tree, drep,
				hf_dcerpc_array_max_count, &di->array_max_count);
		di->array_max_count_offset=offset-4;
		offset = dissect_ndr_uint32 (tvb, offset, pinfo, tree, drep,
				hf_dcerpc_array_offset, &di->array_offset);
		di->array_offset_offset=offset-4;
		offset = dissect_ndr_uint32 (tvb, offset, pinfo, tree, drep,
				hf_dcerpc_array_actual_count, &di->array_actual_count);
		di->array_actual_count_offset=offset-4;
		di->conformant_run=1;
		di->conformant_eaten=offset-old_offset;
	} else {
		/* we dont dont remember where  in the bytestream these fields were */
		proto_tree_add_uint(tree, hf_dcerpc_array_max_count, tvb, di->array_max_count_offset, 4, di->array_max_count);
		proto_tree_add_uint(tree, hf_dcerpc_array_offset, tvb, di->array_offset_offset, 4, di->array_offset);
		proto_tree_add_uint(tree, hf_dcerpc_array_actual_count, tvb, di->array_actual_count_offset, 4, di->array_actual_count);

		/* real run, dissect the elements */
		for(i=0;i<di->array_actual_count;i++){
			offset = (*fnct)(tvb, offset, pinfo, tree, drep);
		}
	}

	return offset;
}

/* Dissect an string of bytes.  This corresponds to
   IDL of the form '[string] byte *foo'.

   It can also be used for a conformant varying array of bytes if
   the contents of the array should be shown as a big blob, rather
   than showing each byte as an individual element.

   XXX - which of those is really the IDL type for, for example,
   the encrypted data in some MAPI packets?  (Microsoft haven't
   released that IDL.)

   XXX - does this need to do all the conformant array stuff that
   "dissect_ndr_ucvarray()" does?  These are presumably for strings
   that are conformant and varying - they're stored like conformant
   varying arrays of bytes.  */
int
dissect_ndr_byte_array(tvbuff_t *tvb, int offset, packet_info *pinfo, 
                            proto_tree *tree, guint8 *drep)
{
    dcerpc_info *di;
    guint32 len;

    di=pinfo->private_data;
    if(di->conformant_run){
      /* just a run to handle conformant arrays, no scalars to dissect */
      return offset;
    }

    /* NDR array header */

    offset = dissect_ndr_uint32(tvb, offset, pinfo, tree, drep,
                                hf_dcerpc_array_max_count, NULL);

    offset = dissect_ndr_uint32(tvb, offset, pinfo, tree, drep,
                                hf_dcerpc_array_offset, NULL);

    offset = dissect_ndr_uint32(tvb, offset, pinfo, tree, drep,
                                hf_dcerpc_array_actual_count, &len);

    if (tree && len)
        proto_tree_add_item(tree, hf_dcerpc_array_buffer,
                            tvb, offset, len, drep[0] & 0x10);

    offset += len;

    return offset;
}

/* For dissecting arrays that are to be interpreted as strings.  */

/* Dissect an NDR conformant varying string of elements.
   The length of each element is given by the 'size_is' parameter;
   the elements are assumed to be characters or wide characters.

   XXX - does this need to do all the conformant array stuff that
   "dissect_ndr_ucvarray()" does?  */
int
dissect_ndr_cvstring(tvbuff_t *tvb, int offset, packet_info *pinfo, 
		     proto_tree *tree, guint8 *drep, int size_is,
		     int hfindex, gboolean add_subtree, char **data)
{
    dcerpc_info *di;
    proto_item *string_item;
    proto_tree *string_tree;
    guint32 len, buffer_len;
    char *s;
    header_field_info *hfinfo;

    di=pinfo->private_data;
    if(di->conformant_run){
      /* just a run to handle conformant arrays, no scalars to dissect */
      return offset;
    }

    if (add_subtree) {
        string_item = proto_tree_add_text(tree, tvb, offset, -1, "%s",
                                          proto_registrar_get_name(hfindex));
        string_tree = proto_item_add_subtree(string_item, ett_dcerpc_string);
    } else {
        string_item = NULL;
        string_tree = tree;
    }

    /* NDR array header */

    offset = dissect_ndr_uint32(tvb, offset, pinfo, string_tree, drep,
                                hf_dcerpc_array_max_count, NULL);

    offset = dissect_ndr_uint32(tvb, offset, pinfo, string_tree, drep,
                                hf_dcerpc_array_offset, NULL);

    offset = dissect_ndr_uint32(tvb, offset, pinfo, string_tree, drep,
                                hf_dcerpc_array_actual_count, &len);

    buffer_len = size_is * len;

    /* Adjust offset */
    if (offset % size_is)
        offset += size_is - (offset % size_is);

    if (size_is == sizeof(guint16)) {
        /* XXX - use drep to determine the byte order? */
        s = tvb_fake_unicode(tvb, offset, buffer_len / 2, TRUE);
        /*
         * XXX - we don't support a string type with Unicode
         * characters, so if this is a string item, we make
         * its value be the "fake Unicode" string.
         */
        if (tree && buffer_len) {
            hfinfo = proto_registrar_get_nth(hfindex);
            if (hfinfo->type == FT_STRING) {
                proto_tree_add_string(string_tree, hfindex, tvb, offset,
                                      buffer_len, s);
            } else {
                proto_tree_add_item(string_tree, hfindex, tvb, offset,
                                    buffer_len, drep[0] & 0x10);
            }
        }
    } else {
        /*
         * "tvb_get_string()" throws an exception if the entire string
         * isn't in the tvbuff.  If the length is bogus, this should
         * keep us from trying to allocate an immensely large buffer.
         * (It won't help if the length is *valid* but immensely large,
         * but that's another matter; in any case, that would happen only
         * if we had an immensely large tvbuff....)
         */
        s = tvb_get_string(tvb, offset, buffer_len);
        if (tree && buffer_len)
            proto_tree_add_item(string_tree, hfindex, tvb, offset,
                                buffer_len, drep[0] & 0x10);
    }

    if (string_item != NULL)
        proto_item_append_text(string_item, ": %s", s);

    if (data)
	    *data = s;
    else
	    g_free(s);
    
    offset += buffer_len;

    proto_item_set_end(string_item, tvb, offset);

    return offset;
}

/* Dissect an conformant varying string of chars.
   This corresponds to IDL of the form '[string] char *foo'.

   XXX - at least according to the DCE RPC 1.1 spec, a string has
   a null terminator, which isn't necessary as a terminator for
   the transfer language (as there's a length), but is presumably
   there for the benefit of null-terminated-string languages
   such as C.  Is this ever used for purely counted strings?
   (Not that it matters if it is.) */
int
dissect_ndr_char_cvstring(tvbuff_t *tvb, int offset, packet_info *pinfo, 
			proto_tree *tree, guint8 *drep)
{
    dcerpc_info *di;
    di=pinfo->private_data;

    return dissect_ndr_cvstring(tvb, offset, pinfo, tree, drep,
				sizeof(guint8), di->hf_index,
				FALSE, NULL);
}

/* Dissect a conformant varying string of wchars (wide characters).
   This corresponds to IDL of the form '[string] wchar *foo'

   XXX - at least according to the DCE RPC 1.1 spec, a string has
   a null terminator, which isn't necessary as a terminator for
   the transfer language (as there's a length), but is presumably
   there for the benefit of null-terminated-string languages
   such as C.  Is this ever used for purely counted strings?
   (Not that it matters if it is.) */
int
dissect_ndr_wchar_cvstring(tvbuff_t *tvb, int offset, packet_info *pinfo, 
			proto_tree *tree, guint8 *drep)
{
    dcerpc_info *di;
    di=pinfo->private_data;

    return dissect_ndr_cvstring(tvb, offset, pinfo, tree, drep,
				sizeof(guint16), di->hf_index,
				FALSE, NULL);
}

/* ndr pointer handling */
/* list of pointers encountered so far */
static GSList *ndr_pointer_list = NULL;

/* position where in the list to insert newly encountered pointers */
static int ndr_pointer_list_pos=0;

/* boolean controlling whether pointers are top-level or embedded */
static gboolean pointers_are_top_level = TRUE;

/* as a kludge, we represent all embedded reference pointers as id==-1
   hoping that his will not collide with any non-ref pointers */
typedef struct ndr_pointer_data {
	guint32 id;
	proto_item *item;	/* proto_item for pointer */
	proto_tree *tree;	/* subtree of above item */
	dcerpc_dissect_fnct_t *fnct; /*if non-NULL, we have not called it yet*/
	int hf_index;
	dcerpc_callback_fnct_t *callback;
	void *callback_args;
} ndr_pointer_data_t;

void
init_ndr_pointer_list(packet_info *pinfo)
{
	dcerpc_info *di;

	di=pinfo->private_data;
	di->conformant_run=0;

	while(ndr_pointer_list){
		ndr_pointer_data_t *npd;

		npd=g_slist_nth_data(ndr_pointer_list, 0);
		ndr_pointer_list=g_slist_remove(ndr_pointer_list, npd);
		if(npd){
			g_free(npd);
		}
	}

	ndr_pointer_list=NULL;
	ndr_pointer_list_pos=0;
	pointers_are_top_level=TRUE;
}

static int
dissect_deferred_pointers(packet_info *pinfo, tvbuff_t *tvb, int offset, guint8 *drep)
{
	int found_new_pointer;
	dcerpc_info *di;
	int old_offset;
	int next_pointer;

	next_pointer=0;
	di=pinfo->private_data;
	do{
		int i, len;

		found_new_pointer=0;
		len=g_slist_length(ndr_pointer_list);
		for(i=next_pointer;i<len;i++){
			ndr_pointer_data_t *tnpd;
			tnpd=g_slist_nth_data(ndr_pointer_list, i);
			if(tnpd->fnct){
				dcerpc_dissect_fnct_t *fnct;

				next_pointer=i+1;
				found_new_pointer=1;
				fnct=tnpd->fnct;
				tnpd->fnct=NULL;
				ndr_pointer_list_pos=i+1;
				di->hf_index=tnpd->hf_index;
				/* first a run to handle any conformant
				   array headers */
				di->conformant_run=1;
				di->conformant_eaten=0;
				old_offset = offset;
				offset = (*(fnct))(tvb, offset, pinfo, NULL, drep);

				g_assert((offset-old_offset)==di->conformant_eaten);
				/* This is to check for any bugs in the dissectors.
				 *
				 * Basically, the NDR representation will store all
				 * arrays in two blocks, one block with the dimension
				 * discreption, like size, number of elements and such,
				 * and another block that contains the actual data stored
				 * in the array.
				 * If the array is embedded directly inside another,
				 * encapsulating aggregate type, like a union or struct,
				 * then these two blocks will be stored at different places
				 * in the bytestream, with other data between the blocks.
				 *
				 * For this reason, all pointers to types (both aggregate
				 * and scalar, for simplicity no distinction is made)
				 * will have its dissector called twice.
				 * The dissector will first be called with conformant_run==1
				 * in which mode the dissector MUST NOT consume any data from
				 * the tvbuff (i.e. may not dissect anything) except the
				 * initial control block for arrays.
				 * The second time the dissector is called, with
				 * conformant_run==0, all other data for the type will be
				 * dissected.
				 *
				 * All dissect_ndr_<type> dissectors are already prepared
				 * for this and knows when it should eat data from the tvb
				 * and when not to, so implementors of dissectors will
				 * normally not need to worry about this or even know about
				 * it. However, if a dissector for an aggregate type calls
				 * a subdissector from outside packet-dcerpc.c, such as
				 * the dissector in packet-smb.c for NT Security Descriptors
				 * as an example, then it is VERY important to encapsulate
				 * this call to an external subdissector with the appropriate
				 * test for conformant_run, i.e. it will need something like
				 *
				 * 	dcerpc_info *di;
				 *
				 *	di=pinfo->private_data;
				 *	if(di->conformant_run){
				 *		return offset;
				 *	}
				 *
				 * to make sure it makes the right thing.
				 * This assert will signal when someone has forgotten to
				 * make the dissector aware of this requirement.
				 */

				/* now we dissect the actual pointer */
				di->conformant_run=0;
				old_offset = offset;
				offset = (*(fnct))(tvb, offset, pinfo, tnpd->tree, drep);
				if (tnpd->callback)
					tnpd->callback(pinfo, tnpd->tree, tnpd->item, tvb, old_offset, offset, tnpd->callback_args);
				break;
			}
		}
	} while(found_new_pointer);

	return offset;
}


static void
add_pointer_to_list(packet_info *pinfo, proto_tree *tree, proto_item *item,
		    dcerpc_dissect_fnct_t *fnct, guint32 id, int hf_index, 
		    dcerpc_callback_fnct_t *callback, void *callback_args)
{
	ndr_pointer_data_t *npd;

	/* check if this pointer is valid */
	if(id!=0xffffffff){
		dcerpc_info *di;
	        dcerpc_call_value *value;

		di=pinfo->private_data;
		value=di->call_data;

		if(di->request){
			if(!(pinfo->fd->flags.visited)){
				if(id>value->max_ptr){
					value->max_ptr=id;
				}
			}
		} else {
			/* if we havent seen the request bail out since we cant
			   know whether this is the first non-NULL instance
			   or not */
			if(value->req_frame==0){
				/* XXX THROW EXCEPTION */
			}

			/* We saw this one in the request frame, nothing to
			   dissect later */
			if(id<=value->max_ptr){
				return;
			}
		}
	}

	npd=g_malloc(sizeof(ndr_pointer_data_t));
	npd->id=id;
	npd->tree=tree;
	npd->item=item;
	npd->fnct=fnct;
	npd->hf_index=hf_index;
	npd->callback=callback;
	npd->callback_args=callback_args;
	ndr_pointer_list = g_slist_insert(ndr_pointer_list, npd,
					ndr_pointer_list_pos);
	ndr_pointer_list_pos++;
}


static int
find_pointer_index(guint32 id)
{
	ndr_pointer_data_t *npd;
	int i,len;

	len=g_slist_length(ndr_pointer_list);
	for(i=0;i<len;i++){
		npd=g_slist_nth_data(ndr_pointer_list, i);
		if(npd){
			if(npd->id==id){
				return i;
			}
		}
	}

	return -1;
}

/* This function dissects an NDR pointer and stores the callback for later
 * deferred dissection.
 *
 *   fnct is the callback function for when we have reached this object in
 *   the bytestream.
 *
 *   type is what type of pointer.
 *
 *   this is text is what text we should put in any created tree node.
 *
 *   hf_index is what hf value we want to pass to the callback function when
 *   it is called, the callback can later pich this one up from di->hf_index.
 *
 *   callback is executed after the pointer has been dereferenced.
 *
 *   callback_args is passed as an argument to the callback function
 *
 * See packet-dcerpc-samr.c for examples
 */
int
dissect_ndr_pointer_cb(tvbuff_t *tvb, gint offset, packet_info *pinfo,
		    proto_tree *tree, guint8 *drep, dcerpc_dissect_fnct_t *fnct,
		    int type, char *text, int hf_index, 
		    dcerpc_callback_fnct_t *callback, void *callback_args)
{
	dcerpc_info *di;

	di=pinfo->private_data;
	if(di->conformant_run){
		/* this call was only for dissecting the header for any
		   embedded conformant array. we will not parse any
		   pointers in this mode.
		*/
		return offset;
	}

	/*TOP LEVEL REFERENCE POINTER*/
	if( pointers_are_top_level
	&&(type==NDR_POINTER_REF) ){
		proto_item *item;
		proto_tree *tr;

		/* we must find out a nice way to do the length here */
		item=proto_tree_add_text(tree, tvb, offset, 0,
			"%s", text);
		tr=proto_item_add_subtree(item,ett_dcerpc_pointer_data);

		add_pointer_to_list(pinfo, tr, item, fnct, 0xffffffff, 
				    hf_index, callback, callback_args);
		goto after_ref_id;
	}

	/*TOP LEVEL FULL POINTER*/
	if( pointers_are_top_level
	&& (type==NDR_POINTER_PTR) ){
		int idx;
		guint32 id;
		proto_item *item;
		proto_tree *tr;

		/* get the referent id */
		offset = dissect_ndr_uint32(tvb, offset, pinfo, NULL, drep, -1, &id);

		/* we got a NULL pointer */
		if(id==0){
			proto_tree_add_text(tree, tvb, offset-4, 4,
				"(NULL pointer) %s",text);
			goto after_ref_id;
		}

		/* see if we have seen this pointer before */
		idx=find_pointer_index(id);

		/* we have seen this pointer before */
		if(idx>=0){
			proto_tree_add_text(tree, tvb, offset-4, 4,
				"(duplicate PTR) %s",text);
			goto after_ref_id;
		}

		/* new pointer */
		item=proto_tree_add_text(tree, tvb, offset-4, 4,
			"%s", text);
		tr=proto_item_add_subtree(item,ett_dcerpc_pointer_data);
		proto_tree_add_uint(tr, hf_dcerpc_referent_id, tvb, offset-4, 4, id);
		add_pointer_to_list(pinfo, tr, item, fnct, id, hf_index, 
				    callback, callback_args);
		goto after_ref_id;
	}
	/*TOP LEVEL UNIQUE POINTER*/
	if( pointers_are_top_level
	&& (type==NDR_POINTER_UNIQUE) ){
		guint32 id;
		proto_item *item;
		proto_tree *tr;

		/* get the referent id */
		offset = dissect_ndr_uint32(tvb, offset, pinfo, NULL, drep, -1, &id);

		/* we got a NULL pointer */
		if(id==0){
			proto_tree_add_text(tree, tvb, offset-4, 4,
				"(NULL pointer) %s",text);
			goto after_ref_id;
		}

		/* new pointer */
		item=proto_tree_add_text(tree, tvb, offset-4, 4,
			"%s", text);
		tr=proto_item_add_subtree(item,ett_dcerpc_pointer_data);
		proto_tree_add_uint(tr, hf_dcerpc_referent_id, tvb, offset-4, 4, id);
		add_pointer_to_list(pinfo, tr, item, fnct, 0xffffffff, 
				    hf_index, callback, callback_args);
		goto after_ref_id;
	}

	/*EMBEDDED REFERENCE POINTER*/
	if( (!pointers_are_top_level)
	&& (type==NDR_POINTER_REF) ){
		guint32 id;
		proto_item *item;
		proto_tree *tr;

		/* get the referent id */
		offset = dissect_ndr_uint32(tvb, offset, pinfo, NULL, drep, -1, &id);

		/* new pointer */
		item=proto_tree_add_text(tree, tvb, offset-4, 4,
			"%s",text);
		tr=proto_item_add_subtree(item,ett_dcerpc_pointer_data);
		proto_tree_add_uint(tr, hf_dcerpc_referent_id, tvb, offset-4, 4, id);
		add_pointer_to_list(pinfo, tr, item, fnct, 0xffffffff, 
				    hf_index, callback, callback_args);
		goto after_ref_id;
	}

	/*EMBEDDED UNIQUE POINTER*/
	if( (!pointers_are_top_level)
	&& (type==NDR_POINTER_UNIQUE) ){
		guint32 id;
		proto_item *item;
		proto_tree *tr;

		/* get the referent id */
		offset = dissect_ndr_uint32(tvb, offset, pinfo, NULL, drep, -1, &id);

		/* we got a NULL pointer */
		if(id==0){
			proto_tree_add_text(tree, tvb, offset-4, 4,
				"(NULL pointer) %s", text);
			goto after_ref_id;
		}

		/* new pointer */
		item=proto_tree_add_text(tree, tvb, offset-4, 4,
			"%s",text);
		tr=proto_item_add_subtree(item,ett_dcerpc_pointer_data);
		proto_tree_add_uint(tr, hf_dcerpc_referent_id, tvb, offset-4, 4, id);
		add_pointer_to_list(pinfo, tr, item, fnct, 0xffffffff, 
				    hf_index, callback, callback_args);
		goto after_ref_id;
	}

	/*EMBEDDED FULL POINTER*/
	if( (!pointers_are_top_level)
	&& (type==NDR_POINTER_PTR) ){
		int idx;
		guint32 id;
		proto_item *item;
		proto_tree *tr;

		/* get the referent id */
		offset = dissect_ndr_uint32(tvb, offset, pinfo, NULL, drep, -1, &id);

		/* we got a NULL pointer */
		if(id==0){
			proto_tree_add_text(tree, tvb, offset-4, 4,
				"(NULL pointer) %s",text);
			goto after_ref_id;
		}

		/* see if we have seen this pointer before */
		idx=find_pointer_index(id);

		/* we have seen this pointer before */
		if(idx>=0){
			proto_tree_add_text(tree, tvb, offset-4, 4,
				"(duplicate PTR) %s",text);
			goto after_ref_id;
		}

		/* new pointer */
		item=proto_tree_add_text(tree, tvb, offset-4, 4,
			"%s", text);
		tr=proto_item_add_subtree(item,ett_dcerpc_pointer_data);
		proto_tree_add_uint(tr, hf_dcerpc_referent_id, tvb, offset-4, 4, id);
		add_pointer_to_list(pinfo, tr, item, fnct, id, hf_index, 
				    callback, callback_args);
		goto after_ref_id;
	}


after_ref_id:
	/* After each top level pointer we have dissected we have to
	   dissect all deferrals before we move on to the next top level
	   argument */
	if(pointers_are_top_level==TRUE){
		pointers_are_top_level=FALSE;
		offset = dissect_deferred_pointers(pinfo, tvb, offset, drep);
		pointers_are_top_level=TRUE;
	}

	return offset;
}

int
dissect_ndr_pointer(tvbuff_t *tvb, gint offset, packet_info *pinfo,
		    proto_tree *tree, guint8 *drep, dcerpc_dissect_fnct_t *fnct,
		    int type, char *text, int hf_index)
{
	return dissect_ndr_pointer_cb(
		tvb, offset, pinfo, tree, drep, fnct, type, text, hf_index,
		NULL, NULL);
}

static void
show_stub_data (tvbuff_t *tvb, gint offset, proto_tree *dcerpc_tree,
                dcerpc_auth_info *auth_info, gboolean is_encrypted)
{
    int length;

    /*
     * We don't show stub data unless we have some in the tvbuff;
     * however, in the protocol tree, we show, as the number of
     * bytes, the reported number of bytes, not the number of bytes
     * that happen to be in the tvbuff.
     */
    if (tvb_length_remaining (tvb, offset) > 0) {
        length = tvb_reported_length_remaining (tvb, offset);
        if (auth_info != NULL &&
            auth_info->auth_level == DCE_C_AUTHN_LEVEL_PKT_PRIVACY) {
            if (is_encrypted) {
                proto_tree_add_text(dcerpc_tree, tvb, offset, -1,
                                    "Encrypted stub data (%d byte%s)",
                                    length, plurality(length, "", "s"));
            } else {
                proto_tree_add_text(dcerpc_tree, tvb, offset, -1,
                                    "Decrypted stub data (%d byte%s)",
                                    length, plurality(length, "", "s"));
            }
        } else {
            proto_tree_add_text (dcerpc_tree, tvb, offset, -1,
                                 "Stub data (%d byte%s)", length,
                                 plurality(length, "", "s"));
        }
    }
}

static int
dcerpc_try_handoff (packet_info *pinfo, proto_tree *tree,
                    proto_tree *dcerpc_tree,
                    tvbuff_t *volatile tvb, tvbuff_t *decrypted_tvb,
                    guint8 *drep, dcerpc_info *info,
                    dcerpc_auth_info *auth_info)
{
    volatile gint offset = 0;
    dcerpc_uuid_key key;
    dcerpc_uuid_value *sub_proto;
    proto_tree *volatile sub_tree = NULL;
    dcerpc_sub_dissector *proc;
    gchar *name = NULL;
    dcerpc_dissect_fnct_t *volatile sub_dissect;
    const char *volatile saved_proto;
    void *volatile saved_private_data;
    guint length, reported_length;
    tvbuff_t *volatile stub_tvb;
    volatile guint auth_pad_len;
    volatile int auth_pad_offset;

    key.uuid = info->call_data->uuid;
    key.ver = info->call_data->ver;


    if ((sub_proto = g_hash_table_lookup (dcerpc_uuids, &key)) == NULL
         || !proto_is_protocol_enabled(sub_proto->proto)) {
        /*
         * We don't have a dissector for this UUID, or the protocol
         * for that UUID is disabled.
         */

	proto_tree_add_boolean_hidden(dcerpc_tree, hf_dcerpc_unknown_if_id,
					  tvb, offset, 0, TRUE);
	if (check_col (pinfo->cinfo, COL_INFO)) {
		col_append_fstr (pinfo->cinfo, COL_INFO, " UNKUUID: %08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x rpcver: %u",
			info->call_data->uuid.Data1, info->call_data->uuid.Data2, info->call_data->uuid.Data3, info->call_data->uuid.Data4[0],
			info->call_data->uuid.Data4[1], info->call_data->uuid.Data4[2], info->call_data->uuid.Data4[3],
			info->call_data->uuid.Data4[4], info->call_data->uuid.Data4[5], info->call_data->uuid.Data4[6],
			info->call_data->uuid.Data4[7], info->call_data->ver);
	}

        if (decrypted_tvb != NULL) {
            show_stub_data (decrypted_tvb, 0, dcerpc_tree, auth_info,
                            FALSE);
        } else
            show_stub_data (tvb, 0, dcerpc_tree, auth_info, TRUE);
        return -1;
    }

    for (proc = sub_proto->procs; proc->name; proc++) {
        if (proc->num == info->call_data->opnum) {
            name = proc->name;
            break;
        }
    }

    if (!name)
        name = "Unknown?!";

    if (check_col (pinfo->cinfo, COL_PROTOCOL)) {
        col_set_str (pinfo->cinfo, COL_PROTOCOL, sub_proto->name);
    }

    if (check_col (pinfo->cinfo, COL_INFO)) {
        col_add_fstr (pinfo->cinfo, COL_INFO, "%s %s",
                      name, info->request ? "request" : "response");
    }

    if (tree) {
        proto_item *sub_item;
        sub_item = proto_tree_add_item (tree, sub_proto->proto_id, tvb, 0,
                                        -1, FALSE);

        if (sub_item) {
            sub_tree = proto_item_add_subtree (sub_item, sub_proto->ett);
            proto_item_append_text(sub_item, ", %s", name);
        }

        /*
         * Put the operation number into the tree along with
         * the operation's name.
         */

	if (sub_proto->opnum_hf != -1)
            proto_tree_add_uint_format(sub_tree, sub_proto->opnum_hf,
                                       tvb, 0, 0, info->call_data->opnum,
                                       "Operation: %s (%u)",
                                       name, info->call_data->opnum);
	else
            proto_tree_add_uint_format(sub_tree, hf_dcerpc_op, tvb,
                                       0, 0, info->call_data->opnum,
                                       "Operation: %s (%u)",
                                       name, info->call_data->opnum);
    }

    sub_dissect = info->request ? proc->dissect_rqst : proc->dissect_resp;

    if (decrypted_tvb != NULL) {
        /* Either there was no encryption or we successfully decrypted
           the entrypted payload. */
        if (sub_dissect) {
            /* We have a subdissector - call it. */
            saved_proto = pinfo->current_proto;
            saved_private_data = pinfo->private_data;
            pinfo->current_proto = sub_proto->name;
            pinfo->private_data = (void *)info;
	    
            init_ndr_pointer_list(pinfo);

            /*
             * Remove the authentication padding from the stub data.
             */
            if (auth_info != NULL && auth_info->auth_pad_len != 0) {
                length = tvb_length(decrypted_tvb);
                reported_length = tvb_reported_length(decrypted_tvb);
                if (reported_length >= auth_info->auth_pad_len) {
                    /*
                     * OK, the padding length isn't so big that it
                     * exceeds the stub length.  Trim the reported
                     * length of the tvbuff.
                     */
                    reported_length -= auth_info->auth_pad_len;

                    /*
                     * If that exceeds the actual amount of data in
                     * the tvbuff (which means we have at least one
                     * byte of authentication padding in the tvbuff),
                     * trim the actual amount.
                     */
                    if (length > reported_length)
                        length = reported_length;

                    stub_tvb = tvb_new_subset(tvb, 0, length, reported_length);
                    auth_pad_len = auth_info->auth_pad_len;
                    auth_pad_offset = reported_length;
                } else {
                    /*
                     * The padding length exceeds the stub length.
                     * Don't bother dissecting the stub, trim the padding
                     * length to what's in the stub data, and show the
                     * entire stub as authentication padding.
                     */
                    stub_tvb = NULL;
                    auth_pad_len = reported_length;
                    auth_pad_offset = 0;
                }
            } else {
                /*
                 * No authentication padding.
                 */
                stub_tvb = decrypted_tvb;
                auth_pad_len = 0;
                auth_pad_offset = 0;
            }

            if (stub_tvb != NULL) {
                /*
                 * Catch all exceptions other than BoundsError, so that even
                 * if the stub data is bad, we still show the authentication
                 * padding, if any.
                 *
                 * If we get BoundsError, it means the frame was cut short
                 * by a snapshot length, so there's nothing more to
                 * dissect; just re-throw that exception.
                 */
                TRY {
                    offset = sub_dissect (decrypted_tvb, 0, pinfo, sub_tree,
                                          drep);

                    /* If we have a subdissector and it didn't dissect all
                       data in the tvb, make a note of it. */

                    if (tvb_reported_length_remaining(stub_tvb, offset) > 0) {
                        if (check_col(pinfo->cinfo, COL_INFO))
                            col_append_fstr(pinfo->cinfo, COL_INFO,
                                            "[Long frame (%d bytes)]",
                                            tvb_reported_length_remaining(stub_tvb, offset));
                    }
                } CATCH(BoundsError) {
                    RETHROW;
                } CATCH_ALL {
                    show_exception(decrypted_tvb, pinfo, tree, EXCEPT_CODE);
                } ENDTRY;
            }

            /* If there is auth padding at the end of the stub, display it */
            if (auth_pad_len != 0) {
                proto_tree_add_text (sub_tree, decrypted_tvb, auth_pad_offset,
                                     auth_pad_len,
                                     "Auth Padding (%u byte%s)",
                                     auth_pad_len,
                                     plurality(auth_pad_len, "", "s"));
            }

            pinfo->current_proto = saved_proto;
            pinfo->private_data = saved_private_data;
        } else {
            /* No subdissector - show it as stub data. */
            if(decrypted_tvb){
               show_stub_data (decrypted_tvb, 0, sub_tree, auth_info, FALSE);
            } else {
               show_stub_data (tvb, 0, sub_tree, auth_info, TRUE);
            }
        }
    } else
        show_stub_data (tvb, 0, sub_tree, auth_info, TRUE);

    tap_queue_packet(dcerpc_tap, pinfo, info);
    return 0;
}

static int
dissect_dcerpc_verifier (tvbuff_t *tvb, packet_info *pinfo, 
			 proto_tree *dcerpc_tree, e_dce_cn_common_hdr_t *hdr,
			 dcerpc_auth_info *auth_info)
{
    int auth_offset;

    auth_info->auth_data = NULL;

    if (auth_info->auth_size != 0) {
	dcerpc_auth_subdissector_fns *auth_fns;
	tvbuff_t *auth_tvb;

	auth_offset = hdr->frag_len - hdr->auth_len;

	auth_tvb = tvb_new_subset(tvb, auth_offset, hdr->auth_len,
				  hdr->auth_len);

	auth_info->auth_data = auth_tvb;

	if ((auth_fns = get_auth_subdissector_fns(auth_info->auth_level,
						  auth_info->auth_type))) {
	    /*
	     * Catch all exceptions, so that even if the verifier is bad
	     * or we don't have all of it, we still show the stub data.
	     */
	    TRY {
		dissect_auth_verf(auth_tvb, pinfo, dcerpc_tree, auth_fns,
				  hdr, auth_info);
	    } CATCH_ALL {
		show_exception(auth_tvb, pinfo, dcerpc_tree, EXCEPT_CODE);
	    } ENDTRY;
	} else {
	    proto_tree_add_text (dcerpc_tree, auth_tvb, 0, hdr->auth_len,
				 "Auth Verifier");
	}
    }

    return hdr->auth_len;
}

static void
dissect_dcerpc_cn_auth (tvbuff_t *tvb, int stub_offset, packet_info *pinfo,
                        proto_tree *dcerpc_tree, e_dce_cn_common_hdr_t *hdr,
                        gboolean are_credentials, dcerpc_auth_info *auth_info)
{
    volatile int offset;

    /*
     * Initially set auth_level and auth_type to zero to indicate that we 
     * haven't yet seen any authentication level information.
     */
    auth_info->auth_level = 0;
    auth_info->auth_type = 0;
    auth_info->auth_size = 0;
    auth_info->auth_pad_len = 0;
    
    /*
     * The authentication information is at the *end* of the PDU; in
     * request and response PDUs, the request and response stub data
     * come before it.
     *
     * Is there any authentication data (i.e., is the authentication length
     * non-zero), and is the authentication length valid (i.e., is it, plus
     * 8 bytes for the type/level/pad length/reserved/context id, less than
     * or equal to the fragment length minus the starting offset of the
     * stub data?)
     */

    if (hdr->auth_len
        && (hdr->auth_len + 8 <= hdr->frag_len - stub_offset)) {

        /*
         * Yes, there is authentication data, and the length is valid.
         * Do we have all the bytes of stub data?
         * (If not, we'd throw an exception dissecting *that*, so don't
         * bother trying to dissect the authentication information and
         * throwing another exception there.)
         */
        offset = hdr->frag_len - (hdr->auth_len + 8);
        if (offset == 0 || tvb_offset_exists(tvb, offset - 1)) {
            /*
             * Either there's no stub data, or the last byte of the stub
             * data is present in the captured data, so we shouldn't
             * get a BoundsError dissecting the stub data.
             *
             * Try dissecting the authentication data.
             * Catch all exceptions, so that even if the auth info is bad
             * or we don't have all of it, we still show the stuff we
             * dissect after this, such as stub data.
             */
            TRY {
                offset = dissect_dcerpc_uint8 (tvb, offset, pinfo, dcerpc_tree, hdr->drep,
                                               hf_dcerpc_auth_type, 
                                               &auth_info->auth_type);
                offset = dissect_dcerpc_uint8 (tvb, offset, pinfo, dcerpc_tree, hdr->drep,
                                               hf_dcerpc_auth_level, 
                                               &auth_info->auth_level);

                offset = dissect_dcerpc_uint8 (tvb, offset, pinfo, dcerpc_tree, hdr->drep,
                                               hf_dcerpc_auth_pad_len, 
                                               &auth_info->auth_pad_len);
                offset = dissect_dcerpc_uint8 (tvb, offset, pinfo, dcerpc_tree, hdr->drep,
                                               hf_dcerpc_auth_rsrvd, NULL);
                offset = dissect_dcerpc_uint32 (tvb, offset, pinfo, dcerpc_tree, hdr->drep,
                                                hf_dcerpc_auth_ctx_id, NULL);

                /*
                 * Dissect the authentication data.
                 */
                if (are_credentials) {
                    tvbuff_t *auth_tvb;
                    dcerpc_auth_subdissector_fns *auth_fns;

                    auth_tvb = tvb_new_subset(tvb, offset, hdr->auth_len,
                                              hdr->auth_len);

                    if ((auth_fns = get_auth_subdissector_fns(auth_info->auth_level,
                                                              auth_info->auth_type)))
                        dissect_auth_verf(auth_tvb, pinfo, dcerpc_tree, auth_fns, 
                                          hdr, auth_info);
                    else
                        proto_tree_add_text (dcerpc_tree, tvb, offset, hdr->auth_len,
                                             "Auth Credentials");
                }
	
                /* Compute the size of the auth block.  Note that this should not 
                   include auth padding, since when NTLMSSP encryption is used, the
                   padding is actually inside the encrypted stub */
                   auth_info->auth_size = hdr->auth_len + 8;
            } CATCH_ALL {
                show_exception(tvb, pinfo, dcerpc_tree, EXCEPT_CODE);
            } ENDTRY;
        }
    }
}


/* We need to hash in the SMB fid number to generate a unique hash table
   key as DCERPC over SMB allows several pipes over the same TCP/IP
   socket. */

static guint16 get_smb_fid (void *private_data)
{
    dcerpc_private_info *priv = (dcerpc_private_info *)private_data;

    if (!priv)
        return 0;	/* Nothing to see here */

    /* DCERPC over smb */

    if (priv->transport_type == DCERPC_TRANSPORT_SMB)
        return priv->data.smb.fid;

    /* Some other transport... */

    return 0;
}

/*
 * Connection oriented packet types
 */

static void
dissect_dcerpc_cn_bind (tvbuff_t *tvb, gint offset, packet_info *pinfo, 
			proto_tree *dcerpc_tree, e_dce_cn_common_hdr_t *hdr)
{
    conversation_t *conv = NULL;
    guint8 num_ctx_items = 0;
    guint i;
    gboolean saw_ctx_item = FALSE;
    guint16 ctx_id;
    guint16 num_trans_items;
    guint j;
    e_uuid_t if_id;
    e_uuid_t trans_id;
    guint32 trans_ver;
    guint16 if_ver, if_ver_minor;
    char uuid_str[DCERPC_UUID_STR_LEN]; 
    int uuid_str_len;
    dcerpc_auth_info auth_info;
#ifdef WIN32
    char UUID_NAME[MAX_PATH];
#endif

    offset = dissect_dcerpc_uint16 (tvb, offset, pinfo, dcerpc_tree, hdr->drep,
                                    hf_dcerpc_cn_max_xmit, NULL);

    offset = dissect_dcerpc_uint16 (tvb, offset, pinfo, dcerpc_tree, hdr->drep,
                                    hf_dcerpc_cn_max_recv, NULL);

    offset = dissect_dcerpc_uint32 (tvb, offset, pinfo, dcerpc_tree, hdr->drep,
                                    hf_dcerpc_cn_assoc_group, NULL);

    offset = dissect_dcerpc_uint8 (tvb, offset, pinfo, dcerpc_tree, hdr->drep,
                                    hf_dcerpc_cn_num_ctx_items, &num_ctx_items);

    /* padding */
    offset += 3;

    for (i = 0; i < num_ctx_items; i++) {
	    proto_tree *ctx_tree = NULL, *iface_tree = NULL;

      offset = dissect_dcerpc_uint16 (tvb, offset, pinfo, NULL, hdr->drep,
                                      hf_dcerpc_cn_ctx_id, &ctx_id);

      if (dcerpc_tree) {
	      proto_item *ctx_item;

	      ctx_item = proto_tree_add_item(dcerpc_tree, hf_dcerpc_cn_ctx_id,
					     tvb, offset - 2, 2, 
					     hdr->drep[0] & 0x10);

	      ctx_tree = proto_item_add_subtree(ctx_item, ett_dcerpc_cn_ctx);
      }

      offset = dissect_dcerpc_uint16 (tvb, offset, pinfo, ctx_tree, hdr->drep,
                                      hf_dcerpc_cn_num_trans_items, &num_trans_items);

      /* XXX - use "dissect_ndr_uuid_t()"? */
      dcerpc_tvb_get_uuid (tvb, offset, hdr->drep, &if_id);
      if (ctx_tree) {
	  proto_item *iface_item;

	  uuid_str_len = snprintf(uuid_str, DCERPC_UUID_STR_LEN, 
			          "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                                  if_id.Data1, if_id.Data2, if_id.Data3,
                                  if_id.Data4[0], if_id.Data4[1],
                                  if_id.Data4[2], if_id.Data4[3],
                                  if_id.Data4[4], if_id.Data4[5],
                                  if_id.Data4[6], if_id.Data4[7]);

	  if (uuid_str_len >= DCERPC_UUID_STR_LEN)
		  memset(uuid_str, 0, DCERPC_UUID_STR_LEN);
#ifdef WIN32
	  if(ResolveWin32UUID(if_id, UUID_NAME, MAX_PATH))
		  iface_item = proto_tree_add_string_format (ctx_tree, hf_dcerpc_cn_bind_if_id, tvb,
                                        offset, 16, uuid_str, "Interface [%s] UUID: %s", UUID_NAME, uuid_str);
	  else
#endif
          iface_item = proto_tree_add_string_format (ctx_tree, hf_dcerpc_cn_bind_if_id, tvb,
                                        offset, 16, uuid_str, "Interface UUID: %s", uuid_str);
	  iface_tree = proto_item_add_subtree(iface_item, ett_dcerpc_cn_iface);
      }
      offset += 16;

      if (hdr->drep[0] & 0x10) {
          offset = dissect_dcerpc_uint16 (tvb, offset, pinfo, iface_tree, hdr->drep,
                                          hf_dcerpc_cn_bind_if_ver, &if_ver);
          offset = dissect_dcerpc_uint16 (tvb, offset, pinfo, iface_tree, hdr->drep,
                                          hf_dcerpc_cn_bind_if_ver_minor, &if_ver_minor);
      } else {
          offset = dissect_dcerpc_uint16 (tvb, offset, pinfo, iface_tree, hdr->drep,
                                          hf_dcerpc_cn_bind_if_ver_minor, &if_ver_minor);
          offset = dissect_dcerpc_uint16 (tvb, offset, pinfo, iface_tree, hdr->drep,
                                          hf_dcerpc_cn_bind_if_ver, &if_ver);
      }

      if (!saw_ctx_item) {
        conv = find_conversation (&pinfo->src, &pinfo->dst, pinfo->ptype,
                                  pinfo->srcport, pinfo->destport, 0);
        if (conv == NULL) {
            conv = conversation_new (&pinfo->src, &pinfo->dst, pinfo->ptype,
                                     pinfo->srcport, pinfo->destport, 0);
        }

	/* if this is the first time we see this packet, we need to
	   update the dcerpc_binds table so that any later calls can
	   match to the interface.
	   XXX We assume that BINDs will NEVER be fragmented.
	*/
	if(!(pinfo->fd->flags.visited)){
		dcerpc_bind_key *key;
		dcerpc_bind_value *value;

	        key = g_mem_chunk_alloc (dcerpc_bind_key_chunk);
        	key->conv = conv;
        	key->ctx_id = ctx_id;
        	key->smb_fid = get_smb_fid(pinfo->private_data);

        	value = g_mem_chunk_alloc (dcerpc_bind_value_chunk);
        	value->uuid = if_id;
        	value->ver = if_ver;

		/* add this entry to the bind table, first removing any
		   previous ones that are identical
		 */
		if(g_hash_table_lookup(dcerpc_binds, key)){
			g_hash_table_remove(dcerpc_binds, key);
		}
        	g_hash_table_insert (dcerpc_binds, key, value);
	}

        if (check_col (pinfo->cinfo, COL_INFO)) {
	  dcerpc_uuid_key key;
	  dcerpc_uuid_value *value;

	  key.uuid = if_id;
	  key.ver = if_ver;

	  if (num_ctx_items > 1)
		  col_append_fstr(pinfo->cinfo, COL_INFO, ", %u context items, 1st", num_ctx_items);
		
	  if ((value = g_hash_table_lookup(dcerpc_uuids, &key)))
		  col_append_fstr(pinfo->cinfo, COL_INFO, " UUID: %s", value->name);
	  else
#ifdef WIN32
		if(ResolveWin32UUID(if_id, UUID_NAME, MAX_PATH))
			col_append_fstr(pinfo->cinfo, COL_INFO, " [%s] UUID: %08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x ver %u.%u",
                           UUID_NAME, if_id.Data1, if_id.Data2, if_id.Data3,
                           if_id.Data4[0], if_id.Data4[1],
                           if_id.Data4[2], if_id.Data4[3],
                           if_id.Data4[4], if_id.Data4[5],
                           if_id.Data4[6], if_id.Data4[7],
                           if_ver, if_ver_minor);
	  else
#endif
			col_append_fstr(pinfo->cinfo, COL_INFO, " UUID: %08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x ver %u.%u",
                           if_id.Data1, if_id.Data2, if_id.Data3,
                           if_id.Data4[0], if_id.Data4[1],
                           if_id.Data4[2], if_id.Data4[3],
                           if_id.Data4[4], if_id.Data4[5],
                           if_id.Data4[6], if_id.Data4[7],
                           if_ver, if_ver_minor);
        }
        saw_ctx_item = TRUE;
      }

      for (j = 0; j < num_trans_items; j++) {
        /* XXX - use "dissect_ndr_uuid_t()"? */
        dcerpc_tvb_get_uuid (tvb, offset, hdr->drep, &trans_id);
        if (iface_tree) {
	    uuid_str_len = snprintf(uuid_str, DCERPC_UUID_STR_LEN, 
                                  "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                                  trans_id.Data1, trans_id.Data2, trans_id.Data3,
                                  trans_id.Data4[0], trans_id.Data4[1],
                                  trans_id.Data4[2], trans_id.Data4[3],
                                  trans_id.Data4[4], trans_id.Data4[5],
                                  trans_id.Data4[6], trans_id.Data4[7]);
            if (uuid_str_len >= DCERPC_UUID_STR_LEN)
                memset(uuid_str, 0, DCERPC_UUID_STR_LEN);
            proto_tree_add_string_format (iface_tree, hf_dcerpc_cn_bind_trans_id, tvb,
                                          offset, 16, uuid_str, "Transfer Syntax: %s", uuid_str);
        }
        offset += 16;

        offset = dissect_dcerpc_uint32 (tvb, offset, pinfo, iface_tree, hdr->drep,
                                        hf_dcerpc_cn_bind_trans_ver, &trans_ver);
      }
    }

    /*
     * XXX - we should save the authentication type *if* we have
     * an authentication header, and associate it with an authentication
     * context, so subsequent PDUs can use that context.
     */
    dissect_dcerpc_cn_auth (tvb, offset, pinfo, dcerpc_tree, hdr, TRUE, &auth_info);
}

static void
dissect_dcerpc_cn_bind_ack (tvbuff_t *tvb, gint offset, packet_info *pinfo, 
			    proto_tree *dcerpc_tree, e_dce_cn_common_hdr_t *hdr)
{
    guint16 max_xmit, max_recv;
    guint16 sec_addr_len;
    guint8 num_results;
    guint i;
    guint16 result;
    guint16 reason;
    e_uuid_t trans_id;
    guint32 trans_ver;
    char uuid_str[DCERPC_UUID_STR_LEN]; 
    int uuid_str_len;
    dcerpc_auth_info auth_info;

    offset = dissect_dcerpc_uint16 (tvb, offset, pinfo, dcerpc_tree, hdr->drep,
                                    hf_dcerpc_cn_max_xmit, &max_xmit);

    offset = dissect_dcerpc_uint16 (tvb, offset, pinfo, dcerpc_tree, hdr->drep,
                                    hf_dcerpc_cn_max_recv, &max_recv);

    offset = dissect_dcerpc_uint32 (tvb, offset, pinfo, dcerpc_tree, hdr->drep,
                                    hf_dcerpc_cn_assoc_group, NULL);

    offset = dissect_dcerpc_uint16 (tvb, offset, pinfo, dcerpc_tree, hdr->drep,
                                    hf_dcerpc_cn_sec_addr_len, &sec_addr_len);
    if (sec_addr_len != 0) {
        proto_tree_add_item (dcerpc_tree, hf_dcerpc_cn_sec_addr, tvb, offset,
                             sec_addr_len, FALSE);
        offset += sec_addr_len;
    }

    if (offset % 4) {
        offset += 4 - offset % 4;
    }

    offset = dissect_dcerpc_uint8 (tvb, offset, pinfo, dcerpc_tree, hdr->drep,
                                   hf_dcerpc_cn_num_results, &num_results);

    /* padding */
    offset += 3;

    for (i = 0; i < num_results; i++) {
        offset = dissect_dcerpc_uint16 (tvb, offset, pinfo, dcerpc_tree,
                                        hdr->drep, hf_dcerpc_cn_ack_result,
                                        &result);
        if (result != 0) {
            offset = dissect_dcerpc_uint16 (tvb, offset, pinfo, dcerpc_tree,
                                            hdr->drep, hf_dcerpc_cn_ack_reason,
                                            &reason);
        } else {
            /*
             * The reason for rejection isn't meaningful, and often isn't
             * set, when the syntax was accepted.
             */
            offset += 2;
        }

        /* XXX - use "dissect_ndr_uuid_t()"? */
        dcerpc_tvb_get_uuid (tvb, offset, hdr->drep, &trans_id);
        if (dcerpc_tree) {
	    uuid_str_len = snprintf(uuid_str, DCERPC_UUID_STR_LEN, 
                                  "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                                  trans_id.Data1, trans_id.Data2, trans_id.Data3,
                                  trans_id.Data4[0], trans_id.Data4[1],
                                  trans_id.Data4[2], trans_id.Data4[3],
                                  trans_id.Data4[4], trans_id.Data4[5],
                                  trans_id.Data4[6], trans_id.Data4[7]);
	    if (uuid_str_len >= DCERPC_UUID_STR_LEN)
		  memset(uuid_str, 0, DCERPC_UUID_STR_LEN);
            proto_tree_add_string_format (dcerpc_tree, hf_dcerpc_cn_ack_trans_id, tvb,
                                          offset, 16, uuid_str, "Transfer Syntax: %s", uuid_str);
        }
        offset += 16;

        offset = dissect_dcerpc_uint32 (tvb, offset, pinfo, dcerpc_tree, hdr->drep,
                                        hf_dcerpc_cn_ack_trans_ver, &trans_ver);
    }

    /*
     * XXX - do we need to do anything with the authentication level
     * we get back from this?
     */
    dissect_dcerpc_cn_auth (tvb, offset, pinfo, dcerpc_tree, hdr, TRUE, &auth_info);

    if (check_col (pinfo->cinfo, COL_INFO)) {
        if (num_results != 0 && result == 0) {
            /* XXX - only checks the last result */
            col_append_fstr (pinfo->cinfo, COL_INFO,
                             " accept max_xmit: %u max_recv: %u",
                             max_xmit, max_recv);
        } else {
            /* XXX - only shows the last result and reason */
            col_append_fstr (pinfo->cinfo, COL_INFO, " %s, reason: %s",
                             val_to_str(result, p_cont_result_vals,
                                        "Unknown result (%u)"),
                             val_to_str(reason, p_provider_reason_vals,
                                        "Unknown (%u)"));
        }
    }
}

static void
dissect_dcerpc_cn_bind_nak (tvbuff_t *tvb, gint offset, packet_info *pinfo, 
			    proto_tree *dcerpc_tree, e_dce_cn_common_hdr_t *hdr)
{
    guint16 reason;
    guint8 num_protocols;
    guint i;

    offset = dissect_dcerpc_uint16 (tvb, offset, pinfo, dcerpc_tree,
                                    hdr->drep, hf_dcerpc_cn_reject_reason,
                                    &reason);

    if (check_col (pinfo->cinfo, COL_INFO)) {
        col_append_fstr (pinfo->cinfo, COL_INFO, " reason: %s",
                      val_to_str(reason, reject_reason_vals, "Unknown (%u)"));
    }

    if (reason == PROTOCOL_VERSION_NOT_SUPPORTED) {
        offset = dissect_dcerpc_uint8 (tvb, offset, pinfo, dcerpc_tree, hdr->drep,
                                       hf_dcerpc_cn_num_protocols,
                                       &num_protocols);

        for (i = 0; i < num_protocols; i++) {
            offset = dissect_dcerpc_uint8 (tvb, offset, pinfo, dcerpc_tree,
                                        hdr->drep, hf_dcerpc_cn_protocol_ver_major,
                                        NULL);
            offset = dissect_dcerpc_uint8 (tvb, offset, pinfo, dcerpc_tree,
                                        hdr->drep, hf_dcerpc_cn_protocol_ver_minor,
                                        NULL);
        }
    }
}

/* Return a string describing a DCE/RPC fragment as first, middle, or end
   fragment. */

#define PFC_FRAG_MASK  0x03

static char *
fragment_type(guint8 flags)
{
	flags = flags & PFC_FRAG_MASK;

	if (flags == PFC_FIRST_FRAG)
		return "first";

	if (flags == 0)
		return "middle";

	if (flags == PFC_LAST_FRAG)
		return "last";

	if (flags == (PFC_FIRST_FRAG | PFC_LAST_FRAG))
		return "whole";

	return "unknown";
}

/* Dissect stub data (payload) of a DCERPC packet. */

static void
dissect_dcerpc_cn_stub (tvbuff_t *tvb, int offset, packet_info *pinfo,
                        proto_tree *dcerpc_tree, proto_tree *tree,
                        e_dce_cn_common_hdr_t *hdr, dcerpc_info *di,
                        dcerpc_auth_info *auth_info, guint32 alloc_hint,
                        guint32 frame)
{
    gboolean save_fragmented;
    fragment_data *fd_head=NULL;
    guint32 tot_len;
    tvbuff_t *payload_tvb, *decrypted_tvb;
    proto_item *pi;

    save_fragmented = pinfo->fragmented;

    payload_tvb = tvb_new_subset(
	    tvb, offset, tvb_length_remaining(tvb, offset) - 
	    auth_info->auth_size, tvb_length_remaining(tvb, offset) - 
	    auth_info->auth_size);    

    /* Decrypt the PDU if it is encrypted */

    if (auth_info->auth_type &&
        auth_info->auth_level == DCE_C_AUTHN_LEVEL_PKT_PRIVACY) {
	    /*
	     * We know the authentication type, and the authentication
	     * level is "Packet privacy", meaning the payload is
	     * encrypted; attempt to decrypt it.
	     */
	    dcerpc_auth_subdissector_fns *auth_fns;
	    
	    /* Start out assuming we won't succeed in decrypting. */
	    decrypted_tvb = NULL;

	    if ((auth_fns = get_auth_subdissector_fns(
			 auth_info->auth_level, auth_info->auth_type))) {
		    tvbuff_t *result;
		    
		    result = decode_encrypted_data(
			    payload_tvb, pinfo, auth_fns,
			    hdr->ptype == PDU_REQ, auth_info);	    
		    
		    if (result) {
			    if (dcerpc_tree)
				proto_tree_add_text(
					    dcerpc_tree, payload_tvb, 0, -1,
					    "Encrypted stub data (%d byte%s)",
					    tvb_reported_length(payload_tvb),

			    plurality(tvb_length(payload_tvb), "", "s"));

			    add_new_data_source(
				    pinfo, result, "Decrypted stub data");
			    
			    /* We succeeded. */
			    decrypted_tvb = result;
		    }
	    }
    } else
	    decrypted_tvb = payload_tvb;

    /* if this packet is not fragmented, just dissect it and exit */
    if(PFC_NOT_FRAGMENTED(hdr)){
	pinfo->fragmented = FALSE;

	dcerpc_try_handoff(
		pinfo, tree, dcerpc_tree, payload_tvb, decrypted_tvb,
		hdr->drep, di, auth_info);
	
	pinfo->fragmented = save_fragmented;
	return;
    }

    /* The packet is fragmented. */
    pinfo->fragmented = TRUE;

    /* if we are not doing reassembly and this is the first fragment
       then just dissect it and exit
       XXX - if we're not doing reassembly, can we decrypt an
       encrypted stub?
    */
    if( (!dcerpc_reassemble) && hdr->flags&PFC_FIRST_FRAG ){

	dcerpc_try_handoff(
		pinfo, tree, dcerpc_tree, payload_tvb, decrypted_tvb,
		hdr->drep, di, auth_info);
	
        if (check_col(pinfo->cinfo, COL_INFO)) {
            col_append_fstr(pinfo->cinfo, COL_INFO,
                            " [DCE/RPC %s fragment]", fragment_type(hdr->flags));
        }
        pinfo->fragmented = save_fragmented;
        return;
    }

    /* if we have already seen this packet, see if it was reassembled
       and if so dissect the full pdu.
       then exit 
    */
    if(pinfo->fd->flags.visited){
	fd_head=fragment_get(pinfo, frame, dcerpc_co_reassemble_table);
	goto end_cn_stub;
    }

    /* if we are not doing reassembly and it was neither a complete PDU
       nor the first fragment then there is nothing more we can do
       so we just have to exit
    */
    if( !dcerpc_reassemble )
        goto end_cn_stub;

    /* if we didnt get 'frame' we dont know where the PDU started and thus
       it is pointless to continue 
    */
    if(!frame)
        goto end_cn_stub;

    /* from now on we must attempt to reassemble the PDU 
    */

    /* if we get here we know it is the first time we see the packet
       and we also know it is only a fragment and not a full PDU,
       thus we must reassemble it.
    */

    /* Do we have any non-encrypted data to reassemble? */
    if (decrypted_tvb == NULL) {
      /* No.  We can't even try to reassemble.  */
      goto end_cn_stub;
    }

    /* if this is the first fragment we need to start reassembly
    */
    if(hdr->flags&PFC_FIRST_FRAG){
	fragment_add(decrypted_tvb, 0, pinfo, frame, dcerpc_co_reassemble_table,
		     0, tvb_length(decrypted_tvb), TRUE);
	fragment_set_tot_len(pinfo, frame,
		 dcerpc_co_reassemble_table, alloc_hint);

	goto end_cn_stub;
    }

    /* if this is a middle fragment, just add it and exit */
    if(!(hdr->flags&PFC_LAST_FRAG)){
	tot_len = fragment_get_tot_len(pinfo, frame,
		 dcerpc_co_reassemble_table);
	fragment_add(decrypted_tvb, 0, pinfo, frame,
		 dcerpc_co_reassemble_table,
		 tot_len-alloc_hint, tvb_length(decrypted_tvb),
		 TRUE);

	goto end_cn_stub;
    }

    /* this was the last fragment add it to reassembly
    */
    tot_len = fragment_get_tot_len(pinfo, frame,
		dcerpc_co_reassemble_table);
    fd_head = fragment_add(decrypted_tvb, 0, pinfo,
		frame,
		dcerpc_co_reassemble_table,
		tot_len-alloc_hint, tvb_length(decrypted_tvb),
		TRUE);

end_cn_stub:

    /* if reassembly is complete, dissect the full PDU
    */
    if(fd_head && (fd_head->flags&FD_DEFRAGMENTED) ){

	if(pinfo->fd->num==fd_head->reassembled_in){
	    tvbuff_t *next_tvb;

	    next_tvb = tvb_new_real_data(fd_head->data, fd_head->datalen, fd_head->datalen);
	    tvb_set_child_real_data_tvbuff(decrypted_tvb, next_tvb);
	    add_new_data_source(pinfo, next_tvb, "Reassembled DCE/RPC");
	    show_fragment_tree(fd_head, &dcerpc_frag_items,
		dcerpc_tree, pinfo, next_tvb);

	    pinfo->fragmented = FALSE;

	    dcerpc_try_handoff (pinfo, tree, dcerpc_tree, next_tvb,
		next_tvb, hdr->drep, di, auth_info);

	} else {
	    pi = proto_tree_add_uint(dcerpc_tree, hf_dcerpc_reassembled_in,
				decrypted_tvb, 0, 0, fd_head->reassembled_in);
        PROTO_ITEM_SET_GENERATED(pi);
	    if (check_col(pinfo->cinfo, COL_INFO)) {
		col_append_fstr(pinfo->cinfo, COL_INFO,
			" [DCE/RPC %s fragment, reas: #%u]", fragment_type(hdr->flags), fd_head->reassembled_in);
	    }
	}
    } else {
	/* Reassembly not complete - some fragments
	   are missing.  Just show the stub data. */

	if (check_col(pinfo->cinfo, COL_INFO)) {
	    col_append_fstr(pinfo->cinfo, COL_INFO,
			" [DCE/RPC %s fragment]", fragment_type(hdr->flags));
	}

	if(decrypted_tvb){
	        show_stub_data (decrypted_tvb, 0, tree, auth_info, FALSE);
	} else {
	        show_stub_data (payload_tvb, 0, tree, auth_info, TRUE);
	}
    }

    pinfo->fragmented = save_fragmented;
}

static void
dissect_dcerpc_cn_rqst (tvbuff_t *tvb, gint offset, packet_info *pinfo, 
			proto_tree *dcerpc_tree, proto_tree *tree, e_dce_cn_common_hdr_t *hdr)
{
    conversation_t *conv;
    guint16 ctx_id;
    guint16 opnum;
    e_uuid_t obj_id;
    dcerpc_auth_info auth_info;
    guint32 alloc_hint;
    char uuid_str[DCERPC_UUID_STR_LEN]; 
    int uuid_str_len;
    proto_item *pi;

    offset = dissect_dcerpc_uint32 (tvb, offset, pinfo, dcerpc_tree, hdr->drep,
                                    hf_dcerpc_cn_alloc_hint, &alloc_hint);

    offset = dissect_dcerpc_uint16 (tvb, offset, pinfo, dcerpc_tree, hdr->drep,
                                    hf_dcerpc_cn_ctx_id, &ctx_id);

    offset = dissect_dcerpc_uint16 (tvb, offset, pinfo, dcerpc_tree, hdr->drep,
                                    hf_dcerpc_opnum, &opnum);

    if (check_col (pinfo->cinfo, COL_INFO)) {
        col_append_fstr (pinfo->cinfo, COL_INFO, " opnum: %u ctx_id: %u",
                         opnum, ctx_id);
    }

    if (hdr->flags & PFC_OBJECT_UUID) {
        /* XXX - use "dissect_ndr_uuid_t()"? */
        dcerpc_tvb_get_uuid (tvb, offset, hdr->drep, &obj_id);
        if (dcerpc_tree) {
	    uuid_str_len = snprintf(uuid_str, DCERPC_UUID_STR_LEN, 
                                    "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                                    obj_id.Data1, obj_id.Data2, obj_id.Data3,
                                    obj_id.Data4[0],
                                    obj_id.Data4[1],
                                    obj_id.Data4[2],
                                    obj_id.Data4[3],
                                    obj_id.Data4[4],
                                    obj_id.Data4[5],
                                    obj_id.Data4[6],
                                    obj_id.Data4[7]);
	    if (uuid_str_len >= DCERPC_UUID_STR_LEN)
		  memset(uuid_str, 0, DCERPC_UUID_STR_LEN);
            proto_tree_add_string_format (dcerpc_tree, hf_dcerpc_obj_id, tvb,
                                          offset, 16, uuid_str, "Object UUID: %s", uuid_str);
        }
        offset += 16;
    }

    /*
     * XXX - what if this was set when the connection was set up,
     * and we just have a security context?
     */
    dissect_dcerpc_cn_auth (tvb, offset, pinfo, dcerpc_tree, hdr, FALSE, &auth_info);
    dissect_dcerpc_verifier (tvb, pinfo, dcerpc_tree, hdr, &auth_info);

    conv = find_conversation (&pinfo->src, &pinfo->dst, pinfo->ptype,
                              pinfo->srcport, pinfo->destport, 0);
    if (!conv)
        show_stub_data (tvb, offset, dcerpc_tree, &auth_info, TRUE);
    else {
        dcerpc_matched_key matched_key, *new_matched_key;
        dcerpc_call_value *value;

	/* !!! we can NOT check flags.visited here since this will interact
	   badly with when SMB handles (i.e. calls the subdissector)
	   and desegmented pdu's .
	   Instead we check if this pdu is already in the matched table or not
	*/
	matched_key.frame = pinfo->fd->num;
	matched_key.call_id = hdr->call_id;
	value = g_hash_table_lookup(dcerpc_matched, &matched_key);
	if(!value){
		dcerpc_bind_key bind_key;
		dcerpc_bind_value *bind_value;

		bind_key.conv=conv;
		bind_key.ctx_id=ctx_id;
		bind_key.smb_fid=get_smb_fid(pinfo->private_data);

		if((bind_value=g_hash_table_lookup(dcerpc_binds, &bind_key)) ){
			if(!(hdr->flags&PFC_FIRST_FRAG)){
				dcerpc_call_key call_key;
				dcerpc_call_value *call_value;

				call_key.conv=conv;
				call_key.call_id=hdr->call_id;
				call_key.smb_fid=get_smb_fid(pinfo->private_data);
				if((call_value=g_hash_table_lookup(dcerpc_calls, &call_key))){
					new_matched_key = g_mem_chunk_alloc(dcerpc_matched_key_chunk);
					*new_matched_key = matched_key;
					g_hash_table_insert (dcerpc_matched, new_matched_key, call_value);
					value = call_value;
				}
			} else {
				dcerpc_call_key *call_key;
				dcerpc_call_value *call_value;

				/* We found the binding and it is the first fragment 
				   (or a complete PDU) of a dcerpc pdu so just add 
				   the call to both the call table and the 
				   matched table
				*/
				call_key=g_mem_chunk_alloc (dcerpc_call_key_chunk);
				call_key->conv=conv;
				call_key->call_id=hdr->call_id;
				call_key->smb_fid=get_smb_fid(pinfo->private_data);

				/* if there is already a matching call in the table
				   remove it so it is replaced with the new one */
				if(g_hash_table_lookup(dcerpc_calls, call_key)){
					g_hash_table_remove(dcerpc_calls, call_key);
				}

				call_value=g_mem_chunk_alloc (dcerpc_call_value_chunk);
				call_value->uuid = bind_value->uuid;
				call_value->ver = bind_value->ver;
				call_value->opnum = opnum;
				call_value->req_frame=pinfo->fd->num;
				call_value->req_time.secs=pinfo->fd->abs_secs;
				call_value->req_time.nsecs=pinfo->fd->abs_usecs*1000;
				call_value->rep_frame=0;
				call_value->max_ptr=0;
				call_value->private_data = NULL;
				g_hash_table_insert (dcerpc_calls, call_key, call_value);

				new_matched_key = g_mem_chunk_alloc(dcerpc_matched_key_chunk);
				*new_matched_key = matched_key;
				g_hash_table_insert (dcerpc_matched, new_matched_key, call_value);
				value = call_value;
			}
		}
	}

        if (value) {
            dcerpc_info *di;

            di=get_next_di();
            /* handoff this call */
	    di->conv = conv;
	    di->call_id = hdr->call_id;
	    di->smb_fid = get_smb_fid(pinfo->private_data);
	    di->request = TRUE;
	    di->call_data = value;
		di->hf_index = -1;

	    if(value->rep_frame!=0){
		pi = proto_tree_add_uint(dcerpc_tree, hf_dcerpc_response_in,
				    tvb, 0, 0, value->rep_frame);
        PROTO_ITEM_SET_GENERATED(pi);
	    }

	    dissect_dcerpc_cn_stub (tvb, offset, pinfo, dcerpc_tree, tree,
				    hdr, di, &auth_info, alloc_hint,
				    value->req_frame);
	} else
	    show_stub_data (tvb, offset, dcerpc_tree, &auth_info, TRUE);
    }
}

static void
dissect_dcerpc_cn_resp (tvbuff_t *tvb, gint offset, packet_info *pinfo,
			proto_tree *dcerpc_tree, proto_tree *tree, e_dce_cn_common_hdr_t *hdr)
{
    dcerpc_call_value *value = NULL;
    conversation_t *conv;
    guint16 ctx_id;
    dcerpc_auth_info auth_info;
    guint32 alloc_hint;
    proto_item *pi;

    offset = dissect_dcerpc_uint32 (tvb, offset, pinfo, dcerpc_tree, hdr->drep,
                                    hf_dcerpc_cn_alloc_hint, &alloc_hint);

    offset = dissect_dcerpc_uint16 (tvb, offset, pinfo, dcerpc_tree, hdr->drep,
                                    hf_dcerpc_cn_ctx_id, &ctx_id);

    if (check_col (pinfo->cinfo, COL_INFO)) {
        col_append_fstr (pinfo->cinfo, COL_INFO, " ctx_id: %u", ctx_id);
    }

    offset = dissect_dcerpc_uint8 (tvb, offset, pinfo, dcerpc_tree, hdr->drep,
                                   hf_dcerpc_cn_cancel_count, NULL);
    /* padding */
    offset++;

    /*
     * XXX - what if this was set when the connection was set up,
     * and we just have a security context?
     */
    dissect_dcerpc_cn_auth (tvb, offset, pinfo, dcerpc_tree, hdr, FALSE, &auth_info);
    dissect_dcerpc_verifier (tvb, pinfo, dcerpc_tree, hdr, &auth_info);

    conv = find_conversation (&pinfo->src, &pinfo->dst, pinfo->ptype,
                              pinfo->srcport, pinfo->destport, 0);

    if (!conv) {
        /* no point in creating one here, really */
        show_stub_data (tvb, offset, dcerpc_tree, &auth_info, TRUE);
    } else {
	dcerpc_matched_key matched_key, *new_matched_key;

	/* !!! we can NOT check flags.visited here since this will interact
	   badly with when SMB handles (i.e. calls the subdissector)
	   and desegmented pdu's .
	   Instead we check if this pdu is already in the matched table or not
	*/
	matched_key.frame = pinfo->fd->num;
	matched_key.call_id = hdr->call_id;
	value=g_hash_table_lookup(dcerpc_matched, &matched_key);
	if(!value){
		dcerpc_call_key call_key;
		dcerpc_call_value *call_value;

		call_key.conv=conv;
		call_key.call_id=hdr->call_id;
		call_key.smb_fid=get_smb_fid(pinfo->private_data);

		if((call_value=g_hash_table_lookup(dcerpc_calls, &call_key))){
			new_matched_key = g_mem_chunk_alloc(dcerpc_matched_key_chunk);
			*new_matched_key = matched_key;
			g_hash_table_insert (dcerpc_matched, new_matched_key, call_value);
			value = call_value;
			if(call_value->rep_frame==0){
				call_value->rep_frame=pinfo->fd->num;
			}
		}
	}

        if (value) {
            dcerpc_info *di;

            di=get_next_di();
            /* handoff this call */
	    di->conv = conv;
	    di->call_id = hdr->call_id;
	    di->smb_fid = get_smb_fid(pinfo->private_data);
	    di->request = FALSE;
	    di->call_data = value;

	    proto_tree_add_uint (dcerpc_tree, hf_dcerpc_opnum, tvb, 0, 0, value->opnum);
	    if(value->req_frame!=0){
		nstime_t ns;
		pi = proto_tree_add_uint(dcerpc_tree, hf_dcerpc_request_in,
				    tvb, 0, 0, value->req_frame);
        PROTO_ITEM_SET_GENERATED(pi);
		ns.secs= pinfo->fd->abs_secs-value->req_time.secs;
		ns.nsecs=pinfo->fd->abs_usecs*1000-value->req_time.nsecs;
		if(ns.nsecs<0){
			ns.nsecs+=1000000000;
			ns.secs--;
		}
		pi = proto_tree_add_time(dcerpc_tree, hf_dcerpc_time, tvb, offset, 0, &ns);
        PROTO_ITEM_SET_GENERATED(pi);
	    }

	    dissect_dcerpc_cn_stub (tvb, offset, pinfo, dcerpc_tree, tree,
				    hdr, di, &auth_info, alloc_hint,
				    value->rep_frame);
        } else
            show_stub_data (tvb, offset, dcerpc_tree, &auth_info, TRUE);
    }
}

static void
dissect_dcerpc_cn_fault (tvbuff_t *tvb, gint offset, packet_info *pinfo,
                         proto_tree *dcerpc_tree, e_dce_cn_common_hdr_t *hdr)
{
    dcerpc_call_value *value = NULL;
    conversation_t *conv;
    guint16 ctx_id;
    guint32 status;
    guint32 alloc_hint;
    dcerpc_auth_info auth_info;
    proto_item *pi;

    offset = dissect_dcerpc_uint32 (tvb, offset, pinfo, dcerpc_tree, hdr->drep,
                                    hf_dcerpc_cn_alloc_hint, &alloc_hint);

    offset = dissect_dcerpc_uint16 (tvb, offset, pinfo, dcerpc_tree, hdr->drep,
                                    hf_dcerpc_cn_ctx_id, &ctx_id);

    offset = dissect_dcerpc_uint8 (tvb, offset, pinfo, dcerpc_tree, hdr->drep,
                                   hf_dcerpc_cn_cancel_count, NULL);
    /* padding */
    offset++;

    offset = dissect_dcerpc_uint32 (tvb, offset, pinfo, dcerpc_tree, hdr->drep,
                                    hf_dcerpc_cn_status, &status);

    if (check_col (pinfo->cinfo, COL_INFO)) {
        col_append_fstr (pinfo->cinfo, COL_INFO,
                      " ctx_id: %u status: %s", ctx_id,
                      val_to_str(status, reject_status_vals,
                                 "Unknown (0x%08x)"));
    }

    /* padding */
    offset += 4;

    /*
     * XXX - what if this was set when the connection was set up,
     * and we just have a security context?
     */
    dissect_dcerpc_cn_auth (tvb, offset, pinfo, dcerpc_tree, hdr, FALSE, &auth_info);

    conv = find_conversation (&pinfo->src, &pinfo->dst, pinfo->ptype,
                              pinfo->srcport, pinfo->destport, 0);
    if (!conv) {
        /* no point in creating one here, really */
    } else {
	dcerpc_matched_key matched_key, *new_matched_key;

	/* !!! we can NOT check flags.visited here since this will interact
	   badly with when SMB handles (i.e. calls the subdissector)
	   and desegmented pdu's .
	   Instead we check if this pdu is already in the matched table or not
	*/
	matched_key.frame = pinfo->fd->num;
	matched_key.call_id = hdr->call_id;
	value=g_hash_table_lookup(dcerpc_matched, &matched_key);
	if(!value){
		dcerpc_call_key call_key;
		dcerpc_call_value *call_value;

		call_key.conv=conv;
		call_key.call_id=hdr->call_id;
		call_key.smb_fid=get_smb_fid(pinfo->private_data);

		if((call_value=g_hash_table_lookup(dcerpc_calls, &call_key))){
			new_matched_key = g_mem_chunk_alloc(dcerpc_matched_key_chunk);
			*new_matched_key = matched_key;
			g_hash_table_insert (dcerpc_matched, new_matched_key, call_value);
			value = call_value;
			if(call_value->rep_frame==0){
				call_value->rep_frame=pinfo->fd->num;
			}

		}
	}

        if (value) {
            int length, reported_length, stub_length;
            dcerpc_info *di;

            di=get_next_di();
            /* handoff this call */
	    di->conv = conv;
	    di->call_id = hdr->call_id;
	    di->smb_fid = get_smb_fid(pinfo->private_data);
	    di->request = FALSE;
	    di->call_data = value;

	    proto_tree_add_uint (dcerpc_tree, hf_dcerpc_opnum, tvb, 0, 0, value->opnum);
	    if(value->req_frame!=0){
		nstime_t ns;
		pi = proto_tree_add_uint(dcerpc_tree, hf_dcerpc_request_in,
				    tvb, 0, 0, value->req_frame);
        PROTO_ITEM_SET_GENERATED(pi);
		ns.secs= pinfo->fd->abs_secs-value->req_time.secs;
		ns.nsecs=pinfo->fd->abs_usecs*1000-value->req_time.nsecs;
		if(ns.nsecs<0){
			ns.nsecs+=1000000000;
			ns.secs--;
		}
		pi = proto_tree_add_time(dcerpc_tree, hf_dcerpc_time, tvb, offset, 0, &ns);
        PROTO_ITEM_SET_GENERATED(pi);
	    }

	    length = tvb_length_remaining(tvb, offset);
	    reported_length = tvb_reported_length_remaining(tvb, offset);
	    stub_length = hdr->frag_len - offset - auth_info.auth_size;
	    if (length > stub_length)
	      length = stub_length;
	    if (reported_length > stub_length)
	      reported_length = stub_length;

	    /* If we don't have reassembly enabled, or this packet contains
	       the entire PDU, or if we don't have all the data in this
	       fragment, just call the handoff directly if this is the
	       first fragment or the PDU isn't fragmented. */
	    if( (!dcerpc_reassemble) || PFC_NOT_FRAGMENTED(hdr) ||
			!tvb_bytes_exist(tvb, offset, stub_length) ){
		if(hdr->flags&PFC_FIRST_FRAG){
		    /* First fragment, possibly the only fragment */
		    /*
		     * XXX - should there be a third routine for each
		     * function in an RPC subdissector, to handle
		     * fault responses?  The DCE RPC 1.1 spec says
		     * three's "stub data" here, which I infer means
		     * that it's protocol-specific and call-specific.
		     *
		     * It should probably get passed the status code
		     * as well, as that might be protocol-specific.
		     */
		    if (dcerpc_tree) {
			if (stub_length > 0) {
			    proto_tree_add_text (dcerpc_tree, tvb, offset, stub_length,
						 "Fault stub data (%d byte%s)",
						 stub_length,
						 plurality(stub_length, "", "s"));
			}
		    }
		} else {
		    /* PDU is fragmented and this isn't the first fragment */
		    if (check_col(pinfo->cinfo, COL_INFO)) {
			col_append_fstr(pinfo->cinfo, COL_INFO,
			                " [DCE/RPC fragment]");
		    }
		    if (dcerpc_tree) {
			if (stub_length > 0) {
			    proto_tree_add_text (dcerpc_tree, tvb, offset, stub_length,
						 "Fragment data (%d byte%s)",
						 stub_length,
						 plurality(stub_length, "", "s"));
			}
		    }
		}
            } else {
		/* Reassembly is enabled, the PDU is fragmented, and
		   we have all the data in the fragment; the first two
		   of those mean we should attempt reassembly, and the
		   third means we can attempt reassembly. */
		if (dcerpc_tree) {
		    if (length > 0) {
			proto_tree_add_text (dcerpc_tree, tvb, offset, stub_length,
					     "Fragment data (%d byte%s)",
					     stub_length,
					     plurality(stub_length, "", "s"));
		    }
		}
	        if(hdr->flags&PFC_FIRST_FRAG){  /* FIRST fragment */
		    if( (!pinfo->fd->flags.visited) && value->rep_frame ){
			fragment_add(tvb, offset, pinfo, value->rep_frame,
			     dcerpc_co_reassemble_table,
			     0,
			     stub_length,
			     TRUE);
			fragment_set_tot_len(pinfo, value->rep_frame,
			     dcerpc_co_reassemble_table, alloc_hint);
		    }
		    if (check_col(pinfo->cinfo, COL_INFO)) {
			col_append_fstr(pinfo->cinfo, COL_INFO,
			                " [DCE/RPC fragment]");
		    }
		} else if(hdr->flags&PFC_LAST_FRAG){  /* LAST fragment */
		    if( value->rep_frame ){
			fragment_data *fd_head;
			guint32 tot_len;

			tot_len = fragment_get_tot_len(pinfo, value->rep_frame,
			               dcerpc_co_reassemble_table);
			fd_head = fragment_add(tvb, offset, pinfo,
			     value->rep_frame,
			     dcerpc_co_reassemble_table,
			     tot_len-alloc_hint,
			     stub_length,
			     TRUE);

			if(fd_head){
			    /* We completed reassembly */
			    tvbuff_t *next_tvb;

			    next_tvb = tvb_new_real_data(fd_head->data, fd_head->datalen, fd_head->datalen);
			    tvb_set_child_real_data_tvbuff(tvb, next_tvb);
			    add_new_data_source(pinfo, next_tvb, "Reassembled DCE/RPC");
			    show_fragment_tree(fd_head, &dcerpc_frag_items,
				dcerpc_tree, pinfo, next_tvb);

			    /*
			     * XXX - should there be a third routine for each
			     * function in an RPC subdissector, to handle
			     * fault responses?  The DCE RPC 1.1 spec says
			     * three's "stub data" here, which I infer means
			     * that it's protocol-specific and call-specific.
			     *
			     * It should probably get passed the status code
			     * as well, as that might be protocol-specific.
			     */
			    if (dcerpc_tree) {
				if (length > 0) {
				     proto_tree_add_text (dcerpc_tree, tvb, offset, stub_length,
							  "Fault stub data (%d byte%s)",
							  stub_length,
							  plurality(stub_length, "", "s"));
				}
			    }
			} else {
			    /* Reassembly not complete - some fragments
			       are missing */
			    if (check_col(pinfo->cinfo, COL_INFO)) {
				col_append_fstr(pinfo->cinfo, COL_INFO,
				                " [DCE/RPC fragment]");
			    }
			}
		    }
		} else {  /* MIDDLE fragment(s) */
		    if( (!pinfo->fd->flags.visited) && value->rep_frame ){
			guint32 tot_len;
			tot_len = fragment_get_tot_len(pinfo, value->rep_frame,
			               dcerpc_co_reassemble_table);
			fragment_add(tvb, offset, pinfo, value->rep_frame,
			     dcerpc_co_reassemble_table,
			     tot_len-alloc_hint,
			     stub_length,
			     TRUE);
		    }
		    if (check_col(pinfo->cinfo, COL_INFO)) {
			col_append_fstr(pinfo->cinfo, COL_INFO,
			                " [DCE/RPC fragment]");
		    }
		}
	    }
        }
    }
}

/*
 * DCERPC dissector for connection oriented calls
 */
static gboolean
dissect_dcerpc_cn (tvbuff_t *tvb, int offset, packet_info *pinfo,
                   proto_tree *tree, gboolean can_desegment, int *pkt_len)
{
    static const guint8 nulls[4] = { 0 };
    int start_offset;
    int padding = 0;
    proto_item *ti = NULL;
    proto_item *tf = NULL;
    proto_tree *dcerpc_tree = NULL;
    proto_tree *cn_flags_tree = NULL;
    proto_tree *drep_tree = NULL;
    e_dce_cn_common_hdr_t hdr;
    dcerpc_auth_info auth_info;

    /*
     * when done over nbt, dcerpc requests are padded with 4 bytes of null
     * data for some reason.
     *
     * XXX - if that's always the case, the right way to do this would
     * be to have a "dissect_dcerpc_cn_nb" routine which strips off
     * the 4 bytes of null padding, and make that the dissector
     * used for "netbios".
     */
    if (tvb_memeql (tvb, offset, nulls, 4) == 0) {

        /*
         * Skip the padding.
         */
        offset += 4;
        padding += 4;
    }

    /*
     * Check if this looks like a C/O DCERPC call
     */
    if (!tvb_bytes_exist (tvb, offset, sizeof (hdr))) {
        return FALSE;	/* not enough information to check */
    }
    start_offset = offset;
    hdr.rpc_ver = tvb_get_guint8 (tvb, offset++);
    if (hdr.rpc_ver != 5)
        return FALSE;
    hdr.rpc_ver_minor = tvb_get_guint8 (tvb, offset++);
    if (hdr.rpc_ver_minor != 0 && hdr.rpc_ver_minor != 1)
        return FALSE;
    hdr.ptype = tvb_get_guint8 (tvb, offset++);
    if (hdr.ptype > 19)
        return FALSE;

    hdr.flags = tvb_get_guint8 (tvb, offset++);
    tvb_memcpy (tvb, (guint8 *)hdr.drep, offset, sizeof (hdr.drep));
    offset += sizeof (hdr.drep);

    hdr.frag_len = dcerpc_tvb_get_ntohs (tvb, offset, hdr.drep);
    offset += 2;
    hdr.auth_len = dcerpc_tvb_get_ntohs (tvb, offset, hdr.drep);
    offset += 2;
    hdr.call_id = dcerpc_tvb_get_ntohl (tvb, offset, hdr.drep);
    offset += 4;

    if (can_desegment && pinfo->can_desegment
        && !tvb_bytes_exist(tvb, start_offset, hdr.frag_len)) {
        pinfo->desegment_offset = start_offset;
        pinfo->desegment_len = hdr.frag_len - tvb_length_remaining (tvb, start_offset);
        *pkt_len = 0;	/* desegmentation required */
        return TRUE;
    }

    if (check_col (pinfo->cinfo, COL_PROTOCOL))
        col_set_str (pinfo->cinfo, COL_PROTOCOL, "DCERPC");
    if (check_col (pinfo->cinfo, COL_INFO))
        col_add_fstr (pinfo->cinfo, COL_INFO, "%s: call_id: %u",
	    pckt_vals[hdr.ptype].strptr, hdr.call_id);

    if (tree) {
        offset = start_offset;
        ti = proto_tree_add_item (tree, proto_dcerpc, tvb, offset, hdr.frag_len, FALSE);
        if (ti) {
            dcerpc_tree = proto_item_add_subtree (ti, ett_dcerpc);
        }
        proto_tree_add_uint (dcerpc_tree, hf_dcerpc_ver, tvb, offset++, 1, hdr.rpc_ver);
        proto_tree_add_uint (dcerpc_tree, hf_dcerpc_ver_minor, tvb, offset++, 1, hdr.rpc_ver_minor);
        proto_tree_add_uint (dcerpc_tree, hf_dcerpc_packet_type, tvb, offset++, 1, hdr.ptype);
        tf = proto_tree_add_uint (dcerpc_tree, hf_dcerpc_cn_flags, tvb, offset, 1, hdr.flags);
        cn_flags_tree = proto_item_add_subtree (tf, ett_dcerpc_cn_flags);
        if (cn_flags_tree) {
            proto_tree_add_boolean (cn_flags_tree, hf_dcerpc_cn_flags_object, tvb, offset, 1, hdr.flags);
            proto_tree_add_boolean (cn_flags_tree, hf_dcerpc_cn_flags_maybe, tvb, offset, 1, hdr.flags);
            proto_tree_add_boolean (cn_flags_tree, hf_dcerpc_cn_flags_dne, tvb, offset, 1, hdr.flags);
            proto_tree_add_boolean (cn_flags_tree, hf_dcerpc_cn_flags_mpx, tvb, offset, 1, hdr.flags);
            proto_tree_add_boolean (cn_flags_tree, hf_dcerpc_cn_flags_reserved, tvb, offset, 1, hdr.flags);
            proto_tree_add_boolean (cn_flags_tree, hf_dcerpc_cn_flags_cancel_pending, tvb, offset, 1, hdr.flags);
            proto_tree_add_boolean (cn_flags_tree, hf_dcerpc_cn_flags_last_frag, tvb, offset, 1, hdr.flags);
            proto_tree_add_boolean (cn_flags_tree, hf_dcerpc_cn_flags_first_frag, tvb, offset, 1, hdr.flags);
        }
        offset++;

        tf = proto_tree_add_bytes (dcerpc_tree, hf_dcerpc_drep, tvb, offset, 4, hdr.drep);
        drep_tree = proto_item_add_subtree (tf, ett_dcerpc_drep);
        if (drep_tree) {
            proto_tree_add_uint(drep_tree, hf_dcerpc_drep_byteorder, tvb, offset, 1, hdr.drep[0] >> 4);
            proto_tree_add_uint(drep_tree, hf_dcerpc_drep_character, tvb, offset, 1, hdr.drep[0] & 0x0f);
            proto_tree_add_uint(drep_tree, hf_dcerpc_drep_fp, tvb, offset+1, 1, hdr.drep[1]);
        }
        offset += sizeof (hdr.drep);

        proto_tree_add_uint (dcerpc_tree, hf_dcerpc_cn_frag_len, tvb, offset, 2, hdr.frag_len);
        offset += 2;

        proto_tree_add_uint (dcerpc_tree, hf_dcerpc_cn_auth_len, tvb, offset, 2, hdr.auth_len);
        offset += 2;

        proto_tree_add_uint (dcerpc_tree, hf_dcerpc_cn_call_id, tvb, offset, 4, hdr.call_id);
        offset += 4;
    }

    /*
     * None of the stuff done above should throw an exception, because
     * we would have rejected this as "not DCE RPC" if we didn't have all
     * of it.  (XXX - perhaps we should request reassembly if we have
     * enough of the header to consider it DCE RPC but not enough to
     * get the fragment length; in that case the stuff still wouldn't
     * throw an exception.)
     *
     * The rest of the stuff might, so return the PDU length to our caller.
     * XXX - should we construct a tvbuff containing only the PDU and
     * use that?  Or should we have separate "is this a DCE RPC PDU",
     * "how long is it", and "dissect it" routines - which might let us
     * do most of the work in "tcp_dissect_pdus()"?
     */
    if (pkt_len != NULL)
        *pkt_len = hdr.frag_len + padding;

    /*
     * Packet type specific stuff is next.
     */
    switch (hdr.ptype) {
    case PDU_BIND:
    case PDU_ALTER:
        dissect_dcerpc_cn_bind (tvb, offset, pinfo, dcerpc_tree, &hdr);
        break;

    case PDU_BIND_ACK:
    case PDU_ALTER_ACK:
        dissect_dcerpc_cn_bind_ack (tvb, offset, pinfo, dcerpc_tree, &hdr);
        break;

    case PDU_AUTH3:
        /*
         * Nothing after the common header other than credentials.
         */
        dissect_dcerpc_cn_auth (tvb, offset, pinfo, dcerpc_tree, &hdr, TRUE, 
				&auth_info);
        break;

    case PDU_REQ:
        dissect_dcerpc_cn_rqst (tvb, offset, pinfo, dcerpc_tree, tree, &hdr);
        break;

    case PDU_RESP:
        dissect_dcerpc_cn_resp (tvb, offset, pinfo, dcerpc_tree, tree, &hdr);
        break;

    case PDU_FAULT:
        dissect_dcerpc_cn_fault (tvb, offset, pinfo, dcerpc_tree, &hdr);
        break;

    case PDU_BIND_NAK:
        dissect_dcerpc_cn_bind_nak (tvb, offset, pinfo, dcerpc_tree, &hdr);
        break;

    case PDU_CO_CANCEL:
    case PDU_ORPHANED:
        /*
         * Nothing after the common header other than an authentication
         * verifier.
         */
        dissect_dcerpc_cn_auth (tvb, offset, pinfo, dcerpc_tree, &hdr, FALSE, 
				&auth_info);
        break;

    case PDU_SHUTDOWN:
        /*
         * Nothing after the common header, not even an authentication
         * verifier.
         */
        break;

    default:
        /* might as well dissect the auth info */
        dissect_dcerpc_cn_auth (tvb, offset, pinfo, dcerpc_tree, &hdr, FALSE, 
				&auth_info);
        break;
    }
    return TRUE;
}

/*
 * DCERPC dissector for connection oriented calls over packet-oriented
 * transports
 */
static gboolean
dissect_dcerpc_cn_pk (tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
    /*
     * Only one PDU per transport packet, and only one transport
     * packet per PDU.
     */
    if (!dissect_dcerpc_cn (tvb, 0, pinfo, tree, FALSE, NULL)) {
        /*
         * It wasn't a DCERPC PDU.
         */
        return FALSE;
    } else {
        /*
         * It was.
         */
        return TRUE;
    }
}

/*
 * DCERPC dissector for connection oriented calls over byte-stream
 * transports
 */
static gboolean
dissect_dcerpc_cn_bs (tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
    volatile int offset = 0;
    int pdu_len;
    volatile gboolean is_dcerpc_pdu;
    volatile gboolean ret = FALSE;

    /*
     * There may be multiple PDUs per transport packet; keep
     * processing them.
     */
    while (tvb_reported_length_remaining(tvb, offset) != 0) {
        /*
         * Catch ReportedBoundsError, so that even if the stub data is bad,
         * we don't abort the full DCE RPC dissection - there might be more
         * than one DCE RPC PDU in the data being dissected.
         *
         * If we get BoundsError, it means the frame was cut short by a
         * snapshot length, so there's nothing more to dissect; just
         * re-throw that exception.
         */
        is_dcerpc_pdu = FALSE;
        TRY {
            is_dcerpc_pdu = dissect_dcerpc_cn (tvb, offset, pinfo, tree,
                                               dcerpc_cn_desegment, &pdu_len);
        } CATCH(BoundsError) {
            RETHROW;
        } CATCH(ReportedBoundsError) {
            show_reported_bounds_error(tvb, pinfo, tree);
        } ENDTRY;

        if (!is_dcerpc_pdu) {
            /*
             * Not a DCERPC PDU.
             */
            break;
        }

        /*
         * Well, we've seen at least one DCERPC PDU.
         */
        ret = TRUE;

        if (pdu_len == 0) {
            /*
             * Desegmentation required - bail now.
             */
            break;
	}

        /*
         * Step to the next PDU.
         */
        offset += pdu_len;
    }
    return ret;
}

static void
dissect_dcerpc_dg_auth (tvbuff_t *tvb, int offset, proto_tree *dcerpc_tree,
                        e_dce_dg_common_hdr_t *hdr, int *auth_level_p)
{
    proto_item *ti = NULL;
    proto_tree *auth_tree = NULL;
    guint8 protection_level;

    /*
     * Initially set "*auth_level_p" to -1 to indicate that we haven't
     * yet seen any authentication level information.
     */
    if (auth_level_p != NULL)
        *auth_level_p = -1;

    /*
     * The authentication information is at the *end* of the PDU; in
     * request and response PDUs, the request and response stub data
     * come before it.
     *
     * If the full packet is here, and there's data past the end of the
     * packet body, then dissect the auth info.
     */
    offset += hdr->frag_len;
    if (tvb_length_remaining(tvb, offset) > 0) {
    	switch (hdr->auth_proto) {

        case DCE_C_RPC_AUTHN_PROTOCOL_KRB5:
            ti = proto_tree_add_text (dcerpc_tree, tvb, offset, -1, "Kerberos authentication verifier");
            auth_tree = proto_item_add_subtree (ti, ett_dcerpc_krb5_auth_verf);
            protection_level = tvb_get_guint8 (tvb, offset);
            if (auth_level_p != NULL)
                *auth_level_p = protection_level;
            proto_tree_add_uint (auth_tree, hf_dcerpc_krb5_av_prot_level, tvb, offset, 1, protection_level);
            offset++;
            proto_tree_add_item (auth_tree, hf_dcerpc_krb5_av_key_vers_num, tvb, offset, 1, FALSE);
            offset++;
            if (protection_level == DCE_C_AUTHN_LEVEL_PKT_PRIVACY)
                offset += 6;    /* 6 bytes of padding */
            else
                offset += 2;    /* 6 bytes of padding */
            proto_tree_add_item (auth_tree, hf_dcerpc_krb5_av_key_auth_verifier, tvb, offset, 16, FALSE);
            offset += 16;
            break;

    	default:
            proto_tree_add_text (dcerpc_tree, tvb, offset, -1, "Authentication verifier");
            break;
        }
    }
}

static void
dissect_dcerpc_dg_cancel_ack (tvbuff_t *tvb, int offset, packet_info *pinfo,
                              proto_tree *dcerpc_tree,
                              e_dce_dg_common_hdr_t *hdr)
{
    guint32 version;

    offset = dissect_dcerpc_uint32 (tvb, offset, pinfo, dcerpc_tree,
                                    hdr->drep, hf_dcerpc_dg_cancel_vers,
                                    &version);

    switch (version) {

    case 0:
        /* The only version we know about */
        offset = dissect_dcerpc_uint32 (tvb, offset, pinfo, dcerpc_tree,
                                        hdr->drep, hf_dcerpc_dg_cancel_id,
                                        NULL);
        offset = dissect_dcerpc_uint8 (tvb, offset, pinfo, dcerpc_tree,
                                       hdr->drep, hf_dcerpc_dg_server_accepting_cancels,
                                       NULL);
        break;
    }
}

static void
dissect_dcerpc_dg_cancel (tvbuff_t *tvb, int offset, packet_info *pinfo,
                          proto_tree *dcerpc_tree,
                          e_dce_dg_common_hdr_t *hdr)
{
    guint32 version;

    offset = dissect_dcerpc_uint32 (tvb, offset, pinfo, dcerpc_tree,
                                    hdr->drep, hf_dcerpc_dg_cancel_vers,
                                    &version);

    switch (version) {

    case 0:
        /* The only version we know about */
        offset = dissect_dcerpc_uint32 (tvb, offset, pinfo, dcerpc_tree,
                                        hdr->drep, hf_dcerpc_dg_cancel_id,
                                        NULL);
        /* XXX - are NDR booleans 32 bits? */
        offset = dissect_dcerpc_uint32 (tvb, offset, pinfo, dcerpc_tree,
                                        hdr->drep, hf_dcerpc_dg_server_accepting_cancels,
                                        NULL);
        break;
    }
}

static void
dissect_dcerpc_dg_fack (tvbuff_t *tvb, int offset, packet_info *pinfo,
                        proto_tree *dcerpc_tree,
                        e_dce_dg_common_hdr_t *hdr)
{
    guint8 version;
    guint16 serial_num;
    guint16 selack_len;
    guint i;

    offset = dissect_dcerpc_uint8 (tvb, offset, pinfo, dcerpc_tree,
                                  hdr->drep, hf_dcerpc_dg_fack_vers,
                                  &version);
    /* padding */
    offset++;

    switch (version) {

    case 0:	/* The only version documented in the DCE RPC 1.1 spec */
    case 1:	/* This appears to be the same */
        offset = dissect_dcerpc_uint16 (tvb, offset, pinfo, dcerpc_tree,
                                        hdr->drep, hf_dcerpc_dg_fack_window_size,
                                        NULL);
        offset = dissect_dcerpc_uint32 (tvb, offset, pinfo, dcerpc_tree,
                                        hdr->drep, hf_dcerpc_dg_fack_max_tsdu,
                                        NULL);
        offset = dissect_dcerpc_uint32 (tvb, offset, pinfo, dcerpc_tree,
                                        hdr->drep, hf_dcerpc_dg_fack_max_frag_size,
                                        NULL);
        offset = dissect_dcerpc_uint16 (tvb, offset, pinfo, dcerpc_tree,
                                        hdr->drep, hf_dcerpc_dg_fack_serial_num,
                                        &serial_num);
        if (check_col (pinfo->cinfo, COL_INFO)) {
            col_append_fstr (pinfo->cinfo, COL_INFO, " serial: %u",
                             serial_num);
        }
        offset = dissect_dcerpc_uint16 (tvb, offset, pinfo, dcerpc_tree,
                                        hdr->drep, hf_dcerpc_dg_fack_selack_len,
                                        &selack_len);
        for (i = 0; i < selack_len; i++) {
            offset = dissect_dcerpc_uint32 (tvb, offset, pinfo, dcerpc_tree,
                                            hdr->drep, hf_dcerpc_dg_fack_selack,
                                            NULL);
        }

        break;
    }
}

static void
dissect_dcerpc_dg_reject_fault (tvbuff_t *tvb, int offset, packet_info *pinfo,
                        proto_tree *dcerpc_tree,
                        e_dce_dg_common_hdr_t *hdr)
{
    guint32 status;

    offset = dissect_dcerpc_uint32 (tvb, offset, pinfo, dcerpc_tree,
                                    hdr->drep, hf_dcerpc_dg_status,
                                    &status);

    if (check_col (pinfo->cinfo, COL_INFO)) {
        col_append_fstr (pinfo->cinfo, COL_INFO,
                      ": status: %s",
                      val_to_str(status, reject_status_vals, "Unknown (0x%08x)"));
    }
}

static void
dissect_dcerpc_dg_stub (tvbuff_t *tvb, int offset, packet_info *pinfo,
                        proto_tree *dcerpc_tree, proto_tree *tree,
                        e_dce_dg_common_hdr_t *hdr, dcerpc_info *di)
{
    int length, reported_length, stub_length;
    gboolean save_fragmented;
    fragment_data *fd_head;
    tvbuff_t *next_tvb;
    proto_item *pi;

    if (check_col (pinfo->cinfo, COL_INFO))
        col_append_fstr (pinfo->cinfo, COL_INFO, " opnum: %u len: %u", 
            di->call_data->opnum, hdr->frag_len );

    length = tvb_length_remaining (tvb, offset);
    reported_length = tvb_reported_length_remaining (tvb, offset);
    stub_length = hdr->frag_len;
    if (length > stub_length)
        length = stub_length;
    if (reported_length > stub_length)
        reported_length = stub_length;

    save_fragmented = pinfo->fragmented;

    /* If we don't have reassembly enabled, or this packet contains
       the entire PDU, or if this is a short frame (or a frame
       not reassembled at a lower layer) that doesn't include all
       the data in the fragment, just call the handoff directly if
       this is the first fragment or the PDU isn't fragmented. */
    if( (!dcerpc_reassemble) || !(hdr->flags1 & PFCL1_FRAG) ||
		!tvb_bytes_exist(tvb, offset, stub_length) ){
	if(hdr->frag_num == 0) {


	    /* First fragment, possibly the only fragment */

	    /*
	     * XXX - authentication info?
	     */
	    pinfo->fragmented = (hdr->flags1 & PFCL1_FRAG);
	    next_tvb = tvb_new_subset (tvb, offset, length,
				       reported_length);
	    dcerpc_try_handoff (pinfo, tree, dcerpc_tree, next_tvb,
				next_tvb, hdr->drep, di, NULL);
	} else {
	    /* PDU is fragmented and this isn't the first fragment */
	    if (check_col(pinfo->cinfo, COL_INFO)) {
		col_append_fstr(pinfo->cinfo, COL_INFO, " [DCE/RPC fragment]");
	    }
	    if (dcerpc_tree) {
		if (length > 0) {
		    proto_tree_add_text (dcerpc_tree, tvb, offset, stub_length,
					 "Fragment data (%d byte%s)",
					 stub_length,
					 plurality(stub_length, "", "s"));
		}
	    }
        }
    } else {
	/* Reassembly is enabled, the PDU is fragmented, and
	   we have all the data in the fragment; the first two
	   of those mean we should attempt reassembly, and the
	   third means we can attempt reassembly. */
	if (dcerpc_tree) {
	    if (length > 0) {
		proto_tree_add_text (dcerpc_tree, tvb, offset, stub_length,
				     "Fragment data (%d byte%s)", stub_length,
				     plurality(stub_length, "", "s"));
	    }
	}

	fd_head = fragment_add_seq(tvb, offset, pinfo,
			hdr->seqnum, dcerpc_cl_reassemble_table,
			hdr->frag_num, stub_length,
			!(hdr->flags1 & PFCL1_LASTFRAG));
	if (fd_head != NULL) {
	    /* We completed reassembly... */
        if(pinfo->fd->num==fd_head->reassembled_in) {
            /* ...and this is the reassembled RPC PDU */
	    next_tvb = tvb_new_real_data(fd_head->data, fd_head->len, fd_head->len);
	    tvb_set_child_real_data_tvbuff(tvb, next_tvb);
	    add_new_data_source(pinfo, next_tvb, "Reassembled DCE/RPC");
	    show_fragment_seq_tree(fd_head, &dcerpc_frag_items,
				   dcerpc_tree, pinfo, next_tvb);

	    /*
	     * XXX - authentication info?
	     */
	    pinfo->fragmented = FALSE;
	    dcerpc_try_handoff (pinfo, tree, dcerpc_tree, next_tvb,
				next_tvb, hdr->drep, di, NULL);
	} else {
            /* ...and this isn't the reassembled RPC PDU */
	        pi = proto_tree_add_uint(dcerpc_tree, hf_dcerpc_reassembled_in,
				    tvb, 0, 0, fd_head->reassembled_in);
            PROTO_ITEM_SET_GENERATED(pi);
	        if (check_col(pinfo->cinfo, COL_INFO)) {
		    col_append_fstr(pinfo->cinfo, COL_INFO,
			    " [DCE/RPC fragment, reas: #%u]", fd_head->reassembled_in);
	        }
        }
	} else {
	    /* Reassembly isn't completed yet */
	    if (check_col(pinfo->cinfo, COL_INFO)) {
		col_append_fstr(pinfo->cinfo, COL_INFO, " [DCE/RPC fragment]");
	    }
	}
    }
    pinfo->fragmented = save_fragmented;
}

static void
dissect_dcerpc_dg_rqst (tvbuff_t *tvb, int offset, packet_info *pinfo,
                        proto_tree *dcerpc_tree, proto_tree *tree,
                        e_dce_dg_common_hdr_t *hdr, conversation_t *conv)
{
    dcerpc_info *di;
    dcerpc_call_value *value, v;
    dcerpc_matched_key matched_key, *new_matched_key;
    proto_item *pi;

    di=get_next_di();
    if(!(pinfo->fd->flags.visited)){
	dcerpc_call_value *call_value;
	dcerpc_call_key *call_key;

	call_key=g_mem_chunk_alloc (dcerpc_call_key_chunk);
	call_key->conv=conv;
	call_key->call_id=hdr->seqnum;
	call_key->smb_fid=get_smb_fid(pinfo->private_data);

	call_value=g_mem_chunk_alloc (dcerpc_call_value_chunk);
	call_value->uuid = hdr->if_id;
	call_value->ver = hdr->if_ver;
	call_value->opnum = hdr->opnum;
	call_value->req_frame=pinfo->fd->num;
	call_value->req_time.secs=pinfo->fd->abs_secs;
	call_value->req_time.nsecs=pinfo->fd->abs_usecs*1000;
	call_value->rep_frame=0;
	call_value->max_ptr=0;
	call_value->private_data = NULL;
	g_hash_table_insert (dcerpc_calls, call_key, call_value);

	new_matched_key = g_mem_chunk_alloc(dcerpc_matched_key_chunk);
	new_matched_key->frame = pinfo->fd->num;
	new_matched_key->call_id = hdr->seqnum;
	g_hash_table_insert (dcerpc_matched, new_matched_key, call_value);
    }

    matched_key.frame = pinfo->fd->num;
    matched_key.call_id = hdr->seqnum;
    value=g_hash_table_lookup(dcerpc_matched, &matched_key);
    if (!value) {
        v.uuid = hdr->if_id;
        v.ver = hdr->if_ver;
        v.opnum = hdr->opnum;
        v.req_frame = pinfo->fd->num;
        v.rep_frame = 0;
        v.max_ptr = 0;
        v.private_data=NULL;
        value = &v;
    }

    di->conv = conv;
    di->call_id = hdr->seqnum;
    di->smb_fid = -1;
    di->request = TRUE;
    di->call_data = value;

    if(value->rep_frame!=0){
	pi = proto_tree_add_uint(dcerpc_tree, hf_dcerpc_response_in,
			    tvb, 0, 0, value->rep_frame);
    PROTO_ITEM_SET_GENERATED(pi);
    }
    dissect_dcerpc_dg_stub (tvb, offset, pinfo, dcerpc_tree, tree, hdr, di);
}

static void
dissect_dcerpc_dg_resp (tvbuff_t *tvb, int offset, packet_info *pinfo,
                        proto_tree *dcerpc_tree, proto_tree *tree,
                        e_dce_dg_common_hdr_t *hdr, conversation_t *conv)
{
    dcerpc_info *di;
    dcerpc_call_value *value, v;
    dcerpc_matched_key matched_key, *new_matched_key;
    proto_item *pi;

    di=get_next_di();
    if(!(pinfo->fd->flags.visited)){
	dcerpc_call_value *call_value;
	dcerpc_call_key call_key;

	call_key.conv=conv;
	call_key.call_id=hdr->seqnum;
	call_key.smb_fid=get_smb_fid(pinfo->private_data);

	if((call_value=g_hash_table_lookup(dcerpc_calls, &call_key))){
	    new_matched_key = g_mem_chunk_alloc(dcerpc_matched_key_chunk);
	    new_matched_key->frame = pinfo->fd->num;
	    new_matched_key->call_id = hdr->seqnum;
	    g_hash_table_insert (dcerpc_matched, new_matched_key, call_value);
	    if(call_value->rep_frame==0){
		call_value->rep_frame=pinfo->fd->num;
	    }
	}
    }

    matched_key.frame = pinfo->fd->num;
    matched_key.call_id = hdr->seqnum;
    value=g_hash_table_lookup(dcerpc_matched, &matched_key);
    if (!value) {
        v.uuid = hdr->if_id;
        v.ver = hdr->if_ver;
        v.opnum = hdr->opnum;
        v.req_frame=0;
        v.rep_frame=pinfo->fd->num;
        v.private_data=NULL;
        value = &v;
    }

    di->conv = conv;
    di->call_id = 0;
    di->smb_fid = -1;
    di->request = FALSE;
    di->call_data = value;

    if(value->req_frame!=0){
	nstime_t ns;
	pi = proto_tree_add_uint(dcerpc_tree, hf_dcerpc_request_in,
			    tvb, 0, 0, value->req_frame);
    PROTO_ITEM_SET_GENERATED(pi);
	ns.secs= pinfo->fd->abs_secs-value->req_time.secs;
	ns.nsecs=pinfo->fd->abs_usecs*1000-value->req_time.nsecs;
	if(ns.nsecs<0){
		ns.nsecs+=1000000000;
		ns.secs--;
	}
	pi = proto_tree_add_time(dcerpc_tree, hf_dcerpc_time, tvb, offset, 0, &ns);
    PROTO_ITEM_SET_GENERATED(pi);
    }
    dissect_dcerpc_dg_stub (tvb, offset, pinfo, dcerpc_tree, tree, hdr, di);
}

/*
 * DCERPC dissector for connectionless calls
 */
static gboolean
dissect_dcerpc_dg (tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
    proto_item *ti = NULL;
    proto_item *tf = NULL;
    proto_tree *dcerpc_tree = NULL;
    proto_tree *dg_flags1_tree = NULL;
    proto_tree *dg_flags2_tree = NULL;
    proto_tree *drep_tree = NULL;
    e_dce_dg_common_hdr_t hdr;
    int offset = 0;
    conversation_t *conv;
    int auth_level;
    char uuid_str[DCERPC_UUID_STR_LEN]; 
    int uuid_str_len;

    /*
     * Check if this looks like a CL DCERPC call.  All dg packets
     * have an 80 byte header on them.  Which starts with
     * version (4), pkt_type.
     */
    if (!tvb_bytes_exist (tvb, 0, sizeof (hdr))) {
        return FALSE;
    }
    hdr.rpc_ver = tvb_get_guint8 (tvb, offset++);
    if (hdr.rpc_ver != 4)
        return FALSE;
    hdr.ptype = tvb_get_guint8 (tvb, offset++);
    if (hdr.ptype > 19)
        return FALSE;

    if (check_col (pinfo->cinfo, COL_PROTOCOL))
        col_set_str (pinfo->cinfo, COL_PROTOCOL, "DCERPC");
    if (check_col (pinfo->cinfo, COL_INFO))
        col_add_str (pinfo->cinfo, COL_INFO, pckt_vals[hdr.ptype].strptr);

    hdr.flags1 = tvb_get_guint8 (tvb, offset++);
    hdr.flags2 = tvb_get_guint8 (tvb, offset++);
    tvb_memcpy (tvb, (guint8 *)hdr.drep, offset, sizeof (hdr.drep));
    offset += sizeof (hdr.drep);
    hdr.serial_hi = tvb_get_guint8 (tvb, offset++);
    dcerpc_tvb_get_uuid (tvb, offset, hdr.drep, &hdr.obj_id);
    offset += 16;
    dcerpc_tvb_get_uuid (tvb, offset, hdr.drep, &hdr.if_id);
    offset += 16;
    dcerpc_tvb_get_uuid (tvb, offset, hdr.drep, &hdr.act_id);
    offset += 16;
    hdr.server_boot = dcerpc_tvb_get_ntohl (tvb, offset, hdr.drep);
    offset += 4;
    hdr.if_ver = dcerpc_tvb_get_ntohl (tvb, offset, hdr.drep);
    offset += 4;
    hdr.seqnum = dcerpc_tvb_get_ntohl (tvb, offset, hdr.drep);
    offset += 4;
    hdr.opnum = dcerpc_tvb_get_ntohs (tvb, offset, hdr.drep);
    offset += 2;
    hdr.ihint = dcerpc_tvb_get_ntohs (tvb, offset, hdr.drep);
    offset += 2;
    hdr.ahint = dcerpc_tvb_get_ntohs (tvb, offset, hdr.drep);
    offset += 2;
    hdr.frag_len = dcerpc_tvb_get_ntohs (tvb, offset, hdr.drep);
    offset += 2;
    hdr.frag_num = dcerpc_tvb_get_ntohs (tvb, offset, hdr.drep);
    offset += 2;
    hdr.auth_proto = tvb_get_guint8 (tvb, offset++);
    hdr.serial_lo = tvb_get_guint8 (tvb, offset++);

    if (tree) {
        ti = proto_tree_add_item (tree, proto_dcerpc, tvb, 0, -1, FALSE);
        if (ti) {
            dcerpc_tree = proto_item_add_subtree(ti, ett_dcerpc);
        }
    }
    offset = 0;

    if (tree)
        proto_tree_add_uint (dcerpc_tree, hf_dcerpc_ver, tvb, offset, 1, hdr.rpc_ver);
    offset++;

    if (tree)
        proto_tree_add_uint (dcerpc_tree, hf_dcerpc_packet_type, tvb, offset, 1, hdr.ptype);
    offset++;

    if (tree) {
        tf = proto_tree_add_uint (dcerpc_tree, hf_dcerpc_dg_flags1, tvb, offset, 1, hdr.flags1);
        dg_flags1_tree = proto_item_add_subtree (tf, ett_dcerpc_dg_flags1);
        if (dg_flags1_tree) {
            proto_tree_add_boolean (dg_flags1_tree, hf_dcerpc_dg_flags1_rsrvd_80, tvb, offset, 1, hdr.flags1);
            proto_tree_add_boolean (dg_flags1_tree, hf_dcerpc_dg_flags1_broadcast, tvb, offset, 1, hdr.flags1);
            proto_tree_add_boolean (dg_flags1_tree, hf_dcerpc_dg_flags1_idempotent, tvb, offset, 1, hdr.flags1);
            proto_tree_add_boolean (dg_flags1_tree, hf_dcerpc_dg_flags1_maybe, tvb, offset, 1, hdr.flags1);
            proto_tree_add_boolean (dg_flags1_tree, hf_dcerpc_dg_flags1_nofack, tvb, offset, 1, hdr.flags1);
            proto_tree_add_boolean (dg_flags1_tree, hf_dcerpc_dg_flags1_frag, tvb, offset, 1, hdr.flags1);
            proto_tree_add_boolean (dg_flags1_tree, hf_dcerpc_dg_flags1_last_frag, tvb, offset, 1, hdr.flags1);
            proto_tree_add_boolean (dg_flags1_tree, hf_dcerpc_dg_flags1_rsrvd_01, tvb, offset, 1, hdr.flags1);
        }
    }
    offset++;

    if (tree) {
        tf = proto_tree_add_uint (dcerpc_tree, hf_dcerpc_dg_flags2, tvb, offset, 1, hdr.flags2);
        dg_flags2_tree = proto_item_add_subtree (tf, ett_dcerpc_dg_flags2);
        if (dg_flags2_tree) {
            proto_tree_add_boolean (dg_flags2_tree, hf_dcerpc_dg_flags2_rsrvd_80, tvb, offset, 1, hdr.flags2);
            proto_tree_add_boolean (dg_flags2_tree, hf_dcerpc_dg_flags2_rsrvd_40, tvb, offset, 1, hdr.flags2);
            proto_tree_add_boolean (dg_flags2_tree, hf_dcerpc_dg_flags2_rsrvd_20, tvb, offset, 1, hdr.flags2);
            proto_tree_add_boolean (dg_flags2_tree, hf_dcerpc_dg_flags2_rsrvd_10, tvb, offset, 1, hdr.flags2);
            proto_tree_add_boolean (dg_flags2_tree, hf_dcerpc_dg_flags2_rsrvd_08, tvb, offset, 1, hdr.flags2);
            proto_tree_add_boolean (dg_flags2_tree, hf_dcerpc_dg_flags2_rsrvd_04, tvb, offset, 1, hdr.flags2);
            proto_tree_add_boolean (dg_flags2_tree, hf_dcerpc_dg_flags2_cancel_pending, tvb, offset, 1, hdr.flags2);
            proto_tree_add_boolean (dg_flags2_tree, hf_dcerpc_dg_flags2_rsrvd_01, tvb, offset, 1, hdr.flags2);
        }
    }
    offset++;

    if (tree) {
        tf = proto_tree_add_bytes (dcerpc_tree, hf_dcerpc_drep, tvb, offset, sizeof (hdr.drep), hdr.drep);
        drep_tree = proto_item_add_subtree (tf, ett_dcerpc_drep);
        if (drep_tree) {
            proto_tree_add_uint(drep_tree, hf_dcerpc_drep_byteorder, tvb, offset, 1, hdr.drep[0] >> 4);
            proto_tree_add_uint(drep_tree, hf_dcerpc_drep_character, tvb, offset, 1, hdr.drep[0] & 0x0f);
            proto_tree_add_uint(drep_tree, hf_dcerpc_drep_fp, tvb, offset+1, 1, hdr.drep[1]);
        }
    }
    offset += sizeof (hdr.drep);

    if (tree)
        proto_tree_add_uint (dcerpc_tree, hf_dcerpc_dg_serial_hi, tvb, offset, 1, hdr.serial_hi);
    offset++;

    if (tree) {
        /* XXX - use "dissect_ndr_uuid_t()"? */
	uuid_str_len = snprintf(uuid_str, DCERPC_UUID_STR_LEN, 
                                "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                                hdr.obj_id.Data1, hdr.obj_id.Data2, hdr.obj_id.Data3,
                                hdr.obj_id.Data4[0],
                                hdr.obj_id.Data4[1],
                                hdr.obj_id.Data4[2],
                                hdr.obj_id.Data4[3],
                                hdr.obj_id.Data4[4],
                                hdr.obj_id.Data4[5],
                                hdr.obj_id.Data4[6],
                                hdr.obj_id.Data4[7]);
        if (uuid_str_len >= DCERPC_UUID_STR_LEN)
		memset(uuid_str, 0, DCERPC_UUID_STR_LEN);
        proto_tree_add_string_format (dcerpc_tree, hf_dcerpc_obj_id, tvb,
                                      offset, 16, uuid_str, "Object UUID: %s", uuid_str);
    }
    offset += 16;

    if (tree) {
        /* XXX - use "dissect_ndr_uuid_t()"? */
	uuid_str_len = snprintf(uuid_str, DCERPC_UUID_STR_LEN, 
                                "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                                hdr.if_id.Data1, hdr.if_id.Data2, hdr.if_id.Data3,
                                hdr.if_id.Data4[0],
                                hdr.if_id.Data4[1],
                                hdr.if_id.Data4[2],
                                hdr.if_id.Data4[3],
                                hdr.if_id.Data4[4],
                                hdr.if_id.Data4[5],
                                hdr.if_id.Data4[6],
                                hdr.if_id.Data4[7]);
        if (uuid_str_len >= DCERPC_UUID_STR_LEN)
		memset(uuid_str, 0, DCERPC_UUID_STR_LEN);
        proto_tree_add_string_format (dcerpc_tree, hf_dcerpc_dg_if_id, tvb,
                                      offset, 16, uuid_str, "Interface: %s", uuid_str);
    }
    offset += 16;

    if (tree) {
        /* XXX - use "dissect_ndr_uuid_t()"? */
	uuid_str_len = snprintf(uuid_str, DCERPC_UUID_STR_LEN, 
                                "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                                hdr.act_id.Data1, hdr.act_id.Data2, hdr.act_id.Data3,
                                hdr.act_id.Data4[0],
                                hdr.act_id.Data4[1],
                                hdr.act_id.Data4[2],
                                hdr.act_id.Data4[3],
                                hdr.act_id.Data4[4],
                                hdr.act_id.Data4[5],
                                hdr.act_id.Data4[6],
                                hdr.act_id.Data4[7]);
        if (uuid_str_len >= DCERPC_UUID_STR_LEN)
		memset(uuid_str, 0, DCERPC_UUID_STR_LEN);
        proto_tree_add_string_format (dcerpc_tree, hf_dcerpc_dg_act_id, tvb,
                                      offset, 16, uuid_str, "Activity: %s", uuid_str);
    }
    offset += 16;

    if (tree) {
        nstime_t server_boot;

        server_boot.secs  = hdr.server_boot;
        server_boot.nsecs = 0;

        if (hdr.server_boot == 0)
            proto_tree_add_time_format (dcerpc_tree, hf_dcerpc_dg_server_boot,
                                        tvb, offset, 4, &server_boot,
                                        "Server boot time: Unknown (0)");
        else
            proto_tree_add_time (dcerpc_tree, hf_dcerpc_dg_server_boot,
                                 tvb, offset, 4, &server_boot);
    }
    offset += 4;

    if (tree)
        proto_tree_add_uint (dcerpc_tree, hf_dcerpc_dg_if_ver, tvb, offset, 4, hdr.if_ver);
    offset += 4;

    if (tree)
        proto_tree_add_uint (dcerpc_tree, hf_dcerpc_dg_seqnum, tvb, offset, 4, hdr.seqnum);
    if (check_col (pinfo->cinfo, COL_INFO)) {
        col_append_fstr (pinfo->cinfo, COL_INFO, ": seq: %u", hdr.seqnum);
    }
    offset += 4;

    if (tree)
        proto_tree_add_uint (dcerpc_tree, hf_dcerpc_opnum, tvb, offset, 2, hdr.opnum);
    offset += 2;

    if (tree)
        proto_tree_add_uint (dcerpc_tree, hf_dcerpc_dg_ihint, tvb, offset, 2, hdr.ihint);
    offset += 2;

    if (tree)
        proto_tree_add_uint (dcerpc_tree, hf_dcerpc_dg_ahint, tvb, offset, 2, hdr.ahint);
    offset += 2;

    if (tree)
        proto_tree_add_uint (dcerpc_tree, hf_dcerpc_dg_frag_len, tvb, offset, 2, hdr.frag_len);
    offset += 2;

    if (tree)
        proto_tree_add_uint (dcerpc_tree, hf_dcerpc_dg_frag_num, tvb, offset, 2, hdr.frag_num);
    if (check_col (pinfo->cinfo, COL_INFO)) {
        if (hdr.flags1 & PFCL1_FRAG) {
            /* Fragmented - put the fragment number into the Info column */
            col_append_fstr (pinfo->cinfo, COL_INFO, " frag: %u",
                             hdr.frag_num);
        }
    }
    offset += 2;

    if (tree)
        proto_tree_add_uint (dcerpc_tree, hf_dcerpc_dg_auth_proto, tvb, offset, 1, hdr.auth_proto);
    offset++;

    if (tree)
        proto_tree_add_uint (dcerpc_tree, hf_dcerpc_dg_serial_lo, tvb, offset, 1, hdr.serial_lo);
    if (check_col (pinfo->cinfo, COL_INFO)) {
        if (hdr.flags1 & PFCL1_FRAG) {
            /* Fragmented - put the serial number into the Info column */
            col_append_fstr (pinfo->cinfo, COL_INFO, " serial: %u",
                             (hdr.serial_hi << 8) | hdr.serial_lo);
        }
    }
    offset++;

    if (tree) {
        /*
         * XXX - for Kerberos, we get a protection level; if it's
         * DCE_C_AUTHN_LEVEL_PKT_PRIVACY, we can't dissect the
         * stub data.
         */
        dissect_dcerpc_dg_auth (tvb, offset, dcerpc_tree, &hdr,
                                &auth_level);
    }

    /*
     * keeping track of the conversation shouldn't really be necessary
     * for connectionless packets, because everything we need to know
     * to dissect is in the header for each packet.  Unfortunately,
     * Microsoft's implementation is buggy and often puts the
     * completely wrong if_id in the header.  go figure.  So, keep
     * track of the seqnum and use that if possible.  Note: that's not
     * completely correct.  It should really be done based on both the
     * activity_id and seqnum.  I haven't seen anywhere that it would
     * make a difference, but for future reference...
     */
    conv = find_conversation (&pinfo->src, &pinfo->dst, pinfo->ptype,
                              pinfo->srcport, pinfo->destport, 0);
    if (!conv) {
        conv = conversation_new (&pinfo->src, &pinfo->dst, pinfo->ptype,
                                 pinfo->srcport, pinfo->destport, 0);
    }

    /*
     * Packet type specific stuff is next.
     */

    switch (hdr.ptype) {

    case PDU_CANCEL_ACK:
        /* Body is optional */
        /* XXX - we assume "frag_len" is the length of the body */
        if (hdr.frag_len != 0)
            dissect_dcerpc_dg_cancel_ack (tvb, offset, pinfo, dcerpc_tree, &hdr);
        break;

    case PDU_CL_CANCEL:
        /*
         * XXX - The DCE RPC 1.1 spec doesn't say the body is optional,
         * but in at least one capture none of the Cl_cancel PDUs had a
         * body.
         */
        /* XXX - we assume "frag_len" is the length of the body */
        if (hdr.frag_len != 0)
            dissect_dcerpc_dg_cancel (tvb, offset, pinfo, dcerpc_tree, &hdr);
        break;

    case PDU_NOCALL:
        /* Body is optional; if present, it's the same as PDU_FACK */
        /* XXX - we assume "frag_len" is the length of the body */
        if (hdr.frag_len != 0)
            dissect_dcerpc_dg_fack (tvb, offset, pinfo, dcerpc_tree, &hdr);
        break;

    case PDU_FACK:
        dissect_dcerpc_dg_fack (tvb, offset, pinfo, dcerpc_tree, &hdr);
        break;

    case PDU_REJECT:
    case PDU_FAULT:
        dissect_dcerpc_dg_reject_fault (tvb, offset, pinfo, dcerpc_tree, &hdr);
        break;

    case PDU_REQ:
        dissect_dcerpc_dg_rqst (tvb, offset, pinfo, dcerpc_tree, tree, &hdr, conv);
        break;

    case PDU_RESP:
        dissect_dcerpc_dg_resp (tvb, offset, pinfo, dcerpc_tree, tree, &hdr, conv);
        break;

    /* these requests have no body */
    case PDU_ACK:
    case PDU_PING:
    case PDU_WORKING:
    default:
        break;
    }

    return TRUE;
}

static void
dcerpc_init_protocol (void)
{
	/* structures and data for BIND */
	if (dcerpc_binds){
		g_hash_table_destroy (dcerpc_binds);
	}
	dcerpc_binds = g_hash_table_new (dcerpc_bind_hash, dcerpc_bind_equal);

	if (dcerpc_bind_key_chunk){
		g_mem_chunk_destroy (dcerpc_bind_key_chunk);
	}
	dcerpc_bind_key_chunk = g_mem_chunk_new ("dcerpc_bind_key_chunk",
                                             sizeof (dcerpc_bind_key),
                                             200 * sizeof (dcerpc_bind_key),
                                             G_ALLOC_ONLY);
	if (dcerpc_bind_value_chunk){
		g_mem_chunk_destroy (dcerpc_bind_value_chunk);
	}
	dcerpc_bind_value_chunk = g_mem_chunk_new ("dcerpc_bind_value_chunk",
                                             sizeof (dcerpc_bind_value),
                                             200 * sizeof (dcerpc_bind_value),
                                             G_ALLOC_ONLY);
	/* structures and data for CALL */
	if (dcerpc_calls){
		g_hash_table_destroy (dcerpc_calls);
	}
	dcerpc_calls = g_hash_table_new (dcerpc_call_hash, dcerpc_call_equal);
	if (dcerpc_call_key_chunk){
		g_mem_chunk_destroy (dcerpc_call_key_chunk);
	}
	dcerpc_call_key_chunk = g_mem_chunk_new ("dcerpc_call_key_chunk",
                                             sizeof (dcerpc_call_key),
                                             200 * sizeof (dcerpc_call_key),
                                             G_ALLOC_ONLY);
	if (dcerpc_call_value_chunk){
		g_mem_chunk_destroy (dcerpc_call_value_chunk);
	}
	dcerpc_call_value_chunk = g_mem_chunk_new ("dcerpc_call_value_chunk",
                                             sizeof (dcerpc_call_value),
                                             200 * sizeof (dcerpc_call_value),
                                             G_ALLOC_ONLY);

	/* structure and data for MATCHED */
	if (dcerpc_matched){
		g_hash_table_destroy (dcerpc_matched);
	}
	dcerpc_matched = g_hash_table_new (dcerpc_matched_hash, dcerpc_matched_equal);
	if (dcerpc_matched_key_chunk){
		g_mem_chunk_destroy (dcerpc_matched_key_chunk);
	}
	dcerpc_matched_key_chunk = g_mem_chunk_new ("dcerpc_matched_key_chunk",
                                             sizeof (dcerpc_matched_key),
                                             200 * sizeof (dcerpc_matched_key),
                                             G_ALLOC_ONLY);
}

void
proto_register_dcerpc (void)
{
    static hf_register_info hf[] = {
	{ &hf_dcerpc_request_in,
		{ "Request in frame", "dcerpc.request_in", FT_FRAMENUM, BASE_NONE,
		NULL, 0, "This packet is a response to the packet with this number", HFILL }},
	{ &hf_dcerpc_response_in,
		{ "Response in frame", "dcerpc.response_in", FT_FRAMENUM, BASE_NONE,
		NULL, 0, "This packet will be responded in the packet with this number", HFILL }},
	{ &hf_dcerpc_referent_id,
		{ "Referent ID", "dcerpc.referent_id", FT_UINT32, BASE_HEX,
		NULL, 0, "Referent ID for this NDR encoded pointer", HFILL }},
        { &hf_dcerpc_ver,
          { "Version", "dcerpc.ver", FT_UINT8, BASE_DEC, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_ver_minor,
          { "Version (minor)", "dcerpc.ver_minor", FT_UINT8, BASE_DEC, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_packet_type,
          { "Packet type", "dcerpc.pkt_type", FT_UINT8, BASE_DEC, VALS (pckt_vals), 0x0, "", HFILL }},
        { &hf_dcerpc_cn_flags,
          { "Packet Flags", "dcerpc.cn_flags", FT_UINT8, BASE_HEX, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_cn_flags_first_frag,
          { "First Frag", "dcerpc.cn_flags.first_frag", FT_BOOLEAN, 8, TFS (&flags_set_truth), PFC_FIRST_FRAG, "", HFILL }},
        { &hf_dcerpc_cn_flags_last_frag,
          { "Last Frag", "dcerpc.cn_flags.last_frag", FT_BOOLEAN, 8, TFS (&flags_set_truth), PFC_LAST_FRAG, "", HFILL }},
        { &hf_dcerpc_cn_flags_cancel_pending,
          { "Cancel Pending", "dcerpc.cn_flags.cancel_pending", FT_BOOLEAN, 8, TFS (&flags_set_truth), PFC_PENDING_CANCEL, "", HFILL }},
        { &hf_dcerpc_cn_flags_reserved,
          { "Reserved", "dcerpc.cn_flags.reserved", FT_BOOLEAN, 8, TFS (&flags_set_truth), PFC_RESERVED_1, "", HFILL }},
        { &hf_dcerpc_cn_flags_mpx,
          { "Multiplex", "dcerpc.cn_flags.mpx", FT_BOOLEAN, 8, TFS (&flags_set_truth), PFC_CONC_MPX, "", HFILL }},
        { &hf_dcerpc_cn_flags_dne,
          { "Did Not Execute", "dcerpc.cn_flags.dne", FT_BOOLEAN, 8, TFS (&flags_set_truth), PFC_DID_NOT_EXECUTE, "", HFILL }},
        { &hf_dcerpc_cn_flags_maybe,
          { "Maybe", "dcerpc.cn_flags.maybe", FT_BOOLEAN, 8, TFS (&flags_set_truth), PFC_MAYBE, "", HFILL }},
        { &hf_dcerpc_cn_flags_object,
          { "Object", "dcerpc.cn_flags.object", FT_BOOLEAN, 8, TFS (&flags_set_truth), PFC_OBJECT_UUID, "", HFILL }},
        { &hf_dcerpc_drep,
          { "Data Representation", "dcerpc.drep", FT_BYTES, BASE_HEX, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_drep_byteorder,
          { "Byte order", "dcerpc.drep.byteorder", FT_UINT8, BASE_DEC, VALS (drep_byteorder_vals), 0x0, "", HFILL }},
        { &hf_dcerpc_drep_character,
          { "Character", "dcerpc.drep.character", FT_UINT8, BASE_DEC, VALS (drep_character_vals), 0x0, "", HFILL }},
        { &hf_dcerpc_drep_fp,
          { "Floating-point", "dcerpc.drep.fp", FT_UINT8, BASE_DEC, VALS (drep_fp_vals), 0x0, "", HFILL }},
        { &hf_dcerpc_cn_frag_len,
          { "Frag Length", "dcerpc.cn_frag_len", FT_UINT16, BASE_DEC, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_cn_auth_len,
          { "Auth Length", "dcerpc.cn_auth_len", FT_UINT16, BASE_DEC, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_cn_call_id,
          { "Call ID", "dcerpc.cn_call_id", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_cn_max_xmit,
          { "Max Xmit Frag", "dcerpc.cn_max_xmit", FT_UINT16, BASE_DEC, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_cn_max_recv,
          { "Max Recv Frag", "dcerpc.cn_max_recv", FT_UINT16, BASE_DEC, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_cn_assoc_group,
          { "Assoc Group", "dcerpc.cn_assoc_group", FT_UINT32, BASE_HEX, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_cn_num_ctx_items,
          { "Num Ctx Items", "dcerpc.cn_num_ctx_items", FT_UINT8, BASE_DEC, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_cn_ctx_id,
          { "Context ID", "dcerpc.cn_ctx_id", FT_UINT16, BASE_DEC, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_cn_num_trans_items,
          { "Num Trans Items", "dcerpc.cn_num_trans_items", FT_UINT16, BASE_DEC, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_cn_bind_if_id,
          { "Interface UUID", "dcerpc.cn_bind_to_uuid", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_cn_bind_if_ver,
          { "Interface Ver", "dcerpc.cn_bind_if_ver", FT_UINT16, BASE_DEC, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_cn_bind_if_ver_minor,
          { "Interface Ver Minor", "dcerpc.cn_bind_if_ver_minor", FT_UINT16, BASE_DEC, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_cn_bind_trans_id,
          { "Transfer Syntax", "dcerpc.cn_bind_trans_id", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_cn_bind_trans_ver,
          { "Syntax ver", "dcerpc.cn_bind_trans_ver", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_cn_alloc_hint,
          { "Alloc hint", "dcerpc.cn_alloc_hint", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_cn_sec_addr_len,
          { "Scndry Addr len", "dcerpc.cn_sec_addr_len", FT_UINT16, BASE_DEC, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_cn_sec_addr,
          { "Scndry Addr", "dcerpc.cn_sec_addr", FT_STRINGZ, BASE_NONE, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_cn_num_results,
          { "Num results", "dcerpc.cn_num_results", FT_UINT8, BASE_DEC, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_cn_ack_result,
          { "Ack result", "dcerpc.cn_ack_result", FT_UINT16, BASE_DEC, VALS(p_cont_result_vals), 0x0, "", HFILL }},
        { &hf_dcerpc_cn_ack_reason,
          { "Ack reason", "dcerpc.cn_ack_reason", FT_UINT16, BASE_DEC, VALS(p_provider_reason_vals), 0x0, "", HFILL }},
        { &hf_dcerpc_cn_ack_trans_id,
          { "Transfer Syntax", "dcerpc.cn_ack_trans_id", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_cn_ack_trans_ver,
          { "Syntax ver", "dcerpc.cn_ack_trans_ver", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_cn_reject_reason,
          { "Reject reason", "dcerpc.cn_reject_reason", FT_UINT16, BASE_DEC, VALS(reject_reason_vals), 0x0, "", HFILL }},
        { &hf_dcerpc_cn_num_protocols,
          { "Number of protocols", "dcerpc.cn_num_protocols", FT_UINT8, BASE_DEC, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_cn_protocol_ver_major,
          { "Protocol major version", "dcerpc.cn_protocol_ver_major", FT_UINT8, BASE_DEC, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_cn_protocol_ver_minor,
          { "Protocol minor version", "dcerpc.cn_protocol_ver_minor", FT_UINT8, BASE_DEC, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_cn_cancel_count,
          { "Cancel count", "dcerpc.cn_cancel_count", FT_UINT8, BASE_DEC, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_cn_status,
          { "Status", "dcerpc.cn_status", FT_UINT32, BASE_HEX, VALS(reject_status_vals), 0x0, "", HFILL }},
        { &hf_dcerpc_auth_type,
          { "Auth type", "dcerpc.auth_type", FT_UINT8, BASE_DEC, VALS (authn_protocol_vals), 0x0, "", HFILL }},
        { &hf_dcerpc_auth_level,
          { "Auth level", "dcerpc.auth_level", FT_UINT8, BASE_DEC, VALS (authn_level_vals), 0x0, "", HFILL }},
        { &hf_dcerpc_auth_pad_len,
          { "Auth pad len", "dcerpc.auth_pad_len", FT_UINT8, BASE_DEC, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_auth_rsrvd,
          { "Auth Rsrvd", "dcerpc.auth_rsrvd", FT_UINT8, BASE_DEC, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_auth_ctx_id,
          { "Auth Context ID", "dcerpc.auth_ctx_id", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_dg_flags1,
          { "Flags1", "dcerpc.dg_flags1", FT_UINT8, BASE_HEX, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_dg_flags1_rsrvd_01,
          { "Reserved", "dcerpc.dg_flags1_rsrvd_01", FT_BOOLEAN, 8, TFS (&flags_set_truth), PFCL1_RESERVED_01, "", HFILL }},
        { &hf_dcerpc_dg_flags1_last_frag,
          { "Last Fragment", "dcerpc.dg_flags1_last_frag", FT_BOOLEAN, 8, TFS (&flags_set_truth), PFCL1_LASTFRAG, "", HFILL }},
        { &hf_dcerpc_dg_flags1_frag,
          { "Fragment", "dcerpc.dg_flags1_frag", FT_BOOLEAN, 8, TFS (&flags_set_truth), PFCL1_FRAG, "", HFILL }},
        { &hf_dcerpc_dg_flags1_nofack,
          { "No Fack", "dcerpc.dg_flags1_nofack", FT_BOOLEAN, 8, TFS (&flags_set_truth), PFCL1_NOFACK, "", HFILL }},
        { &hf_dcerpc_dg_flags1_maybe,
          { "Maybe", "dcerpc.dg_flags1_maybe", FT_BOOLEAN, 8, TFS (&flags_set_truth), PFCL1_MAYBE, "", HFILL }},
        { &hf_dcerpc_dg_flags1_idempotent,
          { "Idempotent", "dcerpc.dg_flags1_idempotent", FT_BOOLEAN, 8, TFS (&flags_set_truth), PFCL1_IDEMPOTENT, "", HFILL }},
        { &hf_dcerpc_dg_flags1_broadcast,
          { "Broadcast", "dcerpc.dg_flags1_broadcast", FT_BOOLEAN, 8, TFS (&flags_set_truth), PFCL1_BROADCAST, "", HFILL }},
        { &hf_dcerpc_dg_flags1_rsrvd_80,
          { "Reserved", "dcerpc.dg_flags1_rsrvd_80", FT_BOOLEAN, 8, TFS (&flags_set_truth), PFCL1_RESERVED_80, "", HFILL }},
        { &hf_dcerpc_dg_flags2,
          { "Flags2", "dcerpc.dg_flags2", FT_UINT8, BASE_HEX, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_dg_flags2_rsrvd_01,
          { "Reserved", "dcerpc.dg_flags2_rsrvd_01", FT_BOOLEAN, 8, TFS (&flags_set_truth), PFCL2_RESERVED_01, "", HFILL }},
        { &hf_dcerpc_dg_flags2_cancel_pending,
          { "Cancel Pending", "dcerpc.dg_flags2_cancel_pending", FT_BOOLEAN, 8, TFS (&flags_set_truth), PFCL2_CANCEL_PENDING, "", HFILL }},
        { &hf_dcerpc_dg_flags2_rsrvd_04,
          { "Reserved", "dcerpc.dg_flags2_rsrvd_04", FT_BOOLEAN, 8, TFS (&flags_set_truth), PFCL2_RESERVED_04, "", HFILL }},
        { &hf_dcerpc_dg_flags2_rsrvd_08,
          { "Reserved", "dcerpc.dg_flags2_rsrvd_08", FT_BOOLEAN, 8, TFS (&flags_set_truth), PFCL2_RESERVED_08, "", HFILL }},
        { &hf_dcerpc_dg_flags2_rsrvd_10,
          { "Reserved", "dcerpc.dg_flags2_rsrvd_10", FT_BOOLEAN, 8, TFS (&flags_set_truth), PFCL2_RESERVED_10, "", HFILL }},
        { &hf_dcerpc_dg_flags2_rsrvd_20,
          { "Reserved", "dcerpc.dg_flags2_rsrvd_20", FT_BOOLEAN, 8, TFS (&flags_set_truth), PFCL2_RESERVED_20, "", HFILL }},
        { &hf_dcerpc_dg_flags2_rsrvd_40,
          { "Reserved", "dcerpc.dg_flags2_rsrvd_40", FT_BOOLEAN, 8, TFS (&flags_set_truth), PFCL2_RESERVED_40, "", HFILL }},
        { &hf_dcerpc_dg_flags2_rsrvd_80,
          { "Reserved", "dcerpc.dg_flags2_rsrvd_80", FT_BOOLEAN, 8, TFS (&flags_set_truth), PFCL2_RESERVED_80, "", HFILL }},
        { &hf_dcerpc_dg_serial_lo,
          { "Serial Low", "dcerpc.dg_serial_lo", FT_UINT8, BASE_HEX, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_dg_serial_hi,
          { "Serial High", "dcerpc.dg_serial_hi", FT_UINT8, BASE_HEX, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_dg_ahint,
          { "Activity Hint", "dcerpc.dg_ahint", FT_UINT16, BASE_HEX, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_dg_ihint,
          { "Interface Hint", "dcerpc.dg_ihint", FT_UINT16, BASE_HEX, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_dg_frag_len,
          { "Fragment len", "dcerpc.dg_frag_len", FT_UINT16, BASE_DEC, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_dg_frag_num,
          { "Fragment num", "dcerpc.dg_frag_num", FT_UINT16, BASE_DEC, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_dg_auth_proto,
          { "Auth proto", "dcerpc.dg_auth_proto", FT_UINT8, BASE_DEC, VALS (authn_protocol_vals), 0x0, "", HFILL }},
        { &hf_dcerpc_dg_seqnum,
          { "Sequence num", "dcerpc.dg_seqnum", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_dg_server_boot,
          { "Server boot time", "dcerpc.dg_server_boot", FT_ABSOLUTE_TIME, BASE_NONE, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_dg_if_ver,
          { "Interface Ver", "dcerpc.dg_if_ver", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_krb5_av_prot_level,
          { "Protection Level", "dcerpc.krb5_av.prot_level", FT_UINT8, BASE_DEC, VALS(authn_level_vals), 0x0, "", HFILL }},
        { &hf_dcerpc_krb5_av_key_vers_num,
          { "Key Version Number", "dcerpc.krb5_av.key_vers_num", FT_UINT8, BASE_DEC, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_krb5_av_key_auth_verifier,
          { "Authentication Verifier", "dcerpc.krb5_av.auth_verifier", FT_BYTES, BASE_NONE, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_obj_id,
          { "Object", "dcerpc.obj_id", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_dg_if_id,
          { "Interface", "dcerpc.dg_if_id", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_dg_act_id,
          { "Activity", "dcerpc.dg_act_id", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL }},
        { &hf_dcerpc_opnum,
          { "Opnum", "dcerpc.opnum", FT_UINT16, BASE_DEC, NULL, 0x0, "", HFILL }},

        { &hf_dcerpc_dg_cancel_vers,
          { "Cancel Version", "dcerpc.dg_cancel_vers", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL }},

        { &hf_dcerpc_dg_cancel_id,
          { "Cancel ID", "dcerpc.dg_cancel_id", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL }},

        { &hf_dcerpc_dg_server_accepting_cancels,
          { "Server accepting cancels", "dcerpc.server_accepting_cancels", FT_BOOLEAN, BASE_NONE, NULL, 0x0, "", HFILL }},

        { &hf_dcerpc_dg_fack_vers,
          { "FACK Version", "dcerpc.fack_vers", FT_UINT8, BASE_DEC, NULL, 0x0, "", HFILL }},

        { &hf_dcerpc_dg_fack_window_size,
          { "Window Size", "dcerpc.fack_window_size", FT_UINT16, BASE_DEC, NULL, 0x0, "", HFILL }},

        { &hf_dcerpc_dg_fack_max_tsdu,
          { "Max TSDU", "dcerpc.fack_max_tsdu", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL }},

        { &hf_dcerpc_dg_fack_max_frag_size,
          { "Max Frag Size", "dcerpc.fack_max_frag_size", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL }},

        { &hf_dcerpc_dg_fack_serial_num,
          { "Serial Num", "dcerpc.fack_serial_num", FT_UINT16, BASE_DEC, NULL, 0x0, "", HFILL }},

        { &hf_dcerpc_dg_fack_selack_len,
          { "Selective ACK Len", "dcerpc.fack_selack_len", FT_UINT16, BASE_DEC, NULL, 0x0, "", HFILL }},

        { &hf_dcerpc_dg_fack_selack,
          { "Selective ACK", "dcerpc.fack_selack", FT_UINT32, BASE_HEX, NULL, 0x0, "", HFILL }},

        { &hf_dcerpc_dg_status,
          { "Status", "dcerpc.dg_status", FT_UINT32, BASE_HEX, VALS(reject_status_vals), 0x0, "", HFILL }},

        { &hf_dcerpc_array_max_count,
          { "Max Count", "dcerpc.array.max_count", FT_UINT32, BASE_DEC, NULL, 0x0, "Maximum Count: Number of elements in the array", HFILL }},

        { &hf_dcerpc_array_offset,
          { "Offset", "dcerpc.array.offset", FT_UINT32, BASE_DEC, NULL, 0x0, "Offset for first element in array", HFILL }},

        { &hf_dcerpc_array_actual_count,
          { "Actual Count", "dcerpc.array.actual_count", FT_UINT32, BASE_DEC, NULL, 0x0, "Actual Count: Actual number of elements in the array", HFILL }},

	{ &hf_dcerpc_array_buffer,
	  { "Buffer", "dcerpc.array.buffer", FT_BYTES, BASE_NONE, NULL, 0x0, "Buffer: Buffer containing elements of the array", HFILL }},
		
        { &hf_dcerpc_op,
          { "Operation", "dcerpc.op", FT_UINT16, BASE_DEC, NULL, 0x0, "", HFILL }},

	{ &hf_dcerpc_fragments,
	  { "DCE/RPC Fragments", "dcerpc.fragments", FT_NONE, BASE_NONE,
	  NULL, 0x0, "DCE/RPC Fragments", HFILL }},

	{ &hf_dcerpc_fragment,
	  { "DCE/RPC Fragment", "dcerpc.fragment", FT_FRAMENUM, BASE_NONE,
	  NULL, 0x0, "DCE/RPC Fragment", HFILL }},

	{ &hf_dcerpc_fragment_overlap,
	  { "Fragment overlap",	"dcerpc.fragment.overlap", FT_BOOLEAN, BASE_NONE, 
      NULL, 0x0, "Fragment overlaps with other fragments", HFILL }},

	{ &hf_dcerpc_fragment_overlap_conflict,
	  { "Conflicting data in fragment overlap", "dcerpc.fragment.overlap.conflict", FT_BOOLEAN, BASE_NONE, 
      NULL, 0x0, "Overlapping fragments contained conflicting data", HFILL }},

	{ &hf_dcerpc_fragment_multiple_tails,
	  { "Multiple tail fragments found", "dcerpc.fragment.multipletails", FT_BOOLEAN, BASE_NONE, 
      NULL, 0x0, "Several tails were found when defragmenting the packet", HFILL }},

	{ &hf_dcerpc_fragment_too_long_fragment,
	  { "Fragment too long", "dcerpc.fragment.toolongfragment", FT_BOOLEAN, BASE_NONE, 
      NULL, 0x0, "Fragment contained data past end of packet", HFILL }},

	{ &hf_dcerpc_fragment_error,
	  { "Defragmentation error", "dcerpc.fragment.error", FT_FRAMENUM, BASE_NONE, 
      NULL, 0x0, "Defragmentation error due to illegal fragments", HFILL }},

	{ &hf_dcerpc_time, 
	  { "Time from request", "dcerpc.time", FT_RELATIVE_TIME, BASE_NONE, 
      NULL, 0, "Time between Request and Response for DCE-RPC calls", HFILL }},

	{ &hf_dcerpc_reassembled_in,
	  { "Reassembled PDU in frame", "dcerpc.reassembled_in", FT_FRAMENUM, BASE_NONE, 
      NULL, 0x0, "The DCE/RPC PDU is completely reassembled in the packet with this number", HFILL }},

	{ &hf_dcerpc_unknown_if_id, 
	  { "Unknown DCERPC interface id", "dcerpc.unknown_if_id", FT_BOOLEAN, BASE_NONE, NULL, 0x0, "", HFILL }},
   };
    static gint *ett[] = {
        &ett_dcerpc,
        &ett_dcerpc_cn_flags,
        &ett_dcerpc_cn_ctx,
        &ett_dcerpc_cn_iface,
        &ett_dcerpc_drep,
        &ett_dcerpc_dg_flags1,
        &ett_dcerpc_dg_flags2,
        &ett_dcerpc_pointer_data,
        &ett_dcerpc_string,
        &ett_dcerpc_fragments,
        &ett_dcerpc_fragment,
        &ett_dcerpc_krb5_auth_verf,
    };
    module_t *dcerpc_module;

    proto_dcerpc = proto_register_protocol ("DCE RPC", "DCERPC", "dcerpc");
    proto_register_field_array (proto_dcerpc, hf, array_length (hf));
    proto_register_subtree_array (ett, array_length (ett));
    register_init_routine (dcerpc_init_protocol);
    dcerpc_module = prefs_register_protocol (proto_dcerpc, NULL);
    prefs_register_bool_preference (dcerpc_module,
                                    "desegment_dcerpc",
                                    "Desegment all DCE/RPC over TCP",
                                    "Whether the DCE/RPC dissector should desegment all DCE/RPC over TCP",
                                    &dcerpc_cn_desegment);
    prefs_register_bool_preference (dcerpc_module,
                                    "reassemble_dcerpc",
                                    "Reassemble DCE/RPC fragments",
                                    "Whether the DCE/RPC dissector should reassemble all fragmented PDUs",
                                    &dcerpc_reassemble);
    register_init_routine(dcerpc_reassemble_init);
    dcerpc_uuids = g_hash_table_new (dcerpc_uuid_hash, dcerpc_uuid_equal);
    dcerpc_tap=register_tap("dcerpc");
}

void
proto_reg_handoff_dcerpc (void)
{
    heur_dissector_add ("tcp", dissect_dcerpc_cn_bs, proto_dcerpc);
    heur_dissector_add ("netbios", dissect_dcerpc_cn_pk, proto_dcerpc);
    heur_dissector_add ("udp", dissect_dcerpc_dg, proto_dcerpc);
    heur_dissector_add ("smb_transact", dissect_dcerpc_cn_bs, proto_dcerpc);
    dcerpc_smb_init(proto_dcerpc);
}
