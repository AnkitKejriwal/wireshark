/* packet-dcerpc-nt.c
 * Routines for DCERPC over SMB packet disassembly
 * Copyright 2001, Tim Potter <tpot@samba.org>
 *
 * $Id: packet-dcerpc-nt.c,v 1.44 2002/08/22 01:13:12 tpot Exp $
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
#include "packet-dcerpc-nt.h"
#include "smb.h"
#include "packet-smb-common.h" /* for dissect_smb_64bit_time() */

/*
 * This file contains helper routines that are used by the DCERPC over SMB
 * dissectors for ethereal.
 */

/* Align offset to a n-byte boundary */

int prs_align(int offset, int n)
{
	if (offset % n)
		offset += n - (offset % n);
	
	return offset;
}

/* Parse a 8-bit integer */

int prs_uint8(tvbuff_t *tvb, int offset, packet_info *pinfo _U_,
	      proto_tree *tree, guint8 *data, char *name)
{
	guint8 i;
	
	/* No alignment required */

	i = tvb_get_guint8(tvb, offset);
	offset++;

	if (name && tree)
		proto_tree_add_text(tree, tvb, offset - 1, 1, 
				    "%s: %u", name, i);

	if (data)
		*data = i;

	return offset;
}

int prs_uint8s(tvbuff_t *tvb, int offset, packet_info *pinfo _U_,
	       proto_tree *tree, int count, int *data_offset, char *name)
{
	/* No alignment required */

	if (name && tree)
		proto_tree_add_text(tree, tvb, offset, count, "%s", name);

	if (data_offset)
		*data_offset = offset;

	offset += count;

	return offset;
}

/* Parse a 16-bit integer */

int prs_uint16(tvbuff_t *tvb, int offset, packet_info *pinfo _U_,
	       proto_tree *tree, guint16 *data, char *name)
{
	guint16 i;
	
	offset = prs_align(offset, 2);
	
	i = tvb_get_letohs(tvb, offset);
	offset += 2;

	if (name && tree)
		proto_tree_add_text(tree, tvb, offset - 2, 2, 
				    "%s: %u", name, i);
	if (data)
		*data = i;

	return offset;
}

/* Parse a number of uint16's */

int prs_uint16s(tvbuff_t *tvb, int offset, packet_info *pinfo _U_,
		proto_tree *tree, int count, int *data_offset, char *name)
{
	offset = prs_align(offset, 2);
	
	if (name && tree)
		proto_tree_add_text(tree, tvb, offset, count * 2, 
				    "%s", name);
	if (data_offset)
		*data_offset = offset;

	offset += count * 2;

	return offset;
}

/* Parse a 32-bit integer */

int prs_uint32(tvbuff_t *tvb, int offset, packet_info *pinfo _U_,
	       proto_tree *tree, guint32 *data, char *name)
{
	guint32 i;
	
	offset = prs_align(offset, 4);
	
	i = tvb_get_letohl(tvb, offset);
	offset += 4;

	if (name && tree)
		proto_tree_add_text(tree, tvb, offset - 4, 4, 
				    "%s: %u", name, i);

	if (data)
		*data = i;

	return offset;
}

/* Parse a number of 32-bit integers */

int prs_uint32s(tvbuff_t *tvb, int offset, packet_info *pinfo _U_,
		proto_tree *tree, int count, int *data_offset, char *name)
{
	offset = prs_align(offset, 4);
	
	if (name && tree)
		proto_tree_add_text(tree, tvb, offset - 4, 4, 
				    "%s", name);
	if (data_offset)
		*data_offset = offset;

	offset += count * 4;

	return offset;
}

/* Parse a NT status code */

int prs_ntstatus(tvbuff_t *tvb, int offset, packet_info *pinfo,
		 proto_tree *tree)
{
	guint32 status;

	offset = prs_uint32(tvb, offset, pinfo, tree, &status, NULL);

	if (tree)
		proto_tree_add_text(tree, tvb, offset - 4, 4, "Status: %s",
				    val_to_str(status, NT_errors, "???"));

	return offset;
}

/*
 * We need to keep track of deferred referrents as they appear in the
 * packet after all the non-pointer objects.
 * to keep track of pointers as they are parsed as scalars and need to be
 * remembered for the next call to the prs function.
 *
 * Pointers are stored in a linked list and pushed in the PARSE_SCALARS
 * section of the prs function and popped in the PARSE_BUFFERS section.  If
 * we try to pop off a referrent that has a different name then we are
 * expecting then something has gone wrong.
 */

#undef DEBUG_PTRS

struct ptr {
	char *name;
	guint32 value;
};

/* Create a new pointer */

static struct ptr *new_ptr(char *name, guint32 value)
{
	struct ptr *p;

	p = g_malloc(sizeof(struct ptr));

	p->name = g_strdup(name);
	p->value = value;

	return p;
}

/* Free a pointer */

static void free_ptr(struct ptr *p)
{
	if (p) {
		g_free(p->name);
		g_free(p);
	}
}

/* Parse a pointer and store it's value in a linked list */

int prs_push_ptr(tvbuff_t *tvb, int offset, packet_info *pinfo,
		 proto_tree *tree, GList **ptr_list, char *name)
{
	struct ptr *p;
	guint32 value;

	offset = prs_uint32(tvb, offset, pinfo, tree, &value, NULL);

	if (name && tree)
		proto_tree_add_text(tree, tvb, offset - 4, 4, 
				    "%s pointer: 0x%08x", name, value);

	p = new_ptr(name, value);

	*ptr_list = g_list_append(*ptr_list, p);

#ifdef DEBUG_PTRS
	fprintf(stderr, "DEBUG_PTRS: pushing %s ptr = 0x%08x, %d ptrs in "
		"list\n", name, value, g_list_length(*ptr_list));
#endif

	return offset;
}

/* Pop a pointer of a given name.  Return it's value. */

guint32 prs_pop_ptr(GList **ptr_list, char *name _U_)
{
	GList *elt;
	struct ptr *p;
	guint32 result;

	g_assert(g_list_length(*ptr_list) != 0);	/* List too short */

	/* Get pointer at head of list */

	elt = g_list_first(*ptr_list);
	p = (struct ptr *)elt->data;
	result = p->value;

#ifdef DEBUG_PTRS
	if (strcmp(p->name, name) != 0) {
		fprintf(stderr, "DEBUG_PTRS: wrong pointer (%s != %s)\n",
			p->name, name);
	}
#endif

	/* Free pointer record */

	*ptr_list = g_list_remove_link(*ptr_list, elt);

#ifdef DEBUG_PTRS
	fprintf(stderr, "DEBUG_PTRS: popping %s ptr = 0x%08x, %d ptrs in "
		"list\n", p->name, p->value, g_list_length(*ptr_list));
#endif

	free_ptr(p);

	return result;
}

/* Convert a string from little-endian unicode to ascii.  At the moment we
   fake it by taking every odd byte.  )-:  The caller must free the
   result returned. */

char *fake_unicode(tvbuff_t *tvb, int offset, int len)
{
	char *buffer;
	int i;
	guint16 character;

	/* Make sure we have enough data before allocating the buffer,
	   so we don't blow up if the length is huge.
	   We do so by attempting to fetch the last character; it'll
	   throw an exception if it's past the end. */
	tvb_get_letohs(tvb, offset + 2*(len - 1));

	/* We know we won't throw an exception, so we don't have to worry
	   about leaking this buffer. */
	buffer = g_malloc(len + 1);

	for (i = 0; i < len; i++) {
		character = tvb_get_letohs(tvb, offset);
		buffer[i] = character & 0xff;
		offset += 2;
	}

	buffer[len] = 0;

	return buffer;
}

/* Parse a UNISTR2 structure */

int prs_UNISTR2(tvbuff_t *tvb, int offset, packet_info *pinfo,
		proto_tree *tree, int flags, char **data, char *name _U_)
{
	guint32 len = 0, unknown = 0, max_len = 0;

	if (flags & PARSE_SCALARS) {
		offset = prs_uint32(tvb, offset, pinfo, tree, &len, "Length");
		offset = prs_uint32(tvb, offset, pinfo, tree, &unknown, 
				    "Offset");
		offset = prs_uint32(tvb, offset, pinfo, tree, &max_len, 
				    "Max length");
	}

	if (flags & PARSE_BUFFERS) {
		int data16_offset;

		offset = prs_uint16s(tvb, offset, pinfo, tree, max_len,
				     &data16_offset, "Buffer");

		if (data)
			*data = fake_unicode(tvb, data16_offset, max_len);
	}

	return offset;
}

/* following are a few functions for dissecting common structures used by NT 
   services. These might need to be cleaned up at a later time but at least we get
   them out of the real service dissectors.
*/


/* UNICODE_STRING  BEGIN */
/* functions to dissect a UNICODE_STRING structure, common to many 
   NT services
   struct {
     short len;
     short size;
     [size_is(size/2), length_is(len/2), ptr] unsigned short *string;
   } UNICODE_STRING;

   these variables can be found in packet-dcerpc-samr.c 
*/
extern int hf_nt_str_len;
extern int hf_nt_str_off;
extern int hf_nt_str_max_len;
extern int hf_nt_string_length;
extern int hf_nt_string_size;

gint ett_nt_unicode_string = -1;
static gint ett_nt_policy_hnd = -1;

/* this function will dissect the
     [size_is(size/2), length_is(len/2), ptr] unsigned short *string;
  part of the unicode string

   struct {
     short len;
     short size;
     [size_is(size/2), length_is(len/2), ptr] unsigned short *string;
   } UNICODE_STRING;
  structure used by NT to transmit unicode string values.

  This function also looks at di->levels to see if whoever called us wanted us to append
  the name: string to any higher levels in the tree .
*/
int
dissect_ndr_nt_UNICODE_STRING_str(tvbuff_t *tvb, int offset, 
			packet_info *pinfo, proto_tree *tree, 
			char *drep)
{
	guint32 len, off, max_len;
	int data16_offset;
	char *text;
	int old_offset;
	dcerpc_info *di;

	di=pinfo->private_data;
	if(di->conformant_run){
		/*just a run to handle conformant arrays, nothing to dissect */
		return offset;
	}

	offset = dissect_ndr_uint32 (tvb, offset, pinfo, tree, drep,
			hf_nt_str_max_len, &max_len);
	offset = dissect_ndr_uint32 (tvb, offset, pinfo, tree, drep,
			hf_nt_str_off, &off);
	offset = dissect_ndr_uint32 (tvb, offset, pinfo, tree, drep,
			hf_nt_str_len, &len);

	old_offset=offset;
	offset = prs_uint16s(tvb, offset, pinfo, tree, len, &data16_offset,
			NULL);
	text = fake_unicode(tvb, data16_offset, len);

	proto_tree_add_string(tree, di->hf_index, tvb, old_offset,
		offset-old_offset, text);

	/* need to test di->levels before doing the proto_item_append_text()
	   since netlogon has these objects as top level objects in its representation
	   and trying to append to the tree object in that case will dump core */
	if(tree && (di->levels>-1)){
		proto_item_append_text(tree, ": %s", text);
		di->levels--;
		if(di->levels>-1){
			tree=tree->parent;
			proto_item_append_text(tree, ": %s", text);
			di->levels--;
			while(di->levels>-1){
				tree=tree->parent;
				proto_item_append_text(tree, " %s", text);
				di->levels--;
			}
		}
	}
	g_free(text);
  	return offset;
}

/* this function will dissect the
   struct {
     short len;
     short size;
     [size_is(size/2), length_is(len/2), ptr] unsigned short *string;
   } UNICODE_STRING;
  structure used by NT to transmit unicode string values.
 
  the function takes one additional parameter, level
  which specifies how many additional levels up in the tree where we should
  append the string.  If unsure, specify levels as 0.
*/
int
dissect_ndr_nt_UNICODE_STRING(tvbuff_t *tvb, int offset, 
			packet_info *pinfo, proto_tree *parent_tree, 
			char *drep, int hf_index, int levels)
{
	proto_item *item=NULL;
	proto_tree *tree=NULL;
	int old_offset=offset;
	dcerpc_info *di;
	char *name;

	ALIGN_TO_4_BYTES;  /* strcture starts with short, but is aligned for longs */

	di=pinfo->private_data;
	if(di->conformant_run){
		/*just a run to handle conformant arrays, nothing to dissect */
		return offset;
	}

	name = proto_registrar_get_name(hf_index);
	if(parent_tree){
		item = proto_tree_add_text(parent_tree, tvb, offset, -1,
			"%s", name);
		tree = proto_item_add_subtree(item, ett_nt_unicode_string);
	}

	offset = dissect_ndr_uint16 (tvb, offset, pinfo, tree, drep,
			hf_nt_string_length, NULL);
	offset = dissect_ndr_uint16 (tvb, offset, pinfo, tree, drep,
			hf_nt_string_size, NULL);
	di->levels=1;	/* XXX - is this necessary? */
	/* Add 1 level, for the extra level we added */
	offset = dissect_ndr_pointer(tvb, offset, pinfo, tree, drep,
			dissect_ndr_nt_UNICODE_STRING_str, NDR_POINTER_UNIQUE,
			name, hf_index, levels + 1);

	proto_item_set_len(item, offset-old_offset);
	return offset;
}
/* UNICODE_STRING  END */

/* functions to dissect a STRING structure, common to many 
   NT services
   struct {
     short len;
     short size;
     [size_is(size), length_is(len), ptr] char *string;
   } STRING;
*/
int
dissect_ndr_nt_STRING_string (tvbuff_t *tvb, int offset, 
                             packet_info *pinfo, proto_tree *tree, 
                             char *drep)
{
	guint32 len, off, max_len;
	int text_offset;
	const guint8 *text;
	int old_offset;
	header_field_info *hfi;
	dcerpc_info *di;

	di=pinfo->private_data;
	if(di->conformant_run){
		/*just a run to handle conformant arrays, nothing to dissect */
		return offset;
	}

        offset = dissect_ndr_uint32 (tvb, offset, pinfo, tree, drep,
                                     hf_nt_str_max_len, &max_len);
        offset = dissect_ndr_uint32 (tvb, offset, pinfo, tree, drep,
                                     hf_nt_str_off, &off);
        offset = dissect_ndr_uint32 (tvb, offset, pinfo, tree, drep,
                                     hf_nt_str_len, &len);

	old_offset=offset;
	hfi = proto_registrar_get_nth(di->hf_index);

	switch(hfi->type){
	case FT_STRING:
		offset = prs_uint8s(tvb, offset, pinfo, tree, len,
			&text_offset, NULL);
		text = tvb_get_ptr(tvb, text_offset, len);
		proto_tree_add_string_format(tree, di->hf_index, 
			tvb, old_offset, offset-old_offset,
			text, "%s: %s", hfi->name, text);
		break;
	case FT_BYTES:
		text = NULL;
		proto_tree_add_item(tree, di->hf_index, tvb, offset, len, FALSE);
		offset += len;
		break;
	default:
		text = NULL;
		g_assert_not_reached();
	}

	if(tree && text && (di->levels>-1)){
		proto_item_append_text(tree, ": %s", text);
		if(di->levels>-1){
			tree=tree->parent;
			proto_item_append_text(tree, ": %s", text);
			while(di->levels>0){
				tree=tree->parent;
				proto_item_append_text(tree, " %s", text);
				di->levels--;
			}
		}
	}
  	return offset;
}

int
dissect_ndr_nt_STRING (tvbuff_t *tvb, int offset, 
                             packet_info *pinfo, proto_tree *parent_tree, 
                             char *drep, int hf_index, int levels)
{
	proto_item *item=NULL;
	proto_tree *tree=NULL;
	int old_offset=offset;
	dcerpc_info *di;
	char *name;

	ALIGN_TO_4_BYTES;  /* strcture starts with short, but is aligned for longs */

	di=pinfo->private_data;
	if(di->conformant_run){
		/*just a run to handle conformant arrays, nothing to dissect */
		return offset;
	}

	name = proto_registrar_get_name(hf_index);
	if(parent_tree){
		item = proto_tree_add_text(parent_tree, tvb, offset, -1,
			"%s", name);
		tree = proto_item_add_subtree(item, ett_nt_unicode_string);
	}

        offset = dissect_ndr_uint16 (tvb, offset, pinfo, tree, drep,
                                     hf_nt_string_length, NULL);
        offset = dissect_ndr_uint16 (tvb, offset, pinfo, tree, drep,
                                     hf_nt_string_size, NULL);
        offset = dissect_ndr_pointer(tvb, offset, pinfo, tree, drep,
			dissect_ndr_nt_STRING_string, NDR_POINTER_UNIQUE,
			name, hf_index, levels);

	proto_item_set_len(item, offset-old_offset);
	return offset;
}


/* This function is used to dissect a DCERPC encoded 64 bit time value.
   XXX it should be fixed both here and in dissect_smb_64bit_time so
   it can handle both BIG and LITTLE endian encodings 
 */
int
dissect_ndr_nt_NTTIME (tvbuff_t *tvb, int offset, 
			packet_info *pinfo, proto_tree *tree, 
			char *drep _U_, int hf_index)
{
	dcerpc_info *di;

	di=pinfo->private_data;
	if(di->conformant_run){
		/*just a run to handle conformant arrays, nothing to dissect */
		return offset;
	}

	ALIGN_TO_4_BYTES;

	offset = dissect_smb_64bit_time(tvb, tree, offset, hf_index);
	return offset;
}

/* Define this symbol to display warnings about request/response and
   policy handle hash table collisions.  This happens when a packet with
   the same conversation, smb fid and dcerpc call id occurs.  I think this
   is due to a bug in the dcerpc/smb fragment reassembly code. */

#undef DEBUG_HASH_COLL

/*
 * Policy handle hashing
 */

typedef struct {
	guint8 policy_hnd[20];
} pol_hash_key;

typedef struct {
	guint32 open_frame, close_frame; /* Frame numbers for open/close */
	char *name;			 /* Name of policy handle */
} pol_hash_value;

#define POL_HASH_INIT_COUNT 100

static GHashTable *pol_hash;
static GMemChunk *pol_hash_key_chunk;
static GMemChunk *pol_hash_value_chunk;

/* Hash function */

static guint pol_hash_fn(gconstpointer k)
{
	pol_hash_key *key = (pol_hash_key *)k;

	/* Bytes 4-7 of the policy handle are a timestamp so should make a
	   reasonable hash value */
	
	return key->policy_hnd[4] + (key->policy_hnd[5] << 8) +
		(key->policy_hnd[6] << 16) + (key->policy_hnd[7] << 24);
}

/* Return true if a policy handle is all zeros */

static gboolean is_null_pol(e_ctx_hnd *policy_hnd)
{
	static guint8 null_policy_hnd[20];

	return memcmp(policy_hnd, null_policy_hnd, 20) == 0;
}

/* Hash compare function */

static gint pol_hash_compare(gconstpointer k1, gconstpointer k2)
{
	pol_hash_key *key1 = (pol_hash_key *)k1;
	pol_hash_key *key2 = (pol_hash_key *)k2;

	return memcmp(key1->policy_hnd, key2->policy_hnd, 
		      sizeof(key1->policy_hnd)) == 0;
}

/* Store the open and close frame numbers of a policy handle */

void dcerpc_smb_store_pol_pkts(e_ctx_hnd *policy_hnd, guint32 open_frame, 
			       guint32 close_frame)
{
	pol_hash_key *key;
	pol_hash_value *value;

	if (is_null_pol(policy_hnd) || (open_frame == 0 && close_frame == 0))
		return;

	/* Look up existing value */

	key = g_mem_chunk_alloc(pol_hash_key_chunk);

	memcpy(&key->policy_hnd, policy_hnd, sizeof(key->policy_hnd));

	if ((value = g_hash_table_lookup(pol_hash, key))) {

		/* Update existing value */

		if (open_frame) {
#ifdef DEBUG_HASH_COLL
			if (value->open_frame != open_frame)
				g_warning("dcerpc_smb: pol_hash open frame collision %d/%d\n", value->open_frame, open_frame);
#endif
			value->open_frame = open_frame;
		}

		if (close_frame) {
#ifdef DEBUG_HASH_COLL
			if (value->close_frame != close_frame)
				g_warning("dcerpc_smb: pol_hash close frame collision %d/%d\n", value->close_frame, close_frame);
#endif
			value->close_frame = close_frame;
		}

		return;
	}

	/* Create a new value */

	value = g_mem_chunk_alloc(pol_hash_value_chunk);

	value->open_frame = open_frame;
	value->close_frame = close_frame;

	value->name = NULL;

	g_hash_table_insert(pol_hash, key, value);
}

/* Store a text string with a policy handle */

void dcerpc_smb_store_pol_name(e_ctx_hnd *policy_hnd, char *name)
{
	pol_hash_key *key;
	pol_hash_value *value;

	if (is_null_pol(policy_hnd))
		return;

	/* Look up existing value */

	key = g_mem_chunk_alloc(pol_hash_key_chunk);

	memcpy(&key->policy_hnd, policy_hnd, sizeof(key->policy_hnd));

	if ((value = g_hash_table_lookup(pol_hash, key))) {

		/* Update existing value */

		if (value->name && name) {
#ifdef DEBUG_HASH_COLL
			if (strcmp(value->name, name) != 0)
				g_warning("dcerpc_smb: pol_hash name collision %s/%s\n", value->name, name);
#endif
			free(value->name);
		}

		value->name = strdup(name);

		return;
	}

	/* Create a new value */

	value = g_mem_chunk_alloc(pol_hash_value_chunk);

	value->open_frame = 0;
	value->close_frame = 0;

	if (name)
		value->name = strdup(name);
	else
		value->name = strdup("UNKNOWN");

	g_hash_table_insert(pol_hash, key, value);
}

/* Retrieve a policy handle */

gboolean dcerpc_smb_fetch_pol(e_ctx_hnd *policy_hnd, char **name, 
			      guint32 *open_frame, guint32 *close_frame)
{
	pol_hash_key key;
	pol_hash_value *value;

	/* Prevent uninitialised return vars */

	if (name)
		*name = NULL;

	if (open_frame)
		*open_frame = 0;

	if (close_frame)
		*close_frame = 0;

	/* Look up existing value */

	memcpy(&key.policy_hnd, policy_hnd, sizeof(key.policy_hnd));

	value = g_hash_table_lookup(pol_hash, &key);

	/* Return name and frame numbers */
	
	if (value) {
		if (name)
			*name = value->name;

		if (open_frame)
			*open_frame = value->open_frame;

		if (close_frame)
			*close_frame = value->close_frame;
	}

	return value != NULL;
}

/* Iterator to free a policy handle key/value pair */

static void free_pol_keyvalue(gpointer key _U_, gpointer value,
    gpointer user_data _U_)
{
	pol_hash_value *pol_value = (pol_hash_value *)value;

	/* Free user data */

	if (pol_value->name) {
		free(pol_value->name);
		pol_value->name = NULL;
	}
}

/* Initialise policy handle hash */

static void init_pol_hash(void)
{
	/* Initialise memory chunks */

	if (pol_hash_key_chunk)
		g_mem_chunk_destroy(pol_hash_key_chunk);

	pol_hash_key_chunk = g_mem_chunk_new(
		"Policy handle hash keys", sizeof(pol_hash_key),
		POL_HASH_INIT_COUNT * sizeof(pol_hash_key), G_ALLOC_ONLY);
					    
	if (pol_hash_value_chunk)
		g_mem_chunk_destroy(pol_hash_value_chunk);

	pol_hash_value_chunk = g_mem_chunk_new(
		"Policy handle hash values", sizeof(pol_hash_value),
		POL_HASH_INIT_COUNT * sizeof(pol_hash_value), G_ALLOC_ONLY);

	/* Initialise hash table */

	if (pol_hash) {
		g_hash_table_foreach(pol_hash, free_pol_keyvalue, NULL);
		g_hash_table_destroy(pol_hash);
	}

	pol_hash = g_hash_table_new(pol_hash_fn, pol_hash_compare);
}

/* Dissect an access mask.  All this stuff is kind of explained at MSDN:

http://msdn.microsoft.com/library/default.asp?url=/library/en-us/security/security/windows_2000_windows_nt_access_mask_format.asp 

*/

static gint ett_nt_access_mask = -1;
static gint ett_nt_access_mask_generic = -1;
static gint ett_nt_access_mask_standard = -1;
static gint ett_nt_access_mask_specific = -1;

static int hf_access_sacl = -1;
static int hf_access_maximum_allowed = -1;
static int hf_access_generic_read = -1;
static int hf_access_generic_write = -1;
static int hf_access_generic_execute = -1;
static int hf_access_generic_all = -1;
static int hf_access_standard_delete = -1;
static int hf_access_standard_read_control = -1;
static int hf_access_standard_synchronise = -1;
static int hf_access_standard_write_dac = -1;
static int hf_access_standard_write_owner = -1;
static int hf_access_specific_15 = -1;
static int hf_access_specific_14 = -1;
static int hf_access_specific_13 = -1;
static int hf_access_specific_12 = -1;
static int hf_access_specific_11 = -1;
static int hf_access_specific_10 = -1;
static int hf_access_specific_9 = -1;
static int hf_access_specific_8 = -1;
static int hf_access_specific_7 = -1;
static int hf_access_specific_6 = -1;
static int hf_access_specific_5 = -1;
static int hf_access_specific_4 = -1;
static int hf_access_specific_3 = -1;
static int hf_access_specific_2 = -1;
static int hf_access_specific_1 = -1;
static int hf_access_specific_0 = -1;

int
dissect_nt_access_mask(tvbuff_t *tvb, gint offset, packet_info *pinfo,
		       proto_tree *tree, char *drep, int hfindex,
		       nt_access_mask_fn_t *specific_rights_fn)
{
	proto_item *item;
	proto_tree *subtree, *generic, *standard, *specific;
	guint32 access;

	offset = dissect_ndr_uint32(tvb, offset, pinfo, NULL, drep,
				    hfindex, &access);

	item = proto_tree_add_text(tree, tvb, offset - 4, 4, 
				   "Access Mask: 0x%08x", access);

	subtree = proto_item_add_subtree(item, ett_nt_access_mask);

	/* Generic access rights */

	item = proto_tree_add_text(subtree, tvb, offset - 4, 4,
				   "Generic rights: 0x%08x",
				   access & GENERIC_RIGHTS_MASK);

	generic = proto_item_add_subtree(item, ett_nt_access_mask_generic);

	proto_tree_add_boolean(
		generic, hf_access_generic_read, tvb, offset - 4, 4,
		access);

	proto_tree_add_boolean(
		generic, hf_access_generic_write, tvb, offset - 4, 4,
		access);

	proto_tree_add_boolean(
		generic, hf_access_generic_execute, tvb, offset - 4, 4,
		access);

	proto_tree_add_boolean(
		generic, hf_access_generic_all, tvb, offset - 4, 4,
		access);

	/* Reserved (??) */

	proto_tree_add_boolean(
		subtree, hf_access_maximum_allowed, tvb, offset - 4, 4,
		access);

	/* Access system security */

	proto_tree_add_boolean(
		subtree, hf_access_sacl, tvb, offset - 4, 4,
		access);

	/* Standard access rights */

	item = proto_tree_add_text(subtree, tvb, offset - 4, 4,
				   "Standard rights: 0x%08x",
				   access & STANDARD_RIGHTS_MASK);

	standard = proto_item_add_subtree(item, ett_nt_access_mask_standard);

	proto_tree_add_boolean(
		standard, hf_access_standard_synchronise, tvb, offset - 4, 4,
		access);

	proto_tree_add_boolean(
		standard, hf_access_standard_write_owner, tvb, offset - 4, 4,
		access);
	
	proto_tree_add_boolean(
		standard, hf_access_standard_write_dac, tvb, offset - 4, 4,
		access);
	
	proto_tree_add_boolean(
		standard, hf_access_standard_read_control, tvb, offset - 4, 4,
		access);

	proto_tree_add_boolean(
		standard, hf_access_standard_delete, tvb, offset - 4, 4,
		access);

	/* Specific access rights.  Call the specific_rights_fn
	   pointer if we have one, otherwise just display bits 0-15 in
	   boring fashion. */

	item = proto_tree_add_text(subtree, tvb, offset - 4, 4,
				   "Specific rights: 0x%08x",
				   access & SPECIFIC_RIGHTS_MASK);

	specific = proto_item_add_subtree(item, ett_nt_access_mask_specific);

	if (specific_rights_fn) {
		specific_rights_fn(tvb, offset - 4, specific, access);
		return offset;
	}

	proto_tree_add_boolean(
		specific, hf_access_specific_15, tvb, offset - 4, 4,
		access);
 
	proto_tree_add_boolean(
		specific, hf_access_specific_14, tvb, offset - 4, 4,
		access);
 
	proto_tree_add_boolean(
		specific, hf_access_specific_13, tvb, offset - 4, 4,
		access);
 
	proto_tree_add_boolean(
		specific, hf_access_specific_12, tvb, offset - 4, 4,
		access);
 
	proto_tree_add_boolean(
		specific, hf_access_specific_11, tvb, offset - 4, 4,
		access);
 
	proto_tree_add_boolean(
		specific, hf_access_specific_10, tvb, offset - 4, 4,
		access);
 
	proto_tree_add_boolean(
		specific, hf_access_specific_9, tvb, offset - 4, 4,
		access);
 
	proto_tree_add_boolean(
		specific, hf_access_specific_8, tvb, offset - 4, 4,
		access);
 
	proto_tree_add_boolean(
		specific, hf_access_specific_7, tvb, offset - 4, 4,
		access);
 
	proto_tree_add_boolean(
		specific, hf_access_specific_6, tvb, offset - 4, 4,
		access);
 
	proto_tree_add_boolean(
		specific, hf_access_specific_5, tvb, offset - 4, 4,
		access);
 
	proto_tree_add_boolean(
		specific, hf_access_specific_4, tvb, offset - 4, 4,
		access);
 
	proto_tree_add_boolean(
		specific, hf_access_specific_3, tvb, offset - 4, 4,
		access);
 
	proto_tree_add_boolean(
		specific, hf_access_specific_2, tvb, offset - 4, 4,
		access);
 
	proto_tree_add_boolean(
		specific, hf_access_specific_1, tvb, offset - 4, 4,
		access);
 
	proto_tree_add_boolean(
		specific, hf_access_specific_0, tvb, offset - 4, 4,
		access);
 
	return offset;
}

/*
 * Initialise global DCERPC/SMB data structures
 */

static void dcerpc_smb_init(void)
{
	/* Initialise policy handle hash */

	init_pol_hash();
}

/*
 * Register ett_ values, and register "dcerpc_smb_init()" as an
 * initialisation routine.
 */
void proto_register_dcerpc_smb(void)
{
	static hf_register_info hf[] = {

		/* Access mask */

		{ &hf_access_generic_read,
		  { "Generic read", "nt.access_mask.generic_read",
		    FT_BOOLEAN, 32, TFS(&flags_set_truth), 
		    GENERIC_READ_ACCESS, "Generic read", HFILL }},

		{ &hf_access_generic_write,
		  { "Generic write", "nt.access_mask.generic_write",
		    FT_BOOLEAN, 32, TFS(&flags_set_truth), 
		    GENERIC_WRITE_ACCESS, "Generic write", HFILL }},

		{ &hf_access_generic_execute,
		  { "Generic execute", "nt.access_mask.generic_execute",
		    FT_BOOLEAN, 32, TFS(&flags_set_truth), 
		    GENERIC_EXECUTE_ACCESS, "Generic execute", HFILL }},

		{ &hf_access_generic_all,
		  { "Generic all", "nt.access_mask.generic_all",
		    FT_BOOLEAN, 32, TFS(&flags_set_truth), 
		    GENERIC_ALL_ACCESS, "Generic all", HFILL }},

		{ &hf_access_maximum_allowed,
		  { "Maximum allowed", "nt.access_mask.maximum_allowed",
		    FT_BOOLEAN, 32, TFS(&flags_set_truth), 
		    MAXIMUM_ALLOWED_ACCESS, "Maximum allowed", HFILL }},

		{ &hf_access_sacl,
		  { "Access SACL", "nt.access_mask.access_sacl",
		    FT_BOOLEAN, 32, TFS(&flags_set_truth),
		    ACCESS_SACL_ACCESS, "Access SACL", HFILL }},

		{ &hf_access_standard_read_control,
		  { "Read control", "nt.access_mask.read_control",
		    FT_BOOLEAN, 32, TFS(&flags_set_truth),
		    READ_CONTROL_ACCESS, "Read control", HFILL }},

		{ &hf_access_standard_delete,
		  { "Delete", "nt.access_mask.delete",
		    FT_BOOLEAN, 32, TFS(&flags_set_truth),
		    DELETE_ACCESS, "Delete", HFILL }},

		{ &hf_access_standard_synchronise,
		  { "Synchronise", "nt.access_mask.synchronise",
		    FT_BOOLEAN, 32, TFS(&flags_set_truth),
		    SYNCHRONIZE_ACCESS, "Synchronise", HFILL }},

		{ &hf_access_standard_write_dac,
		  { "Write DAC", "nt.access_mask.write_dac",
		    FT_BOOLEAN, 32, TFS(&flags_set_truth),
		    WRITE_DAC_ACCESS, "Write DAC", HFILL }},

		{ &hf_access_standard_write_owner,
		  { "Write owner", "nt.access_mask.write_owner",
		    FT_BOOLEAN, 32, TFS(&flags_set_truth),
		    WRITE_OWNER_ACCESS, "Write owner", HFILL }},

		{ &hf_access_specific_15,
		  { "Specific access, bit 15", "nt.access_mask.specific_15",
		    FT_BOOLEAN, 32, TFS(&flags_set_truth),
		    0x8000, "Specific access, bit 15", HFILL }},

		{ &hf_access_specific_14,
		  { "Specific access, bit 14", "nt.access_mask.specific_14",
		    FT_BOOLEAN, 32, TFS(&flags_set_truth),
		    0x4000, "Specific access, bit 14", HFILL }},

		{ &hf_access_specific_13,
		  { "Specific access, bit 13", "nt.access_mask.specific_13",
		    FT_BOOLEAN, 32, TFS(&flags_set_truth),
		    0x2000, "Specific access, bit 13", HFILL }},

		{ &hf_access_specific_12,
		  { "Specific access, bit 12", "nt.access_mask.specific_12",
		    FT_BOOLEAN, 32, TFS(&flags_set_truth),
		    0x1000, "Specific access, bit 12", HFILL }},

		{ &hf_access_specific_11,
		  { "Specific access, bit 11", "nt.access_mask.specific_11",
		    FT_BOOLEAN, 32, TFS(&flags_set_truth),
		    0x0800, "Specific access, bit 11", HFILL }},

		{ &hf_access_specific_10,
		  { "Specific access, bit 10", "nt.access_mask.specific_10",
		    FT_BOOLEAN, 32, TFS(&flags_set_truth),
		    0x0400, "Specific access, bit 10", HFILL }},

		{ &hf_access_specific_9,
		  { "Specific access, bit 9", "nt.access_mask.specific_9",
		    FT_BOOLEAN, 32, TFS(&flags_set_truth),
		    0x0200, "Specific access, bit 9", HFILL }},

		{ &hf_access_specific_8,
		  { "Specific access, bit 8", "nt.access_mask.specific_8",
		    FT_BOOLEAN, 32, TFS(&flags_set_truth),
		    0x0100, "Specific access, bit 8", HFILL }},

		{ &hf_access_specific_7,
		  { "Specific access, bit 7", "nt.access_mask.specific_7",
		    FT_BOOLEAN, 32, TFS(&flags_set_truth),
		    0x0080, "Specific access, bit 7", HFILL }},

		{ &hf_access_specific_6,
		  { "Specific access, bit 6", "nt.access_mask.specific_6",
		    FT_BOOLEAN, 32, TFS(&flags_set_truth),
		    0x0040, "Specific access, bit 6", HFILL }},

		{ &hf_access_specific_5,
		  { "Specific access, bit 5", "nt.access_mask.specific_5",
		    FT_BOOLEAN, 32, TFS(&flags_set_truth),
		    0x0020, "Specific access, bit 5", HFILL }},

		{ &hf_access_specific_4,
		  { "Specific access, bit 4", "nt.access_mask.specific_4",
		    FT_BOOLEAN, 32, TFS(&flags_set_truth),
		    0x0010, "Specific access, bit 4", HFILL }},

		{ &hf_access_specific_3,
		  { "Specific access, bit 3", "nt.access_mask.specific_3",
		    FT_BOOLEAN, 32, TFS(&flags_set_truth),
		    0x0008, "Specific access, bit 3", HFILL }},

		{ &hf_access_specific_2,
		  { "Specific access, bit 2", "nt.access_mask.specific_2",
		    FT_BOOLEAN, 32, TFS(&flags_set_truth),
		    0x0004, "Specific access, bit 2", HFILL }},

		{ &hf_access_specific_1,
		  { "Specific access, bit 1", "nt.access_mask.specific_1",
		    FT_BOOLEAN, 32, TFS(&flags_set_truth),
		    0x0002, "Specific access, bit 1", HFILL }},

		{ &hf_access_specific_0,
		  { "Specific access, bit 0", "nt.access_mask.specific_0",
		    FT_BOOLEAN, 32, TFS(&flags_set_truth),
		    0x0001, "Specific access, bit 0", HFILL }},
	};

	static gint *ett[] = {
		&ett_nt_unicode_string,
		&ett_nt_policy_hnd,
		&ett_nt_access_mask,
		&ett_nt_access_mask_generic,
		&ett_nt_access_mask_standard,
		&ett_nt_access_mask_specific,
	};

	static int proto_dcerpc_nt = -1;

	/* Register ett's */

	proto_register_subtree_array(ett, array_length(ett));

	/* Register hf's */

	proto_dcerpc_nt = proto_register_protocol(
		"Dummy Protocol", "DCERPC_NT", "DCERPC_NT");

	proto_register_field_array(proto_dcerpc_nt, hf, array_length(hf));

        /* Register a routine to be called whenever initialisation
           is done. */

        register_init_routine(dcerpc_smb_init);
}

/* Check if there is unparsed data remaining in a frame and display an
   error.  I guess this could be made into an exception like the malformed
   frame exception.  For the DCERPC over SMB dissectors a long frame
   indicates a bug in a dissector. */

void dcerpc_smb_check_long_frame(tvbuff_t *tvb, int offset, 
				 packet_info *pinfo, proto_tree *tree)
{
	if (tvb_length_remaining(tvb, offset) != 0) {

		proto_tree_add_text(
			tree, tvb, offset, tvb_length_remaining(tvb, offset), 
			"[Long frame (%d bytes): SPOOLSS]",
			tvb_length_remaining(tvb, offset));

		if (check_col(pinfo->cinfo, COL_INFO))
			col_append_fstr(pinfo->cinfo, COL_INFO,
					"[Long frame (%d bytes): SPOOLSS]",
					tvb_length_remaining(tvb, offset));
	}
}

/* Dissect a NT status code */

int
dissect_ntstatus(tvbuff_t *tvb, gint offset, packet_info *pinfo,
		 proto_tree *tree, char *drep, 
		 int hfindex, guint32 *pdata)
{
	guint32 status;

	offset = dissect_ndr_uint32(tvb, offset, pinfo, tree, drep,
				    hfindex, &status);

	if (tree && status != 0 && check_col(pinfo->cinfo, COL_INFO))
		col_append_fstr(pinfo->cinfo, COL_INFO, ", %s",
				val_to_str(status, NT_errors, 
					   "Unknown error 0x%08x"));
	if (pdata)
		*pdata = status;

	return offset;
}

/* Dissect a DOS status code */

int
dissect_doserror(tvbuff_t *tvb, gint offset, packet_info *pinfo,
	       proto_tree *tree, char *drep, 
	       int hfindex, guint32 *pdata)
{
	guint32 status;

	offset = dissect_ndr_uint32(tvb, offset, pinfo, tree, drep,
				    hfindex, &status);

	if (tree && status != 0 && check_col(pinfo->cinfo, COL_INFO))
		col_append_fstr(pinfo->cinfo, COL_INFO, ", %s",
				val_to_str(status, DOS_errors, 
					   "Unknown error 0x%08x"));
	if (pdata)
		*pdata = status;

	return offset;
}

/* Dissect a NT policy handle */

int
dissect_nt_policy_hnd(tvbuff_t *tvb, gint offset, packet_info *pinfo,
		      proto_tree *tree, char *drep, int hfindex, 
		      e_ctx_hnd *pdata, gboolean is_open, gboolean is_close)
{
	proto_item *item;
	proto_tree *subtree;
	e_ctx_hnd hnd;
	guint32 open_frame = 0, close_frame = 0;
	char *name;
	int old_offset = offset;

	/* Add to proto tree */

	item = proto_tree_add_text(tree, tvb, offset, sizeof(e_ctx_hnd), 
				   "Policy Handle");

	subtree = proto_item_add_subtree(item, ett_nt_policy_hnd);

	offset = dissect_ndr_ctx_hnd(tvb, offset, pinfo, subtree, drep, 
				     hfindex, &hnd); 

	/* Store request/reply information */
		
	dcerpc_smb_store_pol_pkts(&hnd, 0, is_close ? pinfo->fd->num : 0);
	dcerpc_smb_store_pol_pkts(&hnd, is_open ? pinfo->fd->num: 0, 0);

	/* Insert request/reply information if known */

	if (dcerpc_smb_fetch_pol(&hnd, &name, &open_frame, &close_frame)) {

		if (open_frame)
			proto_tree_add_text(subtree, tvb, old_offset,
					    sizeof(e_ctx_hnd),
					    "Opened in frame %u", open_frame);

		if (close_frame)
			proto_tree_add_text(subtree, tvb, old_offset,
					    sizeof(e_ctx_hnd),
					    "Closed in frame %u", close_frame);
		if (name != NULL)
			proto_item_append_text(item, ": %s", name);
	}

	if (pdata)
		*pdata = hnd;

	return offset;
}

/* Some helper routines to dissect a range of uint8 characters.  I don't
   think these are "official" NDR representations and are probably specific
   to NT so for the moment they're put here instead of in packet-dcerpc.c
   and packet-dcerpc-ndr.c. */

int
dissect_dcerpc_uint8s(tvbuff_t *tvb, gint offset, packet_info *pinfo _U_,
                      proto_tree *tree, char *drep, int hfindex, 
		      int length, guint8 **pdata)
{
    guint8 *data;

    data = (guint8 *)tvb_get_ptr(tvb, offset, length);

    if (tree) {
        proto_tree_add_item (tree, hfindex, tvb, offset, length, (drep[0] & 0x10));
    }

    if (pdata)
        *pdata = data;

    return offset + length;
}

int
dissect_ndr_uint8s(tvbuff_t *tvb, gint offset, packet_info *pinfo,
                   proto_tree *tree, char *drep, 
                   int hfindex, int length, guint8 **pdata)
{
    dcerpc_info *di;

    di=pinfo->private_data;
    if(di->conformant_run){
      /* just a run to handle conformant arrays, no scalars to dissect */
      return offset;
    }

    /* no alignment needed */
    return dissect_dcerpc_uint8s(tvb, offset, pinfo, 
                                 tree, drep, hfindex, length, pdata);
}

int
dissect_dcerpc_uint16s(tvbuff_t *tvb, gint offset, packet_info *pinfo _U_,
                      proto_tree *tree, char *drep, int hfindex, 
		      int length, guint16 **pdata)
{
    guint16 *data;

    data = (guint16 *)tvb_get_ptr(tvb, offset, length * 2);

    if (tree) {
        proto_tree_add_item (tree, hfindex, tvb, offset, length * 2, (drep[0] & 0x10));
    }

    if (pdata)
        *pdata = data;

    return offset + length * 2;
}

int
dissect_ndr_uint16s(tvbuff_t *tvb, gint offset, packet_info *pinfo,
                   proto_tree *tree, char *drep, 
                   int hfindex, int length, guint16 **pdata)
{
    dcerpc_info *di;

    di=pinfo->private_data;
    if(di->conformant_run){
      /* just a run to handle conformant arrays, no scalars to dissect */
      return offset;
    }

    if (offset % 2)
        offset++;

    return dissect_dcerpc_uint16s(tvb, offset, pinfo, 
                                 tree, drep, hfindex, length, pdata);
}
