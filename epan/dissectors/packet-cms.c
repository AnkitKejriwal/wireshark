/* Do not modify this file.                                                   */
/* It is created automatically by the ASN.1 to Ethereal dissector compiler    */
/* packet-cms.c                                                               */
/* ../../tools/asn2eth.py -X -b -p cms -c cms.cnf -s packet-cms-template CryptographicMessageSyntax.asn */

/* Input file: packet-cms-template.c */
/* Include files: packet-cms-hf.c, packet-cms-ett.c, packet-cms-fn.c, packet-cms-hfarr.c, packet-cms-ettarr.c, packet-cms-val.h */

/* packet-cms.c
 * Routines for RFC2630 Cryptographic Message Syntax packet dissection
 *
 * $Id: packet-cms-template.c,v 1.2 2004/05/25 21:07:43 guy Exp $
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
#include <epan/conversation.h>

#include <stdio.h>
#include <string.h>

#include "packet-ber.h"
#include "packet-cms.h"
#include "packet-x509af.h"
#include "packet-x509if.h"

#define PNAME  "Cryptographic Message Syntax"
#define PSNAME "CMS"
#define PFNAME "cms"

/* Initialize the protocol and registered fields */
int proto_cms = -1;
static int hf_cms_keyAttr_id = -1;

/*--- Included file: packet-cms-hf.c ---*/

/* Do not modify this file.                                                   */
/* It is created automatically by the ASN.1 to Ethereal dissector compiler    */
/* packet-cms-hf.c                                                            */
/* ../../tools/asn2eth.py -X -b -p cms -c cms.cnf -s packet-cms-template CryptographicMessageSyntax.asn */

static int hf_cms_version = -1;                   /* CMSVersion */
static int hf_cms_digestAlgorithms = -1;          /* DigestAlgorithmIdentifiers */
static int hf_cms_encapContentInfo = -1;          /* EncapsulatedContentInfo */
static int hf_cms_certificates = -1;              /* CertificateSet */
static int hf_cms_crls = -1;                      /* CertificateRevocationLists */
static int hf_cms_signerInfos = -1;               /* SignerInfos */
static int hf_cms_DigestAlgorithmIdentifiers_item = -1;  /* DigestAlgorithmIdentifier */
static int hf_cms_SignerInfos_item = -1;          /* SignerInfo */
static int hf_cms_eContentType = -1;              /* ContentType */
static int hf_cms_eContent = -1;                  /* OCTET_STRING */
static int hf_cms_sid = -1;                       /* SignerIdentifier */
static int hf_cms_digestAlgorithm = -1;           /* DigestAlgorithmIdentifier */
static int hf_cms_signedAttrs = -1;               /* SignedAttributes */
static int hf_cms_signatureAlgorithm = -1;        /* SignatureAlgorithmIdentifier */
static int hf_cms_signature = -1;                 /* SignatureValue */
static int hf_cms_unsignedAttrs = -1;             /* UnsignedAttributes */
static int hf_cms_issuerAndSerialNumber = -1;     /* IssuerAndSerialNumber */
static int hf_cms_subjectKeyIdentifier = -1;      /* SubjectKeyIdentifier */
static int hf_cms_SignedAttributes_item = -1;     /* Attribute */
static int hf_cms_UnsignedAttributes_item = -1;   /* Attribute */
static int hf_cms_attrType = -1;                  /* OBJECT_IDENTIFIER */
static int hf_cms_originatorInfo = -1;            /* OriginatorInfo */
static int hf_cms_recipientInfos = -1;            /* RecipientInfos */
static int hf_cms_encryptedContentInfo = -1;      /* EncryptedContentInfo */
static int hf_cms_unprotectedAttrs = -1;          /* UnprotectedAttributes */
static int hf_cms_certs = -1;                     /* CertificateSet */
static int hf_cms_RecipientInfos_item = -1;       /* RecipientInfo */
static int hf_cms_contentType = -1;               /* ContentType */
static int hf_cms_contentEncryptionAlgorithm = -1;  /* ContentEncryptionAlgorithmIdentifier */
static int hf_cms_encryptedContent = -1;          /* EncryptedContent */
static int hf_cms_UnprotectedAttributes_item = -1;  /* Attribute */
static int hf_cms_ktri = -1;                      /* KeyTransRecipientInfo */
static int hf_cms_kari = -1;                      /* KeyAgreeRecipientInfo */
static int hf_cms_kekri = -1;                     /* KEKRecipientInfo */
static int hf_cms_rid = -1;                       /* RecipientIdentifier */
static int hf_cms_keyEncryptionAlgorithm = -1;    /* KeyEncryptionAlgorithmIdentifier */
static int hf_cms_encryptedKey = -1;              /* EncryptedKey */
static int hf_cms_originator = -1;                /* OriginatorIdentifierOrKey */
static int hf_cms_ukm = -1;                       /* UserKeyingMaterial */
static int hf_cms_recipientEncryptedKeys = -1;    /* RecipientEncryptedKeys */
static int hf_cms_originatorKey = -1;             /* OriginatorPublicKey */
static int hf_cms_algorithm = -1;                 /* AlgorithmIdentifier */
static int hf_cms_publicKey = -1;                 /* BIT_STRING */
static int hf_cms_RecipientEncryptedKeys_item = -1;  /* RecipientEncryptedKey */
static int hf_cms_rid1 = -1;                      /* KeyAgreeRecipientIdentifier */
static int hf_cms_rKeyId = -1;                    /* RecipientKeyIdentifier */
static int hf_cms_date = -1;                      /* GeneralizedTime */
static int hf_cms_other = -1;                     /* OtherKeyAttribute */
static int hf_cms_kekid = -1;                     /* KEKIdentifier */
static int hf_cms_keyIdentifier = -1;             /* OCTET_STRING */
static int hf_cms_digest = -1;                    /* Digest */
static int hf_cms_macAlgorithm = -1;              /* MessageAuthenticationCodeAlgorithm */
static int hf_cms_authenticatedAttributes = -1;   /* AuthAttributes */
static int hf_cms_mac = -1;                       /* MessageAuthenticationCode */
static int hf_cms_unauthenticatedAttributes = -1;  /* UnauthAttributes */
static int hf_cms_AuthAttributes_item = -1;       /* Attribute */
static int hf_cms_UnauthAttributes_item = -1;     /* Attribute */
static int hf_cms_CertificateRevocationLists_item = -1;  /* CertificateList */
static int hf_cms_certificate = -1;               /* Certificate */
static int hf_cms_extendedCertificate = -1;       /* ExtendedCertificate */
static int hf_cms_attrCert = -1;                  /* AttributeCertificate */
static int hf_cms_CertificateSet_item = -1;       /* CertificateChoices */
static int hf_cms_issuer = -1;                    /* Name */
static int hf_cms_serialNumber = -1;              /* CertificateSerialNumber */
static int hf_cms_extendedCertificateInfo = -1;   /* ExtendedCertificateInfo */
static int hf_cms_signature1 = -1;                /* Signature */
static int hf_cms_attributes = -1;                /* UnauthAttributes */

/*--- End of included file: packet-cms-hf.c ---*/


/* Initialize the subtree pointers */

/*--- Included file: packet-cms-ett.c ---*/

/* Do not modify this file.                                                   */
/* It is created automatically by the ASN.1 to Ethereal dissector compiler    */
/* packet-cms-ett.c                                                           */
/* ../../tools/asn2eth.py -X -b -p cms -c cms.cnf -s packet-cms-template CryptographicMessageSyntax.asn */

static gint ett_cms_SignedData = -1;
static gint ett_cms_DigestAlgorithmIdentifiers = -1;
static gint ett_cms_SignerInfos = -1;
static gint ett_cms_EncapsulatedContentInfo = -1;
static gint ett_cms_SignerInfo = -1;
static gint ett_cms_SignerIdentifier = -1;
static gint ett_cms_SignedAttributes = -1;
static gint ett_cms_UnsignedAttributes = -1;
static gint ett_cms_Attribute = -1;
static gint ett_cms_EnvelopedData = -1;
static gint ett_cms_OriginatorInfo = -1;
static gint ett_cms_RecipientInfos = -1;
static gint ett_cms_EncryptedContentInfo = -1;
static gint ett_cms_UnprotectedAttributes = -1;
static gint ett_cms_RecipientInfo = -1;
static gint ett_cms_KeyTransRecipientInfo = -1;
static gint ett_cms_RecipientIdentifier = -1;
static gint ett_cms_KeyAgreeRecipientInfo = -1;
static gint ett_cms_OriginatorIdentifierOrKey = -1;
static gint ett_cms_OriginatorPublicKey = -1;
static gint ett_cms_RecipientEncryptedKeys = -1;
static gint ett_cms_RecipientEncryptedKey = -1;
static gint ett_cms_KeyAgreeRecipientIdentifier = -1;
static gint ett_cms_RecipientKeyIdentifier = -1;
static gint ett_cms_KEKRecipientInfo = -1;
static gint ett_cms_KEKIdentifier = -1;
static gint ett_cms_DigestedData = -1;
static gint ett_cms_EncryptedData = -1;
static gint ett_cms_AuthenticatedData = -1;
static gint ett_cms_AuthAttributes = -1;
static gint ett_cms_UnauthAttributes = -1;
static gint ett_cms_CertificateRevocationLists = -1;
static gint ett_cms_CertificateChoices = -1;
static gint ett_cms_CertificateSet = -1;
static gint ett_cms_IssuerAndSerialNumber = -1;
static gint ett_cms_OtherKeyAttribute = -1;
static gint ett_cms_ExtendedCertificate = -1;
static gint ett_cms_ExtendedCertificateInfo = -1;

/*--- End of included file: packet-cms-ett.c ---*/


static int dissect_cms_OtherKeyAttribute(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index);



/*--- Included file: packet-cms-fn.c ---*/

/* Do not modify this file.                                                   */
/* It is created automatically by the ASN.1 to Ethereal dissector compiler    */
/* packet-cms-fn.c                                                            */
/* ../../tools/asn2eth.py -X -b -p cms -c cms.cnf -s packet-cms-template CryptographicMessageSyntax.asn */

static int dissect_algorithm(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_x509af_AlgorithmIdentifier(FALSE, tvb, offset, pinfo, tree, hf_cms_algorithm);
}
static int dissect_CertificateRevocationLists_item(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_x509af_CertificateList(FALSE, tvb, offset, pinfo, tree, hf_cms_CertificateRevocationLists_item);
}
static int dissect_certificate(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_x509af_Certificate(FALSE, tvb, offset, pinfo, tree, hf_cms_certificate);
}
static int dissect_attrCert_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_x509af_AttributeCertificate(TRUE, tvb, offset, pinfo, tree, hf_cms_attrCert);
}
static int dissect_issuer(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_x509if_Name(FALSE, tvb, offset, pinfo, tree, hf_cms_issuer);
}
static int dissect_serialNumber(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_x509af_CertificateSerialNumber(FALSE, tvb, offset, pinfo, tree, hf_cms_serialNumber);
}

static int
dissect_cms_ContentType(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_object_identifier(implicit_tag, pinfo, tree, tvb, offset,
                                         hf_index, NULL);

  return offset;
}
static int dissect_eContentType(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_ContentType(FALSE, tvb, offset, pinfo, tree, hf_cms_eContentType);
}
static int dissect_contentType(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_ContentType(FALSE, tvb, offset, pinfo, tree, hf_cms_contentType);
}


static const value_string CMSVersion_vals[] = {
  {   0, "v0" },
  {   1, "v1" },
  {   2, "v2" },
  {   3, "v3" },
  {   4, "v4" },
  { 0, NULL }
};


static int
dissect_cms_CMSVersion(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_integer(pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_version(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_CMSVersion(FALSE, tvb, offset, pinfo, tree, hf_cms_version);
}


static int
dissect_cms_DigestAlgorithmIdentifier(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_x509af_AlgorithmIdentifier(implicit_tag, tvb, offset, pinfo, tree, hf_index);

  return offset;
}
static int dissect_DigestAlgorithmIdentifiers_item(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_DigestAlgorithmIdentifier(FALSE, tvb, offset, pinfo, tree, hf_cms_DigestAlgorithmIdentifiers_item);
}
static int dissect_digestAlgorithm(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_DigestAlgorithmIdentifier(FALSE, tvb, offset, pinfo, tree, hf_cms_digestAlgorithm);
}

static ber_sequence DigestAlgorithmIdentifiers_set_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_DigestAlgorithmIdentifiers_item },
};

static int
dissect_cms_DigestAlgorithmIdentifiers(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_set_of(implicit_tag, pinfo, tree, tvb, offset,
                              DigestAlgorithmIdentifiers_set_of, hf_index, ett_cms_DigestAlgorithmIdentifiers);

  return offset;
}
static int dissect_digestAlgorithms(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_DigestAlgorithmIdentifiers(FALSE, tvb, offset, pinfo, tree, hf_cms_digestAlgorithms);
}


static int
dissect_cms_OCTET_STRING(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_eContent(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_OCTET_STRING(FALSE, tvb, offset, pinfo, tree, hf_cms_eContent);
}
static int dissect_keyIdentifier(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_OCTET_STRING(FALSE, tvb, offset, pinfo, tree, hf_cms_keyIdentifier);
}

static ber_sequence EncapsulatedContentInfo_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_OID, BER_FLAGS_NOOWNTAG, dissect_eContentType },
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL, dissect_eContent },
  { 0, 0, 0, NULL }
};

static int
dissect_cms_EncapsulatedContentInfo(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                EncapsulatedContentInfo_sequence, hf_index, ett_cms_EncapsulatedContentInfo);

  return offset;
}
static int dissect_encapContentInfo(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_EncapsulatedContentInfo(FALSE, tvb, offset, pinfo, tree, hf_cms_encapContentInfo);
}


static int
dissect_cms_OBJECT_IDENTIFIER(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_object_identifier(implicit_tag, pinfo, tree, tvb, offset,
                                         hf_index, NULL);

  return offset;
}
static int dissect_attrType(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_OBJECT_IDENTIFIER(FALSE, tvb, offset, pinfo, tree, hf_cms_attrType);
}

static ber_sequence Attribute_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_OID, BER_FLAGS_NOOWNTAG, dissect_attrType },
  { 0, 0, 0, NULL }
};

static int
dissect_cms_Attribute(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                Attribute_sequence, hf_index, ett_cms_Attribute);

  return offset;
}
static int dissect_SignedAttributes_item(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_Attribute(FALSE, tvb, offset, pinfo, tree, hf_cms_SignedAttributes_item);
}
static int dissect_UnsignedAttributes_item(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_Attribute(FALSE, tvb, offset, pinfo, tree, hf_cms_UnsignedAttributes_item);
}
static int dissect_UnprotectedAttributes_item(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_Attribute(FALSE, tvb, offset, pinfo, tree, hf_cms_UnprotectedAttributes_item);
}
static int dissect_AuthAttributes_item(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_Attribute(FALSE, tvb, offset, pinfo, tree, hf_cms_AuthAttributes_item);
}
static int dissect_UnauthAttributes_item(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_Attribute(FALSE, tvb, offset, pinfo, tree, hf_cms_UnauthAttributes_item);
}

static ber_sequence UnauthAttributes_set_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_UnauthAttributes_item },
};

static int
dissect_cms_UnauthAttributes(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_set_of(implicit_tag, pinfo, tree, tvb, offset,
                              UnauthAttributes_set_of, hf_index, ett_cms_UnauthAttributes);

  return offset;
}
static int dissect_unauthenticatedAttributes_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_UnauthAttributes(TRUE, tvb, offset, pinfo, tree, hf_cms_unauthenticatedAttributes);
}
static int dissect_attributes(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_UnauthAttributes(FALSE, tvb, offset, pinfo, tree, hf_cms_attributes);
}

static ber_sequence ExtendedCertificateInfo_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_version },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_certificate },
  { BER_CLASS_UNI, BER_UNI_TAG_SET, BER_FLAGS_NOOWNTAG, dissect_attributes },
  { 0, 0, 0, NULL }
};

static int
dissect_cms_ExtendedCertificateInfo(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                ExtendedCertificateInfo_sequence, hf_index, ett_cms_ExtendedCertificateInfo);

  return offset;
}
static int dissect_extendedCertificateInfo(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_ExtendedCertificateInfo(FALSE, tvb, offset, pinfo, tree, hf_cms_extendedCertificateInfo);
}


static int
dissect_cms_SignatureAlgorithmIdentifier(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_x509af_AlgorithmIdentifier(implicit_tag, tvb, offset, pinfo, tree, hf_index);

  return offset;
}
static int dissect_signatureAlgorithm(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_SignatureAlgorithmIdentifier(FALSE, tvb, offset, pinfo, tree, hf_cms_signatureAlgorithm);
}


static int
dissect_cms_Signature(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_bitstring(implicit_tag, pinfo, tree, tvb, offset,
                                 NULL, hf_index, -1,
                                 NULL);

  return offset;
}
static int dissect_signature1(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_Signature(FALSE, tvb, offset, pinfo, tree, hf_cms_signature1);
}

static ber_sequence ExtendedCertificate_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_extendedCertificateInfo },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_signatureAlgorithm },
  { BER_CLASS_UNI, BER_UNI_TAG_BITSTRING, BER_FLAGS_NOOWNTAG, dissect_signature1 },
  { 0, 0, 0, NULL }
};

static int
dissect_cms_ExtendedCertificate(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                ExtendedCertificate_sequence, hf_index, ett_cms_ExtendedCertificate);

  return offset;
}
static int dissect_extendedCertificate_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_ExtendedCertificate(TRUE, tvb, offset, pinfo, tree, hf_cms_extendedCertificate);
}


static const value_string CertificateChoices_vals[] = {
  {   0, "certificate" },
  {   1, "extendedCertificate" },
  {   2, "attrCert" },
  { 0, NULL }
};

static ber_choice CertificateChoices_choice[] = {
  {   0, BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_certificate },
  {   1, BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_extendedCertificate_impl },
  {   2, BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_attrCert_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_cms_CertificateChoices(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              CertificateChoices_choice, hf_index, ett_cms_CertificateChoices);

  return offset;
}
static int dissect_CertificateSet_item(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_CertificateChoices(FALSE, tvb, offset, pinfo, tree, hf_cms_CertificateSet_item);
}

static ber_sequence CertificateSet_set_of[1] = {
  { -1/*choice*/ , -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_CertificateSet_item },
};

static int
dissect_cms_CertificateSet(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_set_of(implicit_tag, pinfo, tree, tvb, offset,
                              CertificateSet_set_of, hf_index, ett_cms_CertificateSet);

  return offset;
}
static int dissect_certificates_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_CertificateSet(TRUE, tvb, offset, pinfo, tree, hf_cms_certificates);
}
static int dissect_certs_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_CertificateSet(TRUE, tvb, offset, pinfo, tree, hf_cms_certs);
}

static ber_sequence CertificateRevocationLists_set_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_CertificateRevocationLists_item },
};

static int
dissect_cms_CertificateRevocationLists(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_set_of(implicit_tag, pinfo, tree, tvb, offset,
                              CertificateRevocationLists_set_of, hf_index, ett_cms_CertificateRevocationLists);

  return offset;
}
static int dissect_crls_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_CertificateRevocationLists(TRUE, tvb, offset, pinfo, tree, hf_cms_crls);
}

static ber_sequence IssuerAndSerialNumber_sequence[] = {
  { BER_CLASS_ANY, -1, BER_FLAGS_NOOWNTAG, dissect_issuer },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_serialNumber },
  { 0, 0, 0, NULL }
};

static int
dissect_cms_IssuerAndSerialNumber(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                IssuerAndSerialNumber_sequence, hf_index, ett_cms_IssuerAndSerialNumber);

  return offset;
}
static int dissect_issuerAndSerialNumber(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_IssuerAndSerialNumber(FALSE, tvb, offset, pinfo, tree, hf_cms_issuerAndSerialNumber);
}


static int
dissect_cms_SubjectKeyIdentifier(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_subjectKeyIdentifier(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_SubjectKeyIdentifier(FALSE, tvb, offset, pinfo, tree, hf_cms_subjectKeyIdentifier);
}


static const value_string SignerIdentifier_vals[] = {
  {   0, "issuerAndSerialNumber" },
  {   1, "subjectKeyIdentifier" },
  { 0, NULL }
};

static ber_choice SignerIdentifier_choice[] = {
  {   0, BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_issuerAndSerialNumber },
  {   1, BER_CLASS_CON, 0, 0, dissect_subjectKeyIdentifier },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_cms_SignerIdentifier(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              SignerIdentifier_choice, hf_index, ett_cms_SignerIdentifier);

  return offset;
}
static int dissect_sid(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_SignerIdentifier(FALSE, tvb, offset, pinfo, tree, hf_cms_sid);
}

static ber_sequence SignedAttributes_set_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_SignedAttributes_item },
};

static int
dissect_cms_SignedAttributes(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_set_of(implicit_tag, pinfo, tree, tvb, offset,
                              SignedAttributes_set_of, hf_index, ett_cms_SignedAttributes);

  return offset;
}
static int dissect_signedAttrs_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_SignedAttributes(TRUE, tvb, offset, pinfo, tree, hf_cms_signedAttrs);
}


static int
dissect_cms_SignatureValue(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_signature(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_SignatureValue(FALSE, tvb, offset, pinfo, tree, hf_cms_signature);
}

static ber_sequence UnsignedAttributes_set_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_UnsignedAttributes_item },
};

static int
dissect_cms_UnsignedAttributes(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_set_of(implicit_tag, pinfo, tree, tvb, offset,
                              UnsignedAttributes_set_of, hf_index, ett_cms_UnsignedAttributes);

  return offset;
}
static int dissect_unsignedAttrs_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_UnsignedAttributes(TRUE, tvb, offset, pinfo, tree, hf_cms_unsignedAttrs);
}

static ber_sequence SignerInfo_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_version },
  { -1/*choice*/ , -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_sid },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_digestAlgorithm },
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_signedAttrs_impl },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_signatureAlgorithm },
  { BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_signature },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_unsignedAttrs_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_cms_SignerInfo(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                SignerInfo_sequence, hf_index, ett_cms_SignerInfo);

  return offset;
}
static int dissect_SignerInfos_item(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_SignerInfo(FALSE, tvb, offset, pinfo, tree, hf_cms_SignerInfos_item);
}

static ber_sequence SignerInfos_set_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_SignerInfos_item },
};

static int
dissect_cms_SignerInfos(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_set_of(implicit_tag, pinfo, tree, tvb, offset,
                              SignerInfos_set_of, hf_index, ett_cms_SignerInfos);

  return offset;
}
static int dissect_signerInfos(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_SignerInfos(FALSE, tvb, offset, pinfo, tree, hf_cms_signerInfos);
}

static ber_sequence SignedData_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_version },
  { BER_CLASS_UNI, BER_UNI_TAG_SET, BER_FLAGS_NOOWNTAG, dissect_digestAlgorithms },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_encapContentInfo },
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_certificates_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_crls_impl },
  { BER_CLASS_UNI, BER_UNI_TAG_SET, BER_FLAGS_NOOWNTAG, dissect_signerInfos },
  { 0, 0, 0, NULL }
};

int
dissect_cms_SignedData(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                SignedData_sequence, hf_index, ett_cms_SignedData);

  return offset;
}

static ber_sequence OriginatorInfo_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_certs_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_crls_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_cms_OriginatorInfo(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                OriginatorInfo_sequence, hf_index, ett_cms_OriginatorInfo);

  return offset;
}
static int dissect_originatorInfo_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_OriginatorInfo(TRUE, tvb, offset, pinfo, tree, hf_cms_originatorInfo);
}


static const value_string RecipientIdentifier_vals[] = {
  {   0, "issuerAndSerialNumber" },
  {   1, "subjectKeyIdentifier" },
  { 0, NULL }
};

static ber_choice RecipientIdentifier_choice[] = {
  {   0, BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_issuerAndSerialNumber },
  {   1, BER_CLASS_CON, 0, 0, dissect_subjectKeyIdentifier },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_cms_RecipientIdentifier(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              RecipientIdentifier_choice, hf_index, ett_cms_RecipientIdentifier);

  return offset;
}
static int dissect_rid(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_RecipientIdentifier(FALSE, tvb, offset, pinfo, tree, hf_cms_rid);
}


static int
dissect_cms_KeyEncryptionAlgorithmIdentifier(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_x509af_AlgorithmIdentifier(implicit_tag, tvb, offset, pinfo, tree, hf_index);

  return offset;
}
static int dissect_keyEncryptionAlgorithm(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_KeyEncryptionAlgorithmIdentifier(FALSE, tvb, offset, pinfo, tree, hf_cms_keyEncryptionAlgorithm);
}


static int
dissect_cms_EncryptedKey(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_encryptedKey(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_EncryptedKey(FALSE, tvb, offset, pinfo, tree, hf_cms_encryptedKey);
}

static ber_sequence KeyTransRecipientInfo_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_version },
  { -1/*choice*/ , -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_rid },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_keyEncryptionAlgorithm },
  { BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_encryptedKey },
  { 0, 0, 0, NULL }
};

static int
dissect_cms_KeyTransRecipientInfo(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                KeyTransRecipientInfo_sequence, hf_index, ett_cms_KeyTransRecipientInfo);

  return offset;
}
static int dissect_ktri(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_KeyTransRecipientInfo(FALSE, tvb, offset, pinfo, tree, hf_cms_ktri);
}


static int
dissect_cms_BIT_STRING(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_bitstring(implicit_tag, pinfo, tree, tvb, offset,
                                 NULL, hf_index, -1,
                                 NULL);

  return offset;
}
static int dissect_publicKey(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_BIT_STRING(FALSE, tvb, offset, pinfo, tree, hf_cms_publicKey);
}

static ber_sequence OriginatorPublicKey_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_algorithm },
  { BER_CLASS_UNI, BER_UNI_TAG_BITSTRING, BER_FLAGS_NOOWNTAG, dissect_publicKey },
  { 0, 0, 0, NULL }
};

static int
dissect_cms_OriginatorPublicKey(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                OriginatorPublicKey_sequence, hf_index, ett_cms_OriginatorPublicKey);

  return offset;
}
static int dissect_originatorKey(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_OriginatorPublicKey(FALSE, tvb, offset, pinfo, tree, hf_cms_originatorKey);
}


static const value_string OriginatorIdentifierOrKey_vals[] = {
  {   0, "issuerAndSerialNumber" },
  {   1, "subjectKeyIdentifier" },
  {   2, "originatorKey" },
  { 0, NULL }
};

static ber_choice OriginatorIdentifierOrKey_choice[] = {
  {   0, BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_issuerAndSerialNumber },
  {   1, BER_CLASS_CON, 0, 0, dissect_subjectKeyIdentifier },
  {   2, BER_CLASS_CON, 1, 0, dissect_originatorKey },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_cms_OriginatorIdentifierOrKey(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              OriginatorIdentifierOrKey_choice, hf_index, ett_cms_OriginatorIdentifierOrKey);

  return offset;
}
static int dissect_originator(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_OriginatorIdentifierOrKey(FALSE, tvb, offset, pinfo, tree, hf_cms_originator);
}


static int
dissect_cms_UserKeyingMaterial(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_ukm(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_UserKeyingMaterial(FALSE, tvb, offset, pinfo, tree, hf_cms_ukm);
}


static int
dissect_cms_GeneralizedTime(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_generalized_time(pinfo, tree, tvb, offset, hf_index);

  return offset;
}
static int dissect_date(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_GeneralizedTime(FALSE, tvb, offset, pinfo, tree, hf_cms_date);
}

static int dissect_other(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_OtherKeyAttribute(FALSE, tvb, offset, pinfo, tree, hf_cms_other);
}

static ber_sequence RecipientKeyIdentifier_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_subjectKeyIdentifier },
  { BER_CLASS_UNI, BER_UNI_TAG_GeneralizedTime, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_date },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_other },
  { 0, 0, 0, NULL }
};

static int
dissect_cms_RecipientKeyIdentifier(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                RecipientKeyIdentifier_sequence, hf_index, ett_cms_RecipientKeyIdentifier);

  return offset;
}
static int dissect_rKeyId_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_RecipientKeyIdentifier(TRUE, tvb, offset, pinfo, tree, hf_cms_rKeyId);
}


static const value_string KeyAgreeRecipientIdentifier_vals[] = {
  {   0, "issuerAndSerialNumber" },
  {   1, "rKeyId" },
  { 0, NULL }
};

static ber_choice KeyAgreeRecipientIdentifier_choice[] = {
  {   0, BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_issuerAndSerialNumber },
  {   1, BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_rKeyId_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_cms_KeyAgreeRecipientIdentifier(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              KeyAgreeRecipientIdentifier_choice, hf_index, ett_cms_KeyAgreeRecipientIdentifier);

  return offset;
}
static int dissect_rid1(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_KeyAgreeRecipientIdentifier(FALSE, tvb, offset, pinfo, tree, hf_cms_rid1);
}

static ber_sequence RecipientEncryptedKey_sequence[] = {
  { -1/*choice*/ , -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_rid1 },
  { BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_encryptedKey },
  { 0, 0, 0, NULL }
};

static int
dissect_cms_RecipientEncryptedKey(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                RecipientEncryptedKey_sequence, hf_index, ett_cms_RecipientEncryptedKey);

  return offset;
}
static int dissect_RecipientEncryptedKeys_item(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_RecipientEncryptedKey(FALSE, tvb, offset, pinfo, tree, hf_cms_RecipientEncryptedKeys_item);
}

static ber_sequence RecipientEncryptedKeys_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_RecipientEncryptedKeys_item },
};

static int
dissect_cms_RecipientEncryptedKeys(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_sequence_of(implicit_tag, pinfo, tree, tvb, offset,
                                   RecipientEncryptedKeys_sequence_of, hf_index, ett_cms_RecipientEncryptedKeys);

  return offset;
}
static int dissect_recipientEncryptedKeys(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_RecipientEncryptedKeys(FALSE, tvb, offset, pinfo, tree, hf_cms_recipientEncryptedKeys);
}

static ber_sequence KeyAgreeRecipientInfo_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_version },
  { BER_CLASS_CON, 0, BER_FLAGS_NOTCHKTAG, dissect_originator },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL, dissect_ukm },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_keyEncryptionAlgorithm },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_recipientEncryptedKeys },
  { 0, 0, 0, NULL }
};

static int
dissect_cms_KeyAgreeRecipientInfo(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                KeyAgreeRecipientInfo_sequence, hf_index, ett_cms_KeyAgreeRecipientInfo);

  return offset;
}
static int dissect_kari(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_KeyAgreeRecipientInfo(FALSE, tvb, offset, pinfo, tree, hf_cms_kari);
}

static ber_sequence KEKIdentifier_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_keyIdentifier },
  { BER_CLASS_UNI, BER_UNI_TAG_GeneralizedTime, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_date },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_other },
  { 0, 0, 0, NULL }
};

static int
dissect_cms_KEKIdentifier(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                KEKIdentifier_sequence, hf_index, ett_cms_KEKIdentifier);

  return offset;
}
static int dissect_kekid(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_KEKIdentifier(FALSE, tvb, offset, pinfo, tree, hf_cms_kekid);
}

static ber_sequence KEKRecipientInfo_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_version },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_kekid },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_keyEncryptionAlgorithm },
  { BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_encryptedKey },
  { 0, 0, 0, NULL }
};

static int
dissect_cms_KEKRecipientInfo(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                KEKRecipientInfo_sequence, hf_index, ett_cms_KEKRecipientInfo);

  return offset;
}
static int dissect_kekri(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_KEKRecipientInfo(FALSE, tvb, offset, pinfo, tree, hf_cms_kekri);
}


static const value_string RecipientInfo_vals[] = {
  {   0, "ktri" },
  {   1, "kari" },
  {   2, "kekri" },
  { 0, NULL }
};

static ber_choice RecipientInfo_choice[] = {
  {   0, BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_ktri },
  {   1, BER_CLASS_CON, 1, 0, dissect_kari },
  {   2, BER_CLASS_CON, 2, 0, dissect_kekri },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_cms_RecipientInfo(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                              RecipientInfo_choice, hf_index, ett_cms_RecipientInfo);

  return offset;
}
static int dissect_RecipientInfos_item(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_RecipientInfo(FALSE, tvb, offset, pinfo, tree, hf_cms_RecipientInfos_item);
}

static ber_sequence RecipientInfos_set_of[1] = {
  { -1/*choice*/ , -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_RecipientInfos_item },
};

static int
dissect_cms_RecipientInfos(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_set_of(implicit_tag, pinfo, tree, tvb, offset,
                              RecipientInfos_set_of, hf_index, ett_cms_RecipientInfos);

  return offset;
}
static int dissect_recipientInfos(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_RecipientInfos(FALSE, tvb, offset, pinfo, tree, hf_cms_recipientInfos);
}


static int
dissect_cms_ContentEncryptionAlgorithmIdentifier(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_x509af_AlgorithmIdentifier(implicit_tag, tvb, offset, pinfo, tree, hf_index);

  return offset;
}
static int dissect_contentEncryptionAlgorithm(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_ContentEncryptionAlgorithmIdentifier(FALSE, tvb, offset, pinfo, tree, hf_cms_contentEncryptionAlgorithm);
}


static int
dissect_cms_EncryptedContent(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_encryptedContent_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_EncryptedContent(TRUE, tvb, offset, pinfo, tree, hf_cms_encryptedContent);
}

static ber_sequence EncryptedContentInfo_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_OID, BER_FLAGS_NOOWNTAG, dissect_contentType },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_contentEncryptionAlgorithm },
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_encryptedContent_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_cms_EncryptedContentInfo(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                EncryptedContentInfo_sequence, hf_index, ett_cms_EncryptedContentInfo);

  return offset;
}
static int dissect_encryptedContentInfo(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_EncryptedContentInfo(FALSE, tvb, offset, pinfo, tree, hf_cms_encryptedContentInfo);
}

static ber_sequence UnprotectedAttributes_set_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_UnprotectedAttributes_item },
};

static int
dissect_cms_UnprotectedAttributes(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_set_of(implicit_tag, pinfo, tree, tvb, offset,
                              UnprotectedAttributes_set_of, hf_index, ett_cms_UnprotectedAttributes);

  return offset;
}
static int dissect_unprotectedAttrs_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_UnprotectedAttributes(TRUE, tvb, offset, pinfo, tree, hf_cms_unprotectedAttrs);
}

static ber_sequence EnvelopedData_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_version },
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_originatorInfo_impl },
  { BER_CLASS_UNI, BER_UNI_TAG_SET, BER_FLAGS_NOOWNTAG, dissect_recipientInfos },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_encryptedContentInfo },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_unprotectedAttrs_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_cms_EnvelopedData(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                EnvelopedData_sequence, hf_index, ett_cms_EnvelopedData);

  return offset;
}


static int
dissect_cms_Digest(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_digest(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_Digest(FALSE, tvb, offset, pinfo, tree, hf_cms_digest);
}

static ber_sequence DigestedData_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_version },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_digestAlgorithm },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_encapContentInfo },
  { BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_digest },
  { 0, 0, 0, NULL }
};

static int
dissect_cms_DigestedData(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                DigestedData_sequence, hf_index, ett_cms_DigestedData);

  return offset;
}

static ber_sequence EncryptedData_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_version },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_encryptedContentInfo },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_unprotectedAttrs_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_cms_EncryptedData(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                EncryptedData_sequence, hf_index, ett_cms_EncryptedData);

  return offset;
}


static int
dissect_cms_MessageAuthenticationCodeAlgorithm(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_x509af_AlgorithmIdentifier(implicit_tag, tvb, offset, pinfo, tree, hf_index);

  return offset;
}
static int dissect_macAlgorithm(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_MessageAuthenticationCodeAlgorithm(FALSE, tvb, offset, pinfo, tree, hf_cms_macAlgorithm);
}

static ber_sequence AuthAttributes_set_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_AuthAttributes_item },
};

static int
dissect_cms_AuthAttributes(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_set_of(implicit_tag, pinfo, tree, tvb, offset,
                              AuthAttributes_set_of, hf_index, ett_cms_AuthAttributes);

  return offset;
}
static int dissect_authenticatedAttributes_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_AuthAttributes(TRUE, tvb, offset, pinfo, tree, hf_cms_authenticatedAttributes);
}


static int
dissect_cms_MessageAuthenticationCode(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                    NULL);

  return offset;
}
static int dissect_mac(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_cms_MessageAuthenticationCode(FALSE, tvb, offset, pinfo, tree, hf_cms_mac);
}

static ber_sequence AuthenticatedData_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_version },
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_originatorInfo_impl },
  { BER_CLASS_UNI, BER_UNI_TAG_SET, BER_FLAGS_NOOWNTAG, dissect_recipientInfos },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_macAlgorithm },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL, dissect_digestAlgorithm },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_encapContentInfo },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_authenticatedAttributes_impl },
  { BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_mac },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_unauthenticatedAttributes_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_cms_AuthenticatedData(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                AuthenticatedData_sequence, hf_index, ett_cms_AuthenticatedData);

  return offset;
}


int
dissect_cms_Countersignature(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_cms_SignerInfo(implicit_tag, tvb, offset, pinfo, tree, hf_index);

  return offset;
}


/*--- End of included file: packet-cms-fn.c ---*/



static char keyAttr_id[64]; /*64 chars should be long enough? */
static int 
dissect_keyAttrId(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) 
{
  offset = dissect_ber_object_identifier(FALSE, pinfo, tree, tvb, offset,
                                         hf_cms_keyAttr_id, keyAttr_id);
  return offset;
}

static int 
dissect_keyAttr_type(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) 
{
  offset=call_ber_oid_callback(keyAttr_id, tvb, offset, pinfo, tree);

  return offset;
}

static ber_sequence OtherKeyAttribute_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_OID, BER_FLAGS_NOOWNTAG, dissect_keyAttrId },
  { BER_CLASS_ANY, 0, 0, dissect_keyAttr_type },
  { 0, 0, 0, NULL }
};

static int
dissect_cms_OtherKeyAttribute(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                OtherKeyAttribute_sequence, hf_index, ett_cms_OtherKeyAttribute);

  return offset;
}

static void
dissect_cms_SignedData_callback(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	dissect_cms_SignedData(FALSE, tvb, 0, pinfo, tree, -1);
}

static void
dissect_cms_EnvelopedData_callback(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	dissect_cms_EnvelopedData(FALSE, tvb, 0, pinfo, tree, -1);
}

static void
dissect_cms_DigestedData_callback(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	dissect_cms_DigestedData(FALSE, tvb, 0, pinfo, tree, -1);
}


static void
dissect_cms_EncryptedData_callback(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	dissect_cms_EncryptedData(FALSE, tvb, 0, pinfo, tree, -1);
}

static void
dissect_cms_AuthenticatedData_callback(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	dissect_cms_AuthenticatedData(FALSE, tvb, 0, pinfo, tree, -1);
}

/*--- proto_register_cms ----------------------------------------------*/
void proto_register_cms(void) {

  /* List of fields */
  static hf_register_info hf[] = {
    { &hf_cms_keyAttr_id,
      { "keyAttr_id", "cms.keyAttr_id",
        FT_STRING, BASE_NONE, NULL, 0,
        "keyAttr_id", HFILL }},

/*--- Included file: packet-cms-hfarr.c ---*/

/* Do not modify this file.                                                   */
/* It is created automatically by the ASN.1 to Ethereal dissector compiler    */
/* packet-cms-hfarr.c                                                         */
/* ../../tools/asn2eth.py -X -b -p cms -c cms.cnf -s packet-cms-template CryptographicMessageSyntax.asn */

    { &hf_cms_version,
      { "version", "cms.version",
        FT_INT32, BASE_DEC, VALS(CMSVersion_vals), 0,
        "", HFILL }},
    { &hf_cms_digestAlgorithms,
      { "digestAlgorithms", "cms.digestAlgorithms",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SignedData/digestAlgorithms", HFILL }},
    { &hf_cms_encapContentInfo,
      { "encapContentInfo", "cms.encapContentInfo",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_cms_certificates,
      { "certificates", "cms.certificates",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SignedData/certificates", HFILL }},
    { &hf_cms_crls,
      { "crls", "cms.crls",
        FT_UINT32, BASE_DEC, NULL, 0,
        "", HFILL }},
    { &hf_cms_signerInfos,
      { "signerInfos", "cms.signerInfos",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SignedData/signerInfos", HFILL }},
    { &hf_cms_DigestAlgorithmIdentifiers_item,
      { "Item(##)", "cms.DigestAlgorithmIdentifiers_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "DigestAlgorithmIdentifiers/_item", HFILL }},
    { &hf_cms_SignerInfos_item,
      { "Item(##)", "cms.SignerInfos_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "SignerInfos/_item", HFILL }},
    { &hf_cms_eContentType,
      { "eContentType", "cms.eContentType",
        FT_STRING, BASE_NONE, NULL, 0,
        "EncapsulatedContentInfo/eContentType", HFILL }},
    { &hf_cms_eContent,
      { "eContent", "cms.eContent",
        FT_BYTES, BASE_HEX, NULL, 0,
        "EncapsulatedContentInfo/eContent", HFILL }},
    { &hf_cms_sid,
      { "sid", "cms.sid",
        FT_UINT32, BASE_DEC, VALS(SignerIdentifier_vals), 0,
        "SignerInfo/sid", HFILL }},
    { &hf_cms_digestAlgorithm,
      { "digestAlgorithm", "cms.digestAlgorithm",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_cms_signedAttrs,
      { "signedAttrs", "cms.signedAttrs",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SignerInfo/signedAttrs", HFILL }},
    { &hf_cms_signatureAlgorithm,
      { "signatureAlgorithm", "cms.signatureAlgorithm",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_cms_signature,
      { "signature", "cms.signature",
        FT_BYTES, BASE_HEX, NULL, 0,
        "SignerInfo/signature", HFILL }},
    { &hf_cms_unsignedAttrs,
      { "unsignedAttrs", "cms.unsignedAttrs",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SignerInfo/unsignedAttrs", HFILL }},
    { &hf_cms_issuerAndSerialNumber,
      { "issuerAndSerialNumber", "cms.issuerAndSerialNumber",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_cms_subjectKeyIdentifier,
      { "subjectKeyIdentifier", "cms.subjectKeyIdentifier",
        FT_BYTES, BASE_HEX, NULL, 0,
        "", HFILL }},
    { &hf_cms_SignedAttributes_item,
      { "Item(##)", "cms.SignedAttributes_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "SignedAttributes/_item", HFILL }},
    { &hf_cms_UnsignedAttributes_item,
      { "Item(##)", "cms.UnsignedAttributes_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "UnsignedAttributes/_item", HFILL }},
    { &hf_cms_attrType,
      { "attrType", "cms.attrType",
        FT_STRING, BASE_NONE, NULL, 0,
        "Attribute/attrType", HFILL }},
    { &hf_cms_originatorInfo,
      { "originatorInfo", "cms.originatorInfo",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_cms_recipientInfos,
      { "recipientInfos", "cms.recipientInfos",
        FT_UINT32, BASE_DEC, NULL, 0,
        "", HFILL }},
    { &hf_cms_encryptedContentInfo,
      { "encryptedContentInfo", "cms.encryptedContentInfo",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_cms_unprotectedAttrs,
      { "unprotectedAttrs", "cms.unprotectedAttrs",
        FT_UINT32, BASE_DEC, NULL, 0,
        "", HFILL }},
    { &hf_cms_certs,
      { "certs", "cms.certs",
        FT_UINT32, BASE_DEC, NULL, 0,
        "OriginatorInfo/certs", HFILL }},
    { &hf_cms_RecipientInfos_item,
      { "Item(##)", "cms.RecipientInfos_item",
        FT_UINT32, BASE_DEC, VALS(RecipientInfo_vals), 0,
        "RecipientInfos/_item", HFILL }},
    { &hf_cms_contentType,
      { "contentType", "cms.contentType",
        FT_STRING, BASE_NONE, NULL, 0,
        "EncryptedContentInfo/contentType", HFILL }},
    { &hf_cms_contentEncryptionAlgorithm,
      { "contentEncryptionAlgorithm", "cms.contentEncryptionAlgorithm",
        FT_NONE, BASE_NONE, NULL, 0,
        "EncryptedContentInfo/contentEncryptionAlgorithm", HFILL }},
    { &hf_cms_encryptedContent,
      { "encryptedContent", "cms.encryptedContent",
        FT_BYTES, BASE_HEX, NULL, 0,
        "EncryptedContentInfo/encryptedContent", HFILL }},
    { &hf_cms_UnprotectedAttributes_item,
      { "Item(##)", "cms.UnprotectedAttributes_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "UnprotectedAttributes/_item", HFILL }},
    { &hf_cms_ktri,
      { "ktri", "cms.ktri",
        FT_NONE, BASE_NONE, NULL, 0,
        "RecipientInfo/ktri", HFILL }},
    { &hf_cms_kari,
      { "kari", "cms.kari",
        FT_NONE, BASE_NONE, NULL, 0,
        "RecipientInfo/kari", HFILL }},
    { &hf_cms_kekri,
      { "kekri", "cms.kekri",
        FT_NONE, BASE_NONE, NULL, 0,
        "RecipientInfo/kekri", HFILL }},
    { &hf_cms_rid,
      { "rid", "cms.rid",
        FT_UINT32, BASE_DEC, VALS(RecipientIdentifier_vals), 0,
        "KeyTransRecipientInfo/rid", HFILL }},
    { &hf_cms_keyEncryptionAlgorithm,
      { "keyEncryptionAlgorithm", "cms.keyEncryptionAlgorithm",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_cms_encryptedKey,
      { "encryptedKey", "cms.encryptedKey",
        FT_BYTES, BASE_HEX, NULL, 0,
        "", HFILL }},
    { &hf_cms_originator,
      { "originator", "cms.originator",
        FT_UINT32, BASE_DEC, VALS(OriginatorIdentifierOrKey_vals), 0,
        "KeyAgreeRecipientInfo/originator", HFILL }},
    { &hf_cms_ukm,
      { "ukm", "cms.ukm",
        FT_BYTES, BASE_HEX, NULL, 0,
        "KeyAgreeRecipientInfo/ukm", HFILL }},
    { &hf_cms_recipientEncryptedKeys,
      { "recipientEncryptedKeys", "cms.recipientEncryptedKeys",
        FT_UINT32, BASE_DEC, NULL, 0,
        "KeyAgreeRecipientInfo/recipientEncryptedKeys", HFILL }},
    { &hf_cms_originatorKey,
      { "originatorKey", "cms.originatorKey",
        FT_NONE, BASE_NONE, NULL, 0,
        "OriginatorIdentifierOrKey/originatorKey", HFILL }},
    { &hf_cms_algorithm,
      { "algorithm", "cms.algorithm",
        FT_NONE, BASE_NONE, NULL, 0,
        "OriginatorPublicKey/algorithm", HFILL }},
    { &hf_cms_publicKey,
      { "publicKey", "cms.publicKey",
        FT_BYTES, BASE_HEX, NULL, 0,
        "OriginatorPublicKey/publicKey", HFILL }},
    { &hf_cms_RecipientEncryptedKeys_item,
      { "Item[##]", "cms.RecipientEncryptedKeys_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "RecipientEncryptedKeys/_item", HFILL }},
    { &hf_cms_rid1,
      { "rid", "cms.rid",
        FT_UINT32, BASE_DEC, VALS(KeyAgreeRecipientIdentifier_vals), 0,
        "RecipientEncryptedKey/rid", HFILL }},
    { &hf_cms_rKeyId,
      { "rKeyId", "cms.rKeyId",
        FT_NONE, BASE_NONE, NULL, 0,
        "KeyAgreeRecipientIdentifier/rKeyId", HFILL }},
    { &hf_cms_date,
      { "date", "cms.date",
        FT_STRING, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_cms_other,
      { "other", "cms.other",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_cms_kekid,
      { "kekid", "cms.kekid",
        FT_NONE, BASE_NONE, NULL, 0,
        "KEKRecipientInfo/kekid", HFILL }},
    { &hf_cms_keyIdentifier,
      { "keyIdentifier", "cms.keyIdentifier",
        FT_BYTES, BASE_HEX, NULL, 0,
        "KEKIdentifier/keyIdentifier", HFILL }},
    { &hf_cms_digest,
      { "digest", "cms.digest",
        FT_BYTES, BASE_HEX, NULL, 0,
        "DigestedData/digest", HFILL }},
    { &hf_cms_macAlgorithm,
      { "macAlgorithm", "cms.macAlgorithm",
        FT_NONE, BASE_NONE, NULL, 0,
        "AuthenticatedData/macAlgorithm", HFILL }},
    { &hf_cms_authenticatedAttributes,
      { "authenticatedAttributes", "cms.authenticatedAttributes",
        FT_UINT32, BASE_DEC, NULL, 0,
        "AuthenticatedData/authenticatedAttributes", HFILL }},
    { &hf_cms_mac,
      { "mac", "cms.mac",
        FT_BYTES, BASE_HEX, NULL, 0,
        "AuthenticatedData/mac", HFILL }},
    { &hf_cms_unauthenticatedAttributes,
      { "unauthenticatedAttributes", "cms.unauthenticatedAttributes",
        FT_UINT32, BASE_DEC, NULL, 0,
        "AuthenticatedData/unauthenticatedAttributes", HFILL }},
    { &hf_cms_AuthAttributes_item,
      { "Item(##)", "cms.AuthAttributes_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "AuthAttributes/_item", HFILL }},
    { &hf_cms_UnauthAttributes_item,
      { "Item(##)", "cms.UnauthAttributes_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "UnauthAttributes/_item", HFILL }},
    { &hf_cms_CertificateRevocationLists_item,
      { "Item(##)", "cms.CertificateRevocationLists_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "CertificateRevocationLists/_item", HFILL }},
    { &hf_cms_certificate,
      { "certificate", "cms.certificate",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_cms_extendedCertificate,
      { "extendedCertificate", "cms.extendedCertificate",
        FT_NONE, BASE_NONE, NULL, 0,
        "CertificateChoices/extendedCertificate", HFILL }},
    { &hf_cms_attrCert,
      { "attrCert", "cms.attrCert",
        FT_NONE, BASE_NONE, NULL, 0,
        "CertificateChoices/attrCert", HFILL }},
    { &hf_cms_CertificateSet_item,
      { "Item(##)", "cms.CertificateSet_item",
        FT_UINT32, BASE_DEC, VALS(CertificateChoices_vals), 0,
        "CertificateSet/_item", HFILL }},
    { &hf_cms_issuer,
      { "issuer", "cms.issuer",
        FT_UINT32, BASE_DEC, VALS(Name_vals), 0,
        "IssuerAndSerialNumber/issuer", HFILL }},
    { &hf_cms_serialNumber,
      { "serialNumber", "cms.serialNumber",
        FT_INT32, BASE_DEC, NULL, 0,
        "IssuerAndSerialNumber/serialNumber", HFILL }},
    { &hf_cms_extendedCertificateInfo,
      { "extendedCertificateInfo", "cms.extendedCertificateInfo",
        FT_NONE, BASE_NONE, NULL, 0,
        "ExtendedCertificate/extendedCertificateInfo", HFILL }},
    { &hf_cms_signature1,
      { "signature", "cms.signature",
        FT_BYTES, BASE_HEX, NULL, 0,
        "ExtendedCertificate/signature", HFILL }},
    { &hf_cms_attributes,
      { "attributes", "cms.attributes",
        FT_UINT32, BASE_DEC, NULL, 0,
        "ExtendedCertificateInfo/attributes", HFILL }},

/*--- End of included file: packet-cms-hfarr.c ---*/

  };

  /* List of subtrees */
  static gint *ett[] = {

/*--- Included file: packet-cms-ettarr.c ---*/

/* Do not modify this file.                                                   */
/* It is created automatically by the ASN.1 to Ethereal dissector compiler    */
/* packet-cms-ettarr.c                                                        */
/* ../../tools/asn2eth.py -X -b -p cms -c cms.cnf -s packet-cms-template CryptographicMessageSyntax.asn */

    &ett_cms_SignedData,
    &ett_cms_DigestAlgorithmIdentifiers,
    &ett_cms_SignerInfos,
    &ett_cms_EncapsulatedContentInfo,
    &ett_cms_SignerInfo,
    &ett_cms_SignerIdentifier,
    &ett_cms_SignedAttributes,
    &ett_cms_UnsignedAttributes,
    &ett_cms_Attribute,
    &ett_cms_EnvelopedData,
    &ett_cms_OriginatorInfo,
    &ett_cms_RecipientInfos,
    &ett_cms_EncryptedContentInfo,
    &ett_cms_UnprotectedAttributes,
    &ett_cms_RecipientInfo,
    &ett_cms_KeyTransRecipientInfo,
    &ett_cms_RecipientIdentifier,
    &ett_cms_KeyAgreeRecipientInfo,
    &ett_cms_OriginatorIdentifierOrKey,
    &ett_cms_OriginatorPublicKey,
    &ett_cms_RecipientEncryptedKeys,
    &ett_cms_RecipientEncryptedKey,
    &ett_cms_KeyAgreeRecipientIdentifier,
    &ett_cms_RecipientKeyIdentifier,
    &ett_cms_KEKRecipientInfo,
    &ett_cms_KEKIdentifier,
    &ett_cms_DigestedData,
    &ett_cms_EncryptedData,
    &ett_cms_AuthenticatedData,
    &ett_cms_AuthAttributes,
    &ett_cms_UnauthAttributes,
    &ett_cms_CertificateRevocationLists,
    &ett_cms_CertificateChoices,
    &ett_cms_CertificateSet,
    &ett_cms_IssuerAndSerialNumber,
    &ett_cms_OtherKeyAttribute,
    &ett_cms_ExtendedCertificate,
    &ett_cms_ExtendedCertificateInfo,

/*--- End of included file: packet-cms-ettarr.c ---*/

  };

  /* Register protocol */
  proto_cms = proto_register_protocol(PNAME, PSNAME, PFNAME);

  /* Register fields and subtrees */
  proto_register_field_array(proto_cms, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));

}


/*--- proto_reg_handoff_cms -------------------------------------------*/
void proto_reg_handoff_cms(void) {
	register_ber_oid_dissector("1.2.840.113549.1.7.2", dissect_cms_SignedData_callback, proto_cms, "id-signedData");
	register_ber_oid_dissector("1.2.840.113549.1.7.3", dissect_cms_EnvelopedData_callback, proto_cms, "id-envelopedData");
	register_ber_oid_dissector("1.2.840.113549.1.7.5", dissect_cms_DigestedData_callback, proto_cms, "id-digestedData");
	register_ber_oid_dissector("1.2.840.113549.1.7.6", dissect_cms_EncryptedData_callback, proto_cms, "id-encryptedData");
	register_ber_oid_dissector("1.2.840.113549.1.9.16.1.2", dissect_cms_AuthenticatedData_callback, proto_cms, "id-ct-authenticatedData");
}

