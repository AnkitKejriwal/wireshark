/* packet-nt-oui.c
 * Register an LLC dissector table for Nortel's OUI 00:00:0c
 *
 * $Id$
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

#include "config.h"

#include <epan/packet.h>
#include "packet-llc.h"
#include "oui.h"

static int hf_llc_nortel_pid = -1;

static const value_string nortel_pid_vals[] = {
	{ 0x01a1,	"SONMP flatnet hello" },
	{ 0x01a2,	"SONMP segment hello" },
	{ 0x01a3,	"SONMP bridge hello" },
	{ 0,		NULL }
};

/*
 * NOTE: there's no dissector here, just registration routines to set
 * up the dissector table for the Nortel OUI.
 */
void
proto_register_nortel_oui(void)
{
	static hf_register_info hf = {
	    &hf_llc_nortel_pid,
		{ "PID",	"llc.nortel_pid",  FT_UINT16, BASE_HEX,
		  VALS(nortel_pid_vals), 0x0, "", HFILL },
	};

	llc_add_oui(OUI_NORTEL, "llc.nortel_pid", "Nortel OUI PID", &hf);
}
