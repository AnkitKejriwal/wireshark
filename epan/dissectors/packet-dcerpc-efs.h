/* autogenerated by pidl */

/* DO NOT EDIT
	This filter was automatically generated
	from efs.idl and efs.cnf.
	
	Pidl is a perl based IDL compiler for DCE/RPC idl files. 
	It is maintained by the Samba team, not the Wireshark team.
	Instructions on how to download and install Pidl can be 
	found at http://wiki.wireshark.org/Pidl

	$Id$
*/


#ifndef __PACKET_DCERPC_EFS_H
#define __PACKET_DCERPC_EFS_H

int efs_dissect_struct_EFS_HASH_BLOB(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int efs_dissect_struct_ENCRYPTION_CERTIFICATE_HASH(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int efs_dissect_struct_ENCRYPTION_CERTIFICATE_HASH_LIST(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int efs_dissect_struct_EFS_CERTIFICATE_BLOB(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int efs_dissect_struct_ENCRYPTION_CERTIFICATE(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
#endif /* __PACKET_DCERPC_EFS_H */
