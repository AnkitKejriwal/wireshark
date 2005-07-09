/* packet-uma.c
 * Routines for Unlicensed Mobile Access(UMA) dissection
 * Copyright 2005, Anders Broman <anders.broman[at]ericsson.com>
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
 *
 * References:
 * http://www.umatechnology.org/ 
 * UMA Protocols (Stage 3) R1.0.3 (2005-02-26)
 * 
 * http://www.3gpp.org/specs/numbering.htm 
 * 3GPP TS 24.008 V6.2.0 (2003-09)
 * Technical Specification
 * 3rd Generation Partnership Project;
 * Technical Specification Group Core Network;
 * Mobile radio interface Layer 3 specification;
 * Core network protocols; Stage 3
 * (Release 6)
 *
 * 3GPP TS 44.018 V6.11.0 (2005-01)
 * 3rd Generation Partnership Project;
 * Technical Specification Group GSM/EDGE Radio Access Network;
 * Mobile radio interface layer 3 specification;
 * Radio Resource Control (RRC) protocol
 * (Release 6)
 *
 * 3GPP TS 45.009 V6.1.0 (2004-02)
 * 3rd Generation Partnership Project;
 * Technical Specification Group GSM/EDGE
 * Radio Access Network;
 * Link adaptation
 * (Release 6)
 *
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <epan/conversation.h>

#ifdef NEED_SNPRINTF_H
# include "snprintf.h"
#endif

#include <epan/packet.h>
#include "prefs.h"
#include "packet-bssap.h"
#include "packet-gsm_a.h"
#include "packet-rtp.h"
#include "packet-rtcp.h"

static dissector_handle_t uma_tcp_handle = NULL;
static dissector_handle_t uma_udp_handle = NULL;
static dissector_handle_t data_handle = NULL;
static dissector_table_t bssap_pdu_type_table=NULL;
static dissector_handle_t rtp_handle = NULL;
static dissector_handle_t rtcp_handle = NULL;
static dissector_handle_t llc_handle = NULL;

/* Initialize the protocol and registered fields */
static int proto_uma					= -1;
static int hf_uma_length_indicator		= -1;
static int hf_uma_pd					= -1;
static int hf_uma_skip_ind				= -1;
static int hf_uma_urr_msg_type			= -1;
static int hf_uma_urlc_msg_type			= -1;
static int hf_uma_urlc_TLLI				= -1;
static int hf_uma_urlc_seq_nr			= -1;
static int hf_uma_urr_IE				= -1;
static int hf_uma_urr_IE_len			= -1;
static int hf_uma_urr_IE_len2			= -1;
static int hf_uma_urr_mobile_identity_type	= -1; 
static int hf_uma_urr_odde_even_ind		= -1;
static int hf_uma_urr_imsi				= -1;
static int hf_uma_urr_imei				= -1;
static int hf_uma_urr_imeisv			= -1;
static int hf_uma_urr_tmsi_p_tmsi		= -1;
static int hf_uma_urr_uri				= -1;
static int hf_uma_urr_radio_type_of_id	= -1;
static int hf_uma_urr_radio_id			= -1;
static int hf_uma_urr_cell_id			= -1;
static int hf_uma_urr_mcc				= -1;
static int hf_uma_urr_mnc				= -1;
static int hf_uma_urr_lac				= -1;
static int hf_uma_urr_gci				= -1;
static int hf_uma_urr_tura				= -1;
static int hf_uma_urr_tlra				= -1;
static int hf_uma_urr_rrs				= -1;
static int hf_uma_urr_location_estimate = -1;
static int hf_uma_urr_IP_Address_type	= -1;
static int hf_uma_urr_FQDN				= -1;
static int hf_uma_urr_sgw_ipv4			= -1;
static int hf_uma_urr_redirection_counter = -1;
static int hf_uma_urr_dis_rej_cau		= -1;
static int hf_uma_urr_MSCR				= -1;
static int hf_uma_urr_ATT				= -1;
static int hf_uma_urr_DTM				= -1;
static int hf_uma_urr_GPRS				= -1;
static int hf_uma_urr_NMO				= -1;
static int hf_uma_urr_ECMC				= -1;
static int hf_uma_urr_EC				= -1;
static int hf_uma_urr_T3212_timer		= -1;
static int hf_uma_urr_RAC				= -1;
static int hf_uma_urr_ap_location		= -1;
static int hf_uma_urr_SGSNR				= -1;
static int hf_uma_urr_ECMP				= -1;
static int hf_uma_urr_RE				= -1;
static int hf_uma_urr_PFCFM				= -1;
static int hf_uma_urr_3GECS				= -1;
static int hf_uma_urr_cell_id_disc		= -1;
static int hf_uma_urr_RNC_ID			= -1;
static int hf_uma_urr_ci				= -1;
static int hf_uma_urr_bcc				= -1;
static int hf_uma_urr_ncc				= -1;
static int hf_uma_urr_bcch_arfcn		= -1;
static int hf_uma_urr_TU3907_timer		= -1;
static int hf_uma_urr_GSM_RR_state		= -1;
static int hf_uma_urr_UMA_band			= -1;
static int hf_uma_urr_URR_state			= -1;
static int hf_uma_urr_register_reject_cause = -1;
static int hf_uma_urr_TU3906_timer		= -1;
static int hf_uma_urr_TU3910_timer		= -1;
static int hf_uma_urr_TU3902_timer		= -1;
static int hf_uma_urr_communication_port = -1;
static int hf_uma_urr_L3_Message		= -1;
static int hf_uma_urr_L3_protocol_discriminator = -1;
static int hf_uma_urr_channel_mode		= -1;
static int hf_uma_urr_MSC2_rev			= -1;
static int hf_uma_urr_ES_IND			= -1;
static int hf_uma_urr_A5_1_algorithm_sup = -1;
static int hf_uma_urr_RF_power_capability = -1;
static int hf_uma_urr_ps_sup_cap		= -1;
static int hf_uma_urr_SS_screening_indicator = -1;
static int hf_uma_urr_SM_capability		 = -1;
static int hf_uma_urr_VBS_notification_rec = -1;
static int hf_uma_urr_VGCS_notification_rec = -1;
static int hf_uma_urr_FC_frequency_cap	= -1;
static int hf_uma_urr_CM3				= -1;
static int hf_uma_urr_LCS_VA_cap		= -1;
static int hf_uma_urr_UCS2_treatment	= -1;
static int hf_uma_urr_SoLSA				= -1;
static int hf_uma_urr_CMSP				= -1;
static int hf_uma_urr_A5_3_algorithm_sup= -1;
static int hf_uma_urr_A5_2_algorithm_sup = -1;
static int hf_uma_urr_RR_cause			= -1;
static int hf_uma_urr_sc				= -1;
static int hf_uma_urr_algorithm_id		= -1;
static int hf_uma_urr_GPRS_resumption	= -1;
static int hf_uma_urr_ULQI				= -1;
static int hf_uma_urr_TLLI				= -1;
static int hf_uma_urr_packet_flow_id	= -1;
static int hf_uma_urr_suspension_cause	= -1;
static int hf_uma_urr_TU3920_timer		= -1;
static int hf_uma_urr_rate				= -1;
static int hf_uma_urr_precedence		= -1;
static int hf_uma_urr_a_bit				= -1;
static int hf_uma_urr_t_bit				= -1;
static int hf_uma_urr_c_r_bit			= -1;
static int hf_uma_urr_URLCcause			= -1;
static int hf_uma_urr_udr				= -1;
static int hf_uma_urr_TU4001_timer		= -1;
static int hf_uma_urr_LS				= -1;
static int hf_uma_urr_cipher_res		= -1;
static int hf_uma_urr_rand_val			= -1;
static int hf_uma_urr_ciphering_command_mac = -1;
static int hf_uma_urr_ciphering_key_seq_num = -1;
static int hf_uma_urr_sapi_id			= -1;
static int hf_uma_urr_establishment_cause = -1;
static int hf_uma_urr_channel			= -1;
static int hf_uma_urr_PDU_in_error		= -1;
static int hf_uma_urr_sample_size		= -1;
static int hf_uma_urr_payload_type		= -1;
static int hf_uma_urr_multirate_speech_ver = -1;
static int hf_uma_urr_NCSB				= -1;
static int hf_uma_urr_ICMI				= -1;
static int hf_uma_urr_start_mode		= -1;
static int hf_uma_urr_LLC_PDU			= -1;
static int hf_uma_urr_LBLI				= -1;
static int hf_uma_urr_RI				= -1;
static int hf_uma_urr_TU4003_timer		= -1;
static int hf_uma_urr_ap_service_name_type = -1;
static int hf_uma_urr_ap_Service_name_value = -1;
static int hf_uma_urr_uma_service_zone_icon_ind = -1;
static int hf_uma_urr_uma_service_zone_str_len = -1;
static int hf_uma_urr_window_size	= -1;
static int hf_uma_urr_uma_codec_mode	= -1;
static int hf_uma_urr_UTRAN_cell_id_disc = -1;
static int hf_uma_urr_ms_radio_id		= -1;
static int hf_uma_urr_uma_service_zone_str = -1;
static int hf_uma_urr_suti				= -1;
static int hf_uma_urr_uma_mps			= -1;
static int hf_uma_urr_num_of_plms		= -1;
static int hf_uma_urr_unc_ipv4			= -1;
static int hf_uma_unc_FQDN				= -1;
static int hf_uma_urr_GPRS_user_data_transport_ipv4 = -1;
static int hf_uma_urr_GPRS_port			= -1;
static int hf_uma_urr_UNC_tcp_port		= -1;
static int hf_uma_urr_RTP_port			= -1;
static int hf_uma_urr_RTCP_port			= -1;

/* Initialize the subtree pointers */
static int ett_uma = -1;
static int ett_uma_toc = -1;
static int ett_urr_ie = -1; 

/* The dynamic payload type which will be dissected as uma */

static guint gbl_umaTcpPort1 = 14001;

/* Global variables */
	guint32		sgw_ipv4_address;
	guint32		unc_ipv4_address;
	guint32		rtp_ipv4_address;
	guint32		rtcp_ipv4_address;
	guint32		GPRS_user_data_ipv4_address;

/* 
 * Protocol Discriminator (PD)
 */
static const value_string uma_pd_vals[] = {
	{ 1,		"URR"},
	{ 2,		"URLC"}, 
	{ 0,	NULL }
};
/* 
 * Message types for Unlicensed Radio Resources management
 */
static const value_string uma_urr_msg_type_vals[] = {
	{ 1,		"URR DISCOVERY REQUEST"},
	{ 2,		"URR DISCOVERY ACCEPT"}, 
	{ 3,		"URR DISCOVERY REJECT"}, 
	{ 16,		"URR REGISTER REQUEST"},
	{ 17,		"URR REGISTER ACCEPT"},
	{ 18,		"URR REGISTER REDIRECT"},
	{ 19,		"URR REGISTER REJECT"},
	{ 20,		"URR DEREGISTER"},
	{ 21,		"URR REGISTER UPDATE UPLINK"},
	{ 22,		"URR REGISTER UPDATE DOWNLINK"},
	{ 32,		"URR CIPHERING MODE COMMAND"},
	{ 33,		"URR CIPHERING MODE COMPLETE"},
	{ 48,		"URR ACTIVATE CHANNEL"},
	{ 49,		"URR ACTIVATE CHANNEL ACK"},
	{ 50,		"URR ACTIVATE CHANNEL COMPLETE"},
	{ 51,		"URR ACTIVATE CHANNEL FAILURE"},
	{ 52,		"URR CHANNEL MODE MODIFY"},
	{ 53,		"URR CHANNEL MODE MODIFY ACKNOWLEDGE"},
	{ 64,		"URR RELEASE"},
	{ 65,		"URR RELEASE COMPLETE"},
	{ 66,		"URR CLEAR REQUEST"},
	{ 80,		"URR HANDOVER ACCESS"},
	{ 81,		"URR HANDOVER COMPLETE"},
	{ 82,		"URR UPLINK QUALITY INDICATION"},
	{ 83,		"URR HANDOVER REQUIRED"},
	{ 84,		"URR HANDOVER COMMAND"},
	{ 85,		"URR HANDOVER FAILURE"},
	{ 96,		"URR PAGING REQUEST"},
	{ 97,		"URR PAGING RESPONSE"},
	{ 112,		"URR UPLINK DIRECT TRANSFER"},
	{ 113,		"URR INITIAL DIRECT TRANSFER"},
	{ 114,		"URR DOWNLINK DIRECT TRANSFER"},
	{ 115,		"URR STATUS"},
	{ 116,		"URR KEEP ALIVE"},
	{ 117,		"URR CLASSMARK ENQUIRY"},
	{ 118,		"URR CLASSMARK CHANGE"},
	{ 119,		"URR GPRS SUSPENSION REQUEST"},
	{ 120,		"URR SYNCHRONIZATION INFORMATION"},
	{ 128,		"URR REQUEST"},
	{ 129,		"URR REQUEST ACCEPT"},
	{ 130,		"URR REQUEST REJECT"},	
	{ 0,	NULL }
};
/* 
 * Message types for URLC signaling
 */
static const value_string uma_urlc_msg_type_vals[] = {
	{ 1,		"URLC-DATA"},
	{ 2,		"URLC UNITDATA"},
	{ 3,		"URLC-PS-PAGE"},
	{ 6,		"URLC-UFC-REQ"},
	{ 7,		"URLC-DFC-REQ"},
	{ 8,		"URLC-ACTIVATE-UTC-REQ"},
	{ 9,		"URLC-ACTIVATE-UTC-ACK"},
	{ 10,		"URLC-DEACTIVATE-UTC-REQ"},
	{ 11,		"URLC-DEACTIVATE-UTC-ACK"},
	{ 12,		"URLC STATUS"},
	{ 0,	NULL }
};
/* 
 * IE type and identifiers for Unlicensed Radio Resources management
 */
static const value_string uma_urr_IE_type_vals[] = {
	{ 1,		"Mobile Identity"},
	{ 2,		"UMA Release Indicator"},
	{ 3,		"Radio Identity"},
	{ 4,		"Cell Identity"},
	{ 5,		"Location Area Identification"},
	{ 6,		"GSM Coverage Indicator"},
	{ 7,		"UMA Classmark"},
	{ 8,		"Geographical Location"},
	{ 9,		"UNC SGW IP Address"},
	{ 10,		"UNC SGW Fully Qualified Domain/Host Name"},
	{ 11,		"Redirection Counter"},
	{ 12,		"Discovery Reject Cause"},
	{ 13,		"UMA Cell Description"},
	{ 14,		"UMA Control Channel Description"},
	{ 15,		"Cell Identifier List"},
	{ 16,		"TU3907 Timer"},
	{ 17,		"GSM RR State"},
	{ 18,		"Routing Area Identification"},
	{ 19,		"UMA Band"},
	{ 20,		"URR State"},
	{ 21,		"Register Reject Cause"},
	{ 22,		"TU3906 Timer"},
	{ 23,		"TU3910 Timer"},
	{ 24,		"TU3902 Timer"},
	{ 25,		"Communication Port Identity"},
	{ 26,		"L3 Message"},
	{ 27,		"Channel Mode"},
	{ 28,		"Mobile Station Classmark 2"},
	{ 29,		"RR Cause"},
	{ 30,		"Cipher Mode Setting"},
	{ 31,		"GPRS Resumption"},
	{ 32,		"Handover From UMAN Command"},
	{ 33,		"UL Quality Indication"},
	{ 34,		"TLLI"},
	{ 35,		"Packet Flow Identifier"},
	{ 36,		"Suspension Cause"},
	{ 37,		"TU3920 Timer"},
	{ 38,		"QoS"},
	{ 39,		"URLC Cause"},
	{ 40,		"User Data Rate"},
	{ 41,		"Routing Area Code"},
	{ 42,		"AP Location"},
	{ 43,		"TU4001 Timer"},
	{ 44,		"Location Status"},
	{ 45,		"Cipher Response"},
	{ 46,		"Ciphering Command RAND"},
	{ 47,		"Ciphering Command MAC"},
	{ 48,		"Ciphering Key Sequence Number"},
	{ 49,		"SAPI ID"},
	{ 50,		"Establishment Cause"},
	{ 51,		"Channel Needed"},
	{ 52,		"PDU in Error"},
	{ 53,		"Sample Size"},
	{ 54,		"Payload Type"},
	{ 55,		"Multi-rate Configuration"},
	{ 56,		"Mobile Station Classmark 3"},
	{ 57,		"LLC-PDU"},
	{ 58,		"Location Black List indicator"},
	{ 59,		"Reset Indicator"},
	{ 60,		"TU4003 Timer"},
	{ 61,		"AP Service Name"},
	{ 62,		"UMA Service Zone Information"},
	{ 63,		"RTP Redundancy Configuration"},
	{ 64,		"UTRAN Classmark"},
	{ 65,		"Classmark Enquiry Mask"},
	{ 66,		"UTRAN Cell Identifier List"},
	{ 67,		"Serving UNC table indicator"},
	{ 68,		"Registration indicators"},
	{ 69,		"UMA PLMN List"},
	{ 70,		"Received Signal Level List"},
	{ 96,		"MS Radio Identity"},
	{ 97,		"UNC IP Address"},
	{ 98,		"UNC Fully Qualified Domain/Host Name"},
	{ 99,		"IP address for GPRS user data transport"},
	{ 100,		"UDP Port for GPRS user data transport"},
	{ 103,		"UNC TCP port"},
	{ 104,		"RTP UDP port"},
	{ 105,		"RTCP UDP port"},
	{ 106,		"GERAN Received Signal Level List"},
	{ 107,		"UTRAN Received Signal Level List"},
	{ 0,	NULL }
};

static const value_string uma_urr_mobile_identity_type_vals[] = {
	{ 1,		"IMSI"},
	{ 2,		"IMEI"},
	{ 3,		"IMEISV"},
	{ 4,		"TMSI/P-TMSI"},
	{ 0,		"No Identity"},
	{ 0,	NULL }
};

static const value_string uma_urr_oddevenind_vals[] = {
	{ 0,		"Even number of identity digits"},
	{ 1,		"Odd number of identity digits"},
	{ 0,	NULL }
};

static const value_string radio_type_of_id_vals[] = {
	{ 0,		"IEEE MAC-address format"},
	{ 0,	NULL }
};

/* GCI, GSM Coverage Indicator (octet 3) */
static const value_string gci_vals[] = {
	{ 0,		"Normal Service in the GERAN"},
	{ 1,		"Limited Service in the GERAN"},
	{ 2,		"MS has not found GSM coverage (LAI information taken from SIM, if available)"},
	{ 3,		"MS has found GSM coverage, service state unknown"},
	{ 0,	NULL }
};
/* TURA, Type of Unlicensed Radio (octet 3) */
static const value_string tura_vals[] = {
	{ 0,		"No radio"},
	{ 1,		"Bluetooth"},
	{ 2,		"WLAN 802.11"},
	{ 3,		"Unspecified"},
	{ 0,	NULL }
};

/*TLRA, Type of Licensed Radio (octet 3)*/
static const value_string tlra_vals[] = {
	{ 0,		"None"},
	{ 1,		"GERAN"},
	{ 0,	NULL }
};
/*RRS, RTP Redundancy Support (octet 4)*/
static const value_string rrs_vals[] = {
	{ 0,		"RTP Redundancy not supported"},
	{ 1,		"RTP Redundancy supported"},
	{ 0,	NULL }
};
/* TS 23 032 Table 2a: Coding of Type of Shape */
static const value_string type_of_shape_vals[] = {
	{ 0,		"Ellipsoid Point"},
	{ 1,		"Ellipsoid point with uncertainty Circle"},
	{ 2,		"Ellipsoid point with uncertainty Ellipse"},
	{ 5,		"Polygon"},
	{ 8,		"Ellipsoid point with altitude"},
	{ 9,		"Ellipsoid point with altitude and uncertainty Ellipsoid"},
	{ 10,		"Ellipsoid Arc"},
	{ 0,	NULL }
};


/*IP address type number value (octet 3)*/
static const value_string IP_address_type_vals[] = {
	{ 0x21,		"IPv4 address"},
	{ 0x57,		"IPv6 address"},
	{ 0,	NULL }
};

/*Discovery Reject Cause (octet 3) */
static const value_string discovery_reject_cause_vals[] = {
	{ 0,		"Network Congestion"},
	{ 1,		"Unspecified"},
	{ 2,		"IMSI not allowed"},
	{ 0,	NULL }
};
/*EC Emergency Call allowed (octet 3)*/
static const value_string EC_vals[] = {
	{ 0,		"Emergency call allowed in the cell to all MSs"},
	{ 1,		"Emergency call not allowed in the cell except for the MSs that belong to one of the classes between 11 to 15."},
	{ 0,	NULL }
};
/*ECMC, Early Classmark Sending Control (octet 3)*/
static const value_string ECMC_vals[] = {
	{ 0,		"Early Classmark Sending is allowed"},
	{ 1,		"Early Classmark Sending is forbidden"},
	{ 0,	NULL }
};
/*NMO, Network Mode of Operation (octet 3)*/
static const value_string NMO_vals[] = {
	{ 0,		"Network Mode of Operation I"},
	{ 1,		"Network Mode of Operation II"},
	{ 2,		"Network Mode of Operation III"},
	{ 3,		"Reserved"},
	{ 0,	NULL }
};
/*GPRS, GPRS Availability (octet 3)*/
static const value_string GPRS_avail_vals[] = {
	{ 0,		"GPRS available"},
	{ 1,		"GPRS not available"},
	{ 0,	NULL }
};
/*DTM, Dual Transfer Mode of Operation by network (octet 3)*/
static const value_string DTM_vals[] = {
	{ 0,		"Network does not support dual transfer mode"},
	{ 1,		"Network supports dual transfer mode"},
	{ 0,	NULL }
};
/*ATT, Attach-detach allowed (octet 3)*/
static const value_string ATT_vals[] = {
	{ 0,		"MSs in the cell are not allowed to apply IMSI attach and detach procedure."},
	{ 1,		"MSs in the cell shall apply IMSI attach and detach procedure."},
	{ 0,	NULL }
};
/*MSCR, MSC Release (octet 3)*/
static const value_string MSCR_vals[] = {
	{ 0,		"MSC is Release '98 or older"},
	{ 1,		"MSC is Release '99 onwards"},
	{ 0,	NULL }
};

/* SGSNR, SGSN Release (octet 6)*/
static const value_string SGSNR_vals[] = {
	{ 0,		"SGSN is Release '98 or older"},
	{ 1,		"SGSN is Release '99 onwards"},
	{ 0,	NULL }
};
/* ECMP, Emergency Call Mode Preference (octet 6)*/

static const value_string ECMP_vals[] = {
	{ 0,		"GSM GERAN is preferred for Emergency calls"},
	{ 1,		"UMAN is preferred for Emergency calls"},
	{ 0,	NULL }
};
/* RE, Call reestablishment allowed (octet 6) */
static const value_string RE_vals[] = {
	{ 0,		"Call Reestablishment allowed in the cell"},
	{ 1,		"Call Reestablishment not allowed in the cell"},
	{ 0,	NULL }
};
/* PFCFM, PFC_FEATURE_MODE (octet 6) */
static const value_string PFCFM_vals[] = {
	{ 0,		"The network does not support packet flow context procedures"},
	{ 1,		"The network supports packet flow context procedures"},
	{ 0,	NULL }
};

/* 3GECS, 3G Early Classmark Sending Restriction (octet 6) */
static const value_string Three_GECS_vals[] = {
	{ 0,		"UTRAN classmark change message shall be sent with the Early classmark sending"},
	{ 1,		"The sending of UTRAN Classmark Sending messages is controlled by the Early Classmark Sending Control parameter"},
	{ 0,	NULL }
};
/* Cell identification discriminator */
static const value_string cell_id_disc_vals[] = {
	{ 0,		"The whole Cell Global Identification, CGI, is used to identify the cells."},
	{ 1,		"Location Area Code, LAC, and Cell Identify, CI, is used to identify the cells."},
	{ 2,		"Cell Identity, CI, is used to identify the cells."},
	{ 3,		"No cell is associated with the transaction."},
	{ 4,		"Location Area Identification, LAI, is used to identify all cells within a Location Area."},
	{ 5,		"Location Area Code, LAC, is used to identify all cells within a location area."},
	{ 6,		"All cells on the BSS are identified."},
	{ 8,		"Intersystem Handover to UTRAN or cdma2000. PLMN-ID, LAC, and RNC-ID, are encoded to identify the target RNC."},
	{ 9,		"Intersystem Handover to UTRAN or cdma2000. The RNC-ID is coded to identify the target RNC."},
	{ 10,		"Intersystem Handover to UTRAN or cdma2000. LAC and RNC-ID are encoded to identify the target RNC."},
	{ 0,	NULL }
};

/*GRS, GSM RR State (octet 3)*/
static const value_string GRS_GSM_RR_State_vals[] = {
	{ 0,		"GSM RR is in IDLE state"},
	{ 1,		"GSM RR is in DEDICATED state"},
	{ 0,	NULL }
};
/* UMA Band (4 bit field) */
static const value_string UMA_band_vals[] = {
	{ 0,		"E-GSM is supported"},
	{ 1,		"P-GSM is supported"},
	{ 2,		"GSM 1800 is supported"},
	{ 3,		"GSM 450 is supported"},
	{ 4,		"GSM 480 is supported"},
	{ 5,		"GSM 850 is supported"},
	{ 6,		"GSM 1900 is supported"},
	{ 7,		"GSM 700 is supported"},
	{ 0,	NULL }
};
/*URS, URR State (octet 3) */
static const value_string URR_state_vals[] = {
	{ 0,		"URR is in URR-IDLE state"},
	{ 1,		"URR is in URR-DEDICATED state"},
	{ 2,		"URR is in URR-REGISTERED state"},
	{ 0,	NULL }
};
/* Mobile Station Classmark 2 
 * Revision level (octet 3)
 */
static const value_string MSC2_rev_vals[] = {
	{ 0,		"Reserved for GSM phase 1"},
	{ 1,		"Used by GSM phase 2 mobile stations"},
	{ 2,		"Used by mobile stations supporting R99 or later versions of the protocol"},
	{ 3,		"Reserved for future use"},
	{ 0,	NULL }
};
/* RR cause value (octet 2) */
static const value_string RR_cause_vals[] = {
	{ 0,		"Normal event"},
	{ 1,		"Abnormal release, unspecified"},
	{ 2,		"Abnormal release, channel unacceptable"},
	{ 3,		"Abnormal release, timer expired"},
	{ 4,		"Abnormal release, no activity on the radio path"},
	{ 5,		"Preemptive release"},
	{ 6,		"UTRAN configuration unknown"},
	{ 8,		"Handover impossible, timing advance out of range"},
	{ 9,		"Channel mode unacceptable"},
	{ 10,		"Frequency not implemented"},
	{ 13,		"Originator or talker leaving group call area"},
	{ 12,		"Lower layer failure"},
	{ 0x41,		"Call already cleared"},
	{ 0x5f,		"Semantically incorrect message"},
	{ 0x60,		"Invalid mandatory information"},
	{ 0x61,		"Message type non-existent or not implemented"},
	{ 0x62,		"Message type not compatible with protocol state"},
	{ 0x64,		"Conditional IE error"},
	{ 0x65,		"No cell allocation available"},
	{ 0x6f,		"Protocol error unspecified"},
	{ 0,	NULL }
};
/* Register Reject Cause (octet 3) */
static const value_string register_reject_cause_vals[] = {
	{ 0,		"Network Congestion"},
	{ 1,		"AP not allowed"},
	{ 2,		"Location not allowed"},
	{ 3,		"Invalid UNC"},
	{ 4,		"Geo Location not known"},
	{ 5,		"IMSI not allowed"},
	{ 6,		"Unspecified"},
	{ 7,		"UNC-SGW certificate not valid"},
	{ 8,		"EAP_SIM authentication failed"},
	{ 9,		"TCP establishment failed"},
	{ 10,		"Redirection"},
	{ 11,		"EAP-AKA authentication failed"},
	{ 0,	NULL }
};
	
/* L3 Protocol discriminator values according to TS 24 007 (640)  */
static const value_string protocol_discriminator_vals[] = {
	{0x0,		"Group call control"},
	{0x1,		"Broadcast call control"},
	{0x2,		"Reserved: was allocated in earlier phases of the protocol"},
	{0x3,		"Call Control; call related SS messages"},
	{0x4,		"GPRS Transparent Transport Protocol (GTTP)"},
	{0x5,		"Mobility Management messages"},
	{0x6,		"Radio Resources Management messages"},
	{0x7,		"Unknown"},
	{0x8,		"GPRS mobility management messages"},
	{0x9,		"SMS messages"},
	{0xa,		"GPRS session management messages"},
	{0xb,		"Non call related SS messages"},
	{0xc,		"Location services specified in 3GPP TS 44.071 [8a]"},
	{0xd,		"Unknown"},
	{0xe,		"Reserved for extension of the PD to one octet length "},
	{0xf,		"Reserved for tests procedures described in 3GPP TS 44.014 [5a] and 3GPP TS 34.109 [17a]."},
	{ 0,	NULL }
};
/* Channel Mode  */
static const value_string channel_mode_vals[] = {
{ 0x00,		"signalling only"},
{ 0x01,		"speech full rate or half rate version 1(GSM FR or GSM HR)"},
{ 0x21,		"speech full rate or half rate version 2(GSM EFR)"},
{ 0x41,		"speech full rate or half rate version 3(FR AMR or HR AMR)"},
{ 0x81,		"speech full rate or half rate version 4(OFR AMR-WB or OHR AMR-WB)"},
{ 0x82,		"speech full rate or half rate version 5(FR AMR-WB )"},
{ 0x83,		"speech full rate or half rate version 6(OHR AMR )"},
{ 0x61,		"data, 43.5 kbit/s (downlink)+14.5 kbps (uplink)"},
{ 0x62,		"data, 29.0 kbit/s (downlink)+14.5 kbps (uplink)"},
{ 0x64,		"data, 43.5 kbit/s (downlink)+29.0 kbps (uplink)"},
{ 0x67,		"data, 14.5 kbit/s (downlink)+43.5 kbps (uplink)"},
{ 0x65,		"data, 14.5 kbit/s (downlink)+29.0 kbps (uplink)"},
{ 0x66,		"data, 29.0 kbit/s (downlink)+43.5 kbps (uplink)"},
{ 0x27,		"data, 43.5 kbit/s radio interface rate"},
{ 0x63,		"data, 32.0 kbit/s radio interface rate"},
{ 0x43,		"data, 29.0 kbit/s radio interface rate"},
{ 0x0f,		"data, 14.5 kbit/s radio interface rate"},
{ 0x03,		"data, 12.0 kbit/s radio interface rate"},
{ 0x0b,		"data, 6.0 kbit/s radio interface rate"},
{ 0x13,		"data, 3.6 kbit/s radio interface rate"},
	{ 0,	NULL }
};
/* ES IND (octet 3, bit 5) "Controlled Early Classmark Sending" option implementation */
static const value_string ES_IND_vals[] = {
	{ 0,		"Controlled Early Classmark Sending option is not implemented in the MS"},
	{ 1,		"Controlled Early Classmark Sending option is implemented in the MS"},
	{ 0,	NULL }
};
/* A5/1 algorithm supported (octet 3, bit 4 */
static const value_string A5_1_algorithm_sup_vals[] = {
	{ 0,		"encryption algorithm A5/1 available"},
	{ 1,		"encryption algorithm A5/1 not available"},
	{ 0,	NULL }
};
/* RF Power Capability (Octet 3) */
static const value_string RF_power_capability_vals[] = {
	{ 0,		"class 1"},
	{ 1,		"class 2"},
	{ 2,		"class 3"},
	{ 3,		"class 4"},
	{ 4,		"class 5"},
	{ 7,		"RF Power capability is irrelevant in this information element"},
	{ 0,	NULL }
};
/* PS capability (pseudo-synchronization capability) (octet 4) */
static const value_string ps_sup_cap_vals[] = {
	{ 0,		"PS capability not present"},
	{ 1,		"PS capability present"},
	{ 0,	NULL }
};
/* SS Screening Indicator (octet 4)defined in 3GPP TS 24.080 */
static const value_string SS_screening_indicator_vals[] = {
	{ 0,		"Default value of phase 1"},
	{ 1,		"Capability of handling of ellipsis notation and phase 2 error handling "},
	{ 2,		"For future use"},
	{ 3,		"For future use"},
	{ 0,	NULL }
};
/* SM capability (MT SMS pt to pt capability) (octet 4)*/
static const value_string SM_capability_vals[] = {
	{ 0,		"Mobile station does not support mobile terminated point to point SMS"},
	{ 1,		"Mobile station supports mobile terminated point to point SMS"},
	{ 0,	NULL }
};
/* VBS notification reception (octet 4) */
static const value_string VBS_notification_rec_vals[] = {
	{ 0,		"no VBS capability or no notifications wanted"},
	{ 1,		"VBS capability and notifications wanted"},
	{ 0,	NULL }
};
/* VGCS notification reception (octet 4) */
static const value_string VGCS_notification_rec_vals[] = {
	{ 0,		"no VGCS capability or no notifications wanted"},
	{ 1,		"VGCS capability and notifications wanted"},
	{ 0,	NULL }
};
/* FC Frequency Capability (octet 4 ) */
static const value_string FC_frequency_cap_vals[] = {
	{ 0,		"The MS does not support the E-GSM or R-GSM band"},
	{ 1,		"The MS does support the E-GSM or R-GSM "},
	{ 0,	NULL }
};
/* CM3 (octet 5, bit 8) */
static const value_string CM3_vals[] = {
	{ 0,		"The MS does not support any options that are indicated in CM3"},
	{ 1,		"The MS supports options that are indicated in classmark 3 IE"},
	{ 0,	NULL }
};
/* LCS VA capability (LCS value added location request notification capability) (octet 5,bit 6) */
static const value_string LCS_VA_cap_vals[] = {
	{ 0,		"LCS value added location request notification capability not supported"},
	{ 1,		"LCS value added location request notification capability supported"},
	{ 0,	NULL }
};
/* UCS2 treatment (octet 5, bit 5) */
static const value_string UCS2_treatment_vals[] = {
	{ 0,		"the ME has a preference for the default alphabet"},
	{ 1,		"the ME has no preference between the use of the default alphabet and the use of UCS2"},
	{ 0,	NULL }
};
/* SoLSA (octet 5, bit 4) */
static const value_string SoLSA_vals[] = {
	{ 0,		"The ME does not support SoLSA"},
	{ 1,		"The ME supports SoLSA"},
	{ 0,	NULL }
};
/* CMSP: CM Service Prompt (octet 5, bit 3) */
static const value_string CMSP_vals[] = {
	{ 0,		"Network initiated MO CM connection request not supported"},
	{ 1,		"Network initiated MO CM connection request supported for at least one CM protocol"},
	{ 0,	NULL }
};
/* A5/3 algorithm supported (octet 5, bit 2) */
static const value_string A5_3_algorithm_sup_vals[] = {
	{ 0,		"encryption algorithm A5/3 available"},
	{ 1,		"encryption algorithm A5/3 not available"},
	{ 0,	NULL }
};

/* A5/2 algorithm supported (octet 5, bit 1) */
static const value_string A5_2_algorithm_sup_vals[] = {
	{ 0,		"encryption algorithm A5/2 available"},
	{ 1,		"encryption algorithm A5/3 not available"},
	{ 0,	NULL }
};

/* algorithm identifier
 * If SC=1 then:
 * bits
 * 4 3 2
 */
 static const value_string algorithm_identifier_vals[] = {
	{ 0,		"Cipher with algorithm A5/1"},
 	{ 1,		"Cipher with algorithm A5/2"},
 	{ 2,		"Cipher with algorithm A5/3"},
	{ 3,		"Cipher with algorithm A5/4"},
 	{ 4,		"Cipher with algorithm A5/5"},
 	{ 5,		"Cipher with algorithm A5/6"},
 	{ 6,		"Cipher with algorithm A5/7"},
	{ 7,		"Reserved"},
	{ 0,	NULL }
};
/*  GPRS Resumption */
 static const value_string GPRS_resumption_vals[] = {
	{ 0,		"Resumption of GPRS services not successfully acknowledged"},
	{ 1,		"Resumption of GPRS services successfully acknowledged"},
	{ 0,	NULL }
};
/* SC (octet 1) */
static const value_string SC_vals[] = {
	{ 0,		"No ciphering"},
	{ 1,		"Start ciphering"},
	{ 0,	NULL }
};

/* ULQI, UL Quality Indication (octet 3) */
static const value_string ULQI_vals[] = {
	{ 0,		"Quality ok"},
	{ 1,		"Radio problem"},
	{ 2,		"Network problem"},
	{ 4,		"Undetermined problem"},
	{ 0,	NULL }
};

/* Packet Flow Identifier value (octet 3) */
static const value_string packet_flow_id_vals[] = {
	{ 0,		"Best Effort"},
	{ 1,		"Signaling"},
	{ 2,		"SMS"},
	{ 3,		"TOM8"},
	{ 0,	NULL }
};
/*Suspension cause value (octet 2)*/
static const value_string suspension_cause_vals[] = {
	{ 0,		"Emergency call, mobile originating call or call re-establishment"},
	{ 1,		"Location Area Update"},
	{ 2,		"MO Short message service"},
	{ 3,		"Other procedure which can be completed with an SDCCH"},
	{ 4,		"MO Voice broadcast or group call"},
	{ 5,		"Mobile terminating CS connection"},
	{ 6,		"DTM not supported in the cell"},
	{ 0,	NULL }
};
static const value_string precedence_vals[] = {
	{ 0,		"Radio priority 1"},
	{ 1,		"Radio priority 2"},
	{ 2,		"Radio priority 3"},
	{ 3,		"Radio priority 4"},
	{ 3,		"Radio Priority Unknown"},
	{ 0,	NULL }
};
static const value_string a_bit_vals[] = {
	{ 0,		"Radio interface uses RLC/MAC ARQ functionality"},
	{ 1,		"Radio interface uses RLC/MAC-UNITDATA functionality"},
	{ 0,	NULL }
};
static const value_string t_bit_vals[] = {
	{ 0,		"The SDU contains signalling (e.g. related to GMM)"},
	{ 1,		"The SDU contains data"},
	{ 0,	NULL }
};
static const value_string c_r_bit_vals[] = {
	{ 0,		"The SDU contains a LLC ACK or SACK command/response frame type "},
	{ 1,		"The SDU does not contain a LLC ACK or SACK command/response frame type"},
	{ 0,	NULL }
};
/*URLC Cause (octet 3) */
static const value_string URLC_cause_vals[] = {
	{ 0,		"success"},
	{ 2,		"no available resources"},
	{ 3,		"UNC failure"},
	{ 4,		"not authorized for data service"},
	{ 5,		"message type non existent or not implemented"},
	{ 6,		"message type not compatible with the protocol state"},
	{ 7,		"invalid mandatory information"},
	{ 8,		"syntactically incorrect message"},
	{ 9,		"GPRS suspended"},
	{ 10,		"normal deactivation"},
	{ 12,		"conditional IE error"},
	{ 13,		"semantically incorrect message"},
	{ 0,	NULL }
};

/* LS, Location Status (octet 3) */
static const value_string LS_vals[] = {
	{ 0,		"MS location known"},
	{ 1,		"MS location unknown"},
	{ 0,	NULL }
};

/* CR Cipher Response (octet 1) */
static const value_string CR_vals[] = {
	{ 0,		"IMEISV shall not be included"},
	{ 1,		"IMEISV shall be included"},
	{ 0,	NULL }
};

/* SAPI ID, SAPI Identifier (octet 3) */
static const value_string sapi_id_vals[] = {
	{ 0,		"SAPI 0 (all other except SMS)"},
	{ 3,		"SAPI 3 (SMS)"},
	{ 0,	NULL }
};
/*	Sample Size (octet 3)*/
static const value_string sample_size_vals[] = {
	{ 20,		"20 ms of CS payload included in each RTP/UDP packet"},
	{ 40,		"40 ms of CS payload included in each RTP/UDP packet"},
	{ 60,		"60 ms of CS payload included in each RTP/UDP packet"},
	{ 80,		"80 ms of CS payload included in each RTP/UDP packet"},
	{ 0,	NULL }
};
	
/*	Multirate speech version Octet 3 Bits 8 7 6 */
static const value_string multirate_speech_ver_vals[] = {
	{ 1,		"Adaptive Multirate speech version 1"},
	{ 2,		"Adaptive Multirate speech version 2"},
	{ 0,	NULL }
};
/* Bit	5 	NSCB: Noise Suppression Control Bit */
static const value_string NSCB_vals[] = {
	{ 0,		"Noise Suppression can be used (default)"},
	{ 1,		"Noise Suppression shall be turned off"},
	{ 0,	NULL }
};
/* Bit	4	ICMI: Initial Codec Mode Indicator */
static const value_string ICMI_vals[] = {
	{ 0,		"The initial codec mode is defined by the implicit rule provided in 3GPP TS 05.09"},
	{ 1,		"The initial codec mode is defined by the Start Mode field"},
	{ 0,	NULL }
};
/* MPS, Manual PLMN Selection indicator (octet 3) */
static const value_string mps_vals[] = {
	{ 0,		"The MS is in Automatic PLMN selection mode."},
	{ 1,		"The MS is in Manual PLMN selection mode and request the listof PLMN identities that may provide UMAN service in the current location."},
	{ 2,		"The MS is in Manual PLMN selection mode and tries to register; no PLMN list is needed."},
	{ 0,	NULL }
};
/* LBLI, Location Black List indicator (octet 3) */
static const value_string LBLI_vals[] = {
	{ 0,		"MCC"},
	{ 1,		"MCC and MNC"},
	{ 2,		"MCC, MNC and LAC"},
	{ 0,	NULL }
};
/* AP Service Name type */
static const value_string ap_service_name_type_vals[] = {
	{ 0,		"SSID"},
	{ 1,		"PAN Service Name"},
	{ 0,	NULL }
};

/* UMA Service Zone Icon Indicator, octet 3 */
static const value_string uma_service_zone_icon_ind_vals[] = {
	{ 1,		"Unlimited Calls"},
	{ 0,	NULL }
};
/*Establishment Cause (octet 3)*/
static const value_string establishment_cause_vals[] = {
	{ 0xa0,		"Emergency"},
	{ 0xc0,		"Call re-establishment"},
	{ 0x00,		"Location Update"},
	{ 0x10,		"Other SDCCH procedures including IMSI Detach, SMS, SS, paging response"},
/* note: Paging response for SDCCH needed is using codepoint  0001 0000 */
	{ 0x20,		"Paging response (TCH/F needed)"},
	{ 0x30,		"Paging response (TCH/F or TCH/H needed)"},
	{ 0x80,		"Paging response (any channel needed)"},
	{ 0x40,		"Originating speech call from dual-rate mobile station when TCH/H is sufficient"},
	{ 0x50,		"Originating data call from dual-rate mobile station when TCH/H is sufficient"},
	{ 0xe0,		"Originating speech call and TCH/F is needed"},
	{ 0xf0,		"Originating data call and TCH/F is needed"},
	{ 0,	NULL }
};
/*CHANNEL (octet 3) */
static const value_string channel_vals[] = {
	{ 0,		"Any channel"},
	{ 1,		"SDCCH"},
	{ 2,		"TCH/F (Full rate)"},
	{ 3,		"TCH/H or TCH/F (Dual rate)"},
	{ 0,	NULL }
};

/*RI, Reset Indicator (octet 3)*/
/*CHANNEL (octet 3) */
static const value_string RI_vals[] = {
	{ 0,		"The flow control condition continues to exist"},
	{ 1,		"The flow control condition no longer exists"},
	{ 0,	NULL }
};

/* Window Size (octet 3 to octet n) */

static const value_string window_size_vals[] = {
	{ 0,		"Window size 1 (single redundancy)"},
	{ 1,		"Window size 2 (double redundancy)"},
	{ 0,	NULL }
};

static const value_string UTRAN_cell_id_disc_vals[] = {
	{ 0,		"PLMN-ID, LAC and a 28-bit Cell Id are used to identify the target UTRAN cell."},
	{ 0,	NULL }
};

/* SUTI, Serving UNC table indicator indicator (octet 3) */

static const value_string suti_vals[] = {
	{ 0,		"The MS is not allowed to store information in the stored Serving UNC table."},
	{ 1,		"The MS is allowed to store information in the stored Serving UNC table."},
	{ 0,	NULL }
};
	/* Code to actually dissect the packets */
static int
dissect_mcc_mnc(tvbuff_t *tvb, proto_tree *urr_ie_tree, int offset){

	int			start_offset;	
	guint8		octet;
	guint16		mcc, mnc;
	guint8		mcc1, mcc2, mcc3, mnc1, mnc2, mnc3;

	start_offset = offset;
	/* Mobile country code MCC */
	octet = tvb_get_guint8(tvb,offset);
	mcc1 = octet & 0x0f;
	mcc2 = octet >> 4;
	offset++;
	octet = tvb_get_guint8(tvb,offset);
	mcc3 = octet & 0x0f;
	/* MNC, Mobile network code (octet 3 bits 5 to 8, octet 4)  */
	mnc3 = octet >> 4;
	offset++;
	octet = tvb_get_guint8(tvb,offset);
	mnc1 = octet & 0x0f;
	mnc2 = octet >> 4;

	mcc = 100 * mcc1 + 10 * mcc2 + mcc3;
	mnc = 10 * mnc1 + mnc2;
	if (mnc3 != 0xf) {
		mnc += 10 * mnc + mnc3;
	}
	proto_tree_add_uint(urr_ie_tree, hf_uma_urr_mcc , tvb, start_offset, 2, mcc );
	proto_tree_add_uint(urr_ie_tree, hf_uma_urr_mnc , tvb, start_offset + 1, 2, mnc );
	return offset;
}
static int
dissect_uma_IE(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset)
{
	tvbuff_t	*l3_tvb;
	tvbuff_t	*llc_tvb;
	int			ie_offset;
	guint8		ie_value;
	guint16		ie_len = 0;
	guint8		octet;
	guint8		mobile_identity_type;
	proto_item	*urr_ie_item;
	proto_tree	*urr_ie_tree;
	char		digit_str[16]; /* Max no of digits = 15 */
	guint16		i,n;
	guint16		num_digits;
	guint16		bcch_arfcn;
	const guint8	*haddr;
	char		*string;
	guint16		GPRS_user_data_transport_UDP_port,UNC_tcp_port,RTP_UDP_port,RTCP_UDP_port, communication_port;
	guint32		udr;
	conversation_t *conversation;
	address dst_addr, null_addr;
	guint8		str_len;

	ie_value = tvb_get_guint8(tvb,offset);
	urr_ie_item = proto_tree_add_text(tree,tvb,offset,-1,"%s",
		val_to_str(ie_value, uma_urr_IE_type_vals, "Unknown IE (%u)"));
	urr_ie_tree = proto_item_add_subtree(urr_ie_item, ett_urr_ie);

	proto_tree_add_item(urr_ie_tree, hf_uma_urr_IE, tvb, offset, 1, FALSE);
	offset++;
	/* Some IE:s might have a lengt field of 2 octets */
	ie_len = tvb_get_guint8(tvb,offset);
	switch(ie_value){
	case 57:
		if ( (ie_len & 0x80) == 0x80 ){
			offset++;
			ie_len = (ie_len & 0x7f) << 8;
			ie_len = ie_len | (tvb_get_guint8(tvb,offset));
			proto_item_set_len(urr_ie_item, ie_len + 3);
			proto_tree_add_item(urr_ie_tree, hf_uma_urr_IE_len2, tvb, offset-1, 2, FALSE);
			ie_offset = offset +1;
		}else{
		proto_item_set_len(urr_ie_item, ie_len + 2);
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_IE_len, tvb, offset, 1, FALSE);
		ie_offset = offset +1;
		}
		break;
	default:
		proto_item_set_len(urr_ie_item, ie_len + 2);
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_IE_len, tvb, offset, 1, FALSE);
		ie_offset = offset +1;
		break;
	}

	switch(ie_value){
	case 1:			/* Mobile Identity */
		octet = tvb_get_guint8(tvb,ie_offset);
		mobile_identity_type = octet & 0x7;
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_mobile_identity_type, tvb, ie_offset, 1, FALSE);
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_odde_even_ind, tvb, ie_offset, 1, FALSE);

		i = 0;
		n = 0;
	    num_digits = 1 + ((ie_len-1) * 2);
		if ((octet & 0x8) == 0 ) num_digits--; /* Even number of digits */
		octet = octet >> 4;
		digit_str[i] = octet + 0x30;
		n++;
		i++;
		while ( i < 16 ){
			octet = tvb_get_guint8(tvb,ie_offset+n);
			digit_str[i] = ( octet & 0xf ) + 0x30;
			if ( i == num_digits-1){
				i++;
				digit_str[i] = '\0';
				break;
			}
			i++;
			digit_str[i] = ( octet >> 4 ) + 0x30;
			if ( i == num_digits-1){
				i++;
				digit_str[i] = '\0';
				break;
			}
			i++;
			n++;
		}
		switch(mobile_identity_type){
		case 1: 
			/*IMSI*/
			proto_tree_add_string(urr_ie_tree, hf_uma_urr_imsi, tvb, ie_offset, ie_len, digit_str);
			break;
		case 2: 
			/*IMEI*/
			proto_tree_add_string(urr_ie_tree, hf_uma_urr_imei, tvb, ie_offset, ie_len, digit_str);
			break;
		case 3: 
			/*IMEISV*/
			proto_tree_add_string(urr_ie_tree, hf_uma_urr_imeisv, tvb, ie_offset, ie_len, digit_str);
			break;
		case 4 :
			/*TMSI/P-TMSI*/
			proto_tree_add_string(urr_ie_tree, hf_uma_urr_tmsi_p_tmsi, tvb, ie_offset, ie_len, digit_str);
			break;
		default: /*No Identity*/
			proto_tree_add_text(urr_ie_tree, tvb, ie_offset, ie_len,"No Identity");

			break;
		}
	break;

	case 2:			
		/* UMA Release Indicator */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_uri, tvb, ie_offset, 1, FALSE);
		break;
	case 3:			/* Radio Identity */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_radio_type_of_id, tvb, ie_offset, 1, FALSE);
		octet = tvb_get_guint8(tvb,ie_offset);
		if (( octet & 0xf) == 0){ /* IEEE MAC-address format */
			ie_offset++;
			haddr = tvb_get_ptr(tvb, ie_offset, ie_len);
			proto_tree_add_ether(urr_ie_tree, hf_uma_urr_radio_id, tvb, ie_offset, ie_len, haddr);
		}else{
			proto_tree_add_text(urr_ie_tree, tvb, ie_offset, ie_len,"Unknown format");
		}
		break;
	case 4:			
		/* Cell Identity 
		 * The rest of the IE is coded as in [TS 24.008] not including IEI and length, if present.
		 */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_cell_id, tvb, ie_offset, 2, FALSE);
		break;
	case 5:			
		/* Location Area Identification 
		 * The rest of the IE is coded as in [TS 24.008] not including IEI and
		 * length, if present.
		 *
		 * Mobile country code MCC */
		ie_offset = dissect_mcc_mnc(tvb, urr_ie_tree, ie_offset);
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_lac, tvb, ie_offset, 2, FALSE);
		break;
	case 6:			
		/* GSM Coverage Indicator */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_gci, tvb, ie_offset, 1, FALSE);
		break;
	case 7:			
		/* UMA Classmark */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_tura, tvb, ie_offset, 1, FALSE);
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_tlra, tvb, ie_offset, 1, FALSE);
		/* UMA Protocols (Stage 3) R1.0.3 */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_rrs, tvb, ie_offset, 1, FALSE);
		break;
	case 8:			
		/* Geographical Location 
		 * The Location Estimate field is composed of 1 or more octets with an internal structure 
		 * according to section 7 in [23.032].
		 */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_location_estimate, tvb, ie_offset, ie_len, FALSE);
		/* TODO:Add further dissecton here ? */
		break;
	case 9:			
		/* UNC SGW IP Address 
		 * IP Address type
		 */
		octet = tvb_get_guint8(tvb,ie_offset);
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_IP_Address_type, tvb, ie_offset, 1, FALSE);
		ie_offset++;
		if ( octet == 0x57 ){ /* IPv6 */

		}else{ /* All other values shall be interpreted as Ipv4 address in this version of the protocol.*/
			tvb_memcpy(tvb, (guint8 *)&sgw_ipv4_address, ie_offset, 4);
			proto_tree_add_ipv4(urr_ie_tree, hf_uma_urr_sgw_ipv4, tvb, ie_offset, 4, sgw_ipv4_address);

		}
		break;		 
	case 10:		/* UNC SGW Fully Qualified Domain/Host Name */
		if ( ie_len > 0){
			string = tvb_get_string(tvb, ie_offset, ie_len);
			proto_tree_add_string(urr_ie_tree, hf_uma_urr_FQDN, tvb, ie_offset, ie_len, string);
			g_free(string);
		}else{
			proto_tree_add_text(urr_ie_tree,tvb,offset,1,"FQDN not present");
		}
		break;
	case 11:		/* Redirection Counter */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_redirection_counter, tvb, ie_offset, 1, FALSE);
		break;
	case 12:		/* Discovery Reject Cause */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_dis_rej_cau, tvb, ie_offset, 1, FALSE);
		break;
	case 13:		
		/* UMA Cell Description 
		 * The rest of the IE is coded as in [TS 44.018], Cell Description IE, not including IEI and length, if present
		 */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_bcc, tvb, ie_offset, 1, FALSE);
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_ncc, tvb, ie_offset, 1, FALSE);
		bcch_arfcn = (tvb_get_guint8(tvb,ie_offset) & 0xc0) << 2;
		bcch_arfcn = bcch_arfcn | tvb_get_guint8(tvb,ie_offset+1);
		proto_tree_add_uint(urr_ie_tree, hf_uma_urr_bcch_arfcn , tvb, ie_offset, 2, bcch_arfcn );
		break;
	case 14:		
		/* UMA Control Channel Description 
		 */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_EC, tvb, ie_offset, 1, FALSE);
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_ECMC, tvb, ie_offset, 1, FALSE);
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_NMO, tvb, ie_offset, 1, FALSE);
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_GPRS, tvb, ie_offset, 1, FALSE);
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_DTM, tvb, ie_offset, 1, FALSE);
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_ATT, tvb, ie_offset, 1, FALSE);
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_MSCR, tvb, ie_offset, 1, FALSE);
		/* T3212 timeout value */
		ie_offset++;
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_T3212_timer, tvb, ie_offset, 1, FALSE);
		/* RAC, Routing Area Code (octet 5) */
		ie_offset++;
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_RAC, tvb, ie_offset, 1, FALSE);
		ie_offset++;
		/* SGSNR, SGSN Release (octet 6) */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_SGSNR, tvb, ie_offset, 1, FALSE);
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_ECMP, tvb, ie_offset, 1, FALSE);
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_RE, tvb, ie_offset, 1, FALSE);
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_PFCFM, tvb, ie_offset, 1, FALSE);
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_3GECS, tvb, ie_offset, 1, FALSE);
		ie_offset++;
		proto_tree_add_text(urr_ie_tree,tvb,ie_offset,2,"Access Control Class N");
		break;
	case 15:		
		/* Cell Identifier List 
		 * The rest of the IE is coded as in [TS 48.008], not including IEI and length, if present
		 *
		 * Cell identification discriminator 
		 */
		octet = tvb_get_guint8(tvb,ie_offset);
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_cell_id_disc, tvb, ie_offset, 1, FALSE);
		ie_offset++;
		/* TODO make a proper loop over the list, once an example trace is avalable */
		switch (octet & 0xf){
		case 0:
			ie_offset = dissect_mcc_mnc(tvb, urr_ie_tree, ie_offset);
			proto_tree_add_item(urr_ie_tree, hf_uma_urr_lac, tvb, ie_offset, 2, FALSE);
			ie_offset = ie_offset +2;
			proto_tree_add_item(urr_ie_tree, hf_uma_urr_ci, tvb, ie_offset, 2, FALSE);
			ie_offset = ie_offset + 2;
			break;
		case 1:
			proto_tree_add_item(urr_ie_tree, hf_uma_urr_lac, tvb, ie_offset, 2, FALSE);
			ie_offset = ie_offset + 2;
			proto_tree_add_item(urr_ie_tree, hf_uma_urr_ci, tvb, ie_offset, 2, FALSE);
			ie_offset = ie_offset + 2;
			break;
		case 2:
			proto_tree_add_item(urr_ie_tree, hf_uma_urr_ci, tvb, ie_offset, 2, FALSE);
			ie_offset = ie_offset + 2;
			break;
		case 4:
			ie_offset = dissect_mcc_mnc(tvb, urr_ie_tree, ie_offset);
			proto_tree_add_item(urr_ie_tree, hf_uma_urr_lac, tvb, ie_offset, 2, FALSE);
			ie_offset = ie_offset +2;
			break;
		case 5:
			proto_tree_add_item(urr_ie_tree, hf_uma_urr_lac, tvb, ie_offset, 2, FALSE);
			ie_offset = ie_offset + 2;
		case 8:
			ie_offset = dissect_mcc_mnc(tvb, urr_ie_tree, ie_offset);
			proto_tree_add_item(urr_ie_tree, hf_uma_urr_lac, tvb, ie_offset, 2, FALSE);
			ie_offset = ie_offset +2;
			proto_tree_add_item(urr_ie_tree, hf_uma_urr_RNC_ID, tvb, ie_offset, 2, FALSE);
			ie_offset = ie_offset + 2;
			break;
		case 9:
			proto_tree_add_item(urr_ie_tree, hf_uma_urr_RNC_ID, tvb, ie_offset, 2, FALSE);
			ie_offset = ie_offset + 2;
			break;
		case 10:
			proto_tree_add_item(urr_ie_tree, hf_uma_urr_lac, tvb, ie_offset, 2, FALSE);
			ie_offset = ie_offset + 2;
			proto_tree_add_item(urr_ie_tree, hf_uma_urr_RNC_ID, tvb, ie_offset, 2, FALSE);
			ie_offset = ie_offset + 2;
			break;
		default:
			proto_tree_add_text(urr_ie_tree,tvb,offset+1,ie_len,"Unknown Cell identification discriminator");
			break;
		}
		break;
	case 16:		/* TU3907 Timer */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_TU3907_timer, tvb, ie_offset, 2, FALSE);
		break;
	case 17:		/* GSM RR State */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_GSM_RR_state, tvb, ie_offset, 2, FALSE);
		break;
	case 18:		/* Routing Area Identification */
		/* The rest of the IE is coded as in [TS 24.008] not including IEI and length, if present.*/
		ie_offset = dissect_mcc_mnc(tvb, urr_ie_tree, ie_offset);
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_lac, tvb, ie_offset, 2, FALSE);
		ie_offset = ie_offset +2;
		/*Routing area code */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_RAC, tvb, ie_offset, 1, FALSE);
		break;
	case 19:		/* UMA Band */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_UMA_band, tvb, ie_offset, 1, FALSE);
		break;
	case 20:		/* URR State */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_URR_state, tvb, ie_offset, 1, FALSE);
		break;
	case 21:		/* Register Reject Cause */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_register_reject_cause, tvb, ie_offset, 1, FALSE);
		break;
	case 22:		/* TU3906 Timer */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_TU3906_timer, tvb, ie_offset, 2, FALSE);
		break;
	case 23:		/* TU3910 Timer */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_TU3910_timer, tvb, ie_offset, 2, FALSE);
		break;
	case 24:		/* TU3902 Timer */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_TU3902_timer, tvb, ie_offset, 2, FALSE);
		break;
	case 25:
		/* Communication Port Identity */
		communication_port = tvb_get_ntohs(tvb,ie_offset);
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_communication_port, tvb, ie_offset, 2, FALSE);
		break;

	case 26:		/* L3 Message */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_L3_protocol_discriminator, tvb, ie_offset, 1, FALSE);
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_L3_Message, tvb, ie_offset, ie_len, FALSE);
		l3_tvb = tvb_new_subset(tvb, ie_offset,ie_len, ie_len );
		if  (!dissector_try_port(bssap_pdu_type_table,BSSAP_PDU_TYPE_DTAP, l3_tvb, pinfo, urr_ie_tree))
		   		call_dissector(data_handle, l3_tvb, pinfo, urr_ie_tree);
		break;
	case 27:		
		/* Channel Mode 
		 * The rest of the IE is coded as in [TS 44.018], not including IEI and length, if present
		 */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_channel_mode, tvb, ie_offset, ie_len, FALSE);
		break;
	case 28:		
		/* Mobile Station Classmark 2 
		 * The rest of the IE is coded as in [TS 24.008], not including IEI and length, if present
		 */
		/* Revision level bit 7 6 */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_MSC2_rev, tvb, ie_offset, 1, FALSE);
		/* ES IND (octet 3, bit 5) "Controlled Early Classmark Sending" option implementation */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_ES_IND, tvb, ie_offset, 1, FALSE);
		/* A5/1 algorithm supported (octet 3, bit 4) */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_A5_1_algorithm_sup, tvb, ie_offset, 1, FALSE);
		/* RF Power Capability (Octet 3) */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_RF_power_capability, tvb, ie_offset, 1, FALSE);
		ie_offset++;
		/* PS capability (pseudo-synchronization capability) (octet 4) */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_ps_sup_cap, tvb, ie_offset, 1, FALSE);
		/* SS Screening Indicator (octet 4)defined in 3GPP TS 24.080 */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_SS_screening_indicator, tvb, ie_offset, 1, FALSE);
		/* SM capability (MT SMS pt to pt capability) (octet 4)*/
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_SM_capability, tvb, ie_offset, 1, FALSE);
		/* VBS notification reception (octet 4) */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_VBS_notification_rec, tvb, ie_offset, 1, FALSE);
		/*VGCS notification reception (octet 4)*/
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_VGCS_notification_rec, tvb, ie_offset, 1, FALSE);
		/* FC Frequency Capability (octet 4 ) */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_FC_frequency_cap, tvb, ie_offset, 1, FALSE);
		ie_offset++;
		/* CM3 (octet 5, bit 8) */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_CM3, tvb, ie_offset, 1, FALSE);
		/* LCS VA capability (LCS value added location request notification capability) (octet 5,bit 6) */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_LCS_VA_cap, tvb, ie_offset, 1, FALSE);
		/* UCS2 treatment (octet 5, bit 5) */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_UCS2_treatment, tvb, ie_offset, 1, FALSE);
		/* SoLSA (octet 5, bit 4) */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_SoLSA, tvb, ie_offset, 1, FALSE);
		/* CMSP: CM Service Prompt (octet 5, bit 3) */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_CMSP, tvb, ie_offset, 1, FALSE);
		/* A5/3 algorithm supported (octet 5, bit 2) */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_A5_3_algorithm_sup, tvb, ie_offset, 1, FALSE);
		/* A5/2 algorithm supported (octet 5, bit 1) */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_A5_2_algorithm_sup, tvb, ie_offset, 1, FALSE);
		break;
	case 29:		
		/* RR Cause
		 * The rest of the IE is coded as in [TS 44.018], not including IEI and length, if present
		 */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_RR_cause, tvb, ie_offset, 1, FALSE);
		break;
	case 30:		
		/* Cipher Mode Setting
		 * Note: The coding of fields SC and algorithm identifier is defined in [44.018] 
		 * as part of the Cipher Mode Setting IE.
		 */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_sc, tvb, ie_offset, 1, FALSE);
		octet = tvb_get_guint8(tvb,ie_offset);
		if ( (octet & 1) == 1){ /* Start ciphering */
			/* algorithm identifier */
			proto_tree_add_item(urr_ie_tree, hf_uma_urr_algorithm_id, tvb, ie_offset, 1, FALSE);
		}
		break;
	case 31:		
		/* GPRS Resumption 
		 * If the target RAT is GERAN, the rest of the IE is coded as HANDOVER COMMAND message in [TS 44.018]
		 */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_GPRS_resumption, tvb, ie_offset, 1, FALSE);
		break;
	case 32:		
		/* Handover From UMAN Command 
		 * If the target RAT is GERAN, the rest of the IE is coded as HANDOVER COMMAND message in [TS 44.018]
		 *
		 * If the target RAT is UTRAN, the rest of the IE is coded as
		 * HANDOVER TO UTRAN COMMAND message in [TS 25.331].
		 */
		break;
	case 33:		/* UL Quality Indication */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_ULQI, tvb, ie_offset, 1, FALSE);
		break;
	case 34:		
		/* TLLI 
		 * The rest of the IE is coded as in [TS 44.018], not including IEI and length, if present.
		 * [TS 44.018]:
		 * The TLLI is encoded as a binary number with a length of 4 octets. TLLI is defined in 3GPP TS 23.003
		 */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_TLLI, tvb, ie_offset, 1, FALSE);
		break;
	case 35:		
		/* Packet Flow Identifier 
		 * The rest of the IE is coded as in [TS 24.008], not including IEI and length, if present.
		 */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_packet_flow_id, tvb, ie_offset, 1, FALSE);
		break;
	case 36:		
		/* Suspension Cause 
		 * The rest of the IE is coded as in [TS 44.018], not including IEI and length, if present.
		 */		
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_suspension_cause, tvb, ie_offset, 1, FALSE);
		break;
	case 37:		/* TU3920 Timer */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_TU3920_timer, tvb, ie_offset, 2, FALSE);
		break;
	case 38:		
		/* QoS 
		 * The rest of the IE is coded as QoS Profile IE in [TS 48.018], not including IEI and length, if present.
		 * octet 3-4	Peak bit rate provided by the network, coded as the Bucket Leak Rate "R value" part, 
		 * see sub-clause 11.3.4 (note)
		 * NOTE:	The bit rate 0 (zero) shall mean "best effort" in this IE.
		 * The R field is the binary encoding of the rate information expressed in 100 bits/s increments, 
		 * starting from 0 x 100 bits/s until 65 535 x 100 bits/s (6 Mbps).
		 */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_rate, tvb, ie_offset, 1, FALSE);
		ie_offset = ie_offset + 2;
		/*
		 * octet 5	SPARE	C/R	T	A	Precedence 
		 */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_precedence, tvb, ie_offset, 1, FALSE);
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_a_bit, tvb, ie_offset, 1, FALSE);
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_t_bit, tvb, ie_offset, 1, FALSE);
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_c_r_bit, tvb, ie_offset, 1, FALSE);
		break;
	case 39:		/* URLC Cause */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_URLCcause, tvb, ie_offset, 1, FALSE);
		break;
	case 40:		/* User Data Rate */
		udr = tvb_get_ntoh24(tvb, ie_offset) * 100;
		proto_tree_add_uint(urr_ie_tree, hf_uma_urr_udr , tvb, ie_offset, 3, udr );
		break;
	case 41:		
		/* Routing Area Code
		 * The rest of the IE is coded as in [TS 23.003] not including IEI and length, if present.
		 */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_RAC, tvb, ie_offset, 1, FALSE);
		break;
	case 42:		
		/* AP Location
		 * The rest of the IE is coded as in [GEOPRIV], not including IEI and length, if present
		 * http://www.ietf.org/internet-drafts/draft-ietf-geopriv-dhcp-civil-05.txt
		 */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_ap_location, tvb, ie_offset, ie_len, FALSE);
		break;
	case 43:		/* TU4001 Timer */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_TU4001_timer, tvb, ie_offset, 2, FALSE);
		break;
	case 44:		/* Location Status */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_LS, tvb, ie_offset, 1, FALSE);
		break;
	case 45:		/* Cipher Response */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_cipher_res, tvb, ie_offset, 1, FALSE);
		break;
	case 46:		/* Ciphering Command RAND */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_rand_val, tvb, ie_offset, ie_len, FALSE);
		break;
	case 47:		/* Ciphering Command MAC (Message Authentication Code)*/
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_ciphering_command_mac, tvb, ie_offset, ie_len, FALSE);
		break;
	case 48:		/* Ciphering Key Sequence Number */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_ciphering_key_seq_num, tvb, ie_offset, 1, FALSE);
		break;
	case 49:		/* SAPI ID */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_sapi_id, tvb, ie_offset, 1, FALSE);
		break;
	case 50:		/* Establishment Cause */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_establishment_cause, tvb, ie_offset, 1, FALSE);
		break;
	case 51:		/* Channel Needed */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_channel, tvb, ie_offset, 1, FALSE);
		break;
	case 52:		/* PDU in Error */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_PDU_in_error, tvb, ie_offset, ie_len, FALSE);
		break;
	case 53:		
		/* Sample Size 
		 */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_sample_size, tvb, ie_offset, 1, FALSE);
		break;
	case 54:		
		/* Payload Type 
		 * Payload Type (octet 3) Allowed values are between 96 and 127.
		 */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_payload_type, tvb, ie_offset, 1, FALSE);
		break;
	case 55:		
		/* Multi-rate Configuration 
		 * The rest of the IE is coded as in [TS 44.018], not including IEI and length, if present
		 * Octet 3 Multirate speech version	NSCB	ICMI	spare	Start mode
		 */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_multirate_speech_ver, tvb, ie_offset, 1, FALSE);
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_NCSB, tvb, ie_offset, 1, FALSE);
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_ICMI, tvb, ie_offset, 1, FALSE);
		/* The initial codec mode is coded as in 3GPP TS 45.009 */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_start_mode, tvb, ie_offset, 1, FALSE);
		octet = ( tvb_get_guint8(tvb,ie_offset) &0xe0 ) >> 5;
		ie_offset++;
		switch ( octet){
		case 1:
			/* Adaptive Multirate speech version 1 */
			/* Set of AMR codec modes */
			break;
		case 2:
			/* Adaptive Multirate speech version 2 */
			break;
		default:
			proto_tree_add_text(urr_ie_tree,tvb,ie_offset,ie_len,"Unknown version");
			break;
		}
		ie_offset++;
		break;
	case 56:		
		/* Mobile Station Classmark 3 
		 * The rest of the IE is coded as in [TS 24.008], not including IEI and length, if present
		 */
		break;
	case 57:		
		/* LLC-PDU 
		 * The rest of the IE is coded as in [TS 48.018], not including IEI and length, if present
		 */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_LLC_PDU, tvb, ie_offset, ie_len, FALSE);
		llc_tvb = tvb_new_subset(tvb, ie_offset,ie_len, ie_len );
		  if (llc_handle) {
			  if (check_col(pinfo->cinfo, COL_PROTOCOL)) {
					col_append_str(pinfo->cinfo, COL_PROTOCOL, "/");
					col_set_fence(pinfo->cinfo, COL_PROTOCOL);
			  }

			  call_dissector(llc_handle, llc_tvb, pinfo, urr_ie_tree);
		  }else{
			  if (data_handle)
				  call_dissector(data_handle, llc_tvb, pinfo, urr_ie_tree);
		  }
		break;
	case 58:		/* Location Black List indicator */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_LBLI, tvb, ie_offset, 1, FALSE);
		break;
	case 59:		/* Reset Indicator */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_RI, tvb, ie_offset, 1, FALSE);
		break;
	case 60:		/* TU4003 Timer */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_TU4003_timer, tvb, ie_offset, 2, FALSE);
		break;
	case 61:
		/* AP Service Name */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_ap_service_name_type, tvb, ie_offset, 1, FALSE);
		ie_offset++;
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_ap_Service_name_value, tvb, ie_offset, ie_len -1, FALSE);
		break;
	case 62:
		/* UMA Service Zone Information 
		 * UMA Service Zone Icon Indicator, octet 3
		 */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_uma_service_zone_icon_ind, tvb, ie_offset, 1, FALSE);
		ie_offset++;
		/* Length of UMA Service Zone string */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_uma_service_zone_str_len, tvb, ie_offset, 1, FALSE);
		str_len = tvb_get_guint8(tvb,ie_offset);
		ie_offset++;
		/* UMA Service Zone string, 1st character */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_uma_service_zone_str, tvb, ie_offset, str_len, FALSE);
		break;
	case 63:
		/* RTP Redundancy Configuration */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_window_size, tvb, ie_offset, 1, FALSE);
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_uma_codec_mode, tvb, ie_offset, 1, FALSE);
		break;
	case 64:
		/* UTRAN Classmark 
		 * The rest of the IE is the INTER RAT HANDOVER INFO coded as in
		 * [TS 25.331], not including IEI and length, if present
		 */
		break;
	case 65:
		/* Classmark Enquiry Mask 
		 * The rest of the IE is the Classmark Enquiry Mask coded as in [TS 44.018], not including IEI and length, if present
		 */
		break;
	case 66:
		/* UTRAN Cell Identifier List 
		 * UTRAN Cell Identification Discriminator
		 */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_UTRAN_cell_id_disc, tvb, ie_offset, 1, FALSE);
		octet = tvb_get_guint8(tvb,ie_offset);
		ie_offset++;
		if ( octet == 0 ){
			ie_offset = dissect_mcc_mnc(tvb, urr_ie_tree, ie_offset);
			proto_tree_add_item(urr_ie_tree, hf_uma_urr_lac, tvb, ie_offset, 2, FALSE);
			ie_offset = ie_offset + 2;
			/* The octets 9-12 are coded as shown in 3GPP TS 25.331, Table 'Cell identity'. */
		}
		break;
	case 67:
		/* Serving UNC table indicator */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_suti, tvb, ie_offset, 1, FALSE);
		break;
	case 68:
		/* Registration indicators */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_uma_mps, tvb, ie_offset, 1, FALSE);
		break;
	case 69:
		/* UMA PLMN List */
		octet = tvb_get_guint8(tvb,ie_offset);
		proto_tree_add_uint(urr_ie_tree, hf_uma_urr_num_of_plms , tvb, ie_offset, 1, octet);
		/* TODO insert while loop here */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_lac, tvb, ie_offset, 2, FALSE);
		break;
	case 70:
		/* Received Signal Level List */
		break;
	case 96:		/* MS Radio Identity */
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_radio_type_of_id, tvb, ie_offset, 1, FALSE);
		octet = tvb_get_guint8(tvb,ie_offset);
		if (( octet & 0xf) == 0){ /* IEEE MAC-address format */
			ie_offset++;
			haddr = tvb_get_ptr(tvb, ie_offset, ie_len);
			proto_tree_add_ether(urr_ie_tree, hf_uma_urr_ms_radio_id, tvb, ie_offset, ie_len, haddr);
		}else{
			proto_tree_add_text(urr_ie_tree, tvb, ie_offset, ie_len,"Unknown format");
		}
		break;

	case 97:		
		/* UNC IP Address 
		 * IP Address type
		 */
		octet = tvb_get_guint8(tvb,ie_offset);
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_IP_Address_type, tvb, ie_offset, 1, FALSE);
		if (ie_len > 4 )
		ie_offset++;
		if ( octet == 0x57 ){ /* IPv6 */

		}else{ /* All other values shall be interpreted as Ipv4 address in this version of the protocol.*/
			tvb_memcpy(tvb, (guint8 *)&unc_ipv4_address, ie_offset, 4);
			proto_tree_add_ipv4(urr_ie_tree, hf_uma_urr_unc_ipv4, tvb, ie_offset, 4, unc_ipv4_address);
			rtp_ipv4_address = unc_ipv4_address;

		}
		break;
	case 98:		/* UNC Fully Qualified Domain/Host Name */
		if ( ie_len > 0){
			string = tvb_get_string(tvb, ie_offset, ie_len);
			proto_tree_add_string(urr_ie_tree, hf_uma_unc_FQDN, tvb, ie_offset, ie_len, string);
			g_free(string);
		}else{
			proto_tree_add_text(urr_ie_tree,tvb,offset,1,"UNC FQDN not present");
		}
		break;
	case 99:		
		/* IP address for GPRS user data transport 
		 * IP Address type
		 */
		octet = tvb_get_guint8(tvb,ie_offset);
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_IP_Address_type, tvb, ie_offset, 1, FALSE);
		ie_offset++;
		if ( octet == 0x57 ){ /* IPv6 */

		}else{ /* All other values shall be interpreted as Ipv4 address in this version of the protocol.*/
			tvb_memcpy(tvb, (guint8 *)&GPRS_user_data_ipv4_address, ie_offset, 4);
			proto_tree_add_ipv4(urr_ie_tree, hf_uma_urr_GPRS_user_data_transport_ipv4, tvb, ie_offset, 4, GPRS_user_data_ipv4_address);

		}
		break;
	case 100:		/* UDP Port for GPRS user data transport */
		GPRS_user_data_transport_UDP_port = tvb_get_ntohs(tvb,ie_offset);
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_GPRS_port, tvb, ie_offset, 2, FALSE);
		/*
		 * If this isn't the first time this packet has been processed,
		 * we've already done this work, so we don't need to do it
		 * again.
		 */
		if (pinfo->fd->flags.visited)
		{
			break;
		}
		SET_ADDRESS(&null_addr, AT_NONE, 0, NULL);

		dst_addr.type=AT_IPv4;
		dst_addr.len=4;
		dst_addr.data=(guint8 *)&GPRS_user_data_ipv4_address;

		conversation = find_conversation(pinfo->fd->num,&dst_addr,
			&null_addr, PT_UDP, GPRS_user_data_transport_UDP_port,
			0, NO_ADDR_B|NO_PORT_B);

		if (conversation == NULL) {
			/* It's not part of any conversation - create a new one. */
			conversation = conversation_new(pinfo->fd->num, &dst_addr,
			    &null_addr, PT_UDP,GPRS_user_data_transport_UDP_port ,
			    0, NO_ADDR2|NO_PORT2);

		/* Set dissector */
		conversation_set_dissector(conversation, uma_udp_handle);
		}

		break;
	case 103:		/* UNC TCP port */
		UNC_tcp_port = tvb_get_ntohs(tvb,ie_offset);
		proto_tree_add_uint(urr_ie_tree, hf_uma_urr_UNC_tcp_port , tvb, ie_offset, 2, UNC_tcp_port);

		/*
		 * If this isn't the first time this packet has been processed,
		 * we've already done this work, so we don't need to do it
		 * again.
		 */
		if (pinfo->fd->flags.visited)
		{
			break;
		}
		SET_ADDRESS(&null_addr, AT_NONE, 0, NULL);

		dst_addr.type=AT_IPv4;
		dst_addr.len=4;
		dst_addr.data=(guint8 *)&unc_ipv4_address;

		conversation = find_conversation(pinfo->fd->num,&dst_addr,
			&null_addr, PT_TCP, UNC_tcp_port,
			0, NO_ADDR_B|NO_PORT_B);

		if (conversation == NULL) {
			/* It's not part of any conversation - create a new one. */
			conversation = conversation_new(pinfo->fd->num, &dst_addr,
			    &null_addr, PT_TCP,UNC_tcp_port ,
			    0, NO_ADDR2|NO_PORT2);
			/* Set dissector */
			conversation_set_dissector(conversation, uma_tcp_handle);
		}

		break;
	case 104:		/* RTP UDP port */
		RTP_UDP_port = tvb_get_ntohs(tvb,ie_offset);
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_RTP_port, tvb, ie_offset, 2, FALSE);
		/* TODO find out exactly which element contains IP addr */
		/* Debug
		proto_tree_add_text(urr_ie_tree,tvb,ie_offset,ie_len,"IP %u, Port %u Handle %u",
			rtp_ipv4_address,RTP_UDP_port,rtp_handle);
			*/

		if((!pinfo->fd->flags.visited) && rtp_ipv4_address!=0 && RTP_UDP_port!=0 && rtp_handle){
			address src_addr;

			src_addr.type=AT_IPv4;
			src_addr.len=4;
			src_addr.data=(guint8 *)&rtp_ipv4_address;

			rtp_add_address(pinfo, &src_addr, RTP_UDP_port, 0, "UMA", pinfo->fd->num, 0);
			if ((RTP_UDP_port & 0x1) == 0){ /* Even number RTP port RTCP should follow on odd number */
				RTCP_UDP_port = RTP_UDP_port + 1;
				rtcp_add_address(pinfo, &src_addr, RTCP_UDP_port, 0, "UMA", pinfo->fd->num);
			}
		}
		break;
	case 105:		/* RTCP UDP port */
		RTCP_UDP_port = tvb_get_ntohs(tvb,ie_offset);
		proto_tree_add_item(urr_ie_tree, hf_uma_urr_RTCP_port, tvb, ie_offset, 2, FALSE);
		/* TODO find out exactly which element contains IP addr */
		if((!pinfo->fd->flags.visited) && rtcp_ipv4_address!=0 && RTCP_UDP_port!=0 && rtcp_handle){
			address src_addr;

			src_addr.type=AT_IPv4;
			src_addr.len=4;
			src_addr.data=(guint8 *)&rtcp_ipv4_address;

			rtcp_add_address(pinfo, &src_addr, RTCP_UDP_port, 0, "UMA", pinfo->fd->num);
		}
		break;
	default:
		proto_tree_add_text(urr_ie_tree,tvb,ie_offset,ie_len,"DATA");
		break;
	}
	offset = offset + ie_len;
	return offset;
}
static int
dissect_uma(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	int		offset = 0;
	guint8	octet, pd;
	guint16 msg_len;

/* Set up structures needed to add the protocol subtree and manage it */
	proto_item *ti;
	proto_tree *uma_tree;

/* Make entries in Protocol column and Info column on summary display */
	if (check_col(pinfo->cinfo, COL_PROTOCOL)) 
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "UMA");
	if (check_col(pinfo->cinfo, COL_INFO))
		col_clear(pinfo->cinfo, COL_INFO);

		ti = proto_tree_add_item(tree, proto_uma, tvb, 0, -1, FALSE);
		uma_tree = proto_item_add_subtree(ti, ett_uma);

/* add an item to the subtree, see section 1.6 for more information */
		msg_len = tvb_get_ntohs(tvb,offset);
		proto_tree_add_item(uma_tree, hf_uma_length_indicator, tvb, offset, 2, FALSE);
		offset = offset + 2;
		octet = tvb_get_guint8(tvb,offset);
		pd = octet & 0x0f;
		proto_tree_add_item(uma_tree, hf_uma_skip_ind, tvb, offset, 1, FALSE);
		if ((octet & 0xf0) != 0 ){
			proto_tree_add_text(uma_tree, tvb,offset,-1,"Skipp this message");
			return tvb_length(tvb);
		}
			
		proto_tree_add_item(uma_tree, hf_uma_pd, tvb, offset, 1, FALSE);
		switch  ( pd ){
		case 1: /* URR */
			offset++;
			octet = tvb_get_guint8(tvb,offset);
			proto_tree_add_item(uma_tree, hf_uma_urr_msg_type, tvb, offset, 1, FALSE);
			if (check_col(pinfo->cinfo, COL_INFO))
				col_add_fstr(pinfo->cinfo, COL_INFO, "%s",val_to_str(octet, uma_urr_msg_type_vals, "Unknown URR (%u)"));
			while ((msg_len + 1) > offset ){
				offset++;
				offset = dissect_uma_IE(tvb, pinfo, uma_tree, offset);
			}
			return offset;
			break;
		case 2:	/* URLC */
			offset++;
			octet = tvb_get_guint8(tvb,offset);
			proto_tree_add_item(uma_tree, hf_uma_urlc_msg_type, tvb, offset, 1, FALSE);
			if (check_col(pinfo->cinfo, COL_INFO)){
				col_add_fstr(pinfo->cinfo, COL_INFO, "%s ",val_to_str(octet, uma_urlc_msg_type_vals, "Unknown URLC (%u)"));
				col_set_fence(pinfo->cinfo,COL_INFO);
			}
			offset++;
			proto_tree_add_item(uma_tree, hf_uma_urlc_TLLI, tvb, offset, 4, FALSE);
			offset = offset + 3;
			while ((msg_len + 1) > offset ){
				offset++;
				offset = dissect_uma_IE(tvb, pinfo, uma_tree, offset);
			}
			return offset;
			break;
		default:
			proto_tree_add_text(uma_tree, tvb,offset,-1,"Unknown protocol %u",pd);
			return tvb_length(tvb);

		}

	return offset;
}

static int
dissect_uma_urlc_udp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{

	int		offset = 0;
	guint8	octet;
	guint16 msg_len;

/* Set up structures needed to add the protocol subtree and manage it */
	proto_item *ti;
	proto_tree *uma_tree;

/* Make entries in Protocol column and Info column on summary display */
	if (check_col(pinfo->cinfo, COL_PROTOCOL)) 
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "UMA");
	if (check_col(pinfo->cinfo, COL_INFO))
		col_clear(pinfo->cinfo, COL_INFO);

	ti = proto_tree_add_item(tree, proto_uma, tvb, 0, -1, FALSE);
	uma_tree = proto_item_add_subtree(ti, ett_uma);

	octet = tvb_get_guint8(tvb,offset);
	proto_tree_add_item(uma_tree, hf_uma_urlc_msg_type, tvb, offset, 1, FALSE);
	if (check_col(pinfo->cinfo, COL_INFO)){
		col_add_fstr(pinfo->cinfo, COL_INFO, "%s ",val_to_str(octet, uma_urlc_msg_type_vals, "Unknown URLC (%u)"));
		col_set_fence(pinfo->cinfo,COL_INFO);
	}
	msg_len = tvb_length_remaining(tvb,offset) - 1;

	switch  ( octet ){

	case 2:	/* RLC UNITDATA */
	case 6: /* URLC-UFC-REQ */
	case 7: /* URLC-DFC-REQ only allowed message types*/
		offset++;
		proto_tree_add_item(uma_tree, hf_uma_urlc_TLLI, tvb, offset, 4, FALSE);
		offset = offset + 4;
		proto_tree_add_item(uma_tree, hf_uma_urlc_seq_nr, tvb, offset, 2, FALSE);
		offset++;
		while (msg_len > offset){
			offset++;
			offset = dissect_uma_IE(tvb, pinfo, uma_tree, offset);
		}
		return offset;
		break;
	default:
		proto_tree_add_text(uma_tree, tvb,offset,-1,"Wrong message type %u",octet);
		return tvb_length(tvb);

	}

}


/* Register the protocol with Ethereal */
/* If this dissector uses sub-dissector registration add a registration routine.
   This format is required because a script is used to find these routines and
   create the code that calls these routines.
*/
void
proto_reg_handoff_uma(void)
{
	static int Initialized=FALSE;
	static int TcpPort1=0;
	
	if (!Initialized) {
		uma_tcp_handle = new_create_dissector_handle(dissect_uma, proto_uma);
		uma_udp_handle = new_create_dissector_handle(dissect_uma_urlc_udp, proto_uma);
		dissector_add("tcp.port", 0, uma_udp_handle);
		Initialized=TRUE;
	} else {
		dissector_delete("tcp.port", TcpPort1, uma_tcp_handle);
	}

	/* set port for future deletes */
	TcpPort1=gbl_umaTcpPort1;
	dissector_add("tcp.port", gbl_umaTcpPort1, uma_tcp_handle);
	data_handle = find_dissector("data");
	rtp_handle = find_dissector("rtp");
	rtcp_handle = find_dissector("rtcp");
	llc_handle = find_dissector("llcgprs");


}

/* this format is require because a script is used to build the C function
   that calls all the protocol registration.
*/

void
proto_register_uma(void)
{                 

	module_t *uma_module;

/* Setup list of header fields  See Section 1.6.1 for details*/
	static hf_register_info hf[] = {
		{ &hf_uma_length_indicator,
			{ "Length Indicator","uma.li",
			FT_UINT16, BASE_DEC, NULL, 0x0,          
			"Length Indicator", HFILL }
		},
		{ &hf_uma_pd,
			{ "Protocol Discriminator","uma.pd",
			FT_UINT8, BASE_DEC, VALS(uma_pd_vals), 0x0f,          
			"Protocol Discriminator", HFILL }
		},
		{ &hf_uma_skip_ind,
			{ "Skip Indicator",           "uma.skip.ind",
			FT_UINT8, BASE_DEC, NULL, 0xf0,          
			"Skip Indicator", HFILL }
		},
		{ &hf_uma_urr_msg_type,
			{ "URR Message Type", "uma.urr.msg.type",
			FT_UINT16, BASE_DEC, VALS(uma_urr_msg_type_vals), 0x0,          
			"URR Message Type", HFILL }
		},
		{ &hf_uma_urlc_msg_type,
			{ "URLC Message Type", "uma.urlc.msg.type",
			FT_UINT8, BASE_DEC, VALS(uma_urlc_msg_type_vals), 0x0,          
			"URLC Message Type", HFILL }
		},
		{ &hf_uma_urlc_TLLI,
			{ "Temporary Logical Link Identifier","uma.urlc.tlli",
			FT_BYTES,BASE_DEC,  NULL, 0x0,          
			"Temporary Logical Link Identifier", HFILL }
		},
		{ &hf_uma_urlc_seq_nr,
			{ "Sequence Number","uma.urlc.seq.nr",
			FT_BYTES,BASE_DEC,  NULL, 0x0,          
			"Sequence Number", HFILL }
		},
		{ &hf_uma_urr_IE,
			{ "URR Information Element","uma.urr.ie.type",
			FT_UINT8, BASE_DEC, VALS(uma_urr_IE_type_vals), 0x0,          
			"URR Information Element", HFILL }
		},
		{ &hf_uma_urr_IE_len,
			{ "URR Information Element length","uma.urr.ie.len",
			FT_UINT8, BASE_DEC, NULL, 0x0,          
			"URR Information Element length", HFILL }
		},
		{ &hf_uma_urr_IE_len2,
			{ "URR Information Element length","uma.urr.ie.len2",
			FT_UINT16, BASE_DEC, NULL, 0x7fff,          
			"URR Information Element length", HFILL }
		},
		{ &hf_uma_urr_mobile_identity_type,
			{ "Mobile Identity Type","uma.urr.ie.mobileid.type",
			FT_UINT8, BASE_DEC, VALS(uma_urr_mobile_identity_type_vals), 0x07,          
			"Mobile Identity Type", HFILL }
		},
		{ &hf_uma_urr_odde_even_ind,
			{ "Odd/even indication","uma.urr.oddevenind",
			FT_UINT8, BASE_DEC, uma_urr_oddevenind_vals, 0x08,          
			"Mobile Identity", HFILL }
		},
		{ &hf_uma_urr_imsi,
			{ "IMSI", "uma_urr.imsi",
			FT_STRING, BASE_NONE, NULL, 0,
			"IMSI", HFILL }
		},
		{ &hf_uma_urr_imei,
			{ "IMEI", "uma_urr.imei",
			FT_STRING, BASE_NONE, NULL, 0,
			"IMEI", HFILL }
		},
		{ &hf_uma_urr_imeisv,
			{ "IMEISV", "uma_urr.imeisv",
			FT_STRING, BASE_NONE, NULL, 0,
			"IMEISV", HFILL }
		},
		{ &hf_uma_urr_tmsi_p_tmsi,
			{ "TMSI/P-TMSI", "uma_urr.tmsi_p_tmsi",
			FT_STRING, BASE_NONE, NULL, 0,
			"TMSI/P-TMSI", HFILL }
		},
		{ &hf_uma_urr_uri,
			{ "UMA Release Indicator (URI)","uma.urr.uri",
			FT_UINT8, BASE_DEC, NULL, 0x07,          
			"URI", HFILL }
		},
		{ &hf_uma_urr_radio_type_of_id,
			{ "Type of identity","uma.urr.radio_type_of_id",
			FT_UINT8, BASE_DEC, VALS(radio_type_of_id_vals), 0x0f,          
			"Type of identity", HFILL }
		},
		{ &hf_uma_urr_radio_id,
			{ "Radio Identity","uma.urr.radio_id",
			FT_ETHER, BASE_DEC, NULL, 0x00,          
			"Radio Identity", HFILL }
		},

		{ &hf_uma_urr_cell_id,
			{ "Cell Identity","uma.urr.cell_id",
			FT_UINT16, BASE_DEC, NULL, 0x00,          
			"Cell Identity", HFILL }
		},

		{ &hf_uma_urr_mcc,
			{ "Mobile Country Code","uma.urr.mcc",
			FT_UINT16, BASE_DEC, NULL, 0x0,          
			"Mobile Country Code", HFILL }
		},
		{ &hf_uma_urr_mnc,
			{ "Mobile network code","uma.urr.mnc",
			FT_UINT16, BASE_DEC, NULL, 0x0,          
			"Mobile network code ", HFILL }
		},
		{ &hf_uma_urr_lac,
			{ "Location area code","uma.urr.lac",
			FT_UINT16, BASE_DEC, NULL, 0x0,          
			"Location area code", HFILL }
		},
		{ &hf_uma_urr_gci,
			{ "GCI, GSM Coverage Indicator","uma.urr.gci",
			FT_UINT8, BASE_DEC,  VALS(gci_vals), 0x0,          
			"GCI, GSM Coverage Indicator", HFILL }
		},
		{ &hf_uma_urr_tura,
			{ "TURA, Type of Unlicensed Radio","uma.urr.tura",
			FT_UINT8,BASE_DEC,  VALS(tura_vals), 0x3,          
			"TURA, Type of Unlicensed Radio", HFILL }
		},
		{ &hf_uma_urr_tlra,
			{ "TLRA, Type of Licensed Radio","uma.urr.tlra",
			FT_UINT8,BASE_DEC, VALS(tlra_vals), 0xc,          
			"TLRA, Type of Licensed Radio", HFILL }
		},
		{ &hf_uma_urr_rrs,
			{ "RTP Redundancy Support(RRS)","uma.urr.rrs",
			FT_UINT8,BASE_DEC, VALS(rrs_vals), 0xc,          
			"RTP Redundancy Support(RRS)", HFILL }
		},
		{ &hf_uma_urr_location_estimate,
			{ "Location estimate","uma.urr.location_estimate",
			FT_UINT8,BASE_DEC, VALS(type_of_shape_vals), 0xf,          
			"Location estimate", HFILL }
		},
		{ &hf_uma_urr_IP_Address_type,
			{ "IP address type number value","uma.urr.ip_type",
			FT_UINT8,BASE_DEC,  VALS(IP_address_type_vals), 0x0,          
			"IP address type number value", HFILL }
		},
		{ &hf_uma_urr_FQDN,
		       { "Fully Qualified Domain/Host Name (FQDN)", "uma.urr.fqdn",
		       FT_STRING, BASE_NONE,NULL,0x0,
			"Fully Qualified Domain/Host Name (FQDN)", HFILL }
		},
		{ &hf_uma_urr_sgw_ipv4,
			{ "SGW IPv4 address","uma.urr.sgwipv4",
			FT_IPv4,BASE_NONE,  NULL, 0x0,          
			"SGW IPv4 address", HFILL }
		},
		{ &hf_uma_urr_redirection_counter,
			{ "Redirection Counter","uma.urr.redirection_counter",
			FT_UINT8,BASE_DEC,  NULL, 0x0,          
			"Redirection Counter", HFILL }
		},
		{ &hf_uma_urr_dis_rej_cau,
			{ "Discovery Reject Cause","uma.urr.is_rej_cau",
			FT_UINT8,BASE_DEC,  VALS(discovery_reject_cause_vals), 0x0,          
			"Discovery Reject Cause", HFILL }
		},
		{ &hf_uma_urr_EC,
			{ "EC Emergency Call allowed","uma.urr.EC",
			FT_UINT8,BASE_DEC,  VALS(EC_vals), 0x1,          
			"EC Emergency Call allowed", HFILL }
		},
		{ &hf_uma_urr_ECMC,
			{ "ECMC, Early Classmark Sending Control","uma.urr.is_rej_cau",
			FT_UINT8,BASE_DEC,  VALS(ECMC_vals), 0x2,          
			"ECMC, Early Classmark Sending Control", HFILL }
		},
		{ &hf_uma_urr_NMO,
			{ "NMO, Network Mode of Operation","uma.urr.NMO",
			FT_UINT8,BASE_DEC,  VALS(NMO_vals), 0xc,          
			"NMO, Network Mode of Operation", HFILL }
		},
		{ &hf_uma_urr_GPRS,
			{ "GPRS, GPRS Availability","uma.urr.is_rej_cau",
			FT_UINT8,BASE_DEC,  VALS(GPRS_avail_vals), 0x10,          
			"GPRS, GPRS Availability", HFILL }
		},
		{ &hf_uma_urr_DTM,
			{ "DTM, Dual Transfer Mode of Operation by network","uma.urr.dtm",
			FT_UINT8,BASE_DEC,  VALS(DTM_vals), 0x20,          
			"DTM, Dual Transfer Mode of Operation by network", HFILL }
		},
		{ &hf_uma_urr_ATT,
			{ "ATT, Attach-detach allowed","uma.urr.att",
			FT_UINT8,BASE_DEC,  VALS(ATT_vals), 0x40,          
			"ATT, Attach-detach allowed", HFILL }
		},
		{ &hf_uma_urr_MSCR,
			{ "MSCR, MSC Release","uma.urr.mscr",
			FT_UINT8,BASE_DEC,  VALS(MSCR_vals), 0x80,          
			"MSCR, MSC Release", HFILL }
		},
		{ &hf_uma_urr_T3212_timer,
			{ "T3212 Timer value(seconds)","uma.urr.t3212",
			FT_UINT8,BASE_DEC,  NULL, 0x0,          
			"T3212 Timer value(seconds)", HFILL }
		},
		{ &hf_uma_urr_RAC,
			{ "Routing Area Code","uma.urr.rac",
			FT_UINT8,BASE_DEC,  NULL, 0x0,          
			"Routing Area Code", HFILL }
		},
		{ &hf_uma_urr_ap_location,
			{ "AP Location","uma.urr.ap_location",
			FT_BYTES,BASE_HEX,  NULL, 0x0,          
			"AP Location", HFILL }
		},
		{ &hf_uma_urr_SGSNR,
			{ "SGSN Release","uma.urr.SGSNR",
			FT_UINT8,BASE_DEC,  VALS(SGSNR_vals), 0x01,          
			"SGSN Release", HFILL }
		},
		{ &hf_uma_urr_ECMP,
			{ "ECMP, Emergency Call Mode Preference","uma.urr.ECMP",
			FT_UINT8,BASE_DEC,  VALS(ECMP_vals), 0x02,          
			"ECMP, Emergency Call Mode Preference", HFILL }
		},
		{ &hf_uma_urr_RE,
			{ "RE, Call reestablishment allowed","uma.urr.RE",
			FT_UINT8,BASE_DEC,  VALS(RE_vals), 0x04,          
			"RE, Call reestablishment allowed", HFILL }
		},
		{ &hf_uma_urr_PFCFM,
			{ "PFCFM, PFC_FEATURE_MODE","uma.urr.PFCFM",
			FT_UINT8,BASE_DEC,  VALS(PFCFM_vals), 0x08,          
			"PFCFM, PFC_FEATURE_MODE", HFILL }
		},
		{ &hf_uma_urr_3GECS,
			{ "3GECS, 3G Early Classmark Sending Restriction","uma.urr.3GECS",
			FT_UINT8,BASE_DEC,  VALS(Three_GECS_vals), 0x10,          
			"3GECS, 3G Early Classmark Sending Restriction", HFILL }
		},
		{ &hf_uma_urr_cell_id_disc,
			{ "Cell identification discriminator","uma.urr.cell_id_disc",
			FT_UINT8,BASE_DEC,  VALS(cell_id_disc_vals), 0x0f,          
			"Cell identificationdiscriminator", HFILL }
		},
		{ &hf_uma_urr_ci,
			{ "Cell identification ","uma.urr.ci",
			FT_UINT16,BASE_DEC,  NULL, 0x0,          
			"Cell identification ", HFILL }
		},
		{ &hf_uma_urr_RNC_ID,
			{ "RNC-ID","uma.urr.rnc_id",
			FT_UINT16,BASE_DEC,  NULL, 0x0,          
			"RNC-ID", HFILL }
		},
		{ &hf_uma_urr_bcc,
			{ "BCC","uma.urr.bcc",
			FT_UINT8,BASE_DEC,  NULL, 0x07,          
			"BCC", HFILL }
		},
		{ &hf_uma_urr_ncc,
			{ "NCC","uma.urr.ncc",
			FT_UINT8,BASE_DEC,  NULL, 0x38,          
			"NCC", HFILL }
		},
		{ &hf_uma_urr_bcch_arfcn,
			{ "BCCH ARFCN(RF channel number)","uma.urr.bcch_arfcn",
			FT_UINT16,BASE_DEC,  NULL, 0x0,          
			"BCCH ARFCN", HFILL }
		},
		{ &hf_uma_urr_TU3907_timer,
			{ "TU3907 Timer value(seconds)","uma.urr.tu3907",
			FT_UINT16,BASE_DEC,  NULL, 0x0,          
			"TU3907 Timer value(seconds)", HFILL }
		},
		{ &hf_uma_urr_GSM_RR_state,
			{ "GSM RR State value","uma.urr.gsmrrstate",
			FT_UINT8,BASE_DEC,  VALS(GRS_GSM_RR_State_vals), 0x1,          
			"GSM RR State value", HFILL }
		},
		{ &hf_uma_urr_UMA_band,
			{ "UMA Band","uma.urr.umaband",
			FT_UINT8,BASE_DEC,  VALS(UMA_band_vals), 0x0f,          
			"UMA Band", HFILL }
		},
		{ &hf_uma_urr_URR_state,
			{ "URR State","uma.urr.state",
			FT_UINT8,BASE_DEC,  VALS(URR_state_vals), 0x03,          
			"URR State", HFILL }
		},
		{ &hf_uma_urr_register_reject_cause,
			{ "Register Reject Cause","uma.urr.state",
			FT_UINT8,BASE_DEC,  VALS(register_reject_cause_vals), 0x0,          
			"Register Reject Cause", HFILL }
		},
		{ &hf_uma_urr_TU3906_timer,
			{ "TU3907 Timer value(seconds)","uma.urr.tu3906",
			FT_UINT16,BASE_DEC,  NULL, 0x0,          
			"TU3906 Timer value(seconds)", HFILL }
		},
		{ &hf_uma_urr_TU3910_timer,
			{ "TU3907 Timer value(seconds)","uma.urr.tu3910",
			FT_UINT16,BASE_DEC,  NULL, 0x0,          
			"TU3910 Timer value(seconds)", HFILL }
		},
		{ &hf_uma_urr_TU3902_timer,
			{ "TU3902 Timer value(seconds)","uma.urr.tu3902",
			FT_UINT16,BASE_DEC,  NULL, 0x0,          
			"TU3902 Timer value(seconds)", HFILL }
		},
		{ &hf_uma_urr_communication_port,
			{ "Communication Port","uma.urr.communication_port",
			FT_UINT16,BASE_DEC,  NULL, 0x0,          
			"Communication Portt", HFILL }
		},
		{ &hf_uma_urr_L3_Message,
			{ "L3 message contents","uma.urr.l3",
			FT_BYTES,BASE_HEX,  NULL, 0x0,          
			"L3 message contents", HFILL }
		},
		{ &hf_uma_urr_L3_protocol_discriminator,
			{ "Protocol discriminator","uma.urr.L3_protocol_discriminator",
			FT_UINT8,BASE_DEC,  VALS(protocol_discriminator_vals), 0x0f,          
			"Protocol discriminator", HFILL }
		},
		{ &hf_uma_urr_channel_mode,
			{ "Channel Mode","uma.urr.channel_mode",
			FT_UINT8,BASE_DEC,  VALS(channel_mode_vals), 0x0,          
			"Channel Mode", HFILL }
		},
		{ &hf_uma_urr_MSC2_rev,
			{ "Revision Level","uma.urr.MSC2_rev",
			FT_UINT8,BASE_DEC,  VALS(gsm_a_msc_rev_vals), 0x60,          
			"Revision level", HFILL }
		},
		{ &hf_uma_urr_ES_IND,
			{ "ES IND","uma.urr.MSC2_rev",
			FT_UINT8,BASE_DEC,  VALS(ES_IND_vals), 0x20,          
			"ES IND", HFILL }
		},
		{ &hf_uma_urr_A5_1_algorithm_sup,
			{ "A5/1 algorithm supported","uma.urr.MSC2_rev",
			FT_UINT8,BASE_DEC,  VALS(A5_1_algorithm_sup_vals), 0x10,          
			"A5/1 algorithm supported ", HFILL }
		},
		{ &hf_uma_urr_RF_power_capability,
			{ "RF Power Capability","uma.urr.MSC2_rev",
			FT_UINT8,BASE_DEC,  VALS(RF_power_capability_vals), 0x07,          
			"RF Power Capability", HFILL }
		},
		{ &hf_uma_urr_ps_sup_cap,
			{ "PS capability (pseudo-synchronization capability)","uma.urr.ps_sup_cap",
			FT_UINT8,BASE_DEC,  VALS(ps_sup_cap_vals), 0x40,          
			"PS capability (pseudo-synchronization capability)", HFILL }
		},
		{ &hf_uma_urr_SS_screening_indicator,
			{ "SS Screening Indicator","uma.urr.SS_screening_indicator",
			FT_UINT8,BASE_DEC,  VALS(SS_screening_indicator_vals), 0x30,          
			"SS Screening Indicator", HFILL }
		},
		{ &hf_uma_urr_SM_capability,
			{ "SM capability (MT SMS pt to pt capability)","uma.urr.SM_cap",
			FT_UINT8,BASE_DEC,  VALS(SM_capability_vals), 0x08,          
			"SM capability (MT SMS pt to pt capability)", HFILL }
		},
		{ &hf_uma_urr_VBS_notification_rec,
			{ "VBS notification reception ","uma.urr.VBS_notification_rec",
			FT_UINT8,BASE_DEC,  VALS(VBS_notification_rec_vals), 0x04,          
			"VBS notification reception ", HFILL }
		},
		{ &hf_uma_urr_VGCS_notification_rec,
			{ "VGCS notification reception ","uma.urr.VGCS_notification_rec",
			FT_UINT8,BASE_DEC,  VALS(VGCS_notification_rec_vals), 0x02,          
			"VGCS notification reception", HFILL }
		},
		{ &hf_uma_urr_FC_frequency_cap,
			{ "FC Frequency Capability","uma.urr.FC_frequency_cap",
			FT_UINT8,BASE_DEC,  VALS(FC_frequency_cap_vals), 0x01,          
			"FC Frequency Capability", HFILL }
		},
		{ &hf_uma_urr_CM3,
			{ "CM3","uma.urr.CM3",
			FT_UINT8,BASE_DEC,  VALS(CM3_vals), 0x01,          
			"CM3", HFILL }
		},
		{ &hf_uma_urr_LCS_VA_cap,
			{ "LCS VA capability (LCS value added location request notification capability) ","uma.urr.LCS_VA_cap",
			FT_UINT8,BASE_DEC,  VALS(LCS_VA_cap_vals), 0x80,          
			"LCS VA capability (LCS value added location request notification capability) ", HFILL }
		},
		{ &hf_uma_urr_UCS2_treatment,
			{ "UCS2 treatment ","uma.urr.UCS2_treatment",
			FT_UINT8,BASE_DEC,  VALS(UCS2_treatment_vals), 0x10,          
			"UCS2 treatment ", HFILL }
		},
		{ &hf_uma_urr_SoLSA,
			{ "SoLSA","uma.urr.SoLSA",
			FT_UINT8,BASE_DEC,  VALS(SoLSA_vals), 0x08,          
			"SoLSA", HFILL }
		},
		{ &hf_uma_urr_CMSP,
			{ "CMSP: CM Service Prompt ","uma.urr.CMSP",
			FT_UINT8,BASE_DEC,  VALS(CMSP_vals), 0x04,          
			"CMSP: CM Service Prompt ", HFILL }
		},
		{ &hf_uma_urr_A5_3_algorithm_sup,
			{ "A5/3 algorithm supported ","uma.urr.A5_3_algorithm_sup",
			FT_UINT8,BASE_DEC,  VALS(A5_3_algorithm_sup_vals), 0x02,          
			"A5/3 algorithm supported ", HFILL }
		},
		{ &hf_uma_urr_A5_2_algorithm_sup,
			{ "A5/2 algorithm supported ","uma.urr.A5_2_algorithm_sup",
			FT_UINT8,BASE_DEC,  VALS(A5_2_algorithm_sup_vals), 0x01,          
			"A5/2 algorithm supported ", HFILL }
		},
		{ &hf_uma_urr_RR_cause,
			{ "RR cause value","uma.urr.RRcause",
			FT_UINT8,BASE_DEC,  VALS(RR_cause_vals), 0x0,          
			"RR cause value", HFILL }
		},
		{ &hf_uma_urr_sc,
			{ "SC","uma.urr.SC",
			FT_UINT8,BASE_DEC,  VALS(SC_vals), 0x1,          
			"SC", HFILL }
		},
		{ &hf_uma_urr_algorithm_id,
			{ "Algorithm identifier","uma.urr.algorithm_identifier",
			FT_UINT8,BASE_DEC,  VALS(algorithm_identifier_vals), 0xe,          
			"Algorithm_identifier", HFILL }
		},
		{ &hf_uma_urr_GPRS_resumption,
			{ "GPRS resumption ACK","uma.urr.GPRS_resumption",
			FT_UINT8,BASE_DEC,  VALS(GPRS_resumption_vals), 0x1,          
			"GPRS resumption ACK", HFILL }
		},
		{ &hf_uma_urr_ULQI,
			{ "ULQI, UL Quality Indication","uma.urr.ULQI",
			FT_UINT8,BASE_DEC,  VALS(ULQI_vals), 0x0f,          
			"ULQI, UL Quality Indication", HFILL }
		},
		{ &hf_uma_urr_TLLI,
			{ "Temporary Logical Link Identifier","uma.urr.tlli",
			FT_BYTES,BASE_DEC,  NULL, 0x0,          
			"Temporary Logical Link Identifier", HFILL }
		},
		{ &hf_uma_urr_packet_flow_id,
			{ "Packet Flow Identifier value","uma.urr.packet_flow_id",
			FT_UINT8,BASE_DEC,  VALS(packet_flow_id_vals), 0x7f,          
			"Packet Flow Identifier value", HFILL }
		},
		{ &hf_uma_urr_suspension_cause,
			{ "Suspension cause value","uma.urr.suspension_cause",
			FT_UINT8,BASE_DEC,  VALS(suspension_cause_vals), 0x0,          
			"Suspension cause value", HFILL }
		},
		{ &hf_uma_urr_TU3920_timer,
			{ "TU3920 Timer value(seconds)","uma.urr.tu3920",
			FT_UINT16,BASE_DEC,  NULL, 0x0,          
			"TU3920 Timer value(seconds)", HFILL }
		},
		{ &hf_uma_urr_rate,
			{ "Peak bit rate provided by the network(100 bits/s)","uma.urr.tu3920",
			FT_UINT16,BASE_DEC,  NULL, 0x0,          
			"Peak bit rate provided by the network(100 bits/s)", HFILL }
		},
		{ &hf_uma_urr_precedence,
			{ "Precedence","uma.urr.precedence",
			FT_UINT8,BASE_DEC,  VALS(precedence_vals), 0x7,          
			"Precedence", HFILL }
		},
		{ &hf_uma_urr_a_bit,
			{ "A bit","uma.urr.a_bit",
			FT_UINT8,BASE_DEC,  VALS(a_bit_vals), 0x8,          
			"A bit", HFILL }
		},
		{ &hf_uma_urr_t_bit,
			{ "T bit","uma.urr.t_bit",
			FT_UINT8,BASE_DEC,  VALS(t_bit_vals), 0x10,          
			"T bit", HFILL }
		},
		{ &hf_uma_urr_c_r_bit,
			{ "C/R bit","uma.urr.c_r_bit",
			FT_UINT8,BASE_DEC,  VALS(c_r_bit_vals), 0x20,          
			"C/R bit", HFILL }
		},
		{ &hf_uma_urr_URLCcause,
			{ "URLC Cause","uma.urr.URLCcause",
			FT_UINT8,BASE_DEC,  VALS(URLC_cause_vals), 0x0,          
			"URLC Cause", HFILL }
		},
		{ &hf_uma_urr_udr,
			{ "User Data Rate value (bits/s)","uma.urr.URLCcause",
			FT_UINT32,BASE_DEC,  NULL, 0x0,          
			"User Data Rate value (bits/s)", HFILL }
		},
		{ &hf_uma_urr_TU4001_timer,
			{ "TU4001 Timer value(seconds)","uma.urr.tu4001",
			FT_UINT16,BASE_DEC,  NULL, 0x0,          
			"TU4001 Timer value(seconds)", HFILL }
		},
		{ &hf_uma_urr_LS,
			{ "Location Status(LS)","uma.urr.LS",
			FT_UINT8,BASE_DEC,  VALS(LS_vals), 0x3,          
			"Location Status(LS)", HFILL }
		},
		{ &hf_uma_urr_cipher_res,
			{ "Cipher Response(CR)","uma.urr.CR",
			FT_UINT8,BASE_DEC,  VALS(CR_vals), 0x3,          
			"Cipher Response(CR)", HFILL }
		},
		{ &hf_uma_urr_rand_val,
			{ "Ciphering Command RAND value","uma.rand_val",
			FT_BYTES,BASE_HEX,  NULL, 0x0,          
			"Ciphering Command RAND value", HFILL }
		},
		{ &hf_uma_urr_ciphering_command_mac,
			{ "Ciphering Command MAC (Message Authentication Code)","uma.ciphering_command_mac",
			FT_BYTES,BASE_HEX,  NULL, 0x0,          
			"Ciphering Command MAC (Message Authentication Code)", HFILL }
		},
		{ &hf_uma_urr_ciphering_key_seq_num,
			{ "Values for the ciphering key","uma.ciphering_key_seq_num",
			FT_UINT8,BASE_DEC,  NULL, 0x7,          
			"Values for the ciphering key", HFILL }
		},
		{ &hf_uma_urr_sapi_id,
			{ "SAPI ID","uma.sapi_id",
			FT_UINT8,BASE_DEC,  VALS(sapi_id_vals), 0x7,          
			"SAPI ID", HFILL }
		},
		{ &hf_uma_urr_establishment_cause,
			{ "Establishment Cause","uma.urr.establishment_cause",
			FT_UINT8,BASE_DEC,  VALS(establishment_cause_vals), 0x0,          
			"Establishment Cause", HFILL }
		},
		{ &hf_uma_urr_channel,
			{ "Channel","uma.urr.establishment_cause",
			FT_UINT8,BASE_DEC,  VALS(channel_vals), 0x3,          
			"Channel", HFILL }
		},
		{ &hf_uma_urr_PDU_in_error,
			{ "PDU in Error,","uma.urr.PDU_in_error",
			FT_BYTES,BASE_HEX,  NULL, 0x0,          
			"PDU in Error,", HFILL }
		},
		{ &hf_uma_urr_sample_size,
			{ "Sample Size","uma.urr.sample_size",
			FT_UINT8,BASE_DEC,  VALS(sample_size_vals), 0x0,          
			"Sample Size", HFILL }
		},
		{ &hf_uma_urr_payload_type,
			{ "Payload Type","uma.urr.sample_size",
			FT_UINT8,BASE_DEC,  NULL, 0x0,          
			"Payload Type", HFILL }
		},
		{ &hf_uma_urr_multirate_speech_ver,
			{ "Multirate speech version","uma.urr.multirate_speech_ver",
			FT_UINT8,BASE_DEC,  VALS(multirate_speech_ver_vals), 0xe0,          
			"Multirate speech version", HFILL }
		},
		{ &hf_uma_urr_NCSB,
			{ "NSCB: Noise Suppression Control Bit","uma.urr.NCSB",
			FT_UINT8,BASE_DEC,  VALS(NSCB_vals), 0x10,          
			"NSCB: Noise Suppression Control Bit", HFILL }
		},
		{ &hf_uma_urr_ICMI,
			{ "ICMI: Initial Codec Mode Indicator","uma.urr.ICMI",
			FT_UINT8,BASE_DEC,  VALS(ICMI_vals), 0x8,          
			"ICMI: Initial Codec Mode Indicator", HFILL }
		},
		{ &hf_uma_urr_start_mode,
			{ "Start Mode","uma.urr.start_mode",
			FT_UINT8,BASE_DEC,  NULL, 0x3,          
			"Start Mode", HFILL }
		},
		{ &hf_uma_urr_LLC_PDU,
			{ "LLC-PDU","uma.urr.llc_pdu",
			FT_BYTES,BASE_HEX,  NULL, 0x0,          
			"LLC-PDU", HFILL }
		},
		{ &hf_uma_urr_LBLI,
			{ "LBLI, Location Black List indicator","uma.urr.LBLI",
			FT_UINT8,BASE_DEC,  VALS(LBLI_vals), 0x0,          
			"LBLI, Location Black List indicator", HFILL }
		},
		{ &hf_uma_urr_RI,
			{ "Reset Indicator(RI)","uma.urr.RI",
			FT_UINT8,BASE_DEC,  VALS(RI_vals), 0x1,          
			"Reset Indicator(RI)", HFILL }
		},
		{ &hf_uma_urr_TU4003_timer,
			{ "TU4003 Timer value(seconds)","uma.urr.tu4003",
			FT_UINT16,BASE_DEC,  NULL, 0x0,          
			"TU4003 Timer value(seconds)", HFILL }
		},
		{ &hf_uma_urr_ap_service_name_type,
			{ "AP Service Name type","uma.urr.ap_service_name_type",
			FT_UINT8,BASE_DEC,  VALS(ap_service_name_type_vals), 0x0,          
			"AP Service Name type", HFILL }
		},
		{ &hf_uma_urr_ap_Service_name_value,
			{ "AP Service Name Value","uma.urr.ap_service_name_value",
			FT_STRING,BASE_NONE,  NULL, 0x0,          
			"AP Service Name Value", HFILL }
		},
		{ &hf_uma_urr_uma_service_zone_icon_ind,
			{ "UMA Service Zone Icon Indicator","uma.urr.uma_service_zone_icon_ind",
			FT_UINT8,BASE_DEC,  VALS(uma_service_zone_icon_ind_vals), 0x0,          
			"UMA Service Zone Icon Indicator", HFILL }
		},
		{ &hf_uma_urr_uma_service_zone_str_len,
			{ "Length of UMA Service Zone string","uma.urr.service_zone_str_len",
			FT_UINT8,BASE_DEC,  NULL, 0x0,          
			"Length of UMA Service Zone string", HFILL }
		},
		{ &hf_uma_urr_window_size,
			{ "Window Size","uma.urr.uma_window_size",
			FT_UINT8,BASE_DEC,  VALS(window_size_vals), 0x3,          
			"Window Size", HFILL }
		},
		{ &hf_uma_urr_uma_codec_mode,
			{ "Codec Mode","uma.urr.uma_codec_mode",
			FT_UINT8,BASE_DEC,  NULL, 0xc0,          
			"Codec Mode", HFILL }
		},
		{ &hf_uma_urr_UTRAN_cell_id_disc,
			{ "UTRAN Cell Identification Discriminator","uma.urr.uma_UTRAN_cell_id_disc",
			FT_UINT8,BASE_DEC,  VALS(UTRAN_cell_id_disc_vals), 0x0f,          
			"UTRAN Cell Identification Discriminator", HFILL }
		},
		{ &hf_uma_urr_suti,
			{ "SUTI, Serving UNC table indicator indicator","uma.urr.uma_suti",
			FT_UINT8,BASE_DEC,  VALS(suti_vals), 0x01,          
			"SUTI, Serving UNC table indicator indicator", HFILL }
		},
		{ &hf_uma_urr_uma_mps,
			{ "UMPS, Manual PLMN Selection indicator","uma.urr.mps",
			FT_UINT8,BASE_DEC,  VALS(mps_vals), 0x3,          
			"MPS, Manual PLMN Selection indicator", HFILL }
		},
		{ &hf_uma_urr_num_of_plms,
			{ "Number of PLMN:s","uma.urr.num_of_plms",
			FT_UINT8,BASE_DEC,  NULL, 0x0,          
			"Number of PLMN:s", HFILL }
		},
		{ &hf_uma_urr_ms_radio_id,
			{ "MS Radio Identity","uma.urr.ms_radio_id",
			FT_ETHER, BASE_DEC, NULL, 0x00,          
			"MS Radio Identity", HFILL }
		},
		{ &hf_uma_urr_uma_service_zone_str,
			{ "UMA Service Zone string,","uma.urr.uma_service_zone_str",
			FT_STRING,BASE_NONE,  NULL, 0x0,          
			"UMA Service Zone string,", HFILL }
		},
		{ &hf_uma_urr_unc_ipv4,
			{ "UNC IPv4 address","uma.urr.uncipv4",
			FT_IPv4,BASE_NONE,  NULL, 0x0,          
			"UNC IPv4 address", HFILL }
		},
		{ &hf_uma_unc_FQDN,
		       { "UNC Fully Qualified Domain/Host Name (FQDN)", "uma.urr.unc_fqdn",
		       FT_STRING, BASE_NONE,NULL,0x0,
			"UNC Fully Qualified Domain/Host Name (FQDN)", HFILL }
		},
		{ &hf_uma_urr_GPRS_user_data_transport_ipv4,
			{ "IP address for GPRS user data transport","uma.urr.gprs_usr_data_ipv4",
			FT_IPv4,BASE_NONE,  NULL, 0x0,          
			"IP address for GPRS user data transport", HFILL }
		},
		{ &hf_uma_urr_GPRS_port,
			{ "UDP Port for GPRS user data transport","uma.urr.gprs_port",
			FT_UINT16,BASE_DEC,  NULL, 0x0,          
			"UDP Port for GPRS user data transport", HFILL }
		},
		{ &hf_uma_urr_UNC_tcp_port,
			{ "UNC TCP port","uma.urr.gprs_port",
			FT_UINT16,BASE_DEC,  NULL, 0x0,          
			"UDP Port for GPRS user data transport", HFILL }
		},
		{ &hf_uma_urr_RTP_port,
			{ "RTP UDP port","uma.urr.rtp_port",
			FT_UINT16,BASE_DEC,  NULL, 0x0,          
			"RTP UDP port", HFILL }
		},
		{ &hf_uma_urr_RTCP_port,
			{ "RTCP UDP port","uma.urr.rtcp_port",
			FT_UINT16,BASE_DEC,  NULL, 0x0,          
			"RTCP UDP port", HFILL }
		},
	};

/* Setup protocol subtree array */
	static gint *ett[] = {
		&ett_uma,
		&ett_uma_toc,
		&ett_urr_ie,
	};

/* Register the protocol name and description */
	proto_uma = proto_register_protocol("Unlicensed Mobile Access","UMA", "uma");
	bssap_pdu_type_table = find_dissector_table("bssap.pdu_type");

/* Required function calls to register the header fields and subtrees used */
	proto_register_field_array(proto_uma, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));
	/* Register a configuration option for port */

	
	uma_module = prefs_register_protocol(proto_uma, proto_reg_handoff_uma);

	prefs_register_uint_preference(uma_module, "tcp.port1",
								   "Unlicensed Mobile Access TCP Port1",
								   "Set the TCP port1 for Unlicensed Mobile Access messages",
								   10,
								   &gbl_umaTcpPort1);


}


