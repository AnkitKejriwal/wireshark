/* DO NOT EDIT
	This filter was automatically generated
	from rfr.idl and rfr.cnf.
	
	Pidl is a perl based IDL compiler for DCE/RPC idl files. 
	It is maintained by the Samba team, not the Wireshark team.
	Instructions on how to download and install Pidl can be 
	found at http://wiki.wireshark.org/Pidl
*/


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef _MSC_VER
#pragma warning(disable:4005)
#pragma warning(disable:4013)
#pragma warning(disable:4018)
#pragma warning(disable:4101)
#endif

#include <glib.h>
#include <string.h>
#include <epan/packet.h>

#include "packet-dcerpc.h"
#include "packet-dcerpc-nt.h"
#include "packet-windows-common.h"
#include "packet-dcerpc-rfr.h"

/* Ett declarations */
static gint ett_dcerpc_rfr = -1;


/* Header field declarations */
static gint hf_rfr_MAPISTATUS_status = -1;
static gint hf_rfr_RfrGetFQDNFromLegacyDN_ulFlags = -1;
static gint hf_rfr_RfrGetFQDNFromLegacyDN_szMailboxServerDN = -1;
static gint hf_rfr_opnum = -1;
static gint hf_rfr_RfrGetNewDSA_ulFlags = -1;
static gint hf_rfr_RfrGetFQDNFromLegacyDN_cbMailboxServerDN = -1;
static gint hf_rfr_RfrGetNewDSA_pUserDN = -1;
static gint hf_rfr_RfrGetNewDSA_ppszUnused = -1;
static gint hf_rfr_RfrGetNewDSA_ppszServer = -1;
static gint hf_rfr_RfrGetFQDNFromLegacyDN_ppszServerFQDN = -1;

static gint proto_dcerpc_rfr = -1;
/* Version information */


static e_uuid_t uuid_dcerpc_rfr = {
	0x1544f5e0, 0x613c, 0x11d1,
	{ 0x93, 0xdf, 0x00, 0xc0, 0x4f, 0xd7, 0xbd, 0x09 }
};
static guint16 ver_dcerpc_rfr = 1;

const value_string rfr_MAPISTATUS_vals[] = {
	{ MAPI_E_SUCCESS, "MAPI_E_SUCCESS" },
	{ MAPI_E_NO_SUPPORT, "MAPI_E_NO_SUPPORT" },
	{ MAPI_E_BAD_CHARWIDTH, "MAPI_E_BAD_CHARWIDTH" },
	{ MAPI_E_STRING_TOO_LONG, "MAPI_E_STRING_TOO_LONG" },
	{ MAPI_E_UNKNOWN_FLAGS, "MAPI_E_UNKNOWN_FLAGS" },
	{ MAPI_E_INVALID_ENTRYID, "MAPI_E_INVALID_ENTRYID" },
	{ MAPI_E_INVALID_OBJECT, "MAPI_E_INVALID_OBJECT" },
	{ MAPI_E_OBJECT_CHANGED, "MAPI_E_OBJECT_CHANGED" },
	{ MAPI_E_OBJECT_DELETED, "MAPI_E_OBJECT_DELETED" },
	{ MAPI_E_BUSY, "MAPI_E_BUSY" },
	{ MAPI_E_NOT_ENOUGH_DISK, "MAPI_E_NOT_ENOUGH_DISK" },
	{ MAPI_E_NOT_ENOUGH_RESOURCES, "MAPI_E_NOT_ENOUGH_RESOURCES" },
	{ MAPI_E_NOT_FOUND, "MAPI_E_NOT_FOUND" },
	{ MAPI_E_VERSION, "MAPI_E_VERSION" },
	{ MAPI_E_LOGON_FAILED, "MAPI_E_LOGON_FAILED" },
	{ MAPI_E_SESSION_LIMIT, "MAPI_E_SESSION_LIMIT" },
	{ MAPI_E_USER_CANCEL, "MAPI_E_USER_CANCEL" },
	{ MAPI_E_UNABLE_TO_ABORT, "MAPI_E_UNABLE_TO_ABORT" },
	{ MAPI_E_NETWORK_ERROR, "MAPI_E_NETWORK_ERROR" },
	{ MAPI_E_DISK_ERROR, "MAPI_E_DISK_ERROR" },
	{ MAPI_E_TOO_COMPLEX, "MAPI_E_TOO_COMPLEX" },
	{ MAPI_E_BAD_COLUMN, "MAPI_E_BAD_COLUMN" },
	{ MAPI_E_EXTENDED_ERROR, "MAPI_E_EXTENDED_ERROR" },
	{ MAPI_E_COMPUTED, "MAPI_E_COMPUTED" },
	{ MAPI_E_CORRUPT_DATA, "MAPI_E_CORRUPT_DATA" },
	{ MAPI_E_UNCONFIGURED, "MAPI_E_UNCONFIGURED" },
	{ MAPI_E_FAILONEPROVIDER, "MAPI_E_FAILONEPROVIDER" },
	{ MAPI_E_UNKNOWN_CPID, "MAPI_E_UNKNOWN_CPID" },
	{ MAPI_E_UNKNOWN_LCID, "MAPI_E_UNKNOWN_LCID" },
	{ MAPI_E_PASSWORD_CHANGE_REQUIRED, "MAPI_E_PASSWORD_CHANGE_REQUIRED" },
	{ MAPI_E_PASSWORD_EXPIRED, "MAPI_E_PASSWORD_EXPIRED" },
	{ MAPI_E_INVALID_WORKSTATION_ACCOUNT, "MAPI_E_INVALID_WORKSTATION_ACCOUNT" },
	{ MAPI_E_INVALID_ACCESS_TIME, "MAPI_E_INVALID_ACCESS_TIME" },
	{ MAPI_E_ACCOUNT_DISABLED, "MAPI_E_ACCOUNT_DISABLED" },
	{ MAPI_E_END_OF_SESSION, "MAPI_E_END_OF_SESSION" },
	{ MAPI_E_UNKNOWN_ENTRYID, "MAPI_E_UNKNOWN_ENTRYID" },
	{ MAPI_E_MISSING_REQUIRED_COLUMN, "MAPI_E_MISSING_REQUIRED_COLUMN" },
	{ MAPI_W_NO_SERVICE, "MAPI_W_NO_SERVICE" },
	{ MAPI_E_BAD_VALUE, "MAPI_E_BAD_VALUE" },
	{ MAPI_E_INVALID_TYPE, "MAPI_E_INVALID_TYPE" },
	{ MAPI_E_TYPE_NO_SUPPORT, "MAPI_E_TYPE_NO_SUPPORT" },
	{ MAPI_E_UNEXPECTED_TYPE, "MAPI_E_UNEXPECTED_TYPE" },
	{ MAPI_E_TOO_BIG, "MAPI_E_TOO_BIG" },
	{ MAPI_E_DECLINE_COPY, "MAPI_E_DECLINE_COPY" },
	{ MAPI_E_UNEXPECTED_ID, "MAPI_E_UNEXPECTED_ID" },
	{ MAPI_W_ERRORS_RETURNED, "MAPI_W_ERRORS_RETURNED" },
	{ MAPI_E_UNABLE_TO_COMPLETE, "MAPI_E_UNABLE_TO_COMPLETE" },
	{ MAPI_E_TIMEOUT, "MAPI_E_TIMEOUT" },
	{ MAPI_E_TABLE_EMPTY, "MAPI_E_TABLE_EMPTY" },
	{ MAPI_E_TABLE_TOO_BIG, "MAPI_E_TABLE_TOO_BIG" },
	{ MAPI_E_INVALID_BOOKMARK, "MAPI_E_INVALID_BOOKMARK" },
	{ MAPI_W_POSITION_CHANGED, "MAPI_W_POSITION_CHANGED" },
	{ MAPI_W_APPROX_COUNT, "MAPI_W_APPROX_COUNT" },
	{ MAPI_E_WAIT, "MAPI_E_WAIT" },
	{ MAPI_E_CANCEL, "MAPI_E_CANCEL" },
	{ MAPI_E_NOT_ME, "MAPI_E_NOT_ME" },
	{ MAPI_W_CANCEL_MESSAGE, "MAPI_W_CANCEL_MESSAGE" },
	{ MAPI_E_CORRUPT_STORE, "MAPI_E_CORRUPT_STORE" },
	{ MAPI_E_NOT_IN_QUEUE, "MAPI_E_NOT_IN_QUEUE" },
	{ MAPI_E_NO_SUPPRESS, "MAPI_E_NO_SUPPRESS" },
	{ MAPI_E_COLLISION, "MAPI_E_COLLISION" },
	{ MAPI_E_NOT_INITIALIZED, "MAPI_E_NOT_INITIALIZED" },
	{ MAPI_E_NON_STANDARD, "MAPI_E_NON_STANDARD" },
	{ MAPI_E_NO_RECIPIENTS, "MAPI_E_NO_RECIPIENTS" },
	{ MAPI_E_SUBMITTED, "MAPI_E_SUBMITTED" },
	{ MAPI_E_HAS_FOLDERS, "MAPI_E_HAS_FOLDERS" },
	{ MAPI_E_HAS_MESAGES, "MAPI_E_HAS_MESAGES" },
	{ MAPI_E_FOLDER_CYCLE, "MAPI_E_FOLDER_CYCLE" },
	{ MAPI_W_PARTIAL_COMPLETION, "MAPI_W_PARTIAL_COMPLETION" },
	{ MAPI_E_AMBIGUOUS_RECIP, "MAPI_E_AMBIGUOUS_RECIP" },
	{ MAPI_E_RESERVED, "MAPI_E_RESERVED" },
{ 0, NULL }
};
static int rfr_dissect_element_RfrGetNewDSA_ulFlags(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_);
static int rfr_dissect_element_RfrGetNewDSA_pUserDN(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_);
static int rfr_dissect_element_RfrGetNewDSA_pUserDN_(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_);
static int rfr_dissect_element_RfrGetNewDSA_ppszUnused(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_);
static int rfr_dissect_element_RfrGetNewDSA_ppszUnused_(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_);
static int rfr_dissect_element_RfrGetNewDSA_ppszUnused__(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_);
static int rfr_dissect_element_RfrGetNewDSA_ppszServer(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_);
static int rfr_dissect_element_RfrGetNewDSA_ppszServer_(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_);
static int rfr_dissect_element_RfrGetNewDSA_ppszServer__(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_);
static int rfr_dissect_element_RfrGetFQDNFromLegacyDN_ulFlags(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_);
static int rfr_dissect_element_RfrGetFQDNFromLegacyDN_cbMailboxServerDN(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_);
static int rfr_dissect_element_RfrGetFQDNFromLegacyDN_szMailboxServerDN(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_);
static int rfr_dissect_element_RfrGetFQDNFromLegacyDN_szMailboxServerDN_(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_);
static int rfr_dissect_element_RfrGetFQDNFromLegacyDN_ppszServerFQDN(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_);
static int rfr_dissect_element_RfrGetFQDNFromLegacyDN_ppszServerFQDN_(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_);
static int rfr_dissect_element_RfrGetFQDNFromLegacyDN_ppszServerFQDN__(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_);


/* IDL: enum { */
/* IDL: 	MAPI_E_SUCCESS=0x00000000, */
/* IDL: 	MAPI_E_NO_SUPPORT=0x80040102, */
/* IDL: 	MAPI_E_BAD_CHARWIDTH=0x80040103, */
/* IDL: 	MAPI_E_STRING_TOO_LONG=0x80040105, */
/* IDL: 	MAPI_E_UNKNOWN_FLAGS=0x80040106, */
/* IDL: 	MAPI_E_INVALID_ENTRYID=0x80040107, */
/* IDL: 	MAPI_E_INVALID_OBJECT=0x80040108, */
/* IDL: 	MAPI_E_OBJECT_CHANGED=0x80040109, */
/* IDL: 	MAPI_E_OBJECT_DELETED=0x8004010A, */
/* IDL: 	MAPI_E_BUSY=0x8004010B, */
/* IDL: 	MAPI_E_NOT_ENOUGH_DISK=0x8004010D, */
/* IDL: 	MAPI_E_NOT_ENOUGH_RESOURCES=0x8004010E, */
/* IDL: 	MAPI_E_NOT_FOUND=0x8004010F, */
/* IDL: 	MAPI_E_VERSION=0x80040110, */
/* IDL: 	MAPI_E_LOGON_FAILED=0x80040111, */
/* IDL: 	MAPI_E_SESSION_LIMIT=0x80040112, */
/* IDL: 	MAPI_E_USER_CANCEL=0x80040113, */
/* IDL: 	MAPI_E_UNABLE_TO_ABORT=0x80040114, */
/* IDL: 	MAPI_E_NETWORK_ERROR=0x80040115, */
/* IDL: 	MAPI_E_DISK_ERROR=0x80040116, */
/* IDL: 	MAPI_E_TOO_COMPLEX=0x80040117, */
/* IDL: 	MAPI_E_BAD_COLUMN=0x80040118, */
/* IDL: 	MAPI_E_EXTENDED_ERROR=0x80040119, */
/* IDL: 	MAPI_E_COMPUTED=0x8004011A, */
/* IDL: 	MAPI_E_CORRUPT_DATA=0x8004011B, */
/* IDL: 	MAPI_E_UNCONFIGURED=0x8004011C, */
/* IDL: 	MAPI_E_FAILONEPROVIDER=0x8004011D, */
/* IDL: 	MAPI_E_UNKNOWN_CPID=0x8004011E, */
/* IDL: 	MAPI_E_UNKNOWN_LCID=0x8004011F, */
/* IDL: 	MAPI_E_PASSWORD_CHANGE_REQUIRED=0x80040120, */
/* IDL: 	MAPI_E_PASSWORD_EXPIRED=0x80040121, */
/* IDL: 	MAPI_E_INVALID_WORKSTATION_ACCOUNT=0x80040122, */
/* IDL: 	MAPI_E_INVALID_ACCESS_TIME=0x80040123, */
/* IDL: 	MAPI_E_ACCOUNT_DISABLED=0x80040124, */
/* IDL: 	MAPI_E_END_OF_SESSION=0x80040200, */
/* IDL: 	MAPI_E_UNKNOWN_ENTRYID=0x80040201, */
/* IDL: 	MAPI_E_MISSING_REQUIRED_COLUMN=0x80040202, */
/* IDL: 	MAPI_W_NO_SERVICE=0x80040203, */
/* IDL: 	MAPI_E_BAD_VALUE=0x80040301, */
/* IDL: 	MAPI_E_INVALID_TYPE=0x80040302, */
/* IDL: 	MAPI_E_TYPE_NO_SUPPORT=0x80040303, */
/* IDL: 	MAPI_E_UNEXPECTED_TYPE=0x80040304, */
/* IDL: 	MAPI_E_TOO_BIG=0x80040305, */
/* IDL: 	MAPI_E_DECLINE_COPY=0x80040306, */
/* IDL: 	MAPI_E_UNEXPECTED_ID=0x80040307, */
/* IDL: 	MAPI_W_ERRORS_RETURNED=0x80040380, */
/* IDL: 	MAPI_E_UNABLE_TO_COMPLETE=0x80040400, */
/* IDL: 	MAPI_E_TIMEOUT=0x80040401, */
/* IDL: 	MAPI_E_TABLE_EMPTY=0x80040402, */
/* IDL: 	MAPI_E_TABLE_TOO_BIG=0x80040403, */
/* IDL: 	MAPI_E_INVALID_BOOKMARK=0x80040405, */
/* IDL: 	MAPI_W_POSITION_CHANGED=0x80040481, */
/* IDL: 	MAPI_W_APPROX_COUNT=0x80040482, */
/* IDL: 	MAPI_E_WAIT=0x80040500, */
/* IDL: 	MAPI_E_CANCEL=0x80040501, */
/* IDL: 	MAPI_E_NOT_ME=0x80040502, */
/* IDL: 	MAPI_W_CANCEL_MESSAGE=0x80040580, */
/* IDL: 	MAPI_E_CORRUPT_STORE=0x80040600, */
/* IDL: 	MAPI_E_NOT_IN_QUEUE=0x80040601, */
/* IDL: 	MAPI_E_NO_SUPPRESS=0x80040602, */
/* IDL: 	MAPI_E_COLLISION=0x80040604, */
/* IDL: 	MAPI_E_NOT_INITIALIZED=0x80040605, */
/* IDL: 	MAPI_E_NON_STANDARD=0x80040606, */
/* IDL: 	MAPI_E_NO_RECIPIENTS=0x80040607, */
/* IDL: 	MAPI_E_SUBMITTED=0x80040608, */
/* IDL: 	MAPI_E_HAS_FOLDERS=0x80040609, */
/* IDL: 	MAPI_E_HAS_MESAGES=0x8004060A, */
/* IDL: 	MAPI_E_FOLDER_CYCLE=0x8004060B, */
/* IDL: 	MAPI_W_PARTIAL_COMPLETION=0x80040680, */
/* IDL: 	MAPI_E_AMBIGUOUS_RECIP=0x80040700, */
/* IDL: 	MAPI_E_RESERVED=0xFFFFFFFF, */
/* IDL: } */

int
rfr_dissect_enum_MAPISTATUS(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_, int hf_index _U_, guint32 *param _U_)
{
	guint32 parameter=0;
	if(param){
		parameter=(guint32)*param;
	}
	offset = dissect_ndr_uint32(tvb, offset, pinfo, tree, drep, hf_index, &parameter);
	if(param){
		*param=(guint32)parameter;
	}
	return offset;
}

static int
rfr_dissect_element_RfrGetNewDSA_ulFlags(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_)
{
	offset = PIDL_dissect_uint32(tvb, offset, pinfo, tree, drep, hf_rfr_RfrGetNewDSA_ulFlags, 0);

	return offset;
}

static int
rfr_dissect_element_RfrGetNewDSA_pUserDN(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_)
{
	offset = dissect_ndr_toplevel_pointer(tvb, offset, pinfo, tree, drep, rfr_dissect_element_RfrGetNewDSA_pUserDN_, NDR_POINTER_REF, "Pointer to Puserdn (uint8)",hf_rfr_RfrGetNewDSA_pUserDN);

	return offset;
}

static int
rfr_dissect_element_RfrGetNewDSA_pUserDN_(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_)
{
	char *data;

	offset = dissect_ndr_cvstring(tvb, offset, pinfo, tree, drep, sizeof(guint8), hf_rfr_RfrGetNewDSA_pUserDN, FALSE, &data);
	proto_item_append_text(tree, ": %s", data);

	return offset;
}

static int
rfr_dissect_element_RfrGetNewDSA_ppszUnused(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_)
{
	offset = dissect_ndr_toplevel_pointer(tvb, offset, pinfo, tree, drep, rfr_dissect_element_RfrGetNewDSA_ppszUnused_, NDR_POINTER_UNIQUE, "Pointer to Ppszunused (uint8)",hf_rfr_RfrGetNewDSA_ppszUnused);

	return offset;
}

static int
rfr_dissect_element_RfrGetNewDSA_ppszUnused_(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_)
{
	offset = dissect_ndr_embedded_pointer(tvb, offset, pinfo, tree, drep, rfr_dissect_element_RfrGetNewDSA_ppszUnused__, NDR_POINTER_UNIQUE, "Pointer to Ppszunused (uint8)",hf_rfr_RfrGetNewDSA_ppszUnused);

	return offset;
}

static int
rfr_dissect_element_RfrGetNewDSA_ppszUnused__(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_)
{
	char *data;

	offset = dissect_ndr_cvstring(tvb, offset, pinfo, tree, drep, sizeof(guint8), hf_rfr_RfrGetNewDSA_ppszUnused, FALSE, &data);
	proto_item_append_text(tree, ": %s", data);

	return offset;
}

static int
rfr_dissect_element_RfrGetNewDSA_ppszServer(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_)
{
	offset = dissect_ndr_toplevel_pointer(tvb, offset, pinfo, tree, drep, rfr_dissect_element_RfrGetNewDSA_ppszServer_, NDR_POINTER_UNIQUE, "Pointer to Ppszserver (uint8)",hf_rfr_RfrGetNewDSA_ppszServer);

	return offset;
}

static int
rfr_dissect_element_RfrGetNewDSA_ppszServer_(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_)
{
	offset = dissect_ndr_embedded_pointer(tvb, offset, pinfo, tree, drep, rfr_dissect_element_RfrGetNewDSA_ppszServer__, NDR_POINTER_UNIQUE, "Pointer to Ppszserver (uint8)",hf_rfr_RfrGetNewDSA_ppszServer);

	return offset;
}

static int
rfr_dissect_element_RfrGetNewDSA_ppszServer__(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_)
{
	char *data;

	offset = dissect_ndr_cvstring(tvb, offset, pinfo, tree, drep, sizeof(guint8), hf_rfr_RfrGetNewDSA_ppszServer, FALSE, &data);
	proto_item_append_text(tree, ": %s", data);

	return offset;
}

/* IDL: MAPISTATUS RfrGetNewDSA( */
/* IDL: [in] uint32 ulFlags, */
/* IDL: [ref] [in] [charset(DOS)] uint8 *pUserDN, */
/* IDL: [out] [unique(1)] [in] [charset(DOS)] uint8 **ppszUnused, */
/* IDL: [out] [unique(1)] [in] [charset(DOS)] uint8 **ppszServer */
/* IDL: ); */

static int
rfr_dissect_RfrGetNewDSA_response(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_)
{
	guint32 status;

	pinfo->dcerpc_procedure_name="RfrGetNewDSA";
	offset = rfr_dissect_element_RfrGetNewDSA_ppszUnused(tvb, offset, pinfo, tree, drep);
	offset = dissect_deferred_pointers(pinfo, tvb, offset, drep);

	offset = rfr_dissect_element_RfrGetNewDSA_ppszServer(tvb, offset, pinfo, tree, drep);
	offset = dissect_deferred_pointers(pinfo, tvb, offset, drep);

	offset = dissect_ndr_uint32(tvb, offset, pinfo, tree, drep, hf_rfr_MAPISTATUS_status, &status);
	if (status != 0 && check_col(pinfo->cinfo, COL_INFO))
		col_append_fstr(pinfo->cinfo, COL_INFO, ", Status: %s", val_to_str(status, rfr_MAPISTATUS_vals, "Unknown MAPISTATUS error 0x%08x"));

	return offset;
}

static int
rfr_dissect_RfrGetNewDSA_request(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_)
{
	pinfo->dcerpc_procedure_name="RfrGetNewDSA";
	offset = rfr_dissect_element_RfrGetNewDSA_ulFlags(tvb, offset, pinfo, tree, drep);
	offset = dissect_deferred_pointers(pinfo, tvb, offset, drep);
	offset = rfr_dissect_element_RfrGetNewDSA_pUserDN(tvb, offset, pinfo, tree, drep);
	offset = dissect_deferred_pointers(pinfo, tvb, offset, drep);
	offset = rfr_dissect_element_RfrGetNewDSA_ppszUnused(tvb, offset, pinfo, tree, drep);
	offset = dissect_deferred_pointers(pinfo, tvb, offset, drep);
	offset = rfr_dissect_element_RfrGetNewDSA_ppszServer(tvb, offset, pinfo, tree, drep);
	offset = dissect_deferred_pointers(pinfo, tvb, offset, drep);
	return offset;
}

static int
rfr_dissect_element_RfrGetFQDNFromLegacyDN_ulFlags(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_)
{
	offset = PIDL_dissect_uint32(tvb, offset, pinfo, tree, drep, hf_rfr_RfrGetFQDNFromLegacyDN_ulFlags, 0);

	return offset;
}

static int
rfr_dissect_element_RfrGetFQDNFromLegacyDN_cbMailboxServerDN(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_)
{
	offset = PIDL_dissect_uint32(tvb, offset, pinfo, tree, drep, hf_rfr_RfrGetFQDNFromLegacyDN_cbMailboxServerDN, 0);

	return offset;
}

static int
rfr_dissect_element_RfrGetFQDNFromLegacyDN_szMailboxServerDN(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_)
{
	offset = dissect_ndr_toplevel_pointer(tvb, offset, pinfo, tree, drep, rfr_dissect_element_RfrGetFQDNFromLegacyDN_szMailboxServerDN_, NDR_POINTER_REF, "Pointer to Szmailboxserverdn (uint8)",hf_rfr_RfrGetFQDNFromLegacyDN_szMailboxServerDN);

	return offset;
}

static int
rfr_dissect_element_RfrGetFQDNFromLegacyDN_szMailboxServerDN_(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_)
{
	char *data;

	offset = dissect_ndr_cvstring(tvb, offset, pinfo, tree, drep, sizeof(guint8), hf_rfr_RfrGetFQDNFromLegacyDN_szMailboxServerDN, FALSE, &data);
	proto_item_append_text(tree, ": %s", data);

	return offset;
}

static int
rfr_dissect_element_RfrGetFQDNFromLegacyDN_ppszServerFQDN(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_)
{
	offset = dissect_ndr_toplevel_pointer(tvb, offset, pinfo, tree, drep, rfr_dissect_element_RfrGetFQDNFromLegacyDN_ppszServerFQDN_, NDR_POINTER_REF, "Pointer to Ppszserverfqdn (uint8)",hf_rfr_RfrGetFQDNFromLegacyDN_ppszServerFQDN);

	return offset;
}

static int
rfr_dissect_element_RfrGetFQDNFromLegacyDN_ppszServerFQDN_(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_)
{
	offset = dissect_ndr_embedded_pointer(tvb, offset, pinfo, tree, drep, rfr_dissect_element_RfrGetFQDNFromLegacyDN_ppszServerFQDN__, NDR_POINTER_REF, "Pointer to Ppszserverfqdn (uint8)",hf_rfr_RfrGetFQDNFromLegacyDN_ppszServerFQDN);

	return offset;
}

static int
rfr_dissect_element_RfrGetFQDNFromLegacyDN_ppszServerFQDN__(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_)
{
	char *data;

	offset = dissect_ndr_cvstring(tvb, offset, pinfo, tree, drep, sizeof(guint8), hf_rfr_RfrGetFQDNFromLegacyDN_ppszServerFQDN, FALSE, &data);
	proto_item_append_text(tree, ": %s", data);

	return offset;
}

/* IDL: MAPISTATUS RfrGetFQDNFromLegacyDN( */
/* IDL: [in] uint32 ulFlags, */
/* IDL: [in] [range(10 1024)] uint32 cbMailboxServerDN, */
/* IDL: [ref] [in] [charset(DOS)] [size_is(cbMailboxServerDN)] uint8 *szMailboxServerDN, */
/* IDL: [out] [ref] [charset(DOS)] uint8 **ppszServerFQDN */
/* IDL: ); */

static int
rfr_dissect_RfrGetFQDNFromLegacyDN_response(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_)
{
	guint32 status;

	pinfo->dcerpc_procedure_name="RfrGetFQDNFromLegacyDN";
	offset = rfr_dissect_element_RfrGetFQDNFromLegacyDN_ppszServerFQDN(tvb, offset, pinfo, tree, drep);
	offset = dissect_deferred_pointers(pinfo, tvb, offset, drep);

	offset = dissect_ndr_uint32(tvb, offset, pinfo, tree, drep, hf_rfr_MAPISTATUS_status, &status);
	if (status != 0 && check_col(pinfo->cinfo, COL_INFO))
		col_append_fstr(pinfo->cinfo, COL_INFO, ", Status: %s", val_to_str(status, rfr_MAPISTATUS_vals, "Unknown MAPISTATUS error 0x%08x"));

	return offset;
}

static int
rfr_dissect_RfrGetFQDNFromLegacyDN_request(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_)
{
	pinfo->dcerpc_procedure_name="RfrGetFQDNFromLegacyDN";
	offset = rfr_dissect_element_RfrGetFQDNFromLegacyDN_ulFlags(tvb, offset, pinfo, tree, drep);
	offset = dissect_deferred_pointers(pinfo, tvb, offset, drep);
	offset = rfr_dissect_element_RfrGetFQDNFromLegacyDN_cbMailboxServerDN(tvb, offset, pinfo, tree, drep);
	offset = dissect_deferred_pointers(pinfo, tvb, offset, drep);
	offset = rfr_dissect_element_RfrGetFQDNFromLegacyDN_szMailboxServerDN(tvb, offset, pinfo, tree, drep);
	offset = dissect_deferred_pointers(pinfo, tvb, offset, drep);
	return offset;
}


static dcerpc_sub_dissector rfr_dissectors[] = {
	{ 0, "RfrGetNewDSA",
	   rfr_dissect_RfrGetNewDSA_request, rfr_dissect_RfrGetNewDSA_response},
	{ 1, "RfrGetFQDNFromLegacyDN",
	   rfr_dissect_RfrGetFQDNFromLegacyDN_request, rfr_dissect_RfrGetFQDNFromLegacyDN_response},
	{ 0, NULL, NULL, NULL }
};

void proto_register_dcerpc_rfr(void)
{
	static hf_register_info hf[] = {
	{ &hf_rfr_MAPISTATUS_status, 
	  { "MAPISTATUS", "rfr.MAPISTATUS_status", FT_UINT32, BASE_HEX, VALS(rfr_MAPISTATUS_vals), 0, NULL, HFILL }},
	{ &hf_rfr_RfrGetFQDNFromLegacyDN_ulFlags, 
	  { "Ulflags", "rfr.RfrGetFQDNFromLegacyDN.ulFlags", FT_UINT32, BASE_DEC, NULL, 0, NULL, HFILL }},
	{ &hf_rfr_RfrGetFQDNFromLegacyDN_szMailboxServerDN, 
	  { "Szmailboxserverdn", "rfr.RfrGetFQDNFromLegacyDN.szMailboxServerDN", FT_STRING, BASE_NONE, NULL, 0, NULL, HFILL }},
	{ &hf_rfr_opnum, 
	  { "Operation", "rfr.opnum", FT_UINT16, BASE_DEC, NULL, 0, NULL, HFILL }},
	{ &hf_rfr_RfrGetNewDSA_ulFlags, 
	  { "Ulflags", "rfr.RfrGetNewDSA.ulFlags", FT_UINT32, BASE_DEC, NULL, 0, NULL, HFILL }},
	{ &hf_rfr_RfrGetFQDNFromLegacyDN_cbMailboxServerDN, 
	  { "Cbmailboxserverdn", "rfr.RfrGetFQDNFromLegacyDN.cbMailboxServerDN", FT_UINT32, BASE_DEC, NULL, 0, NULL, HFILL }},
	{ &hf_rfr_RfrGetNewDSA_pUserDN, 
	  { "Puserdn", "rfr.RfrGetNewDSA.pUserDN", FT_STRING, BASE_NONE, NULL, 0, NULL, HFILL }},
	{ &hf_rfr_RfrGetNewDSA_ppszUnused, 
	  { "Ppszunused", "rfr.RfrGetNewDSA.ppszUnused", FT_STRING, BASE_NONE, NULL, 0, NULL, HFILL }},
	{ &hf_rfr_RfrGetNewDSA_ppszServer, 
	  { "Ppszserver", "rfr.RfrGetNewDSA.ppszServer", FT_STRING, BASE_NONE, NULL, 0, NULL, HFILL }},
	{ &hf_rfr_RfrGetFQDNFromLegacyDN_ppszServerFQDN, 
	  { "Ppszserverfqdn", "rfr.RfrGetFQDNFromLegacyDN.ppszServerFQDN", FT_STRING, BASE_NONE, NULL, 0, NULL, HFILL }},
	};


	static gint *ett[] = {
		&ett_dcerpc_rfr,
	};

	proto_dcerpc_rfr = proto_register_protocol("Exchange 2003 Directory Request For Response", "RFR", "rfr");
	proto_register_field_array(proto_dcerpc_rfr, hf, array_length (hf));
	proto_register_subtree_array(ett, array_length(ett));
}

void proto_reg_handoff_dcerpc_rfr(void)
{
	dcerpc_init_uuid(proto_dcerpc_rfr, ett_dcerpc_rfr,
		&uuid_dcerpc_rfr, ver_dcerpc_rfr,
		rfr_dissectors, hf_rfr_opnum);
}
