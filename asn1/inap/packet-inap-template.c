/* packet-inap-template.c
 * Routines for INAP
 * Copyright 2004, Tim Endean <endeant@hotmail.com>
 * Built from the gsm-map dissector Copyright 2004, Anders Broman <anders.broman@ericsson.com>
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

#include "packet-ber.h"
#include "packet-inap.h"
#include "packet-q931.h"
#include "packet-e164.h"

#define PNAME  "INAP"
#define PSNAME "INAP"
#define PFNAME "inap"

/* Initialize the protocol and registered fields */
int proto_inap = -1;
static int hf_inap_invokeCmd = -1;             /* Opcode */
static int hf_inap_invokeid = -1;              /* INTEGER */
static int hf_inap_absent = -1;                /* NULL */
static int hf_inap_invokeId = -1;              /* InvokeId */
static int hf_inap_invoke = -1;                /* InvokePDU */
static int hf_inap_returnResult = -1;                /* InvokePDU */
static int hf_inap_returnResult_result = -1;
static int hf_inap_getPassword = -1;  
static int hf_inap_currentPassword = -1;  
#include "packet-inap-hf.c"
static guint global_tcap_itu_ssn = 241;

/* Initialize the subtree pointers */
static gint ett_inap = -1;
static gint ett_inap_InvokeId = -1;
static gint ett_inap_InvokePDU = -1;
static gint ett_inap_ReturnResultPDU = -1;
static gint ett_inap_ReturnResult_result = -1;
static gint ett_inap_INAPPDU = -1;
static gint ett_inapisup_parameter = -1;
#include "packet-inap-ett.c"

static int  dissect_invokeCmd(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset);

#include "packet-inap-fn.c"


const value_string inap_opr_code_strings[] = {

{16, "AssistRequestInstructions"},
{44, "CallInformationReport"},
{45, "CallInformationRequest"},
{53, "Cancel"},
{20, "Connect"},
	{19,"ConnectToResource"},
	{17,"EstablishTemporaryConnection"},
	{24,"EventReportBCSM"},
	{34,"FurnishChargingInformation"},
	{0,"InitialDP"},
	{47,"PlayAnnouncement"},
	{48,"PromptAndCollectUserInformation"},
	{99,"ReceivedInformation"}, /*???????*/
	{33,"ResetTimer"},
	{23,"RequestReportBCSMEvent"},
	{49,"SpecializedResourceReport"}
};


static guint32 opcode=0;

static int
dissect_inap_Opcode(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_integer(FALSE, pinfo, tree, tvb, offset, hf_index, &opcode);

  if (check_col(pinfo->cinfo, COL_INFO)){
    col_set_str(pinfo->cinfo, COL_INFO, val_to_str(opcode, inap_opr_code_strings, "Unknown Inap (%u)"));
  }

  return offset;
}

static int dissect_invokeData(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  switch(opcode){
  case  16: /*AssistRequestInstructions*/
    offset=dissect_inap_AssistRequestInstructionsarg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case  44: /*CallInformationReport*/
    offset=dissect_inap_CallInformationReportarg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case  45: /*CallInformationRequest*/
    offset=dissect_inap_CallInformationRequestarg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case  53: /*Cancel*/
    offset=dissect_inap_Cancelarg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case  20: /*Connect*/
    offset=dissect_inap_Connectarg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
    case  18: /*DisconnectForwardConnections*/
    proto_tree_add_text(tree, tvb, offset, -1, "Disconnect Forward Connection");
    break;
  case  19: /*ConnectToResource*/
    offset=dissect_inap_ConnectToResource(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case  17: /*EstablishTemporaryConnection*/
    offset=dissect_inap_EstablishTemporaryConnection(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case  24: /*EventReportBCSM*/
    offset=dissect_inap_EventReportBCSM(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 34: /*FurnishChargingInformation*/
    offset=dissect_inap_FurnishChargingInformationarg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 0: /*InitialDP*/
    offset=dissect_inap_InitialDP(FALSE, tvb, offset, pinfo, tree, -1);
    break;
    
    case 23: /*InitialDP*/
    offset=dissect_inap_RequestReportBCSMEvent(FALSE, tvb, offset, pinfo, tree, -1);
    break;
 
  case 47: /*PlayAnnouncement*/
    offset=dissect_inap_PlayAnnouncement(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 48: /*PromptAndCollectUserInformation*/
    offset=dissect_inap_PromptAndCollectUserInformationarg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 33: /*ResetTimer*/
    offset=dissect_inap_ResetTimer(FALSE, tvb, offset, pinfo, tree, -1);
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
    offset=dissect_inap_PromptAndCollectUserInformationres(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  default:
    proto_tree_add_text(tree, tvb, offset, -1, "Unknown returnResultData blob");
  }
  return offset;
}

static int 
dissect_invokeCmd(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_inap_Opcode(FALSE, tvb, offset, pinfo, tree, hf_inap_invokeCmd);
}

static int dissect_invokeid(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_ber_integer(FALSE, pinfo, tree, tvb, offset, hf_inap_invokeid, NULL);
}


static const value_string InvokeId_vals[] = {
  {   0, "invokeid" },
  {   1, "absent" },
  { 0, NULL }
};

static int dissect_absent(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_inap_NULL(FALSE, tvb, offset, pinfo, tree, hf_inap_absent);
}


static const ber_choice_t InvokeId_choice[] = {
  {   0, BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_invokeid },
  {   1, BER_CLASS_UNI, BER_UNI_TAG_NULL, BER_FLAGS_NOOWNTAG, dissect_absent },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_inap_InvokeId(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              InvokeId_choice, hf_index, ett_inap_InvokeId);

  return offset;
}
static int dissect_invokeId(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_inap_InvokeId(FALSE, tvb, offset, pinfo, tree, hf_inap_invokeId);
}

static const ber_sequence_t InvokePDU_sequence[] = {
  { BER_CLASS_UNI, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_invokeId },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_invokeCmd },
  { BER_CLASS_UNI, -1/*depends on Cmd*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_invokeData },
  { 0, 0, 0, NULL }
};

static int
dissect_inap_InvokePDU(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                InvokePDU_sequence, hf_index, ett_inap_InvokePDU);

  return offset;
}
static int dissect_invoke_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_inap_InvokePDU(TRUE, tvb, offset, pinfo, tree, hf_inap_invoke);
}

static const ber_sequence_t ReturnResult_result_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_invokeCmd },
  { BER_CLASS_UNI, -1/*depends on Cmd*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_returnResultData },
  { 0, 0, 0, NULL }
};
static int
dissect_returnResult_result(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  offset = dissect_ber_sequence(FALSE, pinfo, tree, tvb, offset,
                                ReturnResult_result_sequence, hf_inap_returnResult_result, ett_inap_ReturnResult_result);

  return offset;
}

static const ber_sequence_t ReturnResultPDU_sequence[] = {
  { BER_CLASS_UNI, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_invokeId },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_returnResult_result },
  { 0, 0, 0, NULL }
};

static int
dissect_inap_returnResultPDU(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                ReturnResultPDU_sequence, hf_index, ett_inap_ReturnResultPDU);

  return offset;
}
static int dissect_returnResult_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_inap_returnResultPDU(TRUE, tvb, offset, pinfo, tree, hf_inap_returnResult);
}

static const value_string INAPPDU_vals[] = {
  {   1, "invoke" },
  {   2, "returnResult" },
  {   3, "returnError" },
  {   4, "reject" },
  { 0, NULL }
};

static const ber_choice_t INAPPDU_choice[] = {
  {   1, BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_invoke_impl },
  {   2, BER_CLASS_CON, 2, BER_FLAGS_IMPLTAG, dissect_returnResult_impl },
#ifdef REMOVED
  {   3, BER_CLASS_CON, 3, BER_FLAGS_IMPLTAG, dissect_returnError_impl },
  {   4, BER_CLASS_CON, 4, BER_FLAGS_IMPLTAG, dissect_reject_impl },
#endif
  { 0, 0, 0, 0, NULL }
};

static guint8 inap_pdu_type = 0;
static guint8 inap_pdu_size = 0;

static int
dissect_inap_INAPPDU(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {

  inap_pdu_type = tvb_get_guint8(tvb, offset)&0x0f;
  /* Get the length and add 2 */
  inap_pdu_size = tvb_get_guint8(tvb, offset+1)+2;

  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              INAPPDU_choice, hf_index, ett_inap_INAPPDU);

  if (check_col(pinfo->cinfo, COL_INFO)){
    col_prepend_fstr(pinfo->cinfo, COL_INFO, val_to_str(opcode, inap_opr_code_strings, "Unknown INAP (%u)"));
  }

  return offset;
}




static void
dissect_inap(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree)
{
    proto_item		*item=NULL;
    proto_tree		*tree=NULL;


    if (check_col(pinfo->cinfo, COL_PROTOCOL))
    {
	col_set_str(pinfo->cinfo, COL_PROTOCOL, "INAP");
    }

    /* create display subtree for the protocol */
    if(parent_tree){
       item = proto_tree_add_item(parent_tree, proto_inap, tvb, 0, -1, FALSE);
       tree = proto_item_add_subtree(item, ett_inap);
    }

    dissect_inap_INAPPDU(FALSE, tvb, 0, pinfo, tree, -1);


}

/*--- proto_reg_handoff_inap ---------------------------------------*/
void proto_reg_handoff_inap(void) {
    dissector_handle_t	inap_handle;

    inap_handle = create_dissector_handle(dissect_inap, proto_inap);
    dissector_add("tcap.itu_ssn", global_tcap_itu_ssn, inap_handle);
   
}


void proto_register_inap(void) {
	module_t *inap_module;
  /* List of fields */
  static hf_register_info hf[] = {
    { &hf_inap_invokeCmd,
      { "invokeCmd", "inap.invokeCmd",
        FT_UINT32, BASE_DEC, VALS(inap_opr_code_strings), 0,
        "InvokePDU/invokeCmd", HFILL }},
    { &hf_inap_invokeid,
      { "invokeid", "inap.invokeid",
        FT_INT32, BASE_DEC, NULL, 0,
        "InvokeId/invokeid", HFILL }},
    { &hf_inap_absent,
      { "absent", "inap.absent",
        FT_NONE, BASE_NONE, NULL, 0,
        "InvokeId/absent", HFILL }},
    { &hf_inap_invokeId,
      { "invokeId", "inap.invokeId",
        FT_UINT32, BASE_DEC, VALS(InvokeId_vals), 0,
        "InvokePDU/invokeId", HFILL }},
    { &hf_inap_invoke,
      { "invoke", "inap.invoke",
        FT_NONE, BASE_NONE, NULL, 0,
        "INAPPDU/invoke", HFILL }},
    { &hf_inap_returnResult,
      { "returnResult", "inap.returnResult",
        FT_NONE, BASE_NONE, NULL, 0,
        "INAPPDU/returnResult", HFILL }},
     

#include "packet-inap-hfarr.c"
  };






  /* List of subtrees */
  static gint *ett[] = {
    &ett_inap,
    &ett_inap_InvokeId,
    &ett_inap_InvokePDU,
    &ett_inap_ReturnResultPDU,
    &ett_inap_ReturnResult_result,
    &ett_inap_INAPPDU,
    &ett_inapisup_parameter,
#include "packet-inap-ettarr.c"
  };

  /* Register protocol */
  proto_inap = proto_register_protocol(PNAME, PSNAME, PFNAME);
/*XXX  register_dissector("inap", dissect_inap, proto_inap);*/
  /* Register fields and subtrees */
  proto_register_field_array(proto_inap, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));
  
  inap_module = prefs_register_protocol(proto_inap, proto_reg_handoff_inap);
  
  prefs_register_uint_preference(inap_module, "tcap.itu_ssn",
		"Subsystem number used for INAP",
		"Set Subsystem number used for INAP",
		10, &global_tcap_itu_ssn);


}



