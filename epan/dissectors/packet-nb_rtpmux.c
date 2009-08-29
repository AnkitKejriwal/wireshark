/* packet-nb_rtpmux.c
 * Routines for 3GPP RTP Multiplex dissection, 3GPP TS 29.414
 * Copyright 2009, ip.access ltd <amp@ipaccess.com>
 *
 * $Id$
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glib.h>

#include <epan/packet.h>

/* Initialize the protocol and registered fields */
static int proto_nb_rtpmux = -1;
static int hf_nb_rtpmux_compressed = -1;
static int hf_nb_rtpmux_dstport    = -1;
static int hf_nb_rtpmux_length     = -1;
static int hf_nb_rtpmux_srcport    = -1;
static int hf_nb_rtpmux_data       = -1;

/* Initialize the subtree pointers */
static gint ett_nb_rtpmux = -1;

static dissector_handle_t rtpdissector;

/* Code to actually dissect the packets */
static int
dissect_nb_rtpmux(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
    /* Set up structures needed to add the protocol subtree and manage it */
    proto_item *ti;
    proto_tree *nb_rtpmux_tree;
    unsigned int offset = 0;

    /*  First, if at all possible, do some heuristics to check if the packet cannot
     *  possibly belong to your protocol.  This is especially important for
     *  protocols directly on top of TCP or UDP where port collisions are
     *  common place (e.g., even though your protocol uses a well known port,
     *  someone else may set up, for example, a web server on that port which,
     *  if someone analyzed that web server's traffic in Wireshark, would result
     *  in Wireshark handing an HTTP packet to your dissector).  For example:
     */

    /*
     * XXX - this is *FAR* too weak a heuristic; it could cause all sorts
     * of stuff to be incorrectly identified as Nb_RTPmux.  Either this
     * needs a stronger heuristic, or it needs to have a preference to
     * set the port on which to dissect it, or it needs to be a non-heuristic
     * dissector and *require* that a user use "Decode As..." to decode
     * traffic as Nb_RTPmux.
     *
     * Look for a payload that looks like an RTP packet, using the
     * same (weakish) heuristics as RTP uses?
     */

    /* Check that there's enough data */
    if (tvb_length(tvb) < 6)
        return 0;

    /* Make entries in Protocol column and Info column on summary display */
    col_set_str(pinfo->cinfo, COL_PROTOCOL, "Nb_RTPmux");

    /* NOTE: The offset and length values in the call to
       "proto_tree_add_item()" define what data bytes to highlight in the hex
       display window when the line in the protocol tree display
       corresponding to that item is selected.

       Supplying a length of -1 is the way to highlight all data from the
       offset to the end of the packet. */

    /* create display subtree for the protocol */
    while (offset < tvb_reported_length(tvb)-5)
    {
        guint16 dstport, srcport;
        unsigned int length;
        gint captured_length;
        tvbuff_t* next_tvb;

        length = tvb_get_guint8(tvb, offset+2);
        ti = proto_tree_add_item(tree, proto_nb_rtpmux, tvb, offset,
            length+5, FALSE);
        nb_rtpmux_tree = proto_item_add_subtree(ti, ett_nb_rtpmux);

        /* XXX - what if the T bit is set? */
        proto_tree_add_item(nb_rtpmux_tree,
            hf_nb_rtpmux_compressed, tvb, offset, 1, FALSE);
        dstport = (tvb_get_ntohs(tvb, offset) & 0x7fff) << 1;
        proto_tree_add_uint(nb_rtpmux_tree, hf_nb_rtpmux_dstport, tvb, offset, 2, dstport );
        proto_tree_add_item(nb_rtpmux_tree,
            hf_nb_rtpmux_length, tvb, offset+2, 1, FALSE);
        srcport = (tvb_get_ntohs(tvb, offset+3) & 0x7fff) << 1;
        proto_tree_add_uint(nb_rtpmux_tree, hf_nb_rtpmux_srcport, tvb, offset+3, 2, srcport );

        if (length != 0)
        {
            /* We have an RTP payload. */
            if (rtpdissector)
            {
                captured_length = tvb_length_remaining(tvb, offset + 5);
                if (captured_length > (gint)length)
                    captured_length = length;
                next_tvb = tvb_new_subset(tvb, offset+5, captured_length,
                                          length);

                call_dissector(rtpdissector, next_tvb, pinfo, nb_rtpmux_tree);
            }
            else
            {
                proto_tree_add_item(nb_rtpmux_tree,
                    hf_nb_rtpmux_data, tvb, offset+5, length, FALSE);
            }
        }
        offset += 5+length;
    }

    /* Return the amount of data this dissector was able to dissect */
    return tvb_length(tvb);
}


/* Register the protocol with Wireshark */

/* this format is require because a script is used to build the C function
   that calls all the protocol registration.
*/
void
proto_register_nb_rtpmux(void)
{

    static hf_register_info hf[] = {
        { &hf_nb_rtpmux_compressed,
            { "Compressed headers", "nb_rtpmux.compressed",
             FT_BOOLEAN, BASE_NONE, NULL, 0x80,
            NULL, HFILL }
        },
        { &hf_nb_rtpmux_dstport,
            { "Dst port", "nb_rtpmux.dstport",
             FT_UINT16, BASE_DEC, NULL, 0x0000,
            NULL, HFILL }
        },
        { &hf_nb_rtpmux_length,
            { "Length", "nb_rtpmux.length",
             FT_UINT8, BASE_DEC, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_nb_rtpmux_srcport,
            { "Src port", "nb_rtpmux.srcport",
             FT_UINT16, BASE_DEC, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_nb_rtpmux_data,
            { "RTP Packet", "nb_rtpmux.data",
             FT_BYTES, BASE_HEX, NULL, 0x00,
            NULL, HFILL }
        }
    };

    /* Setup protocol subtree array */
    static gint *ett[] = {
        &ett_nb_rtpmux
    };

    /* Register the protocol name and description */
    proto_nb_rtpmux = proto_register_protocol("3GPP Nb Interface RTP Multiplex",
        "nb_rtpmux", "nb_rtpmux");

    /* Required function calls to register the header fields and subtrees used */
    proto_register_field_array(proto_nb_rtpmux, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));

}


/* If this dissector uses sub-dissector registration add a registration routine.
   This exact format is required because a script is used to find these
   routines and create the code that calls these routines.

   This function is also called by preferences whenever "Apply" is pressed
   (see prefs_register_protocol above) so it should accommodate being called
   more than once.
*/
void
proto_reg_handoff_nb_rtpmux(void)
{
    dissector_handle_t nb_rtpmux_handle;

    /*  Use new_create_dissector_handle() to indicate that dissect_nb_rtpmux()
     *  returns the number of bytes it dissected (or 0 if it thinks the packet
     *  does not belong to PROTONAME).
     */
    nb_rtpmux_handle = new_create_dissector_handle(dissect_nb_rtpmux,
                                                   proto_nb_rtpmux);

    dissector_add_handle("udp.port", nb_rtpmux_handle);
    rtpdissector = find_dissector("rtp");
}

