/* packet-fddi.c
 * Routines for FDDI packet disassembly
 *
 * Laurent Deniel <deniel@worldnet.fr>
 *
 * $Id: packet-fddi.c,v 1.10 1999/03/02 20:50:05 gram Exp $
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@zing.org>
 * Copyright 1998 Gerald Combs
 *
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

#include <gtk/gtk.h>

#include <stdio.h>


#include "ethereal.h"
#include "packet.h"
#include "resolv.h"

/* FDDI Frame Control values */

#define FDDI_FC_VOID		0x00		/* Void frame */
#define FDDI_FC_NRT		0x80		/* Nonrestricted token */
#define FDDI_FC_RT		0xc0		/* Restricted token */
#define FDDI_FC_MAC		0xc0		/* MAC frame */
#define FDDI_FC_SMT		0x40		/* SMT frame */
#define FDDI_FC_SMT_INFO	0x41		/* SMT Info */
#define FDDI_FC_SMT_NSA		0x4F		/* SMT Next station adrs */
#define FDDI_FC_SMT_MIN		FDDI_FC_SMT_INFO
#define FDDI_FC_SMT_MAX		FDDI_FC_SMT_NSA
#define FDDI_FC_MAC_MIN		0xc1
#define FDDI_FC_MAC_BEACON	0xc2		/* MAC Beacon frame */
#define FDDI_FC_MAC_CLAIM	0xc3		/* MAC Claim frame */
#define FDDI_FC_MAC_MAX		0xcf
#define FDDI_FC_LLC_ASYNC	0x50		/* Async. LLC frame */
#define FDDI_FC_LLC_ASYNC_MIN	FDDI_FC_LLC_ASYNC
#define FDDI_FC_LLC_ASYNC_DEF	0x54
#define FDDI_FC_LLC_ASYNC_MAX	0x5f
#define FDDI_FC_LLC_SYNC	0xd0		/* Sync. LLC frame */
#define FDDI_FC_LLC_SYNC_MIN	FDDI_FC_LLC_SYNC
#define FDDI_FC_LLC_SYNC_MAX	0xd7
#define FDDI_FC_IMP_ASYNC	0x60		/* Implementor Async. */
#define FDDI_FC_IMP_ASYNC_MIN	FDDI_FC_IMP_ASYNC
#define FDDI_FC_IMP_ASYNC_MAX	0x6f
#define FDDI_FC_IMP_SYNC	0xe0		/* Implementor Synch. */

#define FDDI_HEADER_SIZE	13

/* field positions */

#define FDDI_P_FC		0
#define FDDI_P_DHOST		1
#define FDDI_P_SHOST		7

/* On some systems, the FDDI MAC addresses are bit-swapped. */
#if !defined(ultrix) && !defined(__alpha) && !defined(__bsdi)
#define BIT_SWAPPED_MAC_ADDRS
#endif

#ifdef BIT_SWAPPED_MAC_ADDRS
/* "swaptab[i]" is the value of "i" with the bits reversed. */
static u_char swaptab[256] = {
  0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
  0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
  0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
  0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
  0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
  0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
  0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
  0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
  0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
  0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
  0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
  0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
  0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
  0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
  0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
  0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
  0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
  0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
  0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
  0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
  0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
  0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
  0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
  0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
  0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
  0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
  0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
  0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
  0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
  0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
  0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
  0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
};
#endif

static void get_mac_addr(u_char *swapped_addr, const u_char *addr)
{
  int i;

  for (i = 0; i < 6; i++) {
#ifdef BIT_SWAPPED_MAC_ADDRS
    swapped_addr[i] = swaptab[addr[i]];
#else
    swapped_addr[i] = addr[i];
#endif
  }
}

void
capture_fddi(const u_char *pd, guint32 cap_len, packet_counts *ld) {
  int        offset = 0, fc;

  if (cap_len < FDDI_HEADER_SIZE) {
    ld->other++;
    return;
  }
  offset = FDDI_HEADER_SIZE;

  fc = (int) pd[FDDI_P_FC];

  switch (fc) {

    /* From now, only 802.2 SNAP (Async. LCC frame) is supported */

    case FDDI_FC_LLC_ASYNC + 0  :
    case FDDI_FC_LLC_ASYNC + 1  :
    case FDDI_FC_LLC_ASYNC + 2  :
    case FDDI_FC_LLC_ASYNC + 3  :
    case FDDI_FC_LLC_ASYNC + 4  :
    case FDDI_FC_LLC_ASYNC + 5  :
    case FDDI_FC_LLC_ASYNC + 6  :
    case FDDI_FC_LLC_ASYNC + 7  :
    case FDDI_FC_LLC_ASYNC + 8  :
    case FDDI_FC_LLC_ASYNC + 9  :
    case FDDI_FC_LLC_ASYNC + 10 :
    case FDDI_FC_LLC_ASYNC + 11 :
    case FDDI_FC_LLC_ASYNC + 12 :
    case FDDI_FC_LLC_ASYNC + 13 :
    case FDDI_FC_LLC_ASYNC + 14 :
    case FDDI_FC_LLC_ASYNC + 15 :
      capture_llc(pd, offset, cap_len, ld);
      return;
      
    default :
      ld->other++;
      return;

  } /* fc */

} /* capture_fddi */

void dissect_fddi(const u_char *pd, frame_data *fd, GtkTree *tree) 
{
  int        offset = 0, fc;
  GtkWidget *fh_tree, *ti;
  u_char     src[6], dst[6];

  if (fd->cap_len < FDDI_HEADER_SIZE) {
    dissect_data(pd, offset, fd, tree);
    return;
  }

  /* Extract the source and destination addresses, possibly bit-swapping
     them. */
  get_mac_addr(dst, (u_char *)&pd[FDDI_P_DHOST]);
  get_mac_addr(src, (u_char *)&pd[FDDI_P_SHOST]);

  fc = (int) pd[FDDI_P_FC];

  if (check_col(fd, COL_RES_DL_SRC))
    col_add_str(fd, COL_RES_DL_SRC, get_ether_name(src));
  if (check_col(fd, COL_RES_DL_DST))
    col_add_str(fd, COL_RES_DL_DST, get_ether_name(dst));
  if (check_col(fd, COL_UNRES_DL_SRC))
    col_add_str(fd, COL_UNRES_DL_SRC, ether_to_str(src));
  if (check_col(fd, COL_UNRES_DL_DST))
    col_add_str(fd, COL_UNRES_DL_DST, ether_to_str(dst));
  if (check_col(fd, COL_PROTOCOL))
    col_add_str(fd, COL_PROTOCOL, "N/A");
  if (check_col(fd, COL_INFO))
    col_add_str(fd, COL_INFO, "FDDI");

  offset = FDDI_HEADER_SIZE;

  if (tree) {
    ti = add_item_to_tree(GTK_WIDGET(tree), 0, offset,
			  "FDDI %s",
			  (fc >= FDDI_FC_LLC_ASYNC_MIN && fc <= FDDI_FC_LLC_ASYNC_MAX) ?
			  "Async LLC" : "unsupported FC");

      fh_tree = gtk_tree_new();
      add_subtree(ti, fh_tree, ETT_FDDI);
      add_item_to_tree(fh_tree, FDDI_P_FC, 1, "Frame Control: 0x%02x", fc);
      add_item_to_tree(fh_tree, FDDI_P_DHOST, 6, "Destination: %s (%s)",
		       ether_to_str(dst), get_ether_name(dst));
      add_item_to_tree(fh_tree, FDDI_P_SHOST, 6, "Source: %s (%s)",
		       ether_to_str(src), get_ether_name(src));
    }

  switch (fc) {

    /* From now, only 802.2 SNAP (Async. LCC frame) is supported */

    case FDDI_FC_LLC_ASYNC + 0  :
    case FDDI_FC_LLC_ASYNC + 1  :
    case FDDI_FC_LLC_ASYNC + 2  :
    case FDDI_FC_LLC_ASYNC + 3  :
    case FDDI_FC_LLC_ASYNC + 4  :
    case FDDI_FC_LLC_ASYNC + 5  :
    case FDDI_FC_LLC_ASYNC + 6  :
    case FDDI_FC_LLC_ASYNC + 7  :
    case FDDI_FC_LLC_ASYNC + 8  :
    case FDDI_FC_LLC_ASYNC + 9  :
    case FDDI_FC_LLC_ASYNC + 10 :
    case FDDI_FC_LLC_ASYNC + 11 :
    case FDDI_FC_LLC_ASYNC + 12 :
    case FDDI_FC_LLC_ASYNC + 13 :
    case FDDI_FC_LLC_ASYNC + 14 :
    case FDDI_FC_LLC_ASYNC + 15 :
      dissect_llc(pd, offset, fd, tree);
      return;
      
    default :
      dissect_data(pd, offset, fd, tree);
      return;

  } /* fc */

} /* dissect_fddi */
