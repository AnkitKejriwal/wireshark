/* packet-opsi.c
 * Routines for OPSI protocol dissection
 * Copyright 2004, Laurent Rabret (France Telecom R&D) <laurent.rabret@i.hate.spams.org>
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

#include "plugins/plugin_api.h"
#include "moduleinfo.h"
#include <gmodule.h>
#include "plugins/plugin_api_defs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#ifdef NEED_SNPRINTF_H
# include "snprintf.h"
#endif

#include <epan/packet.h>
#include <epan/dissectors/packet-tcp.h>
#include "prefs.h"

#include "packet-opsi.h"

/* Plugin exported constants */
#ifndef ENABLE_STATIC
G_MODULE_EXPORT const gchar version[] = VERSION;
#endif 


/* Initialize the protocol and registered fields */
static int proto_opsi 			= -1;
static int hf_opsi_major_version 	= -1;
static int hf_opsi_minor_version 	= -1;
static int hf_opsi_opcode	 	= -1;
static int hf_opsi_hook_id	 	= -1;
static int hf_opsi_length	 	= -1;
static int hf_opsi_session_id	 	= -1;
static int hf_user_name_att		= -1;
static int hf_password_att		= -1;
static int hf_chap_password_att		= -1;
static int hf_nas_ip_add_att		= -1;
static int hf_nas_port_att		= -1;
static int hf_service_type_att		= -1;
static int hf_framed_protocol_att	= -1;
static int hf_framed_address_att	= -1;
static int hf_framed_netmask_att	= -1;
static int hf_framed_routing_att	= -1;
static int hf_framed_filter_att		= -1;
static int hf_framed_mtu_att		= -1;
static int hf_framed_compression_att	= -1;
static int hf_called_station_att	= -1;
static int hf_calling_station_att	= -1;
static int hf_nas_identifier_att	= -1;
static int hf_accounting_att		= -1;
static int hf_acct_session_id_att	= -1;
static int hf_chap_challenge_att	= -1;
static int hf_nas_port_type_att		= -1;
static int hf_designation_num_att	= -1;
static int hf_nas_port_id_att		= -1;
static int hf_smc_aaa_id_att		= -1;
static int hf_smc_vpn_id_att		= -1;
static int hf_smc_vpn_name_att		= -1;
static int hf_smc_ran_id_att		= -1;
static int hf_smc_ran_ip_att		= -1;
static int hf_smc_ran_name_att		= -1;
static int hf_smc_pop_id_att		= -1;
static int hf_smc_pop_name_att		= -1;
static int hf_smc_id_att		= -1;
static int hf_smc_receive_time_att	= -1;
static int hf_smc_stat_time_att		= -1;

/* Initialize the subtree pointers */
static gint ett_opsi 			= -1;
static gint ett_opsi_user_name		= -1;
static gint ett_opsi_user_password	= -1;
static gint ett_opsi_chap_password	= -1;
static gint ett_opsi_nas_ip_address	= -1;
static gint ett_opsi_nas_port		= -1;
static gint ett_opsi_service_type	= -1;
static gint ett_opsi_framed_protocol	= -1;
static gint ett_opsi_framed_address	= -1;
static gint ett_opsi_framed_netmask	= -1;
static gint ett_opsi_framed_routing	= -1;
static gint ett_opsi_framed_filter	= -1;
static gint ett_opsi_framed_mtu		= -1;
static gint ett_opsi_framed_compression	= -1;
static gint ett_opsi_called_station_id	= -1;
static gint ett_opsi_calling_station_id	= -1;
static gint ett_opsi_nas_identifier	= -1;
static gint ett_opsi_accounting		= -1;
static gint ett_opsi_acct_session_id	= -1;
static gint ett_opsi_chap_challenge	= -1;
static gint ett_opsi_nas_port_type	= -1;
static gint ett_opsi_designation_number	= -1;
static gint ett_opsi_nas_port_id	= -1;
static gint ett_opsi_smc_aaa_id		= -1;
static gint ett_opsi_smc_vpn_id		= -1;
static gint ett_opsi_smc_vpn_name	= -1;
static gint ett_opsi_smc_ran_id		= -1;
static gint ett_opsi_smc_ran_ip		= -1;
static gint ett_opsi_smc_ran_name	= -1;
static gint ett_opsi_smc_pop_id		= -1;
static gint ett_opsi_smc_pop_name	= -1;
static gint ett_opsi_smc_id		= -1;
static gint ett_opsi_smc_receive_time	= -1;
static gint ett_opsi_smc_stat_time	= -1;


/* Code mapping */
static const value_string opsi_opcode[] = {
		{ DISCOVER_REQUEST, 	"Discover Request" },
		{ DISCOVER_RESPONSE, 	"Discover Response" },
		{ SERVICE_REQUEST, 	"Service Request" },
		{ SERVICE_ACCEPT,  	"Service Accept" }, 
		{ SERVICE_REJECT,  	"Service Reject" },
		{ TERMINATE_REQUEST, 	"Terminate Request" },
		{ 0,       		NULL }
	};
	
static const value_string opsi_service_type_code[] = {	
		{ 1, "Login" },
       		{ 2, "Framed" },
       		{ 3, "Callback Login" },
       		{ 4, "Callback Framed" },
       		{ 5, "Outbound" },
       		{ 6, "Administrative" },
       		{ 7, "NAS Prompt" },
       		{ 8, "Authenticate Only" },
       		{ 9, "Callback NAS Prompt" },
		{ 0,       		NULL }
	};

static const value_string opsi_framed_protocol_code[] = {	
		{ 1, 	"PPP" },
       		{ 2,  	"SLIP" },
        	{ 3,  	"AppleTalk Remote Access Protocol (ARAP)" },
        	{ 4,  	"Gandalf proprietary SingleLink/MultiLink protocol" },
       		{ 5,  	"Xylogics proprietary IPX/SLIP" },
       		{ 255, 	"Ascend ARA" },
       		{ 256,	"MPP" },
       		{ 257,	"EURAW" },
       		{ 258,	"EUUI" },
       		{ 259, 	"X25" },
       		{ 260,	"COMB" },
       		{ 261,	"FR" },
       		{ 262,	"MP" },
       		{ 263,	"FR-CIR"},
        	{ 0,       		NULL }
	};
	
static const value_string opsi_framed_routing_code[] = {
		{ 0,	"None" },
		{ 1,	"Broadcast" },
		{ 2,	"Listen" },
		{ 3,	"Broadcast-Listen" },
		{ 4,	"Broadcast V2" },
		{ 5,	"Listen V2" },
		{ 6,	"Broadcast-Listen V2" },
		{ 0,	NULL },
	};

static const value_string opsi_framed_compression_code[] = {
		{ 0,	"None" },
		{ 1,	"Van Jacobsen TCP/IP" },
		{ 2, 	"IPX header compression" },
		{ 0,	NULL }
	};

static const value_string opsi_nas_port_type_code[] = {	
		{ 0, "Async" },
      		{ 1, "Sync" },
      		{ 2, "ISDN Sync" },
      		{ 3, "ISDN Async V.120" },
      		{ 4, "ISDN Async V.110" },
      		{ 5, "Virtual" },
      		{ 6, "PIAFS" },
      		{ 7, "HDLC Clear Channel" },
      		{ 8, "X.25" },
      		{ 9, "X.75" },
      		{ 10, "G.3 Fax" },
      		{ 11, "SDSL - Symmetric DSL" },
      		{ 12, "ADSL-CAP - Asymmetric DSL, Carrierless Amplitude Phase Modulation" },
      		{ 13, "ADSL-DMT - Asymmetric DSL, Discrete Multi-Tone" },
      		{ 14, "IDSL - ISDN Digital Subscriber Line" },
      		{ 15, "Ethernet" },
      		{ 16, "xDSL - Digital Subscriber Line of unknown type" },
      		{ 17, "Cable" },
      		{ 18, "Wireless - Other" },
      		{ 19, "Wireless - IEEE 802.11" },
      		{ 201,"Voice over IP" },
      		{ 0,       		NULL }
	};

	
/* Structure used to decode OPSI frame attributes	*/
/* CAUTION : it is compulsory to sort this array	*/
/* (first argument of the opsi_attribute_handle_t)	*/
/* in ascending order 					*/
/*							*/
static opsi_attribute_handle_t opsi_attributes[] = {
	{ USER_NAME_ATTRIBUTE,		/* 1 */
	"User name attribute", &ett_opsi_user_name, &hf_user_name_att, decode_string_attribute },
	{ USER_PASSWD_ATTRIBUTE,	/* 2 */
	"User password attribute" , &ett_opsi_user_password, &hf_password_att, decode_string_attribute },
	{ CHAP_PASSWD_ATTRIBUTE,	/* 3 */
	"CHAP password attribute", &ett_opsi_chap_password, &hf_chap_password_att, decode_string_attribute },
	{ NAS_IP_ADDRESS_ATTRIBUTE,	/* 4 */
	"NAS IP address attribute", &ett_opsi_nas_ip_address, &hf_nas_ip_add_att, decode_ipv4_attribute },
	{NAS_PORT_ATTRIBUTE,		/* 5 */
	"NAS port attribute", &ett_opsi_nas_port, &hf_nas_port_att, decode_longint_attribute },
	{SERVICE_TYPE_ATTRIBUTE,	/* 6 */
	"Service type attribute", &ett_opsi_service_type, &hf_service_type_att, decode_value_string_attribute },
	{FRAMED_PROTOCOL_ATTRIBUTE,	/* 7 */
	"Framed protocol attribute", &ett_opsi_framed_protocol, &hf_framed_protocol_att, decode_value_string_attribute },
	{FRAMED_ADDRESS_ATTRIBUTE, 	/* 8 */
	"Framed address attribute", &ett_opsi_framed_address, &hf_framed_address_att, decode_ipv4_attribute },
	{FRAMED_NETMASK_ATTRIBUTE, 	/* 9 */
	"Framed netmask attribute", &ett_opsi_framed_netmask, &hf_framed_netmask_att, decode_ipv4_attribute },
	{FRAMED_ROUTING_ATTRIBUTE, 	/* 10 */
	"Framed routing attribute", &ett_opsi_framed_routing, &hf_framed_routing_att, decode_value_string_attribute },
	{FRAMED_FILTER_ATTRIBUTE, 	/* 11 */
	"Framed filter attribute", &ett_opsi_framed_filter, &hf_framed_filter_att, decode_string_attribute },
	{FRAMED_MTU_ATTRIBUTE, 		/* 12 */
	"Framed MTU attribute", &ett_opsi_framed_mtu, &hf_framed_mtu_att, decode_longint_attribute },
	{FRAMED_COMPRESSION_ATTRIBUTE, 	/* 13 */
	"Framed compression attribute", &ett_opsi_framed_compression, &hf_framed_compression_att, decode_value_string_attribute },
	{CALLED_STATION_ID_ATTRIBUTE,	/* 30 */
	"Called station ID attribute", &ett_opsi_called_station_id, &hf_called_station_att, decode_string_attribute },
	{CALLING_STATION_ID_ATTRIBUTE,	/* 31 */
	"Calling station ID attribute", &ett_opsi_calling_station_id, &hf_calling_station_att, decode_string_attribute },
	{NAS_IDENTIFIER,		/* 32 */
	"NAS Identifier attribute", &ett_opsi_nas_identifier, &hf_nas_identifier_att, decode_string_attribute },
	{ACCOUNTING_40_ATTRIBUTE,	/* 40 */
	"Accounting attribute", &ett_opsi_accounting, &hf_accounting_att, decode_string_attribute },
	{ACCOUNTING_41_ATTRIBUTE,	/* 41 */
	"Accounting attribute", &ett_opsi_accounting, &hf_accounting_att, decode_string_attribute },
	{ACCOUNTING_42_ATTRIBUTE,	/* 42 */
	"Accounting attribute", &ett_opsi_accounting, &hf_accounting_att, decode_string_attribute },
	{ACCOUNTING_43_ATTRIBUTE,	/* 43 */
	"Accounting attribute", &ett_opsi_accounting, &hf_accounting_att, decode_string_attribute },
	{ACCOUNTING_SESSION_ID_ATTRIBUTE,	/* 44 */ 
	"Accounting session ID attribute", &ett_opsi_acct_session_id, &hf_acct_session_id_att, decode_string_attribute },
	{ACCOUNTING_45_ATTRIBUTE,	/* 45 */
	"Accounting attribute", &ett_opsi_accounting, &hf_accounting_att, decode_string_attribute },
	{ACCOUNTING_46_ATTRIBUTE,	/* 46 */
	"Accounting attribute", &ett_opsi_accounting, &hf_accounting_att, decode_string_attribute },
	{ACCOUNTING_47_ATTRIBUTE,	/* 47 */
	"Accounting attribute", &ett_opsi_accounting, &hf_accounting_att, decode_string_attribute },
	{ACCOUNTING_48_ATTRIBUTE,	/* 48 */
	"Accounting attribute", &ett_opsi_accounting, &hf_accounting_att, decode_string_attribute },
	{ACCOUNTING_49_ATTRIBUTE,	/* 49 */
	"Accounting attribute", &ett_opsi_accounting, &hf_accounting_att, decode_string_attribute },
	{ACCOUNTING_50_ATTRIBUTE,	/* 50 */
	"Accounting attribute", &ett_opsi_accounting, &hf_accounting_att, decode_string_attribute },
	{ACCOUNTING_51_ATTRIBUTE,	/* 51 */
	"Accounting attribute", &ett_opsi_accounting, &hf_accounting_att, decode_string_attribute },
	{ACCOUNTING_52_ATTRIBUTE,	/* 52 */
	"Accounting attribute", &ett_opsi_accounting, &hf_accounting_att, decode_string_attribute },
	{ACCOUNTING_53_ATTRIBUTE,	/* 53 */
	"Accounting attribute", &ett_opsi_accounting, &hf_accounting_att, decode_string_attribute },
	{ACCOUNTING_54_ATTRIBUTE,	/* 54 */
	"Accounting attribute", &ett_opsi_accounting, &hf_accounting_att, decode_string_attribute },
	{ACCOUNTING_55_ATTRIBUTE,	/* 55 */
	"Accounting attribute", &ett_opsi_accounting, &hf_accounting_att, decode_string_attribute },
	{ACCOUNTING_56_ATTRIBUTE,	/* 56 */
	"Accounting attribute", &ett_opsi_accounting, &hf_accounting_att, decode_string_attribute },
	{ACCOUNTING_57_ATTRIBUTE,	/* 57 */
	"Accounting attribute", &ett_opsi_accounting, &hf_accounting_att, decode_string_attribute },
	{ACCOUNTING_58_ATTRIBUTE,	/* 58 */
	"Accounting attribute", &ett_opsi_accounting, &hf_accounting_att, decode_string_attribute },
	{ACCOUNTING_59_ATTRIBUTE,	/* 59 */
	"Accounting attribute", &ett_opsi_accounting, &hf_accounting_att, decode_string_attribute },
	{CHAP_CHALLENGE_ATTRIBUTE,	/* 60 */
	"CHAP challenge",	&ett_opsi_chap_challenge, &hf_chap_challenge_att, decode_string_attribute },
	{NAS_PORT_TYPE_ATTRIBUTE,	/* 61 */
	"NAS port type attribute", &ett_opsi_nas_port_type, &hf_nas_port_type_att, decode_value_string_attribute },
	{DESIGNATION_NUMBER_ATTRIBUTE,	/* 77 */
	"Designation number attribute", &ett_opsi_designation_number, &hf_designation_num_att, decode_string_attribute },
	{NAS_PORT_ID_ATTRIBUTE,		/* 87 */
	"NAS port ID attribute", &ett_opsi_nas_port_id, &hf_nas_port_id_att, decode_string_attribute },
	{SMC_AAAID_ATTRIBUTE,		/* 651 */
	"SMC AAA ID attribute", &ett_opsi_smc_aaa_id, &hf_smc_aaa_id_att, decode_longint_attribute },
	{SMC_VPNID_ATTRIBUTE,		/* 652 */
	"SMC VPN ID attribute", &ett_opsi_smc_vpn_id, &hf_smc_vpn_id_att, decode_longint_attribute },
	{SMC_VPNNAME_ATTRIBUTE,		/* 653 */
	"SMC VPN name attribute", &ett_opsi_smc_vpn_name, &hf_smc_vpn_name_att, decode_string_attribute },
	{SMC_RANID_ATTRIBUTE,           /* 654 */
	"SMC RAN ID attribute", &ett_opsi_smc_ran_id, &hf_smc_ran_id_att, decode_longint_attribute },
	{SMC_RANIP_ATTRIBUTE,           /* 655 */
	"SMC RAN IP attribute", &ett_opsi_smc_ran_ip, &hf_smc_ran_ip_att, decode_ipv4_attribute },
	{SMC_RANNAME_ATTRIBUTE,         /* 656 */
	"SMC RAN name attribute", &ett_opsi_smc_ran_name, &hf_smc_ran_name_att, decode_string_attribute },
	{SMC_POPID_ATTRIBUTE,		/* 657 */
	"SMC POP ID attribute", &ett_opsi_smc_pop_id, &hf_smc_pop_id_att, decode_longint_attribute },
	{SMC_POPNAME_ATTRIBUTE,		/* 658 */
	"SMC POP name attribute", &ett_opsi_smc_pop_name, &hf_smc_pop_name_att, decode_string_attribute },
	{SMC_SMCID_ATTRIBUTE,		/* 659 */
	"SMC ID attribute", &ett_opsi_smc_id, &hf_smc_id_att, decode_longint_attribute },
	{SMC_RECEIVE_TIME_ATTRIBUTE,	/* 660 */
	"SMC receive time attribute", &ett_opsi_smc_receive_time, &hf_smc_receive_time_att, decode_time_attribute },
	{SMC_STAT_TIME_ATTRIBUTE,	/* 661 */
	"SMC stat time attribute", &ett_opsi_smc_stat_time, &hf_smc_stat_time_att, decode_longint_attribute },
};
#define OPSI_ATTRIBUTES_COUNT (sizeof(opsi_attributes)/sizeof(opsi_attribute_handle_t))

/* Desegmentation of OPSI (over TCP) */
static gboolean opsi_desegment = TRUE;
/* To check if we must create or update the information column  */
static gboolean opsi_first;

void decode_string_attribute(tvbuff_t *tvb, proto_tree *tree, int* hfValue, int offset, int length)
{
	guint8* pbuffer;
	if (length < 4) {
		proto_tree_add_text(tree, tvb, offset, length, "Too short attribute!");
		return;
	}
	
	pbuffer=tvb_get_string(tvb, offset+4, length-4);
	proto_tree_add_string(tree, *hfValue, tvb, offset+4, length-4, pbuffer);
	g_free(pbuffer);	
}


void decode_ipv4_attribute(tvbuff_t *tvb, proto_tree *tree, int* hfValue, int offset, int length)
{
	guint32 ip_address;
	if (length < 8) {
		proto_tree_add_text(tree, tvb, offset, length, "Too short attribute!");
		return;
	}
	tvb_memcpy(tvb, (guint8 *)&ip_address, offset+4, 4);
	proto_tree_add_ipv4(tree, *hfValue, tvb, offset+4, 4, ip_address);
}

void decode_longint_attribute(tvbuff_t *tvb, proto_tree *tree, int* hfValue, int offset, int length)
{
	if (length < 8) {
		proto_tree_add_text(tree, tvb, offset, length, "Too short attribute!");
		return;
	}
	proto_tree_add_uint(tree, *hfValue, tvb, offset+4, 4, tvb_get_ntohl(tvb, offset+4));
}

void decode_value_string_attribute(tvbuff_t *tvb, proto_tree *tree, int* hfValue, int offset, int length)
{
	if (length < 8) {
		proto_tree_add_text(tree, tvb, offset, length, "Too short attribute!");
		return;
	}
	proto_tree_add_item(tree, *hfValue, tvb, offset+4, 4, FALSE);
}

void decode_time_attribute(tvbuff_t *tvb, proto_tree *tree, int* hfValue, int offset, int length)
{
	nstime_t ns;
	
	if (length < 8) {
		proto_tree_add_text(tree, tvb, offset, length, "Too short attribute!");
		return;
	}
      ns.secs  = tvb_get_ntohl(tvb, offset+4);
      ns.nsecs = 0;
      proto_tree_add_time(tree, *hfValue, tvb, offset+4, 4, &ns);
}

/****************************************************************************/
/********** End of attribute decoding ***************************************/
/****************************************************************************/

/* To find the correct size of the PDU. Needed by the desegmentation feature*/
static guint get_opsi_pdu_len(tvbuff_t *tvb, int offset)
{
  /*
   * Get the length of the OPSI packet.
   * We are guaranteed there're enough chars in tvb in order to
   * extract the length value. No exception thrown !!
   */
  return tvb_get_ntohs(tvb, offset + 4);
}

static int get_opsi_attribute_index(int min, int max, int attribute_type)
{
	int middle, at;
	
	middle = (min+max)/2;
	at = opsi_attributes[middle].attribute_type;
	if (at == attribute_type) return middle;
	if (attribute_type > at) {
		return (middle == max) ? -1 : get_opsi_attribute_index(middle+1, max, attribute_type);
	}
	return (middle == min) ? -1 : get_opsi_attribute_index(min, middle-1, attribute_type);
}


static void 
dissect_attributes(tvbuff_t *tvb, proto_tree *opsi_tree, int offset, int length)
{
	int i;
	int attribute_type;
	int attribute_length;
	proto_item *ti;
	proto_tree *ntree = NULL;
	
	while (length >= 4) {
		attribute_type 		= tvb_get_ntohs(tvb, offset);
		attribute_length 	= tvb_get_ntohs(tvb, offset+2);
		if (attribute_length > length) break;
		/* We perform a standard log(n) lookup */
		i = get_opsi_attribute_index(0, OPSI_ATTRIBUTES_COUNT-1, attribute_type);
		if (i == -1) {
			proto_tree_add_text(opsi_tree, tvb, offset, attribute_length, "Unknown attribute (%d)", attribute_type);			
		}
		else {
			ti = proto_tree_add_text(opsi_tree, tvb, offset, attribute_length, "%s (%d)", opsi_attributes[i].tree_text, attribute_type);
			ntree = proto_item_add_subtree(ti, *opsi_attributes[i].tree_id);
			proto_tree_add_text(ntree, tvb, offset+2, 2, "Length (%d)", attribute_length);
			opsi_attributes[i].dissect(tvb, ntree, opsi_attributes[i].hf_type_attribute, offset, attribute_length);
		}
		offset += attribute_length;
		length -= attribute_length;
	}
	if (length) {
		proto_tree_add_text(opsi_tree, tvb, offset, -1, "Short frame");
	}
}

static void
dissect_opsi_pdu(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	proto_item *ti;
	proto_tree *opsi_tree;
	
	if (opsi_first == TRUE) {
		opsi_first = FALSE;
		
		if (check_col(pinfo->cinfo, COL_PROTOCOL)) 
			col_set_str(pinfo->cinfo, COL_PROTOCOL, "OPSI");
    
		if (check_col(pinfo->cinfo, COL_INFO)) {
			col_clear(pinfo->cinfo, COL_INFO);
			if (tvb_length(tvb) < CODE_OFFSET+1) {
				col_set_str(pinfo->cinfo, COL_INFO, "Open Policy Service Interface");
			}
			else {
				col_add_fstr(pinfo->cinfo, COL_INFO, "Open Policy Service Interface, %s",
					val_to_str(tvb_get_guint8(tvb, CODE_OFFSET), opsi_opcode,  
				     "<Unknown opcode %d>"));
			}
		}
	}
	else if (check_col(pinfo->cinfo, COL_INFO) && (tvb_length(tvb) > CODE_OFFSET)) {
		col_append_fstr(pinfo->cinfo, COL_INFO, ", %s",
			  val_to_str(tvb_get_guint8(tvb, CODE_OFFSET), opsi_opcode, 
				     "<Unknown opcode %d>"));
	}

	if (tree) {
		ti = proto_tree_add_item(tree, proto_opsi, tvb, 0, -1, FALSE);
		opsi_tree = proto_item_add_subtree(ti, ett_opsi);
		if (tvb_length(tvb) < 8 ) {
			proto_tree_add_text(opsi_tree, tvb, 0, -1, "Too short OPSI packet!");
			return;
		}
		
		proto_tree_add_item(opsi_tree, hf_opsi_major_version, tvb, MAJOR_VERSION_OFFSET, 1, FALSE);
		proto_tree_add_item(opsi_tree, hf_opsi_minor_version, tvb, MINOR_VERSION_OFFSET, 1, FALSE);
		proto_tree_add_item(opsi_tree, hf_opsi_opcode, tvb, CODE_OFFSET, 1, FALSE);
		proto_tree_add_item(opsi_tree, hf_opsi_hook_id, tvb, HOOK_ID_OFFSET, 1, FALSE);
		proto_tree_add_item(opsi_tree, hf_opsi_length, tvb, PACKET_LENGTH_OFFSET, 2, FALSE);
		proto_tree_add_item(opsi_tree, hf_opsi_session_id, tvb, SESSION_OFFSET, 2, FALSE);
		
		dissect_attributes(tvb, opsi_tree, HEADER_LENGTH, MIN(((int)tvb_length(tvb)-HEADER_LENGTH), (tvb_get_ntohs(tvb, PACKET_LENGTH_OFFSET)-HEADER_LENGTH)));
	}
}


static void
dissect_opsi(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	opsi_first = TRUE;
	/* we need at least 6 bytes to get the payload size ! */
	tcp_dissect_pdus(tvb, pinfo, tree, opsi_desegment, 6, get_opsi_pdu_len,
		dissect_opsi_pdu);
}


void
proto_register_opsi(void)
{                 

/* Setup list of header fields  See Section 1.6.1 for details*/
	static hf_register_info hf[] = {
		{ &hf_opsi_major_version,
			{ "Major version",           "opsi.major",
			FT_UINT8, BASE_DEC, NULL, 0x0,          
			"", HFILL }
		},
		{ &hf_opsi_minor_version,
			{ "Minor version",           "opsi.minor",
			FT_UINT8, BASE_DEC, NULL, 0x0,          
			"", HFILL }
		},
		{ &hf_opsi_opcode,
		        { "Operation code",		"opsi.opcode",
		        FT_UINT8, BASE_DEC, VALS(opsi_opcode), 0x0,
		        "", HFILL }
		},
		{ &hf_opsi_hook_id,
			{ "Hook ID",			"opsi.hook",
			FT_UINT8, BASE_DEC, NULL, 0x0,
			"", HFILL }
		},
		{ &hf_opsi_length,
			{ "Message length",		"opsi.length",
			FT_UINT16, BASE_DEC, NULL, 0x0,
			"", HFILL }
		},
		{ &hf_opsi_session_id,
			{ "Session ID",			"opsi.session_id",
			FT_UINT16, BASE_DEC, NULL, 0x0,
			"", HFILL }
		},
		{ &hf_user_name_att,
			{ "User name",			"opsi.attr.user_name",
			FT_STRING, BASE_NONE, NULL, 0x00,
			"", HFILL }
		},
		{ &hf_password_att,
			{ "User password",		"opsi.attr.password",
			FT_STRING, BASE_NONE, NULL, 0x00,
			"", HFILL }
		},
		{ &hf_chap_password_att,
			{ "CHAP password attribute",	"opsi.attr.chap_password",
			FT_STRING, BASE_NONE, NULL, 0x00,
			"", HFILL }
		},
		{ &hf_nas_ip_add_att,
			{ "NAS IP address",		"opsi.attr.nas_ip_addr",
			FT_IPv4, BASE_DEC, NULL, 0x00,
			"", HFILL }
		},
		{ &hf_nas_port_att,
			{ "NAS port",			"opsi.attr.nas_port",
			FT_UINT32, BASE_HEX, NULL, 0x00,
			"", HFILL }
		},
		{ &hf_service_type_att,
		        { "Service type",		"opsi.attr.service_type",
			FT_UINT32, BASE_DEC, VALS(opsi_service_type_code), 0x0,
			"", HFILL }
		},
		{ &hf_framed_protocol_att,
			{ "Framed protocol",		"opsi.attr.framed_protocol",
			FT_UINT32, BASE_DEC, VALS(opsi_framed_protocol_code), 0x0,
			"", HFILL }
		},
		{ &hf_framed_address_att,
			{ "Framed address",		"opsi.attr.framed_address",
			FT_IPv4, BASE_DEC, NULL, 0x00,
			"", HFILL }
		},
		{ &hf_framed_netmask_att,
			{ "Framed netmask",		"opsi.attr.framed_netmask",
			FT_IPv4, BASE_DEC, NULL, 0x00,
			"", HFILL }
		},
		{ &hf_framed_routing_att,
			{ "Framed routing",		"opsi.attr.framed_routing",
			FT_UINT32, BASE_DEC, VALS(opsi_framed_routing_code), 0x0,
			"", HFILL }
		},
		{ &hf_framed_filter_att,
			{ "Framed filter",		"opsi.attr.framed_filter",
			FT_STRING, BASE_NONE, NULL, 0x00,
			"", HFILL }
		},
		{ &hf_framed_mtu_att,
			{ "Framed MTU",		"opsi.attr.framed_mtu",
			FT_UINT32, BASE_DEC, NULL, 0x00,
			"", HFILL }
		},
		{ &hf_framed_compression_att,
			{ "Framed compression",		"opsi.attr.framed_compression",
			FT_UINT32, BASE_DEC, VALS(opsi_framed_compression_code), 0x0,
			"", HFILL }
		},
		{ &hf_called_station_att,
			{ "Called station ID",		"opsi.attr.called_station_id",
			FT_STRING, BASE_NONE, NULL, 0x00,	
			"", HFILL }
		},
		{ &hf_calling_station_att,
			{ "Calling station ID",		"opsi.attr.calling_station_id",
			FT_STRING, BASE_NONE, NULL, 0x00,
			"", HFILL }
		},
		{ &hf_nas_identifier_att,
			{ "NAS ID",			"opsi.attr.nas_id",
			FT_STRING, BASE_NONE, NULL, 0x00,
			"", HFILL }
		},
		{ &hf_accounting_att,
			{ "Accounting",			"opsi.attr.accounting",
			FT_STRING, BASE_NONE, NULL, 0x00,
			"", HFILL }
		},
		{ &hf_acct_session_id_att,
			{ "Accounting session ID",	"opsi.attr.acct.session_id",
			FT_STRING, BASE_NONE, NULL, 0x00,
			"", HFILL }
		},
		{ &hf_chap_challenge_att,
			{ "CHAP challenge",		"opsi.attr.chap_challenge",
			FT_STRING, BASE_NONE, NULL, 0x00,
			"", HFILL }
		},
		{ &hf_nas_port_type_att,
			{ "NAS port type",		"opsi.attr.nas_port_type",
			FT_UINT32, BASE_DEC, VALS(opsi_nas_port_type_code), 0x0,
			"", HFILL }
		},
		{ &hf_designation_num_att,
			{ "Designation number",		"opsi.attr.designation_number",
			FT_STRING, BASE_NONE, NULL, 0x00,
			"", HFILL }
		},
		{ &hf_nas_port_id_att,
			{ "NAS port ID", 		"opsi.attr.nas_port_id",
			FT_STRING, BASE_NONE, NULL, 0x00,
			"", HFILL }
		},
		{ &hf_smc_aaa_id_att,
			{ "SMC AAA ID",			"opsi.attr.smc_aaa_id",
			FT_UINT32, BASE_DEC, NULL, 0x00,
			"", HFILL }
		},
		{ &hf_smc_vpn_id_att,	
			{ "SMC VPN ID",			"opsi.attr.smc_vpn_id",
			FT_UINT32, BASE_DEC, NULL, 0x00,
			"", HFILL }
		},
		{ &hf_smc_vpn_name_att,
			{ "SMC VPN name",		"opsi.attr.smc_vpn_name",
			FT_STRING, BASE_NONE, NULL, 0x00,
			"", HFILL }
		},
		{ &hf_smc_ran_id_att,
			{ "SMC RAN ID",			"opsi.attr.smc_ran_id",
			FT_UINT32, BASE_DEC, NULL, 0x00,
			"", HFILL }
		},
		{ &hf_smc_ran_ip_att,	
			{ "SMC RAN IP address",		"opsi.attr.smc_ran_ip",
			FT_IPv4, BASE_DEC, NULL, 0x00,
			"", HFILL }
		},
		{ &hf_smc_ran_name_att,
			{ "SMC RAN name",		"opsi.attr.smc_ran_name",
			FT_STRING, BASE_NONE, NULL, 0x00,
			"", HFILL }
		},
		{ &hf_smc_pop_id_att,
			{ "SMC POP id",			"opsi.attr.smc_pop_id",
			FT_UINT32, BASE_DEC, NULL, 0x00,
			"", HFILL }
		},
		{ &hf_smc_pop_name_att,
			{ "SMC POP name",		"opsi.attr.smc_pop_name",
			FT_STRING, BASE_NONE, NULL, 0x00,
			"", HFILL }
		},
		{ &hf_smc_id_att,        
			{ "SMC ID",			"opsi.attr.smc_id",
			FT_UINT32, BASE_DEC, NULL, 0x00,
			"", HFILL }
		},
		{ &hf_smc_receive_time_att,
			{ "SMC receive time",		"opsi.attr.smc_receive_time",
			FT_ABSOLUTE_TIME, BASE_NONE, NULL, 0x00,
			"", HFILL }
		},
		{ &hf_smc_stat_time_att,
			{ "SMC stat time",		"opsi.attr.smc_stat_time",
			FT_UINT32, BASE_DEC, NULL, 0x00,
			"", HFILL }
		},
	};

/* Setup protocol subtree array */
	static gint *ett[] = {
		&ett_opsi,
		&ett_opsi_user_name,
		&ett_opsi_user_password,
		&ett_opsi_chap_password,
		&ett_opsi_nas_ip_address,
		&ett_opsi_nas_port,
		&ett_opsi_service_type,
		&ett_opsi_framed_protocol,
		&ett_opsi_framed_address,
		&ett_opsi_framed_netmask,
		&ett_opsi_framed_routing,
		&ett_opsi_framed_filter,
		&ett_opsi_framed_mtu,
		&ett_opsi_framed_compression,
		&ett_opsi_called_station_id,
		&ett_opsi_calling_station_id,
		&ett_opsi_nas_identifier,
		&ett_opsi_accounting,
		&ett_opsi_acct_session_id,
		&ett_opsi_chap_challenge,
		&ett_opsi_nas_port_type,
		&ett_opsi_designation_number,
		&ett_opsi_nas_port_id,
		&ett_opsi_smc_aaa_id,
		&ett_opsi_smc_vpn_id,		
		&ett_opsi_smc_vpn_name,
		&ett_opsi_smc_ran_id,	
		&ett_opsi_smc_ran_ip,		
		&ett_opsi_smc_ran_name,
		&ett_opsi_smc_pop_id,	
		&ett_opsi_smc_pop_name,
		&ett_opsi_smc_id,	        
		&ett_opsi_smc_receive_time,
		&ett_opsi_smc_stat_time,
	};
	
/* For desegmentation / reassembly */
	module_t *opsi_module;	

/* Register the protocol name and description */
	proto_opsi = proto_register_protocol("Open Policy Service Interface",
	    "OPSI", "opsi");

/* Required function calls to register the header fields and subtrees used */
	proto_register_field_array(proto_opsi, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));
	
/* We activate the desegmentation / reassembly feature */	
	opsi_module = prefs_register_protocol(proto_opsi, NULL);
  	prefs_register_bool_preference(opsi_module, "desegment_opsi_messages",
    		"Desegment all OPSI messages spanning multiple TCP segments",
    		"Whether the OPSI dissector should desegment all messages spanning multiple TCP segments",
    		&opsi_desegment);
}


void
proto_reg_handoff_opsi(void)
{
	dissector_handle_t opsi_handle;
	opsi_handle = create_dissector_handle(dissect_opsi, proto_opsi);
	dissector_add("tcp.port", TCP_PORT_OPSI, opsi_handle);
}

#ifndef ENABLE_STATIC
G_MODULE_EXPORT void
plugin_init(plugin_address_table_t *pat
#ifndef PLUGINS_NEED_ADDRESS_TABLE
_U_
#endif
)
{
	/* initialise the table of pointers needed in Win32 DLLs */
	plugin_address_table_init(pat);

	/* register the new protocol, protocol fields, and subtrees */
	if (proto_opsi == -1) { /* execute protocol initialization only once */
		proto_register_opsi();
	}

}
#endif

#ifndef ENABLE_STATIC
G_MODULE_EXPORT void
plugin_reg_handoff(void)
{
	proto_reg_handoff_opsi();
}
#endif
