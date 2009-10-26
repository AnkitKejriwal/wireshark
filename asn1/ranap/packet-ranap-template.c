/* packet-ranap.c
 * Routines for UMTS Node B Application Part(RANAP) packet dissection
 * Copyright 2005, Anders Broman <anders.broman[AT]ericsson.com>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * References: 3GPP TS 25.413 version 6.6.0 Release
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glib.h>
#include <epan/packet.h>

#include <stdio.h>
#include <string.h>
#include <epan/emem.h>
#include <epan/strutil.h>
#include <epan/asn1.h>
#include <epan/prefs.h>

#include "packet-ber.h"
#include "packet-per.h"
#include "packet-gsm_map.h"
#include "packet-ranap.h"
#include "packet-e212.h"
#include "packet-sccp.h"

#ifdef _MSC_VER
/* disable: "warning C4146: unary minus operator applied to unsigned type, result still unsigned" */
#pragma warning(disable:4146)
#endif

#define SCCP_SSN_RANAP 142

#define PNAME  "Radio Access Network Application Part"
#define PSNAME "RANAP"
#define PFNAME "ranap"

/* Higest Ranap_ProcedureCode_value, use in heuristics */
#define RANAP_MAX_PC  45 /* id_RANAPenhancedRelocation =  45 */

#include "packet-ranap-val.h"

/* Initialize the protocol and registered fields */
static int proto_ranap = -1;

/* initialise sub-dissector handles */
static dissector_handle_t rrc_handle = NULL;

static int hf_ranap_imsi_digits = -1;
static int hf_ranap_transportLayerAddress_ipv4 = -1;
static int hf_ranap_transportLayerAddress_ipv6 = -1;

#include "packet-ranap-hf.c"

/* Initialize the subtree pointers */
static int ett_ranap = -1;
static int ett_ranap_TransportLayerAddress = -1;

#include "packet-ranap-ett.c"

/* Global variables */
static guint32 ProcedureCode;
static guint32 ProtocolIE_ID;
static guint32 ProtocolExtensionID;

/* Some IE:s identities uses the same value for different IE:s
 * depending on PDU type:
 * InitiatingMessage
 * SuccessfulOutcome
 * UnsuccessfulOutcome
 * Outcome
 * As a workarond a value is added to the IE:id in the .cnf file.
 * Example:
 * ResetResourceList                N rnsap.ies IMSG||id-IuSigConIdList  # no spaces are allowed in value as a space is delimiter
 * PDU type is stored in a global variable and can is used in the IE decoding section.
 */
/*
 * 	&InitiatingMessage				,
 *	&SuccessfulOutcome				OPTIONAL,
 *	&UnsuccessfulOutcome				OPTIONAL,
 *	&Outcome					OPTIONAL,
 *
 * Only these two needed currently 
 */
#define IMSG (1<<16)
#define SOUT (2<<16)
#define SPECIAL (4<<16)

int pdu_type = 0; /* 0 means wildcard */

/* Initialise the Preferences */
static gint global_ranap_sccp_ssn = SCCP_SSN_RANAP;

/* Dissector tables */
static dissector_table_t ranap_ies_dissector_table;
static dissector_table_t ranap_ies_p1_dissector_table;
static dissector_table_t ranap_ies_p2_dissector_table;
static dissector_table_t ranap_extension_dissector_table;
static dissector_table_t ranap_proc_imsg_dissector_table;
static dissector_table_t ranap_proc_sout_dissector_table;
static dissector_table_t ranap_proc_uout_dissector_table;
static dissector_table_t ranap_proc_out_dissector_table;
static dissector_table_t nas_pdu_dissector_table;

static int dissect_ProtocolIEFieldValue(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);
static int dissect_ProtocolIEFieldPairFirstValue(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);
static int dissect_ProtocolIEFieldPairSecondValue(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);
static int dissect_ProtocolExtensionFieldExtensionValue(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);
static int dissect_InitiatingMessageValue(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);
static int dissect_SuccessfulOutcomeValue(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);
static int dissect_UnsuccessfulOutcomeValue(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);
static int dissect_OutcomeValue(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);
void proto_reg_handoff_ranap(void);

#include "packet-ranap-fn.c"

static int dissect_ProtocolIEFieldValue(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{

  int ret = 0;
  int key;

  /* Special handling, same ID used for different IE's depending on signal */
  switch(ProcedureCode){
	  case id_RelocationPreparation:
		  if((ProtocolIE_ID == id_Source_ToTarget_TransparentContainer)||(ProtocolIE_ID == id_Target_ToSource_TransparentContainer)){
			  key = SPECIAL | ProtocolIE_ID;
			  ret = (dissector_try_port_new(ranap_ies_dissector_table, key, tvb, pinfo, tree, FALSE)) ? tvb_length(tvb) : 0;
			  break;
		  }
		  /* Fall trough */
	  default:
		  /* no special handling */
		  ret = (dissector_try_port_new(ranap_ies_dissector_table, ProtocolIE_ID, tvb, pinfo, tree, FALSE)) ? tvb_length(tvb) : 0;
		  if (ret == 0) {
			  key = pdu_type || ProtocolIE_ID;
			  ret = (dissector_try_port_new(ranap_ies_dissector_table, key, tvb, pinfo, tree, FALSE)) ? tvb_length(tvb) : 0;
		  }
		  break;
  }
  return ret;
}

static int dissect_ProtocolIEFieldPairFirstValue(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
  return (dissector_try_port_new(ranap_ies_p1_dissector_table, ProtocolIE_ID, tvb, pinfo, tree, FALSE)) ? tvb_length(tvb) : 0;
}

static int dissect_ProtocolIEFieldPairSecondValue(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
  return (dissector_try_port_new(ranap_ies_p2_dissector_table, ProtocolIE_ID, tvb, pinfo, tree, FALSE)) ? tvb_length(tvb) : 0;
}

static int dissect_ProtocolExtensionFieldExtensionValue(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
  return (dissector_try_port_new(ranap_extension_dissector_table, ProtocolExtensionID, tvb, pinfo, tree, FALSE)) ? tvb_length(tvb) : 0;
}

static int dissect_InitiatingMessageValue(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
  pdu_type = IMSG;
  return (dissector_try_port_new(ranap_proc_imsg_dissector_table, ProcedureCode, tvb, pinfo, tree, FALSE)) ? tvb_length(tvb) : 0;
  pdu_type = 0;
}

static int dissect_SuccessfulOutcomeValue(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
  pdu_type = SOUT;
  return (dissector_try_port_new(ranap_proc_sout_dissector_table, ProcedureCode, tvb, pinfo, tree, FALSE)) ? tvb_length(tvb) : 0;
  pdu_type = 0;
}

static int dissect_UnsuccessfulOutcomeValue(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
  return (dissector_try_port_new(ranap_proc_uout_dissector_table, ProcedureCode, tvb, pinfo, tree, FALSE)) ? tvb_length(tvb) : 0;
}

static int dissect_OutcomeValue(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
  return (dissector_try_port_new(ranap_proc_out_dissector_table, ProcedureCode, tvb, pinfo, tree, FALSE)) ? tvb_length(tvb) : 0;
}

static void
dissect_ranap(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	proto_item	*ranap_item = NULL;
	proto_tree	*ranap_tree = NULL;

	pdu_type = 0;

	/* make entry in the Protocol column on summary display */
	col_set_str(pinfo->cinfo, COL_PROTOCOL, "RANAP");

	/* create the ranap protocol tree */
	ranap_item = proto_tree_add_item(tree, proto_ranap, tvb, 0, -1, FALSE);
	ranap_tree = proto_item_add_subtree(ranap_item, ett_ranap);

	dissect_RANAP_PDU_PDU(tvb, pinfo, ranap_tree);
	if (pinfo->sccp_info) {
		sccp_msg_info_t* sccp_msg = pinfo->sccp_info;

		if (sccp_msg->data.co.assoc)
			sccp_msg->data.co.assoc->payload = SCCP_PLOAD_RANAP;

		if (! sccp_msg->data.co.label && ProcedureCode != 0xFFFFFFFF) {
			const gchar* str = val_to_str(ProcedureCode, ranap_ProcedureCode_vals,"Unknown RANAP");
			sccp_msg->data.co.label = se_strdup(str);
		}
	}
}

static gboolean
dissect_sccp_ranap_heur(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
    guint8 temp;
	asn1_ctx_t asn1_ctx;
	guint length;
	int offset;

	asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);

    /* Is it a ranap packet?
     *
     * 4th octet should be the length of the rest of the message.
     * 2nd octet is the message-type e Z[0, 28]
     * (obviously there must be at least four octets)
     *
     * If both hold true we'll assume its RANAP
     */

    #define LENGTH_OFFSET 3
    #define MSG_TYPE_OFFSET 1
    if (tvb_length(tvb) < 4) { return FALSE; }
    /*if (tvb_get_guint8(tvb, LENGTH_OFFSET) != (tvb_length(tvb) - 4)) { return FALSE; }*/
	/* Read the length NOTE offset in bits */
	offset = dissect_per_length_determinant(tvb, LENGTH_OFFSET<<3, &asn1_ctx, tree, -1, &length);
	offset = offset>>3;
	if (length!= (tvb_length(tvb) - offset)){
		return FALSE; 
	}

    temp = tvb_get_guint8(tvb, MSG_TYPE_OFFSET);
    if (temp > RANAP_MAX_PC) { return FALSE; }

    dissect_ranap(tvb, pinfo, tree);

    return TRUE;
}

/*--- proto_register_ranap -------------------------------------------*/
void proto_register_ranap(void) {
  module_t *ranap_module;

  /* List of fields */

  static hf_register_info hf[] = {
	{ &hf_ranap_imsi_digits,
      { "IMSI digits", "ranap.imsi_digits",
        FT_STRING, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ranap_transportLayerAddress_ipv4,
      { "transportLayerAddress IPv4", "ranap.transportLayerAddress_ipv4",
        FT_IPv4, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ranap_transportLayerAddress_ipv6,
      { "transportLayerAddress IPv6", "ranap.transportLayerAddress_ipv6",
        FT_IPv6, BASE_NONE, NULL, 0,
        NULL, HFILL }},


#include "packet-ranap-hfarr.c"
  };

  /* List of subtrees */
  static gint *ett[] = {
		  &ett_ranap,
		  &ett_ranap_TransportLayerAddress,
#include "packet-ranap-ettarr.c"
  };


  /* Register protocol */
  proto_ranap = proto_register_protocol(PNAME, PSNAME, PFNAME);
  /* Register fields and subtrees */
  proto_register_field_array(proto_ranap, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));

  /* Register dissector */
  register_dissector("ranap", dissect_ranap, proto_ranap);

  /* Register dissector tables */
  ranap_ies_dissector_table = register_dissector_table("ranap.ies", "RANAP-PROTOCOL-IES", FT_UINT32, BASE_DEC);
  ranap_ies_p1_dissector_table = register_dissector_table("ranap.ies.pair.first", "RANAP-PROTOCOL-IES-PAIR FirstValue", FT_UINT32, BASE_DEC);
  ranap_ies_p2_dissector_table = register_dissector_table("ranap.ies.pair.second", "RANAP-PROTOCOL-IES-PAIR SecondValue", FT_UINT32, BASE_DEC);
  ranap_extension_dissector_table = register_dissector_table("ranap.extension", "RANAP-PROTOCOL-EXTENSION", FT_UINT32, BASE_DEC);
  ranap_proc_imsg_dissector_table = register_dissector_table("ranap.proc.imsg", "RANAP-ELEMENTARY-PROCEDURE InitiatingMessage", FT_UINT32, BASE_DEC);
  ranap_proc_sout_dissector_table = register_dissector_table("ranap.proc.sout", "RANAP-ELEMENTARY-PROCEDURE SuccessfulOutcome", FT_UINT32, BASE_DEC);
  ranap_proc_uout_dissector_table = register_dissector_table("ranap.proc.uout", "RANAP-ELEMENTARY-PROCEDURE UnsuccessfulOutcome", FT_UINT32, BASE_DEC);
  ranap_proc_out_dissector_table = register_dissector_table("ranap.proc.out", "RANAP-ELEMENTARY-PROCEDURE Outcome", FT_UINT32, BASE_DEC);

  nas_pdu_dissector_table = register_dissector_table("ranap.nas_pdu", "RANAP NAS PDU", FT_UINT8, BASE_DEC);

  ranap_module = prefs_register_protocol(proto_ranap, proto_reg_handoff_ranap);
  prefs_register_uint_preference(ranap_module, "sccp_ssn", "SCCP SSN for RANAP",
				 "The SCCP SubSystem Number for RANAP (default 142)", 10,
				 &global_ranap_sccp_ssn);
}


/*--- proto_reg_handoff_ranap ---------------------------------------*/
void
proto_reg_handoff_ranap(void)
{
	static gboolean initialized = FALSE;
	static dissector_handle_t ranap_handle;
	static gint local_ranap_sccp_ssn;

	if (!initialized) {
		ranap_handle = find_dissector("ranap");
		rrc_handle = find_dissector("rrc.s_to_trnc_cont");
		initialized = TRUE;
#include "packet-ranap-dis-tab.c"
	} else {
		dissector_delete("sccp.ssn", local_ranap_sccp_ssn, ranap_handle);
	}

	dissector_add("sccp.ssn", global_ranap_sccp_ssn, ranap_handle);
	local_ranap_sccp_ssn = global_ranap_sccp_ssn;
	/* Add heuristic dissector
	* Perhaps we want a preference whether the heuristic dissector
	* is or isn't enabled
	*/
	heur_dissector_add("sccp", dissect_sccp_ranap_heur, proto_ranap); 
	heur_dissector_add("sua", dissect_sccp_ranap_heur, proto_ranap); 
}


