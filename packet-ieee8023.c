/* packet-ieee8023.c
 * Routine for dissecting 802.3 (as opposed to D/I/X Ethernet) packets.
 *
 * $Id: packet-ieee8023.c,v 1.9 2004/02/21 05:12:45 guy Exp $
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
#include "packet-ieee8023.h"
#include "packet-eth.h"
#include "packet-frame.h"

static dissector_handle_t ipx_handle;
static dissector_handle_t llc_handle;

void
dissect_802_3(int length, gboolean is_802_2, tvbuff_t *tvb,
	      int offset_after_length, packet_info *pinfo, proto_tree *tree,
	      proto_tree *fh_tree, int length_id, int trailer_id,
	      int fcs_len)
{
  tvbuff_t		*volatile next_tvb;
  tvbuff_t		*volatile trailer_tvb;
  const char		*saved_proto;

  if (fh_tree)
    proto_tree_add_uint(fh_tree, length_id, tvb, offset_after_length - 2, 2,
			length);

  /* Give the next dissector only 'length' number of bytes */
  TRY {
    next_tvb = tvb_new_subset(tvb, offset_after_length, length, length);
    trailer_tvb = tvb_new_subset(tvb, offset_after_length + length, -1, -1);
  }
  CATCH2(BoundsError, ReportedBoundsError) {
    /* Either:

	  the packet doesn't have "length" bytes worth of
	  captured data left in it - or it may not even have
	  "length" bytes worth of data in it, period -
	  so the "tvb_new_subset()" creating "next_tvb"
	  threw an exception

       or

	  the packet has exactly "length" bytes worth of
	  captured data left in it, so the "tvb_new_subset()"
	  creating "trailer_tvb" threw an exception.

       In either case, this means that all the data in the frame
       is within the length value, so we give all the data to the
       next protocol and have no trailer. */
    next_tvb = tvb_new_subset(tvb, offset_after_length, -1, length);
    trailer_tvb = NULL;
  }
  ENDTRY;

  /* Dissect the payload either as IPX or as an LLC frame.
     Catch BoundsError and ReportedBoundsError, so that if the
     reported length of "next_tvb" was reduced by some dissector
     before an exception was thrown, we can still put in an item
     for the trailer. */
  saved_proto = pinfo->current_proto;
  TRY {
    if (is_802_2)
      call_dissector(llc_handle, next_tvb, pinfo, tree);
    else
      call_dissector(ipx_handle, next_tvb, pinfo, tree);
  }
  CATCH(BoundsError) {
   /* Somebody threw BoundsError, which means that dissecting the payload
      found that the packet was cut off by a snapshot length before the
      end of the payload.  The trailer comes after the payload, so *all*
      of the trailer is cut off - don't bother adding the trailer, just
      rethrow the exception so it gets reported. */
   RETHROW;
  }
  CATCH_ALL {
    /* Well, somebody threw an exception other than BoundsError.
       Show the exception, and then drive on to show the trailer,
       restoring the protocol value that was in effect before we
       called the subdissector. */
    show_exception(next_tvb, pinfo, tree, EXCEPT_CODE);
    pinfo->current_proto = saved_proto;
  }
  ENDTRY;

  add_ethernet_trailer(fh_tree, trailer_id, tvb, trailer_tvb, fcs_len);
}

void
proto_reg_handoff_ieee802_3(void)
{
  /*
   * Get handles for the IPX and LLC dissectors.
   */
  ipx_handle = find_dissector("ipx");
  llc_handle = find_dissector("llc");
}
