/* Do not modify this file.                                                   */
/* It is created automatically by the ASN.1 to Ethereal dissector compiler    */
/* packet-h235.h                                                              */
/* ../../tools/asn2eth.py -X -p h235 -c h235.cnf -s packet-h235-template H235-SECURITY-MESSAGES.asn */

/* Input file: packet-h235-template.h */
/* Include files: packet-h235-exp.h, packet-h235-valexp.h */

/* packet-h235.h
 * Routines for H.235 packet dissection
 * 2004  Tomas Kukosa
 *
 * $Id: packet-h235.h,v 1.6 2004/06/24 21:50:04 sahlberg Exp $
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

#ifndef PACKET_H235_H
#define PACKET_H235_H


/*--- Included file: packet-h235-exp.h ---*/

/* Do not modify this file.                                                   */
/* It is created automatically by the ASN.1 to Ethereal dissector compiler    */
/* packet-h235-exp.h                                                          */
/* ../../tools/asn2eth.py -X -p h235 -c h235.cnf -s packet-h235-template H235-SECURITY-MESSAGES.asn */

extern const value_string AuthenticationMechanism_vals[];
extern const value_string CryptoToken_vals[];
int dissect_h235_TimeStamp(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);
int dissect_h235_AuthenticationMechanism(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);
int dissect_h235_ClearToken(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);
int dissect_h235_SIGNEDxxx(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);
int dissect_h235_ENCRYPTEDxxx(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);
int dissect_h235_HASHEDxxx(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);
int dissect_h235_CryptoToken(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);

/*--- End of included file: packet-h235-exp.h ---*/


#endif  /* PACKET_H235_H */

