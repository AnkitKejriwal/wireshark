/* packet-gsm_map-template.c
 * Routines for GSM MobileApplication packet dissection
 * Copyright 2004, Anders Broman <anders.broman@ericsson.com>
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
#include "packet-q931.h"
#include "packet-gsm_map.h"

#define PNAME  "GSM_MobileAPplication"
#define PSNAME "GSM_MAP"
#define PFNAME "gsm_map"

/* Initialize the protocol and registered fields */
int proto_gsm_map = -1;
static int hf_gsm_map_invokeCmd = -1;             /* Opcode */
static int hf_gsm_map_invokeid = -1;              /* INTEGER */
static int hf_gsm_map_absent = -1;                /* NULL */
static int hf_gsm_map_invokeId = -1;              /* InvokeId */
static int hf_gsm_map_invoke = -1;                /* InvokePDU */
static int hf_gsm_map_returnResult = -1;          /* InvokePDU */
static int hf_gsm_map_returnResult_result = -1;
static int hf_gsm_map_returnError_result = -1;
static int hf_gsm_map_returnError = -1;
static int hf_gsm_map_local_errorCode = -1;
static int hf_gsm_map_global_errorCode_oid = -1;
static int hf_gsm_map_global_errorCode = -1;
static int hf_gsm_map_SendAuthenticationInfoArg = -1;
static int hf_gsm_mapSendEndSignal = -1;
static int hf_gsm_map_getPassword = -1;  
static int hf_gsm_map_currentPassword = -1;
static int hf_gsm_map_extension = -1;
static int hf_gsm_map_nature_of_number = -1;
static int hf_gsm_map_number_plan = -1;
static int hf_gsm_map_misdn_digits = -1;
static int hf_gsm_map_servicecentreaddress_digits = -1;
static int hf_gsm_map_imsi_digits = -1;
static int hf_gsm_map_map_gmsc_address_digits = -1;
static int hf_gsm_map_map_RoamingNumber_digits = -1;
static int hf_gsm_map_map_hlr_number_digits = -1;
static int hf_gsm_map_Ss_Status_unused = -1;
static int hf_gsm_map_Ss_Status_q_bit = -1;
static int hf_gsm_map_Ss_Status_p_bit = -1;
static int hf_gsm_map_Ss_Status_r_bit = -1;
static int hf_gsm_map_Ss_Status_a_bit = -1;

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

#include "packet-gsm_map-ett.c"

static dissector_table_t	sms_dissector_table;	/* SMS TPDU */
/* Preferenc settings default */
static guint tcap_itu_ssn1 = 6;
static guint tcap_itu_ssn2 = 7;
static guint tcap_itu_ssn3 = 8;
static guint tcap_itu_ssn4 = 9;

static guint global_tcap_itu_ssn1 = 6;
static guint global_tcap_itu_ssn2 = 7;
static guint global_tcap_itu_ssn3 = 8;
static guint global_tcap_itu_ssn4 = 9;

/* Global variables */
static proto_tree *top_tree;
int application_context_version;
gint protocolId;
static int gsm_map_tap = -1;


static char*
unpack_digits(tvbuff_t *tvb, int offset){

	int length;
	guint8 octet;
	int i=0;
	char *digit_str;

	length = tvb_length(tvb);
	length = length - offset;
	digit_str = g_malloc(length+1);

	while ( offset <= length ){

		octet = tvb_get_guint8(tvb,offset);
		digit_str[i] = ((octet & 0x0f) + 0x30);
		i++;

		/*
		 * unpack second value in byte
		 */
		octet = octet >> 4;

		if (octet == 0x0f)	/* odd number bytes - hit filler */
			break;

		digit_str[i] = ((octet & 0x0f) + 0x30);
		i++;
		offset++;

	}
	digit_str[i]= '\0';
	return digit_str;
}


#include "packet-gsm_map-fn.c"

/* Stuff included from the "old" packet-gsm_map.c for tapping purposes */
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
/* End includes from old" packet-gsm_map.c */

const value_string gsm_map_opr_code_strings[] = {
  {   2, "updateLocation" },
  {   3, "cancelLocation" },
  {   4, "provideRoamingNumber" },
  {   6, "resumeCallHandling" },
  {   7, "insertSubscriberData" },
  {   8, "deleteSubscriberData" },
  {   9, "sendParameters" },					/* map-ac infoRetrieval (14) version1 (1)*/
  {  10, "registerSS" },
  {  11, "eraseSS" },
  {  12, "activateSS" },
  {  13, "deactivateSS" },
  {  14, "interrogateSS" },
  {  17, "registerPassword" },
  {  18, "getPassword" },
  {  19, "processUnstructuredSS-Data" },		/* map-ac networkFunctionalSs (18) version1 (1) */
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
  {  63, "informServiceCentre" },
  {  64, "alertServiceCentre" },
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
  {  83, "provideSubscriberLocation" },
  {  85, "sendRoutingInfoForLCS" },
  {  86, "subscriberLocationReport" },
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

static guint32 opcode=0;

static int
dissect_gsm_map_Opcode(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_integer(FALSE, pinfo, tree, tvb, offset, hf_index, &opcode);

  if (check_col(pinfo->cinfo, COL_INFO)){
    col_append_fstr(pinfo->cinfo, COL_INFO, val_to_str(opcode, gsm_map_opr_code_strings, "Unknown GSM-MAP (%u)"));
  }

  return offset;
}

static int dissect_invokeData(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {

	guint8 octet;

  switch(opcode){
  case  2: /*updateLocation*/
    offset=dissect_gsm_map_UpdateLocationArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case  3: /*cancelLocation*/
    offset=dissect_gsm_map_CancelLocationArg(FALSE, tvb, offset, pinfo, tree, -1);
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
    offset=dissect_gsm_map_Ss_ForBS(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 12: /*activateSS*/
    offset=dissect_gsm_map_Ss_ForBS(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 13: /*deactivateSS*/
    offset=dissect_gsm_map_Ss_ForBS(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 14: /*interrogateSS*/
    offset=dissect_gsm_map_InterrogateSS_Res(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 17: /*registerPassword*/
    offset=dissect_gsm_map_Ss_Code(FALSE, tvb, offset, pinfo, tree, hf_gsm_map_ss_Code);
    break;
  case 18: /*getPassword*/
    offset=dissect_gsm_map_GetPasswordArg(FALSE, tvb, offset, pinfo, tree, hf_gsm_map_getPassword);
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
	if ( octet == 3){ /* This is a V9 message ??? */
		offset = offset +2;
		offset=dissect_gsm_map_SendEndSignalV9Arg(TRUE, tvb, offset, pinfo, tree, hf_gsm_mapSendEndSignal);
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
    offset=dissect_gsm_map_Bss_APDU(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 34: /*forwardAccessSignalling*/
    offset=dissect_gsm_map_Bss_APDU(FALSE, tvb, offset, pinfo, tree, -1);
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
    offset=dissect_gsm_map_CheckIMEIArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 44: /*mt-forwardSM*/
    offset=dissect_gsm_map_CheckIMEIArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 45: /*sendRoutingInfoForSM*/
    offset=dissect_gsm_map_RoutingInfoForSMRes(FALSE, tvb, offset, pinfo, tree, -1);
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
    offset=dissect_gsm_map_Tmsi(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 56: /*sendAuthenticationInfo*/
	  if (application_context_version < 3 ){
		  offset=dissect_gsm_map_SendAuthenticationInfoArg(FALSE, tvb, offset, pinfo, tree, hf_gsm_map_imsi);
	  }else{
		  offset=dissect_gsm_map_SendAuthenticationInfoArgV3(FALSE, tvb, offset, pinfo, tree, hf_gsm_map_SendAuthenticationInfoArg);
	  }
	break;
  case 57: /*restoreData*/
	offset=dissect_gsm_map_RestoreDataArg(FALSE, tvb, offset, pinfo, tree, -1);
	break;
  case 58: /*sendIMSI*/
	offset=dissect_gsm_map_Msisdn(FALSE, tvb, offset, pinfo, tree, -1);
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
  case 63: /*informServiceCentre*/
    offset=dissect_gsm_map_InformServiceCentreArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 64: /*alertServiceCentre*/
    offset=dissect_gsm_map_AlertServiceCentreArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 66: /*readyForSM*/
    offset=dissect_gsm_map_ReadyForSM_Arg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 67: /*purgeMS*/
    offset=dissect_gsm_map_PurgeMS_Arg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 68: /*prepareHandover*/
    offset=dissect_gsm_map_PrepareHO_Arg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 69: /*prepareSubsequentHandover*/
    offset=dissect_gsm_map_PrepareSubsequentHO_Arg(FALSE, tvb, offset, pinfo, tree, -1);
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
  case 83: /*provideSubscriberLocation*/
    offset=dissect_gsm_map_ProvideSubscriberLocation_Arg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 85: /*sendRoutingInfoForLCS*/
    offset=dissect_gsm_map_RoutingInfoForLCS_Arg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 86: /*subscriberLocationReport*/
    offset=dissect_gsm_map_SubscriberLocationReport_Arg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  default:
    proto_tree_add_text(tree, tvb, offset, -1, "Unknown invokeData blob");
  }
  return offset;
}


static int dissect_returnResultData(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  switch(opcode){
  case  2: /*updateLocation*/
    offset=dissect_gsm_map_UpdateLocationRes(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case  3: /*cancelLocation*/
    offset=dissect_gsm_map_CancelLocationRes(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case  4: /*provideRoamingNumber*/
    offset=dissect_gsm_map_ProvideRoamingNumberRes(FALSE, tvb, offset, pinfo, tree, -1);
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
    offset=dissect_gsm_map_Ss_Info(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case  11: /*eraseSS*/
    offset=dissect_gsm_map_Ss_Info(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 12: /*activateSS*/
    offset=dissect_gsm_map_Ss_Info(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 13: /*deactivateSS*/
    offset=dissect_gsm_map_Ss_Info(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 14: /*interrogateSS*/
    offset=dissect_gsm_map_InterrogateSS_Res(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 17: /*registerPassword*/
    offset=dissect_gsm_map_NewPassword(FALSE, tvb, offset, pinfo, tree, hf_gsm_map_ss_Code);
    break;
  case 18: /*getPassword*/
    offset=dissect_gsm_map_CurrentPassword(FALSE, tvb, offset, pinfo, tree, hf_gsm_map_currentPassword);
    break;
  case 22: /*sendRoutingInfo*/
	  /* This is done to get around a problem with IMPLICIT tag:s */
    offset = offset +2;
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
    offset=dissect_gsm_map_ExtensionContainer(FALSE, tvb, offset, pinfo, tree, -1);
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
    offset=dissect_gsm_map_EquipmentStatus(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 45: /*sendRoutingInfoForSM*/
    offset=dissect_gsm_map_RoutingInfoForSMRes(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 46: /*mo-forwardSM*/
    offset=dissect_gsm_map_Mo_forwardSM_Res(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 48: /*reportSM-DeliveryStatus*/
    offset=dissect_gsm_map_ReportSM_DeliveryStatusArg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 50: /*activateTraceMode*/
    offset=dissect_gsm_map_ActivateTraceModeRes(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 51: /*deactivateTraceMode*/
    offset=dissect_gsm_map_DeactivateTraceModeRes(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 55: /*sendIdentification*/
    offset=dissect_gsm_map_SendIdentificationRes(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 56:
	  offset = dissect_gsm_map_SendAuthenticationInfoRes(FALSE, tvb, offset, pinfo, tree, -1);
  case 57: /*restoreData*/
	offset=dissect_gsm_map_RestoreDataRes(FALSE, tvb, offset, pinfo, tree, -1);
	break;
  case 58: /*sendIMSI*/
	offset=dissect_gsm_map_Imsi(FALSE, tvb, offset, pinfo, tree,hf_gsm_map_imsi);
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
  case 66: /*readyForSM*/
    offset=dissect_gsm_map_ReadyForSM_Res(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 67: /*purgeMS*/
    offset=dissect_gsm_map_PurgeMS_Res(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 68: /*prepareHandover*/
    offset=dissect_gsm_map_PrepareHO_Res(FALSE, tvb, offset, pinfo, tree, -1);
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
  case 83: /*provideSubscriberLocation*/
    offset=dissect_gsm_map_ProvideSubscriberLocation_Res(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 85: /*sendRoutingInfoForLCS*/
    offset=dissect_gsm_map_RoutingInfoForLCS_Arg(FALSE, tvb, offset, pinfo, tree, -1);
    break;
  case 86: /*subscriberLocationReport*/
    offset=dissect_gsm_map_SubscriberLocationReport_Res(FALSE, tvb, offset, pinfo, tree, -1);
    break;
 default:
    proto_tree_add_text(tree, tvb, offset, -1, "Unknown returnResultData blob");
  }
  return offset;
}

static int 
dissect_invokeCmd(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_map_Opcode(FALSE, tvb, offset, pinfo, tree, hf_gsm_map_invokeCmd);
}

static int dissect_invokeid(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_ber_integer(FALSE, pinfo, tree, tvb, offset, hf_gsm_map_invokeid, NULL);
}


static const value_string InvokeId_vals[] = {
  {   0, "invokeid" },
  {   1, "absent" },
  { 0, NULL }
};

static int dissect_absent(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_map_NULL(FALSE, tvb, offset, pinfo, tree, hf_gsm_map_absent);
}


static const ber_choice_t InvokeId_choice[] = {
  {   0, BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_invokeid },
  {   1, BER_CLASS_UNI, BER_UNI_TAG_NULL, BER_FLAGS_NOOWNTAG, dissect_absent },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_gsm_map_InvokeId(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              InvokeId_choice, hf_index, ett_gsm_map_InvokeId);

  return offset;
}
static int dissect_invokeId(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_map_InvokeId(FALSE, tvb, offset, pinfo, tree, hf_gsm_map_invokeId);
}

static const ber_sequence_t InvokePDU_sequence[] = {
  { BER_CLASS_UNI, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_invokeId },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_invokeCmd },
  { BER_CLASS_UNI, -1/*depends on Cmd*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_invokeData },
  { 0, 0, 0, NULL }
};

static int
dissect_gsm_map_InvokePDU(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                InvokePDU_sequence, hf_index, ett_gsm_map_InvokePDU);

  return offset;
}
static int dissect_invoke_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_map_InvokePDU(TRUE, tvb, offset, pinfo, tree, hf_gsm_map_invoke);
}

static const ber_sequence_t ReturnResult_result_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_invokeCmd },
  { BER_CLASS_UNI, -1/*depends on Cmd*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_returnResultData },
  { 0, 0, 0, NULL }
};
static int
dissect_returnResult_result(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  offset = dissect_ber_sequence(FALSE, pinfo, tree, tvb, offset,
                                ReturnResult_result_sequence, hf_gsm_map_returnResult_result, ett_gsm_map_ReturnResult_result);

  return offset;
}

static const ber_sequence_t ReturnResultPDU_sequence[] = {
  { BER_CLASS_UNI, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_invokeId },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_returnResult_result },
  { 0, 0, 0, NULL }
};

static int
dissect_gsm_map_returnResultPDU(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                ReturnResultPDU_sequence, hf_index, ett_gsm_map_ReturnResultPDU);

  return offset;
}
static int dissect_returnResult_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_map_returnResultPDU(TRUE, tvb, offset, pinfo, tree, hf_gsm_map_returnResult);
}

static int
dissect_local_errorCode(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_ber_integer(FALSE, pinfo, tree, tvb, offset, hf_gsm_map_local_errorCode, NULL);
}

static int
dissect_global_errorCode(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  offset =  dissect_ber_object_identifier(FALSE, pinfo, tree, tvb, offset,
                                         hf_gsm_map_global_errorCode_oid, NULL);
  return dissect_ber_integer(FALSE, pinfo, tree, tvb, offset, hf_gsm_map_global_errorCode, NULL);
}
static const ber_choice_t ReturnError_result_choice[] = {
  {   0, BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_local_errorCode },
  {   1, BER_CLASS_UNI, BER_UNI_TAG_OID, BER_FLAGS_NOOWNTAG, dissect_global_errorCode },
  { 0, 0, 0, NULL }
};


static int
dissect_ReturnError_result(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {

  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              ReturnError_result_choice, hf_gsm_map_returnError_result, ett_gsm_map_ReturnError_result);

  return offset;
}

static const ber_sequence_t ReturnErrorPDU_sequence[] = {
  { BER_CLASS_UNI, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_invokeId },
  { BER_CLASS_UNI, -1/*choice*/,BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_ReturnError_result },
  { 0, 0, 0, NULL }
};

static int
dissect_gsm_map_ReturnErrorPDU(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                ReturnErrorPDU_sequence, hf_index, ett_gsm_map_ReturnErrorPDU);

  return offset;
}
static int dissect_returnError_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_map_ReturnErrorPDU(TRUE, tvb, offset, pinfo, tree, hf_gsm_map_returnError);
}


static const value_string GSMMAPPDU_vals[] = {
  {   1, "Invoke " },
  {   2, "ReturnResult " },
  {   3, "ReturnError " },
  {   4, "Reject " },
  { 0, NULL }
};

static const ber_choice_t GSMMAPPDU_choice[] = {
  {   1, BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_invoke_impl },
  {   2, BER_CLASS_CON, 2, BER_FLAGS_IMPLTAG, dissect_returnResult_impl },
  {   3, BER_CLASS_CON, 3, BER_FLAGS_IMPLTAG, dissect_returnError_impl },
#ifdef REMOVED
  {   4, BER_CLASS_CON, 4, BER_FLAGS_IMPLTAG, dissect_reject_impl },
#endif
  { 0, 0, 0, 0, NULL }
};

static guint8 gsmmap_pdu_type = 0;
static guint8 gsm_map_pdu_size = 0;

static int
dissect_gsm_map_GSMMAPPDU(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo , proto_tree *tree, int hf_index) {

	char *version_ptr, *version_str;

	opcode = 0;
	application_context_version = 0;
	if (pinfo->private_data != NULL){
		version_ptr = strrchr(pinfo->private_data,'.');
		version_str = g_strdup(version_ptr+1);
		application_context_version = atoi(version_str);
	}

  gsmmap_pdu_type = tvb_get_guint8(tvb, offset)&0x0f;
  /* Get the length and add 2 */
  gsm_map_pdu_size = tvb_get_guint8(tvb, offset+1)+2;

  if (check_col(pinfo->cinfo, COL_INFO)){
    col_set_str(pinfo->cinfo, COL_INFO, val_to_str(gsmmap_pdu_type, GSMMAPPDU_vals, "Unknown GSM-MAP PDU (%u)"));
  }

  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              GSMMAPPDU_choice, hf_index, ett_gsm_map_GSMMAPPDU);


  return offset;
}




static void
dissect_gsm_map(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree)
{
    proto_item		*item=NULL;
    proto_tree		*tree=NULL;
	/* Used for gsm_map TAP */
	static			gsm_map_tap_rec_t tap_rec;
	gint			op_idx;
    gchar			*str = NULL;


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
	str = my_match_strval(opcode, gsm_map_opr_code_strings, &op_idx);

	tap_rec.invoke = FALSE;
	if ( gsmmap_pdu_type  == 1 )
		tap_rec.invoke = TRUE;
	tap_rec.opr_code_idx = op_idx;
	tap_rec.size = gsm_map_pdu_size;
	/*
	tap_queue_packet(gsm_map_tap, pinfo, &tap_rec);
	*/


}

static const value_string ssCode_vals[] = {
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
/*--- proto_reg_handoff_gsm_map ---------------------------------------*/
void proto_reg_handoff_gsm_map(void) {
    dissector_handle_t	map_handle;
	static int map_prefs_initialized = FALSE;

    map_handle = create_dissector_handle(dissect_gsm_map, proto_gsm_map);

	if (!map_prefs_initialized) {
		map_prefs_initialized = TRUE;
	}
	else {
		dissector_delete("tcap.itu_ssn", tcap_itu_ssn1, map_handle);
		dissector_delete("tcap.itu_ssn", tcap_itu_ssn2, map_handle);
		dissector_delete("tcap.itu_ssn", tcap_itu_ssn3, map_handle);
		dissector_delete("tcap.itu_ssn", tcap_itu_ssn4, map_handle);
	}
		/* Set our sub system number for future use */
	tcap_itu_ssn1 = global_tcap_itu_ssn1;
	tcap_itu_ssn2 = global_tcap_itu_ssn2;
	tcap_itu_ssn3 = global_tcap_itu_ssn3;
	tcap_itu_ssn4 = global_tcap_itu_ssn4;

    dissector_add("tcap.itu_ssn", tcap_itu_ssn1, map_handle);
    dissector_add("tcap.itu_ssn", tcap_itu_ssn2, map_handle);
    dissector_add("tcap.itu_ssn", tcap_itu_ssn3, map_handle);
    dissector_add("tcap.itu_ssn", tcap_itu_ssn4, map_handle);
}

/*--- proto_register_gsm_map -------------------------------------------*/
void proto_register_gsm_map(void) {
	module_t *gsm_map_module;

  /* List of fields */
  static hf_register_info hf[] = {
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
	{ &hf_gsm_map_SendAuthenticationInfoArg,
      { "SendAuthenticationInfoArg", "gsm_map.SendAuthenticationInfoArg",
        FT_BYTES, BASE_NONE, NULL, 0,
        "SendAuthenticationInfoArg", HFILL }},
    { &hf_gsm_map_currentPassword,
      { "currentPassword", "gsm_map.currentPassword",
        FT_STRING, BASE_NONE, NULL, 0,
        "", HFILL }},
	{ &hf_gsm_mapSendEndSignal,
      { "mapSendEndSignalArg", "gsm_map.mapsendendsignalarg",
        FT_BYTES, BASE_NONE, NULL, 0,
        "mapSendEndSignalArg", HFILL }},
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
        FT_BYTES, BASE_NONE, NULL, 0,
        "globalerrorCodeoid", HFILL }},
	{&hf_gsm_map_global_errorCode,
      { "Global Error Code", "gsm_map.globalerrorCode",
        FT_UINT32, BASE_DEC, NULL, 0,
        "globalerrorCode", HFILL }},
    { &hf_gsm_map_getPassword,
      { "Password", "gsm_map.password",
        FT_UINT8, BASE_DEC, VALS(gsm_map_GetPasswordArg_vals), 0,
        "Password", HFILL }},
    { &hf_gsm_map_extension,
      { "Extension", "gsm_map.extension",
        FT_BOOLEAN, 8, TFS(&gsm_map_extension_value), 0x80,
        "Extension", HFILL }},
    { &hf_gsm_map_nature_of_number,
      { "Nature of number", "gsm_map.nature_of_number",
        FT_UINT8, BASE_HEX, VALS(gsm_map_nature_of_number_values), 0x70,
        "ature of number", HFILL }},
    { &hf_gsm_map_number_plan,
      { "Number plan", "gsm_map.number_plan",
        FT_UINT8, BASE_HEX, VALS(gsm_map_number_plan_values), 0x0f,
        "Number plan", HFILL }},
	{ &hf_gsm_map_misdn_digits,
      { "Misdn digits", "gsm_map.misdn_digits",
        FT_STRING, BASE_NONE, NULL, 0,
        "Misdn digits", HFILL }},
	{ &hf_gsm_map_servicecentreaddress_digits,
      { "ServiceCentreAddress digits", "gsm_map.servicecentreaddress_digits",
        FT_STRING, BASE_NONE, NULL, 0,
        "ServiceCentreAddress digits", HFILL }},
	{ &hf_gsm_map_map_gmsc_address_digits,
      { "Gmsc Address digits digits", "gsm_map.gmsc_address_digits",
        FT_STRING, BASE_NONE, NULL, 0,
        "Gmsc Address digits", HFILL }},
	{ &hf_gsm_map_imsi_digits,
      { "Imsi digits", "gsm_map.imsi_digits",
        FT_STRING, BASE_NONE, NULL, 0,
        "Imsi digits", HFILL }},
	{&hf_gsm_map_map_RoamingNumber_digits,
      { "RoamingNumber digits", "gsm_map.RoamingNumber_digits",
        FT_STRING, BASE_NONE, NULL, 0,
        "RoamingNumber digits", HFILL }},
	{&hf_gsm_map_map_hlr_number_digits,
      { "Hlr-Number digits", "gsm_map.hlr_number_digits",
        FT_STRING, BASE_NONE, NULL, 0,
        "Hlr-Number digits", HFILL }},
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
      { "R bit", "ss_status_r_bit",
        FT_BOOLEAN, 8, TFS(&gsm_map_Ss_Status_r_values), 0x02,
        "R bit", HFILL }},
	{ &hf_gsm_map_Ss_Status_a_bit,
      { "A bit", "ss_status_a_bit",
        FT_BOOLEAN, 8, TFS(&gsm_map_Ss_Status_a_values), 0x01,
        "A bit", HFILL }},

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
#include "packet-gsm_map-ettarr.c"
  };

  /* Register protocol */
  proto_gsm_map = proto_register_protocol(PNAME, PSNAME, PFNAME);
/*XXX  register_dissector("gsm_map", dissect_gsm_map, proto_gsm_map);*/
  /* Register fields and subtrees */
  proto_register_field_array(proto_gsm_map, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));

	sms_dissector_table = register_dissector_table("gsm_map.sms_tpdu", 
		"GSM SMS TPDU",FT_UINT8, BASE_DEC);

	gsm_map_tap = register_tap("gsm_map");
	register_ber_oid_name("0.4.0.0.1.0.1.3","itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) networkLocUp(1) version3(3)" );
	register_ber_oid_name("0.4.0.0.1.0.1.2","itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) networkLocUp(1) version2(2)" );
	register_ber_oid_name("0.4.0.0.1.0.2.2","itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) locationCancel(2) version2(2)" );
	register_ber_oid_name("0.4.0.0.1.0.2.1","itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) locationCancel(2) version1(1)" );
	register_ber_oid_name("0.4.0.0.1.0.3.2","itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) roamingNbEnquiry(3) version3(3)" );
	register_ber_oid_name("0.4.0.0.1.0.3.2","itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) roamingNbEnquiry(3) version2(2)" );
	register_ber_oid_name("0.4.0.0.1.0.3.1","itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) roamingNbEnquiry(3) version1(1)" );
	register_ber_oid_name("0.4.0.0.1.0.5.3","itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) locInfoRetrieval(5) version3(3)" );
	register_ber_oid_name("0.4.0.0.1.0.5.2","itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) locInfoRetrieval(5) version2(2)" );
	register_ber_oid_name("0.4.0.0.1.0.5.1","itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) locInfoRetrieval(5) version1(1)" );
	register_ber_oid_name("0.4.0.0.1.0.10.2","itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) reset(10) version2(2)" );
	register_ber_oid_name("0.4.0.0.1.0.10.1","itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) reset(10) version1(1)" );
	register_ber_oid_name("0.4.0.0.1.0.11.2","itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) handoverControl(11) version2(2)" );
	register_ber_oid_name("0.4.0.0.1.0.11.1","itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) handoverControl(11) version1(1)" );
	register_ber_oid_name("0.4.0.0.1.0.26.2","itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) imsiRetrieval(26) version2(2)" );
	register_ber_oid_name("0.4.0.0.1.0.13.2","itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) equipmentMngt(13) version2(2)" );
	register_ber_oid_name("0.4.0.0.1.0.13.1","itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) equipmentMngt(13) version1(1)" );
	register_ber_oid_name("0.4.0.0.1.0.14.3","itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) infoRetrieval(14) version3(3)" );
	register_ber_oid_name("0.4.0.0.1.0.14.2","itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) infoRetrieval(14) version2(2)" );
	register_ber_oid_name("0.4.0.0.1.0.14.1","itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) infoRetrieval(14) version1(1)" );
	register_ber_oid_name("0.4.0.0.1.0.15.1","itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) interVlrInfoRetrieval(15) version2(2)" );
	register_ber_oid_name("0.4.0.0.1.0.16.3","itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) subscriberDataMngt(16) version3(3)" );
	register_ber_oid_name("0.4.0.0.1.0.16.2","itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) subscriberDataMngt(16) version2(2)" );
	register_ber_oid_name("0.4.0.0.1.0.16.1","itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) subscriberDataMngt(16) version1(1)" );
	register_ber_oid_name("0.4.0.0.1.0.17.2","itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) tracing(17) version2(2)" );
	register_ber_oid_name("0.4.0.0.1.0.17.1","itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) tracing(17) version1(1)" );
	register_ber_oid_name("0.4.0.0.1.0.18.2","itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) networkFunctionalSs(18) version2(2)" );
	register_ber_oid_name("0.4.0.0.1.0.18.1","itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) networkFunctionalSs(18) version1(1)" );
	register_ber_oid_name("0.4.0.0.1.0.19.1","itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) networkUnstructuredSs(19) version2(2)" );
	register_ber_oid_name("0.4.0.0.1.0.20.2","itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) shortMsgGateway(20) version2(2)" );
	register_ber_oid_name("0.4.0.0.1.0.20.1","itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) shortMsgGateway(20) version1(1)" );
	register_ber_oid_name("0.4.0.0.1.0.21.2","itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) shortMsgMO-Relay(21) version2(2)" );
	register_ber_oid_name("0.4.0.0.1.0.21.1","itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) --shortMsgRelay--21 version1(1)" );
	register_ber_oid_name("0.4.0.0.1.0.23.2","itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) shortMsgAlert(23) version2(2)" );
	register_ber_oid_name("0.4.0.0.1.0.23.1","itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) shortMsgAlert(23) version1(1)" );
	register_ber_oid_name("0.4.0.0.1.0.24.2","itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) mwdMngt(24) version2(2)" );
	register_ber_oid_name("0.4.0.0.1.0.24.1","itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) mwdMngt(24) version1(1)" );
	register_ber_oid_name("0.4.0.0.1.0.25.2","itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) shortMsgMT-Relay(25) version2(2)" );
	register_ber_oid_name("0.4.0.0.1.0.25.1","itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) map-ac(0) msPurging(27) version2(2)" );

	/* Register our configuration options, particularly our ssn:s */

	gsm_map_module = prefs_register_protocol(proto_gsm_map, proto_reg_handoff_gsm_map);
	prefs_register_uint_preference(gsm_map_module, "tcap.itu_ssn1",
		"Subsystem number used for GSM MAP 1",
		"Set Subsystem number used for GSM MAP",
		10, &global_tcap_itu_ssn1);
	prefs_register_uint_preference(gsm_map_module, "tcap.itu_ssn2",
		"Subsystem number used for GSM MAP 2",
		"Set Subsystem number used for GSM MAP",
		10, &global_tcap_itu_ssn2);
	prefs_register_uint_preference(gsm_map_module, "tcap.itu_ssn3",
		"Subsystem number used for GSM MAP 3",
		"Set Subsystem number used for GSM MAP",
		10, &global_tcap_itu_ssn3);
	prefs_register_uint_preference(gsm_map_module, "tcap.itu_ssn4",
		"Subsystem number used for GSM MAP 4",
		"Set Subsystem number used for GSM MAP",
		10, &global_tcap_itu_ssn4);


}


