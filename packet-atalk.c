/* packet-atalk.c
 * Routines for Appletalk packet disassembly (DDP, currently).
 *
 * $Id: packet-atalk.c,v 1.26 1999/12/08 23:55:01 nneul Exp $
 *
 * Simon Wilkinson <sxw@dcs.ed.ac.uk>
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

#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif

#include <glib.h>
#include "packet.h"
#include "packet-atalk.h"

static int proto_ddp = -1;
static int hf_ddp_hopcount = -1;
static int hf_ddp_len = -1;
static int hf_ddp_checksum = -1;
static int hf_ddp_dst_net = -1;
static int hf_ddp_src_net = -1;
static int hf_ddp_dst_node = -1;
static int hf_ddp_src_node = -1;
static int hf_ddp_dst_socket = -1;
static int hf_ddp_src_socket = -1;
static int hf_ddp_type = -1;

static int proto_nbp = -1;
static int hf_nbp_op = -1;
static int hf_nbp_info = -1;
static int hf_nbp_count = -1;
static int hf_nbp_tid = -1;

static gint ett_nbp = -1;
static gint ett_nbp_info = -1;
static gint ett_ddp = -1;

/* P = Padding, H = Hops, L = Len */
#if BYTE_ORDER == BIG_ENDIAN
 /* PPHHHHLL LLLLLLLL */
# define ddp_hops(x)	( ( x >> 10) & 0x3C )
# define ddp_len(x)		( x & 0x03ff )
#else
 /* LLLLLLLL PPHHHHLL*/
# define ddp_hops(x)	( x & 0x3C )
# define ddp_len(x)		( ntohs(x) & 0x03ff )
#endif
typedef struct _e_ddp {
  guint16	hops_len; /* combines pad, hops, and len */
  guint16	sum,dnet,snet;
  guint8	dnode,snode;
  guint8	dport,sport;
  guint8	type;
} e_ddp;

#define DDP_RTMPDATA	0x01
#define DDP_NBP		0x02
#define DDP_ATP		0x03
#define DDP_AEP		0x04
#define DDP_RTMPREQ	0x05
#define DDP_ZIP		0x06
#define DDP_ADSP	0x07
#define DDP_HEADER_SIZE 13

#define NBP_LOOKUP 2

gchar *
atalk_addr_to_str(const struct atalk_ddp_addr *addrp)
{
  static gchar	str[3][14];
  static gchar	*cur;

  if (cur == &str[0][0]) {
    cur = &str[1][0];
  } else if (cur == &str[1][0]) {
    cur = &str[2][0];
  } else {
    cur = &str[0][0];
  }

  sprintf(cur, "%u.%u:%u", addrp->net, addrp->node, addrp->port);
  return cur;
}

static const value_string op_vals[] = {
  {DDP_RTMPDATA, "AppleTalk Routing Table response or data" },
  {DDP_NBP, "AppleTalk Name Binding Protocol packet"},
  {DDP_ATP, "AppleTalk Transaction Protocol packet"},
  {DDP_AEP, "AppleTalk Echo Protocol packet"},
  {DDP_RTMPREQ, "AppleTalk Routing Table request"},
  {DDP_ZIP, "AppleTalk Zone Information Protocol packet"},
  {DDP_ADSP, "AppleTalk Data Stream Protocol"},
  {0, NULL}
};

static const value_string nbp_op_vals[] = {
  {NBP_LOOKUP, "lookup"},
  {0, NULL}
};

static void
dissect_rtmp_request(const u_char *pd, int offset, frame_data *fd, proto_tree *tree) {
  dissect_data(pd, offset, fd, tree);
  return;
}

static void
dissect_rtmp_data(const u_char *pd, int offset, frame_data *fd, proto_tree *tree) {
  dissect_data(pd, offset, fd, tree);
  return;
}

static void
dissect_nbp(const u_char *pd, int offset, frame_data *fd, proto_tree *tree) {
  proto_tree *nbp_tree;
  proto_tree *nbp_info_tree;
  proto_item *ti, *info_item;
  guint op, count;

  if (!BYTES_ARE_IN_FRAME(offset, 2)) {
    dissect_data(pd, offset, fd, tree);
    return;
  }

  op = pd[offset] >> 4;
  count = pd[offset] & 0x0F;

  if (check_col(fd, COL_PROTOCOL))
    col_add_str(fd, COL_PROTOCOL, "NBP");

  if (check_col(fd, COL_INFO))
    col_add_fstr(fd, COL_INFO, "Op: %s  Count: %d",
      val_to_str(op, nbp_op_vals, "unknown (%1x)"), count);
  
  if (tree) {
    ti = proto_tree_add_item(tree, proto_nbp, offset, END_OF_FRAME, NULL);
    nbp_tree = proto_item_add_subtree(ti, ett_nbp);

    info_item = proto_tree_add_item_format(nbp_tree, hf_nbp_info, offset, 1,
		pd[offset], 
		"Info: 0x%01X  Operation: %s  Count: %d", pd[offset],
		val_to_str(op, nbp_op_vals, "unknown"),
		count);
	nbp_info_tree = proto_item_add_subtree(info_item, ett_nbp_info);
    proto_tree_add_item(nbp_info_tree, hf_nbp_op, offset, 1, pd[offset]);
    proto_tree_add_item(nbp_info_tree, hf_nbp_count, offset, 1, pd[offset]);
    proto_tree_add_item(nbp_tree, hf_nbp_tid, offset+1, 1, pd[offset+1]);
  }

  return;
}

void
dissect_ddp(const u_char *pd, int offset, frame_data *fd, proto_tree *tree) {
  e_ddp       ddp;
  proto_tree *ddp_tree;
  proto_item *ti;
  static struct atalk_ddp_addr src, dst;

  if (!BYTES_ARE_IN_FRAME(offset, DDP_HEADER_SIZE)) {
    dissect_data(pd, offset, fd, tree);
    return;
  }

  memcpy(&ddp, &pd[offset], sizeof(e_ddp));
  ddp.dnet=ntohs(ddp.dnet);
  ddp.snet=ntohs(ddp.snet);
  ddp.sum=ntohs(ddp.sum);
  
  src.net = ddp.snet;
  src.node = ddp.snode;
  src.port = ddp.sport;
  dst.net = ddp.dnet;
  dst.node = ddp.dnode;
  dst.port = ddp.dport;
  SET_ADDRESS(&pi.net_src, AT_ATALK, sizeof src, (guint8 *)&src);
  SET_ADDRESS(&pi.src, AT_ATALK, sizeof src, (guint8 *)&src);
  SET_ADDRESS(&pi.net_dst, AT_ATALK, sizeof dst, (guint8 *)&dst);
  SET_ADDRESS(&pi.dst, AT_ATALK, sizeof dst, (guint8 *)&dst);

  if (check_col(fd, COL_PROTOCOL))
    col_add_str(fd, COL_PROTOCOL, "DDP");
  if (check_col(fd, COL_INFO))
    col_add_str(fd, COL_INFO,
      val_to_str(ddp.type, op_vals, "Unknown DDP protocol (%02x)"));
  
  if (tree) {
    ti = proto_tree_add_item(tree, proto_ddp, offset, DDP_HEADER_SIZE, NULL);
    ddp_tree = proto_item_add_subtree(ti, ett_ddp);
    proto_tree_add_item(ddp_tree, hf_ddp_hopcount, offset,      1, 
			ddp_hops(ddp.hops_len));
    proto_tree_add_item(ddp_tree, hf_ddp_len, offset,	    2, 
			ddp_len(ddp.hops_len));
    proto_tree_add_item(ddp_tree, hf_ddp_checksum, offset + 2,  2, ddp.sum);
    proto_tree_add_item(ddp_tree, hf_ddp_dst_net, offset + 4,  2, ddp.dnet);
    proto_tree_add_item(ddp_tree, hf_ddp_src_net,  offset + 6,  2, ddp.snet);
    proto_tree_add_item(ddp_tree, hf_ddp_dst_node, offset + 8,  1, ddp.dnode);
    proto_tree_add_item(ddp_tree, hf_ddp_src_node, offset + 9,  1, ddp.snode);
    proto_tree_add_item(ddp_tree, hf_ddp_dst_socket, offset + 10, 1, ddp.dport);
    proto_tree_add_item(ddp_tree, hf_ddp_src_socket, offset + 11, 1, ddp.sport);
    proto_tree_add_item(ddp_tree, hf_ddp_type, offset + 12, 1, ddp.type);  
  }

  offset += DDP_HEADER_SIZE;

  switch ( ddp.type ) {
    case DDP_NBP:
      dissect_nbp(pd, offset, fd, tree);
      break;
    case DDP_RTMPREQ:
      dissect_rtmp_request(pd, offset, fd, tree);
      break;
    case DDP_RTMPDATA:
      dissect_rtmp_data(pd, offset, fd, tree);
      break;
    default:
      dissect_data(pd, offset, fd, tree);
      break;
  }
}

void
proto_register_atalk(void)
{
  static hf_register_info hf_ddp[] = {
    { &hf_ddp_hopcount,
      { "Hop count",		"ddp.hopcount",	FT_UINT8,  BASE_DEC, NULL, 0x0,
      	"" }},

    { &hf_ddp_len,
      { "Datagram length",	"ddp.len",	FT_UINT16, BASE_DEC, NULL, 0x0,
      	"" }},

    { &hf_ddp_checksum,
      { "Checksum",		"ddp.checksum",	FT_UINT16, BASE_DEC, NULL, 0x0,
      	"" }},

    { &hf_ddp_dst_net,
      { "Destination Net",	"ddp.dst.net",	FT_UINT16, BASE_DEC, NULL, 0x0,
      	"" }},

    { &hf_ddp_src_net,
      { "Source Net",		"ddp.src.net",	FT_UINT16, BASE_DEC, NULL, 0x0,
      	"" }},

    { &hf_ddp_dst_node,
      { "Destination Node",	"ddp.dst.node",	FT_UINT8,  BASE_DEC, NULL, 0x0,
      	"" }},

    { &hf_ddp_src_node,
      { "Source Node",		"ddp.src.node",	FT_UINT8,  BASE_DEC, NULL, 0x0,
      	"" }},

    { &hf_ddp_dst_socket,
      { "Destination Socket",	"ddp.dst.socket", FT_UINT8,  BASE_DEC, NULL, 0x0,
      	"" }},

    { &hf_ddp_src_socket,
      { "Source Socket",       	"ddp.src.socket", FT_UINT8,  BASE_DEC, NULL, 0x0,
      	"" }},

    { &hf_ddp_type,
      { "Protocol type",       	"ddp.type",	FT_UINT8,  BASE_DEC, VALS(op_vals), 0x0,
      	"" }},
  };

  static hf_register_info hf_nbp[] = {
    { &hf_nbp_op,
      { "Operation",		"nbp.op",	FT_UINT8,  BASE_DEC, 
		VALS(nbp_op_vals), 0xF0, "Operation" }},
    { &hf_nbp_info,
      { "Info",		"nbp.info",	FT_UINT8,  BASE_HEX, 
		NULL, 0x0, "Info" }},
    { &hf_nbp_count,
      { "Count",		"nbp.count",	FT_UINT8,  BASE_DEC, 
		NULL, 0x0F, "Count" }},
    { &hf_nbp_tid,
      { "Transaction ID",		"nbp.tid",	FT_UINT8,  BASE_DEC, 
		NULL, 0x0, "Transaction ID" }}
  };

  static gint *ett[] = {
    &ett_ddp,
	&ett_nbp,
	&ett_nbp_info
  };

  proto_ddp = proto_register_protocol("Datagram Delivery Protocol", "ddp");
  proto_register_field_array(proto_ddp, hf_ddp, array_length(hf_ddp));

  proto_nbp = proto_register_protocol("Name Binding Protocol", "nbp");
  proto_register_field_array(proto_nbp, hf_nbp, array_length(hf_nbp));

  proto_register_subtree_array(ett, array_length(ett));
}
