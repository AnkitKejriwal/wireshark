/* dfilter.h
 * Definitions for display filters
 *
 * $Id: dfilter.h,v 1.7 1999/08/13 23:47:41 gram Exp $
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

#define DFILTER_CONTAINS_FILTER(x)	((x)->dftree)

typedef struct {

	GNode *dftree;
	gchar *dftext;

	/* space for dfilter_nodes */
	GMemChunk *node_memchunk;

	/* list of byte arrays we allocate during parse. We can traverse this list
	 * faster than the tree when we go back and free the byte arrays */
	GSList *list_of_byte_arrays;

} dfilter;

/* Initialization of the symbol table. Called once during program startup */
void dfilter_init(void);

/* Allocate and initialize new dfilter struct. Returns pointer to new dfilter */
dfilter* dfilter_new(void);

/* Frees all memory used by dfilter, and frees dfilter itself */
void dfilter_destroy(dfilter *df);

/* Compile display filter text */
int dfilter_compile(dfilter* df, gchar* dfilter_text);

/* Apply compiled dfilter to a proto_tree */
gboolean dfilter_apply(dfilter *df, proto_tree *ptree, const guint8* pd);

/* Clears the current filter int the dfilter */
void dfilter_clear_filter(dfilter *df);


#endif /* ! __DFILTER_H__ */
