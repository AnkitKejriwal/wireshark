/* autogenerated by pidl */

/* DO NOT EDIT
	This filter was automatically generated
	from wkssvc.idl and wkssvc.cnf.
	
	Pidl is a perl based IDL compiler for DCE/RPC idl files. 
	It is maintained by the Samba team, not the Wireshark team.
	Instructions on how to download and install Pidl can be 
	found at http://wiki.wireshark.org/Pidl
*/


#ifndef __PACKET_DCERPC_WKSSVC_H
#define __PACKET_DCERPC_WKSSVC_H

#include "packet-dcerpc-srvsvc.h"

int wkssvc_dissect_struct_NetWkstaInfo100(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *parent_tree, guint8 *drep, int hf_index, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo101(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *parent_tree, guint8 *drep, int hf_index, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo102(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *parent_tree, guint8 *drep, int hf_index, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo502(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *parent_tree, guint8 *drep, int hf_index, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo1010(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *parent_tree, guint8 *drep, int hf_index, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo1011(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *parent_tree, guint8 *drep, int hf_index, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo1012(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *parent_tree, guint8 *drep, int hf_index, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo1013(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *parent_tree, guint8 *drep, int hf_index, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo1018(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *parent_tree, guint8 *drep, int hf_index, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo1023(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *parent_tree, guint8 *drep, int hf_index, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo1027(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *parent_tree, guint8 *drep, int hf_index, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo1033(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *parent_tree, guint8 *drep, int hf_index, guint32 param _U_);
int wkssvc_dissect_struct_USER_INFO_0(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *parent_tree, guint8 *drep, int hf_index, guint32 param _U_);
int wkssvc_dissect_struct_USER_INFO_0_CONTAINER(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *parent_tree, guint8 *drep, int hf_index, guint32 param _U_);
int wkssvc_dissect_struct_USER_INFO_1(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *parent_tree, guint8 *drep, int hf_index, guint32 param _U_);
int wkssvc_dissect_struct_USER_INFO_1_CONTAINER(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *parent_tree, guint8 *drep, int hf_index, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaTransportInfo0(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *parent_tree, guint8 *drep, int hf_index, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaTransportCtr0(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *parent_tree, guint8 *drep, int hf_index, guint32 param _U_);
int wkssvc_dissect_struct_PasswordBuffer(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *parent_tree, guint8 *drep, int hf_index, guint32 param _U_);
int wkssvc_dissect_bitmap_joinflags(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep, int hf_index, guint32 param);
int wkssvc_dissect_bitmap_renameflags(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep, int hf_index, guint32 param);
#endif /* __PACKET_DCERPC_WKSSVC_H */
