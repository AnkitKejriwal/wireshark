/* autogenerated by pidl */

/* DO NOT EDIT
	This filter was automatically generated
	from wkssvc.idl and wkssvc.cnf.
	
	Pidl is a perl based IDL compiler for DCE/RPC idl files. 
	It is maintained by the Samba team, not the Wireshark team.
	Instructions on how to download and install Pidl can be 
	found at http://wiki.wireshark.org/Pidl
*/


#include "packet-dcerpc-srvsvc.h"
#include "packet-dcerpc-lsa.h"

#ifndef __PACKET_DCERPC_WKSSVC_H
#define __PACKET_DCERPC_WKSSVC_H

int wkssvc_dissect_struct_lsa_String(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo100(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo101(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo102(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo502(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo1010(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo1011(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo1012(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo1013(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo1018(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo1023(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo1027(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo1028(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo1032(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo1033(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo1041(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo1042(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo1043(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo1044(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo1045(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo1046(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo1047(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo1048(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo1049(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo1050(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo1051(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo1052(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo1053(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo1054(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo1055(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo1056(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo1057(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo1058(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo1059(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo1060(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo1061(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaInfo1062(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetrWkstaUserInfo0(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaEnumUsersCtr0(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetrWkstaUserInfo1(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaEnumUsersCtr1(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaEnumUsersInfo(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetrWkstaUserInfo1101(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaTransportInfo0(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaTransportCtr0(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetWkstaTransportInfo(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetrUseInfo3(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetrUseInfo2(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetrUseInfo1(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetrUseInfo0(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetrUseEnumCtr2(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetrUseEnumCtr1(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetrUseEnumCtr0(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetrUseEnumInfo(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_struct_NetrWorkstationStatistics(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_bitmap_renameflags(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
#define NetSetupUnknown (0)
#define NetSetupMachine (1)
#define NetSetupWorkgroup (2)
#define NetSetupDomain (3)
#define NetSetupNonExistentDomain (4)
#define NetSetupDnsMachine (5)
extern const value_string wkssvc_wkssvc_NetValidateNameType_vals[];
int wkssvc_dissect_enum_NetValidateNameType(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 *param _U_);
#define NetSetupUnknownStatus (0)
#define NetSetupUnjoined (1)
#define NetSetupWorkgroupName (2)
#define NetSetupDomainName (3)
extern const value_string wkssvc_wkssvc_NetJoinStatus_vals[];
int wkssvc_dissect_enum_NetJoinStatus(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 *param _U_);
int wkssvc_dissect_struct_PasswordBuffer(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
int wkssvc_dissect_bitmap_joinflags(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
#define NetPrimaryComputerName (0)
#define NetAlternateComputerNames (1)
#define NetAllComputerNames (2)
#define NetComputerNameTypeMax (3)
extern const value_string wkssvc_wkssvc_ComputerNameType_vals[];
int wkssvc_dissect_enum_ComputerNameType(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 *param _U_);
int wkssvc_dissect_struct_ComputerNamesCtr(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *parent_tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 param _U_);
#endif /* __PACKET_DCERPC_WKSSVC_H */
