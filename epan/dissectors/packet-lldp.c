/* packet-lldp.c
 * Routines for LLDP dissection
 * By Juan Gonzalez <juan.gonzalez@pikatech.com>
 * Copyright 2005 MITEL
 *
 * July 2005
 * Modified by: Brian Bogora <brian_bogora@mitel.com>
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

#include <gmodule.h>
#include <glib.h>

#include <epan/packet.h>
#include <epan/prefs.h>
#include "etypes.h"

/* TLV Types */
#define END_OF_LLDPDU_TLV_TYPE		0x00	/* Mandatory */
#define CHASSIS_ID_TLV_TYPE		0x01	/* Mandatory */
#define PORT_ID_TLV_TYPE		0x02	/* Mandatory */
#define TIME_TO_LIVE_TLV_TYPE		0x03	/* Mandatory */
#define PORT_DESCRIPTION_TLV_TYPE	0x04
#define SYSTEM_NAME_TLV_TYPE		0x05
#define SYSTEM_DESCRIPTION_TLV_TYPE	0x06
#define SYSTEM_CAPABILITIES_TLV_TYPE	0x07
#define MANAGEMENT_ADDR_TLV_TYPE	0x08
#define ORGANIZATION_SPECIFIC_TLV_TYPE	0x7F

/* Masks */
#define TLV_TYPE_MASK		0xFE00
#define TLV_TYPE(value)		(((value) & TLV_TYPE_MASK) >> 9)
#define TLV_INFO_LEN_MASK	0x01FF
#define TLV_INFO_LEN(value)	((value) & TLV_INFO_LEN_MASK)

/* Initialize the protocol and registered fields */
static int proto_lldp = -1;
static int hf_lldp_tlv_type = -1;
static int hf_lldp_tlv_len = -1;
static int hf_chassis_id_subtype = -1;
static int hf_chassis_id = -1;
static int hf_chassis_id_mac = -1;
static int hf_chassis_id_ip4 = -1;
static int hf_chassis_id_ip6 = -1;
static int hf_port_id_subtype = -1;
static int hf_port_id_mac = -1;
static int hf_port_id_ip4 = -1;
static int hf_port_id_ip6 = -1;
static int hf_time_to_live = -1;
static int hf_mgn_addr_ipv4 = -1;
static int hf_mgn_addr_ipv6 = -1;
static int hf_mgn_addr_hex = -1;
static int hf_mgn_obj_id = -1;
static int hf_org_spc_oui = -1;
static int hf_ieee_802_1_subtype = -1;
static int hf_ieee_802_3_subtype = -1;
static int hf_media_tlv_subtype = -1;

/* Initialize the subtree pointers */
static gint ett_lldp = -1;
static gint ett_chassis_id = -1;
static gint ett_port_id = -1;
static gint ett_time_to_live = -1;
static gint ett_end_of_lldpdu = -1;
static gint ett_port_description = -1;
static gint ett_system_name = -1;
static gint ett_system_cap = -1;
static gint ett_system_cap_summary = -1;
static gint ett_system_cap_enabled = -1;
static gint ett_management_address = -1;
static gint ett_unknown_tlv = -1;
static gint ett_org_spc_tlv = -1;
static gint ett_port_vlan_flags = -1;
static gint ett_802_3_flags = -1;
static gint ett_802_3_power = -1;
static gint ett_802_3_aggregation = -1;
static gint ett_media_capabilities = -1;

const value_string tlv_types[] = {
	{ END_OF_LLDPDU_TLV_TYPE, 		"End of LLDPDU"},
	{ CHASSIS_ID_TLV_TYPE,			"Chassis Id"},
	{ PORT_ID_TLV_TYPE,			"Port Id"},
	{ TIME_TO_LIVE_TLV_TYPE,		"Time to Live"},
	{ PORT_DESCRIPTION_TLV_TYPE,		"Port Description"},
	{ SYSTEM_NAME_TLV_TYPE,			"System Name"},
	{ SYSTEM_DESCRIPTION_TLV_TYPE,		"System Description"},
	{ SYSTEM_CAPABILITIES_TLV_TYPE,		"System Capabilities"},
	{ MANAGEMENT_ADDR_TLV_TYPE,		"Management Address"},
	{ ORGANIZATION_SPECIFIC_TLV_TYPE,	"Organization Specific"},
	{ 0, NULL} 
};

const value_string chassis_id_subtypes[] = {
	{ 0,	"Reserved"},
	{ 1,	"Chassis component"},
	{ 2,	"Interface alias"},
	{ 3,	"Port component"},
	{ 4, 	"MAC address"},
	{ 5,	"Network address"},
	{ 6,	"Interface name"},
	{ 7,	"Locally assigned"},
	{ 0, NULL} 
};

const value_string port_id_subtypes[] = {
	{ 0,	"Reserved"},
	{ 1,	"Interface alias"},
	{ 2,	"Port component"},
	{ 3, 	"MAC address"},
	{ 4,	"Network address"},
	{ 5,	"Interface name"},
	{ 6, 	"Agent circuit Id"},
	{ 7,	"Locally assigned"},
	{ 0, NULL} 
};

const value_string management_addr_values[] = {
	{ 0,	"Reserved"},
	{ 1,	"IPv4"},
	{ 2,	"IPv6"},
	{ 3,	"NSAP"},
	{ 4,	"HDLC (8-bit multidrop)"},
	{ 5,	"BBN 1822"},
	{ 6,	"802 (includes all 802 media plus Ethernet)"},
	{ 7,	"E.163"},
	{ 8,	"E.164 (SMDS, Frame Relay, ATM)"},
	{ 9,	"F.69 (Telex)"},
	{ 10,	"X.121 (X.25, Frame Relay)"},
	{ 11,	"IPX"},
	{ 12,	"Appletalk"},
	{ 13,	"Decnet IV"},
	{ 14,	"Banyan Vines"},
	{ 15,	"E.164 with NSAP format subaddress"},
	{ 16,	"DNS (Domain Name System)"},
	{ 17,	"Distinguished Name"},
	{ 18,	"AS Number"},
	{ 19,	"XTP over IP version 4"},
	{ 20,	"XTP over IP version 6"},
	{ 21,	"XTP native mode XTP"},
	{ 22,	"Fibre Channel World-Wide Port Name"},
	{ 23,	"Fibre Channel World-Wide Node Name"},
	{ 24,	"GWID"},
	{ 0, NULL}
};

const value_string interface_subtype_values[] = {
	{ 1,	"Unknown"},
	{ 2,	"ifIndex"},
	{ 3, 	"System port number"},
	{ 0, NULL} 
};

#define OUI_IEEE_802_1			0x0080C2	/* IEEE 802.1 */
#define OUI_IEEE_802_3			0x00120F	/* IEEE 802.3 */
#define OUI_MEDIA_ENDPOINT		0x0012BB	/* Media */

const value_string tlv_oui_subtype_vals[] = {
	{ OUI_IEEE_802_1,     	"IEEE 802.1" },
	{ OUI_IEEE_802_3,     	"IEEE 802.3" },
	{ OUI_MEDIA_ENDPOINT,	"TIA" },
	{ 0, NULL }
};

/* IEEE 802.1 Subtypes */
const value_string ieee_802_1_subtypes[] = {
	{ 0x01,	"Port VLAN ID" },
	{ 0x02, "Port and Protocol VLAN ID" },
	{ 0x03, "VLAN Name" },
	{ 0x04, "Protocol Identity" },
	{ 0, NULL }
};

/* IEEE 802.3 Subtypes */
const value_string ieee_802_3_subtypes[] = {
	{ 0x01,	"MAC/PHY Configuration/Status" },
	{ 0x02,	"Power Via MDI" },
	{ 0x03,	"Link Aggregatiion" },
	{ 0x04, "Maximum Frame Size" },
	{ 0, NULL }
};

/* Media Subtypes */
const value_string media_subtypes[] = {
	{ 1,	"Media Capabilities" },
	{ 2, 	"Network Policy" },
	{ 3, 	"Location Identification" },
	{ 4, 	"Extended Power-via-MDI" },
	{ 5,	"Inventory - Hardware Revision" },
	{ 6,	"Inventory - Firmware Revision" },
	{ 7,	"Inventory - Software Revision" },
	{ 8,	"Inventory - Serial Number" },
	{ 9,	"Inventory - Manufacturer Name" },
	{ 10,	"Inventory - Model Name" },
	{ 11, 	"Inventory - Asset ID" },
	{ 0, NULL }
};

/* Media Class Values */
const value_string media_class_values[] = {
	{ 0,	"Type Not Defined" },
	{ 1,	"Endpoint Class I" },
	{ 2,	"Endpoint Class II" },
	{ 3,	"Endpoint Class III" },
	{ 4,	"Network Connectivity" },
	{ 0, NULL }
};

/* Media Application Types */
const value_string media_application_type[] = {
	{ 0,	"Reserved" },
	{ 1,	"Voice" },
	{ 2,	"Voice Signaling" },
	{ 3,	"Guest Voice" },
	{ 4,	"Guest Voice Signaling" },
	{ 5,	"Softphone Voice" },
	{ 6,	"Video Conferencing" },
	{ 7,	"Streaming Video" },
	{ 8,	"Video Signaling" },
	{ 0, NULL }
};

/* Power Type */
const value_string media_power_type[] = {
	{ 0,	"PSE Device" },
	{ 1,	"PD Device" },
	{ 0, NULL }
};

/* Power Priority */
const value_string media_power_priority[] = {
	{ 0, 	"Unknown" },
	{ 1,	"Critical" },
	{ 2,	"High" },
	{ 3,	"Low" },
	{ 0, NULL }
};

/* Power Sources */
const value_string media_power_pd_device[] = {
	{ 0,	"Unknown" },
	{ 1,	"PSE" },
	{ 2,	"Local" },
	{ 3,	"PSE and Local" },
	{ 0, NULL }
};
const value_string media_power_pse_device[] = {
	{ 0,	"Unknown" },
	{ 1,	"Primary Power Source" },
	{ 2,	"Backup Power Source" },
	{ 0, NULL }
};

/* Location data format */
const value_string location_data_format[] = {
	{ 0, 	"Invalid " },
	{ 1,	"Coordinate-based LCI" },
	{ 2,	"Civic Address LCI" },
	{ 3, 	"ECS ELIN" },
	{ 0, NULL }
};

/* Civic Address LCI - What field */
const value_string civic_address_what_values[] = {
	{ 0,	"Location of the DHCP server" },
	{ 1,	"Location of the network element believed to be closest to the client" },
	{ 2,	"Location of the client"},
	{ 0, NULL}
};

/* Civic Address Type field */
const value_string civic_address_type_values[] = {
	{ 0,	"Language" },
	{ 1,	"National subdivisions (province, state, etc)" },
	{ 2,	"County, parish, district" },
	{ 3,	"City, township" },
	{ 4,	"City division, borough, ward" },
	{ 5,	"Neighborhood, block" },
	{ 6,	"Street" },
	{ 16,	"Leading street direction" },
	{ 17,	"Trailing street suffix" },
	{ 18,	"Street suffix" },
	{ 19,	"House number" },
	{ 20,	"House number suffix" },
	{ 21,	"Landmark or vanity address" },
	{ 22,	"Additional location information" },
	{ 23,	"Name" },
	{ 24, 	"Postal/ZIP code" },
	{ 25,	"Building" },
	{ 26,	"Unit" },
	{ 27,	"Floor" },
	{ 28,	"Room number" },
	{ 29,	"Place type" },
	{ 128,	"Script" },
	{ 0, NULL }
};

/* System Capabilities */
#define SYSTEM_CAPABILITY_OTHER 	0x0001
#define SYSTEM_CAPABILITY_REPEATER 	0x0002
#define SYSTEM_CAPABILITY_BRIDGE 	0x0004
#define SYSTEM_CAPABILITY_WLAN 		0x0008
#define SYSTEM_CAPABILITY_ROUTER 	0x0010
#define SYSTEM_CAPABILITY_TELEPHONE	0x0020
#define SYSTEM_CAPABILITY_DOCSIS 	0x0040
#define SYSTEM_CAPABILITY_STATION 	0x0080

/* Media Capabilities */
#define MEDIA_CAPABILITY_LLDP				0x0001
#define MEDIA_CAPABILITY_NETWORK_POLICY		0x0002
#define MEDIA_CAPABILITY_LOCATION_ID		0x0004
#define MEDIA_CAPABILITY_MDI_PSE			0x0008
#define MEDIA_CAPABILITY_MDI_PD				0x0010
#define MEDIA_CAPABILITY_INVENTORY			0x0020

#define MAX_MAC_LEN	6

/* Calculate Latitude and Longitude string */
/*
	Parameters:
		option = 0 -> Latitude
		option = 1 -> Longitude
*/
static void
get_latitude_or_longitude(int option, guint64 value, guint8 *strPtr)
{
	guint64 tempValue = value;
	guint8 negativeNum = 0;	/* False */
	guint32 integerPortion = 0;
	
	gchar tempStr[100];
	
	strcpy(strPtr," ");
	
	/* The latitude and longitude are 34 bit fixed point value consisting
	   of 9 bits of integer and 25 bits of fraction.  
	   When option is equal to 0, positive numbers are represent a location
	   north of the equator and negative (2s complement) numbers are south of the equator.
	   When option is equal to 1, positive values are east of the prime 
	   meridian and negative (2s complement) numbers are west of the prime meridian.
	*/
	
	if (value & G_GINT64_CONSTANT(0x0000000200000000))
	{
		/* Have a negative number (2s complement) */
		negativeNum = 1;
		
		tempValue = ~value;
		tempValue += 1;
	}
	
	/* Get the integer portion */
	integerPortion = (guint32)((tempValue & G_GINT64_CONSTANT(0x00000003FE000000)) >> 25);
	
	/* Calculate decimal portion (using 25 bits for fraction) */
	tempValue = (tempValue & G_GINT64_CONSTANT(0x0000000001FFFFFF))/33554432;
	
	sprintf(tempStr,"%u.%04" PRIu64 " degrees ", integerPortion, tempValue);
	
	if (option == 0)
	{
		/* Latitude - north/south directions */
		if (negativeNum == 1)
			strcat(tempStr,"South");
		else
			strcat(tempStr,"North");
	}
	else
	{
		/* Longitude - east/west directions */
		if (negativeNum == 1)
			strcat(tempStr,"West");
		else
			strcat(tempStr,"East");
	}
	
	strcpy(strPtr,tempStr);
	
	return;
}

/* Dissect Chassis Id TLV (Mandatory) */
static gint32
dissect_lldp_chassis_id(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, guint32 offset)
{
	guint8 tempType;
	guint16 tempShort;
	guint32 tempLen = 0;
	const char *strPtr;
	guint8 incorrectLen = 0;	/* incorrect length if 1 */
	
	const guint8 *mac_addr = NULL;
	guint32 ip_addr;
	struct e_in6_addr ip6_addr;
	
	proto_tree	*chassis_tree = NULL;
	proto_item 	*tf = NULL;
	
	/* Get tlv type */
	tempShort = tvb_get_ntohs(tvb, offset);
	tempType = TLV_TYPE(tempShort);
	if (tempType != CHASSIS_ID_TLV_TYPE)
	{
		if (tree)
		{
			tf = proto_tree_add_text(tree, tvb, offset, 2, "Invalid Chassis ID (0x%02X)", tempType);
			chassis_tree = proto_item_add_subtree(tf, ett_chassis_id);
			proto_tree_add_text(chassis_tree, tvb, offset, 2, "%s Invalid Chassis ID (%u)",
						decode_boolean_bitfield(tempType, TLV_TYPE_MASK, 16, "", ""), tempType);
		}
		
		return -1;
	}
		
	/* Get tlv length */
	tempLen = TLV_INFO_LEN(tempShort);
	if (tempLen < 2)
	{
		if (tree)
		{
			tf = proto_tree_add_text(tree, tvb, offset, 2, "Invalid Chassis ID Length (%u)", tempLen);
			chassis_tree = proto_item_add_subtree(tf, ett_chassis_id);
			proto_tree_add_item(chassis_tree, hf_lldp_tlv_type, tvb, offset, 2, FALSE);
			proto_tree_add_text(chassis_tree, tvb, offset, 2, "%s Invalid Length: %u",
						decode_boolean_bitfield(tempLen, TLV_INFO_LEN_MASK, 16, "", ""), tempLen);
		}
		
		return -1;
	}
	
	/* Get tlv subtype */
	tempType = tvb_get_guint8(tvb, (offset+2));
	
	switch (tempType)
	{
	case 4:	/* MAC address */
	{
		if (tempLen != 7)
		{
			incorrectLen = 1; 	/* Invalid length */
			break;
		}
			
		mac_addr=tvb_get_ptr(tvb, (offset+3), 6);
		strPtr = ether_to_str(mac_addr);
	
		break;
	}
	case 5:	/* Network address */
	{
		/* Check for IPv4 or IPv6 */
		if (tempLen == 5)
		{
			ip_addr = tvb_get_ipv4(tvb, (offset+3));
			strPtr = ip_to_str((guint8 *)&ip_addr);
		}
		else if  (tempLen == 17)
		{
			tvb_get_ipv6(tvb, (offset+3), &ip6_addr);
			strPtr = ip6_to_str(&ip6_addr);
		}
		else
		{
			incorrectLen = 1; 	/* Invalid length */
			break;
		}
			
		break;
	}
	case 2:	/* Interface alias */
	case 6: /* Interface name */
	case 7:	/* Locally assigned */
	{
		if (tempLen > 256)
		{
			incorrectLen = 1; 	/* Invalid length */
			break;
		}
		
		strPtr = tvb_format_stringzpad(tvb, (offset+3), (tempLen-1));

		break;
	}
	case 1:	/* Chassis component */
	case 3:	/* Port component */
	{
		if (tempLen > 256)
		{
			incorrectLen = 1; 	/* Invalid length */
			break;
		}
		
		strPtr = tvb_bytes_to_str(tvb, (offset+3), (tempLen-1));
		break;
	}
	default:	/* Reserved types */
	{
		if (tempLen > 256)
		{
			incorrectLen = 1; 	/* Invalid length */
			break;
		}
		
		strPtr = "Reserved";
		
		break;
	}
	}
	
	if (incorrectLen == 1)
	{
		if (tree)
		{
			tf = proto_tree_add_text(tree, tvb, offset, 2, "Invalid Chassis ID Length (%u)", tempLen);
			chassis_tree = proto_item_add_subtree(tf, ett_chassis_id);
			proto_tree_add_item(chassis_tree, hf_lldp_tlv_type, tvb, offset, 2, FALSE);
			proto_tree_add_text(chassis_tree, tvb, offset, 2, "%s Invalid Length: %u",
						decode_boolean_bitfield(tempLen, TLV_INFO_LEN_MASK, 16, "", ""), tempLen);			
			/* Get chassis id subtype */
			proto_tree_add_item(chassis_tree, hf_chassis_id_subtype, tvb, (offset+2), 1, FALSE);
		
		}
		
		return -1;
	}
	
	if (check_col(pinfo->cinfo, COL_INFO))
		col_add_fstr(pinfo->cinfo, COL_INFO, "Chassis Id = %s ", strPtr);
			
	if (tree)
	{
		/* Set chassis tree */
		tf = proto_tree_add_text(tree, tvb, offset, (tempLen + 2), "Chassis Subtype = %s", 
								val_to_str(tempType, chassis_id_subtypes, "Reserved" ));
		chassis_tree = proto_item_add_subtree(tf, ett_chassis_id);
		
		proto_tree_add_item(chassis_tree, hf_lldp_tlv_type, tvb, offset, 2, FALSE);
		proto_tree_add_item(chassis_tree, hf_lldp_tlv_len, tvb, offset, 2, FALSE);
		
		/* Get chassis id subtype */
		proto_tree_add_item(chassis_tree, hf_chassis_id_subtype, tvb, (offset+2), 1, FALSE);
		
		/* Get chassis id */
		switch (tempType)
		{
		case 4:	/* MAC address */
			proto_tree_add_ether(chassis_tree, hf_chassis_id_mac, tvb, (offset+3), 6, mac_addr);
			break;
		case 5: /* Network address */
			if (tempLen == 5)
				proto_tree_add_ipv4(chassis_tree, hf_chassis_id_ip4, tvb, (offset+3), 4, ip_addr);
			else
				proto_tree_add_ipv6(chassis_tree, hf_chassis_id_ip6, tvb, (offset+3), 16, ip6_addr.bytes);
			break;
		case 2:	/* Interface alias */
		case 6: /* Interface name */
		case 7:	/* Locally assigned */
			proto_tree_add_text(chassis_tree, tvb, (offset+3), (tempLen-1), "Chassis Id: %s", strPtr);
			break;
		case 1:	/* Chassis component */
		case 3:	/* Port component */
			proto_tree_add_item(chassis_tree, hf_chassis_id, tvb, (offset+3), (tempLen-1), FALSE);
			break;
		}
	}
	
	return (tempLen + 2);
}

/* Dissect Port Id TLV (Mandatory) */
static gint32
dissect_lldp_port_id(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, guint32 offset)
{
	guint8 tempType;
	guint16 tempShort;
	guint32 tempLen = 0;
	const char *strPtr;
	const guint8 *mac_addr = NULL;
	guint32 ip_addr;
	struct e_in6_addr ip6_addr;
	
	proto_tree	*port_tree = NULL;
	proto_item 	*tf = NULL;
	
	/* Get tlv type */
	tempShort = tvb_get_ntohs(tvb, offset);
	tempType = TLV_TYPE(tempShort);
	if (tempType != PORT_ID_TLV_TYPE)
		return -1;
		
	/* Get tlv length and subtype */
	tempLen = TLV_INFO_LEN(tempShort);
	tempType = tvb_get_guint8(tvb, (offset+2));
	
	/* Get port id */
	switch (tempType)
	{
	case 3:	/* MAC address */
	{
		if (tempLen != 7)
			return -1; 	/* Invalid port id */
			
		mac_addr=tvb_get_ptr(tvb, (offset+3), 6);
		strPtr = ether_to_str(mac_addr);
	
		break;
	}
	case 4:	/* Network address */
	{
		/* Check for IPv4 or IPv6 */
		if (tempLen == 5)
		{
			ip_addr = tvb_get_ipv4(tvb, (offset+3));
			strPtr = ip_to_str((guint8 *)&ip_addr);
		}
		else if  (tempLen == 17)
		{
			tvb_get_ipv6(tvb, (offset+3), &ip6_addr);
			strPtr = ip6_to_str(&ip6_addr);
		}
		else
			return -1;	/* Invalid chassis id */
			
		break;
	}
	default:
	{
		strPtr = tvb_format_stringzpad(tvb, (offset+3), (tempLen-1));
	
		break;
	}
	}
	
	if (check_col(pinfo->cinfo, COL_INFO))
		col_append_fstr(pinfo->cinfo, COL_INFO, "Port Id = %s ", strPtr);
			
	if (tree)
	{
		/* Set port tree */
		tf = proto_tree_add_text(tree, tvb, offset, (tempLen + 2), "Port Subtype = %s", 
								val_to_str(tempType, port_id_subtypes, "Unknown" ));
		port_tree = proto_item_add_subtree(tf, ett_port_id);
		
		proto_tree_add_item(port_tree, hf_lldp_tlv_type, tvb, offset, 2, FALSE);
		proto_tree_add_item(port_tree, hf_lldp_tlv_len, tvb, offset, 2, FALSE);
		
		/* Get port id subtype */
		proto_tree_add_item(port_tree, hf_port_id_subtype, tvb, (offset+2), 1, FALSE);
		
		/* Get port id */
		/*proto_tree_add_text(port_tree, tvb, (offset+3), (tempLen-1), "Port Id: %s", strPtr);*/
		switch (tempType)
		{
		case 3:	/* MAC address */
			proto_tree_add_ether(port_tree, hf_port_id_mac, tvb, (offset+3), 6, mac_addr);
			break;
		case 4: /* Network address */
			if (tempLen == 5)
				proto_tree_add_ipv4(port_tree, hf_port_id_ip4, tvb, (offset+3), 4, ip_addr);
			else
				proto_tree_add_ipv6(port_tree, hf_port_id_ip6, tvb, (offset+3), 16, ip6_addr.bytes);
			break;
		default:
			proto_tree_add_text(port_tree, tvb, (offset+3), (tempLen-1), "Port Id: %s", strPtr);
			break;
		}
		
	}
	
	return (tempLen + 2);
}

/* Dissect Time To Live TLV (Mandatory) */
static gint32
dissect_lldp_time_to_live(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, guint32 offset)
{
	guint8 tempType;
	guint16 tempShort;
	guint32 tempLen = 0;
	
	proto_tree	*time_to_live_tree = NULL;
	proto_item 	*tf = NULL;
	
	/* Get tlv type */
	tempShort = tvb_get_ntohs(tvb, offset);
	tempType = TLV_TYPE(tempShort);
	if (tempType != TIME_TO_LIVE_TLV_TYPE)
		return -1;
		
	/* Get tlv length and seconds field */
	tempLen = TLV_INFO_LEN(tempShort);
	tempShort = tvb_get_ntohs(tvb, (offset+2));
	
	if (check_col(pinfo->cinfo, COL_INFO))
		col_append_fstr(pinfo->cinfo, COL_INFO, "TTL = %u ", tempShort);
			
	if (tree)
	{
		/* Set port tree */
		tf = proto_tree_add_text(tree, tvb, offset, (tempLen + 2), "Time To Live = %u sec", tempShort);
		time_to_live_tree = proto_item_add_subtree(tf, ett_time_to_live);
		
		proto_tree_add_item(time_to_live_tree, hf_lldp_tlv_type, tvb, offset, 2, FALSE);
		proto_tree_add_item(time_to_live_tree, hf_lldp_tlv_len, tvb, offset, 2, FALSE);
		
		/* Display time to live information */
		proto_tree_add_item(time_to_live_tree, hf_time_to_live, tvb, (offset+2), 2, FALSE);
	}
	
	return (tempLen + 2);
}

/* Dissect End of LLDPDU TLV (Mandatory) */
static gint32
dissect_lldp_end_of_lldpdu(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, guint32 offset)
{
	guint16 tempLen;
	guint16 tempShort;
	
	proto_tree	*end_of_lldpdu_tree = NULL;
	proto_item 	*tf = NULL;
	
	/* Get tlv type and length */
	tempShort = tvb_get_ntohs(tvb, offset);
		
	/* Get tlv length */
	tempLen = TLV_INFO_LEN(tempShort);
			
	if (tree)
	{
		/* Set port tree */
		tf = proto_tree_add_text(tree, tvb, offset, (tempLen + 2), "End of LLDPDU");
		end_of_lldpdu_tree = proto_item_add_subtree(tf, ett_end_of_lldpdu);
		
		proto_tree_add_item(end_of_lldpdu_tree, hf_lldp_tlv_type, tvb, offset, 2, FALSE);
		proto_tree_add_item(end_of_lldpdu_tree, hf_lldp_tlv_len, tvb, offset, 2, FALSE);
	}
	
	return -1;	/* Force the lldp dissector to terminate */
}

/* Dissect Port Description TLV */
static gint32
dissect_lldp_port_desc(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, guint32 offset)
{
	guint16 tempShort;
	guint32 tempLen = 0;
	const char *strPtr;
	
	proto_tree	*port_desc_tree = NULL;
	proto_item 	*tf = NULL;
	
	/* Get tlv type and length */
	tempShort = tvb_get_ntohs(tvb, offset);
		
	/* Get tlv length */
	tempLen = TLV_INFO_LEN(tempShort);
			
	if (tree)
	{
		strPtr = tvb_format_stringzpad(tvb, (offset+2), tempLen);

		/* Set port tree */
		tf = proto_tree_add_text(tree, tvb, offset, (tempLen + 2), "Port Description = %s", strPtr);
		port_desc_tree = proto_item_add_subtree(tf, ett_port_description);
		
		proto_tree_add_item(port_desc_tree, hf_lldp_tlv_type, tvb, offset, 2, FALSE);
		proto_tree_add_item(port_desc_tree, hf_lldp_tlv_len, tvb, offset, 2, FALSE);
		
		/* Display port description information */
		proto_tree_add_text(port_desc_tree, tvb, (offset+2), tempLen, "Port Description: %s",
		    strPtr);
	}
	
	return (tempLen + 2);
}

/* Dissect System Name and description TLV */
static gint32
dissect_lldp_system_name(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, guint32 offset)
{
	guint16 tempShort;
	guint32 tempLen = 0;
	guint8 tempType;
	const char *strPtr;
	
	proto_tree	*system_name_tree = NULL;
	proto_item 	*tf = NULL;
	
	/* Get tlv type and length */
	tempShort = tvb_get_ntohs(tvb, offset);
	tempType = TLV_TYPE(tempShort);
		
	/* Get tlv length */
	tempLen = TLV_INFO_LEN(tempShort);
			
	if (tree)
	{
		strPtr = tvb_format_stringzpad(tvb, (offset+2), tempLen);

		/* Set system name tree */
		if (tempType == SYSTEM_NAME_TLV_TYPE) 
			tf = proto_tree_add_text(tree, tvb, offset, (tempLen + 2), "System Name = %s", strPtr);
		else
			tf = proto_tree_add_text(tree, tvb, offset, (tempLen + 2), "System Description = %s", strPtr);
		system_name_tree = proto_item_add_subtree(tf, ett_system_name);
		
		proto_tree_add_item(system_name_tree, hf_lldp_tlv_type, tvb, offset, 2, FALSE);
		proto_tree_add_item(system_name_tree, hf_lldp_tlv_len, tvb, offset, 2, FALSE);
		
		/* Display system name information */
		proto_tree_add_text(system_name_tree, tvb, (offset+2), tempLen, "%s = %s", 
					((tempType == SYSTEM_NAME_TLV_TYPE) ? "System Name" : "System Description"),
					strPtr);
	}
	
	return (tempLen + 2);
}

/* Dissect System Capabilities TLV */
static gint32
dissect_lldp_system_capabilities(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, guint32 offset)
{
	guint16 tempShort;
	guint32 tempLen = 0;
	guint16 tempCapability;
	
	proto_tree	*system_capabilities_tree = NULL;
	proto_tree	*capabilities_summary_tree = NULL;
	proto_tree  *capabilities_enabled_tree = NULL;
	proto_item 	*tf = NULL;
	
	/* Get tlv type and length */
	tempShort = tvb_get_ntohs(tvb, offset);
		
	/* Get tlv length */
	tempLen = TLV_INFO_LEN(tempShort);
	
	/* Get system capabilities */
	tempCapability = tvb_get_ntohs(tvb, (offset+2));
			
	if (tree)
	{
		/* Set system capabilities tree */
		tf = proto_tree_add_text(tree, tvb, offset, (tempLen + 2), "Capabilities");
		system_capabilities_tree = proto_item_add_subtree(tf, ett_system_cap);
		
		proto_tree_add_item(system_capabilities_tree, hf_lldp_tlv_type, tvb, offset, 2, FALSE);
		proto_tree_add_item(system_capabilities_tree, hf_lldp_tlv_len, tvb, offset, 2, FALSE);
		
		/* Display system capability information */
		tf = proto_tree_add_text(system_capabilities_tree, tvb, (offset+2), 2, "Capabilities: 0x%04x", tempCapability);
		capabilities_summary_tree = proto_item_add_subtree(tf, ett_system_cap_summary);
		/* Add capabilities to summary tree */
		if (tempCapability & SYSTEM_CAPABILITY_OTHER)
			proto_tree_add_text(capabilities_summary_tree, tvb, (offset+2), 2, "%s",
						decode_boolean_bitfield(tempCapability, SYSTEM_CAPABILITY_OTHER,
						16, "Other", ""));
		if (tempCapability & SYSTEM_CAPABILITY_REPEATER)
			proto_tree_add_text(capabilities_summary_tree, tvb, (offset+2), 2, "%s",
						decode_boolean_bitfield(tempCapability, SYSTEM_CAPABILITY_REPEATER,
						16, "Repeater", ""));
		if (tempCapability & SYSTEM_CAPABILITY_BRIDGE)
			proto_tree_add_text(capabilities_summary_tree, tvb, (offset+2), 2, "%s",
						decode_boolean_bitfield(tempCapability, SYSTEM_CAPABILITY_BRIDGE,
						16, "Bridge", ""));
		if (tempCapability & SYSTEM_CAPABILITY_WLAN)
			proto_tree_add_text(capabilities_summary_tree, tvb, (offset+2), 2, "%s",
						decode_boolean_bitfield(tempCapability, SYSTEM_CAPABILITY_WLAN,
						16, "WLAN access point", ""));
		if (tempCapability & SYSTEM_CAPABILITY_ROUTER)
			proto_tree_add_text(capabilities_summary_tree, tvb, (offset+2), 2, "%s",
						decode_boolean_bitfield(tempCapability, SYSTEM_CAPABILITY_ROUTER,
						16, "Router", ""));
		if (tempCapability & SYSTEM_CAPABILITY_TELEPHONE)
			proto_tree_add_text(capabilities_summary_tree, tvb, (offset+2), 2, "%s",
						decode_boolean_bitfield(tempCapability, SYSTEM_CAPABILITY_TELEPHONE,
						16, "Telephone", ""));
		if (tempCapability & SYSTEM_CAPABILITY_DOCSIS)
			proto_tree_add_text(capabilities_summary_tree, tvb, (offset+2), 2, "%s",
						decode_boolean_bitfield(tempCapability, SYSTEM_CAPABILITY_DOCSIS,
						16, "DOCSIS cable device", ""));
		if (tempCapability & SYSTEM_CAPABILITY_STATION)
			proto_tree_add_text(capabilities_summary_tree, tvb, (offset+2), 2, "%s",
						decode_boolean_bitfield(tempCapability, SYSTEM_CAPABILITY_STATION,
						16, "Station only", ""));
						
		/* Get enabled summary */
		tempShort = tvb_get_ntohs(tvb, (offset+4));
  
		/* Display system capability information */
		tf = proto_tree_add_text(system_capabilities_tree, tvb, (offset+4), 2, "Enabled Capabilities: 0x%04x", tempShort);
		capabilities_enabled_tree = proto_item_add_subtree(tf, ett_system_cap_enabled);
		/* Add capabilities to summary tree */
		if (tempShort & SYSTEM_CAPABILITY_OTHER)
			proto_tree_add_text(capabilities_enabled_tree, tvb, (offset+4), 2, "%s",
						decode_boolean_bitfield(tempShort, SYSTEM_CAPABILITY_OTHER,
						16, "Other", ""));
		if (tempShort & SYSTEM_CAPABILITY_REPEATER)
			proto_tree_add_text(capabilities_enabled_tree, tvb, (offset+4), 2, "%s",
						decode_boolean_bitfield(tempShort, SYSTEM_CAPABILITY_REPEATER,
						16, "Repeater", ""));
		if (tempShort & SYSTEM_CAPABILITY_BRIDGE)
			proto_tree_add_text(capabilities_enabled_tree, tvb, (offset+4), 2, "%s",
						decode_boolean_bitfield(tempShort, SYSTEM_CAPABILITY_BRIDGE,
						16, "Bridge", ""));
		if (tempShort & SYSTEM_CAPABILITY_WLAN)
			proto_tree_add_text(capabilities_enabled_tree, tvb, (offset+4), 2, "%s",
						decode_boolean_bitfield(tempShort, SYSTEM_CAPABILITY_WLAN,
						16, "WLAN access point", ""));
		if (tempShort & SYSTEM_CAPABILITY_ROUTER)
			proto_tree_add_text(capabilities_enabled_tree, tvb, (offset+4), 2, "%s",
						decode_boolean_bitfield(tempShort, SYSTEM_CAPABILITY_ROUTER,
						16, "Router", ""));
		if (tempShort & SYSTEM_CAPABILITY_TELEPHONE)
			proto_tree_add_text(capabilities_enabled_tree, tvb, (offset+4), 2, "%s",
						decode_boolean_bitfield(tempShort, SYSTEM_CAPABILITY_TELEPHONE,
						16, "Telephone", ""));
		if (tempShort & SYSTEM_CAPABILITY_DOCSIS)
			proto_tree_add_text(capabilities_enabled_tree, tvb, (offset+4), 2, "%s",
						decode_boolean_bitfield(tempShort, SYSTEM_CAPABILITY_DOCSIS,
						16, "DOCSIS cable device", ""));
		if (tempShort & SYSTEM_CAPABILITY_STATION)
			proto_tree_add_text(capabilities_enabled_tree, tvb, (offset+4), 2, "%s",
						decode_boolean_bitfield(tempShort, SYSTEM_CAPABILITY_STATION,
						16, "Station only", ""));
	}
	
	return (tempLen + 2);
}

/* Dissect Management Address TLV */
static gint32
dissect_lldp_management_address(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, guint32 offset)
{
	guint16 tempShort;
	guint32 tempLen = 0;
	guint8  tempByte;
	guint8  stringLen = 0;
	guint32 tempOffset = offset;
	guint32 tempLong;
	
	proto_tree	*system_mgm_addr = NULL;
	proto_item 	*tf = NULL;
	
	/* Get tlv type and length */
	tempShort = tvb_get_ntohs(tvb, tempOffset);
		
	/* Get tlv length */
	tempLen = TLV_INFO_LEN(tempShort);
			
	if (tree)
	{
		/* Set system capabilities tree */
		tf = proto_tree_add_text(tree, tvb, tempOffset, (tempLen + 2), "Management Address");
		system_mgm_addr = proto_item_add_subtree(tf, ett_management_address);
		
		proto_tree_add_item(system_mgm_addr, hf_lldp_tlv_type, tvb, tempOffset, 2, FALSE);
		proto_tree_add_item(system_mgm_addr, hf_lldp_tlv_len, tvb, tempOffset, 2, FALSE);
		
		tempOffset += 2;
		
		/* Get management address string length */
		stringLen = tvb_get_guint8(tvb, tempOffset);
		proto_tree_add_text(system_mgm_addr, tvb, tempOffset, 1, "Address String Length: %u", stringLen);
		
		tempOffset++;
		
		/* Get management address subtype */
		tempByte = tvb_get_guint8(tvb, tempOffset);
		proto_tree_add_text(system_mgm_addr, tvb, tempOffset, 1, "Address Subtype: %s (%u)",
							val_to_str(tempByte, management_addr_values, "Undefined"),
							tempByte);
							
		tempOffset++;
		
		/* Get address */
		switch (tempByte)
		{
		case 1:		/* IPv4 */
			proto_tree_add_item(system_mgm_addr, hf_mgn_addr_ipv4, tvb, tempOffset, 4, FALSE);
			break;
		case 2:		/* IPv6 */
			proto_tree_add_item(system_mgm_addr, hf_mgn_addr_ipv6, tvb, tempOffset, (stringLen-1), FALSE);
			break;
		default:	
			proto_tree_add_item(system_mgm_addr, hf_mgn_addr_hex, tvb, tempOffset, (stringLen-1), FALSE);
			break;
		}
		
		tempOffset += (stringLen-1);
		
		/* Get interface numbering subtype */
		tempByte = tvb_get_guint8(tvb, tempOffset);
		proto_tree_add_text(system_mgm_addr, tvb, tempOffset, 1, "Interface Subtype: %s (%u)",
							val_to_str(tempByte, interface_subtype_values, "Undefined"),
							tempByte);
		
		tempOffset++;
							
		/* Get interface number */
		tempLong = tvb_get_ntohl(tvb, tempOffset);
		proto_tree_add_text(system_mgm_addr, tvb, tempOffset, 4, "Interface Number: %u", tempLong);
		
		tempOffset += 4;
		
		/* Get OID string length */
		stringLen = tvb_get_guint8(tvb, tempOffset);
		proto_tree_add_text(system_mgm_addr, tvb, tempOffset, 1, "OID String Length: %u", stringLen);
		
		if (stringLen > 0)
		{
			tempOffset++;
			
			/* Get OID identifier */
			proto_tree_add_item(system_mgm_addr, hf_mgn_obj_id, tvb, tempOffset, stringLen, FALSE);
		}
	}
	
	return (tempLen + 2);
}

/* Dissect IEEE 802.1 TLVs */
static void
dissect_ieee_802_1_tlv(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, guint32 offset)
{
	guint8 subType;
	guint8 tempByte;
	guint16 tempShort;
	guint32 tempOffset = offset;
	
	proto_tree	*vlan_flags = NULL;
	proto_item 	*tf = NULL;
	
	/* Get subtype */
	subType = tvb_get_guint8(tvb, tempOffset);
	
	if (tree)
		proto_tree_add_item(tree, hf_ieee_802_1_subtype, tvb, tempOffset, 1, FALSE);
		
	tempOffset++;
	
	switch (subType)
	{
	case 0x01:	/* Port VLAN ID */
	{
		/* Get port vland id */
		tempShort = tvb_get_ntohs(tvb, tempOffset);
		if (tree)
			proto_tree_add_text(tree, tvb, tempOffset, 2, "Port VLAN Identifier: 0x%04X", tempShort);
			
		break;
	}
	case 0x02:	/* Port and Protocol VLAN ID */
	{
		/* Get flags */
		tempByte = tvb_get_guint8(tvb, tempOffset);
		if (tree)
		{
			tf = proto_tree_add_text(tree, tvb, tempOffset, 1, "Flags: 0x%02x", tempByte);
			vlan_flags = proto_item_add_subtree(tf, ett_port_vlan_flags);
		
			/* Get supported flag */
			proto_tree_add_text(vlan_flags, tvb, tempOffset, 1, "%s",
						decode_boolean_bitfield(tempByte, 0x01, 8, "Port and Protocol VLAN: Supported", 
												"Port and Protocol VLAN: Not Supported"));
									
			/* Get enabled flag */
			proto_tree_add_text(vlan_flags, tvb, tempOffset, 1, "%s",
						decode_boolean_bitfield(tempByte, 0x02, 8, "Port and Protocol VLAN: Enabled", 
												"Port and Protocol VLAN: Not Enabled"));
		}
		
		tempOffset++;
		
		/* Get port and protocol vlan id */
		tempShort = tvb_get_ntohs(tvb, tempOffset);
		if (tree)
			proto_tree_add_text(tree, tvb, tempOffset, 2, "Port and Protocol VLAN Identifier: 0x%04X", tempShort);
		
		break;
	}
	case 0x03:	/* VLAN Name */
	{
		/* Get vlan id */
		tempShort = tvb_get_ntohs(tvb, tempOffset);
		if (tree)
			proto_tree_add_text(tree, tvb, tempOffset, 2, "VLAN Identifier: 0x%04X", tempShort);
			
		tempOffset += 2;
		
		/* Get vlan name length */
		tempByte = tvb_get_guint8(tvb, tempOffset);
		if (tree)
			proto_tree_add_text(tree, tvb, tempOffset, 1, "VLAN Name Length: %u", tempByte);
		
		tempOffset++;
		
		if (tempByte > 0)
		{
			if (tree)
				proto_tree_add_text(tree, tvb, tempOffset, tempByte, "VLAN Name: %s",
				    tvb_format_stringzpad(tvb, tempOffset, tempByte));
		}
		
		break;
	}
	case 0x04:	/* Protocol ID */
	{
		/* Get protocal id length */
		tempByte = tvb_get_guint8(tvb, tempOffset);
		if (tree)
			proto_tree_add_text(tree, tvb, tempOffset, 1, "Protocol Identity Length: %u", tempByte);
		
		tempOffset++;
		
		if (tempByte > 0)
		{
			if (tree)
				proto_tree_add_text(tree, tvb, tempOffset, tempByte, "Protocol Identity: %s",
					tvb_bytes_to_str(tvb, tempOffset, tempByte));
		}
		
		break;
	}
	}
	
	return;
}

/* Dissect IEEE 802.3 TLVs */
static void
dissect_ieee_802_3_tlv(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, guint32 offset)
{
	guint8 subType;
	guint8 tempByte;
	guint16 tempShort;
	guint32 tempLong;
	guint32 tempOffset = offset;
	
	proto_tree	*mac_phy_flags = NULL;
	proto_item 	*tf = NULL;
	
	/* Get subtype */
	subType = tvb_get_guint8(tvb, tempOffset);
	
	if (tree)
		proto_tree_add_item(tree, hf_ieee_802_3_subtype, tvb, tempOffset, 1, FALSE);
		
	tempOffset++;
	
	switch (subType)
	{
	case 0x01:	/* MAC/PHY Configuration/Status */
	{
		/* Get auto-negotiation info */
		tempByte = tvb_get_guint8(tvb, tempOffset);
		if (tree)
		{
			tf = proto_tree_add_text(tree, tvb, tempOffset, 1, "Auto-Negotiation Support/Status: 0x%02x", tempByte);
			mac_phy_flags = proto_item_add_subtree(tf, ett_802_3_flags);
		
			/* Get supported flag */
			proto_tree_add_text(mac_phy_flags, tvb, tempOffset, 1, "%s",
						decode_boolean_bitfield(tempByte, 0x01, 8, "Auto-Negotiation: Supported", 
												"Auto-Negotiation: Not Supported"));
									
			/* Get enabled flag */
			proto_tree_add_text(mac_phy_flags, tvb, tempOffset, 1, "%s",
						decode_boolean_bitfield(tempByte, 0x02, 8, "Auto-Negotiation: Enabled", 
												"Auto-Negotiation: Not Enabled"));
		}
		
		tempOffset++;
		
		/* Get pmd auto-negotiation advertised capability */
		tempShort = tvb_get_ntohs(tvb, tempOffset);
		if (tree)
			proto_tree_add_text(tree, tvb, tempOffset, 2, "PMD Auto-Negotiation Advertised Capability: 0x%04X", tempShort);
			
		tempOffset += 2;
		
		/* Get operational MAU type */
		tempShort = tvb_get_ntohs(tvb, tempOffset);
		if (tree)
			proto_tree_add_text(tree, tvb, tempOffset, 2, "Operational MAU Type: 0x%04X", tempShort);
			
		tempOffset += 2;
		
		break;
	}
	case 0x02:	/* MDI Power Support */
	{
		/* Get MDI power support info */
		tempByte = tvb_get_guint8(tvb, tempOffset);
		if (tree)
		{
			tf = proto_tree_add_text(tree, tvb, tempOffset, 1, "MDI Power Support: 0x%02x", tempByte);
			mac_phy_flags = proto_item_add_subtree(tf, ett_802_3_power);
		
			/* Get port class */
			proto_tree_add_text(mac_phy_flags, tvb, tempOffset, 1, "%s",
						decode_boolean_bitfield(tempByte, 0x01, 8, "Port Class: PSE", 
												"Port Class: PD"));
									
			/* Get PSE MDI power support */
			proto_tree_add_text(mac_phy_flags, tvb, tempOffset, 1, "%s",
						decode_boolean_bitfield(tempByte, 0x02, 8, "PSE MDI Power: Supported", 
												"PSE MDI Power: Not Supported"));
											
			/* Get PSE MDI power state */
			proto_tree_add_text(mac_phy_flags, tvb, tempOffset, 1, "%s",
						decode_boolean_bitfield(tempByte, 0x04, 8, "PSE MDI Power Enabled: Yes", 
												"PSE MDI Power Enabled: No"));
												
			/* Get PSE pairs control ability */
			proto_tree_add_text(mac_phy_flags, tvb, tempOffset, 1, "%s",
						decode_boolean_bitfield(tempByte, 0x08, 8, "PSE Pairs Control Ability: Yes", 
												"PSE Pairs Control Ability: No"));
		}
		
		tempOffset++;
		
		/* Get PSE power pair */
		tempByte = tvb_get_guint8(tvb, tempOffset);
		if (tree)
			proto_tree_add_text(tree, tvb, tempOffset, 1, "PSE Power Pair: %u", tempByte);
			
		tempOffset++;
		
		/* Get power class */
		tempByte = tvb_get_guint8(tvb, tempOffset);
		if (tree)
			proto_tree_add_text(tree, tvb, tempOffset, 1, "Power Class: %u", tempByte);
			
		tempOffset++;
		
		break;
	}
	case 0x03:	/* Link Aggregation */
	{
		/* Get aggregation status */
		tempByte = tvb_get_guint8(tvb, tempOffset);
		if (tree)
		{
			tf = proto_tree_add_text(tree, tvb, tempOffset, 1, "Aggregation Status: 0x%02x", tempByte);
			mac_phy_flags = proto_item_add_subtree(tf, ett_802_3_aggregation);
		
			/* Get aggregation capability */
			proto_tree_add_text(mac_phy_flags, tvb, tempOffset, 1, "%s",
						decode_boolean_bitfield(tempByte, 0x01, 8, "Aggregation Capability: Yes", 
												"Aggregation Capability: No"));
									
			/* Get aggregation status */
			proto_tree_add_text(mac_phy_flags, tvb, tempOffset, 1, "%s",
						decode_boolean_bitfield(tempByte, 0x02, 8, "Aggregation Status: Enabled", 
												"Aggregation Status: Not Enabled"));
		}
		
		tempOffset++;
		
		/* Get aggregated port id */
		tempLong = tvb_get_ntohl(tvb, tempOffset);
		if (tree)
			proto_tree_add_text(tree, tvb, tempOffset, 4, "Aggregated Port Id: %u", tempLong);
			
		tempOffset += 4;
		
		break;
	}
	case 0x04:	/* Maximum Frame Size */
	{
		/* Get maximum frame size */
		tempShort = tvb_get_ntohs(tvb, tempOffset);
		if (tree)
			proto_tree_add_text(tree, tvb, tempOffset, 2, "Maximum Frame Size: %u", tempShort);
			
		tempOffset += 2;
		
		break; 
	}
	}
		
	return;
}

/* Dissect Media TLVs */
static void
dissect_media_tlv(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, guint32 offset, guint16 tlvLen)
{
	guint32 tempOffset = offset;
	guint8 subType;
	guint16 tempShort;
	guint16 tempVLAN;
	guint8 tempByte;
	guint32 tempLong;
	guint8 tempStr[255];
	const char *strPtr;
	guint32 LCI_Length;
	guint64 temp64bit = 0;
	
	proto_tree	*media_flags = NULL;
	proto_item 	*tf = NULL;
	
	/* Get subtype */
	subType = tvb_get_guint8(tvb, tempOffset);
	if (tree)
		proto_tree_add_item(tree, hf_media_tlv_subtype, tvb, tempOffset, 1, FALSE);
	tempOffset++;
	tlvLen--;
	
	switch (subType)
	{
	case 1:		/* LLDP-MED Capabilities */
	{
		/* Get capabilities */
		if (tlvLen < 2)
		{
			proto_tree_add_text(tree, tvb, tempOffset, 0, "TLV too short");
			return;
		}
		tempShort = tvb_get_ntohs(tvb, tempOffset);
		if (tree)
		{
			tf = proto_tree_add_text(tree, tvb, tempOffset, 2, "Capabilities: 0x%04x", tempShort);
			media_flags = proto_item_add_subtree(tf, ett_media_capabilities);
			if (tempShort & MEDIA_CAPABILITY_LLDP)
				proto_tree_add_text(media_flags, tvb, tempOffset, 2, "%s",
						decode_boolean_bitfield(tempShort, MEDIA_CAPABILITY_LLDP, 16, 
												"LLDP-MED Capabilities", ""));
			if (tempShort & MEDIA_CAPABILITY_NETWORK_POLICY)
				proto_tree_add_text(media_flags, tvb, tempOffset, 2, "%s",
						decode_boolean_bitfield(tempShort, MEDIA_CAPABILITY_NETWORK_POLICY, 16, 
												"Network Policy", ""));
			if (tempShort & MEDIA_CAPABILITY_LOCATION_ID)
				proto_tree_add_text(media_flags, tvb, tempOffset, 2, "%s",
						decode_boolean_bitfield(tempShort, MEDIA_CAPABILITY_LOCATION_ID, 16, 
												"Location Identification", ""));
			if (tempShort & MEDIA_CAPABILITY_MDI_PSE)
				proto_tree_add_text(media_flags, tvb, tempOffset, 2, "%s",
						decode_boolean_bitfield(tempShort, MEDIA_CAPABILITY_MDI_PSE, 16, 
												"Extended Power via MDI-PSE", ""));
			if (tempShort & MEDIA_CAPABILITY_MDI_PD)
				proto_tree_add_text(media_flags, tvb, tempOffset, 2, "%s",
						decode_boolean_bitfield(tempShort, MEDIA_CAPABILITY_MDI_PD, 16, 
												"Extended Power via MDI-PD", ""));
			if (tempShort & MEDIA_CAPABILITY_INVENTORY)
				proto_tree_add_text(media_flags, tvb, tempOffset, 2, "%s",
						decode_boolean_bitfield(tempShort, MEDIA_CAPABILITY_INVENTORY, 16, 
												"Inventory", ""));
		}
		tempOffset += 2;
		tlvLen -= 2;
		
		/* Get Class type */
		if (tlvLen < 1)
		{
			proto_tree_add_text(tree, tvb, tempOffset, 0, "TLV too short");
			return;
		}
		tempByte = tvb_get_guint8(tvb, tempOffset);
		if (tree)
			proto_tree_add_text(tree, tvb, tempOffset, 1, "Class Type: %s", val_to_str(tempByte, media_class_values, "Unknown"));
		tempOffset++;
		tlvLen--;
	
		break;
	}
	case 2:		/* Network Policy */
	{
		/* Get application type */
		if (tlvLen < 1)
		{
			proto_tree_add_text(tree, tvb, tempOffset, 0, "TLV too short");
			return;
		}
		tempByte = tvb_get_guint8(tvb, tempOffset);
		if (tree)
			proto_tree_add_text(tree, tvb, tempOffset, 1, "Applicaton Type: %s (%u)", 
							val_to_str(tempByte, media_application_type, "Unknown"), tempByte);
		tempOffset++;
		tlvLen--;
		
		/* Get flags */
		if (tlvLen < 1)
		{
			proto_tree_add_text(tree, tvb, tempOffset, 0, "TLV too short");
			return;
		}
		tempByte = tvb_get_guint8(tvb, tempOffset);
		
		/* Unknown policy flag */
		if (tree)
			proto_tree_add_text(tree, tvb, tempOffset, 1, "%s",
						decode_boolean_bitfield(tempByte, 0x80, 8,"Policy: Unknown", "Policy: Defined"));
		
		/* Tagged flag */
		if (tree)
			proto_tree_add_text(tree, tvb, tempOffset, 1, "%s",
						decode_boolean_bitfield(tempByte, 0x40, 8,"Tagged: Yes", "Tagged: No"));
		tempOffset++;
		tlvLen--;
						
		/* Get vlan id */
		if (tlvLen < 1)
		{
			proto_tree_add_text(tree, tvb, tempOffset, 0, "TLV too short");
			return;
		}
		tempShort = tvb_get_ntohs(tvb, tempOffset);
		tempVLAN = (tempShort & 0x1FFE) >> 1;
		if (tree)
			proto_tree_add_text(tree, tvb, tempOffset, 2, "%s %u",
						decode_boolean_bitfield(tempShort, 0x1FFE, 16, "VLAN Id:", "VLAN Id:"), tempVLAN);
		tempOffset++;
		tlvLen--;
		
		/* Get L2 priority */
		if (tlvLen < 1)
		{
			proto_tree_add_text(tree, tvb, tempOffset, 0, "TLV too short");
			return;
		}
		tempShort = tvb_get_ntohs(tvb, tempOffset);
		if (tree)
			proto_tree_add_text(tree, tvb, tempOffset, 2, "%s %u",
						decode_boolean_bitfield(tempShort, 0x01C0, 16, "L2 Priority:", "L2 Priority:"), 
												((tempShort & 0x01C0) >> 6));
		tempOffset++;
		tlvLen--;
													
		/* Get DSCP value */
		tempByte = tvb_get_guint8(tvb, tempOffset);
		if (tree)
			proto_tree_add_text(tree, tvb, tempOffset, 1, "%s %u",
						decode_boolean_bitfield(tempByte, 0x3F, 8, "DSCP Value:", "DSCP Value:"), 
												(tempByte & 0x3F));
		
		break;
	}
	case 3:	/* Location Identification */
	{
		/* Get location data format */
		if (tlvLen < 1)
		{
			proto_tree_add_text(tree, tvb, tempOffset, 0, "TLV too short");
			return;
		}
		tempByte = tvb_get_guint8(tvb, tempOffset);
		if (tree)
			proto_tree_add_text(tree, tvb, tempOffset, 1, "Location Data Format: %s (%u)", 
							val_to_str(tempByte, location_data_format, "Unknown"), tempByte);
		tempOffset++;
		tlvLen--;
		
		switch (tempByte)
		{
		case 1:	/* Coordinate-based LCI */
		{
			if (tlvLen < 16)
			{
				proto_tree_add_text(tree, tvb, tempOffset, 0, "TLV too short");
				return;
			}
			
			/* Get latitude resolution */
			tempByte = tvb_get_guint8(tvb, tempOffset);
			if (tree)
				proto_tree_add_text(tree, tvb, tempOffset, 1, "%s %u",
						decode_boolean_bitfield(tempByte, 0xFC, 8, "Latitude Resolution:", "Latitude Resolution:"), 
												((tempByte & 0xFC) >> 2));
												
			/* Get latitude */
			temp64bit = tvb_get_ntoh64(tvb, tempOffset);
			temp64bit = (temp64bit & G_GINT64_CONSTANT(0x03FFFFFFFF000000)) >> 24;
			get_latitude_or_longitude(0,temp64bit,tempStr);
			if (tree)
				proto_tree_add_text(tree, tvb, tempOffset, 5, "Latitude: %s (0x%16" PRIX64 ")", 
									tempStr, temp64bit);
				
			tempOffset += 5;
			
			/* Get longitude resolution */
			tempByte = tvb_get_guint8(tvb, tempOffset);
			if (tree)
				proto_tree_add_text(tree, tvb, tempOffset, 1, "%s %u",
					decode_boolean_bitfield(tempByte, 0xFC, 8, "Longitude Resolution:", "Longitude Resolution:"), 
											((tempByte & 0xFC) >> 2));

			/* Get longitude */
			temp64bit = tvb_get_ntoh64(tvb, tempOffset);
			temp64bit = (temp64bit & G_GINT64_CONSTANT(0x03FFFFFFFF000000)) >> 24;
			get_latitude_or_longitude(1,temp64bit,tempStr);
			if (tree)
				proto_tree_add_text(tree, tvb, tempOffset, 5, "Longitude: %s (0x%16" PRIX64 ")", 
									tempStr,temp64bit);
				
			tempOffset += 5;
			
			/* Altitude Type */
			tempByte = tvb_get_guint8(tvb, tempOffset);
			if (tree)
			{
				tf = proto_tree_add_text(tree, tvb, tempOffset, 1, "%s",
						decode_boolean_bitfield(tempByte, 0xF0, 8, "Altitude Type: ", "Altitude Type: "));
				
				switch ((tempByte >> 4))
				{
				case 1:
					proto_item_append_text(tf, "Meters (1)");
					break;
				case 2:
					proto_item_append_text(tf, "Floors (2)");
					break;
				default:
					proto_item_append_text(tf, " Unknown (%u)", (tempByte >> 4));
					break;
				}
			}
			
			/* Get Altitude Resolution */
			tempShort = tvb_get_ntohs(tvb, tempOffset);
			if (tree)
				proto_tree_add_text(tree, tvb, tempOffset, 2, "%s %u",
						decode_boolean_bitfield(tempShort, 0x0FC0, 16, "Altitude Resolution: ", "Altitude Type: "),
						((tempShort & 0x0FC0) >> 6));
						
			tempOffset++;
			
			/* Get Altitude */
			tempLong = (tvb_get_ntohl(tvb, tempOffset) & 0x03FFFFFFF);
			if (tree)
				proto_tree_add_text(tree, tvb, tempOffset, 4, "Altitude: 0x%08X", tempLong);
				
			tempOffset += 4;
			
			/* Get datum */
			tempByte = tvb_get_guint8(tvb, tempOffset);
			if (tree)
				proto_tree_add_text(tree, tvb, tempOffset, 1, "Datum: %u", tempByte);	
			
			break;
		}
		case 2: /* Civic Address LCI */
		{
			if (tlvLen < 1)
			{
				proto_tree_add_text(tree, tvb, tempOffset, 0, "TLV too short");
				return;
			}
			
			/* Get LCI length */
			tempByte = tvb_get_guint8(tvb, tempOffset);
			tlvLen--;
			if (tempByte > tlvLen)
			{
				if (tree)
					proto_tree_add_text(tree, tvb, tempOffset, 1, "LCI Length: %u (greater than TLV length)", tempByte); 
					
				return;
			}
			
			if (tree)
				proto_tree_add_text(tree, tvb, tempOffset, 1, "LCI Length: %u", tempByte); 
				
			LCI_Length = (guint32)tempByte;
				
			tempOffset++;
			
			/* Get what value */
			if (LCI_Length < 1)
			{
				proto_tree_add_text(tree, tvb, tempOffset, 0, "LCI Length too short");
				return;
			}
			tempByte = tvb_get_guint8(tvb, tempOffset);
			if (tree)
				proto_tree_add_text(tree, tvb, tempOffset, 1, "What: %s (%u)",
									val_to_str(tempByte,civic_address_what_values,"Unknown"),
									tempByte);
			tempOffset++;
			LCI_Length--;
			
			/* Get country code */
			if (LCI_Length < 2)
			{
				proto_tree_add_text(tree, tvb, tempOffset, 0, "LCI Length too short");
				return;
			}
			if (tree)
				proto_tree_add_text(tree, tvb, tempOffset, 2, "Country: %s",
				    tvb_format_text(tvb, tempOffset, 2));
				
			tempOffset += 2;
			LCI_Length -= 2;
			
			while (LCI_Length > 0)
			{
				/* Get CA Type */
				if (LCI_Length < 1)
				{
					proto_tree_add_text(tree, tvb, tempOffset, 0, "LCI Length too short");
					return;
				}
				tempByte = tvb_get_guint8(tvb, tempOffset);
				if (tree)
					proto_tree_add_text(tree, tvb, tempOffset, 1, "CA Type: %s (%u)",
									val_to_str(tempByte,civic_address_type_values,"Unknown"),
									tempByte);
									
				tempOffset++;
				LCI_Length--;
				
				/* Get CA Length */
				if (LCI_Length < 1)
				{
					proto_tree_add_text(tree, tvb, tempOffset, 0, "LCI Length too short");
					return;
				}
				tempByte = tvb_get_guint8(tvb, tempOffset);
				if (tree)
					proto_tree_add_text(tree, tvb, tempOffset, 1, "CA Length: %u", tempByte);
				
				tempOffset++;
				LCI_Length--;
				
				/* Make sure the CA value is within the specified length */
				if (tempByte > LCI_Length)
					return;
				
				if (tempByte > 0)
				{
					/* Get CA Value */
					if (tree)
						proto_tree_add_text(tree, tvb, tempOffset, tempByte, "CA Value: %s",
						    tvb_format_stringzpad(tvb, tempOffset, tempByte));
				
					tempOffset += tempByte;
					LCI_Length -= tempByte;
				}
			}
		
			break;
		}
		case 3: /* ECS ELIN */
		{
			if (tlvLen > 0)
			{
				if (tree)
					proto_tree_add_text(tree, tvb, tempOffset, tlvLen, "ELIN: %s",
					    tvb_format_stringzpad(tvb, tempOffset, tlvLen));
					
			}
		
			break;
		}
		}
		
		break;
	}
	case 4: /* Extended Power-via-MDI */
	{
		/* Get first byte */
		tempByte = tvb_get_guint8(tvb, tempOffset);
		
		/* Determine power type */
		subType = ((tempByte & 0xC0) >> 6);
		if (tree)
			proto_tree_add_text(tree, tvb, tempOffset, 1, "%s %s",
						decode_boolean_bitfield(tempByte, 0xC0, 8, "Power Type:", "Power Type:"), 
												val_to_str(subType, media_power_type, "Unknown"));
				
		/* Determine power source */								
		switch (subType)
		{
		case 0:
		{
			subType = ((tempByte & 0x30) >> 4);
			strPtr = val_to_str(subType, media_power_pse_device, "Reserved");
			
			break;
		}
		case 1:
		{
			subType = ((tempByte & 0x30) >> 4);
			strPtr = val_to_str(subType, media_power_pd_device, "Reserved");
			
			break;
		}
		default:
		{
			strPtr = "Unknown";
			break;
		}
		}
		if (tree)
			proto_tree_add_text(tree, tvb, tempOffset, 1, "%s %s",
						decode_boolean_bitfield(tempByte, 0x30, 8, "Power Source:", "Power Source:"), 
												strPtr);
						
		/* Determine power priority */
		subType = (tempByte & 0x0F);
		if (tree)
			proto_tree_add_text(tree, tvb, tempOffset, 1, "%s %s",
						decode_boolean_bitfield(tempByte, 0x0F, 8, "Power Priority:", "Power Priority:"), 
												val_to_str(subType, media_power_priority, "Reserved"));
							
		tempOffset++;
							
		/* Get power value */
		tempShort = tvb_get_ntohs(tvb, tempOffset);
		if (tree)
			proto_tree_add_text(tree, tvb, tempOffset, 2, "Power Value: %u", tempShort);
								
		break;
	}
	case 5:	/* Hardware Revision */
	{
		/* Figure out the length of the hardware revision field */
		if (tlvLen > 0)
		{
			if (tree)
				proto_tree_add_text(tree, tvb, tempOffset, tlvLen, "Hardware Revision: %s",
				    tvb_format_stringzpad(tvb, tempOffset, tlvLen));
		}
		
		break;
	}
	case 6:	/* Firmware Revision */
	{
		/* Figure out the length of the firmware revision field */
		if (tlvLen > 0)
		{
			if (tree)
				proto_tree_add_text(tree, tvb, tempOffset, tlvLen, "Firmware Revision: %s",
				    tvb_format_stringzpad(tvb, tempOffset, tlvLen));
		}
		
		break;
	}
	case 7:	/* Software Revision */
	{
		/* Figure out the length of the software revision field */
		if (tlvLen > 0)
		{
			if (tree)
				proto_tree_add_text(tree, tvb, tempOffset, tlvLen, "Software Revision: %s",
				    tvb_format_stringzpad(tvb, tempOffset, tlvLen));
		}
		
		break;
	}
	case 8:	/* Serial Number */
	{
		/* Figure out the length of the serial number field */
		if (tlvLen > 0)
		{
			if (tree)
				proto_tree_add_text(tree, tvb, tempOffset, tlvLen, "Serial Number: %s",
				    tvb_format_stringzpad(tvb, tempOffset, tlvLen));
		}
		
		break;
	}
	case 9:	/* Manufacturer Name */
	{
		/* Figure out the length of the manufacturer name field */
		if (tlvLen > 0)
		{
			if (tree)
				proto_tree_add_text(tree, tvb, tempOffset, tlvLen, "Manufacturer Name: %s",
				    tvb_format_stringzpad(tvb, tempOffset, tlvLen));
		}
		
		break;
	}
	case 10:	/* Model Name */
	{
		/* Figure out the length of the model name field */
		if (tlvLen > 0)
		{
			if (tree)
				proto_tree_add_text(tree, tvb, tempOffset, tlvLen, "Model Name: %s",
				    tvb_format_stringzpad(tvb, tempOffset, tlvLen));
		}
		
		break;
	}
	case 11:	/* Asset ID */
	{
		/* Figure out the length of the asset id field */
		if (tlvLen > 0)
		{
			if (tree)
				proto_tree_add_text(tree, tvb, tempOffset, tlvLen, "Asset ID: %s",
				    tvb_format_stringzpad(tvb, tempOffset, tlvLen));
		}
		
		break;
	}
	}
	
	return;
}

/* Dissect Organizational Specific TLV */
static gint32
dissect_organizational_specific_tlv(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, guint32 offset)
{
	guint16 tempLen;
	guint16 tempShort;
	guint32 oui;
	guint8 subType;
	char tempStr[255];
	
	proto_tree	*org_tlv_tree = NULL;
	proto_item 	*tf = NULL;
	
	/* Get tlv type and length */
	tempShort = tvb_get_ntohs(tvb, offset);
		
	/* Get tlv length */
	tempLen = TLV_INFO_LEN(tempShort);
	
	/* Get OUI value */
	oui = tvb_get_ntoh24(tvb, (offset+2));
	subType = tvb_get_guint8(tvb, (offset+5));
	
	sprintf(tempStr,"%s - ",val_to_str(oui, tlv_oui_subtype_vals, "Unknown"));
	switch(oui)
	{
	case OUI_IEEE_802_1:
		strcat(tempStr,val_to_str(subType, ieee_802_1_subtypes, "Unknown"));
		break;
	case OUI_IEEE_802_3:
		strcat(tempStr,val_to_str(subType, ieee_802_3_subtypes, "Unknown"));
		break;
	case OUI_MEDIA_ENDPOINT:
		strcat(tempStr,val_to_str(subType, media_subtypes, "Unknown"));
		break;
	default:
		strcat(tempStr,"Unknown");
		break;
	}
	
	if (tree)
	{
		tf = proto_tree_add_text(tree, tvb, offset, (tempLen + 2), "%s", tempStr);
		org_tlv_tree = proto_item_add_subtree(tf, ett_org_spc_tlv);
		
		proto_tree_add_item(org_tlv_tree, hf_lldp_tlv_type, tvb, offset, 2, FALSE);
	}
	if (tempLen < 4)
	{
		if (tree)
			proto_tree_add_uint_format(org_tlv_tree, hf_lldp_tlv_len, tvb, offset, 2,
			    tempShort, "TLV Length: %u (too short, must be >= 4)", tempLen);
			
		return (tempLen + 2);
	}
	if (tree)
	{
		proto_tree_add_item(org_tlv_tree, hf_lldp_tlv_len, tvb, offset, 2, FALSE);
		
		/* Display organizational unique id */
		proto_tree_add_uint(org_tlv_tree, hf_org_spc_oui, tvb, (offset+2), 3, oui);
	}
	
	switch (oui)
	{
	case OUI_IEEE_802_1:
		dissect_ieee_802_1_tlv(tvb, pinfo, org_tlv_tree, (offset+5));
		break;
	case OUI_IEEE_802_3:
		dissect_ieee_802_3_tlv(tvb, pinfo, org_tlv_tree, (offset+5));
		break;
	case OUI_MEDIA_ENDPOINT:
		dissect_media_tlv(tvb, pinfo, org_tlv_tree, (offset+5), (tempLen-3));
		break;
	}
	
	return (tempLen + 2);
}

/* Dissect Unknown TLV */
static gint32
dissect_lldp_unknown_tlv(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, guint32 offset)
{
	guint16 tempLen;
	guint16 tempShort;
	
	proto_tree	*unknown_tlv_tree = NULL;
	proto_item 	*tf = NULL;
	
	/* Get tlv type and length */
	tempShort = tvb_get_ntohs(tvb, offset);
		
	/* Get tlv length */
	tempLen = TLV_INFO_LEN(tempShort);
	
	if (tree)
	{
		tf = proto_tree_add_text(tree, tvb, offset, (tempLen + 2), "Unknown TLV");
		unknown_tlv_tree = proto_item_add_subtree(tf, ett_unknown_tlv);
		
		proto_tree_add_item(unknown_tlv_tree, hf_lldp_tlv_type, tvb, offset, 2, FALSE);
		proto_tree_add_item(unknown_tlv_tree, hf_lldp_tlv_len, tvb, offset, 2, FALSE);
	}
	
	return (tempLen + 2);
}


/* Dissect LLDP packets */
static void
dissect_lldp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	proto_item *ti;
	proto_tree *lldp_tree = NULL;
	
	guint32 offset = 0;
	gint32 rtnValue = 0;
	guint16 tempShort;
	guint8 tempType;
	gboolean reachedEnd = FALSE;

	if (check_col(pinfo->cinfo, COL_PROTOCOL)) 
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "LLDP");
		
	/* Clear the information column on summary display */
	if (check_col(pinfo->cinfo, COL_INFO)) {
			col_clear(pinfo->cinfo, COL_INFO);
	}

	if (tree) 
	{
		ti = proto_tree_add_item(tree, proto_lldp, tvb, offset, -1, FALSE);
		lldp_tree = proto_item_add_subtree(ti, ett_lldp);
	}
	
	/* Get chassis id tlv */
	rtnValue = dissect_lldp_chassis_id(tvb, pinfo, lldp_tree, offset);
	if (rtnValue < 0)
	{
		if (check_col(pinfo->cinfo, COL_INFO)) 
			col_set_str(pinfo->cinfo, COL_INFO, "Invalid Chassis ID TLV");
			
		return;
	}
	
	offset += rtnValue;
	
	/* Get port id tlv */
	rtnValue = dissect_lldp_port_id(tvb, pinfo, lldp_tree, offset);
	if (rtnValue < 0)
	{
		if (check_col(pinfo->cinfo, COL_INFO)) 
			col_set_str(pinfo->cinfo, COL_INFO, "Invalid Port ID TLV");
			
		return;
	}
	
	offset += rtnValue;
	
	/* Get time to live tlv */
	rtnValue = dissect_lldp_time_to_live(tvb, pinfo, lldp_tree, offset);
	if (rtnValue < 0)
	{
		if (check_col(pinfo->cinfo, COL_INFO)) 
			col_set_str(pinfo->cinfo, COL_INFO, "Invalid Time-to-Live TLV");
			
		return;
	}
	
	offset += rtnValue;
	
	/* Dissect optional tlv's until end-of-lldpdu is reached */
	while (!reachedEnd)
	{
		tempShort = tvb_get_ntohs(tvb, offset);
		tempType = TLV_TYPE(tempShort);
		
		switch (tempType)
		{
		case CHASSIS_ID_TLV_TYPE:
			rtnValue = dissect_lldp_chassis_id(tvb, pinfo, lldp_tree, offset);
			rtnValue = -1;	/* Duplicate chassis id tlv */
			if (check_col(pinfo->cinfo, COL_INFO)) 
				col_set_str(pinfo->cinfo, COL_INFO, "Duplicate Chassis ID TLV");
			break;
		case PORT_ID_TLV_TYPE:
			rtnValue = dissect_lldp_port_id(tvb, pinfo, lldp_tree, offset);
			rtnValue = -1;	/* Duplicate port id tlv */
			if (check_col(pinfo->cinfo, COL_INFO)) 
				col_set_str(pinfo->cinfo, COL_INFO, "Duplicate Port ID TLV");
			break;
		case TIME_TO_LIVE_TLV_TYPE:
			rtnValue = dissect_lldp_time_to_live(tvb, pinfo, lldp_tree, offset);
			rtnValue = -1;	/* Duplicate time-to-live tlv */
			if (check_col(pinfo->cinfo, COL_INFO)) 
				col_set_str(pinfo->cinfo, COL_INFO, "Duplicate Time-To-Live TLV");
			break;
		case END_OF_LLDPDU_TLV_TYPE:
			rtnValue = dissect_lldp_end_of_lldpdu(tvb, pinfo, lldp_tree, offset);
			break;
		case PORT_DESCRIPTION_TLV_TYPE:
			rtnValue = dissect_lldp_port_desc(tvb, pinfo, lldp_tree, offset);
			break;
		case SYSTEM_NAME_TLV_TYPE:
		case SYSTEM_DESCRIPTION_TLV_TYPE:
			rtnValue = dissect_lldp_system_name(tvb, pinfo, lldp_tree, offset);
			break;
		case SYSTEM_CAPABILITIES_TLV_TYPE:
			rtnValue = dissect_lldp_system_capabilities(tvb, pinfo, lldp_tree, offset);
			break;
		case MANAGEMENT_ADDR_TLV_TYPE:
			rtnValue = dissect_lldp_management_address(tvb, pinfo, lldp_tree, offset);
			break;
		case ORGANIZATION_SPECIFIC_TLV_TYPE:
			rtnValue = dissect_organizational_specific_tlv(tvb, pinfo, lldp_tree, offset);
			break;
		default:
			rtnValue = dissect_lldp_unknown_tlv(tvb, pinfo, lldp_tree, offset);
			break;
		}
		
		if (rtnValue < 0)
			reachedEnd = TRUE;
		else
			offset += rtnValue;
	}
	
}

/* Register the protocol with Ethereal */
void
proto_register_lldp(void)
{
	/* Setup list of header fields */
	static hf_register_info hf[] = {
		{ &hf_lldp_tlv_type,
			{ "TLV Type", "lldp.tlv.type", FT_UINT16, BASE_DEC, 
			VALS(tlv_types), TLV_TYPE_MASK, "", HFILL }
		},
		{ &hf_lldp_tlv_len,
			{ "TLV Length", "lldp.tlv.len", FT_UINT16, BASE_DEC, 
			NULL, TLV_INFO_LEN_MASK, "", HFILL }
		},
		{ &hf_chassis_id_subtype,
			{ "Chassis Id Subtype", "lldp.chassis.subtype", FT_UINT8, BASE_DEC, 
			VALS(chassis_id_subtypes), 0, "", HFILL }
		},
		{ &hf_chassis_id,
			{ "Chassis Id", "lldp.chassis.id", FT_BYTES, BASE_HEX, 
			NULL, 0, "", HFILL }
		},
		{ &hf_chassis_id_mac,
			{ "Chassis Id", "lldp.chassis.id.mac", FT_ETHER, BASE_NONE, 
			NULL, 0, "", HFILL }
		},
		{ &hf_chassis_id_ip4,
			{ "Chassis Id", "lldp.chassis.id.ip4", FT_IPv4, BASE_NONE, 
			NULL, 0, "", HFILL }
		},
		{ &hf_chassis_id_ip6,
			{ "Chassis Id", "lldp.chassis.id.ip6", FT_IPv6, BASE_NONE, 
			NULL, 0, "", HFILL }
		},
		{ &hf_port_id_subtype,
			{ "Port Id Subtype", "lldp.port.subtype", FT_UINT8, BASE_DEC, 
			VALS(port_id_subtypes), 0, "", HFILL }
		},
		{ &hf_port_id_mac,
			{ "Port Id", "lldp.port.id.mac", FT_ETHER, BASE_NONE, 
			NULL, 0, "", HFILL }
		},
		{ &hf_port_id_ip4,
			{ "Port Id", "lldp.port.id.ip4", FT_IPv4, BASE_NONE, 
			NULL, 0, "", HFILL }
		},
		{ &hf_port_id_ip6,
			{ "Port Id", "lldp.port.id.ip6", FT_IPv6, BASE_NONE, 
			NULL, 0, "", HFILL }
		},
		{ &hf_time_to_live,
			{ "Seconds", "lldp.time_to_live", FT_UINT16, BASE_DEC, 
			NULL, 0, "", HFILL }
		},
		{ &hf_mgn_addr_ipv4,
			{ "Management Address", "lldp.mgn.addr.ip4", FT_IPv4, BASE_NONE, 
			NULL, 0, "", HFILL }
		},
		{ &hf_mgn_addr_ipv6,
			{ "Management Address", "lldp.mgn.addr.ip6", FT_IPv6, BASE_NONE, 
			NULL, 0, "", HFILL }
		},
		{ &hf_mgn_addr_hex,
			{ "Management Address", "lldp.mgn.addr.hex", FT_BYTES, BASE_HEX, 
			NULL, 0, "", HFILL }
		},
		{ &hf_mgn_obj_id,
			{ "Object Identifier", "lldp.mgn.obj.id", FT_BYTES, BASE_HEX, 
			NULL, 0, "", HFILL }
		},
		{ & hf_org_spc_oui,
			{ "Organization Unique Code",	"lldp.orgtlv.oui", FT_UINT24, BASE_HEX,
	   		VALS(tlv_oui_subtype_vals), 0x0, "", HFILL }
		},
		{ &hf_ieee_802_1_subtype,
			{ "IEEE 802.1 Subtype",	"lldp.ieee.802_1.subtype", FT_UINT8, BASE_HEX,
	   		VALS(ieee_802_1_subtypes), 0x0, "", HFILL }
		},
		{ &hf_ieee_802_3_subtype,
			{ "IEEE 802.3 Subtype",	"lldp.ieee.802_3.subtype", FT_UINT8, BASE_HEX,
	   		VALS(ieee_802_3_subtypes), 0x0, "", HFILL }
		},
		{ &hf_media_tlv_subtype,
			{ "Media Subtype",	"lldp.media.subtype", FT_UINT8, BASE_HEX,
	   		VALS(media_subtypes), 0x0, "", HFILL }
		},
	};
	
	/* Setup protocol subtree array */
	static gint *ett[] = {
		&ett_lldp,
		&ett_chassis_id,
		&ett_port_id,
		&ett_time_to_live,
		&ett_end_of_lldpdu,
		&ett_port_description,
		&ett_system_name,
		&ett_system_cap,
		&ett_system_cap_summary,
		&ett_system_cap_enabled,
		&ett_management_address,
		&ett_unknown_tlv,
		&ett_org_spc_tlv,
		&ett_port_vlan_flags,
		&ett_802_3_flags,
		&ett_802_3_power,
		&ett_802_3_aggregation,
		&ett_media_capabilities,
	};

	/* Register the protocol name and description */
	proto_lldp = proto_register_protocol("Link Layer Discovery Protocol", "LLDP", "lldp");

	/* Required function calls to register the header fields and subtrees used */
	proto_register_field_array(proto_lldp, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));
}

void
proto_reg_handoff_lldp(void)
{
	dissector_handle_t lldp_handle;

	lldp_handle = create_dissector_handle(dissect_lldp,proto_lldp);
	dissector_add("ethertype", ETHERTYPE_LLDP, lldp_handle);
}
