/* packet-rpc.c
 * Routines for rpc dissection
 * Copyright 1999, Uwe Girlich <Uwe.Girlich@philosys.de>
 *
 * $Id: packet-rpc.c,v 1.108 2002/11/13 21:45:56 guy Exp $
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@ethereal.com>
 * Copyright 1998 Gerald Combs
 *
 * Copied from packet-smb.c
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

#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <epan/packet.h>
#include <epan/conversation.h>
#include "packet-rpc.h"
#include "packet-frame.h"
#include "packet-tcp.h"
#include "prefs.h"
#include "reassemble.h"
#include "rpc_defrag.h"
#include "packet-nfs.h"
#include "tap.h"

/*
 * See:
 *
 *	RFC 1831, "RPC: Remote Procedure Call Protocol Specification
 *	Version 2";
 *
 *	RFC 1832, "XDR: External Data Representation Standard";
 *
 *	RFC 2203, "RPCSEC_GSS Protocol Specification".
 *
 * See also
 *
 *	RFC 2695, "Authentication Mechanisms for ONC RPC"
 *
 *	although we don't currently dissect AUTH_DES or AUTH_KERB.
 */

/* desegmentation of RPC over TCP */
static gboolean rpc_desegment = TRUE;

/* defragmentation of fragmented RPC over TCP records */
static gboolean rpc_defragment = FALSE;

static struct true_false_string yesno = { "Yes", "No" };

static int rpc_tap = -1;

static const value_string rpc_msg_type[] = {
	{ RPC_CALL, "Call" },
	{ RPC_REPLY, "Reply" },
	{ 0, NULL }
};

static const value_string rpc_reply_state[] = {
	{ MSG_ACCEPTED, "accepted" },
	{ MSG_DENIED, "denied" },
	{ 0, NULL }
};

const value_string rpc_auth_flavor[] = {
	{ AUTH_NULL, "AUTH_NULL" },
	{ AUTH_UNIX, "AUTH_UNIX" },
	{ AUTH_SHORT, "AUTH_SHORT" },
	{ AUTH_DES, "AUTH_DES" },
	{ RPCSEC_GSS, "RPCSEC_GSS" },
	{ 0, NULL }
};

static const value_string rpc_authgss_proc[] = {
	{ RPCSEC_GSS_DATA, "RPCSEC_GSS_DATA" },
	{ RPCSEC_GSS_INIT, "RPCSEC_GSS_INIT" },
	{ RPCSEC_GSS_CONTINUE_INIT, "RPCSEC_GSS_CONTINUE_INIT" },
	{ RPCSEC_GSS_DESTROY, "RPCSEC_GSS_DESTROY" },
	{ 0, NULL }
};

value_string rpc_authgss_svc[] = {
	{ RPCSEC_GSS_SVC_NONE, "rpcsec_gss_svc_none" },
	{ RPCSEC_GSS_SVC_INTEGRITY, "rpcsec_gss_svc_integrity" },
	{ RPCSEC_GSS_SVC_PRIVACY, "rpcsec_gss_svc_privacy" },
	{ 0, NULL }
};

static const value_string rpc_accept_state[] = {
	{ SUCCESS, "RPC executed successfully" },
	{ PROG_UNAVAIL, "remote hasn't exported program" },
	{ PROG_MISMATCH, "remote can't support version #" },
	{ PROC_UNAVAIL, "program can't support procedure" },
	{ GARBAGE_ARGS, "procedure can't decode params" },
	{ 0, NULL }
};

static const value_string rpc_reject_state[] = {
	{ RPC_MISMATCH, "RPC_MISMATCH" },
	{ AUTH_ERROR, "AUTH_ERROR" },
	{ 0, NULL }
};

static const value_string rpc_auth_state[] = {
	{ AUTH_BADCRED, "bad credential (seal broken)" },
	{ AUTH_REJECTEDCRED, "client must begin new session" },
	{ AUTH_BADVERF, "bad verifier (seal broken)" },
	{ AUTH_REJECTEDVERF, "verifier expired or replayed" },
	{ AUTH_TOOWEAK, "rejected for security reasons" },
	{ RPCSEC_GSSCREDPROB, "GSS credential problem" },
	{ RPCSEC_GSSCTXPROB, "GSS context problem" },
	{ 0, NULL }
};

static const value_string rpc_authdes_namekind[] = {
	{ AUTHDES_NAMEKIND_FULLNAME, "ADN_FULLNAME" },
	{ AUTHDES_NAMEKIND_NICKNAME, "ADN_NICKNAME" },
	{ 0, NULL }
};

/* the protocol number */
static int proto_rpc = -1;
static int hf_rpc_lastfrag = -1;
static int hf_rpc_fraglen = -1;
static int hf_rpc_xid = -1;
static int hf_rpc_msgtype = -1;
static int hf_rpc_version = -1;
static int hf_rpc_version_min = -1;
static int hf_rpc_version_max = -1;
static int hf_rpc_program = -1;
static int hf_rpc_programversion = -1;
static int hf_rpc_programversion_min = -1;
static int hf_rpc_programversion_max = -1;
static int hf_rpc_procedure = -1;
static int hf_rpc_auth_flavor = -1;
static int hf_rpc_auth_length = -1;
static int hf_rpc_auth_machinename = -1;
static int hf_rpc_auth_stamp = -1;
static int hf_rpc_auth_uid = -1;
static int hf_rpc_auth_gid = -1;
static int hf_rpc_authgss_v = -1;
static int hf_rpc_authgss_proc = -1;
static int hf_rpc_authgss_seq = -1;
static int hf_rpc_authgss_svc = -1;
static int hf_rpc_authgss_ctx = -1;
static int hf_rpc_authgss_major = -1;
static int hf_rpc_authgss_minor = -1;
static int hf_rpc_authgss_window = -1;
static int hf_rpc_authgss_token_length = -1;
static int hf_rpc_authgss_data_length = -1;
static int hf_rpc_authgss_data = -1;
static int hf_rpc_authgss_checksum = -1;
static int hf_rpc_authdes_namekind = -1;
static int hf_rpc_authdes_netname = -1;
static int hf_rpc_authdes_convkey = -1;
static int hf_rpc_authdes_window = -1;
static int hf_rpc_authdes_nickname = -1;
static int hf_rpc_authdes_timestamp = -1;
static int hf_rpc_authdes_windowverf = -1;
static int hf_rpc_authdes_timeverf = -1;
static int hf_rpc_state_accept = -1;
static int hf_rpc_state_reply = -1;
static int hf_rpc_state_reject = -1;
static int hf_rpc_state_auth = -1;
static int hf_rpc_dup = -1;
static int hf_rpc_call_dup = -1;
static int hf_rpc_reply_dup = -1;
static int hf_rpc_value_follows = -1;
static int hf_rpc_array_len = -1;
static int hf_rpc_time = -1;
static int hf_rpc_fragments = -1;
static int hf_rpc_fragment = -1;
static int hf_rpc_fragment_overlap = -1;
static int hf_rpc_fragment_overlap_conflict = -1;
static int hf_rpc_fragment_multiple_tails = -1;
static int hf_rpc_fragment_too_long_fragment = -1;
static int hf_rpc_fragment_error = -1;

static gint ett_rpc = -1;
static gint ett_rpc_fragments = -1;
static gint ett_rpc_fragment = -1;
static gint ett_rpc_fraghdr = -1;
static gint ett_rpc_string = -1;
static gint ett_rpc_cred = -1;
static gint ett_rpc_verf = -1;
static gint ett_rpc_gids = -1;
static gint ett_rpc_gss_token = -1;
static gint ett_rpc_gss_data = -1;
static gint ett_rpc_array = -1;

static dissector_handle_t rpc_tcp_handle;
static dissector_handle_t rpc_handle;
static dissector_handle_t gssapi_handle;
static dissector_handle_t data_handle;

static const fragment_items rpc_frag_items = {
	&ett_rpc_fragment,
	&ett_rpc_fragments,
	&hf_rpc_fragments,
	&hf_rpc_fragment,
	&hf_rpc_fragment_overlap,
	&hf_rpc_fragment_overlap_conflict,
	&hf_rpc_fragment_multiple_tails,
	&hf_rpc_fragment_too_long_fragment,
	&hf_rpc_fragment_error,
	"fragments"
};

/* Hash table with info on RPC program numbers */
GHashTable *rpc_progs;

/* Hash table with info on RPC procedure numbers */
GHashTable *rpc_procs;

static void dissect_rpc(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);
static void dissect_rpc_tcp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);

/***********************************/
/* Hash array with procedure names */
/***********************************/

/* compare 2 keys */
static gint
rpc_proc_equal(gconstpointer k1, gconstpointer k2)
{
	rpc_proc_info_key* key1 = (rpc_proc_info_key*) k1;
	rpc_proc_info_key* key2 = (rpc_proc_info_key*) k2;

	return ((key1->prog == key2->prog &&
		key1->vers == key2->vers &&
		key1->proc == key2->proc) ?
	TRUE : FALSE);
}

/* calculate a hash key */
static guint
rpc_proc_hash(gconstpointer k)
{
	rpc_proc_info_key* key = (rpc_proc_info_key*) k;

	return (key->prog ^ (key->vers<<16) ^ (key->proc<<24));
}


/* insert some entries */
void
rpc_init_proc_table(guint prog, guint vers, const vsff *proc_table,
    int procedure_hf)
{
	rpc_prog_info_key rpc_prog_key;
	rpc_prog_info_value *rpc_prog;
	const vsff *proc;

	/*
	 * Add the operation number hfinfo value for this version of the
	 * program.
	 */
	rpc_prog_key.prog = prog;
	rpc_prog = g_hash_table_lookup(rpc_progs, &rpc_prog_key);
	g_assert(rpc_prog != NULL);
	rpc_prog->procedure_hfs = g_array_set_size(rpc_prog->procedure_hfs,
	    vers);
	g_array_insert_val(rpc_prog->procedure_hfs, vers, procedure_hf);

	for (proc = proc_table ; proc->strptr!=NULL; proc++) {
		rpc_proc_info_key *key;
		rpc_proc_info_value *value;

		key = (rpc_proc_info_key *) g_malloc(sizeof(rpc_proc_info_key));
		key->prog = prog;
		key->vers = vers;
		key->proc = proc->value;

		value = (rpc_proc_info_value *) g_malloc(sizeof(rpc_proc_info_value));
		value->name = proc->strptr;
		value->dissect_call = proc->dissect_call;
		value->dissect_reply = proc->dissect_reply;

		g_hash_table_insert(rpc_procs,key,value);
	}
}

/*	return the name associated with a previously registered procedure. */
char *rpc_proc_name(guint32 prog, guint32 vers, guint32 proc)
{
	rpc_proc_info_key key;
	rpc_proc_info_value *value;
	char *procname;
	static char procname_static[20];

	key.prog = prog;
	key.vers = vers;
	key.proc = proc;

	if ((value = g_hash_table_lookup(rpc_procs,&key)) != NULL)
		procname = value->name;
	else {
		/* happens only with strange program versions or
		   non-existing dissectors */
		sprintf(procname_static, "proc-%u", key.proc);
		procname = procname_static;
	}
	return procname;
}

/*----------------------------------------*/
/* end of Hash array with procedure names */
/*----------------------------------------*/


/*********************************/
/* Hash array with program names */
/*********************************/

/* compare 2 keys */
static gint
rpc_prog_equal(gconstpointer k1, gconstpointer k2)
{
	rpc_prog_info_key* key1 = (rpc_prog_info_key*) k1;
	rpc_prog_info_key* key2 = (rpc_prog_info_key*) k2;

	return ((key1->prog == key2->prog) ?
	TRUE : FALSE);
}


/* calculate a hash key */
static guint
rpc_prog_hash(gconstpointer k)
{
	rpc_prog_info_key* key = (rpc_prog_info_key*) k;

	return (key->prog);
}


void
rpc_init_prog(int proto, guint32 prog, int ett)
{
	rpc_prog_info_key *key;
	rpc_prog_info_value *value;

	key = (rpc_prog_info_key *) g_malloc(sizeof(rpc_prog_info_key));
	key->prog = prog;

	value = (rpc_prog_info_value *) g_malloc(sizeof(rpc_prog_info_value));
	value->proto = proto;
	value->ett = ett;
	value->progname = proto_get_protocol_short_name(proto);
	value->procedure_hfs = g_array_new(FALSE, TRUE, sizeof (int));

	g_hash_table_insert(rpc_progs,key,value);
}

/*	return the name associated with a previously registered program. This
	should probably eventually be expanded to use the rpc YP/NIS map
	so that it can give names for programs not handled by ethereal */
char *rpc_prog_name(guint32 prog)
{
	char *progname = NULL;
	rpc_prog_info_key       rpc_prog_key;
	rpc_prog_info_value     *rpc_prog;

	rpc_prog_key.prog = prog;
	if ((rpc_prog = g_hash_table_lookup(rpc_progs,&rpc_prog_key)) == NULL) {
		progname = "Unknown";
	}
	else {
		progname = rpc_prog->progname;
	}
	return progname;
}


/*--------------------------------------*/
/* end of Hash array with program names */
/*--------------------------------------*/

typedef struct _rpc_call_info_key {
	guint32	xid;
	conversation_t *conversation;
} rpc_call_info_key;

static GMemChunk *rpc_call_info_key_chunk;

static GMemChunk *rpc_call_info_value_chunk;

static GHashTable *rpc_calls;

static GHashTable *rpc_indir_calls;

/* compare 2 keys */
static gint
rpc_call_equal(gconstpointer k1, gconstpointer k2)
{
	rpc_call_info_key* key1 = (rpc_call_info_key*) k1;
	rpc_call_info_key* key2 = (rpc_call_info_key*) k2;

	return (key1->xid == key2->xid &&
	    key1->conversation == key2->conversation);
}


/* calculate a hash key */
static guint
rpc_call_hash(gconstpointer k)
{
	rpc_call_info_key* key = (rpc_call_info_key*) k;

	return key->xid + (guint32)(key->conversation);
}


unsigned int
rpc_roundup(unsigned int a)
{
	unsigned int mod = a % 4;
	return a + ((mod)? 4-mod : 0);
}


int
dissect_rpc_bool(tvbuff_t *tvb, proto_tree *tree,
int hfindex, int offset)
{
	if (tree)
		proto_tree_add_item(tree, hfindex, tvb, offset, 4, FALSE);
	return offset + 4;
}


int
dissect_rpc_uint32(tvbuff_t *tvb, proto_tree *tree,
int hfindex, int offset)
{
	if (tree)
		proto_tree_add_item(tree, hfindex, tvb, offset, 4, FALSE);
	return offset + 4;
}


int
dissect_rpc_uint64(tvbuff_t *tvb, proto_tree *tree,
int hfindex, int offset)
{
	header_field_info	*hfinfo;

	hfinfo = proto_registrar_get_nth(hfindex);
	g_assert(hfinfo->type == FT_UINT64);
	if (tree)
		proto_tree_add_item(tree, hfindex, tvb, offset, 8, FALSE);

	return offset + 8;
}


static int
dissect_rpc_opaque_data(tvbuff_t *tvb, int offset,
    proto_tree *tree, int hfindex,
    gboolean fixed_length, guint32 length,
    gboolean string_data, char **string_buffer_ret)
{
	proto_item *string_item = NULL;
	proto_tree *string_tree = NULL;
	int old_offset = offset;

	guint32 string_length;
	guint32 string_length_full;
	guint32 string_length_packet;
	guint32 string_length_captured;
	guint32 string_length_copy;

	int fill_truncated;
	guint32 fill_length;
	guint32 fill_length_packet;
	guint32 fill_length_captured;
	guint32 fill_length_copy;

	int exception = 0;

	char *string_buffer = NULL;
	char *string_buffer_print = NULL;

	if (fixed_length) {
		string_length = length;
		string_length_captured = tvb_length_remaining(tvb, offset);
		string_length_packet = tvb_reported_length_remaining(tvb, offset);
	}
	else {
	string_length = tvb_get_ntohl(tvb,offset+0);
	string_length_captured = tvb_length_remaining(tvb, offset + 4);
	string_length_packet = tvb_reported_length_remaining(tvb, offset + 4);
	}
	string_length_full = rpc_roundup(string_length);
	if (string_length_captured < string_length) {
		/* truncated string */
		string_length_copy = string_length_captured;
		fill_truncated = 2;
		fill_length = 0;
		fill_length_copy = 0;
		if (string_length_packet < string_length)
			exception = ReportedBoundsError;
		else
			exception = BoundsError;
	}
	else {
		/* full string data */
		string_length_copy = string_length;
		fill_length = string_length_full - string_length;
		if (fixed_length) {
			fill_length_captured = tvb_length_remaining(tvb,
			    offset + string_length);
			fill_length_packet = tvb_reported_length_remaining(tvb,
			    offset + string_length);
		}
		else {
		fill_length_captured = tvb_length_remaining(tvb,
		    offset + 4 + string_length);
		fill_length_packet = tvb_reported_length_remaining(tvb,
		    offset + 4 + string_length);
		}
		if (fill_length_captured < fill_length) {
			/* truncated fill bytes */
			fill_length_copy = fill_length_packet;
			fill_truncated = 1;
			if (fill_length_packet < fill_length)
				exception = ReportedBoundsError;
			else
				exception = BoundsError;
		}
		else {
			/* full fill bytes */
			fill_length_copy = fill_length;
			fill_truncated = 0;
		}
	}
	string_buffer = (char*)g_malloc(string_length_copy +
			(string_data ? 1 : 0));
	if (fixed_length)
		tvb_memcpy(tvb,string_buffer, offset, string_length_copy);
	else
	tvb_memcpy(tvb,string_buffer,offset+4,string_length_copy);
	if (string_data)
		string_buffer[string_length_copy] = '\0';

	/* calculate a nice printable string */
	if (string_length) {
		if (string_length != string_length_copy) {
			if (string_data) {
				/* alloc maximum data area */
				string_buffer_print = (char*)g_malloc(string_length_copy + 12 + 1);
				/* copy over the data */
				memcpy(string_buffer_print,string_buffer,string_length_copy);
				/* append a 0 byte for sure printing */
				string_buffer_print[string_length_copy] = '\0';
				/* append <TRUNCATED> */
				/* This way, we get the TRUNCATED even
				   in the case of totally wrong packets,
				   where \0 are inside the string.
				   TRUNCATED will appear at the
				   first \0 or at the end (where we
				   put the securing \0).
				*/
				strcat(string_buffer_print,"<TRUNCATED>");
			}
			else {
				string_buffer_print = g_strdup("<DATA><TRUNCATED>");
			}
		}
		else {
			if (string_data) {
				string_buffer_print = g_strdup(string_buffer);
			}
			else {
				string_buffer_print = g_strdup("<DATA>");
			}
		}
	}
	else {
		string_buffer_print = g_strdup("<EMPTY>");
	}

	if (tree) {
		string_item = proto_tree_add_text(tree, tvb,offset+0, -1,
			"%s: %s", proto_registrar_get_name(hfindex), string_buffer_print);
		if (string_data) {
			proto_tree_add_string_hidden(tree, hfindex, tvb, 
				(fixed_length)? offset : offset + 4,
				string_length_copy, string_buffer);
		}
		if (string_item) {
			string_tree = proto_item_add_subtree(string_item, ett_rpc_string);
		}
	}
	if (!fixed_length) {
	if (string_tree)
		proto_tree_add_text(string_tree, tvb,offset+0,4,
			"length: %u", string_length);
	offset += 4;
	}

	if (string_tree) {
		if (string_data) {
			proto_tree_add_string_format(string_tree,
			    hfindex, tvb, offset, string_length_copy,
				string_buffer_print,
				"contents: %s", string_buffer_print);
		} else {
			proto_tree_add_bytes_format(string_tree,
			    hfindex, tvb, offset, string_length_copy,
				string_buffer_print,
				"contents: %s", string_buffer_print);
		}
	}
	offset += string_length_copy;
	if (fill_length) {
		if (string_tree) {
			if (fill_truncated) {
				proto_tree_add_text(string_tree, tvb,
				offset,fill_length_copy,
				"fill bytes: opaque data<TRUNCATED>");
			}
			else {
				proto_tree_add_text(string_tree, tvb,
				offset,fill_length_copy,
				"fill bytes: opaque data");
			}
		}
		offset += fill_length_copy;
	}

	if (string_item) {
		proto_item_set_len(string_item, offset - old_offset);
	}

	if (string_buffer       != NULL) g_free (string_buffer      );
	if (string_buffer_print != NULL) {
		if (string_buffer_ret != NULL)
			*string_buffer_ret = string_buffer_print;
		else
			g_free (string_buffer_print);
	}

	/*
	 * If the data was truncated, throw the appropriate exception,
	 * so that dissection stops and the frame is properly marked.
	 */
	if (exception != 0)
		THROW(exception);
	return offset;
}


int
dissect_rpc_string(tvbuff_t *tvb, proto_tree *tree,
    int hfindex, int offset, char **string_buffer_ret)
{
	offset = dissect_rpc_opaque_data(tvb, offset, tree,
	    hfindex, FALSE, 0, TRUE, string_buffer_ret);
	return offset;
}


int
dissect_rpc_data(tvbuff_t *tvb, proto_tree *tree,
    int hfindex, int offset)
{
	offset = dissect_rpc_opaque_data(tvb, offset, tree,
	    hfindex, FALSE, 0, FALSE, NULL);
	return offset;
}


int
dissect_rpc_bytes(tvbuff_t *tvb, proto_tree *tree,
    int hfindex, int offset, guint32 length,
    gboolean string_data, char **string_buffer_ret)
{
	offset = dissect_rpc_opaque_data(tvb, offset, tree,
	    hfindex, TRUE, length, string_data, string_buffer_ret);
	return offset;
}


int
dissect_rpc_list(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
	int offset, dissect_function_t *rpc_list_dissector)
{
	guint32 value_follows;

	while (1) {
		value_follows = tvb_get_ntohl(tvb, offset+0);
		proto_tree_add_boolean(tree,hf_rpc_value_follows, tvb,
			offset+0, 4, value_follows);
		offset += 4;
		if (value_follows == 1) {
			offset = rpc_list_dissector(tvb, offset, pinfo, tree);
		}
		else {
			break;
		}
	}

	return offset;
}

int
dissect_rpc_array(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
	int offset, dissect_function_t *rpc_array_dissector,
	int hfindex)
{
	proto_item* lock_item;
	proto_tree* lock_tree;
	guint32	num;
	int old_offset = offset;

	num = tvb_get_ntohl(tvb, offset);

	if( num == 0 ){
		proto_tree_add_none_format(tree, hfindex, tvb, offset, 4,
			"no values");
		offset += 4;

		return offset;
	}

	lock_item = proto_tree_add_item(tree, hfindex, tvb, offset, -1, FALSE);

	lock_tree = proto_item_add_subtree(lock_item, ett_rpc_array);

	offset = dissect_rpc_uint32(tvb, lock_tree,
			hf_rpc_array_len, offset);

	while (num--) {
		offset = rpc_array_dissector(tvb, offset, pinfo, lock_tree);
	}

	proto_item_set_len(lock_item, offset-old_offset);
	return offset;
}

static int
dissect_rpc_authunix_cred(tvbuff_t* tvb, proto_tree* tree, int offset)
{
	guint stamp;
	guint uid;
	guint gid;
	guint gids_count;
	guint gids_i;
	guint gids_entry;
	proto_item *gitem;
	proto_tree *gtree = NULL;

	stamp = tvb_get_ntohl(tvb,offset+0);
	if (tree)
		proto_tree_add_uint(tree, hf_rpc_auth_stamp, tvb,
			offset+0, 4, stamp);
	offset += 4;

	offset = dissect_rpc_string(tvb, tree,
			hf_rpc_auth_machinename, offset, NULL);

	uid = tvb_get_ntohl(tvb,offset+0);
	if (tree)
		proto_tree_add_uint(tree, hf_rpc_auth_uid, tvb,
			offset+0, 4, uid);
	offset += 4;

	gid = tvb_get_ntohl(tvb,offset+0);
	if (tree)
		proto_tree_add_uint(tree, hf_rpc_auth_gid, tvb,
			offset+0, 4, gid);
	offset += 4;

	gids_count = tvb_get_ntohl(tvb,offset+0);
	if (tree) {
		gitem = proto_tree_add_text(tree, tvb,
			offset, 4+gids_count*4, "Auxiliary GIDs");
		gtree = proto_item_add_subtree(gitem, ett_rpc_gids);
	}
	offset += 4;

	for (gids_i = 0 ; gids_i < gids_count ; gids_i++) {
		gids_entry = tvb_get_ntohl(tvb,offset+0);
		if (gtree)
		proto_tree_add_uint(gtree, hf_rpc_auth_gid, tvb,
			offset, 4, gids_entry);
		offset+=4;
	}
	/* how can I NOW change the gitem to print a list with
		the first 16 gids? */

	return offset;
}

static int
dissect_rpc_authgss_cred(tvbuff_t* tvb, proto_tree* tree, int offset)
{
	guint agc_v;
	guint agc_proc;
	guint agc_seq;
	guint agc_svc;

	agc_v = tvb_get_ntohl(tvb, offset+0);
	if (tree)
		proto_tree_add_uint(tree, hf_rpc_authgss_v,
				    tvb, offset+0, 4, agc_v);
	offset += 4;

	agc_proc = tvb_get_ntohl(tvb, offset+0);
	if (tree)
		proto_tree_add_uint(tree, hf_rpc_authgss_proc,
				    tvb, offset+0, 4, agc_proc);
	offset += 4;

	agc_seq = tvb_get_ntohl(tvb, offset+0);
	if (tree)
		proto_tree_add_uint(tree, hf_rpc_authgss_seq,
				    tvb, offset+0, 4, agc_seq);
	offset += 4;

	agc_svc = tvb_get_ntohl(tvb, offset+0);
	if (tree)
		proto_tree_add_uint(tree, hf_rpc_authgss_svc,
				    tvb, offset+0, 4, agc_svc);
	offset += 4;

	offset = dissect_rpc_data(tvb, tree, hf_rpc_authgss_ctx,
			offset);

	return offset;
}

static int
dissect_rpc_authdes_desblock(tvbuff_t *tvb, proto_tree *tree,
int hfindex, int offset)
{
	guint32 value_low;
	guint32 value_high;

	value_high = tvb_get_ntohl(tvb, offset + 0);
	value_low  = tvb_get_ntohl(tvb, offset + 4);

	if (tree) {
		proto_tree_add_text(tree, tvb, offset, 8,
			"%s: 0x%x%08x", proto_registrar_get_name(hfindex), value_high,
			value_low);
	}

	return offset + 8;
}

static int
dissect_rpc_authdes_cred(tvbuff_t* tvb, proto_tree* tree, int offset)
{
	guint adc_namekind;
	guint window = 0;
	guint nickname = 0;

	adc_namekind = tvb_get_ntohl(tvb, offset+0);
	if (tree)
		proto_tree_add_uint(tree, hf_rpc_authdes_namekind,
				    tvb, offset+0, 4, adc_namekind);
	offset += 4;

	switch(adc_namekind)
	{
	case AUTHDES_NAMEKIND_FULLNAME:
		offset = dissect_rpc_string(tvb, tree,
			hf_rpc_authdes_netname, offset, NULL);
		offset = dissect_rpc_authdes_desblock(tvb, tree,
			hf_rpc_authdes_convkey, offset);
		window = tvb_get_ntohl(tvb, offset+0);
		proto_tree_add_uint(tree, hf_rpc_authdes_window, tvb, offset+0, 4,
			window);
		offset += 4;
		break;

	case AUTHDES_NAMEKIND_NICKNAME:
		nickname = tvb_get_ntohl(tvb, offset+0);
		proto_tree_add_uint(tree, hf_rpc_authdes_nickname, tvb, offset+0, 4,
			window);
		offset += 4;
		break;
	}

	return offset;
}

static int
dissect_rpc_cred(tvbuff_t* tvb, proto_tree* tree, int offset)
{
	guint flavor;
	guint length;

	proto_item *citem;
	proto_tree *ctree;

	flavor = tvb_get_ntohl(tvb,offset+0);
	length = tvb_get_ntohl(tvb,offset+4);
	length = rpc_roundup(length);

	if (tree) {
		citem = proto_tree_add_text(tree, tvb, offset,
					    8+length, "Credentials");
		ctree = proto_item_add_subtree(citem, ett_rpc_cred);
		proto_tree_add_uint(ctree, hf_rpc_auth_flavor, tvb,
				    offset+0, 4, flavor);
		proto_tree_add_uint(ctree, hf_rpc_auth_length, tvb,
				    offset+4, 4, length);

		switch (flavor) {
		case AUTH_UNIX:
			dissect_rpc_authunix_cred(tvb, ctree, offset+8);
			break;
		/*
		case AUTH_SHORT:

		break;
		*/
		case AUTH_DES:
			dissect_rpc_authdes_cred(tvb, ctree, offset+8);
			break;

		case RPCSEC_GSS:
			dissect_rpc_authgss_cred(tvb, ctree, offset+8);
			break;
		default:
			if (length)
				proto_tree_add_text(ctree, tvb, offset+8,
						    length,"opaque data");
		break;
		}
	}
	offset += 8 + length;

	return offset;
}

/* AUTH_DES verifiers are asymmetrical, so we need to know what type of
 * verifier we're decoding (CALL or REPLY).
 */
static int
dissect_rpc_verf(tvbuff_t* tvb, proto_tree* tree, int offset, int msg_type)
{
	guint flavor;
	guint length;

	proto_item *vitem;
	proto_tree *vtree;

	flavor = tvb_get_ntohl(tvb,offset+0);
	length = tvb_get_ntohl(tvb,offset+4);
	length = rpc_roundup(length);

	if (tree) {
		vitem = proto_tree_add_text(tree, tvb, offset,
					    8+length, "Verifier");
		vtree = proto_item_add_subtree(vitem, ett_rpc_verf);
		proto_tree_add_uint(vtree, hf_rpc_auth_flavor, tvb,
				    offset+0, 4, flavor);

		switch (flavor) {
		case AUTH_UNIX:
			proto_tree_add_uint(vtree, hf_rpc_auth_length, tvb,
					    offset+4, 4, length);
			dissect_rpc_authunix_cred(tvb, vtree, offset+8);
			break;
		case AUTH_DES:
			proto_tree_add_uint(vtree, hf_rpc_auth_length, tvb,
				offset+4, 4, length);

			if (msg_type == RPC_CALL)
			{
				guint window;

				dissect_rpc_authdes_desblock(tvb, vtree,
					hf_rpc_authdes_timestamp, offset+8);
				window = tvb_get_ntohl(tvb, offset+16);
				proto_tree_add_uint(vtree, hf_rpc_authdes_windowverf, tvb,
					offset+16, 4, window);
			}
			else
			{
				/* must be an RPC_REPLY */
				guint nickname;

				dissect_rpc_authdes_desblock(tvb, vtree,
					hf_rpc_authdes_timeverf, offset+8);
				nickname = tvb_get_ntohl(tvb, offset+16);
				proto_tree_add_uint(vtree, hf_rpc_authdes_nickname, tvb,
				 	offset+16, 4, nickname);
			}
			break;
		case RPCSEC_GSS:
			dissect_rpc_data(tvb, vtree,
				hf_rpc_authgss_checksum, offset+4);
			break;
		default:
			proto_tree_add_uint(vtree, hf_rpc_auth_length, tvb,
					    offset+4, 4, length);
			if (length)
				proto_tree_add_text(vtree, tvb, offset+8,
						    length, "opaque data");
			break;
		}
	}
	offset += 8 + length;

	return offset;
}

/*
 * XDR opaque object, the contents of which are interpreted as a GSS-API
 * token.
 */
static int
dissect_rpc_authgss_token(tvbuff_t* tvb, proto_tree* tree, int offset,
    packet_info *pinfo)
{
	guint32 opaque_length, rounded_length;
	gint length, reported_length;
	tvbuff_t *new_tvb;

	proto_item *gitem;
	proto_tree *gtree = NULL;

	opaque_length = tvb_get_ntohl(tvb, offset+0);
	rounded_length = rpc_roundup(opaque_length);
	if (tree) {
		gitem = proto_tree_add_text(tree, tvb, offset,
					    4+rounded_length, "GSS Token");
		gtree = proto_item_add_subtree(gitem, ett_rpc_gss_token);
		proto_tree_add_uint(gtree, hf_rpc_authgss_token_length,
				    tvb, offset+0, 4, opaque_length);
	}
	offset += 4;
	length = tvb_length_remaining(tvb, offset);
	reported_length = tvb_reported_length_remaining(tvb, offset);
	g_assert(length >= 0);
	g_assert(reported_length >= 0);
	if (length > reported_length)
		length = reported_length;
	if ((guint32)length > opaque_length)
		length = opaque_length;
	if ((guint32)reported_length > opaque_length)
		reported_length = opaque_length;
	new_tvb = tvb_new_subset(tvb, offset, length, reported_length);
	call_dissector(gssapi_handle, new_tvb, pinfo, gtree);
	offset += rounded_length;

	return offset;
}

static int
dissect_rpc_authgss_initarg(tvbuff_t* tvb, proto_tree* tree, int offset,
    packet_info *pinfo)
{
	return dissect_rpc_authgss_token(tvb, tree, offset, pinfo);
}

static int
dissect_rpc_authgss_initres(tvbuff_t* tvb, proto_tree* tree, int offset,
    packet_info *pinfo)
{
	int major, minor, window;

	offset = dissect_rpc_data(tvb, tree, hf_rpc_authgss_ctx,
			offset);

	major = tvb_get_ntohl(tvb,offset+0);
	if (tree)
		proto_tree_add_uint(tree, hf_rpc_authgss_major, tvb,
				    offset+0, 4, major);
	offset += 4;

	minor = tvb_get_ntohl(tvb,offset+0);
	if (tree)
		proto_tree_add_uint(tree, hf_rpc_authgss_minor, tvb,
				    offset+0, 4, minor);
	offset += 4;

	window = tvb_get_ntohl(tvb,offset+0);
	if (tree)
		proto_tree_add_uint(tree, hf_rpc_authgss_window, tvb,
				    offset+0, 4, window);
	offset += 4;

	offset = dissect_rpc_authgss_token(tvb, tree, offset, pinfo);

	return offset;
}


static int
call_dissect_function(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
	int offset, dissect_function_t* dissect_function, const char *progname)
{
	const char *saved_proto;

	if (dissect_function != NULL) {
		/* set the current protocol name */
		saved_proto = pinfo->current_proto;
		if (progname != NULL)
			pinfo->current_proto = progname;

		/* call the dissector for the next level */
		offset = dissect_function(tvb, offset, pinfo, tree);

		/* restore the protocol name */
		pinfo->current_proto = saved_proto;
	}

	return offset;
}


static int
dissect_rpc_authgss_integ_data(tvbuff_t *tvb, packet_info *pinfo,
	proto_tree *tree, int offset,
	dissect_function_t* dissect_function,
	const char *progname)
{
	guint32 length, rounded_length, seq;

	proto_item *gitem;
	proto_tree *gtree = NULL;

	length = tvb_get_ntohl(tvb, offset+0);
	rounded_length = rpc_roundup(length);
	seq = tvb_get_ntohl(tvb, offset+4);

	if (tree) {
		gitem = proto_tree_add_text(tree, tvb, offset,
					    4+rounded_length, "GSS Data");
		gtree = proto_item_add_subtree(gitem, ett_rpc_gss_data);
		proto_tree_add_uint(gtree, hf_rpc_authgss_data_length,
				    tvb, offset+0, 4, length);
		proto_tree_add_uint(gtree, hf_rpc_authgss_seq,
				    tvb, offset+4, 4, seq);
	}
	offset += 8;

	if (dissect_function != NULL) {
		/* offset = */
		call_dissect_function(tvb, pinfo, gtree, offset,
				      dissect_function, progname);
	}
	offset += rounded_length - 4;
	offset = dissect_rpc_data(tvb, tree, hf_rpc_authgss_checksum,
			offset);
	return offset;
}


static int
dissect_rpc_authgss_priv_data(tvbuff_t *tvb, proto_tree *tree, int offset)
{
	offset = dissect_rpc_data(tvb, tree, hf_rpc_authgss_data,
			offset);
	return offset;
}

/*
 * Dissect the arguments to an indirect call; used by the portmapper/RPCBIND
 * dissector.
 *
 * Record this call in a hash table, similar to the hash table for
 * direct calls, so we can find it when dissecting an indirect call reply.
 */
int
dissect_rpc_indir_call(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
    int offset, int args_id, guint32 prog, guint32 vers, guint32 proc)
{
	conversation_t* conversation;
	static address null_address = { AT_NONE, 0, NULL };
	rpc_proc_info_key key;
	rpc_proc_info_value *value;
	rpc_call_info_value *rpc_call;
	rpc_call_info_key rpc_call_key;
	rpc_call_info_key *new_rpc_call_key;
	dissect_function_t *dissect_function = NULL;

	key.prog = prog;
	key.vers = vers;
	key.proc = proc;
	if ((value = g_hash_table_lookup(rpc_procs,&key)) != NULL) {
		dissect_function = value->dissect_call;

		/* Keep track of the address and port whence the call came,
		   and the port to which the call is being sent, so that
		   we can match up calls with replies.

		   If the transport is connection-oriented (we check, for
		   now, only for "pinfo->ptype" of PT_TCP), we take
		   into account the address from which the call was sent
		   and the address to which the call was sent, because
		   the addresses of the two endpoints should be the same
		   for all calls and replies.

		   If the transport is connectionless, we don't worry
		   about the address to which the call was sent and from
		   which the reply was sent, because there's no
		   guarantee that the reply will come from the address
		   to which the call was sent. */
		if (pinfo->ptype == PT_TCP) {
			conversation = find_conversation(&pinfo->src,
			    &pinfo->dst, pinfo->ptype, pinfo->srcport,
			    pinfo->destport, 0);
		} else {
			/*
			 * XXX - can we just use NO_ADDR_B?  Unfortunately,
			 * you currently still have to pass a non-null
			 * pointer for the second address argument even
			 * if you do that.
			 */
			conversation = find_conversation(&pinfo->src,
			    &null_address, pinfo->ptype, pinfo->srcport,
			    pinfo->destport, 0);
		}
		if (conversation == NULL) {
			/* It's not part of any conversation - create a new
			   one.

			   XXX - this should never happen, as we should've
			   created a conversation for it in the RPC
			   dissector. */
			if (pinfo->ptype == PT_TCP) {
				conversation = conversation_new(&pinfo->src,
				    &pinfo->dst, pinfo->ptype, pinfo->srcport,
				    pinfo->destport, 0);
			} else {
				conversation = conversation_new(&pinfo->src,
				    &null_address, pinfo->ptype, pinfo->srcport,
				    pinfo->destport, 0);
			}
		}

		/* Make the dissector for this conversation the non-heuristic
		   RPC dissector. */
		conversation_set_dissector(conversation,
		    (pinfo->ptype == PT_TCP) ? rpc_tcp_handle : rpc_handle);

		/* Prepare the key data.

		   Dissectors for RPC procedure calls and replies shouldn't
		   create new tvbuffs, and we don't create one ourselves,
		   so we should have been handed the tvbuff for this RPC call;
		   as such, the XID is at offset 0 in this tvbuff. */
		rpc_call_key.xid = tvb_get_ntohl(tvb, 0);
		rpc_call_key.conversation = conversation;

		/* look up the request */
		rpc_call = g_hash_table_lookup(rpc_indir_calls, &rpc_call_key);
		if (rpc_call == NULL) {
			/* We didn't find it; create a new entry.
			   Prepare the value data.
			   Not all of it is needed for handling indirect
			   calls, so we set a bunch of items to 0. */
			new_rpc_call_key = g_mem_chunk_alloc(rpc_call_info_key_chunk);
			*new_rpc_call_key = rpc_call_key;
			rpc_call = g_mem_chunk_alloc(rpc_call_info_value_chunk);
			rpc_call->req_num = 0;
			rpc_call->rep_num = 0;
			rpc_call->prog = prog;
			rpc_call->vers = vers;
			rpc_call->proc = proc;
			rpc_call->private_data = NULL;

			/*
			 * XXX - what about RPCSEC_GSS?
			 * Do we have to worry about it?
			 */
			rpc_call->flavor = FLAVOR_NOT_GSSAPI;
			rpc_call->gss_proc = 0;
			rpc_call->gss_svc = 0;
			rpc_call->proc_info = value;
			/* store it */
			g_hash_table_insert(rpc_indir_calls, new_rpc_call_key,
			    rpc_call);
		}
	}
	else {
		/* We don't know the procedure.
		   Happens only with strange program versions or
		   non-existing dissectors.
		   Just show the arguments as opaque data. */
		offset = dissect_rpc_data(tvb, tree, args_id,
		    offset);
		return offset;
	}

	if ( tree )
	{
		proto_tree_add_text(tree, tvb, offset, 4,
			"Argument length: %u",
			tvb_get_ntohl(tvb, offset));
	}
	offset += 4;

	/* Dissect the arguments */
	offset = call_dissect_function(tvb, pinfo, tree, offset,
			dissect_function, NULL);
	return offset;
}

/*
 * Dissect the results in an indirect reply; used by the portmapper/RPCBIND
 * dissector.
 */
int
dissect_rpc_indir_reply(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
    int offset, int result_id, int prog_id, int vers_id, int proc_id)
{
	conversation_t* conversation;
	static address null_address = { AT_NONE, 0, NULL };
	rpc_call_info_key rpc_call_key;
	rpc_call_info_value *rpc_call;
	char *procname = NULL;
	char procname_static[20];
	dissect_function_t *dissect_function = NULL;

	/* Look for the matching call in the hash table of indirect
	   calls.  A reply must match a call that we've seen, and the
	   reply must be sent to the same port and address that the
	   call came from, and must come from the port to which the
	   call was sent.

	   If the transport is connection-oriented (we check, for
	   now, only for "pinfo->ptype" of PT_TCP), we take
	   into account the address from which the call was sent
	   and the address to which the call was sent, because
	   the addresses of the two endpoints should be the same
	   for all calls and replies.

	   If the transport is connectionless, we don't worry
	   about the address to which the call was sent and from
	   which the reply was sent, because there's no
	   guarantee that the reply will come from the address
	   to which the call was sent. */
	if (pinfo->ptype == PT_TCP) {
		conversation = find_conversation(&pinfo->src, &pinfo->dst,
		    pinfo->ptype, pinfo->srcport, pinfo->destport, 0);
	} else {
		/*
		 * XXX - can we just use NO_ADDR_B?  Unfortunately,
		 * you currently still have to pass a non-null
		 * pointer for the second address argument even
		 * if you do that.
		 */
		conversation = find_conversation(&null_address, &pinfo->dst,
		    pinfo->ptype, pinfo->srcport, pinfo->destport, 0);
	}
	if (conversation == NULL) {
		/* We haven't seen an RPC call for that conversation,
		   so we can't check for a reply to that call.
		   Just show the reply stuff as opaque data. */
		offset = dissect_rpc_data(tvb, tree, result_id,
		    offset);
		return offset;
	}

	/* The XIDs of the call and reply must match. */
	rpc_call_key.xid = tvb_get_ntohl(tvb, 0);
	rpc_call_key.conversation = conversation;
	rpc_call = g_hash_table_lookup(rpc_indir_calls, &rpc_call_key);
	if (rpc_call == NULL) {
		/* The XID doesn't match a call from that
		   conversation, so it's probably not an RPC reply.
		   Just show the reply stuff as opaque data. */
		offset = dissect_rpc_data(tvb, tree, result_id,
		    offset);
		return offset;
	}

	if (rpc_call->proc_info != NULL) {
		dissect_function = rpc_call->proc_info->dissect_reply;
		if (rpc_call->proc_info->name != NULL) {
			procname = rpc_call->proc_info->name;
		}
		else {
			sprintf(procname_static, "proc-%u", rpc_call->proc);
			procname = procname_static;
		}
	}
	else {
#if 0
		dissect_function = NULL;
#endif
		sprintf(procname_static, "proc-%u", rpc_call->proc);
		procname = procname_static;
	}

	if ( tree )
	{
		/* Put the program, version, and procedure into the tree. */
		proto_tree_add_uint_format(tree, prog_id, tvb,
			0, 0, rpc_call->prog, "Program: %s (%u)",
			rpc_prog_name(rpc_call->prog), rpc_call->prog);
		proto_tree_add_uint(tree, vers_id, tvb, 0, 0, rpc_call->vers);
		proto_tree_add_uint_format(tree, proc_id, tvb,
			0, 0, rpc_call->proc, "Procedure: %s (%u)",
			procname, rpc_call->proc);
	}

	if (dissect_function == NULL) {
		/* We don't know how to dissect the reply procedure.
		   Just show the reply stuff as opaque data. */
		offset = dissect_rpc_data(tvb, tree, result_id,
		    offset);
		return offset;
	}

	if (tree) {
		/* Put the length of the reply value into the tree. */
		proto_tree_add_text(tree, tvb, offset, 4,
			"Argument length: %u",
			tvb_get_ntohl(tvb, offset));
	}
	offset += 4;

	/* Dissect the return value */
	offset = call_dissect_function(tvb, pinfo, tree, offset,
			dissect_function, NULL);
	return offset;
}

/*
 * Just mark this as a continuation of an earlier packet.
 */
static void
dissect_rpc_continuation(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	proto_item *rpc_item;
	proto_tree *rpc_tree;

	if (check_col(pinfo->cinfo, COL_PROTOCOL))
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "RPC");
	if (check_col(pinfo->cinfo, COL_INFO))
		col_set_str(pinfo->cinfo, COL_INFO, "Continuation");

	if (tree) {
		rpc_item = proto_tree_add_item(tree, proto_rpc, tvb, 0, -1,
				FALSE);
		rpc_tree = proto_item_add_subtree(rpc_item, ett_rpc);
		proto_tree_add_text(rpc_tree, tvb, 0, -1, "Continuation data");
	}
}

static gboolean
dissect_rpc_message(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
    tvbuff_t *frag_tvb, fragment_data *ipfd_head, gboolean is_tcp,
    guint32 rpc_rm)
{
	guint32	msg_type;
	rpc_call_info_key rpc_call_key;
	rpc_call_info_value *rpc_call = NULL;
	rpc_prog_info_value *rpc_prog = NULL;
	rpc_prog_info_key rpc_prog_key;

	unsigned int xid;
	unsigned int rpcvers;
	unsigned int prog = 0;
	unsigned int vers = 0;
	unsigned int proc = 0;
	flavor_t flavor = FLAVOR_UNKNOWN;
	unsigned int gss_proc = 0;
	unsigned int gss_svc = 0;
	int	proto = 0;
	int	ett = 0;
	int	procedure_hf;

	unsigned int reply_state;
	unsigned int accept_state;
	unsigned int reject_state;

	char *msg_type_name = NULL;
	char *progname = NULL;
	char *procname = NULL;
	static char procname_static[20];

	unsigned int vers_low;
	unsigned int vers_high;

	unsigned int auth_state;

	proto_item *rpc_item = NULL;
	proto_tree *rpc_tree = NULL;

	proto_item *pitem = NULL;
	proto_tree *ptree = NULL;
	int offset = (is_tcp && tvb == frag_tvb) ? 4 : 0;
	int offset_old = offset;

	rpc_call_info_key	*new_rpc_call_key;
	rpc_proc_info_key	key;
	rpc_proc_info_value	*value = NULL;
	conversation_t* conversation;
	static address null_address = { AT_NONE, 0, NULL };
	nstime_t ns;

	dissect_function_t *dissect_function = NULL;
	gboolean dissect_rpc = TRUE;

	/*
	 * Check to see whether this looks like an RPC call or reply.
	 */
	if (!tvb_bytes_exist(tvb, offset, 8)) {
		/* Captured data in packet isn't enough to let us tell. */
		return FALSE;
	}

	/* both directions need at least this */
	msg_type = tvb_get_ntohl(tvb, offset + 4);

	switch (msg_type) {

	case RPC_CALL:
		/* check for RPC call */
		if (!tvb_bytes_exist(tvb, offset, 16)) {
			/* Captured data in packet isn't enough to let us
			   tell. */
			return FALSE;
		}

		/* XID can be anything, we don't check it.
		   We already have the message type.
		   Check whether an RPC version number of 2 is in the
		   location where it would be, and that an RPC program
		   number we know about is in the location where it would be. */
		rpc_prog_key.prog = tvb_get_ntohl(tvb, offset + 12);
		if (tvb_get_ntohl(tvb, offset + 8) != 2 ||
		    ((rpc_prog = g_hash_table_lookup(rpc_progs, &rpc_prog_key))
		       == NULL)) {
			/* They're not, so it's probably not an RPC call. */
			return FALSE;
		}
		break;

	case RPC_REPLY:
		/* Check for RPC reply.  A reply must match a call that
		   we've seen, and the reply must be sent to the same
		   port and address that the call came from, and must
		   come from the port to which the call was sent.

		   If the transport is connection-oriented (we check, for
		   now, only for "pinfo->ptype" of PT_TCP), we take
		   into account the address from which the call was sent
		   and the address to which the call was sent, because
		   the addresses of the two endpoints should be the same
		   for all calls and replies.

		   If the transport is connectionless, we don't worry
		   about the address to which the call was sent and from
		   which the reply was sent, because there's no
		   guarantee that the reply will come from the address
		   to which the call was sent. */
		if (pinfo->ptype == PT_TCP) {
			conversation = find_conversation(&pinfo->src,
			    &pinfo->dst, pinfo->ptype, pinfo->srcport,
			    pinfo->destport, 0);
		} else {
			/*
			 * XXX - can we just use NO_ADDR_B?  Unfortunately,
			 * you currently still have to pass a non-null
			 * pointer for the second address argument even
			 * if you do that.
			 */
			conversation = find_conversation(&null_address,
			    &pinfo->dst, pinfo->ptype, pinfo->srcport,
			    pinfo->destport, 0);
		}
		if (conversation == NULL) {
			/* We haven't seen an RPC call for that conversation,
			   so we can't check for a reply to that call. */
			return FALSE;
		}

		/* The XIDs of the call and reply must match. */
		rpc_call_key.xid = tvb_get_ntohl(tvb, offset + 0);
		rpc_call_key.conversation = conversation;
		rpc_call = g_hash_table_lookup(rpc_calls, &rpc_call_key);
		if (rpc_call == NULL) {
			/* The XID doesn't match a call from that
			   conversation, so it's probably not an RPC reply. */
			return FALSE;
		}
		/* pass rpc_info to subdissectors */
		rpc_call->request=FALSE;
		pinfo->private_data=rpc_call;
		break;

	default:
		/* The putative message type field contains neither
		   RPC_CALL nor RPC_REPLY, so it's not an RPC call or
		   reply. */
		return FALSE;
	}

	if (is_tcp) {
		/*
		 * This is RPC-over-TCP; check if this is the last
		 * fragment.
		 */
		if (!(rpc_rm & RPC_RM_LASTFRAG)) {
			/*
			 * This isn't the last fragment.
			 * If we're doing reassembly, just return
			 * TRUE to indicate that this looks like
			 * the beginning of an RPC message,
			 * and let them do fragment reassembly.
			 */
			if (rpc_defragment)
				return TRUE;
		}
	}

	if (check_col(pinfo->cinfo, COL_PROTOCOL))
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "RPC");
	if (check_col(pinfo->cinfo, COL_INFO))
		col_clear(pinfo->cinfo, COL_INFO);

	if (tree) {
		rpc_item = proto_tree_add_item(tree, proto_rpc, tvb, 0, -1,
		    FALSE);
		rpc_tree = proto_item_add_subtree(rpc_item, ett_rpc);

		if (is_tcp) {
			show_rpc_fraginfo(tvb, frag_tvb, rpc_tree, rpc_rm,
			    ipfd_head, pinfo);
		}
	}

	xid      = tvb_get_ntohl(tvb, offset + 0);
	if (rpc_tree) {
		proto_tree_add_uint_format(rpc_tree,hf_rpc_xid, tvb,
			offset+0, 4, xid, "XID: 0x%x (%u)", xid, xid);
	}

	msg_type_name = val_to_str(msg_type,rpc_msg_type,"%u");
	if (rpc_tree) {
		proto_tree_add_uint(rpc_tree, hf_rpc_msgtype, tvb,
			offset+4, 4, msg_type);
	}

	offset += 8;

	switch (msg_type) {

	case RPC_CALL:
		/* we know already the proto-entry, the ETT-const,
		   and "rpc_prog" */
		proto = rpc_prog->proto;
		ett = rpc_prog->ett;
		progname = rpc_prog->progname;

		rpcvers = tvb_get_ntohl(tvb, offset + 0);
		if (rpc_tree) {
			proto_tree_add_uint(rpc_tree,
				hf_rpc_version, tvb, offset+0, 4, rpcvers);
		}

		prog = tvb_get_ntohl(tvb, offset + 4);

		if (rpc_tree) {
			proto_tree_add_uint_format(rpc_tree,
				hf_rpc_program, tvb, offset+4, 4, prog,
				"Program: %s (%u)", progname, prog);
		}

		if (check_col(pinfo->cinfo, COL_PROTOCOL)) {
			/* Set the protocol name to the underlying
			   program name. */
			col_set_str(pinfo->cinfo, COL_PROTOCOL, progname);
		}

		vers = tvb_get_ntohl(tvb, offset+8);
		if (rpc_tree) {
			proto_tree_add_uint(rpc_tree,
				hf_rpc_programversion, tvb, offset+8, 4, vers);
		}

		proc = tvb_get_ntohl(tvb, offset+12);

		key.prog = prog;
		key.vers = vers;
		key.proc = proc;

		if ((value = g_hash_table_lookup(rpc_procs,&key)) != NULL) {
			dissect_function = value->dissect_call;
			procname = value->name;
		}
		else {
			/* happens only with strange program versions or
			   non-existing dissectors */
#if 0
			dissect_function = NULL;
#endif
			sprintf(procname_static, "proc-%u", proc);
			procname = procname_static;
		}

		if (rpc_tree) {
			proto_tree_add_uint_format(rpc_tree,
				hf_rpc_procedure, tvb, offset+12, 4, proc,
				"Procedure: %s (%u)", procname, proc);
		}

		if (check_col(pinfo->cinfo, COL_INFO)) {
			col_add_fstr(pinfo->cinfo, COL_INFO,"V%u %s %s XID 0x%x",
				vers,
				procname,
				msg_type_name,
				xid);
		}

		/* Check for RPCSEC_GSS */
		if (tvb_bytes_exist(tvb, offset+16, 4)) {
			switch (tvb_get_ntohl(tvb, offset+16)) {

			case RPCSEC_GSS:
				/*
				 * It's GSS-API authentication...
				 */
				if (tvb_bytes_exist(tvb, offset+28, 8)) {
					/*
					 * ...and we have the procedure
					 * and service information for it.
					 */
					flavor = FLAVOR_GSSAPI;
					gss_proc = tvb_get_ntohl(tvb, offset+28);
					gss_svc = tvb_get_ntohl(tvb, offset+36);
				} else {
					/*
					 * ...but the procedure and service
					 * information isn't available.
					 */
					flavor = FLAVOR_GSSAPI_NO_INFO;
				}
				break;

			default:
				/*
				 * It's not GSS-API authentication.
				 */
				flavor = FLAVOR_NOT_GSSAPI;
				break;
			}
		}

		/* Keep track of the address and port whence the call came,
		   and the port to which the call is being sent, so that
		   we can match up calls with replies.

		   If the transport is connection-oriented (we check, for
		   now, only for "pinfo->ptype" of PT_TCP), we take
		   into account the address from which the call was sent
		   and the address to which the call was sent, because
		   the addresses of the two endpoints should be the same
		   for all calls and replies.

		   If the transport is connectionless, we don't worry
		   about the address to which the call was sent and from
		   which the reply was sent, because there's no
		   guarantee that the reply will come from the address
		   to which the call was sent. */
		if (pinfo->ptype == PT_TCP) {
			conversation = find_conversation(&pinfo->src,
			    &pinfo->dst, pinfo->ptype, pinfo->srcport,
			    pinfo->destport, 0);
		} else {
			/*
			 * XXX - can we just use NO_ADDR_B?  Unfortunately,
			 * you currently still have to pass a non-null
			 * pointer for the second address argument even
			 * if you do that.
			 */
			conversation = find_conversation(&pinfo->src,
			    &null_address, pinfo->ptype, pinfo->srcport,
			    pinfo->destport, 0);
		}
		if (conversation == NULL) {
			/* It's not part of any conversation - create a new
			   one. */
			if (pinfo->ptype == PT_TCP) {
				conversation = conversation_new(&pinfo->src,
				    &pinfo->dst, pinfo->ptype, pinfo->srcport,
				    pinfo->destport, 0);
			} else {
				conversation = conversation_new(&pinfo->src,
				    &null_address, pinfo->ptype, pinfo->srcport,
				    pinfo->destport, 0);
			}
		}

		/* Make the dissector for this conversation the non-heuristic
		   RPC dissector. */
		conversation_set_dissector(conversation,
		    (pinfo->ptype == PT_TCP) ? rpc_tcp_handle : rpc_handle);

		/* prepare the key data */
		rpc_call_key.xid = xid;
		rpc_call_key.conversation = conversation;

		/* look up the request */
		rpc_call = g_hash_table_lookup(rpc_calls, &rpc_call_key);
		if (rpc_call != NULL) {
			/* We've seen a request with this XID, with the same
			   source and destination, before - but was it
			   *this* request? */
			if (pinfo->fd->num != rpc_call->req_num) {
				/* No, so it's a duplicate request.
				   Mark it as such. */
				if (check_col(pinfo->cinfo, COL_INFO)) {
					col_append_fstr(pinfo->cinfo, COL_INFO,
						" dup XID 0x%x", xid);
					if (rpc_tree) {
						proto_tree_add_uint_hidden(rpc_tree,
							hf_rpc_dup, tvb, 0,0, xid);
						proto_tree_add_uint_hidden(rpc_tree,
							hf_rpc_call_dup, tvb, 0,0, xid);
					}
				}
			}
		}
		else {
			/* Prepare the value data.
			   "req_num" and "rep_num" are frame numbers;
			   frame numbers are 1-origin, so we use 0
			   to mean "we don't yet know in which frame
			   the reply for this call appears". */
			new_rpc_call_key = g_mem_chunk_alloc(rpc_call_info_key_chunk);
			*new_rpc_call_key = rpc_call_key;
			rpc_call = g_mem_chunk_alloc(rpc_call_info_value_chunk);
			rpc_call->req_num = pinfo->fd->num;
			rpc_call->rep_num = 0;
			rpc_call->prog = prog;
			rpc_call->vers = vers;
			rpc_call->proc = proc;
			rpc_call->private_data = NULL;
			rpc_call->xid = xid;
			rpc_call->flavor = flavor;
			rpc_call->gss_proc = gss_proc;
			rpc_call->gss_svc = gss_svc;
			rpc_call->proc_info = value;
			rpc_call->req_time.secs=pinfo->fd->abs_secs;
			rpc_call->req_time.nsecs=pinfo->fd->abs_usecs*1000;

			/* store it */
			g_hash_table_insert(rpc_calls, new_rpc_call_key,
			    rpc_call);
		}

		if(rpc_call && rpc_call->rep_num){
			proto_tree_add_text(rpc_tree, tvb, 0, 0,
			    "The reply to this request is in frame %u",
			    rpc_call->rep_num);
		}

		offset += 16;

		offset = dissect_rpc_cred(tvb, rpc_tree, offset);
		offset = dissect_rpc_verf(tvb, rpc_tree, offset, msg_type);

		/* pass rpc_info to subdissectors */
		rpc_call->request=TRUE;
		pinfo->private_data=rpc_call;

		/* go to the next dissector */

		break;	/* end of RPC call */

	case RPC_REPLY:
		/* we know already the type from the calling routine,
		   and we already have "rpc_call" set above. */
		prog = rpc_call->prog;
		vers = rpc_call->vers;
		proc = rpc_call->proc;
		flavor = rpc_call->flavor;
		gss_proc = rpc_call->gss_proc;
		gss_svc = rpc_call->gss_svc;

		if (rpc_call->proc_info != NULL) {
			dissect_function = rpc_call->proc_info->dissect_reply;
			if (rpc_call->proc_info->name != NULL) {
				procname = rpc_call->proc_info->name;
			}
			else {
				sprintf(procname_static, "proc-%u", proc);
				procname = procname_static;
			}
		}
		else {
#if 0
			dissect_function = NULL;
#endif
			sprintf(procname_static, "proc-%u", proc);
			procname = procname_static;
		}

		rpc_prog_key.prog = prog;
		if ((rpc_prog = g_hash_table_lookup(rpc_progs,&rpc_prog_key)) == NULL) {
			proto = 0;
			ett = 0;
			progname = "Unknown";
		}
		else {
			proto = rpc_prog->proto;
			ett = rpc_prog->ett;
			progname = rpc_prog->progname;

			if (check_col(pinfo->cinfo, COL_PROTOCOL)) {
				/* Set the protocol name to the underlying
				   program name. */
				col_set_str(pinfo->cinfo, COL_PROTOCOL, progname);
			}
		}

		if (check_col(pinfo->cinfo, COL_INFO)) {
			col_add_fstr(pinfo->cinfo, COL_INFO,"V%u %s %s XID 0x%x",
				vers,
				procname,
				msg_type_name,
				xid);
		}

		if (rpc_tree) {
			proto_tree_add_uint_format(rpc_tree,
				hf_rpc_program, tvb, 0, 0, prog,
				"Program: %s (%u)", progname, prog);
			proto_tree_add_uint(rpc_tree,
				hf_rpc_programversion, tvb, 0, 0, vers);
			proto_tree_add_uint_format(rpc_tree,
				hf_rpc_procedure, tvb, 0, 0, proc,
				"Procedure: %s (%u)", procname, proc);
		}

		reply_state = tvb_get_ntohl(tvb,offset+0);
		if (rpc_tree) {
			proto_tree_add_uint(rpc_tree, hf_rpc_state_reply, tvb,
				offset+0, 4, reply_state);
		}
		offset += 4;

		/* Indicate the frame to which this is a reply. */
		if(rpc_call && rpc_call->req_num){
			proto_tree_add_text(rpc_tree, tvb, 0, 0,
			    "This is a reply to a request in frame %u",
			    rpc_call->req_num);
			ns.secs= pinfo->fd->abs_secs-rpc_call->req_time.secs;
			ns.nsecs=pinfo->fd->abs_usecs*1000-rpc_call->req_time.nsecs;
			if(ns.nsecs<0){
				ns.nsecs+=1000000000;
				ns.secs--;
			}
			proto_tree_add_time(rpc_tree, hf_rpc_time, tvb, offset, 0,
				&ns);
		}


		if (rpc_call->rep_num == 0) {
			/* We have not yet seen a reply to that call, so
			   this must be the first reply; remember its
			   frame number. */
			rpc_call->rep_num = pinfo->fd->num;
		} else {
			/* We have seen a reply to this call - but was it
			   *this* reply? */
			if (rpc_call->rep_num != pinfo->fd->num) {
				/* No, so it's a duplicate reply.
				   Mark it as such. */
				if (check_col(pinfo->cinfo, COL_INFO)) {
					col_append_fstr(pinfo->cinfo, COL_INFO,
						" dup XID 0x%x", xid);
					if (rpc_tree) {
						proto_tree_add_uint_hidden(rpc_tree,
							hf_rpc_dup, tvb, 0,0, xid);
						proto_tree_add_uint_hidden(rpc_tree,
							hf_rpc_reply_dup, tvb, 0,0, xid);
					}
				}
			}
		}

		switch (reply_state) {

		case MSG_ACCEPTED:
			offset = dissect_rpc_verf(tvb, rpc_tree, offset, msg_type);
			accept_state = tvb_get_ntohl(tvb,offset+0);
			if (rpc_tree) {
				proto_tree_add_uint(rpc_tree, hf_rpc_state_accept, tvb,
					offset+0, 4, accept_state);
			}
			offset += 4;
			switch (accept_state) {

			case SUCCESS:
				/* go to the next dissector */
				break;

			case PROG_MISMATCH:
				vers_low = tvb_get_ntohl(tvb,offset+0);
				vers_high = tvb_get_ntohl(tvb,offset+4);
				if (rpc_tree) {
					proto_tree_add_uint(rpc_tree,
						hf_rpc_programversion_min,
						tvb, offset+0, 4, vers_low);
					proto_tree_add_uint(rpc_tree,
						hf_rpc_programversion_max,
						tvb, offset+4, 4, vers_high);
				}
				offset += 8;

				/*
				 * There's no protocol reply, so don't
				 * try to dissect it.
				 */
				dissect_rpc = FALSE;
				break;

			default:
				/*
				 * There's no protocol reply, so don't
				 * try to dissect it.
				 */
				dissect_rpc = FALSE;
				break;
			}
			break;

		case MSG_DENIED:
			reject_state = tvb_get_ntohl(tvb,offset+0);
			if (rpc_tree) {
				proto_tree_add_uint(rpc_tree,
					hf_rpc_state_reject, tvb, offset+0, 4,
					reject_state);
			}
			offset += 4;

			if (reject_state==RPC_MISMATCH) {
				vers_low = tvb_get_ntohl(tvb,offset+0);
				vers_high = tvb_get_ntohl(tvb,offset+4);
				if (rpc_tree) {
					proto_tree_add_uint(rpc_tree,
						hf_rpc_version_min,
						tvb, offset+0, 4, vers_low);
					proto_tree_add_uint(rpc_tree,
						hf_rpc_version_max,
						tvb, offset+4, 4, vers_high);
				}
				offset += 8;
			} else if (reject_state==AUTH_ERROR) {
				auth_state = tvb_get_ntohl(tvb,offset+0);
				if (rpc_tree) {
					proto_tree_add_uint(rpc_tree,
						hf_rpc_state_auth, tvb, offset+0, 4,
						auth_state);
				}
				offset += 4;
			}

			/*
			 * There's no protocol reply, so don't
			 * try to dissect it.
			 */
			dissect_rpc = FALSE;
			break;

		default:
			/*
			 * This isn't a valid reply state, so we have
			 * no clue what's going on; don't try to dissect
			 * the protocol reply.
			 */
			dissect_rpc = FALSE;
			break;
		}
		break; /* end of RPC reply */

	default:
		/*
		 * The switch statement at the top returned if
		 * this was neither an RPC call nor a reply.
		 */
		g_assert_not_reached();
	}

	/* now we know, that RPC was shorter */
	if (rpc_item) {
		proto_item_set_len(rpc_item, offset - offset_old);
	}

	if (!dissect_rpc) {
		/*
		 * There's no RPC call or reply here; just dissect
		 * whatever's left as data.
		 */
		call_dissector(data_handle,
		    tvb_new_subset(tvb, offset, -1, -1), pinfo, rpc_tree);
		return TRUE;
	}

	/* create here the program specific sub-tree */
	if (tree) {
		pitem = proto_tree_add_item(tree, proto, tvb, offset, -1,
		    FALSE);
		if (pitem) {
			ptree = proto_item_add_subtree(pitem, ett);
		}

		if (ptree) {
			proto_tree_add_uint(ptree,
				hf_rpc_programversion, tvb, 0, 0, vers);
			if (rpc_prog->procedure_hfs->len > vers)
				procedure_hf = g_array_index(rpc_prog->procedure_hfs, int, vers);
			else {
				/*
				 * No such element in the GArray.
				 */
				procedure_hf = 0;
			}
			if (procedure_hf != 0 && procedure_hf != -1) {
				proto_tree_add_uint(ptree,
					procedure_hf, tvb, 0, 0, proc);
			} else {
				proto_tree_add_uint_format(ptree,
					hf_rpc_procedure, tvb, 0, 0, proc,
					"Procedure: %s (%u)", procname, proc);
			}
		}
	}

	if (!proto_is_protocol_enabled(proto))
		dissect_function = NULL;

	/*
	 * Handle RPCSEC_GSS specially.
	 */
	switch (flavor) {

	case FLAVOR_UNKNOWN:
		/*
		 * We don't know the authentication flavor, so we can't
		 * dissect the payload.
		 */
		proto_tree_add_text(ptree, tvb, offset, -1,
		    "Unknown authentication flavor - cannot dissect");
		return TRUE;

	case FLAVOR_NOT_GSSAPI:
		/*
		 * It's not GSS-API authentication.  Just dissect the
		 * payload.
		 */
		offset = call_dissect_function(tvb, pinfo, ptree, offset,
				dissect_function, progname);
		break;

	case FLAVOR_GSSAPI_NO_INFO:
		/*
		 * It's GSS-API authentication, but we don't have the
		 * procedure and service information, so we can't dissect
		 * the payload.
		 */
		proto_tree_add_text(ptree, tvb, offset, -1,
		    "GSS-API authentication, but procedure and service unknown - cannot dissect");
		return TRUE;

	case FLAVOR_GSSAPI:
		/*
		 * It's GSS-API authentication, and we have the procedure
		 * and service information; process the GSS-API stuff,
		 * and process the payload if there is any.
		 */
		switch (gss_proc) {

		case RPCSEC_GSS_INIT:
		case RPCSEC_GSS_CONTINUE_INIT:
			if (msg_type == RPC_CALL) {
				offset = dissect_rpc_authgss_initarg(tvb,
					ptree, offset, pinfo);
			}
			else {
				offset = dissect_rpc_authgss_initres(tvb,
					ptree, offset, pinfo);
			}
			break;

		case RPCSEC_GSS_DATA:
			if (gss_svc == RPCSEC_GSS_SVC_NONE) {
				offset = call_dissect_function(tvb,
						pinfo, ptree, offset,
						dissect_function,
						progname);
			}
			else if (gss_svc == RPCSEC_GSS_SVC_INTEGRITY) {
				offset = dissect_rpc_authgss_integ_data(tvb,
						pinfo, ptree, offset,
						dissect_function,
						progname);
			}
			else if (gss_svc == RPCSEC_GSS_SVC_PRIVACY) {
				offset = dissect_rpc_authgss_priv_data(tvb,
						ptree, offset);
			}
			break;

		default:
			break;
		}
	}

	/* dissect any remaining bytes (incomplete dissection) as pure data in
	   the ptree */
	call_dissector(data_handle,
	    tvb_new_subset(tvb, offset, -1, -1), pinfo, ptree);


	/* XXX this should really loop over all fhandles registred for the frame */
	if(nfs_fhandle_reqrep_matching){
		nfs_fhandle_data_t *fhd;
		switch (msg_type) {
		case RPC_CALL:
			if(rpc_call && rpc_call->rep_num){
				fhd=(nfs_fhandle_data_t *)g_hash_table_lookup(
					nfs_fhandle_frame_table,
					(gconstpointer)rpc_call->rep_num);
				if(fhd){
					dissect_fhandle_hidden(pinfo,
						ptree, fhd);
				}
			}
			break;
		case RPC_REPLY:
			if(rpc_call && rpc_call->req_num){
				fhd=(nfs_fhandle_data_t *)g_hash_table_lookup(
					nfs_fhandle_frame_table,
					(gconstpointer)rpc_call->req_num);
				if(fhd){
					dissect_fhandle_hidden(pinfo,
						ptree, fhd);
				}
			}
			break;
		}
	}

	tap_queue_packet(rpc_tap, pinfo, rpc_call);
	return TRUE;
}

static gboolean
dissect_rpc_heur(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	return dissect_rpc_message(tvb, pinfo, tree, NULL, NULL, FALSE, 0);
}

static void
dissect_rpc(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	if (!dissect_rpc_message(tvb, pinfo, tree, NULL, NULL, FALSE, 0))
		dissect_rpc_continuation(tvb, pinfo, tree);
}

/* Defragmentation of RPC-over-TCP records */
/* table to hold defragmented RPC records */
static GHashTable *rpc_fragment_table = NULL;

static GHashTable *rpc_reassembly_table = NULL;
static GMemChunk *rpc_fragment_key_chunk = NULL;
static int rpc_fragment_init_count = 200;

typedef struct _rpc_fragment_key {
	guint32 conv_id;
	guint32 seq;
	guint32 offset;
	/* xxx */
	guint32 start_seq;
} rpc_fragment_key;

static guint
rpc_fragment_hash(gconstpointer k)
{
	rpc_fragment_key *key = (rpc_fragment_key *)k;

	return key->conv_id + key->seq;
}

static gint
rpc_fragment_equal(gconstpointer k1, gconstpointer k2)
{
	rpc_fragment_key *key1 = (rpc_fragment_key *)k1;
	rpc_fragment_key *key2 = (rpc_fragment_key *)k2;

	return key1->conv_id == key2->conv_id &&
	    key1->seq == key2->seq;
}

static void
show_rpc_fragheader(tvbuff_t *tvb, proto_tree *tree, guint32 rpc_rm)
{
	proto_item *hdr_item;
	proto_tree *hdr_tree;
	guint32 fraglen;

	if (tree) {
		fraglen = rpc_rm & RPC_RM_FRAGLEN;

		hdr_item = proto_tree_add_text(tree, tvb, 0, 4,
		    "Fragment header: %s%u %s",
		    (rpc_rm & RPC_RM_LASTFRAG) ? "Last fragment, " : "",
		    fraglen, plurality(fraglen, "byte", "bytes"));
		hdr_tree = proto_item_add_subtree(hdr_item, ett_rpc_fraghdr);

		proto_tree_add_boolean(hdr_tree, hf_rpc_lastfrag, tvb, 0, 4,
		    rpc_rm);
		proto_tree_add_uint(hdr_tree, hf_rpc_fraglen, tvb, 0, 4,
		    rpc_rm);
	}
}

static void
show_rpc_fragment(tvbuff_t *tvb, proto_tree *tree, guint32 rpc_rm)
{
	if (tree) {
		/*
		 * Show the fragment header and the data for the fragment.
		 */
		show_rpc_fragheader(tvb, tree, rpc_rm);
		proto_tree_add_text(tree, tvb, 4, -1, "Fragment Data");
	}
}

static void
make_frag_tree(tvbuff_t *tvb, proto_tree *tree, int proto, gint ett,
    guint32 rpc_rm)
{
	proto_item *frag_item;
	proto_tree *frag_tree;

	if (tree == NULL)
		return;		/* nothing to do */

	frag_item = proto_tree_add_protocol_format(tree, proto, tvb, 0, -1,
	    "%s Fragment", proto_get_protocol_name(proto));
	frag_tree = proto_item_add_subtree(frag_item, ett);
	show_rpc_fragment(tvb, frag_tree, rpc_rm);
}

void
show_rpc_fraginfo(tvbuff_t *tvb, tvbuff_t *frag_tvb, proto_tree *tree,
    guint32 rpc_rm, fragment_data *ipfd_head, packet_info *pinfo)
{
	if (tree == NULL)
		return;		/* don't do any work */

	if (tvb != frag_tvb) {
		/*
		 * This message was not all in one fragment,
		 * so show the fragment header *and* the data
		 * for the fragment (which is the last fragment),
		 * and a tree with information about all fragments.
		 */
		show_rpc_fragment(frag_tvb, tree, rpc_rm);

		/*
		 * Show a tree with information about all fragments.
		 */
		show_fragment_tree(ipfd_head, &rpc_frag_items, tree, pinfo, tvb);
	} else {
		/*
		 * This message was all in one fragment, so just show
		 * the fragment header.
		 */
		show_rpc_fragheader(tvb, tree, rpc_rm);
	}
}

static gboolean
call_message_dissector(tvbuff_t *tvb, tvbuff_t *rec_tvb, packet_info *pinfo,
    proto_tree *tree, tvbuff_t *frag_tvb, rec_dissector_t dissector,
    fragment_data *ipfd_head, guint32 rpc_rm)
{
	const char *saved_proto;
	volatile gboolean rpc_succeeded;

	/*
	 * Catch the ReportedBoundsError exception; if
	 * this particular message happens to get a
	 * ReportedBoundsError exception, that doesn't
	 * mean that we should stop dissecting RPC
	 * messages within this frame or chunk of
	 * reassembled data.
	 *
	 * If it gets a BoundsError, we can stop, as there's
	 * nothing more to see, so we just re-throw it.
	 */
	saved_proto = pinfo->current_proto;
	rpc_succeeded = FALSE;
	TRY {
		rpc_succeeded = (*dissector)(rec_tvb, pinfo, tree,
		    frag_tvb, ipfd_head, TRUE, rpc_rm);
	}
	CATCH(BoundsError) {
		RETHROW;
	}
	CATCH(ReportedBoundsError) {
		show_reported_bounds_error(tvb, pinfo, tree);
		pinfo->current_proto = saved_proto;

		/*
		 * We treat this as a "successful" dissection of
		 * an RPC packet, as "dissect_rpc_message()"
		 * *did* decide it was an RPC packet, throwing
		 * an exception while dissecting it as such.
		 */
		rpc_succeeded = TRUE;
	}
	ENDTRY;
	return rpc_succeeded;
}

int
dissect_rpc_fragment(tvbuff_t *tvb, int offset, packet_info *pinfo,
    proto_tree *tree, rec_dissector_t dissector, gboolean is_heur,
    int proto, int ett, gboolean defragment)
{
	struct tcpinfo *tcpinfo = pinfo->private_data;
	guint32 seq = tcpinfo->seq + offset;
	guint32 rpc_rm;
	volatile gint32 len;
	gint32 seglen;
	gint tvb_len, tvb_reported_len;
	tvbuff_t *frag_tvb;
	gboolean rpc_succeeded;
	gboolean save_fragmented;
	rpc_fragment_key old_rfk, *rfk, *new_rfk;
	conversation_t *conversation;
	fragment_data *ipfd_head;
	tvbuff_t *rec_tvb;

	/*
	 * Get the record mark.
	 */
	if (!tvb_bytes_exist(tvb, offset, 4)) {
		/*
		 * XXX - we should somehow arrange to handle
		 * a record mark split across TCP segments.
		 */
		return 0;	/* not enough to tell if it's valid */
	}
	rpc_rm = tvb_get_ntohl(tvb, offset);

	len = rpc_rm & RPC_RM_FRAGLEN;

	/*
	 * Do TCP desegmentation, if enabled.
	 *
	 * XXX - reject fragments bigger than 2 megabytes.
	 * This is arbitrary, but should at least prevent
	 * some crashes from either packets with really
	 * large RPC-over-TCP fragments or from stuff that's
	 * not really valid for this protocol.
	 */
	if (len > 2*1024*1024)
		return 0;	/* pretend it's not valid */
	if (rpc_desegment) {
		seglen = tvb_length_remaining(tvb, offset + 4);

		if (len > seglen && pinfo->can_desegment) {
			/*
			 * This frame doesn't have all of the
			 * data for this message, but we can do
			 * reassembly on it.
			 *
			 * If this is a heuristic dissector, just
			 * return 0 - we don't want to try to get
			 * more data, as that's too likely to cause
			 * us to misidentify this as valid.
			 *
			 * If this isn't a heuristic dissector,
			 * we've already identified this conversation
			 * as containing data for this protocol, as we
			 * saw valid data in previous frames.  Try to
			 * get more data.
			 */
			if (is_heur)
				return 0;	/* not valid */
			else {
				pinfo->desegment_offset = offset;
				pinfo->desegment_len = len - seglen;
				return -pinfo->desegment_len;
			}
		}
	}
	len += 4;	/* include record mark */
	tvb_len = tvb_length_remaining(tvb, offset);
	tvb_reported_len = tvb_reported_length_remaining(tvb, offset);
	if (tvb_len > len)
		tvb_len = len;
	if (tvb_reported_len > len)
		tvb_reported_len = len;
	frag_tvb = tvb_new_subset(tvb, offset, tvb_len,
	    tvb_reported_len);

	/*
	 * If we're not defragmenting, just hand this to the
	 * disssector.
	 */
	if (!defragment) {
		/*
		 * This is the first fragment we've seen, and it's also
		 * the last fragment; that means the record wasn't
		 * fragmented.  Hand the dissector the tvbuff for the
		 * fragment as the tvbuff for the record.
		 */
		rec_tvb = frag_tvb;
		ipfd_head = NULL;

		/*
		 * Mark this as fragmented, so if somebody throws an
		 * exception, we don't report it as a malformed frame.
		 */
		save_fragmented = pinfo->fragmented;
		pinfo->fragmented = TRUE;
		rpc_succeeded = call_message_dissector(tvb, rec_tvb, pinfo,
		    tree, frag_tvb, dissector, ipfd_head, rpc_rm);
		pinfo->fragmented = save_fragmented;
		if (!rpc_succeeded)
			return 0;	/* not RPC */
		return len;
	}

	/*
	 * First, we check to see if this fragment is part of a record
	 * that we're in the process of defragmenting.
	 *
	 * The key is the conversation ID for the conversation to which
	 * the packet belongs and the current sequence number.
	 * We must first find the conversation and, if we don't find
	 * one, create it.  We know this is running over TCP, so the
	 * conversation should not wildcard either address or port.
	 */
	conversation = find_conversation(&pinfo->src, &pinfo->dst,
	    pinfo->ptype, pinfo->srcport, pinfo->destport, 0);
	if (conversation == NULL) {
		/*
		 * It's not part of any conversation - create a new one.
		 */
		conversation = conversation_new(&pinfo->src, &pinfo->dst,
		    pinfo->ptype, pinfo->srcport, pinfo->destport, 0);
	}
	old_rfk.conv_id = conversation->index;
	old_rfk.seq = seq;
	rfk = g_hash_table_lookup(rpc_reassembly_table, &old_rfk);

	if (rfk == NULL) {
		/*
		 * This fragment was not found in our table, so it doesn't
		 * contain a continuation of a higher-level PDU.
		 * Is it the last fragment?
		 */
		if (!(rpc_rm & RPC_RM_LASTFRAG)) {
			/*
			 * This isn't the last fragment, so we don't
			 * have the complete record.
			 *
			 * It's the first fragment we've seen, so if
			 * it's truly the first fragment of the record,
			 * and it has enough data, the dissector can at
			 * least check whether it looks like a valid
			 * message, as it contains the start of the
			 * message.
			 *
			 * The dissector should not dissect anything
			 * if the "last fragment" flag isn't set in
			 * the record marker, so it shouldn't throw
			 * an exception.
			 */
			if (!(*dissector)(frag_tvb, pinfo, tree, frag_tvb,
			    NULL, TRUE, rpc_rm))
				return 0;	/* not valid */

			/*
			 * OK, now start defragmentation with that
			 * fragment.  Add this fragment, and set up
			 * next packet/sequence number as well.
			 *
			 * We must remember this fragment.
			 */

			rfk = g_mem_chunk_alloc(rpc_fragment_key_chunk);
			rfk->conv_id = conversation->index;
			rfk->seq = seq;
			rfk->offset = 0;
			rfk->start_seq = seq;
			g_hash_table_insert(rpc_reassembly_table, rfk, rfk);

			/*
			 * Start defragmentation.
			 */
			ipfd_head = fragment_add(tvb, offset + 4, pinfo,
			    rfk->start_seq, rpc_fragment_table,
			    rfk->offset, len - 4, TRUE);

			/*
			 * Make sure that defragmentation isn't complete;
			 * it shouldn't be, as this is the first fragment
			 * we've seen, and the "last fragment" bit wasn't
			 * set on it.
			 */
			g_assert(ipfd_head == NULL);

			new_rfk = g_mem_chunk_alloc(rpc_fragment_key_chunk);
			new_rfk->conv_id = rfk->conv_id;
			new_rfk->seq = seq + len;
			new_rfk->offset = rfk->offset + len - 4;
			new_rfk->start_seq = rfk->start_seq;
			g_hash_table_insert(rpc_reassembly_table, new_rfk,
			    new_rfk);

			/*
			 * This is part of a fragmented record,
			 * but it's not the first part.
			 * Show it as a record marker plus data, under
			 * a top-level tree for this protocol.
			 */
			make_frag_tree(frag_tvb, tree, proto, ett,rpc_rm);

			/*
			 * No more processing need be done, as we don't
			 * have a complete record.
			 */
			return len;
		}

		/*
		 * This is the first fragment we've seen, and it's also
		 * the last fragment; that means the record wasn't
		 * fragmented.  Hand the dissector the tvbuff for the
		 * fragment as the tvbuff for the record.
		 */
		rec_tvb = frag_tvb;
		ipfd_head = NULL;
	} else {
		/*
		 * OK, this fragment was found, which means it continues
		 * a record.  This means we must defragment it.
		 * Add it to the defragmentation lists.
		 */
		ipfd_head = fragment_add(tvb, offset + 4, pinfo,
		    rfk->start_seq, rpc_fragment_table,
		    rfk->offset, len - 4, !(rpc_rm & RPC_RM_LASTFRAG));

		if (ipfd_head == NULL) {
			/*
			 * fragment_add() returned NULL, This means that
			 * defragmentation is not completed yet.
			 *
			 * We must add an entry to the hash table with
			 * the sequence number following this fragment
			 * as the starting sequence number, so that when
			 * we see that fragment we'll find that entry.
			 *
			 * XXX - as TCP stream data is not currently
			 * guaranteed to be provided in order to dissectors,
			 * RPC fragments aren't guaranteed to be provided
			 * in order, either.
			 */
			new_rfk = g_mem_chunk_alloc(rpc_fragment_key_chunk);
			new_rfk->conv_id = rfk->conv_id;
			new_rfk->seq = seq + len;
			new_rfk->offset = rfk->offset + len - 4;
			new_rfk->start_seq = rfk->start_seq;
			g_hash_table_insert(rpc_reassembly_table, new_rfk,
			    new_rfk);

			/*
			 * This is part of a fragmented record,
			 * but it's not the first part.
			 * Show it as a record marker plus data, under
			 * a top-level tree for this protocol,
			 * but don't hand it to the dissector
			 */
			make_frag_tree(frag_tvb, tree, proto, ett, rpc_rm);

			/*
			 * No more processing need be done, as we don't
			 * have a complete record.
			 */
			return len;
		}

		/*
		 * It's completely defragmented.
		 *
		 * We only call subdissector for the last fragment.
		 * XXX - this assumes in-order delivery of RPC
		 * fragments, which requires in-order delivery of TCP
		 * segments.
		 */
		if (!(rpc_rm & RPC_RM_LASTFRAG)) {
			/*
			 * Well, it's defragmented, but this isn't
			 * the last fragment; this probably means
			 * this isn't the first pass, so we don't
			 * need to start defragmentation.
			 *
			 * This is part of a fragmented record,
			 * but it's not the first part.
			 * Show it as a record marker plus data, under
			 * a top-level tree for this protocol,
			 * but don't show it to the dissector.
			 */
			make_frag_tree(frag_tvb, tree, proto, ett, rpc_rm);

			/*
			 * No more processing need be done, as we
			 * only disssect the data with the last
			 * fragment.
			 */
			return len;
		}

		/*
		 * OK, this is the last segment.
		 * Create a tvbuff for the defragmented
		 * record.
		 */

		/*
		 * Create a new TVB structure for
		 * defragmented data.
		 */
		rec_tvb = tvb_new_real_data(ipfd_head->data,
		    ipfd_head->datalen, ipfd_head->datalen);

		/*
		 * Add this tvb as a child to the original
		 * one.
		 */
		tvb_set_child_real_data_tvbuff(tvb, rec_tvb);

		/*
		 * Add defragmented data to the data source list.
		 */
		add_new_data_source(pinfo, rec_tvb, "Defragmented");
	}

	/*
	 * We have something to hand to the RPC message
	 * dissector.
	 */
	if (!call_message_dissector(tvb, rec_tvb, pinfo, tree,
	    frag_tvb, dissector, ipfd_head, rpc_rm))
		return 0;	/* not RPC */
	return len;
}

/*
 * Can return:
 *
 *	NEED_MORE_DATA, if we don't have enough data to dissect anything;
 *
 *	IS_RPC, if we dissected at least one message in its entirety
 *	as RPC;
 *
 *	IS_NOT_RPC, if we found no RPC message.
 */
typedef enum {
	NEED_MORE_DATA,
	IS_RPC,
	IS_NOT_RPC
} rpc_tcp_return_t;

static rpc_tcp_return_t
dissect_rpc_tcp_common(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
    gboolean is_heur)
{
	int offset = 0;
	gboolean saw_rpc = FALSE;
	int len;

	while (tvb_reported_length_remaining(tvb, offset) != 0) {
		/*
		 * Process this fragment.
		 */
		len = dissect_rpc_fragment(tvb, offset, pinfo, tree,
		    dissect_rpc_message, is_heur, proto_rpc, ett_rpc,
		    rpc_defragment);
		if (len < 0) {
			/*
			 * We need more data from the TCP stream for
			 * this fragment.
			 */
			return NEED_MORE_DATA;
		}
		if (len == 0) {
			/*
			 * It's not RPC.  Stop processing.
			 */
			break;
		}

		offset += len;
		saw_rpc = TRUE;
	}
	return saw_rpc ? IS_RPC : IS_NOT_RPC;
}

static gboolean
dissect_rpc_tcp_heur(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	switch (dissect_rpc_tcp_common(tvb, pinfo, tree, TRUE)) {

	case IS_RPC:
		return TRUE;

	case IS_NOT_RPC:
		return FALSE;

	default:
		/* "Can't happen" */
		g_assert_not_reached();
		return FALSE;
	}
}

static void
dissect_rpc_tcp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	if (dissect_rpc_tcp_common(tvb, pinfo, tree, FALSE) == IS_NOT_RPC)
		dissect_rpc_continuation(tvb, pinfo, tree);
}

/* Discard any state we've saved. */
static void
rpc_init_protocol(void)
{
	if (rpc_calls != NULL) {
		g_hash_table_destroy(rpc_calls);
		rpc_calls = NULL;
	}
	if (rpc_indir_calls != NULL) {
		g_hash_table_destroy(rpc_indir_calls);
		rpc_indir_calls = NULL;
	}
	if (rpc_call_info_key_chunk != NULL) {
		g_mem_chunk_destroy(rpc_call_info_key_chunk);
		rpc_call_info_key_chunk = NULL;
	}
	if (rpc_call_info_value_chunk != NULL) {
		g_mem_chunk_destroy(rpc_call_info_value_chunk);
		rpc_call_info_value_chunk = NULL;
	}
	if (rpc_fragment_key_chunk != NULL) {
		g_mem_chunk_destroy(rpc_fragment_key_chunk);
		rpc_fragment_key_chunk = NULL;
	}
	if (rpc_reassembly_table != NULL) {
		g_hash_table_destroy(rpc_reassembly_table);
		rpc_reassembly_table = NULL;
	}

	rpc_calls = g_hash_table_new(rpc_call_hash, rpc_call_equal);
	rpc_indir_calls = g_hash_table_new(rpc_call_hash, rpc_call_equal);
	rpc_call_info_key_chunk = g_mem_chunk_new("call_info_key_chunk",
	    sizeof(rpc_call_info_key),
	    200 * sizeof(rpc_call_info_key),
	    G_ALLOC_ONLY);
	rpc_call_info_value_chunk = g_mem_chunk_new("call_info_value_chunk",
	    sizeof(rpc_call_info_value),
	    200 * sizeof(rpc_call_info_value),
	    G_ALLOC_ONLY);
	rpc_fragment_key_chunk = g_mem_chunk_new("rpc_fragment_key_chunk",
	    sizeof(rpc_fragment_key),
	    rpc_fragment_init_count*sizeof(rpc_fragment_key),
	    G_ALLOC_ONLY);
	rpc_reassembly_table = g_hash_table_new(rpc_fragment_hash,
	    rpc_fragment_equal);

	fragment_table_init(&rpc_fragment_table);
}

/* will be called once from register.c at startup time */
void
proto_register_rpc(void)
{
	static hf_register_info hf[] = {
		{ &hf_rpc_lastfrag, {
			"Last Fragment", "rpc.lastfrag", FT_BOOLEAN, 32,
			&yesno, RPC_RM_LASTFRAG, "Last Fragment", HFILL }},
		{ &hf_rpc_fraglen, {
			"Fragment Length", "rpc.fraglen", FT_UINT32, BASE_DEC,
			NULL, RPC_RM_FRAGLEN, "Fragment Length", HFILL }},
		{ &hf_rpc_xid, {
			"XID", "rpc.xid", FT_UINT32, BASE_HEX,
			NULL, 0, "XID", HFILL }},
		{ &hf_rpc_msgtype, {
			"Message Type", "rpc.msgtyp", FT_UINT32, BASE_DEC,
			VALS(rpc_msg_type), 0, "Message Type", HFILL }},
		{ &hf_rpc_state_reply, {
			"Reply State", "rpc.replystat", FT_UINT32, BASE_DEC,
			VALS(rpc_reply_state), 0, "Reply State", HFILL }},
		{ &hf_rpc_state_accept, {
			"Accept State", "rpc.state_accept", FT_UINT32, BASE_DEC,
			VALS(rpc_accept_state), 0, "Accept State", HFILL }},
		{ &hf_rpc_state_reject, {
			"Reject State", "rpc.state_reject", FT_UINT32, BASE_DEC,
			VALS(rpc_reject_state), 0, "Reject State", HFILL }},
		{ &hf_rpc_state_auth, {
			"Auth State", "rpc.state_auth", FT_UINT32, BASE_DEC,
			VALS(rpc_auth_state), 0, "Auth State", HFILL }},
		{ &hf_rpc_version, {
			"RPC Version", "rpc.version", FT_UINT32, BASE_DEC,
			NULL, 0, "RPC Version", HFILL }},
		{ &hf_rpc_version_min, {
			"RPC Version (Minimum)", "rpc.version.min", FT_UINT32,
			BASE_DEC, NULL, 0, "Program Version (Minimum)", HFILL }},
		{ &hf_rpc_version_max, {
			"RPC Version (Maximum)", "rpc.version.max", FT_UINT32,
			BASE_DEC, NULL, 0, "RPC Version (Maximum)", HFILL }},
		{ &hf_rpc_program, {
			"Program", "rpc.program", FT_UINT32, BASE_DEC,
			NULL, 0, "Program", HFILL }},
		{ &hf_rpc_programversion, {
			"Program Version", "rpc.programversion", FT_UINT32,
			BASE_DEC, NULL, 0, "Program Version", HFILL }},
		{ &hf_rpc_programversion_min, {
			"Program Version (Minimum)", "rpc.programversion.min", FT_UINT32,
			BASE_DEC, NULL, 0, "Program Version (Minimum)", HFILL }},
		{ &hf_rpc_programversion_max, {
			"Program Version (Maximum)", "rpc.programversion.max", FT_UINT32,
			BASE_DEC, NULL, 0, "Program Version (Maximum)", HFILL }},
		{ &hf_rpc_procedure, {
			"Procedure", "rpc.procedure", FT_UINT32, BASE_DEC,
			NULL, 0, "Procedure", HFILL }},
		{ &hf_rpc_auth_flavor, {
			"Flavor", "rpc.auth.flavor", FT_UINT32, BASE_DEC,
			VALS(rpc_auth_flavor), 0, "Flavor", HFILL }},
		{ &hf_rpc_auth_length, {
			"Length", "rpc.auth.length", FT_UINT32, BASE_DEC,
			NULL, 0, "Length", HFILL }},
		{ &hf_rpc_auth_stamp, {
			"Stamp", "rpc.auth.stamp", FT_UINT32, BASE_HEX,
			NULL, 0, "Stamp", HFILL }},
		{ &hf_rpc_auth_uid, {
			"UID", "rpc.auth.uid", FT_UINT32, BASE_DEC,
			NULL, 0, "UID", HFILL }},
		{ &hf_rpc_auth_gid, {
			"GID", "rpc.auth.gid", FT_UINT32, BASE_DEC,
			NULL, 0, "GID", HFILL }},
		{ &hf_rpc_authgss_v, {
			"GSS Version", "rpc.authgss.version", FT_UINT32,
			BASE_DEC, NULL, 0, "GSS Version", HFILL }},
		{ &hf_rpc_authgss_proc, {
			"GSS Procedure", "rpc.authgss.procedure", FT_UINT32,
			BASE_DEC, VALS(rpc_authgss_proc), 0, "GSS Procedure", HFILL }},
		{ &hf_rpc_authgss_seq, {
			"GSS Sequence Number", "rpc.authgss.seqnum", FT_UINT32,
			BASE_DEC, NULL, 0, "GSS Sequence Number", HFILL }},
		{ &hf_rpc_authgss_svc, {
			"GSS Service", "rpc.authgss.service", FT_UINT32,
			BASE_DEC, VALS(rpc_authgss_svc), 0, "GSS Service", HFILL }},
		{ &hf_rpc_authgss_ctx, {
			"GSS Context", "rpc.authgss.context", FT_BYTES,
			BASE_HEX, NULL, 0, "GSS Context", HFILL }},
		{ &hf_rpc_authgss_major, {
			"GSS Major Status", "rpc.authgss.major", FT_UINT32,
			BASE_DEC, NULL, 0, "GSS Major Status", HFILL }},
		{ &hf_rpc_authgss_minor, {
			"GSS Minor Status", "rpc.authgss.minor", FT_UINT32,
			BASE_DEC, NULL, 0, "GSS Minor Status", HFILL }},
		{ &hf_rpc_authgss_window, {
			"GSS Sequence Window", "rpc.authgss.window", FT_UINT32,
			BASE_DEC, NULL, 0, "GSS Sequence Window", HFILL }},
		{ &hf_rpc_authgss_token_length, {
			"GSS Token Length", "rpc.authgss.token_length", FT_UINT32,
			BASE_DEC, NULL, 0, "GSS Token Length", HFILL }},
		{ &hf_rpc_authgss_data_length, {
			"Length", "rpc.authgss.data.length", FT_UINT32,
			BASE_DEC, NULL, 0, "Length", HFILL }},
		{ &hf_rpc_authgss_data, {
			"GSS Data", "rpc.authgss.data", FT_BYTES,
			BASE_HEX, NULL, 0, "GSS Data", HFILL }},
		{ &hf_rpc_authgss_checksum, {
			"GSS Checksum", "rpc.authgss.checksum", FT_BYTES,
			BASE_HEX, NULL, 0, "GSS Checksum", HFILL }},
		{ &hf_rpc_authdes_namekind, {
			"Namekind", "rpc.authdes.namekind", FT_UINT32, BASE_DEC,
			VALS(rpc_authdes_namekind), 0, "Namekind", HFILL }},
		{ &hf_rpc_authdes_netname, {
			"Netname", "rpc.authdes.netname", FT_STRING,
			BASE_DEC, NULL, 0, "Netname", HFILL }},
		{ &hf_rpc_authdes_convkey, {
			"Conversation Key (encrypted)", "rpc.authdes.convkey", FT_UINT32,
			BASE_HEX, NULL, 0, "Conversation Key (encrypted)", HFILL }},
		{ &hf_rpc_authdes_window, {
			"Window (encrypted)", "rpc.authdes.window", FT_UINT32,
			BASE_HEX, NULL, 0, "Windows (encrypted)", HFILL }},
		{ &hf_rpc_authdes_nickname, {
			"Nickname", "rpc.authdes.nickname", FT_UINT32,
			BASE_HEX, NULL, 0, "Nickname", HFILL }},
		{ &hf_rpc_authdes_timestamp, {
			"Timestamp (encrypted)", "rpc.authdes.timestamp", FT_UINT32,
			BASE_HEX, NULL, 0, "Timestamp (encrypted)", HFILL }},
		{ &hf_rpc_authdes_windowverf, {
			"Window verifier (encrypted)", "rpc.authdes.windowverf", FT_UINT32,
			BASE_HEX, NULL, 0, "Window verifier (encrypted)", HFILL }},
		{ &hf_rpc_authdes_timeverf, {
			"Timestamp verifier (encrypted)", "rpc.authdes.timeverf", FT_UINT32,
			BASE_HEX, NULL, 0, "Timestamp verifier (encrypted)", HFILL }},
		{ &hf_rpc_auth_machinename, {
			"Machine Name", "rpc.auth.machinename", FT_STRING,
			BASE_DEC, NULL, 0, "Machine Name", HFILL }},
		{ &hf_rpc_dup, {
			"Duplicate Transaction", "rpc.dup", FT_UINT32, BASE_DEC,
			NULL, 0, "Duplicate Transaction", HFILL }},
		{ &hf_rpc_call_dup, {
			"Duplicate Call", "rpc.call.dup", FT_UINT32, BASE_DEC,
			NULL, 0, "Duplicate Call", HFILL }},
		{ &hf_rpc_reply_dup, {
			"Duplicate Reply", "rpc.reply.dup", FT_UINT32, BASE_DEC,
			NULL, 0, "Duplicate Reply", HFILL }},
		{ &hf_rpc_value_follows, {
			"Value Follows", "rpc.value_follows", FT_BOOLEAN, BASE_NONE,
			&yesno, 0, "Value Follows", HFILL }},
		{ &hf_rpc_array_len, {
			"num", "rpc.array.len", FT_UINT32, BASE_DEC,
			NULL, 0, "Length of RPC array", HFILL }},

		{ &hf_rpc_time, {
			"Time from request", "rpc.time", FT_RELATIVE_TIME, BASE_NONE,
			NULL, 0, "Time between Request and Reply for ONC-RPC calls", HFILL }},

		{ &hf_rpc_fragment_overlap,
		{ "Fragment overlap",	"rpc.fragment.overlap", FT_BOOLEAN, BASE_NONE, NULL, 0x0,
			"Fragment overlaps with other fragments", HFILL }},

		{ &hf_rpc_fragment_overlap_conflict,
		{ "Conflicting data in fragment overlap",	"rpc.fragment.overlap.conflict", FT_BOOLEAN, BASE_NONE, NULL, 0x0,
			"Overlapping fragments contained conflicting data", HFILL }},

		{ &hf_rpc_fragment_multiple_tails,
		{ "Multiple tail fragments found",	"rpc.fragment.multipletails", FT_BOOLEAN, BASE_NONE, NULL, 0x0,
			"Several tails were found when defragmenting the packet", HFILL }},

		{ &hf_rpc_fragment_too_long_fragment,
		{ "Fragment too long",	"rpc.fragment.toolongfragment", FT_BOOLEAN, BASE_NONE, NULL, 0x0,
			"Fragment contained data past end of packet", HFILL }},

		{ &hf_rpc_fragment_error,
		{ "Defragmentation error", "rpc.fragment.error", FT_NONE, BASE_NONE, NULL, 0x0,
			"Defragmentation error due to illegal fragments", HFILL }},

		{ &hf_rpc_fragment,
		{ "RPC Fragment", "rpc.fragment", FT_NONE, BASE_NONE, NULL, 0x0,
			"RPC Fragment", HFILL }},

		{ &hf_rpc_fragments,
		{ "RPC Fragments", "rpc.fragments", FT_NONE, BASE_NONE, NULL, 0x0,
			"RPC Fragments", HFILL }},
	};
	static gint *ett[] = {
		&ett_rpc,
		&ett_rpc_fragments,
		&ett_rpc_fragment,
		&ett_rpc_fraghdr,
		&ett_rpc_string,
		&ett_rpc_cred,
		&ett_rpc_verf,
		&ett_rpc_gids,
		&ett_rpc_gss_token,
		&ett_rpc_gss_data,
		&ett_rpc_array,
	};
	module_t *rpc_module;

	proto_rpc = proto_register_protocol("Remote Procedure Call",
	    "RPC", "rpc");
	proto_register_field_array(proto_rpc, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));
	register_init_routine(&rpc_init_protocol);

	rpc_module = prefs_register_protocol(proto_rpc, NULL);
	prefs_register_bool_preference(rpc_module, "desegment_rpc_over_tcp",
		"Desegment all RPC-over-TCP messages",
		"Whether the RPC dissector should desegment all RPC-over-TCP messages",
		&rpc_desegment);
	prefs_register_bool_preference(rpc_module, "defragment_rpc_over_tcp",
		"Defragment all RPC-over-TCP messages",
		"Whether the RPC dissector should defragment multi-fragment RPC-over-TCP messages",
		&rpc_defragment);

	register_dissector("rpc", dissect_rpc, proto_rpc);
	rpc_handle = find_dissector("rpc");
	register_dissector("rpc-tcp", dissect_rpc_tcp, proto_rpc);
	rpc_tcp_handle = find_dissector("rpc-tcp");
	rpc_tap = register_tap("rpc");

	/*
	 * Init the hash tables.  Dissectors for RPC protocols must
	 * have a "handoff registration" routine that registers the
	 * protocol with RPC; they must not do it in their protocol
	 * registration routine, as their protocol registration
	 * routine might be called before this routine is called and
	 * thus might be called before the hash tables are initialized,
	 * but it's guaranteed that all protocol registration routines
	 * will be called before any handoff registration routines
	 * are called.
	 */
	rpc_progs = g_hash_table_new(rpc_prog_hash, rpc_prog_equal);
	rpc_procs = g_hash_table_new(rpc_proc_hash, rpc_proc_equal);
}

void
proto_reg_handoff_rpc(void)
{
	heur_dissector_add("tcp", dissect_rpc_tcp_heur, proto_rpc);
	heur_dissector_add("udp", dissect_rpc_heur, proto_rpc);
	gssapi_handle = find_dissector("gssapi");
	data_handle = find_dissector("data");
}
