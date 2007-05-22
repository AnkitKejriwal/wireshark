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
** Description: OpcUa Service Type Parser
**
** This file was autogenerated on 8.5.2007 18:53:26.
** DON'T MODIFY THIS FILE!
**
******************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glib.h>
#include <epan/packet.h>

void parseTestStackRequest(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseTestStackResponse(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseTestStackExRequest(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseTestStackExResponse(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseFindServersRequest(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseFindServersResponse(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseGetEndpointsRequest(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseGetEndpointsResponse(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseRegisterServerRequest(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseRegisterServerResponse(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseOpenSecureChannelRequest(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseOpenSecureChannelResponse(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseCloseSecureChannelRequest(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseCloseSecureChannelResponse(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseCreateSessionRequest(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseCreateSessionResponse(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseActivateSessionRequest(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseActivateSessionResponse(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseCloseSessionRequest(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseCloseSessionResponse(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseCancelRequest(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseCancelResponse(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseAddNodesRequest(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseAddNodesResponse(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseAddReferencesRequest(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseAddReferencesResponse(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseDeleteNodesRequest(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseDeleteNodesResponse(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseDeleteReferencesRequest(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseDeleteReferencesResponse(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseBrowsePropertiesRequest(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseBrowsePropertiesResponse(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseBrowseRequest(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseBrowseResponse(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseBrowseNextRequest(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseBrowseNextResponse(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseTranslateBrowsePathsToNodeIdsRequest(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseTranslateBrowsePathsToNodeIdsResponse(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseQueryFirstRequest(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseQueryFirstResponse(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseQueryNextRequest(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseQueryNextResponse(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseReadRequest(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseReadResponse(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseHistoryReadRequest(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseHistoryReadResponse(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseWriteRequest(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseWriteResponse(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseHistoryUpdateRequest(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseHistoryUpdateResponse(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseCallRequest(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseCallResponse(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseCreateMonitoredItemsRequest(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseCreateMonitoredItemsResponse(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseModifyMonitoredItemsRequest(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseModifyMonitoredItemsResponse(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseSetMonitoringModeRequest(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseSetMonitoringModeResponse(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseSetTriggeringRequest(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseSetTriggeringResponse(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseDeleteMonitoredItemsRequest(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseDeleteMonitoredItemsResponse(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseCreateSubscriptionRequest(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseCreateSubscriptionResponse(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseModifySubscriptionRequest(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseModifySubscriptionResponse(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseSetPublishingModeRequest(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseSetPublishingModeResponse(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parsePublishRequest(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parsePublishResponse(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseRepublishRequest(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseRepublishResponse(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseTransferSubscriptionsRequest(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseTransferSubscriptionsResponse(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseDeleteSubscriptionsRequest(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void parseDeleteSubscriptionsResponse(proto_tree *tree, tvbuff_t *tvb, gint *pOffset);
void registerServiceTypes(void);
