/* packet-ftam_asn1.c
 * Routine to dissect OSI ISO 8571 FTAM Protocol packets
 * based on the ASN.1 specification from http://www.itu.int/ITU-T/asn1/database/iso/8571-4/1988/
 *
 * also based on original handwritten dissector by
 * Yuriy Sidelnikov <YSidelnikov@hotmail.com>
 *
 * Anders Broman and Ronnie Sahlberg 2005
 *
 * $Id: packet-ftam-template.c 13967 2005-03-29 14:56:09Z gerald $
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

#include <glib.h>
#include <epan/packet.h>
#include <epan/conversation.h>

#include <stdio.h>
#include <string.h>

#include "packet-ber.h"
#include "packet-acse.h"
#include "packet-ftam.h"

#define PNAME  "FTAM"
#define PSNAME "FTAM"
#define PFNAME "ftam"

/* Initialize the protocol and registered fields */
int proto_ftam = -1;

static char object_identifier_id[MAX_OID_STR_LEN];

static struct SESSION_DATA_STRUCTURE* session = NULL;

#include "packet-ftam-hf.c"

/* Initialize the subtree pointers */
static gint ett_ftam = -1;
#include "packet-ftam-ett.c"

#include "packet-ftam-fn.c"

/*
* Dissect FTAM PDUs inside a PPDU.
*/
static void
dissect_ftam(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree)
{
	int offset = 0;
	int old_offset;
	proto_item *item=NULL;
	proto_tree *tree=NULL;

	if(parent_tree){
		item = proto_tree_add_item(parent_tree, proto_ftam, tvb, 0, -1, FALSE);
		tree = proto_item_add_subtree(item, ett_ftam);
	}
	if (check_col(pinfo->cinfo, COL_PROTOCOL))
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "FTAM");
  	if (check_col(pinfo->cinfo, COL_INFO))
  		col_clear(pinfo->cinfo, COL_INFO);

	while (tvb_reported_length_remaining(tvb, offset) > 0){
		old_offset=offset;
		offset=dissect_ftam_PDU(FALSE, tvb, offset, pinfo , tree, -1);
		if(offset == old_offset){
			proto_tree_add_text(tree, tvb, offset, -1,"Internal error, zero-byte FTAM PDU");
			offset = tvb_length(tvb);
			break;
		}
	}
}


/*--- proto_register_ftam -------------------------------------------*/
void proto_register_ftam(void) {

  /* List of fields */
  static hf_register_info hf[] =
  {
#include "packet-ftam-hfarr.c"
  };

  /* List of subtrees */
  static gint *ett[] = {
    &ett_ftam,
#include "packet-ftam-ettarr.c"
  };

  /* Register protocol */
  proto_ftam = proto_register_protocol(PNAME, PSNAME, PFNAME);
  register_dissector("ftam", dissect_ftam, proto_ftam);
  /* Register fields and subtrees */
  proto_register_field_array(proto_ftam, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));

}


/*--- proto_reg_handoff_ftam --- */
void proto_reg_handoff_ftam(void) {
	register_ber_oid_dissector("1.0.8571.1.1", dissect_ftam, proto_ftam, 
	  "ftam");

}
