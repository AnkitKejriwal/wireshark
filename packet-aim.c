/* packet-aim.c
 * Routines for AIM Instant Messenger (OSCAR) dissection
 * Copyright 2000, Ralf Hoelzer <ralf@well.com>
 * Copyright 2004, Jelmer Vernooij <jelmer@samba.org>
 *
 * $Id: packet-aim.c,v 1.35 2004/03/23 07:23:43 guy Exp $
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
#include <ctype.h>

#include <glib.h>

#include <epan/packet.h>
#include <epan/strutil.h>

#include "packet-tcp.h"
#include "packet-aim.h"
#include "prefs.h"

#define TCP_PORT_AIM 5190

#define STRIP_TAGS 1

/* channels */
#define CHANNEL_NEW_CONN    0x01
#define CHANNEL_SNAC_DATA   0x02
#define CHANNEL_FLAP_ERR    0x03
#define CHANNEL_CLOSE_CONN  0x04
#define CHANNEL_KEEP_ALIVE  0x05

#define CLI_COOKIE		    0x01

#define FAMILY_ALL_ERROR_INVALID_HEADER				   0x0001
#define FAMILY_ALL_ERROR_SERVER_RATE_LIMIT_EXCEEDED    0x0002
#define FAMILY_ALL_ERROR_CLIENT_RATE_LIMIT_EXCEEDED    0x0003
#define FAMILY_ALL_ERROR_RECIPIENT_NOT_LOGGED_IN       0x0004
#define FAMILY_ALL_ERROR_REQUESTED_SERVICE_UNAVAILABLE 0x0005
#define FAMILY_ALL_ERROR_REQUESTED_SERVICE_NOT_DEFINED 0x0006
#define FAMILY_ALL_ERROR_OBSOLETE_SNAC				   0x0007
#define FAMILY_ALL_ERROR_NOT_SUPPORTED_BY_SERVER	   0x0008
#define FAMILY_ALL_ERROR_NOT_SUPPORTED_BY_CLIENT	   0x0009
#define FAMILY_ALL_ERROR_REFUSED_BY_CLIENT             0x000a
#define FAMILY_ALL_ERROR_REPLY_TOO_BIG                 0x000b
#define FAMILY_ALL_ERROR_RESPONSES_LOST                0x000c
#define FAMILY_ALL_ERROR_REQUEST_DENIED                0x000d
#define FAMILY_ALL_ERROR_INCORRECT_SNAC_FORMAT         0x000e
#define FAMILY_ALL_ERROR_INSUFFICIENT_RIGHTS           0x000f
#define FAMILY_ALL_ERROR_RECIPIENT_BLOCKED             0x0010
#define FAMILY_ALL_ERROR_SENDER_TOO_EVIL               0x0011
#define FAMILY_ALL_ERROR_RECEIVER_TOO_EVIL             0x0012
#define FAMILY_ALL_ERROR_USER_TEMP_UNAVAILABLE         0x0013
#define FAMILY_ALL_ERROR_NO_MATCH                      0x0014
#define FAMILY_ALL_ERROR_LIST_OVERFLOW                 0x0015
#define FAMILY_ALL_ERROR_REQUEST_AMBIGUOUS             0x0016
#define FAMILY_ALL_ERROR_SERVER_QUEUE_FULL             0x0017
#define FAMILY_ALL_ERROR_NOT_WHILE_ON_AOL              0x0018

static const value_string aim_flap_channels[] = {
    { CHANNEL_NEW_CONN, "New Connection" },
    { CHANNEL_SNAC_DATA, "SNAC Data" },
    { CHANNEL_FLAP_ERR, "FLAP-Level Error" },
    { CHANNEL_CLOSE_CONN, "Close Connection" },
    { CHANNEL_KEEP_ALIVE, "Keep Alive" },
    { 0, NULL }
};

static const value_string aim_snac_errors[] = {
  { FAMILY_ALL_ERROR_INVALID_HEADER, "Invalid SNAC Header" },
  { FAMILY_ALL_ERROR_SERVER_RATE_LIMIT_EXCEEDED, "Server rate limit exceeded" },
  { FAMILY_ALL_ERROR_CLIENT_RATE_LIMIT_EXCEEDED, "Client rate limit exceeded" },
  { FAMILY_ALL_ERROR_RECIPIENT_NOT_LOGGED_IN, "Recipient not logged in" },
  { FAMILY_ALL_ERROR_REQUESTED_SERVICE_UNAVAILABLE, "Requested service unavailable" },
  { FAMILY_ALL_ERROR_REQUESTED_SERVICE_NOT_DEFINED, "Requested service not defined" },
  { FAMILY_ALL_ERROR_OBSOLETE_SNAC, "Obsolete SNAC issued" },
  { FAMILY_ALL_ERROR_NOT_SUPPORTED_BY_SERVER, "Not supported by server" },
  { FAMILY_ALL_ERROR_NOT_SUPPORTED_BY_CLIENT, "Not supported by client" },
  { FAMILY_ALL_ERROR_REFUSED_BY_CLIENT, "Refused by client" },
  { FAMILY_ALL_ERROR_REPLY_TOO_BIG, "Reply too big" },
  { FAMILY_ALL_ERROR_RESPONSES_LOST, "Responses lost" },
  { FAMILY_ALL_ERROR_REQUEST_DENIED, "Request denied" },
  { FAMILY_ALL_ERROR_INCORRECT_SNAC_FORMAT, "Incorrect SNAC format" },
  { FAMILY_ALL_ERROR_INSUFFICIENT_RIGHTS, "Insufficient rights" },
  { FAMILY_ALL_ERROR_RECIPIENT_BLOCKED, "Recipient blocked" },
  { FAMILY_ALL_ERROR_SENDER_TOO_EVIL, "Sender too evil" },
  { FAMILY_ALL_ERROR_RECEIVER_TOO_EVIL, "Receiver too evil" },
  { FAMILY_ALL_ERROR_USER_TEMP_UNAVAILABLE, "User temporarily unavailable" },
  { FAMILY_ALL_ERROR_NO_MATCH, "No match" },
  { FAMILY_ALL_ERROR_LIST_OVERFLOW, "List overflow" },
  { FAMILY_ALL_ERROR_REQUEST_AMBIGUOUS, "Request ambiguous" },
  { FAMILY_ALL_ERROR_SERVER_QUEUE_FULL, "Server queue full" },
  { FAMILY_ALL_ERROR_NOT_WHILE_ON_AOL, "Not while on AOL" },
  { 0, NULL }
};

#define AIM_TLV_SCREEN_NAME				0x0001
#define AIM_TLV_ROASTED_PASSWORD        0x0002
#define AIM_TLV_CLIENT_ID_STRING       	0x0003
#define AIM_TLV_ERRORURL                0x0004
#define AIM_TLV_BOS_SERVER_STRING       0x0005
#define AIM_TLV_AUTH_COOKIE             0x0006
#define AIM_TLV_ERRORCODE			    0x0008
#define AIM_TLV_GENERIC_SERVICE_ID      0x000d
#define AIM_TLV_CLIENT_COUNTRY          0x000e
#define AIM_TLV_CLIENT_LANGUAGE         0x000f
#define AIM_TLV_EMAILADDR			    0x0011
#define AIM_TLV_REGSTATUS			    0x0013
#define AIM_TLV_CLIENT_DISTRIBUTION_NUM 0x0014
#define AIM_TLV_CLIENT_ID               0x0016
#define AIM_TLV_CLIENT_MAJOR_VERSION    0x0017
#define AIM_TLV_CLIENT_MINOR_VERSION    0x0018
#define AIM_TLV_CLIENT_LESSER_VERSION   0x0019
#define AIM_TLV_CLIENT_BUILD_NUMBER     0x001a
#define AIM_TLV_PASSWORD				0x0025
#define AIM_TLV_LATESTBETABUILD     	0x0040
#define AIM_TLV_LATESTBETAURL       	0x0041
#define AIM_TLV_LATESTBETAINFO      	0x0042
#define AIM_TLV_LATESTBETANAME      	0x0043
#define AIM_TLV_LATESTRELEASEBUILD  	0x0044
#define AIM_TLV_LATESTRELEASEURL    	0x0045
#define AIM_TLV_LATESTRELEASEINFO   	0x0046
#define AIM_TLV_LATESTRELEASENAME   	0x0047
#define AIM_TLV_CLIENTUSESSI   			0x004a

const aim_tlv global_tlvs[] = {
  {  AIM_TLV_SCREEN_NAME, "Screen name", FT_STRING },
  {  AIM_TLV_ROASTED_PASSWORD, "Roasted password array", FT_BYTES },
  {  AIM_TLV_CLIENT_ID_STRING, "Client id string (name, version)", FT_STRING },
  {  AIM_TLV_CLIENT_ID, "Client id number", FT_UINT16 },
  {  AIM_TLV_CLIENT_MAJOR_VERSION, "Client major version", FT_UINT16 },
  {  AIM_TLV_CLIENT_MINOR_VERSION, "Client minor version", FT_UINT16 },
  {  AIM_TLV_CLIENT_LESSER_VERSION, "Client lesser version", FT_UINT16 },
  {  AIM_TLV_CLIENT_BUILD_NUMBER, "Client build number", FT_UINT16 },
  {  AIM_TLV_CLIENT_DISTRIBUTION_NUM, "Client distribution number", FT_UINT16 },
  {  AIM_TLV_CLIENT_LANGUAGE, "Client language", FT_STRING },
  {  AIM_TLV_CLIENT_COUNTRY, "Client country", FT_STRING },
  {  AIM_TLV_BOS_SERVER_STRING, "BOS server string", FT_STRING },
  {  AIM_TLV_AUTH_COOKIE, "Authorization cookie", FT_BYTES },
  {  AIM_TLV_ERRORURL, "Error URL", FT_STRING },
  {  AIM_TLV_ERRORCODE, "Error Code", FT_UINT16 },
  {  AIM_TLV_EMAILADDR, "Account Email address", FT_STRING },
  {  AIM_TLV_REGSTATUS, "Registration Status", FT_UINT16 },
  {  AIM_TLV_LATESTBETABUILD, "Latest Beta Build", FT_UINT32 },
  {  AIM_TLV_LATESTBETAURL, "Latest Beta URL", FT_STRING },
  {  AIM_TLV_LATESTBETAINFO, "Latest Beta Info", FT_STRING },
  {  AIM_TLV_LATESTBETANAME, "Latest Beta Name", FT_STRING },
  {  AIM_TLV_LATESTRELEASEBUILD, "Latest Release Build", FT_UINT32 },
  {  AIM_TLV_LATESTRELEASEURL, "Latest Release URL", FT_STRING },
  {  AIM_TLV_LATESTRELEASEINFO, "Latest Release Info", FT_STRING },
  {  AIM_TLV_LATESTRELEASENAME, "Latest Release Name", FT_STRING },
  {  AIM_TLV_CLIENTUSESSI, "Use SSI", FT_UINT8 },
  {  AIM_TLV_GENERIC_SERVICE_ID, "Service (Family) ID", FT_UINT16 },
  { 0, "Unknown", 0 },
};

                                                                                                                              
#define FAMILY_BUDDYLIST_USERFLAGS      0x0001
#define FAMILY_BUDDYLIST_MEMBERSINCE    0x0002
#define FAMILY_BUDDYLIST_ONSINCE        0x0003
#define FAMILY_BUDDYLIST_IDLETIME       0x0004
#define FAMILY_BUDDYLIST_ICQSTATUS      0x0006
#define FAMILY_BUDDYLIST_ICQIPADDR      0x000a
#define FAMILY_BUDDYLIST_ICQSTUFF       0x000c
#define FAMILY_BUDDYLIST_CAPINFO        0x000d
#define FAMILY_BUDDYLIST_UNKNOWN        0x000e
#define FAMILY_BUDDYLIST_SESSIONLEN     0x000f
#define FAMILY_BUDDYLIST_ICQSESSIONLEN  0x0010
                                                                                                                              
static const aim_tlv buddylist_tlvs[] = {
  { FAMILY_BUDDYLIST_USERFLAGS, "User flags", FT_UINT16 },
  { FAMILY_BUDDYLIST_MEMBERSINCE, "Member since date", FT_UINT32 },
  { FAMILY_BUDDYLIST_ONSINCE, "Online since", FT_UINT32 },
  { FAMILY_BUDDYLIST_IDLETIME, "Idle time (sec)", FT_UINT16 },
  { FAMILY_BUDDYLIST_ICQSTATUS, "ICQ Online status", FT_UINT16 },
  { FAMILY_BUDDYLIST_ICQIPADDR, "ICQ User IP Address", FT_IPv4 },
  { FAMILY_BUDDYLIST_ICQSTUFF, "ICQ Info", FT_BYTES },
  { FAMILY_BUDDYLIST_CAPINFO, "Capability Info", FT_BYTES },
  { FAMILY_BUDDYLIST_UNKNOWN, "Unknown", FT_UINT16 },
  { FAMILY_BUDDYLIST_SESSIONLEN, "Session Length (sec)", FT_UINT32 },
  { FAMILY_BUDDYLIST_SESSIONLEN, "ICQ Session Length (sec)", FT_UINT32 },
  { 0, "Unknown", 0 }
};

struct aim_family {
	guint16 family;
	const char *name;
	const value_string *subtypes;
};

static GList *families = NULL;

#define FAMILY_GENERIC_MOTD_MOTD					   0x000B

static const aim_tlv aim_fnac_family_generic_motd_tlv[] = {
  { FAMILY_GENERIC_MOTD_MOTD, "Message of the day message", FT_STRING },
  { 0, "Unknown", 0 }
};

#define FAMILY_GENERIC_REDIRECT_SERVER_ADDRESS		   0x0005
#define FAMILY_GENERIC_REDIRECT_AUTH_COOKIE			   0x0006
#define FAMILY_GENERIC_REDIRECT_FAMILY_ID              0x000D

static const aim_tlv aim_fnac_family_generic_redirect_tlv[] = {
  { FAMILY_GENERIC_REDIRECT_SERVER_ADDRESS, "Server address and (optional) port", FT_STRING },
  { FAMILY_GENERIC_REDIRECT_AUTH_COOKIE, "Authorization cookie", FT_STRING },
  { FAMILY_GENERIC_REDIRECT_FAMILY_ID, "Family ID", FT_UINT16 },
  { 0, "Unknown", 0 }
};

#define FAMILY_GENERIC_MOTD_MOTDTYPE_MDT_UPGRADE       0x0001
#define FAMILY_GENERIC_MOTD_MOTDTYPE_ADV_UPGRADE       0x0002
#define FAMILY_GENERIC_MOTD_MOTDTYPE_SYS_BULLETIN      0x0003
#define FAMILY_GENERIC_MOTD_MOTDTYPE_NORMAL            0x0004
#define FAMILY_GENERIC_MOTD_MOTDTYPE_NEWS              0x0006

static const value_string aim_snac_generic_motd_motdtypes[] = {
  { FAMILY_GENERIC_MOTD_MOTDTYPE_MDT_UPGRADE, "Mandatory Upgrade Needed Notice" },
  { FAMILY_GENERIC_MOTD_MOTDTYPE_ADV_UPGRADE, "Advisable Upgrade Notice" },
  { FAMILY_GENERIC_MOTD_MOTDTYPE_SYS_BULLETIN, "AIM/ICQ Service System Announcements" },
  { FAMILY_GENERIC_MOTD_MOTDTYPE_NORMAL, "Standard Notice" },
  { FAMILY_GENERIC_MOTD_MOTDTYPE_NEWS, "News from AOL service" },
  { 0, NULL }
};

static int dissect_aim(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);
static guint get_aim_pdu_len(tvbuff_t *tvb, int offset);
static void dissect_aim_pdu(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);

static void dissect_aim_newconn(tvbuff_t *tvb, packet_info *pinfo, int offset, proto_tree *tree);
static void dissect_aim_snac(tvbuff_t *tvb, packet_info *pinfo, 
			     int offset, proto_tree *tree, proto_tree *root_tree);
static void dissect_aim_flap_err(tvbuff_t *tvb, packet_info *pinfo, 
				 int offset, proto_tree *tree);
static void dissect_aim_keep_alive(tvbuff_t *tvb, packet_info *pinfo, 
				   int offset, proto_tree *tree);
static void dissect_aim_close_conn(tvbuff_t *tvb, packet_info *pinfo, 
				   int offset, proto_tree *tree);
static void dissect_aim_unknown_channel(tvbuff_t *tvb, packet_info *pinfo, 
					int offset, proto_tree *tree);

static dissector_table_t subdissector_table;

/* Initialize the protocol and registered fields */
static int proto_aim = -1;
static int hf_aim_cmd_start = -1;
static int hf_aim_channel = -1;
static int hf_aim_seqno = -1;
static int hf_aim_data = -1;
static int hf_aim_data_len = -1;
static int hf_aim_signon_challenge_len = -1;
static int hf_aim_signon_challenge = -1;
static int hf_aim_fnac_family = -1;
static int hf_aim_fnac_subtype = -1;
static int hf_aim_fnac_flags = -1;
static int hf_aim_fnac_id = -1;
static int hf_aim_infotype = -1;
static int hf_aim_buddyname_len = -1;
static int hf_aim_buddyname = -1;
static int hf_aim_userinfo_warninglevel = -1;
static int hf_aim_snac_error = -1;
static int hf_aim_userinfo_tlvcount = -1;
static int hf_aim_authcookie = -1;

/* Initialize the subtree pointers */
static gint ett_aim          = -1;
static gint ett_aim_fnac     = -1;
static gint ett_aim_tlv      = -1;

/* desegmentation of AIM over TCP */
static gboolean aim_desegment = TRUE;

/* Code to actually dissect the packets */
static int dissect_aim(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
/* check, if this is really an AIM packet, they start with 0x2a */
/* XXX - I've seen some stuff starting with 0x5a followed by 0x2a */

  if(tvb_bytes_exist(tvb, 0, 1) && tvb_get_guint8(tvb, 0) != 0x2a) {
    /* Not an instant messenger packet, just happened to use the same port */
    /* XXX - if desegmentation disabled, this might be a continuation
       packet, not a non-AIM packet */
    return 0;
  }

  tcp_dissect_pdus(tvb, pinfo, tree, aim_desegment, 6, get_aim_pdu_len,
	dissect_aim_pdu);
  return tvb_length(tvb);
}

static guint get_aim_pdu_len(tvbuff_t *tvb, int offset)
{
  guint16 plen;

  /*
   * Get the length of the AIM packet.
   */
  plen = tvb_get_ntohs(tvb, offset + 4);

  /*
   * That length doesn't include the length of the header itself; add that in.
   */
  return plen + 6;
}

static void dissect_aim_pdu(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
  /* Header fields */
  unsigned char  hdr_channel;           /* channel ID */
  unsigned short hdr_sequence_no;       /* Internal frame sequence number, not needed */
  unsigned short hdr_data_field_length; /* length of data within frame */

  int offset=0;

/* Set up structures we will need to add the protocol subtree and manage it */
  proto_item *ti;
  proto_tree *aim_tree = NULL;

/* Make entries in Protocol column and Info column on summary display */
  if (check_col(pinfo->cinfo, COL_PROTOCOL))
    col_set_str(pinfo->cinfo, COL_PROTOCOL, "AIM");

  if (check_col(pinfo->cinfo, COL_INFO))
    col_add_str(pinfo->cinfo, COL_INFO, "AOL Instant Messenger");

  /* get relevant header information */
  offset += 1;          /* XXX - put the identifier into the tree? */	
  hdr_channel           = tvb_get_guint8(tvb, offset);
  offset += 1;
  hdr_sequence_no       = tvb_get_ntohs(tvb, offset);
  offset += 2;
  hdr_data_field_length = tvb_get_ntohs(tvb, offset);
  offset += 2;

/* In the interest of speed, if "tree" is NULL, don't do any work not
   necessary to generate protocol tree items. */
  if (tree) {
    ti = proto_tree_add_item(tree, proto_aim, tvb, 0, -1, FALSE);
    aim_tree = proto_item_add_subtree(ti, ett_aim);
    proto_tree_add_uint(aim_tree, hf_aim_cmd_start, tvb, 0, 1, '*');
    proto_tree_add_item(aim_tree, hf_aim_channel, tvb, 1, 1, FALSE);
    proto_tree_add_uint(aim_tree, hf_aim_seqno, tvb, 2, 2, hdr_sequence_no);
    proto_tree_add_uint(aim_tree, hf_aim_data_len, tvb, 4, 2, hdr_data_field_length);

  }

  switch(hdr_channel)
  {
    case CHANNEL_NEW_CONN:
      dissect_aim_newconn(tvb, pinfo, offset, aim_tree);
      break;
    case CHANNEL_SNAC_DATA:
      dissect_aim_snac(tvb, pinfo, offset, aim_tree, tree);
      break;
    case CHANNEL_FLAP_ERR:
      dissect_aim_flap_err(tvb, pinfo, offset, aim_tree);
      break;
    case CHANNEL_CLOSE_CONN:
      dissect_aim_close_conn(tvb, pinfo, offset, aim_tree);
      break;
    case CHANNEL_KEEP_ALIVE:
     dissect_aim_keep_alive(tvb, pinfo, offset, aim_tree);
     break;
    default:
      dissect_aim_unknown_channel(tvb, pinfo, offset, aim_tree);
      break;
  }

}

const char *aim_get_subtypename( guint16 famnum, guint16 subtype )
{
	GList *gl = families;
	while(gl) {
		struct aim_family *fam = gl->data;
		if(fam->family == famnum) return match_strval(subtype, fam->subtypes);
		gl = gl->next;
	}

	return NULL;

}

const char *aim_get_familyname( guint16 famnum ) 
{
	GList *gl = families;
	while(gl) {
		struct aim_family *fam = gl->data;
		if(fam->family == famnum) return fam->name;
		gl = gl->next;
	}

	return NULL;
}

int aim_get_buddyname( char *name, tvbuff_t *tvb, int len_offset, int name_offset)
{
  guint8 buddyname_length;

  buddyname_length = tvb_get_guint8(tvb, len_offset);

  if(buddyname_length > MAX_BUDDYNAME_LENGTH ) buddyname_length = MAX_BUDDYNAME_LENGTH;
  tvb_get_nstringz0(tvb, name_offset, buddyname_length + 1, name);

  return buddyname_length;
}


void aim_get_message( guchar *msg, tvbuff_t *tvb, int msg_offset, int msg_length)
{
  int i,j,c;
  int bracket = FALSE;
  int max, tagchars = 0;
  int new_offset = msg_offset;
  int new_length = msg_length;



  /* make sure nothing bigger than 1000 bytes is printed */
  if( msg_length > 999 ) return;

  memset( msg, '\0', 1000);
  i = 0;
  c = 0;

  /* loop until HTML tag is reached - quick&dirty way to find start of message
   * (it is nearly impossible to find the correct start offset for all client versions) */
  while( (tagchars < 6) && (new_length > 5) )
  {
     j = tvb_get_guint8(tvb, new_offset);
     if( ( (j == '<') && (tagchars == 0) ) ||
         ( (j == 'h') && (tagchars == 1) ) ||
         ( (j == 'H') && (tagchars == 1) ) ||
         ( (j == 't') && (tagchars == 2) ) ||
         ( (j == 'T') && (tagchars == 2) ) ||
         ( (j == 'm') && (tagchars == 3) ) ||
         ( (j == 'M') && (tagchars == 3) ) ||
         ( (j == 'l') && (tagchars == 4) ) ||
         ( (j == 'L') && (tagchars == 4) ) ||
         ( (j == '>') && (tagchars == 5) ) ) tagchars++;
     new_offset++;
     new_length--;
  }

  /* set offset and length of message to after the first HTML tag */
  msg_offset = new_offset;
  msg_length = new_length;
  max = msg_length - 1;
  tagchars = 0;

  /* find the rest of the message until either a </html> is reached or the end of the frame.
   * All other HTML tags are stripped to display only the raw message (printable characters) */
  while( (c < max) && (tagchars < 7) )
  {
     j = tvb_get_guint8(tvb, msg_offset+c);


     /* make sure this is an HTML tag by checking the order of the chars */
     if( ( (j == '<') && (tagchars == 0) ) ||
         ( (j == '/') && (tagchars == 1) ) ||
         ( (j == 'h') && (tagchars == 2) ) ||
         ( (j == 'H') && (tagchars == 2) ) ||
         ( (j == 't') && (tagchars == 3) ) ||
         ( (j == 'T') && (tagchars == 3) ) ||
         ( (j == 'm') && (tagchars == 4) ) ||
         ( (j == 'M') && (tagchars == 4) ) ||
         ( (j == 'l') && (tagchars == 5) ) ||
         ( (j == 'L') && (tagchars == 5) ) ||
         ( (j == '>') && (tagchars == 6) ) ) tagchars++;

#ifdef STRIP_TAGS
     if( j == '<' ) bracket = TRUE;
     if( j == '>' ) bracket = FALSE;
     if( (isprint(j) ) && (bracket == FALSE) && (j != '>'))
#else
     if( isprint(j) )
#endif
     {
       msg[i] = j;
       i++;
     }
     c++;
  }
}

void aim_init_family(guint16 family, const char *name, const value_string *subtypes) 
{
	struct aim_family *fam = g_new(struct aim_family, 1);
	fam->name = g_strdup(name);
	fam->family = family;
	fam->subtypes = subtypes;
	families = g_list_append(families, fam);
}

static void dissect_aim_newconn(tvbuff_t *tvb, packet_info *pinfo, 
				int offset, proto_tree *tree)
{
  if (check_col(pinfo->cinfo, COL_INFO)) 
    col_add_fstr(pinfo->cinfo, COL_INFO, "New Connection");

  if (tvb_length_remaining(tvb, offset) > 0) {
	  proto_tree_add_item(tree, hf_aim_authcookie, tvb, offset, 4, FALSE);
	  offset+=4;
	  while(tvb_length_remaining(tvb, offset) > 0) {
		  offset = dissect_aim_tlv(tvb, pinfo, offset, tree);
	  }
  }

  if (tvb_length_remaining(tvb, offset) > 0)
    proto_tree_add_item(tree, hf_aim_data, tvb, offset, -1, FALSE);
}


int dissect_aim_snac_error(tvbuff_t *tvb, packet_info *pinfo, 
			     int offset, proto_tree *aim_tree)
{
  char *name;
  if ((name = match_strval(tvb_get_ntohs(tvb, offset), aim_snac_errors)) != NULL) {
     if (check_col(pinfo->cinfo, COL_INFO))
		col_add_fstr(pinfo->cinfo, COL_INFO, name);
  }

  proto_tree_add_item (aim_tree, hf_aim_snac_error,
			   tvb, offset, 2, FALSE);
  return tvb_length_remaining(tvb, 2);
}

static void dissect_aim_snac(tvbuff_t *tvb, packet_info *pinfo, 
			     int offset, proto_tree *aim_tree, proto_tree *root_tree)
{
  guint16 family;
  guint16 subtype;
  guint16 flags;
  guint32 id;
  proto_item *ti1;
  struct aiminfo aiminfo;
  const char *fam_name, *subtype_name;
  proto_tree *aim_tree_fnac = NULL;
  tvbuff_t *subtvb;
  int orig_offset;

  orig_offset = offset;
  family = tvb_get_ntohs(tvb, offset);
  fam_name = aim_get_familyname(family);
  offset += 2;
  subtype = tvb_get_ntohs(tvb, offset);
  subtype_name = aim_get_subtypename(family, subtype);
  offset += 2;
  flags = tvb_get_ntohs(tvb, offset);
  offset += 2;
  id = tvb_get_ntohl(tvb, offset);
  offset += 4;
  
  if (check_col(pinfo->cinfo, COL_INFO)) {
    col_add_fstr(pinfo->cinfo, COL_INFO, "SNAC data");
  }
  
  if( aim_tree )
    {
      offset = orig_offset;
      ti1 = proto_tree_add_text(aim_tree, tvb, 6, 10, "FNAC");
      aim_tree_fnac = proto_item_add_subtree(ti1, ett_aim_fnac);

      proto_tree_add_text (aim_tree_fnac, 
			   tvb, offset, 2, "Family: %s (0x%04x)", fam_name?fam_name:"Unknown", family);
      offset += 2;

      proto_tree_add_text (aim_tree_fnac, 
			   tvb, offset, 2, "Subtype: %s (0x%04x)", subtype_name?subtype_name:"Unknown", subtype);
      offset += 2;

      proto_tree_add_uint(aim_tree_fnac, hf_aim_fnac_flags, tvb, offset, 
			  2, flags);
      offset += 2;
      proto_tree_add_uint(aim_tree_fnac, hf_aim_fnac_id, tvb, offset,
			  4, id);
      offset += 4;
    }

  subtvb = tvb_new_subset(tvb, offset, -1, -1);
  aiminfo.tcpinfo = pinfo->private_data;
  aiminfo.family = family;
  aiminfo.subtype = subtype;
  pinfo->private_data = &aiminfo;
  
  if (check_col(pinfo->cinfo, COL_INFO)) {
     if(fam_name) col_append_fstr(pinfo->cinfo, COL_INFO, ", %s", fam_name);
	 else col_append_fstr(pinfo->cinfo, COL_INFO, ", Family: 0x%04x", family);
	 if(subtype_name) col_append_fstr(pinfo->cinfo, COL_INFO, ", %s", subtype_name);
	 else col_append_fstr(pinfo->cinfo, COL_INFO, ", Subtype: 0x%04x", subtype);
  }

  if(tvb_length_remaining(tvb,offset) == 0 || !dissector_try_port(subdissector_table, family, subtvb, pinfo, aim_tree)) {
    /* Show the undissected payload */
    if (tvb_length_remaining(tvb, offset) > 0)
      proto_tree_add_item(aim_tree, hf_aim_data, tvb, offset, -1, FALSE);
  }
}

static void dissect_aim_flap_err(tvbuff_t *tvb, packet_info *pinfo, 
				 int offset, proto_tree *tree)
{
  if (check_col(pinfo->cinfo, COL_INFO)) {
    col_add_fstr(pinfo->cinfo, COL_INFO, "FLAP error");
  }

  /* Show the undissected payload */
  if (tvb_length_remaining(tvb, offset) > 0)
    proto_tree_add_item(tree, hf_aim_data, tvb, offset, -1, FALSE);
}

static void dissect_aim_keep_alive(tvbuff_t *tvb, packet_info *pinfo, 
				   int offset, proto_tree *tree)
{
  if (check_col(pinfo->cinfo, COL_INFO)) {
    col_add_fstr(pinfo->cinfo, COL_INFO, "Keep Alive");
  }

  /* Show the undissected payload */
  if (tvb_length_remaining(tvb, offset) > 0)
    proto_tree_add_item(tree, hf_aim_data, tvb, offset, -1, FALSE);
}

static void dissect_aim_close_conn(tvbuff_t *tvb, packet_info *pinfo, 
				   int offset, proto_tree *tree)
{
  if (check_col(pinfo->cinfo, COL_INFO)) {
    col_add_fstr(pinfo->cinfo, COL_INFO, "Close Connection");
  }	  
  
  while(tvb_length_remaining(tvb, offset) > 0) {
	  offset = dissect_aim_tlv(tvb, pinfo, offset, tree);
  }
}

static void dissect_aim_unknown_channel(tvbuff_t *tvb, packet_info *pinfo, 
					int offset, proto_tree *tree)
{
  if (check_col(pinfo->cinfo, COL_INFO)) {
    col_add_fstr(pinfo->cinfo, COL_INFO, "Unknown Channel");
  }

  /* Show the undissected payload */
  if (tvb_length_remaining(tvb, offset) > 0)
    proto_tree_add_item(tree, hf_aim_data, tvb, offset, -1, FALSE);
}


int dissect_aim_tlv(tvbuff_t *tvb, packet_info *pinfo _U_, 
			   int offset, proto_tree *tree)
{
	return dissect_aim_tlv_specific(tvb, pinfo, offset, tree, global_tlvs);
}


int dissect_aim_tlv_buddylist(tvbuff_t *tvb, packet_info *pinfo _U_, 
			   int offset, proto_tree *tree)
{

	return dissect_aim_tlv_specific(tvb, pinfo, offset, tree, buddylist_tlvs);
}

/* Dissect a TLV value */
int dissect_aim_tlv_specific(tvbuff_t *tvb, packet_info *pinfo _U_, 
			   int offset, proto_tree *tree, const aim_tlv *tlv)
{
  guint16 valueid;
  guint16 length;
  int i = 0;
  const aim_tlv *tmp;
  proto_item *ti1;
  proto_tree *tlv_tree;
  int orig_offset;
  guint16 value16;
  guint32 value32;

  /* Record the starting offset so we can reuse it at the second pass */
  orig_offset = offset;

  /* Get the value ID */
  valueid = tvb_get_ntohs(tvb, offset);
  offset += 2;

  /* Figure out which entry applies from the tlv list */
  tmp = tlv;
  while (tmp[i].valueid) {
    if (tmp[i].valueid == valueid) {
      /* We found a match */
      break;
    }
    i++;
  }

  /* At this point, we are either pointing at the correct record, or 
     we didn't find the record, and are pointing at the last item in the 
     list */

  length = tvb_get_ntohs(tvb, offset);
  offset += 2;
  offset += length;

  if (tree) {
    offset = orig_offset;
    
    /* Show the info in the top of the tree if it's one of the standard
       data types */
    if (tmp[i].datatype == FT_STRING && length > 0) {
      guint8 *buf;
      buf = tvb_get_string(tvb, offset + 4, length);
      ti1 = proto_tree_add_text(tree, tvb, offset, length + 4, 
				"%s: %s", tmp[i].desc, buf);
      g_free(buf);
    }
    else if (tmp[i].datatype == FT_UINT16) {
      value16 = tvb_get_ntohs(tvb, offset + 4);
      ti1 = proto_tree_add_text(tree, tvb, offset, length + 4, 
				"%s: %d", tmp[i].desc, value16);
    }
    else if (tmp[i].datatype == FT_UINT32) {
      value32 = tvb_get_ntohl(tvb, offset + 4);
      ti1 = proto_tree_add_text(tree, tvb, offset, length + 4, 
				"%s: %d", tmp[i].desc, value32);
    }
    else {
      ti1 = proto_tree_add_text(tree, tvb, offset, length + 4, 
				"%s", tmp[i].desc);
    }

    tlv_tree = proto_item_add_subtree(ti1, ett_aim_tlv);

    proto_tree_add_text(tlv_tree, tvb, offset, 2,
			"Value ID: %s (0x%04x)", tmp[i].desc, valueid);
    offset += 2;
    
    proto_tree_add_text(tlv_tree, tvb, offset, 2,
			"Length: %d", length);
    offset += 2;

    ti1 = proto_tree_add_text(tlv_tree, tvb, offset, length,
			      "Value");
    offset += length;
  }

  /* Return the new length */
  return offset;
}


/* Register the protocol with Ethereal */
void
proto_register_aim(void)
{

/* Setup list of header fields */
  static hf_register_info hf[] = {
    { &hf_aim_cmd_start,
      { "Command Start", "aim.cmd_start", FT_UINT8, BASE_HEX, NULL, 0x0, "", HFILL }
    },
    { &hf_aim_channel,
      { "Channel ID", "aim.channel", FT_UINT8, BASE_HEX, VALS(aim_flap_channels), 0x0, "", HFILL }
    },
    { &hf_aim_seqno,
      { "Sequence Number", "aim.seqno", FT_UINT16, BASE_DEC, NULL, 0x0, "", HFILL }
    },
	{ &hf_aim_authcookie,
	  { "Authentication Cookie", "aim.authcookie", FT_BYTES, BASE_DEC, NULL, 0x0, "", HFILL },
	},
    { &hf_aim_data_len,
      { "Data Field Length", "aim.datalen", FT_UINT16, BASE_DEC, NULL, 0x0, "", HFILL }
    },
    { &hf_aim_data,
      { "Data", "aim.data", FT_BYTES, BASE_HEX, NULL, 0x0, "", HFILL }
    },
    { &hf_aim_signon_challenge_len,
      { "Signon challenge length", "aim.signon.challengelen", FT_UINT16, BASE_DEC, NULL, 0x0, "", HFILL }
    },
    { &hf_aim_signon_challenge,
      { "Signon challenge", "aim.signon.challenge", FT_STRING, BASE_HEX, NULL, 0x0, "", HFILL }
    },
    { &hf_aim_fnac_family,
      { "FNAC Family ID", "aim.fnac.family", FT_UINT16, BASE_HEX, NULL, 0x0, "", HFILL }
    },
    { &hf_aim_fnac_subtype,
      { "FNAC Subtype ID", "aim.fnac.subtype", FT_UINT16, BASE_HEX, NULL, 0x0, "", HFILL }
    },
    { &hf_aim_fnac_flags,
      { "FNAC Flags", "aim.fnac.flags", FT_UINT16, BASE_HEX, NULL, 0x0, "", HFILL }
    },
    { &hf_aim_fnac_id,
      { "FNAC ID", "aim.fnac.id", FT_UINT32, BASE_HEX, NULL, 0x0, "", HFILL }
    },
    { &hf_aim_infotype,
      { "Infotype", "aim.infotype", FT_UINT16, BASE_HEX, NULL, 0x0, "", HFILL }
    },
    { &hf_aim_buddyname_len,
      { "Buddyname len", "aim.buddynamelen", FT_UINT8, BASE_DEC, NULL, 0x0, "", HFILL }
    },
    { &hf_aim_buddyname,
      { "Buddy Name", "aim.buddyname", FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL }
    },
    { &hf_aim_userinfo_warninglevel,
      { "Warning Level", "aim.userinfo.warninglevel", FT_UINT16, BASE_DEC, NULL, 0x0, "", HFILL },
    },
    { &hf_aim_userinfo_tlvcount,
      { "TLV Count", "aim.userinfo.tlvcount", FT_UINT16, BASE_DEC, NULL, 0x0, "", HFILL },
    },
	{ &hf_aim_snac_error,
	  { "SNAC Error", "aim.snac.error", FT_UINT16,
		  BASE_HEX, VALS(aim_snac_errors), 0x0, "", HFILL },
	},
  };

/* Setup protocol subtree array */
  static gint *ett[] = {
    &ett_aim,
    &ett_aim_fnac,
    &ett_aim_tlv,
  };
  module_t *aim_module;

/* Register the protocol name and description */
  proto_aim = proto_register_protocol("AOL Instant Messenger", "AIM", "aim");

/* Required function calls to register the header fields and subtrees used */
  proto_register_field_array(proto_aim, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));

  aim_module = prefs_register_protocol(proto_aim, NULL);
  prefs_register_bool_preference(aim_module, "desegment",
    "Desegment all AIM messages spanning multiple TCP segments",
    "Whether the AIM dissector should desegment all messages spanning multiple TCP segments",
    &aim_desegment);

  subdissector_table = register_dissector_table("aim.family", 
		"Family ID", FT_UINT16, BASE_HEX);
}

void
proto_reg_handoff_aim(void)
{
  dissector_handle_t aim_handle;

  aim_handle = new_create_dissector_handle(dissect_aim, proto_aim);
  dissector_add("tcp.port", TCP_PORT_AIM, aim_handle);
}
