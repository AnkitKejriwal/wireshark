/* autogenerated by pidl */

/* DO NOT EDIT
	This filter was automatically generated
	from dssetup.idl and dssetup.cnf.
	
	Pidl is a perl based IDL compiler for DCE/RPC idl files. 
	It is maintained by the Samba team, not the Wireshark team.
	Instructions on how to download and install Pidl can be 
	found at http://wiki.wireshark.org/Pidl
*/


#ifndef __PACKET_DCERPC_DSSETUP_H
#define __PACKET_DCERPC_DSSETUP_H

#define DS_ROLE_STANDALONE_WORKSTATION (0)
#define DS_ROLE_MEMBER_WORKSTATION (1)
#define DS_ROLE_STANDALONE_SERVER (2)
#define DS_ROLE_MEMBER_SERVER (3)
#define DS_ROLE_BACKUP_DC (4)
#define DS_ROLE_PRIMARY_DC (5)
extern const value_string dssetup_dssetup_DsRole_vals[];
int dssetup_dissect_enum_DsRole(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep, int hf_index, guint32 param);
int dssetup_dissect_bitmap_DsRoleFlags(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep, int hf_index, guint32 param);
int dssetup_dissect_struct_DsRolePrimaryDomInfoBasic(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *parent_tree, guint8 *drep, int hf_index, guint32 param _U_);
#define DS_ROLE_NOT_UPGRADING (0)
#define DS_ROLE_UPGRADING (1)
extern const value_string dssetup_dssetup_DsUpgrade_vals[];
int dssetup_dissect_enum_DsUpgrade(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep, int hf_index, guint32 param);
#define DS_ROLE_PREVIOUS_UNKNOWN (0)
#define DS_ROLE_PREVIOUS_PRIMARY (1)
#define DS_ROLE_PREVIOUS_BACKUP (2)
extern const value_string dssetup_dssetup_DsPrevious_vals[];
int dssetup_dissect_enum_DsPrevious(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep, int hf_index, guint32 param);
int dssetup_dissect_struct_DsRoleUpgradeStatus(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *parent_tree, guint8 *drep, int hf_index, guint32 param _U_);
#define DS_ROLE_OP_IDLE (0)
#define DS_ROLE_OP_ACTIVE (1)
#define DS_ROLE_OP_NEEDS_REBOOT (2)
extern const value_string dssetup_dssetup_DsRoleOp_vals[];
int dssetup_dissect_enum_DsRoleOp(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep, int hf_index, guint32 param);
int dssetup_dissect_struct_DsRoleOpStatus(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *parent_tree, guint8 *drep, int hf_index, guint32 param _U_);
#define DS_ROLE_BASIC_INFORMATION (1)
#define DS_ROLE_UPGRADE_STATUS (2)
#define DS_ROLE_OP_STATUS (3)
extern const value_string dssetup_dssetup_DsRoleInfoLevel_vals[];
int dssetup_dissect_enum_DsRoleInfoLevel(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep, int hf_index, guint32 param);
#endif /* __PACKET_DCERPC_DSSETUP_H */
