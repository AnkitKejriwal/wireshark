/* packet-cops.c
 * Routines for the COPS (Common Open Policy Service) protocol dissection
 * RFC2748 & COPS-PR extension RFC3084
 *
 * Copyright 2000, Heikki Vatiainen <hessu@cs.tut.fi>
 *
 * $Id: packet-cops.c,v 1.34 2003/09/08 19:40:10 guy Exp $
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <glib.h>
#include <epan/packet.h>
#include "packet-ipv6.h"
#include "packet-tcp.h"

#ifdef HAVE_SOME_SNMP
#ifdef HAVE_NET_SNMP
# include <net-snmp/net-snmp-config.h>
# include <net-snmp/mib_api.h>
# include <net-snmp/library/default_store.h>
# include <net-snmp/config_api.h>
#else /* HAVE_NET_SNMP */
# include <ucd-snmp/ucd-snmp-config.h>
# include <ucd-snmp/asn1.h>
# include <ucd-snmp/snmp_api.h>
# include <ucd-snmp/snmp_impl.h>
# include <ucd-snmp/mib.h>
# include <ucd-snmp/default_store.h>
# include <ucd-snmp/read_config.h>
# include <ucd-snmp/tools.h>
#endif /* HAVE_NET_SNMP */
#endif /* HAVE_SOME_SNMP */

#include "asn1.h"
#include "format-oid.h"
#include "prefs.h"

#define TCP_PORT_COPS 3288

/* Preference: Variable to hold the tcp port preference */
static guint global_cops_tcp_port = TCP_PORT_COPS;

/* Preference: desegmentation of COPS */
static gboolean cops_desegment = TRUE;

/* Variable to allow for proper deletion of dissector registration
 * when the user changes port from the gui
 */

static guint cops_tcp_port = 0;

/*Some local globals needed to read COPS-PR ASN.1 Types from PIB-MIBs */
/*MAX_OID_LEN from NET-SNMP's asn1.h*/

#ifdef HAVE_NET_SNMP
static  subid_t last_decoded_prid_oid[MAX_OID_LEN]={0};
static  subid_t last_decoded_prid_oid_length=0;
extern struct tree *tree_head;

/* Preference: COPS-PR ASN.1 type decoding based on PIB/MIB or data in packet */
static gboolean cops_typefrommib = FALSE;

#endif /* HAVE_NET_SNMP */

#define COPS_OBJECT_HDR_SIZE 4

/* Null string of type "guchar[]". */
static const guchar nullstring[] = "";

#define	SAFE_STRING(s)	(((s) != NULL) ? (s) : nullstring)

/* COPS PR Tags */

#define COPS_IPA    0		/* IP Address */
#define COPS_U32    2		/* Unsigned 32*/
#define COPS_TIT    3		/* TimeTicks */
#define COPS_OPQ    4		/* Opaque */
#define COPS_I64    10		/* Integer64 */
#define COPS_U64    11		/* Uinteger64 */

/* COPS PR Types */

#define COPS_NULL                0
#define COPS_INTEGER             1    /* l  */
#define COPS_OCTETSTR            2    /* c  */
#define COPS_OBJECTID            3    /* ul */
#define COPS_IPADDR              4    /* uc */
#define COPS_UNSIGNED32          5    /* ul */
#define COPS_TIMETICKS           7    /* ul */
#define COPS_OPAQUE              8    /* c  */
#define COPS_INTEGER64           10   /* ll */
#define COPS_UNSIGNED64          11   /* ull  */


typedef struct _COPS_CNV COPS_CNV;

struct _COPS_CNV
{
  guint class;
  guint tag;
  gint  syntax;
  gchar *name;
};

static COPS_CNV CopsCnv [] =
{
  {ASN1_UNI, ASN1_NUL, COPS_NULL,      "NULL"},
  {ASN1_UNI, ASN1_INT, COPS_INTEGER,   "INTEGER"},
  {ASN1_UNI, ASN1_OTS, COPS_OCTETSTR,  "OCTET STRING"},
  {ASN1_UNI, ASN1_OJI, COPS_OBJECTID,  "OBJECTID"},
  {ASN1_APL, COPS_IPA, COPS_IPADDR,    "IPADDR"},
  {ASN1_APL, COPS_U32, COPS_UNSIGNED32,"UNSIGNED32"},
  {ASN1_APL, COPS_TIT, COPS_TIMETICKS, "TIMETICKS"},
  {ASN1_APL, COPS_OPQ, COPS_OPAQUE,    "OPAQUE"},
  {ASN1_APL, COPS_I64, COPS_INTEGER64, "INTEGER64"},
  {ASN1_APL, COPS_U64, COPS_UNSIGNED64, "UNSIGNED64"},
  {0,       0,         -1,                  NULL}
};

static gchar *
cops_tag_cls2syntax ( guint tag, guint cls, gushort *syntax)
{
  COPS_CNV *cnv;

  cnv = CopsCnv;
  while (cnv->syntax != -1)
  {
    if (cnv->tag == tag && cnv->class == cls)
    {
      *syntax = cnv->syntax;
      return cnv->name;
    }
    cnv++;
  }
  return NULL;
}

static const value_string cops_flags_vals[] = {
  { 0x00,          "None" },
  { 0x01,          "Solicited Message Flag Bit" },
  { 0, NULL },
};

/* The different COPS message types */
enum cops_op_code {
  COPS_NO_MSG,          /* Not a COPS Message type     */

  COPS_MSG_REQ,         /* Request (REQ)               */
  COPS_MSG_DEC,         /* Decision (DEC)              */
  COPS_MSG_RPT,         /* Report State (RPT)          */
  COPS_MSG_DRQ,         /* Delete Request State (DRQ)  */
  COPS_MSG_SSQ,         /* Synchronize State Req (SSQ) */
  COPS_MSG_OPN,         /* Client-Open (OPN)           */
  COPS_MSG_CAT,         /* Client-Accept (CAT)         */
  COPS_MSG_CC,          /* Client-Close (CC)           */
  COPS_MSG_KA,          /* Keep-Alive (KA)             */
  COPS_MSG_SSC,         /* Synchronize Complete (SSC)  */

  COPS_LAST_OP_CODE     /* For error checking          */
};

static const value_string cops_op_code_vals[] = {
  { COPS_MSG_REQ,          "Request (REQ)" },
  { COPS_MSG_DEC,          "Decision (DEC)" },
  { COPS_MSG_RPT,          "Report State (RPT)" },
  { COPS_MSG_DRQ,          "Delete Request State (DRQ)" },
  { COPS_MSG_SSQ,          "Synchronize State Req (SSQ)" },
  { COPS_MSG_OPN,          "Client-Open (OPN)" },
  { COPS_MSG_CAT,          "Client-Accept (CAT)" },
  { COPS_MSG_CC,           "Client-Close (CC)" },
  { COPS_MSG_KA,           "Keep-Alive (KA)" },
  { COPS_MSG_SSC,          "Synchronize Complete (SSC)" },
  { 0, NULL },
};


/* The different objects in COPS messages */
enum cops_c_num {
  COPS_NO_OBJECT,        /* Not a COPS Object type               */

  COPS_OBJ_HANDLE,       /* Handle Object (Handle)               */
  COPS_OBJ_CONTEXT,      /* Context Object (Context)             */
  COPS_OBJ_IN_INT,       /* In-Interface Object (IN-Int)         */
  COPS_OBJ_OUT_INT,      /* Out-Interface Object (OUT-Int)       */
  COPS_OBJ_REASON,       /* Reason Object (Reason)               */
  COPS_OBJ_DECISION,     /* Decision Object (Decision)           */
  COPS_OBJ_LPDPDECISION, /* LPDP Decision Object (LPDPDecision)  */
  COPS_OBJ_ERROR,        /* Error Object (Error)                 */
  COPS_OBJ_CLIENTSI,     /* Client Specific Information Object (ClientSI) */
  COPS_OBJ_KATIMER,      /* Keep-Alive Timer Object (KATimer)    */
  COPS_OBJ_PEPID,        /* PEP Identification Object (PEPID)    */
  COPS_OBJ_REPORT_TYPE,  /* Report-Type Object (Report-Type)     */
  COPS_OBJ_PDPREDIRADDR, /* PDP Redirect Address Object (PDPRedirAddr) */
  COPS_OBJ_LASTPDPADDR,  /* Last PDP Address (LastPDPaddr)       */
  COPS_OBJ_ACCTTIMER,    /* Accounting Timer Object (AcctTimer)  */
  COPS_OBJ_INTEGRITY,    /* Message Integrity Object (Integrity) */
  COPS_LAST_C_NUM        /* For error checking                   */
};

static const value_string cops_c_num_vals[] = {
  { COPS_OBJ_HANDLE,       "Handle Object (Handle)" },
  { COPS_OBJ_CONTEXT,      "Context Object (Context)" },
  { COPS_OBJ_IN_INT,       "In-Interface Object (IN-Int)" },
  { COPS_OBJ_OUT_INT,      "Out-Interface Object (OUT-Int)" },
  { COPS_OBJ_REASON,       "Reason Object (Reason)" },
  { COPS_OBJ_DECISION,     "Decision Object (Decision)" },
  { COPS_OBJ_LPDPDECISION, "LPDP Decision Object (LPDPDecision)" },
  { COPS_OBJ_ERROR,        "Error Object (Error)" },
  { COPS_OBJ_CLIENTSI,     "Client Specific Information Object (ClientSI)" },
  { COPS_OBJ_KATIMER,      "Keep-Alive Timer Object (KATimer)" },
  { COPS_OBJ_PEPID,        "PEP Identification Object (PEPID)" },
  { COPS_OBJ_REPORT_TYPE,  "Report-Type Object (Report-Type)" },
  { COPS_OBJ_PDPREDIRADDR, "PDP Redirect Address Object (PDPRedirAddr)" },
  { COPS_OBJ_LASTPDPADDR,  "Last PDP Address (LastPDPaddr)" },
  { COPS_OBJ_ACCTTIMER,    "Accounting Timer Object (AcctTimer)" },
  { COPS_OBJ_INTEGRITY,    "Message Integrity Object (Integrity)" },
  { 0, NULL },
};


/* The different objects in COPS-PR messages */
enum cops_s_num {
  COPS_NO_PR_OBJECT,     /* Not a COPS-PR Object type               */
  COPS_OBJ_PRID,         /* Provisioning Instance Identifier (PRID) */
  COPS_OBJ_PPRID,        /* Prefix Provisioning Instance Identifier (PPRID) */
  COPS_OBJ_EPD,          /* Encoded Provisioning Instance Data (EPD) */
  COPS_OBJ_GPERR,        /* Global Provisioning Error Object (GPERR) */
  COPS_OBJ_CPERR,        /* PRC Class Provisioning Error Object (CPERR) */
  COPS_OBJ_ERRPRID,      /* Error Provisioning Instance Identifier (ErrorPRID)*/

  COPS_LAST_S_NUM        /* For error checking                   */
};


static const value_string cops_s_num_vals[] = {
  { COPS_OBJ_PRID,         "Provisioning Instance Identifier (PRID)" },
  { COPS_OBJ_PPRID,        "Prefix Provisioning Instance Identifier (PPRID)" },
  { COPS_OBJ_EPD,          "Encoded Provisioning Instance Data (EPD)" },
  { COPS_OBJ_GPERR,        "Global Provisioning Error Object (GPERR)" },
  { COPS_OBJ_CPERR,        "PRC Class Provisioning Error Object (CPERR)" },
  { COPS_OBJ_ERRPRID,      "Error Provisioning Instance Identifier (ErrorPRID)" },
  { 0, NULL },

};

/* R-Type is carried within the Context Object */
static const value_string cops_r_type_vals[] = {
  { 0x01, "Incoming-Message/Admission Control request" },
  { 0x02, "Resource-Allocation request" },
  { 0x04, "Outgoing-Message request" },
  { 0x08, "Configuration request" },
  { 0, NULL },
};
/* S-Type is carried within the ClientSI Object for COPS-PR*/
static const value_string cops_s_type_vals[] = {
  { 0x01, "BER" },
  { 0, NULL },
};

/* Reason-Code is carried within the Reason object */
static const value_string cops_reason_vals[] = {
  { 1,  "Unspecified" },
  { 2,  "Management" },
  { 3,  "Preempted (Another request state takes precedence)" },
  { 4,  "Tear (Used to communicate a signaled state removal)" },
  { 5,  "Timeout (Local state has timed-out)" },
  { 6,  "Route Change (Change invalidates request state)" },
  { 7,  "Insufficient Resources (No local resource available)" },
  { 8,  "PDP's Directive (PDP decision caused the delete)" },
  { 9,  "Unsupported decision (PDP decision not supported)" },
  { 10, "Synchronize Handle Unknown" },
  { 11, "Transient Handle (stateless event)" },
  { 12, "Malformed Decision (could not recover)" },
  { 13, "Unknown COPS Object from PDP" },
  { 0, NULL },
};

/* Command-Code is carried within the Decision object if C-Type is 1 */
static const value_string cops_dec_cmd_code_vals[] = {
  { 0, "NULL Decision (No configuration data available)" },
  { 1, "Install (Admit request/Install configuration)" },
  { 2, "Remove (Remove request/Remove configuration)" },
  { 0, NULL },
};

/* Decision flags are also carried with the Decision object if C-Type is 1 */
static const value_string cops_dec_cmd_flag_vals[] = {
  { 0x00, "<None set>" },
  { 0x01, "Trigger Error (Trigger error message if set)" },
  { 0, NULL },
};

/* Error-Code from Error object */
static const value_string cops_error_vals[] = {
  {1,  "Bad handle" },
  {2,  "Invalid handle reference" },
  {3,  "Bad message format (Malformed Message)" },
  {4,  "Unable to process (server gives up on query)" },
  {5,  "Mandatory client-specific info missing" },
  {6,  "Unsupported client" },
  {7,  "Mandatory COPS object missing" },
  {8,  "Client Failure" },
  {9,  "Communication Failure" },
  {10, "Unspecified" },
  {11, "Shutting down" },
  {12, "Redirect to Preferred Server" },
  {13, "Unknown COPS Object" },
  {14, "Authentication Failure" },
  {15, "Authentication Required" },
  {0,  NULL },
};
/* Error-Code from GPERR object */
static const value_string cops_gperror_vals[] = {
  {1,  "AvailMemLow" },
  {2,  "AvailMemExhausted" },
  {3,  "unknownASN.1Tag" },
  {4,  "maxMsgSizeExceeded" },
  {5,  "unknownError" },
  {6,  "maxRequestStatesOpen" },
  {7,  "invalidASN.1Length" },
  {8,  "invalidObjectPad" },
  {9,  "unknownPIBData" },
  {10, "unknownCOPSPRObject" },
  {11, "malformedDecision" },
  {0,  NULL },
};

/* Error-Code from CPERR object */
static const value_string cops_cperror_vals[] = {
  {1,  "priSpaceExhausted" },
  {2,  "priInstanceInvalid" },
  {3,  "attrValueInvalid" },
  {4,  "attrValueSupLimited" },
  {5,  "attrEnumSupLimited" },
  {6,  "attrMaxLengthExceeded" },
  {7,  "attrReferenceUnknown" },
  {8,  "priNotifyOnly" },
  {9,  "unknownPrc" },
  {10, "tooFewAttrs" },
  {11, "invalidAttrType" },
  {12, "deletedInRef" },
  {13, "priSpecificError" },
 	{0,  NULL },
};


/* Report-Type from Report-Type object */
static const value_string cops_report_type_vals[] = {
  {1, " Success   : Decision was successful at the PEP" },
  {2, " Failure   : Decision could not be completed by PEP" },
  {3, " Accounting: Accounting update for an installed state" },
  {0, NULL },
};

/* Initialize the protocol and registered fields */
static gint proto_cops = -1;
static gint hf_cops_ver_flags = -1;
static gint hf_cops_version = -1;
static gint hf_cops_flags = -1;

static gint hf_cops_op_code = -1;
static gint hf_cops_client_type = -1;
static gint hf_cops_msg_len = -1;

static gint hf_cops_obj_len = -1;
static gint hf_cops_obj_c_num = -1;
static gint hf_cops_obj_c_type = -1;

static gint hf_cops_obj_s_num = -1;
static gint hf_cops_obj_s_type = -1;

static gint hf_cops_r_type_flags = -1;
static gint hf_cops_m_type_flags = -1;

static gint hf_cops_in_int_ipv4 = -1;
static gint hf_cops_in_int_ipv6 = -1;
static gint hf_cops_out_int_ipv4 = -1;
static gint hf_cops_out_int_ipv6 = -1;
static gint hf_cops_int_ifindex = -1;

static gint hf_cops_reason = -1;
static gint hf_cops_reason_sub = -1;

static gint hf_cops_dec_cmd_code = -1;
static gint hf_cops_dec_flags = -1;

static gint hf_cops_error = -1;
static gint hf_cops_error_sub = -1;

static gint hf_cops_gperror = -1;
static gint hf_cops_gperror_sub = -1;

static gint hf_cops_cperror = -1;
static gint hf_cops_cperror_sub = -1;

static gint hf_cops_katimer = -1;

static gint hf_cops_pepid = -1;

static gint hf_cops_report_type = -1;

static gint hf_cops_pdprediraddr_ipv4 = -1;
static gint hf_cops_pdprediraddr_ipv6 = -1;
static gint hf_cops_lastpdpaddr_ipv4 = -1;
static gint hf_cops_lastpdpaddr_ipv6 = -1;
static gint hf_cops_pdp_tcp_port = -1;

static gint hf_cops_accttimer = -1;

static gint hf_cops_key_id = -1;
static gint hf_cops_seq_num = -1;

/* Initialize the subtree pointers */
static gint ett_cops = -1;
static gint ett_cops_ver_flags = -1;
static gint ett_cops_obj = -1;
static gint ett_cops_pr_obj = -1;
static gint ett_cops_obj_data = -1;
static gint ett_cops_r_type_flags = -1;
static gint ett_cops_itf = -1;
static gint ett_cops_reason = -1;
static gint ett_cops_decision = -1;
static gint ett_cops_error = -1;
static gint ett_cops_clientsi = -1;
static gint ett_cops_asn1 = -1;
static gint ett_cops_gperror = -1;
static gint ett_cops_cperror = -1;
static gint ett_cops_pdp = -1;

void proto_reg_handoff_cops(void);

static guint get_cops_pdu_len(tvbuff_t *tvb, int offset);
static void dissect_cops_pdu(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);

static int dissect_cops_object(tvbuff_t *tvb, guint32 offset, proto_tree *tree);
static void dissect_cops_object_data(tvbuff_t *tvb, guint32 offset, proto_tree *tree,
                                     guint8 c_num, guint8 c_type, guint16 len);

static void dissect_cops_pr_objects(tvbuff_t *tvb, guint32 offset, proto_tree *tree, guint16 pr_len);
static int dissect_cops_pr_object_data(tvbuff_t *tvb, guint32 offset, proto_tree *tree,
                                       guint8 s_num, guint8 s_type, guint16 len);

/* Code to actually dissect the packets */
static void
dissect_cops(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
  tcp_dissect_pdus(tvb, pinfo, tree, cops_desegment, 8,
                   get_cops_pdu_len, dissect_cops_pdu);
}

static guint
get_cops_pdu_len(tvbuff_t *tvb, int offset)
{
  /*
   * Get the length of the COPS message.
   */
  return tvb_get_ntohl(tvb, offset + 4);
}

static void
dissect_cops_pdu(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
  guint8 op_code;
  int object_len;

  if (check_col(pinfo->cinfo, COL_PROTOCOL))
    col_set_str(pinfo->cinfo, COL_PROTOCOL, "COPS");
  if (check_col(pinfo->cinfo, COL_INFO))
    col_clear(pinfo->cinfo, COL_INFO);

  op_code = tvb_get_guint8(tvb, 1);
  if (check_col(pinfo->cinfo, COL_INFO))
    col_add_fstr(pinfo->cinfo, COL_INFO, "COPS %s",
                 val_to_str(op_code, cops_op_code_vals, "Unknown Op Code"));

  if (tree) {
    proto_item *ti, *tv;
    proto_tree *cops_tree, *ver_flags_tree;
    guint32 msg_len;
    guint32 offset = 0;
    guint8 ver_flags;
    gint garbage;

    ti = proto_tree_add_item(tree, proto_cops, tvb, offset, -1, FALSE);
    cops_tree = proto_item_add_subtree(ti, ett_cops);

    /* Version and flags share the same byte, put them in a subtree */
    ver_flags = tvb_get_guint8(tvb, offset);
    tv = proto_tree_add_uint_format(cops_tree, hf_cops_ver_flags, tvb, offset, 1,
                                      ver_flags, "Version: %u, Flags: %s",
                                      hi_nibble(ver_flags),
                                      val_to_str(lo_nibble(ver_flags), cops_flags_vals, "Unknown"));
    ver_flags_tree = proto_item_add_subtree(tv, ett_cops_ver_flags);
    proto_tree_add_uint(ver_flags_tree, hf_cops_version, tvb, offset, 1, ver_flags);
    proto_tree_add_uint(ver_flags_tree, hf_cops_flags, tvb, offset, 1, ver_flags);
    offset++;

    proto_tree_add_item(cops_tree, hf_cops_op_code, tvb, offset, 1, FALSE);
    offset ++;
    proto_tree_add_item(cops_tree, hf_cops_client_type, tvb, offset, 2, FALSE);
    offset += 2;

    msg_len = tvb_get_ntohl(tvb, offset);
    proto_tree_add_uint(cops_tree, hf_cops_msg_len, tvb, offset, 4, msg_len);
    offset += 4;

    while (tvb_reported_length_remaining(tvb, offset) >= COPS_OBJECT_HDR_SIZE) {
      object_len = dissect_cops_object(tvb, offset, cops_tree);
      if (object_len < 0)
        return;
      offset += object_len;
    }

    garbage = tvb_length_remaining(tvb, offset);
    if (garbage > 0)
      proto_tree_add_text(cops_tree, tvb, offset, garbage,
                          "Trailing garbage: %d byte%s", garbage,
                          plurality(garbage, "", "s"));
  }
}

static char *cops_c_type_to_str(guint8 c_num, guint8 c_type)
{
  switch (c_num) {
  case COPS_OBJ_HANDLE:
    if (c_type == 1)
      return "Client Handle";
    break;
  case COPS_OBJ_IN_INT:
  case COPS_OBJ_OUT_INT:
    if (c_type == 1)
      return "IPv4 Address + Interface";
    else if (c_type == 2)
      return "IPv6 Address + Interface";
    break;
  case COPS_OBJ_DECISION:
  case COPS_OBJ_LPDPDECISION:
    if (c_type == 1)
      return "Decision Flags (Mandatory)";
    else if (c_type == 2)
      return "Stateless Data";
    else if (c_type == 3)
      return "Replacement Data";
    else if (c_type == 4)
      return "Client Specific Decision Data";
    else if (c_type == 5)
      return "Named Decision Data";
    break;
  case COPS_OBJ_CLIENTSI:
    if (c_type == 1)
      return "Signaled ClientSI";
    else if (c_type == 2)
      return "Named ClientSI";
    break;
  case COPS_OBJ_KATIMER:
    if (c_type == 1)
      return "Keep-alive timer value";
    break;
  case COPS_OBJ_PDPREDIRADDR:
  case COPS_OBJ_LASTPDPADDR:
    if (c_type == 1)
      return "IPv4 Address + TCP Port";
    else if (c_type == 2)
      return "IPv6 Address + TCP Port";
    break;
  case COPS_OBJ_ACCTTIMER:
    if (c_type == 1)
      return "Accounting timer value";
    break;
  case COPS_OBJ_INTEGRITY:
    if (c_type == 1)
      return "HMAC digest";
    break;
  }

  return "";
}

static int dissect_cops_object(tvbuff_t *tvb, guint32 offset, proto_tree *tree)
{
  int object_len, contents_len;
  guint8 c_num, c_type;
  proto_item *ti;
  proto_tree *obj_tree;
  char *type_str;

  object_len = tvb_get_ntohs(tvb, offset);
  if (object_len < COPS_OBJECT_HDR_SIZE) {
    /* Bogus! */
    proto_tree_add_text(tree, tvb, offset, 2,
                        "Bad COPS object length: %u, should be at least %u\n",
                        object_len, COPS_OBJECT_HDR_SIZE);
    return -1;
  }
  c_num = tvb_get_guint8(tvb, offset + 2);
  c_type = tvb_get_guint8(tvb, offset + 3);

  ti = proto_tree_add_uint_format(tree, hf_cops_obj_c_num, tvb, offset, object_len, c_num,
                                  "%s: %s", val_to_str(c_num, cops_c_num_vals, "Unknown"),
                                  cops_c_type_to_str(c_num, c_type));
  obj_tree = proto_item_add_subtree(ti, ett_cops_obj);

  proto_tree_add_uint(obj_tree, hf_cops_obj_len, tvb, offset, 2, object_len);
  offset += 2;

  proto_tree_add_uint(obj_tree, hf_cops_obj_c_num, tvb, offset, 1, c_num);
  offset++;

  type_str = cops_c_type_to_str(c_num, c_type);
  proto_tree_add_text(obj_tree, tvb, offset, 1, "C-Type: %s%s%u%s",
                      type_str,
                      strlen(type_str) ? " (" : "",
                      c_type,
                      strlen(type_str) ? ")" : "");
  offset++;

  contents_len = object_len - COPS_OBJECT_HDR_SIZE;
  dissect_cops_object_data(tvb, offset, obj_tree, c_num, c_type, contents_len);

  /* Pad to 32bit boundary */
  if (object_len % sizeof (guint32))
    object_len += (sizeof (guint32) - object_len % sizeof (guint32));

  return object_len;
}

static void dissect_cops_pr_objects(tvbuff_t *tvb, guint32 offset, proto_tree *tree, guint16 pr_len)
{
  int object_len, contents_len;
  guint8 s_num, s_type;
  char *type_str;
  int ret;
  proto_tree *cops_pr_tree, *obj_tree;
  proto_item *ti;

  cops_pr_tree = proto_item_add_subtree(tree, ett_cops_pr_obj);

  while (pr_len >= COPS_OBJECT_HDR_SIZE) {
    object_len = tvb_get_ntohs(tvb, offset);
    if (object_len < COPS_OBJECT_HDR_SIZE) {
      /* Bogus! */
      proto_tree_add_text(tree, tvb, offset, 2,
                          "Bad COPS PR object length: %u, should be at least %u\n",
                          object_len, COPS_OBJECT_HDR_SIZE);
      return;
    }
    s_num = tvb_get_guint8(tvb, offset + 2);

    ti = proto_tree_add_uint_format(cops_pr_tree, hf_cops_obj_s_num, tvb, offset, object_len, s_num,
                                    "%s", val_to_str(s_num, cops_s_num_vals, "Unknown"));
    obj_tree = proto_item_add_subtree(cops_pr_tree, ett_cops_pr_obj);

    proto_tree_add_uint(obj_tree, hf_cops_obj_len, tvb, offset, 2, object_len);
    offset += 2;
    pr_len -= 2;

    proto_tree_add_uint(obj_tree, hf_cops_obj_s_num, tvb, offset, 1, s_num);
    offset++;
    pr_len--;

    s_type = tvb_get_guint8(tvb, offset);
    type_str = val_to_str(s_type, cops_s_type_vals, "Unknown");
    proto_tree_add_text(obj_tree, tvb, offset, 1, "S-Type: %s%s%u%s",
                        type_str,
                        strlen(type_str) ? " (" : "",
                        s_type,
                        strlen(type_str) ? ")" : "");
    offset++;
    pr_len--;

    contents_len = object_len - COPS_OBJECT_HDR_SIZE;
    ret = dissect_cops_pr_object_data(tvb, offset, obj_tree, s_num, s_type, contents_len);
    if (ret < 0)
      break;

    /*Pad to 32bit boundary */
    if (object_len % sizeof (guint32))
      object_len += (sizeof (guint32) - object_len % sizeof (guint32));

    pr_len -= object_len - COPS_OBJECT_HDR_SIZE;
    offset += object_len - COPS_OBJECT_HDR_SIZE;
  }
}

static void dissect_cops_object_data(tvbuff_t *tvb, guint32 offset, proto_tree *tree,
                                     guint8 c_num, guint8 c_type, guint16 len)
{
  proto_item *ti;
  proto_tree *r_type_tree, *itf_tree, *reason_tree, *dec_tree, *error_tree, *clientsi_tree, *pdp_tree;
  guint16 r_type, m_type, reason, reason_sub, cmd_code, cmd_flags, error, error_sub, tcp_port;
  guint32 ipv4addr, ifindex;
  struct e_in6_addr ipv6addr;

  switch (c_num) {
  case COPS_OBJ_CONTEXT:
    r_type = tvb_get_ntohs(tvb, offset);
    m_type = tvb_get_ntohs(tvb, offset + 2);
    ti = proto_tree_add_text(tree, tvb, offset, 4, "Contents: R-Type: %s, M-Type: %u",
                             val_to_str(r_type, cops_r_type_vals, "Unknown"),
                             m_type);

    r_type_tree = proto_item_add_subtree(ti, ett_cops_r_type_flags);
    proto_tree_add_uint(r_type_tree, hf_cops_r_type_flags, tvb, offset, 2, r_type);
    offset += 2;
    proto_tree_add_uint(r_type_tree, hf_cops_m_type_flags, tvb, offset, 2, m_type);

    break;
  case COPS_OBJ_IN_INT:
  case COPS_OBJ_OUT_INT:
    if (c_type == 1) {          /* IPv4 */
      tvb_memcpy(tvb, (guint8 *)&ipv4addr, offset, 4);
      ifindex = tvb_get_ntohl(tvb, offset + 4);
      ti = proto_tree_add_text(tree, tvb, offset, 8, "Contents: IPv4 address %s, ifIndex: %u",
                               ip_to_str((guint8 *)&ipv4addr), ifindex);
      itf_tree = proto_item_add_subtree(ti, ett_cops_itf);
      proto_tree_add_ipv4(itf_tree,
                          (c_num == COPS_OBJ_IN_INT) ? hf_cops_in_int_ipv4 : hf_cops_out_int_ipv4,
                          tvb, offset, 4, ipv4addr);
      offset += 4;
    } else if (c_type == 2) {   /* IPv6 */
      tvb_memcpy(tvb, (guint8 *)&ipv6addr, offset, sizeof ipv6addr);
      ifindex = tvb_get_ntohl(tvb, offset + sizeof ipv6addr);
      ti = proto_tree_add_text(tree, tvb, offset, 20, "Contents: IPv6 address %s, ifIndex: %u",
                               ip6_to_str(&ipv6addr), ifindex);
      itf_tree = proto_item_add_subtree(ti, ett_cops_itf);
      proto_tree_add_ipv6(itf_tree,
                          (c_num == COPS_OBJ_IN_INT) ? hf_cops_in_int_ipv6 : hf_cops_out_int_ipv6,
                          tvb, offset, 16, (guint8 *)&ipv6addr);
      offset += 16;
    } else {
      break;
    }
    proto_tree_add_uint(itf_tree, hf_cops_int_ifindex, tvb, offset, 4, ifindex);

    break;
  case COPS_OBJ_REASON:
    reason = tvb_get_ntohs(tvb, offset);
    reason_sub = tvb_get_ntohs(tvb, offset + 2);
    ti = proto_tree_add_text(tree, tvb, offset, 4, "Contents: Reason-Code: %s, Reason Sub-code: 0x%04x",
                             val_to_str(reason, cops_reason_vals, "<Unknown value>"), reason_sub);
    reason_tree = proto_item_add_subtree(ti, ett_cops_reason);
    proto_tree_add_uint(reason_tree, hf_cops_reason, tvb, offset, 2, reason);
    offset += 2;
    if (reason == 13) {
      proto_tree_add_text(reason_tree, tvb, offset, 2, "Reason Sub-code: "
                          "Unknown object's C-Num %u, C-Type %u",
                          tvb_get_guint8(tvb, offset), tvb_get_guint8(tvb, offset + 1));
    } else
      proto_tree_add_uint(reason_tree, hf_cops_reason_sub, tvb, offset, 2, reason_sub);

    break;
  case COPS_OBJ_DECISION:
  case COPS_OBJ_LPDPDECISION:
    if (c_type == 1) {
      cmd_code = tvb_get_ntohs(tvb, offset);
      cmd_flags = tvb_get_ntohs(tvb, offset + 2);
      ti = proto_tree_add_text(tree, tvb, offset, 4, "Contents: Command-Code: %s, Flags: %s",
                               val_to_str(cmd_code, cops_dec_cmd_code_vals, "<Unknown value>"),
                               val_to_str(cmd_flags, cops_dec_cmd_flag_vals, "<Unknown flag>"));
      dec_tree = proto_item_add_subtree(ti, ett_cops_decision);
      proto_tree_add_uint(dec_tree, hf_cops_dec_cmd_code, tvb, offset, 2, cmd_code);
      offset += 2;
      proto_tree_add_uint(dec_tree, hf_cops_dec_flags, tvb, offset, 2, cmd_flags);
    } else if (c_type == 5) { /*COPS-PR Data*/
      ti = proto_tree_add_text(tree, tvb, offset, 4, "Contents: %u bytes", len);
      dec_tree = proto_item_add_subtree(ti, ett_cops_decision);
      dissect_cops_pr_objects(tvb, offset, dec_tree, len);
    }

    break;
  case COPS_OBJ_ERROR:
    if (c_type != 1)
      break;

    error = tvb_get_ntohs(tvb, offset);
    error_sub = tvb_get_ntohs(tvb, offset + 2);
    ti = proto_tree_add_text(tree, tvb, offset, 4, "Contents: Error-Code: %s, Error Sub-code: 0x%04x",
                             val_to_str(error, cops_error_vals, "<Unknown value>"), error_sub);
    error_tree = proto_item_add_subtree(ti, ett_cops_error);
    proto_tree_add_uint(error_tree, hf_cops_error, tvb, offset, 2, error);
    offset += 2;
    if (error == 13) {
      proto_tree_add_text(error_tree, tvb, offset, 2, "Error Sub-code: "
                          "Unknown object's C-Num %u, C-Type %u",
                          tvb_get_guint8(tvb, offset), tvb_get_guint8(tvb, offset + 1));
    } else
      proto_tree_add_uint(error_tree, hf_cops_error_sub, tvb, offset, 2, error_sub);

    break;
  case COPS_OBJ_CLIENTSI:

    if (c_type != 2) /*Not COPS-PR data*/
      break;

    ti = proto_tree_add_text(tree, tvb, offset, 4, "Contents: %u bytes", len);
    clientsi_tree = proto_item_add_subtree(ti, ett_cops_clientsi);

    dissect_cops_pr_objects(tvb, offset, clientsi_tree, len);

    break;
  case COPS_OBJ_KATIMER:
    if (c_type != 1)
      break;

    proto_tree_add_item(tree, hf_cops_katimer, tvb, offset + 2, 2, FALSE);
    if (tvb_get_ntohs(tvb, offset + 2) == 0)
      proto_tree_add_text(tree, tvb, offset, 0, "Value of zero implies infinity.");

    break;
  case COPS_OBJ_PEPID:
    if (c_type != 1)
      break;

    if (tvb_strnlen(tvb, offset, len) == -1)
      proto_tree_add_text(tree, tvb, offset, len, "<PEP Id is not a NUL terminated ASCII string>");
    else
      proto_tree_add_item(tree, hf_cops_pepid, tvb, offset,
                          tvb_strnlen(tvb, offset, len) + 1, FALSE);

    break;
  case COPS_OBJ_REPORT_TYPE:
    if (c_type != 1)
      break;

    proto_tree_add_item(tree, hf_cops_report_type, tvb, offset, 2, FALSE);

    break;
  case COPS_OBJ_PDPREDIRADDR:
  case COPS_OBJ_LASTPDPADDR:
    if (c_type == 1) {          /* IPv4 */
      tvb_memcpy(tvb, (guint8 *)&ipv4addr, offset, 4);
      tcp_port = tvb_get_ntohs(tvb, offset + 4 + 2);
      ti = proto_tree_add_text(tree, tvb, offset, 8, "Contents: IPv4 address %s, TCP Port Number: %u",
                               ip_to_str((guint8 *)&ipv4addr), tcp_port);
      pdp_tree = proto_item_add_subtree(ti, ett_cops_pdp);
      proto_tree_add_ipv4(pdp_tree,
                          (c_num == COPS_OBJ_PDPREDIRADDR) ? hf_cops_pdprediraddr_ipv4 : hf_cops_lastpdpaddr_ipv4,
                          tvb, offset, 4, ipv4addr);
      offset += 4;
    } else if (c_type == 2) {   /* IPv6 */
      tvb_memcpy(tvb, (guint8 *)&ipv6addr, offset, sizeof ipv6addr);
      tcp_port = tvb_get_ntohs(tvb, offset + sizeof ipv6addr + 2);
      ti = proto_tree_add_text(tree, tvb, offset, 20, "Contents: IPv6 address %s, TCP Port Number: %u",
                               ip6_to_str(&ipv6addr), tcp_port);
      pdp_tree = proto_item_add_subtree(ti, ett_cops_pdp);
      proto_tree_add_ipv6(pdp_tree,
                          (c_num == COPS_OBJ_PDPREDIRADDR) ? hf_cops_pdprediraddr_ipv6 : hf_cops_lastpdpaddr_ipv6,
                          tvb, offset, 16, (guint8 *)&ipv6addr);
      offset += 16;
    } else {
      break;
    }
    offset += 2;
    proto_tree_add_uint(pdp_tree, hf_cops_pdp_tcp_port, tvb, offset, 2, tcp_port);

    break;
  case COPS_OBJ_ACCTTIMER:
    if (c_type != 1)
      break;

    proto_tree_add_item(tree, hf_cops_accttimer, tvb, offset + 2, 2, FALSE);
    if (tvb_get_ntohs(tvb, offset + 2) == 0)
      proto_tree_add_text(tree, tvb, offset, 0, "Value of zero means "
                          "there SHOULD be no unsolicited accounting updates.");

    break;
  case COPS_OBJ_INTEGRITY:
    if (c_type != 1)
      break;      /* Not HMAC digest */

    proto_tree_add_item(tree, hf_cops_key_id, tvb, offset, 4, FALSE);
    proto_tree_add_item(tree, hf_cops_seq_num, tvb, offset + 4, 4, FALSE);
    proto_tree_add_text(tree, tvb, offset + 8 , len - 8, "Contents: Keyed Message Digest");

    break;
  default:
    proto_tree_add_text(tree, tvb, offset, len, "Contents: %u bytes", len);

    break;
  }
}

#ifdef HAVE_NET_SNMP
static guchar*format_asn_value (struct variable_list *variable, subid_t *variable_oid,
                                guint variable_oid_length,gushort type_from_packet)
{
  struct tree *subtree=tree_head;
  
  guchar *buf=NULL;
  size_t buf_len=0;
  size_t out_len=0;
  
  /*Get the ASN.1 type etc. from the PIB-MIB. If unsuccessful use the type from packet*/
  subtree = get_tree(variable_oid,variable_oid_length, subtree);
  
  if (subtree->type == 0)
    variable->type=type_from_packet;
  
  buf_len = SPRINT_MAX_LEN; /*defined in NET-SNMP's snmp-impl.h*/
  buf = g_malloc(buf_len);
  *buf = '\0';
  out_len = 0;
 
  /*If the ASN.1 type was found from PIB-MIB, use it for decoding*/
  if (!variable->type)
    variable->type=mib_to_asn_type(subtree->type);
 
  if (!sprint_realloc_by_type(&buf, &buf_len, &out_len, TRUE, variable, subtree->enums, subtree->hint, NULL))
    sprintf(buf,"%s","sprint_realloc_by_type failed");
  
  return buf;
}
#endif	/* HAVE_NET_SNMP */

static int decode_cops_pr_asn1_data(tvbuff_t *tvb, guint32 offset,
    proto_tree *tree, guint epdlen, gboolean inepd
#ifndef HAVE_NET_SNMP
						  _U_
#endif
    )
{
  ASN1_SCK asn1;
  int start;
  gboolean def;
  guint length;

  guint vb_length;
  gushort vb_type;
  gchar *vb_type_name;

  int ret;
  guint cls, con, tag;
  subid_t epd_attribute_index=0;

  gint32 vb_integer_value;
  guint32 vb_uinteger_value;

  guint8 *vb_octet_string;

  subid_t *vb_oid;
  guint vb_oid_length;

  gchar *vb_display_string;
  gchar *vb_display_string2;

#ifdef HAVE_NET_SNMP
  struct variable_list variable;
  long value;
#endif	/* HAVE_NET_SNMP */
  
  unsigned int i;
  gchar *buf;
  int len;

  while (epdlen > 0) { /*while there is stuff to be decoded in the EPD*/

    epd_attribute_index++;
#ifdef HAVE_NET_SNMP
    last_decoded_prid_oid[last_decoded_prid_oid_length-1]=epd_attribute_index;
#endif	/* HAVE_NET_SNMP */
    asn1_open(&asn1, tvb, offset);

    /* parse the type of the object */

    start = asn1.offset;

    ret = asn1_header_decode (&asn1, &cls, &con, &tag, &def, &vb_length);
    if (ret != ASN1_ERR_NOERROR)
      return 0;
    if (!def)
      return ASN1_ERR_LENGTH_NOT_DEFINITE;

    /* Convert the class, constructed flag, and tag to a type. */
    vb_type_name = cops_tag_cls2syntax(tag, cls, &vb_type);
    if (vb_type_name == NULL) {
      /*
       * Unsupported type.
       * Dissect the value as an opaque string of octets.
       */
      vb_type_name = "unsupported type";
      vb_type = COPS_OPAQUE;
    }

    /* parse the value */

    switch (vb_type) {

    case COPS_INTEGER:
      ret = asn1_int32_value_decode(&asn1, vb_length, &vb_integer_value);
      if (ret != ASN1_ERR_NOERROR)
        return ret;
      length = asn1.offset - start;
      if (tree) {
#ifdef HAVE_NET_SNMP
        if (cops_typefrommib == TRUE)
        {
          variable.type = NULL;
          value = vb_integer_value;
          variable.val.integer = &value;
          variable.val_len = vb_length ;
          vb_display_string=format_asn_value(&variable, 
                                             last_decoded_prid_oid,last_decoded_prid_oid_length,ASN_INTEGER);
   
          proto_tree_add_text(tree, asn1.tvb, offset, length,
                              "Value: %s", vb_display_string);
          g_free(vb_display_string);
        }
        else
#endif /* HAVE_NET_SNMP */
          proto_tree_add_text(tree, asn1.tvb, offset, length,
                              "Value: %s: %d (%#x)", vb_type_name,
                              vb_integer_value, vb_integer_value);
      }
      break;

    case COPS_UNSIGNED32:
    case COPS_TIMETICKS:
      ret = asn1_uint32_value_decode(&asn1, vb_length, &vb_uinteger_value);
      if (ret != ASN1_ERR_NOERROR)
        return ret;
      length = asn1.offset - start;
      if (tree) {
#ifdef HAVE_NET_SNMP
        if (cops_typefrommib == TRUE)
        {
          variable.type = NULL;
          value = vb_uinteger_value;
          variable.val.integer = &value;
          variable.val_len = vb_length;

          vb_display_string=format_asn_value(&variable,
                                             last_decoded_prid_oid,last_decoded_prid_oid_length,ASN_UINTEGER);
     
          proto_tree_add_text(tree, asn1.tvb, offset, length, "Value %s: %s",vb_type_name, vb_display_string);
  
          g_free(vb_display_string);
        }
        else
#endif /* HAVE_NET_SNMP */
          proto_tree_add_text(tree, asn1.tvb, offset, length,
                              "Value: %s: %u (%#x)", vb_type_name,
                              vb_uinteger_value, vb_uinteger_value);
      }
      break;

    case COPS_OCTETSTR:
    case COPS_IPADDR:
    case COPS_OPAQUE:
    case COPS_UNSIGNED64:
    case COPS_INTEGER64:
      ret = asn1_string_value_decode (&asn1, vb_length, &vb_octet_string);
      if (ret != ASN1_ERR_NOERROR)
        return ret;
      length = asn1.offset - start;
      if (tree) {
#ifdef HAVE_NET_SNMP
        if (cops_typefrommib == TRUE)
        {
          variable.type = NULL;
          variable.val.string = vb_octet_string;
          variable.val_len = vb_length;
          vb_display_string = format_asn_value(&variable, 
                                               last_decoded_prid_oid,last_decoded_prid_oid_length,ASN_OCTET_STR);
          proto_tree_add_text(tree, asn1.tvb, offset, length,
                              "Value: %s (ASN.1 type from packet: %s)", vb_display_string, vb_type_name);
       
          g_free(vb_display_string);
        }
        else
        {
#endif /* HAVE_NET_SNMP */
          for (i = 0; i < vb_length; i++) {
            if (!(isprint(vb_octet_string[i]) ||isspace(vb_octet_string[i])))
              break;
          }

          /*
           * If some characters are not printable, display the string as bytes.
           */
          if (i < vb_length) {
            /*
             * We stopped, due to a non-printable character, before we got
             * to the end of the string.
             */
            vb_display_string = g_malloc(4*vb_length);
            buf = &vb_display_string[0];
            len = sprintf(buf, "%03u", vb_octet_string[0]);
            buf += len;
            for (i = 1; i < vb_length; i++) {
              len = sprintf(buf, ".%03u", vb_octet_string[i]);
              buf += len;
            }
            proto_tree_add_text(tree, asn1.tvb, offset, length,
                                "Value: %s: %s", vb_type_name, vb_display_string);
            g_free(vb_display_string);
          } else {
            proto_tree_add_text(tree, asn1.tvb, offset, length,
                                "Value: %s: %.*s", vb_type_name, (int)vb_length,
                                SAFE_STRING(vb_octet_string));
          }
#ifdef HAVE_NET_SNMP
        }
#endif /* HAVE_NET_SNMP */
      }
      g_free(vb_octet_string);
      break;

    case COPS_NULL:
      ret = asn1_null_decode (&asn1, vb_length);
      if (ret != ASN1_ERR_NOERROR)
        return ret;
      length = asn1.offset - start;
      if (tree)
        proto_tree_add_text(tree, asn1.tvb, offset, length, "Value: %s", vb_type_name);
      break;

    case COPS_OBJECTID:
      ret = asn1_oid_value_decode (&asn1, vb_length, &vb_oid, &vb_oid_length);
      if (ret != ASN1_ERR_NOERROR)
        return ret;
      length = asn1.offset - start;

      if (tree) {
        /*strip the instance Id from the OIDs before decoding and paste it back during printing*/
        new_format_oid(vb_oid,vb_oid_length-1,&vb_display_string,&vb_display_string2);
  
        /*if OID couldn't be decoded, print only numeric format*/
        if (!vb_display_string2)
          proto_tree_add_text(tree, asn1.tvb, offset, length,
                              "Value: %s: %s.%lu", vb_type_name,
                              vb_display_string,
                              (unsigned long)vb_oid[vb_oid_length-1]);
        else
          proto_tree_add_text(tree, asn1.tvb, offset, length,
                              "Value: %s: %s.%lu (%s.%lu)", vb_type_name,
                              vb_display_string,
                              (unsigned long)vb_oid[vb_oid_length-1],
                              vb_display_string2,
                              (unsigned long)vb_oid[vb_oid_length-1]);
#ifdef HAVE_NET_SNMP
        if (!inepd) {
          /* we're decoding PRID, so let's store the OID of the PRID so that later
             when we're decoding this PRID's EPD we can finetune the output.*/
          memcpy(last_decoded_prid_oid,vb_oid,vb_oid_length*sizeof(subid_t));
          last_decoded_prid_oid_length=vb_oid_length;
        }
#endif /* HAVE_NET_SNMP */
      }
      g_free(vb_display_string);
      if(vb_display_string2)
        g_free(vb_display_string2);
      g_free(vb_oid);
      break;

    default:
      g_assert_not_reached();
      return ASN1_ERR_WRONG_TYPE;
    }

    asn1_close(&asn1,&offset);

    epdlen -= length;
  }
  epd_attribute_index=0;
  return 0;
}

static int dissect_cops_pr_object_data(tvbuff_t *tvb, guint32 offset, proto_tree *tree,
                                       guint8 s_num, guint8 s_type, guint16 len)
{
  proto_item *ti;
  proto_tree *asn1_object_tree, *gperror_tree, *cperror_tree;
  guint16 gperror=0, gperror_sub=0, cperror=0, cperror_sub=0;

  switch (s_num){
  case COPS_OBJ_PRID:
   if (s_type != 1) /* Not Provisioning Instance Identifier (PRID) */
      break;

    ti=proto_tree_add_text(tree, tvb, offset, len, "Contents:");
    asn1_object_tree = proto_item_add_subtree(ti, ett_cops_asn1);

    decode_cops_pr_asn1_data(tvb, offset, asn1_object_tree, len, FALSE);

    break;
  case COPS_OBJ_PPRID:
    if (s_type != 1) /* Not Prefix Provisioning Instance Identifier (PPRID) */
      break;

    ti = proto_tree_add_text(tree, tvb, offset, len, "Contents:");
    asn1_object_tree = proto_item_add_subtree(ti, ett_cops_asn1);

    decode_cops_pr_asn1_data(tvb, offset, asn1_object_tree, len, FALSE);

    break;
  case COPS_OBJ_EPD:
    if (s_type != 1) /* Not  Encoded Provisioning Instance Data (EPD) */
      break;

    ti = proto_tree_add_text(tree, tvb, offset, len, "Contents:");
    asn1_object_tree = proto_item_add_subtree(ti, ett_cops_asn1);

    decode_cops_pr_asn1_data(tvb, offset, asn1_object_tree, len, TRUE);

    break;
  case COPS_OBJ_GPERR:
    if (s_type != 1) /* Not Global Provisioning Error Object (GPERR) */
      break;

    gperror = tvb_get_ntohs(tvb, offset);
    gperror_sub = tvb_get_ntohs(tvb, offset + 2);
    ti = proto_tree_add_text(tree, tvb, offset, 4, "Contents: Error-Code: %s, Error Sub-code: 0x%04x",
                       val_to_str(gperror, cops_gperror_vals, "<Unknown value>"), gperror_sub);
    gperror_tree = proto_item_add_subtree(ti, ett_cops_gperror);
    proto_tree_add_uint(gperror_tree, hf_cops_gperror, tvb, offset, 2, gperror);
    offset += 2;
    if (cperror == 13) {
      proto_tree_add_text(gperror_tree, tvb, offset, 2, "Error Sub-code: "
                          "Unknown object's C-Num %u, C-Type %u",
                          tvb_get_guint8(tvb, offset), tvb_get_guint8(tvb, offset + 1));
    } else
      proto_tree_add_uint(gperror_tree, hf_cops_gperror_sub, tvb, offset, 2, gperror_sub);

    break;
  case COPS_OBJ_CPERR:
    if (s_type != 1) /*Not PRC Class Provisioning Error Object (CPERR) */
      break;

    break;

    cperror = tvb_get_ntohs(tvb, offset);
    cperror_sub = tvb_get_ntohs(tvb, offset + 2);
    ti = proto_tree_add_text(tree, tvb, offset, 4, "Contents: Error-Code: %s, Error Sub-code: 0x%04x",
                       val_to_str(cperror, cops_cperror_vals, "<Unknown value>"), cperror_sub);
    cperror_tree = proto_item_add_subtree(ti, ett_cops_cperror);
    proto_tree_add_uint(cperror_tree, hf_cops_cperror, tvb, offset, 2, cperror);
    offset += 2;
    if (cperror == 13) {
      proto_tree_add_text(cperror_tree, tvb, offset, 2, "Error Sub-code: "
                          "Unknown object's S-Num %u, C-Type %u",
                          tvb_get_guint8(tvb, offset), tvb_get_guint8(tvb, offset + 1));
    } else
      proto_tree_add_uint(cperror_tree, hf_cops_cperror_sub, tvb, offset, 2, cperror_sub);

    break;
  case COPS_OBJ_ERRPRID:
    if (s_type != 1) /*Not  Error Provisioning Instance Identifier (ErrorPRID)*/
      break;

    ti = proto_tree_add_text(tree, tvb, offset, len, "Contents:");
    asn1_object_tree = proto_item_add_subtree(ti, ett_cops_asn1);

    decode_cops_pr_asn1_data(tvb, offset, asn1_object_tree, len, FALSE);

    break;
  default:
    proto_tree_add_text(tree, tvb, offset, len, "Contents: %u bytes", len);
    break;
  }

  return 0;
}


/* Register the protocol with Ethereal */
void proto_register_cops(void)
{
  /* Setup list of header fields */
  static hf_register_info hf[] = {
    { &hf_cops_ver_flags,
      { "Version and Flags",           "cops.ver_flags",
      FT_UINT8, BASE_HEX, NULL, 0x0,
      "Version and Flags in COPS Common Header", HFILL }
    },
    { &hf_cops_version,
      { "Version",           "cops.version",
      FT_UINT8, BASE_DEC, NULL, 0xF0,
      "Version in COPS Common Header", HFILL }
    },
    { &hf_cops_flags,
      { "Flags",           "cops.flags",
      FT_UINT8, BASE_HEX, VALS(cops_flags_vals), 0x0F,
      "Flags in COPS Common Header", HFILL }
    },
    { &hf_cops_op_code,
      { "Op Code",           "cops.op_code",
      FT_UINT8, BASE_DEC, VALS(cops_op_code_vals), 0x0,
      "Op Code in COPS Common Header", HFILL }
    },
    { &hf_cops_client_type,
      { "Client Type",           "cops.client_type",
      FT_UINT16, BASE_DEC, NULL, 0x0,
      "Client Type in COPS Common Header", HFILL }
    },
    { &hf_cops_msg_len,
      { "Message Length",           "cops.msg_len",
      FT_UINT32, BASE_DEC, NULL, 0x0,
      "Message Length in COPS Common Header", HFILL }
    },
    { &hf_cops_obj_len,
      { "Object Length",           "cops.obj.len",
      FT_UINT32, BASE_DEC, NULL, 0x0,
      "Object Length in COPS Object Header", HFILL }
    },
    { &hf_cops_obj_c_num,
      { "C-Num",           "cops.c_num",
      FT_UINT8, BASE_DEC, VALS(cops_c_num_vals), 0x0,
      "C-Num in COPS Object Header", HFILL }
    },
    { &hf_cops_obj_c_type,
      { "C-Type",           "cops.c_type",
      FT_UINT8, BASE_DEC, NULL, 0x0,
      "C-Type in COPS Object Header", HFILL }
    },

    { &hf_cops_obj_s_num,
      { "S-Num",           "cops.s_num",
      FT_UINT8, BASE_DEC, VALS(cops_s_num_vals), 0x0,
      "S-Num in COPS-PR Object Header", HFILL }
    },
    { &hf_cops_obj_s_type,
      { "S-Type",           "cops.s_type",
      FT_UINT8, BASE_DEC, NULL, 0x0,
      "S-Type in COPS-PR Object Header", HFILL }
    },

    { &hf_cops_r_type_flags,
      { "R-Type",           "cops.context.r_type",
      FT_UINT16, BASE_HEX, VALS(cops_r_type_vals), 0xFFFF,
      "R-Type in COPS Context Object", HFILL }
    },
    { &hf_cops_m_type_flags,
      { "M-Type",           "cops.context.m_type",
      FT_UINT16, BASE_HEX, NULL, 0xFFFF,
      "M-Type in COPS Context Object", HFILL }
    },
    { &hf_cops_in_int_ipv4,
      { "IPv4 address",           "cops.in-int.ipv4",
      FT_IPv4, 0, NULL, 0xFFFF,
      "IPv4 address in COPS IN-Int object", HFILL }
    },
    { &hf_cops_in_int_ipv6,
      { "IPv6 address",           "cops.in-int.ipv6",
      FT_IPv6, 0, NULL, 0xFFFF,
      "IPv6 address in COPS IN-Int object", HFILL }
    },
    { &hf_cops_out_int_ipv4,
      { "IPv4 address",           "cops.out-int.ipv4",
      FT_IPv4, 0, NULL, 0xFFFF,
      "IPv4 address in COPS OUT-Int object", HFILL }
    },
    { &hf_cops_out_int_ipv6,
      { "IPv6 address",           "cops.out-int.ipv6",
      FT_IPv6, 0, NULL, 0xFFFF,
      "IPv6 address in COPS OUT-Int", HFILL }
    },
    { &hf_cops_int_ifindex,
      { "ifIndex",           "cops.in-out-int.ifindex",
      FT_UINT32, BASE_DEC, NULL, 0x0,
      "If SNMP is supported, corresponds to MIB-II ifIndex", HFILL }
    },
    { &hf_cops_reason,
      { "Reason",           "cops.reason",
      FT_UINT16, BASE_DEC, VALS(cops_reason_vals), 0,
      "Reason in Reason object", HFILL }
    },
    { &hf_cops_reason_sub,
      { "Reason Sub-code",           "cops.reason_sub",
      FT_UINT16, BASE_HEX, NULL, 0,
      "Reason Sub-code in Reason object", HFILL }
    },
    { &hf_cops_dec_cmd_code,
      { "Command-Code",           "cops.decision.cmd",
      FT_UINT16, BASE_DEC, VALS(cops_dec_cmd_code_vals), 0,
      "Command-Code in Decision/LPDP Decision object", HFILL }
    },
    { &hf_cops_dec_flags,
      { "Flags",           "cops.decision.flags",
      FT_UINT16, BASE_HEX, VALS(cops_dec_cmd_flag_vals), 0xffff,
      "Flags in Decision/LPDP Decision object", HFILL }
    },
    { &hf_cops_error,
      { "Error",           "cops.error",
      FT_UINT16, BASE_DEC, VALS(cops_error_vals), 0,
      "Error in Error object", HFILL }
    },
    { &hf_cops_error_sub,
      { "Error Sub-code",           "cops.error_sub",
      FT_UINT16, BASE_HEX, NULL, 0,
      "Error Sub-code in Error object", HFILL }
    },
    { &hf_cops_katimer,
      { "Contents: KA Timer Value",           "cops.katimer.value",
      FT_UINT16, BASE_DEC, NULL, 0,
      "Keep-Alive Timer Value in KATimer object", HFILL }
    },
    { &hf_cops_pepid,
      { "Contents: PEP Id",           "cops.pepid.id",
      FT_STRING, BASE_NONE, NULL, 0,
      "PEP Id in PEPID object", HFILL }
    },
    { &hf_cops_report_type,
      { "Contents: Report-Type",           "cops.report_type",
      FT_UINT16, BASE_DEC, VALS(cops_report_type_vals), 0,
      "Report-Type in Report-Type object", HFILL }
    },
    { &hf_cops_pdprediraddr_ipv4,
      { "IPv4 address",           "cops.pdprediraddr.ipv4",
      FT_IPv4, 0, NULL, 0xFFFF,
      "IPv4 address in COPS PDPRedirAddr object", HFILL }
    },
    { &hf_cops_pdprediraddr_ipv6,
      { "IPv6 address",           "cops.pdprediraddr.ipv6",
      FT_IPv6, 0, NULL, 0xFFFF,
      "IPv6 address in COPS PDPRedirAddr object", HFILL }
    },
    { &hf_cops_lastpdpaddr_ipv4,
      { "IPv4 address",           "cops.lastpdpaddr.ipv4",
      FT_IPv4, 0, NULL, 0xFFFF,
      "IPv4 address in COPS LastPDPAddr object", HFILL }
    },
    { &hf_cops_lastpdpaddr_ipv6,
      { "IPv6 address",           "cops.lastpdpaddr.ipv6",
      FT_IPv6, 0, NULL, 0xFFFF,
      "IPv6 address in COPS LastPDPAddr object", HFILL }
    },
    { &hf_cops_pdp_tcp_port,
      { "TCP Port Number",           "cops.pdp.tcp_port",
      FT_UINT32, BASE_DEC, NULL, 0x0,
       "TCP Port Number of PDP in PDPRedirAddr/LastPDPAddr object", HFILL }
    },
    { &hf_cops_accttimer,
      { "Contents: ACCT Timer Value",           "cops.accttimer.value",
      FT_UINT16, BASE_DEC, NULL, 0,
      "Accounting Timer Value in AcctTimer object", HFILL }
    },
    { &hf_cops_key_id,
      { "Contents: Key ID",           "cops.integrity.key_id",
      FT_UINT32, BASE_DEC, NULL, 0,
      "Key ID in Integrity object", HFILL }
    },
    { &hf_cops_seq_num,
      { "Contents: Sequence Number",           "cops.integrity.seq_num",
      FT_UINT32, BASE_DEC, NULL, 0,
      "Sequence Number in Integrity object", HFILL }
    },
    { &hf_cops_gperror,
      { "Error",           "cops.gperror",
      FT_UINT16, BASE_DEC, VALS(cops_gperror_vals), 0,
      "Error in Error object", HFILL }
    },
    { &hf_cops_gperror_sub,
      { "Error Sub-code",           "cops.gperror_sub",
      FT_UINT16, BASE_HEX, NULL, 0,
      "Error Sub-code in Error object", HFILL }
    },
    { &hf_cops_cperror,
      { "Error",           "cops.cperror",
      FT_UINT16, BASE_DEC, VALS(cops_cperror_vals), 0,
      "Error in Error object", HFILL }
    },
    { &hf_cops_cperror_sub,
      { "Error Sub-code",           "cops.cperror_sub",
      FT_UINT16, BASE_HEX, NULL, 0,
      "Error Sub-code in Error object", HFILL }
    },

  };

  /* Setup protocol subtree array */
  static gint *ett[] = {
    &ett_cops,
    &ett_cops_ver_flags,
    &ett_cops_obj,
    &ett_cops_pr_obj,
    &ett_cops_obj_data,
    &ett_cops_r_type_flags,
    &ett_cops_itf,
    &ett_cops_reason,
    &ett_cops_decision,
    &ett_cops_error,
    &ett_cops_clientsi,
    &ett_cops_asn1,
    &ett_cops_gperror,
    &ett_cops_cperror,
    &ett_cops_pdp,
  };

  module_t* cops_module;

  /* Register the protocol name and description */
  proto_cops = proto_register_protocol("Common Open Policy Service",
      "COPS", "cops");

  /* Required function calls to register the header fields and subtrees used */
  proto_register_field_array(proto_cops, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));

  /* Register our configuration options for cops */
  cops_module = prefs_register_protocol(proto_cops, proto_reg_handoff_cops);
  prefs_register_uint_preference(cops_module,"tcp.cops_port",
                                 "COPS TCP Port",
                                 "Set the TCP port for COPS messages",
                                 10,&global_cops_tcp_port);
  prefs_register_bool_preference(cops_module, "desegment",
                                 "Desegment all COPS messages spanning multiple TCP segments",
                                 "Whether the COPS dissector should desegment all messages spanning multiple TCP segments",
                                 &cops_desegment);
#ifdef HAVE_NET_SNMP /*enable preference only if compiled with NET-SNMP*/
  prefs_register_bool_preference(cops_module, "typefrommib",
                                 "Decode COPS-PR ASN.1 types by reading them from PIBs (converted to MIBs)",
                                 "Whether the COPS dissector should decode COPS-PR ASN.1 types based on data types read from packet or PIBs (converted to MIBs)",
                                 &cops_typefrommib);
#endif /*HAVE_NET_SNMP*/
};

void proto_reg_handoff_cops(void)
{
  static int cops_prefs_initialized = FALSE;
  static dissector_handle_t cops_handle;
  
  if (!cops_prefs_initialized) {
    cops_handle = create_dissector_handle(dissect_cops, proto_cops);
    cops_prefs_initialized = TRUE;
  } else
    dissector_delete("tcp.port",cops_tcp_port,cops_handle);
  
  /* Set our port numbers for future use */
  cops_tcp_port = global_cops_tcp_port;
  
  dissector_add("tcp.port", cops_tcp_port, cops_handle);
}
