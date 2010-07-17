/* packet-mip6.h
 *
 * $Id$
 *
 * Definitions for Mobile IPv6 dissection (RFC 3775)
 * and Fast Handover for Mobile IPv6 (FMIPv6, RFC 4068)
 * Copyright 2003 Oy L M Ericsson Ab <teemu.rinta-aho@ericsson.fi>
 *
 * FMIPv6 support added by Martin Andre <andre@clarinet.u-strasbg.fr>
 * Copyright 2006, Nicolas DICHTEL - 6WIND - <nicolas.dichtel@6wind.com>
 *
 * Modifications for NEMO packets (RFC 3963): Bruno Deniaud
 * (bdeniaud@irisa.fr, nono@chez.com) 12 Oct 2005
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __PACKET_MIP6_H_DEFINED__
#define __PACKET_MIP6_H_DEFINED__

/* Mobility Header types */
typedef enum {
	BRR  = 0,
	HOTI = 1,
	MHCOTI = 2,
	HOT  = 3,
	MHCOT  = 4,
	BU   = 5,
	BA   = 6,
	BE    = 7,
	FBU   = 8,
	FBACK = 9,
	FNA   = 10,
	EMH   = 11,
	HAS   = 12,
	HB    = 13,
	HI    = 14,
	HAck  = 15,
	BR    = 16,
} mhTypes;

static const value_string mip6_mh_types[] = {
	{BRR,   "Binding Refresh Request"},
	{HOTI,  "Home Test Init"},
	{MHCOTI,  "Care-of Test Init"},
	{HOT,   "Home Test"},
	{MHCOT,   "Care-of Test"},
	{BU,    "Binding Update"},
	{BA,    "Binding Acknowledgement"},
	{BE,    "Binding Error"},
	{FBU,   "Fast Binding Update"},
	{FBACK, "Fast Binding Acknowledgment"},
	{FNA,   "Fast Neighbor Advertisement"},
	{EMH,   "Experimental Mobility Header"},
	{HAS,   "Home Agent Switch"},
	{HB,    "Heartbeat"},
	{HI,    "Handover Initiate"},
	{HAck,  "Handover Acknowledge"},
	{BR,    "Binding Revocation"},
	{0,     NULL}
};

/* Mobility Option types 
 * http://www.iana.org/assignments/mobility-parameters/mobility-parameters.xhtml
 */
typedef enum {
	PAD1 = 0,	/* 0 Pad1 [RFC3775] */
	PADN = 1,	/* 1 PadN [RFC3775] */
	BRA  = 2,	/* 2 Binding Refresh Advice */
	ACOA = 3,	/* 3 Alternate Care-of Address */
	NI   = 4,	/* 4 Nonce Indices */
	AUTD = 5,	/* 5 Authorization Data */
	MNP  = 6,	/* 6 Mobile Network Prefix Option */
	MHLLA  = 7,	/* 7 Mobility Header Link-Layer Address option [RFC5568] */
	MNID = 8,	/* 8 MN-ID-OPTION-TYPE */
	AUTH = 9,	/* 9 AUTH-OPTION-TYPE */
	MESGID = 10,	/* 10 MESG-ID-OPTION-TYPE [RFC4285]  */
	CGAPR = 11,	/* 11 CGA Parameters Request [RFC4866]  */
	CGAR = 12,	/* 12 CGA Parameters [RFC4866]  */
	SIGN = 13,	/* 13 Signature [RFC4866]  */
	PHKT = 14,	/* 14 Permanent Home Keygen Token [RFC4866]  */ 
	MOCOTI = 15,	/* 15 Care-of Test Init [RFC4866]  */
	MOCOT = 16,	/* 16 Care-of Test [RFC4866]  */
	DNSU = 17,	/* 17 DNS-UPDATE-TYPE [RFC5026]  */
	EM = 18,	/* 18 Experimental Mobility Option [RFC5096]  */
	VSM = 19,	/* 19 Vendor Specific Mobility Option [RFC5094]  */
	SSM = 20,	/* 20 Service Selection Mobility Option [RFC5149]  */
	BADFF = 21,	/* 21 Binding Authorization Data for FMIPv6 (BADF) [RFC5568]  */
	HNP  = 22,	/* 22 Home Network Prefix Option [RFC5213]   */
	MOHI = 23,	/* 23 Handoff Indicator Option [RFC5213]   */
	ATT = 24,	/* 24 Access Technology Type Option [RFC5213]  */ 
	MNLLI = 25,	/* 25 Mobile Node Link-layer Identifier Option [RFC5213]  */ 
	LLA = 26,	/* 26 Link-local Address Option [RFC5213   */
	TS   = 27,	/* 27 Timestamp */
	RC = 28,	/* 28 Restart Counter [RFC-ietf-netlmm-pmipv6-heartbeat-07] */ 
	IPV4HA = 29,	/* 29 IPv4 Home Address [RFC5555]  */
	IPV4AA = 30,	/* 30 IPv4 Address Acknowledgement [RFC5555] */ 
	NATD = 31,	/* 31 NAT Detection [RFC5555]  */
	IPV4COA = 32,	/* 32 IPv4 Care-of Address [RFC5555]  */
	GREK = 33,	/* 33 GRE Key Option [RFC-ietf-netlmm-grekey-option-09]  */
	MHIPV6AP = 34,	/* 34 Mobility Header IPv6 Address/Prefix [RFC5568]  */
	BI = 35,	/* 35 Binding Identifier [RFC-ietf-monami6-multiplecoa-14]  */
	IPV4HAREQ = 36,	/* 36 IPv4 Home Address Request [RFC5844] */
	IPV4HAREP = 37,	/* 37 IPv4 Home Address Reply [RFC5844] */
	IPV4DRA = 38,	/* 38 IPv4 Default-Router Address [RFC5844] */
	IPV4DSM = 39,	/* 39 IPv4 DHCP Support Mode [RFC5844] */
	CR = 40,	/* 40 Context Request Option [RFC-ietf-mipshop-pfmipv6-14] */
	LMAA = 41,	/* 41 Local Mobility Anchor Address Option [RFC-ietf-mipshop-pfmipv6-14] */
	MNLLAII = 42,	/* 42 Mobile Node Link-local Address Interface Identifier Option [RFC-ietf-mipshop-pfmipv6-14] */
} optTypes;

/* Binding Update flag description */
static const true_false_string mip6_bu_a_flag_value = {
	"Binding Acknowledgement requested",
	"Binding Acknowledgement not requested"
};

static const true_false_string mip6_bu_h_flag_value = {
	"Home Registration",
	"No Home Registration"
};

static const true_false_string mip6_bu_l_flag_value = {
	"Link-Local Address Compatibility",
	"No Link-Local Address Compatibility"
};

static const true_false_string mip6_bu_k_flag_value = {
	"Key Management Mobility Compatibility",
	"No Key Management Mobility Compatibility"
};

static const true_false_string mip6_bu_m_flag_value = {
	"MAP Registration Compatibility",
	"No MAP Registration Compatibility",
};

static const true_false_string nemo_bu_r_flag_value = {
	"Mobile Router Compatibility",
	"No Mobile Router Compatibility"
};

static const true_false_string proxy_bu_p_flag_value = {
	"Proxy Registration",
	"No Proxy Registration"
};

static const true_false_string proxy_bu_f_flag_value = {
	"Forcing UDP encapsulation used",
	"No Forcing UDP encapsulation"
};

static const true_false_string proxy_bu_t_flag_value = {
	"TLV-header format used",
	"No TLV-header format"
};

/* Binding Acknowledgement status values */
static const value_string mip6_ba_status_value[] = {
	{   0, "Binding Update accepted" },
	{   1, "Accepted but prefix discovery necessary" },
	{ 128, "Reason unspecified" },
	{ 129, "Administratively prohibited" },
	{ 130, "Insufficient resources" },
	{ 131, "Home registration not supported" },
	{ 132, "Not home subnet" },
	{ 133, "Not home agent for this mobile node" },
	{ 134, "Duplicate Address Detection failed" },
	{ 135, "Sequence number out of window" },
	{ 136, "Expired home nonce index" },
	{ 137, "Expired care-of nonce index" },
	{ 138, "Expired nonces" },
	{ 139, "Registration type change disallowed" },
	{ 140, "Mobile Router Operation not permitted" },
	{ 141, "Invalid Prefix" },
	{ 142, "Not Authorized for Prefix" },
	{ 143, "Mobile Network Prefix information unavailable" },
	{ 145, "Proxy Registration not supported by the LMA" },
	{ 146, "Proxy Registrations from this MAG not allowed" },
	{ 147, "No home address for this NAI" },
	{ 148, "Invalid Time Stamp Option" },
	{   0, NULL }
};

/* Binding Error status values */
static const value_string mip6_be_status_value[] = {
	{ 1, "Unknown binding for Home Address destination option" },
	{ 2, "Unrecognized MH type value" },
	{ 0, NULL }
};

/* Fast Binding Update flag description */
static const true_false_string fmip6_fbu_a_flag_value = {
	"Fast Binding Acknowledgement requested",
	"Fast Binding Acknowledgement not requested"
};

static const true_false_string fmip6_fbu_h_flag_value = {
	"Home Registration",
	"No Home Registration"
};

static const true_false_string fmip6_fbu_l_flag_value = {
	"Link-Local Address Compatibility",
	"No Link-Local Address Compatibility"
};

static const true_false_string fmip6_fbu_k_flag_value = {
	"Key Management Mobility Compatibility",
	"No Key Management Mobility Compatibility"
};

/* Fast Binding Acknowledgement status values */
static const value_string fmip6_fback_status_value[] = {
	{   0, "Fast Binding Update accepted" },
	{   1, "Accepted but use supplied NCoA" },
	{ 128, "Reason unspecified" },
	{ 129, "Administratively prohibited" },
	{ 130, "Insufficient resources" },
	{ 131, "Incorrect interface identifier length" },
	{   0, NULL }
};

/* Heartbeat flag description */
static const true_false_string mip6_hb_u_flag_value = {
	"Unsolicited Heartbeat Response",
	"Otherwise"
};

static const true_false_string mip6_hb_r_flag_value = {
	"Heartbeat Response",
	"Heartbeat Request"
};

/* MH LLA Option code */
static const value_string fmip6_lla_optcode_value[] = {
	{   2, "Link Layer Address of the MN" },
	{   0, NULL }
};

/* Mobile Node Identifier Option code */
static const value_string mip6_mnid_subtype_value[] = {
	{   1, "Network Access Identifier (NAI)" },
	{   0, NULL }
};

/* mobile network prefix flag description */
static const true_false_string mip6_ipv4ha_p_flag_value = {
	"mobile network prefixt requested",
	"mobile network prefix not requested"
};

/* Vendor-Specific Mobility Option */
static const value_string mip6_vsm_subtype_value[] = {
	{   0, NULL }
};

/* Vendor-Specific Mobility Option (3GPP TS29.282) */
static const value_string mip6_vsm_subtype_3gpp_value[] = {
	{   1, "Protocol Configuration Options" },
	{   2, "3GPP Specific PMIPv6 Error Code" },
	{   3, "PMIPv6 PDN GW IP Address" },
	{   4, "PMIPv6 DHCPv4 Address Allocation Procedure Indication" },
	{   5, "PMIPv6 Fully Qualified PDN Connection Set Identifier" },
	{   6, "PMIPv6 PDN type indication" },
	{   7, "Charging ID" },
	{   8, "Selection Mode" },
	{   9, "I-WLAN Mobility Access Point Name (APN)" },
	{  10, "Charging Characteristics" },
	{  11, "Mobile Equipment Identity (MEI)" },
	{  12, "MSISDN" },
	{  13, "Serving Network" },
	{  14, "APN Restriction" },
	{  15, "Maximum APN Restriction" },
	{  16, "Unauthenticated IMSI" },
	{  17, "PDN Connection ID" },
	{   0, NULL }
};

/* Handoff Indicator Option type */
static const value_string pmip6_hi_opttype_value[] = {
	{   0, "Reserved" },
	{   1, "Attachment over a new interface" },
	{   2, "Handoff between two different interfaces of the mobile node" },
	{   3, "Handoff between mobile access gateways for the same interface" },
	{   4, "Handoff state unknown" },
	{   5, "Handoff state not changed (Re-registration)" },
	{   0, NULL }
};

/* Access Technology Type Option type */
static const value_string pmip6_att_opttype_value[] = {
	{   0, "Reserved" },
	{   1, "Virtual" },
	{   2, "PPP" },
	{   3, "IEEE 802.3" },
	{   4, "IEEE 802.11a/b/g" },
	{   5, "IEEE 802.16e" },
	{   6, "3GPP GERAN" },
	{   7, "3GPP UTRAN" },
	{   8, "3GPP E-UTRAN" },
	{   9, "3GPP2 eHRPD" },
	{  10, "3GPP2 HRPD" },
	{  11, "3GPP2 1xRTT" },
	{  12, "3GPP2 UMB" },
	{   0, NULL }
};

/* Mobility Option types 
 * http://www.iana.org/assignments/mobility-parameters/mobility-parameters.xhtml
 */

static const value_string mip6_mobility_options[] = {
	{ PAD1,	"Pad1"},										/* RFC3775 */ 
	{ PADN,	"PadN"},										/* RFC3775 */ 
	{ BRA,	"Binding Refresh Advice"},						/* RFC3775 */ 
	{ ACOA,	"Alternate Care-of Address"},					/* RFC3775 */ 
	{ NI,	"Nonce Indices"},								/* RFC3775 */ 
	{ AUTD,	"Authorization Data"},							/* RFC3775 */ 
	{ MNP,	"Mobile Network Prefix Option"},				/* RFC3963 */ 
	{ MHLLA,	"Mobility Header Link-Layer Address option"},	/* RFC5568 */ 
	{ MNID,	"MN-ID-OPTION-TYPE"},							/* RFC4283 */ 
	{ AUTH,	"AUTH-OPTION-TYPE"},							/* RFC4285 */ 
	{ MESGID,	"MESG-ID-OPTION-TYPE"},							/* RFC4285 */ 
	{ CGAPR,	"CGA Parameters Request"},						/* RFC4866 */ 
	{ CGAR,	"CGA Parameters"},								/* RFC4866 */ 
	{ SIGN,	"Signature"},									/* RFC4866 */ 
	{ PHKT,	"Permanent Home Keygen Token"},					/* RFC4866 */ 
	{ MOCOTI,	"Care-of Test Init"},							/* RFC4866 */ 
	{ MOCOT,	"Care-of Test"},								/* RFC4866 */ 
	{ DNSU,	"DNS-UPDATE-TYPE"},								/* RFC5026 */ 
	{ EM,	"Experimental Mobility Option"},				/* RFC5096 */ 
	{ VSM,	"Vendor Specific Mobility Option"},				/* RFC5094 */ 
	{ SSM,	"Service Selection Mobility Option"},			/* RFC5149 */ 
	{ BADFF,	"Binding Authorization Data for FMIPv6 (BADF)"}, /* RFC5568 */ 
	{ HNP,	"Home Network Prefix Option"},					/* RFC5213 */ 
	{ MOHI,	"Handoff Indicator Option"},					/* RFC5213 */ 
	{ ATT,	"Access Technology Type Option"},				/* RFC5213 */ 
	{ MNLLI,	"Mobile Node Link-layer Identifier Option"},	/* RFC5213 */ 
	{ LLA,	"Link-local Address Option"},					/* RFC5213 */ 
	{ TS,	"Timestamp Option"},							/* RFC5213 */ 
	{ RC,	"Restart Counter"},								/* RFC5847 */ 
	{ IPV4HA,	"IPv4 Home Address"},							/* RFC5555 */ 
	{ IPV4AA,	"IPv4 Address Acknowledgement"},				/* RFC5555 */ 
	{ NATD,	"NAT Detection"},								/* RFC5555 */ 
	{ IPV4COA,	"IPv4 Care-of Address"},						/* RFC5555 */ 
	{ GREK,	"GRE Key Option"},								/* RFC5845 */ 
	{ MHIPV6AP,	"Mobility Header IPv6 Address/Prefix"},			/* RFC5568 */ 
	{ BI,	"Binding Identifier"},							/* RFC5648 */ 
	{ IPV4HAREQ,	"IPv4 Home Address Request"},					/* RFC5844 */ 
	{ IPV4HAREP,	"IPv4 Home Address Reply"},						/* RFC5844 */ 
	{ IPV4DRA,	"IPv4 Default-Router Address"},					/* RFC5844 */ 
	{ IPV4DSM,	"IPv4 DHCP Support Mode"},						/* RFC5844 */ 
	{ CR,	"Context Request Option"},						/* RFC-ietf-mipshop-pfmipv6-14 */ 
	{ LMAA,	"Local Mobility Anchor Address Option"},		/* RFC-ietf-mipshop-pfmipv6-14 */ 
	{ MNLLAII,	"Mobile Node Link-local Address Interface Identifier Option"},	/* RFC-ietf-mipshop-pfmipv6-14 */ 
	{ 0, NULL }
};

/* Message lengths */
#define MIP6_BRR_LEN          2
#define MIP6_HOTI_LEN        10
#define MIP6_COTI_LEN        10
#define MIP6_HOT_LEN         18
#define MIP6_COT_LEN         18
#define MIP6_BU_LEN           6
#define MIP6_BA_LEN           6
#define MIP6_BE_LEN          18
#define FMIP6_FBU_LEN         6
#define FMIP6_FBACK_LEN       6
#define FMIP6_FNA_LEN         2
#define MIP6_EMH_LEN          0
#define MIP6_HAS_LEN         18
#define MIP6_HB_LEN           6
#define MIP6_HI_LEN           4
#define MIP6_HAck_LEN         4
#define MIP6_BR_LEN           6

/* Field offsets & lengths for mobility headers */
#define MIP6_PROTO_OFF        0
#define MIP6_HLEN_OFF         1
#define MIP6_TYPE_OFF         2
#define MIP6_RES_OFF          3
#define MIP6_CSUM_OFF         4
#define MIP6_DATA_OFF         6
#define MIP6_PROTO_LEN        1
#define MIP6_HLEN_LEN         1
#define MIP6_TYPE_LEN         1
#define MIP6_RES_LEN          1
#define MIP6_CSUM_LEN         2

#define MIP6_BRR_RES_OFF      6
#define MIP6_BRR_OPTS_OFF     8
#define MIP6_BRR_RES_LEN      2

#define MIP6_HOTI_RES_OFF     6
#define MIP6_HOTI_COOKIE_OFF  8
#define MIP6_HOTI_OPTS_OFF   16
#define MIP6_HOTI_RES_LEN     2
#define MIP6_HOTI_COOKIE_LEN  8

#define MIP6_COTI_RES_OFF     6
#define MIP6_COTI_COOKIE_OFF  8
#define MIP6_COTI_OPTS_OFF   16
#define MIP6_COTI_RES_LEN     2
#define MIP6_COTI_COOKIE_LEN  8

#define MIP6_HOT_INDEX_OFF    6
#define MIP6_HOT_COOKIE_OFF   8
#define MIP6_HOT_TOKEN_OFF   16
#define MIP6_HOT_OPTS_OFF    24
#define MIP6_HOT_INDEX_LEN    2
#define MIP6_HOT_COOKIE_LEN   8
#define MIP6_HOT_TOKEN_LEN    8

#define MIP6_COT_INDEX_OFF    6
#define MIP6_COT_COOKIE_OFF   8
#define MIP6_COT_TOKEN_OFF   16
#define MIP6_COT_OPTS_OFF    24
#define MIP6_COT_INDEX_LEN    2
#define MIP6_COT_COOKIE_LEN   8
#define MIP6_COT_TOKEN_LEN    8

#define MIP6_BU_SEQNR_OFF     6
#define MIP6_BU_FLAGS_OFF     8
#define MIP6_BU_RES_OFF       9
#define MIP6_BU_LIFETIME_OFF 10
#define MIP6_BU_OPTS_OFF     12
#define MIP6_BU_SEQNR_LEN     2
#define MIP6_BU_FLAGS_LEN     2
#define MIP6_BU_RES_LEN       0
#define MIP6_BU_LIFETIME_LEN  2

#define MIP6_BA_STATUS_OFF    6
#define MIP6_BA_FLAGS_OFF     7
#define MIP6_BA_SEQNR_OFF     8
#define MIP6_BA_LIFETIME_OFF 10
#define MIP6_BA_OPTS_OFF     12
#define MIP6_BA_STATUS_LEN    1
#define MIP6_BA_FLAGS_LEN     1
#define MIP6_BA_SEQNR_LEN     2
#define MIP6_BA_LIFETIME_LEN  2

#define MIP6_BE_STATUS_OFF    6
#define MIP6_BE_RES_OFF       7
#define MIP6_BE_HOA_OFF       8
#define MIP6_BE_OPTS_OFF     24
#define MIP6_BE_STATUS_LEN    1
#define MIP6_BE_RES_LEN       1
#define MIP6_BE_HOA_LEN      16

#define FMIP6_FBU_SEQNR_OFF     6
#define FMIP6_FBU_FLAGS_OFF     8
#define FMIP6_FBU_RES_OFF       9
#define FMIP6_FBU_LIFETIME_OFF 10
#define FMIP6_FBU_OPTS_OFF     12
#define FMIP6_FBU_SEQNR_LEN     2
#define FMIP6_FBU_FLAGS_LEN     1
#define FMIP6_FBU_RES_LEN       1
#define FMIP6_FBU_LIFETIME_LEN  2

#define FMIP6_FBACK_STATUS_OFF    6
#define FMIP6_FBACK_FLAGS_OFF     7
#define FMIP6_FBACK_SEQNR_OFF     8
#define FMIP6_FBACK_LIFETIME_OFF 10
#define FMIP6_FBACK_OPTS_OFF     12
#define FMIP6_FBACK_STATUS_LEN    1
#define FMIP6_FBACK_FLAGS_LEN     1
#define FMIP6_FBACK_SEQNR_LEN     2
#define FMIP6_FBACK_LIFETIME_LEN  2

#define FMIP6_FNA_RES_OFF     6
#define FMIP6_FNA_OPTS_OFF    8
#define FMIP6_FNA_RES_LEN     2

#define MIP6_HAS_NRADR_OFF    6
#define MIP6_HAS_RES_OFF      7
#define MIP6_HAS_HAA_OFF      8
#define MIP6_HAS_OPTS_OFF    24
#define MIP6_HAS_NRADR_LEN    1
#define MIP6_HAS_RES_LEN      1
#define MIP6_HAS_HAA_LEN     16

#define MIP6_HB_RES_OFF       6
#define MIP6_HB_FLAGS_OFF     7
#define MIP6_HB_SEQNR_OFF     8
#define MIP6_HB_OPTS_OFF     12
#define MIP6_HB_RES_LEN       1
#define MIP6_HB_FLAGS_LEN     1
#define MIP6_HB_SEQNR_LEN     4

#define MIP6_HI_SEQNR_OFF     6
#define MIP6_HI_FLAGS_OFF     8
#define MIP6_HI_CODE_OFF      9
#define MIP6_HI_OPTS_OFF     10
#define MIP6_HI_SEQNR_LEN     2
#define MIP6_HI_FLAGS_LEN     1
#define MIP6_HI_CODE_LEN      1

#define MIP6_HAck_SEQNR_OFF   6
#define MIP6_HAck_RES_OFF     8
#define MIP6_HAck_CODE_OFF    9
#define MIP6_HAck_OPTS_OFF   10
#define MIP6_HAck_SEQNR_LEN   2
#define MIP6_HAck_RES_LEN     1
#define MIP6_HAck_CODE_LEN    1

#define MIP6_BR_TYPE_OFF      6
#define MIP6_BR_TRGR_OFF      7
#define MIP6_BR_SEQNR_OFF     8
#define MIP6_BR_FLAGS_OFF    10
#define MIP6_BR_RES_OFF      11
#define MIP6_BR_OPTS_OFF     12
#define MIP6_BR_TYPE_LEN      1
#define MIP6_BR_TRGR_LEN      1
#define MIP6_BR_SEQNR_LEN     2
#define MIP6_BR_FLAGS_LEN     1
#define MIP6_BR_RES_LEN       1


/* Field offsets & field and option lengths for mobility options.
 * The option length does *not* include the option type and length
 * fields.  The field offsets, however, do include the type and
 * length fields. */
#define MIP6_BRA_LEN          2
#define MIP6_BRA_RI_OFF       2
#define MIP6_BRA_RI_LEN       2

#define MIP6_ACOA_LEN        16
#define MIP6_ACOA_ACOA_OFF    2
#define MIP6_ACOA_ACOA_LEN   16

#define NEMO_MNP_LEN         18
#define NEMO_MNP_PL_OFF       3
#define NEMO_MNP_MNP_OFF      4
#define NEMO_MNP_MNP_LEN     16

#define MIP6_NI_LEN           4
#define MIP6_NI_HNI_OFF       2
#define MIP6_NI_CNI_OFF       4
#define MIP6_NI_HNI_LEN       2
#define MIP6_NI_CNI_LEN       2

#define MIP6_BAD_AUTH_OFF     2

#define FMIP6_LLA_MINLEN      1
#define FMIP6_LLA_OPTCODE_OFF 2
#define FMIP6_LLA_LLA_OFF     3
#define FMIP6_LLA_OPTCODE_LEN 1

#define MIP6_MNID_MINLEN      2
#define MIP6_MNID_SUBTYPE_OFF 2
#define MIP6_MNID_SUBTYPE_LEN 1
#define MIP6_MNID_MNID_OFF    3

#define MIP6_VSM_MINLEN       2
#define MIP6_VSM_VID_OFF      2
#define MIP6_VSM_VID_LEN      4
#define MIP6_VSM_SUBTYPE_OFF  6
#define MIP6_VSM_SUBTYPE_LEN  1
#define MIP6_VSM_DATA_OFF     7


#define MIP6_SSM_MINLEN       2
#define MIP6_SSM_SSM_OFF      2

#define PMIP6_HI_LEN          2
#define PMIP6_HI_HI_OFF       3
#define PMIP6_HI_HI_LEN       1

#define PMIP6_ATT_LEN         2
#define PMIP6_ATT_ATT_OFF     3
#define PMIP6_ATT_ATT_LEN     1

#define PMIP6_TS_LEN          8

#define MIP6_IPV4HA_LEN       6
#define MIP6_IPV4HA_PREFIXL_OFF 2
#define MIP6_IPV4HA_PREFIXL_LEN 1
#define MIP6_IPV4HA_HA_OFF    4
#define MIP6_IPV4HA_HA_LEN    4

#define MIP6_IPV4AA_LEN       6
#define MIP6_IPV4AA_STATUS_OFF 2
#define MIP6_IPV4AA_STATUS_LEN 1
#define MIP6_IPV4AA_PREFIXL_OFF 3
#define MIP6_IPV4AA_PREFIXL_LEN 1
#define MIP6_IPV4AA_HA_OFF    4
#define MIP6_IPV4AA_HA_LEN    4

#define PMIP6_GREK_LEN        6
#define PMIP6_GREK_ID_OFF     4
#define PMIP6_GREK_ID_LEN     4

#endif /* __PACKET_MIP6_H_DEFINED__ */
