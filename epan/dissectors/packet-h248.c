/* Do not modify this file.                                                   */
/* It is created automatically by the ASN.1 to Wireshark dissector compiler   */
/* .\packet-h248.c                                                            */
/* ../../tools/asn2wrs.py -b -e -p h248 -c h248.cnf -s packet-h248-template h248v3.asn */

/* Input file: packet-h248-template.c */

#line 1 "packet-h248-template.c"
/* packet-h248.c
 * Routines for H.248/MEGACO packet dissection
 *
 * Ronnie Sahlberg 2004
 *
 * Luis Ontanon 2005 - Context and Transaction Tracing
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "packet-h248.h"
#include "tap.h"
#include "packet-tpkt.h"

#define PNAME  "H.248 MEGACO"
#define PSNAME "H248"
#define PFNAME "h248"

#define GATEWAY_CONTROL_PROTOCOL_USER_ID 14


/* Initialize the protocol and registered fields */
static int proto_h248				= -1;
static int hf_h248_mtpaddress_ni	= -1;
static int hf_h248_mtpaddress_pc	= -1;
static int hf_h248_pkg_name		= -1;
static int hf_248_pkg_param = -1;
static int hf_h248_event_name		= -1;
static int hf_h248_signal_name		= -1;
static int hf_h248_signal_code		= -1;
static int hf_h248_event_code		= -1;
static int hf_h248_pkg_bcp_BNCChar_PDU = -1;



static int hf_h248_context_id = -1;
static int hf_h248_error_code = -1;
static int hf_h248_term_wild_type = -1;
static int hf_h248_term_wild_level = -1;
static int hf_h248_term_wild_position = -1;

static int hf_h248_no_pkg = -1;
static int hf_h248_no_sig = -1;
static int hf_h248_no_evt = -1;
static int hf_h248_param = -1;

static int hf_h248_serviceChangeReasonStr = -1;


/*--- Included file: packet-h248-hf.c ---*/
#line 1 "packet-h248-hf.c"
static int hf_h248_authHeader = -1;               /* AuthenticationHeader */
static int hf_h248_mess = -1;                     /* Message */
static int hf_h248_secParmIndex = -1;             /* SecurityParmIndex */
static int hf_h248_seqNum = -1;                   /* SequenceNum */
static int hf_h248_ad = -1;                       /* AuthData */
static int hf_h248_version = -1;                  /* INTEGER_0_99 */
static int hf_h248_mId = -1;                      /* MId */
static int hf_h248_messageBody = -1;              /* T_messageBody */
static int hf_h248_messageError = -1;             /* ErrorDescriptor */
static int hf_h248_transactions = -1;             /* SEQUENCE_OF_Transaction */
static int hf_h248_transactions_item = -1;        /* Transaction */
static int hf_h248_ip4Address = -1;               /* IP4Address */
static int hf_h248_ip6Address = -1;               /* IP6Address */
static int hf_h248_domainName = -1;               /* DomainName */
static int hf_h248_deviceName = -1;               /* PathName */
static int hf_h248_mtpAddress = -1;               /* MtpAddress */
static int hf_h248_domName = -1;                  /* IA5String */
static int hf_h248_portNumber = -1;               /* INTEGER_0_65535 */
static int hf_h248_iP4Address = -1;               /* OCTET_STRING_SIZE_4 */
static int hf_h248_iP6Address = -1;               /* OCTET_STRING_SIZE_16 */
static int hf_h248_transactionRequest = -1;       /* TransactionRequest */
static int hf_h248_transactionPending = -1;       /* TransactionPending */
static int hf_h248_transactionReply = -1;         /* TransactionReply */
static int hf_h248_transactionResponseAck = -1;   /* TransactionResponseAck */
static int hf_h248_segmentReply = -1;             /* SegmentReply */
static int hf_h248_transactionId = -1;            /* transactionId */
static int hf_h248_actions = -1;                  /* SEQUENCE_OF_ActionRequest */
static int hf_h248_actions_item = -1;             /* ActionRequest */
static int hf_h248_immAckRequired = -1;           /* NULL */
static int hf_h248_transactionResult = -1;        /* T_transactionResult */
static int hf_h248_transactionError = -1;         /* ErrorDescriptor */
static int hf_h248_actionReplies = -1;            /* SEQUENCE_OF_ActionReply */
static int hf_h248_actionReplies_item = -1;       /* ActionReply */
static int hf_h248_segmentNumber = -1;            /* SegmentNumber */
static int hf_h248_segmentationComplete = -1;     /* NULL */
static int hf_h248_transactionId1 = -1;           /* TransactionId */
static int hf_h248_TransactionResponseAck_item = -1;  /* TransactionAck */
static int hf_h248_firstAck = -1;                 /* TransactionId */
static int hf_h248_lastAck = -1;                  /* TransactionId */
static int hf_h248_errorCode = -1;                /* T_errorCode */
static int hf_h248_errorText = -1;                /* ErrorText */
static int hf_h248_contextId = -1;                /* contextId */
static int hf_h248_contextRequest = -1;           /* ContextRequest */
static int hf_h248_contextAttrAuditReq = -1;      /* T_contextAttrAuditReq */
static int hf_h248_commandRequests = -1;          /* SEQUENCE_OF_CommandRequest */
static int hf_h248_commandRequests_item = -1;     /* CommandRequest */
static int hf_h248_errorDescriptor = -1;          /* ErrorDescriptor */
static int hf_h248_contextReply = -1;             /* ContextRequest */
static int hf_h248_commandReply = -1;             /* SEQUENCE_OF_CommandReply */
static int hf_h248_commandReply_item = -1;        /* CommandReply */
static int hf_h248_priority = -1;                 /* INTEGER_0_15 */
static int hf_h248_emergency = -1;                /* BOOLEAN */
static int hf_h248_topologyReq = -1;              /* T_topologyReq */
static int hf_h248_topologyReq_item = -1;         /* TopologyRequest */
static int hf_h248_iepscallind = -1;              /* BOOLEAN */
static int hf_h248_contextProp = -1;              /* SEQUENCE_OF_PropertyParm */
static int hf_h248_contextProp_item = -1;         /* PropertyParm */
static int hf_h248_contextList = -1;              /* SEQUENCE_OF_ContextIDinList */
static int hf_h248_contextList_item = -1;         /* ContextIDinList */
static int hf_h248_topology = -1;                 /* NULL */
static int hf_h248_cAAREmergency = -1;            /* NULL */
static int hf_h248_cAARPriority = -1;             /* NULL */
static int hf_h248_iepscallind1 = -1;             /* NULL */
static int hf_h248_contextPropAud = -1;           /* SEQUENCE_OF_IndAudPropertyParm */
static int hf_h248_contextPropAud_item = -1;      /* IndAudPropertyParm */
static int hf_h248_selectpriority = -1;           /* INTEGER_0_15 */
static int hf_h248_selectemergency = -1;          /* BOOLEAN */
static int hf_h248_selectiepscallind = -1;        /* BOOLEAN */
static int hf_h248_selectLogic = -1;              /* SelectLogic */
static int hf_h248_andAUDITSelect = -1;           /* NULL */
static int hf_h248_orAUDITSelect = -1;            /* NULL */
static int hf_h248_command = -1;                  /* Command */
static int hf_h248_optional = -1;                 /* NULL */
static int hf_h248_wildcardReturn = -1;           /* NULL */
static int hf_h248_addReq = -1;                   /* T_addReq */
static int hf_h248_moveReq = -1;                  /* T_moveReq */
static int hf_h248_modReq = -1;                   /* T_modReq */
static int hf_h248_subtractReq = -1;              /* T_subtractReq */
static int hf_h248_auditCapRequest = -1;          /* T_auditCapRequest */
static int hf_h248_auditValueRequest = -1;        /* T_auditValueRequest */
static int hf_h248_notifyReq = -1;                /* T_notifyReq */
static int hf_h248_serviceChangeReq = -1;         /* ServiceChangeRequest */
static int hf_h248_addReply = -1;                 /* T_addReply */
static int hf_h248_moveReply = -1;                /* T_moveReply */
static int hf_h248_modReply = -1;                 /* T_modReply */
static int hf_h248_subtractReply = -1;            /* T_subtractReply */
static int hf_h248_auditCapReply = -1;            /* T_auditCapReply */
static int hf_h248_auditValueReply = -1;          /* T_auditValueReply */
static int hf_h248_notifyReply = -1;              /* T_notifyReply */
static int hf_h248_serviceChangeReply = -1;       /* ServiceChangeReply */
static int hf_h248_terminationFrom = -1;          /* TerminationID */
static int hf_h248_terminationTo = -1;            /* TerminationID */
static int hf_h248_topologyDirection = -1;        /* T_topologyDirection */
static int hf_h248_streamID = -1;                 /* StreamID */
static int hf_h248_topologyDirectionExtension = -1;  /* T_topologyDirectionExtension */
static int hf_h248_terminationIDList = -1;        /* TerminationIDList */
static int hf_h248_descriptors = -1;              /* SEQUENCE_OF_AmmDescriptor */
static int hf_h248_descriptors_item = -1;         /* AmmDescriptor */
static int hf_h248_mediaDescriptor = -1;          /* MediaDescriptor */
static int hf_h248_modemDescriptor = -1;          /* ModemDescriptor */
static int hf_h248_muxDescriptor = -1;            /* MuxDescriptor */
static int hf_h248_eventsDescriptor = -1;         /* EventsDescriptor */
static int hf_h248_eventBufferDescriptor = -1;    /* EventBufferDescriptor */
static int hf_h248_signalsDescriptor = -1;        /* SignalsDescriptor */
static int hf_h248_digitMapDescriptor = -1;       /* DigitMapDescriptor */
static int hf_h248_auditDescriptor = -1;          /* AuditDescriptor */
static int hf_h248_statisticsDescriptor = -1;     /* StatisticsDescriptor */
static int hf_h248_terminationAudit = -1;         /* TerminationAudit */
static int hf_h248_terminationID = -1;            /* TerminationID */
static int hf_h248_contextAuditResult = -1;       /* TerminationIDList */
static int hf_h248_error = -1;                    /* ErrorDescriptor */
static int hf_h248_auditResult = -1;              /* AuditResult */
static int hf_h248_auditResultTermList = -1;      /* TermListAuditResult */
static int hf_h248_terminationAuditResult = -1;   /* TerminationAudit */
static int hf_h248_TerminationAudit_item = -1;    /* AuditReturnParameter */
static int hf_h248_observedEventsDescriptor = -1;  /* ObservedEventsDescriptor */
static int hf_h248_packagesDescriptor = -1;       /* PackagesDescriptor */
static int hf_h248_emptyDescriptors = -1;         /* AuditDescriptor */
static int hf_h248_auditToken = -1;               /* T_auditToken */
static int hf_h248_auditPropertyToken = -1;       /* SEQUENCE_OF_IndAuditParameter */
static int hf_h248_auditPropertyToken_item = -1;  /* IndAuditParameter */
static int hf_h248_indaudmediaDescriptor = -1;    /* IndAudMediaDescriptor */
static int hf_h248_indaudeventsDescriptor = -1;   /* IndAudEventsDescriptor */
static int hf_h248_indaudeventBufferDescriptor = -1;  /* IndAudEventBufferDescriptor */
static int hf_h248_indaudsignalsDescriptor = -1;  /* IndAudSignalsDescriptor */
static int hf_h248_indauddigitMapDescriptor = -1;  /* IndAudDigitMapDescriptor */
static int hf_h248_indaudstatisticsDescriptor = -1;  /* IndAudStatisticsDescriptor */
static int hf_h248_indaudpackagesDescriptor = -1;  /* IndAudPackagesDescriptor */
static int hf_h248_indAudTerminationStateDescriptor = -1;  /* IndAudTerminationStateDescriptor */
static int hf_h248_indAudMediaDescriptorStreams = -1;  /* indAudMediaDescriptorStreams */
static int hf_h248_oneStream = -1;                /* IndAudStreamParms */
static int hf_h248_multiStream = -1;              /* SEQUENCE_OF_IndAudStreamDescriptor */
static int hf_h248_multiStream_item = -1;         /* IndAudStreamDescriptor */
static int hf_h248_indAudStreamParms = -1;        /* IndAudStreamParms */
static int hf_h248_iASPLocalControlDescriptor = -1;  /* IndAudLocalControlDescriptor */
static int hf_h248_iASPLocalDescriptor = -1;      /* IndAudLocalRemoteDescriptor */
static int hf_h248_iASPRemoteDescriptor = -1;     /* IndAudLocalRemoteDescriptor */
static int hf_h248_statisticsDescriptor1 = -1;    /* IndAudStatisticsDescriptor */
static int hf_h248_iALCDStreamMode = -1;          /* NULL */
static int hf_h248_iALCDReserveValue = -1;        /* NULL */
static int hf_h248_iALCDReserveGroup = -1;        /* NULL */
static int hf_h248_indAudPropertyParms = -1;      /* SEQUENCE_OF_IndAudPropertyParm */
static int hf_h248_indAudPropertyParms_item = -1;  /* IndAudPropertyParm */
static int hf_h248_streamModeSel = -1;            /* StreamMode */
static int hf_h248_name = -1;                     /* PkgdName */
static int hf_h248_propertyParms = -1;            /* PropertyParm */
static int hf_h248_propGroupID = -1;              /* INTEGER_0_65535 */
static int hf_h248_iAPropertyGroup = -1;          /* IndAudPropertyGroup */
static int hf_h248_IndAudPropertyGroup_item = -1;  /* IndAudPropertyParm */
static int hf_h248_eventBufferControl = -1;       /* NULL */
static int hf_h248_iATSDServiceState = -1;        /* NULL */
static int hf_h248_serviceStateSel = -1;          /* ServiceState */
static int hf_h248_requestID = -1;                /* RequestID */
static int hf_h248_iAEDPkgdName = -1;             /* PkgdName */
static int hf_h248_iAEBDEventName = -1;           /* PkgdName */
static int hf_h248_indAudSignal = -1;             /* IndAudSignal */
static int hf_h248_indAudSeqSigList = -1;         /* IndAudSeqSigList */
static int hf_h248_id = -1;                       /* INTEGER_0_65535 */
static int hf_h248_iASignalList = -1;             /* IndAudSignal */
static int hf_h248_iASignalName = -1;             /* PkgdName */
static int hf_h248_signalRequestID = -1;          /* RequestID */
static int hf_h248_digitMapName = -1;             /* DigitMapName */
static int hf_h248_iAStatName = -1;               /* PkgdName */
static int hf_h248_packageName = -1;              /* Name */
static int hf_h248_packageVersion = -1;           /* INTEGER_0_99 */
static int hf_h248_requestId = -1;                /* RequestID */
static int hf_h248_observedEventLst = -1;         /* SEQUENCE_OF_ObservedEvent */
static int hf_h248_observedEventLst_item = -1;    /* ObservedEvent */
static int hf_h248_eventName = -1;                /* EventName */
static int hf_h248_eventParList = -1;             /* SEQUENCE_OF_EventParameter */
static int hf_h248_eventParList_item = -1;        /* EventParameter */
static int hf_h248_timeNotation = -1;             /* TimeNotation */
static int hf_h248_eventParameterName = -1;       /* EventParameterName */
static int hf_h248_eventParamValue = -1;          /* EventParamValues */
static int hf_h248_extraInfo = -1;                /* T_extraInfo */
static int hf_h248_relation = -1;                 /* Relation */
static int hf_h248_range = -1;                    /* BOOLEAN */
static int hf_h248_sublist = -1;                  /* BOOLEAN */
static int hf_h248_EventParamValues_item = -1;    /* EventParamValue */
static int hf_h248_serviceChangeParms = -1;       /* ServiceChangeParm */
static int hf_h248_serviceChangeResult = -1;      /* ServiceChangeResult */
static int hf_h248_serviceChangeResParms = -1;    /* ServiceChangeResParm */
static int hf_h248_wildcard = -1;                 /* SEQUENCE_OF_WildcardField */
static int hf_h248_wildcard_item = -1;            /* WildcardField */
static int hf_h248_terminationId = -1;            /* T_terminationId */
static int hf_h248_TerminationIDList_item = -1;   /* TerminationID */
static int hf_h248_termStateDescr = -1;           /* TerminationStateDescriptor */
static int hf_h248_streams = -1;                  /* T_streams */
static int hf_h248_mediaDescriptorOneStream = -1;  /* StreamParms */
static int hf_h248_mediaDescriptorMultiStream = -1;  /* SEQUENCE_OF_StreamDescriptor */
static int hf_h248_mediaDescriptorMultiStream_item = -1;  /* StreamDescriptor */
static int hf_h248_streamParms = -1;              /* StreamParms */
static int hf_h248_localControlDescriptor = -1;   /* LocalControlDescriptor */
static int hf_h248_localDescriptor = -1;          /* LocalRemoteDescriptor */
static int hf_h248_remoteDescriptor = -1;         /* LocalRemoteDescriptor */
static int hf_h248_streamMode = -1;               /* StreamMode */
static int hf_h248_reserveValue = -1;             /* BOOLEAN */
static int hf_h248_reserveGroup = -1;             /* BOOLEAN */
static int hf_h248_propertyParms1 = -1;           /* SEQUENCE_OF_PropertyParm */
static int hf_h248_propertyParms_item = -1;       /* PropertyParm */
static int hf_h248_propertyName = -1;             /* PropertyName */
static int hf_h248_propertyParamValue = -1;       /* SEQUENCE_OF_PropertyID */
static int hf_h248_propertyParamValue_item = -1;  /* PropertyID */
static int hf_h248_extraInfo1 = -1;               /* T_extraInfo1 */
static int hf_h248_propGrps = -1;                 /* SEQUENCE_OF_PropertyGroup */
static int hf_h248_propGrps_item = -1;            /* PropertyGroup */
static int hf_h248_PropertyGroup_item = -1;       /* PropertyParm */
static int hf_h248_tSEventBufferControl = -1;     /* EventBufferControl */
static int hf_h248_serviceState = -1;             /* ServiceState */
static int hf_h248_muxType = -1;                  /* MuxType */
static int hf_h248_termList = -1;                 /* SEQUENCE_OF_TerminationID */
static int hf_h248_termList_item = -1;            /* TerminationID */
static int hf_h248_nonStandardData = -1;          /* NonStandardData */
static int hf_h248_eventList = -1;                /* SEQUENCE_OF_RequestedEvent */
static int hf_h248_eventList_item = -1;           /* RequestedEvent */
static int hf_h248_eventAction = -1;              /* RequestedActions */
static int hf_h248_evParList = -1;                /* SEQUENCE_OF_EventParameter */
static int hf_h248_evParList_item = -1;           /* EventParameter */
static int hf_h248_secondEvent = -1;              /* SecondEventsDescriptor */
static int hf_h248_notifyImmediate = -1;          /* NULL */
static int hf_h248_notifyRegulated = -1;          /* RegulatedEmbeddedDescriptor */
static int hf_h248_neverNotify = -1;              /* NULL */
static int hf_h248_keepActive = -1;               /* BOOLEAN */
static int hf_h248_eventDM = -1;                  /* EventDM */
static int hf_h248_notifyBehaviour = -1;          /* NotifyBehaviour */
static int hf_h248_resetEventsDescriptor = -1;    /* NULL */
static int hf_h248_digitMapValue = -1;            /* DigitMapValue */
static int hf_h248_secondaryEventList = -1;       /* SEQUENCE_OF_SecondRequestedEvent */
static int hf_h248_secondaryEventList_item = -1;  /* SecondRequestedEvent */
static int hf_h248_pkgdName = -1;                 /* PkgdName */
static int hf_h248_secondaryEventAction = -1;     /* SecondRequestedActions */
static int hf_h248_EventBufferDescriptor_item = -1;  /* EventSpec */
static int hf_h248_SignalsDescriptor_item = -1;   /* SignalRequest */
static int hf_h248_signal = -1;                   /* Signal */
static int hf_h248_seqSigList = -1;               /* SeqSigList */
static int hf_h248_signalList = -1;               /* SEQUENCE_OF_Signal */
static int hf_h248_signalList_item = -1;          /* Signal */
static int hf_h248_signalName = -1;               /* SignalName */
static int hf_h248_sigType = -1;                  /* SignalType */
static int hf_h248_duration = -1;                 /* INTEGER_0_65535 */
static int hf_h248_notifyCompletion = -1;         /* NotifyCompletion */
static int hf_h248_sigParList = -1;               /* SEQUENCE_OF_SigParameter */
static int hf_h248_sigParList_item = -1;          /* SigParameter */
static int hf_h248_direction = -1;                /* SignalDirection */
static int hf_h248_intersigDelay = -1;            /* INTEGER_0_65535 */
static int hf_h248_sigParameterName = -1;         /* SigParameterName */
static int hf_h248_value = -1;                    /* SigParamValues */
static int hf_h248_extraInfo2 = -1;               /* T_extraInfo2 */
static int hf_h248_SigParamValues_item = -1;      /* SigParamValue */
static int hf_h248_mtl = -1;                      /* SEQUENCE_OF_ModemType */
static int hf_h248_mtl_item = -1;                 /* ModemType */
static int hf_h248_mpl = -1;                      /* SEQUENCE_OF_PropertyParm */
static int hf_h248_mpl_item = -1;                 /* PropertyParm */
static int hf_h248_startTimer = -1;               /* INTEGER_0_99 */
static int hf_h248_shortTimer = -1;               /* INTEGER_0_99 */
static int hf_h248_longTimer = -1;                /* INTEGER_0_99 */
static int hf_h248_digitMapBody = -1;             /* IA5String */
static int hf_h248_durationTimer = -1;            /* INTEGER_0_99 */
static int hf_h248_serviceChangeMethod = -1;      /* ServiceChangeMethod */
static int hf_h248_serviceChangeAddress = -1;     /* ServiceChangeAddress */
static int hf_h248_serviceChangeVersion = -1;     /* INTEGER_0_99 */
static int hf_h248_serviceChangeProfile = -1;     /* ServiceChangeProfile */
static int hf_h248_serviceChangeReason = -1;      /* SCreasonValue */
static int hf_h248_serviceChangeDelay = -1;       /* INTEGER_0_4294967295 */
static int hf_h248_serviceChangeMgcId = -1;       /* MId */
static int hf_h248_timeStamp = -1;                /* TimeNotation */
static int hf_h248_serviceChangeInfo = -1;        /* AuditDescriptor */
static int hf_h248_serviceChangeIncompleteFlag = -1;  /* NULL */
static int hf_h248_SCreasonValue_item = -1;       /* SCreasonValueOctetStr */
static int hf_h248_timestamp = -1;                /* TimeNotation */
static int hf_h248_profileName = -1;              /* IA5String_SIZE_1_67 */
static int hf_h248_PackagesDescriptor_item = -1;  /* PackagesItem */
static int hf_h248_StatisticsDescriptor_item = -1;  /* StatisticsParameter */
static int hf_h248_statName = -1;                 /* StatName */
static int hf_h248_statValue = -1;                /* StatValue */
static int hf_h248_nonStandardIdentifier = -1;    /* NonStandardIdentifier */
static int hf_h248_data = -1;                     /* OCTET_STRING */
static int hf_h248_object = -1;                   /* OBJECT_IDENTIFIER */
static int hf_h248_h221NonStandard = -1;          /* H221NonStandard */
static int hf_h248_experimental = -1;             /* IA5String_SIZE_8 */
static int hf_h248_t35CountryCode1 = -1;          /* INTEGER_0_255 */
static int hf_h248_t35CountryCode2 = -1;          /* INTEGER_0_255 */
static int hf_h248_t35Extension = -1;             /* INTEGER_0_255 */
static int hf_h248_manufacturerCode = -1;         /* INTEGER_0_65535 */
static int hf_h248_date = -1;                     /* IA5String_SIZE_8 */
static int hf_h248_time = -1;                     /* IA5String_SIZE_8 */
static int hf_h248_Value_item = -1;               /* OCTET_STRING */
/* named bits */
static int hf_h248_T_auditToken_muxToken = -1;
static int hf_h248_T_auditToken_modemToken = -1;
static int hf_h248_T_auditToken_mediaToken = -1;
static int hf_h248_T_auditToken_eventsToken = -1;
static int hf_h248_T_auditToken_signalsToken = -1;
static int hf_h248_T_auditToken_digitMapToken = -1;
static int hf_h248_T_auditToken_statsToken = -1;
static int hf_h248_T_auditToken_observedEventsToken = -1;
static int hf_h248_T_auditToken_packagesToken = -1;
static int hf_h248_T_auditToken_eventBufferToken = -1;
static int hf_h248_NotifyCompletion_onTimeOut = -1;
static int hf_h248_NotifyCompletion_onInterruptByEvent = -1;
static int hf_h248_NotifyCompletion_onInterruptByNewSignalDescr = -1;
static int hf_h248_NotifyCompletion_otherReason = -1;
static int hf_h248_NotifyCompletion_onIteration = -1;

/*--- End of included file: packet-h248-hf.c ---*/
#line 67 "packet-h248-template.c"

/* Initialize the subtree pointers */
static gint ett_h248 = -1;
static gint ett_mtpaddress = -1;
static gint ett_packagename = -1;
static gint ett_codec = -1;
static gint ett_wildcard = -1;

static gint ett_h248_no_pkg = -1;
static gint ett_h248_no_sig = -1;
static gint ett_h248_no_evt = -1;

static int h248_tap = -1;

static gcp_hf_ett_t h248_arrel = {{-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1}};


/*--- Included file: packet-h248-ett.c ---*/
#line 1 "packet-h248-ett.c"
static gint ett_h248_MegacoMessage = -1;
static gint ett_h248_AuthenticationHeader = -1;
static gint ett_h248_Message = -1;
static gint ett_h248_T_messageBody = -1;
static gint ett_h248_SEQUENCE_OF_Transaction = -1;
static gint ett_h248_MId = -1;
static gint ett_h248_DomainName = -1;
static gint ett_h248_IP4Address = -1;
static gint ett_h248_IP6Address = -1;
static gint ett_h248_Transaction = -1;
static gint ett_h248_TransactionRequest = -1;
static gint ett_h248_SEQUENCE_OF_ActionRequest = -1;
static gint ett_h248_TransactionPending = -1;
static gint ett_h248_TransactionReply = -1;
static gint ett_h248_T_transactionResult = -1;
static gint ett_h248_SEQUENCE_OF_ActionReply = -1;
static gint ett_h248_SegmentReply = -1;
static gint ett_h248_TransactionResponseAck = -1;
static gint ett_h248_TransactionAck = -1;
static gint ett_h248_ErrorDescriptor = -1;
static gint ett_h248_ActionRequest = -1;
static gint ett_h248_SEQUENCE_OF_CommandRequest = -1;
static gint ett_h248_ActionReply = -1;
static gint ett_h248_SEQUENCE_OF_CommandReply = -1;
static gint ett_h248_ContextRequest = -1;
static gint ett_h248_T_topologyReq = -1;
static gint ett_h248_SEQUENCE_OF_PropertyParm = -1;
static gint ett_h248_SEQUENCE_OF_ContextIDinList = -1;
static gint ett_h248_ContextAttrAuditRequest = -1;
static gint ett_h248_SEQUENCE_OF_IndAudPropertyParm = -1;
static gint ett_h248_SelectLogic = -1;
static gint ett_h248_CommandRequest = -1;
static gint ett_h248_Command = -1;
static gint ett_h248_CommandReply = -1;
static gint ett_h248_TopologyRequest = -1;
static gint ett_h248_AmmRequest = -1;
static gint ett_h248_SEQUENCE_OF_AmmDescriptor = -1;
static gint ett_h248_AmmDescriptor = -1;
static gint ett_h248_AmmsReply = -1;
static gint ett_h248_SubtractRequest = -1;
static gint ett_h248_AuditRequest = -1;
static gint ett_h248_AuditReply = -1;
static gint ett_h248_AuditResult = -1;
static gint ett_h248_TermListAuditResult = -1;
static gint ett_h248_TerminationAudit = -1;
static gint ett_h248_AuditReturnParameter = -1;
static gint ett_h248_AuditDescriptor = -1;
static gint ett_h248_T_auditToken = -1;
static gint ett_h248_SEQUENCE_OF_IndAuditParameter = -1;
static gint ett_h248_IndAuditParameter = -1;
static gint ett_h248_IndAudMediaDescriptor = -1;
static gint ett_h248_indAudMediaDescriptorStreams = -1;
static gint ett_h248_SEQUENCE_OF_IndAudStreamDescriptor = -1;
static gint ett_h248_IndAudStreamDescriptor = -1;
static gint ett_h248_IndAudStreamParms = -1;
static gint ett_h248_IndAudLocalControlDescriptor = -1;
static gint ett_h248_IndAudPropertyParm = -1;
static gint ett_h248_IndAudLocalRemoteDescriptor = -1;
static gint ett_h248_IndAudPropertyGroup = -1;
static gint ett_h248_IndAudTerminationStateDescriptor = -1;
static gint ett_h248_IndAudEventsDescriptor = -1;
static gint ett_h248_IndAudEventBufferDescriptor = -1;
static gint ett_h248_IndAudSignalsDescriptor = -1;
static gint ett_h248_IndAudSeqSigList = -1;
static gint ett_h248_IndAudSignal = -1;
static gint ett_h248_IndAudDigitMapDescriptor = -1;
static gint ett_h248_IndAudStatisticsDescriptor = -1;
static gint ett_h248_IndAudPackagesDescriptor = -1;
static gint ett_h248_NotifyRequest = -1;
static gint ett_h248_NotifyReply = -1;
static gint ett_h248_ObservedEventsDescriptor = -1;
static gint ett_h248_SEQUENCE_OF_ObservedEvent = -1;
static gint ett_h248_ObservedEvent = -1;
static gint ett_h248_SEQUENCE_OF_EventParameter = -1;
static gint ett_h248_EventParameter = -1;
static gint ett_h248_T_extraInfo = -1;
static gint ett_h248_EventParamValues = -1;
static gint ett_h248_ServiceChangeRequest = -1;
static gint ett_h248_ServiceChangeReply = -1;
static gint ett_h248_ServiceChangeResult = -1;
static gint ett_h248_TerminationID = -1;
static gint ett_h248_SEQUENCE_OF_WildcardField = -1;
static gint ett_h248_TerminationIDList = -1;
static gint ett_h248_MediaDescriptor = -1;
static gint ett_h248_T_streams = -1;
static gint ett_h248_SEQUENCE_OF_StreamDescriptor = -1;
static gint ett_h248_StreamDescriptor = -1;
static gint ett_h248_StreamParms = -1;
static gint ett_h248_LocalControlDescriptor = -1;
static gint ett_h248_PropertyParm = -1;
static gint ett_h248_SEQUENCE_OF_PropertyID = -1;
static gint ett_h248_T_extraInfo1 = -1;
static gint ett_h248_LocalRemoteDescriptor = -1;
static gint ett_h248_SEQUENCE_OF_PropertyGroup = -1;
static gint ett_h248_PropertyGroup = -1;
static gint ett_h248_TerminationStateDescriptor = -1;
static gint ett_h248_MuxDescriptor = -1;
static gint ett_h248_SEQUENCE_OF_TerminationID = -1;
static gint ett_h248_EventsDescriptor = -1;
static gint ett_h248_SEQUENCE_OF_RequestedEvent = -1;
static gint ett_h248_RequestedEvent = -1;
static gint ett_h248_RegulatedEmbeddedDescriptor = -1;
static gint ett_h248_NotifyBehaviour = -1;
static gint ett_h248_RequestedActions = -1;
static gint ett_h248_EventDM = -1;
static gint ett_h248_SecondEventsDescriptor = -1;
static gint ett_h248_SEQUENCE_OF_SecondRequestedEvent = -1;
static gint ett_h248_SecondRequestedEvent = -1;
static gint ett_h248_SecondRequestedActions = -1;
static gint ett_h248_EventBufferDescriptor = -1;
static gint ett_h248_EventSpec = -1;
static gint ett_h248_SignalsDescriptor = -1;
static gint ett_h248_SignalRequest = -1;
static gint ett_h248_SeqSigList = -1;
static gint ett_h248_SEQUENCE_OF_Signal = -1;
static gint ett_h248_Signal = -1;
static gint ett_h248_SEQUENCE_OF_SigParameter = -1;
static gint ett_h248_NotifyCompletion = -1;
static gint ett_h248_SigParameter = -1;
static gint ett_h248_T_extraInfo2 = -1;
static gint ett_h248_SigParamValues = -1;
static gint ett_h248_ModemDescriptor = -1;
static gint ett_h248_SEQUENCE_OF_ModemType = -1;
static gint ett_h248_DigitMapDescriptor = -1;
static gint ett_h248_DigitMapValue = -1;
static gint ett_h248_ServiceChangeParm = -1;
static gint ett_h248_SCreasonValue = -1;
static gint ett_h248_ServiceChangeAddress = -1;
static gint ett_h248_ServiceChangeResParm = -1;
static gint ett_h248_ServiceChangeProfile = -1;
static gint ett_h248_PackagesDescriptor = -1;
static gint ett_h248_PackagesItem = -1;
static gint ett_h248_StatisticsDescriptor = -1;
static gint ett_h248_StatisticsParameter = -1;
static gint ett_h248_NonStandardData = -1;
static gint ett_h248_NonStandardIdentifier = -1;
static gint ett_h248_H221NonStandard = -1;
static gint ett_h248_TimeNotation = -1;
static gint ett_h248_Value = -1;

/*--- End of included file: packet-h248-ett.c ---*/
#line 84 "packet-h248-template.c"

static dissector_handle_t h248_term_handle;

static emem_tree_t* msgs = NULL;
static emem_tree_t* trxs = NULL;
static emem_tree_t* ctxs_by_trx = NULL;
static emem_tree_t* ctxs = NULL;

static gboolean h248_prefs_initialized = FALSE;
static gboolean keep_persistent_data = FALSE;
static guint32 udp_port = 2945;
static guint32 temp_udp_port = 2945;
static guint32 tcp_port = 2945;
static guint32 temp_tcp_port = 2945;
static gboolean h248_desegment = TRUE;



static proto_tree *h248_tree;
static tvbuff_t* h248_tvb;

static dissector_handle_t h248_handle;
static dissector_handle_t h248_term_handle;
static dissector_handle_t h248_tpkt_handle;

/* Forward declarations */
static int dissect_h248_ServiceChangeReasonStr(gboolean implicit_tag, tvbuff_t *tvb, int offset, asn1_ctx_t *actx, proto_tree *tree, int hf_index);

static const value_string package_name_vals[] = {
  {   0x0000, "Media stream properties H.248.1 Annex C" },
  {   0x0001, "Generic H.248.1 Annex E" },
  {   0x0002, "root H.248.1 Annex E" },
  {   0x0003, "tonegen H.248.1 Annex E" },
  {   0x0004, "tonedet H.248.1 Annex E" },
  {   0x0005, "dg H.248.1 Annex E" },
  {   0x0006, "dd H.248.1 Annex E" },
  {   0x0007, "cg H.248.1 Annex E" },
  {   0x0008, "cd H.248.1 Annex E" },
  {   0x0009, "al H.248.1 Annex E" },
  {   0x000a, "ct H.248.1 Annex E" },
  {   0x000b, "nt H.248.1 Annex E" },
  {   0x000c, "rtp H.248.1 Annex E" },
  {   0x000d, "tdmc H.248.1 Annex E" },
  {   0x000e, "ftmd H.248.1 Annex E" },
  {   0x000f, "txc H.248.2" },											/* H.248.2 */
  {   0x0010, "txp H.248.2" },
  {   0x0011, "ctyp H.248.2" },
  {   0x0012, "fax H.248.2" },
  {   0x0013, "ipfax H.248.2" },
  {   0x0014, "dis H.248.3" },											/* H.248.3 */
  {   0x0015, "key H.248.3" },
  {   0x0016, "kp H.248.3" },
  {   0x0017, "labelkey H.248.3" },
  {   0x0018, "kf H.248.3" },
  {   0x0019, "ind H.248.3" },
  {   0x001a, "ks H.248.3" },
  {   0x001b, "anci H.248.3" },
  {   0x001c, "dtd H.248.6" },											/* H.248.6 */
  {   0x001d, "an H.248.7" },											/* H.248.7 */
  {   0x001e, "Bearer Characteristics Q.1950 Annex A" }, 				/* Q.1950 Annex A */
  {   0x001f, "Bearer Network Connection Cut Q.1950 Annex A" },
  {   0x0020, "Reuse Idle Q.1950 Annex A" },
  {   0x0021, "Generic Bearer Connection Q.1950 Annex A" },
  {   0x0022, "Bearer Control Tunnelling Q.1950 Annex A" },
  {   0x0023, "Basic Call Progress Tones Q.1950 Annex A" },
  {   0x0024, "Expanded Call Progress Tones Q.1950 Annex A" },
  {   0x0025, "Basic Services Tones Q.1950 Annex A" },
  {   0x0026, "Expanded Services Tones Q.1950 Annex A" },
  {   0x0027, "Intrusion Tones Q.1950 Annex A" },
  {   0x0028, "Business Tones Q.1950 Annex A" },
  {   0x0029, "Media Gateway Resource Congestion Handling H.248.10" },	/* H.248.10 */
  {   0x002a, "H245 package H248.12" },									/* H.248.12 */
  {   0x002b, "H323 bearer control package H.248.12" },					/* H.248.12 */
  {   0x002c, "H324 package H.248.12" },								/* H.248.12 */
  {   0x002d, "H245 command package H.248.12" },						/* H.248.12 */
  {   0x002e, "H245 indication package H.248.12" },						/* H.248.12 */
  {   0x002f, "3G User Plane" },										/* 3GPP TS 29.232 v4.1.0 */
  {   0x0030, "3G Circuit Switched Data" },
  {   0x0031, "3G TFO Control" },
  {   0x0032, "3G Expanded Call Progress Tones" },
  {   0x0033, "Advanced Audio Server (AAS Base)" },						/* H.248.9 */
  {   0x0034, "AAS Digit Collection" }, 								/* H.248.9 */
  {   0x0035, "AAS Recording" }, 										/* H.248.9 */
  {   0x0036, "AAS Segment Management" },								/* H.248.9 */
  {   0x0037, "Quality Alert Ceasing" },								/* H.248.13 */
  {   0x0038, "Conferencing Tones Generation" },						/* H.248.27 */
  {   0x0039, "Diagnostic Tones Generation" },							/* H.248.27 */
  {   0x003a, "Carrier Tones Generation Package H.248.23" },			/* H.248.27 */
  {   0x003b, "Enhanced Alerting Package H.248.23" },					/* H.248.23 */
  {   0x003c, "Analog Display Signalling Package H.248.23" },			/* H.248.23 */
  {   0x003d, "Multi-Frequency Tone Generation Package H.248.24" },		/* H.248.24 */
  {   0x003e, "H.248.23Multi-Frequency Tone Detection Package H.248.24" }, /* H.248.24 */
  {   0x003f, "Basic CAS Package H.248.25" },							/* H.248.25 */
  {   0x0040, "Robbed Bit Signalling Package H.248.25" },		        /* H.248.25 */
  {   0x0041, "Operator Services and Emgergency Services Package H.248.25" },
  {   0x0042, "Operator Services Extension Package H.248.25" },
  {   0x0043, "Extended Analog Line Supervision Package H.248.26" },
  {   0x0044, "Automatic Metering Package H.248.26" },
  {   0x0045, "Inactivity Timer Package H.248.14" },
  {   0x0046, "3G Modification of Link Characteristics Bearer Capability" }, /* 3GPP TS 29.232 v4.4.0 */
  {   0x0047, "Base Announcement Syntax H.248.9" },
  {   0x0048, "Voice Variable Syntax H.248.9" },
  {   0x0049, "Announcement Set Syntax H.248.9" },
  {   0x004a, "Phrase Variable Syntax H.248.9" },
  {   0x004b, "Basic NAS package" },
  {   0x004c, "NAS incoming package" },
  {   0x004d, "NAS outgoing package" },
  {   0x004e, "NAS control package" },
  {   0x004f, "NAS root package" },
  {   0x0050, "Profile Handling Package H.248.18" },
  {   0x0051, "Media Gateway Overload Control Package H.248.11" },
  {   0x0052, "Extended DTMF Detection Package H.248.16" },
  {   0x0053, "Quiet Termination Line Test" },
  {   0x0054, "Loopback Line Test Response" }, 							/* H.248.17 */
  {   0x0055, "ITU 404Hz Line Test" },									/* H.248.17 */
  {   0x0056, "ITU 816Hz Line Test" },									/* H.248.17 */
  {   0x0057, "ITU 1020Hz Line Test" },									/* H.248.17 */
  {   0x0058, "ITU 2100Hz Disable Tone Line Test" },					/* H.248.17 */
  {   0x0059, "ITU 2100Hz Disable Echo Canceller Tone Line Test" },		/* H.248.17 */
  {   0x005a, "ITU 2804Hz Tone Line Test" },							/* H.248.17 */
  {   0x005b, "ITU Noise Test Tone Line Test" },						/* H.248.17 */
  {   0x005c, "ITU Digital Pseudo Random Test Line Test" },				/* H.248.17 */
  {   0x005d, "ITU ATME No.2 Test Line Response" },						/* H.248.17 */
  {   0x005e, "ANSI 1004Hz Test Tone Line Test" },						/* H.248.17 */
  {   0x005f, "ANSI Test Responder Line Test" },						/* H.248.17 */
  {   0x0060, "ANSI 2225Hz Test Progress Tone Line Test" },				/* H.248.17 */
  {   0x0061, "ANSI Digital Test Signal Line Test" },					/* H.248.17 */
  {   0x0062, "ANSI Inverting Loopback Line Test Repsonse" },			/* H.248.17 */
  {   0x0063, "Extended H.324 Packages H.248.12 Annex A" },
  {   0x0064, "Extended H.245 Command Package H.248.12 Annex A" },
  {   0x0065, "Extended H.245 Indication Package H.248.12 Annex A" },
  {   0x0066, "Enhanced DTMF Detection Package H.248.16" },
  {   0x0067, "Connection Group Identity Package Q.1950 Annex E" },
  {   0x0068, "CTM Text Transport 3GPP TS 29.232 v5.2.0" },
  {   0x0069, "SPNE Control Package Q.115.0" },
  {   0x006a, "Semi-permanent Connection Package H.248.21" },
  {   0x006b, "Shared Risk Group Package H.248.22" },
  {   0x006c, "isuptn Annex B of ITU-T Rec. J.171" },
  {   0x006d, "Basic CAS Addressing Package H.248.25" },
  {   0x006e, "Floor Control Package H.248.19" },
  {   0x006f, "Indication of Being Viewed Package H.248.19" },
  {   0x0070, "Volume Control Package H.248.19" },
  {   0x0071, "UNASSIGNED" },
  {   0x0072, "Volume Detection Package H.248.19" },
  {   0x0073, "Volume Level Mixing Package H.248.19" },
  {   0x0074, "Mixing Volume Level Control Package H.248.19" },
  {   0x0075, "Voice Activated Video Switch Package H.248.19" },
  {   0x0076, "Lecture Video Mode Package H.248.19" },
  {   0x0077, "Contributing Video Source Package H.248.19" },
  {   0x0078, "Video Window Package H.248.19" },
  {   0x0079, "Tiled Window Package H.248.19" },
  {   0x007a, "Adaptive Jitter Buffer Package H.248.31" },
  {   0x007b, "International CAS Package H.248.28" },
  {   0x007c, "CAS Blocking Package H.248.28" },
  {   0x007d, "International CAS Compelled Package H.248.29" },
  {   0x007e, "International CAS Compelled with Overlap Package H.248.29" },
  {   0x007f, "International CAS Compelled with End-to-end Package H.248.29" },
  {   0x0080, "RTCP XR Package H.248.30" },
  {   0x0081, "RTCP XR Burst Metrics Package H.248.30" },
  {   0x0082, "threegcsden 3G Circuit Switched Data" },				/* 3GPP TS 29.232 v5.6.0 */
  {   0x0083, "threegiptra 3G Circuit Switched Data" },				/* 3GPP TS 29.232 v5.6.0 */
  {   0x0084, "threegflex 3G Circuit Switched Data" },				/* 3GPP TS 29.232 v5.6.0 */
  {   0x0085, "H.248 PCMSB" },
  {   0x008a, "TIPHON Extended H.248/MEGACO Package" },				/* ETSI specification TS 101 3 */
  {   0x008b, "Differentiated Services Package" },					/* Annex A of ETSI TS 102 333 */
  {   0x008c, "Gate Management Package" },							/* Annex B of ETSI TS 102 333 */
  {   0x008d, "Traffic Management Package" },						/* Annex C of ETSI TS 102 333 */
  {   0x008e, "Gate Recovery Information Package" },				/* Annex D of ETSI TS 102 333 */
  {   0x008f, "NAT Traversal Package" },							/* Annex E of ETSI TS 102 333 */
  {   0x0090, "MPLS Package" },										/* Annex F of ETSI TS 102 333 */
  {   0x0091, "VLAN Package" },										/* Annex G of ETSI TS 102 333 */
  {   0x8000, "Ericsson IU" },
  {   0x8001, "Ericsson UMTS and GSM Circuit" },
  {   0x8002, "Ericsson Tone Generator Package" },
  {   0x8003, "Ericsson Line Test Package" },
  {   0x8004, "Nokia Advanced TFO Package" },
  {   0x8005, "Nokia IWF Package" },
  {   0x8006, "Nokia Root Package" },
  {   0x8007, "Nokia Trace Package" },
  {   0x8008, "Ericsson  V5.2 Layer" },
  {   0x8009, "Ericsson Detailed Termination Information Package" },
  {   0x800a, "Nokia Bearer Characteristics Package" },
	{0,     NULL}
};
/*
 * This table consist of PackageName + EventName and its's corresponding string
 *
 */
static const value_string event_name_vals[] = {
  {   0x00000000, "Media stream properties H.248.1 Annex C" },
  {   0x00010000, "g H.248.1 Annex E" },
  {   0x00010001, "g/Cause" },
  {   0x00010002, "g/Signal Completion" },
  {   0x00040000, "tonedet H.248.1 Annex E" },
  {   0x00040001, "tonedet/std(Start tone detected)" },
  {   0x00040002, "tonedet/etd(End tone detected)" },
  {   0x00040003, "tonedet/ltd(Long tone detected)" },
  {   0x00060000, "dd H.248.1 Annex E" },
  {   0x00060001, "dd/std" },
  {   0x00060002, "dd/etd" },
  {   0x00060003, "dd/ltd" },
  {   0x00060004, "dd, DigitMap Completion Event" },
  {   0x00060010, "dd/d0, DTMF character 0" },
  {   0x00060011, "dd/d1, DTMF character 1" },
  {   0x00060012, "dd/d2, DTMF character 2" },
  {   0x00060013, "dd/d3, DTMF character 3" },
  {   0x00060014, "dd/d4, DTMF character 4" },
  {   0x00060015, "dd/d5, DTMF character 5" },
  {   0x00060016, "dd/d6, DTMF character 6" },
  {   0x00060017, "dd/d7, DTMF character 7" },
  {   0x00060018, "dd/d8, DTMF character 8" },
  {   0x00060019, "dd/d9, DTMF character 9" },
  {   0x0006001a, "dd/a, DTMF character A" },
  {   0x0006001b, "dd/b, DTMF character B" },
  {   0x0006001c, "dd/c, DTMF character C" },
  {   0x0006001d, "dd/d, DTMF character D" },
  {   0x00060020, "dd/*, DTMF character *" },
  {   0x00060021, "dd/#, DTMF character #" },
  {   0x00080030, "cd, Dial Tone" },
  {   0x00080031, "cd, Ringing Tone" },
  {   0x00080032, "cd, Busy Tone" },
  {   0x00080033, "cd, Congestion Tone" },
  {   0x00080034, "cd, Special Information Tone" },
  {   0x00080035, "cd, (Recording) Warning Tone" },
  {   0x00080036, "cd, Payphone Recognition Tone" },
  {   0x00080037, "cd, Call Waiting Tone" },
  {   0x00080038, "cd, Caller Waiting Tone" },
  {   0x00090004, "al, onhook" },
  {   0x00090005, "al, offhook" },
  {   0x00090006, "al, flashhook" },
  {   0x0009ffff, "al, *" },
  {   0x000a0005, "ct, Completion of Continuity test" },
  {   0x000b0005, "nt, network failure" },
  {   0x000b0006, "nt, quality alert" },
  {   0x000c0001, "rtp, Payload Transition" },
  {   0x00210000, "Generic Bearer Connection Q.1950 Annex A" },
  {   0x00210001, "GB/BNCChange" },
  {   0x00220001, "BT/TIND (Tunnel Indication)" },
  {   0x002a0001, "H.245/h245msg (Incoming H.245 Message)" },
  {   0x002a0004, "H.245/h245ChC (H.245 Channel Closed)" },
  {   0x00450000, "Inactivity Timer H.248.14" },
  {   0x00450001, "it/ito" },
  {   0x00450002, "it/ito" },
  {   0x00460001, "threegmlc/mod_link_supp (Bearer Modification Support Event)" },
  {   0x800a0000, "Nokia Bearer Characteristics Package" },
	{0,     NULL}
};

/*
 * This table consist of PackageName + SignalName and its's corresponding string
 */
static const value_string signal_name_vals[] = {
  {   0x00000000, "Media stream properties H.248.1 Annex C" },
  {   0x00010000, "g H.248.1 Annex E" },
  {   0x00030001, "tonegen/pt(Play tone)" },
  {   0x00050010, "dg, DTMF character 0" },
  {   0x00050011, "dg, DTMF character 1" },
  {   0x00050012, "dg, DTMF character 2" },
  {   0x00050013, "dg, DTMF character 3" },
  {   0x00050014, "dg, DTMF character 4" },
  {   0x00050015, "dg, DTMF character 5" },
  {   0x00050016, "dg, DTMF character 6" },
  {   0x00050017, "dg, DTMF character 7" },
  {   0x00050018, "dg, DTMF character 8" },
  {   0x00050019, "dg, DTMF character 9" },
  {   0x0005001a, "dg, DTMF character A" },
  {   0x0005001b, "dg, DTMF character B" },
  {   0x0005001c, "dg, DTMF character C" },
  {   0x0005001d, "dg, DTMF character D" },
  {   0x00050020, "dg, DTMF character *" },
  {   0x00050021, "dg, DTMF character #" },
  {   0x00070030, "cg, Dial Tone" },
  {   0x00070031, "cg/rt (Ringing Tone)" },
  {   0x00070032, "cg, Busy Tone" },
  {   0x00070033, "cg, Congestion Tone" },
  {   0x00070034, "cg, Special Information Tone" },
  {   0x00070035, "cg, (Recording) Warning Tone" },
  {   0x00070036, "cg, Payphone Recognition Tone" },
  {   0x00070037, "cg, Call Waiting Tone" },
  {   0x00070038, "cg, Caller Waiting Tone" },
  {   0x00090002, "al, ring" },
  {   0x0009ffff, "al, *" },
  {   0x000a0003, "ct, Continuity test" },
  {   0x000a0004, "ct, Continuity respond" },
  {   0x00210000, "GB Generic Bearer Connection Q.1950 Annex A" },
  {   0x00210001, "GB/EstBNC(Establish BNC)" },
  {   0x00210002, "GB/ModBNC (Modify BNC)" },
  {   0x00210003, "GB/RelBNC(Release BNC)" },

  {   0x002a0001, "H.245/cs (channel state)" },
  {   0x002a0002, "H.245/termtype (Terminal Type)" },

  {   0x002c0001, "H.324/cmod (Communication mode)" },
  {   0x002c0002, "H.324/muxlv (Highest Multiplexing level)" },
  {   0x002c0003, "H.324/demux (Demultiplex)" },
  {   0x002c0004, "H.324/h223capr (Remote H.223 capability)" },
  {   0x002c0005, "H.324/muxtbl_in (Incoming Multiplex Table)" },
  {   0x002c0006, "H.324/muxtbl_out (Outgoing Multiplex Table)" },

  {   0x800a0000, "Nokia Bearer Characteristics Package" },
  {0,     NULL}
};


#if 0
static const value_string context_id_type[] = {
	{NULL_CONTEXT,"0 (Null Context)"},
	{CHOOSE_CONTEXT,"$ (Choose Context)"},
	{ALL_CONTEXTS,"* (All Contexts)"},
	{0,NULL}
};
#endif

static const value_string h248_reasons[] = {
    { 400, "Syntax error in message"},
    { 401, "Protocol Error"},
    { 402, "Unauthorized"},
    { 403, "Syntax error in transaction request"},
    { 406, "Version Not Supported"},
    { 410, "Incorrect identifier"},
    { 411, "The transaction refers to an unknown ContextId"},
    { 412, "No ContextIDs available"},
    { 421, "Unknown action or illegal combination of actions"},
    { 422, "Syntax Error in Action"},
    { 430, "Unknown TerminationID"},
    { 431, "No TerminationID matched a wildcard"},
    { 432, "Out of TerminationIDs or No TerminationID available"},
    { 433, "TerminationID is already in a Context"},
    { 434, "Max number of Terminations in a Context exceeded"},
    { 435, "Termination ID is not in specified Context"},
    { 440, "Unsupported or unknown Package"},
    { 441, "Missing Remote or Local Descriptor"},
    { 442, "Syntax Error in Command"},
    { 443, "Unsupported or Unknown Command"},
    { 444, "Unsupported or Unknown Descriptor"},
    { 445, "Unsupported or Unknown Property"},
    { 446, "Unsupported or Unknown Parameter"},
    { 447, "Descriptor not legal in this command"},
    { 448, "Descriptor appears twice in a command"},
    { 449, "Unsupported or Unknown Parameter or Property Value"},
    { 450, "No such property in this package"},
    { 451, "No such event in this package"},
    { 452, "No such signal in this package"},
    { 453, "No such statistic in this package"},
    { 454, "No such parameter value in this package"},
    { 455, "Property illegal in this Descriptor"},
    { 456, "Property appears twice in this Descriptor"},
    { 457, "Missing parameter in signal or event"},
    { 458, "Unexpected Event/Request ID"},
    { 459, "Unsupported or Unknown Profile"},
    { 460, "Unable to set statistic on stream"},
    { 471, "Implied Add for Multiplex failure"},
    { 500, "Internal software Failure in MG"},
    { 501, "Not Implemented"},
    { 502, "Not ready"},
    { 503, "Service Unavailable"},
    { 504, "Command Received from unauthorized entity"},
    { 505, "Transaction Request Received before a Service Change Reply has been received"},
    { 506, "Number of Transaction Pendings Exceeded"},
    { 510, "Insufficient resources"},
    { 512, "Media Gateway unequipped to detect requested Event"},
    { 513, "Media Gateway unequipped to generate requested Signals"},
    { 514, "Media Gateway cannot send the specified announcement"},
    { 515, "Unsupported Media Type"},
    { 517, "Unsupported or invalid mode"},
    { 518, "Event buffer full"},
    { 519, "Out of space to store digit map"},
    { 520, "Digit Map undefined in the MG"},
    { 521, "Termination is ServiceChangeing"},
    { 522, "Functionality Requested in Topology Triple Not Supported"},
    { 526, "Insufficient bandwidth"},
    { 529, "Internal hardware failure in MG"},
    { 530, "Temporary Network failure"},
    { 531, "Permanent Network failure"},
    { 532, "Audited Property, Statistic, Event or Signal does not exist"},
    { 533, "Response exceeds maximum transport PDU size"},
    { 534, "Illegal write or read only property"},
    { 540, "Unexpected initial hook state"},
    { 542, "Command is not allowed on this termination"},
    { 581, "Does Not Exist"},
    { 600, "Illegal syntax within an announcement specification"},
    { 601, "Variable type not supported"},
    { 602, "Variable value out of range"},
    { 603, "Category not supported"},
    { 604, "Selector type not supported"},
    { 605, "Selector value not supported"},
    { 606, "Unknown segment ID"},
    { 607, "Mismatch between play specification and provisioned data"},
    { 608, "Provisioning error"},
    { 609, "Invalid offset"},
    { 610, "No free segment IDs"},
    { 611, "Temporary segment not found"},
    { 612, "Segment in use"},
    { 613, "ISP port limit overrun"},
    { 614, "No modems available"},
    { 615, "Calling number unacceptable"},
    { 616, "Called number unacceptable"},
    { 900, "Service Restored"},
    { 901, "Cold Boot"},
    { 902, "Warm Boot"},
    { 903, "MGC Directed Change"},
    { 904, "Termination malfunctioning"},
    { 905, "Termination taken out of service"},
    { 906, "Loss of lower layer connectivity (e.g. downstream sync)"},
    { 907, "Transmission Failure"},
    { 908, "MG Impending Failure"},
    { 909, "MGC Impending Failure"},
    { 910, "Media Capability Failure"},
    { 911, "Modem Capability Failure"},
    { 912, "Mux Capability Failure"},
    { 913, "Signal Capability Failure"},
    { 914, "Event Capability Failure"},
    { 915, "State Loss"},
    { 916, "Packages Change"},
    { 917, "Capabilities Change"},
    { 918, "Cancel Graceful"},
    { 919, "Warm Failover"},
    { 920, "Cold Failover"},
	{0,NULL}
};

static const value_string wildcard_modes[] = {
    { 0, "Choose" },
    { 1, "All" },
    { 0, NULL }
};

static const value_string wildcard_levels[] = {
    { 0, "This One Level" },
    { 1, "This Level and those below" },
    { 0, NULL }
};

static h248_curr_info_t curr_info = {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
static guint32 error_code;
static gcp_wildcard_t wild_term;


extern void h248_param_ber_integer(proto_tree* tree, tvbuff_t* tvb, packet_info* pinfo, int hfid, h248_curr_info_t* u _U_, void* implicit) {
	dissect_ber_integer(implicit ? *((gboolean*)implicit) : FALSE, pinfo, tree, tvb, 0, hfid, NULL);
}

extern void h248_param_ber_octetstring(proto_tree* tree, tvbuff_t* tvb, packet_info* pinfo, int hfid, h248_curr_info_t* u _U_, void* implicit) {
	dissect_ber_octet_string(implicit ? *((gboolean*)implicit) : FALSE, pinfo, tree, tvb, 0, hfid, NULL);
}

extern void h248_param_ber_boolean(proto_tree* tree, tvbuff_t* tvb, packet_info* pinfo, int hfid, h248_curr_info_t* u _U_, void* implicit) {
	dissect_ber_boolean(implicit ? *((gboolean*)implicit) : FALSE, pinfo, tree, tvb, 0, hfid);
}

extern void h248_param_item(proto_tree* tree,
							 tvbuff_t* tvb,
							 packet_info* pinfo _U_,
							 int hfid,
							 h248_curr_info_t* h248_info _U_,
							 void* lenp ) {
	int len = lenp ? *((int*)lenp) : -1;
	proto_tree_add_item(tree,hfid,tvb,0,len,FALSE);
}

extern void h248_param_external_dissector(proto_tree* tree, tvbuff_t* tvb, packet_info* pinfo , int hfid _U_, h248_curr_info_t* u _U_, void* dissector_hdl) {
	call_dissector((dissector_handle_t) dissector_hdl,tvb,pinfo,tree);
}


static const h248_package_t no_package = { 0xffff, &hf_h248_no_pkg, &ett_h248_no_pkg, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
static const h248_pkg_sig_t no_signal = { 0, &hf_h248_no_sig, &ett_h248_no_sig, NULL, NULL };
static const h248_pkg_param_t no_param = { 0, &hf_h248_param, h248_param_item,  NULL };
static const h248_pkg_evt_t no_event = { 0, &hf_h248_no_evt, &ett_h248_no_evt, NULL, NULL };

static GPtrArray* packages = NULL;

extern void h248_param_PkgdName(proto_tree* tree, tvbuff_t* tvb, packet_info* pinfo , int hfid _U_, h248_curr_info_t* u1 _U_, void* u2 _U_) {
  tvbuff_t *new_tvb = NULL;
  proto_tree *package_tree=NULL;
  guint16 name_major, name_minor;
  int old_offset;
  const h248_package_t* pkg = NULL;
  guint i;
  int offset = 0;
  
	old_offset=offset;
	offset = dissect_ber_octet_string(FALSE, pinfo, tree, tvb, offset, hfid , &new_tvb);
  	
  if (new_tvb) {
    /* this field is always 4 bytes  so just read it into two integers */
    name_major=tvb_get_ntohs(new_tvb, 0);
    name_minor=tvb_get_ntohs(new_tvb, 2);

    /* do the prettification */
    proto_item_append_text(ber_last_created_item, "  %s (%04x)", val_to_str(name_major, package_name_vals, "Unknown Package"), name_major);

    if(tree){
	proto_item* pi;
	const gchar* strval;
	    
	package_tree = proto_item_add_subtree(ber_last_created_item, ett_packagename);
	proto_tree_add_uint(package_tree, hf_h248_pkg_name, tvb, offset-4, 2, name_major);

	for(i=0; i < packages->len; i++) {
		pkg = g_ptr_array_index(packages,i);

		if (name_major == pkg->id) {
			break;
		} else {
			pkg = NULL;
		}
	}

	if (! pkg ) pkg = &no_package;


	pi = proto_tree_add_uint(package_tree, hf_248_pkg_param, tvb, offset-2, 2, name_minor);
	
	if (pkg->signal_names && ( strval = match_strval(name_minor, pkg->signal_names) )) {
		strval = ep_strdup_printf("%s (%d)",strval,name_minor);
	} else {
		strval = ep_strdup_printf("Unknown (%d)",name_minor);
	}

	proto_item_set_text(pi,"Signal ID: %s", strval);
	}
	
  }
}


static int dissect_h248_trx_id(gboolean implicit_tag, packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset, guint32* trx_id_p) {
	guint64 trx_id = 0;
  	gint8 class;
	gboolean pc;
	gint32 tag;
	guint32 len;
	guint32 i;

	if(!implicit_tag){
		offset=dissect_ber_identifier(pinfo, tree, tvb, offset, &class, &pc, &tag);
		offset=dissect_ber_length(pinfo, tree, tvb, offset, &len, NULL);
	} else {
		len=tvb_length_remaining(tvb, offset);
	}


	if (len > 8 || len < 1) {
		THROW(BoundsError);
	} else {
		for(i=1;i<=len;i++){
			trx_id=(trx_id<<8)|tvb_get_guint8(tvb, offset);
			offset++;
		}
		if (trx_id > 0xffffffff) {
			proto_item* pi = proto_tree_add_text(tree, tvb, offset-len, len,"transactionId %" PRIu64, trx_id);
            proto_item_set_expert_flags(pi, PI_MALFORMED, PI_WARN);

            *trx_id_p = 0;

		} else {
			proto_tree_add_uint(tree, hf_h248_transactionId, tvb, offset-len, len, (guint32)trx_id);
            *trx_id_p = (guint32)trx_id;
		}
	}

    return offset;
}

static int dissect_h248_ctx_id(gboolean implicit_tag, packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset, guint32* ctx_id_p) {
	gint8 class;
	gboolean pc;
	gint32 tag;
	guint32 len;
	guint64 ctx_id = 0;
	guint32 i;

	if(!implicit_tag){
		offset=dissect_ber_identifier(pinfo, tree, tvb, offset, &class, &pc, &tag);
		offset=dissect_ber_length(pinfo, tree, tvb, offset, &len, NULL);
	} else {
		len=tvb_length_remaining(tvb, offset);
	}


	if (len > 8 || len < 1) {
		THROW(BoundsError);
	} else {
		for(i=1;i<=len;i++){
			ctx_id=(ctx_id<<8)|tvb_get_guint8(tvb, offset);
			offset++;
		}

		if (ctx_id > 0xffffffff) {
			proto_item* pi = proto_tree_add_text(tree, tvb, offset-len, len,
                                                 "contextId: %" PRIu64, ctx_id);
            proto_item_set_expert_flags(pi, PI_MALFORMED, PI_WARN);

            *ctx_id_p = 0xfffffffd;

		} else {
			proto_item* pi = proto_tree_add_uint(tree, hf_h248_context_id, tvb, offset-len, len, (guint32)ctx_id);

            if ( ctx_id ==  NULL_CONTEXT ) {
                proto_item_set_text(pi,"contextId: Null Context(0)");
            } else if ( ctx_id ==  CHOOSE_CONTEXT ) {
                proto_item_set_text(pi,"contextId: $ (Choose Context = 0xfffffffe)");
            } else if ( ctx_id ==  ALL_CONTEXTS ) {
                proto_item_set_text(pi,"contextId: * (All Contexts = 0xffffffff)");
            }

            *ctx_id_p = (guint32) ctx_id;
		}
	}

	return offset;
}

void h248_register_package(const h248_package_t* pkg) {
	if (! packages) packages = g_ptr_array_new();

	g_assert(pkg != NULL);

	g_ptr_array_add(packages,(void*)pkg);
}

static guint32 packageandid;

static int dissect_h248_PkgdName(gboolean implicit_tag, tvbuff_t *tvb, int offset, asn1_ctx_t *actx _U_, proto_tree *tree, int hf_index) {
  tvbuff_t *new_tvb = NULL;
  proto_tree *package_tree=NULL;
  guint16 name_major, name_minor;
  int old_offset;
  const h248_package_t* pkg = NULL;
  guint i;

  old_offset=offset;
  offset = dissect_ber_octet_string(implicit_tag, actx->pinfo, tree, tvb, offset, hf_index, &new_tvb);

  if (new_tvb) {
    /* this field is always 4 bytes  so just read it into two integers */
    name_major=tvb_get_ntohs(new_tvb, 0);
    name_minor=tvb_get_ntohs(new_tvb, 2);
    packageandid=(name_major<<16)|name_minor;

    /* do the prettification */
    proto_item_append_text(ber_last_created_item, "  %s (%04x)", val_to_str(name_major, package_name_vals, "Unknown Package"), name_major);

    if(tree){
		package_tree = proto_item_add_subtree(ber_last_created_item, ett_packagename);
		proto_tree_add_uint(package_tree, hf_h248_pkg_name, tvb, offset-4, 2, name_major);
    }

	for(i=0; i < packages->len; i++) {
		pkg = g_ptr_array_index(packages,i);

		if (name_major == pkg->id) {
			break;
		} else {
			pkg = NULL;
		}
	}

	if (! pkg ) pkg = &no_package;
		
	{
		proto_item* pi = proto_tree_add_uint(package_tree, hf_248_pkg_param, tvb, offset-2, 2, name_minor);
		const gchar* strval;
		
		if (pkg->param_names && ( strval = match_strval(name_minor, pkg->param_names) )) {
			strval = ep_strdup_printf("%s (%d)",strval,name_minor);
		} else {
			strval = ep_strdup_printf("Unknown (%d)",name_minor);
		}
	
		proto_item_set_text(pi,"Parameter: %s", strval);
	}
  } else {
	  pkg = &no_package;
  }

  curr_info.pkg = pkg;

  return offset;
}

static int dissect_h248_EventName(gboolean implicit_tag, tvbuff_t *tvb, int offset, asn1_ctx_t *actx _U_, proto_tree *tree, int hf_index) {
  tvbuff_t *new_tvb;
  proto_tree *package_tree=NULL;
  guint16 name_major, name_minor;
  int old_offset;
  const h248_package_t* pkg = NULL;
  const h248_pkg_evt_t* evt = NULL;
  guint i;

  old_offset=offset;
  offset = dissect_ber_octet_string(implicit_tag, actx->pinfo, tree, tvb, offset, hf_index, &new_tvb);

  if (new_tvb) {
    /* this field is always 4 bytes  so just read it into two integers */
    name_major=tvb_get_ntohs(new_tvb, 0);
    name_minor=tvb_get_ntohs(new_tvb, 2);
    packageandid=(name_major<<16)|name_minor;

    /* do the prettification */
    proto_item_append_text(ber_last_created_item, "  %s (%04x)", val_to_str(name_major, package_name_vals, "Unknown Package"), name_major);
    if(tree){
      package_tree = proto_item_add_subtree(ber_last_created_item, ett_packagename);
    }
    proto_tree_add_uint(package_tree, hf_h248_pkg_name, tvb, offset-4, 2, name_major);


	for(i=0; i < packages->len; i++) {
		pkg = g_ptr_array_index(packages,i);

		if (name_major == pkg->id) {
			break;
		} else {
			pkg = NULL;
		}
	}

	if (! pkg ) pkg = &no_package;

	curr_info.pkg = pkg;

	if (pkg->events) {
		for (evt = pkg->events; evt->hfid; evt++) {
			if (name_minor == evt->id) {
				break;
			}
		}

		if (! evt->hfid) evt = &no_event;
	} else {
		evt = &no_event;
	}

	curr_info.evt = evt;

	{
		proto_item* pi = proto_tree_add_uint(package_tree, hf_h248_event_code, tvb, offset-2, 2, name_minor);
		const gchar* strval;
	
		if (pkg->event_names && ( strval = match_strval(name_minor, pkg->event_names) )) {
			strval = ep_strdup_printf("%s (%d)",strval,name_minor);
		} else {
			strval = ep_strdup_printf("Unknown (%d)",name_minor);
		}
	
		proto_item_set_text(pi,"Event ID: %s", strval);
	}

  } else {
	  curr_info.pkg = &no_package;
	  curr_info.evt = &no_event;
  }

  return offset;
}



static int dissect_h248_SignalName(gboolean implicit_tag , tvbuff_t *tvb, int offset,  asn1_ctx_t *actx _U_, proto_tree *tree, int hf_index) {
  tvbuff_t *new_tvb;
  proto_tree *package_tree=NULL;
  guint16 name_major, name_minor;
  int old_offset;
  const h248_package_t* pkg = NULL;
  const h248_pkg_sig_t* sig;
  guint i;

  old_offset=offset;
  offset = dissect_ber_octet_string(implicit_tag, actx->pinfo, tree, tvb, offset, hf_index, &new_tvb);

  if (new_tvb) {
    /* this field is always 4 bytes so just read it into two integers */
    name_major=tvb_get_ntohs(new_tvb, 0);
    name_minor=tvb_get_ntohs(new_tvb, 2);
    packageandid=(name_major<<16)|name_minor;

    /* do the prettification */
    proto_item_append_text(ber_last_created_item, "  %s (%04x)", val_to_str(name_major, package_name_vals, "Unknown Package"), name_major);
    if(tree){
      package_tree = proto_item_add_subtree(ber_last_created_item, ett_packagename);
    }
    proto_tree_add_uint(package_tree, hf_h248_pkg_name, tvb, offset-4, 2, name_major);

    for(i=0; i < packages->len; i++) {
		pkg = g_ptr_array_index(packages,i);

		if (name_major == pkg->id) {
			break;
		} else {
			pkg = NULL;
		}
	}

	if (! pkg ) pkg = &no_package;

	if (pkg->signals) {
		for (sig = pkg->signals; sig->hfid; sig++) {
			if (name_minor == sig->id) {
				break;
			}
		}

		if (! sig->hfid) sig = &no_signal;

		curr_info.pkg = pkg;
		curr_info.sig = sig;
	} else {
		curr_info.pkg = &no_package;
		curr_info.sig = &no_signal;
	}

	{
		proto_item* pi = proto_tree_add_uint(package_tree, hf_h248_signal_code, tvb, offset-2, 2, name_minor);
		const gchar* strval;
	
		if (pkg->signal_names && ( strval = match_strval(name_minor, pkg->signal_names) )) {
			strval = ep_strdup_printf("%s (%d)",strval,name_minor);
		} else {
			strval = ep_strdup_printf("Unknown (%d)",name_minor);
		}
	
		proto_item_set_text(pi,"Signal ID: %s", strval);
	}

  } else {
	  curr_info.pkg = &no_package;
	  curr_info.sig = &no_signal;
  }

  return offset;
}

static int dissect_h248_PropertyID(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset,  asn1_ctx_t *actx _U_, proto_tree *tree, int hf_index _U_) {

	gint8 class;
	gboolean pc, ind;
	gint32 tag;
	guint32 len;
	guint16 name_major;
	guint16 name_minor;
	int old_offset, end_offset;
	tvbuff_t *next_tvb;
	const h248_package_t* pkg;
	const h248_pkg_param_t* prop;

	old_offset=offset;
	offset=dissect_ber_identifier(actx->pinfo, tree, tvb, offset, &class, &pc, &tag);
	offset=dissect_ber_length(actx->pinfo, tree, tvb, offset, &len, &ind);
	end_offset=offset+len;

	if( (class!=BER_CLASS_UNI)
	  ||(tag!=BER_UNI_TAG_OCTETSTRING) ){
		proto_tree_add_text(tree, tvb, offset-2, 2, "H.248 BER Error: OctetString expected but Class:%d PC:%d Tag:%d was unexpected", class, pc, tag);
		return end_offset;
	}


	next_tvb = tvb_new_subset(tvb, offset , len , len );
	name_major = packageandid >> 16;
	name_minor = packageandid & 0xffff;

	pkg = (curr_info.pkg) ? curr_info.pkg : &no_package;

	if (pkg->properties) {
		for (prop = pkg->properties; prop && prop->hfid; prop++) {
			if (name_minor == prop->id) {
				break;
			}
		}
	} else {
		prop = &no_param;
	}

	if (prop && prop->hfid ) {
		if (!prop->dissector) prop = &no_param;
		prop->dissector(tree, next_tvb, actx->pinfo, *(prop->hfid), &curr_info, prop->data);
	}

	return end_offset;
}


static int dissect_h248_SigParameterName(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset,  asn1_ctx_t *actx _U_, proto_tree *tree, int hf_index _U_) {
	tvbuff_t *next_tvb;
	guint32 param_id = 0xffffffff;
	const h248_pkg_param_t* sigpar;
	const gchar* strval;
	proto_item* pi;
	
	offset = dissect_ber_octet_string(implicit_tag, actx->pinfo, tree, tvb, offset,  hf_index, &next_tvb);
	pi = get_ber_last_created_item();
	
	switch(tvb_length(next_tvb)) {
		case 4: param_id = tvb_get_ntohl(next_tvb,0); break;
		case 3: param_id = tvb_get_ntoh24(next_tvb,0); break;
		case 2: param_id = tvb_get_ntohs(next_tvb,0); break;
		case 1: param_id = tvb_get_guint8(next_tvb,0); break;
		default: break;
	}

	curr_info.par = &no_param;

	if (curr_info.sig && curr_info.sig->parameters) {
		for(sigpar = curr_info.sig->parameters; sigpar->hfid; sigpar++) {
			if (sigpar->id == param_id) {
				curr_info.par = sigpar;
				break;
			}
		}
	}

	if (curr_info.sig && curr_info.sig->param_names && ( strval = match_strval(param_id, curr_info.sig->param_names) )) {
		strval = ep_strdup_printf("%s (%d)",strval,param_id);
	} else {
		strval = ep_strdup_printf("Unknown (%d)",param_id);
	}
	
	proto_item_set_text(pi,"Parameter: %s", strval);

	return offset;
}

static int dissect_h248_SigParamValue(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset,  asn1_ctx_t *actx _U_, proto_tree *tree, int hf_index _U_) {
	tvbuff_t *next_tvb;
	int old_offset, end_offset;
	gint8 class;
	gboolean pc, ind;
	gint32 tag;
	guint32 len;

	old_offset=offset;
	offset=dissect_ber_identifier(actx->pinfo, tree, tvb, offset, &class, &pc, &tag);
	offset=dissect_ber_length(actx->pinfo, tree, tvb, offset, &len, &ind);
	end_offset=offset+len;

	if( (class!=BER_CLASS_UNI)
		||(tag!=BER_UNI_TAG_OCTETSTRING) ){
		proto_tree_add_text(tree, tvb, offset-2, 2, "H.248 BER Error: OctetString expected but Class:%d PC:%d Tag:%d was unexpected", class, pc, tag);
		return end_offset;
	}


	next_tvb = tvb_new_subset(tvb,offset,len,len);

	if ( curr_info.par && curr_info.par->dissector) {
		curr_info.par->dissector(tree, next_tvb, actx->pinfo, *(curr_info.par->hfid), &curr_info, curr_info.par->data);
	}

	return end_offset;
}

static int dissect_h248_EventParameterName(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset,  asn1_ctx_t *actx _U_, proto_tree *tree, int hf_index _U_) {
	tvbuff_t *next_tvb;
	guint32 param_id = 0xffffffff;
	const h248_pkg_param_t* evtpar;
	const gchar* strval;
	proto_item* pi;

	offset = dissect_ber_octet_string(implicit_tag, actx->pinfo, tree, tvb, offset, hf_index, &next_tvb);
	pi = get_ber_last_created_item();

	if (next_tvb) {
		switch(tvb_length(next_tvb)) {
			case 4: param_id = tvb_get_ntohl(next_tvb,0); break;
			case 3: param_id = tvb_get_ntoh24(next_tvb,0); break;
			case 2: param_id = tvb_get_ntohs(next_tvb,0); break;
			case 1: param_id = tvb_get_guint8(next_tvb,0); break;
			default: break;
		}
	}


	curr_info.par = &no_param;

	if (curr_info.evt && curr_info.evt->parameters) {
		for(evtpar = curr_info.evt->parameters; evtpar->hfid; evtpar++) {
			if (evtpar->id == param_id) {
				curr_info.par = evtpar;
				break;
			}
		}
	} else {
		curr_info.par = &no_param;
	}
	
	if (curr_info.evt && curr_info.evt->param_names && ( strval = match_strval(param_id, curr_info.evt->param_names) )) {
		strval = ep_strdup_printf("%s (%d)",strval,param_id);
	} else {
		strval = ep_strdup_printf("Unknown (%d)",param_id);
	}
	
	proto_item_set_text(pi,"Parameter: %s", strval);

	
	return offset;
}

static int dissect_h248_EventParamValue(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset,  asn1_ctx_t *actx _U_, proto_tree *tree, int hf_index _U_) {
	tvbuff_t *next_tvb;
	int old_offset, end_offset;
	gint8 class;
	gboolean pc, ind;
	gint32 tag;
	guint32 len;

	old_offset=offset;
	offset=dissect_ber_identifier(actx->pinfo, tree, tvb, offset, &class, &pc, &tag);
	offset=dissect_ber_length(actx->pinfo, tree, tvb, offset, &len, &ind);
	end_offset=offset+len;

	if( (class!=BER_CLASS_UNI)
		||(tag!=BER_UNI_TAG_OCTETSTRING) ){
		proto_tree_add_text(tree, tvb, offset-2, 2, "H.248 BER Error: OctetString expected but Class:%d PC:%d Tag:%d was unexpected", class, pc, tag);
		return end_offset;
	}


	next_tvb = tvb_new_subset(tvb,offset,len,len);

	if ( curr_info.par && curr_info.par->dissector) {
		curr_info.par->dissector(tree, next_tvb, actx->pinfo, *(curr_info.par->hfid), &curr_info, curr_info.par->data);
	}

	return end_offset;
}

static int dissect_h248_MtpAddress(gboolean implicit_tag, tvbuff_t *tvb, int offset,  asn1_ctx_t *actx _U_, proto_tree *tree, int hf_index) {
  tvbuff_t *new_tvb;
  proto_tree *mtp_tree=NULL;
  guint32 val;
  int i, len, old_offset;

  old_offset=offset;
  offset = dissect_ber_octet_string(implicit_tag, actx->pinfo, tree, tvb, offset, hf_index, &new_tvb);

  if (new_tvb) {
    /* this field is either 2 or 4 bytes  so just read it into an integer */
    val=0;
    len=tvb_length(new_tvb);
    for(i=0;i<len;i++){
      val= (val<<8)|tvb_get_guint8(new_tvb, i);
    }

    /* do the prettification */
    proto_item_append_text(ber_last_created_item, "  NI = %d, PC = %d ( %d-%d )", val&0x03,val>>2,val&0x03,val>>2);
    if(tree){
      mtp_tree = proto_item_add_subtree(ber_last_created_item, ett_mtpaddress);
    }
    proto_tree_add_uint(mtp_tree, hf_h248_mtpaddress_ni, tvb, old_offset, offset-old_offset, val&0x03);
    proto_tree_add_uint(mtp_tree, hf_h248_mtpaddress_pc, tvb, old_offset, offset-old_offset, val>>2);
  }

  return offset;
}

#define H248_TAP() do { if (keep_persistent_data && curr_info.cmd) tap_queue_packet(h248_tap, actx->pinfo, curr_info.cmd); } while(0)


/*--- Included file: packet-h248-fn.c ---*/
#line 1 "packet-h248-fn.c"
/*--- Cyclic dependencies ---*/

/* SecondEventsDescriptor -> SecondEventsDescriptor/eventList -> SecondRequestedEvent -> SecondRequestedActions -> NotifyBehaviour -> RegulatedEmbeddedDescriptor -> SecondEventsDescriptor */
static int dissect_h248_SecondEventsDescriptor(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_);

static int dissect_secondEvent_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SecondEventsDescriptor(TRUE, tvb, offset, actx, tree, hf_h248_secondEvent);
}


/*--- Fields for imported types ---*/




static int
dissect_h248_SecurityParmIndex(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, actx->pinfo, tree, tvb, offset, hf_index,
                                       NULL);

  return offset;
}
static int dissect_secParmIndex_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SecurityParmIndex(TRUE, tvb, offset, actx, tree, hf_h248_secParmIndex);
}



static int
dissect_h248_SequenceNum(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, actx->pinfo, tree, tvb, offset, hf_index,
                                       NULL);

  return offset;
}
static int dissect_seqNum_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SequenceNum(TRUE, tvb, offset, actx, tree, hf_h248_seqNum);
}



static int
dissect_h248_AuthData(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, actx->pinfo, tree, tvb, offset, hf_index,
                                       NULL);

  return offset;
}
static int dissect_ad_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_AuthData(TRUE, tvb, offset, actx, tree, hf_h248_ad);
}


static const ber_sequence_t AuthenticationHeader_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_secParmIndex_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_seqNum_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_IMPLTAG, dissect_ad_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_AuthenticationHeader(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   AuthenticationHeader_sequence, hf_index, ett_h248_AuthenticationHeader);

  return offset;
}
static int dissect_authHeader_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_AuthenticationHeader(TRUE, tvb, offset, actx, tree, hf_h248_authHeader);
}



static int
dissect_h248_INTEGER_0_99(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx->pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_version_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_INTEGER_0_99(TRUE, tvb, offset, actx, tree, hf_h248_version);
}
static int dissect_packageVersion_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_INTEGER_0_99(TRUE, tvb, offset, actx, tree, hf_h248_packageVersion);
}
static int dissect_startTimer_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_INTEGER_0_99(TRUE, tvb, offset, actx, tree, hf_h248_startTimer);
}
static int dissect_shortTimer_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_INTEGER_0_99(TRUE, tvb, offset, actx, tree, hf_h248_shortTimer);
}
static int dissect_longTimer_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_INTEGER_0_99(TRUE, tvb, offset, actx, tree, hf_h248_longTimer);
}
static int dissect_durationTimer_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_INTEGER_0_99(TRUE, tvb, offset, actx, tree, hf_h248_durationTimer);
}
static int dissect_serviceChangeVersion_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_INTEGER_0_99(TRUE, tvb, offset, actx, tree, hf_h248_serviceChangeVersion);
}



static int
dissect_h248_OCTET_STRING_SIZE_4(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, actx->pinfo, tree, tvb, offset, hf_index,
                                       NULL);

  return offset;
}
static int dissect_iP4Address_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_OCTET_STRING_SIZE_4(TRUE, tvb, offset, actx, tree, hf_h248_iP4Address);
}



static int
dissect_h248_INTEGER_0_65535(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx->pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_portNumber_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_INTEGER_0_65535(TRUE, tvb, offset, actx, tree, hf_h248_portNumber);
}
static int dissect_propGroupID_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_INTEGER_0_65535(TRUE, tvb, offset, actx, tree, hf_h248_propGroupID);
}
static int dissect_id_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_INTEGER_0_65535(TRUE, tvb, offset, actx, tree, hf_h248_id);
}
static int dissect_duration_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_INTEGER_0_65535(TRUE, tvb, offset, actx, tree, hf_h248_duration);
}
static int dissect_intersigDelay_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_INTEGER_0_65535(TRUE, tvb, offset, actx, tree, hf_h248_intersigDelay);
}
static int dissect_manufacturerCode_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_INTEGER_0_65535(TRUE, tvb, offset, actx, tree, hf_h248_manufacturerCode);
}


static const ber_sequence_t IP4Address_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_iP4Address_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_portNumber_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_IP4Address(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   IP4Address_sequence, hf_index, ett_h248_IP4Address);

  return offset;
}
static int dissect_ip4Address_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_IP4Address(TRUE, tvb, offset, actx, tree, hf_h248_ip4Address);
}



static int
dissect_h248_OCTET_STRING_SIZE_16(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, actx->pinfo, tree, tvb, offset, hf_index,
                                       NULL);

  return offset;
}
static int dissect_iP6Address_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_OCTET_STRING_SIZE_16(TRUE, tvb, offset, actx, tree, hf_h248_iP6Address);
}


static const ber_sequence_t IP6Address_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_iP6Address_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_portNumber_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_IP6Address(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   IP6Address_sequence, hf_index, ett_h248_IP6Address);

  return offset;
}
static int dissect_ip6Address_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_IP6Address(TRUE, tvb, offset, actx, tree, hf_h248_ip6Address);
}



static int
dissect_h248_IA5String(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_restricted_string(implicit_tag, BER_UNI_TAG_IA5String,
                                            actx->pinfo, tree, tvb, offset, hf_index,
                                            NULL);

  return offset;
}
static int dissect_domName_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_IA5String(TRUE, tvb, offset, actx, tree, hf_h248_domName);
}
static int dissect_digitMapBody_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_IA5String(TRUE, tvb, offset, actx, tree, hf_h248_digitMapBody);
}


static const ber_sequence_t DomainName_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_domName_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_portNumber_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_DomainName(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   DomainName_sequence, hf_index, ett_h248_DomainName);

  return offset;
}
static int dissect_domainName_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_DomainName(TRUE, tvb, offset, actx, tree, hf_h248_domainName);
}



static int
dissect_h248_PathName(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_restricted_string(implicit_tag, BER_UNI_TAG_IA5String,
                                            actx->pinfo, tree, tvb, offset, hf_index,
                                            NULL);

  return offset;
}
static int dissect_deviceName_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_PathName(TRUE, tvb, offset, actx, tree, hf_h248_deviceName);
}

static int dissect_mtpAddress_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_MtpAddress(TRUE, tvb, offset, actx, tree, hf_h248_mtpAddress);
}


static const value_string h248_MId_vals[] = {
  {   0, "ip4Address" },
  {   1, "ip6Address" },
  {   2, "domainName" },
  {   3, "deviceName" },
  {   4, "mtpAddress" },
  { 0, NULL }
};

static const ber_choice_t MId_choice[] = {
  {   0, BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_ip4Address_impl },
  {   1, BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_ip6Address_impl },
  {   2, BER_CLASS_CON, 2, BER_FLAGS_IMPLTAG, dissect_domainName_impl },
  {   3, BER_CLASS_CON, 3, BER_FLAGS_IMPLTAG, dissect_deviceName_impl },
  {   4, BER_CLASS_CON, 4, BER_FLAGS_IMPLTAG, dissect_mtpAddress_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_h248_MId(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_choice(actx, tree, tvb, offset,
                                 MId_choice, hf_index, ett_h248_MId,
                                 NULL);

  return offset;
}
static int dissect_mId_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_MId(TRUE, tvb, offset, actx, tree, hf_h248_mId);
}
static int dissect_serviceChangeMgcId_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_MId(TRUE, tvb, offset, actx, tree, hf_h248_serviceChangeMgcId);
}




static int
dissect_h248_T_errorCode(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 237 "h248.cnf"
    offset = dissect_ber_integer(implicit_tag, actx->pinfo, tree, tvb, offset, hf_h248_error_code, &error_code);
    expert_add_info_format(actx->pinfo, get_ber_last_created_item(), PI_RESPONSE_CODE, PI_WARN, "Errored Command");
    
    if (curr_info.cmd) {
        gcp_cmd_set_error(curr_info.cmd,error_code);
    } else if (curr_info.trx) {
        gcp_trx_set_error(curr_info.trx,error_code);
    }
    
    return offset;


  return offset;
}
static int dissect_errorCode_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_T_errorCode(TRUE, tvb, offset, actx, tree, hf_h248_errorCode);
}



static int
dissect_h248_ErrorText(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_restricted_string(implicit_tag, BER_UNI_TAG_IA5String,
                                            actx->pinfo, tree, tvb, offset, hf_index,
                                            NULL);

  return offset;
}
static int dissect_errorText_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_ErrorText(TRUE, tvb, offset, actx, tree, hf_h248_errorText);
}


static const ber_sequence_t ErrorDescriptor_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_errorCode_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_errorText_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_ErrorDescriptor(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   ErrorDescriptor_sequence, hf_index, ett_h248_ErrorDescriptor);

  return offset;
}
static int dissect_messageError_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_ErrorDescriptor(TRUE, tvb, offset, actx, tree, hf_h248_messageError);
}
static int dissect_transactionError_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_ErrorDescriptor(TRUE, tvb, offset, actx, tree, hf_h248_transactionError);
}
static int dissect_errorDescriptor_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_ErrorDescriptor(TRUE, tvb, offset, actx, tree, hf_h248_errorDescriptor);
}
static int dissect_error_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_ErrorDescriptor(TRUE, tvb, offset, actx, tree, hf_h248_error);
}



static int
dissect_h248_TransactionId(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx->pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_transactionId1_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_TransactionId(TRUE, tvb, offset, actx, tree, hf_h248_transactionId1);
}
static int dissect_firstAck_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_TransactionId(TRUE, tvb, offset, actx, tree, hf_h248_firstAck);
}
static int dissect_lastAck_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_TransactionId(TRUE, tvb, offset, actx, tree, hf_h248_lastAck);
}



static int
dissect_h248_transactionId(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 92 "h248.cnf"
    guint32 trx_id = 0;
	offset = dissect_h248_trx_id(implicit_tag, actx->pinfo, tree, tvb, offset, &trx_id);
    curr_info.trx = gcp_trx(curr_info.msg, trx_id, GCP_TRX_REQUEST, keep_persistent_data);
    error_code = 0;


  return offset;
}
static int dissect_transactionId_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_transactionId(TRUE, tvb, offset, actx, tree, hf_h248_transactionId);
}




static int
dissect_h248_contextId(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 99 "h248.cnf"
    guint32 ctx_id = 0;
	offset = dissect_h248_ctx_id(implicit_tag, actx->pinfo, tree, tvb, offset, &ctx_id);
    curr_info.ctx = gcp_ctx(curr_info.msg,curr_info.trx,ctx_id,keep_persistent_data);
    curr_info.cmd = NULL;
    curr_info.term = NULL;


  return offset;
}
static int dissect_contextId_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_contextId(TRUE, tvb, offset, actx, tree, hf_h248_contextId);
}



static int
dissect_h248_INTEGER_0_15(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx->pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_priority_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_INTEGER_0_15(TRUE, tvb, offset, actx, tree, hf_h248_priority);
}
static int dissect_selectpriority_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_INTEGER_0_15(TRUE, tvb, offset, actx, tree, hf_h248_selectpriority);
}



static int
dissect_h248_BOOLEAN(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_boolean(implicit_tag, actx->pinfo, tree, tvb, offset, hf_index);

  return offset;
}
static int dissect_emergency_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_BOOLEAN(TRUE, tvb, offset, actx, tree, hf_h248_emergency);
}
static int dissect_iepscallind_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_BOOLEAN(TRUE, tvb, offset, actx, tree, hf_h248_iepscallind);
}
static int dissect_selectemergency_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_BOOLEAN(TRUE, tvb, offset, actx, tree, hf_h248_selectemergency);
}
static int dissect_selectiepscallind_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_BOOLEAN(TRUE, tvb, offset, actx, tree, hf_h248_selectiepscallind);
}
static int dissect_range_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_BOOLEAN(TRUE, tvb, offset, actx, tree, hf_h248_range);
}
static int dissect_sublist_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_BOOLEAN(TRUE, tvb, offset, actx, tree, hf_h248_sublist);
}
static int dissect_reserveValue_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_BOOLEAN(TRUE, tvb, offset, actx, tree, hf_h248_reserveValue);
}
static int dissect_reserveGroup_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_BOOLEAN(TRUE, tvb, offset, actx, tree, hf_h248_reserveGroup);
}
static int dissect_keepActive_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_BOOLEAN(TRUE, tvb, offset, actx, tree, hf_h248_keepActive);
}



static int
dissect_h248_WildcardField(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 255 "h248.cnf"
    tvbuff_t* new_tvb;
    offset = dissect_ber_octet_string(implicit_tag, actx->pinfo, tree, tvb, offset, hf_index, &new_tvb);
    tree = proto_item_add_subtree(get_ber_last_created_item(),ett_wildcard);
    proto_tree_add_item(tree,hf_h248_term_wild_type,new_tvb,0,1,FALSE);
    proto_tree_add_item(tree,hf_h248_term_wild_level,new_tvb,0,1,FALSE);
    proto_tree_add_item(tree,hf_h248_term_wild_position,new_tvb,0,1,FALSE);

    wild_term = tvb_get_guint8(new_tvb,0) & 0x80 ? GCP_WILDCARD_CHOOSE : GCP_WILDCARD_ALL;
    


  return offset;
}
static int dissect_wildcard_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_WildcardField(FALSE, tvb, offset, actx, tree, hf_h248_wildcard_item);
}


static const ber_sequence_t SEQUENCE_OF_WildcardField_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_wildcard_item },
};

static int
dissect_h248_SEQUENCE_OF_WildcardField(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                      SEQUENCE_OF_WildcardField_sequence_of, hf_index, ett_h248_SEQUENCE_OF_WildcardField);

  return offset;
}
static int dissect_wildcard_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SEQUENCE_OF_WildcardField(TRUE, tvb, offset, actx, tree, hf_h248_wildcard);
}



static int
dissect_h248_T_terminationId(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 267 "h248.cnf"
	tvbuff_t* new_tvb;
	offset = dissect_ber_octet_string(implicit_tag, actx->pinfo, tree, tvb, offset, hf_index, &new_tvb);
	
	if (new_tvb) {
		curr_info.term->len = tvb_length(new_tvb);
		curr_info.term->type = 0; /* unknown */

		if (curr_info.term->len) {
			curr_info.term->buffer = ep_tvb_memdup(new_tvb,0,curr_info.term->len);
			curr_info.term->str = bytes_to_str(curr_info.term->buffer,curr_info.term->len);
		}

		curr_info.term = gcp_cmd_add_term(curr_info.msg, curr_info.trx, curr_info.cmd, curr_info.term, wild_term, keep_persistent_data);

		if (h248_term_handle) {
			call_dissector(h248_term_handle, new_tvb, actx->pinfo, tree);
		}
	} else {
		curr_info.term->len = 0;
		curr_info.term->buffer = (guint8*)ep_strdup("");
		curr_info.term->str = ep_strdup("?");
	}


  return offset;
}
static int dissect_terminationId_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_T_terminationId(TRUE, tvb, offset, actx, tree, hf_h248_terminationId);
}


static const ber_sequence_t TerminationID_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_wildcard_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_terminationId_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_TerminationID(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 250 "h248.cnf"
    curr_info.term = ep_new0(gcp_term_t);
    wild_term = GCP_WILDCARD_NONE;

  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   TerminationID_sequence, hf_index, ett_h248_TerminationID);

  return offset;
}
static int dissect_terminationFrom_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_TerminationID(TRUE, tvb, offset, actx, tree, hf_h248_terminationFrom);
}
static int dissect_terminationTo_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_TerminationID(TRUE, tvb, offset, actx, tree, hf_h248_terminationTo);
}
static int dissect_terminationID_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_TerminationID(TRUE, tvb, offset, actx, tree, hf_h248_terminationID);
}
static int dissect_TerminationIDList_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_TerminationID(FALSE, tvb, offset, actx, tree, hf_h248_TerminationIDList_item);
}
static int dissect_termList_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_TerminationID(FALSE, tvb, offset, actx, tree, hf_h248_termList_item);
}


static const value_string h248_T_topologyDirection_vals[] = {
  {   0, "bothway" },
  {   1, "isolate" },
  {   2, "oneway" },
  { 0, NULL }
};


static int
dissect_h248_T_topologyDirection(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx->pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_topologyDirection_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_T_topologyDirection(TRUE, tvb, offset, actx, tree, hf_h248_topologyDirection);
}



static int
dissect_h248_StreamID(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx->pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_streamID_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_StreamID(TRUE, tvb, offset, actx, tree, hf_h248_streamID);
}


static const value_string h248_T_topologyDirectionExtension_vals[] = {
  {   0, "onewayexternal" },
  {   1, "onewayboth" },
  { 0, NULL }
};


static int
dissect_h248_T_topologyDirectionExtension(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx->pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_topologyDirectionExtension_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_T_topologyDirectionExtension(TRUE, tvb, offset, actx, tree, hf_h248_topologyDirectionExtension);
}


static const ber_sequence_t TopologyRequest_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_terminationFrom_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_terminationTo_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_IMPLTAG, dissect_topologyDirection_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_streamID_impl },
  { BER_CLASS_CON, 4, BER_FLAGS_IMPLTAG, dissect_topologyDirectionExtension_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_TopologyRequest(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   TopologyRequest_sequence, hf_index, ett_h248_TopologyRequest);

  return offset;
}
static int dissect_topologyReq_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_TopologyRequest(FALSE, tvb, offset, actx, tree, hf_h248_topologyReq_item);
}


static const ber_sequence_t T_topologyReq_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_topologyReq_item },
};

static int
dissect_h248_T_topologyReq(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 181 "h248.cnf"
      curr_info.cmd = gcp_cmd(curr_info.msg,curr_info.trx,curr_info.ctx,GCP_CMD_TOPOLOGY_REQ,offset,keep_persistent_data);
      H248_TAP();

  offset = dissect_ber_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                      T_topologyReq_sequence_of, hf_index, ett_h248_T_topologyReq);

#line 185 "h248.cnf"
      curr_info.cmd = NULL;

  return offset;
}
static int dissect_topologyReq_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_T_topologyReq(TRUE, tvb, offset, actx, tree, hf_h248_topologyReq);
}

static int dissect_name_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_PkgdName(TRUE, tvb, offset, actx, tree, hf_h248_name);
}
static int dissect_iAEDPkgdName_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_PkgdName(TRUE, tvb, offset, actx, tree, hf_h248_iAEDPkgdName);
}
static int dissect_iAEBDEventName_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_PkgdName(TRUE, tvb, offset, actx, tree, hf_h248_iAEBDEventName);
}
static int dissect_iASignalName_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_PkgdName(TRUE, tvb, offset, actx, tree, hf_h248_iASignalName);
}
static int dissect_iAStatName_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_PkgdName(TRUE, tvb, offset, actx, tree, hf_h248_iAStatName);
}
static int dissect_pkgdName_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_PkgdName(TRUE, tvb, offset, actx, tree, hf_h248_pkgdName);
}



static int
dissect_h248_PropertyName(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_h248_PkgdName(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_propertyName_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_PropertyName(TRUE, tvb, offset, actx, tree, hf_h248_propertyName);
}

static int dissect_propertyParamValue_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_PropertyID(FALSE, tvb, offset, actx, tree, hf_h248_propertyParamValue_item);
}


static const ber_sequence_t SEQUENCE_OF_PropertyID_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_propertyParamValue_item },
};

static int
dissect_h248_SEQUENCE_OF_PropertyID(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                      SEQUENCE_OF_PropertyID_sequence_of, hf_index, ett_h248_SEQUENCE_OF_PropertyID);

  return offset;
}
static int dissect_propertyParamValue_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SEQUENCE_OF_PropertyID(TRUE, tvb, offset, actx, tree, hf_h248_propertyParamValue);
}


static const value_string h248_Relation_vals[] = {
  {   0, "greaterThan" },
  {   1, "smallerThan" },
  {   2, "unequalTo" },
  { 0, NULL }
};


static int
dissect_h248_Relation(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx->pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_relation_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_Relation(TRUE, tvb, offset, actx, tree, hf_h248_relation);
}


static const value_string h248_T_extraInfo1_vals[] = {
  {   0, "relation" },
  {   1, "range" },
  {   2, "sublist" },
  { 0, NULL }
};

static const ber_choice_t T_extraInfo1_choice[] = {
  {   0, BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_relation_impl },
  {   1, BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_range_impl },
  {   2, BER_CLASS_CON, 2, BER_FLAGS_IMPLTAG, dissect_sublist_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_h248_T_extraInfo1(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_choice(actx, tree, tvb, offset,
                                 T_extraInfo1_choice, hf_index, ett_h248_T_extraInfo1,
                                 NULL);

  return offset;
}
static int dissect_extraInfo1_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_T_extraInfo1(TRUE, tvb, offset, actx, tree, hf_h248_extraInfo1);
}


static const ber_sequence_t PropertyParm_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_propertyName_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_propertyParamValue_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_extraInfo1_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_PropertyParm(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   PropertyParm_sequence, hf_index, ett_h248_PropertyParm);

  return offset;
}
static int dissect_contextProp_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_PropertyParm(FALSE, tvb, offset, actx, tree, hf_h248_contextProp_item);
}
static int dissect_propertyParms_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_PropertyParm(TRUE, tvb, offset, actx, tree, hf_h248_propertyParms);
}
static int dissect_propertyParms_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_PropertyParm(FALSE, tvb, offset, actx, tree, hf_h248_propertyParms_item);
}
static int dissect_PropertyGroup_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_PropertyParm(FALSE, tvb, offset, actx, tree, hf_h248_PropertyGroup_item);
}
static int dissect_mpl_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_PropertyParm(FALSE, tvb, offset, actx, tree, hf_h248_mpl_item);
}


static const ber_sequence_t SEQUENCE_OF_PropertyParm_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_contextProp_item },
};

static int
dissect_h248_SEQUENCE_OF_PropertyParm(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                      SEQUENCE_OF_PropertyParm_sequence_of, hf_index, ett_h248_SEQUENCE_OF_PropertyParm);

  return offset;
}
static int dissect_contextProp_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SEQUENCE_OF_PropertyParm(TRUE, tvb, offset, actx, tree, hf_h248_contextProp);
}
static int dissect_propertyParms1_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SEQUENCE_OF_PropertyParm(TRUE, tvb, offset, actx, tree, hf_h248_propertyParms1);
}
static int dissect_mpl_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SEQUENCE_OF_PropertyParm(TRUE, tvb, offset, actx, tree, hf_h248_mpl);
}



static int
dissect_h248_ContextIDinList(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx->pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_contextList_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_ContextIDinList(FALSE, tvb, offset, actx, tree, hf_h248_contextList_item);
}


static const ber_sequence_t SEQUENCE_OF_ContextIDinList_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_contextList_item },
};

static int
dissect_h248_SEQUENCE_OF_ContextIDinList(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                      SEQUENCE_OF_ContextIDinList_sequence_of, hf_index, ett_h248_SEQUENCE_OF_ContextIDinList);

  return offset;
}
static int dissect_contextList_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SEQUENCE_OF_ContextIDinList(TRUE, tvb, offset, actx, tree, hf_h248_contextList);
}


static const ber_sequence_t ContextRequest_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_priority_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_emergency_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_topologyReq_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_iepscallind_impl },
  { BER_CLASS_CON, 4, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_contextProp_impl },
  { BER_CLASS_CON, 5, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_contextList_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_ContextRequest(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   ContextRequest_sequence, hf_index, ett_h248_ContextRequest);

  return offset;
}
static int dissect_contextRequest_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_ContextRequest(TRUE, tvb, offset, actx, tree, hf_h248_contextRequest);
}
static int dissect_contextReply_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_ContextRequest(TRUE, tvb, offset, actx, tree, hf_h248_contextReply);
}



static int
dissect_h248_NULL(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_null(implicit_tag, actx->pinfo, tree, tvb, offset, hf_index);

  return offset;
}
static int dissect_immAckRequired_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_NULL(TRUE, tvb, offset, actx, tree, hf_h248_immAckRequired);
}
static int dissect_segmentationComplete_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_NULL(TRUE, tvb, offset, actx, tree, hf_h248_segmentationComplete);
}
static int dissect_topology_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_NULL(TRUE, tvb, offset, actx, tree, hf_h248_topology);
}
static int dissect_cAAREmergency_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_NULL(TRUE, tvb, offset, actx, tree, hf_h248_cAAREmergency);
}
static int dissect_cAARPriority_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_NULL(TRUE, tvb, offset, actx, tree, hf_h248_cAARPriority);
}
static int dissect_iepscallind1_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_NULL(TRUE, tvb, offset, actx, tree, hf_h248_iepscallind1);
}
static int dissect_andAUDITSelect_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_NULL(TRUE, tvb, offset, actx, tree, hf_h248_andAUDITSelect);
}
static int dissect_orAUDITSelect_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_NULL(TRUE, tvb, offset, actx, tree, hf_h248_orAUDITSelect);
}
static int dissect_optional_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_NULL(TRUE, tvb, offset, actx, tree, hf_h248_optional);
}
static int dissect_wildcardReturn_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_NULL(TRUE, tvb, offset, actx, tree, hf_h248_wildcardReturn);
}
static int dissect_iALCDStreamMode_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_NULL(TRUE, tvb, offset, actx, tree, hf_h248_iALCDStreamMode);
}
static int dissect_iALCDReserveValue_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_NULL(TRUE, tvb, offset, actx, tree, hf_h248_iALCDReserveValue);
}
static int dissect_iALCDReserveGroup_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_NULL(TRUE, tvb, offset, actx, tree, hf_h248_iALCDReserveGroup);
}
static int dissect_eventBufferControl_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_NULL(TRUE, tvb, offset, actx, tree, hf_h248_eventBufferControl);
}
static int dissect_iATSDServiceState_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_NULL(TRUE, tvb, offset, actx, tree, hf_h248_iATSDServiceState);
}
static int dissect_notifyImmediate_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_NULL(TRUE, tvb, offset, actx, tree, hf_h248_notifyImmediate);
}
static int dissect_neverNotify_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_NULL(TRUE, tvb, offset, actx, tree, hf_h248_neverNotify);
}
static int dissect_resetEventsDescriptor_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_NULL(TRUE, tvb, offset, actx, tree, hf_h248_resetEventsDescriptor);
}
static int dissect_serviceChangeIncompleteFlag_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_NULL(TRUE, tvb, offset, actx, tree, hf_h248_serviceChangeIncompleteFlag);
}


static const ber_sequence_t IndAudPropertyParm_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_name_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_propertyParms_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_IndAudPropertyParm(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   IndAudPropertyParm_sequence, hf_index, ett_h248_IndAudPropertyParm);

  return offset;
}
static int dissect_contextPropAud_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_IndAudPropertyParm(FALSE, tvb, offset, actx, tree, hf_h248_contextPropAud_item);
}
static int dissect_indAudPropertyParms_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_IndAudPropertyParm(FALSE, tvb, offset, actx, tree, hf_h248_indAudPropertyParms_item);
}
static int dissect_IndAudPropertyGroup_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_IndAudPropertyParm(FALSE, tvb, offset, actx, tree, hf_h248_IndAudPropertyGroup_item);
}


static const ber_sequence_t SEQUENCE_OF_IndAudPropertyParm_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_contextPropAud_item },
};

static int
dissect_h248_SEQUENCE_OF_IndAudPropertyParm(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                      SEQUENCE_OF_IndAudPropertyParm_sequence_of, hf_index, ett_h248_SEQUENCE_OF_IndAudPropertyParm);

  return offset;
}
static int dissect_contextPropAud_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SEQUENCE_OF_IndAudPropertyParm(TRUE, tvb, offset, actx, tree, hf_h248_contextPropAud);
}
static int dissect_indAudPropertyParms_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SEQUENCE_OF_IndAudPropertyParm(TRUE, tvb, offset, actx, tree, hf_h248_indAudPropertyParms);
}


static const value_string h248_SelectLogic_vals[] = {
  {   0, "andAUDITSelect" },
  {   1, "orAUDITSelect" },
  { 0, NULL }
};

static const ber_choice_t SelectLogic_choice[] = {
  {   0, BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_andAUDITSelect_impl },
  {   1, BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_orAUDITSelect_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_h248_SelectLogic(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_choice(actx, tree, tvb, offset,
                                 SelectLogic_choice, hf_index, ett_h248_SelectLogic,
                                 NULL);

  return offset;
}
static int dissect_selectLogic_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SelectLogic(TRUE, tvb, offset, actx, tree, hf_h248_selectLogic);
}


static const ber_sequence_t ContextAttrAuditRequest_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_topology_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_cAAREmergency_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_cAARPriority_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_iepscallind1_impl },
  { BER_CLASS_CON, 4, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_contextPropAud_impl },
  { BER_CLASS_CON, 5, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_selectpriority_impl },
  { BER_CLASS_CON, 6, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_selectemergency_impl },
  { BER_CLASS_CON, 7, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_selectiepscallind_impl },
  { BER_CLASS_CON, 8, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_selectLogic_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_ContextAttrAuditRequest(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   ContextAttrAuditRequest_sequence, hf_index, ett_h248_ContextAttrAuditRequest);

  return offset;
}



static int
dissect_h248_T_contextAttrAuditReq(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 189 "h248.cnf"
      curr_info.cmd = gcp_cmd(curr_info.msg,curr_info.trx,curr_info.ctx,GCP_CMD_CTX_ATTR_AUDIT_REQ,offset,keep_persistent_data);
      H248_TAP();

  offset = dissect_h248_ContextAttrAuditRequest(implicit_tag, tvb, offset, actx, tree, hf_index);

#line 193 "h248.cnf"
      curr_info.cmd = NULL;

  return offset;
}
static int dissect_contextAttrAuditReq_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_T_contextAttrAuditReq(TRUE, tvb, offset, actx, tree, hf_h248_contextAttrAuditReq);
}


static const ber_sequence_t TerminationIDList_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_TerminationIDList_item },
};

static int
dissect_h248_TerminationIDList(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                      TerminationIDList_sequence_of, hf_index, ett_h248_TerminationIDList);

  return offset;
}
static int dissect_terminationIDList_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_TerminationIDList(TRUE, tvb, offset, actx, tree, hf_h248_terminationIDList);
}
static int dissect_contextAuditResult_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_TerminationIDList(TRUE, tvb, offset, actx, tree, hf_h248_contextAuditResult);
}


static const value_string h248_EventBufferControl_vals[] = {
  {   0, "off" },
  {   1, "lockStep" },
  { 0, NULL }
};


static int
dissect_h248_EventBufferControl(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx->pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_tSEventBufferControl_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_EventBufferControl(TRUE, tvb, offset, actx, tree, hf_h248_tSEventBufferControl);
}


static const value_string h248_ServiceState_vals[] = {
  {   0, "test" },
  {   1, "outOfSvc" },
  {   2, "inSvc" },
  { 0, NULL }
};


static int
dissect_h248_ServiceState(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx->pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_serviceStateSel_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_ServiceState(TRUE, tvb, offset, actx, tree, hf_h248_serviceStateSel);
}
static int dissect_serviceState_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_ServiceState(TRUE, tvb, offset, actx, tree, hf_h248_serviceState);
}


static const ber_sequence_t TerminationStateDescriptor_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_propertyParms1_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_tSEventBufferControl_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_serviceState_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_TerminationStateDescriptor(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   TerminationStateDescriptor_sequence, hf_index, ett_h248_TerminationStateDescriptor);

  return offset;
}
static int dissect_termStateDescr_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_TerminationStateDescriptor(TRUE, tvb, offset, actx, tree, hf_h248_termStateDescr);
}


static const value_string h248_StreamMode_vals[] = {
  {   0, "sendOnly" },
  {   1, "recvOnly" },
  {   2, "sendRecv" },
  {   3, "inactive" },
  {   4, "loopBack" },
  { 0, NULL }
};


static int
dissect_h248_StreamMode(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx->pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_streamModeSel_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_StreamMode(TRUE, tvb, offset, actx, tree, hf_h248_streamModeSel);
}
static int dissect_streamMode_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_StreamMode(TRUE, tvb, offset, actx, tree, hf_h248_streamMode);
}


static const ber_sequence_t LocalControlDescriptor_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_streamMode_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_reserveValue_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_reserveGroup_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_IMPLTAG, dissect_propertyParms1_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_LocalControlDescriptor(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   LocalControlDescriptor_sequence, hf_index, ett_h248_LocalControlDescriptor);

  return offset;
}
static int dissect_localControlDescriptor_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_LocalControlDescriptor(TRUE, tvb, offset, actx, tree, hf_h248_localControlDescriptor);
}


static const ber_sequence_t PropertyGroup_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_PropertyGroup_item },
};

static int
dissect_h248_PropertyGroup(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                      PropertyGroup_sequence_of, hf_index, ett_h248_PropertyGroup);

  return offset;
}
static int dissect_propGrps_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_PropertyGroup(FALSE, tvb, offset, actx, tree, hf_h248_propGrps_item);
}


static const ber_sequence_t SEQUENCE_OF_PropertyGroup_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_propGrps_item },
};

static int
dissect_h248_SEQUENCE_OF_PropertyGroup(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                      SEQUENCE_OF_PropertyGroup_sequence_of, hf_index, ett_h248_SEQUENCE_OF_PropertyGroup);

  return offset;
}
static int dissect_propGrps_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SEQUENCE_OF_PropertyGroup(TRUE, tvb, offset, actx, tree, hf_h248_propGrps);
}


static const ber_sequence_t LocalRemoteDescriptor_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_propGrps_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_LocalRemoteDescriptor(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   LocalRemoteDescriptor_sequence, hf_index, ett_h248_LocalRemoteDescriptor);

  return offset;
}
static int dissect_localDescriptor_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_LocalRemoteDescriptor(TRUE, tvb, offset, actx, tree, hf_h248_localDescriptor);
}
static int dissect_remoteDescriptor_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_LocalRemoteDescriptor(TRUE, tvb, offset, actx, tree, hf_h248_remoteDescriptor);
}



static int
dissect_h248_StatName(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_h248_PkgdName(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_statName_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_StatName(TRUE, tvb, offset, actx, tree, hf_h248_statName);
}



static int
dissect_h248_OCTET_STRING(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, actx->pinfo, tree, tvb, offset, hf_index,
                                       NULL);

  return offset;
}
static int dissect_data_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_OCTET_STRING(TRUE, tvb, offset, actx, tree, hf_h248_data);
}
static int dissect_Value_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_OCTET_STRING(FALSE, tvb, offset, actx, tree, hf_h248_Value_item);
}


static const ber_sequence_t Value_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_Value_item },
};

static int
dissect_h248_Value(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                      Value_sequence_of, hf_index, ett_h248_Value);

  return offset;
}



static int
dissect_h248_StatValue(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_h248_Value(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_statValue_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_StatValue(TRUE, tvb, offset, actx, tree, hf_h248_statValue);
}


static const ber_sequence_t StatisticsParameter_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_statName_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_statValue_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_StatisticsParameter(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   StatisticsParameter_sequence, hf_index, ett_h248_StatisticsParameter);

  return offset;
}
static int dissect_StatisticsDescriptor_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_StatisticsParameter(FALSE, tvb, offset, actx, tree, hf_h248_StatisticsDescriptor_item);
}


static const ber_sequence_t StatisticsDescriptor_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_StatisticsDescriptor_item },
};

static int
dissect_h248_StatisticsDescriptor(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                      StatisticsDescriptor_sequence_of, hf_index, ett_h248_StatisticsDescriptor);

  return offset;
}
static int dissect_statisticsDescriptor_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_StatisticsDescriptor(TRUE, tvb, offset, actx, tree, hf_h248_statisticsDescriptor);
}


static const ber_sequence_t StreamParms_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_localControlDescriptor_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_localDescriptor_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_remoteDescriptor_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_statisticsDescriptor_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_StreamParms(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   StreamParms_sequence, hf_index, ett_h248_StreamParms);

  return offset;
}
static int dissect_mediaDescriptorOneStream_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_StreamParms(TRUE, tvb, offset, actx, tree, hf_h248_mediaDescriptorOneStream);
}
static int dissect_streamParms_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_StreamParms(TRUE, tvb, offset, actx, tree, hf_h248_streamParms);
}


static const ber_sequence_t StreamDescriptor_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_streamID_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_streamParms_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_StreamDescriptor(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   StreamDescriptor_sequence, hf_index, ett_h248_StreamDescriptor);

  return offset;
}
static int dissect_mediaDescriptorMultiStream_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_StreamDescriptor(FALSE, tvb, offset, actx, tree, hf_h248_mediaDescriptorMultiStream_item);
}


static const ber_sequence_t SEQUENCE_OF_StreamDescriptor_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_mediaDescriptorMultiStream_item },
};

static int
dissect_h248_SEQUENCE_OF_StreamDescriptor(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                      SEQUENCE_OF_StreamDescriptor_sequence_of, hf_index, ett_h248_SEQUENCE_OF_StreamDescriptor);

  return offset;
}
static int dissect_mediaDescriptorMultiStream_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SEQUENCE_OF_StreamDescriptor(TRUE, tvb, offset, actx, tree, hf_h248_mediaDescriptorMultiStream);
}


static const value_string h248_T_streams_vals[] = {
  {   0, "oneStream" },
  {   1, "multiStream" },
  { 0, NULL }
};

static const ber_choice_t T_streams_choice[] = {
  {   0, BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_mediaDescriptorOneStream_impl },
  {   1, BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_mediaDescriptorMultiStream_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_h248_T_streams(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_choice(actx, tree, tvb, offset,
                                 T_streams_choice, hf_index, ett_h248_T_streams,
                                 NULL);

  return offset;
}
static int dissect_streams_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_T_streams(TRUE, tvb, offset, actx, tree, hf_h248_streams);
}


static const ber_sequence_t MediaDescriptor_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_termStateDescr_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_streams_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_MediaDescriptor(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   MediaDescriptor_sequence, hf_index, ett_h248_MediaDescriptor);

  return offset;
}
static int dissect_mediaDescriptor_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_MediaDescriptor(TRUE, tvb, offset, actx, tree, hf_h248_mediaDescriptor);
}


static const value_string h248_ModemType_vals[] = {
  {   0, "v18" },
  {   1, "v22" },
  {   2, "v22bis" },
  {   3, "v32" },
  {   4, "v32bis" },
  {   5, "v34" },
  {   6, "v90" },
  {   7, "v91" },
  {   8, "synchISDN" },
  { 0, NULL }
};


static int
dissect_h248_ModemType(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx->pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_mtl_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_ModemType(FALSE, tvb, offset, actx, tree, hf_h248_mtl_item);
}


static const ber_sequence_t SEQUENCE_OF_ModemType_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_ENUMERATED, BER_FLAGS_NOOWNTAG, dissect_mtl_item },
};

static int
dissect_h248_SEQUENCE_OF_ModemType(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                      SEQUENCE_OF_ModemType_sequence_of, hf_index, ett_h248_SEQUENCE_OF_ModemType);

  return offset;
}
static int dissect_mtl_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SEQUENCE_OF_ModemType(TRUE, tvb, offset, actx, tree, hf_h248_mtl);
}



static int
dissect_h248_OBJECT_IDENTIFIER(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_object_identifier(implicit_tag, actx->pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_object_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_OBJECT_IDENTIFIER(TRUE, tvb, offset, actx, tree, hf_h248_object);
}



static int
dissect_h248_INTEGER_0_255(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx->pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_t35CountryCode1_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_INTEGER_0_255(TRUE, tvb, offset, actx, tree, hf_h248_t35CountryCode1);
}
static int dissect_t35CountryCode2_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_INTEGER_0_255(TRUE, tvb, offset, actx, tree, hf_h248_t35CountryCode2);
}
static int dissect_t35Extension_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_INTEGER_0_255(TRUE, tvb, offset, actx, tree, hf_h248_t35Extension);
}


static const ber_sequence_t H221NonStandard_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_t35CountryCode1_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_t35CountryCode2_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_IMPLTAG, dissect_t35Extension_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_IMPLTAG, dissect_manufacturerCode_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_H221NonStandard(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   H221NonStandard_sequence, hf_index, ett_h248_H221NonStandard);

  return offset;
}
static int dissect_h221NonStandard_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_H221NonStandard(TRUE, tvb, offset, actx, tree, hf_h248_h221NonStandard);
}



static int
dissect_h248_IA5String_SIZE_8(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_restricted_string(implicit_tag, BER_UNI_TAG_IA5String,
                                            actx->pinfo, tree, tvb, offset, hf_index,
                                            NULL);

  return offset;
}
static int dissect_experimental_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_IA5String_SIZE_8(TRUE, tvb, offset, actx, tree, hf_h248_experimental);
}
static int dissect_date_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_IA5String_SIZE_8(TRUE, tvb, offset, actx, tree, hf_h248_date);
}
static int dissect_time_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_IA5String_SIZE_8(TRUE, tvb, offset, actx, tree, hf_h248_time);
}


static const value_string h248_NonStandardIdentifier_vals[] = {
  {   0, "object" },
  {   1, "h221NonStandard" },
  {   2, "experimental" },
  { 0, NULL }
};

static const ber_choice_t NonStandardIdentifier_choice[] = {
  {   0, BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_object_impl },
  {   1, BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_h221NonStandard_impl },
  {   2, BER_CLASS_CON, 2, BER_FLAGS_IMPLTAG, dissect_experimental_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_h248_NonStandardIdentifier(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_choice(actx, tree, tvb, offset,
                                 NonStandardIdentifier_choice, hf_index, ett_h248_NonStandardIdentifier,
                                 NULL);

  return offset;
}
static int dissect_nonStandardIdentifier_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_NonStandardIdentifier(TRUE, tvb, offset, actx, tree, hf_h248_nonStandardIdentifier);
}


static const ber_sequence_t NonStandardData_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_nonStandardIdentifier_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_data_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_NonStandardData(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   NonStandardData_sequence, hf_index, ett_h248_NonStandardData);

  return offset;
}
static int dissect_nonStandardData_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_NonStandardData(TRUE, tvb, offset, actx, tree, hf_h248_nonStandardData);
}


static const ber_sequence_t ModemDescriptor_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_mtl_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_mpl_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_nonStandardData_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_ModemDescriptor(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   ModemDescriptor_sequence, hf_index, ett_h248_ModemDescriptor);

  return offset;
}
static int dissect_modemDescriptor_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_ModemDescriptor(TRUE, tvb, offset, actx, tree, hf_h248_modemDescriptor);
}


static const value_string h248_MuxType_vals[] = {
  {   0, "h221" },
  {   1, "h223" },
  {   2, "h226" },
  {   3, "v76" },
  {   4, "nx64k" },
  { 0, NULL }
};


static int
dissect_h248_MuxType(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx->pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_muxType_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_MuxType(TRUE, tvb, offset, actx, tree, hf_h248_muxType);
}


static const ber_sequence_t SEQUENCE_OF_TerminationID_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_termList_item },
};

static int
dissect_h248_SEQUENCE_OF_TerminationID(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                      SEQUENCE_OF_TerminationID_sequence_of, hf_index, ett_h248_SEQUENCE_OF_TerminationID);

  return offset;
}
static int dissect_termList_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SEQUENCE_OF_TerminationID(TRUE, tvb, offset, actx, tree, hf_h248_termList);
}


static const ber_sequence_t MuxDescriptor_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_muxType_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_termList_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_nonStandardData_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_MuxDescriptor(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   MuxDescriptor_sequence, hf_index, ett_h248_MuxDescriptor);

  return offset;
}
static int dissect_muxDescriptor_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_MuxDescriptor(TRUE, tvb, offset, actx, tree, hf_h248_muxDescriptor);
}



static int
dissect_h248_RequestID(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx->pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_requestID_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_RequestID(TRUE, tvb, offset, actx, tree, hf_h248_requestID);
}
static int dissect_signalRequestID_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_RequestID(TRUE, tvb, offset, actx, tree, hf_h248_signalRequestID);
}
static int dissect_requestId_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_RequestID(TRUE, tvb, offset, actx, tree, hf_h248_requestId);
}

static int dissect_eventName_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_EventName(TRUE, tvb, offset, actx, tree, hf_h248_eventName);
}



static int
dissect_h248_Name(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, actx->pinfo, tree, tvb, offset, hf_index,
                                       NULL);

  return offset;
}
static int dissect_packageName_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_Name(TRUE, tvb, offset, actx, tree, hf_h248_packageName);
}



static int
dissect_h248_DigitMapName(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_h248_Name(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_digitMapName_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_DigitMapName(TRUE, tvb, offset, actx, tree, hf_h248_digitMapName);
}


static const ber_sequence_t DigitMapValue_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_startTimer_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_shortTimer_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_longTimer_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_IMPLTAG, dissect_digitMapBody_impl },
  { BER_CLASS_CON, 4, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_durationTimer_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_DigitMapValue(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   DigitMapValue_sequence, hf_index, ett_h248_DigitMapValue);

  return offset;
}
static int dissect_digitMapValue_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_DigitMapValue(TRUE, tvb, offset, actx, tree, hf_h248_digitMapValue);
}


static const value_string h248_EventDM_vals[] = {
  {   0, "digitMapName" },
  {   1, "digitMapValue" },
  { 0, NULL }
};

static const ber_choice_t EventDM_choice[] = {
  {   0, BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_digitMapName_impl },
  {   1, BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_digitMapValue_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_h248_EventDM(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_choice(actx, tree, tvb, offset,
                                 EventDM_choice, hf_index, ett_h248_EventDM,
                                 NULL);

  return offset;
}
static int dissect_eventDM_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_EventDM(TRUE, tvb, offset, actx, tree, hf_h248_eventDM);
}

static int dissect_signalName_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SignalName(TRUE, tvb, offset, actx, tree, hf_h248_signalName);
}


static const value_string h248_SignalType_vals[] = {
  {   0, "brief" },
  {   1, "onOff" },
  {   2, "timeOut" },
  { 0, NULL }
};


static int
dissect_h248_SignalType(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx->pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_sigType_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SignalType(TRUE, tvb, offset, actx, tree, hf_h248_sigType);
}


static const asn_namedbit NotifyCompletion_bits[] = {
  {  0, &hf_h248_NotifyCompletion_onTimeOut, -1, -1, "onTimeOut", NULL },
  {  1, &hf_h248_NotifyCompletion_onInterruptByEvent, -1, -1, "onInterruptByEvent", NULL },
  {  2, &hf_h248_NotifyCompletion_onInterruptByNewSignalDescr, -1, -1, "onInterruptByNewSignalDescr", NULL },
  {  3, &hf_h248_NotifyCompletion_otherReason, -1, -1, "otherReason", NULL },
  {  4, &hf_h248_NotifyCompletion_onIteration, -1, -1, "onIteration", NULL },
  { 0, NULL, 0, 0, NULL, NULL }
};

static int
dissect_h248_NotifyCompletion(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_bitstring(implicit_tag, actx->pinfo, tree, tvb, offset,
                                    NotifyCompletion_bits, hf_index, ett_h248_NotifyCompletion,
                                    NULL);

  return offset;
}
static int dissect_notifyCompletion_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_NotifyCompletion(TRUE, tvb, offset, actx, tree, hf_h248_notifyCompletion);
}

static int dissect_sigParameterName_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SigParameterName(TRUE, tvb, offset, actx, tree, hf_h248_sigParameterName);
}

static int dissect_SigParamValues_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SigParamValue(FALSE, tvb, offset, actx, tree, hf_h248_SigParamValues_item);
}


static const ber_sequence_t SigParamValues_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_SigParamValues_item },
};

static int
dissect_h248_SigParamValues(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                      SigParamValues_sequence_of, hf_index, ett_h248_SigParamValues);

  return offset;
}
static int dissect_value_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SigParamValues(TRUE, tvb, offset, actx, tree, hf_h248_value);
}


static const value_string h248_T_extraInfo2_vals[] = {
  {   0, "relation" },
  {   1, "range" },
  {   2, "sublist" },
  { 0, NULL }
};

static const ber_choice_t T_extraInfo2_choice[] = {
  {   0, BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_relation_impl },
  {   1, BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_range_impl },
  {   2, BER_CLASS_CON, 2, BER_FLAGS_IMPLTAG, dissect_sublist_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_h248_T_extraInfo2(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_choice(actx, tree, tvb, offset,
                                 T_extraInfo2_choice, hf_index, ett_h248_T_extraInfo2,
                                 NULL);

  return offset;
}
static int dissect_extraInfo2_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_T_extraInfo2(TRUE, tvb, offset, actx, tree, hf_h248_extraInfo2);
}


static const ber_sequence_t SigParameter_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_sigParameterName_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_value_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_extraInfo2_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_SigParameter(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   SigParameter_sequence, hf_index, ett_h248_SigParameter);

  return offset;
}
static int dissect_sigParList_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SigParameter(FALSE, tvb, offset, actx, tree, hf_h248_sigParList_item);
}


static const ber_sequence_t SEQUENCE_OF_SigParameter_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_sigParList_item },
};

static int
dissect_h248_SEQUENCE_OF_SigParameter(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                      SEQUENCE_OF_SigParameter_sequence_of, hf_index, ett_h248_SEQUENCE_OF_SigParameter);

  return offset;
}
static int dissect_sigParList_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SEQUENCE_OF_SigParameter(TRUE, tvb, offset, actx, tree, hf_h248_sigParList);
}


static const value_string h248_SignalDirection_vals[] = {
  {   0, "internal" },
  {   1, "external" },
  {   2, "both" },
  { 0, NULL }
};


static int
dissect_h248_SignalDirection(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx->pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_direction_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SignalDirection(TRUE, tvb, offset, actx, tree, hf_h248_direction);
}


static const ber_sequence_t Signal_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_signalName_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_streamID_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_sigType_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_duration_impl },
  { BER_CLASS_CON, 4, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_notifyCompletion_impl },
  { BER_CLASS_CON, 5, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_keepActive_impl },
  { BER_CLASS_CON, 6, BER_FLAGS_IMPLTAG, dissect_sigParList_impl },
  { BER_CLASS_CON, 7, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_direction_impl },
  { BER_CLASS_CON, 8, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_requestID_impl },
  { BER_CLASS_CON, 9, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_intersigDelay_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_Signal(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   Signal_sequence, hf_index, ett_h248_Signal);

  return offset;
}
static int dissect_signal_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_Signal(TRUE, tvb, offset, actx, tree, hf_h248_signal);
}
static int dissect_signalList_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_Signal(FALSE, tvb, offset, actx, tree, hf_h248_signalList_item);
}


static const ber_sequence_t SEQUENCE_OF_Signal_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_signalList_item },
};

static int
dissect_h248_SEQUENCE_OF_Signal(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                      SEQUENCE_OF_Signal_sequence_of, hf_index, ett_h248_SEQUENCE_OF_Signal);

  return offset;
}
static int dissect_signalList_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SEQUENCE_OF_Signal(TRUE, tvb, offset, actx, tree, hf_h248_signalList);
}


static const ber_sequence_t SeqSigList_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_id_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_signalList_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_SeqSigList(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   SeqSigList_sequence, hf_index, ett_h248_SeqSigList);

  return offset;
}
static int dissect_seqSigList_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SeqSigList(TRUE, tvb, offset, actx, tree, hf_h248_seqSigList);
}


static const value_string h248_SignalRequest_vals[] = {
  {   0, "signal" },
  {   1, "seqSigList" },
  { 0, NULL }
};

static const ber_choice_t SignalRequest_choice[] = {
  {   0, BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_signal_impl },
  {   1, BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_seqSigList_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_h248_SignalRequest(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_choice(actx, tree, tvb, offset,
                                 SignalRequest_choice, hf_index, ett_h248_SignalRequest,
                                 NULL);

  return offset;
}
static int dissect_SignalsDescriptor_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SignalRequest(FALSE, tvb, offset, actx, tree, hf_h248_SignalsDescriptor_item);
}


static const ber_sequence_t SignalsDescriptor_sequence_of[1] = {
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_SignalsDescriptor_item },
};

static int
dissect_h248_SignalsDescriptor(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                      SignalsDescriptor_sequence_of, hf_index, ett_h248_SignalsDescriptor);

  return offset;
}
static int dissect_signalsDescriptor_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SignalsDescriptor(TRUE, tvb, offset, actx, tree, hf_h248_signalsDescriptor);
}


static const ber_sequence_t RegulatedEmbeddedDescriptor_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_secondEvent_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_signalsDescriptor_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_RegulatedEmbeddedDescriptor(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   RegulatedEmbeddedDescriptor_sequence, hf_index, ett_h248_RegulatedEmbeddedDescriptor);

  return offset;
}
static int dissect_notifyRegulated_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_RegulatedEmbeddedDescriptor(TRUE, tvb, offset, actx, tree, hf_h248_notifyRegulated);
}


static const value_string h248_NotifyBehaviour_vals[] = {
  {   0, "notifyImmediate" },
  {   1, "notifyRegulated" },
  {   2, "neverNotify" },
  { 0, NULL }
};

static const ber_choice_t NotifyBehaviour_choice[] = {
  {   0, BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_notifyImmediate_impl },
  {   1, BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_notifyRegulated_impl },
  {   2, BER_CLASS_CON, 2, BER_FLAGS_IMPLTAG, dissect_neverNotify_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_h248_NotifyBehaviour(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_choice(actx, tree, tvb, offset,
                                 NotifyBehaviour_choice, hf_index, ett_h248_NotifyBehaviour,
                                 NULL);

  return offset;
}
static int dissect_notifyBehaviour_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_NotifyBehaviour(TRUE, tvb, offset, actx, tree, hf_h248_notifyBehaviour);
}


static const ber_sequence_t SecondRequestedActions_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_keepActive_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_eventDM_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_signalsDescriptor_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_notifyBehaviour_impl },
  { BER_CLASS_CON, 4, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_resetEventsDescriptor_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_SecondRequestedActions(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   SecondRequestedActions_sequence, hf_index, ett_h248_SecondRequestedActions);

  return offset;
}
static int dissect_secondaryEventAction_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SecondRequestedActions(TRUE, tvb, offset, actx, tree, hf_h248_secondaryEventAction);
}

static int dissect_eventParameterName_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_EventParameterName(TRUE, tvb, offset, actx, tree, hf_h248_eventParameterName);
}

static int dissect_EventParamValues_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_EventParamValue(FALSE, tvb, offset, actx, tree, hf_h248_EventParamValues_item);
}


static const ber_sequence_t EventParamValues_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_EventParamValues_item },
};

static int
dissect_h248_EventParamValues(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                      EventParamValues_sequence_of, hf_index, ett_h248_EventParamValues);

  return offset;
}
static int dissect_eventParamValue_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_EventParamValues(TRUE, tvb, offset, actx, tree, hf_h248_eventParamValue);
}


static const value_string h248_T_extraInfo_vals[] = {
  {   0, "relation" },
  {   1, "range" },
  {   2, "sublist" },
  { 0, NULL }
};

static const ber_choice_t T_extraInfo_choice[] = {
  {   0, BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_relation_impl },
  {   1, BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_range_impl },
  {   2, BER_CLASS_CON, 2, BER_FLAGS_IMPLTAG, dissect_sublist_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_h248_T_extraInfo(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_choice(actx, tree, tvb, offset,
                                 T_extraInfo_choice, hf_index, ett_h248_T_extraInfo,
                                 NULL);

  return offset;
}
static int dissect_extraInfo_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_T_extraInfo(TRUE, tvb, offset, actx, tree, hf_h248_extraInfo);
}


static const ber_sequence_t EventParameter_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_eventParameterName_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_eventParamValue_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_extraInfo_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_EventParameter(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   EventParameter_sequence, hf_index, ett_h248_EventParameter);

  return offset;
}
static int dissect_eventParList_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_EventParameter(FALSE, tvb, offset, actx, tree, hf_h248_eventParList_item);
}
static int dissect_evParList_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_EventParameter(FALSE, tvb, offset, actx, tree, hf_h248_evParList_item);
}


static const ber_sequence_t SEQUENCE_OF_EventParameter_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_eventParList_item },
};

static int
dissect_h248_SEQUENCE_OF_EventParameter(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                      SEQUENCE_OF_EventParameter_sequence_of, hf_index, ett_h248_SEQUENCE_OF_EventParameter);

  return offset;
}
static int dissect_eventParList_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SEQUENCE_OF_EventParameter(TRUE, tvb, offset, actx, tree, hf_h248_eventParList);
}
static int dissect_evParList_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SEQUENCE_OF_EventParameter(TRUE, tvb, offset, actx, tree, hf_h248_evParList);
}


static const ber_sequence_t SecondRequestedEvent_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_pkgdName_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_streamID_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_secondaryEventAction_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_IMPLTAG, dissect_evParList_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_SecondRequestedEvent(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   SecondRequestedEvent_sequence, hf_index, ett_h248_SecondRequestedEvent);

  return offset;
}
static int dissect_secondaryEventList_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SecondRequestedEvent(FALSE, tvb, offset, actx, tree, hf_h248_secondaryEventList_item);
}


static const ber_sequence_t SEQUENCE_OF_SecondRequestedEvent_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_secondaryEventList_item },
};

static int
dissect_h248_SEQUENCE_OF_SecondRequestedEvent(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                      SEQUENCE_OF_SecondRequestedEvent_sequence_of, hf_index, ett_h248_SEQUENCE_OF_SecondRequestedEvent);

  return offset;
}
static int dissect_secondaryEventList_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SEQUENCE_OF_SecondRequestedEvent(TRUE, tvb, offset, actx, tree, hf_h248_secondaryEventList);
}


static const ber_sequence_t SecondEventsDescriptor_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_requestID_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_secondaryEventList_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_SecondEventsDescriptor(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   SecondEventsDescriptor_sequence, hf_index, ett_h248_SecondEventsDescriptor);

  return offset;
}


static const ber_sequence_t RequestedActions_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_keepActive_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_eventDM_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_secondEvent_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_signalsDescriptor_impl },
  { BER_CLASS_CON, 4, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_notifyBehaviour_impl },
  { BER_CLASS_CON, 5, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_resetEventsDescriptor_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_RequestedActions(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   RequestedActions_sequence, hf_index, ett_h248_RequestedActions);

  return offset;
}
static int dissect_eventAction_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_RequestedActions(TRUE, tvb, offset, actx, tree, hf_h248_eventAction);
}


static const ber_sequence_t RequestedEvent_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_eventName_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_streamID_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_eventAction_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_IMPLTAG, dissect_evParList_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_RequestedEvent(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   RequestedEvent_sequence, hf_index, ett_h248_RequestedEvent);

  return offset;
}
static int dissect_eventList_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_RequestedEvent(FALSE, tvb, offset, actx, tree, hf_h248_eventList_item);
}


static const ber_sequence_t SEQUENCE_OF_RequestedEvent_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_eventList_item },
};

static int
dissect_h248_SEQUENCE_OF_RequestedEvent(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                      SEQUENCE_OF_RequestedEvent_sequence_of, hf_index, ett_h248_SEQUENCE_OF_RequestedEvent);

  return offset;
}
static int dissect_eventList_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SEQUENCE_OF_RequestedEvent(TRUE, tvb, offset, actx, tree, hf_h248_eventList);
}


static const ber_sequence_t EventsDescriptor_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_requestID_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_eventList_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_EventsDescriptor(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   EventsDescriptor_sequence, hf_index, ett_h248_EventsDescriptor);

  return offset;
}
static int dissect_eventsDescriptor_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_EventsDescriptor(TRUE, tvb, offset, actx, tree, hf_h248_eventsDescriptor);
}


static const ber_sequence_t EventSpec_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_eventName_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_streamID_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_IMPLTAG, dissect_eventParList_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_EventSpec(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   EventSpec_sequence, hf_index, ett_h248_EventSpec);

  return offset;
}
static int dissect_EventBufferDescriptor_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_EventSpec(FALSE, tvb, offset, actx, tree, hf_h248_EventBufferDescriptor_item);
}


static const ber_sequence_t EventBufferDescriptor_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_EventBufferDescriptor_item },
};

static int
dissect_h248_EventBufferDescriptor(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                      EventBufferDescriptor_sequence_of, hf_index, ett_h248_EventBufferDescriptor);

  return offset;
}
static int dissect_eventBufferDescriptor_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_EventBufferDescriptor(TRUE, tvb, offset, actx, tree, hf_h248_eventBufferDescriptor);
}


static const ber_sequence_t DigitMapDescriptor_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_digitMapName_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_digitMapValue_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_DigitMapDescriptor(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   DigitMapDescriptor_sequence, hf_index, ett_h248_DigitMapDescriptor);

  return offset;
}
static int dissect_digitMapDescriptor_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_DigitMapDescriptor(TRUE, tvb, offset, actx, tree, hf_h248_digitMapDescriptor);
}


static const asn_namedbit T_auditToken_bits[] = {
  {  0, &hf_h248_T_auditToken_muxToken, -1, -1, "muxToken", NULL },
  {  1, &hf_h248_T_auditToken_modemToken, -1, -1, "modemToken", NULL },
  {  2, &hf_h248_T_auditToken_mediaToken, -1, -1, "mediaToken", NULL },
  {  3, &hf_h248_T_auditToken_eventsToken, -1, -1, "eventsToken", NULL },
  {  4, &hf_h248_T_auditToken_signalsToken, -1, -1, "signalsToken", NULL },
  {  5, &hf_h248_T_auditToken_digitMapToken, -1, -1, "digitMapToken", NULL },
  {  6, &hf_h248_T_auditToken_statsToken, -1, -1, "statsToken", NULL },
  {  7, &hf_h248_T_auditToken_observedEventsToken, -1, -1, "observedEventsToken", NULL },
  {  8, &hf_h248_T_auditToken_packagesToken, -1, -1, "packagesToken", NULL },
  {  9, &hf_h248_T_auditToken_eventBufferToken, -1, -1, "eventBufferToken", NULL },
  { 0, NULL, 0, 0, NULL, NULL }
};

static int
dissect_h248_T_auditToken(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_bitstring(implicit_tag, actx->pinfo, tree, tvb, offset,
                                    T_auditToken_bits, hf_index, ett_h248_T_auditToken,
                                    NULL);

  return offset;
}
static int dissect_auditToken_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_T_auditToken(TRUE, tvb, offset, actx, tree, hf_h248_auditToken);
}


static const ber_sequence_t IndAudTerminationStateDescriptor_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_indAudPropertyParms_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_eventBufferControl_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_iATSDServiceState_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_serviceStateSel_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_IndAudTerminationStateDescriptor(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   IndAudTerminationStateDescriptor_sequence, hf_index, ett_h248_IndAudTerminationStateDescriptor);

  return offset;
}
static int dissect_indAudTerminationStateDescriptor_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_IndAudTerminationStateDescriptor(TRUE, tvb, offset, actx, tree, hf_h248_indAudTerminationStateDescriptor);
}


static const ber_sequence_t IndAudLocalControlDescriptor_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_iALCDStreamMode_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_iALCDReserveValue_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_iALCDReserveGroup_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_indAudPropertyParms_impl },
  { BER_CLASS_CON, 4, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_streamModeSel_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_IndAudLocalControlDescriptor(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   IndAudLocalControlDescriptor_sequence, hf_index, ett_h248_IndAudLocalControlDescriptor);

  return offset;
}
static int dissect_iASPLocalControlDescriptor_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_IndAudLocalControlDescriptor(TRUE, tvb, offset, actx, tree, hf_h248_iASPLocalControlDescriptor);
}


static const ber_sequence_t IndAudPropertyGroup_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_IndAudPropertyGroup_item },
};

static int
dissect_h248_IndAudPropertyGroup(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                      IndAudPropertyGroup_sequence_of, hf_index, ett_h248_IndAudPropertyGroup);

  return offset;
}
static int dissect_iAPropertyGroup_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_IndAudPropertyGroup(TRUE, tvb, offset, actx, tree, hf_h248_iAPropertyGroup);
}


static const ber_sequence_t IndAudLocalRemoteDescriptor_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_propGroupID_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_iAPropertyGroup_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_IndAudLocalRemoteDescriptor(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   IndAudLocalRemoteDescriptor_sequence, hf_index, ett_h248_IndAudLocalRemoteDescriptor);

  return offset;
}
static int dissect_iASPLocalDescriptor_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_IndAudLocalRemoteDescriptor(TRUE, tvb, offset, actx, tree, hf_h248_iASPLocalDescriptor);
}
static int dissect_iASPRemoteDescriptor_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_IndAudLocalRemoteDescriptor(TRUE, tvb, offset, actx, tree, hf_h248_iASPRemoteDescriptor);
}


static const ber_sequence_t IndAudStatisticsDescriptor_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_iAStatName_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_IndAudStatisticsDescriptor(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   IndAudStatisticsDescriptor_sequence, hf_index, ett_h248_IndAudStatisticsDescriptor);

  return offset;
}
static int dissect_indaudstatisticsDescriptor_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_IndAudStatisticsDescriptor(TRUE, tvb, offset, actx, tree, hf_h248_indaudstatisticsDescriptor);
}
static int dissect_statisticsDescriptor1_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_IndAudStatisticsDescriptor(TRUE, tvb, offset, actx, tree, hf_h248_statisticsDescriptor1);
}


static const ber_sequence_t IndAudStreamParms_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_iASPLocalControlDescriptor_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_iASPLocalDescriptor_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_iASPRemoteDescriptor_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_statisticsDescriptor1_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_IndAudStreamParms(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   IndAudStreamParms_sequence, hf_index, ett_h248_IndAudStreamParms);

  return offset;
}
static int dissect_oneStream_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_IndAudStreamParms(TRUE, tvb, offset, actx, tree, hf_h248_oneStream);
}
static int dissect_indAudStreamParms_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_IndAudStreamParms(TRUE, tvb, offset, actx, tree, hf_h248_indAudStreamParms);
}


static const ber_sequence_t IndAudStreamDescriptor_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_streamID_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_indAudStreamParms_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_IndAudStreamDescriptor(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   IndAudStreamDescriptor_sequence, hf_index, ett_h248_IndAudStreamDescriptor);

  return offset;
}
static int dissect_multiStream_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_IndAudStreamDescriptor(FALSE, tvb, offset, actx, tree, hf_h248_multiStream_item);
}


static const ber_sequence_t SEQUENCE_OF_IndAudStreamDescriptor_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_multiStream_item },
};

static int
dissect_h248_SEQUENCE_OF_IndAudStreamDescriptor(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                      SEQUENCE_OF_IndAudStreamDescriptor_sequence_of, hf_index, ett_h248_SEQUENCE_OF_IndAudStreamDescriptor);

  return offset;
}
static int dissect_multiStream_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SEQUENCE_OF_IndAudStreamDescriptor(TRUE, tvb, offset, actx, tree, hf_h248_multiStream);
}


static const value_string h248_indAudMediaDescriptorStreams_vals[] = {
  {   0, "oneStream" },
  {   1, "multiStream" },
  { 0, NULL }
};

static const ber_choice_t indAudMediaDescriptorStreams_choice[] = {
  {   0, BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_oneStream_impl },
  {   1, BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_multiStream_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_h248_indAudMediaDescriptorStreams(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_choice(actx, tree, tvb, offset,
                                 indAudMediaDescriptorStreams_choice, hf_index, ett_h248_indAudMediaDescriptorStreams,
                                 NULL);

  return offset;
}
static int dissect_indAudMediaDescriptorStreams_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_indAudMediaDescriptorStreams(TRUE, tvb, offset, actx, tree, hf_h248_indAudMediaDescriptorStreams);
}


static const ber_sequence_t IndAudMediaDescriptor_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_indAudTerminationStateDescriptor_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_indAudMediaDescriptorStreams_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_IndAudMediaDescriptor(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   IndAudMediaDescriptor_sequence, hf_index, ett_h248_IndAudMediaDescriptor);

  return offset;
}
static int dissect_indaudmediaDescriptor_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_IndAudMediaDescriptor(TRUE, tvb, offset, actx, tree, hf_h248_indaudmediaDescriptor);
}


static const ber_sequence_t IndAudEventsDescriptor_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_requestID_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_iAEDPkgdName_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_streamID_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_IndAudEventsDescriptor(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   IndAudEventsDescriptor_sequence, hf_index, ett_h248_IndAudEventsDescriptor);

  return offset;
}
static int dissect_indaudeventsDescriptor_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_IndAudEventsDescriptor(TRUE, tvb, offset, actx, tree, hf_h248_indaudeventsDescriptor);
}


static const ber_sequence_t IndAudEventBufferDescriptor_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_iAEBDEventName_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_streamID_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_IndAudEventBufferDescriptor(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   IndAudEventBufferDescriptor_sequence, hf_index, ett_h248_IndAudEventBufferDescriptor);

  return offset;
}
static int dissect_indaudeventBufferDescriptor_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_IndAudEventBufferDescriptor(TRUE, tvb, offset, actx, tree, hf_h248_indaudeventBufferDescriptor);
}


static const ber_sequence_t IndAudSignal_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_iASignalName_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_streamID_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_signalRequestID_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_IndAudSignal(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   IndAudSignal_sequence, hf_index, ett_h248_IndAudSignal);

  return offset;
}
static int dissect_indAudSignal_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_IndAudSignal(TRUE, tvb, offset, actx, tree, hf_h248_indAudSignal);
}
static int dissect_iASignalList_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_IndAudSignal(TRUE, tvb, offset, actx, tree, hf_h248_iASignalList);
}


static const ber_sequence_t IndAudSeqSigList_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_id_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_iASignalList_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_IndAudSeqSigList(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   IndAudSeqSigList_sequence, hf_index, ett_h248_IndAudSeqSigList);

  return offset;
}
static int dissect_indAudSeqSigList_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_IndAudSeqSigList(TRUE, tvb, offset, actx, tree, hf_h248_indAudSeqSigList);
}


static const value_string h248_IndAudSignalsDescriptor_vals[] = {
  {   0, "signal" },
  {   1, "seqSigList" },
  { 0, NULL }
};

static const ber_choice_t IndAudSignalsDescriptor_choice[] = {
  {   0, BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_indAudSignal_impl },
  {   1, BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_indAudSeqSigList_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_h248_IndAudSignalsDescriptor(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_choice(actx, tree, tvb, offset,
                                 IndAudSignalsDescriptor_choice, hf_index, ett_h248_IndAudSignalsDescriptor,
                                 NULL);

  return offset;
}
static int dissect_indaudsignalsDescriptor_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_IndAudSignalsDescriptor(TRUE, tvb, offset, actx, tree, hf_h248_indaudsignalsDescriptor);
}


static const ber_sequence_t IndAudDigitMapDescriptor_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_digitMapName_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_IndAudDigitMapDescriptor(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   IndAudDigitMapDescriptor_sequence, hf_index, ett_h248_IndAudDigitMapDescriptor);

  return offset;
}
static int dissect_indauddigitMapDescriptor_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_IndAudDigitMapDescriptor(TRUE, tvb, offset, actx, tree, hf_h248_indauddigitMapDescriptor);
}


static const ber_sequence_t IndAudPackagesDescriptor_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_packageName_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_packageVersion_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_IndAudPackagesDescriptor(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   IndAudPackagesDescriptor_sequence, hf_index, ett_h248_IndAudPackagesDescriptor);

  return offset;
}
static int dissect_indaudpackagesDescriptor_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_IndAudPackagesDescriptor(TRUE, tvb, offset, actx, tree, hf_h248_indaudpackagesDescriptor);
}


static const value_string h248_IndAuditParameter_vals[] = {
  {   0, "indaudmediaDescriptor" },
  {   1, "indaudeventsDescriptor" },
  {   2, "indaudeventBufferDescriptor" },
  {   3, "indaudsignalsDescriptor" },
  {   4, "indauddigitMapDescriptor" },
  {   5, "indaudstatisticsDescriptor" },
  {   6, "indaudpackagesDescriptor" },
  { 0, NULL }
};

static const ber_choice_t IndAuditParameter_choice[] = {
  {   0, BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_indaudmediaDescriptor_impl },
  {   1, BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_indaudeventsDescriptor_impl },
  {   2, BER_CLASS_CON, 2, BER_FLAGS_IMPLTAG, dissect_indaudeventBufferDescriptor_impl },
  {   3, BER_CLASS_CON, 3, BER_FLAGS_IMPLTAG, dissect_indaudsignalsDescriptor_impl },
  {   4, BER_CLASS_CON, 4, BER_FLAGS_IMPLTAG, dissect_indauddigitMapDescriptor_impl },
  {   5, BER_CLASS_CON, 5, BER_FLAGS_IMPLTAG, dissect_indaudstatisticsDescriptor_impl },
  {   6, BER_CLASS_CON, 6, BER_FLAGS_IMPLTAG, dissect_indaudpackagesDescriptor_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_h248_IndAuditParameter(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_choice(actx, tree, tvb, offset,
                                 IndAuditParameter_choice, hf_index, ett_h248_IndAuditParameter,
                                 NULL);

  return offset;
}
static int dissect_auditPropertyToken_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_IndAuditParameter(FALSE, tvb, offset, actx, tree, hf_h248_auditPropertyToken_item);
}


static const ber_sequence_t SEQUENCE_OF_IndAuditParameter_sequence_of[1] = {
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_auditPropertyToken_item },
};

static int
dissect_h248_SEQUENCE_OF_IndAuditParameter(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                      SEQUENCE_OF_IndAuditParameter_sequence_of, hf_index, ett_h248_SEQUENCE_OF_IndAuditParameter);

  return offset;
}
static int dissect_auditPropertyToken_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SEQUENCE_OF_IndAuditParameter(TRUE, tvb, offset, actx, tree, hf_h248_auditPropertyToken);
}


static const ber_sequence_t AuditDescriptor_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_auditToken_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_auditPropertyToken_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_AuditDescriptor(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   AuditDescriptor_sequence, hf_index, ett_h248_AuditDescriptor);

  return offset;
}
static int dissect_auditDescriptor_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_AuditDescriptor(TRUE, tvb, offset, actx, tree, hf_h248_auditDescriptor);
}
static int dissect_emptyDescriptors_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_AuditDescriptor(TRUE, tvb, offset, actx, tree, hf_h248_emptyDescriptors);
}
static int dissect_serviceChangeInfo_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_AuditDescriptor(TRUE, tvb, offset, actx, tree, hf_h248_serviceChangeInfo);
}


static const value_string h248_AmmDescriptor_vals[] = {
  {   0, "mediaDescriptor" },
  {   1, "modemDescriptor" },
  {   2, "muxDescriptor" },
  {   3, "eventsDescriptor" },
  {   4, "eventBufferDescriptor" },
  {   5, "signalsDescriptor" },
  {   6, "digitMapDescriptor" },
  {   7, "auditDescriptor" },
  {   8, "statisticsDescriptor" },
  {   8, "statisticsDescriptor" },
  { 0, NULL }
};

static const ber_choice_t AmmDescriptor_choice[] = {
  {   0, BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_mediaDescriptor_impl },
  {   1, BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_modemDescriptor_impl },
  {   2, BER_CLASS_CON, 2, BER_FLAGS_IMPLTAG, dissect_muxDescriptor_impl },
  {   3, BER_CLASS_CON, 3, BER_FLAGS_IMPLTAG, dissect_eventsDescriptor_impl },
  {   4, BER_CLASS_CON, 4, BER_FLAGS_IMPLTAG, dissect_eventBufferDescriptor_impl },
  {   5, BER_CLASS_CON, 5, BER_FLAGS_IMPLTAG, dissect_signalsDescriptor_impl },
  {   6, BER_CLASS_CON, 6, BER_FLAGS_IMPLTAG, dissect_digitMapDescriptor_impl },
  {   7, BER_CLASS_CON, 7, BER_FLAGS_IMPLTAG, dissect_auditDescriptor_impl },
  {   8, BER_CLASS_CON, 8, BER_FLAGS_IMPLTAG, dissect_statisticsDescriptor_impl },
  {   8, BER_CLASS_CON, 8, BER_FLAGS_IMPLTAG, dissect_statisticsDescriptor_impl },
  {   8, BER_CLASS_CON, 8, BER_FLAGS_IMPLTAG, dissect_statisticsDescriptor_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_h248_AmmDescriptor(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_choice(actx, tree, tvb, offset,
                                 AmmDescriptor_choice, hf_index, ett_h248_AmmDescriptor,
                                 NULL);

  return offset;
}
static int dissect_descriptors_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_AmmDescriptor(FALSE, tvb, offset, actx, tree, hf_h248_descriptors_item);
}


static const ber_sequence_t SEQUENCE_OF_AmmDescriptor_sequence_of[1] = {
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_descriptors_item },
};

static int
dissect_h248_SEQUENCE_OF_AmmDescriptor(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                      SEQUENCE_OF_AmmDescriptor_sequence_of, hf_index, ett_h248_SEQUENCE_OF_AmmDescriptor);

  return offset;
}
static int dissect_descriptors_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SEQUENCE_OF_AmmDescriptor(TRUE, tvb, offset, actx, tree, hf_h248_descriptors);
}


static const ber_sequence_t AmmRequest_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_terminationIDList_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_descriptors_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_AmmRequest(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   AmmRequest_sequence, hf_index, ett_h248_AmmRequest);

  return offset;
}



static int
dissect_h248_T_addReq(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 114 "h248.cnf"
	  curr_info.cmd = gcp_cmd(curr_info.msg,curr_info.trx,curr_info.ctx,GCP_CMD_ADD_REQ,offset,keep_persistent_data);
      H248_TAP();

  offset = dissect_h248_AmmRequest(implicit_tag, tvb, offset, actx, tree, hf_index);

#line 119 "h248.cnf"
      curr_info.cmd = NULL;

  return offset;
}
static int dissect_addReq_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_T_addReq(TRUE, tvb, offset, actx, tree, hf_h248_addReq);
}



static int
dissect_h248_T_moveReq(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 123 "h248.cnf"
	  curr_info.cmd = gcp_cmd(curr_info.msg,curr_info.trx,curr_info.ctx,GCP_CMD_MOVE_REQ,offset,keep_persistent_data);
      H248_TAP();


  offset = dissect_h248_AmmRequest(implicit_tag, tvb, offset, actx, tree, hf_index);

#line 129 "h248.cnf"
      curr_info.cmd = NULL;

  return offset;
}
static int dissect_moveReq_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_T_moveReq(TRUE, tvb, offset, actx, tree, hf_h248_moveReq);
}



static int
dissect_h248_T_modReq(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 133 "h248.cnf"
	  curr_info.cmd = gcp_cmd(curr_info.msg,curr_info.trx,curr_info.ctx,GCP_CMD_MOD_REQ,offset,keep_persistent_data);
      H248_TAP();

  offset = dissect_h248_AmmRequest(implicit_tag, tvb, offset, actx, tree, hf_index);

#line 137 "h248.cnf"
      curr_info.cmd = NULL;

  return offset;
}
static int dissect_modReq_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_T_modReq(TRUE, tvb, offset, actx, tree, hf_h248_modReq);
}


static const ber_sequence_t SubtractRequest_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_terminationIDList_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_auditDescriptor_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_SubtractRequest(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   SubtractRequest_sequence, hf_index, ett_h248_SubtractRequest);

  return offset;
}



static int
dissect_h248_T_subtractReq(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 141 "h248.cnf"
	  curr_info.cmd = gcp_cmd(curr_info.msg,curr_info.trx,curr_info.ctx,GCP_CMD_SUB_REQ,offset,keep_persistent_data);
      H248_TAP();

  offset = dissect_h248_SubtractRequest(implicit_tag, tvb, offset, actx, tree, hf_index);

#line 145 "h248.cnf"
      curr_info.cmd = NULL;

  return offset;
}
static int dissect_subtractReq_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_T_subtractReq(TRUE, tvb, offset, actx, tree, hf_h248_subtractReq);
}


static const ber_sequence_t AuditRequest_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_terminationID_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_auditDescriptor_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_terminationIDList_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_AuditRequest(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   AuditRequest_sequence, hf_index, ett_h248_AuditRequest);

  return offset;
}



static int
dissect_h248_T_auditCapRequest(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 149 "h248.cnf"
	  curr_info.cmd = gcp_cmd(curr_info.msg,curr_info.trx,curr_info.ctx,GCP_CMD_AUDITCAP_REQ,offset,keep_persistent_data);
      H248_TAP();

  offset = dissect_h248_AuditRequest(implicit_tag, tvb, offset, actx, tree, hf_index);

#line 153 "h248.cnf"
      curr_info.cmd = NULL;

  return offset;
}
static int dissect_auditCapRequest_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_T_auditCapRequest(TRUE, tvb, offset, actx, tree, hf_h248_auditCapRequest);
}



static int
dissect_h248_T_auditValueRequest(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 157 "h248.cnf"
	  curr_info.cmd = gcp_cmd(curr_info.msg,curr_info.trx,curr_info.ctx,GCP_CMD_AUDITVAL_REQ,offset,keep_persistent_data);
      H248_TAP();

  offset = dissect_h248_AuditRequest(implicit_tag, tvb, offset, actx, tree, hf_index);

#line 161 "h248.cnf"
      curr_info.cmd = NULL;

  return offset;
}
static int dissect_auditValueRequest_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_T_auditValueRequest(TRUE, tvb, offset, actx, tree, hf_h248_auditValueRequest);
}


static const ber_sequence_t TimeNotation_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_date_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_time_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_TimeNotation(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   TimeNotation_sequence, hf_index, ett_h248_TimeNotation);

  return offset;
}
static int dissect_timeNotation_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_TimeNotation(TRUE, tvb, offset, actx, tree, hf_h248_timeNotation);
}
static int dissect_timeStamp_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_TimeNotation(TRUE, tvb, offset, actx, tree, hf_h248_timeStamp);
}
static int dissect_timestamp_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_TimeNotation(TRUE, tvb, offset, actx, tree, hf_h248_timestamp);
}


static const ber_sequence_t ObservedEvent_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_eventName_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_streamID_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_IMPLTAG, dissect_eventParList_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_timeNotation_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_ObservedEvent(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   ObservedEvent_sequence, hf_index, ett_h248_ObservedEvent);

  return offset;
}
static int dissect_observedEventLst_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_ObservedEvent(FALSE, tvb, offset, actx, tree, hf_h248_observedEventLst_item);
}


static const ber_sequence_t SEQUENCE_OF_ObservedEvent_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_observedEventLst_item },
};

static int
dissect_h248_SEQUENCE_OF_ObservedEvent(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                      SEQUENCE_OF_ObservedEvent_sequence_of, hf_index, ett_h248_SEQUENCE_OF_ObservedEvent);

  return offset;
}
static int dissect_observedEventLst_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SEQUENCE_OF_ObservedEvent(TRUE, tvb, offset, actx, tree, hf_h248_observedEventLst);
}


static const ber_sequence_t ObservedEventsDescriptor_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_requestId_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_observedEventLst_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_ObservedEventsDescriptor(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   ObservedEventsDescriptor_sequence, hf_index, ett_h248_ObservedEventsDescriptor);

  return offset;
}
static int dissect_observedEventsDescriptor_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_ObservedEventsDescriptor(TRUE, tvb, offset, actx, tree, hf_h248_observedEventsDescriptor);
}


static const ber_sequence_t NotifyRequest_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_terminationIDList_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_observedEventsDescriptor_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_errorDescriptor_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_NotifyRequest(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   NotifyRequest_sequence, hf_index, ett_h248_NotifyRequest);

  return offset;
}



static int
dissect_h248_T_notifyReq(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 165 "h248.cnf"
	  curr_info.cmd = gcp_cmd(curr_info.msg,curr_info.trx,curr_info.ctx,GCP_CMD_NOTIFY_REQ,offset,keep_persistent_data);
      H248_TAP();

  offset = dissect_h248_NotifyRequest(implicit_tag, tvb, offset, actx, tree, hf_index);

#line 169 "h248.cnf"
      curr_info.cmd = NULL;

  return offset;
}
static int dissect_notifyReq_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_T_notifyReq(TRUE, tvb, offset, actx, tree, hf_h248_notifyReq);
}


static const value_string h248_ServiceChangeMethod_vals[] = {
  {   0, "failover" },
  {   1, "forced" },
  {   2, "graceful" },
  {   3, "restart" },
  {   4, "disconnected" },
  {   5, "handOff" },
  { 0, NULL }
};


static int
dissect_h248_ServiceChangeMethod(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx->pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_serviceChangeMethod_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_ServiceChangeMethod(TRUE, tvb, offset, actx, tree, hf_h248_serviceChangeMethod);
}


static const value_string h248_ServiceChangeAddress_vals[] = {
  {   0, "portNumber" },
  {   1, "ip4Address" },
  {   2, "ip6Address" },
  {   3, "domainName" },
  {   4, "deviceName" },
  {   5, "mtpAddress" },
  { 0, NULL }
};

static const ber_choice_t ServiceChangeAddress_choice[] = {
  {   0, BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_portNumber_impl },
  {   1, BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_ip4Address_impl },
  {   2, BER_CLASS_CON, 2, BER_FLAGS_IMPLTAG, dissect_ip6Address_impl },
  {   3, BER_CLASS_CON, 3, BER_FLAGS_IMPLTAG, dissect_domainName_impl },
  {   4, BER_CLASS_CON, 4, BER_FLAGS_IMPLTAG, dissect_deviceName_impl },
  {   5, BER_CLASS_CON, 5, BER_FLAGS_IMPLTAG, dissect_mtpAddress_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_h248_ServiceChangeAddress(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_choice(actx, tree, tvb, offset,
                                 ServiceChangeAddress_choice, hf_index, ett_h248_ServiceChangeAddress,
                                 NULL);

  return offset;
}
static int dissect_serviceChangeAddress_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_ServiceChangeAddress(TRUE, tvb, offset, actx, tree, hf_h248_serviceChangeAddress);
}



static int
dissect_h248_IA5String_SIZE_1_67(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_restricted_string(implicit_tag, BER_UNI_TAG_IA5String,
                                            actx->pinfo, tree, tvb, offset, hf_index,
                                            NULL);

  return offset;
}
static int dissect_profileName_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_IA5String_SIZE_1_67(TRUE, tvb, offset, actx, tree, hf_h248_profileName);
}


static const ber_sequence_t ServiceChangeProfile_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_profileName_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_ServiceChangeProfile(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   ServiceChangeProfile_sequence, hf_index, ett_h248_ServiceChangeProfile);

  return offset;
}
static int dissect_serviceChangeProfile_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_ServiceChangeProfile(TRUE, tvb, offset, actx, tree, hf_h248_serviceChangeProfile);
}



static int
dissect_h248_SCreasonValueOctetStr(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 293 "h248.cnf"

 tvbuff_t	*parameter_tvb;
   offset = dissect_ber_octet_string(implicit_tag, actx->pinfo, tree, tvb, offset, hf_index,
                                       &parameter_tvb);


 if (!parameter_tvb)
	return offset;

 dissect_h248_ServiceChangeReasonStr(FALSE, parameter_tvb, 0, actx, tree, hf_h248_serviceChangeReasonStr);



  return offset;
}
static int dissect_SCreasonValue_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SCreasonValueOctetStr(FALSE, tvb, offset, actx, tree, hf_h248_SCreasonValue_item);
}


static const ber_sequence_t SCreasonValue_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_SCreasonValue_item },
};

static int
dissect_h248_SCreasonValue(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                      SCreasonValue_sequence_of, hf_index, ett_h248_SCreasonValue);

  return offset;
}
static int dissect_serviceChangeReason_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SCreasonValue(TRUE, tvb, offset, actx, tree, hf_h248_serviceChangeReason);
}



static int
dissect_h248_INTEGER_0_4294967295(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx->pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_serviceChangeDelay_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_INTEGER_0_4294967295(TRUE, tvb, offset, actx, tree, hf_h248_serviceChangeDelay);
}


static const ber_sequence_t ServiceChangeParm_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_serviceChangeMethod_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_serviceChangeAddress_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_serviceChangeVersion_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_serviceChangeProfile_impl },
  { BER_CLASS_CON, 4, BER_FLAGS_IMPLTAG, dissect_serviceChangeReason_impl },
  { BER_CLASS_CON, 5, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_serviceChangeDelay_impl },
  { BER_CLASS_CON, 6, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_serviceChangeMgcId_impl },
  { BER_CLASS_CON, 7, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_timeStamp_impl },
  { BER_CLASS_CON, 8, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_nonStandardData_impl },
  { BER_CLASS_CON, 9, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_serviceChangeInfo_impl },
  { BER_CLASS_CON, 10, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_serviceChangeIncompleteFlag_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_ServiceChangeParm(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   ServiceChangeParm_sequence, hf_index, ett_h248_ServiceChangeParm);

  return offset;
}
static int dissect_serviceChangeParms_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_ServiceChangeParm(TRUE, tvb, offset, actx, tree, hf_h248_serviceChangeParms);
}


static const ber_sequence_t ServiceChangeRequest_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_terminationIDList_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_serviceChangeParms_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_ServiceChangeRequest(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 173 "h248.cnf"
      curr_info.cmd = gcp_cmd(curr_info.msg,curr_info.trx,curr_info.ctx,GCP_CMD_SVCCHG_REQ,offset,keep_persistent_data);
      H248_TAP();

  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   ServiceChangeRequest_sequence, hf_index, ett_h248_ServiceChangeRequest);

#line 177 "h248.cnf"
      curr_info.cmd = NULL;

  return offset;
}
static int dissect_serviceChangeReq_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_ServiceChangeRequest(TRUE, tvb, offset, actx, tree, hf_h248_serviceChangeReq);
}


static const value_string h248_Command_vals[] = {
  {   0, "addReq" },
  {   1, "moveReq" },
  {   2, "modReq" },
  {   3, "subtractReq" },
  {   4, "auditCapRequest" },
  {   5, "auditValueRequest" },
  {   6, "notifyReq" },
  {   7, "serviceChangeReq" },
  { 0, NULL }
};

static const ber_choice_t Command_choice[] = {
  {   0, BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_addReq_impl },
  {   1, BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_moveReq_impl },
  {   2, BER_CLASS_CON, 2, BER_FLAGS_IMPLTAG, dissect_modReq_impl },
  {   3, BER_CLASS_CON, 3, BER_FLAGS_IMPLTAG, dissect_subtractReq_impl },
  {   4, BER_CLASS_CON, 4, BER_FLAGS_IMPLTAG, dissect_auditCapRequest_impl },
  {   5, BER_CLASS_CON, 5, BER_FLAGS_IMPLTAG, dissect_auditValueRequest_impl },
  {   6, BER_CLASS_CON, 6, BER_FLAGS_IMPLTAG, dissect_notifyReq_impl },
  {   7, BER_CLASS_CON, 7, BER_FLAGS_IMPLTAG, dissect_serviceChangeReq_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_h248_Command(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_choice(actx, tree, tvb, offset,
                                 Command_choice, hf_index, ett_h248_Command,
                                 NULL);

  return offset;
}
static int dissect_command_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_Command(TRUE, tvb, offset, actx, tree, hf_h248_command);
}


static const ber_sequence_t CommandRequest_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_command_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_optional_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_wildcardReturn_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_CommandRequest(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   CommandRequest_sequence, hf_index, ett_h248_CommandRequest);

  return offset;
}
static int dissect_commandRequests_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_CommandRequest(FALSE, tvb, offset, actx, tree, hf_h248_commandRequests_item);
}


static const ber_sequence_t SEQUENCE_OF_CommandRequest_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_commandRequests_item },
};

static int
dissect_h248_SEQUENCE_OF_CommandRequest(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                      SEQUENCE_OF_CommandRequest_sequence_of, hf_index, ett_h248_SEQUENCE_OF_CommandRequest);

  return offset;
}
static int dissect_commandRequests_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SEQUENCE_OF_CommandRequest(TRUE, tvb, offset, actx, tree, hf_h248_commandRequests);
}


static const ber_sequence_t ActionRequest_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_contextId_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_contextRequest_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_contextAttrAuditReq_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_IMPLTAG, dissect_commandRequests_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_ActionRequest(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   ActionRequest_sequence, hf_index, ett_h248_ActionRequest);

  return offset;
}
static int dissect_actions_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_ActionRequest(FALSE, tvb, offset, actx, tree, hf_h248_actions_item);
}


static const ber_sequence_t SEQUENCE_OF_ActionRequest_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_actions_item },
};

static int
dissect_h248_SEQUENCE_OF_ActionRequest(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                      SEQUENCE_OF_ActionRequest_sequence_of, hf_index, ett_h248_SEQUENCE_OF_ActionRequest);

  return offset;
}
static int dissect_actions_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SEQUENCE_OF_ActionRequest(TRUE, tvb, offset, actx, tree, hf_h248_actions);
}


static const ber_sequence_t TransactionRequest_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_transactionId_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_actions_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_TransactionRequest(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   TransactionRequest_sequence, hf_index, ett_h248_TransactionRequest);

  return offset;
}
static int dissect_transactionRequest_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_TransactionRequest(TRUE, tvb, offset, actx, tree, hf_h248_transactionRequest);
}


static const ber_sequence_t TransactionPending_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_transactionId_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_TransactionPending(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   TransactionPending_sequence, hf_index, ett_h248_TransactionPending);

  return offset;
}
static int dissect_transactionPending_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_TransactionPending(TRUE, tvb, offset, actx, tree, hf_h248_transactionPending);
}


static const ber_sequence_t PackagesItem_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_packageName_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_packageVersion_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_PackagesItem(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   PackagesItem_sequence, hf_index, ett_h248_PackagesItem);

  return offset;
}
static int dissect_PackagesDescriptor_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_PackagesItem(FALSE, tvb, offset, actx, tree, hf_h248_PackagesDescriptor_item);
}


static const ber_sequence_t PackagesDescriptor_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_PackagesDescriptor_item },
};

static int
dissect_h248_PackagesDescriptor(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                      PackagesDescriptor_sequence_of, hf_index, ett_h248_PackagesDescriptor);

  return offset;
}
static int dissect_packagesDescriptor_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_PackagesDescriptor(TRUE, tvb, offset, actx, tree, hf_h248_packagesDescriptor);
}


static const value_string h248_AuditReturnParameter_vals[] = {
  {   0, "errorDescriptor" },
  {   1, "mediaDescriptor" },
  {   2, "modemDescriptor" },
  {   3, "muxDescriptor" },
  {   4, "eventsDescriptor" },
  {   5, "eventBufferDescriptor" },
  {   6, "signalsDescriptor" },
  {   7, "digitMapDescriptor" },
  {   8, "observedEventsDescriptor" },
  {   9, "statisticsDescriptor" },
  {  10, "packagesDescriptor" },
  {  11, "emptyDescriptors" },
  { 0, NULL }
};

static const ber_choice_t AuditReturnParameter_choice[] = {
  {   0, BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_errorDescriptor_impl },
  {   1, BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_mediaDescriptor_impl },
  {   2, BER_CLASS_CON, 2, BER_FLAGS_IMPLTAG, dissect_modemDescriptor_impl },
  {   3, BER_CLASS_CON, 3, BER_FLAGS_IMPLTAG, dissect_muxDescriptor_impl },
  {   4, BER_CLASS_CON, 4, BER_FLAGS_IMPLTAG, dissect_eventsDescriptor_impl },
  {   5, BER_CLASS_CON, 5, BER_FLAGS_IMPLTAG, dissect_eventBufferDescriptor_impl },
  {   6, BER_CLASS_CON, 6, BER_FLAGS_IMPLTAG, dissect_signalsDescriptor_impl },
  {   7, BER_CLASS_CON, 7, BER_FLAGS_IMPLTAG, dissect_digitMapDescriptor_impl },
  {   8, BER_CLASS_CON, 8, BER_FLAGS_IMPLTAG, dissect_observedEventsDescriptor_impl },
  {   9, BER_CLASS_CON, 9, BER_FLAGS_IMPLTAG, dissect_statisticsDescriptor_impl },
  {  10, BER_CLASS_CON, 10, BER_FLAGS_IMPLTAG, dissect_packagesDescriptor_impl },
  {  11, BER_CLASS_CON, 11, BER_FLAGS_IMPLTAG, dissect_emptyDescriptors_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_h248_AuditReturnParameter(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_choice(actx, tree, tvb, offset,
                                 AuditReturnParameter_choice, hf_index, ett_h248_AuditReturnParameter,
                                 NULL);

  return offset;
}
static int dissect_TerminationAudit_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_AuditReturnParameter(FALSE, tvb, offset, actx, tree, hf_h248_TerminationAudit_item);
}


static const ber_sequence_t TerminationAudit_sequence_of[1] = {
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_TerminationAudit_item },
};

static int
dissect_h248_TerminationAudit(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                      TerminationAudit_sequence_of, hf_index, ett_h248_TerminationAudit);

  return offset;
}
static int dissect_terminationAudit_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_TerminationAudit(TRUE, tvb, offset, actx, tree, hf_h248_terminationAudit);
}
static int dissect_terminationAuditResult_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_TerminationAudit(TRUE, tvb, offset, actx, tree, hf_h248_terminationAuditResult);
}


static const ber_sequence_t AmmsReply_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_terminationIDList_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_terminationAudit_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_AmmsReply(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   AmmsReply_sequence, hf_index, ett_h248_AmmsReply);

  return offset;
}



static int
dissect_h248_T_addReply(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 197 "h248.cnf"
      curr_info.cmd = gcp_cmd(curr_info.msg,curr_info.trx,curr_info.ctx,GCP_CMD_ADD_REPLY,offset,keep_persistent_data);
      H248_TAP();

  offset = dissect_h248_AmmsReply(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_addReply_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_T_addReply(TRUE, tvb, offset, actx, tree, hf_h248_addReply);
}



static int
dissect_h248_T_moveReply(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 202 "h248.cnf"
      curr_info.cmd = gcp_cmd(curr_info.msg,curr_info.trx,curr_info.ctx,GCP_CMD_MOVE_REPLY,offset,keep_persistent_data);
      H248_TAP();

  offset = dissect_h248_AmmsReply(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_moveReply_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_T_moveReply(TRUE, tvb, offset, actx, tree, hf_h248_moveReply);
}



static int
dissect_h248_T_modReply(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 207 "h248.cnf"
      curr_info.cmd = gcp_cmd(curr_info.msg,curr_info.trx,curr_info.ctx,GCP_CMD_MOD_REPLY,offset,keep_persistent_data);
      H248_TAP();

  offset = dissect_h248_AmmsReply(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_modReply_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_T_modReply(TRUE, tvb, offset, actx, tree, hf_h248_modReply);
}



static int
dissect_h248_T_subtractReply(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 212 "h248.cnf"
      curr_info.cmd = gcp_cmd(curr_info.msg,curr_info.trx,curr_info.ctx,GCP_CMD_SUB_REPLY,offset,keep_persistent_data);
      H248_TAP();

  offset = dissect_h248_AmmsReply(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_subtractReply_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_T_subtractReply(TRUE, tvb, offset, actx, tree, hf_h248_subtractReply);
}


static const ber_sequence_t AuditResult_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_terminationID_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_terminationAuditResult_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_AuditResult(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   AuditResult_sequence, hf_index, ett_h248_AuditResult);

  return offset;
}
static int dissect_auditResult_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_AuditResult(TRUE, tvb, offset, actx, tree, hf_h248_auditResult);
}


static const ber_sequence_t TermListAuditResult_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_terminationIDList_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_terminationAuditResult_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_TermListAuditResult(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   TermListAuditResult_sequence, hf_index, ett_h248_TermListAuditResult);

  return offset;
}
static int dissect_auditResultTermList_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_TermListAuditResult(TRUE, tvb, offset, actx, tree, hf_h248_auditResultTermList);
}


static const value_string h248_AuditReply_vals[] = {
  {   0, "contextAuditResult" },
  {   1, "error" },
  {   2, "auditResult" },
  {   3, "auditResultTermList" },
  {   3, "auditResultTermList" },
  { 0, NULL }
};

static const ber_choice_t AuditReply_choice[] = {
  {   0, BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_contextAuditResult_impl },
  {   1, BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_error_impl },
  {   2, BER_CLASS_CON, 2, BER_FLAGS_IMPLTAG, dissect_auditResult_impl },
  {   3, BER_CLASS_CON, 3, BER_FLAGS_IMPLTAG, dissect_auditResultTermList_impl },
  {   3, BER_CLASS_CON, 3, BER_FLAGS_IMPLTAG, dissect_auditResultTermList_impl },
  {   3, BER_CLASS_CON, 3, BER_FLAGS_IMPLTAG, dissect_auditResultTermList_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_h248_AuditReply(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_choice(actx, tree, tvb, offset,
                                 AuditReply_choice, hf_index, ett_h248_AuditReply,
                                 NULL);

  return offset;
}



static int
dissect_h248_T_auditCapReply(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 227 "h248.cnf"
      curr_info.cmd = gcp_cmd(curr_info.msg,curr_info.trx,curr_info.ctx,GCP_CMD_AUDITCAP_REPLY,offset,keep_persistent_data);
      H248_TAP();

  offset = dissect_h248_AuditReply(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_auditCapReply_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_T_auditCapReply(TRUE, tvb, offset, actx, tree, hf_h248_auditCapReply);
}



static int
dissect_h248_T_auditValueReply(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 232 "h248.cnf"
      curr_info.cmd = gcp_cmd(curr_info.msg,curr_info.trx,curr_info.ctx,GCP_CMD_AUDITVAL_REPLY,offset,keep_persistent_data);
      H248_TAP();

  offset = dissect_h248_AuditReply(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_auditValueReply_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_T_auditValueReply(TRUE, tvb, offset, actx, tree, hf_h248_auditValueReply);
}


static const ber_sequence_t NotifyReply_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_terminationIDList_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_errorDescriptor_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_NotifyReply(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   NotifyReply_sequence, hf_index, ett_h248_NotifyReply);

  return offset;
}



static int
dissect_h248_T_notifyReply(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 217 "h248.cnf"
      curr_info.cmd = gcp_cmd(curr_info.msg,curr_info.trx,curr_info.ctx,GCP_CMD_NOTIFY_REPLY,offset,keep_persistent_data);
      H248_TAP();

  offset = dissect_h248_NotifyReply(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_notifyReply_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_T_notifyReply(TRUE, tvb, offset, actx, tree, hf_h248_notifyReply);
}


static const ber_sequence_t ServiceChangeResParm_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_serviceChangeMgcId_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_serviceChangeAddress_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_serviceChangeVersion_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_serviceChangeProfile_impl },
  { BER_CLASS_CON, 4, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_timestamp_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_ServiceChangeResParm(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   ServiceChangeResParm_sequence, hf_index, ett_h248_ServiceChangeResParm);

  return offset;
}
static int dissect_serviceChangeResParms_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_ServiceChangeResParm(TRUE, tvb, offset, actx, tree, hf_h248_serviceChangeResParms);
}


static const value_string h248_ServiceChangeResult_vals[] = {
  {   0, "errorDescriptor" },
  {   1, "serviceChangeResParms" },
  { 0, NULL }
};

static const ber_choice_t ServiceChangeResult_choice[] = {
  {   0, BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_errorDescriptor_impl },
  {   1, BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_serviceChangeResParms_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_h248_ServiceChangeResult(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_choice(actx, tree, tvb, offset,
                                 ServiceChangeResult_choice, hf_index, ett_h248_ServiceChangeResult,
                                 NULL);

  return offset;
}
static int dissect_serviceChangeResult_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_ServiceChangeResult(TRUE, tvb, offset, actx, tree, hf_h248_serviceChangeResult);
}


static const ber_sequence_t ServiceChangeReply_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_terminationIDList_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_serviceChangeResult_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_ServiceChangeReply(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 222 "h248.cnf"
      curr_info.cmd = gcp_cmd(curr_info.msg,curr_info.trx,curr_info.ctx,GCP_CMD_SVCCHG_REPLY,offset,keep_persistent_data);
      H248_TAP();

  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   ServiceChangeReply_sequence, hf_index, ett_h248_ServiceChangeReply);

  return offset;
}
static int dissect_serviceChangeReply_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_ServiceChangeReply(TRUE, tvb, offset, actx, tree, hf_h248_serviceChangeReply);
}


static const value_string h248_CommandReply_vals[] = {
  {   0, "addReply" },
  {   1, "moveReply" },
  {   2, "modReply" },
  {   3, "subtractReply" },
  {   4, "auditCapReply" },
  {   5, "auditValueReply" },
  {   6, "notifyReply" },
  {   7, "serviceChangeReply" },
  { 0, NULL }
};

static const ber_choice_t CommandReply_choice[] = {
  {   0, BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_addReply_impl },
  {   1, BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_moveReply_impl },
  {   2, BER_CLASS_CON, 2, BER_FLAGS_IMPLTAG, dissect_modReply_impl },
  {   3, BER_CLASS_CON, 3, BER_FLAGS_IMPLTAG, dissect_subtractReply_impl },
  {   4, BER_CLASS_CON, 4, BER_FLAGS_IMPLTAG, dissect_auditCapReply_impl },
  {   5, BER_CLASS_CON, 5, BER_FLAGS_IMPLTAG, dissect_auditValueReply_impl },
  {   6, BER_CLASS_CON, 6, BER_FLAGS_IMPLTAG, dissect_notifyReply_impl },
  {   7, BER_CLASS_CON, 7, BER_FLAGS_IMPLTAG, dissect_serviceChangeReply_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_h248_CommandReply(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_choice(actx, tree, tvb, offset,
                                 CommandReply_choice, hf_index, ett_h248_CommandReply,
                                 NULL);

  return offset;
}
static int dissect_commandReply_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_CommandReply(FALSE, tvb, offset, actx, tree, hf_h248_commandReply_item);
}


static const ber_sequence_t SEQUENCE_OF_CommandReply_sequence_of[1] = {
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_commandReply_item },
};

static int
dissect_h248_SEQUENCE_OF_CommandReply(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                      SEQUENCE_OF_CommandReply_sequence_of, hf_index, ett_h248_SEQUENCE_OF_CommandReply);

  return offset;
}
static int dissect_commandReply_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SEQUENCE_OF_CommandReply(TRUE, tvb, offset, actx, tree, hf_h248_commandReply);
}


static const ber_sequence_t ActionReply_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_contextId_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_errorDescriptor_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_contextReply_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_IMPLTAG, dissect_commandReply_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_ActionReply(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   ActionReply_sequence, hf_index, ett_h248_ActionReply);

#line 107 "h248.cnf"
    if (!curr_info.cmd) {
	  curr_info.cmd = gcp_cmd(curr_info.msg,curr_info.trx,curr_info.ctx,GCP_CMD_REPLY,offset,keep_persistent_data);
      H248_TAP();
	}

  return offset;
}
static int dissect_actionReplies_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_ActionReply(FALSE, tvb, offset, actx, tree, hf_h248_actionReplies_item);
}


static const ber_sequence_t SEQUENCE_OF_ActionReply_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_actionReplies_item },
};

static int
dissect_h248_SEQUENCE_OF_ActionReply(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                      SEQUENCE_OF_ActionReply_sequence_of, hf_index, ett_h248_SEQUENCE_OF_ActionReply);

  return offset;
}
static int dissect_actionReplies_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SEQUENCE_OF_ActionReply(TRUE, tvb, offset, actx, tree, hf_h248_actionReplies);
}


static const value_string h248_T_transactionResult_vals[] = {
  {   0, "transactionError" },
  {   1, "actionReplies" },
  { 0, NULL }
};

static const ber_choice_t T_transactionResult_choice[] = {
  {   0, BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_transactionError_impl },
  {   1, BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_actionReplies_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_h248_T_transactionResult(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_choice(actx, tree, tvb, offset,
                                 T_transactionResult_choice, hf_index, ett_h248_T_transactionResult,
                                 NULL);

  return offset;
}
static int dissect_transactionResult_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_T_transactionResult(TRUE, tvb, offset, actx, tree, hf_h248_transactionResult);
}



static int
dissect_h248_SegmentNumber(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx->pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_segmentNumber_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SegmentNumber(TRUE, tvb, offset, actx, tree, hf_h248_segmentNumber);
}


static const ber_sequence_t TransactionReply_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_transactionId_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_immAckRequired_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_IMPLTAG, dissect_transactionResult_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_segmentNumber_impl },
  { BER_CLASS_CON, 4, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_segmentationComplete_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_TransactionReply(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   TransactionReply_sequence, hf_index, ett_h248_TransactionReply);

  return offset;
}
static int dissect_transactionReply_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_TransactionReply(TRUE, tvb, offset, actx, tree, hf_h248_transactionReply);
}


static const ber_sequence_t TransactionAck_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_firstAck_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_lastAck_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_TransactionAck(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   TransactionAck_sequence, hf_index, ett_h248_TransactionAck);

  return offset;
}
static int dissect_TransactionResponseAck_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_TransactionAck(FALSE, tvb, offset, actx, tree, hf_h248_TransactionResponseAck_item);
}


static const ber_sequence_t TransactionResponseAck_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_TransactionResponseAck_item },
};

static int
dissect_h248_TransactionResponseAck(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                      TransactionResponseAck_sequence_of, hf_index, ett_h248_TransactionResponseAck);

  return offset;
}
static int dissect_transactionResponseAck_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_TransactionResponseAck(TRUE, tvb, offset, actx, tree, hf_h248_transactionResponseAck);
}


static const ber_sequence_t SegmentReply_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_transactionId1_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_segmentNumber_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_segmentationComplete_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_SegmentReply(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   SegmentReply_sequence, hf_index, ett_h248_SegmentReply);

  return offset;
}
static int dissect_segmentReply_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SegmentReply(TRUE, tvb, offset, actx, tree, hf_h248_segmentReply);
}


static const value_string h248_Transaction_vals[] = {
  {   0, "transactionRequest" },
  {   1, "transactionPending" },
  {   2, "transactionReply" },
  {   3, "transactionResponseAck" },
  {   4, "segmentReply" },
  {   4, "segmentReply" },
  { 0, NULL }
};

static const ber_choice_t Transaction_choice[] = {
  {   0, BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_transactionRequest_impl },
  {   1, BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_transactionPending_impl },
  {   2, BER_CLASS_CON, 2, BER_FLAGS_IMPLTAG, dissect_transactionReply_impl },
  {   3, BER_CLASS_CON, 3, BER_FLAGS_IMPLTAG, dissect_transactionResponseAck_impl },
  {   4, BER_CLASS_CON, 4, BER_FLAGS_IMPLTAG, dissect_segmentReply_impl },
  {   4, BER_CLASS_CON, 4, BER_FLAGS_IMPLTAG, dissect_segmentReply_impl },
  {   4, BER_CLASS_CON, 4, BER_FLAGS_IMPLTAG, dissect_segmentReply_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_h248_Transaction(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_choice(actx, tree, tvb, offset,
                                 Transaction_choice, hf_index, ett_h248_Transaction,
                                 NULL);

  return offset;
}
static int dissect_transactions_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_Transaction(FALSE, tvb, offset, actx, tree, hf_h248_transactions_item);
}


static const ber_sequence_t SEQUENCE_OF_Transaction_sequence_of[1] = {
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_transactions_item },
};

static int
dissect_h248_SEQUENCE_OF_Transaction(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                      SEQUENCE_OF_Transaction_sequence_of, hf_index, ett_h248_SEQUENCE_OF_Transaction);

  return offset;
}
static int dissect_transactions_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_SEQUENCE_OF_Transaction(TRUE, tvb, offset, actx, tree, hf_h248_transactions);
}


static const value_string h248_T_messageBody_vals[] = {
  {   0, "messageError" },
  {   1, "transactions" },
  { 0, NULL }
};

static const ber_choice_t T_messageBody_choice[] = {
  {   0, BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_messageError_impl },
  {   1, BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_transactions_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_h248_T_messageBody(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_choice(actx, tree, tvb, offset,
                                 T_messageBody_choice, hf_index, ett_h248_T_messageBody,
                                 NULL);

  return offset;
}
static int dissect_messageBody_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_T_messageBody(TRUE, tvb, offset, actx, tree, hf_h248_messageBody);
}


static const ber_sequence_t Message_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_version_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_mId_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_IMPLTAG, dissect_messageBody_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_Message(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 80 "h248.cnf"
    curr_info.msg = gcp_msg(actx->pinfo,TVB_RAW_OFFSET(tvb),keep_persistent_data);

  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   Message_sequence, hf_index, ett_h248_Message);

#line 84 "h248.cnf"
    if (check_col(actx->pinfo->cinfo, COL_INFO))
        col_set_str(actx->pinfo->cinfo, COL_INFO, gcp_msg_to_str(curr_info.msg,keep_persistent_data));
        
    if (keep_persistent_data)
        gcp_analyze_msg(h248_tree, h248_tvb, curr_info.msg, &h248_arrel);

  return offset;
}
static int dissect_mess_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_h248_Message(TRUE, tvb, offset, actx, tree, hf_h248_mess);
}


static const ber_sequence_t MegacoMessage_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_authHeader_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_mess_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_h248_MegacoMessage(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   MegacoMessage_sequence, hf_index, ett_h248_MegacoMessage);

  return offset;
}



static int
dissect_h248_ServiceChangeReasonStr(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_restricted_string(implicit_tag, BER_UNI_TAG_IA5String,
                                            actx->pinfo, tree, tvb, offset, hf_index,
                                            NULL);

  return offset;
}


/*--- End of included file: packet-h248-fn.c ---*/
#line 1143 "packet-h248-template.c"

static void dissect_h248_tpkt(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree) {
	dissect_tpkt_encap(tvb, pinfo, tree, h248_desegment, h248_handle);
}

static void
dissect_h248(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
    proto_item *h248_item;
	asn1_ctx_t asn1_ctx;
    h248_tree = NULL;
    h248_tvb = NULL;

	asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);

    curr_info.msg = NULL;
    curr_info.trx = NULL;
    curr_info.ctx = NULL;
    curr_info.cmd = NULL;
    curr_info.term = NULL;
    curr_info.pkg = NULL;
    curr_info.evt = NULL;
    curr_info.sig = NULL;
    curr_info.stat = NULL;
    curr_info.par = NULL;

    /* Check if it is actually a text-based H.248 encoding, which we
       dissect with the "megaco" dissector in Wireshark.  (Both
       encodings are MEGACO (RFC 3015) and both are H.248.)
     */
    if(tvb_length(tvb)>=6){
        if(!tvb_strneql(tvb, 0, "MEGACO", 6)){
            static dissector_handle_t megaco_handle=NULL;
            if(!megaco_handle){
                megaco_handle = find_dissector("megaco");
            }
            if(megaco_handle){
                call_dissector(megaco_handle, tvb, pinfo, tree);
                return;
            }
        }
    }

    /* Make entry in the Protocol column on summary display */
    if (check_col(pinfo->cinfo, COL_PROTOCOL))
        col_set_str(pinfo->cinfo, COL_PROTOCOL, "H.248");

    if (tree) {
        h248_item = proto_tree_add_item(tree, proto_h248, tvb, 0, -1, FALSE);
        h248_tree = proto_item_add_subtree(h248_item, ett_h248);
    }

    dissect_h248_MegacoMessage(FALSE, tvb, 0, &asn1_ctx, h248_tree, -1);

}


static void h248_init(void)  {

    if (!h248_prefs_initialized) {
		h248_prefs_initialized = TRUE;
    } else {
        if ( udp_port )
            dissector_delete("udp.port", udp_port, h248_handle);
		
		if ( tcp_port)
            dissector_delete("tcp.port", tcp_port, h248_tpkt_handle);
	}

    udp_port = temp_udp_port;
    tcp_port = temp_tcp_port;
	
    if ( udp_port ) {
		dissector_add("udp.port", udp_port, h248_handle);
	}

	if ( tcp_port) {
		dissector_add("tcp.port", tcp_port, h248_tpkt_handle);
	}
	
	if (!h248_term_handle){
		h248_term_handle = find_dissector("h248term");
	}

}

/*--- proto_register_h248 ----------------------------------------------*/
void proto_register_h248(void) {

  /* List of fields */
  static hf_register_info hf[] = {
    { &hf_h248_mtpaddress_ni, {
      "NI", "h248.mtpaddress.ni", FT_UINT32, BASE_DEC,
      NULL, 0, "NI", HFILL }},
    { &hf_h248_mtpaddress_pc, {
      "PC", "h248.mtpaddress.pc", FT_UINT32, BASE_DEC,
      NULL, 0, "PC", HFILL }},
    { &hf_h248_pkg_name, {
      "Package", "h248.package_name", FT_UINT16, BASE_HEX,
      VALS(package_name_vals), 0, "Package", HFILL }},
    { &hf_248_pkg_param, {
      "Parameter ID", "h248.package_paramid", FT_UINT16, BASE_HEX,
      NULL, 0, "Parameter ID", HFILL }},
    { &hf_h248_signal_code, {
      "Signal ID", "h248.package_signalid", FT_UINT16, BASE_HEX,
      NULL, 0, "Parameter ID", HFILL }},
    { &hf_h248_event_code, {
      "Event ID", "h248.package_eventid", FT_UINT16, BASE_HEX,
      NULL, 0, "Parameter ID", HFILL }},
    { &hf_h248_event_name, {
      "Package and Event name", "h248.event_name", FT_UINT32, BASE_HEX,
      VALS(event_name_vals), 0, "Package", HFILL }},
  { &hf_h248_signal_name, {
      "Package and Signal name", "h248.signal_name", FT_UINT32, BASE_HEX,
      VALS(signal_name_vals), 0, "Package", HFILL }},
	{ &hf_h248_pkg_bcp_BNCChar_PDU,
      { "BNCChar", "h248.package_bcp.BNCChar",
        FT_UINT32, BASE_DEC, VALS(gcp_term_types), 0,
        "BNCChar", HFILL }},

  { &hf_h248_error_code,
  { "errorCode", "h248.errorCode",
      FT_UINT32, BASE_DEC, VALS(h248_reasons), 0,
      "ErrorDescriptor/errorCode", HFILL }},
  { &hf_h248_context_id,
  { "contextId", "h248.contextId",
      FT_UINT32, BASE_HEX, NULL, 0,
      "Context ID", HFILL }},
  { &hf_h248_term_wild_type,
  { "Wildcard Mode", "h248.term.wildcard.mode",
      FT_UINT8, BASE_DEC, VALS(wildcard_modes), 0x80,
      "", HFILL }},
  { &hf_h248_term_wild_level,
  { "Wildcarding Level", "h248.term.wildcard.level",
      FT_UINT8, BASE_DEC, VALS(wildcard_levels), 0x40,
      "", HFILL }},
  { &hf_h248_term_wild_position,
  { "Wildcarding Position", "h248.term.wildcard.pos",
      FT_UINT8, BASE_DEC, NULL, 0x3F,
      "", HFILL }},

  { &hf_h248_no_pkg,
  { "Unknown Package", "h248.pkg.unknown",
	  FT_BYTES, BASE_HEX, NULL, 0,
	  "", HFILL }},
  { &hf_h248_no_sig,
  { "Unknown Signal", "h248.pkg.unknown.sig",
	  FT_BYTES, BASE_HEX, NULL, 0,
	  "", HFILL }},
  { &hf_h248_no_evt,
  { "Unknown Event", "h248.pkg.unknown.evt",
	  FT_BYTES, BASE_HEX, NULL, 0,
	  "", HFILL }},
  { &hf_h248_param,
  { "Parameter", "h248.pkg.unknown.param",
	  FT_BYTES, BASE_HEX, NULL, 0,
	  "", HFILL }},
  { &hf_h248_serviceChangeReasonStr,
      { "ServiceChangeReasonStr", "h248.serviceChangeReasonstr",
        FT_STRING, BASE_NONE, NULL, 0,
        "h248.IA5String", HFILL }},


/*--- Included file: packet-h248-hfarr.c ---*/
#line 1 "packet-h248-hfarr.c"
    { &hf_h248_authHeader,
      { "authHeader", "h248.authHeader",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.AuthenticationHeader", HFILL }},
    { &hf_h248_mess,
      { "mess", "h248.mess",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.Message", HFILL }},
    { &hf_h248_secParmIndex,
      { "secParmIndex", "h248.secParmIndex",
        FT_BYTES, BASE_HEX, NULL, 0,
        "h248.SecurityParmIndex", HFILL }},
    { &hf_h248_seqNum,
      { "seqNum", "h248.seqNum",
        FT_BYTES, BASE_HEX, NULL, 0,
        "h248.SequenceNum", HFILL }},
    { &hf_h248_ad,
      { "ad", "h248.ad",
        FT_BYTES, BASE_HEX, NULL, 0,
        "h248.AuthData", HFILL }},
    { &hf_h248_version,
      { "version", "h248.version",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.INTEGER_0_99", HFILL }},
    { &hf_h248_mId,
      { "mId", "h248.mId",
        FT_UINT32, BASE_DEC, VALS(h248_MId_vals), 0,
        "h248.MId", HFILL }},
    { &hf_h248_messageBody,
      { "messageBody", "h248.messageBody",
        FT_UINT32, BASE_DEC, VALS(h248_T_messageBody_vals), 0,
        "h248.T_messageBody", HFILL }},
    { &hf_h248_messageError,
      { "messageError", "h248.messageError",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.ErrorDescriptor", HFILL }},
    { &hf_h248_transactions,
      { "transactions", "h248.transactions",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.SEQUENCE_OF_Transaction", HFILL }},
    { &hf_h248_transactions_item,
      { "Item", "h248.transactions_item",
        FT_UINT32, BASE_DEC, VALS(h248_Transaction_vals), 0,
        "h248.Transaction", HFILL }},
    { &hf_h248_ip4Address,
      { "ip4Address", "h248.ip4Address",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.IP4Address", HFILL }},
    { &hf_h248_ip6Address,
      { "ip6Address", "h248.ip6Address",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.IP6Address", HFILL }},
    { &hf_h248_domainName,
      { "domainName", "h248.domainName",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.DomainName", HFILL }},
    { &hf_h248_deviceName,
      { "deviceName", "h248.deviceName",
        FT_STRING, BASE_NONE, NULL, 0,
        "h248.PathName", HFILL }},
    { &hf_h248_mtpAddress,
      { "mtpAddress", "h248.mtpAddress",
        FT_BYTES, BASE_HEX, NULL, 0,
        "h248.MtpAddress", HFILL }},
    { &hf_h248_domName,
      { "name", "h248.name",
        FT_STRING, BASE_NONE, NULL, 0,
        "h248.IA5String", HFILL }},
    { &hf_h248_portNumber,
      { "portNumber", "h248.portNumber",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.INTEGER_0_65535", HFILL }},
    { &hf_h248_iP4Address,
      { "address", "h248.address",
        FT_IPv4, BASE_NONE, NULL, 0,
        "h248.OCTET_STRING_SIZE_4", HFILL }},
    { &hf_h248_iP6Address,
      { "address", "h248.address",
        FT_IPv6, BASE_NONE, NULL, 0,
        "h248.OCTET_STRING_SIZE_16", HFILL }},
    { &hf_h248_transactionRequest,
      { "transactionRequest", "h248.transactionRequest",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.TransactionRequest", HFILL }},
    { &hf_h248_transactionPending,
      { "transactionPending", "h248.transactionPending",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.TransactionPending", HFILL }},
    { &hf_h248_transactionReply,
      { "transactionReply", "h248.transactionReply",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.TransactionReply", HFILL }},
    { &hf_h248_transactionResponseAck,
      { "transactionResponseAck", "h248.transactionResponseAck",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.TransactionResponseAck", HFILL }},
    { &hf_h248_segmentReply,
      { "segmentReply", "h248.segmentReply",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.SegmentReply", HFILL }},
    { &hf_h248_transactionId,
      { "transactionId", "h248.transactionId",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.transactionId", HFILL }},
    { &hf_h248_actions,
      { "actions", "h248.actions",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.SEQUENCE_OF_ActionRequest", HFILL }},
    { &hf_h248_actions_item,
      { "Item", "h248.actions_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.ActionRequest", HFILL }},
    { &hf_h248_immAckRequired,
      { "immAckRequired", "h248.immAckRequired",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.NULL", HFILL }},
    { &hf_h248_transactionResult,
      { "transactionResult", "h248.transactionResult",
        FT_UINT32, BASE_DEC, VALS(h248_T_transactionResult_vals), 0,
        "h248.T_transactionResult", HFILL }},
    { &hf_h248_transactionError,
      { "transactionError", "h248.transactionError",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.ErrorDescriptor", HFILL }},
    { &hf_h248_actionReplies,
      { "actionReplies", "h248.actionReplies",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.SEQUENCE_OF_ActionReply", HFILL }},
    { &hf_h248_actionReplies_item,
      { "Item", "h248.actionReplies_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.ActionReply", HFILL }},
    { &hf_h248_segmentNumber,
      { "segmentNumber", "h248.segmentNumber",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.SegmentNumber", HFILL }},
    { &hf_h248_segmentationComplete,
      { "segmentationComplete", "h248.segmentationComplete",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.NULL", HFILL }},
    { &hf_h248_transactionId1,
      { "transactionId", "h248.transactionId",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.TransactionId", HFILL }},
    { &hf_h248_TransactionResponseAck_item,
      { "Item", "h248.TransactionResponseAck_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.TransactionAck", HFILL }},
    { &hf_h248_firstAck,
      { "firstAck", "h248.firstAck",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.TransactionId", HFILL }},
    { &hf_h248_lastAck,
      { "lastAck", "h248.lastAck",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.TransactionId", HFILL }},
    { &hf_h248_errorCode,
      { "errorCode", "h248.errorCode",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.T_errorCode", HFILL }},
    { &hf_h248_errorText,
      { "errorText", "h248.errorText",
        FT_STRING, BASE_NONE, NULL, 0,
        "h248.ErrorText", HFILL }},
    { &hf_h248_contextId,
      { "contextId", "h248.contextId",
        FT_UINT32, BASE_HEX, NULL, 0,
        "h248.contextId", HFILL }},
    { &hf_h248_contextRequest,
      { "contextRequest", "h248.contextRequest",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.ContextRequest", HFILL }},
    { &hf_h248_contextAttrAuditReq,
      { "contextAttrAuditReq", "h248.contextAttrAuditReq",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.T_contextAttrAuditReq", HFILL }},
    { &hf_h248_commandRequests,
      { "commandRequests", "h248.commandRequests",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.SEQUENCE_OF_CommandRequest", HFILL }},
    { &hf_h248_commandRequests_item,
      { "Item", "h248.commandRequests_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.CommandRequest", HFILL }},
    { &hf_h248_errorDescriptor,
      { "errorDescriptor", "h248.errorDescriptor",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.ErrorDescriptor", HFILL }},
    { &hf_h248_contextReply,
      { "contextReply", "h248.contextReply",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.ContextRequest", HFILL }},
    { &hf_h248_commandReply,
      { "commandReply", "h248.commandReply",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.SEQUENCE_OF_CommandReply", HFILL }},
    { &hf_h248_commandReply_item,
      { "Item", "h248.commandReply_item",
        FT_UINT32, BASE_DEC, VALS(h248_CommandReply_vals), 0,
        "h248.CommandReply", HFILL }},
    { &hf_h248_priority,
      { "priority", "h248.priority",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.INTEGER_0_15", HFILL }},
    { &hf_h248_emergency,
      { "emergency", "h248.emergency",
        FT_BOOLEAN, 8, NULL, 0,
        "h248.BOOLEAN", HFILL }},
    { &hf_h248_topologyReq,
      { "topologyReq", "h248.topologyReq",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.T_topologyReq", HFILL }},
    { &hf_h248_topologyReq_item,
      { "Item", "h248.topologyReq_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.TopologyRequest", HFILL }},
    { &hf_h248_iepscallind,
      { "iepscallind", "h248.iepscallind",
        FT_BOOLEAN, 8, NULL, 0,
        "h248.BOOLEAN", HFILL }},
    { &hf_h248_contextProp,
      { "contextProp", "h248.contextProp",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.SEQUENCE_OF_PropertyParm", HFILL }},
    { &hf_h248_contextProp_item,
      { "Item", "h248.contextProp_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.PropertyParm", HFILL }},
    { &hf_h248_contextList,
      { "contextList", "h248.contextList",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.SEQUENCE_OF_ContextIDinList", HFILL }},
    { &hf_h248_contextList_item,
      { "Item", "h248.contextList_item",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.ContextIDinList", HFILL }},
    { &hf_h248_topology,
      { "topology", "h248.topology",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.NULL", HFILL }},
    { &hf_h248_cAAREmergency,
      { "emergency", "h248.emergency",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.NULL", HFILL }},
    { &hf_h248_cAARPriority,
      { "priority", "h248.priority",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.NULL", HFILL }},
    { &hf_h248_iepscallind1,
      { "iepscallind", "h248.iepscallind",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.NULL", HFILL }},
    { &hf_h248_contextPropAud,
      { "contextPropAud", "h248.contextPropAud",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.SEQUENCE_OF_IndAudPropertyParm", HFILL }},
    { &hf_h248_contextPropAud_item,
      { "Item", "h248.contextPropAud_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.IndAudPropertyParm", HFILL }},
    { &hf_h248_selectpriority,
      { "selectpriority", "h248.selectpriority",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.INTEGER_0_15", HFILL }},
    { &hf_h248_selectemergency,
      { "selectemergency", "h248.selectemergency",
        FT_BOOLEAN, 8, NULL, 0,
        "h248.BOOLEAN", HFILL }},
    { &hf_h248_selectiepscallind,
      { "selectiepscallind", "h248.selectiepscallind",
        FT_BOOLEAN, 8, NULL, 0,
        "h248.BOOLEAN", HFILL }},
    { &hf_h248_selectLogic,
      { "selectLogic", "h248.selectLogic",
        FT_UINT32, BASE_DEC, VALS(h248_SelectLogic_vals), 0,
        "h248.SelectLogic", HFILL }},
    { &hf_h248_andAUDITSelect,
      { "andAUDITSelect", "h248.andAUDITSelect",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.NULL", HFILL }},
    { &hf_h248_orAUDITSelect,
      { "orAUDITSelect", "h248.orAUDITSelect",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.NULL", HFILL }},
    { &hf_h248_command,
      { "command", "h248.command",
        FT_UINT32, BASE_DEC, VALS(h248_Command_vals), 0,
        "h248.Command", HFILL }},
    { &hf_h248_optional,
      { "optional", "h248.optional",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.NULL", HFILL }},
    { &hf_h248_wildcardReturn,
      { "wildcardReturn", "h248.wildcardReturn",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.NULL", HFILL }},
    { &hf_h248_addReq,
      { "addReq", "h248.addReq",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.T_addReq", HFILL }},
    { &hf_h248_moveReq,
      { "moveReq", "h248.moveReq",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.T_moveReq", HFILL }},
    { &hf_h248_modReq,
      { "modReq", "h248.modReq",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.T_modReq", HFILL }},
    { &hf_h248_subtractReq,
      { "subtractReq", "h248.subtractReq",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.T_subtractReq", HFILL }},
    { &hf_h248_auditCapRequest,
      { "auditCapRequest", "h248.auditCapRequest",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.T_auditCapRequest", HFILL }},
    { &hf_h248_auditValueRequest,
      { "auditValueRequest", "h248.auditValueRequest",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.T_auditValueRequest", HFILL }},
    { &hf_h248_notifyReq,
      { "notifyReq", "h248.notifyReq",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.T_notifyReq", HFILL }},
    { &hf_h248_serviceChangeReq,
      { "serviceChangeReq", "h248.serviceChangeReq",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.ServiceChangeRequest", HFILL }},
    { &hf_h248_addReply,
      { "addReply", "h248.addReply",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.T_addReply", HFILL }},
    { &hf_h248_moveReply,
      { "moveReply", "h248.moveReply",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.T_moveReply", HFILL }},
    { &hf_h248_modReply,
      { "modReply", "h248.modReply",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.T_modReply", HFILL }},
    { &hf_h248_subtractReply,
      { "subtractReply", "h248.subtractReply",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.T_subtractReply", HFILL }},
    { &hf_h248_auditCapReply,
      { "auditCapReply", "h248.auditCapReply",
        FT_UINT32, BASE_DEC, VALS(h248_AuditReply_vals), 0,
        "h248.T_auditCapReply", HFILL }},
    { &hf_h248_auditValueReply,
      { "auditValueReply", "h248.auditValueReply",
        FT_UINT32, BASE_DEC, VALS(h248_AuditReply_vals), 0,
        "h248.T_auditValueReply", HFILL }},
    { &hf_h248_notifyReply,
      { "notifyReply", "h248.notifyReply",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.T_notifyReply", HFILL }},
    { &hf_h248_serviceChangeReply,
      { "serviceChangeReply", "h248.serviceChangeReply",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.ServiceChangeReply", HFILL }},
    { &hf_h248_terminationFrom,
      { "terminationFrom", "h248.terminationFrom",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.TerminationID", HFILL }},
    { &hf_h248_terminationTo,
      { "terminationTo", "h248.terminationTo",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.TerminationID", HFILL }},
    { &hf_h248_topologyDirection,
      { "topologyDirection", "h248.topologyDirection",
        FT_UINT32, BASE_DEC, VALS(h248_T_topologyDirection_vals), 0,
        "h248.T_topologyDirection", HFILL }},
    { &hf_h248_streamID,
      { "streamID", "h248.streamID",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.StreamID", HFILL }},
    { &hf_h248_topologyDirectionExtension,
      { "topologyDirectionExtension", "h248.topologyDirectionExtension",
        FT_UINT32, BASE_DEC, VALS(h248_T_topologyDirectionExtension_vals), 0,
        "h248.T_topologyDirectionExtension", HFILL }},
    { &hf_h248_terminationIDList,
      { "terminationID", "h248.terminationID",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.TerminationIDList", HFILL }},
    { &hf_h248_descriptors,
      { "descriptors", "h248.descriptors",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.SEQUENCE_OF_AmmDescriptor", HFILL }},
    { &hf_h248_descriptors_item,
      { "Item", "h248.descriptors_item",
        FT_UINT32, BASE_DEC, VALS(h248_AmmDescriptor_vals), 0,
        "h248.AmmDescriptor", HFILL }},
    { &hf_h248_mediaDescriptor,
      { "mediaDescriptor", "h248.mediaDescriptor",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.MediaDescriptor", HFILL }},
    { &hf_h248_modemDescriptor,
      { "modemDescriptor", "h248.modemDescriptor",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.ModemDescriptor", HFILL }},
    { &hf_h248_muxDescriptor,
      { "muxDescriptor", "h248.muxDescriptor",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.MuxDescriptor", HFILL }},
    { &hf_h248_eventsDescriptor,
      { "eventsDescriptor", "h248.eventsDescriptor",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.EventsDescriptor", HFILL }},
    { &hf_h248_eventBufferDescriptor,
      { "eventBufferDescriptor", "h248.eventBufferDescriptor",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.EventBufferDescriptor", HFILL }},
    { &hf_h248_signalsDescriptor,
      { "signalsDescriptor", "h248.signalsDescriptor",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.SignalsDescriptor", HFILL }},
    { &hf_h248_digitMapDescriptor,
      { "digitMapDescriptor", "h248.digitMapDescriptor",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.DigitMapDescriptor", HFILL }},
    { &hf_h248_auditDescriptor,
      { "auditDescriptor", "h248.auditDescriptor",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.AuditDescriptor", HFILL }},
    { &hf_h248_statisticsDescriptor,
      { "statisticsDescriptor", "h248.statisticsDescriptor",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.StatisticsDescriptor", HFILL }},
    { &hf_h248_terminationAudit,
      { "terminationAudit", "h248.terminationAudit",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.TerminationAudit", HFILL }},
    { &hf_h248_terminationID,
      { "terminationID", "h248.terminationID",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.TerminationID", HFILL }},
    { &hf_h248_contextAuditResult,
      { "contextAuditResult", "h248.contextAuditResult",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.TerminationIDList", HFILL }},
    { &hf_h248_error,
      { "error", "h248.error",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.ErrorDescriptor", HFILL }},
    { &hf_h248_auditResult,
      { "auditResult", "h248.auditResult",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.AuditResult", HFILL }},
    { &hf_h248_auditResultTermList,
      { "auditResultTermList", "h248.auditResultTermList",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.TermListAuditResult", HFILL }},
    { &hf_h248_terminationAuditResult,
      { "terminationAuditResult", "h248.terminationAuditResult",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.TerminationAudit", HFILL }},
    { &hf_h248_TerminationAudit_item,
      { "Item", "h248.TerminationAudit_item",
        FT_UINT32, BASE_DEC, VALS(h248_AuditReturnParameter_vals), 0,
        "h248.AuditReturnParameter", HFILL }},
    { &hf_h248_observedEventsDescriptor,
      { "observedEventsDescriptor", "h248.observedEventsDescriptor",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.ObservedEventsDescriptor", HFILL }},
    { &hf_h248_packagesDescriptor,
      { "packagesDescriptor", "h248.packagesDescriptor",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.PackagesDescriptor", HFILL }},
    { &hf_h248_emptyDescriptors,
      { "emptyDescriptors", "h248.emptyDescriptors",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.AuditDescriptor", HFILL }},
    { &hf_h248_auditToken,
      { "auditToken", "h248.auditToken",
        FT_BYTES, BASE_HEX, NULL, 0,
        "h248.T_auditToken", HFILL }},
    { &hf_h248_auditPropertyToken,
      { "auditPropertyToken", "h248.auditPropertyToken",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.SEQUENCE_OF_IndAuditParameter", HFILL }},
    { &hf_h248_auditPropertyToken_item,
      { "Item", "h248.auditPropertyToken_item",
        FT_UINT32, BASE_DEC, VALS(h248_IndAuditParameter_vals), 0,
        "h248.IndAuditParameter", HFILL }},
    { &hf_h248_indaudmediaDescriptor,
      { "indaudmediaDescriptor", "h248.indaudmediaDescriptor",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.IndAudMediaDescriptor", HFILL }},
    { &hf_h248_indaudeventsDescriptor,
      { "indaudeventsDescriptor", "h248.indaudeventsDescriptor",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.IndAudEventsDescriptor", HFILL }},
    { &hf_h248_indaudeventBufferDescriptor,
      { "indaudeventBufferDescriptor", "h248.indaudeventBufferDescriptor",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.IndAudEventBufferDescriptor", HFILL }},
    { &hf_h248_indaudsignalsDescriptor,
      { "indaudsignalsDescriptor", "h248.indaudsignalsDescriptor",
        FT_UINT32, BASE_DEC, VALS(h248_IndAudSignalsDescriptor_vals), 0,
        "h248.IndAudSignalsDescriptor", HFILL }},
    { &hf_h248_indauddigitMapDescriptor,
      { "indauddigitMapDescriptor", "h248.indauddigitMapDescriptor",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.IndAudDigitMapDescriptor", HFILL }},
    { &hf_h248_indaudstatisticsDescriptor,
      { "indaudstatisticsDescriptor", "h248.indaudstatisticsDescriptor",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.IndAudStatisticsDescriptor", HFILL }},
    { &hf_h248_indaudpackagesDescriptor,
      { "indaudpackagesDescriptor", "h248.indaudpackagesDescriptor",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.IndAudPackagesDescriptor", HFILL }},
    { &hf_h248_indAudTerminationStateDescriptor,
      { "termStateDescr", "h248.termStateDescr",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.IndAudTerminationStateDescriptor", HFILL }},
    { &hf_h248_indAudMediaDescriptorStreams,
      { "streams", "h248.streams",
        FT_UINT32, BASE_DEC, VALS(h248_indAudMediaDescriptorStreams_vals), 0,
        "h248.indAudMediaDescriptorStreams", HFILL }},
    { &hf_h248_oneStream,
      { "oneStream", "h248.oneStream",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.IndAudStreamParms", HFILL }},
    { &hf_h248_multiStream,
      { "multiStream", "h248.multiStream",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.SEQUENCE_OF_IndAudStreamDescriptor", HFILL }},
    { &hf_h248_multiStream_item,
      { "Item", "h248.multiStream_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.IndAudStreamDescriptor", HFILL }},
    { &hf_h248_indAudStreamParms,
      { "streamParms", "h248.streamParms",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.IndAudStreamParms", HFILL }},
    { &hf_h248_iASPLocalControlDescriptor,
      { "localControlDescriptor", "h248.localControlDescriptor",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.IndAudLocalControlDescriptor", HFILL }},
    { &hf_h248_iASPLocalDescriptor,
      { "localDescriptor", "h248.localDescriptor",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.IndAudLocalRemoteDescriptor", HFILL }},
    { &hf_h248_iASPRemoteDescriptor,
      { "remoteDescriptor", "h248.remoteDescriptor",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.IndAudLocalRemoteDescriptor", HFILL }},
    { &hf_h248_statisticsDescriptor1,
      { "statisticsDescriptor", "h248.statisticsDescriptor",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.IndAudStatisticsDescriptor", HFILL }},
    { &hf_h248_iALCDStreamMode,
      { "streamMode", "h248.streamMode",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.NULL", HFILL }},
    { &hf_h248_iALCDReserveValue,
      { "reserveValue", "h248.reserveValue",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.NULL", HFILL }},
    { &hf_h248_iALCDReserveGroup,
      { "reserveGroup", "h248.reserveGroup",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.NULL", HFILL }},
    { &hf_h248_indAudPropertyParms,
      { "propertyParms", "h248.propertyParms",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.SEQUENCE_OF_IndAudPropertyParm", HFILL }},
    { &hf_h248_indAudPropertyParms_item,
      { "Item", "h248.propertyParms_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.IndAudPropertyParm", HFILL }},
    { &hf_h248_streamModeSel,
      { "streamModeSel", "h248.streamModeSel",
        FT_UINT32, BASE_DEC, VALS(h248_StreamMode_vals), 0,
        "h248.StreamMode", HFILL }},
    { &hf_h248_name,
      { "name", "h248.name",
        FT_BYTES, BASE_HEX, NULL, 0,
        "h248.PkgdName", HFILL }},
    { &hf_h248_propertyParms,
      { "propertyParms", "h248.propertyParms",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.PropertyParm", HFILL }},
    { &hf_h248_propGroupID,
      { "propGroupID", "h248.propGroupID",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.INTEGER_0_65535", HFILL }},
    { &hf_h248_iAPropertyGroup,
      { "propGrps", "h248.propGrps",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.IndAudPropertyGroup", HFILL }},
    { &hf_h248_IndAudPropertyGroup_item,
      { "Item", "h248.IndAudPropertyGroup_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.IndAudPropertyParm", HFILL }},
    { &hf_h248_eventBufferControl,
      { "eventBufferControl", "h248.eventBufferControl",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.NULL", HFILL }},
    { &hf_h248_iATSDServiceState,
      { "serviceState", "h248.serviceState",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.NULL", HFILL }},
    { &hf_h248_serviceStateSel,
      { "serviceStateSel", "h248.serviceStateSel",
        FT_UINT32, BASE_DEC, VALS(h248_ServiceState_vals), 0,
        "h248.ServiceState", HFILL }},
    { &hf_h248_requestID,
      { "requestID", "h248.requestID",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.RequestID", HFILL }},
    { &hf_h248_iAEDPkgdName,
      { "pkgdName", "h248.pkgdName",
        FT_BYTES, BASE_HEX, NULL, 0,
        "h248.PkgdName", HFILL }},
    { &hf_h248_iAEBDEventName,
      { "eventName", "h248.eventName",
        FT_BYTES, BASE_HEX, NULL, 0,
        "h248.PkgdName", HFILL }},
    { &hf_h248_indAudSignal,
      { "signal", "h248.signal",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.IndAudSignal", HFILL }},
    { &hf_h248_indAudSeqSigList,
      { "seqSigList", "h248.seqSigList",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.IndAudSeqSigList", HFILL }},
    { &hf_h248_id,
      { "id", "h248.id",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.INTEGER_0_65535", HFILL }},
    { &hf_h248_iASignalList,
      { "signalList", "h248.signalList",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.IndAudSignal", HFILL }},
    { &hf_h248_iASignalName,
      { "signalName", "h248.signalName",
        FT_BYTES, BASE_HEX, NULL, 0,
        "h248.PkgdName", HFILL }},
    { &hf_h248_signalRequestID,
      { "signalRequestID", "h248.signalRequestID",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.RequestID", HFILL }},
    { &hf_h248_digitMapName,
      { "digitMapName", "h248.digitMapName",
        FT_BYTES, BASE_HEX, NULL, 0,
        "h248.DigitMapName", HFILL }},
    { &hf_h248_iAStatName,
      { "statName", "h248.statName",
        FT_BYTES, BASE_HEX, NULL, 0,
        "h248.PkgdName", HFILL }},
    { &hf_h248_packageName,
      { "packageName", "h248.packageName",
        FT_BYTES, BASE_HEX, NULL, 0,
        "h248.Name", HFILL }},
    { &hf_h248_packageVersion,
      { "packageVersion", "h248.packageVersion",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.INTEGER_0_99", HFILL }},
    { &hf_h248_requestId,
      { "requestId", "h248.requestId",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.RequestID", HFILL }},
    { &hf_h248_observedEventLst,
      { "observedEventLst", "h248.observedEventLst",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.SEQUENCE_OF_ObservedEvent", HFILL }},
    { &hf_h248_observedEventLst_item,
      { "Item", "h248.observedEventLst_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.ObservedEvent", HFILL }},
    { &hf_h248_eventName,
      { "eventName", "h248.eventName",
        FT_BYTES, BASE_HEX, NULL, 0,
        "h248.EventName", HFILL }},
    { &hf_h248_eventParList,
      { "eventParList", "h248.eventParList",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.SEQUENCE_OF_EventParameter", HFILL }},
    { &hf_h248_eventParList_item,
      { "Item", "h248.eventParList_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.EventParameter", HFILL }},
    { &hf_h248_timeNotation,
      { "timeNotation", "h248.timeNotation",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.TimeNotation", HFILL }},
    { &hf_h248_eventParameterName,
      { "eventParameterName", "h248.eventParameterName",
        FT_BYTES, BASE_HEX, NULL, 0,
        "h248.EventParameterName", HFILL }},
    { &hf_h248_eventParamValue,
      { "eventParamValue", "h248.eventParamValue",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.EventParamValues", HFILL }},
    { &hf_h248_extraInfo,
      { "extraInfo", "h248.extraInfo",
        FT_UINT32, BASE_DEC, VALS(h248_T_extraInfo_vals), 0,
        "h248.T_extraInfo", HFILL }},
    { &hf_h248_relation,
      { "relation", "h248.relation",
        FT_UINT32, BASE_DEC, VALS(h248_Relation_vals), 0,
        "h248.Relation", HFILL }},
    { &hf_h248_range,
      { "range", "h248.range",
        FT_BOOLEAN, 8, NULL, 0,
        "h248.BOOLEAN", HFILL }},
    { &hf_h248_sublist,
      { "sublist", "h248.sublist",
        FT_BOOLEAN, 8, NULL, 0,
        "h248.BOOLEAN", HFILL }},
    { &hf_h248_EventParamValues_item,
      { "Item", "h248.EventParamValues_item",
        FT_BYTES, BASE_HEX, NULL, 0,
        "h248.EventParamValue", HFILL }},
    { &hf_h248_serviceChangeParms,
      { "serviceChangeParms", "h248.serviceChangeParms",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.ServiceChangeParm", HFILL }},
    { &hf_h248_serviceChangeResult,
      { "serviceChangeResult", "h248.serviceChangeResult",
        FT_UINT32, BASE_DEC, VALS(h248_ServiceChangeResult_vals), 0,
        "h248.ServiceChangeResult", HFILL }},
    { &hf_h248_serviceChangeResParms,
      { "serviceChangeResParms", "h248.serviceChangeResParms",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.ServiceChangeResParm", HFILL }},
    { &hf_h248_wildcard,
      { "wildcard", "h248.wildcard",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.SEQUENCE_OF_WildcardField", HFILL }},
    { &hf_h248_wildcard_item,
      { "Item", "h248.wildcard_item",
        FT_BYTES, BASE_HEX, NULL, 0,
        "h248.WildcardField", HFILL }},
    { &hf_h248_terminationId,
      { "id", "h248.id",
        FT_BYTES, BASE_HEX, NULL, 0,
        "h248.T_terminationId", HFILL }},
    { &hf_h248_TerminationIDList_item,
      { "Item", "h248.TerminationIDList_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.TerminationID", HFILL }},
    { &hf_h248_termStateDescr,
      { "termStateDescr", "h248.termStateDescr",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.TerminationStateDescriptor", HFILL }},
    { &hf_h248_streams,
      { "streams", "h248.streams",
        FT_UINT32, BASE_DEC, VALS(h248_T_streams_vals), 0,
        "h248.T_streams", HFILL }},
    { &hf_h248_mediaDescriptorOneStream,
      { "oneStream", "h248.oneStream",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.StreamParms", HFILL }},
    { &hf_h248_mediaDescriptorMultiStream,
      { "multiStream", "h248.multiStream",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.SEQUENCE_OF_StreamDescriptor", HFILL }},
    { &hf_h248_mediaDescriptorMultiStream_item,
      { "Item", "h248.multiStream_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.StreamDescriptor", HFILL }},
    { &hf_h248_streamParms,
      { "streamParms", "h248.streamParms",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.StreamParms", HFILL }},
    { &hf_h248_localControlDescriptor,
      { "localControlDescriptor", "h248.localControlDescriptor",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.LocalControlDescriptor", HFILL }},
    { &hf_h248_localDescriptor,
      { "localDescriptor", "h248.localDescriptor",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.LocalRemoteDescriptor", HFILL }},
    { &hf_h248_remoteDescriptor,
      { "remoteDescriptor", "h248.remoteDescriptor",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.LocalRemoteDescriptor", HFILL }},
    { &hf_h248_streamMode,
      { "streamMode", "h248.streamMode",
        FT_UINT32, BASE_DEC, VALS(h248_StreamMode_vals), 0,
        "h248.StreamMode", HFILL }},
    { &hf_h248_reserveValue,
      { "reserveValue", "h248.reserveValue",
        FT_BOOLEAN, 8, NULL, 0,
        "h248.BOOLEAN", HFILL }},
    { &hf_h248_reserveGroup,
      { "reserveGroup", "h248.reserveGroup",
        FT_BOOLEAN, 8, NULL, 0,
        "h248.BOOLEAN", HFILL }},
    { &hf_h248_propertyParms1,
      { "propertyParms", "h248.propertyParms",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.SEQUENCE_OF_PropertyParm", HFILL }},
    { &hf_h248_propertyParms_item,
      { "Item", "h248.propertyParms_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.PropertyParm", HFILL }},
    { &hf_h248_propertyName,
      { "propertyName", "h248.propertyName",
        FT_BYTES, BASE_HEX, NULL, 0,
        "h248.PropertyName", HFILL }},
    { &hf_h248_propertyParamValue,
      { "value", "h248.value",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.SEQUENCE_OF_PropertyID", HFILL }},
    { &hf_h248_propertyParamValue_item,
      { "Item", "h248.value_item",
        FT_BYTES, BASE_HEX, NULL, 0,
        "h248.PropertyID", HFILL }},
    { &hf_h248_extraInfo1,
      { "extraInfo", "h248.extraInfo",
        FT_UINT32, BASE_DEC, VALS(h248_T_extraInfo1_vals), 0,
        "h248.T_extraInfo1", HFILL }},
    { &hf_h248_propGrps,
      { "propGrps", "h248.propGrps",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.SEQUENCE_OF_PropertyGroup", HFILL }},
    { &hf_h248_propGrps_item,
      { "Item", "h248.propGrps_item",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.PropertyGroup", HFILL }},
    { &hf_h248_PropertyGroup_item,
      { "Item", "h248.PropertyGroup_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.PropertyParm", HFILL }},
    { &hf_h248_tSEventBufferControl,
      { "eventBufferControl", "h248.eventBufferControl",
        FT_UINT32, BASE_DEC, VALS(h248_EventBufferControl_vals), 0,
        "h248.EventBufferControl", HFILL }},
    { &hf_h248_serviceState,
      { "serviceState", "h248.serviceState",
        FT_UINT32, BASE_DEC, VALS(h248_ServiceState_vals), 0,
        "h248.ServiceState", HFILL }},
    { &hf_h248_muxType,
      { "muxType", "h248.muxType",
        FT_UINT32, BASE_DEC, VALS(h248_MuxType_vals), 0,
        "h248.MuxType", HFILL }},
    { &hf_h248_termList,
      { "termList", "h248.termList",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.SEQUENCE_OF_TerminationID", HFILL }},
    { &hf_h248_termList_item,
      { "Item", "h248.termList_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.TerminationID", HFILL }},
    { &hf_h248_nonStandardData,
      { "nonStandardData", "h248.nonStandardData",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.NonStandardData", HFILL }},
    { &hf_h248_eventList,
      { "eventList", "h248.eventList",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.SEQUENCE_OF_RequestedEvent", HFILL }},
    { &hf_h248_eventList_item,
      { "Item", "h248.eventList_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.RequestedEvent", HFILL }},
    { &hf_h248_eventAction,
      { "eventAction", "h248.eventAction",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.RequestedActions", HFILL }},
    { &hf_h248_evParList,
      { "evParList", "h248.evParList",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.SEQUENCE_OF_EventParameter", HFILL }},
    { &hf_h248_evParList_item,
      { "Item", "h248.evParList_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.EventParameter", HFILL }},
    { &hf_h248_secondEvent,
      { "secondEvent", "h248.secondEvent",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.SecondEventsDescriptor", HFILL }},
    { &hf_h248_notifyImmediate,
      { "notifyImmediate", "h248.notifyImmediate",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.NULL", HFILL }},
    { &hf_h248_notifyRegulated,
      { "notifyRegulated", "h248.notifyRegulated",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.RegulatedEmbeddedDescriptor", HFILL }},
    { &hf_h248_neverNotify,
      { "neverNotify", "h248.neverNotify",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.NULL", HFILL }},
    { &hf_h248_keepActive,
      { "keepActive", "h248.keepActive",
        FT_BOOLEAN, 8, NULL, 0,
        "h248.BOOLEAN", HFILL }},
    { &hf_h248_eventDM,
      { "eventDM", "h248.eventDM",
        FT_UINT32, BASE_DEC, VALS(h248_EventDM_vals), 0,
        "h248.EventDM", HFILL }},
    { &hf_h248_notifyBehaviour,
      { "notifyBehaviour", "h248.notifyBehaviour",
        FT_UINT32, BASE_DEC, VALS(h248_NotifyBehaviour_vals), 0,
        "h248.NotifyBehaviour", HFILL }},
    { &hf_h248_resetEventsDescriptor,
      { "resetEventsDescriptor", "h248.resetEventsDescriptor",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.NULL", HFILL }},
    { &hf_h248_digitMapValue,
      { "digitMapValue", "h248.digitMapValue",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.DigitMapValue", HFILL }},
    { &hf_h248_secondaryEventList,
      { "eventList", "h248.eventList",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.SEQUENCE_OF_SecondRequestedEvent", HFILL }},
    { &hf_h248_secondaryEventList_item,
      { "Item", "h248.eventList_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.SecondRequestedEvent", HFILL }},
    { &hf_h248_pkgdName,
      { "pkgdName", "h248.pkgdName",
        FT_BYTES, BASE_HEX, NULL, 0,
        "h248.PkgdName", HFILL }},
    { &hf_h248_secondaryEventAction,
      { "eventAction", "h248.eventAction",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.SecondRequestedActions", HFILL }},
    { &hf_h248_EventBufferDescriptor_item,
      { "Item", "h248.EventBufferDescriptor_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.EventSpec", HFILL }},
    { &hf_h248_SignalsDescriptor_item,
      { "Item", "h248.SignalsDescriptor_item",
        FT_UINT32, BASE_DEC, VALS(h248_SignalRequest_vals), 0,
        "h248.SignalRequest", HFILL }},
    { &hf_h248_signal,
      { "signal", "h248.signal",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.Signal", HFILL }},
    { &hf_h248_seqSigList,
      { "seqSigList", "h248.seqSigList",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.SeqSigList", HFILL }},
    { &hf_h248_signalList,
      { "signalList", "h248.signalList",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.SEQUENCE_OF_Signal", HFILL }},
    { &hf_h248_signalList_item,
      { "Item", "h248.signalList_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.Signal", HFILL }},
    { &hf_h248_signalName,
      { "signalName", "h248.signalName",
        FT_BYTES, BASE_HEX, NULL, 0,
        "h248.SignalName", HFILL }},
    { &hf_h248_sigType,
      { "sigType", "h248.sigType",
        FT_UINT32, BASE_DEC, VALS(h248_SignalType_vals), 0,
        "h248.SignalType", HFILL }},
    { &hf_h248_duration,
      { "duration", "h248.duration",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.INTEGER_0_65535", HFILL }},
    { &hf_h248_notifyCompletion,
      { "notifyCompletion", "h248.notifyCompletion",
        FT_BYTES, BASE_HEX, NULL, 0,
        "h248.NotifyCompletion", HFILL }},
    { &hf_h248_sigParList,
      { "sigParList", "h248.sigParList",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.SEQUENCE_OF_SigParameter", HFILL }},
    { &hf_h248_sigParList_item,
      { "Item", "h248.sigParList_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.SigParameter", HFILL }},
    { &hf_h248_direction,
      { "direction", "h248.direction",
        FT_UINT32, BASE_DEC, VALS(h248_SignalDirection_vals), 0,
        "h248.SignalDirection", HFILL }},
    { &hf_h248_intersigDelay,
      { "intersigDelay", "h248.intersigDelay",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.INTEGER_0_65535", HFILL }},
    { &hf_h248_sigParameterName,
      { "sigParameterName", "h248.sigParameterName",
        FT_BYTES, BASE_HEX, NULL, 0,
        "h248.SigParameterName", HFILL }},
    { &hf_h248_value,
      { "value", "h248.value",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.SigParamValues", HFILL }},
    { &hf_h248_extraInfo2,
      { "extraInfo", "h248.extraInfo",
        FT_UINT32, BASE_DEC, VALS(h248_T_extraInfo2_vals), 0,
        "h248.T_extraInfo2", HFILL }},
    { &hf_h248_SigParamValues_item,
      { "Item", "h248.SigParamValues_item",
        FT_BYTES, BASE_HEX, NULL, 0,
        "h248.SigParamValue", HFILL }},
    { &hf_h248_mtl,
      { "mtl", "h248.mtl",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.SEQUENCE_OF_ModemType", HFILL }},
    { &hf_h248_mtl_item,
      { "Item", "h248.mtl_item",
        FT_UINT32, BASE_DEC, VALS(h248_ModemType_vals), 0,
        "h248.ModemType", HFILL }},
    { &hf_h248_mpl,
      { "mpl", "h248.mpl",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.SEQUENCE_OF_PropertyParm", HFILL }},
    { &hf_h248_mpl_item,
      { "Item", "h248.mpl_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.PropertyParm", HFILL }},
    { &hf_h248_startTimer,
      { "startTimer", "h248.startTimer",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.INTEGER_0_99", HFILL }},
    { &hf_h248_shortTimer,
      { "shortTimer", "h248.shortTimer",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.INTEGER_0_99", HFILL }},
    { &hf_h248_longTimer,
      { "longTimer", "h248.longTimer",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.INTEGER_0_99", HFILL }},
    { &hf_h248_digitMapBody,
      { "digitMapBody", "h248.digitMapBody",
        FT_STRING, BASE_NONE, NULL, 0,
        "h248.IA5String", HFILL }},
    { &hf_h248_durationTimer,
      { "durationTimer", "h248.durationTimer",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.INTEGER_0_99", HFILL }},
    { &hf_h248_serviceChangeMethod,
      { "serviceChangeMethod", "h248.serviceChangeMethod",
        FT_UINT32, BASE_DEC, VALS(h248_ServiceChangeMethod_vals), 0,
        "h248.ServiceChangeMethod", HFILL }},
    { &hf_h248_serviceChangeAddress,
      { "serviceChangeAddress", "h248.serviceChangeAddress",
        FT_UINT32, BASE_DEC, VALS(h248_ServiceChangeAddress_vals), 0,
        "h248.ServiceChangeAddress", HFILL }},
    { &hf_h248_serviceChangeVersion,
      { "serviceChangeVersion", "h248.serviceChangeVersion",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.INTEGER_0_99", HFILL }},
    { &hf_h248_serviceChangeProfile,
      { "serviceChangeProfile", "h248.serviceChangeProfile",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.ServiceChangeProfile", HFILL }},
    { &hf_h248_serviceChangeReason,
      { "serviceChangeReason", "h248.serviceChangeReason",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.SCreasonValue", HFILL }},
    { &hf_h248_serviceChangeDelay,
      { "serviceChangeDelay", "h248.serviceChangeDelay",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.INTEGER_0_4294967295", HFILL }},
    { &hf_h248_serviceChangeMgcId,
      { "serviceChangeMgcId", "h248.serviceChangeMgcId",
        FT_UINT32, BASE_DEC, VALS(h248_MId_vals), 0,
        "h248.MId", HFILL }},
    { &hf_h248_timeStamp,
      { "timeStamp", "h248.timeStamp",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.TimeNotation", HFILL }},
    { &hf_h248_serviceChangeInfo,
      { "serviceChangeInfo", "h248.serviceChangeInfo",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.AuditDescriptor", HFILL }},
    { &hf_h248_serviceChangeIncompleteFlag,
      { "serviceChangeIncompleteFlag", "h248.serviceChangeIncompleteFlag",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.NULL", HFILL }},
    { &hf_h248_SCreasonValue_item,
      { "Item", "h248.SCreasonValue_item",
        FT_BYTES, BASE_HEX, NULL, 0,
        "h248.SCreasonValueOctetStr", HFILL }},
    { &hf_h248_timestamp,
      { "timestamp", "h248.timestamp",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.TimeNotation", HFILL }},
    { &hf_h248_profileName,
      { "profileName", "h248.profileName",
        FT_STRING, BASE_NONE, NULL, 0,
        "h248.IA5String_SIZE_1_67", HFILL }},
    { &hf_h248_PackagesDescriptor_item,
      { "Item", "h248.PackagesDescriptor_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.PackagesItem", HFILL }},
    { &hf_h248_StatisticsDescriptor_item,
      { "Item", "h248.StatisticsDescriptor_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.StatisticsParameter", HFILL }},
    { &hf_h248_statName,
      { "statName", "h248.statName",
        FT_BYTES, BASE_HEX, NULL, 0,
        "h248.StatName", HFILL }},
    { &hf_h248_statValue,
      { "statValue", "h248.statValue",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.StatValue", HFILL }},
    { &hf_h248_nonStandardIdentifier,
      { "nonStandardIdentifier", "h248.nonStandardIdentifier",
        FT_UINT32, BASE_DEC, VALS(h248_NonStandardIdentifier_vals), 0,
        "h248.NonStandardIdentifier", HFILL }},
    { &hf_h248_data,
      { "data", "h248.data",
        FT_BYTES, BASE_HEX, NULL, 0,
        "h248.OCTET_STRING", HFILL }},
    { &hf_h248_object,
      { "object", "h248.object",
        FT_OID, BASE_NONE, NULL, 0,
        "h248.OBJECT_IDENTIFIER", HFILL }},
    { &hf_h248_h221NonStandard,
      { "h221NonStandard", "h248.h221NonStandard",
        FT_NONE, BASE_NONE, NULL, 0,
        "h248.H221NonStandard", HFILL }},
    { &hf_h248_experimental,
      { "experimental", "h248.experimental",
        FT_STRING, BASE_NONE, NULL, 0,
        "h248.IA5String_SIZE_8", HFILL }},
    { &hf_h248_t35CountryCode1,
      { "t35CountryCode1", "h248.t35CountryCode1",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.INTEGER_0_255", HFILL }},
    { &hf_h248_t35CountryCode2,
      { "t35CountryCode2", "h248.t35CountryCode2",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.INTEGER_0_255", HFILL }},
    { &hf_h248_t35Extension,
      { "t35Extension", "h248.t35Extension",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.INTEGER_0_255", HFILL }},
    { &hf_h248_manufacturerCode,
      { "manufacturerCode", "h248.manufacturerCode",
        FT_UINT32, BASE_DEC, NULL, 0,
        "h248.INTEGER_0_65535", HFILL }},
    { &hf_h248_date,
      { "date", "h248.date",
        FT_STRING, BASE_NONE, NULL, 0,
        "h248.IA5String_SIZE_8", HFILL }},
    { &hf_h248_time,
      { "time", "h248.time",
        FT_STRING, BASE_NONE, NULL, 0,
        "h248.IA5String_SIZE_8", HFILL }},
    { &hf_h248_Value_item,
      { "Item", "h248.Value_item",
        FT_BYTES, BASE_HEX, NULL, 0,
        "h248.OCTET_STRING", HFILL }},
    { &hf_h248_T_auditToken_muxToken,
      { "muxToken", "h248.muxToken",
        FT_BOOLEAN, 8, NULL, 0x80,
        "", HFILL }},
    { &hf_h248_T_auditToken_modemToken,
      { "modemToken", "h248.modemToken",
        FT_BOOLEAN, 8, NULL, 0x40,
        "", HFILL }},
    { &hf_h248_T_auditToken_mediaToken,
      { "mediaToken", "h248.mediaToken",
        FT_BOOLEAN, 8, NULL, 0x20,
        "", HFILL }},
    { &hf_h248_T_auditToken_eventsToken,
      { "eventsToken", "h248.eventsToken",
        FT_BOOLEAN, 8, NULL, 0x10,
        "", HFILL }},
    { &hf_h248_T_auditToken_signalsToken,
      { "signalsToken", "h248.signalsToken",
        FT_BOOLEAN, 8, NULL, 0x08,
        "", HFILL }},
    { &hf_h248_T_auditToken_digitMapToken,
      { "digitMapToken", "h248.digitMapToken",
        FT_BOOLEAN, 8, NULL, 0x04,
        "", HFILL }},
    { &hf_h248_T_auditToken_statsToken,
      { "statsToken", "h248.statsToken",
        FT_BOOLEAN, 8, NULL, 0x02,
        "", HFILL }},
    { &hf_h248_T_auditToken_observedEventsToken,
      { "observedEventsToken", "h248.observedEventsToken",
        FT_BOOLEAN, 8, NULL, 0x01,
        "", HFILL }},
    { &hf_h248_T_auditToken_packagesToken,
      { "packagesToken", "h248.packagesToken",
        FT_BOOLEAN, 8, NULL, 0x80,
        "", HFILL }},
    { &hf_h248_T_auditToken_eventBufferToken,
      { "eventBufferToken", "h248.eventBufferToken",
        FT_BOOLEAN, 8, NULL, 0x40,
        "", HFILL }},
    { &hf_h248_NotifyCompletion_onTimeOut,
      { "onTimeOut", "h248.onTimeOut",
        FT_BOOLEAN, 8, NULL, 0x80,
        "", HFILL }},
    { &hf_h248_NotifyCompletion_onInterruptByEvent,
      { "onInterruptByEvent", "h248.onInterruptByEvent",
        FT_BOOLEAN, 8, NULL, 0x40,
        "", HFILL }},
    { &hf_h248_NotifyCompletion_onInterruptByNewSignalDescr,
      { "onInterruptByNewSignalDescr", "h248.onInterruptByNewSignalDescr",
        FT_BOOLEAN, 8, NULL, 0x20,
        "", HFILL }},
    { &hf_h248_NotifyCompletion_otherReason,
      { "otherReason", "h248.otherReason",
        FT_BOOLEAN, 8, NULL, 0x10,
        "", HFILL }},
    { &hf_h248_NotifyCompletion_onIteration,
      { "onIteration", "h248.onIteration",
        FT_BOOLEAN, 8, NULL, 0x08,
        "", HFILL }},

/*--- End of included file: packet-h248-hfarr.c ---*/
#line 1306 "packet-h248-template.c"

	GCP_HF_ARR_ELEMS("h248",h248_arrel)

  };

  /* List of subtrees */
  static gint *ett[] = {
    &ett_h248,
    &ett_mtpaddress,
    &ett_packagename,
    &ett_codec,
    &ett_wildcard,
    &ett_h248_no_pkg,
    &ett_h248_no_sig,
    &ett_h248_no_evt,
	GCP_ETT_ARR_ELEMS(h248_arrel),
	  

/*--- Included file: packet-h248-ettarr.c ---*/
#line 1 "packet-h248-ettarr.c"
    &ett_h248_MegacoMessage,
    &ett_h248_AuthenticationHeader,
    &ett_h248_Message,
    &ett_h248_T_messageBody,
    &ett_h248_SEQUENCE_OF_Transaction,
    &ett_h248_MId,
    &ett_h248_DomainName,
    &ett_h248_IP4Address,
    &ett_h248_IP6Address,
    &ett_h248_Transaction,
    &ett_h248_TransactionRequest,
    &ett_h248_SEQUENCE_OF_ActionRequest,
    &ett_h248_TransactionPending,
    &ett_h248_TransactionReply,
    &ett_h248_T_transactionResult,
    &ett_h248_SEQUENCE_OF_ActionReply,
    &ett_h248_SegmentReply,
    &ett_h248_TransactionResponseAck,
    &ett_h248_TransactionAck,
    &ett_h248_ErrorDescriptor,
    &ett_h248_ActionRequest,
    &ett_h248_SEQUENCE_OF_CommandRequest,
    &ett_h248_ActionReply,
    &ett_h248_SEQUENCE_OF_CommandReply,
    &ett_h248_ContextRequest,
    &ett_h248_T_topologyReq,
    &ett_h248_SEQUENCE_OF_PropertyParm,
    &ett_h248_SEQUENCE_OF_ContextIDinList,
    &ett_h248_ContextAttrAuditRequest,
    &ett_h248_SEQUENCE_OF_IndAudPropertyParm,
    &ett_h248_SelectLogic,
    &ett_h248_CommandRequest,
    &ett_h248_Command,
    &ett_h248_CommandReply,
    &ett_h248_TopologyRequest,
    &ett_h248_AmmRequest,
    &ett_h248_SEQUENCE_OF_AmmDescriptor,
    &ett_h248_AmmDescriptor,
    &ett_h248_AmmsReply,
    &ett_h248_SubtractRequest,
    &ett_h248_AuditRequest,
    &ett_h248_AuditReply,
    &ett_h248_AuditResult,
    &ett_h248_TermListAuditResult,
    &ett_h248_TerminationAudit,
    &ett_h248_AuditReturnParameter,
    &ett_h248_AuditDescriptor,
    &ett_h248_T_auditToken,
    &ett_h248_SEQUENCE_OF_IndAuditParameter,
    &ett_h248_IndAuditParameter,
    &ett_h248_IndAudMediaDescriptor,
    &ett_h248_indAudMediaDescriptorStreams,
    &ett_h248_SEQUENCE_OF_IndAudStreamDescriptor,
    &ett_h248_IndAudStreamDescriptor,
    &ett_h248_IndAudStreamParms,
    &ett_h248_IndAudLocalControlDescriptor,
    &ett_h248_IndAudPropertyParm,
    &ett_h248_IndAudLocalRemoteDescriptor,
    &ett_h248_IndAudPropertyGroup,
    &ett_h248_IndAudTerminationStateDescriptor,
    &ett_h248_IndAudEventsDescriptor,
    &ett_h248_IndAudEventBufferDescriptor,
    &ett_h248_IndAudSignalsDescriptor,
    &ett_h248_IndAudSeqSigList,
    &ett_h248_IndAudSignal,
    &ett_h248_IndAudDigitMapDescriptor,
    &ett_h248_IndAudStatisticsDescriptor,
    &ett_h248_IndAudPackagesDescriptor,
    &ett_h248_NotifyRequest,
    &ett_h248_NotifyReply,
    &ett_h248_ObservedEventsDescriptor,
    &ett_h248_SEQUENCE_OF_ObservedEvent,
    &ett_h248_ObservedEvent,
    &ett_h248_SEQUENCE_OF_EventParameter,
    &ett_h248_EventParameter,
    &ett_h248_T_extraInfo,
    &ett_h248_EventParamValues,
    &ett_h248_ServiceChangeRequest,
    &ett_h248_ServiceChangeReply,
    &ett_h248_ServiceChangeResult,
    &ett_h248_TerminationID,
    &ett_h248_SEQUENCE_OF_WildcardField,
    &ett_h248_TerminationIDList,
    &ett_h248_MediaDescriptor,
    &ett_h248_T_streams,
    &ett_h248_SEQUENCE_OF_StreamDescriptor,
    &ett_h248_StreamDescriptor,
    &ett_h248_StreamParms,
    &ett_h248_LocalControlDescriptor,
    &ett_h248_PropertyParm,
    &ett_h248_SEQUENCE_OF_PropertyID,
    &ett_h248_T_extraInfo1,
    &ett_h248_LocalRemoteDescriptor,
    &ett_h248_SEQUENCE_OF_PropertyGroup,
    &ett_h248_PropertyGroup,
    &ett_h248_TerminationStateDescriptor,
    &ett_h248_MuxDescriptor,
    &ett_h248_SEQUENCE_OF_TerminationID,
    &ett_h248_EventsDescriptor,
    &ett_h248_SEQUENCE_OF_RequestedEvent,
    &ett_h248_RequestedEvent,
    &ett_h248_RegulatedEmbeddedDescriptor,
    &ett_h248_NotifyBehaviour,
    &ett_h248_RequestedActions,
    &ett_h248_EventDM,
    &ett_h248_SecondEventsDescriptor,
    &ett_h248_SEQUENCE_OF_SecondRequestedEvent,
    &ett_h248_SecondRequestedEvent,
    &ett_h248_SecondRequestedActions,
    &ett_h248_EventBufferDescriptor,
    &ett_h248_EventSpec,
    &ett_h248_SignalsDescriptor,
    &ett_h248_SignalRequest,
    &ett_h248_SeqSigList,
    &ett_h248_SEQUENCE_OF_Signal,
    &ett_h248_Signal,
    &ett_h248_SEQUENCE_OF_SigParameter,
    &ett_h248_NotifyCompletion,
    &ett_h248_SigParameter,
    &ett_h248_T_extraInfo2,
    &ett_h248_SigParamValues,
    &ett_h248_ModemDescriptor,
    &ett_h248_SEQUENCE_OF_ModemType,
    &ett_h248_DigitMapDescriptor,
    &ett_h248_DigitMapValue,
    &ett_h248_ServiceChangeParm,
    &ett_h248_SCreasonValue,
    &ett_h248_ServiceChangeAddress,
    &ett_h248_ServiceChangeResParm,
    &ett_h248_ServiceChangeProfile,
    &ett_h248_PackagesDescriptor,
    &ett_h248_PackagesItem,
    &ett_h248_StatisticsDescriptor,
    &ett_h248_StatisticsParameter,
    &ett_h248_NonStandardData,
    &ett_h248_NonStandardIdentifier,
    &ett_h248_H221NonStandard,
    &ett_h248_TimeNotation,
    &ett_h248_Value,

/*--- End of included file: packet-h248-ettarr.c ---*/
#line 1324 "packet-h248-template.c"
  };

  module_t *h248_module;


  /* Register protocol */
  proto_h248 = proto_register_protocol(PNAME, PSNAME, PFNAME);
  register_dissector("h248", dissect_h248, proto_h248);
  register_dissector("h248.tpkt", dissect_h248_tpkt, proto_h248);

  /* Register fields and subtrees */
  proto_register_field_array(proto_h248, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));

  h248_module = prefs_register_protocol(proto_h248, h248_init);
  prefs_register_bool_preference(h248_module, "ctx_info",
                                 "Track Context",
                                 "Mantain relationships between transactions and contexts and display an extra tree showing context data",
                                 &keep_persistent_data);
  prefs_register_uint_preference(h248_module, "udp_port",
                                 "UDP port",
                                 "Port to be decoded as h248",
                                 10,
                                 &temp_udp_port);
  prefs_register_uint_preference(h248_module, "tcp_port",
                                 "TCP port",
                                 "Port to be decoded as h248",
                                 10,
                                 &temp_tcp_port);
  prefs_register_bool_preference(h248_module, "desegment",
                                 "Desegment H.248 over TCP",
                                 "Desegment H.248 messages that span more TCP segments",
                                 &h248_desegment);
  
  
  register_init_routine( &h248_init );

  msgs = se_tree_create(EMEM_TREE_TYPE_RED_BLACK, "h248_msgs");
  trxs = se_tree_create(EMEM_TREE_TYPE_RED_BLACK, "h248_trxs");
  ctxs_by_trx = se_tree_create(EMEM_TREE_TYPE_RED_BLACK, "h248_ctxs_by_trx");
  ctxs = se_tree_create(EMEM_TREE_TYPE_RED_BLACK, "h248_ctxs");

  h248_tap = register_tap("h248");

  gcp_init();
}

/*--- proto_reg_handoff_h248 -------------------------------------------*/
void proto_reg_handoff_h248(void) {

  h248_handle = find_dissector("h248");
  h248_tpkt_handle = find_dissector("h248.tpkt");

  dissector_add("mtp3.service_indicator", GATEWAY_CONTROL_PROTOCOL_USER_ID, h248_handle);
  dissector_add("udp.port", udp_port, h248_handle);
  dissector_add("tcp.port", tcp_port, h248_tpkt_handle);
}



