/* packet-aarp.c
 * Routines for Appletalk ARP packet disassembly
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

#include <gtk/gtk.h>
#include <pcap.h>

#include <stdio.h>

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif

#include "ethereal.h"
#include "packet.h"
#include "etypes.h"

typedef struct _e_ether_aarp {
        guint16 htype, ptype;
        guint8  halen, palen;
        guint16 op;
        guint8  hsaddr[6];
        guint8  psaddr[4];
        guint8  hdaddr[6];
        guint8  pdaddr[4];
} e_ether_aarp;

#ifndef AARP_REQUEST
#define AARP_REQUEST 	0x0001
#endif
#ifndef AARP_REPLY
#define AARP_REPLY	0x0002
#endif
#ifndef AARP_PROBE	
#define AARP_PROBE	0x0003
#endif

gchar *
atalkid_to_str(guint8 *ad) {
  gint node;
  static gchar  str[3][16];
  static gchar *cur;
  
  if (cur == &str[0][0]) {
    cur = &str[1][0];
  } else if (cur == &str[1][0]) {  
    cur = &str[2][0];
  } else {  
    cur = &str[0][0];
  }
  
  node=ad[1]<<8|ad[2];
  sprintf(cur, "%d.%d",node,ad[3]);
  return cur;
}
    
void
dissect_aarp(const u_char *pd, int offset, frame_data *fd, GtkTree *tree) {
  e_ether_aarp  ea;
  GtkWidget   *aarp_tree, *ti;
  gchar       *op_str;
  value_string op_vals[] = { {AARP_REQUEST,  "AARP request" },
                             {AARP_REPLY,    "AARP reply"   },
                             {AARP_PROBE,    "AARP probe"   },
                             {0,             NULL           } };

  ea.htype = pntohs(&pd[offset]);
  ea.ptype = pntohs(&pd[offset + 2]);
  ea.halen = (guint8) pd[offset + 4];
  ea.palen = (guint8) pd[offset + 5];
  ea.op  = pletohs(&pd[offset + 6]);
  memcpy(&ea.hsaddr, &pd[offset +  8], 6);
  memcpy(&ea.psaddr, &pd[offset + 14], 4);
  memcpy(&ea.hdaddr, &pd[offset + 18], 6);
  memcpy(&ea.pdaddr, &pd[offset + 24], 4);
  
  if (fd->win_info[COL_NUM]) { strcpy(fd->win_info[COL_PROTOCOL], "AARP"); }
  
  if (tree) {
    if ((op_str = match_strval(ea.op, op_vals)))
      ti = add_item_to_tree(GTK_WIDGET(tree), offset, 28, op_str);
    else
      ti = add_item_to_tree(GTK_WIDGET(tree), offset, 28,
        "Unknown AARP (opcode 0x%04x)", ea.op);
    aarp_tree = gtk_tree_new();
    add_subtree(ti, aarp_tree, ETT_AARP);
    add_item_to_tree(aarp_tree, offset,      2,
      "Hardware type: 0x%04x", ea.htype);
    add_item_to_tree(aarp_tree, offset +  2, 2,
      "Protocol type: 0x%04x", ea.ptype);
    add_item_to_tree(aarp_tree, offset +  4, 1,
      "Hardware size: 0x%02x", ea.halen);
    add_item_to_tree(aarp_tree, offset +  5, 1,
      "Protocol size: 0x%02x", ea.palen);
    add_item_to_tree(aarp_tree, offset +  6, 2,
      "Opcode: 0x%04x (%s)", ea.op, op_str ? op_str : "Unknown");
    add_item_to_tree(aarp_tree, offset +  8, 6,
      "Sender ether: %s", ether_to_str((guint8 *) ea.hsaddr));
    add_item_to_tree(aarp_tree, offset + 14, 4,
      "Sender ID: %s", atalkid_to_str((guint8 *) ea.psaddr));
    add_item_to_tree(aarp_tree, offset + 18, 6,
      "Target ether: %s", ether_to_str((guint8 *) ea.hdaddr));
    add_item_to_tree(aarp_tree, offset + 24, 4,
      "Target ID: %s", atalkid_to_str((guint8 *) ea.pdaddr));
  }

  if (ea.ptype != ETHERTYPE_AARP && ea.ptype !=ETHERTYPE_ATALK && 
      fd->win_info[COL_NUM]) {
    sprintf(fd->win_info[COL_INFO], "h/w %d (%d) prot %d (%d) op 0x%04x",
      ea.htype, ea.halen, ea.ptype, ea.palen, ea.op);
    return;
  }
  switch (ea.op) {
    case AARP_REQUEST:
      if (fd->win_info[COL_NUM]) {
        sprintf(fd->win_info[COL_INFO], "Who has %s?  Tell %s",
          atalkid_to_str((guint8 *) ea.pdaddr), atalkid_to_str((guint8 *) ea.psaddr));
      }
      break;
    case AARP_REPLY:
      if (fd->win_info[COL_NUM]) {
        sprintf(fd->win_info[COL_INFO], "%s is at %s",
          atalkid_to_str((guint8 *) ea.psaddr),
          ether_to_str((guint8 *) ea.hsaddr));
      }
      break;
    case AARP_PROBE:
      if (fd->win_info[COL_NUM]) {
        sprintf(fd->win_info[COL_INFO], "Is there a %s",
          atalkid_to_str((guint8 *) ea.pdaddr));
      }
      break;
  }
}
