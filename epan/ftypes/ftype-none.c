/*
 * $Id: ftype-none.c,v 1.8 2003/12/06 16:35:19 gram Exp $
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@zing.org>
 * Copyright 2001 Gerald Combs
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
#include "config.h"
#endif

#include <ftypes-int.h>


void
ftype_register_none(void)
{

	static ftype_t none_type = {
		"FT_NONE",
		"label",
		0,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,				/* val_to_string_repr */
		NULL,				/* len_string_repr */

		NULL,
		NULL,
		NULL,

		NULL,
		NULL,
		NULL,

		NULL,				/* cmp_eq */
		NULL,				/* cmp_ne */
		NULL,				/* cmp_gt */
		NULL,				/* cmp_ge */
		NULL,				/* cmp_lt */
		NULL,				/* cmp_le */
		NULL,				/* cmp_contains */
		NULL,				/* cmp_matches */

		NULL,
		NULL,
	};
	ftype_register(FT_NONE, &none_type);
}
