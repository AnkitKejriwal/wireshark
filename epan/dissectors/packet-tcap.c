/* Do not modify this file.                                                   */
/* It is created automatically by the ASN.1 to Ethereal dissector compiler    */
/* .\packet-tcap.c                                                            */
/* ../../tools/asn2eth.py -X -b -e -p tcap -c tcap.cnf -s packet-tcap-template tcap.asn */

/* Input file: packet-tcap-template.c */

/* packet-tcap-template.c
 * Routines for  TCAP
 * Copyright 2004 - 2005, Tim Endean <endeant@hotmail.com>
 * Built from the gsm-map dissector Copyright 2004 - 2005, Anders Broman <anders.broman@ericsson.com>
 *
 * $Id$
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
 * References: ETSI 300 374
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glib.h>
#include <epan/packet.h>
#include <epan/prefs.h>
#include <epan/conversation.h>

#include <stdio.h>
#include <string.h>
#include <epan/asn1.h>
#include "packet-ber.h"
#include "packet-tcap.h"

#define PNAME  "Transaction Capabilities Application Part"
#define PSNAME "TCAP"
#define PFNAME "tcap"

/* Initialize the protocol and registered fields */
int proto_tcap = -1;
static int hf_tcap_tag = -1;         
static int hf_tcap_length = -1;         
static int hf_tcap_data = -1;

/*--- Included file: packet-tcap-hf.c ---*/

static int hf_tcap_dialogueRequest = -1;          /* AARQ_apdu */
static int hf_tcap_dialogueResponse = -1;         /* AARE_apdu */
static int hf_tcap_dialogueAbort = -1;            /* ABRT_apdu */
static int hf_tcap_oid = -1;                      /* OBJECT_IDENTIFIER */
static int hf_tcap_dialog = -1;                   /* Dialog1 */
static int hf_tcap_useroid = -1;                  /* UserInfoOID */
static int hf_tcap_externuserinfo = -1;           /* ExternUserInfo */
static int hf_tcap_protocol_versionrq = -1;       /* T_protocol_versionrq */
static int hf_tcap_application_context_name = -1;  /* Applicationcontext */
static int hf_tcap_user_information = -1;         /* User_information */
static int hf_tcap_protocol_versionre = -1;       /* T_protocol_versionre */
static int hf_tcap_result = -1;                   /* Associate_result */
static int hf_tcap_result_source_diagnostic = -1;  /* Associate_source_diagnostic */
static int hf_tcap_reasonrq = -1;                 /* Release_request_reason */
static int hf_tcap_reasonre = -1;                 /* Release_response_reason */
static int hf_tcap_abort_source = -1;             /* ABRT_source */
static int hf_tcap_dialogue_service_user = -1;    /* T_dialogue_service_user */
static int hf_tcap_dialogue_service_provider = -1;  /* T_dialogue_service_provider */
static int hf_tcap_unidialoguePDU = -1;           /* AUDT_apdu */
static int hf_tcap_protocol_version3 = -1;        /* T_protocol_version3 */
static int hf_tcap_unidirectional = -1;           /* Unidirectional */
static int hf_tcap_begin = -1;                    /* Begin */
static int hf_tcap_end = -1;                      /* End */
static int hf_tcap_continue = -1;                 /* Continue */
static int hf_tcap_abort = -1;                    /* Abort */
static int hf_tcap_ansiunidirectional = -1;       /* UniTransactionPDU */
static int hf_tcap_ansiqueryWithPerm = -1;        /* TransactionPDU */
static int hf_tcap_ansiqueryWithoutPerm = -1;     /* TransactionPDU */
static int hf_tcap_ansiresponse = -1;             /* TransactionPDU */
static int hf_tcap_ansiconversationWithPerm = -1;  /* TransactionPDU */
static int hf_tcap_ansiconversationWithoutPerm = -1;  /* TransactionPDU */
static int hf_tcap_ansiabort = -1;                /* AbortPDU */
static int hf_tcap_dialoguePortion = -1;          /* DialoguePortion */
static int hf_tcap_components = -1;               /* ComponentPortion */
static int hf_tcap_otid = -1;                     /* OrigTransactionID */
static int hf_tcap_dtid = -1;                     /* DestTransactionID */
static int hf_tcap_reason = -1;                   /* Reason */
static int hf_tcap_p_abortCause = -1;             /* P_AbortCause */
static int hf_tcap_u_abortCause = -1;             /* DialoguePortion */
static int hf_tcap_ComponentPortion_item = -1;    /* Component */
static int hf_tcap_invoke = -1;                   /* Invoke */
static int hf_tcap_returnResultLast = -1;         /* ReturnResult */
static int hf_tcap_returnError = -1;              /* ReturnError */
static int hf_tcap_reject = -1;                   /* Reject */
static int hf_tcap_returnResultNotLast = -1;      /* ReturnResult */
static int hf_tcap_invokeID = -1;                 /* InvokeIdType */
static int hf_tcap_linkedID = -1;                 /* InvokeIdType */
static int hf_tcap_opCode = -1;                   /* OPERATION */
static int hf_tcap_parameter = -1;                /* Parameter */
static int hf_tcap_resultretres = -1;             /* T_resultretres */
static int hf_tcap_errorCode = -1;                /* ErrorCode */
static int hf_tcap_invokeIDRej = -1;              /* T_invokeIDRej */
static int hf_tcap_derivable = -1;                /* InvokeIdType */
static int hf_tcap_not_derivable = -1;            /* NULL */
static int hf_tcap_problem = -1;                  /* T_problem */
static int hf_tcap_generalProblem = -1;           /* GeneralProblem */
static int hf_tcap_invokeProblem = -1;            /* InvokeProblem */
static int hf_tcap_returnResultProblem = -1;      /* ReturnResultProblem */
static int hf_tcap_returnErrorProblem = -1;       /* ReturnErrorProblem */
static int hf_tcap_localValue = -1;               /* INTEGER */
static int hf_tcap_globalValue = -1;              /* OBJECT_IDENTIFIER */
static int hf_tcap_identifier = -1;               /* TransactionID */
static int hf_tcap_dialoguePortionansi = -1;      /* DialoguePortionANSI */
static int hf_tcap_componentPortion = -1;         /* ComponentSequence */
static int hf_tcap_causeInformation = -1;         /* T_causeInformation */
static int hf_tcap_abortCause = -1;               /* P_Abort_cause */
static int hf_tcap_userInformation = -1;          /* UserInformation */
static int hf_tcap_version = -1;                  /* ProtocolVersion */
static int hf_tcap_applicationContext = -1;       /* T_applicationContext */
static int hf_tcap_integerApplicationId = -1;     /* IntegerApplicationContext */
static int hf_tcap_objectApplicationId = -1;      /* ObjectIDApplicationContext */
static int hf_tcap_securityContext = -1;          /* T_securityContext */
static int hf_tcap_integerSecurityId = -1;        /* INTEGER */
static int hf_tcap_objectSecurityId = -1;         /* OBJECT_IDENTIFIER */
static int hf_tcap_confidentiality = -1;          /* Confidentiality */
static int hf_tcap_confidentialityId = -1;        /* T_confidentialityId */
static int hf_tcap_integerConfidentialityId = -1;  /* INTEGER */
static int hf_tcap_objectConfidentialityId = -1;  /* OBJECT_IDENTIFIER */
static int hf_tcap_ComponentSequence_item = -1;   /* ComponentPDU */
static int hf_tcap_invokeLastansi = -1;           /* InvokePDU */
static int hf_tcap_returnResultLastansi = -1;     /* ReturnResultPDU */
static int hf_tcap_returnErroransi = -1;          /* ReturnErrorPDU */
static int hf_tcap_rejectansi = -1;               /* RejectPDU */
static int hf_tcap_invokeNotLastansi = -1;        /* InvokePDU */
static int hf_tcap_returnResultNotLastansi = -1;  /* ReturnResultPDU */
static int hf_tcap_componentIDs = -1;             /* OCTET_STRING_SIZE_0_2 */
static int hf_tcap_operationCode = -1;            /* OperationCode */
static int hf_tcap_parameterinv = -1;             /* ANSIparamch */
static int hf_tcap_ansiparams = -1;               /* ANSIParameters */
static int hf_tcap_ansiparams1 = -1;              /* ANSIParameters */
static int hf_tcap_ansiparams2 = -1;              /* ANSIParameters */
static int hf_tcap_ansiparams3 = -1;              /* ANSIParameters */
static int hf_tcap_ansiparams4 = -1;              /* ANSIParameters */
static int hf_tcap_ansiparams5 = -1;              /* ANSIParameters */
static int hf_tcap_ansiparams6 = -1;              /* ANSIParameters */
static int hf_tcap_ansiparams7 = -1;              /* ANSIParameters */
static int hf_tcap_ansiparams8 = -1;              /* ANSIParameters */
static int hf_tcap_ansiparams9 = -1;              /* ANSIParameters */
static int hf_tcap_ansiparams10 = -1;             /* ANSIParameters */
static int hf_tcap_ansiparams11 = -1;             /* ANSIParameters */
static int hf_tcap_ansiparams12 = -1;             /* ANSIParameters */
static int hf_tcap_ansiparams13 = -1;             /* ANSIParameters */
static int hf_tcap_ansiparams14 = -1;             /* ANSIParameters */
static int hf_tcap_ansiparams15 = -1;             /* ANSIParameters */
static int hf_tcap_ansiparams16 = -1;             /* ANSIParameters */
static int hf_tcap_ansiparams17 = -1;             /* ANSIParameters */
static int hf_tcap_ansiparams18 = -1;             /* ANSIParameters */
static int hf_tcap_ansiparams19 = -1;             /* ANSIParameters */
static int hf_tcap_ansiparams20 = -1;             /* ANSIParameters */
static int hf_tcap_ansiparams21 = -1;             /* ANSIParameters */
static int hf_tcap_componentID = -1;              /* ComponentID */
static int hf_tcap_parameterrr = -1;              /* ANSIparamch */
static int hf_tcap_parameterre = -1;              /* ANSIparamch */
static int hf_tcap_rejectProblem = -1;            /* ProblemPDU */
static int hf_tcap_parameterrj = -1;              /* ANSIparamch */
static int hf_tcap_national = -1;                 /* INTEGER_M32768_32767 */
static int hf_tcap_private = -1;                  /* INTEGER */
static int hf_tcap_nationaler = -1;               /* INTEGER_M32768_32767 */
static int hf_tcap_privateer = -1;                /* INTEGER */
/* named bits */
static int hf_tcap_T_protocol_versionrq_version1 = -1;
static int hf_tcap_T_protocol_versionre_version1 = -1;
static int hf_tcap_T_protocol_version3_version1 = -1;

/*--- End of included file: packet-tcap-hf.c ---*/

static guint tcap_itu_ssn = 106;

static guint global_tcap_itu_ssn = 1;

/* Initialize the subtree pointers */
static gint ett_tcap = -1;
static gint ett_param = -1;


/*--- Included file: packet-tcap-ett.c ---*/

static gint ett_tcap_DialoguePDU = -1;
static gint ett_tcap_ExternalPDU = -1;
static gint ett_tcap_UserInformation = -1;
static gint ett_tcap_AARQ_apdu = -1;
static gint ett_tcap_T_protocol_versionrq = -1;
static gint ett_tcap_AARE_apdu = -1;
static gint ett_tcap_T_protocol_versionre = -1;
static gint ett_tcap_RLRQ_apdu = -1;
static gint ett_tcap_RLRE_apdu = -1;
static gint ett_tcap_ABRT_apdu = -1;
static gint ett_tcap_Associate_source_diagnostic = -1;
static gint ett_tcap_UniDialoguePDU = -1;
static gint ett_tcap_AUDT_apdu = -1;
static gint ett_tcap_T_protocol_version3 = -1;
static gint ett_tcap_MessageType = -1;
static gint ett_tcap_Unidirectional = -1;
static gint ett_tcap_Begin = -1;
static gint ett_tcap_End = -1;
static gint ett_tcap_Continue = -1;
static gint ett_tcap_Abort = -1;
static gint ett_tcap_Reason = -1;
static gint ett_tcap_ComponentPortion = -1;
static gint ett_tcap_Component = -1;
static gint ett_tcap_Invoke = -1;
static gint ett_tcap_ReturnResult = -1;
static gint ett_tcap_T_resultretres = -1;
static gint ett_tcap_ReturnError = -1;
static gint ett_tcap_Reject = -1;
static gint ett_tcap_T_invokeIDRej = -1;
static gint ett_tcap_T_problem = -1;
static gint ett_tcap_OPERATION = -1;
static gint ett_tcap_ERROR = -1;
static gint ett_tcap_UniTransactionPDU = -1;
static gint ett_tcap_TransactionPDU = -1;
static gint ett_tcap_AbortPDU = -1;
static gint ett_tcap_T_causeInformation = -1;
static gint ett_tcap_DialoguePortionANSI = -1;
static gint ett_tcap_T_applicationContext = -1;
static gint ett_tcap_T_securityContext = -1;
static gint ett_tcap_Confidentiality = -1;
static gint ett_tcap_T_confidentialityId = -1;
static gint ett_tcap_ComponentSequence = -1;
static gint ett_tcap_ComponentPDU = -1;
static gint ett_tcap_InvokePDU = -1;
static gint ett_tcap_ANSIparamch = -1;
static gint ett_tcap_ReturnResultPDU = -1;
static gint ett_tcap_ReturnErrorPDU = -1;
static gint ett_tcap_RejectPDU = -1;
static gint ett_tcap_OperationCode = -1;
static gint ett_tcap_ErrorCode = -1;

/*--- End of included file: packet-tcap-ett.c ---*/


#define MAX_SSN 254
static range_t *global_ssn_range;
static range_t *ssn_range;

static packet_info *g_pinfo = NULL;
static proto_tree *g_tcap_tree = NULL;
static gboolean g_tcap_ends_def_len = FALSE;


dissector_handle_t	tcap_handle;
static dissector_table_t ber_oid_dissector_table=NULL;
static char * cur_oid;
static char * tcapext_oid;
static proto_tree	*tcap_top_tree=NULL;
static dissector_handle_t data_handle;
static dissector_table_t tcap_itu_ssn_dissector_table; /* map use ssn in sccp */
static dissector_table_t tcap_ansi_ssn_dissector_table; /* map use ssn in sccp */

static int dissect_tcap_param(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset);
static int dissect_tcap_UserInformation(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_);



/*--- Included file: packet-tcap-fn.c ---*/

/*--- Fields for imported types ---*/


static const asn_namedbit T_protocol_versionrq_bits[] = {
  {  0, &hf_tcap_T_protocol_versionrq_version1, -1, -1, "version1", NULL },
  { 0, NULL, 0, 0, NULL, NULL }
};

static int
dissect_tcap_T_protocol_versionrq(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_bitstring(implicit_tag, pinfo, tree, tvb, offset,
                                 T_protocol_versionrq_bits, hf_index, ett_tcap_T_protocol_versionrq,
                                 NULL);

  return offset;
}
static int dissect_protocol_versionrq_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_T_protocol_versionrq(TRUE, tvb, offset, pinfo, tree, hf_tcap_protocol_versionrq);
}



static int
dissect_tcap_Applicationcontext(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
	static char buffer[128];
	cur_oid = buffer;
	pinfo->private_data = buffer;
  offset = dissect_ber_object_identifier(FALSE, pinfo, tree, tvb, offset,
                                         hf_index, cur_oid);


  return offset;
}
static int dissect_application_context_name(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_Applicationcontext(FALSE, tvb, offset, pinfo, tree, hf_tcap_application_context_name);
}


static int
dissect_tcap_User_information(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
tvbuff_t	*parameter_tvb;
tvbuff_t	*next_tvb;
guint8 class;
	gboolean pc;
	guint32 tag;
	guint32 len;
	guint32 ind_field; 


offset = get_ber_identifier(tvb, offset, &class, &pc, &tag);
offset = get_ber_length(tree, tvb, offset, &len, &ind_field);
next_tvb = tvb_new_subset(tvb, offset, len, len);		
dissect_ber_octet_string(TRUE, pinfo, tree, next_tvb, 0, hf_index,
                                    &parameter_tvb);
 
dissect_tcap_UserInformation(TRUE, parameter_tvb, 0, pinfo, tree, -1);
 
 
return offset+len;

  return offset;
}
static int dissect_user_information_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_User_information(TRUE, tvb, offset, pinfo, tree, hf_tcap_user_information);
}

static const ber_sequence_t AARQ_apdu_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_protocol_versionrq_impl },
  { BER_CLASS_CON, 1, 0, dissect_application_context_name },
  { BER_CLASS_CON, 30, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_user_information_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_tcap_AARQ_apdu(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                AARQ_apdu_sequence, hf_index, ett_tcap_AARQ_apdu);

  return offset;
}
static int dissect_dialogueRequest(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_AARQ_apdu(FALSE, tvb, offset, pinfo, tree, hf_tcap_dialogueRequest);
}

static const asn_namedbit T_protocol_versionre_bits[] = {
  {  0, &hf_tcap_T_protocol_versionre_version1, -1, -1, "version1", NULL },
  { 0, NULL, 0, 0, NULL, NULL }
};

static int
dissect_tcap_T_protocol_versionre(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_bitstring(implicit_tag, pinfo, tree, tvb, offset,
                                 T_protocol_versionre_bits, hf_index, ett_tcap_T_protocol_versionre,
                                 NULL);

  return offset;
}
static int dissect_protocol_versionre_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_T_protocol_versionre(TRUE, tvb, offset, pinfo, tree, hf_tcap_protocol_versionre);
}


static const value_string tcap_Associate_result_vals[] = {
  {   0, "accepted" },
  {   1, "reject-permanent" },
  { 0, NULL }
};


static int
dissect_tcap_Associate_result(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_result(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_Associate_result(FALSE, tvb, offset, pinfo, tree, hf_tcap_result);
}


static const value_string tcap_T_dialogue_service_user_vals[] = {
  {   0, "null" },
  {   1, "no-reason-given" },
  {   2, "application-context-name-not-supported" },
  { 0, NULL }
};


static int
dissect_tcap_T_dialogue_service_user(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_dialogue_service_user(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_T_dialogue_service_user(FALSE, tvb, offset, pinfo, tree, hf_tcap_dialogue_service_user);
}


static const value_string tcap_T_dialogue_service_provider_vals[] = {
  {   0, "null" },
  {   1, "no-reason-given" },
  {   2, "no-common-dialogue-portion" },
  { 0, NULL }
};


static int
dissect_tcap_T_dialogue_service_provider(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_dialogue_service_provider(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_T_dialogue_service_provider(FALSE, tvb, offset, pinfo, tree, hf_tcap_dialogue_service_provider);
}


static const value_string tcap_Associate_source_diagnostic_vals[] = {
  {   1, "dialogue-service-user" },
  {   2, "dialogue-service-provider" },
  { 0, NULL }
};

static const ber_choice_t Associate_source_diagnostic_choice[] = {
  {   1, BER_CLASS_CON, 1, 0, dissect_dialogue_service_user },
  {   2, BER_CLASS_CON, 2, 0, dissect_dialogue_service_provider },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_tcap_Associate_source_diagnostic(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              Associate_source_diagnostic_choice, hf_index, ett_tcap_Associate_source_diagnostic);

  return offset;
}
static int dissect_result_source_diagnostic(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_Associate_source_diagnostic(FALSE, tvb, offset, pinfo, tree, hf_tcap_result_source_diagnostic);
}

static const ber_sequence_t AARE_apdu_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_protocol_versionre_impl },
  { BER_CLASS_CON, 1, 0, dissect_application_context_name },
  { BER_CLASS_CON, 2, 0, dissect_result },
  { BER_CLASS_CON, 3, BER_FLAGS_NOTCHKTAG, dissect_result_source_diagnostic },
  { BER_CLASS_CON, 30, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_user_information_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_tcap_AARE_apdu(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                AARE_apdu_sequence, hf_index, ett_tcap_AARE_apdu);

  return offset;
}
static int dissect_dialogueResponse(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_AARE_apdu(FALSE, tvb, offset, pinfo, tree, hf_tcap_dialogueResponse);
}


static const value_string tcap_ABRT_source_vals[] = {
  {   0, "dialogue-service-user" },
  {   1, "dialogue-service-provider" },
  { 0, NULL }
};


static int
dissect_tcap_ABRT_source(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_abort_source_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_ABRT_source(TRUE, tvb, offset, pinfo, tree, hf_tcap_abort_source);
}

static const ber_sequence_t ABRT_apdu_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_abort_source_impl },
  { BER_CLASS_CON, 30, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_user_information_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_tcap_ABRT_apdu(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                ABRT_apdu_sequence, hf_index, ett_tcap_ABRT_apdu);

  return offset;
}
static int dissect_dialogueAbort(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_ABRT_apdu(FALSE, tvb, offset, pinfo, tree, hf_tcap_dialogueAbort);
}


static const value_string tcap_DialoguePDU_vals[] = {
  {   0, "dialogueRequest" },
  {   1, "dialogueResponse" },
  {   4, "dialogueAbort" },
  { 0, NULL }
};

static const ber_choice_t DialoguePDU_choice[] = {
  {   0, BER_CLASS_APP, 0, BER_FLAGS_NOOWNTAG, dissect_dialogueRequest },
  {   1, BER_CLASS_APP, 1, BER_FLAGS_NOOWNTAG, dissect_dialogueResponse },
  {   4, BER_CLASS_APP, 4, BER_FLAGS_NOOWNTAG, dissect_dialogueAbort },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_tcap_DialoguePDU(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              DialoguePDU_choice, hf_index, ett_tcap_DialoguePDU);

  return offset;
}



static int
dissect_tcap_OBJECT_IDENTIFIER(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_object_identifier(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                            NULL);

  return offset;
}
static int dissect_oid(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_OBJECT_IDENTIFIER(FALSE, tvb, offset, pinfo, tree, hf_tcap_oid);
}
static int dissect_globalValue(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_OBJECT_IDENTIFIER(FALSE, tvb, offset, pinfo, tree, hf_tcap_globalValue);
}
static int dissect_objectSecurityId_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_OBJECT_IDENTIFIER(TRUE, tvb, offset, pinfo, tree, hf_tcap_objectSecurityId);
}
static int dissect_objectConfidentialityId_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_OBJECT_IDENTIFIER(TRUE, tvb, offset, pinfo, tree, hf_tcap_objectConfidentialityId);
}


static int
dissect_tcap_Dialog1(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
tvbuff_t	*parameter_tvb;
tvbuff_t	*next_tvb;
guint8 class;
	gboolean pc;
	guint32 tag;
	guint32 len;
	guint32 ind_field;
	

offset = get_ber_identifier(tvb, offset, &class, &pc, &tag);
offset = get_ber_length(tree, tvb, offset, &len, &ind_field);
next_tvb = tvb_new_subset(tvb, offset, len, len);		
 offset = dissect_ber_octet_string(TRUE, pinfo, tree, tvb, 0, hf_index,
                                    &parameter_tvb);
                                    
 /*offset = dissect_ber_octet_string(TRUE, pinfo, tree, tvb, offset, hf_index,
  *                                 &parameter_tvb);
  */
 if (!parameter_tvb)
	return offset;
 dissect_tcap_DialoguePDU(TRUE, parameter_tvb, 0, pinfo, tree, -1);


  return offset;
}
static int dissect_dialog_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_Dialog1(TRUE, tvb, offset, pinfo, tree, hf_tcap_dialog);
}

static const ber_sequence_t ExternalPDU_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_OID, BER_FLAGS_NOOWNTAG, dissect_oid },
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_dialog_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_tcap_ExternalPDU(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                ExternalPDU_sequence, hf_index, ett_tcap_ExternalPDU);

  return offset;
}



static int
dissect_tcap_UserInfoOID(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
	static char buffer[128];
	tcapext_oid = buffer;
	pinfo->private_data = buffer;
  offset = dissect_ber_object_identifier(FALSE, pinfo, tree, tvb, offset,
                                         hf_index, tcapext_oid);


  return offset;
}
static int dissect_useroid(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_UserInfoOID(FALSE, tvb, offset, pinfo, tree, hf_tcap_useroid);
}


static int
dissect_tcap_ExternUserInfo(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
tvbuff_t	*next_tvb;
guint8 class;
	gboolean pc;
	guint32 tag;
	guint32 len, start_offset;
	guint32 ind_field;
/* 
 * ok lets look at the oid and ssn and try and find a dissector, otherwise lets decode it.
 */
ber_oid_dissector_table = find_dissector_table("ber.oid");
start_offset = offset;
offset = get_ber_identifier(tvb, offset, &class, &pc, &tag);
offset = get_ber_length(tree, tvb, offset, &len, &ind_field);
/* Use the recived length, XXX What if it was indefenet? */
next_tvb = tvb_new_subset(tvb, start_offset, len +(offset - start_offset), len+(offset - start_offset));		
if (ber_oid_dissector_table && tcapext_oid){
if(!dissector_try_string(ber_oid_dissector_table, tcapext_oid, next_tvb, pinfo, tcap_top_tree))	
	{
	}
	}
offset+=len;



  return offset;
}
static int dissect_externuserinfo_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_ExternUserInfo(TRUE, tvb, offset, pinfo, tree, hf_tcap_externuserinfo);
}

static const ber_sequence_t UserInformation_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_OID, BER_FLAGS_NOOWNTAG, dissect_useroid },
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_externuserinfo_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_tcap_UserInformation(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                UserInformation_sequence, hf_index, ett_tcap_UserInformation);

  return offset;
}
static int dissect_userInformation(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_UserInformation(FALSE, tvb, offset, pinfo, tree, hf_tcap_userInformation);
}


static const value_string tcap_Release_request_reason_vals[] = {
  {   0, "normal" },
  {   1, "urgent" },
  {  30, "user-defined" },
  { 0, NULL }
};


static int
dissect_tcap_Release_request_reason(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_reasonrq_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_Release_request_reason(TRUE, tvb, offset, pinfo, tree, hf_tcap_reasonrq);
}

static const ber_sequence_t RLRQ_apdu_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_reasonrq_impl },
  { BER_CLASS_CON, 30, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_user_information_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_tcap_RLRQ_apdu(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                RLRQ_apdu_sequence, hf_index, ett_tcap_RLRQ_apdu);

  return offset;
}


static const value_string tcap_Release_response_reason_vals[] = {
  {   0, "normal" },
  {   1, "not-finished" },
  {  30, "user-defined" },
  { 0, NULL }
};


static int
dissect_tcap_Release_response_reason(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_reasonre_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_Release_response_reason(TRUE, tvb, offset, pinfo, tree, hf_tcap_reasonre);
}

static const ber_sequence_t RLRE_apdu_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_reasonre_impl },
  { BER_CLASS_CON, 30, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_user_information_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_tcap_RLRE_apdu(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                RLRE_apdu_sequence, hf_index, ett_tcap_RLRE_apdu);

  return offset;
}

static const asn_namedbit T_protocol_version3_bits[] = {
  {  0, &hf_tcap_T_protocol_version3_version1, -1, -1, "version1", NULL },
  { 0, NULL, 0, 0, NULL, NULL }
};

static int
dissect_tcap_T_protocol_version3(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_bitstring(implicit_tag, pinfo, tree, tvb, offset,
                                 T_protocol_version3_bits, hf_index, ett_tcap_T_protocol_version3,
                                 NULL);

  return offset;
}
static int dissect_protocol_version3_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_T_protocol_version3(TRUE, tvb, offset, pinfo, tree, hf_tcap_protocol_version3);
}

static const ber_sequence_t AUDT_apdu_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_protocol_version3_impl },
  { BER_CLASS_CON, 1, 0, dissect_application_context_name },
  { BER_CLASS_CON, 30, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_user_information_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_tcap_AUDT_apdu(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                AUDT_apdu_sequence, hf_index, ett_tcap_AUDT_apdu);

  return offset;
}
static int dissect_unidialoguePDU(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_AUDT_apdu(FALSE, tvb, offset, pinfo, tree, hf_tcap_unidialoguePDU);
}


static const value_string tcap_UniDialoguePDU_vals[] = {
  {   0, "unidialoguePDU" },
  { 0, NULL }
};

static const ber_choice_t UniDialoguePDU_choice[] = {
  {   0, BER_CLASS_APP, 0, BER_FLAGS_NOOWNTAG, dissect_unidialoguePDU },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_tcap_UniDialoguePDU(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              UniDialoguePDU_choice, hf_index, ett_tcap_UniDialoguePDU);

  return offset;
}


static int
dissect_tcap_DialogueOC(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
tvbuff_t	*parameter_tvb;
tvbuff_t	*next_tvb;
guint8 class;
	gboolean pc;
	guint32 tag;
	guint32 len;
	guint32 ind_field;
	

offset = get_ber_identifier(tvb, offset, &class, &pc, &tag);
offset = get_ber_length(tree, tvb, offset, &len, &ind_field);
next_tvb = tvb_new_subset(tvb, offset, len, len);		
 offset = dissect_ber_octet_string(TRUE, pinfo, tree, next_tvb, 0, hf_index,
                                    &parameter_tvb);
                                    
 
 if (!parameter_tvb)
	return offset;
 dissect_tcap_ExternalPDU(TRUE, parameter_tvb, 2, pinfo, tree, -1);

return offset+2;


  return offset;
}


static int
dissect_tcap_DialoguePortion(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_tcap_DialogueOC(implicit_tag, tvb, offset, pinfo, tree, hf_index);

  return offset;
}
static int dissect_dialoguePortion(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_DialoguePortion(FALSE, tvb, offset, pinfo, tree, hf_tcap_dialoguePortion);
}
static int dissect_u_abortCause(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_DialoguePortion(FALSE, tvb, offset, pinfo, tree, hf_tcap_u_abortCause);
}



static int
dissect_tcap_InvokeIdType(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_invokeID(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_InvokeIdType(FALSE, tvb, offset, pinfo, tree, hf_tcap_invokeID);
}
static int dissect_linkedID_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_InvokeIdType(TRUE, tvb, offset, pinfo, tree, hf_tcap_linkedID);
}
static int dissect_derivable(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_InvokeIdType(FALSE, tvb, offset, pinfo, tree, hf_tcap_derivable);
}



static int
dissect_tcap_INTEGER(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_localValue(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_INTEGER(FALSE, tvb, offset, pinfo, tree, hf_tcap_localValue);
}
static int dissect_integerSecurityId_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_INTEGER(TRUE, tvb, offset, pinfo, tree, hf_tcap_integerSecurityId);
}
static int dissect_integerConfidentialityId_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_INTEGER(TRUE, tvb, offset, pinfo, tree, hf_tcap_integerConfidentialityId);
}
static int dissect_private_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_INTEGER(TRUE, tvb, offset, pinfo, tree, hf_tcap_private);
}
static int dissect_privateer_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_INTEGER(TRUE, tvb, offset, pinfo, tree, hf_tcap_privateer);
}


static const value_string tcap_OPERATION_vals[] = {
  {   0, "localValue" },
  {   1, "globalValue" },
  { 0, NULL }
};

static const ber_choice_t OPERATION_choice[] = {
  {   0, BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_localValue },
  {   1, BER_CLASS_UNI, BER_UNI_TAG_OID, BER_FLAGS_NOOWNTAG, dissect_globalValue },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_tcap_OPERATION(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              OPERATION_choice, hf_index, ett_tcap_OPERATION);

  return offset;
}
static int dissect_opCode(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_OPERATION(FALSE, tvb, offset, pinfo, tree, hf_tcap_opCode);
}



static int
dissect_tcap_Parameter(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
tvbuff_t	*next_tvb;
guint8 class;
	gboolean pc;
	guint32 tag;
	guint32 len;
	guint32 ind_field;


 offset = get_ber_identifier(tvb, offset, &class, &pc, &tag);
 offset = get_ber_length(tree, tvb, offset, &len, &ind_field);
 offset = dissect_ber_octet_string(TRUE, pinfo, tree, tvb, 0, hf_index,
                                    &next_tvb);
 
  dissect_tcap_param(pinfo,tree,next_tvb,0);
  return offset;


  return offset;
}
static int dissect_parameter(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_Parameter(FALSE, tvb, offset, pinfo, tree, hf_tcap_parameter);
}

static const ber_sequence_t Invoke_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_invokeID },
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_linkedID_impl },
  { BER_CLASS_UNI, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_opCode },
  { BER_CLASS_ANY, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_parameter },
  { 0, 0, 0, NULL }
};

static int
dissect_tcap_Invoke(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                Invoke_sequence, hf_index, ett_tcap_Invoke);

  return offset;
}
static int dissect_invoke_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_Invoke(TRUE, tvb, offset, pinfo, tree, hf_tcap_invoke);
}

static const ber_sequence_t T_resultretres_sequence[] = {
  { BER_CLASS_UNI, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_opCode },
  { BER_CLASS_ANY, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_parameter },
  { 0, 0, 0, NULL }
};

static int
dissect_tcap_T_resultretres(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                T_resultretres_sequence, hf_index, ett_tcap_T_resultretres);

  return offset;
}
static int dissect_resultretres(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_T_resultretres(FALSE, tvb, offset, pinfo, tree, hf_tcap_resultretres);
}

static const ber_sequence_t ReturnResult_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_invokeID },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_resultretres },
  { 0, 0, 0, NULL }
};

static int
dissect_tcap_ReturnResult(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                ReturnResult_sequence, hf_index, ett_tcap_ReturnResult);

  return offset;
}
static int dissect_returnResultLast_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_ReturnResult(TRUE, tvb, offset, pinfo, tree, hf_tcap_returnResultLast);
}
static int dissect_returnResultNotLast_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_ReturnResult(TRUE, tvb, offset, pinfo, tree, hf_tcap_returnResultNotLast);
}



static int
dissect_tcap_INTEGER_M32768_32767(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_national_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_INTEGER_M32768_32767(TRUE, tvb, offset, pinfo, tree, hf_tcap_national);
}
static int dissect_nationaler_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_INTEGER_M32768_32767(TRUE, tvb, offset, pinfo, tree, hf_tcap_nationaler);
}


static const value_string tcap_ErrorCode_vals[] = {
  {  19, "nationaler" },
  {  20, "privateer" },
  { 0, NULL }
};

static const ber_choice_t ErrorCode_choice[] = {
  {  19, BER_CLASS_PRI, 19, BER_FLAGS_IMPLTAG, dissect_nationaler_impl },
  {  20, BER_CLASS_PRI, 20, BER_FLAGS_IMPLTAG, dissect_privateer_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_tcap_ErrorCode(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              ErrorCode_choice, hf_index, ett_tcap_ErrorCode);

  return offset;
}
static int dissect_errorCode(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_ErrorCode(FALSE, tvb, offset, pinfo, tree, hf_tcap_errorCode);
}

static const ber_sequence_t ReturnError_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_invokeID },
  { BER_CLASS_PRI, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_errorCode },
  { BER_CLASS_ANY, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_parameter },
  { 0, 0, 0, NULL }
};

static int
dissect_tcap_ReturnError(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                ReturnError_sequence, hf_index, ett_tcap_ReturnError);

  return offset;
}
static int dissect_returnError_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_ReturnError(TRUE, tvb, offset, pinfo, tree, hf_tcap_returnError);
}



static int
dissect_tcap_NULL(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_null(implicit_tag, pinfo, tree, tvb, offset, hf_index);

  return offset;
}
static int dissect_not_derivable(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_NULL(FALSE, tvb, offset, pinfo, tree, hf_tcap_not_derivable);
}


static const value_string tcap_T_invokeIDRej_vals[] = {
  {   0, "derivable" },
  {   1, "not-derivable" },
  { 0, NULL }
};

static const ber_choice_t T_invokeIDRej_choice[] = {
  {   0, BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_derivable },
  {   1, BER_CLASS_UNI, BER_UNI_TAG_NULL, BER_FLAGS_NOOWNTAG, dissect_not_derivable },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_tcap_T_invokeIDRej(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              T_invokeIDRej_choice, hf_index, ett_tcap_T_invokeIDRej);

  return offset;
}
static int dissect_invokeIDRej(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_T_invokeIDRej(FALSE, tvb, offset, pinfo, tree, hf_tcap_invokeIDRej);
}


static const value_string tcap_GeneralProblem_vals[] = {
  {   0, "unrecognizedComponent" },
  {   1, "mistypedComponent" },
  {   2, "badlyStructuredComponent" },
  { 0, NULL }
};


static int
dissect_tcap_GeneralProblem(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_generalProblem_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_GeneralProblem(TRUE, tvb, offset, pinfo, tree, hf_tcap_generalProblem);
}


static const value_string tcap_InvokeProblem_vals[] = {
  {   0, "duplicateInvokeID" },
  {   1, "unrecognizedOperation" },
  {   2, "mistypedParameter" },
  {   3, "resourceLimitation" },
  {   4, "initiatingRelease" },
  {   5, "unrecognizedLinkedID" },
  {   6, "linkedResponseUnexpected" },
  {   7, "unexpectedLinkedOperation" },
  { 0, NULL }
};


static int
dissect_tcap_InvokeProblem(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_invokeProblem_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_InvokeProblem(TRUE, tvb, offset, pinfo, tree, hf_tcap_invokeProblem);
}


static const value_string tcap_ReturnResultProblem_vals[] = {
  {   0, "unrecognizedInvokeID" },
  {   1, "returnResultUnexpected" },
  {   2, "mistypedParameter" },
  { 0, NULL }
};


static int
dissect_tcap_ReturnResultProblem(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_returnResultProblem_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_ReturnResultProblem(TRUE, tvb, offset, pinfo, tree, hf_tcap_returnResultProblem);
}


static const value_string tcap_ReturnErrorProblem_vals[] = {
  {   0, "unrecognizedInvokeID" },
  {   1, "returnErrorUnexpected" },
  {   2, "unrecognizedError" },
  {   3, "unexpectedError" },
  {   4, "mistypedParameter" },
  { 0, NULL }
};


static int
dissect_tcap_ReturnErrorProblem(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_returnErrorProblem_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_ReturnErrorProblem(TRUE, tvb, offset, pinfo, tree, hf_tcap_returnErrorProblem);
}


static const value_string tcap_T_problem_vals[] = {
  {   0, "generalProblem" },
  {   1, "invokeProblem" },
  {   2, "returnResultProblem" },
  {   3, "returnErrorProblem" },
  { 0, NULL }
};

static const ber_choice_t T_problem_choice[] = {
  {   0, BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_generalProblem_impl },
  {   1, BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_invokeProblem_impl },
  {   2, BER_CLASS_CON, 2, BER_FLAGS_IMPLTAG, dissect_returnResultProblem_impl },
  {   3, BER_CLASS_CON, 3, BER_FLAGS_IMPLTAG, dissect_returnErrorProblem_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_tcap_T_problem(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              T_problem_choice, hf_index, ett_tcap_T_problem);

  return offset;
}
static int dissect_problem(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_T_problem(FALSE, tvb, offset, pinfo, tree, hf_tcap_problem);
}

static const ber_sequence_t Reject_sequence[] = {
  { BER_CLASS_UNI, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_invokeIDRej },
  { BER_CLASS_CON, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_problem },
  { 0, 0, 0, NULL }
};

static int
dissect_tcap_Reject(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                Reject_sequence, hf_index, ett_tcap_Reject);

  return offset;
}
static int dissect_reject_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_Reject(TRUE, tvb, offset, pinfo, tree, hf_tcap_reject);
}


static const value_string tcap_Component_vals[] = {
  {   1, "invoke" },
  {   2, "returnResultLast" },
  {   3, "returnError" },
  {   4, "reject" },
  {   7, "returnResultNotLast" },
  { 0, NULL }
};

static const ber_choice_t Component_choice[] = {
  {   1, BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_invoke_impl },
  {   2, BER_CLASS_CON, 2, BER_FLAGS_IMPLTAG, dissect_returnResultLast_impl },
  {   3, BER_CLASS_CON, 3, BER_FLAGS_IMPLTAG, dissect_returnError_impl },
  {   4, BER_CLASS_CON, 4, BER_FLAGS_IMPLTAG, dissect_reject_impl },
  {   7, BER_CLASS_CON, 7, BER_FLAGS_IMPLTAG, dissect_returnResultNotLast_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_tcap_Component(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
tvbuff_t	*next_tvb;
guint8 class;
	gboolean pc;
	guint32 tag;
	guint32 len;
	guint32 ind_field;
/* 
 * ok lets look at the oid and ssn and try and find a dissector, otherwise lets decode it.
 */
ber_oid_dissector_table = find_dissector_table("ber.oid");
tcap_itu_ssn_dissector_table = find_dissector_table("tcap.itu_ssn");

next_tvb = tvb_new_subset(tvb, offset, tvb_length_remaining(tvb, offset), tvb_length_remaining(tvb, offset));		
offset = get_ber_identifier(tvb, offset, &class, &pc, &tag);
offset = get_ber_length(tree, tvb, offset, &len, &ind_field);
if (ber_oid_dissector_table && cur_oid){
if(!dissector_try_string(ber_oid_dissector_table, cur_oid, next_tvb, pinfo, tcap_top_tree))	
	{
	if (!dissector_try_port(tcap_itu_ssn_dissector_table, pinfo->match_port, next_tvb,pinfo, tcap_top_tree))
			 	dissect_ber_choice(pinfo, tree, next_tvb, 0,
                        Component_choice, hf_index, ett_tcap_Component);
	}
}
else
	 if (!dissector_try_port(tcap_itu_ssn_dissector_table, pinfo->match_port, next_tvb, pinfo, tcap_top_tree))
			dissect_ber_choice(pinfo, tree, next_tvb, 0,
                        Component_choice, hf_index, ett_tcap_Component);

offset+=len;


  return offset;
}
static int dissect_ComponentPortion_item(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_Component(FALSE, tvb, offset, pinfo, tree, hf_tcap_ComponentPortion_item);
}

static const ber_sequence_t ComponentPortion_sequence_of[1] = {
  { BER_CLASS_CON, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_ComponentPortion_item },
};

static int
dissect_tcap_ComponentPortion(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, pinfo, tree, tvb, offset,
                                   ComponentPortion_sequence_of, hf_index, ett_tcap_ComponentPortion);

  return offset;
}
static int dissect_components(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_ComponentPortion(FALSE, tvb, offset, pinfo, tree, hf_tcap_components);
}

static const ber_sequence_t Unidirectional_sequence[] = {
  { BER_CLASS_APP, 11, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_dialoguePortion },
  { BER_CLASS_APP, 12, BER_FLAGS_NOOWNTAG, dissect_components },
  { 0, 0, 0, NULL }
};

static int
dissect_tcap_Unidirectional(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                Unidirectional_sequence, hf_index, ett_tcap_Unidirectional);

  return offset;
}
static int dissect_unidirectional_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_Unidirectional(TRUE, tvb, offset, pinfo, tree, hf_tcap_unidirectional);
}


static int
dissect_tcap_OrigTransactionID(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
tvbuff_t *parameter_tvb;
guint8 len, i;
offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    &parameter_tvb);
len = tvb_length_remaining(parameter_tvb, 0);
if ((len)&&(check_col(pinfo->cinfo, COL_INFO)))
	   {
	   	
	   	col_append_fstr(pinfo->cinfo, COL_INFO, "otid(");
	   	for(i=0;i<len;i++)
          col_append_fstr(pinfo->cinfo, COL_INFO, "%02x",tvb_get_guint8(parameter_tvb,i));
        col_append_fstr(pinfo->cinfo, COL_INFO, ") ");
		}


  return offset;
}
static int dissect_otid(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_OrigTransactionID(FALSE, tvb, offset, pinfo, tree, hf_tcap_otid);
}

static const ber_sequence_t Begin_sequence[] = {
  { BER_CLASS_APP, 8, BER_FLAGS_NOOWNTAG, dissect_otid },
  { BER_CLASS_APP, 11, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_dialoguePortion },
  { BER_CLASS_APP, 12, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_components },
  { 0, 0, 0, NULL }
};

static int
dissect_tcap_Begin(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
if (check_col(pinfo->cinfo, COL_INFO))
		col_append_fstr(pinfo->cinfo, COL_INFO, " Begin ");
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                Begin_sequence, hf_index, ett_tcap_Begin);

  return offset;
}
static int dissect_begin_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_Begin(TRUE, tvb, offset, pinfo, tree, hf_tcap_begin);
}


static int
dissect_tcap_DestTransactionID(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
tvbuff_t *parameter_tvb;
guint8 len , i;
offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    &parameter_tvb);
len = tvb_length_remaining(parameter_tvb, 0);
if ((len)&&(check_col(pinfo->cinfo, COL_INFO)))
	   {
	   	col_append_fstr(pinfo->cinfo, COL_INFO, "dtid(");
	   	for(i=0;i<len;i++)
          col_append_fstr(pinfo->cinfo, COL_INFO, "%02x",tvb_get_guint8(parameter_tvb,i));
        col_append_fstr(pinfo->cinfo, COL_INFO, ") ");
		}


  return offset;
}
static int dissect_dtid(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_DestTransactionID(FALSE, tvb, offset, pinfo, tree, hf_tcap_dtid);
}

static const ber_sequence_t End_sequence[] = {
  { BER_CLASS_APP, 9, BER_FLAGS_NOOWNTAG, dissect_dtid },
  { BER_CLASS_APP, 11, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_dialoguePortion },
  { BER_CLASS_APP, 12, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_components },
  { 0, 0, 0, NULL }
};

static int
dissect_tcap_End(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
if (check_col(pinfo->cinfo, COL_INFO))
		col_append_fstr(pinfo->cinfo, COL_INFO, " End ");
offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                End_sequence, hf_index, ett_tcap_End);

  return offset;
}
static int dissect_end_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_End(TRUE, tvb, offset, pinfo, tree, hf_tcap_end);
}

static const ber_sequence_t Continue_sequence[] = {
  { BER_CLASS_APP, 8, BER_FLAGS_NOOWNTAG, dissect_otid },
  { BER_CLASS_APP, 9, BER_FLAGS_NOOWNTAG, dissect_dtid },
  { BER_CLASS_APP, 11, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_dialoguePortion },
  { BER_CLASS_APP, 12, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_components },
  { 0, 0, 0, NULL }
};

static int
dissect_tcap_Continue(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
if (check_col(pinfo->cinfo, COL_INFO))
		col_append_fstr(pinfo->cinfo, COL_INFO, " Continue ");
offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                Continue_sequence, hf_index, ett_tcap_Continue);

  return offset;
}
static int dissect_continue_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_Continue(TRUE, tvb, offset, pinfo, tree, hf_tcap_continue);
}


static const value_string tcap_P_AbortCause_vals[] = {
  {   0, "unrecognizedMessageType" },
  {   1, "unrecognizedTransactionID" },
  {   2, "badlyFormattedTransactionPortion" },
  {   3, "incorrectTransactionPortion" },
  {   4, "resourceLimitation" },
  { 0, NULL }
};


static int
dissect_tcap_P_AbortCause(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_p_abortCause(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_P_AbortCause(FALSE, tvb, offset, pinfo, tree, hf_tcap_p_abortCause);
}


static const value_string tcap_Reason_vals[] = {
  {  10, "p-abortCause" },
  {  11, "u-abortCause" },
  { 0, NULL }
};

static const ber_choice_t Reason_choice[] = {
  {  10, BER_CLASS_APP, 10, BER_FLAGS_NOOWNTAG, dissect_p_abortCause },
  {  11, BER_CLASS_APP, 11, BER_FLAGS_NOOWNTAG, dissect_u_abortCause },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_tcap_Reason(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              Reason_choice, hf_index, ett_tcap_Reason);

  return offset;
}
static int dissect_reason(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_Reason(FALSE, tvb, offset, pinfo, tree, hf_tcap_reason);
}

static const ber_sequence_t Abort_sequence[] = {
  { BER_CLASS_APP, 9, BER_FLAGS_NOOWNTAG, dissect_dtid },
  { BER_CLASS_APP, -1/*choice*/, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_reason },
  { 0, 0, 0, NULL }
};

static int
dissect_tcap_Abort(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
if (check_col(pinfo->cinfo, COL_INFO))
		col_append_fstr(pinfo->cinfo, COL_INFO, " Abort ");
 offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                Abort_sequence, hf_index, ett_tcap_Abort); 

  return offset;
}
static int dissect_abort_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_Abort(TRUE, tvb, offset, pinfo, tree, hf_tcap_abort);
}


static int
dissect_tcap_TransactionID(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_identifier(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_TransactionID(FALSE, tvb, offset, pinfo, tree, hf_tcap_identifier);
}


static int
dissect_tcap_ProtocolVersion(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_version(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_ProtocolVersion(FALSE, tvb, offset, pinfo, tree, hf_tcap_version);
}



static int
dissect_tcap_IntegerApplicationContext(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_integerApplicationId(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_IntegerApplicationContext(FALSE, tvb, offset, pinfo, tree, hf_tcap_integerApplicationId);
}



static int
dissect_tcap_ObjectIDApplicationContext(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_object_identifier(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                            NULL);

  return offset;
}
static int dissect_objectApplicationId(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_ObjectIDApplicationContext(FALSE, tvb, offset, pinfo, tree, hf_tcap_objectApplicationId);
}


static const value_string tcap_T_applicationContext_vals[] = {
  {  27, "integerApplicationId" },
  {  28, "objectApplicationId" },
  { 0, NULL }
};

static const ber_choice_t T_applicationContext_choice[] = {
  {  27, BER_CLASS_PRI, 27, BER_FLAGS_NOOWNTAG, dissect_integerApplicationId },
  {  28, BER_CLASS_PRI, 28, BER_FLAGS_NOOWNTAG, dissect_objectApplicationId },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_tcap_T_applicationContext(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              T_applicationContext_choice, hf_index, ett_tcap_T_applicationContext);

  return offset;
}
static int dissect_applicationContext(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_T_applicationContext(FALSE, tvb, offset, pinfo, tree, hf_tcap_applicationContext);
}


static const value_string tcap_T_securityContext_vals[] = {
  {   0, "integerSecurityId" },
  {   1, "objectSecurityId" },
  { 0, NULL }
};

static const ber_choice_t T_securityContext_choice[] = {
  {   0, BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_integerSecurityId_impl },
  {   1, BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_objectSecurityId_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_tcap_T_securityContext(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              T_securityContext_choice, hf_index, ett_tcap_T_securityContext);

  return offset;
}
static int dissect_securityContext(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_T_securityContext(FALSE, tvb, offset, pinfo, tree, hf_tcap_securityContext);
}


static const value_string tcap_T_confidentialityId_vals[] = {
  {   0, "integerConfidentialityId" },
  {   1, "objectConfidentialityId" },
  { 0, NULL }
};

static const ber_choice_t T_confidentialityId_choice[] = {
  {   0, BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_integerConfidentialityId_impl },
  {   1, BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_objectConfidentialityId_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_tcap_T_confidentialityId(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              T_confidentialityId_choice, hf_index, ett_tcap_T_confidentialityId);

  return offset;
}
static int dissect_confidentialityId(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_T_confidentialityId(FALSE, tvb, offset, pinfo, tree, hf_tcap_confidentialityId);
}

static const ber_sequence_t Confidentiality_sequence[] = {
  { BER_CLASS_CON, -1/*choice*/, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_confidentialityId },
  { 0, 0, 0, NULL }
};

static int
dissect_tcap_Confidentiality(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                Confidentiality_sequence, hf_index, ett_tcap_Confidentiality);

  return offset;
}
static int dissect_confidentiality_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_Confidentiality(TRUE, tvb, offset, pinfo, tree, hf_tcap_confidentiality);
}

static const ber_sequence_t DialoguePortionANSI_sequence[] = {
  { BER_CLASS_PRI, 26, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_version },
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL, dissect_applicationContext },
  { BER_CLASS_UNI, 8, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_userInformation },
  { BER_CLASS_CON, -1/*choice*/, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_securityContext },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_confidentiality_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_tcap_DialoguePortionANSI(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                DialoguePortionANSI_sequence, hf_index, ett_tcap_DialoguePortionANSI);

  return offset;
}
static int dissect_dialoguePortionansi(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_DialoguePortionANSI(FALSE, tvb, offset, pinfo, tree, hf_tcap_dialoguePortionansi);
}


static int
dissect_tcap_OCTET_STRING_SIZE_0_2(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_componentIDs_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_OCTET_STRING_SIZE_0_2(TRUE, tvb, offset, pinfo, tree, hf_tcap_componentIDs);
}


static const value_string tcap_OperationCode_vals[] = {
  {  16, "national" },
  {  17, "private" },
  { 0, NULL }
};

static const ber_choice_t OperationCode_choice[] = {
  {  16, BER_CLASS_PRI, 16, BER_FLAGS_IMPLTAG, dissect_national_impl },
  {  17, BER_CLASS_PRI, 17, BER_FLAGS_IMPLTAG, dissect_private_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_tcap_OperationCode(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              OperationCode_choice, hf_index, ett_tcap_OperationCode);

  return offset;
}
static int dissect_operationCode(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_OperationCode(FALSE, tvb, offset, pinfo, tree, hf_tcap_operationCode);
}



static int
dissect_tcap_ANSIParameters(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
/* we are doing the ParamSet here so need to look at the tags*/
	guint32 len;
len = tvb_length_remaining(tvb, offset);
if (len > 2)  /* arghhh I dont know whether this is constructed or not! */
		offset = dissect_tcap_param(pinfo,tree,tvb,offset);
else
offset = dissect_ber_octet_string(TRUE, pinfo, tree, tvb, 0, hf_index,
                                    NULL);



  return offset;
}
static int dissect_ansiparams(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_ANSIParameters(FALSE, tvb, offset, pinfo, tree, hf_tcap_ansiparams);
}
static int dissect_ansiparams1(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_ANSIParameters(FALSE, tvb, offset, pinfo, tree, hf_tcap_ansiparams1);
}
static int dissect_ansiparams2(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_ANSIParameters(FALSE, tvb, offset, pinfo, tree, hf_tcap_ansiparams2);
}
static int dissect_ansiparams3(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_ANSIParameters(FALSE, tvb, offset, pinfo, tree, hf_tcap_ansiparams3);
}
static int dissect_ansiparams4(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_ANSIParameters(FALSE, tvb, offset, pinfo, tree, hf_tcap_ansiparams4);
}
static int dissect_ansiparams5(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_ANSIParameters(FALSE, tvb, offset, pinfo, tree, hf_tcap_ansiparams5);
}
static int dissect_ansiparams6(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_ANSIParameters(FALSE, tvb, offset, pinfo, tree, hf_tcap_ansiparams6);
}
static int dissect_ansiparams7(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_ANSIParameters(FALSE, tvb, offset, pinfo, tree, hf_tcap_ansiparams7);
}
static int dissect_ansiparams8(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_ANSIParameters(FALSE, tvb, offset, pinfo, tree, hf_tcap_ansiparams8);
}
static int dissect_ansiparams9(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_ANSIParameters(FALSE, tvb, offset, pinfo, tree, hf_tcap_ansiparams9);
}
static int dissect_ansiparams10(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_ANSIParameters(FALSE, tvb, offset, pinfo, tree, hf_tcap_ansiparams10);
}
static int dissect_ansiparams11(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_ANSIParameters(FALSE, tvb, offset, pinfo, tree, hf_tcap_ansiparams11);
}
static int dissect_ansiparams12(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_ANSIParameters(FALSE, tvb, offset, pinfo, tree, hf_tcap_ansiparams12);
}
static int dissect_ansiparams13(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_ANSIParameters(FALSE, tvb, offset, pinfo, tree, hf_tcap_ansiparams13);
}
static int dissect_ansiparams14(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_ANSIParameters(FALSE, tvb, offset, pinfo, tree, hf_tcap_ansiparams14);
}
static int dissect_ansiparams15(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_ANSIParameters(FALSE, tvb, offset, pinfo, tree, hf_tcap_ansiparams15);
}
static int dissect_ansiparams16(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_ANSIParameters(FALSE, tvb, offset, pinfo, tree, hf_tcap_ansiparams16);
}
static int dissect_ansiparams17(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_ANSIParameters(FALSE, tvb, offset, pinfo, tree, hf_tcap_ansiparams17);
}
static int dissect_ansiparams18(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_ANSIParameters(FALSE, tvb, offset, pinfo, tree, hf_tcap_ansiparams18);
}
static int dissect_ansiparams19(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_ANSIParameters(FALSE, tvb, offset, pinfo, tree, hf_tcap_ansiparams19);
}
static int dissect_ansiparams20(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_ANSIParameters(FALSE, tvb, offset, pinfo, tree, hf_tcap_ansiparams20);
}
static int dissect_ansiparams21(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_ANSIParameters(FALSE, tvb, offset, pinfo, tree, hf_tcap_ansiparams21);
}

static const ber_sequence_t ANSIparamch_sequence[] = {
  { BER_CLASS_ANY, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_ansiparams },
  { BER_CLASS_ANY, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_ansiparams1 },
  { BER_CLASS_ANY, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_ansiparams2 },
  { BER_CLASS_ANY, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_ansiparams3 },
  { BER_CLASS_ANY, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_ansiparams4 },
  { BER_CLASS_ANY, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_ansiparams5 },
  { BER_CLASS_ANY, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_ansiparams6 },
  { BER_CLASS_ANY, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_ansiparams7 },
  { BER_CLASS_ANY, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_ansiparams8 },
  { BER_CLASS_ANY, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_ansiparams9 },
  { BER_CLASS_ANY, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_ansiparams10 },
  { BER_CLASS_ANY, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_ansiparams11 },
  { BER_CLASS_ANY, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_ansiparams12 },
  { BER_CLASS_ANY, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_ansiparams13 },
  { BER_CLASS_ANY, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_ansiparams14 },
  { BER_CLASS_ANY, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_ansiparams15 },
  { BER_CLASS_ANY, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_ansiparams16 },
  { BER_CLASS_ANY, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_ansiparams17 },
  { BER_CLASS_ANY, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_ansiparams18 },
  { BER_CLASS_ANY, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_ansiparams19 },
  { BER_CLASS_ANY, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_ansiparams20 },
  { BER_CLASS_ANY, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_ansiparams21 },
  { 0, 0, 0, NULL }
};

static int
dissect_tcap_ANSIparamch(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                ANSIparamch_sequence, hf_index, ett_tcap_ANSIparamch);

  return offset;
}
static int dissect_parameterinv(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_ANSIparamch(FALSE, tvb, offset, pinfo, tree, hf_tcap_parameterinv);
}
static int dissect_parameterrr(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_ANSIparamch(FALSE, tvb, offset, pinfo, tree, hf_tcap_parameterrr);
}
static int dissect_parameterre(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_ANSIparamch(FALSE, tvb, offset, pinfo, tree, hf_tcap_parameterre);
}
static int dissect_parameterrj(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_ANSIparamch(FALSE, tvb, offset, pinfo, tree, hf_tcap_parameterrj);
}

static const ber_sequence_t InvokePDU_sequence[] = {
  { BER_CLASS_PRI, 15, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_componentIDs_impl },
  { BER_CLASS_PRI, -1/*choice*/, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_operationCode },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_parameterinv },
  { 0, 0, 0, NULL }
};

static int
dissect_tcap_InvokePDU(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                InvokePDU_sequence, hf_index, ett_tcap_InvokePDU);

  return offset;
}
static int dissect_invokeLastansi_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_InvokePDU(TRUE, tvb, offset, pinfo, tree, hf_tcap_invokeLastansi);
}
static int dissect_invokeNotLastansi_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_InvokePDU(TRUE, tvb, offset, pinfo, tree, hf_tcap_invokeNotLastansi);
}


static int
dissect_tcap_ComponentID(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_componentID(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_ComponentID(FALSE, tvb, offset, pinfo, tree, hf_tcap_componentID);
}

static const ber_sequence_t ReturnResultPDU_sequence[] = {
  { BER_CLASS_PRI, 15, BER_FLAGS_NOOWNTAG, dissect_componentID },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_parameterrr },
  { 0, 0, 0, NULL }
};

static int
dissect_tcap_ReturnResultPDU(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                ReturnResultPDU_sequence, hf_index, ett_tcap_ReturnResultPDU);

  return offset;
}
static int dissect_returnResultLastansi_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_ReturnResultPDU(TRUE, tvb, offset, pinfo, tree, hf_tcap_returnResultLastansi);
}
static int dissect_returnResultNotLastansi_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_ReturnResultPDU(TRUE, tvb, offset, pinfo, tree, hf_tcap_returnResultNotLastansi);
}

static const ber_sequence_t ReturnErrorPDU_sequence[] = {
  { BER_CLASS_PRI, 15, BER_FLAGS_NOOWNTAG, dissect_componentID },
  { BER_CLASS_PRI, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_errorCode },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_parameterre },
  { 0, 0, 0, NULL }
};

static int
dissect_tcap_ReturnErrorPDU(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                ReturnErrorPDU_sequence, hf_index, ett_tcap_ReturnErrorPDU);

  return offset;
}
static int dissect_returnErroransi_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_ReturnErrorPDU(TRUE, tvb, offset, pinfo, tree, hf_tcap_returnErroransi);
}


static const value_string tcap_ProblemPDU_vals[] = {
  { 257, "general-unrecognisedComponentType" },
  { 258, "general-incorrectComponentPortion" },
  { 259, "general-badlyStructuredCompPortion" },
  { 0, NULL }
};


static int
dissect_tcap_ProblemPDU(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_rejectProblem_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_ProblemPDU(TRUE, tvb, offset, pinfo, tree, hf_tcap_rejectProblem);
}

static const ber_sequence_t RejectPDU_sequence[] = {
  { BER_CLASS_PRI, 15, BER_FLAGS_NOOWNTAG, dissect_componentID },
  { BER_CLASS_PRI, 21, BER_FLAGS_IMPLTAG, dissect_rejectProblem_impl },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_parameterrj },
  { 0, 0, 0, NULL }
};

static int
dissect_tcap_RejectPDU(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                RejectPDU_sequence, hf_index, ett_tcap_RejectPDU);

  return offset;
}
static int dissect_rejectansi_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_RejectPDU(TRUE, tvb, offset, pinfo, tree, hf_tcap_rejectansi);
}


static const value_string tcap_ComponentPDU_vals[] = {
  {   9, "invokeLastansi" },
  {  10, "returnResultLastansi" },
  {  11, "returnErroransi" },
  {  12, "rejectansi" },
  {  13, "invokeNotLastansi" },
  {  14, "returnResultNotLastansi" },
  { 0, NULL }
};

static const ber_choice_t ComponentPDU_choice[] = {
  {   9, BER_CLASS_PRI, 9, BER_FLAGS_IMPLTAG, dissect_invokeLastansi_impl },
  {  10, BER_CLASS_PRI, 10, BER_FLAGS_IMPLTAG, dissect_returnResultLastansi_impl },
  {  11, BER_CLASS_PRI, 11, BER_FLAGS_IMPLTAG, dissect_returnErroransi_impl },
  {  12, BER_CLASS_PRI, 12, BER_FLAGS_IMPLTAG, dissect_rejectansi_impl },
  {  13, BER_CLASS_PRI, 13, BER_FLAGS_IMPLTAG, dissect_invokeNotLastansi_impl },
  {  14, BER_CLASS_PRI, 14, BER_FLAGS_IMPLTAG, dissect_returnResultNotLastansi_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_tcap_ComponentPDU(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
tvbuff_t *next_tvb;

next_tvb = tvb_new_subset(tvb, offset, tvb_length_remaining(tvb, offset), tvb_length_remaining(tvb, offset));		

if (!dissector_try_port(tcap_ansi_ssn_dissector_table, pinfo->match_port, next_tvb, pinfo, tcap_top_tree))
    {
	offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              ComponentPDU_choice, hf_index, ett_tcap_ComponentPDU);
	}
	
	

  return offset;
}
static int dissect_ComponentSequence_item(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_ComponentPDU(FALSE, tvb, offset, pinfo, tree, hf_tcap_ComponentSequence_item);
}

static const ber_sequence_t ComponentSequence_sequence_of[1] = {
  { BER_CLASS_PRI, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_ComponentSequence_item },
};

static int
dissect_tcap_ComponentSequence(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, pinfo, tree, tvb, offset,
                                   ComponentSequence_sequence_of, hf_index, ett_tcap_ComponentSequence);

  return offset;
}
static int dissect_componentPortion(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_ComponentSequence(FALSE, tvb, offset, pinfo, tree, hf_tcap_componentPortion);
}

static const ber_sequence_t UniTransactionPDU_sequence[] = {
  { BER_CLASS_PRI, 7, BER_FLAGS_NOOWNTAG, dissect_identifier },
  { BER_CLASS_PRI, 25, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_dialoguePortionansi },
  { BER_CLASS_PRI, 8, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_componentPortion },
  { 0, 0, 0, NULL }
};

static int
dissect_tcap_UniTransactionPDU(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                UniTransactionPDU_sequence, hf_index, ett_tcap_UniTransactionPDU);

  return offset;
}
static int dissect_ansiunidirectional_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_UniTransactionPDU(TRUE, tvb, offset, pinfo, tree, hf_tcap_ansiunidirectional);
}

static const ber_sequence_t TransactionPDU_sequence[] = {
  { BER_CLASS_PRI, 7, BER_FLAGS_NOOWNTAG, dissect_identifier },
  { BER_CLASS_PRI, 25, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_dialoguePortionansi },
  { BER_CLASS_PRI, 8, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_componentPortion },
  { 0, 0, 0, NULL }
};

static int
dissect_tcap_TransactionPDU(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
if ((hf_index == hf_tcap_ansiqueryWithPerm)&&(check_col(pinfo->cinfo, COL_INFO)))
				col_append_fstr(pinfo->cinfo, COL_INFO, " QueryWithPerm");		
				
if ((hf_index == hf_tcap_ansiqueryWithoutPerm)&&(check_col(pinfo->cinfo, COL_INFO)))
				col_append_fstr(pinfo->cinfo, COL_INFO, " QueryWithOutPerm");		
				
if ((hf_index == hf_tcap_ansiresponse)&&(check_col(pinfo->cinfo, COL_INFO)))
				col_append_fstr(pinfo->cinfo, COL_INFO, " Response");		
				
if ((hf_index == hf_tcap_ansiconversationWithPerm)&&(check_col(pinfo->cinfo, COL_INFO)))
				col_append_fstr(pinfo->cinfo, COL_INFO, " ConversationWithPerm");		
				
if ((hf_index == hf_tcap_ansiconversationWithoutPerm)&&(check_col(pinfo->cinfo, COL_INFO)))
				col_append_fstr(pinfo->cinfo, COL_INFO, " ConversationWithoutPerm");		
				
 offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                TransactionPDU_sequence, hf_index, ett_tcap_TransactionPDU);  
          

  return offset;
}
static int dissect_ansiqueryWithPerm_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_TransactionPDU(TRUE, tvb, offset, pinfo, tree, hf_tcap_ansiqueryWithPerm);
}
static int dissect_ansiqueryWithoutPerm_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_TransactionPDU(TRUE, tvb, offset, pinfo, tree, hf_tcap_ansiqueryWithoutPerm);
}
static int dissect_ansiresponse_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_TransactionPDU(TRUE, tvb, offset, pinfo, tree, hf_tcap_ansiresponse);
}
static int dissect_ansiconversationWithPerm_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_TransactionPDU(TRUE, tvb, offset, pinfo, tree, hf_tcap_ansiconversationWithPerm);
}
static int dissect_ansiconversationWithoutPerm_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_TransactionPDU(TRUE, tvb, offset, pinfo, tree, hf_tcap_ansiconversationWithoutPerm);
}


static const value_string tcap_P_Abort_cause_vals[] = {
  {   1, "unrecognizedPackageType" },
  {   2, "incorrestTransactionPortion" },
  {   3, "badlyStructuredTransactionPortion" },
  {   4, "unassignedRespondingTransactionID" },
  {   5, "permissionToReleaseProblem" },
  {   6, "resourceUnavilable" },
  {   7, "unrecognizedDialoguePortionID" },
  {   8, "badlyStructuredDialoguePortion" },
  {   9, "missingDialoguePortion" },
  {  10, "inconsistentDialoguePortion" },
  { 0, NULL }
};


static int
dissect_tcap_P_Abort_cause(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_abortCause(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_P_Abort_cause(FALSE, tvb, offset, pinfo, tree, hf_tcap_abortCause);
}


static const value_string tcap_T_causeInformation_vals[] = {
  {   0, "abortCause" },
  {   1, "userInformation" },
  { 0, NULL }
};

static const ber_choice_t T_causeInformation_choice[] = {
  {   0, BER_CLASS_PRI, 23, BER_FLAGS_NOOWNTAG, dissect_abortCause },
  {   1, BER_CLASS_UNI, 8, BER_FLAGS_NOOWNTAG, dissect_userInformation },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_tcap_T_causeInformation(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              T_causeInformation_choice, hf_index, ett_tcap_T_causeInformation);

  return offset;
}
static int dissect_causeInformation(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_T_causeInformation(FALSE, tvb, offset, pinfo, tree, hf_tcap_causeInformation);
}

static const ber_sequence_t AbortPDU_sequence[] = {
  { BER_CLASS_PRI, 7, BER_FLAGS_NOOWNTAG, dissect_identifier },
  { BER_CLASS_PRI, 25, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_dialoguePortionansi },
  { -1/*choice*/ , -1/*choice*/, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_causeInformation },
  { 0, 0, 0, NULL }
};

static int
dissect_tcap_AbortPDU(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
if (check_col(pinfo->cinfo, COL_INFO))
		col_append_fstr(pinfo->cinfo, COL_INFO, " Abort ");
   offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                AbortPDU_sequence, hf_index, ett_tcap_AbortPDU);

  return offset;
}
static int dissect_ansiabort_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_tcap_AbortPDU(TRUE, tvb, offset, pinfo, tree, hf_tcap_ansiabort);
}


static const value_string tcap_MessageType_vals[] = {
  {   0, "unidirectional" },
  {   1, "begin" },
  {   2, "end" },
  {   3, "continue" },
  {   4, "abort" },
  {   5, "ansiunidirectional" },
  {   6, "ansiqueryWithPerm" },
  {   7, "ansiqueryWithoutPerm" },
  {   8, "ansiresponse" },
  {   9, "ansiconversationWithPerm" },
  {  10, "ansiconversationWithoutPerm" },
  {  11, "ansiabort" },
  { 0, NULL }
};

static const ber_choice_t MessageType_choice[] = {
  {   0, BER_CLASS_APP, 1, BER_FLAGS_IMPLTAG, dissect_unidirectional_impl },
  {   1, BER_CLASS_APP, 2, BER_FLAGS_IMPLTAG, dissect_begin_impl },
  {   2, BER_CLASS_APP, 4, BER_FLAGS_IMPLTAG, dissect_end_impl },
  {   3, BER_CLASS_APP, 5, BER_FLAGS_IMPLTAG, dissect_continue_impl },
  {   4, BER_CLASS_APP, 7, BER_FLAGS_IMPLTAG, dissect_abort_impl },
  {   5, BER_CLASS_PRI, 1, BER_FLAGS_IMPLTAG, dissect_ansiunidirectional_impl },
  {   6, BER_CLASS_PRI, 2, BER_FLAGS_IMPLTAG, dissect_ansiqueryWithPerm_impl },
  {   7, BER_CLASS_PRI, 3, BER_FLAGS_IMPLTAG, dissect_ansiqueryWithoutPerm_impl },
  {   8, BER_CLASS_PRI, 4, BER_FLAGS_IMPLTAG, dissect_ansiresponse_impl },
  {   9, BER_CLASS_PRI, 5, BER_FLAGS_IMPLTAG, dissect_ansiconversationWithPerm_impl },
  {  10, BER_CLASS_PRI, 6, BER_FLAGS_IMPLTAG, dissect_ansiconversationWithoutPerm_impl },
  {  11, BER_CLASS_PRI, 22, BER_FLAGS_IMPLTAG, dissect_ansiabort_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_tcap_MessageType(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              MessageType_choice, hf_index, ett_tcap_MessageType);

  return offset;
}


static const value_string tcap_ERROR_vals[] = {
  {   0, "localValue" },
  {   1, "globalValue" },
  { 0, NULL }
};

static const ber_choice_t ERROR_choice[] = {
  {   0, BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_localValue },
  {   1, BER_CLASS_UNI, BER_UNI_TAG_OID, BER_FLAGS_NOOWNTAG, dissect_globalValue },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_tcap_ERROR(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              ERROR_choice, hf_index, ett_tcap_ERROR);

  return offset;
}


/*--- End of included file: packet-tcap-fn.c ---*/




const value_string tcap_component_type_str[] = {
    { TCAP_COMP_INVOKE,		"Invoke" },
    { TCAP_COMP_RRL,		"Return Result(L)" },
    { TCAP_COMP_RE,			"Return Error" },
    { TCAP_COMP_REJECT,		"Reject" },
    { TCAP_COMP_RRN,		"Return Result(NL)" },
    { 0,			NULL } };


static void
dissect_tcap(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree)
{
    proto_item		*item=NULL;
    proto_tree		*tree=NULL;

	tcap_top_tree = parent_tree;
    if (check_col(pinfo->cinfo, COL_PROTOCOL))
    {
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "TCAP");
    }

    /* create display subtree for the protocol */
    if(parent_tree){
       item = proto_tree_add_item(parent_tree, proto_tcap, tvb, 0, -1, FALSE);
       tree = proto_item_add_subtree(item, ett_tcap);
    }
	cur_oid = NULL;
	tcapext_oid = NULL;
	pinfo->private_data = NULL;
    dissect_tcap_MessageType(FALSE, tvb, 0, pinfo, tree, -1);


}



/* Register the protocol with Ethereal */

void proto_reg_handoff_tcap(void);

/* this format is require because a script is used to build the C function
   that calls all the protocol registration.
*/
void
proto_register_tcap(void)
{

/* Setup list of header fields  See Section 1.6.1 for details*/
    static hf_register_info hf[] = {
	{ &hf_tcap_tag,
		{ "Tag",           "tcap.msgtype",
		FT_UINT8, BASE_HEX, NULL, 0,
		"", HFILL }
	},
	{ &hf_tcap_length,
		{ "Length", "tcap.len",
		FT_UINT8, BASE_HEX, NULL, 0,
		"", HFILL }
	},
	{ &hf_tcap_data,
		{ "Data", "tcap.data",
		FT_BYTES, BASE_HEX, NULL, 0,
		"", HFILL }
	},

/*--- Included file: packet-tcap-hfarr.c ---*/

    { &hf_tcap_dialogueRequest,
      { "dialogueRequest", "tcap.dialogueRequest",
        FT_NONE, BASE_NONE, NULL, 0,
        "DialoguePDU/dialogueRequest", HFILL }},
    { &hf_tcap_dialogueResponse,
      { "dialogueResponse", "tcap.dialogueResponse",
        FT_NONE, BASE_NONE, NULL, 0,
        "DialoguePDU/dialogueResponse", HFILL }},
    { &hf_tcap_dialogueAbort,
      { "dialogueAbort", "tcap.dialogueAbort",
        FT_NONE, BASE_NONE, NULL, 0,
        "DialoguePDU/dialogueAbort", HFILL }},
    { &hf_tcap_oid,
      { "oid", "tcap.oid",
        FT_STRING, BASE_NONE, NULL, 0,
        "ExternalPDU/oid", HFILL }},
    { &hf_tcap_dialog,
      { "dialog", "tcap.dialog",
        FT_BYTES, BASE_HEX, NULL, 0,
        "ExternalPDU/dialog", HFILL }},
    { &hf_tcap_useroid,
      { "useroid", "tcap.useroid",
        FT_STRING, BASE_NONE, NULL, 0,
        "UserInformation/useroid", HFILL }},
    { &hf_tcap_externuserinfo,
      { "externuserinfo", "tcap.externuserinfo",
        FT_BYTES, BASE_HEX, NULL, 0,
        "UserInformation/externuserinfo", HFILL }},
    { &hf_tcap_protocol_versionrq,
      { "protocol-versionrq", "tcap.protocol_versionrq",
        FT_BYTES, BASE_HEX, NULL, 0,
        "AARQ-apdu/protocol-versionrq", HFILL }},
    { &hf_tcap_application_context_name,
      { "application-context-name", "tcap.application_context_name",
        FT_STRING, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_tcap_user_information,
      { "user-information", "tcap.user_information",
        FT_BYTES, BASE_HEX, NULL, 0,
        "", HFILL }},
    { &hf_tcap_protocol_versionre,
      { "protocol-versionre", "tcap.protocol_versionre",
        FT_BYTES, BASE_HEX, NULL, 0,
        "AARE-apdu/protocol-versionre", HFILL }},
    { &hf_tcap_result,
      { "result", "tcap.result",
        FT_INT32, BASE_DEC, VALS(tcap_Associate_result_vals), 0,
        "AARE-apdu/result", HFILL }},
    { &hf_tcap_result_source_diagnostic,
      { "result-source-diagnostic", "tcap.result_source_diagnostic",
        FT_UINT32, BASE_DEC, VALS(tcap_Associate_source_diagnostic_vals), 0,
        "AARE-apdu/result-source-diagnostic", HFILL }},
    { &hf_tcap_reasonrq,
      { "reasonrq", "tcap.reasonrq",
        FT_INT32, BASE_DEC, VALS(tcap_Release_request_reason_vals), 0,
        "RLRQ-apdu/reasonrq", HFILL }},
    { &hf_tcap_reasonre,
      { "reasonre", "tcap.reasonre",
        FT_INT32, BASE_DEC, VALS(tcap_Release_response_reason_vals), 0,
        "RLRE-apdu/reasonre", HFILL }},
    { &hf_tcap_abort_source,
      { "abort-source", "tcap.abort_source",
        FT_INT32, BASE_DEC, VALS(tcap_ABRT_source_vals), 0,
        "ABRT-apdu/abort-source", HFILL }},
    { &hf_tcap_dialogue_service_user,
      { "dialogue-service-user", "tcap.dialogue_service_user",
        FT_INT32, BASE_DEC, VALS(tcap_T_dialogue_service_user_vals), 0,
        "Associate-source-diagnostic/dialogue-service-user", HFILL }},
    { &hf_tcap_dialogue_service_provider,
      { "dialogue-service-provider", "tcap.dialogue_service_provider",
        FT_INT32, BASE_DEC, VALS(tcap_T_dialogue_service_provider_vals), 0,
        "Associate-source-diagnostic/dialogue-service-provider", HFILL }},
    { &hf_tcap_unidialoguePDU,
      { "unidialoguePDU", "tcap.unidialoguePDU",
        FT_NONE, BASE_NONE, NULL, 0,
        "UniDialoguePDU/unidialoguePDU", HFILL }},
    { &hf_tcap_protocol_version3,
      { "protocol-version3", "tcap.protocol_version3",
        FT_BYTES, BASE_HEX, NULL, 0,
        "AUDT-apdu/protocol-version3", HFILL }},
    { &hf_tcap_unidirectional,
      { "unidirectional", "tcap.unidirectional",
        FT_NONE, BASE_NONE, NULL, 0,
        "MessageType/unidirectional", HFILL }},
    { &hf_tcap_begin,
      { "begin", "tcap.begin",
        FT_NONE, BASE_NONE, NULL, 0,
        "MessageType/begin", HFILL }},
    { &hf_tcap_end,
      { "end", "tcap.end",
        FT_NONE, BASE_NONE, NULL, 0,
        "MessageType/end", HFILL }},
    { &hf_tcap_continue,
      { "continue", "tcap.continue",
        FT_NONE, BASE_NONE, NULL, 0,
        "MessageType/continue", HFILL }},
    { &hf_tcap_abort,
      { "abort", "tcap.abort",
        FT_NONE, BASE_NONE, NULL, 0,
        "MessageType/abort", HFILL }},
    { &hf_tcap_ansiunidirectional,
      { "ansiunidirectional", "tcap.ansiunidirectional",
        FT_NONE, BASE_NONE, NULL, 0,
        "MessageType/ansiunidirectional", HFILL }},
    { &hf_tcap_ansiqueryWithPerm,
      { "ansiqueryWithPerm", "tcap.ansiqueryWithPerm",
        FT_NONE, BASE_NONE, NULL, 0,
        "MessageType/ansiqueryWithPerm", HFILL }},
    { &hf_tcap_ansiqueryWithoutPerm,
      { "ansiqueryWithoutPerm", "tcap.ansiqueryWithoutPerm",
        FT_NONE, BASE_NONE, NULL, 0,
        "MessageType/ansiqueryWithoutPerm", HFILL }},
    { &hf_tcap_ansiresponse,
      { "ansiresponse", "tcap.ansiresponse",
        FT_NONE, BASE_NONE, NULL, 0,
        "MessageType/ansiresponse", HFILL }},
    { &hf_tcap_ansiconversationWithPerm,
      { "ansiconversationWithPerm", "tcap.ansiconversationWithPerm",
        FT_NONE, BASE_NONE, NULL, 0,
        "MessageType/ansiconversationWithPerm", HFILL }},
    { &hf_tcap_ansiconversationWithoutPerm,
      { "ansiconversationWithoutPerm", "tcap.ansiconversationWithoutPerm",
        FT_NONE, BASE_NONE, NULL, 0,
        "MessageType/ansiconversationWithoutPerm", HFILL }},
    { &hf_tcap_ansiabort,
      { "ansiabort", "tcap.ansiabort",
        FT_NONE, BASE_NONE, NULL, 0,
        "MessageType/ansiabort", HFILL }},
    { &hf_tcap_dialoguePortion,
      { "dialoguePortion", "tcap.dialoguePortion",
        FT_BYTES, BASE_HEX, NULL, 0,
        "", HFILL }},
    { &hf_tcap_components,
      { "components", "tcap.components",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_tcap_otid,
      { "otid", "tcap.otid",
        FT_BYTES, BASE_HEX, NULL, 0,
        "", HFILL }},
    { &hf_tcap_dtid,
      { "dtid", "tcap.dtid",
        FT_BYTES, BASE_HEX, NULL, 0,
        "", HFILL }},
    { &hf_tcap_reason,
      { "reason", "tcap.reason",
        FT_UINT32, BASE_DEC, VALS(tcap_Reason_vals), 0,
        "Abort/reason", HFILL }},
    { &hf_tcap_p_abortCause,
      { "p-abortCause", "tcap.p_abortCause",
        FT_INT32, BASE_DEC, VALS(tcap_P_AbortCause_vals), 0,
        "Reason/p-abortCause", HFILL }},
    { &hf_tcap_u_abortCause,
      { "u-abortCause", "tcap.u_abortCause",
        FT_BYTES, BASE_HEX, NULL, 0,
        "Reason/u-abortCause", HFILL }},
    { &hf_tcap_ComponentPortion_item,
      { "Item", "tcap.ComponentPortion_item",
        FT_UINT32, BASE_DEC, VALS(tcap_Component_vals), 0,
        "ComponentPortion/_item", HFILL }},
    { &hf_tcap_invoke,
      { "invoke", "tcap.invoke",
        FT_NONE, BASE_NONE, NULL, 0,
        "Component/invoke", HFILL }},
    { &hf_tcap_returnResultLast,
      { "returnResultLast", "tcap.returnResultLast",
        FT_NONE, BASE_NONE, NULL, 0,
        "Component/returnResultLast", HFILL }},
    { &hf_tcap_returnError,
      { "returnError", "tcap.returnError",
        FT_NONE, BASE_NONE, NULL, 0,
        "Component/returnError", HFILL }},
    { &hf_tcap_reject,
      { "reject", "tcap.reject",
        FT_NONE, BASE_NONE, NULL, 0,
        "Component/reject", HFILL }},
    { &hf_tcap_returnResultNotLast,
      { "returnResultNotLast", "tcap.returnResultNotLast",
        FT_NONE, BASE_NONE, NULL, 0,
        "Component/returnResultNotLast", HFILL }},
    { &hf_tcap_invokeID,
      { "invokeID", "tcap.invokeID",
        FT_UINT32, BASE_DEC, NULL, 0,
        "", HFILL }},
    { &hf_tcap_linkedID,
      { "linkedID", "tcap.linkedID",
        FT_UINT32, BASE_DEC, NULL, 0,
        "Invoke/linkedID", HFILL }},
    { &hf_tcap_opCode,
      { "opCode", "tcap.opCode",
        FT_UINT32, BASE_DEC, VALS(tcap_OPERATION_vals), 0,
        "", HFILL }},
    { &hf_tcap_parameter,
      { "parameter", "tcap.parameter",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_tcap_resultretres,
      { "resultretres", "tcap.resultretres",
        FT_NONE, BASE_NONE, NULL, 0,
        "ReturnResult/resultretres", HFILL }},
    { &hf_tcap_errorCode,
      { "errorCode", "tcap.errorCode",
        FT_UINT32, BASE_DEC, VALS(tcap_ErrorCode_vals), 0,
        "", HFILL }},
    { &hf_tcap_invokeIDRej,
      { "invokeIDRej", "tcap.invokeIDRej",
        FT_UINT32, BASE_DEC, VALS(tcap_T_invokeIDRej_vals), 0,
        "Reject/invokeIDRej", HFILL }},
    { &hf_tcap_derivable,
      { "derivable", "tcap.derivable",
        FT_UINT32, BASE_DEC, NULL, 0,
        "Reject/invokeIDRej/derivable", HFILL }},
    { &hf_tcap_not_derivable,
      { "not-derivable", "tcap.not_derivable",
        FT_NONE, BASE_NONE, NULL, 0,
        "Reject/invokeIDRej/not-derivable", HFILL }},
    { &hf_tcap_problem,
      { "problem", "tcap.problem",
        FT_UINT32, BASE_DEC, VALS(tcap_T_problem_vals), 0,
        "Reject/problem", HFILL }},
    { &hf_tcap_generalProblem,
      { "generalProblem", "tcap.generalProblem",
        FT_INT32, BASE_DEC, VALS(tcap_GeneralProblem_vals), 0,
        "Reject/problem/generalProblem", HFILL }},
    { &hf_tcap_invokeProblem,
      { "invokeProblem", "tcap.invokeProblem",
        FT_INT32, BASE_DEC, VALS(tcap_InvokeProblem_vals), 0,
        "Reject/problem/invokeProblem", HFILL }},
    { &hf_tcap_returnResultProblem,
      { "returnResultProblem", "tcap.returnResultProblem",
        FT_INT32, BASE_DEC, VALS(tcap_ReturnResultProblem_vals), 0,
        "Reject/problem/returnResultProblem", HFILL }},
    { &hf_tcap_returnErrorProblem,
      { "returnErrorProblem", "tcap.returnErrorProblem",
        FT_INT32, BASE_DEC, VALS(tcap_ReturnErrorProblem_vals), 0,
        "Reject/problem/returnErrorProblem", HFILL }},
    { &hf_tcap_localValue,
      { "localValue", "tcap.localValue",
        FT_INT32, BASE_DEC, NULL, 0,
        "", HFILL }},
    { &hf_tcap_globalValue,
      { "globalValue", "tcap.globalValue",
        FT_STRING, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_tcap_identifier,
      { "identifier", "tcap.identifier",
        FT_BYTES, BASE_HEX, NULL, 0,
        "", HFILL }},
    { &hf_tcap_dialoguePortionansi,
      { "dialoguePortionansi", "tcap.dialoguePortionansi",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_tcap_componentPortion,
      { "componentPortion", "tcap.componentPortion",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_tcap_causeInformation,
      { "causeInformation", "tcap.causeInformation",
        FT_UINT32, BASE_DEC, VALS(tcap_T_causeInformation_vals), 0,
        "AbortPDU/causeInformation", HFILL }},
    { &hf_tcap_abortCause,
      { "abortCause", "tcap.abortCause",
        FT_INT32, BASE_DEC, VALS(tcap_P_Abort_cause_vals), 0,
        "AbortPDU/causeInformation/abortCause", HFILL }},
    { &hf_tcap_userInformation,
      { "userInformation", "tcap.userInformation",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_tcap_version,
      { "version", "tcap.version",
        FT_BYTES, BASE_HEX, NULL, 0,
        "DialoguePortionANSI/version", HFILL }},
    { &hf_tcap_applicationContext,
      { "applicationContext", "tcap.applicationContext",
        FT_UINT32, BASE_DEC, VALS(tcap_T_applicationContext_vals), 0,
        "DialoguePortionANSI/applicationContext", HFILL }},
    { &hf_tcap_integerApplicationId,
      { "integerApplicationId", "tcap.integerApplicationId",
        FT_INT32, BASE_DEC, NULL, 0,
        "DialoguePortionANSI/applicationContext/integerApplicationId", HFILL }},
    { &hf_tcap_objectApplicationId,
      { "objectApplicationId", "tcap.objectApplicationId",
        FT_STRING, BASE_NONE, NULL, 0,
        "DialoguePortionANSI/applicationContext/objectApplicationId", HFILL }},
    { &hf_tcap_securityContext,
      { "securityContext", "tcap.securityContext",
        FT_UINT32, BASE_DEC, VALS(tcap_T_securityContext_vals), 0,
        "DialoguePortionANSI/securityContext", HFILL }},
    { &hf_tcap_integerSecurityId,
      { "integerSecurityId", "tcap.integerSecurityId",
        FT_INT32, BASE_DEC, NULL, 0,
        "DialoguePortionANSI/securityContext/integerSecurityId", HFILL }},
    { &hf_tcap_objectSecurityId,
      { "objectSecurityId", "tcap.objectSecurityId",
        FT_STRING, BASE_NONE, NULL, 0,
        "DialoguePortionANSI/securityContext/objectSecurityId", HFILL }},
    { &hf_tcap_confidentiality,
      { "confidentiality", "tcap.confidentiality",
        FT_NONE, BASE_NONE, NULL, 0,
        "DialoguePortionANSI/confidentiality", HFILL }},
    { &hf_tcap_confidentialityId,
      { "confidentialityId", "tcap.confidentialityId",
        FT_UINT32, BASE_DEC, VALS(tcap_T_confidentialityId_vals), 0,
        "Confidentiality/confidentialityId", HFILL }},
    { &hf_tcap_integerConfidentialityId,
      { "integerConfidentialityId", "tcap.integerConfidentialityId",
        FT_INT32, BASE_DEC, NULL, 0,
        "Confidentiality/confidentialityId/integerConfidentialityId", HFILL }},
    { &hf_tcap_objectConfidentialityId,
      { "objectConfidentialityId", "tcap.objectConfidentialityId",
        FT_STRING, BASE_NONE, NULL, 0,
        "Confidentiality/confidentialityId/objectConfidentialityId", HFILL }},
    { &hf_tcap_ComponentSequence_item,
      { "Item", "tcap.ComponentSequence_item",
        FT_UINT32, BASE_DEC, VALS(tcap_ComponentPDU_vals), 0,
        "ComponentSequence/_item", HFILL }},
    { &hf_tcap_invokeLastansi,
      { "invokeLastansi", "tcap.invokeLastansi",
        FT_NONE, BASE_NONE, NULL, 0,
        "ComponentPDU/invokeLastansi", HFILL }},
    { &hf_tcap_returnResultLastansi,
      { "returnResultLastansi", "tcap.returnResultLastansi",
        FT_NONE, BASE_NONE, NULL, 0,
        "ComponentPDU/returnResultLastansi", HFILL }},
    { &hf_tcap_returnErroransi,
      { "returnErroransi", "tcap.returnErroransi",
        FT_NONE, BASE_NONE, NULL, 0,
        "ComponentPDU/returnErroransi", HFILL }},
    { &hf_tcap_rejectansi,
      { "rejectansi", "tcap.rejectansi",
        FT_NONE, BASE_NONE, NULL, 0,
        "ComponentPDU/rejectansi", HFILL }},
    { &hf_tcap_invokeNotLastansi,
      { "invokeNotLastansi", "tcap.invokeNotLastansi",
        FT_NONE, BASE_NONE, NULL, 0,
        "ComponentPDU/invokeNotLastansi", HFILL }},
    { &hf_tcap_returnResultNotLastansi,
      { "returnResultNotLastansi", "tcap.returnResultNotLastansi",
        FT_NONE, BASE_NONE, NULL, 0,
        "ComponentPDU/returnResultNotLastansi", HFILL }},
    { &hf_tcap_componentIDs,
      { "componentIDs", "tcap.componentIDs",
        FT_BYTES, BASE_HEX, NULL, 0,
        "InvokePDU/componentIDs", HFILL }},
    { &hf_tcap_operationCode,
      { "operationCode", "tcap.operationCode",
        FT_UINT32, BASE_DEC, VALS(tcap_OperationCode_vals), 0,
        "InvokePDU/operationCode", HFILL }},
    { &hf_tcap_parameterinv,
      { "parameterinv", "tcap.parameterinv",
        FT_NONE, BASE_NONE, NULL, 0,
        "InvokePDU/parameterinv", HFILL }},
    { &hf_tcap_ansiparams,
      { "ansiparams", "tcap.ansiparams",
        FT_NONE, BASE_NONE, NULL, 0,
        "ANSIparamch/ansiparams", HFILL }},
    { &hf_tcap_ansiparams1,
      { "ansiparams1", "tcap.ansiparams1",
        FT_NONE, BASE_NONE, NULL, 0,
        "ANSIparamch/ansiparams1", HFILL }},
    { &hf_tcap_ansiparams2,
      { "ansiparams2", "tcap.ansiparams2",
        FT_NONE, BASE_NONE, NULL, 0,
        "ANSIparamch/ansiparams2", HFILL }},
    { &hf_tcap_ansiparams3,
      { "ansiparams3", "tcap.ansiparams3",
        FT_NONE, BASE_NONE, NULL, 0,
        "ANSIparamch/ansiparams3", HFILL }},
    { &hf_tcap_ansiparams4,
      { "ansiparams4", "tcap.ansiparams4",
        FT_NONE, BASE_NONE, NULL, 0,
        "ANSIparamch/ansiparams4", HFILL }},
    { &hf_tcap_ansiparams5,
      { "ansiparams5", "tcap.ansiparams5",
        FT_NONE, BASE_NONE, NULL, 0,
        "ANSIparamch/ansiparams5", HFILL }},
    { &hf_tcap_ansiparams6,
      { "ansiparams6", "tcap.ansiparams6",
        FT_NONE, BASE_NONE, NULL, 0,
        "ANSIparamch/ansiparams6", HFILL }},
    { &hf_tcap_ansiparams7,
      { "ansiparams7", "tcap.ansiparams7",
        FT_NONE, BASE_NONE, NULL, 0,
        "ANSIparamch/ansiparams7", HFILL }},
    { &hf_tcap_ansiparams8,
      { "ansiparams8", "tcap.ansiparams8",
        FT_NONE, BASE_NONE, NULL, 0,
        "ANSIparamch/ansiparams8", HFILL }},
    { &hf_tcap_ansiparams9,
      { "ansiparams9", "tcap.ansiparams9",
        FT_NONE, BASE_NONE, NULL, 0,
        "ANSIparamch/ansiparams9", HFILL }},
    { &hf_tcap_ansiparams10,
      { "ansiparams10", "tcap.ansiparams10",
        FT_NONE, BASE_NONE, NULL, 0,
        "ANSIparamch/ansiparams10", HFILL }},
    { &hf_tcap_ansiparams11,
      { "ansiparams11", "tcap.ansiparams11",
        FT_NONE, BASE_NONE, NULL, 0,
        "ANSIparamch/ansiparams11", HFILL }},
    { &hf_tcap_ansiparams12,
      { "ansiparams12", "tcap.ansiparams12",
        FT_NONE, BASE_NONE, NULL, 0,
        "ANSIparamch/ansiparams12", HFILL }},
    { &hf_tcap_ansiparams13,
      { "ansiparams13", "tcap.ansiparams13",
        FT_NONE, BASE_NONE, NULL, 0,
        "ANSIparamch/ansiparams13", HFILL }},
    { &hf_tcap_ansiparams14,
      { "ansiparams14", "tcap.ansiparams14",
        FT_NONE, BASE_NONE, NULL, 0,
        "ANSIparamch/ansiparams14", HFILL }},
    { &hf_tcap_ansiparams15,
      { "ansiparams15", "tcap.ansiparams15",
        FT_NONE, BASE_NONE, NULL, 0,
        "ANSIparamch/ansiparams15", HFILL }},
    { &hf_tcap_ansiparams16,
      { "ansiparams16", "tcap.ansiparams16",
        FT_NONE, BASE_NONE, NULL, 0,
        "ANSIparamch/ansiparams16", HFILL }},
    { &hf_tcap_ansiparams17,
      { "ansiparams17", "tcap.ansiparams17",
        FT_NONE, BASE_NONE, NULL, 0,
        "ANSIparamch/ansiparams17", HFILL }},
    { &hf_tcap_ansiparams18,
      { "ansiparams18", "tcap.ansiparams18",
        FT_NONE, BASE_NONE, NULL, 0,
        "ANSIparamch/ansiparams18", HFILL }},
    { &hf_tcap_ansiparams19,
      { "ansiparams19", "tcap.ansiparams19",
        FT_NONE, BASE_NONE, NULL, 0,
        "ANSIparamch/ansiparams19", HFILL }},
    { &hf_tcap_ansiparams20,
      { "ansiparams20", "tcap.ansiparams20",
        FT_NONE, BASE_NONE, NULL, 0,
        "ANSIparamch/ansiparams20", HFILL }},
    { &hf_tcap_ansiparams21,
      { "ansiparams21", "tcap.ansiparams21",
        FT_NONE, BASE_NONE, NULL, 0,
        "ANSIparamch/ansiparams21", HFILL }},
    { &hf_tcap_componentID,
      { "componentID", "tcap.componentID",
        FT_BYTES, BASE_HEX, NULL, 0,
        "", HFILL }},
    { &hf_tcap_parameterrr,
      { "parameterrr", "tcap.parameterrr",
        FT_NONE, BASE_NONE, NULL, 0,
        "ReturnResultPDU/parameterrr", HFILL }},
    { &hf_tcap_parameterre,
      { "parameterre", "tcap.parameterre",
        FT_NONE, BASE_NONE, NULL, 0,
        "ReturnErrorPDU/parameterre", HFILL }},
    { &hf_tcap_rejectProblem,
      { "rejectProblem", "tcap.rejectProblem",
        FT_INT32, BASE_DEC, VALS(tcap_ProblemPDU_vals), 0,
        "RejectPDU/rejectProblem", HFILL }},
    { &hf_tcap_parameterrj,
      { "parameterrj", "tcap.parameterrj",
        FT_NONE, BASE_NONE, NULL, 0,
        "RejectPDU/parameterrj", HFILL }},
    { &hf_tcap_national,
      { "national", "tcap.national",
        FT_UINT32, BASE_DEC, NULL, 0,
        "OperationCode/national", HFILL }},
    { &hf_tcap_private,
      { "private", "tcap.private",
        FT_INT32, BASE_DEC, NULL, 0,
        "OperationCode/private", HFILL }},
    { &hf_tcap_nationaler,
      { "nationaler", "tcap.nationaler",
        FT_UINT32, BASE_DEC, NULL, 0,
        "ErrorCode/nationaler", HFILL }},
    { &hf_tcap_privateer,
      { "privateer", "tcap.privateer",
        FT_INT32, BASE_DEC, NULL, 0,
        "ErrorCode/privateer", HFILL }},
    { &hf_tcap_T_protocol_versionrq_version1,
      { "version1", "tcap.version1",
        FT_BOOLEAN, 8, NULL, 0x80,
        "", HFILL }},
    { &hf_tcap_T_protocol_versionre_version1,
      { "version1", "tcap.version1",
        FT_BOOLEAN, 8, NULL, 0x80,
        "", HFILL }},
    { &hf_tcap_T_protocol_version3_version1,
      { "version1", "tcap.version1",
        FT_BOOLEAN, 8, NULL, 0x80,
        "", HFILL }},

/*--- End of included file: packet-tcap-hfarr.c ---*/

    };

/* Setup protocol subtree array */
    static gint *ett[] = {
	&ett_tcap,
	&ett_param,

/*--- Included file: packet-tcap-ettarr.c ---*/

    &ett_tcap_DialoguePDU,
    &ett_tcap_ExternalPDU,
    &ett_tcap_UserInformation,
    &ett_tcap_AARQ_apdu,
    &ett_tcap_T_protocol_versionrq,
    &ett_tcap_AARE_apdu,
    &ett_tcap_T_protocol_versionre,
    &ett_tcap_RLRQ_apdu,
    &ett_tcap_RLRE_apdu,
    &ett_tcap_ABRT_apdu,
    &ett_tcap_Associate_source_diagnostic,
    &ett_tcap_UniDialoguePDU,
    &ett_tcap_AUDT_apdu,
    &ett_tcap_T_protocol_version3,
    &ett_tcap_MessageType,
    &ett_tcap_Unidirectional,
    &ett_tcap_Begin,
    &ett_tcap_End,
    &ett_tcap_Continue,
    &ett_tcap_Abort,
    &ett_tcap_Reason,
    &ett_tcap_ComponentPortion,
    &ett_tcap_Component,
    &ett_tcap_Invoke,
    &ett_tcap_ReturnResult,
    &ett_tcap_T_resultretres,
    &ett_tcap_ReturnError,
    &ett_tcap_Reject,
    &ett_tcap_T_invokeIDRej,
    &ett_tcap_T_problem,
    &ett_tcap_OPERATION,
    &ett_tcap_ERROR,
    &ett_tcap_UniTransactionPDU,
    &ett_tcap_TransactionPDU,
    &ett_tcap_AbortPDU,
    &ett_tcap_T_causeInformation,
    &ett_tcap_DialoguePortionANSI,
    &ett_tcap_T_applicationContext,
    &ett_tcap_T_securityContext,
    &ett_tcap_Confidentiality,
    &ett_tcap_T_confidentialityId,
    &ett_tcap_ComponentSequence,
    &ett_tcap_ComponentPDU,
    &ett_tcap_InvokePDU,
    &ett_tcap_ANSIparamch,
    &ett_tcap_ReturnResultPDU,
    &ett_tcap_ReturnErrorPDU,
    &ett_tcap_RejectPDU,
    &ett_tcap_OperationCode,
    &ett_tcap_ErrorCode,

/*--- End of included file: packet-tcap-ettarr.c ---*/

    };

    /*static enum_val_t tcap_options[] = {
	{ "itu", "ITU",  ITU_TCAP_STANDARD },
	{ "ansi", "ANSI", ANSI_TCAP_STANDARD },
	{ NULL, NULL, 0 }
    };*/

    module_t *tcap_module;

/* Register the protocol name and description */
    proto_tcap = proto_register_protocol(PNAME, PSNAME, PFNAME);

/* Required function calls to register the header fields and subtrees used */
    proto_register_field_array(proto_tcap, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));

    tcap_module = prefs_register_protocol(proto_tcap, proto_reg_handoff_tcap);

    /*prefs_register_enum_preference(tcap_module, "standard", "ITU TCAP standard",
	"The SS7 standard used in ITU TCAP packets",
	&tcap_standard, tcap_options, FALSE);*/

/*    prefs_register_bool_preference(tcap_module, "lock_info_col", "Lock Info column",
	"Always show TCAP in Info column",
	&lock_info_col);
*/
    /* Set default SSNs */
    range_convert_str(&global_ssn_range, "1-255", MAX_SSN);
    ssn_range = range_empty();

    prefs_register_range_preference(tcap_module, "ssn", "SCCP SSNs",
	"SCCP (and SUA) SSNs to decode as TCAP",
	&global_ssn_range, MAX_SSN);

    /* we will fake a ssn subfield which has the same value obtained from sccp */
    tcap_itu_ssn_dissector_table = register_dissector_table("tcap.itu_ssn", "ITU TCAP SSN", FT_UINT8, BASE_DEC);
    tcap_ansi_ssn_dissector_table = register_dissector_table("tcap.ansi_ssn", "ANSI TCAP SSN", FT_UINT8, BASE_DEC);

	
}




/* If this dissector uses sub-dissector registration add a registration routine.
   This format is required because a script is used to find these routines and
   create the code that calls these routines.
*/

static void range_delete_callback(guint32 ssn)
{
    if (ssn) {
	dissector_delete("sccp.ssn", ssn, tcap_handle);
	dissector_delete("sua.ssn", ssn, tcap_handle);
    }
}

static void range_add_callback(guint32 ssn)
{
    if (ssn) {
	dissector_add("sccp.ssn", ssn, tcap_handle);
	dissector_add("sua.ssn", ssn, tcap_handle);
    }
}

void
proto_reg_handoff_tcap(void)
{
    static gboolean prefs_initialized = FALSE;

    if (!prefs_initialized) {

	tcap_handle = create_dissector_handle(dissect_tcap, proto_tcap);
	
	prefs_initialized = TRUE;

    } else {

	range_foreach(ssn_range, range_delete_callback);
    }

    g_free(ssn_range);
    ssn_range = range_copy(global_ssn_range);

    range_foreach(ssn_range, range_add_callback);
	register_ber_oid_name("0.0.17.773.1.1.1",
		"itu-t(0) recommendation(0) q(17) 773 as(1) dialogue-as(1) version1(1)");

}




/* some functions from packet_tcap which are needed by some other dissectors */

#define TC_DS_OK 1
#define TC_DS_FAIL 0


int
tcap_find_eoc(ASN1_SCK *asn1)
{
    guint	saved_offset;
    gint        prev_offset;
    guint	tag;
    guint	len;
    gboolean	def_len;

    saved_offset = asn1->offset;

    while (!asn1_eoc(asn1, -1))
    {
	prev_offset = asn1->offset;
	asn1_id_decode1(asn1, &tag);
	asn1_length_decode(asn1, &def_len, &len);

	if (def_len)
	{
	    asn1->offset += len;
	}
	else
	{
	    asn1->offset += tcap_find_eoc(asn1);
	    asn1_eoc_decode(asn1, -1);
	}
	if (prev_offset >= asn1->offset)
	    THROW(ReportedBoundsError);
    }

    len = asn1->offset - saved_offset;
    asn1->offset = saved_offset;

    return(len);
}

gboolean
tcap_check_tag(ASN1_SCK *asn1, guint tag)
{
    guint saved_offset, real_tag;

    if (tvb_length_remaining(asn1->tvb, asn1->offset) <= 0)
    {
	return (FALSE);
    }

    saved_offset = asn1->offset;
    asn1_id_decode1(asn1, &real_tag);
    asn1->offset = saved_offset;
    return (tag == real_tag);
}


static int
dissect_tcap_param(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset)
{
    gint orig_offset, tag_offset, saved_offset, len_offset;
    tvbuff_t	*next_tvb;
    proto_tree *subtree;
	proto_item *pi;
	gint32 tlen;
	guint8 class;
	gboolean pc;
	guint32 tag;
	guint32 len;
	guint32 ind_field;
	guint32 soffset = offset;
	
orig_offset = offset;
tlen = tvb_length_remaining(tvb, offset);      
	while (tlen > (offset-orig_offset))
    {
    saved_offset = offset;
	
    offset = get_ber_identifier(tvb, offset, &class, &pc, &tag);
    tag_offset = offset;
	offset = get_ber_length(tree, tvb, offset, &len, &ind_field);
	len_offset = offset;
	

	
	if (pc)
	{
	    pi =
		proto_tree_add_text(tree, tvb, saved_offset, -1, "CONSTRUCTOR");
	    subtree = proto_item_add_subtree(pi, ett_param);
	    proto_tree_add_uint_format(subtree, hf_tcap_tag, tvb,
		saved_offset, tag_offset-saved_offset, tag, "CONSTRUCTOR Tag");
	    proto_tree_add_uint(subtree, hf_tcap_tag, tvb,
		saved_offset, tag_offset-saved_offset, class);

		proto_tree_add_uint(subtree, hf_tcap_length, tvb,
		    tag_offset, len_offset-tag_offset, len);
	/* need to handle indefinite length */
		next_tvb = tvb_new_subset(tvb, offset, len, len);		
	     dissect_tcap_param(pinfo, subtree,next_tvb,0);
	     offset += len;
	}
	else
	{


	pi =
	    proto_tree_add_text(tree, tvb,
		saved_offset, len + (len_offset - saved_offset), "Parameter");

	subtree = proto_item_add_subtree(pi, ett_param);

	proto_tree_add_uint(subtree, hf_tcap_tag, tvb,
	    saved_offset, 1, tag);

	proto_tree_add_uint(subtree, hf_tcap_length, tvb,
	    saved_offset+1, 1, len);
	next_tvb = tvb_new_subset(tvb, offset, len, len);		
	dissect_ber_octet_string(TRUE, pinfo, tree, next_tvb, 0, hf_tcap_data,
                                    NULL);
	     offset += len;
	}
    }
return offset;
}