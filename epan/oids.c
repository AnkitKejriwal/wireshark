/* oids.c
 * Routines for OBJECT IDENTIFIER operations
 *
 * (c) 2007, Luis E. Garcia Ontanon <luis.ontanon@gmail.com>
 *
 * $Id$
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
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

#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "emem.h"
#include "uat-int.h"
#include "prefs.h"
#include "packet.h"
#include "report_err.h"
#include "filesystem.h"
#include "dissectors/packet-ber.h"

#ifdef HAVE_SMI
#include <smi.h>
#endif

#include "oids.h"

static const oid_value_type_t integer_type = { FT_INT32, BASE_DEC, BER_CLASS_UNI, BER_UNI_TAG_INTEGER, 1, 4 };
static const oid_value_type_t bytes_type = { FT_BYTES, BASE_NONE, BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, 0, -1 };
static const oid_value_type_t oid_type = { FT_OID, BASE_NONE, BER_CLASS_UNI, BER_UNI_TAG_OID, 1, -1 };
static const oid_value_type_t ipv4_type = { FT_IPv4, BASE_NONE, BER_CLASS_APP, 0, 4, 4 };
static const oid_value_type_t counter32_type = { FT_UINT32, BASE_DEC, BER_CLASS_APP, 1, 1, 4 };
static const oid_value_type_t unsigned32_type = { FT_UINT32, BASE_DEC, BER_CLASS_APP, 2, 1, 4 };
static const oid_value_type_t timeticks_type = { FT_UINT32, BASE_DEC, BER_CLASS_APP, 3, 1, 4 };
static const oid_value_type_t opaque_type = { FT_BYTES, BASE_NONE, BER_CLASS_APP, 4, 1, 4 };
static const oid_value_type_t counter64_type = { FT_UINT64, BASE_NONE, BER_CLASS_APP, 6, 8, 8 };
static const oid_value_type_t ipv6_type = {FT_IPv6, BASE_NONE, BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, 16, 16 };
static const oid_value_type_t float_type = {FT_FLOAT, BASE_DEC, BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, 4, 4 };
static const oid_value_type_t double_type = {FT_DOUBLE, BASE_DEC, BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, 8, 8 };
static const oid_value_type_t ether_type = {FT_ETHER, BASE_NONE, BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, 6, 6 };
static const oid_value_type_t string_type = {FT_STRING, BASE_NONE, BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, 0, -1 };
static const oid_value_type_t unknown_type = {FT_BYTES, BASE_NONE, BER_CLASS_ANY, BER_TAG_ANY, 0, -1 };

static oid_info_t oid_root = { 0, NULL, NULL, &unknown_type,-1, NULL, NULL};
static emem_tree_t* oids_by_name = NULL;

static oid_info_t* add_oid(char* name, const oid_value_type_t* type, guint oid_len, guint32 *subids) {
	guint i = 0;
	oid_info_t* c = &oid_root;
		
	oid_len--;
	
	do {
		oid_info_t* n = emem_tree_lookup32(c->children,subids[i]);
		
		if(n) {
			if (i == oid_len) {
				if (! n->name) {
					n->name = g_strdup(name);
				}
				
				if (! n->value_type) {
					n->value_type = type;
				}
				
				return n;
			}
		} else {
			n = g_malloc(sizeof(oid_info_t));
			n->subid = subids[i];
			n->children = pe_tree_create(EMEM_TREE_TYPE_RED_BLACK,"oid_children");
			n->value_hfid = -2;
			n->parent = c;
			n->bits = NULL;

			emem_tree_insert32(c->children,n->subid,n);

			if (i == oid_len) {
				n->name = g_strdup(name);
				n->value_type = type;
				return n;
			} else {
				n->name = g_strdup(name);
				n->value_type = NULL;
			}
		}
		c = n;
	} while(++i);
	
	g_assert_not_reached();
	return NULL;
}

extern void oid_add(char* name, guint oid_len, guint32 *subids) {
	add_oid(name,&unknown_type,oid_len,subids);
}

#ifdef HAVE_SMI
typedef struct smi_module_t {
	char* name;
} smi_module_t;

static smi_module_t* smi_paths = NULL;
static guint num_smi_paths = 0;
static uat_t* smi_paths_uat = NULL;

static smi_module_t* smi_modules = NULL;
static guint num_smi_modules = 0;
static uat_t* smi_modules_uat = NULL;

UAT_CSTRING_CB_DEF(smi_mod,name,smi_module_t)

static void* smi_mod_copy_cb(void* dest, const void* orig, unsigned len _U_) {
	const smi_module_t* m = orig;
	smi_module_t* d = dest;
	
	d->name = g_strdup(m->name);
	
	return d;
}	

static void smi_mod_free_cb(void* p) {
	smi_module_t* m = p;
	if (m->name) g_free(m->name);
}


static char* alnumerize(const char* name) {
	char* s = g_strdup(name);
	char* r = s;
	char* w = r;
	char c;
	
	for (;(c = *r); r++) {
		if (isalnum(c) || c == '_' || c == '-' || c == '.') {
			*(w++) = c;
		} else if (c == ':' && r[1] == ':') {
			*(w++) = '.';
		}
	}
	
	*w = '\0';
	
	return s;
}

typedef struct {
	char* name;
	SmiBasetype base;
	const oid_value_type_t* type;
} oid_value_type_mapping_t;

const oid_value_type_t* get_typedata(SmiType* smiType) {
	static const oid_value_type_mapping_t types[] =  {
		{"IpAddress", SMI_BASETYPE_UNKNOWN, &ipv4_type},
		{"InetAddressIPv4",SMI_BASETYPE_UNKNOWN,&ipv4_type},
		{"InetAddressIPv6",SMI_BASETYPE_UNKNOWN,&ipv6_type},
		{"NetworkAddress",SMI_BASETYPE_UNKNOWN,&ipv4_type},
		{"MacAddress",SMI_BASETYPE_UNKNOWN,&ether_type},
		{"TimeTicks",SMI_BASETYPE_UNKNOWN,&timeticks_type},
		{"Ipv6Address",SMI_BASETYPE_UNKNOWN,&ipv6_type},
		{"TimeStamp",SMI_BASETYPE_UNKNOWN,&integer_type},
		{"DisplayString",SMI_BASETYPE_UNKNOWN,&string_type},
		{"DateAndTime",SMI_BASETYPE_UNKNOWN,&string_type},
		{"Counter",SMI_BASETYPE_UNKNOWN,&counter32_type},
		{"Counter32",SMI_BASETYPE_UNKNOWN,&counter32_type},
		{"Unsigned32",SMI_BASETYPE_UNKNOWN,&unsigned32_type},
		{"Gauge",SMI_BASETYPE_UNKNOWN,&unsigned32_type},
		{"Gauge32",SMI_BASETYPE_UNKNOWN,&unsigned32_type},
		{"i32",SMI_BASETYPE_INTEGER32,&integer_type},
		{"octets",SMI_BASETYPE_OCTETSTRING,&bytes_type},
		{"oid",SMI_BASETYPE_OBJECTIDENTIFIER,&oid_type},
		{"u32",SMI_BASETYPE_UNSIGNED32,&unsigned32_type},
		{"u64",SMI_BASETYPE_UNSIGNED64,&counter64_type},
		{"f32",SMI_BASETYPE_FLOAT32,&float_type},
		{"f64",SMI_BASETYPE_FLOAT64,&double_type},
		{"f128",SMI_BASETYPE_FLOAT128,&bytes_type},
		{"enum",SMI_BASETYPE_ENUM,&integer_type},
		{"bits",SMI_BASETYPE_BITS,&bytes_type},
		{"unk",SMI_BASETYPE_UNKNOWN,&unknown_type},
		{NULL,0,NULL}
	};
	const oid_value_type_mapping_t* t;
	SmiType* sT = smiType;

	if (!smiType) return NULL;
		
	do {
		for (t = types; t->type ; t++ ) {
			const char* name = smiRenderType(sT, SMI_RENDER_NAME);
			if (name && t->name && g_str_equal(name, t->name )) {
				return t->type;
			}
		}
	} while(( sT  = smiGetParentType(sT) ));
	
	for (t = types; t->type ; t++ ) {
		if(smiType->basetype == t->base) {
			return t->type;
		}
	}

	return &unknown_type;
}

#define IS_ENUMABLE(ft) ( (ft == FT_UINT8) || (ft == FT_UINT16) || (ft == FT_UINT24) || (ft == FT_UINT32) \
						   || (ft == FT_INT8) || (ft == FT_INT16) || (ft == FT_INT24) || (ft == FT_INT32) \
						   || (ft == FT_UINT64) || (ft == FT_INT64) )

void register_mibs(void) {
	SmiModule *smiModule;
    SmiNode *smiNode;
	guint i;
	int proto_mibs = -1;
	module_t* mibs_module;
	GArray* hfa = g_array_new(FALSE,TRUE,sizeof(hf_register_info));
	GArray* etta = g_array_new(FALSE,TRUE,sizeof(gint*));
	static uat_field_t smi_fields[] = {
		UAT_FLD_CSTRING(smi_mod,name,"The module's name"),
		UAT_END_FIELDS
	};
	static uat_field_t smi_paths_fields[] = {
		UAT_FLD_CSTRING(smi_mod,name,"The directory name"),
		UAT_END_FIELDS
	};
	char* smi_load_error = NULL;
	GString* path_str;
	
	smi_modules_uat = uat_new("SMI Modules",
							  sizeof(smi_module_t),
							  "smi_modules",
							  (void**)&smi_modules,
							  &num_smi_modules,
							  UAT_CAT_GENERAL,
							  "ChSNMPSMIModules",
							  smi_mod_copy_cb,
							  NULL,
							  smi_mod_free_cb,
							  smi_fields);
	
	smi_paths_uat = uat_new("SMI Paths",
							  sizeof(smi_module_t),
							  "smi_paths",
							  (void**)&smi_paths,
							  &num_smi_paths,
							  UAT_CAT_GENERAL,
							  "ChSNMPSMIPaths",
							  smi_mod_copy_cb,
							  NULL,
							  smi_mod_free_cb,
							  smi_paths_fields);
	
	
	uat_load(smi_modules_uat, &smi_load_error);
	
	if (smi_load_error) {
		report_failure("Error Loading SMI Modules Table: %s",smi_load_error);
		return;
	}

	uat_load(smi_paths_uat, &smi_load_error);

	if (smi_load_error) {
		report_failure("Error Loading SMI Paths Table: %s",smi_load_error);
		return;
	}
	
	path_str = g_string_new(get_datafile_path("mibs"));
	g_string_sprintfa(path_str,":%s",get_persconffile_path("mibs", FALSE));

	for(i=0;i<num_smi_paths;i++) {
		if (!smi_paths[i].name) continue;
	}
		
	smiInit(NULL);
	
	smiSetPath(path_str->str);
	
	g_string_free(path_str,TRUE);
	
	for(i=0;i<num_smi_modules;i++) {
		if (!smi_modules[i].name) continue;
	

		if (smiIsLoaded(smi_modules[i].name)) {
			continue;
		} else {
			char* mod_name =  smiLoadModule(smi_modules[i].name);
			printf("%d %s %s\n",i,smi_modules[i].name,mod_name);
		}
	}
		
	for (smiModule = smiGetFirstModule();
		 smiModule;
		 smiModule = smiGetNextModule(smiModule)) {
		
		for (smiNode = smiGetFirstNode(smiModule, SMI_NODEKIND_ANY); 
			 smiNode;
			 smiNode = smiGetNextNode(smiNode, SMI_NODEKIND_ANY)) {
			
			SmiType* smiType =  smiGetNodeType(smiNode);
			const oid_value_type_t* typedata =  get_typedata(smiType);
			oid_info_t* oid_data = add_oid(smiRenderOID(smiNode->oidlen, smiNode->oid, SMI_RENDER_QUALIFIED),
										   typedata,
										   smiNode->oidlen,
										   smiNode->oid);

			if ( typedata && oid_data->value_hfid == -2 ) {
				SmiNamedNumber* smiEnum; 
				hf_register_info hf = { &(oid_data->value_hfid), { 
					oid_data->name,
					alnumerize(oid_data->name),
					typedata->ft_type,
					typedata->display,
					NULL,
					0,
					g_strdup(smiRenderOID(smiNode->oidlen, smiNode->oid, SMI_RENDER_ALL)),
					HFILL }};
				
				oid_data->value_hfid = -1;
				
				if ( IS_ENUMABLE(hf.hfinfo.type) && (smiEnum = smiGetFirstNamedNumber(smiType))) {
					GArray* vals = g_array_new(TRUE,TRUE,sizeof(value_string));
					
					for(;smiEnum; smiEnum = smiGetNextNamedNumber(smiEnum)) {
						if (smiEnum->name) {
							value_string val = {smiEnum->value.value.integer32,g_strdup(smiEnum->name)};
							g_array_append_val(vals,val);
						}
					}
					
					hf.hfinfo.strings = VALS(vals->data);
					g_array_free(vals,FALSE);
				}
#if 0
 /* packet-snmp does not use this yet */
				else if (smiType->basetype == SMI_BASETYPE_BITS && ( smiEnum = smiGetFirstNamedNumber(smiType) )) {
					guint n = 0;
					oid_bits_info_t* bits = g_malloc(sizeof(oid_bits_info_t));
					gint* ettp = &(bits->ett);
					
					bits->num = 0;
					bits->ett = -1;
					
					g_array_append_val(etta,ettp);
					
					for(;smiEnum; smiEnum = smiGetNextNamedNumber(smiEnum), bits->num++);
					
					bits->data = g_malloc(sizeof(struct _oid_bit_t)*bits->num);
					
					for(smiEnum = smiGetFirstNamedNumber(smiType),n=0;
						smiEnum;
						smiEnum = smiGetNextNamedNumber(smiEnum),n++) {
						guint mask = 1 << (smiEnum->value.value.integer32 % 8);
						char* base = alnumerize(oid_data->name);
						char* ext = alnumerize(smiEnum->name);
						hf_register_info hf2 = { &(bits->data[n].hfid), { NULL, NULL, FT_UINT8, BASE_HEX, NULL, mask, "", HFILL }};
						
						bits->data[n].hfid = -1;
						bits->data[n].offset = smiEnum->value.value.integer32 / 8;
						
						hf2.hfinfo.name = g_strdup_printf("%s:%s",oid_data->name,smiEnum->name);
						hf2.hfinfo.abbrev = g_strdup_printf("%s.%s",base,ext);
						
						g_free(base);
						g_free(ext);
						g_array_append_val(hfa,hf2);
					}
				}
#endif
				g_array_append_val(hfa,hf);
			}
		}
	}
	
	proto_mibs = proto_register_protocol("MIBs", "MIBS", "mibs");
	
	proto_register_field_array(proto_mibs, (hf_register_info*)hfa->data, hfa->len);
	mibs_module = prefs_register_protocol(proto_mibs, NULL);
	
	prefs_register_uat_preference(mibs_module, "smi_paths",
								  "MIB paths",
								  "List of directories where MIBs are to be looked for",
								  smi_paths_uat);
	
	prefs_register_uat_preference(mibs_module, "smi_modules",
											   "MIB modules",
											   "List of MIB modules to be loaded",
											   smi_modules_uat);
	
	proto_register_subtree_array((gint**)etta->data, etta->len);

	
	g_array_free(etta,TRUE);
	g_array_free(hfa,FALSE);
}
#endif


void oid_init(void) {
	oid_root.children = pe_tree_create(EMEM_TREE_TYPE_RED_BLACK,"oid_root");
	oids_by_name = pe_tree_create(EMEM_TREE_TYPE_RED_BLACK,"oid_names");
	
#ifdef HAVE_SMI
	register_mibs();
#endif
}

const char* oid_subid2string(guint32* subids, guint len) {
	char* s = ep_alloc0(len*11);
	char* w = s;
	
	do {
		w += sprintf(w,"%u.",*subids++);
	} while(--len);
	
	if (w!=s) *(w-1) = '\0'; else *(w) = '\0';
	
	return s;
}

guint check_num_oid(const char* str) {
	const char* r = str;
	char c = '\0';
	guint n = 0;
	
	if (*r == '.') return 0;
	
	do {
		switch(*r++) {
			case '.':
				n++;
				if (c == '.') return 0; 
			case '1' : case '2' : case '3' : case '4' : case '5' : 
			case '6' : case '7' : case '8' : case '9' : case '0' :
				continue;
			case '\0':
				break;
			default:
				return 0;
		} 
		c = *r;
	} while(1);
	
	if (c == '.') return 0;
	
	return n;
}

guint oid_string2subid(const char* str, guint32** subids_p) {
	const char* r = str;
	guint32* subids;
	guint n = check_num_oid(str);
	
	if (!n) {
		*subids_p = NULL;
		return 0;
	}
	
	*subids_p = subids = ep_alloc_array(guint32,n);
	
	do switch(*r) {
		case '.':
			subids++;
			continue;
		case '1' : case '2' : case '3' : case '4' : case '5' : 
		case '6' : case '7' : case '8' : case '9' : case '0' :
			*(subids) *= 10;
			*(subids) += *r - '0';
			continue;
		case '\0':
			break;
		default:
			return 0;
	} while(1);
	
	return n;
}


guint oid_encoded2subid(const guint8 *oid_bytes, gint oid_len, guint32** subids_p) {
	gint i;
	guint n = 1;
	guint32 subid = 0;
	gboolean is_first = TRUE;
	guint32* subids;
	
	for (i=0; i<oid_len; i++) { if (! (oid_bytes[i] & 0x80 )) n++; }
	
	*subids_p = subids = ep_alloc(sizeof(guint32)*n);
	
	for (i=0; i<oid_len; i++){
		guint8 byte = oid_bytes[i];
		
		subid <<= 7;
		subid |= byte & 0x7F;
		
		if (byte & 0x80) {
			continue;
		}
		
		if (is_first) {
			guint32 subid0 = 0;
			
			if (subid >= 40) { subid0++; subid-=40; }
			if (subid >= 40) { subid0++; subid-=40; }
			
			*subids++ = subid0;
			
			is_first = FALSE;
		}
		
		*subids++ = subid;
		subid = 0;
	}
	
	return n;
}

oid_info_t* oid_get(guint len, guint32* subids, guint* matched, guint* left) {
	oid_info_t* curr_oid = &oid_root;
	guint i;
	
	for( i=0; i < len; i++) {
		oid_info_t* next_oid = emem_tree_lookup32(curr_oid->children,subids[i]);
		if (next_oid) {
			curr_oid = next_oid;
		} else {
			goto done;
		}
	}
done:
	*matched = i;
	*left = len - i;
	return curr_oid;
}


oid_info_t* oid_get_from_encoded(const guint8 *bytes, gint byteslen, guint32** subids_p, guint* matched_p, guint* left_p) {
	return oid_get(oid_encoded2subid(bytes, byteslen, subids_p), *subids_p, matched_p, left_p);
}	
	
oid_info_t* oid_get_from_string(const gchar *oid_str, guint32** subids_p, guint* matched, guint* left) {
	return oid_get(oid_string2subid(oid_str, subids_p), *subids_p, matched, left);
}

const gchar *oid_resolved_from_encoded(const guint8 *oid, gint oid_len) {
	guint32 *subid_oid;
	guint subid_oid_length = oid_encoded2subid(oid, oid_len, &subid_oid);
	guint matched;
	guint left;
	oid_info_t* curr_oid = oid_get(subid_oid_length, subid_oid, &matched, &left);
		
		if (matched == subid_oid_length) {
			return curr_oid->name;
		} else {
			return ep_strdup_printf("%s.%s",
									curr_oid->name,
									oid_subid2string(&(subid_oid[matched]),left) );
		}
	
}


guint oid_subid2encoded(guint subids_len, guint32* subids, guint8** bytes_p) {
	guint bytelen = 0;
	guint i;
	guint32 subid;
	guint8* bytes;
	guint8* b;
	
	if (subids_len < 2) {
		*bytes_p = NULL;
		return 0;
	}
	
	subid = (subids[0] * 40) + subids[1];
	i = 2;
	
	do {
		if (subid <= 0x0000007F) {
			bytelen += 1;
		} else if (subid <= 0x00003FFF ) {
			bytelen += 2;
		} else if (subid <= 0x001FFFFF ) {
			bytelen += 3;
		} else if (subid <= 0x0FFFFFFF ) {
			bytelen += 4;
		} else {
			bytelen += 5;
		}
		
			subid = subids[i];
	} while ( i++ < subids_len );
	
	*bytes_p = b = bytes = ep_alloc(bytelen);
	
	subid = (subids[0] * 40) + subids[1];
	i = 2;
	
	do {
		guint len;
		
		if ((subid <= 0x0000007F )) len = 1;
		else if ((subid <= 0x00003FFF )) len = 2;
		else if ((subid <= 0x001FFFFF )) len = 3;
		else if ((subid <= 0x0FFFFFFF )) len = 4;
		else len = 5;
		
		switch(len) {
			default: DISSECTOR_ASSERT_NOT_REACHED(); break;
			case 5: *(b++) = ((subid & 0xF0000000) << 28) | 0x80;
			case 4: *(b++) = ((subid & 0x0FE00000 ) >> 21)  | 0x80;
			case 3: *(b++) = ((subid & 0x001FC000 ) >> 14)  | 0x80;
			case 2: *(b++) = ((subid & 0x00003F10 ) >> 7)  | 0x80;
			case 1: *(b++) =  subid & 0x0000007F ; break;
		}
		
		subid = subids[i];
	} while ( i++ < subids_len);
	
	return bytelen;
}

const gchar* oid_encoded2string(const guint8* encoded, guint len) {
	guint32* subids;
	guint subids_len = oid_encoded2subid(encoded, len, &subids);
	
	if (subids_len) {
		return oid_subid2string(subids,subids_len);
	} else {
		return "";
	}
}



guint oid_string2encoded(const char *oid_str, guint8 **bytes) {
	guint32* subids;
	guint32 subids_len;
	guint byteslen;
		
		if ( ( subids_len = oid_string2subid(oid_str, &subids) )
			 && 
			 ( byteslen = oid_subid2encoded(subids_len, subids, bytes) )  ) {
			return byteslen;
		}
	return 0;
}

char* oid2str(oid_info_t* oid, guint32* subids, guint len, guint left) {
	if (left == 0) {
		return oid->name;
	} else {
		return ep_strdup_printf("%s.%s",oid->name,oid_subid2string(subids+(len-left),left));
	}
}

const gchar *oid_resolved_from_string(const gchar *oid_str) {
	guint32* subids;
	guint num_subids = oid_string2subid(oid_str, &subids);
	
	if (num_subids) {
		guint matched;
		guint left;
		oid_info_t* oid = oid_get(num_subids, subids, &matched, &left);
		return oid2str(oid, subids, num_subids, left);
	} else {
		return emem_tree_lookup_string(oids_by_name, oid_str);
	}
}

extern char* oid_test_a2b(guint32 num_subids, guint32* subids);
char* oid_test_a2b(guint32 num_subids, guint32* subids) {
	guint8* sub2enc;
	guint8* str2enc;
	guint32* enc2sub;
	guint32* str2sub;
	const char* sub2str = oid_subid2string(subids, num_subids);
	guint sub2enc_len = oid_subid2encoded(num_subids, subids,&sub2enc);
	guint enc2sub_len = oid_encoded2subid(sub2enc, sub2enc_len, &enc2sub);
	const char* enc2str = oid_encoded2string(sub2enc, sub2enc_len);
	guint str2enc_len = oid_string2encoded(sub2str,&str2enc);
	guint str2sub_len = oid_string2subid(sub2str,&str2sub);
	
	return ep_strdup_printf(
							"oid_subid2string=%s \n"
							"oid_subid2encoded=[%d]%s \n"
							"oid_encoded2subid=%s \n "
							"oid_encoded2string=%s \n"
							"oid_string2encoded=[%d]%s \n"
							"oid_string2subid=%s \n "
							,sub2str
							,sub2enc_len,bytestring_to_str(sub2enc, sub2enc_len, ':')
							,enc2sub ? oid_subid2string(enc2sub,enc2sub_len) : "-"
							,enc2str
							,str2enc_len,bytestring_to_str(str2enc, str2enc_len, ':')
							,str2sub ? oid_subid2string(str2sub,str2sub_len) : "-"
							);	
}

const gchar *oid_resolved(guint32 num_subids, guint32* subids) {
	guint matched;
	guint left;
	oid_info_t* oid = oid_get(num_subids, subids, &matched, &left);
	
	while (! oid->name ) {
		if (!(oid = oid->parent)) {
			return oid_subid2string(subids,num_subids);
		}
		left++;
		matched--;
	}
	
	if (left) {
		return ep_strdup_printf("%s.%s",
								oid->name,
								oid_subid2string(&(subids[matched]),left));
	} else {
		return oid->name;
	}
}

