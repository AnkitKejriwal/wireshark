/* packet-smpp.c
 * Routines for Short Message Peer to Peer dissection
 * Copyright 2001, Tom Uijldert <tom.uijldert@cmg.nl>
 *
 * UDH and WSP dissection of SMS message, Short Message reassembly,
 * "Decode Short Message with Port Number UDH as CL-WSP" preference,
 * "Always try subdissection of 1st fragment" preference,
 * Data Coding Scheme decoding for GSM (SMS and CBS),
 * provided by Olivier Biot.
 *
 * $Id: packet-smpp.c,v 1.24 2003/12/22 02:06:27 guy Exp $
 *
 * Note on SMS Message reassembly
 * ------------------------------
 *   The current Short Message reassembly is possible thanks to the
 *   message identifier (8 or 16 bit identifier). It is able to reassemble
 *   short messages that are sent over either the same SMPP connection or
 *   distinct SMPP connections. Normally the reassembly code is able to deal
 *   with duplicate message identifiers since the fragment_add_seq_check()
 *   call is used.
 *
 *   The SMPP preference "always try subdissection of 1st fragment" allows
 *   a subdissector to be called for the first Short Message fragment,
 *   even if reassembly is not possible. This way partial dissection
 *   is still possible. This preference is switched off by default.
 *
 * Note on Short Message decoding as CL-WSP
 * ----------------------------------------
 *    The SMPP preference "port_number_udh_means_wsp" is switched off
 *    by default. If it is enabled, then any Short Message with a Port Number
 *    UDH will be decoded as CL-WSP if:
 *    -  The Short Message is not segmented
 *    -  The entire segmented Short Message is reassembled
 *    -  It is the 1st segment of an unreassembled Short Message (if the
 *       "always try subdissection of 1st fragment" preference is enabled)
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
 * ----------
 *
 * Dissector of an SMPP (Short Message Peer to Peer) PDU, as defined by the
 * SMS forum (www.smsforum.net) in "SMPP protocol specification v3.4"
 * (document version: 12-Oct-1999 Issue 1.2)
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <glib.h>

#include <epan/packet.h>

#include "prefs.h"
#include "reassemble.h"

/* Forward declarations		*/
static void dissect_smpp(tvbuff_t *, packet_info *, proto_tree *t);

/*
 * Initialize the protocol and registered fields
 *
 * Fixed header section
 */
static int proto_smpp				= -1;

static int hf_smpp_command_id			= -1;
static int hf_smpp_command_length		= -1;
static int hf_smpp_command_status		= -1;
static int hf_smpp_sequence_number		= -1;

/*
 * Fixed body section
 */
static int hf_smpp_system_id			= -1;
static int hf_smpp_password			= -1;
static int hf_smpp_system_type			= -1;
static int hf_smpp_interface_version		= -1;
static int hf_smpp_addr_ton			= -1;
static int hf_smpp_addr_npi			= -1;
static int hf_smpp_address_range		= -1;
static int hf_smpp_service_type			= -1;
static int hf_smpp_source_addr_ton		= -1;
static int hf_smpp_source_addr_npi		= -1;
static int hf_smpp_source_addr			= -1;
static int hf_smpp_dest_addr_ton		= -1;
static int hf_smpp_dest_addr_npi		= -1;
static int hf_smpp_destination_addr		= -1;
static int hf_smpp_esm_submit_msg_mode		= -1;
static int hf_smpp_esm_submit_msg_type		= -1;
static int hf_smpp_esm_submit_features		= -1;
static int hf_smpp_protocol_id			= -1;
static int hf_smpp_priority_flag		= -1;
static int hf_smpp_schedule_delivery_time	= -1;
static int hf_smpp_schedule_delivery_time_r	= -1;
static int hf_smpp_validity_period		= -1;
static int hf_smpp_validity_period_r		= -1;
static int hf_smpp_regdel_receipt		= -1;
static int hf_smpp_regdel_acks			= -1;
static int hf_smpp_regdel_notif			= -1;
static int hf_smpp_replace_if_present_flag	= -1;
static int hf_smpp_data_coding			= -1;
static int hf_smpp_sm_default_msg_id		= -1;
static int hf_smpp_sm_length			= -1;
static int hf_smpp_short_message		= -1;
static int hf_smpp_message_id			= -1;
static int hf_smpp_dlist			= -1;
static int hf_smpp_dlist_resp			= -1;
static int hf_smpp_dl_name			= -1;
static int hf_smpp_final_date			= -1;
static int hf_smpp_final_date_r			= -1;
static int hf_smpp_message_state		= -1;
static int hf_smpp_error_code			= -1;
static int hf_smpp_error_status_code		= -1;
static int hf_smpp_esme_addr_ton		= -1;
static int hf_smpp_esme_addr_npi		= -1;
static int hf_smpp_esme_addr			= -1;

/*
 * Optional parameter section
 */
static int hf_smpp_opt_param			= -1;
static int hf_smpp_vendor_op			= -1;
static int hf_smpp_reserved_op			= -1;

static int hf_smpp_dest_addr_subunit		= -1;
static int hf_smpp_dest_network_type		= -1;
static int hf_smpp_dest_bearer_type		= -1;
static int hf_smpp_dest_telematics_id		= -1;
static int hf_smpp_source_addr_subunit		= -1;
static int hf_smpp_source_network_type		= -1;
static int hf_smpp_source_bearer_type		= -1;
static int hf_smpp_source_telematics_id		= -1;
static int hf_smpp_qos_time_to_live		= -1;
static int hf_smpp_payload_type			= -1;
static int hf_smpp_additional_status_info_text	= -1;
static int hf_smpp_receipted_message_id		= -1;
static int hf_smpp_msg_wait_ind			= -1;
static int hf_smpp_msg_wait_type		= -1;
static int hf_smpp_privacy_indicator		= -1;
static int hf_smpp_source_subaddress		= -1;
static int hf_smpp_dest_subaddress		= -1;
static int hf_smpp_user_message_reference	= -1;
static int hf_smpp_user_response_code		= -1;
static int hf_smpp_source_port			= -1;
static int hf_smpp_destination_port		= -1;
static int hf_smpp_sar_msg_ref_num		= -1;
static int hf_smpp_language_indicator		= -1;
static int hf_smpp_sar_total_segments		= -1;
static int hf_smpp_sar_segment_seqnum		= -1;
static int hf_smpp_SC_interface_version		= -1;
static int hf_smpp_callback_num_pres		= -1;
static int hf_smpp_callback_num_scrn		= -1;
static int hf_smpp_callback_num_atag		= -1;
static int hf_smpp_number_of_messages		= -1;
static int hf_smpp_callback_num			= -1;
static int hf_smpp_dpf_result			= -1;
static int hf_smpp_set_dpf			= -1;
static int hf_smpp_ms_availability_status	= -1;
static int hf_smpp_network_error_type		= -1;
static int hf_smpp_network_error_code		= -1;
static int hf_smpp_message_payload		= -1;
static int hf_smpp_delivery_failure_reason	= -1;
static int hf_smpp_more_messages_to_send	= -1;
static int hf_smpp_ussd_service_op		= -1;
static int hf_smpp_display_time			= -1;
static int hf_smpp_sms_signal			= -1;
static int hf_smpp_ms_validity			= -1;
static int hf_smpp_alert_on_message_delivery	= -1;
static int hf_smpp_its_reply_type		= -1;
static int hf_smpp_its_session_number		= -1;
static int hf_smpp_its_session_sequence		= -1;
static int hf_smpp_its_session_ind		= -1;

/*
 * Data Coding Scheme section
 */
static int hf_smpp_dcs = -1;
static int hf_smpp_dcs_sms_coding_group = -1;
static int hf_smpp_dcs_text_compression = -1;
static int hf_smpp_dcs_class_present = -1;
static int hf_smpp_dcs_charset = -1;
static int hf_smpp_dcs_class = -1;
static int hf_smpp_dcs_cbs_coding_group = -1;
static int hf_smpp_dcs_cbs_language = -1;
static int hf_smpp_dcs_wap_charset = -1;
static int hf_smpp_dcs_wap_class = -1;
static int hf_smpp_dcs_cbs_class = -1;

/*
 * User Data Header section
 */
static int hf_smpp_udh_length	= -1;
static int hf_smpp_udh_iei		= -1;
static int hf_smpp_udh_multiple_messages			= -1;
static int hf_smpp_udh_multiple_messages_msg_id		= -1;
static int hf_smpp_udh_multiple_messages_msg_parts	= -1;
static int hf_smpp_udh_multiple_messages_msg_part	= -1;
static int hf_smpp_udh_ports		= -1;
static int hf_smpp_udh_ports_src	= -1;
static int hf_smpp_udh_ports_dst	= -1;

/*
 * Short Message fragment handling
 */
static int hf_sm_fragments					= -1;
static int hf_sm_fragment					= -1;
static int hf_sm_fragment_overlap			= -1;
static int hf_sm_fragment_overlap_conflicts	= -1;
static int hf_sm_fragment_multiple_tails	= -1;
static int hf_sm_fragment_too_long_fragment	= -1;
static int hf_sm_fragment_error				= -1;
static int hf_sm_reassembled_in				= -1;

/* Initialize the subtree pointers */
static gint ett_smpp		= -1;
static gint ett_dlist		= -1;
static gint ett_dlist_resp	= -1;
static gint ett_opt_param	= -1;
static gint ett_dcs			= -1;
static gint ett_udh						= -1;
static gint ett_udh_multiple_messages	= -1;
static gint ett_udh_ports				= -1;
static gint ett_sm_fragment		= -1;
static gint ett_sm_fragments	= -1;

/* Subdissector declarations */
static dissector_table_t smpp_dissector_table;

/* Short Message reassembly */
static GHashTable *sm_fragment_table = NULL;
static GHashTable *sm_reassembled_table = NULL;

static const fragment_items sm_frag_items = {
	/* Fragment subtrees */
	&ett_sm_fragment,
	&ett_sm_fragments,
	/* Fragment fields */
	&hf_sm_fragments,
	&hf_sm_fragment,
	&hf_sm_fragment_overlap,
	&hf_sm_fragment_overlap_conflicts,
	&hf_sm_fragment_multiple_tails,
	&hf_sm_fragment_too_long_fragment,
	&hf_sm_fragment_error,
	/* Reassembled in field */
	&hf_sm_reassembled_in,
	/* Tag */
	"Short Message fragments"
};

/* Dissect all SM data as WSP if the UDH contains a Port Number IE */
static gboolean port_number_udh_means_wsp = FALSE;
/* Always try dissecting the 1st fragment of a SM,
 * even if it is not reassembled */
static gboolean try_dissect_1st_frag = FALSE;

static dissector_handle_t wsp_handle;

static void
sm_defragment_init (void)
{
	fragment_table_init (&sm_fragment_table);
	reassembled_table_init(&sm_reassembled_table);
}

/*
 * Value-arrays for field-contents
 */
static const value_string vals_command_id[] = {		/* Operation	*/
    { 0x80000000, "Generic_nack" },
    { 0x00000001, "Bind_receiver" },
    { 0x80000001, "Bind_receiver - resp" },
    { 0x00000002, "Bind_transmitter" },
    { 0x80000002, "Bind_transmitter - resp" },
    { 0x00000003, "Query_sm" },
    { 0x80000003, "Query_sm - resp" },
    { 0x00000004, "Submit_sm" },
    { 0x80000004, "Submit_sm - resp" },
    { 0x00000005, "Deliver_sm" },
    { 0x80000005, "Deliver_sm - resp" },
    { 0x00000006, "Unbind" },
    { 0x80000006, "Unbind - resp" },
    { 0x00000007, "Replace_sm" },
    { 0x80000007, "Replace_sm - resp" },
    { 0x00000008, "Cancel_sm" },
    { 0x80000008, "Cancel_sm - resp" },
    { 0x00000009, "Bind_transceiver" },
    { 0x80000009, "Bind_transceiver - resp" },
    { 0x0000000B, "Outbind" },
    { 0x00000015, "Enquire_link" },
    { 0x80000015, "Enquire_link - resp" },
    { 0x00000021, "Submit_multi" },
    { 0x80000021, "Submit_multi - resp" },
    { 0x00000102, "Alert_notification" },
    { 0x00000103, "Data_sm" },
    { 0x80000103, "Data_sm - resp" },
    { 0, NULL }
};

static const value_string vals_command_status[] = {	/* Status	*/
    { 0x00000000, "Ok" },
    { 0x00000001, "Message length is invalid" },
    { 0x00000002, "Command length is invalid" },
    { 0x00000003, "Invalid command ID" },
    { 0x00000004, "Incorrect BIND status for given command" },
    { 0x00000005, "ESME already in bound state" },
    { 0x00000006, "Invalid priority flag" },
    { 0x00000007, "Invalid registered delivery flag" },
    { 0x00000008, "System error" },
    { 0x00000009, "[Reserved]" },
    { 0x0000000A, "Invalid source address" },
    { 0x0000000B, "Invalid destination address" },
    { 0x0000000C, "Message ID is invalid" },
    { 0x0000000D, "Bind failed" },
    { 0x0000000E, "Invalid password" },
    { 0x0000000F, "Invalid system ID" },
    { 0x00000010, "[Reserved]" },
    { 0x00000011, "Cancel SM failed" },
    { 0x00000012, "[Reserved]" },
    { 0x00000013, "Replace SM failed" },
    { 0x00000014, "Message queue full" },
    { 0x00000015, "Invalid service type" },
    { 0x00000033, "Invalid number of destinations" },
    { 0x00000034, "Invalid distribution list name" },
    { 0x00000040, "Destination flag is invalid (submit_multi)" },
    { 0x00000041, "[Reserved]" },
    { 0x00000042, "Invalid 'submit with replace' request" },
    { 0x00000043, "Invalid esm_class field data" },
    { 0x00000044, "Cannot submit to distribution list" },
    { 0x00000045, "submit_sm or submit_multi failed" },
    { 0x00000046, "[Reserved]" },
    { 0x00000047, "[Reserved]" },
    { 0x00000048, "Invalid source address TON" },
    { 0x00000049, "Invalid source address NPI" },
    { 0x00000050, "Invalid destination address TON" },
    { 0x00000051, "Invalid destination address NPI" },
    { 0x00000052, "[Reserved]" },
    { 0x00000053, "Invalid system_type field" },
    { 0x00000054, "Invalid replace_if_present flag" },
    { 0x00000055, "Invalid number of messages" },
    { 0x00000056, "[Reserved]" },
    { 0x00000057, "[Reserved]" },
    { 0x00000058, "Throttling error (ESME exceeded allowed message limits)" },
    { 0x00000059, "[Reserved]" },
    { 0x00000060, "[Reserved]" },
    { 0x00000061, "Invalid scheduled delivery time" },
    { 0x00000062, "Invalid message validity period (expirey time)" },
    { 0x00000063, "Predefined message invalid or not found" },
    { 0x00000064, "ESME receiver temporary app error code" },
    { 0x00000065, "ESME receiver permanent app error code" },
    { 0x00000066, "ESME receiver reject message error code" },
    { 0x00000067, "query_sm request failed" },
    { 0x000000C0, "Error in the optional part of the PDU body" },
    { 0x000000C1, "Optional parameter not allowed" },
    { 0x000000C2, "Invalid parameter length" },
    { 0x000000C3, "Expected optional parameter missing" },
    { 0x000000C4, "Invalid optional parameter  value" },
    { 0x000000FE, "Delivery failure (used for data_sm_resp)" },
    { 0x000000FF, "Unknown error" },
    { 0, NULL }
};

static const value_string vals_addr_ton[] = {
    { 0, "Unknown" },
    { 1, "International" },
    { 2, "National" },
    { 3, "Network specific" },
    { 4, "Subscriber number" },
    { 5, "Alphanumeric" },
    { 6, "Abbreviated" },
    { 0, NULL }
};

static const value_string vals_addr_npi[] = {
    {  0, "Unknown" },
    {  1, "ISDN (E163/E164)" },
    {  3, "Data (X.121)" },
    {  4, "Telex (F.69)" },
    {  6, "Land mobile (E.212)" },
    {  8, "National" },
    {  9, "Private" },
    { 10, "ERMES" },
    { 14, "Internet (IP)" },
    { 18, "WAP client Id" },
    {  0, NULL }
};

static const value_string vals_esm_submit_msg_mode[] = {
    {  0x0, "Default SMSC mode" },
    {  0x1, "Datagram mode" },
    {  0x2, "Forward mode" },
    {  0x3, "Store and forward mode" },
    {  0, NULL }
};

static const value_string vals_esm_submit_msg_type[] = {
    {  0x0, "Default message type" },
    {  0x1, "Short message contains SMSC Delivery Receipt" },
    {  0x2, "Short message contains (E)SME delivery acknowledgement" },
    {  0x3, "Reserved" },
    {  0x4, "Short message contains (E)SME manual/user acknowledgement" },
    {  0x5, "Reserved" },
    {  0x6, "Short message contains conversation abort" },
    {  0x7, "Reserved" },
    {  0x8, "Short message contains intermediate delivery notification" },
    {  0, NULL }
};

static const value_string vals_esm_submit_features[] = {
    {  0x0, "No specific features selected" },
    {  0x1, "UDHI indicator" },
    {  0x2, "Reply path" },
    {  0x3, "UDHI and reply path" },
    {  0, NULL }
};

static const value_string vals_priority_flag[] = {
    {  0, "GSM: None      ANSI-136: Bulk         IS-95: Normal" },
    {  1, "GSM: priority  ANSI-136: Normal       IS-95: Interactive" },
    {  2, "GSM: priority  ANSI-136: Urgent       IS-95: Urgent" },
    {  3, "GSM: priority  ANSI-136: Very Urgent  IS-95: Emergency" },
    {  0, NULL }
};

static const value_string vals_regdel_receipt[] = {
    {  0x0, "No SMSC delivery receipt requested" },
    {  0x1, "Delivery receipt requested (for success or failure)" },
    {  0x2, "Delivery receipt requested (for failure)" },
    {  0x3, "Reserved" },
    {  0, NULL }
};

static const value_string vals_regdel_acks[] = {
    {  0x0, "No recipient SME acknowledgement requested" },
    {  0x1, "SME delivery acknowledgement requested" },
    {  0x2, "SME manual/user acknowledgement requested" },
    {  0x3, "Both delivery and manual/user acknowledgement requested" },
    {  0, NULL }
};

static const value_string vals_regdel_notif[] = {
    {  0x0, "No intermediate notification requested" },
    {  0x1, "Intermediate notification requested" },
    {  0, NULL }
};

static const value_string vals_replace_if_present_flag[] = {
    {  0x0, "Don't replace" },
    {  0x1, "Replace" },
    {  0, NULL }
};

static const value_string vals_data_coding[] = {
    {  0, "SMSC default alphabet" },
    {  1, "IA5 (CCITT T.50/ASCII (ANSI X3.4)" },
    {  2, "Octet unspecified (8-bit binary)" },
    {  3, "Latin 1 (ISO-8859-1)" },
    {  4, "Octet unspecified (8-bit binary)" },
    {  5, "JIS (X 0208-1990)" },
    {  6, "Cyrillic (ISO-8859-5)" },
    {  7, "Latin/Hebrew (ISO-8859-8)" },
    {  8, "UCS2 (ISO/IEC-10646)" },
    {  9, "Pictogram encoding" },
    {  10, "ISO-2022-JP (Music codes)" },
    {  11, "reserved" },
    {  12, "reserved" },
    {  13, "Extended Kanji JIS(X 0212-1990)" },
    {  14, "KS C 5601" },
    /*! \todo Rest to be defined (bitmask?) according GSM 03.38	*/
    {  0, NULL }
};

static const value_string vals_message_state[] = {
    {  1, "ENROUTE" },
    {  2, "DELIVERED" },
    {  3, "EXPIRED" },
    {  4, "DELETED" },
    {  5, "UNDELIVERABLE" },
    {  6, "ACCEPTED" },
    {  7, "UNKNOWN" },
    {  8, "REJECTED" },
    {  0, NULL }
};

static const value_string vals_addr_subunit[] = {
    {  0, "Unknown -default-" },
    {  1, "MS Display" },
    {  2, "Mobile equipment" },
    {  3, "Smart card 1" },
    {  4, "External unit 1" },
    {  0, NULL }
};

static const value_string vals_network_type[] = {
    {  0, "Unknown" },
    {  1, "GSM" },
    {  2, "ANSI-136/TDMA" },
    {  3, "IS-95/CDMA" },
    {  4, "PDC" },
    {  5, "PHS" },
    {  6, "iDEN" },
    {  7, "AMPS" },
    {  8, "Paging network" },
    {  0, NULL }
};

static const value_string vals_bearer_type[] = {
    {  0, "Unknown" },
    {  1, "SMS" },
    {  2, "Circuit Switched Data (CSD)" },
    {  3, "Packet data" },
    {  4, "USSD" },
    {  5, "CDPD" },
    {  6, "DataTAC" },
    {  7, "FLEX/ReFLEX" },
    {  8, "Cell Broadcast" },
    {  0, NULL }
};

static const value_string vals_payload_type[] = {
    {  0, "Default" },
    {  1, "WCMP message" },
    {  0, NULL }
};

static const value_string vals_privacy_indicator[] = {
    {  0, "Not restricted -default-" },
    {  1, "Restricted" },
    {  2, "Confidential" },
    {  3, "Secret" },
    {  0, NULL }
};

static const value_string vals_language_indicator[] = {
    {  0, "Unspecified -default-" },
    {  1, "english" },
    {  2, "french" },
    {  3, "spanish" },
    {  4, "german" },
    {  5, "portuguese" },
    {  0, NULL }
};

static const value_string vals_display_time[] = {
    {  0, "Temporary" },
    {  1, "Default -default-" },
    {  2, "Invoke" },
    {  0, NULL }
};

static const value_string vals_ms_validity[] = {
    {  0, "Store indefinitely -default-" },
    {  1, "Power down" },
    {  2, "SID based registration area" },
    {  3, "Display only" },
    {  0, NULL }
};

static const value_string vals_dpf_result[] = {
    {  0, "DPF not set" },
    {  1, "DPF set" },
    {  0, NULL }
};

static const value_string vals_set_dpf[] = {
    {  0, "Not requested (Set DPF for delivery failure)" },
    {  1, "Requested (Set DPF for delivery failure)" },
    {  0, NULL }
};

static const value_string vals_ms_availability_status[] = {
    {  0, "Available -default-" },
    {  1, "Denied" },
    {  2, "Unavailable" },
    {  0, NULL }
};

static const value_string vals_delivery_failure_reason[] = {
    {  0, "Destination unavailable" },
    {  1, "Destination address invalid" },
    {  2, "Permanent network error" },
    {  3, "Temporary network error" },
    {  0, NULL }
};

static const value_string vals_more_messages_to_send[] = {
    {  0, "No more messages" },
    {  1, "More messages -default-" },
    {  0, NULL }
};

static const value_string vals_its_reply_type[] = {
    {  0, "Digit" },
    {  1, "Number" },
    {  2, "Telephone no." },
    {  3, "Password" },
    {  4, "Character line" },
    {  5, "Menu" },
    {  6, "Date" },
    {  7, "Time" },
    {  8, "Continue" },
    {  0, NULL }
};

static const value_string vals_ussd_service_op[] = {
    {  0, "PSSD indication" },
    {  1, "PSSR indication" },
    {  2, "USSR request" },
    {  3, "USSN request" },
    { 16, "PSSD response" },
    { 17, "PSSR response" },
    { 18, "USSR confirm" },
    { 19, "USSN confirm" },
    {  0, NULL }
};

static const value_string vals_msg_wait_ind[] = {
    {  0, "Set indication inactive" },
    {  1, "Set indication active" },
    {  0, NULL }
};

static const value_string vals_msg_wait_type[] = {
    {  0, "Voicemail message waiting" },
    {  1, "Fax message waiting" },
    {  2, "Electronic mail message waiting" },
    {  3, "Other message waiting" },
    {  0, NULL }
};

static const value_string vals_callback_num_pres[] = {
    {  0, "Presentation allowed" },
    {  1, "Presentation restricted" },
    {  2, "Number not available" },
    {  3, "[Reserved]" },
    {  0, NULL }
};

static const value_string vals_callback_num_scrn[] = {
    {  0, "User provided, not screened" },
    {  1, "User provided, verified and passed" },
    {  2, "User provided, verified and failed" },
    {  3, "Network provided" },
    {  0, NULL }
};

static const value_string vals_network_error_type[] = {
    {  1, "ANSI-136" },
    {  2, "IS-95" },
    {  3, "GSM" },
    {  4, "[Reserved]" },
    {  0, NULL }
};

static const value_string vals_its_session_ind[] = {
    {  0, "End of session indicator inactive" },
    {  1, "End of session indicator active" },
    {  0, NULL }
};

/* Data Coding Scheme: see 3GPP TS 23.040 and 3GPP TS 23.038 */
static const value_string vals_dcs_sms_coding_group[] = {
	{ 0x00, "SMS DCS: General Data Coding indication - Uncompressed text, no message class" },
	{ 0x01, "SMS DCS: General Data Coding indication - Uncompressed text" },
	{ 0x02, "SMS DCS: General Data Coding indication - Compressed text, no message class" },
	{ 0x03, "SMS DCS: General Data Coding indication - Compressed text" },
	{ 0x04, "SMS DCS: Message Marked for Automatic Deletion - Uncompressed text, no message class" },
	{ 0x05, "SMS DCS: Message Marked for Automatic Deletion - Uncompressed text" },
	{ 0x06, "SMS DCS: Message Marked for Automatic Deletion - Compressed text, no message class" },
	{ 0x07, "SMS DCS: Message Marked for Automatic Deletion - Compressed text" },
	{ 0x08, "SMS DCS: Reserved" },
	{ 0x09, "SMS DCS: Reserved" },
	{ 0x0A, "SMS DCS: Reserved" },
	{ 0x0B, "SMS DCS: Reserved" },
	{ 0x0C, "SMS DCS: Message Waiting Indication - Discard Message" },
	{ 0x0D, "SMS DCS: Message Waiting Indication - Store Message (GSM 7-bit default alphabet)" },
	{ 0x0E, "SMS DCS: Message Waiting Indication - Store Message (UCS-2 character set)" },
	{ 0x0F, "SMS DCS: Data coding / message class" },
	{ 0x00, NULL }
};

static const true_false_string tfs_dcs_text_compression = {
	"Compressed text",
	"Uncompressed text"
};

static const true_false_string tfs_dcs_class_present = {
	"Message class is present",
	"No message class"
};

static const value_string vals_dcs_charset[] = {
	{ 0x00, "GSM 7-bit default alphabet" },
	{ 0x01, "8-bit data" },
	{ 0x02, "UCS-2 (16-bit) data" },
	{ 0x03, "Reserved" },
	{ 0x00, NULL }
};

static const value_string vals_dcs_class[] = {
	{ 0x00, "Class 0" },
	{ 0x01, "Class 1 - ME specific" },
	{ 0x02, "Class 2 - (U)SIM specific" },
	{ 0x03, "Class 3 - TE specific" },
	{ 0x00, NULL }
};

static const value_string vals_dcs_cbs_coding_group[] = {
	{ 0x00, "CBS DCS: Language using the GSM 7-bit default alphabet" },
	{ 0x01, "CBS DCS: Language indication at beginning of message" },
	{ 0x02, "CBS DCS: Language using the GSM 7-bit default alphabet" },
	{ 0x03, "CBS DCS: Reserved" },
	{ 0x04, "CBS DCS: General Data Coding indication - Uncompressed text, no message class" },
	{ 0x05, "CBS DCS: General Data Coding indication - Uncompressed text" },
	{ 0x06, "CBS DCS: General Data Coding indication - Compressed text, no message class" },
	{ 0x07, "CBS DCS: General Data Coding indication - Compressed text" },
	{ 0x08, "CBS DCS: Reserved" },
	{ 0x09, "CBS DCS: Message with User Data Header structure" },
	{ 0x0A, "CBS DCS: Reserved" },
	{ 0x0B, "CBS DCS: Reserved" },
	{ 0x0C, "CBS DCS: Reserved" },
	{ 0x0D, "CBS DCS: Reserved" },
	{ 0x0E, "CBS DCS: Defined by the WAP Forum" },
	{ 0x0F, "SMS DCS: Data coding / message class" },
	{ 0x00, NULL }
};

static const value_string vals_dcs_cbs_language[] = {
	{ 0x00, "German" },
	{ 0x01, "English" },
	{ 0x02, "Italian" },
	{ 0x03, "French" },
	{ 0x04, "Spanish" },
	{ 0x05, "Dutch" },
	{ 0x06, "Swedish" },
	{ 0x07, "Danish" },
	{ 0x08, "Portuguese" },
	{ 0x09, "Finnish" },
	{ 0x0A, "Norwegian" },
	{ 0x0B, "Greek" },
	{ 0x0C, "Turkish" },
	{ 0x0D, "Hungarian" },
	{ 0x0E, "Polish" },
	{ 0x0F, "Language not specified" },
	{ 0x10, "GSM 7-bit default alphabet - message preceeded by language indication" },
	{ 0x11, "UCS-2 (16-bit) - message preceeded by language indication" },
	{ 0x20, "Czech" },
	{ 0x21, "Hebrew" },
	{ 0x22, "Arabic" },
	{ 0x23, "Russian" },
	{ 0x24, "Icelandic" },
	{ 0x00, NULL }
};

static const value_string vals_dcs_cbs_class[] = {
	{ 0x00, "No message class" },
	{ 0x01, "Class 1 - User defined" },
	{ 0x02, "Class 2 - User defined" },
	{ 0x03, "Class 3 - TE specific" },
	{ 0x00, NULL }
};

static const value_string vals_dcs_wap_class[] = {
	{ 0x00, "No message class" },
	{ 0x01, "Class 1 - ME specific" },
	{ 0x02, "Class 2 - (U)SIM specific" },
	{ 0x03, "Class 3 - TE specific" },
	{ 0x00, NULL }
};

static const value_string vals_dcs_wap_charset[] = {
	{ 0x00, "Reserved" },
	{ 0x01, "8-bit data" },
	{ 0x02, "Reserved" },
	{ 0x03, "Reserved" },
	{ 0x00, NULL }
};


/* 3GPP TS 23.040 V6.1.0 (2003-06) */
static const value_string vals_udh_iei[] = {
	{ 0x00, "SMS - Concatenated short messages, 8-bit reference number" },
	{ 0x01, "SMS - Special SMS Message Indication" },
	{ 0x02, "Reserved" },
	{ 0x03, "Value not used to avoid misinterpretation as <LF> character" },
	{ 0x04, "SMS - Application port addressing scheme, 8 bit address" },
	{ 0x05, "SMS - Application port addressing scheme, 16 bit address" },
	{ 0x06, "SMS - SMSC Control Parameters" },
	{ 0x07, "SMS - UDH Source Indicator" },
	{ 0x08, "SMS - Concatenated short message, 16-bit reference number" },
	{ 0x09, "SMS - Wireless Control Message Protocol" },
	{ 0x0A, "EMS - Text Formatting" },
	{ 0x0B, "EMS - Predefined Sound" },
	{ 0x0C, "EMS - User Defined Sound (iMelody max 128 bytes)" },
	{ 0x0D, "EMS - Predefined Animation" },
	{ 0x0E, "EMS - Large Animation (16*16 times 4 = 32*4 =128 bytes)" },
	{ 0x0F, "EMS - Small Animation (8*8 times 4 = 8*4 =32 bytes)" },
	{ 0x10, "EMS - Large Picture (32*32 = 128 bytes)" },
	{ 0x11, "EMS - Small Picture (16*16 = 32 bytes)" },
	{ 0x12, "EMS - Variable Picture" },
	{ 0x13, "EMS - User prompt indicator" },
	{ 0x14, "EMS - Extended Object" },
	{ 0x15, "EMS - Reused Extended Object" },
	{ 0x16, "EMS - Compression Control" },
	{ 0x17, "EMS - Object Distribution Indicator" },
	{ 0x18, "EMS - Standard WVG object" },
	{ 0x19, "EMS - Character Size WVG object" },
	{ 0x1A, "EMS - Extended Object Data Request Command" },
	{ 0x20, "SMS - RFC 822 E-Mail Header" },
	{ 0x21, "SMS - Hyperlink format element" },
	{ 0x22, "SMS - Reply Address Element" },
	{ 0x00, NULL }
};	

/*!
 * SMPP equivalent of mktime() (3). Convert date to standard 'time_t' format
 *
 * \param	datestr	The SMPP-formatted date to convert
 * \param	secs	Returns the 'time_t' equivalent
 * \param	nsecs	Returns the additional nano-seconds
 *
 * \return		Whether time is specified relative or absolute
 * \retval	TRUE	Relative time
 * \retval	FALSE	Absolute time
 */
static gboolean
smpp_mktime(const char *datestr, time_t *secs, int *nsecs)
{
    struct tm	 r_time;
    time_t	 t_diff;
    gboolean	 relative = FALSE;

    r_time.tm_year = 10 * (datestr[0] - '0') + (datestr[1] - '0');
    /*
     * Y2K rollover date as recommended in appendix C
     */
    if (r_time.tm_year < 38)
	r_time.tm_year += 100;
    r_time.tm_mon  = 10 * (datestr[2] - '0') + (datestr[3] - '0');
    r_time.tm_mon--;
    r_time.tm_mday = 10 * (datestr[4] - '0') + (datestr[5] - '0');
    r_time.tm_hour = 10 * (datestr[6] - '0') + (datestr[7] - '0');
    r_time.tm_min  = 10 * (datestr[8] - '0') + (datestr[9] - '0');
    r_time.tm_sec  = 10 * (datestr[10] - '0') + (datestr[11] - '0');
    r_time.tm_isdst = -1;
    *secs = mktime(&r_time);
    *nsecs = (datestr[12] - '0') * 100000000;
    t_diff = (10 * (datestr[13] - '0') + (datestr[14] - '0')) * 900;
    if (datestr[15] == '+')
	*secs += t_diff;
    else if (datestr[15] == '-')
	*secs -= t_diff;
    else				/* Must be relative ('R')	*/
	relative = TRUE;
    return relative;
}

/*!
 * Scanning routines to add standard types (byte, int, string...) to the
 * protocol tree.
 *
 * \param	tree	The protocol tree to add to
 * \param	tvb	Buffer containing the data
 * \param	field	Actual field whose value needs displaying
 * \param	offset	Location of field in buffer, returns location of
 * 			next field
 */
static void
smpp_handle_string(proto_tree *tree, tvbuff_t *tvb, int field, int *offset)
{
    guint	 len;

    len = tvb_strsize(tvb, *offset);
    if (len > 1) {
      proto_tree_add_string(tree, field, tvb, *offset, len,
          tvb_get_ptr(tvb, *offset, len));
    }
    (*offset) += len;
}

static void
smpp_handle_string_z(proto_tree *tree, tvbuff_t *tvb, int field, int *offset,
		const char *null_string)
{
    guint	 len;

    len = tvb_strsize(tvb, *offset);
    if (len > 1) {
      proto_tree_add_string(tree, field, tvb, *offset, len,
          tvb_get_ptr(tvb, *offset, len));
    } else {
		proto_tree_add_string(tree, field, tvb, *offset, len, null_string);
	}
    (*offset) += len;
}

static void
smpp_handle_int1(proto_tree *tree, tvbuff_t *tvb, int field, int *offset)
{
    guint8	 val;

    val = tvb_get_guint8(tvb, *offset);
    proto_tree_add_uint(tree, field, tvb, *offset, 1, val);
    (*offset)++;
}

static void
smpp_handle_int2(proto_tree *tree, tvbuff_t *tvb, int field, int *offset)
{
    guint	 val;

    val = tvb_get_ntohs(tvb, *offset);
    proto_tree_add_uint(tree, field, tvb, *offset, 2, val);
    (*offset) += 2;
}

static void
smpp_handle_int4(proto_tree *tree, tvbuff_t *tvb, int field, int *offset)
{
    guint	 val;

    val = tvb_get_ntohl(tvb, *offset);
    proto_tree_add_uint(tree, field, tvb, *offset, 4, val);
    (*offset) += 4;
}

static void
smpp_handle_time(proto_tree *tree, tvbuff_t *tvb,
		 int field, int field_R, int *offset)
{
    char	 *strval;
    gint	 len;
    nstime_t	 tmptime;

    strval = tvb_get_stringz(tvb, *offset, &len);
    if (*strval)
    {
	if (smpp_mktime(strval, &tmptime.secs, &tmptime.nsecs))
	    proto_tree_add_time(tree, field_R, tvb, *offset, len, &tmptime);
	else
	    proto_tree_add_time(tree, field, tvb, *offset, len, &tmptime);
    }
    g_free(strval);
    *offset += len;
}

/*!
 * Scanning routine to handle the destination-list of 'submit_multi'
 *
 * \param	tree	The protocol tree to add to
 * \param	tvb	Buffer containing the data
 * \param	offset	Location of field in buffer, returns location of
 * 			next field
 */
static void
smpp_handle_dlist(proto_tree *tree, tvbuff_t *tvb, int *offset)
{
    guint8	 entries;
    int		 tmpoff = *offset;
    proto_item	*sub_tree = NULL;
    guint8	 dest_flag;

    if ((entries = tvb_get_guint8(tvb, tmpoff++)))
    {
	sub_tree = proto_tree_add_item(tree, hf_smpp_dlist,
					tvb, *offset, 1, FALSE);
	proto_item_add_subtree(sub_tree, ett_dlist);
    }
    while (entries--)
    {
	dest_flag = tvb_get_guint8(tvb, tmpoff++);
	if (dest_flag == 1)			/* SME address	*/
	{
	    smpp_handle_int1(sub_tree, tvb, hf_smpp_dest_addr_ton, &tmpoff);
	    smpp_handle_int1(sub_tree, tvb, hf_smpp_dest_addr_npi, &tmpoff);
	    smpp_handle_string(sub_tree,tvb,hf_smpp_destination_addr,&tmpoff);
	}
	else					/* Distribution list	*/
	{
	    smpp_handle_string(sub_tree, tvb, hf_smpp_dl_name, &tmpoff);
	}
    }
    *offset = tmpoff;
}

/*!
 * Scanning routine to handle the destination result list
 * of 'submit_multi_resp'
 *
 * \param	tree	The protocol tree to add to
 * \param	tvb	Buffer containing the data
 * \param	offset	Location of field in buffer, returns location of
 * 			next field
 */
static void
smpp_handle_dlist_resp(proto_tree *tree, tvbuff_t *tvb, int *offset)
{
    guint8	 entries;
    int		 tmpoff = *offset;
    proto_item	*sub_tree = NULL;

    if ((entries = tvb_get_guint8(tvb, tmpoff++)))
    {
	sub_tree = proto_tree_add_item(tree, hf_smpp_dlist_resp,
				       tvb, *offset, 1, FALSE);
	proto_item_add_subtree(sub_tree, ett_dlist_resp);
    }
    while (entries--)
    {
	smpp_handle_int1(sub_tree, tvb, hf_smpp_dest_addr_ton, &tmpoff);
	smpp_handle_int1(sub_tree, tvb, hf_smpp_dest_addr_npi, &tmpoff);
	smpp_handle_string(sub_tree,tvb,hf_smpp_destination_addr,&tmpoff);
	smpp_handle_int4(sub_tree, tvb, hf_smpp_error_status_code, &tmpoff);
    }
    *offset = tmpoff;
}

/*!
 * Scanning routine to handle all optional parameters of SMPP-operations.
 * The parameters have the format Tag Length Value (TLV), with a 2-byte tag
 * and 2-byte length.
 *
 * \param	tree	The protocol tree to add to
 * \param	tvb	Buffer containing the data
 * \param	offset	Location of field in buffer, returns location of
 * 			next field
 */
static void
smpp_handle_tlv(proto_tree *tree, tvbuff_t *tvb, int *offset)
{
    proto_item	*sub_tree = NULL;
    guint	 tag;
    guint	 length;
    guint8	 field;
    guint8	 major, minor;
    char	 strval[BUFSIZ];

    if (tvb_reported_length_remaining(tvb, *offset) >= 4)
    {
	sub_tree = proto_tree_add_item(tree, hf_smpp_opt_param,
				       tvb, *offset, 0, FALSE);
	proto_item_add_subtree(sub_tree, ett_opt_param);
    }

    while (tvb_reported_length_remaining(tvb, *offset) >= 4)
    {
	tag = tvb_get_ntohs(tvb, *offset);
	*offset += 2;
	length = tvb_get_ntohs(tvb, *offset);
	*offset += 2;
	switch (tag) {
	    case  0x0005:	/* dest_addr_subunit	*/
		smpp_handle_int1(sub_tree, tvb,
				 hf_smpp_dest_addr_subunit, offset);
		break;
	    case  0x0006:	/* dest_network_type	*/
		smpp_handle_int1(sub_tree, tvb,
				 hf_smpp_dest_network_type, offset);
		break;
	    case  0x0007:	/* dest_bearer_type	*/
		smpp_handle_int1(sub_tree, tvb,
				 hf_smpp_dest_bearer_type, offset);
		break;
	    case  0x0008:	/* dest_telematics_id	*/
		smpp_handle_int2(sub_tree, tvb,
				 hf_smpp_dest_telematics_id, offset);
		break;
	    case  0x000D:	/* source_addr_subunit	*/
		smpp_handle_int1(sub_tree, tvb,
				 hf_smpp_source_addr_subunit, offset);
		break;
	    case  0x000E:	/* source_network_type	*/
		smpp_handle_int1(sub_tree, tvb,
				 hf_smpp_source_network_type, offset);
		break;
	    case  0x000F:	/* source_bearer_type	*/
		smpp_handle_int1(sub_tree, tvb,
				 hf_smpp_source_bearer_type, offset);
		break;
	    case  0x0010:	/* source_telematics_id	*/
		smpp_handle_int2(sub_tree, tvb,
				 hf_smpp_source_telematics_id, offset);
		break;
	    case  0x0017:	/* qos_time_to_live	*/
		smpp_handle_int4(sub_tree, tvb,
				 hf_smpp_qos_time_to_live, offset);
		break;
	    case  0x0019:	/* payload_type	*/
		smpp_handle_int1(sub_tree, tvb,
				 hf_smpp_payload_type, offset);
		break;
	    case  0x001D:	/* additional_status_info_text	*/
		smpp_handle_string(sub_tree, tvb,
				   hf_smpp_additional_status_info_text, offset);
		break;
	    case  0x001E:	/* receipted_message_id	*/
		smpp_handle_string(sub_tree, tvb,
				   hf_smpp_receipted_message_id, offset);
		break;
	    case  0x0030:	/* ms_msg_wait_facilities	*/
		field = tvb_get_guint8(tvb, *offset);
		proto_tree_add_item(sub_tree, hf_smpp_msg_wait_ind,
				    tvb, *offset, 1, field);
		proto_tree_add_item(sub_tree, hf_smpp_msg_wait_type,
				    tvb, *offset, 1, field);
		(*offset)++;
		break;
	    case  0x0201:	/* privacy_indicator	*/
		smpp_handle_int1(sub_tree, tvb,
				 hf_smpp_privacy_indicator, offset);
		break;
	    case  0x0202:	/* source_subaddress	*/
		smpp_handle_string(sub_tree, tvb,
				   hf_smpp_source_subaddress, offset);
		break;
	    case  0x0203:	/* dest_subaddress	*/
		smpp_handle_string(sub_tree, tvb,
				   hf_smpp_dest_subaddress, offset);
		break;
	    case  0x0204:	/* user_message_reference	*/
		smpp_handle_int2(sub_tree, tvb,
				 hf_smpp_user_message_reference, offset);
		break;
	    case  0x0205:	/* user_response_code	*/
		smpp_handle_int1(sub_tree, tvb,
				 hf_smpp_user_response_code, offset);
		break;
	    case  0x020A:	/* source_port	*/
		smpp_handle_int2(sub_tree, tvb,
				 hf_smpp_source_port, offset);
		break;
	    case  0x020B:	/* destination_port	*/
		smpp_handle_int2(sub_tree, tvb,
				 hf_smpp_destination_port, offset);
		break;
	    case  0x020C:	/* sar_msg_ref_num	*/
		smpp_handle_int2(sub_tree, tvb,
				 hf_smpp_sar_msg_ref_num, offset);
		break;
	    case  0x020D:	/* language_indicator	*/
		smpp_handle_int1(sub_tree, tvb,
				 hf_smpp_language_indicator, offset);
		break;
	    case  0x020E:	/* sar_total_segments	*/
		smpp_handle_int1(sub_tree, tvb,
				 hf_smpp_sar_total_segments, offset);
		break;
	    case  0x020F:	/* sar_segment_seqnum	*/
		smpp_handle_int1(sub_tree, tvb,
				 hf_smpp_sar_segment_seqnum, offset);
		break;
	    case  0x0210:	/* SC_interface_version	*/
		field = tvb_get_guint8(tvb, *offset);
		minor = field & 0x0F;
		major = (field & 0xF0) >> 4;
		sprintf(strval, "%u.%u", major, minor);
		proto_tree_add_string(sub_tree, hf_smpp_SC_interface_version,
			    	      tvb, *offset, 1, strval);
		(*offset)++;
		break;
	    case  0x0302:	/* callback_num_pres_ind	*/
		field = tvb_get_guint8(tvb, *offset);
		proto_tree_add_item(sub_tree, hf_smpp_callback_num_pres,
				    tvb, *offset, 1, field);
		proto_tree_add_item(sub_tree, hf_smpp_callback_num_scrn,
				    tvb, *offset, 1, field);
		(*offset)++;
		break;
	    case  0x0303:	/* callback_num_atag	*/
		if (length)
		    proto_tree_add_item(sub_tree, hf_smpp_callback_num_atag,
					tvb, *offset, length, FALSE);
		(*offset) += length;
		break;
	    case  0x0304:	/* number_of_messages	*/
		smpp_handle_int1(sub_tree, tvb,
				 hf_smpp_number_of_messages, offset);
		break;
	    case  0x0381:	/* callback_num	*/
		if (length)
		    proto_tree_add_item(sub_tree, hf_smpp_callback_num,
					tvb, *offset, length, FALSE);
		(*offset) += length;
		break;
	    case  0x0420:	/* dpf_result	*/
		smpp_handle_int1(sub_tree, tvb,
				 hf_smpp_dpf_result, offset);
		break;
	    case  0x0421:	/* set_dpf	*/
		smpp_handle_int1(sub_tree, tvb,
				 hf_smpp_set_dpf, offset);
		break;
	    case  0x0422:	/* ms_availability_status	*/
		smpp_handle_int1(sub_tree, tvb,
				 hf_smpp_ms_availability_status, offset);
		break;
	    case  0x0423:	/* network_error_code	*/
		smpp_handle_int1(sub_tree, tvb,
				 hf_smpp_network_error_type, offset);
		smpp_handle_int2(sub_tree, tvb,
				 hf_smpp_network_error_code, offset);
		(*offset) += length;
		break;
	    case  0x0424:	/* message_payload	*/
		if (length)
		    proto_tree_add_item(sub_tree, hf_smpp_message_payload,
					tvb, *offset, length, FALSE);
		(*offset) += length;
		break;
	    case  0x0425:	/* delivery_failure_reason	*/
		smpp_handle_int1(sub_tree, tvb,
				 hf_smpp_delivery_failure_reason, offset);
		break;
	    case  0x0426:	/* more_messages_to_send	*/
		smpp_handle_int1(sub_tree, tvb,
				 hf_smpp_more_messages_to_send, offset);
		break;
	    case  0x0427:	/* message_state	*/
		smpp_handle_int1(sub_tree, tvb,
				 hf_smpp_message_state, offset);
		break;
	    case  0x0501:	/* ussd_service_op	*/
		smpp_handle_int1(sub_tree, tvb,
				 hf_smpp_ussd_service_op, offset);
		break;
	    case  0x1201:	/* display_time	*/
		smpp_handle_int1(sub_tree, tvb,
				 hf_smpp_display_time, offset);
		break;
	    case  0x1203:	/* sms_signal	*/
		smpp_handle_int2(sub_tree, tvb,
				 hf_smpp_sms_signal, offset);
		/*! \todo Fill as per TIA/EIA-136-710-A		*/
		break;
	    case  0x1204:	/* ms_validity	*/
		smpp_handle_int1(sub_tree, tvb,
				 hf_smpp_ms_validity, offset);
		break;
	    case  0x130C:	/* alert_on_message_delivery	*/
		proto_tree_add_item(sub_tree,
			    	    hf_smpp_alert_on_message_delivery,
				    tvb, *offset, length, FALSE);
		(*offset) += length;
		break;
	    case  0x1380:	/* its_reply_type	*/
		smpp_handle_int1(sub_tree, tvb,
				 hf_smpp_its_reply_type, offset);
		break;
	    case  0x1383:	/* its_session_info	*/
		smpp_handle_int1(sub_tree, tvb,
				 hf_smpp_its_session_number, offset);
		field = tvb_get_guint8(tvb, *offset);
		proto_tree_add_item(sub_tree, hf_smpp_its_session_sequence,
				    tvb, *offset, 1, field);
		proto_tree_add_item(sub_tree, hf_smpp_its_session_ind,
				    tvb, *offset, 1, field);
		(*offset)++;
		break;
	    default:
		if ((tag >= 0x1400) && (tag <= 0x3FFF))
		    proto_tree_add_item(sub_tree, hf_smpp_vendor_op, tvb,
			    		*offset, length, FALSE);
		else
		    proto_tree_add_item(sub_tree, hf_smpp_reserved_op, tvb,
			    		*offset, length, FALSE);
		(*offset) += length;
		break;
	}
    }
}

static void
smpp_handle_dcs(proto_tree *tree, tvbuff_t *tvb, int *offset)
{
    guint8	 val;
	int off = *offset;
	proto_item *subtree = NULL;

    val = tvb_get_guint8(tvb, off);
	subtree = proto_tree_add_uint(tree,
			hf_smpp_data_coding, tvb, off, 1, val);
	proto_item_add_subtree(subtree, ett_dcs);
	/* SMPP Data Coding Scheme */
	proto_tree_add_uint(subtree, hf_smpp_dcs, tvb, off, 1, val);
	/* GSM SMS Data Coding Scheme */
	proto_tree_add_text(subtree, tvb, off, 1,
					"GSM SMS Data Coding");
	proto_tree_add_uint(subtree,
			hf_smpp_dcs_sms_coding_group, tvb, off, 1, val);
	if (val>>6 == 2) { /* Reserved */
			;
	} else if (val < 0xF0) {
		proto_tree_add_boolean(subtree,
				hf_smpp_dcs_text_compression, tvb, off, 1, val);
		proto_tree_add_boolean(subtree,
				hf_smpp_dcs_class_present, tvb, off, 1, val);
		proto_tree_add_uint(subtree,
				hf_smpp_dcs_charset, tvb, off, 1, val);
		if (val & 0x10)
			proto_tree_add_uint(subtree,
					hf_smpp_dcs_class, tvb, off, 1, val);
	} else {
		if (val & 0x08)
			proto_tree_add_text(subtree, tvb, off, 1,
					"SMPP: Bit .... 1... should be 0 (reserved)");
		proto_tree_add_uint(subtree,
				hf_smpp_dcs_charset, tvb, off, 1, val);
		proto_tree_add_uint(subtree,
				hf_smpp_dcs_class, tvb, off, 1, val);
	}
	/* Cell Broadcast Service (CBS) Data Coding Scheme */
	proto_tree_add_text(subtree, tvb, off, 1,
					"GSM CBS Data Coding");
	proto_tree_add_uint(subtree,
			hf_smpp_dcs_cbs_coding_group, tvb, off, 1, val);
	if (val < 0x40) { /* Language specified */
		proto_tree_add_uint(subtree,
				hf_smpp_dcs_cbs_language, tvb, off, 1, val);
	} else if (val>>6 == 1) { /* General Data Coding indication */
		proto_tree_add_boolean(subtree,
				hf_smpp_dcs_text_compression, tvb, off, 1, val);
		proto_tree_add_boolean(subtree,
				hf_smpp_dcs_class_present, tvb, off, 1, val);
		proto_tree_add_uint(subtree,
				hf_smpp_dcs_charset, tvb, off, 1, val);
		if (val & 0x10)
			proto_tree_add_uint(subtree,
					hf_smpp_dcs_class, tvb, off, 1, val);
	} else if (val>>6 == 2) { /* Message with UDH structure */
		proto_tree_add_uint(subtree,
				hf_smpp_dcs_charset, tvb, off, 1, val);
		proto_tree_add_uint(subtree,
				hf_smpp_dcs_class, tvb, off, 1, val);
	} else if (val>>4 == 14) { /* WAP Forum */
		proto_tree_add_uint(subtree,
				hf_smpp_dcs_wap_charset, tvb, off, 1, val);
		proto_tree_add_uint(subtree,
				hf_smpp_dcs_wap_class, tvb, off, 1, val);
	} else if (val>>4 == 15) { /* Data coding / message handling */
		if (val & 0x08)
			proto_tree_add_text(subtree, tvb, off, 1,
					"SMPP: Bit .... 1... should be 0 (reserved)");
		proto_tree_add_uint(subtree,
				hf_smpp_dcs_charset, tvb, off, 1, val);
		proto_tree_add_uint(subtree,
				hf_smpp_dcs_cbs_class, tvb, off, 1, val);
	}

    (*offset)++;
}

/*!
 * The next set of routines handle the different operations, associated
 * with SMPP.
 */
static void
bind_receiver(proto_tree *tree, tvbuff_t *tvb)
{
    int		 offset = 0;
    guint8	 field;
    guint8	 major, minor;
    char	 strval[BUFSIZ];

    smpp_handle_string(tree, tvb, hf_smpp_system_id, &offset);
    smpp_handle_string(tree, tvb, hf_smpp_password, &offset);
    smpp_handle_string(tree, tvb, hf_smpp_system_type, &offset);
    field = tvb_get_guint8(tvb, offset++);
    minor = field & 0x0F;
    major = (field & 0xF0) >> 4;
    sprintf(strval, "%u.%u", major, minor);
    proto_tree_add_string(tree, hf_smpp_interface_version, tvb,
			  offset - 1, 1, strval);
    smpp_handle_int1(tree, tvb, hf_smpp_addr_ton, &offset);
    smpp_handle_int1(tree, tvb, hf_smpp_addr_npi, &offset);
    smpp_handle_string(tree, tvb, hf_smpp_address_range, &offset);
}

#define bind_transmitter(a, b) bind_receiver(a, b)

static void
query_sm(proto_tree *tree, tvbuff_t *tvb)
{
    int		 offset = 0;

    smpp_handle_string(tree, tvb, hf_smpp_message_id, &offset);
    smpp_handle_int1(tree, tvb, hf_smpp_source_addr_ton, &offset);
    smpp_handle_int1(tree, tvb, hf_smpp_source_addr_npi, &offset);
    smpp_handle_string(tree, tvb, hf_smpp_source_addr, &offset);
}

#define bind_transceiver(a, b) bind_receiver(a, b)

static void
outbind(proto_tree *tree, tvbuff_t *tvb)
{
    int		 offset = 0;

    smpp_handle_string(tree, tvb, hf_smpp_system_id, &offset);
    smpp_handle_string(tree, tvb, hf_smpp_password, &offset);
}

/* Parse Short Message, only if UDH present
 * (otherwise this function is not called).
 * Call WSP dissector if port matches WSP traffic.
 */
static void
parse_sm_message(proto_tree *sm_tree, tvbuff_t *tvb, packet_info *pinfo)
{
	tvbuff_t *sm_tvb = NULL;
	proto_item *subtree, *tree;
	guint8 udh_len, udh, len;
	guint sm_len = tvb_reported_length (tvb);
	guint sm_data_len;
	guint32 i = 0;
	/* Multiple Messages UDH */
	gboolean is_fragmented = FALSE;
	fragment_data *fd_sm = NULL;
	guint16 sm_id = 0, frags = 0, frag = 0;
	gboolean save_fragmented = FALSE, try_sm_reassemble = FALSE;
	/* SMS Message reassembly */
	gboolean reassembled = FALSE;
	guint32 reassembled_in = 0;
	/* Port Number UDH */
	guint16 p_src = 0, p_dst = 0;
	gboolean ports_available = FALSE;

	udh_len = tvb_get_guint8(tvb, i++);
	tree = proto_tree_add_uint(sm_tree, hf_smpp_udh_length, tvb, 0, 1, udh_len);
	tree = proto_item_add_subtree(tree, ett_udh);
	while (i < udh_len) {
		udh = tvb_get_guint8(tvb, i++);
		len = tvb_get_guint8(tvb, i++);
		subtree = proto_tree_add_uint(tree, hf_smpp_udh_iei,
				tvb, i-2, 2+len, udh);
		switch (udh) {
			case 0x00: /* Multiple messages - 8-bit message ID */
				if (len == 3) {
					is_fragmented = TRUE;
					sm_id = tvb_get_guint8(tvb, i++);
					frags = tvb_get_guint8(tvb, i++);
					frag  = tvb_get_guint8(tvb, i++);
					proto_item_append_text(subtree,
							": message %u, part %u of %u", sm_id, frag, frags);
					subtree = proto_item_add_subtree(subtree,
							ett_udh_multiple_messages);
					proto_tree_add_uint (subtree,
							hf_smpp_udh_multiple_messages_msg_id,
							tvb, i-3, 1, sm_id);
					proto_tree_add_uint (subtree,
							hf_smpp_udh_multiple_messages_msg_parts,
							tvb, i-2, 1, frags);
					proto_tree_add_uint (subtree,
							hf_smpp_udh_multiple_messages_msg_part,
							tvb, i-1, 1, frag);
				} else {
					proto_item_append_text(subtree, " - Invalid format!");
					i += len;
				}
				break;

			case 0x08: /* Multiple messages - 16-bit message ID */
				if (len == 4) {
					is_fragmented = TRUE;
					sm_id = tvb_get_ntohs(tvb, i); i += 2;
					frags = tvb_get_guint8(tvb, i++);
					frag  = tvb_get_guint8(tvb, i++);
					proto_item_append_text(subtree,
							": message %u, part %u of %u", sm_id, frag, frags);
					subtree = proto_item_add_subtree(subtree,
							ett_udh_multiple_messages);
					proto_tree_add_uint (subtree,
							hf_smpp_udh_multiple_messages_msg_id,
							tvb, i-4, 2, sm_id);
					proto_tree_add_uint (subtree,
							hf_smpp_udh_multiple_messages_msg_parts,
							tvb, i-2, 1, frags);
					proto_tree_add_uint (subtree,
							hf_smpp_udh_multiple_messages_msg_part,
							tvb, i-1, 1, frag);
				} else {
					proto_item_append_text(subtree, " - Invalid format!");
					i += len;
				}
				break;

			case 0x04: /* Port Number UDH - 8-bit address */
				if (len == 2) { /* Port fields */
					p_dst = tvb_get_guint8(tvb, i++);
					p_src = tvb_get_guint8(tvb, i++);
					proto_item_append_text(subtree,
							": source port %u, destination port %u",
							p_src, p_dst);
					subtree = proto_item_add_subtree(subtree, ett_udh_ports);
					proto_tree_add_uint (subtree, hf_smpp_udh_ports_dst,
							tvb, i-2, 1, p_dst);
					proto_tree_add_uint (subtree, hf_smpp_udh_ports_src,
							tvb, i-1, 1, p_src);
					ports_available = TRUE;
				} else {
					proto_item_append_text(subtree, " - Invalid format!");
					i += len;
				}
				break;

			case 0x05: /* Port Number UDH - 16-bit address */
				if (len == 4) { /* Port fields */
					p_dst = tvb_get_ntohs(tvb, i); i += 2;
					p_src = tvb_get_ntohs(tvb, i); i += 2;
					proto_item_append_text(subtree,
							": source port %u, destination port %u",
							p_src, p_dst);
					subtree = proto_item_add_subtree(subtree, ett_udh_ports);
					proto_tree_add_uint (subtree, hf_smpp_udh_ports_dst,
							tvb, i-4, 2, p_dst);
					proto_tree_add_uint (subtree, hf_smpp_udh_ports_src,
							tvb, i-2, 2, p_src);
					ports_available = TRUE;
				} else {
					proto_item_append_text(subtree, " - Invalid format!");
					i += len;
				}
				break;

			default:
				i += len;
				break;
		}
	}
	if (tvb_reported_length_remaining(tvb, i) <= 0)
		return; /* No more data */

	/*
	 * XXX - where does the "1" come from?  If it weren't there,
	 * "sm_data_len" would, I think, be the same as
	 * "tvb_reported_length_remaining(tvb, i)".
	 *
	 * I think that the above check ensures that "sm_len" won't
	 * be less than or equal to "udh_len", so it ensures that
	 * "sm_len" won't be less than "1 + udh_len", so we don't
	 * have to worry about "sm_data_len" being negative.
	 */
	sm_data_len = sm_len - (1 + udh_len);
	if (sm_data_len == 0)
		return;	/* no more data */

	/*
	 * Try reassembling the packets.
	 * XXX - fragment numbers are 1-origin, but the fragment number
	 * field could be 0.
	 * Should we flag a fragmented message with a fragment number field
	 * of 0?
	 * What if the fragment count is 0?  Should we flag that as well?
	 */
	if ( is_fragmented && frag != 0 && frags != 0 &&
	    tvb_bytes_exist (tvb, i, sm_data_len) ) {
		try_sm_reassemble = TRUE;
		save_fragmented = pinfo->fragmented;
		pinfo->fragmented = TRUE;
		fd_sm = fragment_add_seq_check (tvb, i, pinfo,
				sm_id, /* guint32 ID for fragments belonging together */
				sm_fragment_table, /* list of message fragments */
				sm_reassembled_table, /* list of reassembled messages */
				frag-1, /* guint32 fragment sequence number */
				sm_data_len, /* guint32 fragment length */
				(frag != frags)); /* More fragments? */
		if (fd_sm) {
			reassembled = TRUE;
			reassembled_in = fd_sm->reassembled_in;
		}
		sm_tvb = process_reassembled_data(tvb, i, pinfo,
		    "Reassembled Short Message", fd_sm, &sm_frag_items,
		    NULL, sm_tree);
		if (reassembled) { /* Reassembled */
			if (check_col (pinfo->cinfo, COL_INFO))
				col_append_str (pinfo->cinfo, COL_INFO,
						" (Short Message Reassembled)");
		} else {
			/* Not last packet of reassembled Short Message */
			if (check_col (pinfo->cinfo, COL_INFO))
				col_append_fstr (pinfo->cinfo, COL_INFO,
						" (Short Message fragment %u of %u)", frag, frags);
		}
	} /* Else: not fragmented */

	if (! sm_tvb) /* One single Short Message, or not reassembled */
		sm_tvb = tvb_new_subset (tvb, i, -1, -1);
	/* Try calling a subdissector */
	if (sm_tvb) {
		if (reassembled && pinfo->fd->num != reassembled_in) {
			/* Reassembled, but not in this packet;
			 * so point to the reassembled packet */
			proto_tree_add_uint(tree, hf_sm_reassembled_in,
					tvb, 0, 0, reassembled_in);
		} else if ((reassembled /* This means that
								   pinfo->fd->num == reassembled_in */)
			|| frag==0 || (frag==1 && try_dissect_1st_frag)) {
			/* Try calling a subdissector only if:
			 *  - the Short Message is reassembled in this very packet,
			 *  - the Short Message consists of only one "fragment",
			 *  - the preference "Always Try Dissection for 1st SM fragment"
			 *    is switched on, and this is the SM's 1st fragment. */
			if ( ports_available ) {
				if ( port_number_udh_means_wsp ) {
					call_dissector (wsp_handle, sm_tvb, pinfo, sm_tree);
				} else {
					if (! dissector_try_port(smpp_dissector_table, p_src,
								sm_tvb, pinfo, sm_tree)) {
						if (! dissector_try_port(smpp_dissector_table, p_dst,
									sm_tvb, pinfo, sm_tree)) {
							if (sm_tree) { /* Only display if needed */
								proto_tree_add_text (sm_tree, sm_tvb, 0, -1,
										"Short Message body");
							}
						}
					}
				}
			} else { /* No ports IE */
				proto_tree_add_text (sm_tree, sm_tvb, 0, -1,
						"Short Message body");
			}
		} else { /* Not 1st fragment and not reassembled */
			proto_tree_add_text (sm_tree, sm_tvb, 0, -1,
					"Unreassembled Short Message fragment %u of %u",
					frag, frags);
		}
	}

	if (try_sm_reassemble) /* Clean up defragmentation */
		pinfo->fragmented = save_fragmented;
	return;
}

static void
submit_sm(proto_tree *tree, tvbuff_t *tvb, packet_info *pinfo)
{
    tvbuff_t	*tvb_msg;
    int		 offset = 0;
    guint8	 flag, udhi;
    guint8	 length;

    smpp_handle_string_z(tree, tvb, hf_smpp_service_type, &offset, "(Default)");
    smpp_handle_int1(tree, tvb, hf_smpp_source_addr_ton, &offset);
    smpp_handle_int1(tree, tvb, hf_smpp_source_addr_npi, &offset);
    smpp_handle_string(tree, tvb, hf_smpp_source_addr, &offset);
    smpp_handle_int1(tree, tvb, hf_smpp_dest_addr_ton, &offset);
    smpp_handle_int1(tree, tvb, hf_smpp_dest_addr_npi, &offset);
    smpp_handle_string(tree, tvb, hf_smpp_destination_addr, &offset);
    flag = tvb_get_guint8(tvb, offset);
    udhi = flag & 0x40;
    proto_tree_add_item(tree, hf_smpp_esm_submit_msg_mode,
			tvb, offset, 1, flag);
    proto_tree_add_item(tree, hf_smpp_esm_submit_msg_type,
			tvb, offset, 1, flag);
    proto_tree_add_item(tree, hf_smpp_esm_submit_features,
			tvb, offset, 1, flag);
    offset++;
    smpp_handle_int1(tree, tvb, hf_smpp_protocol_id, &offset);
    smpp_handle_int1(tree, tvb, hf_smpp_priority_flag, &offset);
	if (tvb_get_guint8(tvb,offset)) {
    smpp_handle_time(tree, tvb, hf_smpp_schedule_delivery_time,
				hf_smpp_schedule_delivery_time_r, &offset);
	} else { /* Time = NULL means Immediate delivery */
		proto_tree_add_text(tree, tvb, offset++, 1,
				"Scheduled delivery time: Immediate delivery");
	}
	if (tvb_get_guint8(tvb,offset)) {
    smpp_handle_time(tree, tvb, hf_smpp_validity_period,
				hf_smpp_validity_period_r, &offset);
	} else { /* Time = NULL means SMSC default validity */
		proto_tree_add_text(tree, tvb, offset++, 1,
				"Validity period: SMSC default validity period");
	}
    flag = tvb_get_guint8(tvb, offset);
    proto_tree_add_item(tree, hf_smpp_regdel_receipt, tvb, offset, 1, flag);
    proto_tree_add_item(tree, hf_smpp_regdel_acks, tvb, offset, 1, flag);
    proto_tree_add_item(tree, hf_smpp_regdel_notif, tvb, offset, 1, flag);
    offset++;
    smpp_handle_int1(tree, tvb, hf_smpp_replace_if_present_flag, &offset);
	smpp_handle_dcs(tree, tvb, &offset);
    smpp_handle_int1(tree, tvb, hf_smpp_sm_default_msg_id, &offset);
    length = tvb_get_guint8(tvb, offset);
    proto_tree_add_uint(tree, hf_smpp_sm_length, tvb, offset++, 1, length);
    if (length)
    {
	proto_tree_add_item(tree, hf_smpp_short_message,
			    tvb, offset, length, FALSE);
	if (udhi) /* UDHI indicator present */
	{
	    tvb_msg = tvb_new_subset (tvb, offset,
				      MIN(length, tvb_reported_length(tvb) - offset), length);
	    parse_sm_message(tree, tvb_msg, pinfo);
	}
    	offset += length;
    }
    smpp_handle_tlv(tree, tvb, &offset);
}

#define deliver_sm(a, b, c) submit_sm(a, b, c)

static void
replace_sm(proto_tree *tree, tvbuff_t *tvb)
{
    int		 offset = 0;
    guint8	 flag;
    guint8	 length;

    smpp_handle_string(tree, tvb, hf_smpp_message_id, &offset);
    smpp_handle_int1(tree, tvb, hf_smpp_source_addr_ton, &offset);
    smpp_handle_int1(tree, tvb, hf_smpp_source_addr_npi, &offset);
    smpp_handle_string(tree, tvb, hf_smpp_source_addr, &offset);
	if (tvb_get_guint8(tvb,offset)) {
    smpp_handle_time(tree, tvb, hf_smpp_schedule_delivery_time,
				hf_smpp_schedule_delivery_time_r, &offset);
	} else { /* Time = NULL */
		proto_tree_add_text(tree, tvb, offset++, 1,
				"Scheduled delivery time: Keep initial delivery time setting");
	}
	if (tvb_get_guint8(tvb,offset)) {
    smpp_handle_time(tree, tvb, hf_smpp_validity_period,
				hf_smpp_validity_period_r, &offset);
	} else { /* Time = NULL */
		proto_tree_add_text(tree, tvb, offset++, 1,
				"Validity period: Keep initial validity period setting");
	}
    flag = tvb_get_guint8(tvb, offset);
    proto_tree_add_item(tree, hf_smpp_regdel_receipt, tvb, offset, 1, flag);
    proto_tree_add_item(tree, hf_smpp_regdel_acks, tvb, offset, 1, flag);
    proto_tree_add_item(tree, hf_smpp_regdel_notif, tvb, offset, 1, flag);
    offset++;
    smpp_handle_int1(tree, tvb, hf_smpp_sm_default_msg_id, &offset);
    length = tvb_get_guint8(tvb, offset);
    proto_tree_add_uint(tree, hf_smpp_sm_length, tvb, offset++, 1, length);
    if (length)
	proto_tree_add_item(tree, hf_smpp_short_message,
			    tvb, offset, length, FALSE);
    offset += length;
}

static void
cancel_sm(proto_tree *tree, tvbuff_t *tvb)
{
    int		 offset = 0;

    smpp_handle_string_z(tree, tvb, hf_smpp_service_type, &offset, "(Default)");
    smpp_handle_string(tree, tvb, hf_smpp_message_id, &offset);
    smpp_handle_int1(tree, tvb, hf_smpp_source_addr_ton, &offset);
    smpp_handle_int1(tree, tvb, hf_smpp_source_addr_npi, &offset);
    smpp_handle_string(tree, tvb, hf_smpp_source_addr, &offset);
    smpp_handle_int1(tree, tvb, hf_smpp_dest_addr_ton, &offset);
    smpp_handle_int1(tree, tvb, hf_smpp_dest_addr_npi, &offset);
    smpp_handle_string(tree, tvb, hf_smpp_destination_addr, &offset);
}

static void
submit_multi(proto_tree *tree, tvbuff_t *tvb)
{
    int		 offset = 0;
    guint8	 flag;
    guint8	 length;

    smpp_handle_string_z(tree, tvb, hf_smpp_service_type, &offset, "(Default)");
    smpp_handle_int1(tree, tvb, hf_smpp_source_addr_ton, &offset);
    smpp_handle_int1(tree, tvb, hf_smpp_source_addr_npi, &offset);
    smpp_handle_string(tree, tvb, hf_smpp_source_addr, &offset);

    smpp_handle_dlist(tree, tvb, &offset);

    flag = tvb_get_guint8(tvb, offset);
    proto_tree_add_item(tree, hf_smpp_esm_submit_msg_mode,
			tvb, offset, 1, flag);
    proto_tree_add_item(tree, hf_smpp_esm_submit_msg_type,
			tvb, offset, 1, flag);
    proto_tree_add_item(tree, hf_smpp_esm_submit_features,
			tvb, offset, 1, flag);
    offset++;
    smpp_handle_int1(tree, tvb, hf_smpp_protocol_id, &offset);
    smpp_handle_int1(tree, tvb, hf_smpp_priority_flag, &offset);
	if (tvb_get_guint8(tvb,offset)) {
    smpp_handle_time(tree, tvb, hf_smpp_schedule_delivery_time,
				hf_smpp_schedule_delivery_time_r, &offset);
	} else { /* Time = NULL means Immediate delivery */
		proto_tree_add_text(tree, tvb, offset++, 1,
				"Scheduled delivery time: Immediate delivery");
	}
	if (tvb_get_guint8(tvb,offset)) {
    smpp_handle_time(tree, tvb, hf_smpp_validity_period,
				hf_smpp_validity_period_r, &offset);
	} else { /* Time = NULL means SMSC default validity */
		proto_tree_add_text(tree, tvb, offset++, 1,
				"Validity period: SMSC default validity period");
	}
    flag = tvb_get_guint8(tvb, offset);
    proto_tree_add_item(tree, hf_smpp_regdel_receipt, tvb, offset, 1, flag);
    proto_tree_add_item(tree, hf_smpp_regdel_acks, tvb, offset, 1, flag);
    proto_tree_add_item(tree, hf_smpp_regdel_notif, tvb, offset, 1, flag);
    offset++;
    smpp_handle_int1(tree, tvb, hf_smpp_replace_if_present_flag, &offset);
	smpp_handle_dcs(tree, tvb, &offset);
    smpp_handle_int1(tree, tvb, hf_smpp_sm_default_msg_id, &offset);
    length = tvb_get_guint8(tvb, offset);
    proto_tree_add_uint(tree, hf_smpp_sm_length, tvb, offset++, 1, length);
    if (length)
	proto_tree_add_item(tree, hf_smpp_short_message,
			    tvb, offset, length, FALSE);
    offset += length;
    smpp_handle_tlv(tree, tvb, &offset);
}

static void
alert_notification(proto_tree *tree, tvbuff_t *tvb)
{
    int		 offset = 0;

    smpp_handle_int1(tree, tvb, hf_smpp_source_addr_ton, &offset);
    smpp_handle_int1(tree, tvb, hf_smpp_source_addr_npi, &offset);
    smpp_handle_string(tree, tvb, hf_smpp_source_addr, &offset);
    smpp_handle_int1(tree, tvb, hf_smpp_esme_addr_ton, &offset);
    smpp_handle_int1(tree, tvb, hf_smpp_esme_addr_npi, &offset);
    smpp_handle_string(tree, tvb, hf_smpp_esme_addr, &offset);
    smpp_handle_tlv(tree, tvb, &offset);
}

static void
data_sm(proto_tree *tree, tvbuff_t *tvb)
{
    int		 offset = 0;
    guint8	 flag;

    smpp_handle_string_z(tree, tvb, hf_smpp_service_type, &offset, "(Default)");
    smpp_handle_int1(tree, tvb, hf_smpp_source_addr_ton, &offset);
    smpp_handle_int1(tree, tvb, hf_smpp_source_addr_npi, &offset);
    smpp_handle_string(tree, tvb, hf_smpp_source_addr, &offset);
    smpp_handle_int1(tree, tvb, hf_smpp_dest_addr_ton, &offset);
    smpp_handle_int1(tree, tvb, hf_smpp_dest_addr_npi, &offset);
    smpp_handle_string(tree, tvb, hf_smpp_destination_addr, &offset);
    flag = tvb_get_guint8(tvb, offset);
    proto_tree_add_item(tree, hf_smpp_esm_submit_msg_mode,
			tvb, offset, 1, flag);
    proto_tree_add_item(tree, hf_smpp_esm_submit_msg_type,
			tvb, offset, 1, flag);
    proto_tree_add_item(tree, hf_smpp_esm_submit_features,
			tvb, offset, 1, flag);
    offset++;
    flag = tvb_get_guint8(tvb, offset);
    proto_tree_add_item(tree, hf_smpp_regdel_receipt, tvb, offset, 1, flag);
    proto_tree_add_item(tree, hf_smpp_regdel_acks, tvb, offset, 1, flag);
    proto_tree_add_item(tree, hf_smpp_regdel_notif, tvb, offset, 1, flag);
    offset++;
	smpp_handle_dcs(tree, tvb, &offset);
    smpp_handle_tlv(tree, tvb, &offset);
}

/*!
 * The next set of routines handle the different operation-responses,
 * associated with SMPP.
 */
static void
bind_receiver_resp(proto_tree *tree, tvbuff_t *tvb)
{
    int		 offset = 0;

    smpp_handle_string(tree, tvb, hf_smpp_system_id, &offset);
    smpp_handle_tlv(tree, tvb, &offset);
}

#define bind_transmitter_resp(a, b) bind_receiver_resp(a, b)

static void
query_sm_resp(proto_tree *tree, tvbuff_t *tvb)
{
    int		 offset = 0;

    smpp_handle_string(tree, tvb, hf_smpp_message_id, &offset);
    smpp_handle_time(tree, tvb, hf_smpp_final_date,
				hf_smpp_final_date_r, &offset);
    smpp_handle_int1(tree, tvb, hf_smpp_message_state, &offset);
    smpp_handle_int1(tree, tvb, hf_smpp_error_code, &offset);
}

#define bind_transceiver_resp(a, b) bind_receiver_resp(a, b)

static void
submit_sm_resp(proto_tree *tree, tvbuff_t *tvb)
{
    int		 offset = 0;

    smpp_handle_string(tree, tvb, hf_smpp_message_id, &offset);
}

#define deliver_sm_resp(a, b) submit_sm_resp(a, b)

static void
submit_multi_resp(proto_tree *tree, tvbuff_t *tvb)
{
    int		 offset = 0;

    smpp_handle_string(tree, tvb, hf_smpp_message_id, &offset);
    smpp_handle_dlist_resp(tree, tvb, &offset);
}

static void
data_sm_resp(proto_tree *tree, tvbuff_t *tvb)
{
    int		 offset = 0;

    smpp_handle_string(tree, tvb, hf_smpp_message_id, &offset);
    smpp_handle_tlv(tree, tvb, &offset);
}

/*
 * A 'heuristic dissector' that attemtps to establish whether we have
 * a genuine SMPP PDU here.
 * Only works when:
 *	at least the fixed header is there
 *	it has a correct overall PDU length
 *	it is a 'well-known' operation
 *	has a 'well-known' status
 */
static gboolean
dissect_smpp_heur(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
    guint	 command_id;		/* SMPP command		*/
    guint	 command_status;	/* Status code		*/
    guint	 command_length;	/* length of PDU	*/

    if (tvb_reported_length(tvb) < 4 * 4)	/* Mandatory header	*/
	return FALSE;
    command_length = tvb_get_ntohl(tvb, 0);
    if (command_length > 64 * 1024)
	return FALSE;
    command_id = tvb_get_ntohl(tvb, 4);		/* Only known commands	*/
    if (match_strval(command_id, vals_command_id) == NULL)
	return FALSE;
    command_status = tvb_get_ntohl(tvb, 8);	/* ..with known status	*/
    if (match_strval(command_status, vals_command_status) == NULL)
	return FALSE;
    dissect_smpp(tvb, pinfo, tree);
    return TRUE;
}

/* Code to actually dissect the packets */
static void
dissect_smpp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
    int		 offset = 0;		/* Offset within tvbuff	*/
    guint	 command_length;	/* length of PDU	*/
    guint	 command_id;		/* SMPP command		*/
    guint	 command_status;	/* Status code		*/
    guint	 sequence_number;	/* ...of command	*/

    /* Set up structures needed to add the protocol subtree and manage it */
    proto_item	*ti;
    proto_tree	*smpp_tree;
    tvbuff_t	*tmp_tvb;

    /*
     * Safety: don't even try it when the mandatory header isn't present.
     */
    if (tvb_reported_length(tvb) < 4 * 4)
	return;
    command_length = tvb_get_ntohl(tvb, offset);
    offset += 4;
    command_id = tvb_get_ntohl(tvb, offset);
    offset += 4;
    command_status = tvb_get_ntohl(tvb, offset);
    offset +=4;
    sequence_number = tvb_get_ntohl(tvb, offset);
    offset += 4;
    /* Make entries in Protocol column and Info column on summary display */
    if (check_col(pinfo->cinfo, COL_PROTOCOL))
	col_set_str(pinfo->cinfo, COL_PROTOCOL, "SMPP");

    if (check_col(pinfo->cinfo, COL_INFO))
    {
	col_clear(pinfo->cinfo, COL_INFO);
	col_add_fstr(pinfo->cinfo, COL_INFO, "SMPP %s",
		 val_to_str(command_id,vals_command_id,"unknown operation"));
	if (command_id & 0x80000000)
	    col_append_fstr(pinfo->cinfo, COL_INFO, ": \"%s\"",
			    val_to_str(command_status, vals_command_status,
				       "reserved error"));
	if (command_length > tvb_reported_length(tvb))
	    col_append_str(pinfo->cinfo, COL_INFO, " [short packet]");
	if (command_length < tvb_reported_length(tvb))
	    col_append_str(pinfo->cinfo, COL_INFO, " [trailing data]");
    }

    /* In the interest of speed, if "tree" is NULL, don't do any work not
     * necessary to generate protocol tree items.
	 *
	 * Exception: sm_submit (command_id == 0x00000004) - for SMS reassembly
     */
    if (tree || (command_id == 4)) {

	/* create display subtree for the protocol	*/
	ti = proto_tree_add_item(tree, proto_smpp, tvb, 0,
				 command_length, FALSE);
	smpp_tree = proto_item_add_subtree(ti, ett_smpp);

	/* add an item to the subtree			*/
	proto_tree_add_uint(smpp_tree, hf_smpp_command_length, tvb,
			    0, 4, command_length);
	proto_tree_add_uint(smpp_tree, hf_smpp_command_id, tvb,
			    4, 4, command_id);
	proto_item_append_text (ti, ", %s",
			match_strval (command_id, vals_command_id));
	/* Status is only meaningful with responses	*/
	if (command_id & 0x80000000) {
	    proto_tree_add_uint(smpp_tree, hf_smpp_command_status, tvb,
				8, 4, command_status);
		proto_item_append_text (ti, ": \"%s\"",
				match_strval (command_status, vals_command_status));
	}
	proto_tree_add_uint(smpp_tree, hf_smpp_sequence_number, tvb,
			    12, 4, sequence_number);
	proto_item_append_text (ti, ", Seq: %u, Len: %u",
			sequence_number, command_length);
	/*
	 * End of header. Don't dissect variable part if it is shortened.
	 */
	if (command_length > tvb_reported_length(tvb))
	    return;
	tmp_tvb = tvb_new_subset(tvb, offset, -1, command_length - offset);
	if (command_id & 0x80000000)
	{
	    switch (command_id & 0x7FFFFFFF) {
		/*
		 * All of these only have a fixed header
		 */
		case 0:				/* Generic nack	*/
		case 6:				/* Unbind resp	*/
		case 7:				/* Replace SM resp	*/
		case 8:				/* Cancel SM resp	*/
		case 21:			/* Enquire link resp	*/
		    break;
		case 1:
		    if (!command_status)
			bind_receiver_resp(smpp_tree, tmp_tvb);
		    break;
		case 2:
		    if (!command_status)
			bind_transmitter_resp(smpp_tree, tmp_tvb);
		    break;
		case 3:
		    if (!command_status)
			query_sm_resp(smpp_tree, tmp_tvb);
		    break;
		case 4:
		    if (!command_status)
			submit_sm_resp(smpp_tree, tmp_tvb);
		    break;
		case 5:
		    if (!command_status)
			deliver_sm_resp(smpp_tree, tmp_tvb);
		    break;
		case 9:
		    if (!command_status)
			bind_transceiver_resp(smpp_tree, tmp_tvb);
		    break;
		case 33:
		    if (!command_status)
			submit_multi_resp(smpp_tree, tmp_tvb);
		    break;
		case 259:
		    if (!command_status)
			data_sm_resp(smpp_tree, tmp_tvb);
		    break;
		default:
		    break;
	    }
	}
	else
	{
	    switch (command_id) {
		case  1:
		    bind_receiver(smpp_tree, tmp_tvb);
		    break;
		case  2:
		    bind_transmitter(smpp_tree, tmp_tvb);
		    break;
		case  3:
		    query_sm(smpp_tree, tmp_tvb);
		    break;
		case  4:
		    submit_sm(smpp_tree, tmp_tvb, pinfo);
		    break;
		case  5:
		    deliver_sm(smpp_tree, tmp_tvb, pinfo);
		    break;
		case  6:			/* Unbind	*/
		case 21:			/* Enquire link	*/
		    break;
		case  7:
		    replace_sm(smpp_tree, tmp_tvb);
		    break;
		case  8:
		    cancel_sm(smpp_tree, tmp_tvb);
		    break;
		case  9:
		    bind_transceiver(smpp_tree, tmp_tvb);
		    break;
		case  11:
		    outbind(smpp_tree, tmp_tvb);
		    break;
		case  33:
		    submit_multi(smpp_tree, tmp_tvb);
		    break;
		case  258:
		    alert_notification(smpp_tree, tmp_tvb);
		    break;
		case  259:
		    data_sm(smpp_tree, tmp_tvb);
		    break;
		default:
		    break;
	    }
	}
    }
    /* If this protocol has a sub-dissector call it here.	*/
    return;
}

/* Register the protocol with Ethereal */
void
proto_register_smpp(void)
{
	module_t *smpp_module; /* Preferences for SMPP */

    /* Setup list of header fields	*/
    static hf_register_info hf[] = {
	{   &hf_smpp_command_length,
	    {   "Length    ", "smpp.command_length",
		FT_UINT32, BASE_DEC, NULL, 0x00,
		"Total length of the SMPP PDU.",
		HFILL
	    }
	},
	{   &hf_smpp_command_id,
	    {   "Operation ", "smpp.command_id",
		FT_UINT32, BASE_HEX, VALS(vals_command_id), 0x00,
		"Defines the SMPP PDU.",
		HFILL
	    }
	},
	{   &hf_smpp_command_status,
	    {   "Result    ", "smpp.command_status",
		FT_UINT32, BASE_HEX, VALS(vals_command_status), 0x00,
		"Indicates success or failure of the SMPP request.",
		HFILL
	    }
	},
	{   &hf_smpp_sequence_number,
	    {   "Sequence #", "smpp.sequence_number",
		FT_UINT32, BASE_DEC, NULL, 0x00,
		"A number to correlate requests with responses.",
		HFILL
	    }
	},
	{   &hf_smpp_system_id,
	    {   "System ID", "smpp.system_id",
		FT_STRING, BASE_NONE, NULL, 0x00,
		"Identifies a system.",
		HFILL
	    }
	},
	{   &hf_smpp_password,
	    {   "Password", "smpp.password",
		FT_STRING, BASE_NONE, NULL, 0x00,
		"Password used for authentication.",
		HFILL
	    }
	},
	{   &hf_smpp_system_type,
	    {   "System type", "smpp.system_type",
		FT_STRING, BASE_NONE, NULL, 0x00,
		"Categorises the system.",
		HFILL
	    }
	},
	{   &hf_smpp_interface_version,
	    {   "Version (if)", "smpp.interface_version",
		FT_STRING, BASE_NONE, NULL, 0x00,
		"Version of SMPP interface supported.",
		HFILL
	    }
	},
	{   &hf_smpp_service_type,
	    {   "Service type", "smpp.service_type",
		FT_STRING, BASE_NONE, NULL, 0x00,
		"SMS application service associated with the message.",
		HFILL
	    }
	},
	{   &hf_smpp_addr_ton,
	    {   "Type of number", "smpp.addr_ton",
		FT_UINT8, BASE_HEX, VALS(vals_addr_ton), 0x00,
		"Indicates the type of number, given in the address.",
		HFILL
	    }
	},
	{   &hf_smpp_source_addr_ton,
	    {   "Type of number (originator)", "smpp.source_addr_ton",
		FT_UINT8, BASE_HEX, VALS(vals_addr_ton), 0x00,
		"Indicates originator type of number, given in the address.",
		HFILL
	    }
	},
	{   &hf_smpp_dest_addr_ton,
	    {   "Type of number (recipient)", "smpp.dest_addr_ton",
		FT_UINT8, BASE_HEX, VALS(vals_addr_ton), 0x00,
		"Indicates recipient type of number, given in the address.",
		HFILL
	    }
	},
	{   &hf_smpp_addr_npi,
	    {   "Numbering plan indicator", "smpp.addr_npi",
		FT_UINT8, BASE_HEX, VALS(vals_addr_npi), 0x00,
		"Gives the numbering plan this address belongs to.",
		HFILL
	    }
	},
	{   &hf_smpp_source_addr_npi,
	    {   "Numbering plan indicator (originator)", "smpp.source_addr_npi",
		FT_UINT8, BASE_HEX, VALS(vals_addr_npi), 0x00,
		"Gives originator numbering plan this address belongs to.",
		HFILL
	    }
	},
	{   &hf_smpp_dest_addr_npi,
	    {   "Numbering plan indicator (recipient)", "smpp.dest_addr_npi",
		FT_UINT8, BASE_HEX, VALS(vals_addr_npi), 0x00,
		"Gives recipient numbering plan this address belongs to.",
		HFILL
	    }
	},
	{   &hf_smpp_address_range,
	    {   "Address", "smpp.address_range",
		FT_STRING, BASE_NONE, NULL, 0x00,
		"Given address or address range.",
		HFILL
	    }
	},
	{   &hf_smpp_source_addr,
	    {   "Originator address", "smpp.source_addr",
		FT_STRING, BASE_NONE, NULL, 0x00,
		"Address of SME originating this message.",
		HFILL
	    }
	},
	{   &hf_smpp_destination_addr,
	    {   "Recipient address", "smpp.destination_addr",
		FT_STRING, BASE_NONE, NULL, 0x00,
		"Address of SME receiving this message.",
		HFILL
	    }
	},
	{   &hf_smpp_esm_submit_msg_mode,
	    {   "Messaging mode", "smpp.esm.submit.msg_mode",
		FT_UINT8, BASE_HEX, VALS(vals_esm_submit_msg_mode), 0x03,
		"Mode attribute for this message.",
		HFILL
	    }
	},
	{   &hf_smpp_esm_submit_msg_type,
	    {   "Message type  ", "smpp.esm.submit.msg_type",
		FT_UINT8, BASE_HEX, VALS(vals_esm_submit_msg_type), 0x3C,
		"Type attribute for this message.",
		HFILL
	    }
	},
	{   &hf_smpp_esm_submit_features,
	    {   "GSM features  ", "smpp.esm.submit.features",
		FT_UINT8, BASE_HEX, VALS(vals_esm_submit_features), 0xC0,
		"GSM network specific features.",
		HFILL
	    }
	},
	/*! \todo Get proper values from GSM-spec.	*/
	{   &hf_smpp_protocol_id,
	    {   "Protocol id.", "smpp.protocol_id",
		FT_UINT8, BASE_HEX, NULL, 0x00,
		"Protocol identifier according GSM 03.40.",
		HFILL
	    }
	},
	{   &hf_smpp_priority_flag,
	    {   "Priority level", "smpp.priority_flag",
		FT_UINT8, BASE_HEX, VALS(vals_priority_flag), 0x00,
		"The priority level of the short message.",
		HFILL
	    }
	},
	{   &hf_smpp_schedule_delivery_time,
	    {   "Scheduled delivery time", "smpp.schedule_delivery_time",
		FT_ABSOLUTE_TIME, BASE_NONE, NULL, 0x00,
		"Scheduled time for delivery of short message.",
		HFILL
	    }
	},
	{   &hf_smpp_schedule_delivery_time_r,
	    {   "Scheduled delivery time", "smpp.schedule_delivery_time_r",
		FT_RELATIVE_TIME, BASE_NONE, NULL, 0x00,
		"Scheduled time for delivery of short message.",
		HFILL
	    }
	},
	{   &hf_smpp_validity_period,
	    {   "Validity period", "smpp.validity_period",
		FT_ABSOLUTE_TIME, BASE_NONE, NULL, 0x00,
		"Validity period of this message.",
		HFILL
	    }
	},
	{   &hf_smpp_validity_period_r,
	    {   "Validity period", "smpp.validity_period_r",
		FT_RELATIVE_TIME, BASE_NONE, NULL, 0x00,
		"Validity period of this message.",
		HFILL
	    }
	},
	{   &hf_smpp_regdel_receipt,
	    {   "Delivery receipt  ", "smpp.regdel.receipt",
		FT_UINT8, BASE_HEX, VALS(vals_regdel_receipt), 0x03,
		"SMSC delivery receipt request.",
		HFILL
	    }
	},
	{   &hf_smpp_regdel_acks,
	    {   "Message type      ", "smpp.regdel.acks",
		FT_UINT8, BASE_HEX, VALS(vals_regdel_acks), 0x0C,
		"SME acknowledgement request.",
		HFILL
	    }
	},
	{   &hf_smpp_regdel_notif,
	    {   "Intermediate notif", "smpp.regdel.notif",
		FT_UINT8, BASE_HEX, VALS(vals_regdel_notif), 0x10,
		"Intermediate notification request.",
		HFILL
	    }
	},
	{   &hf_smpp_replace_if_present_flag,
	    {   "Replace           ", "smpp.replace_if_present_flag",
		FT_UINT8, BASE_HEX, VALS(vals_replace_if_present_flag), 0x01,
		"Replace the short message with this one or not.",
		HFILL
	    }
	},
	{   &hf_smpp_data_coding,
	    {   "Data coding", "smpp.data_coding",
		FT_UINT8, BASE_HEX, NULL, 0x00,
		"Defines the encoding scheme of the message.",
		HFILL
	    }
	},
	{   &hf_smpp_sm_default_msg_id,
	    {   "Predefined message", "smpp.sm_default_msg_id",
		FT_UINT8, BASE_DEC, NULL, 0x00,
		"Index of a predefined ('canned') short message.",
		HFILL
	    }
	},
	{   &hf_smpp_sm_length,
	    {   "Message length", "smpp.sm_length",
		FT_UINT8, BASE_DEC, NULL, 0x00,
		"Length of the message content.",
		HFILL
	    }
	},
	{   &hf_smpp_short_message,
	    {   "Message", "smpp.message",
		FT_NONE, BASE_NONE, NULL, 0x00,
		"The actual message or data.",
		HFILL
	    }
	},
	{   &hf_smpp_message_id,
	    {   "Message id.", "smpp.message_id",
		FT_STRING, BASE_NONE, NULL, 0x00,
		"Identifier of the submitted short message.",
		HFILL
	    }
	},
	{   &hf_smpp_dlist,
	    {   "Destination list", "smpp.dlist",
		FT_NONE, BASE_NONE, NULL, 0x00,
		"The list of destinations for a short message.",
		HFILL
	    }
	},
	{   &hf_smpp_dlist_resp,
	    {   "Unsuccesfull delivery list", "smpp.dlist_resp",
		FT_NONE, BASE_NONE, NULL, 0x00,
		"The list of unsuccesfull deliveries to destinations.",
		HFILL
	    }
	},
	{   &hf_smpp_dl_name,
	    {   "Distr. list name", "smpp.dl_name",
		FT_STRING, BASE_NONE, NULL, 0x00,
		"The name of the distribution list.",
		HFILL
	    }
	},
	{   &hf_smpp_final_date,
	    {   "Final date", "smpp.final_date",
		FT_ABSOLUTE_TIME, BASE_NONE, NULL, 0x00,
		"Date-time when the queried message reached a final state.",
		HFILL
	    }
	},
	{   &hf_smpp_final_date_r,
	    {   "Final date", "smpp.final_date_r",
		FT_RELATIVE_TIME, BASE_NONE, NULL, 0x00,
		"Date-time when the queried message reached a final state.",
		HFILL
	    }
	},
	{   &hf_smpp_message_state,
	    {   "Message state", "smpp.message_state",
		FT_UINT8, BASE_DEC, VALS(vals_message_state), 0x00,
		"Specifies the status of the queried short message.",
		HFILL
	    }
	},
	{   &hf_smpp_error_code,
	    {   "Error code", "smpp.error_code",
		FT_UINT8, BASE_DEC, NULL, 0x00,
		"Network specific error code defining reason for failure.",
		HFILL
	    }
	},
	{   &hf_smpp_error_status_code,
	    {   "Status", "smpp.error_status_code",
		FT_UINT32, BASE_HEX, VALS(vals_command_status), 0x00,
		"Indicates success/failure of request for this address.",
		HFILL
	    }
	},
	{   &hf_smpp_esme_addr_ton,
	    {   "Type of number (ESME)", "smpp.esme_addr_ton",
		FT_UINT8, BASE_HEX, VALS(vals_addr_ton), 0x00,
		"Indicates recipient type of number, given in the address.",
		HFILL
	    }
	},
	{   &hf_smpp_esme_addr_npi,
	    {   "Numbering plan indicator (ESME)", "smpp.esme_addr_npi",
		FT_UINT8, BASE_HEX, VALS(vals_addr_npi), 0x00,
		"Gives the numbering plan this address belongs to.",
		HFILL
	    }
	},
	{   &hf_smpp_esme_addr,
	    {   "ESME address", "smpp.esme_addr",
		FT_STRING, BASE_NONE, NULL, 0x00,
		"Address of ESME originating this message.",
		HFILL
	    }
	},
	{   &hf_smpp_dest_addr_subunit,
	    {   "Subunit destination", "smpp.dest_addr_subunit",
		FT_UINT8, BASE_HEX, VALS(vals_addr_subunit), 0x00,
		"Subunit address within mobile to route message to.",
		HFILL
	    }
	},
	{   &hf_smpp_source_addr_subunit,
	    {   "Subunit origin", "smpp.source_addr_subunit",
		FT_UINT8, BASE_HEX, VALS(vals_addr_subunit), 0x00,
		"Subunit address within mobile that generated the message.",
		HFILL
	    }
	},
	{   &hf_smpp_dest_network_type,
	    {   "Destination network", "smpp.dest_network_type",
		FT_UINT8, BASE_HEX, VALS(vals_network_type), 0x00,
		"Network associated with the destination address.",
		HFILL
	    }
	},
	{   &hf_smpp_source_network_type,
	    {   "Originator network", "smpp.source_network_type",
		FT_UINT8, BASE_HEX, VALS(vals_network_type), 0x00,
		"Network associated with the originator address.",
		HFILL
	    }
	},
	{   &hf_smpp_dest_bearer_type,
	    {   "Destination bearer", "smpp.dest_bearer_type",
		FT_UINT8, BASE_HEX, VALS(vals_bearer_type), 0x00,
		"Desired bearer for delivery of message.",
		HFILL
	    }
	},
	{   &hf_smpp_source_bearer_type,
	    {   "Originator bearer", "smpp.source_bearer_type",
		FT_UINT8, BASE_HEX, VALS(vals_bearer_type), 0x00,
		"Bearer over which the message originated.",
		HFILL
	    }
	},
	{   &hf_smpp_dest_telematics_id,
	    {   "Telematic interworking (dest)", "smpp.dest_telematics_id",
		FT_UINT16, BASE_HEX, NULL, 0x00,
		"Telematic interworking to be used for message delivery.",
		HFILL
	    }
	},
	{   &hf_smpp_source_telematics_id,
	    {   "Telematic interworking (orig)", "smpp.source_telematics_id",
		FT_UINT16, BASE_HEX, NULL, 0x00,
		"Telematic interworking used for message submission.",
		HFILL
	    }
	},
	{   &hf_smpp_qos_time_to_live,
	    {   "Validity period", "smpp.qos_time_to_live",
		FT_UINT32, BASE_DEC, NULL, 0x00,
		"Number of seconds to retain message before expiry.",
		HFILL
	    }
	},
	{   &hf_smpp_payload_type,
	    {   "Payload", "smpp.payload_type",
		FT_UINT8, BASE_DEC, VALS(vals_payload_type), 0x00,
		"PDU type contained in the message payload.",
		HFILL
	    }
	},
	{   &hf_smpp_additional_status_info_text,
	    {   "Information", "smpp.additional_status_info_text",
		FT_STRING, BASE_NONE, NULL, 0x00,
		"Description of the meaning of a response PDU.",
		HFILL
	    }
	},
	{   &hf_smpp_receipted_message_id,
	    {   "SMSC identifier", "smpp.receipted_message_id",
		FT_STRING, BASE_NONE, NULL, 0x00,
		"SMSC handle of the message being received.",
		HFILL
	    }
	},
	{   &hf_smpp_privacy_indicator,
	    {   "Privacy indicator", "smpp.privacy_indicator",
		FT_UINT8, BASE_DEC, VALS(vals_privacy_indicator), 0x00,
		"Indicates the privacy level of the message.",
		HFILL
	    }
	},
	{   &hf_smpp_user_message_reference,
	    {   "Message reference", "smpp.user_message_reference",
		FT_UINT16, BASE_HEX, NULL, 0x00,
		"Reference to the message, assigned by the user.",
		HFILL
	    }
	},
	{   &hf_smpp_user_response_code,
	    {   "Application response code", "smpp.user_response_code",
		FT_UINT8, BASE_HEX, NULL, 0x00,
		"A response code set by the user.",
		HFILL
	    }
	},
	{   &hf_smpp_language_indicator,
	    {   "Language", "smpp.language_indicator",
		FT_UINT8, BASE_DEC, VALS(vals_language_indicator), 0x00,
		"Indicates the language of the short message.",
		HFILL
	    }
	},
	{   &hf_smpp_source_port,
	    {   "Source port", "smpp.source_port",
		FT_UINT16, BASE_HEX, NULL, 0x00,
		"Application port associated with the source of the message.",
		HFILL
	    }
	},
	{   &hf_smpp_destination_port,
	    {   "Destination port", "smpp.destination_port",
		FT_UINT16, BASE_HEX, NULL, 0x00,
		"Application port associated with the destination of the message.",
		HFILL
	    }
	},
	{   &hf_smpp_sar_msg_ref_num,
	    {   "SAR reference number", "smpp.sar_msg_ref_num",
		FT_UINT16, BASE_DEC, NULL, 0x00,
		"Reference number for a concatenated short message.",
		HFILL
	    }
	},
	{   &hf_smpp_sar_total_segments,
	    {   "SAR size", "smpp.sar_total_segments",
		FT_UINT16, BASE_DEC, NULL, 0x00,
		"Number of segments of a concatenated short message.",
		HFILL
	    }
	},
	{   &hf_smpp_sar_segment_seqnum,
	    {   "SAR sequence number", "smpp.sar_segment_seqnum",
		FT_UINT8, BASE_DEC, NULL, 0x00,
		"Segment number within a concatenated short message.",
		HFILL
	    }
	},
	{   &hf_smpp_display_time,
	    {   "Display time", "smpp.display_time",
		FT_UINT8, BASE_DEC, VALS(vals_display_time), 0x00,
		"Associates a display time with the message on the handset.",
		HFILL
	    }
	},
	{   &hf_smpp_ms_validity,
	    {   "Validity info", "smpp.ms_validity",
		FT_UINT8, BASE_DEC, VALS(vals_ms_validity), 0x00,
		"Associates validity info with the message on the handset.",
		HFILL
	    }
	},
	{   &hf_smpp_dpf_result,
	    {   "Delivery pending set?", "smpp.dpf_result",
		FT_UINT8, BASE_DEC, VALS(vals_dpf_result), 0x00,
		"Indicates whether Delivery Pending Flag was set.",
		HFILL
	    }
	},
	{   &hf_smpp_set_dpf,
	    {   "Request DPF set", "smpp.set_dpf",
		FT_UINT8, BASE_DEC, VALS(vals_set_dpf), 0x00,
		"Request to set the DPF for certain failure scenario's.",
		HFILL
	    }
	},
	{   &hf_smpp_ms_availability_status,
	    {   "Availability status", "smpp.ms_availability_status",
		FT_UINT8, BASE_DEC, VALS(vals_ms_availability_status), 0x00,
		"Indicates the availability state of the handset.",
		HFILL
	    }
	},
	{   &hf_smpp_delivery_failure_reason,
	    {   "Delivery failure reason", "smpp.delivery_failure_reason",
		FT_UINT8, BASE_DEC, VALS(vals_delivery_failure_reason), 0x00,
		"Indicates the reason for a failed delivery attempt.",
		HFILL
	    }
	},
	{   &hf_smpp_more_messages_to_send,
	    {   "More messages?", "smpp.more_messages_to_send",
		FT_UINT8, BASE_DEC, VALS(vals_more_messages_to_send), 0x00,
		"Indicates more messages pending for the same destination.",
		HFILL
	    }
	},
	{   &hf_smpp_number_of_messages,
	    {   "Number of messages", "smpp.number_of_messages",
		FT_UINT8, BASE_DEC, NULL, 0x00,
		"Indicates number of messages stored in a mailbox.",
		HFILL
	    }
	},
	{   &hf_smpp_its_reply_type,
	    {   "Reply method", "smpp.its_reply_type",
		FT_UINT8, BASE_DEC, VALS(vals_its_reply_type), 0x00,
		"Indicates the handset reply method on message receipt.",
		HFILL
	    }
	},
	{   &hf_smpp_ussd_service_op,
	    {   "USSD service operation", "smpp.ussd_service_op",
		FT_UINT8, BASE_DEC, VALS(vals_ussd_service_op), 0x00,
		"Indicates the USSD service operation.",
		HFILL
	    }
	},
	{   &hf_smpp_vendor_op,
	    {   "Optional parameter - Vendor-specific", "smpp.vendor_op",
		FT_NONE, BASE_NONE, NULL, 0x00,
		"A supplied optional parameter specific to an SMSC-vendor.",
		HFILL
	    }
	},
	{   &hf_smpp_reserved_op,
	    {   "Optional parameter - Reserved", "smpp.reserved_op",
		FT_NONE, BASE_NONE, NULL, 0x00,
		"An optional parameter that is reserved in this version.",
		HFILL
	    }
	},
	{   &hf_smpp_msg_wait_ind,
	    {   "Indication", "smpp.msg_wait.ind",
		FT_UINT8, BASE_HEX, VALS(vals_msg_wait_ind), 0x80,
		"Indicates to the handset that a message is waiting.",
		HFILL
	    }
	},
	{   &hf_smpp_msg_wait_type,
	    {   "Type      ", "smpp.msg_wait.type",
		FT_UINT8, BASE_HEX, VALS(vals_msg_wait_type), 0x03,
		"Indicates type of message that is waiting.",
		HFILL
	    }
	},
	{   &hf_smpp_SC_interface_version,
	    {   "SMSC-supported version", "smpp.SC_interface_version",
		FT_STRING, BASE_NONE, NULL, 0x00,
		"Version of SMPP interface supported by the SMSC.",
		HFILL
	    }
	},
	{   &hf_smpp_callback_num_pres,
	    {   "Presentation", "smpp.callback_num.pres",
		FT_UINT8, BASE_HEX, VALS(vals_callback_num_pres), 0x0C,
		"Controls the presentation indication.",
		HFILL
	    }
	},
	{   &hf_smpp_callback_num_scrn,
	    {   "Screening   ", "smpp.callback_num.scrn",
		FT_UINT8, BASE_HEX, VALS(vals_callback_num_scrn), 0x03,
		"Controls screening of the callback-number.",
		HFILL
	    }
	},
	{   &hf_smpp_callback_num_atag,
	    {   "Callback number - alphanumeric display tag",
		"smpp.callback_num_atag",
		FT_NONE, BASE_NONE, NULL, 0x00,
		"Associates an alphanumeric display with call back number.",
		HFILL
	    }
	},
	{   &hf_smpp_callback_num,
	    {   "Callback number", "smpp.callback_num",
		FT_NONE, BASE_NONE, NULL, 0x00,
		"Associates a call back number with the message.",
		HFILL
	    }
	},
	{   &hf_smpp_network_error_type,
	    {   "Error type", "smpp.network_error.type",
		FT_UINT8, BASE_DEC, VALS(vals_network_error_type), 0x00,
		"Indicates the network type.",
		HFILL
	    }
	},
	{   &hf_smpp_network_error_code,
	    {   "Error code", "smpp.network_error.code",
		FT_UINT16, BASE_HEX, NULL, 0x00,
		"Gives the actual network error code.",
		HFILL
	    }
	},
	{   &hf_smpp_message_payload,
	    {   "Payload", "smpp.message_payload",
		FT_NONE, BASE_NONE, NULL, 0x00,
		"Short message user data.",
		HFILL
	    }
	},
	{   &hf_smpp_alert_on_message_delivery,
	    {   "Alert on delivery", "smpp.alert_on_message_delivery",
		FT_NONE, BASE_NONE, NULL, 0x00,
		"Instructs the handset to alert user on message delivery.",
		HFILL
	    }
	},
	{   &hf_smpp_its_session_number,
	    {   "Session number", "smpp.its_session.number",
		FT_UINT8, BASE_DEC, NULL, 0x00,
		"Session number of interactive teleservice.",
		HFILL
	    }
	},
	{   &hf_smpp_its_session_sequence,
	    {   "Sequence number  ", "smpp.its_session.sequence",
		FT_UINT8, BASE_HEX, NULL, 0xFE,
		"Sequence number of the dialogue unit.",
		HFILL
	    }
	},
	{   &hf_smpp_its_session_ind,
	    {   "Session indicator", "smpp.its_session.ind",
		FT_UINT8, BASE_HEX, VALS(vals_its_session_ind), 0x01,
		"Indicates whether this message is end of conversation.",
		HFILL
	    }
	},
	{   &hf_smpp_opt_param,
	    {   "Optional parameters", "smpp.opt_param",
		FT_NONE, BASE_NONE, NULL, 0x00,
		"The list of optional parameters in this operation.",
		HFILL
	    }
	},
	{	&hf_smpp_dcs,
		{ "SMPP Data Coding Scheme", "smpp.dcs",
		FT_UINT8, BASE_HEX, VALS(vals_data_coding), 0x00,
		"Data Coding Scheme according to SMPP.",
		HFILL
	    }
	},
	{	&hf_smpp_dcs_sms_coding_group,
		{	"DCS Coding Group for SMS", "smpp.dcs.sms_coding_group",
			FT_UINT8, BASE_HEX, VALS(vals_dcs_sms_coding_group), 0xF0,
			"Data Coding Scheme coding group for GSM Short Message Service.",
			HFILL
		}
	},
	{	&hf_smpp_dcs_text_compression,
		{	"DCS Text compression", "smpp.dcs.text_compression",
			FT_BOOLEAN, 8, TFS(&tfs_dcs_text_compression), 0x20,
			"Indicates if text compression is used.", HFILL
		}
	},
	{	&hf_smpp_dcs_class_present,
		{	"DCS Class present", "smpp.dcs.class_present",
			FT_BOOLEAN, 8, TFS(&tfs_dcs_class_present), 0x10,
			"Indicates if the message class is present (defined).", HFILL
		}
	},
	{	&hf_smpp_dcs_charset,
		{	"DCS Character set", "smpp.dcs.charset",
			FT_UINT8, BASE_HEX, VALS(vals_dcs_charset), 0x0C,
			"Specifies the character set used in the message.", HFILL
		}
	},
	{	&hf_smpp_dcs_class,
		{	"DCS Message class", "smpp.dcs.class",
			FT_UINT8, BASE_HEX, VALS(vals_dcs_class), 0x03,
			"Specifies the message class.", HFILL
		}
	},
	{	&hf_smpp_dcs_cbs_coding_group,
		{	"DCS Coding Group for CBS", "smpp.dcs.cbs_coding_group",
			FT_UINT8, BASE_HEX, VALS(vals_dcs_cbs_coding_group), 0xF0,
			"Data Coding Scheme coding group for GSM Cell Broadcast Service.",
			HFILL
		}
	},
	{	&hf_smpp_dcs_cbs_language,
		{	"DCS CBS Message language", "smpp.dcs.cbs_language",
			FT_UINT8, BASE_HEX, VALS(vals_dcs_cbs_language), 0x3F,
			"Language of the GSM Cell Broadcast Service message.", HFILL
		}
	},
	{	&hf_smpp_dcs_cbs_class,
		{	"DCS CBS Message class", "smpp.dcs.cbs_class",
			FT_UINT8, BASE_HEX, VALS(vals_dcs_cbs_class), 0x03,
			"Specifies the message class for GSM Cell Broadcast Service, "
			"for the Data coding / message handling code group.", HFILL
		}
	},
	{	&hf_smpp_dcs_wap_charset,
		{	"DCS Message coding", "smpp.dcs.wap_coding",
			FT_UINT8, BASE_HEX, VALS(vals_dcs_wap_charset), 0x0C,
			"Specifies the used message encoding, "
			"as specified by the WAP Forum (WAP over GSM USSD).", HFILL
		}
	},
	{	&hf_smpp_dcs_wap_class,
		{	"DCS CBS Message class", "smpp.dcs.wap_class",
			FT_UINT8, BASE_HEX, VALS(vals_dcs_wap_class), 0x03,
			"Specifies the message class for GSM Cell Broadcast Service, "
			"as specified by the WAP Forum (WAP over GSM USSD).", HFILL
		}
	},
	{   &hf_smpp_udh_iei,
		{	"IE Id", "smpp.udh.iei",
		FT_UINT8, BASE_HEX, VALS(vals_udh_iei), 0x00,
		"Name of the User Data Header Information Element.",
		HFILL
		}
	},
	{   &hf_smpp_udh_length,
	    {	"UDH Length", "smpp.udh.len",
		FT_UINT8, BASE_DEC, NULL, 0x00,
		"Length of the User Data Header (bytes)",
		HFILL
	    }
	},
	{   &hf_smpp_udh_multiple_messages,
	    {	"Multiple messages UDH", "smpp.udh.mm",
		FT_NONE, BASE_NONE, NULL, 0x00,
		"Multiple messages User Data Header",
		HFILL
	    }
	},
	{   &hf_smpp_udh_multiple_messages_msg_id,
	    {	"Message identifier", "smpp.udh.mm.msg_id",
		FT_UINT16, BASE_DEC, NULL, 0x00,
		"Identification of the message",
		HFILL
	    }
	},
	{   &hf_smpp_udh_multiple_messages_msg_parts,
	    {	"Message parts", "smpp.udh.mm.msg_parts",
		FT_UINT8, BASE_DEC, NULL, 0x00,
		"Total number of message parts (fragments)",
		HFILL
	    }
	},
	{   &hf_smpp_udh_multiple_messages_msg_part,
	    {	"Message part number", "smpp.udh.mm.msg_part",
		FT_UINT8, BASE_DEC, NULL, 0x00,
		"Message part (fragment) sequence number",
		HFILL
	    }
	},
	{   &hf_smpp_udh_ports,
	    {	"Port number UDH", "smpp.udh.ports",
		FT_NONE, BASE_NONE, NULL, 0x00,
		"Port number User Data Header",
		HFILL
	    }
	},
	{   &hf_smpp_udh_ports_src,
	    {	"Source port", "smpp.udh.ports.src",
		FT_UINT8, BASE_DEC, NULL, 0x00,
		"Source port",
		HFILL
	    }
	},
	{   &hf_smpp_udh_ports_dst,
	    {	"Destination port", "smpp.udh.ports.dst",
		FT_UINT8, BASE_DEC, NULL, 0x00,
		"Destination port",
		HFILL
	    }
	},
	/* Short Message fragment reassembly */
	{	&hf_sm_fragments,
		{	"Short Message fragments", "smpp.fragments",
			FT_NONE, BASE_NONE, NULL, 0x00,
			"SMPP Short Message fragments",
			HFILL
		}
	},
	{	&hf_sm_fragment,
		{	"Short Message fragment", "smpp.fragment",
			FT_FRAMENUM, BASE_NONE, NULL, 0x00,
			"SMPP Short Message fragment",
			HFILL
		}
	},
	{	&hf_sm_fragment_overlap,
		{	"Short Message fragment overlap", "smpp.fragment.overlap",
			FT_BOOLEAN, BASE_NONE, NULL, 0x00,
			"SMPP Short Message fragment overlaps with other fragment(s)",
			HFILL
		}
	},
	{	&hf_sm_fragment_overlap_conflicts,
		{	"Short Message fragment overlapping with conflicting data",
			"smpp.fragment.overlap.conflicts",
			FT_BOOLEAN, BASE_NONE, NULL, 0x00,
			"SMPP Short Message fragment overlaps with conflicting data",
			HFILL
		}
	},
	{	&hf_sm_fragment_multiple_tails,
		{	"Short Message has multiple tail fragments",
			"smpp.fragment.multiple_tails",
			FT_BOOLEAN, BASE_NONE, NULL, 0x00,
			"SMPP Short Message fragment has multiple tail fragments",
			HFILL
		}
	},
	{	&hf_sm_fragment_too_long_fragment,
		{	"Short Message fragment too long",
			"smpp.fragment.too_long_fragment",
			FT_BOOLEAN, BASE_NONE, NULL, 0x00,
			"SMPP Short Message fragment data goes beyond the packet end",
			HFILL
		}
	},
	{	&hf_sm_fragment_error,
		{	"Short Message defragmentation error", "smpp.fragment.error",
			FT_FRAMENUM, BASE_NONE, NULL, 0x00,
			"SMPP Short Message defragmentation error due to illegal fragments",
			HFILL
		}
	},
	{	&hf_sm_reassembled_in,
		{	"Reassembled in",
			"smpp.reassembled.in",
			FT_FRAMENUM, BASE_NONE, NULL, 0x00,
			"SMPP Short Message has been reassembled in this packet.", HFILL
		}
	},
    };
    /* Setup protocol subtree array */
    static gint *ett[] = {
	    &ett_smpp,
	    &ett_dlist,
	    &ett_dlist_resp,
	    &ett_opt_param,
		&ett_dcs,
		&ett_udh,
	    &ett_udh_multiple_messages,
	    &ett_udh_ports,
		&ett_sm_fragment,
		&ett_sm_fragments,
    };
    /* Register the protocol name and description */
    proto_smpp = proto_register_protocol("Short Message Peer to Peer",
    					 "SMPP", "smpp");

    /* Required function calls to register header fields and subtrees used */
    proto_register_field_array(proto_smpp, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));

    /* Subdissector code */
    smpp_dissector_table = register_dissector_table("smpp.udh.port",
			"SMPP UDH port", FT_UINT16, BASE_DEC);

	/* Preferences for SMPP */
	smpp_module = prefs_register_protocol (proto_smpp, NULL);
	prefs_register_bool_preference (smpp_module, "port_number_udh_means_wsp",
			"Port Number IE in UDH always triggers CL-WSP dissection",
			"Always decode a Short Message as Connectionless WSP "
			"if a Port Number Information Element is present "
			"in the User Data Header.",
			&port_number_udh_means_wsp);
	prefs_register_bool_preference (smpp_module, "try_dissect_1st_fragment",
			"Always try subdissection of 1st Short Message fragment",
			"Always try subdissection of the 1st fragment of a fragmented "
			"Short Message. If reassembly is possible, the Short Message "
			"may be dissected twice (once as a short frame, once in its "
			"entirety).",
			&try_dissect_1st_frag);

	/* SMPP dissector initialization routines */
	register_init_routine (sm_defragment_init);
}

/*
 * If dissector uses sub-dissector registration add a registration routine.
 * This format is required because a script is used to find these routines and
 * create the code that calls these routines.
 */
void
proto_reg_handoff_smpp(void)
{
    dissector_handle_t smpp_handle;

    /*
     * SMPP can be spoken on any port under TCP or X.25
     * ...how *do* we do that under X.25?
     *
     * We can register the heuristic SMPP dissector with X.25, for one
     * thing.  We don't currently have any mechanism to allow the user
     * to specify that a given X.25 circuit is to be dissected as SMPP,
     * however.
     */
    smpp_handle = create_dissector_handle(dissect_smpp, proto_smpp);
    dissector_add_handle("tcp.port", smpp_handle);
    heur_dissector_add("tcp", dissect_smpp_heur, proto_smpp);
    heur_dissector_add("x.25", dissect_smpp_heur, proto_smpp);

	/* Required for call_dissector() */
	wsp_handle = find_dissector ("wsp-cl");
	assert (wsp_handle);
}
