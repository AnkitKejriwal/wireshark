/* Do not modify this file.                                                   */
/* It is created automatically by the ASN.1 to Ethereal dissector compiler    */
/* packet-h235.c                                                              */
/* ../../tools/asn2eth.py -X -p h235 -c h235.cnf -s packet-h235-template H235-SECURITY-MESSAGES.asn */

/* Input file: packet-h235-template.c */
/* Include files: packet-h235-hf.c, packet-h235-ett.c, packet-h235-fn.c, packet-h235-hfarr.c, packet-h235-ettarr.c */

/* packet-h235.c
 * Routines for H.235 packet dissection
 * 2004  Tomas Kukosa
 *
 * $Id: packet-h235.c,v 1.5 2004/06/04 11:30:35 sahlberg Exp $
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

#include "packet-per.h"
#include "packet-h235.h"

#define PNAME  "H235-SECURITY-MESSAGES"
#define PSNAME "H235"
#define PFNAME "h235"

/* Initialize the protocol and registered fields */
int proto_h235 = -1;

/*--- Included file: packet-h235-hf.c ---*/

/* Do not modify this file.                                                   */
/* It is created automatically by the ASN.1 to Ethereal dissector compiler    */
/* packet-h235-hf.c                                                           */
/* ../../tools/asn2eth.py -X -p h235 -c h235.cnf -s packet-h235-template H235-SECURITY-MESSAGES.asn */

static int hf_h235_nonStandardIdentifier = -1;    /* OBJECT_IDENTIFIER */
static int hf_h235_data = -1;                     /* OCTET_STRING */
static int hf_h235_halfkey = -1;                  /* BIT_STRING_SIZE_0_2048 */
static int hf_h235_modSize = -1;                  /* BIT_STRING_SIZE_0_2048 */
static int hf_h235_generator = -1;                /* BIT_STRING_SIZE_0_2048 */
static int hf_h235_x = -1;                        /* BIT_STRING_SIZE_0_511 */
static int hf_h235_y = -1;                        /* BIT_STRING_SIZE_0_511 */
static int hf_h235_eckasdhp = -1;                 /* T_eckasdhp */
static int hf_h235_public_key = -1;               /* ECpoint */
static int hf_h235_modulus = -1;                  /* BIT_STRING_SIZE_0_511 */
static int hf_h235_base = -1;                     /* ECpoint */
static int hf_h235_weierstrassA = -1;             /* BIT_STRING_SIZE_0_511 */
static int hf_h235_weierstrassB = -1;             /* BIT_STRING_SIZE_0_511 */
static int hf_h235_eckasdh2 = -1;                 /* T_eckasdh2 */
static int hf_h235_fieldSize = -1;                /* BIT_STRING_SIZE_0_511 */
static int hf_h235_type = -1;                     /* OBJECT_IDENTIFIER */
static int hf_h235_certificatedata = -1;          /* OCTET_STRING */
static int hf_h235_default = -1;                  /* NULL */
static int hf_h235_radius = -1;                   /* NULL */
static int hf_h235_dhExch = -1;                   /* NULL */
static int hf_h235_pwdSymEnc = -1;                /* NULL */
static int hf_h235_pwdHash = -1;                  /* NULL */
static int hf_h235_certSign = -1;                 /* NULL */
static int hf_h235_ipsec = -1;                    /* NULL */
static int hf_h235_tls = -1;                      /* NULL */
static int hf_h235_nonStandard = -1;              /* NonStandardParameter */
static int hf_h235_authenticationBES = -1;        /* AuthenticationBES */
static int hf_h235_tokenOID = -1;                 /* OBJECT_IDENTIFIER */
static int hf_h235_timeStamp = -1;                /* TimeStamp */
static int hf_h235_password = -1;                 /* Password */
static int hf_h235_dhkey = -1;                    /* DHset */
static int hf_h235_challenge = -1;                /* ChallengeString */
static int hf_h235_random = -1;                   /* RandomVal */
static int hf_h235_certificate = -1;              /* TypedCertificate */
static int hf_h235_generalID = -1;                /* Identifier */
static int hf_h235_eckasdhkey = -1;               /* ECKASDH */
static int hf_h235_sendersID = -1;                /* Identifier */
static int hf_h235_h235Key = -1;                  /* H235Key */
static int hf_h235_toBeSigned = -1;               /* ToBeSigned */
static int hf_h235_algorithmOID = -1;             /* OBJECT_IDENTIFIER */
static int hf_h235_paramS = -1;                   /* Params */
static int hf_h235_signaturedata = -1;            /* BIT_STRING */
static int hf_h235_encryptedData = -1;            /* OCTET_STRING */
static int hf_h235_hash = -1;                     /* BIT_STRING */
static int hf_h235_ranInt = -1;                   /* INTEGER */
static int hf_h235_iv8 = -1;                      /* IV8 */
static int hf_h235_iv16 = -1;                     /* IV16 */
static int hf_h235_iv = -1;                       /* OCTET_STRING */
static int hf_h235_clearSalt = -1;                /* OCTET_STRING */
static int hf_h235_cryptoEncryptedToken = -1;     /* T_cryptoEncryptedToken */
static int hf_h235_encryptedToken = -1;           /* ENCRYPTEDxxx */
static int hf_h235_cryptoSignedToken = -1;        /* T_cryptoSignedToken */
static int hf_h235_signedToken = -1;              /* SIGNEDxxx */
static int hf_h235_cryptoHashedToken = -1;        /* T_cryptoHashedToken */
static int hf_h235_hashedVals = -1;               /* ClearToken */
static int hf_h235_hashedToken = -1;              /* HASHEDxxx */
static int hf_h235_cryptoPwdEncr = -1;            /* ENCRYPTEDxxx */
static int hf_h235_secureChannel = -1;            /* KeyMaterial */
static int hf_h235_sharedSecret = -1;             /* ENCRYPTEDxxx */
static int hf_h235_certProtectedKey = -1;         /* SIGNEDxxx */
static int hf_h235_secureSharedSecret = -1;       /* V3KeySyncMaterial */
static int hf_h235_encryptedSessionKey = -1;      /* OCTET_STRING */
static int hf_h235_encryptedSaltingKey = -1;      /* OCTET_STRING */
static int hf_h235_clearSaltingKey = -1;          /* OCTET_STRING */
static int hf_h235_paramSsalt = -1;               /* Params */
static int hf_h235_keyDerivationOID = -1;         /* OBJECT_IDENTIFIER */

/*--- End of included file: packet-h235-hf.c ---*/


/* Initialize the subtree pointers */

/*--- Included file: packet-h235-ett.c ---*/

/* Do not modify this file.                                                   */
/* It is created automatically by the ASN.1 to Ethereal dissector compiler    */
/* packet-h235-ett.c                                                          */
/* ../../tools/asn2eth.py -X -p h235 -c h235.cnf -s packet-h235-template H235-SECURITY-MESSAGES.asn */

static gint ett_h235_NonStandardParameter = -1;
static gint ett_h235_DHset = -1;
static gint ett_h235_ECpoint = -1;
static gint ett_h235_ECKASDH = -1;
static gint ett_h235_T_eckasdhp = -1;
static gint ett_h235_T_eckasdh2 = -1;
static gint ett_h235_TypedCertificate = -1;
static gint ett_h235_AuthenticationBES = -1;
static gint ett_h235_AuthenticationMechanism = -1;
static gint ett_h235_ClearToken = -1;
static gint ett_h235_SIGNEDxxx = -1;
static gint ett_h235_ENCRYPTEDxxx = -1;
static gint ett_h235_HASHEDxxx = -1;
static gint ett_h235_Params = -1;
static gint ett_h235_CryptoToken = -1;
static gint ett_h235_T_cryptoEncryptedToken = -1;
static gint ett_h235_T_cryptoSignedToken = -1;
static gint ett_h235_T_cryptoHashedToken = -1;
static gint ett_h235_H235Key = -1;
static gint ett_h235_V3KeySyncMaterial = -1;

/*--- End of included file: packet-h235-ett.c ---*/


static guint32
dissect_xxx_ToBeSigned(tvbuff_t *tvb, guint32 offset, packet_info *pinfo, proto_tree *tree, int hf_index _U_) {
PER_NOT_DECODED_YET("ToBeSigned");
  return offset;
}


/*--- Included file: packet-h235-fn.c ---*/

/* Do not modify this file.                                                   */
/* It is created automatically by the ASN.1 to Ethereal dissector compiler    */
/* packet-h235-fn.c                                                           */
/* ../../tools/asn2eth.py -X -p h235 -c h235.cnf -s packet-h235-template H235-SECURITY-MESSAGES.asn */

static int dissect_hf_h235_toBeSigned(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_xxx_ToBeSigned(tvb, offset, pinfo, tree, hf_h235_toBeSigned);
}

static int
dissect_h235_ChallengeString(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_per_octet_string(tvb, offset, pinfo, tree, hf_index,
                                    8, 128,
                                    NULL, NULL);

  return offset;
}
static int dissect_hf_h235_challenge(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_ChallengeString(tvb, offset, pinfo, tree, hf_h235_challenge);
}



int
dissect_h235_TimeStamp(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_per_constrained_integer(tvb, offset, pinfo, tree, hf_index,
                                           1U, 4294967295U, NULL, NULL, FALSE);

  return offset;
}
static int dissect_hf_h235_timeStamp(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_TimeStamp(tvb, offset, pinfo, tree, hf_h235_timeStamp);
}



static int
dissect_h235_RandomVal(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_per_integer(tvb, offset, pinfo, tree, hf_index,
                               NULL, NULL);

  return offset;
}
static int dissect_hf_h235_random(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_RandomVal(tvb, offset, pinfo, tree, hf_h235_random);
}


static int
dissect_h235_Password(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_per_BMPString(tvb, offset, pinfo, tree, hf_index,
                                 1, 128);

  return offset;
}
static int dissect_hf_h235_password(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_Password(tvb, offset, pinfo, tree, hf_h235_password);
}


static int
dissect_h235_Identifier(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_per_BMPString(tvb, offset, pinfo, tree, hf_index,
                                 1, 128);

  return offset;
}
static int dissect_hf_h235_generalID(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_Identifier(tvb, offset, pinfo, tree, hf_h235_generalID);
}
static int dissect_hf_h235_sendersID(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_Identifier(tvb, offset, pinfo, tree, hf_h235_sendersID);
}


static int
dissect_h235_KeyMaterial(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_per_bit_string(tvb, offset, pinfo, tree, hf_index,
                                  1, 2048);

  return offset;
}
static int dissect_hf_h235_secureChannel(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_KeyMaterial(tvb, offset, pinfo, tree, hf_h235_secureChannel);
}


static int
dissect_h235_OBJECT_IDENTIFIER(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_per_object_identifier(tvb, offset, pinfo, tree, hf_index,
                                         NULL);

  return offset;
}
static int dissect_hf_h235_nonStandardIdentifier(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_OBJECT_IDENTIFIER(tvb, offset, pinfo, tree, hf_h235_nonStandardIdentifier);
}
static int dissect_hf_h235_type(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_OBJECT_IDENTIFIER(tvb, offset, pinfo, tree, hf_h235_type);
}
static int dissect_hf_h235_tokenOID(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_OBJECT_IDENTIFIER(tvb, offset, pinfo, tree, hf_h235_tokenOID);
}
static int dissect_hf_h235_algorithmOID(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_OBJECT_IDENTIFIER(tvb, offset, pinfo, tree, hf_h235_algorithmOID);
}
static int dissect_hf_h235_keyDerivationOID(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_OBJECT_IDENTIFIER(tvb, offset, pinfo, tree, hf_h235_keyDerivationOID);
}


static int
dissect_h235_OCTET_STRING(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_per_octet_string(tvb, offset, pinfo, tree, hf_index,
                                    -1, -1,
                                    NULL, NULL);

  return offset;
}
static int dissect_hf_h235_data(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_OCTET_STRING(tvb, offset, pinfo, tree, hf_h235_data);
}
static int dissect_hf_h235_certificatedata(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_OCTET_STRING(tvb, offset, pinfo, tree, hf_h235_certificatedata);
}
static int dissect_hf_h235_encryptedData(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_OCTET_STRING(tvb, offset, pinfo, tree, hf_h235_encryptedData);
}
static int dissect_hf_h235_iv(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_OCTET_STRING(tvb, offset, pinfo, tree, hf_h235_iv);
}
static int dissect_hf_h235_clearSalt(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_OCTET_STRING(tvb, offset, pinfo, tree, hf_h235_clearSalt);
}
static int dissect_hf_h235_encryptedSessionKey(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_OCTET_STRING(tvb, offset, pinfo, tree, hf_h235_encryptedSessionKey);
}
static int dissect_hf_h235_encryptedSaltingKey(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_OCTET_STRING(tvb, offset, pinfo, tree, hf_h235_encryptedSaltingKey);
}
static int dissect_hf_h235_clearSaltingKey(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_OCTET_STRING(tvb, offset, pinfo, tree, hf_h235_clearSaltingKey);
}

static per_sequence_t NonStandardParameter_sequence[] = {
  { "nonStandardIdentifier"       , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_hf_h235_nonStandardIdentifier },
  { "data"                        , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_hf_h235_data },
  { NULL, 0, 0, NULL }
};

static int
dissect_h235_NonStandardParameter(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_per_sequence(tvb, offset, pinfo, tree, hf_index,
                                ett_h235_NonStandardParameter, NonStandardParameter_sequence);

  return offset;
}
static int dissect_hf_h235_nonStandard(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_NonStandardParameter(tvb, offset, pinfo, tree, hf_h235_nonStandard);
}


static int
dissect_h235_BIT_STRING_SIZE_0_2048(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_per_bit_string(tvb, offset, pinfo, tree, hf_index,
                                  0, 2048);

  return offset;
}
static int dissect_hf_h235_halfkey(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_BIT_STRING_SIZE_0_2048(tvb, offset, pinfo, tree, hf_h235_halfkey);
}
static int dissect_hf_h235_modSize(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_BIT_STRING_SIZE_0_2048(tvb, offset, pinfo, tree, hf_h235_modSize);
}
static int dissect_hf_h235_generator(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_BIT_STRING_SIZE_0_2048(tvb, offset, pinfo, tree, hf_h235_generator);
}

static per_sequence_t DHset_sequence[] = {
  { "halfkey"                     , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_hf_h235_halfkey },
  { "modSize"                     , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_hf_h235_modSize },
  { "generator"                   , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_hf_h235_generator },
  { NULL, 0, 0, NULL }
};

static int
dissect_h235_DHset(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_per_sequence(tvb, offset, pinfo, tree, hf_index,
                                ett_h235_DHset, DHset_sequence);

  return offset;
}
static int dissect_hf_h235_dhkey(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_DHset(tvb, offset, pinfo, tree, hf_h235_dhkey);
}


static int
dissect_h235_BIT_STRING_SIZE_0_511(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_per_bit_string(tvb, offset, pinfo, tree, hf_index,
                                  0, 511);

  return offset;
}
static int dissect_hf_h235_x(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_BIT_STRING_SIZE_0_511(tvb, offset, pinfo, tree, hf_h235_x);
}
static int dissect_hf_h235_y(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_BIT_STRING_SIZE_0_511(tvb, offset, pinfo, tree, hf_h235_y);
}
static int dissect_hf_h235_modulus(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_BIT_STRING_SIZE_0_511(tvb, offset, pinfo, tree, hf_h235_modulus);
}
static int dissect_hf_h235_weierstrassA(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_BIT_STRING_SIZE_0_511(tvb, offset, pinfo, tree, hf_h235_weierstrassA);
}
static int dissect_hf_h235_weierstrassB(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_BIT_STRING_SIZE_0_511(tvb, offset, pinfo, tree, hf_h235_weierstrassB);
}
static int dissect_hf_h235_fieldSize(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_BIT_STRING_SIZE_0_511(tvb, offset, pinfo, tree, hf_h235_fieldSize);
}

static per_sequence_t ECpoint_sequence[] = {
  { "x"                           , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_hf_h235_x },
  { "y"                           , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_hf_h235_y },
  { NULL, 0, 0, NULL }
};

static int
dissect_h235_ECpoint(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_per_sequence(tvb, offset, pinfo, tree, hf_index,
                                ett_h235_ECpoint, ECpoint_sequence);

  return offset;
}
static int dissect_hf_h235_public_key(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_ECpoint(tvb, offset, pinfo, tree, hf_h235_public_key);
}
static int dissect_hf_h235_base(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_ECpoint(tvb, offset, pinfo, tree, hf_h235_base);
}

static per_sequence_t T_eckasdhp_sequence[] = {
  { "public-key"                  , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_hf_h235_public_key },
  { "modulus"                     , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_hf_h235_modulus },
  { "base"                        , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_hf_h235_base },
  { "weierstrassA"                , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_hf_h235_weierstrassA },
  { "weierstrassB"                , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_hf_h235_weierstrassB },
  { NULL, 0, 0, NULL }
};

static int
dissect_h235_T_eckasdhp(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_per_sequence(tvb, offset, pinfo, tree, hf_index,
                                ett_h235_T_eckasdhp, T_eckasdhp_sequence);

  return offset;
}
static int dissect_hf_h235_eckasdhp(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_T_eckasdhp(tvb, offset, pinfo, tree, hf_h235_eckasdhp);
}

static per_sequence_t T_eckasdh2_sequence[] = {
  { "public-key"                  , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_hf_h235_public_key },
  { "fieldSize"                   , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_hf_h235_fieldSize },
  { "base"                        , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_hf_h235_base },
  { "weierstrassA"                , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_hf_h235_weierstrassA },
  { "weierstrassB"                , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_hf_h235_weierstrassB },
  { NULL, 0, 0, NULL }
};

static int
dissect_h235_T_eckasdh2(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_per_sequence(tvb, offset, pinfo, tree, hf_index,
                                ett_h235_T_eckasdh2, T_eckasdh2_sequence);

  return offset;
}
static int dissect_hf_h235_eckasdh2(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_T_eckasdh2(tvb, offset, pinfo, tree, hf_h235_eckasdh2);
}


static const value_string ECKASDH_vals[] = {
  {   0, "eckasdhp" },
  {   1, "eckasdh2" },
  { 0, NULL }
};

static per_choice_t ECKASDH_choice[] = {
  {   0, "eckasdhp"                    , ASN1_EXTENSION_ROOT    , dissect_hf_h235_eckasdhp },
  {   1, "eckasdh2"                    , ASN1_EXTENSION_ROOT    , dissect_hf_h235_eckasdh2 },
  { 0, NULL, 0, NULL }
};

static int
dissect_h235_ECKASDH(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_per_choice(tvb, offset, pinfo, tree, hf_index,
                              ett_h235_ECKASDH, ECKASDH_choice, "ECKASDH",
                              NULL);

  return offset;
}
static int dissect_hf_h235_eckasdhkey(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_ECKASDH(tvb, offset, pinfo, tree, hf_h235_eckasdhkey);
}

static per_sequence_t TypedCertificate_sequence[] = {
  { "type"                        , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_hf_h235_type },
  { "certificate"                 , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_hf_h235_certificatedata },
  { NULL, 0, 0, NULL }
};

static int
dissect_h235_TypedCertificate(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_per_sequence(tvb, offset, pinfo, tree, hf_index,
                                ett_h235_TypedCertificate, TypedCertificate_sequence);

  return offset;
}
static int dissect_hf_h235_certificate(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_TypedCertificate(tvb, offset, pinfo, tree, hf_h235_certificate);
}


static int
dissect_h235_NULL(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  { proto_item *ti_tmp;
  ti_tmp = proto_tree_add_item(tree, hf_index, tvb, offset>>8, 0, FALSE);
  proto_item_append_text(ti_tmp, ": NULL");
  }

  return offset;
}
static int dissect_hf_h235_default(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_NULL(tvb, offset, pinfo, tree, hf_h235_default);
}
static int dissect_hf_h235_radius(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_NULL(tvb, offset, pinfo, tree, hf_h235_radius);
}
static int dissect_hf_h235_dhExch(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_NULL(tvb, offset, pinfo, tree, hf_h235_dhExch);
}
static int dissect_hf_h235_pwdSymEnc(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_NULL(tvb, offset, pinfo, tree, hf_h235_pwdSymEnc);
}
static int dissect_hf_h235_pwdHash(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_NULL(tvb, offset, pinfo, tree, hf_h235_pwdHash);
}
static int dissect_hf_h235_certSign(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_NULL(tvb, offset, pinfo, tree, hf_h235_certSign);
}
static int dissect_hf_h235_ipsec(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_NULL(tvb, offset, pinfo, tree, hf_h235_ipsec);
}
static int dissect_hf_h235_tls(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_NULL(tvb, offset, pinfo, tree, hf_h235_tls);
}


static const value_string AuthenticationBES_vals[] = {
  {   0, "default" },
  {   1, "radius" },
  { 0, NULL }
};

static per_choice_t AuthenticationBES_choice[] = {
  {   0, "default"                     , ASN1_EXTENSION_ROOT    , dissect_hf_h235_default },
  {   1, "radius"                      , ASN1_EXTENSION_ROOT    , dissect_hf_h235_radius },
  { 0, NULL, 0, NULL }
};

static int
dissect_h235_AuthenticationBES(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_per_choice(tvb, offset, pinfo, tree, hf_index,
                              ett_h235_AuthenticationBES, AuthenticationBES_choice, "AuthenticationBES",
                              NULL);

  return offset;
}
static int dissect_hf_h235_authenticationBES(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_AuthenticationBES(tvb, offset, pinfo, tree, hf_h235_authenticationBES);
}


const value_string AuthenticationMechanism_vals[] = {
  {   0, "dhExch" },
  {   1, "pwdSymEnc" },
  {   2, "pwdHash" },
  {   3, "certSign" },
  {   4, "ipsec" },
  {   5, "tls" },
  {   6, "nonStandard" },
  {   7, "authenticationBES" },
  { 0, NULL }
};

static per_choice_t AuthenticationMechanism_choice[] = {
  {   0, "dhExch"                      , ASN1_EXTENSION_ROOT    , dissect_hf_h235_dhExch },
  {   1, "pwdSymEnc"                   , ASN1_EXTENSION_ROOT    , dissect_hf_h235_pwdSymEnc },
  {   2, "pwdHash"                     , ASN1_EXTENSION_ROOT    , dissect_hf_h235_pwdHash },
  {   3, "certSign"                    , ASN1_EXTENSION_ROOT    , dissect_hf_h235_certSign },
  {   4, "ipsec"                       , ASN1_EXTENSION_ROOT    , dissect_hf_h235_ipsec },
  {   5, "tls"                         , ASN1_EXTENSION_ROOT    , dissect_hf_h235_tls },
  {   6, "nonStandard"                 , ASN1_EXTENSION_ROOT    , dissect_hf_h235_nonStandard },
  {   7, "authenticationBES"           , ASN1_NOT_EXTENSION_ROOT, dissect_hf_h235_authenticationBES },
  { 0, NULL, 0, NULL }
};

int
dissect_h235_AuthenticationMechanism(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_per_choice(tvb, offset, pinfo, tree, hf_index,
                              ett_h235_AuthenticationMechanism, AuthenticationMechanism_choice, "AuthenticationMechanism",
                              NULL);

  return offset;
}



static int
dissect_h235_INTEGER(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_per_integer(tvb, offset, pinfo, tree, hf_index,
                               NULL, NULL);

  return offset;
}
static int dissect_hf_h235_ranInt(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_INTEGER(tvb, offset, pinfo, tree, hf_h235_ranInt);
}


static int
dissect_h235_IV8(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_per_octet_string(tvb, offset, pinfo, tree, hf_index,
                                    8, 8,
                                    NULL, NULL);

  return offset;
}
static int dissect_hf_h235_iv8(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_IV8(tvb, offset, pinfo, tree, hf_h235_iv8);
}


static int
dissect_h235_IV16(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_per_octet_string(tvb, offset, pinfo, tree, hf_index,
                                    16, 16,
                                    NULL, NULL);

  return offset;
}
static int dissect_hf_h235_iv16(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_IV16(tvb, offset, pinfo, tree, hf_h235_iv16);
}

static per_sequence_t Params_sequence[] = {
  { "ranInt"                      , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_hf_h235_ranInt },
  { "iv8"                         , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_hf_h235_iv8 },
  { "iv16"                        , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_hf_h235_iv16 },
  { "iv"                          , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_hf_h235_iv },
  { "clearSalt"                   , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_hf_h235_clearSalt },
  { NULL, 0, 0, NULL }
};

static int
dissect_h235_Params(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_per_sequence(tvb, offset, pinfo, tree, hf_index,
                                ett_h235_Params, Params_sequence);

  return offset;
}
static int dissect_hf_h235_paramS(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_Params(tvb, offset, pinfo, tree, hf_h235_paramS);
}
static int dissect_hf_h235_paramSsalt(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_Params(tvb, offset, pinfo, tree, hf_h235_paramSsalt);
}

static per_sequence_t ENCRYPTEDxxx_sequence[] = {
  { "algorithmOID"                , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_hf_h235_algorithmOID },
  { "paramS"                      , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_hf_h235_paramS },
  { "encryptedData"               , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_hf_h235_encryptedData },
  { NULL, 0, 0, NULL }
};

int
dissect_h235_ENCRYPTEDxxx(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  proto_tree_add_item_hidden(tree, proto_h235, tvb, offset, 0, FALSE);
  offset = dissect_per_sequence(tvb, offset, pinfo, tree, hf_index,
                                ett_h235_ENCRYPTEDxxx, ENCRYPTEDxxx_sequence);

  return offset;
}
static int dissect_hf_h235_encryptedToken(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_ENCRYPTEDxxx(tvb, offset, pinfo, tree, hf_h235_encryptedToken);
}
static int dissect_hf_h235_cryptoPwdEncr(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_ENCRYPTEDxxx(tvb, offset, pinfo, tree, hf_h235_cryptoPwdEncr);
}
static int dissect_hf_h235_sharedSecret(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_ENCRYPTEDxxx(tvb, offset, pinfo, tree, hf_h235_sharedSecret);
}


static int
dissect_h235_BIT_STRING(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_per_bit_string(tvb, offset, pinfo, tree, hf_index,
                                  -1, -1);

  return offset;
}
static int dissect_hf_h235_signaturedata(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_BIT_STRING(tvb, offset, pinfo, tree, hf_h235_signaturedata);
}
static int dissect_hf_h235_hash(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_BIT_STRING(tvb, offset, pinfo, tree, hf_h235_hash);
}

static per_sequence_t SIGNEDxxx_sequence[] = {
  { "toBeSigned"                  , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_hf_h235_toBeSigned },
  { "algorithmOID"                , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_hf_h235_algorithmOID },
  { "paramS"                      , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_hf_h235_paramS },
  { "signature"                   , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_hf_h235_signaturedata },
  { NULL, 0, 0, NULL }
};

int
dissect_h235_SIGNEDxxx(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  proto_tree_add_item_hidden(tree, proto_h235, tvb, offset, 0, FALSE);
  offset = dissect_per_sequence(tvb, offset, pinfo, tree, hf_index,
                                ett_h235_SIGNEDxxx, SIGNEDxxx_sequence);

  return offset;
}
static int dissect_hf_h235_signedToken(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_SIGNEDxxx(tvb, offset, pinfo, tree, hf_h235_signedToken);
}
static int dissect_hf_h235_certProtectedKey(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_SIGNEDxxx(tvb, offset, pinfo, tree, hf_h235_certProtectedKey);
}

static per_sequence_t V3KeySyncMaterial_sequence[] = {
  { "generalID"                   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_hf_h235_generalID },
  { "algorithmOID"                , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_hf_h235_algorithmOID },
  { "paramS"                      , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_hf_h235_paramS },
  { "encryptedSessionKey"         , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_hf_h235_encryptedSessionKey },
  { "encryptedSaltingKey"         , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_hf_h235_encryptedSaltingKey },
  { "clearSaltingKey"             , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_hf_h235_clearSaltingKey },
  { "paramSsalt"                  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_hf_h235_paramSsalt },
  { "keyDerivationOID"            , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_hf_h235_keyDerivationOID },
  { NULL, 0, 0, NULL }
};

static int
dissect_h235_V3KeySyncMaterial(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_per_sequence(tvb, offset, pinfo, tree, hf_index,
                                ett_h235_V3KeySyncMaterial, V3KeySyncMaterial_sequence);

  return offset;
}
static int dissect_hf_h235_secureSharedSecret(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_V3KeySyncMaterial(tvb, offset, pinfo, tree, hf_h235_secureSharedSecret);
}


static const value_string H235Key_vals[] = {
  {   0, "secureChannel" },
  {   1, "sharedSecret" },
  {   2, "certProtectedKey" },
  {   3, "secureSharedSecret" },
  { 0, NULL }
};

static per_choice_t H235Key_choice[] = {
  {   0, "secureChannel"               , ASN1_EXTENSION_ROOT    , dissect_hf_h235_secureChannel },
  {   1, "sharedSecret"                , ASN1_EXTENSION_ROOT    , dissect_hf_h235_sharedSecret },
  {   2, "certProtectedKey"            , ASN1_EXTENSION_ROOT    , dissect_hf_h235_certProtectedKey },
  {   3, "secureSharedSecret"          , ASN1_NOT_EXTENSION_ROOT, dissect_hf_h235_secureSharedSecret },
  { 0, NULL, 0, NULL }
};

static int
dissect_h235_H235Key(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_per_choice(tvb, offset, pinfo, tree, hf_index,
                              ett_h235_H235Key, H235Key_choice, "H235Key",
                              NULL);

  return offset;
}
static int dissect_hf_h235_h235Key(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_H235Key(tvb, offset, pinfo, tree, hf_h235_h235Key);
}

static per_sequence_t ClearToken_sequence[] = {
  { "tokenOID"                    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_hf_h235_tokenOID },
  { "timeStamp"                   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_hf_h235_timeStamp },
  { "password"                    , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_hf_h235_password },
  { "dhkey"                       , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_hf_h235_dhkey },
  { "challenge"                   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_hf_h235_challenge },
  { "random"                      , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_hf_h235_random },
  { "certificate"                 , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_hf_h235_certificate },
  { "generalID"                   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_hf_h235_generalID },
  { "nonStandard"                 , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_hf_h235_nonStandard },
  { "eckasdhkey"                  , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_hf_h235_eckasdhkey },
  { "sendersID"                   , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_hf_h235_sendersID },
  { "h235Key"                     , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_hf_h235_h235Key },
  { NULL, 0, 0, NULL }
};

int
dissect_h235_ClearToken(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  proto_tree_add_item_hidden(tree, proto_h235, tvb, offset, 0, FALSE);
  offset = dissect_per_sequence(tvb, offset, pinfo, tree, hf_index,
                                ett_h235_ClearToken, ClearToken_sequence);

  return offset;
}
static int dissect_hf_h235_hashedVals(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_ClearToken(tvb, offset, pinfo, tree, hf_h235_hashedVals);
}

static per_sequence_t HASHEDxxx_sequence[] = {
  { "algorithmOID"                , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_hf_h235_algorithmOID },
  { "paramS"                      , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_hf_h235_paramS },
  { "hash"                        , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_hf_h235_hash },
  { NULL, 0, 0, NULL }
};

int
dissect_h235_HASHEDxxx(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  proto_tree_add_item_hidden(tree, proto_h235, tvb, offset, 0, FALSE);
  offset = dissect_per_sequence(tvb, offset, pinfo, tree, hf_index,
                                ett_h235_HASHEDxxx, HASHEDxxx_sequence);

  return offset;
}
static int dissect_hf_h235_hashedToken(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_HASHEDxxx(tvb, offset, pinfo, tree, hf_h235_hashedToken);
}

static per_sequence_t T_cryptoEncryptedToken_sequence[] = {
  { "tokenOID"                    , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_hf_h235_tokenOID },
  { "token"                       , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_hf_h235_encryptedToken },
  { NULL, 0, 0, NULL }
};

static int
dissect_h235_T_cryptoEncryptedToken(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_per_sequence(tvb, offset, pinfo, tree, hf_index,
                                ett_h235_T_cryptoEncryptedToken, T_cryptoEncryptedToken_sequence);

  return offset;
}
static int dissect_hf_h235_cryptoEncryptedToken(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_T_cryptoEncryptedToken(tvb, offset, pinfo, tree, hf_h235_cryptoEncryptedToken);
}

static per_sequence_t T_cryptoSignedToken_sequence[] = {
  { "tokenOID"                    , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_hf_h235_tokenOID },
  { "token"                       , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_hf_h235_signedToken },
  { NULL, 0, 0, NULL }
};

static int
dissect_h235_T_cryptoSignedToken(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_per_sequence(tvb, offset, pinfo, tree, hf_index,
                                ett_h235_T_cryptoSignedToken, T_cryptoSignedToken_sequence);

  return offset;
}
static int dissect_hf_h235_cryptoSignedToken(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_T_cryptoSignedToken(tvb, offset, pinfo, tree, hf_h235_cryptoSignedToken);
}

static per_sequence_t T_cryptoHashedToken_sequence[] = {
  { "tokenOID"                    , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_hf_h235_tokenOID },
  { "hashedVals"                  , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_hf_h235_hashedVals },
  { "token"                       , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_hf_h235_hashedToken },
  { NULL, 0, 0, NULL }
};

static int
dissect_h235_T_cryptoHashedToken(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  offset = dissect_per_sequence(tvb, offset, pinfo, tree, hf_index,
                                ett_h235_T_cryptoHashedToken, T_cryptoHashedToken_sequence);

  return offset;
}
static int dissect_hf_h235_cryptoHashedToken(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  return dissect_h235_T_cryptoHashedToken(tvb, offset, pinfo, tree, hf_h235_cryptoHashedToken);
}


const value_string CryptoToken_vals[] = {
  {   0, "cryptoEncryptedToken" },
  {   1, "cryptoSignedToken" },
  {   2, "cryptoHashedToken" },
  {   3, "cryptoPwdEncr" },
  { 0, NULL }
};

static per_choice_t CryptoToken_choice[] = {
  {   0, "cryptoEncryptedToken"        , ASN1_EXTENSION_ROOT    , dissect_hf_h235_cryptoEncryptedToken },
  {   1, "cryptoSignedToken"           , ASN1_EXTENSION_ROOT    , dissect_hf_h235_cryptoSignedToken },
  {   2, "cryptoHashedToken"           , ASN1_EXTENSION_ROOT    , dissect_hf_h235_cryptoHashedToken },
  {   3, "cryptoPwdEncr"               , ASN1_EXTENSION_ROOT    , dissect_hf_h235_cryptoPwdEncr },
  { 0, NULL, 0, NULL }
};

int
dissect_h235_CryptoToken(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index) {
  proto_tree_add_item_hidden(tree, proto_h235, tvb, offset, 0, FALSE);
  offset = dissect_per_choice(tvb, offset, pinfo, tree, hf_index,
                              ett_h235_CryptoToken, CryptoToken_choice, "CryptoToken",
                              NULL);

  return offset;
}


/*--- End of included file: packet-h235-fn.c ---*/



/*--- proto_register_h235 ----------------------------------------------*/
void proto_register_h235(void) {

  /* List of fields */
  static hf_register_info hf[] = {

/*--- Included file: packet-h235-hfarr.c ---*/

/* Do not modify this file.                                                   */
/* It is created automatically by the ASN.1 to Ethereal dissector compiler    */
/* packet-h235-hfarr.c                                                        */
/* ../../tools/asn2eth.py -X -p h235 -c h235.cnf -s packet-h235-template H235-SECURITY-MESSAGES.asn */

    { &hf_h235_nonStandardIdentifier,
      { "nonStandardIdentifier", "h235.nonStandardIdentifier",
        FT_STRING, BASE_NONE, NULL, 0,
        "NonStandardParameter/nonStandardIdentifier", HFILL }},
    { &hf_h235_data,
      { "data", "h235.data",
        FT_BYTES, BASE_HEX, NULL, 0,
        "NonStandardParameter/data", HFILL }},
    { &hf_h235_halfkey,
      { "halfkey", "h235.halfkey",
        FT_BYTES, BASE_HEX, NULL, 0,
        "DHset/halfkey", HFILL }},
    { &hf_h235_modSize,
      { "modSize", "h235.modSize",
        FT_BYTES, BASE_HEX, NULL, 0,
        "DHset/modSize", HFILL }},
    { &hf_h235_generator,
      { "generator", "h235.generator",
        FT_BYTES, BASE_HEX, NULL, 0,
        "DHset/generator", HFILL }},
    { &hf_h235_x,
      { "x", "h235.x",
        FT_BYTES, BASE_HEX, NULL, 0,
        "ECpoint/x", HFILL }},
    { &hf_h235_y,
      { "y", "h235.y",
        FT_BYTES, BASE_HEX, NULL, 0,
        "ECpoint/y", HFILL }},
    { &hf_h235_eckasdhp,
      { "eckasdhp", "h235.eckasdhp",
        FT_NONE, BASE_NONE, NULL, 0,
        "ECKASDH/eckasdhp", HFILL }},
    { &hf_h235_public_key,
      { "public-key", "h235.public_key",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_h235_modulus,
      { "modulus", "h235.modulus",
        FT_BYTES, BASE_HEX, NULL, 0,
        "ECKASDH/eckasdhp/modulus", HFILL }},
    { &hf_h235_base,
      { "base", "h235.base",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_h235_weierstrassA,
      { "weierstrassA", "h235.weierstrassA",
        FT_BYTES, BASE_HEX, NULL, 0,
        "", HFILL }},
    { &hf_h235_weierstrassB,
      { "weierstrassB", "h235.weierstrassB",
        FT_BYTES, BASE_HEX, NULL, 0,
        "", HFILL }},
    { &hf_h235_eckasdh2,
      { "eckasdh2", "h235.eckasdh2",
        FT_NONE, BASE_NONE, NULL, 0,
        "ECKASDH/eckasdh2", HFILL }},
    { &hf_h235_fieldSize,
      { "fieldSize", "h235.fieldSize",
        FT_BYTES, BASE_HEX, NULL, 0,
        "ECKASDH/eckasdh2/fieldSize", HFILL }},
    { &hf_h235_type,
      { "type", "h235.type",
        FT_STRING, BASE_NONE, NULL, 0,
        "TypedCertificate/type", HFILL }},
    { &hf_h235_certificatedata,
      { "certificate", "h235.certificate",
        FT_BYTES, BASE_HEX, NULL, 0,
        "TypedCertificate/certificate", HFILL }},
    { &hf_h235_default,
      { "default", "h235.default",
        FT_NONE, BASE_NONE, NULL, 0,
        "AuthenticationBES/default", HFILL }},
    { &hf_h235_radius,
      { "radius", "h235.radius",
        FT_NONE, BASE_NONE, NULL, 0,
        "AuthenticationBES/radius", HFILL }},
    { &hf_h235_dhExch,
      { "dhExch", "h235.dhExch",
        FT_NONE, BASE_NONE, NULL, 0,
        "AuthenticationMechanism/dhExch", HFILL }},
    { &hf_h235_pwdSymEnc,
      { "pwdSymEnc", "h235.pwdSymEnc",
        FT_NONE, BASE_NONE, NULL, 0,
        "AuthenticationMechanism/pwdSymEnc", HFILL }},
    { &hf_h235_pwdHash,
      { "pwdHash", "h235.pwdHash",
        FT_NONE, BASE_NONE, NULL, 0,
        "AuthenticationMechanism/pwdHash", HFILL }},
    { &hf_h235_certSign,
      { "certSign", "h235.certSign",
        FT_NONE, BASE_NONE, NULL, 0,
        "AuthenticationMechanism/certSign", HFILL }},
    { &hf_h235_ipsec,
      { "ipsec", "h235.ipsec",
        FT_NONE, BASE_NONE, NULL, 0,
        "AuthenticationMechanism/ipsec", HFILL }},
    { &hf_h235_tls,
      { "tls", "h235.tls",
        FT_NONE, BASE_NONE, NULL, 0,
        "AuthenticationMechanism/tls", HFILL }},
    { &hf_h235_nonStandard,
      { "nonStandard", "h235.nonStandard",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_h235_authenticationBES,
      { "authenticationBES", "h235.authenticationBES",
        FT_UINT32, BASE_DEC, VALS(AuthenticationBES_vals), 0,
        "AuthenticationMechanism/authenticationBES", HFILL }},
    { &hf_h235_tokenOID,
      { "tokenOID", "h235.tokenOID",
        FT_STRING, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_h235_timeStamp,
      { "timeStamp", "h235.timeStamp",
        FT_UINT32, BASE_DEC, NULL, 0,
        "ClearToken/timeStamp", HFILL }},
    { &hf_h235_password,
      { "password", "h235.password",
        FT_STRING, BASE_NONE, NULL, 0,
        "ClearToken/password", HFILL }},
    { &hf_h235_dhkey,
      { "dhkey", "h235.dhkey",
        FT_NONE, BASE_NONE, NULL, 0,
        "ClearToken/dhkey", HFILL }},
    { &hf_h235_challenge,
      { "challenge", "h235.challenge",
        FT_BYTES, BASE_HEX, NULL, 0,
        "ClearToken/challenge", HFILL }},
    { &hf_h235_random,
      { "random", "h235.random",
        FT_INT32, BASE_DEC, NULL, 0,
        "ClearToken/random", HFILL }},
    { &hf_h235_certificate,
      { "certificate", "h235.certificate",
        FT_NONE, BASE_NONE, NULL, 0,
        "ClearToken/certificate", HFILL }},
    { &hf_h235_generalID,
      { "generalID", "h235.generalID",
        FT_STRING, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_h235_eckasdhkey,
      { "eckasdhkey", "h235.eckasdhkey",
        FT_UINT32, BASE_DEC, VALS(ECKASDH_vals), 0,
        "ClearToken/eckasdhkey", HFILL }},
    { &hf_h235_sendersID,
      { "sendersID", "h235.sendersID",
        FT_STRING, BASE_NONE, NULL, 0,
        "ClearToken/sendersID", HFILL }},
    { &hf_h235_h235Key,
      { "h235Key", "h235.h235Key",
        FT_UINT32, BASE_DEC, VALS(H235Key_vals), 0,
        "ClearToken/h235Key", HFILL }},
    { &hf_h235_toBeSigned,
      { "toBeSigned", "h235.toBeSigned",
        FT_NONE, BASE_NONE, NULL, 0,
        "SIGNEDxxx/toBeSigned", HFILL }},
    { &hf_h235_algorithmOID,
      { "algorithmOID", "h235.algorithmOID",
        FT_STRING, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_h235_paramS,
      { "paramS", "h235.paramS",
        FT_NONE, BASE_NONE, NULL, 0,
        "", HFILL }},
    { &hf_h235_signaturedata,
      { "signature", "h235.signature",
        FT_BYTES, BASE_HEX, NULL, 0,
        "SIGNEDxxx/signature", HFILL }},
    { &hf_h235_encryptedData,
      { "encryptedData", "h235.encryptedData",
        FT_BYTES, BASE_HEX, NULL, 0,
        "ENCRYPTEDxxx/encryptedData", HFILL }},
    { &hf_h235_hash,
      { "hash", "h235.hash",
        FT_BYTES, BASE_HEX, NULL, 0,
        "HASHEDxxx/hash", HFILL }},
    { &hf_h235_ranInt,
      { "ranInt", "h235.ranInt",
        FT_INT32, BASE_DEC, NULL, 0,
        "Params/ranInt", HFILL }},
    { &hf_h235_iv8,
      { "iv8", "h235.iv8",
        FT_BYTES, BASE_HEX, NULL, 0,
        "Params/iv8", HFILL }},
    { &hf_h235_iv16,
      { "iv16", "h235.iv16",
        FT_BYTES, BASE_HEX, NULL, 0,
        "Params/iv16", HFILL }},
    { &hf_h235_iv,
      { "iv", "h235.iv",
        FT_BYTES, BASE_HEX, NULL, 0,
        "Params/iv", HFILL }},
    { &hf_h235_clearSalt,
      { "clearSalt", "h235.clearSalt",
        FT_BYTES, BASE_HEX, NULL, 0,
        "Params/clearSalt", HFILL }},
    { &hf_h235_cryptoEncryptedToken,
      { "cryptoEncryptedToken", "h235.cryptoEncryptedToken",
        FT_NONE, BASE_NONE, NULL, 0,
        "CryptoToken/cryptoEncryptedToken", HFILL }},
    { &hf_h235_encryptedToken,
      { "token", "h235.token",
        FT_NONE, BASE_NONE, NULL, 0,
        "CryptoToken/cryptoEncryptedToken/token", HFILL }},
    { &hf_h235_cryptoSignedToken,
      { "cryptoSignedToken", "h235.cryptoSignedToken",
        FT_NONE, BASE_NONE, NULL, 0,
        "CryptoToken/cryptoSignedToken", HFILL }},
    { &hf_h235_signedToken,
      { "token", "h235.token",
        FT_NONE, BASE_NONE, NULL, 0,
        "CryptoToken/cryptoSignedToken/token", HFILL }},
    { &hf_h235_cryptoHashedToken,
      { "cryptoHashedToken", "h235.cryptoHashedToken",
        FT_NONE, BASE_NONE, NULL, 0,
        "CryptoToken/cryptoHashedToken", HFILL }},
    { &hf_h235_hashedVals,
      { "hashedVals", "h235.hashedVals",
        FT_NONE, BASE_NONE, NULL, 0,
        "CryptoToken/cryptoHashedToken/hashedVals", HFILL }},
    { &hf_h235_hashedToken,
      { "token", "h235.token",
        FT_NONE, BASE_NONE, NULL, 0,
        "CryptoToken/cryptoHashedToken/token", HFILL }},
    { &hf_h235_cryptoPwdEncr,
      { "cryptoPwdEncr", "h235.cryptoPwdEncr",
        FT_NONE, BASE_NONE, NULL, 0,
        "CryptoToken/cryptoPwdEncr", HFILL }},
    { &hf_h235_secureChannel,
      { "secureChannel", "h235.secureChannel",
        FT_BYTES, BASE_HEX, NULL, 0,
        "H235Key/secureChannel", HFILL }},
    { &hf_h235_sharedSecret,
      { "sharedSecret", "h235.sharedSecret",
        FT_NONE, BASE_NONE, NULL, 0,
        "H235Key/sharedSecret", HFILL }},
    { &hf_h235_certProtectedKey,
      { "certProtectedKey", "h235.certProtectedKey",
        FT_NONE, BASE_NONE, NULL, 0,
        "H235Key/certProtectedKey", HFILL }},
    { &hf_h235_secureSharedSecret,
      { "secureSharedSecret", "h235.secureSharedSecret",
        FT_NONE, BASE_NONE, NULL, 0,
        "H235Key/secureSharedSecret", HFILL }},
    { &hf_h235_encryptedSessionKey,
      { "encryptedSessionKey", "h235.encryptedSessionKey",
        FT_BYTES, BASE_HEX, NULL, 0,
        "V3KeySyncMaterial/encryptedSessionKey", HFILL }},
    { &hf_h235_encryptedSaltingKey,
      { "encryptedSaltingKey", "h235.encryptedSaltingKey",
        FT_BYTES, BASE_HEX, NULL, 0,
        "V3KeySyncMaterial/encryptedSaltingKey", HFILL }},
    { &hf_h235_clearSaltingKey,
      { "clearSaltingKey", "h235.clearSaltingKey",
        FT_BYTES, BASE_HEX, NULL, 0,
        "V3KeySyncMaterial/clearSaltingKey", HFILL }},
    { &hf_h235_paramSsalt,
      { "paramSsalt", "h235.paramSsalt",
        FT_NONE, BASE_NONE, NULL, 0,
        "V3KeySyncMaterial/paramSsalt", HFILL }},
    { &hf_h235_keyDerivationOID,
      { "keyDerivationOID", "h235.keyDerivationOID",
        FT_STRING, BASE_NONE, NULL, 0,
        "V3KeySyncMaterial/keyDerivationOID", HFILL }},

/*--- End of included file: packet-h235-hfarr.c ---*/

  };

  /* List of subtrees */
  static gint *ett[] = {

/*--- Included file: packet-h235-ettarr.c ---*/

/* Do not modify this file.                                                   */
/* It is created automatically by the ASN.1 to Ethereal dissector compiler    */
/* packet-h235-ettarr.c                                                       */
/* ../../tools/asn2eth.py -X -p h235 -c h235.cnf -s packet-h235-template H235-SECURITY-MESSAGES.asn */

    &ett_h235_NonStandardParameter,
    &ett_h235_DHset,
    &ett_h235_ECpoint,
    &ett_h235_ECKASDH,
    &ett_h235_T_eckasdhp,
    &ett_h235_T_eckasdh2,
    &ett_h235_TypedCertificate,
    &ett_h235_AuthenticationBES,
    &ett_h235_AuthenticationMechanism,
    &ett_h235_ClearToken,
    &ett_h235_SIGNEDxxx,
    &ett_h235_ENCRYPTEDxxx,
    &ett_h235_HASHEDxxx,
    &ett_h235_Params,
    &ett_h235_CryptoToken,
    &ett_h235_T_cryptoEncryptedToken,
    &ett_h235_T_cryptoSignedToken,
    &ett_h235_T_cryptoHashedToken,
    &ett_h235_H235Key,
    &ett_h235_V3KeySyncMaterial,

/*--- End of included file: packet-h235-ettarr.c ---*/

  };

  /* Register protocol */
  proto_h235 = proto_register_protocol(PNAME, PSNAME, PFNAME);

  /* Register fields and subtrees */
  proto_register_field_array(proto_h235, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));

}


/*--- proto_reg_handoff_h235 -------------------------------------------*/
void proto_reg_handoff_h235(void) {
}

