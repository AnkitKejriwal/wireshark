/*
 * packet-3g-a11.c
 * Routines for CDMA2000 A11 packet trace
 * Copyright 2002, Ryuji Somegawa <somegawa@wide.ad.jp>
 * packet-3g-a11.c was written based on 'packet-mip.c'.
 *
 * packet-mip.c
 * Routines for Mobile IP dissection
 * Copyright 2000, Stefan Raab <sraab@cisco.com>
 *
 * $Id$
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <glib.h>
#include <time.h>

#include <epan/packet.h>

/* Initialize the protocol and registered fields */
static int proto_a11 = -1;
static int hf_a11_type = -1;
static int hf_a11_flags = -1;
static int hf_a11_s = -1;
static int hf_a11_b = -1;
static int hf_a11_d = -1;
static int hf_a11_m = -1;
static int hf_a11_g = -1;
static int hf_a11_v = -1;
static int hf_a11_t = -1;
static int hf_a11_code = -1;
static int hf_a11_status = -1;
static int hf_a11_life = -1;
static int hf_a11_homeaddr = -1;
static int hf_a11_haaddr = -1;
static int hf_a11_coa = -1;
static int hf_a11_ident = -1;
static int hf_a11_ext_type = -1;
static int hf_a11_ext_stype = -1;
static int hf_a11_ext_len = -1;
static int hf_a11_ext = -1;
static int hf_a11_aext_spi = -1;
static int hf_a11_aext_auth = -1;
static int hf_a11_next_nai = -1;

static int hf_a11_ses_key = -1;
static int hf_a11_ses_mnsrid = -1;
static int hf_a11_ses_sidver = -1;
static int hf_a11_ses_msid_type = -1;
static int hf_a11_ses_msid_len = -1;
static int hf_a11_ses_msid = -1;
static int hf_a11_ses_ptype = -1;

static int hf_a11_vse_vid = -1;
static int hf_a11_vse_apptype = -1;
static int hf_a11_vse_canid = -1;
static int hf_a11_vse_panid = -1;
static int hf_a11_vse_srvopt = -1;
static int hf_a11_vse_pdit = -1;
static int hf_a11_vse_code = -1;
static int hf_a11_vse_dormant = -1;
static int hf_a11_vse_ppaddr = -1;

/* Initialize the subtree pointers */
static gint ett_a11 = -1;
static gint ett_a11_flags = -1;
static gint ett_a11_ext = -1;
static gint ett_a11_exts = -1;
static gint ett_a11_radius = -1;
static gint ett_a11_radiuses = -1;

/* Port used for Mobile IP based Tunneling Protocol (A11) */
#define UDP_PORT_3GA11    699
#define NTP_BASETIME 2208988800ul
#define THE3GPP2_VENDOR_ID 0x159f

typedef enum {
    REGISTRATION_REQUEST = 1,
    REGISTRATION_REPLY = 3,
    REGISTRATION_UPDATE = 20,
    REGISTRATION_ACK =21,
    SESSION_UPDATE = 22,
    SESSION_ACK = 23
} a11MessageTypes;

static const value_string a11_types[] = {
  {REGISTRATION_REQUEST, "Registration Request"},
  {REGISTRATION_REPLY,   "Registration Reply"},
  {REGISTRATION_UPDATE,   "Registration Update"},
  {REGISTRATION_ACK,   "Registration Ack"},
  {SESSION_UPDATE,   "Session Update"},
  {SESSION_ACK,   "Session Update Ack"},
  {0, NULL},
};

static const value_string a11_reply_codes[]= {
  {0, "Reg Accepted"},
  {9, "Connection Update"},
#if 0
  {1, "Reg Accepted, but Simultaneous Bindings Unsupported."},
  {64, "Reg Deny (FA)- Unspecified Reason"}, 
  {65, "Reg Deny (FA)- Administratively Prohibited"},
  {66, "Reg Deny (FA)- Insufficient Resources"},
  {67, "Reg Deny (FA)- MN failed Authentication"},
  {68, "Reg Deny (FA)- HA failed Authentication"},
  {69, "Reg Deny (FA)- Requested Lifetime too Long"},
  {70, "Reg Deny (FA)- Poorly Formed Request"},
  {71, "Reg Deny (FA)- Poorly Formed Reply"},
  {72, "Reg Deny (FA)- Requested Encapsulation Unavailable"},
  {73, "Reg Deny (FA)- VJ Compression Unavailable"},
  {74, "Reg Deny (FA)- Requested Reverse Tunnel Unavailable"},
  {75, "Reg Deny (FA)- Reverse Tunnel is Mandatory and 'T' Bit Not Set"},
  {76, "Reg Deny (FA)- Mobile Node Too Distant"},
  {79, "Reg Deny (FA)- Delivery Style Not Supported"},
  {80, "Reg Deny (FA)- Home Network Unreachable"},
  {81, "Reg Deny (FA)- HA Host Unreachable"},
  {82, "Reg Deny (FA)- HA Port Unreachable"},
  {88, "Reg Deny (FA)- HA Unreachable"},
  {96, "Reg Deny (FA)(NAI) - Non Zero Home Address Required"},
  {97, "Reg Deny (FA)(NAI) - Missing NAI"},
  {98, "Reg Deny (FA)(NAI) - Missing Home Agent"},
  {99, "Reg Deny (FA)(NAI) - Missing Home Address"},
#endif
  {128, "Reg Deny (HA)- Unspecified"},
  {129, "Reg Deny (HA)- Administratively Prohibited"},
  {130, "Reg Deny (HA)- Insufficient Resources"},
  {131, "Reg Deny (HA)- PCF Failed Authentication"},
  /* {132, "Reg Deny (HA)- FA Failed Authentication"}, */
  {133, "Reg Deny (HA)- Identification Mismatch"},
  {134, "Reg Deny (HA)- Poorly Formed Request"},
  /* {135, "Reg Deny (HA)- Too Many Simultaneous Bindings"}, */
  {136, "Reg Deny (HA)- Unknown PDSN Address"},
  {137, "Reg Deny (HA)- Requested Reverse Tunnel Unavailable"},
  {138, "Reg Deny (HA)- Reverse Tunnel is Mandatory and 'T' Bit Not Set"},
  {139, "Reg Deny (HA)- Requested Encapsulation Unavailable"},
  {141, "Reg Deny (HA)- unsupported Vendor ID / Application Type in CVSE"},
};


static const value_string a11_ack_status[]= {
  {0, "Update Accepted"},
  {128, "Update Deny - Unspecified"},
  {131, "Update Deny - Sending Node Failed Authentication"},
  {133, "Update Deny - Registration ID Mismatch"},
  {134, "Update Deny - Poorly Formed Request"},
  {201, "Update Deny - Session Parameter Not Updated"},
};

typedef enum {
  MH_AUTH_EXT = 32,
  MF_AUTH_EXT = 33,
  FH_AUTH_EXT = 34,
  GEN_AUTH_EXT = 36,      /* RFC 3012 */
  OLD_CVSE_EXT = 37,      /* RFC 3115 */
  CVSE_EXT = 38,          /* RFC 3115 */
  SS_EXT = 39,            /* 3GPP2 IOS4.2 */
  RU_AUTH_EXT = 40,       /* 3GPP2 IOS4.2 */
  MN_NAI_EXT = 131,
  MF_CHALLENGE_EXT = 132, /* RFC 3012 */
  OLD_NVSE_EXT = 133,     /* RFC 3115 */
  NVSE_EXT = 134          /* RFC 3115 */
} MIP_EXTS;


static const value_string a11_ext_types[]= {
  {MH_AUTH_EXT, "Mobile-Home Authentication Extension"},
  {MF_AUTH_EXT, "Mobile-Foreign Authentication Extension"},
  {FH_AUTH_EXT, "Foreign-Home Authentication Extension"},
  {MN_NAI_EXT,  "Mobile Node NAI Extension"},
  {GEN_AUTH_EXT, "Generalized Mobile-IP Authentication Extension"},
  {MF_CHALLENGE_EXT, "MN-FA Challenge Extension"},
  {CVSE_EXT, "Critical Vendor/Organization Specific Extension"},
  {SS_EXT, "Session Specific Extension"},
  {RU_AUTH_EXT, "Registration Update Authentication Extension"},
  {OLD_CVSE_EXT, "Critical Vendor/Organization Specific Extension (OLD)"},
  {NVSE_EXT, "Normal Vendor/Organization Specific Extension"},
  {OLD_NVSE_EXT, "Normal Vendor/Organization Specific Extension (OLD)"},
  {0, NULL},
};

static const value_string a11_ext_stypes[]= {
  {1, "MN AAA Extension"},
  {0, NULL},
};

static const value_string a11_ext_nvose_srvopt[]= {
  {0x0021, "3G High Speed Packet Data"},
  {0x003C, "Link Layer Assisted Header Removal"},
  {0x003D, "Link Layer Assisted Robust Header Compression"},
  {0, NULL},
};

static const value_string a11_ext_nvose_pdsn_code[]= {
  {0xc1, "Connection Release - reason unspecified"},
  {0xc2, "Connection Release - PPP time-out"},
  {0xc3, "Connection Release - registration time-out"},
  {0xc4, "Connection Release - PDSN error"},
  {0xc5, "Connection Release - inter-PCF handoff"},
  {0xc6, "Connection Release - inter-PDSN handoff"},
  {0xc7, "Connection Release - PDSN OAM&P intervention"},
  {0xc8, "Connection Release - accounting error"},
  {0xca, "Connection Release - user (NAI) failed authentication"},
  {0x00, NULL},
};

static const value_string a11_ext_dormant[]= {
  {0x0000, "all MS packet data service instances are dormant"},
  {0, NULL},
};

static const value_string a11_ext_app[]= {
  {0x0101, "Accounting (RADIUS)"},
  {0x0102, "Accounting (DIAMETER)"},
  {0x0201, "Mobility Event Indicator (Mobility)"},
  {0x0301, "Data Available Indicator (Data Ready to Send)"},
  {0x0401, "Access Network Identifiers (ANID)"},
  {0x0501, "PDSN Identifiers (Anchor P-P Address)"},
  {0x0601, "Indicators (All Dormant Indicator)"},
  {0x0701, "PDSN Code (PDSN Code)"},
  {0x0801, "Session Parameter (RN-PDIT:Radio Network Packet Data Inactivity Timer)"},
  {0x0901, "Service Option (Service Option Value)"},
  {0, NULL},
};

static const value_string a11_airlink_types[]= {
  {1, "Session Setup (Y=1)"},
  {2, "Active Start (Y=2)"},
  {3, "Active Stop (Y=3)"},
  {4, "Short Data Burst (Y=4)"},
  {0, NULL},
};

#define ATTRIBUTE_NAME_LEN_MAX 128
#define ATTR_TYPE_NULL 0
#define ATTR_TYPE_INT 1 
#define ATTR_TYPE_STR 2 
#define ATTR_TYPE_IPV4 3
#define ATTR_TYPE_TYPE 4 
#define ATTR_TYPE_MSID 5 

struct radius_attribute {
  char attrname[ATTRIBUTE_NAME_LEN_MAX];
  int type;
  int subtype;
  int bytes;
  int data_type;
};

static const struct radius_attribute attrs[]={
  {"Airlink Record",          26, 40,  4, ATTR_TYPE_TYPE},
  {"R-P Session ID",          26, 41,  4, ATTR_TYPE_INT},
  {"Airlink Sequence Number", 26, 42,  4, ATTR_TYPE_INT},
#if 0
  {"MSID",                    31, -1, 15, ATTR_TYPE_MSID},
#endif
  {"Serving PCF",             26,  9,  4, ATTR_TYPE_IPV4},
  {"BSID",                    26, 10, 12, ATTR_TYPE_STR},
  {"ESN",                     26, 52, 15, ATTR_TYPE_STR},
  {"User Zone",               26, 11,  4, ATTR_TYPE_INT},
  {"Forward FCH Mux Option",  26, 12,  4, ATTR_TYPE_INT},
  {"Reverse FCH Mux Option",  26, 13,  4, ATTR_TYPE_INT},
  {"Forward Fundamental Rate (IOS 4.1)",26, 14,  4, ATTR_TYPE_INT},
  {"Reverse Fundamental Rate (IOS 4.1)",26, 15,  4, ATTR_TYPE_INT},
  {"Service Option",          26, 16,  4, ATTR_TYPE_INT},
  {"Forward Traffic Type",    26, 17,  4, ATTR_TYPE_INT},
  {"Reverse Traffic Type",    26, 18,  4, ATTR_TYPE_INT},
  {"FCH Frame Size",          26, 19,  4, ATTR_TYPE_INT},
  {"Forward FCH RC",          26, 20,  4, ATTR_TYPE_INT},
  {"Reverse FCH RC",          26, 21,  4, ATTR_TYPE_INT},
  {"DCCH Frame Size 0/5/20",  26, 50,  4, ATTR_TYPE_INT},
  {"Forward DCCH Mux Option", 26, 84,  4, ATTR_TYPE_INT},
  {"Reverse DCCH Mux Option", 26, 85,  4, ATTR_TYPE_INT},
  {"Forward DCCH RC",         26, 86,  4, ATTR_TYPE_INT},
  {"Reverse DCCH RC",         26, 87,  4, ATTR_TYPE_INT},
  {"Airlink Priority",        26, 39,  4, ATTR_TYPE_INT},
  {"Active Connection Time",  26, 49,  4, ATTR_TYPE_INT},
  {"Mobile Orig/Term Ind.",   26, 45,  4, ATTR_TYPE_INT},
  {"SDB Octet Count (Term.)", 26, 31,  4, ATTR_TYPE_INT},
  {"SDB Octet Count (Orig.)", 26, 32,  4, ATTR_TYPE_INT},
  {"Unknown",                 -1, -1, -1, ATTR_TYPE_NULL},
};
#define NUM_ATTR (sizeof(attrs)/sizeof(struct radius_attribute))

#define MAX_STRVAL 16

#define RADIUS_VENDOR_SPECIFIC 26
#define SKIP_HDR_LEN 6

static void dissect_a11_radius( tvbuff_t *, int, proto_tree *, int);

/* RADIUS attributed */
static void
dissect_a11_radius( tvbuff_t *tvb, int offset, proto_tree *tree, int app_len)
{
  proto_item *ti;
  proto_tree *radius_tree=NULL;
  size_t     radius_len;
  guint8     radius_type;
  guint8     radius_subtype;
  int        attribute_type;
  guint      attribute_len;
  guint      offset0;
  guint      radius_offset;
  guint      i;
  guchar     str_val[MAX_STRVAL];
  guint      radius_vendor_id = 0;

  /* None of this really matters if we don't have a tree */
  if (!tree) return;

  offset0 = offset;

  /* return if length of extension is not valid */
  if (tvb_reported_length_remaining(tvb, offset)  < 12) {
    return;
  }

  ti = proto_tree_add_text(tree, tvb, offset - 2, app_len, "Airlink Record");

  radius_tree = proto_item_add_subtree(ti, ett_a11_radiuses);

  /* And, handle each record */
  while (tvb_reported_length_remaining(tvb, offset) > 0
         && ((int)offset-(int)offset0) <app_len-2)
  {

    radius_type = tvb_get_guint8(tvb, offset);
	radius_len = tvb_get_guint8(tvb, offset + 1);

    if (radius_type == RADIUS_VENDOR_SPECIFIC)
    {
      radius_vendor_id = tvb_get_ntohl(tvb, offset +2); 

      if(radius_vendor_id != THE3GPP2_VENDOR_ID)
      {
        ti = proto_tree_add_text(radius_tree, tvb, offset, radius_len,
                "Unknown Vendor-specific Attribute (Vendor Id: %x)", radius_vendor_id);
        offset += radius_len;
        continue;
      }
    }
    else
    {

      /**** ad-hoc ***/
      if(radius_type == 31)
      {
        strncpy(str_val, tvb_get_ptr(tvb,offset+2,radius_len-2), 
                radius_len-2);
        if(radius_len-2 < MAX_STRVAL) {
          str_val[radius_len-2] = '\0';
        }
        else {
          str_val[MAX_STRVAL-1] = '\0';
        }
        ti = proto_tree_add_text(radius_tree, tvb, offset, radius_len,
            "MSID: %s", str_val);
      }
      else if (radius_type == 46)
      {
        ti = proto_tree_add_text(radius_tree, tvb, offset, radius_len,
            "Acct Session Time: %d",tvb_get_ntohl(tvb,offset+2));
      }
      else
      {
        ti = proto_tree_add_text(radius_tree, tvb, offset, radius_len,
              "Unknown RADIUS Attributes (Type: %d)", radius_type);
      }

      offset += radius_len;
      continue;
    }
    
	radius_len = tvb_get_guint8(tvb, offset + 1);

    offset += SKIP_HDR_LEN;
    radius_offset = 0;

	/* Detect Airlink Record Type */

    while (radius_len - 6 > radius_offset)
    {

      radius_subtype = tvb_get_guint8(tvb, offset + radius_offset);	
      attribute_len = tvb_get_guint8(tvb, offset + radius_offset + 1);	

      attribute_type = -1;
      for(i = 0; i < NUM_ATTR; i++) {
        if (attrs[i].subtype == radius_subtype) {
            attribute_type = i;
            break;
        }
	  }

      if(attribute_type >= 0) {
        switch(attrs[attribute_type].data_type) {
        case ATTR_TYPE_INT:
          ti = proto_tree_add_text(radius_tree, tvb, offset + radius_offset,
            attribute_len, "3GPP2: %s (%04x)", attrs[attribute_type].attrname,
            tvb_get_ntohl(tvb,offset + radius_offset + 2));
          break;
        case ATTR_TYPE_IPV4:
          ti = proto_tree_add_text(radius_tree, tvb, offset + radius_offset,
            attribute_len, "3GPP2: %s (%s)", attrs[attribute_type].attrname,
            ip_to_str(tvb_get_ptr(tvb,offset + radius_offset + 2,4)));
          break;
        case ATTR_TYPE_TYPE:
          ti = proto_tree_add_text(radius_tree, tvb, offset + radius_offset,
            attribute_len, "3GPP2: %s (%s)", attrs[attribute_type].attrname,
            val_to_str(tvb_get_ntohl(tvb,offset+radius_offset+2),
            a11_airlink_types,"Unknown"));
          break;
        case ATTR_TYPE_STR:
          strncpy(str_val, tvb_get_ptr(tvb,offset+radius_offset+2,attribute_len - 2),
             attribute_len - 2);
          if(attribute_len - 2 < MAX_STRVAL) {
             str_val[attribute_len - 2] = '\0';
          }
          else {
            str_val[MAX_STRVAL-1] = '\0';
          }
          ti = proto_tree_add_text(radius_tree, tvb, offset+radius_offset,
             attribute_len,
            "3GPP2: %s (%s)", attrs[attribute_type].attrname, str_val);
          break;
        case ATTR_TYPE_NULL:
          break;
        default:
          ti = proto_tree_add_text(radius_tree, tvb, offset, radius_len,
            "RADIUS: %s", attrs[attribute_type].attrname);
          break;
        }
      }
      else {
        ti = proto_tree_add_text(radius_tree, tvb, offset, radius_len,
          "RADIUS: Unknown 3GPP2 Attribute (Type:%d, SubType:%d)",
          radius_type,radius_subtype);
      }

      radius_offset += attribute_len;
    }
    offset += radius_len - 6;

  }

}

/* Code to dissect extensions */
static void
dissect_a11_extensions( tvbuff_t *tvb, int offset, proto_tree *tree)
{
  proto_item   *ti;
  proto_tree   *exts_tree=NULL;
  proto_tree   *ext_tree;
  size_t        ext_len;
  guint8        ext_type;
  guint8        ext_subtype=0;
  size_t        hdrLen;

  gint16       apptype = -1;

  /* None of this really matters if we don't have a tree */
  if (!tree) return;

  /* Add our tree, if we have extensions */
  ti = proto_tree_add_text(tree, tvb, offset, -1, "Extensions");
  exts_tree = proto_item_add_subtree(ti, ett_a11_exts);

  /* And, handle each extension */
  while (tvb_reported_length_remaining(tvb, offset) > 0) {

	/* Get our extension info */
	ext_type = tvb_get_guint8(tvb, offset);
	if (ext_type == GEN_AUTH_EXT) {
	  /*
	   * Very nasty . . breaks normal extensions, since the length is
	   * in the wrong place :(
	   */
	  ext_subtype = tvb_get_guint8(tvb, offset + 1);
	  ext_len = tvb_get_ntohs(tvb, offset + 2);
	  hdrLen = 4;
    } else if (ext_type == CVSE_EXT || ext_type == OLD_CVSE_EXT) {
	  ext_len = tvb_get_ntohs(tvb, offset + 2);
      ext_subtype = tvb_get_guint8(tvb, offset + 8);	
      hdrLen = 4;
	} else {
	  ext_len = tvb_get_guint8(tvb, offset + 1);
	  hdrLen = 2;
	}
	
	ti = proto_tree_add_text(exts_tree, tvb, offset, ext_len + hdrLen,
				 "Extension: %s",
				 val_to_str(ext_type, a11_ext_types,
				            "Unknown Extension %u"));
	ext_tree = proto_item_add_subtree(ti, ett_a11_ext);

	proto_tree_add_item(ext_tree, hf_a11_ext_type, tvb, offset, 1, ext_type);
	offset++;

	if (ext_type == SS_EXT) {
	  proto_tree_add_uint(ext_tree, hf_a11_ext_len, tvb, offset, 1, ext_len);
	  offset++;
	}
	else if(ext_type == CVSE_EXT || ext_type == OLD_CVSE_EXT) {
	  offset++;
	  proto_tree_add_uint(ext_tree, hf_a11_ext_len, tvb, offset, 2, ext_len);
	  offset+=2;
	}
	else if (ext_type != GEN_AUTH_EXT) {
	  /* Another nasty hack since GEN_AUTH_EXT broke everything */
	  proto_tree_add_uint(ext_tree, hf_a11_ext_len, tvb, offset, 1, ext_len);
	  offset++;
	}

	switch(ext_type) {
	case SS_EXT:
	  proto_tree_add_item(ext_tree, hf_a11_ses_ptype, tvb, offset, 2, FALSE);
	  proto_tree_add_item(ext_tree, hf_a11_ses_key, tvb, offset+2, 4, FALSE);
	  proto_tree_add_item(ext_tree, hf_a11_ses_sidver, tvb, offset+7, 1, FALSE);
	  proto_tree_add_item(ext_tree, hf_a11_ses_mnsrid, tvb, offset+8, 2, FALSE);
	  proto_tree_add_item(ext_tree, hf_a11_ses_msid_type, tvb, offset+10, 2, FALSE);
	  proto_tree_add_item(ext_tree, hf_a11_ses_msid_len, tvb, offset+12, 1, FALSE);
	  proto_tree_add_item(ext_tree, hf_a11_ses_msid, tvb, offset+13, ext_len-13, FALSE);
	  break;
	case MH_AUTH_EXT:
	case MF_AUTH_EXT:
	case FH_AUTH_EXT:
	  /* All these extensions look the same.  4 byte SPI followed by a key */
	  proto_tree_add_item(ext_tree, hf_a11_aext_spi, tvb, offset, 4, FALSE);
	  proto_tree_add_item(ext_tree, hf_a11_aext_auth, tvb, offset+4, ext_len-4,
						  FALSE);
	  break;
	case MN_NAI_EXT:
	  proto_tree_add_item(ext_tree, hf_a11_next_nai, tvb, offset, 
						  ext_len, FALSE);
	  break;

	case GEN_AUTH_EXT:      /* RFC 3012 */
	  /*
	   * Very nasty . . breaks normal extensions, since the length is
	   * in the wrong place :(
	   */
	  proto_tree_add_uint(ext_tree, hf_a11_ext_stype, tvb, offset, 1, ext_subtype);
	  offset++;
	  proto_tree_add_uint(ext_tree, hf_a11_ext_len, tvb, offset, 2, ext_len);
	  offset+=2;
	  /* SPI */
	  proto_tree_add_item(ext_tree, hf_a11_aext_spi, tvb, offset, 4, FALSE);
	  /* Key */
	  proto_tree_add_item(ext_tree, hf_a11_aext_auth, tvb, offset + 4,
						  ext_len - 4, FALSE);
	  
	  break;
	case OLD_CVSE_EXT:      /* RFC 3115 */
	case CVSE_EXT:          /* RFC 3115 */
	  proto_tree_add_item(ext_tree, hf_a11_vse_vid, tvb, offset, 4, FALSE);
	  proto_tree_add_item(ext_tree, hf_a11_vse_apptype, tvb, offset+4, 2, FALSE);
	  apptype = tvb_get_ntohs(tvb, offset+4);
      if(apptype == 0x0101) {
        if (tvb_reported_length_remaining(tvb, offset) > 0) {
          dissect_a11_radius(tvb, offset+6, ext_tree, ext_len + hdrLen - 8);
        }
      }
	  break;
	case OLD_NVSE_EXT:      /* RFC 3115 */
	case NVSE_EXT:          /* RFC 3115 */
	  proto_tree_add_item(ext_tree, hf_a11_vse_vid, tvb, offset+2, 4, FALSE);
	  proto_tree_add_item(ext_tree, hf_a11_vse_apptype, tvb, offset+6, 2, FALSE);

	  apptype = tvb_get_ntohs(tvb, offset+6);
      switch(apptype) {
        case 0x0401:
          proto_tree_add_item(ext_tree, hf_a11_vse_panid, tvb, offset+8, 5, FALSE);
          proto_tree_add_item(ext_tree, hf_a11_vse_canid, tvb, offset+13, 5, FALSE);
          break;
        case 0x0501:
          proto_tree_add_item(ext_tree, hf_a11_vse_ppaddr, tvb, offset+8, 4, FALSE);
          break;
        case 0x0601:
          proto_tree_add_item(ext_tree, hf_a11_vse_dormant, tvb, offset+8, 2, FALSE);
          break;
        case 0x0701:
          proto_tree_add_item(ext_tree, hf_a11_vse_code, tvb, offset+8, 1, FALSE);
          break;
        case 0x0801:
          proto_tree_add_item(ext_tree, hf_a11_vse_pdit, tvb, offset+8, 1, FALSE);
          break;
        case 0x0802:
          proto_tree_add_text(ext_tree, tvb, offset+8, -1, "Session Parameter - Always On");
          break;
        case 0x0901:
          proto_tree_add_item(ext_tree, hf_a11_vse_srvopt, tvb, offset+8, 2, FALSE);
          break;
      }

	  break;
	case MF_CHALLENGE_EXT:  /* RFC 3012 */
	  /* The default dissector is good here.  The challenge is all hex anyway. */
	default:
	  proto_tree_add_item(ext_tree, hf_a11_ext, tvb, offset, ext_len, FALSE);
	  break;
	} /* ext type */

	offset += ext_len;
  } /* while data remaining */

} /* dissect_a11_extensions */

/* Code to actually dissect the packets */
static int
dissect_a11( tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
  /* Set up structures we will need to add the protocol subtree and manage it */
  proto_item	*ti;
  proto_tree	*a11_tree=NULL;
  proto_item    *tf;
  proto_tree    *flags_tree;
  guint8         type;
  guint8         flags;
  nstime_t       ident_time;
  size_t         offset=0;
  
  if (!tvb_bytes_exist(tvb, offset, 1))
	return 0;	/* not enough data to check message type */

  type = tvb_get_guint8(tvb, offset);
  if (match_strval(type, a11_types) == NULL)
	return 0;	/* not a known message type */

  /* Make entries in Protocol column and Info column on summary display */
  
  if (check_col(pinfo->cinfo, COL_PROTOCOL)) 
	col_set_str(pinfo->cinfo, COL_PROTOCOL, "3GPP2 A11");
  if (check_col(pinfo->cinfo, COL_INFO)) 
	col_clear(pinfo->cinfo, COL_INFO);

  switch (type) {
  case REGISTRATION_REQUEST:
	if (check_col(pinfo->cinfo, COL_INFO)) 
	  col_add_fstr(pinfo->cinfo, COL_INFO, "Reg Request: PDSN=%s PCF=%s", 
				   ip_to_str(tvb_get_ptr(tvb, 8, 4)),
				   ip_to_str(tvb_get_ptr(tvb,12,4)));
	
	if (tree) {
	  ti = proto_tree_add_item(tree, proto_a11, tvb, offset, -1, FALSE);
	  a11_tree = proto_item_add_subtree(ti, ett_a11);
	  
	  /* type */
	  proto_tree_add_uint(a11_tree, hf_a11_type, tvb, offset, 1, type);
	  offset++;
	  
	  /* flags */
	  flags = tvb_get_guint8(tvb, offset);
	  tf = proto_tree_add_uint(a11_tree, hf_a11_flags, tvb,
							   offset, 1, flags);
	  flags_tree = proto_item_add_subtree(tf, ett_a11_flags);
	  proto_tree_add_boolean(flags_tree, hf_a11_s, tvb, offset, 1, flags);
	  proto_tree_add_boolean(flags_tree, hf_a11_b, tvb, offset, 1, flags);
	  proto_tree_add_boolean(flags_tree, hf_a11_d, tvb, offset, 1, flags);
	  proto_tree_add_boolean(flags_tree, hf_a11_m, tvb, offset, 1, flags);
	  proto_tree_add_boolean(flags_tree, hf_a11_g, tvb, offset, 1, flags);
	  proto_tree_add_boolean(flags_tree, hf_a11_v, tvb, offset, 1, flags);
	  proto_tree_add_boolean(flags_tree, hf_a11_t, tvb, offset, 1, flags);
	  offset++;

	  /* lifetime */
	  proto_tree_add_item(a11_tree, hf_a11_life, tvb, offset, 2, FALSE);
	  offset +=2;
	  
	  /* home address */
	  proto_tree_add_item(a11_tree, hf_a11_homeaddr, tvb, offset, 4, FALSE);
	  offset += 4;
	  
	  /* home agent address */
	  proto_tree_add_item(a11_tree, hf_a11_haaddr, tvb, offset, 4, FALSE);
	  offset += 4;
	  
	  /* Care of Address */
	  proto_tree_add_item(a11_tree, hf_a11_coa, tvb, offset, 4, FALSE);
	  offset += 4;

	  /* Identifier */
	  ident_time.secs =  tvb_get_ntohl(tvb,16)-(guint32) NTP_BASETIME;
	  ident_time.nsecs = tvb_get_ntohl(tvb,20)*1000;
	  proto_tree_add_time(a11_tree, hf_a11_ident, tvb, offset, 8, &ident_time);
	  offset += 8;
		
	} /* if tree */
	break;
  case REGISTRATION_REPLY:
	if (check_col(pinfo->cinfo, COL_INFO)) 
	  col_add_fstr(pinfo->cinfo, COL_INFO, "Reg Reply:   PDSN=%s, Code=%u", 
				   ip_to_str(tvb_get_ptr(tvb,8,4)), tvb_get_guint8(tvb,1));
	
	if (tree) {
	  /* Add Subtree */
	  ti = proto_tree_add_item(tree, proto_a11, tvb, offset, -1, FALSE);
	  a11_tree = proto_item_add_subtree(ti, ett_a11);
	  
	  /* Type */
  	  proto_tree_add_uint(a11_tree, hf_a11_type, tvb, offset, 1, type);
	  offset++;
	  
	  /* Reply Code */
	  proto_tree_add_item(a11_tree, hf_a11_code, tvb, offset, 1, FALSE);
	  offset++;

	  /* Registration Lifetime */
	  proto_tree_add_item(a11_tree, hf_a11_life, tvb, offset, 2, FALSE);
	  offset += 2;

	  /* Home address */
	  proto_tree_add_item(a11_tree, hf_a11_homeaddr, tvb, offset, 4, FALSE);
	  offset += 4;

	  /* Home Agent Address */
	  proto_tree_add_item(a11_tree, hf_a11_haaddr, tvb, offset, 4, FALSE);
	  offset += 4;

	  /* Identifier */
	  ident_time.secs =  tvb_get_ntohl(tvb,12)-(guint32) NTP_BASETIME;
	  ident_time.nsecs = tvb_get_ntohl(tvb,16)*1000;
	  proto_tree_add_time(a11_tree, hf_a11_ident, tvb, offset, 8, &ident_time);
	  offset += 8;
	} /* if tree */
	
	break;
  case REGISTRATION_UPDATE:
	if (check_col(pinfo->cinfo, COL_INFO)) 
	  col_add_fstr(pinfo->cinfo, COL_INFO,"Reg Update:  PDSN=%s", 
				   ip_to_str(tvb_get_ptr(tvb,8,4)));
	if (tree) {
	  /* Add Subtree */
	  ti = proto_tree_add_item(tree, proto_a11, tvb, offset, -1, FALSE);
	  a11_tree = proto_item_add_subtree(ti, ett_a11);
	  
	  /* Type */
  	  proto_tree_add_uint(a11_tree, hf_a11_type, tvb, offset, 1, type);
	  offset++;

	  /* Reserved */
	  offset+=3;
	
	  /* Home address */
	  proto_tree_add_item(a11_tree, hf_a11_homeaddr, tvb, offset, 4, FALSE);
	  offset += 4;

	  /* Home Agent Address */
	  proto_tree_add_item(a11_tree, hf_a11_haaddr, tvb, offset, 4, FALSE);
	  offset += 4;

	  /* Identifier */
	  ident_time.secs =  tvb_get_ntohl(tvb,12)-(guint32) NTP_BASETIME;
	  ident_time.nsecs = tvb_get_ntohl(tvb,16)*1000;
	  proto_tree_add_time(a11_tree, hf_a11_ident, tvb, offset, 8, &ident_time);
	  offset += 8;

	} /* if tree */
	break;
  case REGISTRATION_ACK:
	if (check_col(pinfo->cinfo, COL_INFO)) 
	  col_add_fstr(pinfo->cinfo, COL_INFO, "Reg Ack:     PCF=%s Status=%u", 
				   ip_to_str(tvb_get_ptr(tvb, 8, 4)),
				   tvb_get_guint8(tvb,3));
	if (tree) {
	  /* Add Subtree */
	  ti = proto_tree_add_item(tree, proto_a11, tvb, offset, -1, FALSE);
	  a11_tree = proto_item_add_subtree(ti, ett_a11);
	  
	  /* Type */
  	  proto_tree_add_uint(a11_tree, hf_a11_type, tvb, offset, 1, type);
	  offset++;
	
	  /* Reserved */
	  offset+=2;

	  /* Ack Status */
	  proto_tree_add_item(a11_tree, hf_a11_status, tvb, offset, 1, FALSE);
	  offset++;

	  /* Home address */
	  proto_tree_add_item(a11_tree, hf_a11_homeaddr, tvb, offset, 4, FALSE);
	  offset += 4;

	  /* Care of Address */
	  proto_tree_add_item(a11_tree, hf_a11_coa, tvb, offset, 4, FALSE);
	  offset += 4;

	  /* Identifier */
	  ident_time.secs =  tvb_get_ntohl(tvb,12)-(guint32) NTP_BASETIME;
	  ident_time.nsecs = tvb_get_ntohl(tvb,16)*1000;
	  proto_tree_add_time(a11_tree, hf_a11_ident, tvb, offset, 8, &ident_time);
	  offset += 8;

	} /* if tree */
	break;
  case SESSION_UPDATE: /* IOS4.3 */
	if (check_col(pinfo->cinfo, COL_INFO)) 
	  col_add_fstr(pinfo->cinfo, COL_INFO,"Ses Update:  PDSN=%s", 
				   ip_to_str(tvb_get_ptr(tvb,8,4)));
	if (tree) {
	  /* Add Subtree */
	  ti = proto_tree_add_item(tree, proto_a11, tvb, offset, -1, FALSE);
	  a11_tree = proto_item_add_subtree(ti, ett_a11);
	  
	  /* Type */
  	  proto_tree_add_uint(a11_tree, hf_a11_type, tvb, offset, 1, type);
	  offset++;

	  /* Reserved */
	  offset+=3;
	
	  /* Home address */
	  proto_tree_add_item(a11_tree, hf_a11_homeaddr, tvb, offset, 4, FALSE);
	  offset += 4;

	  /* Home Agent Address */
	  proto_tree_add_item(a11_tree, hf_a11_haaddr, tvb, offset, 4, FALSE);
	  offset += 4;

	  /* Identifier */
	  ident_time.secs =  tvb_get_ntohl(tvb,12)-(guint32) NTP_BASETIME;
	  ident_time.nsecs = tvb_get_ntohl(tvb,16)*1000;
	  proto_tree_add_time(a11_tree, hf_a11_ident, tvb, offset, 8, &ident_time);
	  offset += 8;

	} /* if tree */
	break;
  case SESSION_ACK: /* IOS4.3 */
	if (check_col(pinfo->cinfo, COL_INFO)) 
	  col_add_fstr(pinfo->cinfo, COL_INFO, "Ses Upd Ack: PCF=%s, Status=%u", 
				   ip_to_str(tvb_get_ptr(tvb, 8, 4)),
				   tvb_get_guint8(tvb,3));
	if (tree) {
	  /* Add Subtree */
	  ti = proto_tree_add_item(tree, proto_a11, tvb, offset, -1, FALSE);
	  a11_tree = proto_item_add_subtree(ti, ett_a11);
	  
	  /* Type */
  	  proto_tree_add_uint(a11_tree, hf_a11_type, tvb, offset, 1, type);
	  offset++;
	
	  /* Reserved */
	  offset+=2;

	  /* Ack Status */
	  proto_tree_add_item(a11_tree, hf_a11_status, tvb, offset, 1, FALSE);
	  offset++;

	  /* Home address */
	  proto_tree_add_item(a11_tree, hf_a11_homeaddr, tvb, offset, 4, FALSE);
	  offset += 4;

	  /* Care of Address */
	  proto_tree_add_item(a11_tree, hf_a11_coa, tvb, offset, 4, FALSE);
	  offset += 4;

	  /* Identifier */
	  ident_time.secs =  tvb_get_ntohl(tvb,12)-(guint32) NTP_BASETIME;
	  ident_time.nsecs = tvb_get_ntohl(tvb,16)*1000;
	  proto_tree_add_time(a11_tree, hf_a11_ident, tvb, offset, 8, &ident_time);
	  offset += 8;

	} /* if tree */
	break;
  } /* End switch */

  if (tree) {
	if (tvb_reported_length_remaining(tvb, offset) > 0)
	  dissect_a11_extensions(tvb, offset, a11_tree);
  }
  return tvb_length(tvb);
} /* dissect_a11 */

/* Register the protocol with Ethereal */
void proto_register_a11(void)
{                 

/* Setup list of header fields */
	static hf_register_info hf[] = {
	  { &hf_a11_type,
		 { "Message Type",           "a11.type",
			FT_UINT8, BASE_DEC, VALS(a11_types), 0,          
			"A11 Message type.", HFILL }
	  },
	  { &hf_a11_flags,
		{"Flags", "a11.flags",
		 FT_UINT8, BASE_HEX, NULL, 0x0,
		 "", HFILL}
	  },
	  { &hf_a11_s,
		 {"Simultaneous Bindings",           "a11.s",

		   FT_BOOLEAN, 8, NULL, 128,          
		   "Simultaneous Bindings Allowed", HFILL }
	  },
	  { &hf_a11_b,
		 {"Broadcast Datagrams",           "a11.b",
		   FT_BOOLEAN, 8, NULL, 64,          
		   "Broadcast Datagrams requested", HFILL }
	  },
	  { &hf_a11_d,
		 { "Co-lcated Care-of Address",           "a11.d",
		   FT_BOOLEAN, 8, NULL, 32,          
		   "MN using Co-located Care-of address", HFILL }
	  },
	  { &hf_a11_m,
		 {"Minimal Encapsulation",           "a11.m",
		   FT_BOOLEAN, 8, NULL, 16,          
		   "MN wants Minimal encapsulation", HFILL }
	  },
	  { &hf_a11_g,
		 {"GRE",           "a11.g",
		   FT_BOOLEAN, 8, NULL, 8,          
		   "MN wants GRE encapsulation", HFILL }
	  },
	  { &hf_a11_v,
		 { "Van Jacobson",           "a11.v",
		   FT_BOOLEAN, 8, NULL, 4,          
		   "Van Jacobson", HFILL }
	  },
	  { &hf_a11_t,
		 { "Reverse Tunneling",           "a11.t",
		   FT_BOOLEAN, 8, NULL, 2,          
		   "Reverse tunneling requested", HFILL }
	  },
	  { &hf_a11_code,
		 { "Reply Code",           "a11.code",
			FT_UINT8, BASE_DEC, VALS(a11_reply_codes), 0,          
			"A11 Registration Reply code.", HFILL }
	  },
	  { &hf_a11_status,
		 { "Reply Status",           "a11.ackstat",
			FT_UINT8, BASE_DEC, VALS(a11_ack_status), 0,          
			"A11 Registration Ack Status.", HFILL }
	  },
	  { &hf_a11_life,
		 { "Lifetime",           "a11.life",
			FT_UINT16, BASE_DEC, NULL, 0,          
			"A11 Registration Lifetime.", HFILL }
	  },
	  { &hf_a11_homeaddr,
		 { "Home Address",           "a11.homeaddr",
			FT_IPv4, BASE_NONE, NULL, 0,          
			"Mobile Node's home address.", HFILL }
	  },
	  
	  { &hf_a11_haaddr,
		 { "Home Agent",           "a11.haaddr",
			FT_IPv4, BASE_NONE, NULL, 0,          
			"Home agent IP Address.", HFILL }
	  },
	  { &hf_a11_coa,
		 { "Care of Address",           "a11.coa",
			FT_IPv4, BASE_NONE, NULL, 0,          
			"Care of Address.", HFILL }
	  },
	  { &hf_a11_ident,
		 { "Identification",           "a11.ident",
			FT_ABSOLUTE_TIME, BASE_NONE, NULL, 0,          
			"MN Identification.", HFILL }
	  },
	  { &hf_a11_ext_type,
		 { "Extension Type",           "a11.ext.type",
			FT_UINT8, BASE_DEC, VALS(a11_ext_types), 0,          
			"Mobile IP Extension Type.", HFILL }
	  },
	  { &hf_a11_ext_stype,
		 { "Gen Auth Ext SubType",           "a11.ext.auth.subtype",
			FT_UINT8, BASE_DEC, VALS(a11_ext_stypes), 0,          
			"Mobile IP Auth Extension Sub Type.", HFILL }
	  },
	  { &hf_a11_ext_len,
		 { "Extension Length",         "a11.ext.len",
			FT_UINT16, BASE_DEC, NULL, 0,
			"Mobile IP Extension Length.", HFILL }
	  },
	  { &hf_a11_ext,
		 { "Extension",                      "a11.extension",
			FT_BYTES, BASE_HEX, NULL, 0,
			"Extension", HFILL }
	  },
	  { &hf_a11_aext_spi,
		 { "SPI",                      "a11.auth.spi",
			FT_UINT32, BASE_HEX, NULL, 0,
			"Authentication Header Security Parameter Index.", HFILL }
	  },
	  { &hf_a11_aext_auth,
		 { "Authenticator",            "a11.auth.auth",
			FT_BYTES, BASE_NONE, NULL, 0,
			"Authenticator.", HFILL }
	  },
	  { &hf_a11_next_nai,
		 { "NAI",                      "a11.nai",
			FT_STRING, BASE_NONE, NULL, 0,
			"NAI", HFILL }
	  },
	  { &hf_a11_ses_key,
		 { "Key",                      "a11.ext.key",
			FT_UINT32, BASE_HEX, NULL, 0,
			"Session Key.", HFILL }
	  },
	  { &hf_a11_ses_sidver,
		{ "Session ID Version",         "a11.ext.sidver",
			FT_UINT8, BASE_DEC, NULL, 3,
			"Session ID Version", HFILL}
	  },
	  { &hf_a11_ses_mnsrid,
		 { "MNSR-ID",                      "a11.ext.mnsrid",
			FT_UINT16, BASE_HEX, NULL, 0,
			"MNSR-ID", HFILL }
	  },
	  { &hf_a11_ses_msid_type,
		 { "MSID Type",                      "a11.ext.msid_type",
			FT_UINT16, BASE_DEC, NULL, 0,
			"MSID Type.", HFILL }
	  },
	  { &hf_a11_ses_msid_len,
		 { "MSID Length",                      "a11.ext.msid_len",
			FT_UINT8, BASE_DEC, NULL, 0,
			"MSID Length.", HFILL }
	  },
	  { &hf_a11_ses_msid,
		 { "MSID(BCD)",                      "a11.ext.msid",
			FT_BYTES, BASE_HEX, NULL, 0,
			"MSID(BCD).", HFILL }
	  },
	  { &hf_a11_ses_ptype,
		 { "Protocol Type",                      "a11.ext.ptype",
			FT_UINT16, BASE_HEX, NULL, 0,
			"Protocol Type.", HFILL }
	  },
	  { &hf_a11_vse_vid,
		 { "Vendor ID",                      "a11.ext.vid",
			FT_UINT32, BASE_HEX, NULL, 0,
			"Vendor ID.", HFILL }
	  },
	  { &hf_a11_vse_apptype,
		 { "Application Type",                      "a11.ext.apptype",
			FT_UINT8, BASE_HEX, VALS(a11_ext_app), 0,
			"Application Type.", HFILL }
	  },
	  { &hf_a11_vse_ppaddr,
		 { "Anchor P-P Address",           "a11.ext.ppaddr",
			FT_IPv4, BASE_NONE, NULL, 0,          
			"Anchor P-P Address.", HFILL }
	  },
	  { &hf_a11_vse_dormant,
		 { "All Dormant Indicator",           "a11.ext.dormant",
			FT_UINT16, BASE_HEX, VALS(a11_ext_dormant), 0,          
			"All Dormant Indicator.", HFILL }
	  },
	  { &hf_a11_vse_code,
		 { "Reply Code",           "a11.ext.code",
			FT_UINT8, BASE_DEC, VALS(a11_reply_codes), 0,          
			"PDSN Code.", HFILL }
	  },
	  { &hf_a11_vse_pdit,
		 { "PDSN Code",                      "a11.ext.code",
			FT_UINT8, BASE_HEX, VALS(a11_ext_nvose_pdsn_code), 0,
			"PDSN Code.", HFILL }
	  },
	  { &hf_a11_vse_srvopt,
		 { "Service Option",                      "a11.ext.srvopt",
			FT_UINT16, BASE_HEX, VALS(a11_ext_nvose_srvopt), 0,
			"Service Option.", HFILL }
	  },
	  { &hf_a11_vse_panid,
		 { "PANID",                      "a11.ext.panid",
			FT_BYTES, BASE_HEX, NULL, 0,
			"PANID", HFILL }
	  },
	  { &hf_a11_vse_canid,
		 { "CANID",                      "a11.ext.canid",
			FT_BYTES, BASE_HEX, NULL, 0,
			"CANID", HFILL }
	  },
     
	};

	/* Setup protocol subtree array */
	static gint *ett[] = {
		&ett_a11,
		&ett_a11_flags,
		&ett_a11_ext,
		&ett_a11_exts,
		&ett_a11_radius,
		&ett_a11_radiuses,
	};

	/* Register the protocol name and description */
	proto_a11 = proto_register_protocol("3GPP2 A11", "3GPP2 A11", "a11");

	/* Register the dissector by name */
	new_register_dissector("a11", dissect_a11, proto_a11);

	/* Required function calls to register the header fields and subtrees used */
	proto_register_field_array(proto_a11, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));
}

void
proto_reg_handoff_a11(void)
{
	dissector_handle_t a11_handle;

	a11_handle = find_dissector("a11");
	dissector_add("udp.port", UDP_PORT_3GA11, a11_handle);
}

