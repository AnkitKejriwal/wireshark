/* packet-telnet.c
 * Routines for Telnet packet dissection; see RFC 854 and RFC 855
 * Copyright 1999, Richard Sharpe <rsharpe@ns.aus.com>
 *
 * $Id: packet-telnet.c,v 1.44 2004/02/03 18:41:19 guy Exp $
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@ethereal.com>
 * Copyright 1998 Gerald Combs
 *
 * Copied from packet-pop.c
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
/* Telnet authentication options as per     RFC2941
 * Kerberos v5 telnet authentication as per RFC2942
 */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>

#include <string.h>
#include <glib.h>
#include <epan/packet.h>
#include <epan/strutil.h>
#include "packet-kerberos.h"

static int proto_telnet = -1;
static int hf_telnet_auth_cmd = -1;
static int hf_telnet_auth_name = -1;
static int hf_telnet_auth_type = -1;
static int hf_telnet_auth_mod_who = -1;
static int hf_telnet_auth_mod_how = -1;
static int hf_telnet_auth_mod_cred_fwd = -1;
static int hf_telnet_auth_mod_enc = -1;
static int hf_telnet_auth_krb5_type = -1;

static gint ett_telnet = -1;
static gint ett_telnet_subopt = -1;
static gint ett_status_subopt = -1;
static gint ett_rcte_subopt = -1;
static gint ett_olw_subopt = -1;
static gint ett_ops_subopt = -1;
static gint ett_crdisp_subopt = -1;
static gint ett_htstops_subopt = -1;
static gint ett_htdisp_subopt = -1;
static gint ett_ffdisp_subopt = -1;
static gint ett_vtstops_subopt = -1;
static gint ett_vtdisp_subopt = -1;
static gint ett_lfdisp_subopt = -1;
static gint ett_extasc_subopt = -1;
static gint ett_bytemacro_subopt = -1;
static gint ett_det_subopt = -1;
static gint ett_supdupout_subopt = -1;
static gint ett_sendloc_subopt = -1;
static gint ett_termtype_subopt = -1;
static gint ett_tacacsui_subopt = -1;
static gint ett_outmark_subopt = -1;
static gint ett_tlocnum_subopt = -1;
static gint ett_tn3270reg_subopt = -1;
static gint ett_x3pad_subopt = -1;
static gint ett_naws_subopt = -1;
static gint ett_tspeed_subopt = -1;
static gint ett_rfc_subopt = -1;
static gint ett_linemode_subopt = -1;
static gint ett_xdpyloc_subopt = -1;
static gint ett_env_subopt = -1;
static gint ett_auth_subopt = -1;
static gint ett_enc_subopt = -1;
static gint ett_newenv_subopt = -1;
static gint ett_tn3270e_subopt = -1;
static gint ett_xauth_subopt = -1;
static gint ett_charset_subopt = -1;
static gint ett_rsp_subopt = -1;
static gint ett_comport_subopt = -1;


/* Some defines for Telnet */

#define TCP_PORT_TELNET			23

#define TN_IAC   255
#define TN_DONT  254
#define TN_DO    253
#define TN_WONT  252
#define TN_WILL  251
#define TN_SB    250
#define TN_GA    249
#define TN_EL    248
#define TN_EC    247
#define TN_AYT   246
#define TN_AO    245
#define TN_IP    244
#define TN_BRK   243
#define TN_DM    242
#define TN_NOP   241
#define TN_SE    240
#define TN_EOR   239
#define TN_ABORT 238
#define TN_SUSP  237
#define TN_EOF   236


typedef enum {
  NO_LENGTH,		/* option has no data, hence no length */
  FIXED_LENGTH,		/* option always has the same length */
  VARIABLE_LENGTH	/* option is variable-length - optlen is minimum */
} tn_opt_len_type;

/* Member of table of IP or TCP options. */
typedef struct tn_opt {
  char  *name;			/* name of option */
  gint  *subtree_index;		/* pointer to subtree index for option */
  tn_opt_len_type len_type;	/* type of option length field */
  int	optlen;			/* value length should be (minimum if VARIABLE) */
  void	(*dissect)(packet_info *pinfo, const char *, tvbuff_t *, int, int, proto_tree *);
				/* routine to dissect option */
} tn_opt;

static void
dissect_string_subopt(packet_info *pinfo _U_, const char *optname, tvbuff_t *tvb, int offset, int len,
                      proto_tree *tree)
{
  guint8 cmd;

  cmd = tvb_get_guint8(tvb, offset);
  switch (cmd) {

  case 0:	/* IS */
    proto_tree_add_text(tree, tvb, offset, 1, "Here's my %s", optname);
    offset++;
    len--;
    if (len > 0) {
      proto_tree_add_text(tree, tvb, offset, len, "Value: %s",
                          tvb_format_text(tvb, offset, len));
    }
    break;

  case 1:	/* SEND */
    proto_tree_add_text(tree, tvb, offset, 1, "Send your %s", optname);
    offset++;
    len--;
    if (len > 0)
      proto_tree_add_text(tree, tvb, offset, len, "Extra data");
    break;

  default:
    proto_tree_add_text(tree, tvb, offset, 1, "Invalid %s subcommand %u",
                        optname, cmd);
    offset++;
    len--;
    if (len > 0)
      proto_tree_add_text(tree, tvb, offset, len, "Subcommand data");
    break;
  }
}

static void
dissect_outmark_subopt(packet_info *pinfo _U_, const char *optname _U_, tvbuff_t *tvb, int offset,
                       int len, proto_tree *tree)
{
  guint8 cmd;
  int gs_offset, datalen;

  while (len > 0) {
    cmd = tvb_get_guint8(tvb, offset);
    switch (cmd) {

    case 6:	/* ACK */
      proto_tree_add_text(tree, tvb, offset, 1, "ACK");
      break;

    case 21:	/* NAK */
      proto_tree_add_text(tree, tvb, offset, 1, "NAK");
      break;

    case 'D':
      proto_tree_add_text(tree, tvb, offset, 1, "Default");
      break;

    case 'T':
      proto_tree_add_text(tree, tvb, offset, 1, "Top");
      break;

    case 'B':
      proto_tree_add_text(tree, tvb, offset, 1, "Bottom");
      break;

    case 'L':
      proto_tree_add_text(tree, tvb, offset, 1, "Left");
      break;

    case 'R':
      proto_tree_add_text(tree, tvb, offset, 1, "Right");
      break;

    default:
      proto_tree_add_text(tree, tvb, offset, 1, "Bogus value: %u", cmd);
      break;
    }
    offset++;
    len--;

    /* Look for a GS */
    gs_offset = tvb_find_guint8(tvb, offset, len, 29);
    if (gs_offset == -1) {
      /* None found - run to the end of the packet. */
      gs_offset = offset + len;
    }
    datalen = gs_offset - offset;
    if (datalen > 0) {
      proto_tree_add_text(tree, tvb, offset, datalen, "Banner: %s",
                          tvb_format_text(tvb, offset, datalen));
      offset += datalen;
      len -= datalen;
    }
  }
}

static void
dissect_htstops_subopt(packet_info *pinfo _U_, const char *optname, tvbuff_t *tvb, int offset, int len,
                       proto_tree *tree)
{
  guint8 cmd;
  guint8 tabval;

  cmd = tvb_get_guint8(tvb, offset);
  switch (cmd) {

  case 0:	/* IS */
    proto_tree_add_text(tree, tvb, offset, 1, "Here's my %s", optname);
    offset++;
    len--;
    break;

  case 1:	/* SEND */
    proto_tree_add_text(tree, tvb, offset, 1, "Send your %s", optname);
    offset++;
    len--;
    break;

  default:
    proto_tree_add_text(tree, tvb, offset, 1, "Invalid %s subcommand %u",
                        optname, cmd);
    offset++;
    len--;
    if (len > 0)
      proto_tree_add_text(tree, tvb, offset, len, "Subcommand data");
    return;
  }

  while (len > 0) {
    tabval = tvb_get_guint8(tvb, offset);
    switch (tabval) {

    case 0:
      proto_tree_add_text(tree, tvb, offset, 1,
                          "Sender wants to handle tab stops");
      break;

    default:
      proto_tree_add_text(tree, tvb, offset, 1,
                          "Sender wants receiver to handle tab stop at %u",
                          tabval);
      break;

    case 251:
    case 252:
    case 253:
    case 254:
      proto_tree_add_text(tree, tvb, offset, 1,
                          "Invalid value: %u", tabval);
      break;

    case 255:
      proto_tree_add_text(tree, tvb, offset, 1,
                          "Sender wants receiver to handle tab stops");
      break;
    }
    offset++;
    len--;
  }
}

static void
dissect_naws_subopt(packet_info *pinfo _U_, const char *optname _U_, tvbuff_t *tvb, int offset,
                    int len _U_, proto_tree *tree)
{
  proto_tree_add_text(tree, tvb, offset, 2, "Width: %u",
                      tvb_get_ntohs(tvb, offset));
  offset += 2;
  proto_tree_add_text(tree, tvb, offset, 2, "Height: %u",
                      tvb_get_ntohs(tvb, offset));
}

/* BEGIN RFC-2217 (COM Port Control) Definitions */

#define TNCOMPORT_SIGNATURE		0
#define TNCOMPORT_SETBAUDRATE		1
#define TNCOMPORT_SETDATASIZE		2
#define TNCOMPORT_SETPARITY		3
#define TNCOMPORT_SETSTOPSIZE		4
#define TNCOMPORT_SETCONTROL		5
#define TNCOMPORT_NOTIFYLINESTATE	6
#define TNCOMPORT_NOTIFYMODEMSTATE	7
#define TNCOMPORT_FLOWCONTROLSUSPEND	8
#define TNCOMPORT_FLOWCONTROLRESUME      9
#define TNCOMPORT_SETLINESTATEMASK	10
#define TNCOMPORT_SETMODEMSTATEMASK	11
#define TNCOMPORT_PURGEDATA		12

/* END RFC-2217 (COM Port Control) Definitions */

static void
dissect_comport_subopt(packet_info *pinfo _U_, const char *optname, tvbuff_t *tvb, int offset, int len,
                       proto_tree *tree)
{static const char *datasizes[] = {
    "Request",
    "<invalid>",
    "<invalid>",
    "<invalid>",
    "<invalid>",
    "5",
    "6",
    "7",
    "8"
 };
 static const char *parities[] = {
    "Request",
    "None",
    "Odd",
    "Even",
    "Mark",
    "Space"
 };
 static const char *stops[] = {
    "Request",
    "1",
    "2",
    "1.5"
 };
 static const char *control[] = {
    "Output Flow Control Request",
    "Output Flow: None",
    "Output Flow: XON/XOFF",
    "Output Flow: CTS/RTS",
    "Break Request",
    "Break: ON",
    "Break: OFF",
    "DTR Request",
    "DTR: ON",
    "DTR: OFF",
    "RTS Request",
    "RTS: ON",
    "RTS: OFF",
    "Input Flow Control Request",
    "Input Flow: None",
    "Input Flow: XON/XOFF",
    "Input Flow: CTS/RTS",
    "Output Flow: DCD",
    "Input Flow: DTR",
    "Output Flow: DSR"
 };
 static const char *linestate_bits[] = {
    "Data Ready",
    "Overrun Error",
    "Parity Error",
    "Framing Error",
    "Break Detected",
    "Transfer Holding Register Empty",
    "Transfer Shift Register Empty",
    "Timeout Error"
 };
 static const char *modemstate_bits[] = {
     "DCTS",
     "DDSR",
     "TERI",
     "DDCD",
     "CTS",
     "DSR",
     "RI",
     "DCD"
 };
 static const char *purges[] = {
     "Purge None",
     "Purge RX",
     "Purge TX",
     "Purge RX/TX"
 };
  
  guint8 cmd;
  guint8 isservercmd;
  char *source;
  
  cmd = tvb_get_guint8(tvb, offset);
  isservercmd = cmd > 99;
  cmd = (isservercmd) ? (cmd - 100) : cmd;
  source = (isservercmd) ? "Server" : "Client";
  switch (cmd) {

  case TNCOMPORT_SIGNATURE:
    len--;
    if (len == 0) {
        proto_tree_add_text(tree, tvb, offset, 1, "%s Requests Signature",source);
    } else {
        guint8 *sig = tvb_get_string(tvb, offset + 1, len);
        proto_tree_add_text(tree, tvb, offset, 1 + len, "%s Signature: %s",source, sig);
	g_free(sig);
    }
    break;

  case TNCOMPORT_SETBAUDRATE:
    len--;
    if (len >= 4) {
  	guint32 baud = tvb_get_ntohl(tvb, offset+1);
        if (baud == 0) {
            proto_tree_add_text(tree, tvb, offset, 5, "%s Requests Baud Rate",source);            
        } else {
            proto_tree_add_text(tree, tvb, offset, 5, "%s Baud Rate: %d",source,baud);
        }
    } else {
        proto_tree_add_text(tree, tvb, offset, 1 + len, "%s <Invalid Baud Rate Packet>",source);
    }
    break;
    
  case TNCOMPORT_SETDATASIZE:
    len--;
    if (len >= 1) {
  	guint8 datasize = tvb_get_guint8(tvb, offset+1);
        const char *ds = (datasize > 8) ? "<invalid>" : datasizes[datasize];
        proto_tree_add_text(tree, tvb, offset, 2, "%s Data Size: %s",source,ds);
    } else {
        proto_tree_add_text(tree, tvb, offset, 1 + len, "%s <Invalid Data Size Packet>",source);
    }
    break;

  case TNCOMPORT_SETPARITY:
    len--;
    if (len >= 1) {
  	guint8 parity = tvb_get_guint8(tvb, offset+1);
        const char *pr = (parity > 5) ? "<invalid>" : parities[parity];
        proto_tree_add_text(tree, tvb, offset, 2, "%s Parity: %s",source,pr);
    } else {
        proto_tree_add_text(tree, tvb, offset, 1 + len, "%s <Invalid Parity Packet>",source);
    }
    break;
    
  case TNCOMPORT_SETSTOPSIZE:
    len--;
    if (len >= 1) {
  	guint8 stop = tvb_get_guint8(tvb, offset+1);
        const char *st = (stop > 3) ? "<invalid>" : stops[stop];
        proto_tree_add_text(tree, tvb, offset, 2, "%s Stop: %s",source,st);
    } else {
        proto_tree_add_text(tree, tvb, offset, 1 + len, "%s <Invalid Stop Packet>",source);
    }
    break;

  case TNCOMPORT_SETCONTROL:
    len--;
    if (len >= 1) {
  	guint8 crt = tvb_get_guint8(tvb, offset+1);
        const char *c = (crt > 19) ? "Control: <invalid>" : control[crt];
        proto_tree_add_text(tree, tvb, offset, 2, "%s %s",source,c);
    } else {
        proto_tree_add_text(tree, tvb, offset, 1 + len, "%s <Invalid Control Packet>",source);
    }
    break;
    
  case TNCOMPORT_SETLINESTATEMASK:
  case TNCOMPORT_NOTIFYLINESTATE:
    len--;
    if (len >= 1) {
        const char *print_pattern = (cmd == TNCOMPORT_SETLINESTATEMASK) ?
                                        "%s Set Linestate Mask: %s" : "%s Linestate: %s";
        char ls_buffer[512];
  	guint8 ls = tvb_get_guint8(tvb, offset+1);
        int print_count = 0;
        int idx;
        ls_buffer[0] = '\0';
        for (idx = 0; idx < 8; idx++) {
            int bit = ls & 1;
            if (bit) {
                if (print_count != 0) {
                    strcat(ls_buffer,", ");
                }
                strcat(ls_buffer,linestate_bits[idx]);
                print_count++;
            }
            ls = ls >> 1;
        }
        proto_tree_add_text(tree, tvb, offset, 2, print_pattern, source, ls_buffer);
    } else {
        const char *print_pattern = (cmd == TNCOMPORT_SETLINESTATEMASK) ?
                                        "%s <Invalid Linestate Mask>" : "%s <Invalid Linestate Packet>";
        proto_tree_add_text(tree, tvb, offset, 1 + len, print_pattern, source);
    }
    break;
    
  case TNCOMPORT_SETMODEMSTATEMASK:
  case TNCOMPORT_NOTIFYMODEMSTATE:
    len--;
    if (len >= 1) {
        const char *print_pattern = (cmd == TNCOMPORT_SETMODEMSTATEMASK) ?
                                        "%s Set Modemstate Mask: %s" : "%s Modemstate: %s";
        char ms_buffer[256];
  	    guint8 ms = tvb_get_guint8(tvb, offset+1);
        int print_count = 0;
        int idx;
        ms_buffer[0] = '\0';
        for (idx = 0; idx < 8; idx++) {
            int bit = ms & 1;
            if (bit) {
                if (print_count != 0) {
                    strcat(ms_buffer,", ");
                }
                strcat(ms_buffer,modemstate_bits[idx]);
                print_count++;
            }
            ms = ms >> 1;
        }
        proto_tree_add_text(tree, tvb, offset, 2, print_pattern, source, ms_buffer);
    } else {
        const char *print_pattern = (cmd == TNCOMPORT_SETMODEMSTATEMASK) ?
                                         "%s <Invalid Modemstate Mask>" : "%s <Invalid Modemstate Packet>";
        proto_tree_add_text(tree, tvb, offset, 1 + len, print_pattern, source);
    }
    break;

  case TNCOMPORT_FLOWCONTROLSUSPEND:
    len--;
    proto_tree_add_text(tree, tvb, offset, 1, "%s Flow Control Suspend",source);
    break;
  
  case TNCOMPORT_FLOWCONTROLRESUME:
    len--;
    proto_tree_add_text(tree, tvb, offset, 1, "%s Flow Control Resume",source);
    break;
  
  case TNCOMPORT_PURGEDATA:
    len--;
    if (len >= 1) {
  	    guint8 purge = tvb_get_guint8(tvb, offset+1);
        const char *p = (purge > 3) ? "<Purge invalid>" : purges[purge];
        proto_tree_add_text(tree, tvb, offset, 2, "%s %s",source,p);
    } else {
        proto_tree_add_text(tree, tvb, offset, 1 + len, "%s <Invalid Purge Packet>",source);
    }
    break;
  
  default:
    proto_tree_add_text(tree, tvb, offset, 1, "Invalid %s subcommand %u",
                        optname, cmd);
    offset++;
    len--;
    if (len > 0)
      proto_tree_add_text(tree, tvb, offset, len, "Subcommand data");
    return;
  }

}

static const value_string rfc_opt_vals[] = {
	{ 0, "OFF" },
	{ 1, "ON" },
	{ 2, "RESTART-ANY" },
	{ 3, "RESTART-XON" },
	{ 0, NULL }
};

static void
dissect_rfc_subopt(packet_info *pinfo _U_, const char *optname _U_, tvbuff_t *tvb, int offset,
                   int len _U_, proto_tree *tree)
{
  guint8 cmd;

  cmd = tvb_get_guint8(tvb, offset);
  proto_tree_add_text(tree, tvb, offset, 2, "%s",
                      val_to_str(cmd, rfc_opt_vals, "Unknown (%u)"));
}


#define TN_AC_IS	0
#define TN_AC_SEND	1
#define TN_AC_REPLY	2
#define TN_AC_NAME	3
static const value_string auth_cmd_vals[] = {
	{ TN_AC_IS,	"IS" },
	{ TN_AC_SEND,	"SEND" },
	{ TN_AC_REPLY,	"REPLY" },
	{ TN_AC_NAME,	"NAME" },
	{ 0, NULL }
};

#define TN_AT_NULL	0
#define TN_AT_KRB4	1
#define TN_AT_KRB5	2
#define TN_AT_SPX	3
#define TN_AT_MINK	4
#define TN_AT_SRP	5
#define TN_AT_RSA	6
#define TN_AT_SSL	7
#define TN_AT_LOKI	10
#define TN_AT_SSA	11
#define TN_AT_KEA_SJ	12
#define TN_AT_KEA_SJ_INTEG	13
#define TN_AT_DSS	14
#define TN_AT_NTLM	15
static const value_string auth_type_vals[] = {
	{ TN_AT_NULL,	"NULL" },
	{ TN_AT_KRB4,	"Kerberos v4" },
	{ TN_AT_KRB5,	"Kerberos v5" },
	{ TN_AT_SPX,	"SPX" },
	{ TN_AT_MINK,	"MINK" },
	{ TN_AT_SRP,	"SRP" },
	{ TN_AT_RSA,	"RSA" },
	{ TN_AT_SSL,	"SSL" },
	{ TN_AT_LOKI,	"LOKI" },
	{ TN_AT_SSA,	"SSA" },
	{ TN_AT_KEA_SJ,	"KEA_SJ" },
	{ TN_AT_KEA_SJ_INTEG, "KEA_SJ_INTEG" },
	{ TN_AT_DSS,	"DSS" },
	{ TN_AT_NTLM,	"NTLM" },
	{ 0, NULL }
};
static const true_false_string auth_mod_cred_fwd = {
	"Client WILL forward auth creds",
	"Client will NOT forward auth creds"
};
static const true_false_string auth_mod_who = {
	"Mask server to client",
	"Mask client to server"
};
static const true_false_string auth_mod_how = {
	"MUTUAL authentication",
	"One Way authentication"
};
#define TN_AM_OFF		0x00
#define TN_AM_USING_TELOPT	0x01
#define TN_AM_AFTER_EXCHANGE	0x02
#define TN_AM_RESERVED		0x04
static const value_string auth_mod_enc[] = {
	{ TN_AM_OFF,		"Off" },
	{ TN_AM_USING_TELOPT,	"Telnet Options" },
	{ TN_AM_AFTER_EXCHANGE, "After Exchange" },
	{ TN_AM_RESERVED,	"Reserved" },
	{ 0, NULL }
};
#define TN_KRB5_TYPE_AUTH		0
#define TN_KRB5_TYPE_REJECT		1
#define TN_KRB5_TYPE_ACCEPT		2
#define TN_KRB5_TYPE_RESPONSE		3
#define TN_KRB5_TYPE_FORWARD		4
#define TN_KRB5_TYPE_FORWARD_ACCEPT	5
#define TN_KRB5_TYPE_FORWARD_REJECT	6
static const value_string auth_krb5_types[] = {
	{ TN_KRB5_TYPE_AUTH,		"Auth" },
	{ TN_KRB5_TYPE_REJECT,		"Reject" },
	{ TN_KRB5_TYPE_ACCEPT,		"Accept" },
	{ TN_KRB5_TYPE_RESPONSE,	"Response" },
	{ TN_KRB5_TYPE_FORWARD,		"Forward" },
	{ TN_KRB5_TYPE_FORWARD_ACCEPT,	"Forward Accept" },
	{ TN_KRB5_TYPE_FORWARD_REJECT,	"Forward Reject" },
	{ 0, NULL }
};
static void
dissect_authentication_type_pair(packet_info *pinfo _U_, tvbuff_t *tvb, int offset, proto_tree *tree)
{
	guint8 type, mod;

	type=tvb_get_guint8(tvb, offset);
	proto_tree_add_uint(tree, hf_telnet_auth_type, tvb, offset, 1, type);

	mod=tvb_get_guint8(tvb, offset+1);
	proto_tree_add_uint(tree, hf_telnet_auth_mod_enc, tvb, offset+1, 1, mod);
	proto_tree_add_boolean(tree, hf_telnet_auth_mod_cred_fwd, tvb, offset+1, 1, mod);
	proto_tree_add_boolean(tree, hf_telnet_auth_mod_how, tvb, offset+1, 1, mod);
	proto_tree_add_boolean(tree, hf_telnet_auth_mod_who, tvb, offset+1, 1, mod);
}

/* no kerberos blobs are ever >10kb ? (arbitrary limit) */
#define MAX_KRB5_BLOB_LEN	10240

static tvbuff_t *
unescape_and_tvbuffify_telnet_option(packet_info *pinfo, tvbuff_t *tvb, int offset, int len)
{
	tvbuff_t *krb5_tvb;
	guint8 *buf;
	const guint8 *spos;
	guint8 *dpos;
	int skip, l;

	if(len>=MAX_KRB5_BLOB_LEN)
		return NULL;

	/* XXX we never g_free() this one.  This is done automagically
	   when the parent tvb is destroyed?
        */
	buf=g_malloc(len);
	spos=tvb_get_ptr(tvb, offset, len);
	dpos=buf;
	skip=0;
	l=len;
	while(l>0){
		if((spos[0]==0xff) && (spos[1]==0xff)){
			skip++;
			l-=2;
			*(dpos++)=0xff;
			spos+=2;
			continue;
		}
		*(dpos++)=*(spos++);
		l--;
	}
	krb5_tvb = tvb_new_real_data(buf, len-skip, len-skip); 
	tvb_set_child_real_data_tvbuff(tvb, krb5_tvb);
	add_new_data_source(pinfo, krb5_tvb, "Unpacked Telnet Uption");

	return krb5_tvb;
}


/* as per RFC2942 */
static void
dissect_krb5_authentication_data(packet_info *pinfo, tvbuff_t *tvb, int offset, int len, proto_tree *tree, guint8 acmd)
{
	tvbuff_t *krb5_tvb;
	guint8 krb5_cmd;

	dissect_authentication_type_pair(pinfo, tvb, offset, tree);
	offset+=2;
	len-=2;


	krb5_cmd=tvb_get_guint8(tvb, offset);
	proto_tree_add_uint(tree, hf_telnet_auth_krb5_type, tvb, offset, 1, krb5_cmd);
	offset++;
	len--;


	/* IAC SB AUTHENTICATION IS <authentication-type-pair> AUTH <Kerberos V5 KRB_AP_REQ message> IAC SE */
	if((acmd==TN_AC_IS)&&(krb5_cmd==TN_KRB5_TYPE_AUTH)){
		krb5_tvb=unescape_and_tvbuffify_telnet_option(pinfo, tvb, offset, len);
		if(krb5_tvb)
			dissect_kerberos_main(krb5_tvb, pinfo, tree, FALSE);
		else
			proto_tree_add_text(tree, tvb, offset, len, "Kerberos blob (too long to dissect - length %u > %u",
			    len, MAX_KRB5_BLOB_LEN);
	}



	/* IAC SB AUTHENTICATION REPLY <authentication-type-pair> ACCEPT IAC SE */
	/* nothing more to dissect */



	/* IAC SB AUTHENTICATION REPLY <authentication-type-pair> REJECT <optional reason for rejection> IAC SE*/
/*qqq*/


	/* IAC SB AUTHENTICATION REPLY <authentication-type-pair> RESPONSE <KRB_AP_REP message> IAC SE */
	if((acmd==TN_AC_REPLY)&&(krb5_cmd==TN_KRB5_TYPE_RESPONSE)){
		krb5_tvb=unescape_and_tvbuffify_telnet_option(pinfo, tvb, offset, len);
		dissect_kerberos_main(krb5_tvb, pinfo, tree, FALSE);
	}


	/* IAC SB AUTHENTICATION <authentication-type-pair> FORWARD <KRB_CRED message> IAC SE */
	/* XXX unclear what this one looks like */


	/* IAC SB AUTHENTICATION <authentication-type-pair> FORWARD_ACCEPT IAC SE */
	/* nothing more to dissect */



	/* IAC SB AUTHENTICATION <authentication-type-pair> FORWARD_REJECT */
	/* nothing more to dissect */
}

static void
dissect_authentication_subopt(packet_info *pinfo, const char *optname _U_, tvbuff_t *tvb, int offset, int len, proto_tree *tree)
{
	guint8 acmd;
	char name[256];

/* XXX here we should really split it up in a conversation struct keeping
       track of what method we actually use and not just assume it is always
       kerberos v5
*/
	acmd=tvb_get_guint8(tvb, offset);
	proto_tree_add_uint(tree, hf_telnet_auth_cmd, tvb, offset, 1, acmd);
	offset++;
	len--;

	switch(acmd){
	case TN_AC_REPLY:
	case TN_AC_IS:
		/* XXX here we shouldnt just assume it is krb5 */
		dissect_krb5_authentication_data(pinfo, tvb, offset, len, tree, acmd);
		break;
	case TN_AC_SEND:
		while(len>0){
			dissect_authentication_type_pair(pinfo, tvb, offset, tree);
			offset+=2;
			len-=2;
		}
		break;
	case TN_AC_NAME:
		if(len<255){
			tvb_memcpy(tvb, name, offset, len);
			name[len]=0;
		} else {
			strcpy(name, "<...name too long...>");
		}
		proto_tree_add_string(tree, hf_telnet_auth_name, tvb, offset, len, name);
		break;
	}
}

static tn_opt options[] = {
  {
    "Binary Transmission",			/* RFC 856 */
    NULL,					/* no suboption negotiation */
    NO_LENGTH,
    0,
    NULL
  },
  {
    "Echo",					/* RFC 857 */
    NULL,					/* no suboption negotiation */
    NO_LENGTH,
    0,
    NULL
  },
  {    
    "Reconnection",				/* DOD Protocol Handbook */
    NULL,
    NO_LENGTH,
    0,
    NULL
  },
  {
    "Suppress Go Ahead",			/* RFC 858 */
    NULL,					/* no suboption negotiation */
    NO_LENGTH,
    0,
    NULL
  },
  {
    "Approx Message Size Negotiation",		/* Ethernet spec(!) */
    NULL,
    NO_LENGTH,
    0,
    NULL
  },
  {
    "Status",					/* RFC 859 */
    &ett_status_subopt,
    VARIABLE_LENGTH,
    1,
    NULL					/* XXX - fill me in */
  },
  {
    "Timing Mark",				/* RFC 860 */
    NULL,					/* no suboption negotiation */
    NO_LENGTH,
    0,
    NULL
  },
  {
    "Remote Controlled Trans and Echo",		/* RFC 726 */
    &ett_rcte_subopt,
    VARIABLE_LENGTH,
    1,
    NULL					/* XXX - fill me in */
  },
  {
    "Output Line Width",			/* DOD Protocol Handbook */
    &ett_olw_subopt,
    VARIABLE_LENGTH,				/* XXX - fill me in */
    0,						/* XXX - fill me in */
    NULL					/* XXX - fill me in */
  },
  {
    "Output Page Size",				/* DOD Protocol Handbook */
    &ett_ops_subopt,
    VARIABLE_LENGTH,				/* XXX - fill me in */
    0,						/* XXX - fill me in */
    NULL					/* XXX - fill me in */
  },
  {
    "Output Carriage-Return Disposition",	/* RFC 652 */
    &ett_crdisp_subopt,
    FIXED_LENGTH,
    2,
    NULL					/* XXX - fill me in */
  },
  {
    "Output Horizontal Tab Stops",		/* RFC 653 */
    &ett_htstops_subopt,
    VARIABLE_LENGTH,
    1,
    dissect_htstops_subopt
  },
  {
    "Output Horizontal Tab Disposition",	/* RFC 654 */
    &ett_htdisp_subopt,
    FIXED_LENGTH,
    2,
    NULL					/* XXX - fill me in */
  },
  {
    "Output Formfeed Disposition",		/* RFC 655 */
    &ett_ffdisp_subopt,
    FIXED_LENGTH,
    2,
    NULL					/* XXX - fill me in */
  },
  {
    "Output Vertical Tabstops",			/* RFC 656 */
    &ett_vtstops_subopt,
    VARIABLE_LENGTH,
    1,
    NULL					/* XXX - fill me in */
  },
  {
    "Output Vertical Tab Disposition",		/* RFC 657 */
    &ett_vtdisp_subopt,
    FIXED_LENGTH,
    2,
    NULL					/* XXX - fill me in */
  },
  {
    "Output Linefeed Disposition",		/* RFC 658 */
    &ett_lfdisp_subopt,
    FIXED_LENGTH,
    2,
    NULL					/* XXX - fill me in */
  },
  {
    "Extended ASCII",				/* RFC 698 */
    &ett_extasc_subopt,
    FIXED_LENGTH,
    2,
    NULL					/* XXX - fill me in */
  },
  {
    "Logout",					/* RFC 727 */
    NULL,					/* no suboption negotiation */
    NO_LENGTH,
    0,
    NULL
  },
  {
    "Byte Macro",				/* RFC 735 */
    &ett_bytemacro_subopt,
    VARIABLE_LENGTH,
    2,
    NULL					/* XXX - fill me in */
  },
  {
    "Data Entry Terminal",			/* RFC 732, RFC 1043 */
    &ett_det_subopt,
    VARIABLE_LENGTH,
    2,
    NULL					/* XXX - fill me in */
  },
  {
    "SUPDUP",					/* RFC 734, RFC 736 */
    NULL,					/* no suboption negotiation */
    NO_LENGTH,
    0,
    NULL
  },
  {
    "SUPDUP Output",				/* RFC 749 */
    &ett_supdupout_subopt,
    VARIABLE_LENGTH,
    1,
    NULL					/* XXX - fill me in */
  },
  {
    "Send Location",				/* RFC 779 */
    &ett_sendloc_subopt,
    VARIABLE_LENGTH,
    0,
    NULL					/* XXX - fill me in */
  },
  {
    "Terminal Type",				/* RFC 1091 */
    &ett_termtype_subopt,
    VARIABLE_LENGTH,
    1,
    dissect_string_subopt
  },
  {
    "End of Record",				/* RFC 885 */
    NULL,					/* no suboption negotiation */
    NO_LENGTH,
    0,
    NULL
  },
  {
    "TACACS User Identification",		/* RFC 927 */
    &ett_tacacsui_subopt,
    FIXED_LENGTH,
    4,
    NULL					/* XXX - fill me in */
  },
  {
    "Output Marking",				/* RFC 933 */
    &ett_outmark_subopt,
    VARIABLE_LENGTH,
    1,
    dissect_outmark_subopt,
  },
  {
    "Terminal Location Number",			/* RFC 946 */
    &ett_tlocnum_subopt,
    VARIABLE_LENGTH,
    1,
    NULL					/* XXX - fill me in */
  },
  {
    "Telnet 3270 Regime",			/* RFC 1041 */
    &ett_tn3270reg_subopt,
    VARIABLE_LENGTH,
    1,
    NULL					/* XXX - fill me in */
  },
  {
    "X.3 PAD",					/* RFC 1053 */
    &ett_x3pad_subopt,
    VARIABLE_LENGTH,
    1,
    NULL					/* XXX - fill me in */
  },
  {
    "Negotiate About Window Size",		/* RFC 1073, DW183 */
    &ett_naws_subopt,
    FIXED_LENGTH,
    4,
    dissect_naws_subopt
  },
  {
    "Terminal Speed",				/* RFC 1079 */
    &ett_tspeed_subopt,
    VARIABLE_LENGTH,
    1,
    NULL					/* XXX - fill me in */
  },
  {
    "Remote Flow Control",			/* RFC 1372 */
    &ett_rfc_subopt,
    FIXED_LENGTH,
    1,
    dissect_rfc_subopt
  },
  {
    "Linemode",					/* RFC 1184 */
    &ett_linemode_subopt,
    VARIABLE_LENGTH,
    1,
    NULL					/* XXX - fill me in */
  },
  {
    "X Display Location",			/* RFC 1096 */
    &ett_xdpyloc_subopt,
    VARIABLE_LENGTH,
    1,
    dissect_string_subopt
  },
  {
    "Environment Option",			/* RFC 1408, RFC 1571 */
    &ett_env_subopt,
    VARIABLE_LENGTH,
    1,
    NULL					/* XXX - fill me in */
  },
  {
    "Authentication Option",			/* RFC 2941 */
    &ett_auth_subopt,
    VARIABLE_LENGTH,
    1,
    dissect_authentication_subopt		
  },
  {
    "Encryption Option",			/* RFC 2946 */
    &ett_enc_subopt,
    VARIABLE_LENGTH,
    1,
    NULL					/* XXX - fill me in */
  },
  {
    "New Environment Option",			/* RFC 1572 */
    &ett_newenv_subopt,
    VARIABLE_LENGTH,
    1,
    NULL					/* XXX - fill me in */
  },
  {
    "TN3270E",					/* RFC 1647 */
    &ett_tn3270e_subopt,
    VARIABLE_LENGTH,
    1,
    NULL					/* XXX - fill me in */
  },
  {
    "XAUTH",					/* XAUTH  */
    &ett_xauth_subopt,
    VARIABLE_LENGTH,
    1,
    NULL					/* XXX - fill me in */
  },
  {
    "CHARSET",					/* CHARSET  */
    &ett_charset_subopt,
    VARIABLE_LENGTH,
    1,
    NULL					/* XXX - fill me in */
  },
  {
    "Remote Serial Port",				/* Remote Serial Port */
    &ett_rsp_subopt,
    VARIABLE_LENGTH,
    1,
    NULL					/* XXX - fill me in */
  },
  {
    "COM Port Control",					/* RFC 2217 */
    &ett_comport_subopt,
    VARIABLE_LENGTH,
    1,
    dissect_comport_subopt
  },
  
};

#define	NOPTIONS	(sizeof options / sizeof options[0])

static int
telnet_sub_option(packet_info *pinfo, proto_tree *telnet_tree, tvbuff_t *tvb, int start_offset)
{
  proto_tree *ti, *option_tree;
  int offset = start_offset;
  guint8 opt_byte;
  int subneg_len;
  const char *opt;
  gint ett;
  int iac_offset;
  guint len;
  void (*dissect)(packet_info *, const char *, tvbuff_t *, int, int, proto_tree *);
  gint cur_offset;
  gboolean iac_found;

  offset += 2;	/* skip IAC and SB */

  /* Get the option code */
  opt_byte = tvb_get_guint8(tvb, offset);
  if (opt_byte > NOPTIONS) {
    opt = "<unknown option>";
    ett = ett_telnet_subopt;
    dissect = NULL;
  } else {
    opt = options[opt_byte].name;
    if (options[opt_byte].subtree_index != NULL)
      ett = *(options[opt_byte].subtree_index);
    else
      ett = ett_telnet_subopt;
    dissect = options[opt_byte].dissect;
  }
  offset++;

  /* Search for an unescaped IAC. */
  cur_offset = offset;
  iac_found = FALSE;
  len = tvb_length_remaining(tvb, offset);
  do {
      iac_offset = tvb_find_guint8(tvb, cur_offset, len, TN_IAC);
      iac_found = TRUE;
      if (iac_offset == -1) {
        /* None found - run to the end of the packet. */
        offset += len;
      } else {
        if (((guint)(iac_offset + 1) >= len) ||
             (tvb_get_guint8(tvb, iac_offset + 1) != TN_IAC)) {
            /* We really found a single IAC, so we're done */
            offset = iac_offset;
        } else {
            /*
             * We saw an escaped IAC, so we have to move ahead to the
             * next section
             */
            iac_found = FALSE;
            cur_offset = iac_offset + 2;
        }
      }

  } while (!iac_found);
  
  subneg_len = offset - start_offset;

  ti = proto_tree_add_text(telnet_tree, tvb, start_offset, subneg_len,
                           "Suboption Begin: %s", opt);
  option_tree = proto_item_add_subtree(ti, ett);
  start_offset += 3;	/* skip IAC, SB, and option code */
  subneg_len -= 3;

  if (subneg_len > 0) {
    switch (options[opt_byte].len_type) {

    case NO_LENGTH:
      /* There isn't supposed to *be* sub-option negotiation for this. */
      proto_tree_add_text(option_tree, tvb, start_offset, subneg_len,
                          "Bogus suboption data");
      return offset;

    case FIXED_LENGTH:
      /* Make sure the length is what it's supposed to be. */
      if (subneg_len != options[opt_byte].optlen) {
        proto_tree_add_text(option_tree, tvb, start_offset, subneg_len,
                          "Suboption parameter length is %d, should be %d",
                          subneg_len, options[opt_byte].optlen);
        return offset;
      }
      break;

    case VARIABLE_LENGTH:
      /* Make sure the length is greater than the minimum. */
      if (subneg_len < options[opt_byte].optlen) {
        proto_tree_add_text(option_tree, tvb, start_offset, subneg_len,
                            "Suboption parameter length is %d, should be at least %d",
                            subneg_len, options[opt_byte].optlen);
        return offset;
      }
      break;
    }

    /* Now dissect the suboption parameters. */
    if (dissect != NULL) {
      /* We have a dissector for this suboption's parameters; call it. */
      (*dissect)(pinfo, opt, tvb, start_offset, subneg_len, option_tree);
    } else {
      /* We don't have a dissector for them; just show them as data. */
      proto_tree_add_text(option_tree, tvb, start_offset, subneg_len,
                          "Option data");
    }
  }
  return offset;
}

static int
telnet_will_wont_do_dont(proto_tree *telnet_tree, tvbuff_t *tvb,
			int start_offset, char *type)
{
  int offset = start_offset;
  guint8 opt_byte;
  const char *opt;

  offset += 2;	/* skip IAC and WILL,WONT,DO,DONT} */
  opt_byte = tvb_get_guint8(tvb, offset);
  if (opt_byte > NOPTIONS)
    opt = "<unknown option>";
  else
    opt = options[opt_byte].name;
  offset++;

  proto_tree_add_text(telnet_tree, tvb, start_offset, 3,
			"Command: %s %s", type, opt);
  return offset;
}

static int
telnet_command(packet_info *pinfo, proto_tree *telnet_tree, tvbuff_t *tvb, int start_offset)
{
  int offset = start_offset;
  guchar optcode;

  offset += 1;	/* skip IAC */
  optcode = tvb_get_guint8(tvb, offset);
  offset++;
  switch(optcode) {

  case TN_EOF:
    proto_tree_add_text(telnet_tree, tvb, start_offset, 2,
			"Command: End of File");
    break;

  case TN_SUSP:
    proto_tree_add_text(telnet_tree, tvb, start_offset, 2,
			"Command: Suspend Current Process");
    break;

  case TN_ABORT:
    proto_tree_add_text(telnet_tree, tvb, start_offset, 2,
			"Command: Abort Process");
    break;

  case TN_EOR:
    proto_tree_add_text(telnet_tree, tvb, start_offset, 2,
			"Command: End of Record");
    break;

  case TN_SE:
    proto_tree_add_text(telnet_tree, tvb, start_offset, 2,
			"Command: Suboption End");
    break;

  case TN_NOP:
    proto_tree_add_text(telnet_tree, tvb, start_offset, 2,
			"Command: No Operation");
    break;

  case TN_DM:
    proto_tree_add_text(telnet_tree, tvb, start_offset, 2,
			"Command: Data Mark");
    break;

  case TN_BRK:
    proto_tree_add_text(telnet_tree, tvb, start_offset, 2,
			"Command: Break");
    break;

  case TN_IP:
    proto_tree_add_text(telnet_tree, tvb, start_offset, 2,
			"Command: Interrupt Process");
    break;

  case TN_AO:
    proto_tree_add_text(telnet_tree, tvb, start_offset, 2,
			"Command: Abort Output");
    break;

  case TN_AYT:
    proto_tree_add_text(telnet_tree, tvb, start_offset, 2,
			"Command: Are You There?");
    break;

  case TN_EC:
    proto_tree_add_text(telnet_tree, tvb, start_offset, 2,
			"Command: Escape Character");
    break;

  case TN_EL:
    proto_tree_add_text(telnet_tree, tvb, start_offset, 2,
			"Command: Erase Line");
    break;

  case TN_GA:
    proto_tree_add_text(telnet_tree, tvb, start_offset, 2,
			"Command: Go Ahead");
    break;

  case TN_SB:
    offset = telnet_sub_option(pinfo, telnet_tree, tvb, start_offset);
    break;

  case TN_WILL:
    offset = telnet_will_wont_do_dont(telnet_tree, tvb, start_offset,
					"Will");
    break;

  case TN_WONT:
    offset = telnet_will_wont_do_dont(telnet_tree, tvb, start_offset,
					"Won't");
    break;

  case TN_DO:
    offset = telnet_will_wont_do_dont(telnet_tree, tvb, start_offset,
					"Do");
    break;

  case TN_DONT:
    offset = telnet_will_wont_do_dont(telnet_tree, tvb, start_offset,
					"Don't");
    break;

  default:
    proto_tree_add_text(telnet_tree, tvb, start_offset, 2,
			"Command: Unknown (0x%02x)", optcode);
    break;
  }

  return offset;
}

static void
telnet_add_text(proto_tree *tree, tvbuff_t *tvb, int offset, int len)
{
  gint next_offset;
  int linelen;
  guint8 c;
  gboolean last_char_was_cr;

  while (len != 0 && tvb_offset_exists(tvb, offset)) {
    /*
     * Find the end of the line.
     */
    linelen = tvb_find_line_end(tvb, offset, len, &next_offset, FALSE);
    len -= next_offset - offset;	/* subtract out the line's characters */

    /*
     * In Telnet, CR NUL is the way you send a CR by itself in the
     * default ASCII mode; don't treat CR by itself as a line ending,
     * treat only CR NUL, CR LF, or LF by itself as a line ending.
     */
    if (next_offset == offset + linelen + 1 && len >= 1) {
      /*
       * Well, we saw a one-character line ending, so either it's a CR
       * or an LF; we have at least two characters left, including the
       * CR.
       *
       * If the line ending is a CR, skip all subsequent CRs; at
       * least one capture appeared to have multiple CRs at the end of
       * a line.
       */
      if (tvb_get_guint8(tvb, offset + linelen) == '\r') {
      	last_char_was_cr = TRUE;
      	while (len != 0 && tvb_offset_exists(tvb, next_offset)) {
          c = tvb_get_guint8(tvb, next_offset);
      	  next_offset++;	/* skip over that character */
      	  len--;
          if (c == '\n' || (c == '\0' && last_char_was_cr)) {
            /*
	     * LF is a line ending, whether preceded by CR or not.
	     * NUL is a line ending if preceded by CR.
	     */
            break;
          }
      	  last_char_was_cr = (c == '\r');
      	}
      }
    }

    /*
     * Now compute the length of the line *including* the end-of-line
     * indication, if any; we display it all.
     */
    linelen = next_offset - offset;

    proto_tree_add_text(tree, tvb, offset, linelen,
			"Data: %s",
			tvb_format_text(tvb, offset, linelen));
    offset = next_offset;
  }
}

static void
dissect_telnet(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
        proto_tree      *telnet_tree, *ti;


	if (check_col(pinfo->cinfo, COL_PROTOCOL))
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "TELNET");

	if (check_col(pinfo->cinfo, COL_INFO))
		col_add_fstr(pinfo->cinfo, COL_INFO, "Telnet Data ...");

	if (tree) {
	  gint offset = 0;
	  guint len;
	  int data_len;
	  gint iac_offset;

	  ti = proto_tree_add_item(tree, proto_telnet, tvb, offset, -1, FALSE);
	  telnet_tree = proto_item_add_subtree(ti, ett_telnet);

	  /*
	   * Scan through the buffer looking for an IAC byte.
	   */
	  while ((len = tvb_length_remaining(tvb, offset)) > 0) {
	    iac_offset = tvb_find_guint8(tvb, offset, len, TN_IAC);
	    if (iac_offset != -1) {
	      /*
	       * We found an IAC byte.
	       * If there's any data before it, add that data to the
	       * tree, a line at a time.
	       */
	      data_len = iac_offset - offset;
	      if (data_len > 0)
	      	telnet_add_text(telnet_tree, tvb, offset, data_len);

	      /*
	       * Now interpret the command.
	       */
	      offset = telnet_command(pinfo, telnet_tree, tvb, iac_offset);
	    }
	    else {
	      /*
	       * We found no IAC byte, so what remains in the buffer
	       * is the last of the data in the packet.
	       * Add it to the tree, a line at a time, and then quit.
	       */
	      telnet_add_text(telnet_tree, tvb, offset, len);
	      break;
	    }
	  }
	}
}

void
proto_register_telnet(void)
{
	static hf_register_info hf[] = {  
	{ &hf_telnet_auth_name,
		{ "Name", "telnet.auth.name", FT_STRING, BASE_NONE,
		  NULL, 0, "Name of user being authenticated", HFILL }},
	{ &hf_telnet_auth_cmd,
		{ "Auth Cmd", "telnet.auth.cmd", FT_UINT8, BASE_DEC,
		  VALS(auth_cmd_vals), 0, "Authentication Command", HFILL }},
     	{ &hf_telnet_auth_type,
		{ "Auth Type", "telnet.auth.type", FT_UINT8, BASE_DEC,
		  VALS(auth_type_vals), 0, "Authentication Type", HFILL }},
       	{ &hf_telnet_auth_mod_cred_fwd,
		{ "Cred Fwd", "telnet.auth.mod.cred_fwd", FT_BOOLEAN, 8,
		  TFS(&auth_mod_cred_fwd), 0x08, "Modifier: Whether client will forward creds or not", HFILL }},
       	{ &hf_telnet_auth_mod_who,
		{ "Who", "telnet.auth.mod.who", FT_BOOLEAN, 8,
		  TFS(&auth_mod_who), 0x01, "Modifier: Who to mask", HFILL }},
       	{ &hf_telnet_auth_mod_how,
		{ "How", "telnet.auth.mod.how", FT_BOOLEAN, 8,
		  TFS(&auth_mod_how), 0x02, "Modifier: How to mask", HFILL }},
       	{ &hf_telnet_auth_mod_enc,
		{ "Encrypt", "telnet.auth.mod.enc", FT_UINT8, BASE_DEC,
		  VALS(auth_mod_enc), 0x14, "Modifier: How to enable Encryption", HFILL }},
       	{ &hf_telnet_auth_krb5_type,
		{ "Command", "telnet.auth.krb5.cmd", FT_UINT8, BASE_DEC,
		  VALS(auth_krb5_types), 0, "Krb5 Authentication sub-command", HFILL }},
		
        };
	static gint *ett[] = {
		&ett_telnet,
		&ett_telnet_subopt,
		&ett_status_subopt,
		&ett_rcte_subopt,
		&ett_olw_subopt,
		&ett_ops_subopt,
		&ett_crdisp_subopt,
		&ett_htstops_subopt,
		&ett_htdisp_subopt,
		&ett_ffdisp_subopt,
		&ett_vtstops_subopt,
		&ett_vtdisp_subopt,
		&ett_lfdisp_subopt,
		&ett_extasc_subopt,
		&ett_bytemacro_subopt,
		&ett_det_subopt,
		&ett_supdupout_subopt,
		&ett_sendloc_subopt,
		&ett_termtype_subopt,
		&ett_tacacsui_subopt,
		&ett_outmark_subopt,
		&ett_tlocnum_subopt,
		&ett_tn3270reg_subopt,
		&ett_x3pad_subopt,
		&ett_naws_subopt,
		&ett_tspeed_subopt,
		&ett_rfc_subopt,
		&ett_linemode_subopt,
		&ett_xdpyloc_subopt,
		&ett_env_subopt,
		&ett_auth_subopt,
		&ett_enc_subopt,
		&ett_newenv_subopt,
		&ett_tn3270e_subopt,
		&ett_xauth_subopt,
		&ett_charset_subopt,
		&ett_rsp_subopt,
		&ett_comport_subopt,
	};

        proto_telnet = proto_register_protocol("Telnet", "TELNET", "telnet");
        proto_register_field_array(proto_telnet, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));
}

void
proto_reg_handoff_telnet(void)
{
	dissector_handle_t telnet_handle;

	telnet_handle = create_dissector_handle(dissect_telnet, proto_telnet);
	dissector_add("tcp.port", TCP_PORT_TELNET, telnet_handle);
}
