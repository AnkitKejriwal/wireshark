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
 *
 * $Id: packet-snmp.c,v 1.102 2002/11/11 17:34:22 guy Exp $
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@ethereal.com>
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
#include "packet-ipx.h"

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
# define netsnmp_ds_set_boolean ds_set_boolean
# define netsnmp_ds_set_int ds_set_int
# define NETSNMP_DS_LIBRARY_ID DS_LIBRARY_ID
# define NETSNMP_DS_LIB_NO_TOKEN_WARNINGS DS_LIB_NO_TOKEN_WARNINGS
# define NETSNMP_DS_LIB_PRINT_SUFFIX_ONLY DS_LIB_PRINT_SUFFIX_ONLY
#endif /* HAVE_NET_SNMP */

#ifdef WIN32
# include <epan/filesystem.h>
#endif /* WIN32 */

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

#endif /* HAVE_SOME_SNMP */

#include "asn1.h"

#include "packet-snmp.h"
#include "format-oid.h"

/* Null string of type "guchar[]". */
static const guchar nullstring[] = "";

/* Take a pointer that may be null and return a pointer that's not null
   by turning null pointers into pointers to the above null string. */
#define	SAFE_STRING(s)	(((s) != NULL) ? (s) : nullstring)

static int proto_snmp = -1;
static int proto_smux = -1;

static gint ett_snmp = -1;
static gint ett_smux = -1;
static gint ett_parameters = -1;
static gint ett_parameters_qos = -1;
static gint ett_global = -1;
static gint ett_flags = -1;
static gint ett_secur = -1;

static int hf_snmpv3_flags = -1;
static int hf_snmpv3_flags_auth = -1;
static int hf_snmpv3_flags_crypt = -1;
static int hf_snmpv3_flags_report = -1;

static dissector_handle_t snmp_handle;
static dissector_handle_t data_handle;

#define TH_AUTH   0x01
#define TH_CRYPT  0x02
#define TH_REPORT 0x04

#define UDP_PORT_SNMP		161
#define UDP_PORT_SNMP_TRAP	162
#define TCP_PORT_SMUX		199

/* Protocol version numbers */
#define SNMP_VERSION_1	0
#define SNMP_VERSION_2c	1
#define SNMP_VERSION_2u	2
#define SNMP_VERSION_3	3

static const value_string versions[] = {
	{ SNMP_VERSION_1,	"1" },
	{ SNMP_VERSION_2c,	"2C" },
	{ SNMP_VERSION_2u,	"2U" },
	{ SNMP_VERSION_3,	"3" },
	{ 0,			NULL },
};

/* PDU types */
#define SNMP_MSG_GET		0
#define SNMP_MSG_GETNEXT	1
#define SNMP_MSG_RESPONSE	2
#define SNMP_MSG_SET		3
#define SNMP_MSG_TRAP		4

#define SNMP_MSG_GETBULK	5
#define SNMP_MSG_INFORM		6
#define SNMP_MSG_TRAP2		7
#define SNMP_MSG_REPORT		8

static const value_string pdu_types[] = {
	{ SNMP_MSG_GET,		"GET" },
	{ SNMP_MSG_GETNEXT,	"GET-NEXT" },
	{ SNMP_MSG_SET,		"SET" },
	{ SNMP_MSG_RESPONSE,	"RESPONSE" },
	{ SNMP_MSG_TRAP, 	"TRAP-V1" },
	{ SNMP_MSG_GETBULK, 	"GETBULK" },
	{ SNMP_MSG_INFORM, 	"INFORM" },
	{ SNMP_MSG_TRAP2, 	"TRAP-V2" },
	{ SNMP_MSG_REPORT,	"REPORT" },
	{ 0,			NULL }
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

/* SMUX Closing causes */
#define SMUX_CLOSE_DOWN			0
#define SMUX_CLOSE_VERSION		1
#define SMUX_CLOSE_PACKET		2
#define SMUX_CLOSE_PROTOCOL		3
#define SMUX_CLOSE_INTERNAL		4
#define SMUX_CLOSE_NOAUTH		5

static const value_string smux_close[] = {
	{ SMUX_CLOSE_DOWN,	"Going down" },
	{ SMUX_CLOSE_VERSION,	"Unsupported Version" },
	{ SMUX_CLOSE_PACKET,	"Packet Format Error" },
	{ SMUX_CLOSE_PROTOCOL,	"Protocol Error" },
	{ SMUX_CLOSE_INTERNAL,	"Internal Error" },
	{ SMUX_CLOSE_NOAUTH,	"Unauthorized" },
	{ 0,			NULL }
};

/* SMUX Request codes */
#define SMUX_RREQ_DELETE		0
#define SMUX_RREQ_READONLY		1
#define SMUX_RREQ_READWRITE		2

static const value_string smux_rreq[] = {
	{ SMUX_RREQ_DELETE,	"Delete" },
	{ SMUX_RREQ_READONLY,	"Read Only" },
	{ SMUX_RREQ_READWRITE,	"Read Write" },
	{ 0,			NULL }
};

static const value_string smux_prio[] = {
	{ -1,				"Failure" },
	{ 0,				NULL }
};

/* SMUX SOut codes */
#define SMUX_SOUT_COMMIT		0
#define SMUX_SOUT_ROLLBACK		1

static const value_string smux_sout[] = {
	{ SMUX_SOUT_COMMIT,		"Commit" },
	{ SMUX_SOUT_ROLLBACK,		"Rollback" },
	{ 0,			        NULL }
};

/* Error status values */
#define SNMP_ERR_NOERROR		0
#define SNMP_ERR_TOOBIG			1
#define SNMP_ERR_NOSUCHNAME		2
#define SNMP_ERR_BADVALUE		3
#define SNMP_ERR_READONLY		4
#define SNMP_ERR_GENERROR		5

#define SNMP_ERR_NOACCESS		6
#define SNMP_ERR_WRONGTYPE		7
#define SNMP_ERR_WRONGLENGTH		8
#define SNMP_ERR_WRONGENCODING		9
#define SNMP_ERR_WRONGVALUE		10
#define SNMP_ERR_NOCREATION		11
#define SNMP_ERR_INCONSISTENTVALUE	12
#define SNMP_ERR_RESOURCEUNAVAILABLE	13
#define SNMP_ERR_COMMITFAILED		14
#define SNMP_ERR_UNDOFAILED		15
#define SNMP_ERR_AUTHORIZATIONERROR	16
#define SNMP_ERR_NOTWRITABLE		17
#define SNMP_ERR_INCONSISTENTNAME	18

static const value_string error_statuses[] = {
	{ SNMP_ERR_NOERROR,		"NO ERROR" },
	{ SNMP_ERR_TOOBIG,		"TOOBIG" },
	{ SNMP_ERR_NOSUCHNAME,		"NO SUCH NAME" },
	{ SNMP_ERR_BADVALUE,		"BAD VALUE" },
	{ SNMP_ERR_READONLY,		"READ ONLY" },
	{ SNMP_ERR_GENERROR,		"GENERIC ERROR" },
	{ SNMP_ERR_NOACCESS,		"NO ACCESS" },
	{ SNMP_ERR_WRONGTYPE,		"WRONG TYPE" },
	{ SNMP_ERR_WRONGLENGTH,		"WRONG LENGTH" },
	{ SNMP_ERR_WRONGENCODING,	"WRONG ENCODING" },
	{ SNMP_ERR_WRONGVALUE,		"WRONG VALUE" },
	{ SNMP_ERR_NOCREATION,		"NO CREATION" },
	{ SNMP_ERR_INCONSISTENTVALUE,	"INCONSISTENT VALUE" },
	{ SNMP_ERR_RESOURCEUNAVAILABLE,	"RESOURCE UNAVAILABLE" },
	{ SNMP_ERR_COMMITFAILED,	"COMMIT FAILED" },
	{ SNMP_ERR_UNDOFAILED,		"UNDO FAILED" },
	{ SNMP_ERR_AUTHORIZATIONERROR,	"AUTHORIZATION ERROR" },
	{ SNMP_ERR_NOTWRITABLE,		"NOT WRITABLE" },
	{ SNMP_ERR_INCONSISTENTNAME,	"INCONSISTENT NAME" },
	{ 0,				NULL }
};

/* General SNMP V1 Traps */

#define SNMP_TRAP_COLDSTART		0
#define SNMP_TRAP_WARMSTART		1
#define SNMP_TRAP_LINKDOWN		2
#define SNMP_TRAP_LINKUP		3
#define SNMP_TRAP_AUTHFAIL		4
#define SNMP_TRAP_EGPNEIGHBORLOSS	5
#define SNMP_TRAP_ENTERPRISESPECIFIC	6

static const value_string trap_types[] = {
	{ SNMP_TRAP_COLDSTART,		"COLD START" },
	{ SNMP_TRAP_WARMSTART,		"WARM START" },
	{ SNMP_TRAP_LINKDOWN,		"LINK DOWN" },
	{ SNMP_TRAP_LINKUP,		"LINK UP" },
	{ SNMP_TRAP_AUTHFAIL,		"AUTHENTICATION FAILED" },
	{ SNMP_TRAP_EGPNEIGHBORLOSS,	"EGP NEIGHBORLOSS" },
	{ SNMP_TRAP_ENTERPRISESPECIFIC,	"ENTERPRISE SPECIFIC" },
	{ 0,				NULL }
};

/* Security Models */

#define SNMP_SEC_ANY			0
#define SNMP_SEC_V1			1
#define SNMP_SEC_V2C			2
#define SNMP_SEC_USM			3

static const value_string sec_models[] = {
	{ SNMP_SEC_ANY,			"Any" },
	{ SNMP_SEC_V1,			"V1" },
	{ SNMP_SEC_V2C,			"V2C" },
	{ SNMP_SEC_USM,			"USM" },
	{ 0,				NULL }
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
  gchar *name;
};

static SNMP_CNV SnmpCnv [] =
{
  {ASN1_UNI, ASN1_NUL, SNMP_NULL,      "NULL"},
  {ASN1_UNI, ASN1_INT, SNMP_INTEGER,   "INTEGER"},
  {ASN1_UNI, ASN1_OTS, SNMP_OCTETSTR,  "OCTET STRING"},
  {ASN1_UNI, ASN1_OJI, SNMP_OBJECTID,  "OBJECTID"},
  {ASN1_APL, SNMP_IPA, SNMP_IPADDR,    "IPADDR"},
  {ASN1_APL, SNMP_CNT, SNMP_COUNTER,   "COUNTER"},  /* Counter32 */
  {ASN1_APL, SNMP_GGE, SNMP_GAUGE,     "GAUGE"},    /* Gauge32 == Unsigned32  */
  {ASN1_APL, SNMP_TIT, SNMP_TIMETICKS, "TIMETICKS"},
  {ASN1_APL, SNMP_OPQ, SNMP_OPAQUE,    "OPAQUE"},

/* SNMPv2 data types and errors */

  {ASN1_UNI, ASN1_BTS, SNMP_BITSTR,         "BITSTR"},
  {ASN1_APL, SNMP_C64, SNMP_COUNTER64,      "COUNTER64"},
  {ASN1_CTX, SERR_NSO, SNMP_NOSUCHOBJECT,   "NOSUCHOBJECT"},
  {ASN1_CTX, SERR_NSI, SNMP_NOSUCHINSTANCE, "NOSUCHINSTANCE"},
  {ASN1_CTX, SERR_EOM, SNMP_ENDOFMIBVIEW,   "ENDOFMIBVIEW"},
  {0,       0,         -1,                  NULL}
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

static gchar *
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

static void
dissect_snmp_parse_error(tvbuff_t *tvb, int offset, packet_info *pinfo,
		   proto_tree *tree, const char *field_name, int ret)
{
	char *errstr;

	errstr = asn1_err_to_str(ret);

	if (check_col(pinfo->cinfo, COL_INFO)) {
		col_add_fstr(pinfo->cinfo, COL_INFO,
		    "ERROR: Couldn't parse %s: %s", field_name, errstr);
	}
	if (tree != NULL) {
		proto_tree_add_text(tree, tvb, offset, 0,
		    "ERROR: Couldn't parse %s: %s", field_name, errstr);
		call_dissector(data_handle,
		    tvb_new_subset(tvb, offset, -1, -1), pinfo, tree);
	}
}

static void
dissect_snmp_error(tvbuff_t *tvb, int offset, packet_info *pinfo,
		   proto_tree *tree, const char *message)
{
	if (check_col(pinfo->cinfo, COL_INFO))
		col_add_str(pinfo->cinfo, COL_INFO, message);

	if (tree != NULL) {
		proto_tree_add_text(tree, tvb, offset, 0, "%s", message);
		call_dissector(data_handle,
		    tvb_new_subset(tvb, offset, -1, -1), pinfo, tree);
	}
}

gchar *
format_oid(subid_t *oid, guint oid_length)
{
	char *result;
	int result_len;
	int len;
	unsigned int i;
	char *buf;
#ifdef HAVE_SOME_SNMP
	guchar *oid_string;
	size_t oid_string_len;
	size_t oid_out_len;
#endif

	result_len = oid_length * 22;

#ifdef HAVE_SOME_SNMP
	/*
	 * Get the decoded form of the OID, and add its length to the
	 * length of the result string.
	 *
	 * XXX - check for "malloc" and "sprint_realloc_objid()" failure.
	 */
	oid_string_len = 256;
	oid_string = malloc(oid_string_len);
	*oid_string = '\0';
	oid_out_len = 0;
	sprint_realloc_objid(&oid_string, &oid_string_len, &oid_out_len, 1,
	    oid, oid_length);
	result_len += strlen(oid_string) + 3;
#endif

	result = g_malloc(result_len + 1);
	buf = result;
	len = sprintf(buf, "%lu", (unsigned long)oid[0]);
	buf += len;
	for (i = 1; i < oid_length;i++) {
		len = sprintf(buf, ".%lu", (unsigned long)oid[i]);
		buf += len;
	}

#ifdef HAVE_SOME_SNMP
	/*
	 * Append the decoded form of the OID.
	 */
	sprintf(buf, " (%s)", oid_string);
	free(oid_string);
#endif

	return result;
}

#ifdef HAVE_SOME_SNMP
static guchar *
check_var_length(guint vb_length, guint required_length)
{
	guchar *buf;
	static const char badlen_fmt[] = "Length is %u, should be %u";

	if (vb_length != required_length) {
		/* Enough room for the largest "Length is XXX,
		   should be XXX" message - 10 digits for each
		   XXX. */
		buf = malloc(sizeof badlen_fmt + 10 + 10);
		sprintf(buf, badlen_fmt, vb_length, required_length);
		return buf;
	}
	return NULL;	/* length is OK */
}

static guchar *
format_var(struct variable_list *variable, subid_t *variable_oid,
    guint variable_oid_length, gushort vb_type, guint val_len)
{
	guchar *buf;
	size_t buf_len;
	size_t out_len;

	switch (vb_type) {

	case SNMP_IPADDR:
		/* Length has to be 4 bytes. */
		buf = check_var_length(val_len, 4);
		if (buf != NULL)
			return buf;	/* it's not 4 bytes */
		break;

	case SNMP_COUNTER64:
		/* Length has to be 8 bytes. */
		buf = check_var_length(val_len, 8);
		if (buf != NULL)
			return buf;	/* it's not 8 bytes */
		break;

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
	 * XXX - check for "malloc" and "sprint_realloc_objid()" failure.
	 */
	buf_len = 256;
	buf = malloc(buf_len);
	*buf = '\0';
	out_len = 0;
	sprint_realloc_value(&buf, &buf_len, &out_len, 1,  variable_oid,
	    variable_oid_length, variable);
	return buf;
}
#endif

static int
snmp_variable_decode(proto_tree *snmp_tree,
    subid_t *variable_oid
#ifndef HAVE_SOME_SNMP
	_U_
#endif
    ,
    guint variable_oid_length
#ifndef HAVE_SOME_SNMP
	_U_
#endif
    ,
    ASN1_SCK *asn1, int offset, guint *lengthp)
{
	int start;
	guint length;
	gboolean def;
	guint vb_length;
	gushort vb_type;
	gchar *vb_type_name;
	int ret;
	guint cls, con, tag;

	gint32 vb_integer_value;
	guint32 vb_uinteger_value;

	guint8 *vb_octet_string;

	subid_t *vb_oid;
	guint vb_oid_length;

	gchar *vb_display_string;

#ifdef HAVE_SOME_SNMP
	struct variable_list variable;
	long value;
#else /* HAVE_SOME_SNMP */
	unsigned int i;
	gchar *buf;
	int len;
#endif	/* HAVE_SOME_SNMP */

	/* parse the type of the object */
	start = asn1->offset;
	ret = asn1_header_decode (asn1, &cls, &con, &tag, &def, &vb_length);
	if (ret != ASN1_ERR_NOERROR)
		return ret;
	if (!def)
		return ASN1_ERR_LENGTH_NOT_DEFINITE;

	/* Convert the class, constructed flag, and tag to a type. */
	vb_type_name = snmp_tag_cls2syntax(tag, cls, &vb_type);
	if (vb_type_name == NULL) {
		/*
		 * Unsupported type.
		 * Dissect the value as an opaque string of octets.
		 */
		vb_type_name = "unsupported type";
		vb_type = SNMP_OPAQUE;
	}

	/* parse the value */
	switch (vb_type) {

	case SNMP_INTEGER:
		ret = asn1_int32_value_decode(asn1, vb_length,
		    &vb_integer_value);
		if (ret != ASN1_ERR_NOERROR)
			return ret;
		length = asn1->offset - start;
		if (snmp_tree) {
#ifdef HAVE_SOME_SNMP
			value = vb_integer_value;
			variable.val.integer = &value;
			vb_display_string = format_var(&variable,
			    variable_oid, variable_oid_length, vb_type,
			    vb_length);
			proto_tree_add_text(snmp_tree, asn1->tvb, offset,
			    length,
			    "Value: %s", vb_display_string);
			free(vb_display_string);
#else /* HAVE_SOME_SNMP */
			proto_tree_add_text(snmp_tree, asn1->tvb, offset,
			    length,
			    "Value: %s: %d (%#x)", vb_type_name,
			    vb_integer_value, vb_integer_value);
#endif /* HAVE_SOME_SNMP */
		}
		break;

	case SNMP_COUNTER:
	case SNMP_GAUGE:
	case SNMP_TIMETICKS:
		ret = asn1_uint32_value_decode(asn1, vb_length,
		    &vb_uinteger_value);
		if (ret != ASN1_ERR_NOERROR)
			return ret;
		length = asn1->offset - start;
		if (snmp_tree) {
#ifdef HAVE_SOME_SNMP
			value = vb_uinteger_value;
			variable.val.integer = &value;
			vb_display_string = format_var(&variable,
			    variable_oid, variable_oid_length, vb_type,
			    vb_length);
			proto_tree_add_text(snmp_tree, asn1->tvb, offset,
			    length,
			    "Value: %s", vb_display_string);
			free(vb_display_string);
#else /* HAVE_SOME_SNMP */
			proto_tree_add_text(snmp_tree, asn1->tvb, offset,
			    length,
			    "Value: %s: %u (%#x)", vb_type_name,
			    vb_uinteger_value, vb_uinteger_value);
#endif /* HAVE_SOME_SNMP */
		}
		break;

	case SNMP_OCTETSTR:
	case SNMP_IPADDR:
	case SNMP_OPAQUE:
	case SNMP_NSAP:
	case SNMP_BITSTR:
	case SNMP_COUNTER64:
		ret = asn1_string_value_decode (asn1, vb_length,
		    &vb_octet_string);
		if (ret != ASN1_ERR_NOERROR)
			return ret;
		length = asn1->offset - start;
		if (snmp_tree) {
#ifdef HAVE_SOME_SNMP
			variable.val.string = vb_octet_string;
			vb_display_string = format_var(&variable,
			    variable_oid, variable_oid_length, vb_type,
			    vb_length);
			proto_tree_add_text(snmp_tree, asn1->tvb, offset,
			    length,
			    "Value: %s", vb_display_string);
			free(vb_display_string);
#else /* HAVE_SOME_SNMP */
			/*
			 * If some characters are not printable, display
			 * the string as bytes.
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
				vb_display_string = g_malloc(4*vb_length);
				buf = &vb_display_string[0];
				len = sprintf(buf, "%03u", vb_octet_string[0]);
				buf += len;
				for (i = 1; i < vb_length; i++) {
					len = sprintf(buf, ".%03u",
					    vb_octet_string[i]);
					buf += len;
				}
				proto_tree_add_text(snmp_tree, asn1->tvb, offset,
				    length,
				    "Value: %s: %s", vb_type_name,
				    vb_display_string);
				g_free(vb_display_string);
			} else {
				proto_tree_add_text(snmp_tree, asn1->tvb, offset,
				    length,
				    "Value: %s: %.*s", vb_type_name,
				    (int)vb_length,
				    SAFE_STRING(vb_octet_string));
			}
#endif /* HAVE_SOME_SNMP */
		}
		g_free(vb_octet_string);
		break;

	case SNMP_NULL:
		ret = asn1_null_decode (asn1, vb_length);
		if (ret != ASN1_ERR_NOERROR)
			return ret;
		length = asn1->offset - start;
		if (snmp_tree) {
			proto_tree_add_text(snmp_tree, asn1->tvb, offset, length,
			    "Value: %s", vb_type_name);
		}
		break;

	case SNMP_OBJECTID:
		ret = asn1_oid_value_decode (asn1, vb_length, &vb_oid,
		    &vb_oid_length);
		if (ret != ASN1_ERR_NOERROR)
			return ret;
		length = asn1->offset - start;
		if (snmp_tree) {
#ifdef HAVE_SOME_SNMP
			variable.val.objid = vb_oid;
			vb_display_string = format_var(&variable,
			    variable_oid, variable_oid_length, vb_type,
			    vb_oid_length * sizeof (subid_t));
			proto_tree_add_text(snmp_tree, asn1->tvb, offset,
			    length,
			    "Value: %s", vb_display_string);
			free(vb_display_string);
#else /* HAVE_SOME_SNMP */
			vb_display_string = format_oid(vb_oid, vb_oid_length);
			proto_tree_add_text(snmp_tree, asn1->tvb, offset,
			    length,
			    "Value: %s: %s", vb_type_name, vb_display_string);
			g_free(vb_display_string);
#endif /* HAVE_SOME_SNMP */
		}
		g_free(vb_oid);
		break;

	case SNMP_NOSUCHOBJECT:
		length = asn1->offset - start;
		if (snmp_tree) {
			proto_tree_add_text(snmp_tree, asn1->tvb, offset, length,
			    "Value: %s: no such object", vb_type_name);
		}
		break;

	case SNMP_NOSUCHINSTANCE:
		length = asn1->offset - start;
		if (snmp_tree) {
			proto_tree_add_text(snmp_tree, asn1->tvb, offset, length,
			    "Value: %s: no such instance", vb_type_name);
		}
		break;

	case SNMP_ENDOFMIBVIEW:
		length = asn1->offset - start;
		if (snmp_tree) {
			proto_tree_add_text(snmp_tree, asn1->tvb, offset, length,
			    "Value: %s: end of mib view", vb_type_name);
		}
		break;

	default:
		g_assert_not_reached();
		return ASN1_ERR_WRONG_TYPE;
	}
	*lengthp = length;
	return ASN1_ERR_NOERROR;
}

static void
dissect_common_pdu(tvbuff_t *tvb, int offset, packet_info *pinfo,
    proto_tree *tree, ASN1_SCK asn1, guint pdu_type, int start)
{
	gboolean def;
	guint length;
	guint sequence_length;

	guint32 request_id;

	guint32 error_status;

	guint32 error_index;

	char *pdu_type_string;

	subid_t *enterprise;
	guint enterprise_length;

	guint8 *agent_address;
	guint agent_address_length;

	guint32 trap_type;

	guint32 specific_type;

	guint timestamp;
	guint timestamp_length;

	gchar *oid_string;

	guint variable_bindings_length;

	int vb_index;
	guint variable_length;
	subid_t *variable_oid;
	guint variable_oid_length;

	int ret;
	guint cls, con, tag;

	pdu_type_string = val_to_str(pdu_type, pdu_types,
	    "Unknown PDU type %#x");
	if (check_col(pinfo->cinfo, COL_INFO))
		col_add_str(pinfo->cinfo, COL_INFO, pdu_type_string);
	length = asn1.offset - start;
	if (tree) {
		proto_tree_add_text(tree, tvb, offset, length,
		    "PDU type: %s", pdu_type_string);
	}
	offset += length;

	/* get the fields in the PDU preceeding the variable-bindings sequence */
	switch (pdu_type) {

	case SNMP_MSG_GET:
	case SNMP_MSG_GETNEXT:
	case SNMP_MSG_RESPONSE:
	case SNMP_MSG_SET:
	case SNMP_MSG_GETBULK:
	case SNMP_MSG_INFORM:
	case SNMP_MSG_TRAP2:
	case SNMP_MSG_REPORT:
		/* request id */
		ret = asn1_uint32_decode (&asn1, &request_id, &length);
		if (ret != ASN1_ERR_NOERROR) {
			dissect_snmp_parse_error(tvb, offset, pinfo, tree,
			    "request ID", ret);
			return;
		}
		if (tree) {
			proto_tree_add_text(tree, tvb, offset, length,
			    "Request Id: %#x", request_id);
		}
		offset += length;

		/* error status, or getbulk non-repeaters */
		ret = asn1_uint32_decode (&asn1, &error_status, &length);
		if (ret != ASN1_ERR_NOERROR) {
			dissect_snmp_parse_error(tvb, offset, pinfo, tree,
			    (pdu_type == SNMP_MSG_GETBULK) ? "non-repeaters"
			    				   : "error status",
			    ret);
			return;
		}
		if (tree) {
			if (pdu_type == SNMP_MSG_GETBULK) {
				proto_tree_add_text(tree, tvb, offset,
				    length, "Non-repeaters: %u", error_status);
			} else {
				proto_tree_add_text(tree, tvb, offset,
				    length, "Error Status: %s",
				    val_to_str(error_status, error_statuses,
				      "Unknown (%d)"));
			}
		}
		offset += length;

		/* error index, or getbulk max-repetitions */
		ret = asn1_uint32_decode (&asn1, &error_index, &length);
		if (ret != ASN1_ERR_NOERROR) {
			dissect_snmp_parse_error(tvb, offset, pinfo, tree,
			    (pdu_type == SNMP_MSG_GETBULK) ? "max repetitions"
			    				   : "error index",
			    ret);
			return;
		}
		if (tree) {
			if (pdu_type == SNMP_MSG_GETBULK) {
				proto_tree_add_text(tree, tvb, offset,
				    length, "Max repetitions: %u", error_index);
			} else {
				proto_tree_add_text(tree, tvb, offset,
				    length, "Error Index: %u", error_index);
			}
		}
		offset += length;
		break;

	case SNMP_MSG_TRAP:
		/* enterprise */
		ret = asn1_oid_decode (&asn1, &enterprise, &enterprise_length,
		    &length);
		if (ret != ASN1_ERR_NOERROR) {
			dissect_snmp_parse_error(tvb, offset, pinfo, tree,
			    "enterprise OID", ret);
			return;
		}
		if (tree) {
			oid_string = format_oid(enterprise, enterprise_length);
			proto_tree_add_text(tree, tvb, offset, length,
			    "Enterprise: %s", oid_string);
			g_free(oid_string);
		}
		g_free(enterprise);
		offset += length;

		/* agent address */
		start = asn1.offset;
		ret = asn1_header_decode (&asn1, &cls, &con, &tag,
		    &def, &agent_address_length);
		if (ret != ASN1_ERR_NOERROR) {
			dissect_snmp_parse_error(tvb, offset, pinfo, tree,
			    "agent address", ret);
			return;
		}
		if (!((cls == ASN1_APL && con == ASN1_PRI && tag == SNMP_IPA) ||
		    (cls == ASN1_UNI && con == ASN1_PRI && tag == ASN1_OTS))) {
			/* GXSNMP 0.0.15 says the latter is "needed for
			   Banyan" */
			dissect_snmp_parse_error(tvb, offset, pinfo, tree,
			    "agent_address", ASN1_ERR_WRONG_TYPE);
			return;
		}
		if (agent_address_length != 4) {
			dissect_snmp_parse_error(tvb, offset, pinfo, tree,
			    "agent_address", ASN1_ERR_WRONG_LENGTH_FOR_TYPE);
			return;
		}
		ret = asn1_string_value_decode (&asn1,
		    agent_address_length, &agent_address);
		if (ret != ASN1_ERR_NOERROR) {
			dissect_snmp_parse_error(tvb, offset, pinfo, tree,
			    "agent address", ret);
			return;
		}
		length = asn1.offset - start;
		if (tree) {
			if (agent_address_length != 4) {
				proto_tree_add_text(tree, tvb, offset,
				    length,
				    "Agent address: <length is %u, not 4>",
				    agent_address_length);
			} else {
				proto_tree_add_text(tree, tvb, offset,
				    length,
				    "Agent address: %s",
				    ip_to_str(agent_address));
			}
		}
		g_free(agent_address);
		offset += length;

	        /* generic trap type */
		ret = asn1_uint32_decode (&asn1, &trap_type, &length);
		if (ret != ASN1_ERR_NOERROR) {
			dissect_snmp_parse_error(tvb, offset, pinfo, tree,
			    "generic trap type", ret);
			return;
		}
		if (tree) {
			proto_tree_add_text(tree, tvb, offset, length,
			    "Trap type: %s",
			    val_to_str(trap_type, trap_types, "Unknown (%u)"));
		}
		offset += length;

	        /* specific trap type */
		ret = asn1_uint32_decode (&asn1, &specific_type, &length);
		if (ret != ASN1_ERR_NOERROR) {
			dissect_snmp_parse_error(tvb, offset, pinfo, tree,
			    "specific trap type", ret);
			return;
		}
		if (tree) {
			proto_tree_add_text(tree, tvb, offset, length,
			    "Specific trap type: %u (%#x)",
			    specific_type, specific_type);
		}
		offset += length;

	        /* timestamp */
		start = asn1.offset;
		ret = asn1_header_decode (&asn1, &cls, &con, &tag,
		    &def, &timestamp_length);
		if (ret != ASN1_ERR_NOERROR) {
			dissect_snmp_parse_error(tvb, offset, pinfo, tree,
			    "timestamp", ret);
			return;
		}
		if (!((cls == ASN1_APL && con == ASN1_PRI && tag == SNMP_TIT) ||
		    (cls == ASN1_UNI && con == ASN1_PRI && tag == ASN1_INT))) {
			dissect_snmp_parse_error(tvb, offset, pinfo, tree,
			    "timestamp", ASN1_ERR_WRONG_TYPE);
			return;
		}
		ret = asn1_uint32_value_decode(&asn1, timestamp_length,
		    &timestamp);
		if (ret != ASN1_ERR_NOERROR) {
			dissect_snmp_parse_error(tvb, offset, pinfo, tree,
			    "timestamp", ret);
			return;
		}
		length = asn1.offset - start;
		if (tree) {
			proto_tree_add_text(tree, tvb, offset, length,
			    "Timestamp: %u", timestamp);
		}
		offset += length;
		break;
	}

	/* variable bindings */
	/* get header for variable-bindings sequence */
	ret = asn1_sequence_decode(&asn1, &variable_bindings_length, &length);
	if (ret != ASN1_ERR_NOERROR) {
		dissect_snmp_parse_error(tvb, offset, pinfo, tree,
			"variable bindings header", ret);
		return;
	}
	offset += length;

	/* loop on variable bindings */
	vb_index = 0;
	while (variable_bindings_length > 0) {
		vb_index++;
		sequence_length = 0;

		/* parse type */
		ret = asn1_sequence_decode(&asn1, &variable_length, &length);
		if (ret != ASN1_ERR_NOERROR) {
			dissect_snmp_parse_error(tvb, offset, pinfo, tree,
				"variable binding header", ret);
			return;
		}
		sequence_length += length;

		/* parse object identifier */
		ret = asn1_oid_decode (&asn1, &variable_oid,
		    &variable_oid_length, &length);
		if (ret != ASN1_ERR_NOERROR) {
			dissect_snmp_parse_error(tvb, offset, pinfo, tree,
			    "variable binding OID", ret);
			return;
		}
		sequence_length += length;

		if (tree) {
			oid_string = format_oid(variable_oid,
			    variable_oid_length);
			proto_tree_add_text(tree, tvb, offset, sequence_length,
			    "Object identifier %d: %s", vb_index, oid_string);
			g_free(oid_string);
		}
		offset += sequence_length;
		variable_bindings_length -= sequence_length;

		/* Parse the variable's value */
		ret = snmp_variable_decode(tree, variable_oid,
		    variable_oid_length, &asn1, offset, &length);
		if (ret != ASN1_ERR_NOERROR) {
			dissect_snmp_parse_error(tvb, offset, pinfo, tree,
			    "variable", ret);
			return;
		}
		offset += length;
		variable_bindings_length -= length;
	}
}

static const value_string qos_vals[] = {
	{ 0x0,	"No authentication or privacy" },
	{ 0x1,	"Authentication, no privacy" },
	{ 0x2,	"Authentication and privacy" },
	{ 0x3,	"Authentication and privacy" },
	{ 0,	NULL },
};

static void
dissect_snmp2u_parameters(proto_tree *tree, tvbuff_t *tvb, int offset, int length,
    guchar *parameters, int parameters_length)
{
	proto_item *item;
	proto_tree *parameters_tree;
	proto_tree *qos_tree;
	guint8 model;
	guint8 qos;
	guint8 len;

	item = proto_tree_add_text(tree, tvb, offset, length,
	    "Parameters");
	parameters_tree = proto_item_add_subtree(item, ett_parameters);
	offset += length - parameters_length;

	if (parameters_length < 1)
		return;
	model = *parameters;
	proto_tree_add_text(parameters_tree, tvb, offset, 1,
	    "model: %u", model);
	offset += 1;
	parameters += 1;
	parameters_length -= 1;
	if (model != 1) {
		/* Unknown model. */
		proto_tree_add_text(parameters_tree, tvb, offset,
		    parameters_length, "parameters: %s",
		    bytes_to_str(parameters, parameters_length));
		return;
	}

	if (parameters_length < 1)
		return;
	qos = *parameters;
	item = proto_tree_add_text(parameters_tree, tvb, offset, 1,
	    "qoS: 0x%x", qos);
	qos_tree = proto_item_add_subtree(item, ett_parameters_qos);
	proto_tree_add_text(qos_tree, tvb, offset, 1, "%s",
	    decode_boolean_bitfield(qos, 0x04,
		8, "Generation of report PDU allowed",
		   "Generation of report PDU not allowed"));
	proto_tree_add_text(qos_tree, tvb, offset, 1, "%s",
	    decode_enumerated_bitfield(qos, 0x03,
		8, qos_vals, "%s"));
	offset += 1;
	parameters += 1;
	parameters_length -= 1;

	if (parameters_length < 12)
		return;
	proto_tree_add_text(parameters_tree, tvb, offset, 12,
	    "agentID: %s", bytes_to_str(parameters, 12));
	offset += 12;
	parameters += 12;
	parameters_length -= 12;

	if (parameters_length < 4)
		return;
	proto_tree_add_text(parameters_tree, tvb, offset, 4,
	    "agentBoots: %u", pntohl(parameters));
	offset += 4;
	parameters += 4;
	parameters_length -= 4;

	if (parameters_length < 4)
		return;
	proto_tree_add_text(parameters_tree, tvb, offset, 4,
	    "agentTime: %u", pntohl(parameters));
	offset += 4;
	parameters += 4;
	parameters_length -= 4;

	if (parameters_length < 2)
		return;
	proto_tree_add_text(parameters_tree, tvb, offset, 2,
	    "maxSize: %u", pntohs(parameters));
	offset += 2;
	parameters += 2;
	parameters_length -= 2;

	if (parameters_length < 1)
		return;
	len = *parameters;
	proto_tree_add_text(parameters_tree, tvb, offset, 1,
	    "userLen: %u", len);
	offset += 1;
	parameters += 1;
	parameters_length -= 1;

	if (parameters_length < len)
		return;
	proto_tree_add_text(parameters_tree, tvb, offset, len,
	    "userName: %.*s", len, parameters);
	offset += len;
	parameters += len;
	parameters_length -= len;

	if (parameters_length < 1)
		return;
	len = *parameters;
	proto_tree_add_text(parameters_tree, tvb, offset, 1,
	    "authLen: %u", len);
	offset += 1;
	parameters += 1;
	parameters_length -= 1;

	if (parameters_length < len)
		return;
	proto_tree_add_text(parameters_tree, tvb, offset, len,
	    "authDigest: %s", bytes_to_str(parameters, len));
	offset += len;
	parameters += len;
	parameters_length -= len;

	if (parameters_length < 1)
		return;
	proto_tree_add_text(parameters_tree, tvb, offset, parameters_length,
	    "contextSelector: %s", bytes_to_str(parameters, parameters_length));
}

void
dissect_snmp_pdu(tvbuff_t *tvb, int offset, packet_info *pinfo,
    proto_tree *tree, char *proto_name, int proto, gint ett)
{
	ASN1_SCK asn1;
	int start;
	gboolean def;
	gboolean encrypted;
	guint length;
	guint message_length;
	guint global_length;

	guint32 version;
	guint32 msgid;
	guint32 msgmax;
	guint32 msgsec;
	guint32 engineboots;
	guint32 enginetime;

	guchar *msgflags;
	guchar *community;
	guchar *secparm;
	guchar *cengineid;
	guchar *cname;
	guchar *cryptpdu;
	guchar *aengineid;
	guchar *username;
	guchar *authpar;
	guchar *privpar;
	int msgflags_length;
	int community_length;
	int secparm_length;
	int cengineid_length;
	int cname_length;
	int cryptpdu_length;
	int aengineid_length;
	int username_length;
	int authpar_length;
	int privpar_length;

	guint pdu_type;
	guint pdu_length;

	proto_tree *snmp_tree = NULL;
	proto_tree *global_tree = NULL;
	proto_tree *flags_tree = NULL;
	proto_tree *secur_tree = NULL;
	proto_item *item = NULL;
	int ret;
	guint cls, con, tag;

	if (check_col(pinfo->cinfo, COL_PROTOCOL))
		col_add_str(pinfo->cinfo, COL_PROTOCOL, proto_name);

	if (tree) {
		item = proto_tree_add_item(tree, proto, tvb, offset, -1, FALSE);
		snmp_tree = proto_item_add_subtree(item, ett);
	}

	/* NOTE: we have to parse the message piece by piece, since the
	 * capture length may be less than the message length: a 'global'
	 * parsing is likely to fail.
	 */
	/* parse the SNMP header */
	asn1_open(&asn1, tvb, offset);
	ret = asn1_sequence_decode(&asn1, &message_length, &length);
	if (ret != ASN1_ERR_NOERROR) {
		dissect_snmp_parse_error(tvb, offset, pinfo, snmp_tree,
			"message header", ret);
		return;
	}
	offset += length;

	ret = asn1_uint32_decode (&asn1, &version, &length);
	if (ret != ASN1_ERR_NOERROR) {
		dissect_snmp_parse_error(tvb, offset, pinfo, snmp_tree,
		    "version number", ret);
		return;
	}
	if (snmp_tree) {
		proto_tree_add_text(snmp_tree, tvb, offset, length,
		    "Version: %s",
		    val_to_str(version, versions, "Unknown version %#x"));
	}
	offset += length;


	switch (version) {
	case SNMP_VERSION_1:
	case SNMP_VERSION_2c:
		ret = asn1_octet_string_decode (&asn1, &community,
		    &community_length, &length);
		if (ret != ASN1_ERR_NOERROR) {
			dissect_snmp_parse_error(tvb, offset, pinfo, snmp_tree,
			    "community", ret);
			return;
		}
		if (tree) {
			proto_tree_add_text(snmp_tree, tvb, offset, length,
			    "Community: %.*s", community_length,
			    SAFE_STRING(community));
		}
		g_free(community);
		offset += length;
		break;
	case SNMP_VERSION_2u:
		ret = asn1_octet_string_decode (&asn1, &community,
		    &community_length, &length);
		if (tree) {
			dissect_snmp2u_parameters(snmp_tree, tvb, offset, length,
			    community, community_length);
		}
		g_free(community);
		offset += length;
		break;
	case SNMP_VERSION_3:
		ret = asn1_sequence_decode(&asn1, &global_length, &length);
		if (ret != ASN1_ERR_NOERROR) {
			dissect_snmp_parse_error(tvb, offset, pinfo, snmp_tree,
				"message global header", ret);
			return;
		}
		if (snmp_tree) {
			item = proto_tree_add_text(snmp_tree, tvb, offset,
			    global_length + length, "Message Global Header");
			global_tree = proto_item_add_subtree(item, ett_global);
			proto_tree_add_text(global_tree, tvb, offset,
		 	    length,
			    "Message Global Header Length: %d", global_length);
		}
		offset += length;
		ret = asn1_uint32_decode (&asn1, &msgid, &length);
		if (ret != ASN1_ERR_NOERROR) {
			dissect_snmp_parse_error(tvb, offset, pinfo, snmp_tree,
			    "message id", ret);
			return;
		}
		if (global_tree) {
			proto_tree_add_text(global_tree, tvb, offset,
			    length, "Message ID: %d", msgid);
		}
		offset += length;
		ret = asn1_uint32_decode (&asn1, &msgmax, &length);
		if (ret != ASN1_ERR_NOERROR) {
			dissect_snmp_parse_error(tvb, offset, pinfo, snmp_tree,
			    "message max size", ret);
			return;
		}
		if (global_tree) {
			proto_tree_add_text(global_tree, tvb, offset,
			    length, "Message Max Size: %d", msgmax);
		}
		offset += length;
		ret = asn1_octet_string_decode (&asn1, &msgflags,
		    &msgflags_length, &length);
		if (ret != ASN1_ERR_NOERROR) {
			dissect_snmp_parse_error(tvb, offset, pinfo, snmp_tree,
			    "message flags", ret);
			return;
		}
		if (msgflags_length != 1) {
			dissect_snmp_parse_error(tvb, offset, pinfo, snmp_tree,
			    "message flags wrong length", ret);
			g_free(msgflags);
			return;
		}
		if (global_tree) {
			item = proto_tree_add_uint_format(global_tree,
			    hf_snmpv3_flags, tvb, offset, length,
			    msgflags[0], "Flags: 0x%02x", msgflags[0]);
			flags_tree = proto_item_add_subtree(item, ett_flags);
			proto_tree_add_boolean(flags_tree, hf_snmpv3_flags_report,
			    tvb, offset, length, msgflags[0]);
			proto_tree_add_boolean(flags_tree, hf_snmpv3_flags_crypt,
			    tvb, offset, length, msgflags[0]);
			proto_tree_add_boolean(flags_tree, hf_snmpv3_flags_auth,
			    tvb, offset, length, msgflags[0]);
		}
		encrypted = msgflags[0] & TH_CRYPT;
		g_free(msgflags);
		offset += length;
		ret = asn1_uint32_decode (&asn1, &msgsec, &length);
		if (ret != ASN1_ERR_NOERROR) {
			dissect_snmp_parse_error(tvb, offset, pinfo, snmp_tree,
			    "message security model", ret);
			return;
		}
		if (global_tree) {
			proto_tree_add_text(global_tree, tvb, offset,
			    length, "Message Security Model: %s",
			    val_to_str(msgsec, sec_models,
			    "Unknown model %#x"));
		}
		offset += length;
		switch(msgsec) {
		case SNMP_SEC_USM:
			start = asn1.offset;
			ret = asn1_header_decode (&asn1, &cls, &con, &tag,
			    &def, &secparm_length);
			length = asn1.offset - start;
			if (cls != ASN1_UNI && con != ASN1_PRI &&
			    tag != ASN1_OTS) {
				dissect_snmp_parse_error(tvb, offset, pinfo,
				    snmp_tree, "Message Security Parameters",
				    ASN1_ERR_WRONG_TYPE);
				return;
			}
			if (snmp_tree) {
				item = proto_tree_add_text(snmp_tree, tvb,
				    offset, secparm_length + length,
				    "Message Security Parameters");
				secur_tree = proto_item_add_subtree(item,
				    ett_secur);
				proto_tree_add_text(secur_tree, tvb, offset,
			 	    length,
				    "Message Security Parameters Length: %d",
				    secparm_length);
			}
			offset += length;
			ret = asn1_sequence_decode(&asn1, &secparm_length,
			    &length);
			if (ret != ASN1_ERR_NOERROR) {
				dissect_snmp_parse_error(tvb, offset, pinfo,
				    snmp_tree, "USM sequence header", ret);
				return;
			}
			offset += length;
			ret = asn1_octet_string_decode (&asn1, &aengineid,
			    &aengineid_length, &length);
			if (ret != ASN1_ERR_NOERROR) {
				dissect_snmp_parse_error(tvb, offset, pinfo,
				    snmp_tree, "authoritative engine id", ret);
				return;
			}
			if (secur_tree) {
				proto_tree_add_text(secur_tree, tvb, offset,
				    length, "Authoritative Engine ID: %s",
				    bytes_to_str(aengineid, aengineid_length));
			}
			g_free(aengineid);
			offset += length;
			ret = asn1_uint32_decode (&asn1, &engineboots, &length);
			if (ret != ASN1_ERR_NOERROR) {
				dissect_snmp_parse_error(tvb, offset, pinfo,
				    snmp_tree, "engine boots", ret);
				return;
			}
			if (secur_tree) {
				proto_tree_add_text(secur_tree, tvb,
				    offset, length, "Engine Boots: %d",
				    engineboots);
			}
			offset += length;
			ret = asn1_uint32_decode (&asn1, &enginetime, &length);
			if (ret != ASN1_ERR_NOERROR) {
				dissect_snmp_parse_error(tvb, offset, pinfo,
				    snmp_tree,  "engine time", ret);
				return;
			}
			if (secur_tree) {
				proto_tree_add_text(secur_tree, tvb,
				    offset, length, "Engine Time: %d",
				    enginetime);
			}
			offset += length;
			ret = asn1_octet_string_decode (&asn1, &username,
			    &username_length, &length);
			if (ret != ASN1_ERR_NOERROR) {
				dissect_snmp_parse_error(tvb, offset, pinfo,
				    snmp_tree, "user name", ret);
				return;
			}
			if (secur_tree) {
				proto_tree_add_text(secur_tree, tvb, offset,
				    length, "User Name: %.*s",
				    username_length,
				    SAFE_STRING(username));
			}
			g_free(username);
			offset += length;
			ret = asn1_octet_string_decode (&asn1, &authpar,
			    &authpar_length, &length);
			if (ret != ASN1_ERR_NOERROR) {
				dissect_snmp_parse_error(tvb, offset, pinfo,
				    snmp_tree, "authentication parameter", ret);
				return;
			}
			if (secur_tree) {
				proto_tree_add_text(secur_tree, tvb, offset,
				    length, "Authentication Parameter: %s",
				    bytes_to_str(authpar, authpar_length));
			}
			g_free(authpar);
			offset += length;
			ret = asn1_octet_string_decode (&asn1, &privpar,
			    &privpar_length, &length);
			if (ret != ASN1_ERR_NOERROR) {
				dissect_snmp_parse_error(tvb, offset, pinfo,
				    snmp_tree, "privacy parameter", ret);
				return;
			}
			if (secur_tree) {
				proto_tree_add_text(secur_tree, tvb, offset,
				    length, "Privacy Parameter: %s",
				    bytes_to_str(privpar, privpar_length));
			}
			g_free(privpar);
			offset += length;
			break;
		default:
			ret = asn1_octet_string_decode (&asn1,
			    &secparm, &secparm_length, &length);
			if (ret != ASN1_ERR_NOERROR) {
				dissect_snmp_parse_error(tvb, offset, pinfo,
				    snmp_tree, "Message Security Parameters",
				    ret);
				return;
			}
			if (snmp_tree) {
				proto_tree_add_text(snmp_tree, tvb, offset,
				    length,
				    "Message Security Parameters Data"
				    " (%d bytes)", secparm_length);
			}
			g_free(secparm);
			offset += length;
			break;
		}
		/* PDU starts here */
		if (encrypted) {
			ret = asn1_octet_string_decode (&asn1, &cryptpdu,
			    &cryptpdu_length, &length);
			if (ret != ASN1_ERR_NOERROR) {
				dissect_snmp_parse_error(tvb, offset, pinfo,
				    snmp_tree, "encrypted PDU header", ret);
				return;
			}
			proto_tree_add_text(snmp_tree, tvb, offset, length,
			    "Encrypted PDU (%d bytes)", length);
			g_free(cryptpdu);
			if (check_col(pinfo->cinfo, COL_INFO))
				col_set_str(pinfo->cinfo, COL_INFO, "Encrypted PDU");
			return;
		}
		ret = asn1_sequence_decode(&asn1, &global_length, &length);
		if (ret != ASN1_ERR_NOERROR) {
			dissect_snmp_parse_error(tvb, offset, pinfo, snmp_tree,
				"PDU header", ret);
			return;
		}
		offset += length;
		ret = asn1_octet_string_decode (&asn1, &cengineid,
		    &cengineid_length, &length);
		if (ret != ASN1_ERR_NOERROR) {
			dissect_snmp_parse_error(tvb, offset, pinfo, snmp_tree,
			    "context engine id", ret);
			return;
		}
		if (snmp_tree) {
			proto_tree_add_text(snmp_tree, tvb, offset, length,
			    "Context Engine ID: %s",
			    bytes_to_str(cengineid, cengineid_length));
		}
		g_free(cengineid);
		offset += length;
		ret = asn1_octet_string_decode (&asn1, &cname,
		    &cname_length, &length);
		if (ret != ASN1_ERR_NOERROR) {
			dissect_snmp_parse_error(tvb, offset, pinfo, snmp_tree,
			    "context name", ret);
			return;
		}
		if (snmp_tree) {
			proto_tree_add_text(snmp_tree, tvb, offset, length,
			    "Context Name: %.*s", cname_length,
			    SAFE_STRING(cname));
		}
		g_free(cname);
		offset += length;
		break;
	default:
		dissect_snmp_error(tvb, offset, pinfo, snmp_tree,
		    "PDU for unknown version of SNMP");
		return;
	}

	start = asn1.offset;
	ret = asn1_header_decode (&asn1, &cls, &con, &pdu_type, &def,
	    &pdu_length);
	if (ret != ASN1_ERR_NOERROR) {
		dissect_snmp_parse_error(tvb, offset, pinfo, snmp_tree,
		    "PDU type", ret);
		return;
	}
	if (cls != ASN1_CTX || con != ASN1_CON) {
		dissect_snmp_parse_error(tvb, offset, pinfo, snmp_tree,
		    "PDU type", ASN1_ERR_WRONG_TYPE);
		return;
	}
	dissect_common_pdu(tvb, offset, pinfo, snmp_tree, asn1, pdu_type, start);
}

static void
dissect_smux_pdu(tvbuff_t *tvb, int offset, packet_info *pinfo,
    proto_tree *tree, int proto, gint ett)
{
	ASN1_SCK asn1;
	int start;
	gboolean def;
	guint length;

	guint pdu_type;
	char *pdu_type_string;
	guint pdu_length;

	guint32 version;
	guint32 cause;
	guint32 priority;
	guint32 operation;
	guint32 commit;

	guchar *password;
	int password_length;

	guchar *application;
	int application_length;

	subid_t *regid;
	guint regid_length;

	gchar *oid_string;

	proto_tree *smux_tree = NULL;
	proto_item *item = NULL;
	int ret;
	guint cls, con;

	if (check_col(pinfo->cinfo, COL_PROTOCOL))
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "SMUX");

	if (tree) {
		item = proto_tree_add_item(tree, proto, tvb, offset, -1, FALSE);
		smux_tree = proto_item_add_subtree(item, ett);
	}

	/* NOTE: we have to parse the message piece by piece, since the
	 * capture length may be less than the message length: a 'global'
	 * parsing is likely to fail.
	 */
	/* parse the SNMP header */
	asn1_open(&asn1, tvb, offset);
	start = asn1.offset;
	ret = asn1_header_decode (&asn1, &cls, &con, &pdu_type, &def,
	    &pdu_length);
	if (ret != ASN1_ERR_NOERROR) {
		dissect_snmp_parse_error(tvb, offset, pinfo, smux_tree,
		    "PDU type", ret);
		return;
	}

	/* Dissect SMUX here */
	if (cls == ASN1_APL && con == ASN1_CON && pdu_type == SMUX_MSG_OPEN) {
		pdu_type_string = val_to_str(pdu_type, smux_types,
		    "Unknown PDU type %#x");
		if (check_col(pinfo->cinfo, COL_INFO))
			col_add_str(pinfo->cinfo, COL_INFO, pdu_type_string);
		length = asn1.offset - start;
		if (tree) {
			proto_tree_add_text(smux_tree, tvb, offset, length,
			    "PDU type: %s", pdu_type_string);
		}
		offset += length;
		ret = asn1_uint32_decode (&asn1, &version, &length);
		if (ret != ASN1_ERR_NOERROR) {
			dissect_snmp_parse_error(tvb, offset, pinfo, smux_tree,
			    "version", ret);
			return;
		}
		if (tree) {
			proto_tree_add_text(smux_tree, tvb, offset, length,
			    "Version: %d", version);
		}
		offset += length;

		ret = asn1_oid_decode (&asn1, &regid, &regid_length, &length);
		if (ret != ASN1_ERR_NOERROR) {
			dissect_snmp_parse_error(tvb, offset, pinfo, smux_tree,
			    "registration OID", ret);
			return;
		}
		if (tree) {
			oid_string = format_oid(regid, regid_length);
			proto_tree_add_text(smux_tree, tvb, offset, length,
			    "Registration: %s", oid_string);
			g_free(oid_string);
		}
		g_free(regid);
		offset += length;

		ret = asn1_octet_string_decode (&asn1, &application,
		    &application_length, &length);
		if (ret != ASN1_ERR_NOERROR) {
			dissect_snmp_parse_error(tvb, offset, pinfo, smux_tree,
			    "application", ret);
			return;
		}
		if (tree) {
			proto_tree_add_text(smux_tree, tvb, offset, length,
			    "Application: %.*s", application_length,
			     SAFE_STRING(application));
		}
		g_free(application);
		offset += length;

		ret = asn1_octet_string_decode (&asn1, &password,
		    &password_length, &length);
		if (ret != ASN1_ERR_NOERROR) {
			dissect_snmp_parse_error(tvb, offset, pinfo, smux_tree,
			    "password", ret);
			return;
		}
		if (tree) {
			proto_tree_add_text(smux_tree, tvb, offset, length,
			    "Password: %.*s", password_length,
			    SAFE_STRING(password));
		}
		g_free(password);
		offset += length;
		return;
	}
	if (cls == ASN1_APL && con == ASN1_PRI && pdu_type == SMUX_MSG_CLOSE) {
		pdu_type_string = val_to_str(pdu_type, smux_types,
		    "Unknown PDU type %#x");
		if (check_col(pinfo->cinfo, COL_INFO))
			col_add_str(pinfo->cinfo, COL_INFO, pdu_type_string);
		length = asn1.offset - start;
		if (tree) {
			proto_tree_add_text(smux_tree, tvb, offset, length,
			    "PDU type: %s", pdu_type_string);
		}
		offset += length;
		ret = asn1_uint32_value_decode (&asn1, pdu_length, &cause);
		if (ret != ASN1_ERR_NOERROR) {
			dissect_snmp_parse_error(tvb, offset, pinfo, smux_tree,
			    "cause", ret);
			return;
		}
		if (tree) {
			proto_tree_add_text(smux_tree, tvb, offset,
			    pdu_length, "Cause: %s",
			    val_to_str(cause, smux_close,
				"Unknown cause %#x"));
		}
		offset += pdu_length;
		return;
	}
	if (cls == ASN1_APL && con == ASN1_CON && pdu_type == SMUX_MSG_RREQ) {
		pdu_type_string = val_to_str(pdu_type, smux_types,
		    "Unknown PDU type %#x");
		if (check_col(pinfo->cinfo, COL_INFO))
			col_add_str(pinfo->cinfo, COL_INFO, pdu_type_string);
		length = asn1.offset - start;
		if (tree) {
			proto_tree_add_text(smux_tree, tvb, offset, length,
			    "PDU type: %s", pdu_type_string);
		}
		offset += length;
		ret = asn1_oid_decode (&asn1, &regid, &regid_length, &length);
		if (ret != ASN1_ERR_NOERROR) {
			dissect_snmp_parse_error(tvb, offset, pinfo, smux_tree,
			    "registration subtree", ret);
			return;
		}
		if (tree) {
			oid_string = format_oid(regid, regid_length);
			proto_tree_add_text(smux_tree, tvb, offset, length,
			    "Registration: %s", oid_string);
			g_free(oid_string);
		}
		g_free(regid);
		offset += length;

		ret = asn1_uint32_decode (&asn1, &priority, &length);
		if (ret != ASN1_ERR_NOERROR) {
			dissect_snmp_parse_error(tvb, offset, pinfo, smux_tree,
			    "priority", ret);
			return;
		}
		if (tree) {
			proto_tree_add_text(smux_tree, tvb, offset, length,
			    "Priority: %d", priority);
		}
		offset += length;

		ret = asn1_uint32_decode (&asn1, &operation, &length);
		if (ret != ASN1_ERR_NOERROR) {
			dissect_snmp_parse_error(tvb, offset, pinfo, smux_tree,
			    "operation", ret);
			return;
		}
		if (tree) {
			proto_tree_add_text(smux_tree, tvb, offset, length,
			    "Operation: %s",
			    val_to_str(operation, smux_rreq,
				"Unknown operation %#x"));
		}
		offset += length;
		return;
	}
	if (cls == ASN1_APL && con == ASN1_PRI && pdu_type == SMUX_MSG_RRSP) {
		pdu_type_string = val_to_str(pdu_type, smux_types,
		    "Unknown PDU type %#x");
		if (check_col(pinfo->cinfo, COL_INFO))
			col_add_str(pinfo->cinfo, COL_INFO, pdu_type_string);
		length = asn1.offset - start;
		if (tree) {
			proto_tree_add_text(smux_tree, tvb, offset, length,
			    "PDU type: %s", pdu_type_string);
		}
		offset += length;
		ret = asn1_uint32_value_decode (&asn1, pdu_length, &priority);
		if (ret != ASN1_ERR_NOERROR) {
			dissect_snmp_parse_error(tvb, offset, pinfo, smux_tree,
			    "priority", ret);
			return;
		}
		if (tree) {
			proto_tree_add_text(smux_tree, tvb, offset,
			    pdu_length, "%s",
			    val_to_str(priority, smux_prio,
				"Priority: %#x"));
		}
		offset += pdu_length;
		return;
	}
	if (cls == ASN1_APL && con == ASN1_PRI && pdu_type == SMUX_MSG_SOUT) {
		pdu_type_string = val_to_str(pdu_type, smux_types,
		    "Unknown PDU type %#x");
		if (check_col(pinfo->cinfo, COL_INFO))
			col_add_str(pinfo->cinfo, COL_INFO, pdu_type_string);
		length = asn1.offset - start;
		if (tree) {
			proto_tree_add_text(smux_tree, tvb, offset, length,
			    "PDU type: %s", pdu_type_string);
		}
		offset += length;
		ret = asn1_uint32_value_decode (&asn1, pdu_length, &commit);
		if (ret != ASN1_ERR_NOERROR) {
			dissect_snmp_parse_error(tvb, offset, pinfo, smux_tree,
			    "commit", ret);
			return;
		}
		if (tree) {
			proto_tree_add_text(smux_tree, tvb, offset,
			    pdu_length, "%s",
			    val_to_str(commit, smux_sout,
				"Unknown SOUT Value: %#x"));
		}
		offset += pdu_length;
		return;
	}
	if (cls != ASN1_CTX || con != ASN1_CON) {
		dissect_snmp_parse_error(tvb, offset, pinfo, smux_tree,
		    "PDU type", ASN1_ERR_WRONG_TYPE);
		return;
	}
	dissect_common_pdu(tvb, offset, pinfo, smux_tree, asn1, pdu_type, start);
}

static void
dissect_snmp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	conversation_t  *conversation;

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
	  conversation = find_conversation(&pinfo->src, &pinfo->dst, PT_UDP,
					   pinfo->srcport, 0, NO_PORT_B);
	  if (conversation == NULL) {
	    conversation = conversation_new(&pinfo->src, &pinfo->dst, PT_UDP,
					    pinfo->srcport, 0, NO_PORT2);
	    conversation_set_dissector(conversation, snmp_handle);
	  }
	}

	dissect_snmp_pdu(tvb, 0, pinfo, tree, "SNMP", proto_snmp, ett_snmp);
}

static void
dissect_smux(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	dissect_smux_pdu(tvb, 0, pinfo, tree, proto_smux, ett_smux);
}

void
proto_register_snmp(void)
{
#if defined(WIN32) && defined(HAVE_SOME_SNMP)
	char *mib_path;
#define MIB_PATH_APPEND "snmp\\mibs"
#endif

	static hf_register_info hf[] = {
		{ &hf_snmpv3_flags,
		{ "SNMPv3 Flags", "snmpv3.flags", FT_UINT8, BASE_HEX, NULL,
		    0x0, "", HFILL }},
		{ &hf_snmpv3_flags_auth,
		{ "Authenticated", "snmpv3.flags.auth", FT_BOOLEAN, 8,
		    TFS(&flags_set_truth), TH_AUTH, "", HFILL }},
		{ &hf_snmpv3_flags_crypt,
		{ "Encrypted", "snmpv3.flags.crypt", FT_BOOLEAN, 8,
		    TFS(&flags_set_truth), TH_CRYPT, "", HFILL }},
		{ &hf_snmpv3_flags_report,
		{ "Reportable", "snmpv3.flags.report", FT_BOOLEAN, 8,
		    TFS(&flags_set_truth), TH_REPORT, "", HFILL }},
        };
	static gint *ett[] = {
		&ett_snmp,
		&ett_smux,
		&ett_parameters,
		&ett_parameters_qos,
		&ett_global,
		&ett_flags,
		&ett_secur,
	};

#ifdef HAVE_SOME_SNMP

#ifdef WIN32
	/* Set MIBDIRS so that the SNMP library can find its mibs. */
	/* XXX - Should we set MIBS or MIBFILES as well? */

	mib_path = g_malloc (strlen(get_datafile_dir()) + strlen(MIB_PATH_APPEND) + 20);
	sprintf (mib_path, "MIBDIRS=%s\\%s", get_datafile_dir(), MIB_PATH_APPEND);
	/* Amazingly enough, Windows does not provide setenv(). */
	if (getenv("MIBDIRS") == NULL)
		_putenv(mib_path);
	g_free(mib_path);

#endif	/* WIN32 */

	/*
	 * Suppress warnings about unknown tokens - we aren't initializing
	 * UCD SNMP in its entirety, we're just initializing the
	 * MIB-handling part because that's all we're using, which
	 * means that entries in the configuration file for other
	 * pars of the library will not be handled, and we don't want
	 * the config file reading code to whine about that.
	 */
	netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID,
                               NETSNMP_DS_LIB_NO_TOKEN_WARNINGS, TRUE);
	netsnmp_ds_set_int(NETSNMP_DS_LIBRARY_ID,
                           NETSNMP_DS_LIB_PRINT_SUFFIX_ONLY, 2);
	register_mib_handlers();
	read_premib_configs();
	init_mib();
	read_configs();
#endif /* HAVE_SOME_SNMP */
        proto_snmp = proto_register_protocol("Simple Network Management Protocol",
	    "SNMP", "snmp");
        proto_smux = proto_register_protocol("SNMP Multiplex Protocol",
	    "SMUX", "smux");
        proto_register_field_array(proto_snmp, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));
	snmp_handle = create_dissector_handle(dissect_snmp, proto_snmp);
}

void
proto_reg_handoff_snmp(void)
{
	dissector_handle_t smux_handle;

	dissector_add("udp.port", UDP_PORT_SNMP, snmp_handle);
	dissector_add("udp.port", UDP_PORT_SNMP_TRAP, snmp_handle);
	smux_handle = create_dissector_handle(dissect_smux, proto_smux);
	dissector_add("tcp.port", TCP_PORT_SMUX, smux_handle);
	dissector_add("ethertype", ETHERTYPE_SNMP, snmp_handle);
	dissector_add("ipx.socket", IPX_SOCKET_SNMP_AGENT, snmp_handle);
	dissector_add("ipx.socket", IPX_SOCKET_SNMP_SINK, snmp_handle);
	data_handle = find_dissector("data");
}
