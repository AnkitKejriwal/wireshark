/* packet-dcerpc-wkssvc.c
 * Routines for SMB \\PIPE\\wkssvc packet disassembly
 * Copyright 2001, Tim Potter <tpot@samba.org>
 * Copyright 2003, Richard Sharpe <rsharpe@richardsharpe.com>
 *
 * $Id: packet-dcerpc-wkssvc.c,v 1.15 2003/04/30 17:45:04 sharpe Exp $
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
#include <epan/packet.h>
#include "packet-dcerpc.h"
#include "packet-dcerpc-wkssvc.h"
#include "packet-dcerpc-nt.h"
#include "smb.h"

static int proto_dcerpc_wkssvc = -1;
static int hf_wkssvc_server = -1;
static int hf_wkssvc_info_level = -1;
static int hf_wkssvc_platform_id = -1; 
static int hf_wkssvc_net_group = -1;
static int hf_wkssvc_ver_major = -1;
static int hf_wkssvc_ver_minor = -1;
static int hf_wkssvc_lan_root = -1;
static int hf_wkssvc_rc = -1;
static int hf_wkssvc_logged_on_users = -1;
static int hf_wkssvc_pref_max = -1;
static int hf_wkssvc_enum_handle = -1;
static int hf_wkssvc_junk = -1;
static int hf_wkssvc_user_name = -1;
static int hf_wkssvc_num_entries = -1;
static int hf_wkssvc_logon_domain = -1;
static int hf_wkssvc_other_domains = -1;
static int hf_wkssvc_logon_server = -1;
static int hf_wkssvc_entries_read = -1;
static int hf_wkssvc_total_entries = -1;
static gint ett_dcerpc_wkssvc = -1;

extern const value_string platform_id_vals[];

static e_uuid_t uuid_dcerpc_wkssvc = {
        0x6bffd098, 0xa112, 0x3610,
        { 0x98, 0x33, 0x46, 0xc3, 0xf8, 0x7e, 0x34, 0x5a }
};

static int
wkssvc_dissect_ENUM_HANDLE(tvbuff_t *tvb, int offset,
			   packet_info *pinfo, proto_tree *tree,
			   char *drep)
{

  offset = dissect_ndr_uint32(tvb, offset, pinfo, tree, drep,
			      hf_wkssvc_enum_handle, 0);
  return offset;

}

static guint16 ver_dcerpc_wkssvc = 1;

/*
 * IDL typedef struct {
 * IDL   long platform_id;
 * IDL   [string] [unique] wchar_t *server;
 * IDL   [string] [unique] wchar_t *lan_grp;
 * IDL   long ver_major;
 * IDL   long ver_minor;
 * IDL } WKS_INFO_100;
 */
static int
wkssvc_dissect_WKS_INFO_100(tvbuff_t *tvb, int offset,
			    packet_info *pinfo, proto_tree *tree,
			    char *drep)
{
	offset = dissect_ndr_uint32(tvb, offset, pinfo, tree, drep,
			hf_wkssvc_platform_id, NULL);

        offset = dissect_ndr_str_pointer_item(tvb, offset, pinfo, tree, drep,
			NDR_POINTER_UNIQUE, "Server", hf_wkssvc_server, 0);

        offset = dissect_ndr_str_pointer_item(tvb, offset, pinfo, tree, drep,
			NDR_POINTER_UNIQUE, "Net Group", hf_wkssvc_net_group, 0);

	offset = dissect_ndr_uint32(tvb, offset, pinfo, tree, drep,
			hf_wkssvc_ver_major, NULL);

	offset = dissect_ndr_uint32(tvb, offset, pinfo, tree, drep,
			hf_wkssvc_ver_minor, NULL);

	return offset;
}

/*
 * IDL typedef struct {
 * IDL   long platform_id;
 * IDL   [string] [unique] wchar_t *server;
 * IDL   [string] [unique] wchar_t *lan_grp;
 * IDL   long ver_major;
 * IDL   long ver_minor;
 * IDL   [string] [unique] wchar_t *lan_root;
 * IDL } WKS_INFO_101;
 */
static int
wkssvc_dissect_WKS_INFO_101(tvbuff_t *tvb, int offset,
			    packet_info *pinfo, proto_tree *tree,
			    char *drep)
{
	offset = dissect_ndr_uint32(tvb, offset, pinfo, tree, drep,
			hf_wkssvc_platform_id, NULL);

        offset = dissect_ndr_str_pointer_item(tvb, offset, pinfo, tree, drep,
			NDR_POINTER_UNIQUE, "Server", hf_wkssvc_server, 0);

        offset = dissect_ndr_str_pointer_item(tvb, offset, pinfo, tree, drep,
			NDR_POINTER_UNIQUE, "Net Group", hf_wkssvc_net_group, 0);

	offset = dissect_ndr_uint32(tvb, offset, pinfo, tree, drep,
			hf_wkssvc_ver_major, NULL);

	offset = dissect_ndr_uint32(tvb, offset, pinfo, tree, drep,
			hf_wkssvc_ver_minor, NULL);

        offset = dissect_ndr_str_pointer_item(tvb, offset, pinfo, tree, drep,
			NDR_POINTER_UNIQUE, "Lan Root", hf_wkssvc_lan_root, 0);

	return offset;
}

/*
 * IDL typedef struct {
 * IDL   long platform_id;
 * IDL   [string] [unique] wchar_t *server;
 * IDL   [string] [unique] wchar_t *lan_grp;
 * IDL   long ver_major;
 * IDL   long ver_minor;
 * IDL   [string] [unique] wchar_t *lan_root;
 * IDL   long logged_on_users;
 * IDL } WKS_INFO_102;
 */
static int
wkssvc_dissect_WKS_INFO_102(tvbuff_t *tvb, int offset,
			    packet_info *pinfo, proto_tree *tree,
			    char *drep)
{
	offset = dissect_ndr_uint32(tvb, offset, pinfo, tree, drep,
			hf_wkssvc_platform_id, NULL);

        offset = dissect_ndr_str_pointer_item(tvb, offset, pinfo, tree, drep,
			NDR_POINTER_UNIQUE, "Server", hf_wkssvc_server, 0);

        offset = dissect_ndr_str_pointer_item(tvb, offset, pinfo, tree, drep,
			NDR_POINTER_UNIQUE, "Net Group", hf_wkssvc_net_group, 0);

	offset = dissect_ndr_uint32(tvb, offset, pinfo, tree, drep,
			hf_wkssvc_ver_major, NULL);

	offset = dissect_ndr_uint32(tvb, offset, pinfo, tree, drep,
			hf_wkssvc_ver_minor, NULL);

        offset = dissect_ndr_str_pointer_item(tvb, offset, pinfo, tree, drep,
			NDR_POINTER_UNIQUE, "Lan Root", hf_wkssvc_lan_root, 0);

	offset = dissect_ndr_uint32(tvb, offset, pinfo, tree, drep,
			hf_wkssvc_logged_on_users, NULL);

	return offset;
}

/*
 * IDL long NetWkstaGetInfo(
 * IDL      [in] [string] [unique] wchar_t *ServerName,
 * IDL      [in] long level,
 * IDL      [out] [ref] WKS_INFO_UNION *wks
 * IDL );
 */
static int
wkssvc_dissect_netwkstagetinfo_rqst(tvbuff_t *tvb, int offset, 
				    packet_info *pinfo, proto_tree *tree,
				    char *drep)
{
        offset = dissect_ndr_str_pointer_item(tvb, offset, pinfo, tree, drep,
					      NDR_POINTER_UNIQUE, "Server", 
					      hf_wkssvc_server, 0);

	offset = dissect_ndr_uint32(tvb, offset, pinfo, tree, drep,
				    hf_wkssvc_info_level, 0);

	return offset;

}

/*
 * IDL typedef [switch_type(long)] union {
 * IDL   [case(100)] [unique] WKS_INFO_100 *wks100;
 * IDL   [case(101)] [unique] WKS_INFO_101 *wks101;
 * IDL   [case(102)] [unique] WKS_INFO_102 *wks102;
 * IDL } WKS_INFO_UNION;
 */
static int
wkssvc_dissect_WKS_GETINFO_UNION(tvbuff_t *tvb, int offset,
				 packet_info *pinfo, proto_tree *tree,
				 char *drep)
{
	guint32 level;

	ALIGN_TO_4_BYTES;

	offset = dissect_ndr_uint32(tvb, offset, pinfo, tree, drep, hf_wkssvc_info_level, &level);

	switch(level){
	case 100:
		offset = dissect_ndr_pointer(tvb, offset, pinfo, tree, drep,
			wkssvc_dissect_WKS_INFO_100,
			NDR_POINTER_UNIQUE, "WKS_INFO_100:", -1);
		break;

	case 101:
		offset = dissect_ndr_pointer(tvb, offset, pinfo, tree, drep,
			wkssvc_dissect_WKS_INFO_101,
			NDR_POINTER_UNIQUE, "WKS_INFO_101:", -1);
		break;

	case 102:
		offset = dissect_ndr_pointer(tvb, offset, pinfo, tree, drep,
			wkssvc_dissect_WKS_INFO_102,
			NDR_POINTER_UNIQUE, "WKS_INFO_101:", -1);
		break;

	}

	return offset;

}

static int wkssvc_dissect_netwkstagetinfo_reply(tvbuff_t *tvb, int offset,
						packet_info *pinfo, 
						proto_tree *tree,
						char *drep)
{
	offset = dissect_ndr_pointer(tvb, offset, pinfo, tree, drep,
			wkssvc_dissect_WKS_GETINFO_UNION,
			NDR_POINTER_REF, "Server Info", -1);

	offset = dissect_doserror(tvb, offset, pinfo, tree, drep,
			hf_wkssvc_rc, NULL);

	return offset;
}

/*
 * IDL typedef struct {
 * IDL   [string] [unique] wchar_t *dev;
 * IDL } USER_INFO_0;
 */
static int
wkssvc_dissect_USER_INFO_0(tvbuff_t *tvb, int offset,
				     packet_info *pinfo, proto_tree *tree,
				     char *drep)
{
        offset = dissect_ndr_str_pointer_item(tvb, offset, pinfo, tree, drep,
			NDR_POINTER_UNIQUE, "User Name", 
			hf_wkssvc_user_name, 0);

	return offset;
}

static int
wkssvc_dissect_USER_INFO_0_array(tvbuff_t *tvb, int offset,
				     packet_info *pinfo, proto_tree *tree,
				     char *drep)
{
	offset = dissect_ndr_ucarray(tvb, offset, pinfo, tree, drep,
			wkssvc_dissect_USER_INFO_0);

	return offset;
}

/*
 * IDL typedef struct {
 * IDL   long EntriesRead;
 * IDL   [size_is(EntriesRead)] [unique] USER_INFO_0 *devs;
 * IDL } USER_INFO_0_CONTAINER;
 */
static int
wkssvc_dissect_USER_INFO_0_CONTAINER(tvbuff_t *tvb, int offset,
				     packet_info *pinfo, proto_tree *tree,
				     char *drep)
{
	offset = dissect_ndr_uint32(tvb, offset, pinfo, tree, drep,
		hf_wkssvc_num_entries, NULL);

	offset = dissect_ndr_pointer(tvb, offset, pinfo, tree, drep,
		wkssvc_dissect_USER_INFO_0_array, NDR_POINTER_UNIQUE,
		"USER_INFO_0 array:", -1);

	return offset;
}

/*
 * IDL typedef struct {
 * IDL   [string] [unique] wchar_t *user_name;
 * IDL   [string] [unique] wchar_t *logon_domain;
 * IDL   [string] [unique] wchar_t *other_domains;
 * IDL   [string] [unique] wchar_t *logon_server;
 * IDL } USER_INFO_1;
 */
static int
wkssvc_dissect_USER_INFO_1(tvbuff_t *tvb, int offset,
				     packet_info *pinfo, proto_tree *tree,
				     char *drep)
{
        offset = dissect_ndr_str_pointer_item(tvb, offset, pinfo, tree, drep,
					      NDR_POINTER_UNIQUE, "User Name", 
					      hf_wkssvc_user_name, 0);

        offset = dissect_ndr_str_pointer_item(tvb, offset, pinfo, tree, drep,
			NDR_POINTER_UNIQUE, "Logon Domain", 
					      hf_wkssvc_logon_domain, 0);

        offset = dissect_ndr_str_pointer_item(tvb, offset, pinfo, tree, drep,
			NDR_POINTER_UNIQUE, "Other Domains", 
					      hf_wkssvc_other_domains, 0);

        offset = dissect_ndr_str_pointer_item(tvb, offset, pinfo, tree, drep,
			NDR_POINTER_UNIQUE, "Logon Server", 
					      hf_wkssvc_logon_server, 0);


	return offset;
}
static int
wkssvc_dissect_USER_INFO_1_array(tvbuff_t *tvb, int offset,
				     packet_info *pinfo, proto_tree *tree,
				     char *drep)
{
	offset = dissect_ndr_ucarray(tvb, offset, pinfo, tree, drep,
			wkssvc_dissect_USER_INFO_1);

	return offset;
}

/*
 * IDL typedef struct {
 * IDL   long EntriesRead;
 * IDL   [size_is(EntriesRead)] [unique] USER_INFO_1 *devs;
 * IDL } USER_INFO_1_CONTAINER;
 */
static int
wkssvc_dissect_USER_INFO_1_CONTAINER(tvbuff_t *tvb, int offset,
				     packet_info *pinfo, proto_tree *tree,
				     char *drep)
{
	offset = dissect_ndr_uint32(tvb, offset, pinfo, tree, drep,
		hf_wkssvc_num_entries, NULL);

	offset = dissect_ndr_pointer(tvb, offset, pinfo, tree, drep,
		wkssvc_dissect_USER_INFO_1_array, NDR_POINTER_UNIQUE,
		"USER_INFO_1 array:", -1);

	return offset;
}

/*
 * IDL typedef [switch_type(long)] union {
 * IDL   [case(0)] [unique] USER_INFO_0_CONTAINER *dev0;
 * IDL   [case(1)] [unique] USER_INFO_1_CONTAINER *dev1;
 * IDL } CHARDEV_ENUM_UNION;
 */
static int
wkssvc_dissect_USER_ENUM_UNION(tvbuff_t *tvb, int offset,
				     packet_info *pinfo, proto_tree *tree,
				     char *drep)
{
	guint32 level;

	ALIGN_TO_4_BYTES;

	offset = dissect_ndr_uint32(tvb, offset, pinfo, tree, drep, hf_wkssvc_info_level, &level);

	switch(level){
	case 0:
		offset = dissect_ndr_pointer(tvb, offset, pinfo, tree, drep,
			wkssvc_dissect_USER_INFO_0_CONTAINER,
			NDR_POINTER_UNIQUE, "USER_INFO_0_CONTAINER:", -1);
		break;
	case 1:
		offset = dissect_ndr_pointer(tvb, offset, pinfo, tree, drep,
			wkssvc_dissect_USER_INFO_1_CONTAINER,
			NDR_POINTER_UNIQUE, "USER_INFO_1_CONTAINER:", -1);
		break;
	}

	return offset;
}

/*
 * IDL long NetWkstaEnumUsers(
 * IDL      [in] [string] [unique] wchar_t *ServerName,
 * IDL      [in] long level,
 * IDL      [in] [out] [ref] WKS_USER_ENUM_STRUCT *users,
 * IDL      [in] long prefmaxlen,
 * IDL      [out] long *entriesread,
 * IDL      [out] long *totalentries,
 * IDL      [in] [out] [ref] *resumehandle
 * IDL );
 */
static int
wkssvc_dissect_netwkstaenumusers_rqst(tvbuff_t *tvb, int offset, 
				      packet_info *pinfo, proto_tree *tree,
				      char *drep)
{
        offset = dissect_ndr_str_pointer_item(tvb, offset, pinfo, tree, drep,
					      NDR_POINTER_UNIQUE, "Server", 
					      hf_wkssvc_server, 0);

	offset = dissect_ndr_uint32(tvb, offset, pinfo, tree, drep,
				    hf_wkssvc_info_level, 0);

  	offset = dissect_ndr_pointer(tvb, offset, pinfo, tree, drep,
			wkssvc_dissect_USER_ENUM_UNION,
			NDR_POINTER_REF, "User Info", -1);
	/* Seems to be junk here ... */
	/*	offset = dissect_ndr_uint32(tvb, offset, pinfo, tree, drep, 
				    hf_wkssvc_junk, 0);

	offset = dissect_ndr_pointer(tvb, offset, pinfo, tree, drep, 
				     wkssvc_dissect_ENUM_HANDLE,
				     NDR_POINTER_UNIQUE, "Junk Handle", -1);

	offset = dissect_ndr_uint32(tvb, offset, pinfo, tree, drep, 
	hf_wkssvc_junk, 0); */

	offset = dissect_ndr_uint32(tvb, offset, pinfo, tree, drep,
				    hf_wkssvc_pref_max, 0);

	offset = dissect_ndr_pointer(tvb, offset, pinfo, tree, drep, 
				     wkssvc_dissect_ENUM_HANDLE,
				     NDR_POINTER_UNIQUE, "Enum Handle", -1);

	return offset;

}

static int wkssvc_dissect_netwkstaenumusers_reply(tvbuff_t *tvb, int offset,
						  packet_info *pinfo, 
						  proto_tree *tree,
						  char *drep)
{
        /* There seems to be an info level there first */
	offset = dissect_ndr_uint32(tvb, offset, pinfo, tree, drep,
				    hf_wkssvc_info_level, 0);

  	offset = dissect_ndr_pointer(tvb, offset, pinfo, tree, drep,
			wkssvc_dissect_USER_ENUM_UNION,
			NDR_POINTER_REF, "User Info", -1);

	/* Entries read seems to be in the enum array ... */

	offset = dissect_ndr_uint32(tvb, offset, pinfo, tree, drep,
				    hf_wkssvc_total_entries, 0);

	offset = dissect_ndr_pointer(tvb, offset, pinfo, tree, drep, 
				     wkssvc_dissect_ENUM_HANDLE,
				     NDR_POINTER_UNIQUE, "Enum Handle", -1);

	offset = dissect_doserror(tvb, offset, pinfo, tree, drep,
	hf_wkssvc_rc, NULL);

	return offset;
}

static dcerpc_sub_dissector dcerpc_wkssvc_dissectors[] = {
        { WKS_NetWkstaGetInfo, "NetWkstaGetInfo", 
	  wkssvc_dissect_netwkstagetinfo_rqst, 
	  wkssvc_dissect_netwkstagetinfo_reply},
        { WKS_NetWkstaEnumUsers, "NetWkstaEnumUsers",
	  wkssvc_dissect_netwkstaenumusers_rqst,
 	  wkssvc_dissect_netwkstaenumusers_reply},
        {0, NULL, NULL,  NULL }
};

void
proto_register_dcerpc_wkssvc(void)
{
        static hf_register_info hf[] = { 
	  { &hf_wkssvc_server,
	    { "Server", "wkssvc.server", FT_STRING, BASE_NONE,
	      NULL, 0x0, "Server Name", HFILL}},
	  { &hf_wkssvc_net_group,
	    { "Net Group", "wkssvc.netgrp", FT_STRING, BASE_NONE,
	      NULL, 0x0, "Net Group", HFILL}},
	  { &hf_wkssvc_info_level,
	    { "Info Level", "wkssvc.info_level", FT_UINT32,
	      BASE_DEC, NULL, 0x0, "Info Level", HFILL}},
	  { &hf_wkssvc_platform_id,
	    { "Platform ID", "wkssvc.info.platform_id", FT_UINT32,
	      BASE_DEC, VALS(platform_id_vals), 0x0, "Platform ID", HFILL}},
	  { &hf_wkssvc_ver_major,
	    { "Major Version", "wkssvc.version.major", FT_UINT32,
	      BASE_DEC, NULL, 0x0, "Major Version", HFILL}},
	  { &hf_wkssvc_ver_minor,
	    { "Minor Version", "wkssvc.version.minor", FT_UINT32,
	      BASE_DEC, NULL, 0x0, "Minor Version", HFILL}},
	  { &hf_wkssvc_lan_root,
	    { "Lan Root", "wkssvc.lan.root", FT_STRING, BASE_NONE,
	      NULL, 0x0, "Lan Root", HFILL}},
	  { &hf_wkssvc_logged_on_users,
	    { "Logged On Users", "wkssvc.logged.on.users", FT_UINT32,
	      BASE_DEC, NULL, 0x0, "Logged On Users", HFILL}},
	  { &hf_wkssvc_pref_max, 
	    { "Preferred Max Len", "wkssvc.pref.max.len", FT_INT32,
	      BASE_DEC, NULL, 0x0, "Preferred Max Len", HFILL}},
	  { &hf_wkssvc_junk, 
	    { "Junk", "wkssvc.junk", FT_UINT32,
	      BASE_DEC, NULL, 0x0, "Junk", HFILL}},
	  { &hf_wkssvc_enum_handle,
	    { "Enumeration handle", "wkssvc.enum_hnd", FT_BYTES,
	      BASE_HEX, NULL, 0x0, "Enumeration Handle", HFILL}},
	  { &hf_wkssvc_user_name,
	    { "User Name", "wkssvc.user.name", FT_STRING, BASE_NONE,
	      NULL, 0x0, "User Name", HFILL}},
	  { &hf_wkssvc_logon_domain,
	    { "Logon Domain", "wkssvc.logon.domain", FT_STRING, BASE_NONE,
	      NULL, 0x0, "Logon Domain", HFILL}},
	  { &hf_wkssvc_other_domains,
	    { "Other Domains", "wkssvc.other.domains", FT_STRING, BASE_NONE,
	      NULL, 0x0, "Other Domains", HFILL}},
	  { &hf_wkssvc_logon_server,
	    { "Logon Server", "wkssvc.logon.server", FT_STRING, BASE_NONE,
	      NULL, 0x0, "Logon Server", HFILL}},
	  { &hf_wkssvc_rc,
	    { "Return code", "srvsvc.rc", FT_UINT32,
	      BASE_HEX, VALS(DOS_errors), 0x0, "Return Code", HFILL}},
	  { &hf_wkssvc_num_entries, 
	    { "Num Entries", "wkssvc.num.entries", FT_INT32,
	      BASE_DEC, NULL, 0x0, "Num Entries", HFILL}},
	  { &hf_wkssvc_entries_read, 
	    { "Entries Read", "wkssvc.entries.read", FT_INT32,
	      BASE_DEC, NULL, 0x0, "Entries Read", HFILL}},
	  { &hf_wkssvc_total_entries, 
	    { "Total Entries", "wkssvc.total.entries", FT_INT32,
	      BASE_DEC, NULL, 0x0, "Total Entries", HFILL}},

	};
        static gint *ett[] = {
                &ett_dcerpc_wkssvc
        };

        proto_dcerpc_wkssvc = proto_register_protocol(
                "Microsoft Workstation Service", "WKSSVC", "wkssvc");

	proto_register_field_array(proto_dcerpc_wkssvc, hf, array_length(hf));
        proto_register_subtree_array(ett, array_length(ett));
}

void
proto_reg_handoff_dcerpc_wkssvc(void)
{
        /* Register protocol as dcerpc */

        dcerpc_init_uuid(proto_dcerpc_wkssvc, ett_dcerpc_wkssvc,
                         &uuid_dcerpc_wkssvc, ver_dcerpc_wkssvc,
                         dcerpc_wkssvc_dissectors, -1);
}
