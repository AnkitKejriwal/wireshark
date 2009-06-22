/******************************************************************************
** $Id$
**
** Copyright (C) 2006-2009 ascolab GmbH. All Rights Reserved.
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
** Description: OpcUa Enum Type Parser
**
** This file was autogenerated on 31.03.2009.
** DON'T MODIFY THIS FILE!
**
******************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glib.h>
#include <epan/packet.h>

#include "opcua_enumparser.h"

/** NodeIdType enum table */
static const value_string g_NodeIdTypeTable[] = {
  { 0, "TwoByte" },
  { 1, "FourByte" },
  { 2, "Numeric" },
  { 3, "String" },
  { 4, "Uri" },
  { 5, "Guid" },
  { 6, "ByteString" },
  { 0, NULL }
};
static int hf_opcua_NodeIdType = -1;

void parseNodeIdType(proto_tree *tree, tvbuff_t *tvb, gint *pOffset)
{
    proto_tree_add_item(tree, hf_opcua_NodeIdType, tvb, *pOffset, 4, TRUE); *pOffset+=4;
}
/** DialogConditionChoice enum table */
static const value_string g_DialogConditionChoiceTable[] = {
  { 0, "None" },
  { 1, "Ok" },
  { 2, "Cancel" },
  { 4, "Yes" },
  { 8, "No" },
  { 16, "Abort" },
  { 0, NULL }
};
static int hf_opcua_DialogConditionChoice = -1;

void parseDialogConditionChoice(proto_tree *tree, tvbuff_t *tvb, gint *pOffset)
{
    proto_tree_add_item(tree, hf_opcua_DialogConditionChoice, tvb, *pOffset, 4, TRUE); *pOffset+=4;
}
/** IdType enum table */
static const value_string g_IdTypeTable[] = {
  { 0, "Numeric" },
  { 1, "String" },
  { 2, "Guid" },
  { 3, "Opaque" },
  { 0, NULL }
};
static int hf_opcua_IdType = -1;

void parseIdType(proto_tree *tree, tvbuff_t *tvb, gint *pOffset)
{
    proto_tree_add_item(tree, hf_opcua_IdType, tvb, *pOffset, 4, TRUE); *pOffset+=4;
}
/** NodeClass enum table */
static const value_string g_NodeClassTable[] = {
  { 0, "Unspecified" },
  { 1, "Object" },
  { 2, "Variable" },
  { 4, "Method" },
  { 8, "ObjectType" },
  { 16, "VariableType" },
  { 32, "ReferenceType" },
  { 64, "DataType" },
  { 128, "View" },
  { 0, NULL }
};
static int hf_opcua_NodeClass = -1;

void parseNodeClass(proto_tree *tree, tvbuff_t *tvb, gint *pOffset)
{
    proto_tree_add_item(tree, hf_opcua_NodeClass, tvb, *pOffset, 4, TRUE); *pOffset+=4;
}
/** MessageSecurityMode enum table */
static const value_string g_MessageSecurityModeTable[] = {
  { 0, "Invalid" },
  { 1, "None" },
  { 2, "Sign" },
  { 3, "SignAndEncrypt" },
  { 0, NULL }
};
static int hf_opcua_MessageSecurityMode = -1;

void parseMessageSecurityMode(proto_tree *tree, tvbuff_t *tvb, gint *pOffset)
{
    proto_tree_add_item(tree, hf_opcua_MessageSecurityMode, tvb, *pOffset, 4, TRUE); *pOffset+=4;
}
/** UserTokenType enum table */
static const value_string g_UserTokenTypeTable[] = {
  { 0, "Anonymous" },
  { 1, "UserName" },
  { 2, "Certificate" },
  { 3, "IssuedToken" },
  { 0, NULL }
};
static int hf_opcua_UserTokenType = -1;

void parseUserTokenType(proto_tree *tree, tvbuff_t *tvb, gint *pOffset)
{
    proto_tree_add_item(tree, hf_opcua_UserTokenType, tvb, *pOffset, 4, TRUE); *pOffset+=4;
}
/** ApplicationType enum table */
static const value_string g_ApplicationTypeTable[] = {
  { 0, "Server" },
  { 1, "Client" },
  { 2, "ClientAndServer" },
  { 3, "DiscoveryServer" },
  { 0, NULL }
};
static int hf_opcua_ApplicationType = -1;

void parseApplicationType(proto_tree *tree, tvbuff_t *tvb, gint *pOffset)
{
    proto_tree_add_item(tree, hf_opcua_ApplicationType, tvb, *pOffset, 4, TRUE); *pOffset+=4;
}
/** SecurityTokenRequestType enum table */
static const value_string g_SecurityTokenRequestTypeTable[] = {
  { 0, "Issue" },
  { 1, "Renew" },
  { 0, NULL }
};
static int hf_opcua_SecurityTokenRequestType = -1;

void parseSecurityTokenRequestType(proto_tree *tree, tvbuff_t *tvb, gint *pOffset)
{
    proto_tree_add_item(tree, hf_opcua_SecurityTokenRequestType, tvb, *pOffset, 4, TRUE); *pOffset+=4;
}
/** ComplianceLevel enum table */
static const value_string g_ComplianceLevelTable[] = {
  { 0, "Untested" },
  { 1, "Partial" },
  { 2, "SelfTested" },
  { 3, "Certified" },
  { 0, NULL }
};
static int hf_opcua_ComplianceLevel = -1;

void parseComplianceLevel(proto_tree *tree, tvbuff_t *tvb, gint *pOffset)
{
    proto_tree_add_item(tree, hf_opcua_ComplianceLevel, tvb, *pOffset, 4, TRUE); *pOffset+=4;
}
/** AttributeWriteMask enum table */
static const value_string g_AttributeWriteMaskTable[] = {
  { 0, "None" },
  { 1, "AccessLevel" },
  { 2, "ArrayDimensions" },
  { 4, "BrowseName" },
  { 8, "ContainsNoLoops" },
  { 16, "DataType" },
  { 32, "Description" },
  { 64, "DisplayName" },
  { 128, "EventNotifier" },
  { 256, "Executable" },
  { 512, "Historizing" },
  { 1024, "InverseName" },
  { 2048, "IsAbstract" },
  { 4096, "MinimumSamplingInterval" },
  { 8192, "NodeClass" },
  { 16384, "NodeId" },
  { 32768, "Symmetric" },
  { 65536, "UserAccessLevel" },
  { 131072, "UserExecutable" },
  { 262144, "UserWriteMask" },
  { 524288, "ValueRank" },
  { 1048576, "WriteMask" },
  { 2097152, "ValueForVariableType" },
  { 0, NULL }
};
static int hf_opcua_AttributeWriteMask = -1;

void parseAttributeWriteMask(proto_tree *tree, tvbuff_t *tvb, gint *pOffset)
{
    proto_tree_add_item(tree, hf_opcua_AttributeWriteMask, tvb, *pOffset, 4, TRUE); *pOffset+=4;
}
/** NodeAttributesMask enum table */
static const value_string g_NodeAttributesMaskTable[] = {
  { 0, "None" },
  { 1, "AccessLevel" },
  { 2, "ArrayDimensions" },
  { 8, "ContainsNoLoops" },
  { 16, "DataType" },
  { 32, "Description" },
  { 64, "DisplayName" },
  { 128, "EventNotifier" },
  { 256, "Executable" },
  { 512, "Historizing" },
  { 1024, "InverseName" },
  { 2048, "IsAbstract" },
  { 4096, "MinimumSamplingInterval" },
  { 32768, "Symmetric" },
  { 65536, "UserAccessLevel" },
  { 131072, "UserExecutable" },
  { 262144, "UserWriteMask" },
  { 524288, "ValueRank" },
  { 1048576, "WriteMask" },
  { 2097152, "Value" },
  { 4194303, "All" },
  { 1335396, "BaseNode" },
  { 1335524, "Object" },
  { 1337444, "ObjectTypeOrDataType" },
  { 4026999, "Variable" },
  { 3958902, "VariableType" },
  { 1466724, "Method" },
  { 1371236, "ReferenceType" },
  { 1335532, "View" },
  { 0, NULL }
};
static int hf_opcua_NodeAttributesMask = -1;

void parseNodeAttributesMask(proto_tree *tree, tvbuff_t *tvb, gint *pOffset)
{
    proto_tree_add_item(tree, hf_opcua_NodeAttributesMask, tvb, *pOffset, 4, TRUE); *pOffset+=4;
}
/** EnumeratedTestType enum table */
static const value_string g_EnumeratedTestTypeTable[] = {
  { 1, "Red" },
  { 4, "Yellow" },
  { 5, "Green" },
  { 0, NULL }
};
static int hf_opcua_EnumeratedTestType = -1;

void parseEnumeratedTestType(proto_tree *tree, tvbuff_t *tvb, gint *pOffset)
{
    proto_tree_add_item(tree, hf_opcua_EnumeratedTestType, tvb, *pOffset, 4, TRUE); *pOffset+=4;
}
/** BrowseDirection enum table */
static const value_string g_BrowseDirectionTable[] = {
  { 0, "Forward" },
  { 1, "Inverse" },
  { 2, "Both" },
  { 0, NULL }
};
static int hf_opcua_BrowseDirection = -1;

void parseBrowseDirection(proto_tree *tree, tvbuff_t *tvb, gint *pOffset)
{
    proto_tree_add_item(tree, hf_opcua_BrowseDirection, tvb, *pOffset, 4, TRUE); *pOffset+=4;
}
/** BrowseResultMask enum table */
static const value_string g_BrowseResultMaskTable[] = {
  { 0, "None" },
  { 1, "ReferenceTypeId" },
  { 2, "IsForward" },
  { 4, "NodeClass" },
  { 8, "BrowseName" },
  { 16, "DisplayName" },
  { 32, "TypeDefinition" },
  { 63, "All" },
  { 3, "ReferenceTypeInfo" },
  { 60, "TargetInfo" },
  { 0, NULL }
};
static int hf_opcua_BrowseResultMask = -1;

void parseBrowseResultMask(proto_tree *tree, tvbuff_t *tvb, gint *pOffset)
{
    proto_tree_add_item(tree, hf_opcua_BrowseResultMask, tvb, *pOffset, 4, TRUE); *pOffset+=4;
}
/** FilterOperator enum table */
static const value_string g_FilterOperatorTable[] = {
  { 0, "Equals" },
  { 1, "IsNull" },
  { 2, "GreaterThan" },
  { 3, "LessThan" },
  { 4, "GreaterThanOrEqual" },
  { 5, "LessThanOrEqual" },
  { 6, "Like" },
  { 7, "Not" },
  { 8, "Between" },
  { 9, "InList" },
  { 10, "And" },
  { 11, "Or" },
  { 12, "Cast" },
  { 13, "InView" },
  { 14, "OfType" },
  { 15, "RelatedTo" },
  { 16, "BitwiseAnd" },
  { 17, "BitwiseOr" },
  { 0, NULL }
};
static int hf_opcua_FilterOperator = -1;

void parseFilterOperator(proto_tree *tree, tvbuff_t *tvb, gint *pOffset)
{
    proto_tree_add_item(tree, hf_opcua_FilterOperator, tvb, *pOffset, 4, TRUE); *pOffset+=4;
}
/** TimestampsToReturn enum table */
static const value_string g_TimestampsToReturnTable[] = {
  { 0, "Source" },
  { 1, "Server" },
  { 2, "Both" },
  { 3, "Neither" },
  { 0, NULL }
};
static int hf_opcua_TimestampsToReturn = -1;

void parseTimestampsToReturn(proto_tree *tree, tvbuff_t *tvb, gint *pOffset)
{
    proto_tree_add_item(tree, hf_opcua_TimestampsToReturn, tvb, *pOffset, 4, TRUE); *pOffset+=4;
}
/** HistoryUpdateMode enum table */
static const value_string g_HistoryUpdateModeTable[] = {
  { 1, "Insert" },
  { 2, "Replace" },
  { 3, "InsertReplace" },
  { 0, NULL }
};
static int hf_opcua_HistoryUpdateMode = -1;

void parseHistoryUpdateMode(proto_tree *tree, tvbuff_t *tvb, gint *pOffset)
{
    proto_tree_add_item(tree, hf_opcua_HistoryUpdateMode, tvb, *pOffset, 4, TRUE); *pOffset+=4;
}
/** MonitoringMode enum table */
static const value_string g_MonitoringModeTable[] = {
  { 0, "Disabled" },
  { 1, "Sampling" },
  { 2, "Reporting" },
  { 0, NULL }
};
static int hf_opcua_MonitoringMode = -1;

void parseMonitoringMode(proto_tree *tree, tvbuff_t *tvb, gint *pOffset)
{
    proto_tree_add_item(tree, hf_opcua_MonitoringMode, tvb, *pOffset, 4, TRUE); *pOffset+=4;
}
/** DataChangeTrigger enum table */
static const value_string g_DataChangeTriggerTable[] = {
  { 0, "Status" },
  { 1, "StatusValue" },
  { 2, "StatusValueTimestamp" },
  { 0, NULL }
};
static int hf_opcua_DataChangeTrigger = -1;

void parseDataChangeTrigger(proto_tree *tree, tvbuff_t *tvb, gint *pOffset)
{
    proto_tree_add_item(tree, hf_opcua_DataChangeTrigger, tvb, *pOffset, 4, TRUE); *pOffset+=4;
}
/** DeadbandType enum table */
static const value_string g_DeadbandTypeTable[] = {
  { 0, "None" },
  { 1, "Absolute" },
  { 2, "Percent" },
  { 0, NULL }
};
static int hf_opcua_DeadbandType = -1;

void parseDeadbandType(proto_tree *tree, tvbuff_t *tvb, gint *pOffset)
{
    proto_tree_add_item(tree, hf_opcua_DeadbandType, tvb, *pOffset, 4, TRUE); *pOffset+=4;
}
/** RedundancySupport enum table */
static const value_string g_RedundancySupportTable[] = {
  { 0, "None" },
  { 1, "Cold" },
  { 2, "Warm" },
  { 3, "Hot" },
  { 4, "Transparent" },
  { 0, NULL }
};
static int hf_opcua_RedundancySupport = -1;

void parseRedundancySupport(proto_tree *tree, tvbuff_t *tvb, gint *pOffset)
{
    proto_tree_add_item(tree, hf_opcua_RedundancySupport, tvb, *pOffset, 4, TRUE); *pOffset+=4;
}
/** ServerState enum table */
static const value_string g_ServerStateTable[] = {
  { 0, "Running" },
  { 1, "Failed" },
  { 2, "NoConfiguration" },
  { 3, "Suspended" },
  { 4, "Shutdown" },
  { 5, "Test" },
  { 6, "CommunicationFault" },
  { 7, "Unknown" },
  { 0, NULL }
};
static int hf_opcua_ServerState = -1;

void parseServerState(proto_tree *tree, tvbuff_t *tvb, gint *pOffset)
{
    proto_tree_add_item(tree, hf_opcua_ServerState, tvb, *pOffset, 4, TRUE); *pOffset+=4;
}
/** ModelChangeStructureVerbMask enum table */
static const value_string g_ModelChangeStructureVerbMaskTable[] = {
  { 1, "NodeAdded" },
  { 2, "NodeDeleted" },
  { 4, "ReferenceAdded" },
  { 8, "ReferenceDeleted" },
  { 16, "DataTypeChanged" },
  { 0, NULL }
};
static int hf_opcua_ModelChangeStructureVerbMask = -1;

void parseModelChangeStructureVerbMask(proto_tree *tree, tvbuff_t *tvb, gint *pOffset)
{
    proto_tree_add_item(tree, hf_opcua_ModelChangeStructureVerbMask, tvb, *pOffset, 4, TRUE); *pOffset+=4;
}
/** ExceptionDeviationFormat enum table */
static const value_string g_ExceptionDeviationFormatTable[] = {
  { 0, "AbsoluteValue" },
  { 1, "PercentOfRange" },
  { 2, "PercentOfValue" },
  { 3, "PercentOfEURange" },
  { 4, "Unknown" },
  { 0, NULL }
};
static int hf_opcua_ExceptionDeviationFormat = -1;

void parseExceptionDeviationFormat(proto_tree *tree, tvbuff_t *tvb, gint *pOffset)
{
    proto_tree_add_item(tree, hf_opcua_ExceptionDeviationFormat, tvb, *pOffset, 4, TRUE); *pOffset+=4;
}

/** Register enum types. */
void registerEnumTypes(int proto)
{
    /** header field definitions */
    static hf_register_info hf[] =
    {
        { &hf_opcua_NodeIdType,
        {  "NodeIdType", "", FT_UINT32, BASE_HEX,  VALS(g_NodeIdTypeTable), 0x0, "", HFILL }
        },
        { &hf_opcua_DialogConditionChoice,
        {  "DialogConditionChoice", "", FT_UINT32, BASE_HEX,  VALS(g_DialogConditionChoiceTable), 0x0, "", HFILL }
        },
        { &hf_opcua_IdType,
        {  "IdType", "", FT_UINT32, BASE_HEX,  VALS(g_IdTypeTable), 0x0, "", HFILL }
        },
        { &hf_opcua_NodeClass,
        {  "NodeClass", "", FT_UINT32, BASE_HEX,  VALS(g_NodeClassTable), 0x0, "", HFILL }
        },
        { &hf_opcua_MessageSecurityMode,
        {  "MessageSecurityMode", "", FT_UINT32, BASE_HEX,  VALS(g_MessageSecurityModeTable), 0x0, "", HFILL }
        },
        { &hf_opcua_UserTokenType,
        {  "UserTokenType", "", FT_UINT32, BASE_HEX,  VALS(g_UserTokenTypeTable), 0x0, "", HFILL }
        },
        { &hf_opcua_ApplicationType,
        {  "ApplicationType", "", FT_UINT32, BASE_HEX,  VALS(g_ApplicationTypeTable), 0x0, "", HFILL }
        },
        { &hf_opcua_SecurityTokenRequestType,
        {  "SecurityTokenRequestType", "", FT_UINT32, BASE_HEX,  VALS(g_SecurityTokenRequestTypeTable), 0x0, "", HFILL }
        },
        { &hf_opcua_ComplianceLevel,
        {  "ComplianceLevel", "", FT_UINT32, BASE_HEX,  VALS(g_ComplianceLevelTable), 0x0, "", HFILL }
        },
        { &hf_opcua_AttributeWriteMask,
        {  "AttributeWriteMask", "", FT_UINT32, BASE_HEX,  VALS(g_AttributeWriteMaskTable), 0x0, "", HFILL }
        },
        { &hf_opcua_NodeAttributesMask,
        {  "NodeAttributesMask", "", FT_UINT32, BASE_HEX,  VALS(g_NodeAttributesMaskTable), 0x0, "", HFILL }
        },
        { &hf_opcua_EnumeratedTestType,
        {  "EnumeratedTestType", "", FT_UINT32, BASE_HEX,  VALS(g_EnumeratedTestTypeTable), 0x0, "", HFILL }
        },
        { &hf_opcua_BrowseDirection,
        {  "BrowseDirection", "", FT_UINT32, BASE_HEX,  VALS(g_BrowseDirectionTable), 0x0, "", HFILL }
        },
        { &hf_opcua_BrowseResultMask,
        {  "BrowseResultMask", "", FT_UINT32, BASE_HEX,  VALS(g_BrowseResultMaskTable), 0x0, "", HFILL }
        },
        { &hf_opcua_FilterOperator,
        {  "FilterOperator", "", FT_UINT32, BASE_HEX,  VALS(g_FilterOperatorTable), 0x0, "", HFILL }
        },
        { &hf_opcua_TimestampsToReturn,
        {  "TimestampsToReturn", "", FT_UINT32, BASE_HEX,  VALS(g_TimestampsToReturnTable), 0x0, "", HFILL }
        },
        { &hf_opcua_HistoryUpdateMode,
        {  "HistoryUpdateMode", "", FT_UINT32, BASE_HEX,  VALS(g_HistoryUpdateModeTable), 0x0, "", HFILL }
        },
        { &hf_opcua_MonitoringMode,
        {  "MonitoringMode", "", FT_UINT32, BASE_HEX,  VALS(g_MonitoringModeTable), 0x0, "", HFILL }
        },
        { &hf_opcua_DataChangeTrigger,
        {  "DataChangeTrigger", "", FT_UINT32, BASE_HEX,  VALS(g_DataChangeTriggerTable), 0x0, "", HFILL }
        },
        { &hf_opcua_DeadbandType,
        {  "DeadbandType", "", FT_UINT32, BASE_HEX,  VALS(g_DeadbandTypeTable), 0x0, "", HFILL }
        },
        { &hf_opcua_RedundancySupport,
        {  "RedundancySupport", "", FT_UINT32, BASE_HEX,  VALS(g_RedundancySupportTable), 0x0, "", HFILL }
        },
        { &hf_opcua_ServerState,
        {  "ServerState", "", FT_UINT32, BASE_HEX,  VALS(g_ServerStateTable), 0x0, "", HFILL }
        },
        { &hf_opcua_ModelChangeStructureVerbMask,
        {  "ModelChangeStructureVerbMask", "", FT_UINT32, BASE_HEX,  VALS(g_ModelChangeStructureVerbMaskTable), 0x0, "", HFILL }
        },
        { &hf_opcua_ExceptionDeviationFormat,
        {  "ExceptionDeviationFormat", "", FT_UINT32, BASE_HEX,  VALS(g_ExceptionDeviationFormatTable), 0x0, "", HFILL }
        },
    };

    proto_register_field_array(proto, hf, array_length(hf));
}

