/* packet-ymsg.c
 * Routines for Yahoo Messenger YMSG protocol packet version 9 dissection
 * Copyright 2003, Wayne Parrott <wayne_p@pacific.net.au>
 * Copied from packet-yhoo.c and updated
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>

#include <string.h>
#include <glib.h>
#include <epan/packet.h>

#include "packet-tcp.h"
#include "prefs.h"

static int proto_ymsg = -1;
static int hf_ymsg_version = -1;
static int hf_ymsg_len = -1;
static int hf_ymsg_service = -1;
static int hf_ymsg_status = -1;
static int hf_ymsg_session_id = -1;
static int hf_ymsg_content = -1;

static gint ett_ymsg = -1;
static gint ett_ymsg_content = -1;

#define TCP_PORT_YMSG	23	/* XXX - this is Telnet! */
#define TCP_PORT_YMSG_2	25	/* And this is SMTP! */
#define TCP_PORT_YMSG_3	5050	/* This, however, is regular Yahoo Messenger */

/* desegmentation of YMSG over TCP */
static gboolean ymsg_desegment = TRUE;

/*
 * This is from yahoolib2.c from libyahoo2.
 *
 * See also
 *
 *	http://libyahoo2.sourceforge.net/ymsg-9.txt
 *
 * and
 *
 *	http://www.venkydude.com/articles/yahoo.htm
 *
 * and
 *
 *	http://www.cse.iitb.ac.in/~varunk/YahooProtocol.htm
 *
 * and
 *
 *	http://www.geocrawler.com/archives/3/4893/2002/1/0/7459037/
 *
 * and
 *
 *	http://www.geocities.com/ziggycubbe/ym.html
 */

/* Service constants */
enum yahoo_service { /* these are easier to see in hex */
	YAHOO_SERVICE_LOGON = 1,
	YAHOO_SERVICE_LOGOFF,
	YAHOO_SERVICE_ISAWAY,
	YAHOO_SERVICE_ISBACK,
	YAHOO_SERVICE_IDLE, /* 5 (placemarker) */
	YAHOO_SERVICE_MESSAGE,
	YAHOO_SERVICE_IDACT,
	YAHOO_SERVICE_IDDEACT,
	YAHOO_SERVICE_MAILSTAT,
	YAHOO_SERVICE_USERSTAT, /* 0xa */
	YAHOO_SERVICE_NEWMAIL,
	YAHOO_SERVICE_CHATINVITE,
	YAHOO_SERVICE_CALENDAR,
	YAHOO_SERVICE_NEWPERSONALMAIL,
	YAHOO_SERVICE_NEWCONTACT,
	YAHOO_SERVICE_ADDIDENT, /* 0x10 */
	YAHOO_SERVICE_ADDIGNORE,
	YAHOO_SERVICE_PING,
	YAHOO_SERVICE_GOTGROUPRENAME, /* < 1, 36(old), 37(new) */
	YAHOO_SERVICE_SYSMESSAGE = 0x14,
	YAHOO_SERVICE_PASSTHROUGH2 = 0x16,
	YAHOO_SERVICE_CONFINVITE = 0x18,
	YAHOO_SERVICE_CONFLOGON,
	YAHOO_SERVICE_CONFDECLINE,
	YAHOO_SERVICE_CONFLOGOFF,
	YAHOO_SERVICE_CONFADDINVITE,
	YAHOO_SERVICE_CONFMSG,
	YAHOO_SERVICE_CHATLOGON,
	YAHOO_SERVICE_CHATLOGOFF,
	YAHOO_SERVICE_CHATMSG = 0x20,
	YAHOO_SERVICE_GAMELOGON = 0x28,
	YAHOO_SERVICE_GAMELOGOFF,
	YAHOO_SERVICE_GAMEMSG = 0x2a,
	YAHOO_SERVICE_FILETRANSFER = 0x46,
	YAHOO_SERVICE_VOICECHAT = 0x4A,
	YAHOO_SERVICE_NOTIFY,
	YAHOO_SERVICE_VERIFY,
	YAHOO_SERVICE_P2PFILEXFER,
	YAHOO_SERVICE_PEERTOPEER = 0x4F,        /* Checks if P2P possible */
	YAHOO_SERVICE_AUTHRESP = 0x54,
	YAHOO_SERVICE_LIST,
	YAHOO_SERVICE_AUTH = 0x57,
	YAHOO_SERVICE_ADDBUDDY = 0x83,
	YAHOO_SERVICE_REMBUDDY,
	YAHOO_SERVICE_IGNORECONTACT,    /* > 1, 7, 13 < 1, 66, 13, 0*/
	YAHOO_SERVICE_REJECTCONTACT,
	YAHOO_SERVICE_GROUPRENAME = 0x89, /* > 1, 65(new), 66(0), 67(old) */
	YAHOO_SERVICE_CHATONLINE = 0x96, /* > 109(id), 1, 6(abcde) < 0,1*/
	YAHOO_SERVICE_CHATGOTO,
	YAHOO_SERVICE_CHATJOIN, /* > 1 104-room 129-1600326591 62-2 */
	YAHOO_SERVICE_CHATLEAVE,
	YAHOO_SERVICE_CHATEXIT = 0x9b,
	YAHOO_SERVICE_CHATLOGOUT = 0xa0,
	YAHOO_SERVICE_CHATPING,
	YAHOO_SERVICE_COMMENT = 0xa8
};

/* Message flags */
enum yahoo_status {
        YAHOO_STATUS_AVAILABLE = 0,
        YAHOO_STATUS_BRB,
        YAHOO_STATUS_BUSY,
        YAHOO_STATUS_NOTATHOME,
        YAHOO_STATUS_NOTATDESK,
        YAHOO_STATUS_NOTINOFFICE,
        YAHOO_STATUS_ONPHONE,
        YAHOO_STATUS_ONVACATION,
        YAHOO_STATUS_OUTTOLUNCH,
        YAHOO_STATUS_STEPPEDOUT,
        YAHOO_STATUS_INVISIBLE = 12,
        YAHOO_STATUS_CUSTOM = 99,
        YAHOO_STATUS_IDLE = 999,
        YAHOO_STATUS_OFFLINE = 0x5a55aa56, /* don't ask */
        YAHOO_STATUS_TYPING = 0x16
};

struct yahoo_rawpacket
{
	char ymsg[4];			/* Packet identification string (YMSG) */
	unsigned char version[4];	/* 4 bytes, little endian? */
	unsigned char len[2];		/* length - little endian */
	unsigned char service[2];	/* service - little endian */
	unsigned char status[4];	/* Status - online, away etc.*/
	unsigned char session_id[4];	/* Session ID */
	char content[6];		/* 6 is the minimum size of the content */
};

static const value_string ymsg_service_vals[] = {
	{YAHOO_SERVICE_LOGON, "Pager Logon"},
	{YAHOO_SERVICE_LOGOFF, "Pager Logoff"},
	{YAHOO_SERVICE_ISAWAY, "Is Away"},
	{YAHOO_SERVICE_ISBACK, "Is Back"},
	{YAHOO_SERVICE_IDLE, "Idle"},
	{YAHOO_SERVICE_MESSAGE, "Message"},
	{YAHOO_SERVICE_IDACT, "Activate Identity"},
	{YAHOO_SERVICE_IDDEACT, "Deactivate Identity"},
	{YAHOO_SERVICE_MAILSTAT, "Mail Status"},
	{YAHOO_SERVICE_USERSTAT, "User Status"},
	{YAHOO_SERVICE_NEWMAIL, "New Mail"},
	{YAHOO_SERVICE_CHATINVITE, "Chat Invitation"},
	{YAHOO_SERVICE_CALENDAR, "Calendar Reminder"},
	{YAHOO_SERVICE_NEWPERSONALMAIL, "New Personals Mail"},
	{YAHOO_SERVICE_NEWCONTACT, "New Friend"},
	{YAHOO_SERVICE_ADDIDENT, "Add Identity"},
	{YAHOO_SERVICE_ADDIGNORE, "Add Ignore"},
	{YAHOO_SERVICE_PING, "Ping"},
	{YAHOO_SERVICE_GOTGROUPRENAME, "YAHOO_SERVICE_GOTGROUPRENAME"},
	{YAHOO_SERVICE_SYSMESSAGE, "System Message"},
	{YAHOO_SERVICE_PASSTHROUGH2, "Passthrough 2"},
	{YAHOO_SERVICE_CONFINVITE, "Conference Invitation"},
	{YAHOO_SERVICE_CONFLOGON, "Conference Logon"},
	{YAHOO_SERVICE_CONFDECLINE, "Conference Decline"},
	{YAHOO_SERVICE_CONFLOGOFF, "Conference Logoff"},
	{YAHOO_SERVICE_CONFADDINVITE, "Conference Additional Invitation"},
	{YAHOO_SERVICE_CONFMSG, "Conference Message"},
	{YAHOO_SERVICE_CHATLOGON, "Chat Logon"},
	{YAHOO_SERVICE_CHATLOGOFF, "Chat Logoff"},
	{YAHOO_SERVICE_CHATMSG, "Chat Message"},
	{YAHOO_SERVICE_GAMELOGON, "Game Logon"},
	{YAHOO_SERVICE_GAMELOGOFF, "Game Logoff"},
	{YAHOO_SERVICE_GAMEMSG, "Game Message"},
	{YAHOO_SERVICE_FILETRANSFER, "File Transfer"},
	{YAHOO_SERVICE_VOICECHAT, "Voice Chat"},
	{YAHOO_SERVICE_NOTIFY, "YAHOO_SERVICE_NOTIFY"},
	{YAHOO_SERVICE_VERIFY, "YAHOO_SERVICE_VERIFY"},
	{YAHOO_SERVICE_P2PFILEXFER, "YAHOO_SERVICE_P2PFILEXFER"}, 
	{YAHOO_SERVICE_PEERTOPEER, "YAHOO_SERVICE_PEERTOPEER"},
	{YAHOO_SERVICE_AUTHRESP, "YAHOO_SERVICE_AUTHRESP"},
	{YAHOO_SERVICE_LIST, "YAHOO_SERVICE_LIST"},
	{YAHOO_SERVICE_AUTH, "YAHOO_SERVICE_AUTH"},
	{YAHOO_SERVICE_ADDBUDDY, "YAHOO_SERVICE_ADDBUDDY"},
	{YAHOO_SERVICE_REMBUDDY, "YAHOO_SERVICE_REMBUDDY"},
	{YAHOO_SERVICE_IGNORECONTACT, "YAHOO_SERVICE_IGNORECONTACT"},
	{YAHOO_SERVICE_REJECTCONTACT, "YAHOO_SERVICE_REJECTCONTACT"},
	{YAHOO_SERVICE_GROUPRENAME, "Group Renamed"},
	{YAHOO_SERVICE_CHATONLINE, "YAHOO_SERVICE_CHATONLINE"},
	{YAHOO_SERVICE_CHATGOTO, "YAHOO_SERVICE_CHATGOTO"},
	{YAHOO_SERVICE_CHATJOIN, "YAHOO_SERVICE_CHATJOIN"},
	{YAHOO_SERVICE_CHATLEAVE, "YAHOO_SERVICE_CHATLEAVE"},
	{YAHOO_SERVICE_CHATEXIT, "YAHOO_SERVICE_CHATEXIT"},
	{YAHOO_SERVICE_CHATLOGOUT, "YAHOO_SERVICE_CHATLOGOUT"},
	{YAHOO_SERVICE_CHATPING, "YAHOO_SERVICE_CHATPING"},
	{YAHOO_SERVICE_COMMENT, "YAHOO_SERVICE_COMMENT"},
	{0, NULL}
};

static const value_string ymsg_status_vals[] = {
	{YAHOO_STATUS_AVAILABLE, "YAHOO_STATUS_AVAILABLE"},
	{YAHOO_STATUS_BRB, "YAHOO_STATUS_BRB"},
	{YAHOO_STATUS_BUSY, "YAHOO_STATUS_BUSY"},
	{YAHOO_STATUS_NOTATHOME, "YAHOO_STATUS_NOTATHOME"},
	{YAHOO_STATUS_NOTATDESK, "YAHOO_STATUS_NOTATDESK"},
	{YAHOO_STATUS_NOTINOFFICE, "YAHOO_STATUS_NOTINOFFICE"},
	{YAHOO_STATUS_ONPHONE, "YAHOO_STATUS_ONPHONE"},
	{YAHOO_STATUS_ONVACATION, "YAHOO_STATUS_ONVACATION"},
	{YAHOO_STATUS_OUTTOLUNCH, "YAHOO_STATUS_OUTTOLUNCH"},
	{YAHOO_STATUS_STEPPEDOUT, "YAHOO_STATUS_STEPPEDOUT"},
	{YAHOO_STATUS_INVISIBLE, "YAHOO_STATUS_INVISIBLE"},
	{YAHOO_STATUS_CUSTOM, "YAHOO_STATUS_CUSTOM"},
	{YAHOO_STATUS_IDLE, "YAHOO_STATUS_IDLE"},
	{YAHOO_STATUS_OFFLINE, "YAHOO_STATUS_OFFLINE"},
	{YAHOO_STATUS_TYPING, "YAHOO_STATUS_TYPING"},
	{0, NULL}
};

static guint get_ymsg_pdu_len(tvbuff_t *tvb, int offset);
static void dissect_ymsg_pdu(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);

int
get_content_item_length(tvbuff_t *tvb, int offset) {
	int origoffset = offset;
	guint16 curdata;
	for (;;) {
		curdata = tvb_get_ntohs(tvb, offset);
		if (curdata == 0xc080) {
			break;
		}
		offset++;
	}
	return offset - origoffset;
}


static gboolean
dissect_ymsg(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{

  if (tvb_memeql(tvb, 0, "YMSG", 4) == -1) {
    /* Not a Yahoo Messenger packet. */
    return FALSE;
  }
  
  tcp_dissect_pdus(tvb, pinfo, tree, ymsg_desegment, 8, get_ymsg_pdu_len,
		   dissect_ymsg_pdu);
  return TRUE;
}

static guint
get_ymsg_pdu_len(tvbuff_t *tvb, int offset)
{
  guint16 plen;

  /*
   * Get the length of the YMSG packet.
   */
  plen = tvb_get_ntohs(tvb, offset + 8);

  /*
   * That length doesn't include the length of the header itself; add that in.
   */
  return plen + 20;
}

static void
dissect_ymsg_pdu(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	proto_tree      *ymsg_tree, *ti;
	proto_item      *content_item;
	proto_tree      *content_tree;
	char *keybuf;
	char *valbuf;
	int headersize = sizeof(struct yahoo_rawpacket)-6;
	int keylen = 0;
	int vallen = 0;
	int offset = 0;
	int content_len = 0;

	if (check_col(pinfo->cinfo, COL_PROTOCOL))
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "YMSG");

	offset = 0;
	if (check_col(pinfo->cinfo, COL_INFO)) {
		col_add_fstr(pinfo->cinfo, COL_INFO,
			"%s, %s",
			val_to_str(tvb_get_ntohs(tvb, offset + 10),
				 ymsg_service_vals, "Unknown Service: %u"),
			val_to_str(tvb_get_ntohl(tvb, offset + 12),
				 ymsg_status_vals, "Unknown Status: %u")
		);
	}

	if (tree) {
		ti = proto_tree_add_item(tree, proto_ymsg, tvb, offset, -1,
				FALSE);
		ymsg_tree = proto_item_add_subtree(ti, ett_ymsg);

		offset += 4; /* skip the YMSG string */

		proto_tree_add_item(ymsg_tree, hf_ymsg_version, tvb,
			offset, 2, FALSE);
		offset += 2;

		offset += 2;	/* XXX - padding? */

		content_len = tvb_get_ntohs(tvb, offset);
		proto_tree_add_item(ymsg_tree, hf_ymsg_len, tvb,
			offset, 2, FALSE);
		offset += 2;

		proto_tree_add_item(ymsg_tree, hf_ymsg_service, tvb,
			offset, 2, FALSE);
		offset += 2;

		proto_tree_add_item(ymsg_tree, hf_ymsg_status, tvb,
			offset, 4, FALSE);
		offset += 4;

		proto_tree_add_item(ymsg_tree, hf_ymsg_session_id, tvb,
			offset, 4, TRUE);
		offset += 4;

		content_item = proto_tree_add_item(ymsg_tree, hf_ymsg_content, tvb,
				offset, -1, TRUE);
		content_tree = proto_item_add_subtree(content_item, ett_ymsg_content);

		for (;;) {
			if (offset >= headersize+content_len) {
				break;
			}
			keylen = get_content_item_length (tvb, offset);
			keybuf = tvb_format_text(tvb, offset, keylen);
			vallen = get_content_item_length (tvb, offset+keylen+2);
			content_item = proto_tree_add_text(content_tree, tvb, offset, keylen+vallen+4, "%s: ", keybuf);
			valbuf = tvb_format_text(tvb, offset+keylen+2, vallen);
			proto_item_append_text(content_item, "%s", valbuf);
			offset += keylen+vallen+4;
		}
	}

	return;
}

void
proto_register_ymsg(void)
{
	static hf_register_info hf[] = {
			{ &hf_ymsg_version, {
				"Version", "ymsg.version", FT_UINT16, BASE_DEC,
				NULL, 0, "Packet version identifier", HFILL }},
			{ &hf_ymsg_len, {
				"Packet Length", "ymsg.len", FT_UINT16, BASE_DEC,
				NULL, 0, "Packet Length", HFILL }},
			{ &hf_ymsg_service, {
				"Service", "ymsg.service", FT_UINT16, BASE_DEC,
				VALS(ymsg_service_vals), 0, "Service Type", HFILL }},
			{ &hf_ymsg_status, {
				"Status", "ymsg.status", FT_UINT32, BASE_DEC,
				VALS(ymsg_status_vals), 0, "Message Type Flags", HFILL }},
			{ &hf_ymsg_session_id, {
				"Session ID", "ymsg.session_id", FT_UINT32, BASE_HEX,
				NULL, 0, "Connection ID", HFILL }},
			{ &hf_ymsg_content, {
				"Content", "ymsg.content", FT_STRING, 0,
				NULL, 0, "Data portion of the packet", HFILL }},
        };
	static gint *ett[] = {
		&ett_ymsg,
		&ett_ymsg_content,
	};
	module_t *ymsg_module;

	proto_ymsg = proto_register_protocol("Yahoo YMSG Messenger Protocol",
	    "YMSG", "ymsg");

	proto_register_field_array(proto_ymsg, hf, array_length(hf));

	proto_register_subtree_array(ett, array_length(ett));

	ymsg_module = prefs_register_protocol(proto_ymsg, NULL);
	prefs_register_bool_preference(ymsg_module, "desegment",
				       "Reasssemble YMSG messages spanning multiple TCP segments",
				       "Whether the YMSG dissector should reasssemble messages spanning multiple TCP segments. "
				       "To use this option, you must also enable \"Allow subdissectors to reassemble TCP streams\" in the TCP protocol settings.",
				       &ymsg_desegment);
}

void
proto_reg_handoff_ymsg(void)
{
	/*
	 * DO NOT register for port 23, as that's Telnet, or for port
	 * 25, as that's SMTP.
	 *
	 * Also, DO NOT register for port 5050, as that's used by the
	 * old and new Yahoo messenger protocols.
	 *
	 * Just register as a heuristic TCP dissector, and reject stuff
	 * that doesn't begin with a YMSG signature.
	 */
	heur_dissector_add("tcp", dissect_ymsg, proto_ymsg);
}
