/* color_utils.h
 * Declarations of utilities for converting between "toolkit-independent"
 * and GDK notions of color
 *
 * $Id: color_utils.h,v 1.3 2004/06/01 20:28:04 ulfl Exp $
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

#ifndef __COLOR_UTILS_H__
#define __COLOR_UTILS_H__

/** @file
 * Utilities for converting between "toolkit-independent"
 * and GDK notions of color.
 */

/** Convert color_t to GdkColor.
 *
 * @param target the GdkColor to be filled
 * @param source the source color_t
 */
void color_t_to_gdkcolor(GdkColor *target, color_t *source);

/** Convert GdkColor to color_t.
 *
 * @param target the source color_t
 * @param source the GdkColor to be filled
 */
void gdkcolor_to_color_t(color_t *target, GdkColor *source);

#endif
