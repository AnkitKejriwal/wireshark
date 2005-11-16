/* Do not modify this file.                                                   */
/* It is created automatically by the ASN.1 to Ethereal dissector compiler    */
/* ./packet-disp.c                                                            */
/* ../../tools/asn2eth.py -X -b -e -p disp -c disp.cnf -s packet-disp-template disp.asn */

/* Input file: packet-disp-template.c */

/* packet-disp.c
 * Routines for X.525 (X.500 Directory Shadow Asbtract Service) and X.519 DISP packet dissection
 * Graeme Lunt 2005
 *
 * $Id: packet-disp-template.c 14773 2005-06-26 10:59:15Z etxrab $
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
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glib.h>
#include <epan/packet.h>
#include <epan/prefs.h>
#include <epan/conversation.h>

#include <stdio.h>
#include <string.h>

#include "packet-ber.h"
#include "packet-acse.h"
#include "packet-ros.h"
#include "packet-rtse.h"

#include "packet-x509if.h"
#include "packet-x509af.h"
#include "packet-x509sat.h"
#include "packet-crmf.h"

#include "packet-x501.h"
#include "packet-dap.h"
#include "packet-dsp.h"
#include "packet-disp.h"
#include "packet-acse.h"


/* we don't have a separate dissector for X519 - 
   and most of DISP is defined in X525 */
#define PNAME  "X.519 Directory Information Shadowing Protocol"
#define PSNAME "DISP"
#define PFNAME "disp"

static guint global_disp_tcp_port = 102;
static guint tcp_port = 0;
static dissector_handle_t tpkt_handle = NULL;
void prefs_register_disp(void); /* forwad declaration for use in preferences registration */


/* Initialize the protocol and registered fields */
int proto_disp = -1;

static struct SESSION_DATA_STRUCTURE* session = NULL;


/*--- Included file: packet-disp-hf.c ---*/

static int hf_disp_modifiedSecondaryShadows = -1;  /* SET_OF_SupplierAndConsumers */
static int hf_disp_secondaryShadows_item = -1;    /* SupplierAndConsumers */
static int hf_disp_shadowSubject = -1;            /* UnitOfReplication */
static int hf_disp_updateMode = -1;               /* UpdateMode */
static int hf_disp_master = -1;                   /* AccessPoint */
static int hf_disp_secondaryShadows = -1;         /* BOOLEAN */
static int hf_disp_area = -1;                     /* AreaSpecification */
static int hf_disp_replication_attributes = -1;   /* AttributeSelection */
static int hf_disp_knowledge = -1;                /* Knowledge */
static int hf_disp_subordinates = -1;             /* BOOLEAN */
static int hf_disp_contextSelection = -1;         /* ContextSelection */
static int hf_disp_supplyContexts = -1;           /* T_supplyContexts */
static int hf_disp_allContexts = -1;              /* NULL */
static int hf_disp_selectedContexts = -1;         /* T_selectedContexts */
static int hf_disp_selectedContexts_item = -1;    /* OBJECT_IDENTIFIER */
static int hf_disp_contextPrefix = -1;            /* DistinguishedName */
static int hf_disp_replicationArea = -1;          /* SubtreeSpecification */
static int hf_disp_knowledgeType = -1;            /* T_knowledgeType */
static int hf_disp_extendedKnowledge = -1;        /* BOOLEAN */
static int hf_disp_AttributeSelection_item = -1;  /* ClassAttributeSelection */
static int hf_disp_class = -1;                    /* OBJECT_IDENTIFIER */
static int hf_disp_classAttributes = -1;          /* ClassAttributes */
static int hf_disp_allAttributes = -1;            /* NULL */
static int hf_disp_include = -1;                  /* AttributeTypes */
static int hf_disp_exclude = -1;                  /* AttributeTypes */
static int hf_disp_AttributeTypes_item = -1;      /* AttributeType */
static int hf_disp_supplierInitiated = -1;        /* SupplierUpdateMode */
static int hf_disp_consumerInitiated = -1;        /* ConsumerUpdateMode */
static int hf_disp_onChange = -1;                 /* BOOLEAN */
static int hf_disp_scheduled = -1;                /* SchedulingParameters */
static int hf_disp_periodic = -1;                 /* PeriodicStrategy */
static int hf_disp_othertimes = -1;               /* BOOLEAN */
static int hf_disp_beginTime = -1;                /* Time */
static int hf_disp_windowSize = -1;               /* INTEGER */
static int hf_disp_updateInterval = -1;           /* INTEGER */
static int hf_disp_agreementID = -1;              /* AgreementID */
static int hf_disp_lastUpdate = -1;               /* Time */
static int hf_disp_updateStrategy = -1;           /* T_updateStrategy */
static int hf_disp_standardUpdate = -1;           /* StandardUpdate */
static int hf_disp_other = -1;                    /* EXTERNAL */
static int hf_disp_securityParameters = -1;       /* SecurityParameters */
static int hf_disp_unsignedCoordinateShadowUpdateArgument = -1;  /* CoordinateShadowUpdateArgumentData */
static int hf_disp_signedCoordinateShadowUpdateArgument = -1;  /* T_signedCoordinateShadowUpdateArgument */
static int hf_disp_coordinateShadowUpdateArgument = -1;  /* CoordinateShadowUpdateArgumentData */
static int hf_disp_algorithmIdentifier = -1;      /* AlgorithmIdentifier */
static int hf_disp_encrypted = -1;                /* BIT_STRING */
static int hf_disp_null = -1;                     /* NULL */
static int hf_disp_information = -1;              /* Information */
static int hf_disp_performer = -1;                /* DistinguishedName */
static int hf_disp_aliasDereferenced = -1;        /* BOOLEAN */
static int hf_disp_notification = -1;             /* SEQUENCE_OF_Attribute */
static int hf_disp_notification_item = -1;        /* Attribute */
static int hf_disp_unsignedInformation = -1;      /* InformationData */
static int hf_disp_signedInformation = -1;        /* T_signedInformation */
static int hf_disp_information_data = -1;         /* InformationData */
static int hf_disp_requestedStrategy = -1;        /* T_requestedStrategy */
static int hf_disp_standard = -1;                 /* T_standard */
static int hf_disp_unsignedRequestShadowUpdateArgument = -1;  /* RequestShadowUpdateArgumentData */
static int hf_disp_signedRequestShadowUpdateArgument = -1;  /* T_signedRequestShadowUpdateArgument */
static int hf_disp_requestShadowUpdateArgument = -1;  /* RequestShadowUpdateArgumentData */
static int hf_disp_updateTime = -1;               /* Time */
static int hf_disp_updateWindow = -1;             /* UpdateWindow */
static int hf_disp_updatedInfo = -1;              /* RefreshInformation */
static int hf_disp_unsignedUpdateShadowArgument = -1;  /* UpdateShadowArgumentData */
static int hf_disp_signedUpdateShadowArgument = -1;  /* T_signedUpdateShadowArgument */
static int hf_disp_updateShadowArgument = -1;     /* UpdateShadowArgumentData */
static int hf_disp_start = -1;                    /* Time */
static int hf_disp_stop = -1;                     /* Time */
static int hf_disp_noRefresh = -1;                /* NULL */
static int hf_disp_total = -1;                    /* TotalRefresh */
static int hf_disp_incremental = -1;              /* IncrementalRefresh */
static int hf_disp_otherStrategy = -1;            /* EXTERNAL */
static int hf_disp_sDSE = -1;                     /* SDSEContent */
static int hf_disp_subtree = -1;                  /* SET_OF_Subtree */
static int hf_disp_subtree_item = -1;             /* Subtree */
static int hf_disp_sDSEType = -1;                 /* SDSEType */
static int hf_disp_subComplete = -1;              /* BOOLEAN */
static int hf_disp_attComplete = -1;              /* BOOLEAN */
static int hf_disp_attributes = -1;               /* SET_OF_Attribute */
static int hf_disp_attributes_item = -1;          /* Attribute */
static int hf_disp_attValIncomplete = -1;         /* SET_OF_AttributeType */
static int hf_disp_attValIncomplete_item = -1;    /* AttributeType */
static int hf_disp_rdn = -1;                      /* RelativeDistinguishedName */
static int hf_disp_IncrementalRefresh_item = -1;  /* IncrementalStepRefresh */
static int hf_disp_sDSEChanges = -1;              /* T_sDSEChanges */
static int hf_disp_add = -1;                      /* SDSEContent */
static int hf_disp_remove = -1;                   /* NULL */
static int hf_disp_modify = -1;                   /* ContentChange */
static int hf_disp_subordinateUpdates = -1;       /* SEQUENCE_OF_SubordinateChanges */
static int hf_disp_subordinateUpdates_item = -1;  /* SubordinateChanges */
static int hf_disp_rename = -1;                   /* T_rename */
static int hf_disp_newRDN = -1;                   /* RelativeDistinguishedName */
static int hf_disp_newDN = -1;                    /* DistinguishedName */
static int hf_disp_attributeChanges = -1;         /* T_attributeChanges */
static int hf_disp_replace = -1;                  /* SET_OF_Attribute */
static int hf_disp_replace_item = -1;             /* Attribute */
static int hf_disp_changes = -1;                  /* SEQUENCE_OF_EntryModification */
static int hf_disp_changes_item = -1;             /* EntryModification */
static int hf_disp_subordinate = -1;              /* RelativeDistinguishedName */
static int hf_disp_subordinate_changes = -1;      /* IncrementalStepRefresh */
static int hf_disp_problem = -1;                  /* ShadowProblem */
static int hf_disp_unsignedShadowError = -1;      /* ShadowErrorData */
static int hf_disp_signedShadowError = -1;        /* T_signedShadowError */
static int hf_disp_shadowError = -1;              /* ShadowErrorData */

/*--- End of included file: packet-disp-hf.c ---*/


/* Initialize the subtree pointers */
static gint ett_disp = -1;

/*--- Included file: packet-disp-ett.c ---*/

static gint ett_disp_ModificationParameter = -1;
static gint ett_disp_SET_OF_SupplierAndConsumers = -1;
static gint ett_disp_ShadowingAgreementInfo = -1;
static gint ett_disp_UnitOfReplication = -1;
static gint ett_disp_T_supplyContexts = -1;
static gint ett_disp_T_selectedContexts = -1;
static gint ett_disp_AreaSpecification = -1;
static gint ett_disp_Knowledge = -1;
static gint ett_disp_AttributeSelection = -1;
static gint ett_disp_ClassAttributeSelection = -1;
static gint ett_disp_ClassAttributes = -1;
static gint ett_disp_AttributeTypes = -1;
static gint ett_disp_UpdateMode = -1;
static gint ett_disp_SupplierUpdateMode = -1;
static gint ett_disp_SchedulingParameters = -1;
static gint ett_disp_PeriodicStrategy = -1;
static gint ett_disp_CoordinateShadowUpdateArgumentData = -1;
static gint ett_disp_T_updateStrategy = -1;
static gint ett_disp_CoordinateShadowUpdateArgument = -1;
static gint ett_disp_T_signedCoordinateShadowUpdateArgument = -1;
static gint ett_disp_CoordinateShadowUpdateResult = -1;
static gint ett_disp_InformationData = -1;
static gint ett_disp_SEQUENCE_OF_Attribute = -1;
static gint ett_disp_Information = -1;
static gint ett_disp_T_signedInformation = -1;
static gint ett_disp_RequestShadowUpdateArgumentData = -1;
static gint ett_disp_T_requestedStrategy = -1;
static gint ett_disp_RequestShadowUpdateArgument = -1;
static gint ett_disp_T_signedRequestShadowUpdateArgument = -1;
static gint ett_disp_RequestShadowUpdateResult = -1;
static gint ett_disp_UpdateShadowArgumentData = -1;
static gint ett_disp_UpdateShadowArgument = -1;
static gint ett_disp_T_signedUpdateShadowArgument = -1;
static gint ett_disp_UpdateShadowResult = -1;
static gint ett_disp_UpdateWindow = -1;
static gint ett_disp_RefreshInformation = -1;
static gint ett_disp_TotalRefresh = -1;
static gint ett_disp_SET_OF_Subtree = -1;
static gint ett_disp_SDSEContent = -1;
static gint ett_disp_SET_OF_Attribute = -1;
static gint ett_disp_SET_OF_AttributeType = -1;
static gint ett_disp_Subtree = -1;
static gint ett_disp_IncrementalRefresh = -1;
static gint ett_disp_IncrementalStepRefresh = -1;
static gint ett_disp_T_sDSEChanges = -1;
static gint ett_disp_SEQUENCE_OF_SubordinateChanges = -1;
static gint ett_disp_ContentChange = -1;
static gint ett_disp_T_rename = -1;
static gint ett_disp_T_attributeChanges = -1;
static gint ett_disp_SEQUENCE_OF_EntryModification = -1;
static gint ett_disp_SubordinateChanges = -1;
static gint ett_disp_ShadowErrorData = -1;
static gint ett_disp_ShadowError = -1;
static gint ett_disp_T_signedShadowError = -1;

/*--- End of included file: packet-disp-ett.c ---*/



/*--- Included file: packet-disp-fn.c ---*/

/*--- Cyclic dependencies ---*/

/* Subtree -> Subtree/subtree -> Subtree */
static int dissect_disp_Subtree(gboolean implicit_tag, tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);

static int dissect_subtree_item(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_Subtree(FALSE, tvb, offset, pinfo, tree, hf_disp_subtree_item);
}

/* IncrementalStepRefresh -> IncrementalStepRefresh/subordinateUpdates -> SubordinateChanges -> IncrementalStepRefresh */
static int dissect_disp_IncrementalStepRefresh(gboolean implicit_tag, tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);

static int dissect_IncrementalRefresh_item(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_IncrementalStepRefresh(FALSE, tvb, offset, pinfo, tree, hf_disp_IncrementalRefresh_item);
}
static int dissect_subordinate_changes(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_IncrementalStepRefresh(FALSE, tvb, offset, pinfo, tree, hf_disp_subordinate_changes);
}


/*--- Fields for imported types ---*/

static int dissect_secondaryShadows_item(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_x501_SupplierAndConsumers(FALSE, tvb, offset, pinfo, tree, hf_disp_secondaryShadows_item);
}
static int dissect_master(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_dsp_AccessPoint(FALSE, tvb, offset, pinfo, tree, hf_disp_master);
}
static int dissect_contextSelection(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_dap_ContextSelection(FALSE, tvb, offset, pinfo, tree, hf_disp_contextSelection);
}
static int dissect_contextPrefix(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_x509if_DistinguishedName(FALSE, tvb, offset, pinfo, tree, hf_disp_contextPrefix);
}
static int dissect_replicationArea(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_x509if_SubtreeSpecification(FALSE, tvb, offset, pinfo, tree, hf_disp_replicationArea);
}
static int dissect_AttributeTypes_item(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_x509if_AttributeType(FALSE, tvb, offset, pinfo, tree, hf_disp_AttributeTypes_item);
}
static int dissect_other(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_acse_EXTERNAL(FALSE, tvb, offset, pinfo, tree, hf_disp_other);
}
static int dissect_securityParameters(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_dap_SecurityParameters(FALSE, tvb, offset, pinfo, tree, hf_disp_securityParameters);
}
static int dissect_securityParameters_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_dap_SecurityParameters(TRUE, tvb, offset, pinfo, tree, hf_disp_securityParameters);
}
static int dissect_algorithmIdentifier(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_x509af_AlgorithmIdentifier(FALSE, tvb, offset, pinfo, tree, hf_disp_algorithmIdentifier);
}
static int dissect_performer_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_x509if_DistinguishedName(TRUE, tvb, offset, pinfo, tree, hf_disp_performer);
}
static int dissect_notification_item(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_x509if_Attribute(FALSE, tvb, offset, pinfo, tree, hf_disp_notification_item);
}
static int dissect_otherStrategy(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_acse_EXTERNAL(FALSE, tvb, offset, pinfo, tree, hf_disp_otherStrategy);
}
static int dissect_attributes_item(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_x509if_Attribute(FALSE, tvb, offset, pinfo, tree, hf_disp_attributes_item);
}
static int dissect_attValIncomplete_item(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_x509if_AttributeType(FALSE, tvb, offset, pinfo, tree, hf_disp_attValIncomplete_item);
}
static int dissect_rdn(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_x509if_RelativeDistinguishedName(FALSE, tvb, offset, pinfo, tree, hf_disp_rdn);
}
static int dissect_newRDN(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_x509if_RelativeDistinguishedName(FALSE, tvb, offset, pinfo, tree, hf_disp_newRDN);
}
static int dissect_newDN(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_x509if_DistinguishedName(FALSE, tvb, offset, pinfo, tree, hf_disp_newDN);
}
static int dissect_replace_item(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_x509if_Attribute(FALSE, tvb, offset, pinfo, tree, hf_disp_replace_item);
}
static int dissect_changes_item(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_dap_EntryModification(FALSE, tvb, offset, pinfo, tree, hf_disp_changes_item);
}
static int dissect_subordinate(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_x509if_RelativeDistinguishedName(FALSE, tvb, offset, pinfo, tree, hf_disp_subordinate);
}



static int
dissect_disp_DSAShadowBindArgument(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_dap_DirectoryBindArgument(implicit_tag, tvb, offset, pinfo, tree, hf_index);

  return offset;
}



static int
dissect_disp_DSAShadowBindResult(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_dap_DirectoryBindArgument(implicit_tag, tvb, offset, pinfo, tree, hf_index);

  return offset;
}



static int
dissect_disp_DSAShadowBindError(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_dap_DirectoryBindError(implicit_tag, tvb, offset, pinfo, tree, hf_index);

  return offset;
}


static const ber_sequence_t SET_OF_SupplierAndConsumers_set_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SET, BER_FLAGS_NOOWNTAG, dissect_secondaryShadows_item },
};

static int
dissect_disp_SET_OF_SupplierAndConsumers(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_set_of(implicit_tag, pinfo, tree, tvb, offset,
                                 SET_OF_SupplierAndConsumers_set_of, hf_index, ett_disp_SET_OF_SupplierAndConsumers);

  return offset;
}
static int dissect_modifiedSecondaryShadows(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_SET_OF_SupplierAndConsumers(FALSE, tvb, offset, pinfo, tree, hf_disp_modifiedSecondaryShadows);
}


static const ber_sequence_t ModificationParameter_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SET, BER_FLAGS_NOOWNTAG, dissect_modifiedSecondaryShadows },
  { 0, 0, 0, NULL }
};

static int
dissect_disp_ModificationParameter(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   ModificationParameter_sequence, hf_index, ett_disp_ModificationParameter);

  return offset;
}



static int
dissect_disp_AgreementID(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_dap_OperationalBindingID(implicit_tag, tvb, offset, pinfo, tree, hf_index);

  return offset;
}
static int dissect_agreementID(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_AgreementID(FALSE, tvb, offset, pinfo, tree, hf_disp_agreementID);
}


static const ber_sequence_t AreaSpecification_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_contextPrefix },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_replicationArea },
  { 0, 0, 0, NULL }
};

static int
dissect_disp_AreaSpecification(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   AreaSpecification_sequence, hf_index, ett_disp_AreaSpecification);

  return offset;
}
static int dissect_area(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_AreaSpecification(FALSE, tvb, offset, pinfo, tree, hf_disp_area);
}



static int
dissect_disp_OBJECT_IDENTIFIER(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_object_identifier(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_selectedContexts_item(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_OBJECT_IDENTIFIER(FALSE, tvb, offset, pinfo, tree, hf_disp_selectedContexts_item);
}
static int dissect_class(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_OBJECT_IDENTIFIER(FALSE, tvb, offset, pinfo, tree, hf_disp_class);
}



static int
dissect_disp_NULL(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_null(implicit_tag, pinfo, tree, tvb, offset, hf_index);

  return offset;
}
static int dissect_allContexts(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_NULL(FALSE, tvb, offset, pinfo, tree, hf_disp_allContexts);
}
static int dissect_allAttributes(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_NULL(FALSE, tvb, offset, pinfo, tree, hf_disp_allAttributes);
}
static int dissect_null(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_NULL(FALSE, tvb, offset, pinfo, tree, hf_disp_null);
}
static int dissect_noRefresh(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_NULL(FALSE, tvb, offset, pinfo, tree, hf_disp_noRefresh);
}
static int dissect_remove(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_NULL(FALSE, tvb, offset, pinfo, tree, hf_disp_remove);
}


static const ber_sequence_t AttributeTypes_set_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_OID, BER_FLAGS_NOOWNTAG, dissect_AttributeTypes_item },
};

static int
dissect_disp_AttributeTypes(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_set_of(implicit_tag, pinfo, tree, tvb, offset,
                                 AttributeTypes_set_of, hf_index, ett_disp_AttributeTypes);

  return offset;
}
static int dissect_include_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_AttributeTypes(TRUE, tvb, offset, pinfo, tree, hf_disp_include);
}
static int dissect_exclude_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_AttributeTypes(TRUE, tvb, offset, pinfo, tree, hf_disp_exclude);
}


static const value_string disp_ClassAttributes_vals[] = {
  {   0, "allAttributes" },
  {   1, "include" },
  {   2, "exclude" },
  { 0, NULL }
};

static const ber_choice_t ClassAttributes_choice[] = {
  {   0, BER_CLASS_UNI, BER_UNI_TAG_NULL, BER_FLAGS_NOOWNTAG, dissect_allAttributes },
  {   1, BER_CLASS_CON, 0, 0, dissect_include_impl },
  {   2, BER_CLASS_CON, 1, 0, dissect_exclude_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_disp_ClassAttributes(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                                 ClassAttributes_choice, hf_index, ett_disp_ClassAttributes,
                                 NULL);

  return offset;
}
static int dissect_classAttributes(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_ClassAttributes(FALSE, tvb, offset, pinfo, tree, hf_disp_classAttributes);
}


static const ber_sequence_t ClassAttributeSelection_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_OID, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_class },
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_classAttributes },
  { 0, 0, 0, NULL }
};

static int
dissect_disp_ClassAttributeSelection(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   ClassAttributeSelection_sequence, hf_index, ett_disp_ClassAttributeSelection);

  return offset;
}
static int dissect_AttributeSelection_item(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_ClassAttributeSelection(FALSE, tvb, offset, pinfo, tree, hf_disp_AttributeSelection_item);
}


static const ber_sequence_t AttributeSelection_set_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_AttributeSelection_item },
};

static int
dissect_disp_AttributeSelection(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_set_of(implicit_tag, pinfo, tree, tvb, offset,
                                 AttributeSelection_set_of, hf_index, ett_disp_AttributeSelection);

  return offset;
}
static int dissect_replication_attributes(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_AttributeSelection(FALSE, tvb, offset, pinfo, tree, hf_disp_replication_attributes);
}


static const value_string disp_T_knowledgeType_vals[] = {
  {   0, "master" },
  {   1, "shadow" },
  {   2, "both" },
  { 0, NULL }
};


static int
dissect_disp_T_knowledgeType(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_knowledgeType(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_T_knowledgeType(FALSE, tvb, offset, pinfo, tree, hf_disp_knowledgeType);
}



static int
dissect_disp_BOOLEAN(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_boolean(implicit_tag, pinfo, tree, tvb, offset, hf_index);

  return offset;
}
static int dissect_secondaryShadows_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_BOOLEAN(TRUE, tvb, offset, pinfo, tree, hf_disp_secondaryShadows);
}
static int dissect_subordinates(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_BOOLEAN(FALSE, tvb, offset, pinfo, tree, hf_disp_subordinates);
}
static int dissect_extendedKnowledge(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_BOOLEAN(FALSE, tvb, offset, pinfo, tree, hf_disp_extendedKnowledge);
}
static int dissect_onChange(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_BOOLEAN(FALSE, tvb, offset, pinfo, tree, hf_disp_onChange);
}
static int dissect_othertimes(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_BOOLEAN(FALSE, tvb, offset, pinfo, tree, hf_disp_othertimes);
}
static int dissect_aliasDereferenced_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_BOOLEAN(TRUE, tvb, offset, pinfo, tree, hf_disp_aliasDereferenced);
}
static int dissect_subComplete_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_BOOLEAN(TRUE, tvb, offset, pinfo, tree, hf_disp_subComplete);
}
static int dissect_attComplete_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_BOOLEAN(TRUE, tvb, offset, pinfo, tree, hf_disp_attComplete);
}


static const ber_sequence_t Knowledge_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_ENUMERATED, BER_FLAGS_NOOWNTAG, dissect_knowledgeType },
  { BER_CLASS_UNI, BER_UNI_TAG_BOOLEAN, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_extendedKnowledge },
  { 0, 0, 0, NULL }
};

static int
dissect_disp_Knowledge(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   Knowledge_sequence, hf_index, ett_disp_Knowledge);

  return offset;
}
static int dissect_knowledge(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_Knowledge(FALSE, tvb, offset, pinfo, tree, hf_disp_knowledge);
}


static const ber_sequence_t T_selectedContexts_set_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_OID, BER_FLAGS_NOOWNTAG, dissect_selectedContexts_item },
};

static int
dissect_disp_T_selectedContexts(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_set_of(implicit_tag, pinfo, tree, tvb, offset,
                                 T_selectedContexts_set_of, hf_index, ett_disp_T_selectedContexts);

  return offset;
}
static int dissect_selectedContexts(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_T_selectedContexts(FALSE, tvb, offset, pinfo, tree, hf_disp_selectedContexts);
}


static const value_string disp_T_supplyContexts_vals[] = {
  {   0, "allContexts" },
  {   1, "selectedContexts" },
  { 0, NULL }
};

static const ber_choice_t T_supplyContexts_choice[] = {
  {   0, BER_CLASS_UNI, BER_UNI_TAG_NULL, BER_FLAGS_NOOWNTAG, dissect_allContexts },
  {   1, BER_CLASS_UNI, BER_UNI_TAG_SET, BER_FLAGS_NOOWNTAG, dissect_selectedContexts },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_disp_T_supplyContexts(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                                 T_supplyContexts_choice, hf_index, ett_disp_T_supplyContexts,
                                 NULL);

  return offset;
}
static int dissect_supplyContexts_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_T_supplyContexts(TRUE, tvb, offset, pinfo, tree, hf_disp_supplyContexts);
}


static const ber_sequence_t UnitOfReplication_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_area },
  { BER_CLASS_UNI, BER_UNI_TAG_SET, BER_FLAGS_NOOWNTAG, dissect_replication_attributes },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_knowledge },
  { BER_CLASS_UNI, BER_UNI_TAG_BOOLEAN, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_subordinates },
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_contextSelection },
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_supplyContexts_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_disp_UnitOfReplication(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   UnitOfReplication_sequence, hf_index, ett_disp_UnitOfReplication);

  return offset;
}
static int dissect_shadowSubject(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_UnitOfReplication(FALSE, tvb, offset, pinfo, tree, hf_disp_shadowSubject);
}



static int
dissect_disp_Time(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_GeneralizedTime(implicit_tag, pinfo, tree, tvb, offset, hf_index);

  return offset;
}
static int dissect_beginTime(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_Time(FALSE, tvb, offset, pinfo, tree, hf_disp_beginTime);
}
static int dissect_lastUpdate(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_Time(FALSE, tvb, offset, pinfo, tree, hf_disp_lastUpdate);
}
static int dissect_updateTime(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_Time(FALSE, tvb, offset, pinfo, tree, hf_disp_updateTime);
}
static int dissect_start(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_Time(FALSE, tvb, offset, pinfo, tree, hf_disp_start);
}
static int dissect_stop(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_Time(FALSE, tvb, offset, pinfo, tree, hf_disp_stop);
}



static int
dissect_disp_INTEGER(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_windowSize(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_INTEGER(FALSE, tvb, offset, pinfo, tree, hf_disp_windowSize);
}
static int dissect_updateInterval(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_INTEGER(FALSE, tvb, offset, pinfo, tree, hf_disp_updateInterval);
}


static const ber_sequence_t PeriodicStrategy_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_GeneralizedTime, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_beginTime },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_windowSize },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_updateInterval },
  { 0, 0, 0, NULL }
};

static int
dissect_disp_PeriodicStrategy(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   PeriodicStrategy_sequence, hf_index, ett_disp_PeriodicStrategy);

  return offset;
}
static int dissect_periodic(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_PeriodicStrategy(FALSE, tvb, offset, pinfo, tree, hf_disp_periodic);
}


static const ber_sequence_t SchedulingParameters_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_periodic },
  { BER_CLASS_UNI, BER_UNI_TAG_BOOLEAN, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_othertimes },
  { 0, 0, 0, NULL }
};

static int
dissect_disp_SchedulingParameters(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   SchedulingParameters_sequence, hf_index, ett_disp_SchedulingParameters);

  return offset;
}
static int dissect_scheduled(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_SchedulingParameters(FALSE, tvb, offset, pinfo, tree, hf_disp_scheduled);
}


static const value_string disp_SupplierUpdateMode_vals[] = {
  {   0, "onChange" },
  {   1, "scheduled" },
  { 0, NULL }
};

static const ber_choice_t SupplierUpdateMode_choice[] = {
  {   0, BER_CLASS_UNI, BER_UNI_TAG_BOOLEAN, BER_FLAGS_NOOWNTAG, dissect_onChange },
  {   1, BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_scheduled },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_disp_SupplierUpdateMode(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                                 SupplierUpdateMode_choice, hf_index, ett_disp_SupplierUpdateMode,
                                 NULL);

  return offset;
}
static int dissect_supplierInitiated_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_SupplierUpdateMode(TRUE, tvb, offset, pinfo, tree, hf_disp_supplierInitiated);
}



static int
dissect_disp_ConsumerUpdateMode(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_disp_SchedulingParameters(implicit_tag, tvb, offset, pinfo, tree, hf_index);

  return offset;
}
static int dissect_consumerInitiated_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_ConsumerUpdateMode(TRUE, tvb, offset, pinfo, tree, hf_disp_consumerInitiated);
}


static const value_string disp_UpdateMode_vals[] = {
  {   0, "supplierInitiated" },
  {   1, "consumerInitiated" },
  { 0, NULL }
};

static const ber_choice_t UpdateMode_choice[] = {
  {   0, BER_CLASS_CON, 0, 0, dissect_supplierInitiated_impl },
  {   1, BER_CLASS_CON, 1, 0, dissect_consumerInitiated_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_disp_UpdateMode(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                                 UpdateMode_choice, hf_index, ett_disp_UpdateMode,
                                 NULL);

  return offset;
}
static int dissect_updateMode(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_UpdateMode(FALSE, tvb, offset, pinfo, tree, hf_disp_updateMode);
}


static const ber_sequence_t ShadowingAgreementInfo_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_shadowSubject },
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_updateMode },
  { BER_CLASS_UNI, BER_UNI_TAG_SET, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_master },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_secondaryShadows_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_disp_ShadowingAgreementInfo(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   ShadowingAgreementInfo_sequence, hf_index, ett_disp_ShadowingAgreementInfo);

  return offset;
}


static const value_string disp_StandardUpdate_vals[] = {
  {   0, "noChanges" },
  {   1, "incremental" },
  {   2, "total" },
  { 0, NULL }
};


static int
dissect_disp_StandardUpdate(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  guint32 update;

    offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  &update);


  if (check_col(pinfo->cinfo, COL_INFO)) {
	col_append_fstr(pinfo->cinfo, COL_INFO, " %s", val_to_str(update, disp_StandardUpdate_vals, "unknown(%d)"));
  }


  return offset;
}
static int dissect_standardUpdate(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_StandardUpdate(FALSE, tvb, offset, pinfo, tree, hf_disp_standardUpdate);
}


static const value_string disp_T_updateStrategy_vals[] = {
  {   0, "standard" },
  {   1, "other" },
  { 0, NULL }
};

static const ber_choice_t T_updateStrategy_choice[] = {
  {   0, BER_CLASS_UNI, BER_UNI_TAG_ENUMERATED, BER_FLAGS_NOOWNTAG, dissect_standardUpdate },
  {   1, BER_CLASS_UNI, 8, BER_FLAGS_NOOWNTAG, dissect_other },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_disp_T_updateStrategy(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                                 T_updateStrategy_choice, hf_index, ett_disp_T_updateStrategy,
                                 NULL);

  return offset;
}
static int dissect_updateStrategy(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_T_updateStrategy(FALSE, tvb, offset, pinfo, tree, hf_disp_updateStrategy);
}


static const ber_sequence_t CoordinateShadowUpdateArgumentData_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_agreementID },
  { BER_CLASS_UNI, BER_UNI_TAG_GeneralizedTime, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_lastUpdate },
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_updateStrategy },
  { BER_CLASS_UNI, BER_UNI_TAG_SET, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_securityParameters },
  { 0, 0, 0, NULL }
};

static int
dissect_disp_CoordinateShadowUpdateArgumentData(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   CoordinateShadowUpdateArgumentData_sequence, hf_index, ett_disp_CoordinateShadowUpdateArgumentData);

  return offset;
}
static int dissect_unsignedCoordinateShadowUpdateArgument_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_CoordinateShadowUpdateArgumentData(TRUE, tvb, offset, pinfo, tree, hf_disp_unsignedCoordinateShadowUpdateArgument);
}
static int dissect_coordinateShadowUpdateArgument_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_CoordinateShadowUpdateArgumentData(TRUE, tvb, offset, pinfo, tree, hf_disp_coordinateShadowUpdateArgument);
}



static int
dissect_disp_BIT_STRING(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_bitstring(implicit_tag, pinfo, tree, tvb, offset,
                                    NULL, hf_index, -1,
                                    NULL);

  return offset;
}
static int dissect_encrypted(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_BIT_STRING(FALSE, tvb, offset, pinfo, tree, hf_disp_encrypted);
}


static const ber_sequence_t T_signedCoordinateShadowUpdateArgument_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_coordinateShadowUpdateArgument_impl },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_algorithmIdentifier },
  { BER_CLASS_UNI, BER_UNI_TAG_BITSTRING, BER_FLAGS_NOOWNTAG, dissect_encrypted },
  { 0, 0, 0, NULL }
};

static int
dissect_disp_T_signedCoordinateShadowUpdateArgument(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   T_signedCoordinateShadowUpdateArgument_sequence, hf_index, ett_disp_T_signedCoordinateShadowUpdateArgument);

  return offset;
}
static int dissect_signedCoordinateShadowUpdateArgument(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_T_signedCoordinateShadowUpdateArgument(FALSE, tvb, offset, pinfo, tree, hf_disp_signedCoordinateShadowUpdateArgument);
}


static const value_string disp_CoordinateShadowUpdateArgument_vals[] = {
  {   0, "unsignedCoordinateShadowUpdateArgument" },
  {   1, "signedCoordinateShadowUpdateArgument" },
  { 0, NULL }
};

static const ber_choice_t CoordinateShadowUpdateArgument_choice[] = {
  {   0, BER_CLASS_CON, 0, 0, dissect_unsignedCoordinateShadowUpdateArgument_impl },
  {   1, BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_signedCoordinateShadowUpdateArgument },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_disp_CoordinateShadowUpdateArgument(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                                 CoordinateShadowUpdateArgument_choice, hf_index, ett_disp_CoordinateShadowUpdateArgument,
                                 NULL);

  return offset;
}


static const ber_sequence_t SEQUENCE_OF_Attribute_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_notification_item },
};

static int
dissect_disp_SEQUENCE_OF_Attribute(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, pinfo, tree, tvb, offset,
                                      SEQUENCE_OF_Attribute_sequence_of, hf_index, ett_disp_SEQUENCE_OF_Attribute);

  return offset;
}
static int dissect_notification_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_SEQUENCE_OF_Attribute(TRUE, tvb, offset, pinfo, tree, hf_disp_notification);
}


static const ber_sequence_t InformationData_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_agreementID },
  { BER_CLASS_UNI, BER_UNI_TAG_GeneralizedTime, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_lastUpdate },
  { BER_CLASS_CON, 30, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_securityParameters_impl },
  { BER_CLASS_CON, 29, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_performer_impl },
  { BER_CLASS_CON, 28, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_aliasDereferenced_impl },
  { BER_CLASS_CON, 27, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_notification_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_disp_InformationData(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   InformationData_sequence, hf_index, ett_disp_InformationData);

  return offset;
}
static int dissect_unsignedInformation_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_InformationData(TRUE, tvb, offset, pinfo, tree, hf_disp_unsignedInformation);
}
static int dissect_information_data_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_InformationData(TRUE, tvb, offset, pinfo, tree, hf_disp_information_data);
}


static const ber_sequence_t T_signedInformation_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_information_data_impl },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_algorithmIdentifier },
  { BER_CLASS_UNI, BER_UNI_TAG_BITSTRING, BER_FLAGS_NOOWNTAG, dissect_encrypted },
  { 0, 0, 0, NULL }
};

static int
dissect_disp_T_signedInformation(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   T_signedInformation_sequence, hf_index, ett_disp_T_signedInformation);

  return offset;
}
static int dissect_signedInformation(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_T_signedInformation(FALSE, tvb, offset, pinfo, tree, hf_disp_signedInformation);
}


static const value_string disp_Information_vals[] = {
  {   0, "unsignedInformation" },
  {   1, "signedInformation" },
  { 0, NULL }
};

static const ber_choice_t Information_choice[] = {
  {   0, BER_CLASS_CON, 0, 0, dissect_unsignedInformation_impl },
  {   1, BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_signedInformation },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_disp_Information(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                                 Information_choice, hf_index, ett_disp_Information,
                                 NULL);

  return offset;
}
static int dissect_information(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_Information(FALSE, tvb, offset, pinfo, tree, hf_disp_information);
}


static const value_string disp_CoordinateShadowUpdateResult_vals[] = {
  {   0, "null" },
  {   1, "information" },
  { 0, NULL }
};

static const ber_choice_t CoordinateShadowUpdateResult_choice[] = {
  {   0, BER_CLASS_UNI, BER_UNI_TAG_NULL, BER_FLAGS_NOOWNTAG, dissect_null },
  {   1, BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG, dissect_information },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_disp_CoordinateShadowUpdateResult(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  guint32 update;

    offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                                 CoordinateShadowUpdateResult_choice, hf_index, ett_disp_CoordinateShadowUpdateResult,
                                 &update);


  if (check_col(pinfo->cinfo, COL_INFO)) {
	col_append_fstr(pinfo->cinfo, COL_INFO, " %s", val_to_str(update, disp_CoordinateShadowUpdateResult_vals, "unknown(%d)"));
  }


  return offset;
}


static const value_string disp_T_standard_vals[] = {
  {   1, "incremental" },
  {   2, "total" },
  { 0, NULL }
};


static int
dissect_disp_T_standard(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  guint32 update;

    offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  &update);


  if (check_col(pinfo->cinfo, COL_INFO)) {
	col_append_fstr(pinfo->cinfo, COL_INFO, " %s", val_to_str(update, disp_T_standard_vals, "standard(%d"));
  }


  return offset;
}
static int dissect_standard(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_T_standard(FALSE, tvb, offset, pinfo, tree, hf_disp_standard);
}


static const value_string disp_T_requestedStrategy_vals[] = {
  {   0, "standard" },
  {   1, "other" },
  { 0, NULL }
};

static const ber_choice_t T_requestedStrategy_choice[] = {
  {   0, BER_CLASS_UNI, BER_UNI_TAG_ENUMERATED, BER_FLAGS_NOOWNTAG, dissect_standard },
  {   1, BER_CLASS_UNI, 8, BER_FLAGS_NOOWNTAG, dissect_other },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_disp_T_requestedStrategy(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                                 T_requestedStrategy_choice, hf_index, ett_disp_T_requestedStrategy,
                                 NULL);

  return offset;
}
static int dissect_requestedStrategy(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_T_requestedStrategy(FALSE, tvb, offset, pinfo, tree, hf_disp_requestedStrategy);
}


static const ber_sequence_t RequestShadowUpdateArgumentData_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_agreementID },
  { BER_CLASS_UNI, BER_UNI_TAG_GeneralizedTime, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_lastUpdate },
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_requestedStrategy },
  { BER_CLASS_UNI, BER_UNI_TAG_SET, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_securityParameters },
  { 0, 0, 0, NULL }
};

static int
dissect_disp_RequestShadowUpdateArgumentData(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   RequestShadowUpdateArgumentData_sequence, hf_index, ett_disp_RequestShadowUpdateArgumentData);

  return offset;
}
static int dissect_unsignedRequestShadowUpdateArgument_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_RequestShadowUpdateArgumentData(TRUE, tvb, offset, pinfo, tree, hf_disp_unsignedRequestShadowUpdateArgument);
}
static int dissect_requestShadowUpdateArgument_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_RequestShadowUpdateArgumentData(TRUE, tvb, offset, pinfo, tree, hf_disp_requestShadowUpdateArgument);
}


static const ber_sequence_t T_signedRequestShadowUpdateArgument_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_requestShadowUpdateArgument_impl },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_algorithmIdentifier },
  { BER_CLASS_UNI, BER_UNI_TAG_BITSTRING, BER_FLAGS_NOOWNTAG, dissect_encrypted },
  { 0, 0, 0, NULL }
};

static int
dissect_disp_T_signedRequestShadowUpdateArgument(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   T_signedRequestShadowUpdateArgument_sequence, hf_index, ett_disp_T_signedRequestShadowUpdateArgument);

  return offset;
}
static int dissect_signedRequestShadowUpdateArgument(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_T_signedRequestShadowUpdateArgument(FALSE, tvb, offset, pinfo, tree, hf_disp_signedRequestShadowUpdateArgument);
}


static const value_string disp_RequestShadowUpdateArgument_vals[] = {
  {   0, "unsignedRequestShadowUpdateArgument" },
  {   1, "signedRequestShadowUpdateArgument" },
  { 0, NULL }
};

static const ber_choice_t RequestShadowUpdateArgument_choice[] = {
  {   0, BER_CLASS_CON, 0, 0, dissect_unsignedRequestShadowUpdateArgument_impl },
  {   1, BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_signedRequestShadowUpdateArgument },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_disp_RequestShadowUpdateArgument(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                                 RequestShadowUpdateArgument_choice, hf_index, ett_disp_RequestShadowUpdateArgument,
                                 NULL);

  return offset;
}


static const value_string disp_RequestShadowUpdateResult_vals[] = {
  {   0, "null" },
  {   1, "information" },
  { 0, NULL }
};

static const ber_choice_t RequestShadowUpdateResult_choice[] = {
  {   0, BER_CLASS_UNI, BER_UNI_TAG_NULL, BER_FLAGS_NOOWNTAG, dissect_null },
  {   1, BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG, dissect_information },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_disp_RequestShadowUpdateResult(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  guint32 update;

    offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                                 RequestShadowUpdateResult_choice, hf_index, ett_disp_RequestShadowUpdateResult,
                                 &update);


  if (check_col(pinfo->cinfo, COL_INFO)) {
	col_append_fstr(pinfo->cinfo, COL_INFO, " %s", val_to_str(update, disp_RequestShadowUpdateResult_vals, "unknown(%d)"));
  }


  return offset;
}


static const ber_sequence_t UpdateWindow_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_GeneralizedTime, BER_FLAGS_NOOWNTAG, dissect_start },
  { BER_CLASS_UNI, BER_UNI_TAG_GeneralizedTime, BER_FLAGS_NOOWNTAG, dissect_stop },
  { 0, 0, 0, NULL }
};

static int
dissect_disp_UpdateWindow(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   UpdateWindow_sequence, hf_index, ett_disp_UpdateWindow);

  return offset;
}
static int dissect_updateWindow(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_UpdateWindow(FALSE, tvb, offset, pinfo, tree, hf_disp_updateWindow);
}



static int
dissect_disp_SDSEType(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_x501_DSEType(implicit_tag, tvb, offset, pinfo, tree, hf_index);

  return offset;
}
static int dissect_sDSEType(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_SDSEType(FALSE, tvb, offset, pinfo, tree, hf_disp_sDSEType);
}


static const ber_sequence_t SET_OF_Attribute_set_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_attributes_item },
};

static int
dissect_disp_SET_OF_Attribute(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_set_of(implicit_tag, pinfo, tree, tvb, offset,
                                 SET_OF_Attribute_set_of, hf_index, ett_disp_SET_OF_Attribute);

  return offset;
}
static int dissect_attributes(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_SET_OF_Attribute(FALSE, tvb, offset, pinfo, tree, hf_disp_attributes);
}
static int dissect_replace_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_SET_OF_Attribute(TRUE, tvb, offset, pinfo, tree, hf_disp_replace);
}


static const ber_sequence_t SET_OF_AttributeType_set_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_OID, BER_FLAGS_NOOWNTAG, dissect_attValIncomplete_item },
};

static int
dissect_disp_SET_OF_AttributeType(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_set_of(implicit_tag, pinfo, tree, tvb, offset,
                                 SET_OF_AttributeType_set_of, hf_index, ett_disp_SET_OF_AttributeType);

  return offset;
}
static int dissect_attValIncomplete(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_SET_OF_AttributeType(FALSE, tvb, offset, pinfo, tree, hf_disp_attValIncomplete);
}


static const ber_sequence_t SDSEContent_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_BITSTRING, BER_FLAGS_NOOWNTAG, dissect_sDSEType },
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_subComplete_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_attComplete_impl },
  { BER_CLASS_UNI, BER_UNI_TAG_SET, BER_FLAGS_NOOWNTAG, dissect_attributes },
  { BER_CLASS_UNI, BER_UNI_TAG_SET, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_attValIncomplete },
  { 0, 0, 0, NULL }
};

static int
dissect_disp_SDSEContent(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   SDSEContent_sequence, hf_index, ett_disp_SDSEContent);

  return offset;
}
static int dissect_sDSE(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_SDSEContent(FALSE, tvb, offset, pinfo, tree, hf_disp_sDSE);
}
static int dissect_add_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_SDSEContent(TRUE, tvb, offset, pinfo, tree, hf_disp_add);
}


static const ber_sequence_t SET_OF_Subtree_set_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_subtree_item },
};

static int
dissect_disp_SET_OF_Subtree(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_set_of(implicit_tag, pinfo, tree, tvb, offset,
                                 SET_OF_Subtree_set_of, hf_index, ett_disp_SET_OF_Subtree);

  return offset;
}
static int dissect_subtree(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_SET_OF_Subtree(FALSE, tvb, offset, pinfo, tree, hf_disp_subtree);
}


static const ber_sequence_t Subtree_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SET, BER_FLAGS_NOOWNTAG, dissect_rdn },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_sDSE },
  { BER_CLASS_UNI, BER_UNI_TAG_SET, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_subtree },
  { 0, 0, 0, NULL }
};

static int
dissect_disp_Subtree(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   Subtree_sequence, hf_index, ett_disp_Subtree);

  return offset;
}


static const ber_sequence_t TotalRefresh_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_sDSE },
  { BER_CLASS_UNI, BER_UNI_TAG_SET, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_subtree },
  { 0, 0, 0, NULL }
};

static int
dissect_disp_TotalRefresh(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   TotalRefresh_sequence, hf_index, ett_disp_TotalRefresh);

  return offset;
}
static int dissect_total_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_TotalRefresh(TRUE, tvb, offset, pinfo, tree, hf_disp_total);
}


static const value_string disp_T_rename_vals[] = {
  {   0, "newRDN" },
  {   1, "newDN" },
  { 0, NULL }
};

static const ber_choice_t T_rename_choice[] = {
  {   0, BER_CLASS_UNI, BER_UNI_TAG_SET, BER_FLAGS_NOOWNTAG, dissect_newRDN },
  {   1, BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_newDN },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_disp_T_rename(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                                 T_rename_choice, hf_index, ett_disp_T_rename,
                                 NULL);

  return offset;
}
static int dissect_rename(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_T_rename(FALSE, tvb, offset, pinfo, tree, hf_disp_rename);
}


static const ber_sequence_t SEQUENCE_OF_EntryModification_sequence_of[1] = {
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG, dissect_changes_item },
};

static int
dissect_disp_SEQUENCE_OF_EntryModification(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, pinfo, tree, tvb, offset,
                                      SEQUENCE_OF_EntryModification_sequence_of, hf_index, ett_disp_SEQUENCE_OF_EntryModification);

  return offset;
}
static int dissect_changes_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_SEQUENCE_OF_EntryModification(TRUE, tvb, offset, pinfo, tree, hf_disp_changes);
}


static const value_string disp_T_attributeChanges_vals[] = {
  {   0, "replace" },
  {   1, "changes" },
  { 0, NULL }
};

static const ber_choice_t T_attributeChanges_choice[] = {
  {   0, BER_CLASS_CON, 0, 0, dissect_replace_impl },
  {   1, BER_CLASS_CON, 1, 0, dissect_changes_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_disp_T_attributeChanges(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                                 T_attributeChanges_choice, hf_index, ett_disp_T_attributeChanges,
                                 NULL);

  return offset;
}
static int dissect_attributeChanges(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_T_attributeChanges(FALSE, tvb, offset, pinfo, tree, hf_disp_attributeChanges);
}


static const ber_sequence_t ContentChange_sequence[] = {
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_rename },
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_attributeChanges },
  { BER_CLASS_UNI, BER_UNI_TAG_BITSTRING, BER_FLAGS_NOOWNTAG, dissect_sDSEType },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_subComplete_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_attComplete_impl },
  { BER_CLASS_UNI, BER_UNI_TAG_SET, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_attValIncomplete },
  { 0, 0, 0, NULL }
};

static int
dissect_disp_ContentChange(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   ContentChange_sequence, hf_index, ett_disp_ContentChange);

  return offset;
}
static int dissect_modify_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_ContentChange(TRUE, tvb, offset, pinfo, tree, hf_disp_modify);
}


static const value_string disp_T_sDSEChanges_vals[] = {
  {   0, "add" },
  {   1, "remove" },
  {   2, "modify" },
  { 0, NULL }
};

static const ber_choice_t T_sDSEChanges_choice[] = {
  {   0, BER_CLASS_CON, 0, 0, dissect_add_impl },
  {   1, BER_CLASS_UNI, BER_UNI_TAG_NULL, BER_FLAGS_NOOWNTAG, dissect_remove },
  {   2, BER_CLASS_CON, 1, 0, dissect_modify_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_disp_T_sDSEChanges(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                                 T_sDSEChanges_choice, hf_index, ett_disp_T_sDSEChanges,
                                 NULL);

  return offset;
}
static int dissect_sDSEChanges(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_T_sDSEChanges(FALSE, tvb, offset, pinfo, tree, hf_disp_sDSEChanges);
}


static const ber_sequence_t SubordinateChanges_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SET, BER_FLAGS_NOOWNTAG, dissect_subordinate },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_subordinate_changes },
  { 0, 0, 0, NULL }
};

static int
dissect_disp_SubordinateChanges(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   SubordinateChanges_sequence, hf_index, ett_disp_SubordinateChanges);

  return offset;
}
static int dissect_subordinateUpdates_item(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_SubordinateChanges(FALSE, tvb, offset, pinfo, tree, hf_disp_subordinateUpdates_item);
}


static const ber_sequence_t SEQUENCE_OF_SubordinateChanges_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_subordinateUpdates_item },
};

static int
dissect_disp_SEQUENCE_OF_SubordinateChanges(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, pinfo, tree, tvb, offset,
                                      SEQUENCE_OF_SubordinateChanges_sequence_of, hf_index, ett_disp_SEQUENCE_OF_SubordinateChanges);

  return offset;
}
static int dissect_subordinateUpdates(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_SEQUENCE_OF_SubordinateChanges(FALSE, tvb, offset, pinfo, tree, hf_disp_subordinateUpdates);
}


static const ber_sequence_t IncrementalStepRefresh_sequence[] = {
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_sDSEChanges },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_subordinateUpdates },
  { 0, 0, 0, NULL }
};

static int
dissect_disp_IncrementalStepRefresh(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   IncrementalStepRefresh_sequence, hf_index, ett_disp_IncrementalStepRefresh);

  return offset;
}


static const ber_sequence_t IncrementalRefresh_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_IncrementalRefresh_item },
};

static int
dissect_disp_IncrementalRefresh(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, pinfo, tree, tvb, offset,
                                      IncrementalRefresh_sequence_of, hf_index, ett_disp_IncrementalRefresh);

  return offset;
}
static int dissect_incremental_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_IncrementalRefresh(TRUE, tvb, offset, pinfo, tree, hf_disp_incremental);
}


static const value_string disp_RefreshInformation_vals[] = {
  {   0, "noRefresh" },
  {   1, "total" },
  {   2, "incremental" },
  {   3, "otherStrategy" },
  { 0, NULL }
};

static const ber_choice_t RefreshInformation_choice[] = {
  {   0, BER_CLASS_UNI, BER_UNI_TAG_NULL, BER_FLAGS_NOOWNTAG, dissect_noRefresh },
  {   1, BER_CLASS_CON, 0, 0, dissect_total_impl },
  {   2, BER_CLASS_CON, 1, 0, dissect_incremental_impl },
  {   3, BER_CLASS_UNI, 8, BER_FLAGS_NOOWNTAG, dissect_otherStrategy },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_disp_RefreshInformation(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  guint32 update;

    offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                                 RefreshInformation_choice, hf_index, ett_disp_RefreshInformation,
                                 &update);


  if (check_col(pinfo->cinfo, COL_INFO)) {
	col_append_fstr(pinfo->cinfo, COL_INFO, " %s", val_to_str(update, disp_RefreshInformation_vals, "unknown(%d)"));
  }


  return offset;
}
static int dissect_updatedInfo(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_RefreshInformation(FALSE, tvb, offset, pinfo, tree, hf_disp_updatedInfo);
}


static const ber_sequence_t UpdateShadowArgumentData_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_agreementID },
  { BER_CLASS_UNI, BER_UNI_TAG_GeneralizedTime, BER_FLAGS_NOOWNTAG, dissect_updateTime },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_updateWindow },
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_updatedInfo },
  { BER_CLASS_UNI, BER_UNI_TAG_SET, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_securityParameters },
  { 0, 0, 0, NULL }
};

static int
dissect_disp_UpdateShadowArgumentData(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   UpdateShadowArgumentData_sequence, hf_index, ett_disp_UpdateShadowArgumentData);

  return offset;
}
static int dissect_unsignedUpdateShadowArgument_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_UpdateShadowArgumentData(TRUE, tvb, offset, pinfo, tree, hf_disp_unsignedUpdateShadowArgument);
}
static int dissect_updateShadowArgument_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_UpdateShadowArgumentData(TRUE, tvb, offset, pinfo, tree, hf_disp_updateShadowArgument);
}


static const ber_sequence_t T_signedUpdateShadowArgument_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_updateShadowArgument_impl },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_algorithmIdentifier },
  { BER_CLASS_UNI, BER_UNI_TAG_BITSTRING, BER_FLAGS_NOOWNTAG, dissect_encrypted },
  { 0, 0, 0, NULL }
};

static int
dissect_disp_T_signedUpdateShadowArgument(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   T_signedUpdateShadowArgument_sequence, hf_index, ett_disp_T_signedUpdateShadowArgument);

  return offset;
}
static int dissect_signedUpdateShadowArgument(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_T_signedUpdateShadowArgument(FALSE, tvb, offset, pinfo, tree, hf_disp_signedUpdateShadowArgument);
}


static const value_string disp_UpdateShadowArgument_vals[] = {
  {   0, "unsignedUpdateShadowArgument" },
  {   1, "signedUpdateShadowArgument" },
  { 0, NULL }
};

static const ber_choice_t UpdateShadowArgument_choice[] = {
  {   0, BER_CLASS_CON, 0, 0, dissect_unsignedUpdateShadowArgument_impl },
  {   1, BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_signedUpdateShadowArgument },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_disp_UpdateShadowArgument(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                                 UpdateShadowArgument_choice, hf_index, ett_disp_UpdateShadowArgument,
                                 NULL);

  return offset;
}


static const value_string disp_UpdateShadowResult_vals[] = {
  {   0, "null" },
  {   1, "information" },
  { 0, NULL }
};

static const ber_choice_t UpdateShadowResult_choice[] = {
  {   0, BER_CLASS_UNI, BER_UNI_TAG_NULL, BER_FLAGS_NOOWNTAG, dissect_null },
  {   1, BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG, dissect_information },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_disp_UpdateShadowResult(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  guint32 update;

    offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                                 UpdateShadowResult_choice, hf_index, ett_disp_UpdateShadowResult,
                                 &update);


  if (check_col(pinfo->cinfo, COL_INFO)) {
	col_append_fstr(pinfo->cinfo, COL_INFO, " %s", val_to_str(update, disp_UpdateShadowResult_vals, "unknown(%d)"));
  }


  return offset;
}


static const value_string disp_ShadowProblem_vals[] = {
  {   1, "invalidAgreementID" },
  {   2, "inactiveAgreement" },
  {   3, "invalidInformationReceived" },
  {   4, "unsupportedStrategy" },
  {   5, "missedPrevious" },
  {   6, "fullUpdateRequired" },
  {   7, "unwillingToPerform" },
  {   8, "unsuitableTiming" },
  {   9, "updateAlreadyReceived" },
  {  10, "invalidSequencing" },
  {  11, "insufficientResources" },
  { 0, NULL }
};


static int
dissect_disp_ShadowProblem(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  guint32 problem;

    offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  &problem);


  if (check_col(pinfo->cinfo, COL_INFO)) {
	col_append_fstr(pinfo->cinfo, COL_INFO, " %s", val_to_str(problem, disp_ShadowProblem_vals, "ShadowProblem(%d)"));
  }


  return offset;
}
static int dissect_problem(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_ShadowProblem(FALSE, tvb, offset, pinfo, tree, hf_disp_problem);
}


static const ber_sequence_t ShadowErrorData_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_problem },
  { BER_CLASS_UNI, BER_UNI_TAG_GeneralizedTime, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_lastUpdate },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_updateWindow },
  { BER_CLASS_CON, 30, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_securityParameters_impl },
  { BER_CLASS_CON, 29, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_performer_impl },
  { BER_CLASS_CON, 28, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_aliasDereferenced_impl },
  { BER_CLASS_CON, 27, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_notification_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_disp_ShadowErrorData(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   ShadowErrorData_sequence, hf_index, ett_disp_ShadowErrorData);

  return offset;
}
static int dissect_unsignedShadowError(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_ShadowErrorData(FALSE, tvb, offset, pinfo, tree, hf_disp_unsignedShadowError);
}
static int dissect_shadowError(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_ShadowErrorData(FALSE, tvb, offset, pinfo, tree, hf_disp_shadowError);
}


static const ber_sequence_t T_signedShadowError_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_shadowError },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_algorithmIdentifier },
  { BER_CLASS_UNI, BER_UNI_TAG_BITSTRING, BER_FLAGS_NOOWNTAG, dissect_encrypted },
  { 0, 0, 0, NULL }
};

static int
dissect_disp_T_signedShadowError(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   T_signedShadowError_sequence, hf_index, ett_disp_T_signedShadowError);

  return offset;
}
static int dissect_signedShadowError_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_disp_T_signedShadowError(TRUE, tvb, offset, pinfo, tree, hf_disp_signedShadowError);
}


static const value_string disp_ShadowError_vals[] = {
  {   0, "unsignedShadowError" },
  {   1, "signedShadowError" },
  { 0, NULL }
};

static const ber_choice_t ShadowError_choice[] = {
  {   0, BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_unsignedShadowError },
  {   1, BER_CLASS_CON, 0, 0, dissect_signedShadowError_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_disp_ShadowError(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                                 ShadowError_choice, hf_index, ett_disp_ShadowError,
                                 NULL);

  return offset;
}


/*--- End of included file: packet-disp-fn.c ---*/


/*
* Dissect DISP PDUs inside a ROS PDUs
*/
static void
dissect_disp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree)
{
	int offset = 0;
	int old_offset;
	proto_item *item=NULL;
	proto_tree *tree=NULL;
	int (*disp_dissector)(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) = NULL;
	char *disp_op_name;

	/* do we have operation information from the ROS dissector?  */
	if( !pinfo->private_data ){
		if(parent_tree){
			proto_tree_add_text(parent_tree, tvb, offset, -1,
				"Internal error: can't get operation information from ROS dissector.");
		} 
		return  ;
	} else {
		session  = ( (struct SESSION_DATA_STRUCTURE*)(pinfo->private_data) );
	}

	if(parent_tree){
		item = proto_tree_add_item(parent_tree, proto_disp, tvb, 0, -1, FALSE);
		tree = proto_item_add_subtree(item, ett_disp);
	}
	if (check_col(pinfo->cinfo, COL_PROTOCOL))
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "DISP");
  	if (check_col(pinfo->cinfo, COL_INFO))
  		col_clear(pinfo->cinfo, COL_INFO);

	switch(session->ros_op & ROS_OP_MASK) {
	case (ROS_OP_BIND | ROS_OP_ARGUMENT):	/*  BindInvoke */
	  disp_dissector = dissect_disp_DSAShadowBindArgument;
	  disp_op_name = "Shadow-Bind-Argument";
	  break;
	case (ROS_OP_BIND | ROS_OP_RESULT):	/*  BindResult */
	  disp_dissector = dissect_disp_DSAShadowBindResult;
	  disp_op_name = "Shadow-Bind-Result";
	  break;
	case (ROS_OP_BIND | ROS_OP_ERROR):	/*  BindError */
	  disp_dissector = dissect_disp_DSAShadowBindError;
	  disp_op_name = "Shadow-Bind-Error";
	  break;
	case (ROS_OP_INVOKE | ROS_OP_ARGUMENT):	/*  Invoke Argument */
	  switch(session->ros_op & ROS_OP_OPCODE_MASK) {
	  case 1: /* requestShadowUpdate */
	    disp_dissector = dissect_disp_RequestShadowUpdateArgument;
	    disp_op_name = "Request-Shadow-Update-Argument";
	    break;
	  case 2: /* updateShadow*/
	    disp_dissector = dissect_disp_UpdateShadowArgument;
	    disp_op_name = "Update-Shadow-Argument";
	    break;
	  case 3: /* coordinateShadowUpdate */
	    disp_dissector = dissect_disp_CoordinateShadowUpdateArgument;
	    disp_op_name = "Coordinate-Shadow-Update-Argument";
	    break;
	  default:
	    proto_tree_add_text(tree, tvb, offset, -1,"Unsupported DISP opcode (%d)",
				session->ros_op & ROS_OP_OPCODE_MASK);
	    break;
	  }
	  break;
	case (ROS_OP_INVOKE | ROS_OP_RESULT):	/*  Return Result */
	  switch(session->ros_op & ROS_OP_OPCODE_MASK) {
	  case 1: /* requestShadowUpdate */
	    disp_dissector = dissect_disp_RequestShadowUpdateResult;
	    disp_op_name = "Request-Shadow-Result";
	    break;
	  case 2: /* updateShadow */
	    disp_dissector = dissect_disp_UpdateShadowResult;
	    disp_op_name = "Update-Shadow-Result";
	    break;
	  case 3: /* coordinateShadowUpdate */
	    disp_dissector = dissect_disp_CoordinateShadowUpdateResult;
	    disp_op_name = "Coordinate-Shadow-Update-Result";
	    break;
	  default:
	    proto_tree_add_text(tree, tvb, offset, -1,"Unsupported DISP opcode");
	    break;
	  }
	  break;
	case (ROS_OP_INVOKE | ROS_OP_ERROR):	/*  Return Error */
	  switch(session->ros_op & ROS_OP_OPCODE_MASK) {
	  case 1: /* shadowError */
	    disp_dissector = dissect_disp_ShadowError;
	    disp_op_name = "Shadow-Error";
	    break;
	  default:
	    proto_tree_add_text(tree, tvb, offset, -1,"Unsupported DISP errcode");
	    break;
	  }
	  break;
	default:
	  proto_tree_add_text(tree, tvb, offset, -1,"Unsupported DISP PDU");
	  return;
	}

	if(disp_dissector) {
	  if (check_col(pinfo->cinfo, COL_INFO))
	    col_add_str(pinfo->cinfo, COL_INFO, disp_op_name);

	  while (tvb_reported_length_remaining(tvb, offset) > 0){
	    old_offset=offset;
	    offset=(*disp_dissector)(FALSE, tvb, offset, pinfo , tree, -1);
	    if(offset == old_offset){
	      proto_tree_add_text(tree, tvb, offset, -1,"Internal error, zero-byte DISP PDU");
	      offset = tvb_length(tvb);
	      break;
	    }
	  }
	}
}


/*--- proto_register_disp -------------------------------------------*/
void proto_register_disp(void) {

  /* List of fields */
  static hf_register_info hf[] =
  {

/*--- Included file: packet-disp-hfarr.c ---*/

    { &hf_disp_modifiedSecondaryShadows,
      { "secondaryShadows", "disp.secondaryShadows",
        FT_UINT32, BASE_DEC, NULL, 0,
        "ModificationParameter/secondaryShadows", HFILL }},
    { &hf_disp_secondaryShadows_item,
      { "Item", "disp.secondaryShadows_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "ModificationParameter/secondaryShadows/_item", HFILL }},
    { &hf_disp_shadowSubject,
      { "shadowSubject", "disp.shadowSubject",
        FT_NONE, BASE_NONE, NULL, 0,
        "ShadowingAgreementInfo/shadowSubject", HFILL }},
    { &hf_disp_updateMode,
      { "updateMode", "disp.updateMode",
        FT_UINT32, BASE_DEC, VALS(disp_UpdateMode_vals), 0,
        "ShadowingAgreementInfo/updateMode", HFILL }},
    { &hf_disp_master,
      { "master", "disp.master",
        FT_NONE, BASE_NONE, NULL, 0,
        "ShadowingAgreementInfo/master", HFILL }},
    { &hf_disp_secondaryShadows,
      { "secondaryShadows", "disp.secondaryShadows",
        FT_BOOLEAN, 8, NULL, 0,
        "ShadowingAgreementInfo/secondaryShadows", HFILL }},
    { &hf_disp_area,
      { "area", "disp.area",
        FT_NONE, BASE_NONE, NULL, 0,
        "UnitOfReplication/area", HFILL }},
    { &hf_disp_replication_attributes,
      { "attributes", "disp.attributes",
        FT_UINT32, BASE_DEC, NULL, 0,
        "UnitOfReplication/attributes", HFILL }},
    { &hf_disp_knowledge,
      { "knowledge", "disp.knowledge",
        FT_NONE, BASE_NONE, NULL, 0,
        "UnitOfReplication/knowledge", HFILL }},
    { &hf_disp_subordinates,
      { "subordinates", "disp.subordinates",
        FT_BOOLEAN, 8, NULL, 0,
        "UnitOfReplication/subordinates", HFILL }},
    { &hf_disp_contextSelection,
      { "contextSelection", "disp.contextSelection",
        FT_UINT32, BASE_DEC, VALS(dap_ContextSelection_vals), 0,
        "UnitOfReplication/contextSelection", HFILL }},
    { &hf_disp_supplyContexts,
      { "supplyContexts", "disp.supplyContexts",
        FT_UINT32, BASE_DEC, VALS(disp_T_supplyContexts_vals), 0,
        "UnitOfReplication/supplyContexts", HFILL }},
    { &hf_disp_allContexts,
      { "allContexts", "disp.allContexts",
        FT_NONE, BASE_NONE, NULL, 0,
        "UnitOfReplication/supplyContexts/allContexts", HFILL }},
    { &hf_disp_selectedContexts,
      { "selectedContexts", "disp.selectedContexts",
        FT_UINT32, BASE_DEC, NULL, 0,
        "UnitOfReplication/supplyContexts/selectedContexts", HFILL }},
    { &hf_disp_selectedContexts_item,
      { "Item", "disp.selectedContexts_item",
        FT_STRING, BASE_NONE, NULL, 0,
        "UnitOfReplication/supplyContexts/selectedContexts/_item", HFILL }},
    { &hf_disp_contextPrefix,
      { "contextPrefix", "disp.contextPrefix",
        FT_UINT32, BASE_DEC, NULL, 0,
        "AreaSpecification/contextPrefix", HFILL }},
    { &hf_disp_replicationArea,
      { "replicationArea", "disp.replicationArea",
        FT_NONE, BASE_NONE, NULL, 0,
        "AreaSpecification/replicationArea", HFILL }},
    { &hf_disp_knowledgeType,
      { "knowledgeType", "disp.knowledgeType",
        FT_UINT32, BASE_DEC, VALS(disp_T_knowledgeType_vals), 0,
        "Knowledge/knowledgeType", HFILL }},
    { &hf_disp_extendedKnowledge,
      { "extendedKnowledge", "disp.extendedKnowledge",
        FT_BOOLEAN, 8, NULL, 0,
        "Knowledge/extendedKnowledge", HFILL }},
    { &hf_disp_AttributeSelection_item,
      { "Item", "disp.AttributeSelection_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "AttributeSelection/_item", HFILL }},
    { &hf_disp_class,
      { "class", "disp.class",
        FT_STRING, BASE_NONE, NULL, 0,
        "ClassAttributeSelection/class", HFILL }},
    { &hf_disp_classAttributes,
      { "classAttributes", "disp.classAttributes",
        FT_UINT32, BASE_DEC, VALS(disp_ClassAttributes_vals), 0,
        "ClassAttributeSelection/classAttributes", HFILL }},
    { &hf_disp_allAttributes,
      { "allAttributes", "disp.allAttributes",
        FT_NONE, BASE_NONE, NULL, 0,
        "ClassAttributes/allAttributes", HFILL }},
    { &hf_disp_include,
      { "include", "disp.include",
        FT_UINT32, BASE_DEC, NULL, 0,
        "ClassAttributes/include", HFILL }},
    { &hf_disp_exclude,
      { "exclude", "disp.exclude",
        FT_UINT32, BASE_DEC, NULL, 0,
        "ClassAttributes/exclude", HFILL }},
    { &hf_disp_AttributeTypes_item,
      { "Item", "disp.AttributeTypes_item",
        FT_STRING, BASE_NONE, NULL, 0,
        "AttributeTypes/_item", HFILL }},
    { &hf_disp_supplierInitiated,
      { "supplierInitiated", "disp.supplierInitiated",
        FT_UINT32, BASE_DEC, VALS(disp_SupplierUpdateMode_vals), 0,
        "UpdateMode/supplierInitiated", HFILL }},
    { &hf_disp_consumerInitiated,
      { "consumerInitiated", "disp.consumerInitiated",
        FT_NONE, BASE_NONE, NULL, 0,
        "UpdateMode/consumerInitiated", HFILL }},
    { &hf_disp_onChange,
      { "onChange", "disp.onChange",
        FT_BOOLEAN, 8, NULL, 0,
        "SupplierUpdateMode/onChange", HFILL }},
    { &hf_disp_scheduled,
      { "scheduled", "disp.scheduled",
        FT_NONE, BASE_NONE, NULL, 0,
        "SupplierUpdateMode/scheduled", HFILL }},
    { &hf_disp_periodic,
      { "periodic", "disp.periodic",
        FT_NONE, BASE_NONE, NULL, 0,
        "SchedulingParameters/periodic", HFILL }},
    { &hf_disp_othertimes,
      { "othertimes", "disp.othertimes",
        FT_BOOLEAN, 8, NULL, 0,
        "SchedulingParameters/othertimes", HFILL }},
    { &hf_disp_beginTime,
      { "beginTime", "disp.beginTime",
        FT_STRING, BASE_NONE, NULL, 0,
        "PeriodicStrategy/beginTime", HFILL }},
    { &hf_disp_windowSize,
      { "windowSize", "disp.windowSize",
        FT_INT32, BASE_DEC, NULL, 0,
        "PeriodicStrategy/windowSize", HFILL }},
    { &hf_disp_updateInterval,
      { "updateInterval", "disp.updateInterval",
        FT_INT32, BASE_DEC, NULL, 0,
        "PeriodicStrategy/updateInterval", HFILL }},
    { &hf_disp_agreementID,
      { "agreementID", "disp.agreementID",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_disp_lastUpdate,
      { "lastUpdate", "disp.lastUpdate",
        FT_STRING, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_disp_updateStrategy,
      { "updateStrategy", "disp.updateStrategy",
        FT_UINT32, BASE_DEC, VALS(disp_T_updateStrategy_vals), 0,
        "CoordinateShadowUpdateArgumentData/updateStrategy", HFILL }},
    { &hf_disp_standardUpdate,
      { "standard", "disp.standard",
        FT_UINT32, BASE_DEC, VALS(disp_StandardUpdate_vals), 0,
        "CoordinateShadowUpdateArgumentData/updateStrategy/standard", HFILL }},
    { &hf_disp_other,
      { "other", "disp.other",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_disp_securityParameters,
      { "securityParameters", "disp.securityParameters",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_disp_unsignedCoordinateShadowUpdateArgument,
      { "unsignedCoordinateShadowUpdateArgument", "disp.unsignedCoordinateShadowUpdateArgument",
        FT_NONE, BASE_NONE, NULL, 0,
        "CoordinateShadowUpdateArgument/unsignedCoordinateShadowUpdateArgument", HFILL }},
    { &hf_disp_signedCoordinateShadowUpdateArgument,
      { "signedCoordinateShadowUpdateArgument", "disp.signedCoordinateShadowUpdateArgument",
        FT_NONE, BASE_NONE, NULL, 0,
        "CoordinateShadowUpdateArgument/signedCoordinateShadowUpdateArgument", HFILL }},
    { &hf_disp_coordinateShadowUpdateArgument,
      { "coordinateShadowUpdateArgument", "disp.coordinateShadowUpdateArgument",
        FT_NONE, BASE_NONE, NULL, 0,
        "CoordinateShadowUpdateArgument/signedCoordinateShadowUpdateArgument/coordinateShadowUpdateArgument", HFILL }},
    { &hf_disp_algorithmIdentifier,
      { "algorithmIdentifier", "disp.algorithmIdentifier",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_disp_encrypted,
      { "encrypted", "disp.encrypted",
        FT_BYTES, BASE_HEX, NULL, 0,
        "", HFILL }},
    { &hf_disp_null,
      { "null", "disp.null",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_disp_information,
      { "information", "disp.information",
        FT_UINT32, BASE_DEC, VALS(disp_Information_vals), 0,
        "", HFILL }},
    { &hf_disp_performer,
      { "performer", "disp.performer",
        FT_UINT32, BASE_DEC, NULL, 0,
        "", HFILL }},
    { &hf_disp_aliasDereferenced,
      { "aliasDereferenced", "disp.aliasDereferenced",
        FT_BOOLEAN, 8, NULL, 0,
        "", HFILL }},
    { &hf_disp_notification,
      { "notification", "disp.notification",
        FT_UINT32, BASE_DEC, NULL, 0,
        "", HFILL }},
    { &hf_disp_notification_item,
      { "Item", "disp.notification_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_disp_unsignedInformation,
      { "unsignedInformation", "disp.unsignedInformation",
        FT_NONE, BASE_NONE, NULL, 0,
        "Information/unsignedInformation", HFILL }},
    { &hf_disp_signedInformation,
      { "signedInformation", "disp.signedInformation",
        FT_NONE, BASE_NONE, NULL, 0,
        "Information/signedInformation", HFILL }},
    { &hf_disp_information_data,
      { "information", "disp.information",
        FT_NONE, BASE_NONE, NULL, 0,
        "Information/signedInformation/information", HFILL }},
    { &hf_disp_requestedStrategy,
      { "requestedStrategy", "disp.requestedStrategy",
        FT_UINT32, BASE_DEC, VALS(disp_T_requestedStrategy_vals), 0,
        "RequestShadowUpdateArgumentData/requestedStrategy", HFILL }},
    { &hf_disp_standard,
      { "standard", "disp.standard",
        FT_UINT32, BASE_DEC, VALS(disp_T_standard_vals), 0,
        "RequestShadowUpdateArgumentData/requestedStrategy/standard", HFILL }},
    { &hf_disp_unsignedRequestShadowUpdateArgument,
      { "unsignedRequestShadowUpdateArgument", "disp.unsignedRequestShadowUpdateArgument",
        FT_NONE, BASE_NONE, NULL, 0,
        "RequestShadowUpdateArgument/unsignedRequestShadowUpdateArgument", HFILL }},
    { &hf_disp_signedRequestShadowUpdateArgument,
      { "signedRequestShadowUpdateArgument", "disp.signedRequestShadowUpdateArgument",
        FT_NONE, BASE_NONE, NULL, 0,
        "RequestShadowUpdateArgument/signedRequestShadowUpdateArgument", HFILL }},
    { &hf_disp_requestShadowUpdateArgument,
      { "requestShadowUpdateArgument", "disp.requestShadowUpdateArgument",
        FT_NONE, BASE_NONE, NULL, 0,
        "RequestShadowUpdateArgument/signedRequestShadowUpdateArgument/requestShadowUpdateArgument", HFILL }},
    { &hf_disp_updateTime,
      { "updateTime", "disp.updateTime",
        FT_STRING, BASE_NONE, NULL, 0,
        "UpdateShadowArgumentData/updateTime", HFILL }},
    { &hf_disp_updateWindow,
      { "updateWindow", "disp.updateWindow",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_disp_updatedInfo,
      { "updatedInfo", "disp.updatedInfo",
        FT_UINT32, BASE_DEC, VALS(disp_RefreshInformation_vals), 0,
        "UpdateShadowArgumentData/updatedInfo", HFILL }},
    { &hf_disp_unsignedUpdateShadowArgument,
      { "unsignedUpdateShadowArgument", "disp.unsignedUpdateShadowArgument",
        FT_NONE, BASE_NONE, NULL, 0,
        "UpdateShadowArgument/unsignedUpdateShadowArgument", HFILL }},
    { &hf_disp_signedUpdateShadowArgument,
      { "signedUpdateShadowArgument", "disp.signedUpdateShadowArgument",
        FT_NONE, BASE_NONE, NULL, 0,
        "UpdateShadowArgument/signedUpdateShadowArgument", HFILL }},
    { &hf_disp_updateShadowArgument,
      { "updateShadowArgument", "disp.updateShadowArgument",
        FT_NONE, BASE_NONE, NULL, 0,
        "UpdateShadowArgument/signedUpdateShadowArgument/updateShadowArgument", HFILL }},
    { &hf_disp_start,
      { "start", "disp.start",
        FT_STRING, BASE_NONE, NULL, 0,
        "UpdateWindow/start", HFILL }},
    { &hf_disp_stop,
      { "stop", "disp.stop",
        FT_STRING, BASE_NONE, NULL, 0,
        "UpdateWindow/stop", HFILL }},
    { &hf_disp_noRefresh,
      { "noRefresh", "disp.noRefresh",
        FT_NONE, BASE_NONE, NULL, 0,
        "RefreshInformation/noRefresh", HFILL }},
    { &hf_disp_total,
      { "total", "disp.total",
        FT_NONE, BASE_NONE, NULL, 0,
        "RefreshInformation/total", HFILL }},
    { &hf_disp_incremental,
      { "incremental", "disp.incremental",
        FT_UINT32, BASE_DEC, NULL, 0,
        "RefreshInformation/incremental", HFILL }},
    { &hf_disp_otherStrategy,
      { "otherStrategy", "disp.otherStrategy",
        FT_NONE, BASE_NONE, NULL, 0,
        "RefreshInformation/otherStrategy", HFILL }},
    { &hf_disp_sDSE,
      { "sDSE", "disp.sDSE",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_disp_subtree,
      { "subtree", "disp.subtree",
        FT_UINT32, BASE_DEC, NULL, 0,
        "", HFILL }},
    { &hf_disp_subtree_item,
      { "Item", "disp.subtree_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_disp_sDSEType,
      { "sDSEType", "disp.sDSEType",
        FT_BYTES, BASE_HEX, NULL, 0,
        "", HFILL }},
    { &hf_disp_subComplete,
      { "subComplete", "disp.subComplete",
        FT_BOOLEAN, 8, NULL, 0,
        "", HFILL }},
    { &hf_disp_attComplete,
      { "attComplete", "disp.attComplete",
        FT_BOOLEAN, 8, NULL, 0,
        "", HFILL }},
    { &hf_disp_attributes,
      { "attributes", "disp.attributes",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SDSEContent/attributes", HFILL }},
    { &hf_disp_attributes_item,
      { "Item", "disp.attributes_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "SDSEContent/attributes/_item", HFILL }},
    { &hf_disp_attValIncomplete,
      { "attValIncomplete", "disp.attValIncomplete",
        FT_UINT32, BASE_DEC, NULL, 0,
        "", HFILL }},
    { &hf_disp_attValIncomplete_item,
      { "Item", "disp.attValIncomplete_item",
        FT_STRING, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_disp_rdn,
      { "rdn", "disp.rdn",
        FT_UINT32, BASE_DEC, NULL, 0,
        "Subtree/rdn", HFILL }},
    { &hf_disp_IncrementalRefresh_item,
      { "Item", "disp.IncrementalRefresh_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "IncrementalRefresh/_item", HFILL }},
    { &hf_disp_sDSEChanges,
      { "sDSEChanges", "disp.sDSEChanges",
        FT_UINT32, BASE_DEC, VALS(disp_T_sDSEChanges_vals), 0,
        "IncrementalStepRefresh/sDSEChanges", HFILL }},
    { &hf_disp_add,
      { "add", "disp.add",
        FT_NONE, BASE_NONE, NULL, 0,
        "IncrementalStepRefresh/sDSEChanges/add", HFILL }},
    { &hf_disp_remove,
      { "remove", "disp.remove",
        FT_NONE, BASE_NONE, NULL, 0,
        "IncrementalStepRefresh/sDSEChanges/remove", HFILL }},
    { &hf_disp_modify,
      { "modify", "disp.modify",
        FT_NONE, BASE_NONE, NULL, 0,
        "IncrementalStepRefresh/sDSEChanges/modify", HFILL }},
    { &hf_disp_subordinateUpdates,
      { "subordinateUpdates", "disp.subordinateUpdates",
        FT_UINT32, BASE_DEC, NULL, 0,
        "IncrementalStepRefresh/subordinateUpdates", HFILL }},
    { &hf_disp_subordinateUpdates_item,
      { "Item", "disp.subordinateUpdates_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "IncrementalStepRefresh/subordinateUpdates/_item", HFILL }},
    { &hf_disp_rename,
      { "rename", "disp.rename",
        FT_UINT32, BASE_DEC, VALS(disp_T_rename_vals), 0,
        "ContentChange/rename", HFILL }},
    { &hf_disp_newRDN,
      { "newRDN", "disp.newRDN",
        FT_UINT32, BASE_DEC, NULL, 0,
        "ContentChange/rename/newRDN", HFILL }},
    { &hf_disp_newDN,
      { "newDN", "disp.newDN",
        FT_UINT32, BASE_DEC, NULL, 0,
        "ContentChange/rename/newDN", HFILL }},
    { &hf_disp_attributeChanges,
      { "attributeChanges", "disp.attributeChanges",
        FT_UINT32, BASE_DEC, VALS(disp_T_attributeChanges_vals), 0,
        "ContentChange/attributeChanges", HFILL }},
    { &hf_disp_replace,
      { "replace", "disp.replace",
        FT_UINT32, BASE_DEC, NULL, 0,
        "ContentChange/attributeChanges/replace", HFILL }},
    { &hf_disp_replace_item,
      { "Item", "disp.replace_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "ContentChange/attributeChanges/replace/_item", HFILL }},
    { &hf_disp_changes,
      { "changes", "disp.changes",
        FT_UINT32, BASE_DEC, NULL, 0,
        "ContentChange/attributeChanges/changes", HFILL }},
    { &hf_disp_changes_item,
      { "Item", "disp.changes_item",
        FT_UINT32, BASE_DEC, VALS(dap_EntryModification_vals), 0,
        "ContentChange/attributeChanges/changes/_item", HFILL }},
    { &hf_disp_subordinate,
      { "subordinate", "disp.subordinate",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SubordinateChanges/subordinate", HFILL }},
    { &hf_disp_subordinate_changes,
      { "changes", "disp.changes",
        FT_NONE, BASE_NONE, NULL, 0,
        "SubordinateChanges/changes", HFILL }},
    { &hf_disp_problem,
      { "problem", "disp.problem",
        FT_INT32, BASE_DEC, VALS(disp_ShadowProblem_vals), 0,
        "ShadowErrorData/problem", HFILL }},
    { &hf_disp_unsignedShadowError,
      { "unsignedShadowError", "disp.unsignedShadowError",
        FT_NONE, BASE_NONE, NULL, 0,
        "ShadowError/unsignedShadowError", HFILL }},
    { &hf_disp_signedShadowError,
      { "signedShadowError", "disp.signedShadowError",
        FT_NONE, BASE_NONE, NULL, 0,
        "ShadowError/signedShadowError", HFILL }},
    { &hf_disp_shadowError,
      { "shadowError", "disp.shadowError",
        FT_NONE, BASE_NONE, NULL, 0,
        "ShadowError/signedShadowError/shadowError", HFILL }},

/*--- End of included file: packet-disp-hfarr.c ---*/

  };

  /* List of subtrees */
  static gint *ett[] = {
    &ett_disp,

/*--- Included file: packet-disp-ettarr.c ---*/

    &ett_disp_ModificationParameter,
    &ett_disp_SET_OF_SupplierAndConsumers,
    &ett_disp_ShadowingAgreementInfo,
    &ett_disp_UnitOfReplication,
    &ett_disp_T_supplyContexts,
    &ett_disp_T_selectedContexts,
    &ett_disp_AreaSpecification,
    &ett_disp_Knowledge,
    &ett_disp_AttributeSelection,
    &ett_disp_ClassAttributeSelection,
    &ett_disp_ClassAttributes,
    &ett_disp_AttributeTypes,
    &ett_disp_UpdateMode,
    &ett_disp_SupplierUpdateMode,
    &ett_disp_SchedulingParameters,
    &ett_disp_PeriodicStrategy,
    &ett_disp_CoordinateShadowUpdateArgumentData,
    &ett_disp_T_updateStrategy,
    &ett_disp_CoordinateShadowUpdateArgument,
    &ett_disp_T_signedCoordinateShadowUpdateArgument,
    &ett_disp_CoordinateShadowUpdateResult,
    &ett_disp_InformationData,
    &ett_disp_SEQUENCE_OF_Attribute,
    &ett_disp_Information,
    &ett_disp_T_signedInformation,
    &ett_disp_RequestShadowUpdateArgumentData,
    &ett_disp_T_requestedStrategy,
    &ett_disp_RequestShadowUpdateArgument,
    &ett_disp_T_signedRequestShadowUpdateArgument,
    &ett_disp_RequestShadowUpdateResult,
    &ett_disp_UpdateShadowArgumentData,
    &ett_disp_UpdateShadowArgument,
    &ett_disp_T_signedUpdateShadowArgument,
    &ett_disp_UpdateShadowResult,
    &ett_disp_UpdateWindow,
    &ett_disp_RefreshInformation,
    &ett_disp_TotalRefresh,
    &ett_disp_SET_OF_Subtree,
    &ett_disp_SDSEContent,
    &ett_disp_SET_OF_Attribute,
    &ett_disp_SET_OF_AttributeType,
    &ett_disp_Subtree,
    &ett_disp_IncrementalRefresh,
    &ett_disp_IncrementalStepRefresh,
    &ett_disp_T_sDSEChanges,
    &ett_disp_SEQUENCE_OF_SubordinateChanges,
    &ett_disp_ContentChange,
    &ett_disp_T_rename,
    &ett_disp_T_attributeChanges,
    &ett_disp_SEQUENCE_OF_EntryModification,
    &ett_disp_SubordinateChanges,
    &ett_disp_ShadowErrorData,
    &ett_disp_ShadowError,
    &ett_disp_T_signedShadowError,

/*--- End of included file: packet-disp-ettarr.c ---*/

  };
  module_t *disp_module;

  /* Register protocol */
  proto_disp = proto_register_protocol(PNAME, PSNAME, PFNAME);
  register_dissector("disp", dissect_disp, proto_disp);

  /* Register fields and subtrees */
  proto_register_field_array(proto_disp, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));

  /* Register our configuration options for DISP, particularly our port */

  disp_module = prefs_register_protocol(proto_disp, prefs_register_disp);

  prefs_register_uint_preference(disp_module, "tcp.port", "DISP TCP Port",
				 "Set the port for DISP operations (if other"
				 " than the default of 102)",
				 10, &global_disp_tcp_port);

}


/*--- proto_reg_handoff_disp --- */
void proto_reg_handoff_disp(void) {
  dissector_handle_t handle = NULL;

  /* #include "packet-disp-dis-tab.c" */

  /* APPLICATION CONTEXT */

  register_ber_oid_name("2.5.3.4", "id-ac-shadow-consumer-initiated");
  register_ber_oid_name("2.5.3.5", "id-ac-shadow-supplier-initiated");
  register_ber_oid_name("2.5.3.6", "id-ac-reliable-shadow-consumer-initiated");
  register_ber_oid_name("2.5.3.7", "id-ac-reliable-shadow-supplier-initiated");

  /* ABSTRACT SYNTAXES */

  if((handle = find_dissector("disp"))) {

    register_ros_oid_dissector_handle("2.5.9.3", handle, 0, "id-as-directory-shadow", FALSE); 

    register_rtse_oid_dissector_handle("2.5.9.5", handle, 0, "id-as-directory-reliable-shadow", FALSE); 
    register_rtse_oid_dissector_handle("2.5.9.6", handle, 0, "id-as-directory-reliable-binding", FALSE); 
  } 

  tpkt_handle = find_dissector("tpkt");

}


void prefs_register_disp(void) {

  /* de-register the old port */
  /* port 102 is registered by TPKT - don't undo this! */
  if((tcp_port != 102) && tpkt_handle)
    dissector_delete("tcp.port", tcp_port, tpkt_handle);

  /* Set our port number for future use */
  tcp_port = global_disp_tcp_port;

  if((tcp_port > 0) && (tcp_port != 102) && tpkt_handle)
    dissector_add("tcp.port", global_disp_tcp_port, tpkt_handle);

}
