/* proto.c
 * Routines for protocol tree
 *
 * $Id: proto.c,v 1.31 1999/10/04 18:09:05 gram Exp $
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@zing.org>
 * Copyright 1998 Gerald Combs
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

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#ifndef _STDIO_H
#include <stdio.h>
#endif

#include <stdarg.h>

#ifndef _STRING_H
#include <string.h>
#endif

#ifdef NEED_SNPRINTF_H
# include "snprintf.h"
#endif

#ifndef __G_LIB_H__
#include <glib.h>
#endif

#ifndef __PROTO_H__
#include "proto.h"
#endif

#ifndef __PACKET_H__
#include "packet.h"
#endif

#ifndef __RESOLV_H__
#include "resolv.h"
#endif

#define cVALS(x) (const value_string*)(x)

#if defined(HAVE_UCD_SNMP_SNMP_H)
# define WITH_SNMP_UCD 1
#elif defined(HAVE_SNMP_SNMP_H)
# define WITH_SNMP_CMU 1
#endif

static gboolean
proto_tree_free_node(GNode *node, gpointer data);

static struct header_field_info*
find_hfinfo_record(int hfindex);

static proto_item *
proto_tree_add_item_value(proto_tree *tree, int hfindex, gint start,
	gint length, int include_format, int visible, va_list ap);

static gboolean check_for_protocol_or_field_id(GNode *node, gpointer data);
static gboolean check_for_field_within_protocol(GNode *node, gpointer data);

static int proto_register_field_init(header_field_info *hfinfo, int parent);

/* centralization of registration functions */
void proto_register_aarp(void);
void proto_register_arp(void);
void proto_register_ascend(void);
void proto_register_atalk(void);
void proto_register_atm(void);
void proto_register_bootp(void);
void proto_register_cdp(void);
void proto_register_clnp(void);
void proto_register_cotp(void);
void proto_register_data(void);
void proto_register_dns(void);
void proto_register_eth(void);
void proto_register_fddi(void);
void proto_register_frame(void);
void proto_register_ftp(void);
void proto_register_giop(void);
void proto_register_gre(void);
void proto_register_http(void);
void proto_register_icmp(void);
void proto_register_icmpv6(void);
void proto_register_icp(void);
void proto_register_igmp(void);
void proto_register_ip(void);
void proto_register_ipp(void);
void proto_register_ipsec(void);
void proto_register_ipv6(void);
void proto_register_ipx(void);
void proto_register_isakmp(void);
void proto_register_lapb(void);
void proto_register_llc(void);
void proto_register_mp(void);
void proto_register_nbipx(void);
void proto_register_nbt(void);
void proto_register_ncp(void);
void proto_register_netbios(void);
void proto_register_nntp(void);
void proto_register_null(void);
void proto_register_ospf(void);
void proto_register_pop(void);
void proto_register_ppp(void);
void proto_register_radius(void);
void proto_register_rip(void);
void proto_register_rsvp(void);
void proto_register_rtsp(void);
void proto_register_sdp(void);
void proto_register_smb(void);
#if defined(WITH_SNMP_CMU) || defined(WITH_SNMP_UCD)
void proto_register_snmp(void);
#endif
void proto_register_telnet(void);
void proto_register_tftp(void);
void proto_register_tcp(void);
void proto_register_tr(void);
void proto_register_trmac(void);
void proto_register_udp(void);
void proto_register_x25(void);

/* special-case header field used within proto.c */
int hf_text_only = 1;

/* Contains information about protocols and header fields. Used when
 * dissectors register their data */
GMemChunk *gmc_hfinfo = NULL;

/* Contains information about a field when a dissector calls
 * proto_tree_add_item.  */
GMemChunk *gmc_field_info = NULL;

/* String space for protocol and field items for the GUI */
GMemChunk *gmc_item_labels = NULL;

/* List which stores protocols and fields that have been registered */
GPtrArray *gpa_hfinfo = NULL;

/* Is the parsing being done for a visible proto_tree or an invisible one?
 * By setting this correctly, the proto_tree creation is sped up by not
 * having to call vsnprintf and copy strings around.
 */
gboolean proto_tree_is_visible = TRUE;

/* initialize data structures and register protocols and fields */
void
proto_init(void)
{
	if (gmc_hfinfo)
		g_mem_chunk_destroy(gmc_hfinfo);
	if (gmc_field_info)
		g_mem_chunk_destroy(gmc_field_info);
	if (gmc_item_labels)
		g_mem_chunk_destroy(gmc_item_labels);
	if (gpa_hfinfo)
		g_ptr_array_free(gpa_hfinfo, FALSE);

	gmc_hfinfo = g_mem_chunk_new("gmc_hfinfo",
		sizeof(struct header_field_info), 50 * sizeof(struct 
		header_field_info), G_ALLOC_ONLY);
	gmc_field_info = g_mem_chunk_new("gmc_field_info",
		sizeof(struct field_info), 200 * sizeof(struct field_info),
		G_ALLOC_AND_FREE);
	gmc_item_labels = g_mem_chunk_new("gmc_item_labels",
		ITEM_LABEL_LENGTH, 20 * ITEM_LABEL_LENGTH,
		G_ALLOC_AND_FREE);
	gpa_hfinfo = g_ptr_array_new();

	/* Have each dissector register its protocols and fields. The
	 * order doesn't matter. Put the calls in alphabetical order
	 * just to make it easy. */
	proto_register_aarp();
	proto_register_arp();
	proto_register_ascend();
	proto_register_atalk();
	proto_register_atm();
	proto_register_bootp();
	proto_register_cdp();
	proto_register_clnp();
	proto_register_cotp();
	proto_register_data();
	proto_register_dns();
	proto_register_eth();
	proto_register_fddi();
	proto_register_frame();
	proto_register_ftp();
	proto_register_giop();
	proto_register_gre();
	proto_register_http();
	proto_register_icmp();
	proto_register_icmpv6();
	proto_register_icp();
	proto_register_igmp();
	proto_register_ip();
	proto_register_ipp();
	proto_register_ipsec();
	proto_register_ipv6();
	proto_register_ipx();
	proto_register_isakmp();
	proto_register_lapb();
	proto_register_llc();
	proto_register_mp();
	proto_register_nbipx();
	proto_register_nbt();
	proto_register_ncp();
	proto_register_netbios();
	proto_register_nntp();
	proto_register_null();
	proto_register_ospf();
	proto_register_pop();
	proto_register_ppp();
	proto_register_radius();
	proto_register_rip();
	proto_register_rsvp();
	proto_register_rtsp();
	proto_register_sdp();
	proto_register_smb();
#if defined(WITH_SNMP_CMU) || defined(WITH_SNMP_UCD)
	proto_register_snmp();
#endif
	proto_register_telnet();
	proto_register_tftp();
	proto_register_tcp();
	proto_register_tr();
	proto_register_trmac();
	proto_register_udp();
	proto_register_x25();

	/* Register one special-case FT_TEXT_ONLY field for use when
		converting ethereal to new-style proto_tree. These fields
		are merely strings on the GUI tree; they are not filterable */
	hf_text_only = proto_register_field (
		/* name */	"Text",
		/* abbrev */	"text",
		/* ftype */	FT_TEXT_ONLY,
		/* parent */	-1,
		/* vals[] */	NULL );
}

void
proto_cleanup(void)
{
	if (gmc_hfinfo)
		g_mem_chunk_destroy(gmc_hfinfo);
	if (gmc_field_info)
		g_mem_chunk_destroy(gmc_field_info);
	if (gmc_item_labels)
		g_mem_chunk_destroy(gmc_item_labels);
	if (gpa_hfinfo)
		g_ptr_array_free(gpa_hfinfo, FALSE);
}

/* frees the resources that the dissection a proto_tree uses */
void
proto_tree_free(proto_tree *tree)
{
	g_node_traverse((GNode*)tree, G_IN_ORDER, G_TRAVERSE_ALL, -1,
		proto_tree_free_node, NULL);
	g_node_destroy((GNode*)tree);
}

static gboolean
proto_tree_free_node(GNode *node, gpointer data)
{
	field_info *fi = (field_info*) (node->data);

	if (fi != NULL) {
		if (fi->representation)
			g_mem_chunk_free(gmc_item_labels, fi->representation);
		if (fi->hfinfo->type == FT_STRING)
			g_free(fi->value.string);
		else if (fi->hfinfo->type == FT_BYTES) 
			g_free(fi->value.bytes);
		g_mem_chunk_free(gmc_field_info, fi);
	}
	return FALSE; /* FALSE = do not end traversal of GNode tree */
}	

/* Finds a record in the hf_info_records array. */
static struct header_field_info*
find_hfinfo_record(int hfindex)
{
	g_assert(hfindex >= 0 && hfindex < gpa_hfinfo->len);
	return g_ptr_array_index(gpa_hfinfo, hfindex);
}

proto_item *
proto_tree_add_item(proto_tree *tree, int hfindex, gint start, gint length, ...)
{
	proto_item	*pi;
	va_list		ap;

	va_start(ap, length);
	pi = proto_tree_add_item_value(tree, hfindex, start, length, 0, 1, ap);
	va_end(ap);

	return pi;
}

proto_item *
proto_tree_add_item_hidden(proto_tree *tree, int hfindex, gint start, gint length, ...)
{
	proto_item	*pi;
	va_list		ap;

	va_start(ap, length);
	pi = proto_tree_add_item_value(tree, hfindex, start, length, 0, 0, ap);
	va_end(ap);

	return pi;
}

proto_item *
proto_tree_add_item_format(proto_tree *tree, int hfindex, gint start, gint length, ...)
{
	proto_item	*pi;
	va_list		ap;

	va_start(ap, length);
	pi = proto_tree_add_item_value(tree, hfindex, start, length, 1, 1, ap);
	va_end(ap);

	return pi;
}

proto_item *
proto_tree_add_text(proto_tree *tree, gint start, gint length, ...)
{
	proto_item	*pi;
	va_list		ap;

	va_start(ap, length);
	pi = proto_tree_add_item_value(tree, hf_text_only, start, length, 1, 1, ap);
	va_end(ap);

	return pi;
}

static proto_item *
proto_tree_add_item_value(proto_tree *tree, int hfindex, gint start,
	gint length, int include_format, int visible, va_list ap)
{
	proto_item	*pi;
	field_info	*fi;
	char		*junk, *format;

	if (!tree)
		return(NULL);

	/* either visibility flag can nullify the other */
	visible = proto_tree_is_visible && visible;

	fi = g_mem_chunk_alloc(gmc_field_info);

	fi->hfinfo = find_hfinfo_record(hfindex);
	g_assert(fi->hfinfo != NULL);
	fi->start = start;
	fi->length = length;
	fi->tree_type = ETT_NONE;
	fi->visible = visible;

/* from the stdarg man page on Solaris 2.6:
NOTES
     It is up to the calling routine to specify  in  some  manner
     how  many arguments there are, since it is not always possi-
     ble to determine the number  of  arguments  from  the  stack
     frame.   For example, execl is passed a zero pointer to sig-
     nal the end of the list.  printf can tell how many arguments
     there  are  by  the format.  It is non-portable to specify a
     second argument of char, short, or float to va_arg,  because
     arguments  seen  by the called function are not char, short,
     or float.  C converts char and short arguments  to  int  and
     converts  float arguments to double before passing them to a
     function.
*/
	switch(fi->hfinfo->type) {
		case FT_NONE:
			junk = va_arg(ap, guint8*);
			break;

		case FT_BOOLEAN:
			fi->value.numeric = va_arg(ap, unsigned int) ? TRUE : FALSE;
			break;

		case FT_BYTES:
			/* This g_malloc'ed memory is freed in
			   proto_tree_free_node() */
			fi->value.bytes = (guint8 *)g_malloc(length);
			memcpy(fi->value.bytes, va_arg(ap, guint8*), length);
			break;

		case FT_UINT8:
		case FT_VALS_UINT8:
			fi->value.numeric = va_arg(ap, unsigned int);
			break;

		case FT_UINT16:
		case FT_VALS_UINT16:
			fi->value.numeric = va_arg(ap, unsigned int);
			break;

		case FT_UINT32:
		case FT_VALS_UINT24:
		case FT_VALS_UINT32:
		case FT_IPv4:
		case FT_IPXNET:
			fi->value.numeric = va_arg(ap, unsigned int);
			break;

		case FT_DOUBLE:
			fi->value.floating = va_arg(ap, double);
			break;

		case FT_ETHER:
			memcpy(fi->value.ether, va_arg(ap, guint8*), 6);
			break;

		case FT_ABSOLUTE_TIME:
		case FT_RELATIVE_TIME:
			memcpy(&fi->value.time, va_arg(ap, struct timeval*),
				sizeof(struct timeval));
			break;

		case FT_STRING:
			/* This g_strdup'ed memory is freed in proto_tree_free_node() */
			fi->value.string = g_strdup(va_arg(ap, char*));
			break;

		case FT_TEXT_ONLY:
			; /* nothing */
			break;

		default:
			g_error("hfinfo->type %d not handled\n", fi->hfinfo->type);
			break;
	}

	pi = (proto_item*) g_node_new(fi);
	g_node_append((GNode*)tree, (GNode*)pi);

	/* are there any formatting arguments? */
	if (visible && include_format) {
		fi->representation = g_mem_chunk_alloc(gmc_item_labels);
		format = va_arg(ap, char*);
		vsnprintf(fi->representation, ITEM_LABEL_LENGTH,
				format, ap);
	}
	else {
		fi->representation = NULL;
	}

	return pi;
}

void
proto_item_set_len(proto_item *pi, gint length)
{
	field_info *fi = (field_info*) (((GNode*)pi)->data);
	fi->length = length;
}

proto_tree*
proto_tree_create_root(void)
{
	return (proto_tree*) g_node_new(NULL);
}

proto_tree*
proto_item_add_subtree(proto_item *pi,  gint idx) {
	field_info *fi = (field_info*) (((GNode*)pi)->data);
	fi->tree_type = idx;
	return (proto_tree*) pi;
}


int
proto_register_protocol(char *name, char *abbrev)
{
	return proto_register_field(name, abbrev, FT_NONE, -1, NULL);
}

/* for use with static arrays only, since we don't allocate our own copies
of the header_field_info struct contained withing the hf_register_info struct */
void
proto_register_field_array(int parent, hf_register_info *hf, int num_records)
{
	int			field_id, i;
	hf_register_info	*ptr = hf;

	for (i = 0; i < num_records; i++, ptr++) {
		field_id = proto_register_field_init(&ptr->hfinfo, parent);
		*ptr->p_id = field_id;
	}
}


/* Here we do allocate a new header_field_info struct */
int
proto_register_field(char *name, char *abbrev, enum ftenum type, int parent,
	struct value_string* vals)
{
	struct header_field_info *hfinfo;

	hfinfo = g_mem_chunk_alloc(gmc_hfinfo);
	hfinfo->name = name; /* should I g_strdup? */
	hfinfo->abbrev = abbrev; /* should I g_strdup? */
	hfinfo->type = type;
	hfinfo->vals = vals;
	hfinfo->parent = parent; /* this field differentiates protos and fields */

	return proto_register_field_init(hfinfo, parent);
}

static int
proto_register_field_init(header_field_info *hfinfo, int parent)
{
	g_assert((hfinfo->vals == NULL) || (hfinfo->type == FT_VALS_UINT8 || hfinfo->type == FT_VALS_UINT16 ||
		hfinfo->type == FT_VALS_UINT24 || hfinfo->type == FT_VALS_UINT32));

	hfinfo->parent = parent;

	/* if we always add and never delete, then id == len - 1 is correct */
	g_ptr_array_add(gpa_hfinfo, hfinfo);
	hfinfo->id = gpa_hfinfo->len - 1;
	return hfinfo->id;
}

void
proto_item_fill_label(field_info *fi, gchar *label_str)
{
	char *s;

	switch(fi->hfinfo->type) {
		case FT_NONE:
			snprintf(label_str, ITEM_LABEL_LENGTH,
				"%s", fi->hfinfo->name);
			break;

		case FT_BOOLEAN:
			snprintf(label_str, ITEM_LABEL_LENGTH,
				"%s: %s", fi->hfinfo->name,
				fi->value.numeric == TRUE ? "True" : "False");
			break;

		case FT_BYTES:
			snprintf(label_str, ITEM_LABEL_LENGTH,
				"%s: %s", fi->hfinfo->name, 
				 bytes_to_str(fi->value.bytes, fi->length));
			break;

		case FT_UINT8:
		case FT_UINT16:
		case FT_UINT32:
			snprintf(label_str, ITEM_LABEL_LENGTH,
				"%s: %u", fi->hfinfo->name,
				fi->value.numeric);
			break;

		case FT_DOUBLE:
			snprintf(label_str, ITEM_LABEL_LENGTH,
				"%s: %g", fi->hfinfo->name,
				fi->value.floating);
			break;

		case FT_ABSOLUTE_TIME:
			snprintf(label_str, ITEM_LABEL_LENGTH,
				"%s: %s", fi->hfinfo->name,
				abs_time_to_str(&fi->value.time));
			break;

		case FT_RELATIVE_TIME:
			snprintf(label_str, ITEM_LABEL_LENGTH,
				"%s: %s seconds", fi->hfinfo->name,
				rel_time_to_str(&fi->value.time));
			break;

		case FT_VALS_UINT8:
			s = match_strval(fi->value.numeric, cVALS(fi->hfinfo->vals));
			snprintf(label_str, ITEM_LABEL_LENGTH,
				"%s: %s (0x%02x)", fi->hfinfo->name,
				(s ? s : "Unknown"), fi->value.numeric);
			break;

		case FT_VALS_UINT16:
			s = match_strval(fi->value.numeric, cVALS(fi->hfinfo->vals));
			snprintf(label_str, ITEM_LABEL_LENGTH,
				"%s: %s (0x%04x)", fi->hfinfo->name,
				(s ? s : "Unknown"), fi->value.numeric);
			break;

		case FT_VALS_UINT24:
			s = match_strval(fi->value.numeric, cVALS(fi->hfinfo->vals));
			snprintf(label_str, ITEM_LABEL_LENGTH,
				"%s: %s (0x%06x)", fi->hfinfo->name,
				(s ? s : "Unknown"), fi->value.numeric);
			break;


		case FT_VALS_UINT32:
			s = match_strval(fi->value.numeric, cVALS(fi->hfinfo->vals));
			snprintf(label_str, ITEM_LABEL_LENGTH,
				"%s: %s (0x%08x)", fi->hfinfo->name,
				(s ? s : "Unknown"), fi->value.numeric);
			break;

		case FT_IPXNET:
			snprintf(label_str, ITEM_LABEL_LENGTH,
				"%s: 0x%08X", fi->hfinfo->name, fi->value.numeric);
			break;

		case FT_ETHER:
			snprintf(label_str, ITEM_LABEL_LENGTH,
				"%s: %s (%s)", fi->hfinfo->name,
				ether_to_str(fi->value.ether),
				get_ether_name(fi->value.ether));
			break;

		case FT_IPv4:
			snprintf(label_str, ITEM_LABEL_LENGTH,
				"%s: %s (%s)", fi->hfinfo->name,
				get_hostname(fi->value.numeric),
				ip_to_str((guint8*)&fi->value.numeric));
			break;
	
		case FT_STRING:
			snprintf(label_str, ITEM_LABEL_LENGTH,
				"%s: %s", fi->hfinfo->name, fi->value.string);
			break;

		default:
			g_error("hfinfo->type %d not handled\n", fi->hfinfo->type);
			break;
	}
}

int
proto_registrar_n(void)
{
	return gpa_hfinfo->len;
}

char*
proto_registrar_get_abbrev(int n)
{
	struct header_field_info *hfinfo;

	hfinfo = find_hfinfo_record(n);
	if (hfinfo)
		return hfinfo->abbrev;
	else
		return NULL;
}

int
proto_registrar_get_ftype(int n)
{
	struct header_field_info *hfinfo;

	hfinfo = find_hfinfo_record(n);
	if (hfinfo)
		return hfinfo->type;
	else
		return -1;
}

int
proto_registrar_get_parent(int n)
{
	struct header_field_info *hfinfo;

	hfinfo = find_hfinfo_record(n);
	if (hfinfo)
		return hfinfo->parent;
	else
		return -2;
}

gboolean
proto_registrar_is_protocol(int n)
{
	struct header_field_info *hfinfo;

	hfinfo = find_hfinfo_record(n);
	if (hfinfo)
		return (hfinfo->parent == -1 ? TRUE : FALSE);
	else
		return FALSE;
}

/* Looks for a protocol or a field in a proto_tree. Returns TRUE if
 * it exists anywhere, or FALSE if it exists nowhere. */
gboolean
proto_check_for_protocol_or_field(proto_tree* tree, int id)
{
	proto_tree_search_info	sinfo;

	sinfo.target = id;
	sinfo.result.node = NULL;
	sinfo.parent = -1;
	sinfo.traverse_func = NULL;

	/* do a quicker check if target is a protocol */
	if (proto_registrar_is_protocol(id) == TRUE) {
		proto_find_protocol_multi(tree, id, &check_for_protocol_or_field_id, &sinfo);
	}
	else {
		/* find the field's parent protocol */
		sinfo.parent = proto_registrar_get_parent(id);

		/* Go through each protocol subtree, checking if the protocols
		 * is the parent protocol of the field that we're looking for.
		 * We may have protocols that occur more than once (e.g., IP in IP),
		 * so we do indeed have to check all protocol subtrees, looking
		 * for the parent protocol. That's why proto_find_protocol()
		 * is not used --- it assumes a protocol occurs only once. */
		g_node_traverse((GNode*)tree, G_IN_ORDER, G_TRAVERSE_ALL, 2,
						check_for_field_within_protocol, &sinfo);
	}

	if (sinfo.result.node)
		return TRUE;
	else
		return FALSE;
}

static gboolean
check_for_protocol_or_field_id(GNode *node, gpointer data)
{
	field_info		*fi = (field_info*) (node->data);
	proto_tree_search_info	*sinfo = (proto_tree_search_info*) data;

	if (fi) { /* !fi == the top most container node which holds nothing */
		if (fi->hfinfo->id == sinfo->target) {
			sinfo->result.node = node;
			return TRUE; /* halt traversal */
		}
	}
	return FALSE; /* keep traversing */
}

static gboolean
check_for_field_within_protocol(GNode *node, gpointer data)
{
	field_info		*fi = (field_info*) (node->data);
	proto_tree_search_info	*sinfo = (proto_tree_search_info*) data;

	if (fi) { /* !fi == the top most container node which holds nothing */
		if (fi->hfinfo->id == sinfo->parent) {
			g_node_traverse(node, G_IN_ORDER, G_TRAVERSE_ALL, -1,
					check_for_protocol_or_field_id, sinfo);
			if (sinfo->result.node)
				return TRUE; /* halt traversal */
		}
	}
	return FALSE; /* keep traversing */
}

/* Looks for a protocol at the top layer of the tree. The protocol can occur
 * more than once, for those encapsulated protocols. For each protocol subtree
 * that is found, the callback function is called.
 */
void
proto_find_protocol_multi(proto_tree* tree, int target, GNodeTraverseFunc callback,
			proto_tree_search_info *sinfo)
{
	g_assert(callback != NULL);
	g_node_traverse((GNode*)tree, G_IN_ORDER, G_TRAVERSE_ALL, 2, callback, (gpointer*)sinfo);
}

/* Simple wrappter to traverse all nodes, calling the sinfo traverse function with sinfo as an arg */
gboolean
proto_get_field_values(proto_tree* subtree, proto_tree_search_info *sinfo)
{
	/* Don't try to check value of top-level NULL GNode */
	if (!((GNode*)subtree)->data) {
		return FALSE; /* don't halt */
	}
	g_node_traverse((GNode*)subtree, G_IN_ORDER, G_TRAVERSE_ALL, -1, sinfo->traverse_func, (gpointer*)sinfo);
	return FALSE; /* don't halt */
}

/* Dumps the contents of the registration database to stdout. An indepedent program can take
 * this output and format it into nice tables or HTML or whatever.
 *
 * There is one record per line. Each record is either a protocol or a header
 * field, differentiated by the first field. The fields are tab-delimited.
 *
 * Protocols
 * ---------
 * Field 1 = 'P'
 * Field 2 = protocol name
 * Field 3 = protocol abbreviation
 *
 * Header Fields
 * -------------
 * Field 1 = 'F'
 * Field 2 = field name
 * Field 3 = field abbreviation
 * Field 4 = type ( textual representation of the the ftenum type )
 * Field 5 = parent protocol abbreviation
 */
void
proto_registrar_dump(void)
{
	header_field_info	*hfinfo, *parent_hfinfo;
	int			i, len;
	const char 		*enum_name;

	len = gpa_hfinfo->len;
	for (i = 0; i < len ; i++) {
		hfinfo = find_hfinfo_record(i);

		/* format for protocols */
		if (proto_registrar_is_protocol(i)) {
			printf("P\t%s\t%s\n", hfinfo->name, hfinfo->abbrev);
		}
		/* format for header fields */
		else {
			parent_hfinfo = find_hfinfo_record(hfinfo->parent);
			g_assert(parent_hfinfo);

			switch(hfinfo->type) {
			case FT_NONE:
				enum_name = "FT_NONE";
				break;
			case FT_BOOLEAN:
				enum_name = "FT_BOOLEAN";
				break;
			case FT_UINT8:
				enum_name = "FT_UINT8";
				break;
			case FT_UINT16:
				enum_name = "FT_UINT16";
				break;
			case FT_UINT32:
				enum_name = "FT_UINT32";
				break;
			case FT_DOUBLE:
				enum_name = "FT_DOUBLE";
				break;
			case FT_ABSOLUTE_TIME:
				enum_name = "FT_ABSOLUTE_TIME";
				break;
			case FT_RELATIVE_TIME:
				enum_name = "FT_RELATIVE_TIME";
				break;
			case FT_STRING:
				enum_name = "FT_STRING";
				break;
			case FT_ETHER:
				enum_name = "FT_ETHER";
				break;
			case FT_BYTES:
				enum_name = "FT_BYTES";
				break;
			case FT_IPv4:
				enum_name = "FT_IPv4";
				break;
			case FT_IPv6:
				enum_name = "FT_IPv6";
				break;
			case FT_IPXNET:
				enum_name = "FT_IPXNET";
				break;
			case FT_VALS_UINT8:
				enum_name = "FT_VALS_UINT8";
				break;
			case FT_VALS_UINT16:
				enum_name = "FT_VALS_UINT16";
				break;
			case FT_VALS_UINT24:
				enum_name = "FT_VALS_UINT24";
				break;
			case FT_VALS_UINT32:
				enum_name = "FT_VALS_UINT32";
				break;
			case FT_TEXT_ONLY:
				enum_name = "FT_TEXT_ONLY";
				break;
			default:
				enum_name = "UNKNOWN";
				break;
			}
			printf("F\t%s\t%s\t%s\t%s\n", hfinfo->name, hfinfo->abbrev,
				enum_name,parent_hfinfo->abbrev);
		}
	}
}
