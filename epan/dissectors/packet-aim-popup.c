/* packet-aim.c
 * Routines for AIM Instant Messenger (OSCAR) dissection
 * Copyright 2004, Jelmer Vernooij <jelmer@samba.org>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <glib.h>

#include <epan/packet.h>
#include <epan/strutil.h>

#include "packet-aim.h"

/* SNAC families */
#define FAMILY_POPUP      0x0008

/* Family Popup */
#define FAMILY_POPUP_ERROR            0x0001
#define FAMILY_POPUP_COMMAND          0x0002
#define FAMILY_POPUP_DEFAULT          0xffff

static const value_string aim_fnac_family_popup[] = {
  { FAMILY_POPUP_ERROR, "Error" },
  { FAMILY_POPUP_COMMAND, "Display Popup Message Server Command" },
  { FAMILY_POPUP_DEFAULT, "Popup Default" },
  { 0, NULL }
};

#define AIM_POPUP_TLV_MESSAGE_TEXT		0x001
#define AIM_POPUP_TLV_URL_STRING		0x002
#define AIM_POPUP_TLV_WINDOW_WIDTH		0x003
#define AIM_POPUP_TLV_WINDOW_HEIGHT		0x004
#define AIM_POPUP_TLV_AUTOHIDE_DELAY	0x005

const aim_tlv popup_tlvs[] = {
	{ AIM_POPUP_TLV_MESSAGE_TEXT, "Message text (html)", dissect_aim_tlv_value_string },
	{ AIM_POPUP_TLV_URL_STRING, "URL string", dissect_aim_tlv_value_string },
	{ AIM_POPUP_TLV_WINDOW_WIDTH, "Window Width (pixels)", dissect_aim_tlv_value_uint16 },
	{ AIM_POPUP_TLV_WINDOW_HEIGHT, "Window Height (pixels)", dissect_aim_tlv_value_uint16 },
	{ AIM_POPUP_TLV_AUTOHIDE_DELAY, "Autohide delay (seconds)", dissect_aim_tlv_value_uint16 },
	{ 0, NULL, NULL }
};

/* Initialize the protocol and registered fields */
static int proto_aim_popup = -1;

/* Initialize the subtree pointers */
static gint ett_aim_popup    = -1;

static int dissect_aim_snac_popup(tvbuff_t *tvb, packet_info *pinfo, 
								  proto_tree *tree) 
{
    int offset = 0;
    struct aiminfo *aiminfo = pinfo->private_data;
    proto_item *ti = NULL;
    proto_tree *popup_tree = NULL;
                                                                                
    if(tree) {
        ti = proto_tree_add_text(tree, tvb, 0, -1,"AIM Popup Service");
        popup_tree = proto_item_add_subtree(ti, ett_aim_popup);
    }
                                                             
	switch(aiminfo->subtype) {
	case FAMILY_POPUP_ERROR:
      return dissect_aim_snac_error(tvb, pinfo, 0, popup_tree);
	case FAMILY_POPUP_COMMAND:
	  return dissect_aim_tlv(tvb, pinfo, offset, popup_tree, popup_tlvs);
	}

	return 0;
}

/* Register the protocol with Ethereal */
void
proto_register_aim_popup(void)
{

/* Setup protocol subtree array */
  static gint *ett[] = {
    &ett_aim_popup,
  };

/* Register the protocol name and description */
  proto_aim_popup = proto_register_protocol("AIM Popup", "AIM Popup", "aim_popup");

/* Required function calls to register the header fields and subtrees used */
  proto_register_subtree_array(ett, array_length(ett));
}

void
proto_reg_handoff_aim_popup(void)
{
  dissector_handle_t aim_handle;
  aim_handle = new_create_dissector_handle(dissect_aim_snac_popup, proto_aim_popup);
  dissector_add("aim.family", FAMILY_POPUP, aim_handle);
  aim_init_family(FAMILY_POPUP, "Popup", aim_fnac_family_popup);
}
