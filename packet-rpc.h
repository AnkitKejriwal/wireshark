/* packet-rpc.h
 *
 * $Id: packet-rpc.h,v 1.37 2002/08/28 21:00:29 jmayer Exp $
 *
 * (c) 1999 Uwe Girlich
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@ethereal.com>
 * Copyright 1998 Gerald Combs
 *
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

#ifndef __PACKET_RPC_H__
#define __PACKET_RPC_H__

#include <glib.h>
#include <epan/packet.h>
#include <epan/conversation.h>

#define RPC_CALL 0
#define RPC_REPLY 1

#define AUTH_NULL 0
#define AUTH_UNIX 1
#define AUTH_SHORT 2
#define AUTH_DES 3
#define RPCSEC_GSS 6

#define MSG_ACCEPTED 0
#define MSG_DENIED 1

#define SUCCESS 0
#define PROG_UNAVAIL 1
#define PROG_MISMATCH 2
#define PROC_UNAVAIL 3
#define GARBAGE_ARGS 4

#define RPC_MISMATCH 0
#define AUTH_ERROR 1

#define AUTH_BADCRED 1
#define AUTH_REJECTEDCRED 2
#define AUTH_BADVERF 3
#define AUTH_REJECTEDVERF 4
#define AUTH_TOOWEAK 5
#define RPCSEC_GSSCREDPROB 13
#define RPCSEC_GSSCTXPROB 14

#define RPCSEC_GSS_DATA 0
#define RPCSEC_GSS_INIT 1
#define RPCSEC_GSS_CONTINUE_INIT 2
#define RPCSEC_GSS_DESTROY 3

#define RPCSEC_GSS_SVC_NONE 1
#define RPCSEC_GSS_SVC_INTEGRITY 2
#define RPCSEC_GSS_SVC_PRIVACY 3

#define AUTHDES_NAMEKIND_FULLNAME 0
#define AUTHDES_NAMEKIND_NICKNAME 1

extern value_string rpc_authgss_svc[];
typedef enum {
	FLAVOR_UNKNOWN,		/* authentication flavor unknown */
	FLAVOR_NOT_GSSAPI,	/* flavor isn't GSSAPI */
	FLAVOR_GSSAPI_NO_INFO,	/* flavor is GSSAPI, procedure & service unknown */
	FLAVOR_GSSAPI		/* flavor is GSSAPI, procedure & service known */
} flavor_t;

typedef struct _rpc_call_info_value {
	guint32	req_num;	/* frame number of first request seen */
	guint32	rep_num;	/* frame number of first reply seen */
	guint32	prog;
	guint32	vers;
	guint32	proc;
	guint32	xid;
	flavor_t flavor;
	guint32 gss_proc;
	guint32 gss_svc;
	struct _rpc_proc_info_value*	proc_info;
	gboolean request;	/* Is this a request or not ?*/
	nstime_t req_time;
	void *private_data;
} rpc_call_info_value;


typedef int (dissect_function_t)(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree* tree);

typedef struct _vsff {
	guint32	value;
	gchar   *strptr;
	dissect_function_t *dissect_call;
	dissect_function_t *dissect_reply;
} vsff;

extern const value_string rpc_auth_flavor[];

extern void rpc_init_proc_table(guint prog, guint vers, const vsff *proc_table);
extern void rpc_init_prog(int proto, guint32 prog, int ett);
extern char *rpc_prog_name(guint32 prog);
extern char *rpc_proc_name(guint32 prog, guint32 vers, guint32 proc);

extern unsigned int rpc_roundup(unsigned int a);
extern int dissect_rpc_bool(tvbuff_t *tvb,
	proto_tree *tree, int hfindex, int offset);
extern int dissect_rpc_string(tvbuff_t *tvb,
	proto_tree *tree, int hfindex, int offset, char **string_buffer_ret);
extern int dissect_rpc_data(tvbuff_t *tvb,
	proto_tree *tree, int hfindex, int offset);
extern int dissect_rpc_list(tvbuff_t *tvb, packet_info *pinfo,
	proto_tree *tree, int offset, dissect_function_t *rpc_list_dissector);
extern int dissect_rpc_array(tvbuff_t *tvb, packet_info *pinfo,
	proto_tree *tree, int offset, dissect_function_t *rpc_array_dissector,
	int hfindex);
extern int dissect_rpc_uint32(tvbuff_t *tvb,
	proto_tree *tree, int hfindex, int offset);
extern int dissect_rpc_uint64(tvbuff_t *tvb,
	proto_tree *tree, int hfindex, int offset);

extern int dissect_rpc_indir_call(tvbuff_t *tvb, packet_info *pinfo,
	proto_tree *tree, int offset, int args_id, guint32 prog, guint32 vers,
	guint32 proc);
extern int dissect_rpc_indir_reply(tvbuff_t *tvb, packet_info *pinfo,
	proto_tree *tree, int offset, int result_id, int prog_id, int vers_id,
	int proc_id);

#endif /* packet-rpc.h */

