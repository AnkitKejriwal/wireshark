/* packet-pcli.c
 * Routines for Packet Cable Lawful Intercept packet disassembly
 *
 * $Id: packet-pcli.c,v 1.2 2002/10/08 08:26:31 guy Exp $
 *
 * Copyright (c) 2000 by Ed Warnicke <hagbard@physics.rutgers.edu>
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@ethereal.com>
 * Copyright 1999 Gerald Combs
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

/* Include files */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "plugins/plugin_api.h"

#include "moduleinfo.h"

#include <stdio.h>
#include <stdlib.h>
#include <gmodule.h>
#include <ctype.h>
#include <time.h>
#include <string.h>
#include <epan/packet.h>
#include <epan/resolv.h>
#include "prefs.h"
#include <epan/strutil.h>

#include "plugins/plugin_api_defs.h"

/* Define version if we are not building ethereal statically */

#ifndef __ETHEREAL_STATIC__
G_MODULE_EXPORT const gchar version[] = VERSION;
#endif

/* Define udp_port for lawful intercept */

#define UDP_PORT_PCLI 9000

void proto_reg_handoff_pcli(void);

/* Define the pcli proto */

static int proto_pcli = -1;

/* Define headers for pcli */

static int hf_pcli_cccid = -1;

/* Define the tree for pcli */

static int ett_pcli = -1;

/* 
 * Here are the global variables associated with the preferences
 * for pcli
 */

static guint global_udp_port_pcli = UDP_PORT_PCLI;
static guint udp_port_pcli = UDP_PORT_PCLI;

/* A static handle for the ip dissector */
static dissector_handle_t ip_handle;

static void 
dissect_pcli(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree) {
  
  guint32 cccid;
  proto_tree *ti,*pcli_tree;
  tvbuff_t * next_tvb;

  /* Set the protocol column */
  if(check_col(pinfo->cinfo,COL_PROTOCOL)){
    col_add_str(pinfo->cinfo,COL_PROTOCOL,"PCLI");
  }

  /* Get the CCCID */
  cccid = tvb_get_ntohl(tvb,0);

  /* Set the info column */
  if(check_col(pinfo->cinfo,COL_INFO)){
    col_add_fstr(pinfo->cinfo, COL_INFO, "CCCID: %u",cccid);
  }

  /* 
   *If we have a non-null tree (ie we are building the proto_tree
   * instead of just filling out the columns ), then add a PLCI
   * tree node and put a CCCID header element under it.  Then hand
   * of to the IP dissector.
   */
  if(tree) {
    ti = proto_tree_add_item(tree,proto_pcli,tvb,0,0,FALSE);
    pcli_tree = proto_item_add_subtree(ti,ett_pcli);
    proto_tree_add_uint(pcli_tree,hf_pcli_cccid,tvb,
			0,4,cccid);
    next_tvb = tvb_new_subset(tvb,4,-1,-1);
    call_dissector(ip_handle,next_tvb,pinfo,tree);
  }
}

void 
proto_register_pcli(void) {
  static hf_register_info hf[] = {
    { &hf_pcli_cccid,
      { "CCCID", "pcli.cccid", FT_UINT32, BASE_DEC, NULL, 0x0,
	"Call Content Connection Identifier", HFILL }},
  };

  static gint *ett[] = {
    &ett_pcli,
  };

  module_t *pcli_module;

  proto_pcli = proto_register_protocol("Packet Cable Lawful Intercept",
				       "PCLI","pcli");
  proto_register_field_array(proto_pcli,hf,array_length(hf));
  proto_register_subtree_array(ett,array_length(ett));

  pcli_module = prefs_register_protocol(proto_pcli,
					proto_reg_handoff_pcli);
  prefs_register_uint_preference(pcli_module, "pcli.udp_port",
				 "PCLI UDP Port",
				 "Set the UPD port on which "
				 "Packet Cable Lawful Intercept"
				 "packets will be sent",
				 10,&global_udp_port_pcli);

}

/* The registration hand-off routing */

void
proto_reg_handoff_pcli(void) {
  static int pcli_initialized = FALSE;
  static dissector_handle_t pcli_handle;

  ip_handle = find_dissector("ip");

  if(!pcli_initialized) {
    pcli_handle = create_dissector_handle(dissect_pcli,proto_pcli);
    pcli_initialized = TRUE;
  } else {
    dissector_delete("udp.port",udp_port_pcli,pcli_handle);
  }

  udp_port_pcli = global_udp_port_pcli;
  
  dissector_add("udp.port",global_udp_port_pcli,pcli_handle);
}

/* Start the functions we need for the plugin stuff */

#ifndef __ETHEREAL_STATIC__

G_MODULE_EXPORT void
plugin_reg_handoff(void){
  proto_reg_handoff_pcli();
}

G_MODULE_EXPORT void
plugin_init(plugin_address_table_t *pat
#ifndef PLUGINS_NEED_ADDRESS_TABLE
_U_
#endif
){
  /* initialise the table of pointers needed in Win32 DLLs */
  plugin_address_table_init(pat);
  /* register the new protocol, protocol fields, and subtrees */
  if (proto_pcli == -1) { /* execute protocol initialization only once */
    proto_register_pcli();
  }
}

#endif

/* End the functions we need for plugin stuff */

