/* autogenerated by pidl */

/* DO NOT EDIT
	This filter was automatically generated
	from atsvc.idl and atsvc.cnf.
	
	Pidl is a perl based IDL compiler for DCE/RPC idl files. 
	It is maintained by the Samba team, not the Ethereal team.
	Instructions on how to download and install Pidl can be 
	found at http://wiki.ethereal.com/Pidl
*/


#ifndef __PACKET_DCERPC_ATSVC_H
#define __PACKET_DCERPC_ATSVC_H

int atsvc_dissect_bitmap_DaysOfMonth(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep, int hf_index, guint32 param);
int atsvc_dissect_bitmap_Flags(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep, int hf_index, guint32 param);
int atsvc_dissect_bitmap_DaysOfWeek(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep, int hf_index, guint32 param);
int atsvc_dissect_struct_JobInfo(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *parent_tree, guint8 *drep, int hf_index, guint32 param _U_);
int atsvc_dissect_struct_JobEnumInfo(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *parent_tree, guint8 *drep, int hf_index, guint32 param _U_);
int atsvc_dissect_struct_enum_ctr(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *parent_tree, guint8 *drep, int hf_index, guint32 param _U_);
#endif /* __PACKET_DCERPC_ATSVC_H */
