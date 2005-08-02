/* DO NOT EDIT
 * This dissector is autogenerated
 */

/* packet-dcerpc-atsvc.c
 * Routines for ATSVC packet disassembly
 * based on the original dissector that was
 *      * Copyright 2003 Jean-Baptiste Marchand <jbm@hsc.fr>
 * and IDL file from samba 4
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
#include "packet-dcerpc-atsvc.h"

static int proto_atsvc = -1;
ETH_HF

ETH_ETT

int
atsvc_dissect_JobTime(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep, int hf_index, guint32 param _U_)
{
	guint32 job_time;
	guint8 job_hour, job_min, job_sec;
	guint16 job_msec;

	offset = dissect_ndr_uint32(tvb, offset, pinfo, NULL, drep,
			-1, &job_time);

	job_hour = job_time / 3600000;
	job_min = (job_time - job_hour * 3600000) / 60000;
	job_sec = (job_time - (job_hour * 3600000) - (job_min * 60000)) / 1000;
	job_msec = (job_time - (job_hour * 3600000) - (job_min * 60000) - (job_sec * 1000));

	proto_tree_add_uint_format(tree, hf_index, tvb, offset - 4,
			4, job_time, "Time: %02d:%02d:%02d:%03d", job_hour, job_min, job_sec, job_msec);

	return offset;
}

ETH_CODE

void
proto_register_atsvc(void)
{
        static hf_register_info hf[] = {
ETH_HFARR
	};

        static gint *ett[] = {
ETH_ETTARR
        };

        proto_atsvc = proto_register_protocol(
                "Microsoft Task Scheduler Service", 
		"ATSVC", "atsvc");
	proto_register_field_array(proto_atsvc, hf, array_length(hf));
        proto_register_subtree_array(ett, array_length(ett));
}

static dcerpc_sub_dissector function_dissectors[] = {
ETH_FT
	{ 0, NULL, NULL, NULL },
};

void
proto_reg_handoff_atsvc(void)
{
ETH_HANDOFF
}

