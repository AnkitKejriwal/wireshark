/* packet-camel-template.c
 * Routines for Camel
 * Copyright 2004, Tim Endean <endeant@hotmail.com>
 * Copyright 2005, Olivier Jacques <olivier.jacques@hp.com>
 * Built from the gsm-map dissector Copyright 2004, Anders Broman <anders.broman@ericsson.com>
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
 * References: ETSI 300 374
 */
/* 
 * Indentation logic: this file is indented with 2 spaces indentation. 
 *                    there are no tabs.
 */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glib.h>
#include <epan/packet.h>
#include <epan/prefs.h>
#include <epan/conversation.h>
#include <epan/tap.h>

#include <stdio.h>
#include <string.h>

#include "packet-ber.h"
#include "packet-camel.h"
#include "packet-q931.h"
#include "packet-e164.h"
#include "packet-isup.h"

#define PNAME  "Camel"
#define PSNAME "CAMEL"
#define PFNAME "camel"

/* Initialize the protocol and registered fields */
int proto_camel = -1;
static int hf_camel_invokeCmd = -1;             /* Opcode */
static int hf_camel_invokeid = -1;              /* INTEGER */
static int hf_camel_absent = -1;                /* NULL */
static int hf_camel_invokeId = -1;              /* InvokeId */
static int hf_camel_invoke = -1;                /* InvokePDU */
static int hf_camel_returnResult = -1;          /* InvokePDU */
static int hf_camel_returnResult_result = -1;
static int hf_camel_getPassword = -1;  
static int hf_camel_currentPassword = -1;  
#include "packet-camel-hf.c"
static guint global_tcap_itu_ssn = 0;

/* Initialize the subtree pointers */
static gint ett_camel = -1;
static gint ett_camel_InvokeId = -1;
static gint ett_camel_InvokePDU = -1;
static gint ett_camel_ReturnResultPDU = -1;
static gint ett_camel_ReturnResult_result = -1;
static gint ett_camel_camelPDU = -1;
static gint ett_camelisup_parameter = -1;
#include "packet-camel-ett.c"

static int hf_camel_nature_of_number = -1;
static int hf_camel_number_plan = -1;
static int hf_camel_cause_indicator = -1;

/* Preference settings default */
#define MAX_SSN 254
static range_t *global_ssn_range;
static range_t *ssn_range;
dissector_handle_t  camel_handle;

/* Global variables */

int application_context_version;

static int  dissect_invokeCmd(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset);

static char*
unpack_digits(tvbuff_t *tvb, int offset){

  int length;
  guint8 octet;
  int i=0;
  char *digit_str;

  length = tvb_length(tvb);
  length = length - offset;
  digit_str = g_malloc(length+1);

  while ( offset < length ){
    octet = tvb_get_guint8(tvb,offset);
    digit_str[i] = ((octet & 0x0f) + 0x30);
    i++;

    /*
     * unpack second value in byte
     */
    octet = octet >> 4;

    if (octet == 0x0f)  /* odd number bytes - hit filler */
      break;

    digit_str[i] = ((octet & 0x0f) + 0x30);
    i++;
    offset++;

  }
  digit_str[i]= '\0';
  return digit_str;
}

static const true_false_string camel_extension_value = {
  "No Extension",
  "Extension"
};
static const value_string camel_nature_of_number_values[] = {
  {   0x00,  "unknown" },
  {   0x01,  "International Number" },
  {   0x02,  "National Significant Number" },
  {   0x03,  "Network Specific Number" },
  {   0x04,  "Subscriber Number" },
  {   0x05,  "Reserved" },
  {   0x06,  "Abbreviated Number" },
  {   0x07,  "Reserved for extension" },
  { 0, NULL }
};
static const value_string camel_number_plan_values[] = {
  {   0x00,  "unknown" },
  {   0x01,  "ISDN/Telephony Numbering (Rec ITU-T E.164)" },
  {   0x02,  "spare" },
  {   0x03,  "Data Numbering (ITU-T Rec. X.121)" },
  {   0x04,  "Telex Numbering (ITU-T Rec. F.69)" },
  {   0x05,  "spare" },
  {   0x06,  "Land Mobile Numbering (ITU-T Rec. E.212)" },
  {   0x07,  "spare" },
  {   0x08,  "National Numbering" },
  {   0x09,  "Private Numbering" },
  {   0x0f,  "Reserved for extension" },
  { 0, NULL }
};

/* End includes from old" packet-camel.c */

#include "packet-camel-fn.c"

const value_string camel_opr_code_strings[] = {

  {0, "InitialDP"},
  {16, "AssistRequestInstructions"},
  {17, "EstablishTemporaryConnection"},
  {18, "DisconnectForwardConnection"},
  {19, "ConnectToResource"},
  {20, "Connect"},
  {22, "ReleaseCall"},
  {23, "RequestReportBCSMEvent"},
  {24, "EventReportBCSM"},
  {31, "Continue"},
  {33, "ResetTimer"},
  {34, "FurnishChargingInformation"},
  {35, "ApplyCharging"},
  {36, "ApplyChargingReport"},
  {41, "CallGap"},
  {44, "CallInformationReport"},
  {45, "CallInformationRequest"},
  {46, "SendChargingInformation"},
  {47, "PlayAnnouncement"},
  {48, "PromptAndCollectUserInformation"},
  {49, "SpecializedResourceReport"},
  {53, "Cancel"},
  {55, "ActivityTest"},
  {56, "ContinueWithArgument"},
  {60, "InitialDPSMS"},
  {61, "FurnishChargingInformationSMS"},
  {62, "ConnectSMS"},
  {63, "RequestReportSMSEvent"},
  {64, "EventReportSMS"},
  {65, "ContinueSMS"},
  {66, "ReleaseSMS"},
  {67, "ResetTimerSMS"},
  {70, "ActivityTestGPRS"},
  {71, "ApplyChargingGPRS"},
  {72, "ApplyChargingReportGPRS"},
  {73, "CancelGPRS"},
  {74, "ConnectGPRS"},
  {75, "ContinueGPRS"},
  {76, "EntityReleasedGPRS"},
  {77, "FurnishChargingInformationGPRS"},
  {78, "InitialDPGPRS"},
  {79, "ReleaseGPRS"},
  {80, "EventReportGPRS"},
  {81, "RequestReportGPRSEvent"},
  {82, "ResetTimerGPRS"},
  {83, "SendChargingInformationGPRS"}
};


static guint32 opcode=0;

static int
dissect_camel_Opcode(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_integer(FALSE, pinfo, tree, tvb, offset, hf_index, &opcode);

  if (check_col(pinfo->cinfo, COL_INFO)){
    /* Add Camel Opcode to INFO column */
    col_append_fstr(pinfo->cinfo, COL_INFO, val_to_str(opcode, camel_opr_code_strings, "Unknown Camel (%u)"));
    col_append_fstr(pinfo->cinfo, COL_INFO, " ");
  }
  return offset;
}

static int dissect_invokeData(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  switch(opcode){
  case 0: /*InitialDP*/
    offset=dissect_camel_InitialDPArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 16: /*AssistRequestInstructions*/
    offset=dissect_camel_AssistRequestInstructionsArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 17: /*EstablishTemporaryConnection*/
    offset=dissect_camel_EstablishTemporaryConnectionArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 18: /*DisconnectForwardConnections*/
    proto_tree_add_text(tree, tvb, offset, -1, "Disconnect Forward Connection");
    break;
  case 19: /*ConnectToResource*/
    offset=dissect_camel_ConnectToResourceArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 20: /*Connect*/
    offset=dissect_camel_ConnectArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 22: /*releaseCall*/
    offset=dissect_camel_ReleaseCallArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 23: /*RequestReportBCSMEvent*/
    offset=dissect_camel_RequestReportBCSMEventArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 24: /*EventReportBCSM*/
    offset=dissect_camel_EventReportBCSMArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 31: /*Continue*/
    /* Continue: no arguments - do nothing */
    break;
  case 33: /*ResetTimer*/
    offset=dissect_camel_ResetTimerArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 34: /*FurnishChargingInformation*/
    offset=dissect_camel_FurnishChargingInformationArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 35: /*ApplyCharging*/
    offset=dissect_camel_ApplyChargingArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 36: /*ApplyChargingReport*/
    offset=dissect_camel_ApplyChargingReportArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 41: /*CallGap*/
    offset=dissect_camel_CallGapArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 44: /*CallInformationReport*/
    offset=dissect_camel_CallInformationReportArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 45: /*CallInformationRequest*/
    offset=dissect_camel_CallInformationRequestArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 46: /*SendChargingInformation*/
    offset=dissect_camel_SendChargingInformationArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 47: /*PlayAnnouncement*/
    offset=dissect_camel_PlayAnnouncementArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 48: /*PromptAndCollectUserInformation*/
    offset=dissect_camel_PromptAndCollectUserInformationArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 49: /*SpecializedResourceReport*/
    offset=dissect_camel_SpecializedResourceReportArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 53: /*Cancel*/
    offset=dissect_camel_CancelArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 56: /*ContinueWithArgument*/
    offset=dissect_camel_ContinueWithArgumentArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 60: /*InitialDPSMS*/
    offset=dissect_camel_InitialDPSMSArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 61: /*FurnishChargingInformationSMS*/
    offset=dissect_camel_FurnishChargingInformationSMSArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 62: /*ConnectSMS*/
    offset=dissect_camel_ConnectSMSArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 63: /*RequestReportSMSEvent*/
    offset=dissect_camel_RequestReportSMSEventArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 64: /*EventReportSMS*/
    offset=dissect_camel_EventReportSMSArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 65: /*ContinueSMS*/
    /* ContinueSMS: no arguments - do nothing */
    break;
  case 66: /*ReleaseSMS*/
    offset=dissect_camel_ReleaseSMSArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 67: /*ResetTimerSMS*/
    offset=dissect_camel_ResetTimerSMSArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 71: /*ApplyChargingGPRS*/
    offset=dissect_camel_ApplyChargingGPRSArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 73: /*CancelGPRS*/
    offset=dissect_camel_CancelGPRSArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 74: /*ConnectGPRS*/
    offset=dissect_camel_ConnectGPRSArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 75: /*ContinueGPRS*/
    offset=dissect_camel_ContinueGPRSArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 77: /*FurnishChargingInformationGPRS*/
    offset=dissect_camel_FurnishChargingInformationGPRSArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 78: /*InitialDPGPRS*/
    offset=dissect_camel_InitialDPGPRSArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 79: /*ReleaseGPRS*/
    offset=dissect_camel_ReleaseGPRSArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 81: /*RequestReportGPRSEvent*/
    offset=dissect_camel_RequestReportGPRSEventArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 82: /*ResetTimerGPRS*/
    offset=dissect_camel_ResetTimerGPRSArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 83: /*SendChargingInformationGPRS*/
    offset=dissect_camel_SendChargingInformationGPRSArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  default:
    proto_tree_add_text(tree, tvb, offset, -1, "Unknown invokeData blob");
    /* todo call the asn.1 dissector */
  }
  return offset;
}


static int dissect_returnResultData(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  switch(opcode){
  case 48: /*PromptAndCollectUserInformation*/
    offset=dissect_camel_ReceivedInformationArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 55: /*ActivityTest*/
    /* ActivityTest: no arguments - do nothing */
    break;
  case 70: /*ActivityTestGPRS*/
    /* ActivityTestGPRS: no arguments - do nothing */
    break;
  case 72: /*ApplyChargingReportGPRS*/
    offset=dissect_camel_ApplyChargingReportGPRSArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 76: /*EntityReleasedGPRS*/
    offset=dissect_camel_EntityReleasedGPRSArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 80: /*EventReportGPRS*/
    offset=dissect_camel_EventReportGPRSArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  default:
    proto_tree_add_text(tree, tvb, offset, -1, "Unknown returnResultData blob");
  }
  return offset;
}

static int 
dissect_invokeCmd(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_Opcode(FALSE, tvb, offset, pinfo, tree, hf_camel_invokeCmd);
}

static int dissect_invokeid(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_ber_integer(FALSE, pinfo, tree, tvb, offset, hf_camel_invokeid, NULL);
}


static const value_string InvokeId_vals[] = {
  {   0, "invokeid" },
  {   1, "absent" },
  { 0, NULL }
};

static int dissect_absent(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_NULL(FALSE, tvb, offset, pinfo, tree, hf_camel_absent);
}

static const ber_choice_t InvokeId_choice[] = {
  {   0, BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_invokeid },
  {   1, BER_CLASS_UNI, BER_UNI_TAG_NULL, BER_FLAGS_NOOWNTAG, dissect_absent },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_camel_InvokeId(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              InvokeId_choice, hf_index, ett_camel_InvokeId);

  return offset;
}
static int dissect_invokeId(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_InvokeId(FALSE, tvb, offset, pinfo, tree, hf_camel_invokeId);
}

static const ber_sequence_t InvokePDU_sequence[] = {
  { BER_CLASS_UNI, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_invokeId },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_invokeCmd },
  { BER_CLASS_UNI, -1/*depends on Cmd*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_invokeData },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_InvokePDU(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                InvokePDU_sequence, hf_index, ett_camel_InvokePDU);

  return offset;
}
static int dissect_invoke_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_InvokePDU(TRUE, tvb, offset, pinfo, tree, hf_camel_invoke);
}

static const ber_sequence_t ReturnResult_result_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_invokeCmd },
  { BER_CLASS_UNI, -1/*depends on Cmd*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_returnResultData },
  { 0, 0, 0, NULL }
};
static int
dissect_returnResult_result(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  offset = dissect_ber_sequence(FALSE, pinfo, tree, tvb, offset,
                                ReturnResult_result_sequence, hf_camel_returnResult_result, ett_camel_ReturnResult_result);

  return offset;
}

static const ber_sequence_t ReturnResultPDU_sequence[] = {
  { BER_CLASS_UNI, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_invokeId },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_returnResult_result },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_returnResultPDU(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                ReturnResultPDU_sequence, hf_index, ett_camel_ReturnResultPDU);

  return offset;
}
static int dissect_returnResult_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_returnResultPDU(TRUE, tvb, offset, pinfo, tree, hf_camel_returnResult);
}

static const value_string camelPDU_vals[] = {
  {   1, "Invoke " },
  {   2, "ReturnResult " },
  {   3, "ReturnError " },
  {   4, "Reject " },
  { 0, NULL }
};

static const ber_choice_t camelPDU_choice[] = {
  {   1, BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_invoke_impl },
  {   2, BER_CLASS_CON, 2, BER_FLAGS_IMPLTAG, dissect_returnResult_impl },
#ifdef REMOVED
  {   3, BER_CLASS_CON, 3, BER_FLAGS_IMPLTAG, dissect_returnError_impl },
  {   4, BER_CLASS_CON, 4, BER_FLAGS_IMPLTAG, dissect_reject_impl },
#endif
  { 0, 0, 0, 0, NULL }
};

static guint8 camel_pdu_type = 0;
static guint8 camel_pdu_size = 0;

static int
dissect_camel_camelPDU(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {

  char *version_ptr, *version_str;

  opcode = 0;
  application_context_version = 0;
  if (pinfo->private_data != NULL){
    version_ptr = strrchr(pinfo->private_data,'.');
    version_str = g_strdup(version_ptr+1);
    application_context_version = atoi(version_str);
  }

  camel_pdu_type = tvb_get_guint8(tvb, offset)&0x0f;
  /* Get the length and add 2 */
  camel_pdu_size = tvb_get_guint8(tvb, offset+1)+2;

  if (check_col(pinfo->cinfo, COL_INFO)){
    /* Populate the info column with PDU type*/
    col_append_fstr(pinfo->cinfo, COL_INFO, val_to_str(camel_pdu_type, camelPDU_vals, "Unknown Camel (%u)"));
  }

  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              camelPDU_choice, hf_index, ett_camel_camelPDU);

  return offset;
}

static void
dissect_camel(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree)
{
  proto_item    *item=NULL;
  proto_tree    *tree=NULL;
  gchar         *str = NULL;

  if (check_col(pinfo->cinfo, COL_PROTOCOL)) {
    col_set_str(pinfo->cinfo, COL_PROTOCOL, "Camel");
  }

  /* create display subtree for the protocol */
  if(parent_tree){
     item = proto_tree_add_item(parent_tree, proto_camel, tvb, 0, -1, FALSE);
     tree = proto_item_add_subtree(item, ett_camel);
  }

  dissect_camel_camelPDU(FALSE, tvb, 0, pinfo, tree, -1);

}

/*--- proto_reg_handoff_camel ---------------------------------------*/
static void range_delete_callback(guint32 ssn)
{
  if (ssn) {
    dissector_delete("tcap.itu_ssn", ssn, camel_handle);
  }
}

static void range_add_callback(guint32 ssn)
{
  if (ssn) {
    dissector_add("tcap.itu_ssn", ssn, camel_handle);
  }
}

void proto_reg_handoff_camel(void) {

  static int camel_prefs_initialized = FALSE;
  
  if (!camel_prefs_initialized) {
    camel_prefs_initialized = TRUE;
    camel_handle = create_dissector_handle(dissect_camel, proto_camel);
  } else {
    range_foreach(ssn_range, range_delete_callback);
  }

  g_free(ssn_range);
  ssn_range = range_copy(global_ssn_range);

  range_foreach(ssn_range, range_add_callback);
  
}

void proto_register_camel(void) {
  module_t *camel_module;
  /* List of fields */
  static hf_register_info hf[] = {
  { &hf_camel_cause_indicator, /* Currently not enabled */
    { "Cause indicator",  "camel.cause_indicator",
      FT_UINT8, BASE_DEC, VALS(q850_cause_code_vals), 0x7f,
      "", HFILL }},
    { &hf_camel_invokeCmd,
      { "invokeCmd", "camel.invokeCmd",
        FT_UINT32, BASE_DEC, VALS(camel_opr_code_strings), 0,
        "InvokePDU/invokeCmd", HFILL }},
    { &hf_camel_invokeid,
      { "invokeid", "camel.invokeid",
        FT_INT32, BASE_DEC, NULL, 0,
        "InvokeId/invokeid", HFILL }},
    { &hf_camel_absent,
      { "absent", "camel.absent",
        FT_NONE, BASE_NONE, NULL, 0,
        "InvokeId/absent", HFILL }},
    { &hf_camel_invokeId,
      { "invokeId", "camel.invokeId",
        FT_UINT32, BASE_DEC, VALS(InvokeId_vals), 0,
        "InvokePDU/invokeId", HFILL }},
    { &hf_camel_invoke,
      { "invoke", "camel.invoke",
        FT_NONE, BASE_NONE, NULL, 0,
        "camelPDU/invoke", HFILL }},
    { &hf_camel_returnResult,
      { "returnResult", "camel.returnResult",
        FT_NONE, BASE_NONE, NULL, 0,
        "camelPDU/returnResult", HFILL }},
#ifdef REMOVED
    { &hf_camel_extension,
     { "Extension", "camel.extension",
        FT_BOOLEAN, 8, TFS(&camel_extension_value), 0x80,
        "Extension", HFILL }},
    { &hf_camel_nature_of_number,
      { "Nature of number", "gsm_map.nature_of_number",
        FT_UINT8, BASE_HEX, VALS(camel_nature_of_number_values), 0x70,
        "Nature of number", HFILL }},
    { &hf_camel_number_plan,
      { "Number plan", "camel.number_plan",
        FT_UINT8, BASE_HEX, VALS(camel_number_plan_values), 0x0f,
        "Number plan", HFILL }},
  { &hf_camel_digits,
      { "Address digits", "camel.address_digits",
        FT_STRING, BASE_NONE, NULL, 0,
        "Address digits", HFILL }},
#endif
#include "packet-camel-hfarr.c"
  };

  /* List of subtrees */
  static gint *ett[] = {
    &ett_camel,
    &ett_camel_InvokeId,
    &ett_camel_InvokePDU,
    &ett_camel_ReturnResultPDU,
    &ett_camel_ReturnResult_result,
    &ett_camel_camelPDU,
    &ett_camelisup_parameter,
#include "packet-camel-ettarr.c"
  };

  /* Register protocol */
  proto_camel = proto_register_protocol(PNAME, PSNAME, PFNAME);

  proto_register_field_array(proto_camel, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));
  register_ber_oid_name("0.4.0.0.1.0.50.1","itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network|umts-Network(1) applicationContext(0) cap-gsmssf-to-gsmscf(50) version2(1)" );
  register_ber_oid_name("0.4.0.0.1.0.51.1","itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network|umts-Network(1) applicationContext(0) cap-assist-handoff-gsmssf-to-gsmscf(51) version2(1)" );
  register_ber_oid_name("0.4.0.0.1.0.52.1","itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network|umts-Network(1) applicationContext(0) cap-gsmSRF-to-gsmscf(52) version2(1)" );

  /* Register our configuration options, particularly our ssn:s */
  /* Set default SSNs */
  range_convert_str(&global_ssn_range, "6-9", MAX_SSN);
  ssn_range = range_empty();

  camel_module = prefs_register_protocol(proto_camel, proto_reg_handoff_camel);

  prefs_register_range_preference(camel_module, "tcap.ssn",
    "TCAP SSNs",
    "TCAP Subsystem numbers used for Camel",
    &global_ssn_range, MAX_SSN);
}

