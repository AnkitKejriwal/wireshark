/* Do not modify this file.                                                   */
/* It is created automatically by the ASN.1 to Ethereal dissector compiler    */
/* ./packet-crmf.h                                                            */
/* ../../tools/asn2eth.py -X -b -e -p crmf -c crmf.cnf -s packet-crmf-template CRMF.asn */

/* Input file: packet-crmf-template.h */

/* packet-crmf.h
 * Routines for RFC2511 Certificate Request Message Format packet dissection
 *   Ronnie Sahlberg 2004
 *
 * $Id: packet-crmf-template.h 12573 2004-11-22 03:36:26Z sahlberg $
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

#ifndef PACKET_CRMF_H
#define PACKET_CRMF_H


/*--- Included file: packet-crmf-exp.h ---*/

extern const value_string crmf_ProofOfPossession_vals[];
extern const value_string crmf_POPOPrivKey_vals[];
extern const value_string crmf_SubsequentMessage_vals[];
extern const value_string crmf_PKIArchiveOptions_vals[];
extern const value_string crmf_EncryptedKey_vals[];
int dissect_crmf_CertReqMessages(gboolean implicit_tag, tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);
int dissect_crmf_CertReqMsg(gboolean implicit_tag, tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);
int dissect_crmf_CertRequest(gboolean implicit_tag, tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);
int dissect_crmf_CertTemplate(gboolean implicit_tag, tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);
int dissect_crmf_OptionalValidity(gboolean implicit_tag, tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);
int dissect_crmf_Controls(gboolean implicit_tag, tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);
int dissect_crmf_AttributeTypeAndValue(gboolean implicit_tag, tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);
int dissect_crmf_ProofOfPossession(gboolean implicit_tag, tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);
int dissect_crmf_POPOSigningKey(gboolean implicit_tag, tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);
int dissect_crmf_POPOSigningKeyInput(gboolean implicit_tag, tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);
int dissect_crmf_PKMACValue(gboolean implicit_tag, tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);
int dissect_crmf_PBMParameter(gboolean implicit_tag, tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);
int dissect_crmf_POPOPrivKey(gboolean implicit_tag, tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);
int dissect_crmf_SubsequentMessage(gboolean implicit_tag, tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);
int dissect_crmf_RegToken(gboolean implicit_tag, tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);
int dissect_crmf_Authenticator(gboolean implicit_tag, tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);
int dissect_crmf_PKIPublicationInfo(gboolean implicit_tag, tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);
int dissect_crmf_SinglePubInfo(gboolean implicit_tag, tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);
int dissect_crmf_PKIArchiveOptions(gboolean implicit_tag, tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);
int dissect_crmf_EncryptedKey(gboolean implicit_tag, tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);
int dissect_crmf_EncryptedValue(gboolean implicit_tag, tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);
int dissect_crmf_KeyGenParameters(gboolean implicit_tag, tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);
int dissect_crmf_OldCertId(gboolean implicit_tag, tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);
int dissect_crmf_CertId(gboolean implicit_tag, tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);
int dissect_crmf_ProtocolEncrKey(gboolean implicit_tag, tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);
int dissect_crmf_UTF8Pairs(gboolean implicit_tag, tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);
int dissect_crmf_CertReq(gboolean implicit_tag, tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);

/*--- End of included file: packet-crmf-exp.h ---*/


#endif  /* PACKET_CRMF_H */

