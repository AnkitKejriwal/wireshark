/* packet-smb-sidsnooping.c
 * Routines for snooping SID to name mappings
 * Copyright 2003, Ronnie Sahlberg
 *
 * $Id: packet-smb-sidsnooping.c,v 1.2 2003/03/25 19:52:56 guy Exp $
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
# include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include "epan/packet_info.h"
#include "epan/epan_dissect.h"
#include "epan/proto.h"
#include "tap.h"
#include "packet-dcerpc.h"
#include "register.h"
#include "smb.h"
#include "packet-smb-sidsnooping.h"

static int hf_lsa = -1;
static int hf_lsa_opnum = -1;
static int hf_lsa_domain = -1;
static int hf_smb_sid = -1;



static GHashTable *sid_name_table = NULL;
typedef struct _sid_name {
	char *sid;
	char *name;
} sid_name;
static GMemChunk *sid_name_chunk = NULL;
static int sid_name_init_count = 200;

static void *lsa_QueryInfoPolicy_l3_reply_flag = NULL;


char *
find_sid_name(char *sid)
{
	sid_name *sn;
	sid_name old_sn;

	old_sn.sid=sid;
	sn=g_hash_table_lookup(sid_name_table, &old_sn);
	if(!sn){
		return NULL;
	}
	return sn->name;
}

static void
add_sid_name_mapping(char *sid, char *name)
{
	sid_name *sn;
	sid_name old_sn;

	old_sn.sid=sid;
	sn=g_hash_table_lookup(sid_name_table, &old_sn);
	if(sn){
		return;
	}

	sn=g_mem_chunk_alloc(sid_name_chunk);
	sn->sid=g_strdup(sid);
	sn->name=g_strdup(name);
	g_hash_table_insert(sid_name_table, sn, sn);
}

/*
 * QueryInfoPolicy level 3 contains both a domain name and a sid
 * which we can use to map a sid into a name.
 */
static int
lsa_QueryInfoPolicy_l3_reply(void *dummy _U_, packet_info *pinfo _U_, epan_dissect_t *edt, void *pri _U_)
{
/*	dcerpc_info *ri=pri;*/
	GPtrArray *gp_sid;
	GPtrArray *gp_domain;
	field_info *fi;
	char *domain;
	char *sid;

	gp_domain=proto_get_finfo_ptr_array(edt->tree, hf_lsa_domain);
	if(!gp_domain || gp_domain->len!=1){
		return 0;
	}
	fi=gp_domain->pdata[0];
	domain=fi->value->value.string;

	gp_sid=proto_get_finfo_ptr_array(edt->tree, hf_smb_sid);
	if(!gp_sid || gp_sid->len!=1){
		return 0;
	}
	fi=gp_sid->pdata[0];
	sid=fi->value->value.string;

	add_sid_name_mapping(sid, domain);
	return 0;
}

static gboolean
free_all_sid_names(gpointer key_arg, gpointer value _U_, gpointer user_data _U_)
{
	sid_name *sn = (sid_name *)key_arg;

	if(sn->sid){
		g_free((gpointer)sn->sid);
		sn->sid=NULL;
	}
	if(sn->name){
		g_free((gpointer)sn->name);
		sn->name=NULL;
	}
	return TRUE;
}

static gint
sid_name_equal(gconstpointer k1, gconstpointer k2)
{
	const sid_name *sn1 = (const sid_name *)k1;
	const sid_name *sn2 = (const sid_name *)k2;
	
	return !strcmp(sn1->sid, sn2->sid);
}

static guint
sid_name_hash(gconstpointer k)
{
	const sid_name *sn = (const sid_name *)k;
	int i, sum;

	for(sum=0,i=strlen(sn->sid)-1;i>=0;i--){
		sum+=sn->sid[i];
	}

	return sum;
}

static void
sid_snooping_init(void)
{
	header_field_info *hfi;

	if(lsa_QueryInfoPolicy_l3_reply_flag){
		remove_tap_listener(lsa_QueryInfoPolicy_l3_reply_flag);
		lsa_QueryInfoPolicy_l3_reply_flag=NULL;
	}

	if(sid_name_table){
		g_hash_table_foreach_remove(sid_name_table, free_all_sid_names, NULL);
		sid_name_table=NULL;
	}
	if(sid_name_chunk){
		g_mem_chunk_destroy(sid_name_chunk);
		sid_name_chunk=NULL;
	}


	if(!sid_name_snooping){
		return;
	}


	sid_name_table=g_hash_table_new(sid_name_hash, sid_name_equal);
	sid_name_chunk = g_mem_chunk_new("sid_name_chunk",
	    sizeof(sid_name),
	    sid_name_init_count * sizeof(sid_name),
	    G_ALLOC_ONLY);


	hf_lsa=proto_get_id_by_filter_name("lsa");

	hfi=proto_registrar_get_byname("lsa.opnum");
	if(hfi){
		hf_lsa_opnum=hfi->id;
	}

	hfi=proto_registrar_get_byname("smb.sid");
	if(hfi){
		hf_smb_sid=hfi->id;
	}

	hfi=proto_registrar_get_byname("lsa.domain");
	if(hfi){
		hf_lsa_domain=hfi->id;
	}



	if(register_tap_listener("dcerpc", lsa_QueryInfoPolicy_l3_reply, "dcerpc.pkt_type==2 and lsa.opnum==7 and lsa.info.level==3 and lsa.domain and smb.sid", NULL, lsa_QueryInfoPolicy_l3_reply, NULL)){
		/* error, we failed to attach to the tap. clean up */

		fprintf(stderr,"tethereal: proto_reg_handoff_smb_sidsnooping()/lsa_QueryInfoPolicy_l3_reply failed to attach to tap.\n");
		exit(1);
	}
	lsa_QueryInfoPolicy_l3_reply_flag=lsa_QueryInfoPolicy_l3_reply;
}

void
proto_register_smb_sidsnooping(void)
{
	register_init_routine(sid_snooping_init);
}

void
proto_reg_handoff_smb_sidsnooping(void)
{
}

