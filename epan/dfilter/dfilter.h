/*
 * $Id: dfilter.h,v 1.5 2002/05/09 23:50:30 gram Exp $
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

#ifndef DFILTER_H
#define DFILTER_H

#include <glib.h>

/* Passed back to user */
typedef struct _dfilter_t dfilter_t;

#include <epan/epan.h>
#include <epan/proto.h>


/* Module-level initialization */
void
dfilter_init(void);

/* Module-level cleanup */
void
dfilter_cleanup(void);

/* Compiles a string to a dfilter_t.
 * On success, sets the dfilter* pointed to by dfp
 * to either a NULL pointer (if the filter is a null
 * filter, as generated by an all-blank string) or to
 * a pointer to the newly-allocated dfilter_t
 * structure.
 *
 * On failure, dfilter_error_msg points to an
 * appropriate error message. This error message is
 * a global string, so another invocation of
 * dfilter_compile() will clear it. The dfilter*
 * will be set to NULL after a failure.
 *
 * Returns TRUE on success, FALSE on failure.
 */
gboolean
dfilter_compile(gchar *text, dfilter_t **dfp);

/* Frees all memory used by dfilter, and frees
 * the dfilter itself. */
void
dfilter_free(dfilter_t *df);


/* dfilter_error_msg is NULL if there was no error during dfilter_compile,
 * otherwise it points to a displayable error message. */
extern gchar *dfilter_error_msg;

/* Apply compiled dfilter */
gboolean
dfilter_apply_edt(dfilter_t *df, epan_dissect_t* edt);

/* Apply compiled dfilter */
gboolean
dfilter_apply(dfilter_t *df, proto_tree *tree);

/* Prime a proto_tree using the fields/protocols used in a dfilter. */
void
dfilter_prime_proto_tree(dfilter_t *df, proto_tree *tree);

/* Print bytecode of dfilter to stdout */
void
dfilter_dump(dfilter_t *df);

#endif
