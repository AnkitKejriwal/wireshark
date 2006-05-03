/* packet-gsm_map-template.c
 * Routines for GSM MobileApplication packet dissection
 * Copyright 2004 - 2006 , Anders Broman <anders.broman [AT] ericsson.com>
 * Based on the dissector by:
 * Felix Fei <felix.fei [AT] utstar.com>
 * and Michael Lum <mlum [AT] telostech.com>
 *
 * $Id$
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
 * References: ETSI TS 129 002
 * Updated to ETSI TS 129 002 V6.9.0 (2005-3GPP TS 29.002 version 6.9.0 Release 6)
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glib.h>
#include <epan/packet.h>
#include <epan/prefs.h>
#include <epan/conversation.h>
#include <epan/tap.h>
#include <epan/emem.h>

#include <stdio.h>
#include <string.h>

#include "packet-ber.h"
#include "packet-q931.h"
#include "packet-gsm_map.h"
#include "packet-gsm_a.h"
#include "packet-tcap.h"
#include "packet-e212.h"
#include "packet-smpp.h"
#include "packet-gsm_sms.h"

#define PNAME  "GSM Mobile Application"
#define PSNAME "GSM_MAP"
#define PFNAME "gsm_map"

/* Initialize the protocol and registered fields */
int proto_gsm_map = -1;
/*
static int hf_gsm_map_invokeCmd = -1;             / Opcode /
static int hf_gsm_map_invokeid = -1;              / INTEGER /
static int hf_gsm_map_absent = -1;                / NULL /
static int hf_gsm_map_invokeId = -1;              / InvokeId /
static int hf_gsm_map_invoke = -1;                / InvokePDU /
static int hf_gsm_map_returnResult = -1;          / InvokePDU /
static int hf_gsm_map_returnResult_result = -1;
static int hf_gsm_map_returnError_result = -1;
static int hf_gsm_map_returnError = -1;
static int hf_gsm_map_local_errorCode = -1;
static int hf_gsm_map_global_errorCode_oid = -1;
static int hf_gsm_map_global_errorCode = -1;
*/
static int hf_gsm_map_SendAuthenticationInfoArg = -1;
static int hf_gsm_map_SendAuthenticationInfoRes = -1;
static int hf_gsm_mapSendEndSignal = -1;
static int hf_gsm_map_getPassword = -1;
static int hf_gsm_map_CheckIMEIArg = -1;
static int hf_gsm_map_currentPassword = -1;
static int hf_gsm_map_extension = -1;
static int hf_gsm_map_nature_of_number = -1;
static int hf_gsm_map_number_plan = -1;
static int hf_gsm_map_isdn_address_digits = -1;
static int hf_gsm_map_address_digits = -1;
static int hf_gsm_map_servicecentreaddress_digits = -1;
static int hf_gsm_map_imsi_digits = -1;
static int hf_gsm_map_Ss_Status_unused = -1;
static int hf_gsm_map_Ss_Status_q_bit = -1;
static int hf_gsm_map_Ss_Status_p_bit = -1;
static int hf_gsm_map_Ss_Status_r_bit = -1;
static int hf_gsm_map_Ss_Status_a_bit = -1;
static int hf_gsm_map_notification_to_forwarding_party = -1;
static int hf_gsm_map_redirecting_presentation = -1;
static int hf_gsm_map_notification_to_calling_party = -1;
static int hf_gsm_map_forwarding_reason = -1;
static int hf_gsm_map_pdp_type_org = -1;
static int hf_gsm_map_etsi_pdp_type_number = -1;
static int hf_gsm_map_ietf_pdp_type_number = -1;
static int hf_gsm_map_ext_qos_subscribed_pri = -1;

static int hf_gsm_map_qos_traffic_cls = -1;
static int hf_gsm_map_qos_del_order = -1;
static int hf_gsm_map_qos_del_of_err_sdu = -1;
static int hf_gsm_map_qos_ber = -1;
static int hf_gsm_map_qos_sdu_err_rat = -1;
static int hf_gsm_map_qos_traff_hdl_pri = -1;
static int hf_gsm_map_qos_max_sdu = -1;
static int hf_gsm_map_max_brate_ulink = -1;
static int hf_gsm_map_max_brate_dlink = -1;
static int hf_gsm_map_qos_transfer_delay = -1;
static int hf_gsm_map_guaranteed_max_brate_ulink = -1;
static int hf_gsm_map_guaranteed_max_brate_dlink = -1;
static int hf_gsm_map_GSNAddress_IPv4 = -1;
static int hf_gsm_map_GSNAddress_IPv6 = -1;
#include "packet-gsm_map-hf.c"

/* Initialize the subtree pointers */
static gint ett_gsm_map = -1;
static gint ett_gsm_map_InvokeId = -1;
static gint ett_gsm_map_InvokePDU = -1;
static gint ett_gsm_map_ReturnResultPDU = -1;
static gint ett_gsm_map_ReturnErrorPDU = -1;
static gint ett_gsm_map_ReturnResult_result = -1;
static gint ett_gsm_map_ReturnError_result = -1;
static gint ett_gsm_map_GSMMAPPDU = -1;
static gint ett_gsm_map_ext_qos_subscribed = -1;
static gint ett_gsm_map_pdptypenumber = -1;
static gint ett_gsm_map_RAIdentity = -1; 
static gint ett_gsm_map_LAIFixedLength = -1;

#include "packet-gsm_map-ett.c"

static dissector_table_t	sms_dissector_table;	/* SMS TPDU */
static dissector_handle_t data_handle;

/* Preferenc settings default */
#define MAX_SSN 254
static range_t *global_ssn_range;
static range_t *ssn_range;
dissector_handle_t	map_handle;

/* Global variables */
static guint32 opcode=0;
static guint32 errorCode;
static proto_tree *top_tree;
static int application_context_version;
gint protocolId;
gint AccessNetworkProtocolId;
const char *obj_id = NULL;
static int gsm_map_tap = -1;

/* Forward declarations */
static int dissect_invokeData(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset);
static int dissect_returnResultData(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset);
static int dissect_returnErrorData(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset);

/* Value strings */

const value_string gsm_map_PDP_Type_Organisation_vals[] = {
  {  0, "ETSI" },
  {  1, "IETF" },
  { 0, NULL }
};

const value_string gsm_map_ietf_defined_pdp_vals[] = {
  {  0x21, "IPv4 Address" },
  {  0x57, "IPv6 Address" },
  { 0, NULL }
};

const value_string gsm_map_etsi_defined_pdp_vals[] = {
  {  1, "PPP" },
  { 0, NULL }
};

char*
unpack_digits(tvbuff_t *tvb, int offset){

	int length;
	guint8 octet;
	int i=0;
	char *digit_str;

	length = tvb_length(tvb);
	if (length < offset)
		return "";
	digit_str = ep_alloc((length - offset)*2+1);

	while ( offset < length ){

		octet = tvb_get_guint8(tvb,offset);
		digit_str[i] = ((octet & 0x0f) + '0');
		i++;

		/*
		 * unpack second value in byte
		 */
		octet = octet >> 4;

		if (octet == 0x0f)	/* odd number bytes - hit filler */
			break;

		digit_str[i] = ((octet & 0x0f) + '0');
		i++;
		offset++;

	}
	digit_str[i]= '\0';
	return digit_str;
}

/* returns value in kb/s */
static guint
gsm_map_calc_bitrate(guint8 value){

	guint8 granularity;
	guint returnvalue; 

	if (value == 0xff)
		return 0;

	granularity = value >> 6;
	returnvalue = value & 0x7f;
	switch (granularity){
	case 0:
		break;
	case 1:
		returnvalue = ((returnvalue - 0x40) << 3)+64;
		break;
	case 2:
		returnvalue = (returnvalue << 6)+576;
		break;
	case 3:
		returnvalue = (returnvalue << 6)+576;
		break;
	}
	return returnvalue;

}

static void 
dissect_gsm_map_ext_qos_subscribed(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree){
	int offset = 0;
    proto_item *item;
    proto_tree *subtree;
	guint8 octet;
	guint16 value;

	item = get_ber_last_created_item();
	subtree = proto_item_add_subtree(item, ett_gsm_map_ext_qos_subscribed);
	/*  OCTET 1:
		Allocation/Retention Priority (This octet encodes each priority level defined in
		23.107 as the binary value of the priority level, declaration in 29.060)
		Octets 2-9 are coded according to 3GPP TS 24.008[35] Quality of Service Octets
		6-13.
	 */
	/* Allocation/Retention Priority */
	proto_tree_add_item(subtree, hf_gsm_map_ext_qos_subscribed_pri, tvb, offset, 1, FALSE);
	offset++;

	/* Quality of Service Octets 6-13.( Octet 2 - 9 Here) */

	/* Traffic class, octet 6 (see 3GPP TS 23.107) Bits 8 7 6 */
	proto_tree_add_item(subtree, hf_gsm_map_qos_traffic_cls, tvb, offset, 1, FALSE);
	/* Delivery order, octet 6 (see 3GPP TS 23.107) Bits 5 4 */
	proto_tree_add_item(subtree, hf_gsm_map_qos_del_order, tvb, offset, 1, FALSE);
	/* Delivery of erroneous SDUs, octet 6 (see 3GPP TS 23.107) Bits 3 2 1 */
	proto_tree_add_item(subtree, hf_gsm_map_qos_del_of_err_sdu, tvb, offset, 1, FALSE);
	offset++;

	/* Maximum SDU size, octet 7 (see 3GPP TS 23.107) */
	octet = tvb_get_guint8(tvb,offset);
	switch (octet){
	case 0:
		proto_tree_add_text(subtree, tvb, offset, 1, "Subscribed Maximum SDU size/Reserved");
		break;
	case 0x93:
		value = 1502;
		proto_tree_add_uint(subtree, hf_gsm_map_qos_max_sdu, tvb, offset, 1, value);
		break;
	case 0x98:
		value = 1510;
		proto_tree_add_uint(subtree, hf_gsm_map_qos_max_sdu, tvb, offset, 1, value);
		break;
	case 0x99:
		value = 1532;
		proto_tree_add_uint(subtree, hf_gsm_map_qos_max_sdu, tvb, offset, 1, value);
		break;
	default:
		if (octet<0x97){
			value = octet * 10;
			proto_tree_add_uint(subtree, hf_gsm_map_qos_max_sdu, tvb, offset, 1, value);
		}else{
			proto_tree_add_text(subtree, tvb, offset, 1, "Maximum SDU size value 0x%x not defined in TS 24.008",octet);
		}			
	}
	offset++;

	/* Maximum bit rate for uplink, octet 8 */
	octet = tvb_get_guint8(tvb,offset);
	if (octet == 0 ){
		proto_tree_add_text(subtree, tvb, offset, 1, "Subscribed Maximum bit rate for uplink/Reserved"  );
	}else{
		proto_tree_add_uint(subtree, hf_gsm_map_max_brate_ulink, tvb, offset, 1, gsm_map_calc_bitrate(octet));
	}
	offset++;
	/* Maximum bit rate for downlink, octet 9 (see 3GPP TS 23.107) */
	octet = tvb_get_guint8(tvb,offset);
	if (octet == 0 ){
		proto_tree_add_text(subtree, tvb, offset, 1, "Subscribed Maximum bit rate for downlink/Reserved"  );
	}else{
		proto_tree_add_uint(subtree, hf_gsm_map_max_brate_dlink, tvb, offset, 1, gsm_map_calc_bitrate(octet));
	}
	offset++;
	/* Residual Bit Error Rate (BER), octet 10 (see 3GPP TS 23.107) Bits 8 7 6 5 */ 
	proto_tree_add_item(subtree, hf_gsm_map_qos_ber, tvb, offset, 1, FALSE);
	/* SDU error ratio, octet 10 (see 3GPP TS 23.107) */
	proto_tree_add_item(subtree, hf_gsm_map_qos_sdu_err_rat, tvb, offset, 1, FALSE);
	offset++;

	/* Transfer delay, octet 11 (See 3GPP TS 23.107) Bits 8 7 6 5 4 3 */
	proto_tree_add_item(subtree, hf_gsm_map_qos_transfer_delay, tvb, offset, 1, FALSE);
	/* Traffic handling priority, octet 11 (see 3GPP TS 23.107) Bits 2 1 */
	proto_tree_add_item(subtree, hf_gsm_map_qos_traff_hdl_pri, tvb, offset, 1, FALSE);
	offset++;

	/*	Guaranteed bit rate for uplink, octet 12 (See 3GPP TS 23.107)
		Coding is identical to that of Maximum bit rate for uplink.
	 */
	octet = tvb_get_guint8(tvb,offset);
	if (octet == 0 ){
		proto_tree_add_text(subtree, tvb, offset, 1, "Subscribed Guaranteed bit rate for uplink/Reserved"  );
	}else{
		proto_tree_add_uint(subtree, hf_gsm_map_guaranteed_max_brate_ulink, tvb, offset, 1, gsm_map_calc_bitrate(octet));
	}
	offset++;

	/*	Guaranteed bit rate for downlink, octet 13(See 3GPP TS 23.107)
		Coding is identical to that of Maximum bit rate for uplink.
	 */
	octet = tvb_get_guint8(tvb,offset);
	if (octet == 0 ){
		proto_tree_add_text(subtree, tvb, offset, 1, "Subscribed Guaranteed bit rate for downlink/Reserved"  );
	}else{
		proto_tree_add_uint(subtree, hf_gsm_map_guaranteed_max_brate_dlink, tvb, offset, 1, gsm_map_calc_bitrate(octet));
	}

}

#include "packet-gsm_map-fn.c"

const value_string gsm_map_opr_code_strings[] = {
  {   2, "updateLocation" },
  {   3, "cancelLocation" },
  {   4, "provideRoamingNumber" },
  {	  5, "noteSubscriberDataModified" },	
  {   6, "resumeCallHandling" },
  {   7, "insertSubscriberData" },
  {   8, "deleteSubscriberData" },
  {   9, "sendParameters" },					/* map-ac infoRetrieval (14) version1 (1)*/
  {  10, "registerSS" },
  {  11, "eraseSS" },
  {  12, "activateSS" },
  {  13, "deactivateSS" },
  {  14, "interrogateSS" },
  {	 15, "authenticationFailureReport" },	
  {  17, "registerPassword" },
  {  18, "getPassword" },
  {  19, "processUnstructuredSS-Data" },		/* map-ac networkFunctionalSs (18) version1 (1) */
  {  20, "releaseResources" },
  {  22, "sendRoutingInfo" },
  {  23, "updateGprsLocation" },
  {  24, "sendRoutingInfoForGprs" },
  {  25, "failureReport" },
  {  26, "noteMsPresentForGprs" },
  {  28, "performHandover" },					/* map-ac handoverControl (11) version1 (1)*/
  {  29, "sendEndSignal" },
  {  30, "performSubsequentHandover" },			/* map-ac handoverControl (11) version1 (1) */
  {  31, "provideSIWFSNumber" },
  {  32, "sIWFSSignallingModify" },
  {  33, "processAccessSignalling" },
  {  34, "forwardAccessSignalling" },
  {  35, "noteInternalHandover" },				/* map-ac handoverControl (11) version1 (1) */
  {  37, "reset" },
  {  38, "forwardCheckSS-Indication" },
  {  39, "prepareGroupCall" },
  {  40, "sendGroupCallEndSignal" },
  {  41, "processGroupCallSignalling" },
  {  42, "forwardGroupCallSignalling" },
  {  43, "checkIMEI" },
  {  44, "mt-forwardSM" },
  {  45, "sendRoutingInfoForSM" },
  {  46, "mo-forwardSM" },
  {  47, "reportSM-DeliveryStatus" },
  {  48, "noteSubscriberPresent" },				/* map-ac mwdMngt (24) version1 (1) */
  {  49, "alertServiceCentreWithoutResult" },	/* map-ac shortMsgAlert (23) version1 (1) */
  {  50, "activateTraceMode" },
  {  51, "deactivateTraceMode" },
  {  52, "traceSubscriberActivity" },			/* map-ac handoverControl (11) version1 (1) */
  {  54, "beginSubscriberActivity" },			/* map-ac networkFunctionalSs (18) version1 (1) */
  {  55, "sendIdentification" },
  {  56, "sendAuthenticationInfo" },
  {  57, "restoreData" },
  {  58, "sendIMSI" },
  {  59, "processUnstructuredSS-Request" },
  {  60, "unstructuredSS-Request" },
  {  61, "unstructuredSS-Notify" },
  {  62, "anyTimeSubscriptionInterrogation" },
  {  63, "informServiceCentre" },
  {  64, "alertServiceCentre" },
  {  65, "anyTimeModification" },
  {  66, "readyForSM" },
  {  67, "purgeMS" },
  {  68, "prepareHandover" },
  {  69, "prepareSubsequentHandover" },
  {  70, "provideSubscriberInfo" },
  {  71, "anyTimeInterrogation" },
  {  72, "ss-InvocationNotification" },
  {  73, "setReportingState" },
  {  74, "statusReport" },
  {  75, "remoteUserFree" },
  {  76, "registerCC-Entry" },
  {  77, "eraseCC-Entry" },
  {  78, "secureTransportClass1" },
  {  79, "secureTransportClass2" },
  {  80, "secureTransportClass3" },
  {  81, "secureTransportClass4" },
  {  83, "provideSubscriberLocation" },
  {  85, "sendRoutingInfoForLCS" },
  {  86, "subscriberLocationReport" },
  {	 87, "ist-Alert" },
  {	 88, "ist-Command" },
  {  89, "noteMM-Event" },
  { 0, NULL }
};
static const value_string gsm_map_err_code_string_vals[] = {
    { 1,	"Unknown Subscriber" },
    { 3,	"Unknown MSC" },
    { 5,	"Unidentified Subscriber" },
    { 6,	"Absent Subscriber SM" },
    { 7,	"Unknown Equipment" },
    { 8,	"Roaming Not Allowed" },
    { 9,	"Illegal Subscriber" },
    { 10,	"Bearer Service Not Provisioned" },
    { 11,	"Teleservice Not Provisioned" },
    { 12,	"Illegal Equipment" },
    { 13,	"Call Barred" },
    { 14,	"Forwarding Violation" },
    { 15,	"CUG Reject" },
    { 16,	"Illegal SS Operation" },
    { 17,	"SS Error Status" },
    { 18,	"SS Not Available" },
    { 19,	"SS Subscription Violation" },
    { 20,	"SS Incompatibility" },
    { 21,	"Facility Not Supported" },
    { 25,	"No Handover Number Available" },
    { 26,	"Subsequent Handover Failure" },
    { 27,	"Absent Subscriber" },
    { 28,	"Incompatible Terminal" },
    { 29,	"Short Term Denial" },
    { 30,	"Long Term Denial" },
    { 31,	"Subscriber Busy For MT SMS" },
    { 32,	"SM Delivery Failure" },
    { 33,	"Message Waiting List Full" },
    { 34,	"System Failure" },
    { 35,	"Data Missing" },
    { 36,	"Unexpected Data Value" },
    { 37,	"PW Registration Failure" },
    { 38,	"Negative PW Check" },
    { 39,	"No Roaming Number Available" },
    { 40,	"Tracing Buffer Full" },
    { 42,	"Target Cell Outside Group Call Area" },
    { 43,	"Number Of PW Attempts Violation" },
    { 44,	"Number Changed" },
    { 45,	"Busy Subscriber" },
    { 46,	"No Subscriber Reply" },
    { 47,	"Forwarding Failed" },
    { 48,	"OR Not Allowed" },
    { 49,	"ATI Not Allowed" },
    { 50,	"No Group Call Number Available" },
    { 51,	"Resource Limitation" },
    { 52,	"Unauthorized Requesting Network" },
    { 53,	"Unauthorized LCS Client" },
    { 54,	"Position Method Failure" },
    { 58,	"Unknown Or Unreachable LCS Client" },
    { 59,	"MM Event Not Supported" },
    { 60,	"ATSI Not Allowed" },
    { 61,	"ATM Not Allowed" },
    { 62,	"Information Not Available" },
    { 71,	"Unknown Alphabet" },
    { 72,	"USSD Busy" },
    { 120,	"Nbr Sb Exceeded" },
    { 121,	"Rejected By User" },
    { 122,	"Rejected By Network" },
    { 123,	"Deflection To Served Subscriber" },
    { 124,	"Special Service Code" },
    { 125,	"Invalid Deflected To Number" },
    { 126,	"Max Number Of MPTY Participants Exceeded" },
    { 127,	"Resources Not Available" },
    { 0, NULL }
};
static const true_false_string gsm_map_extension_value = {
  "No Extension",
  "Extension"
};
static const value_string gsm_map_nature_of_number_values[] = {
	{   0x00,	"unknown" },
	{   0x01,	"International Number" },
	{   0x02,	"National Significant Number" },
	{   0x03,	"Network Specific Number" },
	{   0x04,	"Subscriber Number" },
	{   0x05,	"Reserved" },
	{   0x06,	"Abbreviated Number" },
	{   0x07,	"Reserved for extension" },
	{ 0, NULL }
};
static const value_string gsm_map_number_plan_values[] = {
	{   0x00,	"unknown" },
	{   0x01,	"ISDN/Telephony Numbering (Rec ITU-T E.164)" },
	{   0x02,	"spare" },
	{   0x03,	"Data Numbering (ITU-T Rec. X.121)" },
	{   0x04,	"Telex Numbering (ITU-T Rec. F.69)" },
	{   0x05,	"spare" },
	{   0x06,	"Land Mobile Numbering (ITU-T Rec. E.212)" },
	{   0x07,	"spare" },
	{   0x08,	"National Numbering" },
	{   0x09,	"Private Numbering" },
	{   0x0f,	"Reserved for extension" },
	{ 0, NULL }
};

static const true_false_string gsm_map_Ss_Status_q_bit_values = {
  "Quiescent",
  "Operative"
};
static const true_false_string gsm_map_Ss_Status_p_values = {
  "Provisioned",
  "Not Provisioned"
};
static const true_false_string gsm_map_Ss_Status_r_values = {
  "Registered",
  "Not Registered"
};
static const true_false_string gsm_map_Ss_Status_a_values = {
  "Active",
  "not Active"
};


static int
dissect_gsm_map_Opcode(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_integer(FALSE, pinfo, tree, tvb, offset, hf_index, &opcode);

  if (check_col(pinfo->cinfo, COL_INFO)){
    col_append_fstr(pinfo->cinfo, COL_INFO, val_to_str(opcode, gsm_map_opr_code_strings, "Unknown GSM-MAP (%u)"));
  }

  return offset;
}

static int dissect_invokeData(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
 
  gint8 bug_class;
  gboolean bug_pc, bug_ind_field;
  gint32 bug_tag;
  guint32 bug_len1;
  
  guint8 octet;

  switch(opcode){
  case  2: /*updateLocation*/	
	  offset=dissect_gsm_map_UpdateLocationArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case  3: /*cancelLocation*/
 	octet = tvb_get_guint8(tvb,0) & 0xf;
	if ( octet == 3){ /*   */ 
	  /* XXX  asn2eth can not yet handle tagged assignment yes so this
	   * XXX is some conformance file magic to work around that bug
	   */
	  offset = get_ber_identifier(tvb, offset, &bug_class, &bug_pc, &bug_tag);
	  offset = get_ber_length(tree, tvb, offset, &bug_len1, &bug_ind_field);
		offset=dissect_gsm_map_CancelLocationArg(TRUE, tvb, offset, pinfo, tree, -1);
	}else{
    offset=dissect_gsm_map_CancelLocationArgV2(FALSE, tvb, offset, pinfo, tree, -1);
	}
    break;
  case  4: /*provideRoamingNumber*/
    offset=dissect_gsm_map_ProvideRoamingNumberArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case  6: /*resumeCallHandling*/
    offset=dissect_gsm_map_ResumeCallHandlingArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case  7: /*insertSubscriberData*/
    offset=dissect_gsm_map_InsertSubscriberDataArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case  8: /*deleteSubscriberData*/
    offset=dissect_gsm_map_DeleteSubscriberDataArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
	/* TODO find out why this isn't in the ASN1 file
  case  9: sendParameters
    offset=dissect_gsm_map_DeleteSubscriberDataArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
	*/
  case  10: /*registerSS*/
    offset=dissect_gsm_map_RegisterSS_Arg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case  11: /*eraseSS*/
    offset=dissect_gsm_map_SS_ForBS_Code(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 12: /*activateSS*/
    offset=dissect_gsm_map_SS_ForBS_Code(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 13: /*deactivateSS*/
    offset=dissect_gsm_map_SS_ForBS_Code(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 14: /*interrogateSS*/
    offset=dissect_gsm_map_SS_ForBS_Code(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 15: /*authenticationFailureReport*/
	  offset=dissect_gsm_map_AuthenticationFailureReportArg(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 17: /*registerPassword*/
    offset=dissect_gsm_map_SS_Code(FALSE, tvb, offset, pinfo, tree, hf_gsm_map_ss_Code);
    break;
  case 18: /*getPassword*/
    offset=dissect_gsm_map_GetPasswordArg(FALSE, tvb, offset, pinfo, tree, hf_gsm_map_getPassword);
    break;
  case 20: /*releaseResources*/
    offset=dissect_gsm_map_ReleaseResourcesArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 22: /*sendRoutingInfo*/
    offset=dissect_gsm_map_SendRoutingInfoArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 23: /*updateGprsLocation*/
    offset=dissect_gsm_map_UpdateGprsLocationArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 24: /*sendRoutingInfoForGprs*/
    offset=dissect_gsm_map_SendRoutingInfoForGprsArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 25: /*failureReport*/
    offset=dissect_gsm_map_FailureReportArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 26: /*noteMsPresentForGprs*/
    offset=dissect_gsm_map_NoteMsPresentForGprsArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 29: /*sendEndSignal*/
	octet = tvb_get_guint8(tvb,0) & 0xf;
	if ( octet == 3){ /* This is a V3 message ??? */ 
	  /* XXX  asn2eth can not yet handle tagged assignment yes so this
	   * XXX is some conformance file magic to work around that bug
	   */
	  offset = get_ber_identifier(tvb, offset, &bug_class, &bug_pc, &bug_tag);
	  offset = get_ber_length(tree, tvb, offset, &bug_len1, &bug_ind_field);
		offset=dissect_gsm_map_SendEndSignalArgV3(TRUE, tvb, offset, pinfo, tree, hf_gsm_mapSendEndSignal);
	}else{
		offset=dissect_gsm_map_Bss_APDU(FALSE, tvb, offset, pinfo, tree, hf_gsm_mapSendEndSignal);
	}
    break;
  case 31: /*provideSIWFSNumbe*/
    offset=dissect_gsm_map_ProvideSIWFSNumberArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 32: /*sIWFSSignallingModify*/
    offset=dissect_gsm_map_SIWFSSignallingModifyArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 33: /*processAccessSignalling*/
	octet = tvb_get_guint8(tvb,0) & 0xf;
	if ( octet == 3){ /* This is a V3 message ??? */ 
	  /* XXX  asn2eth can not yet handle tagged assignment yes so this
	   * XXX is some conformance file magic to work around that bug
	   */
	  offset = get_ber_identifier(tvb, offset, &bug_class, &bug_pc, &bug_tag);
	  offset = get_ber_length(tree, tvb, offset, &bug_len1, &bug_ind_field);
		offset = dissect_gsm_map_ProcessAccessSignallingArgV3(TRUE, tvb, offset, pinfo, tree, -1);
	}else{
    offset=dissect_gsm_map_Bss_APDU(FALSE, tvb, offset, pinfo, tree, -1);
	}
    break;
  case 34: /*forwardAccessSignalling*/
	octet = tvb_get_guint8(tvb,0) & 0xf;
	if ( octet == 3){ /* This is a V3 message ??? */
	  /* XXX  asn2eth can not yet handle tagged assignment yes so this
	   * XXX is some conformance file magic to work around that bug
	   */
	  offset = get_ber_identifier(tvb, offset, &bug_class, &bug_pc, &bug_tag);
	  offset = get_ber_length(tree, tvb, offset, &bug_len1, &bug_ind_field);
		offset=dissect_gsm_map_ForwardAccessSignallingArgV3(TRUE, tvb, offset, pinfo, tree, -1);
	}else{
		 offset=dissect_gsm_map_Bss_APDU(FALSE, tvb, offset, pinfo, tree, -1);
	}
    break;
  case 37: /*reset*/
    offset=dissect_gsm_map_ResetArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 38: /*forwardCheckSS-Indication*/
    return offset;
    break;
  case 39: /*prepareGroupCall*/
    offset=dissect_gsm_map_PrepareGroupCallArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 40: /*sendGroupCallEndSignal*/
    dissect_gsm_map_SendGroupCallEndSignalArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 42: /*processGroupCallSignalling*/
    offset=dissect_gsm_map_ProcessGroupCallSignallingArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 43: /*checkIMEI*/
	  if (application_context_version < 3 ){
		  offset = dissect_gsm_map_IMEI(FALSE, tvb, offset, pinfo, tree, hf_gsm_map_imei);
	  }else{
		  offset=dissect_gsm_map_CheckIMEIArgV3(FALSE, tvb, offset, pinfo, tree, hf_gsm_map_CheckIMEIArg);
	  }
    break;
  case 44: /*mt-forwardSM*/
    offset=dissect_gsm_map_Mt_forwardSM_Arg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 45: /*sendRoutingInfoForSM*/
    offset=dissect_gsm_map_RoutingInfoForSMArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 46: /*mo-forwardSM*/
    offset=dissect_gsm_map_Mo_forwardSM_Arg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 47: /*reportSM-DeliveryStatus*/
    offset=dissect_gsm_map_ReportSM_DeliveryStatusArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 50: /*activateTraceMode*/
    offset=dissect_gsm_map_ActivateTraceModeArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 51: /*deactivateTraceMode*/
    offset=dissect_gsm_map_DeactivateTraceModeArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 55: /*sendIdentification*/
    offset=dissect_gsm_map_SendIdentificationArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 56: /*sendAuthenticationInfo*/
	  if (application_context_version < 3 ){
		  offset=dissect_gsm_map_SendAuthenticationInfoArg(FALSE, tvb, offset, pinfo, tree, hf_gsm_map_SendAuthenticationInfoArg);
	  }else{
		  offset=dissect_gsm_map_SendAuthenticationInfoArgV2(FALSE, tvb, offset, pinfo, tree, hf_gsm_map_SendAuthenticationInfoArg);
	  }
	break;
  case 57: /*restoreData*/
	offset=dissect_gsm_map_RestoreDataArg(FALSE, tvb, offset, pinfo, tree, -1);
	break;
  case 58: /*sendIMSI*/
	offset = dissect_gsm_map_ISDN_AddressString(FALSE, tvb, offset, pinfo, tree, hf_gsm_map_msisdn);
	break;
  case 59: /*processUnstructuredSS-Request*/
    offset=dissect_gsm_map_Ussd_Arg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 60: /*unstructuredSS-Request*/
    offset=dissect_gsm_map_Ussd_Arg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 61: /*unstructuredSS-Notify*/
    offset=dissect_gsm_map_Ussd_Arg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 62: /*AnyTimeSubscriptionInterrogation*/
	  offset=dissect_gsm_map_AnyTimeSubscriptionInterrogationArg(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 63: /*informServiceCentre*/
    offset=dissect_gsm_map_InformServiceCentreArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 64: /*alertServiceCentre*/
    offset=dissect_gsm_map_AlertServiceCentreArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 65: /*AnyTimeModification*/
	  offset=dissect_gsm_map_AnyTimeModificationArg(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 66: /*readyForSM*/
    offset=dissect_gsm_map_ReadyForSM_Arg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 67: /*purgeMS*/
	/* XXX  asn2eth can not yet handle tagged assignment yes so this
	 * XXX is some conformance file magic to work around that bug
	 */
	offset = get_ber_identifier(tvb, offset, &bug_class, &bug_pc, &bug_tag);
	offset = get_ber_length(tree, tvb, offset, &bug_len1, &bug_ind_field);
    offset=dissect_gsm_map_PurgeMSArg(TRUE, tvb, offset, pinfo, tree, -1);
    break;
  case 68: /*prepareHandover*/
	octet = tvb_get_guint8(tvb,0) & 0xf;
	if ( octet == 3){ /* This is a V3 message ??? */ 
		/* XXX  asn2eth can not yet handle tagged assignment yes so this
		 * XXX is some conformance file magic to work around that bug
		 */
		offset = get_ber_identifier(tvb, offset, &bug_class, &bug_pc, &bug_tag);
		offset = get_ber_length(tree, tvb, offset, &bug_len1, &bug_ind_field);
		offset=dissect_gsm_map_PrepareHO_ArgV3(TRUE, tvb, offset, pinfo, tree, -1);
	}else{
		offset=dissect_gsm_map_PrepareHO_Arg(FALSE, tvb, offset, pinfo, tree, -1);
	}
    break;
  case 69: /*prepareSubsequentHandover*/
    offset=dissect_gsm_map_PrepareSubsequentHOArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 70: /*provideSubscriberInfo*/
    offset=dissect_gsm_map_ProvideSubscriberInfoArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 71: /*anyTimeInterrogation*/
    offset=dissect_gsm_map_AnyTimeInterrogationArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 72: /*ss-InvocationNotificatio*/
    offset=dissect_gsm_map_Ss_InvocationNotificationArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 73: /*setReportingState*/
    offset=dissect_gsm_map_SetReportingStateArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 74: /*statusReport*/
    offset=dissect_gsm_map_StatusReportArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 75: /*remoteUserFree*/
    offset=dissect_gsm_map_RemoteUserFreeArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 76: /*registerCC-Entry*/
    offset=dissect_gsm_map_RegisterCC_EntryArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 77: /*eraseCC-Entry*/
    offset=dissect_gsm_map_EraseCC_EntryArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 78: /*secureTransportClass1*/
  case 79: /*secureTransportClass1*/
  case 80: /*secureTransportClass1*/
  case 81: /*secureTransportClass1*/
    offset=dissect_gsm_map_SecureTransportArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 83: /*provideSubscriberLocation*/
    offset=dissect_gsm_map_ProvideSubscriberLocation_Arg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 85: /*sendRoutingInfoForLCS*/
    offset=dissect_gsm_map_RoutingInfoForLCS_Arg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 86: /*subscriberLocationReport*/
    offset=dissect_gsm_map_SubscriberLocationReport_Arg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 87: /*ist-Alert*/
    offset=dissect_gsm_map_IST_AlertArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 88: /*ist-Command*/
    offset=dissect_gsm_map_IST_CommandArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 89: /*noteMM-Event*/
    offset=dissect_gsm_map_NoteMM_EventArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  default:
    proto_tree_add_text(tree, tvb, offset, -1, "Unknown invokeData blob");
  }
  return offset;
}


static int dissect_returnResultData(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {

 gint8 bug_class;
 gboolean bug_pc, bug_ind_field;
 gint32 bug_tag;
 guint32 bug_len1;
	
  guint8 octet;
  switch(opcode){
  case  2: /*updateLocation*/
	octet = tvb_get_guint8(tvb,offset);
	/* As it seems like SEQUENCE OF sometimes is omitted, find out if it's there */
	if ( octet == 0x30 ){ /* Class 0 Univerasl, P/C 1 Constructed,Tag 16 Sequence OF */
		offset=dissect_gsm_map_UpdateLocationRes(FALSE, tvb, offset, pinfo, tree, -1);
	}else{ /* Try decoding with IMPLICIT flag set */ 
		offset=dissect_gsm_map_UpdateLocationRes(TRUE, tvb, offset, pinfo, tree, -1);
	  }
    break;
  case  3: /*cancelLocation*/
    offset=dissect_gsm_map_CancelLocationRes(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case  4: /*provideRoamingNumber*/
    offset=dissect_gsm_map_ProvideRoamingNumberRes(FALSE, tvb, offset, pinfo, tree, -1); /* TRUE florent */
    break;
  case  6: /*resumeCallHandling*/
    offset=dissect_gsm_map_ResumeCallHandlingRes(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case  7: /*insertSubscriberData*/
    offset=dissect_gsm_map_InsertSubscriberDataRes(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case  8: /*deleteSubscriberData*/
    offset=dissect_gsm_map_DeleteSubscriberDataRes(FALSE, tvb, offset, pinfo, tree, -1);
    break;
	/* TODO find out why this isn't in the ASN1 file
  case  9: sendParameters
    offset=dissect_gsm_map_DeleteSubscriberDataArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
	*/
  case  10: /*registerSS*/
    offset=dissect_gsm_map_SS_Info(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case  11: /*eraseSS*/
    offset=dissect_gsm_map_SS_Info(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 12: /*activateSS*/
    offset=dissect_gsm_map_SS_Info(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 13: /*deactivateSS*/
    offset=dissect_gsm_map_SS_Info(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 14: /*interrogateSS*/
    offset=dissect_gsm_map_InterrogateSS_Res(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 15: /*authenticationFailureReport*/
	offset=dissect_gsm_map_AuthenticationFailureReportRes(FALSE, tvb, offset, pinfo, tree, -1);
	break;
  case 17: /*registerPassword*/
    offset=dissect_gsm_map_NewPassword(FALSE, tvb, offset, pinfo, tree, hf_gsm_map_ss_Code);
    break;
  case 18: /*getPassword*/
    offset=dissect_gsm_map_CurrentPassword(FALSE, tvb, offset, pinfo, tree, hf_gsm_map_currentPassword);
    break;
  case 20: /*releaseResources*/
    offset=dissect_gsm_map_ReleaseResourcesRes(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 22: /*sendRoutingInfo*/
	  /* This is done to get around a problem with IMPLICIT tag:s */
	offset = get_ber_identifier(tvb, offset, &bug_class, &bug_pc, &bug_tag);
	offset = get_ber_length(tree, tvb, offset, &bug_len1, &bug_ind_field);
    offset=dissect_gsm_map_SendRoutingInfoRes(TRUE, tvb, offset, pinfo, tree, -1);
    break;
  case 23: /*updateGprsLocation*/
    offset=dissect_gsm_map_UpdateGprsLocationRes(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 24: /*sendRoutingInfoForGprs*/
    offset=dissect_gsm_map_SendRoutingInfoForGprsRes(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 25: /*failureReport*/
    offset=dissect_gsm_map_FailureReportRes(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 26: /*noteMsPresentForGprs*/
    offset=dissect_gsm_map_NoteMsPresentForGprsRes(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 29: /*sendEndSignal*/
	  /* Taken from MAP-MobileServiceOperations{ 0 identified-organization (4) etsi (0) mobileDomain 
	   * (0) gsm-Network (1) modules (3) map-MobileServiceOperations (5) version9 (9) }
	   */
    offset=dissect_gsm_map_SendEndSignalRes(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 31: /*provideSIWFSNumbe*/
    offset=dissect_gsm_map_ProvideSIWFSNumberRes(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 32: /*provideSIWFSNumbe*/
    offset=dissect_gsm_map_SIWFSSignallingModifyRes(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 39: /*prepareGroupCall*/
    offset=dissect_gsm_map_PrepareGroupCallRes(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 40: /*sendGroupCallEndSignal*/
    dissect_gsm_map_SendGroupCallEndSignalRes(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 43: /*checkIMEI*/
	if (application_context_version < 3 ){
		offset = dissect_gsm_map_EquipmentStatus(FALSE, tvb, offset, pinfo, tree, hf_gsm_map_equipmentStatus);
	}else{
		offset=dissect_gsm_map_CheckIMEIRes(FALSE, tvb, offset, pinfo, tree, -1);
	}
    break;
  case 44: /*mt-forwardSM*/
    offset=dissect_gsm_map_Mt_forwardSM_Res(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 45: /*sendRoutingInfoForSM*/
    offset=dissect_gsm_map_RoutingInfoForSM_Res(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 46: /*mo-forwardSM*/
    offset=dissect_gsm_map_Mo_forwardSM_Res(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 48: /*reportSM-DeliveryStatus*/
    offset=dissect_gsm_map_ReportSM_DeliveryStatusRes(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 50: /*activateTraceMode*/
    offset=dissect_gsm_map_ActivateTraceModeRes(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 51: /*deactivateTraceMode*/
    offset=dissect_gsm_map_DeactivateTraceModeRes(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 55: /*sendIdentification
			* In newer versions IMSI and authenticationSetList is OPTIONAL and two new parameters added
			* however if the tag (3) is stripped of it should work with the 'new' def.(?) 
			*/
	octet = tvb_get_guint8(tvb,0) & 0xf;
	if ( octet == 3){ /* This is a V3 message ??? */ 
	  /* XXX  asn2eth can not yet handle tagged assignment yes so this
	   * XXX is some conformance file magic to work around that bug
	   */
	  offset = get_ber_identifier(tvb, offset, &bug_class, &bug_pc, &bug_tag);
	  offset = get_ber_length(tree, tvb, offset, &bug_len1, &bug_ind_field);
	}
	offset=dissect_gsm_map_SendIdentificationRes(TRUE, tvb, offset, pinfo, tree, -1);
    break;
  case 56: /*sendAuthenticationInfo*/
    octet = tvb_get_guint8(tvb,0) & 0xf;
    if ( octet == 3){ /* This is a V3 message ??? */
      /* XXX  asn2eth can not yet handle tagged assignment yes so this
       * XXX is some conformance file magic to work around that bug
       */
		offset = get_ber_identifier(tvb, offset, &bug_class, &bug_pc, &bug_tag);
		offset = get_ber_length(tree, tvb, offset, &bug_len1, &bug_ind_field);
 
		offset=dissect_gsm_map_SendAuthenticationInfoResV3(TRUE, tvb, offset, pinfo, tree, hf_gsm_map_SendAuthenticationInfoRes);
	}else{
		offset=dissect_gsm_map_SendAuthenticationInfoRes(FALSE, tvb, offset, pinfo, tree, -1);
	}
	break;
  case 57: /*restoreData*/
    offset=dissect_gsm_map_RestoreDataRes(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 58: /*sendIMSI*/
    offset=dissect_gsm_map_IMSI(FALSE, tvb, offset, pinfo, tree, hf_gsm_map_imsi);
    break;
  case 59: /*unstructuredSS-Request*/
    offset=dissect_gsm_map_Ussd_Res(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 60: /*unstructuredSS-Request*/
    offset=dissect_gsm_map_Ussd_Res(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 61: /*unstructuredSS-Notify*/
    /* TRUE ? */
    proto_tree_add_text(tree, tvb, offset, -1, "Unknown returnResultData blob");
    break;
  case 62: /*AnyTimeSubscriptionInterrogation*/
	offset=dissect_gsm_map_AnyTimeSubscriptionInterrogationRes(FALSE, tvb, offset, pinfo, tree, -1);
	break;
  case 65: /*AnyTimeModification*/
	offset=dissect_gsm_map_AnyTimeModificationRes(FALSE, tvb, offset, pinfo, tree, -1);
	break;
  case 66: /*readyForSM*/
    offset=dissect_gsm_map_ReadyForSM_Res(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 67: /*purgeMS*/
    offset=dissect_gsm_map_PurgeMSRes(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 68: /*prepareHandover*/
	octet = tvb_get_guint8(tvb,0) & 0xf;
	if ( octet == 3){ /* This is a V3 message ??? */
	  /* XXX  asn2eth can not yet handle tagged assignment yes so this
	   * XXX is some conformance file magic to work around that bug
	   */
		offset = get_ber_identifier(tvb, offset, &bug_class, &bug_pc, &bug_tag);
		offset = get_ber_length(tree, tvb, offset, &bug_len1, &bug_ind_field);
		offset=dissect_gsm_map_PrepareHO_ResV3(TRUE, tvb, offset, pinfo, tree, hf_gsm_mapSendEndSignal);
	}else{
		offset=dissect_gsm_map_PrepareHO_Res(FALSE, tvb, offset, pinfo, tree, -1);
	}
    break;
  case 69: /*prepareSubsequentHandover*/
	offset=dissect_gsm_map_Bss_APDU(FALSE, tvb, offset, pinfo, tree, -1);
	break;
  case 70: /*provideSubscriberInfo*/
    offset=dissect_gsm_map_ProvideSubscriberInfoRes(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 71: /*anyTimeInterrogation*/
    offset=dissect_gsm_map_AnyTimeInterrogationRes(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 72: /*ss-InvocationNotificatio*/
    offset=dissect_gsm_map_Ss_InvocationNotificationRes(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 73: /*setReportingState*/
    offset=dissect_gsm_map_SetReportingStateRes(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 74: /*statusReport*/
    offset=dissect_gsm_map_StatusReportRes(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 75: /*remoteUserFree*/
    offset=dissect_gsm_map_RemoteUserFreeRes(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 76: /*registerCC-Entry*/
    offset=dissect_gsm_map_RegisterCC_EntryRes(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 77: /*eraseCC-Entry*/
    offset=dissect_gsm_map_EraseCC_EntryRes(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 78: /*secureTransportClass1*/
  case 79: /*secureTransportClass2*/
  case 80: /*secureTransportClass3*/
  case 81: /*secureTransportClass4*/
    offset=dissect_gsm_map_SecureTransportRes(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 83: /*provideSubscriberLocation*/
    offset=dissect_gsm_map_ProvideSubscriberLocation_Res(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 85: /*sendRoutingInfoForLCS*/
    offset=dissect_gsm_map_RoutingInfoForLCS_Arg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 86: /*subscriberLocationReport*/
    offset=dissect_gsm_map_SubscriberLocationReport_Res(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 87: /*ist-Alert*/
    offset=dissect_gsm_map_IST_AlertRes(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 88: /*ist-Command*/
    offset=dissect_gsm_map_IST_CommandRes(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 89: /*noteMM-Event*/
    offset=dissect_gsm_map_NoteMM_EventRes(FALSE, tvb, offset, pinfo, tree, -1);
    break;
 default:
    proto_tree_add_text(tree, tvb, offset, -1, "Unknown returnResultData blob");
  }
  return offset;
}



static int dissect_returnErrorData(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
	
  switch(errorCode){
  case 1: /* UnknownSubscriberParam */
	  offset=dissect_gsm_map_UnknownSubscriberParam(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 4: /* SecureTransportErrorParam */
	  offset=dissect_gsm_map_SecureTransportErrorParam(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 5: /* UnidentifiedSubParam */
	  offset=dissect_gsm_map_UnidentifiedSubParam(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 6: /* AbsentSubscriberSM-Param */
	  offset=dissect_gsm_map_AbsentSubscriberSM_Param(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 8: /* RoamingNotAllowedParam */
	  offset=dissect_gsm_map_RoamingNotAllowedParam(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 9: /* IllegalSubscriberParam */
	  offset=dissect_gsm_map_IllegalSubscriberParam(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 10: /* BearerServNotProvParam */
	  offset=dissect_gsm_map_BearerServNotProvParam(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 11: /* TeleservNotProvParam */
	  offset=dissect_gsm_map_TeleservNotProvParam(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 12: /* IllegalEquipmentParam */
	  offset=dissect_gsm_map_IllegalEquipmentParam(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 13: /* CallBarredParam */
	  offset=dissect_gsm_map_CallBarredParam(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 14: /* ForwardingViolationParam */
	  offset=dissect_gsm_map_ForwardingViolationParam(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 15: /* CUG-RejectParam */
	  offset=dissect_gsm_map_CUG_RejectParam(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 16: /* IllegalSS-OperationParam */
	  offset=dissect_gsm_map_IllegalSS_OperationParam(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 18: /* SS-NotAvailableParam */
	  offset=dissect_gsm_map_SS_NotAvailableParam(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 19: /* SS-SubscriptionViolationParam */
	  offset=dissect_gsm_map_SS_SubscriptionViolationParam(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 20: /* SS-IncompatibilityCause */
	  offset=dissect_gsm_map_SS_IncompatibilityCause(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 21: /* FacilityNotSupParam */
	  offset=dissect_gsm_map_FacilityNotSupParam(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 27: /* AbsentSubscriberParam */
	  offset=dissect_gsm_map_AbsentSubscriberParam(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 28: /* IncompatibleTerminalParam */
	  offset=dissect_gsm_map_IncompatibleTerminalParam(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 29: /* ShortTermDenialParam */
	  offset=dissect_gsm_map_ShortTermDenialParam(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 30: /* LongTermDenialParam */
	  offset=dissect_gsm_map_LongTermDenialParam(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 31: /* SubBusyForMT-SMS-Param */
	  offset=dissect_gsm_map_SubBusyForMT_SMS_Param(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 32: /* SM-DeliveryFailureCause */
	  offset=dissect_gsm_map_SM_DeliveryFailureCause(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 33: /* MessageWaitListFullParam */
	  offset=dissect_gsm_map_MessageWaitListFullParam(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 34: /* SystemFailureParam */
	  offset=dissect_gsm_map_SystemFailureParam(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 35: /* DataMissingParam */
	  offset=dissect_gsm_map_DataMissingParam(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 36: /* UnexpectedDataParam */
	  offset=dissect_gsm_map_UnexpectedDataParam(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 37: /* PW-RegistrationFailureCause */
	  offset=dissect_gsm_map_PW_RegistrationFailureCause(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 39: /* NoRoamingNbParam */
	  offset=dissect_gsm_map_NoRoamingNbParam(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 40: /* TracingBufferFullParam */
	  offset=dissect_gsm_map_TracingBufferFullParam(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 42: /* TargetCellOutsideGCA-Param */
	  offset=dissect_gsm_map_TargetCellOutsideGCA_Param(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 44: /* NumberChangedParam */
	  offset=dissect_gsm_map_NumberChangedParam(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 45: /* BusySubscriberParam */
	  offset=dissect_gsm_map_BusySubscriberParam(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 46: /* NoSubscriberReplyParam */
	  offset=dissect_gsm_map_NoSubscriberReplyParam(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 47: /* ForwardingFailedParam */
	  offset=dissect_gsm_map_ForwardingFailedParam(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 48: /* OR-NotAllowedParam */
	  offset=dissect_gsm_map_OR_NotAllowedParam(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 49: /* ATI-NotAllowedParam */
	  offset=dissect_gsm_map_ATI_NotAllowedParam(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 50: /* NoGroupCallNbParam */
	  offset=dissect_gsm_map_NoGroupCallNbParam(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 51: /* ResourceLimitationParam */
	  offset=dissect_gsm_map_ResourceLimitationParam(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 52: /* UnauthorizedRequestingNetwork-Param */
	  offset=dissect_gsm_map_UnauthorizedRequestingNetwork_Param(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 53: /* UnauthorizedLCSClient-Param */
	  offset=dissect_gsm_map_UnauthorizedLCSClient_Param(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 54: /* PositionMethodFailure-Param */
	  offset=dissect_gsm_map_PositionMethodFailure_Param(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 58: /* UnknownOrUnreachableLCSClient-Param */
	  offset=dissect_gsm_map_UnknownOrUnreachableLCSClient_Param(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 59: /* MM-EventNotSupported-Param */
	  offset=dissect_gsm_map_MM_EventNotSupported_Param(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 60: /* ATSI-NotAllowedParam */
	  offset=dissect_gsm_map_ATSI_NotAllowedParam(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 61: /* ATM-NotAllowedParam */
	  offset=dissect_gsm_map_ATM_NotAllowedParam(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  case 62: /* InformationNotAvailableParam */
	  offset=dissect_gsm_map_InformationNotAvailableParam(FALSE, tvb, offset, pinfo, tree, -1);
	  break;
  default:
	  proto_tree_add_text(tree, tvb, offset, -1, "Unknown returnErrorData blob");
	  break;
  }
  return offset;
}
static guint8 gsmmap_pdu_type = 0;
static guint8 gsm_map_pdu_size = 0;

static int
dissect_gsm_map_GSMMAPPDU(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo , proto_tree *tree, int hf_index) {

  char *version_ptr;

  opcode = 0;
  application_context_version = 0;
  if (pinfo->private_data != NULL){
    version_ptr = strrchr(pinfo->private_data,'.');
	if (version_ptr) {
		application_context_version = atoi(version_ptr+1);
	}
  }

  gsmmap_pdu_type = tvb_get_guint8(tvb, offset)&0x0f;
  /* Get the length and add 2 */
  gsm_map_pdu_size = tvb_get_guint8(tvb, offset+1)+2;

  if (check_col(pinfo->cinfo, COL_INFO)){
    col_set_str(pinfo->cinfo, COL_INFO, val_to_str(gsmmap_pdu_type, gsm_map_Component_vals, "Unknown GSM-MAP PDU (%u)"));
	col_append_fstr(pinfo->cinfo, COL_INFO, " ");
  }
  offset = dissect_gsm_map_Component(FALSE, tvb, 0, pinfo, tree, hf_gsm_map_Component_PDU);
  return offset;
/*
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              GSMMAPPDU_choice, hf_index, ett_gsm_map_GSMMAPPDU, NULL);
*/

  return offset;
}




static void
dissect_gsm_map(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree)
{
    proto_item		*item=NULL;
    proto_tree		*tree=NULL;
    /* Used for gsm_map TAP */
    static		gsm_map_tap_rec_t tap_rec;
    gint		op_idx;


    if (check_col(pinfo->cinfo, COL_PROTOCOL))
    {
	col_set_str(pinfo->cinfo, COL_PROTOCOL, "GSM MAP");
    }

    top_tree = parent_tree;

    /* create display subtree for the protocol */
    if(parent_tree){
        item = proto_tree_add_item(parent_tree, proto_gsm_map, tvb, 0, -1, FALSE);
        tree = proto_item_add_subtree(item, ett_gsm_map);
    }

    dissect_gsm_map_GSMMAPPDU(FALSE, tvb, 0, pinfo, tree, -1);
    match_strval_idx(opcode, gsm_map_opr_code_strings, &op_idx);

    tap_rec.invoke = FALSE;
    if ( gsmmap_pdu_type  == 1 )
	tap_rec.invoke = TRUE;
    tap_rec.opr_code_idx = op_idx;
    tap_rec.size = gsm_map_pdu_size;

    tap_queue_packet(gsm_map_tap, pinfo, &tap_rec);
}

const value_string ssCode_vals[] = {
  { 0x00, "allSS - all SS" },
  { 0x10 ,"allLineIdentificationSS - all line identification SS" },
  { 0x11 ,"clip - calling line identification presentation" },
  { 0x12 ,"clir - calling line identification restriction" },
  { 0x13 ,"colp - connected line identification presentation" },
  { 0x14 ,"colr - connected line identification restriction" },
  { 0x15 ,"mci - malicious call identification" },
  { 0x18 ,"allNameIdentificationSS - all name indentification SS" },
  { 0x19 ,"cnap - calling name presentation" },
  { 0x20 ,"allForwardingSS - all forwarding SS" },
  { 0x21 ,"cfu - call forwarding unconditional" },
  { 0x28 ,"allCondForwardingSS - all conditional forwarding SS" },
  { 0x29 ,"cfb - call forwarding busy" },
  { 0x2a ,"cfnry - call forwarding on no reply" },
  { 0x2b ,"cfnrc - call forwarding on mobile subscriber not reachable" },
  { 0x24 ,"cd - call deflection" },
  { 0x30 ,"allCallOfferingSS - all call offering SS includes also all forwarding SS" },
  { 0x31 ,"ect - explicit call transfer" },
  { 0x32 ,"mah - mobile access hunting" },
  { 0x40 ,"allCallCompletionSS - all Call completion SS" },
  { 0x41 ,"cw - call waiting" },
  { 0x42 ,"hold - call hold" },
  { 0x43 ,"ccbs-A - completion of call to busy subscribers, originating side" },
  { 0x44 ,"ccbs-B - completion of call to busy subscribers, destination side" },
  { 0x45 ,"mc - multicall" },
  { 0x50 ,"allMultiPartySS - all multiparty SS" },
  { 0x51 ,"multiPTY - multiparty" },
  { 0x60 ,"allCommunityOfInterestSS - all community of interest SS" },
  { 0x61 ,"cug - closed user group" },
  { 0x70 ,"allChargingSS - all charging SS" },
  { 0x71 ,"aoci - advice of charge information" },
  { 0x72 ,"aocc - advice of charge charging" },
  { 0x80 ,"allAdditionalInfoTransferSS - all additional information transfer SS" },
  { 0x81 ,"uus1 - UUS1 user-to-user signalling" },
  { 0x82 ,"uus2 - UUS2 user-to-user signalling" },
  { 0x83 ,"uus3 - UUS3 user-to-user signalling" },
  { 0x90 ,"allCallRestrictionSS - all Callrestriction SS" },
  { 0x91 ,"barringOfOutgoingCalls" },
  { 0x92 ,"baoc - barring of all outgoing calls" },
  { 0x93 ,"boic - barring of outgoing international calls" },
  { 0x94 ,"boicExHC - barring of outgoing international calls except those directed to the home PLMN" },
  { 0x99 ,"barringOfIncomingCalls" },
  { 0x9a ,"baic - barring of all incoming calls" },
  { 0x9b ,"bicRoam - barring of incoming calls when roaming outside home PLMN Country" },
  { 0xf0 ,"allPLMN-specificSS" },
  { 0xa0 ,"allCallPrioritySS - all call priority SS" },
  { 0xa1 ,"emlpp - enhanced Multilevel Precedence Pre-emption (EMLPP) service" },
  { 0xb0 ,"allLCSPrivacyException - all LCS Privacy Exception Classes" },
  { 0xb1 ,"universal - allow location by any LCS client" },
  { 0xb2 ,"callrelated - allow location by any value added LCS client to which a call is established from the target MS" },
  { 0xb3 ,"callunrelated - allow location by designated external value added LCS clients" },
  { 0xb4 ,"plmnoperator - allow location by designated PLMN operator LCS clients" },
  { 0xc0 ,"allMOLR-SS - all Mobile Originating Location Request Classes" },
  { 0xc1 ,"basicSelfLocation - allow an MS to request its own location" },
  { 0xc2 ,"autonomousSelfLocation - allow an MS to perform self location without interaction with the PLMN for a predetermined period of time" },
  { 0xc3 ,"transferToThirdParty - allow an MS to request transfer of its location to another LCS client" },

  { 0xf1 ,"plmn-specificSS-1" },
  { 0xf2 ,"plmn-specificSS-2" },
  { 0xf3 ,"plmn-specificSS-3" },
  { 0xf4 ,"plmn-specificSS-4" },
  { 0xf5 ,"plmn-specificSS-5" },
  { 0xf6 ,"plmn-specificSS-6" },
  { 0xf7 ,"plmn-specificSS-7" },
  { 0xf8 ,"plmn-specificSS-8" },
  { 0xf9 ,"plmn-specificSS-9" },
  { 0xfa ,"plmn-specificSS-a" },
  { 0xfb ,"plmn-specificSS-b" },
  { 0xfc ,"plmn-specificSS-c" },
  { 0xfd ,"plmn-specificSS-d" },
  { 0xfe ,"plmn-specificSS-e" },
  { 0xff ,"plmn-specificSS-f" },
  { 0, NULL }
};

static const value_string Teleservice_vals[] = {
{0x00, "allTeleservices" },
{0x10, "allSpeechTransmissionServices" },
{0x11, "telephony" },
{0x12, "emergencyCalls" },
{0x20, "allShortMessageServices" },
{0x21, "shortMessageMT-PP" },
{0x22, "shortMessageMO-PP" },
{0x60, "allFacsimileTransmissionServices" },
{0x61, "facsimileGroup3AndAlterSpeech" },
{0x62, "automaticFacsimileGroup3" },
{0x63, "facsimileGroup4" },

{0x70, "allDataTeleservices" },
{0x80, "allTeleservices-ExeptSMS" },

{0x90, "allVoiceGroupCallServices" },
{0x91, "voiceGroupCall" },
{0x92, "voiceBroadcastCall" },

{0xd0, "allPLMN-specificTS" },
{0xd1, "plmn-specificTS-1" },
{0xd2, "plmn-specificTS-2" },
{0xd3, "plmn-specificTS-3" },
{0xd4, "plmn-specificTS-4" },
{0xd5, "plmn-specificTS-5" },
{0xd6, "plmn-specificTS-6" },
{0xd7, "plmn-specificTS-7" },
{0xd8, "plmn-specificTS-8" },
{0xd9, "plmn-specificTS-9" },
{0xda, "plmn-specificTS-A" },
{0xdb, "plmn-specificTS-B" },
{0xdc, "plmn-specificTS-C" },
{0xdd, "plmn-specificTS-D" },
{0xde, "plmn-specificTS-E" },
{0xdf, "plmn-specificTS-F" },
  { 0, NULL }
};

static const value_string Bearerservice_vals[] = {
{0x00, "allBearerServices" },
{0x10, "allDataCDA-Services" },
{0x11, "dataCDA-300bps" },
{0x12, "dataCDA-1200bps" },
{0x13, "dataCDA-1200-75bps" },
{0x14, "dataCDA-2400bps" },
{0x15, "dataCDA-4800bps" },
{0x16, "dataCDA-9600bps" },
{0x17, "general-dataCDA" },

{0x18, "allDataCDS-Services" },
{0x1A, "dataCDS-1200bps" },
{0x1C, "dataCDS-2400bps" },
{0x1D, "dataCDS-4800bps" },
{0x1E, "dataCDS-9600bps" },
{0x1F, "general-dataCDS" },

{0x20, "allPadAccessCA-Services" },
{0x21, "padAccessCA-300bps" },
{0x22, "padAccessCA-1200bps" },
{0x23, "padAccessCA-1200-75bps" },
{0x24, "padAccessCA-2400bps" },
{0x25, "padAccessCA-4800bps" },
{0x26, "padAccessCA-9600bps" },
{0x27, "general-padAccessCA" },

{0x28, "allDataPDS-Services" },
{0x2C, "dataPDS-2400bps" },
{0x2D, "dataPDS-4800bps" },
{0x2E, "dataPDS-9600bps" },
{0x2F, "general-dataPDS" },

{0x30, "allAlternateSpeech-DataCDA" },
{0x38, "allAlternateSpeech-DataCDS" },
{0x40, "allSpeechFollowedByDataCDA" },
{0x48, "allSpeechFollowedByDataCDS" },

{0x50, "allDataCircuitAsynchronous" },
{0x60, "allAsynchronousServices" },
{0x58, "allDataCircuitSynchronous" },
{0x68, "allSynchronousServices" },

{0xD0, "allPLMN-specificBS" },
{0xD1, "plmn-specificBS-1" },
{0xD2, "plmn-specificBS-2" },
{0xD3, "plmn-specificBS-3" },
{0xD4, "plmn-specificBS-4" },
{0xD5, "plmn-specificBS-5" },
{0xD6, "plmn-specificBS-6" },
{0xD7, "plmn-specificBS-7" },
{0xD8, "plmn-specificBS-8" },
{0xD9, "plmn-specificBS-9" },
{0xDA, "plmn-specificBS-A" },
{0xDB, "plmn-specificBS-B" },
{0xDC, "plmn-specificBS-C" },
{0xDD, "plmn-specificBS-D" },
{0xDE, "plmn-specificBS-E" },
{0xDF, "plmn-specificBS-F" },

{ 0, NULL }
};

/* ForwardingOptions 

-- bit 8: notification to forwarding party
-- 0 no notification
-- 1 notification
*/
static const true_false_string notification_value  = {
  "Notification",
  "No notification"
};
/*
-- bit 7: redirecting presentation
-- 0 no presentation
-- 1 presentation
*/
static const true_false_string redirecting_presentation_value  = {
  "Presentation",
  "No presentationn"
};
/*
-- bit 6: notification to calling party
-- 0 no notification
-- 1 notification
*/
/*
-- bit 5: 0 (unused)
-- bits 43: forwarding reason
-- 00 ms not reachable
-- 01 ms busy
-- 10 no reply
-- 11 unconditional when used in a SRI Result,
-- or call deflection when used in a RCH Argument
*/
static const value_string forwarding_reason_values[] = {
{0x0, "ms not reachable" },
{0x1, "ms busy" },
{0x2, "no reply" },
{0x3, "unconditional when used in a SRI Result or call deflection when used in a RCH Argument" },
{ 0, NULL }
};
/*
-- bits 21: 00 (unused)
*/

static const value_string pdp_type_org_values[] = {
{0x0, "ETSI" },
{0x1, "IETF" },
{0xf, "Empty PDP type" },
{ 0, NULL }
};

static const value_string etsi_pdp_type_number_values[] = {
{0x0, "Reserved, used in earlier version of this protocol" },
{0x1, "PPP" },
{ 0, NULL }
};

static const value_string ietf_pdp_type_number_values[] = {
{0x21, "IPv4 Address" },
{0x57, "IPv6 Address" },
{ 0, NULL }
};

/*
ChargingCharacteristics ::= OCTET STRING (SIZE (2))
-- Octets are coded according to 3GPP TS 32.015.
-- From 3GPP TS 32.015.
--
-- Descriptions for the bits of the flag set:
--
-- Bit 1: H (Hot billing) := '00000001'B
-- Bit 2: F (Flat rate) := '00000010'B
-- Bit 3: P (Prepaid service) := '00000100'B
-- Bit 4: N (Normal billing) := '00001000'B
-- Bit 5: - (Reserved, set to 0) := '00010000'B
-- Bit 6: - (Reserved, set to 0) := '00100000'B
-- Bit 7: - (Reserved, set to 0) := '01000000'B
-- Bit 8: - (Reserved, set to 0) := '10000000'B
*/
static const value_string chargingcharacteristics_values[] = {
{0x1, "H (Hot billing)" },
{0x2, "F (Flat rate)" },
{0x4, "P (Prepaid service)" },
{0x8, "N (Normal billing)" },
{ 0, NULL }
};
/*--- proto_reg_handoff_gsm_map ---------------------------------------*/
static void range_delete_callback(guint32 ssn)
{
    if (ssn) {
	delete_itu_tcap_subdissector(ssn, map_handle);
    }
}

static void range_add_callback(guint32 ssn)
{
    if (ssn) {
	add_itu_tcap_subdissector(ssn, map_handle);
    }
}

void proto_reg_handoff_gsm_map(void) {

    static int map_prefs_initialized = FALSE;
    data_handle = find_dissector("data");

    if (!map_prefs_initialized) {
	map_prefs_initialized = TRUE;
	map_handle = create_dissector_handle(dissect_gsm_map, proto_gsm_map);
  register_ber_oid_dissector_handle("0.4.0.0.1.0.1.3", map_handle, proto_gsm_map, "itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) networkLocUp(1) version3(3)");  
  register_ber_oid_dissector_handle("0.4.0.0.1.0.1.2", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) networkLocUp(1) version2(2)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.2.3", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) locationCancel(2) version3(3)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.2.2", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) locationCancel(2) version2(2)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.2.1", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) locationCancel(2) version1(1)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.3.3", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) roamingNbEnquiry(3) version3(3)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.3.2", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) roamingNbEnquiry(3) version2(2)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.3.1", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) roamingNbEnquiry(3) version1(1)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.5.3", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) locInfoRetrieval(5) version3(3)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.5.2", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) locInfoRetrieval(5) version2(2)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.5.1", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) locInfoRetrieval(5) version1(1)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.6.4", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) callControlTransfer(6) version4(4)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.7.3", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) reporting(7) version3(3)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.8.3", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) callCompletion(8) version3(3)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.10.2", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) reset(10) version2(2)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.10.1", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) reset(10) version1(1)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.11.3", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) handoverControl(11) version3(3)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.11.2", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) handoverControl(11) version2(2)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.11.1", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) handoverControl(11) version1(1)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.12.3", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) sIWFSAllocation(12) version3(3)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.13.2", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) equipmentMngt(13) version2(2)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.13.1", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) equipmentMngt(13) version1(1)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.14.3", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) infoRetrieval(14) version3(3)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.14.2", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) infoRetrieval(14) version2(2)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.14.1", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) infoRetrieval(14) version1(1)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.15.1", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) interVlrInfoRetrieval(15) version1(1)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.15.2", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) interVlrInfoRetrieval(15) version2(2)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.15.3", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) interVlrInfoRetrieval(15) version3(3)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.16.3", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) subscriberDataMngt(16) version3(3)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.16.2", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) subscriberDataMngt(16) version2(2)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.16.1", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) subscriberDataMngt(16) version1(1)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.17.2", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) tracing(17) version2(2)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.17.1", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) tracing(17) version1(1)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.18.2", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) networkFunctionalSs(18) version2(2)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.18.1", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) networkFunctionalSs(18) version1(1)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.19.1", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) networkUnstructuredSs(19) version2(2)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.20.3", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) shortMsgGateway(20) version3(3)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.20.2", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) shortMsgGateway(20) version2(2)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.20.1", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) shortMsgGateway(20) version1(1)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.21.2", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) shortMsgMO-Relay(21) version2(2)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.21.1", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) --shortMsgRelay--21 version1(1)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.22.3", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) subscriberDataModificationNotification(22) version3(3)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.23.2", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) shortMsgAlert(23) version2(2)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.23.1", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) shortMsgAlert(23) version1(1)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.24.2", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) mwdMngt(24) version2(2)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.24.1", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) mwdMngt(24) version1(1)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.25.3", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) shortMsgMT-Relay(25) version3(3)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.25.2", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) shortMsgMT-Relay(25) version2(2)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.26.2", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) imsiRetrieval(26) version2(2)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.27.2", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) msPurging(27) version2(2)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.27.3", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) msPurging(27) version3(3)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.29.3", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) anyTimeInfoEnquiry(29) version3(3)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.31.2", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) oupCallControl(31) version3(3)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.32.3", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) gprsLocationUpdate(32) version3(3)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.33.4", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) rsLocationInfoRetrieval(33) version4(4)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.34.3", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) failureReport(34) version3(3)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.36.3", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) ss-InvocationNotification(36) version3(3)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.37.3", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) locationSvcGateway(37) version3(3)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.38.3", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) locationSvcEnquiry(38) version3(3)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.39.3", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) authenticationFailureReport(39) version3(3)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.40.3", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) secureTransportHandling(40) version3(3)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.42.3", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) mm-EventReporting(42) version3(3)" );
  register_ber_oid_dissector_handle("0.4.0.0.1.0.43.3", map_handle, proto_gsm_map,"itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) anyTimeInfoHandling(43) version3(3)" );
    }
    else {
	range_foreach(ssn_range, range_delete_callback);
    }

    g_free(ssn_range);
    ssn_range = range_copy(global_ssn_range);

    range_foreach(ssn_range, range_add_callback);

}

/*--- proto_register_gsm_map -------------------------------------------*/
void proto_register_gsm_map(void) {
	module_t *gsm_map_module;

  /* List of fields */
  static hf_register_info hf[] = {
	  /*
    { &hf_gsm_map_invokeCmd,
      { "invokeCmd", "gsm_map.invokeCmd",
        FT_UINT32, BASE_DEC, VALS(gsm_map_opr_code_strings), 0,
        "InvokePDU/invokeCmd", HFILL }},
    { &hf_gsm_map_invokeid,
      { "invokeid", "gsm_map.invokeid",
        FT_INT32, BASE_DEC, NULL, 0,
        "InvokeId/invokeid", HFILL }},
    { &hf_gsm_map_absent,
      { "absent", "gsm_map.absent",
        FT_NONE, BASE_NONE, NULL, 0,
        "InvokeId/absent", HFILL }},
    { &hf_gsm_map_invokeId,
      { "invokeId", "gsm_map.invokeId",
        FT_UINT32, BASE_DEC, VALS(InvokeId_vals), 0,
        "InvokePDU/invokeId", HFILL }},
    { &hf_gsm_map_invoke,
      { "invoke", "gsm_map.invoke",
        FT_NONE, BASE_NONE, NULL, 0,
        "GSMMAPPDU/invoke", HFILL }},
    { &hf_gsm_map_returnResult,
      { "returnResult", "gsm_map.returnResult",
        FT_NONE, BASE_NONE, NULL, 0,
        "GSMMAPPDU/returnResult", HFILL }},
	{&hf_gsm_map_returnResult_result,
      { "returnResult_result", "gsm_map.returnresultresult",
        FT_BYTES, BASE_NONE, NULL, 0,
        "returnResult_result", HFILL }},
	{&hf_gsm_map_returnError_result,
      { "returnError_result", "gsm_map.returnerrorresult",
        FT_UINT32, BASE_DEC, NULL, 0,
        "returnError_result", HFILL }},
	{&hf_gsm_map_returnError,
      { "returnError", "gsm_map.returnError",
        FT_NONE, BASE_NONE, NULL, 0,
        "GSMMAPPDU/returnError", HFILL }},
	{&hf_gsm_map_local_errorCode,
      { "Local Error Code", "gsm_map.localerrorCode",
        FT_UINT32, BASE_DEC, VALS(gsm_map_err_code_string_vals), 0,
        "localerrorCode", HFILL }},
	{&hf_gsm_map_global_errorCode_oid,
      { "Global Error Code OID", "gsm_map.hlobalerrorCodeoid",
        FT_STRING, BASE_NONE, NULL, 0,
        "globalerrorCodeoid", HFILL }},
	{&hf_gsm_map_global_errorCode,
      { "Global Error Code", "gsm_map.globalerrorCode",
        FT_UINT32, BASE_DEC, NULL, 0,
        "globalerrorCode", HFILL }},
    { &hf_gsm_map_getPassword,
      { "Password", "gsm_map.password",
        FT_UINT8, BASE_DEC, VALS(gsm_map_GetPasswordArg_vals), 0,
        "Password", HFILL }},

		*/
	{ &hf_gsm_map_SendAuthenticationInfoArg,
      { "SendAuthenticationInfoArg", "gsm_map.SendAuthenticationInfoArg",
        FT_BYTES, BASE_NONE, NULL, 0,
        "SendAuthenticationInfoArg", HFILL }},
	{ &hf_gsm_map_SendAuthenticationInfoRes,
      { "SendAuthenticationInfoRes", "gsm_map.SendAuthenticationInfoRes",
        FT_BYTES, BASE_NONE, NULL, 0,
        "SendAuthenticationInfoRes", HFILL }},
    { &hf_gsm_map_currentPassword,
      { "currentPassword", "gsm_map.currentPassword",
        FT_STRING, BASE_NONE, NULL, 0,
        "", HFILL }},
	{ &hf_gsm_mapSendEndSignal,
      { "mapSendEndSignalArg", "gsm_map.mapsendendsignalarg",
        FT_BYTES, BASE_NONE, NULL, 0,
        "mapSendEndSignalArg", HFILL }},
	{ &hf_gsm_map_CheckIMEIArg,
      { "gsm_CheckIMEIArg", "gsm_map.CheckIMEIArg",
        FT_BYTES, BASE_NONE, NULL, 0,
        "gsm_CheckIMEIArg", HFILL }},
    { &hf_gsm_map_extension,
      { "Extension", "gsm_map.extension",
        FT_BOOLEAN, 8, TFS(&gsm_map_extension_value), 0x80,
        "Extension", HFILL }},
    { &hf_gsm_map_nature_of_number,
      { "Nature of number", "gsm_map.nature_of_number",
        FT_UINT8, BASE_HEX, VALS(gsm_map_nature_of_number_values), 0x70,
        "Nature of number", HFILL }},
    { &hf_gsm_map_number_plan,
      { "Number plan", "gsm_map.number_plan",
        FT_UINT8, BASE_HEX, VALS(gsm_map_number_plan_values), 0x0f,
        "Number plan", HFILL }},
	{ &hf_gsm_map_isdn_address_digits,
      { "ISDN Address digits", "gsm_map.isdn.address.digits",
        FT_STRING, BASE_NONE, NULL, 0,
        "ISDN Address digits", HFILL }},
	{ &hf_gsm_map_address_digits,
      { "Address digits", "gsm_map.address.digits",
        FT_STRING, BASE_NONE, NULL, 0,
        "Address digits", HFILL }},
	{ &hf_gsm_map_servicecentreaddress_digits,
      { "ServiceCentreAddress digits", "gsm_map.servicecentreaddress_digits",
        FT_STRING, BASE_NONE, NULL, 0,
        "ServiceCentreAddress digits", HFILL }},
	{ &hf_gsm_map_imsi_digits,
      { "Imsi digits", "gsm_map.imsi_digits",
        FT_STRING, BASE_NONE, NULL, 0,
        "Imsi digits", HFILL }},
	{ &hf_gsm_map_Ss_Status_unused,
      { "Unused", "gsm_map.unused",
        FT_UINT8, BASE_HEX, NULL, 0xf0,
        "Unused", HFILL }},
	{ &hf_gsm_map_Ss_Status_q_bit,
      { "Q bit", "gsm_map.ss_status_q_bit",
        FT_BOOLEAN, 8, TFS(&gsm_map_Ss_Status_q_bit_values), 0x08,
        "Q bit", HFILL }},
	{ &hf_gsm_map_Ss_Status_p_bit,
      { "P bit", "gsm_map.ss_status_p_bit",
        FT_BOOLEAN, 8, TFS(&gsm_map_Ss_Status_p_values), 0x04,
        "P bit", HFILL }},
	{ &hf_gsm_map_Ss_Status_r_bit,
      { "R bit", "gsm_map.ss_status_r_bit",
        FT_BOOLEAN, 8, TFS(&gsm_map_Ss_Status_r_values), 0x02,
        "R bit", HFILL }},
	{ &hf_gsm_map_Ss_Status_a_bit,
      { "A bit", "gsm_map.ss_status_a_bit",
        FT_BOOLEAN, 8, TFS(&gsm_map_Ss_Status_a_values), 0x01,
        "A bit", HFILL }},
	{ &hf_gsm_map_notification_to_forwarding_party,
      { "Notification to forwarding party", "gsm_map.notification_to_forwarding_party",
        FT_BOOLEAN, 8, TFS(&notification_value), 0x80,
        "Notification to forwarding party", HFILL }},
	{ &hf_gsm_map_redirecting_presentation,
      { "Redirecting presentation", "gsm_map.redirecting_presentation",
        FT_BOOLEAN, 8, TFS(&redirecting_presentation_value), 0x40,
        "Redirecting presentation", HFILL }},
	{ &hf_gsm_map_notification_to_calling_party,
      { "Notification to calling party", "gsm_map.notification_to_clling_party",
        FT_BOOLEAN, 8, TFS(&notification_value), 0x20,
        "Notification to calling party", HFILL }},
    { &hf_gsm_map_forwarding_reason,
      { "Forwarding reason", "gsm_map.forwarding_reason",
        FT_UINT8, BASE_HEX, VALS(forwarding_reason_values), 0x0c,
        "forwarding reason", HFILL }},
    { &hf_gsm_map_pdp_type_org,
      { "PDP Type Organization", "gsm_map.pdp_type_org",
        FT_UINT8, BASE_HEX, VALS(pdp_type_org_values), 0x0f,
        "PDP Type Organization", HFILL }},
    { &hf_gsm_map_etsi_pdp_type_number,
      { "PDP Type Number", "gsm_map.pdp_type_org",
        FT_UINT8, BASE_HEX, VALS(etsi_pdp_type_number_values), 0,
        "ETSI PDP Type Number", HFILL }},
    { &hf_gsm_map_ietf_pdp_type_number,
      { "PDP Type Number", "gsm_map.ietf_pdp_type_number",
        FT_UINT8, BASE_HEX, VALS(ietf_pdp_type_number_values), 0,
        "IETF PDP Type Number", HFILL }},
    { &hf_gsm_map_ext_qos_subscribed_pri,
      { "Allocation/Retention priority", "gsm_map.ext_qos_subscribed_pri",
        FT_UINT8, BASE_DEC, NULL, 0xff,
        "Allocation/Retention priority", HFILL }},
    { &hf_gsm_map_qos_traffic_cls,
      { "Traffic class", "gsm_map.qos.traffic_cls",
        FT_UINT8, BASE_DEC, VALS(gsm_a_qos_traffic_cls_vals), 0xe0,
        "Traffic class", HFILL }},
    { &hf_gsm_map_qos_del_order,
      { "Delivery order", "gsm_map.qos.del_order",
        FT_UINT8, BASE_DEC, VALS(gsm_a_qos_traffic_cls_vals), 0x18,
        "Delivery order", HFILL }},
    { &hf_gsm_map_qos_del_of_err_sdu,
      { "Delivery of erroneous SDUs", "gsm_map.qos.del_of_err_sdu",
        FT_UINT8, BASE_DEC, VALS(gsm_a_qos_del_of_err_sdu_vals), 0x03,
        "Delivery of erroneous SDUs", HFILL }},
    { &hf_gsm_map_qos_ber,
      { "Residual Bit Error Rate (BER)", "gsm_map.qos.ber",
        FT_UINT8, BASE_DEC, VALS(gsm_a_qos_ber_vals), 0xf0,
        "Residual Bit Error Rate (BER)", HFILL }},
    { &hf_gsm_map_qos_sdu_err_rat,
      { "SDU error ratio", "gsm_map.qos.sdu_err_rat",
        FT_UINT8, BASE_DEC, VALS(gsm_a_qos_sdu_err_rat_vals), 0x0f,
        "SDU error ratio", HFILL }},
    { &hf_gsm_map_qos_traff_hdl_pri,
      { "Traffic handling priority", "gsm_map.qos.traff_hdl_pri",
        FT_UINT8, BASE_DEC, VALS(gsm_a_qos_traff_hdl_pri_vals), 0x03,
        "Traffic handling priority", HFILL }},

    { &hf_gsm_map_qos_max_sdu,
      { "Maximum SDU size", "gsm_map.qos.max_sdu",
        FT_UINT32, BASE_DEC, NULL, 0x0,
        "Maximum SDU size", HFILL }},		
    { &hf_gsm_map_max_brate_ulink,
      { "Maximum bit rate for uplink in kbit/s", "gsm_map.qos.max_brate_ulink",
        FT_UINT32, BASE_DEC, NULL, 0x0,
        "Maximum bit rate for uplink", HFILL }},
    { &hf_gsm_map_max_brate_dlink,
      { "Maximum bit rate for downlink in kbit/s", "gsm_map.qos.max_brate_dlink",
        FT_UINT32, BASE_DEC, NULL, 0x0,
        "Maximum bit rate for downlink", HFILL }},
    { &hf_gsm_map_qos_transfer_delay,
      { "Transfer delay (Raw data see TS 24.008 for interpretation)", "gsm_map.qos.transfer_delay",
        FT_UINT8, BASE_DEC, NULL, 0xfc,
        "Transfer delay", HFILL }},
    { &hf_gsm_map_guaranteed_max_brate_ulink,
      { "Guaranteed bit rate for uplink in kbit/s", "gsm_map.qos.brate_ulink",
        FT_UINT32, BASE_DEC, NULL, 0x0,
        "Guaranteed bit rate for uplink", HFILL }},
    { &hf_gsm_map_guaranteed_max_brate_dlink,
      { "Guaranteed bit rate for downlink in kbit/s", "gsm_map.qos.brate_dlink",
        FT_UINT32, BASE_DEC, NULL, 0x0,
        "Guaranteed bit rate for downlink", HFILL }},
   { &hf_gsm_map_GSNAddress_IPv4,
      { "GSN-Address IPv4",  "gsm_map.gsnaddress_ipv4",
	  FT_IPv4, BASE_NONE, NULL, 0,
	  "IPAddress IPv4", HFILL }},
   { &hf_gsm_map_GSNAddress_IPv6,
      { "GSN Address IPv6",  "gsm_map.gsnaddress_ipv6",
	  FT_IPv4, BASE_NONE, NULL, 0,
	  "IPAddress IPv6", HFILL }},

#include "packet-gsm_map-hfarr.c"
  };

  /* List of subtrees */
  static gint *ett[] = {
    &ett_gsm_map,
    &ett_gsm_map_InvokeId,
    &ett_gsm_map_InvokePDU,
    &ett_gsm_map_ReturnResultPDU,
	&ett_gsm_map_ReturnErrorPDU,
    &ett_gsm_map_ReturnResult_result,
	&ett_gsm_map_ReturnError_result,
    &ett_gsm_map_GSMMAPPDU,
	&ett_gsm_map_ext_qos_subscribed,
	&ett_gsm_map_pdptypenumber,
	&ett_gsm_map_RAIdentity,
	&ett_gsm_map_LAIFixedLength,

#include "packet-gsm_map-ettarr.c"
  };

  /* Register protocol */
  proto_gsm_map = proto_register_protocol(PNAME, PSNAME, PFNAME);
/*XXX  register_dissector("gsm_map", dissect_gsm_map, proto_gsm_map);*/
  /* Register fields and subtrees */
  proto_register_field_array(proto_gsm_map, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));

  sms_dissector_table = register_dissector_table("gsm_map.sms_tpdu", 
						 "GSM SMS TPDU", FT_UINT8,
						 BASE_DEC);

  gsm_map_tap = register_tap("gsm_map");

/* #include "packet-gsm_map-dis-tab.c" */
  register_ber_oid_name("1.2.826.0.1249.58.1.0","iso(1) member-body(2) bsi(826) disc(0) ericsson(1249) gsmNetworkApplicationsDefinition(58) gsm-Map(1) gsm-Map-Ext(0)" );
  register_ber_oid_name("1.3.12.2.1107.3.66.1.2","accessTypeNotAllowed-id" );
  /*register_ber_oid_name("0.4.0.0.1.0.1.3","itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) networkLocUp(1) version3(3)" );
   *
   * Register our configuration options, particularly our ssn:s
   * Set default SSNs
   */
  range_convert_str(&global_ssn_range, "6-9", MAX_SSN);
  ssn_range = range_empty();


  gsm_map_module = prefs_register_protocol(proto_gsm_map, proto_reg_handoff_gsm_map);

  prefs_register_range_preference(gsm_map_module, "tcap.ssn", "TCAP SSNs",
				  "TCAP Subsystem numbers used for GSM MAP",
				  &global_ssn_range, MAX_SSN);
}


