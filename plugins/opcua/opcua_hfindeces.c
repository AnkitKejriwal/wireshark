/******************************************************************************
** $Id$
**
** Copyright (C) 2006-2007 ascolab GmbH. All Rights Reserved.
** Web: http://www.ascolab.com
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
** 
** Project: OpcUa Wireshark Plugin
**
** Description: This file contains protocol field information.
**
** This file was autogenerated on 8.5.2007 18:53:26.
** DON'T MODIFY THIS FILE!
**
******************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <gmodule.h>
#include <epan/packet.h>

int hf_opcua_TestId = -1;
int hf_opcua_Iteration = -1;
int hf_opcua_ServerUris = -1;
int hf_opcua_ProfileUris = -1;
int hf_opcua_ClientCertificate = -1;
int hf_opcua_SecureChannelId = -1;
int hf_opcua_SecurityPolicyUri = -1;
int hf_opcua_ClientNonce = -1;
int hf_opcua_RequestedLifetime = -1;
int hf_opcua_ServerCertificate = -1;
int hf_opcua_ServerNonce = -1;
int hf_opcua_ClientName = -1;
int hf_opcua_RequestedSessionTimeout = -1;
int hf_opcua_SessionId = -1;
int hf_opcua_RevisedSessionTimeout = -1;
int hf_opcua_LocaleIds = -1;
int hf_opcua_CertificateResults = -1;
int hf_opcua_SequenceNumber = -1;
int hf_opcua_Results = -1;
int hf_opcua_MaxResultsToReturn = -1;
int hf_opcua_IncludeSubtypes = -1;
int hf_opcua_NodeClassMask = -1;
int hf_opcua_ContinuationPoint = -1;
int hf_opcua_ReleaseContinuationPoint = -1;
int hf_opcua_RevisedContinuationPoint = -1;
int hf_opcua_MaxDescriptionsToReturn = -1;
int hf_opcua_MaxReferencesToReturn = -1;
int hf_opcua_MaxReferencedNodesToReturn = -1;
int hf_opcua_MaxTime = -1;
int hf_opcua_MaxAge = -1;
int hf_opcua_ReleaseContinuationPoints = -1;
int hf_opcua_SubscriptionId = -1;
int hf_opcua_MonitoredItemIds = -1;
int hf_opcua_TriggeringItemId = -1;
int hf_opcua_LinksToAdd = -1;
int hf_opcua_LinksToRemove = -1;
int hf_opcua_AddResults = -1;
int hf_opcua_RemoveResults = -1;
int hf_opcua_RequestedPublishingInterval = -1;
int hf_opcua_RequestedLifetimeCounter = -1;
int hf_opcua_RequestedMaxKeepAliveCount = -1;
int hf_opcua_PublishingEnabled = -1;
int hf_opcua_Priority = -1;
int hf_opcua_RevisedPublishingInterval = -1;
int hf_opcua_RevisedLifetimeCounter = -1;
int hf_opcua_RevisedMaxKeepAliveCount = -1;
int hf_opcua_SubscriptionIds = -1;
int hf_opcua_AvailableSequenceNumbers = -1;
int hf_opcua_MoreNotifications = -1;
int hf_opcua_RetransmitSequenceNumber = -1;
int hf_opcua_IsInverse = -1;
int hf_opcua_ServerIndex = -1;
int hf_opcua_NodeClass = -1;
int hf_opcua_EventNotifier = -1;
int hf_opcua_IsAbstract = -1;
int hf_opcua_ArraySize = -1;
int hf_opcua_AccessLevel = -1;
int hf_opcua_UserAccessLevel = -1;
int hf_opcua_MinimumSamplingInterval = -1;
int hf_opcua_Historizing = -1;
int hf_opcua_Symmetric = -1;
int hf_opcua_Executable = -1;
int hf_opcua_UserExecutable = -1;
int hf_opcua_ContainsNoLoops = -1;
int hf_opcua_Index = -1;
int hf_opcua_Uri = -1;
int hf_opcua_Name = -1;
int hf_opcua_StatusCode = -1;
int hf_opcua_EventId = -1;
int hf_opcua_SourceName = -1;
int hf_opcua_Time = -1;
int hf_opcua_ReceiveTime = -1;
int hf_opcua_Severity = -1;
int hf_opcua_Digest = -1;
int hf_opcua_SymmetricSignature = -1;
int hf_opcua_SymmetricKeyWrap = -1;
int hf_opcua_SymmetricEncryption = -1;
int hf_opcua_SymmetricKeyLength = -1;
int hf_opcua_AsymmetricSignature = -1;
int hf_opcua_AsymmetricKeyWrap = -1;
int hf_opcua_AsymmetricEncryption = -1;
int hf_opcua_MinimumAsymmetricKeyLength = -1;
int hf_opcua_MaximumAsymmetricKeyLength = -1;
int hf_opcua_DerivedKey = -1;
int hf_opcua_DerivedEncryptionKeyLength = -1;
int hf_opcua_DerivedSignatureKeyLength = -1;
int hf_opcua_IssuerType = -1;
int hf_opcua_IssuerUrl = -1;
int hf_opcua_ServerUri = -1;
int hf_opcua_DiscoveryUrls = -1;
int hf_opcua_EndpointUrl = -1;
int hf_opcua_SupportedProfiles = -1;
int hf_opcua_SendTimeout = -1;
int hf_opcua_OperationTimeout = -1;
int hf_opcua_UseBinaryEncoding = -1;
int hf_opcua_MaxMessageSize = -1;
int hf_opcua_MaxArrayLength = -1;
int hf_opcua_MaxStringLength = -1;
int hf_opcua_UserName = -1;
int hf_opcua_Password = -1;
int hf_opcua_HashAlgorithm = -1;
int hf_opcua_CertificateData = -1;
int hf_opcua_TokenData = -1;
int hf_opcua_ProfileUri = -1;
int hf_opcua_ProfileName = -1;
int hf_opcua_ApplicationUri = -1;
int hf_opcua_ManufacturerName = -1;
int hf_opcua_ApplicationName = -1;
int hf_opcua_SoftwareVersion = -1;
int hf_opcua_BuildNumber = -1;
int hf_opcua_BuildDate = -1;
int hf_opcua_IssuedBy = -1;
int hf_opcua_IssuedDate = -1;
int hf_opcua_ExpirationDate = -1;
int hf_opcua_ApplicationCertificate = -1;
int hf_opcua_IssuerCertificateThumbprint = -1;
int hf_opcua_IssuerSignatureAlgorithm = -1;
int hf_opcua_IssuerSignature = -1;
int hf_opcua_IsForward = -1;
int hf_opcua_TargetServerUri = -1;
int hf_opcua_TargetNodeClass = -1;
int hf_opcua_DeleteTargetReferences = -1;
int hf_opcua_ServerId = -1;
int hf_opcua_ServiceLevel = -1;
int hf_opcua_SamplingRate = -1;
int hf_opcua_SamplingErrorCount = -1;
int hf_opcua_SampledMonitoredItemsCount = -1;
int hf_opcua_MaxSampledMonitoredItemsCount = -1;
int hf_opcua_DisabledMonitoredItemsSamplingCount = -1;
int hf_opcua_ServerViewCount = -1;
int hf_opcua_CurrentSessionCount = -1;
int hf_opcua_CumulatedSessionCount = -1;
int hf_opcua_SecurityRejectedSessionCount = -1;
int hf_opcua_RejectSessionCount = -1;
int hf_opcua_SessionTimeoutCount = -1;
int hf_opcua_SessionAbortCount = -1;
int hf_opcua_SamplingRateCount = -1;
int hf_opcua_PublishingRateCount = -1;
int hf_opcua_CurrentSubscriptionCount = -1;
int hf_opcua_CumulatedSubscriptionCount = -1;
int hf_opcua_SecurityRejectedRequestsCount = -1;
int hf_opcua_RejectedRequestsCount = -1;
int hf_opcua_StartTime = -1;
int hf_opcua_CurrentTime = -1;
int hf_opcua_TotalCount = -1;
int hf_opcua_UnauthorizedCount = -1;
int hf_opcua_ErrorCount = -1;
int hf_opcua_ActualSessionTimeout = -1;
int hf_opcua_ClientConnectionTime = -1;
int hf_opcua_ClientLastContactTime = -1;
int hf_opcua_CurrentSubscriptionsCount = -1;
int hf_opcua_CurrentMonitoredItemsCount = -1;
int hf_opcua_CurrentPublishRequestsInQueue = -1;
int hf_opcua_CurrentPublishTimerExpirations = -1;
int hf_opcua_KeepAliveCount = -1;
int hf_opcua_CurrentRepublishRequestsInQueue = -1;
int hf_opcua_MaxRepublishRequestsInQueue = -1;
int hf_opcua_RepublishCounter = -1;
int hf_opcua_PublishingCount = -1;
int hf_opcua_PublishingQueueOverflowCount = -1;
int hf_opcua_ClientUserIdOfSession = -1;
int hf_opcua_ClientUserIdHistory = -1;
int hf_opcua_AuthenticationMechanism = -1;
int hf_opcua_Encoding = -1;
int hf_opcua_TransportProtocol = -1;
int hf_opcua_SecurityPolicy = -1;
int hf_opcua_PublishingInterval = -1;
int hf_opcua_MaxKeepAliveCount = -1;
int hf_opcua_ModifyCount = -1;
int hf_opcua_EnableCount = -1;
int hf_opcua_DisableCount = -1;
int hf_opcua_RepublishRequestCount = -1;
int hf_opcua_RepublishMessageRequestCount = -1;
int hf_opcua_RepublishMessageCount = -1;
int hf_opcua_TransferRequestCount = -1;
int hf_opcua_TransferredToAltClientCount = -1;
int hf_opcua_TransferredToSameClientCount = -1;
int hf_opcua_PublishRequestCount = -1;
int hf_opcua_DataChangeNotificationsCount = -1;
int hf_opcua_EventNotificationsCount = -1;
int hf_opcua_NotificationsCount = -1;
int hf_opcua_LateStateCount = -1;
int hf_opcua_KeepAliveStateCount = -1;
int hf_opcua_Low = -1;
int hf_opcua_High = -1;
int hf_opcua_NamespaceUri = -1;
int hf_opcua_UnitId = -1;
int hf_opcua_Message = -1;
int hf_opcua_AnnotationTime = -1;
int hf_opcua_Id = -1;
int hf_opcua_Description = -1;
int hf_opcua_Timestamp = -1;
int hf_opcua_Boolean = -1;
int hf_opcua_SByte = -1;
int hf_opcua_Byte = -1;
int hf_opcua_Int16 = -1;
int hf_opcua_UInt16 = -1;
int hf_opcua_Int32 = -1;
int hf_opcua_UInt32 = -1;
int hf_opcua_Int64 = -1;
int hf_opcua_UInt64 = -1;
int hf_opcua_Float = -1;
int hf_opcua_Double = -1;
int hf_opcua_String = -1;
int hf_opcua_DateTime = -1;
int hf_opcua_Guid = -1;
int hf_opcua_ByteString = -1;
int hf_opcua_XmlElement = -1;
int hf_opcua_RequestId = -1;
int hf_opcua_ReturnDiagnostics = -1;
int hf_opcua_AuditLogEntryId = -1;
int hf_opcua_TimeoutHint = -1;
int hf_opcua_ServiceResult = -1;
int hf_opcua_StringTable = -1;
int hf_opcua_Value1 = -1;
int hf_opcua_Value2 = -1;
int hf_opcua_Booleans = -1;
int hf_opcua_SBytes = -1;
int hf_opcua_Int16s = -1;
int hf_opcua_UInt16s = -1;
int hf_opcua_Int32s = -1;
int hf_opcua_UInt32s = -1;
int hf_opcua_Int64s = -1;
int hf_opcua_UInt64s = -1;
int hf_opcua_Floats = -1;
int hf_opcua_Doubles = -1;
int hf_opcua_Strings = -1;
int hf_opcua_DateTimes = -1;
int hf_opcua_Guids = -1;
int hf_opcua_ByteStrings = -1;
int hf_opcua_XmlElements = -1;
int hf_opcua_StatusCodes = -1;
int hf_opcua_SemaphoreFilePath = -1;
int hf_opcua_IsOnline = -1;
int hf_opcua_ChannelId = -1;
int hf_opcua_TokenId = -1;
int hf_opcua_CreatedAt = -1;
int hf_opcua_RevisedLifetime = -1;
int hf_opcua_Algorithm = -1;
int hf_opcua_Signature = -1;
int hf_opcua_PropertyStatusCode = -1;
int hf_opcua_ViewVersion = -1;
int hf_opcua_RelativePath = -1;
int hf_opcua_AttributeId = -1;
int hf_opcua_IndexRange = -1;
int hf_opcua_IncludeSubTypes = -1;
int hf_opcua_Alias = -1;
int hf_opcua_Result = -1;
int hf_opcua_IndexOfInvalidElement = -1;
int hf_opcua_AttributeStatusCodes = -1;
int hf_opcua_NumValuesPerNode = -1;
int hf_opcua_EndTime = -1;
int hf_opcua_IsReadModified = -1;
int hf_opcua_ReturnBounds = -1;
int hf_opcua_ResampleInterval = -1;
int hf_opcua_ReqTimes = -1;
int hf_opcua_ClientHandle = -1;
int hf_opcua_PerformInsert = -1;
int hf_opcua_PerformReplace = -1;
int hf_opcua_IsDeleteModified = -1;
int hf_opcua_OperationResult = -1;
int hf_opcua_InputArgumentResults = -1;
int hf_opcua_DeadbandType = -1;
int hf_opcua_DeadbandValue = -1;
int hf_opcua_SelectClauseResults = -1;
int hf_opcua_SamplingInterval = -1;
int hf_opcua_QueueSize = -1;
int hf_opcua_DiscardOldest = -1;
int hf_opcua_MonitoredItemId = -1;
int hf_opcua_RevisedSamplingInterval = -1;
int hf_opcua_RevisedQueueSize = -1;
int hf_opcua_MonitorItemId = -1;
int hf_opcua_PublishTime = -1;
int hf_opcua_AvailableSequenceNumbersRanges = -1;

/** header field definitions */
static hf_register_info hf[] =
{

   { &hf_opcua_TestId, { "TestId", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_Iteration, { "Iteration", "", FT_INT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_ServerUris, { "ServerUris", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_ProfileUris, { "ProfileUris", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_ClientCertificate, { "ClientCertificate", "", FT_BYTES, BASE_HEX, NULL, 0x0, "", HFILL } },
   { &hf_opcua_SecureChannelId, { "SecureChannelId", "", FT_GUID, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_SecurityPolicyUri, { "SecurityPolicyUri", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_ClientNonce, { "ClientNonce", "", FT_BYTES, BASE_HEX, NULL, 0x0, "", HFILL } },
   { &hf_opcua_RequestedLifetime, { "RequestedLifetime", "", FT_INT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_ServerCertificate, { "ServerCertificate", "", FT_BYTES, BASE_HEX, NULL, 0x0, "", HFILL } },
   { &hf_opcua_ServerNonce, { "ServerNonce", "", FT_BYTES, BASE_HEX, NULL, 0x0, "", HFILL } },
   { &hf_opcua_ClientName, { "ClientName", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_RequestedSessionTimeout, { "RequestedSessionTimeout", "", FT_INT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_SessionId, { "SessionId", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_RevisedSessionTimeout, { "RevisedSessionTimeout", "", FT_INT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_LocaleIds, { "LocaleIds", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_CertificateResults, { "CertificateResults", "", FT_UINT32, BASE_HEX, NULL, 0x0, "", HFILL } },
   { &hf_opcua_SequenceNumber, { "SequenceNumber", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_Results, { "Results", "", FT_UINT32, BASE_HEX, NULL, 0x0, "", HFILL } },
   { &hf_opcua_MaxResultsToReturn, { "MaxResultsToReturn", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_IncludeSubtypes, { "IncludeSubtypes", "", FT_BOOLEAN, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_NodeClassMask, { "NodeClassMask", "", FT_INT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_ContinuationPoint, { "ContinuationPoint", "", FT_BYTES, BASE_HEX, NULL, 0x0, "", HFILL } },
   { &hf_opcua_ReleaseContinuationPoint, { "ReleaseContinuationPoint", "", FT_BOOLEAN, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_RevisedContinuationPoint, { "RevisedContinuationPoint", "", FT_BYTES, BASE_HEX, NULL, 0x0, "", HFILL } },
   { &hf_opcua_MaxDescriptionsToReturn, { "MaxDescriptionsToReturn", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_MaxReferencesToReturn, { "MaxReferencesToReturn", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_MaxReferencedNodesToReturn, { "MaxReferencedNodesToReturn", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_MaxTime, { "MaxTime", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_MaxAge, { "MaxAge", "", FT_INT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_ReleaseContinuationPoints, { "ReleaseContinuationPoints", "", FT_BOOLEAN, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_SubscriptionId, { "SubscriptionId", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_MonitoredItemIds, { "MonitoredItemIds", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_TriggeringItemId, { "TriggeringItemId", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_LinksToAdd, { "LinksToAdd", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_LinksToRemove, { "LinksToRemove", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_AddResults, { "AddResults", "", FT_UINT32, BASE_HEX, NULL, 0x0, "", HFILL } },
   { &hf_opcua_RemoveResults, { "RemoveResults", "", FT_UINT32, BASE_HEX, NULL, 0x0, "", HFILL } },
   { &hf_opcua_RequestedPublishingInterval, { "RequestedPublishingInterval", "", FT_INT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_RequestedLifetimeCounter, { "RequestedLifetimeCounter", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_RequestedMaxKeepAliveCount, { "RequestedMaxKeepAliveCount", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_PublishingEnabled, { "PublishingEnabled", "", FT_BOOLEAN, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_Priority, { "Priority", "", FT_UINT8, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_RevisedPublishingInterval, { "RevisedPublishingInterval", "", FT_INT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_RevisedLifetimeCounter, { "RevisedLifetimeCounter", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_RevisedMaxKeepAliveCount, { "RevisedMaxKeepAliveCount", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_SubscriptionIds, { "SubscriptionIds", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_AvailableSequenceNumbers, { "AvailableSequenceNumbers", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_MoreNotifications, { "MoreNotifications", "", FT_BOOLEAN, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_RetransmitSequenceNumber, { "RetransmitSequenceNumber", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_IsInverse, { "IsInverse", "", FT_BOOLEAN, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_ServerIndex, { "ServerIndex", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_NodeClass, { "NodeClass", "", FT_INT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_EventNotifier, { "EventNotifier", "", FT_UINT8, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_IsAbstract, { "IsAbstract", "", FT_BOOLEAN, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_ArraySize, { "ArraySize", "", FT_INT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_AccessLevel, { "AccessLevel", "", FT_UINT8, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_UserAccessLevel, { "UserAccessLevel", "", FT_UINT8, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_MinimumSamplingInterval, { "MinimumSamplingInterval", "", FT_INT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_Historizing, { "Historizing", "", FT_BOOLEAN, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_Symmetric, { "Symmetric", "", FT_BOOLEAN, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_Executable, { "Executable", "", FT_BOOLEAN, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_UserExecutable, { "UserExecutable", "", FT_BOOLEAN, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_ContainsNoLoops, { "ContainsNoLoops", "", FT_BOOLEAN, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_Index, { "Index", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_Uri, { "Uri", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_Name, { "Name", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_StatusCode, { "StatusCode", "", FT_UINT32, BASE_HEX, NULL, 0x0, "", HFILL } },
   { &hf_opcua_EventId, { "EventId", "", FT_BYTES, BASE_HEX, NULL, 0x0, "", HFILL } },
   { &hf_opcua_SourceName, { "SourceName", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_Time, { "Time", "", FT_ABSOLUTE_TIME, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_ReceiveTime, { "ReceiveTime", "", FT_ABSOLUTE_TIME, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_Severity, { "Severity", "", FT_UINT16, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_Digest, { "Digest", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_SymmetricSignature, { "SymmetricSignature", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_SymmetricKeyWrap, { "SymmetricKeyWrap", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_SymmetricEncryption, { "SymmetricEncryption", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_SymmetricKeyLength, { "SymmetricKeyLength", "", FT_INT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_AsymmetricSignature, { "AsymmetricSignature", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_AsymmetricKeyWrap, { "AsymmetricKeyWrap", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_AsymmetricEncryption, { "AsymmetricEncryption", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_MinimumAsymmetricKeyLength, { "MinimumAsymmetricKeyLength", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_MaximumAsymmetricKeyLength, { "MaximumAsymmetricKeyLength", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_DerivedKey, { "DerivedKey", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_DerivedEncryptionKeyLength, { "DerivedEncryptionKeyLength", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_DerivedSignatureKeyLength, { "DerivedSignatureKeyLength", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_IssuerType, { "IssuerType", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_IssuerUrl, { "IssuerUrl", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_ServerUri, { "ServerUri", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_DiscoveryUrls, { "DiscoveryUrls", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_EndpointUrl, { "EndpointUrl", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_SupportedProfiles, { "SupportedProfiles", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_SendTimeout, { "SendTimeout", "", FT_INT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_OperationTimeout, { "OperationTimeout", "", FT_INT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_UseBinaryEncoding, { "UseBinaryEncoding", "", FT_BOOLEAN, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_MaxMessageSize, { "MaxMessageSize", "", FT_INT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_MaxArrayLength, { "MaxArrayLength", "", FT_INT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_MaxStringLength, { "MaxStringLength", "", FT_INT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_UserName, { "UserName", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_Password, { "Password", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_HashAlgorithm, { "HashAlgorithm", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_CertificateData, { "CertificateData", "", FT_BYTES, BASE_HEX, NULL, 0x0, "", HFILL } },
   { &hf_opcua_TokenData, { "TokenData", "", FT_BYTES, BASE_HEX, NULL, 0x0, "", HFILL } },
   { &hf_opcua_ProfileUri, { "ProfileUri", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_ProfileName, { "ProfileName", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_ApplicationUri, { "ApplicationUri", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_ManufacturerName, { "ManufacturerName", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_ApplicationName, { "ApplicationName", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_SoftwareVersion, { "SoftwareVersion", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_BuildNumber, { "BuildNumber", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_BuildDate, { "BuildDate", "", FT_ABSOLUTE_TIME, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_IssuedBy, { "IssuedBy", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_IssuedDate, { "IssuedDate", "", FT_ABSOLUTE_TIME, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_ExpirationDate, { "ExpirationDate", "", FT_ABSOLUTE_TIME, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_ApplicationCertificate, { "ApplicationCertificate", "", FT_BYTES, BASE_HEX, NULL, 0x0, "", HFILL } },
   { &hf_opcua_IssuerCertificateThumbprint, { "IssuerCertificateThumbprint", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_IssuerSignatureAlgorithm, { "IssuerSignatureAlgorithm", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_IssuerSignature, { "IssuerSignature", "", FT_BYTES, BASE_HEX, NULL, 0x0, "", HFILL } },
   { &hf_opcua_IsForward, { "IsForward", "", FT_BOOLEAN, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_TargetServerUri, { "TargetServerUri", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_TargetNodeClass, { "TargetNodeClass", "", FT_INT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_DeleteTargetReferences, { "DeleteTargetReferences", "", FT_BOOLEAN, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_ServerId, { "ServerId", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_ServiceLevel, { "ServiceLevel", "", FT_INT8, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_SamplingRate, { "SamplingRate", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_SamplingErrorCount, { "SamplingErrorCount", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_SampledMonitoredItemsCount, { "SampledMonitoredItemsCount", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_MaxSampledMonitoredItemsCount, { "MaxSampledMonitoredItemsCount", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_DisabledMonitoredItemsSamplingCount, { "DisabledMonitoredItemsSamplingCount", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_ServerViewCount, { "ServerViewCount", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_CurrentSessionCount, { "CurrentSessionCount", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_CumulatedSessionCount, { "CumulatedSessionCount", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_SecurityRejectedSessionCount, { "SecurityRejectedSessionCount", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_RejectSessionCount, { "RejectSessionCount", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_SessionTimeoutCount, { "SessionTimeoutCount", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_SessionAbortCount, { "SessionAbortCount", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_SamplingRateCount, { "SamplingRateCount", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_PublishingRateCount, { "PublishingRateCount", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_CurrentSubscriptionCount, { "CurrentSubscriptionCount", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_CumulatedSubscriptionCount, { "CumulatedSubscriptionCount", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_SecurityRejectedRequestsCount, { "SecurityRejectedRequestsCount", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_RejectedRequestsCount, { "RejectedRequestsCount", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_StartTime, { "StartTime", "", FT_ABSOLUTE_TIME, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_CurrentTime, { "CurrentTime", "", FT_ABSOLUTE_TIME, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_TotalCount, { "TotalCount", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_UnauthorizedCount, { "UnauthorizedCount", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_ErrorCount, { "ErrorCount", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_ActualSessionTimeout, { "ActualSessionTimeout", "", FT_INT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_ClientConnectionTime, { "ClientConnectionTime", "", FT_ABSOLUTE_TIME, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_ClientLastContactTime, { "ClientLastContactTime", "", FT_ABSOLUTE_TIME, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_CurrentSubscriptionsCount, { "CurrentSubscriptionsCount", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_CurrentMonitoredItemsCount, { "CurrentMonitoredItemsCount", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_CurrentPublishRequestsInQueue, { "CurrentPublishRequestsInQueue", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_CurrentPublishTimerExpirations, { "CurrentPublishTimerExpirations", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_KeepAliveCount, { "KeepAliveCount", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_CurrentRepublishRequestsInQueue, { "CurrentRepublishRequestsInQueue", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_MaxRepublishRequestsInQueue, { "MaxRepublishRequestsInQueue", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_RepublishCounter, { "RepublishCounter", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_PublishingCount, { "PublishingCount", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_PublishingQueueOverflowCount, { "PublishingQueueOverflowCount", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_ClientUserIdOfSession, { "ClientUserIdOfSession", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_ClientUserIdHistory, { "ClientUserIdHistory", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_AuthenticationMechanism, { "AuthenticationMechanism", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_Encoding, { "Encoding", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_TransportProtocol, { "TransportProtocol", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_SecurityPolicy, { "SecurityPolicy", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_PublishingInterval, { "PublishingInterval", "", FT_INT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_MaxKeepAliveCount, { "MaxKeepAliveCount", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_ModifyCount, { "ModifyCount", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_EnableCount, { "EnableCount", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_DisableCount, { "DisableCount", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_RepublishRequestCount, { "RepublishRequestCount", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_RepublishMessageRequestCount, { "RepublishMessageRequestCount", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_RepublishMessageCount, { "RepublishMessageCount", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_TransferRequestCount, { "TransferRequestCount", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_TransferredToAltClientCount, { "TransferredToAltClientCount", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_TransferredToSameClientCount, { "TransferredToSameClientCount", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_PublishRequestCount, { "PublishRequestCount", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_DataChangeNotificationsCount, { "DataChangeNotificationsCount", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_EventNotificationsCount, { "EventNotificationsCount", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_NotificationsCount, { "NotificationsCount", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_LateStateCount, { "LateStateCount", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_KeepAliveStateCount, { "KeepAliveStateCount", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_Low, { "Low", "", FT_DOUBLE, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_High, { "High", "", FT_DOUBLE, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_NamespaceUri, { "NamespaceUri", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_UnitId, { "UnitId", "", FT_INT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_Message, { "Message", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_AnnotationTime, { "AnnotationTime", "", FT_ABSOLUTE_TIME, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_Id, { "Id", "", FT_INT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_Description, { "Description", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_Timestamp, { "Timestamp", "", FT_ABSOLUTE_TIME, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_Boolean, { "Boolean", "", FT_BOOLEAN, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_SByte, { "SByte", "", FT_INT8, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_Byte, { "Byte", "", FT_UINT8, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_Int16, { "Int16", "", FT_INT16, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_UInt16, { "UInt16", "", FT_UINT16, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_Int32, { "Int32", "", FT_INT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_UInt32, { "UInt32", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_Int64, { "Int64", "", FT_INT64, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_UInt64, { "UInt64", "", FT_UINT64, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_Float, { "Float", "", FT_FLOAT, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_Double, { "Double", "", FT_DOUBLE, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_String, { "String", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_DateTime, { "DateTime", "", FT_ABSOLUTE_TIME, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_Guid, { "Guid", "", FT_GUID, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_ByteString, { "ByteString", "", FT_BYTES, BASE_HEX, NULL, 0x0, "", HFILL } },
   { &hf_opcua_XmlElement, { "XmlElement", "", FT_BYTES, BASE_HEX, NULL, 0x0, "", HFILL } },
   { &hf_opcua_RequestId, { "RequestId", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_ReturnDiagnostics, { "ReturnDiagnostics", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_AuditLogEntryId, { "AuditLogEntryId", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_TimeoutHint, { "TimeoutHint", "", FT_INT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_ServiceResult, { "ServiceResult", "", FT_UINT32, BASE_HEX, NULL, 0x0, "", HFILL } },
   { &hf_opcua_StringTable, { "StringTable", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_Value1, { "Value1", "", FT_ABSOLUTE_TIME, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_Value2, { "Value2", "", FT_ABSOLUTE_TIME, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_Booleans, { "Booleans", "", FT_BOOLEAN, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_SBytes, { "SBytes", "", FT_INT8, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_Int16s, { "Int16s", "", FT_INT16, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_UInt16s, { "UInt16s", "", FT_UINT16, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_Int32s, { "Int32s", "", FT_INT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_UInt32s, { "UInt32s", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_Int64s, { "Int64s", "", FT_INT64, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_UInt64s, { "UInt64s", "", FT_UINT64, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_Floats, { "Floats", "", FT_FLOAT, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_Doubles, { "Doubles", "", FT_DOUBLE, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_Strings, { "Strings", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_DateTimes, { "DateTimes", "", FT_ABSOLUTE_TIME, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_Guids, { "Guids", "", FT_GUID, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_ByteStrings, { "ByteStrings", "", FT_BYTES, BASE_HEX, NULL, 0x0, "", HFILL } },
   { &hf_opcua_XmlElements, { "XmlElements", "", FT_BYTES, BASE_HEX, NULL, 0x0, "", HFILL } },
   { &hf_opcua_StatusCodes, { "StatusCodes", "", FT_UINT32, BASE_HEX, NULL, 0x0, "", HFILL } },
   { &hf_opcua_SemaphoreFilePath, { "SemaphoreFilePath", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_IsOnline, { "IsOnline", "", FT_BOOLEAN, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_ChannelId, { "ChannelId", "", FT_GUID, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_TokenId, { "TokenId", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_CreatedAt, { "CreatedAt", "", FT_ABSOLUTE_TIME, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_RevisedLifetime, { "RevisedLifetime", "", FT_INT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_Algorithm, { "Algorithm", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_Signature, { "Signature", "", FT_BYTES, BASE_HEX, NULL, 0x0, "", HFILL } },
   { &hf_opcua_PropertyStatusCode, { "PropertyStatusCode", "", FT_UINT32, BASE_HEX, NULL, 0x0, "", HFILL } },
   { &hf_opcua_ViewVersion, { "ViewVersion", "", FT_INT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_RelativePath, { "RelativePath", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_AttributeId, { "AttributeId", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_IndexRange, { "IndexRange", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_IncludeSubTypes, { "IncludeSubTypes", "", FT_BOOLEAN, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_Alias, { "Alias", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_Result, { "Result", "", FT_UINT32, BASE_HEX, NULL, 0x0, "", HFILL } },
   { &hf_opcua_IndexOfInvalidElement, { "IndexOfInvalidElement", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_AttributeStatusCodes, { "AttributeStatusCodes", "", FT_UINT32, BASE_HEX, NULL, 0x0, "", HFILL } },
   { &hf_opcua_NumValuesPerNode, { "NumValuesPerNode", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_EndTime, { "EndTime", "", FT_ABSOLUTE_TIME, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_IsReadModified, { "IsReadModified", "", FT_BOOLEAN, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_ReturnBounds, { "ReturnBounds", "", FT_BOOLEAN, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_ResampleInterval, { "ResampleInterval", "", FT_INT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_ReqTimes, { "ReqTimes", "", FT_ABSOLUTE_TIME, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_ClientHandle, { "ClientHandle", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_PerformInsert, { "PerformInsert", "", FT_BOOLEAN, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_PerformReplace, { "PerformReplace", "", FT_BOOLEAN, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_IsDeleteModified, { "IsDeleteModified", "", FT_BOOLEAN, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_OperationResult, { "OperationResult", "", FT_UINT32, BASE_HEX, NULL, 0x0, "", HFILL } },
   { &hf_opcua_InputArgumentResults, { "InputArgumentResults", "", FT_UINT32, BASE_HEX, NULL, 0x0, "", HFILL } },
   { &hf_opcua_DeadbandType, { "DeadbandType", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_DeadbandValue, { "DeadbandValue", "", FT_DOUBLE, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_SelectClauseResults, { "SelectClauseResults", "", FT_UINT32, BASE_HEX, NULL, 0x0, "", HFILL } },
   { &hf_opcua_SamplingInterval, { "SamplingInterval", "", FT_INT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_QueueSize, { "QueueSize", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_DiscardOldest, { "DiscardOldest", "", FT_BOOLEAN, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_MonitoredItemId, { "MonitoredItemId", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_RevisedSamplingInterval, { "RevisedSamplingInterval", "", FT_INT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_RevisedQueueSize, { "RevisedQueueSize", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_MonitorItemId, { "MonitorItemId", "", FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL } },
   { &hf_opcua_PublishTime, { "PublishTime", "", FT_ABSOLUTE_TIME, BASE_NONE, NULL, 0x0, "", HFILL } },
   { &hf_opcua_AvailableSequenceNumbersRanges, { "AvailableSequenceNumbersRanges", "", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } }
};

/** Register field types. */
void registerFieldTypes(int proto)
{
    proto_register_field_array(proto, hf, array_length(hf));
}
