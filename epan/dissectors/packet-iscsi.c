/* TODO for the cases where one just can not autodetect whether header digest 
   is used or not we might need a new preference 
   HeaderDigest : 
       Automatic (default)
       None
       CRC32
*/
 
/* packet-iscsi.c
 * Routines for iSCSI dissection
 * Copyright 2001, Eurologic and Mark Burton <markb@ordern.com>
 *  2004 Request/Response matching and Service Response Time: ronnie sahlberg
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

#include <epan/packet.h>
#include <epan/prefs.h>
#include <epan/conversation.h>
#include "packet-scsi.h"
#include "epan/nstime.h"

/* the absolute values of these constants don't matter as long as
 * latter revisions of the protocol are assigned a larger number */
#define ISCSI_PROTOCOL_DRAFT08 1
#define ISCSI_PROTOCOL_DRAFT09 2
#define ISCSI_PROTOCOL_DRAFT11 3
#define ISCSI_PROTOCOL_DRAFT12 4
#define ISCSI_PROTOCOL_DRAFT13 5

static enum_val_t iscsi_protocol_versions[] = {
    { "draft-08", "Draft 08", ISCSI_PROTOCOL_DRAFT08 },
    { "draft-09", "Draft 09", ISCSI_PROTOCOL_DRAFT09 },
    { "draft-11", "Draft 11", ISCSI_PROTOCOL_DRAFT11 },
    { "draft-12", "Draft 12", ISCSI_PROTOCOL_DRAFT12 },
    { "draft-13", "Draft 13", ISCSI_PROTOCOL_DRAFT13 },
    { NULL, NULL, 0 }
};

static dissector_handle_t iscsi_handle=NULL;

static gint iscsi_protocol_version = ISCSI_PROTOCOL_DRAFT13;

static gboolean iscsi_desegment = TRUE;

static int demand_good_f_bit = FALSE;
static int enable_bogosity_filter = TRUE;
static guint32 bogus_pdu_data_length_threshold = 256 * 1024;

static int enableDataDigests = FALSE;

static int dataDigestIsCRC32 = TRUE;

static int dataDigestSize = 4;

static guint iscsi_port = 3260;

/* Initialize the protocol and registered fields */
static int proto_iscsi = -1;
static int hf_iscsi_time = -1;
static int hf_iscsi_request_frame = -1;
static int hf_iscsi_data_in_frame = -1;
static int hf_iscsi_data_out_frame = -1;
static int hf_iscsi_response_frame = -1;
static int hf_iscsi_AHS = -1;
static int hf_iscsi_Padding = -1;
static int hf_iscsi_ping_data = -1;
static int hf_iscsi_immediate_data = -1;
static int hf_iscsi_write_data = -1;
static int hf_iscsi_read_data = -1;
static int hf_iscsi_error_pdu_data = -1;
static int hf_iscsi_async_message_data = -1;
static int hf_iscsi_vendor_specific_data = -1;
static int hf_iscsi_Opcode = -1;
static int hf_iscsi_Flags = -1;
static int hf_iscsi_HeaderDigest32 = -1;
static int hf_iscsi_DataDigest = -1;
static int hf_iscsi_DataDigest32 = -1;
/* #ifdef DRAFT08 */
static int hf_iscsi_X = -1;
/* #endif */
static int hf_iscsi_I = -1;
static int hf_iscsi_SCSICommand_F = -1;
static int hf_iscsi_SCSICommand_R = -1;
static int hf_iscsi_SCSICommand_W = -1;
static int hf_iscsi_SCSICommand_Attr = -1;
static int hf_iscsi_SCSICommand_CRN = -1;
static int hf_iscsi_SCSICommand_AddCDB = -1;
static int hf_iscsi_DataSegmentLength = -1;
static int hf_iscsi_TotalAHSLength = -1;
static int hf_iscsi_LUN = -1;
static int hf_iscsi_InitiatorTaskTag = -1;
static int hf_iscsi_ExpectedDataTransferLength = -1;
static int hf_iscsi_CmdSN = -1;
static int hf_iscsi_ExpStatSN = -1;
static int hf_iscsi_StatSN = -1;
static int hf_iscsi_ExpCmdSN = -1;
static int hf_iscsi_MaxCmdSN = -1;
static int hf_iscsi_SCSIResponse_o = -1;
static int hf_iscsi_SCSIResponse_u = -1;
static int hf_iscsi_SCSIResponse_O = -1;
static int hf_iscsi_SCSIResponse_U = -1;
static int hf_iscsi_SCSIResponse_BidiReadResidualCount = -1;
static int hf_iscsi_SCSIResponse_ResidualCount = -1;
static int hf_iscsi_SCSIResponse_Response = -1;
static int hf_iscsi_SCSIResponse_Status = -1;
static int hf_iscsi_SenseLength = -1;
static int hf_iscsi_SCSIData_F = -1;
static int hf_iscsi_SCSIData_A = -1;
static int hf_iscsi_SCSIData_S = -1;
static int hf_iscsi_SCSIData_O = -1;
static int hf_iscsi_SCSIData_U = -1;
static int hf_iscsi_TargetTransferTag = -1;
static int hf_iscsi_DataSN = -1;
static int hf_iscsi_BufferOffset = -1;
static int hf_iscsi_SCSIData_ResidualCount = -1;
static int hf_iscsi_VersionMin = -1;
static int hf_iscsi_VersionMax = -1;
static int hf_iscsi_VersionActive = -1;
static int hf_iscsi_CID = -1;
static int hf_iscsi_ISID8 = -1;
static int hf_iscsi_ISID = -1;
/* #if defined(DRAFT09) */
static int hf_iscsi_ISID_Type = -1;
static int hf_iscsi_ISID_NamingAuthority = -1;
static int hf_iscsi_ISID_Qualifier = -1;
/* #elif !defined(DRAFT08) */
static int hf_iscsi_ISID_t = -1;
static int hf_iscsi_ISID_a = -1;
static int hf_iscsi_ISID_b = -1;
static int hf_iscsi_ISID_c = -1;
static int hf_iscsi_ISID_d = -1;
/* #endif */
static int hf_iscsi_TSID = -1;
static int hf_iscsi_TSIH = -1;
static int hf_iscsi_InitStatSN = -1;
static int hf_iscsi_InitCmdSN = -1;
/* #ifdef DRAFT09 */
static int hf_iscsi_Login_X = -1;
/* #endif */
static int hf_iscsi_Login_C = -1;
static int hf_iscsi_Login_T = -1;
static int hf_iscsi_Login_CSG = -1;
static int hf_iscsi_Login_NSG = -1;
static int hf_iscsi_Login_Status = -1;
static int hf_iscsi_KeyValue = -1;
static int hf_iscsi_Text_C = -1;
static int hf_iscsi_Text_F = -1;
static int hf_iscsi_ExpDataSN = -1;
static int hf_iscsi_R2TSN = -1;
static int hf_iscsi_TaskManagementFunction_ReferencedTaskTag = -1;
static int hf_iscsi_RefCmdSN = -1;
static int hf_iscsi_TaskManagementFunction_Function = -1;
static int hf_iscsi_TaskManagementFunction_Response = -1;
static int hf_iscsi_Logout_Reason = -1;
static int hf_iscsi_Logout_Response = -1;
static int hf_iscsi_Time2Wait = -1;
static int hf_iscsi_Time2Retain = -1;
static int hf_iscsi_DesiredDataLength = -1;
static int hf_iscsi_AsyncEvent = -1;
static int hf_iscsi_EventVendorCode = -1;
static int hf_iscsi_Parameter1 = -1;
static int hf_iscsi_Parameter2 = -1;
static int hf_iscsi_Parameter3 = -1;
static int hf_iscsi_Reject_Reason = -1;
static int hf_iscsi_snack_type = -1;
static int hf_iscsi_BegRun = -1;
static int hf_iscsi_RunLength = -1;

/* Initialize the subtree pointers */
static gint ett_iscsi = -1;
static gint ett_iscsi_KeyValues = -1;
static gint ett_iscsi_CDB = -1;
static gint ett_iscsi_Flags = -1;
/* #ifndef DRAFT08 */
static gint ett_iscsi_ISID = -1;
/* #endif */

#define ISCSI_HEADER_DIGEST_AUTO	0
#define ISCSI_HEADER_DIGEST_NONE	1
#define ISCSI_HEADER_DIGEST_CRC32	2
/* this structure contains session wide state for all iscsi sessions */
typedef struct _iscsi_session_t {
	guint32	conv_idx;	/* unique ID for conversation */
	guint32 header_digest;
} iscsi_session_t;
static GHashTable *iscsi_session_table = NULL;
static GMemChunk *iscsi_sessions = NULL;
static gint
iscsi_session_equal(gconstpointer v, gconstpointer w)
{
  const iscsi_session_t *v1 = (const iscsi_session_t *)v;
  const iscsi_session_t *v2 = (const iscsi_session_t *)w;

  return (v1->conv_idx == v2->conv_idx);
}

static guint
iscsi_session_hash (gconstpointer v)
{
	const iscsi_session_t *key = (const iscsi_session_t *)v;

	return key->conv_idx;
}



/* #ifdef DRAFT08 */
#define X_BIT 0x80
/* #endif */

#define I_BIT 0x40

#define OPCODE_MASK 0x3f

#define TARGET_OPCODE_BIT 0x20

#define ISCSI_OPCODE_NOP_OUT                  0x00
#define ISCSI_OPCODE_SCSI_COMMAND             0x01
#define ISCSI_OPCODE_TASK_MANAGEMENT_FUNCTION 0x02
#define ISCSI_OPCODE_LOGIN_COMMAND            0x03
#define ISCSI_OPCODE_TEXT_COMMAND             0x04
#define ISCSI_OPCODE_SCSI_DATA_OUT            0x05
#define ISCSI_OPCODE_LOGOUT_COMMAND           0x06
#define ISCSI_OPCODE_SNACK_REQUEST            0x10
#define ISCSI_OPCODE_VENDOR_SPECIFIC_I0       0x1c
#define ISCSI_OPCODE_VENDOR_SPECIFIC_I1       0x1d
#define ISCSI_OPCODE_VENDOR_SPECIFIC_I2       0x1e

#define ISCSI_OPCODE_NOP_IN                            0x20
#define ISCSI_OPCODE_SCSI_RESPONSE                     0x21
#define ISCSI_OPCODE_TASK_MANAGEMENT_FUNCTION_RESPONSE 0x22
#define ISCSI_OPCODE_LOGIN_RESPONSE                    0x23
#define ISCSI_OPCODE_TEXT_RESPONSE                     0x24
#define ISCSI_OPCODE_SCSI_DATA_IN                      0x25
#define ISCSI_OPCODE_LOGOUT_RESPONSE                   0x26
#define ISCSI_OPCODE_R2T                               0x31
#define ISCSI_OPCODE_ASYNC_MESSAGE                     0x32
#define ISCSI_OPCODE_REJECT                            0x3f
#define ISCSI_OPCODE_VENDOR_SPECIFIC_T0                0x3c
#define ISCSI_OPCODE_VENDOR_SPECIFIC_T1                0x3d
#define ISCSI_OPCODE_VENDOR_SPECIFIC_T2                0x3e

#define CSG_SHIFT 2
#define CSG_MASK  (0x03 << CSG_SHIFT)
#define NSG_MASK  0x03

#define ISCSI_CSG_SECURITY_NEGOTIATION    (0 << CSG_SHIFT)
#define ISCSI_CSG_OPERATIONAL_NEGOTIATION (1 << CSG_SHIFT)
#define ISCSI_CSG_FULL_FEATURE_PHASE      (3 << CSG_SHIFT)

#define ISCSI_SCSI_DATA_FLAG_S 0x01
#define ISCSI_SCSI_DATA_FLAG_U 0x02
#define ISCSI_SCSI_DATA_FLAG_O 0x04
#define ISCSI_SCSI_DATA_FLAG_A 0x40
#define ISCSI_SCSI_DATA_FLAG_F 0x80

static const value_string iscsi_opcodes[] = {
  { ISCSI_OPCODE_NOP_OUT,                           "NOP Out" },
  { ISCSI_OPCODE_SCSI_COMMAND,                      "SCSI Command" },
  { ISCSI_OPCODE_TASK_MANAGEMENT_FUNCTION,          "Task Management Function" },
  { ISCSI_OPCODE_LOGIN_COMMAND,                     "Login Command" },
  { ISCSI_OPCODE_TEXT_COMMAND,                      "Text Command" },
  { ISCSI_OPCODE_SCSI_DATA_OUT,                     "SCSI Data Out" },
  { ISCSI_OPCODE_LOGOUT_COMMAND,                    "Logout Command" },
  { ISCSI_OPCODE_SNACK_REQUEST,                     "SNACK Request" },
  { ISCSI_OPCODE_VENDOR_SPECIFIC_I0,                "Vendor Specific I0" },
  { ISCSI_OPCODE_VENDOR_SPECIFIC_I1,                "Vendor Specific I1" },
  { ISCSI_OPCODE_VENDOR_SPECIFIC_I2,                "Vendor Specific I2" },

  { ISCSI_OPCODE_NOP_IN,                            "NOP In" },
  { ISCSI_OPCODE_SCSI_RESPONSE,                     "SCSI Response" },
  { ISCSI_OPCODE_TASK_MANAGEMENT_FUNCTION_RESPONSE, "Task Management Function Response" },
  { ISCSI_OPCODE_LOGIN_RESPONSE,                    "Login Response" },
  { ISCSI_OPCODE_TEXT_RESPONSE,                     "Text Response" },
  { ISCSI_OPCODE_SCSI_DATA_IN,                      "SCSI Data In" },
  { ISCSI_OPCODE_LOGOUT_RESPONSE,                   "Logout Response" },
  { ISCSI_OPCODE_R2T,                               "Ready To Transfer" },
  { ISCSI_OPCODE_ASYNC_MESSAGE,                     "Asynchronous Message" },
  { ISCSI_OPCODE_REJECT,                            "Reject"},
  { ISCSI_OPCODE_VENDOR_SPECIFIC_T0,                "Vendor Specific T0" },
  { ISCSI_OPCODE_VENDOR_SPECIFIC_T1,                "Vendor Specific T1" },
  { ISCSI_OPCODE_VENDOR_SPECIFIC_T2,                "Vendor Specific T2" },
  {0, NULL},
};

/* #ifdef DRAFT08 */
static const true_false_string iscsi_meaning_X = {
    "Retry",
    "Not retry"
};
/* #endif */

/* #ifdef DRAFT09 */
static const true_false_string iscsi_meaning_login_X = {
    "Reinstate failed connection",
    "New connection"
};
/* #endif */

static const true_false_string iscsi_meaning_I = {
    "Immediate delivery",
    "Queued delivery"
};

static const true_false_string iscsi_meaning_F = {
    "Final PDU in sequence",
    "Not final PDU in sequence"
};

static const true_false_string iscsi_meaning_A = {
    "Acknowledge requested",
    "Acknowledge not requested"
};

static const true_false_string iscsi_meaning_T = {
    "Transit to next login stage",
    "Stay in current login stage"
};

static const true_false_string iscsi_meaning_C = {
    "Text is incomplete",
    "Text is complete"
};

static const true_false_string iscsi_meaning_S = {
    "Response contains SCSI status",
    "Response does not contain SCSI status"
};

static const true_false_string iscsi_meaning_R = {
    "Data will be read from target",
    "No data will be read from target"
};

static const true_false_string iscsi_meaning_W = {
    "Data will be written to target",
    "No data will be written to target"
};

static const true_false_string iscsi_meaning_o = {
    "Read part of bi-directional command overflowed",
    "No overflow of read part of bi-directional command",
};

static const true_false_string iscsi_meaning_u = {
    "Read part of bi-directional command underflowed",
    "No underflow of read part of bi-directional command",
};

static const true_false_string iscsi_meaning_O = {
    "Residual overflow occurred",
    "No residual overflow occurred",
};

static const true_false_string iscsi_meaning_U = {
    "Residual underflow occurred",
    "No residual underflow occurred",
};

static const value_string iscsi_scsi_responses[] = {
    { 0, "Command completed at target" },
    { 1, "Response does not contain SCSI status"},
    { 0, NULL }
};

static const value_string iscsi_scsicommand_taskattrs[] = {
    {0, "Untagged"},
    {1, "Simple"},
    {2, "Ordered"},
    {3, "Head of Queue"},
    {4, "ACA"},
    {0, NULL},
};

static const value_string iscsi_task_management_responses[] = {
    {0, "Function complete"},
    {1, "Task not in task set"},
    {2, "LUN does not exist"},
    {3, "Task still allegiant"},
    {4, "Task failover not supported"},
    {5, "Task management function not supported"},
    {6, "Authorisation failed"},
    {255, "Function rejected"},
    {0, NULL},
};

static const value_string iscsi_task_management_functions[] = {
    {1, "Abort Task"},
    {2, "Abort Task Set"},
    {3, "Clear ACA"},
    {4, "Clear Task Set"},
    {5, "Logical Unit Reset"},
    {6, "Target Warm Reset"},
    {7, "Target Cold Reset"},
    {0, NULL},
};

static const value_string iscsi_login_status[] = {
    {0x0000, "Success"},
    {0x0101, "Target moved temporarily"},
    {0x0102, "Target moved permanently"},
    {0x0200, "Initiator error (miscellaneous error)"},
    {0x0201, "Authentication failed"},
    {0x0202, "Authorisation failure"},
    {0x0203, "Target not found"},
    {0x0204, "Target removed"},
    {0x0205, "Unsupported version"},
    {0x0206, "Too many connections"},
    {0x0207, "Missing parameter"},
    {0x0208, "Can't include in session"},
    {0x0209, "Session type not supported"},
    {0x020a, "Session does not exist"},
    {0x020b, "Invalid request during login"},
    {0x0300, "Target error (miscellaneous error)"},
    {0x0301, "Service unavailable"},
    {0x0302, "Out of resources"},
    {0, NULL},
};

static const value_string iscsi_login_stage[] = {
    {0, "Security negotiation"},
    {1, "Operational negotiation"},
    {3, "Full feature phase"},
    {0, NULL},
};

/* #ifndef DRAFT08 */
static const value_string iscsi_isid_type[] = {
    {0x00, "IEEE OUI"},
    {0x01, "IANA Enterprise Number"},
    {0x02, "Random"},
    {0, NULL},
};
/* #endif */

static const value_string iscsi_logout_reasons[] = {
    {0, "Close session"},
    {1, "Close connection"},
    {2, "Remove connection for recovery"},
    {0, NULL},
};

static const value_string iscsi_logout_response[] = {
    {0, "Connection closed successfully"},
    {1, "CID not found"},
    {2, "Connection recovery not supported"},
    {3, "Cleanup failed for various reasons"},
    {0, NULL},
};

static const value_string iscsi_asyncevents[] = {
    {0, "A SCSI asynchronous event is reported in the sense data"},
    {1, "Target requests logout"},
    {2, "Target will/has dropped connection"},
    {3, "Target will/has dropped all connections"},
    {4, "Target requests parameter negotiation"},
    {0, NULL},
};

static const value_string iscsi_snack_types[] = {
    {0, "Data/R2T"},
    {1, "Status"},
/* #ifndef DRAFT08 */
    {2, "Data ACK"},
/* #endif */
    {3, "R-Data"},
    {0, NULL}
};

static const value_string iscsi_reject_reasons[] = {
/* #ifdef DRAFT08 */
    {0x01, "Full feature phase command before login"},
/* #endif */
    {0x02, "Data (payload) digest error"},
    {0x03, "Data SNACK reject"},
    {0x04, "Protocol error"},
    {0x05, "Command not supported in this session type"},
    {0x06, "Immediate command reject (too many immediate commands)"},
    {0x07, "Task in progress"},
    {0x08, "Invalid Data Ack"},
    {0x09, "Invalid PDU field"},
    {0x0a, "Long operation reject"},
    {0x0b, "Negotiation reset"},
    {0x0c, "Waiting for logout"},
    {0, NULL},
};

/*****************************************************************/
/*                                                               */
/* CRC LOOKUP TABLE                                              */
/* ================                                              */
/* The following CRC lookup table was generated automagically    */
/* by the Rocksoft^tm Model CRC Algorithm Table Generation       */
/* Program V1.0 using the following model parameters:            */
/*                                                               */
/*    Width   : 4 bytes.                                         */
/*    Poly    : 0x1EDC6F41L                                      */
/*    Reverse : TRUE.                                            */
/*                                                               */
/* For more information on the Rocksoft^tm Model CRC Algorithm,  */
/* see the document titled "A Painless Guide to CRC Error        */
/* Detection Algorithms" by Ross Williams                        */
/* (ross@guest.adelaide.edu.au.). This document is likely to be  */
/* in the FTP archive "ftp.adelaide.edu.au/pub/rocksoft".        */
/*                                                               */
/*****************************************************************/

static guint32 crc32Table[256] = {
    0x00000000L, 0xF26B8303L, 0xE13B70F7L, 0x1350F3F4L,
    0xC79A971FL, 0x35F1141CL, 0x26A1E7E8L, 0xD4CA64EBL,
    0x8AD958CFL, 0x78B2DBCCL, 0x6BE22838L, 0x9989AB3BL,
    0x4D43CFD0L, 0xBF284CD3L, 0xAC78BF27L, 0x5E133C24L,
    0x105EC76FL, 0xE235446CL, 0xF165B798L, 0x030E349BL,
    0xD7C45070L, 0x25AFD373L, 0x36FF2087L, 0xC494A384L,
    0x9A879FA0L, 0x68EC1CA3L, 0x7BBCEF57L, 0x89D76C54L,
    0x5D1D08BFL, 0xAF768BBCL, 0xBC267848L, 0x4E4DFB4BL,
    0x20BD8EDEL, 0xD2D60DDDL, 0xC186FE29L, 0x33ED7D2AL,
    0xE72719C1L, 0x154C9AC2L, 0x061C6936L, 0xF477EA35L,
    0xAA64D611L, 0x580F5512L, 0x4B5FA6E6L, 0xB93425E5L,
    0x6DFE410EL, 0x9F95C20DL, 0x8CC531F9L, 0x7EAEB2FAL,
    0x30E349B1L, 0xC288CAB2L, 0xD1D83946L, 0x23B3BA45L,
    0xF779DEAEL, 0x05125DADL, 0x1642AE59L, 0xE4292D5AL,
    0xBA3A117EL, 0x4851927DL, 0x5B016189L, 0xA96AE28AL,
    0x7DA08661L, 0x8FCB0562L, 0x9C9BF696L, 0x6EF07595L,
    0x417B1DBCL, 0xB3109EBFL, 0xA0406D4BL, 0x522BEE48L,
    0x86E18AA3L, 0x748A09A0L, 0x67DAFA54L, 0x95B17957L,
    0xCBA24573L, 0x39C9C670L, 0x2A993584L, 0xD8F2B687L,
    0x0C38D26CL, 0xFE53516FL, 0xED03A29BL, 0x1F682198L,
    0x5125DAD3L, 0xA34E59D0L, 0xB01EAA24L, 0x42752927L,
    0x96BF4DCCL, 0x64D4CECFL, 0x77843D3BL, 0x85EFBE38L,
    0xDBFC821CL, 0x2997011FL, 0x3AC7F2EBL, 0xC8AC71E8L,
    0x1C661503L, 0xEE0D9600L, 0xFD5D65F4L, 0x0F36E6F7L,
    0x61C69362L, 0x93AD1061L, 0x80FDE395L, 0x72966096L,
    0xA65C047DL, 0x5437877EL, 0x4767748AL, 0xB50CF789L,
    0xEB1FCBADL, 0x197448AEL, 0x0A24BB5AL, 0xF84F3859L,
    0x2C855CB2L, 0xDEEEDFB1L, 0xCDBE2C45L, 0x3FD5AF46L,
    0x7198540DL, 0x83F3D70EL, 0x90A324FAL, 0x62C8A7F9L,
    0xB602C312L, 0x44694011L, 0x5739B3E5L, 0xA55230E6L,
    0xFB410CC2L, 0x092A8FC1L, 0x1A7A7C35L, 0xE811FF36L,
    0x3CDB9BDDL, 0xCEB018DEL, 0xDDE0EB2AL, 0x2F8B6829L,
    0x82F63B78L, 0x709DB87BL, 0x63CD4B8FL, 0x91A6C88CL,
    0x456CAC67L, 0xB7072F64L, 0xA457DC90L, 0x563C5F93L,
    0x082F63B7L, 0xFA44E0B4L, 0xE9141340L, 0x1B7F9043L,
    0xCFB5F4A8L, 0x3DDE77ABL, 0x2E8E845FL, 0xDCE5075CL,
    0x92A8FC17L, 0x60C37F14L, 0x73938CE0L, 0x81F80FE3L,
    0x55326B08L, 0xA759E80BL, 0xB4091BFFL, 0x466298FCL,
    0x1871A4D8L, 0xEA1A27DBL, 0xF94AD42FL, 0x0B21572CL,
    0xDFEB33C7L, 0x2D80B0C4L, 0x3ED04330L, 0xCCBBC033L,
    0xA24BB5A6L, 0x502036A5L, 0x4370C551L, 0xB11B4652L,
    0x65D122B9L, 0x97BAA1BAL, 0x84EA524EL, 0x7681D14DL,
    0x2892ED69L, 0xDAF96E6AL, 0xC9A99D9EL, 0x3BC21E9DL,
    0xEF087A76L, 0x1D63F975L, 0x0E330A81L, 0xFC588982L,
    0xB21572C9L, 0x407EF1CAL, 0x532E023EL, 0xA145813DL,
    0x758FE5D6L, 0x87E466D5L, 0x94B49521L, 0x66DF1622L,
    0x38CC2A06L, 0xCAA7A905L, 0xD9F75AF1L, 0x2B9CD9F2L,
    0xFF56BD19L, 0x0D3D3E1AL, 0x1E6DCDEEL, 0xEC064EEDL,
    0xC38D26C4L, 0x31E6A5C7L, 0x22B65633L, 0xD0DDD530L,
    0x0417B1DBL, 0xF67C32D8L, 0xE52CC12CL, 0x1747422FL,
    0x49547E0BL, 0xBB3FFD08L, 0xA86F0EFCL, 0x5A048DFFL,
    0x8ECEE914L, 0x7CA56A17L, 0x6FF599E3L, 0x9D9E1AE0L,
    0xD3D3E1ABL, 0x21B862A8L, 0x32E8915CL, 0xC083125FL,
    0x144976B4L, 0xE622F5B7L, 0xF5720643L, 0x07198540L,
    0x590AB964L, 0xAB613A67L, 0xB831C993L, 0x4A5A4A90L,
    0x9E902E7BL, 0x6CFBAD78L, 0x7FAB5E8CL, 0x8DC0DD8FL,
    0xE330A81AL, 0x115B2B19L, 0x020BD8EDL, 0xF0605BEEL,
    0x24AA3F05L, 0xD6C1BC06L, 0xC5914FF2L, 0x37FACCF1L,
    0x69E9F0D5L, 0x9B8273D6L, 0x88D28022L, 0x7AB90321L,
    0xAE7367CAL, 0x5C18E4C9L, 0x4F48173DL, 0xBD23943EL,
    0xF36E6F75L, 0x0105EC76L, 0x12551F82L, 0xE03E9C81L,
    0x34F4F86AL, 0xC69F7B69L, 0xD5CF889DL, 0x27A40B9EL,
    0x79B737BAL, 0x8BDCB4B9L, 0x988C474DL, 0x6AE7C44EL,
    0xBE2DA0A5L, 0x4C4623A6L, 0x5F16D052L, 0xAD7D5351L
};

#define CRC32C_PRELOAD 0xffffffff

/* 
 * Byte swap fix contributed by Dave Wysochanski <davidw@netapp.com>
 */
#define CRC32C_SWAP(crc32c_value) \
		(((crc32c_value & 0xff000000) >> 24) | \
		((crc32c_value & 0x00ff0000) >>	 8) | \
		((crc32c_value & 0x0000ff00) <<	 8) | \
		((crc32c_value & 0x000000ff) << 24))

static guint32
calculateCRC32(const void *buf, int len, guint32 crc) {
    const guint8 *p = (const guint8 *)buf;
    crc = CRC32C_SWAP(crc);
    while(len-- > 0)
        crc = crc32Table[(crc ^ *p++) & 0xff] ^ (crc >> 8);
    return CRC32C_SWAP(crc);
}





/* structure and functions to keep track of 
 * COMMAND/DATA_IN/DATA_OUT/RESPONSE matching 
 */
typedef struct _iscsi_conv_data {
    guint32 conv_idx;
    guint32 itt;
    guint16 lun;
    guint32 request_frame;
    guint32 data_in_frame;
    guint32 data_out_frame;
    guint32 response_frame;
    guint32 iscsi_dl;
    nstime_t req_time;
} iscsi_conv_data_t;
static GHashTable *iscsi_req_unmatched = NULL;
static GHashTable *iscsi_req_matched = NULL;
static GMemChunk *iscsi_req_vals = NULL;
static guint32 iscsi_init_count = 200;

static gint
iscsi_equal_unmatched(gconstpointer v, gconstpointer w)
{
  const iscsi_conv_data_t *v1 = (const iscsi_conv_data_t *)v;
  const iscsi_conv_data_t *v2 = (const iscsi_conv_data_t *)w;

  return (v1->conv_idx == v2->conv_idx)&&(v1->itt == v2->itt);
}

static guint
iscsi_hash_unmatched (gconstpointer v)
{
	const iscsi_conv_data_t *key = (const iscsi_conv_data_t *)v;
	guint val;

	val = key->conv_idx + key->itt;

	return val;
}

static gint
iscsi_equal_matched(gconstpointer v, gconstpointer w)
{
  const iscsi_conv_data_t *v1 = (const iscsi_conv_data_t *)v;
  const iscsi_conv_data_t *v2 = (const iscsi_conv_data_t *)w;
  int check_frame;

  check_frame=0;
  if (v1->request_frame && (v1->request_frame==v2->request_frame))
      check_frame=1;
  if (v1->data_in_frame && (v1->data_in_frame==v2->data_in_frame))
      check_frame=1;
  if (v1->data_out_frame && (v1->data_out_frame==v2->data_out_frame))
      check_frame=1;
  if (v1->response_frame && (v1->response_frame==v2->response_frame))
      check_frame=1;

  return check_frame&&(v1->conv_idx == v2->conv_idx)&&(v1->itt == v2->itt);
}

static guint
iscsi_hash_matched (gconstpointer v)
{
	const iscsi_conv_data_t *key = (const iscsi_conv_data_t *)v;
	guint val;

	val = key->conv_idx + key->itt;

	return val;
}






/*
 * Protocol initialization
 */
static void
iscsi_init_protocol(void)
{
    if (iscsi_sessions) {
        g_mem_chunk_destroy(iscsi_sessions);
	iscsi_sessions=NULL;
    }
    if (iscsi_req_vals) {
        g_mem_chunk_destroy(iscsi_req_vals);
	iscsi_req_vals=NULL;
    }
    if (iscsi_req_unmatched) {
        g_hash_table_destroy(iscsi_req_unmatched);
	iscsi_req_unmatched=NULL;
    }
    if (iscsi_req_matched) {
        g_hash_table_destroy(iscsi_req_matched);
	iscsi_req_matched=NULL;
    }
    if (iscsi_session_table) {
        g_hash_table_destroy(iscsi_session_table);
	iscsi_session_table=NULL;
    }

    iscsi_req_unmatched = g_hash_table_new(iscsi_hash_unmatched, iscsi_equal_unmatched);
    iscsi_req_matched = g_hash_table_new(iscsi_hash_matched, iscsi_equal_matched);
    iscsi_session_table = g_hash_table_new(iscsi_session_hash, iscsi_session_equal);
    iscsi_req_vals = g_mem_chunk_new("iscsi_req_vals",
                                   sizeof(iscsi_conv_data_t),
                                   iscsi_init_count * sizeof(iscsi_conv_data_t),
                                   G_ALLOC_AND_FREE);
    iscsi_sessions = g_mem_chunk_new("iscsi_sessions",
                                   sizeof(iscsi_session_t),
                                   iscsi_init_count * sizeof(iscsi_session_t),
                                   G_ALLOC_AND_FREE);
}

static int
iscsi_min(int a, int b) {
    return (a < b)? a : b;
}

static gint
addTextKeys(proto_tree *tt, tvbuff_t *tvb, gint offset, guint32 text_len) {
    const gint limit = offset + text_len;
    while(offset < limit) {
	gint len = tvb_strnlen(tvb, offset, limit - offset);
	if(len == -1)
	    len = limit - offset;
	else
	    len = len + 1;
	proto_tree_add_item(tt, hf_iscsi_KeyValue, tvb, offset, len, FALSE);
	offset += len;
    }
    return offset;
}

static gint
handleHeaderDigest(iscsi_session_t *iscsi_session, proto_item *ti, tvbuff_t *tvb, guint offset, int headerLen) {
    int available_bytes = tvb_length_remaining(tvb, offset);

    switch(iscsi_session->header_digest){
    case ISCSI_HEADER_DIGEST_CRC32:
	if(available_bytes >= (headerLen + 4)) {
	    guint32 crc = ~calculateCRC32(tvb_get_ptr(tvb, offset, headerLen), headerLen, CRC32C_PRELOAD);
	    guint32 sent = tvb_get_ntohl(tvb, offset + headerLen);
	    if(crc == sent) {
		proto_tree_add_uint_format(ti, hf_iscsi_HeaderDigest32, tvb, offset + headerLen, 4, sent, "HeaderDigest: 0x%08x (Good CRC32)", sent);
	    } else {
		proto_tree_add_uint_format(ti, hf_iscsi_HeaderDigest32, tvb, offset + headerLen, 4, sent, "HeaderDigest: 0x%08x (Bad CRC32, should be 0x%08x)", sent, crc);
	    }
	}
	return offset + headerLen + 4;
        break;
    }
    return offset + headerLen;
}

static gint
handleDataDigest(proto_item *ti, tvbuff_t *tvb, guint offset, int dataLen) {
    int available_bytes = tvb_length_remaining(tvb, offset);
    if(enableDataDigests) {
	if(dataDigestIsCRC32) {
	    if(available_bytes >= (dataLen + 4)) {
		guint32 crc = ~calculateCRC32(tvb_get_ptr(tvb, offset, dataLen), dataLen, CRC32C_PRELOAD);
		guint32 sent = tvb_get_ntohl(tvb, offset + dataLen);
		if(crc == sent) {
		    proto_tree_add_uint_format(ti, hf_iscsi_DataDigest32, tvb, offset + dataLen, 4, sent, "DataDigest: 0x%08x (Good CRC32)", sent);
		}
		else {
		    proto_tree_add_uint_format(ti, hf_iscsi_DataDigest32, tvb, offset + dataLen, 4, sent, "DataDigest: 0x%08x (Bad CRC32, should be 0x%08x)", sent, crc);
		}
	    }
	    return offset + dataLen + 4;
	}
	if(available_bytes >= (dataLen + dataDigestSize)) {
	    proto_tree_add_item(ti, hf_iscsi_DataDigest, tvb, offset + dataLen, dataDigestSize, FALSE);
	}
	return offset + dataLen + dataDigestSize;
    }
    return offset + dataLen;
}

static int
handleDataSegment(proto_item *ti, tvbuff_t *tvb, guint offset, guint dataSegmentLen, guint endOffset, int hf_id) {
    if(endOffset > offset) {
	int dataOffset = offset;
	int dataLen = iscsi_min(dataSegmentLen, endOffset - offset);
	if(dataLen > 0) {
	    proto_tree_add_item(ti, hf_id, tvb, offset, dataLen, FALSE);
	    offset += dataLen;
	}
	if(offset < endOffset && (offset & 3) != 0) {
	    int padding = 4 - (offset & 3);
	    proto_tree_add_item(ti, hf_iscsi_Padding, tvb, offset, padding, FALSE);
	    offset += padding;
	}
	if(dataSegmentLen > 0 && offset < endOffset)
	    offset = handleDataDigest(ti, tvb, dataOffset, offset - dataOffset);
    }

    return offset;
}

static int
handleDataSegmentAsTextKeys(proto_item *ti, tvbuff_t *tvb, guint offset, guint dataSegmentLen, guint endOffset, int digestsActive) {
    if(endOffset > offset) {
	int dataOffset = offset;
	int textLen = iscsi_min(dataSegmentLen, endOffset - offset);
	if(textLen > 0) {
	    proto_item *tf = proto_tree_add_text(ti, tvb, offset, textLen, "Key/Value Pairs");
	    proto_tree *tt = proto_item_add_subtree(tf, ett_iscsi_KeyValues);
	    offset = addTextKeys(tt, tvb, offset, textLen);
	}
	if(offset < endOffset && (offset & 3) != 0) {
	    int padding = 4 - (offset & 3);
	    proto_tree_add_item(ti, hf_iscsi_Padding, tvb, offset, padding, FALSE);
	    offset += padding;
	}
	if(digestsActive && dataSegmentLen > 0 && offset < endOffset)
	    offset = handleDataDigest(ti, tvb, dataOffset, offset - dataOffset);
    }
    return offset;
}

/* Code to actually dissect the packets */
static void
dissect_iscsi_pdu(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, guint offset, guint8 opcode, const char *opcode_str, guint32 data_segment_len, iscsi_session_t *iscsi_session) {

    guint original_offset = offset;
    proto_tree *ti = NULL;
    guint8 scsi_status = 0;
    gboolean S_bit=FALSE;
    guint cdb_offset = offset + 32; /* offset of CDB from start of PDU */
    guint end_offset = offset + tvb_length_remaining(tvb, offset);
    iscsi_conv_data_t *cdata = NULL;
    scsi_task_id_t task_key;
    int paddedDataSegmentLength = data_segment_len;
    guint16 lun=0xffff;

    if(paddedDataSegmentLength & 3)
	paddedDataSegmentLength += 4 - (paddedDataSegmentLength & 3);

    /* Make entries in Protocol column and Info column on summary display */
    if (check_col(pinfo->cinfo, COL_PROTOCOL))
	col_set_str(pinfo->cinfo, COL_PROTOCOL, "iSCSI");

    if (opcode == ISCSI_OPCODE_SCSI_RESPONSE ||
	opcode == ISCSI_OPCODE_SCSI_DATA_IN) {
        scsi_status = tvb_get_guint8 (tvb, offset+3);
    }

    if ((opcode == ISCSI_OPCODE_SCSI_RESPONSE) ||
        (opcode == ISCSI_OPCODE_SCSI_DATA_IN) ||
        (opcode == ISCSI_OPCODE_SCSI_DATA_OUT)) {
        if (!pinfo->fd->flags.visited){
            iscsi_conv_data_t ckey;
            ckey.conv_idx = iscsi_session->conv_idx;
            ckey.itt = tvb_get_ntohl (tvb, offset+16);

            /* first time we see this packet. check if we can find the request */
            cdata = (iscsi_conv_data_t *)g_hash_table_lookup (iscsi_req_unmatched, &ckey);
            if (cdata){
                if (cdata->data_in_frame+cdata->data_out_frame+cdata->response_frame==0){
                    /* this is the first response to the request, add it to the matched table */
                    g_hash_table_insert (iscsi_req_matched, cdata, cdata);
                }
                switch(opcode){
                case ISCSI_OPCODE_SCSI_RESPONSE:
                    cdata->response_frame=pinfo->fd->num;
                    break;
                case ISCSI_OPCODE_SCSI_DATA_IN:
                    /* a bit ugly but we need to check the S bit here */
                    if(tvb_get_guint8(tvb, offset+1)&ISCSI_SCSI_DATA_FLAG_S){
                        cdata->response_frame=pinfo->fd->num;
                    }
                    cdata->data_in_frame=pinfo->fd->num;
                    break;
                case ISCSI_OPCODE_SCSI_DATA_OUT:
                    cdata->data_out_frame=pinfo->fd->num;
                    break;
                }
            }
        } else {
            iscsi_conv_data_t ckey;
            ckey.conv_idx = iscsi_session->conv_idx;
            ckey.itt = tvb_get_ntohl (tvb, offset+16);
            ckey.request_frame=0;
            ckey.data_in_frame=0;
            ckey.data_out_frame=0;
            ckey.response_frame=0;
            switch(opcode){
            case ISCSI_OPCODE_SCSI_RESPONSE:
                ckey.response_frame=pinfo->fd->num;
                break;
            case ISCSI_OPCODE_SCSI_DATA_IN:
                ckey.data_in_frame=pinfo->fd->num;
                break;
            case ISCSI_OPCODE_SCSI_DATA_OUT:
                ckey.data_out_frame=pinfo->fd->num;
                break;
            }

            /* we have seen this one before,   pick it up from the matched table */
            cdata = (iscsi_conv_data_t *)g_hash_table_lookup (iscsi_req_matched, &ckey);
        }

        if (cdata){
            task_key.conv_id = cdata->conv_idx;
            task_key.task_id = cdata->itt;
            pinfo->private_data = &task_key;
        } else {
            pinfo->private_data = NULL;
        }

    } else if (opcode == ISCSI_OPCODE_SCSI_COMMAND) {
        /*we need the LUN value for some of the commands so we can pass it
          across to the SCSI dissector.
          Not correct but simple  and probably accurate enough :
          If bit 6 of first bit is 0   then just take second byte as the LUN
          If bit 6 of first bit is 1, then take 6 bits from first byte
          and all of second byte and pretend it is the lun value
	  people that care can add host specific dissection of vsa later.

          We need to keep track of this on a per transaction basis since
          for error recoverylevel 0 and when the A bit is clear in a 
          Data-In PDU, there will not be a LUN field in teh iscsi layer.
        */
        if(tvb_get_guint8(tvb, offset+8)&0x40){
          /* volume set addressing */
          lun=tvb_get_guint8(tvb,offset+8)&0x3f;
          lun<<=8;
          lun|=tvb_get_guint8(tvb,offset+9);
        } else {
          lun=tvb_get_guint8(tvb,offset+9);
        }
        if (!pinfo->fd->flags.visited){
            iscsi_conv_data_t ckey;

            /* first time we see this packet. */
            /*check if we have seen this request before and delete it in that case */
            ckey.conv_idx = iscsi_session->conv_idx;
            ckey.itt = tvb_get_ntohl (tvb, offset+16);
            cdata = (iscsi_conv_data_t *)g_hash_table_lookup (iscsi_req_unmatched, &ckey);
            if (cdata){
                g_hash_table_remove(iscsi_req_unmatched, &ckey);
            }

            /* add this new transaction to the unmatched table */
            cdata = g_mem_chunk_alloc (iscsi_req_vals);
            cdata->conv_idx = iscsi_session->conv_idx;
            cdata->itt = tvb_get_ntohl (tvb, offset+16);
            cdata->lun=lun;
            cdata->request_frame=pinfo->fd->num;
            cdata->data_in_frame=0;
            cdata->data_out_frame=0;
            cdata->response_frame=0;
            cdata->req_time.nsecs = pinfo->fd->abs_usecs*1000;
            cdata->req_time.secs = pinfo->fd->abs_secs;

            g_hash_table_insert (iscsi_req_unmatched, cdata, cdata);
        } else {
                iscsi_conv_data_t ckey;
                ckey.conv_idx = iscsi_session->conv_idx;
                ckey.itt = tvb_get_ntohl (tvb, offset+16);
                ckey.request_frame=pinfo->fd->num;
                ckey.data_in_frame=0;
                ckey.data_out_frame=0;
                ckey.response_frame=0;

                /* we have seen this one before,   pick it up from the matched table */
                cdata = (iscsi_conv_data_t *)g_hash_table_lookup (iscsi_req_matched, &ckey);
        }


        if (cdata){
            /* The SCSI protocol uses this as the key to detect a
             * SCSI-level conversation. */
            task_key.conv_id = cdata->conv_idx;
            task_key.task_id = cdata->itt;
            pinfo->private_data = &task_key;
        } else {
            pinfo->private_data=NULL;
        }
    }
    else {
        pinfo->private_data = NULL;
    }

    if (check_col(pinfo->cinfo, COL_INFO)) {

        if (opcode != ISCSI_OPCODE_SCSI_COMMAND) {

            col_append_str(pinfo->cinfo, COL_INFO, opcode_str);

	    if (opcode == ISCSI_OPCODE_SCSI_RESPONSE ||
		(opcode == ISCSI_OPCODE_SCSI_DATA_IN &&
		 (tvb_get_guint8(tvb, offset + 1) & ISCSI_SCSI_DATA_FLAG_S))) {
		col_append_fstr (pinfo->cinfo, COL_INFO, " (%s)",
				 val_to_str (scsi_status, scsi_status_val, "0x%x"));
	    }
	    else if (opcode == ISCSI_OPCODE_LOGIN_RESPONSE) {
		guint16 login_status = tvb_get_ntohs(tvb, offset+36);
		col_append_fstr (pinfo->cinfo, COL_INFO, " (%s)",
				 val_to_str (login_status, iscsi_login_status, "0x%x"));
	    }
	    else if (opcode == ISCSI_OPCODE_LOGOUT_COMMAND) {
		guint8 logoutReason;
		if(iscsi_protocol_version == ISCSI_PROTOCOL_DRAFT08) {
		    logoutReason = tvb_get_guint8(tvb, offset+11);
		} else if(iscsi_protocol_version >= ISCSI_PROTOCOL_DRAFT13) {
		    logoutReason = tvb_get_guint8(tvb, offset+1) & 0x7f;
		}
		else {
		    logoutReason = tvb_get_guint8(tvb, offset+23);
		}
		col_append_fstr (pinfo->cinfo, COL_INFO, " (%s)",
				 val_to_str (logoutReason, iscsi_logout_reasons, "0x%x"));
	    }
	    else if (opcode == ISCSI_OPCODE_TASK_MANAGEMENT_FUNCTION) {
		guint8 tmf = tvb_get_guint8(tvb, offset + 1);
		col_append_fstr (pinfo->cinfo, COL_INFO, " (%s)",
				 val_to_str (tmf, iscsi_task_management_functions, "0x%x"));
	    }
	    else if (opcode == ISCSI_OPCODE_TASK_MANAGEMENT_FUNCTION_RESPONSE) {
		guint8 resp = tvb_get_guint8(tvb, offset + 2);
		col_append_fstr (pinfo->cinfo, COL_INFO, " (%s)",
				 val_to_str (resp, iscsi_task_management_responses, "0x%x"));
	    }
	    else if (opcode == ISCSI_OPCODE_REJECT) {
		guint8 reason = tvb_get_guint8(tvb, offset + 2);
		col_append_fstr (pinfo->cinfo, COL_INFO, " (%s)",
				 val_to_str (reason, iscsi_reject_reasons, "0x%x"));
	    }
	    else if (opcode == ISCSI_OPCODE_ASYNC_MESSAGE) {
		guint8 asyncEvent = tvb_get_guint8(tvb, offset + 36);
		col_append_fstr (pinfo->cinfo, COL_INFO, " (%s)",
				 val_to_str (asyncEvent, iscsi_asyncevents, "0x%x"));
	    }
	}
    }

    /* In the interest of speed, if "tree" is NULL, don't do any
       work not necessary to generate protocol tree items. */
    if (tree) {
	proto_item *tp;
	/* create display subtree for the protocol */
	tp = proto_tree_add_protocol_format(tree, proto_iscsi, tvb,
					    offset, -1, "iSCSI (%s)",
					    opcode_str);
	ti = proto_item_add_subtree(tp, ett_iscsi);
    }
    proto_tree_add_uint(ti, hf_iscsi_Opcode, tvb,
			    offset + 0, 1, opcode);
    if((opcode & TARGET_OPCODE_BIT) == 0) {
	    /* initiator -> target */
	    gint b = tvb_get_guint8(tvb, offset + 0);
	if(iscsi_protocol_version == ISCSI_PROTOCOL_DRAFT08) {
		if(opcode != ISCSI_OPCODE_SCSI_DATA_OUT &&
		   opcode != ISCSI_OPCODE_LOGOUT_COMMAND &&
		   opcode != ISCSI_OPCODE_SNACK_REQUEST)
		    proto_tree_add_boolean(ti, hf_iscsi_X, tvb, offset + 0, 1, b);
	}
	    if(opcode != ISCSI_OPCODE_SCSI_DATA_OUT &&
	       opcode != ISCSI_OPCODE_LOGIN_COMMAND &&
	       opcode != ISCSI_OPCODE_SNACK_REQUEST)
		proto_tree_add_boolean(ti, hf_iscsi_I, tvb, offset + 0, 1, b);
    }

    if(opcode == ISCSI_OPCODE_NOP_OUT) {
	    /* NOP Out */
	    if(iscsi_protocol_version > ISCSI_PROTOCOL_DRAFT09) {
		proto_tree_add_item(ti, hf_iscsi_TotalAHSLength, tvb, offset + 4, 1, FALSE);
	    }
	    proto_tree_add_uint(ti, hf_iscsi_DataSegmentLength, tvb, offset + 5, 3, data_segment_len);
	    proto_tree_add_item(ti, hf_iscsi_LUN, tvb, offset + 8, 8, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_InitiatorTaskTag, tvb, offset + 16, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_TargetTransferTag, tvb, offset + 20, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_CmdSN, tvb, offset + 24, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_ExpStatSN, tvb, offset + 28, 4, FALSE);
	    offset = handleHeaderDigest(iscsi_session, ti, tvb, offset, 48);
	    offset = handleDataSegment(ti, tvb, offset, data_segment_len, end_offset, hf_iscsi_ping_data);
    } else if(opcode == ISCSI_OPCODE_NOP_IN) {
	    /* NOP In */
	    if(iscsi_protocol_version > ISCSI_PROTOCOL_DRAFT09) {
		proto_tree_add_item(ti, hf_iscsi_TotalAHSLength, tvb, offset + 4, 1, FALSE);
	    }
	    proto_tree_add_uint(ti, hf_iscsi_DataSegmentLength, tvb, offset + 5, 3, data_segment_len);
	    proto_tree_add_item(ti, hf_iscsi_LUN, tvb, offset + 8, 8, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_InitiatorTaskTag, tvb, offset + 16, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_TargetTransferTag, tvb, offset + 20, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_StatSN, tvb, offset + 24, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_ExpCmdSN, tvb, offset + 28, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_MaxCmdSN, tvb, offset + 32, 4, FALSE);
	    offset = handleHeaderDigest(iscsi_session, ti, tvb, offset, 48);
	    offset = handleDataSegment(ti, tvb, offset, data_segment_len, end_offset, hf_iscsi_ping_data);
    } else if(opcode == ISCSI_OPCODE_SCSI_COMMAND) {
	    /* SCSI Command */
	    guint32 ahsLen = tvb_get_guint8(tvb, offset + 4) * 4;
    	    {
		gint b = tvb_get_guint8(tvb, offset + 1);
		proto_item *tf = proto_tree_add_uint(ti, hf_iscsi_Flags, tvb, offset + 1, 1, b);
		proto_tree *tt = proto_item_add_subtree(tf, ett_iscsi_Flags);

		proto_tree_add_boolean(tt, hf_iscsi_SCSICommand_F, tvb, offset + 1, 1, b);
		proto_tree_add_boolean(tt, hf_iscsi_SCSICommand_R, tvb, offset + 1, 1, b);
		proto_tree_add_boolean(tt, hf_iscsi_SCSICommand_W, tvb, offset + 1, 1, b);
		proto_tree_add_uint(tt, hf_iscsi_SCSICommand_Attr, tvb, offset + 1, 1, b);
	    }
	    if(iscsi_protocol_version < ISCSI_PROTOCOL_DRAFT12) {
		proto_tree_add_item(ti, hf_iscsi_SCSICommand_CRN, tvb, offset + 3, 1, FALSE);
	    }
	    proto_tree_add_item(ti, hf_iscsi_TotalAHSLength, tvb, offset + 4, 1, FALSE);
	    proto_tree_add_uint(ti, hf_iscsi_DataSegmentLength, tvb, offset + 5, 3, data_segment_len);
	    proto_tree_add_item(ti, hf_iscsi_LUN, tvb, offset + 8, 8, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_InitiatorTaskTag, tvb, offset + 16, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_ExpectedDataTransferLength, tvb, offset + 20, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_CmdSN, tvb, offset + 24, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_ExpStatSN, tvb, offset + 28, 4, FALSE);
	    {
		if(ahsLen > 0) {
		    /* FIXME - disssect AHS? */
		    proto_tree_add_item(ti, hf_iscsi_AHS, tvb, offset + 48, ahsLen, FALSE);
		}
		offset = handleHeaderDigest(iscsi_session, ti, tvb, offset, 48 + ahsLen);
	    }
	    offset = handleDataSegment(ti, tvb, offset, data_segment_len, end_offset, hf_iscsi_immediate_data);
    } else if(opcode == ISCSI_OPCODE_SCSI_RESPONSE) {
	    /* SCSI Response */
	    {
		gint b = tvb_get_guint8(tvb, offset + 1);
		proto_item *tf = proto_tree_add_uint(ti, hf_iscsi_Flags, tvb, offset + 1, 1, b);
		proto_tree *tt = proto_item_add_subtree(tf, ett_iscsi_Flags);

		proto_tree_add_boolean(tt, hf_iscsi_SCSIResponse_o, tvb, offset + 1, 1, b);
		proto_tree_add_boolean(tt, hf_iscsi_SCSIResponse_u, tvb, offset + 1, 1, b);
		proto_tree_add_boolean(tt, hf_iscsi_SCSIResponse_O, tvb, offset + 1, 1, b);
		proto_tree_add_boolean(tt, hf_iscsi_SCSIResponse_U, tvb, offset + 1, 1, b);
	    }
	    proto_tree_add_item(ti, hf_iscsi_SCSIResponse_Response, tvb, offset + 2, 1, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_SCSIResponse_Status, tvb, offset + 3, 1, FALSE);
	    if(iscsi_protocol_version > ISCSI_PROTOCOL_DRAFT09) {
		proto_tree_add_item(ti, hf_iscsi_TotalAHSLength, tvb, offset + 4, 1, FALSE);
	    }
	    proto_tree_add_uint(ti, hf_iscsi_DataSegmentLength, tvb, offset + 5, 3, data_segment_len);
	    proto_tree_add_item(ti, hf_iscsi_InitiatorTaskTag, tvb, offset + 16, 4, FALSE);
	    if(iscsi_protocol_version <= ISCSI_PROTOCOL_DRAFT09) {
		proto_tree_add_item(ti, hf_iscsi_SCSIResponse_ResidualCount, tvb, offset + 20, 4, FALSE);
	    }
	    proto_tree_add_item(ti, hf_iscsi_StatSN, tvb, offset + 24, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_ExpCmdSN, tvb, offset + 28, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_MaxCmdSN, tvb, offset + 32, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_ExpDataSN, tvb, offset + 36, 4, FALSE);
	    if(iscsi_protocol_version <= ISCSI_PROTOCOL_DRAFT09) {
		proto_tree_add_item(ti, hf_iscsi_SCSIResponse_BidiReadResidualCount, tvb, offset + 44, 4, FALSE);
	    }
	    else {
		proto_tree_add_item(ti, hf_iscsi_SCSIResponse_BidiReadResidualCount, tvb, offset + 40, 4, FALSE);
		proto_tree_add_item(ti, hf_iscsi_SCSIResponse_ResidualCount, tvb, offset + 44, 4, FALSE);
	    }
	    offset = handleHeaderDigest(iscsi_session, ti, tvb, offset, 48);
	    /* do not update offset here because the data segment is
	     * dissected below */
	    handleDataDigest(ti, tvb, offset, paddedDataSegmentLength);
    } else if(opcode == ISCSI_OPCODE_TASK_MANAGEMENT_FUNCTION) {
	    /* Task Management Function */
 	    proto_tree_add_item(ti, hf_iscsi_TaskManagementFunction_Function, tvb, offset + 1, 1, FALSE);
	    if(iscsi_protocol_version > ISCSI_PROTOCOL_DRAFT09) {
		proto_tree_add_item(ti, hf_iscsi_TotalAHSLength, tvb, offset + 4, 1, FALSE);
		proto_tree_add_uint(ti, hf_iscsi_DataSegmentLength, tvb, offset + 5, 3, data_segment_len);
	    }
	    proto_tree_add_item(ti, hf_iscsi_LUN, tvb, offset + 8, 8, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_InitiatorTaskTag, tvb, offset + 16, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_TaskManagementFunction_ReferencedTaskTag, tvb, offset + 20, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_CmdSN, tvb, offset + 24, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_ExpStatSN, tvb, offset + 28, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_RefCmdSN, tvb, offset + 32, 4, FALSE);
	    offset = handleHeaderDigest(iscsi_session, ti, tvb, offset, 48);
    } else if(opcode == ISCSI_OPCODE_TASK_MANAGEMENT_FUNCTION_RESPONSE) {
	    /* Task Management Function Response */
	    proto_tree_add_item(ti, hf_iscsi_TaskManagementFunction_Response, tvb, offset + 2, 1, FALSE);
	    if(iscsi_protocol_version <= ISCSI_PROTOCOL_DRAFT09) {
		proto_tree_add_item(ti, hf_iscsi_TotalAHSLength, tvb, offset + 4, 1, FALSE);
		proto_tree_add_uint(ti, hf_iscsi_DataSegmentLength, tvb, offset + 5, 3, data_segment_len);
	    }
	    proto_tree_add_item(ti, hf_iscsi_InitiatorTaskTag, tvb, offset + 16, 4, FALSE);
	    if(iscsi_protocol_version < ISCSI_PROTOCOL_DRAFT12) {
		proto_tree_add_item(ti, hf_iscsi_TaskManagementFunction_ReferencedTaskTag, tvb, offset + 20, 4, FALSE);
	    }
	    proto_tree_add_item(ti, hf_iscsi_StatSN, tvb, offset + 24, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_ExpCmdSN, tvb, offset + 28, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_MaxCmdSN, tvb, offset + 32, 4, FALSE);
	    offset = handleHeaderDigest(iscsi_session, ti, tvb, offset, 48);
    } else if(opcode == ISCSI_OPCODE_LOGIN_COMMAND) {
	    /* Login Command */
	    int digestsActive = 0;
	    {
		gint b = tvb_get_guint8(tvb, offset + 1);
		if(iscsi_protocol_version == ISCSI_PROTOCOL_DRAFT08) {
		    if((b & CSG_MASK) >= ISCSI_CSG_OPERATIONAL_NEGOTIATION)
			digestsActive = 1;
		}
#if 0
		proto_item *tf = proto_tree_add_uint(ti, hf_iscsi_Flags, tvb, offset + 1, 1, b);
		proto_tree *tt = proto_item_add_subtree(tf, ett_iscsi_Flags);
#endif

		proto_tree_add_boolean(ti, hf_iscsi_Login_T, tvb, offset + 1, 1, b);
		if(iscsi_protocol_version >= ISCSI_PROTOCOL_DRAFT13) {
		    proto_tree_add_boolean(ti, hf_iscsi_Login_C, tvb, offset + 1, 1, b);
		}
		if(iscsi_protocol_version == ISCSI_PROTOCOL_DRAFT08) {
		    proto_tree_add_boolean(ti, hf_iscsi_Login_X, tvb, offset + 1, 1, b);
		}
		proto_tree_add_item(ti, hf_iscsi_Login_CSG, tvb, offset + 1, 1, FALSE);

		/* NSG is undefined unless T is set */
		if(b&0x80){
			proto_tree_add_item(ti, hf_iscsi_Login_NSG, tvb, offset + 1, 1, FALSE);
		}
	    }
	    proto_tree_add_item(ti, hf_iscsi_VersionMax, tvb, offset + 2, 1, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_VersionMin, tvb, offset + 3, 1, FALSE);
	    if(iscsi_protocol_version > ISCSI_PROTOCOL_DRAFT09) {
		proto_tree_add_item(ti, hf_iscsi_TotalAHSLength, tvb, offset + 4, 1, FALSE);
	    }
	    proto_tree_add_uint(ti, hf_iscsi_DataSegmentLength, tvb, offset + 5, 3, data_segment_len);
	    if(iscsi_protocol_version == ISCSI_PROTOCOL_DRAFT08) {
		proto_tree_add_item(ti, hf_iscsi_CID, tvb, offset + 8, 2, FALSE);
		proto_tree_add_item(ti, hf_iscsi_ISID8, tvb, offset + 12, 2, FALSE);
	    }
	    else {
		proto_item *tf = proto_tree_add_item(ti, hf_iscsi_ISID, tvb, offset + 8, 6, FALSE);
		proto_tree *tt = proto_item_add_subtree(tf, ett_iscsi_ISID);
		if(iscsi_protocol_version == ISCSI_PROTOCOL_DRAFT09) {
		    proto_tree_add_item(tt, hf_iscsi_ISID_Type, tvb, offset + 8, 1, FALSE);
		    proto_tree_add_item(tt, hf_iscsi_ISID_NamingAuthority, tvb, offset + 9, 3, FALSE);
		    proto_tree_add_item(tt, hf_iscsi_ISID_Qualifier, tvb, offset + 12, 2, FALSE);
		}
		else {
		    proto_tree_add_item(tt, hf_iscsi_ISID_t, tvb, offset + 8, 1, FALSE);
		    proto_tree_add_item(tt, hf_iscsi_ISID_a, tvb, offset + 8, 1, FALSE);
		    proto_tree_add_item(tt, hf_iscsi_ISID_b, tvb, offset + 9, 2, FALSE);
		    proto_tree_add_item(tt, hf_iscsi_ISID_c, tvb, offset + 11, 1, FALSE);
		    proto_tree_add_item(tt, hf_iscsi_ISID_d, tvb, offset + 12, 2, FALSE);
		}
	    }
	    if(iscsi_protocol_version < ISCSI_PROTOCOL_DRAFT12) {
		proto_tree_add_item(ti, hf_iscsi_TSID, tvb, offset + 14, 2, FALSE);
	    }
	    else {
		proto_tree_add_item(ti, hf_iscsi_TSIH, tvb, offset + 14, 2, FALSE);
	    }
	    proto_tree_add_item(ti, hf_iscsi_InitiatorTaskTag, tvb, offset + 16, 4, FALSE);
	    if(iscsi_protocol_version > ISCSI_PROTOCOL_DRAFT08) {
		proto_tree_add_item(ti, hf_iscsi_CID, tvb, offset + 20, 2, FALSE);
	    }
	    proto_tree_add_item(ti, hf_iscsi_CmdSN, tvb, offset + 24, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_ExpStatSN, tvb, offset + 28, 4, FALSE);
	    if(digestsActive){
		offset = handleHeaderDigest(iscsi_session, ti, tvb, offset, 48);
	    } else {
		offset += 48;
	    }
	    offset = handleDataSegmentAsTextKeys(ti, tvb, offset, data_segment_len, end_offset, digestsActive);
    } else if(opcode == ISCSI_OPCODE_LOGIN_RESPONSE) {
	    /* Login Response */
	    int digestsActive = 0;
	    {
		gint b = tvb_get_guint8(tvb, offset + 1);
		if(iscsi_protocol_version == ISCSI_PROTOCOL_DRAFT08) {
		    if((b & CSG_MASK) >= ISCSI_CSG_OPERATIONAL_NEGOTIATION)
			digestsActive = 1;
		}
#if 0
		proto_item *tf = proto_tree_add_uint(ti, hf_iscsi_Flags, tvb, offset + 1, 1, b);
		proto_tree *tt = proto_item_add_subtree(tf, ett_iscsi_Flags);
#endif

		proto_tree_add_boolean(ti, hf_iscsi_Login_T, tvb, offset + 1, 1, b);
		if(iscsi_protocol_version >= ISCSI_PROTOCOL_DRAFT13) {
		    proto_tree_add_boolean(ti, hf_iscsi_Login_C, tvb, offset + 1, 1, b);
		}
		proto_tree_add_item(ti, hf_iscsi_Login_CSG, tvb, offset + 1, 1, FALSE);
		/* NSG is undefined unless T is set */
		if(b&0x80){
			proto_tree_add_item(ti, hf_iscsi_Login_NSG, tvb, offset + 1, 1, FALSE);
		}
	    }

	    proto_tree_add_item(ti, hf_iscsi_VersionMax, tvb, offset + 2, 1, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_VersionActive, tvb, offset + 3, 1, FALSE);
	    if(iscsi_protocol_version > ISCSI_PROTOCOL_DRAFT09) {
		proto_tree_add_item(ti, hf_iscsi_TotalAHSLength, tvb, offset + 4, 1, FALSE);
	    }
	    proto_tree_add_uint(ti, hf_iscsi_DataSegmentLength, tvb, offset + 5, 3, data_segment_len);
	    if(iscsi_protocol_version == ISCSI_PROTOCOL_DRAFT08) {
		proto_tree_add_item(ti, hf_iscsi_ISID8, tvb, offset + 12, 2, FALSE);
	    }
	    else {
		proto_item *tf = proto_tree_add_item(ti, hf_iscsi_ISID, tvb, offset + 8, 6, FALSE);
		proto_tree *tt = proto_item_add_subtree(tf, ett_iscsi_ISID);
		if(iscsi_protocol_version == ISCSI_PROTOCOL_DRAFT09) {
		    proto_tree_add_item(tt, hf_iscsi_ISID_Type, tvb, offset + 8, 1, FALSE);
		    proto_tree_add_item(tt, hf_iscsi_ISID_NamingAuthority, tvb, offset + 9, 3, FALSE);
		    proto_tree_add_item(tt, hf_iscsi_ISID_Qualifier, tvb, offset + 12, 2, FALSE);
		}
		else {
		    proto_tree_add_item(tt, hf_iscsi_ISID_t, tvb, offset + 8, 1, FALSE);
		    proto_tree_add_item(tt, hf_iscsi_ISID_a, tvb, offset + 8, 1, FALSE);
		    proto_tree_add_item(tt, hf_iscsi_ISID_b, tvb, offset + 9, 2, FALSE);
		    proto_tree_add_item(tt, hf_iscsi_ISID_c, tvb, offset + 11, 1, FALSE);
		    proto_tree_add_item(tt, hf_iscsi_ISID_d, tvb, offset + 12, 2, FALSE);
		}
	    }
	    if(iscsi_protocol_version < ISCSI_PROTOCOL_DRAFT12) {
		proto_tree_add_item(ti, hf_iscsi_TSID, tvb, offset + 14, 2, FALSE);
	    }
	    else {
		proto_tree_add_item(ti, hf_iscsi_TSIH, tvb, offset + 14, 2, FALSE);
	    }
	    proto_tree_add_item(ti, hf_iscsi_InitiatorTaskTag, tvb, offset + 16, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_StatSN, tvb, offset + 24, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_ExpCmdSN, tvb, offset + 28, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_MaxCmdSN, tvb, offset + 32, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_Login_Status, tvb, offset + 36, 2, FALSE);
	    if(digestsActive){
		offset = handleHeaderDigest(iscsi_session, ti, tvb, offset, 48);
	    } else {
		offset += 48;
	    }
	    offset = handleDataSegmentAsTextKeys(ti, tvb, offset, data_segment_len, end_offset, digestsActive);
    } else if(opcode == ISCSI_OPCODE_TEXT_COMMAND) {
	    /* Text Command */
	    {
		gint b = tvb_get_guint8(tvb, offset + 1);
		proto_item *tf = proto_tree_add_uint(ti, hf_iscsi_Flags, tvb, offset + 1, 1, b);
		proto_tree *tt = proto_item_add_subtree(tf, ett_iscsi_Flags);

		proto_tree_add_boolean(tt, hf_iscsi_Text_F, tvb, offset + 1, 1, b);
		if(iscsi_protocol_version >= ISCSI_PROTOCOL_DRAFT13) {
		    proto_tree_add_boolean(tt, hf_iscsi_Text_C, tvb, offset + 1, 1, b);
		}
	    }
	    if(iscsi_protocol_version > ISCSI_PROTOCOL_DRAFT09) {
		proto_tree_add_item(ti, hf_iscsi_TotalAHSLength, tvb, offset + 4, 1, FALSE);
	    }
	    proto_tree_add_uint(ti, hf_iscsi_DataSegmentLength, tvb, offset + 5, 3, data_segment_len);
	    if(iscsi_protocol_version > ISCSI_PROTOCOL_DRAFT09) {
		proto_tree_add_item(ti, hf_iscsi_LUN, tvb, offset + 8, 8, FALSE);
	    }
	    proto_tree_add_item(ti, hf_iscsi_InitiatorTaskTag, tvb, offset + 16, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_TargetTransferTag, tvb, offset + 20, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_CmdSN, tvb, offset + 24, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_ExpStatSN, tvb, offset + 28, 4, FALSE);
	    offset = handleHeaderDigest(iscsi_session, ti, tvb, offset, 48);
	    offset = handleDataSegmentAsTextKeys(ti, tvb, offset, data_segment_len, end_offset, TRUE);
    } else if(opcode == ISCSI_OPCODE_TEXT_RESPONSE) {
	    /* Text Response */
	    {
		gint b = tvb_get_guint8(tvb, offset + 1);
		proto_item *tf = proto_tree_add_uint(ti, hf_iscsi_Flags, tvb, offset + 1, 1, b);
		proto_tree *tt = proto_item_add_subtree(tf, ett_iscsi_Flags);

		proto_tree_add_boolean(tt, hf_iscsi_Text_F, tvb, offset + 1, 1, b);
		if(iscsi_protocol_version >= ISCSI_PROTOCOL_DRAFT13) {
		    proto_tree_add_boolean(tt, hf_iscsi_Text_C, tvb, offset + 1, 1, b);
		}
	    }
	    if(iscsi_protocol_version > ISCSI_PROTOCOL_DRAFT09) {
		proto_tree_add_item(ti, hf_iscsi_TotalAHSLength, tvb, offset + 4, 1, FALSE);
	    }
	    proto_tree_add_uint(ti, hf_iscsi_DataSegmentLength, tvb, offset + 5, 3, data_segment_len);
	    if(iscsi_protocol_version > ISCSI_PROTOCOL_DRAFT09) {
		proto_tree_add_item(ti, hf_iscsi_LUN, tvb, offset + 8, 8, FALSE);
	    }
	    proto_tree_add_item(ti, hf_iscsi_InitiatorTaskTag, tvb, offset + 16, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_TargetTransferTag, tvb, offset + 20, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_StatSN, tvb, offset + 24, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_ExpCmdSN, tvb, offset + 28, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_MaxCmdSN, tvb, offset + 32, 4, FALSE);
	    offset = handleHeaderDigest(iscsi_session, ti, tvb, offset, 48);
	    offset = handleDataSegmentAsTextKeys(ti, tvb, offset, data_segment_len, end_offset, TRUE);
    } else if(opcode == ISCSI_OPCODE_SCSI_DATA_OUT) {
	    /* SCSI Data Out (write) */
	    {
		gint b = tvb_get_guint8(tvb, offset + 1);
		proto_item *tf = proto_tree_add_uint(ti, hf_iscsi_Flags, tvb, offset + 1, 1, b);
		proto_tree *tt = proto_item_add_subtree(tf, ett_iscsi_Flags);

		proto_tree_add_boolean(tt, hf_iscsi_SCSIData_F, tvb, offset + 1, 1, b);
	    }
	    if(iscsi_protocol_version > ISCSI_PROTOCOL_DRAFT09) {
		proto_tree_add_item(ti, hf_iscsi_TotalAHSLength, tvb, offset + 4, 1, FALSE);
	    }
	    proto_tree_add_uint(ti, hf_iscsi_DataSegmentLength, tvb, offset + 5, 3, data_segment_len);
	    proto_tree_add_item(ti, hf_iscsi_LUN, tvb, offset + 8, 8, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_InitiatorTaskTag, tvb, offset + 16, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_TargetTransferTag, tvb, offset + 20, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_ExpStatSN, tvb, offset + 28, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_DataSN, tvb, offset + 36, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_BufferOffset, tvb, offset + 40, 4, FALSE);
	    offset = handleHeaderDigest(iscsi_session, ti, tvb, offset, 48);
	    /* do not update offset here because the data segment is
	     * dissected below */
	    handleDataDigest(ti, tvb, offset, paddedDataSegmentLength);
    } else if(opcode == ISCSI_OPCODE_SCSI_DATA_IN) {
	    /* SCSI Data In (read) */
	    {
		gint b = tvb_get_guint8(tvb, offset + 1);
		proto_item *tf = proto_tree_add_uint(ti, hf_iscsi_Flags, tvb, offset + 1, 1, b);
		proto_tree *tt = proto_item_add_subtree(tf, ett_iscsi_Flags);

                if(b&ISCSI_SCSI_DATA_FLAG_S){
                   S_bit=TRUE;
                }
		proto_tree_add_boolean(tt, hf_iscsi_SCSIData_F, tvb, offset + 1, 1, b);
		if(iscsi_protocol_version > ISCSI_PROTOCOL_DRAFT08) {
		    proto_tree_add_boolean(tt, hf_iscsi_SCSIData_A, tvb, offset + 1, 1, b);
		}
		proto_tree_add_boolean(tt, hf_iscsi_SCSIData_O, tvb, offset + 1, 1, b);
		proto_tree_add_boolean(tt, hf_iscsi_SCSIData_U, tvb, offset + 1, 1, b);
		proto_tree_add_boolean(tt, hf_iscsi_SCSIData_S, tvb, offset + 1, 1, b);
	    }
	    if(S_bit){
		proto_tree_add_item(ti, hf_iscsi_SCSIResponse_Status, tvb, offset + 3, 1, FALSE);
	    }
	    if(iscsi_protocol_version > ISCSI_PROTOCOL_DRAFT09) {
		proto_tree_add_item(ti, hf_iscsi_TotalAHSLength, tvb, offset + 4, 1, FALSE);
	    }
	    proto_tree_add_uint(ti, hf_iscsi_DataSegmentLength, tvb, offset + 5, 3, data_segment_len);
	    if(iscsi_protocol_version > ISCSI_PROTOCOL_DRAFT09) {
		proto_tree_add_item(ti, hf_iscsi_LUN, tvb, offset + 8, 8, FALSE);
	    }
	    proto_tree_add_item(ti, hf_iscsi_InitiatorTaskTag, tvb, offset + 16, 4, FALSE);
	    if(iscsi_protocol_version <= ISCSI_PROTOCOL_DRAFT09) {
		proto_tree_add_item(ti, hf_iscsi_SCSIData_ResidualCount, tvb, offset + 20, 4, FALSE);
	    }
	    else {
		proto_tree_add_item(ti, hf_iscsi_TargetTransferTag, tvb, offset + 20, 4, FALSE);
	    }
	    proto_tree_add_item(ti, hf_iscsi_StatSN, tvb, offset + 24, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_ExpCmdSN, tvb, offset + 28, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_MaxCmdSN, tvb, offset + 32, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_DataSN, tvb, offset + 36, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_BufferOffset, tvb, offset + 40, 4, FALSE);
	    if(iscsi_protocol_version > ISCSI_PROTOCOL_DRAFT09) {
		proto_tree_add_item(ti, hf_iscsi_SCSIData_ResidualCount, tvb, offset + 44, 4, FALSE);
	    }
	    offset = handleHeaderDigest(iscsi_session, ti, tvb, offset, 48);
	    /* do not update offset here because the data segment is
	     * dissected below */
	    handleDataDigest(ti, tvb, offset, paddedDataSegmentLength);
    } else if(opcode == ISCSI_OPCODE_LOGOUT_COMMAND) {
	    /* Logout Command */
	    if(iscsi_protocol_version >= ISCSI_PROTOCOL_DRAFT13) {
		proto_tree_add_item(ti, hf_iscsi_Logout_Reason, tvb, offset + 1, 1, FALSE);
	    }
	    if(iscsi_protocol_version > ISCSI_PROTOCOL_DRAFT09) {
		proto_tree_add_item(ti, hf_iscsi_TotalAHSLength, tvb, offset + 4, 1, FALSE);
		proto_tree_add_uint(ti, hf_iscsi_DataSegmentLength, tvb, offset + 5, 3, data_segment_len);
	    }
	    if(iscsi_protocol_version == ISCSI_PROTOCOL_DRAFT08) {
		proto_tree_add_item(ti, hf_iscsi_CID, tvb, offset + 8, 2, FALSE);
		proto_tree_add_item(ti, hf_iscsi_Logout_Reason, tvb, offset + 11, 1, FALSE);
	    }
	    proto_tree_add_item(ti, hf_iscsi_InitiatorTaskTag, tvb, offset + 16, 4, FALSE);
	    if(iscsi_protocol_version > ISCSI_PROTOCOL_DRAFT08) {
		proto_tree_add_item(ti, hf_iscsi_CID, tvb, offset + 20, 2, FALSE);
		if(iscsi_protocol_version < ISCSI_PROTOCOL_DRAFT13) {
		    proto_tree_add_item(ti, hf_iscsi_Logout_Reason, tvb, offset + 23, 1, FALSE);
		}
	    }
	    proto_tree_add_item(ti, hf_iscsi_CmdSN, tvb, offset + 24, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_ExpStatSN, tvb, offset + 28, 4, FALSE);
	    offset = handleHeaderDigest(iscsi_session, ti, tvb, offset, 48);
    } else if(opcode == ISCSI_OPCODE_LOGOUT_RESPONSE) {
	    /* Logout Response */
	    proto_tree_add_item(ti, hf_iscsi_Logout_Response, tvb, offset + 2, 1, FALSE);
	    if(iscsi_protocol_version > ISCSI_PROTOCOL_DRAFT09) {
		proto_tree_add_item(ti, hf_iscsi_TotalAHSLength, tvb, offset + 4, 1, FALSE);
		proto_tree_add_uint(ti, hf_iscsi_DataSegmentLength, tvb, offset + 5, 3, data_segment_len);
	    }
	    proto_tree_add_item(ti, hf_iscsi_InitiatorTaskTag, tvb, offset + 16, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_StatSN, tvb, offset + 24, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_ExpCmdSN, tvb, offset + 28, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_MaxCmdSN, tvb, offset + 32, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_Time2Wait, tvb, offset + 40, 2, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_Time2Retain, tvb, offset + 42, 2, FALSE);
	    offset = handleHeaderDigest(iscsi_session, ti, tvb, offset, 48);
    } else if(opcode == ISCSI_OPCODE_SNACK_REQUEST) {
	    /* SNACK Request */
	    {
		gint b = tvb_get_guint8(tvb, offset + 1);
#if 0
		proto_item *tf = proto_tree_add_uint(ti, hf_iscsi_Flags, tvb, offset + 1, 1, b);
		proto_tree *tt = proto_item_add_subtree(tf, ett_iscsi_Flags);
#endif

		proto_tree_add_item(ti, hf_iscsi_snack_type, tvb, offset + 1, 1, b);
	    }
	    if(iscsi_protocol_version > ISCSI_PROTOCOL_DRAFT09) {
		proto_tree_add_item(ti, hf_iscsi_TotalAHSLength, tvb, offset + 4, 1, FALSE);
		proto_tree_add_uint(ti, hf_iscsi_DataSegmentLength, tvb, offset + 5, 3, data_segment_len);
		proto_tree_add_item(ti, hf_iscsi_LUN, tvb, offset + 8, 8, FALSE);
	    }
	    proto_tree_add_item(ti, hf_iscsi_InitiatorTaskTag, tvb, offset + 16, 4, FALSE);
	    if(iscsi_protocol_version <= ISCSI_PROTOCOL_DRAFT09) {
		proto_tree_add_item(ti, hf_iscsi_BegRun, tvb, offset + 20, 4, FALSE);
		proto_tree_add_item(ti, hf_iscsi_RunLength, tvb, offset + 24, 4, FALSE);
		proto_tree_add_item(ti, hf_iscsi_ExpStatSN, tvb, offset + 28, 4, FALSE);
		proto_tree_add_item(ti, hf_iscsi_ExpDataSN, tvb, offset + 36, 4, FALSE);
	    }
	    else {
		proto_tree_add_item(ti, hf_iscsi_TargetTransferTag, tvb, offset + 20, 4, FALSE);
		proto_tree_add_item(ti, hf_iscsi_ExpStatSN, tvb, offset + 28, 4, FALSE);
		proto_tree_add_item(ti, hf_iscsi_BegRun, tvb, offset + 40, 4, FALSE);
		proto_tree_add_item(ti, hf_iscsi_RunLength, tvb, offset + 44, 4, FALSE);
	    }
	    offset = handleHeaderDigest(iscsi_session, ti, tvb, offset, 48);
    } else if(opcode == ISCSI_OPCODE_R2T) {
	    /* R2T */
	    if(iscsi_protocol_version > ISCSI_PROTOCOL_DRAFT09) {
		proto_tree_add_item(ti, hf_iscsi_TotalAHSLength, tvb, offset + 4, 1, FALSE);
		proto_tree_add_uint(ti, hf_iscsi_DataSegmentLength, tvb, offset + 5, 3, data_segment_len);
		proto_tree_add_item(ti, hf_iscsi_LUN, tvb, offset + 8, 8, FALSE);
	    }
	    proto_tree_add_item(ti, hf_iscsi_InitiatorTaskTag, tvb, offset + 16, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_TargetTransferTag, tvb, offset + 20, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_StatSN, tvb, offset + 24, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_ExpCmdSN, tvb, offset + 28, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_MaxCmdSN, tvb, offset + 32, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_R2TSN, tvb, offset + 36, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_BufferOffset, tvb, offset + 40, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_DesiredDataLength, tvb, offset + 44, 4, FALSE);
	    offset = handleHeaderDigest(iscsi_session, ti, tvb, offset, 48);
    } else if(opcode == ISCSI_OPCODE_ASYNC_MESSAGE) {
	    /* Asynchronous Message */
	    if(iscsi_protocol_version > ISCSI_PROTOCOL_DRAFT09) {
		proto_tree_add_item(ti, hf_iscsi_TotalAHSLength, tvb, offset + 4, 1, FALSE);
	    }
	    proto_tree_add_uint(ti, hf_iscsi_DataSegmentLength, tvb, offset + 5, 3, data_segment_len);
	    proto_tree_add_item(ti, hf_iscsi_LUN, tvb, offset + 8, 8, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_StatSN, tvb, offset + 24, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_ExpCmdSN, tvb, offset + 28, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_MaxCmdSN, tvb, offset + 32, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_AsyncEvent, tvb, offset + 36, 1, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_EventVendorCode, tvb, offset + 37, 1, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_Parameter1, tvb, offset + 38, 2, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_Parameter2, tvb, offset + 40, 2, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_Parameter3, tvb, offset + 42, 2, FALSE);
	    offset = handleHeaderDigest(iscsi_session, ti, tvb, offset, 48);
	    offset = handleDataSegment(ti, tvb, offset, data_segment_len, end_offset, hf_iscsi_async_message_data);
    } else if(opcode == ISCSI_OPCODE_REJECT) {
	    /* Reject */
	    proto_tree_add_item(ti, hf_iscsi_Reject_Reason, tvb, offset + 2, 1, FALSE);
	    if(iscsi_protocol_version > ISCSI_PROTOCOL_DRAFT09) {
		proto_tree_add_item(ti, hf_iscsi_TotalAHSLength, tvb, offset + 4, 1, FALSE);
	    }
	    proto_tree_add_uint(ti, hf_iscsi_DataSegmentLength, tvb, offset + 5, 3, data_segment_len);
	    proto_tree_add_item(ti, hf_iscsi_StatSN, tvb, offset + 24, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_ExpCmdSN, tvb, offset + 28, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_MaxCmdSN, tvb, offset + 32, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_DataSN, tvb, offset + 36, 4, FALSE);
	    offset = handleHeaderDigest(iscsi_session, ti, tvb, offset, 48);
	    offset = handleDataSegment(ti, tvb, offset, data_segment_len, end_offset, hf_iscsi_error_pdu_data);
    } else if(opcode == ISCSI_OPCODE_VENDOR_SPECIFIC_I0 ||
		opcode == ISCSI_OPCODE_VENDOR_SPECIFIC_I1 ||
		opcode == ISCSI_OPCODE_VENDOR_SPECIFIC_I2 ||
		opcode == ISCSI_OPCODE_VENDOR_SPECIFIC_T0 ||
		opcode == ISCSI_OPCODE_VENDOR_SPECIFIC_T1 ||
		opcode == ISCSI_OPCODE_VENDOR_SPECIFIC_T2) {
	    /* Vendor specific opcodes */
	    if(iscsi_protocol_version > ISCSI_PROTOCOL_DRAFT09) {
		proto_tree_add_item(ti, hf_iscsi_TotalAHSLength, tvb, offset + 4, 1, FALSE);
	    }
	    proto_tree_add_uint(ti, hf_iscsi_DataSegmentLength, tvb, offset + 5, 3, data_segment_len);
	    offset = handleHeaderDigest(iscsi_session, ti, tvb, offset, 48);
	    offset = handleDataSegment(ti, tvb, offset, data_segment_len, end_offset, hf_iscsi_vendor_specific_data);
    }



    /* handle request/response matching */
    if (cdata){
        switch(opcode){
        case ISCSI_OPCODE_SCSI_RESPONSE:
            if (cdata->request_frame){
                nstime_t delta_time;
                proto_tree_add_uint(ti, hf_iscsi_request_frame, tvb, 0, 0, cdata->request_frame);
                delta_time.secs = pinfo->fd->abs_secs - cdata->req_time.secs;
                delta_time.nsecs = pinfo->fd->abs_usecs*1000 - cdata->req_time.nsecs;
                if (delta_time.nsecs<0){
                    delta_time.nsecs+=1000000000;
                    delta_time.secs--;
                }
                proto_tree_add_time(ti, hf_iscsi_time, tvb, 0, 0, &delta_time);
                
            }
            if (cdata->data_in_frame)
                proto_tree_add_uint(ti, hf_iscsi_data_in_frame, tvb, 0, 0, cdata->data_in_frame);
            if (cdata->data_out_frame)
                proto_tree_add_uint(ti, hf_iscsi_data_out_frame, tvb, 0, 0, cdata->data_out_frame);
            break;
        case ISCSI_OPCODE_SCSI_DATA_IN:
            /* if we have phase collaps then we might have the
               response embedded in the last DataIn segment */
            if(!S_bit){
                if (cdata->request_frame)
                    proto_tree_add_uint(ti, hf_iscsi_request_frame, tvb, 0, 0, cdata->request_frame);
                if (cdata->response_frame)
                    proto_tree_add_uint(ti, hf_iscsi_response_frame, tvb, 0, 0, cdata->response_frame);
            } else {
                if (cdata->request_frame){
                     nstime_t delta_time;
                     proto_tree_add_uint(ti, hf_iscsi_request_frame, tvb, 0, 0, cdata->request_frame);
                     delta_time.secs = pinfo->fd->abs_secs - cdata->req_time.secs;
                     delta_time.nsecs = pinfo->fd->abs_usecs*1000 - cdata->req_time.nsecs;
                     if (delta_time.nsecs<0){
                          delta_time.nsecs+=1000000000;
                          delta_time.secs--;
                     }
                     proto_tree_add_time(ti, hf_iscsi_time, tvb, 0, 0, &delta_time);
                
                }
            }
            if (cdata->data_out_frame)
                proto_tree_add_uint(ti, hf_iscsi_data_out_frame, tvb, 0, 0, cdata->data_out_frame);
            break;
        case ISCSI_OPCODE_SCSI_DATA_OUT:
            if (cdata->request_frame)
                proto_tree_add_uint(ti, hf_iscsi_request_frame, tvb, 0, 0, cdata->request_frame);
            if (cdata->data_in_frame)
                proto_tree_add_uint(ti, hf_iscsi_data_in_frame, tvb, 0, 0, cdata->data_in_frame);
            if (cdata->response_frame)
                proto_tree_add_uint(ti, hf_iscsi_response_frame, tvb, 0, 0, cdata->response_frame);
            break;
        case ISCSI_OPCODE_SCSI_COMMAND:
            if (cdata->data_in_frame)
                proto_tree_add_uint(ti, hf_iscsi_data_in_frame, tvb, 0, 0, cdata->data_in_frame);
            if (cdata->data_out_frame)
                proto_tree_add_uint(ti, hf_iscsi_data_out_frame, tvb, 0, 0, cdata->data_out_frame);
            if (cdata->response_frame)
                proto_tree_add_uint(ti, hf_iscsi_response_frame, tvb, 0, 0, cdata->response_frame);
            break;
        }
    }



    proto_item_set_len(ti, offset - original_offset);

    if((opcode & ((iscsi_protocol_version == ISCSI_PROTOCOL_DRAFT08)?
		  ~(X_BIT | I_BIT) :
		  ~I_BIT)) == ISCSI_OPCODE_SCSI_COMMAND) {
        /* SCSI Command */
        dissect_scsi_cdb (tvb, pinfo, tree, cdb_offset, 16, SCSI_DEV_UNKNOWN, 
		lun);
    }
    else if (opcode == ISCSI_OPCODE_SCSI_RESPONSE) {
        if (scsi_status == 0x2) {
            /* A SCSI response with Check Condition contains sense data */
            /* offset is setup correctly by the iscsi code for response above */
	    if((end_offset - offset) >= 2) {
		int senseLen = tvb_get_ntohs(tvb, offset);
		if(ti != NULL)
		    proto_tree_add_item(ti, hf_iscsi_SenseLength, tvb, offset, 2, FALSE);
		offset += 2;
		if(senseLen > 0)
		    dissect_scsi_snsinfo (tvb, pinfo, tree, offset,
					  iscsi_min (senseLen,
						     end_offset-offset),
					  
					   (guint16) (cdata?cdata->lun:0xffff) );
	    }
        }
        else {
            dissect_scsi_rsp (tvb, pinfo, tree, 
		(guint16) (cdata?cdata->lun:0xffff), scsi_status);
        }
    }
    else if ((opcode == ISCSI_OPCODE_SCSI_DATA_IN) ||
             (opcode == ISCSI_OPCODE_SCSI_DATA_OUT)) {
        /* offset is setup correctly by the iscsi code for response above */
        dissect_scsi_payload (tvb, pinfo, tree, offset, 
			      (opcode==ISCSI_OPCODE_SCSI_DATA_OUT),
                              iscsi_min (data_segment_len, end_offset-offset),
			      (guint16) (cdata?cdata->lun:0xffff) );
    }
}

static gboolean
dissect_iscsi(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, gboolean check_port) {
    /* Set up structures needed to add the protocol subtree and manage it */
    guint iSCSIPdusDissected = 0;
    guint offset = 0;
    guint32 available_bytes = tvb_length_remaining(tvb, offset);
    guint32 pduLen = 48;
    int digestsActive = 1;
    conversation_t *conversation = NULL;
    iscsi_session_t *iscsi_session=NULL;

    /* quick check to see if the packet is long enough to contain the
     * minimum amount of information we need */
    if (available_bytes < 48 && (!iscsi_desegment || available_bytes < 8)) {
	/* no, so give up */
	return FALSE;
    }

    /* process multiple iSCSI PDUs per packet */
    while(available_bytes >= 48 || (iscsi_desegment && available_bytes >= 8)) {
	const char *opcode_str = NULL;
	guint32 data_segment_len;
	guint8 opcode = tvb_get_guint8(tvb, offset + 0);
	guint8 secondPduByte = tvb_get_guint8(tvb, offset + 1);
	int badPdu = FALSE;

	/* mask out any extra bits in the opcode byte */
	opcode &= OPCODE_MASK;

	opcode_str = match_strval(opcode, iscsi_opcodes);
	if(opcode == ISCSI_OPCODE_TASK_MANAGEMENT_FUNCTION ||
	   opcode == ISCSI_OPCODE_TASK_MANAGEMENT_FUNCTION_RESPONSE ||
	   opcode == ISCSI_OPCODE_R2T ||
	   opcode == ISCSI_OPCODE_LOGOUT_COMMAND ||
	   opcode == ISCSI_OPCODE_LOGOUT_RESPONSE ||
	   opcode == ISCSI_OPCODE_SNACK_REQUEST)
	    data_segment_len = 0;
	else
	    data_segment_len = tvb_get_ntohl(tvb, offset + 4) & 0x00ffffff;

	if(opcode_str == NULL) {
	    badPdu = TRUE;
	}
	else if(check_port && iscsi_port != 0 &&
		(((opcode & TARGET_OPCODE_BIT) && pinfo->srcport != iscsi_port) ||
		 (!(opcode & TARGET_OPCODE_BIT) && pinfo->destport != iscsi_port))) {
	    badPdu = TRUE;
	}
	else if(enable_bogosity_filter) {
	    /* try and distinguish between data and real headers */
	    if(data_segment_len > bogus_pdu_data_length_threshold) {
		badPdu = TRUE;
	    }
	    else if(demand_good_f_bit &&
		    !(secondPduByte & 0x80) &&
		    (opcode == ISCSI_OPCODE_NOP_OUT ||
		     opcode == ISCSI_OPCODE_NOP_IN ||
		     opcode == ISCSI_OPCODE_LOGOUT_COMMAND ||
		     opcode == ISCSI_OPCODE_LOGOUT_RESPONSE ||
		     opcode == ISCSI_OPCODE_SCSI_RESPONSE ||
		     opcode == ISCSI_OPCODE_TASK_MANAGEMENT_FUNCTION_RESPONSE ||
		     opcode == ISCSI_OPCODE_R2T ||
		     opcode == ISCSI_OPCODE_ASYNC_MESSAGE ||
		     opcode == ISCSI_OPCODE_SNACK_REQUEST ||
		     opcode == ISCSI_OPCODE_REJECT)) {
		badPdu = TRUE;
	    } else if(opcode==ISCSI_OPCODE_NOP_OUT) {
		/* TransferTag for NOP-Out should either be -1 or
		   the tag value we want for a response. 
		   Assume 0 means we are just inside a big all zero
		   datablock.
		*/
		if(tvb_get_ntohl(tvb, offset+20)==0){
		    badPdu = TRUE;
		}
	    }
	}

	if(badPdu) {
	    return iSCSIPdusDissected > 0;
	}

	if(opcode == ISCSI_OPCODE_LOGIN_COMMAND ||
	    opcode == ISCSI_OPCODE_LOGIN_RESPONSE) {
	    if(iscsi_protocol_version == ISCSI_PROTOCOL_DRAFT08) {
		if((secondPduByte & CSG_MASK) < ISCSI_CSG_OPERATIONAL_NEGOTIATION) {
		    /* digests are not yet turned on */
		    digestsActive = 0;
		}
	    } else {
		digestsActive = 0;
	    }
	}

	if(opcode == ISCSI_OPCODE_SCSI_COMMAND) {
	    /* ahsLen */
	    pduLen += tvb_get_guint8(tvb, offset + 4) * 4;
	}

	pduLen += data_segment_len;
	if((pduLen & 3) != 0)
	    pduLen += 4 - (pduLen & 3);


	if(digestsActive && data_segment_len > 0 && enableDataDigests) {
	    if(dataDigestIsCRC32)
		pduLen += 4;
	    else
		pduLen += dataDigestSize;
	}

	/* make sure we have a conversation for this session */
        conversation = find_conversation (pinfo->fd->num, &pinfo->src, &pinfo->dst,
                                          pinfo->ptype, pinfo->srcport,
                                          pinfo->destport, 0);
        if (!conversation) {
            conversation = conversation_new (pinfo->fd->num, &pinfo->src, &pinfo->dst,
                                             pinfo->ptype, pinfo->srcport,
                                             pinfo->destport, 0);
            iscsi_session=g_mem_chunk_alloc(iscsi_sessions);
            iscsi_session->conv_idx=conversation->index;
            iscsi_session->header_digest=ISCSI_HEADER_DIGEST_AUTO;
            g_hash_table_insert(iscsi_session_table, iscsi_session, iscsi_session);
        }
        if(!iscsi_session){
            iscsi_session_t key;
            key.conv_idx=conversation->index;
            /* this should never return NULL */
            iscsi_session = (iscsi_session_t *)g_hash_table_lookup (iscsi_session_table, &key);
            if(!iscsi_session){
                iscsi_session=g_mem_chunk_alloc(iscsi_sessions);
                iscsi_session->conv_idx=conversation->index;
                iscsi_session->header_digest=ISCSI_HEADER_DIGEST_AUTO;
                g_hash_table_insert(iscsi_session_table, iscsi_session, iscsi_session);
		/* DataOut PDUs are often mistaken by DCERPC heuristics to be
		   that protocol. Now that we know this is iscsi, set a 
		   dissector for conversation to block other heuristic
		   dissectors. 
		*/
		conversation_set_dissector(conversation, iscsi_handle);
            }
        }
        /* try to autodetect if header digest is used or not */
	if(digestsActive && (available_bytes>=52) && (iscsi_session->header_digest==ISCSI_HEADER_DIGEST_AUTO) ){
            guint32 crc;
		/* we have enough data to test if HeaderDigest is enabled */
            crc= ~calculateCRC32(tvb_get_ptr(tvb, offset, 48), 48, CRC32C_PRELOAD);
            if(crc==tvb_get_ntohl(tvb,48)){
                iscsi_session->header_digest=ISCSI_HEADER_DIGEST_CRC32;
            } else {
                iscsi_session->header_digest=ISCSI_HEADER_DIGEST_NONE;
            }
	}


	/* Add header digest length to pdulen */
	if(digestsActive){
		switch(iscsi_session->header_digest){
		case ISCSI_HEADER_DIGEST_CRC32:
			pduLen += 4;
			break;
		case ISCSI_HEADER_DIGEST_NONE:
			break;
		case ISCSI_HEADER_DIGEST_AUTO:
			/* oops we didnt know what digest is used yet */
			/* here we should use some default */
			break;
		default:
			g_assert_not_reached();
		}
	}

	/*
	 * Desegmentation check.
	 */
	if(iscsi_desegment && pinfo->can_desegment) {
	    if(pduLen > available_bytes) {
		/*
		 * This frame doesn't have all of the data for
		 * this message, but we can do reassembly on it.
		 *
		 * Tell the TCP dissector where the data for this
		 * message starts in the data it handed us, and
		 * how many more bytes we need, and return.
		 */
		pinfo->desegment_offset = offset;
		pinfo->desegment_len = pduLen - available_bytes;
		return TRUE;
	    }
	}

	/* This is to help TCP keep track of PDU boundaries
	   and allows it to find PDUs that are not aligned to 
	   the start of a TCP segments.
	   Since it also allows TCP to know what is in the middle
	   of a large PDU, it reduces the probability of a segment
	   in the middle of a large PDU transfer being misdissected as
	   a PDU.
	*/
	if(!pinfo->fd->flags.visited){
	    if(pduLen>(guint32)tvb_reported_length_remaining(tvb, offset)){
		pinfo->want_pdu_tracking=2;
		pinfo->bytes_until_next_pdu=pduLen-tvb_reported_length_remaining(tvb, offset);
	    }
	}

	if(check_col(pinfo->cinfo, COL_INFO)) {
	    if(iSCSIPdusDissected == 0)
		col_set_str(pinfo->cinfo, COL_INFO, "");
	    else
		col_append_str(pinfo->cinfo, COL_INFO, ", ");
	}

	dissect_iscsi_pdu(tvb, pinfo, tree, offset, opcode, opcode_str, data_segment_len, iscsi_session);
	if(pduLen > available_bytes)
	    pduLen = available_bytes;
	offset += pduLen;
	available_bytes -= pduLen;
	++iSCSIPdusDissected;
    }

    return iSCSIPdusDissected > 0;
}

/* This is called for those sessions where we have explicitely said
   this to be iSCSI using "Decode As..."
   In this case we will not check the port number for sanity and just
   do as the user said.
*/
static void
dissect_iscsi_handle(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree) {
    dissect_iscsi(tvb, pinfo, tree, FALSE);
}

/* This is called through the heuristic handler.
   In this case we also want to check that the port matches the preference
   setting for iSCSI in order to reduce the number of
   false positives.
*/
static gboolean
dissect_iscsi_heur(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree) {
    return dissect_iscsi(tvb, pinfo, tree, TRUE);
}


/* Register the protocol with Ethereal */

/*
 * this format is require because a script is used to build the C
 * function that calls all the protocol registration.
*/

void
proto_register_iscsi(void)
{

    /* Setup list of header fields  See Section 1.6.1 for details*/
    static hf_register_info hf[] = {
	{ &hf_iscsi_request_frame,
	  { "Request in", "iscsi.request_frame",
	    FT_FRAMENUM, BASE_NONE, NULL, 0,
	    "The request to this transaction is in this frame", HFILL }},

	{ &hf_iscsi_time,
	  { "Time from request", "iscsi.time",
	    FT_RELATIVE_TIME, BASE_NONE, NULL, 0,
	    "Time between the Command and the Response", HFILL }},

	{ &hf_iscsi_data_in_frame,
	  { "Data In in", "iscsi.data_in_frame",
	    FT_FRAMENUM, BASE_NONE, NULL, 0,
	    "The Data In for this transaction is in this frame", HFILL }},

	{ &hf_iscsi_data_out_frame,
	  { "Data Out in", "iscsi.data_out_frame",
	    FT_FRAMENUM, BASE_NONE, NULL, 0,
	    "The Data Out for this transaction is in this frame", HFILL }},

	{ &hf_iscsi_response_frame,
	  { "Response in", "iscsi.response_frame",
	    FT_FRAMENUM, BASE_NONE, NULL, 0,
	    "The response to this transaction is in this frame", HFILL }},

	{ &hf_iscsi_AHS,
	  { "AHS", "iscsi.ahs",
	    FT_BYTES, BASE_HEX, NULL, 0,
	    "Additional header segment", HFILL }
	},
	{ &hf_iscsi_Padding,
	  { "Padding", "iscsi.padding",
	    FT_BYTES, BASE_HEX, NULL, 0,
	    "Padding to 4 byte boundary", HFILL }
	},
	{ &hf_iscsi_ping_data,
	  { "PingData", "iscsi.pingdata",
	    FT_BYTES, BASE_HEX, NULL, 0,
	    "Ping Data", HFILL }
	},
	{ &hf_iscsi_immediate_data,
	  { "ImmediateData", "iscsi.immediatedata",
	    FT_BYTES, BASE_HEX, NULL, 0,
	    "Immediate Data", HFILL }
	},
	{ &hf_iscsi_write_data,
	  { "WriteData", "iscsi.writedata",
	    FT_BYTES, BASE_HEX, NULL, 0,
	    "Write Data", HFILL }
	},
	{ &hf_iscsi_read_data,
	  { "ReadData", "iscsi.readdata",
	    FT_BYTES, BASE_HEX, NULL, 0,
	    "Read Data", HFILL }
	},
	{ &hf_iscsi_error_pdu_data,
	  { "ErrorPDUData", "iscsi.errorpdudata",
	    FT_BYTES, BASE_HEX, NULL, 0,
	    "Error PDU Data", HFILL }
	},
	{ &hf_iscsi_async_message_data,
	  { "AsyncMessageData", "iscsi.asyncmessagedata",
	    FT_BYTES, BASE_HEX, NULL, 0,
	    "Async Message Data", HFILL }
	},
	{ &hf_iscsi_vendor_specific_data,
	  { "VendorSpecificData", "iscsi.vendorspecificdata",
	    FT_BYTES, BASE_HEX, NULL, 0,
	    "Vendor Specific Data", HFILL }
	},
	{ &hf_iscsi_HeaderDigest32,
	  { "HeaderDigest", "iscsi.headerdigest32",
	    FT_UINT32, BASE_HEX, NULL, 0,
	    "Header Digest", HFILL }
	},
	{ &hf_iscsi_DataDigest,
	  { "DataDigest", "iscsi.datadigest",
	    FT_BYTES, BASE_HEX, NULL, 0,
	    "Data Digest", HFILL }
	},
	{ &hf_iscsi_DataDigest32,
	  { "DataDigest", "iscsi.datadigest32",
	    FT_UINT32, BASE_HEX, NULL, 0,
	    "Data Digest", HFILL }
	},
	{ &hf_iscsi_Opcode,
	  { "Opcode", "iscsi.opcode",
	    FT_UINT8, BASE_HEX, VALS(iscsi_opcodes), 0,
	    "Opcode", HFILL }
	},
/* #ifdef DRAFT08 */
	{ &hf_iscsi_X,
	  { "X", "iscsi.X",
	    FT_BOOLEAN, 8, TFS(&iscsi_meaning_X), 0x80,
	    "Command Retry", HFILL }
	},
/* #endif */
	{ &hf_iscsi_I,
	  { "I", "iscsi.I",
	    FT_BOOLEAN, 8, TFS(&iscsi_meaning_I), 0x40,
	    "Immediate delivery", HFILL }
	},
	{ &hf_iscsi_Flags,
	  { "Flags", "iscsi.flags",
	    FT_UINT8, BASE_HEX, NULL, 0,
	    "Opcode specific flags", HFILL }
	},
	{ &hf_iscsi_SCSICommand_F,
	  { "F", "iscsi.scsicommand.F",
	    FT_BOOLEAN, 8, TFS(&iscsi_meaning_F), 0x80,
	    "PDU completes command", HFILL }
	},
	{ &hf_iscsi_SCSICommand_R,
	  { "R", "iscsi.scsicommand.R",
	    FT_BOOLEAN, 8, TFS(&iscsi_meaning_R), 0x40,
	    "Command reads from SCSI target", HFILL }
	},
	{ &hf_iscsi_SCSICommand_W,
	  { "W", "iscsi.scsicommand.W",
	    FT_BOOLEAN, 8, TFS(&iscsi_meaning_W), 0x20,
	    "Command writes to SCSI target", HFILL }
	},
	{ &hf_iscsi_SCSICommand_Attr,
	  { "Attr", "iscsi.scsicommand.attr",
	    FT_UINT8, BASE_HEX, VALS(iscsi_scsicommand_taskattrs), 0x07,
	    "SCSI task attributes", HFILL }
	},
	{ &hf_iscsi_SCSICommand_CRN,
	  { "CRN", "iscsi.scsicommand.crn",
	    FT_UINT8, BASE_HEX, NULL, 0,
	    "SCSI command reference number", HFILL }
	},
	{ &hf_iscsi_SCSICommand_AddCDB,
	  { "AddCDB", "iscsi.scsicommand.addcdb",
	    FT_UINT8, BASE_HEX, NULL, 0,
	    "Additional CDB length (in 4 byte units)", HFILL }
	},
	{ &hf_iscsi_DataSegmentLength,
	  { "DataSegmentLength", "iscsi.datasegmentlength",
	    FT_UINT32, BASE_HEX, NULL, 0,
	    "Data segment length (bytes)", HFILL }
	},
	{ &hf_iscsi_TotalAHSLength,
	  { "TotalAHSLength", "iscsi.totalahslength",
	    FT_UINT8, BASE_HEX, NULL, 0,
	    "Total additional header segment length (4 byte words)", HFILL }
	},
	{ &hf_iscsi_LUN,
	  { "LUN", "iscsi.lun",
	    FT_BYTES, BASE_HEX, NULL, 0,
	    "Logical Unit Number", HFILL }
	},
	{ &hf_iscsi_InitiatorTaskTag,
	  { "InitiatorTaskTag", "iscsi.initiatortasktag",
	    FT_UINT32, BASE_HEX, NULL, 0,
	    "Initiator's task tag", HFILL }
	},
	{ &hf_iscsi_ExpectedDataTransferLength,
	  { "ExpectedDataTransferLength", "iscsi.scsicommand.expecteddatatransferlength",
	    FT_UINT32, BASE_HEX, NULL, 0,
	    "Expected length of data transfer", HFILL }
	},
	{ &hf_iscsi_CmdSN,
	  { "CmdSN", "iscsi.cmdsn",
	    FT_UINT32, BASE_HEX, NULL, 0,
	    "Sequence number for this command", HFILL }
	},
	{ &hf_iscsi_ExpStatSN,
	  { "ExpStatSN", "iscsi.expstatsn",
	    FT_UINT32, BASE_HEX, NULL, 0,
	    "Next expected status sequence number", HFILL }
	},
	{ &hf_iscsi_SCSIResponse_ResidualCount,
	  { "ResidualCount", "iscsi.scsiresponse.residualcount",
	    FT_UINT32, BASE_HEX, NULL, 0,
	    "Residual count", HFILL }
	},
	{ &hf_iscsi_StatSN,
	  { "StatSN", "iscsi.statsn",
	    FT_UINT32, BASE_HEX, NULL, 0,
	    "Status sequence number", HFILL }
	},
	{ &hf_iscsi_ExpCmdSN,
	  { "ExpCmdSN", "iscsi.expcmdsn",
	    FT_UINT32, BASE_HEX, NULL, 0,
	    "Next expected command sequence number", HFILL }
	},
	{ &hf_iscsi_MaxCmdSN,
	  { "MaxCmdSN", "iscsi.maxcmdsn",
	    FT_UINT32, BASE_HEX, NULL, 0,
	    "Maximum acceptable command sequence number", HFILL }
	},
	{ &hf_iscsi_SCSIResponse_o,
	  { "o", "iscsi.scsiresponse.o",
	    FT_BOOLEAN, 8, TFS(&iscsi_meaning_o), 0x10,
	    "Bi-directional read residual overflow", HFILL }
	},
	{ &hf_iscsi_SCSIResponse_u,
	  { "u", "iscsi.scsiresponse.u",
	    FT_BOOLEAN, 8, TFS(&iscsi_meaning_u), 0x08,
	    "Bi-directional read residual underflow", HFILL }
	},
	{ &hf_iscsi_SCSIResponse_O,
	  { "O", "iscsi.scsiresponse.O",
	    FT_BOOLEAN, 8, TFS(&iscsi_meaning_O), 0x04,
	    "Residual overflow", HFILL }
	},
	{ &hf_iscsi_SCSIResponse_U,
	  { "U", "iscsi.scsiresponse.U",
	    FT_BOOLEAN, 8, TFS(&iscsi_meaning_U), 0x02,
	    "Residual underflow", HFILL }
	},
	{ &hf_iscsi_SCSIResponse_Status,
	  { "Status", "iscsi.scsiresponse.status",
	    FT_UINT8, BASE_HEX, VALS(scsi_status_val), 0,
	    "SCSI command status value", HFILL }
	},
	{ &hf_iscsi_SCSIResponse_Response,
	  { "Response", "iscsi.scsiresponse.response",
	    FT_UINT8, BASE_HEX, VALS(iscsi_scsi_responses), 0,
	    "SCSI command response value", HFILL }
	},
	{ &hf_iscsi_SCSIResponse_BidiReadResidualCount,
	  { "BidiReadResidualCount", "iscsi.scsiresponse.bidireadresidualcount",
	    FT_UINT32, BASE_HEX, NULL, 0,
	    "Bi-directional read residual count", HFILL }
	},
	{ &hf_iscsi_SenseLength,
	  { "SenseLength", "iscsi.scsiresponse.senselength",
	    FT_UINT16, BASE_HEX, NULL, 0,
	    "Sense data length", HFILL }
	},
	{ &hf_iscsi_SCSIData_F,
	  { "F", "iscsi.scsidata.F",
	    FT_BOOLEAN, 8, TFS(&iscsi_meaning_F), ISCSI_SCSI_DATA_FLAG_F,
	    "Final PDU", HFILL }
	},
	{ &hf_iscsi_SCSIData_A,
	  { "A", "iscsi.scsidata.A",
	    FT_BOOLEAN, 8, TFS(&iscsi_meaning_A), ISCSI_SCSI_DATA_FLAG_A,
	    "Acknowledge Requested", HFILL }
	},
	{ &hf_iscsi_SCSIData_S,
	  { "S", "iscsi.scsidata.S",
	    FT_BOOLEAN, 8, TFS(&iscsi_meaning_S), ISCSI_SCSI_DATA_FLAG_S,
	    "PDU Contains SCSI command status", HFILL }
	},
	{ &hf_iscsi_SCSIData_U,
	  { "U", "iscsi.scsidata.U",
	    FT_BOOLEAN, 8,  TFS(&iscsi_meaning_U), ISCSI_SCSI_DATA_FLAG_U,
	    "Residual underflow", HFILL }
	},
	{ &hf_iscsi_SCSIData_O,
	  { "O", "iscsi.scsidata.O",
	    FT_BOOLEAN, 8,  TFS(&iscsi_meaning_O), ISCSI_SCSI_DATA_FLAG_O,
	    "Residual overflow", HFILL }
	},
	{ &hf_iscsi_TargetTransferTag,
	  { "TargetTransferTag", "iscsi.targettransfertag",
	    FT_UINT32, BASE_HEX, NULL, 0,
	    "Target transfer tag", HFILL }
	},
	{ &hf_iscsi_BufferOffset,
	  { "BufferOffset", "iscsi.bufferOffset",
	    FT_UINT32, BASE_HEX, NULL, 0,
	    "Buffer offset", HFILL }
	},
	{ &hf_iscsi_SCSIData_ResidualCount,
	  { "ResidualCount", "iscsi.scsidata.readresidualcount",
	    FT_UINT32, BASE_HEX, NULL, 0,
	    "Residual count", HFILL }
	},
	{ &hf_iscsi_DataSN,
	  { "DataSN", "iscsi.datasn",
	    FT_UINT32, BASE_HEX, NULL, 0,
	    "Data sequence number", HFILL }
	},
	{ &hf_iscsi_VersionMax,
	  { "VersionMax", "iscsi.versionmax",
	    FT_UINT8, BASE_HEX, NULL, 0,
	    "Maximum supported protocol version", HFILL }
	},
	{ &hf_iscsi_VersionMin,
	  { "VersionMin", "iscsi.versionmin",
	    FT_UINT8, BASE_HEX, NULL, 0,
	    "Minimum supported protocol version", HFILL }
	},
	{ &hf_iscsi_VersionActive,
	  { "VersionActive", "iscsi.versionactive",
	    FT_UINT8, BASE_HEX, NULL, 0,
	    "Negotiated protocol version", HFILL }
	},
	{ &hf_iscsi_CID,
	  { "CID", "iscsi.cid",
	    FT_UINT16, BASE_HEX, NULL, 0,
	    "Connection identifier", HFILL }
	},
/* #ifdef DRAFT08 */
	{ &hf_iscsi_ISID8,
	  { "ISID", "iscsi.isid",
	    FT_UINT16, BASE_HEX, NULL, 0,
	    "Initiator part of session identifier", HFILL }
	},
/* #else */
	{ &hf_iscsi_ISID,
	  { "ISID", "iscsi.isid",
	    FT_BYTES, BASE_HEX, NULL, 0,
	    "Initiator part of session identifier", HFILL }
	},
/* #ifdef DRAFT09 */
	{ &hf_iscsi_ISID_Type,
	  { "ISID_Type", "iscsi.isid.type",
	    FT_UINT8, BASE_HEX, VALS(iscsi_isid_type), 0,
	    "Initiator part of session identifier - type", HFILL }
	},
	{ &hf_iscsi_ISID_NamingAuthority,
	  { "ISID_NamingAuthority", "iscsi.isid.namingauthority",
	    FT_UINT24, BASE_HEX, NULL, 0,
	    "Initiator part of session identifier - naming authority", HFILL }
	},
	{ &hf_iscsi_ISID_Qualifier,
	  { "ISID_Qualifier", "iscsi.isid.qualifier",
	    FT_UINT8, BASE_HEX, NULL, 0,
	    "Initiator part of session identifier - qualifier", HFILL }
	},
/* #else */
	{ &hf_iscsi_ISID_t,
	  { "ISID_t", "iscsi.isid.t",
	    FT_UINT8, BASE_HEX, VALS(iscsi_isid_type), 0xc0,
	    "Initiator part of session identifier - t", HFILL }
	},
	{ &hf_iscsi_ISID_a,
	  { "ISID_a", "iscsi.isid.a",
	    FT_UINT8, BASE_HEX, NULL, 0x3f,
	    "Initiator part of session identifier - a", HFILL }
	},
	{ &hf_iscsi_ISID_b,
	  { "ISID_b", "iscsi.isid.b",
	    FT_UINT16, BASE_HEX, NULL, 0,
	    "Initiator part of session identifier - b", HFILL }
	},
	{ &hf_iscsi_ISID_c,
	  { "ISID_c", "iscsi.isid.c",
	    FT_UINT8, BASE_HEX, NULL, 0,
	    "Initiator part of session identifier - c", HFILL }
	},
	{ &hf_iscsi_ISID_d,
	  { "ISID_d", "iscsi.isid.d",
	    FT_UINT16, BASE_HEX, NULL, 0,
	    "Initiator part of session identifier - d", HFILL }
	},
/* #endif */
/* #endif */
	{ &hf_iscsi_TSID,
	  { "TSID", "iscsi.tsid",
	    FT_UINT16, BASE_HEX, NULL, 0,
	    "Target part of session identifier", HFILL }
	},
	{ &hf_iscsi_TSIH,
	  { "TSIH", "iscsi.tsih",
	    FT_UINT16, BASE_HEX, NULL, 0,
	    "Target session identifying handle", HFILL }
	},
	{ &hf_iscsi_InitStatSN,
	  { "InitStatSN", "iscsi.initstatsn",
	    FT_UINT32, BASE_HEX, NULL, 0,
	    "Initial status sequence number", HFILL }
	},
	{ &hf_iscsi_InitCmdSN,
	  { "InitCmdSN", "iscsi.initcmdsn",
	    FT_UINT32, BASE_HEX, NULL, 0,
	    "Initial command sequence number", HFILL }
	},
	{ &hf_iscsi_Login_T,
	  { "T", "iscsi.login.T",
	    FT_BOOLEAN, 8, TFS(&iscsi_meaning_T), 0x80,
	    "Transit to next login stage",  HFILL }
	},
	{ &hf_iscsi_Login_C,
	  { "C", "iscsi.login.C",
	    FT_BOOLEAN, 8, TFS(&iscsi_meaning_C), 0x40,
	    "Text incomplete",  HFILL }
	},
/* #ifdef DRAFT09 */
	{ &hf_iscsi_Login_X,
	  { "X", "iscsi.login.X",
	    FT_BOOLEAN, 8, TFS(&iscsi_meaning_login_X), 0x40,
	    "Restart Connection",  HFILL }
	},
/* #endif */
	{ &hf_iscsi_Login_CSG,
	  { "CSG", "iscsi.login.csg",
	    FT_UINT8, BASE_HEX, VALS(iscsi_login_stage), CSG_MASK,
	    "Current stage",  HFILL }
	},
	{ &hf_iscsi_Login_NSG,
	  { "NSG", "iscsi.login.nsg",
	    FT_UINT8, BASE_HEX, VALS(iscsi_login_stage), NSG_MASK,
	    "Next stage",  HFILL }
	},
	{ &hf_iscsi_Login_Status,
	  { "Status", "iscsi.login.status",
	    FT_UINT16, BASE_HEX, VALS(iscsi_login_status), 0,
	    "Status class and detail", HFILL }
	},
	{ &hf_iscsi_KeyValue,
	  { "KeyValue", "iscsi.keyvalue",
	    FT_STRING, 0, NULL, 0,
	    "Key/value pair", HFILL }
	},
	{ &hf_iscsi_Text_F,
	  { "F", "iscsi.text.F",
	    FT_BOOLEAN, 8, TFS(&iscsi_meaning_F), 0x80,
	    "Final PDU in text sequence", HFILL }
	},
	{ &hf_iscsi_Text_C,
	  { "C", "iscsi.text.C",
	    FT_BOOLEAN, 8, TFS(&iscsi_meaning_C), 0x40,
	    "Text incomplete", HFILL }
	},
	{ &hf_iscsi_ExpDataSN,
	  { "ExpDataSN", "iscsi.expdatasn",
	    FT_UINT32, BASE_HEX, NULL, 0,
	    "Next expected data sequence number", HFILL }
	},
	{ &hf_iscsi_R2TSN,
	  { "R2TSN", "iscsi.r2tsn",
	    FT_UINT32, BASE_HEX, NULL, 0,
	    "R2T PDU Number", HFILL }
	},
	{ &hf_iscsi_TaskManagementFunction_Response,
	  { "Response", "iscsi.taskmanfun.response",
	    FT_UINT8, BASE_HEX, VALS(iscsi_task_management_responses), 0,
	    "Response", HFILL }
	},
	{ &hf_iscsi_TaskManagementFunction_ReferencedTaskTag,
	  { "ReferencedTaskTag", "iscsi.taskmanfun.referencedtasktag",
	    FT_UINT32, BASE_HEX, NULL, 0,
	    "Referenced task tag", HFILL }
	},
	{ &hf_iscsi_RefCmdSN,
	  { "RefCmdSN", "iscsi.refcmdsn",
	    FT_UINT32, BASE_HEX, NULL, 0,
	    "Command sequence number for command to be aborted", HFILL }
	},
	{ &hf_iscsi_TaskManagementFunction_Function,
	  { "Function", "iscsi.taskmanfun.function",
	    FT_UINT8, BASE_HEX, VALS(iscsi_task_management_functions), 0x7F,
	    "Requested task function", HFILL }
	},
	{ &hf_iscsi_Logout_Reason,
	  { "Reason", "iscsi.logout.reason",
	    FT_UINT8, BASE_HEX, VALS(iscsi_logout_reasons), 0x7F,
	    "Reason for logout", HFILL }
	},
	{ &hf_iscsi_Logout_Response,
	  { "Response", "iscsi.logout.response",
	    FT_UINT8, BASE_HEX, VALS(iscsi_logout_response), 0,
	    "Logout response", HFILL }
	},
	{ &hf_iscsi_Time2Wait,
	  { "Time2Wait", "iscsi.time2wait",
	    FT_UINT16, BASE_HEX, NULL, 0,
	    "Time2Wait", HFILL }
	},
	{ &hf_iscsi_Time2Retain,
	  { "Time2Retain", "iscsi.time2retain",
	    FT_UINT16, BASE_HEX, NULL, 0,
	    "Time2Retain", HFILL }
	},
	{ &hf_iscsi_DesiredDataLength,
	  { "DesiredDataLength", "iscsi.desireddatalength",
	    FT_UINT32, BASE_HEX, NULL, 0,
	    "Desired data length (bytes)", HFILL }
	},
	{ &hf_iscsi_AsyncEvent,
	  { "AsyncEvent", "iscsi.asyncevent",
	    FT_UINT8, BASE_HEX, VALS(iscsi_asyncevents), 0,
	    "Async event type", HFILL }
	},
	{ &hf_iscsi_EventVendorCode,
	  { "EventVendorCode", "iscsi.eventvendorcode",
	    FT_UINT8, BASE_HEX, NULL, 0,
	    "Event vendor code", HFILL }
	},
	{ &hf_iscsi_Parameter1,
	  { "Parameter1", "iscsi.parameter1",
	    FT_UINT16, BASE_HEX, NULL, 0,
	    "Parameter 1", HFILL }
	},
	{ &hf_iscsi_Parameter2,
	  { "Parameter2", "iscsi.parameter2",
	    FT_UINT16, BASE_HEX, NULL, 0,
	    "Parameter 2", HFILL }
	},
	{ &hf_iscsi_Parameter3,
	  { "Parameter3", "iscsi.parameter3",
	    FT_UINT16, BASE_HEX, NULL, 0,
	    "Parameter 3", HFILL }
	},
	{ &hf_iscsi_Reject_Reason,
	  { "Reason", "iscsi.reject.reason",
	    FT_UINT8, BASE_HEX, VALS(iscsi_reject_reasons), 0,
	    "Reason for command rejection", HFILL }
	},
	{ &hf_iscsi_snack_type,
	  { "S", "iscsi.snack.type",
	    FT_UINT8, BASE_DEC, VALS(iscsi_snack_types), 0x0f,
	    "Type of SNACK requested", HFILL }
	},
	{ &hf_iscsi_BegRun,
	  { "BegRun", "iscsi.snack.begrun",
	    FT_UINT32, BASE_HEX, NULL, 0,
	    "First missed DataSN or StatSN", HFILL }
	},
	{ &hf_iscsi_RunLength,
	  { "RunLength", "iscsi.snack.runlength",
	    FT_UINT32, BASE_HEX, NULL, 0,
	    "Number of additional missing status PDUs in this run", HFILL }
	},
    };

    /* Setup protocol subtree array */
    static gint *ett[] = {
	&ett_iscsi,
	&ett_iscsi_KeyValues,
	&ett_iscsi_CDB,
	&ett_iscsi_Flags,
/* #ifndef DRAFT08 */
	&ett_iscsi_ISID,
/* #endif */
    };

    /* Register the protocol name and description */
    proto_iscsi = proto_register_protocol("iSCSI", "iSCSI", "iscsi");

    /* Required function calls to register the header fields and
     * subtrees used */
    proto_register_field_array(proto_iscsi, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));
    register_init_routine (&iscsi_init_protocol);

    {
	module_t *iscsi_module = prefs_register_protocol(proto_iscsi, NULL);

	prefs_register_enum_preference(iscsi_module,
				       "protocol_version",
				       "Protocol version",
				       "The iSCSI protocol version",
				       &iscsi_protocol_version,
				       iscsi_protocol_versions,
				       FALSE);

	prefs_register_bool_preference(iscsi_module,
				       "desegment_iscsi_messages",
				       "Reassemble iSCSI messages\nspanning multiple TCP segments",
				       "Whether the iSCSI dissector should reassemble messages spanning multiple TCP segments."
				       " To use this option, you must also enable \"Allow subdissectors to reassemble TCP streams\" in the TCP protocol settings.",
				       &iscsi_desegment);

	prefs_register_bool_preference(iscsi_module,
				       "bogus_pdu_filter",
				       "Enable bogus pdu filter",
				       "When enabled, packets that appear bogus are ignored",
				       &enable_bogosity_filter);

	prefs_register_bool_preference(iscsi_module,
				       "demand_good_f_bit",
				       "Ignore packets with bad F bit",
				       "Ignore packets that haven't set the F bit when they should have",
				       &demand_good_f_bit);

	prefs_register_uint_preference(iscsi_module,
				       "bogus_pdu_max_data_len",
				       "Bogus pdu max data length threshold",
				       "Treat packets whose data segment length is greater than this value as bogus",
				       10,
				       &bogus_pdu_data_length_threshold);


	prefs_register_uint_preference(iscsi_module,
				       "target_port",
				       "Target port",
				       "Port number of iSCSI target",
				       10,
				       &iscsi_port);

	prefs_register_bool_preference(iscsi_module,
				       "enable_data_digests",
				       "Enable data digests",
				       "When enabled, pdus are assumed to contain a data digest",
				       &enableDataDigests);

	prefs_register_bool_preference(iscsi_module,
				       "data_digest_is_crc32c",
				       "Data digest is CRC32C",
				       "When enabled, data digests are assumed to be CRC32C",
				       &dataDigestIsCRC32);

	prefs_register_uint_preference(iscsi_module,
				       "data_digest_size",
				       "Data digest size",
				       "The size of a data digest (bytes)",
				       10,
				       &dataDigestSize);

	/* Preference supported in older versions.
	   Register them as obsolete. */
	prefs_register_obsolete_preference(iscsi_module,
				       "version_03_compatible");
	prefs_register_obsolete_preference(iscsi_module,
				       "bogus_pdu_max_digest_padding");
	prefs_register_obsolete_preference(iscsi_module,
				       "header_digest_is_crc32c");
	prefs_register_obsolete_preference(iscsi_module,
				       "header_digest_size");
	prefs_register_obsolete_preference(iscsi_module,
				       "enable_header_digests");
    }
}


/*
 * If this dissector uses sub-dissector registration add a
 * registration routine.
 */

/*
 * This format is required because a script is used to find these
 * routines and create the code that calls these routines.
 */
void
proto_reg_handoff_iscsi(void)
{
    heur_dissector_add("tcp", dissect_iscsi_heur, proto_iscsi);

    iscsi_handle = create_dissector_handle(dissect_iscsi_handle, proto_iscsi);
    dissector_add_handle("tcp.port", iscsi_handle);
}
