/* packet-dcerpc-rs_misc.c
 *
 * Routines for dcerpc RS-MISC
 * Copyright 2002, Jaime Fournier <jafour1@yahoo.com>
 * This information is based off the released idl files from opengroup.
 * ftp://ftp.opengroup.org/pub/dce122/dce/src/security.tar.gz security/idl/rs_misc.idl
 *      
 * $Id: packet-dcerpc-rs_misc.c,v 1.4 2003/06/26 04:30:29 tpot Exp $
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


#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include <string.h>

#include <glib.h>
#include <epan/packet.h>
#include "packet-dcerpc.h"


static int proto_rs_misc = -1;
static int hf_rs_misc_opnum = -1;
static int hf_rs_misc_login_get_info_rqst_var = -1;
static int hf_rs_misc_login_get_info_rqst_key_size = -1;
static int hf_rs_misc_login_get_info_rqst_key_t = -1;


static gint ett_rs_misc = -1;


static e_uuid_t uuid_rs_misc = { 0x4c878280, 0x5000, 0x0000, { 0x0d, 0x00, 0x02, 0x87, 0x14, 0x00, 0x00, 0x00 } };
static guint16  ver_rs_misc = 1;


static int
rs_misc_dissect_login_get_info_rqst (tvbuff_t *tvb, int offset,
	packet_info *pinfo, proto_tree *tree, char *drep)
{

	guint32 key_size;
	const char *key_t = NULL;

	offset = dissect_ndr_uint32 (tvb, offset, pinfo, tree, drep, 
			hf_rs_misc_login_get_info_rqst_var, NULL);
	offset = dissect_ndr_uint32 (tvb, offset, pinfo, tree, drep,
			hf_rs_misc_login_get_info_rqst_key_size, &key_size);

	if (key_size){ /* Not able to yet decipher the OTHER versions of this call just yet. */

		proto_tree_add_string (tree, hf_rs_misc_login_get_info_rqst_key_t, tvb, offset, hf_rs_misc_login_get_info_rqst_key_size, tvb_get_ptr (tvb, offset, key_size));
		key_t = (const char *)tvb_get_ptr(tvb,offset,key_size);
		offset += key_size;

		if (check_col(pinfo->cinfo, COL_INFO)) {
			col_append_fstr(pinfo->cinfo, COL_INFO,
				"rs_login_get_info Request for: %s ", key_t);
		}
	} else {
		if (check_col(pinfo->cinfo, COL_INFO)) {
			col_append_str(pinfo->cinfo, COL_INFO, 
				"rs_login_get_info Request (other)");
		}
	}

	return offset;
};


static dcerpc_sub_dissector rs_misc_dissectors[] = {
	{ 0, "rs_login_get_info", rs_misc_dissect_login_get_info_rqst, NULL},
	{ 1, "rs_wait_until_consistent", NULL, NULL},
	{ 2, "rs_check_consistency", NULL, NULL},
	{ 0, NULL, NULL, NULL }
};

void
proto_register_rs_misc (void)
{
	static hf_register_info hf[] = {
	{ &hf_rs_misc_opnum,
		{ "Operation", "rs_misc.opnum", FT_UINT16, BASE_DEC,
		NULL, 0x0, "Operation", HFILL }},
	{ &hf_rs_misc_login_get_info_rqst_var,
		{ "Var", "rs_misc.login_get_info_rqst_var", FT_UINT32, BASE_DEC,
		NULL, 0x0, "", HFILL }},
	{ &hf_rs_misc_login_get_info_rqst_key_size,
		{ "Key Size", "rs_misc.login_get_info_rqst_key_size", FT_UINT32, BASE_DEC,
		NULL, 0x0, "", HFILL }},
	{ &hf_rs_misc_login_get_info_rqst_key_t,
		{ "Key", "rs.misc_login_get_info_rqst_key_t", FT_STRING, BASE_NONE,
		NULL, 0x0, "", HFILL }}
	};

	static gint *ett[] = {
		&ett_rs_misc,
	};
	proto_rs_misc = proto_register_protocol ("DCE/RPC RS_MISC", "rs_misc", "rs_misc");
	proto_register_field_array (proto_rs_misc, hf, array_length (hf));
	proto_register_subtree_array (ett, array_length (ett));
}

void
proto_reg_handoff_rs_misc (void)
{
	header_field_info *hf_info;

	/* Register the protocol as dcerpc */
	dcerpc_init_uuid (proto_rs_misc, ett_rs_misc, &uuid_rs_misc, ver_rs_misc, rs_misc_dissectors, hf_rs_misc_opnum);

	/* Set opnum strings from subdissector list */

	hf_info = proto_registrar_get_nth(hf_rs_misc_opnum);
	hf_info->strings = value_string_from_subdissectors(
		rs_misc_dissectors, array_length(rs_misc_dissectors));

}
