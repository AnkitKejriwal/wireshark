/* dfilter.h
 * Definitions for display filters
 *
 * $Id: dfilter.h,v 1.17 2000/08/01 18:10:06 gram Exp $
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

#ifndef __DFILTER_H__
#define __DFILTER_H__

#ifndef __PROTO_H__
#include "proto.h"
#endif

/* dfilter_error_msg is NULL if there was no error during dfilter_compile,
 * otherwise it points to a displayable error message. */
extern gchar *dfilter_error_msg;
extern gchar dfilter_error_msg_buf[1024];

typedef struct {

	GNode *dftree;

	/* space for dfilter_nodes */
	GMemChunk *node_memchunk;

	/* list of byte arrays we allocate during parse. We can traverse this list
	 * faster than the tree when we go back and free the byte arrays */
	GSList *list_of_byte_arrays;

	/* List of strings allocated during parse. */
	GSList *list_of_strings;

} dfilter;

/* Initialization of the symbol table. Called once during program startup */
void dfilter_init(void);

/* Free the memory used by the symbol table. Called at program shutdown */
void dfilter_cleanup(void);

/* Allocate and initialize new dfilter struct. Returns pointer to new dfilter */
dfilter* dfilter_new(void);

/* Frees all memory used by dfilter, and frees dfilter itself */
void dfilter_destroy(dfilter *df);

/* Compile display filter text */
int dfilter_compile(gchar* dfilter_text, dfilter** dfp);

/* Apply compiled dfilter to a proto_tree */
gboolean dfilter_apply(dfilter *df, proto_tree *ptree, const guint8* pd, guint pd_len);

#endif /* ! __DFILTER_H__ */
