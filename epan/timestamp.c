/* timestamp.c
 * Routines for timestamp type setting.
 *
 * $Id: timestamp.c,v 1.1 2004/03/18 21:14:37 obiot Exp $
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "timestamp.h"

/* Init with an invalid value, so that "recent" in gtk/menu.c can detect this
 * and distinguish it from a command line value */
ts_type timestamp_type = TS_NOT_SET;

ts_type get_timestamp_setting(void)
{
	return timestamp_type;
}

void set_timestamp_setting(ts_type ts_t)
{
	timestamp_type = ts_t;
}
