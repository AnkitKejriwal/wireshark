/* packet-k12.c
* Routines for displaying frames from k12 rf5 files
*
* Luis E. Garcia Ontanon <luis.ontanon@gmail.com>
*
* $Id$
*
* Wireshark - Network traffic analyzer
* By Gerald Combs <gerald@wireshark.org>
* Copyright 1998
*
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
#include <errno.h>
#include <glib.h>
#include <string.h>
#include <epan/packet.h>
#include <prefs.h>
#include <epan/report_err.h>
#include <epan/emem.h>
#include <epan/uat.h>
#include <epan/expert.h>
#include "packet-sscop.h"

typedef struct _k12_hdls_t {
	char* match;
	char* protos;
	dissector_handle_t* handles;
} k12_handles_t;

static int proto_k12 = -1;

static int hf_k12_port_id = -1;
static int hf_k12_port_name = -1;
static int hf_k12_stack_file = -1;
static int hf_k12_port_type = -1;
static int hf_k12_atm_vp = -1;
static int hf_k12_atm_vc = -1;
static int hf_k12_atm_cid = -1;

static int hf_k12_ts = -1;

static gint ett_k12 = -1;
static gint ett_port = -1;
static gint ett_stack_item = -1;

static dissector_handle_t k12_handle;
static dissector_handle_t data_handle;
static dissector_handle_t sscop_handle;

extern int proto_sscop;

static emem_tree_t* port_handles = NULL;
static uat_t* k12_uat = NULL;
static k12_handles_t* k12_handles = NULL;
static guint nk12_handles = 0;

static const value_string  k12_port_types[] = {
	{	K12_PORT_DS1, "Ds1" },
	{	K12_PORT_DS0S, "Ds0 Range" },
	{	K12_PORT_ATMPVC, "ATM PVC" },
	{ 0,NULL}
};

static void dissect_k12(tvbuff_t* tvb,packet_info* pinfo,proto_tree* tree) {
	static dissector_handle_t data_handles[] = {NULL,NULL};
	proto_item* k12_item;
	proto_tree* k12_tree;
	proto_item* stack_item;
	dissector_handle_t sub_handle = NULL;
	dissector_handle_t* handles;
	guint i;

	k12_item = proto_tree_add_protocol_format(tree, proto_k12, tvb, 0, 0, "Packet from: '%s' (0x%.8x)",
											  pinfo->pseudo_header->k12.input_name,
											  pinfo->pseudo_header->k12.input);

	k12_tree = proto_item_add_subtree(k12_item, ett_k12);

	proto_tree_add_uint(k12_tree, hf_k12_port_id, tvb, 0,0,pinfo->pseudo_header->k12.input);
	proto_tree_add_string(k12_tree, hf_k12_port_name, tvb, 0,0,pinfo->pseudo_header->k12.input_name);
	stack_item = proto_tree_add_string(k12_tree, hf_k12_stack_file, tvb, 0,0,pinfo->pseudo_header->k12.stack_file);

	k12_item = proto_tree_add_uint(k12_tree, hf_k12_port_type, tvb, 0, 0,
								   pinfo->pseudo_header->k12.input_type);

	k12_tree = proto_item_add_subtree(k12_item, ett_port);

	switch ( pinfo->pseudo_header->k12.input_type ) {
		case K12_PORT_DS0S:
			proto_tree_add_uint(k12_tree, hf_k12_ts, tvb, 0,0,pinfo->pseudo_header->k12.input_info.ds0mask);
			break;
		case K12_PORT_ATMPVC:
        {
            gchar* circuit_str = ep_strdup_printf("%u:%u:%u",
                                                  (guint)pinfo->pseudo_header->k12.input_info.atm.vp,
                                                  (guint)pinfo->pseudo_header->k12.input_info.atm.vc,
                                                  (guint)pinfo->pseudo_header->k12.input_info.atm.cid);
            
			/*
			 * XXX: this is prone to collisions!
			 * we need an uniform way to manage circuits between dissectors
			 */
            pinfo->circuit_id = g_str_hash(circuit_str);
            
			proto_tree_add_uint(k12_tree, hf_k12_atm_vp, tvb, 0,0,pinfo->pseudo_header->k12.input_info.atm.vp);
			proto_tree_add_uint(k12_tree, hf_k12_atm_vc, tvb, 0,0,pinfo->pseudo_header->k12.input_info.atm.vc);
            if (pinfo->pseudo_header->k12.input_info.atm.cid) 
                proto_tree_add_uint(k12_tree, hf_k12_atm_cid, tvb, 0,0,pinfo->pseudo_header->k12.input_info.atm.cid);
			break;
        }
		default:
			break;
	}

	handles = se_tree_lookup32(port_handles, pinfo->pseudo_header->k12.input);
	
	if (! handles ) {
		for (i=0 ; i < nk12_handles; i++) {
			if ( strcasestr(pinfo->pseudo_header->k12.stack_file, k12_handles[i].match) ) {
				handles = k12_handles[i].handles;
				break;
			}
		}
		
		if (!handles) {
			data_handles[0] = data_handle;
			handles = data_handles;
		}
		
		se_tree_insert32(port_handles, pinfo->pseudo_header->k12.input, handles);
		
	}
	
	if (handles == data_handles) {
		proto_tree* stack_tree = proto_item_add_subtree(stack_item,ett_stack_item);
		proto_item* item;
		
		item = proto_tree_add_text(stack_tree,tvb,0,0,
								   "Warning: stk file not matched in the 'K12 Protocols' table");
		PROTO_ITEM_SET_GENERATED(item);
		expert_add_info_format(pinfo, item, PI_UNDECODED, PI_WARN, "unmatched stk file");
		
		item = proto_tree_add_text(stack_tree,tvb,0,0,
								   "Info: You can edit the 'K12 Protocols' table from Preferences->Protocols->k12xx");
		PROTO_ITEM_SET_GENERATED(item);
	}
	
	sub_handle = handles[0];

	/* Setup subdissector information */
		
	for (i = 0; handles[i] && handles[i+1]; ++i) {
		if (handles[i] == sscop_handle) {
			sscop_payload_info *p_sscop_info = p_get_proto_data(pinfo->fd, proto_sscop);
			if (p_sscop_info)
				p_sscop_info->subdissector = handles[i+1];
			else {
				p_sscop_info = ep_alloc0(sizeof(sscop_payload_info));
				if (p_sscop_info) {
					p_sscop_info->subdissector = handles[i+1];
					p_add_proto_data(pinfo->fd, proto_sscop, p_sscop_info);
				}
			}
		}
		/* Add more protocols here */
	}

	call_dissector(sub_handle, tvb, pinfo, tree);
}

static void k12_update_cb(void* r, char** err) {
	k12_handles_t* h = r;
	gchar** protos;
	guint num_protos, i;
	
	protos = ep_strsplit(h->protos,":",0);
	
	for (num_protos = 0; protos[num_protos]; num_protos++) g_strstrip(protos[num_protos]);
	
	if (h->handles) g_free(h->handles);
	
	h->handles = g_malloc(sizeof(dissector_handle_t)*num_protos);
	
	for (i = 0; i < num_protos; i++) {
		if ( ! (h->handles[i] = find_dissector(protos[i])) ) {
			h->handles[i] = data_handle;
			*err = ep_strdup_printf("Could not find dissector for: '%s'",protos[i]);
			return;
		}
	}
	
	*err = NULL;
}

static void* k12_copy_cb(void* dest, const void* orig, unsigned len _U_) {
	k12_handles_t* d = dest;
	const k12_handles_t* o = orig;
	gchar** protos = ep_strsplit(d->protos,":",0);
	guint num_protos;
	
	for (num_protos = 0; protos[num_protos]; num_protos++) g_strstrip(protos[num_protos]);
	
	d->match = g_strdup(o->match);
	d->protos = g_strdup(o->protos);
	d->handles = g_memdup(o->handles,sizeof(dissector_handle_t)*(num_protos+1));
	
	return dest;
}

static void k12_free_cb(void* r) {
	k12_handles_t* h = r;
	if (h->match) g_free(h->match);
	if (h->protos) g_free(h->protos);
	if (h->handles) g_free(h->handles);
}


static gboolean protos_chk_cb(void* r _U_, const char* p, unsigned len, void* u1 _U_, void* u2 _U_, char** err) {
	gchar** protos;
	gchar* line = ep_strndup(p,len);
	guint num_protos, i;
	
	g_strstrip(line);
	g_strdown(line);

	protos = ep_strsplit(line,":",0);

	for (num_protos = 0; protos[num_protos]; num_protos++) g_strstrip(protos[num_protos]);
	
	if (!num_protos) {
		*err = ep_strdup_printf("No protocols given");
		return FALSE;			
	}
	
	for (i = 0; i < num_protos; i++) {
		if (!find_dissector(protos[i])) {
			*err = ep_strdup_printf("Could not find dissector for: '%s'",protos[i]);
			return FALSE;
		}
	}
	
	return TRUE;
}

UAT_CSTRING_CB_DEF(k12,match,k12_handles_t)
UAT_CSTRING_CB_DEF(k12,protos,k12_handles_t)

/* Make sure handles for various protocols are initialized */
static void initialize_handles_once(void) {
	static gboolean initialized = FALSE;
	if (!initialized) {
		k12_handle = find_dissector("k12");
		data_handle = find_dissector("data");
		sscop_handle = find_dissector("sscop");
		initialized = TRUE;
	}
}

void proto_reg_handoff_k12(void) {
	initialize_handles_once();
	dissector_add("wtap_encap", WTAP_ENCAP_K12, k12_handle);
}

void
proto_register_k12(void)
{
	static hf_register_info hf[] = {
		{ &hf_k12_port_id, { "Port Id", "k12.port_id", FT_UINT32, BASE_HEX,	NULL, 0x0, "", HFILL }},
		{ &hf_k12_port_name, { "Port Name", "k12.port_name", FT_STRING, BASE_NONE, NULL, 0x0,"", HFILL }},
		{ &hf_k12_stack_file, { "Stack file used", "k12.stack_file", FT_STRING, BASE_NONE, NULL, 0x0,"", HFILL }},
		{ &hf_k12_port_type, { "Port type", "k12.input_type", FT_UINT32, BASE_HEX, VALS(k12_port_types), 0x0,"", HFILL }},
		{ &hf_k12_ts, { "Timeslot mask", "k12.ds0.ts", FT_UINT32, BASE_HEX, NULL, 0x0, "", HFILL }},
		{ &hf_k12_atm_vp, { "ATM VPI", "atm.vpi", FT_UINT16, BASE_DEC, NULL, 0x0, "", HFILL }},
        { &hf_k12_atm_vc, { "ATM VCI", "atm.vci", FT_UINT16, BASE_DEC, NULL, 0x0, "", HFILL }},
        { &hf_k12_atm_cid, { "AAL2 CID", "aal2.cid", FT_UINT16, BASE_DEC, NULL, 0x0, "", HFILL }}
	};

  static gint *ett[] = {
	  &ett_k12,
	  &ett_port,
	  &ett_stack_item
  };

  static uat_field_t uat_k12_flds[] = {
	  UAT_FLD_CSTRING_ISPRINT(k12,match),
      UAT_FLD_CSTRING_OTHER(k12,protos,protos_chk_cb),
	  UAT_END_FIELDS
  };
  
  module_t *k12_module;
  
  proto_k12 = proto_register_protocol("K12xx", "K12xx", "k12");
  proto_register_field_array(proto_k12, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));
  register_dissector("k12", dissect_k12, proto_k12);

  k12_uat = uat_new("K12 Protocols",
					sizeof(k12_handles_t),
					"k12_protos",
					(void**) &k12_handles,
					&nk12_handles,
					k12_copy_cb,
					k12_update_cb,
					k12_free_cb,
					uat_k12_flds );
  
  k12_module = prefs_register_protocol(proto_k12, NULL);

  prefs_register_obsolete_preference(k12_module, "config");

  prefs_register_uat_preference(k12_module, "cfg",
								"K12 Protocols",
								"A table of matches vs stack filenames and relative protocols",
								k12_uat);
  
  port_handles = se_tree_create(EMEM_TREE_TYPE_RED_BLACK, "k12_port_handles");

}
