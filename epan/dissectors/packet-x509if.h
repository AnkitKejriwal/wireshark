/* Do not modify this file.                                                   */
/* It is created automatically by the ASN.1 to Ethereal dissector compiler    */
/* ./packet-x509if.h                                                          */
/* ../../tools/asn2eth.py -X -b -e -p x509if -c x509if.cnf -s packet-x509if-template InformationFramework.asn */

/* Input file: packet-x509if-template.h */

/* packet-x509if.h
 * Routines for X.509 Information Framework packet dissection
 *
 * $Id: packet-x509if-template.h 12433 2004-10-29 11:39:58Z sahlberg $
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

#ifndef PACKET_X509IF_H
#define PACKET_X509IF_H


/*--- Included file: packet-x509if-exp.h ---*/

extern const value_string Name_vals[];
int dissect_x509if_Attribute(gboolean implicit_tag, tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);
int dissect_x509if_AttributeType(gboolean implicit_tag, tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);
int dissect_x509if_Name(gboolean implicit_tag, tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);
int dissect_x509if_DistinguishedName(gboolean implicit_tag, tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);
int dissect_x509if_RelativeDistinguishedName(gboolean implicit_tag, tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_index);

/*--- End of included file: packet-x509if-exp.h ---*/


#endif  /* PACKET_X509IF_H */

