/* Do not modify this file.                                                   */
/* It is created automatically by the ASN.1 to Wireshark dissector compiler   */
/* packet-t125.c                                                              */
/* ../../tools/asn2wrs.py -b -p t125 -c ./t125.cnf -s ./packet-t125-template -D . -O ../../epan/dissectors MCS-PROTOCOL.asn */

/* Input file: packet-t125-template.c */

#line 1 "../../asn1/t125/packet-t125-template.c"
/* packet-t125.c
 * Routines for t125 packet dissection
 * Copyright 2007, Ronnie Sahlberg
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
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glib.h>
#include <epan/packet.h>

#include <epan/asn1.h>
#include "packet-ber.h"
#include "packet-per.h"

#include "packet-t124.h"

#define PNAME  "MULTIPOINT-COMMUNICATION-SERVICE T.125"
#define PSNAME "T.125"
#define PFNAME "t125"


/* Initialize the protocol and registered fields */
static int proto_t125 = -1;
static proto_tree *top_tree = NULL;

/*--- Included file: packet-t125-hf.c ---*/
#line 1 "../../asn1/t125/packet-t125-hf.c"
static int hf_t125_ConnectMCSPDU_PDU = -1;        /* ConnectMCSPDU */
static int hf_t125_maxChannelIds = -1;            /* INTEGER_0_MAX */
static int hf_t125_maxUserIds = -1;               /* INTEGER_0_MAX */
static int hf_t125_maxTokenIds = -1;              /* INTEGER_0_MAX */
static int hf_t125_numPriorities = -1;            /* INTEGER_0_MAX */
static int hf_t125_minThroughput = -1;            /* INTEGER_0_MAX */
static int hf_t125_maxHeight = -1;                /* INTEGER_0_MAX */
static int hf_t125_maxMCSPDUsize = -1;            /* INTEGER_0_MAX */
static int hf_t125_protocolVersion = -1;          /* INTEGER_0_MAX */
static int hf_t125_callingDomainSelector = -1;    /* OCTET_STRING */
static int hf_t125_calledDomainSelector = -1;     /* OCTET_STRING */
static int hf_t125_upwardFlag = -1;               /* BOOLEAN */
static int hf_t125_targetParameters = -1;         /* DomainParameters */
static int hf_t125_minimumParameters = -1;        /* DomainParameters */
static int hf_t125_maximumParameters = -1;        /* DomainParameters */
static int hf_t125_userData = -1;                 /* T_userData */
static int hf_t125_result = -1;                   /* Result */
static int hf_t125_calledConnectId = -1;          /* INTEGER_0_MAX */
static int hf_t125_domainParameters = -1;         /* DomainParameters */
static int hf_t125_userData_01 = -1;              /* T_userData_01 */
static int hf_t125_dataPriority = -1;             /* DataPriority */
static int hf_t125_static = -1;                   /* T_static */
static int hf_t125_channelId = -1;                /* StaticChannelId */
static int hf_t125_userId = -1;                   /* T_userId */
static int hf_t125_joined = -1;                   /* BOOLEAN */
static int hf_t125_userId_01 = -1;                /* UserId */
static int hf_t125_private = -1;                  /* T_private */
static int hf_t125_channelId_01 = -1;             /* PrivateChannelId */
static int hf_t125_manager = -1;                  /* UserId */
static int hf_t125_admitted = -1;                 /* SET_OF_UserId */
static int hf_t125_admitted_item = -1;            /* UserId */
static int hf_t125_assigned = -1;                 /* T_assigned */
static int hf_t125_channelId_02 = -1;             /* AssignedChannelId */
static int hf_t125_grabbed = -1;                  /* T_grabbed */
static int hf_t125_tokenId = -1;                  /* TokenId */
static int hf_t125_grabber = -1;                  /* UserId */
static int hf_t125_inhibited = -1;                /* T_inhibited */
static int hf_t125_inhibitors = -1;               /* SET_OF_UserId */
static int hf_t125_inhibitors_item = -1;          /* UserId */
static int hf_t125_giving = -1;                   /* T_giving */
static int hf_t125_recipient = -1;                /* UserId */
static int hf_t125_ungivable = -1;                /* T_ungivable */
static int hf_t125_given = -1;                    /* T_given */
static int hf_t125_initiator = -1;                /* UserId */
static int hf_t125_connect_initial = -1;          /* Connect_Initial */
static int hf_t125_connect_response = -1;         /* Connect_Response */
static int hf_t125_connect_additional = -1;       /* Connect_Additional */
static int hf_t125_connect_result = -1;           /* Connect_Result */
/* named bits */
static int hf_t125_Segmentation_begin = -1;
static int hf_t125_Segmentation_end = -1;

/*--- End of included file: packet-t125-hf.c ---*/
#line 49 "../../asn1/t125/packet-t125-template.c"

/* Initialize the subtree pointers */
static int ett_t125 = -1;

static int hf_t125_connectData = -1;
static int hf_t125_heur = -1;


/*--- Included file: packet-t125-ett.c ---*/
#line 1 "../../asn1/t125/packet-t125-ett.c"
static gint ett_t125_Segmentation = -1;
static gint ett_t125_DomainParameters = -1;
static gint ett_t125_Connect_Initial_U = -1;
static gint ett_t125_Connect_Response_U = -1;
static gint ett_t125_Connect_Additional_U = -1;
static gint ett_t125_Connect_Result_U = -1;
static gint ett_t125_ChannelAttributes = -1;
static gint ett_t125_T_static = -1;
static gint ett_t125_T_userId = -1;
static gint ett_t125_T_private = -1;
static gint ett_t125_SET_OF_UserId = -1;
static gint ett_t125_T_assigned = -1;
static gint ett_t125_TokenAttributes = -1;
static gint ett_t125_T_grabbed = -1;
static gint ett_t125_T_inhibited = -1;
static gint ett_t125_T_giving = -1;
static gint ett_t125_T_ungivable = -1;
static gint ett_t125_T_given = -1;
static gint ett_t125_TokenGrabRequest_U = -1;
static gint ett_t125_ConnectMCSPDU = -1;

/*--- End of included file: packet-t125-ett.c ---*/
#line 57 "../../asn1/t125/packet-t125-template.c"

static heur_dissector_list_t t125_heur_subdissector_list;


/*--- Included file: packet-t125-fn.c ---*/
#line 1 "../../asn1/t125/packet-t125-fn.c"


static int
dissect_t125_ChannelId(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx, tree, tvb, offset, hf_index,
                                                NULL);

  return offset;
}



static int
dissect_t125_StaticChannelId(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_t125_ChannelId(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}



static int
dissect_t125_DynamicChannelId(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_t125_ChannelId(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}



static int
dissect_t125_UserId(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_t125_DynamicChannelId(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}



static int
dissect_t125_PrivateChannelId(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_t125_DynamicChannelId(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}



static int
dissect_t125_AssignedChannelId(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_t125_DynamicChannelId(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}



static int
dissect_t125_TokenId(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx, tree, tvb, offset, hf_index,
                                                NULL);

  return offset;
}


static const value_string t125_TokenStatus_vals[] = {
  {   0, "notInUse" },
  {   1, "selfGrabbed" },
  {   2, "otherGrabbed" },
  {   3, "selfInhibited" },
  {   4, "otherInhibited" },
  {   5, "selfRecipient" },
  {   6, "selfGiving" },
  {   7, "otherGiving" },
  { 0, NULL }
};


static int
dissect_t125_TokenStatus(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}


static const value_string t125_DataPriority_vals[] = {
  {   0, "top" },
  {   1, "high" },
  {   2, "medium" },
  {   3, "low" },
  { 0, NULL }
};


static int
dissect_t125_DataPriority(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}


static const asn_namedbit Segmentation_bits[] = {
  {  0, &hf_t125_Segmentation_begin, -1, -1, "begin", NULL },
  {  1, &hf_t125_Segmentation_end, -1, -1, "end", NULL },
  { 0, NULL, 0, 0, NULL, NULL }
};

static int
dissect_t125_Segmentation(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_bitstring(implicit_tag, actx, tree, tvb, offset,
                                    Segmentation_bits, hf_index, ett_t125_Segmentation,
                                    NULL);

  return offset;
}



static int
dissect_t125_INTEGER_0_MAX(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx, tree, tvb, offset, hf_index,
                                                NULL);

  return offset;
}


static const ber_sequence_t DomainParameters_sequence[] = {
  { &hf_t125_maxChannelIds  , BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_t125_INTEGER_0_MAX },
  { &hf_t125_maxUserIds     , BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_t125_INTEGER_0_MAX },
  { &hf_t125_maxTokenIds    , BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_t125_INTEGER_0_MAX },
  { &hf_t125_numPriorities  , BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_t125_INTEGER_0_MAX },
  { &hf_t125_minThroughput  , BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_t125_INTEGER_0_MAX },
  { &hf_t125_maxHeight      , BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_t125_INTEGER_0_MAX },
  { &hf_t125_maxMCSPDUsize  , BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_t125_INTEGER_0_MAX },
  { &hf_t125_protocolVersion, BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_t125_INTEGER_0_MAX },
  { NULL, 0, 0, 0, NULL }
};

static int
dissect_t125_DomainParameters(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   DomainParameters_sequence, hf_index, ett_t125_DomainParameters);

  return offset;
}



static int
dissect_t125_OCTET_STRING(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, actx, tree, tvb, offset, hf_index,
                                       NULL);

  return offset;
}



static int
dissect_t125_BOOLEAN(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_boolean(implicit_tag, actx, tree, tvb, offset, hf_index, NULL);

  return offset;
}



static int
dissect_t125_T_userData(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 87 "../../asn1/t125/t125.cnf"
    tvbuff_t	*next_tvb = NULL;
  offset = dissect_ber_octet_string(implicit_tag, actx, tree, tvb, offset, hf_index,
                                       &next_tvb);

    if(next_tvb) 
    	dissector_try_heuristic(t125_heur_subdissector_list, next_tvb,
	     actx->pinfo, top_tree);


  return offset;
}


static const ber_sequence_t Connect_Initial_U_sequence[] = {
  { &hf_t125_callingDomainSelector, BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_t125_OCTET_STRING },
  { &hf_t125_calledDomainSelector, BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_t125_OCTET_STRING },
  { &hf_t125_upwardFlag     , BER_CLASS_UNI, BER_UNI_TAG_BOOLEAN, BER_FLAGS_NOOWNTAG, dissect_t125_BOOLEAN },
  { &hf_t125_targetParameters, BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_t125_DomainParameters },
  { &hf_t125_minimumParameters, BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_t125_DomainParameters },
  { &hf_t125_maximumParameters, BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_t125_DomainParameters },
  { &hf_t125_userData       , BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_t125_T_userData },
  { NULL, 0, 0, 0, NULL }
};

static int
dissect_t125_Connect_Initial_U(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   Connect_Initial_U_sequence, hf_index, ett_t125_Connect_Initial_U);

  return offset;
}



static int
dissect_t125_Connect_Initial(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_tagged_type(implicit_tag, actx, tree, tvb, offset,
                                      hf_index, BER_CLASS_APP, 101, TRUE, dissect_t125_Connect_Initial_U);

  return offset;
}


static const value_string t125_Result_vals[] = {
  {   0, "rt-successful" },
  {   1, "rt-domain-merging" },
  {   2, "rt-domain-not-hierarchical" },
  {   3, "rt-no-such-channel" },
  {   4, "rt-no-such-domain" },
  {   5, "rt-no-such-user" },
  {   6, "rt-not-admitted" },
  {   7, "rt-other-user-id" },
  {   8, "rt-parameters-unacceptable" },
  {   9, "rt-token-not-available" },
  {  10, "rt-token-not-possessed" },
  {  11, "rt-too-many-channels" },
  {  12, "rt-too-many-tokens" },
  {  13, "rt-too-many-users" },
  {  14, "rt-unspecified-failure" },
  {  15, "rt-user-rejected" },
  { 0, NULL }
};


static int
dissect_t125_Result(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}



static int
dissect_t125_T_userData_01(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 96 "../../asn1/t125/t125.cnf"
    tvbuff_t	*next_tvb = NULL;
  offset = dissect_ber_octet_string(implicit_tag, actx, tree, tvb, offset, hf_index,
                                       &next_tvb);

    if(next_tvb) 
    	dissector_try_heuristic(t125_heur_subdissector_list, next_tvb,
	     actx->pinfo, top_tree);


  return offset;
}


static const ber_sequence_t Connect_Response_U_sequence[] = {
  { &hf_t125_result         , BER_CLASS_UNI, BER_UNI_TAG_ENUMERATED, BER_FLAGS_NOOWNTAG, dissect_t125_Result },
  { &hf_t125_calledConnectId, BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_t125_INTEGER_0_MAX },
  { &hf_t125_domainParameters, BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_t125_DomainParameters },
  { &hf_t125_userData_01    , BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_t125_T_userData_01 },
  { NULL, 0, 0, 0, NULL }
};

static int
dissect_t125_Connect_Response_U(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   Connect_Response_U_sequence, hf_index, ett_t125_Connect_Response_U);

  return offset;
}



static int
dissect_t125_Connect_Response(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_tagged_type(implicit_tag, actx, tree, tvb, offset,
                                      hf_index, BER_CLASS_APP, 102, TRUE, dissect_t125_Connect_Response_U);

  return offset;
}


static const ber_sequence_t Connect_Additional_U_sequence[] = {
  { &hf_t125_calledConnectId, BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_t125_INTEGER_0_MAX },
  { &hf_t125_dataPriority   , BER_CLASS_UNI, BER_UNI_TAG_ENUMERATED, BER_FLAGS_NOOWNTAG, dissect_t125_DataPriority },
  { NULL, 0, 0, 0, NULL }
};

static int
dissect_t125_Connect_Additional_U(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   Connect_Additional_U_sequence, hf_index, ett_t125_Connect_Additional_U);

  return offset;
}



static int
dissect_t125_Connect_Additional(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_tagged_type(implicit_tag, actx, tree, tvb, offset,
                                      hf_index, BER_CLASS_APP, 103, TRUE, dissect_t125_Connect_Additional_U);

  return offset;
}


static const ber_sequence_t Connect_Result_U_sequence[] = {
  { &hf_t125_result         , BER_CLASS_UNI, BER_UNI_TAG_ENUMERATED, BER_FLAGS_NOOWNTAG, dissect_t125_Result },
  { NULL, 0, 0, 0, NULL }
};

static int
dissect_t125_Connect_Result_U(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   Connect_Result_U_sequence, hf_index, ett_t125_Connect_Result_U);

  return offset;
}



static int
dissect_t125_Connect_Result(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_tagged_type(implicit_tag, actx, tree, tvb, offset,
                                      hf_index, BER_CLASS_APP, 104, TRUE, dissect_t125_Connect_Result_U);

  return offset;
}


static const ber_sequence_t T_static_sequence[] = {
  { &hf_t125_channelId      , BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_t125_StaticChannelId },
  { NULL, 0, 0, 0, NULL }
};

static int
dissect_t125_T_static(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   T_static_sequence, hf_index, ett_t125_T_static);

  return offset;
}


static const ber_sequence_t T_userId_sequence[] = {
  { &hf_t125_joined         , BER_CLASS_UNI, BER_UNI_TAG_BOOLEAN, BER_FLAGS_NOOWNTAG, dissect_t125_BOOLEAN },
  { &hf_t125_userId_01      , BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_t125_UserId },
  { NULL, 0, 0, 0, NULL }
};

static int
dissect_t125_T_userId(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   T_userId_sequence, hf_index, ett_t125_T_userId);

  return offset;
}


static const ber_sequence_t SET_OF_UserId_set_of[1] = {
  { &hf_t125_admitted_item  , BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_t125_UserId },
};

static int
dissect_t125_SET_OF_UserId(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_set_of(implicit_tag, actx, tree, tvb, offset,
                                 SET_OF_UserId_set_of, hf_index, ett_t125_SET_OF_UserId);

  return offset;
}


static const ber_sequence_t T_private_sequence[] = {
  { &hf_t125_joined         , BER_CLASS_UNI, BER_UNI_TAG_BOOLEAN, BER_FLAGS_NOOWNTAG, dissect_t125_BOOLEAN },
  { &hf_t125_channelId_01   , BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_t125_PrivateChannelId },
  { &hf_t125_manager        , BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_t125_UserId },
  { &hf_t125_admitted       , BER_CLASS_UNI, BER_UNI_TAG_SET, BER_FLAGS_NOOWNTAG, dissect_t125_SET_OF_UserId },
  { NULL, 0, 0, 0, NULL }
};

static int
dissect_t125_T_private(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   T_private_sequence, hf_index, ett_t125_T_private);

  return offset;
}


static const ber_sequence_t T_assigned_sequence[] = {
  { &hf_t125_channelId_02   , BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_t125_AssignedChannelId },
  { NULL, 0, 0, 0, NULL }
};

static int
dissect_t125_T_assigned(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   T_assigned_sequence, hf_index, ett_t125_T_assigned);

  return offset;
}


static const value_string t125_ChannelAttributes_vals[] = {
  {   0, "static" },
  {   1, "userId" },
  {   2, "private" },
  {   3, "assigned" },
  { 0, NULL }
};

static const ber_choice_t ChannelAttributes_choice[] = {
  {   0, &hf_t125_static         , BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_t125_T_static },
  {   1, &hf_t125_userId         , BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_t125_T_userId },
  {   2, &hf_t125_private        , BER_CLASS_CON, 2, BER_FLAGS_IMPLTAG, dissect_t125_T_private },
  {   3, &hf_t125_assigned       , BER_CLASS_CON, 3, BER_FLAGS_IMPLTAG, dissect_t125_T_assigned },
  { 0, NULL, 0, 0, 0, NULL }
};

static int
dissect_t125_ChannelAttributes(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_choice(actx, tree, tvb, offset,
                                 ChannelAttributes_choice, hf_index, ett_t125_ChannelAttributes,
                                 NULL);

  return offset;
}


static const ber_sequence_t T_grabbed_sequence[] = {
  { &hf_t125_tokenId        , BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_t125_TokenId },
  { &hf_t125_grabber        , BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_t125_UserId },
  { NULL, 0, 0, 0, NULL }
};

static int
dissect_t125_T_grabbed(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   T_grabbed_sequence, hf_index, ett_t125_T_grabbed);

  return offset;
}


static const ber_sequence_t T_inhibited_sequence[] = {
  { &hf_t125_tokenId        , BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_t125_TokenId },
  { &hf_t125_inhibitors     , BER_CLASS_UNI, BER_UNI_TAG_SET, BER_FLAGS_NOOWNTAG, dissect_t125_SET_OF_UserId },
  { NULL, 0, 0, 0, NULL }
};

static int
dissect_t125_T_inhibited(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   T_inhibited_sequence, hf_index, ett_t125_T_inhibited);

  return offset;
}


static const ber_sequence_t T_giving_sequence[] = {
  { &hf_t125_tokenId        , BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_t125_TokenId },
  { &hf_t125_grabber        , BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_t125_UserId },
  { &hf_t125_recipient      , BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_t125_UserId },
  { NULL, 0, 0, 0, NULL }
};

static int
dissect_t125_T_giving(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   T_giving_sequence, hf_index, ett_t125_T_giving);

  return offset;
}


static const ber_sequence_t T_ungivable_sequence[] = {
  { &hf_t125_tokenId        , BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_t125_TokenId },
  { &hf_t125_grabber        , BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_t125_UserId },
  { NULL, 0, 0, 0, NULL }
};

static int
dissect_t125_T_ungivable(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   T_ungivable_sequence, hf_index, ett_t125_T_ungivable);

  return offset;
}


static const ber_sequence_t T_given_sequence[] = {
  { &hf_t125_tokenId        , BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_t125_TokenId },
  { &hf_t125_recipient      , BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_t125_UserId },
  { NULL, 0, 0, 0, NULL }
};

static int
dissect_t125_T_given(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   T_given_sequence, hf_index, ett_t125_T_given);

  return offset;
}


static const value_string t125_TokenAttributes_vals[] = {
  {   0, "grabbed" },
  {   1, "inhibited" },
  {   2, "giving" },
  {   3, "ungivable" },
  {   4, "given" },
  { 0, NULL }
};

static const ber_choice_t TokenAttributes_choice[] = {
  {   0, &hf_t125_grabbed        , BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_t125_T_grabbed },
  {   1, &hf_t125_inhibited      , BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_t125_T_inhibited },
  {   2, &hf_t125_giving         , BER_CLASS_CON, 2, BER_FLAGS_IMPLTAG, dissect_t125_T_giving },
  {   3, &hf_t125_ungivable      , BER_CLASS_CON, 3, BER_FLAGS_IMPLTAG, dissect_t125_T_ungivable },
  {   4, &hf_t125_given          , BER_CLASS_CON, 4, BER_FLAGS_IMPLTAG, dissect_t125_T_given },
  { 0, NULL, 0, 0, 0, NULL }
};

static int
dissect_t125_TokenAttributes(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_choice(actx, tree, tvb, offset,
                                 TokenAttributes_choice, hf_index, ett_t125_TokenAttributes,
                                 NULL);

  return offset;
}


static const ber_sequence_t TokenGrabRequest_U_sequence[] = {
  { &hf_t125_initiator      , BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_t125_UserId },
  { &hf_t125_tokenId        , BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_t125_TokenId },
  { NULL, 0, 0, 0, NULL }
};

static int
dissect_t125_TokenGrabRequest_U(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   TokenGrabRequest_U_sequence, hf_index, ett_t125_TokenGrabRequest_U);

  return offset;
}



static int
dissect_t125_TokenGrabRequest(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_tagged_type(implicit_tag, actx, tree, tvb, offset,
                                      hf_index, BER_CLASS_APP, 29, TRUE, dissect_t125_TokenGrabRequest_U);

  return offset;
}


static const value_string t125_Reason_vals[] = {
  {   0, "rn-domain-disconnected" },
  {   1, "rn-provider-initiated" },
  {   2, "rn-token-purged" },
  {   3, "rn-user-requested" },
  {   4, "rn-channel-purged" },
  { 0, NULL }
};


static int
dissect_t125_Reason(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}


static const value_string t125_Diagnostic_vals[] = {
  {   0, "dc-inconsistent-merge" },
  {   1, "dc-forbidden-PDU-downward" },
  {   2, "dc-forbidden-PDU-upward" },
  {   3, "dc-invalid-BER-encoding" },
  {   4, "dc-invalid-PER-encoding" },
  {   5, "dc-misrouted-user" },
  {   6, "dc-unrequested-confirm" },
  {   7, "dc-wrong-transport-priority" },
  {   8, "dc-channel-id-conflict" },
  {   9, "dc-token-id-conflict" },
  {  10, "dc-not-user-id-channel" },
  {  11, "dc-too-many-channels" },
  {  12, "dc-too-many-tokens" },
  {  13, "dc-too-many-users" },
  { 0, NULL }
};


static int
dissect_t125_Diagnostic(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}


static const value_string t125_ConnectMCSPDU_vals[] = {
  { 101, "connect-initial" },
  { 102, "connect-response" },
  { 103, "connect-additional" },
  { 104, "connect-result" },
  { 0, NULL }
};

static const ber_choice_t ConnectMCSPDU_choice[] = {
  { 101, &hf_t125_connect_initial, BER_CLASS_APP, 101, BER_FLAGS_NOOWNTAG, dissect_t125_Connect_Initial },
  { 102, &hf_t125_connect_response, BER_CLASS_APP, 102, BER_FLAGS_NOOWNTAG, dissect_t125_Connect_Response },
  { 103, &hf_t125_connect_additional, BER_CLASS_APP, 103, BER_FLAGS_NOOWNTAG, dissect_t125_Connect_Additional },
  { 104, &hf_t125_connect_result , BER_CLASS_APP, 104, BER_FLAGS_NOOWNTAG, dissect_t125_Connect_Result },
  { 0, NULL, 0, 0, 0, NULL }
};

static int
dissect_t125_ConnectMCSPDU(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_choice(actx, tree, tvb, offset,
                                 ConnectMCSPDU_choice, hf_index, ett_t125_ConnectMCSPDU,
                                 NULL);

  return offset;
}

/*--- PDUs ---*/

static int dissect_ConnectMCSPDU_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  offset = dissect_t125_ConnectMCSPDU(FALSE, tvb, offset, &asn1_ctx, tree, hf_t125_ConnectMCSPDU_PDU);
  return offset;
}


/*--- End of included file: packet-t125-fn.c ---*/
#line 61 "../../asn1/t125/packet-t125-template.c"

static int
dissect_t125(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *parent_tree)
{
  proto_item *item = NULL;
  proto_tree *tree = NULL;
  gint8 class;
  gboolean pc;
  gint32 tag;

  top_tree = parent_tree;

  col_set_str(pinfo->cinfo, COL_PROTOCOL, "T.125");
  col_clear(pinfo->cinfo, COL_INFO);

  item = proto_tree_add_item(parent_tree, proto_t125, tvb, 0, tvb_length(tvb), ENC_NA);
  tree = proto_item_add_subtree(item, ett_t125);

  get_ber_identifier(tvb, 0, &class, &pc, &tag);

  if ( (class==BER_CLASS_APP) && (tag>=101) && (tag<=104) ){
    dissect_ConnectMCSPDU_PDU(tvb, pinfo, tree);
  } else  {
    t124_set_top_tree(top_tree);
    dissect_DomainMCSPDU_PDU(tvb, pinfo, tree);
  }

  return tvb_length(tvb);
}

static gboolean
dissect_t125_heur(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *parent_tree)
{
  gint8 class;
  gboolean pc;
  gint32 tag;
  guint32 choice_index = 100;
  asn1_ctx_t asn1_ctx;

  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);

  /* could be BER */
  get_ber_identifier(tvb, 0, &class, &pc, &tag);
  /* or PER */
  dissect_per_constrained_integer(tvb, 0, &asn1_ctx,
				  NULL, hf_t125_heur, 0, 42,
				  &choice_index, FALSE);

  /* is this strong enough ? */
  if ( ((class==BER_CLASS_APP) && ((tag>=101) && (tag<=104))) ||
       (choice_index <=42)) {

    dissect_t125(tvb, pinfo, parent_tree);

    return TRUE;
  }

  return FALSE;
}


/*--- proto_register_t125 -------------------------------------------*/
void proto_register_t125(void) {

  /* List of fields */
  static hf_register_info hf[] = {
    { &hf_t125_connectData,
      { "connectData", "t125.connectData",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_t125_heur,
      { "heuristic", "t125.heuristic",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},

/*--- Included file: packet-t125-hfarr.c ---*/
#line 1 "../../asn1/t125/packet-t125-hfarr.c"
    { &hf_t125_ConnectMCSPDU_PDU,
      { "ConnectMCSPDU", "t125.ConnectMCSPDU",
        FT_UINT32, BASE_DEC, VALS(t125_ConnectMCSPDU_vals), 0,
        NULL, HFILL }},
    { &hf_t125_maxChannelIds,
      { "maxChannelIds", "t125.maxChannelIds",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_MAX", HFILL }},
    { &hf_t125_maxUserIds,
      { "maxUserIds", "t125.maxUserIds",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_MAX", HFILL }},
    { &hf_t125_maxTokenIds,
      { "maxTokenIds", "t125.maxTokenIds",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_MAX", HFILL }},
    { &hf_t125_numPriorities,
      { "numPriorities", "t125.numPriorities",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_MAX", HFILL }},
    { &hf_t125_minThroughput,
      { "minThroughput", "t125.minThroughput",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_MAX", HFILL }},
    { &hf_t125_maxHeight,
      { "maxHeight", "t125.maxHeight",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_MAX", HFILL }},
    { &hf_t125_maxMCSPDUsize,
      { "maxMCSPDUsize", "t125.maxMCSPDUsize",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_MAX", HFILL }},
    { &hf_t125_protocolVersion,
      { "protocolVersion", "t125.protocolVersion",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_MAX", HFILL }},
    { &hf_t125_callingDomainSelector,
      { "callingDomainSelector", "t125.callingDomainSelector",
        FT_BYTES, BASE_NONE, NULL, 0,
        "OCTET_STRING", HFILL }},
    { &hf_t125_calledDomainSelector,
      { "calledDomainSelector", "t125.calledDomainSelector",
        FT_BYTES, BASE_NONE, NULL, 0,
        "OCTET_STRING", HFILL }},
    { &hf_t125_upwardFlag,
      { "upwardFlag", "t125.upwardFlag",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_t125_targetParameters,
      { "targetParameters", "t125.targetParameters",
        FT_NONE, BASE_NONE, NULL, 0,
        "DomainParameters", HFILL }},
    { &hf_t125_minimumParameters,
      { "minimumParameters", "t125.minimumParameters",
        FT_NONE, BASE_NONE, NULL, 0,
        "DomainParameters", HFILL }},
    { &hf_t125_maximumParameters,
      { "maximumParameters", "t125.maximumParameters",
        FT_NONE, BASE_NONE, NULL, 0,
        "DomainParameters", HFILL }},
    { &hf_t125_userData,
      { "userData", "t125.userData",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_t125_result,
      { "result", "t125.result",
        FT_UINT32, BASE_DEC, VALS(t125_Result_vals), 0,
        NULL, HFILL }},
    { &hf_t125_calledConnectId,
      { "calledConnectId", "t125.calledConnectId",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_MAX", HFILL }},
    { &hf_t125_domainParameters,
      { "domainParameters", "t125.domainParameters",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_t125_userData_01,
      { "userData", "t125.userData",
        FT_BYTES, BASE_NONE, NULL, 0,
        "T_userData_01", HFILL }},
    { &hf_t125_dataPriority,
      { "dataPriority", "t125.dataPriority",
        FT_UINT32, BASE_DEC, VALS(t125_DataPriority_vals), 0,
        NULL, HFILL }},
    { &hf_t125_static,
      { "static", "t125.static",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_t125_channelId,
      { "channelId", "t125.channelId",
        FT_UINT32, BASE_DEC, NULL, 0,
        "StaticChannelId", HFILL }},
    { &hf_t125_userId,
      { "userId", "t125.userId",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_t125_joined,
      { "joined", "t125.joined",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_t125_userId_01,
      { "userId", "t125.userId",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_t125_private,
      { "private", "t125.private",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_t125_channelId_01,
      { "channelId", "t125.channelId",
        FT_UINT32, BASE_DEC, NULL, 0,
        "PrivateChannelId", HFILL }},
    { &hf_t125_manager,
      { "manager", "t125.manager",
        FT_UINT32, BASE_DEC, NULL, 0,
        "UserId", HFILL }},
    { &hf_t125_admitted,
      { "admitted", "t125.admitted",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SET_OF_UserId", HFILL }},
    { &hf_t125_admitted_item,
      { "UserId", "t125.UserId",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_t125_assigned,
      { "assigned", "t125.assigned",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_t125_channelId_02,
      { "channelId", "t125.channelId",
        FT_UINT32, BASE_DEC, NULL, 0,
        "AssignedChannelId", HFILL }},
    { &hf_t125_grabbed,
      { "grabbed", "t125.grabbed",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_t125_tokenId,
      { "tokenId", "t125.tokenId",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_t125_grabber,
      { "grabber", "t125.grabber",
        FT_UINT32, BASE_DEC, NULL, 0,
        "UserId", HFILL }},
    { &hf_t125_inhibited,
      { "inhibited", "t125.inhibited",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_t125_inhibitors,
      { "inhibitors", "t125.inhibitors",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SET_OF_UserId", HFILL }},
    { &hf_t125_inhibitors_item,
      { "UserId", "t125.UserId",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_t125_giving,
      { "giving", "t125.giving",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_t125_recipient,
      { "recipient", "t125.recipient",
        FT_UINT32, BASE_DEC, NULL, 0,
        "UserId", HFILL }},
    { &hf_t125_ungivable,
      { "ungivable", "t125.ungivable",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_t125_given,
      { "given", "t125.given",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_t125_initiator,
      { "initiator", "t125.initiator",
        FT_UINT32, BASE_DEC, NULL, 0,
        "UserId", HFILL }},
    { &hf_t125_connect_initial,
      { "connect-initial", "t125.connect_initial",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_t125_connect_response,
      { "connect-response", "t125.connect_response",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_t125_connect_additional,
      { "connect-additional", "t125.connect_additional",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_t125_connect_result,
      { "connect-result", "t125.connect_result",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_t125_Segmentation_begin,
      { "begin", "t125.begin",
        FT_BOOLEAN, 8, NULL, 0x80,
        NULL, HFILL }},
    { &hf_t125_Segmentation_end,
      { "end", "t125.end",
        FT_BOOLEAN, 8, NULL, 0x40,
        NULL, HFILL }},

/*--- End of included file: packet-t125-hfarr.c ---*/
#line 136 "../../asn1/t125/packet-t125-template.c"
  };

  /* List of subtrees */
  static gint *ett[] = {
	  &ett_t125,

/*--- Included file: packet-t125-ettarr.c ---*/
#line 1 "../../asn1/t125/packet-t125-ettarr.c"
    &ett_t125_Segmentation,
    &ett_t125_DomainParameters,
    &ett_t125_Connect_Initial_U,
    &ett_t125_Connect_Response_U,
    &ett_t125_Connect_Additional_U,
    &ett_t125_Connect_Result_U,
    &ett_t125_ChannelAttributes,
    &ett_t125_T_static,
    &ett_t125_T_userId,
    &ett_t125_T_private,
    &ett_t125_SET_OF_UserId,
    &ett_t125_T_assigned,
    &ett_t125_TokenAttributes,
    &ett_t125_T_grabbed,
    &ett_t125_T_inhibited,
    &ett_t125_T_giving,
    &ett_t125_T_ungivable,
    &ett_t125_T_given,
    &ett_t125_TokenGrabRequest_U,
    &ett_t125_ConnectMCSPDU,

/*--- End of included file: packet-t125-ettarr.c ---*/
#line 142 "../../asn1/t125/packet-t125-template.c"
  };

  /* Register protocol */
  proto_t125 = proto_register_protocol(PNAME, PSNAME, PFNAME);
  /* Register fields and subtrees */
  proto_register_field_array(proto_t125, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));

  register_heur_dissector_list("t125", &t125_heur_subdissector_list);

  new_register_dissector("t125", dissect_t125, proto_t125);
}


/*--- proto_reg_handoff_t125 ---------------------------------------*/
void proto_reg_handoff_t125(void) {

  heur_dissector_add("cotp", dissect_t125_heur, proto_t125);
  heur_dissector_add("cotp_is", dissect_t125_heur, proto_t125);
}
