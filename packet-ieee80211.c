/* packet-ieee80211.c
 * Routines for Wireless LAN (IEEE 802.11) dissection
 * Copyright 2000, Axis Communications AB 
 * Inquiries/bugreports should be sent to Johan.Jorgensen@axis.com
 *
 * $Id: packet-ieee80211.c,v 1.21 2001/06/10 07:40:39 guy Exp $
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
 *
 * Credits:
 * 
 * The following people helped me by pointing out bugs etc. Thank you!
 *
 * Marco Molteni
 * Lena-Marie Nilsson     
 * Magnus Hultman-Persson
 *
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif

#ifdef NEED_SNPRINTF_H
# ifdef HAVE_STDARG_H
#  include <stdarg.h>
# else
#  include <varargs.h>
# endif
# include "snprintf.h"
#endif

#include <string.h>
#include <glib.h>
#include "bitswap.h"
#include "proto.h"
#include "packet.h"
#include "resolv.h"
#include "packet-llc.h"
#include "packet-ieee80211.h"
#include "etypes.h"

/* ************************************************************************* */
/*                          Miscellaneous Constants                          */
/* ************************************************************************* */
#define SHORT_STR 128
#define MGT_FRAME_LEN 24

/* ************************************************************************* */
/*  Define some very useful macros that are used to analyze frame types etc. */
/* ************************************************************************* */
#define COMPOSE_FRAME_TYPE(x) (((x & 0x0C)<< 2)+((x & 0xF0) >> 4))	/* Create key to (sub)type */
#define COOK_PROT_VERSION(x)  ((x & 0x3))
#define COOK_FRAME_TYPE(x)    ((x & 0xC) >> 2)
#define COOK_FRAME_SUBTYPE(x) ((x & 0xF0) >> 4)
#define COOK_ADDR_SELECTOR(x) (((x & 0x200)) && ((x & 0x100)))
#define COOK_ASSOC_ID(x)      ((x & 0x3FFF))
#define COOK_FRAGMENT_NUMBER(x) (x & 0x000F)
#define COOK_SEQUENCE_NUMBER(x) ((x & 0xFFF0) >> 4)
#define COOK_FLAGS(x)           ((x & 0xFF00) >> 8)
#define COOK_DS_STATUS(x)       (x & 0x3)
#define COOK_WEP_IV(x)        (x & 0xFFFFFF)
#define COOK_WEP_KEY(x)       ((x & 0xC0000000) >> 30)
#define COL_SHOW_INFO(fd,info) if (check_col(fd,COL_INFO)) \
col_add_str(fd,COL_INFO,info);
#define COL_SHOW_INFO_CONST(fd,info) if (check_col(fd,COL_INFO)) \
col_set_str(fd,COL_INFO,info);

#define IS_TO_DS(x)            ((x & 0x1))
#define IS_FROM_DS(x)          ((x & 0x2))
#define HAVE_FRAGMENTS(x)      ((x & 0x4))
#define IS_RETRY(x)            ((x & 0x8))
#define POWER_MGT_STATUS(x)    ((x & 0x10))
#define HAS_MORE_DATA(x)       ((x & 0x20))
#define IS_WEP(x)              ((x & 0x40))
#define IS_STRICTLY_ORDERED(x) ((x & 0x80))

#define MGT_RESERVED_RANGE(x) (((x>=0x06)&&(x<=0x07))||((x>=0x0D)&&(x<=0x0F)))
#define CTRL_RESERVED_RANGE(x) ((x>=0x10)&&(x<=0x19))
#define DATA_RESERVED_RANGE(x) ((x>=0x28)&&(x<=0x2f))
#define SPEC_RESERVED_RANGE(x) ((x>=0x30)&&(x<=0x3f))


/* ************************************************************************* */
/*              Constants used to identify cooked frame types                */
/* ************************************************************************* */
#define MGT_FRAME            0x00	/* Frame type is management */
#define CONTROL_FRAME        0x01	/* Frame type is control */
#define DATA_FRAME           0x02	/* Frame type is Data */

#define DATA_SHORT_HDR_LEN     24
#define DATA_LONG_HDR_LEN      30
#define MGT_FRAME_HDR_LEN      24	/* Length of Managment frame-headers */
#define CTLR
#define MGT_ASSOC_REQ        0x00	/* Management - association request        */
#define MGT_ASSOC_RESP       0x01	/* Management - association response       */
#define MGT_REASSOC_REQ      0x02	/* Management - reassociation request      */
#define MGT_REASSOC_RESP     0x03	/* Management - reassociation response     */
#define MGT_PROBE_REQ        0x04	/* Management - Probe request              */
#define MGT_PROBE_RESP       0x05	/* Management - Probe response             */
#define MGT_BEACON           0x08	/* Management - Beacon frame               */
#define MGT_ATIM             0x09	/* Management - ATIM                       */
#define MGT_DISASS           0x0A	/* Management - Disassociation             */
#define MGT_AUTHENTICATION   0x0B	/* Management - Authentication             */
#define MGT_DEAUTHENTICATION 0x0C	/* Management - Deauthentication           */

#define CTRL_PS_POLL         0x1A	/* Control - power-save poll               */
#define CTRL_RTS             0x1B	/* Control - request to send               */
#define CTRL_CTS             0x1C	/* Control - clear to send                 */
#define CTRL_ACKNOWLEDGEMENT 0x1D	/* Control - acknowledgement               */
#define CTRL_CFP_END         0x1E	/* Control - contention-free period end    */
#define CTRL_CFP_ENDACK      0x1F	/* Control - contention-free period end/ack */

#define DATA                 0x20	/* Data - Data                             */
#define DATA_CF_ACK          0x21	/* Data - Data + CF acknowledge            */
#define DATA_CF_POLL         0x22	/* Data - Data + CF poll                   */
#define DATA_CF_ACK_POLL     0x23	/* Data - Data + CF acknowledge + CF poll  */
#define DATA_NULL_FUNCTION   0x24	/* Data - Null function (no data)          */
#define DATA_CF_ACK_NOD      0x25	/* Data - CF ack (no data)                 */
#define DATA_CF_POLL_NOD     0x26       /* Data - Data + CF poll (No data)         */
#define DATA_CF_ACK_POLL_NOD 0x27	/* Data - CF ack + CF poll (no data)       */

#define DATA_ADDR_T1         0x0000
#define DATA_ADDR_T2         0x0100
#define DATA_ADDR_T3         0x0200
#define DATA_ADDR_T4         0x0300


/* ************************************************************************* */
/*          Macros used to extract information about fixed fields            */
/* ************************************************************************* */
#define ESS_SET(x) ((x & 0x0001))
#define IBSS_SET(x) ((x & 0x0002))



/* ************************************************************************* */
/*        Logical field codes (dissector's encoding of fixed fields)         */
/* ************************************************************************* */
#define FIELD_TIMESTAMP       0x01	/* 64-bit timestamp                       */
#define FIELD_BEACON_INTERVAL 0x02	/* 16-bit beacon interval                 */
#define FIELD_CAP_INFO        0x03	/* Add capability information tree        */
#define FIELD_AUTH_ALG        0x04	/* Authentication algorithm used          */
#define FIELD_AUTH_TRANS_SEQ  0x05	/* Authentication sequence number         */
#define FIELD_CURRENT_AP_ADDR 0x06
#define FIELD_LISTEN_IVAL     0x07
#define FIELD_REASON_CODE     0x08
#define FIELD_ASSOC_ID        0x09
#define FIELD_STATUS_CODE     0x0A

/* ************************************************************************* */
/*        Logical field codes (IEEE 802.11 encoding of tags)                 */
/* ************************************************************************* */
#define TAG_SSID           0x00
#define TAG_SUPP_RATES     0x01
#define TAG_FH_PARAMETER   0x02
#define TAG_DS_PARAMETER   0x03
#define TAG_CF_PARAMETER   0x04
#define TAG_TIM            0x05
#define TAG_IBSS_PARAMETER 0x06
#define TAG_CHALLENGE_TEXT 0x10


static int proto_wlan = -1;
/* ************************************************************************* */
/*                Header field info values for FC-field                      */
/* ************************************************************************* */
static int hf_fc_field = -1;
static int hf_fc_proto_version = -1;
static int hf_fc_frame_type = -1;
static int hf_fc_frame_subtype = -1;

static int hf_fc_flags = -1;
static int hf_fc_to_ds = -1;
static int hf_fc_from_ds = -1;
static int hf_fc_data_ds = -1;

static int hf_fc_more_frag = -1;
static int hf_fc_retry = -1;
static int hf_fc_pwr_mgt = -1;
static int hf_fc_more_data = -1;
static int hf_fc_wep = -1;
static int hf_fc_order = -1;


/* ************************************************************************* */
/*                   Header values for Duration/ID field                     */
/* ************************************************************************* */
static int hf_did_duration = -1;
static int hf_assoc_id = -1;


/* ************************************************************************* */
/*         Header values for different address-fields (all 4 of them)        */
/* ************************************************************************* */
static int hf_addr_da = -1;	/* Destination address subfield */
static int hf_addr_sa = -1;	/* Source address subfield */
static int hf_addr_ra = -1;	/* Receiver address subfield */
static int hf_addr_ta = -1;	/* Transmitter address subfield */
static int hf_addr_bssid = -1;	/* address is bssid */



/* ************************************************************************* */
/*                Header values for sequence number field                    */
/* ************************************************************************* */
static int hf_frag_number = -1;
static int hf_seq_number = -1;

/* ************************************************************************* */
/*                   Header values for Frame Check field                     */
/* ************************************************************************* */
static int hf_fcs = -1;


/* ************************************************************************* */
/*                      Fixed fields found in mgt frames                     */
/* ************************************************************************* */
static int ff_auth_alg = -1;	/* Authentication algorithm field            */
static int ff_auth_seq = -1;	/* Authentication transaction sequence       */
static int ff_current_ap = -1;	/* Current AP MAC address                    */
static int ff_listen_ival = -1;	/* Listen interval fixed field               */
static int ff_timestamp = -1;	/* 64 bit timestamp                          */
static int ff_beacon_interval = -1;	/* 16 bit Beacon interval            */
static int ff_assoc_id = -1;	/* 16 bit AID field                          */
static int ff_reason = -1;	/* 16 bit reason code                        */
static int ff_status_code = -1;	/* Status code                               */

/* ************************************************************************* */
/*            Flags found in the capability field (fixed field)              */
/* ************************************************************************* */
static int ff_capture = -1;
static int ff_cf_sta_poll = -1; /* CF pollable status for a STA            */
static int ff_cf_ap_poll = -1;	/* CF pollable status for an AP            */
static int ff_cf_ess = -1;
static int ff_cf_ibss = -1;
static int ff_cf_privacy = -1;
static int ff_cf_preamble = -1;
static int ff_cf_pbcc = -1;
static int ff_cf_agility = -1;

/* ************************************************************************* */
/*                       Tagged value format fields                          */
/* ************************************************************************* */
static int tag_number = -1;
static int tag_length = -1;
static int tag_interpretation = -1;



static int hf_fixed_parameters = -1;	/* Protocol payload for management frames */
static int hf_tagged_parameters = -1;	/* Fixed payload item */
static int hf_wep_parameters = -1;
static int hf_wep_iv = -1;
static int hf_wep_key = -1;
static int hf_wep_crc = -1;

/* ************************************************************************* */
/*                               Protocol trees                              */
/* ************************************************************************* */
static gint ett_80211 = -1;
static gint ett_proto_flags = -1;
static gint ett_cap_tree = -1;
static gint ett_fc_tree = -1;
static gint ett_fixed_parameters = -1;
static gint ett_tagged_parameters = -1;
static gint ett_wep_parameters = -1;

static dissector_handle_t llc_handle;

/* ************************************************************************* */
/*            Return the length of the current header (in bytes)             */
/* ************************************************************************* */
int
find_header_length (const u_char * pd, int offset)
{
  guint16 frame_control;

  frame_control = pntohs (pd);
  return ((IS_FROM_DS(frame_control))
	  && (IS_TO_DS(frame_control))) ? 30 : 24;
}


/* ************************************************************************* */
/*          This is the capture function used to update packet counts        */
/* ************************************************************************* */
void
capture_ieee80211 (const u_char * pd, int offset, packet_counts * ld)
{
  guint16 fcf, hdr_length;

  fcf = pletohs (&pd[0]);


  hdr_length = MGT_FRAME_HDR_LEN;	/* Set the header length of the frame */

  if (IS_WEP(COOK_FLAGS(fcf)))
    {
      ld->other++;
      return;
    }

  switch (COMPOSE_FRAME_TYPE (fcf))
    {

    case DATA:			/* We got a data frame */
      hdr_length = find_header_length (pd, offset);
      capture_llc (pd, offset + hdr_length, ld);
      break;

    case DATA_CF_ACK:		/* Data with ACK */
      hdr_length = find_header_length (pd, offset);
      capture_llc (pd, offset + hdr_length, ld);
      break;

    case DATA_CF_POLL:
      hdr_length = find_header_length (pd, offset);
      capture_llc (pd, offset + hdr_length, ld);
      break;

    case DATA_CF_ACK_POLL:
      hdr_length = find_header_length (pd, offset);
      capture_llc (pd, offset + hdr_length, ld);
      break;

    default:
      ld->other++;
      break;
    }
}



/* ************************************************************************* */
/*          Add the subtree used to store the fixed parameters               */
/* ************************************************************************* */
static proto_tree *
get_fixed_parameter_tree (proto_tree * tree, tvbuff_t *tvb, int start, int size)
{
  proto_item *fixed_fields;
  fixed_fields =
    proto_tree_add_uint_format (tree, hf_fixed_parameters, tvb, start,
				size, size, "Fixed parameters (%d bytes)",
				size);

  return proto_item_add_subtree (fixed_fields, ett_fixed_parameters);
}


/* ************************************************************************* */
/*            Add the subtree used to store tagged parameters                */
/* ************************************************************************* */
static proto_tree *
get_tagged_parameter_tree (proto_tree * tree, tvbuff_t *tvb, int start, int size)
{
  proto_item *tagged_fields;

  tagged_fields = proto_tree_add_uint_format (tree, hf_tagged_parameters,
					      tvb,
					      start,
					      size,
					      size,
					      "Tagged parameters (%d bytes)",
					      size);

  return proto_item_add_subtree (tagged_fields, ett_tagged_parameters);
}


/* ************************************************************************* */
/*            Add the subtree used to store WEP parameters                   */
/* ************************************************************************* */
void
get_wep_parameter_tree (proto_tree * tree, tvbuff_t *tvb, int start, int size)
{
  proto_item *wep_fields;
  proto_tree *wep_tree;
  int crc_offset = size - 4;

  wep_fields = proto_tree_add_string_format(tree, hf_wep_parameters, tvb,
					    0, 4, 0, "WEP parameters");

  wep_tree = proto_item_add_subtree (wep_fields, ett_wep_parameters);
  proto_tree_add_uint (wep_tree, hf_wep_iv, tvb, start, 3,
		       COOK_WEP_IV (tvb_get_letohl (tvb, start)));

  proto_tree_add_uint (wep_tree, hf_wep_key, tvb, start + 3, 1,
		       COOK_WEP_KEY (tvb_get_letohl (tvb, start)));

  if (tvb_bytes_exist(tvb, start + crc_offset, 4))
    proto_tree_add_uint (wep_tree, hf_wep_crc, tvb, start + crc_offset, 4,
			 tvb_get_ntohl (tvb, start + crc_offset));
}

/* ************************************************************************* */
/*              Dissect and add fixed mgmt fields to protocol tree           */
/* ************************************************************************* */
static void
add_fixed_field (proto_tree * tree, tvbuff_t * tvb, int offset, int lfcode)
{
  const guint8 *dataptr;
  char out_buff[SHORT_STR];
  guint16 *temp16;
  proto_item *cap_item;
  static proto_tree *cap_tree;
  double temp_double;

  switch (lfcode)
    {
    case FIELD_TIMESTAMP:
      dataptr = tvb_get_ptr (tvb, offset, 8);
      memset (out_buff, 0, SHORT_STR);
      snprintf (out_buff, SHORT_STR, "0x%02X%02X%02X%02X%02X%02X%02X%02X",
		dataptr[7],
		dataptr[6],
		dataptr[5],
		dataptr[4],
		dataptr[3],
		dataptr[2],
		dataptr[1],
		dataptr[0]);

      proto_tree_add_string (tree, ff_timestamp, tvb, offset, 8, out_buff);
      break;


    case FIELD_BEACON_INTERVAL:
      dataptr = tvb_get_ptr (tvb, offset, 2);
      temp_double = ((double) *((guint16 *) dataptr));
      temp_double = temp_double * 1024 / 1000000;
      proto_tree_add_double_format (tree, ff_beacon_interval, tvb, offset, 2,
				    temp_double,"Beacon Interval: %f [Seconds]",
				    temp_double);
      break;


    case FIELD_CAP_INFO:
      dataptr = tvb_get_ptr (tvb, offset, 2);
      temp16 = (guint16 *) dataptr;

      cap_item = proto_tree_add_uint_format (tree, ff_capture, 
					     tvb, offset, 2,
					     pletohs (temp16),
					     "Capability Information: %04X",
					     pletohs (temp16));
      cap_tree = proto_item_add_subtree (cap_item, ett_cap_tree);
      proto_tree_add_boolean (cap_tree, ff_cf_ess, tvb, offset, 1,
			      pletohs (temp16));
      proto_tree_add_boolean (cap_tree, ff_cf_ibss, tvb, offset, 1,
			      pletohs (temp16));
      proto_tree_add_boolean (cap_tree, ff_cf_privacy, tvb, offset, 1,
			      pletohs (temp16));
      proto_tree_add_boolean (cap_tree, ff_cf_preamble, tvb, offset, 1,
			      pletohs (temp16));
      proto_tree_add_boolean (cap_tree, ff_cf_pbcc, tvb, offset, 1,
			      pletohs (temp16));
      proto_tree_add_boolean (cap_tree, ff_cf_agility, tvb, offset, 1,
			      pletohs (temp16));
      if (ESS_SET (pletohs (temp16)) != 0)	/* This is an AP */
	proto_tree_add_uint (cap_tree, ff_cf_ap_poll, tvb, offset, 2,
			     ((pletohs (temp16) & 0xC) >> 2));

      else			/* This is a STA */
	proto_tree_add_uint (cap_tree, ff_cf_sta_poll, tvb, offset, 2,
			     ((pletohs (temp16) & 0xC) >> 2));
      break;


    case FIELD_AUTH_ALG:
      dataptr = tvb_get_ptr (tvb, offset, 2);
      temp16 =(guint16 *) dataptr;
      proto_tree_add_uint (tree, ff_auth_alg, tvb, offset, 2,
			   pletohs (temp16));
      break;


    case FIELD_AUTH_TRANS_SEQ:
      dataptr = tvb_get_ptr (tvb, offset, 2);
      temp16 = (guint16 *)dataptr;
      proto_tree_add_uint (tree, ff_auth_seq, tvb, offset, 2,
			   pletohs (temp16));
      break;


    case FIELD_CURRENT_AP_ADDR:
      dataptr = tvb_get_ptr (tvb, offset, 6);
      proto_tree_add_ether (tree, ff_current_ap, tvb, offset, 6, dataptr);
      break;


    case FIELD_LISTEN_IVAL:
      dataptr = tvb_get_ptr (tvb, offset, 2);
      temp16 = (guint16 *) dataptr;
      proto_tree_add_uint (tree, ff_listen_ival, tvb, offset, 2,
			   pletohs (temp16));
      break;


    case FIELD_REASON_CODE:
      dataptr = tvb_get_ptr (tvb, offset, 2);
      temp16 = (guint16 *) dataptr;
      proto_tree_add_uint (tree, ff_reason, tvb, offset, 2, pletohs (temp16));
      break;


    case FIELD_ASSOC_ID:
      dataptr = tvb_get_ptr (tvb, offset, 2);
      temp16 = (guint16 *) dataptr;
      proto_tree_add_uint (tree, ff_assoc_id, tvb, offset, 2, pletohs (temp16));
      break;

    case FIELD_STATUS_CODE:
      dataptr = tvb_get_ptr (tvb, offset, 2);
      temp16 = (guint16 *) dataptr;
      proto_tree_add_uint (tree, ff_status_code, tvb, offset, 2,
			   pletohs (temp16));
      break;
    }
}


/* ************************************************************************* */
/*           Dissect and add tagged (optional) fields to proto tree          */
/* ************************************************************************* */
static int
add_tagged_field (proto_tree * tree, tvbuff_t * tvb, int offset)
{
  const guint8 *tag_info_ptr;
  const guint8 *tag_data_ptr;
  guint32 tag_no, tag_len;
  int i, n;
  char out_buff[SHORT_STR];


  tag_info_ptr = tvb_get_ptr (tvb, offset, 2);
  tag_no = tag_info_ptr[0];
  tag_len = tag_info_ptr[1];

  tag_data_ptr = tvb_get_ptr (tvb, offset + 2, tag_len);


  if ((tag_no >= 17) && (tag_no <= 31))
    {				/* Reserved for challenge text */
      proto_tree_add_uint_format (tree, tag_number, tvb, offset, 1, tag_no,
				  "Tag Number: %u (Reserved for challenge text)",
				  tag_no);

      proto_tree_add_uint (tree, tag_length, tvb, offset + 1, 1, tag_len);
      proto_tree_add_string (tree, tag_interpretation, tvb, offset + 2,
			     tag_len, "Not interpreted");
      return (int) tag_len + 2;
    }

  /* Next See if tag is reserved - if true, skip it! */
  if (((tag_no >= 7) && (tag_no <= 15))
      || ((tag_no >= 32) && (tag_no <= 255)))
    {
      proto_tree_add_uint_format (tree, tag_number, tvb, offset, 1, tag_no,
				  "Tag Number: %u (Reserved tag number)",
				  tag_no);

      proto_tree_add_uint (tree, tag_length, tvb, offset + 1, 1, tag_len);

      proto_tree_add_string (tree, tag_interpretation, tvb, offset + 2,
			     tag_len, "Not interpreted");
      return (int) tag_len + 2;
    }


  switch (tag_info_ptr[0])
    {


    case TAG_SSID:
      proto_tree_add_uint_format (tree, tag_number, tvb, offset, 1, tag_no,
				  "Tag Number: %u (SSID parameter set)",
				  tag_no);

      proto_tree_add_uint (tree, tag_length, tvb, offset + 1, 1, tag_len);

      memset (out_buff, 0, SHORT_STR);

      memcpy (out_buff, tag_data_ptr, (size_t) tag_len);
      out_buff[tag_len + 1] = 0;

      proto_tree_add_string (tree, tag_interpretation, tvb, offset + 2,
			     tag_len, out_buff);
      break;



    case TAG_SUPP_RATES:
      proto_tree_add_uint_format (tree, tag_number, tvb, offset, 1, tag_no,
				  "Tag Number: %u (Supported Rates)", tag_no);

      proto_tree_add_uint (tree, tag_length, tvb, offset + 1, 1, tag_len);

      memset (out_buff, 0, SHORT_STR);
      strcpy (out_buff, "Supported rates: ");
      n = strlen (out_buff);

      for (i = 0; i < tag_len; i++)
	{
	    n += snprintf (out_buff + n, SHORT_STR - n, "%2.1f%s ",
			   (tag_data_ptr[i] & 0x7F) * 0.5,
			   (tag_data_ptr[i] & 0x80) ? "(B)" : "");
	}
      snprintf (out_buff + n, SHORT_STR - n, "[Mbit/sec]");

      proto_tree_add_string (tree, tag_interpretation, tvb, offset + 2,
			     tag_len, out_buff);
      break;



    case TAG_FH_PARAMETER:
      proto_tree_add_uint_format (tree, tag_number, tvb, offset, 1, tag_no,
				  "Tag Number: %u (FH Parameter set)",
				  tag_no);

      proto_tree_add_uint (tree, tag_length, tvb, offset + 1, 1, tag_len);
      memset (out_buff, 0, SHORT_STR);

      snprintf (out_buff, SHORT_STR,
		"Dwell time 0x%04X, Hop Set %2d, Hop Pattern %2d, "
		"Hop Index %2d", pletohs (tag_data_ptr), tag_data_ptr[2],
		tag_data_ptr[3], tag_data_ptr[4]);

      proto_tree_add_string (tree, tag_interpretation, tvb, offset + 2,
			     tag_len, out_buff);
      break;



    case TAG_DS_PARAMETER:
      proto_tree_add_uint_format (tree, tag_number, tvb, offset, 1, tag_no,
				  "Tag Number: %u (DS Parameter set)",
				  tag_no);

      proto_tree_add_uint (tree, tag_length, tvb, offset + 1, 1, tag_len);
      memset (out_buff, 0, SHORT_STR);

      snprintf (out_buff, SHORT_STR, "Current Channel: %u", tag_data_ptr[0]);
      proto_tree_add_string (tree, tag_interpretation, tvb, offset + 2,
			     tag_len, out_buff);
      break;


    case TAG_CF_PARAMETER:
      proto_tree_add_uint_format (tree, tag_number, tvb, offset, 1, tag_no,
				  "Tag Number: %u (CF Parameter set)",
				  tag_no);

      proto_tree_add_uint (tree, tag_length, tvb, offset + 1, 1, tag_len);
      memset (out_buff, 0, SHORT_STR);

      snprintf (out_buff, SHORT_STR,
		"CFP count %u, CFP period %u, CFP max duration %u, "
		"CFP Remaining %u", tag_data_ptr[0], tag_data_ptr[1],
		pletohs (tag_data_ptr + 2), pletohs (tag_data_ptr + 4));

      proto_tree_add_string (tree, tag_interpretation, tvb, offset + 2,
			     tag_len, out_buff);
      break;


    case TAG_TIM:
      proto_tree_add_uint_format (tree, tag_number, tvb, offset, 1, tag_no,
				  "Tag Number: %u ((TIM) Traffic Indication Map)",
				  tag_no);

      proto_tree_add_uint (tree, tag_length, tvb, offset + 1, 1, tag_len);
      memset (out_buff, 0, SHORT_STR);
      snprintf (out_buff, SHORT_STR,
		"DTIM count %u, DTIM period %u, Bitmap control 0x%X, "
		"(Bitmap suppressed)", tag_data_ptr[0], tag_data_ptr[1],
		tag_data_ptr[2]);
      proto_tree_add_string (tree, tag_interpretation, tvb, offset + 2,
			     tag_len, out_buff);
      break;



    case TAG_IBSS_PARAMETER:
      proto_tree_add_uint_format (tree, tag_number, tvb, offset, 1, tag_no,
				  "Tag Number: %u (IBSS Parameter set)",
				  tag_no);

      proto_tree_add_uint (tree, tag_length, tvb, offset + 1, 1, tag_len);
      memset (out_buff, 0, SHORT_STR);
      snprintf (out_buff, SHORT_STR, "ATIM window 0x%X",
		pletohs (tag_data_ptr));

      proto_tree_add_string (tree, tag_interpretation, tvb, offset + 2,
			     tag_len, out_buff);
      break;



    case TAG_CHALLENGE_TEXT:
      proto_tree_add_uint_format (tree, tag_number, tvb, offset, 1, tag_no,
				  "Tag Number: %u (Challenge text)", tag_no);

      proto_tree_add_uint (tree, tag_length, tvb, offset + 1, 1, tag_len);
      memset (out_buff, 0, SHORT_STR);
      snprintf (out_buff, SHORT_STR, "Challenge text: %.47s", tag_data_ptr);
      proto_tree_add_string (tree, tag_interpretation, tvb, offset, tag_len,
			     out_buff);

      break;

    default:
      return 0;
    }

  return tag_len + 2;
}

static void
set_src_addr_cols(packet_info *pinfo, const guint8 *addr, char *type)
{
  if (check_col(pinfo->fd, COL_RES_DL_SRC))
    col_add_fstr(pinfo->fd, COL_RES_DL_SRC, "%s (%s)",
		    get_ether_name(addr), type);
  if (check_col(pinfo->fd, COL_UNRES_DL_SRC))
    col_add_fstr(pinfo->fd, COL_UNRES_DL_SRC, "%s (%s)",
		     ether_to_str(addr), type);
}

static void
set_dst_addr_cols(packet_info *pinfo, const guint8 *addr, char *type)
{
  if (check_col(pinfo->fd, COL_RES_DL_DST))
    col_add_fstr(pinfo->fd, COL_RES_DL_DST, "%s (%s)",
		     get_ether_name(addr), type);
  if (check_col(pinfo->fd, COL_UNRES_DL_DST))
    col_add_fstr(pinfo->fd, COL_UNRES_DL_DST, "%s (%s)",
		     ether_to_str(addr), type);
}

/* ************************************************************************* */
/*                          Dissect 802.11 frame                             */
/* ************************************************************************* */
static void
dissect_ieee80211 (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  guint16 fcf, flags;
  const guint8 *src = NULL, *dst = NULL;
  proto_item *ti;
  proto_item *flag_item;
  proto_item *fc_item;
  static proto_tree *hdr_tree;
  static proto_tree *flag_tree;
  static proto_tree *fixed_tree;
  static proto_tree *tagged_tree;
  static proto_tree *fc_tree;
  guint16 cap_len, hdr_len;
  tvbuff_t *next_tvb;
  guint32 next_idx;
  guint32 addr_type;
  guint32 next_len;
  int tagged_parameter_tree_len;

  if (check_col (pinfo->fd, COL_PROTOCOL))
    col_set_str (pinfo->fd, COL_PROTOCOL, "IEEE 802.11");
  if (check_col (pinfo->fd, COL_INFO))
    col_clear (pinfo->fd, COL_INFO);

  cap_len = tvb_length(tvb);
  fcf = tvb_get_letohs (tvb, 0);

  /* Add the FC to the current tree */
  if (tree)
    {
      hdr_len = find_header_length (tvb_get_ptr (tvb, 0, cap_len), 0);
      ti = proto_tree_add_protocol_format (tree, proto_wlan, tvb, 0, hdr_len,
					   "IEEE 802.11 Header");
      hdr_tree = proto_item_add_subtree (ti, ett_80211);

      fc_item = proto_tree_add_uint_format (hdr_tree, hf_fc_field, tvb, 0, 2,
					    fcf,
					    "Frame Control: 0x%04X",
					    fcf);

      fc_tree = proto_item_add_subtree (fc_item, ett_fc_tree);


      proto_tree_add_uint (fc_tree, hf_fc_proto_version, tvb, 0, 1,
			   COOK_PROT_VERSION (fcf));

      proto_tree_add_uint (fc_tree, hf_fc_frame_type, tvb, 0, 1,
			   COOK_FRAME_TYPE (fcf));

      proto_tree_add_uint (fc_tree, hf_fc_frame_subtype,
			   tvb, 0, 1,
			   COOK_FRAME_SUBTYPE (fcf));

      flags = COOK_FLAGS (fcf);

      flag_item =
	proto_tree_add_uint_format (fc_tree, hf_fc_flags, tvb, 1, 1,
				    flags, "Flags: 0x%X", flags);

      flag_tree = proto_item_add_subtree (flag_item, ett_proto_flags);

      proto_tree_add_uint (flag_tree, hf_fc_data_ds, tvb, 1, 1,
			   COOK_DS_STATUS (flags));

      proto_tree_add_boolean (flag_tree, hf_fc_more_frag, tvb, 1, 1,
			      flags);

      proto_tree_add_boolean (flag_tree, hf_fc_retry, tvb, 1, 1, flags);

      proto_tree_add_boolean (flag_tree, hf_fc_pwr_mgt, tvb, 1, 1, flags);

      proto_tree_add_boolean (flag_tree, hf_fc_more_data, tvb, 1, 1,
			      flags);

      proto_tree_add_boolean (flag_tree, hf_fc_wep, tvb, 1, 1, flags);

      proto_tree_add_boolean (flag_tree, hf_fc_order, tvb, 1, 1, flags);

      if ((COMPOSE_FRAME_TYPE(fcf))==CTRL_PS_POLL) 
	proto_tree_add_uint(hdr_tree, hf_assoc_id,tvb,2,2,
			    COOK_ASSOC_ID(tvb_get_letohs(tvb,2)));
     
      else
	  proto_tree_add_uint (hdr_tree, hf_did_duration, tvb, 2, 2,
			       tvb_get_letohs (tvb, 2));
    }

  /* Perform tasks which are common to a certain frame type */
  switch (COOK_FRAME_TYPE (fcf))
    {

    case MGT_FRAME:		/* All management frames share a common header */
      src = tvb_get_ptr (tvb, 10, 6);
      dst = tvb_get_ptr (tvb, 4, 6);

      SET_ADDRESS(&pinfo->dl_src, AT_ETHER, 6, src);
      SET_ADDRESS(&pinfo->src, AT_ETHER, 6, src);
      SET_ADDRESS(&pinfo->dl_dst, AT_ETHER, 6, dst);
      SET_ADDRESS(&pinfo->dst, AT_ETHER, 6, dst);

      if (tree)
	{
	  proto_tree_add_ether (hdr_tree, hf_addr_da, tvb, 4, 6,
				tvb_get_ptr (tvb, 4, 6));

	  proto_tree_add_ether (hdr_tree, hf_addr_sa, tvb, 10, 6,
				tvb_get_ptr (tvb, 10, 6));

	  proto_tree_add_ether (hdr_tree, hf_addr_bssid, tvb, 16, 6,
				tvb_get_ptr (tvb, 16, 6));

	  proto_tree_add_uint (hdr_tree, hf_frag_number, tvb, 22, 2,
			       COOK_FRAGMENT_NUMBER (tvb_get_letohs
						     (tvb, 22)));

	  proto_tree_add_uint (hdr_tree, hf_seq_number, tvb, 22, 2,
			       COOK_SEQUENCE_NUMBER (tvb_get_letohs
						     (tvb, 22)));
	  cap_len = cap_len - MGT_FRAME_LEN - 4;
	}
      break;



    case CONTROL_FRAME:
      break;



    case DATA_FRAME:
      addr_type = COOK_ADDR_SELECTOR (fcf);
      hdr_len = find_header_length (tvb_get_ptr (tvb, 0, cap_len), 0);

      /* In order to show src/dst address we must always do the following */
      switch (addr_type)
	{

	case DATA_ADDR_T1:
	  src = tvb_get_ptr (tvb, 10, 6);
	  dst = tvb_get_ptr (tvb, 4, 6);
	  break;


	case DATA_ADDR_T2:
	  src = tvb_get_ptr (tvb, 16, 6);
	  dst = tvb_get_ptr (tvb, 4, 6);
	  break;


	case DATA_ADDR_T3:
	  src = tvb_get_ptr (tvb, 10, 6);
	  dst = tvb_get_ptr (tvb, 16, 6);
	  break;


	case DATA_ADDR_T4:
	  src = tvb_get_ptr (tvb, 24, 6);
	  dst = tvb_get_ptr (tvb, 16, 6);
	  break;
	}

      SET_ADDRESS(&pinfo->dl_src, AT_ETHER, 6, src);
      SET_ADDRESS(&pinfo->src, AT_ETHER, 6, src);
      SET_ADDRESS(&pinfo->dl_dst, AT_ETHER, 6, dst);
      SET_ADDRESS(&pinfo->dst, AT_ETHER, 6, dst);

      /* Now if we have a tree we start adding stuff */
      if (tree)
	{


	  switch (addr_type)
	    {

	    case DATA_ADDR_T1:
	      proto_tree_add_ether (hdr_tree, hf_addr_da, tvb, 4, 6,
				    tvb_get_ptr (tvb, 4, 6));
	      proto_tree_add_ether (hdr_tree, hf_addr_sa, tvb, 10, 6,
				    tvb_get_ptr (tvb, 10, 6));
	      proto_tree_add_ether (hdr_tree, hf_addr_bssid, tvb, 16, 6,
				    tvb_get_ptr (tvb, 16, 6));
	      proto_tree_add_uint (hdr_tree, hf_frag_number, tvb, 22, 2,
				   COOK_FRAGMENT_NUMBER (tvb_get_letohs
							 (tvb, 22)));
	      proto_tree_add_uint (hdr_tree, hf_seq_number, tvb, 22, 2,
				   COOK_SEQUENCE_NUMBER (tvb_get_letohs
							 (tvb, 22)));
	      break;
	      
	      
	    case DATA_ADDR_T2:
	      proto_tree_add_ether (hdr_tree, hf_addr_da, tvb, 4, 6,
				    tvb_get_ptr (tvb, 4, 6));
	      proto_tree_add_ether (hdr_tree, hf_addr_bssid, tvb, 10, 6,
				    tvb_get_ptr (tvb, 10, 6));
	      proto_tree_add_ether (hdr_tree, hf_addr_sa, tvb, 16, 6,
				    tvb_get_ptr (tvb, 16, 6));
	      proto_tree_add_uint (hdr_tree, hf_frag_number, tvb, 22, 2,
				   COOK_FRAGMENT_NUMBER (tvb_get_letohs
							 (tvb, 22)));
	      proto_tree_add_uint (hdr_tree, hf_seq_number, tvb, 22, 2,
				   COOK_SEQUENCE_NUMBER (tvb_get_letohs
							 (tvb, 22)));
	      break;
   

	    case DATA_ADDR_T3:
	      proto_tree_add_ether (hdr_tree, hf_addr_bssid, tvb, 4, 6,
				    tvb_get_ptr (tvb, 4, 6));
	      proto_tree_add_ether (hdr_tree, hf_addr_sa, tvb, 10, 6,
				    tvb_get_ptr (tvb, 10, 6));
	      proto_tree_add_ether (hdr_tree, hf_addr_da, tvb, 16, 6,
				    tvb_get_ptr (tvb, 16, 6));
	      proto_tree_add_uint (hdr_tree, hf_frag_number, tvb, 22, 2,
				   COOK_FRAGMENT_NUMBER (tvb_get_letohs
							 (tvb, 22)));
	      proto_tree_add_uint (hdr_tree, hf_seq_number, tvb, 22, 2,
				   COOK_SEQUENCE_NUMBER (tvb_get_letohs
							 (tvb, 22)));
	      break;
	      

	    case DATA_ADDR_T4:
	      proto_tree_add_ether (hdr_tree, hf_addr_ra, tvb, 4, 6,
				    tvb_get_ptr (tvb, 4, 6));
	      proto_tree_add_ether (hdr_tree, hf_addr_ta, tvb, 10, 6,
				    tvb_get_ptr (tvb, 10, 6));
	      proto_tree_add_ether (hdr_tree, hf_addr_da, tvb, 16, 6,
				    tvb_get_ptr (tvb, 16, 6));
	      proto_tree_add_uint (hdr_tree, hf_frag_number, tvb, 22, 2,
				   COOK_FRAGMENT_NUMBER (tvb_get_letohs
							 (tvb, 22)));
	      proto_tree_add_uint (hdr_tree, hf_seq_number, tvb, 22, 2,
				   COOK_SEQUENCE_NUMBER (tvb_get_letohs
							 (tvb, 22)));
	      proto_tree_add_ether (hdr_tree, hf_addr_sa, tvb, 24, 6,
				    tvb_get_ptr (tvb, 24, 6));
	      break;
	    }

	}
      break;
    }


  switch (COMPOSE_FRAME_TYPE (fcf))
    {

    case MGT_ASSOC_REQ:
      COL_SHOW_INFO_CONST (pinfo->fd, "Association Request");
      if (tree)
	{
	  fixed_tree = get_fixed_parameter_tree (tree, tvb, MGT_FRAME_LEN, 4);
	  add_fixed_field (fixed_tree, tvb, MGT_FRAME_HDR_LEN,
			   FIELD_CAP_INFO);
	  add_fixed_field (fixed_tree, tvb, MGT_FRAME_HDR_LEN + 2,
			   FIELD_LISTEN_IVAL);

	  next_idx = MGT_FRAME_HDR_LEN + 4;	/* Size of fixed fields */
	  tagged_parameter_tree_len =
	      tvb_reported_length_remaining(tvb, next_idx + 4);
	  tagged_tree = get_tagged_parameter_tree (tree, tvb, next_idx,
						   tagged_parameter_tree_len);

	  while (tagged_parameter_tree_len > 0) {
	    if ((next_len=add_tagged_field (tagged_tree, tvb, next_idx))==0)
	      break;
	    next_idx +=next_len;
	    tagged_parameter_tree_len -= next_len;
	  }
	}
      break;



    case MGT_ASSOC_RESP:
      COL_SHOW_INFO_CONST (pinfo->fd, "Association Response");

      if (tree)
	{
	  fixed_tree = get_fixed_parameter_tree (tree, tvb, MGT_FRAME_LEN, 6);
	  add_fixed_field (fixed_tree, tvb, MGT_FRAME_LEN, FIELD_CAP_INFO);
	  add_fixed_field (fixed_tree, tvb, MGT_FRAME_LEN + 2,
			   FIELD_STATUS_CODE);
	  add_fixed_field (fixed_tree, tvb, MGT_FRAME_LEN + 4,
			   FIELD_ASSOC_ID);

	  next_idx = MGT_FRAME_LEN + 6;	/* Size of fixed fields */

	  tagged_parameter_tree_len =
	      tvb_reported_length_remaining(tvb, next_idx + 4);
	  tagged_tree = get_tagged_parameter_tree (tree, tvb, next_idx,
						   tagged_parameter_tree_len);

	  while (tagged_parameter_tree_len > 0) {
	    if ((next_len=add_tagged_field (tagged_tree, tvb, next_idx))==0)
	      break;
	    next_idx +=next_len;
	    tagged_parameter_tree_len -= next_len;
	  }
	}
      break;


    case MGT_REASSOC_REQ:
      COL_SHOW_INFO_CONST (pinfo->fd, "Reassociation Request");
      if (tree)
	{
	  fixed_tree = get_fixed_parameter_tree (tree, tvb, MGT_FRAME_LEN, 10);
	  add_fixed_field (fixed_tree, tvb, MGT_FRAME_LEN, FIELD_CAP_INFO);
	  add_fixed_field (fixed_tree, tvb, MGT_FRAME_LEN + 2,
			   FIELD_LISTEN_IVAL);
	  add_fixed_field (fixed_tree, tvb, MGT_FRAME_LEN + 4,
			   FIELD_CURRENT_AP_ADDR);

	  next_idx = MGT_FRAME_LEN + 10;	/* Size of fixed fields */
	  tagged_parameter_tree_len =
	      tvb_reported_length_remaining(tvb, next_idx + 4);
	  tagged_tree = get_tagged_parameter_tree (tree, tvb, next_idx,
						   tagged_parameter_tree_len);

	  while (tagged_parameter_tree_len > 0) {
	    if ((next_len=add_tagged_field (tagged_tree, tvb, next_idx))==0)
	      break;
	    next_idx +=next_len;
	    tagged_parameter_tree_len -= next_len;
	  }
	}
      break;

    case MGT_REASSOC_RESP:
      COL_SHOW_INFO_CONST (pinfo->fd, "Reassociation Response");
      if (tree)
	{
	  fixed_tree = get_fixed_parameter_tree (tree, tvb, MGT_FRAME_LEN, 10);
	  add_fixed_field (fixed_tree, tvb, MGT_FRAME_LEN, FIELD_CAP_INFO);
	  add_fixed_field (fixed_tree, tvb, MGT_FRAME_LEN + 2,
			   FIELD_STATUS_CODE);
	  add_fixed_field (fixed_tree, tvb, MGT_FRAME_LEN + 4,
			   FIELD_ASSOC_ID);

	  next_idx = MGT_FRAME_LEN + 6;	/* Size of fixed fields */
	  tagged_parameter_tree_len =
	      tvb_reported_length_remaining(tvb, next_idx + 4);
	  tagged_tree = get_tagged_parameter_tree (tree, tvb, next_idx,
						   tagged_parameter_tree_len);

	  while (tagged_parameter_tree_len > 0) {
	    if ((next_len=add_tagged_field (tagged_tree, tvb, next_idx))==0)
	      break;
	    next_idx +=next_len;
	    tagged_parameter_tree_len -= next_len;
	  }
	}
      break;


    case MGT_PROBE_REQ:
      COL_SHOW_INFO_CONST (pinfo->fd, "Probe Request");
      if (tree)
	{
	  next_idx = MGT_FRAME_LEN;
	  tagged_parameter_tree_len =
	      tvb_reported_length_remaining(tvb, next_idx + 4);
	  tagged_tree = get_tagged_parameter_tree (tree, tvb, next_idx,
						   tagged_parameter_tree_len);

	  while (tagged_parameter_tree_len > 0) {
	    if ((next_len=add_tagged_field (tagged_tree, tvb, next_idx))==0)
	      break;
	    next_idx +=next_len;
	    tagged_parameter_tree_len -= next_len;
	  }
	}
      break;


    case MGT_PROBE_RESP:
      COL_SHOW_INFO_CONST (pinfo->fd, "Probe Response");
      if (tree)
	{
	  fixed_tree = get_fixed_parameter_tree (tree, tvb, MGT_FRAME_LEN, 12);
	  add_fixed_field (fixed_tree, tvb, MGT_FRAME_LEN, FIELD_TIMESTAMP);
	  add_fixed_field (fixed_tree, tvb, MGT_FRAME_LEN + 8,
			   FIELD_BEACON_INTERVAL);
	  add_fixed_field (fixed_tree, tvb, MGT_FRAME_LEN + 10,
			   FIELD_CAP_INFO);

	  next_idx = MGT_FRAME_LEN + 12;	/* Size of fixed fields */
	  tagged_parameter_tree_len =
	      tvb_reported_length_remaining(tvb, next_idx + 4);
	  tagged_tree = get_tagged_parameter_tree (tree, tvb, next_idx,
						   tagged_parameter_tree_len);

	  while (tagged_parameter_tree_len > 0) {
	    if ((next_len=add_tagged_field (tagged_tree, tvb, next_idx))==0)
	      break;
	    next_idx +=next_len;
	    tagged_parameter_tree_len -= next_len;
	  }
	}
      break;


    case MGT_BEACON:		/* Dissect protocol payload fields  */
      COL_SHOW_INFO_CONST (pinfo->fd, "Beacon frame");

      if (tree)
	{
	  fixed_tree = get_fixed_parameter_tree (tree, tvb, MGT_FRAME_LEN, 12);

	  add_fixed_field (fixed_tree, tvb, MGT_FRAME_LEN, FIELD_TIMESTAMP);

	  add_fixed_field (fixed_tree, tvb, MGT_FRAME_LEN + 8,
			   FIELD_BEACON_INTERVAL);
	  add_fixed_field (fixed_tree, tvb, MGT_FRAME_LEN + 10,
			   FIELD_CAP_INFO);

	  next_idx = MGT_FRAME_LEN + 12;	/* Size of fixed fields */
	  tagged_parameter_tree_len =
	      tvb_reported_length_remaining(tvb, next_idx + 4);
	  tagged_tree = get_tagged_parameter_tree (tree, tvb, next_idx,
						   tagged_parameter_tree_len);

	  while (tagged_parameter_tree_len > 0) {
	    if ((next_len=add_tagged_field (tagged_tree, tvb, next_idx))==0)
	      break;
	    next_idx +=next_len;
	    tagged_parameter_tree_len -= next_len;
	  }
	}
      break;



    case MGT_ATIM:
      COL_SHOW_INFO_CONST (pinfo->fd, "ATIM");
      if (tree)
	{
	}
      break;

    case MGT_DISASS:
      COL_SHOW_INFO_CONST (pinfo->fd, "Dissassociate");
      if (tree)
	{
	  fixed_tree = get_fixed_parameter_tree (tree, tvb, MGT_FRAME_LEN, cap_len);
	  add_fixed_field (fixed_tree, tvb, MGT_FRAME_LEN, FIELD_REASON_CODE);
	}
      break;

    case MGT_AUTHENTICATION:
      COL_SHOW_INFO_CONST (pinfo->fd, "Authentication");
      if (IS_WEP(COOK_FLAGS(fcf)))
	{
	  int pkt_len = tvb_reported_length (tvb);
	  int cap_len = tvb_length (tvb);

	  if (tree)
	    get_wep_parameter_tree (tree, tvb, MGT_FRAME_LEN, pkt_len);
	  pkt_len = MAX (pkt_len - 8 - MGT_FRAME_LEN, 0);
	  cap_len = MIN (pkt_len, MAX (cap_len - 8 - MGT_FRAME_LEN, 0));
	  next_tvb = tvb_new_subset (tvb, MGT_FRAME_LEN + 4, cap_len, pkt_len);
	  dissect_data (next_tvb, 0, pinfo, tree);
	}
      else
	{
	  if (tree)
	    {
	      fixed_tree = get_fixed_parameter_tree (tree, tvb, MGT_FRAME_LEN, 6);
	      add_fixed_field (fixed_tree, tvb, MGT_FRAME_LEN, FIELD_AUTH_ALG);
	      add_fixed_field (fixed_tree, tvb, MGT_FRAME_LEN + 2,
			       FIELD_AUTH_TRANS_SEQ);
	      add_fixed_field (fixed_tree, tvb, MGT_FRAME_LEN + 4,
			       FIELD_STATUS_CODE);

	      next_idx = MGT_FRAME_LEN + 6;	/* Size of fixed fields */

	      tagged_parameter_tree_len =
		  tvb_reported_length_remaining(tvb, next_idx + 4);
	      if (tagged_parameter_tree_len != 0)
		{
		  tagged_tree = get_tagged_parameter_tree (tree,
							   tvb,
							   next_idx,
							   tagged_parameter_tree_len);

		  while (tagged_parameter_tree_len > 0) {
		    if ((next_len=add_tagged_field (tagged_tree, tvb, next_idx))==0)
		      break;
		    next_idx +=next_len;
		    tagged_parameter_tree_len -= next_len;
		  }
		}
	    }
	}
      break;

    case MGT_DEAUTHENTICATION:
      COL_SHOW_INFO_CONST (pinfo->fd, "Deauthentication");
      if (tree)
	{
	  fixed_tree = get_fixed_parameter_tree (hdr_tree, tvb, MGT_FRAME_LEN, 2);
	  add_fixed_field (fixed_tree, tvb, MGT_FRAME_LEN, FIELD_REASON_CODE);
	}
      break;



    case CTRL_PS_POLL:
      COL_SHOW_INFO_CONST (pinfo->fd, "Power-Save poll");

      src = tvb_get_ptr (tvb, 10, 6);
      dst = tvb_get_ptr (tvb, 4, 6);

      set_src_addr_cols(pinfo, src, "BSSID");
      set_dst_addr_cols(pinfo, dst, "BSSID");

      if (tree)
	{
	  proto_tree_add_ether (hdr_tree, hf_addr_bssid, tvb, 4, 6,
				tvb_get_ptr (tvb, 4, 6));

	  proto_tree_add_ether (hdr_tree, hf_addr_ta, tvb, 10, 6,
				tvb_get_ptr (tvb, 10, 6));

	}
      break;



    case CTRL_RTS:
      COL_SHOW_INFO_CONST (pinfo->fd, "Request-to-send");
      src = tvb_get_ptr (tvb, 10, 6);
      dst = tvb_get_ptr (tvb, 4, 6);

      set_src_addr_cols(pinfo, src, "TA");
      set_dst_addr_cols(pinfo, dst, "RA");

      if (tree)
	{
	  proto_tree_add_ether (hdr_tree, hf_addr_ra, tvb, 4, 6,
				tvb_get_ptr (tvb, 4, 6));

	  proto_tree_add_ether (hdr_tree, hf_addr_ta, tvb, 10, 6,
				tvb_get_ptr (tvb, 10, 6));

	}
      break;



    case CTRL_CTS:
      COL_SHOW_INFO_CONST (pinfo->fd, "Clear-to-send");

      dst = tvb_get_ptr (tvb, 4, 6);

      set_dst_addr_cols(pinfo, dst, "RA");

      if (tree)
	{
	  proto_tree_add_ether (hdr_tree, hf_addr_ra, tvb, 4, 6,
				tvb_get_ptr (tvb, 4, 6));

	}
      break;



    case CTRL_ACKNOWLEDGEMENT:
      COL_SHOW_INFO_CONST (pinfo->fd, "Acknowledgement");

      dst = tvb_get_ptr (tvb, 4, 6);

      set_dst_addr_cols(pinfo, dst, "RA");

      if (tree)
	proto_tree_add_ether (hdr_tree, hf_addr_ra, tvb, 4, 6,
			      tvb_get_ptr (tvb, 4, 6));
      break;



    case CTRL_CFP_END:
      COL_SHOW_INFO_CONST (pinfo->fd, "CF-End (Control-frame)");

      src = tvb_get_ptr (tvb, 10, 6);
      dst = tvb_get_ptr (tvb, 4, 6);

      set_src_addr_cols(pinfo, src, "BSSID");
      set_dst_addr_cols(pinfo, dst, "RA");

      if (tree)
	{
	  proto_tree_add_ether (hdr_tree, hf_addr_ra, tvb, 4, 6,
				tvb_get_ptr (tvb, 4, 6));
	  proto_tree_add_ether (hdr_tree, hf_addr_bssid, tvb, 10, 6,
				tvb_get_ptr (tvb, 10, 6));
	}
      break;



    case CTRL_CFP_ENDACK:
      COL_SHOW_INFO_CONST (pinfo->fd, "CF-End + CF-Ack (Control-frame)");

      src = tvb_get_ptr (tvb, 10, 6);
      dst = tvb_get_ptr (tvb, 4, 6);

      set_src_addr_cols(pinfo, src, "BSSID");
      set_dst_addr_cols(pinfo, dst, "RA");

      if (tree)
	{
	  proto_tree_add_ether (hdr_tree, hf_addr_ra, tvb, 4, 6,
				tvb_get_ptr (tvb, 4, 6));

	  proto_tree_add_ether (hdr_tree, hf_addr_bssid, tvb, 10, 6,
				tvb_get_ptr (tvb, 10, 6));
	}
      break;



    case DATA:
      COL_SHOW_INFO_CONST (pinfo->fd, "Data");
      hdr_len = find_header_length (tvb_get_ptr (tvb, 0, cap_len), 0);

      next_tvb = tvb_new_subset (tvb, hdr_len, -1, -1);

      if (IS_WEP(COOK_FLAGS(fcf)))
	{
	  int pkt_len = tvb_reported_length (next_tvb);
	  int cap_len = tvb_length (next_tvb);

	  if (tree)
	    get_wep_parameter_tree (tree, next_tvb, 0, pkt_len);
	  pkt_len = MAX (pkt_len - 8, 0);
	  cap_len = MIN (pkt_len, MAX (cap_len - 8, 0));
	  next_tvb = tvb_new_subset (tvb, hdr_len + 4, cap_len, pkt_len);
	  dissect_data (next_tvb, 0, pinfo, tree);
	}
      else
	call_dissector (llc_handle, next_tvb, pinfo, tree);
      break;



    case DATA_CF_ACK:
      COL_SHOW_INFO_CONST (pinfo->fd, "Data + CF-Acknowledgement");
      hdr_len = find_header_length (tvb_get_ptr (tvb, 0, cap_len), 0);

      next_tvb = tvb_new_subset (tvb, hdr_len, -1, -1);

      if (IS_WEP(COOK_FLAGS(fcf)))
	{
	  int pkt_len = tvb_reported_length (next_tvb);
	  int cap_len = tvb_length (next_tvb);

	  if (tree)
	    get_wep_parameter_tree (tree, next_tvb, 0, pkt_len);
	  pkt_len = MAX (pkt_len - 8, 0);
	  cap_len = MIN (pkt_len, MAX (cap_len - 8, 0));
	  next_tvb = tvb_new_subset (tvb, hdr_len + 4, cap_len, pkt_len);
	  dissect_data (next_tvb, 0, pinfo, tree);
	}
      else
	call_dissector (llc_handle, next_tvb, pinfo, tree);
      break;



    case DATA_CF_POLL:
      COL_SHOW_INFO_CONST (pinfo->fd, "Data + CF-Poll");
      hdr_len = find_header_length (tvb_get_ptr (tvb, 0, cap_len), 0);
      next_tvb = tvb_new_subset (tvb, hdr_len, -1, -1);

      if (IS_WEP(COOK_FLAGS(fcf)))
	{
	  int pkt_len = tvb_reported_length (next_tvb);
	  int cap_len = tvb_length (next_tvb);

	  if (tree)
	    get_wep_parameter_tree (tree, next_tvb, 0, pkt_len);
	  pkt_len = MAX (pkt_len - 8, 0);
	  cap_len = MIN (pkt_len, MAX (cap_len - 8, 0));
	  next_tvb = tvb_new_subset (tvb, hdr_len + 4, cap_len, pkt_len);
	  dissect_data (next_tvb, 0, pinfo, tree);
	}
      else
	call_dissector (llc_handle, next_tvb, pinfo, tree);
      break;



    case DATA_CF_ACK_POLL:
      COL_SHOW_INFO_CONST (pinfo->fd, "Data + CF-Acknowledgement/Poll");
      hdr_len = find_header_length (tvb_get_ptr (tvb, 0, cap_len), 0);
      next_tvb = tvb_new_subset (tvb, hdr_len, -1, -1);

      if (IS_WEP(COOK_FLAGS(fcf)))
	{
	  int pkt_len = tvb_reported_length (next_tvb);
	  int cap_len = tvb_length (next_tvb);

	  if (tree)
	    get_wep_parameter_tree (tree, next_tvb, 0, pkt_len);
	  pkt_len = MAX (pkt_len - 8, 0);
	  cap_len = MIN (pkt_len, MAX (cap_len - 8, 0));
	  next_tvb = tvb_new_subset (tvb, hdr_len + 4, cap_len, pkt_len);
	  dissect_data (next_tvb, 0, pinfo, tree);
	}
      else
	call_dissector (llc_handle, next_tvb, pinfo, tree);
      break;



    case DATA_NULL_FUNCTION:
      COL_SHOW_INFO_CONST (pinfo->fd, "Null function (No data)");
      break;


    case DATA_CF_ACK_NOD:
      COL_SHOW_INFO_CONST (pinfo->fd, "Data + Acknowledgement (No data)");
      break;


    case DATA_CF_POLL_NOD:
      COL_SHOW_INFO_CONST (pinfo->fd, "Data + CF-Poll (No data)");
      break;


    case DATA_CF_ACK_POLL_NOD:
      COL_SHOW_INFO_CONST (pinfo->fd, "Data + CF-Acknowledgement/Poll (No data)");
      break;


    default:
      COL_SHOW_INFO_CONST (pinfo->fd, "Unrecognized (Reserved frame)");
      break;
    }
}


void
proto_register_wlan (void)
{

  static const value_string tofrom_ds[] = {
    {0, "Not leaving DS or network is operating in AD-HOC mode ( To DS: 0  From DS: 0)"},
    {1, "Frame is exiting DS (To DS: 0  From DS: 1)"},
    {2, "Frame is entering DS (To DS: 1  From DS: 0)"},
    {3, "Frame part of WDS (To DS: 1  From DS: 1)"},
    {0, NULL}
  };

  static const true_false_string tods_flag = {
    "TO DS: Should be false",
    "Not used"
  };

  static const true_false_string fromds_flag = {
    "FROM DS: Should be false",
    "Not used"
  };

  static const true_false_string more_frags = {
    "MSDU/MMPDU is fragmented",
    "No fragments"
  };

  static const true_false_string retry_flags = {
    "Frame is being retransmitted",
    "Frame is not being retransmitted"
  };

  static const true_false_string pm_flags = {
    "STA will go to sleep",
    "STA will stay up"
  };

  static const true_false_string md_flags = {
    "Data is buffered for STA at AP",
    "No data buffered"
  };

  static const true_false_string wep_flags = {
    "WEP is enabled",
    "WEP is disabled"
  };

  static const true_false_string order_flags = {
    "",
    ""
  };

  static const true_false_string cf_ess_flags = {
    "Transmitter is an AP",
    "Transmitter is a STA"
  };


  static const true_false_string cf_privacy_flags = {
    "AP/STA can support WEP",
    "AP/STA cannot support WEP"
  };

  static const true_false_string cf_preamble_flags = {
    "Short preamble allowed",
    "Short preamble not allowed"
  };

  static const true_false_string cf_pbcc_flags = {
    "PBCC modulation allowed",
    "PBCC modulation not allowed"
  };

  static const true_false_string cf_agility_flags = {
    "Channel agility in use",
    "Channel agility not in use"
  };


  static const true_false_string cf_ibss_flags = {
    "Transmitter belongs to an IBSS",
    "Transmitter belongs to a BSS"
  };

  static const value_string sta_cf_pollable[] = {
    {0x00, "Station is not CF-Pollable"},
    {0x02, "Station is CF-Pollable, "
     "not requesting to be placed on the  CF-polling list"},
    {0x01, "Station is CF-Pollable, "
     "requesting to be placed on the CF-polling list"},
    {0x03, "Station is CF-Pollable, requesting never to be polled"},
    {0, NULL}
  };

  static const value_string ap_cf_pollable[] = {
    {0x00, "No point coordinator at AP"},
    {0x02, "Point coordinator at AP for delivery only (no polling)"},
    {0x01, "Point coordinator at AP for delivery and polling"},
    {0x03, "Reserved"},
    {0, NULL}
  };


  static const value_string auth_alg[] = {
    {0x00, "Open System"},
    {0x01, "Shared key"},
    {0, NULL}
  };

  static const value_string reason_codes[] = {
    {0x00, "Reserved"},
    {0x01, "Unspecified reason"},
    {0x02, "Previous authentication no longer valid"},
    {0x03, "Deauthenticated because sending STA is leaving (has left) "
     "IBSS or ESS"},
    {0x04, "Disassociated due to inactivity"},
    {0x05, "Disassociated because AP is unable to handle all currently "
     "associated stations"},
    {0x06, "Class 2 frame received from nonauthenticated station"},
    {0x07, "Class 3 frame received from nonassociated station"},
    {0x08, "Disassociated because sending STA is leaving (has left) BSS"},
    {0x09, "Station requesting (re)association is not authenticated with "
     "responding station"},
    {0x00, NULL}
  };


  static const value_string status_codes[] = {
    {0x00, "Successful"},
    {0x01, "Unspecified failure"},
    {0x0A, "Cannot support all requested capabilities in the "
     "Capability information field"},
    {0x0B, "Reassociation denied due to inability to confirm that "
     "association exists"},
    {0x0C, "Association denied due to reason outside the scope of this "
     "standard"},

    {0x0D, "Responding station does not support the specified authentication "
     "algorithm"},
    {0x0E, "Received an Authentication frame with authentication sequence "
     "transaction sequence number out of expected sequence"},
    {0x0F, "Authentication rejected because of challenge failure"},
    {0x10, "Authentication rejected due to timeout waiting for next "
     "frame in sequence"},
    {0x11, "Association denied because AP is unable to handle additional "
     "associated stations"},
    {0x12, "Association denied due to requesting station not supporting all "
     "of the datarates in the BSSBasicServiceSet Parameter"},
    {0x00, NULL}
  };



  static hf_register_info hf[] = {
    {&hf_fc_field,
     {"Frame Control Field", "wlan.fc", FT_UINT16, BASE_HEX, NULL, 0,
      "MAC Frame control"}},

    {&hf_fc_proto_version,
     {"Version", "wlan.fc.version", FT_UINT8, BASE_DEC, NULL, 0,
      "MAC Protocol version"}},	/* 0 */

    {&hf_fc_frame_type,
     {"Type", "wlan.fc.type", FT_UINT8, BASE_DEC, NULL, 0,
      "Frame type"}},

    {&hf_fc_frame_subtype,
     {"Subtype", "wlan.fc.subtype", FT_UINT8, BASE_DEC, NULL, 0,
      "Frame subtype"}},	/* 2 */

    {&hf_fc_flags,
     {"Protocol Flags", "wlan.flags", FT_UINT8, BASE_HEX, NULL, 0,
      "Protocol flags"}},

    {&hf_fc_data_ds,
     {"DS status", "wlan.fc.ds", FT_UINT8, BASE_HEX, TFS (&tofrom_ds), 0,
      "Data-frame DS-traversal status"}},	/* 3 */

    {&hf_fc_to_ds,
     {"To DS", "wlan.fc.tods", FT_BOOLEAN, 8, TFS (&tods_flag), 0x1,
      "To DS flag"}},		/* 4 */

    {&hf_fc_from_ds,
     {"From DS", "wlan.fc.fromds", FT_BOOLEAN, 8, TFS (&fromds_flag), 0x2,
      "From DS flag"}},		/* 5 */

    {&hf_fc_more_frag,
     {"Fragments", "wlan.fc.frag", FT_BOOLEAN, 8, TFS (&more_frags), 0x4,
      "More Fragments flag"}},	/* 6 */

    {&hf_fc_retry,
     {"Retry", "wlan.fc.retry", FT_BOOLEAN, 8, TFS (&retry_flags), 0x8,
      "Retransmission flag"}},

    {&hf_fc_pwr_mgt,
     {"PWR MGT", "wlan.fc.pwrmgt", FT_BOOLEAN, 8, TFS (&pm_flags), 0x10,
      "Power management status"}},

    {&hf_fc_more_data,
     {"More Data", "wlan.fc.moredata", FT_BOOLEAN, 8, TFS (&md_flags), 0x20,
      "More data flag"}},

    {&hf_fc_wep,
     {"WEP flag", "wlan.fc.wep", FT_BOOLEAN, 8, TFS (&wep_flags), 0x40,
      "WEP flag"}},

    {&hf_fc_order,
     {"Order flag", "wlan.fc.order", FT_BOOLEAN, 8, TFS (&order_flags), 0x80,
      "Strictly ordered flag"}},

    {&hf_assoc_id,
     {"Association ID","wlan.aid",FT_UINT16, BASE_DEC,NULL,0,
      "Association-ID field" }},

    {&hf_did_duration,
     {"Duration", "wlan.duration", FT_UINT16, BASE_DEC, NULL, 0,
      "Duration field"}},

    {&hf_addr_da,
     {"Destination address", "wlan.da", FT_ETHER, BASE_NONE, NULL, 0,
      "Destination Hardware address"}},

    {&hf_addr_sa,
     {"Source address", "wlan.sa", FT_ETHER, BASE_NONE, NULL, 0,
      "Source Hardware address"}},

    {&hf_addr_ra,
     {"Receiver address", "wlan.ra", FT_ETHER, BASE_NONE, NULL, 0,
      "Receiving Station Hardware Address"}},

    {&hf_addr_ta,
     {"Transmitter address", "wlan.ta", FT_ETHER, BASE_NONE, NULL, 0,
      "Transmitting Station Hardware Address"}},

    {&hf_addr_bssid,
     {"BSS Id", "wlan.bssid", FT_ETHER, BASE_NONE, NULL, 0,
      "Basic Service Set ID"}},

    {&hf_frag_number,
     {"Fragment number", "wlan.frag", FT_UINT16, BASE_HEX, NULL, 0,
      "Fragment number"}},

    {&hf_seq_number,
     {"Sequence number", "wlan.seq", FT_UINT16, BASE_HEX, NULL, 0,
      "Fragment number"}},

    {&hf_fcs,
     {"Frame Check Sequence (not verified)", "wlan.fcs", FT_UINT32, BASE_HEX,
      NULL, 0, ""}},

    {&ff_timestamp,
     {"Timestamp", "wlan.fixed.timestamp", FT_STRING, BASE_NONE,
      NULL, 0, ""}},

    {&ff_auth_alg,
     {"Authentication Algorithm", "wlan.fixed.auth.alg",
      FT_UINT16, BASE_DEC, VALS (&auth_alg), 0, ""}},

    {&ff_beacon_interval,
     {"Beacon Interval", "wlan.fixed.beacon", FT_DOUBLE, BASE_DEC, NULL, 0,
      ""}},

    {&hf_fixed_parameters,
     {"Fixed parameters", "wlan.fixed.all", FT_UINT16, BASE_DEC, NULL, 0,
      ""}},

    {&hf_tagged_parameters,
     {"Tagged parameters", "wlan.tagged.all", FT_UINT16, BASE_DEC, NULL, 0,
      ""}},

    {&hf_wep_parameters,
     {"WEP parameters", "wlan.wep.all", FT_STRING, BASE_NONE, NULL, 0,
      ""}},

    {&hf_wep_iv,
     {"Initialization Vector", "wlan.wep.iv", FT_UINT32, BASE_HEX, NULL, 0,
      "Initialization Vector"}},

    {&hf_wep_key,
     {"Key", "wlan.wep.key", FT_UINT32, BASE_DEC, NULL, 0,
      "Key"}},

    {&hf_wep_crc,
     {"WEP CRC (not verified)", "wlan.wep.crc", FT_UINT32, BASE_HEX, NULL, 0,
      "WEP CRC"}},

    {&ff_capture,
     {"Capabilities", "wlan.fixed.capabilities", FT_UINT16, BASE_HEX, NULL, 0,
      "Capability information"}},

    {&ff_cf_sta_poll,
     {"CFP participation capabilities", "wlan.fixed.capabilities.cfpoll.sta",
      FT_UINT16, BASE_HEX, VALS (&sta_cf_pollable), 0,
      "CF-Poll capabilities for a STA"}},

    {&ff_cf_ap_poll,
     {"CFP participation capabilities", "wlan.fixed.capabilities.cfpoll.ap",
      FT_UINT16, BASE_HEX, VALS (&ap_cf_pollable), 0,
      "CF-Poll capabilities for an AP"}},

    {&ff_cf_ess,
     {"ESS capabilities", "wlan.fixed.capabilities.ess",
      FT_BOOLEAN, 8, TFS (&cf_ess_flags), 0x0001, "ESS capabilities"}},


    {&ff_cf_ibss,
     {"IBSS status", "wlan.fixed.capabilities.ibss",
      FT_BOOLEAN, 8, TFS (&cf_ibss_flags), 0x0002, "IBSS participation"}},

    {&ff_cf_privacy,
     {"Privacy", "wlan.fixed.capabilities.privacy",
      FT_BOOLEAN, 8, TFS (&cf_privacy_flags), 0x0010, "WEP support"}},

    {&ff_cf_preamble,
     {"Short Preamble", "wlan.fixed.capabilities.preamble",
      FT_BOOLEAN, 8, TFS (&cf_preamble_flags), 0x0020, "Short Preamble"}},

    {&ff_cf_pbcc,
     {"PBCC", "wlan.fixed.capabilities.pbcc",
      FT_BOOLEAN, 8, TFS (&cf_pbcc_flags), 0x0040, "PBCC Modulation"}},

    {&ff_cf_agility,
     {"Channel Agility", "wlan.fixed.capabilities.agility",
      FT_BOOLEAN, 8, TFS (&cf_agility_flags), 0x0080, "Channel Agility"}},


    {&ff_auth_seq,
     {"Authentication SEQ", "wlan.fixed.auth_seq",
      FT_UINT16, BASE_HEX, NULL, 0, "Authentication sequence number"}},

    {&ff_assoc_id,
     {"Association ID", "wlan.fixed.aid",
      FT_UINT16, BASE_HEX, NULL, 0, "Association ID"}},

    {&ff_listen_ival,
     {"Listen Interval", "wlan.fixed.listen_ival",
      FT_UINT16, BASE_HEX, NULL, 0, "Listen Interval"}},

    {&ff_current_ap,
     {"Current AP", "wlan.fixed.current_ap",
      FT_ETHER, BASE_NONE, NULL, 0, "MAC address of current AP"}},

    {&ff_reason,
     {"Reason code", "wlan.fixed.reason_code",
      FT_UINT16, BASE_HEX, VALS (&reason_codes), 0,
      "Reason for unsolicited notification"}},

    {&ff_status_code,
     {"Status code", "wlan.fixed.status_code",
      FT_UINT16, BASE_HEX, VALS (&status_codes), 0,
      "Status of requested event"}},

    {&tag_number,
     {"Tag", "wlan.tag.number",
      FT_UINT16, BASE_DEC, NULL, 0,
      "Element ID"}},

    {&tag_length,
     {"Tag length", "wlan.tag.length",
      FT_UINT16, BASE_DEC, NULL, 0, "Length of tag"}},

    {&tag_interpretation,
     {"Tag interpretation", "wlan.tag.interpretation",
      FT_STRING, BASE_NONE, NULL, 0, "Interpretation of tag"}}


  };


  static gint *tree_array[] = { &ett_80211,
    &ett_fc_tree,
    &ett_proto_flags,
    &ett_fixed_parameters,
    &ett_tagged_parameters,
    &ett_wep_parameters,
    &ett_cap_tree,
  };

  proto_wlan = proto_register_protocol ("IEEE 802.11 wireless LAN",
					"IEEE 802.11", "wlan");
  proto_register_field_array (proto_wlan, hf, array_length (hf));
  proto_register_subtree_array (tree_array, array_length (tree_array));
}

void
proto_reg_handoff_wlan(void)
{
  /*
   * Get a handle for the LLC dissector.
   */
  llc_handle = find_dissector("llc");

  dissector_add("wtap_encap", WTAP_ENCAP_IEEE_802_11, dissect_ieee80211,
		proto_wlan);
}
