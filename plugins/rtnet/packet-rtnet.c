/* packet-rtnet.c
 * Routines for RTnet packet disassembly
 *
 * $Id$
 *
 * Copyright (c) 2003 by Erwin Rol <erwin@erwinrol.com>
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@ethereal.com>
 * Copyright 1999 Gerald Combs
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

/* Include files */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "plugins/plugin_api.h"

#include "moduleinfo.h"

#include <stdio.h>
#include <stdlib.h>
#include <gmodule.h>
#include <ctype.h>
#include <time.h>
#include <string.h>
#include <epan/packet.h>
#include <epan/resolv.h>
#include "etypes.h"
#include <epan/strutil.h>

#include "plugins/plugin_api_defs.h"

/* Define version if we are not building ethereal statically */

#ifndef ENABLE_STATIC
G_MODULE_EXPORT const gchar version[] = VERSION;
#endif

/*
 * See
 *
 *	http://www.rts.uni-hannover.de/rtnet/
 */

#define RTNET_TYPE_TDMA     0x9031
#define RTNET_TYPE_IP       ETHERTYPE_IP
#define RTNET_TYPE_ARP      ETHERTYPE_ARP

static const value_string rtnet_type_vals[] = {
  { RTNET_TYPE_TDMA, "TDMA" },
  { RTNET_TYPE_IP,   "IP" },
  { RTNET_TYPE_ARP,  "ARP" },
  { 0, NULL }
};

#define RTCFG_MSG_S1_CONFIG    0x0
#define RTCFG_MSG_ANN_NEW      0x1
#define RTCFG_MSG_ANN_REPLY    0x2
#define RTCFG_MSG_S2_CONFIG    0x3
#define RTCFG_MSG_S2_FRAG      0x4
#define RTCFG_MSG_ACK          0x5
#define RTCFG_MSG_READY        0x6
#define RTCFG_MSG_HBEAT        0x7

static const value_string rtcfg_msg_vals[] = {
  { RTCFG_MSG_S1_CONFIG, "Stage 1 Config" },
  { RTCFG_MSG_ANN_NEW,   "New Announce" },
  { RTCFG_MSG_ANN_REPLY, "Reply Announce" },
  { RTCFG_MSG_S2_CONFIG, "Stage 2 Config" },
  { RTCFG_MSG_S2_FRAG,   "Stage 2 Fragment" },
  { RTCFG_MSG_ACK,       "Acknowledge" },
  { RTCFG_MSG_READY,     "Ready" },
  { RTCFG_MSG_HBEAT,     "Heartbeat" },
  { 0, NULL }
};

#define RTCFG_ADDRESS_TYPE_MAC  0x00
#define RTCFG_ADDRESS_TYPE_IP   0x01

static const value_string rtcfg_address_type_vals[] = {
  { RTCFG_ADDRESS_TYPE_MAC,    "MAC" },
  { RTCFG_ADDRESS_TYPE_IP,     "IP" },
  { 0, NULL }
};

#define RTNET_TDMA_MSG_NOTIFY_MASTER          0x10
#define RTNET_TDMA_MSG_REQUEST_TEST           0x11
#define RTNET_TDMA_MSG_ACK_TEST               0x12
#define RTNET_TDMA_MSG_REQUEST_CONF           0x13
#define RTNET_TDMA_MSG_ACK_CONF               0x14
#define RTNET_TDMA_MSG_ACK_ACK_CONF           0x15
#define RTNET_TDMA_MSG_STATION_LIST           0x16
#define RTNET_TDMA_MSG_REQUEST_CHANGE_OFFSET  0x17
#define RTNET_TDMA_MSG_START_OF_FRAME         0x18

static const value_string rtnet_tdma_msg_vals[] = {
  { RTNET_TDMA_MSG_NOTIFY_MASTER,         "Notify Master" },
  { RTNET_TDMA_MSG_REQUEST_TEST,          "Request Test" },
  { RTNET_TDMA_MSG_ACK_TEST,              "Acknowledge Test" },
  { RTNET_TDMA_MSG_REQUEST_CONF,          "Request Config" },
  { RTNET_TDMA_MSG_ACK_CONF,              "Acknowledge Config" },
  { RTNET_TDMA_MSG_ACK_ACK_CONF,          "Ack Ack Config" },
  { RTNET_TDMA_MSG_STATION_LIST,          "Station List" },
  { RTNET_TDMA_MSG_REQUEST_CHANGE_OFFSET, "Request Change Offset" },
  { RTNET_TDMA_MSG_START_OF_FRAME,        "Start of Frame" },
  { 0, NULL }
};

static dissector_handle_t ip_handle;
static dissector_handle_t arp_handle;

void proto_reg_handoff_rtnet(void);
void proto_reg_handoff_rtcfg(void);

/* Define the rtnet proto */
static int proto_rtnet = -1;
static int proto_rtcfg = -1;

static int hf_rtcfg_vers_id = -1;
static int hf_rtcfg_vers = -1;
static int hf_rtcfg_id = -1;
static int hf_rtcfg_address_type = -1;
static int hf_rtcfg_client_ip_address = -1;
static int hf_rtcfg_server_ip_address = -1;
static int hf_rtcfg_burst_rate = -1;
static int hf_rtcfg_padding = -1;
static int hf_rtcfg_s1_config_length = -1;
static int hf_rtcfg_config_data = -1;
static int hf_rtcfg_client_flags = -1;
static int hf_rtcfg_client_flags_available = -1;
static int hf_rtcfg_client_flags_ready = -1;
static int hf_rtcfg_client_flags_res = -1;
static int hf_rtcfg_server_flags = -1;
static int hf_rtcfg_server_flags_res0 = -1;
static int hf_rtcfg_server_flags_ready = -1;
static int hf_rtcfg_server_flags_res2 = -1;

static int hf_rtcfg_active_stations = -1;
static int hf_rtcfg_heartbeat_period = -1;
static int hf_rtcfg_s2_config_length = -1;
static int hf_rtcfg_config_offset = -1;
static int hf_rtcfg_ack_length = -1;

/* Header */
static int hf_rtnet_header_type = -1;
static int hf_rtnet_header_ver = -1;
static int hf_rtnet_header_res = -1;

/* TDMA */
static int hf_rtnet_tdma = -1;
static int hf_rtnet_tdma_msg = -1;

/* TDMA REQUEST_CONF */
static int hf_rtnet_tdma_msg_request_conf_station = -1;
static int hf_rtnet_tdma_msg_request_conf_padding = -1;
static int hf_rtnet_tdma_msg_request_conf_mtu = -1;
static int hf_rtnet_tdma_msg_request_conf_cycle = -1;

/* TDMA ACK_CONF */
static int hf_rtnet_tdma_msg_ack_conf_station = -1;
static int hf_rtnet_tdma_msg_ack_conf_padding = -1;
static int hf_rtnet_tdma_msg_ack_conf_mtu = -1;
static int hf_rtnet_tdma_msg_ack_conf_cycle = -1;

/* TDMA ACK_ACK_CONF */
static int hf_rtnet_tdma_msg_ack_ack_conf_station = -1;
static int hf_rtnet_tdma_msg_ack_ack_conf_padding = -1;

/* TDMA REQUEST_TEST */
static int hf_rtnet_tdma_msg_request_test_counter = -1;
static int hf_rtnet_tdma_msg_request_test_tx = -1;

/* TDMA ACK_TEST */
static int hf_rtnet_tdma_msg_ack_test_counter = -1;
static int hf_rtnet_tdma_msg_ack_test_tx = -1;

/* TDMA STATION_LIST */
static int hf_rtnet_tdma_msg_station_list_nr_stations = -1;
static int hf_rtnet_tdma_msg_station_list_padding = -1;

static int hf_rtnet_tdma_msg_station_list_ip = -1;
static int hf_rtnet_tdma_msg_station_list_nr = -1;

/* TDMA CHANGE_OFFSET */
static int hf_rtnet_tdma_msg_request_change_offset_offset = -1;

/* TDMA START_OF_FRAME */
static int hf_rtnet_tdma_msg_start_of_frame_timestamp = -1;

/* IP */
static int hf_rtnet_ip = -1;

/* ARP */
static int hf_rtnet_arp = -1;

/* Define the tree for rtnet */
static int ett_rtnet = -1;
static int ett_rtcfg = -1;

static guint
dissect_rtnet_tdma_notify_master(tvbuff_t *tvb _U_, guint offset, proto_tree *tree _U_) 
{
  return offset;
}

static guint
dissect_rtnet_tdma_request_test(tvbuff_t *tvb, guint offset, proto_tree *tree) 
{
  proto_tree_add_item(tree, hf_rtnet_tdma_msg_request_test_counter, tvb,
                       offset, 4, TRUE );
  offset += 4;

  proto_tree_add_item(tree, hf_rtnet_tdma_msg_request_test_tx, tvb,
                       offset, 8, TRUE );
  offset += 8;

  return offset;
}

static guint
dissect_rtnet_tdma_ack_test(tvbuff_t *tvb, guint offset, proto_tree *tree) 
{
  proto_tree_add_item(tree, hf_rtnet_tdma_msg_ack_test_counter, tvb,
                       offset, 4, TRUE );
  offset += 4;

  proto_tree_add_item(tree, hf_rtnet_tdma_msg_ack_test_tx, tvb,
                       offset, 8, TRUE );
  offset += 8;

  return offset;
}

static guint
dissect_rtnet_tdma_request_conf(tvbuff_t *tvb, guint offset, proto_tree *tree) 
{
  proto_tree_add_item(tree, hf_rtnet_tdma_msg_request_conf_station, tvb,
                       offset, 1, FALSE );
  offset += 1;

  proto_tree_add_item(tree, hf_rtnet_tdma_msg_request_conf_padding, tvb,
                       offset, 1, FALSE );
  offset += 1;

  proto_tree_add_item(tree, hf_rtnet_tdma_msg_request_conf_mtu, tvb,
                       offset, 2, FALSE );
  offset += 2;

  proto_tree_add_item(tree, hf_rtnet_tdma_msg_request_conf_cycle, tvb,
                       offset, 4, FALSE );
  offset += 4;

  return offset;
}


static guint
dissect_rtnet_tdma_ack_conf(tvbuff_t *tvb, guint offset, proto_tree *tree) 
{
  proto_tree_add_item(tree, hf_rtnet_tdma_msg_ack_conf_station, tvb,
                       offset, 1, FALSE );
  offset += 1;

  proto_tree_add_item(tree, hf_rtnet_tdma_msg_ack_conf_padding, tvb,
                       offset, 1, FALSE );
  offset += 1;

  proto_tree_add_item(tree, hf_rtnet_tdma_msg_ack_conf_mtu, tvb,
                       offset, 2, FALSE );
  offset += 2;

  proto_tree_add_item(tree, hf_rtnet_tdma_msg_ack_conf_cycle, tvb,
                       offset, 4, FALSE );
  offset += 4;

  return offset;
}

static guint
dissect_rtnet_tdma_ack_ack_conf(tvbuff_t *tvb, guint offset, proto_tree *tree) {

  proto_tree_add_item(tree, hf_rtnet_tdma_msg_ack_ack_conf_station, tvb,
                       offset, 1, FALSE );

  offset += 1;

  proto_tree_add_item(tree, hf_rtnet_tdma_msg_ack_ack_conf_padding, tvb,
                       offset, 3, FALSE );
  offset += 3;

  return offset;
}

static guint
dissect_rtnet_tdma_station_list(tvbuff_t *tvb, guint offset, proto_tree *tree) 
{
  guint8 nr_stations;
  guint8 i;
    
  nr_stations = tvb_get_guint8(tvb, offset);
  proto_tree_add_uint(tree, hf_rtnet_tdma_msg_station_list_nr_stations, tvb,
                      offset, 1, nr_stations);
                            
  offset += 1;

  proto_tree_add_item(tree, hf_rtnet_tdma_msg_station_list_padding, tvb,
                       offset, 3, FALSE );
  offset += 3;


  for( i = 0; i < nr_stations; i++ )
  {
    proto_tree_add_item(tree, hf_rtnet_tdma_msg_station_list_ip, tvb,
                        offset, 4, FALSE );

    offset += 4;  
    
    proto_tree_add_item(tree, hf_rtnet_tdma_msg_station_list_nr, tvb,
                        offset, 1, FALSE );

    offset += 1;

    proto_tree_add_item(tree, hf_rtnet_tdma_msg_station_list_padding, tvb,
                        offset, 3, FALSE );
    offset += 3;
  }
  
  return offset;
}

static guint
dissect_rtnet_tdma_request_change_offset(tvbuff_t *tvb, guint offset, proto_tree *tree) 
{
  proto_tree_add_item(tree, hf_rtnet_tdma_msg_request_change_offset_offset, tvb,
                       offset, 4, FALSE );

  offset += 4;

  return offset;
}

static guint
dissect_rtnet_tdma_start_of_frame(tvbuff_t *tvb, guint offset, proto_tree *tree) 
{
  proto_tree_add_item(tree, hf_rtnet_tdma_msg_start_of_frame_timestamp, tvb,
                       offset, 8, FALSE );
  offset += 8;

  return offset;
}

static guint
dissect_rtnet_tdma(tvbuff_t *tvb, packet_info *pinfo, guint offset, proto_tree *tree, proto_tree* ti) {
  guint32 msg;

  msg = tvb_get_ntohl(tvb, offset);
  if( tree )
  {
    proto_tree_add_uint(tree, hf_rtnet_tdma_msg, tvb,
                        offset, 4, msg);
  }

  offset += 4;

  /* set the info column */
  if (check_col(pinfo->cinfo, COL_INFO)) {
    col_append_fstr(pinfo->cinfo, COL_INFO, ", %s",
      val_to_str(msg, rtnet_tdma_msg_vals, "Unknown (0x%04x)"));
  }

  if( tree )
  {  
    proto_item_append_text(ti, ", %s",
             val_to_str(msg, rtnet_tdma_msg_vals, "Unknown (0x%04x)"));
    
    switch( msg ) {
      case RTNET_TDMA_MSG_NOTIFY_MASTER:
        return dissect_rtnet_tdma_notify_master( tvb, offset, tree );
    	
      case RTNET_TDMA_MSG_REQUEST_TEST:
        return dissect_rtnet_tdma_request_test( tvb, offset, tree );

      case RTNET_TDMA_MSG_ACK_TEST:
       return dissect_rtnet_tdma_ack_test( tvb, offset, tree );

      case RTNET_TDMA_MSG_REQUEST_CONF:
        return dissect_rtnet_tdma_request_conf( tvb, offset, tree );

      case RTNET_TDMA_MSG_ACK_CONF:
        return dissect_rtnet_tdma_ack_conf( tvb, offset, tree );

      case RTNET_TDMA_MSG_ACK_ACK_CONF:
        return dissect_rtnet_tdma_ack_ack_conf( tvb, offset, tree );

      case RTNET_TDMA_MSG_STATION_LIST:
        return dissect_rtnet_tdma_station_list( tvb, offset, tree );

      case RTNET_TDMA_MSG_REQUEST_CHANGE_OFFSET:
        return dissect_rtnet_tdma_request_change_offset( tvb, offset, tree );

      case RTNET_TDMA_MSG_START_OF_FRAME:
        return dissect_rtnet_tdma_start_of_frame( tvb, offset, tree );

      default:
        break;	  
    }
  }	

  return offset;
}

static void
dissect_rtnet(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree) {
  gint offset = 0;
  guint size;
  guint8 ver,res;
  guint16 type;
  tvbuff_t *next_tvb = NULL;
  proto_tree *ti=NULL,*hi=NULL,*si=NULL,*rtnet_tree=NULL;

  /* Set the protocol column */
  if(check_col(pinfo->cinfo,COL_PROTOCOL)){
    col_set_str(pinfo->cinfo,COL_PROTOCOL,"RTNET");
  }

  /* Clear out stuff in the info column */
  if(check_col(pinfo->cinfo,COL_INFO)){
    col_clear(pinfo->cinfo,COL_INFO);
  }

  if (tree) {
    ti = proto_tree_add_item(tree, proto_rtnet, tvb, offset, 4, FALSE);
    rtnet_tree = proto_item_add_subtree(ti, ett_rtnet);
  }

  type = tvb_get_ntohs(tvb, offset);
  /* set the info column */
  if (check_col(pinfo->cinfo, COL_INFO)) {
    col_add_fstr(pinfo->cinfo, COL_INFO, "%s",
      val_to_str(type, rtnet_type_vals, "Unknown (0x%04x)"));
  }

  if( rtnet_tree )
  {
    proto_tree_add_uint(rtnet_tree, hf_rtnet_header_type, tvb,
                        offset, 2, type);
  }
  offset += 2;

  ver = tvb_get_guint8(tvb, offset);
  if( rtnet_tree )
  {
    proto_tree_add_uint(rtnet_tree, hf_rtnet_header_ver, tvb,
                        offset, 1, ver);


    proto_item_append_text(ti, ", Vers. %d", ver);
  }
  offset += 1;

  res = tvb_get_guint8(tvb, offset);
  if( rtnet_tree )
  {
    proto_tree_add_uint(rtnet_tree, hf_rtnet_header_res, tvb,
                        offset, 1, res);
  }
  
  offset += 1;

  switch( type ) 
  {
    
    case RTNET_TYPE_TDMA:
      if( tree )
      {
        hi = proto_tree_add_item(tree,
                                 hf_rtnet_tdma,
                                 tvb,
                                 offset,
                                 -1,
                                 FALSE);

        si = proto_item_add_subtree(hi, ett_rtnet);
      }

      size = dissect_rtnet_tdma( tvb, pinfo, offset, si, hi);
      size -= offset;
      
      if( si )
        proto_item_set_len(si, size);

      break;

    case RTNET_TYPE_IP:
      if (!next_tvb)
        next_tvb = tvb_new_subset(tvb, offset, -1, -1);

      if( tree )
        call_dissector(ip_handle, next_tvb, pinfo, tree);
      
      break;

    case RTNET_TYPE_ARP:
      if (!next_tvb)
        next_tvb = tvb_new_subset(tvb, offset, -1, -1);

      if( tree )
        call_dissector(arp_handle, next_tvb, pinfo, tree);

      break;

    default:
      if( tree )
      {
        proto_tree_add_text(tree, tvb, offset, -1,
          "Data (%d bytes)", tvb_reported_length_remaining(tvb, offset));
      }
      break;
  }
}

static void
dissect_rtcfg(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree) {
  gint offset = 0;
  proto_tree *vers_id_tree, *vers_id_item, *flags_tree, *flags_item;
  guint8 vers_id;
  guint8 address_type;
  guint32 config_length,len;
  proto_tree *ti=NULL,*rtcfg_tree=NULL;

  /* Set the protocol column */
  if(check_col(pinfo->cinfo,COL_PROTOCOL)){
    col_set_str(pinfo->cinfo,COL_PROTOCOL,"RTcfg");
  }

  /* Clear out stuff in the info column */
  if(check_col(pinfo->cinfo,COL_INFO)){
    col_clear(pinfo->cinfo,COL_INFO);
  }

  if (tree) {
    ti = proto_tree_add_item(tree, proto_rtcfg, tvb, offset, -1, FALSE);
    rtcfg_tree = proto_item_add_subtree(ti, ett_rtcfg);
  }

  vers_id = tvb_get_guint8(tvb, offset);

  if (check_col(pinfo->cinfo, COL_INFO)) {
    col_add_fstr(pinfo->cinfo, COL_INFO, "%s",
           val_to_str(vers_id, rtcfg_msg_vals, "Unknown (0x%04x)"));
  }

  if( rtcfg_tree )
  {
    vers_id_item = proto_tree_add_uint(rtcfg_tree, hf_rtcfg_vers_id, tvb,
                                       offset, 1, vers_id);

    vers_id_tree=proto_item_add_subtree(vers_id_item, ett_rtcfg);
    proto_tree_add_item(vers_id_tree, hf_rtcfg_vers, tvb, offset, 1, FALSE);
    proto_tree_add_item(vers_id_tree, hf_rtcfg_id, tvb, offset, 1, FALSE);
    offset += 1;

    proto_item_append_text(ti, ", Vers. %d, %s",
             (vers_id >> 5),
             val_to_str(vers_id, rtcfg_msg_vals, "Unknown (0x%04x)"));

    switch( vers_id & 0x1f )
    {
       case RTCFG_MSG_S1_CONFIG:
         address_type = tvb_get_guint8(tvb, offset);
         proto_tree_add_item( rtcfg_tree, hf_rtcfg_address_type, tvb, offset, 1, FALSE );
         offset += 1;

         switch( address_type )
         {
           case RTCFG_ADDRESS_TYPE_MAC:
             /* nothing */
             break;

           case RTCFG_ADDRESS_TYPE_IP:
             proto_tree_add_item( rtcfg_tree, hf_rtcfg_client_ip_address, tvb, offset, 4, FALSE );
             offset += 4;

             proto_tree_add_item( rtcfg_tree, hf_rtcfg_server_ip_address, tvb, offset, 4, FALSE );
             offset += 4;

             break;
         }

         proto_tree_add_item( rtcfg_tree, hf_rtcfg_burst_rate, tvb, offset, 1, FALSE );
         offset += 1;

         config_length = tvb_get_ntohs( tvb, offset );
         proto_tree_add_item( rtcfg_tree, hf_rtcfg_s1_config_length, tvb, offset, 2, FALSE );
         offset += 2;

         if( config_length > 0 ) {
           proto_tree_add_item( rtcfg_tree, hf_rtcfg_config_data, tvb, offset, config_length, FALSE );
           offset += config_length;
         }

         break;

       case RTCFG_MSG_ANN_NEW:
         address_type = tvb_get_guint8(tvb, offset);
         proto_tree_add_item( rtcfg_tree, hf_rtcfg_address_type, tvb, offset, 1, FALSE );
         offset += 1;

         switch( address_type )
         {
           case RTCFG_ADDRESS_TYPE_MAC:
             /* nothing */
             break;

           case RTCFG_ADDRESS_TYPE_IP:
             proto_tree_add_item( rtcfg_tree, hf_rtcfg_client_ip_address, tvb, offset, 4, FALSE );
             offset += 4;
             break;
         }

         flags_item = proto_tree_add_item(rtcfg_tree, hf_rtcfg_client_flags, tvb,
                                          offset, 1, FALSE);

         flags_tree=proto_item_add_subtree(flags_item, ett_rtcfg);
         proto_tree_add_item(flags_tree, hf_rtcfg_client_flags_available, tvb, offset, 1, FALSE);
         proto_tree_add_item(flags_tree, hf_rtcfg_client_flags_ready, tvb, offset, 1, FALSE);
         proto_tree_add_item(flags_tree, hf_rtcfg_client_flags_res, tvb, offset, 1, FALSE);
         offset += 1;
 
         proto_tree_add_item( rtcfg_tree, hf_rtcfg_burst_rate, tvb, offset, 1, FALSE );
         offset += 1;

         break;

       case RTCFG_MSG_ANN_REPLY:
         address_type = tvb_get_guint8(tvb, offset);
         proto_tree_add_item( rtcfg_tree, hf_rtcfg_address_type, tvb, offset, 1, FALSE );
         offset += 1;

         switch( address_type )
         {
           case RTCFG_ADDRESS_TYPE_MAC:
             /* nothing */
             break;

           case RTCFG_ADDRESS_TYPE_IP:
             proto_tree_add_item( rtcfg_tree, hf_rtcfg_client_ip_address, tvb, offset, 4, FALSE );
             offset += 4;
             break;
         }

         flags_item = proto_tree_add_item(rtcfg_tree, hf_rtcfg_client_flags, tvb,
                                          offset, 1, FALSE);

         flags_tree=proto_item_add_subtree(flags_item, ett_rtcfg);
         proto_tree_add_item(flags_tree, hf_rtcfg_client_flags_available, tvb, offset, 1, FALSE);
         proto_tree_add_item(flags_tree, hf_rtcfg_client_flags_ready, tvb, offset, 1, FALSE);
         proto_tree_add_item(flags_tree, hf_rtcfg_client_flags_res, tvb, offset, 1, FALSE);
         offset += 1;

         proto_tree_add_item( rtcfg_tree, hf_rtcfg_padding, tvb, offset, 1, FALSE );
         offset += 1;

         break;

       case RTCFG_MSG_S2_CONFIG:
         flags_item = proto_tree_add_item(rtcfg_tree, hf_rtcfg_server_flags, tvb,
                                          offset, 1, FALSE);

         flags_tree=proto_item_add_subtree(flags_item, ett_rtcfg);
         proto_tree_add_item(flags_tree, hf_rtcfg_server_flags_res0, tvb, offset, 1, FALSE);
         proto_tree_add_item(flags_tree, hf_rtcfg_server_flags_ready, tvb, offset, 1, FALSE);
         proto_tree_add_item(flags_tree, hf_rtcfg_server_flags_res2, tvb, offset, 1, FALSE);
         offset += 1;

         proto_tree_add_item( rtcfg_tree, hf_rtcfg_active_stations, tvb, offset, 4, FALSE );
         offset += 4;

         proto_tree_add_item( rtcfg_tree, hf_rtcfg_heartbeat_period, tvb, offset, 2, FALSE );
         offset += 2;

         config_length = tvb_get_ntohl( tvb, offset );
         proto_tree_add_item( rtcfg_tree, hf_rtcfg_s2_config_length, tvb, offset, 4, FALSE );
         offset += 4;

         if( config_length > 0 ) {
           len = tvb_reported_length_remaining(tvb, offset);
           proto_tree_add_item( rtcfg_tree, hf_rtcfg_config_data, tvb, offset, len, FALSE );
           offset += len;
         }

         break;

       case RTCFG_MSG_S2_FRAG:
         proto_tree_add_item( rtcfg_tree, hf_rtcfg_config_offset, tvb, offset, 4, FALSE );
         offset += 4;

         len = tvb_reported_length_remaining(tvb, offset);
         proto_tree_add_item( rtcfg_tree, hf_rtcfg_config_data, tvb, offset, len, FALSE );
         offset += len;
         break;

       case RTCFG_MSG_ACK:
         proto_tree_add_item( rtcfg_tree, hf_rtcfg_ack_length, tvb, offset, 4, FALSE );
         offset += 4;

         break;

       case RTCFG_MSG_READY:
         break;

       case RTCFG_MSG_HBEAT:
         break;

    }
  }
}

void
proto_register_rtnet(void) {
  static hf_register_info hf[] = {

    /* header */
    { &hf_rtnet_header_type,
      { "Type",
        "rtnet.header.type",
        FT_UINT16, BASE_HEX, VALS(rtnet_type_vals), 0x0,
        "RTNET Frame Type", HFILL }},

    { &hf_rtnet_header_ver,
      { "Version",
        "rtnet.header.ver",
        FT_UINT16, BASE_DEC, NULL, 0x0,
        "RTNET Version", HFILL }},

    { &hf_rtnet_header_res,
      { "Reserved",
        "rtnet.header.res",
        FT_UINT8, BASE_HEX, NULL, 0x0,
        "RTNET Reserved", HFILL }},

    /* TDMA */
    { &hf_rtnet_tdma,
      { "TDMA",
        "rtnet.tdma",
        FT_NONE, BASE_NONE, NULL, 0,
        "RTNET TDMA", HFILL }},

    /* IP */
    { &hf_rtnet_ip,
      { "IP",
        "rtnet.ip",
        FT_NONE, BASE_NONE, NULL, 0,
        "RTNET IP", HFILL }},

    /* ARP */
    { &hf_rtnet_arp,
      { "ARP",
        "rtnet.arp",
        FT_NONE, BASE_NONE, NULL, 0,
        "RTNET ARP", HFILL }},

    /* TDMA msg */
    { &hf_rtnet_tdma_msg,
      { "Message",
        "rtnet.tdma.msg",
        FT_UINT32, BASE_HEX, VALS(rtnet_tdma_msg_vals), 0x0,
        "RTNET TDMA Message", HFILL }},

    /* TDMA request conf */

    { &hf_rtnet_tdma_msg_request_conf_station,
      { "Station",
        "rtnet.tdma.msg.request_conf.station",
        FT_UINT8, BASE_DEC, NULL, 0x0,
        "TDMA Station", HFILL }},

    { &hf_rtnet_tdma_msg_request_conf_padding,
      { "Padding",
        "rtnet.tdma.msg.request_conf.padding",
        FT_UINT8, BASE_HEX, NULL, 0x0,
        "TDMA PAdding", HFILL }},

    { &hf_rtnet_tdma_msg_request_conf_mtu,
      { "MTU",
        "rtnet.tdma.msg.request_conf.mtu",
        FT_UINT8, BASE_DEC, NULL, 0x0,
        "TDMA MTU", HFILL }},

    { &hf_rtnet_tdma_msg_request_conf_cycle,
      { "Cycle",
        "rtnet.tdma.msg.request_conf.cycle",
        FT_UINT8, BASE_DEC, NULL, 0x0,
        "TDMA Cycle", HFILL }},

    /* TDMA ack conf */

    { &hf_rtnet_tdma_msg_ack_conf_station,
      { "Station",
        "rtnet.tdma.msg.ack_conf.station",
        FT_UINT8, BASE_DEC, NULL, 0x0,
        "TDMA Station", HFILL }},

    { &hf_rtnet_tdma_msg_ack_conf_padding,
      { "Padding",
        "rtnet.tdma.msg.ack_conf.padding",
        FT_UINT8, BASE_HEX, NULL, 0x0,
        "TDMA PAdding", HFILL }},

    { &hf_rtnet_tdma_msg_ack_conf_mtu,
      { "MTU",
        "rtnet.tdma.msg.ack_conf.mtu",
        FT_UINT8, BASE_DEC, NULL, 0x0,
        "TDMA MTU", HFILL }},

    { &hf_rtnet_tdma_msg_ack_conf_cycle,
      { "Cycle",
        "rtnet.tdma.msg.ack_conf.cycle",
        FT_UINT8, BASE_DEC, NULL, 0x0,
        "TDMA Cycle", HFILL }},

    /* TDMA ack ack conf */

    { &hf_rtnet_tdma_msg_ack_ack_conf_station,
      { "Station",
        "rtnet.tdma.msg.ack_ack_conf.station",
        FT_UINT8, BASE_DEC, NULL, 0x0,
        "TDMA Station", HFILL }},

    { &hf_rtnet_tdma_msg_ack_ack_conf_padding,
      { "Padding",
        "rtnet.tdma.msg.ack_ack_conf.padding",
        FT_BYTES, BASE_HEX, NULL, 0x0,
        "TDMA Padding", HFILL }},

    /* TDMA request test */

    { &hf_rtnet_tdma_msg_request_test_counter,
      { "Counter",
        "rtnet.tdma.msg.request_test.counter",
        FT_UINT32, BASE_DEC, NULL, 0x0,
        "TDMA Counter", HFILL }},

#ifdef G_HAVE_GINT64
    { &hf_rtnet_tdma_msg_request_test_tx,
      { "TX",
        "rtnet.tdma.msg.request_test.tx",
        FT_UINT64, BASE_DEC, NULL, 0x0,
        "TDMA TX", HFILL }},
#else
    { &hf_rtnet_tdma_msg_request_test_tx,
      { "TX",
        "rtnet.tdma.msg.request_test.tx",
        FT_BYTES, BASE_HEX, NULL, 0x0,
        "TDMA TX", HFILL }},
#endif

    /* TDMA ack test */

    { &hf_rtnet_tdma_msg_ack_test_counter,
      { "Counter",
        "rtnet.tdma.msg.ack_test.counter",
        FT_UINT32, BASE_DEC, NULL, 0x0,
        "TDMA Counter", HFILL }},

#ifdef G_HAVE_GINT64
    { &hf_rtnet_tdma_msg_ack_test_tx,
      { "TX",
        "rtnet.tdma.msg.ack_test.tx",
        FT_UINT64, BASE_DEC, NULL, 0x0,
        "TDMA TX", HFILL }},
#else
    { &hf_rtnet_tdma_msg_ack_test_tx,
      { "TX",
        "rtnet.tdma.msg.ack_test.tx",
        FT_BYTES, BASE_HEX, NULL, 0x0,
        "TDMA TX", HFILL }},
#endif

    /* TDMA ack test */
    
    { &hf_rtnet_tdma_msg_request_change_offset_offset,
      { "Offset",
        "rtnet.tdma.msg.request_change_offset.offset",
        FT_UINT32, BASE_DEC, NULL, 0x0,
        "TDMA Offset", HFILL }},

    /* TDMA start of frame */


#ifdef G_HAVE_GINT64
    { &hf_rtnet_tdma_msg_start_of_frame_timestamp,
      { "Timestamp",
        "rtnet.tdma.msg.start_of_frame.timestamp",
        FT_UINT64, BASE_DEC, NULL, 0x0,
        "TDMA Timestamp", HFILL }},
#else
    { &hf_rtnet_tdma_msg_start_of_frame_timestamp,
      { "Timestamp",
        "rtnet.tdma.msg.start_of_frame.timestamp",
        FT_BYTES, BASE_HEX, NULL, 0x0,
        "TDMA Timestamp", HFILL }},
#endif

    /* TDMA station list */

    { &hf_rtnet_tdma_msg_station_list_nr_stations,
      { "Nr. Stations",
        "rtnet.tdma.msg.station_list.nr_stations",
        FT_UINT8, BASE_DEC, NULL, 0x0,
        "TDMA Nr. Stations", HFILL }},

    { &hf_rtnet_tdma_msg_station_list_nr,
      { "Nr.",
        "rtnet.tdma.msg.station_list.nr",
        FT_UINT8, BASE_DEC, NULL, 0x0,
        "TDMA Station Number", HFILL }},

    { &hf_rtnet_tdma_msg_station_list_ip,
      { "IP",
        "rtnet.tdma.msg.station_list.ip",
        FT_IPv4, BASE_DEC, NULL, 0x0,
        "TDMA Station IP", HFILL }},

    { &hf_rtnet_tdma_msg_station_list_padding,
      { "Padding",
        "rtnet.tdma.msg.station_list.padding",
        FT_BYTES, BASE_HEX, NULL, 0x0,
        "TDMA Padding", HFILL }}

  };

  static gint *ett[] = {
    &ett_rtnet,
  };

  proto_rtnet = proto_register_protocol("RTNET",
				       "RTNET","rtnet");
  proto_register_field_array(proto_rtnet,hf,array_length(hf));
  proto_register_subtree_array(ett,array_length(ett));

}


void
proto_register_rtcfg(void) {
  static hf_register_info hf[] = {
    { &hf_rtcfg_vers_id,
      { "Version and ID",
        "rtcfg.vers_id",
        FT_UINT8, BASE_HEX, NULL, 0x0,
        "RTcfg Version and ID", HFILL }},

    { &hf_rtcfg_vers,
      { "Version",
        "rtcfg.vers",
        FT_UINT8, BASE_DEC, NULL, 0xe0,
        "RTcfg Version", HFILL }},

    { &hf_rtcfg_id,
      { "ID",
        "rtcfg.id",
        FT_UINT8, BASE_HEX, VALS(rtcfg_msg_vals), 0x1f,
        "RTcfg ID", HFILL }},

    { &hf_rtcfg_address_type,
      { "Address Type",
        "rtcfg.address_type",
        FT_UINT8, BASE_DEC, VALS(rtcfg_address_type_vals), 0x00,
        "RTcfg Address Type", HFILL }},

    { &hf_rtcfg_client_ip_address,
      { "Client IP Address",
        "rtcfg.client_ip_address",
        FT_IPv4, BASE_DEC, NULL, 0x0,
        "RTcfg Client IP Address", HFILL }},

    { &hf_rtcfg_server_ip_address,
      { "Server IP Address",
        "rtcfg.server_ip_address",
        FT_IPv4, BASE_DEC, NULL, 0x0,
        "RTcfg Server IP Address", HFILL }},

    { &hf_rtcfg_burst_rate,
      { "Stage 2 Burst Rate",
        "rtcfg.burst_rate",
        FT_UINT8, BASE_DEC, NULL, 0x00,
        "RTcfg Stage 2 Burst Rate", HFILL }},

    { &hf_rtcfg_s1_config_length,
      { "Stage 1 Config Length",
        "rtcfg.s1_config_length",
        FT_UINT16, BASE_DEC, NULL, 0x00,
        "RTcfg Stage 1 Config Length", HFILL }},

    { &hf_rtcfg_config_data,
      { "Config Data",
        "rtcfg.config_data",
        FT_BYTES, BASE_DEC, NULL, 0x00,
        "RTcfg Config Data", HFILL }},

    { &hf_rtcfg_padding,
      { "Padding",
        "rtcfg.padding",
        FT_UINT8, BASE_DEC, NULL, 0x00,
        "RTcfg Padding", HFILL }},

    { &hf_rtcfg_client_flags,
      { "Flags",
        "rtcfg.client_flags",
        FT_UINT8, BASE_HEX, NULL, 0x00,
        "RTcfg Client Flags", HFILL }},

    { &hf_rtcfg_client_flags_available,
      { "Req. Available",
        "rtcfg.client_flags.available",
        FT_UINT8, BASE_DEC, NULL, 0x01,
        "Request Available", HFILL }},

    { &hf_rtcfg_client_flags_ready,
      { "Client Ready",
        "rtcfg.client_flags.ready",
        FT_UINT8, BASE_DEC, NULL, 0x02,
        "Client Ready", HFILL }},

    { &hf_rtcfg_client_flags_res,
      { "Reserved",
        "rtcfg.client_flags.res",
        FT_UINT8, BASE_HEX, NULL, 0xfc,
        "Reserved", HFILL }},

    { &hf_rtcfg_server_flags,
      { "Flags",
        "rtcfg.server_flags",
        FT_UINT8, BASE_HEX, NULL, 0x00,
        "RTcfg Server Flags", HFILL }},

    { &hf_rtcfg_server_flags_res0,
      { "Reserved",
        "rtcfg.server_flags.res0",
        FT_UINT8, BASE_HEX, NULL, 0x01,
        "Reserved", HFILL }},

    { &hf_rtcfg_server_flags_ready,
      { "Server Ready",
        "rtcfg.server_flags.ready",
        FT_UINT8, BASE_DEC, NULL, 0x02,
        "Server Ready", HFILL }},

    { &hf_rtcfg_server_flags_res2,
      { "Reserved",
        "rtcfg.server_flags.res2",
        FT_UINT8, BASE_HEX, NULL, 0xfc,
        "Reserved", HFILL }},
        
    { &hf_rtcfg_active_stations,
      { "Active Stations",
        "rtcfg.active_stations",
        FT_UINT32, BASE_DEC, NULL, 0x00,
        "RTcfg Active Stations", HFILL }},

    { &hf_rtcfg_heartbeat_period,
      { "Heartbeat Period",
        "rtcfg.hearbeat_period",
        FT_UINT16, BASE_DEC, NULL, 0x00,
        "RTcfg Heartbeat Period", HFILL }},

    { &hf_rtcfg_s2_config_length,
      { "Stage 2 Config Length",
        "rtcfg.s2_config_length",
        FT_UINT32, BASE_DEC, NULL, 0x00,
        "RTcfg Stage 2 Config Length", HFILL }},

    { &hf_rtcfg_config_offset,
      { "Config Offset",
        "rtcfg.config_offset",
        FT_UINT32, BASE_DEC, NULL, 0x00,
        "RTcfg Config Offset", HFILL }},

    { &hf_rtcfg_ack_length,
      { "Ack Length",
        "rtcfg.ack_length",
        FT_UINT32, BASE_DEC, NULL, 0x00,
        "RTcfg Ack Length", HFILL }}

  };

  static gint *ett[] = {
    &ett_rtcfg,
  };

  proto_rtcfg = proto_register_protocol("RTcfg","RTcfg","rtcfg");
  proto_register_field_array(proto_rtcfg,hf,array_length(hf));
  proto_register_subtree_array(ett,array_length(ett));
}

/* The registration hand-off routing */

void
proto_reg_handoff_rtnet(void) {
  static int rtnet_initialized = FALSE;
  static dissector_handle_t rtnet_handle;

  if( !rtnet_initialized ){
    rtnet_handle = create_dissector_handle(dissect_rtnet, proto_rtnet);
    rtnet_initialized = TRUE;
  } else {
    dissector_delete("ethertype",ETHERTYPE_RTNET, rtnet_handle);
  }

  dissector_add("ethertype", ETHERTYPE_RTNET, rtnet_handle);


  ip_handle = find_dissector("ip");
  arp_handle = find_dissector("arp");
}

void
proto_reg_handoff_rtcfg(void) {
  static int rtcfg_initialized = FALSE;
  static dissector_handle_t rtcfg_handle;

  if( !rtcfg_initialized ){
    rtcfg_handle = create_dissector_handle(dissect_rtcfg, proto_rtcfg);
    rtcfg_initialized = TRUE;
  } else {
    dissector_delete("ethertype",ETHERTYPE_RTCFG, rtcfg_handle);
  }

  dissector_add("ethertype", ETHERTYPE_RTCFG, rtcfg_handle);
}

/* Start the functions we need for the plugin stuff */

#ifndef ENABLE_STATIC

G_MODULE_EXPORT void
plugin_reg_handoff(void){
  proto_reg_handoff_rtnet();
  proto_reg_handoff_rtcfg();
}

G_MODULE_EXPORT void
plugin_init(plugin_address_table_t *pat
#ifndef PLUGINS_NEED_ADDRESS_TABLE
_U_
#endif
){
  /* initialise the table of pointers needed in Win32 DLLs */
  plugin_address_table_init(pat);
  /* register the new protocol, protocol fields, and subtrees */
  if (proto_rtnet == -1) { /* execute protocol initialization only once */
    proto_register_rtnet();
  }
  if (proto_rtcfg == -1) { /* execute protocol initialization only once */
    proto_register_rtcfg();
  }
}

#endif

/* End the functions we need for plugin stuff */

