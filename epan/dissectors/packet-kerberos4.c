/* packet-kerberos4.c
 * Routines for Kerberos v4 packet dissection
 *
 * Ronnie Sahlberg 2004
 *
 * $Id: packet-kerberos4.c 11410 2004-07-18 18:06:47Z gram $
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
 * PDU structure based on the document :
 *                                                              Section E.2.1
 *
 *                            Kerberos Authentication and Authorization System
 *
 *            by S. P. Miller, B. C. Neuman, J. I. Schiller, and J. H. Saltzer
 *
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glib.h>
#include <epan/packet.h>
#include <epan/conversation.h>

static int proto_krb4 = -1;
static int hf_krb4_version = -1;
static int hf_krb4_auth_msg_type = -1;
static int hf_krb4_m_type = -1;
static int hf_krb4_byte_order = -1;
static int hf_krb4_name = -1;
static int hf_krb4_instance = -1;
static int hf_krb4_realm = -1;
static int hf_krb4_time_sec = -1;
static int hf_krb4_exp_date = -1;
static int hf_krb4_lifetime = -1;
static int hf_krb4_s_name = -1;
static int hf_krb4_s_instance = -1;
static int hf_krb4_kvno = -1;
static int hf_krb4_length = -1;
static int hf_krb4_encrypted_blob = -1;
static int hf_krb4_unknown_transarc_blob = -1;

static gint ett_krb4 = -1;
static gint ett_krb4_auth_msg_type = -1;

#define UDP_PORT_KRB4    750
#define TRANSARC_SPECIAL_VERSION 0x63

static dissector_handle_t krb4_handle;


static const value_string byte_order_vals[] = {
	{ 0,	"Big Endian" },
	{ 1,	"Little Endian" },
	{ 0,	NULL }
};

#define AUTH_MSG_KDC_REQUEST		1
#define AUTH_MSG_KDC_REPLY		2
#define AUTH_MSG_APPL_REQUEST		3
#define AUTH_MSG_APPL_REQUEST_MUTUAL	4
#define AUTH_MSG_ERR_REPLY		5
#define AUTH_MSG_PRIVATE		6
#define AUTH_MSG_SAFE			7
#define AUTH_MSG_APPL_ERR		8
#define AUTH_MSG_DIE			63
static const value_string m_type_vals[] = {
	{ AUTH_MSG_KDC_REQUEST,		"KDC Request" },
	{ AUTH_MSG_KDC_REPLY,		"KDC Reply" },
	{ AUTH_MSG_APPL_REQUEST,	"Appl Request" },
	{ AUTH_MSG_APPL_REQUEST_MUTUAL,	"Appl Request Mutual" },
	{ AUTH_MSG_ERR_REPLY,		"Err Reply" },
	{ AUTH_MSG_PRIVATE,		"Private" },
	{ AUTH_MSG_SAFE,		"Safe" },
	{ AUTH_MSG_APPL_ERR,		"Appl Err" },
	{ AUTH_MSG_DIE,			"Die" },
	{ 0,	NULL }
};


static int
dissect_krb4_string(packet_info *pinfo _U_, int hf_index, proto_tree *tree, tvbuff_t *tvb, int offset)
{
	proto_tree_add_item(tree, hf_index, tvb, offset, -1, FALSE);
	while(tvb_get_guint8(tvb, offset)!=0){
		offset++;
	}
	offset++;

	return offset;
}

static int
dissect_krb4_kdc_request(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset, gboolean little_endian, int version)
{
	nstime_t time_sec;
	guint8 lifetime;

	if(version==TRANSARC_SPECIAL_VERSION){
		proto_tree_add_item(tree, hf_krb4_unknown_transarc_blob, tvb, offset, 8, FALSE);
		offset+=8;
	}

	/* Name */
	offset=dissect_krb4_string(pinfo, hf_krb4_name, tree, tvb, offset);

	/* Instance */
	offset=dissect_krb4_string(pinfo, hf_krb4_instance, tree, tvb, offset);

	/* Realm */
	offset=dissect_krb4_string(pinfo, hf_krb4_realm, tree, tvb, offset);

	/* Time sec */
	time_sec.secs=little_endian?tvb_get_letohl(tvb, offset):tvb_get_ntohl(tvb, offset);
	time_sec.nsecs=0;
	proto_tree_add_time(tree, hf_krb4_time_sec, tvb, offset, 4, &time_sec);
	offset+=4;

	/* lifetime */
	lifetime=tvb_get_guint8(tvb, offset);
	proto_tree_add_uint_format(tree, hf_krb4_lifetime, tvb, offset, 1, lifetime, "Lifetime: %d (%d minutes)", lifetime, lifetime*5);
	offset++;

	/* service Name */
	offset=dissect_krb4_string(pinfo, hf_krb4_s_name, tree, tvb, offset);

	/* service Instance */
	offset=dissect_krb4_string(pinfo, hf_krb4_s_instance, tree, tvb, offset);

	return offset;
}


static int
dissect_krb4_kdc_reply(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset, gboolean little_endian)
{
	nstime_t time_sec;
	guint32 length;

	/* Name */
	offset=dissect_krb4_string(pinfo, hf_krb4_name, tree, tvb, offset);

	/* Instance */
	offset=dissect_krb4_string(pinfo, hf_krb4_instance, tree, tvb, offset);

	/* Realm */
	offset=dissect_krb4_string(pinfo, hf_krb4_realm, tree, tvb, offset);

	/* Time sec */
	time_sec.secs=little_endian?tvb_get_letohl(tvb, offset):tvb_get_ntohl(tvb, offset);
	time_sec.nsecs=0;
	proto_tree_add_time(tree, hf_krb4_time_sec, tvb, offset, 4, &time_sec);
	offset+=4;

	/*XXX unknown byte here */
	offset++;

	/* exp date */
	time_sec.secs=little_endian?tvb_get_letohl(tvb, offset):tvb_get_ntohl(tvb, offset);
	time_sec.nsecs=0;
	proto_tree_add_time(tree, hf_krb4_exp_date, tvb, offset, 4, &time_sec);
	offset+=4;

	/* kvno */
	proto_tree_add_item(tree, hf_krb4_kvno, tvb, offset, 1, FALSE);
	offset++;

	/* length2 */
	length=little_endian?tvb_get_letohs(tvb, offset):tvb_get_ntohs(tvb, offset);
	proto_tree_add_uint_format(tree, hf_krb4_length, tvb, offset, 2, length, "Length: %d", length);
	offset+=2;

	/* encrypted blob */
	proto_tree_add_item(tree, hf_krb4_encrypted_blob, tvb, offset, length, FALSE);
	offset+=length;

	return offset;
}


static int
dissect_krb4_appl_request(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset, gboolean little_endian _U_)
{

	/* kvno */
	proto_tree_add_item(tree, hf_krb4_kvno, tvb, offset, 1, FALSE);
	offset++;

	/* Realm */
	offset=dissect_krb4_string(pinfo, hf_krb4_realm, tree, tvb, offset);

/*qqq doesnt make sense beyond this field*/
	return offset;
}



static int
dissect_krb4_auth_msg_type(packet_info *pinfo, proto_tree *parent_tree, tvbuff_t *tvb, int offset, int version)
{
	proto_tree *tree;
	proto_item *item;
	guint8 auth_msg_type;

	auth_msg_type=tvb_get_guint8(tvb, offset);
	item = proto_tree_add_item(parent_tree, hf_krb4_auth_msg_type, tvb, offset, 1, FALSE);
	tree = proto_item_add_subtree(item, ett_krb4_auth_msg_type);

	/* m_type */
	proto_tree_add_item(tree, hf_krb4_m_type, tvb, offset, 1, FALSE);
	if (check_col(pinfo->cinfo, COL_INFO))
	  col_append_fstr(pinfo->cinfo, COL_INFO, "%s%s",
	   (version==TRANSARC_SPECIAL_VERSION)?"TRANSARC-":"", 
	    val_to_str(auth_msg_type>>1, m_type_vals, "Unknown (0x%04x)"));
	proto_item_append_text(item, " %s%s", 
	   (version==TRANSARC_SPECIAL_VERSION)?"TRANSARC-":"", 
	   val_to_str(auth_msg_type>>1, m_type_vals, "Unknown (0x%04x)"));

	/* byte order */
	proto_tree_add_item(tree, hf_krb4_byte_order, tvb, offset, 1, FALSE);
	proto_item_append_text(item, " (%s)", val_to_str(auth_msg_type&0x01, byte_order_vals, "Unknown (0x%04x)"));

	offset++;
	return offset;
}

static void
dissect_krb4(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree)
{
	proto_tree *tree;
	proto_item *item;
	guint8 version, opcode;
	int offset=0;
	
	/* this should better have the value 4 or it might be a weirdo
	 * Transarc AFS special unknown thing.
	 */
	version=tvb_get_guint8(tvb, offset);
	if((version!=4)&&(version!=TRANSARC_SPECIAL_VERSION)){ 
		return;
	}

	/* create a tree for krb4 */
	item = proto_tree_add_item(parent_tree, proto_krb4, tvb, offset, -1, FALSE);
	tree = proto_item_add_subtree(item, ett_krb4);
	
	if (check_col(pinfo->cinfo, COL_PROTOCOL))
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "KRB4");
	if (check_col(pinfo->cinfo, COL_INFO))
		col_clear(pinfo->cinfo, COL_INFO);

	/* version */
	proto_tree_add_item(tree, hf_krb4_version, tvb, offset, 1, FALSE);
	offset++;

	/* auth_msg_type */
	opcode=tvb_get_guint8(tvb, offset);
	offset = dissect_krb4_auth_msg_type(pinfo, tree, tvb, offset, version);

	switch(opcode>>1){
	case AUTH_MSG_KDC_REQUEST:
		offset = dissect_krb4_kdc_request(pinfo, tree, tvb, offset, opcode&0x01, version);
		break;
	case AUTH_MSG_KDC_REPLY:
		offset = dissect_krb4_kdc_reply(pinfo, tree, tvb, offset, opcode&0x01);
		break;
	case AUTH_MSG_APPL_REQUEST:
		offset = dissect_krb4_appl_request(pinfo, tree, tvb, offset, opcode&0x01);
		break;
	case AUTH_MSG_APPL_REQUEST_MUTUAL:
	case AUTH_MSG_ERR_REPLY:
	case AUTH_MSG_PRIVATE:
	case AUTH_MSG_SAFE:
	case AUTH_MSG_APPL_ERR:
	case AUTH_MSG_DIE:
		break;
	}

}

void
proto_register_krb4(void)
{
  static hf_register_info hf[] = {
    { &hf_krb4_version,
      { "Version", "krb4.version",
	FT_UINT8, BASE_DEC, NULL, 0x0,
      	"Kerberos(v4) version number", HFILL }},
    { &hf_krb4_auth_msg_type,
      { "Msg Type", "krb4.auth_msg_type",
        FT_UINT8, BASE_HEX, NULL, 0x0,
        "Message Type/Byte Order", HFILL }},
    { &hf_krb4_m_type,
      { "M Type", "krb4.m_type",
        FT_UINT8, BASE_HEX, VALS(m_type_vals), 0xfe,
        "Message Type", HFILL }},
    { &hf_krb4_byte_order,
      { "Byte Order", "krb4.byte_order",
        FT_UINT8, BASE_HEX, VALS(byte_order_vals), 0x01,
        "Byte Order", HFILL }},
    { &hf_krb4_name,
      { "Name", "krb4.name",
        FT_STRINGZ, BASE_NONE, NULL, 0x00,
        "Name", HFILL }},
    { &hf_krb4_instance,
      { "Instance", "krb4.instance",
        FT_STRINGZ, BASE_NONE, NULL, 0x00,
        "Instance", HFILL }},
    { &hf_krb4_realm,
      { "Realm", "krb4.realm",
        FT_STRINGZ, BASE_NONE, NULL, 0x00,
        "Realm", HFILL }},
    { &hf_krb4_time_sec,
      { "Time Sec", "krb4.time_sec",
        FT_ABSOLUTE_TIME, BASE_NONE, NULL, 0x00,
        "Time Sec", HFILL }},
    { &hf_krb4_exp_date,
      { "Exp Date", "krb4.exp_date",
        FT_ABSOLUTE_TIME, BASE_NONE, NULL, 0x00,
        "Exp Date", HFILL }},
    { &hf_krb4_lifetime,
      { "Lifetime", "krb4.lifetime",
        FT_UINT8, BASE_DEC, NULL, 0x00,
        "Lifetime (in 5 min units)", HFILL }},
    { &hf_krb4_s_name,
      { "Service Name", "krb4.s_name",
        FT_STRINGZ, BASE_NONE, NULL, 0x00,
        "Service Name", HFILL }},
    { &hf_krb4_s_instance,
      { "Service Instance", "krb4.s_instance",
        FT_STRINGZ, BASE_NONE, NULL, 0x00,
        "Service Instance", HFILL }},
    { &hf_krb4_kvno,
      { "Kvno", "krb4.kvno",
        FT_UINT8, BASE_DEC, NULL, 0x00,
        "Key Version No", HFILL }},
    { &hf_krb4_length,
      { "Length", "krb4.length",
        FT_UINT32, BASE_DEC, NULL, 0x00,
        "Length of encrypted blob", HFILL }},
    { &hf_krb4_encrypted_blob,
      { "Encrypted Blob", "krb4.encrypted_blob",
        FT_BYTES, BASE_HEX, NULL, 0x00,
        "Encrypted blob", HFILL }},
    { &hf_krb4_unknown_transarc_blob,
      { "Unknown Transarc Blob", "krb4.unknown_transarc_blob",
        FT_BYTES, BASE_HEX, NULL, 0x00,
        "Unknown blob only present in Transarc packets", HFILL }},
  };
  static gint *ett[] = {
    &ett_krb4,
    &ett_krb4_auth_msg_type,
  };

  proto_krb4 = proto_register_protocol("Kerberos v4",
				       "KRB4", "krb4");
  proto_register_field_array(proto_krb4, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));

  krb4_handle = create_dissector_handle(dissect_krb4, proto_krb4);
}

void
proto_reg_handoff_krb4(void)
{
  dissector_add("udp.port", UDP_PORT_KRB4, krb4_handle);
}
