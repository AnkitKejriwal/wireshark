/* autogenerated by pidl */

/* DO NOT EDIT
	This filter was automatically generated
	from rfr.idl and rfr.cnf.
	
	Pidl is a perl based IDL compiler for DCE/RPC idl files. 
	It is maintained by the Samba team, not the Wireshark team.
	Instructions on how to download and install Pidl can be 
	found at http://wiki.wireshark.org/Pidl
*/


#ifndef __PACKET_DCERPC_RFR_H
#define __PACKET_DCERPC_RFR_H

#define MAPI_E_SUCCESS (0x00000000)
#define MAPI_E_NO_SUPPORT (0x80040102)
#define MAPI_E_BAD_CHARWIDTH (0x80040103)
#define MAPI_E_STRING_TOO_LONG (0x80040105)
#define MAPI_E_UNKNOWN_FLAGS (0x80040106)
#define MAPI_E_INVALID_ENTRYID (0x80040107)
#define MAPI_E_INVALID_OBJECT (0x80040108)
#define MAPI_E_OBJECT_CHANGED (0x80040109)
#define MAPI_E_OBJECT_DELETED (0x8004010A)
#define MAPI_E_BUSY (0x8004010B)
#define MAPI_E_NOT_ENOUGH_DISK (0x8004010D)
#define MAPI_E_NOT_ENOUGH_RESOURCES (0x8004010E)
#define MAPI_E_NOT_FOUND (0x8004010F)
#define MAPI_E_VERSION (0x80040110)
#define MAPI_E_LOGON_FAILED (0x80040111)
#define MAPI_E_SESSION_LIMIT (0x80040112)
#define MAPI_E_USER_CANCEL (0x80040113)
#define MAPI_E_UNABLE_TO_ABORT (0x80040114)
#define MAPI_E_NETWORK_ERROR (0x80040115)
#define MAPI_E_DISK_ERROR (0x80040116)
#define MAPI_E_TOO_COMPLEX (0x80040117)
#define MAPI_E_BAD_COLUMN (0x80040118)
#define MAPI_E_EXTENDED_ERROR (0x80040119)
#define MAPI_E_COMPUTED (0x8004011A)
#define MAPI_E_CORRUPT_DATA (0x8004011B)
#define MAPI_E_UNCONFIGURED (0x8004011C)
#define MAPI_E_FAILONEPROVIDER (0x8004011D)
#define MAPI_E_UNKNOWN_CPID (0x8004011E)
#define MAPI_E_UNKNOWN_LCID (0x8004011F)
#define MAPI_E_PASSWORD_CHANGE_REQUIRED (0x80040120)
#define MAPI_E_PASSWORD_EXPIRED (0x80040121)
#define MAPI_E_INVALID_WORKSTATION_ACCOUNT (0x80040122)
#define MAPI_E_INVALID_ACCESS_TIME (0x80040123)
#define MAPI_E_ACCOUNT_DISABLED (0x80040124)
#define MAPI_E_END_OF_SESSION (0x80040200)
#define MAPI_E_UNKNOWN_ENTRYID (0x80040201)
#define MAPI_E_MISSING_REQUIRED_COLUMN (0x80040202)
#define MAPI_W_NO_SERVICE (0x80040203)
#define MAPI_E_BAD_VALUE (0x80040301)
#define MAPI_E_INVALID_TYPE (0x80040302)
#define MAPI_E_TYPE_NO_SUPPORT (0x80040303)
#define MAPI_E_UNEXPECTED_TYPE (0x80040304)
#define MAPI_E_TOO_BIG (0x80040305)
#define MAPI_E_DECLINE_COPY (0x80040306)
#define MAPI_E_UNEXPECTED_ID (0x80040307)
#define MAPI_W_ERRORS_RETURNED (0x80040380)
#define MAPI_E_UNABLE_TO_COMPLETE (0x80040400)
#define MAPI_E_TIMEOUT (0x80040401)
#define MAPI_E_TABLE_EMPTY (0x80040402)
#define MAPI_E_TABLE_TOO_BIG (0x80040403)
#define MAPI_E_INVALID_BOOKMARK (0x80040405)
#define MAPI_W_POSITION_CHANGED (0x80040481)
#define MAPI_W_APPROX_COUNT (0x80040482)
#define MAPI_E_WAIT (0x80040500)
#define MAPI_E_CANCEL (0x80040501)
#define MAPI_E_NOT_ME (0x80040502)
#define MAPI_W_CANCEL_MESSAGE (0x80040580)
#define MAPI_E_CORRUPT_STORE (0x80040600)
#define MAPI_E_NOT_IN_QUEUE (0x80040601)
#define MAPI_E_NO_SUPPRESS (0x80040602)
#define MAPI_E_COLLISION (0x80040604)
#define MAPI_E_NOT_INITIALIZED (0x80040605)
#define MAPI_E_NON_STANDARD (0x80040606)
#define MAPI_E_NO_RECIPIENTS (0x80040607)
#define MAPI_E_SUBMITTED (0x80040608)
#define MAPI_E_HAS_FOLDERS (0x80040609)
#define MAPI_E_HAS_MESAGES (0x8004060A)
#define MAPI_E_FOLDER_CYCLE (0x8004060B)
#define MAPI_W_PARTIAL_COMPLETION (0x80040680)
#define MAPI_E_AMBIGUOUS_RECIP (0x80040700)
#define MAPI_E_RESERVED (0xFFFFFFFF)
extern const value_string rfr_MAPISTATUS_vals[];
int rfr_dissect_enum_MAPISTATUS(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 *param _U_);
#endif /* __PACKET_DCERPC_RFR_H */
