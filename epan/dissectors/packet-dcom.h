/* packet-dcom.h
 * Routines for DCOM generics
 *
 * $Id$
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

#ifndef __PACKET_DCERPC_DCOM_H
#define __PACKET_DCERPC_DCOM_H


extern const value_string dcom_hresult_vals[];
extern const value_string dcom_variant_type_vals[];

/* preferences */
extern int dcom_prefs_display_unmarshalling_details;


/* the essential DCOM this and that, starting every call */
extern int
dissect_dcom_this(tvbuff_t *tvb, int offset,
	packet_info *pinfo, proto_tree *tree, guint8 *drep);
extern int
dissect_dcom_that(tvbuff_t *tvb, int offset,
	packet_info *pinfo, proto_tree *tree, guint8 *drep);


/* dissection of somewhat more simple data types */
#define dissect_dcom_BOOLEAN		dissect_ndr_uint8
#define dissect_dcom_BYTE			dissect_ndr_uint8
#define dissect_dcom_WORD			dissect_ndr_uint16
#define dissect_dcom_DWORD			dissect_ndr_uint32
#define dissect_dcom_ID				dissect_ndr_duint32
#define dissect_dcom_UUID			dissect_ndr_uuid_t
#define dissect_dcom_FILETIME		dissect_ndr_duint32 /* ToBeDone */
#define dissect_dcom_VARIANT_BOOL	dissect_ndr_uint16
#define dissect_dcom_FLOAT			dissect_ndr_float
#define dissect_dcom_DOUBLE			dissect_ndr_double
#define dissect_dcom_DATE			dissect_ndr_double

extern int
dissect_dcom_append_UUID(tvbuff_t *tvb, int offset,
	packet_info *pinfo, proto_tree *tree, guint8 *drep,
	int hfindex, gchar *field_name, int field_index);
extern gchar* dcom_uuid_to_str(e_uuid_t *uuid);

extern int
dissect_dcom_indexed_WORD(tvbuff_t *tvb, int offset,	packet_info *pinfo,
					 proto_tree *tree, guint8 *drep, 
					 int hfindex, guint16 * pu16WORD, int field_index);

extern int
dissect_dcom_indexed_DWORD(tvbuff_t *tvb, int offset,	packet_info *pinfo,
					 proto_tree *tree, guint8 *drep, 
					 int hfindex, guint32 * pu32DWORD, int field_index);

extern int
dissect_dcom_HRESULT(tvbuff_t *tvb, int offset,
	packet_info *pinfo, proto_tree *tree, guint8 *drep, guint32 * pu32hresult);

extern int
dissect_dcom_indexed_HRESULT(tvbuff_t *tvb, int offset,	packet_info *pinfo,
					 proto_tree *tree, guint8 *drep, 
					 guint32 * pu32hresult, int field_index);

extern int
dissect_dcom_COMVERSION(tvbuff_t *tvb, int offset,
	packet_info *pinfo, proto_tree *tree, guint8 *drep,
	guint16	* pu16version_major, guint16 * pu16version_minor);

extern int
dissect_dcom_LPWSTR(tvbuff_t *tvb, gint offset, packet_info *pinfo,
                       proto_tree *tree, guint8 *drep, int hfindex,
					   gchar *psz_buffer, guint32 u32max_buffer);

extern int
dissect_dcom_indexed_LPWSTR(tvbuff_t *tvb, gint offset, packet_info *pinfo,
                       proto_tree *tree, guint8 *drep, int hfindex,
					   gchar *pszStr, guint32 u32MaxStr, int field_index);

extern int
dissect_dcom_BSTR(tvbuff_t *tvb, gint offset, packet_info *pinfo,
                       proto_tree *tree, guint8 *drep, int hfindex,
					   gchar *psz_buffer, guint32 u32max_buffer);

extern int
dissect_dcom_DUALSTRINGARRAY(tvbuff_t *tvb, gint offset, packet_info *pinfo,
                       proto_tree *tree, guint8 *drep, int hfindex);

extern int
dissect_dcom_STDOBJREF(tvbuff_t *tvb, gint offset, packet_info *pinfo,
                       proto_tree *tree, guint8 *drep, int hfindex);

extern int
dissect_dcom_OBJREF(tvbuff_t *tvb, gint offset, packet_info *pinfo,
                       proto_tree *tree, guint8 *drep, int hfindex);

extern int
dissect_dcom_MInterfacePointer(tvbuff_t *tvb, gint offset, packet_info *pinfo,
                       proto_tree *tree, guint8 *drep, int hfindex);
extern int
dissect_dcom_PMInterfacePointer(tvbuff_t *tvb, gint offset, packet_info *pinfo,
                       proto_tree *tree, guint8 *drep, int hfindex);

extern int
dissect_dcom_VARTYPE(tvbuff_t *tvb, int offset,
	packet_info *pinfo, proto_tree *tree, guint8 *drep,
	guint16 *pu16Vartype);

extern int
dissect_dcom_VARIANT(tvbuff_t *tvb, int offset, packet_info *pinfo, 
					 proto_tree *tree, guint8 *drep, int hfindex);

/* dcom "dcerpc internal" unmarshalling */
extern int
dissect_dcom_dcerpc_array_size(tvbuff_t *tvb, gint offset, packet_info *pinfo,
                       proto_tree *tree, guint8 *drep, guint32 *pu32array_size);

extern int
dissect_dcom_dcerpc_pointer(tvbuff_t *tvb, gint offset, packet_info *pinfo,
                       proto_tree *tree, guint8 *drep, guint32 *pu32pointer);

/* mark things to be done */
extern int
dissect_dcom_tobedone_data(tvbuff_t *tvb, int offset,
	packet_info *pinfo, proto_tree *tree, guint8 *drep, int length);

/* very simple parameter-profiles dissectors (for very simple requests ;-) */
/* request: no parameters */
extern int 
dissect_dcom_simple_rqst(tvbuff_t *tvb, int offset,
	packet_info *pinfo, proto_tree *tree, guint8 *drep);
/* response: only HRESULT */
extern int 
dissect_dcom_simple_resp(tvbuff_t *tvb, int offset,
	packet_info *pinfo, proto_tree *tree, guint8 *drep);

void dcom_register_server_coclass(int proto, int ett,
	e_uuid_t *uuid, guint16 ver, 
	dcerpc_sub_dissector *sub_dissectors, int opnum_hf);

#endif /* packet-dcerpc-dcom.h */
