/* autogenerated by pidl */

/* DO NOT EDIT
	This filter was automatically generated
	from winreg.idl and winreg.cnf.
	
	Pidl is a perl based IDL compiler for DCE/RPC idl files. 
	It is maintained by the Samba team, not the Ethereal team.
	Instructions on how to download and install Pidl can be 
	found at http://wiki.ethereal.com/Pidl
*/


#ifndef __PACKET_DCERPC_WINREG_H
#define __PACKET_DCERPC_WINREG_H

#include "packet-dcerpc-lsa.h"

#include "packet-dcerpc-initshutdown.h"

#define REG_NONE (0)
#define REG_SZ (1)
#define REG_EXPAND_SZ (2)
#define REG_BINARY (3)
#define REG_DWORD (4)
#define REG_DWORD_BIG_ENDIAN (5)
#define REG_LINK (6)
#define REG_MULTI_SZ (7)
#define REG_RESOURCE_LIST (8)
#define REG_FULL_RESOURCE_DESCRIPTOR (9)
#define REG_RESOURCE_REQUIREMENTS_LIST (10)
#define REG_QWORD (11)
extern const value_string winreg_winreg_Type_vals[];
int winreg_dissect_enum_Type(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep, int hf_index, guint32 param);
int winreg_dissect_struct_String(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *parent_tree, guint8 *drep, int hf_index, guint32 param _U_);
int winreg_dissect_struct_KeySecurityData(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *parent_tree, guint8 *drep, int hf_index, guint32 param _U_);
int winreg_dissect_struct_SecBuf(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *parent_tree, guint8 *drep, int hf_index, guint32 param _U_);
#define REG_ACTION_NONE (0)
#define REG_CREATED_NEW_KEY (1)
#define REG_OPENED_EXISTING_KEY (2)
extern const value_string winreg_winreg_CreateAction_vals[];
int winreg_dissect_enum_CreateAction(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep, int hf_index, guint32 param);
int winreg_dissect_struct_StringBuf(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *parent_tree, guint8 *drep, int hf_index, guint32 param _U_);
int winreg_dissect_struct_QueryMultipleValue(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *parent_tree, guint8 *drep, int hf_index, guint32 param _U_);
#endif /* __PACKET_DCERPC_WINREG_H */
