/* proto.h
 * Definitions for protocol display
 *
 * $Id: proto.h,v 1.67 2004/05/14 15:55:37 ulfl Exp $
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


/*! @file proto.h
    The protocol tree related functions.<BR>
    A protocol tree will hold all necessary data to display the whole dissected packet.
    Creating a protocol tree is done in a two stage process:
    A static part at program startup, and a dynamic part when the dissection with the real packet data is done.<BR>
    The "static" information is provided by creating a hf_register_info hf[] array, and register it using the 
    proto_register_field_array() function. This is usually done at dissector registering.<BR>
    The "dynamic" information is added to the protocol tree by calling one of the proto_tree_add_...() functions, 
    e.g. proto_tree_add_bytes().
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

/** The header-field index for the special text pseudo-field */
extern int hf_text_only;

/** the maximum length of a protocol field string representation */
#define ITEM_LABEL_LENGTH	240

struct _value_string;

/** Make a const value_string[] look like a _value_string pointer, used to set header_field_info.strings */
#define VALS(x)	(const struct _value_string*)(x)

/** Make a const true_false_string[] look like a _true_false_string pointer, used to set header_field_info.strings */
#define TFS(x)	(const struct true_false_string*)(x)

struct _protocol;

/** Structure for information about a protocol */
typedef struct _protocol protocol_t;
 
/** check protocol activation
 * @todo this macro looks like a hack */
#define CHECK_DISPLAY_AS_X(x_handle,index, tvb, pinfo, tree) {	\
	if (!proto_is_protocol_enabled(find_protocol_by_id(index))) {	\
		call_dissector(x_handle,tvb, pinfo, tree);		\
		return;							\
	}								\
  }


/** GNUC has the ability to check format strings that follow the syntax used in printf and others.
 Hide the differences between different compilers in this GNUC_FORMAT_CHECK macro.
 @param archetype one of: printf, scanf, strftime or strfmon
 @param string_index specifies which argument is the format string argument (starting from 1)
 @param first_to_check is the number of the first argument to check against the format string
 @todo as this check is also done at some other places too, move this macro to a more central place? */
#if __GNUC__ >= 2
	#define GNUC_FORMAT_CHECK(archetype, string_index, first_to_check) __attribute__((format (archetype, string_index, first_to_check)))
#else
	#define GNUC_FORMAT_CHECK(archetype, string_index, first_to_check)
#endif


/** radix for decimal values, used in header_field_info.display */
typedef enum {
	BASE_NONE,	/**< none */
	BASE_DEC,	/**< decimal */
	BASE_HEX,	/**< hexadecimal */
	BASE_OCT	/**< octal */
} base_display_e;

/** information describing a header field */
typedef struct _header_field_info header_field_info;

/** information describing a header field */
struct _header_field_info {
	/* ---------- set by dissector --------- */
	char				*name;      /**< full name of this field */
	char				*abbrev;    /**< abbreviated name of this field */
	enum ftenum			type;       /**< field type, one of FT_ (from ftypes.h) */
	int					display;	/**< one of BASE_, or number of field bits for FT_BOOLEAN */
	const void			*strings;	/**< _value_string (or true_false_string for FT_BOOLEAN), typically converted by VALS() or TFS() */
	guint32				bitmask;    /**< FT_BOOLEAN only: bitmask of interesting bits */
	char				*blurb;		/**< Brief description of field. */

	/* ------- set by proto routines (prefilled by HFILL macro, see below) ------ */
	int				id;		/**< Field ID */
	int				parent;		/**< parent protocol tree */
	int				bitshift;	/**< bits to shift (FT_BOOLEAN only) */
	header_field_info		*same_name_next; /**< Link to next hfinfo with same abbrev*/
	header_field_info		*same_name_prev; /**< Link to previous hfinfo with same abbrev*/
};

/**
 * HFILL initializes all the "set by proto routines" fields in a
 * _header_field_info. If new fields are added or removed, it should
 * be changed as necessary.
 */
#define HFILL 0, 0, 0, NULL, NULL

/** Used when registering many fields at once, using proto_register_field_array() */
typedef struct hf_register_info {
	int			*p_id;	/**< written to by register() function */
	header_field_info	hfinfo; /**< the field info to be registered */
} hf_register_info;




/** string representation, if one of the proto_tree_add_..._format() functions used */
typedef struct _item_label_t {
	char representation[ITEM_LABEL_LENGTH];
} item_label_t;


/** Contains the field information for the proto_item. */
typedef struct field_info {
	header_field_info	*hfinfo;    /**< pointer to registered field information */
	gint				start;      /**< current start of data in field_info.ds_tvb */
	gint				length;     /**< current data length of item in field_info.ds_tvb */
	gint				tree_type;  /**< ETT_ */
	item_label_t		*rep;       /**< string for GUI tree */
	int					flags;      /**< one of FI_HIDDEN, ... */
	tvbuff_t			*ds_tvb;    /**< data source tvbuff */
	fvalue_t			value;
} field_info;


/** The protocol field should not be shown in the tree (it's used for filtering only), 
 * used in field_info.flags. */
#define FI_HIDDEN       0x0001
/** The protocol field should be displayed as "generated by Ethereal",
 * used in field_info.flags. */
#define FI_GENERATED    0x0002
/** The protocol field should be displayed as a link to another packet,
 * used in field_info.flags. */
#define FI_LINK         0x0004

/** convenience macro to get field_info.flags */
#define FI_GET_FLAG(fi, flag) (fi->flags & flag)
/** convenience macro to set field_info.flags */
#define FI_SET_FLAG(fi, flag) (fi->flags = fi->flags | flag)



/** One of these exists for the entire protocol tree. Each proto_node
 * in the protocol tree points to the same copy. */
typedef struct {
    GHashTable  *interesting_hfids;
    gboolean    visible;
} tree_data_t;

/** Each proto_tree, proto_item is one of these. */
typedef struct _proto_node {
	struct _proto_node *first_child;
	struct _proto_node *last_child;
	struct _proto_node *next;
	struct _proto_node *parent;
	field_info  *finfo;
	tree_data_t *tree_data;
} proto_node;

/** A protocol tree element. */
typedef proto_node proto_tree;
/** A protocol item element. */
typedef proto_node proto_item;


/** is this protocol field hidden from the protocol tree display (used for filtering only)? */
#define PROTO_ITEM_IS_HIDDEN(proto_item)        \
	((proto_item) ? FI_GET_FLAG(proto_item->finfo, FI_HIDDEN) : 0)
/** mark this protocol field to be hidden from the protocol tree display (used for filtering only) */
#define PROTO_ITEM_SET_HIDDEN(proto_item)       \
	((proto_item) ? FI_SET_FLAG(proto_item->finfo, FI_HIDDEN) : 0)
/** is this protocol field generated by Ethereal (and not read from the packet data)? */
#define PROTO_ITEM_IS_GENERATED(proto_item)	\
	((proto_item) ? FI_GET_FLAG(proto_item->finfo, FI_GENERATED) : 0)
/** mark this protocol field as generated by Ethereal (and not read from the packet data) */
#define PROTO_ITEM_SET_GENERATED(proto_item)	\
	((proto_item) ? FI_SET_FLAG(proto_item->finfo, FI_GENERATED) : 0)
/** is this protocol field a link to another packet? */
#define PROTO_ITEM_IS_LINK(proto_item)	\
	((proto_item) ? FI_GET_FLAG(proto_item->finfo, FI_LINK) : 0)
/** mark this protocol field as a link to another packet */
#define PROTO_ITEM_SET_LINK(proto_item)	\
	((proto_item) ? FI_SET_FLAG(proto_item->finfo, FI_LINK) : 0)


typedef void (*proto_tree_foreach_func)(proto_node *, gpointer);

extern void proto_tree_children_foreach(proto_tree *tree,
    proto_tree_foreach_func func, gpointer data);

/** Retrieve the field_info from a proto_item */
#define PITEM_FINFO(proto_item)  ((proto_item)->finfo)

/** Retrieve the tree_data_t from a proto_tree */
#define PTREE_DATA(proto_tree)   ((proto_tree)->tree_data)



/** Sets up memory used by proto routines. Called at program startup */
extern void proto_init(const char *plugin_dir,
    void (register_all_protocols)(void), void (register_all_handoffs)(void));

/** Frees memory used by proto routines. Called at program shutdown */
extern void proto_cleanup(void);



/** Create a subtree under an existing item.
 @param ti the parent item of the new subtree
 @param idx one of the ett_ array elements registered with proto_register_subtree_array()
 @return the new subtree */
extern proto_tree* proto_item_add_subtree(proto_item *ti, gint idx);

/** Get an existing subtree under an item.
 @param ti the parent item of the subtree
 @return the subtree or NULL */
extern proto_tree* proto_item_get_subtree(proto_item *ti);

/** Get the parent of a subtree item.
 @param ti the child item in the subtree
 @return parent item or NULL */
extern proto_item* proto_item_get_parent(proto_item *ti);

/** Get Nth generation parent item.
 @param ti the child item in the subtree
 @param gen the generation to get (using 1 here is the same as using proto_item_get_parent())
 @return parent item */
extern proto_item* proto_item_get_parent_nth(proto_item *ti, int gen);

/** Replace text of item after it already has been created.
 @param ti the item to set the text
 @param format printf like format string
 @param ... printf like parameters */
extern void proto_item_set_text(proto_item *ti, const char *format, ...)
	GNUC_FORMAT_CHECK(printf, 2,3);

/** Append to text of item after it has already been created.
 @param ti the item to append the text to
 @param format printf like format string
 @param ... printf like parameters */
extern void proto_item_append_text(proto_item *ti, const char *format, ...)
	GNUC_FORMAT_CHECK(printf, 2,3);

/** Set proto_item's length inside tvb, after it has already been created.
 @param ti the item to set the length
 @param length the new length ot the item */
extern void proto_item_set_len(proto_item *ti, gint length);

/**
 * Sets the length of the item based on its start and on the specified
 * offset, which is the offset past the end of the item; as the start
 * in the item is relative to the beginning of the data source tvbuff,
 * we need to pass in a tvbuff.
 @param ti the item to set the length
 @param tvb end is relative to this tvbuff 
 @param end this end offset is relative to the beginning of tvb
 @todo make usage clearer, I don't understand it!
 */
extern void proto_item_set_end(proto_item *ti, tvbuff_t *tvb, gint end);

/** Get length of a proto_item. Useful after using proto_tree_add_item()
 * to add a variable-length field (e.g., FT_NSTRING_UINT8).
 @param ti the item to get the length from
 @return the current length */
extern int proto_item_get_len(proto_item *ti);



/** Creates a new proto_tree root.
 @return the new tree root */
extern proto_tree* proto_tree_create_root(void);

/** Clear memory for entry proto_tree. Clears proto_tree struct also.
 @param tree the tree to free */
extern void proto_tree_free(proto_tree *tree);

/** Set the tree visible or invisible.
 Is the parsing being done for a visible proto_tree or an invisible one?
 By setting this correctly, the proto_tree creation is sped up by not
 having to call vsnprintf and copy strings around.
 @param tree the tree to be set
 @param visible ... or not  */
extern void
proto_tree_set_visible(proto_tree *tree, gboolean visible);

/** Mark a field/protocol ID as "interesting".
 @param tree the tree to be set
 @param hfid the interesting field id
 @todo what *does* interesting mean? */
extern void
proto_tree_prime_hfid(proto_tree *tree, int hfid);

/** Get a parent item of a subtree.
 @param tree the tree to get the parent from
 @return parent item */
extern proto_item* proto_tree_get_parent(proto_tree *tree);



/** Add an item to a proto_tree, using the text label registered to that item.
   The item is extracted from the tvbuff handed to it.
 @param tree the tree to append this item to
 @param hfindex field index
 @param tvb the tv buffer of the current data
 @param start start of data in tvb
 @param length length of data in tvb
 @param little_endian big or little endian byte representation
 @return the newly created item */
extern proto_item *
proto_tree_add_item(proto_tree *tree, int hfindex, tvbuff_t *tvb,
    gint start, gint length, gboolean little_endian);

/** Add a hidden item to a proto_tree.
 @deprecated use proto_tree_add_item() and a subsequent call to PROTO_ITEM_SET_HIDDEN() instead */
extern proto_item *
proto_tree_add_item_hidden(proto_tree *tree, int hfindex, tvbuff_t *tvb,
    gint start, gint length, gboolean little_endian);

/** Add a text-only node to a proto_tree.
 @param tree the tree to append this item to
 @param tvb the tv buffer of the current data
 @param start start of data in tvb
 @param length length of data in tvb
 @param format printf like format string
 @param ... printf like parameters
 @return the newly created item */
extern proto_item *
proto_tree_add_text(proto_tree *tree, tvbuff_t *tvb, gint start, gint length, const char *format,
	...) GNUC_FORMAT_CHECK(printf,5,6);

/** Add a text-only node to a proto_tree using a variable argument list.
 @param tree the tree to append this item to
 @param tvb the tv buffer of the current data
 @param start start of data in tvb
 @param length length of data in tvb
 @param format printf like format string
 @param ap variable argument list
 @return the newly created item */
extern proto_item *
proto_tree_add_text_valist(proto_tree *tree, tvbuff_t *tvb, gint start,
	gint length, const char *format, va_list ap);


/** Add a FT_NONE field to a proto_tree.
 @param tree the tree to append this item to
 @param hfindex field index
 @param tvb the tv buffer of the current data
 @param start start of data in tvb
 @param length length of data in tvb
 @param format printf like format string
 @param ... printf like parameters
 @return the newly created item */
extern proto_item *
proto_tree_add_none_format(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, const char *format, ...) GNUC_FORMAT_CHECK(printf,6,7);

/** Add a FT_PROTOCOL to a proto_tree.
 @param tree the tree to append this item to
 @param hfindex field index
 @param tvb the tv buffer of the current data
 @param start start of data in tvb
 @param length length of data in tvb
 @param format printf like format string
 @param ... printf like parameters
 @return the newly created item */
extern proto_item *
proto_tree_add_protocol_format(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, const char *format, ...) GNUC_FORMAT_CHECK(printf,6,7);




/** Add a FT_BYTES to a proto_tree.
 @param tree the tree to append this item to
 @param hfindex field index
 @param tvb the tv buffer of the current data
 @param start start of data in tvb
 @param length length of data in tvb
 @param start_ptr pointer to the data to display
 @return the newly created item */
extern proto_item *
proto_tree_add_bytes(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, const guint8* start_ptr);

/** Add a hidden FT_BYTES to a proto_tree.
 @deprecated use proto_tree_add_bytes() and a subsequent call to PROTO_ITEM_SET_HIDDEN() instead */
extern proto_item *
proto_tree_add_bytes_hidden(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, const guint8* start_ptr);

/** Add a formatted FT_BYTES to a proto_tree.
 @param tree the tree to append this item to
 @param hfindex field index
 @param tvb the tv buffer of the current data
 @param start start of data in tvb
 @param length length of data in tvb
 @param start_ptr pointer to the data to display
 @param format printf like format string
 @param ... printf like parameters
 @return the newly created item */
extern proto_item *
proto_tree_add_bytes_format(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, const guint8* start_ptr, const char *format, ...) GNUC_FORMAT_CHECK(printf,7,8);

/** Add a FT_ABSOLUTE_TIME or FT_RELATIVE_TIME to a proto_tree.
 @param tree the tree to append this item to
 @param hfindex field index
 @param tvb the tv buffer of the current data
 @param start start of data in tvb
 @param length length of data in tvb
 @param value_ptr pointer to the data to display
 @return the newly created item */
extern proto_item *
proto_tree_add_time(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, nstime_t* value_ptr);

/** Add a hidden FT_ABSOLUTE_TIME or FT_RELATIVE_TIME to a proto_tree.
 @deprecated use proto_tree_add_time() and a subsequent call to PROTO_ITEM_SET_HIDDEN() instead */
extern proto_item *
proto_tree_add_time_hidden(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, nstime_t* value_ptr);

/** Add a formatted FT_ABSOLUTE_TIME or FT_RELATIVE_TIME to a proto_tree.
 @param tree the tree to append this item to
 @param hfindex field index
 @param tvb the tv buffer of the current data
 @param start start of data in tvb
 @param length length of data in tvb
 @param value_ptr pointer to the data to display
 @param format printf like format string
 @param ... printf like parameters
 @return the newly created item */
extern proto_item *
proto_tree_add_time_format(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, nstime_t* value_ptr, const char *format, ...) GNUC_FORMAT_CHECK(printf,7,8);

/** Add a FT_IPXNET to a proto_tree.
 @param tree the tree to append this item to
 @param hfindex field index
 @param tvb the tv buffer of the current data
 @param start start of data in tvb
 @param length length of data in tvb
 @param value data to display
 @return the newly created item */
extern proto_item *
proto_tree_add_ipxnet(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, guint32 value);

/** Add a hidden FT_IPXNET to a proto_tree.
 @deprecated use proto_tree_add_ipxnet() and a subsequent call to PROTO_ITEM_SET_HIDDEN() instead */
extern proto_item *
proto_tree_add_ipxnet_hidden(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, guint32 value);

/** Add a formatted FT_IPXNET to a proto_tree.
 @param tree the tree to append this item to
 @param hfindex field index
 @param tvb the tv buffer of the current data
 @param start start of data in tvb
 @param length length of data in tvb
 @param value data to display
 @param format printf like format string
 @param ... printf like parameters
 @return the newly created item */
extern proto_item *
proto_tree_add_ipxnet_format(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, guint32 value, const char *format, ...) GNUC_FORMAT_CHECK(printf,7,8);

/** Add a FT_IPv4 to a proto_tree.
 @param tree the tree to append this item to
 @param hfindex field index
 @param tvb the tv buffer of the current data
 @param start start of data in tvb
 @param length length of data in tvb
 @param value data to display
 @return the newly created item */
extern proto_item *
proto_tree_add_ipv4(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, guint32 value);

/** Add a hidden FT_IPv4 to a proto_tree.
 @deprecated use proto_tree_add_ipv4() and a subsequent call to PROTO_ITEM_SET_HIDDEN() instead */
extern proto_item *
proto_tree_add_ipv4_hidden(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, guint32 value);

/** Add a formatted FT_IPv4 to a proto_tree.
 @param tree the tree to append this item to
 @param hfindex field index
 @param tvb the tv buffer of the current data
 @param start start of data in tvb
 @param length length of data in tvb
 @param value data to display
 @param format printf like format string
 @param ... printf like parameters
 @return the newly created item */
extern proto_item *
proto_tree_add_ipv4_format(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, guint32 value, const char *format, ...) GNUC_FORMAT_CHECK(printf,7,8);

/** Add a FT_IPv6 to a proto_tree.
 @param tree the tree to append this item to
 @param hfindex field index
 @param tvb the tv buffer of the current data
 @param start start of data in tvb
 @param length length of data in tvb
 @param value_ptr data to display
 @return the newly created item */
extern proto_item *
proto_tree_add_ipv6(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, const guint8* value_ptr);

/** Add a hidden FT_IPv6 to a proto_tree.
 @deprecated use proto_tree_add_ipv6() and a subsequent call to PROTO_ITEM_SET_HIDDEN() instead */
extern proto_item *
proto_tree_add_ipv6_hidden(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, const guint8* value_ptr);

/** Add a formatted FT_IPv6 to a proto_tree.
 @param tree the tree to append this item to
 @param hfindex field index
 @param tvb the tv buffer of the current data
 @param start start of data in tvb
 @param length length of data in tvb
 @param value_ptr data to display
 @param format printf like format string
 @param ... printf like parameters
 @return the newly created item */
extern proto_item *
proto_tree_add_ipv6_format(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, const guint8* value_ptr, const char *format, ...) GNUC_FORMAT_CHECK(printf,7,8);

/** Add a FT_ETHER to a proto_tree.
 @param tree the tree to append this item to
 @param hfindex field index
 @param tvb the tv buffer of the current data
 @param start start of data in tvb
 @param length length of data in tvb
 @param value data to display
 @return the newly created item */
extern proto_item *
proto_tree_add_ether(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, const guint8* value);

/** Add a hidden FT_ETHER to a proto_tree.
 @deprecated use proto_tree_add_ether() and a subsequent call to PROTO_ITEM_SET_HIDDEN() instead */
extern proto_item *
proto_tree_add_ether_hidden(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, const guint8* value);

/** Add a formatted FT_ETHER to a proto_tree.
 @param tree the tree to append this item to
 @param hfindex field index
 @param tvb the tv buffer of the current data
 @param start start of data in tvb
 @param length length of data in tvb
 @param value data to display
 @param format printf like format string
 @param ... printf like parameters
 @return the newly created item */
extern proto_item *
proto_tree_add_ether_format(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, const guint8* value, const char *format, ...) GNUC_FORMAT_CHECK(printf,7,8);

/** Add a FT_STRING to a proto_tree.
 @param tree the tree to append this item to
 @param hfindex field index
 @param tvb the tv buffer of the current data
 @param start start of data in tvb
 @param length length of data in tvb
 @param value data to display
 @return the newly created item */
extern proto_item *
proto_tree_add_string(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, const char* value);

/** Add a hidden FT_STRING to a proto_tree.
 @deprecated use proto_tree_add_string() and a subsequent call to PROTO_ITEM_SET_HIDDEN() instead */
extern proto_item *
proto_tree_add_string_hidden(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, const char* value);

/** Add a formatted FT_STRING to a proto_tree.
 @param tree the tree to append this item to
 @param hfindex field index
 @param tvb the tv buffer of the current data
 @param start start of data in tvb
 @param length length of data in tvb
 @param value data to display
 @param format printf like format string
 @param ... printf like parameters
 @return the newly created item */
extern proto_item *
proto_tree_add_string_format(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, const char* value, const char *format, ...) GNUC_FORMAT_CHECK(printf,7,8);

/** Add a FT_BOOLEAN to a proto_tree.
 @param tree the tree to append this item to
 @param hfindex field index
 @param tvb the tv buffer of the current data
 @param start start of data in tvb
 @param length length of data in tvb
 @param value data to display
 @return the newly created item */
extern proto_item *
proto_tree_add_boolean(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, guint32 value);

/** Add a hidden FT_BOOLEAN to a proto_tree.
 @deprecated use proto_tree_add_boolean() and a subsequent call to PROTO_ITEM_SET_HIDDEN() instead */
extern proto_item *
proto_tree_add_boolean_hidden(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, guint32 value);

/** Add a formatted FT_BOOLEAN to a proto_tree.
 @param tree the tree to append this item to
 @param hfindex field index
 @param tvb the tv buffer of the current data
 @param start start of data in tvb
 @param length length of data in tvb
 @param value data to display
 @param format printf like format string
 @param ... printf like parameters
 @return the newly created item */
extern proto_item *
proto_tree_add_boolean_format(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, guint32 value, const char *format, ...) GNUC_FORMAT_CHECK(printf,7,8);

/** Add a FT_FLOAT to a proto_tree.
 @param tree the tree to append this item to
 @param hfindex field index
 @param tvb the tv buffer of the current data
 @param start start of data in tvb
 @param length length of data in tvb
 @param value data to display
 @return the newly created item */
extern proto_item *
proto_tree_add_float(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, float value);

/** Add a hidden FT_FLOAT to a proto_tree.
 @deprecated use proto_tree_add_float() and a subsequent call to PROTO_ITEM_SET_HIDDEN() instead */
extern proto_item *
proto_tree_add_float_hidden(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, float value);

/** Add a formatted FT_FLOAT to a proto_tree.
 @param tree the tree to append this item to
 @param hfindex field index
 @param tvb the tv buffer of the current data
 @param start start of data in tvb
 @param length length of data in tvb
 @param value data to display
 @param format printf like format string
 @param ... printf like parameters
 @return the newly created item */
extern proto_item *
proto_tree_add_float_format(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, float value, const char *format, ...) GNUC_FORMAT_CHECK(printf,7,8);

/** Add a FT_DOUBLE to a proto_tree.
 @param tree the tree to append this item to
 @param hfindex field index
 @param tvb the tv buffer of the current data
 @param start start of data in tvb
 @param length length of data in tvb
 @param value data to display
 @return the newly created item */
extern proto_item *
proto_tree_add_double(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, double value);

/** Add a hidden FT_DOUBLE to a proto_tree.
 @deprecated use proto_tree_add_double() and a subsequent call to PROTO_ITEM_SET_HIDDEN() instead */
extern proto_item *
proto_tree_add_double_hidden(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, double value);

/** Add a formatted FT_DOUBLE to a proto_tree.
 @param tree the tree to append this item to
 @param hfindex field index
 @param tvb the tv buffer of the current data
 @param start start of data in tvb
 @param length length of data in tvb
 @param value data to display
 @param format printf like format string
 @param ... printf like parameters
 @return the newly created item */
extern proto_item *
proto_tree_add_double_format(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, double value, const char *format, ...) GNUC_FORMAT_CHECK(printf,7,8);

/** Add one of FT_UINT8, FT_UINT16, FT_UINT24 or FT_UINT32 to a proto_tree.
 @param tree the tree to append this item to
 @param hfindex field index
 @param tvb the tv buffer of the current data
 @param start start of data in tvb
 @param length length of data in tvb
 @param value data to display
 @return the newly created item */
extern proto_item *
proto_tree_add_uint(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, guint32 value);

/** Add a hidden of one of FT_UINT8, FT_UINT16, FT_UINT24 or FT_UINT32 to a proto_tree.
 @deprecated use proto_tree_add_uint() and a subsequent call to PROTO_ITEM_SET_HIDDEN() instead */
extern proto_item *
proto_tree_add_uint_hidden(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, guint32 value);

/** Add a formatted of one of FT_UINT8, FT_UINT16, FT_UINT24 or FT_UINT32 to a proto_tree.
 @param tree the tree to append this item to
 @param hfindex field index
 @param tvb the tv buffer of the current data
 @param start start of data in tvb
 @param length length of data in tvb
 @param value data to display
 @param format printf like format string
 @param ... printf like parameters
 @return the newly created item */
extern proto_item *
proto_tree_add_uint_format(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, guint32 value, const char *format, ...) GNUC_FORMAT_CHECK(printf,7,8);

/** Add one of FT_INT8, FT_INT16, FT_INT24 or FT_INT32 to a proto_tree.
 @param tree the tree to append this item to
 @param hfindex field index
 @param tvb the tv buffer of the current data
 @param start start of data in tvb
 @param length length of data in tvb
 @param value data to display
 @return the newly created item */
extern proto_item *
proto_tree_add_int(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, gint32 value);

/** Add a hidden of one of FT_INT8, FT_INT16, FT_INT24 or FT_INT32 to a proto_tree.
 @deprecated use proto_tree_add_int() and a subsequent call to PROTO_ITEM_SET_HIDDEN() instead */
extern proto_item *
proto_tree_add_int_hidden(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, gint32 value);

/** Add a formatted of one of FT_INT8, FT_INT16, FT_INT24 or FT_INT32 to a proto_tree.
 @param tree the tree to append this item to
 @param hfindex field index
 @param tvb the tv buffer of the current data
 @param start start of data in tvb
 @param length length of data in tvb
 @param value data to display
 @param format printf like format string
 @param ... printf like parameters
 @return the newly created item */
extern proto_item *
proto_tree_add_int_format(proto_tree *tree, int hfindex, tvbuff_t *tvb, gint start,
	gint length, gint32 value, const char *format, ...) GNUC_FORMAT_CHECK(printf,7,8);

/** Useful for quick debugging. Also sends string to STDOUT, so don't
 * leave call to this function in production code.
 @param tree the tree to append the text to
 @param format printf like format string
 @param ... printf like parameters
 @return the newly created item */
extern proto_item *
proto_tree_add_debug_text(proto_tree *tree, const char *format, 
	...) GNUC_FORMAT_CHECK(printf,2,3);



/** Append a string to a protocol item.
 @param pi the item to append the string to
 @param str the string to append */
extern void
proto_item_append_string(proto_item *pi, const char *str);



/** Fill given label_str with string representation of field
 @param fi the item to get the info from
 @param label_str the string to fill 
 @todo think about changing the parameter profile */
extern void
proto_item_fill_label(field_info *fi, gchar *label_str);


/** Register a new protocol.
 @param name the name of the new protocol
 @param short_name abbreviated name of the new protocol
 @param filter_name protocol name used for a display filter string 
 @return the new protocol handle */
extern int
proto_register_protocol(char *name, char *short_name, char *filter_name);

/** Register a header_field array.
 @param parent the protocol handle from proto_register_protocol()
 @param hf the hf_register_info array
 @param num_records the number of records in hf */
extern void
proto_register_field_array(int parent, hf_register_info *hf, int num_records);

/** Register a protocol subtree (ett) array.
 @param indices array of ett indices
 @param num_indices the number of records in indices */
extern void
proto_register_subtree_array(gint **indices, int num_indices);

/** Returns number of items (protocols or header fields) registered.
 @return the number of items */
extern int proto_registrar_n(void);

/** Get name of registered header_field number n.
 @param n item # n (0-indexed)
 @return the name of this registered item */
extern char* proto_registrar_get_name(int n);

/** Get abbreviation of registered header_field number n.
 @param n item # n (0-indexed)
 @return the abbreviation of this registered item */
extern char* proto_registrar_get_abbrev(int n);

/** Get the header_field information based upon a field or protocol id.
 @param hfindex item # n (0-indexed)
 @return the registered item */
extern header_field_info* proto_registrar_get_nth(guint hfindex);

/** Get the header_field information based upon a field name.
 @param field_name the field name to search for
 @return the registered item */
extern header_field_info* proto_registrar_get_byname(char *field_name);

/** Get enum ftenum FT_ of registered header_field number n.
 @param n item # n (0-indexed)
 @return the registered item */
extern int proto_registrar_get_ftype(int n);

/** Get parent protocol of registered header_field number n.
 @param n item # n (0-indexed)
 @return -1 if item _is_ a protocol */
extern int proto_registrar_get_parent(int n);

/** Is item #n a protocol?
 @param n item # n (0-indexed)
 @return TRUE if it's a protocol, FALSE if it's not */
extern gboolean proto_registrar_is_protocol(int n);

/* Get length of registered field according to field type.
 @param n item # n (0-indexed)
 @return 0 means undeterminable at registration time, * -1 means unknown field */
extern gint proto_registrar_get_length(int n);


/* Routines to use to iterate over the protocols and their fields;
 * they return the item number of the protocol in question or the
 * appropriate hfinfo pointer, and keep state in "*cookie". */
extern int proto_get_first_protocol(void **cookie);
extern int proto_get_next_protocol(void **cookie);
extern header_field_info *proto_get_first_protocol_field(int proto_id, void **cookle);
extern header_field_info *proto_get_next_protocol_field(void **cookle);

/** Given a protocol's filter_name.
 @param filter_name the filter name to search for
 @return proto_id */
extern int proto_get_id_by_filter_name(gchar* filter_name);

/** Can item #n decoding be disabled?
 @param proto_id protocol id (0-indexed)
 @return TRUE if it's a protocol, FALSE if it's not */
extern gboolean proto_can_toggle_protocol(int proto_id);

/** Get the "protocol_t" structure for the given protocol's item number.
 @param proto_id protocol id (0-indexed) */
extern protocol_t *find_protocol_by_id(int proto_id);

/** Get the protocol's name for the given protocol's item number.
 @param proto_id protocol id (0-indexed)
 @return its name */
extern char *proto_get_protocol_name(int proto_id);

/** Get the protocol's item number, for the given protocol's "protocol_t".
 @return its proto_id */
extern int proto_get_id(protocol_t *protocol);

/** Get the protocol's short name, for the given protocol's "protocol_t".
 @return its short name. */
extern char *proto_get_protocol_short_name(protocol_t *protocol);

/** Is protocol's decoding enabled ?
 @param protocol 
 @return TRUE if decoding is enabled, FALSE if not */
extern gboolean proto_is_protocol_enabled(protocol_t *protocol);

/** Get a protocol's filter name by it's item number.
 @param proto_id protocol id (0-indexed)
 @return its filter name. */
extern char *proto_get_protocol_filter_name(int proto_id);

/** Enable / Disable protocol of the given item number.
 @param proto_id protocol id (0-indexed)
 @param enabled enable / disable the protocol */
extern void proto_set_decoding(int proto_id, gboolean enabled);

/** Disable disabling/enabling of protocol of the given item number.
 @param proto_id protocol id (0-indexed) */
extern void proto_set_cant_toggle(int proto_id);

/** Checks for existence any protocol or field within a tree.
 @param tree "Protocols" are assumed to be a child of the [empty] root node.
 @param id ???
 @return TRUE = found, FALSE = not found
 @todo add explanation of id parameter */
extern gboolean proto_check_for_protocol_or_field(proto_tree* tree, int id);

/* Return GPtrArray* of field_info pointers for all hfindex that appear in
 * tree. Only works with primed trees, and is fast. */
extern GPtrArray* proto_get_finfo_ptr_array(proto_tree *tree, int hfindex);

/* Return GPtrArray* of field_info pointers for all hfindex that appear in
 * tree. Works with any tree, primed or unprimed, and is slower than
 * proto_get_finfo_ptr_array because it has to search through the tree. */
extern GPtrArray* proto_find_finfo(proto_tree *tree, int hfindex);

/** Dumps a glossary of the protocol registrations to STDOUT */
extern void proto_registrar_dump_protocols(void);

/** Dumps a glossary of the protocol and field registrations to STDOUT */
extern void proto_registrar_dump_fields(void);



/** Points to the first element of an array of Booleans, indexed by
   a subtree item type. That array element is TRUE if subtrees of
   an item of that type are to be expanded. With MSVC and a 
   libethereal.dll, we need a special declaration. */
ETH_VAR_IMPORT gboolean	     *tree_is_expanded;

/** Number of elements in the tree_is_expanded array. With MSVC and a 
 * libethereal.dll, we need a special declaration. */
ETH_VAR_IMPORT int           num_tree_types;

/** glib doesn't have g_ptr_array_len of all things!*/
#ifndef g_ptr_array_len
#define g_ptr_array_len(a)      ((a)->len)
#endif

/** Get number of bits of a header_field.
 @param hfinfo header_field
 @return the bitwidth */
extern int
hfinfo_bitwidth(header_field_info *hfinfo);




#include "epan.h"

/** Can we do a "match selected" on this field.
 @param finfo field_info
 @param edt epan dissecting
 @return TRUE if we can do a "match selected" on the field, FALSE otherwise. */
extern gboolean
proto_can_match_selected(field_info *finfo, epan_dissect_t *edt);

/** Construct a display filter string.
 @param finfo field_info
 @param edt epan dissecting
 @return the display filter string */
extern char*
proto_construct_dfilter_string(field_info *finfo, epan_dissect_t *edt);

/** Find field from offset in tvb.
 @param tree 
 @param offset offset in the tvb
 @param tvb the tv buffer
 @return the corresponding field_info */
extern field_info*
proto_find_field_from_offset(proto_tree *tree, guint offset, tvbuff_t *tvb);

#endif /* proto.h */
