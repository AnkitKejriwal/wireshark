/* Do not modify this file.                                                   */
/* It is created automatically by the ASN.1 to Ethereal dissector compiler    */
/* ./packet-gsm_ss.c                                                          */
/* ../../tools/asn2eth.py -X -b -e -p gsm_ss -c gsm_ss.cnf -s packet-gsm_ss-template SS-Operations.asn */

/* Input file: packet-gsm_ss-template.c */

/* packet-gsm_ss-template.c
 * Routines for GSM Supplementary Services dissection
 * Copyright 2005, Anders Broman <anders.broman@ericsson.com>
 * Based on the dissector by:
 * Michael Lum <mlum [AT] telostech.com>
 * In association with Telos Technology Inc.
 *
 * Title		3GPP			Other
 *
 *   Reference [1]
 *   Mobile radio Layer 3 supplementary service specification;
 *   Formats and coding
 *   (3GPP TS 24.080 version )
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
#include "packet-gsm_ss.h"
#include "packet-gsm_map.h"

#define PNAME  "GSM_SS"
#define PSNAME "GSM_SS"
#define PFNAME "gsm_ss"

const value_string gsm_ss_err_code_strings[] = {
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

const value_string gsm_ss_opr_code_strings[] = {
    { 10,	"Register SS" },
    { 11,	"Erase SS" },
    { 12,	"Activate SS" },
    { 13,	"Deactivate SS" },
    { 14,	"Interrogate SS" },
    { 16,	"Notify SS" },
    { 17,	"Register Password" },
    { 18,	"Get Password" },
    { 19,	"Process Unstructured SS Data" },
    { 38,	"Forward Check SS Indication" },
    { 59,	"Process Unstructured SS Request" },
    { 60,	"Unstructured SS Request" },
    { 61,	"Unstructured SS Notify" },
    { 77,	"Erase CC Entry" },
    { 112,	"lcs-AreaEventCancellation" },
    { 113,	"lcs-AreaEventReport" },
    { 114,	"LCS-AreaEventRequest" },
    { 115,	"LCS MOLR" },
    { 116,	"LCS Location Notification" },
    { 117,	"Call Deflection" },
    { 118,	"User User Service" },
    { 119,	"Access Register CC Entry" },
    { 120,	"Forward CUG Info" },
    { 121,	"Split MPTY" },
    { 122,	"Retrieve MPTY" },
    { 123,	"Hold MPTY" },
    { 124,	"Build MPTY" },
    { 125,	"Forward Charge Advice" },
    { 126,	"Explicit CT" },

    { 0, NULL }
};

/* Initialize the protocol and registered fields */
int proto_gsm_ss = -1;

static int hf_gsm_ss_getPassword = -1;
static int hf_gsm_ss_currentPassword = -1;
static int hf_gsm_ss_SS_Code = -1;

/*--- Included file: packet-gsm_ss-hf.c ---*/

static int hf_gsm_ss_ss_Code = -1;                /* SS_Code */
static int hf_gsm_ss_ss_Status = -1;              /* SS_Status */
static int hf_gsm_ss_ss_Notification = -1;        /* SS_Notification */
static int hf_gsm_ss_callIsWaiting_Indicator = -1;  /* NULL */
static int hf_gsm_ss_callOnHold_Indicator = -1;   /* CallOnHold_Indicator */
static int hf_gsm_ss_mpty_Indicator = -1;         /* NULL */
static int hf_gsm_ss_cug_Index = -1;              /* CUG_Index */
static int hf_gsm_ss_clirSuppressionRejected = -1;  /* NULL */
static int hf_gsm_ss_ect_Indicator = -1;          /* ECT_Indicator */
static int hf_gsm_ss_nameIndicator = -1;          /* NameIndicator */
static int hf_gsm_ss_ccbs_Feature = -1;           /* CCBS_Feature */
static int hf_gsm_ss_alertingPattern = -1;        /* AlertingPattern */
static int hf_gsm_ss_multicall_Indicator = -1;    /* Multicall_Indicator */
static int hf_gsm_ss_chargingInformation = -1;    /* ChargingInformation */
static int hf_gsm_ss_e1 = -1;                     /* E1 */
static int hf_gsm_ss_e2 = -1;                     /* E2 */
static int hf_gsm_ss_e3 = -1;                     /* E3 */
static int hf_gsm_ss_e4 = -1;                     /* E4 */
static int hf_gsm_ss_e5 = -1;                     /* E5 */
static int hf_gsm_ss_e6 = -1;                     /* E6 */
static int hf_gsm_ss_e7 = -1;                     /* E7 */
static int hf_gsm_ss_suppressPrefCUG = -1;        /* NULL */
static int hf_gsm_ss_suppressOA = -1;             /* NULL */
static int hf_gsm_ss_ect_CallState = -1;          /* ECT_CallState */
static int hf_gsm_ss_rdn = -1;                    /* RDN */
static int hf_gsm_ss_callingName = -1;            /* Name */
static int hf_gsm_ss_namePresentationAllowed = -1;  /* NameSet */
static int hf_gsm_ss_presentationRestricted = -1;  /* NULL */
static int hf_gsm_ss_nameUnavailable = -1;        /* NULL */
static int hf_gsm_ss_namePresentationRestricted = -1;  /* NameSet */
static int hf_gsm_ss_dataCodingScheme = -1;       /* USSD_DataCodingScheme */
static int hf_gsm_ss_lengthInCharacters = -1;     /* INTEGER */
static int hf_gsm_ss_nameString = -1;             /* USSD_String */
static int hf_gsm_ss_presentationAllowedAddress = -1;  /* RemotePartyNumber */
static int hf_gsm_ss_numberNotAvailableDueToInterworking = -1;  /* NULL */
static int hf_gsm_ss_presentationRestrictedAddress = -1;  /* RemotePartyNumber */
static int hf_gsm_ss_partyNumber = -1;            /* ISDN_AddressString */
static int hf_gsm_ss_partyNumberSubaddress = -1;  /* ISDN_SubaddressString */
static int hf_gsm_ss_deflectedToNumber = -1;      /* AddressString */
static int hf_gsm_ss_deflectedToSubaddress = -1;  /* ISDN_SubaddressString */
static int hf_gsm_ss_uUS_Service = -1;            /* UUS_Service */
static int hf_gsm_ss_uUS_Required = -1;           /* BOOLEAN */
static int hf_gsm_ss_notificationType = -1;       /* NotificationToMSUser */
static int hf_gsm_ss_locationType = -1;           /* LocationType */
static int hf_gsm_ss_lcsClientExternalID = -1;    /* LCSClientExternalID */
static int hf_gsm_ss_lcsClientName = -1;          /* LCSClientName */
static int hf_gsm_ss_lcsRequestorID = -1;         /* LCSRequestorID */
static int hf_gsm_ss_lcsCodeword = -1;            /* LCSCodeword */
static int hf_gsm_ss_lcsServiceTypeID = -1;       /* LCSServiceTypeID */
static int hf_gsm_ss_verificationResponse = -1;   /* VerificationResponse */
static int hf_gsm_ss_molr_Type = -1;              /* MOLR_Type */
static int hf_gsm_ss_locationMethod = -1;         /* LocationMethod */
static int hf_gsm_ss_lcs_QoS = -1;                /* LCS_QoS */
static int hf_gsm_ss_mlc_Number = -1;             /* ISDN_AddressString */
static int hf_gsm_ss_gpsAssistanceData = -1;      /* GPSAssistanceData */
static int hf_gsm_ss_supportedGADShapes = -1;     /* SupportedGADShapes */
static int hf_gsm_ss_ageOfLocationInfo = -1;      /* AgeOfLocationInformation */
static int hf_gsm_ss_pseudonymIndicator = -1;     /* NULL */
static int hf_gsm_ss_locationEstimate = -1;       /* Ext_GeographicalInformation */
static int hf_gsm_ss_decipheringKeys = -1;        /* DecipheringKeys */
static int hf_gsm_ss_add_LocationEstimate = -1;   /* Add_GeographicalInformation */
static int hf_gsm_ss_referenceNumber = -1;        /* LCS_ReferenceNumber */
static int hf_gsm_ss_h_gmlc_address = -1;         /* GSN_Address */
static int hf_gsm_ss_deferredLocationEventType = -1;  /* DeferredLocationEventType */
static int hf_gsm_ss_areaEventInfo = -1;          /* AreaEventInfo */

/*--- End of included file: packet-gsm_ss-hf.c ---*/


/* Initialize the subtree pointers */

/*--- Included file: packet-gsm_ss-ett.c ---*/

static gint ett_gsm_ss_NotifySS_Arg = -1;
static gint ett_gsm_ss_ForwardChargeAdviceArg = -1;
static gint ett_gsm_ss_ChargingInformation = -1;
static gint ett_gsm_ss_ForwardCUG_InfoArg = -1;
static gint ett_gsm_ss_ECT_Indicator = -1;
static gint ett_gsm_ss_NameIndicator = -1;
static gint ett_gsm_ss_Name = -1;
static gint ett_gsm_ss_NameSet = -1;
static gint ett_gsm_ss_RDN = -1;
static gint ett_gsm_ss_RemotePartyNumber = -1;
static gint ett_gsm_ss_AccessRegisterCCEntryArg = -1;
static gint ett_gsm_ss_CallDeflectionArg = -1;
static gint ett_gsm_ss_UserUserServiceArg = -1;
static gint ett_gsm_ss_LocationNotificationArg = -1;
static gint ett_gsm_ss_LocationNotificationRes = -1;
static gint ett_gsm_ss_LCS_MOLRArg = -1;
static gint ett_gsm_ss_LCS_MOLRRes = -1;
static gint ett_gsm_ss_LCS_AreaEventRequestArg = -1;
static gint ett_gsm_ss_LCS_AreaEventReportArg = -1;
static gint ett_gsm_ss_LCS_AreaEventCancellationArg = -1;

/*--- End of included file: packet-gsm_ss-ett.c ---*/


static dissector_table_t	sms_dissector_table;	/* SMS TPDU */

/* Global variables */
static proto_tree *top_tree;




/*--- Included file: packet-gsm_ss-fn.c ---*/

/*--- Fields for imported types ---*/

static int dissect_ss_Code(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_map_SS_Code(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_ss_Code);
}
static int dissect_ss_Status(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_map_SS_Status(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_ss_Status);
}
static int dissect_cug_Index(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_map_CUG_Index(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_cug_Index);
}
static int dissect_ccbs_Feature(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_map_CCBS_Feature(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_ccbs_Feature);
}
static int dissect_alertingPattern(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_map_AlertingPattern(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_alertingPattern);
}
static int dissect_dataCodingScheme(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_map_USSD_DataCodingScheme(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_dataCodingScheme);
}
static int dissect_nameString(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_map_USSD_String(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_nameString);
}
static int dissect_partyNumber(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_map_ISDN_AddressString(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_partyNumber);
}
static int dissect_partyNumberSubaddress(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_map_ISDN_SubaddressString(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_partyNumberSubaddress);
}
static int dissect_deflectedToNumber(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_map_AddressString(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_deflectedToNumber);
}
static int dissect_deflectedToSubaddress(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_map_ISDN_SubaddressString(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_deflectedToSubaddress);
}
static int dissect_notificationType(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_map_NotificationToMSUser(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_notificationType);
}
static int dissect_locationType(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_map_LocationType(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_locationType);
}
static int dissect_lcsClientExternalID(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_map_LCSClientExternalID(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_lcsClientExternalID);
}
static int dissect_lcsClientName(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_map_LCSClientName(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_lcsClientName);
}
static int dissect_lcsRequestorID(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_map_LCSRequestorID(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_lcsRequestorID);
}
static int dissect_lcsCodeword(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_map_LCSCodeword(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_lcsCodeword);
}
static int dissect_lcsServiceTypeID(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_map_LCSServiceTypeID(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_lcsServiceTypeID);
}
static int dissect_lcs_QoS(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_map_LCS_QoS(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_lcs_QoS);
}
static int dissect_mlc_Number(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_map_ISDN_AddressString(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_mlc_Number);
}
static int dissect_supportedGADShapes(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_map_SupportedGADShapes(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_supportedGADShapes);
}
static int dissect_ageOfLocationInfo(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_map_AgeOfLocationInformation(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_ageOfLocationInfo);
}
static int dissect_locationEstimate(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_map_Ext_GeographicalInformation(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_locationEstimate);
}
static int dissect_add_LocationEstimate(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_map_Add_GeographicalInformation(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_add_LocationEstimate);
}
static int dissect_referenceNumber(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_map_LCS_ReferenceNumber(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_referenceNumber);
}
static int dissect_h_gmlc_address(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_map_GSN_Address(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_h_gmlc_address);
}
static int dissect_deferredLocationEventType(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_map_DeferredLocationEventType(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_deferredLocationEventType);
}
static int dissect_areaEventInfo(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_map_AreaEventInfo(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_areaEventInfo);
}


static int
dissect_gsm_ss_SS_UserData(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_restricted_string(implicit_tag, BER_UNI_TAG_IA5String,
                                         pinfo, tree, tvb, offset, hf_index,
                                         NULL);

  return offset;
}



static int
dissect_gsm_ss_SS_Notification(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                       NULL);

  return offset;
}
static int dissect_ss_Notification(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_ss_SS_Notification(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_ss_Notification);
}



static int
dissect_gsm_ss_NULL(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_null(implicit_tag, pinfo, tree, tvb, offset, hf_index);

  return offset;
}
static int dissect_callIsWaiting_Indicator(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_ss_NULL(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_callIsWaiting_Indicator);
}
static int dissect_mpty_Indicator(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_ss_NULL(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_mpty_Indicator);
}
static int dissect_clirSuppressionRejected(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_ss_NULL(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_clirSuppressionRejected);
}
static int dissect_suppressPrefCUG(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_ss_NULL(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_suppressPrefCUG);
}
static int dissect_suppressOA(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_ss_NULL(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_suppressOA);
}
static int dissect_presentationRestricted(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_ss_NULL(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_presentationRestricted);
}
static int dissect_nameUnavailable(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_ss_NULL(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_nameUnavailable);
}
static int dissect_numberNotAvailableDueToInterworking(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_ss_NULL(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_numberNotAvailableDueToInterworking);
}
static int dissect_pseudonymIndicator(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_ss_NULL(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_pseudonymIndicator);
}


static const value_string gsm_ss_CallOnHold_Indicator_vals[] = {
  {   0, "callRetrieved" },
  {   1, "callOnHold" },
  { 0, NULL }
};


static int
dissect_gsm_ss_CallOnHold_Indicator(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_callOnHold_Indicator(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_ss_CallOnHold_Indicator(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_callOnHold_Indicator);
}


static const value_string gsm_ss_ECT_CallState_vals[] = {
  {   0, "alerting" },
  {   1, "active" },
  { 0, NULL }
};


static int
dissect_gsm_ss_ECT_CallState(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_ect_CallState(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_ss_ECT_CallState(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_ect_CallState);
}


static const ber_sequence_t RemotePartyNumber_sequence[] = {
  { BER_CLASS_CON, 0, 0, dissect_partyNumber },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL, dissect_partyNumberSubaddress },
  { 0, 0, 0, NULL }
};

static int
dissect_gsm_ss_RemotePartyNumber(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   RemotePartyNumber_sequence, hf_index, ett_gsm_ss_RemotePartyNumber);

  return offset;
}
static int dissect_presentationAllowedAddress(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_ss_RemotePartyNumber(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_presentationAllowedAddress);
}
static int dissect_presentationRestrictedAddress(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_ss_RemotePartyNumber(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_presentationRestrictedAddress);
}


static const value_string gsm_ss_RDN_vals[] = {
  {   0, "presentationAllowedAddress" },
  {   1, "presentationRestricted" },
  {   2, "numberNotAvailableDueToInterworking" },
  {   3, "presentationRestrictedAddress" },
  { 0, NULL }
};

static const ber_choice_t RDN_choice[] = {
  {   0, BER_CLASS_CON, 0, 0, dissect_presentationAllowedAddress },
  {   1, BER_CLASS_CON, 1, 0, dissect_presentationRestricted },
  {   2, BER_CLASS_CON, 2, 0, dissect_numberNotAvailableDueToInterworking },
  {   3, BER_CLASS_CON, 3, 0, dissect_presentationRestrictedAddress },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_gsm_ss_RDN(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                                 RDN_choice, hf_index, ett_gsm_ss_RDN,
                                 NULL);

  return offset;
}
static int dissect_rdn(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_ss_RDN(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_rdn);
}


static const ber_sequence_t ECT_Indicator_sequence[] = {
  { BER_CLASS_CON, 0, 0, dissect_ect_CallState },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_NOTCHKTAG, dissect_rdn },
  { 0, 0, 0, NULL }
};

static int
dissect_gsm_ss_ECT_Indicator(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   ECT_Indicator_sequence, hf_index, ett_gsm_ss_ECT_Indicator);

  return offset;
}
static int dissect_ect_Indicator(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_ss_ECT_Indicator(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_ect_Indicator);
}



static int
dissect_gsm_ss_INTEGER(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_lengthInCharacters(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_ss_INTEGER(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_lengthInCharacters);
}


static const ber_sequence_t NameSet_sequence[] = {
  { BER_CLASS_CON, 0, 0, dissect_dataCodingScheme },
  { BER_CLASS_CON, 1, 0, dissect_lengthInCharacters },
  { BER_CLASS_CON, 2, 0, dissect_nameString },
  { 0, 0, 0, NULL }
};

static int
dissect_gsm_ss_NameSet(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   NameSet_sequence, hf_index, ett_gsm_ss_NameSet);

  return offset;
}
static int dissect_namePresentationAllowed(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_ss_NameSet(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_namePresentationAllowed);
}
static int dissect_namePresentationRestricted(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_ss_NameSet(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_namePresentationRestricted);
}


static const value_string gsm_ss_Name_vals[] = {
  {   0, "namePresentationAllowed" },
  {   1, "presentationRestricted" },
  {   2, "nameUnavailable" },
  {   3, "namePresentationRestricted" },
  { 0, NULL }
};

static const ber_choice_t Name_choice[] = {
  {   0, BER_CLASS_CON, 0, 0, dissect_namePresentationAllowed },
  {   1, BER_CLASS_CON, 1, 0, dissect_presentationRestricted },
  {   2, BER_CLASS_CON, 2, 0, dissect_nameUnavailable },
  {   3, BER_CLASS_CON, 3, 0, dissect_namePresentationRestricted },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_gsm_ss_Name(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                                 Name_choice, hf_index, ett_gsm_ss_Name,
                                 NULL);

  return offset;
}
static int dissect_callingName(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_ss_Name(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_callingName);
}


static const ber_sequence_t NameIndicator_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_NOTCHKTAG, dissect_callingName },
  { 0, 0, 0, NULL }
};

static int
dissect_gsm_ss_NameIndicator(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   NameIndicator_sequence, hf_index, ett_gsm_ss_NameIndicator);

  return offset;
}
static int dissect_nameIndicator(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_ss_NameIndicator(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_nameIndicator);
}


static const value_string gsm_ss_Multicall_Indicator_vals[] = {
  {   0, "nbr-SNexceeded" },
  {   1, "nbr-Userexceeded" },
  { 0, NULL }
};


static int
dissect_gsm_ss_Multicall_Indicator(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_multicall_Indicator(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_ss_Multicall_Indicator(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_multicall_Indicator);
}


static const ber_sequence_t NotifySS_Arg_sequence[] = {
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL, dissect_ss_Code },
  { BER_CLASS_CON, 4, BER_FLAGS_OPTIONAL, dissect_ss_Status },
  { BER_CLASS_CON, 5, BER_FLAGS_OPTIONAL, dissect_ss_Notification },
  { BER_CLASS_CON, 14, BER_FLAGS_OPTIONAL, dissect_callIsWaiting_Indicator },
  { BER_CLASS_CON, 15, BER_FLAGS_OPTIONAL, dissect_callOnHold_Indicator },
  { BER_CLASS_CON, 16, BER_FLAGS_OPTIONAL, dissect_mpty_Indicator },
  { BER_CLASS_CON, 17, BER_FLAGS_OPTIONAL, dissect_cug_Index },
  { BER_CLASS_CON, 18, BER_FLAGS_OPTIONAL, dissect_clirSuppressionRejected },
  { BER_CLASS_CON, 19, BER_FLAGS_OPTIONAL, dissect_ect_Indicator },
  { BER_CLASS_CON, 20, BER_FLAGS_OPTIONAL, dissect_nameIndicator },
  { BER_CLASS_CON, 21, BER_FLAGS_OPTIONAL, dissect_ccbs_Feature },
  { BER_CLASS_CON, 22, BER_FLAGS_OPTIONAL, dissect_alertingPattern },
  { BER_CLASS_CON, 23, BER_FLAGS_OPTIONAL, dissect_multicall_Indicator },
  { 0, 0, 0, NULL }
};

static int
dissect_gsm_ss_NotifySS_Arg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   NotifySS_Arg_sequence, hf_index, ett_gsm_ss_NotifySS_Arg);

  return offset;
}



static int
dissect_gsm_ss_E1(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_e1(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_ss_E1(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_e1);
}



static int
dissect_gsm_ss_E2(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_e2(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_ss_E2(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_e2);
}



static int
dissect_gsm_ss_E3(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_e3(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_ss_E3(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_e3);
}



static int
dissect_gsm_ss_E4(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_e4(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_ss_E4(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_e4);
}



static int
dissect_gsm_ss_E5(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_e5(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_ss_E5(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_e5);
}



static int
dissect_gsm_ss_E6(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_e6(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_ss_E6(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_e6);
}



static int
dissect_gsm_ss_E7(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_e7(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_ss_E7(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_e7);
}


static const ber_sequence_t ChargingInformation_sequence[] = {
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL, dissect_e1 },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL, dissect_e2 },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL, dissect_e3 },
  { BER_CLASS_CON, 4, BER_FLAGS_OPTIONAL, dissect_e4 },
  { BER_CLASS_CON, 5, BER_FLAGS_OPTIONAL, dissect_e5 },
  { BER_CLASS_CON, 6, BER_FLAGS_OPTIONAL, dissect_e6 },
  { BER_CLASS_CON, 7, BER_FLAGS_OPTIONAL, dissect_e7 },
  { 0, 0, 0, NULL }
};

static int
dissect_gsm_ss_ChargingInformation(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   ChargingInformation_sequence, hf_index, ett_gsm_ss_ChargingInformation);

  return offset;
}
static int dissect_chargingInformation(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_ss_ChargingInformation(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_chargingInformation);
}


static const ber_sequence_t ForwardChargeAdviceArg_sequence[] = {
  { BER_CLASS_CON, 0, 0, dissect_ss_Code },
  { BER_CLASS_CON, 1, 0, dissect_chargingInformation },
  { 0, 0, 0, NULL }
};

static int
dissect_gsm_ss_ForwardChargeAdviceArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   ForwardChargeAdviceArg_sequence, hf_index, ett_gsm_ss_ForwardChargeAdviceArg);

  return offset;
}


static const ber_sequence_t ForwardCUG_InfoArg_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL, dissect_cug_Index },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL, dissect_suppressPrefCUG },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL, dissect_suppressOA },
  { 0, 0, 0, NULL }
};

static int
dissect_gsm_ss_ForwardCUG_InfoArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   ForwardCUG_InfoArg_sequence, hf_index, ett_gsm_ss_ForwardCUG_InfoArg);

  return offset;
}


static const ber_sequence_t AccessRegisterCCEntryArg_sequence[] = {
  { 0, 0, 0, NULL }
};

static int
dissect_gsm_ss_AccessRegisterCCEntryArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   AccessRegisterCCEntryArg_sequence, hf_index, ett_gsm_ss_AccessRegisterCCEntryArg);

  return offset;
}


static const ber_sequence_t CallDeflectionArg_sequence[] = {
  { BER_CLASS_CON, 0, 0, dissect_deflectedToNumber },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL, dissect_deflectedToSubaddress },
  { 0, 0, 0, NULL }
};

static int
dissect_gsm_ss_CallDeflectionArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   CallDeflectionArg_sequence, hf_index, ett_gsm_ss_CallDeflectionArg);

  return offset;
}


static const value_string gsm_ss_UUS_Service_vals[] = {
  {   1, "uUS1" },
  {   2, "uUS2" },
  {   3, "uUS3" },
  { 0, NULL }
};


static int
dissect_gsm_ss_UUS_Service(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_uUS_Service(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_ss_UUS_Service(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_uUS_Service);
}



static int
dissect_gsm_ss_BOOLEAN(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_boolean(implicit_tag, pinfo, tree, tvb, offset, hf_index);

  return offset;
}
static int dissect_uUS_Required(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_ss_BOOLEAN(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_uUS_Required);
}


static const ber_sequence_t UserUserServiceArg_sequence[] = {
  { BER_CLASS_CON, 0, 0, dissect_uUS_Service },
  { BER_CLASS_CON, 1, 0, dissect_uUS_Required },
  { 0, 0, 0, NULL }
};

static int
dissect_gsm_ss_UserUserServiceArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   UserUserServiceArg_sequence, hf_index, ett_gsm_ss_UserUserServiceArg);

  return offset;
}


static const ber_sequence_t LocationNotificationArg_sequence[] = {
  { BER_CLASS_CON, 0, 0, dissect_notificationType },
  { BER_CLASS_CON, 1, 0, dissect_locationType },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL, dissect_lcsClientExternalID },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL, dissect_lcsClientName },
  { BER_CLASS_CON, 4, BER_FLAGS_OPTIONAL, dissect_lcsRequestorID },
  { BER_CLASS_CON, 5, BER_FLAGS_OPTIONAL, dissect_lcsCodeword },
  { BER_CLASS_CON, 6, BER_FLAGS_OPTIONAL, dissect_lcsServiceTypeID },
  { 0, 0, 0, NULL }
};

static int
dissect_gsm_ss_LocationNotificationArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   LocationNotificationArg_sequence, hf_index, ett_gsm_ss_LocationNotificationArg);

  return offset;
}


static const value_string gsm_ss_VerificationResponse_vals[] = {
  {   0, "permissionDenied" },
  {   1, "permissionGranted" },
  { 0, NULL }
};


static int
dissect_gsm_ss_VerificationResponse(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_verificationResponse(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_ss_VerificationResponse(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_verificationResponse);
}


static const ber_sequence_t LocationNotificationRes_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL, dissect_verificationResponse },
  { 0, 0, 0, NULL }
};

static int
dissect_gsm_ss_LocationNotificationRes(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   LocationNotificationRes_sequence, hf_index, ett_gsm_ss_LocationNotificationRes);

  return offset;
}


static const value_string gsm_ss_MOLR_Type_vals[] = {
  {   0, "locationEstimate" },
  {   1, "assistanceData" },
  {   2, "deCipheringKeys" },
  { 0, NULL }
};


static int
dissect_gsm_ss_MOLR_Type(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_molr_Type(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_ss_MOLR_Type(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_molr_Type);
}


static const value_string gsm_ss_LocationMethod_vals[] = {
  {   0, "msBasedEOTD" },
  {   1, "msAssistedEOTD" },
  {   2, "assistedGPS" },
  {   3, "msBasedOTDOA" },
  { 0, NULL }
};


static int
dissect_gsm_ss_LocationMethod(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_locationMethod(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_ss_LocationMethod(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_locationMethod);
}



static int
dissect_gsm_ss_GPSAssistanceData(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                       NULL);

  return offset;
}
static int dissect_gpsAssistanceData(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_ss_GPSAssistanceData(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_gpsAssistanceData);
}


static const ber_sequence_t LCS_MOLRArg_sequence[] = {
  { BER_CLASS_CON, 0, 0, dissect_molr_Type },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL, dissect_locationMethod },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL, dissect_lcs_QoS },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL, dissect_lcsClientExternalID },
  { BER_CLASS_CON, 4, BER_FLAGS_OPTIONAL, dissect_mlc_Number },
  { BER_CLASS_CON, 5, BER_FLAGS_OPTIONAL, dissect_gpsAssistanceData },
  { BER_CLASS_CON, 6, BER_FLAGS_OPTIONAL, dissect_supportedGADShapes },
  { BER_CLASS_CON, 7, BER_FLAGS_OPTIONAL, dissect_lcsServiceTypeID },
  { BER_CLASS_CON, 8, BER_FLAGS_OPTIONAL, dissect_ageOfLocationInfo },
  { BER_CLASS_CON, 9, BER_FLAGS_OPTIONAL, dissect_locationType },
  { BER_CLASS_CON, 10, BER_FLAGS_OPTIONAL, dissect_pseudonymIndicator },
  { 0, 0, 0, NULL }
};

static int
dissect_gsm_ss_LCS_MOLRArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   LCS_MOLRArg_sequence, hf_index, ett_gsm_ss_LCS_MOLRArg);

  return offset;
}



static int
dissect_gsm_ss_DecipheringKeys(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                       NULL);

  return offset;
}
static int dissect_decipheringKeys(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_gsm_ss_DecipheringKeys(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_decipheringKeys);
}


static const ber_sequence_t LCS_MOLRRes_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL, dissect_locationEstimate },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL, dissect_decipheringKeys },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL, dissect_add_LocationEstimate },
  { 0, 0, 0, NULL }
};

static int
dissect_gsm_ss_LCS_MOLRRes(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   LCS_MOLRRes_sequence, hf_index, ett_gsm_ss_LCS_MOLRRes);

  return offset;
}


static const ber_sequence_t LCS_AreaEventRequestArg_sequence[] = {
  { BER_CLASS_CON, 0, 0, dissect_referenceNumber },
  { BER_CLASS_CON, 1, 0, dissect_h_gmlc_address },
  { BER_CLASS_CON, 3, 0, dissect_deferredLocationEventType },
  { BER_CLASS_CON, 4, 0, dissect_areaEventInfo },
  { 0, 0, 0, NULL }
};

static int
dissect_gsm_ss_LCS_AreaEventRequestArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   LCS_AreaEventRequestArg_sequence, hf_index, ett_gsm_ss_LCS_AreaEventRequestArg);

  return offset;
}


static const ber_sequence_t LCS_AreaEventReportArg_sequence[] = {
  { BER_CLASS_CON, 0, 0, dissect_referenceNumber },
  { BER_CLASS_CON, 1, 0, dissect_h_gmlc_address },
  { 0, 0, 0, NULL }
};

static int
dissect_gsm_ss_LCS_AreaEventReportArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   LCS_AreaEventReportArg_sequence, hf_index, ett_gsm_ss_LCS_AreaEventReportArg);

  return offset;
}


static const ber_sequence_t LCS_AreaEventCancellationArg_sequence[] = {
  { BER_CLASS_CON, 0, 0, dissect_referenceNumber },
  { BER_CLASS_CON, 1, 0, dissect_h_gmlc_address },
  { 0, 0, 0, NULL }
};

static int
dissect_gsm_ss_LCS_AreaEventCancellationArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   LCS_AreaEventCancellationArg_sequence, hf_index, ett_gsm_ss_LCS_AreaEventCancellationArg);

  return offset;
}


/*--- End of included file: packet-gsm_ss-fn.c ---*/



int
gsm_ss_dissect(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset,guint32 opcode, gint comp_type_tag)
{
	switch (comp_type_tag){
		case 1: /* invoke */
			switch (opcode){
				case 10: /* Register SS -- imports operations from MAP-SupplementaryServiceOperations*/
					offset=dissect_gsm_map_RegisterSS_Arg(FALSE, tvb, offset, pinfo, tree, -1);
					break;
				case 11: /* Erase SS -- imports operations from MAP-SupplementaryServiceOperations*/
					offset=dissect_gsm_map_SS_ForBS_Code(FALSE, tvb, offset, pinfo, tree, -1);
					break;
				case 12: /* Activate SS -- imports operations from MAP-SupplementaryServiceOperations*/
					offset=dissect_gsm_map_SS_ForBS_Code(FALSE, tvb, offset, pinfo, tree, -1);
					break;
				case 13: /*Deactivate SS -- imports operations from MAP-SupplementaryServiceOperations*/
					offset=dissect_gsm_map_SS_ForBS_Code(FALSE, tvb, offset, pinfo, tree, -1);
					break;
				case 14: /*Interrogate SS -- imports operations from MAP-SupplementaryServiceOperations*/
					offset=dissect_gsm_map_SS_ForBS_Code(FALSE, tvb, offset, pinfo, tree, -1);
					break;
				case 16: /*Notify SS */
					dissect_gsm_ss_NotifySS_Arg(FALSE, tvb, offset, pinfo, tree, -1);
					break;
				case 17: /*Register Password -- imports operations from MAP-SupplementaryServiceOperations*/
					offset=dissect_gsm_map_SS_Code(FALSE, tvb, offset, pinfo, tree, -1);
					break;
				case 18: /*Get Password -- imports operations from MAP-SupplementaryServiceOperations*/
					 offset=dissect_gsm_map_GetPasswordArg(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_getPassword);
					break;
				case 19: /*Process Unstructured SS Data */
					dissect_gsm_ss_SS_UserData(FALSE, tvb, offset, pinfo, tree, -1);
					break;
				case 38: /*Forward Check SS Indication -- imports operation from MAP-MobileServiceOperations*/
					break;
				case 59: /*Process Unstructured SS Request -- imports operations from MAP-SupplementaryServiceOperations*/
					 offset=dissect_gsm_map_Ussd_Arg(FALSE, tvb, offset, pinfo, tree, -1);
					break;
				case 60: /*Unstructured SS Request -- imports operations from MAP-SupplementaryServiceOperations*/
					offset=dissect_gsm_map_Ussd_Arg(FALSE, tvb, offset, pinfo, tree, -1);
					break;
				case 61: /*Unstructured SS Notify -- imports operations from MAP-SupplementaryServiceOperations*/
					offset=dissect_gsm_map_Ussd_Arg(FALSE, tvb, offset, pinfo, tree, -1);
					break;
				case 77: /*Erase CC Entry -- imports operations from MAP-SupplementaryServiceOperations*/
					offset=dissect_gsm_map_EraseCC_EntryArg(FALSE, tvb, offset, pinfo, tree, -1);
					break;
				case 112: /*lcs-AreaEventCancellation */
					dissect_gsm_ss_LCS_AreaEventCancellationArg(FALSE, tvb, offset, pinfo, tree, -1);
					break;
				case 113: /*lcs-AreaEventReport */
					dissect_gsm_ss_LCS_AreaEventReportArg(FALSE, tvb, offset, pinfo, tree, -1);
					break;
				case 114: /*LCS-AreaEventRequest */
					dissect_gsm_ss_LCS_AreaEventRequestArg(FALSE, tvb, offset, pinfo, tree, -1);
					break;
				case 115: /*LCS MOLR */
					dissect_gsm_ss_LCS_MOLRArg(FALSE, tvb, offset, pinfo, tree, -1);
					break;
				case 116: /*LCS Location Notification */
					dissect_gsm_ss_LocationNotificationArg(FALSE, tvb, offset, pinfo, tree, -1);
					break;
				case 117: /*Call Deflection */
					dissect_gsm_ss_CallDeflectionArg(FALSE, tvb, offset, pinfo, tree, -1);
					break;
				case 118: /*User User Service */
					dissect_gsm_ss_UserUserServiceArg(FALSE, tvb, offset, pinfo, tree, -1);
					break;
				case 119: /* Access Register CC Entry */
					dissect_gsm_ss_AccessRegisterCCEntryArg(FALSE, tvb, offset, pinfo, tree, -1);
					break;
				case 120: /*Forward CUG Info */
					break;
				case 121: /*Split MPTY */
					break;
				case 122: /*Retrieve MPTY */
					break;
				case 123: /*Hold MPTY */
					break;
				case 124: /*Build MPTY */
					break;
				case 125: /*Forward Charge Advice */
					dissect_gsm_ss_ForwardChargeAdviceArg(FALSE, tvb, offset, pinfo, tree, -1);
					break;
				case 126: /*Explicit CT */
					break;
				default:
					break;
				}
			break;
		case 2: /* returnResultLast */
			switch (opcode){
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
				case 16: /*Notify SS */
					break;
				case 17: /*Register Password -- imports operations from MAP-SupplementaryServiceOperations*/
				    offset=dissect_gsm_map_NewPassword(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_SS_Code);
					break;
				case 18: /*Get Password -- imports operations from MAP-SupplementaryServiceOperations*/
					offset=dissect_gsm_map_CurrentPassword(FALSE, tvb, offset, pinfo, tree, hf_gsm_ss_currentPassword);
					break;
				case 19: /*Process Unstructured SS Data */
					offset=dissect_gsm_ss_SS_UserData(FALSE, tvb, offset, pinfo, tree, -1);
					break;
				case 38: /*Forward Check SS Indication -- imports operation from MAP-MobileServiceOperations*/
					break;
				case 59: /*Process Unstructured SS Request -- imports operations from MAP-SupplementaryServiceOperations*/
					 offset=dissect_gsm_map_Ussd_Res(FALSE, tvb, offset, pinfo, tree, -1);
					break;
				case 60: /*Unstructured SS Request -- imports operations from MAP-SupplementaryServiceOperations*/
					offset=dissect_gsm_map_Ussd_Res(FALSE, tvb, offset, pinfo, tree, -1);
					break;
				case 61: /*Unstructured SS Notify -- imports operations from MAP-SupplementaryServiceOperations*/
					offset=dissect_gsm_map_Ussd_Res(FALSE, tvb, offset, pinfo, tree, -1);
					break;
				case 77: /*Erase CC Entry -- imports operations from MAP-SupplementaryServiceOperations*/
					offset=dissect_gsm_map_EraseCC_EntryRes(FALSE, tvb, offset, pinfo, tree, -1);
					break;
				case 112: /*lcs-AreaEventCancellation */
					break;
				case 113: /*lcs-AreaEventReport */
					break;
				case 114: /*LCS-AreaEventRequest */
					break;
				case 115: /*LCS MOLR */
					offset=dissect_gsm_ss_LCS_MOLRRes(FALSE, tvb, offset, pinfo, tree, -1);
					break;
				case 116: /*LCS Location Notification */
					offset=dissect_gsm_ss_LocationNotificationRes(FALSE, tvb, offset, pinfo, tree, -1);
					break;
				case 117: /*Call Deflection */
					break;
				case 118: /*User User Service */
					break;
				case 119: /* Access Register CC Entry */
				    offset=dissect_gsm_map_RegisterCC_EntryRes(FALSE, tvb, offset, pinfo, tree, -1);
					break;
				case 120: /*Forward CUG Info */
					break;
				case 121: /*Split MPTY */
					break;
				case 122: /*Retrieve MPTY */
					break;
				case 123: /*Hold MPTY */
					break;
				case 124: /*Build MPTY */
					break;
				case 125: /*Forward Charge Advice */
					break;
				case 126: /*Explicit CT */
					break;
				default:
					break;
			}
			break;
		case 3: /* returnError */
			break;
		case 4: /* reject */
			break;
		default:
			break;
	}
	return offset;
}

static void
dissect_gsm_ss(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{

}

/*--- proto_reg_handoff_gsm_ss ---------------------------------------
This proto is called directly from packet-gsm_a and needs to know component type */
void proto_reg_handoff_gsm_ss(void) {
    dissector_handle_t	gsm_ss_handle;

    gsm_ss_handle = create_dissector_handle(dissect_gsm_ss, proto_gsm_ss);


}

/*--- proto_register_gsm_ss -------------------------------------------*/
void proto_register_gsm_ss(void) {

  /* List of fields */
  static hf_register_info hf[] = {
    { &hf_gsm_ss_getPassword,
      { "Password", "gsm_ss.password",
        FT_UINT8, BASE_DEC, VALS(gsm_map_GetPasswordArg_vals), 0,
        "Password", HFILL }},
    { &hf_gsm_ss_currentPassword,
      { "currentPassword", "gsm_ss.currentPassword",
        FT_STRING, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_gsm_ss_SS_Code,
      { "ss-Code", "gsm_ss.ss_Code",
        FT_UINT8, BASE_DEC, VALS(ssCode_vals), 0,
        "", HFILL }},


/*--- Included file: packet-gsm_ss-hfarr.c ---*/

    { &hf_gsm_ss_ss_Code,
      { "ss-Code", "gsm_ss.ss_Code",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_gsm_ss_ss_Status,
      { "ss-Status", "gsm_ss.ss_Status",
        FT_NONE, BASE_NONE, NULL, 0,
        "NotifySS-Arg/ss-Status", HFILL }},
    { &hf_gsm_ss_ss_Notification,
      { "ss-Notification", "gsm_ss.ss_Notification",
        FT_BYTES, BASE_HEX, NULL, 0,
        "NotifySS-Arg/ss-Notification", HFILL }},
    { &hf_gsm_ss_callIsWaiting_Indicator,
      { "callIsWaiting-Indicator", "gsm_ss.callIsWaiting_Indicator",
        FT_NONE, BASE_NONE, NULL, 0,
        "NotifySS-Arg/callIsWaiting-Indicator", HFILL }},
    { &hf_gsm_ss_callOnHold_Indicator,
      { "callOnHold-Indicator", "gsm_ss.callOnHold_Indicator",
        FT_UINT32, BASE_DEC, VALS(gsm_ss_CallOnHold_Indicator_vals), 0,
        "NotifySS-Arg/callOnHold-Indicator", HFILL }},
    { &hf_gsm_ss_mpty_Indicator,
      { "mpty-Indicator", "gsm_ss.mpty_Indicator",
        FT_NONE, BASE_NONE, NULL, 0,
        "NotifySS-Arg/mpty-Indicator", HFILL }},
    { &hf_gsm_ss_cug_Index,
      { "cug-Index", "gsm_ss.cug_Index",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_gsm_ss_clirSuppressionRejected,
      { "clirSuppressionRejected", "gsm_ss.clirSuppressionRejected",
        FT_NONE, BASE_NONE, NULL, 0,
        "NotifySS-Arg/clirSuppressionRejected", HFILL }},
    { &hf_gsm_ss_ect_Indicator,
      { "ect-Indicator", "gsm_ss.ect_Indicator",
        FT_NONE, BASE_NONE, NULL, 0,
        "NotifySS-Arg/ect-Indicator", HFILL }},
    { &hf_gsm_ss_nameIndicator,
      { "nameIndicator", "gsm_ss.nameIndicator",
        FT_NONE, BASE_NONE, NULL, 0,
        "NotifySS-Arg/nameIndicator", HFILL }},
    { &hf_gsm_ss_ccbs_Feature,
      { "ccbs-Feature", "gsm_ss.ccbs_Feature",
        FT_NONE, BASE_NONE, NULL, 0,
        "NotifySS-Arg/ccbs-Feature", HFILL }},
    { &hf_gsm_ss_alertingPattern,
      { "alertingPattern", "gsm_ss.alertingPattern",
        FT_BYTES, BASE_HEX, NULL, 0,
        "NotifySS-Arg/alertingPattern", HFILL }},
    { &hf_gsm_ss_multicall_Indicator,
      { "multicall-Indicator", "gsm_ss.multicall_Indicator",
        FT_UINT32, BASE_DEC, VALS(gsm_ss_Multicall_Indicator_vals), 0,
        "NotifySS-Arg/multicall-Indicator", HFILL }},
    { &hf_gsm_ss_chargingInformation,
      { "chargingInformation", "gsm_ss.chargingInformation",
        FT_NONE, BASE_NONE, NULL, 0,
        "ForwardChargeAdviceArg/chargingInformation", HFILL }},
    { &hf_gsm_ss_e1,
      { "e1", "gsm_ss.e1",
        FT_UINT32, BASE_DEC, NULL, 0,
        "ChargingInformation/e1", HFILL }},
    { &hf_gsm_ss_e2,
      { "e2", "gsm_ss.e2",
        FT_UINT32, BASE_DEC, NULL, 0,
        "ChargingInformation/e2", HFILL }},
    { &hf_gsm_ss_e3,
      { "e3", "gsm_ss.e3",
        FT_UINT32, BASE_DEC, NULL, 0,
        "ChargingInformation/e3", HFILL }},
    { &hf_gsm_ss_e4,
      { "e4", "gsm_ss.e4",
        FT_UINT32, BASE_DEC, NULL, 0,
        "ChargingInformation/e4", HFILL }},
    { &hf_gsm_ss_e5,
      { "e5", "gsm_ss.e5",
        FT_UINT32, BASE_DEC, NULL, 0,
        "ChargingInformation/e5", HFILL }},
    { &hf_gsm_ss_e6,
      { "e6", "gsm_ss.e6",
        FT_UINT32, BASE_DEC, NULL, 0,
        "ChargingInformation/e6", HFILL }},
    { &hf_gsm_ss_e7,
      { "e7", "gsm_ss.e7",
        FT_UINT32, BASE_DEC, NULL, 0,
        "ChargingInformation/e7", HFILL }},
    { &hf_gsm_ss_suppressPrefCUG,
      { "suppressPrefCUG", "gsm_ss.suppressPrefCUG",
        FT_NONE, BASE_NONE, NULL, 0,
        "ForwardCUG-InfoArg/suppressPrefCUG", HFILL }},
    { &hf_gsm_ss_suppressOA,
      { "suppressOA", "gsm_ss.suppressOA",
        FT_NONE, BASE_NONE, NULL, 0,
        "ForwardCUG-InfoArg/suppressOA", HFILL }},
    { &hf_gsm_ss_ect_CallState,
      { "ect-CallState", "gsm_ss.ect_CallState",
        FT_UINT32, BASE_DEC, VALS(gsm_ss_ECT_CallState_vals), 0,
        "ECT-Indicator/ect-CallState", HFILL }},
    { &hf_gsm_ss_rdn,
      { "rdn", "gsm_ss.rdn",
        FT_UINT32, BASE_DEC, VALS(gsm_ss_RDN_vals), 0,
        "ECT-Indicator/rdn", HFILL }},
    { &hf_gsm_ss_callingName,
      { "callingName", "gsm_ss.callingName",
        FT_UINT32, BASE_DEC, VALS(gsm_ss_Name_vals), 0,
        "NameIndicator/callingName", HFILL }},
    { &hf_gsm_ss_namePresentationAllowed,
      { "namePresentationAllowed", "gsm_ss.namePresentationAllowed",
        FT_NONE, BASE_NONE, NULL, 0,
        "Name/namePresentationAllowed", HFILL }},
    { &hf_gsm_ss_presentationRestricted,
      { "presentationRestricted", "gsm_ss.presentationRestricted",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_gsm_ss_nameUnavailable,
      { "nameUnavailable", "gsm_ss.nameUnavailable",
        FT_NONE, BASE_NONE, NULL, 0,
        "Name/nameUnavailable", HFILL }},
    { &hf_gsm_ss_namePresentationRestricted,
      { "namePresentationRestricted", "gsm_ss.namePresentationRestricted",
        FT_NONE, BASE_NONE, NULL, 0,
        "Name/namePresentationRestricted", HFILL }},
    { &hf_gsm_ss_dataCodingScheme,
      { "dataCodingScheme", "gsm_ss.dataCodingScheme",
        FT_NONE, BASE_NONE, NULL, 0,
        "NameSet/dataCodingScheme", HFILL }},
    { &hf_gsm_ss_lengthInCharacters,
      { "lengthInCharacters", "gsm_ss.lengthInCharacters",
        FT_INT32, BASE_DEC, NULL, 0,
        "NameSet/lengthInCharacters", HFILL }},
    { &hf_gsm_ss_nameString,
      { "nameString", "gsm_ss.nameString",
        FT_NONE, BASE_NONE, NULL, 0,
        "NameSet/nameString", HFILL }},
    { &hf_gsm_ss_presentationAllowedAddress,
      { "presentationAllowedAddress", "gsm_ss.presentationAllowedAddress",
        FT_NONE, BASE_NONE, NULL, 0,
        "RDN/presentationAllowedAddress", HFILL }},
    { &hf_gsm_ss_numberNotAvailableDueToInterworking,
      { "numberNotAvailableDueToInterworking", "gsm_ss.numberNotAvailableDueToInterworking",
        FT_NONE, BASE_NONE, NULL, 0,
        "RDN/numberNotAvailableDueToInterworking", HFILL }},
    { &hf_gsm_ss_presentationRestrictedAddress,
      { "presentationRestrictedAddress", "gsm_ss.presentationRestrictedAddress",
        FT_NONE, BASE_NONE, NULL, 0,
        "RDN/presentationRestrictedAddress", HFILL }},
    { &hf_gsm_ss_partyNumber,
      { "partyNumber", "gsm_ss.partyNumber",
        FT_NONE, BASE_NONE, NULL, 0,
        "RemotePartyNumber/partyNumber", HFILL }},
    { &hf_gsm_ss_partyNumberSubaddress,
      { "partyNumberSubaddress", "gsm_ss.partyNumberSubaddress",
        FT_NONE, BASE_NONE, NULL, 0,
        "RemotePartyNumber/partyNumberSubaddress", HFILL }},
    { &hf_gsm_ss_deflectedToNumber,
      { "deflectedToNumber", "gsm_ss.deflectedToNumber",
        FT_BYTES, BASE_HEX, NULL, 0,
        "CallDeflectionArg/deflectedToNumber", HFILL }},
    { &hf_gsm_ss_deflectedToSubaddress,
      { "deflectedToSubaddress", "gsm_ss.deflectedToSubaddress",
        FT_NONE, BASE_NONE, NULL, 0,
        "CallDeflectionArg/deflectedToSubaddress", HFILL }},
    { &hf_gsm_ss_uUS_Service,
      { "uUS-Service", "gsm_ss.uUS_Service",
        FT_UINT32, BASE_DEC, VALS(gsm_ss_UUS_Service_vals), 0,
        "UserUserServiceArg/uUS-Service", HFILL }},
    { &hf_gsm_ss_uUS_Required,
      { "uUS-Required", "gsm_ss.uUS_Required",
        FT_BOOLEAN, 8, NULL, 0,
        "UserUserServiceArg/uUS-Required", HFILL }},
    { &hf_gsm_ss_notificationType,
      { "notificationType", "gsm_ss.notificationType",
        FT_UINT32, BASE_DEC, VALS(gsm_map_NotificationToMSUser_vals), 0,
        "LocationNotificationArg/notificationType", HFILL }},
    { &hf_gsm_ss_locationType,
      { "locationType", "gsm_ss.locationType",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_gsm_ss_lcsClientExternalID,
      { "lcsClientExternalID", "gsm_ss.lcsClientExternalID",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_gsm_ss_lcsClientName,
      { "lcsClientName", "gsm_ss.lcsClientName",
        FT_NONE, BASE_NONE, NULL, 0,
        "LocationNotificationArg/lcsClientName", HFILL }},
    { &hf_gsm_ss_lcsRequestorID,
      { "lcsRequestorID", "gsm_ss.lcsRequestorID",
        FT_NONE, BASE_NONE, NULL, 0,
        "LocationNotificationArg/lcsRequestorID", HFILL }},
    { &hf_gsm_ss_lcsCodeword,
      { "lcsCodeword", "gsm_ss.lcsCodeword",
        FT_NONE, BASE_NONE, NULL, 0,
        "LocationNotificationArg/lcsCodeword", HFILL }},
    { &hf_gsm_ss_lcsServiceTypeID,
      { "lcsServiceTypeID", "gsm_ss.lcsServiceTypeID",
        FT_UINT32, BASE_DEC, NULL, 0,
        "", HFILL }},
    { &hf_gsm_ss_verificationResponse,
      { "verificationResponse", "gsm_ss.verificationResponse",
        FT_UINT32, BASE_DEC, VALS(gsm_ss_VerificationResponse_vals), 0,
        "LocationNotificationRes/verificationResponse", HFILL }},
    { &hf_gsm_ss_molr_Type,
      { "molr-Type", "gsm_ss.molr_Type",
        FT_UINT32, BASE_DEC, VALS(gsm_ss_MOLR_Type_vals), 0,
        "LCS-MOLRArg/molr-Type", HFILL }},
    { &hf_gsm_ss_locationMethod,
      { "locationMethod", "gsm_ss.locationMethod",
        FT_UINT32, BASE_DEC, VALS(gsm_ss_LocationMethod_vals), 0,
        "LCS-MOLRArg/locationMethod", HFILL }},
    { &hf_gsm_ss_lcs_QoS,
      { "lcs-QoS", "gsm_ss.lcs_QoS",
        FT_NONE, BASE_NONE, NULL, 0,
        "LCS-MOLRArg/lcs-QoS", HFILL }},
    { &hf_gsm_ss_mlc_Number,
      { "mlc-Number", "gsm_ss.mlc_Number",
        FT_NONE, BASE_NONE, NULL, 0,
        "LCS-MOLRArg/mlc-Number", HFILL }},
    { &hf_gsm_ss_gpsAssistanceData,
      { "gpsAssistanceData", "gsm_ss.gpsAssistanceData",
        FT_BYTES, BASE_HEX, NULL, 0,
        "LCS-MOLRArg/gpsAssistanceData", HFILL }},
    { &hf_gsm_ss_supportedGADShapes,
      { "supportedGADShapes", "gsm_ss.supportedGADShapes",
        FT_BYTES, BASE_HEX, NULL, 0,
        "LCS-MOLRArg/supportedGADShapes", HFILL }},
    { &hf_gsm_ss_ageOfLocationInfo,
      { "ageOfLocationInfo", "gsm_ss.ageOfLocationInfo",
        FT_UINT32, BASE_DEC, NULL, 0,
        "LCS-MOLRArg/ageOfLocationInfo", HFILL }},
    { &hf_gsm_ss_pseudonymIndicator,
      { "pseudonymIndicator", "gsm_ss.pseudonymIndicator",
        FT_NONE, BASE_NONE, NULL, 0,
        "LCS-MOLRArg/pseudonymIndicator", HFILL }},
    { &hf_gsm_ss_locationEstimate,
      { "locationEstimate", "gsm_ss.locationEstimate",
        FT_NONE, BASE_NONE, NULL, 0,
        "LCS-MOLRRes/locationEstimate", HFILL }},
    { &hf_gsm_ss_decipheringKeys,
      { "decipheringKeys", "gsm_ss.decipheringKeys",
        FT_BYTES, BASE_HEX, NULL, 0,
        "LCS-MOLRRes/decipheringKeys", HFILL }},
    { &hf_gsm_ss_add_LocationEstimate,
      { "add-LocationEstimate", "gsm_ss.add_LocationEstimate",
        FT_NONE, BASE_NONE, NULL, 0,
        "LCS-MOLRRes/add-LocationEstimate", HFILL }},
    { &hf_gsm_ss_referenceNumber,
      { "referenceNumber", "gsm_ss.referenceNumber",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_gsm_ss_h_gmlc_address,
      { "h-gmlc-address", "gsm_ss.h_gmlc_address",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_gsm_ss_deferredLocationEventType,
      { "deferredLocationEventType", "gsm_ss.deferredLocationEventType",
        FT_BYTES, BASE_HEX, NULL, 0,
        "LCS-AreaEventRequestArg/deferredLocationEventType", HFILL }},
    { &hf_gsm_ss_areaEventInfo,
      { "areaEventInfo", "gsm_ss.areaEventInfo",
        FT_NONE, BASE_NONE, NULL, 0,
        "LCS-AreaEventRequestArg/areaEventInfo", HFILL }},

/*--- End of included file: packet-gsm_ss-hfarr.c ---*/

  };

  /* List of subtrees */
  static gint *ett[] = {

/*--- Included file: packet-gsm_ss-ettarr.c ---*/

    &ett_gsm_ss_NotifySS_Arg,
    &ett_gsm_ss_ForwardChargeAdviceArg,
    &ett_gsm_ss_ChargingInformation,
    &ett_gsm_ss_ForwardCUG_InfoArg,
    &ett_gsm_ss_ECT_Indicator,
    &ett_gsm_ss_NameIndicator,
    &ett_gsm_ss_Name,
    &ett_gsm_ss_NameSet,
    &ett_gsm_ss_RDN,
    &ett_gsm_ss_RemotePartyNumber,
    &ett_gsm_ss_AccessRegisterCCEntryArg,
    &ett_gsm_ss_CallDeflectionArg,
    &ett_gsm_ss_UserUserServiceArg,
    &ett_gsm_ss_LocationNotificationArg,
    &ett_gsm_ss_LocationNotificationRes,
    &ett_gsm_ss_LCS_MOLRArg,
    &ett_gsm_ss_LCS_MOLRRes,
    &ett_gsm_ss_LCS_AreaEventRequestArg,
    &ett_gsm_ss_LCS_AreaEventReportArg,
    &ett_gsm_ss_LCS_AreaEventCancellationArg,

/*--- End of included file: packet-gsm_ss-ettarr.c ---*/

  };

  /* Register protocol */
  proto_gsm_ss = proto_register_protocol(PNAME, PSNAME, PFNAME); 
/*XXX  register_dissector("gsm_ss", dissect_gsm_ss, proto_gsm_ss);*/
  /* Register fields and subtrees */
  proto_register_field_array(proto_gsm_ss, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));


}


