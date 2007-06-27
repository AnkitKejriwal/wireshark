/* Do not modify this file.                                                   */
/* It is created automatically by the ASN.1 to Wireshark dissector compiler   */
/* ./packet-pres.c                                                            */
/* ../../tools/asn2wrs.py -b -e -p pres -c pres.cnf -s packet-pres-template ISO8823-PRESENTATION.asn */

/* Input file: packet-pres-template.c */

#line 1 "packet-pres-template.c"
/* packet-pres.c
 * Routine to dissect ISO 8823 OSI Presentation Protocol packets
 * Based on the dissector by 
 * Yuriy Sidelnikov <YSidelnikov@hotmail.com>
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
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glib.h>
#include <epan/packet.h>
#include <epan/conversation.h>
#include <epan/emem.h>

#include <stdio.h>
#include <string.h>

#include <epan/asn1.h>
#include "packet-ber.h"
#include "packet-ses.h"
#include "packet-pres.h"
#include "packet-rtse.h"


#define PNAME  "ISO 8823 OSI Presentation Protocol"
#define PSNAME "PRES"
#define PFNAME "pres"

/* Initialize the protocol and registered fields */
int proto_pres = -1;
/*   type of session envelop */
static struct SESSION_DATA_STRUCTURE* session = NULL;

/*      pointers for acse dissector  */
proto_tree *global_tree  = NULL;
packet_info *global_pinfo = NULL;

/* dissector for data */
static dissector_handle_t data_handle;

static const char *abstract_syntax_name_oid;
static guint32 presentation_context_identifier;

/* to keep track of presentation context identifiers and protocol-oids */
typedef struct _pres_ctx_oid_t {
	guint32 ctx_id;
	char *oid;
	guint32 index;
} pres_ctx_oid_t;
static GHashTable *pres_ctx_oid_table = NULL;

static int hf_pres_CP_type = -1;
static int hf_pres_CPA_PPDU = -1;
static int hf_pres_Abort_type = -1;
static int hf_pres_CPR_PPDU = -1;
static int hf_pres_Typed_data_type = -1;


/*--- Included file: packet-pres-hf.c ---*/
#line 1 "packet-pres-hf.c"
static int hf_pres_mode_selector = -1;            /* Mode_selector */
static int hf_pres_x410_mode_parameters = -1;     /* RTORQapdu */
static int hf_pres_normal_mode_parameters = -1;   /* T_normal_mode_parameters */
static int hf_pres_protocol_version = -1;         /* Protocol_version */
static int hf_pres_calling_presentation_selector = -1;  /* Calling_presentation_selector */
static int hf_pres_called_presentation_selector = -1;  /* Called_presentation_selector */
static int hf_pres_presentation_context_definition_list = -1;  /* Presentation_context_definition_list */
static int hf_pres_default_context_name = -1;     /* Default_context_name */
static int hf_pres_presentation_requirements = -1;  /* Presentation_requirements */
static int hf_pres_user_session_requirements = -1;  /* User_session_requirements */
static int hf_pres_protocol_options = -1;         /* Protocol_options */
static int hf_pres_initiators_nominated_context = -1;  /* Presentation_context_identifier */
static int hf_pres_extensions = -1;               /* T_extensions */
static int hf_pres_user_data = -1;                /* User_data */
static int hf_pres_cPR_PPDU_x400_mode_parameters = -1;  /* RTOACapdu */
static int hf_pres_cPU_PPDU_normal_mode_parameters = -1;  /* T_CPA_PPDU_normal_mode_parameters */
static int hf_pres_responding_presentation_selector = -1;  /* Responding_presentation_selector */
static int hf_pres_presentation_context_definition_result_list = -1;  /* Presentation_context_definition_result_list */
static int hf_pres_responders_nominated_context = -1;  /* Presentation_context_identifier */
static int hf_pres_cPU_PPDU_x400_mode_parameters = -1;  /* RTORJapdu */
static int hf_pres_cPR_PPDU_normal_mode_parameters = -1;  /* T_CPR_PPDU_normal_mode_parameters */
static int hf_pres_default_context_result = -1;   /* Default_context_result */
static int hf_pres_cPR_PPDU__provider_reason = -1;  /* Provider_reason */
static int hf_pres_aru_ppdu = -1;                 /* ARU_PPDU */
static int hf_pres_arp_ppdu = -1;                 /* ARP_PPDU */
static int hf_pres_aRU_PPDU_x400_mode_parameters = -1;  /* RTABapdu */
static int hf_pres_aRU_PPDU_normal_mode_parameters = -1;  /* T_ARU_PPDU_normal_mode_parameters */
static int hf_pres_presentation_context_identifier_list = -1;  /* Presentation_context_identifier_list */
static int hf_pres_aRU_PPDU_provider_reason = -1;  /* Abort_reason */
static int hf_pres_event_identifier = -1;         /* Event_identifier */
static int hf_pres_acPPDU = -1;                   /* AC_PPDU */
static int hf_pres_acaPPDU = -1;                  /* ACA_PPDU */
static int hf_pres_ttdPPDU = -1;                  /* User_data */
static int hf_pres_presentation_context_addition_list = -1;  /* Presentation_context_addition_list */
static int hf_pres_presentation_context_deletion_list = -1;  /* Presentation_context_deletion_list */
static int hf_pres_presentation_context_addition_result_list = -1;  /* Presentation_context_addition_result_list */
static int hf_pres_presentation_context_deletion_result_list = -1;  /* Presentation_context_deletion_result_list */
static int hf_pres_Context_list_item = -1;        /* Context_list_item */
static int hf_pres_presentation_context_identifier = -1;  /* Presentation_context_identifier */
static int hf_pres_abstract_syntax_name = -1;     /* Abstract_syntax_name */
static int hf_pres_transfer_syntax_name_list = -1;  /* SEQUENCE_OF_Transfer_syntax_name */
static int hf_pres_transfer_syntax_name_list_item = -1;  /* Transfer_syntax_name */
static int hf_pres_transfer_syntax_name = -1;     /* Transfer_syntax_name */
static int hf_pres_mode_value = -1;               /* T_mode_value */
static int hf_pres_Presentation_context_deletion_list_item = -1;  /* Presentation_context_identifier */
static int hf_pres_Presentation_context_deletion_result_list_item = -1;  /* Presentation_context_deletion_result_list_item */
static int hf_pres_Presentation_context_identifier_list_item = -1;  /* Presentation_context_identifier_list_item */
static int hf_pres_Result_list_item = -1;         /* Result_list_item */
static int hf_pres_result = -1;                   /* Result */
static int hf_pres_provider_reason = -1;          /* T_provider_reason */
static int hf_pres_simply_encoded_data = -1;      /* Simply_encoded_data */
static int hf_pres_fully_encoded_data = -1;       /* Fully_encoded_data */
static int hf_pres_Fully_encoded_data_item = -1;  /* PDV_list */
static int hf_pres_presentation_data_values = -1;  /* T_presentation_data_values */
static int hf_pres_single_ASN1_type = -1;         /* T_single_ASN1_type */
static int hf_pres_octet_aligned = -1;            /* T_octet_aligned */
static int hf_pres_arbitrary = -1;                /* BIT_STRING */
/* named bits */
static int hf_pres_Presentation_requirements_context_management = -1;
static int hf_pres_Presentation_requirements_restoration = -1;
static int hf_pres_Protocol_options_nominated_context = -1;
static int hf_pres_Protocol_options_short_encoding = -1;
static int hf_pres_Protocol_options_packed_encoding_rules = -1;
static int hf_pres_Protocol_version_version_1 = -1;
static int hf_pres_User_session_requirements_half_duplex = -1;
static int hf_pres_User_session_requirements_duplex = -1;
static int hf_pres_User_session_requirements_expedited_data = -1;
static int hf_pres_User_session_requirements_minor_synchronize = -1;
static int hf_pres_User_session_requirements_major_synchronize = -1;
static int hf_pres_User_session_requirements_resynchronize = -1;
static int hf_pres_User_session_requirements_activity_management = -1;
static int hf_pres_User_session_requirements_negotiated_release = -1;
static int hf_pres_User_session_requirements_capability_data = -1;
static int hf_pres_User_session_requirements_exceptions = -1;
static int hf_pres_User_session_requirements_typed_data = -1;
static int hf_pres_User_session_requirements_symmetric_synchronize = -1;
static int hf_pres_User_session_requirements_data_separation = -1;

/*--- End of included file: packet-pres-hf.c ---*/
#line 80 "packet-pres-template.c"

/* Initialize the subtree pointers */
static gint ett_pres           = -1;


/*--- Included file: packet-pres-ett.c ---*/
#line 1 "packet-pres-ett.c"
static gint ett_pres_CP_type = -1;
static gint ett_pres_T_normal_mode_parameters = -1;
static gint ett_pres_T_extensions = -1;
static gint ett_pres_CPA_PPDU = -1;
static gint ett_pres_T_CPA_PPDU_normal_mode_parameters = -1;
static gint ett_pres_CPR_PPDU = -1;
static gint ett_pres_T_CPR_PPDU_normal_mode_parameters = -1;
static gint ett_pres_Abort_type = -1;
static gint ett_pres_ARU_PPDU = -1;
static gint ett_pres_T_ARU_PPDU_normal_mode_parameters = -1;
static gint ett_pres_ARP_PPDU = -1;
static gint ett_pres_Typed_data_type = -1;
static gint ett_pres_AC_PPDU = -1;
static gint ett_pres_ACA_PPDU = -1;
static gint ett_pres_RS_PPDU = -1;
static gint ett_pres_RSA_PPDU = -1;
static gint ett_pres_Context_list = -1;
static gint ett_pres_Context_list_item = -1;
static gint ett_pres_SEQUENCE_OF_Transfer_syntax_name = -1;
static gint ett_pres_Default_context_name = -1;
static gint ett_pres_Mode_selector = -1;
static gint ett_pres_Presentation_context_deletion_list = -1;
static gint ett_pres_Presentation_context_deletion_result_list = -1;
static gint ett_pres_Presentation_context_identifier_list = -1;
static gint ett_pres_Presentation_context_identifier_list_item = -1;
static gint ett_pres_Presentation_requirements = -1;
static gint ett_pres_Protocol_options = -1;
static gint ett_pres_Protocol_version = -1;
static gint ett_pres_Result_list = -1;
static gint ett_pres_Result_list_item = -1;
static gint ett_pres_User_data = -1;
static gint ett_pres_Fully_encoded_data = -1;
static gint ett_pres_PDV_list = -1;
static gint ett_pres_T_presentation_data_values = -1;
static gint ett_pres_User_session_requirements = -1;

/*--- End of included file: packet-pres-ett.c ---*/
#line 85 "packet-pres-template.c"


static guint
pres_ctx_oid_hash(gconstpointer k)
{
	pres_ctx_oid_t *pco=(pres_ctx_oid_t *)k;
	return pco->ctx_id;
}

static gint
pres_ctx_oid_equal(gconstpointer k1, gconstpointer k2)
{
	pres_ctx_oid_t *pco1=(pres_ctx_oid_t *)k1;
	pres_ctx_oid_t *pco2=(pres_ctx_oid_t *)k2;
	return (pco1->ctx_id==pco2->ctx_id && pco1->index==pco2->index);
}

static void
pres_init(void)
{
	if( pres_ctx_oid_table ){
		g_hash_table_destroy(pres_ctx_oid_table);
		pres_ctx_oid_table = NULL;
	}
	pres_ctx_oid_table = g_hash_table_new(pres_ctx_oid_hash,
			pres_ctx_oid_equal);

}

static void
register_ctx_id_and_oid(packet_info *pinfo _U_, guint32 idx, const char *oid)
{
	pres_ctx_oid_t *pco, *tmppco;
	conversation_t *conversation;

	if(!oid){
		/* we did not get any oid name, malformed packet? */
		return;
	}

	pco=se_alloc(sizeof(pres_ctx_oid_t));
	pco->ctx_id=idx;
	pco->oid=se_strdup(oid);
	conversation=find_conversation (pinfo->fd->num, &pinfo->src, &pinfo->dst, 
			pinfo->ptype, pinfo->srcport, pinfo->destport, 0);
	if (conversation) {
		pco->index = conversation->index;
	} else {
		pco->index = 0;
	}

	/* if this ctx already exists, remove the old one first */
	tmppco=(pres_ctx_oid_t *)g_hash_table_lookup(pres_ctx_oid_table, pco);
	if(tmppco){
		g_hash_table_remove(pres_ctx_oid_table, tmppco);

	}
	g_hash_table_insert(pres_ctx_oid_table, pco, pco);
}
char *
find_oid_by_pres_ctx_id(packet_info *pinfo _U_, guint32 idx)
{
	pres_ctx_oid_t pco, *tmppco;
	conversation_t *conversation;

	pco.ctx_id=idx;
	conversation=find_conversation (pinfo->fd->num, &pinfo->src, &pinfo->dst, 
			pinfo->ptype, pinfo->srcport, pinfo->destport, 0);
	if (conversation) {
		pco.index = conversation->index;
	} else {
		pco.index = 0;
	}

	tmppco=(pres_ctx_oid_t *)g_hash_table_lookup(pres_ctx_oid_table, &pco);
	if(tmppco){
		return tmppco->oid;
	}
	return NULL;
}



/*--- Included file: packet-pres-fn.c ---*/
#line 1 "packet-pres-fn.c"
/*--- Fields for imported types ---*/

static int dissect_x410_mode_parameters_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_rtse_RTORQapdu(TRUE, tvb, offset, actx, tree, hf_pres_x410_mode_parameters);
}
static int dissect_cPR_PPDU_x400_mode_parameters_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_rtse_RTOACapdu(TRUE, tvb, offset, actx, tree, hf_pres_cPR_PPDU_x400_mode_parameters);
}
static int dissect_cPU_PPDU_x400_mode_parameters(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_rtse_RTORJapdu(FALSE, tvb, offset, actx, tree, hf_pres_cPU_PPDU_x400_mode_parameters);
}
static int dissect_aRU_PPDU_x400_mode_parameters(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_rtse_RTABapdu(FALSE, tvb, offset, actx, tree, hf_pres_aRU_PPDU_x400_mode_parameters);
}


static const value_string pres_T_mode_value_vals[] = {
  {   0, "x410-1984-mode" },
  {   1, "normal-mode" },
  { 0, NULL }
};


static int
dissect_pres_T_mode_value(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_mode_value_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_T_mode_value(TRUE, tvb, offset, actx, tree, hf_pres_mode_value);
}


static const ber_old_sequence_t Mode_selector_set[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_mode_value_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_pres_Mode_selector(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_set(implicit_tag, actx, tree, tvb, offset,
                                  Mode_selector_set, hf_index, ett_pres_Mode_selector);

  return offset;
}
static int dissect_mode_selector_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_Mode_selector(TRUE, tvb, offset, actx, tree, hf_pres_mode_selector);
}


static const asn_namedbit Protocol_version_bits[] = {
  {  0, &hf_pres_Protocol_version_version_1, -1, -1, "version-1", NULL },
  { 0, NULL, 0, 0, NULL, NULL }
};

static int
dissect_pres_Protocol_version(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_bitstring(implicit_tag, actx, tree, tvb, offset,
                                    Protocol_version_bits, hf_index, ett_pres_Protocol_version,
                                    NULL);

  return offset;
}
static int dissect_protocol_version_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_Protocol_version(TRUE, tvb, offset, actx, tree, hf_pres_protocol_version);
}



static int
dissect_pres_Presentation_selector(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, actx, tree, tvb, offset, hf_index,
                                       NULL);

  return offset;
}



static int
dissect_pres_Calling_presentation_selector(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_pres_Presentation_selector(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_calling_presentation_selector_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_Calling_presentation_selector(TRUE, tvb, offset, actx, tree, hf_pres_calling_presentation_selector);
}



static int
dissect_pres_Called_presentation_selector(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_pres_Presentation_selector(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_called_presentation_selector_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_Called_presentation_selector(TRUE, tvb, offset, actx, tree, hf_pres_called_presentation_selector);
}



static int
dissect_pres_Presentation_context_identifier(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 69 "pres.cnf"

    offset = dissect_ber_integer(implicit_tag, actx, tree, tvb, offset, hf_index,
                                  &presentation_context_identifier);


  if(session)
	session->pres_ctx_id = presentation_context_identifier;



  return offset;
}
static int dissect_initiators_nominated_context(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_Presentation_context_identifier(FALSE, tvb, offset, actx, tree, hf_pres_initiators_nominated_context);
}
static int dissect_responders_nominated_context(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_Presentation_context_identifier(FALSE, tvb, offset, actx, tree, hf_pres_responders_nominated_context);
}
static int dissect_presentation_context_identifier(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_Presentation_context_identifier(FALSE, tvb, offset, actx, tree, hf_pres_presentation_context_identifier);
}
static int dissect_Presentation_context_deletion_list_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_Presentation_context_identifier(FALSE, tvb, offset, actx, tree, hf_pres_Presentation_context_deletion_list_item);
}



static int
dissect_pres_Abstract_syntax_name(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_object_identifier_str(implicit_tag, actx, tree, tvb, offset, hf_index, &abstract_syntax_name_oid);

  return offset;
}
static int dissect_abstract_syntax_name(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_Abstract_syntax_name(FALSE, tvb, offset, actx, tree, hf_pres_abstract_syntax_name);
}
static int dissect_abstract_syntax_name_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_Abstract_syntax_name(TRUE, tvb, offset, actx, tree, hf_pres_abstract_syntax_name);
}



static int
dissect_pres_Transfer_syntax_name(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_object_identifier(implicit_tag, actx, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_transfer_syntax_name_list_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_Transfer_syntax_name(FALSE, tvb, offset, actx, tree, hf_pres_transfer_syntax_name_list_item);
}
static int dissect_transfer_syntax_name(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_Transfer_syntax_name(FALSE, tvb, offset, actx, tree, hf_pres_transfer_syntax_name);
}
static int dissect_transfer_syntax_name_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_Transfer_syntax_name(TRUE, tvb, offset, actx, tree, hf_pres_transfer_syntax_name);
}


static const ber_old_sequence_t SEQUENCE_OF_Transfer_syntax_name_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_OID, BER_FLAGS_NOOWNTAG, dissect_transfer_syntax_name_list_item },
};

static int
dissect_pres_SEQUENCE_OF_Transfer_syntax_name(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                          SEQUENCE_OF_Transfer_syntax_name_sequence_of, hf_index, ett_pres_SEQUENCE_OF_Transfer_syntax_name);

  return offset;
}
static int dissect_transfer_syntax_name_list(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_SEQUENCE_OF_Transfer_syntax_name(FALSE, tvb, offset, actx, tree, hf_pres_transfer_syntax_name_list);
}


static const ber_old_sequence_t Context_list_item_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_presentation_context_identifier },
  { BER_CLASS_UNI, BER_UNI_TAG_OID, BER_FLAGS_NOOWNTAG, dissect_abstract_syntax_name },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_transfer_syntax_name_list },
  { 0, 0, 0, NULL }
};

static int
dissect_pres_Context_list_item(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 76 "pres.cnf"
	abstract_syntax_name_oid=NULL;

  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       Context_list_item_sequence, hf_index, ett_pres_Context_list_item);

#line 79 "pres.cnf"
	register_ctx_id_and_oid(actx->pinfo, presentation_context_identifier, abstract_syntax_name_oid);

  return offset;
}
static int dissect_Context_list_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_Context_list_item(FALSE, tvb, offset, actx, tree, hf_pres_Context_list_item);
}


static const ber_old_sequence_t Context_list_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_Context_list_item },
};

static int
dissect_pres_Context_list(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                          Context_list_sequence_of, hf_index, ett_pres_Context_list);

  return offset;
}



static int
dissect_pres_Presentation_context_definition_list(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_pres_Context_list(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_presentation_context_definition_list_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_Presentation_context_definition_list(TRUE, tvb, offset, actx, tree, hf_pres_presentation_context_definition_list);
}


static const ber_old_sequence_t Default_context_name_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_abstract_syntax_name_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_transfer_syntax_name_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_pres_Default_context_name(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       Default_context_name_sequence, hf_index, ett_pres_Default_context_name);

  return offset;
}
static int dissect_default_context_name_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_Default_context_name(TRUE, tvb, offset, actx, tree, hf_pres_default_context_name);
}


static const asn_namedbit Presentation_requirements_bits[] = {
  {  0, &hf_pres_Presentation_requirements_context_management, -1, -1, "context-management", NULL },
  {  1, &hf_pres_Presentation_requirements_restoration, -1, -1, "restoration", NULL },
  { 0, NULL, 0, 0, NULL, NULL }
};

static int
dissect_pres_Presentation_requirements(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_bitstring(implicit_tag, actx, tree, tvb, offset,
                                    Presentation_requirements_bits, hf_index, ett_pres_Presentation_requirements,
                                    NULL);

  return offset;
}
static int dissect_presentation_requirements_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_Presentation_requirements(TRUE, tvb, offset, actx, tree, hf_pres_presentation_requirements);
}


static const asn_namedbit User_session_requirements_bits[] = {
  {  0, &hf_pres_User_session_requirements_half_duplex, -1, -1, "half-duplex", NULL },
  {  1, &hf_pres_User_session_requirements_duplex, -1, -1, "duplex", NULL },
  {  2, &hf_pres_User_session_requirements_expedited_data, -1, -1, "expedited-data", NULL },
  {  3, &hf_pres_User_session_requirements_minor_synchronize, -1, -1, "minor-synchronize", NULL },
  {  4, &hf_pres_User_session_requirements_major_synchronize, -1, -1, "major-synchronize", NULL },
  {  5, &hf_pres_User_session_requirements_resynchronize, -1, -1, "resynchronize", NULL },
  {  6, &hf_pres_User_session_requirements_activity_management, -1, -1, "activity-management", NULL },
  {  7, &hf_pres_User_session_requirements_negotiated_release, -1, -1, "negotiated-release", NULL },
  {  8, &hf_pres_User_session_requirements_capability_data, -1, -1, "capability-data", NULL },
  {  9, &hf_pres_User_session_requirements_exceptions, -1, -1, "exceptions", NULL },
  { 10, &hf_pres_User_session_requirements_typed_data, -1, -1, "typed-data", NULL },
  { 11, &hf_pres_User_session_requirements_symmetric_synchronize, -1, -1, "symmetric-synchronize", NULL },
  { 12, &hf_pres_User_session_requirements_data_separation, -1, -1, "data-separation", NULL },
  { 0, NULL, 0, 0, NULL, NULL }
};

static int
dissect_pres_User_session_requirements(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_bitstring(implicit_tag, actx, tree, tvb, offset,
                                    User_session_requirements_bits, hf_index, ett_pres_User_session_requirements,
                                    NULL);

  return offset;
}
static int dissect_user_session_requirements_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_User_session_requirements(TRUE, tvb, offset, actx, tree, hf_pres_user_session_requirements);
}


static const asn_namedbit Protocol_options_bits[] = {
  {  0, &hf_pres_Protocol_options_nominated_context, -1, -1, "nominated-context", NULL },
  {  1, &hf_pres_Protocol_options_short_encoding, -1, -1, "short-encoding", NULL },
  {  2, &hf_pres_Protocol_options_packed_encoding_rules, -1, -1, "packed-encoding-rules", NULL },
  { 0, NULL, 0, 0, NULL, NULL }
};

static int
dissect_pres_Protocol_options(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_bitstring(implicit_tag, actx, tree, tvb, offset,
                                    Protocol_options_bits, hf_index, ett_pres_Protocol_options,
                                    NULL);

  return offset;
}
static int dissect_protocol_options(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_Protocol_options(FALSE, tvb, offset, actx, tree, hf_pres_protocol_options);
}


static const ber_old_sequence_t T_extensions_sequence[] = {
  { 0, 0, 0, NULL }
};

static int
dissect_pres_T_extensions(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       T_extensions_sequence, hf_index, ett_pres_T_extensions);

  return offset;
}
static int dissect_extensions(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_T_extensions(FALSE, tvb, offset, actx, tree, hf_pres_extensions);
}



static int
dissect_pres_Simply_encoded_data(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, actx, tree, tvb, offset, hf_index,
                                       NULL);

  return offset;
}
static int dissect_simply_encoded_data_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_Simply_encoded_data(TRUE, tvb, offset, actx, tree, hf_pres_simply_encoded_data);
}



static int
dissect_pres_T_single_ASN1_type(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 35 "pres.cnf"

 tvbuff_t	*next_tvb;
 char *oid; 

	oid=find_oid_by_pres_ctx_id(actx->pinfo, presentation_context_identifier);
	if(oid){
		next_tvb = tvb_new_subset(tvb, offset, -1, -1);
		call_ber_oid_callback(oid, next_tvb, offset, actx->pinfo, global_tree);
	} else {
		proto_tree_add_text(tree, tvb, offset, -1,"dissector is not available");
		  offset = dissect_ber_octet_string(implicit_tag, actx, tree, tvb, offset, hf_index,
                                       NULL);
	
	}


  return offset;
}
static int dissect_single_ASN1_type_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_T_single_ASN1_type(TRUE, tvb, offset, actx, tree, hf_pres_single_ASN1_type);
}



static int
dissect_pres_T_octet_aligned(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 48 "pres.cnf"

 tvbuff_t	*next_tvb;
 char *oid; 

	oid=find_oid_by_pres_ctx_id(actx->pinfo, presentation_context_identifier);
	if(oid){
		next_tvb = tvb_new_subset(tvb, offset, -1, -1);
		call_ber_oid_callback(oid, next_tvb, offset, actx->pinfo, global_tree);
	} else {
		proto_tree_add_text(tree, tvb, offset, -1,"dissector is not available");
		  offset = dissect_ber_octet_string(implicit_tag, actx, tree, tvb, offset, hf_index,
                                       NULL);
	
	}




  return offset;
}
static int dissect_octet_aligned_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_T_octet_aligned(TRUE, tvb, offset, actx, tree, hf_pres_octet_aligned);
}



static int
dissect_pres_BIT_STRING(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_bitstring(implicit_tag, actx, tree, tvb, offset,
                                    NULL, hf_index, -1,
                                    NULL);

  return offset;
}
static int dissect_arbitrary_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_BIT_STRING(TRUE, tvb, offset, actx, tree, hf_pres_arbitrary);
}


static const value_string pres_T_presentation_data_values_vals[] = {
  {   0, "single-ASN1-type" },
  {   1, "octet-aligned" },
  {   2, "arbitrary" },
  { 0, NULL }
};

static const ber_old_choice_t T_presentation_data_values_choice[] = {
  {   0, BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_single_ASN1_type_impl },
  {   1, BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_octet_aligned_impl },
  {   2, BER_CLASS_CON, 2, BER_FLAGS_IMPLTAG, dissect_arbitrary_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_pres_T_presentation_data_values(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_choice(actx, tree, tvb, offset,
                                     T_presentation_data_values_choice, hf_index, ett_pres_T_presentation_data_values,
                                     NULL);

  return offset;
}
static int dissect_presentation_data_values(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_T_presentation_data_values(FALSE, tvb, offset, actx, tree, hf_pres_presentation_data_values);
}


static const ber_old_sequence_t PDV_list_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_OID, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_transfer_syntax_name },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_presentation_context_identifier },
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_presentation_data_values },
  { 0, 0, 0, NULL }
};

static int
dissect_pres_PDV_list(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       PDV_list_sequence, hf_index, ett_pres_PDV_list);

  return offset;
}
static int dissect_Fully_encoded_data_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_PDV_list(FALSE, tvb, offset, actx, tree, hf_pres_Fully_encoded_data_item);
}


static const ber_old_sequence_t Fully_encoded_data_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_Fully_encoded_data_item },
};

static int
dissect_pres_Fully_encoded_data(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                          Fully_encoded_data_sequence_of, hf_index, ett_pres_Fully_encoded_data);

  return offset;
}
static int dissect_fully_encoded_data_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_Fully_encoded_data(TRUE, tvb, offset, actx, tree, hf_pres_fully_encoded_data);
}


static const value_string pres_User_data_vals[] = {
  {   0, "simply-encoded-data" },
  {   1, "fully-encoded-data" },
  { 0, NULL }
};

static const ber_old_choice_t User_data_choice[] = {
  {   0, BER_CLASS_APP, 0, BER_FLAGS_IMPLTAG, dissect_simply_encoded_data_impl },
  {   1, BER_CLASS_APP, 1, BER_FLAGS_IMPLTAG, dissect_fully_encoded_data_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_pres_User_data(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_choice(actx, tree, tvb, offset,
                                     User_data_choice, hf_index, ett_pres_User_data,
                                     NULL);

  return offset;
}
static int dissect_user_data(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_User_data(FALSE, tvb, offset, actx, tree, hf_pres_user_data);
}
static int dissect_ttdPPDU(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_User_data(FALSE, tvb, offset, actx, tree, hf_pres_ttdPPDU);
}


static const ber_old_sequence_t T_normal_mode_parameters_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_protocol_version_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_calling_presentation_selector_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_called_presentation_selector_impl },
  { BER_CLASS_CON, 4, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_presentation_context_definition_list_impl },
  { BER_CLASS_CON, 6, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_default_context_name_impl },
  { BER_CLASS_CON, 8, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_presentation_requirements_impl },
  { BER_CLASS_CON, 9, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_user_session_requirements_impl },
  { BER_CLASS_CON, 11, BER_FLAGS_OPTIONAL, dissect_protocol_options },
  { BER_CLASS_CON, 12, BER_FLAGS_OPTIONAL, dissect_initiators_nominated_context },
  { BER_CLASS_CON, 14, BER_FLAGS_OPTIONAL, dissect_extensions },
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_user_data },
  { 0, 0, 0, NULL }
};

static int
dissect_pres_T_normal_mode_parameters(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       T_normal_mode_parameters_sequence, hf_index, ett_pres_T_normal_mode_parameters);

  return offset;
}
static int dissect_normal_mode_parameters_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_T_normal_mode_parameters(TRUE, tvb, offset, actx, tree, hf_pres_normal_mode_parameters);
}


static const ber_old_sequence_t CP_type_set[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_mode_selector_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_x410_mode_parameters_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_normal_mode_parameters_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_pres_CP_type(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_set(implicit_tag, actx, tree, tvb, offset,
                                  CP_type_set, hf_index, ett_pres_CP_type);

  return offset;
}



static int
dissect_pres_CPC_type(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_pres_User_data(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}



static int
dissect_pres_Responding_presentation_selector(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_pres_Presentation_selector(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_responding_presentation_selector_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_Responding_presentation_selector(TRUE, tvb, offset, actx, tree, hf_pres_responding_presentation_selector);
}


static const value_string pres_Result_vals[] = {
  {   0, "acceptance" },
  {   1, "user-rejection" },
  {   2, "provider-rejection" },
  { 0, NULL }
};


static int
dissect_pres_Result(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_result_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_Result(TRUE, tvb, offset, actx, tree, hf_pres_result);
}


static const value_string pres_T_provider_reason_vals[] = {
  {   0, "reason-not-specified" },
  {   1, "abstract-syntax-not-supported" },
  {   2, "proposed-transfer-syntaxes-not-supported" },
  {   3, "local-limit-on-DCS-exceeded" },
  { 0, NULL }
};


static int
dissect_pres_T_provider_reason(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_provider_reason_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_T_provider_reason(TRUE, tvb, offset, actx, tree, hf_pres_provider_reason);
}


static const ber_old_sequence_t Result_list_item_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_result_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_transfer_syntax_name_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_provider_reason_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_pres_Result_list_item(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       Result_list_item_sequence, hf_index, ett_pres_Result_list_item);

  return offset;
}
static int dissect_Result_list_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_Result_list_item(FALSE, tvb, offset, actx, tree, hf_pres_Result_list_item);
}


static const ber_old_sequence_t Result_list_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_Result_list_item },
};

static int
dissect_pres_Result_list(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                          Result_list_sequence_of, hf_index, ett_pres_Result_list);

  return offset;
}



static int
dissect_pres_Presentation_context_definition_result_list(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_pres_Result_list(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_presentation_context_definition_result_list_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_Presentation_context_definition_result_list(TRUE, tvb, offset, actx, tree, hf_pres_presentation_context_definition_result_list);
}


static const ber_old_sequence_t T_CPA_PPDU_normal_mode_parameters_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_protocol_version_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_responding_presentation_selector_impl },
  { BER_CLASS_CON, 5, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_presentation_context_definition_result_list_impl },
  { BER_CLASS_CON, 8, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_presentation_requirements_impl },
  { BER_CLASS_CON, 9, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_user_session_requirements_impl },
  { BER_CLASS_CON, 11, BER_FLAGS_OPTIONAL, dissect_protocol_options },
  { BER_CLASS_CON, 13, BER_FLAGS_OPTIONAL, dissect_responders_nominated_context },
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_user_data },
  { 0, 0, 0, NULL }
};

static int
dissect_pres_T_CPA_PPDU_normal_mode_parameters(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       T_CPA_PPDU_normal_mode_parameters_sequence, hf_index, ett_pres_T_CPA_PPDU_normal_mode_parameters);

  return offset;
}
static int dissect_cPU_PPDU_normal_mode_parameters_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_T_CPA_PPDU_normal_mode_parameters(TRUE, tvb, offset, actx, tree, hf_pres_cPU_PPDU_normal_mode_parameters);
}


static const ber_old_sequence_t CPA_PPDU_set[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_mode_selector_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_cPR_PPDU_x400_mode_parameters_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_cPU_PPDU_normal_mode_parameters_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_pres_CPA_PPDU(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_set(implicit_tag, actx, tree, tvb, offset,
                                  CPA_PPDU_set, hf_index, ett_pres_CPA_PPDU);

  return offset;
}



static int
dissect_pres_Default_context_result(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_pres_Result(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_default_context_result_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_Default_context_result(TRUE, tvb, offset, actx, tree, hf_pres_default_context_result);
}


static const value_string pres_Provider_reason_vals[] = {
  {   0, "reason-not-specified" },
  {   1, "temporary-congestion" },
  {   2, "local-limit-exceeded" },
  {   3, "called-presentation-address-unknown" },
  {   4, "protocol-version-not-supported" },
  {   5, "default-context-not-supported" },
  {   6, "user-data-not-readable" },
  {   7, "no-PSAP-available" },
  { 0, NULL }
};


static int
dissect_pres_Provider_reason(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_cPR_PPDU__provider_reason_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_Provider_reason(TRUE, tvb, offset, actx, tree, hf_pres_cPR_PPDU__provider_reason);
}


static const ber_old_sequence_t T_CPR_PPDU_normal_mode_parameters_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_protocol_version_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_responding_presentation_selector_impl },
  { BER_CLASS_CON, 5, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_presentation_context_definition_result_list_impl },
  { BER_CLASS_CON, 7, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_default_context_result_impl },
  { BER_CLASS_CON, 10, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_cPR_PPDU__provider_reason_impl },
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_user_data },
  { 0, 0, 0, NULL }
};

static int
dissect_pres_T_CPR_PPDU_normal_mode_parameters(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       T_CPR_PPDU_normal_mode_parameters_sequence, hf_index, ett_pres_T_CPR_PPDU_normal_mode_parameters);

  return offset;
}
static int dissect_cPR_PPDU_normal_mode_parameters(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_T_CPR_PPDU_normal_mode_parameters(FALSE, tvb, offset, actx, tree, hf_pres_cPR_PPDU_normal_mode_parameters);
}


static const value_string pres_CPR_PPDU_vals[] = {
  {   0, "x400-mode-parameters" },
  {   1, "normal-mode-parameters" },
  { 0, NULL }
};

static const ber_old_choice_t CPR_PPDU_choice[] = {
  {   0, BER_CLASS_UNI, BER_UNI_TAG_SET, BER_FLAGS_NOOWNTAG, dissect_cPU_PPDU_x400_mode_parameters },
  {   1, BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_cPR_PPDU_normal_mode_parameters },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_pres_CPR_PPDU(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_choice(actx, tree, tvb, offset,
                                     CPR_PPDU_choice, hf_index, ett_pres_CPR_PPDU,
                                     NULL);

  return offset;
}


static const ber_old_sequence_t Presentation_context_identifier_list_item_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_presentation_context_identifier },
  { BER_CLASS_UNI, BER_UNI_TAG_OID, BER_FLAGS_NOOWNTAG, dissect_transfer_syntax_name },
  { 0, 0, 0, NULL }
};

static int
dissect_pres_Presentation_context_identifier_list_item(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       Presentation_context_identifier_list_item_sequence, hf_index, ett_pres_Presentation_context_identifier_list_item);

  return offset;
}
static int dissect_Presentation_context_identifier_list_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_Presentation_context_identifier_list_item(FALSE, tvb, offset, actx, tree, hf_pres_Presentation_context_identifier_list_item);
}


static const ber_old_sequence_t Presentation_context_identifier_list_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_Presentation_context_identifier_list_item },
};

static int
dissect_pres_Presentation_context_identifier_list(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                          Presentation_context_identifier_list_sequence_of, hf_index, ett_pres_Presentation_context_identifier_list);

  return offset;
}
static int dissect_presentation_context_identifier_list_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_Presentation_context_identifier_list(TRUE, tvb, offset, actx, tree, hf_pres_presentation_context_identifier_list);
}


static const ber_old_sequence_t T_ARU_PPDU_normal_mode_parameters_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_presentation_context_identifier_list_impl },
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_user_data },
  { 0, 0, 0, NULL }
};

static int
dissect_pres_T_ARU_PPDU_normal_mode_parameters(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       T_ARU_PPDU_normal_mode_parameters_sequence, hf_index, ett_pres_T_ARU_PPDU_normal_mode_parameters);

  return offset;
}
static int dissect_aRU_PPDU_normal_mode_parameters_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_T_ARU_PPDU_normal_mode_parameters(TRUE, tvb, offset, actx, tree, hf_pres_aRU_PPDU_normal_mode_parameters);
}


static const value_string pres_ARU_PPDU_vals[] = {
  {   0, "x400-mode-parameters" },
  {   1, "normal-mode-parameters" },
  { 0, NULL }
};

static const ber_old_choice_t ARU_PPDU_choice[] = {
  {   0, BER_CLASS_UNI, BER_UNI_TAG_SET, BER_FLAGS_NOOWNTAG, dissect_aRU_PPDU_x400_mode_parameters },
  {   1, BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_aRU_PPDU_normal_mode_parameters_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_pres_ARU_PPDU(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_choice(actx, tree, tvb, offset,
                                     ARU_PPDU_choice, hf_index, ett_pres_ARU_PPDU,
                                     NULL);

  return offset;
}
static int dissect_aru_ppdu(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_ARU_PPDU(FALSE, tvb, offset, actx, tree, hf_pres_aru_ppdu);
}


static const value_string pres_Abort_reason_vals[] = {
  {   0, "reason-not-specified" },
  {   1, "unrecognized-ppdu" },
  {   2, "unexpected-ppdu" },
  {   3, "unexpected-session-service-primitive" },
  {   4, "unrecognized-ppdu-parameter" },
  {   5, "unexpected-ppdu-parameter" },
  {   6, "invalid-ppdu-parameter-value" },
  { 0, NULL }
};


static int
dissect_pres_Abort_reason(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_aRU_PPDU_provider_reason_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_Abort_reason(TRUE, tvb, offset, actx, tree, hf_pres_aRU_PPDU_provider_reason);
}


static const value_string pres_Event_identifier_vals[] = {
  {   0, "cp-PPDU" },
  {   1, "cpa-PPDU" },
  {   2, "cpr-PPDU" },
  {   3, "aru-PPDU" },
  {   4, "arp-PPDU" },
  {   5, "ac-PPDU" },
  {   6, "aca-PPDU" },
  {   7, "td-PPDU" },
  {   8, "ttd-PPDU" },
  {   9, "te-PPDU" },
  {  10, "tc-PPDU" },
  {  11, "tcc-PPDU" },
  {  12, "rs-PPDU" },
  {  13, "rsa-PPDU" },
  {  14, "s-release-indication" },
  {  15, "s-release-confirm" },
  {  16, "s-token-give-indication" },
  {  17, "s-token-please-indication" },
  {  18, "s-control-give-indication" },
  {  19, "s-sync-minor-indication" },
  {  20, "s-sync-minor-confirm" },
  {  21, "s-sync-major-indication" },
  {  22, "s-sync-major-confirm" },
  {  23, "s-p-exception-report-indication" },
  {  24, "s-u-exception-report-indication" },
  {  25, "s-activity-start-indication" },
  {  26, "s-activity-resume-indication" },
  {  27, "s-activity-interrupt-indication" },
  {  28, "s-activity-interrupt-confirm" },
  {  29, "s-activity-discard-indication" },
  {  30, "s-activity-discard-confirm" },
  {  31, "s-activity-end-indication" },
  {  32, "s-activity-end-confirm" },
  { 0, NULL }
};


static int
dissect_pres_Event_identifier(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_event_identifier_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_Event_identifier(TRUE, tvb, offset, actx, tree, hf_pres_event_identifier);
}


static const ber_old_sequence_t ARP_PPDU_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_aRU_PPDU_provider_reason_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_event_identifier_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_pres_ARP_PPDU(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       ARP_PPDU_sequence, hf_index, ett_pres_ARP_PPDU);

  return offset;
}
static int dissect_arp_ppdu(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_ARP_PPDU(FALSE, tvb, offset, actx, tree, hf_pres_arp_ppdu);
}


static const value_string pres_Abort_type_vals[] = {
  {   0, "aru-ppdu" },
  {   1, "arp-ppdu" },
  { 0, NULL }
};

static const ber_old_choice_t Abort_type_choice[] = {
  {   0, BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG, dissect_aru_ppdu },
  {   1, BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_arp_ppdu },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_pres_Abort_type(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_choice(actx, tree, tvb, offset,
                                     Abort_type_choice, hf_index, ett_pres_Abort_type,
                                     NULL);

  return offset;
}



static int
dissect_pres_Presentation_context_addition_list(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_pres_Context_list(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_presentation_context_addition_list_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_Presentation_context_addition_list(TRUE, tvb, offset, actx, tree, hf_pres_presentation_context_addition_list);
}


static const ber_old_sequence_t Presentation_context_deletion_list_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_Presentation_context_deletion_list_item },
};

static int
dissect_pres_Presentation_context_deletion_list(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                          Presentation_context_deletion_list_sequence_of, hf_index, ett_pres_Presentation_context_deletion_list);

  return offset;
}
static int dissect_presentation_context_deletion_list_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_Presentation_context_deletion_list(TRUE, tvb, offset, actx, tree, hf_pres_presentation_context_deletion_list);
}


static const ber_old_sequence_t AC_PPDU_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_presentation_context_addition_list_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_presentation_context_deletion_list_impl },
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_user_data },
  { 0, 0, 0, NULL }
};

static int
dissect_pres_AC_PPDU(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       AC_PPDU_sequence, hf_index, ett_pres_AC_PPDU);

  return offset;
}
static int dissect_acPPDU_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_AC_PPDU(TRUE, tvb, offset, actx, tree, hf_pres_acPPDU);
}



static int
dissect_pres_Presentation_context_addition_result_list(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_pres_Result_list(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_presentation_context_addition_result_list_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_Presentation_context_addition_result_list(TRUE, tvb, offset, actx, tree, hf_pres_presentation_context_addition_result_list);
}


static const value_string pres_Presentation_context_deletion_result_list_item_vals[] = {
  {   0, "acceptance" },
  {   1, "user-rejection" },
  { 0, NULL }
};


static int
dissect_pres_Presentation_context_deletion_result_list_item(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_Presentation_context_deletion_result_list_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_Presentation_context_deletion_result_list_item(FALSE, tvb, offset, actx, tree, hf_pres_Presentation_context_deletion_result_list_item);
}


static const ber_old_sequence_t Presentation_context_deletion_result_list_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_Presentation_context_deletion_result_list_item },
};

static int
dissect_pres_Presentation_context_deletion_result_list(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                          Presentation_context_deletion_result_list_sequence_of, hf_index, ett_pres_Presentation_context_deletion_result_list);

  return offset;
}
static int dissect_presentation_context_deletion_result_list_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_Presentation_context_deletion_result_list(TRUE, tvb, offset, actx, tree, hf_pres_presentation_context_deletion_result_list);
}


static const ber_old_sequence_t ACA_PPDU_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_presentation_context_addition_result_list_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_presentation_context_deletion_result_list_impl },
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_user_data },
  { 0, 0, 0, NULL }
};

static int
dissect_pres_ACA_PPDU(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       ACA_PPDU_sequence, hf_index, ett_pres_ACA_PPDU);

  return offset;
}
static int dissect_acaPPDU_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_pres_ACA_PPDU(TRUE, tvb, offset, actx, tree, hf_pres_acaPPDU);
}


static const value_string pres_Typed_data_type_vals[] = {
  {   0, "acPPDU" },
  {   1, "acaPPDU" },
  {   2, "ttdPPDU" },
  { 0, NULL }
};

static const ber_old_choice_t Typed_data_type_choice[] = {
  {   0, BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_acPPDU_impl },
  {   1, BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_acaPPDU_impl },
  {   2, BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG, dissect_ttdPPDU },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_pres_Typed_data_type(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_choice(actx, tree, tvb, offset,
                                     Typed_data_type_choice, hf_index, ett_pres_Typed_data_type,
                                     NULL);

  return offset;
}


static const ber_old_sequence_t RS_PPDU_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_presentation_context_identifier_list_impl },
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_user_data },
  { 0, 0, 0, NULL }
};

static int
dissect_pres_RS_PPDU(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       RS_PPDU_sequence, hf_index, ett_pres_RS_PPDU);

  return offset;
}


static const ber_old_sequence_t RSA_PPDU_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_presentation_context_identifier_list_impl },
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_user_data },
  { 0, 0, 0, NULL }
};

static int
dissect_pres_RSA_PPDU(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       RSA_PPDU_sequence, hf_index, ett_pres_RSA_PPDU);

  return offset;
}


/*--- End of included file: packet-pres-fn.c ---*/
#line 168 "packet-pres-template.c"


/*
 * Dissect an PPDU.
 */
static int
dissect_ppdu(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree)
{
  proto_item *ti;
  proto_tree *pres_tree = NULL;
  guint s_type;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);

  /* do we have spdu type from the session dissector?  */
	if( !pinfo->private_data ){
		if(tree){
			proto_tree_add_text(tree, tvb, offset, -1,
				"Internal error:can't get spdu type from session dissector.");
			return  FALSE;
		}
	}else{
		session  = ( (struct SESSION_DATA_STRUCTURE*)(pinfo->private_data) );
		if(session->spdu_type == 0 ){
			if(tree){
				proto_tree_add_text(tree, tvb, offset, -1,
					"Internal error:wrong spdu type %x from session dissector.",session->spdu_type);
				return  FALSE;
			}
		}
	}
/* get type of tag      */
	s_type = tvb_get_guint8(tvb, offset);
		/*  set up type of Ppdu */
  	if (check_col(pinfo->cinfo, COL_INFO))
					col_add_str(pinfo->cinfo, COL_INFO,
						val_to_str(session->spdu_type, ses_vals, "Unknown Ppdu type (0x%02x)"));
  if (tree){
	  ti = proto_tree_add_item(tree, proto_pres, tvb, offset, -1,
		    FALSE);
	  pres_tree = proto_item_add_subtree(ti, ett_pres);
  }

	switch(session->spdu_type){
		case SES_CONNECTION_REQUEST:
			offset = dissect_pres_CP_type(FALSE, tvb, offset, &asn1_ctx, pres_tree, hf_pres_CP_type);
			break;
		case SES_CONNECTION_ACCEPT:
			offset = dissect_pres_CPA_PPDU(FALSE, tvb, offset, &asn1_ctx, pres_tree, hf_pres_CPA_PPDU);
			break;
		case SES_ABORT:
		case SES_ABORT_ACCEPT:
			offset = dissect_pres_Abort_type(FALSE, tvb, offset, &asn1_ctx, pres_tree, hf_pres_Abort_type);
			break;
		case SES_DATA_TRANSFER:
			offset = dissect_pres_CPC_type(FALSE, tvb, offset, &asn1_ctx, pres_tree, hf_pres_user_data);
			break;
		case SES_TYPED_DATA:
			offset = dissect_pres_Typed_data_type(FALSE, tvb, offset, &asn1_ctx, pres_tree, hf_pres_Typed_data_type);
			break;
		case SES_RESYNCHRONIZE:
			offset = dissect_pres_RS_PPDU(FALSE, tvb, offset, &asn1_ctx, pres_tree, -1);
			break;
		case SES_RESYNCHRONIZE_ACK:
			offset = dissect_pres_RSA_PPDU(FALSE, tvb, offset, &asn1_ctx, pres_tree, -1);
			break;
		case SES_REFUSE:
			offset = dissect_pres_CPR_PPDU(FALSE, tvb, offset, &asn1_ctx, pres_tree, hf_pres_CPR_PPDU);
			break;
		default:
			offset = dissect_pres_CPC_type(FALSE, tvb, offset, &asn1_ctx, pres_tree, hf_pres_user_data);
			break;
	}

	return offset;
}

void
dissect_pres(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree)
{
	int offset = 0, old_offset;
/* first, try to check length   */
/* do we have at least 4 bytes  */
	if (!tvb_bytes_exist(tvb, 0, 4)){
		session = ((struct SESSION_DATA_STRUCTURE*)(pinfo->private_data));
		if (session && session->spdu_type != SES_MAJOR_SYNC_POINT) {
			proto_tree_add_text(parent_tree, tvb, offset, 
					    tvb_reported_length_remaining(tvb,offset),"User data");
			return;  /* no, it isn't a presentation PDU */
		}
	}

	/*  we can't make any additional checking here   */
	/*  postpone it before dissector will have more information */

	if (check_col(pinfo->cinfo, COL_PROTOCOL))
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "PRES");
  	if (check_col(pinfo->cinfo, COL_INFO))
  		col_clear(pinfo->cinfo, COL_INFO);
	/* save pointers for calling the acse dissector  */
	global_tree = parent_tree;
	global_pinfo = pinfo;

	if (session && session->spdu_type == SES_MAJOR_SYNC_POINT) {
		/* This is a reassembly initiated in packet-ses */
		char *oid = find_oid_by_pres_ctx_id (pinfo, session->pres_ctx_id);
		if (oid) {
			call_ber_oid_callback (oid, tvb, offset, pinfo, parent_tree);
		} else {
			proto_tree_add_text(parent_tree, tvb, offset, 
					    tvb_reported_length_remaining(tvb,offset),"User data");
		}
		return;
         }
            
	while (tvb_reported_length_remaining(tvb, offset) > 0){
		old_offset = offset;
		offset = dissect_ppdu(tvb, offset, pinfo, parent_tree);
		if(offset <= old_offset){
			proto_tree_add_text(parent_tree, tvb, offset, -1,"Invalid offset");
			THROW(ReportedBoundsError);
		}
	}
}


/*--- proto_register_pres -------------------------------------------*/
void proto_register_pres(void) {

  /* List of fields */
  static hf_register_info hf[] = {
    { &hf_pres_CP_type,
      { "CP-type", "pres.cptype",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_pres_CPA_PPDU,
      { "CPA-PPDU", "pres.cpapdu",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_pres_Abort_type,
      { "Abort type", "pres.aborttype",
        FT_UINT32, BASE_DEC, VALS(pres_Abort_type_vals), 0,
        "", HFILL }},
    { &hf_pres_CPR_PPDU,
      { "CPR-PPDU", "pres.cprtype",
        FT_UINT32, BASE_DEC, VALS(pres_CPR_PPDU_vals), 0,
        "", HFILL }},
    { &hf_pres_Typed_data_type,
      { "Typed data type", "pres.Typed_data_type",
        FT_UINT32, BASE_DEC, VALS(pres_Typed_data_type_vals), 0,
        "", HFILL }},



/*--- Included file: packet-pres-hfarr.c ---*/
#line 1 "packet-pres-hfarr.c"
    { &hf_pres_mode_selector,
      { "mode-selector", "pres.mode_selector",
        FT_NONE, BASE_NONE, NULL, 0,
        "pres.Mode_selector", HFILL }},
    { &hf_pres_x410_mode_parameters,
      { "x410-mode-parameters", "pres.x410_mode_parameters",
        FT_NONE, BASE_NONE, NULL, 0,
        "rtse.RTORQapdu", HFILL }},
    { &hf_pres_normal_mode_parameters,
      { "normal-mode-parameters", "pres.normal_mode_parameters",
        FT_NONE, BASE_NONE, NULL, 0,
        "pres.T_normal_mode_parameters", HFILL }},
    { &hf_pres_protocol_version,
      { "protocol-version", "pres.protocol_version",
        FT_BYTES, BASE_HEX, NULL, 0,
        "pres.Protocol_version", HFILL }},
    { &hf_pres_calling_presentation_selector,
      { "calling-presentation-selector", "pres.calling_presentation_selector",
        FT_BYTES, BASE_HEX, NULL, 0,
        "pres.Calling_presentation_selector", HFILL }},
    { &hf_pres_called_presentation_selector,
      { "called-presentation-selector", "pres.called_presentation_selector",
        FT_BYTES, BASE_HEX, NULL, 0,
        "pres.Called_presentation_selector", HFILL }},
    { &hf_pres_presentation_context_definition_list,
      { "presentation-context-definition-list", "pres.presentation_context_definition_list",
        FT_UINT32, BASE_DEC, NULL, 0,
        "pres.Presentation_context_definition_list", HFILL }},
    { &hf_pres_default_context_name,
      { "default-context-name", "pres.default_context_name",
        FT_NONE, BASE_NONE, NULL, 0,
        "pres.Default_context_name", HFILL }},
    { &hf_pres_presentation_requirements,
      { "presentation-requirements", "pres.presentation_requirements",
        FT_BYTES, BASE_HEX, NULL, 0,
        "pres.Presentation_requirements", HFILL }},
    { &hf_pres_user_session_requirements,
      { "user-session-requirements", "pres.user_session_requirements",
        FT_BYTES, BASE_HEX, NULL, 0,
        "pres.User_session_requirements", HFILL }},
    { &hf_pres_protocol_options,
      { "protocol-options", "pres.protocol_options",
        FT_BYTES, BASE_HEX, NULL, 0,
        "pres.Protocol_options", HFILL }},
    { &hf_pres_initiators_nominated_context,
      { "initiators-nominated-context", "pres.initiators_nominated_context",
        FT_INT32, BASE_DEC, NULL, 0,
        "pres.Presentation_context_identifier", HFILL }},
    { &hf_pres_extensions,
      { "extensions", "pres.extensions",
        FT_NONE, BASE_NONE, NULL, 0,
        "pres.T_extensions", HFILL }},
    { &hf_pres_user_data,
      { "user-data", "pres.user_data",
        FT_UINT32, BASE_DEC, VALS(pres_User_data_vals), 0,
        "pres.User_data", HFILL }},
    { &hf_pres_cPR_PPDU_x400_mode_parameters,
      { "x410-mode-parameters", "pres.x410_mode_parameters",
        FT_NONE, BASE_NONE, NULL, 0,
        "rtse.RTOACapdu", HFILL }},
    { &hf_pres_cPU_PPDU_normal_mode_parameters,
      { "normal-mode-parameters", "pres.normal_mode_parameters",
        FT_NONE, BASE_NONE, NULL, 0,
        "pres.T_CPA_PPDU_normal_mode_parameters", HFILL }},
    { &hf_pres_responding_presentation_selector,
      { "responding-presentation-selector", "pres.responding_presentation_selector",
        FT_BYTES, BASE_HEX, NULL, 0,
        "pres.Responding_presentation_selector", HFILL }},
    { &hf_pres_presentation_context_definition_result_list,
      { "presentation-context-definition-result-list", "pres.presentation_context_definition_result_list",
        FT_UINT32, BASE_DEC, NULL, 0,
        "pres.Presentation_context_definition_result_list", HFILL }},
    { &hf_pres_responders_nominated_context,
      { "responders-nominated-context", "pres.responders_nominated_context",
        FT_INT32, BASE_DEC, NULL, 0,
        "pres.Presentation_context_identifier", HFILL }},
    { &hf_pres_cPU_PPDU_x400_mode_parameters,
      { "x400-mode-parameters", "pres.x400_mode_parameters",
        FT_NONE, BASE_NONE, NULL, 0,
        "rtse.RTORJapdu", HFILL }},
    { &hf_pres_cPR_PPDU_normal_mode_parameters,
      { "normal-mode-parameters", "pres.normal_mode_parameters",
        FT_NONE, BASE_NONE, NULL, 0,
        "pres.T_CPR_PPDU_normal_mode_parameters", HFILL }},
    { &hf_pres_default_context_result,
      { "default-context-result", "pres.default_context_result",
        FT_INT32, BASE_DEC, VALS(pres_Result_vals), 0,
        "pres.Default_context_result", HFILL }},
    { &hf_pres_cPR_PPDU__provider_reason,
      { "provider-reason", "pres.provider_reason",
        FT_INT32, BASE_DEC, VALS(pres_Provider_reason_vals), 0,
        "pres.Provider_reason", HFILL }},
    { &hf_pres_aru_ppdu,
      { "aru-ppdu", "pres.aru_ppdu",
        FT_UINT32, BASE_DEC, VALS(pres_ARU_PPDU_vals), 0,
        "pres.ARU_PPDU", HFILL }},
    { &hf_pres_arp_ppdu,
      { "arp-ppdu", "pres.arp_ppdu",
        FT_NONE, BASE_NONE, NULL, 0,
        "pres.ARP_PPDU", HFILL }},
    { &hf_pres_aRU_PPDU_x400_mode_parameters,
      { "x400-mode-parameters", "pres.x400_mode_parameters",
        FT_NONE, BASE_NONE, NULL, 0,
        "rtse.RTABapdu", HFILL }},
    { &hf_pres_aRU_PPDU_normal_mode_parameters,
      { "normal-mode-parameters", "pres.normal_mode_parameters",
        FT_NONE, BASE_NONE, NULL, 0,
        "pres.T_ARU_PPDU_normal_mode_parameters", HFILL }},
    { &hf_pres_presentation_context_identifier_list,
      { "presentation-context-identifier-list", "pres.presentation_context_identifier_list",
        FT_UINT32, BASE_DEC, NULL, 0,
        "pres.Presentation_context_identifier_list", HFILL }},
    { &hf_pres_aRU_PPDU_provider_reason,
      { "provider-reason", "pres.provider_reason",
        FT_INT32, BASE_DEC, VALS(pres_Abort_reason_vals), 0,
        "pres.Abort_reason", HFILL }},
    { &hf_pres_event_identifier,
      { "event-identifier", "pres.event_identifier",
        FT_INT32, BASE_DEC, VALS(pres_Event_identifier_vals), 0,
        "pres.Event_identifier", HFILL }},
    { &hf_pres_acPPDU,
      { "acPPDU", "pres.acPPDU",
        FT_NONE, BASE_NONE, NULL, 0,
        "pres.AC_PPDU", HFILL }},
    { &hf_pres_acaPPDU,
      { "acaPPDU", "pres.acaPPDU",
        FT_NONE, BASE_NONE, NULL, 0,
        "pres.ACA_PPDU", HFILL }},
    { &hf_pres_ttdPPDU,
      { "ttdPPDU", "pres.ttdPPDU",
        FT_UINT32, BASE_DEC, VALS(pres_User_data_vals), 0,
        "pres.User_data", HFILL }},
    { &hf_pres_presentation_context_addition_list,
      { "presentation-context-addition-list", "pres.presentation_context_addition_list",
        FT_UINT32, BASE_DEC, NULL, 0,
        "pres.Presentation_context_addition_list", HFILL }},
    { &hf_pres_presentation_context_deletion_list,
      { "presentation-context-deletion-list", "pres.presentation_context_deletion_list",
        FT_UINT32, BASE_DEC, NULL, 0,
        "pres.Presentation_context_deletion_list", HFILL }},
    { &hf_pres_presentation_context_addition_result_list,
      { "presentation-context-addition-result-list", "pres.presentation_context_addition_result_list",
        FT_UINT32, BASE_DEC, NULL, 0,
        "pres.Presentation_context_addition_result_list", HFILL }},
    { &hf_pres_presentation_context_deletion_result_list,
      { "presentation-context-deletion-result-list", "pres.presentation_context_deletion_result_list",
        FT_UINT32, BASE_DEC, NULL, 0,
        "pres.Presentation_context_deletion_result_list", HFILL }},
    { &hf_pres_Context_list_item,
      { "Item", "pres.Context_list_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "pres.Context_list_item", HFILL }},
    { &hf_pres_presentation_context_identifier,
      { "presentation-context-identifier", "pres.presentation_context_identifier",
        FT_INT32, BASE_DEC, NULL, 0,
        "pres.Presentation_context_identifier", HFILL }},
    { &hf_pres_abstract_syntax_name,
      { "abstract-syntax-name", "pres.abstract_syntax_name",
        FT_OID, BASE_NONE, NULL, 0,
        "pres.Abstract_syntax_name", HFILL }},
    { &hf_pres_transfer_syntax_name_list,
      { "transfer-syntax-name-list", "pres.transfer_syntax_name_list",
        FT_UINT32, BASE_DEC, NULL, 0,
        "pres.SEQUENCE_OF_Transfer_syntax_name", HFILL }},
    { &hf_pres_transfer_syntax_name_list_item,
      { "Item", "pres.transfer_syntax_name_list_item",
        FT_OID, BASE_NONE, NULL, 0,
        "pres.Transfer_syntax_name", HFILL }},
    { &hf_pres_transfer_syntax_name,
      { "transfer-syntax-name", "pres.transfer_syntax_name",
        FT_OID, BASE_NONE, NULL, 0,
        "pres.Transfer_syntax_name", HFILL }},
    { &hf_pres_mode_value,
      { "mode-value", "pres.mode_value",
        FT_INT32, BASE_DEC, VALS(pres_T_mode_value_vals), 0,
        "pres.T_mode_value", HFILL }},
    { &hf_pres_Presentation_context_deletion_list_item,
      { "Item", "pres.Presentation_context_deletion_list_item",
        FT_INT32, BASE_DEC, NULL, 0,
        "pres.Presentation_context_identifier", HFILL }},
    { &hf_pres_Presentation_context_deletion_result_list_item,
      { "Item", "pres.Presentation_context_deletion_result_list_item",
        FT_INT32, BASE_DEC, VALS(pres_Presentation_context_deletion_result_list_item_vals), 0,
        "pres.Presentation_context_deletion_result_list_item", HFILL }},
    { &hf_pres_Presentation_context_identifier_list_item,
      { "Item", "pres.Presentation_context_identifier_list_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "pres.Presentation_context_identifier_list_item", HFILL }},
    { &hf_pres_Result_list_item,
      { "Item", "pres.Result_list_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "pres.Result_list_item", HFILL }},
    { &hf_pres_result,
      { "result", "pres.result",
        FT_INT32, BASE_DEC, VALS(pres_Result_vals), 0,
        "pres.Result", HFILL }},
    { &hf_pres_provider_reason,
      { "provider-reason", "pres.provider_reason",
        FT_INT32, BASE_DEC, VALS(pres_T_provider_reason_vals), 0,
        "pres.T_provider_reason", HFILL }},
    { &hf_pres_simply_encoded_data,
      { "simply-encoded-data", "pres.simply_encoded_data",
        FT_BYTES, BASE_HEX, NULL, 0,
        "pres.Simply_encoded_data", HFILL }},
    { &hf_pres_fully_encoded_data,
      { "fully-encoded-data", "pres.fully_encoded_data",
        FT_UINT32, BASE_DEC, NULL, 0,
        "pres.Fully_encoded_data", HFILL }},
    { &hf_pres_Fully_encoded_data_item,
      { "Item", "pres.Fully_encoded_data_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "pres.PDV_list", HFILL }},
    { &hf_pres_presentation_data_values,
      { "presentation-data-values", "pres.presentation_data_values",
        FT_UINT32, BASE_DEC, VALS(pres_T_presentation_data_values_vals), 0,
        "pres.T_presentation_data_values", HFILL }},
    { &hf_pres_single_ASN1_type,
      { "single-ASN1-type", "pres.single_ASN1_type",
        FT_BYTES, BASE_HEX, NULL, 0,
        "pres.T_single_ASN1_type", HFILL }},
    { &hf_pres_octet_aligned,
      { "octet-aligned", "pres.octet_aligned",
        FT_BYTES, BASE_HEX, NULL, 0,
        "pres.T_octet_aligned", HFILL }},
    { &hf_pres_arbitrary,
      { "arbitrary", "pres.arbitrary",
        FT_BYTES, BASE_HEX, NULL, 0,
        "pres.BIT_STRING", HFILL }},
    { &hf_pres_Presentation_requirements_context_management,
      { "context-management", "pres.context-management",
        FT_BOOLEAN, 8, NULL, 0x80,
        "", HFILL }},
    { &hf_pres_Presentation_requirements_restoration,
      { "restoration", "pres.restoration",
        FT_BOOLEAN, 8, NULL, 0x40,
        "", HFILL }},
    { &hf_pres_Protocol_options_nominated_context,
      { "nominated-context", "pres.nominated-context",
        FT_BOOLEAN, 8, NULL, 0x80,
        "", HFILL }},
    { &hf_pres_Protocol_options_short_encoding,
      { "short-encoding", "pres.short-encoding",
        FT_BOOLEAN, 8, NULL, 0x40,
        "", HFILL }},
    { &hf_pres_Protocol_options_packed_encoding_rules,
      { "packed-encoding-rules", "pres.packed-encoding-rules",
        FT_BOOLEAN, 8, NULL, 0x20,
        "", HFILL }},
    { &hf_pres_Protocol_version_version_1,
      { "version-1", "pres.version-1",
        FT_BOOLEAN, 8, NULL, 0x80,
        "", HFILL }},
    { &hf_pres_User_session_requirements_half_duplex,
      { "half-duplex", "pres.half-duplex",
        FT_BOOLEAN, 8, NULL, 0x80,
        "", HFILL }},
    { &hf_pres_User_session_requirements_duplex,
      { "duplex", "pres.duplex",
        FT_BOOLEAN, 8, NULL, 0x40,
        "", HFILL }},
    { &hf_pres_User_session_requirements_expedited_data,
      { "expedited-data", "pres.expedited-data",
        FT_BOOLEAN, 8, NULL, 0x20,
        "", HFILL }},
    { &hf_pres_User_session_requirements_minor_synchronize,
      { "minor-synchronize", "pres.minor-synchronize",
        FT_BOOLEAN, 8, NULL, 0x10,
        "", HFILL }},
    { &hf_pres_User_session_requirements_major_synchronize,
      { "major-synchronize", "pres.major-synchronize",
        FT_BOOLEAN, 8, NULL, 0x08,
        "", HFILL }},
    { &hf_pres_User_session_requirements_resynchronize,
      { "resynchronize", "pres.resynchronize",
        FT_BOOLEAN, 8, NULL, 0x04,
        "", HFILL }},
    { &hf_pres_User_session_requirements_activity_management,
      { "activity-management", "pres.activity-management",
        FT_BOOLEAN, 8, NULL, 0x02,
        "", HFILL }},
    { &hf_pres_User_session_requirements_negotiated_release,
      { "negotiated-release", "pres.negotiated-release",
        FT_BOOLEAN, 8, NULL, 0x01,
        "", HFILL }},
    { &hf_pres_User_session_requirements_capability_data,
      { "capability-data", "pres.capability-data",
        FT_BOOLEAN, 8, NULL, 0x80,
        "", HFILL }},
    { &hf_pres_User_session_requirements_exceptions,
      { "exceptions", "pres.exceptions",
        FT_BOOLEAN, 8, NULL, 0x40,
        "", HFILL }},
    { &hf_pres_User_session_requirements_typed_data,
      { "typed-data", "pres.typed-data",
        FT_BOOLEAN, 8, NULL, 0x20,
        "", HFILL }},
    { &hf_pres_User_session_requirements_symmetric_synchronize,
      { "symmetric-synchronize", "pres.symmetric-synchronize",
        FT_BOOLEAN, 8, NULL, 0x10,
        "", HFILL }},
    { &hf_pres_User_session_requirements_data_separation,
      { "data-separation", "pres.data-separation",
        FT_BOOLEAN, 8, NULL, 0x08,
        "", HFILL }},

/*--- End of included file: packet-pres-hfarr.c ---*/
#line 322 "packet-pres-template.c"
  };

  /* List of subtrees */
  static gint *ett[] = {
		&ett_pres,

/*--- Included file: packet-pres-ettarr.c ---*/
#line 1 "packet-pres-ettarr.c"
    &ett_pres_CP_type,
    &ett_pres_T_normal_mode_parameters,
    &ett_pres_T_extensions,
    &ett_pres_CPA_PPDU,
    &ett_pres_T_CPA_PPDU_normal_mode_parameters,
    &ett_pres_CPR_PPDU,
    &ett_pres_T_CPR_PPDU_normal_mode_parameters,
    &ett_pres_Abort_type,
    &ett_pres_ARU_PPDU,
    &ett_pres_T_ARU_PPDU_normal_mode_parameters,
    &ett_pres_ARP_PPDU,
    &ett_pres_Typed_data_type,
    &ett_pres_AC_PPDU,
    &ett_pres_ACA_PPDU,
    &ett_pres_RS_PPDU,
    &ett_pres_RSA_PPDU,
    &ett_pres_Context_list,
    &ett_pres_Context_list_item,
    &ett_pres_SEQUENCE_OF_Transfer_syntax_name,
    &ett_pres_Default_context_name,
    &ett_pres_Mode_selector,
    &ett_pres_Presentation_context_deletion_list,
    &ett_pres_Presentation_context_deletion_result_list,
    &ett_pres_Presentation_context_identifier_list,
    &ett_pres_Presentation_context_identifier_list_item,
    &ett_pres_Presentation_requirements,
    &ett_pres_Protocol_options,
    &ett_pres_Protocol_version,
    &ett_pres_Result_list,
    &ett_pres_Result_list_item,
    &ett_pres_User_data,
    &ett_pres_Fully_encoded_data,
    &ett_pres_PDV_list,
    &ett_pres_T_presentation_data_values,
    &ett_pres_User_session_requirements,

/*--- End of included file: packet-pres-ettarr.c ---*/
#line 328 "packet-pres-template.c"
  };

  /* Register protocol */
  proto_pres = proto_register_protocol(PNAME, PSNAME, PFNAME);
  register_dissector("pres", dissect_pres, proto_pres);
  /* Register fields and subtrees */
  proto_register_field_array(proto_pres, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));
  register_init_routine(pres_init);

}


/*--- proto_reg_handoff_pres ---------------------------------------*/
void proto_reg_handoff_pres(void) {

/*	register_ber_oid_dissector("0.4.0.0.1.1.1.1", dissect_pres, proto_pres, 
	  "itu-t(0) identified-organization(4) etsi(0) mobileDomain(0) gsm-Network(1) abstractSyntax(1) pres(1) version1(1)"); */

	data_handle = find_dissector("data");

}
