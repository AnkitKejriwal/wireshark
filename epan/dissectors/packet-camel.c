/* Do not modify this file.                                                   */
/* It is created automatically by the ASN.1 to Ethereal dissector compiler    */
/* ./packet-camel.c                                                           */
/* ../../tools/asn2eth.py -X -b -e -p camel -c camel.cnf -s packet-camel-template camel.asn */

/* Input file: packet-camel-template.c */

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
#include "packet-gsm_map.h"

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

/*--- Included file: packet-camel-hf.c ---*/

static int hf_camel_reserved = -1;                /* INTEGER */
static int hf_camel_aoc = -1;                     /* INTEGER */
static int hf_camel_standardPartEnd = -1;         /* INTEGER */
static int hf_camel_genOfVoiceAnn = -1;           /* INTEGER */
static int hf_camel_voiceInfo2 = -1;              /* INTEGER */
static int hf_camel_voiceInfo1 = -1;              /* INTEGER */
static int hf_camel_voiceBack = -1;               /* INTEGER */
static int hf_camel_iPRoutAdd = -1;               /* INTEGER */
static int hf_camel_bilateralPart = -1;           /* OCTET_STRING_SIZE_0_3 */
static int hf_camel_extension = -1;               /* INTEGER_1 */
static int hf_camel_natureOfAddressIndicator = -1;  /* INTEGER */
static int hf_camel_numberingPlanInd = -1;        /* INTEGER */
static int hf_camel_digits = -1;                  /* OCTET_STRING_SIZE_0_19 */
static int hf_camel_digits1 = -1;                 /* OCTET_STRING_SIZE_0_8 */
static int hf_camel_typeOfShape = -1;             /* INTEGER */
static int hf_camel_spare = -1;                   /* INTEGER */
static int hf_camel_degreesOfLatitude = -1;       /* OCTET_STRING_SIZE_3 */
static int hf_camel_degreesOfLongitude = -1;      /* OCTET_STRING_SIZE_3 */
static int hf_camel_uncertaintyCode = -1;         /* OCTET_STRING_SIZE_1 */
static int hf_camel_typeOfAddress = -1;           /* INTEGER */
static int hf_camel_addressLength = -1;           /* INTEGER_4_16 */
static int hf_camel_address = -1;                 /* OCTET_STRING_SIZE_4_16 */
static int hf_camel_originalReasons = -1;         /* INTEGER */
static int hf_camel_spare1 = -1;                  /* INTEGER_0 */
static int hf_camel_indicator = -1;               /* INTEGER */
static int hf_camel_reason = -1;                  /* INTEGER */
static int hf_camel_spare2 = -1;                  /* INTEGER_0 */
static int hf_camel_counter = -1;                 /* INTEGER */
static int hf_camel_oddEven = -1;                 /* INTEGER */
static int hf_camel_innInd = -1;                  /* INTEGER */
static int hf_camel_digits2 = -1;                 /* OCTET_STRING_SIZE_0_16 */
static int hf_camel_niInd = -1;                   /* INTEGER */
static int hf_camel_presentInd = -1;              /* INTEGER */
static int hf_camel_screening = -1;               /* INTEGER */
static int hf_camel_digits3 = -1;                 /* OCTET_STRING_SIZE_1_10 */
static int hf_camel_o1ext = -1;                   /* INTEGER_1 */
static int hf_camel_codingStandard = -1;          /* INTEGER */
static int hf_camel_location = -1;                /* INTEGER */
static int hf_camel_o2ext = -1;                   /* INTEGER_1 */
static int hf_camel_causeValue = -1;              /* INTEGER */
static int hf_camel_diagnostics = -1;             /* OCTET_STRING_SIZE_0_30 */
static int hf_camel_numberQualifierIndicator = -1;  /* INTEGER */
static int hf_camel_ext = -1;                     /* INTEGER */
static int hf_camel_typeOfNumber = -1;            /* T_typeOfNumber */
static int hf_camel_digits4 = -1;                 /* OCTET_STRING_SIZE_0_40 */
static int hf_camel_destinationReference = -1;    /* Integer4 */
static int hf_camel_originationReference = -1;    /* Integer4 */
static int hf_camel_transferredVolumeRollOver = -1;  /* TransferredVolumeRollOver */
static int hf_camel_elapsedTimeRollOver = -1;     /* ElapsedTimeRollOver */
static int hf_camel_conferenceTreatmentIndicator = -1;  /* OCTET_STRING_SIZE_1 */
static int hf_camel_callCompletionTreatmentIndicator = -1;  /* OCTET_STRING_SIZE_1 */
static int hf_camel_bearerCap = -1;               /* OCTET_STRING_SIZE_2_11 */
static int hf_camel_e1 = -1;                      /* INTEGER_0_8191 */
static int hf_camel_e2 = -1;                      /* INTEGER_0_8191 */
static int hf_camel_e3 = -1;                      /* INTEGER_0_8191 */
static int hf_camel_e4 = -1;                      /* INTEGER_0_8191 */
static int hf_camel_e5 = -1;                      /* INTEGER_0_8191 */
static int hf_camel_e6 = -1;                      /* INTEGER_0_8191 */
static int hf_camel_e7 = -1;                      /* INTEGER_0_8191 */
static int hf_camel_maxTransferredVolume = -1;    /* INTEGER_1_2147483647 */
static int hf_camel_maxElapsedTime = -1;          /* INTEGER_1_86400 */
static int hf_camel_timeGPRSIfNoTariffSwitch = -1;  /* INTEGER_0_86400 */
static int hf_camel_timeGPRSIfTariffSwitch = -1;  /* T_timeGPRSIfTariffSwitch */
static int hf_camel_timeGPRSSinceLastTariffSwitch = -1;  /* INTEGER_0_86400 */
static int hf_camel_timeGPRSTariffSwitchInterval = -1;  /* INTEGER_0_86400 */
static int hf_camel_rOTimeGPRSIfNoTariffSwitch = -1;  /* INTEGER_0_255 */
static int hf_camel_rOTimeGPRSIfTariffSwitch = -1;  /* T_rOTimeGPRSIfTariffSwitch */
static int hf_camel_rOTimeGPRSSinceLastTariffSwitch = -1;  /* INTEGER_0_255 */
static int hf_camel_rOTimeGPRSTariffSwitchInterval = -1;  /* INTEGER_0_255 */
static int hf_camel_callDiversionTreatmentIndicator = -1;  /* OCTET_STRING_SIZE_1 */
static int hf_camel_callingPartyRestrictionIndicator = -1;  /* OCTET_STRING_SIZE_1 */
static int hf_camel_pDPTypeOrganization = -1;     /* OCTET_STRING_SIZE_1 */
static int hf_camel_pDPTypeNumber = -1;           /* OCTET_STRING_SIZE_1 */
static int hf_camel_pDPAddress = -1;              /* OCTET_STRING_SIZE_1_63 */
static int hf_camel_timeSinceTariffSwitch = -1;   /* INTEGER_0_864000 */
static int hf_camel_tariffSwitchInterval = -1;    /* INTEGER_1_864000 */
static int hf_camel_volumeIfNoTariffSwitch = -1;  /* INTEGER_0_2147483647 */
static int hf_camel_volumeIfTariffSwitch = -1;    /* T_volumeIfTariffSwitch */
static int hf_camel_volumeSinceLastTariffSwitch = -1;  /* INTEGER_0_2147483647 */
static int hf_camel_volumeTariffSwitchInterval = -1;  /* INTEGER_0_2147483647 */
static int hf_camel_rOVolumeIfNoTariffSwitch = -1;  /* INTEGER_0_255 */
static int hf_camel_rOVolumeIfTariffSwitch = -1;  /* T_rOVolumeIfTariffSwitch */
static int hf_camel_rOVolumeSinceLastTariffSwitch = -1;  /* INTEGER_0_255 */
static int hf_camel_rOVolumeTariffSwitchInterval = -1;  /* INTEGER_0_255 */
static int hf_camel_local = -1;                   /* INTEGER */
static int hf_camel_global = -1;                  /* OBJECT_IDENTIFIER */
static int hf_camel_foo = -1;                     /* INTEGER_0 */
static int hf_camel_messageType = -1;             /* T_messageType */
static int hf_camel_firstExtensionExtensionType = -1;  /* NULL */
static int hf_camel_extId = -1;                   /* ExtensionSetextensionId */
static int hf_camel_cAI_GSM0224 = -1;             /* CAI_Gsm0224 */
static int hf_camel_tariffSwitchInterval1 = -1;   /* INTEGER_1_86400 */
static int hf_camel_fCIBCCCAMELsequence1 = -1;    /* T_fCIBCCCAMELsequence1 */
static int hf_camel_freeFormatData = -1;          /* FreeFormatData */
static int hf_camel_pDPID = -1;                   /* PDPId */
static int hf_camel_appendFreeFormatData = -1;    /* AppendFreeFormatData */
static int hf_camel_fCIBCCCAMELsequence11 = -1;   /* T_fCIBCCCAMELsequence11 */
static int hf_camel_transferredVolume = -1;       /* TransferredVolume */
static int hf_camel_elapsedTime = -1;             /* ElapsedTime */
static int hf_camel_minimumNbOfDigits = -1;       /* INTEGER_1_30 */
static int hf_camel_maximumNbOfDigits = -1;       /* INTEGER_1_30 */
static int hf_camel_endOfReplyDigit = -1;         /* OCTET_STRING_SIZE_1_2 */
static int hf_camel_cancelDigit = -1;             /* OCTET_STRING_SIZE_1_2 */
static int hf_camel_startDigit = -1;              /* OCTET_STRING_SIZE_1_2 */
static int hf_camel_firstDigitTimeOut = -1;       /* INTEGER_1_127 */
static int hf_camel_interDigitTimeOut = -1;       /* INTEGER_1_127 */
static int hf_camel_errorTreatment = -1;          /* ErrorTreatment */
static int hf_camel_interruptableAnnInd = -1;     /* BOOLEAN */
static int hf_camel_voiceInformation = -1;        /* BOOLEAN */
static int hf_camel_voiceBack1 = -1;              /* BOOLEAN */
static int hf_camel_collectedDigits = -1;         /* CollectedDigits */
static int hf_camel_DestinationRoutingAddress_item = -1;  /* CalledPartyNumber */
static int hf_camel_applicationTimer = -1;        /* ApplicationTimer */
static int hf_camel_routeSelectFailureSpecificInfo = -1;  /* T_routeSelectFailureSpecificInfo */
static int hf_camel_failureCause = -1;            /* Cause */
static int hf_camel_oCalledPartyBusySpecificInfo = -1;  /* T_oCalledPartyBusySpecificInfo */
static int hf_camel_busyCause = -1;               /* Cause */
static int hf_camel_oNoAnswerSpecificInfo = -1;   /* T_oNoAnswerSpecificInfo */
static int hf_camel_oAnswerSpecificInfo = -1;     /* T_oAnswerSpecificInfo */
static int hf_camel_destinationAddress = -1;      /* CalledPartyNumber */
static int hf_camel_or_Call = -1;                 /* NULL */
static int hf_camel_forwardedCall = -1;           /* NULL */
static int hf_camel_oDisconnectSpecificInfo = -1;  /* T_oDisconnectSpecificInfo */
static int hf_camel_releaseCause = -1;            /* Cause */
static int hf_camel_tBusySpecificInfo = -1;       /* T_tBusySpecificInfo */
static int hf_camel_callForwarded = -1;           /* NULL */
static int hf_camel_routeNotPermitted = -1;       /* NULL */
static int hf_camel_tNoAnswerSpecificInfo = -1;   /* T_tNoAnswerSpecificInfo */
static int hf_camel_tAnswerSpecificInfo = -1;     /* T_tAnswerSpecificInfo */
static int hf_camel_tDisconnectSpecificInfo = -1;  /* T_tDisconnectSpecificInfo */
static int hf_camel_o_smsFailureSpecificInfo = -1;  /* T_o_smsFailureSpecificInfo */
static int hf_camel_failureCause1 = -1;           /* SMSCause */
static int hf_camel_o_smsSubmittedSpecificInfo = -1;  /* T_o_smsSubmittedSpecificInfo */
static int hf_camel_duration = -1;                /* Duration */
static int hf_camel_gapInterval = -1;             /* Interval */
static int hf_camel_serviceKey = -1;              /* ServiceKey */
static int hf_camel_GenericNumbers_item = -1;     /* GenericNumber */
static int hf_camel_short_QoS_format = -1;        /* QoS_Subscribed */
static int hf_camel_long_QoS_format = -1;         /* Ext_QoS_Subscribed */
static int hf_camel_gPRSEventType = -1;           /* GPRSEventType */
static int hf_camel_monitorMode = -1;             /* MonitorMode */
static int hf_camel_mSNetworkCapability = -1;     /* MSNetworkCapability */
static int hf_camel_mSRadioAccessCapability = -1;  /* MSRadioAccessCapability */
static int hf_camel_requested_QoS = -1;           /* GPRS_QoS */
static int hf_camel_subscribed_QoS = -1;          /* GPRS_QoS */
static int hf_camel_negotiated_QoS = -1;          /* GPRS_QoS */
static int hf_camel_receivingSideID = -1;         /* LegType */
static int hf_camel_RequestedInformationTypeList_item = -1;  /* RequestedInformationType */
static int hf_camel_callAttemptElapsedTimeValue = -1;  /* INTEGER_0_255 */
static int hf_camel_callStopTimeValue = -1;       /* DateAndTime */
static int hf_camel_callConnectedElapsedTimeValue = -1;  /* Integer4 */
static int hf_camel_releaseCauseValue = -1;       /* Cause */
static int hf_camel_sendingSideID = -1;           /* LegType */
static int hf_camel_forwardServiceInteractionInd = -1;  /* ForwardServiceInteractionInd */
static int hf_camel_backwardServiceInteractionInd = -1;  /* BackwardServiceInteractionInd */
static int hf_camel_bothwayThroughConnectionInd = -1;  /* BothwayThroughConnectionInd */
static int hf_camel_connectedNumberTreatmentInd = -1;  /* ConnectedNumberTreatmentInd */
static int hf_camel_nonCUGCall = -1;              /* NULL */
static int hf_camel_holdTreatmentIndicator = -1;  /* OCTET_STRING_SIZE_1 */
static int hf_camel_cwTreatmentIndicator = -1;    /* OCTET_STRING_SIZE_1 */
static int hf_camel_ectTreatmentIndicator = -1;   /* OCTET_STRING_SIZE_1 */
static int hf_camel_eventTypeSMS = -1;            /* EventTypeSMS */
static int hf_camel_timeIfNoTariffSwitch = -1;    /* TimeIfNoTariffSwitch */
static int hf_camel_timeIfTariffSwitch = -1;      /* TimeIfTariffSwitch */
static int hf_camel_toneID = -1;                  /* Integer4 */
static int hf_camel_duration1 = -1;               /* Integer4 */
static int hf_camel_integer = -1;                 /* Integer4 */
static int hf_camel_number = -1;                  /* Digits */
static int hf_camel_time = -1;                    /* OCTET_STRING_SIZE_2 */
static int hf_camel_date = -1;                    /* OCTET_STRING_SIZE_4 */
static int hf_camel_price = -1;                   /* OCTET_STRING_SIZE_4 */
static int hf_camel_invokeID = -1;                /* InvokeID */
static int hf_camel_allRequests = -1;             /* NULL */
static int hf_camel_digitsResponse = -1;          /* Digits */
static int hf_camel_accessPointName = -1;         /* AccessPointName */
static int hf_camel_pdpID = -1;                   /* PDPId */
static int hf_camel_gPRSCause = -1;               /* GPRSCause */
static int hf_camel_gprsCause = -1;               /* GPRSCause */
static int hf_camel_gPRSEvent = -1;               /* GPRSEventArray */
static int hf_camel_GPRSEventArray_item = -1;     /* GPRSEvent */
static int hf_camel_sCIGPRSBillingChargingCharacteristics = -1;  /* SCIGPRSBillingChargingCharacteristics */
static int hf_camel_assumedIdle = -1;             /* NULL */
static int hf_camel_camelBusy = -1;               /* NULL */
static int hf_camel_netDetNotReachable = -1;      /* NotReachableReason */
static int hf_camel_notProvidedFromVLR = -1;      /* NULL */
static int hf_camel_PrivateExtensionList_item = -1;  /* PrivateExtension */
static int hf_camel_cellIdFixedLength = -1;       /* CellIdFixedLength */
static int hf_camel_laiFixedLength = -1;          /* LAIFixedLength */
static int hf_camel_ext_BearerService = -1;       /* Ext_BearerServiceCode */
static int hf_camel_ext_Teleservice = -1;         /* Ext_TeleserviceCode */
static int hf_camel_aOCInitial = -1;              /* CAI_Gsm0224 */
static int hf_camel_aOCSubsequent = -1;           /* AOCSubsequent */
static int hf_camel_calledAddressValue = -1;      /* Digits */
static int hf_camel_gapOnService = -1;            /* GapOnService */
static int hf_camel_calledAddressAndService = -1;  /* T_calledAddressAndService */
static int hf_camel_callingAddressAndService = -1;  /* T_callingAddressAndService */
static int hf_camel_callingAddressValue = -1;     /* Digits */
static int hf_camel_eventTypeBCSM = -1;           /* EventTypeBCSM */
static int hf_camel_legID = -1;                   /* LegID */
static int hf_camel_dpSpecificCriteria = -1;      /* DpSpecificCriteria */
static int hf_camel_fCIBCCCAMELsequence12 = -1;   /* T_fCIBCCCAMELsequence12 */
static int hf_camel_partyToCharge = -1;           /* SendingSideID */
static int hf_camel_basicGapCriteria = -1;        /* BasicGapCriteria */
static int hf_camel_scfID = -1;                   /* ScfID */
static int hf_camel_compoundGapCriteria = -1;     /* CompoundCriteria */
static int hf_camel_elementaryMessageID = -1;     /* Integer4 */
static int hf_camel_text = -1;                    /* T_text */
static int hf_camel_messageContent = -1;          /* IA5String_SIZE_1_127 */
static int hf_camel_attributes = -1;              /* OCTET_STRING_SIZE_2_10 */
static int hf_camel_elementaryMessageIDs = -1;    /* SEQUENCE_SIZE_1_16_OF_Integer4 */
static int hf_camel_elementaryMessageIDs_item = -1;  /* Integer4 */
static int hf_camel_variableMessage = -1;         /* T_variableMessage */
static int hf_camel_variableParts = -1;           /* VariablePartsArray */
static int hf_camel_VariablePartsArray_item = -1;  /* VariablePart */
static int hf_camel_requestedInformationType = -1;  /* RequestedInformationType */
static int hf_camel_requestedInformationValue = -1;  /* RequestedInformationValue */
static int hf_camel_gmscAddress = -1;             /* ISDN_AddressString */
static int hf_camel_messageID = -1;               /* MessageID */
static int hf_camel_numberOfRepetitions = -1;     /* INTEGER_1_127 */
static int hf_camel_duration2 = -1;               /* INTEGER_0_32767 */
static int hf_camel_interval = -1;                /* INTEGER_0_32767 */
static int hf_camel_inbandInfo = -1;              /* InbandInfo */
static int hf_camel_tone = -1;                    /* Tone */
static int hf_camel_cellGlobalIdOrServiceAreaIdOrLAI = -1;  /* CellGlobalIdOrServiceAreaIdOrLAI */
static int hf_camel_routeingAreaIdentity = -1;    /* RAIdentity */
static int hf_camel_geographicalInformation = -1;  /* GeographicalInformation */
static int hf_camel_sgsn_Number = -1;             /* ISDN_AddressString */
static int hf_camel_selectedLSAIdentity = -1;     /* LSAIdentity */
static int hf_camel_extensionContainer = -1;      /* ExtensionContainer */
static int hf_camel_saiPresent = -1;              /* NULL */
static int hf_camel_RequestedInformationList_item = -1;  /* RequestedInformation */
static int hf_camel_ageOfLocationInformation = -1;  /* AgeOfLocationInformation */
static int hf_camel_vlr_number = -1;              /* ISDN_AddressString */
static int hf_camel_locationNumber = -1;          /* LocationNumber */
static int hf_camel_cellIdOrLAI = -1;             /* CellIdOrLAI */
static int hf_camel_selectedLSA_Id = -1;          /* LSAIdentity */
static int hf_camel_msc_number = -1;              /* ISDN_AddressString */
static int hf_camel_geodeticInformation = -1;     /* GeodeticInformation */
static int hf_camel_currentLocationRetrieved = -1;  /* NULL */
static int hf_camel_informationToSend = -1;       /* InformationToSend */
static int hf_camel_attachChangeOfPositionSpecificInformation = -1;  /* T_attachChangeOfPositionSpecificInformation */
static int hf_camel_locationInformationGPRS = -1;  /* LocationInformationGPRS */
static int hf_camel_pdp_ContextchangeOfPositionSpecificInformation = -1;  /* T_pdp_ContextchangeOfPositionSpecificInformation */
static int hf_camel_chargingID = -1;              /* GPRSChargingID */
static int hf_camel_pDPType = -1;                 /* PDPType */
static int hf_camel_qualityOfService = -1;        /* QualityOfService */
static int hf_camel_timeAndTimeZone = -1;         /* TimeAndTimezone */
static int hf_camel_gGSNAddress = -1;             /* GSNAddress */
static int hf_camel_detachSpecificInformation = -1;  /* T_detachSpecificInformation */
static int hf_camel_inititatingEntity = -1;       /* InitiatingEntity */
static int hf_camel_routeingAreaUpdate = -1;      /* NULL */
static int hf_camel_disconnectSpecificInformation = -1;  /* T_disconnectSpecificInformation */
static int hf_camel_pDPContextEstablishmentSpecificInformation = -1;  /* T_pDPContextEstablishmentSpecificInformation */
static int hf_camel_pDPInitiationType = -1;       /* PDPInitiationType */
static int hf_camel_secondaryPDPContext = -1;     /* NULL */
static int hf_camel_pDPContextEstablishmentAcknowledgementSpecificInformation = -1;  /* T_pDPContextEstablishmentAcknowledgementSpecificInformation */
static int hf_camel_miscGPRSInfo = -1;            /* MiscCallInfo */
static int hf_camel_gPRSEventSpecificInformation = -1;  /* GPRSEventSpecificInformation */
static int hf_camel_type = -1;                    /* SupportedExtensionsid */
static int hf_camel_criticality = -1;             /* CriticalityType */
static int hf_camel_value = -1;                   /* SupportedExtensionsExtensionType */
static int hf_camel_aChBillingChargingCharacteristics = -1;  /* AChBillingChargingCharacteristics */
static int hf_camel_extensions = -1;              /* ExtensionsArray */
static int hf_camel_ExtensionsArray_item = -1;    /* ExtensionField */
static int hf_camel_correlationID = -1;           /* CorrelationID */
static int hf_camel_iPSSPCapabilities = -1;       /* IPSSPCapabilities */
static int hf_camel_requestedInformationTypeList = -1;  /* RequestedInformationTypeList */
static int hf_camel_legID1 = -1;                  /* SendingSideID */
static int hf_camel_destinationRoutingAddress = -1;  /* DestinationRoutingAddress */
static int hf_camel_alertingPattern = -1;         /* AlertingPattern */
static int hf_camel_originalCalledPartyID = -1;   /* OriginalCalledPartyID */
static int hf_camel_carrier = -1;                 /* Carrier */
static int hf_camel_callingPartysCategory = -1;   /* CallingPartysCategory */
static int hf_camel_redirectingPartyID = -1;      /* RedirectingPartyID */
static int hf_camel_redirectionInformation = -1;  /* RedirectionInformation */
static int hf_camel_genericNumbers = -1;          /* GenericNumbers */
static int hf_camel_serviceInteractionIndicatorsTwo = -1;  /* ServiceInteractionIndicatorsTwo */
static int hf_camel_chargeNumber = -1;            /* ChargeNumber */
static int hf_camel_cug_Interlock = -1;           /* CUG_Interlock */
static int hf_camel_cug_OutgoingAccess = -1;      /* NULL */
static int hf_camel_suppressionOfAnnouncement = -1;  /* SuppressionOfAnnouncement */
static int hf_camel_oCSIApplicable = -1;          /* OCSIApplicable */
static int hf_camel_naOliInfo = -1;               /* NAOliInfo */
static int hf_camel_resourceAddress = -1;         /* T_resourceAddress */
static int hf_camel_ipRoutingAddress = -1;        /* IPRoutingAddress */
static int hf_camel_none = -1;                    /* NULL */
static int hf_camel_assistingSSPIPRoutingAddress = -1;  /* AssistingSSPIPRoutingAddress */
static int hf_camel_eventSpecificInformationBCSM = -1;  /* EventSpecificInformationBCSM */
static int hf_camel_legID2 = -1;                  /* ReceivingSideID */
static int hf_camel_miscCallInfo = -1;            /* MiscCallInfo */
static int hf_camel_timerID = -1;                 /* TimerID */
static int hf_camel_timervalue = -1;              /* TimerValue */
static int hf_camel_sCIBillingChargingCharacteristics = -1;  /* SCIBillingChargingCharacteristics */
static int hf_camel_eventSpecificInformationSMS = -1;  /* EventSpecificInformationSMS */
static int hf_camel_sMSEvents = -1;               /* SMSEventArray */
static int hf_camel_SMSEventArray_item = -1;      /* SMSEvent */
static int hf_camel_timeDurationCharging = -1;    /* T_timeDurationCharging */
static int hf_camel_maxCallPeriodDuration = -1;   /* INTEGER_1_864000 */
static int hf_camel_releaseIfdurationExceeded = -1;  /* BOOLEAN */
static int hf_camel_tone1 = -1;                   /* BOOLEAN */
static int hf_camel_timeDurationChargingResult = -1;  /* T_timeDurationChargingResult */
static int hf_camel_partyToCharge1 = -1;          /* ReceivingSideID */
static int hf_camel_timeInformation = -1;         /* TimeInformation */
static int hf_camel_callActive = -1;              /* BOOLEAN */
static int hf_camel_callReleasedAtTcpExpiry = -1;  /* NULL */
static int hf_camel_bcsmEvents = -1;              /* BCSMEventArray */
static int hf_camel_BCSMEventArray_item = -1;     /* BCSMEvent */
static int hf_camel_callingPartysNumber = -1;     /* ISDN_AddressString */
static int hf_camel_destinationSubscriberNumber = -1;  /* CalledPartyBCDNumber */
static int hf_camel_sMSCAddress = -1;             /* ISDN_AddressString */
static int hf_camel_requestedInformationList = -1;  /* RequestedInformationList */
static int hf_camel_disconnectFromIPForbidden = -1;  /* BOOLEAN */
static int hf_camel_requestAnnouncementComplete = -1;  /* BOOLEAN */
static int hf_camel_collectedInfo = -1;           /* CollectedInfo */
static int hf_camel_mSISDN = -1;                  /* ISDN_AddressString */
static int hf_camel_iMSI = -1;                    /* Imsi */
static int hf_camel_gPRSMSClass = -1;             /* GPRSMSClass */
static int hf_camel_sGSNCapabilities = -1;        /* SGSNCapabilities */
static int hf_camel_gapCriteria = -1;             /* GapCriteria */
static int hf_camel_gapIndicators = -1;           /* GapIndicators */
static int hf_camel_controlType = -1;             /* ControlType */
static int hf_camel_gapTreatment = -1;            /* GapTreatment */
static int hf_camel_calledPartyNumber = -1;       /* CalledPartyNumber */
static int hf_camel_callingPartyNumber = -1;      /* CallingPartyNumber */
static int hf_camel_cGEncountered = -1;           /* CGEncountered */
static int hf_camel_highLayerCompatibility = -1;  /* HighLayerCompatibility */
static int hf_camel_additionalCallingPartyNumber = -1;  /* AdditionalCallingPartyNumber */
static int hf_camel_bearerCapability = -1;        /* BearerCapability */
static int hf_camel_cause = -1;                   /* Cause */
static int hf_camel_cug_Index = -1;               /* CUG_Index */
static int hf_camel_subscriberState = -1;         /* SubscriberState */
static int hf_camel_locationInformation = -1;     /* LocationInformation */
static int hf_camel_ext_basicServiceCode = -1;    /* Ext_BasicServiceCode */
static int hf_camel_callReferenceNumber = -1;     /* CallReferenceNumber */
static int hf_camel_mscAddress = -1;              /* ISDN_AddressString */
static int hf_camel_calledPartyBCDNumber = -1;    /* CalledPartyBCDNumber */
static int hf_camel_timeAndTimezone = -1;         /* TimeAndTimezone */
static int hf_camel_gsm_ForwardingPending = -1;   /* NULL */
static int hf_camel_initialDPArgExtension = -1;   /* InitialDPArgExtension */
static int hf_camel_callingPartyNumber1 = -1;     /* ISDN_AddressString */
static int hf_camel_locationInformationMSC = -1;  /* LocationInformation */
static int hf_camel_tPShortMessageSubmissionSpecificInfo = -1;  /* TPShortMessageSubmissionInfo */
static int hf_camel_tPProtocolIdentifier = -1;    /* TPProtocolIdentifier */
static int hf_camel_tPDataCodingScheme = -1;      /* TPDataCodingScheme */
static int hf_camel_tPValidityPeriod = -1;        /* TPValidityPeriod */
static int hf_camel_smsReferenceNumber = -1;      /* CallReferenceNumber */
static int hf_camel_sgsnNumber = -1;              /* ISDN_AddressString */
static int hf_camel_privateExtensionList = -1;    /* PrivateExtensionList */
static int hf_camel_pcs_Extensions = -1;          /* PCS_Extensions */
static int hf_camel_aOCBeforeAnswer = -1;         /* AOCBeforeAnswer */
static int hf_camel_aOCAfterAnswer = -1;          /* AOCSubsequent */
static int hf_camel_aOCGPRS = -1;                 /* AOCGprs */
static int hf_camel_chargingCharacteristics = -1;  /* ChargingCharacteristics */
static int hf_camel_chargingResult = -1;          /* ChargingResult */
static int hf_camel_active = -1;                  /* BOOLEAN */
static int hf_camel_chargingRollOver = -1;        /* ChargingRollOver */

/*--- End of included file: packet-camel-hf.c ---*/

static guint global_tcap_itu_ssn = 0;

/* Initialize the subtree pointers */
static gint ett_camel = -1;
static gint ett_camel_InvokeId = -1;
static gint ett_camel_InvokePDU = -1;
static gint ett_camel_ReturnResultPDU = -1;
static gint ett_camel_ReturnResult_result = -1;
static gint ett_camel_camelPDU = -1;
static gint ett_camelisup_parameter = -1;

/*--- Included file: packet-camel-ett.c ---*/

static gint ett_camel_PBSGSNCapabilities = -1;
static gint ett_camel_PBIPSSPCapabilities = -1;
static gint ett_camel_PBAddressString = -1;
static gint ett_camel_PBISDNAddressString = -1;
static gint ett_camel_PBGeographicalInformation = -1;
static gint ett_camel_PBGSNAddress = -1;
static gint ett_camel_PBRedirectionInformation = -1;
static gint ett_camel_PBCalledPartyNumber = -1;
static gint ett_camel_PBCallingPartyNumber = -1;
static gint ett_camel_PBRedirectingNumber = -1;
static gint ett_camel_PBCause = -1;
static gint ett_camel_PBGenericNumber = -1;
static gint ett_camel_PBLocationNumber = -1;
static gint ett_camel_PBCalledPartyBCDNumber = -1;
static gint ett_camel_CAPGPRSReferenceNumber = -1;
static gint ett_camel_ChargingRollOver = -1;
static gint ett_camel_BackwardServiceInteractionInd = -1;
static gint ett_camel_BearerCapability = -1;
static gint ett_camel_CAI_Gsm0224 = -1;
static gint ett_camel_ChargingCharacteristics = -1;
static gint ett_camel_ElapsedTime = -1;
static gint ett_camel_T_timeGPRSIfTariffSwitch = -1;
static gint ett_camel_ElapsedTimeRollOver = -1;
static gint ett_camel_T_rOTimeGPRSIfTariffSwitch = -1;
static gint ett_camel_ForwardServiceInteractionInd = -1;
static gint ett_camel_PDPType = -1;
static gint ett_camel_TimeIfTariffSwitch = -1;
static gint ett_camel_TransferredVolume = -1;
static gint ett_camel_T_volumeIfTariffSwitch = -1;
static gint ett_camel_TransferredVolumeRollOver = -1;
static gint ett_camel_T_rOVolumeIfTariffSwitch = -1;
static gint ett_camel_Code = -1;
static gint ett_camel_PCS_Extensions = -1;
static gint ett_camel_MiscCallInfo = -1;
static gint ett_camel_SupportedExtensionsExtensionType = -1;
static gint ett_camel_PrivateExtension = -1;
static gint ett_camel_AOCSubsequent = -1;
static gint ett_camel_CamelFCIGPRSBillingChargingCharacteristics = -1;
static gint ett_camel_T_fCIBCCCAMELsequence1 = -1;
static gint ett_camel_CamelFCISMSBillingChargingCharacteristics = -1;
static gint ett_camel_T_fCIBCCCAMELsequence11 = -1;
static gint ett_camel_ChargingResult = -1;
static gint ett_camel_CollectedDigits = -1;
static gint ett_camel_CollectedInfo = -1;
static gint ett_camel_DestinationRoutingAddress = -1;
static gint ett_camel_DpSpecificCriteria = -1;
static gint ett_camel_EventSpecificInformationBCSM = -1;
static gint ett_camel_T_routeSelectFailureSpecificInfo = -1;
static gint ett_camel_T_oCalledPartyBusySpecificInfo = -1;
static gint ett_camel_T_oNoAnswerSpecificInfo = -1;
static gint ett_camel_T_oAnswerSpecificInfo = -1;
static gint ett_camel_T_oDisconnectSpecificInfo = -1;
static gint ett_camel_T_tBusySpecificInfo = -1;
static gint ett_camel_T_tNoAnswerSpecificInfo = -1;
static gint ett_camel_T_tAnswerSpecificInfo = -1;
static gint ett_camel_T_tDisconnectSpecificInfo = -1;
static gint ett_camel_EventSpecificInformationSMS = -1;
static gint ett_camel_T_o_smsFailureSpecificInfo = -1;
static gint ett_camel_T_o_smsSubmittedSpecificInfo = -1;
static gint ett_camel_GapIndicators = -1;
static gint ett_camel_GapOnService = -1;
static gint ett_camel_GenericNumbers = -1;
static gint ett_camel_GPRS_QoS = -1;
static gint ett_camel_GPRSEvent = -1;
static gint ett_camel_GPRSMSClass = -1;
static gint ett_camel_QualityOfService = -1;
static gint ett_camel_ReceivingSideID = -1;
static gint ett_camel_RequestedInformationTypeList = -1;
static gint ett_camel_RequestedInformationValue = -1;
static gint ett_camel_SendingSideID = -1;
static gint ett_camel_ServiceInteractionIndicatorsTwo = -1;
static gint ett_camel_SMSEvent = -1;
static gint ett_camel_TimeInformation = -1;
static gint ett_camel_Tone = -1;
static gint ett_camel_VariablePart = -1;
static gint ett_camel_CancelArg = -1;
static gint ett_camel_ReceivedInformationArg = -1;
static gint ett_camel_ConnectGPRSArg = -1;
static gint ett_camel_EntityReleasedGPRSArg = -1;
static gint ett_camel_ReleaseGPRSArg = -1;
static gint ett_camel_RequestReportGPRSEventArg = -1;
static gint ett_camel_GPRSEventArray = -1;
static gint ett_camel_SendChargingInformationGPRSArg = -1;
static gint ett_camel_SubscriberState = -1;
static gint ett_camel_PrivateExtensionList = -1;
static gint ett_camel_CellIdOrLAI = -1;
static gint ett_camel_Ext_BasicServiceCode = -1;
static gint ett_camel_LegID = -1;
static gint ett_camel_AOCBeforeAnswer = -1;
static gint ett_camel_AOCGprs = -1;
static gint ett_camel_BasicGapCriteria = -1;
static gint ett_camel_T_calledAddressAndService = -1;
static gint ett_camel_T_callingAddressAndService = -1;
static gint ett_camel_BCSMEvent = -1;
static gint ett_camel_CamelFCIBillingChargingCharacteristics = -1;
static gint ett_camel_T_fCIBCCCAMELsequence12 = -1;
static gint ett_camel_CompoundCriteria = -1;
static gint ett_camel_GapCriteria = -1;
static gint ett_camel_MessageID = -1;
static gint ett_camel_T_text = -1;
static gint ett_camel_SEQUENCE_SIZE_1_16_OF_Integer4 = -1;
static gint ett_camel_T_variableMessage = -1;
static gint ett_camel_VariablePartsArray = -1;
static gint ett_camel_RequestedInformation = -1;
static gint ett_camel_InitialDPArgExtension = -1;
static gint ett_camel_InbandInfo = -1;
static gint ett_camel_InformationToSend = -1;
static gint ett_camel_LocationInformationGPRS = -1;
static gint ett_camel_RequestedInformationList = -1;
static gint ett_camel_LocationInformation = -1;
static gint ett_camel_GapTreatment = -1;
static gint ett_camel_GPRSEventSpecificInformation = -1;
static gint ett_camel_T_attachChangeOfPositionSpecificInformation = -1;
static gint ett_camel_T_pdp_ContextchangeOfPositionSpecificInformation = -1;
static gint ett_camel_T_detachSpecificInformation = -1;
static gint ett_camel_T_disconnectSpecificInformation = -1;
static gint ett_camel_T_pDPContextEstablishmentSpecificInformation = -1;
static gint ett_camel_T_pDPContextEstablishmentAcknowledgementSpecificInformation = -1;
static gint ett_camel_EventReportGPRSArg = -1;
static gint ett_camel_ExtensionField = -1;
static gint ett_camel_ApplyChargingArg = -1;
static gint ett_camel_ExtensionsArray = -1;
static gint ett_camel_AssistRequestInstructionsArg = -1;
static gint ett_camel_CallInformationRequestArg = -1;
static gint ett_camel_ConnectArg = -1;
static gint ett_camel_ConnectToResourceArg = -1;
static gint ett_camel_T_resourceAddress = -1;
static gint ett_camel_ContinueWithArgumentArg = -1;
static gint ett_camel_EstablishTemporaryConnectionArg = -1;
static gint ett_camel_EventReportBCSMArg = -1;
static gint ett_camel_ResetTimerArg = -1;
static gint ett_camel_SendChargingInformationArg = -1;
static gint ett_camel_EventReportSMSArg = -1;
static gint ett_camel_RequestReportSMSEventArg = -1;
static gint ett_camel_SMSEventArray = -1;
static gint ett_camel_ResetTimerSMSArg = -1;
static gint ett_camel_CamelAChBillingChargingCharacteristics = -1;
static gint ett_camel_T_timeDurationCharging = -1;
static gint ett_camel_CamelCallResult = -1;
static gint ett_camel_T_timeDurationChargingResult = -1;
static gint ett_camel_RequestReportBCSMEventArg = -1;
static gint ett_camel_BCSMEventArray = -1;
static gint ett_camel_ConnectSMSArg = -1;
static gint ett_camel_CallInformationReportArg = -1;
static gint ett_camel_PlayAnnouncementArg = -1;
static gint ett_camel_PromptAndCollectUserInformationArg = -1;
static gint ett_camel_InitialDPGPRSArg = -1;
static gint ett_camel_CallGapArg = -1;
static gint ett_camel_InitialDPArg = -1;
static gint ett_camel_InitialDPSMSArg = -1;
static gint ett_camel_ExtensionContainer = -1;
static gint ett_camel_CamelSCIBillingChargingCharacteristics = -1;
static gint ett_camel_CamelSCIGPRSBillingChargingCharacteristics = -1;
static gint ett_camel_ApplyChargingGPRSArg = -1;
static gint ett_camel_ApplyChargingReportGPRSArg = -1;
static gint ett_camel_CancelGPRSArg = -1;
static gint ett_camel_ContinueGPRSArg = -1;
static gint ett_camel_ResetTimerGPRSArg = -1;

/*--- End of included file: packet-camel-ett.c ---*/


static int hf_camel_nature_of_number = -1;
static int hf_camel_number_plan = -1;
static int hf_camel_cause_indicator = -1;

/* Preference settings default */
#define MAX_SSN 254
static range_t *global_ssn_range;
static range_t *ssn_range;
dissector_handle_t  camel_handle;

/* Global variables */

static int application_context_version;

static int  dissect_invokeCmd(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset);


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


/*--- Included file: packet-camel-fn.c ---*/

/*--- Fields for imported types ---*/




static int
dissect_camel_INTEGER(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_reserved(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER(FALSE, tvb, offset, pinfo, tree, hf_camel_reserved);
}
static int dissect_aoc(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER(FALSE, tvb, offset, pinfo, tree, hf_camel_aoc);
}
static int dissect_standardPartEnd(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER(FALSE, tvb, offset, pinfo, tree, hf_camel_standardPartEnd);
}
static int dissect_genOfVoiceAnn(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER(FALSE, tvb, offset, pinfo, tree, hf_camel_genOfVoiceAnn);
}
static int dissect_voiceInfo2(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER(FALSE, tvb, offset, pinfo, tree, hf_camel_voiceInfo2);
}
static int dissect_voiceInfo1(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER(FALSE, tvb, offset, pinfo, tree, hf_camel_voiceInfo1);
}
static int dissect_voiceBack(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER(FALSE, tvb, offset, pinfo, tree, hf_camel_voiceBack);
}
static int dissect_iPRoutAdd(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER(FALSE, tvb, offset, pinfo, tree, hf_camel_iPRoutAdd);
}
static int dissect_natureOfAddressIndicator(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER(FALSE, tvb, offset, pinfo, tree, hf_camel_natureOfAddressIndicator);
}
static int dissect_numberingPlanInd(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER(FALSE, tvb, offset, pinfo, tree, hf_camel_numberingPlanInd);
}
static int dissect_typeOfShape(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER(FALSE, tvb, offset, pinfo, tree, hf_camel_typeOfShape);
}
static int dissect_spare(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER(FALSE, tvb, offset, pinfo, tree, hf_camel_spare);
}
static int dissect_typeOfAddress(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER(FALSE, tvb, offset, pinfo, tree, hf_camel_typeOfAddress);
}
static int dissect_originalReasons(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER(FALSE, tvb, offset, pinfo, tree, hf_camel_originalReasons);
}
static int dissect_indicator(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER(FALSE, tvb, offset, pinfo, tree, hf_camel_indicator);
}
static int dissect_reason(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER(FALSE, tvb, offset, pinfo, tree, hf_camel_reason);
}
static int dissect_counter(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER(FALSE, tvb, offset, pinfo, tree, hf_camel_counter);
}
static int dissect_oddEven(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER(FALSE, tvb, offset, pinfo, tree, hf_camel_oddEven);
}
static int dissect_innInd(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER(FALSE, tvb, offset, pinfo, tree, hf_camel_innInd);
}
static int dissect_niInd(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER(FALSE, tvb, offset, pinfo, tree, hf_camel_niInd);
}
static int dissect_presentInd(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER(FALSE, tvb, offset, pinfo, tree, hf_camel_presentInd);
}
static int dissect_screening(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER(FALSE, tvb, offset, pinfo, tree, hf_camel_screening);
}
static int dissect_codingStandard(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER(FALSE, tvb, offset, pinfo, tree, hf_camel_codingStandard);
}
static int dissect_location(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER(FALSE, tvb, offset, pinfo, tree, hf_camel_location);
}
static int dissect_causeValue(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER(FALSE, tvb, offset, pinfo, tree, hf_camel_causeValue);
}
static int dissect_numberQualifierIndicator(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER(FALSE, tvb, offset, pinfo, tree, hf_camel_numberQualifierIndicator);
}
static int dissect_ext(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER(FALSE, tvb, offset, pinfo, tree, hf_camel_ext);
}
static int dissect_local(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER(FALSE, tvb, offset, pinfo, tree, hf_camel_local);
}

static const ber_sequence_t PBSGSNCapabilities_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_reserved },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_aoc },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_PBSGSNCapabilities(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                PBSGSNCapabilities_sequence, hf_index, ett_camel_PBSGSNCapabilities);

  return offset;
}


static int
dissect_camel_OCTET_STRING_SIZE_0_3(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_bilateralPart(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_OCTET_STRING_SIZE_0_3(FALSE, tvb, offset, pinfo, tree, hf_camel_bilateralPart);
}

static const ber_sequence_t PBIPSSPCapabilities_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_standardPartEnd },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_reserved },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_genOfVoiceAnn },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_voiceInfo2 },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_voiceInfo1 },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_voiceBack },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_iPRoutAdd },
  { BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_bilateralPart },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_PBIPSSPCapabilities(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                PBIPSSPCapabilities_sequence, hf_index, ett_camel_PBIPSSPCapabilities);

  return offset;
}



static int
dissect_camel_INTEGER_1(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_extension(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER_1(FALSE, tvb, offset, pinfo, tree, hf_camel_extension);
}
static int dissect_o1ext(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER_1(FALSE, tvb, offset, pinfo, tree, hf_camel_o1ext);
}
static int dissect_o2ext(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER_1(FALSE, tvb, offset, pinfo, tree, hf_camel_o2ext);
}


static int
dissect_camel_OCTET_STRING_SIZE_0_19(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_digits(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_OCTET_STRING_SIZE_0_19(FALSE, tvb, offset, pinfo, tree, hf_camel_digits);
}

static const ber_sequence_t PBAddressString_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_extension },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_natureOfAddressIndicator },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_numberingPlanInd },
  { BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_digits },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_PBAddressString(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                PBAddressString_sequence, hf_index, ett_camel_PBAddressString);

  return offset;
}


static int
dissect_camel_OCTET_STRING_SIZE_0_8(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_digits1(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_OCTET_STRING_SIZE_0_8(FALSE, tvb, offset, pinfo, tree, hf_camel_digits1);
}

static const ber_sequence_t PBISDNAddressString_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_extension },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_natureOfAddressIndicator },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_numberingPlanInd },
  { BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_digits1 },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_PBISDNAddressString(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                PBISDNAddressString_sequence, hf_index, ett_camel_PBISDNAddressString);

  return offset;
}


static int
dissect_camel_OCTET_STRING_SIZE_3(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_degreesOfLatitude(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_OCTET_STRING_SIZE_3(FALSE, tvb, offset, pinfo, tree, hf_camel_degreesOfLatitude);
}
static int dissect_degreesOfLongitude(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_OCTET_STRING_SIZE_3(FALSE, tvb, offset, pinfo, tree, hf_camel_degreesOfLongitude);
}


static int
dissect_camel_OCTET_STRING_SIZE_1(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_uncertaintyCode(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_OCTET_STRING_SIZE_1(FALSE, tvb, offset, pinfo, tree, hf_camel_uncertaintyCode);
}
static int dissect_conferenceTreatmentIndicator_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_OCTET_STRING_SIZE_1(TRUE, tvb, offset, pinfo, tree, hf_camel_conferenceTreatmentIndicator);
}
static int dissect_callCompletionTreatmentIndicator_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_OCTET_STRING_SIZE_1(TRUE, tvb, offset, pinfo, tree, hf_camel_callCompletionTreatmentIndicator);
}
static int dissect_callDiversionTreatmentIndicator_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_OCTET_STRING_SIZE_1(TRUE, tvb, offset, pinfo, tree, hf_camel_callDiversionTreatmentIndicator);
}
static int dissect_callingPartyRestrictionIndicator_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_OCTET_STRING_SIZE_1(TRUE, tvb, offset, pinfo, tree, hf_camel_callingPartyRestrictionIndicator);
}
static int dissect_pDPTypeOrganization_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_OCTET_STRING_SIZE_1(TRUE, tvb, offset, pinfo, tree, hf_camel_pDPTypeOrganization);
}
static int dissect_pDPTypeNumber_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_OCTET_STRING_SIZE_1(TRUE, tvb, offset, pinfo, tree, hf_camel_pDPTypeNumber);
}
static int dissect_holdTreatmentIndicator_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_OCTET_STRING_SIZE_1(TRUE, tvb, offset, pinfo, tree, hf_camel_holdTreatmentIndicator);
}
static int dissect_cwTreatmentIndicator_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_OCTET_STRING_SIZE_1(TRUE, tvb, offset, pinfo, tree, hf_camel_cwTreatmentIndicator);
}
static int dissect_ectTreatmentIndicator_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_OCTET_STRING_SIZE_1(TRUE, tvb, offset, pinfo, tree, hf_camel_ectTreatmentIndicator);
}

static const ber_sequence_t PBGeographicalInformation_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_typeOfShape },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_spare },
  { BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_degreesOfLatitude },
  { BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_degreesOfLongitude },
  { BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_uncertaintyCode },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_PBGeographicalInformation(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                PBGeographicalInformation_sequence, hf_index, ett_camel_PBGeographicalInformation);

  return offset;
}



static int
dissect_camel_INTEGER_4_16(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_addressLength(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER_4_16(FALSE, tvb, offset, pinfo, tree, hf_camel_addressLength);
}


static int
dissect_camel_OCTET_STRING_SIZE_4_16(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_address(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_OCTET_STRING_SIZE_4_16(FALSE, tvb, offset, pinfo, tree, hf_camel_address);
}

static const ber_sequence_t PBGSNAddress_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_typeOfAddress },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_addressLength },
  { BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_address },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_PBGSNAddress(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                PBGSNAddress_sequence, hf_index, ett_camel_PBGSNAddress);

  return offset;
}



static int
dissect_camel_INTEGER_0(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_spare1(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER_0(FALSE, tvb, offset, pinfo, tree, hf_camel_spare1);
}
static int dissect_spare2(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER_0(FALSE, tvb, offset, pinfo, tree, hf_camel_spare2);
}
static int dissect_foo(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER_0(FALSE, tvb, offset, pinfo, tree, hf_camel_foo);
}

static const ber_sequence_t PBRedirectionInformation_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_originalReasons },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_spare1 },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_indicator },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_reason },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_spare2 },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_counter },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_PBRedirectionInformation(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                PBRedirectionInformation_sequence, hf_index, ett_camel_PBRedirectionInformation);

  return offset;
}


static int
dissect_camel_OCTET_STRING_SIZE_0_16(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_digits2(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_OCTET_STRING_SIZE_0_16(FALSE, tvb, offset, pinfo, tree, hf_camel_digits2);
}

static const ber_sequence_t PBCalledPartyNumber_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_oddEven },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_natureOfAddressIndicator },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_innInd },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_numberingPlanInd },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_spare1 },
  { BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_digits2 },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_PBCalledPartyNumber(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                PBCalledPartyNumber_sequence, hf_index, ett_camel_PBCalledPartyNumber);

  return offset;
}

static const ber_sequence_t PBCallingPartyNumber_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_oddEven },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_natureOfAddressIndicator },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_niInd },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_numberingPlanInd },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_presentInd },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_screening },
  { BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_digits1 },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_PBCallingPartyNumber(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                PBCallingPartyNumber_sequence, hf_index, ett_camel_PBCallingPartyNumber);

  return offset;
}


static int
dissect_camel_OCTET_STRING_SIZE_1_10(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_digits3(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_OCTET_STRING_SIZE_1_10(FALSE, tvb, offset, pinfo, tree, hf_camel_digits3);
}

static const ber_sequence_t PBRedirectingNumber_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_oddEven },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_natureOfAddressIndicator },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_innInd },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_numberingPlanInd },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_spare1 },
  { BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_digits3 },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_PBRedirectingNumber(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                PBRedirectingNumber_sequence, hf_index, ett_camel_PBRedirectingNumber);

  return offset;
}


static int
dissect_camel_OCTET_STRING_SIZE_0_30(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_diagnostics(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_OCTET_STRING_SIZE_0_30(FALSE, tvb, offset, pinfo, tree, hf_camel_diagnostics);
}

static const ber_sequence_t PBCause_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_o1ext },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_codingStandard },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_spare1 },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_location },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_o2ext },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_causeValue },
  { BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_diagnostics },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_PBCause(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                PBCause_sequence, hf_index, ett_camel_PBCause);

  return offset;
}

static const ber_sequence_t PBGenericNumber_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_numberQualifierIndicator },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_oddEven },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_natureOfAddressIndicator },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_niInd },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_numberingPlanInd },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_presentInd },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_screening },
  { BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_digits1 },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_PBGenericNumber(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                PBGenericNumber_sequence, hf_index, ett_camel_PBGenericNumber);

  return offset;
}

static const ber_sequence_t PBLocationNumber_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_oddEven },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_natureOfAddressIndicator },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_innInd },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_numberingPlanInd },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_presentInd },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_screening },
  { BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_digits1 },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_PBLocationNumber(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                PBLocationNumber_sequence, hf_index, ett_camel_PBLocationNumber);

  return offset;
}


static const value_string camel_T_typeOfNumber_vals[] = {
  {   0, "unknown" },
  {   1, "international" },
  {   2, "national" },
  {   3, "networkSpecific" },
  {   4, "dedicatedAccess" },
  {   5, "reserved5" },
  {   6, "reserved6" },
  {   7, "reservedExt" },
  { 0, NULL }
};


static int
dissect_camel_T_typeOfNumber(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_typeOfNumber(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_T_typeOfNumber(FALSE, tvb, offset, pinfo, tree, hf_camel_typeOfNumber);
}


static int
dissect_camel_OCTET_STRING_SIZE_0_40(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_digits4(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_OCTET_STRING_SIZE_0_40(FALSE, tvb, offset, pinfo, tree, hf_camel_digits4);
}

static const ber_sequence_t PBCalledPartyBCDNumber_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_ext },
  { BER_CLASS_UNI, BER_UNI_TAG_ENUMERATED, BER_FLAGS_NOOWNTAG, dissect_typeOfNumber },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_numberingPlanInd },
  { BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_digits4 },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_PBCalledPartyBCDNumber(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                PBCalledPartyBCDNumber_sequence, hf_index, ett_camel_PBCalledPartyBCDNumber);

  return offset;
}


static int
dissect_camel_AccessPointName(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_accessPointName_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_AccessPointName(TRUE, tvb, offset, pinfo, tree, hf_camel_accessPointName);
}


static int
dissect_camel_AChBillingChargingCharacteristics(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_aChBillingChargingCharacteristics_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_AChBillingChargingCharacteristics(TRUE, tvb, offset, pinfo, tree, hf_camel_aChBillingChargingCharacteristics);
}


static int
dissect_camel_AlertingPattern(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_alertingPattern_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_AlertingPattern(TRUE, tvb, offset, pinfo, tree, hf_camel_alertingPattern);
}


static const value_string camel_AppendFreeFormatData_vals[] = {
  {   0, "overwrite" },
  {   1, "append" },
  { 0, NULL }
};


static int
dissect_camel_AppendFreeFormatData(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_appendFreeFormatData_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_AppendFreeFormatData(TRUE, tvb, offset, pinfo, tree, hf_camel_appendFreeFormatData);
}



static int
dissect_camel_ApplicationTimer(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_applicationTimer_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_ApplicationTimer(TRUE, tvb, offset, pinfo, tree, hf_camel_applicationTimer);
}


static int
dissect_camel_CalledPartyBCDNumber(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_destinationSubscriberNumber_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_CalledPartyBCDNumber(TRUE, tvb, offset, pinfo, tree, hf_camel_destinationSubscriberNumber);
}
static int dissect_calledPartyBCDNumber_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_CalledPartyBCDNumber(TRUE, tvb, offset, pinfo, tree, hf_camel_calledPartyBCDNumber);
}


static int
dissect_camel_CalledPartyNumber(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_DestinationRoutingAddress_item(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_CalledPartyNumber(FALSE, tvb, offset, pinfo, tree, hf_camel_DestinationRoutingAddress_item);
}
static int dissect_destinationAddress_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_CalledPartyNumber(TRUE, tvb, offset, pinfo, tree, hf_camel_destinationAddress);
}
static int dissect_calledPartyNumber_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_CalledPartyNumber(TRUE, tvb, offset, pinfo, tree, hf_camel_calledPartyNumber);
}


static int
dissect_camel_CallingPartyNumber(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_callingPartyNumber_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_CallingPartyNumber(TRUE, tvb, offset, pinfo, tree, hf_camel_callingPartyNumber);
}


static int
dissect_camel_CallResult(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}



static int
dissect_camel_Integer4(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_destinationReference(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_Integer4(FALSE, tvb, offset, pinfo, tree, hf_camel_destinationReference);
}
static int dissect_originationReference(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_Integer4(FALSE, tvb, offset, pinfo, tree, hf_camel_originationReference);
}
static int dissect_callConnectedElapsedTimeValue_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_Integer4(TRUE, tvb, offset, pinfo, tree, hf_camel_callConnectedElapsedTimeValue);
}
static int dissect_toneID_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_Integer4(TRUE, tvb, offset, pinfo, tree, hf_camel_toneID);
}
static int dissect_duration1_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_Integer4(TRUE, tvb, offset, pinfo, tree, hf_camel_duration1);
}
static int dissect_integer_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_Integer4(TRUE, tvb, offset, pinfo, tree, hf_camel_integer);
}
static int dissect_elementaryMessageID_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_Integer4(TRUE, tvb, offset, pinfo, tree, hf_camel_elementaryMessageID);
}
static int dissect_elementaryMessageIDs_item(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_Integer4(FALSE, tvb, offset, pinfo, tree, hf_camel_elementaryMessageIDs_item);
}

static const ber_sequence_t CAPGPRSReferenceNumber_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL, dissect_destinationReference },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL, dissect_originationReference },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_CAPGPRSReferenceNumber(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                CAPGPRSReferenceNumber_sequence, hf_index, ett_camel_CAPGPRSReferenceNumber);

  return offset;
}


static int
dissect_camel_Carrier(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_carrier_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_Carrier(TRUE, tvb, offset, pinfo, tree, hf_camel_carrier);
}


static int
dissect_camel_Cause(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_failureCause_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_Cause(TRUE, tvb, offset, pinfo, tree, hf_camel_failureCause);
}
static int dissect_busyCause_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_Cause(TRUE, tvb, offset, pinfo, tree, hf_camel_busyCause);
}
static int dissect_releaseCause_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_Cause(TRUE, tvb, offset, pinfo, tree, hf_camel_releaseCause);
}
static int dissect_releaseCauseValue_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_Cause(TRUE, tvb, offset, pinfo, tree, hf_camel_releaseCauseValue);
}
static int dissect_cause_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_Cause(TRUE, tvb, offset, pinfo, tree, hf_camel_cause);
}


static const value_string camel_CGEncountered_vals[] = {
  {   0, "noCGencountered" },
  {   1, "manualCGencountered" },
  {   2, "scpOverload" },
  { 0, NULL }
};


static int
dissect_camel_CGEncountered(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_cGEncountered_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_CGEncountered(TRUE, tvb, offset, pinfo, tree, hf_camel_cGEncountered);
}


static const value_string camel_ConnectedNumberTreatmentInd_vals[] = {
  {   0, "noINImpact" },
  {   1, "presentationRestricted" },
  {   2, "presentCalledINNumber" },
  {   3, "presentCallINNumberRestricted" },
  { 0, NULL }
};


static int
dissect_camel_ConnectedNumberTreatmentInd(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_connectedNumberTreatmentInd_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_ConnectedNumberTreatmentInd(TRUE, tvb, offset, pinfo, tree, hf_camel_connectedNumberTreatmentInd);
}


static const value_string camel_ControlType_vals[] = {
  {   0, "sCPOverloaded" },
  {   1, "manuallyInitiated" },
  { 0, NULL }
};


static int
dissect_camel_ControlType(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_controlType_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_ControlType(TRUE, tvb, offset, pinfo, tree, hf_camel_controlType);
}



static int
dissect_camel_INTEGER_0_255(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_rOTimeGPRSIfNoTariffSwitch_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER_0_255(TRUE, tvb, offset, pinfo, tree, hf_camel_rOTimeGPRSIfNoTariffSwitch);
}
static int dissect_rOTimeGPRSSinceLastTariffSwitch_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER_0_255(TRUE, tvb, offset, pinfo, tree, hf_camel_rOTimeGPRSSinceLastTariffSwitch);
}
static int dissect_rOTimeGPRSTariffSwitchInterval_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER_0_255(TRUE, tvb, offset, pinfo, tree, hf_camel_rOTimeGPRSTariffSwitchInterval);
}
static int dissect_rOVolumeIfNoTariffSwitch_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER_0_255(TRUE, tvb, offset, pinfo, tree, hf_camel_rOVolumeIfNoTariffSwitch);
}
static int dissect_rOVolumeSinceLastTariffSwitch_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER_0_255(TRUE, tvb, offset, pinfo, tree, hf_camel_rOVolumeSinceLastTariffSwitch);
}
static int dissect_rOVolumeTariffSwitchInterval_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER_0_255(TRUE, tvb, offset, pinfo, tree, hf_camel_rOVolumeTariffSwitchInterval);
}
static int dissect_callAttemptElapsedTimeValue_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER_0_255(TRUE, tvb, offset, pinfo, tree, hf_camel_callAttemptElapsedTimeValue);
}

static const ber_sequence_t T_rOVolumeIfTariffSwitch_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_rOVolumeSinceLastTariffSwitch_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_rOVolumeTariffSwitchInterval_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_T_rOVolumeIfTariffSwitch(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                T_rOVolumeIfTariffSwitch_sequence, hf_index, ett_camel_T_rOVolumeIfTariffSwitch);

  return offset;
}
static int dissect_rOVolumeIfTariffSwitch_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_T_rOVolumeIfTariffSwitch(TRUE, tvb, offset, pinfo, tree, hf_camel_rOVolumeIfTariffSwitch);
}


static const value_string camel_TransferredVolumeRollOver_vals[] = {
  {   0, "rOVolumeIfNoTariffSwitch" },
  {   1, "rOVolumeIfTariffSwitch" },
  { 0, NULL }
};

static const ber_choice_t TransferredVolumeRollOver_choice[] = {
  {   0, BER_CLASS_CON, 0, 0, dissect_rOVolumeIfNoTariffSwitch_impl },
  {   1, BER_CLASS_CON, 1, 0, dissect_rOVolumeIfTariffSwitch_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_camel_TransferredVolumeRollOver(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              TransferredVolumeRollOver_choice, hf_index, ett_camel_TransferredVolumeRollOver);

  return offset;
}
static int dissect_transferredVolumeRollOver_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_TransferredVolumeRollOver(TRUE, tvb, offset, pinfo, tree, hf_camel_transferredVolumeRollOver);
}

static const ber_sequence_t T_rOTimeGPRSIfTariffSwitch_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_rOTimeGPRSSinceLastTariffSwitch_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_rOTimeGPRSTariffSwitchInterval_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_T_rOTimeGPRSIfTariffSwitch(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                T_rOTimeGPRSIfTariffSwitch_sequence, hf_index, ett_camel_T_rOTimeGPRSIfTariffSwitch);

  return offset;
}
static int dissect_rOTimeGPRSIfTariffSwitch_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_T_rOTimeGPRSIfTariffSwitch(TRUE, tvb, offset, pinfo, tree, hf_camel_rOTimeGPRSIfTariffSwitch);
}


static const value_string camel_ElapsedTimeRollOver_vals[] = {
  {   0, "rOTimeGPRSIfNoTariffSwitch" },
  {   1, "rOTimeGPRSIfTariffSwitch" },
  { 0, NULL }
};

static const ber_choice_t ElapsedTimeRollOver_choice[] = {
  {   0, BER_CLASS_CON, 0, 0, dissect_rOTimeGPRSIfNoTariffSwitch_impl },
  {   1, BER_CLASS_CON, 1, 0, dissect_rOTimeGPRSIfTariffSwitch_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_camel_ElapsedTimeRollOver(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              ElapsedTimeRollOver_choice, hf_index, ett_camel_ElapsedTimeRollOver);

  return offset;
}
static int dissect_elapsedTimeRollOver_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_ElapsedTimeRollOver(TRUE, tvb, offset, pinfo, tree, hf_camel_elapsedTimeRollOver);
}


static const value_string camel_ChargingRollOver_vals[] = {
  {   0, "transferredVolumeRollOver" },
  {   1, "elapsedTimeRollOver" },
  { 0, NULL }
};

static const ber_choice_t ChargingRollOver_choice[] = {
  {   0, BER_CLASS_CON, 0, 0, dissect_transferredVolumeRollOver_impl },
  {   1, BER_CLASS_CON, 1, 0, dissect_elapsedTimeRollOver_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_camel_ChargingRollOver(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              ChargingRollOver_choice, hf_index, ett_camel_ChargingRollOver);

  return offset;
}
static int dissect_chargingRollOver_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_ChargingRollOver(TRUE, tvb, offset, pinfo, tree, hf_camel_chargingRollOver);
}


static int
dissect_camel_DateAndTime(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_callStopTimeValue_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_DateAndTime(TRUE, tvb, offset, pinfo, tree, hf_camel_callStopTimeValue);
}


static int
dissect_camel_Digits(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_number_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_Digits(TRUE, tvb, offset, pinfo, tree, hf_camel_number);
}
static int dissect_digitsResponse_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_Digits(TRUE, tvb, offset, pinfo, tree, hf_camel_digitsResponse);
}
static int dissect_calledAddressValue_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_Digits(TRUE, tvb, offset, pinfo, tree, hf_camel_calledAddressValue);
}
static int dissect_callingAddressValue_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_Digits(TRUE, tvb, offset, pinfo, tree, hf_camel_callingAddressValue);
}


static const value_string camel_ErrorTreatment_vals[] = {
  {   0, "stdErrorAndInfo" },
  {   1, "help" },
  {   2, "repeatPrompt" },
  { 0, NULL }
};


static int
dissect_camel_ErrorTreatment(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_errorTreatment_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_ErrorTreatment(TRUE, tvb, offset, pinfo, tree, hf_camel_errorTreatment);
}


static const value_string camel_EventTypeBCSM_vals[] = {
  {   2, "collectedInfo" },
  {   3, "analyzedInformation" },
  {   4, "routeSelectFailure" },
  {   5, "oCalledPartyBusy" },
  {   6, "oNoAnswer" },
  {   7, "oAnswer" },
  {   9, "oDisconnect" },
  {  10, "oAbandon" },
  {  12, "termAttemptAuthorized" },
  {  13, "tBusy" },
  {  14, "tNoAnswer" },
  {  15, "tAnswer" },
  {  17, "tDisconnect" },
  {  18, "tAbandon" },
  { 0, NULL }
};


static int
dissect_camel_EventTypeBCSM(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_eventTypeBCSM_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_EventTypeBCSM(TRUE, tvb, offset, pinfo, tree, hf_camel_eventTypeBCSM);
}


static const value_string camel_EventTypeSMS_vals[] = {
  {   1, "sms-CollectedInfo" },
  {   2, "o-smsFailure" },
  {   3, "o-smsSubmitted" },
  { 0, NULL }
};


static int
dissect_camel_EventTypeSMS(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_eventTypeSMS_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_EventTypeSMS(TRUE, tvb, offset, pinfo, tree, hf_camel_eventTypeSMS);
}


static const value_string camel_CriticalityType_vals[] = {
  {   0, "ignore" },
  {   1, "abort" },
  { 0, NULL }
};


static int
dissect_camel_CriticalityType(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_criticality(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_CriticalityType(FALSE, tvb, offset, pinfo, tree, hf_camel_criticality);
}


static int
dissect_camel_FCIBillingChargingCharacteristics(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}


static int
dissect_camel_FCIGPRSBillingChargingCharacteristics(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}


static int
dissect_camel_FCISMSBillingChargingCharacteristics(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}


static int
dissect_camel_GenericNumber(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_GenericNumbers_item(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_GenericNumber(FALSE, tvb, offset, pinfo, tree, hf_camel_GenericNumbers_item);
}


static int
dissect_camel_GPRSCause(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_gPRSCause_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_GPRSCause(TRUE, tvb, offset, pinfo, tree, hf_camel_gPRSCause);
}
static int dissect_gprsCause_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_GPRSCause(TRUE, tvb, offset, pinfo, tree, hf_camel_gprsCause);
}


static int
dissect_camel_GPRSChargingID(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_chargingID_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_GPRSChargingID(TRUE, tvb, offset, pinfo, tree, hf_camel_chargingID);
}


static const value_string camel_PDPInitiationType_vals[] = {
  {   0, "mSInitiated" },
  {   1, "networkInitiated" },
  { 0, NULL }
};


static int
dissect_camel_PDPInitiationType(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_pDPInitiationType_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_PDPInitiationType(TRUE, tvb, offset, pinfo, tree, hf_camel_pDPInitiationType);
}


static const value_string camel_GPRSEventType_vals[] = {
  {   1, "attach" },
  {   2, "attachChangeOfPosition" },
  {   3, "detached" },
  {  11, "pdp-ContextEstablishment" },
  {  12, "pdp-ContextEstablishmentAcknowledgement" },
  {  13, "disonnect" },
  {  14, "pdp-ContextChangeOfPosition" },
  { 0, NULL }
};


static int
dissect_camel_GPRSEventType(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_gPRSEventType_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_GPRSEventType(TRUE, tvb, offset, pinfo, tree, hf_camel_gPRSEventType);
}


static const value_string camel_InitiatingEntity_vals[] = {
  {   0, "ms" },
  {   1, "sgsn" },
  {   2, "hlr" },
  {   3, "ggsn" },
  { 0, NULL }
};


static int
dissect_camel_InitiatingEntity(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_inititatingEntity_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_InitiatingEntity(TRUE, tvb, offset, pinfo, tree, hf_camel_inititatingEntity);
}


static int
dissect_camel_IPSSPCapabilities(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_iPSSPCapabilities_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_IPSSPCapabilities(TRUE, tvb, offset, pinfo, tree, hf_camel_iPSSPCapabilities);
}


static int
dissect_camel_LegType(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_receivingSideID_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_LegType(TRUE, tvb, offset, pinfo, tree, hf_camel_receivingSideID);
}
static int dissect_sendingSideID_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_LegType(TRUE, tvb, offset, pinfo, tree, hf_camel_sendingSideID);
}


static int
dissect_camel_LocationNumber(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_locationNumber_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_LocationNumber(TRUE, tvb, offset, pinfo, tree, hf_camel_locationNumber);
}


static const value_string camel_MonitorMode_vals[] = {
  {   0, "interrupted" },
  {   1, "notifyAndContinue" },
  {   2, "transparent" },
  { 0, NULL }
};


static int
dissect_camel_MonitorMode(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_monitorMode_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_MonitorMode(TRUE, tvb, offset, pinfo, tree, hf_camel_monitorMode);
}


static int
dissect_camel_MSNetworkCapability(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_mSNetworkCapability_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_MSNetworkCapability(TRUE, tvb, offset, pinfo, tree, hf_camel_mSNetworkCapability);
}


static int
dissect_camel_MSRadioAccessCapability(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_mSRadioAccessCapability_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_MSRadioAccessCapability(TRUE, tvb, offset, pinfo, tree, hf_camel_mSRadioAccessCapability);
}


static int
dissect_camel_NAOliInfo(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_naOliInfo_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_NAOliInfo(TRUE, tvb, offset, pinfo, tree, hf_camel_naOliInfo);
}


static int
dissect_camel_OriginalCalledPartyID(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_originalCalledPartyID_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_OriginalCalledPartyID(TRUE, tvb, offset, pinfo, tree, hf_camel_originalCalledPartyID);
}


static int
dissect_camel_OCSIApplicable(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  { proto_item *ti_tmp;
  ti_tmp = proto_tree_add_item(tree, hf_index, tvb, offset>>8, 0, FALSE);
  proto_item_append_text(ti_tmp, ": NULL");
  }

  return offset;
}
static int dissect_oCSIApplicable_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_OCSIApplicable(TRUE, tvb, offset, pinfo, tree, hf_camel_oCSIApplicable);
}


static int
dissect_camel_PDPId(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_pDPID_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_PDPId(TRUE, tvb, offset, pinfo, tree, hf_camel_pDPID);
}
static int dissect_pdpID_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_PDPId(TRUE, tvb, offset, pinfo, tree, hf_camel_pdpID);
}


static int
dissect_camel_RAIdentity(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_routeingAreaIdentity_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_RAIdentity(TRUE, tvb, offset, pinfo, tree, hf_camel_routeingAreaIdentity);
}


static int
dissect_camel_RedirectingPartyID(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_redirectingPartyID_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_RedirectingPartyID(TRUE, tvb, offset, pinfo, tree, hf_camel_redirectingPartyID);
}


static const value_string camel_RequestedInformationType_vals[] = {
  {   0, "callAttemptElapsedTime" },
  {   1, "callStopTime" },
  {   2, "callConnectedElapsedTime" },
  {  30, "releaseCause" },
  { 0, NULL }
};


static int
dissect_camel_RequestedInformationType(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_RequestedInformationTypeList_item(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_RequestedInformationType(FALSE, tvb, offset, pinfo, tree, hf_camel_RequestedInformationTypeList_item);
}
static int dissect_requestedInformationType_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_RequestedInformationType(TRUE, tvb, offset, pinfo, tree, hf_camel_requestedInformationType);
}


static int
dissect_camel_RPCause(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}


static int
dissect_camel_ScfID(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_scfID_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_ScfID(TRUE, tvb, offset, pinfo, tree, hf_camel_scfID);
}


static int
dissect_camel_SCIBillingChargingCharacteristics(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_sCIBillingChargingCharacteristics_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_SCIBillingChargingCharacteristics(TRUE, tvb, offset, pinfo, tree, hf_camel_sCIBillingChargingCharacteristics);
}


static int
dissect_camel_SCIGPRSBillingChargingCharacteristics(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_sCIGPRSBillingChargingCharacteristics_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_SCIGPRSBillingChargingCharacteristics(TRUE, tvb, offset, pinfo, tree, hf_camel_sCIGPRSBillingChargingCharacteristics);
}


static int
dissect_camel_SGSNCapabilities(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_sGSNCapabilities_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_SGSNCapabilities(TRUE, tvb, offset, pinfo, tree, hf_camel_sGSNCapabilities);
}


static const value_string camel_SMSCause_vals[] = {
  {   0, "systemFailureEnum" },
  {   1, "unexpectedDataValueEnum" },
  {   2, "facilityNotSupported" },
  {   3, "sM-DeliveryFailure" },
  {   4, "releaseFromRadioInterface" },
  { 0, NULL }
};


static int
dissect_camel_SMSCause(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_failureCause1_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_SMSCause(TRUE, tvb, offset, pinfo, tree, hf_camel_failureCause1);
}



static int
dissect_camel_TimeIfNoTariffSwitch(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_timeIfNoTariffSwitch_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_TimeIfNoTariffSwitch(TRUE, tvb, offset, pinfo, tree, hf_camel_timeIfNoTariffSwitch);
}


static const value_string camel_TimerID_vals[] = {
  {   0, "tssf" },
  { 0, NULL }
};


static int
dissect_camel_TimerID(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_timerID_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_TimerID(TRUE, tvb, offset, pinfo, tree, hf_camel_timerID);
}


static int
dissect_camel_TimeAndTimezone(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_timeAndTimeZone_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_TimeAndTimezone(TRUE, tvb, offset, pinfo, tree, hf_camel_timeAndTimeZone);
}
static int dissect_timeAndTimezone_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_TimeAndTimezone(TRUE, tvb, offset, pinfo, tree, hf_camel_timeAndTimezone);
}


static int
dissect_camel_TPDataCodingScheme(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_tPDataCodingScheme_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_TPDataCodingScheme(TRUE, tvb, offset, pinfo, tree, hf_camel_tPDataCodingScheme);
}


static int
dissect_camel_TPProtocolIdentifier(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_tPProtocolIdentifier_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_TPProtocolIdentifier(TRUE, tvb, offset, pinfo, tree, hf_camel_tPProtocolIdentifier);
}


static int
dissect_camel_TPShortMessageSubmissionInfo(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_tPShortMessageSubmissionSpecificInfo_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_TPShortMessageSubmissionInfo(TRUE, tvb, offset, pinfo, tree, hf_camel_tPShortMessageSubmissionSpecificInfo);
}


static int
dissect_camel_TPValidityPeriod(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_tPValidityPeriod_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_TPValidityPeriod(TRUE, tvb, offset, pinfo, tree, hf_camel_tPValidityPeriod);
}


static const value_string camel_UnavailableNetworkResource_vals[] = {
  {   0, "unavailableResources" },
  {   1, "componentFailure" },
  {   2, "basicCallProcessingException" },
  {   3, "resourceStatusFailure" },
  {   4, "endUserFailure" },
  { 0, NULL }
};


static int
dissect_camel_UnavailableNetworkResource(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}


static int
dissect_camel_SpecializedResourceReportArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  { proto_item *ti_tmp;
  ti_tmp = proto_tree_add_item(tree, hf_index, tvb, offset>>8, 0, FALSE);
  proto_item_append_text(ti_tmp, ": NULL");
  }

  return offset;
}



static int
dissect_camel_Priority(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}


static int
dissect_camel_CUG_Interlock(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_cug_Interlock_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_CUG_Interlock(TRUE, tvb, offset, pinfo, tree, hf_camel_cug_Interlock);
}



static int
dissect_camel_CUG_Index(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_cug_Index_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_CUG_Index(TRUE, tvb, offset, pinfo, tree, hf_camel_cug_Index);
}


static int
dissect_camel_CallReferenceNumber(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_callReferenceNumber_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_CallReferenceNumber(TRUE, tvb, offset, pinfo, tree, hf_camel_callReferenceNumber);
}
static int dissect_smsReferenceNumber_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_CallReferenceNumber(TRUE, tvb, offset, pinfo, tree, hf_camel_smsReferenceNumber);
}


static int
dissect_camel_SuppressionOfAnnouncement(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  { proto_item *ti_tmp;
  ti_tmp = proto_tree_add_item(tree, hf_index, tvb, offset>>8, 0, FALSE);
  proto_item_append_text(ti_tmp, ": NULL");
  }

  return offset;
}
static int dissect_suppressionOfAnnouncement_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_SuppressionOfAnnouncement(TRUE, tvb, offset, pinfo, tree, hf_camel_suppressionOfAnnouncement);
}


static const value_string camel_NotReachableReason_vals[] = {
  {   0, "msPurged" },
  {   1, "imsiDetached" },
  {   2, "restrictedArea" },
  {   3, "notRegistred" },
  { 0, NULL }
};


static int
dissect_camel_NotReachableReason(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_netDetNotReachable(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_NotReachableReason(FALSE, tvb, offset, pinfo, tree, hf_camel_netDetNotReachable);
}



static int
dissect_camel_AgeOfLocationInformation(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_ageOfLocationInformation(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_AgeOfLocationInformation(FALSE, tvb, offset, pinfo, tree, hf_camel_ageOfLocationInformation);
}


static int
dissect_camel_GeographicalInformation(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_geographicalInformation_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_GeographicalInformation(TRUE, tvb, offset, pinfo, tree, hf_camel_geographicalInformation);
}


static int
dissect_camel_AddressString(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}


static int
dissect_camel_CellIdFixedLength(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_cellIdFixedLength_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_CellIdFixedLength(TRUE, tvb, offset, pinfo, tree, hf_camel_cellIdFixedLength);
}


static int
dissect_camel_LAIFixedLength(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_laiFixedLength_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_LAIFixedLength(TRUE, tvb, offset, pinfo, tree, hf_camel_laiFixedLength);
}



static int
dissect_camel_ServiceKey(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_serviceKey_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_ServiceKey(TRUE, tvb, offset, pinfo, tree, hf_camel_serviceKey);
}


static int
dissect_camel_Ext_TeleserviceCode(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_ext_Teleservice_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_Ext_TeleserviceCode(TRUE, tvb, offset, pinfo, tree, hf_camel_ext_Teleservice);
}


static int
dissect_camel_Ext_BearerServiceCode(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_ext_BearerService_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_Ext_BearerServiceCode(TRUE, tvb, offset, pinfo, tree, hf_camel_ext_BearerService);
}


static int
dissect_camel_Imsi(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_iMSI_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_Imsi(TRUE, tvb, offset, pinfo, tree, hf_camel_iMSI);
}



static int
dissect_camel_Duration(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_duration_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_Duration(TRUE, tvb, offset, pinfo, tree, hf_camel_duration);
}



static int
dissect_camel_Interval(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_gapInterval_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_Interval(TRUE, tvb, offset, pinfo, tree, hf_camel_gapInterval);
}


static int
dissect_camel_CallingPartysCategory(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_callingPartysCategory_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_CallingPartysCategory(TRUE, tvb, offset, pinfo, tree, hf_camel_callingPartysCategory);
}


static int
dissect_camel_RedirectionInformation(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_redirectionInformation_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_RedirectionInformation(TRUE, tvb, offset, pinfo, tree, hf_camel_redirectionInformation);
}


static int
dissect_camel_HighLayerCompatibility(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_highLayerCompatibility_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_HighLayerCompatibility(TRUE, tvb, offset, pinfo, tree, hf_camel_highLayerCompatibility);
}


static const value_string camel_BothwayThroughConnectionInd_vals[] = {
  {   0, "bothwayPathRequired" },
  {   1, "bothwayPathNotRequired" },
  { 0, NULL }
};


static int
dissect_camel_BothwayThroughConnectionInd(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_bothwayThroughConnectionInd_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_BothwayThroughConnectionInd(TRUE, tvb, offset, pinfo, tree, hf_camel_bothwayThroughConnectionInd);
}


static int
dissect_camel_LSAIdentity(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_selectedLSAIdentity_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_LSAIdentity(TRUE, tvb, offset, pinfo, tree, hf_camel_selectedLSAIdentity);
}
static int dissect_selectedLSA_Id_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_LSAIdentity(TRUE, tvb, offset, pinfo, tree, hf_camel_selectedLSA_Id);
}


static int
dissect_camel_QoS_Subscribed(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_short_QoS_format_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_QoS_Subscribed(TRUE, tvb, offset, pinfo, tree, hf_camel_short_QoS_format);
}


static int
dissect_camel_Ext_QoS_Subscribed(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_long_QoS_format_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_Ext_QoS_Subscribed(TRUE, tvb, offset, pinfo, tree, hf_camel_long_QoS_format);
}



static int
dissect_camel_TCInvokeIdSet(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}


static int
dissect_camel_ExtensionSetextensionId(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_object_identifier(implicit_tag, pinfo, tree, tvb, offset,
                                         hf_index, NULL);

  return offset;
}
static int dissect_extId(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_ExtensionSetextensionId(FALSE, tvb, offset, pinfo, tree, hf_camel_extId);
}

static const ber_sequence_t BackwardServiceInteractionInd_sequence[] = {
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_conferenceTreatmentIndicator_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_callCompletionTreatmentIndicator_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_BackwardServiceInteractionInd(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                BackwardServiceInteractionInd_sequence, hf_index, ett_camel_BackwardServiceInteractionInd);

  return offset;
}
static int dissect_backwardServiceInteractionInd_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_BackwardServiceInteractionInd(TRUE, tvb, offset, pinfo, tree, hf_camel_backwardServiceInteractionInd);
}


static int
dissect_camel_OCTET_STRING_SIZE_2_11(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_bearerCap_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_OCTET_STRING_SIZE_2_11(TRUE, tvb, offset, pinfo, tree, hf_camel_bearerCap);
}


static const value_string camel_BearerCapability_vals[] = {
  {   0, "bearerCap" },
  { 0, NULL }
};

static const ber_choice_t BearerCapability_choice[] = {
  {   0, BER_CLASS_CON, 0, 0, dissect_bearerCap_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_camel_BearerCapability(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              BearerCapability_choice, hf_index, ett_camel_BearerCapability);

  return offset;
}
static int dissect_bearerCapability_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_BearerCapability(TRUE, tvb, offset, pinfo, tree, hf_camel_bearerCapability);
}



static int
dissect_camel_INTEGER_0_8191(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_e1_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER_0_8191(TRUE, tvb, offset, pinfo, tree, hf_camel_e1);
}
static int dissect_e2_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER_0_8191(TRUE, tvb, offset, pinfo, tree, hf_camel_e2);
}
static int dissect_e3_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER_0_8191(TRUE, tvb, offset, pinfo, tree, hf_camel_e3);
}
static int dissect_e4_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER_0_8191(TRUE, tvb, offset, pinfo, tree, hf_camel_e4);
}
static int dissect_e5_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER_0_8191(TRUE, tvb, offset, pinfo, tree, hf_camel_e5);
}
static int dissect_e6_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER_0_8191(TRUE, tvb, offset, pinfo, tree, hf_camel_e6);
}
static int dissect_e7_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER_0_8191(TRUE, tvb, offset, pinfo, tree, hf_camel_e7);
}

static const ber_sequence_t CAI_Gsm0224_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_e1_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_e2_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_e3_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_e4_impl },
  { BER_CLASS_CON, 4, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_e5_impl },
  { BER_CLASS_CON, 5, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_e6_impl },
  { BER_CLASS_CON, 6, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_e7_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_CAI_Gsm0224(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                CAI_Gsm0224_sequence, hf_index, ett_camel_CAI_Gsm0224);

  return offset;
}
static int dissect_cAI_GSM0224_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_CAI_Gsm0224(TRUE, tvb, offset, pinfo, tree, hf_camel_cAI_GSM0224);
}
static int dissect_aOCInitial_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_CAI_Gsm0224(TRUE, tvb, offset, pinfo, tree, hf_camel_aOCInitial);
}



static int
dissect_camel_INTEGER_1_2147483647(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_maxTransferredVolume_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER_1_2147483647(TRUE, tvb, offset, pinfo, tree, hf_camel_maxTransferredVolume);
}



static int
dissect_camel_INTEGER_1_86400(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_maxElapsedTime_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER_1_86400(TRUE, tvb, offset, pinfo, tree, hf_camel_maxElapsedTime);
}
static int dissect_tariffSwitchInterval1_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER_1_86400(TRUE, tvb, offset, pinfo, tree, hf_camel_tariffSwitchInterval1);
}


static const value_string camel_ChargingCharacteristics_vals[] = {
  {   0, "maxTransferredVolume" },
  {   1, "maxElapsedTime" },
  { 0, NULL }
};

static const ber_choice_t ChargingCharacteristics_choice[] = {
  {   0, BER_CLASS_CON, 0, 0, dissect_maxTransferredVolume_impl },
  {   1, BER_CLASS_CON, 1, 0, dissect_maxElapsedTime_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_camel_ChargingCharacteristics(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              ChargingCharacteristics_choice, hf_index, ett_camel_ChargingCharacteristics);

  return offset;
}
static int dissect_chargingCharacteristics_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_ChargingCharacteristics(TRUE, tvb, offset, pinfo, tree, hf_camel_chargingCharacteristics);
}



static int
dissect_camel_INTEGER_0_86400(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_timeGPRSIfNoTariffSwitch_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER_0_86400(TRUE, tvb, offset, pinfo, tree, hf_camel_timeGPRSIfNoTariffSwitch);
}
static int dissect_timeGPRSSinceLastTariffSwitch_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER_0_86400(TRUE, tvb, offset, pinfo, tree, hf_camel_timeGPRSSinceLastTariffSwitch);
}
static int dissect_timeGPRSTariffSwitchInterval_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER_0_86400(TRUE, tvb, offset, pinfo, tree, hf_camel_timeGPRSTariffSwitchInterval);
}

static const ber_sequence_t T_timeGPRSIfTariffSwitch_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_timeGPRSSinceLastTariffSwitch_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_timeGPRSTariffSwitchInterval_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_T_timeGPRSIfTariffSwitch(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                T_timeGPRSIfTariffSwitch_sequence, hf_index, ett_camel_T_timeGPRSIfTariffSwitch);

  return offset;
}
static int dissect_timeGPRSIfTariffSwitch_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_T_timeGPRSIfTariffSwitch(TRUE, tvb, offset, pinfo, tree, hf_camel_timeGPRSIfTariffSwitch);
}


static const value_string camel_ElapsedTime_vals[] = {
  {   0, "timeGPRSIfNoTariffSwitch" },
  {   1, "timeGPRSIfTariffSwitch" },
  { 0, NULL }
};

static const ber_choice_t ElapsedTime_choice[] = {
  {   0, BER_CLASS_CON, 0, 0, dissect_timeGPRSIfNoTariffSwitch_impl },
  {   1, BER_CLASS_CON, 1, 0, dissect_timeGPRSIfTariffSwitch_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_camel_ElapsedTime(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              ElapsedTime_choice, hf_index, ett_camel_ElapsedTime);

  return offset;
}
static int dissect_elapsedTime_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_ElapsedTime(TRUE, tvb, offset, pinfo, tree, hf_camel_elapsedTime);
}


static int
dissect_camel_GSNAddress(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_gGSNAddress_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_GSNAddress(TRUE, tvb, offset, pinfo, tree, hf_camel_gGSNAddress);
}

static const ber_sequence_t ForwardServiceInteractionInd_sequence[] = {
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_conferenceTreatmentIndicator_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_callDiversionTreatmentIndicator_impl },
  { BER_CLASS_CON, 4, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_callingPartyRestrictionIndicator_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_ForwardServiceInteractionInd(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                ForwardServiceInteractionInd_sequence, hf_index, ett_camel_ForwardServiceInteractionInd);

  return offset;
}
static int dissect_forwardServiceInteractionInd_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_ForwardServiceInteractionInd(TRUE, tvb, offset, pinfo, tree, hf_camel_forwardServiceInteractionInd);
}


static int
dissect_camel_OCTET_STRING_SIZE_1_63(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_pDPAddress_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_OCTET_STRING_SIZE_1_63(TRUE, tvb, offset, pinfo, tree, hf_camel_pDPAddress);
}

static const ber_sequence_t PDPType_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_pDPTypeOrganization_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_pDPTypeNumber_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_pDPAddress_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_PDPType(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                PDPType_sequence, hf_index, ett_camel_PDPType);

  return offset;
}
static int dissect_pDPType_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_PDPType(TRUE, tvb, offset, pinfo, tree, hf_camel_pDPType);
}



static int
dissect_camel_INTEGER_0_864000(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_timeSinceTariffSwitch_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER_0_864000(TRUE, tvb, offset, pinfo, tree, hf_camel_timeSinceTariffSwitch);
}



static int
dissect_camel_INTEGER_1_864000(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_tariffSwitchInterval_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER_1_864000(TRUE, tvb, offset, pinfo, tree, hf_camel_tariffSwitchInterval);
}
static int dissect_maxCallPeriodDuration_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER_1_864000(TRUE, tvb, offset, pinfo, tree, hf_camel_maxCallPeriodDuration);
}

static const ber_sequence_t TimeIfTariffSwitch_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_timeSinceTariffSwitch_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_tariffSwitchInterval_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_TimeIfTariffSwitch(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                TimeIfTariffSwitch_sequence, hf_index, ett_camel_TimeIfTariffSwitch);

  return offset;
}
static int dissect_timeIfTariffSwitch_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_TimeIfTariffSwitch(TRUE, tvb, offset, pinfo, tree, hf_camel_timeIfTariffSwitch);
}



static int
dissect_camel_INTEGER_0_2147483647(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_volumeIfNoTariffSwitch_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER_0_2147483647(TRUE, tvb, offset, pinfo, tree, hf_camel_volumeIfNoTariffSwitch);
}
static int dissect_volumeSinceLastTariffSwitch_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER_0_2147483647(TRUE, tvb, offset, pinfo, tree, hf_camel_volumeSinceLastTariffSwitch);
}
static int dissect_volumeTariffSwitchInterval_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER_0_2147483647(TRUE, tvb, offset, pinfo, tree, hf_camel_volumeTariffSwitchInterval);
}

static const ber_sequence_t T_volumeIfTariffSwitch_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_volumeSinceLastTariffSwitch_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_volumeTariffSwitchInterval_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_T_volumeIfTariffSwitch(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                T_volumeIfTariffSwitch_sequence, hf_index, ett_camel_T_volumeIfTariffSwitch);

  return offset;
}
static int dissect_volumeIfTariffSwitch_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_T_volumeIfTariffSwitch(TRUE, tvb, offset, pinfo, tree, hf_camel_volumeIfTariffSwitch);
}


static const value_string camel_TransferredVolume_vals[] = {
  {   0, "volumeIfNoTariffSwitch" },
  {   1, "volumeIfTariffSwitch" },
  { 0, NULL }
};

static const ber_choice_t TransferredVolume_choice[] = {
  {   0, BER_CLASS_CON, 0, 0, dissect_volumeIfNoTariffSwitch_impl },
  {   1, BER_CLASS_CON, 1, 0, dissect_volumeIfTariffSwitch_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_camel_TransferredVolume(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              TransferredVolume_choice, hf_index, ett_camel_TransferredVolume);

  return offset;
}
static int dissect_transferredVolume_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_TransferredVolume(TRUE, tvb, offset, pinfo, tree, hf_camel_transferredVolume);
}


static int
dissect_camel_OBJECT_IDENTIFIER(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_object_identifier(implicit_tag, pinfo, tree, tvb, offset,
                                         hf_index, NULL);

  return offset;
}
static int dissect_global(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_OBJECT_IDENTIFIER(FALSE, tvb, offset, pinfo, tree, hf_camel_global);
}


static const value_string camel_Code_vals[] = {
  {   0, "local" },
  {   1, "global" },
  { 0, NULL }
};

static const ber_choice_t Code_choice[] = {
  {   0, BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_local },
  {   1, BER_CLASS_UNI, BER_UNI_TAG_OID, BER_FLAGS_NOOWNTAG, dissect_global },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_camel_Code(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              Code_choice, hf_index, ett_camel_Code);

  return offset;
}

static const ber_sequence_t PCS_Extensions_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_foo },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_PCS_Extensions(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                PCS_Extensions_sequence, hf_index, ett_camel_PCS_Extensions);

  return offset;
}
static int dissect_pcs_Extensions_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_PCS_Extensions(TRUE, tvb, offset, pinfo, tree, hf_camel_pcs_Extensions);
}


static const value_string camel_T_messageType_vals[] = {
  {   0, "request" },
  {   1, "notification" },
  { 0, NULL }
};


static int
dissect_camel_T_messageType(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_messageType_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_T_messageType(TRUE, tvb, offset, pinfo, tree, hf_camel_messageType);
}

static const ber_sequence_t MiscCallInfo_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_messageType_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_MiscCallInfo(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                MiscCallInfo_sequence, hf_index, ett_camel_MiscCallInfo);

  return offset;
}
static int dissect_miscGPRSInfo_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_MiscCallInfo(TRUE, tvb, offset, pinfo, tree, hf_camel_miscGPRSInfo);
}
static int dissect_miscCallInfo_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_MiscCallInfo(TRUE, tvb, offset, pinfo, tree, hf_camel_miscCallInfo);
}


static int
dissect_camel_NULL(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  { proto_item *ti_tmp;
  ti_tmp = proto_tree_add_item(tree, hf_index, tvb, offset>>8, 0, FALSE);
  proto_item_append_text(ti_tmp, ": NULL");
  }

  return offset;
}
static int dissect_firstExtensionExtensionType(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_NULL(FALSE, tvb, offset, pinfo, tree, hf_camel_firstExtensionExtensionType);
}
static int dissect_or_Call_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_NULL(TRUE, tvb, offset, pinfo, tree, hf_camel_or_Call);
}
static int dissect_forwardedCall_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_NULL(TRUE, tvb, offset, pinfo, tree, hf_camel_forwardedCall);
}
static int dissect_callForwarded_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_NULL(TRUE, tvb, offset, pinfo, tree, hf_camel_callForwarded);
}
static int dissect_routeNotPermitted_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_NULL(TRUE, tvb, offset, pinfo, tree, hf_camel_routeNotPermitted);
}
static int dissect_nonCUGCall_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_NULL(TRUE, tvb, offset, pinfo, tree, hf_camel_nonCUGCall);
}
static int dissect_allRequests_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_NULL(TRUE, tvb, offset, pinfo, tree, hf_camel_allRequests);
}
static int dissect_assumedIdle_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_NULL(TRUE, tvb, offset, pinfo, tree, hf_camel_assumedIdle);
}
static int dissect_camelBusy_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_NULL(TRUE, tvb, offset, pinfo, tree, hf_camel_camelBusy);
}
static int dissect_notProvidedFromVLR_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_NULL(TRUE, tvb, offset, pinfo, tree, hf_camel_notProvidedFromVLR);
}
static int dissect_saiPresent_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_NULL(TRUE, tvb, offset, pinfo, tree, hf_camel_saiPresent);
}
static int dissect_currentLocationRetrieved_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_NULL(TRUE, tvb, offset, pinfo, tree, hf_camel_currentLocationRetrieved);
}
static int dissect_routeingAreaUpdate_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_NULL(TRUE, tvb, offset, pinfo, tree, hf_camel_routeingAreaUpdate);
}
static int dissect_secondaryPDPContext_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_NULL(TRUE, tvb, offset, pinfo, tree, hf_camel_secondaryPDPContext);
}
static int dissect_cug_OutgoingAccess_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_NULL(TRUE, tvb, offset, pinfo, tree, hf_camel_cug_OutgoingAccess);
}
static int dissect_none_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_NULL(TRUE, tvb, offset, pinfo, tree, hf_camel_none);
}
static int dissect_callReleasedAtTcpExpiry_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_NULL(TRUE, tvb, offset, pinfo, tree, hf_camel_callReleasedAtTcpExpiry);
}
static int dissect_gsm_ForwardingPending_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_NULL(TRUE, tvb, offset, pinfo, tree, hf_camel_gsm_ForwardingPending);
}


static const value_string camel_SupportedExtensionsExtensionType_vals[] = {
  {   0, "firstExtensionExtensionType" },
  { 0, NULL }
};

static const ber_choice_t SupportedExtensionsExtensionType_choice[] = {
  {   0, BER_CLASS_UNI, BER_UNI_TAG_NULL, BER_FLAGS_NOOWNTAG, dissect_firstExtensionExtensionType },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_camel_SupportedExtensionsExtensionType(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              SupportedExtensionsExtensionType_choice, hf_index, ett_camel_SupportedExtensionsExtensionType);

  return offset;
}
static int dissect_value_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_SupportedExtensionsExtensionType(TRUE, tvb, offset, pinfo, tree, hf_camel_value);
}

static const ber_sequence_t PrivateExtension_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_OID, BER_FLAGS_NOOWNTAG, dissect_extId },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_PrivateExtension(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                PrivateExtension_sequence, hf_index, ett_camel_PrivateExtension);

  return offset;
}
static int dissect_PrivateExtensionList_item(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_PrivateExtension(FALSE, tvb, offset, pinfo, tree, hf_camel_PrivateExtensionList_item);
}


static int
dissect_camel_AdditionalCallingPartyNumber(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_camel_Digits(implicit_tag, tvb, offset, pinfo, tree, hf_index);

  return offset;
}
static int dissect_additionalCallingPartyNumber_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_AdditionalCallingPartyNumber(TRUE, tvb, offset, pinfo, tree, hf_camel_additionalCallingPartyNumber);
}

static const ber_sequence_t AOCSubsequent_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_cAI_GSM0224_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_tariffSwitchInterval1_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_AOCSubsequent(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                AOCSubsequent_sequence, hf_index, ett_camel_AOCSubsequent);

  return offset;
}
static int dissect_aOCSubsequent_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_AOCSubsequent(TRUE, tvb, offset, pinfo, tree, hf_camel_aOCSubsequent);
}
static int dissect_aOCAfterAnswer_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_AOCSubsequent(TRUE, tvb, offset, pinfo, tree, hf_camel_aOCAfterAnswer);
}


static int
dissect_camel_AssistingSSPIPRoutingAddress(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_camel_Digits(implicit_tag, tvb, offset, pinfo, tree, hf_index);

  return offset;
}
static int dissect_assistingSSPIPRoutingAddress_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_AssistingSSPIPRoutingAddress(TRUE, tvb, offset, pinfo, tree, hf_camel_assistingSSPIPRoutingAddress);
}


static int
dissect_camel_FreeFormatData(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_freeFormatData_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_FreeFormatData(TRUE, tvb, offset, pinfo, tree, hf_camel_freeFormatData);
}

static const ber_sequence_t T_fCIBCCCAMELsequence1_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_freeFormatData_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_pDPID_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_appendFreeFormatData_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_T_fCIBCCCAMELsequence1(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                T_fCIBCCCAMELsequence1_sequence, hf_index, ett_camel_T_fCIBCCCAMELsequence1);

  return offset;
}
static int dissect_fCIBCCCAMELsequence1_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_T_fCIBCCCAMELsequence1(TRUE, tvb, offset, pinfo, tree, hf_camel_fCIBCCCAMELsequence1);
}

static const ber_sequence_t CamelFCIGPRSBillingChargingCharacteristics_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_fCIBCCCAMELsequence1_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_CamelFCIGPRSBillingChargingCharacteristics(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                CamelFCIGPRSBillingChargingCharacteristics_sequence, hf_index, ett_camel_CamelFCIGPRSBillingChargingCharacteristics);

  return offset;
}

static const ber_sequence_t T_fCIBCCCAMELsequence11_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_freeFormatData_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_appendFreeFormatData_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_T_fCIBCCCAMELsequence11(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                T_fCIBCCCAMELsequence11_sequence, hf_index, ett_camel_T_fCIBCCCAMELsequence11);

  return offset;
}
static int dissect_fCIBCCCAMELsequence11_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_T_fCIBCCCAMELsequence11(TRUE, tvb, offset, pinfo, tree, hf_camel_fCIBCCCAMELsequence11);
}


static const value_string camel_CamelFCISMSBillingChargingCharacteristics_vals[] = {
  {   0, "fCIBCCCAMELsequence1" },
  { 0, NULL }
};

static const ber_choice_t CamelFCISMSBillingChargingCharacteristics_choice[] = {
  {   0, BER_CLASS_CON, 0, 0, dissect_fCIBCCCAMELsequence11_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_camel_CamelFCISMSBillingChargingCharacteristics(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              CamelFCISMSBillingChargingCharacteristics_choice, hf_index, ett_camel_CamelFCISMSBillingChargingCharacteristics);

  return offset;
}


static int
dissect_camel_ChargeNumber(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_camel_LocationNumber(implicit_tag, tvb, offset, pinfo, tree, hf_index);

  return offset;
}
static int dissect_chargeNumber_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_ChargeNumber(TRUE, tvb, offset, pinfo, tree, hf_camel_chargeNumber);
}


static const value_string camel_ChargingResult_vals[] = {
  {   0, "transferredVolume" },
  {   1, "elapsedTime" },
  { 0, NULL }
};

static const ber_choice_t ChargingResult_choice[] = {
  {   0, BER_CLASS_CON, 0, 0, dissect_transferredVolume_impl },
  {   1, BER_CLASS_CON, 1, 0, dissect_elapsedTime_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_camel_ChargingResult(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              ChargingResult_choice, hf_index, ett_camel_ChargingResult);

  return offset;
}
static int dissect_chargingResult_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_ChargingResult(TRUE, tvb, offset, pinfo, tree, hf_camel_chargingResult);
}



static int
dissect_camel_INTEGER_1_30(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_minimumNbOfDigits_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER_1_30(TRUE, tvb, offset, pinfo, tree, hf_camel_minimumNbOfDigits);
}
static int dissect_maximumNbOfDigits_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER_1_30(TRUE, tvb, offset, pinfo, tree, hf_camel_maximumNbOfDigits);
}


static int
dissect_camel_OCTET_STRING_SIZE_1_2(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_endOfReplyDigit_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_OCTET_STRING_SIZE_1_2(TRUE, tvb, offset, pinfo, tree, hf_camel_endOfReplyDigit);
}
static int dissect_cancelDigit_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_OCTET_STRING_SIZE_1_2(TRUE, tvb, offset, pinfo, tree, hf_camel_cancelDigit);
}
static int dissect_startDigit_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_OCTET_STRING_SIZE_1_2(TRUE, tvb, offset, pinfo, tree, hf_camel_startDigit);
}



static int
dissect_camel_INTEGER_1_127(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_firstDigitTimeOut_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER_1_127(TRUE, tvb, offset, pinfo, tree, hf_camel_firstDigitTimeOut);
}
static int dissect_interDigitTimeOut_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER_1_127(TRUE, tvb, offset, pinfo, tree, hf_camel_interDigitTimeOut);
}
static int dissect_numberOfRepetitions_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER_1_127(TRUE, tvb, offset, pinfo, tree, hf_camel_numberOfRepetitions);
}


static int
dissect_camel_BOOLEAN(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_boolean(implicit_tag, pinfo, tree, tvb, offset, hf_index);

  return offset;
}
static int dissect_interruptableAnnInd_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_BOOLEAN(TRUE, tvb, offset, pinfo, tree, hf_camel_interruptableAnnInd);
}
static int dissect_voiceInformation_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_BOOLEAN(TRUE, tvb, offset, pinfo, tree, hf_camel_voiceInformation);
}
static int dissect_voiceBack1_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_BOOLEAN(TRUE, tvb, offset, pinfo, tree, hf_camel_voiceBack1);
}
static int dissect_releaseIfdurationExceeded_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_BOOLEAN(TRUE, tvb, offset, pinfo, tree, hf_camel_releaseIfdurationExceeded);
}
static int dissect_tone1_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_BOOLEAN(TRUE, tvb, offset, pinfo, tree, hf_camel_tone1);
}
static int dissect_callActive_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_BOOLEAN(TRUE, tvb, offset, pinfo, tree, hf_camel_callActive);
}
static int dissect_disconnectFromIPForbidden_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_BOOLEAN(TRUE, tvb, offset, pinfo, tree, hf_camel_disconnectFromIPForbidden);
}
static int dissect_requestAnnouncementComplete_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_BOOLEAN(TRUE, tvb, offset, pinfo, tree, hf_camel_requestAnnouncementComplete);
}
static int dissect_active_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_BOOLEAN(TRUE, tvb, offset, pinfo, tree, hf_camel_active);
}

static const ber_sequence_t CollectedDigits_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_minimumNbOfDigits_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_maximumNbOfDigits_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_endOfReplyDigit_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_cancelDigit_impl },
  { BER_CLASS_CON, 4, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_startDigit_impl },
  { BER_CLASS_CON, 5, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_firstDigitTimeOut_impl },
  { BER_CLASS_CON, 6, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_interDigitTimeOut_impl },
  { BER_CLASS_CON, 7, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_errorTreatment_impl },
  { BER_CLASS_CON, 8, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_interruptableAnnInd_impl },
  { BER_CLASS_CON, 9, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_voiceInformation_impl },
  { BER_CLASS_CON, 10, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_voiceBack1_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_CollectedDigits(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                CollectedDigits_sequence, hf_index, ett_camel_CollectedDigits);

  return offset;
}
static int dissect_collectedDigits_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_CollectedDigits(TRUE, tvb, offset, pinfo, tree, hf_camel_collectedDigits);
}


static const value_string camel_CollectedInfo_vals[] = {
  {   0, "collectedDigits" },
  { 0, NULL }
};

static const ber_choice_t CollectedInfo_choice[] = {
  {   0, BER_CLASS_CON, 0, 0, dissect_collectedDigits_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_camel_CollectedInfo(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              CollectedInfo_choice, hf_index, ett_camel_CollectedInfo);

  return offset;
}
static int dissect_collectedInfo_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_CollectedInfo(TRUE, tvb, offset, pinfo, tree, hf_camel_collectedInfo);
}


static int
dissect_camel_CorrelationID(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_camel_Digits(implicit_tag, tvb, offset, pinfo, tree, hf_index);

  return offset;
}
static int dissect_correlationID_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_CorrelationID(TRUE, tvb, offset, pinfo, tree, hf_camel_correlationID);
}

static const ber_sequence_t DestinationRoutingAddress_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_DestinationRoutingAddress_item },
};

static int
dissect_camel_DestinationRoutingAddress(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, pinfo, tree, tvb, offset,
                                   DestinationRoutingAddress_sequence_of, hf_index, ett_camel_DestinationRoutingAddress);

  return offset;
}
static int dissect_destinationRoutingAddress_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_DestinationRoutingAddress(TRUE, tvb, offset, pinfo, tree, hf_camel_destinationRoutingAddress);
}


static const value_string camel_DpSpecificCriteria_vals[] = {
  {   1, "applicationTimer" },
  { 0, NULL }
};

static const ber_choice_t DpSpecificCriteria_choice[] = {
  {   1, BER_CLASS_CON, 1, 0, dissect_applicationTimer_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_camel_DpSpecificCriteria(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              DpSpecificCriteria_choice, hf_index, ett_camel_DpSpecificCriteria);

  return offset;
}
static int dissect_dpSpecificCriteria_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_DpSpecificCriteria(TRUE, tvb, offset, pinfo, tree, hf_camel_dpSpecificCriteria);
}

static const ber_sequence_t T_routeSelectFailureSpecificInfo_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_failureCause_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_T_routeSelectFailureSpecificInfo(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                T_routeSelectFailureSpecificInfo_sequence, hf_index, ett_camel_T_routeSelectFailureSpecificInfo);

  return offset;
}
static int dissect_routeSelectFailureSpecificInfo_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_T_routeSelectFailureSpecificInfo(TRUE, tvb, offset, pinfo, tree, hf_camel_routeSelectFailureSpecificInfo);
}

static const ber_sequence_t T_oCalledPartyBusySpecificInfo_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_busyCause_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_T_oCalledPartyBusySpecificInfo(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                T_oCalledPartyBusySpecificInfo_sequence, hf_index, ett_camel_T_oCalledPartyBusySpecificInfo);

  return offset;
}
static int dissect_oCalledPartyBusySpecificInfo_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_T_oCalledPartyBusySpecificInfo(TRUE, tvb, offset, pinfo, tree, hf_camel_oCalledPartyBusySpecificInfo);
}

static const ber_sequence_t T_oNoAnswerSpecificInfo_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_foo },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_T_oNoAnswerSpecificInfo(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                T_oNoAnswerSpecificInfo_sequence, hf_index, ett_camel_T_oNoAnswerSpecificInfo);

  return offset;
}
static int dissect_oNoAnswerSpecificInfo_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_T_oNoAnswerSpecificInfo(TRUE, tvb, offset, pinfo, tree, hf_camel_oNoAnswerSpecificInfo);
}

static const ber_sequence_t T_oAnswerSpecificInfo_sequence[] = {
  { BER_CLASS_CON, 50, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_destinationAddress_impl },
  { BER_CLASS_CON, 51, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_or_Call_impl },
  { BER_CLASS_CON, 52, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_forwardedCall_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_T_oAnswerSpecificInfo(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                T_oAnswerSpecificInfo_sequence, hf_index, ett_camel_T_oAnswerSpecificInfo);

  return offset;
}
static int dissect_oAnswerSpecificInfo_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_T_oAnswerSpecificInfo(TRUE, tvb, offset, pinfo, tree, hf_camel_oAnswerSpecificInfo);
}

static const ber_sequence_t T_oDisconnectSpecificInfo_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_releaseCause_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_T_oDisconnectSpecificInfo(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                T_oDisconnectSpecificInfo_sequence, hf_index, ett_camel_T_oDisconnectSpecificInfo);

  return offset;
}
static int dissect_oDisconnectSpecificInfo_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_T_oDisconnectSpecificInfo(TRUE, tvb, offset, pinfo, tree, hf_camel_oDisconnectSpecificInfo);
}

static const ber_sequence_t T_tBusySpecificInfo_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_busyCause_impl },
  { BER_CLASS_CON, 50, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_callForwarded_impl },
  { BER_CLASS_CON, 51, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_routeNotPermitted_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_T_tBusySpecificInfo(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                T_tBusySpecificInfo_sequence, hf_index, ett_camel_T_tBusySpecificInfo);

  return offset;
}
static int dissect_tBusySpecificInfo_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_T_tBusySpecificInfo(TRUE, tvb, offset, pinfo, tree, hf_camel_tBusySpecificInfo);
}

static const ber_sequence_t T_tNoAnswerSpecificInfo_sequence[] = {
  { BER_CLASS_CON, 50, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_callForwarded_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_T_tNoAnswerSpecificInfo(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                T_tNoAnswerSpecificInfo_sequence, hf_index, ett_camel_T_tNoAnswerSpecificInfo);

  return offset;
}
static int dissect_tNoAnswerSpecificInfo_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_T_tNoAnswerSpecificInfo(TRUE, tvb, offset, pinfo, tree, hf_camel_tNoAnswerSpecificInfo);
}

static const ber_sequence_t T_tAnswerSpecificInfo_sequence[] = {
  { BER_CLASS_CON, 50, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_destinationAddress_impl },
  { BER_CLASS_CON, 51, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_or_Call_impl },
  { BER_CLASS_CON, 52, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_forwardedCall_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_T_tAnswerSpecificInfo(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                T_tAnswerSpecificInfo_sequence, hf_index, ett_camel_T_tAnswerSpecificInfo);

  return offset;
}
static int dissect_tAnswerSpecificInfo_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_T_tAnswerSpecificInfo(TRUE, tvb, offset, pinfo, tree, hf_camel_tAnswerSpecificInfo);
}

static const ber_sequence_t T_tDisconnectSpecificInfo_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_releaseCause_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_T_tDisconnectSpecificInfo(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                T_tDisconnectSpecificInfo_sequence, hf_index, ett_camel_T_tDisconnectSpecificInfo);

  return offset;
}
static int dissect_tDisconnectSpecificInfo_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_T_tDisconnectSpecificInfo(TRUE, tvb, offset, pinfo, tree, hf_camel_tDisconnectSpecificInfo);
}


static const value_string camel_EventSpecificInformationBCSM_vals[] = {
  {   2, "routeSelectFailureSpecificInfo" },
  {   3, "oCalledPartyBusySpecificInfo" },
  {   4, "oNoAnswerSpecificInfo" },
  {   5, "oAnswerSpecificInfo" },
  {   7, "oDisconnectSpecificInfo" },
  {   8, "tBusySpecificInfo" },
  {   9, "tNoAnswerSpecificInfo" },
  {  10, "tAnswerSpecificInfo" },
  {  12, "tDisconnectSpecificInfo" },
  { 0, NULL }
};

static const ber_choice_t EventSpecificInformationBCSM_choice[] = {
  {   2, BER_CLASS_CON, 2, 0, dissect_routeSelectFailureSpecificInfo_impl },
  {   3, BER_CLASS_CON, 3, 0, dissect_oCalledPartyBusySpecificInfo_impl },
  {   4, BER_CLASS_CON, 4, 0, dissect_oNoAnswerSpecificInfo_impl },
  {   5, BER_CLASS_CON, 5, 0, dissect_oAnswerSpecificInfo_impl },
  {   7, BER_CLASS_CON, 7, 0, dissect_oDisconnectSpecificInfo_impl },
  {   8, BER_CLASS_CON, 8, 0, dissect_tBusySpecificInfo_impl },
  {   9, BER_CLASS_CON, 9, 0, dissect_tNoAnswerSpecificInfo_impl },
  {  10, BER_CLASS_CON, 10, 0, dissect_tAnswerSpecificInfo_impl },
  {  12, BER_CLASS_CON, 12, 0, dissect_tDisconnectSpecificInfo_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_camel_EventSpecificInformationBCSM(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              EventSpecificInformationBCSM_choice, hf_index, ett_camel_EventSpecificInformationBCSM);

  return offset;
}
static int dissect_eventSpecificInformationBCSM_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_EventSpecificInformationBCSM(TRUE, tvb, offset, pinfo, tree, hf_camel_eventSpecificInformationBCSM);
}

static const ber_sequence_t T_o_smsFailureSpecificInfo_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_failureCause1_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_T_o_smsFailureSpecificInfo(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                T_o_smsFailureSpecificInfo_sequence, hf_index, ett_camel_T_o_smsFailureSpecificInfo);

  return offset;
}
static int dissect_o_smsFailureSpecificInfo_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_T_o_smsFailureSpecificInfo(TRUE, tvb, offset, pinfo, tree, hf_camel_o_smsFailureSpecificInfo);
}

static const ber_sequence_t T_o_smsSubmittedSpecificInfo_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_foo },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_T_o_smsSubmittedSpecificInfo(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                T_o_smsSubmittedSpecificInfo_sequence, hf_index, ett_camel_T_o_smsSubmittedSpecificInfo);

  return offset;
}
static int dissect_o_smsSubmittedSpecificInfo_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_T_o_smsSubmittedSpecificInfo(TRUE, tvb, offset, pinfo, tree, hf_camel_o_smsSubmittedSpecificInfo);
}


static const value_string camel_EventSpecificInformationSMS_vals[] = {
  {   0, "o-smsFailureSpecificInfo" },
  {   1, "o-smsSubmittedSpecificInfo" },
  { 0, NULL }
};

static const ber_choice_t EventSpecificInformationSMS_choice[] = {
  {   0, BER_CLASS_CON, 0, 0, dissect_o_smsFailureSpecificInfo_impl },
  {   1, BER_CLASS_CON, 1, 0, dissect_o_smsSubmittedSpecificInfo_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_camel_EventSpecificInformationSMS(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              EventSpecificInformationSMS_choice, hf_index, ett_camel_EventSpecificInformationSMS);

  return offset;
}
static int dissect_eventSpecificInformationSMS_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_EventSpecificInformationSMS(TRUE, tvb, offset, pinfo, tree, hf_camel_eventSpecificInformationSMS);
}

static const ber_sequence_t GapIndicators_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_duration_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_gapInterval_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_GapIndicators(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                GapIndicators_sequence, hf_index, ett_camel_GapIndicators);

  return offset;
}
static int dissect_gapIndicators_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_GapIndicators(TRUE, tvb, offset, pinfo, tree, hf_camel_gapIndicators);
}

static const ber_sequence_t GapOnService_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_serviceKey_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_GapOnService(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                GapOnService_sequence, hf_index, ett_camel_GapOnService);

  return offset;
}
static int dissect_gapOnService_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_GapOnService(TRUE, tvb, offset, pinfo, tree, hf_camel_gapOnService);
}

static const ber_sequence_t GenericNumbers_set_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_GenericNumbers_item },
};

static int
dissect_camel_GenericNumbers(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_set_of(implicit_tag, pinfo, tree, tvb, offset,
                              GenericNumbers_set_of, hf_index, ett_camel_GenericNumbers);

  return offset;
}
static int dissect_genericNumbers_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_GenericNumbers(TRUE, tvb, offset, pinfo, tree, hf_camel_genericNumbers);
}


static const value_string camel_GPRS_QoS_vals[] = {
  {   0, "short-QoS-format" },
  {   1, "long-QoS-format" },
  { 0, NULL }
};

static const ber_choice_t GPRS_QoS_choice[] = {
  {   0, BER_CLASS_CON, 0, 0, dissect_short_QoS_format_impl },
  {   1, BER_CLASS_CON, 1, 0, dissect_long_QoS_format_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_camel_GPRS_QoS(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              GPRS_QoS_choice, hf_index, ett_camel_GPRS_QoS);

  return offset;
}
static int dissect_requested_QoS_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_GPRS_QoS(TRUE, tvb, offset, pinfo, tree, hf_camel_requested_QoS);
}
static int dissect_subscribed_QoS_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_GPRS_QoS(TRUE, tvb, offset, pinfo, tree, hf_camel_subscribed_QoS);
}
static int dissect_negotiated_QoS_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_GPRS_QoS(TRUE, tvb, offset, pinfo, tree, hf_camel_negotiated_QoS);
}

static const ber_sequence_t GPRSEvent_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_gPRSEventType_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_monitorMode_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_GPRSEvent(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                GPRSEvent_sequence, hf_index, ett_camel_GPRSEvent);

  return offset;
}
static int dissect_GPRSEventArray_item(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_GPRSEvent(FALSE, tvb, offset, pinfo, tree, hf_camel_GPRSEventArray_item);
}

static const ber_sequence_t GPRSMSClass_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_mSNetworkCapability_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_mSRadioAccessCapability_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_GPRSMSClass(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                GPRSMSClass_sequence, hf_index, ett_camel_GPRSMSClass);

  return offset;
}
static int dissect_gPRSMSClass_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_GPRSMSClass(TRUE, tvb, offset, pinfo, tree, hf_camel_gPRSMSClass);
}


static int
dissect_camel_InvokeID(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_camel_TCInvokeIdSet(implicit_tag, tvb, offset, pinfo, tree, hf_index);

  return offset;
}
static int dissect_invokeID_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_InvokeID(TRUE, tvb, offset, pinfo, tree, hf_camel_invokeID);
}


static int
dissect_camel_IPRoutingAddress(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_camel_CalledPartyNumber(implicit_tag, tvb, offset, pinfo, tree, hf_index);

  return offset;
}
static int dissect_ipRoutingAddress_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_IPRoutingAddress(TRUE, tvb, offset, pinfo, tree, hf_camel_ipRoutingAddress);
}

static const ber_sequence_t QualityOfService_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_requested_QoS_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_subscribed_QoS_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_negotiated_QoS_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_QualityOfService(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                QualityOfService_sequence, hf_index, ett_camel_QualityOfService);

  return offset;
}
static int dissect_qualityOfService_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_QualityOfService(TRUE, tvb, offset, pinfo, tree, hf_camel_qualityOfService);
}


static const value_string camel_ReceivingSideID_vals[] = {
  {   1, "receivingSideID" },
  { 0, NULL }
};

static const ber_choice_t ReceivingSideID_choice[] = {
  {   1, BER_CLASS_CON, 1, 0, dissect_receivingSideID_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_camel_ReceivingSideID(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              ReceivingSideID_choice, hf_index, ett_camel_ReceivingSideID);

  return offset;
}
static int dissect_legID2_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_ReceivingSideID(TRUE, tvb, offset, pinfo, tree, hf_camel_legID2);
}
static int dissect_partyToCharge1_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_ReceivingSideID(TRUE, tvb, offset, pinfo, tree, hf_camel_partyToCharge1);
}

static const ber_sequence_t RequestedInformationTypeList_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_ENUMERATED, BER_FLAGS_NOOWNTAG, dissect_RequestedInformationTypeList_item },
};

static int
dissect_camel_RequestedInformationTypeList(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, pinfo, tree, tvb, offset,
                                   RequestedInformationTypeList_sequence_of, hf_index, ett_camel_RequestedInformationTypeList);

  return offset;
}
static int dissect_requestedInformationTypeList_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_RequestedInformationTypeList(TRUE, tvb, offset, pinfo, tree, hf_camel_requestedInformationTypeList);
}


static const value_string camel_RequestedInformationValue_vals[] = {
  {   0, "callAttemptElapsedTimeValue" },
  {   1, "callStopTimeValue" },
  {   2, "callConnectedElapsedTimeValue" },
  {  30, "releaseCauseValue" },
  { 0, NULL }
};

static const ber_choice_t RequestedInformationValue_choice[] = {
  {   0, BER_CLASS_CON, 0, 0, dissect_callAttemptElapsedTimeValue_impl },
  {   1, BER_CLASS_CON, 1, 0, dissect_callStopTimeValue_impl },
  {   2, BER_CLASS_CON, 2, 0, dissect_callConnectedElapsedTimeValue_impl },
  {  30, BER_CLASS_CON, 30, 0, dissect_releaseCauseValue_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_camel_RequestedInformationValue(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              RequestedInformationValue_choice, hf_index, ett_camel_RequestedInformationValue);

  return offset;
}
static int dissect_requestedInformationValue_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_RequestedInformationValue(TRUE, tvb, offset, pinfo, tree, hf_camel_requestedInformationValue);
}


static const value_string camel_SendingSideID_vals[] = {
  {   0, "sendingSideID" },
  { 0, NULL }
};

static const ber_choice_t SendingSideID_choice[] = {
  {   0, BER_CLASS_CON, 0, 0, dissect_sendingSideID_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_camel_SendingSideID(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              SendingSideID_choice, hf_index, ett_camel_SendingSideID);

  return offset;
}
static int dissect_partyToCharge_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_SendingSideID(TRUE, tvb, offset, pinfo, tree, hf_camel_partyToCharge);
}
static int dissect_legID1_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_SendingSideID(TRUE, tvb, offset, pinfo, tree, hf_camel_legID1);
}

static const ber_sequence_t ServiceInteractionIndicatorsTwo_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_forwardServiceInteractionInd_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_backwardServiceInteractionInd_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_bothwayThroughConnectionInd_impl },
  { BER_CLASS_CON, 4, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_connectedNumberTreatmentInd_impl },
  { BER_CLASS_CON, 13, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_nonCUGCall_impl },
  { BER_CLASS_CON, 50, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_holdTreatmentIndicator_impl },
  { BER_CLASS_CON, 51, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_cwTreatmentIndicator_impl },
  { BER_CLASS_CON, 52, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_ectTreatmentIndicator_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_ServiceInteractionIndicatorsTwo(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                ServiceInteractionIndicatorsTwo_sequence, hf_index, ett_camel_ServiceInteractionIndicatorsTwo);

  return offset;
}
static int dissect_serviceInteractionIndicatorsTwo_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_ServiceInteractionIndicatorsTwo(TRUE, tvb, offset, pinfo, tree, hf_camel_serviceInteractionIndicatorsTwo);
}

static const ber_sequence_t SMSEvent_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_eventTypeSMS_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_monitorMode_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_SMSEvent(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                SMSEvent_sequence, hf_index, ett_camel_SMSEvent);

  return offset;
}
static int dissect_SMSEventArray_item(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_SMSEvent(FALSE, tvb, offset, pinfo, tree, hf_camel_SMSEventArray_item);
}


static const value_string camel_TimeInformation_vals[] = {
  {   0, "timeIfNoTariffSwitch" },
  {   1, "timeIfTariffSwitch" },
  { 0, NULL }
};

static const ber_choice_t TimeInformation_choice[] = {
  {   0, BER_CLASS_CON, 0, 0, dissect_timeIfNoTariffSwitch_impl },
  {   1, BER_CLASS_CON, 1, 0, dissect_timeIfTariffSwitch_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_camel_TimeInformation(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              TimeInformation_choice, hf_index, ett_camel_TimeInformation);

  return offset;
}
static int dissect_timeInformation_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_TimeInformation(TRUE, tvb, offset, pinfo, tree, hf_camel_timeInformation);
}


static int
dissect_camel_TimerValue(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_camel_Integer4(implicit_tag, tvb, offset, pinfo, tree, hf_index);

  return offset;
}
static int dissect_timervalue_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_TimerValue(TRUE, tvb, offset, pinfo, tree, hf_camel_timervalue);
}

static const ber_sequence_t Tone_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_toneID_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_duration1_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_Tone(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                Tone_sequence, hf_index, ett_camel_Tone);

  return offset;
}
static int dissect_tone_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_Tone(TRUE, tvb, offset, pinfo, tree, hf_camel_tone);
}


static int
dissect_camel_OCTET_STRING_SIZE_2(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_time_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_OCTET_STRING_SIZE_2(TRUE, tvb, offset, pinfo, tree, hf_camel_time);
}


static int
dissect_camel_OCTET_STRING_SIZE_4(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_date_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_OCTET_STRING_SIZE_4(TRUE, tvb, offset, pinfo, tree, hf_camel_date);
}
static int dissect_price_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_OCTET_STRING_SIZE_4(TRUE, tvb, offset, pinfo, tree, hf_camel_price);
}


static const value_string camel_VariablePart_vals[] = {
  {   0, "integer" },
  {   1, "number" },
  {   2, "time" },
  {   3, "date" },
  {   4, "price" },
  { 0, NULL }
};

static const ber_choice_t VariablePart_choice[] = {
  {   0, BER_CLASS_CON, 0, 0, dissect_integer_impl },
  {   1, BER_CLASS_CON, 1, 0, dissect_number_impl },
  {   2, BER_CLASS_CON, 2, 0, dissect_time_impl },
  {   3, BER_CLASS_CON, 3, 0, dissect_date_impl },
  {   4, BER_CLASS_CON, 4, 0, dissect_price_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_camel_VariablePart(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              VariablePart_choice, hf_index, ett_camel_VariablePart);

  return offset;
}
static int dissect_VariablePartsArray_item(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_VariablePart(FALSE, tvb, offset, pinfo, tree, hf_camel_VariablePartsArray_item);
}


static int
dissect_camel_ApplyChargingReportArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}


static const value_string camel_CancelArg_vals[] = {
  {   0, "invokeID" },
  {   1, "allRequests" },
  { 0, NULL }
};

static const ber_choice_t CancelArg_choice[] = {
  {   0, BER_CLASS_CON, 0, 0, dissect_invokeID_impl },
  {   1, BER_CLASS_CON, 1, 0, dissect_allRequests_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_camel_CancelArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              CancelArg_choice, hf_index, ett_camel_CancelArg);

  return offset;
}


static int
dissect_camel_FurnishChargingInformationArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}


static int
dissect_camel_ReleaseCallArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}


static const value_string camel_ReceivedInformationArg_vals[] = {
  {   0, "digitsResponse" },
  { 0, NULL }
};

static const ber_choice_t ReceivedInformationArg_choice[] = {
  {   0, BER_CLASS_CON, 0, 0, dissect_digitsResponse_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_camel_ReceivedInformationArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              ReceivedInformationArg_choice, hf_index, ett_camel_ReceivedInformationArg);

  return offset;
}


static int
dissect_camel_FurnishChargingInformationSMSArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}

static const ber_sequence_t ConnectGPRSArg_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_accessPointName_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_pdpID_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_ConnectGPRSArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                ConnectGPRSArg_sequence, hf_index, ett_camel_ConnectGPRSArg);

  return offset;
}

static const ber_sequence_t EntityReleasedGPRSArg_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_gPRSCause_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_pDPID_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_EntityReleasedGPRSArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                EntityReleasedGPRSArg_sequence, hf_index, ett_camel_EntityReleasedGPRSArg);

  return offset;
}


static int
dissect_camel_FurnishChargingInformationGPRSArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}

static const ber_sequence_t ReleaseGPRSArg_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_gprsCause_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_pDPID_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_ReleaseGPRSArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                ReleaseGPRSArg_sequence, hf_index, ett_camel_ReleaseGPRSArg);

  return offset;
}

static const ber_sequence_t GPRSEventArray_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_GPRSEventArray_item },
};

static int
dissect_camel_GPRSEventArray(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, pinfo, tree, tvb, offset,
                                   GPRSEventArray_sequence_of, hf_index, ett_camel_GPRSEventArray);

  return offset;
}
static int dissect_gPRSEvent_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_GPRSEventArray(TRUE, tvb, offset, pinfo, tree, hf_camel_gPRSEvent);
}

static const ber_sequence_t RequestReportGPRSEventArg_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_gPRSEvent_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_pDPID_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_RequestReportGPRSEventArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                RequestReportGPRSEventArg_sequence, hf_index, ett_camel_RequestReportGPRSEventArg);

  return offset;
}

static const ber_sequence_t SendChargingInformationGPRSArg_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_sCIGPRSBillingChargingCharacteristics_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_SendChargingInformationGPRSArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                SendChargingInformationGPRSArg_sequence, hf_index, ett_camel_SendChargingInformationGPRSArg);

  return offset;
}


static const value_string camel_SubscriberState_vals[] = {
  {   0, "assumedIdle" },
  {   1, "camelBusy" },
  {   2, "netDetNotReachable" },
  {   3, "notProvidedFromVLR" },
  { 0, NULL }
};

static const ber_choice_t SubscriberState_choice[] = {
  {   0, BER_CLASS_CON, 0, 0, dissect_assumedIdle_impl },
  {   1, BER_CLASS_CON, 1, 0, dissect_camelBusy_impl },
  {   2, BER_CLASS_UNI, BER_UNI_TAG_ENUMERATED, BER_FLAGS_NOOWNTAG, dissect_netDetNotReachable },
  {   3, BER_CLASS_CON, 2, 0, dissect_notProvidedFromVLR_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_camel_SubscriberState(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              SubscriberState_choice, hf_index, ett_camel_SubscriberState);

  return offset;
}
static int dissect_subscriberState_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_SubscriberState(TRUE, tvb, offset, pinfo, tree, hf_camel_subscriberState);
}

static const ber_sequence_t PrivateExtensionList_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_PrivateExtensionList_item },
};

static int
dissect_camel_PrivateExtensionList(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, pinfo, tree, tvb, offset,
                                   PrivateExtensionList_sequence_of, hf_index, ett_camel_PrivateExtensionList);

  return offset;
}
static int dissect_privateExtensionList_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_PrivateExtensionList(TRUE, tvb, offset, pinfo, tree, hf_camel_privateExtensionList);
}


static int
dissect_camel_ISDN_AddressString(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_gmscAddress_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_ISDN_AddressString(TRUE, tvb, offset, pinfo, tree, hf_camel_gmscAddress);
}
static int dissect_sgsn_Number_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_ISDN_AddressString(TRUE, tvb, offset, pinfo, tree, hf_camel_sgsn_Number);
}
static int dissect_vlr_number_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_ISDN_AddressString(TRUE, tvb, offset, pinfo, tree, hf_camel_vlr_number);
}
static int dissect_msc_number_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_ISDN_AddressString(TRUE, tvb, offset, pinfo, tree, hf_camel_msc_number);
}
static int dissect_callingPartysNumber_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_ISDN_AddressString(TRUE, tvb, offset, pinfo, tree, hf_camel_callingPartysNumber);
}
static int dissect_sMSCAddress_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_ISDN_AddressString(TRUE, tvb, offset, pinfo, tree, hf_camel_sMSCAddress);
}
static int dissect_mSISDN_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_ISDN_AddressString(TRUE, tvb, offset, pinfo, tree, hf_camel_mSISDN);
}
static int dissect_mscAddress_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_ISDN_AddressString(TRUE, tvb, offset, pinfo, tree, hf_camel_mscAddress);
}
static int dissect_callingPartyNumber1_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_ISDN_AddressString(TRUE, tvb, offset, pinfo, tree, hf_camel_callingPartyNumber1);
}
static int dissect_sgsnNumber_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_ISDN_AddressString(TRUE, tvb, offset, pinfo, tree, hf_camel_sgsnNumber);
}


static const value_string camel_CellIdOrLAI_vals[] = {
  {   0, "cellIdFixedLength" },
  {   1, "laiFixedLength" },
  { 0, NULL }
};

static const ber_choice_t CellIdOrLAI_choice[] = {
  {   0, BER_CLASS_CON, 0, 0, dissect_cellIdFixedLength_impl },
  {   1, BER_CLASS_CON, 1, 0, dissect_laiFixedLength_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_camel_CellIdOrLAI(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              CellIdOrLAI_choice, hf_index, ett_camel_CellIdOrLAI);

  return offset;
}
static int dissect_cellIdOrLAI_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_CellIdOrLAI(TRUE, tvb, offset, pinfo, tree, hf_camel_cellIdOrLAI);
}


static const value_string camel_Ext_BasicServiceCode_vals[] = {
  {   2, "ext-BearerService" },
  {   3, "ext-Teleservice" },
  { 0, NULL }
};

static const ber_choice_t Ext_BasicServiceCode_choice[] = {
  {   2, BER_CLASS_CON, 2, 0, dissect_ext_BearerService_impl },
  {   3, BER_CLASS_CON, 3, 0, dissect_ext_Teleservice_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_camel_Ext_BasicServiceCode(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              Ext_BasicServiceCode_choice, hf_index, ett_camel_Ext_BasicServiceCode);

  return offset;
}
static int dissect_ext_basicServiceCode_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_Ext_BasicServiceCode(TRUE, tvb, offset, pinfo, tree, hf_camel_ext_basicServiceCode);
}


static const value_string camel_LegID_vals[] = {
  {   0, "sendingSideID" },
  {   1, "receivingSideID" },
  { 0, NULL }
};

static const ber_choice_t LegID_choice[] = {
  {   0, BER_CLASS_CON, 0, 0, dissect_sendingSideID_impl },
  {   1, BER_CLASS_CON, 1, 0, dissect_receivingSideID_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_camel_LegID(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              LegID_choice, hf_index, ett_camel_LegID);

  return offset;
}
static int dissect_legID_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_LegID(TRUE, tvb, offset, pinfo, tree, hf_camel_legID);
}

static const ber_sequence_t AOCBeforeAnswer_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_aOCInitial_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_aOCSubsequent_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_AOCBeforeAnswer(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                AOCBeforeAnswer_sequence, hf_index, ett_camel_AOCBeforeAnswer);

  return offset;
}
static int dissect_aOCBeforeAnswer_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_AOCBeforeAnswer(TRUE, tvb, offset, pinfo, tree, hf_camel_aOCBeforeAnswer);
}

static const ber_sequence_t AOCGprs_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_aOCInitial_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_aOCSubsequent_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_AOCGprs(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                AOCGprs_sequence, hf_index, ett_camel_AOCGprs);

  return offset;
}
static int dissect_aOCGPRS_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_AOCGprs(TRUE, tvb, offset, pinfo, tree, hf_camel_aOCGPRS);
}

static const ber_sequence_t T_calledAddressAndService_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_calledAddressValue_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_serviceKey_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_T_calledAddressAndService(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                T_calledAddressAndService_sequence, hf_index, ett_camel_T_calledAddressAndService);

  return offset;
}
static int dissect_calledAddressAndService_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_T_calledAddressAndService(TRUE, tvb, offset, pinfo, tree, hf_camel_calledAddressAndService);
}

static const ber_sequence_t T_callingAddressAndService_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_callingAddressValue_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_serviceKey_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_T_callingAddressAndService(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                T_callingAddressAndService_sequence, hf_index, ett_camel_T_callingAddressAndService);

  return offset;
}
static int dissect_callingAddressAndService_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_T_callingAddressAndService(TRUE, tvb, offset, pinfo, tree, hf_camel_callingAddressAndService);
}


static const value_string camel_BasicGapCriteria_vals[] = {
  {   0, "calledAddressValue" },
  {   2, "gapOnService" },
  {  29, "calledAddressAndService" },
  {  30, "callingAddressAndService" },
  { 0, NULL }
};

static const ber_choice_t BasicGapCriteria_choice[] = {
  {   0, BER_CLASS_CON, 0, 0, dissect_calledAddressValue_impl },
  {   2, BER_CLASS_CON, 2, 0, dissect_gapOnService_impl },
  {  29, BER_CLASS_CON, 29, 0, dissect_calledAddressAndService_impl },
  {  30, BER_CLASS_CON, 30, 0, dissect_callingAddressAndService_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_camel_BasicGapCriteria(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              BasicGapCriteria_choice, hf_index, ett_camel_BasicGapCriteria);

  return offset;
}
static int dissect_basicGapCriteria(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_BasicGapCriteria(FALSE, tvb, offset, pinfo, tree, hf_camel_basicGapCriteria);
}
static int dissect_basicGapCriteria_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_BasicGapCriteria(TRUE, tvb, offset, pinfo, tree, hf_camel_basicGapCriteria);
}

static const ber_sequence_t BCSMEvent_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_eventTypeBCSM_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_monitorMode_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_legID_impl },
  { BER_CLASS_CON, 30, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_dpSpecificCriteria_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_BCSMEvent(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                BCSMEvent_sequence, hf_index, ett_camel_BCSMEvent);

  return offset;
}
static int dissect_BCSMEventArray_item(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_BCSMEvent(FALSE, tvb, offset, pinfo, tree, hf_camel_BCSMEventArray_item);
}

static const ber_sequence_t T_fCIBCCCAMELsequence12_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_freeFormatData_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_partyToCharge_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_appendFreeFormatData_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_T_fCIBCCCAMELsequence12(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                T_fCIBCCCAMELsequence12_sequence, hf_index, ett_camel_T_fCIBCCCAMELsequence12);

  return offset;
}
static int dissect_fCIBCCCAMELsequence12_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_T_fCIBCCCAMELsequence12(TRUE, tvb, offset, pinfo, tree, hf_camel_fCIBCCCAMELsequence12);
}


static const value_string camel_CamelFCIBillingChargingCharacteristics_vals[] = {
  {   0, "fCIBCCCAMELsequence1" },
  { 0, NULL }
};

static const ber_choice_t CamelFCIBillingChargingCharacteristics_choice[] = {
  {   0, BER_CLASS_CON, 0, 0, dissect_fCIBCCCAMELsequence12_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_camel_CamelFCIBillingChargingCharacteristics(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              CamelFCIBillingChargingCharacteristics_choice, hf_index, ett_camel_CamelFCIBillingChargingCharacteristics);

  return offset;
}

static const ber_sequence_t CompoundCriteria_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_basicGapCriteria_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_scfID_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_CompoundCriteria(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                CompoundCriteria_sequence, hf_index, ett_camel_CompoundCriteria);

  return offset;
}
static int dissect_compoundGapCriteria(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_CompoundCriteria(FALSE, tvb, offset, pinfo, tree, hf_camel_compoundGapCriteria);
}


static const value_string camel_GapCriteria_vals[] = {
  {   0, "basicGapCriteria" },
  {   1, "compoundGapCriteria" },
  { 0, NULL }
};

static const ber_choice_t GapCriteria_choice[] = {
  {   0, BER_CLASS_CON, -1/*choice*/, BER_FLAGS_NOOWNTAG, dissect_basicGapCriteria },
  {   1, BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_compoundGapCriteria },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_camel_GapCriteria(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              GapCriteria_choice, hf_index, ett_camel_GapCriteria);

  return offset;
}
static int dissect_gapCriteria_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_GapCriteria(TRUE, tvb, offset, pinfo, tree, hf_camel_gapCriteria);
}


static int
dissect_camel_IA5String_SIZE_1_127(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_restricted_string(implicit_tag, 0,
                                         pinfo, tree, tvb, offset, hf_index,
                                         NULL);

  return offset;
}
static int dissect_messageContent_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_IA5String_SIZE_1_127(TRUE, tvb, offset, pinfo, tree, hf_camel_messageContent);
}


static int
dissect_camel_OCTET_STRING_SIZE_2_10(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_attributes_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_OCTET_STRING_SIZE_2_10(TRUE, tvb, offset, pinfo, tree, hf_camel_attributes);
}

static const ber_sequence_t T_text_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_messageContent_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_attributes_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_T_text(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                T_text_sequence, hf_index, ett_camel_T_text);

  return offset;
}
static int dissect_text_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_T_text(TRUE, tvb, offset, pinfo, tree, hf_camel_text);
}

static const ber_sequence_t SEQUENCE_SIZE_1_16_OF_Integer4_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_elementaryMessageIDs_item },
};

static int
dissect_camel_SEQUENCE_SIZE_1_16_OF_Integer4(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, pinfo, tree, tvb, offset,
                                   SEQUENCE_SIZE_1_16_OF_Integer4_sequence_of, hf_index, ett_camel_SEQUENCE_SIZE_1_16_OF_Integer4);

  return offset;
}
static int dissect_elementaryMessageIDs_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_SEQUENCE_SIZE_1_16_OF_Integer4(TRUE, tvb, offset, pinfo, tree, hf_camel_elementaryMessageIDs);
}

static const ber_sequence_t VariablePartsArray_sequence_of[1] = {
  { BER_CLASS_CON, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_VariablePartsArray_item },
};

static int
dissect_camel_VariablePartsArray(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, pinfo, tree, tvb, offset,
                                   VariablePartsArray_sequence_of, hf_index, ett_camel_VariablePartsArray);

  return offset;
}
static int dissect_variableParts_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_VariablePartsArray(TRUE, tvb, offset, pinfo, tree, hf_camel_variableParts);
}

static const ber_sequence_t T_variableMessage_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_elementaryMessageID_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_variableParts_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_T_variableMessage(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                T_variableMessage_sequence, hf_index, ett_camel_T_variableMessage);

  return offset;
}
static int dissect_variableMessage_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_T_variableMessage(TRUE, tvb, offset, pinfo, tree, hf_camel_variableMessage);
}


static const value_string camel_MessageID_vals[] = {
  {   0, "elementaryMessageID" },
  {   1, "text" },
  {  29, "elementaryMessageIDs" },
  {  30, "variableMessage" },
  { 0, NULL }
};

static const ber_choice_t MessageID_choice[] = {
  {   0, BER_CLASS_CON, 0, 0, dissect_elementaryMessageID_impl },
  {   1, BER_CLASS_CON, 1, 0, dissect_text_impl },
  {  29, BER_CLASS_CON, 29, 0, dissect_elementaryMessageIDs_impl },
  {  30, BER_CLASS_CON, 30, 0, dissect_variableMessage_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_camel_MessageID(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              MessageID_choice, hf_index, ett_camel_MessageID);

  return offset;
}
static int dissect_messageID_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_MessageID(TRUE, tvb, offset, pinfo, tree, hf_camel_messageID);
}

static const ber_sequence_t RequestedInformation_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_requestedInformationType_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_requestedInformationValue_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_RequestedInformation(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                RequestedInformation_sequence, hf_index, ett_camel_RequestedInformation);

  return offset;
}
static int dissect_RequestedInformationList_item(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_RequestedInformation(FALSE, tvb, offset, pinfo, tree, hf_camel_RequestedInformationList_item);
}

static const ber_sequence_t InitialDPArgExtension_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_gmscAddress_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_InitialDPArgExtension(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                InitialDPArgExtension_sequence, hf_index, ett_camel_InitialDPArgExtension);

  return offset;
}
static int dissect_initialDPArgExtension_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_InitialDPArgExtension(TRUE, tvb, offset, pinfo, tree, hf_camel_initialDPArgExtension);
}



static int
dissect_camel_INTEGER_0_32767(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_duration2_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER_0_32767(TRUE, tvb, offset, pinfo, tree, hf_camel_duration2);
}
static int dissect_interval_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_INTEGER_0_32767(TRUE, tvb, offset, pinfo, tree, hf_camel_interval);
}

static const ber_sequence_t InbandInfo_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_messageID_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_numberOfRepetitions_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_duration2_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_interval_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_InbandInfo(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                InbandInfo_sequence, hf_index, ett_camel_InbandInfo);

  return offset;
}
static int dissect_inbandInfo_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_InbandInfo(TRUE, tvb, offset, pinfo, tree, hf_camel_inbandInfo);
}


static const value_string camel_InformationToSend_vals[] = {
  {   0, "inbandInfo" },
  {   1, "tone" },
  { 0, NULL }
};

static const ber_choice_t InformationToSend_choice[] = {
  {   0, BER_CLASS_CON, 0, 0, dissect_inbandInfo_impl },
  {   1, BER_CLASS_CON, 1, 0, dissect_tone_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_camel_InformationToSend(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              InformationToSend_choice, hf_index, ett_camel_InformationToSend);

  return offset;
}
static int dissect_informationToSend_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_InformationToSend(TRUE, tvb, offset, pinfo, tree, hf_camel_informationToSend);
}


static int
dissect_camel_CellGlobalIdOrServiceAreaIdOrLAI(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_cellGlobalIdOrServiceAreaIdOrLAI_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_CellGlobalIdOrServiceAreaIdOrLAI(TRUE, tvb, offset, pinfo, tree, hf_camel_cellGlobalIdOrServiceAreaIdOrLAI);
}

static const ber_sequence_t ExtensionContainer_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_privateExtensionList_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_pcs_Extensions_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_ExtensionContainer(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                ExtensionContainer_sequence, hf_index, ett_camel_ExtensionContainer);

  return offset;
}
static int dissect_extensionContainer_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_ExtensionContainer(TRUE, tvb, offset, pinfo, tree, hf_camel_extensionContainer);
}

static const ber_sequence_t LocationInformationGPRS_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_cellGlobalIdOrServiceAreaIdOrLAI_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_routeingAreaIdentity_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_geographicalInformation_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_sgsn_Number_impl },
  { BER_CLASS_CON, 4, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_selectedLSAIdentity_impl },
  { BER_CLASS_CON, 5, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_extensionContainer_impl },
  { BER_CLASS_CON, 6, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_saiPresent_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_LocationInformationGPRS(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                LocationInformationGPRS_sequence, hf_index, ett_camel_LocationInformationGPRS);

  return offset;
}
static int dissect_locationInformationGPRS_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_LocationInformationGPRS(TRUE, tvb, offset, pinfo, tree, hf_camel_locationInformationGPRS);
}

static const ber_sequence_t RequestedInformationList_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_RequestedInformationList_item },
};

static int
dissect_camel_RequestedInformationList(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, pinfo, tree, tvb, offset,
                                   RequestedInformationList_sequence_of, hf_index, ett_camel_RequestedInformationList);

  return offset;
}
static int dissect_requestedInformationList_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_RequestedInformationList(TRUE, tvb, offset, pinfo, tree, hf_camel_requestedInformationList);
}


static int
dissect_camel_GeodeticInformation(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_geodeticInformation_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_GeodeticInformation(TRUE, tvb, offset, pinfo, tree, hf_camel_geodeticInformation);
}

static const ber_sequence_t LocationInformation_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_ageOfLocationInformation },
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_geographicalInformation_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_vlr_number_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_locationNumber_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_cellIdOrLAI_impl },
  { BER_CLASS_CON, 4, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_extensionContainer_impl },
  { BER_CLASS_CON, 5, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_selectedLSA_Id_impl },
  { BER_CLASS_CON, 6, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_msc_number_impl },
  { BER_CLASS_CON, 7, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_geodeticInformation_impl },
  { BER_CLASS_CON, 8, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_currentLocationRetrieved_impl },
  { BER_CLASS_CON, 9, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_saiPresent_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_LocationInformation(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                LocationInformation_sequence, hf_index, ett_camel_LocationInformation);

  return offset;
}
static int dissect_locationInformation_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_LocationInformation(TRUE, tvb, offset, pinfo, tree, hf_camel_locationInformation);
}
static int dissect_locationInformationMSC_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_LocationInformation(TRUE, tvb, offset, pinfo, tree, hf_camel_locationInformationMSC);
}


static const value_string camel_GapTreatment_vals[] = {
  {   0, "informationToSend" },
  {   1, "releaseCause" },
  { 0, NULL }
};

static const ber_choice_t GapTreatment_choice[] = {
  {   0, BER_CLASS_CON, 0, 0, dissect_informationToSend_impl },
  {   1, BER_CLASS_CON, 1, 0, dissect_releaseCause_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_camel_GapTreatment(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              GapTreatment_choice, hf_index, ett_camel_GapTreatment);

  return offset;
}
static int dissect_gapTreatment_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_GapTreatment(TRUE, tvb, offset, pinfo, tree, hf_camel_gapTreatment);
}

static const ber_sequence_t T_attachChangeOfPositionSpecificInformation_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_locationInformationGPRS_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_T_attachChangeOfPositionSpecificInformation(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                T_attachChangeOfPositionSpecificInformation_sequence, hf_index, ett_camel_T_attachChangeOfPositionSpecificInformation);

  return offset;
}
static int dissect_attachChangeOfPositionSpecificInformation_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_T_attachChangeOfPositionSpecificInformation(TRUE, tvb, offset, pinfo, tree, hf_camel_attachChangeOfPositionSpecificInformation);
}

static const ber_sequence_t T_pdp_ContextchangeOfPositionSpecificInformation_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_accessPointName_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_chargingID_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_locationInformationGPRS_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_pDPType_impl },
  { BER_CLASS_CON, 4, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_qualityOfService_impl },
  { BER_CLASS_CON, 5, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_timeAndTimeZone_impl },
  { BER_CLASS_CON, 6, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_gGSNAddress_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_T_pdp_ContextchangeOfPositionSpecificInformation(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                T_pdp_ContextchangeOfPositionSpecificInformation_sequence, hf_index, ett_camel_T_pdp_ContextchangeOfPositionSpecificInformation);

  return offset;
}
static int dissect_pdp_ContextchangeOfPositionSpecificInformation_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_T_pdp_ContextchangeOfPositionSpecificInformation(TRUE, tvb, offset, pinfo, tree, hf_camel_pdp_ContextchangeOfPositionSpecificInformation);
}

static const ber_sequence_t T_detachSpecificInformation_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_inititatingEntity_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_routeingAreaUpdate_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_T_detachSpecificInformation(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                T_detachSpecificInformation_sequence, hf_index, ett_camel_T_detachSpecificInformation);

  return offset;
}
static int dissect_detachSpecificInformation_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_T_detachSpecificInformation(TRUE, tvb, offset, pinfo, tree, hf_camel_detachSpecificInformation);
}

static const ber_sequence_t T_disconnectSpecificInformation_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_inititatingEntity_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_routeingAreaUpdate_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_T_disconnectSpecificInformation(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                T_disconnectSpecificInformation_sequence, hf_index, ett_camel_T_disconnectSpecificInformation);

  return offset;
}
static int dissect_disconnectSpecificInformation_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_T_disconnectSpecificInformation(TRUE, tvb, offset, pinfo, tree, hf_camel_disconnectSpecificInformation);
}

static const ber_sequence_t T_pDPContextEstablishmentSpecificInformation_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_accessPointName_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_pDPType_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_qualityOfService_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_locationInformationGPRS_impl },
  { BER_CLASS_CON, 4, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_timeAndTimeZone_impl },
  { BER_CLASS_CON, 5, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_pDPInitiationType_impl },
  { BER_CLASS_CON, 6, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_secondaryPDPContext_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_T_pDPContextEstablishmentSpecificInformation(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                T_pDPContextEstablishmentSpecificInformation_sequence, hf_index, ett_camel_T_pDPContextEstablishmentSpecificInformation);

  return offset;
}
static int dissect_pDPContextEstablishmentSpecificInformation_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_T_pDPContextEstablishmentSpecificInformation(TRUE, tvb, offset, pinfo, tree, hf_camel_pDPContextEstablishmentSpecificInformation);
}

static const ber_sequence_t T_pDPContextEstablishmentAcknowledgementSpecificInformation_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_accessPointName_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_chargingID_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_pDPType_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_qualityOfService_impl },
  { BER_CLASS_CON, 4, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_locationInformationGPRS_impl },
  { BER_CLASS_CON, 5, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_timeAndTimeZone_impl },
  { BER_CLASS_CON, 6, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_gGSNAddress_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_T_pDPContextEstablishmentAcknowledgementSpecificInformation(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                T_pDPContextEstablishmentAcknowledgementSpecificInformation_sequence, hf_index, ett_camel_T_pDPContextEstablishmentAcknowledgementSpecificInformation);

  return offset;
}
static int dissect_pDPContextEstablishmentAcknowledgementSpecificInformation_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_T_pDPContextEstablishmentAcknowledgementSpecificInformation(TRUE, tvb, offset, pinfo, tree, hf_camel_pDPContextEstablishmentAcknowledgementSpecificInformation);
}


static const value_string camel_GPRSEventSpecificInformation_vals[] = {
  {   0, "attachChangeOfPositionSpecificInformation" },
  {   1, "pdp-ContextchangeOfPositionSpecificInformation" },
  {   2, "detachSpecificInformation" },
  {   3, "disconnectSpecificInformation" },
  {   4, "pDPContextEstablishmentSpecificInformation" },
  {   5, "pDPContextEstablishmentAcknowledgementSpecificInformation" },
  { 0, NULL }
};

static const ber_choice_t GPRSEventSpecificInformation_choice[] = {
  {   0, BER_CLASS_CON, 0, 0, dissect_attachChangeOfPositionSpecificInformation_impl },
  {   1, BER_CLASS_CON, 1, 0, dissect_pdp_ContextchangeOfPositionSpecificInformation_impl },
  {   2, BER_CLASS_CON, 2, 0, dissect_detachSpecificInformation_impl },
  {   3, BER_CLASS_CON, 3, 0, dissect_disconnectSpecificInformation_impl },
  {   4, BER_CLASS_CON, 4, 0, dissect_pDPContextEstablishmentSpecificInformation_impl },
  {   5, BER_CLASS_CON, 5, 0, dissect_pDPContextEstablishmentAcknowledgementSpecificInformation_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_camel_GPRSEventSpecificInformation(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              GPRSEventSpecificInformation_choice, hf_index, ett_camel_GPRSEventSpecificInformation);

  return offset;
}
static int dissect_gPRSEventSpecificInformation_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_GPRSEventSpecificInformation(TRUE, tvb, offset, pinfo, tree, hf_camel_gPRSEventSpecificInformation);
}

static const ber_sequence_t EventReportGPRSArg_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_gPRSEventType_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_miscGPRSInfo_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_gPRSEventSpecificInformation_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_pDPID_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_EventReportGPRSArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                EventReportGPRSArg_sequence, hf_index, ett_camel_EventReportGPRSArg);

  return offset;
}


static int
dissect_camel_SupportedExtensionsid(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_camel_Code(implicit_tag, tvb, offset, pinfo, tree, hf_index);

  return offset;
}
static int dissect_type(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_SupportedExtensionsid(FALSE, tvb, offset, pinfo, tree, hf_camel_type);
}

static const ber_sequence_t ExtensionField_sequence[] = {
  { BER_CLASS_UNI, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_type },
  { BER_CLASS_UNI, BER_UNI_TAG_ENUMERATED, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_criticality },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_value_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_ExtensionField(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                ExtensionField_sequence, hf_index, ett_camel_ExtensionField);

  return offset;
}
static int dissect_ExtensionsArray_item(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_ExtensionField(FALSE, tvb, offset, pinfo, tree, hf_camel_ExtensionsArray_item);
}

static const ber_sequence_t ExtensionsArray_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_ExtensionsArray_item },
};

static int
dissect_camel_ExtensionsArray(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, pinfo, tree, tvb, offset,
                                   ExtensionsArray_sequence_of, hf_index, ett_camel_ExtensionsArray);

  return offset;
}
static int dissect_extensions_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_ExtensionsArray(TRUE, tvb, offset, pinfo, tree, hf_camel_extensions);
}

static const ber_sequence_t ApplyChargingArg_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_aChBillingChargingCharacteristics_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_partyToCharge_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_extensions_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_ApplyChargingArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                ApplyChargingArg_sequence, hf_index, ett_camel_ApplyChargingArg);

  return offset;
}

static const ber_sequence_t AssistRequestInstructionsArg_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_correlationID_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_IMPLTAG, dissect_iPSSPCapabilities_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_extensions_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_AssistRequestInstructionsArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                AssistRequestInstructionsArg_sequence, hf_index, ett_camel_AssistRequestInstructionsArg);

  return offset;
}

static const ber_sequence_t CallInformationRequestArg_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_requestedInformationTypeList_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_extensions_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_legID1_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_CallInformationRequestArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                CallInformationRequestArg_sequence, hf_index, ett_camel_CallInformationRequestArg);

  return offset;
}

static const ber_sequence_t ConnectArg_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_destinationRoutingAddress_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_alertingPattern_impl },
  { BER_CLASS_CON, 6, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_originalCalledPartyID_impl },
  { BER_CLASS_CON, 10, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_extensions_impl },
  { BER_CLASS_CON, 11, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_carrier_impl },
  { BER_CLASS_CON, 28, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_callingPartysCategory_impl },
  { BER_CLASS_CON, 29, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_redirectingPartyID_impl },
  { BER_CLASS_CON, 30, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_redirectionInformation_impl },
  { BER_CLASS_CON, 14, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_genericNumbers_impl },
  { BER_CLASS_CON, 15, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_serviceInteractionIndicatorsTwo_impl },
  { BER_CLASS_CON, 19, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_chargeNumber_impl },
  { BER_CLASS_CON, 31, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_cug_Interlock_impl },
  { BER_CLASS_CON, 32, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_cug_OutgoingAccess_impl },
  { BER_CLASS_CON, 55, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_suppressionOfAnnouncement_impl },
  { BER_CLASS_CON, 56, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_oCSIApplicable_impl },
  { BER_CLASS_CON, 57, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_naOliInfo_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_ConnectArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                ConnectArg_sequence, hf_index, ett_camel_ConnectArg);

  return offset;
}


static const value_string camel_T_resourceAddress_vals[] = {
  {   0, "ipRoutingAddress" },
  {   3, "none" },
  { 0, NULL }
};

static const ber_choice_t T_resourceAddress_choice[] = {
  {   0, BER_CLASS_CON, 0, 0, dissect_ipRoutingAddress_impl },
  {   3, BER_CLASS_CON, 3, 0, dissect_none_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_camel_T_resourceAddress(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              T_resourceAddress_choice, hf_index, ett_camel_T_resourceAddress);

  return offset;
}
static int dissect_resourceAddress(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_T_resourceAddress(FALSE, tvb, offset, pinfo, tree, hf_camel_resourceAddress);
}

static const ber_sequence_t ConnectToResourceArg_sequence[] = {
  { BER_CLASS_CON, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_resourceAddress },
  { BER_CLASS_CON, 4, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_extensions_impl },
  { BER_CLASS_CON, 7, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_serviceInteractionIndicatorsTwo_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_ConnectToResourceArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                ConnectToResourceArg_sequence, hf_index, ett_camel_ConnectToResourceArg);

  return offset;
}

static const ber_sequence_t ContinueWithArgumentArg_sequence[] = {
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_alertingPattern_impl },
  { BER_CLASS_CON, 6, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_extensions_impl },
  { BER_CLASS_CON, 7, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_serviceInteractionIndicatorsTwo_impl },
  { BER_CLASS_CON, 12, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_callingPartysCategory_impl },
  { BER_CLASS_CON, 16, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_genericNumbers_impl },
  { BER_CLASS_CON, 17, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_cug_Interlock_impl },
  { BER_CLASS_CON, 18, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_cug_OutgoingAccess_impl },
  { BER_CLASS_CON, 50, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_chargeNumber_impl },
  { BER_CLASS_CON, 52, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_carrier_impl },
  { BER_CLASS_CON, 55, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_suppressionOfAnnouncement_impl },
  { BER_CLASS_CON, 56, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_naOliInfo_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_ContinueWithArgumentArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                ContinueWithArgumentArg_sequence, hf_index, ett_camel_ContinueWithArgumentArg);

  return offset;
}

static const ber_sequence_t EstablishTemporaryConnectionArg_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_assistingSSPIPRoutingAddress_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_correlationID_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_scfID_impl },
  { BER_CLASS_CON, 4, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_extensions_impl },
  { BER_CLASS_CON, 5, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_carrier_impl },
  { BER_CLASS_CON, 6, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_serviceInteractionIndicatorsTwo_impl },
  { BER_CLASS_CON, 50, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_naOliInfo_impl },
  { BER_CLASS_CON, 51, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_chargeNumber_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_EstablishTemporaryConnectionArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                EstablishTemporaryConnectionArg_sequence, hf_index, ett_camel_EstablishTemporaryConnectionArg);

  return offset;
}

static const ber_sequence_t EventReportBCSMArg_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_eventTypeBCSM_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_eventSpecificInformationBCSM_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_legID2_impl },
  { BER_CLASS_CON, 4, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_miscCallInfo_impl },
  { BER_CLASS_CON, 5, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_extensions_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_EventReportBCSMArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                EventReportBCSMArg_sequence, hf_index, ett_camel_EventReportBCSMArg);

  return offset;
}

static const ber_sequence_t ResetTimerArg_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_timerID_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_timervalue_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_extensions_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_ResetTimerArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                ResetTimerArg_sequence, hf_index, ett_camel_ResetTimerArg);

  return offset;
}

static const ber_sequence_t SendChargingInformationArg_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_sCIBillingChargingCharacteristics_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_partyToCharge_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_extensions_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_SendChargingInformationArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                SendChargingInformationArg_sequence, hf_index, ett_camel_SendChargingInformationArg);

  return offset;
}

static const ber_sequence_t EventReportSMSArg_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_eventTypeSMS_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_eventSpecificInformationSMS_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_miscCallInfo_impl },
  { BER_CLASS_CON, 10, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_extensions_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_EventReportSMSArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                EventReportSMSArg_sequence, hf_index, ett_camel_EventReportSMSArg);

  return offset;
}

static const ber_sequence_t SMSEventArray_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_SMSEventArray_item },
};

static int
dissect_camel_SMSEventArray(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, pinfo, tree, tvb, offset,
                                   SMSEventArray_sequence_of, hf_index, ett_camel_SMSEventArray);

  return offset;
}
static int dissect_sMSEvents_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_SMSEventArray(TRUE, tvb, offset, pinfo, tree, hf_camel_sMSEvents);
}

static const ber_sequence_t RequestReportSMSEventArg_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_sMSEvents_impl },
  { BER_CLASS_CON, 10, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_extensions_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_RequestReportSMSEventArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                RequestReportSMSEventArg_sequence, hf_index, ett_camel_RequestReportSMSEventArg);

  return offset;
}

static const ber_sequence_t ResetTimerSMSArg_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_timerID_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_timervalue_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_extensions_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_ResetTimerSMSArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                ResetTimerSMSArg_sequence, hf_index, ett_camel_ResetTimerSMSArg);

  return offset;
}

static const ber_sequence_t T_timeDurationCharging_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_maxCallPeriodDuration_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_releaseIfdurationExceeded_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_tariffSwitchInterval1_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_tone1_impl },
  { BER_CLASS_CON, 4, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_extensions_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_T_timeDurationCharging(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                T_timeDurationCharging_sequence, hf_index, ett_camel_T_timeDurationCharging);

  return offset;
}
static int dissect_timeDurationCharging_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_T_timeDurationCharging(TRUE, tvb, offset, pinfo, tree, hf_camel_timeDurationCharging);
}


static const value_string camel_CamelAChBillingChargingCharacteristics_vals[] = {
  {   0, "timeDurationCharging" },
  { 0, NULL }
};

static const ber_choice_t CamelAChBillingChargingCharacteristics_choice[] = {
  {   0, BER_CLASS_CON, 0, 0, dissect_timeDurationCharging_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_camel_CamelAChBillingChargingCharacteristics(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              CamelAChBillingChargingCharacteristics_choice, hf_index, ett_camel_CamelAChBillingChargingCharacteristics);

  return offset;
}

static const ber_sequence_t T_timeDurationChargingResult_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_partyToCharge1_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_timeInformation_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_callActive_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_callReleasedAtTcpExpiry_impl },
  { BER_CLASS_CON, 4, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_extensions_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_T_timeDurationChargingResult(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                T_timeDurationChargingResult_sequence, hf_index, ett_camel_T_timeDurationChargingResult);

  return offset;
}
static int dissect_timeDurationChargingResult_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_T_timeDurationChargingResult(TRUE, tvb, offset, pinfo, tree, hf_camel_timeDurationChargingResult);
}


static const value_string camel_CamelCallResult_vals[] = {
  {   0, "timeDurationChargingResult" },
  { 0, NULL }
};

static const ber_choice_t CamelCallResult_choice[] = {
  {   0, BER_CLASS_CON, 0, 0, dissect_timeDurationChargingResult_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_camel_CamelCallResult(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              CamelCallResult_choice, hf_index, ett_camel_CamelCallResult);

  return offset;
}

static const ber_sequence_t BCSMEventArray_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_BCSMEventArray_item },
};

static int
dissect_camel_BCSMEventArray(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, pinfo, tree, tvb, offset,
                                   BCSMEventArray_sequence_of, hf_index, ett_camel_BCSMEventArray);

  return offset;
}
static int dissect_bcsmEvents_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_camel_BCSMEventArray(TRUE, tvb, offset, pinfo, tree, hf_camel_bcsmEvents);
}

static const ber_sequence_t RequestReportBCSMEventArg_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_bcsmEvents_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_extensions_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_RequestReportBCSMEventArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                RequestReportBCSMEventArg_sequence, hf_index, ett_camel_RequestReportBCSMEventArg);

  return offset;
}

static const ber_sequence_t ConnectSMSArg_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_callingPartysNumber_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_destinationSubscriberNumber_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_sMSCAddress_impl },
  { BER_CLASS_CON, 10, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_extensions_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_ConnectSMSArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                ConnectSMSArg_sequence, hf_index, ett_camel_ConnectSMSArg);

  return offset;
}

static const ber_sequence_t CallInformationReportArg_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_requestedInformationList_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_extensions_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_legID2_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_CallInformationReportArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                CallInformationReportArg_sequence, hf_index, ett_camel_CallInformationReportArg);

  return offset;
}

static const ber_sequence_t PlayAnnouncementArg_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_informationToSend_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_disconnectFromIPForbidden_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_requestAnnouncementComplete_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_extensions_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_PlayAnnouncementArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                PlayAnnouncementArg_sequence, hf_index, ett_camel_PlayAnnouncementArg);

  return offset;
}

static const ber_sequence_t PromptAndCollectUserInformationArg_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_collectedInfo_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_disconnectFromIPForbidden_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_informationToSend_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_extensions_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_PromptAndCollectUserInformationArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                PromptAndCollectUserInformationArg_sequence, hf_index, ett_camel_PromptAndCollectUserInformationArg);

  return offset;
}

static const ber_sequence_t InitialDPGPRSArg_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_serviceKey_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_gPRSEventType_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_IMPLTAG, dissect_mSISDN_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_IMPLTAG, dissect_iMSI_impl },
  { BER_CLASS_CON, 4, BER_FLAGS_IMPLTAG, dissect_timeAndTimeZone_impl },
  { BER_CLASS_CON, 5, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_gPRSMSClass_impl },
  { BER_CLASS_CON, 6, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_pDPType_impl },
  { BER_CLASS_CON, 7, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_qualityOfService_impl },
  { BER_CLASS_CON, 8, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_accessPointName_impl },
  { BER_CLASS_CON, 9, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_routeingAreaIdentity_impl },
  { BER_CLASS_CON, 10, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_chargingID_impl },
  { BER_CLASS_CON, 11, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_sGSNCapabilities_impl },
  { BER_CLASS_CON, 12, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_locationInformationGPRS_impl },
  { BER_CLASS_CON, 13, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_pDPInitiationType_impl },
  { BER_CLASS_CON, 14, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_extensions_impl },
  { BER_CLASS_CON, 15, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_gGSNAddress_impl },
  { BER_CLASS_CON, 16, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_secondaryPDPContext_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_InitialDPGPRSArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                InitialDPGPRSArg_sequence, hf_index, ett_camel_InitialDPGPRSArg);

  return offset;
}

static const ber_sequence_t CallGapArg_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_gapCriteria_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_gapIndicators_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_controlType_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_gapTreatment_impl },
  { BER_CLASS_CON, 4, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_extensions_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_CallGapArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                CallGapArg_sequence, hf_index, ett_camel_CallGapArg);

  return offset;
}

static const ber_sequence_t InitialDPArg_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_serviceKey_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_calledPartyNumber_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_callingPartyNumber_impl },
  { BER_CLASS_CON, 5, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_callingPartysCategory_impl },
  { BER_CLASS_CON, 7, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_cGEncountered_impl },
  { BER_CLASS_CON, 8, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_iPSSPCapabilities_impl },
  { BER_CLASS_CON, 10, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_locationNumber_impl },
  { BER_CLASS_CON, 12, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_originalCalledPartyID_impl },
  { BER_CLASS_CON, 15, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_extensions_impl },
  { BER_CLASS_CON, 23, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_highLayerCompatibility_impl },
  { BER_CLASS_CON, 25, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_additionalCallingPartyNumber_impl },
  { BER_CLASS_CON, 27, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_bearerCapability_impl },
  { BER_CLASS_CON, 28, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_eventTypeBCSM_impl },
  { BER_CLASS_CON, 29, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_redirectingPartyID_impl },
  { BER_CLASS_CON, 30, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_redirectionInformation_impl },
  { BER_CLASS_CON, 17, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_cause_impl },
  { BER_CLASS_CON, 32, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_serviceInteractionIndicatorsTwo_impl },
  { BER_CLASS_CON, 37, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_carrier_impl },
  { BER_CLASS_CON, 45, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_cug_Index_impl },
  { BER_CLASS_CON, 46, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_cug_Interlock_impl },
  { BER_CLASS_CON, 47, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_cug_OutgoingAccess_impl },
  { BER_CLASS_CON, 50, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_iMSI_impl },
  { BER_CLASS_CON, 51, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_subscriberState_impl },
  { BER_CLASS_CON, 52, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_locationInformation_impl },
  { BER_CLASS_CON, 53, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_ext_basicServiceCode_impl },
  { BER_CLASS_CON, 54, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_callReferenceNumber_impl },
  { BER_CLASS_CON, 55, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_mscAddress_impl },
  { BER_CLASS_CON, 56, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_calledPartyBCDNumber_impl },
  { BER_CLASS_CON, 57, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_timeAndTimezone_impl },
  { BER_CLASS_CON, 58, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_gsm_ForwardingPending_impl },
  { BER_CLASS_CON, 59, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_initialDPArgExtension_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_InitialDPArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                InitialDPArg_sequence, hf_index, ett_camel_InitialDPArg);

  return offset;
}

static const ber_sequence_t InitialDPSMSArg_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_serviceKey_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_destinationSubscriberNumber_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_callingPartyNumber1_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_eventTypeSMS_impl },
  { BER_CLASS_CON, 4, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_iMSI_impl },
  { BER_CLASS_CON, 5, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_locationInformationMSC_impl },
  { BER_CLASS_CON, 6, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_locationInformationGPRS_impl },
  { BER_CLASS_CON, 7, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_sMSCAddress_impl },
  { BER_CLASS_CON, 8, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_timeAndTimezone_impl },
  { BER_CLASS_CON, 9, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_tPShortMessageSubmissionSpecificInfo_impl },
  { BER_CLASS_CON, 10, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_tPProtocolIdentifier_impl },
  { BER_CLASS_CON, 11, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_tPDataCodingScheme_impl },
  { BER_CLASS_CON, 12, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_tPValidityPeriod_impl },
  { BER_CLASS_CON, 13, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_extensions_impl },
  { BER_CLASS_CON, 14, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_smsReferenceNumber_impl },
  { BER_CLASS_CON, 15, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_mscAddress_impl },
  { BER_CLASS_CON, 16, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_sgsnNumber_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_InitialDPSMSArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                InitialDPSMSArg_sequence, hf_index, ett_camel_InitialDPSMSArg);

  return offset;
}


static const value_string camel_CamelSCIBillingChargingCharacteristics_vals[] = {
  {   0, "aOCBeforeAnswer" },
  {   1, "aOCAfterAnswer" },
  { 0, NULL }
};

static const ber_choice_t CamelSCIBillingChargingCharacteristics_choice[] = {
  {   0, BER_CLASS_CON, 0, 0, dissect_aOCBeforeAnswer_impl },
  {   1, BER_CLASS_CON, 1, 0, dissect_aOCAfterAnswer_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_camel_CamelSCIBillingChargingCharacteristics(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              CamelSCIBillingChargingCharacteristics_choice, hf_index, ett_camel_CamelSCIBillingChargingCharacteristics);

  return offset;
}

static const ber_sequence_t CamelSCIGPRSBillingChargingCharacteristics_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_aOCGPRS_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_pDPID_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_CamelSCIGPRSBillingChargingCharacteristics(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                CamelSCIGPRSBillingChargingCharacteristics_sequence, hf_index, ett_camel_CamelSCIGPRSBillingChargingCharacteristics);

  return offset;
}


static int
dissect_camel_ReleaseSMSArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}

static const ber_sequence_t ApplyChargingGPRSArg_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_chargingCharacteristics_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_tariffSwitchInterval1_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_pDPID_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_ApplyChargingGPRSArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                ApplyChargingGPRSArg_sequence, hf_index, ett_camel_ApplyChargingGPRSArg);

  return offset;
}

static const ber_sequence_t ApplyChargingReportGPRSArg_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_chargingResult_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_qualityOfService_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_active_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_pDPID_impl },
  { BER_CLASS_CON, 4, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_chargingRollOver_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_ApplyChargingReportGPRSArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                ApplyChargingReportGPRSArg_sequence, hf_index, ett_camel_ApplyChargingReportGPRSArg);

  return offset;
}

static const ber_sequence_t CancelGPRSArg_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_pDPID_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_CancelGPRSArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                CancelGPRSArg_sequence, hf_index, ett_camel_CancelGPRSArg);

  return offset;
}

static const ber_sequence_t ContinueGPRSArg_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_pDPID_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_ContinueGPRSArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                ContinueGPRSArg_sequence, hf_index, ett_camel_ContinueGPRSArg);

  return offset;
}

static const ber_sequence_t ResetTimerGPRSArg_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_timerID_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_timervalue_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_camel_ResetTimerGPRSArg(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                ResetTimerGPRSArg_sequence, hf_index, ett_camel_ResetTimerGPRSArg);

  return offset;
}


/*--- End of included file: packet-camel-fn.c ---*/


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

/*--- Included file: packet-camel-hfarr.c ---*/

    { &hf_camel_reserved,
      { "reserved", "camel.reserved",
        FT_INT32, BASE_DEC, NULL, 0,
        "", HFILL }},
    { &hf_camel_aoc,
      { "aoc", "camel.aoc",
        FT_INT32, BASE_DEC, NULL, 0,
        "PBSGSNCapabilities/aoc", HFILL }},
    { &hf_camel_standardPartEnd,
      { "standardPartEnd", "camel.standardPartEnd",
        FT_INT32, BASE_DEC, NULL, 0,
        "PBIPSSPCapabilities/standardPartEnd", HFILL }},
    { &hf_camel_genOfVoiceAnn,
      { "genOfVoiceAnn", "camel.genOfVoiceAnn",
        FT_INT32, BASE_DEC, NULL, 0,
        "PBIPSSPCapabilities/genOfVoiceAnn", HFILL }},
    { &hf_camel_voiceInfo2,
      { "voiceInfo2", "camel.voiceInfo2",
        FT_INT32, BASE_DEC, NULL, 0,
        "PBIPSSPCapabilities/voiceInfo2", HFILL }},
    { &hf_camel_voiceInfo1,
      { "voiceInfo1", "camel.voiceInfo1",
        FT_INT32, BASE_DEC, NULL, 0,
        "PBIPSSPCapabilities/voiceInfo1", HFILL }},
    { &hf_camel_voiceBack,
      { "voiceBack", "camel.voiceBack",
        FT_INT32, BASE_DEC, NULL, 0,
        "PBIPSSPCapabilities/voiceBack", HFILL }},
    { &hf_camel_iPRoutAdd,
      { "iPRoutAdd", "camel.iPRoutAdd",
        FT_INT32, BASE_DEC, NULL, 0,
        "PBIPSSPCapabilities/iPRoutAdd", HFILL }},
    { &hf_camel_bilateralPart,
      { "bilateralPart", "camel.bilateralPart",
        FT_BYTES, BASE_HEX, NULL, 0,
        "PBIPSSPCapabilities/bilateralPart", HFILL }},
    { &hf_camel_extension,
      { "extension", "camel.extension",
        FT_UINT32, BASE_DEC, NULL, 0,
        "", HFILL }},
    { &hf_camel_natureOfAddressIndicator,
      { "natureOfAddressIndicator", "camel.natureOfAddressIndicator",
        FT_INT32, BASE_DEC, NULL, 0,
        "", HFILL }},
    { &hf_camel_numberingPlanInd,
      { "numberingPlanInd", "camel.numberingPlanInd",
        FT_INT32, BASE_DEC, NULL, 0,
        "", HFILL }},
    { &hf_camel_digits,
      { "digits", "camel.digits",
        FT_BYTES, BASE_HEX, NULL, 0,
        "PBAddressString/digits", HFILL }},
    { &hf_camel_digits1,
      { "digits", "camel.digits",
        FT_BYTES, BASE_HEX, NULL, 0,
        "", HFILL }},
    { &hf_camel_typeOfShape,
      { "typeOfShape", "camel.typeOfShape",
        FT_INT32, BASE_DEC, NULL, 0,
        "PBGeographicalInformation/typeOfShape", HFILL }},
    { &hf_camel_spare,
      { "spare", "camel.spare",
        FT_INT32, BASE_DEC, NULL, 0,
        "PBGeographicalInformation/spare", HFILL }},
    { &hf_camel_degreesOfLatitude,
      { "degreesOfLatitude", "camel.degreesOfLatitude",
        FT_BYTES, BASE_HEX, NULL, 0,
        "PBGeographicalInformation/degreesOfLatitude", HFILL }},
    { &hf_camel_degreesOfLongitude,
      { "degreesOfLongitude", "camel.degreesOfLongitude",
        FT_BYTES, BASE_HEX, NULL, 0,
        "PBGeographicalInformation/degreesOfLongitude", HFILL }},
    { &hf_camel_uncertaintyCode,
      { "uncertaintyCode", "camel.uncertaintyCode",
        FT_BYTES, BASE_HEX, NULL, 0,
        "PBGeographicalInformation/uncertaintyCode", HFILL }},
    { &hf_camel_typeOfAddress,
      { "typeOfAddress", "camel.typeOfAddress",
        FT_INT32, BASE_DEC, NULL, 0,
        "PBGSNAddress/typeOfAddress", HFILL }},
    { &hf_camel_addressLength,
      { "addressLength", "camel.addressLength",
        FT_UINT32, BASE_DEC, NULL, 0,
        "PBGSNAddress/addressLength", HFILL }},
    { &hf_camel_address,
      { "address", "camel.address",
        FT_BYTES, BASE_HEX, NULL, 0,
        "PBGSNAddress/address", HFILL }},
    { &hf_camel_originalReasons,
      { "originalReasons", "camel.originalReasons",
        FT_INT32, BASE_DEC, NULL, 0,
        "PBRedirectionInformation/originalReasons", HFILL }},
    { &hf_camel_spare1,
      { "spare", "camel.spare",
        FT_UINT32, BASE_DEC, NULL, 0,
        "", HFILL }},
    { &hf_camel_indicator,
      { "indicator", "camel.indicator",
        FT_INT32, BASE_DEC, NULL, 0,
        "PBRedirectionInformation/indicator", HFILL }},
    { &hf_camel_reason,
      { "reason", "camel.reason",
        FT_INT32, BASE_DEC, NULL, 0,
        "PBRedirectionInformation/reason", HFILL }},
    { &hf_camel_spare2,
      { "spare2", "camel.spare2",
        FT_UINT32, BASE_DEC, NULL, 0,
        "PBRedirectionInformation/spare2", HFILL }},
    { &hf_camel_counter,
      { "counter", "camel.counter",
        FT_INT32, BASE_DEC, NULL, 0,
        "PBRedirectionInformation/counter", HFILL }},
    { &hf_camel_oddEven,
      { "oddEven", "camel.oddEven",
        FT_INT32, BASE_DEC, NULL, 0,
        "", HFILL }},
    { &hf_camel_innInd,
      { "innInd", "camel.innInd",
        FT_INT32, BASE_DEC, NULL, 0,
        "", HFILL }},
    { &hf_camel_digits2,
      { "digits", "camel.digits",
        FT_BYTES, BASE_HEX, NULL, 0,
        "PBCalledPartyNumber/digits", HFILL }},
    { &hf_camel_niInd,
      { "niInd", "camel.niInd",
        FT_INT32, BASE_DEC, NULL, 0,
        "", HFILL }},
    { &hf_camel_presentInd,
      { "presentInd", "camel.presentInd",
        FT_INT32, BASE_DEC, NULL, 0,
        "", HFILL }},
    { &hf_camel_screening,
      { "screening", "camel.screening",
        FT_INT32, BASE_DEC, NULL, 0,
        "", HFILL }},
    { &hf_camel_digits3,
      { "digits", "camel.digits",
        FT_BYTES, BASE_HEX, NULL, 0,
        "PBRedirectingNumber/digits", HFILL }},
    { &hf_camel_o1ext,
      { "o1ext", "camel.o1ext",
        FT_UINT32, BASE_DEC, NULL, 0,
        "PBCause/o1ext", HFILL }},
    { &hf_camel_codingStandard,
      { "codingStandard", "camel.codingStandard",
        FT_INT32, BASE_DEC, NULL, 0,
        "PBCause/codingStandard", HFILL }},
    { &hf_camel_location,
      { "location", "camel.location",
        FT_INT32, BASE_DEC, NULL, 0,
        "PBCause/location", HFILL }},
    { &hf_camel_o2ext,
      { "o2ext", "camel.o2ext",
        FT_UINT32, BASE_DEC, NULL, 0,
        "PBCause/o2ext", HFILL }},
    { &hf_camel_causeValue,
      { "causeValue", "camel.causeValue",
        FT_INT32, BASE_DEC, NULL, 0,
        "PBCause/causeValue", HFILL }},
    { &hf_camel_diagnostics,
      { "diagnostics", "camel.diagnostics",
        FT_BYTES, BASE_HEX, NULL, 0,
        "PBCause/diagnostics", HFILL }},
    { &hf_camel_numberQualifierIndicator,
      { "numberQualifierIndicator", "camel.numberQualifierIndicator",
        FT_INT32, BASE_DEC, NULL, 0,
        "PBGenericNumber/numberQualifierIndicator", HFILL }},
    { &hf_camel_ext,
      { "ext", "camel.ext",
        FT_INT32, BASE_DEC, NULL, 0,
        "PBCalledPartyBCDNumber/ext", HFILL }},
    { &hf_camel_typeOfNumber,
      { "typeOfNumber", "camel.typeOfNumber",
        FT_UINT32, BASE_DEC, VALS(camel_T_typeOfNumber_vals), 0,
        "PBCalledPartyBCDNumber/typeOfNumber", HFILL }},
    { &hf_camel_digits4,
      { "digits", "camel.digits",
        FT_BYTES, BASE_HEX, NULL, 0,
        "PBCalledPartyBCDNumber/digits", HFILL }},
    { &hf_camel_destinationReference,
      { "destinationReference", "camel.destinationReference",
        FT_UINT32, BASE_DEC, NULL, 0,
        "CAPGPRSReferenceNumber/destinationReference", HFILL }},
    { &hf_camel_originationReference,
      { "originationReference", "camel.originationReference",
        FT_UINT32, BASE_DEC, NULL, 0,
        "CAPGPRSReferenceNumber/originationReference", HFILL }},
    { &hf_camel_transferredVolumeRollOver,
      { "transferredVolumeRollOver", "camel.transferredVolumeRollOver",
        FT_UINT32, BASE_DEC, VALS(camel_TransferredVolumeRollOver_vals), 0,
        "ChargingRollOver/transferredVolumeRollOver", HFILL }},
    { &hf_camel_elapsedTimeRollOver,
      { "elapsedTimeRollOver", "camel.elapsedTimeRollOver",
        FT_UINT32, BASE_DEC, VALS(camel_ElapsedTimeRollOver_vals), 0,
        "ChargingRollOver/elapsedTimeRollOver", HFILL }},
    { &hf_camel_conferenceTreatmentIndicator,
      { "conferenceTreatmentIndicator", "camel.conferenceTreatmentIndicator",
        FT_BYTES, BASE_HEX, NULL, 0,
        "", HFILL }},
    { &hf_camel_callCompletionTreatmentIndicator,
      { "callCompletionTreatmentIndicator", "camel.callCompletionTreatmentIndicator",
        FT_BYTES, BASE_HEX, NULL, 0,
        "BackwardServiceInteractionInd/callCompletionTreatmentIndicator", HFILL }},
    { &hf_camel_bearerCap,
      { "bearerCap", "camel.bearerCap",
        FT_BYTES, BASE_HEX, NULL, 0,
        "BearerCapability/bearerCap", HFILL }},
    { &hf_camel_e1,
      { "e1", "camel.e1",
        FT_UINT32, BASE_DEC, NULL, 0,
        "CAI-Gsm0224/e1", HFILL }},
    { &hf_camel_e2,
      { "e2", "camel.e2",
        FT_UINT32, BASE_DEC, NULL, 0,
        "CAI-Gsm0224/e2", HFILL }},
    { &hf_camel_e3,
      { "e3", "camel.e3",
        FT_UINT32, BASE_DEC, NULL, 0,
        "CAI-Gsm0224/e3", HFILL }},
    { &hf_camel_e4,
      { "e4", "camel.e4",
        FT_UINT32, BASE_DEC, NULL, 0,
        "CAI-Gsm0224/e4", HFILL }},
    { &hf_camel_e5,
      { "e5", "camel.e5",
        FT_UINT32, BASE_DEC, NULL, 0,
        "CAI-Gsm0224/e5", HFILL }},
    { &hf_camel_e6,
      { "e6", "camel.e6",
        FT_UINT32, BASE_DEC, NULL, 0,
        "CAI-Gsm0224/e6", HFILL }},
    { &hf_camel_e7,
      { "e7", "camel.e7",
        FT_UINT32, BASE_DEC, NULL, 0,
        "CAI-Gsm0224/e7", HFILL }},
    { &hf_camel_maxTransferredVolume,
      { "maxTransferredVolume", "camel.maxTransferredVolume",
        FT_UINT32, BASE_DEC, NULL, 0,
        "ChargingCharacteristics/maxTransferredVolume", HFILL }},
    { &hf_camel_maxElapsedTime,
      { "maxElapsedTime", "camel.maxElapsedTime",
        FT_UINT32, BASE_DEC, NULL, 0,
        "ChargingCharacteristics/maxElapsedTime", HFILL }},
    { &hf_camel_timeGPRSIfNoTariffSwitch,
      { "timeGPRSIfNoTariffSwitch", "camel.timeGPRSIfNoTariffSwitch",
        FT_UINT32, BASE_DEC, NULL, 0,
        "ElapsedTime/timeGPRSIfNoTariffSwitch", HFILL }},
    { &hf_camel_timeGPRSIfTariffSwitch,
      { "timeGPRSIfTariffSwitch", "camel.timeGPRSIfTariffSwitch",
        FT_NONE, BASE_NONE, NULL, 0,
        "ElapsedTime/timeGPRSIfTariffSwitch", HFILL }},
    { &hf_camel_timeGPRSSinceLastTariffSwitch,
      { "timeGPRSSinceLastTariffSwitch", "camel.timeGPRSSinceLastTariffSwitch",
        FT_UINT32, BASE_DEC, NULL, 0,
        "ElapsedTime/timeGPRSIfTariffSwitch/timeGPRSSinceLastTariffSwitch", HFILL }},
    { &hf_camel_timeGPRSTariffSwitchInterval,
      { "timeGPRSTariffSwitchInterval", "camel.timeGPRSTariffSwitchInterval",
        FT_UINT32, BASE_DEC, NULL, 0,
        "ElapsedTime/timeGPRSIfTariffSwitch/timeGPRSTariffSwitchInterval", HFILL }},
    { &hf_camel_rOTimeGPRSIfNoTariffSwitch,
      { "rOTimeGPRSIfNoTariffSwitch", "camel.rOTimeGPRSIfNoTariffSwitch",
        FT_UINT32, BASE_DEC, NULL, 0,
        "ElapsedTimeRollOver/rOTimeGPRSIfNoTariffSwitch", HFILL }},
    { &hf_camel_rOTimeGPRSIfTariffSwitch,
      { "rOTimeGPRSIfTariffSwitch", "camel.rOTimeGPRSIfTariffSwitch",
        FT_NONE, BASE_NONE, NULL, 0,
        "ElapsedTimeRollOver/rOTimeGPRSIfTariffSwitch", HFILL }},
    { &hf_camel_rOTimeGPRSSinceLastTariffSwitch,
      { "rOTimeGPRSSinceLastTariffSwitch", "camel.rOTimeGPRSSinceLastTariffSwitch",
        FT_UINT32, BASE_DEC, NULL, 0,
        "ElapsedTimeRollOver/rOTimeGPRSIfTariffSwitch/rOTimeGPRSSinceLastTariffSwitch", HFILL }},
    { &hf_camel_rOTimeGPRSTariffSwitchInterval,
      { "rOTimeGPRSTariffSwitchInterval", "camel.rOTimeGPRSTariffSwitchInterval",
        FT_UINT32, BASE_DEC, NULL, 0,
        "ElapsedTimeRollOver/rOTimeGPRSIfTariffSwitch/rOTimeGPRSTariffSwitchInterval", HFILL }},
    { &hf_camel_callDiversionTreatmentIndicator,
      { "callDiversionTreatmentIndicator", "camel.callDiversionTreatmentIndicator",
        FT_BYTES, BASE_HEX, NULL, 0,
        "ForwardServiceInteractionInd/callDiversionTreatmentIndicator", HFILL }},
    { &hf_camel_callingPartyRestrictionIndicator,
      { "callingPartyRestrictionIndicator", "camel.callingPartyRestrictionIndicator",
        FT_BYTES, BASE_HEX, NULL, 0,
        "ForwardServiceInteractionInd/callingPartyRestrictionIndicator", HFILL }},
    { &hf_camel_pDPTypeOrganization,
      { "pDPTypeOrganization", "camel.pDPTypeOrganization",
        FT_BYTES, BASE_HEX, NULL, 0,
        "PDPType/pDPTypeOrganization", HFILL }},
    { &hf_camel_pDPTypeNumber,
      { "pDPTypeNumber", "camel.pDPTypeNumber",
        FT_BYTES, BASE_HEX, NULL, 0,
        "PDPType/pDPTypeNumber", HFILL }},
    { &hf_camel_pDPAddress,
      { "pDPAddress", "camel.pDPAddress",
        FT_BYTES, BASE_HEX, NULL, 0,
        "PDPType/pDPAddress", HFILL }},
    { &hf_camel_timeSinceTariffSwitch,
      { "timeSinceTariffSwitch", "camel.timeSinceTariffSwitch",
        FT_UINT32, BASE_DEC, NULL, 0,
        "TimeIfTariffSwitch/timeSinceTariffSwitch", HFILL }},
    { &hf_camel_tariffSwitchInterval,
      { "tariffSwitchInterval", "camel.tariffSwitchInterval",
        FT_UINT32, BASE_DEC, NULL, 0,
        "TimeIfTariffSwitch/tariffSwitchInterval", HFILL }},
    { &hf_camel_volumeIfNoTariffSwitch,
      { "volumeIfNoTariffSwitch", "camel.volumeIfNoTariffSwitch",
        FT_UINT32, BASE_DEC, NULL, 0,
        "TransferredVolume/volumeIfNoTariffSwitch", HFILL }},
    { &hf_camel_volumeIfTariffSwitch,
      { "volumeIfTariffSwitch", "camel.volumeIfTariffSwitch",
        FT_NONE, BASE_NONE, NULL, 0,
        "TransferredVolume/volumeIfTariffSwitch", HFILL }},
    { &hf_camel_volumeSinceLastTariffSwitch,
      { "volumeSinceLastTariffSwitch", "camel.volumeSinceLastTariffSwitch",
        FT_UINT32, BASE_DEC, NULL, 0,
        "TransferredVolume/volumeIfTariffSwitch/volumeSinceLastTariffSwitch", HFILL }},
    { &hf_camel_volumeTariffSwitchInterval,
      { "volumeTariffSwitchInterval", "camel.volumeTariffSwitchInterval",
        FT_UINT32, BASE_DEC, NULL, 0,
        "TransferredVolume/volumeIfTariffSwitch/volumeTariffSwitchInterval", HFILL }},
    { &hf_camel_rOVolumeIfNoTariffSwitch,
      { "rOVolumeIfNoTariffSwitch", "camel.rOVolumeIfNoTariffSwitch",
        FT_UINT32, BASE_DEC, NULL, 0,
        "TransferredVolumeRollOver/rOVolumeIfNoTariffSwitch", HFILL }},
    { &hf_camel_rOVolumeIfTariffSwitch,
      { "rOVolumeIfTariffSwitch", "camel.rOVolumeIfTariffSwitch",
        FT_NONE, BASE_NONE, NULL, 0,
        "TransferredVolumeRollOver/rOVolumeIfTariffSwitch", HFILL }},
    { &hf_camel_rOVolumeSinceLastTariffSwitch,
      { "rOVolumeSinceLastTariffSwitch", "camel.rOVolumeSinceLastTariffSwitch",
        FT_UINT32, BASE_DEC, NULL, 0,
        "TransferredVolumeRollOver/rOVolumeIfTariffSwitch/rOVolumeSinceLastTariffSwitch", HFILL }},
    { &hf_camel_rOVolumeTariffSwitchInterval,
      { "rOVolumeTariffSwitchInterval", "camel.rOVolumeTariffSwitchInterval",
        FT_UINT32, BASE_DEC, NULL, 0,
        "TransferredVolumeRollOver/rOVolumeIfTariffSwitch/rOVolumeTariffSwitchInterval", HFILL }},
    { &hf_camel_local,
      { "local", "camel.local",
        FT_INT32, BASE_DEC, NULL, 0,
        "Code/local", HFILL }},
    { &hf_camel_global,
      { "global", "camel.global",
        FT_STRING, BASE_NONE, NULL, 0,
        "Code/global", HFILL }},
    { &hf_camel_foo,
      { "foo", "camel.foo",
        FT_UINT32, BASE_DEC, NULL, 0,
        "", HFILL }},
    { &hf_camel_messageType,
      { "messageType", "camel.messageType",
        FT_UINT32, BASE_DEC, VALS(camel_T_messageType_vals), 0,
        "MiscCallInfo/messageType", HFILL }},
    { &hf_camel_firstExtensionExtensionType,
      { "firstExtensionExtensionType", "camel.firstExtensionExtensionType",
        FT_NONE, BASE_NONE, NULL, 0,
        "SupportedExtensionsExtensionType/firstExtensionExtensionType", HFILL }},
    { &hf_camel_extId,
      { "extId", "camel.extId",
        FT_STRING, BASE_NONE, NULL, 0,
        "PrivateExtension/extId", HFILL }},
    { &hf_camel_cAI_GSM0224,
      { "cAI-GSM0224", "camel.cAI_GSM0224",
        FT_NONE, BASE_NONE, NULL, 0,
        "AOCSubsequent/cAI-GSM0224", HFILL }},
    { &hf_camel_tariffSwitchInterval1,
      { "tariffSwitchInterval", "camel.tariffSwitchInterval",
        FT_UINT32, BASE_DEC, NULL, 0,
        "", HFILL }},
    { &hf_camel_fCIBCCCAMELsequence1,
      { "fCIBCCCAMELsequence1", "camel.fCIBCCCAMELsequence1",
        FT_NONE, BASE_NONE, NULL, 0,
        "CamelFCIGPRSBillingChargingCharacteristics/fCIBCCCAMELsequence1", HFILL }},
    { &hf_camel_freeFormatData,
      { "freeFormatData", "camel.freeFormatData",
        FT_BYTES, BASE_HEX, NULL, 0,
        "", HFILL }},
    { &hf_camel_pDPID,
      { "pDPID", "camel.pDPID",
        FT_BYTES, BASE_HEX, NULL, 0,
        "", HFILL }},
    { &hf_camel_appendFreeFormatData,
      { "appendFreeFormatData", "camel.appendFreeFormatData",
        FT_UINT32, BASE_DEC, VALS(camel_AppendFreeFormatData_vals), 0,
        "", HFILL }},
    { &hf_camel_fCIBCCCAMELsequence11,
      { "fCIBCCCAMELsequence1", "camel.fCIBCCCAMELsequence1",
        FT_NONE, BASE_NONE, NULL, 0,
        "CamelFCISMSBillingChargingCharacteristics/fCIBCCCAMELsequence1", HFILL }},
    { &hf_camel_transferredVolume,
      { "transferredVolume", "camel.transferredVolume",
        FT_UINT32, BASE_DEC, VALS(camel_TransferredVolume_vals), 0,
        "ChargingResult/transferredVolume", HFILL }},
    { &hf_camel_elapsedTime,
      { "elapsedTime", "camel.elapsedTime",
        FT_UINT32, BASE_DEC, VALS(camel_ElapsedTime_vals), 0,
        "ChargingResult/elapsedTime", HFILL }},
    { &hf_camel_minimumNbOfDigits,
      { "minimumNbOfDigits", "camel.minimumNbOfDigits",
        FT_UINT32, BASE_DEC, NULL, 0,
        "CollectedDigits/minimumNbOfDigits", HFILL }},
    { &hf_camel_maximumNbOfDigits,
      { "maximumNbOfDigits", "camel.maximumNbOfDigits",
        FT_UINT32, BASE_DEC, NULL, 0,
        "CollectedDigits/maximumNbOfDigits", HFILL }},
    { &hf_camel_endOfReplyDigit,
      { "endOfReplyDigit", "camel.endOfReplyDigit",
        FT_BYTES, BASE_HEX, NULL, 0,
        "CollectedDigits/endOfReplyDigit", HFILL }},
    { &hf_camel_cancelDigit,
      { "cancelDigit", "camel.cancelDigit",
        FT_BYTES, BASE_HEX, NULL, 0,
        "CollectedDigits/cancelDigit", HFILL }},
    { &hf_camel_startDigit,
      { "startDigit", "camel.startDigit",
        FT_BYTES, BASE_HEX, NULL, 0,
        "CollectedDigits/startDigit", HFILL }},
    { &hf_camel_firstDigitTimeOut,
      { "firstDigitTimeOut", "camel.firstDigitTimeOut",
        FT_UINT32, BASE_DEC, NULL, 0,
        "CollectedDigits/firstDigitTimeOut", HFILL }},
    { &hf_camel_interDigitTimeOut,
      { "interDigitTimeOut", "camel.interDigitTimeOut",
        FT_UINT32, BASE_DEC, NULL, 0,
        "CollectedDigits/interDigitTimeOut", HFILL }},
    { &hf_camel_errorTreatment,
      { "errorTreatment", "camel.errorTreatment",
        FT_UINT32, BASE_DEC, VALS(camel_ErrorTreatment_vals), 0,
        "CollectedDigits/errorTreatment", HFILL }},
    { &hf_camel_interruptableAnnInd,
      { "interruptableAnnInd", "camel.interruptableAnnInd",
        FT_BOOLEAN, 8, NULL, 0,
        "CollectedDigits/interruptableAnnInd", HFILL }},
    { &hf_camel_voiceInformation,
      { "voiceInformation", "camel.voiceInformation",
        FT_BOOLEAN, 8, NULL, 0,
        "CollectedDigits/voiceInformation", HFILL }},
    { &hf_camel_voiceBack1,
      { "voiceBack", "camel.voiceBack",
        FT_BOOLEAN, 8, NULL, 0,
        "CollectedDigits/voiceBack", HFILL }},
    { &hf_camel_collectedDigits,
      { "collectedDigits", "camel.collectedDigits",
        FT_NONE, BASE_NONE, NULL, 0,
        "CollectedInfo/collectedDigits", HFILL }},
    { &hf_camel_DestinationRoutingAddress_item,
      { "Item", "camel.DestinationRoutingAddress_item",
        FT_BYTES, BASE_HEX, NULL, 0,
        "DestinationRoutingAddress/_item", HFILL }},
    { &hf_camel_applicationTimer,
      { "applicationTimer", "camel.applicationTimer",
        FT_UINT32, BASE_DEC, NULL, 0,
        "DpSpecificCriteria/applicationTimer", HFILL }},
    { &hf_camel_routeSelectFailureSpecificInfo,
      { "routeSelectFailureSpecificInfo", "camel.routeSelectFailureSpecificInfo",
        FT_NONE, BASE_NONE, NULL, 0,
        "EventSpecificInformationBCSM/routeSelectFailureSpecificInfo", HFILL }},
    { &hf_camel_failureCause,
      { "failureCause", "camel.failureCause",
        FT_BYTES, BASE_HEX, NULL, 0,
        "EventSpecificInformationBCSM/routeSelectFailureSpecificInfo/failureCause", HFILL }},
    { &hf_camel_oCalledPartyBusySpecificInfo,
      { "oCalledPartyBusySpecificInfo", "camel.oCalledPartyBusySpecificInfo",
        FT_NONE, BASE_NONE, NULL, 0,
        "EventSpecificInformationBCSM/oCalledPartyBusySpecificInfo", HFILL }},
    { &hf_camel_busyCause,
      { "busyCause", "camel.busyCause",
        FT_BYTES, BASE_HEX, NULL, 0,
        "", HFILL }},
    { &hf_camel_oNoAnswerSpecificInfo,
      { "oNoAnswerSpecificInfo", "camel.oNoAnswerSpecificInfo",
        FT_NONE, BASE_NONE, NULL, 0,
        "EventSpecificInformationBCSM/oNoAnswerSpecificInfo", HFILL }},
    { &hf_camel_oAnswerSpecificInfo,
      { "oAnswerSpecificInfo", "camel.oAnswerSpecificInfo",
        FT_NONE, BASE_NONE, NULL, 0,
        "EventSpecificInformationBCSM/oAnswerSpecificInfo", HFILL }},
    { &hf_camel_destinationAddress,
      { "destinationAddress", "camel.destinationAddress",
        FT_BYTES, BASE_HEX, NULL, 0,
        "", HFILL }},
    { &hf_camel_or_Call,
      { "or-Call", "camel.or_Call",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_camel_forwardedCall,
      { "forwardedCall", "camel.forwardedCall",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_camel_oDisconnectSpecificInfo,
      { "oDisconnectSpecificInfo", "camel.oDisconnectSpecificInfo",
        FT_NONE, BASE_NONE, NULL, 0,
        "EventSpecificInformationBCSM/oDisconnectSpecificInfo", HFILL }},
    { &hf_camel_releaseCause,
      { "releaseCause", "camel.releaseCause",
        FT_BYTES, BASE_HEX, NULL, 0,
        "", HFILL }},
    { &hf_camel_tBusySpecificInfo,
      { "tBusySpecificInfo", "camel.tBusySpecificInfo",
        FT_NONE, BASE_NONE, NULL, 0,
        "EventSpecificInformationBCSM/tBusySpecificInfo", HFILL }},
    { &hf_camel_callForwarded,
      { "callForwarded", "camel.callForwarded",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_camel_routeNotPermitted,
      { "routeNotPermitted", "camel.routeNotPermitted",
        FT_NONE, BASE_NONE, NULL, 0,
        "EventSpecificInformationBCSM/tBusySpecificInfo/routeNotPermitted", HFILL }},
    { &hf_camel_tNoAnswerSpecificInfo,
      { "tNoAnswerSpecificInfo", "camel.tNoAnswerSpecificInfo",
        FT_NONE, BASE_NONE, NULL, 0,
        "EventSpecificInformationBCSM/tNoAnswerSpecificInfo", HFILL }},
    { &hf_camel_tAnswerSpecificInfo,
      { "tAnswerSpecificInfo", "camel.tAnswerSpecificInfo",
        FT_NONE, BASE_NONE, NULL, 0,
        "EventSpecificInformationBCSM/tAnswerSpecificInfo", HFILL }},
    { &hf_camel_tDisconnectSpecificInfo,
      { "tDisconnectSpecificInfo", "camel.tDisconnectSpecificInfo",
        FT_NONE, BASE_NONE, NULL, 0,
        "EventSpecificInformationBCSM/tDisconnectSpecificInfo", HFILL }},
    { &hf_camel_o_smsFailureSpecificInfo,
      { "o-smsFailureSpecificInfo", "camel.o_smsFailureSpecificInfo",
        FT_NONE, BASE_NONE, NULL, 0,
        "EventSpecificInformationSMS/o-smsFailureSpecificInfo", HFILL }},
    { &hf_camel_failureCause1,
      { "failureCause", "camel.failureCause",
        FT_UINT32, BASE_DEC, VALS(camel_SMSCause_vals), 0,
        "EventSpecificInformationSMS/o-smsFailureSpecificInfo/failureCause", HFILL }},
    { &hf_camel_o_smsSubmittedSpecificInfo,
      { "o-smsSubmittedSpecificInfo", "camel.o_smsSubmittedSpecificInfo",
        FT_NONE, BASE_NONE, NULL, 0,
        "EventSpecificInformationSMS/o-smsSubmittedSpecificInfo", HFILL }},
    { &hf_camel_duration,
      { "duration", "camel.duration",
        FT_UINT32, BASE_DEC, NULL, 0,
        "GapIndicators/duration", HFILL }},
    { &hf_camel_gapInterval,
      { "gapInterval", "camel.gapInterval",
        FT_UINT32, BASE_DEC, NULL, 0,
        "GapIndicators/gapInterval", HFILL }},
    { &hf_camel_serviceKey,
      { "serviceKey", "camel.serviceKey",
        FT_UINT32, BASE_DEC, NULL, 0,
        "", HFILL }},
    { &hf_camel_GenericNumbers_item,
      { "Item", "camel.GenericNumbers_item",
        FT_BYTES, BASE_HEX, NULL, 0,
        "GenericNumbers/_item", HFILL }},
    { &hf_camel_short_QoS_format,
      { "short-QoS-format", "camel.short_QoS_format",
        FT_BYTES, BASE_HEX, NULL, 0,
        "GPRS-QoS/short-QoS-format", HFILL }},
    { &hf_camel_long_QoS_format,
      { "long-QoS-format", "camel.long_QoS_format",
        FT_BYTES, BASE_HEX, NULL, 0,
        "GPRS-QoS/long-QoS-format", HFILL }},
    { &hf_camel_gPRSEventType,
      { "gPRSEventType", "camel.gPRSEventType",
        FT_UINT32, BASE_DEC, VALS(camel_GPRSEventType_vals), 0,
        "", HFILL }},
    { &hf_camel_monitorMode,
      { "monitorMode", "camel.monitorMode",
        FT_UINT32, BASE_DEC, VALS(camel_MonitorMode_vals), 0,
        "", HFILL }},
    { &hf_camel_mSNetworkCapability,
      { "mSNetworkCapability", "camel.mSNetworkCapability",
        FT_BYTES, BASE_HEX, NULL, 0,
        "GPRSMSClass/mSNetworkCapability", HFILL }},
    { &hf_camel_mSRadioAccessCapability,
      { "mSRadioAccessCapability", "camel.mSRadioAccessCapability",
        FT_BYTES, BASE_HEX, NULL, 0,
        "GPRSMSClass/mSRadioAccessCapability", HFILL }},
    { &hf_camel_requested_QoS,
      { "requested-QoS", "camel.requested_QoS",
        FT_UINT32, BASE_DEC, VALS(camel_GPRS_QoS_vals), 0,
        "QualityOfService/requested-QoS", HFILL }},
    { &hf_camel_subscribed_QoS,
      { "subscribed-QoS", "camel.subscribed_QoS",
        FT_UINT32, BASE_DEC, VALS(camel_GPRS_QoS_vals), 0,
        "QualityOfService/subscribed-QoS", HFILL }},
    { &hf_camel_negotiated_QoS,
      { "negotiated-QoS", "camel.negotiated_QoS",
        FT_UINT32, BASE_DEC, VALS(camel_GPRS_QoS_vals), 0,
        "QualityOfService/negotiated-QoS", HFILL }},
    { &hf_camel_receivingSideID,
      { "receivingSideID", "camel.receivingSideID",
        FT_BYTES, BASE_HEX, NULL, 0,
        "", HFILL }},
    { &hf_camel_RequestedInformationTypeList_item,
      { "Item", "camel.RequestedInformationTypeList_item",
        FT_UINT32, BASE_DEC, VALS(camel_RequestedInformationType_vals), 0,
        "RequestedInformationTypeList/_item", HFILL }},
    { &hf_camel_callAttemptElapsedTimeValue,
      { "callAttemptElapsedTimeValue", "camel.callAttemptElapsedTimeValue",
        FT_UINT32, BASE_DEC, NULL, 0,
        "RequestedInformationValue/callAttemptElapsedTimeValue", HFILL }},
    { &hf_camel_callStopTimeValue,
      { "callStopTimeValue", "camel.callStopTimeValue",
        FT_BYTES, BASE_HEX, NULL, 0,
        "RequestedInformationValue/callStopTimeValue", HFILL }},
    { &hf_camel_callConnectedElapsedTimeValue,
      { "callConnectedElapsedTimeValue", "camel.callConnectedElapsedTimeValue",
        FT_UINT32, BASE_DEC, NULL, 0,
        "RequestedInformationValue/callConnectedElapsedTimeValue", HFILL }},
    { &hf_camel_releaseCauseValue,
      { "releaseCauseValue", "camel.releaseCauseValue",
        FT_BYTES, BASE_HEX, NULL, 0,
        "RequestedInformationValue/releaseCauseValue", HFILL }},
    { &hf_camel_sendingSideID,
      { "sendingSideID", "camel.sendingSideID",
        FT_BYTES, BASE_HEX, NULL, 0,
        "", HFILL }},
    { &hf_camel_forwardServiceInteractionInd,
      { "forwardServiceInteractionInd", "camel.forwardServiceInteractionInd",
        FT_NONE, BASE_NONE, NULL, 0,
        "ServiceInteractionIndicatorsTwo/forwardServiceInteractionInd", HFILL }},
    { &hf_camel_backwardServiceInteractionInd,
      { "backwardServiceInteractionInd", "camel.backwardServiceInteractionInd",
        FT_NONE, BASE_NONE, NULL, 0,
        "ServiceInteractionIndicatorsTwo/backwardServiceInteractionInd", HFILL }},
    { &hf_camel_bothwayThroughConnectionInd,
      { "bothwayThroughConnectionInd", "camel.bothwayThroughConnectionInd",
        FT_UINT32, BASE_DEC, VALS(camel_BothwayThroughConnectionInd_vals), 0,
        "ServiceInteractionIndicatorsTwo/bothwayThroughConnectionInd", HFILL }},
    { &hf_camel_connectedNumberTreatmentInd,
      { "connectedNumberTreatmentInd", "camel.connectedNumberTreatmentInd",
        FT_UINT32, BASE_DEC, VALS(camel_ConnectedNumberTreatmentInd_vals), 0,
        "ServiceInteractionIndicatorsTwo/connectedNumberTreatmentInd", HFILL }},
    { &hf_camel_nonCUGCall,
      { "nonCUGCall", "camel.nonCUGCall",
        FT_NONE, BASE_NONE, NULL, 0,
        "ServiceInteractionIndicatorsTwo/nonCUGCall", HFILL }},
    { &hf_camel_holdTreatmentIndicator,
      { "holdTreatmentIndicator", "camel.holdTreatmentIndicator",
        FT_BYTES, BASE_HEX, NULL, 0,
        "ServiceInteractionIndicatorsTwo/holdTreatmentIndicator", HFILL }},
    { &hf_camel_cwTreatmentIndicator,
      { "cwTreatmentIndicator", "camel.cwTreatmentIndicator",
        FT_BYTES, BASE_HEX, NULL, 0,
        "ServiceInteractionIndicatorsTwo/cwTreatmentIndicator", HFILL }},
    { &hf_camel_ectTreatmentIndicator,
      { "ectTreatmentIndicator", "camel.ectTreatmentIndicator",
        FT_BYTES, BASE_HEX, NULL, 0,
        "ServiceInteractionIndicatorsTwo/ectTreatmentIndicator", HFILL }},
    { &hf_camel_eventTypeSMS,
      { "eventTypeSMS", "camel.eventTypeSMS",
        FT_UINT32, BASE_DEC, VALS(camel_EventTypeSMS_vals), 0,
        "", HFILL }},
    { &hf_camel_timeIfNoTariffSwitch,
      { "timeIfNoTariffSwitch", "camel.timeIfNoTariffSwitch",
        FT_UINT32, BASE_DEC, NULL, 0,
        "TimeInformation/timeIfNoTariffSwitch", HFILL }},
    { &hf_camel_timeIfTariffSwitch,
      { "timeIfTariffSwitch", "camel.timeIfTariffSwitch",
        FT_NONE, BASE_NONE, NULL, 0,
        "TimeInformation/timeIfTariffSwitch", HFILL }},
    { &hf_camel_toneID,
      { "toneID", "camel.toneID",
        FT_UINT32, BASE_DEC, NULL, 0,
        "Tone/toneID", HFILL }},
    { &hf_camel_duration1,
      { "duration", "camel.duration",
        FT_UINT32, BASE_DEC, NULL, 0,
        "Tone/duration", HFILL }},
    { &hf_camel_integer,
      { "integer", "camel.integer",
        FT_UINT32, BASE_DEC, NULL, 0,
        "VariablePart/integer", HFILL }},
    { &hf_camel_number,
      { "number", "camel.number",
        FT_BYTES, BASE_HEX, NULL, 0,
        "VariablePart/number", HFILL }},
    { &hf_camel_time,
      { "time", "camel.time",
        FT_BYTES, BASE_HEX, NULL, 0,
        "VariablePart/time", HFILL }},
    { &hf_camel_date,
      { "date", "camel.date",
        FT_BYTES, BASE_HEX, NULL, 0,
        "VariablePart/date", HFILL }},
    { &hf_camel_price,
      { "price", "camel.price",
        FT_BYTES, BASE_HEX, NULL, 0,
        "VariablePart/price", HFILL }},
    { &hf_camel_invokeID,
      { "invokeID", "camel.invokeID",
        FT_UINT32, BASE_DEC, NULL, 0,
        "CancelArg/invokeID", HFILL }},
    { &hf_camel_allRequests,
      { "allRequests", "camel.allRequests",
        FT_NONE, BASE_NONE, NULL, 0,
        "CancelArg/allRequests", HFILL }},
    { &hf_camel_digitsResponse,
      { "digitsResponse", "camel.digitsResponse",
        FT_BYTES, BASE_HEX, NULL, 0,
        "ReceivedInformationArg/digitsResponse", HFILL }},
    { &hf_camel_accessPointName,
      { "accessPointName", "camel.accessPointName",
        FT_BYTES, BASE_HEX, NULL, 0,
        "", HFILL }},
    { &hf_camel_pdpID,
      { "pdpID", "camel.pdpID",
        FT_BYTES, BASE_HEX, NULL, 0,
        "ConnectGPRSArg/pdpID", HFILL }},
    { &hf_camel_gPRSCause,
      { "gPRSCause", "camel.gPRSCause",
        FT_BYTES, BASE_HEX, NULL, 0,
        "EntityReleasedGPRSArg/gPRSCause", HFILL }},
    { &hf_camel_gprsCause,
      { "gprsCause", "camel.gprsCause",
        FT_BYTES, BASE_HEX, NULL, 0,
        "ReleaseGPRSArg/gprsCause", HFILL }},
    { &hf_camel_gPRSEvent,
      { "gPRSEvent", "camel.gPRSEvent",
        FT_NONE, BASE_NONE, NULL, 0,
        "RequestReportGPRSEventArg/gPRSEvent", HFILL }},
    { &hf_camel_GPRSEventArray_item,
      { "Item", "camel.GPRSEventArray_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "GPRSEventArray/_item", HFILL }},
    { &hf_camel_sCIGPRSBillingChargingCharacteristics,
      { "sCIGPRSBillingChargingCharacteristics", "camel.sCIGPRSBillingChargingCharacteristics",
        FT_BYTES, BASE_HEX, NULL, 0,
        "SendChargingInformationGPRSArg/sCIGPRSBillingChargingCharacteristics", HFILL }},
    { &hf_camel_assumedIdle,
      { "assumedIdle", "camel.assumedIdle",
        FT_NONE, BASE_NONE, NULL, 0,
        "SubscriberState/assumedIdle", HFILL }},
    { &hf_camel_camelBusy,
      { "camelBusy", "camel.camelBusy",
        FT_NONE, BASE_NONE, NULL, 0,
        "SubscriberState/camelBusy", HFILL }},
    { &hf_camel_netDetNotReachable,
      { "netDetNotReachable", "camel.netDetNotReachable",
        FT_UINT32, BASE_DEC, VALS(camel_NotReachableReason_vals), 0,
        "SubscriberState/netDetNotReachable", HFILL }},
    { &hf_camel_notProvidedFromVLR,
      { "notProvidedFromVLR", "camel.notProvidedFromVLR",
        FT_NONE, BASE_NONE, NULL, 0,
        "SubscriberState/notProvidedFromVLR", HFILL }},
    { &hf_camel_PrivateExtensionList_item,
      { "Item", "camel.PrivateExtensionList_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "PrivateExtensionList/_item", HFILL }},
    { &hf_camel_cellIdFixedLength,
      { "cellIdFixedLength", "camel.cellIdFixedLength",
        FT_BYTES, BASE_HEX, NULL, 0,
        "CellIdOrLAI/cellIdFixedLength", HFILL }},
    { &hf_camel_laiFixedLength,
      { "laiFixedLength", "camel.laiFixedLength",
        FT_BYTES, BASE_HEX, NULL, 0,
        "CellIdOrLAI/laiFixedLength", HFILL }},
    { &hf_camel_ext_BearerService,
      { "ext-BearerService", "camel.ext_BearerService",
        FT_BYTES, BASE_HEX, NULL, 0,
        "Ext-BasicServiceCode/ext-BearerService", HFILL }},
    { &hf_camel_ext_Teleservice,
      { "ext-Teleservice", "camel.ext_Teleservice",
        FT_BYTES, BASE_HEX, NULL, 0,
        "Ext-BasicServiceCode/ext-Teleservice", HFILL }},
    { &hf_camel_aOCInitial,
      { "aOCInitial", "camel.aOCInitial",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_camel_aOCSubsequent,
      { "aOCSubsequent", "camel.aOCSubsequent",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_camel_calledAddressValue,
      { "calledAddressValue", "camel.calledAddressValue",
        FT_BYTES, BASE_HEX, NULL, 0,
        "", HFILL }},
    { &hf_camel_gapOnService,
      { "gapOnService", "camel.gapOnService",
        FT_NONE, BASE_NONE, NULL, 0,
        "BasicGapCriteria/gapOnService", HFILL }},
    { &hf_camel_calledAddressAndService,
      { "calledAddressAndService", "camel.calledAddressAndService",
        FT_NONE, BASE_NONE, NULL, 0,
        "BasicGapCriteria/calledAddressAndService", HFILL }},
    { &hf_camel_callingAddressAndService,
      { "callingAddressAndService", "camel.callingAddressAndService",
        FT_NONE, BASE_NONE, NULL, 0,
        "BasicGapCriteria/callingAddressAndService", HFILL }},
    { &hf_camel_callingAddressValue,
      { "callingAddressValue", "camel.callingAddressValue",
        FT_BYTES, BASE_HEX, NULL, 0,
        "BasicGapCriteria/callingAddressAndService/callingAddressValue", HFILL }},
    { &hf_camel_eventTypeBCSM,
      { "eventTypeBCSM", "camel.eventTypeBCSM",
        FT_UINT32, BASE_DEC, VALS(camel_EventTypeBCSM_vals), 0,
        "", HFILL }},
    { &hf_camel_legID,
      { "legID", "camel.legID",
        FT_UINT32, BASE_DEC, VALS(camel_LegID_vals), 0,
        "BCSMEvent/legID", HFILL }},
    { &hf_camel_dpSpecificCriteria,
      { "dpSpecificCriteria", "camel.dpSpecificCriteria",
        FT_UINT32, BASE_DEC, VALS(camel_DpSpecificCriteria_vals), 0,
        "BCSMEvent/dpSpecificCriteria", HFILL }},
    { &hf_camel_fCIBCCCAMELsequence12,
      { "fCIBCCCAMELsequence1", "camel.fCIBCCCAMELsequence1",
        FT_NONE, BASE_NONE, NULL, 0,
        "CamelFCIBillingChargingCharacteristics/fCIBCCCAMELsequence1", HFILL }},
    { &hf_camel_partyToCharge,
      { "partyToCharge", "camel.partyToCharge",
        FT_UINT32, BASE_DEC, VALS(camel_SendingSideID_vals), 0,
        "", HFILL }},
    { &hf_camel_basicGapCriteria,
      { "basicGapCriteria", "camel.basicGapCriteria",
        FT_UINT32, BASE_DEC, VALS(camel_BasicGapCriteria_vals), 0,
        "", HFILL }},
    { &hf_camel_scfID,
      { "scfID", "camel.scfID",
        FT_BYTES, BASE_HEX, NULL, 0,
        "", HFILL }},
    { &hf_camel_compoundGapCriteria,
      { "compoundGapCriteria", "camel.compoundGapCriteria",
        FT_NONE, BASE_NONE, NULL, 0,
        "GapCriteria/compoundGapCriteria", HFILL }},
    { &hf_camel_elementaryMessageID,
      { "elementaryMessageID", "camel.elementaryMessageID",
        FT_UINT32, BASE_DEC, NULL, 0,
        "", HFILL }},
    { &hf_camel_text,
      { "text", "camel.text",
        FT_NONE, BASE_NONE, NULL, 0,
        "MessageID/text", HFILL }},
    { &hf_camel_messageContent,
      { "messageContent", "camel.messageContent",
        FT_STRING, BASE_NONE, NULL, 0,
        "MessageID/text/messageContent", HFILL }},
    { &hf_camel_attributes,
      { "attributes", "camel.attributes",
        FT_BYTES, BASE_HEX, NULL, 0,
        "MessageID/text/attributes", HFILL }},
    { &hf_camel_elementaryMessageIDs,
      { "elementaryMessageIDs", "camel.elementaryMessageIDs",
        FT_NONE, BASE_NONE, NULL, 0,
        "MessageID/elementaryMessageIDs", HFILL }},
    { &hf_camel_elementaryMessageIDs_item,
      { "Item", "camel.elementaryMessageIDs_item",
        FT_UINT32, BASE_DEC, NULL, 0,
        "MessageID/elementaryMessageIDs/_item", HFILL }},
    { &hf_camel_variableMessage,
      { "variableMessage", "camel.variableMessage",
        FT_NONE, BASE_NONE, NULL, 0,
        "MessageID/variableMessage", HFILL }},
    { &hf_camel_variableParts,
      { "variableParts", "camel.variableParts",
        FT_NONE, BASE_NONE, NULL, 0,
        "MessageID/variableMessage/variableParts", HFILL }},
    { &hf_camel_VariablePartsArray_item,
      { "Item", "camel.VariablePartsArray_item",
        FT_UINT32, BASE_DEC, VALS(camel_VariablePart_vals), 0,
        "VariablePartsArray/_item", HFILL }},
    { &hf_camel_requestedInformationType,
      { "requestedInformationType", "camel.requestedInformationType",
        FT_UINT32, BASE_DEC, VALS(camel_RequestedInformationType_vals), 0,
        "RequestedInformation/requestedInformationType", HFILL }},
    { &hf_camel_requestedInformationValue,
      { "requestedInformationValue", "camel.requestedInformationValue",
        FT_UINT32, BASE_DEC, VALS(camel_RequestedInformationValue_vals), 0,
        "RequestedInformation/requestedInformationValue", HFILL }},
    { &hf_camel_gmscAddress,
      { "gmscAddress", "camel.gmscAddress",
        FT_BYTES, BASE_HEX, NULL, 0,
        "InitialDPArgExtension/gmscAddress", HFILL }},
    { &hf_camel_messageID,
      { "messageID", "camel.messageID",
        FT_UINT32, BASE_DEC, VALS(camel_MessageID_vals), 0,
        "InbandInfo/messageID", HFILL }},
    { &hf_camel_numberOfRepetitions,
      { "numberOfRepetitions", "camel.numberOfRepetitions",
        FT_UINT32, BASE_DEC, NULL, 0,
        "InbandInfo/numberOfRepetitions", HFILL }},
    { &hf_camel_duration2,
      { "duration", "camel.duration",
        FT_UINT32, BASE_DEC, NULL, 0,
        "InbandInfo/duration", HFILL }},
    { &hf_camel_interval,
      { "interval", "camel.interval",
        FT_UINT32, BASE_DEC, NULL, 0,
        "InbandInfo/interval", HFILL }},
    { &hf_camel_inbandInfo,
      { "inbandInfo", "camel.inbandInfo",
        FT_NONE, BASE_NONE, NULL, 0,
        "InformationToSend/inbandInfo", HFILL }},
    { &hf_camel_tone,
      { "tone", "camel.tone",
        FT_NONE, BASE_NONE, NULL, 0,
        "InformationToSend/tone", HFILL }},
    { &hf_camel_cellGlobalIdOrServiceAreaIdOrLAI,
      { "cellGlobalIdOrServiceAreaIdOrLAI", "camel.cellGlobalIdOrServiceAreaIdOrLAI",
        FT_BYTES, BASE_HEX, NULL, 0,
        "LocationInformationGPRS/cellGlobalIdOrServiceAreaIdOrLAI", HFILL }},
    { &hf_camel_routeingAreaIdentity,
      { "routeingAreaIdentity", "camel.routeingAreaIdentity",
        FT_BYTES, BASE_HEX, NULL, 0,
        "", HFILL }},
    { &hf_camel_geographicalInformation,
      { "geographicalInformation", "camel.geographicalInformation",
        FT_BYTES, BASE_HEX, NULL, 0,
        "", HFILL }},
    { &hf_camel_sgsn_Number,
      { "sgsn-Number", "camel.sgsn_Number",
        FT_BYTES, BASE_HEX, NULL, 0,
        "LocationInformationGPRS/sgsn-Number", HFILL }},
    { &hf_camel_selectedLSAIdentity,
      { "selectedLSAIdentity", "camel.selectedLSAIdentity",
        FT_BYTES, BASE_HEX, NULL, 0,
        "LocationInformationGPRS/selectedLSAIdentity", HFILL }},
    { &hf_camel_extensionContainer,
      { "extensionContainer", "camel.extensionContainer",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_camel_saiPresent,
      { "saiPresent", "camel.saiPresent",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_camel_RequestedInformationList_item,
      { "Item", "camel.RequestedInformationList_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "RequestedInformationList/_item", HFILL }},
    { &hf_camel_ageOfLocationInformation,
      { "ageOfLocationInformation", "camel.ageOfLocationInformation",
        FT_UINT32, BASE_DEC, NULL, 0,
        "LocationInformation/ageOfLocationInformation", HFILL }},
    { &hf_camel_vlr_number,
      { "vlr-number", "camel.vlr_number",
        FT_BYTES, BASE_HEX, NULL, 0,
        "LocationInformation/vlr-number", HFILL }},
    { &hf_camel_locationNumber,
      { "locationNumber", "camel.locationNumber",
        FT_BYTES, BASE_HEX, NULL, 0,
        "", HFILL }},
    { &hf_camel_cellIdOrLAI,
      { "cellIdOrLAI", "camel.cellIdOrLAI",
        FT_UINT32, BASE_DEC, VALS(camel_CellIdOrLAI_vals), 0,
        "LocationInformation/cellIdOrLAI", HFILL }},
    { &hf_camel_selectedLSA_Id,
      { "selectedLSA-Id", "camel.selectedLSA_Id",
        FT_BYTES, BASE_HEX, NULL, 0,
        "LocationInformation/selectedLSA-Id", HFILL }},
    { &hf_camel_msc_number,
      { "msc-number", "camel.msc_number",
        FT_BYTES, BASE_HEX, NULL, 0,
        "LocationInformation/msc-number", HFILL }},
    { &hf_camel_geodeticInformation,
      { "geodeticInformation", "camel.geodeticInformation",
        FT_BYTES, BASE_HEX, NULL, 0,
        "LocationInformation/geodeticInformation", HFILL }},
    { &hf_camel_currentLocationRetrieved,
      { "currentLocationRetrieved", "camel.currentLocationRetrieved",
        FT_NONE, BASE_NONE, NULL, 0,
        "LocationInformation/currentLocationRetrieved", HFILL }},
    { &hf_camel_informationToSend,
      { "informationToSend", "camel.informationToSend",
        FT_UINT32, BASE_DEC, VALS(camel_InformationToSend_vals), 0,
        "", HFILL }},
    { &hf_camel_attachChangeOfPositionSpecificInformation,
      { "attachChangeOfPositionSpecificInformation", "camel.attachChangeOfPositionSpecificInformation",
        FT_NONE, BASE_NONE, NULL, 0,
        "GPRSEventSpecificInformation/attachChangeOfPositionSpecificInformation", HFILL }},
    { &hf_camel_locationInformationGPRS,
      { "locationInformationGPRS", "camel.locationInformationGPRS",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_camel_pdp_ContextchangeOfPositionSpecificInformation,
      { "pdp-ContextchangeOfPositionSpecificInformation", "camel.pdp_ContextchangeOfPositionSpecificInformation",
        FT_NONE, BASE_NONE, NULL, 0,
        "GPRSEventSpecificInformation/pdp-ContextchangeOfPositionSpecificInformation", HFILL }},
    { &hf_camel_chargingID,
      { "chargingID", "camel.chargingID",
        FT_BYTES, BASE_HEX, NULL, 0,
        "", HFILL }},
    { &hf_camel_pDPType,
      { "pDPType", "camel.pDPType",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_camel_qualityOfService,
      { "qualityOfService", "camel.qualityOfService",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_camel_timeAndTimeZone,
      { "timeAndTimeZone", "camel.timeAndTimeZone",
        FT_BYTES, BASE_HEX, NULL, 0,
        "", HFILL }},
    { &hf_camel_gGSNAddress,
      { "gGSNAddress", "camel.gGSNAddress",
        FT_BYTES, BASE_HEX, NULL, 0,
        "", HFILL }},
    { &hf_camel_detachSpecificInformation,
      { "detachSpecificInformation", "camel.detachSpecificInformation",
        FT_NONE, BASE_NONE, NULL, 0,
        "GPRSEventSpecificInformation/detachSpecificInformation", HFILL }},
    { &hf_camel_inititatingEntity,
      { "inititatingEntity", "camel.inititatingEntity",
        FT_UINT32, BASE_DEC, VALS(camel_InitiatingEntity_vals), 0,
        "", HFILL }},
    { &hf_camel_routeingAreaUpdate,
      { "routeingAreaUpdate", "camel.routeingAreaUpdate",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_camel_disconnectSpecificInformation,
      { "disconnectSpecificInformation", "camel.disconnectSpecificInformation",
        FT_NONE, BASE_NONE, NULL, 0,
        "GPRSEventSpecificInformation/disconnectSpecificInformation", HFILL }},
    { &hf_camel_pDPContextEstablishmentSpecificInformation,
      { "pDPContextEstablishmentSpecificInformation", "camel.pDPContextEstablishmentSpecificInformation",
        FT_NONE, BASE_NONE, NULL, 0,
        "GPRSEventSpecificInformation/pDPContextEstablishmentSpecificInformation", HFILL }},
    { &hf_camel_pDPInitiationType,
      { "pDPInitiationType", "camel.pDPInitiationType",
        FT_UINT32, BASE_DEC, VALS(camel_PDPInitiationType_vals), 0,
        "", HFILL }},
    { &hf_camel_secondaryPDPContext,
      { "secondaryPDPContext", "camel.secondaryPDPContext",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_camel_pDPContextEstablishmentAcknowledgementSpecificInformation,
      { "pDPContextEstablishmentAcknowledgementSpecificInformation", "camel.pDPContextEstablishmentAcknowledgementSpecificInformation",
        FT_NONE, BASE_NONE, NULL, 0,
        "GPRSEventSpecificInformation/pDPContextEstablishmentAcknowledgementSpecificInformation", HFILL }},
    { &hf_camel_miscGPRSInfo,
      { "miscGPRSInfo", "camel.miscGPRSInfo",
        FT_NONE, BASE_NONE, NULL, 0,
        "EventReportGPRSArg/miscGPRSInfo", HFILL }},
    { &hf_camel_gPRSEventSpecificInformation,
      { "gPRSEventSpecificInformation", "camel.gPRSEventSpecificInformation",
        FT_UINT32, BASE_DEC, VALS(camel_GPRSEventSpecificInformation_vals), 0,
        "EventReportGPRSArg/gPRSEventSpecificInformation", HFILL }},
    { &hf_camel_type,
      { "type", "camel.type",
        FT_UINT32, BASE_DEC, VALS(camel_Code_vals), 0,
        "ExtensionField/type", HFILL }},
    { &hf_camel_criticality,
      { "criticality", "camel.criticality",
        FT_UINT32, BASE_DEC, VALS(camel_CriticalityType_vals), 0,
        "ExtensionField/criticality", HFILL }},
    { &hf_camel_value,
      { "value", "camel.value",
        FT_UINT32, BASE_DEC, VALS(camel_SupportedExtensionsExtensionType_vals), 0,
        "ExtensionField/value", HFILL }},
    { &hf_camel_aChBillingChargingCharacteristics,
      { "aChBillingChargingCharacteristics", "camel.aChBillingChargingCharacteristics",
        FT_BYTES, BASE_HEX, NULL, 0,
        "ApplyChargingArg/aChBillingChargingCharacteristics", HFILL }},
    { &hf_camel_extensions,
      { "extensions", "camel.extensions",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_camel_ExtensionsArray_item,
      { "Item", "camel.ExtensionsArray_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "ExtensionsArray/_item", HFILL }},
    { &hf_camel_correlationID,
      { "correlationID", "camel.correlationID",
        FT_BYTES, BASE_HEX, NULL, 0,
        "", HFILL }},
    { &hf_camel_iPSSPCapabilities,
      { "iPSSPCapabilities", "camel.iPSSPCapabilities",
        FT_BYTES, BASE_HEX, NULL, 0,
        "", HFILL }},
    { &hf_camel_requestedInformationTypeList,
      { "requestedInformationTypeList", "camel.requestedInformationTypeList",
        FT_NONE, BASE_NONE, NULL, 0,
        "CallInformationRequestArg/requestedInformationTypeList", HFILL }},
    { &hf_camel_legID1,
      { "legID", "camel.legID",
        FT_UINT32, BASE_DEC, VALS(camel_SendingSideID_vals), 0,
        "CallInformationRequestArg/legID", HFILL }},
    { &hf_camel_destinationRoutingAddress,
      { "destinationRoutingAddress", "camel.destinationRoutingAddress",
        FT_NONE, BASE_NONE, NULL, 0,
        "ConnectArg/destinationRoutingAddress", HFILL }},
    { &hf_camel_alertingPattern,
      { "alertingPattern", "camel.alertingPattern",
        FT_BYTES, BASE_HEX, NULL, 0,
        "", HFILL }},
    { &hf_camel_originalCalledPartyID,
      { "originalCalledPartyID", "camel.originalCalledPartyID",
        FT_BYTES, BASE_HEX, NULL, 0,
        "", HFILL }},
    { &hf_camel_carrier,
      { "carrier", "camel.carrier",
        FT_BYTES, BASE_HEX, NULL, 0,
        "", HFILL }},
    { &hf_camel_callingPartysCategory,
      { "callingPartysCategory", "camel.callingPartysCategory",
        FT_BYTES, BASE_HEX, NULL, 0,
        "", HFILL }},
    { &hf_camel_redirectingPartyID,
      { "redirectingPartyID", "camel.redirectingPartyID",
        FT_BYTES, BASE_HEX, NULL, 0,
        "", HFILL }},
    { &hf_camel_redirectionInformation,
      { "redirectionInformation", "camel.redirectionInformation",
        FT_BYTES, BASE_HEX, NULL, 0,
        "", HFILL }},
    { &hf_camel_genericNumbers,
      { "genericNumbers", "camel.genericNumbers",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_camel_serviceInteractionIndicatorsTwo,
      { "serviceInteractionIndicatorsTwo", "camel.serviceInteractionIndicatorsTwo",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_camel_chargeNumber,
      { "chargeNumber", "camel.chargeNumber",
        FT_BYTES, BASE_HEX, NULL, 0,
        "", HFILL }},
    { &hf_camel_cug_Interlock,
      { "cug-Interlock", "camel.cug_Interlock",
        FT_BYTES, BASE_HEX, NULL, 0,
        "", HFILL }},
    { &hf_camel_cug_OutgoingAccess,
      { "cug-OutgoingAccess", "camel.cug_OutgoingAccess",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_camel_suppressionOfAnnouncement,
      { "suppressionOfAnnouncement", "camel.suppressionOfAnnouncement",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_camel_oCSIApplicable,
      { "oCSIApplicable", "camel.oCSIApplicable",
        FT_NONE, BASE_NONE, NULL, 0,
        "ConnectArg/oCSIApplicable", HFILL }},
    { &hf_camel_naOliInfo,
      { "naOliInfo", "camel.naOliInfo",
        FT_BYTES, BASE_HEX, NULL, 0,
        "", HFILL }},
    { &hf_camel_resourceAddress,
      { "resourceAddress", "camel.resourceAddress",
        FT_UINT32, BASE_DEC, VALS(camel_T_resourceAddress_vals), 0,
        "ConnectToResourceArg/resourceAddress", HFILL }},
    { &hf_camel_ipRoutingAddress,
      { "ipRoutingAddress", "camel.ipRoutingAddress",
        FT_BYTES, BASE_HEX, NULL, 0,
        "ConnectToResourceArg/resourceAddress/ipRoutingAddress", HFILL }},
    { &hf_camel_none,
      { "none", "camel.none",
        FT_NONE, BASE_NONE, NULL, 0,
        "ConnectToResourceArg/resourceAddress/none", HFILL }},
    { &hf_camel_assistingSSPIPRoutingAddress,
      { "assistingSSPIPRoutingAddress", "camel.assistingSSPIPRoutingAddress",
        FT_BYTES, BASE_HEX, NULL, 0,
        "EstablishTemporaryConnectionArg/assistingSSPIPRoutingAddress", HFILL }},
    { &hf_camel_eventSpecificInformationBCSM,
      { "eventSpecificInformationBCSM", "camel.eventSpecificInformationBCSM",
        FT_UINT32, BASE_DEC, VALS(camel_EventSpecificInformationBCSM_vals), 0,
        "EventReportBCSMArg/eventSpecificInformationBCSM", HFILL }},
    { &hf_camel_legID2,
      { "legID", "camel.legID",
        FT_UINT32, BASE_DEC, VALS(camel_ReceivingSideID_vals), 0,
        "", HFILL }},
    { &hf_camel_miscCallInfo,
      { "miscCallInfo", "camel.miscCallInfo",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_camel_timerID,
      { "timerID", "camel.timerID",
        FT_UINT32, BASE_DEC, VALS(camel_TimerID_vals), 0,
        "", HFILL }},
    { &hf_camel_timervalue,
      { "timervalue", "camel.timervalue",
        FT_UINT32, BASE_DEC, NULL, 0,
        "", HFILL }},
    { &hf_camel_sCIBillingChargingCharacteristics,
      { "sCIBillingChargingCharacteristics", "camel.sCIBillingChargingCharacteristics",
        FT_BYTES, BASE_HEX, NULL, 0,
        "SendChargingInformationArg/sCIBillingChargingCharacteristics", HFILL }},
    { &hf_camel_eventSpecificInformationSMS,
      { "eventSpecificInformationSMS", "camel.eventSpecificInformationSMS",
        FT_UINT32, BASE_DEC, VALS(camel_EventSpecificInformationSMS_vals), 0,
        "EventReportSMSArg/eventSpecificInformationSMS", HFILL }},
    { &hf_camel_sMSEvents,
      { "sMSEvents", "camel.sMSEvents",
        FT_NONE, BASE_NONE, NULL, 0,
        "RequestReportSMSEventArg/sMSEvents", HFILL }},
    { &hf_camel_SMSEventArray_item,
      { "Item", "camel.SMSEventArray_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "SMSEventArray/_item", HFILL }},
    { &hf_camel_timeDurationCharging,
      { "timeDurationCharging", "camel.timeDurationCharging",
        FT_NONE, BASE_NONE, NULL, 0,
        "CamelAChBillingChargingCharacteristics/timeDurationCharging", HFILL }},
    { &hf_camel_maxCallPeriodDuration,
      { "maxCallPeriodDuration", "camel.maxCallPeriodDuration",
        FT_UINT32, BASE_DEC, NULL, 0,
        "CamelAChBillingChargingCharacteristics/timeDurationCharging/maxCallPeriodDuration", HFILL }},
    { &hf_camel_releaseIfdurationExceeded,
      { "releaseIfdurationExceeded", "camel.releaseIfdurationExceeded",
        FT_BOOLEAN, 8, NULL, 0,
        "CamelAChBillingChargingCharacteristics/timeDurationCharging/releaseIfdurationExceeded", HFILL }},
    { &hf_camel_tone1,
      { "tone", "camel.tone",
        FT_BOOLEAN, 8, NULL, 0,
        "CamelAChBillingChargingCharacteristics/timeDurationCharging/tone", HFILL }},
    { &hf_camel_timeDurationChargingResult,
      { "timeDurationChargingResult", "camel.timeDurationChargingResult",
        FT_NONE, BASE_NONE, NULL, 0,
        "CamelCallResult/timeDurationChargingResult", HFILL }},
    { &hf_camel_partyToCharge1,
      { "partyToCharge", "camel.partyToCharge",
        FT_UINT32, BASE_DEC, VALS(camel_ReceivingSideID_vals), 0,
        "CamelCallResult/timeDurationChargingResult/partyToCharge", HFILL }},
    { &hf_camel_timeInformation,
      { "timeInformation", "camel.timeInformation",
        FT_UINT32, BASE_DEC, VALS(camel_TimeInformation_vals), 0,
        "CamelCallResult/timeDurationChargingResult/timeInformation", HFILL }},
    { &hf_camel_callActive,
      { "callActive", "camel.callActive",
        FT_BOOLEAN, 8, NULL, 0,
        "CamelCallResult/timeDurationChargingResult/callActive", HFILL }},
    { &hf_camel_callReleasedAtTcpExpiry,
      { "callReleasedAtTcpExpiry", "camel.callReleasedAtTcpExpiry",
        FT_NONE, BASE_NONE, NULL, 0,
        "CamelCallResult/timeDurationChargingResult/callReleasedAtTcpExpiry", HFILL }},
    { &hf_camel_bcsmEvents,
      { "bcsmEvents", "camel.bcsmEvents",
        FT_NONE, BASE_NONE, NULL, 0,
        "RequestReportBCSMEventArg/bcsmEvents", HFILL }},
    { &hf_camel_BCSMEventArray_item,
      { "Item", "camel.BCSMEventArray_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "BCSMEventArray/_item", HFILL }},
    { &hf_camel_callingPartysNumber,
      { "callingPartysNumber", "camel.callingPartysNumber",
        FT_BYTES, BASE_HEX, NULL, 0,
        "ConnectSMSArg/callingPartysNumber", HFILL }},
    { &hf_camel_destinationSubscriberNumber,
      { "destinationSubscriberNumber", "camel.destinationSubscriberNumber",
        FT_BYTES, BASE_HEX, NULL, 0,
        "", HFILL }},
    { &hf_camel_sMSCAddress,
      { "sMSCAddress", "camel.sMSCAddress",
        FT_BYTES, BASE_HEX, NULL, 0,
        "", HFILL }},
    { &hf_camel_requestedInformationList,
      { "requestedInformationList", "camel.requestedInformationList",
        FT_NONE, BASE_NONE, NULL, 0,
        "CallInformationReportArg/requestedInformationList", HFILL }},
    { &hf_camel_disconnectFromIPForbidden,
      { "disconnectFromIPForbidden", "camel.disconnectFromIPForbidden",
        FT_BOOLEAN, 8, NULL, 0,
        "", HFILL }},
    { &hf_camel_requestAnnouncementComplete,
      { "requestAnnouncementComplete", "camel.requestAnnouncementComplete",
        FT_BOOLEAN, 8, NULL, 0,
        "PlayAnnouncementArg/requestAnnouncementComplete", HFILL }},
    { &hf_camel_collectedInfo,
      { "collectedInfo", "camel.collectedInfo",
        FT_UINT32, BASE_DEC, VALS(camel_CollectedInfo_vals), 0,
        "PromptAndCollectUserInformationArg/collectedInfo", HFILL }},
    { &hf_camel_mSISDN,
      { "mSISDN", "camel.mSISDN",
        FT_BYTES, BASE_HEX, NULL, 0,
        "InitialDPGPRSArg/mSISDN", HFILL }},
    { &hf_camel_iMSI,
      { "iMSI", "camel.iMSI",
        FT_BYTES, BASE_HEX, NULL, 0,
        "", HFILL }},
    { &hf_camel_gPRSMSClass,
      { "gPRSMSClass", "camel.gPRSMSClass",
        FT_NONE, BASE_NONE, NULL, 0,
        "InitialDPGPRSArg/gPRSMSClass", HFILL }},
    { &hf_camel_sGSNCapabilities,
      { "sGSNCapabilities", "camel.sGSNCapabilities",
        FT_BYTES, BASE_HEX, NULL, 0,
        "InitialDPGPRSArg/sGSNCapabilities", HFILL }},
    { &hf_camel_gapCriteria,
      { "gapCriteria", "camel.gapCriteria",
        FT_UINT32, BASE_DEC, VALS(camel_GapCriteria_vals), 0,
        "CallGapArg/gapCriteria", HFILL }},
    { &hf_camel_gapIndicators,
      { "gapIndicators", "camel.gapIndicators",
        FT_NONE, BASE_NONE, NULL, 0,
        "CallGapArg/gapIndicators", HFILL }},
    { &hf_camel_controlType,
      { "controlType", "camel.controlType",
        FT_UINT32, BASE_DEC, VALS(camel_ControlType_vals), 0,
        "CallGapArg/controlType", HFILL }},
    { &hf_camel_gapTreatment,
      { "gapTreatment", "camel.gapTreatment",
        FT_UINT32, BASE_DEC, VALS(camel_GapTreatment_vals), 0,
        "CallGapArg/gapTreatment", HFILL }},
    { &hf_camel_calledPartyNumber,
      { "calledPartyNumber", "camel.calledPartyNumber",
        FT_BYTES, BASE_HEX, NULL, 0,
        "InitialDPArg/calledPartyNumber", HFILL }},
    { &hf_camel_callingPartyNumber,
      { "callingPartyNumber", "camel.callingPartyNumber",
        FT_BYTES, BASE_HEX, NULL, 0,
        "InitialDPArg/callingPartyNumber", HFILL }},
    { &hf_camel_cGEncountered,
      { "cGEncountered", "camel.cGEncountered",
        FT_UINT32, BASE_DEC, VALS(camel_CGEncountered_vals), 0,
        "InitialDPArg/cGEncountered", HFILL }},
    { &hf_camel_highLayerCompatibility,
      { "highLayerCompatibility", "camel.highLayerCompatibility",
        FT_BYTES, BASE_HEX, NULL, 0,
        "InitialDPArg/highLayerCompatibility", HFILL }},
    { &hf_camel_additionalCallingPartyNumber,
      { "additionalCallingPartyNumber", "camel.additionalCallingPartyNumber",
        FT_BYTES, BASE_HEX, NULL, 0,
        "InitialDPArg/additionalCallingPartyNumber", HFILL }},
    { &hf_camel_bearerCapability,
      { "bearerCapability", "camel.bearerCapability",
        FT_UINT32, BASE_DEC, VALS(camel_BearerCapability_vals), 0,
        "InitialDPArg/bearerCapability", HFILL }},
    { &hf_camel_cause,
      { "cause", "camel.cause",
        FT_BYTES, BASE_HEX, NULL, 0,
        "InitialDPArg/cause", HFILL }},
    { &hf_camel_cug_Index,
      { "cug-Index", "camel.cug_Index",
        FT_UINT32, BASE_DEC, NULL, 0,
        "InitialDPArg/cug-Index", HFILL }},
    { &hf_camel_subscriberState,
      { "subscriberState", "camel.subscriberState",
        FT_UINT32, BASE_DEC, VALS(camel_SubscriberState_vals), 0,
        "InitialDPArg/subscriberState", HFILL }},
    { &hf_camel_locationInformation,
      { "locationInformation", "camel.locationInformation",
        FT_NONE, BASE_NONE, NULL, 0,
        "InitialDPArg/locationInformation", HFILL }},
    { &hf_camel_ext_basicServiceCode,
      { "ext-basicServiceCode", "camel.ext_basicServiceCode",
        FT_UINT32, BASE_DEC, VALS(camel_Ext_BasicServiceCode_vals), 0,
        "InitialDPArg/ext-basicServiceCode", HFILL }},
    { &hf_camel_callReferenceNumber,
      { "callReferenceNumber", "camel.callReferenceNumber",
        FT_BYTES, BASE_HEX, NULL, 0,
        "InitialDPArg/callReferenceNumber", HFILL }},
    { &hf_camel_mscAddress,
      { "mscAddress", "camel.mscAddress",
        FT_BYTES, BASE_HEX, NULL, 0,
        "", HFILL }},
    { &hf_camel_calledPartyBCDNumber,
      { "calledPartyBCDNumber", "camel.calledPartyBCDNumber",
        FT_BYTES, BASE_HEX, NULL, 0,
        "InitialDPArg/calledPartyBCDNumber", HFILL }},
    { &hf_camel_timeAndTimezone,
      { "timeAndTimezone", "camel.timeAndTimezone",
        FT_BYTES, BASE_HEX, NULL, 0,
        "", HFILL }},
    { &hf_camel_gsm_ForwardingPending,
      { "gsm-ForwardingPending", "camel.gsm_ForwardingPending",
        FT_NONE, BASE_NONE, NULL, 0,
        "InitialDPArg/gsm-ForwardingPending", HFILL }},
    { &hf_camel_initialDPArgExtension,
      { "initialDPArgExtension", "camel.initialDPArgExtension",
        FT_NONE, BASE_NONE, NULL, 0,
        "InitialDPArg/initialDPArgExtension", HFILL }},
    { &hf_camel_callingPartyNumber1,
      { "callingPartyNumber", "camel.callingPartyNumber",
        FT_BYTES, BASE_HEX, NULL, 0,
        "InitialDPSMSArg/callingPartyNumber", HFILL }},
    { &hf_camel_locationInformationMSC,
      { "locationInformationMSC", "camel.locationInformationMSC",
        FT_NONE, BASE_NONE, NULL, 0,
        "InitialDPSMSArg/locationInformationMSC", HFILL }},
    { &hf_camel_tPShortMessageSubmissionSpecificInfo,
      { "tPShortMessageSubmissionSpecificInfo", "camel.tPShortMessageSubmissionSpecificInfo",
        FT_BYTES, BASE_HEX, NULL, 0,
        "InitialDPSMSArg/tPShortMessageSubmissionSpecificInfo", HFILL }},
    { &hf_camel_tPProtocolIdentifier,
      { "tPProtocolIdentifier", "camel.tPProtocolIdentifier",
        FT_BYTES, BASE_HEX, NULL, 0,
        "InitialDPSMSArg/tPProtocolIdentifier", HFILL }},
    { &hf_camel_tPDataCodingScheme,
      { "tPDataCodingScheme", "camel.tPDataCodingScheme",
        FT_BYTES, BASE_HEX, NULL, 0,
        "InitialDPSMSArg/tPDataCodingScheme", HFILL }},
    { &hf_camel_tPValidityPeriod,
      { "tPValidityPeriod", "camel.tPValidityPeriod",
        FT_BYTES, BASE_HEX, NULL, 0,
        "InitialDPSMSArg/tPValidityPeriod", HFILL }},
    { &hf_camel_smsReferenceNumber,
      { "smsReferenceNumber", "camel.smsReferenceNumber",
        FT_BYTES, BASE_HEX, NULL, 0,
        "InitialDPSMSArg/smsReferenceNumber", HFILL }},
    { &hf_camel_sgsnNumber,
      { "sgsnNumber", "camel.sgsnNumber",
        FT_BYTES, BASE_HEX, NULL, 0,
        "InitialDPSMSArg/sgsnNumber", HFILL }},
    { &hf_camel_privateExtensionList,
      { "privateExtensionList", "camel.privateExtensionList",
        FT_NONE, BASE_NONE, NULL, 0,
        "ExtensionContainer/privateExtensionList", HFILL }},
    { &hf_camel_pcs_Extensions,
      { "pcs-Extensions", "camel.pcs_Extensions",
        FT_NONE, BASE_NONE, NULL, 0,
        "ExtensionContainer/pcs-Extensions", HFILL }},
    { &hf_camel_aOCBeforeAnswer,
      { "aOCBeforeAnswer", "camel.aOCBeforeAnswer",
        FT_NONE, BASE_NONE, NULL, 0,
        "CamelSCIBillingChargingCharacteristics/aOCBeforeAnswer", HFILL }},
    { &hf_camel_aOCAfterAnswer,
      { "aOCAfterAnswer", "camel.aOCAfterAnswer",
        FT_NONE, BASE_NONE, NULL, 0,
        "CamelSCIBillingChargingCharacteristics/aOCAfterAnswer", HFILL }},
    { &hf_camel_aOCGPRS,
      { "aOCGPRS", "camel.aOCGPRS",
        FT_NONE, BASE_NONE, NULL, 0,
        "CamelSCIGPRSBillingChargingCharacteristics/aOCGPRS", HFILL }},
    { &hf_camel_chargingCharacteristics,
      { "chargingCharacteristics", "camel.chargingCharacteristics",
        FT_UINT32, BASE_DEC, VALS(camel_ChargingCharacteristics_vals), 0,
        "ApplyChargingGPRSArg/chargingCharacteristics", HFILL }},
    { &hf_camel_chargingResult,
      { "chargingResult", "camel.chargingResult",
        FT_UINT32, BASE_DEC, VALS(camel_ChargingResult_vals), 0,
        "ApplyChargingReportGPRSArg/chargingResult", HFILL }},
    { &hf_camel_active,
      { "active", "camel.active",
        FT_BOOLEAN, 8, NULL, 0,
        "ApplyChargingReportGPRSArg/active", HFILL }},
    { &hf_camel_chargingRollOver,
      { "chargingRollOver", "camel.chargingRollOver",
        FT_UINT32, BASE_DEC, VALS(camel_ChargingRollOver_vals), 0,
        "ApplyChargingReportGPRSArg/chargingRollOver", HFILL }},

/*--- End of included file: packet-camel-hfarr.c ---*/

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

/*--- Included file: packet-camel-ettarr.c ---*/

    &ett_camel_PBSGSNCapabilities,
    &ett_camel_PBIPSSPCapabilities,
    &ett_camel_PBAddressString,
    &ett_camel_PBISDNAddressString,
    &ett_camel_PBGeographicalInformation,
    &ett_camel_PBGSNAddress,
    &ett_camel_PBRedirectionInformation,
    &ett_camel_PBCalledPartyNumber,
    &ett_camel_PBCallingPartyNumber,
    &ett_camel_PBRedirectingNumber,
    &ett_camel_PBCause,
    &ett_camel_PBGenericNumber,
    &ett_camel_PBLocationNumber,
    &ett_camel_PBCalledPartyBCDNumber,
    &ett_camel_CAPGPRSReferenceNumber,
    &ett_camel_ChargingRollOver,
    &ett_camel_BackwardServiceInteractionInd,
    &ett_camel_BearerCapability,
    &ett_camel_CAI_Gsm0224,
    &ett_camel_ChargingCharacteristics,
    &ett_camel_ElapsedTime,
    &ett_camel_T_timeGPRSIfTariffSwitch,
    &ett_camel_ElapsedTimeRollOver,
    &ett_camel_T_rOTimeGPRSIfTariffSwitch,
    &ett_camel_ForwardServiceInteractionInd,
    &ett_camel_PDPType,
    &ett_camel_TimeIfTariffSwitch,
    &ett_camel_TransferredVolume,
    &ett_camel_T_volumeIfTariffSwitch,
    &ett_camel_TransferredVolumeRollOver,
    &ett_camel_T_rOVolumeIfTariffSwitch,
    &ett_camel_Code,
    &ett_camel_PCS_Extensions,
    &ett_camel_MiscCallInfo,
    &ett_camel_SupportedExtensionsExtensionType,
    &ett_camel_PrivateExtension,
    &ett_camel_AOCSubsequent,
    &ett_camel_CamelFCIGPRSBillingChargingCharacteristics,
    &ett_camel_T_fCIBCCCAMELsequence1,
    &ett_camel_CamelFCISMSBillingChargingCharacteristics,
    &ett_camel_T_fCIBCCCAMELsequence11,
    &ett_camel_ChargingResult,
    &ett_camel_CollectedDigits,
    &ett_camel_CollectedInfo,
    &ett_camel_DestinationRoutingAddress,
    &ett_camel_DpSpecificCriteria,
    &ett_camel_EventSpecificInformationBCSM,
    &ett_camel_T_routeSelectFailureSpecificInfo,
    &ett_camel_T_oCalledPartyBusySpecificInfo,
    &ett_camel_T_oNoAnswerSpecificInfo,
    &ett_camel_T_oAnswerSpecificInfo,
    &ett_camel_T_oDisconnectSpecificInfo,
    &ett_camel_T_tBusySpecificInfo,
    &ett_camel_T_tNoAnswerSpecificInfo,
    &ett_camel_T_tAnswerSpecificInfo,
    &ett_camel_T_tDisconnectSpecificInfo,
    &ett_camel_EventSpecificInformationSMS,
    &ett_camel_T_o_smsFailureSpecificInfo,
    &ett_camel_T_o_smsSubmittedSpecificInfo,
    &ett_camel_GapIndicators,
    &ett_camel_GapOnService,
    &ett_camel_GenericNumbers,
    &ett_camel_GPRS_QoS,
    &ett_camel_GPRSEvent,
    &ett_camel_GPRSMSClass,
    &ett_camel_QualityOfService,
    &ett_camel_ReceivingSideID,
    &ett_camel_RequestedInformationTypeList,
    &ett_camel_RequestedInformationValue,
    &ett_camel_SendingSideID,
    &ett_camel_ServiceInteractionIndicatorsTwo,
    &ett_camel_SMSEvent,
    &ett_camel_TimeInformation,
    &ett_camel_Tone,
    &ett_camel_VariablePart,
    &ett_camel_CancelArg,
    &ett_camel_ReceivedInformationArg,
    &ett_camel_ConnectGPRSArg,
    &ett_camel_EntityReleasedGPRSArg,
    &ett_camel_ReleaseGPRSArg,
    &ett_camel_RequestReportGPRSEventArg,
    &ett_camel_GPRSEventArray,
    &ett_camel_SendChargingInformationGPRSArg,
    &ett_camel_SubscriberState,
    &ett_camel_PrivateExtensionList,
    &ett_camel_CellIdOrLAI,
    &ett_camel_Ext_BasicServiceCode,
    &ett_camel_LegID,
    &ett_camel_AOCBeforeAnswer,
    &ett_camel_AOCGprs,
    &ett_camel_BasicGapCriteria,
    &ett_camel_T_calledAddressAndService,
    &ett_camel_T_callingAddressAndService,
    &ett_camel_BCSMEvent,
    &ett_camel_CamelFCIBillingChargingCharacteristics,
    &ett_camel_T_fCIBCCCAMELsequence12,
    &ett_camel_CompoundCriteria,
    &ett_camel_GapCriteria,
    &ett_camel_MessageID,
    &ett_camel_T_text,
    &ett_camel_SEQUENCE_SIZE_1_16_OF_Integer4,
    &ett_camel_T_variableMessage,
    &ett_camel_VariablePartsArray,
    &ett_camel_RequestedInformation,
    &ett_camel_InitialDPArgExtension,
    &ett_camel_InbandInfo,
    &ett_camel_InformationToSend,
    &ett_camel_LocationInformationGPRS,
    &ett_camel_RequestedInformationList,
    &ett_camel_LocationInformation,
    &ett_camel_GapTreatment,
    &ett_camel_GPRSEventSpecificInformation,
    &ett_camel_T_attachChangeOfPositionSpecificInformation,
    &ett_camel_T_pdp_ContextchangeOfPositionSpecificInformation,
    &ett_camel_T_detachSpecificInformation,
    &ett_camel_T_disconnectSpecificInformation,
    &ett_camel_T_pDPContextEstablishmentSpecificInformation,
    &ett_camel_T_pDPContextEstablishmentAcknowledgementSpecificInformation,
    &ett_camel_EventReportGPRSArg,
    &ett_camel_ExtensionField,
    &ett_camel_ApplyChargingArg,
    &ett_camel_ExtensionsArray,
    &ett_camel_AssistRequestInstructionsArg,
    &ett_camel_CallInformationRequestArg,
    &ett_camel_ConnectArg,
    &ett_camel_ConnectToResourceArg,
    &ett_camel_T_resourceAddress,
    &ett_camel_ContinueWithArgumentArg,
    &ett_camel_EstablishTemporaryConnectionArg,
    &ett_camel_EventReportBCSMArg,
    &ett_camel_ResetTimerArg,
    &ett_camel_SendChargingInformationArg,
    &ett_camel_EventReportSMSArg,
    &ett_camel_RequestReportSMSEventArg,
    &ett_camel_SMSEventArray,
    &ett_camel_ResetTimerSMSArg,
    &ett_camel_CamelAChBillingChargingCharacteristics,
    &ett_camel_T_timeDurationCharging,
    &ett_camel_CamelCallResult,
    &ett_camel_T_timeDurationChargingResult,
    &ett_camel_RequestReportBCSMEventArg,
    &ett_camel_BCSMEventArray,
    &ett_camel_ConnectSMSArg,
    &ett_camel_CallInformationReportArg,
    &ett_camel_PlayAnnouncementArg,
    &ett_camel_PromptAndCollectUserInformationArg,
    &ett_camel_InitialDPGPRSArg,
    &ett_camel_CallGapArg,
    &ett_camel_InitialDPArg,
    &ett_camel_InitialDPSMSArg,
    &ett_camel_ExtensionContainer,
    &ett_camel_CamelSCIBillingChargingCharacteristics,
    &ett_camel_CamelSCIGPRSBillingChargingCharacteristics,
    &ett_camel_ApplyChargingGPRSArg,
    &ett_camel_ApplyChargingReportGPRSArg,
    &ett_camel_CancelGPRSArg,
    &ett_camel_ContinueGPRSArg,
    &ett_camel_ResetTimerGPRSArg,

/*--- End of included file: packet-camel-ettarr.c ---*/

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

