/* packet-tacacs.c
 * Routines for cisco tacacs/xtacacs/tacacs+ packet dissection
 * Copyright 2001, Paul Ionescu <paul@acorp.ro>
 * 
 * Full Tacacs+ parsing with decryption by
 *   Emanuele Caratti <wiz@iol.it>
 *
 * $Id: packet-tacacs.c,v 1.25 2003/09/20 09:41:48 guy Exp $
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@ethereal.com>
 * Copyright 1998 Gerald Combs
 *
 * Copied from old packet-tacacs.c
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


/* rfc-1492 for tacacs and xtacacs
 * draft-grant-tacacs-02.txt for tacacs+ (tacplus)
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>

#include <string.h>
#include <glib.h>
#include <epan/packet.h>

#include "prefs.h"
#include "crypt-md5.h"
#include "packet-tacacs.h"

static int md5_xor( u_char *data, char *key, int data_len, guint32 session_id, u_char version, u_char seq_no );

static int proto_tacacs = -1;
static int hf_tacacs_version = -1;
static int hf_tacacs_type = -1;
static int hf_tacacs_nonce = -1;
static int hf_tacacs_userlen = -1;
static int hf_tacacs_passlen = -1;
static int hf_tacacs_response = -1;
static int hf_tacacs_reason = -1;
static int hf_tacacs_result1 = -1;
static int hf_tacacs_destaddr = -1;
static int hf_tacacs_destport = -1;
static int hf_tacacs_line = -1;
static int hf_tacacs_result2 = -1;
static int hf_tacacs_result3 = -1;

static gint ett_tacacs = -1;

static char *tacplus_key;

#define VERSION_TACACS	0x00
#define VERSION_XTACACS	0x80

static const value_string tacacs_version_vals[] = {
	{ VERSION_TACACS,  "TACACS" },
	{ VERSION_XTACACS, "XTACACS" },
	{ 0,               NULL }
};

#define TACACS_LOGIN		1
#define TACACS_RESPONSE		2
#define TACACS_CHANGE		3
#define TACACS_FOLLOW		4
#define TACACS_CONNECT		5
#define TACACS_SUPERUSER	6
#define TACACS_LOGOUT		7
#define TACACS_RELOAD		8
#define TACACS_SLIP_ON		9
#define TACACS_SLIP_OFF		10
#define TACACS_SLIP_ADDR	11
static const value_string tacacs_type_vals[] = {
	{ TACACS_LOGIN,     "Login" },
	{ TACACS_RESPONSE,  "Response" },
	{ TACACS_CHANGE,    "Change" },
	{ TACACS_FOLLOW,    "Follow" },
	{ TACACS_CONNECT,   "Connect" },
	{ TACACS_SUPERUSER, "Superuser" },
	{ TACACS_LOGOUT,    "Logout" },
	{ TACACS_RELOAD,    "Reload" },
	{ TACACS_SLIP_ON,   "SLIP on" },
	{ TACACS_SLIP_OFF,  "SLIP off" },
	{ TACACS_SLIP_ADDR, "SLIP Addr" },
	{ 0,                NULL }};

static const value_string tacacs_reason_vals[] = {
	{ 0  , "none" },
	{ 1  , "expiring" },
	{ 2  , "password" },
	{ 3  , "denied" },
	{ 4  , "quit" },
	{ 5  , "idle" },
	{ 6  , "drop" },
	{ 7  , "bad" },
	{ 0  , NULL }
};

static const value_string tacacs_resp_vals[] = {
	{ 0  , "this is not a response" },
	{ 1  , "accepted" },
	{ 2  , "rejected" },
	{ 0  , NULL }
};

#define UDP_PORT_TACACS	49
#define TCP_PORT_TACACS	49

static void
dissect_tacacs(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	proto_tree      *tacacs_tree;
	proto_item      *ti;
	guint8		txt_buff[255+1],version,type,userlen,passlen;

	if (check_col(pinfo->cinfo, COL_PROTOCOL))
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "TACACS");
	if (check_col(pinfo->cinfo, COL_INFO))
		col_clear(pinfo->cinfo, COL_INFO);

	version = tvb_get_guint8(tvb,0);
	if (version != 0) {
		if (check_col(pinfo->cinfo, COL_PROTOCOL))
			col_set_str(pinfo->cinfo, COL_PROTOCOL, "XTACACS");
	}

	type = tvb_get_guint8(tvb,1);
	if (check_col(pinfo->cinfo, COL_INFO))
		col_add_str(pinfo->cinfo, COL_INFO,
		    val_to_str(type, tacacs_type_vals, "Unknown (0x%02x)"));

	if (tree)
	{
		ti = proto_tree_add_protocol_format(tree, proto_tacacs,
		 tvb, 0, -1, version==0?"TACACS":"XTACACS");
		tacacs_tree = proto_item_add_subtree(ti, ett_tacacs);

		proto_tree_add_uint(tacacs_tree, hf_tacacs_version, tvb, 0, 1,
		    version);
		proto_tree_add_uint(tacacs_tree, hf_tacacs_type, tvb, 1, 1,
		    type);
		proto_tree_add_item(tacacs_tree, hf_tacacs_nonce, tvb, 2, 2,
		    FALSE);

	if (version==0)
	    {
	    if (type!=TACACS_RESPONSE)
	    	{
	    	userlen=tvb_get_guint8(tvb,4);
		proto_tree_add_uint(tacacs_tree, hf_tacacs_userlen, tvb, 4, 1,
		    userlen);
	    	passlen=tvb_get_guint8(tvb,5);
		proto_tree_add_uint(tacacs_tree, hf_tacacs_passlen, tvb, 5, 1,
		    passlen);
		tvb_get_nstringz0(tvb,6,userlen+1,txt_buff);
		proto_tree_add_text(tacacs_tree, tvb, 6, userlen,         "Username: %s",txt_buff);
		tvb_get_nstringz0(tvb,6+userlen,passlen+1,txt_buff);
		proto_tree_add_text(tacacs_tree, tvb, 6+userlen, passlen, "Password: %s",txt_buff);
		}
	    else
	    	{
	    	proto_tree_add_item(tacacs_tree, hf_tacacs_response, tvb, 4, 1,
	    	    FALSE);
	    	proto_tree_add_item(tacacs_tree, hf_tacacs_reason, tvb, 5, 1,
	    	    FALSE);
		}
	    }
	else
	    {
	    userlen=tvb_get_guint8(tvb,4);
	    proto_tree_add_uint(tacacs_tree, hf_tacacs_userlen, tvb, 4, 1,
		userlen);
	    passlen=tvb_get_guint8(tvb,5);
	    proto_tree_add_uint(tacacs_tree, hf_tacacs_passlen, tvb, 5, 1,
		passlen);
	    proto_tree_add_item(tacacs_tree, hf_tacacs_response, tvb, 6, 1,
		FALSE);
	    proto_tree_add_item(tacacs_tree, hf_tacacs_reason, tvb, 7, 1,
		FALSE);
	    proto_tree_add_item(tacacs_tree, hf_tacacs_result1, tvb, 8, 4,
		FALSE);
	    proto_tree_add_item(tacacs_tree, hf_tacacs_destaddr, tvb, 12, 4,
		FALSE);
	    proto_tree_add_item(tacacs_tree, hf_tacacs_destport, tvb, 16, 2,
		FALSE);
	    proto_tree_add_item(tacacs_tree, hf_tacacs_line, tvb, 18, 2,
		FALSE);
	    proto_tree_add_item(tacacs_tree, hf_tacacs_result2, tvb, 20, 4,
		FALSE);
	    proto_tree_add_item(tacacs_tree, hf_tacacs_result3, tvb, 24, 2,
		FALSE);
	    if (type!=TACACS_RESPONSE)
	    	{
	    	tvb_get_nstringz0(tvb,26,userlen+1,txt_buff);
	    	proto_tree_add_text(tacacs_tree, tvb, 26, userlen,  "Username: %s",txt_buff);
	    	tvb_get_nstringz0(tvb,26+userlen,passlen+1,txt_buff);
	    	proto_tree_add_text(tacacs_tree, tvb, 26+userlen, passlen, "Password; %s",txt_buff);
	    	}
	    }
	}
}

void
proto_register_tacacs(void)
{
	static hf_register_info hf[] = {
	  { &hf_tacacs_version,
	    { "Version",           "tacacs.version",
	      FT_UINT8, BASE_HEX, VALS(tacacs_version_vals), 0x0,
	      "Version", HFILL }},
	  { &hf_tacacs_type,
	    { "Type",              "tacacs.type",
	      FT_UINT8, BASE_DEC, VALS(tacacs_type_vals), 0x0,
	      "Type", HFILL }},
	  { &hf_tacacs_nonce,
	    { "Nonce",             "tacacs.nonce",
	      FT_UINT16, BASE_HEX, NULL, 0x0,
	      "Nonce", HFILL }},
	  { &hf_tacacs_userlen,
	    { "Username length",   "tacacs.userlen",
	      FT_UINT8, BASE_DEC, NULL, 0x0,
	      "Username length", HFILL }},
	  { &hf_tacacs_passlen,
	    { "Password length",   "tacacs.passlen",
	      FT_UINT8, BASE_DEC, NULL, 0x0,
	      "Password length", HFILL }},
	  { &hf_tacacs_response,
	    { "Response",          "tacacs.response",
	      FT_UINT8, BASE_DEC, VALS(tacacs_resp_vals), 0x0,
	      "Response", HFILL }},
	  { &hf_tacacs_reason,
	    { "Reason",            "tacacs.reason",
	      FT_UINT8, BASE_DEC, VALS(tacacs_reason_vals), 0x0,
	      "Reason", HFILL }},
	  { &hf_tacacs_result1,
	    { "Result 1",          "tacacs.result1",
	      FT_UINT32, BASE_HEX, NULL, 0x0,
	      "Result 1", HFILL }},
	  { &hf_tacacs_destaddr,
	    { "Destination address", "tacacs.destaddr",
	      FT_IPv4, BASE_NONE, NULL, 0x0,
	      "Destination address", HFILL }},
	  { &hf_tacacs_destport,
	    { "Destination port",  "tacacs.destport",
	      FT_UINT16, BASE_DEC, NULL, 0x0,
	      "Destination port", HFILL }},
	  { &hf_tacacs_line,
	    { "Line",              "tacacs.line",
	      FT_UINT16, BASE_DEC, NULL, 0x0,
	      "Line", HFILL }},
	  { &hf_tacacs_result2,
	    { "Result 2",          "tacacs.result2",
	      FT_UINT32, BASE_HEX, NULL, 0x0,
	      "Result 2", HFILL }},
	  { &hf_tacacs_result3,
	    { "Result 3",          "tacacs.result3",
	      FT_UINT16, BASE_HEX, NULL, 0x0,
	      "Result 3", HFILL }},
	};

	static gint *ett[] = {
		&ett_tacacs,
	};
	proto_tacacs = proto_register_protocol("TACACS", "TACACS", "tacacs");
	proto_register_field_array(proto_tacacs, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));
}

void
proto_reg_handoff_tacacs(void)
{
	dissector_handle_t tacacs_handle;

	tacacs_handle = create_dissector_handle(dissect_tacacs, proto_tacacs);
	dissector_add("udp.port", UDP_PORT_TACACS, tacacs_handle);
}

static int proto_tacplus = -1;
static int hf_tacplus_response = -1;
static int hf_tacplus_request = -1;
static int hf_tacplus_majvers = -1;
static int hf_tacplus_minvers = -1;
static int hf_tacplus_type = -1;
static int hf_tacplus_seqno = -1;
static int hf_tacplus_flags = -1;
static int hf_tacplus_flags_payload_type = -1;
static int hf_tacplus_flags_connection_type = -1;
static int hf_tacplus_session_id = -1;
static int hf_tacplus_packet_len = -1;

static gint ett_tacplus = -1;
static gint ett_tacplus_body = -1;
static gint ett_tacplus_body_chap = -1;
static gint ett_tacplus_flags = -1;

#define FLAGS_UNENCRYPTED	0x01

static const true_false_string payload_type = {
  "Unencrypted",
  "Encrypted"
};

#define FLAGS_SINGLE		0x04

static const true_false_string connection_type = {
  "Single",
  "Multiple"
};

static gint 
tacplus_tvb_setup( tvbuff_t *tvb, tvbuff_t **dst_tvb, packet_info *pinfo, guint32 len, guint8 version )
{
	guint8	*buff;
	guint32 tmp_sess;
	
	buff=g_malloc(len);


	tvb_memcpy(tvb,(guint8*)&tmp_sess,4,4);
	tvb_memcpy(tvb, buff, TAC_PLUS_HDR_SIZE, len);

	md5_xor( buff, tacplus_key, len, tmp_sess,version, tvb_get_guint8(tvb,2) );

	*dst_tvb = tvb_new_real_data( buff, len, len );
	tvb_set_child_real_data_tvbuff( tvb, *dst_tvb );
	add_new_data_source(pinfo, *dst_tvb, "TACACS+ Decrypted");

	return 0;
}
static void
dissect_tacplus_args_list( tvbuff_t *tvb, proto_tree *tree, int data_off, int len_off, int arg_cnt )
{
	int i;
	guint8	buff[257];
	for(i=0;i<arg_cnt;i++){
		int len=tvb_get_guint8(tvb,len_off+i);
		proto_tree_add_text( tree, tvb, len_off+i, 1, "Arg(%d) length: %d", i, len );
		tvb_get_nstringz0(tvb, data_off, len+1, buff);
		proto_tree_add_text( tree, tvb, data_off, len, "Arg(%d) value: %s", i, buff );
		data_off+=len;
	}
}


static int
proto_tree_add_tacplus_common_fields( tvbuff_t *tvb, proto_tree *tree,  int offset, int var_off )
{
	int val;
	guint8 buff[257];
	/* priv_lvl */
	proto_tree_add_text( tree, tvb, offset, 1,
			"Privilege Level: %d", tvb_get_guint8(tvb,offset) );
	offset++;

	/* authen_type */
	val=tvb_get_guint8(tvb,offset);
	proto_tree_add_text( tree, tvb, offset, 1,
			"Authentication type: 0x%01x (%s)",
			val, val_to_str( val, tacplus_authen_type_vals, "Unknown Packet" ) );
	offset++;

	/* service */
	val=tvb_get_guint8(tvb,offset);
	proto_tree_add_text( tree, tvb, offset, 1,
			"Service: 0x%01x (%s)",
			val, val_to_str( val, tacplus_authen_service_vals, "Unknown Packet" ) );
	offset++;

	/* user_len && user */
	val=tvb_get_guint8(tvb,offset);
	proto_tree_add_text( tree, tvb, offset, 1, "User len: %d", val );
	if( val ){
		tvb_get_nstringz0(tvb, var_off, val+1, buff);
		proto_tree_add_text( tree, tvb, var_off, val, "User: %s", buff );
		var_off+=val;
	}
	offset++;


	/* port_len &&  port */
	val=tvb_get_guint8(tvb,offset);
	proto_tree_add_text( tree, tvb, offset, 1, "Port len: %d", val );
	if( val ){
		tvb_get_nstringz0(tvb, var_off, val+1, buff);
		proto_tree_add_text( tree, tvb, var_off, val, "Port: %s", buff );
		var_off+=val;
	}
	offset++;

	/* rem_addr_len && rem_addr */
	val=tvb_get_guint8(tvb,offset);
	proto_tree_add_text( tree, tvb, offset, 1, "Remaddr len: %d", val );
	if( val ){
		tvb_get_nstringz0(tvb, var_off, val+1, buff);
		proto_tree_add_text( tree, tvb, var_off, val, "Remote Address: %s", buff );
		var_off+=val;
	}
	return var_off;
}

static void
dissect_tacplus_body_authen_req_login( tvbuff_t* tvb, proto_tree *tree, int var_off )
{
	guint8 buff[257];
	guint8 val;
	val=tvb_get_guint8( tvb, AUTHEN_S_DATA_LEN_OFF );

	switch ( tvb_get_guint8(tvb, AUTHEN_S_AUTHEN_TYPE_OFF ) ) { /* authen_type */

		case TAC_PLUS_AUTHEN_TYPE_ASCII:
			proto_tree_add_text( tree, tvb, AUTHEN_S_DATA_LEN_OFF, 1, "Data: %d (not used)", val );
			if( val )
				proto_tree_add_text( tree, tvb, var_off, val, "Data" );
			break;

		case TAC_PLUS_AUTHEN_TYPE_PAP:
			proto_tree_add_text( tree, tvb, AUTHEN_S_DATA_LEN_OFF, 1, "Password Length %d", val );
			if( val ) {
				tvb_get_nstringz0( tvb, var_off, val+1, buff );
				proto_tree_add_text( tree, tvb, var_off, val, "Password: %s", buff );
			}
			break;

		case TAC_PLUS_AUTHEN_TYPE_CHAP:
			proto_tree_add_text( tree, tvb, AUTHEN_S_DATA_LEN_OFF, 1, "CHAP Data Length %d", val );
			if( val ) {
				proto_item	*pi;
				proto_tree  *pt;
				guint8 chal_len=val-(1+16); /* Response field alwayes 16 octets */
				pi = proto_tree_add_text(tree, tvb, var_off, val, "CHAP Data" );
				pt = proto_item_add_subtree( pi, ett_tacplus_body_chap );
				val= tvb_get_guint8( tvb, var_off );
				proto_tree_add_text( pt, tvb, var_off, 1, "ID: %d", val );
				var_off++;
				tvb_get_nstringz0( tvb, var_off, chal_len+1, buff );
				proto_tree_add_text( pt, tvb, var_off, chal_len, "Challenge: %s", buff );
				var_off+=chal_len;
				tvb_get_nstringz0( tvb, var_off, 16+1, buff );
				proto_tree_add_text( pt, tvb, var_off, 16 , "Response: %s", buff );
			}
			break;
		case TAC_PLUS_AUTHEN_TYPE_MSCHAP:
			proto_tree_add_text( tree, tvb, AUTHEN_S_DATA_LEN_OFF, 1, "MSCHAP Data Length %d", val );
			if( val ) {
				proto_item	*pi;
				proto_tree  *pt;
				guint8 chal_len=val-(1+49);  /* Response field alwayes 49 octets */
				pi = proto_tree_add_text(tree, tvb, var_off, val, "MSCHAP Data" );
				pt = proto_item_add_subtree( pi, ett_tacplus_body_chap );
				val= tvb_get_guint8( tvb, var_off );
				proto_tree_add_text( pt, tvb, var_off, 1, "ID: %d", val );
				var_off++;
				tvb_get_nstringz0( tvb, var_off, chal_len+1, buff );
				proto_tree_add_text( pt, tvb, var_off, chal_len, "Challenge: %s", buff );
				var_off+=chal_len;
				tvb_get_nstringz0( tvb, var_off, 49+1, buff );
				proto_tree_add_text( pt, tvb, var_off, 49 , "Response: %s", buff );
			}
			break;
		case TAC_PLUS_AUTHEN_TYPE_ARAP:
			proto_tree_add_text( tree, tvb, AUTHEN_S_DATA_LEN_OFF, 1, "ARAP Data Length %d", val );
			if( val ) {
				proto_item	*pi;
				proto_tree  *pt;
				pi = proto_tree_add_text(tree, tvb, var_off, val, "ARAP Data" );
				pt = proto_item_add_subtree( pi, ett_tacplus_body_chap );

				tvb_get_nstringz0( tvb, var_off, 8+1, buff );
				proto_tree_add_text( pt, tvb, var_off, 8, "Nas Challenge: %s", buff );
				var_off+=8;
				tvb_get_nstringz0( tvb, var_off, 8+1, buff );
				proto_tree_add_text( pt, tvb, var_off, 8, "Remote Challenge: %s", buff );
				var_off+=8;
				tvb_get_nstringz0( tvb, var_off, 8+1, buff );
				proto_tree_add_text( pt, tvb, var_off, 8, "Remote Response: %s", buff );
				var_off+=8;
			}
			break;

		default: /* Should not be reached */
			proto_tree_add_text( tree, tvb, AUTHEN_S_DATA_LEN_OFF, 1, "Data: %d", val );
			if( val ){
				proto_tree_add_text( tree, tvb, var_off, val, "Data" );
			}
	}
}

static void
dissect_tacplus_body_authen_req( tvbuff_t* tvb, proto_tree *tree )
{
	guint8 val;
	int var_off=AUTHEN_S_VARDATA_OFF;

	/* Action */
	val=tvb_get_guint8( tvb, AUTHEN_S_ACTION_OFF );
	proto_tree_add_text( tree, tvb,
			AUTHEN_S_ACTION_OFF, 1, 
			"Action: 0x%01x (%s)", val,
			val_to_str( val, tacplus_authen_action_vals, "Unknown Packet" ) );

	var_off=proto_tree_add_tacplus_common_fields( tvb, tree , AUTHEN_S_PRIV_LVL_OFF, AUTHEN_S_VARDATA_OFF );

	switch( val ) {
		case TAC_PLUS_AUTHEN_LOGIN:
			dissect_tacplus_body_authen_req_login( tvb, tree, var_off );
			break;
		case TAC_PLUS_AUTHEN_SENDAUTH:
			break;
	}
}

static void
dissect_tacplus_body_authen_req_cont( tvbuff_t *tvb, proto_tree *tree )
{
	int val;
	int var_off=AUTHEN_C_VARDATA_OFF;
	guint8 *buff=NULL;

	val=tvb_get_guint8( tvb, AUTHEN_C_FLAGS_OFF );
	proto_tree_add_text( tree, tvb,
			AUTHEN_R_FLAGS_OFF, 1, "Flags: 0x%02x %s",
			val,
			(val&TAC_PLUS_CONTINUE_FLAG_ABORT?"(Abort)":"") );


	val=tvb_get_ntohs( tvb, AUTHEN_C_USER_LEN_OFF ); 
	proto_tree_add_text( tree, tvb, AUTHEN_C_USER_LEN_OFF, 2 , "User length: %d", val );
	if( val ){
		buff=g_malloc(val+1);
		tvb_get_nstringz0( tvb, var_off, val+1, buff );
		proto_tree_add_text( tree, tvb, var_off, val, "User: %s", buff );
		var_off+=val;
		g_free(buff);
	}

	val=tvb_get_ntohs( tvb, AUTHEN_C_DATA_LEN_OFF ); 
	proto_tree_add_text( tree, tvb, AUTHEN_C_DATA_LEN_OFF, 2 ,
			"Data length: %d", val );
	if( val ){
		proto_tree_add_text( tree, tvb, var_off, val, "Data" );
	}

}

/* Server REPLY */
static void
dissect_tacplus_body_authen_rep( tvbuff_t *tvb, proto_tree *tree )
{
	int val;
	int var_off=AUTHEN_R_VARDATA_OFF;
	guint8 *buff=NULL;

	val=tvb_get_guint8( tvb, AUTHEN_R_STATUS_OFF );
	proto_tree_add_text(tree, tvb,
			AUTHEN_R_STATUS_OFF, 1, "Status: 0x%01x (%s)", val,
			val_to_str( val, tacplus_reply_status_vals, "Unknown Packet" )  );

	val=tvb_get_guint8( tvb, AUTHEN_R_FLAGS_OFF );
	proto_tree_add_text(tree, tvb,
			AUTHEN_R_FLAGS_OFF, 1, "Flags: 0x%02x %s",
			val, (val&TAC_PLUS_REPLY_FLAG_NOECHO?"(NoEcho)":"") );
	

	val=tvb_get_ntohs(tvb, AUTHEN_R_SRV_MSG_LEN_OFF );
	proto_tree_add_text( tree, tvb, AUTHEN_R_SRV_MSG_LEN_OFF, 2 ,
				"Server message length: %d", val );
	if( val ) {
		buff=g_malloc(val+1);
		tvb_get_nstringz0(tvb, var_off, val+1, buff);
		proto_tree_add_text(tree, tvb, var_off, val, "Server message: %s", buff );
		var_off+=val;
		g_free(buff);
	}

	val=tvb_get_ntohs(tvb, AUTHEN_R_DATA_LEN_OFF );
	proto_tree_add_text( tree, tvb, AUTHEN_R_DATA_LEN_OFF, 2 ,
				"Data length: %d", val );
	if( val ){
		proto_tree_add_text(tree, tvb, var_off, val, "Data" );
	}
}

static void
dissect_tacplus_body_author_req( tvbuff_t* tvb, proto_tree *tree )
{
	int val;
	int var_off;

	val=tvb_get_guint8( tvb, AUTHOR_Q_AUTH_METH_OFF ) ;
	proto_tree_add_text( tree, tvb, AUTHOR_Q_AUTH_METH_OFF, 1, 
			"Auth Method: 0x%01x (%s)", val,
				val_to_str( val, tacplus_authen_method, "Unknown Authen Method" ) );

	val=tvb_get_guint8( tvb, AUTHOR_Q_ARGC_OFF );
	var_off=proto_tree_add_tacplus_common_fields( tvb, tree ,
			AUTHOR_Q_PRIV_LVL_OFF,
			AUTHOR_Q_VARDATA_OFF + val );

	proto_tree_add_text( tree, tvb, AUTHOR_Q_ARGC_OFF, 1, "Arg count: %d", val );
	
/* var_off points after rem_addr */

	dissect_tacplus_args_list( tvb, tree, var_off, AUTHOR_Q_VARDATA_OFF, val );
}

static void
dissect_tacplus_body_author_rep( tvbuff_t* tvb, proto_tree *tree )
{
	int offset=AUTHOR_R_VARDATA_OFF;
	int val=tvb_get_guint8( tvb, AUTHOR_R_STATUS_OFF	) ;


	proto_tree_add_text( tree, tvb, AUTHOR_R_STATUS_OFF	, 1, 
			"Auth Status: 0x%01x (%s)", val,
			val_to_str( val, tacplus_author_status, "Unknown Authorization Status" ));

	val=tvb_get_ntohs( tvb, AUTHOR_R_SRV_MSG_LEN_OFF );
	offset+=val;
	proto_tree_add_text( tree, tvb, AUTHOR_R_SRV_MSG_LEN_OFF, 2, "Server Msg length: %d", val );

	val=tvb_get_ntohs( tvb, AUTHOR_R_DATA_LEN_OFF );
	offset+=val;
	proto_tree_add_text( tree, tvb, AUTHOR_R_DATA_LEN_OFF, 2, "Data length: %d", val );

	val=tvb_get_guint8( tvb, AUTHOR_R_ARGC_OFF);
	offset+=val;
	proto_tree_add_text( tree, tvb, AUTHOR_R_ARGC_OFF, 1, "Arg count: %d", val );

	dissect_tacplus_args_list( tvb, tree, offset, AUTHOR_R_VARDATA_OFF, val );
}

static void
dissect_tacplus_body_acct_req( tvbuff_t* tvb, proto_tree *tree )
{
	int val, var_off;

	val=tvb_get_guint8( tvb, ACCT_Q_FLAGS_OFF ); 
	proto_tree_add_text( tree, tvb, ACCT_Q_FLAGS_OFF, 1, "Flags: 0x%04x", val );

	val=tvb_get_guint8( tvb, ACCT_Q_METHOD_OFF );
	proto_tree_add_text( tree, tvb, ACCT_Q_METHOD_OFF, 1, 
			"Authen Method: 0x%04x (%s)",  
			val, val_to_str( val, tacplus_authen_type_vals, "Unknown Packet" ) );

	val=tvb_get_guint8( tvb, ACCT_Q_ARG_CNT_OFF );

	/* authen_type */
	var_off=proto_tree_add_tacplus_common_fields( tvb, tree ,
			ACCT_Q_PRIV_LVL_OFF,
			ACCT_Q_VARDATA_OFF+val
			);

	proto_tree_add_text( tree, tvb, ACCT_Q_ARG_CNT_OFF, 1,
			"Arg Cnt: %d", val  );

	dissect_tacplus_args_list( tvb, tree, var_off, ACCT_Q_VARDATA_OFF, val );


}

static void
dissect_tacplus_body_acct_rep( tvbuff_t* tvb, proto_tree *tree )
{
	int val, var_off=ACCT_Q_VARDATA_OFF;

	guint8 *buff=NULL;

	/* Status */
	val=tvb_get_guint8( tvb, ACCT_R_STATUS_OFF );
	proto_tree_add_text( tree, tvb, ACCT_R_STATUS_OFF, 1, "Status: 0x%02x (%s)", val,
				val_to_str( val, tacplus_acct_status, "Bogus status..") );

	/* Server Message */
	val=tvb_get_ntohs( tvb, ACCT_R_SRV_MSG_LEN_OFF );
	proto_tree_add_text( tree, tvb, ACCT_R_SRV_MSG_LEN_OFF, 2 ,
				"Server message length: %d", val );
	if( val ) {
		buff=(guint8*)g_malloc(val+1);
		tvb_get_nstringz0( tvb, var_off, val+1, buff );
		proto_tree_add_text( tree, tvb, var_off,
				val, "Server message: %s", buff );
		var_off+=val;
		g_free(buff);
	}

	/*  Data */
	val=tvb_get_ntohs( tvb, ACCT_R_DATA_LEN_OFF );
	proto_tree_add_text( tree, tvb, ACCT_R_DATA_LEN_OFF, 2 ,
				"Data length: %d", val );
	if( val ) {
		buff=(guint8*)g_malloc(val+1);
		tvb_get_nstringz0( tvb, var_off, val+1, buff );
		proto_tree_add_text( tree, tvb, var_off,
				val, "Data: %s", buff );
		g_free(buff);
	}
}



static void
dissect_tacplus_body(tvbuff_t * hdr_tvb, tvbuff_t * tvb, proto_tree * tree )
{
	int type = tvb_get_guint8( hdr_tvb, H_TYPE_OFF );
	int seq_no = tvb_get_guint8( hdr_tvb, H_SEQ_NO_OFF );

	switch (type) {
	  case TAC_PLUS_AUTHEN:
		if (  seq_no & 0x01) {
			if ( seq_no == 1 )
				dissect_tacplus_body_authen_req( tvb, tree );
			else
				dissect_tacplus_body_authen_req_cont( tvb, tree );
		} else {
			dissect_tacplus_body_authen_rep( tvb, tree );
		}
		return;
		break;
	  case TAC_PLUS_AUTHOR:
		if ( seq_no & 0x01)
			dissect_tacplus_body_author_req( tvb, tree );
		else 
			dissect_tacplus_body_author_rep( tvb, tree );
		return;
		break;
	  case TAC_PLUS_ACCT:
		if ( seq_no & 0x01)
			dissect_tacplus_body_acct_req( tvb, tree ); 
		else
			dissect_tacplus_body_acct_rep( tvb, tree );
		return;
		break;
	}
	proto_tree_add_text( tree, tvb, 0, tvb_length( tvb ), "Bogus..");
}

static void
dissect_tacplus(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	tvbuff_t	*new_tvb=NULL;
	proto_tree      *tacplus_tree;
	proto_item      *ti;
	guint8		version,flags;
	proto_tree      *flags_tree;
	proto_item      *tf;
	proto_item	*tmp_pi;
	guint32		len;
	gboolean	request=(pinfo->match_port == pinfo->destport);

	if (check_col(pinfo->cinfo, COL_PROTOCOL))
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "TACACS+");

	if (check_col(pinfo->cinfo, COL_INFO))
	{
		int type = tvb_get_guint8(tvb,1);
		col_add_fstr( pinfo->cinfo, COL_INFO, "%s: %s", 
				request ? "Q" : "R",
				val_to_str(type, tacplus_type_vals, "Unknown (0x%02x)"));
	}

	if (tree)
	{
		ti = proto_tree_add_protocol_format(tree, proto_tacplus,
		 tvb, 0, -1, "TACACS+");

		tacplus_tree = proto_item_add_subtree(ti, ett_tacplus);
		if (pinfo->match_port == pinfo->destport)
		{
			proto_tree_add_boolean_hidden(tacplus_tree,
			    hf_tacplus_request, tvb, 0, 0, TRUE);
		}
		else
		{
			proto_tree_add_boolean_hidden(tacplus_tree,
			    hf_tacplus_response, tvb, 0, 0, TRUE);
		}
		version = tvb_get_guint8(tvb,0);
		proto_tree_add_uint_format(tacplus_tree, hf_tacplus_majvers, tvb, 0, 1,
		    version,
		    "Major version: %s",
		    (version&0xf0)==0xc0?"TACACS+":"Unknown Version");
		proto_tree_add_uint(tacplus_tree, hf_tacplus_minvers, tvb, 0, 1,
		    version&0xf);
		proto_tree_add_item(tacplus_tree, hf_tacplus_type, tvb, 1, 1,
		    FALSE);
		proto_tree_add_item(tacplus_tree, hf_tacplus_seqno, tvb, 2, 1,
		    FALSE);
		flags = tvb_get_guint8(tvb,3);
		tf = proto_tree_add_uint_format(tacplus_tree, hf_tacplus_flags,
		    tvb, 3, 1, flags,
		    "Flags: %s, %s (0x%02x)",
		    (flags&FLAGS_UNENCRYPTED) ? "Unencrypted payload" :
						"Encrypted payload",
		    (flags&FLAGS_SINGLE) ? "Single connection" :
					   "Multiple Connections",
		    flags);
		flags_tree = proto_item_add_subtree(tf, ett_tacplus_flags);
		proto_tree_add_boolean(flags_tree, hf_tacplus_flags_payload_type,
		    tvb, 3, 1, flags);
		proto_tree_add_boolean(flags_tree, hf_tacplus_flags_connection_type,
		    tvb, 3, 1, flags);
		proto_tree_add_item(tacplus_tree, hf_tacplus_session_id, tvb, 4, 4,
		    FALSE);
		len = tvb_get_ntohl(tvb,8);
		proto_tree_add_uint(tacplus_tree, hf_tacplus_packet_len, tvb, 8, 4,
		    len);

		tmp_pi = proto_tree_add_text(tacplus_tree, tvb, TAC_PLUS_HDR_SIZE, len, "%s%s",
					((flags&FLAGS_UNENCRYPTED)?"":"Encrypted "), request?"Request":"Reply" );

		if( flags&FLAGS_UNENCRYPTED ) {
			new_tvb = tvb_new_subset( tvb, TAC_PLUS_HDR_SIZE, len, len );
		}  else {
			new_tvb=NULL;
			if( tacplus_key && *tacplus_key ){
				tacplus_tvb_setup(tvb,&new_tvb,pinfo,len,version);
			}
		}
		if( new_tvb ) {
			/* Check to see if I've a decrypted tacacs packet */
			if( !(flags&FLAGS_UNENCRYPTED) ){ 	
				tmp_pi = proto_tree_add_text(tacplus_tree, new_tvb, 0, len, "Decrypted %s",
							request?"Request":"Reply" );
			}
			dissect_tacplus_body( tvb, new_tvb, proto_item_add_subtree( tmp_pi, ett_tacplus_body ));
		}
	}
}

void
proto_register_tacplus(void)
{
	static hf_register_info hf[] = {
	  { &hf_tacplus_response,
	    { "Response",           "tacplus.response",
	      FT_BOOLEAN, BASE_NONE, NULL, 0x0,
	      "TRUE if TACACS+ response", HFILL }},
	  { &hf_tacplus_request,
	    { "Request",            "tacplus.request",
	      FT_BOOLEAN, BASE_NONE, NULL, 0x0,
	      "TRUE if TACACS+ request", HFILL }},
	  { &hf_tacplus_majvers,
	    { "Major version",      "tacplus.majvers",
	      FT_UINT8, BASE_DEC, NULL, 0x0,
	      "Major version number", HFILL }},
	  { &hf_tacplus_minvers,
	    { "Minor version",      "tacplus.minvers",
	      FT_UINT8, BASE_DEC, NULL, 0x0,
	      "Minor version number", HFILL }},
	  { &hf_tacplus_type,
	    { "Type",               "tacplus.type",
	      FT_UINT8, BASE_DEC, VALS(tacplus_type_vals), 0x0,
	      "Type", HFILL }},
	  { &hf_tacplus_seqno,
	    { "Sequence number",    "tacplus.seqno",
	      FT_UINT8, BASE_DEC, NULL, 0x0,
	      "Sequence number", HFILL }},
	  { &hf_tacplus_flags,
	    { "Flags",              "tacplus.flags",
	      FT_UINT8, BASE_HEX, NULL, 0x0,
	      "Flags", HFILL }},
	  { &hf_tacplus_flags_payload_type,
	    { "Payload type",       "tacplus.flags.payload_type",
	      FT_BOOLEAN, 8, TFS(&payload_type), FLAGS_UNENCRYPTED,
	      "Payload type (unencrypted or encrypted)", HFILL }},
	  { &hf_tacplus_flags_connection_type,
	    { "Connection type",    "tacplus.flags.connection_type",
	      FT_BOOLEAN, 8, TFS(&connection_type), FLAGS_SINGLE,
	      "Connection type (single or multiple)", HFILL }},
	  { &hf_tacplus_session_id,
	    { "Session ID",         "tacplus.session_id",
	      FT_UINT32, BASE_DEC, NULL, 0x0,
	      "Session ID", HFILL }},
	  { &hf_tacplus_packet_len,
	    { "Packet length",      "tacplus.packet_len",
	      FT_UINT32, BASE_DEC, NULL, 0x0,
	      "Packet length", HFILL }}
	};
	static gint *ett[] = {
		&ett_tacplus,
		&ett_tacplus_flags,
		&ett_tacplus_body,
		&ett_tacplus_body_chap, 
	};
	module_t *tacplus_module;

	proto_tacplus = proto_register_protocol("TACACS+", "TACACS+", "tacplus");
	proto_register_field_array(proto_tacplus, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));
	tacplus_module = prefs_register_protocol (proto_tacplus, NULL);
	prefs_register_string_preference(tacplus_module, "key",
	    "TACACS+ Encryption Key", "TACACS+ Encryption Key",
	    &tacplus_key);
}

void
proto_reg_handoff_tacplus(void)
{
	dissector_handle_t tacplus_handle;

	tacplus_handle = create_dissector_handle(dissect_tacplus,
	    proto_tacplus);
	dissector_add("tcp.port", TCP_PORT_TACACS, tacplus_handle);
}

#define MD5_LEN 16
	
static int
md5_xor( u_char *data, char *key, int data_len, guint32 session_id, u_char version, u_char seq_no )
{
	int i,j,md5_len;
	md5_byte_t *md5_buff;
	md5_byte_t hash[MD5_LEN];       				/* the md5 hash */
	md5_byte_t *mdp;
	md5_state_t mdcontext;

	md5_len = sizeof(session_id) + strlen(key)
			+ sizeof(version) + sizeof(seq_no);
	
	md5_buff = (md5_byte_t*)g_malloc(md5_len+MD5_LEN);

	mdp = md5_buff;
	bcopy(&session_id, mdp, sizeof(session_id));
	mdp += sizeof(session_id);
	bcopy(key, mdp, strlen(key));
	mdp += strlen(key);
	bcopy(&version, mdp, sizeof(version));
	mdp += sizeof(version);
	bcopy(&seq_no, mdp, sizeof(seq_no));
	mdp += sizeof(seq_no);

	md5_init(&mdcontext);
	md5_append(&mdcontext, md5_buff, md5_len);
	md5_finish(&mdcontext,hash);
	md5_len += MD5_LEN;
	for (i = 0; i < data_len; i += 16) {

		for (j = 0; j < 16; j++) {
			if ((i + j) >= data_len) 
				return 0;
			data[i + j] ^= hash[j];
		}
		bcopy( hash, mdp , MD5_LEN );
		md5_init(&mdcontext);
		md5_append(&mdcontext, md5_buff, md5_len);
		md5_finish(&mdcontext,hash);
	}
	g_free( md5_buff );
	return 0;
}
