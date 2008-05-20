/* ptvcursor.h
 *
 * Proto Tree TVBuff cursor
 * Gilbert Ramirez <gram@alumni.rice.edu>
 *
 * $Id$
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 2000 Gerald Combs
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

#ifndef __PTVCURSOR_H__
#define __PTVCURSOR_H__

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glib.h>
#include <epan/packet.h>

#define SUBTREE_UNDEFINED_LENGTH -1

typedef struct ptvcursor ptvcursor_t;

/* Allocates an initializes a ptvcursor_t with 3 variables:
 * proto_tree, tvbuff, and offset. */
ptvcursor_t*
ptvcursor_new(proto_tree* tree, tvbuff_t* tvb, gint offset);

/* Gets data from tvbuff, adds it to proto_tree, increments offset,
 * and returns proto_item* */
proto_item*
ptvcursor_add(ptvcursor_t* ptvc, int hf, gint length, gboolean endianness);


/* Gets data from tvbuff, adds it to proto_tree, *DOES NOT* increment
 * offset, and returns proto_item* */
proto_item*
ptvcursor_add_no_advance(ptvcursor_t* ptvc, int hf, gint length, gboolean endianness);

/* Advance the ptvcursor's offset within its tvbuff without
 * adding anything to the proto_tree. */
void
ptvcursor_advance(ptvcursor_t* ptvc, gint length);

/* Frees memory for ptvcursor_t, but nothing deeper than that. */
void
ptvcursor_free(ptvcursor_t* ptvc);

/* Returns tvbuff. */
tvbuff_t*
ptvcursor_tvbuff(ptvcursor_t* ptvc);

/* Returns current offset. */
gint
ptvcursor_current_offset(ptvcursor_t* ptvc);

/* Returns the proto_tree* */
proto_tree*
ptvcursor_tree(ptvcursor_t* ptvc);

/* Sets a new proto_tree* for the ptvcursor_t */
void
ptvcursor_set_tree(ptvcursor_t* ptvc, proto_tree* tree);

/* push a subtree in the tree stack of the cursor */
proto_tree*
ptvcursor_push_subtree(ptvcursor_t* ptvc, proto_item* it, gint ett_subtree);

/* pop a subtree in the tree stack of the cursor */
void
ptvcursor_pop_subtree(ptvcursor_t* ptvc);

/* Add an item to the tree and create a subtree
 * If the length is unknown, length may be defined as SUBTREE_UNDEFINED_LENGTH.
 * In this case, when the subtree will be closed, the parent item length will
 * be equal to the advancement of the cursor since the creation of the subtree.
 */
proto_tree*
ptvcursor_add_with_subtree(ptvcursor_t* ptvc, int hfindex, gint length,
    gboolean little_endian, gint ett_subtree);

/* Add a text node to the tree and create a subtree
 * If the length is unknown, length may be defined as SUBTREE_UNDEFINED_LENGTH.
 * In this case, when the subtree will be closed, the item length will be equal
 * to the advancement of the cursor since the creation of the subtree.
 */
proto_tree*
ptvcursor_add_text_with_subtree(ptvcursor_t* ptvc, gint length,
    gint ett_subtree, const char* format, ...);

/* Creates a subtree and adds it to the cursor as the working tree but does not
 * save the old working tree */
proto_tree*
ptvcursor_set_subtree(ptvcursor_t* ptvc, proto_item* it, gint ett_subtree);

#endif /* __PTVCURSOR_H__ */
