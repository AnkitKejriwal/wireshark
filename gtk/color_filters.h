/* color_filters.h
 * Definitions for color filters
 *
 * $Id: color_filters.h,v 1.5 2004/03/14 23:55:53 deniel Exp $
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
#ifndef  __COLOR_FILTERS_H__
#define  __COLOR_FILTERS_H__

#define CFILTERS_CONTAINS_FILTER(filter) \
	((filter)->num_of_filters != 0)

void colfilter_init(void);

gboolean write_filters(void);
gboolean revert_filters(void);

color_filter_t *new_color_filter(gchar *name, gchar *filter_string);
void remove_color_filter(color_filter_t *colorf);
gboolean read_other_filters(gchar *path, gpointer arg);
gboolean write_other_filters(gchar *path, gboolean only_marked);

#endif
