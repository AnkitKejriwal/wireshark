/* packet-sccp.c
 * Routines for Signalling Connection Control Part (SCCP) dissection
 *
 * It is hopefully compliant to:
 *   ANSI T1.112.3-1996
 *   ITU-T Q.713 7/1996
 *   YDN 038-1997 (Chinese ITU variant)
 *
 * Copyright 2002, Jeff Morriss <jeff.morriss[AT]ulticom.com>
 *
 * $Id: packet-sccp.c,v 1.10 2003/04/19 20:13:22 tuexen Exp $
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@ethereal.com>
 * Copyright 1998 Gerald Combs
 *
 * Copied from packet-m2pa.c
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

#ifdef NEED_SNPRINTF_H
#include "snprintf.h"
#endif

#include <epan/packet.h>
#include "packet-mtp3.h"

#define SCCP_SI 3

#define MESSAGE_TYPE_OFFSET 0
#define MESSAGE_TYPE_LENGTH 1
#define POINTER_LENGTH      1
#define POINTER_LENGTH_LONG 2

#define MESSAGE_TYPE_CR    0x01
#define MESSAGE_TYPE_CC    0x02
#define MESSAGE_TYPE_CREF  0x03
#define MESSAGE_TYPE_RLSD  0x04
#define MESSAGE_TYPE_RLC   0x05
#define MESSAGE_TYPE_DT1   0x06
#define MESSAGE_TYPE_DT2   0x07
#define MESSAGE_TYPE_AK    0x08
#define MESSAGE_TYPE_UDT   0x09
#define MESSAGE_TYPE_UDTS  0x0a
#define MESSAGE_TYPE_ED    0x0b
#define MESSAGE_TYPE_EA    0x0c
#define MESSAGE_TYPE_RSR   0x0d
#define MESSAGE_TYPE_RSC   0x0e
#define MESSAGE_TYPE_ERR   0x0f
#define MESSAGE_TYPE_IT    0x10
#define MESSAGE_TYPE_XUDT  0x11
#define MESSAGE_TYPE_XUDTS 0x12
/* The below 2 are ITU only */
#define MESSAGE_TYPE_LUDT  0x13
#define MESSAGE_TYPE_LUDTS 0x14

/* Same as below but with names typed out */
static const value_string sccp_message_type_values[] = {
  { MESSAGE_TYPE_CR,     "Connection Request" },
  { MESSAGE_TYPE_CC,     "Connection Confirm" },
  { MESSAGE_TYPE_CREF,   "Connection Refused" },
  { MESSAGE_TYPE_RLSD,   "Released" },
  { MESSAGE_TYPE_RLC,    "Release Complete" },
  { MESSAGE_TYPE_DT1,    "Data Form 1" },
  { MESSAGE_TYPE_DT2,    "Data Form 2" },
  { MESSAGE_TYPE_AK,     "Data Acknowledgement" },
  { MESSAGE_TYPE_UDT,    "Unitdata" },
  { MESSAGE_TYPE_UDTS,   "Unitdata Service" },
  { MESSAGE_TYPE_ED,     "Expedited Data" },
  { MESSAGE_TYPE_EA,     "Expedited Data Acknowledgement" },
  { MESSAGE_TYPE_RSR,    "Reset Request" },
  { MESSAGE_TYPE_RSC,    "Reset Confirmation" },
  { MESSAGE_TYPE_ERR,    "Error" },
  { MESSAGE_TYPE_IT,     "Inactivity Timer" },
  { MESSAGE_TYPE_XUDT,   "Extended Unitdata" },
  { MESSAGE_TYPE_XUDTS,  "Extended Unitdata Service" },
  { MESSAGE_TYPE_LUDT,   "Long Unitdata (ITU)" },
  { MESSAGE_TYPE_LUDTS,  "Long Unitdata Service (ITU)" },
  { 0,                   NULL } };

/* Same as above but in acronym form (for the Info column) */
static const value_string sccp_message_type_acro_values[] = {
  { MESSAGE_TYPE_CR,     "CR" },
  { MESSAGE_TYPE_CC,     "CC" },
  { MESSAGE_TYPE_CREF,   "CREF" },
  { MESSAGE_TYPE_RLSD,   "RLSD" },
  { MESSAGE_TYPE_RLC,    "RLC" },
  { MESSAGE_TYPE_DT1,    "DT1" },
  { MESSAGE_TYPE_DT2,    "DT2" },
  { MESSAGE_TYPE_AK,     "AK" },
  { MESSAGE_TYPE_UDT,    "UDT" },
  { MESSAGE_TYPE_UDTS,   "UDTS" },
  { MESSAGE_TYPE_ED,     "ED" },
  { MESSAGE_TYPE_EA,     "EA" },
  { MESSAGE_TYPE_RSR,    "RSR" },
  { MESSAGE_TYPE_RSC,    "RSC" },
  { MESSAGE_TYPE_ERR,    "ERR" },
  { MESSAGE_TYPE_IT,     "IT" },
  { MESSAGE_TYPE_XUDT,   "XUDT" },
  { MESSAGE_TYPE_XUDTS,  "XUDTS" },
  { MESSAGE_TYPE_LUDT,   "LUDT" },
  { MESSAGE_TYPE_LUDTS,  "LUDTS" },
  { 0,                   NULL } };

#define PARAMETER_LENGTH_LENGTH 1
#define PARAMETER_LONG_DATA_LENGTH_LENGTH 2
#define PARAMETER_TYPE_LENGTH 1

#define PARAMETER_END_OF_OPTIONAL_PARAMETERS   0x00
#define PARAMETER_DESTINATION_LOCAL_REFERENCE  0x01
#define PARAMETER_SOURCE_LOCAL_REFERENCE       0x02
#define PARAMETER_CALLED_PARTY_ADDRESS         0x03
#define PARAMETER_CALLING_PARTY_ADDRESS        0x04
#define PARAMETER_CLASS                        0x05
#define PARAMETER_SEGMENTING_REASSEMBLING      0x06
#define PARAMETER_RECEIVE_SEQUENCE_NUMBER      0x07
#define PARAMETER_SEQUENCING_SEGMENTING        0x08
#define PARAMETER_CREDIT                       0x09
#define PARAMETER_RELEASE_CAUSE                0x0a
#define PARAMETER_RETURN_CAUSE                 0x0b
#define PARAMETER_RESET_CAUSE                  0x0c
#define PARAMETER_ERROR_CAUSE                  0x0d
#define PARAMETER_REFUSAL_CAUSE                0x0e
#define PARAMETER_DATA                         0x0f
#define PARAMETER_SEGMENTATION                 0x10
#define PARAMETER_HOP_COUNTER                  0x11
/* The below 2 are ITU only */
#define PARAMETER_IMPORTANCE                   0x12
#define PARAMETER_LONG_DATA                    0x13
/* ISNI is ANSI only */
#define PARAMETER_ISNI                         0xfa

static const value_string sccp_parameter_values[] = {
  { PARAMETER_END_OF_OPTIONAL_PARAMETERS, "End of Optional Parameters" },
  { PARAMETER_DESTINATION_LOCAL_REFERENCE, "Destination Local Reference" },
  { PARAMETER_SOURCE_LOCAL_REFERENCE,     "Source Local Reference" },
  { PARAMETER_CALLED_PARTY_ADDRESS,       "Called Party Address" },
  { PARAMETER_CALLING_PARTY_ADDRESS,      "Calling Party Address" },
  { PARAMETER_CLASS,                      "Protocol Class" },
  { PARAMETER_SEGMENTING_REASSEMBLING,    "Segmenting/Reassembling" },
  { PARAMETER_RECEIVE_SEQUENCE_NUMBER,    "Receive Sequence Number" },
  { PARAMETER_SEQUENCING_SEGMENTING,      "Sequencing/Segmenting" },
  { PARAMETER_CREDIT,                     "Credit" },
  { PARAMETER_RELEASE_CAUSE,              "Release Cause" },
  { PARAMETER_RETURN_CAUSE,               "Return Cause" },
  { PARAMETER_RESET_CAUSE,                "Reset Cause" },
  { PARAMETER_ERROR_CAUSE,                "Error Cause" },
  { PARAMETER_REFUSAL_CAUSE,              "Refusal Cause" },
  { PARAMETER_DATA,                       "Data" },
  { PARAMETER_SEGMENTATION,               "Segmentation" },
  { PARAMETER_HOP_COUNTER,                "Hop Counter" },
  { PARAMETER_IMPORTANCE,                 "Importance (ITU)" },
  { PARAMETER_LONG_DATA,                  "Long Data (ITU)" },
  { PARAMETER_ISNI,                       "Intermediate Signaling Network Identification (ANSI)" },
  { 0,                                    NULL } };


#define END_OF_OPTIONAL_PARAMETERS_LENGTH   1
#define DESTINATION_LOCAL_REFERENCE_LENGTH  3
#define SOURCE_LOCAL_REFERENCE_LENGTH       3
#define PROTOCOL_CLASS_LENGTH               1
#define RECEIVE_SEQUENCE_NUMBER_LENGTH      1
#define CREDIT_LENGTH                       1
#define RELEASE_CAUSE_LENGTH                1
#define RETURN_CAUSE_LENGTH                 1
#define RESET_CAUSE_LENGTH                  1
#define ERROR_CAUSE_LENGTH                  1
#define REFUSAL_CAUSE_LENGTH                1
#define HOP_COUNTER_LENGTH                  1
#define IMPORTANCE_LENGTH                   1


/* Parts of the Called and Calling Address parameters */
/* Address Indicator */
#define ADDRESS_INDICATOR_LENGTH   1
#define ITU_RESERVED_MASK          0x80
#define ANSI_NATIONAL_MASK         0x80
#define ROUTING_INDICATOR_MASK     0x40
#define GTI_MASK                   0x3C
#define GTI_SHIFT		   2
#define ITU_SSN_INDICATOR_MASK     0x02
#define ITU_PC_INDICATOR_MASK      0x01
#define ANSI_PC_INDICATOR_MASK     0x02
#define ANSI_SSN_INDICATOR_MASK    0x01

static const value_string sccp_national_indicator_values[] = {
  { 0x0,  "Address coded to International standard" },
  { 0x1,  "Address coded to National standard" },
  { 0,    NULL } };

static const value_string sccp_routing_indicator_values[] = {
  { 0x0, "Route on GT" },
  { 0x1, "Route on SSN" },
  { 0,   NULL } };

#define AI_GTI_NO_GT             0x0
#define ITU_AI_GTI_NAI           0x1
#define AI_GTI_TT                0x2
#define ITU_AI_GTI_TT_NP_ES      0x3
#define ITU_AI_GTI_TT_NP_ES_NAI  0x4
static const value_string sccp_itu_global_title_indicator_values[] = {
  { AI_GTI_NO_GT,             "No Global Title" },
  { ITU_AI_GTI_NAI,           "Nature of Address Indicator only" },
  { AI_GTI_TT,                "Translation Type only" },
  { ITU_AI_GTI_TT_NP_ES,      "Translation Type, Numbering Plan, and Encoding Scheme included" },
  { ITU_AI_GTI_TT_NP_ES_NAI,  "Translation Type, Numbering Plan, Encoding Scheme, and Nature of Address Indicator included" },
  { 0,                        NULL } };

/* #define AI_GTI_NO_GT     0x0 */
#define ANSI_AI_GTI_TT_NP_ES  0x1
/* #define AI_GTI_TT          0x2 */
static const value_string sccp_ansi_global_title_indicator_values[] = {
  { AI_GTI_NO_GT,          "No Global Title" },
  { ANSI_AI_GTI_TT_NP_ES,  "Translation Type, Numbering Plan, and Encoding Scheme included" },
  { AI_GTI_TT,             "Translation Type only" },
  { 0,                     NULL } };

static const value_string sccp_ai_pci_values[] = {
  { 0x1,  "Point Code present" },
  { 0x0,  "Point Code not present" },
  { 0,    NULL } };

static const value_string sccp_ai_ssni_values[] = {
  { 0x1,  "SSN present" },
  { 0x0,  "SSN not present" },
  { 0,    NULL } };

#define ADDRESS_SSN_LENGTH    1
#define INVALID_SSN 0xff
static const value_string sccp_ssn_values[] = {
  { 0x00,  "SSN not known/not used" },
  { 0x01,  "SCCP management" },
  { 0x02,  "Reserved for ITU-T allocation" },
  { 0x03,  "ISDN User Part" },
  { 0x04,  "OMAP (Operation, Maintenance, and Administration Part)" },
  { 0x05,  "MAP (Mobile Application Part)" },
  { 0x06,  "HLR (Home Location Register)" },
  { 0x07,  "VLR (Visitor Location Register)" },
  { 0x08,  "MSC (Mobile Switching Center)" },
  { 0x09,  "EIC/EIR (Equipment Identifier Center/Equipment Identification Register)" },
  { 0x0a,  "AUC/AC (Authentication Center)" },
  { 0x0b,  "ISDN supplementary services (ITU only)" },
  { 0x0c,  "Reserved for international use (ITU only)" },
  { 0x0d,  "Broadband ISDN edge-to-edge applications (ITU only)" },
  { 0x0e,  "TC test responder (ITU only)" },
  { 0,     NULL } };


/* * * * * * * * * * * * * * * * *
 * Global Title: ITU GTI == 0001 *
 * * * * * * * * * * * * * * * * */
#define GT_NAI_MASK 0x7F
#define GT_NAI_LENGTH 1
static const value_string sccp_nai_values[] = {
  { 0x00,  "NAI unknown" },
  { 0x01,  "Subscriber Number" },
  { 0x02,  "Reserved for national use" },
  { 0x03,  "National significant number" },
  { 0x04,  "International number" },
  { 0,     NULL } };


#define GT_OE_MASK 0x80
#define GT_OE_EVEN 0
#define GT_OE_ODD  1
static const value_string sccp_oe_values[] = {
  { GT_OE_EVEN,  "Even number of address signals" },
  { GT_OE_ODD,   "Odd number of address signals" },
  { 0,           NULL } };

#define GT_SIGNAL_LENGTH     1
#define GT_ODD_SIGNAL_MASK   0x0f
#define GT_EVEN_SIGNAL_MASK  0xf0
#define GT_EVEN_SIGNAL_SHIFT 4
#define GT_MAX_SIGNALS 32
static const value_string sccp_address_signal_values[] = {
  { 0,  "0" },
  { 1,  "1" },
  { 2,  "2" },
  { 3,  "3" },
  { 4,  "4" },
  { 5,  "5" },
  { 6,  "6" },
  { 7,  "7" },
  { 8,  "8" },
  { 9,  "9" },
  { 10, "(spare)" },
  { 11, "11" },
  { 12, "12" },
  { 13, "(spare)" },
  { 14, "(spare)" },
  { 15, "ST" },
  { 0,  NULL } };


/* * * * * * * * * * * * * * * * * * * * *
 * Global Title: ITU and ANSI GTI == 0010 *
 * * * * * * * * * * * * * * * * * * * * */
#define GT_TT_LENGTH 1


/* * * * * * * * * * * * * * * * * * * * * * * * * *
 * Global Title: ITU GTI == 0011, ANSI GTI == 0001 *
 * * * * * * * * * * * * * * * * * * * * * * * * * */
#define GT_NP_MASK 0xf0
#define GT_NP_ES_LENGTH 1
static const value_string sccp_np_values[] = {
  { 0x0,  "Unknown" },
  { 0x1,  "ISDN/telephony" },
  { 0x2,  "Generic (ITU)/Reserved (ANSI)" },
  { 0x3,  "Data" },
  { 0x4,  "Telex" },
  { 0x5,  "Maritime mobile" },
  { 0x6,  "Land mobile" },
  { 0x7,  "ISDN/mobile" },
  { 0xe,  "Private network or network-specific" },
  { 0xf,  "Reserved" },
  { 0,    NULL } };

#define GT_ES_MASK     0x0f
#define GT_ES_UNKNOWN  0x0
#define GT_ES_BCD_ODD  0x1
#define GT_ES_BCD_EVEN 0x2
#define GT_ES_NATIONAL 0x3
#define GT_ES_RESERVED 0xf
static const value_string sccp_es_values[] = {
  { GT_ES_UNKNOWN,  "Unknown" },
  { GT_ES_BCD_ODD,  "BCD, odd number of digits" },
  { GT_ES_BCD_EVEN, "BCD, even number of digits" },
  { GT_ES_NATIONAL, "National specific" },
  { GT_ES_RESERVED, "Reserved (ITU)/Spare (ANSI)" },
  { 0,           NULL } };

/* Address signals above */


/* * * * * * * * * * * * * * * * *
 * Global Title: ITU GTI == 0100 *
 * * * * * * * * * * * * * * * * */
/* NP above */
/* ES above */
/* NAI above */
/* Address signals above */


#define CLASS_CLASS_MASK 0xf
#define CLASS_SPARE_HANDLING_MASK 0xf0
static const value_string sccp_class_handling_values [] = {
  { 0x0,  "No special options" },
  { 0x8,  "Return message on error" },
  { 0,    NULL } };


#define SEGMENTING_REASSEMBLING_LENGTH 1
#define SEGMENTING_REASSEMBLING_MASK   0x01
#define NO_MORE_DATA 0
#define MORE_DATA    1
/* This is also used by sequencing-segmenting parameter */
static const value_string sccp_segmenting_reassembling_values [] = {
  { NO_MORE_DATA, "No more data" },
  { MORE_DATA,    "More data" },
  { 0,            NULL } };


#define RECEIVE_SEQUENCE_NUMBER_LENGTH 1
#define RSN_MASK                       0xfe

#define SEQUENCING_SEGMENTING_LENGTH    2
#define SEQUENCING_SEGMENTING_SSN_LENGTH 1
#define SEQUENCING_SEGMENTING_RSN_LENGTH 1
#define SEND_SEQUENCE_NUMBER_MASK       0xfe
#define RECEIVE_SEQUENCE_NUMBER_MASK    0xfe
#define SEQUENCING_SEGMENTING_MORE_MASK 0x01


#define CREDIT_LENGTH 1

#define RELEASE_CAUSE_LENGTH                  1
static const value_string sccp_release_cause_values [] = {
  { 0x00,  "End user originated" },
  { 0x01,  "End user congestion" },
  { 0x02,  "End user failure" },
  { 0x03,  "SCCP user originated" },
  { 0x04,  "Remote procedure error" },
  { 0x05,  "Inconsistent connection data" },
  { 0x06,  "Access failure" },
  { 0x07,  "Access congestion" },
  { 0x08,  "Subsystem failure" },
  { 0x09,  "Subsystem congestion" },
  { 0x0a,  "MTP failure" },
  { 0x0b,  "Netowrk congestion" },
  { 0x0c,  "Expiration of reset timer" },
  { 0x0d,  "Expiration of receive inactivity timer" },
  { 0x0e,  "Reserved" },
  { 0x0f,  "Unqualified" },
  { 0x10,  "SCCP failure (ITU only)" },
  { 0,     NULL } };


#define RETURN_CAUSE_LENGTH               1
static const value_string sccp_return_cause_values [] = {
  { 0x00,  "No translation for an address of such nature" },
  { 0x01,  "No translation for this specific address" },
  { 0x02,  "Subsystem congestion" },
  { 0x03,  "Subsystem failure" },
  { 0x04,  "Unequipped failure" },
  { 0x05,  "MTP failure" },
  { 0x06,  "Network congestion" },
  { 0x07,  "Unqualified" },
  { 0x08,  "Error in message transport" },
  { 0x09,  "Error in local processing" },
  { 0x0a,  "Destination cannot perform reassembly" },
  { 0x0b,  "SCCP failure" },
  { 0x0c,  "Hop counter violation" },
  { 0x0d,  "Segmentation not supported (ITU only)" },
  { 0x0e,  "Segmentation failure (ITU only)" },
  { 0xf9,  "Invalid ISNI routing request (ANSI only)"},
  { 0xfa,  "Unauthorized message (ANSI only)" },
  { 0xfb,  "Message incompatibility (ANSI only)" },
  { 0xfc,  "Cannot perform ISNI constrained routing (ANSI only)" },
  { 0xfd,  "Unable to perform ISNI identification (ANSI only)" },
  { 0,     NULL } };


#define RESET_CAUSE_LENGTH                 1
static const value_string sccp_reset_cause_values [] = {
  { 0x00,  "End user originated" },
  { 0x01,  "SCCP user originated" },
  { 0x02,  "Message out of order - incorrect send sequence number" },
  { 0x03,  "Message out of order - incorrect receive sequence number" },
  { 0x04,  "Remote procedure error - message out of window" },
  { 0x05,  "Remote procedure error - incorrect send sequence number after (re)initialization" },
  { 0x06,  "Remote procedure error - general" },
  { 0x07,  "Remote end user operational" },
  { 0x08,  "Network operational" },
  { 0x09,  "Access operational" },
  { 0x0a,  "Network congestion" },
  { 0x0b,  "Reserved (ITU)/Not obtainable (ANSI)" },
  { 0x0c,  "Unqualified" },
  { 0,     NULL } };


#define ERROR_CAUSE_LENGTH      1
static const value_string sccp_error_cause_values [] = {
  { 0x00,  "Local Reference Number (LRN) mismatch - unassigned destination LRN" },
  { 0x01,  "Local Reference Number (LRN) mismatch - inconsistent source LRN" },
  { 0x02,  "Point code mismatch" },
  { 0x03,  "Service class mismatch" },
  { 0x04,  "Unqualified" },
  { 0,     NULL } };


#define REFUSAL_CAUSE_LENGTH                     1
static const value_string sccp_refusal_cause_values [] = {
  { 0x00,  "End user originated" },
  { 0x01,  "End user congestion" },
  { 0x02,  "End user failure" },
  { 0x03,  "SCCP user originated" },
  { 0x04,  "Destination address unknown" },
  { 0x05,  "Destination inaccessible" },
  { 0x06,  "Network resource - QOS not available/non-transient" },
  { 0x07,  "Network resource - QOS not available/transient" },
  { 0x08,  "Access failure" },
  { 0x09,  "Access congestion" },
  { 0x0a,  "Subsystem failure" },
  { 0x0b,  "Subsystem congestion" },
  { 0x0c,  "Expiration of connection establishment timer" },
  { 0x0d,  "Incompatible user data" },
  { 0x0e,  "Reserved" },
  { 0x0f,  "Unqualified" },
  { 0x10,  "Hop counter violation" },
  { 0x11,  "SCCP failure (ITU only)" },
  { 0x12,  "No translation for an address of such nature" },
  { 0x13,  "Unequipped user" },
  { 0,     NULL } };


#define SEGMENTATION_LENGTH		 4
#define SEGMENTATION_FIRST_SEGMENT_MASK  0x80
#define SEGMENTATION_CLASS_MASK          0x40
#define SEGMENTATION_SPARE_MASK          0x30
#define SEGMENTATION_REMAINING_MASK      0x0f
static const value_string sccp_segmentation_first_segment_values [] = {
  { 1,  "First segment" },
  { 0,  "Not first segment" },
  { 0,  NULL } };
static const value_string sccp_segmentation_class_values [] = {
  { 0,  "Class 0 selected" },
  { 1,  "Class 1 selected" },
  { 0,  NULL } };


#define HOP_COUNTER_LENGTH 1

#define IMPORTANCE_LENGTH          1
#define IMPORTANCE_IMPORTANCE_MASK 0x7


#define ANSI_ISNI_ROUTING_CONTROL_LENGTH 1
#define ANSI_ISNI_MI_MASK                0x01
#define ANSI_ISNI_IRI_MASK               0x06
#define ANSI_ISNI_RES_MASK               0x08
#define ANSI_ISNI_TI_MASK                0x10
#define ANSI_ISNI_TI_SHIFT               4
#define ANSI_ISNI_COUNTER_MASK           0xe0

#define ANSI_ISNI_NETSPEC_MASK           0x03

static const value_string sccp_isni_mark_for_id_values [] = {
  { 0x0,  "Do not identify networks" },
  { 0x1,  "Identify networks" },
  { 0,    NULL } };

static const value_string sccp_isni_iri_values [] = {
  { 0x0,  "Neither constrained nor suggested ISNI routing" },
  { 0x1,  "Constrained ISNI routing" },
  { 0x2,  "Reserved for suggested ISNI routing" },
  { 0x3,  "Spare" },
  { 0,    NULL } };

#define ANSI_ISNI_TYPE_0 0x0
#define ANSI_ISNI_TYPE_1 0x1
static const value_string sccp_isni_ti_values [] = {
  { ANSI_ISNI_TYPE_0,  "Type zero ISNI parameter format" },
  { ANSI_ISNI_TYPE_1,  "Type one ISNI parameter format" },
  { 0,                 NULL } };


/* Initialize the protocol and registered fields */
static int proto_sccp = -1;
static int hf_sccp_message_type = -1;
static int hf_sccp_variable_pointer1 = -1;
static int hf_sccp_variable_pointer2 = -1;
static int hf_sccp_variable_pointer3 = -1;
static int hf_sccp_optional_pointer = -1;
static int hf_sccp_ssn = -1;
static int hf_sccp_gt_digits = -1;

/* Called Party address */
static int hf_sccp_called_national_indicator = -1;
static int hf_sccp_called_routing_indicator = -1;
static int hf_sccp_called_itu_global_title_indicator = -1;
static int hf_sccp_called_ansi_global_title_indicator = -1;
static int hf_sccp_called_itu_ssn_indicator = -1;
static int hf_sccp_called_itu_point_code_indicator = -1;
static int hf_sccp_called_ansi_ssn_indicator = -1;
static int hf_sccp_called_ansi_point_code_indicator = -1;
static int hf_sccp_called_ssn = -1;
static int hf_sccp_called_pc_member = -1;
static int hf_sccp_called_pc_cluster = -1;
static int hf_sccp_called_pc_network = -1;
static int hf_sccp_called_ansi_pc = -1;
static int hf_sccp_called_chinese_pc = -1;
static int hf_sccp_called_itu_pc = -1;
static int hf_sccp_called_gt_nai = -1;
static int hf_sccp_called_gt_oe = -1;
static int hf_sccp_called_gt_tt = -1;
static int hf_sccp_called_gt_np = -1;
static int hf_sccp_called_gt_es = -1;
static int hf_sccp_called_gt_digits = -1;

/* Calling party address */
static int hf_sccp_calling_national_indicator = -1;
static int hf_sccp_calling_routing_indicator = -1;
static int hf_sccp_calling_itu_global_title_indicator = -1;
static int hf_sccp_calling_ansi_global_title_indicator = -1;
static int hf_sccp_calling_itu_ssn_indicator = -1;
static int hf_sccp_calling_itu_point_code_indicator = -1;
static int hf_sccp_calling_ansi_ssn_indicator = -1;
static int hf_sccp_calling_ansi_point_code_indicator = -1;
static int hf_sccp_calling_ssn = -1;
static int hf_sccp_calling_pc_member = -1;
static int hf_sccp_calling_pc_cluster = -1;
static int hf_sccp_calling_pc_network = -1;
static int hf_sccp_calling_ansi_pc = -1;
static int hf_sccp_calling_chinese_pc = -1;
static int hf_sccp_calling_itu_pc = -1;
static int hf_sccp_calling_gt_nai = -1;
static int hf_sccp_calling_gt_oe = -1;
static int hf_sccp_calling_gt_tt = -1;
static int hf_sccp_calling_gt_np = -1;
static int hf_sccp_calling_gt_es = -1;
static int hf_sccp_calling_gt_digits = -1;

/* Other parameter values */
static int hf_sccp_dlr = -1;
static int hf_sccp_slr = -1;
static int hf_sccp_class = -1;
static int hf_sccp_handling = -1;
static int hf_sccp_more = -1;
static int hf_sccp_rsn = -1;
static int hf_sccp_sequencing_segmenting_ssn = -1;
static int hf_sccp_sequencing_segmenting_rsn = -1;
static int hf_sccp_sequencing_segmenting_more = -1;
static int hf_sccp_credit = -1;
static int hf_sccp_release_cause = -1;
static int hf_sccp_return_cause = -1;
static int hf_sccp_reset_cause = -1;
static int hf_sccp_error_cause = -1;
static int hf_sccp_refusal_cause = -1;
static int hf_sccp_segmentation_first = -1;
static int hf_sccp_segmentation_class = -1;
static int hf_sccp_segmentation_remaining = -1;
static int hf_sccp_segmentation_slr = -1;
static int hf_sccp_hop_counter = -1;
static int hf_sccp_importance = -1;
static int hf_sccp_ansi_isni_mi = -1;
static int hf_sccp_ansi_isni_iri = -1;
static int hf_sccp_ansi_isni_ti = -1;
static int hf_sccp_ansi_isni_netspec = -1;
static int hf_sccp_ansi_isni_counter = -1;


/* Initialize the subtree pointers */
static gint ett_sccp = -1;
static gint ett_sccp_called = -1;
static gint ett_sccp_called_ai = -1;
static gint ett_sccp_called_pc = -1;
static gint ett_sccp_called_gt = -1;
static gint ett_sccp_calling = -1;
static gint ett_sccp_calling_ai = -1;
static gint ett_sccp_calling_pc = -1;
static gint ett_sccp_calling_gt = -1;
static gint ett_sccp_sequencing_segmenting = -1;
static gint ett_sccp_segmentation = -1;
static gint ett_sccp_ansi_isni_routing_control = -1;


/*  Keep track of SSN value of current message so if/when we get to the data
 *  parameter, we can call appropriate sub-dissector.  TODO: can this info
 *  be stored elsewhere?
 */
static guint8 called_ssn = INVALID_SSN;
static guint8 calling_ssn = INVALID_SSN;

static dissector_handle_t data_handle;
static dissector_table_t sccp_ssn_dissector_table;

static void
dissect_sccp_unknown_message(tvbuff_t *message_tvb, proto_tree *sccp_tree)
{
  guint32 message_length;

  message_length = tvb_length(message_tvb);

  proto_tree_add_text(sccp_tree, message_tvb, 0, message_length,
		      "Unknown message (%u byte%s)",
		      message_length, plurality(message_length, "", "s"));
}

static void
dissect_sccp_unknown_param(tvbuff_t *tvb, proto_tree *tree, guint8 type, guint16 length)
{
  proto_tree_add_text(tree, tvb, 0, length, "Unknown parameter 0x%x (%u byte%s)",
		      type, length, plurality(length, "", "s"));
}

static void
dissect_sccp_dlr_param(tvbuff_t *tvb, proto_tree *tree, guint8 length)
{
  guint32 reference;

  reference = tvb_get_letoh24(tvb, 0);
  proto_tree_add_uint(tree, hf_sccp_dlr, tvb, 0, length, reference);
}

static void
dissect_sccp_slr_param(tvbuff_t *tvb, proto_tree *tree, guint8 length)
{
  guint32 reference;

  reference = tvb_get_letoh24(tvb, 0);
  proto_tree_add_uint(tree, hf_sccp_slr, tvb, 0, length, reference);
}

static void
dissect_sccp_gt_address_information(tvbuff_t *tvb, proto_tree *tree,
				    guint8 length, gboolean even_length,
				    gboolean called)
{
  guint8 offset = 0;
  guint8 odd_signal, even_signal = 0x0f;
  char gt_digits[GT_MAX_SIGNALS] = { 0 };

  while(offset < length)
  {
    odd_signal = tvb_get_guint8(tvb, offset) & GT_ODD_SIGNAL_MASK;
    even_signal = tvb_get_guint8(tvb, offset) & GT_EVEN_SIGNAL_MASK;
    even_signal >>= GT_EVEN_SIGNAL_SHIFT;

    strcat(gt_digits, val_to_str(odd_signal, sccp_address_signal_values,
				 "Unknown"));

    /* If the last signal is NOT filler */
    if (offset != (length - 1) || even_length == TRUE)
      strcat(gt_digits, val_to_str(even_signal, sccp_address_signal_values,
				   "Unknown"));

    offset += GT_SIGNAL_LENGTH;
  }

  proto_tree_add_string_format(tree, called ? hf_sccp_called_gt_digits
					    : hf_sccp_calling_gt_digits,
			     tvb, 0, length,
			     gt_digits,
			     "Address information (digits): %s", gt_digits);
  proto_tree_add_string_hidden(tree, called ? hf_sccp_gt_digits
					    : hf_sccp_gt_digits,
			     tvb, 0, length,
			     gt_digits);
}

static void
dissect_sccp_global_title(tvbuff_t *tvb, proto_tree *tree, guint8 length,
			  guint8 gti, gboolean called)
{
  proto_item *gt_item = 0;
  proto_tree *gt_tree = 0;
  tvbuff_t *signals_tvb;
  guint8 offset = 0;
  guint8 odd_even, nai, tt, np, es;
  gboolean even = TRUE;

  /* Shift GTI to where we can work with it */
  gti >>= GTI_SHIFT;

  gt_item = proto_tree_add_text(tree, tvb, offset, length,
				"Global Title 0x%x (%u byte%s)",
				gti, length, plurality(length,"", "s"));
  gt_tree = proto_item_add_subtree(gt_item, called ? ett_sccp_called_gt
						   : ett_sccp_calling_gt);

  /* Decode Transation Type (if present) */
  switch (gti) {
  case AI_GTI_TT:

    /* Protocol doesn't tell us, so we ASSUME even... */
    even = TRUE;
    /* Fall through */
  case ITU_AI_GTI_TT_NP_ES:
  case ITU_AI_GTI_TT_NP_ES_NAI:
  case ANSI_AI_GTI_TT_NP_ES:

    tt = tvb_get_guint8(tvb, offset);
    proto_tree_add_uint(gt_tree, called ? hf_sccp_called_gt_tt
					: hf_sccp_calling_gt_tt,
			tvb, offset, GT_TT_LENGTH, tt);
    offset += GT_TT_LENGTH;
  }

  /* Decode Numbering Plan and Encoding Scheme (if present) */
  switch (gti) {
  case ITU_AI_GTI_TT_NP_ES:
  case ITU_AI_GTI_TT_NP_ES_NAI:
  case ANSI_AI_GTI_TT_NP_ES:

    np = tvb_get_guint8(tvb, offset) & GT_NP_MASK;
    proto_tree_add_uint(gt_tree, called ? hf_sccp_called_gt_np
					: hf_sccp_calling_gt_np,
			tvb, offset, GT_NP_ES_LENGTH, np);

    es = tvb_get_guint8(tvb, offset) & GT_ES_MASK;
    proto_tree_add_uint(gt_tree, called ? hf_sccp_called_gt_es
					: hf_sccp_calling_gt_es,
			tvb, offset, GT_NP_ES_LENGTH, es);

    even = (es == GT_ES_BCD_EVEN) ? TRUE : FALSE;

    offset += GT_NP_ES_LENGTH;
  }

  /* Decode Odd/Even Indicator (if present) */
  if (gti == ITU_AI_GTI_NAI) {
    odd_even = tvb_get_guint8(tvb, offset) & GT_OE_MASK;
    proto_tree_add_uint(gt_tree, called ? hf_sccp_called_gt_oe
					: hf_sccp_calling_gt_oe,
			tvb, offset, GT_NAI_LENGTH, odd_even);
    even = (odd_even == GT_OE_EVEN) ? TRUE : FALSE;

    /* offset doesn't change */
  }

  /* Decode Nature of Address Indicator (if present) */
  switch (gti) {
  case ITU_AI_GTI_NAI:
  case ITU_AI_GTI_TT_NP_ES_NAI:
    nai = tvb_get_guint8(tvb, offset) & GT_NAI_MASK;
    proto_tree_add_uint(gt_tree, called ? hf_sccp_called_gt_nai
					: hf_sccp_calling_gt_nai,
			tvb, offset, GT_NAI_LENGTH, nai);

    offset += GT_NAI_LENGTH;
  }

  /* Decode address signal(s) */
  signals_tvb = tvb_new_subset(tvb, offset, (length - offset),
			       (length - offset));
  dissect_sccp_gt_address_information(signals_tvb, gt_tree, (length - offset),
				      even,
				      called);
}

static int
dissect_sccp_3byte_pc(tvbuff_t *tvb, proto_tree *call_tree, guint8 offset,
                     gboolean called)
{
  guint32 dpc;
  proto_item *call_pc_item = 0;
  proto_tree *call_pc_tree = 0;
  char pc[ANSI_PC_STRING_LENGTH];
  int *hf_pc;

  if (mtp3_standard == ANSI_STANDARD)
  {
    if (called)
      hf_pc = &hf_sccp_called_ansi_pc;
    else
      hf_pc = &hf_sccp_calling_ansi_pc;
  } else /* CHINESE_ITU_STANDARD */ {
    if (called)
      hf_pc = &hf_sccp_called_chinese_pc;
    else
      hf_pc = &hf_sccp_calling_chinese_pc;
  }

  /* create the DPC tree; modified from that in packet-mtp3.c */
  dpc = tvb_get_ntoh24(tvb, offset);
  snprintf(pc, sizeof(pc), "%d-%d-%d", (dpc & ANSI_NETWORK_MASK),
				       ((dpc & ANSI_CLUSTER_MASK) >> 8),
				       ((dpc & ANSI_MEMBER_MASK) >> 16));

  call_pc_item = proto_tree_add_string_format(call_tree, *hf_pc,
					      tvb, offset, ANSI_PC_LENGTH,
					      pc, "PC (%s)", pc);

  call_pc_tree = proto_item_add_subtree(call_pc_item,
					  called ? ett_sccp_called_pc
						 : ett_sccp_calling_pc);

  proto_tree_add_uint(call_pc_tree, called ? hf_sccp_called_pc_member
					   : hf_sccp_calling_pc_member,
		      tvb, offset, ANSI_NCM_LENGTH, dpc);
  offset += ANSI_NCM_LENGTH;
  proto_tree_add_uint(call_pc_tree, called ? hf_sccp_called_pc_cluster
					   : hf_sccp_calling_pc_cluster,
		      tvb, offset, ANSI_NCM_LENGTH, dpc);
  offset += ANSI_NCM_LENGTH;
  proto_tree_add_uint(call_pc_tree, called ? hf_sccp_called_pc_network
					   : hf_sccp_calling_pc_network,
		      tvb, offset, ANSI_NCM_LENGTH, dpc);
  offset += ANSI_NCM_LENGTH;

  return(offset);
}

/*  FUNCTION dissect_sccp_called_calling_param():
 *  Dissect the Calling or Called Party Address parameters.
 *
 *  The boolean 'called' describes whether this function is decoding a
 *  called (TRUE) or calling (FALSE) party address.  There is simply too
 *  much code in this function to have 2 copies of it (one for called, one
 *  for calling).
 *
 *  NOTE:  this function is called even when (!tree) so that we can get
 *  the SSN and subsequently call subdissectors (if and when there's a data
 *  parameter).  Realistically we should put if (!tree)'s around a lot of the
 *  code, but I think that would make it unreadable--and the expense of not
 *  doing so does not appear to be very high.
 */
static void
dissect_sccp_called_calling_param(tvbuff_t *tvb, proto_tree *tree,
				  guint8 length, gboolean called)
{
  proto_item *call_item = 0, *call_ai_item = 0;
  proto_tree *call_tree = 0, *call_ai_tree = 0;
  guint8 offset;
  guint8 national = -1, routing_ind, gti, pci, ssni, ssn;
  guint32 dpc;
  tvbuff_t *gt_tvb;

  call_item = proto_tree_add_text(tree, tvb, 0, length,
				    "%s Party address (%u byte%s)",
				    called ? "Called" : "Calling", length,
				    plurality(length, "", "s"));
  call_tree = proto_item_add_subtree(call_item, called ? ett_sccp_called
						       : ett_sccp_calling);

  call_ai_item = proto_tree_add_text(call_tree, tvb, 0,
				       ADDRESS_INDICATOR_LENGTH,
				       "Address Indicator");
  call_ai_tree = proto_item_add_subtree(call_ai_item, called ? ett_sccp_called_ai
							     : ett_sccp_calling_ai);

  if (mtp3_standard == ANSI_STANDARD)
  {
    national = tvb_get_guint8(tvb, 0) & ANSI_NATIONAL_MASK;
    proto_tree_add_uint(call_ai_tree, called ? hf_sccp_called_national_indicator
					     : hf_sccp_calling_national_indicator,
			tvb, 0, ADDRESS_INDICATOR_LENGTH, national);
  }

  routing_ind = tvb_get_guint8(tvb, 0) & ROUTING_INDICATOR_MASK;
  proto_tree_add_uint(call_ai_tree, called ? hf_sccp_called_routing_indicator
					   : hf_sccp_calling_routing_indicator,
		      tvb, 0, ADDRESS_INDICATOR_LENGTH, routing_ind);

  gti = tvb_get_guint8(tvb, 0) & GTI_MASK;

  if (mtp3_standard == ITU_STANDARD ||
      mtp3_standard == CHINESE_ITU_STANDARD ||
      national == 0) {

    proto_tree_add_uint(call_ai_tree, called ? hf_sccp_called_itu_global_title_indicator
					     : hf_sccp_called_itu_global_title_indicator,
			tvb, 0, ADDRESS_INDICATOR_LENGTH, gti);

    ssni = tvb_get_guint8(tvb, 0) & ITU_SSN_INDICATOR_MASK;
    proto_tree_add_uint(call_ai_tree, called ? hf_sccp_called_itu_ssn_indicator
					     : hf_sccp_calling_itu_ssn_indicator,
			tvb, 0, ADDRESS_INDICATOR_LENGTH, ssni);

    pci = tvb_get_guint8(tvb, 0) & ITU_PC_INDICATOR_MASK;
    proto_tree_add_uint(call_ai_tree, called ? hf_sccp_called_itu_point_code_indicator
					     : hf_sccp_calling_itu_point_code_indicator,
			tvb, 0, ADDRESS_INDICATOR_LENGTH, pci);

    offset = ADDRESS_INDICATOR_LENGTH;

    /* Dissect PC (if present) */
    if (pci) {
      if (mtp3_standard == ITU_STANDARD)
      {

	dpc = tvb_get_letohs(tvb, offset) & ITU_PC_MASK;
	proto_tree_add_uint(call_tree, called ? hf_sccp_called_itu_pc
					      : hf_sccp_calling_itu_pc,
			    tvb, offset, ITU_PC_LENGTH, dpc);
	offset += ITU_PC_LENGTH;

      } else /* CHINESE_ITU_STANDARD */ {

	offset = dissect_sccp_3byte_pc(tvb, call_tree, offset, called);

      }
    }

    /* Dissect SSN (if present) */
    if (ssni) {
      ssn = tvb_get_guint8(tvb, offset);
      if (called) {
	      called_ssn = ssn;
      }
      else {
	      calling_ssn = ssn;
      }

      proto_tree_add_uint(call_tree, called ? hf_sccp_called_ssn
					    : hf_sccp_calling_ssn,
			  tvb, offset, ADDRESS_SSN_LENGTH, ssn);
      proto_tree_add_uint_hidden(call_tree, hf_sccp_ssn, tvb, offset,
				 ADDRESS_SSN_LENGTH, ssn);
      offset += ADDRESS_SSN_LENGTH;
    }

    if (!tree)
      return;	/* got SSN, that's all we need here... */

    /* Dissect GT (if present) */
    if (gti != AI_GTI_NO_GT) {
      gt_tvb = tvb_new_subset(tvb, offset, (length - offset),
			      (length - offset));
      dissect_sccp_global_title(gt_tvb, call_tree, (length - offset), gti,
				called);
    }

  } else if (mtp3_standard == ANSI_STANDARD) {

    proto_tree_add_uint(call_ai_tree, called ? hf_sccp_called_ansi_global_title_indicator
					     : hf_sccp_calling_ansi_global_title_indicator,
			tvb, 0, ADDRESS_INDICATOR_LENGTH, gti);

    pci = tvb_get_guint8(tvb, 0) & ANSI_PC_INDICATOR_MASK;
    proto_tree_add_uint(call_ai_tree, called ? hf_sccp_called_ansi_point_code_indicator
					     : hf_sccp_calling_ansi_point_code_indicator,
			tvb, 0, ADDRESS_INDICATOR_LENGTH, pci);

    ssni = tvb_get_guint8(tvb, 0) & ANSI_SSN_INDICATOR_MASK;
    proto_tree_add_uint(call_ai_tree, called ? hf_sccp_called_ansi_ssn_indicator
					     : hf_sccp_calling_ansi_ssn_indicator,
			tvb, 0, ADDRESS_INDICATOR_LENGTH, ssni);

    offset = ADDRESS_INDICATOR_LENGTH;

    /* Dissect SSN (if present) */
    if (ssni) {
      ssn = tvb_get_guint8(tvb, offset);
      if (called) {
	      called_ssn = ssn;
      }
      else {
	      calling_ssn = ssn;
      }

      proto_tree_add_uint(call_tree, called ? hf_sccp_called_ssn
					    : hf_sccp_calling_ssn,
			  tvb, offset, ADDRESS_SSN_LENGTH, ssn);
      proto_tree_add_uint_hidden(call_tree, hf_sccp_ssn, tvb, offset,
				 ADDRESS_SSN_LENGTH, ssn);
      offset += ADDRESS_SSN_LENGTH;
    }

    if (!tree)
      return;	/* got SSN, that's all we need here... */

    /* Dissect PC (if present) */
    if (pci) {
      offset = dissect_sccp_3byte_pc(tvb, call_tree, offset, called);
    }

    /* Dissect GT (if present) */
    if (gti != AI_GTI_NO_GT) {
      gt_tvb = tvb_new_subset(tvb, offset, (length - offset),
			      (length - offset));
      dissect_sccp_global_title(gt_tvb, call_tree, (length - offset), gti,
				called);
    }

  }

}

static void
dissect_sccp_called_param(tvbuff_t *tvb, proto_tree *tree, guint8 length)
{
  dissect_sccp_called_calling_param(tvb, tree, length, TRUE);
}

static void
dissect_sccp_calling_param(tvbuff_t *tvb, proto_tree *tree, guint8 length)
{
  dissect_sccp_called_calling_param(tvb, tree, length, FALSE);
}

static void
dissect_sccp_class_param(tvbuff_t *tvb, proto_tree *tree, guint8 length)
{
  guint8 class, handling;

  class = tvb_get_guint8(tvb, 0) & CLASS_CLASS_MASK;
  handling = tvb_get_guint8(tvb, 0) & CLASS_SPARE_HANDLING_MASK;

  proto_tree_add_uint(tree, hf_sccp_class, tvb, 0, length, class);

  if (class == 0 || class == 1)
    proto_tree_add_uint(tree, hf_sccp_handling, tvb, 0, length, handling);
}

static void
dissect_sccp_segmenting_reassembling_param(tvbuff_t *tvb, proto_tree *tree, guint8 length)
{
  guint8 more;

  more = tvb_get_guint8(tvb, 0) & SEGMENTING_REASSEMBLING_MASK;
  proto_tree_add_uint(tree, hf_sccp_more, tvb, 0, length, more);
}

static void
dissect_sccp_receive_sequence_number_param(tvbuff_t *tvb, proto_tree *tree, guint8 length)
{
  guint8 rsn;

  rsn = tvb_get_guint8(tvb, 0) >> 1;
  proto_tree_add_uint(tree, hf_sccp_rsn, tvb, 0, length, rsn);
}

static void
dissect_sccp_sequencing_segmenting_param(tvbuff_t *tvb, proto_tree *tree, guint8 length)
{
  guint8 rsn, ssn, more;
  proto_item *param_item;
  proto_tree *param_tree;

  ssn = tvb_get_guint8(tvb, 0) >> 1;
  rsn = tvb_get_guint8(tvb, SEQUENCING_SEGMENTING_SSN_LENGTH) >> 1;
  more = tvb_get_guint8(tvb, SEQUENCING_SEGMENTING_SSN_LENGTH) & SEQUENCING_SEGMENTING_MORE_MASK;

  param_item = proto_tree_add_text(tree, tvb, 0, length,
				   val_to_str(PARAMETER_SEQUENCING_SEGMENTING,
					      sccp_parameter_values, "Unknown"));
  param_tree = proto_item_add_subtree(param_item,
				      ett_sccp_sequencing_segmenting);

  proto_tree_add_uint(param_tree, hf_sccp_sequencing_segmenting_ssn, tvb, 0,
		      SEQUENCING_SEGMENTING_SSN_LENGTH, ssn);
  proto_tree_add_uint(param_tree, hf_sccp_sequencing_segmenting_rsn, tvb,
		      SEQUENCING_SEGMENTING_SSN_LENGTH,
		      SEQUENCING_SEGMENTING_RSN_LENGTH, rsn);
  proto_tree_add_uint(param_tree, hf_sccp_sequencing_segmenting_more, tvb,
		      SEQUENCING_SEGMENTING_SSN_LENGTH,
		      SEQUENCING_SEGMENTING_RSN_LENGTH, more);
}

static void
dissect_sccp_credit_param(tvbuff_t *tvb, proto_tree *tree, guint8 length)
{
  guint8 credit;

  credit = tvb_get_guint8(tvb, 0);
  proto_tree_add_uint(tree, hf_sccp_credit, tvb, 0, length, credit);
}

static void
dissect_sccp_release_cause_param(tvbuff_t *tvb, proto_tree *tree, guint8 length)
{
  guint8 cause;

  cause = tvb_get_guint8(tvb, 0);
  proto_tree_add_uint(tree, hf_sccp_release_cause, tvb, 0, length, cause);
}

static void
dissect_sccp_return_cause_param(tvbuff_t *tvb, proto_tree *tree, guint8 length)
{
  guint8 cause;

  cause = tvb_get_guint8(tvb, 0);
  proto_tree_add_uint(tree, hf_sccp_return_cause, tvb, 0, length, cause);
}

static void
dissect_sccp_reset_cause_param(tvbuff_t *tvb, proto_tree *tree, guint8 length)
{
  guint8 cause;

  cause = tvb_get_guint8(tvb, 0);
  proto_tree_add_uint(tree, hf_sccp_reset_cause, tvb, 0, length, cause);
}

static void
dissect_sccp_error_cause_param(tvbuff_t *tvb, proto_tree *tree, guint8 length)
{
  guint8 cause;

  cause = tvb_get_guint8(tvb, 0);
  proto_tree_add_uint(tree, hf_sccp_error_cause, tvb, 0, length, cause);
}

static void
dissect_sccp_refusal_cause_param(tvbuff_t *tvb, proto_tree *tree, guint8 length)
{
  guint8 cause;

  cause = tvb_get_guint8(tvb, 0);
  proto_tree_add_uint(tree, hf_sccp_refusal_cause, tvb, 0, length, cause);
}

/* This function is used for both data and long data (ITU only) parameters */
static void
dissect_sccp_data_param(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{

  /* Try subdissectors (if we found a valid SSN on the current message) */
  if ((called_ssn != INVALID_SSN &&
       dissector_try_port(sccp_ssn_dissector_table, called_ssn, tvb, pinfo,
			  tree)) ||
      (calling_ssn != INVALID_SSN &&
       dissector_try_port(sccp_ssn_dissector_table, calling_ssn, tvb, pinfo,
			  tree)))
    return;

    /* No sub-dissection occured, treat it as raw data */
    call_dissector(data_handle, tvb, pinfo, tree);
}

static void
dissect_sccp_segmentation_param(tvbuff_t *tvb, proto_tree *tree, guint8 length)
{
  guint8 first, class, remaining;
  guint32 slr;
  proto_item *param_item;
  proto_tree *param_tree;

  first = tvb_get_guint8(tvb, 0) & SEGMENTATION_FIRST_SEGMENT_MASK;
  class = tvb_get_guint8(tvb, 0) & SEGMENTATION_CLASS_MASK;
  remaining = tvb_get_guint8(tvb, 0) & SEGMENTATION_REMAINING_MASK;

  slr = tvb_get_letoh24(tvb, 1);

  param_item = proto_tree_add_text(tree, tvb, 0, length,
				   val_to_str(PARAMETER_SEGMENTATION,
					      sccp_parameter_values, "Unknown"));
  param_tree = proto_item_add_subtree(param_item, ett_sccp_segmentation);

  proto_tree_add_uint(param_tree, hf_sccp_segmentation_first, tvb, 0, length,
		      first);
  proto_tree_add_uint(param_tree, hf_sccp_segmentation_class, tvb, 0, length,
		      class);
  proto_tree_add_uint(param_tree, hf_sccp_segmentation_remaining, tvb, 0,
		      length, remaining);
  proto_tree_add_uint(param_tree, hf_sccp_segmentation_slr, tvb, 1, length,
		      slr);
}

static void
dissect_sccp_hop_counter_param(tvbuff_t *tvb, proto_tree *tree, guint8 length)
{
  guint8 hops;

  hops = tvb_get_guint8(tvb, 0);
  proto_tree_add_uint(tree, hf_sccp_hop_counter, tvb, 0, length, hops);
}

static void
dissect_sccp_importance_param(tvbuff_t *tvb, proto_tree *tree, guint8 length)
{
  guint8 importance;

  importance = tvb_get_guint8(tvb, 0) & IMPORTANCE_IMPORTANCE_MASK;
  proto_tree_add_uint(tree, hf_sccp_importance, tvb, 0, length, importance);
}

static void
dissect_sccp_isni_param(tvbuff_t *tvb, proto_tree *tree, guint8 length)
{
  guint8 mi, iri, ti, network, netspec;
  guint8 offset = 0;
  proto_item *param_item;
  proto_tree *param_tree;

  /* Create a subtree for ISNI Routing Control */
  param_item = proto_tree_add_text(tree, tvb, offset, ANSI_ISNI_ROUTING_CONTROL_LENGTH,
				   "ISNI Routing Control");
  param_tree = proto_item_add_subtree(param_item,
				      ett_sccp_ansi_isni_routing_control);

  mi = tvb_get_guint8(tvb, offset) & ANSI_ISNI_MI_MASK;
  proto_tree_add_uint(param_tree, hf_sccp_ansi_isni_mi, tvb, offset,
		      ANSI_ISNI_ROUTING_CONTROL_LENGTH, mi);

  iri = tvb_get_guint8(tvb, offset) & ANSI_ISNI_IRI_MASK;
  proto_tree_add_uint(param_tree, hf_sccp_ansi_isni_iri, tvb, offset,
		      ANSI_ISNI_ROUTING_CONTROL_LENGTH, iri);

  ti = tvb_get_guint8(tvb, offset) & ANSI_ISNI_TI_MASK;
  proto_tree_add_uint(param_tree, hf_sccp_ansi_isni_ti, tvb, offset,
		      ANSI_ISNI_ROUTING_CONTROL_LENGTH, ti);

  offset += ANSI_ISNI_ROUTING_CONTROL_LENGTH;

  if ((ti >> ANSI_ISNI_TI_SHIFT) == ANSI_ISNI_TYPE_1) {
    netspec = tvb_get_guint8(tvb, offset) & ANSI_ISNI_NETSPEC_MASK;
    proto_tree_add_uint(param_tree, hf_sccp_ansi_isni_netspec, tvb, offset,
			ANSI_ISNI_ROUTING_CONTROL_LENGTH, ti);
    offset += ANSI_ISNI_ROUTING_CONTROL_LENGTH;
  }

  while (offset < length) {

    network = tvb_get_guint8(tvb, offset);
    proto_tree_add_text(tree, tvb, offset, ANSI_NCM_LENGTH,
			"Network ID network: %d", network);
    offset++;

    network = tvb_get_guint8(tvb, offset);
    proto_tree_add_text(tree, tvb, offset, ANSI_NCM_LENGTH,
			"Network ID cluster: %d", network);
    offset++;
  }

}

/*  FUNCTION dissect_sccp_parameter():
 *  Dissect a parameter given its type, offset into tvb, and length.
 */
static guint16
dissect_sccp_parameter(tvbuff_t *tvb, packet_info *pinfo, proto_tree *sccp_tree,
		       proto_tree *tree, guint8 parameter_type, guint16 offset,
		       guint16 parameter_length)
{
    tvbuff_t *parameter_tvb;

    switch (parameter_type) {
    case PARAMETER_CALLED_PARTY_ADDRESS:
    case PARAMETER_CALLING_PARTY_ADDRESS:
    case PARAMETER_DATA:
    case PARAMETER_LONG_DATA:
      /*  These parameters must be dissected even if !sccp_tree (so that
       *  subdissectors can be called).
       */
      break;

    default:
      if (!sccp_tree)
        return(parameter_length);

    }

    parameter_tvb = tvb_new_subset(tvb, offset, parameter_length, parameter_length);

    switch (parameter_type) {

    case PARAMETER_END_OF_OPTIONAL_PARAMETERS:
      proto_tree_add_text(sccp_tree, tvb, offset, parameter_length,
			  "End of Optional");
      break;

    case PARAMETER_DESTINATION_LOCAL_REFERENCE:
      dissect_sccp_dlr_param(parameter_tvb, sccp_tree, parameter_length);
      break;

    case PARAMETER_SOURCE_LOCAL_REFERENCE:
      dissect_sccp_slr_param(parameter_tvb, sccp_tree, parameter_length);
      break;

    case PARAMETER_CALLED_PARTY_ADDRESS:
      dissect_sccp_called_param(parameter_tvb, sccp_tree, parameter_length);
      break;

    case PARAMETER_CALLING_PARTY_ADDRESS:
      dissect_sccp_calling_param(parameter_tvb, sccp_tree, parameter_length);
      break;

    case PARAMETER_CLASS:
      dissect_sccp_class_param(parameter_tvb, sccp_tree, parameter_length);
      break;

    case PARAMETER_SEGMENTING_REASSEMBLING:
      dissect_sccp_segmenting_reassembling_param(parameter_tvb, sccp_tree,
						   parameter_length);
      break;

    case PARAMETER_RECEIVE_SEQUENCE_NUMBER:
      dissect_sccp_receive_sequence_number_param(parameter_tvb, sccp_tree,
						 parameter_length);
      break;

    case PARAMETER_SEQUENCING_SEGMENTING:
      dissect_sccp_sequencing_segmenting_param(parameter_tvb, sccp_tree,
					       parameter_length);
      break;

    case PARAMETER_CREDIT:
      dissect_sccp_credit_param(parameter_tvb, sccp_tree, parameter_length);
      break;

    case PARAMETER_RELEASE_CAUSE:
      dissect_sccp_release_cause_param(parameter_tvb, sccp_tree, parameter_length);
      break;

    case PARAMETER_RETURN_CAUSE:
      dissect_sccp_return_cause_param(parameter_tvb, sccp_tree, parameter_length);
      break;

    case PARAMETER_RESET_CAUSE:
      dissect_sccp_reset_cause_param(parameter_tvb, sccp_tree, parameter_length);
      break;

    case PARAMETER_ERROR_CAUSE:
      dissect_sccp_error_cause_param(parameter_tvb, sccp_tree, parameter_length);
      break;

    case PARAMETER_REFUSAL_CAUSE:
      dissect_sccp_refusal_cause_param(parameter_tvb, sccp_tree, parameter_length);
      break;

    case PARAMETER_DATA:
      dissect_sccp_data_param(parameter_tvb, pinfo, tree);

      /* TODO? Re-adjust length of SCCP item since it may be sub-dissected */
      /* sccp_length = proto_item_get_len(sccp_item);
       * sccp_length -= parameter_length;
       * proto_item_set_len(sccp_item, sccp_length);
       */
      break;

    case PARAMETER_SEGMENTATION:
      dissect_sccp_segmentation_param(parameter_tvb, sccp_tree, parameter_length);
      break;

    case PARAMETER_HOP_COUNTER:
      dissect_sccp_hop_counter_param(parameter_tvb, sccp_tree, parameter_length);
      break;

    case PARAMETER_IMPORTANCE:
      if (mtp3_standard != ANSI_STANDARD)
	dissect_sccp_importance_param(parameter_tvb, sccp_tree, parameter_length);
      else
	dissect_sccp_unknown_param(parameter_tvb, sccp_tree, parameter_type,
				   parameter_length);
      break;

    case PARAMETER_LONG_DATA:
      if (mtp3_standard != ANSI_STANDARD)
	dissect_sccp_data_param(parameter_tvb, pinfo, tree);
      else
	dissect_sccp_unknown_param(parameter_tvb, sccp_tree, parameter_type,
				   parameter_length);
      break;

    case PARAMETER_ISNI:
      if (mtp3_standard != ANSI_STANDARD)
	dissect_sccp_unknown_param(parameter_tvb, sccp_tree, parameter_type,
				   parameter_length);
      else
	dissect_sccp_isni_param(parameter_tvb, sccp_tree, parameter_length);
      break;

    default:
      dissect_sccp_unknown_param(parameter_tvb, sccp_tree, parameter_type,
				 parameter_length);
      break;
    }

    return(parameter_length);
}

/*  FUNCTION dissect_sccp_variable_parameter():
 *  Dissect a variable parameter given its type and offset into tvb.  Length
 *  of the parameter is gotten from tvb[0].
 *  Length returned is sum of (length + parameter).
 */
static guint16
dissect_sccp_variable_parameter(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *sccp_tree, proto_tree *tree,
				guint8 parameter_type, guint16 offset)
{
  guint16 parameter_length;
  guint8 length_length;

  if (parameter_type != PARAMETER_LONG_DATA)
  {
    parameter_length = tvb_get_guint8(tvb, offset);
    length_length = PARAMETER_LENGTH_LENGTH;
  }
  else
  {
    /* Long data parameter has 16 bit length */
    parameter_length = tvb_get_letohs(tvb, offset);
    length_length = PARAMETER_LONG_DATA_LENGTH_LENGTH;
  }

/*  TODO? I find this annoying, but it could possibly useful for debugging.
 *  Make it a preference?
 * if (sccp_tree)
 *   proto_tree_add_text(sccp_tree, tvb, offset, length_length,
 *			"%s length: %d",
 *			val_to_str(parameter_type, sccp_parameter_values,
 *				   "Unknown"),
 *			parameter_length);
 */

  offset += length_length;

  dissect_sccp_parameter(tvb, pinfo, sccp_tree, tree, parameter_type, offset,
			 parameter_length);

  return(parameter_length + length_length);
}

/*  FUNCTION dissect_sccp_optional_parameters():
 *  Dissect all the optional parameters given the start of the optional
 *  parameters into tvb.  Parameter types and lengths are read from tvb.
 */
static void
dissect_sccp_optional_parameters(tvbuff_t *tvb, packet_info *pinfo,
				 proto_tree *sccp_tree, proto_tree *tree,
				 guint16 offset)
{
  guint8 parameter_type;

  while ((parameter_type = tvb_get_guint8(tvb, offset)) !=
	 PARAMETER_END_OF_OPTIONAL_PARAMETERS) {

    offset += PARAMETER_TYPE_LENGTH;
    offset += dissect_sccp_variable_parameter(tvb, pinfo, sccp_tree, tree,
					      parameter_type, offset);
  }

  /* Process end of optional parameters */
  dissect_sccp_parameter(tvb, pinfo, sccp_tree, tree, parameter_type, offset,
			 END_OF_OPTIONAL_PARAMETERS_LENGTH);

}


static void
dissect_sccp_message(tvbuff_t *tvb, packet_info *pinfo, proto_tree *sccp_tree,
		     proto_tree *tree)
{
  guint8 message_type;
  guint16 variable_pointer1 = 0, variable_pointer2 = 0, variable_pointer3 = 0;
  guint16 optional_pointer = 0;
  guint16 offset = 0;

/* Macro for getting pointer to mandatory variable parameters */
#define VARIABLE_POINTER(var, hf_var, ptr_size) \
    if (ptr_size == POINTER_LENGTH) \
	var = tvb_get_guint8(tvb, offset); \
    else \
	var = tvb_get_letohs(tvb, offset); \
    proto_tree_add_uint(sccp_tree, hf_var, tvb, \
			offset, ptr_size, var); \
    var += offset; \
    offset += ptr_size;

/* Macro for getting pointer to optional parameters */
#define OPTIONAL_POINTER(ptr_size) \
    if (ptr_size == POINTER_LENGTH) \
	optional_pointer = tvb_get_guint8(tvb, offset); \
    else \
	optional_pointer = tvb_get_letohs(tvb, offset); \
    proto_tree_add_uint(sccp_tree, hf_sccp_optional_pointer, tvb, \
			offset, ptr_size, optional_pointer); \
    optional_pointer += offset; \
    offset += ptr_size;


  /* Extract the message type;  all other processing is based on this */
  message_type   = tvb_get_guint8(tvb, MESSAGE_TYPE_OFFSET);
  offset = MESSAGE_TYPE_LENGTH;

  if (check_col(pinfo->cinfo, COL_INFO))
    col_add_fstr(pinfo->cinfo, COL_INFO, "%s ",
		    val_to_str(message_type, sccp_message_type_acro_values, "Unknown"));

  if (sccp_tree) {
    /* add the message type to the protocol tree */
    proto_tree_add_uint(sccp_tree, hf_sccp_message_type, tvb,
			MESSAGE_TYPE_OFFSET, MESSAGE_TYPE_LENGTH, message_type);

  };

  /* Starting a new message dissection; clear the global SSN values */
  called_ssn = INVALID_SSN;
  calling_ssn = INVALID_SSN;

  switch(message_type) {
  case MESSAGE_TYPE_CR:
    offset += dissect_sccp_parameter(tvb, pinfo, sccp_tree, tree,
				     PARAMETER_SOURCE_LOCAL_REFERENCE,
				     offset, SOURCE_LOCAL_REFERENCE_LENGTH);
    offset += dissect_sccp_parameter(tvb, pinfo, sccp_tree, tree,
				     PARAMETER_CLASS, offset,
				     PROTOCOL_CLASS_LENGTH);

    VARIABLE_POINTER(variable_pointer1, hf_sccp_variable_pointer1, POINTER_LENGTH)
    OPTIONAL_POINTER(POINTER_LENGTH)

    dissect_sccp_variable_parameter(tvb, pinfo, sccp_tree, tree,
				    PARAMETER_CALLED_PARTY_ADDRESS,
				    variable_pointer1);
    break;

  case MESSAGE_TYPE_CC:
    /*  TODO: connection has been established;  theoretically we could keep
     *  keep track of the SLR/DLR with the called/calling from the CR and
     *  track the connection (e.g., on subsequent messages regarding this
     *  SLR we could set the global vars "call*_ssn" so data could get
     *  sub-dissected).
     */
    offset += dissect_sccp_parameter(tvb, pinfo, sccp_tree, tree,
				     PARAMETER_DESTINATION_LOCAL_REFERENCE,
				     offset,
				     DESTINATION_LOCAL_REFERENCE_LENGTH);
    offset += dissect_sccp_parameter(tvb, pinfo, sccp_tree, tree,
				     PARAMETER_SOURCE_LOCAL_REFERENCE,
				     offset, SOURCE_LOCAL_REFERENCE_LENGTH);
    offset += dissect_sccp_parameter(tvb, pinfo, sccp_tree, tree,
				     PARAMETER_CLASS, offset,
				     PROTOCOL_CLASS_LENGTH);
    OPTIONAL_POINTER(POINTER_LENGTH)
    break;

  case MESSAGE_TYPE_CREF:
    offset += dissect_sccp_parameter(tvb, pinfo, sccp_tree, tree,
				     PARAMETER_DESTINATION_LOCAL_REFERENCE,
				     offset,
				     DESTINATION_LOCAL_REFERENCE_LENGTH);
    offset += dissect_sccp_parameter(tvb, pinfo, sccp_tree, tree,
				     PARAMETER_REFUSAL_CAUSE, offset,
				     REFUSAL_CAUSE_LENGTH);
    OPTIONAL_POINTER(POINTER_LENGTH)
    break;

  case MESSAGE_TYPE_RLSD:
    offset += dissect_sccp_parameter(tvb, pinfo, sccp_tree, tree,
				     PARAMETER_DESTINATION_LOCAL_REFERENCE,
				     offset,
				     DESTINATION_LOCAL_REFERENCE_LENGTH);
    offset += dissect_sccp_parameter(tvb, pinfo, sccp_tree, tree,
				     PARAMETER_SOURCE_LOCAL_REFERENCE,
				     offset, SOURCE_LOCAL_REFERENCE_LENGTH);
    offset += dissect_sccp_parameter(tvb, pinfo, sccp_tree, tree,
				     PARAMETER_RELEASE_CAUSE, offset,
				     RELEASE_CAUSE_LENGTH);

    OPTIONAL_POINTER(POINTER_LENGTH)
    break;

  case MESSAGE_TYPE_RLC:
    offset += dissect_sccp_parameter(tvb, pinfo, sccp_tree, tree,
				     PARAMETER_DESTINATION_LOCAL_REFERENCE,
				     offset,
				     DESTINATION_LOCAL_REFERENCE_LENGTH);
    offset += dissect_sccp_parameter(tvb, pinfo, sccp_tree, tree,
				     PARAMETER_SOURCE_LOCAL_REFERENCE,
				     offset, SOURCE_LOCAL_REFERENCE_LENGTH);
    break;

  case MESSAGE_TYPE_DT1:
    offset += dissect_sccp_parameter(tvb, pinfo, sccp_tree, tree,
				     PARAMETER_DESTINATION_LOCAL_REFERENCE,
				     offset,
				     DESTINATION_LOCAL_REFERENCE_LENGTH);
    offset += dissect_sccp_parameter(tvb, pinfo, sccp_tree, tree,
				     PARAMETER_SEGMENTING_REASSEMBLING,
				     offset, SEGMENTING_REASSEMBLING_LENGTH);

    VARIABLE_POINTER(variable_pointer1, hf_sccp_variable_pointer1, POINTER_LENGTH)
    dissect_sccp_variable_parameter(tvb, pinfo, sccp_tree, tree, PARAMETER_DATA,
				    variable_pointer1);
    break;

  case MESSAGE_TYPE_DT2:
    offset += dissect_sccp_parameter(tvb, pinfo, sccp_tree, tree,
				     PARAMETER_DESTINATION_LOCAL_REFERENCE,
				     offset,
				     DESTINATION_LOCAL_REFERENCE_LENGTH);
    offset += dissect_sccp_parameter(tvb, pinfo, sccp_tree, tree,
				     PARAMETER_SEQUENCING_SEGMENTING, offset,
				     SEQUENCING_SEGMENTING_LENGTH);
    break;

  case MESSAGE_TYPE_AK:
    offset += dissect_sccp_parameter(tvb, pinfo, sccp_tree, tree,
				     PARAMETER_DESTINATION_LOCAL_REFERENCE,
				     offset,
				     DESTINATION_LOCAL_REFERENCE_LENGTH);
    offset += dissect_sccp_parameter(tvb, pinfo, sccp_tree, tree,
				     PARAMETER_RECEIVE_SEQUENCE_NUMBER,
				     offset, RECEIVE_SEQUENCE_NUMBER_LENGTH);
    offset += dissect_sccp_parameter(tvb, pinfo, sccp_tree, tree,
				     PARAMETER_CREDIT, offset, CREDIT_LENGTH);
    break;

  case MESSAGE_TYPE_UDT:
    offset += dissect_sccp_parameter(tvb, pinfo, sccp_tree, tree,
				     PARAMETER_CLASS, offset,
				     PROTOCOL_CLASS_LENGTH);
    VARIABLE_POINTER(variable_pointer1, hf_sccp_variable_pointer1, POINTER_LENGTH)
    VARIABLE_POINTER(variable_pointer2, hf_sccp_variable_pointer2, POINTER_LENGTH)
    VARIABLE_POINTER(variable_pointer3, hf_sccp_variable_pointer3, POINTER_LENGTH)

    dissect_sccp_variable_parameter(tvb, pinfo, sccp_tree, tree,
				    PARAMETER_CALLED_PARTY_ADDRESS,
				    variable_pointer1);
    dissect_sccp_variable_parameter(tvb, pinfo, sccp_tree, tree,
				    PARAMETER_CALLING_PARTY_ADDRESS,
				    variable_pointer2);
    dissect_sccp_variable_parameter(tvb, pinfo, sccp_tree, tree, PARAMETER_DATA,
				    variable_pointer3);
    break;

  case MESSAGE_TYPE_UDTS:
    offset += dissect_sccp_parameter(tvb, pinfo, sccp_tree, tree,
				     PARAMETER_RETURN_CAUSE, offset,
				     RETURN_CAUSE_LENGTH);

    VARIABLE_POINTER(variable_pointer1, hf_sccp_variable_pointer1, POINTER_LENGTH)
    VARIABLE_POINTER(variable_pointer2, hf_sccp_variable_pointer2, POINTER_LENGTH)
    VARIABLE_POINTER(variable_pointer3, hf_sccp_variable_pointer3, POINTER_LENGTH)

    dissect_sccp_variable_parameter(tvb, pinfo, sccp_tree, tree,
				    PARAMETER_CALLED_PARTY_ADDRESS,
				    variable_pointer1);

    dissect_sccp_variable_parameter(tvb, pinfo, sccp_tree, tree,
				    PARAMETER_CALLING_PARTY_ADDRESS,
				    variable_pointer2);

    dissect_sccp_variable_parameter(tvb, pinfo, sccp_tree, tree, PARAMETER_DATA,
				    variable_pointer3);
    break;

  case MESSAGE_TYPE_ED:
    offset += dissect_sccp_parameter(tvb, pinfo, sccp_tree, tree,
				     PARAMETER_DESTINATION_LOCAL_REFERENCE,
				     offset,
				     DESTINATION_LOCAL_REFERENCE_LENGTH);

    VARIABLE_POINTER(variable_pointer1, hf_sccp_variable_pointer1, POINTER_LENGTH)
    dissect_sccp_variable_parameter(tvb, pinfo, sccp_tree, tree, PARAMETER_DATA,
				    variable_pointer1);
    break;

  case MESSAGE_TYPE_EA:
    offset += dissect_sccp_parameter(tvb, pinfo, sccp_tree, tree,
				     PARAMETER_DESTINATION_LOCAL_REFERENCE,
				     offset,
				     DESTINATION_LOCAL_REFERENCE_LENGTH);
    break;

  case MESSAGE_TYPE_RSR:
    offset += dissect_sccp_parameter(tvb, pinfo, sccp_tree, tree,
				     PARAMETER_DESTINATION_LOCAL_REFERENCE,
				     offset,
				     DESTINATION_LOCAL_REFERENCE_LENGTH);
    offset += dissect_sccp_parameter(tvb, pinfo, sccp_tree, tree,
				     PARAMETER_SOURCE_LOCAL_REFERENCE,
				     offset, SOURCE_LOCAL_REFERENCE_LENGTH);
    offset += dissect_sccp_parameter(tvb, pinfo, sccp_tree, tree,
				     PARAMETER_RESET_CAUSE, offset,
				     RESET_CAUSE_LENGTH);
    break;

  case MESSAGE_TYPE_RSC:
    offset += dissect_sccp_parameter(tvb, pinfo, sccp_tree, tree,
				     PARAMETER_DESTINATION_LOCAL_REFERENCE,
				     offset,
				     DESTINATION_LOCAL_REFERENCE_LENGTH);
    offset += dissect_sccp_parameter(tvb, pinfo, sccp_tree, tree,
				     PARAMETER_SOURCE_LOCAL_REFERENCE,
				     offset, SOURCE_LOCAL_REFERENCE_LENGTH);
    break;

  case MESSAGE_TYPE_ERR:
    offset += dissect_sccp_parameter(tvb, pinfo, sccp_tree, tree,
				     PARAMETER_DESTINATION_LOCAL_REFERENCE,
				     offset,
				     DESTINATION_LOCAL_REFERENCE_LENGTH);
    offset += dissect_sccp_parameter(tvb, pinfo, sccp_tree, tree,
				     PARAMETER_ERROR_CAUSE, offset,
				     ERROR_CAUSE_LENGTH);
    break;

  case MESSAGE_TYPE_IT:
    offset += dissect_sccp_parameter(tvb, pinfo, sccp_tree, tree,
				     PARAMETER_DESTINATION_LOCAL_REFERENCE,
				     offset,
				     DESTINATION_LOCAL_REFERENCE_LENGTH);
    offset += dissect_sccp_parameter(tvb, pinfo, sccp_tree, tree,
				     PARAMETER_SOURCE_LOCAL_REFERENCE,
				     offset, SOURCE_LOCAL_REFERENCE_LENGTH);
    offset += dissect_sccp_parameter(tvb, pinfo, sccp_tree, tree,
				     PARAMETER_CLASS, offset,
				     PROTOCOL_CLASS_LENGTH);
    offset += dissect_sccp_parameter(tvb, pinfo, sccp_tree, tree,
				     PARAMETER_SEQUENCING_SEGMENTING,
				     offset, SEQUENCING_SEGMENTING_LENGTH);
    offset += dissect_sccp_parameter(tvb, pinfo, sccp_tree, tree,
				     PARAMETER_CREDIT, offset, CREDIT_LENGTH);
    break;

  case MESSAGE_TYPE_XUDT:
    offset += dissect_sccp_parameter(tvb, pinfo, sccp_tree, tree,
				     PARAMETER_CLASS, offset,
				     PROTOCOL_CLASS_LENGTH);
    offset += dissect_sccp_parameter(tvb, pinfo, sccp_tree, tree,
				     PARAMETER_HOP_COUNTER, offset,
				     HOP_COUNTER_LENGTH);

    VARIABLE_POINTER(variable_pointer1, hf_sccp_variable_pointer1, POINTER_LENGTH)
    VARIABLE_POINTER(variable_pointer2, hf_sccp_variable_pointer2, POINTER_LENGTH)
    VARIABLE_POINTER(variable_pointer3, hf_sccp_variable_pointer3, POINTER_LENGTH)
    OPTIONAL_POINTER(POINTER_LENGTH)

    dissect_sccp_variable_parameter(tvb, pinfo, sccp_tree, tree,
				    PARAMETER_CALLED_PARTY_ADDRESS,
				    variable_pointer1);
    dissect_sccp_variable_parameter(tvb, pinfo, sccp_tree, tree,
				    PARAMETER_CALLING_PARTY_ADDRESS,
				    variable_pointer2);
    dissect_sccp_variable_parameter(tvb, pinfo, sccp_tree, tree, PARAMETER_DATA,
				    variable_pointer3);
    break;

  case MESSAGE_TYPE_XUDTS:
    offset += dissect_sccp_parameter(tvb, pinfo, sccp_tree, tree,
				     PARAMETER_RETURN_CAUSE, offset,
				     RETURN_CAUSE_LENGTH);
    offset += dissect_sccp_parameter(tvb, pinfo, sccp_tree, tree,
				     PARAMETER_HOP_COUNTER, offset,
				     HOP_COUNTER_LENGTH);

    VARIABLE_POINTER(variable_pointer1, hf_sccp_variable_pointer1, POINTER_LENGTH)
    VARIABLE_POINTER(variable_pointer2, hf_sccp_variable_pointer2, POINTER_LENGTH)
    VARIABLE_POINTER(variable_pointer3, hf_sccp_variable_pointer3, POINTER_LENGTH)
    OPTIONAL_POINTER(POINTER_LENGTH)

    dissect_sccp_variable_parameter(tvb, pinfo, sccp_tree, tree,
				    PARAMETER_CALLED_PARTY_ADDRESS,
				    variable_pointer1);
    dissect_sccp_variable_parameter(tvb, pinfo, sccp_tree, tree,
				    PARAMETER_CALLING_PARTY_ADDRESS,
				    variable_pointer2);
    dissect_sccp_variable_parameter(tvb, pinfo, sccp_tree, tree, PARAMETER_DATA,
				    variable_pointer3);
    break;

  case MESSAGE_TYPE_LUDT:
    if (mtp3_standard != ANSI_STANDARD)
    {
      offset += dissect_sccp_parameter(tvb, pinfo, sccp_tree, tree,
				       PARAMETER_CLASS, offset,
				       PROTOCOL_CLASS_LENGTH);
      offset += dissect_sccp_parameter(tvb, pinfo, sccp_tree, tree,
				       PARAMETER_HOP_COUNTER, offset,
				       HOP_COUNTER_LENGTH);

      VARIABLE_POINTER(variable_pointer1, hf_sccp_variable_pointer1, POINTER_LENGTH_LONG)
      VARIABLE_POINTER(variable_pointer2, hf_sccp_variable_pointer2, POINTER_LENGTH_LONG)
      VARIABLE_POINTER(variable_pointer3, hf_sccp_variable_pointer3, POINTER_LENGTH_LONG)
      OPTIONAL_POINTER(POINTER_LENGTH_LONG)

      dissect_sccp_variable_parameter(tvb, pinfo, sccp_tree, tree,
				      PARAMETER_CALLED_PARTY_ADDRESS,
				      variable_pointer1);
      dissect_sccp_variable_parameter(tvb, pinfo, sccp_tree, tree,
				      PARAMETER_CALLING_PARTY_ADDRESS,
				      variable_pointer2);
      dissect_sccp_variable_parameter(tvb, pinfo, sccp_tree, tree,
				      PARAMETER_LONG_DATA, variable_pointer3);
    } else
      dissect_sccp_unknown_message(tvb, sccp_tree);
    break;

  case MESSAGE_TYPE_LUDTS:
    if (mtp3_standard != ANSI_STANDARD)
    {
      offset += dissect_sccp_parameter(tvb, pinfo, sccp_tree, tree,
				       PARAMETER_RETURN_CAUSE, offset,
				       RETURN_CAUSE_LENGTH);
      offset += dissect_sccp_parameter(tvb, pinfo, sccp_tree, tree,
				       PARAMETER_HOP_COUNTER, offset,
				       HOP_COUNTER_LENGTH);

      VARIABLE_POINTER(variable_pointer1, hf_sccp_variable_pointer1, POINTER_LENGTH_LONG)
      VARIABLE_POINTER(variable_pointer2, hf_sccp_variable_pointer2, POINTER_LENGTH_LONG)
      VARIABLE_POINTER(variable_pointer3, hf_sccp_variable_pointer3, POINTER_LENGTH_LONG)
      OPTIONAL_POINTER(POINTER_LENGTH_LONG)

      dissect_sccp_variable_parameter(tvb, pinfo, sccp_tree, tree,
				      PARAMETER_CALLED_PARTY_ADDRESS,
				      variable_pointer1);
      dissect_sccp_variable_parameter(tvb, pinfo, sccp_tree, tree,
				      PARAMETER_CALLING_PARTY_ADDRESS,
				      variable_pointer2);
      dissect_sccp_variable_parameter(tvb, pinfo, sccp_tree, tree,
				      PARAMETER_LONG_DATA, variable_pointer3);
    } else
      dissect_sccp_unknown_message(tvb, sccp_tree);
    break;

  default:
    dissect_sccp_unknown_message(tvb, sccp_tree);
  }

  if (optional_pointer)
    dissect_sccp_optional_parameters(tvb, pinfo, sccp_tree, tree,
				     optional_pointer);

}

static void
dissect_sccp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
  proto_item *sccp_item;
  proto_tree *sccp_tree = NULL;

  /* Make entry in the Protocol column on summary display */
  if (check_col(pinfo->cinfo, COL_PROTOCOL))
    col_set_str(pinfo->cinfo, COL_PROTOCOL, "SCCP");

  /* In the interest of speed, if "tree" is NULL, don't do any work not
     necessary to generate protocol tree items. */
  if (tree) {
    /* create the sccp protocol tree */
    sccp_item = proto_tree_add_item(tree, proto_sccp, tvb, 0, -1, FALSE);
    sccp_tree = proto_item_add_subtree(sccp_item, ett_sccp);
  }

  /* dissect the message */
  dissect_sccp_message(tvb, pinfo, sccp_tree, tree);
}

/* Register the protocol with Ethereal */
void
proto_register_sccp(void)
{
  /* Setup list of header fields */
  static hf_register_info hf[] = {
    { &hf_sccp_message_type,
      { "Message Type", "sccp.message_type",
	FT_UINT8, BASE_HEX, VALS(sccp_message_type_values), 0x0,
	"", HFILL}},
    { &hf_sccp_variable_pointer1,
      { "Pointer to first Mandatory Variable parameter", "sccp.variable_pointer1",
	FT_UINT16, BASE_DEC, NULL, 0x0,
	"", HFILL}},
    { &hf_sccp_variable_pointer2,
      { "Pointer to second Mandatory Variable parameter", "sccp.variable_pointer2",
	FT_UINT16, BASE_DEC, NULL, 0x0,
	"", HFILL}},
    { &hf_sccp_variable_pointer3,
      { "Pointer to third Mandatory Variable parameter", "sccp.variable_pointer3",
	FT_UINT16, BASE_DEC, NULL, 0x0,
	"", HFILL}},
    { &hf_sccp_optional_pointer,
      { "Pointer to Optional parameter", "sccp.optional_pointer",
	FT_UINT16, BASE_DEC, NULL, 0x0,
	"", HFILL}},
    { &hf_sccp_ssn,
      { "Called or Calling SubSystem Number", "sccp.ssn",
	FT_UINT8, BASE_DEC, VALS(sccp_ssn_values), 0x0,
	"", HFILL}},
    { &hf_sccp_gt_digits,
      { "Called or Calling GT Digits",
	"sccp.digits",
	FT_STRING, BASE_NONE, NULL, 0x0,
	"", HFILL }},

    { &hf_sccp_called_national_indicator,
      { "National Indicator", "sccp.called.ni",
	FT_UINT8, BASE_HEX, VALS(sccp_national_indicator_values), ANSI_NATIONAL_MASK,
	"", HFILL}},
    { &hf_sccp_called_routing_indicator,
      { "Routing Indicator", "sccp.called.ri",
	FT_UINT8, BASE_HEX, VALS(sccp_routing_indicator_values), ROUTING_INDICATOR_MASK,
	"", HFILL}},
    { &hf_sccp_called_itu_global_title_indicator,
      { "Global Title Indicator", "sccp.called.gti",
	FT_UINT8, BASE_HEX, VALS(sccp_itu_global_title_indicator_values), GTI_MASK,
	"", HFILL}},
    { &hf_sccp_called_ansi_global_title_indicator,
      { "Global Title Indicator", "sccp.called.gti",
	FT_UINT8, BASE_HEX, VALS(sccp_ansi_global_title_indicator_values), GTI_MASK,
	"", HFILL}},
    { &hf_sccp_called_itu_ssn_indicator,
      { "SubSystem Number Indicator", "sccp.called.ssni",
	FT_UINT8, BASE_HEX, VALS(sccp_ai_ssni_values), ITU_SSN_INDICATOR_MASK,
	"", HFILL}},
    { &hf_sccp_called_itu_point_code_indicator,
      { "Point Code Indicator", "sccp.called.pci",
	FT_UINT8, BASE_HEX, VALS(sccp_ai_pci_values), ITU_PC_INDICATOR_MASK,
	"", HFILL}},
    { &hf_sccp_called_ansi_ssn_indicator,
      { "SubSystem Number Indicator", "sccp.called.ssni",
	FT_UINT8, BASE_HEX, VALS(sccp_ai_ssni_values), ANSI_SSN_INDICATOR_MASK,
	"", HFILL}},
    { &hf_sccp_called_ansi_point_code_indicator,
      { "Point Code Indicator", "sccp.called.pci",
	FT_UINT8, BASE_HEX, VALS(sccp_ai_pci_values), ANSI_PC_INDICATOR_MASK,
	"", HFILL}},
    { &hf_sccp_called_ssn,
      { "SubSystem Number", "sccp.called.ssn",
	FT_UINT8, BASE_DEC, VALS(sccp_ssn_values), 0x0,
	"", HFILL}},
    { &hf_sccp_called_itu_pc,
      { "PC", "sccp.called.pc",
	FT_UINT16, BASE_DEC, NULL, ITU_PC_MASK,
	"", HFILL}},
    { &hf_sccp_called_ansi_pc,
      { "PC", "sccp.called.ansi_pc",
	FT_STRING, BASE_NONE, NULL, 0x0,
	"", HFILL}},
    { &hf_sccp_called_chinese_pc,
      { "PC", "sccp.called.chinese_pc",
	FT_STRING, BASE_NONE, NULL, 0x0,
	"", HFILL}},
    { &hf_sccp_called_pc_network,
      { "PC Network",
	"sccp.called.network",
	FT_UINT24, BASE_DEC, NULL, ANSI_NETWORK_MASK,
	"", HFILL }},
    { &hf_sccp_called_pc_cluster,
      { "PC Cluster",
	"sccp.called.cluster",
	FT_UINT24, BASE_DEC, NULL, ANSI_CLUSTER_MASK,
	"", HFILL }},
    { &hf_sccp_called_pc_member,
      { "PC Member",
	"sccp.called.member",
	FT_UINT24, BASE_DEC, NULL, ANSI_MEMBER_MASK,
	"", HFILL }},
    { &hf_sccp_called_gt_nai,
      { "Nature of Address Indicator",
	"sccp.called.nai",
	FT_UINT8, BASE_HEX, VALS(sccp_nai_values), GT_NAI_MASK,
	"", HFILL }},
    { &hf_sccp_called_gt_oe,
      { "Odd/Even Indicator",
	"sccp.called.oe",
	FT_UINT8, BASE_HEX, VALS(sccp_oe_values), GT_OE_MASK,
	"", HFILL }},
    { &hf_sccp_called_gt_tt,
      { "Translation Type",
	"sccp.called.tt",
	FT_UINT8, BASE_HEX, NULL, 0x0,
	"", HFILL }},
    { &hf_sccp_called_gt_np,
      { "Numbering Plan",
	"sccp.called.np",
	FT_UINT8, BASE_HEX, VALS(sccp_np_values), GT_NP_MASK,
	"", HFILL }},
    { &hf_sccp_called_gt_es,
      { "Encoding Scheme",
	"sccp.called.es",
	FT_UINT8, BASE_HEX, VALS(sccp_es_values), GT_ES_MASK,
	"", HFILL }},
    { &hf_sccp_called_gt_digits,
      { "GT Digits",
	"sccp.called.digits",
	FT_STRING, BASE_NONE, NULL, 0x0,
	"", HFILL }},

    { &hf_sccp_calling_national_indicator,
      { "National Indicator", "sccp.calling.ni",
	FT_UINT8, BASE_HEX, VALS(sccp_national_indicator_values), ANSI_NATIONAL_MASK,
	"", HFILL}},
    { &hf_sccp_calling_routing_indicator,
      { "Routing Indicator", "sccp.calling.ri",
	FT_UINT8, BASE_HEX, VALS(sccp_routing_indicator_values), ROUTING_INDICATOR_MASK,
	"", HFILL}},
    { &hf_sccp_calling_itu_global_title_indicator,
      { "Global Title Indicator", "sccp.calling.gti",
	FT_UINT8, BASE_HEX, VALS(sccp_itu_global_title_indicator_values), GTI_MASK,
	"", HFILL}},
    { &hf_sccp_calling_ansi_global_title_indicator,
      { "Global Title Indicator", "sccp.calling.gti",
	FT_UINT8, BASE_HEX, VALS(sccp_ansi_global_title_indicator_values), GTI_MASK,
	"", HFILL}},
    { &hf_sccp_calling_itu_ssn_indicator,
      { "SubSystem Number Indicator", "sccp.calling.ssni",
	FT_UINT8, BASE_HEX, VALS(sccp_ai_ssni_values), ITU_SSN_INDICATOR_MASK,
	"", HFILL}},
    { &hf_sccp_calling_itu_point_code_indicator,
      { "Point Code Indicator", "sccp.calling.pci",
	FT_UINT8, BASE_HEX, VALS(sccp_ai_pci_values), ITU_PC_INDICATOR_MASK,
	"", HFILL}},
    { &hf_sccp_calling_ansi_ssn_indicator,
      { "SubSystem Number Indicator", "sccp.calling.ssni",
	FT_UINT8, BASE_HEX, VALS(sccp_ai_ssni_values), ANSI_SSN_INDICATOR_MASK,
	"", HFILL}},
    { &hf_sccp_calling_ansi_point_code_indicator,
      { "Point Code Indicator", "sccp.calling.pci",
	FT_UINT8, BASE_HEX, VALS(sccp_ai_pci_values), ANSI_PC_INDICATOR_MASK,
	"", HFILL}},
    { &hf_sccp_calling_ssn,
      { "SubSystem Number", "sccp.calling.ssn",
	FT_UINT8, BASE_DEC, VALS(sccp_ssn_values), 0x0,
	"", HFILL}},
    { &hf_sccp_calling_itu_pc,
      { "PC", "sccp.calling.pc",
	FT_UINT16, BASE_DEC, NULL, ITU_PC_MASK,
	"", HFILL}},
    { &hf_sccp_calling_ansi_pc,
      { "PC", "sccp.calling.ansi_pc",
	FT_STRING, BASE_NONE, NULL, 0x0,
	"", HFILL}},
    { &hf_sccp_calling_chinese_pc,
      { "PC", "sccp.calling.chinese_pc",
	FT_STRING, BASE_NONE, NULL, 0x0,
	"", HFILL}},
    { &hf_sccp_calling_pc_network,
      { "PC Network",
	"sccp.calling.network",
	FT_UINT24, BASE_DEC, NULL, ANSI_NETWORK_MASK,
	"", HFILL }},
    { &hf_sccp_calling_pc_cluster,
      { "PC Cluster",
	"sccp.calling.cluster",
	FT_UINT24, BASE_DEC, NULL, ANSI_CLUSTER_MASK,
	"", HFILL }},
    { &hf_sccp_calling_pc_member,
      { "PC Member",
	"sccp.calling.member",
	FT_UINT24, BASE_DEC, NULL, ANSI_MEMBER_MASK,
	"", HFILL }},
    { &hf_sccp_calling_gt_nai,
      { "Nature of Address Indicator",
	"sccp.calling.nai",
	FT_UINT8, BASE_HEX, VALS(sccp_nai_values), GT_NAI_MASK,
	"", HFILL }},
    { &hf_sccp_calling_gt_oe,
      { "Odd/Even Indicator",
	"sccp.calling.oe",
	FT_UINT8, BASE_HEX, VALS(sccp_oe_values), GT_OE_MASK,
	"", HFILL }},
    { &hf_sccp_calling_gt_tt,
      { "Translation Type",
	"sccp.calling.tt",
	FT_UINT8, BASE_HEX, NULL, 0x0,
	"", HFILL }},
    { &hf_sccp_calling_gt_np,
      { "Numbering Plan",
	"sccp.calling.np",
	FT_UINT8, BASE_HEX, VALS(sccp_np_values), GT_NP_MASK,
	"", HFILL }},
    { &hf_sccp_calling_gt_es,
      { "Encoding Scheme",
	"sccp.calling.es",
	FT_UINT8, BASE_HEX, VALS(sccp_es_values), GT_ES_MASK,
	"", HFILL }},
    { &hf_sccp_calling_gt_digits,
      { "GT Digits",
	"sccp.calling.digits",
	FT_STRING, BASE_NONE, NULL, 0x0,
	"", HFILL }},

    { &hf_sccp_dlr,
      { "Destination Local Reference", "sccp.dlr",
	FT_UINT24, BASE_HEX, NULL, 0x0,
	"", HFILL}},
    { &hf_sccp_slr,
      { "Source Local Reference", "sccp.slr",
	FT_UINT24, BASE_HEX, NULL, 0x0,
	"", HFILL}},
    { &hf_sccp_class,
      { "Class", "sccp.class",
	FT_UINT8, BASE_HEX, NULL, CLASS_CLASS_MASK,
	"", HFILL}},
    { &hf_sccp_handling,
      { "Message handling", "sccp.handling",
	FT_UINT8, BASE_HEX, VALS(sccp_class_handling_values), CLASS_SPARE_HANDLING_MASK,
	"", HFILL}},
    { &hf_sccp_more,
      { "More data", "sccp.more",
	FT_UINT8, BASE_HEX, VALS(sccp_segmenting_reassembling_values), SEGMENTING_REASSEMBLING_MASK,
	"", HFILL}},
    { &hf_sccp_rsn,
      { "Receive Sequence Number", "sccp.rsn",
	FT_UINT8, BASE_HEX, NULL, RSN_MASK,
	"", HFILL}},
    { &hf_sccp_sequencing_segmenting_ssn,
      { "Sequencing Segmenting: Send Sequence Number", "sccp.sequencing_segmenting.ssn",
	FT_UINT8, BASE_HEX, NULL, SEND_SEQUENCE_NUMBER_MASK,
	"", HFILL}},
    { &hf_sccp_sequencing_segmenting_rsn,
      { "Sequencing Segmenting: Receive Sequence Number", "sccp.sequencing_segmenting.rsn",
	FT_UINT8, BASE_HEX, NULL, RECEIVE_SEQUENCE_NUMBER_MASK,
	"", HFILL}},
    { &hf_sccp_sequencing_segmenting_more,
      { "Sequencing Segmenting: More", "sccp.sequencing_segmenting.more",
	FT_UINT8, BASE_HEX, VALS(sccp_segmenting_reassembling_values), SEQUENCING_SEGMENTING_MORE_MASK,
	"", HFILL}},
    { &hf_sccp_credit,
      { "Credit", "sccp.credit",
	FT_UINT8, BASE_HEX, NULL, 0x0,
	"", HFILL}},
    { &hf_sccp_release_cause,
      { "Release Cause", "sccp.release_cause",
	FT_UINT8, BASE_HEX, VALS(sccp_release_cause_values), 0x0,
	"", HFILL}},
    { &hf_sccp_return_cause,
      { "Return Cause", "sccp.return_cause",
	FT_UINT8, BASE_HEX, VALS(sccp_return_cause_values), 0x0,
	"", HFILL}},
    { &hf_sccp_reset_cause,
      { "Reset Cause", "sccp.reset_cause",
	FT_UINT8, BASE_HEX, VALS(sccp_reset_cause_values), 0x0,
	"", HFILL}},
    { &hf_sccp_error_cause,
      { "Error Cause", "sccp.error_cause",
	FT_UINT8, BASE_HEX, VALS(sccp_error_cause_values), 0x0,
	"", HFILL}},
    { &hf_sccp_refusal_cause,
      { "Refusal Cause", "sccp.refusal_cause",
	FT_UINT8, BASE_HEX, VALS(sccp_refusal_cause_values), 0x0,
	"", HFILL}},
    { &hf_sccp_segmentation_first,
      { "Segmentation: First", "sccp.segmentation.first",
	FT_UINT8, BASE_HEX, VALS(sccp_segmentation_first_segment_values), SEGMENTATION_FIRST_SEGMENT_MASK,
	"", HFILL}},
    { &hf_sccp_segmentation_class,
      { "Segmentation: Class", "sccp.segmentation.class",
	FT_UINT8, BASE_HEX, VALS(sccp_segmentation_class_values), SEGMENTATION_CLASS_MASK,
	"", HFILL}},
    { &hf_sccp_segmentation_remaining,
      { "Segmentation: Remaining", "sccp.segmentation.remaining",
	FT_UINT8, BASE_HEX, NULL, SEGMENTATION_REMAINING_MASK,
	"", HFILL}},
    { &hf_sccp_segmentation_slr,
      { "Segmentation: Source Local Reference", "sccp.segmentation.slr",
	FT_UINT24, BASE_HEX, NULL, 0x0,
	"", HFILL}},
    { &hf_sccp_hop_counter,
      { "Hop Counter", "sccp.hops",
	FT_UINT8, BASE_HEX, NULL, 0x0,
	"", HFILL}},
    { &hf_sccp_importance,
      { "Importance", "sccp.importance",
	FT_UINT8, BASE_HEX, NULL, IMPORTANCE_IMPORTANCE_MASK,
	"", HFILL}},

    /* ISNI is ANSI only */
    { &hf_sccp_ansi_isni_mi,
      { "ISNI Mark for Identification Indicator", "sccp.isni.mi",
	FT_UINT8, BASE_HEX, NULL, ANSI_ISNI_MI_MASK,
	"", HFILL}},
    { &hf_sccp_ansi_isni_iri,
      { "ISNI Routing Indicator", "sccp.isni.iri",
	FT_UINT8, BASE_HEX, NULL, ANSI_ISNI_IRI_MASK,
	"", HFILL}},
    { &hf_sccp_ansi_isni_ti,
      { "ISNI Type Indicator", "sccp.isni.ti",
	FT_UINT8, BASE_HEX, NULL, ANSI_ISNI_TI_MASK,
	"", HFILL}},
    { &hf_sccp_ansi_isni_netspec,
      { "ISNI Network Specific (Type 1)", "sccp.isni.netspec",
	FT_UINT8, BASE_HEX, NULL, ANSI_ISNI_NETSPEC_MASK,
	"", HFILL}},
    { &hf_sccp_ansi_isni_counter,
      { "ISNI Counter", "sccp.isni.counter",
	FT_UINT8, BASE_HEX, NULL, ANSI_ISNI_COUNTER_MASK,
	"", HFILL}},
  };

  /* Setup protocol subtree array */
  static gint *ett[] = {
    &ett_sccp,
    &ett_sccp_called,
    &ett_sccp_called_ai,
    &ett_sccp_called_pc,
    &ett_sccp_called_gt,
    &ett_sccp_calling,
    &ett_sccp_calling_ai,
    &ett_sccp_calling_pc,
    &ett_sccp_calling_gt,
    &ett_sccp_sequencing_segmenting,
    &ett_sccp_segmentation,
    &ett_sccp_ansi_isni_routing_control,
  };

  /* Register the protocol name and description */
  proto_sccp = proto_register_protocol("Signalling Connection Control Part",
				       "SCCP", "sccp");

  /* Required function calls to register the header fields and subtrees used */
  proto_register_field_array(proto_sccp, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));

  sccp_ssn_dissector_table = register_dissector_table("sccp.ssn", "SCCP SSN", FT_UINT8, BASE_DEC);

}

void
proto_reg_handoff_sccp(void)
{
  dissector_handle_t sccp_handle;

  sccp_handle = create_dissector_handle(dissect_sccp, proto_sccp);

  dissector_add("mtp3.service_indicator", SCCP_SI, sccp_handle);
  dissector_add("m3ua.protocol_data_si", SCCP_SI, sccp_handle);

  data_handle = find_dissector("data");
}
