/* globals.h
 * Global defines, etc.
 *
 * $Id: globals.h,v 1.34 2004/06/01 17:37:10 ulfl Exp $
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

#ifndef __GLOBALS_H__
#define __GLOBALS_H__

#include "file.h"
#include <epan/timestamp.h>

extern capture_file cfile;
extern gchar       *ethereal_path;
#ifdef HAVE_LIBPCAP
/** @todo move this to the gtk dir */
extern gboolean     auto_scroll_live;
#endif

#endif
