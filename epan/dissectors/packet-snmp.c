/* Do not modify this file.                                                   */
/* It is created automatically by the ASN.1 to Wireshark dissector compiler   */
/* ./packet-snmp.c                                                            */
/* ../../tools/asn2wrs.py -b -e -p snmp -c snmp.cnf -s packet-snmp-template snmp.asn */

/* Input file: packet-snmp-template.c */

#line 1 "packet-snmp-template.c"
/* packet-snmp.c
 * Routines for SNMP (simple network management protocol)
 * Copyright (C) 1998 Didier Jorand
 *
 * See RFC 1157 for SNMPv1.
 *
 * See RFCs 1901, 1905, and 1906 for SNMPv2c.
 *
 * See RFCs 1905, 1906, 1909, and 1910 for SNMPv2u [historic].
 *
 * See RFCs 2570-2576 for SNMPv3
 * Updated to use the asn2wrs compiler made by Tomas Kukosa
 * Copyright (C) 2005 - 2006 Anders Broman [AT] ericsson.com
 *
 * See RFC 3414 for User-based Security Model for SNMPv3
 * See RFC 3826 for  (AES) Cipher Algorithm in the SNMP USM
 * Copyright (C) 2007 Luis E. Garcia Ontanon <luis.ontanon@gmail.com>
 *
 * $Id$
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * Some stuff from:
 *
 * GXSNMP -- An snmp mangament application
 * Copyright (C) 1998 Gregory McLean & Jochen Friedrich
 * Beholder RMON ethernet network monitor,Copyright (C) 1993 DNPAP group
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
#include <string.h>
#include <ctype.h>

#include <glib.h>

#include <epan/packet.h>
#include <epan/strutil.h>
#include <epan/conversation.h>
#include "etypes.h"
#include <epan/prefs.h>
#include <epan/sminmpec.h>
#include <epan/emem.h>
#include <epan/next_tvb.h>
#include "packet-ipx.h"
#include "packet-hpext.h"


#include "packet-ber.h"

#ifdef HAVE_NET_SNMP
# include <net-snmp/net-snmp-config.h>
# include <net-snmp/mib_api.h>
# include <net-snmp/library/default_store.h>
# include <net-snmp/config_api.h>

#ifdef _WIN32
# include <epan/filesystem.h>
#endif /* _WIN32 */

   /*
    * Define values "sprint_realloc_value()" expects.
    */
# define VALTYPE_INTEGER	ASN_INTEGER
# define VALTYPE_COUNTER	ASN_COUNTER
# define VALTYPE_GAUGE		ASN_GAUGE
# define VALTYPE_TIMETICKS	ASN_TIMETICKS
# define VALTYPE_STRING		ASN_OCTET_STR
# define VALTYPE_IPADDR		ASN_IPADDRESS
# define VALTYPE_OPAQUE		ASN_OPAQUE
# define VALTYPE_NSAP		ASN_NSAP
# define VALTYPE_OBJECTID	ASN_OBJECT_ID
# define VALTYPE_BITSTR		ASN_BIT_STR
# define VALTYPE_COUNTER64	ASN_COUNTER64

#endif /* HAVE_NET_SNMP */
#include "packet-snmp.h"
#include "format-oid.h"

#include <epan/sha1.h>
#include <epan/crypt/crypt-md5.h>
#include <epan/expert.h>
#include <epan/report_err.h>

#ifdef _WIN32
#include <winposixtype.h>
#endif /* _WIN32 */

#ifdef HAVE_LIBGCRYPT
#include <gcrypt.h>
#endif


/* Take a pointer that may be null and return a pointer that's not null
   by turning null pointers into pointers to the above null string,
   and, if the argument pointer wasn't null, make sure we handle
   non-printable characters in the string by escaping them. */
#define	SAFE_STRING(s, l)	(((s) != NULL) ? format_text((s), (l)) : "")

#define PNAME  "Simple Network Management Protocol"
#define PSNAME "SNMP"
#define PFNAME "snmp"

#define UDP_PORT_SNMP		161
#define UDP_PORT_SNMP_TRAP	162
#define TCP_PORT_SNMP		161
#define TCP_PORT_SNMP_TRAP	162
#define TCP_PORT_SMUX		199

/* Initialize the protocol and registered fields */
static int proto_snmp = -1;
static int proto_smux = -1;

/* Default MIB modules to load */
/*
 * XXX - According to Wes Hardaker, we shouldn't do this:
 *       http://www.ethereal.com/lists/ethereal-dev/200412/msg00222.html
 */
#ifdef _WIN32
# define DEF_MIB_MODULES "IP-MIB;IF-MIB;TCP-MIB;UDP-MIB;SNMPv2-MIB;RFC1213-MIB;UCD-SNMP-MIB"
# define IMPORT_SEPARATOR ":"
#else
# define DEF_MIB_MODULES "IP-MIB:IF-MIB:TCP-MIB:UDP-MIB:SNMPv2-MIB:RFC1213-MIB:UCD-SNMP-MIB"
# define IMPORT_SEPARATOR ";"
#endif /* _WIN32 */

static const gchar *mib_modules = DEF_MIB_MODULES;
static gboolean display_oid = TRUE;
static gboolean snmp_var_in_tree = TRUE;

static const gchar* ue_assocs_filename = "";
static const gchar* ue_assocs_filename_loaded = "";
static snmp_ue_assoc_t* ue_assocs = NULL;
static snmp_ue_assoc_t* localized_ues = NULL;
static snmp_ue_assoc_t* unlocalized_ues = NULL;

static snmp_usm_params_t usm_p = {FALSE,FALSE,0,0,0,0,NULL,NULL,NULL,NULL,NULL,NULL,NULL,FALSE};

/* Subdissector tables */
static dissector_table_t variable_oid_dissector_table;

#define TH_AUTH   0x01
#define TH_CRYPT  0x02
#define TH_REPORT 0x04

/* desegmentation of SNMP-over-TCP */
static gboolean snmp_desegment = TRUE;

/* Global variables */

guint32 MsgSecurityModel;
tvbuff_t *oid_tvb=NULL;
tvbuff_t *value_tvb=NULL;

static dissector_handle_t snmp_handle;
static dissector_handle_t data_handle;

static next_tvb_list_t var_list;

static int hf_snmp_v3_flags_auth = -1;
static int hf_snmp_v3_flags_crypt = -1;
static int hf_snmp_v3_flags_report = -1;

static int hf_snmp_engineid_conform = -1;
static int hf_snmp_engineid_enterprise = -1;
static int hf_snmp_engineid_format = -1;
static int hf_snmp_engineid_ipv4 = -1;
static int hf_snmp_engineid_ipv6 = -1;
static int hf_snmp_engineid_mac = -1;
static int hf_snmp_engineid_text = -1;
static int hf_snmp_engineid_time = -1;
static int hf_snmp_engineid_data = -1;
static int hf_snmp_counter64 = -1;
static int hf_snmp_decryptedPDU = -1;
static int hf_snmp_msgAuthentication = -1;


/*--- Included file: packet-snmp-hf.c ---*/
#line 1 "packet-snmp-hf.c"
static int hf_snmp_SMUX_PDUs_PDU = -1;            /* SMUX_PDUs */
static int hf_snmp_simple = -1;                   /* SimpleSyntax */
static int hf_snmp_application_wide = -1;         /* ApplicationSyntax */
static int hf_snmp_integer_value = -1;            /* Integer_value */
static int hf_snmp_string_value = -1;             /* String_value */
static int hf_snmp_objectID_value = -1;           /* ObjectID_value */
static int hf_snmp_empty = -1;                    /* Empty */
static int hf_snmp_ipAddress_value = -1;          /* IpAddress */
static int hf_snmp_counter_value = -1;            /* Counter32 */
static int hf_snmp_timeticks_value = -1;          /* TimeTicks */
static int hf_snmp_arbitrary_value = -1;          /* Opaque */
static int hf_snmp_big_counter_value = -1;        /* Counter64 */
static int hf_snmp_unsigned_integer_value = -1;   /* Unsigned32 */
static int hf_snmp_internet = -1;                 /* IpAddress */
static int hf_snmp_version = -1;                  /* Version */
static int hf_snmp_community = -1;                /* OCTET_STRING */
static int hf_snmp_data = -1;                     /* PDUs */
static int hf_snmp_parameters = -1;               /* OCTET_STRING */
static int hf_snmp_datav2u = -1;                  /* T_datav2u */
static int hf_snmp_v2u_plaintext = -1;            /* PDUs */
static int hf_snmp_encrypted = -1;                /* OCTET_STRING */
static int hf_snmp_msgAuthoritativeEngineID = -1;  /* T_msgAuthoritativeEngineID */
static int hf_snmp_msgAuthoritativeEngineBoots = -1;  /* T_msgAuthoritativeEngineBoots */
static int hf_snmp_msgAuthoritativeEngineTime = -1;  /* T_msgAuthoritativeEngineTime */
static int hf_snmp_msgUserName = -1;              /* T_msgUserName */
static int hf_snmp_msgAuthenticationParameters = -1;  /* T_msgAuthenticationParameters */
static int hf_snmp_msgPrivacyParameters = -1;     /* T_msgPrivacyParameters */
static int hf_snmp_msgVersion = -1;               /* Version */
static int hf_snmp_msgGlobalData = -1;            /* HeaderData */
static int hf_snmp_msgSecurityParameters = -1;    /* T_msgSecurityParameters */
static int hf_snmp_msgData = -1;                  /* ScopedPduData */
static int hf_snmp_msgID = -1;                    /* INTEGER_0_2147483647 */
static int hf_snmp_msgMaxSize = -1;               /* INTEGER_484_2147483647 */
static int hf_snmp_msgFlags = -1;                 /* T_msgFlags */
static int hf_snmp_msgSecurityModel = -1;         /* T_msgSecurityModel */
static int hf_snmp_plaintext = -1;                /* ScopedPDU */
static int hf_snmp_encryptedPDU = -1;             /* T_encryptedPDU */
static int hf_snmp_contextEngineID = -1;          /* OCTET_STRING */
static int hf_snmp_contextName = -1;              /* OCTET_STRING */
static int hf_snmp_get_request = -1;              /* T_get_request */
static int hf_snmp_get_next_request = -1;         /* T_get_next_request */
static int hf_snmp_get_response = -1;             /* T_get_response */
static int hf_snmp_set_request = -1;              /* T_set_request */
static int hf_snmp_trap = -1;                     /* T_trap */
static int hf_snmp_getBulkRequest = -1;           /* T_getBulkRequest */
static int hf_snmp_informRequest = -1;            /* T_informRequest */
static int hf_snmp_sNMPv2_Trap = -1;              /* T_sNMPv2_Trap */
static int hf_snmp_report = -1;                   /* T_report */
static int hf_snmp_request_id = -1;               /* INTEGER */
static int hf_snmp_error_status = -1;             /* T_error_status */
static int hf_snmp_error_index = -1;              /* INTEGER */
static int hf_snmp_variable_bindings = -1;        /* VarBindList */
static int hf_snmp_bulkPDU_request_id = -1;       /* Integer32 */
static int hf_snmp_non_repeaters = -1;            /* INTEGER_0_2147483647 */
static int hf_snmp_max_repetitions = -1;          /* INTEGER_0_2147483647 */
static int hf_snmp_enterprise = -1;               /* OBJECT_IDENTIFIER */
static int hf_snmp_agent_addr = -1;               /* NetworkAddress */
static int hf_snmp_generic_trap = -1;             /* T_generic_trap */
static int hf_snmp_specific_trap = -1;            /* INTEGER */
static int hf_snmp_time_stamp = -1;               /* TimeTicks */
static int hf_snmp_name = -1;                     /* ObjectName */
static int hf_snmp_valueType = -1;                /* ValueType */
static int hf_snmp_value = -1;                    /* ObjectSyntax */
static int hf_snmp_unSpecified = -1;              /* NULL */
static int hf_snmp_noSuchObject = -1;             /* NULL */
static int hf_snmp_noSuchInstance = -1;           /* NULL */
static int hf_snmp_endOfMibView = -1;             /* NULL */
static int hf_snmp_VarBindList_item = -1;         /* VarBind */
static int hf_snmp_open = -1;                     /* OpenPDU */
static int hf_snmp_close = -1;                    /* ClosePDU */
static int hf_snmp_registerRequest = -1;          /* RReqPDU */
static int hf_snmp_registerResponse = -1;         /* RegisterResponse */
static int hf_snmp_commitOrRollback = -1;         /* SOutPDU */
static int hf_snmp_rRspPDU = -1;                  /* RRspPDU */
static int hf_snmp_pDUs = -1;                     /* PDUs */
static int hf_snmp_smux_simple = -1;              /* SimpleOpen */
static int hf_snmp_smux_version = -1;             /* T_smux_version */
static int hf_snmp_identity = -1;                 /* OBJECT_IDENTIFIER */
static int hf_snmp_description = -1;              /* DisplayString */
static int hf_snmp_password = -1;                 /* OCTET_STRING */
static int hf_snmp_subtree = -1;                  /* ObjectName */
static int hf_snmp_priority = -1;                 /* INTEGER_M1_2147483647 */
static int hf_snmp_operation = -1;                /* T_operation */

/*--- End of included file: packet-snmp-hf.c ---*/
#line 197 "packet-snmp-template.c"

static int hf_smux_version = -1;
static int hf_smux_pdutype = -1;

/* Initialize the subtree pointers */
static gint ett_smux = -1;
static gint ett_snmp = -1;
static gint ett_engineid = -1;
static gint ett_msgFlags = -1;
static gint ett_encryptedPDU = -1;
static gint ett_decrypted = -1;
static gint ett_authParameters = -1;


/*--- Included file: packet-snmp-ett.c ---*/
#line 1 "packet-snmp-ett.c"
static gint ett_snmp_ObjectSyntax = -1;
static gint ett_snmp_SimpleSyntax = -1;
static gint ett_snmp_ApplicationSyntax = -1;
static gint ett_snmp_NetworkAddress = -1;
static gint ett_snmp_Message = -1;
static gint ett_snmp_Messagev2u = -1;
static gint ett_snmp_T_datav2u = -1;
static gint ett_snmp_UsmSecurityParameters = -1;
static gint ett_snmp_SNMPv3Message = -1;
static gint ett_snmp_HeaderData = -1;
static gint ett_snmp_ScopedPduData = -1;
static gint ett_snmp_ScopedPDU = -1;
static gint ett_snmp_PDUs = -1;
static gint ett_snmp_PDU = -1;
static gint ett_snmp_BulkPDU = -1;
static gint ett_snmp_Trap_PDU = -1;
static gint ett_snmp_VarBind = -1;
static gint ett_snmp_ValueType = -1;
static gint ett_snmp_VarBindList = -1;
static gint ett_snmp_SMUX_PDUs = -1;
static gint ett_snmp_RegisterResponse = -1;
static gint ett_snmp_OpenPDU = -1;
static gint ett_snmp_SimpleOpen = -1;
static gint ett_snmp_RReqPDU = -1;

/*--- End of included file: packet-snmp-ett.c ---*/
#line 211 "packet-snmp-template.c"


static const true_false_string auth_flags = {
	"OK",
	"Failed"
};

/* defined in net-SNMP; include/net-snmp/library/snmp.h */
#undef SNMP_MSG_GET
#undef SNMP_MSG_SET
#undef SNMP_MSG_GETNEXT
#undef SNMP_MSG_RESPONSE
#undef SNMP_MSG_TRAP
#undef SNMP_MSG_GETBULK
#undef SNMP_MSG_INFORM
#undef SNMP_MSG_TRAP2
#undef SNMP_MSG_REPORT
#undef SNMP_NOSUCHOBJECT
#undef SNMP_NOSUCHINSTANCE
#undef SNMP_ENDOFMIBVIEW

/* Security Models */

#define SNMP_SEC_ANY			0
#define SNMP_SEC_V1				1
#define SNMP_SEC_V2C			2
#define SNMP_SEC_USM			3

static const value_string sec_models[] = {
	{ SNMP_SEC_ANY,			"Any" },
	{ SNMP_SEC_V1,			"V1" },
	{ SNMP_SEC_V2C,			"V2C" },
	{ SNMP_SEC_USM,			"USM" },
	{ 0,				NULL }
};

/* SMUX PDU types */
#define SMUX_MSG_OPEN 		0
#define SMUX_MSG_CLOSE		1
#define SMUX_MSG_RREQ		2
#define SMUX_MSG_RRSP		3
#define SMUX_MSG_SOUT		4

static const value_string smux_types[] = {
	{ SMUX_MSG_OPEN,	"Open" },
	{ SMUX_MSG_CLOSE,	"Close" },
	{ SMUX_MSG_RREQ,	"Registration Request" },
	{ SMUX_MSG_RRSP,	"Registration Response" },
	{ SMUX_MSG_SOUT,	"Commit Or Rollback" },
	{ 0,			NULL }
};

/* SNMP Tags */

#define SNMP_IPA    0		/* IP Address */
#define SNMP_CNT    1		/* Counter (Counter32) */
#define SNMP_GGE    2		/* Gauge (Gauge32) */
#define SNMP_TIT    3		/* TimeTicks */
#define SNMP_OPQ    4		/* Opaque */
#define SNMP_NSP    5		/* NsapAddress */
#define SNMP_C64    6		/* Counter64 */
#define SNMP_U32    7		/* Uinteger32 */

#define SERR_NSO    0
#define SERR_NSI    1
#define SERR_EOM    2

/* SNMPv1 Types */

#define SNMP_NULL                0
#define SNMP_INTEGER             1    /* l  */
#define SNMP_OCTETSTR            2    /* c  */
#define SNMP_DISPLAYSTR          2    /* c  */
#define SNMP_OBJECTID            3    /* ul */
#define SNMP_IPADDR              4    /* uc */
#define SNMP_COUNTER             5    /* ul */
#define SNMP_GAUGE               6    /* ul */
#define SNMP_TIMETICKS           7    /* ul */
#define SNMP_OPAQUE              8    /* c  */

/* additional SNMPv2 Types */

#define SNMP_UINTEGER            5    /* ul */
#define SNMP_BITSTR              9    /* uc */
#define SNMP_NSAP               10    /* uc */
#define SNMP_COUNTER64          11    /* ul */
#define SNMP_NOSUCHOBJECT       12
#define SNMP_NOSUCHINSTANCE     13
#define SNMP_ENDOFMIBVIEW       14


typedef struct _SNMP_CNV SNMP_CNV;

struct _SNMP_CNV
{
  guint class;
  guint tag;
  gint  syntax;
  const gchar *name;
};

static SNMP_CNV SnmpCnv [] =
{
  {BER_CLASS_UNI, BER_UNI_TAG_NULL,			SNMP_NULL,      "NULL"},
  {BER_CLASS_UNI, BER_UNI_TAG_INTEGER,		SNMP_INTEGER,   "INTEGER"},
  {BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING,	SNMP_OCTETSTR,  "OCTET STRING"},
  {BER_CLASS_UNI, BER_UNI_TAG_OID,			SNMP_OBJECTID,  "OBJECTID"},
  {BER_CLASS_APP, SNMP_IPA,					SNMP_IPADDR,    "IPADDR"},
  {BER_CLASS_APP, SNMP_CNT,					SNMP_COUNTER,   "COUNTER"},  /* Counter32 */
  {BER_CLASS_APP, SNMP_GGE,					SNMP_GAUGE,     "GAUGE"},    /* Gauge32 == Unsigned32  */
  {BER_CLASS_APP, SNMP_TIT,					SNMP_TIMETICKS, "TIMETICKS"},
  {BER_CLASS_APP, SNMP_OPQ,					SNMP_OPAQUE,    "OPAQUE"},

/* SNMPv2 data types and errors */

  {BER_CLASS_UNI, BER_UNI_TAG_BITSTRING,	SNMP_BITSTR,         "BITSTR"},
  {BER_CLASS_APP, SNMP_C64,					SNMP_COUNTER64,      "COUNTER64"},
  {BER_CLASS_CON, SERR_NSO,					SNMP_NOSUCHOBJECT,   "NOSUCHOBJECT"},
  {BER_CLASS_CON, SERR_NSI,					SNMP_NOSUCHINSTANCE, "NOSUCHINSTANCE"},
  {BER_CLASS_CON, SERR_EOM,					SNMP_ENDOFMIBVIEW,   "ENDOFMIBVIEW"},
  {0,		0,         -1,                  NULL}
};

/*
 * NAME:        g_snmp_tag_cls2syntax
 * SYNOPSIS:    gboolean g_snmp_tag_cls2syntax
 *                  (
 *                      guint    tag,
 *                      guint    cls,
 *                      gushort *syntax
 *                  )
 * DESCRIPTION: Converts ASN1 tag and class to Syntax tag and name.
 *              See SnmpCnv for conversion.
 * RETURNS:     name on success, NULL on failure
 */

static const gchar *
snmp_tag_cls2syntax ( guint tag, guint cls, gushort *syntax)
{
    SNMP_CNV *cnv;

    cnv = SnmpCnv;
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

int oid_to_subid_buf(const guint8 *oid, gint oid_len, subid_t *buf, int buf_len) {
  int i, out_len;
  guint8 byte;
  guint32 value;
  gboolean is_first;

  value=0; out_len = 0; byte =0; is_first = TRUE;
  for (i=0; i<oid_len; i++){
    if (out_len >= buf_len)
      break;
    byte = oid[i];
    value = (value << 7) | (byte & 0x7F);
    if (byte & 0x80) {
      continue;
    }
    if (is_first) {
      if ( value<40 ){
        buf[0] = 0;
        buf[1] = value;
      }else if ( value < 80 ){
        buf[0] = 1;
        buf[1] = value - 40;
      }else {
        buf[0] = 2;
        buf[1] = value - 80;
      }
      out_len= out_len+2;
      is_first = FALSE;
    }else{
      buf[out_len++] = value;
    }
    value = 0;
  }

  return out_len;
}

gchar *
format_oid(subid_t *oid, guint oid_length)
{
	char *result;
	int result_len;
	int len;
	unsigned int i;
	char *buf;
#ifdef HAVE_NET_SNMP
	guchar *oid_string;
	size_t oid_string_len;
	size_t oid_out_len;
#endif

	result_len = oid_length * 22;

#ifdef HAVE_NET_SNMP
	/*
	 * Get the decoded form of the OID, and add its length to the
	 * length of the result string.
	 *
	 * XXX - check for "sprint_realloc_objid()" failure.
	 */
	oid_string_len = 1024;
	oid_string = ep_alloc(oid_string_len);
	*oid_string = '\0';
	oid_out_len = 0;
	/* We pass an ep allocated block here, NOT a malloced block
	 * so we MUST NOT allow reallocation, hence the fourth
	 * parameter MUST be 0/FALSE
	 */
	sprint_realloc_objid(&oid_string, &oid_string_len, &oid_out_len, FALSE,
	    oid, oid_length);
	result_len += strlen(oid_string) + 3;
#endif

	result = ep_alloc(result_len + 1);
	buf = result;
	len = g_snprintf(buf, result_len + 1 - (buf-result), "%lu", (unsigned long)oid[0]);
	buf += len;
	for (i = 1; i < oid_length;i++) {
		len = g_snprintf(buf, result_len + 1 - (buf-result), ".%lu", (unsigned long)oid[i]);
		buf += len;
	}

#ifdef HAVE_NET_SNMP
	/*
	 * Append the decoded form of the OID.
	 */
	g_snprintf(buf, result_len + 1 -(buf-result), " (%s)", oid_string);
#endif

	return result;
}

/* returns the decoded (can be NULL) and non_decoded OID strings */
void
new_format_oid(subid_t *oid, guint oid_length,
	       gchar **non_decoded, gchar **decoded)
{
	int non_decoded_len;
	int len;
	unsigned int i;
	char *buf;
#ifdef HAVE_NET_SNMP
	guchar *oid_string;
	size_t oid_string_len;
	size_t oid_out_len;
#endif

    if (oid == NULL || oid_length < 1) {
		*decoded = NULL;
		return;
	}

#ifdef HAVE_NET_SNMP
	/*
	 * Get the decoded form of the OID, and add its length to the
	 * length of the result string.
	 */

	/*
	 * XXX - if we convert this to ep_alloc(), make sure the fourth
	 * argument to sprint_realloc_objid() is FALSE.
	 */

	oid_string_len = 1024;
	oid_string = ep_alloc(oid_string_len);
	if (oid_string != NULL) {
		*oid_string = '\0';
		oid_out_len = 0;
		/* We pass an ep allocated block here, NOT a malloced block
		 * so we MUST NOT allow reallocation, hence the fourth
		 * parameter MUST be 0/FALSE
		 */
		sprint_realloc_objid(&oid_string, &oid_string_len, &oid_out_len, FALSE,
				     oid, oid_length);
	}
	*decoded = oid_string;
#else
	*decoded = NULL;
#endif

	non_decoded_len = oid_length * 22 + 1;
	*non_decoded = ep_alloc(non_decoded_len);
	buf = *non_decoded;
	len = g_snprintf(buf, non_decoded_len-(buf-*non_decoded), "%lu", (unsigned long)oid[0]);
	buf += len;
	for (i = 1; i < oid_length; i++) {
	  len = g_snprintf(buf, non_decoded_len-(buf-*non_decoded), ".%lu", (unsigned long)oid[i]);
	  buf += len;
	}
}

#ifdef HAVE_NET_SNMP
static gboolean
check_var_length(guint vb_length, guint required_length, guchar **errmsg)
{
	gchar *buf;
	static const char badlen_fmt[] = "Length is %u, should be %u";

	if (vb_length != required_length) {
		/* Enough room for the largest "Length is XXX,
		   should be XXX" message - 10 digits for each
		   XXX. */
		buf = ep_alloc(sizeof badlen_fmt + 10 + 10);
		if (buf != NULL) {
			g_snprintf(buf, sizeof badlen_fmt + 10 + 10,
			    badlen_fmt, vb_length, required_length);
		}
		*errmsg = buf;
		return FALSE;
	}
	return TRUE;	/* length is OK */
}

static gchar *
format_var(struct variable_list *variable, subid_t *variable_oid,
    guint variable_oid_length, gushort vb_type, guint val_len)
{
	guchar *buf;
	size_t buf_len;
	size_t out_len;

        if (variable_oid == NULL || variable_oid_length == 0)
                return NULL;

	switch (vb_type) {

	case SNMP_IPADDR:
		/* Length has to be 4 bytes. */
		if (!check_var_length(val_len, 4, &buf))
			return buf;	/* it's not 4 bytes */
		break;

#ifdef REMOVED
	/* not all counters are encoded as a full 64bit integer */
	case SNMP_COUNTER64:
		/* Length has to be 8 bytes. */
		if (!check_var_length(val_len, 8, &buf))
			return buf;	/* it's not 8 bytes */
		break;
#endif
	default:
		break;
	}

	variable->next_variable = NULL;
	variable->name = variable_oid;
	variable->name_length = variable_oid_length;
	switch (vb_type) {

	case SNMP_INTEGER:
		variable->type = VALTYPE_INTEGER;
		break;

	case SNMP_COUNTER:
		variable->type = VALTYPE_COUNTER;
		break;

	case SNMP_GAUGE:
		variable->type = VALTYPE_GAUGE;
		break;

	case SNMP_TIMETICKS:
		variable->type = VALTYPE_TIMETICKS;
		break;

	case SNMP_OCTETSTR:
		variable->type = VALTYPE_STRING;
		break;

	case SNMP_IPADDR:
		variable->type = VALTYPE_IPADDR;
		break;

	case SNMP_OPAQUE:
		variable->type = VALTYPE_OPAQUE;
		break;

	case SNMP_NSAP:
		variable->type = VALTYPE_NSAP;
		break;

	case SNMP_OBJECTID:
		variable->type = VALTYPE_OBJECTID;
		break;

	case SNMP_BITSTR:
		variable->type = VALTYPE_BITSTR;
		break;

	case SNMP_COUNTER64:
		variable->type = VALTYPE_COUNTER64;
		break;
	}
	variable->val_len = val_len;

	/*
	 * XXX - check for "sprint_realloc_objid()" failure.
	 */

	buf_len = 1024;
	buf = ep_alloc(buf_len);
	if (buf != NULL) {
		*buf = '\0';
		out_len = 0;
		/* We pass an ep allocated block here, NOT a malloced block
		 * so we MUST NOT allow reallocation, hence the fourth
		 * parameter MUST be 0/FALSE
		 */
		sprint_realloc_value(&buf, &buf_len, &out_len, FALSE,
		    variable_oid, variable_oid_length, variable);
	}
	return buf;
}
#endif


#define F_SNMP_ENGINEID_CONFORM 0x80
#define SNMP_ENGINEID_RFC1910 0x00
#define SNMP_ENGINEID_RFC3411 0x01

static const true_false_string tfs_snmp_engineid_conform = {
  "RFC3411 (SNMPv3)",
  "RFC1910 (Non-SNMPv3)"
};

#define SNMP_ENGINEID_FORMAT_IPV4 0x01
#define SNMP_ENGINEID_FORMAT_IPV6 0x02
#define SNMP_ENGINEID_FORMAT_MACADDRESS 0x03
#define SNMP_ENGINEID_FORMAT_TEXT 0x04
#define SNMP_ENGINEID_FORMAT_OCTETS 0x05

static const value_string snmp_engineid_format_vals[] = {
	{ SNMP_ENGINEID_FORMAT_IPV4,	"IPv4 address" },
	{ SNMP_ENGINEID_FORMAT_IPV6,	"IPv6 address" },
	{ SNMP_ENGINEID_FORMAT_MACADDRESS,	"MAC address" },
	{ SNMP_ENGINEID_FORMAT_TEXT,	"Text, administratively assigned" },
	{ SNMP_ENGINEID_FORMAT_OCTETS,	"Octets, administratively assigned" },
	{ 0,   	NULL }
};

/*
 * SNMP Engine ID dissection according to RFC 3411 (SnmpEngineID TC)
 * or historic RFC 1910 (AgentID)
 */
int
dissect_snmp_engineid(proto_tree *tree, tvbuff_t *tvb, int offset, int len)
{
    proto_item *item = NULL;
    guint8 conformance, format;
    guint32 enterpriseid, seconds;
    nstime_t ts;
    int len_remain = len;

    /* first bit: engine id conformance */
    if (len_remain<4) return offset;
    conformance = ((tvb_get_guint8(tvb, offset)>>7) && 0x01);
    proto_tree_add_item(tree, hf_snmp_engineid_conform, tvb, offset, 1, FALSE);

    /* 4-byte enterprise number/name */
    if (len_remain<4) return offset;
    enterpriseid = tvb_get_ntohl(tvb, offset);
    if (conformance)
      enterpriseid -= 0x80000000; /* ignore first bit */
    proto_tree_add_uint(tree, hf_snmp_engineid_enterprise, tvb, offset, 4, enterpriseid);
    offset+=4;
    len_remain-=4;

    switch(conformance) {

    case SNMP_ENGINEID_RFC1910:
      /* 12-byte AgentID w/ 8-byte trailer */
      if (len_remain==8) {
	proto_tree_add_text(tree, tvb, offset, 8, "AgentID Trailer: 0x%s",
			    tvb_bytes_to_str(tvb, offset, 8));
	offset+=8;
	len_remain-=8;
      } else {
	proto_tree_add_text(tree, tvb, offset, len_remain, "<Data not conforming to RFC1910>");
	return offset;
      }
      break;

    case SNMP_ENGINEID_RFC3411: /* variable length: 5..32 */

      /* 1-byte format specifier */
      if (len_remain<1) return offset;
      format = tvb_get_guint8(tvb, offset);
      item = proto_tree_add_uint_format(tree, hf_snmp_engineid_format, tvb, offset, 1, format, "Engine ID Format: %s (%d)",
			  val_to_str(format, snmp_engineid_format_vals, "Reserved/Enterprise-specific"), format);
      offset+=1;
      len_remain-=1;

      switch(format) {
      case SNMP_ENGINEID_FORMAT_IPV4:
	/* 4-byte IPv4 address */
	if (len_remain==4) {
	  proto_tree_add_item(tree, hf_snmp_engineid_ipv4, tvb, offset, 4, FALSE);
	  offset+=4;
	  len_remain=0;
	}
	break;
      case SNMP_ENGINEID_FORMAT_IPV6:
	/* 16-byte IPv6 address */
	if (len_remain==16) {
	  proto_tree_add_item(tree, hf_snmp_engineid_ipv6, tvb, offset, 16, FALSE);
	  offset+=16;
	  len_remain=0;
	}
	break;
      case SNMP_ENGINEID_FORMAT_MACADDRESS:
	/* 6-byte MAC address */
	if (len_remain==6) {
	  proto_tree_add_item(tree, hf_snmp_engineid_mac, tvb, offset, 6, FALSE);
	  offset+=6;
	  len_remain=0;
	}
	break;
      case SNMP_ENGINEID_FORMAT_TEXT:
	/* max. 27-byte string, administratively assigned */
	if (len_remain<=27) {
	  proto_tree_add_item(tree, hf_snmp_engineid_text, tvb, offset, len_remain, FALSE);
	  offset+=len_remain;
	  len_remain=0;
	}
	break;
      case 128:
	/* most common enterprise-specific format: (ucd|net)-snmp random */
	if ((enterpriseid==2021)||(enterpriseid==8072)) {
	  proto_item_append_text(item, (enterpriseid==2021) ? ": UCD-SNMP Random" : ": Net-SNMP Random");
	  /* demystify: 4B random, 4B epoch seconds */
	  if (len_remain==8) {
	    proto_tree_add_item(tree, hf_snmp_engineid_data, tvb, offset, 4, FALSE);
	    seconds = tvb_get_letohl(tvb, offset+4);
	    ts.secs = seconds;
	    proto_tree_add_time_format(tree, hf_snmp_engineid_time, tvb, offset+4, 4,
                                  &ts, "Engine ID Data: Creation Time: %s",
                                  abs_time_secs_to_str(seconds));
	    offset+=8;
	    len_remain=0;
	  }
	}
	break;
      case SNMP_ENGINEID_FORMAT_OCTETS:
      default:
	/* max. 27 bytes, administratively assigned or unknown format */
	if (len_remain<=27) {
	  proto_tree_add_item(tree, hf_snmp_engineid_data, tvb, offset, len_remain, FALSE);
	  offset+=len_remain;
	  len_remain=0;
	}
	break;
      }
    }

    if (len_remain>0) {
      proto_tree_add_text(tree, tvb, offset, len_remain, "<Data not conforming to RFC3411>");
      offset+=len_remain;
    }
    return offset;
}

/* This code is copied from the original SNMP dissector with minor changes to adapt it to use packet-ber.c routines
 * TODO:
 * - Rewrite it completly as OID as subid_t could be returned from dissect_ber_objectidentifier
 * - vb_type_name is known when calling this routine(?)
 * - All branches not needed(?)
 * ...
 */

static void
snmp_variable_decode(tvbuff_t *tvb, proto_tree *snmp_tree, packet_info *pinfo,tvbuff_t *oid_tvb,
					 int offset, guint *lengthp, tvbuff_t **out_tvb)
{
	int start, vb_value_start;
	guint length;
	guint vb_length;
	gushort vb_type;
	const gchar *vb_type_name;
	gint32 vb_integer_value;
	guint32 vb_uinteger_value;
	guint8 *vb_octet_string;
	const guint8 *oid_buf;
	subid_t *vb_oid;
	guint vb_oid_length;
	gchar *vb_display_string = NULL;
	subid_t *variable_oid = NULL;
	gint oid_len;
	guint variable_oid_length = 0;
	const guint8 *var_oid_buf;
#ifdef HAVE_NET_SNMP
	struct variable_list variable;
	long value;
#endif
	unsigned int i;
	gchar *buf;
	int len;
	gint8 class;
	gboolean pc, ind = 0;
	gint32 ber_tag;

	start = offset;
	/* parse the type of the object */
	offset = dissect_ber_identifier(pinfo, snmp_tree, tvb, start, &class, &pc, &ber_tag);
	offset = dissect_ber_length(pinfo, snmp_tree, tvb, offset, &vb_length, &ind);

	if(vb_length == 0){
		length = offset - start;
		*lengthp = length;
		return;
	}

	vb_value_start = offset;

	/* Convert the class, constructed flag, and tag to a type. */
	vb_type_name = snmp_tag_cls2syntax(ber_tag, class, &vb_type);

	if (vb_type_name == NULL) {
		/*
		 * Unsupported type.
		 * Dissect the value as an opaque string of octets.
		 */
		vb_type_name = "unsupported type";
		vb_type = SNMP_OPAQUE;
	}
	/* construct subid_t variable_oid from oid_tvb */
	if (oid_tvb){
		oid_len = tvb_length_remaining(oid_tvb,0);
		var_oid_buf = tvb_get_ptr(oid_tvb, 0, oid_len);
		variable_oid = ep_alloc((oid_len+1) * sizeof(gulong));
		variable_oid_length = oid_to_subid_buf(var_oid_buf, oid_len, variable_oid, ((oid_len+1) * sizeof(gulong)));
	}
	/* parse the value */
	switch (vb_type) {

	case SNMP_INTEGER:
		offset = dissect_ber_integer(FALSE, pinfo, NULL, tvb, start, -1, (void*)&(vb_integer_value));
		length = offset - vb_value_start;
		if (snmp_tree) {
#ifdef HAVE_NET_SNMP
			value = vb_integer_value;
			variable.val.integer = &value;
			vb_display_string = format_var(&variable,
			    variable_oid, variable_oid_length, vb_type,
			    vb_length);
#else
			vb_display_string = NULL;
#endif
			if (vb_display_string != NULL) {
				proto_tree_add_text(snmp_tree, tvb,
				    vb_value_start, length,
				    "Value: %s", vb_display_string);
			} else {
				proto_tree_add_text(snmp_tree,tvb,
				    vb_value_start, length,
				    "Value: %s: %d (%#x)", vb_type_name,
				    vb_integer_value, vb_integer_value);
			}
		}
		break;

	case SNMP_COUNTER:
	case SNMP_GAUGE:
	case SNMP_TIMETICKS:
		offset = dissect_ber_integer(FALSE, pinfo, NULL, tvb, start, -1, &vb_uinteger_value);
		length = offset - vb_value_start;
		if (snmp_tree) {
#ifdef HAVE_NET_SNMP
			value = vb_uinteger_value;
			variable.val.integer = &value;
			vb_display_string = format_var(&variable,
			    variable_oid, variable_oid_length, vb_type,
			    vb_length);
#else
			vb_display_string = NULL;
#endif
			if (vb_display_string != NULL) {
				proto_tree_add_text(snmp_tree, tvb,
				    vb_value_start, length,
				    "Value: %s", vb_display_string);
			} else {
				proto_tree_add_text(snmp_tree, tvb,
				    vb_value_start, length,
				    "Value: %s: %u (%#x)", vb_type_name,
				    vb_uinteger_value, vb_uinteger_value);
			}
		}
		break;
	case SNMP_COUNTER64:
		offset=dissect_ber_integer64(TRUE, pinfo, snmp_tree, tvb, offset, hf_snmp_counter64, NULL);
		break;
	case SNMP_OCTETSTR:
	case SNMP_IPADDR:
	case SNMP_OPAQUE:
	case SNMP_NSAP:
	case SNMP_BITSTR:
		offset = dissect_ber_octet_string(FALSE, pinfo, NULL, tvb, start, -1, out_tvb);
		vb_octet_string = ep_tvb_memdup(tvb, vb_value_start, vb_length);

		length = offset - vb_value_start;
		if (snmp_tree) {
#ifdef HAVE_NET_SNMP
			variable.val.string = vb_octet_string;
			vb_display_string = format_var(&variable,
			    variable_oid, variable_oid_length, vb_type,
			    vb_length);
#else
			vb_display_string = NULL;
#endif
			if (vb_display_string != NULL) {
				proto_tree_add_text(snmp_tree, tvb,
				    vb_value_start, length,
				    "Value: %s", vb_display_string);
			} else {
				/*
				 * If some characters are not printable,
				 * display the string as bytes.
				 */
				for (i = 0; i < vb_length; i++) {
					if (!(isprint(vb_octet_string[i])
					    || isspace(vb_octet_string[i])))
						break;
				}
				if (i < vb_length) {
					/*
					 * We stopped, due to a non-printable
					 * character, before we got to the end
					 * of the string.
					 */
					vb_display_string = ep_alloc(4*vb_length);
					buf = vb_display_string;
					len = g_snprintf(buf, 4*vb_length, "%03u", vb_octet_string[0]);
					buf += len;
					for (i = 1; i < vb_length; i++) {
						len = g_snprintf(buf, 4*vb_length-(buf-vb_display_string), ".%03u",
						    vb_octet_string[i]);
						buf += len;
					}
					proto_tree_add_text(snmp_tree, tvb, vb_value_start,
					    length,
					    "Value: %s: %s", vb_type_name,
					    vb_display_string);
				} else {
					proto_tree_add_text(snmp_tree, tvb, vb_value_start,
					    length,
					    "Value: %s: %s", vb_type_name,
					    SAFE_STRING(vb_octet_string, vb_length));
				}
			}
		}
		break;

	case SNMP_NULL:
		dissect_ber_null(FALSE, pinfo, NULL, tvb, start, -1);
		length = offset - vb_value_start;
		if (snmp_tree) {
			proto_tree_add_text(snmp_tree, tvb, vb_value_start, length,
			    "Value: %s", vb_type_name);
		}
		break;

	case SNMP_OBJECTID:
		/* XXX Redo this using dissect_ber_object_identifier when
		   it returns tvb or some other binary form of an OID */
		oid_buf = tvb_get_ptr(tvb, vb_value_start, vb_length);
		vb_oid = ep_alloc((vb_length+1) * sizeof(gulong));
		vb_oid_length = oid_to_subid_buf(oid_buf, vb_length, vb_oid, ((vb_length+1) * sizeof(gulong)));

		offset = offset + vb_length;
		length = offset - vb_value_start;
		if (snmp_tree) {
#ifdef HAVE_NET_SNMP
			variable.val.objid = vb_oid;
			vb_display_string = format_var(&variable,
			    variable_oid, variable_oid_length, vb_type,
			    vb_oid_length * sizeof (subid_t));
			if (vb_display_string != NULL) {
				proto_tree_add_text(snmp_tree, tvb,
				    vb_value_start, length,
				    "Value: %s", vb_display_string);
			} else {
				proto_tree_add_text(snmp_tree, tvb,
				    vb_value_start, length,
				    "Value: %s: [Out of memory]", vb_type_name);
			}
#else /* HAVE_NET_SNMP */
			vb_display_string = format_oid(vb_oid, vb_oid_length);
			if (vb_display_string != NULL) {
				proto_tree_add_text(snmp_tree, tvb,
				    vb_value_start, length,
				    "Value: %s: %s", vb_type_name,
				    vb_display_string);
			} else {
				proto_tree_add_text(snmp_tree, tvb,
				    vb_value_start, length,
				    "Value: %s: [Out of memory]", vb_type_name);
			}
#endif /* HAVE_NET_SNMP */
		}
		break;

	case SNMP_NOSUCHOBJECT:
		length = offset - start;
		if (snmp_tree) {
			proto_tree_add_text(snmp_tree, tvb, offset, length,
			    "Value: %s: no such object", vb_type_name);
		}
		break;

	case SNMP_NOSUCHINSTANCE:
		length = offset - start;
		if (snmp_tree) {
			proto_tree_add_text(snmp_tree, tvb, offset, length,
			    "Value: %s: no such instance", vb_type_name);
		}
		break;

	case SNMP_ENDOFMIBVIEW:
		length = offset - start;
		if (snmp_tree) {
			proto_tree_add_text(snmp_tree, tvb, offset, length,
			    "Value: %s: end of mib view", vb_type_name);
		}
		break;

	default:
		DISSECTOR_ASSERT_NOT_REACHED();
		return;
	}
	length = offset - start;
	*lengthp = length;
	return;
}





#define CACHE_INSERT(c,a) if (c) { snmp_ue_assoc_t* t = c; c = a; c->next = t; } else { c = a; a->next = NULL; }

static void renew_ue_cache() {
	if (ue_assocs) {
		static snmp_ue_assoc_t* a;

		localized_ues = NULL;
		unlocalized_ues = NULL;
		
		for(a = ue_assocs; a->user.userName.data; a++) {
			if (a->engine.data) {
				CACHE_INSERT(localized_ues,a);
			} else {
				CACHE_INSERT(unlocalized_ues,a);
			}
			
		}
	}
}


static snmp_ue_assoc_t* localize_ue( snmp_ue_assoc_t* o, const guint8* engine, guint engine_len ) {
	snmp_ue_assoc_t* n = se_memdup(o,sizeof(snmp_ue_assoc_t));
	guint key_size = n->user.authModel->key_size;
	
	n->engine.data = se_memdup(engine,engine_len);
	n->engine.len = engine_len;
	
	n->user.authKey.data = se_alloc(key_size);
	n->user.authKey.len = key_size;
	n->user.authModel->pass2key(n->user.authPassword.data,
								n->user.authPassword.len,
								engine,
								engine_len,
								n->user.authKey.data);

	n->user.privKey.data = se_alloc(key_size);
	n->user.privKey.len = key_size;
	n->user.authModel->pass2key(n->user.privPassword.data,
								n->user.privPassword.len,
								engine,
								engine_len,
								n->user.privKey.data);

	return n;
}


#define localized_match(a,u,ul,e,el) \
	( a->user.userName.len == ul \
	&& a->engine.len == el \
	&& memcmp( a->user.userName.data, u, (a->user.userName.len < ul) ? a->user.userName.len : ul ) == 0 \
	&& memcmp( a->engine.data,   e, (a->engine.len   < el) ? a->engine.len   : el ) == 0 )

#define unlocalized_match(a,u,l) \
	( a->user.userName.len == l && memcmp( a->user.userName.data, u, a->user.userName.len < l ? a->user.userName.len : l) == 0 )

static snmp_ue_assoc_t* get_user_assoc(tvbuff_t* engine_tvb, tvbuff_t* user_tvb) {
	static snmp_ue_assoc_t* a;
	guint given_username_len;
	guint8* given_username;
	guint given_engine_len;
	guint8* given_engine;
	
	if ( ! (localized_ues || unlocalized_ues ) ) return NULL;

	if (! ( user_tvb && engine_tvb ) ) return NULL;
	
	given_username_len = tvb_length_remaining(user_tvb,0);
	given_username = ep_tvb_memdup(user_tvb,0,-1);
	given_engine_len = tvb_length_remaining(engine_tvb,0);
	given_engine = ep_tvb_memdup(engine_tvb,0,-1);
	
	for (a = localized_ues; a; a = a->next) {
		if ( localized_match(a, given_username, given_username_len, given_engine, given_engine_len) ) {
			return a;
		}
	}
	
	for (a = unlocalized_ues; a; a = a->next) {
		if ( unlocalized_match(a, given_username, given_username_len) ) {
			snmp_ue_assoc_t* n = localize_ue( a, given_engine, given_engine_len );
			CACHE_INSERT(localized_ues,n);
			return n;
		}
	}
	
	return NULL;
}

static void destroy_ue_assocs(snmp_ue_assoc_t* assocs) {
	if (assocs) {
		snmp_ue_assoc_t* a;
		
		for(a = assocs; a->user.userName.data; a++) {
			g_free(a->user.userName.data);
			if (a->user.authKey.data) g_free(a->user.authKey.data);
			if (a->user.privKey.data) g_free(a->user.privKey.data);
			if (a->engine.data) g_free(a->engine.data);
		}
		
		g_free(ue_assocs);
	}
}


gboolean snmp_usm_auth_md5(snmp_usm_params_t* p, gchar const** error) {
	guint msg_len;
	guint8* msg;
	guint auth_len;
	guint8* auth;
	guint8* key;
	guint key_len;
	guint8 calc_auth[16];
	guint start;
	guint end;
	guint i;
	
	if (!p->auth_tvb) {
		*error = "No Authenticator";
		return FALSE;		
	}
	
	key = p->user_assoc->user.authKey.data;
	key_len = p->user_assoc->user.authKey.len;
	
	if (! key ) {
		*error = "User has no authKey";
		return FALSE;
	}
	
	
	auth_len = tvb_length_remaining(p->auth_tvb,0);
	
	if (auth_len != 12) {
		*error = "Authenticator length wrong";
		return FALSE;
	}
	
	msg_len = tvb_length_remaining(p->msg_tvb,0);
	msg = ep_tvb_memdup(p->msg_tvb,0,msg_len);
	

	auth = ep_tvb_memdup(p->auth_tvb,0,auth_len);

	start = p->auth_offset - p->start_offset;
	end = 	start + auth_len;

	/* fill the authenticator with zeros */
	for ( i = start ; i < end ; i++ ) {
		msg[i] = '\0';
	}

	md5_hmac(msg, msg_len, key, key_len, calc_auth);
	
	return ( memcmp(auth,calc_auth,12) != 0 ) ? FALSE : TRUE;
}


gboolean snmp_usm_auth_sha1(snmp_usm_params_t* p _U_, gchar const** error _U_) {
	guint msg_len;
	guint8* msg;
	guint auth_len;
	guint8* auth;
	guint8* key;
	guint key_len;
	guint8 calc_auth[20];
	guint start;
	guint end;
	guint i;
	
	if (!p->auth_tvb) {
		*error = "No Authenticator";
		return FALSE;		
	}
	
	key = p->user_assoc->user.authKey.data;
	key_len = p->user_assoc->user.authKey.len;
	
	if (! key ) {
		*error = "User has no authKey";
		return FALSE;
	}
	
	
	auth_len = tvb_length_remaining(p->auth_tvb,0);
	
	
	if (auth_len != 12) {
		*error = "Authenticator length wrong";
		return FALSE;
	}
	
	msg_len = tvb_length_remaining(p->msg_tvb,0);
	msg = ep_tvb_memdup(p->msg_tvb,0,msg_len);

	auth = ep_tvb_memdup(p->auth_tvb,0,auth_len);
	
	start = p->auth_offset - p->start_offset;
	end = 	start + auth_len;
	
	/* fill the authenticator with zeros */
	for ( i = start ; i < end ; i++ ) {
		msg[i] = '\0';
	}
	
	sha1_hmac(key, key_len, msg, msg_len, calc_auth);
	
	return ( memcmp(auth,calc_auth,12) != 0 ) ? FALSE : TRUE;
}

tvbuff_t* snmp_usm_priv_des(snmp_usm_params_t* p _U_, tvbuff_t* encryptedData _U_, gchar const** error _U_) {
#ifdef HAVE_LIBGCRYPT
    gcry_error_t err;
    gcry_cipher_hd_t hd = NULL;
	
	guint8* cleartext;
	guint8* des_key = p->user_assoc->user.privKey.data; /* first 8 bytes */
	guint8* pre_iv = &(p->user_assoc->user.privKey.data[8]); /* last 8 bytes */
	guint8* salt;
	gint salt_len;
	gint cryptgrm_len;
	guint8* cryptgrm;
	tvbuff_t* clear_tvb;
	guint8 iv[8];
	guint i;
	
	
	salt_len = tvb_length_remaining(p->priv_tvb,0);
	
	if (salt_len != 8)  {
		*error = "decryptionError: msgPrivacyParameters lenght != 8";
		return NULL;
	}	

	salt = ep_tvb_memdup(p->priv_tvb,0,salt_len);

	/*
	 The resulting "salt" is XOR-ed with the pre-IV to obtain the IV.
	 */
	for (i=0; i<8; i++) {
		iv[i] = pre_iv[i] ^ salt[i];
	}

	cryptgrm_len = tvb_length_remaining(encryptedData,0);

	if (cryptgrm_len % 8) {
		*error = "decryptionError: the length of the encrypted data is not a mutiple of 8 octets";
		return NULL;
	}
	
	cryptgrm = ep_tvb_memdup(encryptedData,0,-1);

	cleartext = ep_alloc(cryptgrm_len);
	
	err = gcry_cipher_open(&hd, GCRY_CIPHER_DES, GCRY_CIPHER_MODE_CBC, 0);
	if (err != GPG_ERR_NO_ERROR) goto on_gcry_error;
	
    err = gcry_cipher_setiv(hd, iv, 8);
	if (err != GPG_ERR_NO_ERROR) goto on_gcry_error;
	
	err = gcry_cipher_setkey(hd,des_key,8);
	if (err != GPG_ERR_NO_ERROR) goto on_gcry_error;
	
	err = gcry_cipher_decrypt(hd, cleartext, cryptgrm_len, cryptgrm, cryptgrm_len);
	if (err != GPG_ERR_NO_ERROR) goto on_gcry_error;
	
	gcry_cipher_close(hd);
	
	clear_tvb = tvb_new_real_data(cleartext, cryptgrm_len, cryptgrm_len);
	
	return clear_tvb;
	
on_gcry_error:
	*error = (void*)gpg_strerror(err);
	if (hd) gcry_cipher_close(hd);
	return NULL;
#else
	*error = "libgcrypt not present, cannot decrypt";
	return NULL;
#endif
}

tvbuff_t* snmp_usm_priv_aes(snmp_usm_params_t* p _U_, tvbuff_t* encryptedData _U_, gchar const** error _U_) {
#ifdef HAVE_LIBGCRYPT
    gcry_error_t err;
    gcry_cipher_hd_t hd = NULL;
	
	guint8* cleartext;
	guint8* aes_key = p->user_assoc->user.privKey.data; /* first 16 bytes */
	guint8* iv;
	gint iv_len;
	gint cryptgrm_len;
	guint8* cryptgrm;
	tvbuff_t* clear_tvb;

	iv_len = tvb_length_remaining(p->priv_tvb,0);
	
	if (iv_len != 8)  {
		*error = "decryptionError: msgPrivacyParameters lenght != 8";
		return NULL;
	}	
	
	iv = ep_alloc(16);
	iv[0] = (p->boots & 0xff000000) >> 24;
	iv[1] = (p->boots & 0x00ff0000) >> 16;
	iv[2] = (p->boots & 0x0000ff00) >> 8;
	iv[3] = (p->boots & 0x000000ff);
	iv[4] = (p->time & 0xff000000) >> 24;
	iv[5] = (p->time & 0x00ff0000) >> 16;
	iv[6] = (p->time & 0x0000ff00) >> 8;
	iv[7] = (p->time & 0x000000ff);
	tvb_memcpy(p->priv_tvb,&(iv[8]),0,8);
	
	cryptgrm_len = tvb_length_remaining(encryptedData,0);
	cryptgrm = ep_tvb_memdup(encryptedData,0,-1);
	
	cleartext = ep_alloc(cryptgrm_len);
	
	err = gcry_cipher_open(&hd, GCRY_CIPHER_AES, GCRY_CIPHER_MODE_CFB, 0);
	if (err != GPG_ERR_NO_ERROR) goto on_gcry_error;
	
    err = gcry_cipher_setiv(hd, iv, 16);
	if (err != GPG_ERR_NO_ERROR) goto on_gcry_error;
	
	err = gcry_cipher_setkey(hd,aes_key,16);
	if (err != GPG_ERR_NO_ERROR) goto on_gcry_error;
	
	err = gcry_cipher_decrypt(hd, cleartext, cryptgrm_len, cryptgrm, cryptgrm_len);
	if (err != GPG_ERR_NO_ERROR) goto on_gcry_error;
	
	gcry_cipher_close(hd);
	
	clear_tvb = tvb_new_real_data(cleartext, cryptgrm_len, cryptgrm_len);
	
	return clear_tvb;
	
on_gcry_error:
		*error = (void*)gpg_strerror(err);
	if (hd) gcry_cipher_close(hd);
	return NULL;
#else
	*error = "libgcrypt not present, cannot decrypt";
	return NULL;
#endif
}





/*--- Included file: packet-snmp-fn.c ---*/
#line 1 "packet-snmp-fn.c"
/*--- Fields for imported types ---*/




static int
dissect_snmp_ObjectName(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_object_identifier(implicit_tag, pinfo, tree, tvb, offset, hf_index, &oid_tvb);

  return offset;
}
static int dissect_name(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_ObjectName(FALSE, tvb, offset, pinfo, tree, hf_snmp_name);
}
static int dissect_subtree(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_ObjectName(FALSE, tvb, offset, pinfo, tree, hf_snmp_subtree);
}




static int
dissect_snmp_Integer_value(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
#line 330 "snmp.cnf"
	guint length;
	
	snmp_variable_decode(tvb, tree, pinfo, oid_tvb, offset, &length, NULL);
	offset = offset + length;



  return offset;
}
static int dissect_integer_value(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_Integer_value(FALSE, tvb, offset, pinfo, tree, hf_snmp_integer_value);
}



static int
dissect_snmp_String_value(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
#line 324 "snmp.cnf"
	guint length;
	
	snmp_variable_decode(tvb, tree, pinfo, oid_tvb, offset, &length, &value_tvb);
	offset = offset + length;



  return offset;
}
static int dissect_string_value(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_String_value(FALSE, tvb, offset, pinfo, tree, hf_snmp_string_value);
}



static int
dissect_snmp_ObjectID_value(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
#line 336 "snmp.cnf"
	guint length;
	
	snmp_variable_decode(tvb, tree, pinfo, oid_tvb, offset, &length, NULL);
	offset = offset + length;



  return offset;
}
static int dissect_objectID_value(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_ObjectID_value(FALSE, tvb, offset, pinfo, tree, hf_snmp_objectID_value);
}



static int
dissect_snmp_Empty(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
#line 342 "snmp.cnf"
	guint length;
	
	snmp_variable_decode(tvb, tree, pinfo, oid_tvb, offset, &length, NULL);
	offset = offset + length;





  return offset;
}
static int dissect_empty(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_Empty(FALSE, tvb, offset, pinfo, tree, hf_snmp_empty);
}


static const value_string snmp_SimpleSyntax_vals[] = {
  {   0, "integer-value" },
  {   1, "string-value" },
  {   2, "objectID-value" },
  {   3, "empty" },
  { 0, NULL }
};

static const ber_choice_t SimpleSyntax_choice[] = {
  {   0, BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_integer_value },
  {   1, BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_string_value },
  {   2, BER_CLASS_UNI, BER_UNI_TAG_OID, BER_FLAGS_NOOWNTAG, dissect_objectID_value },
  {   3, BER_CLASS_UNI, BER_UNI_TAG_NULL, BER_FLAGS_NOOWNTAG, dissect_empty },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_snmp_SimpleSyntax(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                                 SimpleSyntax_choice, hf_index, ett_snmp_SimpleSyntax,
                                 NULL);

  return offset;
}
static int dissect_simple(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_SimpleSyntax(FALSE, tvb, offset, pinfo, tree, hf_snmp_simple);
}



static int
dissect_snmp_IpAddress(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                       NULL);

  return offset;
}
static int dissect_ipAddress_value(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_IpAddress(FALSE, tvb, offset, pinfo, tree, hf_snmp_ipAddress_value);
}
static int dissect_internet(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_IpAddress(FALSE, tvb, offset, pinfo, tree, hf_snmp_internet);
}



static int
dissect_snmp_Counter32(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_counter_value(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_Counter32(FALSE, tvb, offset, pinfo, tree, hf_snmp_counter_value);
}



static int
dissect_snmp_TimeTicks(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_timeticks_value(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_TimeTicks(FALSE, tvb, offset, pinfo, tree, hf_snmp_timeticks_value);
}
static int dissect_time_stamp(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_TimeTicks(FALSE, tvb, offset, pinfo, tree, hf_snmp_time_stamp);
}



static int
dissect_snmp_Opaque(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                       NULL);

  return offset;
}
static int dissect_arbitrary_value(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_Opaque(FALSE, tvb, offset, pinfo, tree, hf_snmp_arbitrary_value);
}



static int
dissect_snmp_Counter64(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_big_counter_value(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_Counter64(FALSE, tvb, offset, pinfo, tree, hf_snmp_big_counter_value);
}



static int
dissect_snmp_Unsigned32(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_unsigned_integer_value(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_Unsigned32(FALSE, tvb, offset, pinfo, tree, hf_snmp_unsigned_integer_value);
}


static const value_string snmp_ApplicationSyntax_vals[] = {
  {   0, "ipAddress-value" },
  {   1, "counter-value" },
  {   3, "timeticks-value" },
  {   4, "arbitrary-value" },
  {   6, "big-counter-value" },
  {   2, "unsigned-integer-value" },
  { 0, NULL }
};

static const ber_choice_t ApplicationSyntax_choice[] = {
  {   0, BER_CLASS_APP, 0, BER_FLAGS_NOOWNTAG, dissect_ipAddress_value },
  {   1, BER_CLASS_APP, 1, BER_FLAGS_NOOWNTAG, dissect_counter_value },
  {   3, BER_CLASS_APP, 3, BER_FLAGS_NOOWNTAG, dissect_timeticks_value },
  {   4, BER_CLASS_APP, 4, BER_FLAGS_NOOWNTAG, dissect_arbitrary_value },
  {   6, BER_CLASS_APP, 6, BER_FLAGS_NOOWNTAG, dissect_big_counter_value },
  {   2, BER_CLASS_APP, 2, BER_FLAGS_NOOWNTAG, dissect_unsigned_integer_value },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_snmp_ApplicationSyntax(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                                 ApplicationSyntax_choice, hf_index, ett_snmp_ApplicationSyntax,
                                 NULL);

  return offset;
}
static int dissect_application_wide(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_ApplicationSyntax(FALSE, tvb, offset, pinfo, tree, hf_snmp_application_wide);
}


static const value_string snmp_ObjectSyntax_vals[] = {
  { -1/*choice*/, "simple" },
  { -1/*choice*/, "application-wide" },
  { 0, NULL }
};

static const ber_choice_t ObjectSyntax_choice[] = {
  { -1/*choice*/, BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG, dissect_simple },
  { -1/*choice*/, BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG, dissect_application_wide },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_snmp_ObjectSyntax(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                                 ObjectSyntax_choice, hf_index, ett_snmp_ObjectSyntax,
                                 NULL);

  return offset;
}
static int dissect_value(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_ObjectSyntax(FALSE, tvb, offset, pinfo, tree, hf_snmp_value);
}



static int
dissect_snmp_Integer32(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_bulkPDU_request_id(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_Integer32(FALSE, tvb, offset, pinfo, tree, hf_snmp_bulkPDU_request_id);
}


static const value_string snmp_NetworkAddress_vals[] = {
  {   0, "internet" },
  { 0, NULL }
};

static const ber_choice_t NetworkAddress_choice[] = {
  {   0, BER_CLASS_APP, 0, BER_FLAGS_NOOWNTAG, dissect_internet },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_snmp_NetworkAddress(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                                 NetworkAddress_choice, hf_index, ett_snmp_NetworkAddress,
                                 NULL);

  return offset;
}
static int dissect_agent_addr(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_NetworkAddress(FALSE, tvb, offset, pinfo, tree, hf_snmp_agent_addr);
}



static const value_string snmp_Version_vals[] = {
  {   0, "version-1" },
  {   1, "v2c" },
  {   2, "v2u" },
  {   3, "snmpv3" },
  { 0, NULL }
};


static int
dissect_snmp_Version(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_version(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_Version(FALSE, tvb, offset, pinfo, tree, hf_snmp_version);
}
static int dissect_msgVersion(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_Version(FALSE, tvb, offset, pinfo, tree, hf_snmp_msgVersion);
}



static int
dissect_snmp_OCTET_STRING(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                       NULL);

  return offset;
}
static int dissect_community(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_OCTET_STRING(FALSE, tvb, offset, pinfo, tree, hf_snmp_community);
}
static int dissect_parameters(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_OCTET_STRING(FALSE, tvb, offset, pinfo, tree, hf_snmp_parameters);
}
static int dissect_encrypted(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_OCTET_STRING(FALSE, tvb, offset, pinfo, tree, hf_snmp_encrypted);
}
static int dissect_contextEngineID(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_OCTET_STRING(FALSE, tvb, offset, pinfo, tree, hf_snmp_contextEngineID);
}
static int dissect_contextName(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_OCTET_STRING(FALSE, tvb, offset, pinfo, tree, hf_snmp_contextName);
}
static int dissect_password(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_OCTET_STRING(FALSE, tvb, offset, pinfo, tree, hf_snmp_password);
}



static int
dissect_snmp_INTEGER(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_request_id(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_INTEGER(FALSE, tvb, offset, pinfo, tree, hf_snmp_request_id);
}
static int dissect_error_index(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_INTEGER(FALSE, tvb, offset, pinfo, tree, hf_snmp_error_index);
}
static int dissect_specific_trap(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_INTEGER(FALSE, tvb, offset, pinfo, tree, hf_snmp_specific_trap);
}


static const value_string snmp_T_error_status_vals[] = {
  {   0, "noError" },
  {   1, "tooBig" },
  {   2, "noSuchName" },
  {   3, "badValue" },
  {   4, "readOnly" },
  {   5, "genErr" },
  {   6, "noAccess" },
  {   7, "wrongType" },
  {   8, "wrongLength" },
  {   9, "wrongEncoding" },
  {  10, "wrongValue" },
  {  11, "noCreation" },
  {  12, "inconsistentValue" },
  {  13, "resourceUnavailable" },
  {  14, "commitFailed" },
  {  15, "undoFailed" },
  {  16, "authorizationError" },
  {  17, "notWritable" },
  {  18, "inconsistentName" },
  { 0, NULL }
};


static int
dissect_snmp_T_error_status(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_error_status(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_T_error_status(FALSE, tvb, offset, pinfo, tree, hf_snmp_error_status);
}



static int
dissect_snmp_NULL(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_null(implicit_tag, pinfo, tree, tvb, offset, hf_index);

  return offset;
}
static int dissect_unSpecified(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_NULL(FALSE, tvb, offset, pinfo, tree, hf_snmp_unSpecified);
}
static int dissect_noSuchObject_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_NULL(TRUE, tvb, offset, pinfo, tree, hf_snmp_noSuchObject);
}
static int dissect_noSuchInstance_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_NULL(TRUE, tvb, offset, pinfo, tree, hf_snmp_noSuchInstance);
}
static int dissect_endOfMibView_impl(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_NULL(TRUE, tvb, offset, pinfo, tree, hf_snmp_endOfMibView);
}


static const value_string snmp_ValueType_vals[] = {
  {   0, "value" },
  {   1, "unSpecified" },
  {   2, "noSuchObject" },
  {   3, "noSuchInstance" },
  {   4, "endOfMibView" },
  { 0, NULL }
};

static const ber_choice_t ValueType_choice[] = {
  {   0, BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG, dissect_value },
  {   1, BER_CLASS_UNI, BER_UNI_TAG_NULL, BER_FLAGS_NOOWNTAG, dissect_unSpecified },
  {   2, BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_noSuchObject_impl },
  {   3, BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_noSuchInstance_impl },
  {   4, BER_CLASS_CON, 2, BER_FLAGS_IMPLTAG, dissect_endOfMibView_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_snmp_ValueType(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                                 ValueType_choice, hf_index, ett_snmp_ValueType,
                                 NULL);

  return offset;
}
static int dissect_valueType(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_ValueType(FALSE, tvb, offset, pinfo, tree, hf_snmp_valueType);
}


static const ber_sequence_t VarBind_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_OID, BER_FLAGS_NOOWNTAG, dissect_name },
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_valueType },
  { 0, 0, 0, NULL }
};

static int
dissect_snmp_VarBind(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
#line 310 "snmp.cnf"
	oid_tvb = NULL;
	value_tvb = NULL;
   offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   VarBind_sequence, hf_index, ett_snmp_VarBind);

	if (oid_tvb && value_tvb) {
		next_tvb_add_string(&var_list, value_tvb, (snmp_var_in_tree) ? tree : NULL, 
			variable_oid_dissector_table, 
			oid_to_str(tvb_get_ptr(oid_tvb, 0, tvb_length(oid_tvb)), tvb_length(oid_tvb)));
	}



  return offset;
}
static int dissect_VarBindList_item(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_VarBind(FALSE, tvb, offset, pinfo, tree, hf_snmp_VarBindList_item);
}


static const ber_sequence_t VarBindList_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_VarBindList_item },
};

static int
dissect_snmp_VarBindList(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence_of(implicit_tag, pinfo, tree, tvb, offset,
                                      VarBindList_sequence_of, hf_index, ett_snmp_VarBindList);

  return offset;
}
static int dissect_variable_bindings(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_VarBindList(FALSE, tvb, offset, pinfo, tree, hf_snmp_variable_bindings);
}


static const ber_sequence_t PDU_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_request_id },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_error_status },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_error_index },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_variable_bindings },
  { 0, 0, 0, NULL }
};

static int
dissect_snmp_PDU(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   PDU_sequence, hf_index, ett_snmp_PDU);

  return offset;
}




static int
dissect_snmp_T_get_request(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
#line 37 "snmp.cnf"
 gint8 class;
 gboolean pc, ind_field;
 gint32 tag;
 guint32 len1;

 if(!implicit_tag){
   /* XXX  asn2wrs can not yet handle tagged assignment yes so this
    * XXX is some conformance file magic to work around that bug
    */
    offset = get_ber_identifier(tvb, offset, &class, &pc, &tag);
    offset = get_ber_length(tree, tvb, offset, &len1, &ind_field);
 }
 offset = dissect_snmp_PDU(TRUE, tvb, offset, pinfo, tree, hf_index);



  return offset;
}
static int dissect_get_request(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_T_get_request(FALSE, tvb, offset, pinfo, tree, hf_snmp_get_request);
}




static int
dissect_snmp_T_get_next_request(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
#line 52 "snmp.cnf"
 gint8 class;
 gboolean pc, ind_field;
 gint32 tag;
 guint32 len1;

 if(!implicit_tag){
   /* XXX  asn2wrs can not yet handle tagged assignment yes so this
    * XXX is some conformance file magic to work around that bug
    */
    offset = get_ber_identifier(tvb, offset, &class, &pc, &tag);
    offset = get_ber_length(tree, tvb, offset, &len1, &ind_field);
 }
 offset = dissect_snmp_PDU(TRUE, tvb, offset, pinfo, tree, hf_index);



  return offset;
}
static int dissect_get_next_request(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_T_get_next_request(FALSE, tvb, offset, pinfo, tree, hf_snmp_get_next_request);
}




static int
dissect_snmp_T_get_response(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
#line 67 "snmp.cnf"
 gint8 class;
 gboolean pc, ind_field;
 gint32 tag;
 guint32 len1;

 if(!implicit_tag){
   /* XXX  asn2wrs can not yet handle tagged assignment yes so this
    * XXX is some conformance file magic to work around that bug
    */
    offset = get_ber_identifier(tvb, offset, &class, &pc, &tag);
    offset = get_ber_length(tree, tvb, offset, &len1, &ind_field);
 }
 offset = dissect_snmp_PDU(TRUE, tvb, offset, pinfo, tree, hf_index);



  return offset;
}
static int dissect_get_response(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_T_get_response(FALSE, tvb, offset, pinfo, tree, hf_snmp_get_response);
}




static int
dissect_snmp_T_set_request(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
#line 82 "snmp.cnf"
 gint8 class;
 gboolean pc, ind_field;
 gint32 tag;
 guint32 len1;

 if(!implicit_tag){
   /* XXX  asn2wrs can not yet handle tagged assignment yes so this
    * XXX is some conformance file magic to work around that bug
    */
    offset = get_ber_identifier(tvb, offset, &class, &pc, &tag);
    offset = get_ber_length(tree, tvb, offset, &len1, &ind_field);
 }
 offset = dissect_snmp_PDU(TRUE, tvb, offset, pinfo, tree, hf_index);




  return offset;
}
static int dissect_set_request(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_T_set_request(FALSE, tvb, offset, pinfo, tree, hf_snmp_set_request);
}



static int
dissect_snmp_OBJECT_IDENTIFIER(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_object_identifier(implicit_tag, pinfo, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_enterprise(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_OBJECT_IDENTIFIER(FALSE, tvb, offset, pinfo, tree, hf_snmp_enterprise);
}
static int dissect_identity(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_OBJECT_IDENTIFIER(FALSE, tvb, offset, pinfo, tree, hf_snmp_identity);
}


static const value_string snmp_T_generic_trap_vals[] = {
  {   0, "coldStart" },
  {   1, "warmStart" },
  {   2, "linkDown" },
  {   3, "linkUp" },
  {   4, "authenticationFailure" },
  {   5, "egpNeighborLoss" },
  {   6, "enterpriseSpecific" },
  { 0, NULL }
};


static int
dissect_snmp_T_generic_trap(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_generic_trap(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_T_generic_trap(FALSE, tvb, offset, pinfo, tree, hf_snmp_generic_trap);
}


static const ber_sequence_t Trap_PDU_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_OID, BER_FLAGS_NOOWNTAG, dissect_enterprise },
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_agent_addr },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_generic_trap },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_specific_trap },
  { BER_CLASS_APP, 3, BER_FLAGS_NOOWNTAG, dissect_time_stamp },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_variable_bindings },
  { 0, 0, 0, NULL }
};

static int
dissect_snmp_Trap_PDU(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   Trap_PDU_sequence, hf_index, ett_snmp_Trap_PDU);

  return offset;
}



static int
dissect_snmp_T_trap(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
#line 98 "snmp.cnf"
 gint8 class;
 gboolean pc, ind_field;
 gint32 tag;
 guint32 len1;

 if(!implicit_tag){
   /* XXX  asn2wrs can not yet handle tagged assignment yes so this
    * XXX is some conformance file magic to work around that bug
    */
    offset = get_ber_identifier(tvb, offset, &class, &pc, &tag);
    offset = get_ber_length(tree, tvb, offset, &len1, &ind_field);
 }
 offset = dissect_snmp_Trap_PDU(TRUE, tvb, offset, pinfo, tree, hf_index);



  return offset;
}
static int dissect_trap(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_T_trap(FALSE, tvb, offset, pinfo, tree, hf_snmp_trap);
}



static int
dissect_snmp_INTEGER_0_2147483647(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_msgID(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_INTEGER_0_2147483647(FALSE, tvb, offset, pinfo, tree, hf_snmp_msgID);
}
static int dissect_non_repeaters(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_INTEGER_0_2147483647(FALSE, tvb, offset, pinfo, tree, hf_snmp_non_repeaters);
}
static int dissect_max_repetitions(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_INTEGER_0_2147483647(FALSE, tvb, offset, pinfo, tree, hf_snmp_max_repetitions);
}


static const ber_sequence_t BulkPDU_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_bulkPDU_request_id },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_non_repeaters },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_max_repetitions },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_variable_bindings },
  { 0, 0, 0, NULL }
};

static int
dissect_snmp_BulkPDU(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   BulkPDU_sequence, hf_index, ett_snmp_BulkPDU);

  return offset;
}



static int
dissect_snmp_GetBulkRequest_PDU(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_snmp_BulkPDU(implicit_tag, tvb, offset, pinfo, tree, hf_index);

  return offset;
}



static int
dissect_snmp_T_getBulkRequest(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
#line 113 "snmp.cnf"
 gint8 class;
 gboolean pc, ind_field;
 gint32 tag;
 guint32 len1;

 if(!implicit_tag){
   /* XXX  asn2wrs can not yet handle tagged assignment yes so this
    * XXX is some conformance file magic to work around that bug
    */
    offset = get_ber_identifier(tvb, offset, &class, &pc, &tag);
    offset = get_ber_length(tree, tvb, offset, &len1, &ind_field);
 }
 offset = dissect_snmp_GetBulkRequest_PDU(TRUE, tvb, offset, pinfo, tree, hf_index);



  return offset;
}
static int dissect_getBulkRequest(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_T_getBulkRequest(FALSE, tvb, offset, pinfo, tree, hf_snmp_getBulkRequest);
}



static int
dissect_snmp_InformRequest_PDU(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_snmp_PDU(implicit_tag, tvb, offset, pinfo, tree, hf_index);

  return offset;
}



static int
dissect_snmp_T_informRequest(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
#line 128 "snmp.cnf"
 gint8 class;
 gboolean pc, ind_field;
 gint32 tag;
 guint32 len1;

 if(!implicit_tag){
   /* XXX  asn2wrs can not yet handle tagged assignment yes so this
    * XXX is some conformance file magic to work around that bug
    */
    offset = get_ber_identifier(tvb, offset, &class, &pc, &tag);
    offset = get_ber_length(tree, tvb, offset, &len1, &ind_field);
 }
 offset = dissect_snmp_InformRequest_PDU(TRUE, tvb, offset, pinfo, tree, hf_index);



  return offset;
}
static int dissect_informRequest(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_T_informRequest(FALSE, tvb, offset, pinfo, tree, hf_snmp_informRequest);
}



static int
dissect_snmp_SNMPv2_Trap_PDU(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_snmp_PDU(implicit_tag, tvb, offset, pinfo, tree, hf_index);

  return offset;
}



static int
dissect_snmp_T_sNMPv2_Trap(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
#line 143 "snmp.cnf"
 gint8 class;
 gboolean pc, ind_field;
 gint32 tag;
 guint32 len1;

 if(!implicit_tag){
   /* XXX  asn2wrs can not yet handle tagged assignment yes so this
    * XXX is some conformance file magic to work around that bug
    */
    offset = get_ber_identifier(tvb, offset, &class, &pc, &tag);
    offset = get_ber_length(tree, tvb, offset, &len1, &ind_field);
 }
 offset = dissect_snmp_SNMPv2_Trap_PDU(TRUE, tvb, offset, pinfo, tree, hf_index);



  return offset;
}
static int dissect_sNMPv2_Trap(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_T_sNMPv2_Trap(FALSE, tvb, offset, pinfo, tree, hf_snmp_sNMPv2_Trap);
}



static int
dissect_snmp_Report_PDU(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_snmp_PDU(implicit_tag, tvb, offset, pinfo, tree, hf_index);

  return offset;
}



static int
dissect_snmp_T_report(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
#line 158 "snmp.cnf"
 gint8 class;
 gboolean pc, ind_field;
 gint32 tag;
 guint32 len1;

 if(!implicit_tag){
   /* XXX  asn2wrs can not yet handle tagged assignment yes so this
    * XXX is some conformance file magic to work around that bug
    */
    offset = get_ber_identifier(tvb, offset, &class, &pc, &tag);
    offset = get_ber_length(tree, tvb, offset, &len1, &ind_field);
 }
 offset = dissect_snmp_Report_PDU(TRUE, tvb, offset, pinfo, tree, hf_index);



  return offset;
}
static int dissect_report(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_T_report(FALSE, tvb, offset, pinfo, tree, hf_snmp_report);
}


static const value_string snmp_PDUs_vals[] = {
  {   0, "get-request" },
  {   1, "get-next-request" },
  {   2, "get-response" },
  {   3, "set-request" },
  {   4, "trap" },
  {   5, "getBulkRequest" },
  {   6, "informRequest" },
  {   7, "sNMPv2-Trap" },
  {   8, "report" },
  { 0, NULL }
};

static const ber_choice_t PDUs_choice[] = {
  {   0, BER_CLASS_CON, 0, BER_FLAGS_NOOWNTAG, dissect_get_request },
  {   1, BER_CLASS_CON, 1, BER_FLAGS_NOOWNTAG, dissect_get_next_request },
  {   2, BER_CLASS_CON, 2, BER_FLAGS_NOOWNTAG, dissect_get_response },
  {   3, BER_CLASS_CON, 3, BER_FLAGS_NOOWNTAG, dissect_set_request },
  {   4, BER_CLASS_CON, 4, BER_FLAGS_NOOWNTAG, dissect_trap },
  {   5, BER_CLASS_CON, 5, BER_FLAGS_NOOWNTAG, dissect_getBulkRequest },
  {   6, BER_CLASS_CON, 6, BER_FLAGS_NOOWNTAG, dissect_informRequest },
  {   7, BER_CLASS_CON, 7, BER_FLAGS_NOOWNTAG, dissect_sNMPv2_Trap },
  {   8, BER_CLASS_CON, 8, BER_FLAGS_NOOWNTAG, dissect_report },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_snmp_PDUs(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
#line 30 "snmp.cnf"
gint pdu_type;

  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                                 PDUs_choice, hf_index, ett_snmp_PDUs,
                                 &pdu_type);

	if (check_col(pinfo->cinfo, COL_INFO))
		col_add_str(pinfo->cinfo, COL_INFO, val_to_str(pdu_type, snmp_PDUs_vals,"Unknown PDU type (%u)"));



  return offset;
}
static int dissect_data(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_PDUs(FALSE, tvb, offset, pinfo, tree, hf_snmp_data);
}
static int dissect_v2u_plaintext(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_PDUs(FALSE, tvb, offset, pinfo, tree, hf_snmp_v2u_plaintext);
}
static int dissect_pDUs(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_PDUs(FALSE, tvb, offset, pinfo, tree, hf_snmp_pDUs);
}


static const ber_sequence_t Message_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_version },
  { BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_community },
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_data },
  { 0, 0, 0, NULL }
};

static int
dissect_snmp_Message(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   Message_sequence, hf_index, ett_snmp_Message);

  return offset;
}


static const value_string snmp_T_datav2u_vals[] = {
  {   0, "plaintext" },
  {   1, "encrypted" },
  { 0, NULL }
};

static const ber_choice_t T_datav2u_choice[] = {
  {   0, BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG, dissect_v2u_plaintext },
  {   1, BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_encrypted },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_snmp_T_datav2u(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                                 T_datav2u_choice, hf_index, ett_snmp_T_datav2u,
                                 NULL);

  return offset;
}
static int dissect_datav2u(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_T_datav2u(FALSE, tvb, offset, pinfo, tree, hf_snmp_datav2u);
}


static const ber_sequence_t Messagev2u_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_version },
  { BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_parameters },
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_datav2u },
  { 0, 0, 0, NULL }
};

static int
dissect_snmp_Messagev2u(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   Messagev2u_sequence, hf_index, ett_snmp_Messagev2u);

  return offset;
}




static int
dissect_snmp_T_msgAuthoritativeEngineID(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
#line 185 "snmp.cnf"
	tvbuff_t *parameter_tvb = NULL;

  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                       &usm_p.engine_tvb);
	 if (parameter_tvb) {
		proto_tree* engine_tree = proto_item_add_subtree(get_ber_last_created_item(),ett_engineid);
		dissect_snmp_engineid(engine_tree, usm_p.engine_tvb, 0, tvb_length_remaining(usm_p.engine_tvb,0));
	}



  return offset;
}
static int dissect_msgAuthoritativeEngineID(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_T_msgAuthoritativeEngineID(FALSE, tvb, offset, pinfo, tree, hf_snmp_msgAuthoritativeEngineID);
}



static int
dissect_snmp_T_msgAuthoritativeEngineBoots(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  &usm_p.boots);

  return offset;
}
static int dissect_msgAuthoritativeEngineBoots(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_T_msgAuthoritativeEngineBoots(FALSE, tvb, offset, pinfo, tree, hf_snmp_msgAuthoritativeEngineBoots);
}



static int
dissect_snmp_T_msgAuthoritativeEngineTime(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  &usm_p.time);

  return offset;
}
static int dissect_msgAuthoritativeEngineTime(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_T_msgAuthoritativeEngineTime(FALSE, tvb, offset, pinfo, tree, hf_snmp_msgAuthoritativeEngineTime);
}



static int
dissect_snmp_T_msgUserName(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                       &usm_p.user_tvb);

  return offset;
}
static int dissect_msgUserName(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_T_msgUserName(FALSE, tvb, offset, pinfo, tree, hf_snmp_msgUserName);
}



static int
dissect_snmp_T_msgAuthenticationParameters(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
#line 198 "snmp.cnf"
	offset = dissect_ber_octet_string(FALSE, pinfo, tree, tvb, offset, hf_index, &usm_p.auth_tvb);
	if (usm_p.auth_tvb) {
		usm_p.auth_item = get_ber_last_created_item();
		usm_p.auth_offset = offset_from_real_beginning(usm_p.auth_tvb,0);
	}


  return offset;
}
static int dissect_msgAuthenticationParameters(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_T_msgAuthenticationParameters(FALSE, tvb, offset, pinfo, tree, hf_snmp_msgAuthenticationParameters);
}



static int
dissect_snmp_T_msgPrivacyParameters(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                       &usm_p.priv_tvb);

  return offset;
}
static int dissect_msgPrivacyParameters(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_T_msgPrivacyParameters(FALSE, tvb, offset, pinfo, tree, hf_snmp_msgPrivacyParameters);
}


static const ber_sequence_t UsmSecurityParameters_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_msgAuthoritativeEngineID },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_msgAuthoritativeEngineBoots },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_msgAuthoritativeEngineTime },
  { BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_msgUserName },
  { BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_msgAuthenticationParameters },
  { BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_msgPrivacyParameters },
  { 0, 0, 0, NULL }
};

static int
dissect_snmp_UsmSecurityParameters(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   UsmSecurityParameters_sequence, hf_index, ett_snmp_UsmSecurityParameters);

  return offset;
}



static int
dissect_snmp_INTEGER_484_2147483647(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_msgMaxSize(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_INTEGER_484_2147483647(FALSE, tvb, offset, pinfo, tree, hf_snmp_msgMaxSize);
}



static int
dissect_snmp_T_msgFlags(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
#line 294 "snmp.cnf"
	tvbuff_t *parameter_tvb = NULL;

   offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                       &parameter_tvb);

 if (parameter_tvb){
	guint8 v3_flags = tvb_get_guint8(parameter_tvb, 0);
	proto_tree* flags_tree = proto_item_add_subtree(get_ber_last_created_item(),ett_msgFlags);
	
	proto_tree_add_item(flags_tree, hf_snmp_v3_flags_report, parameter_tvb, 0, 1, FALSE);
	proto_tree_add_item(flags_tree, hf_snmp_v3_flags_crypt, parameter_tvb, 0, 1, FALSE);
	proto_tree_add_item(flags_tree, hf_snmp_v3_flags_auth, parameter_tvb, 0, 1, FALSE);
	
	usm_p.encrypted = v3_flags & TH_CRYPT ? TRUE : FALSE;
	usm_p.authenticated = v3_flags & TH_AUTH ? TRUE : FALSE;
  }



  return offset;
}
static int dissect_msgFlags(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_T_msgFlags(FALSE, tvb, offset, pinfo, tree, hf_snmp_msgFlags);
}



static int
dissect_snmp_T_msgSecurityModel(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  &MsgSecurityModel);

  return offset;
}
static int dissect_msgSecurityModel(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_T_msgSecurityModel(FALSE, tvb, offset, pinfo, tree, hf_snmp_msgSecurityModel);
}


static const ber_sequence_t HeaderData_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_msgID },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_msgMaxSize },
  { BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_msgFlags },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_msgSecurityModel },
  { 0, 0, 0, NULL }
};

static int
dissect_snmp_HeaderData(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   HeaderData_sequence, hf_index, ett_snmp_HeaderData);

  return offset;
}
static int dissect_msgGlobalData(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_HeaderData(FALSE, tvb, offset, pinfo, tree, hf_snmp_msgGlobalData);
}



static int
dissect_snmp_T_msgSecurityParameters(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
#line 239 "snmp.cnf"

	switch(MsgSecurityModel){
		case SNMP_SEC_USM:	/* 3 */		
			offset = dissect_snmp_UsmSecurityParameters(FALSE, tvb, offset+2, pinfo, tree, -1);
			usm_p.user_assoc = get_user_assoc(usm_p.engine_tvb, usm_p.user_tvb);
			break;
		case SNMP_SEC_ANY:	/* 0 */
		case SNMP_SEC_V1:	/* 1 */
		case SNMP_SEC_V2C:	/* 2 */
		default:
			  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                       NULL);

			break;
	}



  return offset;
}
static int dissect_msgSecurityParameters(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_T_msgSecurityParameters(FALSE, tvb, offset, pinfo, tree, hf_snmp_msgSecurityParameters);
}


static const ber_sequence_t ScopedPDU_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_contextEngineID },
  { BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_contextName },
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_data },
  { 0, 0, 0, NULL }
};

static int
dissect_snmp_ScopedPDU(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   ScopedPDU_sequence, hf_index, ett_snmp_ScopedPDU);

  return offset;
}
static int dissect_plaintext(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_ScopedPDU(FALSE, tvb, offset, pinfo, tree, hf_snmp_plaintext);
}



static int
dissect_snmp_T_encryptedPDU(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
#line 207 "snmp.cnf"
	tvbuff_t* crypt_tvb;
	offset = dissect_ber_octet_string(FALSE, pinfo, tree, tvb, offset, hf_snmp_encryptedPDU, &crypt_tvb);

	if( usm_p.encrypted && crypt_tvb
		&& usm_p.user_assoc
		&& usm_p.user_assoc->user.privProtocol ) {
			const gchar* error = NULL;

		tvbuff_t* cleartext_tvb = usm_p.user_assoc->user.privProtocol(&usm_p,
														   crypt_tvb,
														   &error );

		if (! cleartext_tvb) {
			proto_item* item = get_ber_last_created_item();
			expert_add_info_format( pinfo, item, PI_UNDECODED, PI_WARN, "Failed to decrypt encryptedPDU: %s", error );
		} else {
			proto_tree* encryptedpdu_tree;
			proto_item* decrypted_item;
			proto_tree* decrypted_tree;

            add_new_data_source(pinfo, cleartext_tvb, "Decrypted ScopedPDU");
			tvb_set_child_real_data_tvbuff(tvb, cleartext_tvb);
			
			encryptedpdu_tree = proto_item_add_subtree(get_ber_last_created_item(),ett_encryptedPDU);
			decrypted_item = proto_tree_add_item(encryptedpdu_tree, hf_snmp_decryptedPDU,cleartext_tvb,0,-1,FALSE);
			decrypted_tree = proto_item_add_subtree(decrypted_item,ett_decrypted);
			dissect_snmp_ScopedPDU(FALSE, cleartext_tvb, 0, pinfo, decrypted_tree, -1);
		 }

	}



  return offset;
}
static int dissect_encryptedPDU(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_T_encryptedPDU(FALSE, tvb, offset, pinfo, tree, hf_snmp_encryptedPDU);
}


static const value_string snmp_ScopedPduData_vals[] = {
  {   0, "plaintext" },
  {   1, "encryptedPDU" },
  { 0, NULL }
};

static const ber_choice_t ScopedPduData_choice[] = {
  {   0, BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_plaintext },
  {   1, BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_encryptedPDU },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_snmp_ScopedPduData(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                                 ScopedPduData_choice, hf_index, ett_snmp_ScopedPduData,
                                 NULL);

  return offset;
}
static int dissect_msgData(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_ScopedPduData(FALSE, tvb, offset, pinfo, tree, hf_snmp_msgData);
}


static const ber_sequence_t SNMPv3Message_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_msgVersion },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_msgGlobalData },
  { BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_msgSecurityParameters },
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_msgData },
  { 0, 0, 0, NULL }
};

static int
dissect_snmp_SNMPv3Message(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   SNMPv3Message_sequence, hf_index, ett_snmp_SNMPv3Message);

#line 254 "snmp.cnf"

	if( usm_p.authenticated
		&& usm_p.user_assoc
		&& usm_p.user_assoc->user.authModel ) {
		const gchar* error = NULL;
		proto_item* authen_item;
		proto_tree* authen_tree = proto_item_add_subtree(usm_p.auth_item,ett_authParameters);
		
		usm_p.authOK = usm_p.user_assoc->user.authModel->authenticate(
													 &usm_p,
                                                     &error );

		if (error) {
			authen_item = proto_tree_add_text(authen_tree,tvb,0,0,"Error while verifying Messsage authenticity: %s", error);
			PROTO_ITEM_SET_GENERATED(authen_item);
			expert_add_info_format( pinfo, authen_item, PI_MALFORMED, PI_ERROR, "Error while verifying Messsage authenticity: %s", error );
		} else {
			int severity;
			gchar* fmt;			

			authen_item = proto_tree_add_boolean(authen_tree, hf_snmp_msgAuthentication, tvb, 0, 0, usm_p.authOK);
			PROTO_ITEM_SET_GENERATED(authen_item);
			
			if (usm_p.authOK) {
				fmt = "SNMP Authentication OK";
				severity = PI_CHAT;
			} else {
				fmt = "SNMP Authentication Error";
				severity = PI_WARN;
			}

			expert_add_info_format( pinfo, authen_item, PI_CHECKSUM, severity, fmt );
		}
	}


  return offset;
}


static const value_string snmp_T_smux_version_vals[] = {
  {   0, "version-1" },
  { 0, NULL }
};


static int
dissect_snmp_T_smux_version(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_smux_version(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_T_smux_version(FALSE, tvb, offset, pinfo, tree, hf_snmp_smux_version);
}



static int
dissect_snmp_DisplayString(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                       NULL);

  return offset;
}
static int dissect_description(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_DisplayString(FALSE, tvb, offset, pinfo, tree, hf_snmp_description);
}


static const ber_sequence_t SimpleOpen_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_smux_version },
  { BER_CLASS_UNI, BER_UNI_TAG_OID, BER_FLAGS_NOOWNTAG, dissect_identity },
  { BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_description },
  { BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_password },
  { 0, 0, 0, NULL }
};

static int
dissect_snmp_SimpleOpen(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   SimpleOpen_sequence, hf_index, ett_snmp_SimpleOpen);

  return offset;
}
static int dissect_smux_simple(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_SimpleOpen(FALSE, tvb, offset, pinfo, tree, hf_snmp_smux_simple);
}


static const value_string snmp_OpenPDU_vals[] = {
  {   0, "smux-simple" },
  { 0, NULL }
};

static const ber_choice_t OpenPDU_choice[] = {
  {   0, BER_CLASS_APP, 0, BER_FLAGS_NOOWNTAG, dissect_smux_simple },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_snmp_OpenPDU(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                                 OpenPDU_choice, hf_index, ett_snmp_OpenPDU,
                                 NULL);

  return offset;
}
static int dissect_open(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_OpenPDU(FALSE, tvb, offset, pinfo, tree, hf_snmp_open);
}


static const value_string snmp_ClosePDU_vals[] = {
  {   0, "goingDown" },
  {   1, "unsupportedVersion" },
  {   2, "packetFormat" },
  {   3, "protocolError" },
  {   4, "internalError" },
  {   5, "authenticationFailure" },
  { 0, NULL }
};


static int
dissect_snmp_ClosePDU(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_close(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_ClosePDU(FALSE, tvb, offset, pinfo, tree, hf_snmp_close);
}



static int
dissect_snmp_INTEGER_M1_2147483647(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_priority(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_INTEGER_M1_2147483647(FALSE, tvb, offset, pinfo, tree, hf_snmp_priority);
}


static const value_string snmp_T_operation_vals[] = {
  {   0, "delete" },
  {   1, "readOnly" },
  {   2, "readWrite" },
  { 0, NULL }
};


static int
dissect_snmp_T_operation(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_operation(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_T_operation(FALSE, tvb, offset, pinfo, tree, hf_snmp_operation);
}


static const ber_sequence_t RReqPDU_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_OID, BER_FLAGS_NOOWNTAG, dissect_subtree },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_priority },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_operation },
  { 0, 0, 0, NULL }
};

static int
dissect_snmp_RReqPDU(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, pinfo, tree, tvb, offset,
                                   RReqPDU_sequence, hf_index, ett_snmp_RReqPDU);

  return offset;
}
static int dissect_registerRequest(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_RReqPDU(FALSE, tvb, offset, pinfo, tree, hf_snmp_registerRequest);
}


static const value_string snmp_RRspPDU_vals[] = {
  {  -1, "failure" },
  { 0, NULL }
};


static int
dissect_snmp_RRspPDU(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_rRspPDU(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_RRspPDU(FALSE, tvb, offset, pinfo, tree, hf_snmp_rRspPDU);
}


static const value_string snmp_RegisterResponse_vals[] = {
  {   0, "rRspPDU" },
  {   1, "pDUs" },
  { 0, NULL }
};

static const ber_choice_t RegisterResponse_choice[] = {
  {   0, BER_CLASS_APP, 3, BER_FLAGS_NOOWNTAG, dissect_rRspPDU },
  {   1, BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG, dissect_pDUs },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_snmp_RegisterResponse(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                                 RegisterResponse_choice, hf_index, ett_snmp_RegisterResponse,
                                 NULL);

  return offset;
}
static int dissect_registerResponse(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_RegisterResponse(FALSE, tvb, offset, pinfo, tree, hf_snmp_registerResponse);
}


static const value_string snmp_SOutPDU_vals[] = {
  {   0, "commit" },
  {   1, "rollback" },
  { 0, NULL }
};


static int
dissect_snmp_SOutPDU(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, pinfo, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_commitOrRollback(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset) {
  return dissect_snmp_SOutPDU(FALSE, tvb, offset, pinfo, tree, hf_snmp_commitOrRollback);
}


static const value_string snmp_SMUX_PDUs_vals[] = {
  {   0, "open" },
  {   1, "close" },
  {   2, "registerRequest" },
  {   3, "registerResponse" },
  {   4, "commitOrRollback" },
  { 0, NULL }
};

static const ber_choice_t SMUX_PDUs_choice[] = {
  {   0, BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG, dissect_open },
  {   1, BER_CLASS_APP, 1, BER_FLAGS_NOOWNTAG, dissect_close },
  {   2, BER_CLASS_APP, 2, BER_FLAGS_NOOWNTAG, dissect_registerRequest },
  {   3, BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG, dissect_registerResponse },
  {   4, BER_CLASS_APP, 4, BER_FLAGS_NOOWNTAG, dissect_commitOrRollback },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_snmp_SMUX_PDUs(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_) {
  offset = dissect_ber_choice(pinfo, tree, tvb, offset,
                                 SMUX_PDUs_choice, hf_index, ett_snmp_SMUX_PDUs,
                                 NULL);

  return offset;
}

/*--- PDUs ---*/

static void dissect_SMUX_PDUs_PDU(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree) {
  dissect_snmp_SMUX_PDUs(FALSE, tvb, 0, pinfo, tree, hf_snmp_SMUX_PDUs_PDU);
}


/*--- End of included file: packet-snmp-fn.c ---*/
#line 1413 "packet-snmp-template.c"


guint
dissect_snmp_pdu(tvbuff_t *tvb, int offset, packet_info *pinfo,
    proto_tree *tree, int proto, gint ett, gboolean is_tcp)
{

	guint length_remaining;
	gint8 class;
	gboolean pc, ind = 0;
	gint32 tag;
	guint32 len;
	guint message_length;
	int start_offset = offset;
	guint32 version = 0;

	proto_tree *snmp_tree = NULL;
	proto_item *item = NULL;

	usm_p.msg_tvb = tvb;
	usm_p.start_offset = offset_from_real_beginning(tvb,0) ;
	usm_p.engine_tvb = NULL;
	usm_p.user_tvb = NULL;
	usm_p.auth_item = NULL;
	usm_p.auth_tvb = NULL;
	usm_p.auth_offset = 0;
	usm_p.priv_tvb = NULL;
	usm_p.user_assoc = NULL;
	usm_p.authenticated = FALSE;
	usm_p.encrypted = FALSE;
	usm_p.boots = 0;
	usm_p.time = 0;
	usm_p.authOK = FALSE;
	
	/*
	 * This will throw an exception if we don't have any data left.
	 * That's what we want.  (See "tcp_dissect_pdus()", which is
	 * similar, but doesn't have to deal with ASN.1.
	 * XXX - can we make "tcp_dissect_pdus()" provide enough
	 * information to the "get_pdu_len" routine so that we could
	 * have that routine deal with ASN.1, and just use
	 * "tcp_dissect_pdus()"?)
	 */
	length_remaining = tvb_ensure_length_remaining(tvb, offset);

	/* NOTE: we have to parse the message piece by piece, since the
	 * capture length may be less than the message length: a 'global'
	 * parsing is likely to fail.
	 */

	/*
	 * If this is SNMP-over-TCP, we might have to do reassembly
	 * in order to read the "Sequence Of" header.
	 */
	if (is_tcp && snmp_desegment && pinfo->can_desegment) {
		/*
		 * This is TCP, and we should, and can, do reassembly.
		 *
		 * Is the "Sequence Of" header split across segment
		 * boundaries?  We requre at least 6 bytes for the
		 * header, which allows for a 4-byte length (ASN.1
		 * BER).
		 */
		if (length_remaining < 6) {
			pinfo->desegment_offset = offset;
			pinfo->desegment_len = 6 - length_remaining;

			/*
			 * Return 0, which means "I didn't dissect anything
			 * because I don't have enough data - we need
			 * to desegment".
			 */
			return 0;
		}
	}

	/*
	 * OK, try to read the "Sequence Of" header; this gets the total
	 * length of the SNMP message.
	 */
	/* Set tree to 0 to not display internakl BER fields if option used.*/
	offset = dissect_ber_identifier(pinfo, 0, tvb, offset, &class, &pc, &tag);
	offset = dissect_ber_length(pinfo, 0, tvb, offset, &len, &ind);

	message_length = len + 2;
	offset = dissect_ber_integer(FALSE, pinfo, 0, tvb, offset, -1, &version);


	/*
	 * If this is SNMP-over-TCP, we might have to do reassembly
	 * to get all of this message.
	 */
	if (is_tcp && snmp_desegment && pinfo->can_desegment) {
		/*
		 * Yes - is the message split across segment boundaries?
		 */
		if (length_remaining < message_length) {
			/*
			 * Yes.  Tell the TCP dissector where the data
			 * for this message starts in the data it handed
			 * us, and how many more bytes we need, and
			 * return.
			 */
			pinfo->desegment_offset = start_offset;
			pinfo->desegment_len =
			    message_length - length_remaining;

			/*
			 * Return 0, which means "I didn't dissect anything
			 * because I don't have enough data - we need
			 * to desegment".
			 */
			return 0;
		}
	}

	next_tvb_init(&var_list);

	if (check_col(pinfo->cinfo, COL_PROTOCOL)) {
		col_set_str(pinfo->cinfo, COL_PROTOCOL,
		    proto_get_protocol_short_name(find_protocol_by_id(proto)));
	}

	if (tree) {
		item = proto_tree_add_item(tree, proto, tvb, offset,
		    message_length, FALSE);
		snmp_tree = proto_item_add_subtree(item, ett);
	}

	switch (version){
	case 0: /* v1 */
	case 1: /* v2c */
		offset = dissect_snmp_Message(FALSE , tvb, start_offset, pinfo, snmp_tree, -1);
		break;
	case 2: /* v2u */
		offset = dissect_snmp_Messagev2u(FALSE , tvb, start_offset, pinfo, snmp_tree, -1);
		break;
			/* v3 */
	case 3:
		offset = dissect_snmp_SNMPv3Message(FALSE , tvb, start_offset, pinfo, snmp_tree, -1);
		break;
	default:
		/*
		 * Return the length remaining in the tvbuff, so
		 * if this is SNMP-over-TCP, our caller thinks there's
		 * nothing left to dissect.
		 */
		proto_tree_add_text(snmp_tree, tvb, offset, -1,"Unknown version");
		return length_remaining;
		break;
	}

	next_tvb_call(&var_list, pinfo, tree, NULL, data_handle);

	return offset;
}

static gint
dissect_snmp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	conversation_t  *conversation;
	int offset;
	gint8 tmp_class;
	gboolean tmp_pc;
	gint32 tmp_tag;
	guint32 tmp_length;
	gboolean tmp_ind;

	/*
	 * See if this looks like SNMP or not. if not, return 0 so
	 * wireshark can try som other dissector instead.
	 */
	/* All SNMP packets are BER encoded and consist of a SEQUENCE
	 * that spans the entire PDU. The first item is an INTEGER that
	 * has the values 0-2 (version 1-3).
	 * if not it is not snmp.
	 */
	/* SNMP starts with a SEQUENCE */
	offset = get_ber_identifier(tvb, 0, &tmp_class, &tmp_pc, &tmp_tag);
	if((tmp_class!=BER_CLASS_UNI)||(tmp_tag!=BER_UNI_TAG_SEQUENCE)){
		return 0;
	}
	/* then comes a length which spans the rest of the tvb */
	offset = get_ber_length(NULL, tvb, offset, &tmp_length, &tmp_ind);
	if(tmp_length!=(guint32)tvb_reported_length_remaining(tvb, offset)){
		return 0;
	}
	/* then comes an INTEGER (version)*/
	offset = get_ber_identifier(tvb, offset, &tmp_class, &tmp_pc, &tmp_tag);
	if((tmp_class!=BER_CLASS_UNI)||(tmp_tag!=BER_UNI_TAG_INTEGER)){
		return 0;
	}
	/* do we need to test that version is 0 - 2 (version1-3) ? */


	/*
	 * The first SNMP packet goes to the SNMP port; the second one
	 * may come from some *other* port, but goes back to the same
	 * IP address and port as the ones from which the first packet
	 * came; all subsequent packets presumably go between those two
	 * IP addresses and ports.
	 *
	 * If this packet went to the SNMP port, we check to see if
	 * there's already a conversation with one address/port pair
	 * matching the source IP address and port of this packet,
	 * the other address matching the destination IP address of this
	 * packet, and any destination port.
	 *
	 * If not, we create one, with its address 1/port 1 pair being
	 * the source address/port of this packet, its address 2 being
	 * the destination address of this packet, and its port 2 being
	 * wildcarded, and give it the SNMP dissector as a dissector.
	 */
	if (pinfo->destport == UDP_PORT_SNMP) {
	  conversation = find_conversation(pinfo->fd->num, &pinfo->src, &pinfo->dst, PT_UDP,
					   pinfo->srcport, 0, NO_PORT_B);
	  if( (conversation == NULL) || (conversation->dissector_handle!=snmp_handle) ){
	    conversation = conversation_new(pinfo->fd->num, &pinfo->src, &pinfo->dst, PT_UDP,
					    pinfo->srcport, 0, NO_PORT2);
	    conversation_set_dissector(conversation, snmp_handle);
	  }
	}

	return dissect_snmp_pdu(tvb, 0, pinfo, tree, proto_snmp, ett_snmp, FALSE);
}
static void
dissect_snmp_tcp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	int offset = 0;
	guint message_len;

	while (tvb_reported_length_remaining(tvb, offset) > 0) {
		message_len = dissect_snmp_pdu(tvb, 0, pinfo, tree,
		    proto_snmp, ett_snmp, TRUE);
		if (message_len == 0) {
			/*
			 * We don't have all the data for that message,
			 * so we need to do desegmentation;
			 * "dissect_snmp_pdu()" has set that up.
			 */
			break;
		}
		offset += message_len;
	}
}

static void
dissect_smux(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	proto_tree *smux_tree = NULL;
	proto_item *item = NULL;

	if (check_col(pinfo->cinfo, COL_PROTOCOL))
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "SMUX");

	if (tree) {
		item = proto_tree_add_item(tree, proto_smux, tvb, 0, -1, FALSE);
		smux_tree = proto_item_add_subtree(item, ett_smux);
	}

	dissect_SMUX_PDUs_PDU(tvb, pinfo, tree);
}


/*
  MD5 Password to Key Algorithm
  from RFC 3414 A.2.1 
*/
void snmp_usm_password_to_key_md5(const guint8 *password,
								  guint   passwordlen,
								  const guint8 *engineID,
								  guint   engineLength,
								  guint8 *key)  {
	md5_state_t     MD;
	guint8     *cp, password_buf[64];
	guint32      password_index = 0;
	guint32      count = 0, i;
	guint8		key1[16];
	md5_init(&MD);   /* initialize MD5 */
	
	/**********************************************/
	/* Use while loop until we've done 1 Megabyte */
	/**********************************************/
	while (count < 1048576) {
		cp = password_buf;
		for (i = 0; i < 64; i++) {
			/*************************************************/
			/* Take the next octet of the password, wrapping */
			/* to the beginning of the password as necessary.*/
			/*************************************************/
			*cp++ = password[password_index++ % passwordlen];
		}
		md5_append(&MD, password_buf, 64);
		count += 64;
	}
	md5_finish(&MD, key1);          /* tell MD5 we're done */
	
	/*****************************************************/
	/* Now localize the key with the engineID and pass   */
	/* through MD5 to produce final key                  */
	/* May want to ensure that engineLength <= 32,       */
	/* otherwise need to use a buffer larger than 64     */
	/*****************************************************/
	
	md5_init(&MD);
	md5_append(&MD, key1, 16);
	md5_append(&MD, engineID, engineLength);
	md5_append(&MD, key1, 16);
	md5_finish(&MD, key);
	
	return;
}


						 
						 
/*
   SHA1 Password to Key Algorithm COPIED from RFC 3414 A.2.2
 */

void snmp_usm_password_to_key_sha1(const guint8 *password, 
								   guint   passwordlen,
								   const guint8 *engineID,
								   guint   engineLength,
								   guint8 *key ) {
	sha1_context     SH;
	guint8     *cp, password_buf[72];
	guint32      password_index = 0;
	guint32      count = 0, i;
	
	sha1_starts(&SH);   /* initialize SHA */
	
	/**********************************************/
	/* Use while loop until we've done 1 Megabyte */
	/**********************************************/
	while (count < 1048576) {
		cp = password_buf;
		for (i = 0; i < 64; i++) {
			/*************************************************/
			/* Take the next octet of the password, wrapping */
			/* to the beginning of the password as necessary.*/
			/*************************************************/
			*cp++ = password[password_index++ % passwordlen];
		}
		sha1_update (&SH, password_buf, 64);
		count += 64;
	}
	sha1_finish(&SH, key);
	
	/*****************************************************/
	/* Now localize the key with the engineID and pass   */
	/* through SHA to produce final key                  */
	/* May want to ensure that engineLength <= 32,       */
	/* otherwise need to use a buffer larger than 72     */
	/*****************************************************/
	memcpy(password_buf, key, 20);
	memcpy(password_buf+20, engineID, engineLength);
	memcpy(password_buf+20+engineLength, key, 20);
	
	sha1_starts(&SH);
	sha1_update(&SH, password_buf, 40+engineLength);
	sha1_finish(&SH, key);
	return;
 }

									   
static void
process_prefs(void)
{
#ifdef HAVE_NET_SNMP
	gchar *tmp_mib_modules;
	static gboolean mibs_loaded = FALSE;

	if (mibs_loaded) {
		/*
		 * Unload the MIBs, as we'll be reloading them based on
		 * the current preference setting.
		 */
		shutdown_mib();	/* unload MIBs */
	}

	/*
	 * Cannot check if MIBS is already set, as it could be set by Wireshark.
	 *
	 * If we have a list of modules to load, put that list in MIBS,
	 * otherwise clear MIBS.
	 */
	if (mib_modules != NULL) {
		tmp_mib_modules = g_strconcat("MIBS=", mib_modules, NULL);
		/*
		 * Try to be clever and replace colons for semicolons under
		 * Windows.  Do the converse on non-Windows systems.  This
		 * handles cases where we've copied a preferences file
		 * between a non-Windows box and a Windows box or upgraded
		 * from an older version of Wireshark under Windows.
		 */
		g_strdelimit(tmp_mib_modules, IMPORT_SEPARATOR, ENV_SEPARATOR_CHAR);

#ifdef _WIN32
		_putenv(tmp_mib_modules);
#else
		putenv(tmp_mib_modules);
#endif /*_WIN32*/
	} else {
#ifdef _WIN32
		_putenv("MIBS");
#else
		putenv("MIBS");
#endif  /* _WIN32 */
	}

	/*
	 * Load the MIBs.
	 */
	register_mib_handlers();
	read_premib_configs();
	init_mib();
	read_configs();
	mibs_loaded = TRUE;
#endif /* HAVE_NET_SNMP */
	
	if ( g_str_equal(ue_assocs_filename_loaded,ue_assocs_filename) ) return;
	ue_assocs_filename_loaded = ue_assocs_filename;
	
	if (ue_assocs) destroy_ue_assocs(ue_assocs);
	
	if ( *ue_assocs_filename ) {
		gchar* err = load_snmp_users_file(ue_assocs_filename,&ue_assocs);
		if (err) report_failure("Error while loading SNMP's users file:\n%s",err);
	} else {
		ue_assocs = NULL;
	}
}
	
	
	
	/*--- proto_register_snmp -------------------------------------------*/
void proto_register_snmp(void) {	
#if defined(_WIN32) && defined(HAVE_NET_SNMP)
	char *mib_path;
	int mib_path_len;
#define MIB_PATH_APPEND "snmp\\mibs"
#endif
	gchar *tmp_mib_modules;
		
  /* List of fields */
  static hf_register_info hf[] = {
		{ &hf_snmp_v3_flags_auth,
		{ "Authenticated", "snmp.v3.flags.auth", FT_BOOLEAN, 8,
		    TFS(&flags_set_truth), TH_AUTH, "", HFILL }},
		{ &hf_snmp_v3_flags_crypt,
		{ "Encrypted", "snmp.v3.flags.crypt", FT_BOOLEAN, 8,
		    TFS(&flags_set_truth), TH_CRYPT, "", HFILL }},
		{ &hf_snmp_v3_flags_report,
		{ "Reportable", "snmp.v3.flags.report", FT_BOOLEAN, 8,
		    TFS(&flags_set_truth), TH_REPORT, "", HFILL }},
		{ &hf_snmp_engineid_conform, {
		    "Engine ID Conformance", "snmp.engineid.conform", FT_BOOLEAN, 8,
		    TFS(&tfs_snmp_engineid_conform), F_SNMP_ENGINEID_CONFORM, "Engine ID RFC3411 Conformance", HFILL }},
		{ &hf_snmp_engineid_enterprise, {
		    "Engine Enterprise ID", "snmp.engineid.enterprise", FT_UINT32, BASE_DEC,
		    VALS(sminmpec_values), 0, "Engine Enterprise ID", HFILL }},
		{ &hf_snmp_engineid_format, {
		    "Engine ID Format", "snmp.engineid.format", FT_UINT8, BASE_DEC,
		    VALS(snmp_engineid_format_vals), 0, "Engine ID Format", HFILL }},
		{ &hf_snmp_engineid_ipv4, {
		    "Engine ID Data: IPv4 address", "snmp.engineid.ipv4", FT_IPv4, BASE_NONE,
		    NULL, 0, "Engine ID Data: IPv4 address", HFILL }},
		{ &hf_snmp_engineid_ipv6, {
		    "Engine ID Data: IPv6 address", "snmp.engineid.ipv6", FT_IPv6, BASE_NONE,
		    NULL, 0, "Engine ID Data: IPv6 address", HFILL }},
		{ &hf_snmp_engineid_mac, {
		    "Engine ID Data: MAC address", "snmp.engineid.mac", FT_ETHER, BASE_NONE,
		    NULL, 0, "Engine ID Data: MAC address", HFILL }},
		{ &hf_snmp_engineid_text, {
		    "Engine ID Data: Text", "snmp.engineid.text", FT_STRING, BASE_NONE,
		    NULL, 0, "Engine ID Data: Text", HFILL }},
		{ &hf_snmp_engineid_time, {
		    "Engine ID Data: Time", "snmp.engineid.time", FT_ABSOLUTE_TIME, BASE_NONE,
		    NULL, 0, "Engine ID Data: Time", HFILL }},
		{ &hf_snmp_engineid_data, {
		    "Engine ID Data", "snmp.engineid.data", FT_BYTES, BASE_HEX,
		    NULL, 0, "Engine ID Data", HFILL }},
		{ &hf_snmp_counter64, {
		    "Value", "snmp.counter64", FT_INT64, BASE_DEC,
		    NULL, 0, "A counter64 value", HFILL }},
		  { &hf_snmp_msgAuthentication,
				{ "Authentication OK", "snmp.v3.authOK", FT_BOOLEAN, 8,
					TFS(&auth_flags), 0, "", HFILL }},
		  { &hf_snmp_decryptedPDU, {
					"Decrypted ScopedPDU", "snmp.decrypted_pdu", FT_BYTES, BASE_HEX,
					NULL, 0, "Decrypted PDU", HFILL }},
	  

/*--- Included file: packet-snmp-hfarr.c ---*/
#line 1 "packet-snmp-hfarr.c"
    { &hf_snmp_SMUX_PDUs_PDU,
      { "SMUX-PDUs", "snmp.SMUX_PDUs",
        FT_UINT32, BASE_DEC, VALS(snmp_SMUX_PDUs_vals), 0,
        "snmp.SMUX_PDUs", HFILL }},
    { &hf_snmp_simple,
      { "simple", "snmp.simple",
        FT_UINT32, BASE_DEC, VALS(snmp_SimpleSyntax_vals), 0,
        "snmp.SimpleSyntax", HFILL }},
    { &hf_snmp_application_wide,
      { "application-wide", "snmp.application_wide",
        FT_UINT32, BASE_DEC, VALS(snmp_ApplicationSyntax_vals), 0,
        "snmp.ApplicationSyntax", HFILL }},
    { &hf_snmp_integer_value,
      { "integer-value", "snmp.integer_value",
        FT_INT32, BASE_DEC, NULL, 0,
        "snmp.Integer_value", HFILL }},
    { &hf_snmp_string_value,
      { "string-value", "snmp.string_value",
        FT_BYTES, BASE_HEX, NULL, 0,
        "snmp.String_value", HFILL }},
    { &hf_snmp_objectID_value,
      { "objectID-value", "snmp.objectID_value",
        FT_OID, BASE_NONE, NULL, 0,
        "snmp.ObjectID_value", HFILL }},
    { &hf_snmp_empty,
      { "empty", "snmp.empty",
        FT_NONE, BASE_NONE, NULL, 0,
        "snmp.Empty", HFILL }},
    { &hf_snmp_ipAddress_value,
      { "ipAddress-value", "snmp.ipAddress_value",
        FT_IPv4, BASE_NONE, NULL, 0,
        "snmp.IpAddress", HFILL }},
    { &hf_snmp_counter_value,
      { "counter-value", "snmp.counter_value",
        FT_UINT32, BASE_DEC, NULL, 0,
        "snmp.Counter32", HFILL }},
    { &hf_snmp_timeticks_value,
      { "timeticks-value", "snmp.timeticks_value",
        FT_UINT32, BASE_DEC, NULL, 0,
        "snmp.TimeTicks", HFILL }},
    { &hf_snmp_arbitrary_value,
      { "arbitrary-value", "snmp.arbitrary_value",
        FT_BYTES, BASE_HEX, NULL, 0,
        "snmp.Opaque", HFILL }},
    { &hf_snmp_big_counter_value,
      { "big-counter-value", "snmp.big_counter_value",
        FT_UINT64, BASE_DEC, NULL, 0,
        "snmp.Counter64", HFILL }},
    { &hf_snmp_unsigned_integer_value,
      { "unsigned-integer-value", "snmp.unsigned_integer_value",
        FT_UINT32, BASE_DEC, NULL, 0,
        "snmp.Unsigned32", HFILL }},
    { &hf_snmp_internet,
      { "internet", "snmp.internet",
        FT_IPv4, BASE_NONE, NULL, 0,
        "snmp.IpAddress", HFILL }},
    { &hf_snmp_version,
      { "version", "snmp.version",
        FT_INT32, BASE_DEC, VALS(snmp_Version_vals), 0,
        "snmp.Version", HFILL }},
    { &hf_snmp_community,
      { "community", "snmp.community",
        FT_STRING, BASE_HEX, NULL, 0,
        "snmp.OCTET_STRING", HFILL }},
    { &hf_snmp_data,
      { "data", "snmp.data",
        FT_UINT32, BASE_DEC, VALS(snmp_PDUs_vals), 0,
        "snmp.PDUs", HFILL }},
    { &hf_snmp_parameters,
      { "parameters", "snmp.parameters",
        FT_BYTES, BASE_HEX, NULL, 0,
        "snmp.OCTET_STRING", HFILL }},
    { &hf_snmp_datav2u,
      { "datav2u", "snmp.datav2u",
        FT_UINT32, BASE_DEC, VALS(snmp_T_datav2u_vals), 0,
        "snmp.T_datav2u", HFILL }},
    { &hf_snmp_v2u_plaintext,
      { "plaintext", "snmp.plaintext",
        FT_UINT32, BASE_DEC, VALS(snmp_PDUs_vals), 0,
        "snmp.PDUs", HFILL }},
    { &hf_snmp_encrypted,
      { "encrypted", "snmp.encrypted",
        FT_BYTES, BASE_HEX, NULL, 0,
        "snmp.OCTET_STRING", HFILL }},
    { &hf_snmp_msgAuthoritativeEngineID,
      { "msgAuthoritativeEngineID", "snmp.msgAuthoritativeEngineID",
        FT_BYTES, BASE_HEX, NULL, 0,
        "snmp.T_msgAuthoritativeEngineID", HFILL }},
    { &hf_snmp_msgAuthoritativeEngineBoots,
      { "msgAuthoritativeEngineBoots", "snmp.msgAuthoritativeEngineBoots",
        FT_UINT32, BASE_DEC, NULL, 0,
        "snmp.T_msgAuthoritativeEngineBoots", HFILL }},
    { &hf_snmp_msgAuthoritativeEngineTime,
      { "msgAuthoritativeEngineTime", "snmp.msgAuthoritativeEngineTime",
        FT_UINT32, BASE_DEC, NULL, 0,
        "snmp.T_msgAuthoritativeEngineTime", HFILL }},
    { &hf_snmp_msgUserName,
      { "msgUserName", "snmp.msgUserName",
        FT_STRING, BASE_HEX, NULL, 0,
        "snmp.T_msgUserName", HFILL }},
    { &hf_snmp_msgAuthenticationParameters,
      { "msgAuthenticationParameters", "snmp.msgAuthenticationParameters",
        FT_BYTES, BASE_HEX, NULL, 0,
        "snmp.T_msgAuthenticationParameters", HFILL }},
    { &hf_snmp_msgPrivacyParameters,
      { "msgPrivacyParameters", "snmp.msgPrivacyParameters",
        FT_BYTES, BASE_HEX, NULL, 0,
        "snmp.T_msgPrivacyParameters", HFILL }},
    { &hf_snmp_msgVersion,
      { "msgVersion", "snmp.msgVersion",
        FT_INT32, BASE_DEC, VALS(snmp_Version_vals), 0,
        "snmp.Version", HFILL }},
    { &hf_snmp_msgGlobalData,
      { "msgGlobalData", "snmp.msgGlobalData",
        FT_NONE, BASE_NONE, NULL, 0,
        "snmp.HeaderData", HFILL }},
    { &hf_snmp_msgSecurityParameters,
      { "msgSecurityParameters", "snmp.msgSecurityParameters",
        FT_BYTES, BASE_HEX, NULL, 0,
        "snmp.T_msgSecurityParameters", HFILL }},
    { &hf_snmp_msgData,
      { "msgData", "snmp.msgData",
        FT_UINT32, BASE_DEC, VALS(snmp_ScopedPduData_vals), 0,
        "snmp.ScopedPduData", HFILL }},
    { &hf_snmp_msgID,
      { "msgID", "snmp.msgID",
        FT_UINT32, BASE_DEC, NULL, 0,
        "snmp.INTEGER_0_2147483647", HFILL }},
    { &hf_snmp_msgMaxSize,
      { "msgMaxSize", "snmp.msgMaxSize",
        FT_UINT32, BASE_DEC, NULL, 0,
        "snmp.INTEGER_484_2147483647", HFILL }},
    { &hf_snmp_msgFlags,
      { "msgFlags", "snmp.msgFlags",
        FT_BYTES, BASE_HEX, NULL, 0,
        "snmp.T_msgFlags", HFILL }},
    { &hf_snmp_msgSecurityModel,
      { "msgSecurityModel", "snmp.msgSecurityModel",
        FT_UINT32, BASE_DEC, VALS(sec_models), 0,
        "snmp.T_msgSecurityModel", HFILL }},
    { &hf_snmp_plaintext,
      { "plaintext", "snmp.plaintext",
        FT_NONE, BASE_NONE, NULL, 0,
        "snmp.ScopedPDU", HFILL }},
    { &hf_snmp_encryptedPDU,
      { "encryptedPDU", "snmp.encryptedPDU",
        FT_BYTES, BASE_HEX, NULL, 0,
        "snmp.T_encryptedPDU", HFILL }},
    { &hf_snmp_contextEngineID,
      { "contextEngineID", "snmp.contextEngineID",
        FT_BYTES, BASE_HEX, NULL, 0,
        "snmp.OCTET_STRING", HFILL }},
    { &hf_snmp_contextName,
      { "contextName", "snmp.contextName",
        FT_BYTES, BASE_HEX, NULL, 0,
        "snmp.OCTET_STRING", HFILL }},
    { &hf_snmp_get_request,
      { "get-request", "snmp.get_request",
        FT_NONE, BASE_NONE, NULL, 0,
        "snmp.T_get_request", HFILL }},
    { &hf_snmp_get_next_request,
      { "get-next-request", "snmp.get_next_request",
        FT_NONE, BASE_NONE, NULL, 0,
        "snmp.T_get_next_request", HFILL }},
    { &hf_snmp_get_response,
      { "get-response", "snmp.get_response",
        FT_NONE, BASE_NONE, NULL, 0,
        "snmp.T_get_response", HFILL }},
    { &hf_snmp_set_request,
      { "set-request", "snmp.set_request",
        FT_NONE, BASE_NONE, NULL, 0,
        "snmp.T_set_request", HFILL }},
    { &hf_snmp_trap,
      { "trap", "snmp.trap",
        FT_NONE, BASE_NONE, NULL, 0,
        "snmp.T_trap", HFILL }},
    { &hf_snmp_getBulkRequest,
      { "getBulkRequest", "snmp.getBulkRequest",
        FT_NONE, BASE_NONE, NULL, 0,
        "snmp.T_getBulkRequest", HFILL }},
    { &hf_snmp_informRequest,
      { "informRequest", "snmp.informRequest",
        FT_NONE, BASE_NONE, NULL, 0,
        "snmp.T_informRequest", HFILL }},
    { &hf_snmp_sNMPv2_Trap,
      { "sNMPv2-Trap", "snmp.sNMPv2_Trap",
        FT_NONE, BASE_NONE, NULL, 0,
        "snmp.T_sNMPv2_Trap", HFILL }},
    { &hf_snmp_report,
      { "report", "snmp.report",
        FT_NONE, BASE_NONE, NULL, 0,
        "snmp.T_report", HFILL }},
    { &hf_snmp_request_id,
      { "request-id", "snmp.request_id",
        FT_INT32, BASE_DEC, NULL, 0,
        "snmp.INTEGER", HFILL }},
    { &hf_snmp_error_status,
      { "error-status", "snmp.error_status",
        FT_INT32, BASE_DEC, VALS(snmp_T_error_status_vals), 0,
        "snmp.T_error_status", HFILL }},
    { &hf_snmp_error_index,
      { "error-index", "snmp.error_index",
        FT_INT32, BASE_DEC, NULL, 0,
        "snmp.INTEGER", HFILL }},
    { &hf_snmp_variable_bindings,
      { "variable-bindings", "snmp.variable_bindings",
        FT_UINT32, BASE_DEC, NULL, 0,
        "snmp.VarBindList", HFILL }},
    { &hf_snmp_bulkPDU_request_id,
      { "request-id", "snmp.request_id",
        FT_INT32, BASE_DEC, NULL, 0,
        "snmp.Integer32", HFILL }},
    { &hf_snmp_non_repeaters,
      { "non-repeaters", "snmp.non_repeaters",
        FT_UINT32, BASE_DEC, NULL, 0,
        "snmp.INTEGER_0_2147483647", HFILL }},
    { &hf_snmp_max_repetitions,
      { "max-repetitions", "snmp.max_repetitions",
        FT_UINT32, BASE_DEC, NULL, 0,
        "snmp.INTEGER_0_2147483647", HFILL }},
    { &hf_snmp_enterprise,
      { "enterprise", "snmp.enterprise",
        FT_OID, BASE_NONE, NULL, 0,
        "snmp.OBJECT_IDENTIFIER", HFILL }},
    { &hf_snmp_agent_addr,
      { "agent-addr", "snmp.agent_addr",
        FT_UINT32, BASE_DEC, VALS(snmp_NetworkAddress_vals), 0,
        "snmp.NetworkAddress", HFILL }},
    { &hf_snmp_generic_trap,
      { "generic-trap", "snmp.generic_trap",
        FT_INT32, BASE_DEC, VALS(snmp_T_generic_trap_vals), 0,
        "snmp.T_generic_trap", HFILL }},
    { &hf_snmp_specific_trap,
      { "specific-trap", "snmp.specific_trap",
        FT_INT32, BASE_DEC, NULL, 0,
        "snmp.INTEGER", HFILL }},
    { &hf_snmp_time_stamp,
      { "time-stamp", "snmp.time_stamp",
        FT_UINT32, BASE_DEC, NULL, 0,
        "snmp.TimeTicks", HFILL }},
    { &hf_snmp_name,
      { "name", "snmp.name",
        FT_OID, BASE_NONE, NULL, 0,
        "snmp.ObjectName", HFILL }},
    { &hf_snmp_valueType,
      { "valueType", "snmp.valueType",
        FT_UINT32, BASE_DEC, VALS(snmp_ValueType_vals), 0,
        "snmp.ValueType", HFILL }},
    { &hf_snmp_value,
      { "value", "snmp.value",
        FT_UINT32, BASE_DEC, VALS(snmp_ObjectSyntax_vals), 0,
        "snmp.ObjectSyntax", HFILL }},
    { &hf_snmp_unSpecified,
      { "unSpecified", "snmp.unSpecified",
        FT_NONE, BASE_NONE, NULL, 0,
        "snmp.NULL", HFILL }},
    { &hf_snmp_noSuchObject,
      { "noSuchObject", "snmp.noSuchObject",
        FT_NONE, BASE_NONE, NULL, 0,
        "snmp.NULL", HFILL }},
    { &hf_snmp_noSuchInstance,
      { "noSuchInstance", "snmp.noSuchInstance",
        FT_NONE, BASE_NONE, NULL, 0,
        "snmp.NULL", HFILL }},
    { &hf_snmp_endOfMibView,
      { "endOfMibView", "snmp.endOfMibView",
        FT_NONE, BASE_NONE, NULL, 0,
        "snmp.NULL", HFILL }},
    { &hf_snmp_VarBindList_item,
      { "Item", "snmp.VarBindList_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "snmp.VarBind", HFILL }},
    { &hf_snmp_open,
      { "open", "snmp.open",
        FT_UINT32, BASE_DEC, VALS(snmp_OpenPDU_vals), 0,
        "snmp.OpenPDU", HFILL }},
    { &hf_snmp_close,
      { "close", "snmp.close",
        FT_INT32, BASE_DEC, VALS(snmp_ClosePDU_vals), 0,
        "snmp.ClosePDU", HFILL }},
    { &hf_snmp_registerRequest,
      { "registerRequest", "snmp.registerRequest",
        FT_NONE, BASE_NONE, NULL, 0,
        "snmp.RReqPDU", HFILL }},
    { &hf_snmp_registerResponse,
      { "registerResponse", "snmp.registerResponse",
        FT_UINT32, BASE_DEC, VALS(snmp_RegisterResponse_vals), 0,
        "snmp.RegisterResponse", HFILL }},
    { &hf_snmp_commitOrRollback,
      { "commitOrRollback", "snmp.commitOrRollback",
        FT_INT32, BASE_DEC, VALS(snmp_SOutPDU_vals), 0,
        "snmp.SOutPDU", HFILL }},
    { &hf_snmp_rRspPDU,
      { "rRspPDU", "snmp.rRspPDU",
        FT_INT32, BASE_DEC, VALS(snmp_RRspPDU_vals), 0,
        "snmp.RRspPDU", HFILL }},
    { &hf_snmp_pDUs,
      { "pDUs", "snmp.pDUs",
        FT_UINT32, BASE_DEC, VALS(snmp_PDUs_vals), 0,
        "snmp.PDUs", HFILL }},
    { &hf_snmp_smux_simple,
      { "smux-simple", "snmp.smux_simple",
        FT_NONE, BASE_NONE, NULL, 0,
        "snmp.SimpleOpen", HFILL }},
    { &hf_snmp_smux_version,
      { "smux-version", "snmp.smux_version",
        FT_INT32, BASE_DEC, VALS(snmp_T_smux_version_vals), 0,
        "snmp.T_smux_version", HFILL }},
    { &hf_snmp_identity,
      { "identity", "snmp.identity",
        FT_OID, BASE_NONE, NULL, 0,
        "snmp.OBJECT_IDENTIFIER", HFILL }},
    { &hf_snmp_description,
      { "description", "snmp.description",
        FT_BYTES, BASE_HEX, NULL, 0,
        "snmp.DisplayString", HFILL }},
    { &hf_snmp_password,
      { "password", "snmp.password",
        FT_BYTES, BASE_HEX, NULL, 0,
        "snmp.OCTET_STRING", HFILL }},
    { &hf_snmp_subtree,
      { "subtree", "snmp.subtree",
        FT_OID, BASE_NONE, NULL, 0,
        "snmp.ObjectName", HFILL }},
    { &hf_snmp_priority,
      { "priority", "snmp.priority",
        FT_INT32, BASE_DEC, NULL, 0,
        "snmp.INTEGER_M1_2147483647", HFILL }},
    { &hf_snmp_operation,
      { "operation", "snmp.operation",
        FT_INT32, BASE_DEC, VALS(snmp_T_operation_vals), 0,
        "snmp.T_operation", HFILL }},

/*--- End of included file: packet-snmp-hfarr.c ---*/
#line 1907 "packet-snmp-template.c"
  };

  /* List of subtrees */
  static gint *ett[] = {
	  &ett_snmp,
	  &ett_engineid,
	  &ett_msgFlags,
	  &ett_encryptedPDU,
	  &ett_decrypted,
	  &ett_authParameters,
	  

/*--- Included file: packet-snmp-ettarr.c ---*/
#line 1 "packet-snmp-ettarr.c"
    &ett_snmp_ObjectSyntax,
    &ett_snmp_SimpleSyntax,
    &ett_snmp_ApplicationSyntax,
    &ett_snmp_NetworkAddress,
    &ett_snmp_Message,
    &ett_snmp_Messagev2u,
    &ett_snmp_T_datav2u,
    &ett_snmp_UsmSecurityParameters,
    &ett_snmp_SNMPv3Message,
    &ett_snmp_HeaderData,
    &ett_snmp_ScopedPduData,
    &ett_snmp_ScopedPDU,
    &ett_snmp_PDUs,
    &ett_snmp_PDU,
    &ett_snmp_BulkPDU,
    &ett_snmp_Trap_PDU,
    &ett_snmp_VarBind,
    &ett_snmp_ValueType,
    &ett_snmp_VarBindList,
    &ett_snmp_SMUX_PDUs,
    &ett_snmp_RegisterResponse,
    &ett_snmp_OpenPDU,
    &ett_snmp_SimpleOpen,
    &ett_snmp_RReqPDU,

/*--- End of included file: packet-snmp-ettarr.c ---*/
#line 1919 "packet-snmp-template.c"
  };
	module_t *snmp_module;

#ifdef HAVE_NET_SNMP

#ifdef _WIN32
	/* Set MIBDIRS so that the SNMP library can find its mibs. */
	/* XXX - Should we set MIBS or MIBFILES as well? */
	mib_path_len=strlen(get_datafile_dir()) + strlen(MIB_PATH_APPEND) + 20;
	mib_path = ep_alloc (mib_path_len);
	g_snprintf (mib_path, mib_path_len, "MIBDIRS=%s\\%s", get_datafile_dir(), MIB_PATH_APPEND);
	/* Amazingly enough, Windows does not provide setenv(). */
	if (getenv("MIBDIRS") == NULL)
		_putenv(mib_path);

#endif	/* _WIN32 */

	/*
	 * Suppress warnings about unknown tokens - we aren't initializing
	 * Net-SNMP in its entirety, we're just initializing the
	 * MIB-handling part because that's all we're using, which
	 * means that entries in the configuration file for other
	 * pars of the library will not be handled, and we don't want
	 * the config file reading code to whine about that.
	 */
	netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID,
                               NETSNMP_DS_LIB_NO_TOKEN_WARNINGS, TRUE);
	netsnmp_ds_set_int(NETSNMP_DS_LIBRARY_ID,
                           NETSNMP_DS_LIB_PRINT_SUFFIX_ONLY, 2);
#endif /* HAVE_NET_SNMP */


  /* Register protocol */
  proto_snmp = proto_register_protocol(PNAME, PSNAME, PFNAME);
  new_register_dissector("snmp", dissect_snmp, proto_snmp);

  /* Register fields and subtrees */
  proto_register_field_array(proto_snmp, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));


	/* Register configuration preferences */
	snmp_module = prefs_register_protocol(proto_snmp, process_prefs);
	prefs_register_bool_preference(snmp_module, "display_oid",
		"Show SNMP OID in info column",
		"Whether the SNMP OID should be shown in the info column",
		&display_oid);

	/*
	 * Set the default value of "mib_modules".
	 *
	 * If the MIBS environment variable is set, make its value
	 * the value of "mib_modules", otherwise, set "mib_modules"
	 * to DEF_MIB_MODULES.
	 */
	tmp_mib_modules = getenv("MIBS");
	if (tmp_mib_modules != NULL)
		mib_modules = tmp_mib_modules;
	prefs_register_string_preference(snmp_module, "mib_modules",
	    "MIB modules to load",
	    "List of MIB modules to load (the list is set to environment variable MIBS if the variable is not already set)"
	    "The list must be separated by colons (:) on non-Windows systems and semicolons (;) on Windows systems",
	    &mib_modules);
	prefs_register_bool_preference(snmp_module, "desegment",
	    "Reassemble SNMP-over-TCP messages\nspanning multiple TCP segments",
	    "Whether the SNMP dissector should reassemble messages spanning multiple TCP segments."
	    " To use this option, you must also enable \"Allow subdissectors to reassemble TCP streams\" in the TCP protocol settings.",
	    &snmp_desegment);

  prefs_register_bool_preference(snmp_module, "var_in_tree",
		"Display dissected variables inside SNMP tree",
		"ON - display dissected variables inside SNMP tree, OFF - display dissected variables in root tree after SNMP",
		&snmp_var_in_tree);

  prefs_register_string_preference(snmp_module, "users_file",
								   "USMuserTable file",
								   "The filename of the user table used for authentication and decryption",
								   &ue_assocs_filename);
 	  
	variable_oid_dissector_table =
	    register_dissector_table("snmp.variable_oid",
	      "SNMP Variable OID", FT_STRING, BASE_NONE);
	
	register_init_routine(renew_ue_cache);
}


/*--- proto_reg_handoff_snmp ---------------------------------------*/
void proto_reg_handoff_snmp(void) {
	dissector_handle_t snmp_tcp_handle;

	snmp_handle = find_dissector("snmp");

	dissector_add("udp.port", UDP_PORT_SNMP, snmp_handle);
	dissector_add("udp.port", UDP_PORT_SNMP_TRAP, snmp_handle);
	dissector_add("ethertype", ETHERTYPE_SNMP, snmp_handle);
	dissector_add("ipx.socket", IPX_SOCKET_SNMP_AGENT, snmp_handle);
	dissector_add("ipx.socket", IPX_SOCKET_SNMP_SINK, snmp_handle);
	dissector_add("hpext.dxsap", HPEXT_SNMP, snmp_handle);

	snmp_tcp_handle = create_dissector_handle(dissect_snmp_tcp, proto_snmp);
	dissector_add("tcp.port", TCP_PORT_SNMP, snmp_tcp_handle);
	dissector_add("tcp.port", TCP_PORT_SNMP_TRAP, snmp_tcp_handle);

	data_handle = find_dissector("data");

	/*
	 * Process preference settings.
	 *
	 * We can't do this in the register routine, as preferences aren't
	 * read until all dissector register routines have been called (so
	 * that all dissector preferences have been registered).
	 */
	process_prefs();

}

void
proto_register_smux(void)
{
	static hf_register_info hf[] = {
		{ &hf_smux_version,
		{ "Version", "smux.version", FT_UINT8, BASE_DEC, NULL,
		    0x0, "", HFILL }},
		{ &hf_smux_pdutype,
		{ "PDU type", "smux.pdutype", FT_UINT8, BASE_DEC, VALS(smux_types),
		    0x0, "", HFILL }},
	};
	static gint *ett[] = {
		&ett_smux,
	};

	proto_smux = proto_register_protocol("SNMP Multiplex Protocol",
	    "SMUX", "smux");
	proto_register_field_array(proto_smux, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));

}

void
proto_reg_handoff_smux(void)
{
	dissector_handle_t smux_handle;

	smux_handle = create_dissector_handle(dissect_smux, proto_smux);
	dissector_add("tcp.port", TCP_PORT_SMUX, smux_handle);
}


