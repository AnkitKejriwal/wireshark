/* proto.h
 * Definitions for protocol display
 *
 * $Id: proto.h,v 1.58 2004/04/30 06:56:15 ulfl Exp $
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

#ifndef __PROTO_H__
#define __PROTO_H__

#ifdef HAVE_STDARG_H
# include <stdarg.h>
#else
# include <varargs.h>
#endif

#include <glib.h>

#include "ipv4.h"
#include "nstime.h"
#include "tvbuff.h"
#include "ftypes/ftypes.h"

/* The header-field index for the special text pseudo-field */
extern int hf_text_only;

struct _value_string;

#define ITEM_LABEL_LENGTH	240

/* In order to make a const value_string[] look like a value_string*, I
 * need this macro */
#define VALS(x)	(const struct _value_string*)(x)

/* ... and similarly, */
#define TFS(x)	(const struct true_false_string*)(x)

struct _protocol;

typedef struct _protocol protocol_t;
 
/* check protocol activation */
#define CHECK_DISPLAY_AS_X(x_handle,index, tvb, pinfo, tree) {	\
	if (!proto_is_protocol_enabled(find_protocol_by_id(index))) {	\
		call_dissector(x_handle,tvb, pinfo, tree);		\
		return;							\
	}								\
  }

enum {
	BASE_NONE,
	BASE_DEC,
	BASE_HEX,
	BASE_OCT
};

typedef struct _header_field_info header_field_info;

/* information describing a header field */
struct _header_field_info {
	/* ---------- set by dissector --------- */
	char				*name;
	char				*abbrev;
	enum ftenum			type;
	int				display;	/* for integers only, so far. Base */
	const void			*strings;	/* val_string or true_false_string */
	guint32				bitmask;
	char				*blurb;		/* Brief description of field. */

	/* ---------- set by proto routines --------- */
	int				id;		/* Field ID */
	int				parent;		/* parent protocol */
	int				bitshift;	/* bits to shift */
	header_field_info		*same_name_next; /* Link to next hfinfo with same abbrev*/
	header_field_info		*same_name_prev; /* Link to previous hfinfo with same abbrev*/
};

/*
 * HFILL initializes all the "set by proto routines" fields in a
 * "header_field_info"; if new fields are added or removed, it should
 * be changed as necessary.
 */
#define HFILL 0, 0, 0, NULL, NULL

/* Used when registering many fields at once */
typedef struct hf_register_info {
	int			*p_id;	/* pointer to int; written to by register() function */
	header_field_info	hfinfo;
} hf_register_info;


typedef struct _item_label_t {
	char representation[ITEM_LABEL_LENGTH];
} item_label_t;

/* Contains the field information for the proto_item. */
typedef struct field_info {
	header_field_info		*hfinfo;
	gint				start;
	gint				length;
	gint				tree_type; /* ETT_* */
	item_label_t			*rep; /* string for GUI tree */
	int				visible;
	tvbuff_t			*ds_tvb;  /* data source tvbuff */
	fvalue_t			value;
} field_info;

/* One of these exists for the entire protocol tree. Each proto_node
 * in the protocol tree points to the same copy. */
typedef struct {
    GHashTable  *interesting_hfids;
    gboolean    visible;
} tree_data_t;

/* Each proto_tree, proto_item is one of these. */
typedef struct _proto_node {
	struct _proto_node *first_child;
	struct _proto_node *last_child;
	struct _proto_node *next;
	struct _proto_node *parent;
	field_info  *finfo;
	tree_data_t *tree_data;
} proto_node;

typedef proto_node proto_tree;
typedef proto_node proto_item;

typedef void (*proto_tree_foreach_func)(proto_node *, gpointer);

extern void proto_tree_children_foreach(proto_tree *tree,
    proto_tree_foreach_func func, gpointer data);

/* Retrieve the field_info from a proto_item */
#define PITEM_FINFO(t)  ((t)->finfo)

/* Retrieve the tree_data_t from a proto_tree */
#define PTREE_DATA(t)   ((t)->tree_data)

/* Sets up memory used by proto routines. Called at program startup */
extern void proto_init(const char *plugin_dir,
    void (register_all_protocols)(void), void (register_all_handoffs)(void));

/* Frees memory used by proto routines. Called at program shutdown */
extern void proto_cleanup(void);

/* Set text of proto_item after having already been created. */
#if __GNUC__ >= 2
extern void proto_item_set_text(proto_item *ti, const char *format, ...)
	__attribute__((format (printf, 2, 3)));
#else
extern void proto_item_set_text(proto_item *ti, const char *format, ...);
#endif

/* Append to text of proto_item after having already been created. */
#if __GNUC__ >= 2
extern void proto_item_append_text(proto_item *ti, const char *format, ...)
	__attribute__((format (printf, 2, 3)));
#else
extern void proto_item_append_text(proto_item *ti, const char *format, ...);
#endif

/* Set length of proto_item after having already been created. */
extern void proto_item_set_len(proto_item *ti, gint length);

/*
 * Sets the length of the item based on its start and on the specified
 * offset, which is the offset past the end of the item; as the start
 * in the item is relative to the beginning of the data source tvbuff,
 * we need to pass in a tvbuff - the end offset is relative to the beginning
 * of that tvbuff.
 */
extern void proto_item_set_end(proto_item *pi, tvbuff_t *tvb, gint end);

/* Get length of proto_item. Useful after using proto_tree_add_item()
 * to add a variable-length field (e.g., FT_NSTRING_UINT8) */
extern int proto_item_get_len(proto_item *ti);

/* Creates new proto_tree root */
extern proto_tree* proto_tree_create_root(void);

/* Mark a field/protocol ID as "interesting". */
extern void
proto_tree_prime_hfid(proto_tree *tree, int hfid);

/* Clear memory for entry proto_tree. Clears proto_tree struct also. */
extern void proto_tree_free(proto_tree *tree);

/* Create a subtree under an existing item; returns tree pointer */
extern proto_tree* proto_item_add_subtree(proto_item *ti, gint idx);

/* Get a subtree under an item; returns tree pointer */
extern proto_tree* proto_item_get_subtree(proto_item *ti);

/* Get a parent item; returns item pointer */
extern proto_item* proto_item_get_parent(proto_item *ti);

/* Get Nth generation parent item; returns item pointer */
extern proto_item* proto_item_get_parent_nth(proto_item *ti, int gen);

/* Get a parent item of subtree; returns item pointer */
extern proto_item* proto_tree_get_parent(proto_tree *tree);

extern int
proto_register_protocol(char *name, char *short_name, char *filter_name);

extern void
proto_register_field_array(int parent, hf_register_info *hf, int num_records);

extern void
proto_register_subtree_array(gint **indices, int num_indices);

/* Add an item to a proto_tree, using the text label registered to that item;
   the item is extracted from the tvbuff handed to it. */
extern proto_item *
proto_tree_add_item(proto_tree *tree, int hfindex, tvbuff_t *tvb,
    gint start, gint length, gboolean little_endian);

extern proto_item *
proto_tree_add_item_hidden(proto_tree *tree, int hfindex, tvbuff_t *tvb,
    gint start, gint length, gboolean little_endian);

/* Add a FT_NONE to a proto_tree */
#if __GNUC__ >= 2
extern proto_item *
proto_tree_add_none_format(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, const char *format, ...)
	__attribute__((format (printf, 6, 7)));
#else
extern proto_item *
proto_tree_add_none_format(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, const char *format, ...);
#endif

/* Add a FT_PROTOCOL to a proto_tree */
#if __GNUC__ >= 2
extern proto_item *
proto_tree_add_protocol_format(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, const char *format, ...)
	__attribute__((format (printf, 6, 7)));
#else
extern proto_item *
proto_tree_add_protocol_format(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, const char *format, ...);
#endif

/* Add a FT_BYTES to a proto_tree */
extern proto_item *
proto_tree_add_bytes(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, const guint8* start_ptr);

extern proto_item *
proto_tree_add_bytes_hidden(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, const guint8* start_ptr);

#if __GNUC__ >= 2
extern proto_item *
proto_tree_add_bytes_format(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, const guint8* start_ptr, const char *format, ...)
	__attribute__((format (printf, 7, 8)));
#else
extern proto_item *
proto_tree_add_bytes_format(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, const guint8* start_ptr, const char *format, ...);
#endif

/* Add a FT_*TIME to a proto_tree */
extern proto_item *
proto_tree_add_time(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, nstime_t* value_ptr);

extern proto_item *
proto_tree_add_time_hidden(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, nstime_t* value_ptr);

#if __GNUC__ >= 2
extern proto_item *
proto_tree_add_time_format(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, nstime_t* value_ptr, const char *format, ...)
	__attribute__((format (printf, 7, 8)));
#else
extern proto_item *
proto_tree_add_time_format(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, nstime_t* value_ptr, const char *format, ...);
#endif

/* Add a FT_IPXNET to a proto_tree */
extern proto_item *
proto_tree_add_ipxnet(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, guint32 value);

extern proto_item *
proto_tree_add_ipxnet_hidden(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, guint32 value);

#if __GNUC__ >= 2
extern proto_item *
proto_tree_add_ipxnet_format(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, guint32 value, const char *format, ...)
	__attribute__((format (printf, 7, 8)));
#else
extern proto_item *
proto_tree_add_ipxnet_format(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, guint32 value, const char *format, ...);
#endif

/* Add a FT_IPv4 to a proto_tree */
extern proto_item *
proto_tree_add_ipv4(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, guint32 value);

extern proto_item *
proto_tree_add_ipv4_hidden(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, guint32 value);

#if __GNUC__ >= 2
extern proto_item *
proto_tree_add_ipv4_format(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, guint32 value, const char *format, ...)
	__attribute__((format (printf, 7, 8)));
#else
extern proto_item *
proto_tree_add_ipv4_format(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, guint32 value, const char *format, ...);
#endif

/* Add a FT_IPv6 to a proto_tree */
extern proto_item *
proto_tree_add_ipv6(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, const guint8* value_ptr);

extern proto_item *
proto_tree_add_ipv6_hidden(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, const guint8* value_ptr);

#if __GNUC__ >= 2
extern proto_item *
proto_tree_add_ipv6_format(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, const guint8* value_ptr, const char *format, ...)
	__attribute__((format (printf, 7, 8)));
#else
extern proto_item *
proto_tree_add_ipv6_format(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, const guint8* value_ptr, const char *format, ...);
#endif

/* Add a FT_ETHER to a proto_tree */
extern proto_item *
proto_tree_add_ether(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, const guint8* value);

extern proto_item *
proto_tree_add_ether_hidden(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, const guint8* value);

#if __GNUC__ >= 2
extern proto_item *
proto_tree_add_ether_format(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, const guint8* value, const char *format, ...)
	__attribute__((format (printf, 7, 8)));
#else
extern proto_item *
proto_tree_add_ether_format(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, const guint8* value, const char *format, ...);
#endif

/* Add a FT_STRING to a proto_tree */
extern proto_item *
proto_tree_add_string(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, const char* value);

extern proto_item *
proto_tree_add_string_hidden(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, const char* value);

#if __GNUC__ >= 2
extern proto_item *
proto_tree_add_string_format(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, const char* value, const char *format, ...)
	__attribute__((format (printf, 7, 8)));
#else
extern proto_item *
proto_tree_add_string_format(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, const char* value, const char *format, ...);
#endif

extern void
proto_item_append_string(proto_item *pi, const char *str);

/* Add a FT_BOOLEAN to a proto_tree */
extern proto_item *
proto_tree_add_boolean(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, guint32 value);

extern proto_item *
proto_tree_add_boolean_hidden(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, guint32 value);

#if __GNUC__ >= 2
extern proto_item *
proto_tree_add_boolean_format(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, guint32 value, const char *format, ...)
	__attribute__((format (printf, 7, 8)));
#else
extern proto_item *
proto_tree_add_boolean_format(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, guint32 value, const char *format, ...);
#endif

/* Add a FT_FLOAT to a proto_tree */
extern proto_item *
proto_tree_add_float(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, float value);

extern proto_item *
proto_tree_add_float_hidden(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, float value);

#if __GNUC__ >= 2
extern proto_item *
proto_tree_add_float_format(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, float value, const char *format, ...)
	__attribute__((format (printf, 7, 8)));
#else
extern proto_item *
proto_tree_add_float_format(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, float value, const char *format, ...);
#endif

/* Add a FT_DOUBLE to a proto_tree */
extern proto_item *
proto_tree_add_double(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, double value);

extern proto_item *
proto_tree_add_double_hidden(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, double value);

#if __GNUC__ >= 2
extern proto_item *
proto_tree_add_double_format(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, double value, const char *format, ...)
	__attribute__((format (printf, 7, 8)));
#else
extern proto_item *
proto_tree_add_double_format(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, double value, const char *format, ...);
#endif

/* Add any FT_UINT* to a proto_tree */
extern proto_item *
proto_tree_add_uint(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, guint32 value);

extern proto_item *
proto_tree_add_uint_hidden(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, guint32 value);

#if __GNUC__ >= 2
extern proto_item *
proto_tree_add_uint_format(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, guint32 value, const char *format, ...)
	__attribute__((format (printf, 7, 8)));
#else
extern proto_item *
proto_tree_add_uint_format(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, guint32 value, const char *format, ...);
#endif

/* Add any FT_INT* to a proto_tree */
extern proto_item *
proto_tree_add_int(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, gint32 value);

extern proto_item *
proto_tree_add_int_hidden(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, gint32 value);

#if __GNUC__ >= 2
extern proto_item *
proto_tree_add_int_format(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, gint32 value, const char *format, ...)
	__attribute__((format (printf, 7, 8)));
#else
extern proto_item *
proto_tree_add_int_format(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, gint32 value, const char *format, ...);
#endif


/* Add a text-only node to the proto_tree */
#if __GNUC__ >= 2
extern proto_item *
proto_tree_add_text(proto_tree *tree, tvbuff_t *tvb, gint start, gint length, const char *,
	...) __attribute__((format (printf, 5, 6)));
#else
extern proto_item *
proto_tree_add_text(proto_tree *tree, tvbuff_t *tvb, gint start, gint length, const char *,
	...);
#endif

extern proto_item *
proto_tree_add_text_valist(proto_tree *tree, tvbuff_t *tvb, gint start,
	gint length, const char *format, va_list ap);


/* Useful for quick debugging. Also sends string to STDOUT, so don't
 * leave call to this function in production code. */
#if __GNUC__ >= 2
extern proto_item *
proto_tree_add_debug_text(proto_tree *tree, const char *format, ...)
	__attribute__((format (printf, 2, 3)));
#else
extern proto_item *
proto_tree_add_debug_text(proto_tree *tree, const char *format, ...);
#endif

extern void
proto_item_fill_label(field_info *fi, gchar *label_str);

extern void
proto_tree_set_visible(proto_tree *tree, gboolean visible);

/* Returns number of items (protocols or header fields) registered. */
extern int proto_registrar_n(void);

/* Returns char* to name for item # n (0-indexed) */
extern char* proto_registrar_get_name(int n);

/* Returns char* to abbrev for item # n (0-indexed) */
extern char* proto_registrar_get_abbrev(int n);

/* get the header field information based upon a field or protocol id */
extern header_field_info* proto_registrar_get_nth(guint hfindex);

/* get the header field information based upon a field name */
extern header_field_info* proto_registrar_get_byname(char *field_name);

/* Returns enum ftenum for item # n */
extern int proto_registrar_get_ftype(int n);

/* Returns parent protocol for item # n.
 * Returns -1 if item _is_ a protocol */
extern int proto_registrar_get_parent(int n);

/* Is item #n a protocol? */
extern gboolean proto_registrar_is_protocol(int n);

/* Is protocol's decoding enabled ? */
extern gboolean proto_is_protocol_enabled(protocol_t *protocol);

/* Can item #n decoding be disabled? */
extern gboolean proto_can_toggle_protocol(int proto_id);

/* Routines to use to iterate over the protocols and their fields;
 * they return the item number of the protocol in question or the
 * appropriate hfinfo pointer, and keep state in "*cookie". */
extern int proto_get_first_protocol(void **cookie);
extern int proto_get_next_protocol(void **cookie);
extern header_field_info *proto_get_first_protocol_field(int proto_id, void **cookle);
extern header_field_info *proto_get_next_protocol_field(void **cookle);

/* Given a protocol's "protocol_t", return its proto_id */
extern int proto_get_id(protocol_t *protocol);

/* Given a protocol's filter_name, return its proto_id */
extern int proto_get_id_by_filter_name(gchar* filter_name);

/* Given a protocol's item number, find the "protocol_t" structure for it */
extern protocol_t *find_protocol_by_id(int proto_id);

/* Given a protocol's item number, return its name. */
extern char *proto_get_protocol_name(int n);

/* Given a protocol's "protocol_t", return its short name. */
extern char *proto_get_protocol_short_name(protocol_t *protocol);

/* Given a protocol's item number, return its filter name. */
extern char *proto_get_protocol_filter_name(int proto_id);

/* Enable / Disable protocol */
extern void proto_set_decoding(int proto_id, gboolean enabled);

/* Disable disabling/enabling of protocol */
extern void proto_set_cant_toggle(int proto_id);

/* Get length of registered field according to field type.
 * 0 means undeterminable at registration time.
 * -1 means unknown field */
extern gint proto_registrar_get_length(int n);

/* Checks for existence any protocol or field within a tree.
 * "Protocols" are assumed to be a child of the [empty] root node.
 * TRUE = found, FALSE = not found */
extern gboolean proto_check_for_protocol_or_field(proto_tree* tree, int id);

/* Return GPtrArray* of field_info pointers for all hfindex that appear in
 * tree. Only works with primed trees, and is fast. */
extern GPtrArray* proto_get_finfo_ptr_array(proto_tree *tree, int hfindex);

/* Return GPtrArray* of field_info pointers for all hfindex that appear in
 * tree. Works with any tree, primed or unprimed, and is slower than
 * proto_get_finfo_ptr_array because it has to search through the tree. */
extern GPtrArray* proto_find_finfo(proto_tree *tree, int hfindex);

/* Dumps a glossary of the protocol registrations to STDOUT */
extern void proto_registrar_dump_protocols(void);

/* Dumps a glossary of the protocol and field registrations to STDOUT */
extern void proto_registrar_dump_fields(void);

/* Points to the first element of an array of Booleans, indexed by
   a subtree item type; that array element is TRUE if subtrees of
   an item of that type are to be expanded. */
extern gboolean	     *tree_is_expanded;

/* Number of elements in that array. */
extern int           num_tree_types;

/* glib doesn't have g_ptr_array_len of all things!*/
#ifndef g_ptr_array_len
#define g_ptr_array_len(a)      ((a)->len)
#endif

extern int
hfinfo_bitwidth(header_field_info *hfinfo);

#include "epan.h"

/*
 * Returns TRUE if we can do a "match selected" on the field, FALSE
 * otherwise.
 */
extern gboolean
proto_can_match_selected(field_info *finfo, epan_dissect_t *edt);

extern char*
proto_construct_dfilter_string(field_info *finfo, epan_dissect_t *edt);

extern field_info*
proto_find_field_from_offset(proto_tree *tree, guint offset, tvbuff_t *tvb);

#endif /* proto.h */
