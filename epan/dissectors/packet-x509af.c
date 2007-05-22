/* Do not modify this file.                                                   */
/* It is created automatically by the ASN.1 to Wireshark dissector compiler   */
/* .\packet-x509af.c                                                          */
/* ../../tools/asn2wrs.py -b -e -p x509af -c x509af.cnf -s packet-x509af-template AuthenticationFramework.asn */

/* Input file: packet-x509af-template.c */

#line 1 "packet-x509af-template.c"
/* packet-x509af.c
 * Routines for X.509 Authentication Framework packet dissection
 *  Ronnie Sahlberg 2004
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
#include <epan/oid_resolv.h>
#include <epan/asn1.h>

#include <stdio.h>
#include <string.h>

#include "packet-ber.h"
#include "packet-x509af.h"
#include "packet-x509ce.h"
#include "packet-x509if.h"
#include "packet-x509sat.h"
#include "packet-ldap.h"

#define PNAME  "X.509 Authentication Framework"
#define PSNAME "X509AF"
#define PFNAME "x509af"

/* Initialize the protocol and registered fields */
static int proto_x509af = -1;
static int hf_x509af_algorithm_id = -1;
static int hf_x509af_extension_id = -1;

/*--- Included file: packet-x509af-hf.c ---*/
#line 1 "packet-x509af-hf.c"
static int hf_x509af_Certificate_PDU = -1;        /* Certificate */
static int hf_x509af_CertificatePair_PDU = -1;    /* CertificatePair */
static int hf_x509af_CertificateList_PDU = -1;    /* CertificateList */
static int hf_x509af_AttributeCertificate_PDU = -1;  /* AttributeCertificate */
static int hf_x509af_DSS_Params_PDU = -1;         /* DSS_Params */
static int hf_x509af_signedCertificate = -1;      /* T_signedCertificate */
static int hf_x509af_version = -1;                /* Version */
static int hf_x509af_serialNumber = -1;           /* CertificateSerialNumber */
static int hf_x509af_signature = -1;              /* AlgorithmIdentifier */
static int hf_x509af_issuer = -1;                 /* Name */
static int hf_x509af_validity = -1;               /* Validity */
static int hf_x509af_subject = -1;                /* SubjectName */
static int hf_x509af_subjectPublicKeyInfo = -1;   /* SubjectPublicKeyInfo */
static int hf_x509af_issuerUniqueIdentifier = -1;  /* UniqueIdentifier */
static int hf_x509af_subjectUniqueIdentifier = -1;  /* UniqueIdentifier */
static int hf_x509af_extensions = -1;             /* Extensions */
static int hf_x509af_algorithmIdentifier = -1;    /* AlgorithmIdentifier */
static int hf_x509af_encrypted = -1;              /* BIT_STRING */
static int hf_x509af_rdnSequence = -1;            /* RDNSequence */
static int hf_x509af_algorithmId = -1;            /* T_algorithmId */
static int hf_x509af_parameters = -1;             /* T_parameters */
static int hf_x509af_notBefore = -1;              /* Time */
static int hf_x509af_notAfter = -1;               /* Time */
static int hf_x509af_algorithm = -1;              /* AlgorithmIdentifier */
static int hf_x509af_subjectPublicKey = -1;       /* BIT_STRING */
static int hf_x509af_utcTime = -1;                /* UTCTime */
static int hf_x509af_generalizedTime = -1;        /* GeneralizedTime */
static int hf_x509af_Extensions_item = -1;        /* Extension */
static int hf_x509af_extnId = -1;                 /* T_extnId */
static int hf_x509af_critical = -1;               /* BOOLEAN */
static int hf_x509af_extnValue = -1;              /* T_extnValue */
static int hf_x509af_userCertificate = -1;        /* Certificate */
static int hf_x509af_certificationPath = -1;      /* ForwardCertificationPath */
static int hf_x509af_ForwardCertificationPath_item = -1;  /* CrossCertificates */
static int hf_x509af_CrossCertificates_item = -1;  /* Certificate */
static int hf_x509af_theCACertificates = -1;      /* SEQUENCE_OF_CertificatePair */
static int hf_x509af_theCACertificates_item = -1;  /* CertificatePair */
static int hf_x509af_issuedByThisCA = -1;         /* Certificate */
static int hf_x509af_issuedToThisCA = -1;         /* Certificate */
static int hf_x509af_signedCertificateList = -1;  /* T_signedCertificateList */
static int hf_x509af_thisUpdate = -1;             /* Time */
static int hf_x509af_nextUpdate = -1;             /* Time */
static int hf_x509af_revokedCertificates = -1;    /* T_revokedCertificates */
static int hf_x509af_revokedCertificates_item = -1;  /* T_revokedCertificates_item */
static int hf_x509af_revokedUserCertificate = -1;  /* CertificateSerialNumber */
static int hf_x509af_revocationDate = -1;         /* Time */
static int hf_x509af_crlEntryExtensions = -1;     /* Extensions */
static int hf_x509af_crlExtensions = -1;          /* Extensions */
static int hf_x509af_attributeCertificate = -1;   /* AttributeCertificate */
static int hf_x509af_acPath = -1;                 /* SEQUENCE_OF_ACPathData */
static int hf_x509af_acPath_item = -1;            /* ACPathData */
static int hf_x509af_certificate = -1;            /* Certificate */
static int hf_x509af_signedAttributeCertificateInfo = -1;  /* AttributeCertificateInfo */
static int hf_x509af_info_subject = -1;           /* InfoSubject */
static int hf_x509af_baseCertificateID = -1;      /* IssuerSerial */
static int hf_x509af_infoSubjectName = -1;        /* GeneralNames */
static int hf_x509af_issuerName = -1;             /* GeneralNames */
static int hf_x509af_attCertValidityPeriod = -1;  /* AttCertValidityPeriod */
static int hf_x509af_attributes = -1;             /* SEQUENCE_OF_Attribute */
static int hf_x509af_attributes_item = -1;        /* Attribute */
static int hf_x509af_issuerUniqueID = -1;         /* UniqueIdentifier */
static int hf_x509af_serial = -1;                 /* CertificateSerialNumber */
static int hf_x509af_issuerUID = -1;              /* UniqueIdentifier */
static int hf_x509af_notBeforeTime = -1;          /* GeneralizedTime */
static int hf_x509af_notAfterTime = -1;           /* GeneralizedTime */
static int hf_x509af_assertion_subject = -1;      /* AssertionSubject */
static int hf_x509af_assertionSubjectName = -1;   /* SubjectName */
static int hf_x509af_assertionIssuer = -1;        /* Name */
static int hf_x509af_attCertValidity = -1;        /* GeneralizedTime */
static int hf_x509af_attType = -1;                /* SET_OF_AttributeType */
static int hf_x509af_attType_item = -1;           /* AttributeType */
static int hf_x509af_p = -1;                      /* INTEGER */
static int hf_x509af_q = -1;                      /* INTEGER */
static int hf_x509af_g = -1;                      /* INTEGER */

/*--- End of included file: packet-x509af-hf.c ---*/
#line 55 "packet-x509af-template.c"

/* Initialize the subtree pointers */
static gint ett_pkix_crl = -1;

/*--- Included file: packet-x509af-ett.c ---*/
#line 1 "packet-x509af-ett.c"
static gint ett_x509af_Certificate = -1;
static gint ett_x509af_T_signedCertificate = -1;
static gint ett_x509af_SubjectName = -1;
static gint ett_x509af_AlgorithmIdentifier = -1;
static gint ett_x509af_Validity = -1;
static gint ett_x509af_SubjectPublicKeyInfo = -1;
static gint ett_x509af_Time = -1;
static gint ett_x509af_Extensions = -1;
static gint ett_x509af_Extension = -1;
static gint ett_x509af_Certificates = -1;
static gint ett_x509af_ForwardCertificationPath = -1;
static gint ett_x509af_CrossCertificates = -1;
static gint ett_x509af_CertificationPath = -1;
static gint ett_x509af_SEQUENCE_OF_CertificatePair = -1;
static gint ett_x509af_CertificatePair = -1;
static gint ett_x509af_CertificateList = -1;
static gint ett_x509af_T_signedCertificateList = -1;
static gint ett_x509af_T_revokedCertificates = -1;
static gint ett_x509af_T_revokedCertificates_item = -1;
static gint ett_x509af_AttributeCertificationPath = -1;
static gint ett_x509af_SEQUENCE_OF_ACPathData = -1;
static gint ett_x509af_ACPathData = -1;
static gint ett_x509af_AttributeCertificate = -1;
static gint ett_x509af_AttributeCertificateInfo = -1;
static gint ett_x509af_InfoSubject = -1;
static gint ett_x509af_SEQUENCE_OF_Attribute = -1;
static gint ett_x509af_IssuerSerial = -1;
static gint ett_x509af_AttCertValidityPeriod = -1;
static gint ett_x509af_AttributeCertificateAssertion = -1;
static gint ett_x509af_AssertionSubject = -1;
static gint ett_x509af_SET_OF_AttributeType = -1;
static gint ett_x509af_DSS_Params = -1;

/*--- End of included file: packet-x509af-ett.c ---*/
#line 59 "packet-x509af-template.c"

static const char *algorithm_id;
static const char *extension_id;


/*--- Included file: packet-x509af-fn.c ---*/
#line 1 "packet-x509af-fn.c"
/*--- Fields for imported types ---*/

static int dissect_issuer(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509if_Name(FALSE, tvb, offset, actx, tree, hf_x509af_issuer);
}
static int dissect_issuerUniqueIdentifier_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509sat_UniqueIdentifier(TRUE, tvb, offset, actx, tree, hf_x509af_issuerUniqueIdentifier);
}
static int dissect_subjectUniqueIdentifier_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509sat_UniqueIdentifier(TRUE, tvb, offset, actx, tree, hf_x509af_subjectUniqueIdentifier);
}
static int dissect_rdnSequence(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509if_RDNSequence(FALSE, tvb, offset, actx, tree, hf_x509af_rdnSequence);
}
static int dissect_infoSubjectName(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509ce_GeneralNames(FALSE, tvb, offset, actx, tree, hf_x509af_infoSubjectName);
}
static int dissect_issuerName(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509ce_GeneralNames(FALSE, tvb, offset, actx, tree, hf_x509af_issuerName);
}
static int dissect_attributes_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509if_Attribute(FALSE, tvb, offset, actx, tree, hf_x509af_attributes_item);
}
static int dissect_issuerUniqueID(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509sat_UniqueIdentifier(FALSE, tvb, offset, actx, tree, hf_x509af_issuerUniqueID);
}
static int dissect_issuerUID(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509sat_UniqueIdentifier(FALSE, tvb, offset, actx, tree, hf_x509af_issuerUID);
}
static int dissect_assertionIssuer(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509if_Name(FALSE, tvb, offset, actx, tree, hf_x509af_assertionIssuer);
}
static int dissect_attType_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509if_AttributeType(FALSE, tvb, offset, actx, tree, hf_x509af_attType_item);
}


const value_string x509af_Version_vals[] = {
  {   0, "v1" },
  {   1, "v2" },
  {   2, "v3" },
  { 0, NULL }
};


int
dissect_x509af_Version(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_version(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_Version(FALSE, tvb, offset, actx, tree, hf_x509af_version);
}



int
dissect_x509af_CertificateSerialNumber(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_serialNumber(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_CertificateSerialNumber(FALSE, tvb, offset, actx, tree, hf_x509af_serialNumber);
}
static int dissect_revokedUserCertificate(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_CertificateSerialNumber(FALSE, tvb, offset, actx, tree, hf_x509af_revokedUserCertificate);
}
static int dissect_serial(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_CertificateSerialNumber(FALSE, tvb, offset, actx, tree, hf_x509af_serial);
}



static int
dissect_x509af_T_algorithmId(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 69 "x509af.cnf"
  const char *name;

    offset = dissect_ber_object_identifier_str(implicit_tag, actx, tree, tvb, offset, hf_x509af_algorithm_id, &algorithm_id);


  if(algorithm_id) {
    name = get_oid_str_name(algorithm_id);

    proto_item_append_text(tree, " (%s)", name ? name : algorithm_id); 
  }



  return offset;
}
static int dissect_algorithmId(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_T_algorithmId(FALSE, tvb, offset, actx, tree, hf_x509af_algorithmId);
}



static int
dissect_x509af_T_parameters(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 80 "x509af.cnf"
  offset=call_ber_oid_callback(algorithm_id, tvb, offset, actx->pinfo, tree);



  return offset;
}
static int dissect_parameters(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_T_parameters(FALSE, tvb, offset, actx, tree, hf_x509af_parameters);
}


static const ber_old_sequence_t AlgorithmIdentifier_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_OID, BER_FLAGS_NOOWNTAG, dissect_algorithmId },
  { BER_CLASS_ANY, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_parameters },
  { 0, 0, 0, NULL }
};

int
dissect_x509af_AlgorithmIdentifier(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       AlgorithmIdentifier_sequence, hf_index, ett_x509af_AlgorithmIdentifier);

  return offset;
}
static int dissect_signature(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_AlgorithmIdentifier(FALSE, tvb, offset, actx, tree, hf_x509af_signature);
}
static int dissect_algorithmIdentifier(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_AlgorithmIdentifier(FALSE, tvb, offset, actx, tree, hf_x509af_algorithmIdentifier);
}
static int dissect_algorithm(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_AlgorithmIdentifier(FALSE, tvb, offset, actx, tree, hf_x509af_algorithm);
}



static int
dissect_x509af_UTCTime(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_restricted_string(implicit_tag, BER_UNI_TAG_UTCTime,
                                            actx, tree, tvb, offset, hf_index,
                                            NULL);

  return offset;
}
static int dissect_utcTime(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_UTCTime(FALSE, tvb, offset, actx, tree, hf_x509af_utcTime);
}



static int
dissect_x509af_GeneralizedTime(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_GeneralizedTime(implicit_tag, actx, tree, tvb, offset, hf_index);

  return offset;
}
static int dissect_generalizedTime(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_GeneralizedTime(FALSE, tvb, offset, actx, tree, hf_x509af_generalizedTime);
}
static int dissect_notBeforeTime(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_GeneralizedTime(FALSE, tvb, offset, actx, tree, hf_x509af_notBeforeTime);
}
static int dissect_notAfterTime(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_GeneralizedTime(FALSE, tvb, offset, actx, tree, hf_x509af_notAfterTime);
}
static int dissect_attCertValidity(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_GeneralizedTime(FALSE, tvb, offset, actx, tree, hf_x509af_attCertValidity);
}


const value_string x509af_Time_vals[] = {
  {   0, "utcTime" },
  {   1, "generalizedTime" },
  { 0, NULL }
};

static const ber_old_choice_t Time_choice[] = {
  {   0, BER_CLASS_UNI, BER_UNI_TAG_UTCTime, BER_FLAGS_NOOWNTAG, dissect_utcTime },
  {   1, BER_CLASS_UNI, BER_UNI_TAG_GeneralizedTime, BER_FLAGS_NOOWNTAG, dissect_generalizedTime },
  { 0, 0, 0, 0, NULL }
};

int
dissect_x509af_Time(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_choice(actx, tree, tvb, offset,
                                     Time_choice, hf_index, ett_x509af_Time,
                                     NULL);

  return offset;
}
static int dissect_notBefore(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_Time(FALSE, tvb, offset, actx, tree, hf_x509af_notBefore);
}
static int dissect_notAfter(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_Time(FALSE, tvb, offset, actx, tree, hf_x509af_notAfter);
}
static int dissect_thisUpdate(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_Time(FALSE, tvb, offset, actx, tree, hf_x509af_thisUpdate);
}
static int dissect_nextUpdate(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_Time(FALSE, tvb, offset, actx, tree, hf_x509af_nextUpdate);
}
static int dissect_revocationDate(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_Time(FALSE, tvb, offset, actx, tree, hf_x509af_revocationDate);
}


static const ber_old_sequence_t Validity_sequence[] = {
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_notBefore },
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_notAfter },
  { 0, 0, 0, NULL }
};

int
dissect_x509af_Validity(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       Validity_sequence, hf_index, ett_x509af_Validity);

  return offset;
}
static int dissect_validity(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_Validity(FALSE, tvb, offset, actx, tree, hf_x509af_validity);
}


static const value_string x509af_SubjectName_vals[] = {
  {   0, "rdnSequence" },
  { 0, NULL }
};

static const ber_old_choice_t SubjectName_choice[] = {
  {   0, BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_rdnSequence },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_x509af_SubjectName(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 107 "x509af.cnf"

  const char* str;
    offset = dissect_ber_old_choice(actx, tree, tvb, offset,
                                     SubjectName_choice, hf_index, ett_x509af_SubjectName,
                                     NULL);


  str = x509if_get_last_dn();
  proto_item_append_text(proto_item_get_parent(tree), " (%s)", str?str:"");



  return offset;
}
static int dissect_subject(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_SubjectName(FALSE, tvb, offset, actx, tree, hf_x509af_subject);
}
static int dissect_assertionSubjectName(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_SubjectName(FALSE, tvb, offset, actx, tree, hf_x509af_assertionSubjectName);
}



static int
dissect_x509af_BIT_STRING(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_bitstring(implicit_tag, actx, tree, tvb, offset,
                                    NULL, hf_index, -1,
                                    NULL);

  return offset;
}
static int dissect_encrypted(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_BIT_STRING(FALSE, tvb, offset, actx, tree, hf_x509af_encrypted);
}
static int dissect_subjectPublicKey(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_BIT_STRING(FALSE, tvb, offset, actx, tree, hf_x509af_subjectPublicKey);
}


static const ber_old_sequence_t SubjectPublicKeyInfo_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_algorithm },
  { BER_CLASS_UNI, BER_UNI_TAG_BITSTRING, BER_FLAGS_NOOWNTAG, dissect_subjectPublicKey },
  { 0, 0, 0, NULL }
};

int
dissect_x509af_SubjectPublicKeyInfo(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       SubjectPublicKeyInfo_sequence, hf_index, ett_x509af_SubjectPublicKeyInfo);

  return offset;
}
static int dissect_subjectPublicKeyInfo(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_SubjectPublicKeyInfo(FALSE, tvb, offset, actx, tree, hf_x509af_subjectPublicKeyInfo);
}



static int
dissect_x509af_T_extnId(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 86 "x509af.cnf"
  const char *name;

    offset = dissect_ber_object_identifier_str(implicit_tag, actx, tree, tvb, offset, hf_x509af_extension_id, &extension_id);


  if(extension_id) {
    name = get_oid_str_name(extension_id);

    proto_item_append_text(tree, " (%s)", name ? name : extension_id);
  }



  return offset;
}
static int dissect_extnId(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_T_extnId(FALSE, tvb, offset, actx, tree, hf_x509af_extnId);
}



static int
dissect_x509af_BOOLEAN(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_boolean(implicit_tag, actx, tree, tvb, offset, hf_index);

  return offset;
}
static int dissect_critical(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_BOOLEAN(FALSE, tvb, offset, actx, tree, hf_x509af_critical);
}



static int
dissect_x509af_T_extnValue(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 97 "x509af.cnf"
  gint8 class;
  gboolean pc, ind;
  gint32 tag;
  guint32 len;
  /* skip past the T and L  */
  offset = dissect_ber_identifier(actx->pinfo, tree, tvb, offset, &class, &pc, &tag);
  offset = dissect_ber_length(actx->pinfo, tree, tvb, offset, &len, &ind);
  offset=call_ber_oid_callback(extension_id, tvb, offset, actx->pinfo, tree);



  return offset;
}
static int dissect_extnValue(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_T_extnValue(FALSE, tvb, offset, actx, tree, hf_x509af_extnValue);
}


static const ber_old_sequence_t Extension_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_OID, BER_FLAGS_NOOWNTAG, dissect_extnId },
  { BER_CLASS_UNI, BER_UNI_TAG_BOOLEAN, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_critical },
  { BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_extnValue },
  { 0, 0, 0, NULL }
};

int
dissect_x509af_Extension(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       Extension_sequence, hf_index, ett_x509af_Extension);

  return offset;
}
static int dissect_Extensions_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_Extension(FALSE, tvb, offset, actx, tree, hf_x509af_Extensions_item);
}


static const ber_old_sequence_t Extensions_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_Extensions_item },
};

int
dissect_x509af_Extensions(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                          Extensions_sequence_of, hf_index, ett_x509af_Extensions);

  return offset;
}
static int dissect_extensions(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_Extensions(FALSE, tvb, offset, actx, tree, hf_x509af_extensions);
}
static int dissect_crlEntryExtensions(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_Extensions(FALSE, tvb, offset, actx, tree, hf_x509af_crlEntryExtensions);
}
static int dissect_crlExtensions(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_Extensions(FALSE, tvb, offset, actx, tree, hf_x509af_crlExtensions);
}


static const ber_old_sequence_t T_signedCertificate_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL, dissect_version },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_serialNumber },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_signature },
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG, dissect_issuer },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_validity },
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_subject },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_subjectPublicKeyInfo },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_issuerUniqueIdentifier_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_subjectUniqueIdentifier_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL, dissect_extensions },
  { 0, 0, 0, NULL }
};

static int
dissect_x509af_T_signedCertificate(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       T_signedCertificate_sequence, hf_index, ett_x509af_T_signedCertificate);

  return offset;
}
static int dissect_signedCertificate(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_T_signedCertificate(FALSE, tvb, offset, actx, tree, hf_x509af_signedCertificate);
}


static const ber_old_sequence_t Certificate_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_signedCertificate },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_algorithmIdentifier },
  { BER_CLASS_UNI, BER_UNI_TAG_BITSTRING, BER_FLAGS_NOOWNTAG, dissect_encrypted },
  { 0, 0, 0, NULL }
};

int
dissect_x509af_Certificate(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       Certificate_sequence, hf_index, ett_x509af_Certificate);

  return offset;
}
static int dissect_userCertificate(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_Certificate(FALSE, tvb, offset, actx, tree, hf_x509af_userCertificate);
}
static int dissect_CrossCertificates_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_Certificate(FALSE, tvb, offset, actx, tree, hf_x509af_CrossCertificates_item);
}
static int dissect_issuedByThisCA(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_Certificate(FALSE, tvb, offset, actx, tree, hf_x509af_issuedByThisCA);
}
static int dissect_issuedToThisCA(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_Certificate(FALSE, tvb, offset, actx, tree, hf_x509af_issuedToThisCA);
}
static int dissect_certificate(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_Certificate(FALSE, tvb, offset, actx, tree, hf_x509af_certificate);
}


static const ber_old_sequence_t CrossCertificates_set_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_CrossCertificates_item },
};

int
dissect_x509af_CrossCertificates(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_set_of(implicit_tag, actx, tree, tvb, offset,
                                     CrossCertificates_set_of, hf_index, ett_x509af_CrossCertificates);

  return offset;
}
static int dissect_ForwardCertificationPath_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_CrossCertificates(FALSE, tvb, offset, actx, tree, hf_x509af_ForwardCertificationPath_item);
}


static const ber_old_sequence_t ForwardCertificationPath_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SET, BER_FLAGS_NOOWNTAG, dissect_ForwardCertificationPath_item },
};

int
dissect_x509af_ForwardCertificationPath(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                          ForwardCertificationPath_sequence_of, hf_index, ett_x509af_ForwardCertificationPath);

  return offset;
}
static int dissect_certificationPath(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_ForwardCertificationPath(FALSE, tvb, offset, actx, tree, hf_x509af_certificationPath);
}


static const ber_old_sequence_t Certificates_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_userCertificate },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_certificationPath },
  { 0, 0, 0, NULL }
};

int
dissect_x509af_Certificates(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       Certificates_sequence, hf_index, ett_x509af_Certificates);

  return offset;
}


static const ber_old_sequence_t CertificatePair_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL, dissect_issuedByThisCA },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL, dissect_issuedToThisCA },
  { 0, 0, 0, NULL }
};

int
dissect_x509af_CertificatePair(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       CertificatePair_sequence, hf_index, ett_x509af_CertificatePair);

  return offset;
}
static int dissect_theCACertificates_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_CertificatePair(FALSE, tvb, offset, actx, tree, hf_x509af_theCACertificates_item);
}


static const ber_old_sequence_t SEQUENCE_OF_CertificatePair_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_theCACertificates_item },
};

static int
dissect_x509af_SEQUENCE_OF_CertificatePair(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                          SEQUENCE_OF_CertificatePair_sequence_of, hf_index, ett_x509af_SEQUENCE_OF_CertificatePair);

  return offset;
}
static int dissect_theCACertificates(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_SEQUENCE_OF_CertificatePair(FALSE, tvb, offset, actx, tree, hf_x509af_theCACertificates);
}


static const ber_old_sequence_t CertificationPath_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_userCertificate },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_theCACertificates },
  { 0, 0, 0, NULL }
};

int
dissect_x509af_CertificationPath(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       CertificationPath_sequence, hf_index, ett_x509af_CertificationPath);

  return offset;
}


static const ber_old_sequence_t T_revokedCertificates_item_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_revokedUserCertificate },
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_revocationDate },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_crlEntryExtensions },
  { 0, 0, 0, NULL }
};

static int
dissect_x509af_T_revokedCertificates_item(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       T_revokedCertificates_item_sequence, hf_index, ett_x509af_T_revokedCertificates_item);

  return offset;
}
static int dissect_revokedCertificates_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_T_revokedCertificates_item(FALSE, tvb, offset, actx, tree, hf_x509af_revokedCertificates_item);
}


static const ber_old_sequence_t T_revokedCertificates_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_revokedCertificates_item },
};

static int
dissect_x509af_T_revokedCertificates(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                          T_revokedCertificates_sequence_of, hf_index, ett_x509af_T_revokedCertificates);

  return offset;
}
static int dissect_revokedCertificates(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_T_revokedCertificates(FALSE, tvb, offset, actx, tree, hf_x509af_revokedCertificates);
}


static const ber_old_sequence_t T_signedCertificateList_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_version },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_signature },
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG, dissect_issuer },
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_thisUpdate },
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_nextUpdate },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_revokedCertificates },
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL, dissect_crlExtensions },
  { 0, 0, 0, NULL }
};

static int
dissect_x509af_T_signedCertificateList(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       T_signedCertificateList_sequence, hf_index, ett_x509af_T_signedCertificateList);

  return offset;
}
static int dissect_signedCertificateList(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_T_signedCertificateList(FALSE, tvb, offset, actx, tree, hf_x509af_signedCertificateList);
}


static const ber_old_sequence_t CertificateList_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_signedCertificateList },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_algorithmIdentifier },
  { BER_CLASS_UNI, BER_UNI_TAG_BITSTRING, BER_FLAGS_NOOWNTAG, dissect_encrypted },
  { 0, 0, 0, NULL }
};

int
dissect_x509af_CertificateList(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       CertificateList_sequence, hf_index, ett_x509af_CertificateList);

  return offset;
}


static const ber_old_sequence_t IssuerSerial_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_issuerName },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_serial },
  { BER_CLASS_UNI, BER_UNI_TAG_BITSTRING, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_issuerUID },
  { 0, 0, 0, NULL }
};

int
dissect_x509af_IssuerSerial(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       IssuerSerial_sequence, hf_index, ett_x509af_IssuerSerial);

  return offset;
}
static int dissect_baseCertificateID(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_IssuerSerial(FALSE, tvb, offset, actx, tree, hf_x509af_baseCertificateID);
}


static const value_string x509af_InfoSubject_vals[] = {
  {   0, "baseCertificateID" },
  {   1, "subjectName" },
  { 0, NULL }
};

static const ber_old_choice_t InfoSubject_choice[] = {
  {   0, BER_CLASS_CON, 0, 0, dissect_baseCertificateID },
  {   1, BER_CLASS_CON, 1, 0, dissect_infoSubjectName },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_x509af_InfoSubject(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_choice(actx, tree, tvb, offset,
                                     InfoSubject_choice, hf_index, ett_x509af_InfoSubject,
                                     NULL);

  return offset;
}
static int dissect_info_subject(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_InfoSubject(FALSE, tvb, offset, actx, tree, hf_x509af_info_subject);
}


static const ber_old_sequence_t AttCertValidityPeriod_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_GeneralizedTime, BER_FLAGS_NOOWNTAG, dissect_notBeforeTime },
  { BER_CLASS_UNI, BER_UNI_TAG_GeneralizedTime, BER_FLAGS_NOOWNTAG, dissect_notAfterTime },
  { 0, 0, 0, NULL }
};

int
dissect_x509af_AttCertValidityPeriod(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       AttCertValidityPeriod_sequence, hf_index, ett_x509af_AttCertValidityPeriod);

  return offset;
}
static int dissect_attCertValidityPeriod(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_AttCertValidityPeriod(FALSE, tvb, offset, actx, tree, hf_x509af_attCertValidityPeriod);
}


static const ber_old_sequence_t SEQUENCE_OF_Attribute_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_attributes_item },
};

static int
dissect_x509af_SEQUENCE_OF_Attribute(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                          SEQUENCE_OF_Attribute_sequence_of, hf_index, ett_x509af_SEQUENCE_OF_Attribute);

  return offset;
}
static int dissect_attributes(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_SEQUENCE_OF_Attribute(FALSE, tvb, offset, actx, tree, hf_x509af_attributes);
}


static const ber_old_sequence_t AttributeCertificateInfo_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_version },
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_info_subject },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_issuerName },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_signature },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_serialNumber },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_attCertValidityPeriod },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_attributes },
  { BER_CLASS_UNI, BER_UNI_TAG_BITSTRING, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_issuerUniqueID },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_extensions },
  { 0, 0, 0, NULL }
};

int
dissect_x509af_AttributeCertificateInfo(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       AttributeCertificateInfo_sequence, hf_index, ett_x509af_AttributeCertificateInfo);

  return offset;
}
static int dissect_signedAttributeCertificateInfo(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_AttributeCertificateInfo(FALSE, tvb, offset, actx, tree, hf_x509af_signedAttributeCertificateInfo);
}


static const ber_old_sequence_t AttributeCertificate_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_signedAttributeCertificateInfo },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_algorithmIdentifier },
  { BER_CLASS_UNI, BER_UNI_TAG_BITSTRING, BER_FLAGS_NOOWNTAG, dissect_encrypted },
  { 0, 0, 0, NULL }
};

int
dissect_x509af_AttributeCertificate(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       AttributeCertificate_sequence, hf_index, ett_x509af_AttributeCertificate);

  return offset;
}
static int dissect_attributeCertificate(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_AttributeCertificate(FALSE, tvb, offset, actx, tree, hf_x509af_attributeCertificate);
}


static const ber_old_sequence_t ACPathData_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL, dissect_certificate },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL, dissect_attributeCertificate },
  { 0, 0, 0, NULL }
};

int
dissect_x509af_ACPathData(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       ACPathData_sequence, hf_index, ett_x509af_ACPathData);

  return offset;
}
static int dissect_acPath_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_ACPathData(FALSE, tvb, offset, actx, tree, hf_x509af_acPath_item);
}


static const ber_old_sequence_t SEQUENCE_OF_ACPathData_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_acPath_item },
};

static int
dissect_x509af_SEQUENCE_OF_ACPathData(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                          SEQUENCE_OF_ACPathData_sequence_of, hf_index, ett_x509af_SEQUENCE_OF_ACPathData);

  return offset;
}
static int dissect_acPath(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_SEQUENCE_OF_ACPathData(FALSE, tvb, offset, actx, tree, hf_x509af_acPath);
}


static const ber_old_sequence_t AttributeCertificationPath_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_attributeCertificate },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_acPath },
  { 0, 0, 0, NULL }
};

int
dissect_x509af_AttributeCertificationPath(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       AttributeCertificationPath_sequence, hf_index, ett_x509af_AttributeCertificationPath);

  return offset;
}


static const value_string x509af_AssertionSubject_vals[] = {
  {   0, "baseCertificateID" },
  {   1, "subjectName" },
  { 0, NULL }
};

static const ber_old_choice_t AssertionSubject_choice[] = {
  {   0, BER_CLASS_CON, 0, 0, dissect_baseCertificateID },
  {   1, BER_CLASS_CON, 1, 0, dissect_assertionSubjectName },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_x509af_AssertionSubject(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_choice(actx, tree, tvb, offset,
                                     AssertionSubject_choice, hf_index, ett_x509af_AssertionSubject,
                                     NULL);

  return offset;
}
static int dissect_assertion_subject(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_AssertionSubject(FALSE, tvb, offset, actx, tree, hf_x509af_assertion_subject);
}


static const ber_old_sequence_t SET_OF_AttributeType_set_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_OID, BER_FLAGS_NOOWNTAG, dissect_attType_item },
};

static int
dissect_x509af_SET_OF_AttributeType(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_set_of(implicit_tag, actx, tree, tvb, offset,
                                     SET_OF_AttributeType_set_of, hf_index, ett_x509af_SET_OF_AttributeType);

  return offset;
}
static int dissect_attType(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_SET_OF_AttributeType(FALSE, tvb, offset, actx, tree, hf_x509af_attType);
}


static const ber_old_sequence_t AttributeCertificateAssertion_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL, dissect_assertion_subject },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL, dissect_assertionIssuer },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL, dissect_attCertValidity },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL, dissect_attType },
  { 0, 0, 0, NULL }
};

int
dissect_x509af_AttributeCertificateAssertion(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       AttributeCertificateAssertion_sequence, hf_index, ett_x509af_AttributeCertificateAssertion);

  return offset;
}



static int
dissect_x509af_INTEGER(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_p(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_INTEGER(FALSE, tvb, offset, actx, tree, hf_x509af_p);
}
static int dissect_q(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_INTEGER(FALSE, tvb, offset, actx, tree, hf_x509af_q);
}
static int dissect_g(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_INTEGER(FALSE, tvb, offset, actx, tree, hf_x509af_g);
}


static const ber_old_sequence_t DSS_Params_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_p },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_q },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_g },
  { 0, 0, 0, NULL }
};

static int
dissect_x509af_DSS_Params(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       DSS_Params_sequence, hf_index, ett_x509af_DSS_Params);

  return offset;
}

/*--- PDUs ---*/

static void dissect_Certificate_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x509af_Certificate(FALSE, tvb, 0, &asn1_ctx, tree, hf_x509af_Certificate_PDU);
}
static void dissect_CertificatePair_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x509af_CertificatePair(FALSE, tvb, 0, &asn1_ctx, tree, hf_x509af_CertificatePair_PDU);
}
static void dissect_CertificateList_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x509af_CertificateList(FALSE, tvb, 0, &asn1_ctx, tree, hf_x509af_CertificateList_PDU);
}
static void dissect_AttributeCertificate_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x509af_AttributeCertificate(FALSE, tvb, 0, &asn1_ctx, tree, hf_x509af_AttributeCertificate_PDU);
}
static void dissect_DSS_Params_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x509af_DSS_Params(FALSE, tvb, 0, &asn1_ctx, tree, hf_x509af_DSS_Params_PDU);
}


/*--- End of included file: packet-x509af-fn.c ---*/
#line 64 "packet-x509af-template.c"

const char *x509af_get_last_algorithm_id(void) {
  return algorithm_id;
}


static int
dissect_pkix_crl(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree)
{
	proto_item *item=NULL;
	proto_tree *tree=NULL;
	asn1_ctx_t asn1_ctx;
	asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);

	if (check_col(pinfo->cinfo, COL_PROTOCOL)) 
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "PKIX-CRL");

	if (check_col(pinfo->cinfo, COL_INFO)) {
		col_clear(pinfo->cinfo, COL_INFO);
		
		col_add_fstr(pinfo->cinfo, COL_INFO, "Certificate Revocation List");
	}


	if(parent_tree){
		item=proto_tree_add_text(parent_tree, tvb, 0, -1, "Certificate Revocation List");
		tree = proto_item_add_subtree(item, ett_pkix_crl);
	}

	return dissect_x509af_CertificateList(FALSE, tvb, 0, &asn1_ctx, tree, -1);
}

/*--- proto_register_x509af ----------------------------------------------*/
void proto_register_x509af(void) {

  /* List of fields */
  static hf_register_info hf[] = {
    { &hf_x509af_algorithm_id,
      { "Algorithm Id", "x509af.algorithm.id",
        FT_OID, BASE_NONE, NULL, 0,
        "Algorithm Id", HFILL }},
    { &hf_x509af_extension_id,
      { "Extension Id", "x509af.extension.id",
        FT_OID, BASE_NONE, NULL, 0,
        "Extension Id", HFILL }},

/*--- Included file: packet-x509af-hfarr.c ---*/
#line 1 "packet-x509af-hfarr.c"
    { &hf_x509af_Certificate_PDU,
      { "Certificate", "x509af.Certificate",
        FT_NONE, BASE_NONE, NULL, 0,
        "x509af.Certificate", HFILL }},
    { &hf_x509af_CertificatePair_PDU,
      { "CertificatePair", "x509af.CertificatePair",
        FT_NONE, BASE_NONE, NULL, 0,
        "x509af.CertificatePair", HFILL }},
    { &hf_x509af_CertificateList_PDU,
      { "CertificateList", "x509af.CertificateList",
        FT_NONE, BASE_NONE, NULL, 0,
        "x509af.CertificateList", HFILL }},
    { &hf_x509af_AttributeCertificate_PDU,
      { "AttributeCertificate", "x509af.AttributeCertificate",
        FT_NONE, BASE_NONE, NULL, 0,
        "x509af.AttributeCertificate", HFILL }},
    { &hf_x509af_DSS_Params_PDU,
      { "DSS-Params", "x509af.DSS_Params",
        FT_NONE, BASE_NONE, NULL, 0,
        "x509af.DSS_Params", HFILL }},
    { &hf_x509af_signedCertificate,
      { "signedCertificate", "x509af.signedCertificate",
        FT_NONE, BASE_NONE, NULL, 0,
        "x509af.T_signedCertificate", HFILL }},
    { &hf_x509af_version,
      { "version", "x509af.version",
        FT_INT32, BASE_DEC, VALS(x509af_Version_vals), 0,
        "x509af.Version", HFILL }},
    { &hf_x509af_serialNumber,
      { "serialNumber", "x509af.serialNumber",
        FT_INT32, BASE_DEC, NULL, 0,
        "x509af.CertificateSerialNumber", HFILL }},
    { &hf_x509af_signature,
      { "signature", "x509af.signature",
        FT_NONE, BASE_NONE, NULL, 0,
        "x509af.AlgorithmIdentifier", HFILL }},
    { &hf_x509af_issuer,
      { "issuer", "x509af.issuer",
        FT_UINT32, BASE_DEC, VALS(x509if_Name_vals), 0,
        "x509if.Name", HFILL }},
    { &hf_x509af_validity,
      { "validity", "x509af.validity",
        FT_NONE, BASE_NONE, NULL, 0,
        "x509af.Validity", HFILL }},
    { &hf_x509af_subject,
      { "subject", "x509af.subject",
        FT_UINT32, BASE_DEC, VALS(x509af_SubjectName_vals), 0,
        "x509af.SubjectName", HFILL }},
    { &hf_x509af_subjectPublicKeyInfo,
      { "subjectPublicKeyInfo", "x509af.subjectPublicKeyInfo",
        FT_NONE, BASE_NONE, NULL, 0,
        "x509af.SubjectPublicKeyInfo", HFILL }},
    { &hf_x509af_issuerUniqueIdentifier,
      { "issuerUniqueIdentifier", "x509af.issuerUniqueIdentifier",
        FT_BYTES, BASE_HEX, NULL, 0,
        "x509sat.UniqueIdentifier", HFILL }},
    { &hf_x509af_subjectUniqueIdentifier,
      { "subjectUniqueIdentifier", "x509af.subjectUniqueIdentifier",
        FT_BYTES, BASE_HEX, NULL, 0,
        "x509sat.UniqueIdentifier", HFILL }},
    { &hf_x509af_extensions,
      { "extensions", "x509af.extensions",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x509af.Extensions", HFILL }},
    { &hf_x509af_algorithmIdentifier,
      { "algorithmIdentifier", "x509af.algorithmIdentifier",
        FT_NONE, BASE_NONE, NULL, 0,
        "x509af.AlgorithmIdentifier", HFILL }},
    { &hf_x509af_encrypted,
      { "encrypted", "x509af.encrypted",
        FT_BYTES, BASE_HEX, NULL, 0,
        "x509af.BIT_STRING", HFILL }},
    { &hf_x509af_rdnSequence,
      { "rdnSequence", "x509af.rdnSequence",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x509if.RDNSequence", HFILL }},
    { &hf_x509af_algorithmId,
      { "algorithmId", "x509af.algorithmId",
        FT_OID, BASE_NONE, NULL, 0,
        "x509af.T_algorithmId", HFILL }},
    { &hf_x509af_parameters,
      { "parameters", "x509af.parameters",
        FT_NONE, BASE_NONE, NULL, 0,
        "x509af.T_parameters", HFILL }},
    { &hf_x509af_notBefore,
      { "notBefore", "x509af.notBefore",
        FT_UINT32, BASE_DEC, VALS(x509af_Time_vals), 0,
        "x509af.Time", HFILL }},
    { &hf_x509af_notAfter,
      { "notAfter", "x509af.notAfter",
        FT_UINT32, BASE_DEC, VALS(x509af_Time_vals), 0,
        "x509af.Time", HFILL }},
    { &hf_x509af_algorithm,
      { "algorithm", "x509af.algorithm",
        FT_NONE, BASE_NONE, NULL, 0,
        "x509af.AlgorithmIdentifier", HFILL }},
    { &hf_x509af_subjectPublicKey,
      { "subjectPublicKey", "x509af.subjectPublicKey",
        FT_BYTES, BASE_HEX, NULL, 0,
        "x509af.BIT_STRING", HFILL }},
    { &hf_x509af_utcTime,
      { "utcTime", "x509af.utcTime",
        FT_STRING, BASE_NONE, NULL, 0,
        "x509af.UTCTime", HFILL }},
    { &hf_x509af_generalizedTime,
      { "generalizedTime", "x509af.generalizedTime",
        FT_STRING, BASE_NONE, NULL, 0,
        "x509af.GeneralizedTime", HFILL }},
    { &hf_x509af_Extensions_item,
      { "Item", "x509af.Extensions_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "x509af.Extension", HFILL }},
    { &hf_x509af_extnId,
      { "extnId", "x509af.extnId",
        FT_OID, BASE_NONE, NULL, 0,
        "x509af.T_extnId", HFILL }},
    { &hf_x509af_critical,
      { "critical", "x509af.critical",
        FT_BOOLEAN, 8, NULL, 0,
        "x509af.BOOLEAN", HFILL }},
    { &hf_x509af_extnValue,
      { "extnValue", "x509af.extnValue",
        FT_BYTES, BASE_HEX, NULL, 0,
        "x509af.T_extnValue", HFILL }},
    { &hf_x509af_userCertificate,
      { "userCertificate", "x509af.userCertificate",
        FT_NONE, BASE_NONE, NULL, 0,
        "x509af.Certificate", HFILL }},
    { &hf_x509af_certificationPath,
      { "certificationPath", "x509af.certificationPath",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x509af.ForwardCertificationPath", HFILL }},
    { &hf_x509af_ForwardCertificationPath_item,
      { "Item", "x509af.ForwardCertificationPath_item",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x509af.CrossCertificates", HFILL }},
    { &hf_x509af_CrossCertificates_item,
      { "Item", "x509af.CrossCertificates_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "x509af.Certificate", HFILL }},
    { &hf_x509af_theCACertificates,
      { "theCACertificates", "x509af.theCACertificates",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x509af.SEQUENCE_OF_CertificatePair", HFILL }},
    { &hf_x509af_theCACertificates_item,
      { "Item", "x509af.theCACertificates_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "x509af.CertificatePair", HFILL }},
    { &hf_x509af_issuedByThisCA,
      { "issuedByThisCA", "x509af.issuedByThisCA",
        FT_NONE, BASE_NONE, NULL, 0,
        "x509af.Certificate", HFILL }},
    { &hf_x509af_issuedToThisCA,
      { "issuedToThisCA", "x509af.issuedToThisCA",
        FT_NONE, BASE_NONE, NULL, 0,
        "x509af.Certificate", HFILL }},
    { &hf_x509af_signedCertificateList,
      { "signedCertificateList", "x509af.signedCertificateList",
        FT_NONE, BASE_NONE, NULL, 0,
        "x509af.T_signedCertificateList", HFILL }},
    { &hf_x509af_thisUpdate,
      { "thisUpdate", "x509af.thisUpdate",
        FT_UINT32, BASE_DEC, VALS(x509af_Time_vals), 0,
        "x509af.Time", HFILL }},
    { &hf_x509af_nextUpdate,
      { "nextUpdate", "x509af.nextUpdate",
        FT_UINT32, BASE_DEC, VALS(x509af_Time_vals), 0,
        "x509af.Time", HFILL }},
    { &hf_x509af_revokedCertificates,
      { "revokedCertificates", "x509af.revokedCertificates",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x509af.T_revokedCertificates", HFILL }},
    { &hf_x509af_revokedCertificates_item,
      { "Item", "x509af.revokedCertificates_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "x509af.T_revokedCertificates_item", HFILL }},
    { &hf_x509af_revokedUserCertificate,
      { "userCertificate", "x509af.userCertificate",
        FT_INT32, BASE_DEC, NULL, 0,
        "x509af.CertificateSerialNumber", HFILL }},
    { &hf_x509af_revocationDate,
      { "revocationDate", "x509af.revocationDate",
        FT_UINT32, BASE_DEC, VALS(x509af_Time_vals), 0,
        "x509af.Time", HFILL }},
    { &hf_x509af_crlEntryExtensions,
      { "crlEntryExtensions", "x509af.crlEntryExtensions",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x509af.Extensions", HFILL }},
    { &hf_x509af_crlExtensions,
      { "crlExtensions", "x509af.crlExtensions",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x509af.Extensions", HFILL }},
    { &hf_x509af_attributeCertificate,
      { "attributeCertificate", "x509af.attributeCertificate",
        FT_NONE, BASE_NONE, NULL, 0,
        "x509af.AttributeCertificate", HFILL }},
    { &hf_x509af_acPath,
      { "acPath", "x509af.acPath",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x509af.SEQUENCE_OF_ACPathData", HFILL }},
    { &hf_x509af_acPath_item,
      { "Item", "x509af.acPath_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "x509af.ACPathData", HFILL }},
    { &hf_x509af_certificate,
      { "certificate", "x509af.certificate",
        FT_NONE, BASE_NONE, NULL, 0,
        "x509af.Certificate", HFILL }},
    { &hf_x509af_signedAttributeCertificateInfo,
      { "signedAttributeCertificateInfo", "x509af.signedAttributeCertificateInfo",
        FT_NONE, BASE_NONE, NULL, 0,
        "x509af.AttributeCertificateInfo", HFILL }},
    { &hf_x509af_info_subject,
      { "subject", "x509af.subject",
        FT_UINT32, BASE_DEC, VALS(x509af_InfoSubject_vals), 0,
        "x509af.InfoSubject", HFILL }},
    { &hf_x509af_baseCertificateID,
      { "baseCertificateID", "x509af.baseCertificateID",
        FT_NONE, BASE_NONE, NULL, 0,
        "x509af.IssuerSerial", HFILL }},
    { &hf_x509af_infoSubjectName,
      { "subjectName", "x509af.subjectName",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x509ce.GeneralNames", HFILL }},
    { &hf_x509af_issuerName,
      { "issuer", "x509af.issuer",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x509ce.GeneralNames", HFILL }},
    { &hf_x509af_attCertValidityPeriod,
      { "attCertValidityPeriod", "x509af.attCertValidityPeriod",
        FT_NONE, BASE_NONE, NULL, 0,
        "x509af.AttCertValidityPeriod", HFILL }},
    { &hf_x509af_attributes,
      { "attributes", "x509af.attributes",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x509af.SEQUENCE_OF_Attribute", HFILL }},
    { &hf_x509af_attributes_item,
      { "Item", "x509af.attributes_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "x509if.Attribute", HFILL }},
    { &hf_x509af_issuerUniqueID,
      { "issuerUniqueID", "x509af.issuerUniqueID",
        FT_BYTES, BASE_HEX, NULL, 0,
        "x509sat.UniqueIdentifier", HFILL }},
    { &hf_x509af_serial,
      { "serial", "x509af.serial",
        FT_INT32, BASE_DEC, NULL, 0,
        "x509af.CertificateSerialNumber", HFILL }},
    { &hf_x509af_issuerUID,
      { "issuerUID", "x509af.issuerUID",
        FT_BYTES, BASE_HEX, NULL, 0,
        "x509sat.UniqueIdentifier", HFILL }},
    { &hf_x509af_notBeforeTime,
      { "notBeforeTime", "x509af.notBeforeTime",
        FT_STRING, BASE_NONE, NULL, 0,
        "x509af.GeneralizedTime", HFILL }},
    { &hf_x509af_notAfterTime,
      { "notAfterTime", "x509af.notAfterTime",
        FT_STRING, BASE_NONE, NULL, 0,
        "x509af.GeneralizedTime", HFILL }},
    { &hf_x509af_assertion_subject,
      { "subject", "x509af.subject",
        FT_UINT32, BASE_DEC, VALS(x509af_AssertionSubject_vals), 0,
        "x509af.AssertionSubject", HFILL }},
    { &hf_x509af_assertionSubjectName,
      { "subjectName", "x509af.subjectName",
        FT_UINT32, BASE_DEC, VALS(x509af_SubjectName_vals), 0,
        "x509af.SubjectName", HFILL }},
    { &hf_x509af_assertionIssuer,
      { "issuer", "x509af.issuer",
        FT_UINT32, BASE_DEC, VALS(x509if_Name_vals), 0,
        "x509if.Name", HFILL }},
    { &hf_x509af_attCertValidity,
      { "attCertValidity", "x509af.attCertValidity",
        FT_STRING, BASE_NONE, NULL, 0,
        "x509af.GeneralizedTime", HFILL }},
    { &hf_x509af_attType,
      { "attType", "x509af.attType",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x509af.SET_OF_AttributeType", HFILL }},
    { &hf_x509af_attType_item,
      { "Item", "x509af.attType_item",
        FT_OID, BASE_NONE, NULL, 0,
        "x509if.AttributeType", HFILL }},
    { &hf_x509af_p,
      { "p", "x509af.p",
        FT_INT32, BASE_DEC, NULL, 0,
        "x509af.INTEGER", HFILL }},
    { &hf_x509af_q,
      { "q", "x509af.q",
        FT_INT32, BASE_DEC, NULL, 0,
        "x509af.INTEGER", HFILL }},
    { &hf_x509af_g,
      { "g", "x509af.g",
        FT_INT32, BASE_DEC, NULL, 0,
        "x509af.INTEGER", HFILL }},

/*--- End of included file: packet-x509af-hfarr.c ---*/
#line 110 "packet-x509af-template.c"
  };

  /* List of subtrees */
  static gint *ett[] = {
    &ett_pkix_crl,

/*--- Included file: packet-x509af-ettarr.c ---*/
#line 1 "packet-x509af-ettarr.c"
    &ett_x509af_Certificate,
    &ett_x509af_T_signedCertificate,
    &ett_x509af_SubjectName,
    &ett_x509af_AlgorithmIdentifier,
    &ett_x509af_Validity,
    &ett_x509af_SubjectPublicKeyInfo,
    &ett_x509af_Time,
    &ett_x509af_Extensions,
    &ett_x509af_Extension,
    &ett_x509af_Certificates,
    &ett_x509af_ForwardCertificationPath,
    &ett_x509af_CrossCertificates,
    &ett_x509af_CertificationPath,
    &ett_x509af_SEQUENCE_OF_CertificatePair,
    &ett_x509af_CertificatePair,
    &ett_x509af_CertificateList,
    &ett_x509af_T_signedCertificateList,
    &ett_x509af_T_revokedCertificates,
    &ett_x509af_T_revokedCertificates_item,
    &ett_x509af_AttributeCertificationPath,
    &ett_x509af_SEQUENCE_OF_ACPathData,
    &ett_x509af_ACPathData,
    &ett_x509af_AttributeCertificate,
    &ett_x509af_AttributeCertificateInfo,
    &ett_x509af_InfoSubject,
    &ett_x509af_SEQUENCE_OF_Attribute,
    &ett_x509af_IssuerSerial,
    &ett_x509af_AttCertValidityPeriod,
    &ett_x509af_AttributeCertificateAssertion,
    &ett_x509af_AssertionSubject,
    &ett_x509af_SET_OF_AttributeType,
    &ett_x509af_DSS_Params,

/*--- End of included file: packet-x509af-ettarr.c ---*/
#line 116 "packet-x509af-template.c"
  };

  /* Register protocol */
  proto_x509af = proto_register_protocol(PNAME, PSNAME, PFNAME);

  /* Register fields and subtrees */
  proto_register_field_array(proto_x509af, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));

}


/*--- proto_reg_handoff_x509af -------------------------------------------*/
void proto_reg_handoff_x509af(void) {
	dissector_handle_t pkix_crl_handle;

	pkix_crl_handle = new_create_dissector_handle(dissect_pkix_crl, proto_x509af);
	dissector_add_string("media_type", "application/pkix-crl", pkix_crl_handle);


/*--- Included file: packet-x509af-dis-tab.c ---*/
#line 1 "packet-x509af-dis-tab.c"
  register_ber_oid_dissector("2.5.4.36", dissect_Certificate_PDU, proto_x509af, "id-at-userCertificate");
  register_ber_oid_dissector("2.5.4.37", dissect_Certificate_PDU, proto_x509af, "id-at-cAcertificate");
  register_ber_oid_dissector("2.5.4.38", dissect_CertificateList_PDU, proto_x509af, "id-at-authorityRevocationList");
  register_ber_oid_dissector("2.5.4.39", dissect_CertificateList_PDU, proto_x509af, "id-at-certificateRevocationList");
  register_ber_oid_dissector("2.5.4.40", dissect_CertificatePair_PDU, proto_x509af, "id-at-crossCertificatePair");
  register_ber_oid_dissector("2.5.4.58", dissect_AttributeCertificate_PDU, proto_x509af, "id-at-attributeCertificate");
  register_ber_oid_dissector("2.5.4.59", dissect_CertificateList_PDU, proto_x509af, "id-at-attributeCertificateRevocationList");
  register_ber_oid_dissector("1.2.840.10040.4.1", dissect_DSS_Params_PDU, proto_x509af, "id-dsa");


/*--- End of included file: packet-x509af-dis-tab.c ---*/
#line 136 "packet-x509af-template.c"

	/*XXX these should really go to a better place but since that
	  I have not that ITU standard, ill put it here for the time
	  being.
	  Only implemented those algorithms that take no parameters 
	  for the time being,   ronnie
	*/
	/* from http://www.alvestrand.no/objectid/1.3.14.3.2.html */
	register_ber_oid_dissector("1.3.14.3.2.2", dissect_ber_oid_NULL_callback, proto_x509af, "md4WithRSA");
	register_ber_oid_dissector("1.3.14.3.2.3", dissect_ber_oid_NULL_callback, proto_x509af, "md5WithRSA");
	register_ber_oid_dissector("1.3.14.3.2.4", dissect_ber_oid_NULL_callback, proto_x509af, "md4WithRSAEncryption");
	register_ber_oid_dissector("1.3.14.3.2.6", dissect_ber_oid_NULL_callback, proto_x509af, "desECB");
	register_ber_oid_dissector("1.3.14.3.2.11", dissect_ber_oid_NULL_callback, proto_x509af, "rsaSignature");
	register_ber_oid_dissector("1.3.14.3.2.14", dissect_ber_oid_NULL_callback, proto_x509af, "mdc2WithRSASignature");
	register_ber_oid_dissector("1.3.14.3.2.15", dissect_ber_oid_NULL_callback, proto_x509af, "shaWithRSASignature");
	register_ber_oid_dissector("1.3.14.3.2.16", dissect_ber_oid_NULL_callback, proto_x509af, "dhWithCommonModulus");
	register_ber_oid_dissector("1.3.14.3.2.17", dissect_ber_oid_NULL_callback, proto_x509af, "desEDE");
	register_ber_oid_dissector("1.3.14.3.2.18", dissect_ber_oid_NULL_callback, proto_x509af, "sha");
	register_ber_oid_dissector("1.3.14.3.2.19", dissect_ber_oid_NULL_callback, proto_x509af, "mdc-2");
	register_ber_oid_dissector("1.3.14.3.2.20", dissect_ber_oid_NULL_callback, proto_x509af, "dsaCommon");
	register_ber_oid_dissector("1.3.14.3.2.21", dissect_ber_oid_NULL_callback, proto_x509af, "dsaCommonWithSHA");
	register_ber_oid_dissector("1.3.14.3.2.22", dissect_ber_oid_NULL_callback, proto_x509af, "rsaKeyTransport");
	register_ber_oid_dissector("1.3.14.3.2.23", dissect_ber_oid_NULL_callback, proto_x509af, "keyed-hash-seal");
	register_ber_oid_dissector("1.3.14.3.2.24", dissect_ber_oid_NULL_callback, proto_x509af, "md2WithRSASignature");
	register_ber_oid_dissector("1.3.14.3.2.25", dissect_ber_oid_NULL_callback, proto_x509af, "md5WithRSASignature");
	register_ber_oid_dissector("1.3.14.3.2.26", dissect_ber_oid_NULL_callback, proto_x509af, "SHA-1");

	/* these will generally be encoded as ";binary" in LDAP */

	register_ldap_name_dissector("cACertificate", dissect_Certificate_PDU, proto_x509af);
	register_ldap_name_dissector("certificate", dissect_Certificate_PDU, proto_x509af);
	
	register_ldap_name_dissector("certificateRevocationList", dissect_CertificateList_PDU, proto_x509af);
	register_ldap_name_dissector("crl", dissect_CertificateList_PDU, proto_x509af);

	register_ldap_name_dissector("authorityRevocationList", dissect_CertificateList_PDU, proto_x509af);
	register_ldap_name_dissector("arl", dissect_CertificateList_PDU, proto_x509af);

	register_ldap_name_dissector("crossCertificatePair", dissect_CertificatePair_PDU, proto_x509af);

	register_ber_syntax_dissector("Certificate", proto_x509af, dissect_Certificate_PDU); 
	register_ber_oid_syntax(".cer", NULL, "Certificate");
	register_ber_oid_syntax(".crt", NULL, "Certificate");
	register_ber_syntax_dissector("CertificateList", proto_x509af, dissect_CertificateList_PDU); 
	register_ber_oid_syntax(".crl", NULL, "CertificateList");
	register_ber_syntax_dissector("CrossCertificatePair", proto_x509af, dissect_CertificatePair_PDU); 

}

