/* packet-ldap.c
 * Routines for ldap packet dissection
 *
 * See RFC 1777 (LDAP v2), RFC 2251 (LDAP v3), and RFC 2222 (SASL).
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/*
 * This is not a complete implementation. It doesn't handle the full version 3, more specifically,
 * it handles only the commands of version 2, but any additional characteristics of the ver3 command are supported.
 * It's also missing extensible search filters.
 *
 * There should probably be alot more error checking, I simply assume that if we have a full packet, it will be a complete
 * and correct packet.
 *
 * AFAIK, it will handle all messages used by the OpenLDAP 1.2.9 server and libraries which was my goal. I do plan to add
 * the remaining commands as time permits but this is not a priority to me. Send me an email if you need it and I'll see what
 * I can do.
 *
 * Doug Nazar
 * nazard@dragoninc.on.ca
 */

/*
 * 11/11/2002 - Fixed problem when decoding LDAP with desegmentation enabled and the
 *              ASN.1 BER Universal Class Tag: "Sequence Of" header is encapsulated across 2
 *              TCP segments.
 *
 * Ronald W. Henderson
 * ronald.henderson@cognicaseusa.com
 */

/*
 * 20-JAN-2004 - added decoding of MS-CLDAP netlogon RPC
 *               using information from the SNIA 2003 conference paper :
 *               Active Directory Domain Controller Location Service
 *                    by Anthony Liguori
 * ronnie sahlberg
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>

#include <string.h>
#include <glib.h>

#ifdef NEED_SNPRINTF_H
# include "snprintf.h"
#endif

#include <epan/packet.h>

#include "asn1.h"
#include "prefs.h"
#include <epan/conversation.h>
#include "packet-frame.h"
#include "tap.h"
#include "packet-ldap.h"

static int proto_ldap = -1;
static int proto_cldap = -1;
static int hf_ldap_response_to = -1;
static int hf_ldap_response_in = -1;
static int hf_ldap_time = -1;
static int hf_ldap_sasl_buffer_length = -1;
static int hf_ldap_length = -1;
static int hf_ldap_message_id = -1;
static int hf_ldap_message_type = -1;
static int hf_ldap_message_length = -1;

static int hf_ldap_message_result = -1;
static int hf_ldap_message_result_matcheddn = -1;
static int hf_ldap_message_result_errormsg = -1;
static int hf_ldap_message_result_referral = -1;

static int hf_ldap_message_bind_version = -1;
static int hf_ldap_message_bind_dn = -1;
static int hf_ldap_message_bind_auth = -1;
static int hf_ldap_message_bind_auth_password = -1;
static int hf_ldap_message_bind_auth_mechanism = -1;
static int hf_ldap_message_bind_auth_credentials = -1;
static int hf_ldap_message_bind_server_credentials = -1;

static int hf_ldap_message_search_base = -1;
static int hf_ldap_message_search_scope = -1;
static int hf_ldap_message_search_deref = -1;
static int hf_ldap_message_search_sizeLimit = -1;
static int hf_ldap_message_search_timeLimit = -1;
static int hf_ldap_message_search_typesOnly = -1;
static int hf_ldap_message_search_filter = -1;
static int hf_ldap_message_search_reference = -1;

static int hf_ldap_message_dn = -1;
static int hf_ldap_message_attribute = -1;
static int hf_ldap_message_value = -1;

static int hf_ldap_message_modrdn_name = -1;
static int hf_ldap_message_modrdn_delete = -1;
static int hf_ldap_message_modrdn_superior = -1;

static int hf_ldap_message_compare = -1;

static int hf_ldap_message_modify_add = -1;
static int hf_ldap_message_modify_replace = -1;
static int hf_ldap_message_modify_delete = -1;

static int hf_ldap_message_abandon_msgid = -1;

static int hf_mscldap_netlogon_type = -1;
static int hf_mscldap_netlogon_flags = -1;
static int hf_mscldap_netlogon_flags_pdc = -1;
static int hf_mscldap_netlogon_flags_gc = -1;
static int hf_mscldap_netlogon_flags_ldap = -1;
static int hf_mscldap_netlogon_flags_ds = -1;
static int hf_mscldap_netlogon_flags_kdc = -1;
static int hf_mscldap_netlogon_flags_timeserv = -1;
static int hf_mscldap_netlogon_flags_closest = -1;
static int hf_mscldap_netlogon_flags_writable = -1;
static int hf_mscldap_netlogon_flags_good_timeserv = -1;
static int hf_mscldap_netlogon_flags_ndnc = -1;
static int hf_mscldap_domain_guid = -1;
static int hf_mscldap_forest = -1;
static int hf_mscldap_domain = -1;
static int hf_mscldap_hostname = -1;
static int hf_mscldap_nb_domain = -1;
static int hf_mscldap_nb_hostname = -1;
static int hf_mscldap_username = -1;
static int hf_mscldap_sitename = -1;
static int hf_mscldap_clientsitename = -1;
static int hf_mscldap_netlogon_version = -1;
static int hf_mscldap_netlogon_lm_token = -1;
static int hf_mscldap_netlogon_nt_token = -1;

static gint ett_ldap = -1;
static gint ett_ldap_gssapi_token = -1;
static gint ett_ldap_referrals = -1;
static gint ett_ldap_attribute = -1;
static gint ett_mscldap_netlogon_flags = -1;

static int ldap_tap = -1;

/* desegmentation of LDAP */
static gboolean ldap_desegment = TRUE;

#define TCP_PORT_LDAP			389
#define UDP_PORT_CLDAP			389
#define TCP_PORT_GLOBALCAT_LDAP         3268 /* Windows 2000 Global Catalog */

static dissector_handle_t gssapi_handle;
static dissector_handle_t gssapi_wrap_handle;


/* different types of rpc calls ontop of ms cldap */
#define	MSCLDAP_RPC_NETLOGON 	1


/*
 * Data structure attached to a conversation, giving authentication
 * information from a bind request.
 * We keep a linked list of them, so that we can free up all the
 * authentication mechanism strings.
 */
typedef struct ldap_conv_info_t {
  struct ldap_conv_info_t *next;
  guint auth_type;		/* authentication type */
  char *auth_mech;		/* authentication mechanism */
  guint32 first_auth_frame;	/* first frame that would use a security layer */
  GHashTable *unmatched;
  GHashTable *matched;
} ldap_conv_info_t;
static GMemChunk *ldap_conv_info_chunk = NULL;
static guint ldap_conv_info_chunk_count = 20;
static ldap_conv_info_t *ldap_info_items;

static GMemChunk *ldap_call_response_chunk = NULL;
static guint ldap_call_response_chunk_count = 200;

static guint
ldap_info_hash_matched(gconstpointer k)
{
  ldap_call_response_t *key = (ldap_call_response_t *)k;

  return key->messageId;
}

static gint
ldap_info_equal_matched(gconstpointer k1, gconstpointer k2)
{
  ldap_call_response_t *key1 = (ldap_call_response_t *)k1;
  ldap_call_response_t *key2 = (ldap_call_response_t *)k2;

  if( key1->req_frame && key2->req_frame && (key1->req_frame!=key2->req_frame) ){
    return 0;
  }
  if( key1->rep_frame && key2->rep_frame && (key1->rep_frame!=key2->rep_frame) ){
    return 0;
  }

  return key1->messageId==key2->messageId;
}

static guint
ldap_info_hash_unmatched(gconstpointer k)
{
  ldap_call_response_t *key = (ldap_call_response_t *)k;

  return key->messageId;
}

static gint
ldap_info_equal_unmatched(gconstpointer k1, gconstpointer k2)
{
  ldap_call_response_t *key1 = (ldap_call_response_t *)k1;
  ldap_call_response_t *key2 = (ldap_call_response_t *)k2;

  return key1->messageId==key2->messageId;
}


static value_string msgTypes [] = {
  {LDAP_REQ_BIND, "Bind Request"},
  {LDAP_REQ_UNBIND, "Unbind Request"},
  {LDAP_REQ_SEARCH, "Search Request"},
  {LDAP_REQ_MODIFY, "Modify Request"},
  {LDAP_REQ_ADD, "Add Request"},
  {LDAP_REQ_DELETE, "Delete Request"},
  {LDAP_REQ_MODRDN, "Modify RDN Request"},
  {LDAP_REQ_COMPARE, "Compare Request"},
  {LDAP_REQ_ABANDON, "Abandon Request"},
  {LDAP_REQ_EXTENDED, "Extended Request"},

  {LDAP_RES_BIND, "Bind Result"},
  {LDAP_RES_SEARCH_ENTRY, "Search Entry"},
  {LDAP_RES_SEARCH_RESULT, "Search Result"},
  {LDAP_RES_SEARCH_REF, "Search Result Reference"},
  {LDAP_RES_MODIFY, "Modify Result"},
  {LDAP_RES_ADD, "Add Result"},
  {LDAP_RES_DELETE, "Delete Result"},
  {LDAP_RES_MODRDN, "Modify RDN Result"},
  {LDAP_RES_COMPARE, "Compare Result"},
  {LDAP_RES_EXTENDED, "Extended Response"},
  {0, NULL},
};

static value_string result_codes[] = {
    {0, "Success"},
    {1, "Operations error"},
    {2, "Protocol error"},
    {3, "Time limit exceeded"},
    {4, "Size limit exceeded"},
    {5, "Compare false"},
    {6, "Compare true"},
    {7, "Authentication method not supported"},
    {8, "Strong authentication required"},
    {10, "Referral"},
    {11, "Administrative limit exceeded"},
    {12, "Unavailable critical extension"},
    {13, "Confidentiality required"},
    {14, "SASL bind in progress"},
    {16, "No such attribute"},
    {17, "Undefined attribute type"},
    {18, "Inappropriate matching"},
    {19, "Constraint violation"},
    {20, "Attribute or value exists"},
    {21, "Invalid attribute syntax"},
    {32, "No such object"},
    {33, "Alias problem"},
    {34, "Invalid DN syntax"},
    {36, "Alias derefetencing problem"},
    {48, "Inappropriate authentication"},
    {49, "Invalid credentials"},
    {50, "Insufficient access rights"},
    {51, "Busy"},
    {52, "Unavailable"},
    {53, "Unwilling to perform"},
    {54, "Loop detected"},
    {64, "Naming violation"},
    {65, "Objectclass violation"},
    {66, "Not allowed on non-leaf"},
    {67, "Not allowed on RDN"},
    {68, "Entry already exists"},
    {69, "Objectclass modification prohibited"},
    {71, "Affects multiple DSAs"},
    {80, "Other"},
    {0,  NULL},
};

static int read_length(ASN1_SCK *a, proto_tree *tree, int hf_id, guint *len)
{
  guint length = 0;
  gboolean def = FALSE;
  int start = a->offset;
  int ret;

  ret = asn1_length_decode(a, &def, &length);
  if (ret != ASN1_ERR_NOERROR) {
    if (tree) {
      proto_tree_add_text(tree, a->tvb, start, 0,
        "%s: ERROR: Couldn't parse length: %s",
        proto_registrar_get_name(hf_id), asn1_err_to_str(ret));
    }
    return ret;
  }

  if (len)
    *len = length;

  if (tree)
    proto_tree_add_uint(tree, hf_id, a->tvb, start, a->offset-start, length);

  return ASN1_ERR_NOERROR;
}

static int read_sequence(ASN1_SCK *a, guint *len)
{
  guint cls, con, tag;
  gboolean def;
  guint length;
  int ret;

  ret = asn1_header_decode(a, &cls, &con, &tag, &def, &length);
  if (ret != ASN1_ERR_NOERROR)
    return ret;
  if (cls != ASN1_UNI || con != ASN1_CON || tag != ASN1_SEQ)
    return ASN1_ERR_WRONG_TYPE;

  if (len)
    *len = length;

  return ASN1_ERR_NOERROR;
}

static int read_set(ASN1_SCK *a, guint *len)
{
  guint cls, con, tag;
  gboolean def;
  guint length;
  int ret;

  ret = asn1_header_decode(a, &cls, &con, &tag, &def, &length);
  if (ret != ASN1_ERR_NOERROR)
    return ret;
  if (cls != ASN1_UNI || con != ASN1_CON || tag != ASN1_SET)
    return ASN1_ERR_WRONG_TYPE;

  if (len)
    *len = length;

  return ASN1_ERR_NOERROR;
}

static int read_integer_value(ASN1_SCK *a, proto_tree *tree, int hf_id,
	proto_item **new_item, guint *i, int start, guint length)
{
  guint integer = 0;
  proto_item *temp_item = NULL;
  int ret;

  ret = asn1_uint32_value_decode(a, length, &integer);
  if (ret != ASN1_ERR_NOERROR) {
    if (tree) {
      proto_tree_add_text(tree, a->tvb, start, 0,
       "%s: ERROR: Couldn't parse value: %s",
        proto_registrar_get_name(hf_id), asn1_err_to_str(ret));
    }
    return ret;
  }

  if (i)
    *i = integer;

  if (tree)
    temp_item = proto_tree_add_uint(tree, hf_id, a->tvb, start, a->offset-start, integer);

  if (new_item)
    *new_item = temp_item;

  return ASN1_ERR_NOERROR;
}

static int read_integer(ASN1_SCK *a, proto_tree *tree, int hf_id,
	proto_item **new_item, guint *i, guint expected_tag)
{
  guint cls, con, tag;
  gboolean def;
  guint length;
  int start = a->offset;
  int ret;

  ret = asn1_header_decode(a, &cls, &con, &tag, &def, &length);
  if (ret == ASN1_ERR_NOERROR) {
    if (cls != ASN1_UNI || con != ASN1_PRI || tag != expected_tag)
      ret = ASN1_ERR_WRONG_TYPE;
  }
  if (ret != ASN1_ERR_NOERROR) {
    if (tree) {
      proto_tree_add_text(tree, a->tvb, start, 0,
        "%s: ERROR: Couldn't parse header: %s",
        (hf_id != -1) ? proto_registrar_get_name(hf_id) : "LDAP message",
        asn1_err_to_str(ret));
    }
    return ret;
  }

  return read_integer_value(a, tree, hf_id, new_item, i, start, length);
}

static int read_boolean_value(ASN1_SCK *a, proto_tree *tree, int hf_id,
	proto_item **new_item, guint *i, int start, guint length)
{
  guint integer = 0;
  proto_item *temp_item = NULL;
  int ret;

  ret = asn1_uint32_value_decode(a, length, &integer);
  if (ret != ASN1_ERR_NOERROR) {
    if (tree) {
      proto_tree_add_text(tree, a->tvb, start, 0,
        "%s: ERROR: Couldn't parse value: %s",
        proto_registrar_get_name(hf_id), asn1_err_to_str(ret));
    }
    return ret;
  }

  if (i)
    *i = integer;

  if (tree)
    temp_item = proto_tree_add_boolean(tree, hf_id, a->tvb, start, a->offset-start, integer);
  if (new_item)
    *new_item = temp_item;

  return ASN1_ERR_NOERROR;
}

static int read_boolean(ASN1_SCK *a, proto_tree *tree, int hf_id,
	proto_item **new_item, guint *i)
{
  guint cls, con, tag;
  gboolean def;
  guint length;
  int start = a->offset;
  int ret;

  ret = asn1_header_decode(a, &cls, &con, &tag, &def, &length);
  if (ret == ASN1_ERR_NOERROR) {
    if (cls != ASN1_UNI || con != ASN1_PRI || tag != ASN1_BOL)
      ret = ASN1_ERR_WRONG_TYPE;
  }
  if (ret != ASN1_ERR_NOERROR) {
    if (tree) {
      proto_tree_add_text(tree, a->tvb, start, 0,
        "%s: ERROR: Couldn't parse header: %s",
        proto_registrar_get_name(hf_id), asn1_err_to_str(ret));
    }
    return ret;
  }

  return read_boolean_value(a, tree, hf_id, new_item, i, start, length);
}

static int read_string_value(ASN1_SCK *a, proto_tree *tree, int hf_id,
	proto_item **new_item, char **s, int start, guint length)
{
  guchar *string;
  proto_item *temp_item = NULL;
  int ret;

  if (length)
  {
    ret = asn1_string_value_decode(a, length, &string);
    if (ret != ASN1_ERR_NOERROR) {
      if (tree) {
        proto_tree_add_text(tree, a->tvb, start, 0,
          "%s: ERROR: Couldn't parse value: %s",
          proto_registrar_get_name(hf_id), asn1_err_to_str(ret));
      }
      return ret;
    }
    string = g_realloc(string, length + 1);
    string[length] = '\0';
  }
  else
    string = "(null)";

  if (tree)
    temp_item = proto_tree_add_string(tree, hf_id, a->tvb, start, a->offset - start, string);
  if (new_item)
    *new_item = temp_item;

  if (s && length)
    *s = string;
  else if (length)
    g_free(string);

  return ASN1_ERR_NOERROR;
}

static int read_string(ASN1_SCK *a, proto_tree *tree, int hf_id,
	proto_item **new_item, char **s, guint *length,
	guint expected_cls, guint expected_tag)
{
  guint cls, con, tag;
  gboolean def;
  guint tmplen;
  int start = a->offset;
  int ret;

  ret = asn1_header_decode(a, &cls, &con, &tag, &def, &tmplen);
  if (ret == ASN1_ERR_NOERROR) {
    if (cls != expected_cls || con != ASN1_PRI || tag != expected_tag)
      ret = ASN1_ERR_WRONG_TYPE;
  }
  if (ret != ASN1_ERR_NOERROR) {
    if (tree) {
      proto_tree_add_text(tree, a->tvb, start, 0,
        "%s: ERROR: Couldn't parse header: %s",
        proto_registrar_get_name(hf_id), asn1_err_to_str(ret));
    }
    return ret;
  }

  if(length){
     *length=tmplen;
  }
  return read_string_value(a, tree, hf_id, new_item, s, start, tmplen);
}

static int read_bytestring_value(ASN1_SCK *a, proto_tree *tree, int hf_id,
	proto_item **new_item, char **s, int start, guint length)
{
  guchar *string;
  proto_item *temp_item = NULL;
  int ret;

  if (length)
  {
    ret = asn1_string_value_decode(a, length, &string);
    if (ret != ASN1_ERR_NOERROR) {
      if (tree) {
        proto_tree_add_text(tree, a->tvb, start, 0,
          "%s: ERROR: Couldn't parse value: %s",
          proto_registrar_get_name(hf_id), asn1_err_to_str(ret));
      }
      return ret;
    }
    string = g_realloc(string, length + 1);
    string[length] = '\0';
  }
  else
    string = "(null)";

  if (tree)
    temp_item = proto_tree_add_bytes(tree, hf_id, a->tvb, start, a->offset - start, string);
  if (new_item)
    *new_item = temp_item;

  if (s && length)
    *s = string;
  else if (length)
    g_free(string);

  return ASN1_ERR_NOERROR;
}

static int read_bytestring(ASN1_SCK *a, proto_tree *tree, int hf_id,
	proto_item **new_item, char **s, guint expected_cls, guint expected_tag)
{
  guint cls, con, tag;
  gboolean def;
  guint length;
  int start = a->offset;
  int ret;

  ret = asn1_header_decode(a, &cls, &con, &tag, &def, &length);
  if (ret == ASN1_ERR_NOERROR) {
    if (cls != expected_cls || con != ASN1_PRI || tag != expected_tag)
      ret = ASN1_ERR_WRONG_TYPE;
  }
  if (ret != ASN1_ERR_NOERROR) {
    if (tree) {
      proto_tree_add_text(tree, a->tvb, start, 0,
        "%s: ERROR: Couldn't parse header: %s",
        proto_registrar_get_name(hf_id), asn1_err_to_str(ret));
    }
    return ret;
  }

  return read_bytestring_value(a, tree, hf_id, new_item, s, start, length);
}

static int parse_filter_strings(ASN1_SCK *a, char **filter, guint *filter_length, const guchar *operation)
{
  guchar *string;
  guchar *string2;
  guint string_length;
  guint string2_length;
  guint string_bytes;
  char *filterp;
  int ret;

  ret = asn1_octet_string_decode(a, &string, &string_length, &string_bytes);
  if (ret != ASN1_ERR_NOERROR)
    return ret;
  ret = asn1_octet_string_decode(a, &string2, &string2_length, &string_bytes);
  if (ret != ASN1_ERR_NOERROR)
    return ret;
  *filter_length += 2 + strlen(operation) + string_length + string2_length;
  *filter = g_realloc(*filter, *filter_length);
  filterp = *filter + strlen(*filter);
  *filterp++ = '(';
  if (string_length != 0) {
  	memcpy(filterp, string, string_length);
  	filterp += string_length;
  }
  strcpy(filterp, operation);
  filterp += strlen(operation);
  if (string2_length != 0) {
  	memcpy(filterp, string2, string2_length);
  	filterp += string2_length;
  }
  *filterp++ = ')';
  *filterp = '\0';
  g_free(string);
  g_free(string2);
  return ASN1_ERR_NOERROR;
}

/* Richard Dawe: To parse substring filters, I added this function. */
static int parse_filter_substrings(ASN1_SCK *a, char **filter, guint *filter_length)
{
  int end;
  guchar *string;
  char *filterp;
  guint string_length;
  guint string_bytes;
  guint seq_len;
  guint header_bytes;
  int ret, any_valued;

  /* For ASN.1 parsing of octet strings */
  guint        cls;
  guint        con;
  guint        tag;
  gboolean     def;

  ret = asn1_octet_string_decode(a, &string, &string_length, &string_bytes);
  if (ret != ASN1_ERR_NOERROR)
    return ret;

  ret = asn1_sequence_decode(a, &seq_len, &header_bytes);
  if (ret != ASN1_ERR_NOERROR)
    return ret;

  *filter_length += 2 + 1 + string_length;
  *filter = g_realloc(*filter, *filter_length);

  filterp = *filter + strlen(*filter);
  *filterp++ = '(';
  if (string_length != 0) {
    memcpy(filterp, string, string_length);
    filterp += string_length;
  }
  *filterp++ = '=';
  *filterp = '\0';
  g_free(string);

  /* Now decode seq_len's worth of octet strings. */
  any_valued = 0;
  end = a->offset + seq_len;

  while (a->offset < end) {
    /* Octet strings here are context-specific, which
     * asn1_octet_string_decode() barfs on. Emulate it, but don't barf. */
    ret = asn1_header_decode (a, &cls, &con, &tag, &def, &string_length);
    if (ret != ASN1_ERR_NOERROR)
      return ret;

    /* XXX - check the tag? */
    if (cls != ASN1_CTX || con != ASN1_PRI) {
    	/* XXX - handle the constructed encoding? */
	return ASN1_ERR_WRONG_TYPE;
    }
    if (!def)
    	return ASN1_ERR_LENGTH_NOT_DEFINITE;

    ret = asn1_string_value_decode(a, (int) string_length, &string);
    if (ret != ASN1_ERR_NOERROR)
      return ret;

    /* If we have an 'any' component with a string value, we need to append
     * an extra asterisk before final component. */
    if ((tag == 1) && (string_length != 0))
      any_valued = 1;

    if ( (tag == 1) || ((tag == 2) && any_valued) )
      (*filter_length)++;
    *filter_length += string_length;
    *filter = g_realloc(*filter, *filter_length);

    filterp = *filter + strlen(*filter);
    if ( (tag == 1) || ((tag == 2) && any_valued) )
      *filterp++ = '*';
    if (tag == 2)
      any_valued = 0;
    if (string_length != 0) {
      memcpy(filterp, string, string_length);
      filterp += string_length;
    }
    *filterp = '\0';
    g_free(string);
  }

  if (any_valued)
  {
    (*filter_length)++;
    *filter = g_realloc(*filter, *filter_length);
    filterp = *filter + strlen(*filter);
    *filterp++ = '*';
  }

  /* NB: Allocated byte for this earlier */
  *filterp++ = ')';
  *filterp = '\0';

  return ASN1_ERR_NOERROR;
}

/* Returns -1 if we're at the end, returns an ASN1_ERR value otherwise. */
static int parse_filter(ASN1_SCK *a, char **filter, guint *filter_length,
			int *end)
{
  guint cls, con, tag;
  guint length;
  gboolean def;
  int ret;

  ret = asn1_header_decode(a, &cls, &con, &tag, &def, &length);
  if (ret != ASN1_ERR_NOERROR)
    return ret;

  if (*end == 0)
  {
    *end = a->offset + length;
    *filter_length = 1;
    *filter = g_malloc0(*filter_length);
  }

  if (cls == ASN1_CTX)	/* XXX - handle other types as errors? */
  {
    switch (tag)
    {
     case LDAP_FILTER_AND:
      {
        int add_end;

        if (con != ASN1_CON)
          return ASN1_ERR_WRONG_TYPE;
        add_end = a->offset + length;
        *filter_length += 3;
        *filter = g_realloc(*filter, *filter_length);
        strcat(*filter, "(&");
        while ((ret = parse_filter(a, filter, filter_length, &add_end))
 		== ASN1_ERR_NOERROR)
	  continue;
	if (ret != -1)
	  return ret;
        strcat(*filter, ")");
      }
      break;
     case LDAP_FILTER_OR:
      {
        int or_end;

        if (con != ASN1_CON)
          return ASN1_ERR_WRONG_TYPE;
        or_end = a->offset + length;
        *filter_length += 3;
        *filter = g_realloc(*filter, *filter_length);
        strcat(*filter, "(|");
        while ((ret = parse_filter(a, filter, filter_length, &or_end))
 		== ASN1_ERR_NOERROR)
	  continue;
	if (ret != -1)
	  return ret;
        strcat(*filter, ")");
      }
      break;
     case LDAP_FILTER_NOT:
      {
        int not_end;

        if (con != ASN1_CON)
          return ASN1_ERR_WRONG_TYPE;
        not_end = a->offset + length;
        *filter_length += 3;
        *filter = g_realloc(*filter, *filter_length);
        strcat(*filter, "(!");
        ret = parse_filter(a, filter, filter_length, &not_end);
        if (ret != -1 && ret != ASN1_ERR_NOERROR)
          return ret;
        strcat(*filter, ")");
      }
      break;
     case LDAP_FILTER_EQUALITY:
      if (con != ASN1_CON)
        return ASN1_ERR_WRONG_TYPE;
      ret = parse_filter_strings(a, filter, filter_length, "=");
      if (ret != ASN1_ERR_NOERROR)
        return ret;
      break;
     case LDAP_FILTER_GE:
      if (con != ASN1_CON)
        return ASN1_ERR_WRONG_TYPE;
      ret = parse_filter_strings(a, filter, filter_length, ">=");
      if (ret != ASN1_ERR_NOERROR)
        return ret;
      break;
     case LDAP_FILTER_LE:
      if (con != ASN1_CON)
        return ASN1_ERR_WRONG_TYPE;
      ret = parse_filter_strings(a, filter, filter_length, "<=");
      if (ret != -1 && ret != ASN1_ERR_NOERROR)
        return ret;
      break;
     case LDAP_FILTER_APPROX:
      if (con != ASN1_CON)
        return ASN1_ERR_WRONG_TYPE;
      ret = parse_filter_strings(a, filter, filter_length, "~=");
      if (ret != ASN1_ERR_NOERROR)
        return ret;
      break;
     case LDAP_FILTER_PRESENT:
      {
        guchar *string;
        char *filterp;

        if (con != ASN1_PRI)
          return ASN1_ERR_WRONG_TYPE;
        ret = asn1_string_value_decode(a, length, &string);
        if (ret != ASN1_ERR_NOERROR)
          return ret;
        *filter_length += 4 + length;
        *filter = g_realloc(*filter, *filter_length);
        filterp = *filter + strlen(*filter);
        *filterp++ = '(';
        if (length != 0) {
          memcpy(filterp, string, length);
          filterp += length;
        }
        *filterp++ = '=';
        *filterp++ = '*';
        *filterp++ = ')';
        *filterp = '\0';
        g_free(string);
      }
      break;
     case LDAP_FILTER_SUBSTRINGS:
      if (con != ASN1_CON)
        return ASN1_ERR_WRONG_TYPE;
      /* Richard Dawe: Handle substrings */
      ret = parse_filter_substrings(a, filter, filter_length);
      if (ret != ASN1_ERR_NOERROR)
        return ret;
      break;
     default:
      return ASN1_ERR_WRONG_TYPE;
    }
  }

  if (a->offset == *end)
    return -1;
  else
    return ASN1_ERR_NOERROR;
}

static gboolean read_filter(ASN1_SCK *a, proto_tree *tree, int hf_id)
{
  int start = a->offset;
  char *filter = 0;
  guint filter_length = 0;
  int end = 0;
  int ret;

  while ((ret = parse_filter(a, &filter, &filter_length, &end))
	== ASN1_ERR_NOERROR)
    continue;

  if (tree) {
    if (ret != -1) {
      proto_tree_add_text(tree, a->tvb, start, 0,
        "%s: ERROR: Can't parse filter: %s",
        proto_registrar_get_name(hf_id), asn1_err_to_str(ret));
    } else
      proto_tree_add_string(tree, hf_id, a->tvb, start, a->offset-start, filter);
  }

  g_free(filter);

  return (ret == -1) ? TRUE : FALSE;
}

/********************************************************************************************/

static void dissect_ldap_result(ASN1_SCK *a, proto_tree *tree, packet_info *pinfo)
{
  guint resultCode = 0;
  int ret;
  if (read_integer(a, tree, hf_ldap_message_result, 0, &resultCode, ASN1_ENUM) != ASN1_ERR_NOERROR)
    return;

  if (resultCode != 0) {
	  if (check_col(pinfo->cinfo, COL_INFO))
		  col_append_fstr(pinfo->cinfo, COL_INFO, ", %s", 
				  val_to_str(resultCode, result_codes,
					     "Unknown (%u)"));
  }

  if (read_string(a, tree, hf_ldap_message_result_matcheddn, 0, 0, 0, ASN1_UNI, ASN1_OTS) != ASN1_ERR_NOERROR)
    return;
  if (read_string(a, tree, hf_ldap_message_result_errormsg, 0, 0, 0, ASN1_UNI, ASN1_OTS) != ASN1_ERR_NOERROR)
    return;

  if (resultCode == 10)		/* Referral */
  {
    int start = a->offset;
    int end;
    guint length;
    proto_item *ti;
    proto_tree *referralTree;

    ret = read_sequence(a, &length);
    if (ret != ASN1_ERR_NOERROR) {
      if (tree) {
        proto_tree_add_text(tree, a->tvb, start, 0,
            "ERROR: Couldn't parse referral URL sequence header: %s",
            asn1_err_to_str(ret));
      }
      return;
    }
    ti = proto_tree_add_text(tree, a->tvb, start, length, "Referral URLs");
    referralTree = proto_item_add_subtree(ti, ett_ldap_referrals);

    end = a->offset + length;
    while (a->offset < end) {
      if (read_string(a, referralTree, hf_ldap_message_result_referral, 0, 0, 0, ASN1_UNI, ASN1_OTS) != ASN1_ERR_NOERROR)
        return;
    }
  }
}

static void dissect_ldap_request_bind(ASN1_SCK *a, proto_tree *tree,
    tvbuff_t *tvb, packet_info *pinfo, ldap_conv_info_t *ldap_info)
{
  guint cls, con, tag;
  gboolean def;
  guint length;
  int start;
  int end;
  int ret;
  char *mechanism, *s = NULL;
  int token_offset;
  gint available_length, reported_length;
  tvbuff_t *new_tvb;
  proto_item *gitem;
  proto_tree *gtree = NULL;

  if (read_integer(a, tree, hf_ldap_message_bind_version, 0, 0, ASN1_INT) != ASN1_ERR_NOERROR)
    return;
  if (read_string(a, tree, hf_ldap_message_bind_dn, 0, &s, 0, ASN1_UNI, ASN1_OTS) != ASN1_ERR_NOERROR)
    return;

  if (check_col(pinfo->cinfo, COL_INFO))
    col_append_fstr(pinfo->cinfo, COL_INFO, ", DN=%s", s != NULL ? s : "(null)");
  g_free(s);

  start = a->offset;
  ret = asn1_header_decode(a, &cls, &con, &tag, &def, &length);
  if (ret == ASN1_ERR_NOERROR) {
    if (cls != ASN1_CTX) {
      /* RFCs 1777 and 2251 say these are context-specific types */
      ret = ASN1_ERR_WRONG_TYPE;
    }
  }
  if (ret != ASN1_ERR_NOERROR) {
    proto_tree_add_text(tree, a->tvb, start, 0,
      "%s: ERROR: Couldn't parse header: %s",
      proto_registrar_get_name(hf_ldap_message_bind_auth),
      asn1_err_to_str(ret));
    return;
  }
  proto_tree_add_uint(tree, hf_ldap_message_bind_auth, a->tvb, start,
			a->offset - start, tag);
  end = a->offset + length;
  switch (tag)
  {
   case LDAP_AUTH_SIMPLE:
    if (read_string_value(a, tree, hf_ldap_message_bind_auth_password, NULL,
                          NULL, start, length) != ASN1_ERR_NOERROR)
      return;
    break;

    /* For Kerberos V4, dissect it as a ticket. */

   case LDAP_AUTH_SASL:
    mechanism = NULL;
    if (read_string(a, tree, hf_ldap_message_bind_auth_mechanism, NULL,
                    &mechanism, 0, ASN1_UNI, ASN1_OTS) != ASN1_ERR_NOERROR)
      return;

    /*
     * We need to remember the authentication type and mechanism for this
     * conversation.
     *
     * XXX - actually, we might need to remember more than one
     * type and mechanism, if you can unbind and rebind with a
     * different type and/or mechanism.
     */
    ldap_info->auth_type = tag;
    ldap_info->auth_mech = mechanism;
    ldap_info->first_auth_frame = 0;	/* not known until we see the bind reply */
    /*
     * If the mechanism in this request is an empty string (which is
     * returned as a null pointer), use the saved mechanism instead.
     * Otherwise, if the saved mechanism is an empty string (null),
     * save this mechanism.
     */
    if (mechanism == NULL)
        mechanism = ldap_info->auth_mech;
    else {
      if (ldap_info->auth_mech == NULL) {
        g_free(ldap_info->auth_mech);
      }
      ldap_info->auth_mech = mechanism;
    }

    if (a->offset < end) {
      if (mechanism != NULL && strcmp(mechanism, "GSS-SPNEGO") == 0) {
        /*
         * This is a GSS-API token ancapsulated within GSS-SPNEGO.
         * Find out how big it is by parsing the ASN.1 header for the
         * OCTET STREAM that contains it.
         */
        token_offset = a->offset;
        ret = asn1_header_decode(a, &cls, &con, &tag, &def, &length);
        if (ret != ASN1_ERR_NOERROR) {
          proto_tree_add_text(tree, a->tvb, token_offset, 0,
            "%s: ERROR: Couldn't parse header: %s",
            proto_registrar_get_name(hf_ldap_message_bind_auth_credentials),
            asn1_err_to_str(ret));
          return;
        }
        if (tree) {
          gitem = proto_tree_add_text(tree, tvb, token_offset,
            (a->offset + length) - token_offset, "GSS-API Token");
          gtree = proto_item_add_subtree(gitem, ett_ldap_gssapi_token);
        }
        available_length = tvb_length_remaining(tvb, token_offset);
        reported_length = tvb_reported_length_remaining(tvb, token_offset);
        g_assert(available_length >= 0);
        g_assert(reported_length >= 0);
        if (available_length > reported_length)
          available_length = reported_length;
        if ((guint)available_length > length)
          available_length = length;
        if ((guint)reported_length > length)
          reported_length = length;
        new_tvb = tvb_new_subset(tvb, a->offset, available_length, reported_length);
        call_dissector(gssapi_handle, new_tvb, pinfo, gtree);
        a->offset += length;
      } else if (mechanism != NULL && strcmp(mechanism, "GSSAPI") == 0) {
        /*
         * This is a raw GSS-API token.
         * Find out how big it is by parsing the ASN.1 header for the
         * OCTET STREAM that contains it.
         */
        token_offset = a->offset;
        ret = asn1_header_decode(a, &cls, &con, &tag, &def, &length);
        if (ret != ASN1_ERR_NOERROR) {
          proto_tree_add_text(tree, a->tvb, token_offset, 0,
            "%s: ERROR: Couldn't parse header: %s",
            proto_registrar_get_name(hf_ldap_message_bind_auth_credentials),
            asn1_err_to_str(ret));
          return;
        }
        if (tree) {
          gitem = proto_tree_add_text(tree, tvb, token_offset,
            (a->offset + length) - token_offset, "GSS-API Token");
          gtree = proto_item_add_subtree(gitem, ett_ldap_gssapi_token);
        }
        if(length==0){
          /* for GSSAPI the third pdu will sometimes be "empty" */
          return;
        }
        available_length = tvb_length_remaining(tvb, token_offset);
        reported_length = tvb_reported_length_remaining(tvb, token_offset);
        g_assert(available_length >= 0);
        g_assert(reported_length >= 0);
        if (available_length > reported_length)
          available_length = reported_length;
        if ((guint)available_length > length)
          available_length = length;
        if ((guint)reported_length > length)
          reported_length = length;
        new_tvb = tvb_new_subset(tvb, a->offset, available_length, reported_length);
        call_dissector(gssapi_handle, new_tvb, pinfo, gtree);
        a->offset += length;
      } else {
        if (read_bytestring(a, tree, hf_ldap_message_bind_auth_credentials,
                            NULL, NULL, ASN1_UNI, ASN1_OTS) != ASN1_ERR_NOERROR)
          return;
      }
    }
    break;
  }
}

static void dissect_ldap_response_bind(ASN1_SCK *a, proto_tree *tree,
		int start, guint length, tvbuff_t *tvb, packet_info *pinfo, ldap_conv_info_t *ldap_info)
{
  guint cls, con, tag;
  gboolean def;
  guint cred_length;
  int end;
  int ret;
  int token_offset;
  gint available_length, reported_length;
  tvbuff_t *new_tvb;
  proto_item *gitem;
  proto_tree *gtree = NULL;

  end = start + length;
  dissect_ldap_result(a, tree, pinfo);
  if (a->offset < end) {
    switch (ldap_info->auth_type) {

      /* For Kerberos V4, dissect it as a ticket. */
      /* XXX - what about LDAP_AUTH_SIMPLE? */

    case LDAP_AUTH_SASL:
      /*
       * All frames after this are assumed to use a security layer.
       *
       * XXX - won't work if there's another reply, with the security
       * layer, starting in the same TCP segment that ends this
       * reply, but as LDAP is a request/response protocol, and
       * as the client probably can't start using authentication until
       * it gets the bind reply and the server won't send a reply until
       * it gets a request, that probably won't happen.
       *
       * XXX - that assumption is invalid; it's not clear where the
       * hell you find out whether there's any security layer.  In
       * one capture, we have two GSS-SPNEGO negotiations, both of
       * which select MS KRB5, and the only differences in the tokens
       * is in the RC4-HMAC ciphertext.  The various
       * draft-ietf--cat-sasl-gssapi-NN.txt drafts seem to imply
       * that the RFC 2222 spoo with the bitmask and maximum
       * output message size stuff is done - but where does that
       * stuff show up?  Is it in the ciphertext, which means it's
       * presumably encrypted?
       *
       * Grrr.  We have to do a gross heuristic, checking whether the
       * putative LDAP message begins with 0x00 or not, making the
       * assumption that we won't have more than 2^24 bytes of
       * encapsulated stuff.
       */
      ldap_info->first_auth_frame = pinfo->fd->num + 1;
      if (ldap_info->auth_mech != NULL &&
          strcmp(ldap_info->auth_mech, "GSS-SPNEGO") == 0) {
        /*
         * This is a GSS-API token.
         * Find out how big it is by parsing the ASN.1 header for the
         * OCTET STREAM that contains it.
         */
        token_offset = a->offset;
        ret = asn1_header_decode(a, &cls, &con, &tag, &def, &cred_length);
        if (ret != ASN1_ERR_NOERROR) {
          proto_tree_add_text(tree, a->tvb, token_offset, 0,
            "%s: ERROR: Couldn't parse header: %s",
            proto_registrar_get_name(hf_ldap_message_bind_auth_credentials),
            asn1_err_to_str(ret));
          return;
        }
        if (tree) {
          gitem = proto_tree_add_text(tree, tvb, token_offset,
            (a->offset + cred_length) - token_offset, "GSS-API Token");
          gtree = proto_item_add_subtree(gitem, ett_ldap_gssapi_token);
        }
        available_length = tvb_length_remaining(tvb, token_offset);
        reported_length = tvb_reported_length_remaining(tvb, token_offset);
        g_assert(available_length >= 0);
        g_assert(reported_length >= 0);
        if (available_length > reported_length)
          available_length = reported_length;
        if ((guint)available_length > cred_length)
          available_length = cred_length;
        if ((guint)reported_length > cred_length)
          reported_length = cred_length;
        new_tvb = tvb_new_subset(tvb, a->offset, available_length, reported_length);
        call_dissector(gssapi_handle, new_tvb, pinfo, gtree);
        a->offset += cred_length;
      } else if (ldap_info->auth_mech != NULL &&
          strcmp(ldap_info->auth_mech, "GSSAPI") == 0) {
        /*
         * This is a GSS-API token.
         * Find out how big it is by parsing the ASN.1 header for the
         * OCTET STREAM that contains it.
         */
        token_offset = a->offset;
        ret = asn1_header_decode(a, &cls, &con, &tag, &def, &cred_length);
        if (ret != ASN1_ERR_NOERROR) {
          proto_tree_add_text(tree, a->tvb, token_offset, 0,
            "%s: ERROR: Couldn't parse header: %s",
            proto_registrar_get_name(hf_ldap_message_bind_auth_credentials),
            asn1_err_to_str(ret));
          return;
        }
        if (tree) {
          gitem = proto_tree_add_text(tree, tvb, token_offset,
            (a->offset + cred_length) - token_offset, "GSS-API Token");
          gtree = proto_item_add_subtree(gitem, ett_ldap_gssapi_token);
        }
        available_length = tvb_length_remaining(tvb, token_offset);
        reported_length = tvb_reported_length_remaining(tvb, token_offset);
        g_assert(available_length >= 0);
        g_assert(reported_length >= 0);
        if (available_length > reported_length)
          available_length = reported_length;
        if ((guint)available_length > cred_length)
          available_length = cred_length;
        if ((guint)reported_length > cred_length)
          reported_length = cred_length;
        new_tvb = tvb_new_subset(tvb, a->offset, available_length, reported_length);
        call_dissector(gssapi_handle, new_tvb, pinfo, gtree);
        a->offset += cred_length;
      } else {
        if (read_bytestring(a, tree, hf_ldap_message_bind_server_credentials,
                            NULL, NULL, ASN1_UNI, ASN1_OTS) != ASN1_ERR_NOERROR)
          return;
      }
      break;

    default:
      if (read_bytestring(a, tree, hf_ldap_message_bind_server_credentials,
                          NULL, NULL, ASN1_CTX, 7) != ASN1_ERR_NOERROR)
        return;
      break;
    }
  }
}

static void dissect_ldap_request_search(ASN1_SCK *a, proto_tree *tree, packet_info *pinfo)
{
  guint seq_length;
  int end;
  int ret;
  char *s = NULL;

  if (read_string(a, tree, hf_ldap_message_search_base, 0, &s, 0, ASN1_UNI, ASN1_OTS) != ASN1_ERR_NOERROR)
    return;

  if (check_col(pinfo->cinfo, COL_INFO))
    col_append_fstr(pinfo->cinfo, COL_INFO, ", Base DN=%s", s != NULL ? s : "(null)");
  g_free(s);

  if (read_integer(a, tree, hf_ldap_message_search_scope, 0, 0, ASN1_ENUM) != ASN1_ERR_NOERROR)
    return;
  if (read_integer(a, tree, hf_ldap_message_search_deref, 0, 0, ASN1_ENUM) != ASN1_ERR_NOERROR)
    return;
  if (read_integer(a, tree, hf_ldap_message_search_sizeLimit, 0, 0, ASN1_INT) != ASN1_ERR_NOERROR)
    return;
  if (read_integer(a, tree, hf_ldap_message_search_timeLimit, 0, 0, ASN1_INT) != ASN1_ERR_NOERROR)
    return;
  if (read_boolean(a, tree, hf_ldap_message_search_typesOnly, 0, 0) != ASN1_ERR_NOERROR)
    return;
  if (!read_filter(a, tree, hf_ldap_message_search_filter))
    return;
  ret = read_sequence(a, &seq_length);
  if (ret != ASN1_ERR_NOERROR) {
    if (tree) {
      proto_tree_add_text(tree, a->tvb, a->offset, 0,
          "ERROR: Couldn't parse LDAP attribute sequence header: %s",
          asn1_err_to_str(ret));
    }
    return;
  }
  end = a->offset + seq_length;
  while (a->offset < end) {
    if (read_string(a, tree, hf_ldap_message_attribute, 0, 0, 0, ASN1_UNI,
                    ASN1_OTS) != ASN1_ERR_NOERROR)
      return;
  }
}

static int dissect_mscldap_string(tvbuff_t *tvb, int offset, char *str, int maxlen, gboolean prepend_dot)
{
  guint8 len;

  len=tvb_get_guint8(tvb, offset);
  offset+=1;
  *str=0;

  while(len){
    /* add potential field separation dot */
    if(prepend_dot){
      if(!maxlen){
        *str=0;
        return offset;
      }
      maxlen--;
      *str++='.';
      *str=0;
    }

    if(len==0xc0){
      int new_offset;
      /* ops its a mscldap compressed string */

      new_offset=tvb_get_guint8(tvb, offset);
      offset+=1;

      dissect_mscldap_string(tvb, new_offset, str, maxlen, FALSE);

      return offset;
    }

    prepend_dot=TRUE;

    if(maxlen<=len){
      if(maxlen>3){
        *str++='.';
        *str++='.';
        *str++='.';
      }
      *str=0;
      return offset; /* will mess up offset in caller, is unlikely */
    }
    tvb_memcpy(tvb, str, offset, len);
    str+=len;
    *str=0;
    maxlen-=len;
    offset+=len;


    len=tvb_get_guint8(tvb, offset);
    offset+=1;
  }
  *str=0;
  return offset;
}


/* These flag bits were found to be defined in the samba sources.
 * I hope they are correct (but have serious doubts about the CLOSEST
 * bit being used or being meaningful).
 */
static const true_false_string tfs_ads_pdc = {
	"This is a PDC",
	"This is NOT a pdc"
};
static const true_false_string tfs_ads_gc = {
	"This is a GLOBAL CATALOGUE of forest",
	"This is NOT a global catalog of forest"
};
static const true_false_string tfs_ads_ldap = {
	"This is an LDAP server",
	"This is NOT an ldap server"
};
static const true_false_string tfs_ads_ds = {
	"This dc supports DS",
	"This dc does NOT support ds"
};
static const true_false_string tfs_ads_kdc = {
	"This is a KDC (kerberos)",
	"This is NOT a kdc (kerberos)"
};
static const true_false_string tfs_ads_timeserv = {
	"This dc is running TIME SERVICES (ntp)",
	"This dc is NOT running time services (ntp)"
};
static const true_false_string tfs_ads_closest = {
	"This is the CLOSEST dc (unreliable?)",
	"This is NOT the closest dc"
};
static const true_false_string tfs_ads_writable = {
	"This dc is WRITABLE",
	"This dc is NOT writable"
};
static const true_false_string tfs_ads_good_timeserv = {
	"This dc has a GOOD TIME SERVICE (i.e. hardware clock)",
	"This dc does NOT have a good time service (i.e. no hardware clock)"
};
static const true_false_string tfs_ads_ndnc = {
	"Domain is NON-DOMAIN NC serviced by ldap server",
	"Domain is NOT non-domain nc serviced by ldap server"
};
static int dissect_mscldap_netlogon_flags(proto_tree *parent_tree, tvbuff_t *tvb, int offset)
{
  guint32 flags;
  proto_item *item;
  proto_tree *tree=NULL;

  flags=tvb_get_letohl(tvb, offset);
  item=proto_tree_add_item(parent_tree, hf_mscldap_netlogon_flags, tvb, offset, 4, TRUE);
  if(parent_tree){
    tree = proto_item_add_subtree(item, ett_mscldap_netlogon_flags);
  }

  proto_tree_add_boolean(tree, hf_mscldap_netlogon_flags_ndnc,
    tvb, offset, 4, flags);
  proto_tree_add_boolean(tree, hf_mscldap_netlogon_flags_good_timeserv,
    tvb, offset, 4, flags);
  proto_tree_add_boolean(tree, hf_mscldap_netlogon_flags_writable,
    tvb, offset, 4, flags);
  proto_tree_add_boolean(tree, hf_mscldap_netlogon_flags_closest,
    tvb, offset, 4, flags);
  proto_tree_add_boolean(tree, hf_mscldap_netlogon_flags_timeserv,
    tvb, offset, 4, flags);
  proto_tree_add_boolean(tree, hf_mscldap_netlogon_flags_kdc,
    tvb, offset, 4, flags);
  proto_tree_add_boolean(tree, hf_mscldap_netlogon_flags_ds,
    tvb, offset, 4, flags);
  proto_tree_add_boolean(tree, hf_mscldap_netlogon_flags_ldap,
    tvb, offset, 4, flags);
  proto_tree_add_boolean(tree, hf_mscldap_netlogon_flags_gc,
    tvb, offset, 4, flags);
  proto_tree_add_boolean(tree, hf_mscldap_netlogon_flags_pdc,
    tvb, offset, 4, flags);

  offset += 4;

  return offset;
}

static void dissect_mscldap_response_netlogon(proto_tree *tree, tvbuff_t *tvb)
{
  int old_offset, offset=0;
  char str[256];

/*qqq*/

  /* Type */
  /*XXX someone that knows what the type means should add that knowledge here*/
  proto_tree_add_item(tree, hf_mscldap_netlogon_type, tvb, offset, 4, TRUE);
  offset += 4;

  /* Flags */
  offset = dissect_mscldap_netlogon_flags(tree, tvb, offset);

  /* Domain GUID */
  proto_tree_add_item(tree, hf_mscldap_domain_guid, tvb, offset, 16, TRUE);
  offset += 16;

  /* Forest */
  old_offset=offset;
  offset=dissect_mscldap_string(tvb, offset, str, 255, FALSE);
  proto_tree_add_string(tree, hf_mscldap_forest, tvb, old_offset, offset-old_offset, str);
  
  /* Domain */
  old_offset=offset;
  offset=dissect_mscldap_string(tvb, offset, str, 255, FALSE);
  proto_tree_add_string(tree, hf_mscldap_domain, tvb, old_offset, offset-old_offset, str);
  
  /* Hostname */
  old_offset=offset;
  offset=dissect_mscldap_string(tvb, offset, str, 255, FALSE);
  proto_tree_add_string(tree, hf_mscldap_hostname, tvb, old_offset, offset-old_offset, str);
  
  /* NetBios Domain */
  old_offset=offset;
  offset=dissect_mscldap_string(tvb, offset, str, 255, FALSE);
  proto_tree_add_string(tree, hf_mscldap_nb_domain, tvb, old_offset, offset-old_offset, str);
  
  /* NetBios Hostname */
  old_offset=offset;
  offset=dissect_mscldap_string(tvb, offset, str, 255, FALSE);
  proto_tree_add_string(tree, hf_mscldap_nb_hostname, tvb, old_offset, offset-old_offset, str);
  
  /* User */
  old_offset=offset;
  offset=dissect_mscldap_string(tvb, offset, str, 255, FALSE);
  proto_tree_add_string(tree, hf_mscldap_username, tvb, old_offset, offset-old_offset, str);
  
  /* Site */
  old_offset=offset;
  offset=dissect_mscldap_string(tvb, offset, str, 255, FALSE);
  proto_tree_add_string(tree, hf_mscldap_sitename, tvb, old_offset, offset-old_offset, str);
  
  /* Client Site */
  old_offset=offset;
  offset=dissect_mscldap_string(tvb, offset, str, 255, FALSE);
  proto_tree_add_string(tree, hf_mscldap_clientsitename, tvb, old_offset, offset-old_offset, str);
  
  /* Version */
  proto_tree_add_item(tree, hf_mscldap_netlogon_version, tvb, offset, 4, TRUE);
  offset += 4;

  /* LM Token */
  proto_tree_add_item(tree, hf_mscldap_netlogon_lm_token, tvb, offset, 2, TRUE);
  offset += 2;

  /* NT Token */
  proto_tree_add_item(tree, hf_mscldap_netlogon_nt_token, tvb, offset, 2, TRUE);
  offset += 2;

}

static void dissect_mscldap_response(proto_tree *tree, tvbuff_t *tvb, guint32 rpc)
{
  switch(rpc){
  case MSCLDAP_RPC_NETLOGON:
    dissect_mscldap_response_netlogon(tree, tvb);
    break;
  default:
    proto_tree_add_text(tree, tvb, 0, tvb_length(tvb),
      "ERROR: Unknown type of MS-CLDAP RPC call");
  }
}


static void dissect_ldap_response_search_entry(ASN1_SCK *a, proto_tree *tree,
		gboolean is_mscldap)
{
  guint seq_length;
  int end_of_sequence;
  int ret;
  char *str=NULL;
  guint32 len;
  guint32 mscldap_rpc;

  if (read_string(a, tree, hf_ldap_message_dn, 0, 0, 0, ASN1_UNI, ASN1_OTS) != ASN1_ERR_NOERROR)
    return;
  ret = read_sequence(a, &seq_length);
  if (ret != ASN1_ERR_NOERROR) {
    if (tree) {
      proto_tree_add_text(tree, a->tvb, a->offset, 0,
          "ERROR: Couldn't parse search entry response sequence header: %s",
          asn1_err_to_str(ret));
    }
    return;
  }

  end_of_sequence = a->offset + seq_length;
  while (a->offset < end_of_sequence)
  {
    proto_item *ti;
    proto_tree *attr_tree;
    guint set_length;
    int end_of_set;

    ret = read_sequence(a, 0);
    if (ret != ASN1_ERR_NOERROR) {
      if (tree) {
        proto_tree_add_text(tree, a->tvb, a->offset, 0,
            "ERROR: Couldn't parse LDAP attribute sequence header: %s",
            asn1_err_to_str(ret));
      }
      return;
    }
    if (read_string(a, tree, hf_ldap_message_attribute, &ti, &str, &len, ASN1_UNI, ASN1_OTS) != ASN1_ERR_NOERROR)
      return;

    mscldap_rpc=0;
    if(is_mscldap){
	if(!strncmp(str, "netlogon", 8)){
		mscldap_rpc=MSCLDAP_RPC_NETLOGON;
	}
    }
    g_free(str);
    str=NULL;


    attr_tree = proto_item_add_subtree(ti, ett_ldap_attribute);

    ret = read_set(a, &set_length);
    if (ret != ASN1_ERR_NOERROR) {
      if (tree) {
        proto_tree_add_text(attr_tree, a->tvb, a->offset, 0,
            "ERROR: Couldn't parse LDAP value set header: %s",
            asn1_err_to_str(ret));
      }
      return;
    }
    end_of_set = a->offset + set_length;
    while (a->offset < end_of_set) {
      if(!is_mscldap){
        if (read_string(a, attr_tree, hf_ldap_message_value, 0, 0, 0, ASN1_UNI,
                        ASN1_OTS) != ASN1_ERR_NOERROR){
          return;
        }
      } else {
        guint cls, con, tag;
        gboolean def;
        guint len;
        int start = a->offset;
        int ret;
        tvbuff_t *mscldap_tvb=NULL;

        ret = asn1_header_decode(a, &cls, &con, &tag, &def, &len);
        if (ret == ASN1_ERR_NOERROR) {
          if (cls != ASN1_UNI || con != ASN1_PRI || tag != ASN1_OTS)
            ret = ASN1_ERR_WRONG_TYPE;
        }
        if (ret != ASN1_ERR_NOERROR) {
          if (tree) {
            proto_tree_add_text(tree, a->tvb, start, 0,
              "%s: ERROR: Couldn't parse header: %s",
            proto_registrar_get_name(hf_ldap_message_value), asn1_err_to_str(ret));
          }
          return;
        }
        mscldap_tvb=tvb_new_subset(a->tvb, a->offset, len, len);
        dissect_mscldap_response(attr_tree, mscldap_tvb, mscldap_rpc);
        a->offset+=len;
      }

    }
  }
}

static void dissect_ldap_response_search_ref(ASN1_SCK *a, proto_tree *tree)
{
  read_string(a, tree, hf_ldap_message_search_reference, 0, 0, 0, ASN1_UNI, ASN1_OTS);
}

static void dissect_ldap_request_add(ASN1_SCK *a, proto_tree *tree, packet_info *pinfo)
{
  guint seq_length;
  int end_of_sequence;
  int ret;
  char *s = NULL;

  if (read_string(a, tree, hf_ldap_message_dn, 0, &s, 0, ASN1_UNI, ASN1_OTS) != ASN1_ERR_NOERROR)
    return;

  if (check_col(pinfo->cinfo, COL_INFO))
    col_append_fstr(pinfo->cinfo, COL_INFO, ", DN=%s", s != NULL ? s : "(null)");
  g_free(s);  

  ret = read_sequence(a, &seq_length);
  if (ret != ASN1_ERR_NOERROR) {
    if (tree) {
      proto_tree_add_text(tree, a->tvb, a->offset, 0,
          "ERROR: Couldn't parse add request sequence header: %s",
          asn1_err_to_str(ret));
    }
    return;
  }

  end_of_sequence = a->offset + seq_length;
  while (a->offset < end_of_sequence)
  {
    proto_item *ti;
    proto_tree *attr_tree;
    guint set_length;
    int end_of_set;

    ret = read_sequence(a, 0);
    if (ret != ASN1_ERR_NOERROR) {
      if (tree) {
        proto_tree_add_text(tree, a->tvb, a->offset, 0,
            "ERROR: Couldn't parse LDAP attribute sequence header: %s",
            asn1_err_to_str(ret));
      }
      return;
    }
    if (read_string(a, tree, hf_ldap_message_attribute, &ti, 0, 0, ASN1_UNI,
                    ASN1_OTS) != ASN1_ERR_NOERROR)
      return;
    attr_tree = proto_item_add_subtree(ti, ett_ldap_attribute);

    ret = read_set(a, &set_length);
    if (ret != ASN1_ERR_NOERROR) {
      if (tree) {
        proto_tree_add_text(attr_tree, a->tvb, a->offset, 0,
            "ERROR: Couldn't parse LDAP value set header: %s",
            asn1_err_to_str(ret));
      }
      return;
    }
    end_of_set = a->offset + set_length;
    while (a->offset < end_of_set) {
      if (read_string(a, attr_tree, hf_ldap_message_value, 0, 0, 0, ASN1_UNI, ASN1_OTS) != ASN1_ERR_NOERROR)
        return;
    }
  }
}

static void dissect_ldap_request_delete(ASN1_SCK *a, proto_tree *tree,
		int start, guint length)
{
  read_string_value(a, tree, hf_ldap_message_dn, NULL, NULL, start, length);
}

static void dissect_ldap_request_modifyrdn(ASN1_SCK *a, proto_tree *tree,
		guint length)
{
  int start = a->offset;

  if (read_string(a, tree, hf_ldap_message_dn, 0, 0, 0, ASN1_UNI, ASN1_OTS) != ASN1_ERR_NOERROR)
    return;
  if (read_string(a, tree, hf_ldap_message_modrdn_name, 0, 0, 0, ASN1_UNI, ASN1_OTS) != ASN1_ERR_NOERROR)
    return;
  if (read_boolean(a, tree, hf_ldap_message_modrdn_delete, 0, 0) != ASN1_ERR_NOERROR)
    return;

  if (a->offset < (int) (start + length)) {
    /* LDAP V3 Modify DN operation, with newSuperior */
    /*      "newSuperior     [0] LDAPDN OPTIONAL" (0x80) */
    if (read_string(a, tree, hf_ldap_message_modrdn_superior, 0, 0, 0, ASN1_CTX, 0) != ASN1_ERR_NOERROR)
      return;
  }
}

static void dissect_ldap_request_compare(ASN1_SCK *a, proto_tree *tree)
{
  int start;
  int length;
  char *string1 = NULL;
  char *string2 = NULL;
  char *s1, *s2;
  char *compare;
  int ret;

  if (read_string(a, tree, hf_ldap_message_dn, 0, 0, 0, ASN1_UNI, ASN1_OTS) != ASN1_ERR_NOERROR)
    return;
  ret = read_sequence(a, 0);
  if (ret != ASN1_ERR_NOERROR) {
    if (tree) {
      proto_tree_add_text(tree, a->tvb, a->offset, 0,
          "ERROR: Couldn't parse compare request sequence header: %s",
          asn1_err_to_str(ret));
    }
    return;
  }

  start = a->offset;
  ret = read_string(a, 0, -1, 0, &string1, 0, ASN1_UNI, ASN1_OTS);
  if (ret != ASN1_ERR_NOERROR) {
    if (tree) {
      proto_tree_add_text(tree, a->tvb, start, 0,
        "ERROR: Couldn't parse compare type: %s", asn1_err_to_str(ret));
    }
    return;
  }
  ret = read_string(a, 0, -1, 0, &string2, 0, ASN1_UNI, ASN1_OTS);
  if (ret != ASN1_ERR_NOERROR) {
    if (tree) {
      proto_tree_add_text(tree, a->tvb, start, 0,
        "ERROR: Couldn't parse compare value: %s", asn1_err_to_str(ret));
    }
    return;
  }

  s1 = (string1 == NULL) ? "(null)" : string1;
  s2 = (string2 == NULL) ? "(null)" : string2;
  length = 2 + strlen(s1) + strlen(s2);
  compare = g_malloc0(length);
  snprintf(compare, length, "%s=%s", s1, s2);
  proto_tree_add_string(tree, hf_ldap_message_compare, a->tvb, start,
      a->offset-start, compare);

  g_free(string1);
  g_free(string2);
  g_free(compare);

  return;
}

static void dissect_ldap_request_modify(ASN1_SCK *a, proto_tree *tree)
{
  guint seq_length;
  int end_of_sequence;
  int ret;

  if (read_string(a, tree, hf_ldap_message_dn, 0, 0, 0, ASN1_UNI, ASN1_OTS) != ASN1_ERR_NOERROR)
    return;
  ret = read_sequence(a, &seq_length);
  if (ret != ASN1_ERR_NOERROR) {
    if (tree) {
      proto_tree_add_text(tree, a->tvb, a->offset, 0,
          "ERROR: Couldn't parse modify request sequence header: %s",
          asn1_err_to_str(ret));
    }
    return;
  }
  end_of_sequence = a->offset + seq_length;
  while (a->offset < end_of_sequence)
  {
    proto_item *ti;
    proto_tree *attr_tree;
    guint set_length;
    int end_of_set;
    guint operation;

    ret = read_sequence(a, 0);
    if (ret != ASN1_ERR_NOERROR) {
      if (tree) {
        proto_tree_add_text(tree, a->tvb, a->offset, 0,
            "ERROR: Couldn't parse modify request item sequence header: %s",
            asn1_err_to_str(ret));
      }
      return;
    }
    ret = read_integer(a, 0, -1, 0, &operation, ASN1_ENUM);
    if (ret != ASN1_ERR_NOERROR) {
      if (tree) {
        proto_tree_add_text(tree, a->tvb, a->offset, 0,
          "ERROR: Couldn't parse modify operation: %s",
          asn1_err_to_str(ret));
        return;
      }
    }
    ret = read_sequence(a, 0);
    if (ret != ASN1_ERR_NOERROR) {
      if (tree) {
        proto_tree_add_text(tree, a->tvb, a->offset, 0,
            "ERROR: Couldn't parse modify request operation sequence header: %s",
            asn1_err_to_str(ret));
      }
      return;
    }

    switch (operation)
    {
     case LDAP_MOD_ADD:
      if (read_string(a, tree, hf_ldap_message_modify_add, &ti, 0, 0, ASN1_UNI,
                      ASN1_OTS) != ASN1_ERR_NOERROR)
        return;
      break;

     case LDAP_MOD_REPLACE:
      if (read_string(a, tree, hf_ldap_message_modify_replace, &ti, 0, 0,
                      ASN1_UNI, ASN1_OTS) != ASN1_ERR_NOERROR)
        return;
      break;

     case LDAP_MOD_DELETE:
      if (read_string(a, tree, hf_ldap_message_modify_delete, &ti, 0, 0,
                      ASN1_UNI, ASN1_OTS) != ASN1_ERR_NOERROR)
        return;
      break;

     default:
       proto_tree_add_text(tree, a->tvb, a->offset, 0,
            "Unknown LDAP modify operation (%u)", operation);
       return;
    }
    attr_tree = proto_item_add_subtree(ti, ett_ldap_attribute);

    ret = read_set(a, &set_length);
    if (ret != ASN1_ERR_NOERROR) {
      if (tree) {
        proto_tree_add_text(attr_tree, a->tvb, a->offset, 0,
            "ERROR: Couldn't parse LDAP value set header: %s",
            asn1_err_to_str(ret));
      }
      return;
    }
    end_of_set = a->offset + set_length;
    while (a->offset < end_of_set) {
      if (read_string(a, attr_tree, hf_ldap_message_value, 0, 0, 0, ASN1_UNI,
                      ASN1_OTS) != ASN1_ERR_NOERROR)
        return;
    }
  }
}

static void dissect_ldap_request_abandon(ASN1_SCK *a, proto_tree *tree,
		int start, guint length)
{
  read_integer_value(a, tree, hf_ldap_message_abandon_msgid, NULL, NULL,
			    start, length);
}

static ldap_call_response_t *
ldap_match_call_response(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, ldap_conv_info_t *ldap_info, guint messageId, guint protocolOpTag)
{
  ldap_call_response_t lcr, *lcrp=NULL;

  if (!pinfo->fd->flags.visited) {
    switch(protocolOpTag){
      case LDAP_REQ_BIND:
      case LDAP_REQ_SEARCH:
      case LDAP_REQ_MODIFY:
      case LDAP_REQ_ADD:
      case LDAP_REQ_DELETE:
      case LDAP_REQ_MODRDN:
      case LDAP_REQ_COMPARE:
      /*case LDAP_REQ_ABANDON: we dont match for this one*/
      /*case LDAP_REQ_UNBIND: we dont match for this one*/
        /* check that we dont already have one of those in the
           unmatched list and if so remove it */
        lcr.messageId=messageId;
        lcrp=g_hash_table_lookup(ldap_info->unmatched, &lcr);
        if(lcrp){
          g_hash_table_remove(ldap_info->unmatched, lcrp);
        }
        /* if we cant reuse the old one, grab a new chunk */
        if(!lcrp){
          lcrp=g_mem_chunk_alloc(ldap_call_response_chunk);
        }
        lcrp->messageId=messageId;
        lcrp->req_frame=pinfo->fd->num;
        lcrp->req_time.secs=pinfo->fd->abs_secs;
        lcrp->req_time.nsecs=pinfo->fd->abs_usecs*1000;
        lcrp->rep_frame=0;
        lcrp->protocolOpTag=protocolOpTag;
        lcrp->is_request=TRUE;
        g_hash_table_insert(ldap_info->unmatched, lcrp, lcrp);
        return NULL;
        break;
      case LDAP_RES_BIND:
      case LDAP_RES_SEARCH_ENTRY:
      case LDAP_RES_SEARCH_REF:
      case LDAP_RES_SEARCH_RESULT:
      case LDAP_RES_MODIFY:
      case LDAP_RES_ADD:
      case LDAP_RES_DELETE:
      case LDAP_RES_MODRDN:
      case LDAP_RES_COMPARE:
        lcr.messageId=messageId;
        lcrp=g_hash_table_lookup(ldap_info->unmatched, &lcr);
        if(lcrp){
          if(!lcrp->rep_frame){
            g_hash_table_remove(ldap_info->unmatched, lcrp);
            lcrp->rep_frame=pinfo->fd->num;
            lcrp->is_request=FALSE;
            g_hash_table_insert(ldap_info->matched, lcrp, lcrp);
          }
        }
    }
  }

  if(!lcrp){
    lcr.messageId=messageId;
    switch(protocolOpTag){
      case LDAP_REQ_BIND:
      case LDAP_REQ_SEARCH:
      case LDAP_REQ_MODIFY:
      case LDAP_REQ_ADD:
      case LDAP_REQ_DELETE:
      case LDAP_REQ_MODRDN:
      case LDAP_REQ_COMPARE:
      /*case LDAP_REQ_ABANDON: we dont match for this one*/
      /*case LDAP_REQ_UNBIND: we dont match for this one*/
        lcr.is_request=TRUE;
        lcr.req_frame=pinfo->fd->num;
        lcr.rep_frame=0;
        break;
      case LDAP_RES_BIND:
      case LDAP_RES_SEARCH_ENTRY:
      case LDAP_RES_SEARCH_REF:
      case LDAP_RES_SEARCH_RESULT:
      case LDAP_RES_MODIFY:
      case LDAP_RES_ADD:
      case LDAP_RES_DELETE:
      case LDAP_RES_MODRDN:
      case LDAP_RES_COMPARE:
        lcr.is_request=FALSE;
        lcr.req_frame=0;
        lcr.rep_frame=pinfo->fd->num;
        break;
    }
    lcrp=g_hash_table_lookup(ldap_info->matched, &lcr);
    if(lcrp){
      lcrp->is_request=lcr.is_request;
    }
  }
  if(lcrp){
    if(lcrp->is_request){
      proto_tree_add_uint(tree, hf_ldap_response_in, tvb, 0, 0, lcrp->rep_frame);
    } else {
      nstime_t ns;
      proto_tree_add_uint(tree, hf_ldap_response_to, tvb, 0, 0, lcrp->req_frame);
      ns.secs=pinfo->fd->abs_secs-lcrp->req_time.secs;
      ns.nsecs=pinfo->fd->abs_usecs*1000-lcrp->req_time.nsecs;
      if(ns.nsecs<0){
        ns.nsecs+=1000000000;
        ns.secs--;
      }
      proto_tree_add_time(tree, hf_ldap_time, tvb, 0, 0, &ns);
    }
    return lcrp;
  }
  return NULL;
}


static void
dissect_ldap_message(tvbuff_t *tvb, int offset, packet_info *pinfo,
		proto_tree *ldap_tree, proto_item *ldap_item, 
		gboolean first_time, ldap_conv_info_t *ldap_info,
		gboolean is_mscldap)
{
  int message_id_start;
  int message_id_length;
  guint messageLength;
  guint messageId;
  int next_offset;
  guint protocolOpCls, protocolOpCon, protocolOpTag;
  gchar *typestr;
  guint opLen;
  ASN1_SCK a;
  int start;
  int ret;
  ldap_call_response_t *lcrp;

  asn1_open(&a, tvb, offset);

  ret = read_sequence(&a, &messageLength);
  if (ret != ASN1_ERR_NOERROR)
  {
    if (first_time)
    {
      if (check_col(pinfo->cinfo, COL_INFO))
      {
        col_add_fstr(pinfo->cinfo, COL_INFO,
                    "Invalid LDAP message (Can't parse sequence header: %s)",
                    asn1_err_to_str(ret));
      }
    }
    if (ldap_tree)
    {
      proto_tree_add_text(ldap_tree, tvb, offset, -1,
			  "Invalid LDAP message (Can't parse sequence header: %s)",
			  asn1_err_to_str(ret));
    }
    return;
  }

  message_id_start = a.offset;
  ret = read_integer(&a, 0, hf_ldap_message_id, 0, &messageId, ASN1_INT);
  if (ret != ASN1_ERR_NOERROR)
  {
    if (first_time && check_col(pinfo->cinfo, COL_INFO))
      col_add_fstr(pinfo->cinfo, COL_INFO, "Invalid LDAP packet (Can't parse Message ID: %s)",
                   asn1_err_to_str(ret));
    if (ldap_tree)
      proto_tree_add_text(ldap_tree, tvb, message_id_start, 1,
                          "Invalid LDAP packet (Can't parse Message ID: %s)",
                          asn1_err_to_str(ret));
      return;
  }
  message_id_length = a.offset - message_id_start;

  start = a.offset;
  asn1_id_decode(&a, &protocolOpCls, &protocolOpCon, &protocolOpTag);
  if (protocolOpCls != ASN1_APL)
    typestr = "Bad message type (not Application)";
  else
    typestr = val_to_str(protocolOpTag, msgTypes, "Unknown message type (%u)");

  if (first_time)
  {
    if (check_col(pinfo->cinfo, COL_INFO))
      col_add_fstr(pinfo->cinfo, COL_INFO, "MsgId=%u %s",
		   messageId, typestr);
  }

  if (ldap_item)
	  proto_item_append_text(ldap_item, ", %s", 
				 val_to_str(protocolOpTag, msgTypes,
					    "Unknown message type (%u)"));

  if (ldap_tree)
  {
    proto_tree_add_uint(ldap_tree, hf_ldap_message_id, tvb, message_id_start, message_id_length, messageId);
    if (protocolOpCls == ASN1_APL)
    {
      proto_tree_add_uint(ldap_tree, hf_ldap_message_type, tvb,
			  start, a.offset - start, protocolOpTag);
    }
    else
    {
      proto_tree_add_text(ldap_tree, tvb, start, a.offset - start,
			  "%s", typestr);
    }
  }
  start = a.offset;
  if (read_length(&a, ldap_tree, hf_ldap_message_length, &opLen) != ASN1_ERR_NOERROR)
    return;

  if (protocolOpCls == ASN1_APL)
  {
    lcrp=ldap_match_call_response(tvb, pinfo, ldap_tree, ldap_info, messageId, protocolOpTag);
    if(lcrp){
      tap_queue_packet(ldap_tap, pinfo, lcrp);
    }

    switch (protocolOpTag)
    {
     case LDAP_REQ_BIND:
      dissect_ldap_request_bind(&a, ldap_tree, tvb, pinfo, ldap_info);
      break;
     case LDAP_REQ_UNBIND:
      /* Nothing to dissect */
      break;
     case LDAP_REQ_SEARCH:
      dissect_ldap_request_search(&a, ldap_tree, pinfo);
      break;
     case LDAP_REQ_MODIFY:
      dissect_ldap_request_modify(&a, ldap_tree);
      break;
     case LDAP_REQ_ADD:
      dissect_ldap_request_add(&a, ldap_tree, pinfo);
      break;
     case LDAP_REQ_DELETE:
      dissect_ldap_request_delete(&a, ldap_tree, start, opLen);
      break;
     case LDAP_REQ_MODRDN:
      dissect_ldap_request_modifyrdn(&a, ldap_tree, opLen);
      break;
     case LDAP_REQ_COMPARE:
      dissect_ldap_request_compare(&a, ldap_tree);
      break;
     case LDAP_REQ_ABANDON:
      dissect_ldap_request_abandon(&a, ldap_tree, start, opLen);
      break;
     case LDAP_RES_BIND:
      dissect_ldap_response_bind(&a, ldap_tree, start, opLen, tvb, pinfo, ldap_info);
      break;
     case LDAP_RES_SEARCH_ENTRY: {
	    /*
	     * XXX - this assumes that the LDAP_RES_SEARCH_ENTRY and
	     * LDAP_RES_SEARCH_RESULT appear in the same frame.
	     */
	    guint32 *num_results = p_get_proto_data(pinfo->fd, proto_ldap);

	    if (!num_results) {
		    num_results = g_malloc(sizeof(guint32));
		    *num_results = 0;
		    p_add_proto_data(pinfo->fd, proto_ldap, num_results);
	    }

	    *num_results += 1;
	    dissect_ldap_response_search_entry(&a, ldap_tree, is_mscldap);

	    break;
     }
     case LDAP_RES_SEARCH_REF:
      dissect_ldap_response_search_ref(&a, ldap_tree);
      break;

     case LDAP_RES_SEARCH_RESULT: {
	     guint32 *num_results = p_get_proto_data(pinfo->fd, proto_ldap);

	     if (num_results) {
		     if (check_col(pinfo->cinfo, COL_INFO))
			     col_append_fstr(pinfo->cinfo, COL_INFO, ", %d result%s", 
					     *num_results, *num_results == 1 ? "" : "s");
		     g_free(num_results);
		     p_rem_proto_data(pinfo->fd, proto_ldap);
	     }

	     dissect_ldap_result(&a, ldap_tree, pinfo);

	     break;
     }

     case LDAP_RES_MODIFY:
     case LDAP_RES_ADD:
     case LDAP_RES_DELETE:
     case LDAP_RES_MODRDN:
     case LDAP_RES_COMPARE:
        dissect_ldap_result(&a, ldap_tree, pinfo);
      break;
     default:
      if (ldap_tree)
      {
        proto_tree_add_text(ldap_tree, a.tvb, a.offset, opLen,
                            "Unknown LDAP operation (%u)", protocolOpTag);
      }
      break;
    }
  }

  /*
   * XXX - what if "next_offset" is past the offset of the next top-level
   * sequence?  Show that as an error?
   */
  asn1_close(&a, &next_offset);	/* XXX - use the new value of next_offset? */
}


static void
dissect_ldap_pdu(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, gboolean is_mscldap)
{
  int offset = 0;
  gboolean first_time = TRUE;
  conversation_t *conversation;
  gboolean doing_sasl_security = FALSE;
  guint length_remaining;
  guint32 sasl_length;
  guint32 message_data_len;
  proto_item *ti = NULL;
  proto_tree *ldap_tree = NULL;
  ASN1_SCK a;
  int ret;
  guint messageLength;
  int messageOffset;
  guint headerLength;
  guint length;
  gint available_length, reported_length;
  int len;
  proto_item *gitem = NULL;
  proto_tree *gtree = NULL;
  tvbuff_t *next_tvb;
  ldap_conv_info_t *ldap_info=NULL;


  /*
   * Do we have a conversation for this connection?
   */
  conversation = find_conversation(&pinfo->src, &pinfo->dst,
                                   pinfo->ptype, pinfo->srcport,
                                   pinfo->destport, 0);
  if (conversation == NULL) {
    /* We don't yet have a conversation, so create one. */
    conversation = conversation_new(&pinfo->src, &pinfo->dst,
                                    pinfo->ptype, pinfo->srcport,
                                    pinfo->destport, 0);
  }
  /*
   * Do we already have a type and mechanism?
   */
  ldap_info = conversation_get_proto_data(conversation, proto_ldap);
  if (ldap_info == NULL) {
    /* No.  Attach that information to the conversation, and add
       it to the list of information structures. */
    ldap_info = g_mem_chunk_alloc(ldap_conv_info_chunk);
    ldap_info->auth_type = 0;
    ldap_info->auth_mech = 0;
    ldap_info->first_auth_frame = 0;
    ldap_info->matched=g_hash_table_new(ldap_info_hash_matched, ldap_info_equal_matched);
    ldap_info->unmatched=g_hash_table_new(ldap_info_hash_unmatched, ldap_info_equal_unmatched);
    conversation_add_proto_data(conversation, proto_ldap, ldap_info);
    ldap_info->next = ldap_info_items;
    ldap_info_items = ldap_info;
  } 

  switch (ldap_info->auth_type) {
    case LDAP_AUTH_SASL:
      /*
       * It's SASL; are we using a security layer?
       */
      if (ldap_info->first_auth_frame != 0 &&
          pinfo->fd->num >= ldap_info->first_auth_frame)
        doing_sasl_security = TRUE;	/* yes */
  }




  while (tvb_reported_length_remaining(tvb, offset) > 0) {
    /*
     * This will throw an exception if we don't have any data left.
     * That's what we want.  (See "tcp_dissect_pdus()", which is
     * similar, but doesn't have to deal with the SASL issues.
     * XXX - can we make "tcp_dissect_pdus()" provide enough information
     * to the "get_pdu_len" routine so that we could have one dealing
     * with the SASL issues, have that routine deal with SASL and
     * ASN.1, and just use "tcp_dissect_pdus()"?)
     */
    length_remaining = tvb_ensure_length_remaining(tvb, offset);

    /*
     * Might we be doing a SASL security layer and, if so, *are* we doing
     * one?
     *
     * Just because we've seen a bind reply for SASL, that doesn't mean
     * that we're using a SASL security layer; I've seen captures in
     * which some SASL negotiations lead to a security layer being used
     * and other negotiations don't, and it's not obvious what's different
     * in the two negotiations.  Therefore, we assume that if the first
     * byte is 0, it's a length for a SASL security layer (that way, we
     * never reassemble more than 16 megabytes, protecting us from
     * chewing up *too* much memory), and otherwise that it's an LDAP
     * message (actually, if it's an LDAP message it should begin with 0x30,
     * but we want to parse garbage as LDAP messages rather than really
     * huge lengths).
     */
    if (doing_sasl_security && tvb_get_guint8(tvb, offset) == 0) {
      /*
       * Yes.  The frame begins with a 4-byte big-endian length.
       * Can we do reassembly?
       */
      if (ldap_desegment && pinfo->can_desegment) {
        /*
         * Yes - is the SASL length split across segment boundaries?
         */
        if (length_remaining < 4) {
          /*
           * Yes.  Tell the TCP dissector where the data for this message
           * starts in the data it handed us, and how many more bytes we
           * need, and return.
           */
          pinfo->desegment_offset = offset;
          pinfo->desegment_len = 4 - length_remaining;
          return;
        }
      }

      /*
       * Get the SASL length, which is the length of data in the buffer
       * following the length (i.e., it's 4 less than the total length).
       *
       * XXX - do we need to reassemble buffers?  For now, we
       * assume that each LDAP message is entirely contained within
       * a buffer.
       */
      sasl_length = tvb_get_ntohl(tvb, offset);
      message_data_len = sasl_length + 4;
      if (message_data_len < 4) {
        /*
         * The message length was probably so large that the total length
	 * overflowed.
         *
         * Report this as an error.
         */
        show_reported_bounds_error(tvb, pinfo, tree);
        return;
      }

      /*
       * Is the buffer split across segment boundaries?
       */
      if (length_remaining < message_data_len) {
        /* provide a hint to TCP where the next PDU starts */
        pinfo->want_pdu_tracking=2;
        pinfo->bytes_until_next_pdu=message_data_len-length_remaining;
        /*
         * Can we do reassembly?
         */
        if (ldap_desegment && pinfo->can_desegment) {
          /*
           * Yes.  Tell the TCP dissector where the data for this message
           * starts in the data it handed us, and how many more bytes we
           * need, and return.
           */
          pinfo->desegment_offset = offset;
          pinfo->desegment_len = message_data_len - length_remaining;
          return;
        }
      }

      /*
       * Construct a tvbuff containing the amount of the payload we have
       * available.  Make its reported length the amount of data in the PDU.
       *
       * XXX - if reassembly isn't enabled. the subdissector will throw a
       * BoundsError exception, rather than a ReportedBoundsError exception.
       * We really want a tvbuff where the length is "length", the reported
       * length is "plen", and the "if the snapshot length were infinite"
       * length is the minimum of the reported length of the tvbuff handed
       * to us and "plen", with a new type of exception thrown if the offset
       * is within the reported length but beyond that third length, with
       * that exception getting the "Unreassembled Packet" error.
       */
      length = length_remaining;
      if (length > message_data_len)
        length = message_data_len;
      next_tvb = tvb_new_subset(tvb, offset, length, message_data_len);

      /*
       * If this is the first PDU, set the Protocol column and clear the
       * Info column.
       */
      if (first_time)
      {
        if (check_col(pinfo->cinfo, COL_PROTOCOL))
          col_set_str(pinfo->cinfo, COL_PROTOCOL, (gchar *)pinfo->current_proto);
        if (check_col(pinfo->cinfo, COL_INFO))
          col_clear(pinfo->cinfo, COL_INFO);
      }

      if (tree)
      {
        ti = proto_tree_add_item(tree, proto_ldap, next_tvb, 0, -1, FALSE);
        ldap_tree = proto_item_add_subtree(ti, ett_ldap);

        proto_tree_add_uint(ldap_tree, hf_ldap_sasl_buffer_length, tvb, 0, 4,
                            sasl_length);
      }

      if (ldap_info->auth_mech != NULL &&
          strcmp(ldap_info->auth_mech, "GSS-SPNEGO") == 0) {
          /*
           * This is GSS-API (using SPNEGO, but we should be done with
           * the negotiation by now).
           *
           * Dissect the GSS_Wrap() token; it'll return the length of
           * the token, from which we compute the offset in the tvbuff at
           * which the plaintext data, i.e. the LDAP message, begins.
           */
          available_length = tvb_length_remaining(tvb, 4);
          reported_length = tvb_reported_length_remaining(tvb, 4);
          g_assert(available_length >= 0);
          g_assert(reported_length >= 0);
          if (available_length > reported_length)
            available_length = reported_length;
          if ((guint)available_length > sasl_length - 4)
            available_length = sasl_length - 4;
          if ((guint)reported_length > sasl_length - 4)
            reported_length = sasl_length - 4;
          next_tvb = tvb_new_subset(tvb, 4, available_length, reported_length);
          if (tree)
          {
            gitem = proto_tree_add_text(ldap_tree, next_tvb, 0, -1, "GSS-API Token");
            gtree = proto_item_add_subtree(gitem, ett_ldap_gssapi_token);
          }
          len = call_dissector(gssapi_wrap_handle, next_tvb, pinfo, gtree);
          /*
           * if len is 0 it probably mean that we got a PDU that is not
           * aligned to the start of the segment.
           */
          if(len==0){
             return;
          }
          if (gitem != NULL)
              proto_item_set_len(gitem, len);

          /*
           * Now dissect the LDAP message.
           */
          dissect_ldap_message(tvb, 4 + len, pinfo, ldap_tree, ti, first_time, ldap_info, is_mscldap);
      } else {
        /*
         * We don't know how to handle other authentication mechanisms
         * yet, so just put in an entry for the SASL buffer.
         */
        proto_tree_add_text(ldap_tree, tvb, 4, -1, "SASL buffer");
      }
      offset += message_data_len;
    } else {
      /*
       * No, we're not doing a SASL security layer.  The frame begins
       * with a "Sequence Of" header.
       * Can we do reassembly?
       */
      if (ldap_desegment && pinfo->can_desegment) {
        /*
         * Yes - is the "Sequence Of" header split across segment
         * boundaries?  We require at least 6 bytes for the header
         * which allows for a 4 byte length (ASN.1 BER).
         */
        if (length_remaining < 6) {
          pinfo->desegment_offset = offset;
          pinfo->desegment_len = 6 - length_remaining;
          return;
        }
      }

      /* It might still be a packet containing a SASL security layer
       * but its just that we never saw the BIND packet.
       * check if it looks like it could be a SASL blob here
       * and in that case just assume it is GSS-SPNEGO
       */
      if( (tvb_bytes_exist(tvb, offset, 5))
        &&(tvb_get_ntohl(tvb, offset)<=(guint)(tvb_reported_length_remaining(tvb, offset)-4))
        &&(tvb_get_guint8(tvb, offset+4)==0x60) ){
         ldap_info->auth_type=LDAP_AUTH_SASL;
         ldap_info->first_auth_frame=pinfo->fd->num;
         ldap_info->auth_mech=g_strdup("GSS-SPNEGO");
         doing_sasl_security=TRUE;
         continue;
      }


      /*
       * OK, try to read the "Sequence Of" header; this gets the total
       * length of the LDAP message.
       */
      asn1_open(&a, tvb, offset);
      ret = read_sequence(&a, &messageLength);
      asn1_close(&a, &messageOffset);

      if (ret == ASN1_ERR_NOERROR) {
      	/*
      	 * Add the length of the "Sequence Of" header to the message
      	 * length.
      	 */
      	headerLength = messageOffset - offset;
      	messageLength += headerLength;
        if (messageLength < headerLength) {
          /*
           * The message length was probably so large that the total length
           * overflowed.
           *
           * Report this as an error.
           */
          show_reported_bounds_error(tvb, pinfo, tree);
          return;
        }
      } else {
      	/*
      	 * We couldn't parse the header; just make it the amount of data
      	 * remaining in the tvbuff, so we'll give up on this segment
      	 * after attempting to parse the message - there's nothing more
      	 * we can do.  "dissect_ldap_message()" will display the error.
      	 */
      	messageLength = length_remaining;
      }

      /*
       * Is the message split across segment boundaries?
       */
      if (length_remaining < messageLength) {
        /* provide a hint to TCP where the next PDU starts */
        pinfo->want_pdu_tracking=2;
        pinfo->bytes_until_next_pdu=messageLength-length_remaining;
        /*
         * Can we do reassembly?
         */
        if (ldap_desegment && pinfo->can_desegment) {
	  /*
	   * Yes.  Tell the TCP dissector where the data for this message
	   * starts in the data it handed us, and how many more bytes
	   * we need, and return.
	   */
	  pinfo->desegment_offset = offset;
	  pinfo->desegment_len = messageLength - length_remaining;
	  return;
        }
      }

      /*
       * If this is the first PDU, set the Protocol column and clear the
       * Info column.
       */
      if (first_time) {
        if (check_col(pinfo->cinfo, COL_PROTOCOL))
          col_set_str(pinfo->cinfo, COL_PROTOCOL, (gchar *)pinfo->current_proto);
        if (check_col(pinfo->cinfo, COL_INFO))
          col_clear(pinfo->cinfo, COL_INFO);
      }

      /*
       * Construct a tvbuff containing the amount of the payload we have
       * available.  Make its reported length the amount of data in the
       * LDAP message.
       *
       * XXX - if reassembly isn't enabled. the subdissector will throw a
       * BoundsError exception, rather than a ReportedBoundsError exception.
       * We really want a tvbuff where the length is "length", the reported
       * length is "plen", and the "if the snapshot length were infinite"
       * length is the minimum of the reported length of the tvbuff handed
       * to us and "plen", with a new type of exception thrown if the offset
       * is within the reported length but beyond that third length, with
       * that exception getting the "Unreassembled Packet" error.
       */
      length = length_remaining;
      if (length > messageLength)
        length = messageLength;
      next_tvb = tvb_new_subset(tvb, offset, length, messageLength);

      /*
       * Now dissect the LDAP message.
       */
      if (tree) {
        ti = proto_tree_add_item(tree, proto_ldap, next_tvb, 0, -1, FALSE);
        ldap_tree = proto_item_add_subtree(ti, ett_ldap);
      } else
        ldap_tree = NULL;
      dissect_ldap_message(next_tvb, 0, pinfo, ldap_tree, ti, first_time, ldap_info, is_mscldap);

      offset += messageLength;
    }

    first_time = FALSE;
  }
}



static void
dissect_ldap(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	dissect_ldap_pdu(tvb, pinfo, tree, FALSE);
	return;
}

static void
dissect_mscldap(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	dissect_ldap_pdu(tvb, pinfo, tree, TRUE);
	return;
}

static void
ldap_reinit(void)
{
  ldap_conv_info_t *ldap_info;

  /* Free up state attached to the ldap_info structures */
  for (ldap_info = ldap_info_items; ldap_info != NULL; ldap_info = ldap_info->next) {
    if (ldap_info->auth_mech != NULL) {
      g_free(ldap_info->auth_mech);
      ldap_info->auth_mech=NULL;
    }
    g_hash_table_destroy(ldap_info->matched);
    ldap_info->matched=NULL;
    g_hash_table_destroy(ldap_info->unmatched);
    ldap_info->unmatched=NULL;
  }

  if (ldap_conv_info_chunk != NULL)
    g_mem_chunk_destroy(ldap_conv_info_chunk);

  ldap_info_items = NULL;

  ldap_conv_info_chunk = g_mem_chunk_new("ldap_conv_info_chunk",
		sizeof(ldap_conv_info_t),
		ldap_conv_info_chunk_count * sizeof(ldap_conv_info_t),
		G_ALLOC_ONLY);

  if (ldap_call_response_chunk != NULL)
    g_mem_chunk_destroy(ldap_call_response_chunk);

  ldap_call_response_chunk = g_mem_chunk_new("ldap_call_response_chunk",
		sizeof(ldap_call_response_t),
		ldap_call_response_chunk_count * sizeof(ldap_call_response_t),
		G_ALLOC_ONLY);
}

void
proto_register_ldap(void)
{
  static value_string auth_types[] = {
    {LDAP_AUTH_SIMPLE,    "Simple"},
    {LDAP_AUTH_KRBV4LDAP, "Kerberos V4 to the LDAP server"},
    {LDAP_AUTH_KRBV4DSA,  "Kerberos V4 to the DSA"},
    {LDAP_AUTH_SASL,      "SASL"},
    {0, NULL},
  };

  static value_string search_scope[] = {
    {0x00, "Base"},
    {0x01, "Single"},
    {0x02, "Subtree"},
    {0x00, NULL},
  };

  static value_string search_dereference[] = {
    {0x00, "Never"},
    {0x01, "Searching"},
    {0x02, "Base Object"},
    {0x03, "Always"},
    {0x00, NULL},
  };

  static hf_register_info hf[] = {
    { &hf_ldap_response_in,
      { "Response In", "ldap.response_in",
        FT_FRAMENUM, BASE_DEC, NULL, 0x0,
        "The response to this packet is in this frame", HFILL }},

    { &hf_ldap_response_to,
      { "Response To", "ldap.response_to",
        FT_FRAMENUM, BASE_DEC, NULL, 0x0,
        "This is a response to the LDAP command in this frame", HFILL }},

    { &hf_ldap_time,
      { "Time", "ldap.time",
        FT_RELATIVE_TIME, BASE_NONE, NULL, 0x0,
        "The time between the Call and the Reply", HFILL }},

    { &hf_ldap_sasl_buffer_length,
      { "SASL Buffer Length",	"ldap.sasl_buffer_length",
	FT_UINT32, BASE_DEC, NULL, 0x0,
	"SASL Buffer Length", HFILL }},

    { &hf_ldap_length,
      { "Length",		"ldap.length",
	FT_UINT32, BASE_DEC, NULL, 0x0,
	"LDAP Length", HFILL }},

    { &hf_ldap_message_id,
      { "Message Id",		"ldap.message_id",
	FT_UINT32, BASE_DEC, NULL, 0x0,
	"LDAP Message Id", HFILL }},
    { &hf_ldap_message_type,
      { "Message Type",		"ldap.message_type",
	FT_UINT8, BASE_HEX, &msgTypes, 0x0,
	"LDAP Message Type", HFILL }},
    { &hf_ldap_message_length,
      { "Message Length",		"ldap.message_length",
	FT_UINT32, BASE_DEC, NULL, 0x0,
	"LDAP Message Length", HFILL }},

    { &hf_ldap_message_result,
      { "Result Code",		"ldap.result.code",
	FT_UINT8, BASE_HEX, result_codes, 0x0,
	"LDAP Result Code", HFILL }},
    { &hf_ldap_message_result_matcheddn,
      { "Matched DN",		"ldap.result.matcheddn",
	FT_STRING, BASE_NONE, NULL, 0x0,
	"LDAP Result Matched DN", HFILL }},
    { &hf_ldap_message_result_errormsg,
      { "Error Message",		"ldap.result.errormsg",
	FT_STRING, BASE_NONE, NULL, 0x0,
	"LDAP Result Error Message", HFILL }},
    { &hf_ldap_message_result_referral,
      { "Referral",		"ldap.result.referral",
	FT_STRING, BASE_NONE, NULL, 0x0,
	"LDAP Result Referral URL", HFILL }},

    { &hf_ldap_message_bind_version,
      { "Version",		"ldap.bind.version",
	FT_UINT32, BASE_DEC, NULL, 0x0,
	"LDAP Bind Version", HFILL }},
    { &hf_ldap_message_bind_dn,
      { "DN",			"ldap.bind.dn",
	FT_STRING, BASE_NONE, NULL, 0x0,
	"LDAP Bind Distinguished Name", HFILL }},
    { &hf_ldap_message_bind_auth,
      { "Auth Type",		"ldap.bind.auth_type",
	FT_UINT8, BASE_HEX, auth_types, 0x0,
	"LDAP Bind Auth Type", HFILL }},
    { &hf_ldap_message_bind_auth_password,
      { "Password",		"ldap.bind.password",
	FT_STRING, BASE_NONE, NULL, 0x0,
	"LDAP Bind Password", HFILL }},
    { &hf_ldap_message_bind_auth_mechanism,
      { "Mechanism",		"ldap.bind.mechanism",
	FT_STRING, BASE_NONE, NULL, 0x0,
	"LDAP Bind Mechanism", HFILL }},
    { &hf_ldap_message_bind_auth_credentials,
      { "Credentials",		"ldap.bind.credentials",
	FT_BYTES, BASE_NONE, NULL, 0x0,
	"LDAP Bind Credentials", HFILL }},
    { &hf_ldap_message_bind_server_credentials,
      { "Server Credentials",	"ldap.bind.server_credentials",
	FT_BYTES, BASE_NONE, NULL, 0x0,
	"LDAP Bind Server Credentials", HFILL }},

    { &hf_ldap_message_search_base,
      { "Base DN",		"ldap.search.basedn",
	FT_STRING, BASE_NONE, NULL, 0x0,
	"LDAP Search Base Distinguished Name", HFILL }},
    { &hf_ldap_message_search_scope,
      { "Scope",			"ldap.search.scope",
	FT_UINT8, BASE_HEX, search_scope, 0x0,
	"LDAP Search Scope", HFILL }},
    { &hf_ldap_message_search_deref,
      { "Dereference",		"ldap.search.dereference",
	FT_UINT8, BASE_HEX, search_dereference, 0x0,
	"LDAP Search Dereference", HFILL }},
    { &hf_ldap_message_search_sizeLimit,
      { "Size Limit",		"ldap.search.sizelimit",
	FT_UINT32, BASE_DEC, NULL, 0x0,
	"LDAP Search Size Limit", HFILL }},
    { &hf_ldap_message_search_timeLimit,
      { "Time Limit",		"ldap.search.timelimit",
	FT_UINT32, BASE_DEC, NULL, 0x0,
	"LDAP Search Time Limit", HFILL }},
    { &hf_ldap_message_search_typesOnly,
      { "Attributes Only",	"ldap.search.typesonly",
	FT_BOOLEAN, BASE_NONE, NULL, 0x0,
	"LDAP Search Attributes Only", HFILL }},
    { &hf_ldap_message_search_filter,
      { "Filter",		"ldap.search.filter",
	FT_STRING, BASE_NONE, NULL, 0x0,
	"LDAP Search Filter", HFILL }},
    { &hf_ldap_message_search_reference,
      { "Reference URL",	"ldap.search.reference",
	FT_STRING, BASE_NONE, NULL, 0x0,
	"LDAP Search Reference URL", HFILL }},
    { &hf_ldap_message_dn,
      { "Distinguished Name",	"ldap.dn",
	FT_STRING, BASE_NONE, NULL, 0x0,
	"LDAP Distinguished Name", HFILL }},
    { &hf_ldap_message_attribute,
      { "Attribute",		"ldap.attribute",
	FT_STRING, BASE_NONE, NULL, 0x0,
	"LDAP Attribute", HFILL }},
    /*
     * XXX - not all LDAP values are text strings; we'd need a file
     * describing which values (by name) are text strings and which are
     * binary.
     *
     * Some values that are, at least in Microsoft's schema, binary
     * are:
     *
     *	invocationId
     *	nTSecurityDescriptor
     *	objectGUID
     */
    { &hf_ldap_message_value,
      { "Value",		"ldap.value",
	FT_STRING, BASE_NONE, NULL, 0x0,
	"LDAP Value", HFILL }},

    { &hf_ldap_message_modrdn_name,
      { "New Name",		"ldap.modrdn.name",
	FT_STRING, BASE_NONE, NULL, 0x0,
	"LDAP New Name", HFILL }},
    { &hf_ldap_message_modrdn_delete,
      { "Delete Values",	"ldap.modrdn.delete",
	FT_BOOLEAN, BASE_NONE, NULL, 0x0,
	"LDAP Modify RDN - Delete original values", HFILL }},
    { &hf_ldap_message_modrdn_superior,
      { "New Location",		"ldap.modrdn.superior",
	FT_STRING, BASE_NONE, NULL, 0x0,
	"LDAP Modify RDN - New Location", HFILL }},

    { &hf_ldap_message_compare,
      { "Test",		"ldap.compare.test",
	FT_STRING, BASE_NONE, NULL, 0x0,
	"LDAP Compare Test", HFILL }},

    { &hf_ldap_message_modify_add,
      { "Add",			"ldap.modify.add",
	FT_STRING, BASE_NONE, NULL, 0x0,
	"LDAP Add", HFILL }},
    { &hf_ldap_message_modify_replace,
      { "Replace",		"ldap.modify.replace",
	FT_STRING, BASE_NONE, NULL, 0x0,
	"LDAP Replace", HFILL }},
    { &hf_ldap_message_modify_delete,
      { "Delete",		"ldap.modify.delete",
	FT_STRING, BASE_NONE, NULL, 0x0,
	"LDAP Delete", HFILL }},

    { &hf_ldap_message_abandon_msgid,
      { "Abandon Msg Id",	"ldap.abandon.msgid",
	FT_UINT32, BASE_DEC, NULL, 0x0,
	"LDAP Abandon Msg Id", HFILL }},

    { &hf_mscldap_netlogon_type,
      { "Type", "mscldap.netlogon.type",
        FT_UINT32, BASE_DEC, NULL, 0x0,
        "Type of <please tell ethereal developers what this type is>", HFILL }},

    { &hf_mscldap_netlogon_version,
      { "Version", "mscldap.netlogon.version",
        FT_UINT32, BASE_DEC, NULL, 0x0,
        "Version of <please tell ethereal developers what this type is>", HFILL }},

    { &hf_mscldap_netlogon_lm_token,
      { "LM Token", "mscldap.netlogon.lm_token",
        FT_UINT16, BASE_HEX, NULL, 0x0,
        "LM Token", HFILL }},

    { &hf_mscldap_netlogon_nt_token,
      { "NT Token", "mscldap.netlogon.nt_token",
        FT_UINT16, BASE_HEX, NULL, 0x0,
        "NT Token", HFILL }},

    { &hf_mscldap_netlogon_flags,
      { "Flags", "mscldap.netlogon.flags",
        FT_UINT32, BASE_HEX, NULL, 0x0,
        "Netlogon flags describing the DC properties", HFILL }},

    { &hf_mscldap_domain_guid,
      { "Domain GUID", "mscldap.domain.guid",
        FT_BYTES, BASE_HEX, NULL, 0x0,
        "Domain GUID", HFILL }},

    { &hf_mscldap_forest,
      { "Forest", "mscldap.forest",
        FT_STRING, BASE_NONE, NULL, 0x0,
        "Forest", HFILL }},

    { &hf_mscldap_domain,
      { "Domain", "mscldap.domain",
        FT_STRING, BASE_NONE, NULL, 0x0,
        "Domainname", HFILL }},

    { &hf_mscldap_hostname,
      { "Hostname", "mscldap.hostname",
        FT_STRING, BASE_NONE, NULL, 0x0,
        "Hostname", HFILL }},

    { &hf_mscldap_nb_domain,
      { "NetBios Domain", "mscldap.nb_domain",
        FT_STRING, BASE_NONE, NULL, 0x0,
        "NetBios Domainname", HFILL }},

    { &hf_mscldap_nb_hostname,
      { "NetBios Hostname", "mscldap.nb_hostname",
        FT_STRING, BASE_NONE, NULL, 0x0,
        "NetBios Hostname", HFILL }},

    { &hf_mscldap_username,
      { "User", "mscldap.username",
        FT_STRING, BASE_NONE, NULL, 0x0,
        "User name", HFILL }},

    { &hf_mscldap_sitename,
      { "Site", "mscldap.sitename",
        FT_STRING, BASE_NONE, NULL, 0x0,
        "Site name", HFILL }},

    { &hf_mscldap_clientsitename,
      { "Client Site", "mscldap.clientsitename",
        FT_STRING, BASE_NONE, NULL, 0x0,
        "Client Site name", HFILL }},

    { &hf_mscldap_netlogon_flags_pdc,
      { "PDC", "mscldap.netlogon.flags.pdc", FT_BOOLEAN, 32,
        TFS(&tfs_ads_pdc), 0x00000001, "Is this DC a PDC or not?", HFILL }},

    { &hf_mscldap_netlogon_flags_gc,
      { "GC", "mscldap.netlogon.flags.gc", FT_BOOLEAN, 32,
        TFS(&tfs_ads_gc), 0x00000004, "Does this dc service as a GLOBAL CATALOGUE?", HFILL }},

    { &hf_mscldap_netlogon_flags_ldap,
      { "LDAP", "mscldap.netlogon.flags.ldap", FT_BOOLEAN, 32,
        TFS(&tfs_ads_ldap), 0x00000008, "Does this DC act as an LDAP server?", HFILL }},

    { &hf_mscldap_netlogon_flags_ds,
      { "DS", "mscldap.netlogon.flags.ds", FT_BOOLEAN, 32,
        TFS(&tfs_ads_ds), 0x00000010, "Does this dc provide DS services?", HFILL }},

    { &hf_mscldap_netlogon_flags_kdc,
      { "KDC", "mscldap.netlogon.flags.kdc", FT_BOOLEAN, 32,
        TFS(&tfs_ads_kdc), 0x00000020, "Does this dc act as a KDC?", HFILL }},

    { &hf_mscldap_netlogon_flags_timeserv,
      { "Time Serv", "mscldap.netlogon.flags.timeserv", FT_BOOLEAN, 32,
        TFS(&tfs_ads_timeserv), 0x00000040, "Does this dc provide time services (ntp) ?", HFILL }},

    { &hf_mscldap_netlogon_flags_closest,
      { "Closest", "mscldap.netlogon.flags.closest", FT_BOOLEAN, 32,
        TFS(&tfs_ads_closest), 0x00000080, "Is this the closest dc? (is this used at all?)", HFILL }},

    { &hf_mscldap_netlogon_flags_writable,
      { "Writable", "mscldap.netlogon.flags.writable", FT_BOOLEAN, 32,
        TFS(&tfs_ads_writable), 0x00000100, "Is this dc writable? (i.e. can it update the AD?)", HFILL }},

    { &hf_mscldap_netlogon_flags_good_timeserv,
      { "Good Time Serv", "mscldap.netlogon.flags.good_timeserv", FT_BOOLEAN, 32,
        TFS(&tfs_ads_good_timeserv), 0x00000200, "Is this a Good Time Server? (i.e. does it have a hardware clock)", HFILL }},

    { &hf_mscldap_netlogon_flags_ndnc,
      { "NDNC", "mscldap.netlogon.flags.ndnc", FT_BOOLEAN, 32,
        TFS(&tfs_ads_ndnc), 0x00000400, "Is this an NDNC dc?", HFILL }},

  };

  static gint *ett[] = {
    &ett_ldap,
    &ett_ldap_gssapi_token,
    &ett_ldap_referrals,
    &ett_ldap_attribute,
    &ett_mscldap_netlogon_flags
  };
  module_t *ldap_module;

  proto_ldap = proto_register_protocol("Lightweight Directory Access Protocol",
				       "LDAP", "ldap");
  proto_register_field_array(proto_ldap, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));

  ldap_module = prefs_register_protocol(proto_ldap, NULL);
  prefs_register_bool_preference(ldap_module, "desegment_ldap_messages",
    "Reassemble LDAP messages spanning multiple TCP segments",
    "Whether the LDAP dissector should reassemble messages spanning multiple TCP segments."
    " To use this option, you must also enable \"Allow subdissectors to reassemble TCP streams\" in the TCP protocol settings.",
    &ldap_desegment);

  proto_cldap = proto_register_protocol(
	  "Connectionless Lightweight Directory Access Protocol",
	  "CLDAP", "cldap");

  register_init_routine(ldap_reinit);
  ldap_tap=register_tap("ldap");
}

void
proto_reg_handoff_ldap(void)
{
  dissector_handle_t ldap_handle, cldap_handle;

  ldap_handle = create_dissector_handle(dissect_ldap, proto_ldap);
  dissector_add("tcp.port", TCP_PORT_LDAP, ldap_handle);
  dissector_add("tcp.port", TCP_PORT_GLOBALCAT_LDAP, ldap_handle);

  cldap_handle = create_dissector_handle(dissect_mscldap, proto_cldap);
  dissector_add("udp.port", UDP_PORT_CLDAP, cldap_handle);

  gssapi_handle = find_dissector("gssapi");
  gssapi_wrap_handle = find_dissector("gssapi_verf");
}
