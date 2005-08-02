/* DO NOT EDIT
 * This dissector is autogenerated
 */

/* packet-dcerpc-efs.c
 * Routines for EFS packet disassembly
 *   ronnie sahlberg 2005
 * Autogenerated based on the IDL definitions by
 *   Jean-Baptiste Marchand
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include <string.h>

#include <epan/packet.h>
#include "packet-dcerpc.h"
#include "packet-dcerpc-nt.h"
#include "packet-windows-common.h"
#include "packet-dcerpc-efs.h"

static int proto_efs = -1;
ETH_HF

ETH_ETT

ETH_CODE

void
proto_register_efs(void)
{
        static hf_register_info hf[] = {
ETH_HFARR
	};

        static gint *ett[] = {
ETH_ETTARR
        };

        proto_efs = proto_register_protocol(
                "Microsoft Encrypted File System Service", 
		"EFS", "efs");
	proto_register_field_array(proto_efs, hf, array_length(hf));
        proto_register_subtree_array(ett, array_length(ett));
}

static dcerpc_sub_dissector function_dissectors[] = {
ETH_FT
	{ 0, NULL, NULL, NULL },
};

void
proto_reg_handoff_efs(void)
{
ETH_HANDOFF
}

