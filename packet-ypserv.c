/* packet-ypserv.c
 * Routines for ypserv dissection
 *
 * $Id: packet-ypserv.c,v 1.14 2001/01/28 03:39:48 guy Exp $
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@zing.org>
 * Copyright 1998 Gerald Combs
 *
 * Copied from packet-smb.c
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


#include "packet-rpc.h"
#include "packet-ypserv.h"

static int proto_ypserv = -1;
static int hf_ypserv_domain = -1;
static int hf_ypserv_servesdomain = -1;
static int hf_ypserv_map = -1;
static int hf_ypserv_key = -1;
static int hf_ypserv_value = -1;
static int hf_ypserv_status = -1;

static gint ett_ypserv = -1;

/* Dissect a domain call */
int dissect_domain_call(const u_char *pd, int offset, frame_data *fd,
	proto_tree *tree)
{
	if ( tree )
	{
		offset = dissect_rpc_string(pd,offset,fd,tree,hf_ypserv_domain,NULL);
	}
	
	return offset;
}

int dissect_domain_reply(const u_char *pd, int offset, frame_data *fd,
	proto_tree *tree)
{
	if ( !BYTES_ARE_IN_FRAME(offset, 4)) return offset;

	if ( tree )
	{
		proto_tree_add_boolean(tree, hf_ypserv_servesdomain, NullTVB,
			offset, 4, pntohl(&pd[offset]));
	}

	offset += 4;	
	return offset;
}

/* Dissect a next call */
int dissect_next_call(const u_char *pd, int offset, frame_data *fd,
	proto_tree *tree)
{
	if ( tree )
	{
		offset = dissect_rpc_string(pd,offset,fd,tree,hf_ypserv_domain,NULL);
		offset = dissect_rpc_string(pd,offset,fd,tree,hf_ypserv_map,NULL);
		offset = dissect_rpc_string(pd,offset,fd,tree,hf_ypserv_key,NULL);
	}
	
	return offset;
}

int dissect_first_call(const u_char *pd, int offset, frame_data *fd,
	proto_tree *tree)
{
	if ( tree )
	{
		offset = dissect_rpc_string(pd,offset,fd,tree,hf_ypserv_domain,NULL);
		offset = dissect_rpc_string(pd,offset,fd,tree,hf_ypserv_map,NULL);
	}
	
	return offset;
}

int dissect_match_call(const u_char *pd, int offset, frame_data *fd,
	proto_tree *tree)
{
	if ( tree )
	{
		offset = dissect_rpc_string(pd,offset,fd,tree,hf_ypserv_domain,NULL);
		offset = dissect_rpc_string(pd,offset,fd,tree,hf_ypserv_map,NULL);
		offset = dissect_rpc_string(pd,offset,fd,tree,hf_ypserv_key,NULL);
	}
	
	return offset;
}

int dissect_match_reply(const u_char *pd, int offset, frame_data *fd,
	proto_tree *tree)
{
	if ( !BYTES_ARE_IN_FRAME(offset, 4)) return offset;

	if ( tree )
	{
		proto_tree_add_boolean(tree, hf_ypserv_status, NullTVB,
			offset, 4, pntohl(&pd[offset]));
		offset += 4;

		offset = dissect_rpc_string(pd,offset,fd,tree,hf_ypserv_value,NULL);
	}
	
	return offset;
}

int dissect_firstnext_reply(const u_char *pd, int offset, frame_data *fd,
	proto_tree *tree)
{
	if ( !BYTES_ARE_IN_FRAME(offset, 4)) return offset;

	if ( tree )
	{
		proto_tree_add_boolean(tree, hf_ypserv_status, NullTVB,
			offset, 4, pntohl(&pd[offset]));
		offset += 4;

		offset = dissect_rpc_string(pd,offset,fd,tree,hf_ypserv_value,NULL);
		offset = dissect_rpc_string(pd,offset,fd,tree,hf_ypserv_key,NULL);
	}
	
	return offset;
}


/* proc number, "proc name", dissect_request, dissect_reply */
/* NULL as function pointer means: type of arguments is "void". */

/* someone please get me a version 1 trace */
static const old_vsff ypserv1_proc[] = {
    { 0, "NULL", NULL, NULL },
    { YPPROC_ALL,   "ALL",      
		NULL, NULL },
    { YPPROC_CLEAR, "CLEAR",        
		NULL, NULL },
    { YPPROC_DOMAIN, "DOMAIN",
		NULL, NULL },
    { YPPROC_DOMAIN_NONACK, "DOMAIN_NONACK",
		NULL, NULL },
    { YPPROC_FIRST, "FIRST",        
		NULL, NULL },
    { YPPROC_MAPLIST,   "MAPLIST",      
		NULL, NULL },
    { YPPROC_MASTER,    "MASTER",       
		NULL, NULL },
    { YPPROC_MATCH, "MATCH",        
		NULL, NULL },
    { YPPROC_NEXT,  "NEXT",     
		NULL, NULL },
    { YPPROC_ORDER, "ORDER",        
		NULL, NULL },
    { YPPROC_XFR,   "XFR",      
		NULL, NULL },
    { 0, NULL, NULL, NULL }
};
/* end of YPServ version 2 */

static const old_vsff ypserv2_proc[] = {
    { 0, "NULL", NULL, NULL },
    { YPPROC_ALL,   "ALL",      
		NULL, NULL },
    { YPPROC_CLEAR, "CLEAR",        
		NULL, NULL },
    { YPPROC_DOMAIN, "DOMAIN",
		dissect_domain_call, dissect_domain_reply },
    { YPPROC_DOMAIN_NONACK, "DOMAIN_NONACK",
		dissect_domain_call, dissect_domain_reply },
    { YPPROC_FIRST, "FIRST",        
		dissect_first_call, dissect_firstnext_reply },
    { YPPROC_MAPLIST,   "MAPLIST",      
		NULL, NULL },
    { YPPROC_MASTER,    "MASTER",       
		NULL, NULL },
    { YPPROC_MATCH, "MATCH",        
		dissect_match_call, dissect_match_reply },
    { YPPROC_NEXT,  "NEXT",     
		dissect_next_call, dissect_firstnext_reply },
    { YPPROC_ORDER, "ORDER",        
		NULL, NULL },
    { YPPROC_XFR,   "XFR",      
		NULL, NULL },
    { 0, NULL, NULL, NULL }
};
/* end of YPServ version 2 */


void
proto_register_ypserv(void)
{
	static struct true_false_string okfailed = { "Ok", "Failed" };
	static struct true_false_string yesno = { "Yes", "No" };
		
	static hf_register_info hf[] = {
		{ &hf_ypserv_domain, {
			"Domain", "ypserv.domain", FT_STRING, BASE_DEC,
			NULL, 0, "Domain" }},
		{ &hf_ypserv_servesdomain, {
			"Serves Domain", "ypserv.servesdomain", FT_BOOLEAN, BASE_DEC,
			&yesno, 0, "Serves Domain" }},
		{ &hf_ypserv_map, {
			"Map Name", "ypserv.map", FT_STRING, BASE_DEC,
			NULL, 0, "Map Name" }},
		{ &hf_ypserv_key, {
			"Key", "ypserv.key", FT_STRING, BASE_DEC,
			NULL, 0, "Key" }},
		{ &hf_ypserv_value, {
			"Value", "ypserv.value", FT_STRING, BASE_DEC,
			NULL, 0, "Value" }},
		{ &hf_ypserv_status, {
			"Status", "ypserv.status", FT_BOOLEAN, BASE_DEC,
			&okfailed , 0, "Status" }},
	};
	static gint *ett[] = {
		&ett_ypserv,
	};

	proto_ypserv = proto_register_protocol("Yellow Pages Service",
	    "YPSERV", "ypserv");
	proto_register_field_array(proto_ypserv, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));
}

void
proto_reg_handoff_ypserv(void)
{
	/* Register the protocol as RPC */
	rpc_init_prog(proto_ypserv, YPSERV_PROGRAM, ett_ypserv);
	/* Register the procedure tables */
	old_rpc_init_proc_table(YPSERV_PROGRAM, 1, ypserv1_proc);
	old_rpc_init_proc_table(YPSERV_PROGRAM, 2, ypserv2_proc);
}
