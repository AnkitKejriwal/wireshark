/* DO NOT EDIT
 * This dissector is autogenerated
 *   ronnie sahlberg 2005
 * Autogenerated based on the IDL definitions from samba4
 */
/* packet-dcerpc-dssetup.h
 * Routines for DSSETUP packet disassembly
 *
 * $Id: packet-dcerpc-dssetup.h 11410 2004-07-18 18:06:47Z gram $
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

#ifndef __PACKET_DCERPC_DSSETUP_H
#define __PACKET_DCERPC_DSSETUP_H



/* INCLUDED FILE : ETH_HDR */

#define DS_ROLE_STANDALONE_WORKSTATION		0
#define DS_ROLE_MEMBER_WORKSTATION		1
#define DS_ROLE_STANDALONE_SERVER		2
#define DS_ROLE_MEMBER_SERVER		3
#define DS_ROLE_BACKUP_DC		4
#define DS_ROLE_PRIMARY_DC		5

extern const value_string dssetup_DsRole_vals[];
int dssetup_dissect_DsRole(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep, int hf_index, guint32 param);
int dssetup_dissect_DsRoleFlags(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep, int hf_index, guint32 param);
int dssetup_dissect_DsRolePrimaryDomInfoBasic(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep, int hf_index, guint32 param);

#define DS_ROLE_NOT_UPGRADING		0
#define DS_ROLE_UPGRADING		1

extern const value_string dssetup_DsUpgrade_vals[];
int dssetup_dissect_DsUpgrade(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep, int hf_index, guint32 param);

#define DS_ROLE_PREVIOUS_UNKNOWN		0
#define DS_ROLE_PREVIOUS_PRIMARY		1
#define DS_ROLE_PREVIOUS_BACKUP		2

extern const value_string dssetup_DsPrevious_vals[];
int dssetup_dissect_DsPrevious(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep, int hf_index, guint32 param);
int dssetup_dissect_DsRoleUpgradeStatus(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep, int hf_index, guint32 param);

#define DS_ROLE_OP_IDLE		0
#define DS_ROLE_OP_ACTIVE		1
#define DS_ROLE_OP_NEEDS_REBOOT		2

extern const value_string dssetup_DsRoleOp_vals[];
int dssetup_dissect_DsRoleOp(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep, int hf_index, guint32 param);
int dssetup_dissect_DsRoleOpStatus(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep, int hf_index, guint32 param);

#define DS_ROLE_BASIC_INFORMATION		1
#define DS_ROLE_UPGRADE_STATUS		2
#define DS_ROLE_OP_STATUS		3

extern const value_string dssetup_DsRoleInfoLevel_vals[];
int dssetup_dissect_DsRoleInfoLevel(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep, int hf_index, guint32 param);
/* END OF INCLUDED FILE : ETH_HDR */



#endif /* packet-dcerpc-dssetup.h */
