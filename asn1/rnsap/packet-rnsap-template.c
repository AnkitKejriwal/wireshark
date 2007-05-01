/* packet-rnsap.c
 * Routines for dissecting Universal Mobile Telecommunications System (UMTS);
 * UTRAN Iur interface Radio Network Subsystem
 * Application Part (RNSAP) signalling
 * (3GPP TS 25.423 version 6.7.0 Release 6) packet dissection
 * Copyright 2005 - 2006, Anders Broman <anders.broman@ericsson.com>
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
 * Ref: 3GPP TS 25.423 version 6.7.0 Release 6
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glib.h>
#include <epan/packet.h>
#include <epan/conversation.h>

#include <stdio.h>
#include <string.h>

#include <epan/asn1.h>

#include "packet-ber.h"
#include "packet-per.h"
#include "packet-rnsap.h"
#include "packet-umts_rrc.h"
/*#include "packet-umts_rrc.h"*/

#define PNAME  "UTRAN Iur interface Radio Network Subsystem Application Part"
#define PSNAME "RNSAP"
#define PFNAME "rnsap"

#define SCCP_SSN_RNSAP 143

#define RNSAP_FDD 1

#define	RNSAP_ID_UNUSED_PROTOCOLIE_RNSAP_ID_216											 216
#define	RNSAP_ID_UNUSED_PROTOCOLIE_RNSAP_ID_218											 218
#define	RNSAP_ID_UNUSED_PROTOCOLIE_RNSAP_ID_219											 219
#define	RNSAP_ID_UNUSED_PROTOCOLIE_RNSAP_ID_223											 223
#define	RNSAP_ID_UNUSED_PROTOCOLIE_RNSAP_ID_226											 226
#define	RNSAP_ID_UNUSED_PROTOCOLIE_RNSAP_ID_228											 228
#define	RNSAP_ID_UNUSED_PROTOCOLIE_RNSAP_ID_324											 324
#define	RNSAP_ID_UNUSED_PROTOCOLIE_RNSAP_ID_229											 229
#define	RNSAP_ID_UNUSED_PROTOCOLIE_RNSAP_ID_29											 29
#define	RNSAP_ID_UNUSED_PROTOCOLIE_RNSAP_ID_225											 225
#define	RNSAP_ID_UNUSED_PROTOCOLIE_RNSAP_ID_246											 246
#define	RNSAP_ID_UNUSED_PROTOCOLIE_RNSAP_ID_277											 277
#define	RNSAP_ID_UNUSED_PROTOCOLIE_RNSAP_ID_247											 247
#define	RNSAP_ID_UNUSED_PROTOCOLIE_RNSAP_ID_295											 295
#define	RNSAP_ID_UNUSED_PROTOCOLIE_RNSAP_ID_248											 248
#define	RNSAP_ID_UNUSED_PROTOCOLIE_RNSAP_ID_253											 253
#define	RNSAP_ID_UNUSED_PROTOCOLIE_RNSAP_ID_522											 522
#define	RNSAP_ID_UNUSED_PROTOCOLIE_RNSAP_ID_523											 523

static dissector_handle_t rnsap_handle=NULL;

/* Initialize the protocol and registered fields */
static int proto_rnsap = -1;

static int hf_rnsap_pdu_length = -1;
static int hf_rnsap_IE_length = -1;
static int hf_rnsap_L3_DL_DCCH_Message_PDU = -1;

#include "packet-rnsap-hf.c"

/* Initialize the subtree pointers */
static int ett_rnsap = -1;
static int ett_rnsap_initiatingMessageValue = -1;
static int ett_rnsap_ProtocolIEValueValue = -1;
static int ett_rnsap_SuccessfulOutcomeValue = -1;
static int ett_rnsap_UnsuccessfulOutcomeValue = -1;

#include "packet-rnsap-ett.c"

/* Global variables */
static proto_tree *top_tree;
static guint32 ProcedureCode;
static guint32 ProtocolIE_ID;
static guint32 ddMode;

#define BYTE_ALIGN_OFFSET(offset)		\
	if(offset&0x07){			\
		offset=(offset&0xfffffff8)+8;	\
	}
#define RNSAP_FDD 1
/* Prodedure ID:s */

/* Protocol IE:s */



static int dissect_rnsap_InitiatingMessageValueValue(tvbuff_t *tvb, int offset, asn1_ctx_t *actx, proto_tree *tree);
static int dissect_rnsap_SuccessfulOutcomeValueValue(tvbuff_t *tvb, int offset, asn1_ctx_t *actx, proto_tree *tree);
static int dissect_rnsap_UnsuccessfulOutcomeValueValue(tvbuff_t *tvb, int offset, asn1_ctx_t *actx, proto_tree *tree);
static int dissect_rnsap_ProtocolIEValueValue(tvbuff_t *tvb, int offset, asn1_ctx_t *actx, proto_tree *tree);
#include "packet-rnsap-fn.c"


static int dissect_rnsap_InitiatingMessageValueValue(tvbuff_t *tvb, int offset, asn1_ctx_t *actx, proto_tree *tree){
	proto_item	*value_item = NULL;
	proto_tree	*value_tree = NULL;
	guint length;

	value_item = proto_tree_add_item(tree, hf_rnsap_initiatingMessageValue, tvb, 0, -1, FALSE);
	value_tree = proto_item_add_subtree(value_item, ett_rnsap_initiatingMessageValue);

	offset = dissect_per_length_determinant(tvb, offset, actx, value_tree, hf_rnsap_pdu_length, &length);
	proto_item_set_len(value_item,length);

	
	switch(ProcedureCode){
	case RNSAP_ID_COMMONTRANSPORTCHANNELRESOURCESINITIALISATION:	/* 0 */
		offset = dissect_id_commonTransportChannelResourcesInitialisation(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_COMMONTRANSPORTCHANNELRESOURCESRELEASE:			/* 1 */
		offset = dissect_id_commonTransportChannelResourcesRelease(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_COMPRESSEDMODECOMMAND:							 /* 2 */
		offset = dissect_id_compressedModeCommand(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DOWNLINKPOWERCONTROL:								 /* 3 */
		offset = dissect_id_downlinkPowerTimeslotControl(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DOWNLINKPOWERTIMESLOTCONTROL:						 /* 4 */
		offset = dissect_id_downlinkPowerTimeslotControl(tvb, offset, actx, value_tree);
		break;
		break;
	case RNSAP_ID_DOWNLINKSIGNALLINGTRANSFER:						 /* 5 */
		offset = dissect_id_downlinkSignallingTransfer(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_ERRORINDICATION:									 /* 6 */
		offset = dissect_id_errorIndication(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DEDICATEDMEASUREMENTFAILURE:						 /* 7 */
		offset = dissect_id_dedicatedMeasurementFailure(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DEDICATEDMEASUREMENTINITIATION:					 /* 8 */
		offset = dissect_id_dedicatedMeasurementInitiation(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DEDICATEDMEASUREMENTREPORTING:					 /* 9 */
		offset = dissect_id_dedicatedMeasurementReporting(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DEDICATEDMEASUREMENTTERMINATION:					 /* 10 */
		offset = dissect_id_dedicatedMeasurementTermination(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_PAGING:											 /* 11 */
		offset = dissect_id_paging(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_PHYSICALCHANNELRECONFIGURATION:					 /* 12 */
		offset = dissect_id_physicalChannelReconfiguration(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_PRIVATEMESSAGE:									 /* 13 */
		offset = dissect_id_privateMessage(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RADIOLINKADDITION:								 /* 14 */
		if (ddMode==RNSAP_FDD){
			offset = dissect_id_radioLinkAddition(tvb, offset, actx, value_tree);
		}else{
			offset = dissect_id_radioLinkAddition_TDD(tvb, offset, actx, value_tree);
		}
		break;
	case RNSAP_ID_RADIOLINKCONGESTION:								 /* 34 */
		offset = dissect_id_radioLinkCongestion(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RADIOLINKDELETION:								 /* 15 */
		offset = dissect_id_radioLinkDeletion(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RADIOLINKFAILURE:									 /* 16 */
		offset = dissect_id_radioLinkFailure(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RADIOLINKPREEMPTION:								 /* 17 */
		offset = dissect_id_radioLinkPreemption(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RADIOLINKRESTORATION:								 /* 18 */
		offset = dissect_id_radioLinkRestoration(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RADIOLINKSETUP:									 /* 19 */
		if (ddMode==RNSAP_FDD){
			offset = dissect_id_radioLinkSetup(tvb, offset, actx, value_tree);
		}else{
			offset = dissect_id_radioLinkSetupTdd(tvb, offset, actx, value_tree);
		}
		break;
	case RNSAP_ID_RELOCATIONCOMMIT:									 /* 20 */
		offset = dissect_id_relocationCommit(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_SYNCHRONISEDRADIOLINKRECONFIGURATIONCANCELLATION:	 /* 21 */
		offset = dissect_id_synchronisedRadioLinkReconfigurationCancellation(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_SYNCHRONISEDRADIOLINKRECONFIGURATIONCOMMIT:		 /* 22 */
		offset = dissect_id_synchronisedRadioLinkReconfigurationCommit(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_SYNCHRONISEDRADIOLINKRECONFIGURATIONPREPARATION:	 /* 23 */
		if (ddMode==RNSAP_FDD){
			offset = dissect_id_synchronisedRadioLinkReconfigurationPreparation(tvb, offset, actx, value_tree);
		}else{
			offset = dissect_id_synchronisedRadioLinkReconfigurationPreparation_TDD(tvb, offset, actx, value_tree);
		}
		break;
	case RNSAP_ID_UNSYNCHRONISEDRADIOLINKRECONFIGURATION:			 /* 24 */
		if (ddMode==RNSAP_FDD){
			offset = dissect_id_unSynchronisedRadioLinkReconfiguration(tvb, offset, actx, value_tree);
		}else{
			offset = dissect_id_unSynchronisedRadioLinkReconfiguration_TDD(tvb, offset, actx, value_tree);
		}
		break;
	case RNSAP_ID_UPLINKSIGNALLINGTRANSFER:							 /* 25 */
		if (ddMode==RNSAP_FDD){
			offset = dissect_id_uplinkSignallingTransfer(tvb, offset, actx, value_tree);
		}else{
			offset = dissect_id_uplinkSignallingTransfer_TDD(tvb, offset, actx, value_tree);
		}
		break;
	case RNSAP_ID_COMMONMEASUREMENTFAILURE:							 /* 26 */
		offset = dissect_id_commonMeasurementFailure(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_COMMONMEASUREMENTINITIATION:						 /* 27 */
		offset = dissect_id_commonMeasurementInitiation(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_COMMONMEASUREMENTREPORTING:						 /* 28 */
		offset = dissect_id_commonMeasurementReporting(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_COMMONMEASUREMENTTERMINATION:						 /* 29 */
		offset = dissect_id_commonMeasurementTermination(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_INFORMATIONEXCHANGEFAILURE:						 /* 30 */
		offset = dissect_id_informationExchangeFailure(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_INFORMATIONEXCHANGEINITIATION:					 /* 31 */
		offset = dissect_id_informationExchangeInitiation(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_INFORMATIONREPORTING:								 /* 32 */
		offset = dissect_id_informationReporting(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_INFORMATIONEXCHANGETERMINATION:					 /* 33 */
		offset = dissect_id_informationExchangeTermination(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RESET:											 /* 35 */
		offset = dissect_id_reset(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RADIOLINKACTIVATION: 								 /* 36 */
		if (ddMode==RNSAP_FDD){
			offset = dissect_id_radioLinkActivation(tvb, offset, actx, value_tree);
		}else{
			offset = dissect_id_radioLinkActivation_TDD(tvb, offset, actx, value_tree);
		}
		break;
	case RNSAP_ID_GERANUPLINKSIGNALLINGTRANSFER:					 /* 37 */
		offset = dissect_id_gERANuplinkSignallingTransfer(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RADIOLINKPARAMETERUPDATE:							 /* 38 */
		if (ddMode==RNSAP_FDD){
			offset = dissect_id_radioLinkParameterUpdate(tvb, offset, actx, value_tree);
		}else{
			offset = dissect_id_radioLinkParameterUpdate_TDD(tvb, offset, actx, value_tree);
		}
		break;
	case RNSAP_ID_UEMEASUREMENTFAILURE:								 /* 39 */
		offset = dissect_id_uEMeasurementFailure(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UEMEASUREMENTINITIATION:							 /* 40 */
		offset = dissect_id_uEMeasurementInitiation(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UEMEASUREMENTREPORTING:							 /* 41 */
		offset = dissect_id_uEMeasurementReporting(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UEMEASUREMENTTERMINATION:							 /* 42 */
		offset = dissect_id_uEMeasurementTermination(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_IURDEACTIVATETRACE:								 /* 43 */
		offset = dissect_id_iurDeactivateTrace(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_IURINVOKETRACE:									 /* 44 */
		offset = dissect_id_iurInvokeTrace(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_MBMSATTACH:										 /* 45 */
		offset = dissect_id_mBMSAttach(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_MBMSDETACH:										 /* 46 */
		offset = dissect_id_mBMSDetach(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DIRECTINFORMATIONTRANSFER:						 /* 48 */
		offset = dissect_id_directInformationTransfer(tvb, offset, actx, value_tree);
		break;
	default:
		offset = offset + (length<<3);
		break;
	}
	BYTE_ALIGN_OFFSET(offset)
	return offset;
}

static int dissect_rnsap_SuccessfulOutcomeValueValue(tvbuff_t *tvb, int offset, asn1_ctx_t *actx, proto_tree *tree){
	proto_item	*value_item = NULL;
	proto_tree	*value_tree = NULL;
	guint length;

	value_item = proto_tree_add_item(tree, hf_rnsap_successfulOutcomeValue, tvb, 0, -1, FALSE);
	value_tree = proto_item_add_subtree(value_item, ett_rnsap_initiatingMessageValue);

	offset = dissect_per_length_determinant(tvb, offset, actx, value_tree, hf_rnsap_pdu_length, &length);
	proto_item_set_len(value_item,length);

	
	switch(ProcedureCode){
	case RNSAP_ID_COMMONTRANSPORTCHANNELRESOURCESINITIALISATION:
		if (ddMode==RNSAP_FDD){
			offset = dissect_id_commonTransportChannelResourcesInitialisation1(tvb, offset, actx, value_tree);
		}else{
			offset = dissect_id_commonTransportChannelResourcesInitialisation_TDD(tvb, offset, actx, value_tree);
		}
		break;
	case RNSAP_ID_DEDICATEDMEASUREMENTINITIATION:					 /* 8 */
		offset = dissect_id_dedicatedMeasurementInitiation2(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_PHYSICALCHANNELRECONFIGURATION:					 /* 12 */
		offset = dissect_id_physicalChannelReconfiguration1(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RADIOLINKADDITION:								 /* 14 */
		if (ddMode==RNSAP_FDD){
			offset = dissect_id_radioLinkAddition1(tvb, offset, actx, value_tree);
		}else{
			offset = dissect_id_radioLinkAddition_TDD1(tvb, offset, actx, value_tree);
		}
		break;
	case RNSAP_ID_RADIOLINKDELETION:								 /* 15 */
		offset = dissect_id_radioLinkDeletion1(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RADIOLINKSETUP:									 /* 19 */
		if (ddMode==RNSAP_FDD){
			offset = dissect_id_radioLinkSetup1(tvb, offset, actx, value_tree);
		}else{
			offset = dissect_id_radioLinkSetupTdd1(tvb, offset, actx, value_tree);
		}
		break;
	case RNSAP_ID_SYNCHRONISEDRADIOLINKRECONFIGURATIONPREPARATION:	 /* 23 */
		if (ddMode==RNSAP_FDD){
			offset = dissect_id_synchronisedRadioLinkReconfigurationPreparation1(tvb, offset, actx, value_tree);
		}else{
			offset = dissect_id_synchronisedRadioLinkReconfigurationPreparation_TDD(tvb, offset, actx, value_tree);
		}
	case RNSAP_ID_UNSYNCHRONISEDRADIOLINKRECONFIGURATION:			 /* 24 */
		if (ddMode==RNSAP_FDD){
			offset = dissect_id_unSynchronisedRadioLinkReconfiguration1(tvb, offset, actx, value_tree);
		}else{
			offset = dissect_id_unSynchronisedRadioLinkReconfiguration_TDD1(tvb, offset, actx, value_tree);
		}
		break;
	case RNSAP_ID_COMMONMEASUREMENTINITIATION:						 /* 27 */
		offset = dissect_id_commonMeasurementInitiation1(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_INFORMATIONEXCHANGEINITIATION:					 /* 31 */
		offset = dissect_id_informationExchangeInitiation1(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RESET:											 /* 35 */
		offset = dissect_id_reset1(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UEMEASUREMENTINITIATION:							 /* 40 */
		offset = dissect_id_uEMeasurementInitiation1(tvb, offset, actx, value_tree);
		break;

	default:
		offset = offset + (length<<3);
		break;
	}
	BYTE_ALIGN_OFFSET(offset)
	return offset;
}

static int dissect_rnsap_UnsuccessfulOutcomeValueValue(tvbuff_t *tvb, int offset, asn1_ctx_t *actx, proto_tree *tree){
	proto_item	*value_item = NULL;
	proto_tree	*value_tree = NULL;
	guint length;

	value_item = proto_tree_add_item(tree, hf_rnsap_unsuccessfulOutcomeValue, tvb, 0, -1, FALSE);
	value_tree = proto_item_add_subtree(value_item, ett_rnsap_UnsuccessfulOutcomeValue);

	offset = dissect_per_length_determinant(tvb, offset, actx, value_tree, hf_rnsap_pdu_length, &length);
	proto_item_set_len(value_item,length);

	
	switch(ProcedureCode){
	case RNSAP_ID_COMMONTRANSPORTCHANNELRESOURCESINITIALISATION:
		offset = dissect_id_commonTransportChannelResourcesInitialisation2(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DEDICATEDMEASUREMENTINITIATION:					 /* 8 */
		offset = dissect_id_dedicatedMeasurementInitiation2(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_PHYSICALCHANNELRECONFIGURATION:					 /* 12 */
		offset = dissect_id_physicalChannelReconfiguration2(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RADIOLINKADDITION:								 /* 14 */
		if (ddMode==RNSAP_FDD){
			offset = dissect_id_radioLinkAddition2(tvb, offset, actx, value_tree);
		}else{
			offset = dissect_id_radioLinkAddition_TDD2(tvb, offset, actx, value_tree);
		}
		break;
	case RNSAP_ID_RADIOLINKSETUP:									 /* 19 */
		if (ddMode==RNSAP_FDD){
			offset = dissect_id_radioLinkSetup2(tvb, offset, actx, value_tree);
		}else{
			offset = dissect_id_radioLinkSetupTdd2(tvb, offset, actx, value_tree);
		}
		break;
	case RNSAP_ID_SYNCHRONISEDRADIOLINKRECONFIGURATIONPREPARATION:	 /* 23 */
		offset = dissect_id_synchronisedRadioLinkReconfigurationPreparation2(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UNSYNCHRONISEDRADIOLINKRECONFIGURATION:			 /* 24 */
		if (ddMode==RNSAP_FDD){
			offset = dissect_id_unSynchronisedRadioLinkReconfiguration2(tvb, offset, actx, value_tree);
		}else{
			offset = dissect_id_unSynchronisedRadioLinkReconfiguration_TDD2(tvb, offset, actx, value_tree);
		}
		break;
	case RNSAP_ID_COMMONMEASUREMENTINITIATION:						 /* 27 */
		offset = dissect_id_commonMeasurementInitiation2(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_INFORMATIONEXCHANGEINITIATION:					 /* 31 */
		offset = dissect_id_informationExchangeInitiation2(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UEMEASUREMENTINITIATION:							 /* 40 */
		offset = dissect_id_uEMeasurementInitiation2(tvb, offset, actx, value_tree);
		break;

	default:
		offset = offset + (length<<3);
		break;
	}
	BYTE_ALIGN_OFFSET(offset)
	return offset;
}

static int dissect_rnsap_ProtocolIEValueValue(tvbuff_t *tvb, int offset, asn1_ctx_t *actx, proto_tree *tree){
	proto_item	*value_item = NULL;
	proto_tree	*value_tree = NULL;
	guint length;

	value_item = proto_tree_add_item(tree, hf_rnsap_value, tvb, 0, -1, FALSE);
	value_tree = proto_item_add_subtree(value_item, ett_rnsap_ProtocolIEValueValue);

	offset = dissect_per_length_determinant(tvb, offset, actx, value_tree, hf_rnsap_IE_length, &length);
	proto_item_set_len(value_item,length);

	
	switch(ProtocolIE_ID){

	case RNSAP_ID_ALLOWEDQUEUINGTIME:											/*  4 */
		offset = dissect_id_AllowedQueuingTime(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_ALLOWED_RATE_INFORMATION:										/*  42 */
		offset = dissect_id_Allowed_Rate_Information(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_ANTENNACOLOCATIONINDICATOR:									/*  309 */
		offset = dissect_id_AntennaColocationIndicator(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_BINDINGID:													/*  5 */
		offset = dissect_id_BindingID(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_C_ID:															/*  6 */
		offset = dissect_id_C_ID(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_C_RNTI:														/*  7 */
		offset = dissect_id_C_RNTI(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_CELL_CAPACITY_CLASS_VALUE:									/*  303 */
		offset = dissect_id_Cell_Capacity_Class_Value(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_CFN:															/*  8 */
		offset = dissect_id_CFN(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_CN_CS_DOMAINIDENTIFIER:										/*  9 */
		offset = dissect_id_CN_CS_DomainIdentifier(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_CN_PS_DOMAINIDENTIFIER:										/*  10 */
		offset = dissect_id_CN_PS_DomainIdentifier(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_CAUSE:														/*  11 */
		offset = dissect_id_Cause(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_COVERAGEINDICATOR:											/*  310 */
		offset = dissect_id_CoverageIndicator(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_CRITICALITYDIAGNOSTICS:										/*  20 */
		offset = dissect_id_CriticalityDiagnostics(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_CONTEXTINFOITEM_RESET:										/*  211 */
		offset = offset + (length<<3);
		break;
	case RNSAP_ID_CONTEXTGROUPINFOITEM_RESET:									/*  515 */
		offset = offset + (length<<3);
		break;
	case RNSAP_ID_D_RNTI:														/*  21 */
		offset = dissect_id_D_RNTI(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_D_RNTI_RELEASEINDICATION:										/*  22 */
		offset = dissect_id_D_RNTI_ReleaseIndication(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DCHS_TO_ADD_FDD:												/*  26 */
		offset = dissect_id_DCHs_to_Add_FDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DCHS_TO_ADD_TDD:												/*  27 */
		offset = dissect_id_DCHs_to_Add_TDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DCH_DELETELIST_RL_RECONFPREPFDD:								/*  30 */
		offset = dissect_id_DCH_DeleteList_RL_ReconfPrepFDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DCH_DELETELIST_RL_RECONFPREPTDD:								/*  31 */
		offset = dissect_id_DCH_DeleteList_RL_ReconfPrepTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DCH_DELETELIST_RL_RECONFRQSTFDD:								/*  32 */
		offset = dissect_id_DCH_DeleteList_RL_ReconfRqstFDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DCH_DELETELIST_RL_RECONFRQSTTDD:								/*  33 */
		offset = dissect_id_DCH_DeleteList_RL_ReconfRqstTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DCH_FDD_INFORMATION:											/*  34 */
		offset = dissect_id_DCH_FDD_Information(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DCH_TDD_INFORMATION:											/*  35 */
		offset = dissect_id_DCH_TDD_Information(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_FDD_DCHS_TO_MODIFY:											/*  39 */
		offset = dissect_id_FDD_DCHs_to_Modify(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_TDD_DCHS_TO_MODIFY:											/*  40 */
		offset = dissect_id_TDD_DCHs_to_Modify(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DCH_INFORMATIONRESPONSE:										/*  43 */
		offset = dissect_id_DCH_InformationResponse(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DCH_RATE_INFORMATIONITEM_RL_CONGESTIND:						/*  38 */
		offset = dissect_id_DCH_Rate_InformationItem_RL_CongestInd(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_CCTRCH_INFORMATIONADDITEM_RL_RECONFPREPTDD:				/*  44 */
		offset = dissect_id_DL_CCTrCH_InformationAddItem_RL_ReconfPrepTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_CCTRCH_INFORMATIONLISTIE_RL_RECONFREADYTDD:				/*  45 */
		offset = dissect_id_DL_CCTrCH_InformationListIE_RL_ReconfReadyTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_CCTRCH_INFORMATIONDELETEITEM_RL_RECONFRQSTTDD:				/*  46 */
		offset = dissect_id_DL_CCTrCH_InformationDeleteItem_RL_ReconfRqstTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_CCTRCH_INFORMATIONITEM_RL_SETUPRQSTTDD:					/*  47 */
		offset = dissect_id_DL_CCTrCH_InformationItem_RL_SetupRqstTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_CCTRCH_INFORMATIONLISTIE_PHYCHRECONFRQSTTDD:				/*  48 */
		offset = dissect_id_DL_CCTrCH_InformationListIE_PhyChReconfRqstTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_CCTRCH_INFORMATIONLISTIE_RL_ADDITIONRSPTDD:				/*  49 */
		offset = dissect_id_DL_CCTrCH_InformationListIE_RL_AdditionRspTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_CCTRCH_INFORMATIONLISTIE_RL_SETUPRSPTDD:					/*  50 */
		offset = dissect_id_DL_CCTrCH_InformationListIE_RL_SetupRspTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_CCTRCH_INFORMATIONADDLIST_RL_RECONFPREPTDD:				/*  51 */
		offset = dissect_id_DL_CCTrCH_InformationAddList_RL_ReconfPrepTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_CCTRCH_INFORMATIONDELETELIST_RL_RECONFRQSTTDD:				/*  52 */
		offset = dissect_id_DL_CCTrCH_InformationDeleteList_RL_ReconfRqstTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_CCTRCH_INFORMATIONLIST_RL_SETUPRQSTTDD:					/*  53 */
		offset = dissect_id_DL_CCTrCH_InformationList_RL_SetupRqstTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_FDD_DL_CODEINFORMATION:										/*  54 */
		offset = dissect_id_FDD_DL_CodeInformation(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_DPCH_INFORMATION_RL_RECONFPREPFDD:							/*  59 */
		offset = dissect_id_DL_DPCH_Information_RL_ReconfPrepFDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_DPCH_INFORMATION_RL_SETUPRQSTFDD:							/*  60 */
		offset = dissect_id_DL_DPCH_Information_RL_SetupRqstFDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_DPCH_INFORMATION_RL_RECONFRQSTFDD:							/*  61 */
		offset = dissect_id_DL_DPCH_Information_RL_ReconfRqstFDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_DPCH_INFORMATIONITEM_PHYCHRECONFRQSTTDD:					/*  62 */
		offset = dissect_id_DL_DPCH_InformationItem_PhyChReconfRqstTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_DPCH_INFORMATIONITEM_RL_ADDITIONRSPTDD:					/*  63 */
		offset = dissect_id_DL_DPCH_InformationItem_RL_AdditionRspTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_DPCH_INFORMATIONITEM_RL_SETUPRSPTDD:						/*  64 */
		offset = dissect_id_DL_DPCH_InformationItem_RL_SetupRspTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_DPCH_TIMINGADJUSTMENT:										/*  278 */
		offset = dissect_id_DL_DPCH_TimingAdjustment(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DLREFERENCEPOWER:												/*  67 */
		offset = dissect_id_DLReferencePower(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DLREFERENCEPOWERLIST_DL_PC_RQST:								/*  68 */
		offset = dissect_id_DLReferencePowerList_DL_PC_Rqst(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_REFERENCEPOWERINFORMATION_DL_PC_RQST:						/*  69 */
		offset = dissect_id_DL_ReferencePowerInformation_DL_PC_Rqst(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DPC_MODE:														/*  12 */
		offset = dissect_id_DPC_Mode(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DRXCYCLELENGTHCOEFFICIENT:									/*  70 */
		offset = dissect_id_DRXCycleLengthCoefficient(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DEDICATEDMEASUREMENTOBJECTTYPE_DM_FAIL_IND:					/*  470 */
		offset = dissect_id_DedicatedMeasurementObjectType_DM_Fail_Ind(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DEDICATEDMEASUREMENTOBJECTTYPE_DM_FAIL:						/*  471 */
		offset = dissect_id_DedicatedMeasurementObjectType_DM_Fail(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DEDICATEDMEASUREMENTOBJECTTYPE_DM_RPRT:						/*  71 */
		offset = dissect_id_DedicatedMeasurementObjectType_DM_Rprt(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DEDICATEDMEASUREMENTOBJECTTYPE_DM_RQST:						/*  72 */
		offset = dissect_id_DedicatedMeasurementObjectType_DM_Rqst(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DEDICATEDMEASUREMENTOBJECTTYPE_DM_RSP:						/*  73 */
		offset = dissect_id_DedicatedMeasurementObjectType_DM_Rsp(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DEDICATEDMEASUREMENTTYPE:										/*  74 */
		offset = dissect_id_DedicatedMeasurementType(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_FACH_INFOFORUESELECTEDS_CCPCH_CTCH_RESOURCERSPFDD:			/*  82 */
		offset = dissect_id_FACH_InfoForUESelectedS_CCPCH_CTCH_ResourceRspFDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_FACH_INFOFORUESELECTEDS_CCPCH_CTCH_RESOURCERSPTDD:			/*  83 */
		offset = dissect_id_FACH_InfoForUESelectedS_CCPCH_CTCH_ResourceRspTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_GUARANTEED_RATE_INFORMATION:									/*  41 */
		offset = dissect_id_Guaranteed_Rate_Information(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_IMSI:															/*  84 */
		offset = dissect_id_IMSI(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_HCS_PRIO:														/*  311 */
		offset = dissect_id_HCS_Prio(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_L3_INFORMATION:												/*  85 */
		offset = dissect_id_L3_Information(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_ADJUSTMENTPERIOD:												/*  90 */
		offset = dissect_id_AdjustmentPeriod(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_MAXADJUSTMENTSTEP:											/*  91 */
		offset = dissect_id_MaxAdjustmentStep(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_MEASUREMENTFILTERCOEFFICIENT:									/*  92 */
		offset = dissect_id_MeasurementFilterCoefficient(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_MESSAGESTRUCTURE:												/*  57 */
		offset = dissect_id_MessageStructure(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_MEASUREMENTID:												/*  93 */
		offset = dissect_id_MeasurementID(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_NEIGHBOURING_GSM_CELLINFORMATION:								/*  13 */
		offset = dissect_id_Neighbouring_GSM_CellInformation(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_NEIGHBOURING_UMTS_CELLINFORMATIONITEM:						/*  95 */
		offset = dissect_id_Neighbouring_UMTS_CellInformationItem(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_NRT_LOAD_INFORMATION_VALUE:									/*  305 */
		offset = dissect_id_NRT_Load_Information_Value(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_NRT_LOAD_INFORMATION_VALUE_INCRDECRTHRES:						/*  306 */
		offset = dissect_id_NRT_Load_Information_Value_IncrDecrThres(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_PAGINGAREA_PAGINGRQST:										/*  102 */
		offset = dissect_id_PagingArea_PagingRqst(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_FACH_FLOWCONTROLINFORMATION:									/*  103 */
		offset = dissect_id_FACH_FlowControlInformation(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_PARTIALREPORTINGINDICATOR:									/*  472 */
		offset = dissect_id_PartialReportingIndicator(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_PERMANENT_NAS_UE_IDENTITY:									/*  17 */
		offset = dissect_id_Permanent_NAS_UE_Identity(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_POWERADJUSTMENTTYPE:											/*  107 */
		offset = dissect_id_PowerAdjustmentType(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RANAP_RELOCATIONINFORMATION:									/*  109 */
		offset = dissect_id_RANAP_RelocationInformation(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_INFORMATION_PHYCHRECONFRQSTFDD:							/*  110 */
		offset = dissect_id_RL_Information_PhyChReconfRqstFDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_INFORMATION_PHYCHRECONFRQSTTDD:							/*  111 */
		offset = dissect_id_RL_Information_PhyChReconfRqstTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_INFORMATION_RL_ADDITIONRQSTFDD:							/*  112 */
		offset = dissect_id_RL_Information_RL_AdditionRqstFDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_INFORMATION_RL_ADDITIONRQSTTDD:							/*  113 */
		offset = dissect_id_RL_Information_RL_AdditionRqstTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_INFORMATION_RL_DELETIONRQST:								/*  114 */
		offset = dissect_id_RL_Information_RL_DeletionRqst(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_INFORMATION_RL_FAILUREIND:									/*  115 */
		offset = dissect_id_RL_Information_RL_FailureInd(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_INFORMATION_RL_RECONFPREPFDD:								/*  116 */
		offset = dissect_id_RL_Information_RL_ReconfPrepFDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_INFORMATION_RL_RESTOREIND:									/*  117 */
		offset = dissect_id_RL_Information_RL_RestoreInd(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_INFORMATION_RL_SETUPRQSTFDD:								/*  118 */
		offset = dissect_id_RL_Information_RL_SetupRqstFDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_INFORMATION_RL_SETUPRQSTTDD:								/*  119 */
		offset = dissect_id_RL_Information_RL_SetupRqstTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_INFORMATIONITEM_RL_CONGESTIND:								/*  55 */
		offset = dissect_id_RL_InformationItem_RL_CongestInd(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_INFORMATIONITEM_DM_RPRT:									/*  120 */
		offset = dissect_id_RL_InformationItem_DM_Rprt(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_INFORMATIONITEM_DM_RQST:									/*  121 */
		offset = dissect_id_RL_InformationItem_DM_Rqst(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_INFORMATIONITEM_DM_RSP:									/*  122 */
		offset = dissect_id_RL_InformationItem_DM_Rsp(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_INFORMATIONITEM_RL_PREEMPTREQUIREDIND:						/*  2 */
		offset = dissect_id_RL_InformationItem_RL_PreemptRequiredInd(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_INFORMATIONITEM_RL_SETUPRQSTFDD:							/*  123 */
		offset = dissect_id_RL_InformationItem_RL_SetupRqstFDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_INFORMATIONLIST_RL_CONGESTIND:								/*  56 */
		offset = dissect_id_RL_InformationList_RL_CongestInd(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_INFORMATIONLIST_RL_ADDITIONRQSTFDD:						/*  124 */
		offset = dissect_id_RL_InformationList_RL_AdditionRqstFDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_INFORMATIONLIST_RL_DELETIONRQST:							/*  125 */
		offset = dissect_id_RL_InformationList_RL_DeletionRqst(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_INFORMATIONLIST_RL_PREEMPTREQUIREDIND:						/*  1 */
		offset = dissect_id_RL_InformationList_RL_PreemptRequiredInd(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_INFORMATIONLIST_RL_RECONFPREPFDD:							/*  126 */
		offset = dissect_id_RL_InformationList_RL_ReconfPrepFDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_INFORMATIONRESPONSE_RL_ADDITIONRSPTDD:						/*  127 */
		offset = dissect_id_RL_InformationResponse_RL_AdditionRspTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_INFORMATIONRESPONSE_RL_RECONFREADYTDD:						/*  128 */
		offset = dissect_id_RL_InformationResponse_RL_ReconfReadyTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_INFORMATIONRESPONSE_RL_SETUPRSPTDD:						/*  129 */
		offset = dissect_id_RL_InformationResponse_RL_SetupRspTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_INFORMATIONRESPONSEITEM_RL_ADDITIONRSPFDD:					/*  130 */
		offset = dissect_id_RL_InformationResponseItem_RL_AdditionRspFDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_INFORMATIONRESPONSEITEM_RL_RECONFREADYFDD:					/*  131 */
		offset = dissect_id_RL_InformationResponseItem_RL_ReconfReadyFDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_INFORMATIONRESPONSEITEM_RL_RECONFRSPFDD:					/*  132 */
		offset = dissect_id_RL_InformationResponseItem_RL_ReconfRspFDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_INFORMATIONRESPONSEITEM_RL_SETUPRSPFDD:					/*  133 */
		offset = dissect_id_RL_InformationResponseItem_RL_SetupRspFDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_INFORMATIONRESPONSELIST_RL_ADDITIONRSPFDD:					/*  134 */
		offset = dissect_id_RL_InformationResponseList_RL_AdditionRspFDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_INFORMATIONRESPONSELIST_RL_RECONFREADYFDD:					/*  135 */
		offset = dissect_id_RL_InformationResponseList_RL_ReconfReadyFDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_INFORMATIONRESPONSELIST_RL_RECONFRSPFDD:					/*  136 */
		offset = dissect_id_RL_InformationResponseList_RL_ReconfRspFDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_INFORMATIONRESPONSE_RL_RECONFRSPTDD:						/*  28 */
		offset = dissect_id_RL_InformationResponse_RL_ReconfRspTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_INFORMATIONRESPONSELIST_RL_SETUPRSPFDD:					/*  137 */
		offset = dissect_id_RL_InformationResponseList_RL_SetupRspFDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_RECONFIGURATIONFAILURE_RL_RECONFFAIL:						/*  141 */
		offset = dissect_id_RL_ReconfigurationFailure_RL_ReconfFail(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_SET_INFORMATIONITEM_DM_RPRT:								/*  143 */
		offset = dissect_id_RL_Set_InformationItem_DM_Rprt(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_SET_INFORMATIONITEM_DM_RQST:								/*  144 */
		offset = dissect_id_RL_Set_InformationItem_DM_Rqst(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_SET_INFORMATIONITEM_DM_RSP:								/*  145 */
		offset = dissect_id_RL_Set_InformationItem_DM_Rsp(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_SET_INFORMATION_RL_FAILUREIND:								/*  146 */
		offset = dissect_id_RL_Set_Information_RL_FailureInd(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_SET_INFORMATION_RL_RESTOREIND:								/*  147 */
		offset = dissect_id_RL_Set_Information_RL_RestoreInd(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_SET_SUCCESSFUL_INFORMATIONITEM_DM_FAIL:					/*  473 */
		offset = dissect_id_RL_Set_Successful_InformationItem_DM_Fail(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_SET_UNSUCCESSFUL_INFORMATIONITEM_DM_FAIL:					/*  474 */
		offset = dissect_id_RL_Set_Unsuccessful_InformationItem_DM_Fail(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_SET_UNSUCCESSFUL_INFORMATIONITEM_DM_FAIL_IND:				/*  475 */
		offset = dissect_id_RL_Set_Unsuccessful_InformationItem_DM_Fail_Ind(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_SUCCESSFUL_INFORMATIONITEM_DM_FAIL:						/*  476 */
		offset = dissect_id_RL_Successful_InformationItem_DM_Fail(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_UNSUCCESSFUL_INFORMATIONITEM_DM_FAIL:						/*  477 */
		offset = dissect_id_RL_Unsuccessful_InformationItem_DM_Fail(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_UNSUCCESSFUL_INFORMATIONITEM_DM_FAIL_IND:					/*  478 */
		offset = dissect_id_RL_Unsuccessful_InformationItem_DM_Fail_Ind(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_REPORTCHARACTERISTICS:										/*  152 */
		offset = dissect_id_ReportCharacteristics(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_REPORTING_OBJECT_RL_FAILUREIND:								/*  153 */
		offset = dissect_id_Reporting_Object_RL_FailureInd(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_REPORING_OBJECT_RL_RESTOREIND:								/*  154 */
		offset = dissect_id_Reporing_Object_RL_RestoreInd(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RT_LOAD_VALUE:												/*  307 */
		offset = dissect_id_RT_Load_Value(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RT_LOAD_VALUE_INCRDECRTHRES:									/*  308 */
		offset = dissect_id_RT_Load_Value_IncrDecrThres(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_S_RNTI:														/*  155 */
		offset = dissect_id_S_RNTI(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RESETINDICATOR:												/*  244 */
		offset = dissect_id_ResetIndicator(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RNC_ID:														/*  245 */
		offset = dissect_id_RNC_ID(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_SAI:															/*  156 */
		offset = dissect_id_SAI(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_SRNC_ID:														/*  157 */
		offset = dissect_id_SRNC_ID(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_SUCCESSFULRL_INFORMATIONRESPONSE_RL_ADDITIONFAILUREFDD:		/*  159 */
		offset = dissect_id_SuccessfulRL_InformationResponse_RL_AdditionFailureFDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_SUCCESSFULRL_INFORMATIONRESPONSE_RL_SETUPFAILUREFDD:			/*  160 */
		offset = dissect_id_SuccessfulRL_InformationResponse_RL_SetupFailureFDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_TRANSPORTBEARERID:											/*  163 */
		offset = dissect_id_TransportBearerID(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_TRANSPORTBEARERREQUESTINDICATOR:								/*  164 */
		offset = dissect_id_TransportBearerRequestIndicator(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_TRANSPORTLAYERADDRESS:										/*  165 */
		offset = dissect_id_TransportLayerAddress(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_TYPEOFERROR:													/*  140 */
		offset = dissect_id_TypeOfError(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UC_ID:														/*  166 */
		offset = dissect_id_UC_ID(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UL_CCTRCH_ADDINFORMATION_RL_RECONFPREPTDD:					/*  167 */
		offset = dissect_id_UL_CCTrCH_AddInformation_RL_ReconfPrepTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UL_CCTRCH_INFORMATIONADDLIST_RL_RECONFPREPTDD:				/*  169 */
		offset = dissect_id_UL_CCTrCH_InformationAddList_RL_ReconfPrepTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UL_CCTRCH_INFORMATIONITEM_RL_SETUPRQSTTDD:					/*  171 */
		offset = dissect_id_UL_CCTrCH_InformationItem_RL_SetupRqstTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UL_CCTRCH_INFORMATIONLIST_RL_SETUPRQSTTDD:					/*  172 */
		offset = dissect_id_UL_CCTrCH_InformationList_RL_SetupRqstTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UL_CCTRCH_INFORMATIONLISTIE_PHYCHRECONFRQSTTDD:				/*  173 */
		offset = dissect_id_UL_CCTrCH_InformationListIE_PhyChReconfRqstTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UL_CCTRCH_INFORMATIONLISTIE_RL_ADDITIONRSPTDD:				/*  174 */
		offset = dissect_id_UL_CCTrCH_InformationListIE_RL_AdditionRspTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UL_CCTRCH_INFORMATIONLISTIE_RL_RECONFREADYTDD:				/*  175 */
		offset = dissect_id_UL_CCTrCH_InformationListIE_RL_ReconfReadyTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UL_CCTRCH_INFORMATIONLISTIE_RL_SETUPRSPTDD:					/*  176 */
		offset = dissect_id_UL_CCTrCH_InformationListIE_RL_SetupRspTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UL_DPCH_INFORMATION_RL_RECONFPREPFDD:							/*  177 */
		offset = dissect_id_UL_DPCH_Information_RL_ReconfPrepFDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UL_DPCH_INFORMATION_RL_RECONFRQSTFDD:							/*  178 */
		offset = dissect_id_UL_DPCH_Information_RL_ReconfRqstFDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UL_DPCH_INFORMATION_RL_SETUPRQSTFDD:							/*  179 */
		offset = dissect_id_UL_DPCH_Information_RL_SetupRqstFDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UL_DPCH_INFORMATIONITEM_PHYCHRECONFRQSTTDD:					/*  180 */
		offset = dissect_id_UL_DPCH_InformationItem_PhyChReconfRqstTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UL_DPCH_INFORMATIONITEM_RL_ADDITIONRSPTDD:					/*  181 */
		offset = dissect_id_UL_DPCH_InformationItem_RL_AdditionRspTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UL_DPCH_INFORMATIONITEM_RL_SETUPRSPTDD:						/*  182 */
		offset = dissect_id_UL_DPCH_InformationItem_RL_SetupRspTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UL_DPCH_INFORMATIONADDLISTIE_RL_RECONFREADYTDD:				/*  183 */
		offset = dissect_id_UL_DPCH_InformationAddListIE_RL_ReconfReadyTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UL_SIRTARGET:													/*  184 */
		offset = dissect_id_UL_SIRTarget(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_URA_INFORMATION:												/*  185 */
		offset = dissect_id_URA_Information(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UNSUCCESSFULRL_INFORMATIONRESPONSE_RL_ADDITIONFAILUREFDD:		/*  188 */
		offset = dissect_id_UnsuccessfulRL_InformationResponse_RL_AdditionFailureFDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UNSUCCESSFULRL_INFORMATIONRESPONSE_RL_SETUPFAILUREFDD:		/*  189 */
		offset = dissect_id_UnsuccessfulRL_InformationResponse_RL_SetupFailureFDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UNSUCCESSFULRL_INFORMATIONRESPONSE_RL_SETUPFAILURETDD:		/*  190 */
		offset = dissect_id_UnsuccessfulRL_InformationResponse_RL_SetupFailureTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_ACTIVE_PATTERN_SEQUENCE_INFORMATION:							/*  193 */
		offset = dissect_id_Active_Pattern_Sequence_Information(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_ADJUSTMENTRATIO:												/*  194 */
		offset = dissect_id_AdjustmentRatio(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_CAUSELEVEL_RL_ADDITIONFAILUREFDD:								/*  197 */
		offset = dissect_id_CauseLevel_RL_AdditionFailureFDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_CAUSELEVEL_RL_ADDITIONFAILURETDD:								/*  198 */
		offset = dissect_id_CauseLevel_RL_AdditionFailureTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_CAUSELEVEL_RL_RECONFFAILURE:									/*  199 */
		offset = dissect_id_CauseLevel_RL_ReconfFailure(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_CAUSELEVEL_RL_SETUPFAILUREFDD:								/*  200 */
		offset = dissect_id_CauseLevel_RL_SetupFailureFDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_CAUSELEVEL_RL_SETUPFAILURETDD:								/*  201 */
		offset = dissect_id_CauseLevel_RL_SetupFailureTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_CCTRCH_INFORMATIONDELETEITEM_RL_RECONFPREPTDD:				/*  205 */
		offset = dissect_id_DL_CCTrCH_InformationDeleteItem_RL_ReconfPrepTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_CCTRCH_INFORMATIONMODIFYITEM_RL_RECONFPREPTDD:				/*  206 */
		offset = dissect_id_DL_CCTrCH_InformationModifyItem_RL_ReconfPrepTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_CCTRCH_INFORMATIONMODIFYITEM_RL_RECONFRQSTTDD:				/*  207 */
		offset = dissect_id_DL_CCTrCH_InformationModifyItem_RL_ReconfRqstTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_CCTRCH_INFORMATIONDELETELIST_RL_RECONFPREPTDD:				/*  208 */
		offset = dissect_id_DL_CCTrCH_InformationDeleteList_RL_ReconfPrepTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_CCTRCH_INFORMATIONMODIFYLIST_RL_RECONFPREPTDD:				/*  209 */
		offset = dissect_id_DL_CCTrCH_InformationModifyList_RL_ReconfPrepTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_CCTRCH_INFORMATIONMODIFYLIST_RL_RECONFRQSTTDD:				/*  210 */
		offset = dissect_id_DL_CCTrCH_InformationModifyList_RL_ReconfRqstTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_DPCH_INFORMATIONADDLISTIE_RL_RECONFREADYTDD:				/*  212 */
		offset = dissect_id_DL_DPCH_InformationAddListIE_RL_ReconfReadyTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_DPCH_INFORMATIONDELETELISTIE_RL_RECONFREADYTDD:			/*  213 */
		offset = dissect_id_DL_DPCH_InformationDeleteListIE_RL_ReconfReadyTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_DPCH_INFORMATIONMODIFYLISTIE_RL_RECONFREADYTDD:			/*  214 */
		offset = dissect_id_DL_DPCH_InformationModifyListIE_RL_ReconfReadyTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DSCHS_TO_ADD_TDD:												/*  215 */
		offset = dissect_id_DSCHs_to_Add_TDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UNUSED_PROTOCOLIE_RNSAP_ID_216:								/*  216 */
		break;
	case RNSAP_ID_DSCH_DELETELIST_RL_RECONFPREPTDD:								/*  217 */
		offset = dissect_id_DSCH_DeleteList_RL_ReconfPrepTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UNUSED_PROTOCOLIE_RNSAP_ID_218:								/*  218 */
		break;
	case RNSAP_ID_UNUSED_PROTOCOLIE_RNSAP_ID_219:								/*  219 */
		break;
	case RNSAP_ID_DSCH_INFORMATIONLISTIE_RL_ADDITIONRSPTDD:						/*  220 */
		offset = dissect_id_DSCH_InformationListIE_RL_AdditionRspTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DSCH_INFORMATIONLISTIES_RL_SETUPRSPTDD:						/*  221 */
		offset = dissect_id_DSCH_InformationListIEs_RL_SetupRspTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DSCH_TDD_INFORMATION:											/*  222 */
		offset = dissect_id_DSCH_TDD_Information(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UNUSED_PROTOCOLIE_RNSAP_ID_223:								/*  223 */
	case RNSAP_ID_UNUSED_PROTOCOLIE_RNSAP_ID_226:								/*  226 */
		break;
	case RNSAP_ID_DSCH_MODIFYLIST_RL_RECONFPREPTDD:								/*  227 */
		offset = dissect_id_DSCH_ModifyList_RL_ReconfPrepTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UNUSED_PROTOCOLIE_RNSAP_ID_228:								/*  228 */
	case RNSAP_ID_UNUSED_PROTOCOLIE_RNSAP_ID_324:								/*  324 */
	case RNSAP_ID_UNUSED_PROTOCOLIE_RNSAP_ID_229:								/*  229 */
		break;
	case RNSAP_ID_DSCHTOBEADDEDORMODIFIEDLIST_RL_RECONFREADYTDD:				/*  230 */
		offset = dissect_id_DSCHToBeAddedOrModifiedList_RL_ReconfReadyTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UNUSED_PROTOCOLIE_RNSAP_ID_29:								/*  29 */
	case RNSAP_ID_UNUSED_PROTOCOLIE_RNSAP_ID_225:								/*  225 */
		break;
	case RNSAP_ID_GA_CELL:														/*  232 */
		offset = dissect_id_GA_Cell(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_GA_CELLADDITIONALSHAPES:										/*  3 */
		offset = dissect_id_GA_CellAdditionalShapes(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UNUSED_PROTOCOLIE_RNSAP_ID_246:								/*  246 */
		break;
	case RNSAP_ID_TRANSMISSION_GAP_PATTERN_SEQUENCE_INFORMATION:				/*  255 */
		offset = dissect_id_Transmission_Gap_Pattern_Sequence_Information(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UL_CCTRCH_DELETEINFORMATION_RL_RECONFPREPTDD:					/*  256 */
		offset = dissect_id_UL_CCTrCH_DeleteInformation_RL_ReconfPrepTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UL_CCTRCH_MODIFYINFORMATION_RL_RECONFPREPTDD:					/*  257 */
		offset = dissect_id_UL_CCTrCH_ModifyInformation_RL_ReconfPrepTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UL_CCTRCH_INFORMATIONMODIFYITEM_RL_RECONFRQSTTDD:				/*  258 */
		offset = dissect_id_UL_CCTrCH_InformationModifyItem_RL_ReconfRqstTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UL_CCTRCH_INFORMATIONDELETELIST_RL_RECONFPREPTDD:				/*  259 */
		offset = dissect_id_UL_CCTrCH_InformationDeleteList_RL_ReconfPrepTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UL_CCTRCH_INFORMATIONMODIFYLIST_RL_RECONFPREPTDD:				/*  260 */
		offset = dissect_id_UL_CCTrCH_InformationModifyList_RL_ReconfPrepTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UL_CCTRCH_INFORMATIONMODIFYLIST_RL_RECONFRQSTTDD:				/*  261 */
		offset = dissect_id_UL_CCTrCH_InformationModifyList_RL_ReconfRqstTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UL_CCTRCH_INFORMATIONDELETEITEM_RL_RECONFRQSTTDD:				/*  262 */
		offset = dissect_id_UL_CCTrCH_InformationDeleteItem_RL_ReconfRqstTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UL_CCTRCH_INFORMATIONDELETELIST_RL_RECONFRQSTTDD:				/*  263 */
		offset = dissect_id_UL_CCTrCH_InformationDeleteList_RL_ReconfRqstTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UL_DPCH_INFORMATIONDELETELISTIE_RL_RECONFREADYTDD:			/*  264 */
		offset = dissect_id_UL_DPCH_InformationDeleteListIE_RL_ReconfReadyTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UL_DPCH_INFORMATIONMODIFYLISTIE_RL_RECONFREADYTDD:			/*  265 */
		offset = dissect_id_UL_DPCH_InformationModifyListIE_RL_ReconfReadyTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UNSUCCESSFULRL_INFORMATIONRESPONSE_RL_ADDITIONFAILURETDD:		/*  266 */
		offset = dissect_id_UnsuccessfulRL_InformationResponse_RL_AdditionFailureTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_USCHS_TO_ADD:													/*  267 */
		offset = dissect_id_USCHs_to_Add(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_USCH_DELETELIST_RL_RECONFPREPTDD:								/*  268 */
		offset = dissect_id_USCH_DeleteList_RL_ReconfPrepTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_USCH_INFORMATIONLISTIE_RL_ADDITIONRSPTDD:						/*  269 */
		offset = dissect_id_USCH_InformationListIE_RL_AdditionRspTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_USCH_INFORMATIONLISTIES_RL_SETUPRSPTDD:						/*  270 */
		offset = dissect_id_USCH_InformationListIEs_RL_SetupRspTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_USCH_INFORMATION:												/*  271 */
		offset = dissect_id_USCH_Information(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_USCH_MODIFYLIST_RL_RECONFPREPTDD:								/*  272 */
		offset = dissect_id_USCH_ModifyList_RL_ReconfPrepTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_USCHTOBEADDEDORMODIFIEDLIST_RL_RECONFREADYTDD:				/*  273 */
		offset = dissect_id_USCHToBeAddedOrModifiedList_RL_ReconfReadyTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_PHYSICAL_CHANNEL_INFORMATION_RL_SETUPRQSTTDD:				/*  274 */
		offset = dissect_id_DL_Physical_Channel_Information_RL_SetupRqstTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UL_PHYSICAL_CHANNEL_INFORMATION_RL_SETUPRQSTTDD:				/*  275 */
		offset = dissect_id_UL_Physical_Channel_Information_RL_SetupRqstTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_CLOSEDLOOPMODE1_SUPPORTINDICATOR:								/*  276 */
		offset = dissect_id_ClosedLoopMode1_SupportIndicator(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UNUSED_PROTOCOLIE_RNSAP_ID_277:								/*  277 */
		break;
	case RNSAP_ID_STTD_SUPPORTINDICATOR:										/*  279 */
		offset = dissect_id_STTD_SupportIndicator(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_CFNREPORTINGINDICATOR:   										/*  14 */
		offset = dissect_id_CFNReportingIndicator(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_CNORIGINATEDPAGE_PAGINGRQST:									/*  23 */
		offset = dissect_id_CNOriginatedPage_PagingRqst(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_INNERLOOPDLPCSTATUS:											/*  24 */
		offset = dissect_id_InnerLoopDLPCStatus(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_PROPAGATIONDELAY:												/*  25 */
		offset = dissect_id_PropagationDelay(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RXTIMINGDEVIATIONFORTA:										/*  36 */
		offset = dissect_id_RxTimingDeviationForTA(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_TIMESLOT_ISCP:												/*  37 */
		offset = dissect_id_timeSlot_ISCP(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_CCTRCH_INFORMATIONITEM_RL_FAILUREIND:							/*  15 */
		offset = dissect_id_CCTrCH_InformationItem_RL_FailureInd(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_CCTRCH_INFORMATIONITEM_RL_RESTOREIND:							/*  16 */
		offset = dissect_id_CCTrCH_InformationItem_RL_RestoreInd(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_COMMONMEASUREMENTACCURACY:									/*  280 */
		offset = dissect_id_CommonMeasurementAccuracy(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_COMMONMEASUREMENTOBJECTTYPE_CM_RPRT:							/*  281 */
		offset = dissect_id_CommonMeasurementObjectType_CM_Rprt(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_COMMONMEASUREMENTOBJECTTYPE_CM_RQST:							/*  282 */
		offset = dissect_id_CommonMeasurementObjectType_CM_Rqst(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_COMMONMEASUREMENTOBJECTTYPE_CM_RSP:							/*  283 */
		offset = dissect_id_CommonMeasurementObjectType_CM_Rsp(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_COMMONMEASUREMENTTYPE:										/*  284 */
		offset = dissect_id_CommonMeasurementType(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_CONGESTIONCAUSE:												/*  18 */
		offset = dissect_id_CongestionCause(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_SFN:															/*  285 */
		offset = dissect_id_SFN(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_SFNREPORTINGINDICATOR:										/*  286 */
		offset = dissect_id_SFNReportingIndicator(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_INFORMATIONEXCHANGEID:										/*  287 */
		offset = dissect_id_InformationExchangeID(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_INFORMATIONEXCHANGEOBJECTTYPE_INFEX_RPRT:						/*  288 */
		offset = dissect_id_InformationExchangeObjectType_InfEx_Rprt(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_INFORMATIONEXCHANGEOBJECTTYPE_INFEX_RQST:						/*  289 */
		offset = dissect_id_InformationExchangeObjectType_InfEx_Rqst(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_INFORMATIONEXCHANGEOBJECTTYPE_INFEX_RSP:						/*  290 */
		offset = dissect_id_InformationExchangeObjectType_InfEx_Rsp(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_INFORMATIONREPORTCHARACTERISTICS:								/*  291 */
		offset = dissect_id_InformationReportCharacteristics(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_INFORMATIONTYPE:												/*  292 */
		offset = dissect_id_InformationType(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_NEIGHBOURING_LCR_TDD_CELLINFORMATION:							/*  58 */
		offset = dissect_id_neighbouring_LCR_TDD_CellInformation(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_TIMESLOT_ISCP_LCR_INFORMATION_RL_SETUPRQSTTDD:				/*  65 */
		offset = dissect_id_DL_Timeslot_ISCP_LCR_Information_RL_SetupRqstTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_LCR_INFORMATIONRESPONSE_RL_SETUPRSPTDD:					/*  66 */
		offset = dissect_id_RL_LCR_InformationResponse_RL_SetupRspTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UL_CCTRCH_LCR_INFORMATIONLISTIE_RL_SETUPRSPTDD:				/*  75 */
		offset = dissect_id_UL_CCTrCH_LCR_InformationListIE_RL_SetupRspTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UL_DPCH_LCR_INFORMATIONITEM_RL_SETUPRSPTDD:					/*  76 */
		offset = dissect_id_UL_DPCH_LCR_InformationItem_RL_SetupRspTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_CCTRCH_LCR_INFORMATIONLISTIE_RL_SETUPRSPTDD:				/*  77 */
		offset = dissect_id_DL_CCTrCH_LCR_InformationListIE_RL_SetupRspTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_DPCH_LCR_INFORMATIONITEM_RL_SETUPRSPTDD:					/*  78 */
		offset = dissect_id_DL_DPCH_LCR_InformationItem_RL_SetupRspTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DSCH_LCR_INFORMATIONLISTIES_RL_SETUPRSPTDD:					/*  79 */
		offset = dissect_id_DSCH_LCR_InformationListIEs_RL_SetupRspTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_USCH_LCR_INFORMATIONLISTIES_RL_SETUPRSPTDD:					/*  80 */
		offset = dissect_id_USCH_LCR_InformationListIEs_RL_SetupRspTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_TIMESLOT_ISCP_LCR_INFORMATION_RL_ADDITIONRQSTTDD:			/*  81 */
		offset = dissect_id_DL_Timeslot_ISCP_LCR_Information_RL_AdditionRqstTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_LCR_INFORMATIONRESPONSE_RL_ADDITIONRSPTDD:					/*  86 */
		offset = dissect_id_RL_LCR_InformationResponse_RL_AdditionRspTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UL_CCTRCH_LCR_INFORMATIONLISTIE_RL_ADDITIONRSPTDD:			/*  87 */
		offset = dissect_id_UL_CCTrCH_LCR_InformationListIE_RL_AdditionRspTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UL_DPCH_LCR_INFORMATIONITEM_RL_ADDITIONRSPTDD:				/*  88 */
		offset = dissect_id_UL_DPCH_LCR_InformationItem_RL_AdditionRspTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_CCTRCH_LCR_INFORMATIONLISTIE_RL_ADDITIONRSPTDD:			/*  89 */
		offset = dissect_id_DL_CCTrCH_LCR_InformationListIE_RL_AdditionRspTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_DPCH_LCR_INFORMATIONITEM_RL_ADDITIONRSPTDD:				/*  94 */
		offset = dissect_id_DL_DPCH_LCR_InformationItem_RL_AdditionRspTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DSCH_LCR_INFORMATIONLISTIES_RL_ADDITIONRSPTDD:				/*  96 */
		offset = dissect_id_DSCH_LCR_InformationListIEs_RL_AdditionRspTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_USCH_LCR_INFORMATIONLISTIES_RL_ADDITIONRSPTDD:				/*  97 */
		offset = dissect_id_USCH_LCR_InformationListIEs_RL_AdditionRspTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UL_DPCH_LCR_INFORMATIONADDLISTIE_RL_RECONFREADYTDD:			/*  98 */
		offset = dissect_id_UL_DPCH_LCR_InformationAddListIE_RL_ReconfReadyTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UL_TIMESLOT_LCR_INFORMATIONMODIFYLIST_RL_RECONFREADYTDD:		/*  100 */
		offset = dissect_id_UL_Timeslot_LCR_InformationModifyList_RL_ReconfReadyTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_DPCH_LCR_INFORMATIONADDLISTIE_RL_RECONFREADYTDD:			/*  101 */
		offset = dissect_id_DL_DPCH_LCR_InformationAddListIE_RL_ReconfReadyTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_TIMESLOT_LCR_INFORMATIONMODIFYLIST_RL_RECONFREADYTDD:		/*  104 */
		offset = dissect_id_DL_Timeslot_LCR_InformationModifyList_RL_ReconfReadyTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UL_TIMESLOT_LCR_INFORMATIONLIST_PHYCHRECONFRQSTTDD:			/*  105 */
		offset = dissect_id_UL_Timeslot_LCR_InformationList_PhyChReconfRqstTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_TIMESLOT_LCR_INFORMATIONLIST_PHYCHRECONFRQSTTDD:			/*  106 */
		offset = dissect_id_DL_Timeslot_LCR_InformationList_PhyChReconfRqstTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_TIMESLOT_ISCP_LCR_LIST_DL_PC_RQST_TDD:						/*  138 */
		offset = dissect_id_timeSlot_ISCP_LCR_List_DL_PC_Rqst_TDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_TSTD_SUPPORT_INDICATOR_RL_SETUPRQSTTDD:						/*  139 */
		offset = dissect_id_TSTD_Support_Indicator_RL_SetupRqstTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RESTRICTIONSTATEINDICATOR:									/*  142 */
		offset = dissect_id_RestrictionStateIndicator(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_LOAD_VALUE:													/*  233 */
		offset = dissect_id_Load_Value(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_LOAD_VALUE_INCRDECRTHRES:										/*  234 */
		offset = dissect_id_Load_Value_IncrDecrThres(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_ONMODIFICATION:												/*  235 */
		offset = dissect_id_OnModification(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RECEIVED_TOTAL_WIDEBAND_POWER_VALUE:							/*  236 */
		offset = dissect_id_Received_Total_Wideband_Power_Value(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RECEIVED_TOTAL_WIDEBAND_POWER_VALUE_INCRDECRTHRES:			/*  237 */
		offset = dissect_id_Received_Total_Wideband_Power_Value_IncrDecrThres(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_SFNSFNMEASUREMENTTHRESHOLDINFORMATION:						/*  238 */
		offset = dissect_id_SFNSFNMeasurementThresholdInformation(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_TRANSMITTED_CARRIER_POWER_VALUE:								/*  239 */
		offset = dissect_id_Transmitted_Carrier_Power_Value(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_TRANSMITTED_CARRIER_POWER_VALUE_INCRDECRTHRES:				/*  240 */
		offset = dissect_id_Transmitted_Carrier_Power_Value_IncrDecrThres(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_TUTRANGPSMEASUREMENTTHRESHOLDINFORMATION:						/*  241 */
		offset = dissect_id_TUTRANGPSMeasurementThresholdInformation(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UL_TIMESLOT_ISCP_VALUE:										/*  242 */
		offset = dissect_id_UL_Timeslot_ISCP_Value(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UL_TIMESLOT_ISCP_VALUE_INCRDECRTHRES:							/*  243 */
		offset = dissect_id_UL_Timeslot_ISCP_Value_IncrDecrThres(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RX_TIMING_DEVIATION_VALUE_LCR:								/*  293 */
		offset = dissect_id_Rx_Timing_Deviation_Value_LCR(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DPC_MODE_CHANGE_SUPPORTINDICATOR:								/*  19 */
		offset = dissect_id_DPC_Mode_Change_SupportIndicator(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UNUSED_PROTOCOLIE_RNSAP_ID_247:								/*  247 */
		break;
	case RNSAP_ID_UNUSED_PROTOCOLIE_RNSAP_ID_295:								/*  295 */
		break;
	case RNSAP_ID_PRIMARYCCPCH_RSCP_RL_RECONFPREPTDD:							/*  202 */
		offset = dissect_id_PrimaryCCPCH_RSCP_RL_ReconfPrepTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_TIMESLOT_ISCP_INFO_RL_RECONFPREPTDD:						/*  203 */
		offset = dissect_id_DL_TimeSlot_ISCP_Info_RL_ReconfPrepTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_TIMESLOT_ISCP_LCR_INFORMATION_RL_RECONFPREPTDD:			/*  204 */
		offset = dissect_id_DL_Timeslot_ISCP_LCR_Information_RL_ReconfPrepTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DSCH_RNTI:													/*  249 */
		offset = dissect_id_DSCH_RNTI(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_POWERBALANCING_INFORMATION:								/*  296 */
		offset = dissect_id_DL_PowerBalancing_Information(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_POWERBALANCING_ACTIVATIONINDICATOR:						/*  297 */
		offset = dissect_id_DL_PowerBalancing_ActivationIndicator(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_POWERBALANCING_UPDATEDINDICATOR:							/*  298 */
		offset = dissect_id_DL_PowerBalancing_UpdatedIndicator(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_REFERENCEPOWERINFORMATION:									/*  299 */
		offset = dissect_id_DL_ReferencePowerInformation(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_ENHANCED_PRIMARYCPICH_ECNO:									/*  224 */
		offset = dissect_id_Enhanced_PrimaryCPICH_EcNo(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_IPDL_TDD_PARAMETERSLCR:										/*  252 */
		offset = dissect_id_IPDL_TDD_ParametersLCR(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_CELLCAPABILITYCONTAINER_FDD:									/*  300 */
		offset = dissect_id_CellCapabilityContainer_FDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_CELLCAPABILITYCONTAINER_TDD:									/*  301 */
		offset = dissect_id_CellCapabilityContainer_TDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_CELLCAPABILITYCONTAINER_TDD_LCR:								/*  302 */
		offset = dissect_id_CellCapabilityContainer_TDD_LCR(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_SPECIFIC_DCH_INFO:											/*  317 */
		offset = dissect_id_RL_Specific_DCH_Info(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_RECONFIGURATIONREQUESTFDD_RL_INFORMATIONLIST:				/*  318 */
		offset = dissect_id_RL_ReconfigurationRequestFDD_RL_InformationList(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_RECONFIGURATIONREQUESTFDD_RL_INFORMATION_IES:				/*  319 */
		offset = dissect_id_RL_ReconfigurationRequestFDD_RL_Information_IEs(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_RECONFIGURATIONREQUESTTDD_RL_INFORMATION:					/*  321 */
		offset = dissect_id_RL_ReconfigurationRequestTDD_RL_Information(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_COMMONTRANSPORTCHANNELRESOURCESINITIALISATIONNOTREQUIRED:		/*  250 */
		offset = dissect_id_CommonTransportChannelResourcesInitialisationNotRequired(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DELAYEDACTIVATION:											/*  312 */
		offset = dissect_id_DelayedActivation(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DELAYEDACTIVATIONLIST_RL_ACTIVATIONCMDFDD:					/*  313 */
		offset = dissect_id_DelayedActivationList_RL_ActivationCmdFDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DELAYEDACTIVATIONINFORMATION_RL_ACTIVATIONCMDFDD:				/*  314 */
		offset = dissect_id_DelayedActivationInformation_RL_ActivationCmdFDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DELAYEDACTIVATIONLIST_RL_ACTIVATIONCMDTDD:					/*  315 */
		offset = dissect_id_DelayedActivationList_RL_ActivationCmdTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DELAYEDACTIVATIONINFORMATION_RL_ACTIVATIONCMDTDD:				/*  316 */
		offset = dissect_id_DelayedActivationInformation_RL_ActivationCmdTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_NEIGHBOURINGTDDCELLMEASUREMENTINFORMATIONLCR:					/*  251 */
		offset = dissect_id_neighbouringTDDCellMeasurementInformationLCR(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UL_SIR_TARGET_CCTRCH_INFORMATIONITEM_RL_SETUPRSPTDD:			/*  150 */
		offset = dissect_id_UL_SIR_Target_CCTrCH_InformationItem_RL_SetupRspTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UL_SIR_TARGET_CCTRCH_LCR_INFORMATIONITEM_RL_SETUPRSPTDD:		/*  151 */
		offset = dissect_id_UL_SIR_Target_CCTrCH_LCR_InformationItem_RL_SetupRspTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_PRIMCCPCH_RSCP_DL_PC_RQSTTDD:									/*  451 */
		offset = dissect_id_PrimCCPCH_RSCP_DL_PC_RqstTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_HSDSCH_FDD_INFORMATION:										/*  452 */
		offset = dissect_id_HSDSCH_FDD_Information(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_HSDSCH_FDD_INFORMATION_RESPONSE:								/*  453 */
		offset = dissect_id_HSDSCH_FDD_Information_Response(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_HSDSCH_FDD_UPDATE_INFORMATION:								/*  466 */
		offset = dissect_id_HSDSCH_FDD_Update_Information(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_HSDSCH_INFORMATION_TO_MODIFY:									/*  456 */
		offset = dissect_id_HSDSCH_Information_to_Modify(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_HSDSCHMACDFLOWSPECIFICINFORMATIONLIST_RL_PREEMPTREQUIREDIND:	/*  516 */
		offset = dissect_id_HSDSCHMacdFlowSpecificInformationList_RL_PreemptRequiredInd(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_HSDSCHMACDFLOWSPECIFICINFORMATIONITEM_RL_PREEMPTREQUIREDIND:	/*  517 */
		offset = dissect_id_HSDSCHMacdFlowSpecificInformationItem_RL_PreemptRequiredInd(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_HSDSCH_RNTI:													/*  457 */
		offset = dissect_id_HSDSCH_RNTI(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_HSDSCH_TDD_INFORMATION:										/*  458 */
		offset = dissect_id_HSDSCH_TDD_Information(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_HSDSCH_TDD_INFORMATION_RESPONSE:								/*  459 */
		offset = dissect_id_HSDSCH_TDD_Information_Response(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_HSDSCH_TDD_UPDATE_INFORMATION:								/*  467 */
		offset = dissect_id_HSDSCH_TDD_Update_Information(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_HSPDSCH_RL_ID:												/*  463 */
		offset = dissect_id_HSPDSCH_RL_ID(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_HSDSCH_MACDFLOWS_TO_ADD:										/*  531 */
		offset = dissect_id_HSDSCH_MACdFlows_to_Add(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_HSDSCH_MACDFLOWS_TO_DELETE:									/*  532 */
		offset = dissect_id_HSDSCH_MACdFlows_to_Delete(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_ANGLE_OF_ARRIVAL_VALUE_LCR:									/*  148 */
		offset = dissect_id_Angle_Of_Arrival_Value_LCR(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_TRAFFICCLASS:													/*  158 */
		offset = dissect_id_TrafficClass(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UNUSED_PROTOCOLIE_RNSAP_ID_248:								/*  248 */
		break;
	case RNSAP_ID_UNUSED_PROTOCOLIE_RNSAP_ID_253:								/*  253 */
		break;
	case RNSAP_ID_PDSCH_RL_ID:													/*  323 */
		offset = dissect_id_PDSCH_RL_ID(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_TIMESLOT_RL_SETUPRSPTDD:										/*  325 */
		offset = dissect_id_TimeSlot_RL_SetupRspTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_GERAN_CELL_CAPABILITY:										/*  468 */
		offset = dissect_id_GERAN_Cell_Capability(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_GERAN_CLASSMARK:												/*  469 */
		offset = dissect_id_GERAN_Classmark(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DSCH_INITIALWINDOWSIZE:										/*  480 */
		offset = dissect_id_DSCH_InitialWindowSize(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UL_SYNCHRONISATION_PARAMETERS_LCR:							/*  464 */
		offset = dissect_id_UL_Synchronisation_Parameters_LCR(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_SNA_INFORMATION:												/*  479 */
		offset = dissect_id_SNA_Information(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_MACHS_RESETINDICATOR:											/*  465 */
		offset = dissect_id_MAChs_ResetIndicator(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_TDD_DL_DPCH_TIMESLOTFORMATMODIFYITEM_LCR_RL_RECONFREADYTDD:	/*  481 */
		offset = dissect_id_TDD_DL_DPCH_TimeSlotFormatModifyItem_LCR_RL_ReconfReadyTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_TDD_UL_DPCH_TIMESLOTFORMATMODIFYITEM_LCR_RL_RECONFREADYTDD:	/*  482 */
		offset = dissect_id_TDD_UL_DPCH_TimeSlotFormatModifyItem_LCR_RL_ReconfReadyTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_TDD_TPC_UPLINKSTEPSIZE_LCR_RL_SETUPRQSTTDD:					/*  483 */
		offset = dissect_id_TDD_TPC_UplinkStepSize_LCR_RL_SetupRqstTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UL_CCTRCH_INFORMATIONLIST_RL_ADDITIONRQSTTDD:					/*  484 */
		offset = dissect_id_UL_CCTrCH_InformationList_RL_AdditionRqstTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UL_CCTRCH_INFORMATIONITEM_RL_ADDITIONRQSTTDD:					/*  485 */
		offset = dissect_id_UL_CCTrCH_InformationItem_RL_AdditionRqstTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_CCTRCH_INFORMATIONLIST_RL_ADDITIONRQSTTDD:					/*  486 */
		offset = dissect_id_DL_CCTrCH_InformationList_RL_AdditionRqstTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_CCTRCH_INFORMATIONITEM_RL_ADDITIONRQSTTDD:					/*  487 */
		offset = dissect_id_DL_CCTrCH_InformationItem_RL_AdditionRqstTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_TDD_TPC_UPLINKSTEPSIZE_INFORMATIONADD_LCR_RL_RECONFPREPTDD:	/*  488 */
		offset = dissect_id_TDD_TPC_UplinkStepSize_InformationAdd_LCR_RL_ReconfPrepTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_TDD_TPC_UPLINKSTEPSIZE_INFORMATIONMODIFY_LCR_RL_RECONFPREPTDD:/*  489 */
		offset = dissect_id_TDD_TPC_UplinkStepSize_InformationModify_LCR_RL_ReconfPrepTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_TDD_TPC_DOWNLINKSTEPSIZE_INFORMATIONADD_RL_RECONFPREPTDD:		/*  490 */
		offset = dissect_id_TDD_TPC_DownlinkStepSize_InformationAdd_RL_ReconfPrepTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_TDD_TPC_DOWNLINKSTEPSIZE_INFORMATIONMODIFY_RL_RECONFPREPTDD:	/*  491 */
		offset = dissect_id_TDD_TPC_DownlinkStepSize_InformationModify_RL_ReconfPrepTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UL_TIMINGADVANCECTRL_LCR:										/*  492 */
		offset = dissect_id_UL_TimingAdvanceCtrl_LCR(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_HSPDSCH_TIMESLOT_INFORMATIONLIST_PHYCHRECONFRQSTTDD:			/*  493 */
		offset = dissect_id_HSPDSCH_Timeslot_InformationList_PhyChReconfRqstTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_HSPDSCH_TIMESLOT_INFORMATIONLISTLCR_PHYCHRECONFRQSTTDD:		/*  494 */
		offset = dissect_id_HSPDSCH_Timeslot_InformationListLCR_PhyChReconfRqstTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_HS_SICH_RECEPTION_QUALITY: 									/*  495 */
		offset = dissect_id_HS_SICH_Reception_Quality(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_HS_SICH_RECEPTION_QUALITY_MEASUREMENT_VALUE:					/*  496 */
		offset = dissect_id_HS_SICH_Reception_Quality_Measurement_Value(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_HSSICH_INFO_DM_RPRT:											/*  497 */
		offset = dissect_id_HSSICH_Info_DM_Rprt(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_HSSICH_INFO_DM_RQST:											/*  498 */
		offset = dissect_id_HSSICH_Info_DM_Rqst(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_HSSICH_INFO_DM:												/*  499 */
		offset = dissect_id_HSSICH_Info_DM(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_CCTRCH_MAXIMUM_DL_POWER_RL_SETUPRSPTDD:						/*  500 */
		offset = dissect_id_CCTrCH_Maximum_DL_Power_RL_SetupRspTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_CCTRCH_MINIMUM_DL_POWER_RL_SETUPRSPTDD:						/*  501 */
		offset = dissect_id_CCTrCH_Minimum_DL_Power_RL_SetupRspTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_CCTRCH_MAXIMUM_DL_POWER_RL_ADDITIONRSPTDD:					/*  502 */
		offset = dissect_id_CCTrCH_Maximum_DL_Power_RL_AdditionRspTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_CCTRCH_MINIMUM_DL_POWER_RL_ADDITIONRSPTDD:					/*  503 */
		offset = dissect_id_CCTrCH_Minimum_DL_Power_RL_AdditionRspTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_CCTRCH_MAXIMUM_DL_POWER_RL_RECONFREADYTDD:					/*  504 */
		offset = dissect_id_CCTrCH_Maximum_DL_Power_RL_ReconfReadyTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_CCTRCH_MINIMUM_DL_POWER_RL_RECONFREADYTDD:					/*  505 */
		offset = dissect_id_CCTrCH_Minimum_DL_Power_RL_ReconfReadyTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_MAXIMUM_DL_POWER_TIMESLOTLCR_INFORMATIONMODIFYITEM_RL_RECONFREADYTDD:	/*  506 */
		offset = dissect_id_Maximum_DL_Power_TimeslotLCR_InformationModifyItem_RL_ReconfReadyTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_MINIMUM_DL_POWER_TIMESLOTLCR_INFORMATIONMODIFYITEM_RL_RECONFREADYTDD:	/*  507 */
		offset = dissect_id_Minimum_DL_Power_TimeslotLCR_InformationModifyItem_RL_ReconfReadyTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_CCTRCH_INFORMATIONLIST_RL_RECONFRSPTDD:					/*  508 */
		offset = dissect_id_DL_CCTrCH_InformationList_RL_ReconfRspTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_DPCH_INFORMATIONMODIFYITEM_LCR_RL_RECONFRSPTDD:			/*  509 */
		offset = dissect_id_DL_DPCH_InformationModifyItem_LCR_RL_ReconfRspTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_MAXIMUM_DL_POWER_TIMESLOTLCR_INFORMATIONITEM:					/*  510 */
		offset = dissect_id_Maximum_DL_Power_TimeslotLCR_InformationItem(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_MINIMUM_DL_POWER_TIMESLOTLCR_INFORMATIONITEM:					/*  511 */
		offset = dissect_id_Minimum_DL_Power_TimeslotLCR_InformationItem(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_TDD_SUPPORT_8PSK:												/*  512 */
		offset = dissect_id_TDD_Support_8PSK(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_TDD_MAXNRDLPHYSICALCHANNELS:									/*  513 */
		offset = dissect_id_TDD_maxNrDLPhysicalchannels(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_EXTENDEDGSMCELLINDIVIDUALOFFSET:								/*  514 */
		offset = dissect_id_ExtendedGSMCellIndividualOffset(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_PARAMETERUPDATEINDICATIONFDD_RL_INFORMATIONLIST:			/*  518 */
		offset = dissect_id_RL_ParameterUpdateIndicationFDD_RL_InformationList(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_PRIMARY_CPICH_USAGE_FOR_CHANNEL_ESTIMATION:					/*  519 */
		offset = dissect_id_Primary_CPICH_Usage_For_Channel_Estimation(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_SECONDARY_CPICH_INFORMATION:									/*  520 */
		offset = dissect_id_Secondary_CPICH_Information(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_SECONDARY_CPICH_INFORMATION_CHANGE:							/*  521 */
		offset = dissect_id_Secondary_CPICH_Information_Change(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UNUSED_PROTOCOLIE_RNSAP_ID_522:								/*  522 */
		break;
	case RNSAP_ID_UNUSED_PROTOCOLIE_RNSAP_ID_523:								/*  523 */
		break;
	case RNSAP_ID_RL_PARAMETERUPDATEINDICATIONFDD_RL_INFORMATION_ITEM:			/*  524 */
		offset = dissect_id_RL_ParameterUpdateIndicationFDD_RL_Information_Item(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_PHASE_REFERENCE_UPDATE_INDICATOR:								/*  525 */
		offset = dissect_id_Phase_Reference_Update_Indicator(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UNIDIRECTIONAL_DCH_INDICATOR:									/*  526 */
		offset = dissect_id_Unidirectional_DCH_Indicator(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_INFORMATION_RL_RECONFPREPTDD:								/*  527 */
		offset = dissect_id_RL_Information_RL_ReconfPrepTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_MULTIPLE_RL_INFORMATIONRESPONSE_RL_RECONFREADYTDD:			/*  528 */
		offset = dissect_id_Multiple_RL_InformationResponse_RL_ReconfReadyTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_RECONFIGURATIONRESPONSETDD_RL_INFORMATION:					/*  529 */
		offset = dissect_id_RL_ReconfigurationResponseTDD_RL_Information(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_SATELLITE_ALMANAC_INFORMATION_EXTITEM:						/*  530 */
		offset = dissect_id_Satellite_Almanac_Information_ExtItem(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_HSDSCH_INFORMATION_TO_MODIFY_UNSYNCHRONISED:					/*  533 */
		offset = dissect_id_HSDSCH_Information_to_Modify_Unsynchronised(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_TNLQOS:														/*  534 */
		offset = dissect_id_TnlQos(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RTLOADVALUE:													/*  535 */
		offset = dissect_id_RTLoadValue(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_NRTLOADINFORMATIONVALUE:										/*  536 */
		offset = dissect_id_NRTLoadInformationValue(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_CELLPORTIONID:												/*  537 */
		offset = dissect_id_CellPortionID(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UPPTSINTERFERENCEVALUE:										/*  538 */
		offset = dissect_id_UpPTSInterferenceValue(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_PRIMARYCCPCH_RSCP_DELTA:										/*  539 */
		offset = dissect_id_PrimaryCCPCH_RSCP_Delta(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UEMEASUREMENTTYPE:											/*  540 */
		offset = dissect_id_UEMeasurementType(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UEMEASUREMENTTIMESLOTINFOHCR:									/*  541 */
		offset = dissect_id_UEMeasurementTimeslotInfoHCR(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UEMEASUREMENTTIMESLOTINFOLCR:									/*  542 */
		offset = dissect_id_UEMeasurementTimeslotInfoLCR(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UEMEASUREMENTREPORTCHARACTERISTICS:							/*  543 */
		offset = dissect_id_UEMeasurementReportCharacteristics(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UEMEASUREMENTPARAMETERMODALLOW:								/*  544 */
		offset = dissect_id_UEMeasurementParameterModAllow(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UEMEASUREMENTVALUEINFORMATION:								/*  545 */
		offset = dissect_id_UEMeasurementValueInformation(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_INTERFACESTOTRACEITEM:										/*  546 */
		offset = dissect_id_InterfacesToTraceItem(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_LISTOFINTERFACESTOTRACE:										/*  547 */
		offset = dissect_id_ListOfInterfacesToTrace(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_TRACEDEPTH:													/*  548 */
		offset = dissect_id_TraceDepth(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_TRACERECORDINGSESSIONREFERENCE:								/*  549 */
		offset = dissect_id_TraceRecordingSessionReference(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_TRACEREFERENCE:												/*  550 */
		offset = dissect_id_TraceReference(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UEIDENTITY:													/*  551 */
		offset = dissect_id_UEIdentity(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_NACC_RELATED_DATA:											/*  552 */
		offset = dissect_id_NACC_Related_Data(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_GSM_CELL_INFEX_RQST:											/*  553 */
		offset = dissect_id_GSM_Cell_InfEx_Rqst(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_MEASUREMENTRECOVERYBEHAVIOR:									/*  554 */
		offset = dissect_id_MeasurementRecoveryBehavior(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_MEASUREMENTRECOVERYREPORTINGINDICATOR:						/*  555 */
		offset = dissect_id_MeasurementRecoveryReportingIndicator(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_MEASUREMENTRECOVERYSUPPORTINDICATOR:							/*  556 */
		offset = dissect_id_MeasurementRecoverySupportIndicator(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_DL_DPCH_POWER_INFORMATION_RL_RECONFPREPFDD:					/*  557 */
		offset = dissect_id_DL_DPCH_Power_Information_RL_ReconfPrepFDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_F_DPCH_INFORMATION_RL_RECONFPREPFDD:							/*  558 */
		offset = dissect_id_F_DPCH_Information_RL_ReconfPrepFDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_F_DPCH_INFORMATION_RL_SETUPRQSTFDD:							/*  559 */
		offset = dissect_id_F_DPCH_Information_RL_SetupRqstFDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_MBMS_BEARER_SERVICE_LIST:										/*  560 */
		offset = dissect_id_MBMS_Bearer_Service_List(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_MBMS_BEARER_SERVICE_LIST_INFEX_RSP:							/*  561 */
		offset = dissect_id_MBMS_Bearer_Service_List_InfEx_Rsp(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_ACTIVE_MBMS_BEARER_SERVICEFDD:								/*  562 */
		offset = dissect_id_Active_MBMS_Bearer_ServiceFDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_ACTIVE_MBMS_BEARER_SERVICETDD:								/*  563 */
		offset = dissect_id_Active_MBMS_Bearer_ServiceTDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_OLD_URA_ID:													/*  564 */
		offset = dissect_id_Old_URA_ID(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UE_STATE:														/*  568 */
		offset = dissect_id_UE_State(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_URA_ID:														/*  569 */
		offset = dissect_id_URA_ID(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_HARQ_PREAMBLE_MODE:											/*  571 */
		offset = dissect_id_HARQ_Preamble_Mode(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_SYNCHRONISATIONINDICATOR:										/*  572 */
		offset = dissect_id_SynchronisationIndicator(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_UL_DPDCHINDICATOREDCH:										/*  573 */
		offset = dissect_id_UL_DPDCHIndicatorEDCH(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_EDPCH_INFORMATION:											/*  574 */
		offset = dissect_id_EDPCH_Information(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_RL_SPECIFIC_EDCH_INFORMATION:									/*  575 */
		offset = dissect_id_RL_Specific_EDCH_Information(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_EDCH_RL_INDICATION:											/*  576 */
		offset = dissect_id_EDCH_RL_Indication(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_EDCH_FDD_INFORMATION:											/*  577 */
		offset = dissect_id_EDCH_FDD_Information(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_EDCH_RLSET_ID:												/*  578 */
		offset = dissect_id_EDCH_RLSet_Id(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_SERVING_EDCHRL_ID:											/*  579 */
		offset = dissect_id_Serving_EDCHRL_Id(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_EDCH_FDD_DL_CONTROLCHANNELINFORMATION:						/*  580 */
		offset = dissect_id_EDCH_FDD_DL_ControlChannelInformation(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_EDCH_FDD_INFORMATIONRESPONSE:									/*  581 */
		offset = dissect_id_EDCH_FDD_InformationResponse(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_EDCH_MACDFLOWS_TO_ADD:										/*  582 */
		offset = dissect_id_EDCH_MACdFlows_To_Add(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_EDCH_FDD_INFORMATION_TO_MODIFY:								/*  583 */
		offset = dissect_id_EDCH_FDD_Information_To_Modify(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_EDCH_MACDFLOWS_TO_DELETE:										/*  584 */
		offset = dissect_id_EDCH_MACdFlows_To_Delete(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_EDPCH_INFORMATION_RLRECONFREQUEST_FDD:						/*  585 */
		offset = dissect_id_EDPCH_Information_RLReconfRequest_FDD(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_EDCH_MACDFLOWSPECIFICINFORMATIONLIST_RL_PREEMPTREQUIREDIND:	/*  586 */
		offset = dissect_id_EDCH_MacdFlowSpecificInformationList_RL_PreemptRequiredInd(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_EDCH_MACDFLOWSPECIFICINFORMATIONITEM_RL_PREEMPTREQUIREDIND:	/*  587 */
		offset = dissect_id_EDCH_MacdFlowSpecificInformationItem_RL_PreemptRequiredInd(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_EDCH_MACDFLOWSPECIFICINFORMATIONLIST_RL_CONGESTIND:			/*  588 */
		offset = dissect_id_EDCH_MacdFlowSpecificInformationList_RL_CongestInd(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_EDCH_MACDFLOWSPECIFICINFORMATIONITEM_RL_CONGESTIND:			/*  589 */
		offset = dissect_id_EDCH_MacdFlowSpecificInformationItem_RL_CongestInd(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_MBMS_BEARER_SERVICE_FULL_ADDRESS:								/*  590 */
		offset = dissect_id_MBMS_Bearer_Service_Full_Address(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_INITIAL_DL_DPCH_TIMINGADJUSTMENT:								/*  591 */
		offset = dissect_id_Initial_DL_DPCH_TimingAdjustment(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_INITIAL_DL_DPCH_TIMINGADJUSTMENT_ALLOWED:						/*  592 */
		offset = dissect_id_Initial_DL_DPCH_TimingAdjustment_Allowed(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_USER_PLANE_CONGESTION_FIELDS_INCLUSION:						/*  593 */
		offset = dissect_id_User_Plane_Congestion_Fields_Inclusion(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_HARQ_PREAMBLE_MODE_ACTIVATION_INDICATOR:						/*  594 */
		offset = dissect_id_HARQ_Preamble_Mode_Activation_Indicator(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_MULTIPLE_DEDICATEDMEASUREMENTVALUELIST_TDD_DM_RSP:			/*  595 */
		offset = dissect_id_multiple_DedicatedMeasurementValueList_TDD_DM_Rsp(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_MULTIPLE_DEDICATEDMEASUREMENTVALUELIST_LCR_TDD_DM_RSP:		/*  596 */
		offset = dissect_id_multiple_DedicatedMeasurementValueList_LCR_TDD_DM_Rsp(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_PROVIDEDINFORMATION:											/*  597 */
		offset = dissect_id_ProvidedInformation(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_ACTIVE_MBMS_BEARER_SERVICEFDD_PFL:							/*  598 */
		offset = dissect_id_Active_MBMS_Bearer_ServiceFDD_PFL(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_ACTIVE_MBMS_BEARER_SERVICETDD_PFL:							/*  599 */
		offset = dissect_id_Active_MBMS_Bearer_ServiceTDD_PFL(tvb, offset, actx, value_tree);
		break;
	case RNSAP_ID_FREQUENCYBANDINDICATOR:										/*  600	*/
		offset = dissect_id_FrequencyBandIndicator(tvb, offset, actx, value_tree);
		break;
	default:
		offset = offset + (length<<3);
		break;
	}
	BYTE_ALIGN_OFFSET(offset);	
	return offset;
}


static void
dissect_rnsap(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	proto_item	*rnsap_item = NULL;
	proto_tree	*rnsap_tree = NULL;

	top_tree = tree;

	/* make entry in the Protocol column on summary display */
	if (check_col(pinfo->cinfo, COL_PROTOCOL))
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "RNSAP");

	/* create the rnsap protocol tree */
	rnsap_item = proto_tree_add_item(tree, proto_rnsap, tvb, 0, -1, FALSE);
	rnsap_tree = proto_item_add_subtree(rnsap_item, ett_rnsap);
	
	dissect_RNSAP_PDU_PDU(tvb, pinfo, rnsap_tree);
}

#if 0
static gboolean
dissect_sccp_rnsap_heur(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
    guint8 temp;

    dissect_rnsap(tvb, pinfo, tree);

    /*
     * Is it a rnsap packet?
     */
    return TRUE;
}
#endif

/*--- proto_register_rnsap -------------------------------------------*/
void proto_register_rnsap(void) {

  /* List of fields */

  static hf_register_info hf[] = {
	{ &hf_rnsap_pdu_length,
		{ "PDU Length", "rnsap.pdu_length", FT_UINT32, BASE_DEC,
		NULL, 0, "Number of octets in the PDU", HFILL }},
	{ &hf_rnsap_IE_length,
		{ "IE Length", "rnsap.ie_length", FT_UINT32, BASE_DEC,
		NULL, 0, "Number of octets in the IE", HFILL }},
    { &hf_rnsap_L3_DL_DCCH_Message_PDU,
      { "DL-DCCH-Message", "rnsap.DL_DCCH_Message",
        FT_NONE, BASE_NONE, NULL, 0,
        "DL-DCCH-Message", HFILL }},

#include "packet-rnsap-hfarr.c"
  };

  /* List of subtrees */
  static gint *ett[] = {
		  &ett_rnsap,
		  &ett_rnsap_initiatingMessageValue,
		  &ett_rnsap_ProtocolIEValueValue,
		  &ett_rnsap_SuccessfulOutcomeValue,
		  &ett_rnsap_UnsuccessfulOutcomeValue,
#include "packet-rnsap-ettarr.c"
  };


  /* Register protocol */
  proto_rnsap = proto_register_protocol(PNAME, PSNAME, PFNAME);
  /* Register fields and subtrees */
  proto_register_field_array(proto_rnsap, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));

 
  register_dissector("rnsap", dissect_rnsap, proto_rnsap);


}


/*--- proto_reg_handoff_rnsap ---------------------------------------*/
void
proto_reg_handoff_rnsap(void)
{

	rnsap_handle = find_dissector("rnsap");
	dissector_add("sccp.ssn", SCCP_SSN_RNSAP, rnsap_handle);
	/* Add heuristic dissector
	 * Perhaps we want a preference whether the heuristic dissector
	 * is or isn't enabled
	 */
	/*heur_dissector_add("sccp", dissect_sccp_rnsap_heur, proto_rnsap); */

}


