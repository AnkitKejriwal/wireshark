/* packet-diameter.c
 * Routines for Diameter packet disassembly
 *
 * $Id$
 *
 * Copyright (c) 2001 by David Frascone <dave@frascone.com>
 * Copyright (c) 2007 by Luis E. Garcia Ontanon <luis.ontanon@gmail.com>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * References:
 * 2004-03-11
 * http://www.ietf.org/rfc/rfc3588.txt
 * http://www.iana.org/assignments/radius-types
 * http://www.ietf.org/internet-drafts/draft-ietf-aaa-diameter-cc-03.txt
 * http://www.ietf.org/internet-drafts/draft-ietf-aaa-diameter-nasreq-14.txt
 * http://www.ietf.org/internet-drafts/draft-ietf-aaa-diameter-mobileip-16.txt
 * http://www.ietf.org/internet-drafts/draft-ietf-aaa-diameter-sip-app-01.txt
 * http://www.ietf.org/html.charters/aaa-charter.html
 * http://www.iana.org/assignments/address-family-numbers
 * http://www.iana.org/assignments/enterprise-numbers
 * http://www.iana.org/assignments/aaa-parameters
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <glib.h>
#include <epan/filesystem.h>
#include <epan/xmlstub.h>
#include <epan/packet.h>
#include <epan/addr_resolv.h>
#include <epan/report_err.h>
#include <epan/prefs.h>
#include <epan/sminmpec.h>
#include <epan/emem.h>
#include <epan/expert.h>
#include "packet-tcp.h"
#include "packet-sip.h"
#include "diam_dict.h"

#define  NTP_TIME_DIFF                   (2208988800U)

#define TCP_PORT_DIAMETER	3868
#define SCTP_PORT_DIAMETER	3868

/* Diameter Header Flags */
/*                                      RPrrrrrrCCCCCCCCCCCCCCCCCCCCCCCC  */
#define DIAM_FLAGS_R 0x80
#define DIAM_FLAGS_P 0x40
#define DIAM_FLAGS_E 0x20
#define DIAM_FLAGS_T 0x10
#define DIAM_FLAGS_RESERVED4 0x08
#define DIAM_FLAGS_RESERVED5 0x04
#define DIAM_FLAGS_RESERVED6 0x02
#define DIAM_FLAGS_RESERVED7 0x01
#define DIAM_FLAGS_RESERVED  0x0f

#define DIAM_LENGTH_MASK  0x00ffffffl
#define DIAM_COMMAND_MASK DIAM_LENGTH_MASK
#define DIAM_GET_FLAGS(dh)                ((dh.flagsCmdCode & ~DIAM_COMMAND_MASK) >> 24)
#define DIAM_GET_VERSION(dh)              ((dh.versionLength & (~DIAM_LENGTH_MASK)) >> 24)
#define DIAM_GET_COMMAND(dh)              (dh.flagsCmdCode & DIAM_COMMAND_MASK)
#define DIAM_GET_LENGTH(dh)               (dh.versionLength & DIAM_LENGTH_MASK)

/* Diameter AVP Flags */
#define AVP_FLAGS_P 0x20
#define AVP_FLAGS_V 0x80
#define AVP_FLAGS_M 0x40
#define AVP_FLAGS_RESERVED3 0x10
#define AVP_FLAGS_RESERVED4 0x08
#define AVP_FLAGS_RESERVED5 0x04
#define AVP_FLAGS_RESERVED6 0x02
#define AVP_FLAGS_RESERVED7 0x01
#define AVP_FLAGS_RESERVED 0x1f          /* 00011111  -- V M P X X X X X */

#define DIAMETER_V16 16
#define DIAMETER_RFC 1

typedef struct _diam_ctx_t {
	proto_tree* tree;
	emem_tree_t* avps;
	gboolean version_rfc;
} diam_ctx_t;

typedef struct _diam_avp_t diam_avp_t;
typedef struct _avp_type_t avp_type_t;

typedef const char* (*diam_avp_dissector_t)(diam_ctx_t*, diam_avp_t*, tvbuff_t*);

struct _diam_avp_t {
	guint32 code;
	guint32 vendor;
	diam_avp_dissector_t dissector_v16;
	diam_avp_dissector_t dissector_rfc;

	gint ett;
	int hf_value;
	void* type_data;
};

typedef struct _diam_vnd_t {
	guint32 code;
	GArray* vs_avps;
	GArray* vs_cmds;
} diam_vnd_t;

#define VND_AVP_VS(v) ((value_string*)((v)->vs_avps->data))
#define VND_CMD_VS(v) ((value_string*)((v)->vs_cmds->data))

typedef struct _diam_dictionary_t {
	emem_tree_t* avps;
	emem_tree_t* vnds;
	value_string* applications;
	value_string* commands;
} diam_dictionary_t;

typedef diam_avp_t* (*avp_constructor_t)(const avp_type_t*, guint32 code, guint32 vendor, const char* name,  value_string* vs);

struct _avp_type_t {
	char* name;
	diam_avp_dissector_t v16;
	diam_avp_dissector_t rfc;
	enum ftenum ft;
	base_display_e base;
	avp_constructor_t build;
};

struct _build_dict {
	GArray* hf;
	GArray* ett;
	GHashTable* types;
	GHashTable* avps;
};

static const char* simple_avp(diam_ctx_t*, diam_avp_t*, tvbuff_t*);

static diam_avp_t unknown_avp = {0, 0, simple_avp, simple_avp, -1, -1, NULL };
static value_string no_vs[] = {{0, NULL} };
static GArray no_garr = { (void*)no_vs, 1 };
static diam_vnd_t unknown_vendor = { 0, &no_garr, &no_garr };
static diam_vnd_t no_vnd = { 0, NULL, NULL };
static diam_dictionary_t dictionary = { NULL, NULL, NULL, NULL };
static struct _build_dict build_dict;
static value_string* vnd_short_vs;
static const true_false_string reserved_set = {
	"Set",
	"Unset"
};

static int proto_diameter = -1;
static int hf_diameter_length = -1;
static int hf_diameter_code = -1;
static int hf_diameter_hopbyhopid =-1;
static int hf_diameter_endtoendid =-1;
static int hf_diameter_version = -1;
static int hf_diameter_vendor_id = -1;
static int hf_diameter_application_id = -1;
static int hf_diameter_flags = -1;
static int hf_diameter_flags_request = -1;
static int hf_diameter_flags_proxyable = -1;
static int hf_diameter_flags_error = -1;
static int hf_diameter_flags_T		= -1;
static int hf_diameter_flags_reserved4 = -1;
static int hf_diameter_flags_reserved5 = -1;
static int hf_diameter_flags_reserved6 = -1;
static int hf_diameter_flags_reserved7 = -1;

static int hf_diameter_avp = -1;
static int hf_diameter_avp_len = -1;
static int hf_diameter_avp_code = -1;
static int hf_diameter_avp_flags = -1;
static int hf_diameter_avp_flags_vendor_specific = -1;
static int hf_diameter_avp_flags_mandatory = -1;
static int hf_diameter_avp_flags_protected = -1;
static int hf_diameter_avp_flags_reserved3 = -1;
static int hf_diameter_avp_flags_reserved4 = -1;
static int hf_diameter_avp_flags_reserved5 = -1;
static int hf_diameter_avp_flags_reserved6 = -1;
static int hf_diameter_avp_flags_reserved7 = -1;
static int hf_diameter_avp_vendor_id = -1;

static gint ett_diameter = -1;
static gint ett_diameter_flags = -1;
static gint ett_diameter_avp_flags = -1;
static gint ett_diameter_avpinfo = -1;

static guint gbl_diameterTcpPort=TCP_PORT_DIAMETER;
static guint gbl_diameterSctpPort=SCTP_PORT_DIAMETER;

/* desegmentation of Diameter over TCP */
static gboolean gbl_diameter_desegment = TRUE;

static const char* avpflags_str[] = {
	"---",
	"--P",
	"-M-",
	"-MP",
	"V--",
	"V-P",
	"VM-",
	"VMP",
};

static int dissect_diameter_avp(diam_ctx_t* c, tvbuff_t* tvb, int offset) {
	guint32 code = tvb_get_ntohl(tvb,offset);
	guint32 len = tvb_get_ntohl(tvb,offset+4);
	guint32 vendor_flag = len & 0x80000000;
	guint32 flags_bits_idx = (len & 0xE0000000) >> 29;
	guint32 flags_bits = (len & 0xFF000000) >> 24;
	guint32 vendorid = vendor_flag ? tvb_get_ntohl(tvb,offset+8) : 0;
	emem_tree_key_t k[] = {
		{1,&code},
		{1,&vendorid},
		{0,NULL}
	};
	diam_avp_t* a = emem_tree_lookup32_array(dictionary.avps,k);
	proto_item *pi, *avp_item;
	proto_tree *avp_tree, *save_tree;
	tvbuff_t* subtvb;
	diam_vnd_t* vendor = vendor_flag ? &no_vnd : emem_tree_lookup32(dictionary.vnds,vendorid);
	value_string* vendor_avp_vs;
	const char* code_str;
	const char* avp_str;

	len &= 0x00ffffff;

	if (!a) a = &unknown_avp;
	if (! vendor) vendor = &unknown_vendor;

	vendor_avp_vs = VND_AVP_VS(vendor);

	avp_item = proto_tree_add_item(c->tree,hf_diameter_avp,tvb,offset,len,FALSE);
	avp_tree = proto_item_add_subtree(avp_item,a->ett);

	pi = proto_tree_add_item(avp_tree,hf_diameter_avp_code,tvb,offset,4,FALSE);
	code_str = val_to_str(code, vendor_avp_vs, "Unknown");
	proto_item_append_text(pi," %s", code_str);
	offset += 4;

	proto_item_set_text(avp_item,"AVP: %s(%u) l=%u f=%s", code_str, code, len, avpflags_str[flags_bits_idx]);

	pi = proto_tree_add_item(avp_tree,hf_diameter_avp_flags,tvb,offset,1,FALSE);
	{
		proto_tree* flags_tree = proto_item_add_subtree(pi,ett_diameter_avp_flags);
		proto_tree_add_item(flags_tree,hf_diameter_avp_flags_vendor_specific,tvb,offset,1,FALSE);
		proto_tree_add_item(flags_tree,hf_diameter_avp_flags_mandatory,tvb,offset,1,FALSE);
		proto_tree_add_item(flags_tree,hf_diameter_avp_flags_protected,tvb,offset,1,FALSE);
		pi = proto_tree_add_item(flags_tree,hf_diameter_avp_flags_reserved3,tvb,offset,1,FALSE);
		if(flags_bits & 0x10) proto_item_set_expert_flags(pi, PI_MALFORMED, PI_WARN);
		pi = proto_tree_add_item(flags_tree,hf_diameter_avp_flags_reserved4,tvb,offset,1,FALSE);
		if(flags_bits & 0x08) proto_item_set_expert_flags(pi, PI_MALFORMED, PI_WARN);
		pi = proto_tree_add_item(flags_tree,hf_diameter_avp_flags_reserved5,tvb,offset,1,FALSE);
		if(flags_bits & 0x04) proto_item_set_expert_flags(pi, PI_MALFORMED, PI_WARN);
		proto_tree_add_item(flags_tree,hf_diameter_avp_flags_reserved6,tvb,offset,1,FALSE);
		if(flags_bits & 0x02) proto_item_set_expert_flags(pi, PI_MALFORMED, PI_WARN);
		proto_tree_add_item(flags_tree,hf_diameter_avp_flags_reserved7,tvb,offset,1,FALSE);
		if(flags_bits & 0x01) proto_item_set_expert_flags(pi, PI_MALFORMED, PI_WARN);
	}
	offset += 1;

	proto_tree_add_item(avp_tree,hf_diameter_avp_len,tvb,offset,3,FALSE);
	offset += 3;

	if (vendor_flag) {
		proto_item_append_text(avp_item," vnd=%s", val_to_str(vendorid, vnd_short_vs, "%d"));
		proto_tree_add_item(avp_tree,hf_diameter_avp_vendor_id,tvb,offset,4,FALSE);
		offset += 4;
	}

	if ( len == (guint32)(vendor_flag ? 12 : 8) ) return len;

	subtvb = tvb_new_subset(tvb,offset,len-(8+(vendor_flag?4:0)),len-(8+(vendor_flag?4:0)));

	save_tree = c->tree;
	c->tree = avp_tree;
	if (c->version_rfc) {
		avp_str = a->dissector_rfc(c,a,subtvb);
	} else {
		avp_str = a->dissector_v16(c,a,subtvb);
	}
	c->tree = save_tree;

	if (avp_str) proto_item_append_text(avp_item," val=%s", avp_str);

	return len;
}


static const char* simple_avp(diam_ctx_t* c, diam_avp_t* a, tvbuff_t* tvb) {
	char* label = ep_alloc(ITEM_LABEL_LENGTH+1);
	proto_item* pi = proto_tree_add_item(c->tree,a->hf_value,tvb,0,tvb_length_remaining(tvb,0),FALSE);
	proto_item_fill_label(pi->finfo, label);
	label = strstr(label,": ")+2;
	return label;
}

static const char* grouped_avp(diam_ctx_t* c, diam_avp_t* a, tvbuff_t* tvb) {
	int offset = 0;
	int len = tvb_length_remaining(tvb,0);
	proto_item* pi = proto_tree_add_item(c->tree, a->hf_value, tvb , 0 , -1, FALSE);
	proto_tree* pt = c->tree;

	c->tree = proto_item_add_subtree(pi,a->ett);

	while (offset < len) {
		offset += dissect_diameter_avp(c, tvb, offset);
		offset +=  (offset % 4) ? 4 - (offset % 4) : 0 ;
	}

	c->tree = pt;

	return NULL;
}

static void dissect_diameter_common(tvbuff_t* tvb, packet_info* pinfo, proto_tree* tree) {
	guint32 first_word  = tvb_get_ntohl(tvb,0);
	guint32 version = (first_word & 0xff000000) >> 24;
	guint32 flags_bits = (tvb_get_ntohl(tvb,4) & 0xff000000) >> 24;
	int packet_len = first_word & 0x00ffffff;
	proto_item *pi;
	proto_item *cmd_item, *version_item;
	diam_ctx_t* c = ep_alloc0(sizeof(diam_ctx_t));
	int offset;
	value_string* cmd_vs;
	const char* cmd_str;
	guint32 cmd = tvb_get_ntoh24(tvb,5);
	guint32 fourth = tvb_get_ntohl(tvb,8);

	if (check_col(pinfo->cinfo, COL_PROTOCOL))
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "DIAMETER");

	pi = proto_tree_add_item(tree,proto_diameter,tvb,0,-1,FALSE);
	tree = proto_item_add_subtree(pi,ett_diameter);

	c->tree = tree;

	version_item = proto_tree_add_item(tree,hf_diameter_version,tvb,0,1,FALSE);
	proto_tree_add_item(tree,hf_diameter_length,tvb,1,3,FALSE);

	pi = proto_tree_add_item(tree,hf_diameter_flags,tvb,4,1,FALSE);
	{
		proto_tree* pt = proto_item_add_subtree(pi,ett_diameter_flags);
		proto_tree_add_item(pt,hf_diameter_flags_request,tvb,4,1,FALSE);
		proto_tree_add_item(pt,hf_diameter_flags_proxyable,tvb,4,1,FALSE);
		proto_tree_add_item(pt,hf_diameter_flags_error,tvb,4,1,FALSE);
		proto_tree_add_item(pt,hf_diameter_flags_T,tvb,4,1,FALSE);
		proto_tree_add_item(pt,hf_diameter_flags_reserved4,tvb,4,1,FALSE);
		if(flags_bits & 0x08) proto_item_set_expert_flags(pi, PI_MALFORMED, PI_WARN);
		pi = proto_tree_add_item(pt,hf_diameter_flags_reserved5,tvb,4,1,FALSE);
		if(flags_bits & 0x04) proto_item_set_expert_flags(pi, PI_MALFORMED, PI_WARN);
		pi = proto_tree_add_item(pt,hf_diameter_flags_reserved6,tvb,4,1,FALSE);
		if(flags_bits & 0x02) proto_item_set_expert_flags(pi, PI_MALFORMED, PI_WARN);
		pi = proto_tree_add_item(pt,hf_diameter_flags_reserved7,tvb,4,1,FALSE);
		if(flags_bits & 0x01) proto_item_set_expert_flags(pi, PI_MALFORMED, PI_WARN);
	}

	cmd_item = proto_tree_add_item(tree,hf_diameter_code,tvb,5,3,FALSE);

	switch (version) {
		case DIAMETER_V16: {
			guint32 vendorid = tvb_get_ntohl(tvb,8);
			diam_vnd_t* vendor;

			if (! ( vendor = emem_tree_lookup32(dictionary.vnds,vendorid) ) ) {
				vendor = &unknown_vendor;
			}

			cmd_vs = VND_CMD_VS(vendor);
			proto_tree_add_item(tree, hf_diameter_vendor_id,tvb,8,4,FALSE);

			c->version_rfc = FALSE;
			break;
		}
		case DIAMETER_RFC: {
			cmd_vs = VND_CMD_VS(&no_vnd);
			proto_tree_add_item(tree, hf_diameter_application_id,tvb,8,4,FALSE);
			c->version_rfc = TRUE;
			break;
		}
		default:
			expert_add_info_format(pinfo, version_item, PI_UNDECODED, PI_WARN, "Unknown Diameter Version");
			c->version_rfc = TRUE;
			cmd_vs = VND_CMD_VS(&no_vnd);
			break;
	}
	cmd_str = val_to_str(cmd, cmd_vs, "Unknown");

	if (check_col(pinfo->cinfo, COL_INFO))
		col_add_fstr(pinfo->cinfo, COL_INFO,
					 "cmd=%s(%d) %s=%s(%d) h2h=%x e2e=%x",
					 cmd_str,
					 cmd,
					 c->version_rfc ? "appl" : "vend",
					 val_to_str(fourth, c->version_rfc ? dictionary.applications : vnd_short_vs, "Unknown"),
					 fourth,
					 tvb_get_ntohl(tvb,12),
					 tvb_get_ntohl(tvb,16));

	proto_item_append_text(cmd_item," %s", cmd_str);

	proto_tree_add_item(tree,hf_diameter_hopbyhopid,tvb,12,4,FALSE);
	proto_tree_add_item(tree,hf_diameter_endtoendid,tvb,16,4,FALSE);

	offset = 20;

	while (offset < packet_len) {
		offset += dissect_diameter_avp(c, tvb, offset);
		offset +=  (offset % 4) ? 4 - (offset % 4) : 0 ;
	}

}

static guint
get_diameter_pdu_len(packet_info *pinfo _U_, tvbuff_t *tvb, int offset)
{
	/* Get the length of the Diameter packet. */
	return tvb_get_ntoh24(tvb, offset + 1);
}

static gboolean
check_diameter(tvbuff_t *tvb)
{
	if (!tvb_bytes_exist(tvb, 0, 1))
		return FALSE;   /* not enough bytes to check the version */

	if (tvb_get_guint8(tvb, 0) != 1)
		return FALSE;   /* not version 1 */

		  /*
		   * XXX - fetch length and make sure it's at least MIN_DIAMETER_SIZE?
		   * Fetch flags and check that none of the DIAM_FLAGS_RESERVED bits
		   * are set?
		   */
	return TRUE;
}

static int
dissect_diameter(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	if (!check_diameter(tvb))
		return 0;
	dissect_diameter_common(tvb, pinfo, tree);
	return tvb_length(tvb);
}

static void
dissect_diameter_tcp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	tcp_dissect_pdus(tvb, pinfo, tree, gbl_diameter_desegment, 4,
					 get_diameter_pdu_len, dissect_diameter_common);
} /* dissect_diameter_tcp */


static char* alnumerize(char* name) {
	char* r = name;
	char* w = name;
	char c;

	for (;(c = *r); r++) {
		if (isalnum(c) || c == '_' || c == '-' || c == '.') {
			*(w++) = c;
		}
	}

	*w = '\0';

	return name;
}


guint reginfo(int* hf_ptr,
			  const char* name,
			  const char* abbr,
			  const char* desc,
			  enum ftenum ft,
			  base_display_e base,
			  value_string* vs,
			  guint32 mask) {
	hf_register_info hf = { hf_ptr, {
		name ? g_strdup(name) : g_strdup(abbr),
		g_strdup(abbr),
		ft,
		base,
		VALS(vs),
		mask,
		desc ? g_strdup(desc) : "",
		HFILL }};

	g_array_append_vals(build_dict.hf,&hf,1);
	return build_dict.hf->len - 1;
}

void basic_avp_reginfo(diam_avp_t* a, const char* name, enum ftenum ft, base_display_e base, value_string* vs) {
	hf_register_info hf[] = {
		{ &(a->hf_value), { NULL, NULL, ft, base, VALS(vs), 0x0, "", HFILL }}
	};
	gint* ettp = &(a->ett);

	hf->hfinfo.name = g_strdup_printf("%s",name);
	hf->hfinfo.abbrev = alnumerize(g_strdup_printf("diameter.%s",name));

	g_array_append_vals(build_dict.hf,hf,1);
	g_array_append_vals(build_dict.ett,&ettp,1);
}

static diam_avp_t* build_simple_avp(const avp_type_t* type,
									guint32 code,
									guint32 vendor,
									const char* name,
									value_string* vs) {
	diam_avp_t* a = g_malloc0(sizeof(diam_avp_t));
	a->code = code;
	a->vendor = vendor;
	a->dissector_v16 = type->v16;
	a->dissector_rfc = type->rfc;
	a->ett = -1;
	a->hf_value = -1;

	basic_avp_reginfo(a,name,type->ft,type->base,vs);

	return a;
}


static const avp_type_t basic_types[] = {
	{"octetstring"				, simple_avp	, simple_avp	, FT_BYTES			, BASE_NONE , build_simple_avp },
	{"utf8string"				, simple_avp	, simple_avp	, FT_STRING			, BASE_NONE	, build_simple_avp },
	{"grouped"					, grouped_avp	, grouped_avp	, FT_BYTES			, BASE_NONE , build_simple_avp },
	{"integer32"				, simple_avp	, simple_avp	, FT_INT32			, BASE_DEC	, build_simple_avp },
	{"unsigned32"				, simple_avp	, simple_avp	, FT_UINT32			, BASE_DEC	, build_simple_avp },
	{"time"						, simple_avp	, simple_avp	, FT_BYTES			, BASE_DEC	, build_simple_avp },
	{"integer64"				, simple_avp	, simple_avp	, FT_INT64			, BASE_DEC	, build_simple_avp },
	{"unsigned64"				, simple_avp	, simple_avp	, FT_UINT64			, BASE_DEC	, build_simple_avp },
	{"float32"					, simple_avp	, simple_avp	, FT_FLOAT			, BASE_DEC	, build_simple_avp },
	{"float64"					, simple_avp	, simple_avp	, FT_DOUBLE			, BASE_DEC	, build_simple_avp },
/*	{"ipaddress"				, simple_avp	, simple_avp	, FT_NONE			, BASE_NONE , build_simple_avp },
	{"diameteruri"				, simple_avp	, simple_avp	, FT_STRING			, BASE_NONE , build_simple_avp },
	{"diameteridentity"			, simple_avp	, simple_avp	, FT_BYTES			, BASE_NONE , build_simple_avp },
	{"ipfilterrule"				, simple_avp	, simple_avp	, FT_BYTES			, BASE_NONE , build_simple_avp },
	{"qosfilterrule"			, simple_avp	, simple_avp	, FT_BYTES			, BASE_NONE , build_simple_avp },
	{"mipregistrationrequest"	, simple_avp	, simple_avp	, FT_BYTES			, BASE_NONE , build_simple_avp }, */
	{NULL, NULL, NULL, FT_NONE, BASE_NONE, NULL }
};

static guint strcase_hash(gconstpointer key) {
	char* k = ep_strdup(key);
	g_strdown(k);
	return g_str_hash(k);
}

static gboolean strcase_equal(gconstpointer ka, gconstpointer kb) {
	char* a = ep_strdup(ka);
	char* b = ep_strdup(kb);
	g_strdown(a);
	g_strdown(b);
	return g_str_equal(a,b);
}

extern int dictionary_load(void);
extern int dictionary_load(void) {
	ddict_t* d;
	ddict_application_t* p;
	ddict_vendor_t* v;
	ddict_cmd_t* c;
	ddict_typedefn_t* t;
	ddict_avp_t* a;
	char* dir = ep_strdup_printf("%s" G_DIR_SEPARATOR_S "diameter" G_DIR_SEPARATOR_S, get_datafile_dir());
	const avp_type_t* type;
	const avp_type_t* bytes = basic_types;
	diam_avp_t* avp;
	GHashTable* vendors = g_hash_table_new(strcase_hash,strcase_equal);
	diam_vnd_t* vnd;
	GArray* vnd_shrt_arr = g_array_new(TRUE,TRUE,sizeof(value_string));


	build_dict.hf = g_array_new(FALSE,TRUE,sizeof(hf_register_info));
	build_dict.ett = g_array_new(FALSE,TRUE,sizeof(gint*));
	build_dict.types = g_hash_table_new(strcase_hash,strcase_equal);
	build_dict.avps = g_hash_table_new(strcase_hash,strcase_equal);


	dictionary.vnds = pe_tree_create(EMEM_TREE_TYPE_RED_BLACK,"diameter_vnds");
	dictionary.avps = pe_tree_create(EMEM_TREE_TYPE_RED_BLACK,"diameter_avps");

	no_vnd.vs_cmds = g_array_new(TRUE,TRUE,sizeof(value_string));
	no_vnd.vs_avps = g_array_new(TRUE,TRUE,sizeof(value_string));

	pe_tree_insert32(dictionary.vnds,0,&no_vnd);
	g_hash_table_insert(vendors,"None",&no_vnd);

	/* initialize the types hash with the known basic types */
	for (type = basic_types; type->name; type++) {
		g_hash_table_insert(build_dict.types,type->name,(void*)type);
	}

	/* load the dictionary */
	d = ddict_scan(dir,"dictionary.xml");
        if (d == NULL) {
		return 0;
	}

	/* populate the types */
	for (t = d->typedefns; t; t = t->next) {
		const avp_type_t* parent = NULL;
		/* try to get the parent type */

		if (g_hash_table_lookup(build_dict.types,t->name))
			continue;

		if (t->parent) {
			parent = g_hash_table_lookup(build_dict.types,t->parent);
		}

		if (!parent) parent = bytes;

		/* insert the parent type for this type */
		g_hash_table_insert(build_dict.types,t->name,(void*)parent);
	}

	/* populate the applications */
	if ((p = d->applications)) {
		GArray* arr = g_array_new(TRUE,TRUE,sizeof(value_string));

		for (; p; p = p->next) {
			value_string item = {p->code,p->name};
			g_array_append_val(arr,item);
		}

		dictionary.applications = (void*)arr->data;
		g_array_free(arr,FALSE);
	}

	if ((v = d->vendors)) {
		for ( ; v; v = v->next) {
			value_string item = {v->code,v->name};

			if (g_hash_table_lookup(vendors,v->name))
				continue;

			g_array_append_val(vnd_shrt_arr,item);

			vnd = g_malloc(sizeof(diam_vnd_t));
			vnd->code = v->code;
			vnd->vs_cmds = g_array_new(TRUE,TRUE,sizeof(value_string));
			vnd->vs_avps = g_array_new(TRUE,TRUE,sizeof(value_string));
			pe_tree_insert32(dictionary.vnds,vnd->code,vnd);
			g_hash_table_insert(vendors,v->name,vnd);
		}
	}

	vnd_short_vs = (void*)vnd_shrt_arr->data;
	g_array_free(vnd_shrt_arr,FALSE);

	if ((c = d->cmds)) {
		for (; c; c = c->next) {
			if ((vnd = g_hash_table_lookup(vendors,c->vendor))) {
				value_string item = {c->code,c->name};
				g_array_append_val(vnd->vs_cmds,item);
			} else {
				g_warning("Diameter Dictionary: No Vendor: %s",c->vendor);
			}
		}
	}


	for (a = d->avps; a; a = a->next) {
		ddict_enum_t* e;
		value_string* vs = NULL;
		const char* vend = a->vendor ? a->vendor : "None";

		if ((vnd = g_hash_table_lookup(vendors,vend))) {
			value_string vndvs = {a->code,a->name};
			g_array_append_val(vnd->vs_avps,vndvs);
		} else {
			g_warning("Diameter Dictionary: No Vendor: %s",vend);
			vnd = &unknown_vendor;
		}

		if ((e = a->enums)) {
			GArray* arr = g_array_new(TRUE,TRUE,sizeof(value_string));

			for (; e; e = e->next) {
				value_string item = {e->code,e->name};
				g_array_append_val(arr,item);
			}
			vs = (void*)arr->data;
		}

		if (! a->type || ! ( type = g_hash_table_lookup(build_dict.types,a->type) ) ) {
			type = bytes;
		}

		avp = type->build( type, a->code, vnd->code, a->name, vs);
		g_hash_table_insert(build_dict.avps, a->name, avp);

		{
			emem_tree_key_t k[] = {
				{ 1, &(a->code) },
				{ 1, &(vnd->code) },
				{ 0 , NULL }
			};
			pe_tree_insert32_array(dictionary.avps,k,avp);
		}
	}

	g_hash_table_destroy(build_dict.types);
	g_hash_table_destroy(build_dict.avps);
	g_hash_table_destroy(vendors);

	return 1;
}


void
proto_reg_handoff_diameter(void)
{
	static int Initialized=FALSE;
	static int TcpPort=0;
	static int SctpPort=0;
	static dissector_handle_t diameter_tcp_handle;
	static dissector_handle_t diameter_handle;

	if (!Initialized) {
		diameter_tcp_handle = create_dissector_handle(dissect_diameter_tcp,
													  proto_diameter);
		diameter_handle = new_create_dissector_handle(dissect_diameter,
													  proto_diameter);
		Initialized=TRUE;
	} else {
		dissector_delete("tcp.port", TcpPort, diameter_tcp_handle);
		dissector_delete("sctp.port", SctpPort, diameter_handle);
	}

	/* set port for future deletes */
	TcpPort=gbl_diameterTcpPort;
	SctpPort=gbl_diameterSctpPort;

	/* g_warning ("Diameter: Adding tcp dissector to port %d",
		gbl_diameterTcpPort); */
	dissector_add("tcp.port", gbl_diameterTcpPort, diameter_tcp_handle);
	dissector_add("sctp.port", gbl_diameterSctpPort, diameter_handle);
}

/* registration with the filtering engine */
void
proto_register_diameter(void)
{
	module_t *diameter_module;
	int to_load_dict_before_hf = dictionary_load();
	hf_register_info hf_base[] = {
	{ &hf_diameter_version,
		  { "Version", "diameter.version", FT_UINT8, BASE_HEX, NULL, 0x00,
			  "", HFILL }},
	{ &hf_diameter_length,
		  { "Length","diameter.length", FT_UINT24, BASE_DEC, NULL, 0x0,
			  "", HFILL }},
	{ &hf_diameter_flags,
		  { "Flags", "diameter.flags", FT_UINT8, BASE_HEX, NULL, 0x0,
			  "", HFILL }},
	{ &hf_diameter_flags_request,
		  { "Request", "diameter.flags.request", FT_BOOLEAN, 8, TFS(&flags_set_truth), DIAM_FLAGS_R,
			  "", HFILL }},
	{ &hf_diameter_flags_proxyable,
		  { "Proxyable", "diameter.flags.proxyable", FT_BOOLEAN, 8, TFS(&flags_set_truth), DIAM_FLAGS_P,
			  "", HFILL }},
	{ &hf_diameter_flags_error,
		  { "Error","diameter.flags.error", FT_BOOLEAN, 8, TFS(&flags_set_truth), DIAM_FLAGS_E,
			  "", HFILL }},
	{ &hf_diameter_flags_T,
		  { "T(Potentially re-transmitted message)","diameter.flags.T", FT_BOOLEAN, 8, TFS(&flags_set_truth),DIAM_FLAGS_T,
			  "", HFILL }},
	{ &hf_diameter_flags_reserved4,
		  { "Reserved","diameter.flags.reserved4", FT_BOOLEAN, 8, TFS(&reserved_set),
			  DIAM_FLAGS_RESERVED4, "", HFILL }},
	{ &hf_diameter_flags_reserved5,
		  { "Reserved","diameter.flags.reserved5", FT_BOOLEAN, 8, TFS(&reserved_set),
			  DIAM_FLAGS_RESERVED5, "", HFILL }},
	{ &hf_diameter_flags_reserved6,
		  { "Reserved","diameter.flags.reserved6", FT_BOOLEAN, 8, TFS(&reserved_set),
			  DIAM_FLAGS_RESERVED6, "", HFILL }},
	{ &hf_diameter_flags_reserved7,
		  { "Reserved","diameter.flags.reserved7", FT_BOOLEAN, 8, TFS(&reserved_set),
			  DIAM_FLAGS_RESERVED7, "", HFILL }},
	{ &hf_diameter_vendor_id,
		  { "VendorId",	"diameter.vendorId", FT_UINT32, BASE_DEC, VALS(sminmpec_values),
			  0x0,"", HFILL }},
	{ &hf_diameter_application_id,
		  { "ApplicationId",	"diameter.applicationId", FT_UINT32, BASE_DEC, dictionary.applications,
			  0x0,"", HFILL }},
	{ &hf_diameter_hopbyhopid,
		  { "Hop-by-Hop Identifier", "diameter.hopbyhopid", FT_UINT32,
			  BASE_HEX, NULL, 0x0, "", HFILL }},
	{ &hf_diameter_endtoendid,
		  { "End-to-End Identifier", "diameter.endtoendid", FT_UINT32,
			  BASE_HEX, NULL, 0x0, "", HFILL }},
	{ &hf_diameter_avp,
		  { "AVP","diameter.avp", FT_BYTES, BASE_HEX,
			  NULL, 0x0, "", HFILL }},
	{ &hf_diameter_avp_len,
		  { "AVP Length","diameter.avp.len", FT_UINT24, BASE_DEC,
			  NULL, 0x0, "", HFILL }},
	{ &hf_diameter_avp_flags,
		  { "AVP Flags","diameter.avp.flags", FT_UINT8, BASE_HEX,
			  NULL, 0x0, "", HFILL }},
	{ &hf_diameter_avp_flags_vendor_specific,
		  { "Vendor-Specific", "diameter.flags.vendorspecific", FT_BOOLEAN, 8, TFS(&flags_set_truth), AVP_FLAGS_V,
			  "", HFILL }},
	{ &hf_diameter_avp_flags_mandatory,
		  { "Mandatory", "diameter.flags.mandatory", FT_BOOLEAN, 8, TFS(&flags_set_truth), AVP_FLAGS_M,
			  "", HFILL }},
	{ &hf_diameter_avp_flags_protected,
		  { "Protected","diameter.avp.flags.protected", FT_BOOLEAN, 8, TFS(&flags_set_truth), AVP_FLAGS_P,
			  "", HFILL }},
	{ &hf_diameter_avp_flags_reserved3,
		  { "Reserved","diameter.avp.flags.reserved3", FT_BOOLEAN, 8, TFS(&reserved_set),
			  AVP_FLAGS_RESERVED3,	"", HFILL }},
	{ &hf_diameter_avp_flags_reserved4,
		  { "Reserved","diameter.avp.flags.reserved4", FT_BOOLEAN, 8, TFS(&reserved_set),
			  AVP_FLAGS_RESERVED4,	"", HFILL }},
	{ &hf_diameter_avp_flags_reserved5,
		  { "Reserved","diameter.avp.flags.reserved5", FT_BOOLEAN, 8, TFS(&reserved_set),
			  AVP_FLAGS_RESERVED5,	"", HFILL }},
	{ &hf_diameter_avp_flags_reserved6,
		  { "Reserved","diameter.avp.flags.reserved6", FT_BOOLEAN, 8, TFS(&reserved_set),
			  AVP_FLAGS_RESERVED6,	"", HFILL }},
	{ &hf_diameter_avp_flags_reserved7,
		  { "Reserved","diameter.avp.flags.reserved7", FT_BOOLEAN, 8, TFS(&reserved_set),
			  AVP_FLAGS_RESERVED7,	"", HFILL }},
	{ &hf_diameter_avp_vendor_id,
		  { "AVP Vendor Id","diameter.avp.vendorId", FT_UINT32, BASE_DEC,
			  VALS(sminmpec_values), 0x0, "", HFILL }},
	{ &(unknown_avp.hf_value),
		  { "Value","diameter.avp.unknown", FT_BYTES, BASE_NONE,
			  NULL, 0x0, "", HFILL }},
	{ &hf_diameter_code,
		  { "Command Code", "diameter.cmd.code", FT_UINT32, BASE_DEC, NULL, 0, "", HFILL }},
	{ &hf_diameter_avp_code,
		  { "AVP Code", "diameter.avp.code", FT_UINT32, BASE_DEC, NULL, 0, "", HFILL }},
	};
	gint *ett_base[] = {
		&ett_diameter,
		&ett_diameter_flags,
		&ett_diameter_avp_flags,
		&ett_diameter_avpinfo,
		&(unknown_avp.ett)
	};


	g_array_append_vals(build_dict.hf, hf_base, array_length(hf_base));
	g_array_append_vals(build_dict.ett, ett_base, array_length(ett_base));

	proto_diameter = proto_register_protocol ("Diameter Protocol", "DIAMETER", "diameter");


	proto_register_field_array(proto_diameter, (hf_register_info*)build_dict.hf->data, build_dict.hf->len);
	proto_register_subtree_array((gint**)build_dict.ett->data, build_dict.ett->len);

	g_array_free(build_dict.hf,FALSE);
	g_array_free(build_dict.ett,TRUE);

	/* Allow dissector to find be found by name. */
	new_register_dissector("diameter", dissect_diameter, proto_diameter);

	/* Register a configuration option for port */
	diameter_module = prefs_register_protocol(proto_diameter,
											  proto_reg_handoff_diameter);

	prefs_register_obsolete_preference(diameter_module, "version");

	prefs_register_uint_preference(diameter_module, "tcp.port",
								   "Diameter TCP Port",
								   "Set the TCP port for Diameter messages",
								   10,
								   &gbl_diameterTcpPort);

	prefs_register_uint_preference(diameter_module, "sctp.port",
								   "Diameter SCTP Port",
								   "Set the SCTP port for Diameter messages",
								   10,
								   &gbl_diameterSctpPort);

	/* Desegmentation */
	prefs_register_bool_preference(diameter_module, "desegment",
                                   "Reassemble Diameter messages\nspanning multiple TCP segments",
								   "Whether the Diameter dissector should reassemble messages spanning multiple TCP segments."
								   " To use this option, you must also enable \"Allow subdissectors to reassemble TCP streams\" in the TCP protocol settings.",
								   &gbl_diameter_desegment);

	/* Register some preferences we no longer support, so we can report
	   them as obsolete rather than just illegal. */
	prefs_register_obsolete_preference(diameter_module, "udp.port");
	prefs_register_obsolete_preference(diameter_module, "command_in_header");
	prefs_register_obsolete_preference(diameter_module, "dictionary.name");
	prefs_register_obsolete_preference(diameter_module, "dictionary.use");
	prefs_register_obsolete_preference(diameter_module, "allow_zero_as_app_id");
	prefs_register_obsolete_preference(diameter_module, "suppress_console_output");

	to_load_dict_before_hf=0;
} /* proto_register_diameter */

