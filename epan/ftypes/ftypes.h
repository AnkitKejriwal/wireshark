/* ftypes.h
 * Definitions for field types
 *
 * $Id: ftypes.h,v 1.28 2003/12/10 21:12:02 gerald Exp $
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@ethereal.com>
 * Copyright 2001 Gerald Combs
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


#ifndef FTYPES_H
#define FTYPES_H

#include <glib.h>
#include "../slab.h"

/* field types */
enum ftenum {
	FT_NONE,	/* used for text labels with no value */
	FT_PROTOCOL,
	FT_BOOLEAN,	/* TRUE and FALSE come from <glib.h> */
	FT_UINT8,
	FT_UINT16,
	FT_UINT24,	/* really a UINT32, but displayed as 3 hex-digits if FD_HEX*/
	FT_UINT32,
	FT_UINT64,
	FT_INT8,
	FT_INT16,
	FT_INT24,	/* same as for UINT24 */
	FT_INT32,
	FT_INT64,
	FT_FLOAT,
	FT_DOUBLE,
	FT_ABSOLUTE_TIME,
	FT_RELATIVE_TIME,
	FT_STRING,
	FT_STRINGZ,	/* for use with proto_tree_add_item() */
	FT_UINT_STRING,	/* for use with proto_tree_add_item() */
	/*FT_UCS2_LE, */    /* Unicode, 2 byte, Little Endian     */
	FT_ETHER,
	FT_BYTES,
	FT_UINT_BYTES,
	FT_IPv4,
	FT_IPv6,
	FT_IPXNET,
	FT_FRAMENUM,	/* a UINT32, but if selected lets you go to frame with that numbe */
	FT_PCRE,		/* a compiled Perl-Compatible Regular Expression object */
	FT_NUM_TYPES /* last item number plus one */
};

typedef enum ftenum ftenum_t;
typedef struct _ftype_t ftype_t;

/* String representation types. */
enum ftrepr {
    FTREPR_DISPLAY,
    FTREPR_DFILTER
};

typedef enum ftrepr ftrepr_t;

#ifdef HAVE_LIBPCRE
typedef struct _pcre_tuple_t pcre_tuple_t;
#endif /* HAVE_LIBPCRE */

/* Initialize the ftypes subsytem. Called once. */
void
ftypes_initialize(void);

/* ---------------- FTYPE ----------------- */

/* Return a string representing the name of the type */
const char*
ftype_name(ftenum_t ftype);

/* Return a string presenting a "pretty" representation of the
 * name of the type. The pretty name means more to the user than
 * that "FT_*" name. */
const char*
ftype_pretty_name(ftenum_t ftype);

/* Returns length of field in packet, or 0 if not determinable/defined. */
int
ftype_length(ftenum_t ftype);

gboolean
ftype_can_slice(enum ftenum ftype);

gboolean
ftype_can_eq(enum ftenum ftype);

gboolean
ftype_can_ne(enum ftenum ftype);

gboolean
ftype_can_gt(enum ftenum ftype);

gboolean
ftype_can_ge(enum ftenum ftype);

gboolean
ftype_can_lt(enum ftenum ftype);

gboolean
ftype_can_le(enum ftenum ftype);

gboolean
ftype_can_contains(enum ftenum ftype);

gboolean
ftype_can_matches(enum ftenum ftype);

/* ---------------- FVALUE ----------------- */

#include <epan/ipv4.h>

#include <epan/tvbuff.h>
#include <epan/nstime.h>
#include <epan/dfilter/drange.h>

typedef struct _fvalue_t {
	ftype_t	*ftype;
	union {
		/* Put a few basic types in here */
		gpointer	pointer;
		guint32		integer;
		gdouble		floating;
		gchar		*string;
		GByteArray	*bytes;
		ipv4_addr	ipv4;
		nstime_t	time;
		tvbuff_t	*tvb;
#ifdef HAVE_LIBPCRE
		pcre_tuple_t	*re;
#endif /* HAVE_LIBPCRE */
	} value;

	/* The following is provided for private use
	 * by the fvalue. */
	gboolean	fvalue_gboolean1;

} fvalue_t;

typedef void (*FvalueNewFunc)(fvalue_t*);
typedef void (*FvalueFreeFunc)(fvalue_t*);
typedef void (*LogFunc)(char*,...);

typedef gboolean (*FvalueFromUnparsed)(fvalue_t*, char*, gboolean, LogFunc);
typedef gboolean (*FvalueFromString)(fvalue_t*, char*, LogFunc);
typedef void (*FvalueToStringRepr)(fvalue_t*, ftrepr_t, char*);
typedef int (*FvalueStringReprLen)(fvalue_t*, ftrepr_t);

typedef void (*FvalueSetFunc)(fvalue_t*, gpointer, gboolean);
typedef void (*FvalueSetIntegerFunc)(fvalue_t*, guint32);
typedef void (*FvalueSetFloatingFunc)(fvalue_t*, gdouble);

typedef gpointer (*FvalueGetFunc)(fvalue_t*);
typedef guint32 (*FvalueGetIntegerFunc)(fvalue_t*);
typedef double (*FvalueGetFloatingFunc)(fvalue_t*);

typedef gboolean (*FvalueCmp)(fvalue_t*, fvalue_t*);

typedef guint (*FvalueLen)(fvalue_t*);
typedef void (*FvalueSlice)(fvalue_t*, GByteArray *, guint offset, guint length);

struct _ftype_t {
	const char		*name;
	const char		*pretty_name;
	int			wire_size;
	FvalueNewFunc		new_value;
	FvalueFreeFunc		free_value;
	FvalueFromUnparsed	val_from_unparsed;
	FvalueFromString	val_from_string;
	FvalueToStringRepr	val_to_string_repr;
	FvalueStringReprLen	len_string_repr;

	/* could be union */
	FvalueSetFunc		set_value;
	FvalueSetIntegerFunc	set_value_integer;
	FvalueSetFloatingFunc	set_value_floating;

	/* could be union */
	FvalueGetFunc		get_value;
	FvalueGetIntegerFunc	get_value_integer;
	FvalueGetFloatingFunc	get_value_floating;

	FvalueCmp		cmp_eq;
	FvalueCmp		cmp_ne;
	FvalueCmp		cmp_gt;
	FvalueCmp		cmp_ge;
	FvalueCmp		cmp_lt;
	FvalueCmp		cmp_le;
	FvalueCmp		cmp_contains;
	FvalueCmp		cmp_matches;

	FvalueLen		len;
	FvalueSlice		slice;
};


fvalue_t*
fvalue_new(ftenum_t ftype);

void
fvalue_init(fvalue_t *fv, ftenum_t ftype);


/* Free all memory used by an fvalue_t */
extern fvalue_t *fvalue_free_list;
#define FVALUE_CLEANUP(fv)					\
	{							\
		register FvalueFreeFunc	free_value;		\
		free_value = (fv)->ftype->free_value;	\
		if (free_value) {				\
			free_value((fv));			\
		}						\
	}

#define FVALUE_FREE(fv)						\
	{							\
		FVALUE_CLEANUP(fv)				\
		SLAB_FREE(fv, fvalue_free_list);		\
	}


fvalue_t*
fvalue_from_unparsed(ftenum_t ftype, char *s, gboolean allow_partial_value, LogFunc logfunc);

fvalue_t*
fvalue_from_string(ftenum_t ftype, char *s, LogFunc logfunc);

/* Returns the length of the string required to hold the
 * string representation of the the field value.
 * The length DOES NOT include the terminating NUL. */
int
fvalue_string_repr_len(fvalue_t *fv, ftrepr_t rtype);

/* Creates the string representation of the field value.
 * If given non-NULL 'buf', the string is written at the memory
 * location pointed to by 'buf'. If 'buf' is NULL, new memory
 * is malloc'ed and the string representation is written there.
 * The pointer to the beginning of the string representation is
 * returned. If 'buf' was NULL, this points to the newly-allocated
 * memory. if 'buf' was non-NULL, then the return value will be
 * 'buf'. */
char *
fvalue_to_string_repr(fvalue_t *fv, ftrepr_t rtype, char *buf);

const char*
fvalue_type_name(fvalue_t *fv);

void
fvalue_set(fvalue_t *fv, gpointer value, gboolean already_copied);

void
fvalue_set_integer(fvalue_t *fv, guint32 value);

void
fvalue_set_floating(fvalue_t *fv, gdouble value);

gpointer
fvalue_get(fvalue_t *fv);

guint32
fvalue_get_integer(fvalue_t *fv);

double
fvalue_get_floating(fvalue_t *fv);

gboolean
fvalue_eq(fvalue_t *a, fvalue_t *b);

gboolean
fvalue_ne(fvalue_t *a, fvalue_t *b);

gboolean
fvalue_gt(fvalue_t *a, fvalue_t *b);

gboolean
fvalue_ge(fvalue_t *a, fvalue_t *b);

gboolean
fvalue_lt(fvalue_t *a, fvalue_t *b);

gboolean
fvalue_le(fvalue_t *a, fvalue_t *b);

gboolean
fvalue_contains(fvalue_t *a, fvalue_t *b);

gboolean
fvalue_matches(fvalue_t *a, fvalue_t *b);

guint
fvalue_length(fvalue_t *fv);

fvalue_t*
fvalue_slice(fvalue_t *fv, drange *drange);

#endif /* ftypes.h */
