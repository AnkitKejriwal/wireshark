/*
 *  packet-wlancap.c
 *	Decode packets with a AVS-WLAN header
 *
 *  AVS linux-wlan-based products use a new sniff header to replace the 
 *  old prism2-specific one dissected in packet-prism2.c.  This one has
 *  additional fields, is designed to be non-hardware-specific, and more 
 *  importantly, version and length fields so it can be extended later 
 *  without breaking anything.
 * 
 * By Solomon Peachy
 *
 * $Id: packet-wlancap.c,v 1.1 2002/11/06 21:49:29 guy Exp $
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@ethereal.com>
 * Copyright 1998 Gerald Combs
 *
 * Copied from README.developer
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

#include <glib.h>
#include <string.h>

#include <epan/packet.h>
#include "packet-ieee80211.h"
#include "packet-wlancap.h"

#define SHORT_STR 256

/* protocol */
static int proto_wlancap = -1;

/* header attached during wlan monitor mode */
struct wlan_header_v1 {
  guint32 version;
  guint32 length;
  guint64 mactime;
  guint64 hosttime;
  guint32 phytype;
  guint32 channel;
  guint32 datarate;
  guint32 antenna;
  guint32 priority;
  guint32 ssi_type;
  gint32 ssi_signal;
  gint32 ssi_noise;
  gint32 preamble;
  gint32 encoding;
};

static int hf_wlan_version = -1;
static int hf_wlan_length = -1;
static int hf_wlan_mactime = -1;
static int hf_wlan_hosttime = -1;
static int hf_wlan_phytype = -1;
static int hf_wlan_channel = -1;
static int hf_wlan_datarate = -1;
static int hf_wlan_antenna = -1;
static int hf_wlan_priority = -1;
static int hf_wlan_ssi_type = -1;
static int hf_wlan_ssi_signal = -1;
static int hf_wlan_ssi_noise = -1;
static int hf_wlan_preamble = -1;
static int hf_wlan_encoding = -1;

static gint ett_wlan = -1;

static dissector_handle_t ieee80211_handle;

static void
dissect_wlancap(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);

void
capture_wlancap(const guchar *pd, int offset, int len, packet_counts *ld)
{
    /* XXX eventually add in a version test. */
    if(!BYTES_ARE_IN_FRAME(offset, len, (int)sizeof(struct wlan_header_v1))) {
        ld->other ++;
        return;
    }
    offset += sizeof(struct wlan_header_v1);

    /* 802.11 header follows */
    capture_ieee80211(pd, offset, len, ld);
}

void
proto_register_wlancap(void)
{

  static const value_string phy_type[] = {
    { 0, "Unknown" },
    { 1, "FHSS 802.11 '97" },
    { 2, "DSSS 802.11 '97" }, 
    { 3, "IR Baseband" },
    { 4, "DSSS 802.11b" },
    { 5, "PBCC 802.11b" }, 
    { 6, "OFDM 802.11g" },
    { 7, "PBCC 802.11g" },
    { 8, "OFDM 802.11a" },
  };

  static const value_string encoding_type[] = {
    { 0, "Unknown" },
    { 1, "CCK" },
    { 2, "PBCC" },
    { 3, "OFDM" },
  };

  static const value_string ssi_type[] = {
    { 0, "None" },
    { 1, "Normalized RSSI" },
    { 2, "dBm" },
    { 3, "Raw RSSI" },
  };

  static const value_string preamble_type[] = {
    { 0, "Unknown" },
    { 1, "Short" },
    { 2, "Long" },
  };

  static hf_register_info hf[] = {
    { &hf_wlan_version, { "Header revision", "wlancap.version", FT_UINT32, 
			  BASE_DEC, NULL, 0x0, "", HFILL } },
    { &hf_wlan_length, { "Header length", "wlancap.length", FT_UINT32, 
			 BASE_DEC, NULL, 0x0, "", HFILL } },
    { &hf_wlan_mactime, { "MAC timestamp", "wlancap.mactime", FT_UINT64, 
			  BASE_DEC, NULL, 0x0, "", HFILL } },
    { &hf_wlan_hosttime, { "Host timestamp", "wlancap.hosttime", FT_UINT64, 
			   BASE_DEC, NULL, 0x0, "", HFILL } },
    { &hf_wlan_phytype, { "PHY type", "wlancap.phytype", FT_UINT32, BASE_DEC,
			  VALS(phy_type), 0x0, "", HFILL } },
    { &hf_wlan_channel, { "Channel", "wlancap.channel", FT_UINT32, BASE_DEC,
			  NULL, 0x0, "", HFILL } },
    { &hf_wlan_datarate, { "Data rate", "wlancap.datarate", FT_UINT32, 
			   BASE_DEC, NULL, 0x0, "", HFILL } },
    { &hf_wlan_antenna, { "Antenna", "wlancap.antenna", FT_UINT32, BASE_DEC,
			  NULL, 0x0, "", HFILL } },
    { &hf_wlan_priority, { "Priority", "wlancap.priority", FT_UINT32, BASE_DEC,
			   NULL, 0x0, "", HFILL } },
    { &hf_wlan_ssi_type, { "SSI Type", "wlancap.ssi_type", FT_UINT32, BASE_DEC,
			   VALS(ssi_type), 0x0, "", HFILL } },
    { &hf_wlan_ssi_signal, { "SSI Signal", "wlancap.ssi_signal", FT_UINT32, 
			     BASE_DEC, NULL, 0x0, "", HFILL } },
    { &hf_wlan_ssi_noise, { "SSI Noise", "wlancap.ssi_noise", FT_INT32, 
			    BASE_DEC, NULL, 0x0, "", HFILL } },
    { &hf_wlan_preamble, { "Preamble", "wlancap.preamble", FT_UINT32, 
			   BASE_DEC, VALS(preamble_type), 0x0, "", HFILL } },
    { &hf_wlan_encoding, { "Encoding Type", "wlancap.encoding", FT_UINT32, 
			   BASE_DEC, VALS(encoding_type), 0x0, "", HFILL } },
  };
  static gint *ett[] = {
    &ett_wlan
  };

  proto_wlancap = proto_register_protocol("AVS WLAN Capture header", "AVS WLANCAP", "wlancap");
  proto_register_field_array(proto_wlancap, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));
  register_dissector("wlancap", dissect_wlancap, proto_wlancap);

}

static void
dissect_wlancap(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
    proto_tree *wlan_tree;
    proto_item *ti;
    tvbuff_t *next_tvb;
    int offset;
    guint32 version;
    guint32 length;

    if(check_col(pinfo->cinfo, COL_PROTOCOL))
        col_set_str(pinfo->cinfo, COL_PROTOCOL, "WLAN");
    if(check_col(pinfo->cinfo, COL_INFO))
        col_clear(pinfo->cinfo, COL_INFO);
    offset = 0;

    version = tvb_get_ntohl(tvb, offset) - WLANCAP_MAGIC_COOKIE_BASE;
    length = tvb_get_ntohl(tvb, offset+4);

    if(check_col(pinfo->cinfo, COL_INFO))
        col_add_fstr(pinfo->cinfo, COL_INFO, "AVS WLAN Capture v%x, Length %d",version, length);

    /* Dissect the packet */
    if (tree) {
      ti = proto_tree_add_protocol_format(tree, proto_wlancap,
            tvb, 0, length, "AVS WLAN Monitoring Header");
      wlan_tree = proto_item_add_subtree(ti, ett_wlan);
      proto_tree_add_uint(wlan_tree, hf_wlan_version, tvb, offset,
			  4, tvb_get_ntohl(tvb, offset) - WLANCAP_MAGIC_COOKIE_BASE);
      offset+=4;
      proto_tree_add_uint(wlan_tree, hf_wlan_length, tvb, offset,
			  4, tvb_get_ntohl(tvb, offset));
      offset+=4;
      proto_tree_add_item(wlan_tree, hf_wlan_mactime, tvb, offset,
			  8, FALSE);
      offset+=8;
      proto_tree_add_item(wlan_tree, hf_wlan_hosttime, tvb, offset,
			  8, FALSE);
      offset+=8;

      proto_tree_add_uint(wlan_tree, hf_wlan_phytype, tvb, offset,
			  4, tvb_get_ntohl(tvb, offset));
      offset+=4;
      /* XXX cook channel (fh uses different numbers) */
      proto_tree_add_uint(wlan_tree, hf_wlan_channel, tvb, offset,
			  4, tvb_get_ntohl(tvb, offset));
      offset+=4;

      proto_tree_add_uint_format(wlan_tree, hf_wlan_datarate, tvb, offset, 
				 4, tvb_get_ntohl(tvb, offset) * 100, 
				 "Datarate: %d kbps", 
				 tvb_get_ntohl(tvb, offset) * 100);
      offset+=4;
      proto_tree_add_uint(wlan_tree, hf_wlan_antenna, tvb, offset,
			  4, tvb_get_ntohl(tvb, offset));
      offset+=4;
      proto_tree_add_uint(wlan_tree, hf_wlan_priority, tvb, offset,
			  4, tvb_get_ntohl(tvb, offset));
      offset+=4;
      proto_tree_add_uint(wlan_tree, hf_wlan_ssi_type, tvb, offset,
			  4, tvb_get_ntohl(tvb, offset));
      offset+=4;
      /* XXX cook ssi_signal (Based on type; ie format) */
      proto_tree_add_uint(wlan_tree, hf_wlan_ssi_signal, tvb, offset,
			  4, tvb_get_ntohl(tvb, offset));
      offset+=4;
      /* XXX cook ssi_noise (Based on type; ie format) */
      proto_tree_add_int(wlan_tree, hf_wlan_ssi_noise, tvb, offset,
			  4, tvb_get_ntohl(tvb, offset));
      offset+=4;
      proto_tree_add_uint(wlan_tree, hf_wlan_preamble, tvb, offset,
			  4, tvb_get_ntohl(tvb, offset));
      offset+=4;
      proto_tree_add_uint(wlan_tree, hf_wlan_encoding, tvb, offset,
			  4, tvb_get_ntohl(tvb, offset));
      offset+=4;
    }

    if (offset == 0)
      offset = length;

    /* dissect the 802.11 header next */
    next_tvb = tvb_new_subset(tvb, offset, -1, -1);
    call_dissector(ieee80211_handle, next_tvb, pinfo, tree);
}

void
proto_reg_handoff_wlancap(void)
{
    dissector_handle_t wlancap_handle;

    /* handle for 802.11 dissector */
    ieee80211_handle = find_dissector("wlan");

    wlancap_handle = create_dissector_handle(dissect_wlancap, proto_wlancap);

    dissector_add("wtap_encap", WTAP_ENCAP_WLAN_HEADER, wlancap_handle);
}
