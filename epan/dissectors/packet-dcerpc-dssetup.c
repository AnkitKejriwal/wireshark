/* DO NOT EDIT
 * This dissector is autogenerated
 */

/* packet-dcerpc-dssetup.c
 * Routines for DS SETUP packet disassembly
 *   ronnie sahlberg 2005
 * Autogenerated based on the IDL definitions from samba 4
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include <string.h>

#include <epan/packet.h>
#include "packet-dcerpc.h"
#include "packet-dcerpc-nt.h"
#include "packet-windows-common.h"
#include "packet-dcerpc-dssetup.h"

static int proto_dssetup = -1;


/* INCLUDED FILE : ETH_HF */
static int hf_dssetup_opnum = -1;
static int hf_dssetup_rc = -1;
static int hf_dssetup_DsRoleFlags_DS_ROLE_PRIMARY_DS_RUNNING = -1;
static int hf_dssetup_DsRoleFlags_DS_ROLE_PRIMARY_DS_MIXED_MODE = -1;
static int hf_dssetup_DsRoleFlags_DS_ROLE_UPGRADE_IN_PROGRESS = -1;
static int hf_dssetup_DsRoleFlags_DS_ROLE_PRIMARY_DOMAIN_GUID_PRESENT = -1;
static int hf_dssetup_DsRolePrimaryDomInfoBasic_role = -1;
static int hf_dssetup_DsRolePrimaryDomInfoBasic_flags = -1;
static int hf_dssetup_DsRolePrimaryDomInfoBasic_domain = -1;
static int hf_dssetup_DsRolePrimaryDomInfoBasic_dns_domain = -1;
static int hf_dssetup_DsRolePrimaryDomInfoBasic_forest = -1;
static int hf_dssetup_DsRolePrimaryDomInfoBasic_domain_guid = -1;
static int hf_dssetup_DsRoleUpgradeStatus_upgrading = -1;
static int hf_dssetup_DsRoleUpgradeStatus_previous_role = -1;
static int hf_dssetup_DsRoleOpStatus_status = -1;
static int hf_dssetup_DsRoleInfo_DS_ROLE_BASIC_INFORMATION_basic = -1;
static int hf_dssetup_DsRoleInfo_DS_ROLE_UPGRADE_STATUS_upgrade = -1;
static int hf_dssetup_DsRoleInfo_DS_ROLE_OP_STATUS_opstatus = -1;
static int hf_dssetup_DsRoleGetPrimaryDomainInformation_level = -1;
static int hf_dssetup_DsRoleGetPrimaryDomainInformation_info = -1;
/* END OF INCLUDED FILE : ETH_HF */





/* INCLUDED FILE : ETH_ETT */
static gint ett_dssetup = -1;
static gint ett_dssetup_DsRoleFlags = -1;
static gint ett_dssetup_DsRolePrimaryDomInfoBasic = -1;
static gint ett_dssetup_DsRoleUpgradeStatus = -1;
static gint ett_dssetup_DsRoleOpStatus = -1;
static gint ett_dssetup_DsRoleInfo = -1;
/* END OF INCLUDED FILE : ETH_ETT */





/* INCLUDED FILE : ETH_CODE */
static e_uuid_t uuid_dcerpc_dssetup = {
    0x3919286a, 0xb10c, 0x11d0,
    { 0x9b, 0xa8, 0x00, 0xc0, 0x4f, 0xd9, 0x2e, 0xf5}
};

static guint16 ver_dssetup = 0;


const value_string dssetup_DsRole_vals[] = {
    { 0	, "DS_ROLE_STANDALONE_WORKSTATION" },
    { 1	, "DS_ROLE_MEMBER_WORKSTATION" },
    { 2	, "DS_ROLE_STANDALONE_SERVER" },
    { 3	, "DS_ROLE_MEMBER_SERVER" },
    { 4	, "DS_ROLE_BACKUP_DC" },
    { 5	, "DS_ROLE_PRIMARY_DC" },
    { 0	, NULL }
};

int
dssetup_dissect_DsRole(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep, int hf_index, guint32 param _U_)
{
    offset=dissect_ndr_uint16(tvb, offset, pinfo, tree, drep, hf_index, NULL);
    return offset;
}

static const true_false_string DS_ROLE_PRIMARY_DS_RUNNING_tfs = {
    "DS_ROLE_PRIMARY_DS_RUNNING is SET",
    "DS_ROLE_PRIMARY_DS_RUNNING is NOT set"
};

static const true_false_string DS_ROLE_PRIMARY_DS_MIXED_MODE_tfs = {
    "DS_ROLE_PRIMARY_DS_MIXED_MODE is SET",
    "DS_ROLE_PRIMARY_DS_MIXED_MODE is NOT set"
};

static const true_false_string DS_ROLE_UPGRADE_IN_PROGRESS_tfs = {
    "DS_ROLE_UPGRADE_IN_PROGRESS is SET",
    "DS_ROLE_UPGRADE_IN_PROGRESS is NOT set"
};

static const true_false_string DS_ROLE_PRIMARY_DOMAIN_GUID_PRESENT_tfs = {
    "DS_ROLE_PRIMARY_DOMAIN_GUID_PRESENT is SET",
    "DS_ROLE_PRIMARY_DOMAIN_GUID_PRESENT is NOT set"
};


int
dssetup_dissect_DsRoleFlags(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *parent_tree, guint8 *drep, int hf_index, guint32 param _U_)
{
    proto_item *item=NULL;
    proto_tree *tree=NULL;
    guint32 flags;

    ALIGN_TO_4_BYTES;

    if(parent_tree){
        item=proto_tree_add_item(parent_tree, hf_index, tvb, offset, 4, TRUE);
        tree=proto_item_add_subtree(item, ett_dssetup_DsRoleFlags);
    }

    offset=dissect_ndr_uint32(tvb, offset, pinfo, NULL, drep, -1, &flags);


    proto_tree_add_boolean(tree, hf_dssetup_DsRoleFlags_DS_ROLE_PRIMARY_DS_RUNNING, tvb, offset-4, 4, flags);
    if(flags&0x00000001){
        proto_item_append_text(item, " DS_ROLE_PRIMARY_DS_RUNNING");
    }
    flags&=(~0x00000001);

    proto_tree_add_boolean(tree, hf_dssetup_DsRoleFlags_DS_ROLE_PRIMARY_DS_MIXED_MODE, tvb, offset-4, 4, flags);
    if(flags&0x00000002){
        proto_item_append_text(item, " DS_ROLE_PRIMARY_DS_MIXED_MODE");
    }
    flags&=(~0x00000002);

    proto_tree_add_boolean(tree, hf_dssetup_DsRoleFlags_DS_ROLE_UPGRADE_IN_PROGRESS, tvb, offset-4, 4, flags);
    if(flags&0x00000004){
        proto_item_append_text(item, " DS_ROLE_UPGRADE_IN_PROGRESS");
    }
    flags&=(~0x00000004);

    proto_tree_add_boolean(tree, hf_dssetup_DsRoleFlags_DS_ROLE_PRIMARY_DOMAIN_GUID_PRESENT, tvb, offset-4, 4, flags);
    if(flags&0x01000000){
        proto_item_append_text(item, " DS_ROLE_PRIMARY_DOMAIN_GUID_PRESENT");
    }
    flags&=(~0x01000000);

    if(flags){
        proto_item_append_text(item, "UNKNOWN-FLAGS");
    }

    return offset;
}
static int
dssetup_dissect_DsRolePrimaryDomInfoBasic_role(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep)
{
    guint32 param=0;
    offset=dssetup_dissect_DsRole(tvb, offset, pinfo, tree, drep, hf_dssetup_DsRolePrimaryDomInfoBasic_role, param);
    return offset;
}

static int
dssetup_dissect_DsRolePrimaryDomInfoBasic_flags(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep)
{
    guint32 param=0;
    offset=dssetup_dissect_DsRoleFlags(tvb, offset, pinfo, tree, drep, hf_dssetup_DsRolePrimaryDomInfoBasic_flags, param);
    return offset;
}


static int
dssetup_dissect_unistr(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep, int hf_index, guint32 param _U_)
{
    offset=dissect_ndr_cvstring(tvb, offset, pinfo, tree, drep, 2, hf_index, FALSE, NULL);
    return offset;
}

static int
dssetup_dissect_DsRolePrimaryDomInfoBasic_domain(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep)
{
    guint32 param=0;
    offset=dssetup_dissect_unistr(tvb, offset, pinfo, tree, drep, hf_dssetup_DsRolePrimaryDomInfoBasic_domain, param);
    return offset;
}

static int
unique_dssetup_dissect_DsRolePrimaryDomInfoBasic_domain(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep)
{
    offset=dissect_ndr_embedded_pointer(tvb, offset, pinfo, tree, drep, dssetup_dissect_DsRolePrimaryDomInfoBasic_domain, NDR_POINTER_UNIQUE, "domain", -1);
    return offset;
}

static int
dssetup_dissect_DsRolePrimaryDomInfoBasic_dns_domain(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep)
{
    guint32 param=0;
    offset=dssetup_dissect_unistr(tvb, offset, pinfo, tree, drep, hf_dssetup_DsRolePrimaryDomInfoBasic_dns_domain, param);
    return offset;
}

static int
unique_dssetup_dissect_DsRolePrimaryDomInfoBasic_dns_domain(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep)
{
    offset=dissect_ndr_embedded_pointer(tvb, offset, pinfo, tree, drep, dssetup_dissect_DsRolePrimaryDomInfoBasic_dns_domain, NDR_POINTER_UNIQUE, "dns_domain", -1);
    return offset;
}

static int
dssetup_dissect_DsRolePrimaryDomInfoBasic_forest(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep)
{
    guint32 param=0;
    offset=dssetup_dissect_unistr(tvb, offset, pinfo, tree, drep, hf_dssetup_DsRolePrimaryDomInfoBasic_forest, param);
    return offset;
}

static int
unique_dssetup_dissect_DsRolePrimaryDomInfoBasic_forest(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep)
{
    offset=dissect_ndr_embedded_pointer(tvb, offset, pinfo, tree, drep, dssetup_dissect_DsRolePrimaryDomInfoBasic_forest, NDR_POINTER_UNIQUE, "forest", -1);
    return offset;
}


static int
dssetup_dissect_GUID(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep, int hf_index, guint32 param _U_)
{
    offset=dissect_ndr_uuid_t(tvb, offset, pinfo, tree, drep, hf_index, NULL);
    return offset;
}

static int
dssetup_dissect_DsRolePrimaryDomInfoBasic_domain_guid(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep)
{
    guint32 param=0;
    offset=dssetup_dissect_GUID(tvb, offset, pinfo, tree, drep, hf_dssetup_DsRolePrimaryDomInfoBasic_domain_guid, param);
    return offset;
}


int
dssetup_dissect_DsRolePrimaryDomInfoBasic(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *parent_tree, guint8 *drep, int hf_index, guint32 param _U_)
{
    proto_item *item=NULL;
    proto_tree *tree=NULL;
    int old_offset;

    ALIGN_TO_4_BYTES;

    old_offset=offset;
    if(parent_tree){
        item=proto_tree_add_item(parent_tree, hf_index, tvb, offset, -1, TRUE);
        tree=proto_item_add_subtree(item, ett_dssetup_DsRolePrimaryDomInfoBasic);
    }

    offset=dssetup_dissect_DsRolePrimaryDomInfoBasic_role(tvb, offset, pinfo, tree, drep);

    offset=dssetup_dissect_DsRolePrimaryDomInfoBasic_flags(tvb, offset, pinfo, tree, drep);

    offset=unique_dssetup_dissect_DsRolePrimaryDomInfoBasic_domain(tvb, offset, pinfo, tree, drep);

    offset=unique_dssetup_dissect_DsRolePrimaryDomInfoBasic_dns_domain(tvb, offset, pinfo, tree, drep);

    offset=unique_dssetup_dissect_DsRolePrimaryDomInfoBasic_forest(tvb, offset, pinfo, tree, drep);

    offset=dssetup_dissect_DsRolePrimaryDomInfoBasic_domain_guid(tvb, offset, pinfo, tree, drep);

    proto_item_set_len(item, offset-old_offset);

    return offset;
}

const value_string dssetup_DsUpgrade_vals[] = {
    { 0	, "DS_ROLE_NOT_UPGRADING" },
    { 1	, "DS_ROLE_UPGRADING" },
    { 0	, NULL }
};

int
dssetup_dissect_DsUpgrade(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep, int hf_index, guint32 param _U_)
{
    offset=dissect_ndr_uint32(tvb, offset, pinfo, tree, drep, hf_index, NULL);
    return offset;
}


const value_string dssetup_DsPrevious_vals[] = {
    { 0	, "DS_ROLE_PREVIOUS_UNKNOWN" },
    { 1	, "DS_ROLE_PREVIOUS_PRIMARY" },
    { 2	, "DS_ROLE_PREVIOUS_BACKUP" },
    { 0	, NULL }
};

int
dssetup_dissect_DsPrevious(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep, int hf_index, guint32 param _U_)
{
    offset=dissect_ndr_uint16(tvb, offset, pinfo, tree, drep, hf_index, NULL);
    return offset;
}

static int
dssetup_dissect_DsRoleUpgradeStatus_upgrading(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep)
{
    guint32 param=0;
    offset=dssetup_dissect_DsUpgrade(tvb, offset, pinfo, tree, drep, hf_dssetup_DsRoleUpgradeStatus_upgrading, param);
    return offset;
}

static int
dssetup_dissect_DsRoleUpgradeStatus_previous_role(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep)
{
    guint32 param=0;
    offset=dssetup_dissect_DsPrevious(tvb, offset, pinfo, tree, drep, hf_dssetup_DsRoleUpgradeStatus_previous_role, param);
    return offset;
}


int
dssetup_dissect_DsRoleUpgradeStatus(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *parent_tree, guint8 *drep, int hf_index, guint32 param _U_)
{
    proto_item *item=NULL;
    proto_tree *tree=NULL;
    int old_offset;

    ALIGN_TO_4_BYTES;

    old_offset=offset;
    if(parent_tree){
        item=proto_tree_add_item(parent_tree, hf_index, tvb, offset, -1, TRUE);
        tree=proto_item_add_subtree(item, ett_dssetup_DsRoleUpgradeStatus);
    }

    offset=dssetup_dissect_DsRoleUpgradeStatus_upgrading(tvb, offset, pinfo, tree, drep);

    offset=dssetup_dissect_DsRoleUpgradeStatus_previous_role(tvb, offset, pinfo, tree, drep);

    proto_item_set_len(item, offset-old_offset);

    return offset;
}

const value_string dssetup_DsRoleOp_vals[] = {
    { 0	, "DS_ROLE_OP_IDLE" },
    { 1	, "DS_ROLE_OP_ACTIVE" },
    { 2	, "DS_ROLE_OP_NEEDS_REBOOT" },
    { 0	, NULL }
};

int
dssetup_dissect_DsRoleOp(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep, int hf_index, guint32 param _U_)
{
    offset=dissect_ndr_uint16(tvb, offset, pinfo, tree, drep, hf_index, NULL);
    return offset;
}

static int
dssetup_dissect_DsRoleOpStatus_status(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep)
{
    guint32 param=0;
    offset=dssetup_dissect_DsRoleOp(tvb, offset, pinfo, tree, drep, hf_dssetup_DsRoleOpStatus_status, param);
    return offset;
}


int
dssetup_dissect_DsRoleOpStatus(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *parent_tree, guint8 *drep, int hf_index, guint32 param _U_)
{
    proto_item *item=NULL;
    proto_tree *tree=NULL;
    int old_offset;

    ALIGN_TO_2_BYTES;

    old_offset=offset;
    if(parent_tree){
        item=proto_tree_add_item(parent_tree, hf_index, tvb, offset, -1, TRUE);
        tree=proto_item_add_subtree(item, ett_dssetup_DsRoleOpStatus);
    }

    offset=dssetup_dissect_DsRoleOpStatus_status(tvb, offset, pinfo, tree, drep);

    proto_item_set_len(item, offset-old_offset);

    return offset;
}

const value_string dssetup_DsRoleInfoLevel_vals[] = {
    { 1	, "DS_ROLE_BASIC_INFORMATION" },
    { 2	, "DS_ROLE_UPGRADE_STATUS" },
    { 3	, "DS_ROLE_OP_STATUS" },
    { 0	, NULL }
};

int
dssetup_dissect_DsRoleInfoLevel(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep, int hf_index, guint32 param _U_)
{
    offset=dissect_ndr_uint16(tvb, offset, pinfo, tree, drep, hf_index, NULL);
    return offset;
}

static int
dssetup_dissect_union_DsRoleInfo_DS_ROLE_BASIC_INFORMATION_basic(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep)
{
    guint32 param=0;
    offset=dssetup_dissect_DsRolePrimaryDomInfoBasic(tvb, offset, pinfo, tree, drep, hf_dssetup_DsRoleInfo_DS_ROLE_BASIC_INFORMATION_basic, param);
    return offset;
}

static int
dssetup_dissect_union_DsRoleInfo_DS_ROLE_UPGRADE_STATUS_upgrade(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep)
{
    guint32 param=0;
    offset=dssetup_dissect_DsRoleUpgradeStatus(tvb, offset, pinfo, tree, drep, hf_dssetup_DsRoleInfo_DS_ROLE_UPGRADE_STATUS_upgrade, param);
    return offset;
}

static int
dssetup_dissect_union_DsRoleInfo_DS_ROLE_OP_STATUS_opstatus(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep)
{
    guint32 param=0;
    offset=dssetup_dissect_DsRoleOpStatus(tvb, offset, pinfo, tree, drep, hf_dssetup_DsRoleInfo_DS_ROLE_OP_STATUS_opstatus, param);
    return offset;
}


static int
dssetup_dissect_union_DsRoleInfo(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *parent_tree, guint8 *drep, int hf_index, guint32 param _U_)
{
    proto_item *item=NULL;
    proto_tree *tree=NULL;
    int old_offset;
    guint16 level;

    ALIGN_TO_2_BYTES;

    old_offset=offset;
    if(parent_tree){
        item=proto_tree_add_text(parent_tree, tvb, offset, -1, "DsRoleInfo");
        tree=proto_item_add_subtree(item, ett_dssetup_DsRoleInfo);
    }

    offset=dissect_ndr_uint16(tvb, offset, pinfo, tree,
                              drep, hf_index, &level);

    switch(level){
    case DS_ROLE_BASIC_INFORMATION:
        ALIGN_TO_4_BYTES;
        offset=dssetup_dissect_union_DsRoleInfo_DS_ROLE_BASIC_INFORMATION_basic(tvb, offset, pinfo, tree, drep);
        break;

    case DS_ROLE_UPGRADE_STATUS:
        ALIGN_TO_4_BYTES;
        offset=dssetup_dissect_union_DsRoleInfo_DS_ROLE_UPGRADE_STATUS_upgrade(tvb, offset, pinfo, tree, drep);
        break;

    case DS_ROLE_OP_STATUS:
        ALIGN_TO_2_BYTES;
        offset=dssetup_dissect_union_DsRoleInfo_DS_ROLE_OP_STATUS_opstatus(tvb, offset, pinfo, tree, drep);
        break;

    }

    proto_item_set_len(item, offset-old_offset);

   return offset;
}
static int
dssetup_dissect_DsRoleGetPrimaryDomainInformation_level(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep)
{
    guint32 param=0;
    offset=dssetup_dissect_DsRoleInfoLevel(tvb, offset, pinfo, tree, drep, hf_dssetup_DsRoleGetPrimaryDomainInformation_level, param);
    return offset;
}

static int
dssetup_dissect_DsRoleGetPrimaryDomainInformation_info(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep)
{
    guint32 param=0;
    offset=dssetup_dissect_union_DsRoleInfo(tvb, offset, pinfo, tree, drep, hf_dssetup_DsRoleGetPrimaryDomainInformation_info, param);
    return offset;
}

static int
unique_dssetup_dissect_DsRoleGetPrimaryDomainInformation_info(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep)
{
    offset=dissect_ndr_toplevel_pointer(tvb, offset, pinfo, tree, drep, dssetup_dissect_DsRoleGetPrimaryDomainInformation_info, NDR_POINTER_UNIQUE, "info", -1);
    return offset;
}


static int
dssetup_dissect_DsRoleGetPrimaryDomainInformation_request(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_)
{
        offset=dssetup_dissect_DsRoleGetPrimaryDomainInformation_level(tvb, offset, pinfo, tree, drep);
        offset=dissect_deferred_pointers(pinfo, tvb, offset, drep);


   return offset;
}

static int
dssetup_dissect_DsRoleGetPrimaryDomainInformation_response(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_)
{
        offset=unique_dssetup_dissect_DsRoleGetPrimaryDomainInformation_info(tvb, offset, pinfo, tree, drep);
        offset=dissect_deferred_pointers(pinfo, tvb, offset, drep);

   offset=dissect_ntstatus(tvb, offset, pinfo, tree, drep, hf_dssetup_rc, NULL);


   return offset;
}

static int
dssetup_dissect_DsRoleDnsNameToFlatName_request(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_)
{

   return offset;
}

static int
dssetup_dissect_DsRoleDnsNameToFlatName_response(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_)
{
   offset=dissect_ntstatus(tvb, offset, pinfo, tree, drep, hf_dssetup_rc, NULL);


   return offset;
}

static int
dssetup_dissect_DsRoleDcAsDc_request(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_)
{

   return offset;
}

static int
dssetup_dissect_DsRoleDcAsDc_response(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_)
{
   offset=dissect_ntstatus(tvb, offset, pinfo, tree, drep, hf_dssetup_rc, NULL);


   return offset;
}

static int
dssetup_dissect_DsRoleDcAsReplica_request(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_)
{

   return offset;
}

static int
dssetup_dissect_DsRoleDcAsReplica_response(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_)
{
   offset=dissect_ntstatus(tvb, offset, pinfo, tree, drep, hf_dssetup_rc, NULL);


   return offset;
}

static int
dssetup_dissect_DsRoleDemoteDc_request(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_)
{

   return offset;
}

static int
dssetup_dissect_DsRoleDemoteDc_response(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_)
{
   offset=dissect_ntstatus(tvb, offset, pinfo, tree, drep, hf_dssetup_rc, NULL);


   return offset;
}

static int
dssetup_dissect_DsRoleGetDcOperationProgress_request(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_)
{

   return offset;
}

static int
dssetup_dissect_DsRoleGetDcOperationProgress_response(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_)
{
   offset=dissect_ntstatus(tvb, offset, pinfo, tree, drep, hf_dssetup_rc, NULL);


   return offset;
}

static int
dssetup_dissect_DsRoleGetDcOperationResults_request(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_)
{

   return offset;
}

static int
dssetup_dissect_DsRoleGetDcOperationResults_response(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_)
{
   offset=dissect_ntstatus(tvb, offset, pinfo, tree, drep, hf_dssetup_rc, NULL);


   return offset;
}

static int
dssetup_dissect_DsRoleCancel_request(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_)
{

   return offset;
}

static int
dssetup_dissect_DsRoleCancel_response(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_)
{
   offset=dissect_ntstatus(tvb, offset, pinfo, tree, drep, hf_dssetup_rc, NULL);


   return offset;
}

static int
dssetup_dissect_DsRoleServerSaveStateForUpgrade_request(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_)
{

   return offset;
}

static int
dssetup_dissect_DsRoleServerSaveStateForUpgrade_response(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_)
{
   offset=dissect_ntstatus(tvb, offset, pinfo, tree, drep, hf_dssetup_rc, NULL);


   return offset;
}

static int
dssetup_dissect_DsRoleUpgradeDownlevelServer_request(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_)
{

   return offset;
}

static int
dssetup_dissect_DsRoleUpgradeDownlevelServer_response(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_)
{
   offset=dissect_ntstatus(tvb, offset, pinfo, tree, drep, hf_dssetup_rc, NULL);


   return offset;
}

static int
dssetup_dissect_DsRoleAbortDownlevelServerUpgrade_request(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_)
{

   return offset;
}

static int
dssetup_dissect_DsRoleAbortDownlevelServerUpgrade_response(tvbuff_t *tvb _U_, int offset _U_, packet_info *pinfo _U_, proto_tree *tree _U_, guint8 *drep _U_)
{
   offset=dissect_ntstatus(tvb, offset, pinfo, tree, drep, hf_dssetup_rc, NULL);


   return offset;
}
/* END OF INCLUDED FILE : ETH_CODE */



void
proto_register_dssetup(void)
{
        static hf_register_info hf[] = {


/* INCLUDED FILE : ETH_HFARR */
        { &hf_dssetup_opnum,
          { "Operation", "dssetup.opnum", FT_UINT16, BASE_DEC,
          NULL, 0,
         "", HFILL }},

        { &hf_dssetup_rc,
          { "Return code", "dssetup.rc", FT_UINT32, BASE_HEX,
          VALS(NT_errors), 0,
         "", HFILL }},

        { &hf_dssetup_DsRoleFlags_DS_ROLE_PRIMARY_DS_RUNNING,
          { "DS_ROLE_PRIMARY_DS_RUNNING", "dssetup.DsRoleFlags.DS_ROLE_PRIMARY_DS_RUNNING", FT_BOOLEAN, 32,
          TFS(&DS_ROLE_PRIMARY_DS_RUNNING_tfs), 0x00000001,
         "", HFILL }},

        { &hf_dssetup_DsRoleFlags_DS_ROLE_PRIMARY_DS_MIXED_MODE,
          { "DS_ROLE_PRIMARY_DS_MIXED_MODE", "dssetup.DsRoleFlags.DS_ROLE_PRIMARY_DS_MIXED_MODE", FT_BOOLEAN, 32,
          TFS(&DS_ROLE_PRIMARY_DS_MIXED_MODE_tfs), 0x00000002,
         "", HFILL }},

        { &hf_dssetup_DsRoleFlags_DS_ROLE_UPGRADE_IN_PROGRESS,
          { "DS_ROLE_UPGRADE_IN_PROGRESS", "dssetup.DsRoleFlags.DS_ROLE_UPGRADE_IN_PROGRESS", FT_BOOLEAN, 32,
          TFS(&DS_ROLE_UPGRADE_IN_PROGRESS_tfs), 0x00000004,
         "", HFILL }},

        { &hf_dssetup_DsRoleFlags_DS_ROLE_PRIMARY_DOMAIN_GUID_PRESENT,
          { "DS_ROLE_PRIMARY_DOMAIN_GUID_PRESENT", "dssetup.DsRoleFlags.DS_ROLE_PRIMARY_DOMAIN_GUID_PRESENT", FT_BOOLEAN, 32,
          TFS(&DS_ROLE_PRIMARY_DOMAIN_GUID_PRESENT_tfs), 0x01000000,
         "", HFILL }},

        { &hf_dssetup_DsRolePrimaryDomInfoBasic_role,
          { "role", "dssetup.DsRolePrimaryDomInfoBasic.role", FT_INT16, BASE_DEC,
          VALS(dssetup_DsRole_vals), 0,
         "", HFILL }},

        { &hf_dssetup_DsRolePrimaryDomInfoBasic_flags,
          { "flags", "dssetup.DsRolePrimaryDomInfoBasic.flags", FT_UINT32, BASE_HEX,
          NULL, 0,
         "", HFILL }},

        { &hf_dssetup_DsRolePrimaryDomInfoBasic_domain,
          { "domain", "dssetup.DsRolePrimaryDomInfoBasic.domain", FT_STRING, BASE_DEC,
          NULL, 0,
         "", HFILL }},

        { &hf_dssetup_DsRolePrimaryDomInfoBasic_dns_domain,
          { "dns_domain", "dssetup.DsRolePrimaryDomInfoBasic.dns_domain", FT_STRING, BASE_DEC,
          NULL, 0,
         "", HFILL }},

        { &hf_dssetup_DsRolePrimaryDomInfoBasic_forest,
          { "forest", "dssetup.DsRolePrimaryDomInfoBasic.forest", FT_STRING, BASE_DEC,
          NULL, 0,
         "", HFILL }},

        { &hf_dssetup_DsRolePrimaryDomInfoBasic_domain_guid,
          { "domain_guid", "dssetup.DsRolePrimaryDomInfoBasic.domain_guid", FT_STRING, BASE_NONE,
          NULL, 0,
         "", HFILL }},

        { &hf_dssetup_DsRoleUpgradeStatus_upgrading,
          { "upgrading", "dssetup.DsRoleUpgradeStatus.upgrading", FT_INT32, BASE_DEC,
          VALS(dssetup_DsUpgrade_vals), 0,
         "", HFILL }},

        { &hf_dssetup_DsRoleUpgradeStatus_previous_role,
          { "previous_role", "dssetup.DsRoleUpgradeStatus.previous_role", FT_INT16, BASE_DEC,
          VALS(dssetup_DsPrevious_vals), 0,
         "", HFILL }},

        { &hf_dssetup_DsRoleOpStatus_status,
          { "status", "dssetup.DsRoleOpStatus.status", FT_INT16, BASE_DEC,
          VALS(dssetup_DsRoleOp_vals), 0,
         "", HFILL }},

        { &hf_dssetup_DsRoleInfo_DS_ROLE_BASIC_INFORMATION_basic,
          { "basic", "dssetup.DsRoleInfo.basic", FT_NONE, BASE_NONE,
          NULL, 0,
         "", HFILL }},

        { &hf_dssetup_DsRoleInfo_DS_ROLE_UPGRADE_STATUS_upgrade,
          { "upgrade", "dssetup.DsRoleInfo.upgrade", FT_NONE, BASE_NONE,
          NULL, 0,
         "", HFILL }},

        { &hf_dssetup_DsRoleInfo_DS_ROLE_OP_STATUS_opstatus,
          { "opstatus", "dssetup.DsRoleInfo.opstatus", FT_NONE, BASE_NONE,
          NULL, 0,
         "", HFILL }},

        { &hf_dssetup_DsRoleGetPrimaryDomainInformation_level,
          { "level", "dssetup.DsRoleGetPrimaryDomainInformation.level", FT_INT16, BASE_DEC,
          VALS(dssetup_DsRoleInfoLevel_vals), 0,
         "", HFILL }},

        { &hf_dssetup_DsRoleGetPrimaryDomainInformation_info,
          { "info", "dssetup.DsRoleGetPrimaryDomainInformation.info", FT_UINT16, BASE_DEC,
          NULL, 0,
         "", HFILL }},

/* END OF INCLUDED FILE : ETH_HFARR */


	};

        static gint *ett[] = {


/* INCLUDED FILE : ETH_ETTARR */
        &ett_dssetup,
        &ett_dssetup_DsRoleFlags,
        &ett_dssetup_DsRolePrimaryDomInfoBasic,
        &ett_dssetup_DsRoleUpgradeStatus,
        &ett_dssetup_DsRoleOpStatus,
        &ett_dssetup_DsRoleInfo,
/* END OF INCLUDED FILE : ETH_ETTARR */


        };

        proto_dssetup = proto_register_protocol(
                "Active Directory Setup", 
		"DSSETUP", "dssetup");
	proto_register_field_array(proto_dssetup, hf, array_length(hf));
        proto_register_subtree_array(ett, array_length(ett));
}

static dcerpc_sub_dissector function_dissectors[] = {


/* INCLUDED FILE : ETH_FT */
    { 0, "DsRoleGetPrimaryDomainInformation",
        dssetup_dissect_DsRoleGetPrimaryDomainInformation_request,
        dssetup_dissect_DsRoleGetPrimaryDomainInformation_response },
    { 1, "DsRoleDnsNameToFlatName",
        dssetup_dissect_DsRoleDnsNameToFlatName_request,
        dssetup_dissect_DsRoleDnsNameToFlatName_response },
    { 2, "DsRoleDcAsDc",
        dssetup_dissect_DsRoleDcAsDc_request,
        dssetup_dissect_DsRoleDcAsDc_response },
    { 3, "DsRoleDcAsReplica",
        dssetup_dissect_DsRoleDcAsReplica_request,
        dssetup_dissect_DsRoleDcAsReplica_response },
    { 4, "DsRoleDemoteDc",
        dssetup_dissect_DsRoleDemoteDc_request,
        dssetup_dissect_DsRoleDemoteDc_response },
    { 5, "DsRoleGetDcOperationProgress",
        dssetup_dissect_DsRoleGetDcOperationProgress_request,
        dssetup_dissect_DsRoleGetDcOperationProgress_response },
    { 6, "DsRoleGetDcOperationResults",
        dssetup_dissect_DsRoleGetDcOperationResults_request,
        dssetup_dissect_DsRoleGetDcOperationResults_response },
    { 7, "DsRoleCancel",
        dssetup_dissect_DsRoleCancel_request,
        dssetup_dissect_DsRoleCancel_response },
    { 8, "DsRoleServerSaveStateForUpgrade",
        dssetup_dissect_DsRoleServerSaveStateForUpgrade_request,
        dssetup_dissect_DsRoleServerSaveStateForUpgrade_response },
    { 9, "DsRoleUpgradeDownlevelServer",
        dssetup_dissect_DsRoleUpgradeDownlevelServer_request,
        dssetup_dissect_DsRoleUpgradeDownlevelServer_response },
    { 10, "DsRoleAbortDownlevelServerUpgrade",
        dssetup_dissect_DsRoleAbortDownlevelServerUpgrade_request,
        dssetup_dissect_DsRoleAbortDownlevelServerUpgrade_response },
/* END OF INCLUDED FILE : ETH_FT */


	{ 0, NULL, NULL, NULL },
};

void
proto_reg_handoff_dssetup(void)
{


/* INCLUDED FILE : ETH_HANDOFF */
    dcerpc_init_uuid(proto_dssetup, ett_dssetup,
        &uuid_dcerpc_dssetup, ver_dssetup,
        function_dissectors, hf_dssetup_opnum);
/* END OF INCLUDED FILE : ETH_HANDOFF */


}

