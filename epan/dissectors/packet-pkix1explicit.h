/* Do not modify this file.                                                   */
/* It is created automatically by the ASN.1 to Ethereal dissector compiler    */
/* ./packet-pkix1explicit.h                                                   */
/* ../../tools/asn2eth.py -e -X -b -p pkix1explicit -c pkix1explicit.cnf -s packet-pkix1explicit-template PKIX1EXPLICIT93.asn */

/* Input file: packet-pkix1explicit-template.h */

/* packet-pkix1explicit.h
 * Routines for PKIX1Explicit packet dissection
 *
 * $Id: packet-pkix1explicit-template.h 12203 2004-10-05 09:18:55Z guy $
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

#ifndef PACKET_PKIX1EXPLICIT_H
#define PACKET_PKIX1EXPLICIT_H

int dissect_pkix1explicit_CertificateSerialNumber(gboolean implicit_tag, tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);
int dissect_pkix1explicit_Name(gboolean implicit_tag, tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);
int dissect_pkix1explicit_AlgorithmIdentifier(gboolean implicit_tag, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index);
int dissect_pkix1explicit_SubjectPublicKeyInfo(gboolean implicit_tag, tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);



/*--- Included file: packet-pkix1explicit-exp.h ---*/

extern const value_string TerminalType_vals[];
int dissect_pkix1explicit_DirectoryString(gboolean implicit_tag, tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);
int dissect_pkix1explicit_TerminalType(gboolean implicit_tag, tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);
int dissect_pkix1explicit_TeletexDomainDefinedAttribute(gboolean implicit_tag, tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);

/*--- End of included file: packet-pkix1explicit-exp.h ---*/


#endif  /* PACKET_PKIX1EXPLICIT_H */

