/* packet-iscsi.c
 * Routines for iSCSI dissection
 * Copyright 2001, Eurologic and Mark Burton <markb@ordern.com>
 *
 * Conforms to the protocol described in: draft-ietf-ips-iscsi-08.txt
 *
 * $Id: packet-iscsi.c,v 1.12 2001/10/20 18:30:50 guy Exp $
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

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif

#include <glib.h>

#ifdef NEED_SNPRINTF_H
# include "snprintf.h"
#endif

#include "packet.h"
#include "prefs.h"

static int enable_bogosity_filter = TRUE;
static guint32 bogus_pdu_data_length_threshold = 1024 * 1024;
static guint32 bogus_pdu_max_digest_padding = 20;

static int enable_force_header_digest_crc32 = FALSE;

/* Initialize the protocol and registered fields */
static int proto_iscsi = -1;
static int hf_iscsi_Padding = -1;
static int hf_iscsi_ping_data = -1;
static int hf_iscsi_immediate_data = -1;
static int hf_iscsi_sense_data = -1;
static int hf_iscsi_write_data = -1;
static int hf_iscsi_read_data = -1;
static int hf_iscsi_error_pdu_data = -1;
static int hf_iscsi_Opcode = -1;
static int hf_iscsi_Flags = -1;
static int hf_iscsi_HeaderDigest32 = -1;
static int hf_iscsi_X = -1;
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
static int hf_iscsi_SCSICommand_CDB = -1;
static int hf_iscsi_SCSICommand_CDB0 = -1;
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
static int hf_iscsi_SCSIData_F = -1;
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
static int hf_iscsi_ISID = -1;
static int hf_iscsi_TSID = -1;
static int hf_iscsi_InitStatSN = -1;
static int hf_iscsi_InitCmdSN = -1;
static int hf_iscsi_Login_T = -1;
static int hf_iscsi_Login_CSG = -1;
static int hf_iscsi_Login_NSG = -1;
static int hf_iscsi_Login_Stage = -1;
static int hf_iscsi_Login_Status = -1;
static int hf_iscsi_KeyValue = -1;
static int hf_iscsi_Text_F = -1;
static int hf_iscsi_ExpDataSN = -1;
static int hf_iscsi_R2TSN = -1;
static int hf_iscsi_SCSITask_ReferencedTaskTag = -1;
static int hf_iscsi_RefCmdSN = -1;
static int hf_iscsi_SCSITask_Function = -1;
static int hf_iscsi_SCSITask_Response = -1;
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
static gint ett_iscsi_KeyValues = -1;
static gint ett_iscsi_CDB = -1;
static gint ett_iscsi_Flags = -1;

#define X_BIT 0x80
#define I_BIT 0x40

#define TARGET_OPCODE_BIT 0x20

#define ISCSI_OPCODE_NOP_OUT                      0x00
#define ISCSI_OPCODE_SCSI_COMMAND                 0x01
#define ISCSI_OPCODE_SCSI_TASK_MANAGEMENT_COMMAND 0x02
#define ISCSI_OPCODE_LOGIN_COMMAND                0x03
#define ISCSI_OPCODE_TEXT_COMMAND                 0x04
#define ISCSI_OPCODE_SCSI_DATA_OUT                0x05
#define ISCSI_OPCODE_LOGOUT_COMMAND               0x06
#define ISCSI_OPCODE_SNACK_REQUEST                0x10

#define ISCSI_OPCODE_NOP_IN                        (0x20 | X_BIT | I_BIT)
#define ISCSI_OPCODE_SCSI_RESPONSE                 (0x21 | X_BIT | I_BIT)
#define ISCSI_OPCODE_SCSI_TASK_MANAGEMENT_RESPONSE (0x22 | X_BIT | I_BIT)
#define ISCSI_OPCODE_LOGIN_RESPONSE                (0x23 | X_BIT | I_BIT)
#define ISCSI_OPCODE_TEXT_RESPONSE                 (0x24 | X_BIT | I_BIT)
#define ISCSI_OPCODE_SCSI_DATA_IN                  (0x25 | X_BIT | I_BIT)
#define ISCSI_OPCODE_LOGOUT_RESPONSE               (0x26 | X_BIT | I_BIT)
#define ISCSI_OPCODE_R2T                           (0x31 | X_BIT | I_BIT)
#define ISCSI_OPCODE_ASYNC_MESSAGE                 (0x32 | X_BIT | I_BIT)
#define ISCSI_OPCODE_REJECT                        (0x3f | X_BIT | I_BIT)

static const value_string iscsi_opcodes[] = {
  { ISCSI_OPCODE_NOP_OUT,                      "NOP Out" },
  { ISCSI_OPCODE_SCSI_COMMAND,                 "SCSI Command" },
  { ISCSI_OPCODE_SCSI_TASK_MANAGEMENT_COMMAND, "SCSI Task Management Command" },
  { ISCSI_OPCODE_LOGIN_COMMAND,                "Login Command" },
  { ISCSI_OPCODE_TEXT_COMMAND,                 "Text Command" },
  { ISCSI_OPCODE_SCSI_DATA_OUT,                "SCSI Write Data" },
  { ISCSI_OPCODE_LOGOUT_COMMAND,               "Logout Command" },
  { ISCSI_OPCODE_SNACK_REQUEST,                "SNACK Request" },

  { ISCSI_OPCODE_NOP_IN,                        "NOP In" },
  { ISCSI_OPCODE_SCSI_RESPONSE,                 "SCSI Command Response" },
  { ISCSI_OPCODE_SCSI_TASK_MANAGEMENT_RESPONSE, "SCSI Task Management Response" },
  { ISCSI_OPCODE_LOGIN_RESPONSE,                "Login Response" },
  { ISCSI_OPCODE_TEXT_RESPONSE,                 "Text Response" },
  { ISCSI_OPCODE_SCSI_DATA_IN,                  "SCSI Read Data" },
  { ISCSI_OPCODE_LOGOUT_RESPONSE,               "Logout Response" },
  { ISCSI_OPCODE_R2T,                           "Ready To Transfer" },
  { ISCSI_OPCODE_ASYNC_MESSAGE,                 "Asynchronous Message" },
  { ISCSI_OPCODE_REJECT,                        "Reject"},
  {0, NULL},
};

static const true_false_string iscsi_meaning_X = {
    "Retry",
    "Not retry"
};

static const true_false_string iscsi_meaning_I = {
    "Immediate delivery",
    "Queued delivery"
};

static const true_false_string iscsi_meaning_F = {
    "Final PDU in sequence",
    "Not final PDU in sequence"
};

static const true_false_string iscsi_meaning_T = {
    "Transit to next login stage",
    "Stay in current login stage"
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

static const value_string iscsi_scsi_cdb0[] = {
    {0x00, "TEST_UNIT_READY"},
    {0x01, "REZERO_UNIT"},
    {0x03, "REQUEST_SENSE"},
    {0x04, "FORMAT_UNIT"},
    {0x05, "READ_BLOCK_LIMITS"},
    {0x07, "REASSIGN_BLOCKS"},
    {0x08, "READ_6"},
    {0x0a, "WRITE_6"},
    {0x0b, "SEEK_6"},
    {0x0f, "READ_REVERSE"},
    {0x10, "WRITE_FILEMARKS"},
    {0x11, "SPACE"},
    {0x12, "INQUIRY"},
    {0x14, "RECOVER_BUFFERED_DATA"},
    {0x15, "MODE_SELECT"},
    {0x16, "RESERVE"},
    {0x17, "RELEASE"},
    {0x18, "COPY"},
    {0x19, "ERASE"},
    {0x1a, "MODE_SENSE"},
    {0x1b, "START_STOP"},
    {0x1c, "RECEIVE_DIAGNOSTIC"},
    {0x1d, "SEND_DIAGNOSTIC"},
    {0x1e, "ALLOW_MEDIUM_REMOVAL"},
    {0x24, "SET_WINDOW"},
    {0x25, "READ_CAPACITY"},
    {0x28, "READ_10"},
    {0x2a, "WRITE_10"},
    {0x2b, "SEEK_10"},
    {0x2e, "WRITE_VERIFY"},
    {0x2f, "VERIFY"},
    {0x30, "SEARCH_HIGH"},
    {0x31, "SEARCH_EQUAL"},
    {0x32, "SEARCH_LOW"},
    {0x33, "SET_LIMITS"},
    {0x34, "PRE_FETCH"},
    {0x34, "READ_POSITION"},
    {0x35, "SYNCHRONIZE_CACHE"},
    {0x36, "LOCK_UNLOCK_CACHE"},
    {0x37, "READ_DEFECT_DATA"},
    {0x38, "MEDIUM_SCAN"},
    {0x39, "COMPARE"},
    {0x3a, "COPY_VERIFY"},
    {0x3b, "WRITE_BUFFER"},
    {0x3c, "READ_BUFFER"},
    {0x3d, "UPDATE_BLOCK"},
    {0x3e, "READ_LONG"},
    {0x3f, "WRITE_LONG"},
    {0x40, "CHANGE_DEFINITION"},
    {0x41, "WRITE_SAME"},
    {0x43, "READ_TOC"},
    {0x4c, "LOG_SELECT"},
    {0x4d, "LOG_SENSE"},
    {0x55, "MODE_SELECT_10"},
    {0x5a, "MODE_SENSE_10"},
    {0xa5, "MOVE_MEDIUM"},
    {0xa8, "READ_12"},
    {0xaa, "WRITE_12"},
    {0xae, "WRITE_VERIFY_12"},
    {0xb0, "SEARCH_HIGH_12"},
    {0xb1, "SEARCH_EQUAL_12"},
    {0xb2, "SEARCH_LOW_12"},
    {0xb8, "READ_ELEMENT_STATUS"},
    {0xb6, "SEND_VOLUME_TAG"},
    {0xea, "WRITE_LONG_2"},
    {0, NULL},
};

static const value_string iscsi_scsi_statuses[] = {
    {0x00, "Good"},
    {0x01, "Check condition"},
    {0x02, "Condition good"},
    {0x04, "Busy"},
    {0x08, "Intermediate good"},
    {0x0a, "Intermediate c good"},
    {0x0c, "Reservation conflict"},
    {0x11, "Command terminated"},
    {0x14, "Queue full"},
    {0, NULL},
};

static const value_string iscsi_task_responses[] = {
    {0, "Function complete"},
    {1, "Task not in task set"},
    {2, "LUN does not exist"},
    {255, "Function rejected"},
    {0, NULL},
};

static const value_string iscsi_task_functions[] = {
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
    {0x0201, "Athentication failed"},
    {0x0202, "Authorisation failure"},
    {0x0203, "Target not found"},
    {0x0204, "Target removed"},
    {0x0205, "Unsupported version"},
    {0x0206, "Too many connections"},
    {0x0207, "Missing parameter"},
    {0x0208, "Can't include in session"},
    {0x0209, "Session type not supported"},
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
    {0, NULL},
};

static const value_string iscsi_snack_types[] = {
    {0, "Data/R2T"},
    {1, "Status"},
    {0, NULL}
};

static const value_string iscsi_reject_reasons[] = {
    {0x01, "Full feature phase command before login"},
    {0x02, "Data (payload) digest error"},
    {0x03, "Data SNACK reject"},
    {0x04, "Protocol error"},
    {0x05, "Command not supported in this session type"},
    {0x06, "Immediate command reject (too many immediate commands)"},
    {0x07, "Task in progress"},
    {0x08, "Invalid SNACK"},
    {0x09, "Bookmark reject (no bookmark for this initiator task tag)"},
    {0x0a, "Bookmark reject (can't generate bookmark - out of resources)"},
    {0x0b, "Negotiation reset"},
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

static guint32
calculateCRC32(const void *buf, int len, guint32 crc) {
    guint8 *p = (guint8 *)buf;
    while(len-- > 0)
        crc = crc32Table[(crc ^ *p++) & 0xff] ^ (crc >> 8);
    return crc;
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
handleHeaderDigest(proto_item *ti, tvbuff_t *tvb, guint offset, int headerLen) {
    int available_bytes = tvb_length_remaining(tvb, offset);
    if(available_bytes >= (headerLen + 4)) {
	guint32 crc = ~calculateCRC32(tvb_get_ptr(tvb, offset, headerLen), headerLen, CRC32C_PRELOAD);
	guint32 sent = tvb_get_ntohl(tvb, offset + headerLen);
	if(crc == sent || enable_force_header_digest_crc32) {
	    if(crc == sent) {
		proto_tree_add_uint_format(ti, hf_iscsi_HeaderDigest32, tvb, offset + headerLen, 4, sent, "HeaderDigest: 0x%08x (Good CRC32)", sent);
		return 4;
	    }
	    else {
		proto_tree_add_uint_format(ti, hf_iscsi_HeaderDigest32, tvb, offset + headerLen, 4, sent, "HeaderDigest: 0x%08x (Bad CRC32)", sent);
		return 4;
	    }
	}
    }
    return 0;
}

/* Code to actually dissect the packets */
static int
dissect_iscsi_pdu(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, guint offset, guint8 opcode, const char *opcode_str, guint32 data_segment_len) {
    guint original_offset = offset;
    proto_item *ti;
    char *scsi_command_name = NULL;
    guint cdb_offset = offset + 32; /* offset of CDB from start of PDU */
    guint end_offset = offset + tvb_length_remaining(tvb, offset);

    /* Make entries in Protocol column and Info column on summary display */
    if (check_col(pinfo->fd, COL_PROTOCOL))
	col_set_str(pinfo->fd, COL_PROTOCOL, "iSCSI");

    if (check_col(pinfo->fd, COL_INFO)) {

	col_add_str(pinfo->fd, COL_INFO, (char *)opcode_str);

	if((opcode & ~(X_BIT | I_BIT)) == ISCSI_OPCODE_SCSI_COMMAND) {
	    /* SCSI Command */
	    guint8 cdb0 = tvb_get_guint8(tvb, cdb_offset);
	    scsi_command_name = match_strval(cdb0, iscsi_scsi_cdb0);
	    if(cdb0 == 0x08 || cdb0 == 0x0a) {
		/* READ_6 and WRITE_6 */
		guint lba = tvb_get_ntohl(tvb, cdb_offset) & 0x1fffff;
		guint len = tvb_get_guint8(tvb, cdb_offset + 4);
		col_append_fstr(pinfo->fd, COL_INFO, " (%s LBA 0x%06x len 0x%02x)", scsi_command_name, lba, len);
	    }
	    else if(cdb0 == 0x28 || cdb0 == 0x2a) {
		/* READ_10 and WRITE_10 */
		guint lba = tvb_get_ntohl(tvb, cdb_offset + 2);
		guint len = tvb_get_ntohs(tvb, cdb_offset + 7);
		col_append_fstr(pinfo->fd, COL_INFO, " (%s LBA 0x%08x len 0x%04x)", scsi_command_name, lba, len);
	    }
	    else if(scsi_command_name != NULL)
		col_append_fstr(pinfo->fd, COL_INFO, " (%s)", scsi_command_name);
	}
	else if(opcode == ISCSI_OPCODE_SCSI_RESPONSE) {
	    /* SCSI Command Response */
	    const char *blurb = NULL;
	    /* look at response byte */
	    if(tvb_get_guint8(tvb, offset + 2) == 0) {
		/* command completed at target */
		blurb = match_strval(tvb_get_guint8(tvb, offset + 3) >> 1, iscsi_scsi_statuses);
	    }
	    else
		blurb = "Target Failure";
	    if(blurb != NULL)
		col_append_fstr(pinfo->fd, COL_INFO, " (%s)", blurb);
	}
    }

    /* In the interest of speed, if "tree" is NULL, don't do any
       work not necessary to generate protocol tree items. */
    if (tree) {

	/* create display subtree for the protocol */
	ti = proto_tree_add_protocol_format(tree, proto_iscsi, tvb,
					    offset, 0, "iSCSI (%s)",
					    (char *)opcode_str);

	proto_tree_add_uint(ti, hf_iscsi_Opcode, tvb,
			    offset + 0, 1, opcode);
	if((opcode & TARGET_OPCODE_BIT) == 0) {
	    /* initiator -> target */
	    gint b = tvb_get_guint8(tvb, offset + 0);
	    if(opcode != ISCSI_OPCODE_SCSI_DATA_OUT &&
	       opcode != ISCSI_OPCODE_LOGOUT_COMMAND &&
	       opcode != ISCSI_OPCODE_SNACK_REQUEST)
		proto_tree_add_boolean(ti, hf_iscsi_X, tvb, offset + 0, 1, b);
	    if(opcode != ISCSI_OPCODE_SCSI_DATA_OUT)
		proto_tree_add_boolean(ti, hf_iscsi_I, tvb, offset + 0, 1, b);
	}

	if(opcode == ISCSI_OPCODE_NOP_OUT) {
	    /* NOP Out */
	    proto_tree_add_uint(ti, hf_iscsi_DataSegmentLength, tvb, offset + 5, 3, data_segment_len);
	    proto_tree_add_item(ti, hf_iscsi_LUN, tvb, offset + 8, 8, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_InitiatorTaskTag, tvb, offset + 16, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_TargetTransferTag, tvb, offset + 20, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_CmdSN, tvb, offset + 24, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_ExpStatSN, tvb, offset + 28, 4, FALSE);
	    offset += 48 + handleHeaderDigest(ti, tvb, offset, 48);
	    if(end_offset > offset) {
		int ping_data_len = iscsi_min(data_segment_len, end_offset - offset);
		if(ping_data_len > 0) {
		    proto_tree_add_item(ti, hf_iscsi_ping_data, tvb, offset, ping_data_len, FALSE);
		    offset += ping_data_len;
		}
		if(offset < end_offset && (offset & 3) != 0) {
		    int padding = 4 - (offset & 3);
		    proto_tree_add_item(ti, hf_iscsi_Padding, tvb, offset, padding, FALSE);
		    offset += padding;
		}
	    }
	}
	else if(opcode == ISCSI_OPCODE_NOP_IN) {
	    /* NOP In */
	    proto_tree_add_uint(ti, hf_iscsi_DataSegmentLength, tvb, offset + 5, 3, data_segment_len);
	    proto_tree_add_item(ti, hf_iscsi_LUN, tvb, offset + 8, 8, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_InitiatorTaskTag, tvb, offset + 16, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_TargetTransferTag, tvb, offset + 20, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_StatSN, tvb, offset + 24, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_ExpCmdSN, tvb, offset + 28, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_MaxCmdSN, tvb, offset + 32, 4, FALSE);
	    offset += 48 + handleHeaderDigest(ti, tvb, offset, 48);
	    if(end_offset > offset) {
		int ping_data_len = iscsi_min(data_segment_len, end_offset - offset);
		if(ping_data_len > 0) {
		    proto_tree_add_item(ti, hf_iscsi_ping_data, tvb, offset, ping_data_len, FALSE);
		    offset += ping_data_len;
		}
		if(offset < end_offset && (offset & 3) != 0) {
		    int padding = 4 - (offset & 3);
		    proto_tree_add_item(ti, hf_iscsi_Padding, tvb, offset, padding, FALSE);
		    offset += padding;
		}
	    }
	}
	else if(opcode == ISCSI_OPCODE_SCSI_COMMAND) {
	    /* SCSI Command */
	    {
		gint b = tvb_get_guint8(tvb, offset + 1);
		proto_item *tf = proto_tree_add_uint(ti, hf_iscsi_Flags, tvb, offset + 1, 1, b);
		proto_tree *tt = proto_item_add_subtree(tf, ett_iscsi_Flags);
		
		proto_tree_add_boolean(tt, hf_iscsi_SCSICommand_F, tvb, offset + 1, 1, b);
		proto_tree_add_boolean(tt, hf_iscsi_SCSICommand_R, tvb, offset + 1, 1, b);
		proto_tree_add_boolean(tt, hf_iscsi_SCSICommand_W, tvb, offset + 1, 1, b);
		proto_tree_add_uint(tt, hf_iscsi_SCSICommand_Attr, tvb, offset + 1, 1, b);
	    }
	    proto_tree_add_item(ti, hf_iscsi_SCSICommand_CRN, tvb, offset + 3, 1, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_TotalAHSLength, tvb, offset + 4, 1, FALSE);
	    proto_tree_add_uint(ti, hf_iscsi_DataSegmentLength, tvb, offset + 5, 3, data_segment_len);
	    proto_tree_add_item(ti, hf_iscsi_LUN, tvb, offset + 8, 8, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_InitiatorTaskTag, tvb, offset + 16, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_ExpectedDataTransferLength, tvb, offset + 20, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_CmdSN, tvb, offset + 24, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_ExpStatSN, tvb, offset + 28, 4, FALSE);
	    {
		/* dissect a little of the CDB for the most common
		 * commands */
		guint8 cdb0 = tvb_get_guint8(tvb, cdb_offset);
		gint cdb_len = 16;
		proto_item *tf;
		/* FIXME - extended CDB */
		if(scsi_command_name == NULL)
		    scsi_command_name = match_strval(cdb0, iscsi_scsi_cdb0);
		if(cdb0 == 0x08 || cdb0 == 0x0a) {
		    /* READ_6 and WRITE_6 */
		    guint lba = tvb_get_ntohl(tvb, cdb_offset) & 0x1fffff;
		    guint len = tvb_get_guint8(tvb, cdb_offset + 4);
		    tf = proto_tree_add_uint_format(ti, hf_iscsi_SCSICommand_CDB0, tvb, cdb_offset, cdb_len, cdb0, "CDB: %s LBA 0x%06x len 0x%02x", scsi_command_name, lba, len);
		}
		else if(cdb0 == 0x28 || cdb0 == 0x2a) {
		    /* READ_10 and WRITE_10 */
		    guint lba = tvb_get_ntohl(tvb, cdb_offset + 2);
		    guint len = tvb_get_ntohs(tvb, cdb_offset + 7);
		    tf = proto_tree_add_uint_format(ti, hf_iscsi_SCSICommand_CDB0, tvb, cdb_offset, cdb_len, cdb0, "CDB: %s LBA 0x%08x len 0x%04x", scsi_command_name, lba, len);
		}
		else
		    tf = proto_tree_add_uint(ti, hf_iscsi_SCSICommand_CDB0, tvb, cdb_offset, cdb_len, cdb0);
		{
		    proto_tree *tt = proto_item_add_subtree(tf, ett_iscsi_CDB);
		    proto_tree_add_item(tt, hf_iscsi_SCSICommand_CDB, tvb, cdb_offset, cdb_len, FALSE);
		}
		offset = cdb_offset + cdb_len + handleHeaderDigest(ti, tvb, offset, cdb_offset + cdb_len);
	    }
	    if(end_offset > offset) {
		int immediate_data_len = iscsi_min(data_segment_len, end_offset - offset);
		if(immediate_data_len > 0) {
		    proto_tree_add_item(ti, hf_iscsi_immediate_data, tvb, offset, immediate_data_len, FALSE);
		    offset += immediate_data_len;
		}
		if(offset < end_offset && (offset & 3) != 0) {
		    int padding = 4 - (offset & 3);
		    proto_tree_add_item(ti, hf_iscsi_Padding, tvb, offset, padding, FALSE);
		    offset += padding;
		}
	    }
	}
	else if(opcode == ISCSI_OPCODE_SCSI_RESPONSE) {
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
	    proto_tree_add_uint(ti, hf_iscsi_DataSegmentLength, tvb, offset + 5, 3, data_segment_len);
	    proto_tree_add_item(ti, hf_iscsi_InitiatorTaskTag, tvb, offset + 16, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_SCSIResponse_ResidualCount, tvb, offset + 20, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_StatSN, tvb, offset + 24, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_ExpCmdSN, tvb, offset + 28, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_MaxCmdSN, tvb, offset + 32, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_ExpDataSN, tvb, offset + 36, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_SCSIResponse_BidiReadResidualCount, tvb, offset + 44, 4, FALSE);
	    offset += 48 + handleHeaderDigest(ti, tvb, offset, 48);
	    if(end_offset > offset) {
		int sense_data_len = iscsi_min(data_segment_len, end_offset - offset);
		if(sense_data_len > 0) {
		    proto_tree_add_item(ti, hf_iscsi_sense_data, tvb, offset, sense_data_len, FALSE);
		    offset += sense_data_len;
		}
		if(offset < end_offset && (offset & 3) != 0) {
		    int padding = 4 - (offset & 3);
		    proto_tree_add_item(ti, hf_iscsi_Padding, tvb, offset, padding, FALSE);
		    offset += padding;
		}
	    }
	}
	else if(opcode == ISCSI_OPCODE_SCSI_TASK_MANAGEMENT_COMMAND) {
	    /* SCSI Task Command */
 	    proto_tree_add_item(ti, hf_iscsi_SCSITask_Function, tvb, offset + 1, 1, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_LUN, tvb, offset + 8, 8, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_InitiatorTaskTag, tvb, offset + 16, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_SCSITask_ReferencedTaskTag, tvb, offset + 20, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_CmdSN, tvb, offset + 24, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_ExpStatSN, tvb, offset + 28, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_RefCmdSN, tvb, offset + 32, 4, FALSE);
	    offset += 48 + handleHeaderDigest(ti, tvb, offset, 48);
	}
	else if(opcode == ISCSI_OPCODE_SCSI_TASK_MANAGEMENT_RESPONSE) {
	    /* SCSI Task Response */
	    proto_tree_add_item(ti, hf_iscsi_SCSITask_Response, tvb, offset + 2, 1, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_InitiatorTaskTag, tvb, offset + 16, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_SCSITask_ReferencedTaskTag, tvb, offset + 20, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_StatSN, tvb, offset + 24, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_ExpCmdSN, tvb, offset + 28, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_MaxCmdSN, tvb, offset + 32, 4, FALSE);
	    offset += 48 + handleHeaderDigest(ti, tvb, offset, 48);
	}
	else if(opcode == ISCSI_OPCODE_LOGIN_COMMAND) {
	    /* Login Command */
	    {
		gint b = tvb_get_guint8(tvb, offset + 1);
#if 0
		proto_item *tf = proto_tree_add_uint(ti, hf_iscsi_Flags, tvb, offset + 1, 1, b);
		proto_tree *tt = proto_item_add_subtree(tf, ett_iscsi_Flags);
#endif
		
		proto_tree_add_boolean(ti, hf_iscsi_Login_T, tvb, offset + 1, 1, b);
		proto_tree_add_item(ti, hf_iscsi_Login_CSG, tvb, offset + 1, 1, FALSE);
		proto_tree_add_item(ti, hf_iscsi_Login_NSG, tvb, offset + 1, 1, FALSE);
	    }
	    proto_tree_add_item(ti, hf_iscsi_VersionMax, tvb, offset + 2, 1, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_VersionMin, tvb, offset + 3, 1, FALSE);
	    proto_tree_add_uint(ti, hf_iscsi_DataSegmentLength, tvb, offset + 5, 3, data_segment_len);
	    proto_tree_add_item(ti, hf_iscsi_CID, tvb, offset + 8, 2, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_ISID, tvb, offset + 12, 2, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_TSID, tvb, offset + 14, 2, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_InitiatorTaskTag, tvb, offset + 16, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_CmdSN, tvb, offset + 24, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_ExpStatSN, tvb, offset + 28, 4, FALSE);
	    offset += 48 + handleHeaderDigest(ti, tvb, offset, 48);
	    if(end_offset > offset) {
		int text_len = iscsi_min(data_segment_len, end_offset - offset);
		if(text_len > 0) {
		    proto_item *tf = proto_tree_add_text(ti, tvb, offset, text_len, "Key/Value Pairs");
		    proto_tree *tt = proto_item_add_subtree(tf, ett_iscsi_KeyValues);
		    offset = addTextKeys(tt, tvb, offset, text_len);
		}
		if(offset < end_offset && (offset & 3) != 0) {
		    int padding = 4 - (offset & 3);
		    proto_tree_add_item(ti, hf_iscsi_Padding, tvb, offset, padding, FALSE);
		    offset += padding;
		}
	    }
	}
	else if(opcode == ISCSI_OPCODE_LOGIN_RESPONSE) {
	    /* Login Response */
	    {
		gint b = tvb_get_guint8(tvb, offset + 1);
#if 0
		proto_item *tf = proto_tree_add_uint(ti, hf_iscsi_Flags, tvb, offset + 1, 1, b);
		proto_tree *tt = proto_item_add_subtree(tf, ett_iscsi_Flags);
#endif
		
		proto_tree_add_boolean(ti, hf_iscsi_Login_T, tvb, offset + 1, 1, b);
		proto_tree_add_item(ti, hf_iscsi_Login_CSG, tvb, offset + 1, 1, FALSE);
		proto_tree_add_item(ti, hf_iscsi_Login_NSG, tvb, offset + 1, 1, FALSE);
	    }

	    proto_tree_add_item(ti, hf_iscsi_VersionMax, tvb, offset + 2, 1, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_VersionActive, tvb, offset + 3, 1, FALSE);
	    proto_tree_add_uint(ti, hf_iscsi_DataSegmentLength, tvb, offset + 5, 3, data_segment_len);
	    proto_tree_add_item(ti, hf_iscsi_ISID, tvb, offset + 12, 2, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_TSID, tvb, offset + 14, 2, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_InitiatorTaskTag, tvb, offset + 16, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_StatSN, tvb, offset + 24, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_ExpCmdSN, tvb, offset + 28, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_MaxCmdSN, tvb, offset + 32, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_Login_Status, tvb, offset + 36, 2, FALSE);
	    offset += 48 + handleHeaderDigest(ti, tvb, offset, 48);
	    if(end_offset > offset) {
		int text_len = iscsi_min(data_segment_len, end_offset - offset);
		if(text_len > 0) {
		    proto_item *tf = proto_tree_add_text(ti, tvb, offset, text_len, "Key/Value Pairs");
		    proto_tree *tt = proto_item_add_subtree(tf, ett_iscsi_KeyValues);
		    offset = addTextKeys(tt, tvb, offset, text_len);
		}
		if(offset < end_offset && (offset & 3) != 0) {
		    int padding = 4 - (offset & 3);
		    proto_tree_add_item(ti, hf_iscsi_Padding, tvb, offset, padding, FALSE);
		    offset += padding;
		}
	    }
	}
	else if(opcode == ISCSI_OPCODE_TEXT_COMMAND) {
	    /* Text Command */
	    {
		gint b = tvb_get_guint8(tvb, offset + 1);
		proto_item *tf = proto_tree_add_uint(ti, hf_iscsi_Flags, tvb, offset + 1, 1, b);
		proto_tree *tt = proto_item_add_subtree(tf, ett_iscsi_Flags);
		
		proto_tree_add_boolean(tt, hf_iscsi_Text_F, tvb, offset + 1, 1, b);
		proto_tree_add_uint(ti, hf_iscsi_DataSegmentLength, tvb, offset + 5, 3, data_segment_len);
	    }
	    proto_tree_add_item(ti, hf_iscsi_InitiatorTaskTag, tvb, offset + 16, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_TargetTransferTag, tvb, offset + 20, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_CmdSN, tvb, offset + 24, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_ExpStatSN, tvb, offset + 28, 4, FALSE);
	    offset += 48 + handleHeaderDigest(ti, tvb, offset, 48);
	    if(end_offset > offset) {
		int text_len = iscsi_min(data_segment_len, end_offset - offset);
		if(text_len > 0) {
		    proto_item *tf = proto_tree_add_text(ti, tvb, offset, text_len, "Key/Value Pairs");
		    proto_tree *tt = proto_item_add_subtree(tf, ett_iscsi_KeyValues);
		    offset = addTextKeys(tt, tvb, offset, text_len);
		}
		if(offset < end_offset && (offset & 3) != 0) {
		    int padding = 4 - (offset & 3);
		    proto_tree_add_item(ti, hf_iscsi_Padding, tvb, offset, padding, FALSE);
		    offset += padding;
		}
	    }
	}
	else if(opcode == ISCSI_OPCODE_TEXT_RESPONSE) {
	    /* Text Response */
	    {
		gint b = tvb_get_guint8(tvb, offset + 1);
		proto_item *tf = proto_tree_add_uint(ti, hf_iscsi_Flags, tvb, offset + 1, 1, b);
		proto_tree *tt = proto_item_add_subtree(tf, ett_iscsi_Flags);
		
		proto_tree_add_boolean(tt, hf_iscsi_Text_F, tvb, offset + 1, 1, b);
		proto_tree_add_uint(ti, hf_iscsi_DataSegmentLength, tvb, offset + 5, 3, data_segment_len);
	    }
	    proto_tree_add_item(ti, hf_iscsi_InitiatorTaskTag, tvb, offset + 16, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_TargetTransferTag, tvb, offset + 20, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_StatSN, tvb, offset + 24, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_ExpCmdSN, tvb, offset + 28, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_MaxCmdSN, tvb, offset + 32, 4, FALSE);
	    offset += 48 + handleHeaderDigest(ti, tvb, offset, 48);
	    if(end_offset > offset) {
		int text_len = iscsi_min(data_segment_len, end_offset - offset);
		if(text_len > 0) {
		    proto_item *tf = proto_tree_add_text(ti, tvb, offset, text_len, "Key/Value Pairs");
		    proto_tree *tt = proto_item_add_subtree(tf, ett_iscsi_KeyValues);
		    offset = addTextKeys(tt, tvb, offset, text_len);
		}
		if(offset < end_offset && (offset & 3) != 0) {
		    int padding = 4 - (offset & 3);
		    proto_tree_add_item(ti, hf_iscsi_Padding, tvb, offset, padding, FALSE);
		    offset += padding;
		}
	    }
	}
	else if(opcode == ISCSI_OPCODE_SCSI_DATA_OUT) {
	    /* SCSI Data Out (write) */
	    {
		gint b = tvb_get_guint8(tvb, offset + 1);
		proto_item *tf = proto_tree_add_uint(ti, hf_iscsi_Flags, tvb, offset + 1, 1, b);
		proto_tree *tt = proto_item_add_subtree(tf, ett_iscsi_Flags);

		proto_tree_add_boolean(tt, hf_iscsi_SCSIData_F, tvb, offset + 1, 1, b);
	    }
	    proto_tree_add_uint(ti, hf_iscsi_DataSegmentLength, tvb, offset + 5, 3, data_segment_len);
	    proto_tree_add_item(ti, hf_iscsi_LUN, tvb, offset + 8, 8, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_InitiatorTaskTag, tvb, offset + 16, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_TargetTransferTag, tvb, offset + 20, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_ExpStatSN, tvb, offset + 28, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_DataSN, tvb, offset + 36, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_BufferOffset, tvb, offset + 40, 4, FALSE);
	    offset += 48 + handleHeaderDigest(ti, tvb, offset, 48);
	    if(end_offset > offset) {
		int write_data_len = iscsi_min(data_segment_len, end_offset - offset);
		if(write_data_len > 0) {
		    proto_tree_add_item(ti, hf_iscsi_write_data, tvb, offset, write_data_len, FALSE);
		    offset += write_data_len;
		}
		if(offset < end_offset && (offset & 3) != 0) {
		    int padding = 4 - (offset & 3);
		    proto_tree_add_item(ti, hf_iscsi_Padding, tvb, offset, padding, FALSE);
		    offset += padding;
		}
	    }
	}
	else if(opcode == ISCSI_OPCODE_SCSI_DATA_IN) {
	    /* SCSI Data In (read) */
	    {
		gint b = tvb_get_guint8(tvb, offset + 1);
		proto_item *tf = proto_tree_add_uint(ti, hf_iscsi_Flags, tvb, offset + 1, 1, b);
		proto_tree *tt = proto_item_add_subtree(tf, ett_iscsi_Flags);

		proto_tree_add_boolean(tt, hf_iscsi_SCSIData_F, tvb, offset + 1, 1, b);
		proto_tree_add_boolean(tt, hf_iscsi_SCSIData_O, tvb, offset + 1, 1, b);
		proto_tree_add_boolean(tt, hf_iscsi_SCSIData_U, tvb, offset + 1, 1, b);
		proto_tree_add_boolean(tt, hf_iscsi_SCSIData_S, tvb, offset + 1, 1, b);
	    }
	    proto_tree_add_item(ti, hf_iscsi_SCSIResponse_Status, tvb, offset + 3, 1, FALSE);
	    proto_tree_add_uint(ti, hf_iscsi_DataSegmentLength, tvb, offset + 5, 3, data_segment_len);
	    proto_tree_add_item(ti, hf_iscsi_InitiatorTaskTag, tvb, offset + 16, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_SCSIData_ResidualCount, tvb, offset + 20, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_StatSN, tvb, offset + 24, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_ExpCmdSN, tvb, offset + 28, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_MaxCmdSN, tvb, offset + 32, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_DataSN, tvb, offset + 36, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_BufferOffset, tvb, offset + 40, 4, FALSE);
	    offset += 48 + handleHeaderDigest(ti, tvb, offset, 48);
	    if(end_offset > offset) {
		int read_data_len = iscsi_min(data_segment_len, end_offset - offset);
		if(read_data_len > 0) {
		    proto_tree_add_item(ti, hf_iscsi_read_data, tvb, offset, read_data_len, FALSE);
		    offset += read_data_len;
		}
		if(offset < end_offset && (offset & 3) != 0) {
		    int padding = 4 - (offset & 3);
		    proto_tree_add_item(ti, hf_iscsi_Padding, tvb, offset, padding, FALSE);
		    offset += padding;
		}
	    }
	}
	else if(opcode == ISCSI_OPCODE_LOGOUT_COMMAND) {
	    /* Logout Command */
	    proto_tree_add_item(ti, hf_iscsi_CID, tvb, offset + 8, 2, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_Logout_Reason, tvb, offset + 11, 1, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_InitiatorTaskTag, tvb, offset + 16, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_ExpStatSN, tvb, offset + 28, 4, FALSE);
	    offset += 48 + handleHeaderDigest(ti, tvb, offset, 48);
	}
	else if(opcode == ISCSI_OPCODE_LOGOUT_RESPONSE) {
	    /* Logout Response */
	    proto_tree_add_item(ti, hf_iscsi_Logout_Response, tvb, offset + 2, 1, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_InitiatorTaskTag, tvb, offset + 16, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_ExpCmdSN, tvb, offset + 28, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_MaxCmdSN, tvb, offset + 32, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_Time2Wait, tvb, offset + 40, 2, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_Time2Retain, tvb, offset + 42, 2, FALSE);
	    offset += 48 + handleHeaderDigest(ti, tvb, offset, 48);
	}
	else if(opcode == ISCSI_OPCODE_SNACK_REQUEST) {
	    /* SNACK Request */
	    {
		gint b = tvb_get_guint8(tvb, offset + 1);
#if 0
		proto_item *tf = proto_tree_add_uint(ti, hf_iscsi_Flags, tvb, offset + 1, 1, b);
		proto_tree *tt = proto_item_add_subtree(tf, ett_iscsi_Flags);
#endif

		proto_tree_add_item(ti, hf_iscsi_snack_type, tvb, offset + 1, 1, b);
	    }
	    proto_tree_add_item(ti, hf_iscsi_InitiatorTaskTag, tvb, offset + 16, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_BegRun, tvb, offset + 20, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_RunLength, tvb, offset + 24, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_ExpStatSN, tvb, offset + 28, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_ExpDataSN, tvb, offset + 36, 4, FALSE);
	    offset += 48 + handleHeaderDigest(ti, tvb, offset, 48);
	}
	else if(opcode == ISCSI_OPCODE_R2T) {
	    /* R2T */
	    proto_tree_add_item(ti, hf_iscsi_InitiatorTaskTag, tvb, offset + 16, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_TargetTransferTag, tvb, offset + 20, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_StatSN, tvb, offset + 24, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_ExpCmdSN, tvb, offset + 28, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_MaxCmdSN, tvb, offset + 32, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_R2TSN, tvb, offset + 36, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_BufferOffset, tvb, offset + 40, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_DesiredDataLength, tvb, offset + 44, 4, FALSE);
	    offset += 48 + handleHeaderDigest(ti, tvb, offset, 48);
	}
	else if(opcode == ISCSI_OPCODE_ASYNC_MESSAGE) {
	    /* Asynchronous Message */
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
	    offset += 48 + handleHeaderDigest(ti, tvb, offset, 48);
	}
	else if(opcode == ISCSI_OPCODE_REJECT) {
	    /* Reject */
	    proto_tree_add_item(ti, hf_iscsi_Reject_Reason, tvb, offset + 2, 1, FALSE);
	    proto_tree_add_uint(ti, hf_iscsi_DataSegmentLength, tvb, offset + 5, 3, data_segment_len);
	    proto_tree_add_item(ti, hf_iscsi_StatSN, tvb, offset + 24, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_ExpCmdSN, tvb, offset + 28, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_MaxCmdSN, tvb, offset + 32, 4, FALSE);
	    proto_tree_add_item(ti, hf_iscsi_DataSN, tvb, offset + 36, 4, FALSE);
	    offset += 48 + handleHeaderDigest(ti, tvb, offset, 48);
	    if(end_offset > offset) {
		int error_pdu_len = iscsi_min(data_segment_len, end_offset - offset);
		if(error_pdu_len > 0) {
		    proto_tree_add_item(ti, hf_iscsi_error_pdu_data, tvb, offset, error_pdu_len, FALSE);
		    offset += error_pdu_len;
		}
		if(offset < end_offset && (offset & 3) != 0) {
		    int padding = 4 - (offset & 3);
		    proto_tree_add_item(ti, hf_iscsi_Padding, tvb, offset, padding, FALSE);
		    offset += padding;
		}
	    }
	}

	proto_item_set_len(ti, offset - original_offset);
    }
    else {
	/* FIXME - this really should gobble up digests and data segment */
	offset += 48;
    }

    return offset - original_offset;
}

static gboolean
dissect_iscsi(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree) {

    /* Set up structures needed to add the protocol subtree and manage it */
    guint offset = 0;
    guint32 available_bytes = tvb_length_remaining(tvb, offset);

    /* quick check to see if the packet is long enough to contain a
     * whole iSCSI header segment */
    if (available_bytes < 48) {
	/* no, so give up */
	return FALSE;
    }

    /* (potentially) process multiple iSCSI PDUs per packet */
    while(available_bytes >= 48) {
	const char *opcode_str = NULL;
	guint32 data_segment_len;
	guint8 opcode = tvb_get_guint8(tvb, offset + 0);

	if((opcode & TARGET_OPCODE_BIT) == 0) {
	    /* initiator -> target */
	    /* mask out X and I bits */
	    opcode &= ~(X_BIT | I_BIT);
	}
	opcode_str = match_strval(opcode, iscsi_opcodes);
	data_segment_len = tvb_get_ntohl(tvb, offset + 4) & 0x00ffffff;

	/* try and distinguish between data and real headers */
	if(opcode_str == NULL ||
	   (enable_bogosity_filter &&
	    (data_segment_len > bogus_pdu_data_length_threshold ||
	     available_bytes > (data_segment_len + 48 + bogus_pdu_max_digest_padding)))) {
#if 1
	    /* scanning a packet for a valid header is very slow so
	     * for the moment we just give up */
	    return FALSE;
#else
	    /* see if the next word starts a header */
	    int inc = 4;
	    offset += inc;
	    available_bytes -= inc;
#endif
	}
	else {
	    dissect_iscsi_pdu(tvb, pinfo, tree, offset, opcode, opcode_str, data_segment_len);
	    return TRUE;
	}
    }

    return FALSE;
}


/* Register the protocol with Ethereal */

/* this format is require because a script is used to build the C function
   that calls all the protocol registration.
*/

void
proto_register_iscsi(void)
{                 

	/* Setup list of header fields  See Section 1.6.1 for details*/
    static hf_register_info hf[] = {
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
	{ &hf_iscsi_sense_data,
	  { "SenseData", "iscsi.sensedata",
	    FT_BYTES, BASE_HEX, NULL, 0,
	    "Sense Data", HFILL }
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
	{ &hf_iscsi_HeaderDigest32,
	  { "HeaderDigest", "iscsi.headerdigest",
	    FT_UINT32, BASE_HEX, NULL, 0,
	    "Header Digest", HFILL }
	},
	{ &hf_iscsi_Opcode,
	  { "Opcode", "iscsi.opcode",
	    FT_UINT8, BASE_HEX, VALS(iscsi_opcodes), 0,          
	    "Opcode", HFILL }
	},
	{ &hf_iscsi_X,
	  { "X", "iscsi.X",
	    FT_BOOLEAN, 8, TFS(&iscsi_meaning_X), 0x80,          
	    "Command Retry", HFILL }
	},
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
	    "Sequence number for this command (0 == immediate)", HFILL }
	},
	{ &hf_iscsi_ExpStatSN,
	  { "ExpStatSN", "iscsi.expstatsn",
	    FT_UINT32, BASE_HEX, NULL, 0,
	    "Next expected status sequence number", HFILL }
	},
	{ &hf_iscsi_SCSICommand_CDB,
	  { "CDB", "iscsi.scsicommand.cdb",
	    FT_BYTES, BASE_HEX, NULL, 0,
	    "SCSI CDB", HFILL }
	},
	{ &hf_iscsi_SCSICommand_CDB0,
	  { "CDB", "iscsi.scsicommand.cdb0",
	    FT_UINT8, BASE_HEX, VALS(iscsi_scsi_cdb0), 0,
	    "SCSI CDB[0]", HFILL }
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
	    FT_UINT8, BASE_HEX, VALS(iscsi_scsi_statuses), 0,
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
	{ &hf_iscsi_SCSIData_F,
	  { "F", "iscsi.scsidata.F",
	    FT_BOOLEAN, 8, TFS(&iscsi_meaning_F), 0x80,          
	    "Final PDU", HFILL }
	},
	{ &hf_iscsi_SCSIData_S,
	  { "S", "iscsi.scsidata.S",
	    FT_BOOLEAN, 8, TFS(&iscsi_meaning_S), 0x01,          
	    "PDU Contains SCSI command status", HFILL }
	},
	{ &hf_iscsi_SCSIData_U,
	  { "U", "iscsi.scsidata.U",
	    FT_BOOLEAN, 8,  TFS(&iscsi_meaning_U), 0x02,          
	    "Residual underflow", HFILL }
	},
	{ &hf_iscsi_SCSIData_O,
	  { "O", "iscsi.scsidata.O",
	    FT_BOOLEAN, 8,  TFS(&iscsi_meaning_O), 0x04,          
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
	{ &hf_iscsi_ISID,
	  { "ISID", "iscsi.isid",
	    FT_UINT16, BASE_HEX, NULL, 0,
	    "Initiator part of session identifier", HFILL }
	},
	{ &hf_iscsi_TSID,
	  { "TSID", "iscsi.tsid",
	    FT_UINT16, BASE_HEX, NULL, 0,
	    "Target part of session identifier", HFILL }
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
	{ &hf_iscsi_Login_CSG,
	  { "CSG", "iscsi.login.csg",
	    FT_UINT8, BASE_HEX, VALS(iscsi_login_stage), 0x0c,          
	    "Current stage",  HFILL }
	},
	{ &hf_iscsi_Login_NSG,
	  { "NSG", "iscsi.login.nsg",
	    FT_UINT8, BASE_HEX, VALS(iscsi_login_stage), 0x03,          
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
	{ &hf_iscsi_SCSITask_Response,
	  { "Response", "iscsi.scsitask.response",
	    FT_UINT8, BASE_HEX, VALS(iscsi_task_responses), 0,
	    "Response", HFILL }
	},
	{ &hf_iscsi_SCSITask_ReferencedTaskTag,
	  { "InitiatorTaskTag", "iscsi.scsitask.referencedtasktag",
	    FT_UINT32, BASE_HEX, NULL, 0,
	    "Task's initiator task tag", HFILL }
	},
	{ &hf_iscsi_RefCmdSN,
	  { "RefCmdSN", "iscsi.refcmdsn",
	    FT_UINT32, BASE_HEX, NULL, 0,
	    "Command sequence number for command to be aborted", HFILL }
	},
	{ &hf_iscsi_SCSITask_Function,
	  { "Function", "iscsi.scsitask.function",
	    FT_UINT8, BASE_HEX, VALS(iscsi_task_functions), 0x7F,
	    "Requested task function", HFILL }
	},
	{ &hf_iscsi_Logout_Reason,
	  { "Reason", "iscsi.logout.reason",
	    FT_UINT8, BASE_HEX, VALS(iscsi_logout_reasons), 0,
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
	&ett_iscsi_KeyValues,
	&ett_iscsi_CDB,
	&ett_iscsi_Flags,
    };

    /* Register the protocol name and description */
    proto_iscsi = proto_register_protocol("iSCSI", "ISCSI", "iscsi");

    /* Required function calls to register the header fields and subtrees used */
    proto_register_field_array(proto_iscsi, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));

    {
	module_t *iscsi_module = prefs_register_protocol(proto_iscsi, NULL);

	prefs_register_bool_preference(iscsi_module,
				       "bogus_pdu_filter", 
				       "Enable bogus pdu filter",
				       "When enabled, packets that appear bogus are ignored",
				       &enable_bogosity_filter);

	prefs_register_uint_preference(iscsi_module,
				       "bogus_pdu_max_data_len", 
				       "Bogus pdu max data length threshold",
				       "Treat packets whose data segment length is greater than this value as bogus",
				       10,
				       &bogus_pdu_data_length_threshold);
	prefs_register_uint_preference(iscsi_module,
				       "bogus_pdu_max_digest_padding", 
				       "Bogus pdu max digest padding",
				       "Treat packets whose apparent total digest size is greater than this value as bogus",
				       10,
				       &bogus_pdu_max_digest_padding);
    }
}


/* If this dissector uses sub-dissector registration add a registration routine.
   This format is required because a script is used to find these routines and
   create the code that calls these routines.
*/
void
proto_reg_handoff_iscsi(void)
{
    heur_dissector_add("tcp", dissect_iscsi, proto_iscsi);
}
