/* packet-cms.c
 * Routines for RFC2630 Cryptographic Message Syntax packet dissection
 *
 * $Id: packet-cms-template.c,v 1.2 2004/05/25 21:07:43 guy Exp $
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
#include "packet-cms.h"
#include "packet-x509af.h"

#define PNAME  "Cryptographic Message Syntax"
#define PSNAME "CMS"
#define PFNAME "cms"

/* Initialize the protocol and registered fields */
int proto_cms = -1;
#include "packet-cms-hf.c"

/* Initialize the subtree pointers */
#include "packet-cms-ett.c"

#include "packet-cms-fn.c"


/*--- proto_register_cms ----------------------------------------------*/
void proto_register_cms(void) {

  /* List of fields */
  static hf_register_info hf[] = {
#include "packet-cms-hfarr.c"
  };

  /* List of subtrees */
  static gint *ett[] = {
#include "packet-cms-ettarr.c"
  };

  /* Register protocol */
  proto_cms = proto_register_protocol(PNAME, PSNAME, PFNAME);

  /* Register fields and subtrees */
  proto_register_field_array(proto_cms, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));

}


/*--- proto_reg_handoff_cms -------------------------------------------*/
void proto_reg_handoff_cms(void) {
}

