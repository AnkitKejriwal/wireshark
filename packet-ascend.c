/* packet-ascend.c
 * Routines for decoding Lucent/Ascend packet traces
 *
 * $Id: packet-ascend.c,v 1.11 2000/05/11 08:14:54 gram Exp $
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@zing.org>
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

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#include <glib.h>
#include <string.h>
#include "packet.h"
#include "packet-eth.h"
#include "packet-ppp.h"

static int proto_ascend  = -1;
static int hf_link_type  = -1;
static int hf_session_id = -1;
static int hf_called_number = -1;
static int hf_chunk      = -1;
static int hf_task       = -1;
static int hf_user_name  = -1;

static gint ett_raw = -1;

static const value_string encaps_vals[] = {
  {ASCEND_PFX_WDS_X, "PPP Transmit"},
  {ASCEND_PFX_WDS_R, "PPP Receive" },
  {ASCEND_PFX_WDD,   "Ethernet"    },
  {0,                NULL          } };

void
dissect_ascend( const u_char *pd, frame_data *fd, proto_tree *tree ) {
  proto_tree *fh_tree;
  proto_item *ti;

  /* load the top pane info. This should be overwritten by
     the next protocol in the stack */
  if(check_col(fd, COL_RES_DL_SRC))
    col_add_str(fd, COL_RES_DL_SRC, "N/A" );
  if(check_col(fd, COL_RES_DL_DST))
    col_add_str(fd, COL_RES_DL_DST, "N/A" );
  if(check_col(fd, COL_PROTOCOL))
    col_add_str(fd, COL_PROTOCOL, "N/A" );
  if(check_col(fd, COL_INFO))
    col_add_str(fd, COL_INFO, "Lucent/Ascend packet trace" );

  /* populate a tree in the second pane with the status of the link
     layer (ie none) */
  if(tree) {
    ti = proto_tree_add_text(tree, NullTVB, 0, 0, "Lucent/Ascend packet trace" );
    fh_tree = proto_item_add_subtree(ti, ett_raw);
    proto_tree_add_item(fh_tree, hf_link_type, NullTVB, 0, 0, 
			fd->pseudo_header.ascend.type);
    if (fd->pseudo_header.ascend.type == ASCEND_PFX_WDD) {
      proto_tree_add_item(fh_tree, hf_called_number, NullTVB, 0, 0, 
			  fd->pseudo_header.ascend.call_num);
      proto_tree_add_item(fh_tree, hf_chunk, NullTVB, 0, 0,
			  fd->pseudo_header.ascend.chunk);
      proto_tree_add_item_hidden(fh_tree, hf_session_id, NullTVB, 0, 0, 0);
    } else {  /* It's wandsession data */
      proto_tree_add_item(fh_tree, hf_user_name, NullTVB, 0, 0, 
			  fd->pseudo_header.ascend.user);
      proto_tree_add_item(fh_tree, hf_session_id, NullTVB, 0, 0,
			  fd->pseudo_header.ascend.sess);
      proto_tree_add_item_hidden(fh_tree, hf_chunk, NullTVB, 0, 0, 0);
    }
    proto_tree_add_item(fh_tree, hf_task, NullTVB, 0, 0, fd->pseudo_header.ascend.task);
  }

  switch (fd->pseudo_header.ascend.type) {
    case ASCEND_PFX_WDS_X:
    case ASCEND_PFX_WDS_R:
      dissect_ppp(pd, 0, fd, tree);
      break;
    case ASCEND_PFX_WDD:
      dissect_eth(pd, 0, fd, tree);
      break;
    default:
      break;
  }
}

void
proto_register_ascend(void)
{
  static hf_register_info hf[] = {
    { &hf_link_type,
    { "Link type",	"ascend.type",	FT_UINT32, BASE_DEC,	VALS(encaps_vals),	0x0,
      "" }},

    { &hf_session_id,
    { "Session ID",	"ascend.sess",	FT_UINT32, BASE_DEC,	NULL, 0x0,
      "" }},

    { &hf_called_number,
    { "Called number",	"ascend.number", FT_STRING, BASE_NONE,	NULL, 0x0,
      "" }},

    { &hf_chunk,
    { "WDD Chunk",	"ascend.chunk",	FT_UINT32, BASE_HEX,	NULL, 0x0,
      "" }},

    { &hf_task,
    { "Task",		"ascend.task",	FT_UINT32, BASE_HEX,	NULL, 0x0,
      "" }},

    { &hf_user_name,
    { "User name",     	"ascend.user",	FT_STRING, BASE_NONE,	NULL, 0x0,
      "" }},
  };
  static gint *ett[] = {
    &ett_raw,
  };

  proto_ascend = proto_register_protocol("Lucent/Ascend debug output", "ascend");
  proto_register_field_array(proto_ascend, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));
}
