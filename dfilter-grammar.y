%{

/* dfilter-grammar.y
 * Parser for display filters
 *
 * $Id: dfilter-grammar.y,v 1.34 1999/10/17 14:15:06 deniel Exp $
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@zing.org>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif

#ifdef NEED_SNPRINTF_H
# ifdef HAVE_STDARG_H
#  include <stdarg.h>
# else
#  include <varargs.h>
# endif
# include "snprintf.h"
#endif

#ifndef __GLIB_H__
#include <glib.h>
#endif

#include <string.h>

#ifndef _STDLIB_H
#include <stdlib.h>
#endif

#ifndef __PROTO_H__
#include "proto.h"
#endif

#ifndef __PACKET_H__
#include "packet.h"
#endif

#ifndef __DFILTER_H__
#include "dfilter.h"
#endif

#include "dfilter-int.h"

#ifndef __RESOLV_H__
#include "resolv.h"
#endif

static GNode* dfilter_mknode_join(GNode *n1, enum node_type ntype, int operand, GNode *n2);
static GNode* dfilter_mknode_unary(int operand, GNode *n2);
static GNode* dfilter_mknode_numeric_variable(gint id);
static GNode* dfilter_mknode_numeric_value(guint32 val);
static GNode* dfilter_mknode_ether_value(gchar*);
static GNode* dfilter_mknode_ether_variable(gint id);
static GNode* dfilter_mknode_ipxnet_value(guint32);
static GNode* dfilter_mknode_ipxnet_variable(gint id);
static GNode* dfilter_mknode_ipv4_value(char *host);
static GNode* dfilter_mknode_ipv4_variable(gint id);
static GNode* dfilter_mknode_ipv6_value(char *host);
static GNode* dfilter_mknode_ipv6_variable(gint id);
static GNode* dfilter_mknode_existence(gint id);
static GNode* dfilter_mknode_bytes_value(GByteArray *barray);
static GNode* dfilter_mknode_bytes_variable(gint id, gint offset, guint length);

static guint32 string_to_value(char *s);
static int ether_str_to_guint8_array(const char *s, guint8 *mac);
static guint dfilter_get_bytes_variable_offset(GNode *gnode);
static guint dfilter_get_bytes_value_length(GNode* gnode);
static void dfilter_set_bytes_variable_length(GNode *gnode, guint length);
static guint dfilter_get_bytes_variable_length(GNode *gnode);
static gint dfilter_get_bytes_variable_field_registered_length(GNode *gnode);
static char* dfilter_get_variable_abbrev(GNode *gnode);
static int check_bytes_variable_sanity(GNode *gnode);

/* This is the dfilter we're currently processing. It's how
 * dfilter_compile communicates with us.
 */
dfilter *global_df = NULL;

%}

%union {
	gint		operand;	/* logical, relation, alternation */
	struct {
		gint	id;
		gint	type;		/* using macros defined below, in this yacc grammar */
	} variable;
	GNode*		node;
	gchar*		string;
	struct {
		gint	offset;
		guint	length;
	} byte_range;
}

%type <node>	statement expression relation
%type <node>	numeric_value numeric_variable
%type <node>	ether_value ether_variable
%type <node>	ipxnet_value ipxnet_variable
%type <node>	ipv4_value ipv4_variable
%type <node>	ipv6_value ipv6_variable
%type <node>	variable_name
%type <node>	bytes_value bytes_variable

%type <operand>	numeric_relation
%type <operand>	equality_relation
%type <operand>	bytes_relation

%type <variable>	any_variable_type

%token <variable>	T_FT_UINT8
%token <variable>	T_FT_UINT16
%token <variable>	T_FT_UINT32
%token <variable>	T_FT_INT8
%token <variable>	T_FT_INT16
%token <variable>	T_FT_INT32
%token <variable>	T_FT_ETHER
%token <variable>	T_FT_IPv4
%token <variable>	T_FT_IPv6
%token <variable>	T_FT_NONE
%token <variable>	T_FT_BYTES
%token <variable>	T_FT_BOOLEAN
%token <variable>	T_FT_STRING
%token <variable>	T_FT_IPXNET

%token <string>	 	T_VAL_UNQUOTED_STRING
%token <string>		T_VAL_BYTE_STRING
%token <string>		T_VAL_NUMBER_STRING
%token <byte_range>	T_VAL_BYTE_RANGE

%token <operand>	TOK_AND TOK_OR TOK_NOT TOK_XOR
%token <operand>	TOK_EQ TOK_NE TOK_GT TOK_GE TOK_LT TOK_LE

%expect 4
%left TOK_AND
%left TOK_OR TOK_XOR
%nonassoc TOK_NOT

%%

statement: expression
		{
			global_df->dftree = $1;
		}
	|	/* NULL */ { if (global_df != NULL) global_df->dftree = NULL; }
	;

expression:	'(' expression ')' { $$ = $2; }
	|	expression TOK_AND expression { $$ = dfilter_mknode_join($1, logical, $2, $3); }
	|	expression TOK_OR expression { $$ = dfilter_mknode_join($1, logical, $2, $3); }
	|	expression TOK_XOR expression { $$ = dfilter_mknode_join($1, logical, $2, $3); }
	|	TOK_NOT expression { $$ = dfilter_mknode_unary(TOK_NOT, $2); }
	|	relation { $$ = $1; }
	|	variable_name { $$ = $1; }
	|	expression error { YYABORT; }
	;

relation:	numeric_variable numeric_relation numeric_value
		{
			$$ = dfilter_mknode_join($1, relation, $2, $3);
		}
	|	numeric_variable numeric_relation numeric_variable
		{
			$$ = dfilter_mknode_join($1, relation, $2, $3);
		}

	|	ether_variable equality_relation ether_value
		{
			$$ = dfilter_mknode_join($1, relation, $2, $3);
		}
	|	ether_variable equality_relation ether_variable
		{
			$$ = dfilter_mknode_join($1, relation, $2, $3);
		}

	|	ipxnet_variable equality_relation ipxnet_value
		{
			$$ = dfilter_mknode_join($1, relation, $2, $3);
		}
	|	ipxnet_variable equality_relation ipxnet_variable
		{
			$$ = dfilter_mknode_join($1, relation, $2, $3);
		}


	|	ipv4_variable numeric_relation ipv4_value
		{
			$$ = dfilter_mknode_join($1, relation, $2, $3);
		}
	|	ipv4_variable numeric_relation ipv4_variable
		{
			$$ = dfilter_mknode_join($1, relation, $2, $3);
		}

	|	ipv6_variable equality_relation ipv6_value
		{
			$$ = dfilter_mknode_join($1, relation, $2, $3);
		}
	|	ipv6_variable equality_relation ipv6_variable
		{
			$$ = dfilter_mknode_join($1, relation, $2, $3);
		}

	|	bytes_variable bytes_relation bytes_value
		{
			int a_len, b_len;

			a_len = dfilter_get_bytes_variable_length($1);
			b_len = dfilter_get_bytes_value_length($3);

			if (a_len == 0) {
				dfilter_set_bytes_variable_length($1, b_len);
				a_len = b_len;
			}

			if (!check_bytes_variable_sanity($1)) {
				YYERROR;
			}

			if (a_len != b_len) {
				dfilter_fail("Field \"%s\" has %u byte%s being compared, but %u byte%s "
					"%s supplied.",
					dfilter_get_variable_abbrev($1),
					a_len, plurality(a_len, "", "s"),
					b_len, plurality(b_len, "", "s"),
					       plurality(b_len, "was", "were"));
				YYERROR;
			}

			$$ = dfilter_mknode_join($1, relation, $2, $3);
		}
	|	bytes_variable bytes_relation bytes_variable
		{
			int a_len, b_len;

			a_len = dfilter_get_bytes_variable_length($1);
			b_len = dfilter_get_bytes_variable_length($3);

			if (!check_bytes_variable_sanity($1)) {
				YYERROR;
			}

			if (!check_bytes_variable_sanity($3)) {
				YYERROR;
			}

			if (a_len != b_len) {
				dfilter_fail("Fields \"%s\" and \"%s\" are being compared with "
					"disparate lengths of %u byte%s and %u byte%s.",
					dfilter_get_variable_abbrev($1),
					dfilter_get_variable_abbrev($3),
					a_len, plurality(a_len, "", "s"),
					b_len, plurality(b_len, "", "s"));
				YYERROR;
			}

			$$ = dfilter_mknode_join($1, relation, $2, $3);
		}

	;


numeric_value:	T_VAL_NUMBER_STRING
	{
		$$ = dfilter_mknode_numeric_value(string_to_value($1));
		g_free($1);
	 }
	;

ether_value:	T_VAL_BYTE_STRING
	{
		$$ = dfilter_mknode_ether_value($1);
		g_free($1);
		if ($$ == NULL) {
			YYERROR;
		}
	}
	;

ipxnet_value:	T_VAL_NUMBER_STRING
	{
		$$ = dfilter_mknode_ipxnet_value(string_to_value($1));
		g_free($1);
	}
	;

ipv4_value:	T_VAL_UNQUOTED_STRING
		{
			$$ = dfilter_mknode_ipv4_value($1);
			g_free($1);
			if ($$ == NULL) {
				YYERROR;
			}
		}

	|	T_VAL_BYTE_STRING
		{
			$$ = dfilter_mknode_ipv4_value($1);
			g_free($1);
			if ($$ == NULL) {
				YYERROR;
			}
		}
	;

ipv6_value:	T_VAL_UNQUOTED_STRING
		{
			$$ = dfilter_mknode_ipv6_value($1);
			g_free($1);
			if ($$ == NULL) {
				YYERROR;
			}
		}

	|	T_VAL_BYTE_STRING
		{
			$$ = dfilter_mknode_ipv6_value($1);
			g_free($1);
			if ($$ == NULL) {
				YYERROR;
			}
		}
	;

bytes_value:	T_VAL_BYTE_STRING
	{
		GByteArray	*barray;

		/* the next function appends to list_of_byte_arrays for me */
		barray = byte_str_to_guint8_array($1);
		$$ = dfilter_mknode_bytes_value(barray);
		g_free($1);
	}

	|	T_VAL_NUMBER_STRING
	{
		guint32		val32 = string_to_value($1);
		guint8		val8;
		GByteArray	*barray;

		if (val32 > 0xff) {
			dfilter_fail("The value \"%s\" cannot be stored in a single-byte byte-string. "
				"Use the multi-byte \"xx:yy\" representation.", $1);
			YYERROR;
		}
		val8 = (guint8) val32;
		barray = g_byte_array_new();
		global_df->list_of_byte_arrays = g_slist_append(global_df->list_of_byte_arrays, barray);
		g_byte_array_append(barray, &val8, 1);

		$$ = dfilter_mknode_bytes_value(barray);
		g_free($1);
	}
	;

numeric_variable:	T_FT_UINT8	{ $$ = dfilter_mknode_numeric_variable($1.id); }
	|		T_FT_UINT16	{ $$ = dfilter_mknode_numeric_variable($1.id); }
	|		T_FT_UINT32	{ $$ = dfilter_mknode_numeric_variable($1.id); }
	|		T_FT_INT8	{ $$ = dfilter_mknode_numeric_variable($1.id); }
	|		T_FT_INT16	{ $$ = dfilter_mknode_numeric_variable($1.id); }
	|		T_FT_INT32	{ $$ = dfilter_mknode_numeric_variable($1.id); }
	;

ether_variable:		T_FT_ETHER	{ $$ = dfilter_mknode_ether_variable($1.id); }
	;

ipxnet_variable:	T_FT_IPXNET	{ $$ = dfilter_mknode_ipxnet_variable($1.id); }
	;

ipv4_variable:		T_FT_IPv4	{ $$ = dfilter_mknode_ipv4_variable($1.id); }
	;

ipv6_variable:		T_FT_IPv6	{ $$ = dfilter_mknode_ipv6_variable($1.id); }
	;

variable_name:		any_variable_type
	{
		GNode	*variable;
		GNode	*value;

		if ($1.type == T_FT_BOOLEAN) {
			/* Make "variable == TRUE" for BOOLEAN variable */
			variable = dfilter_mknode_numeric_variable($1.id);
			value = dfilter_mknode_numeric_value(TRUE);
			$$ = dfilter_mknode_join(variable, relation, TOK_EQ, value);
		}
		else {
			$$ = dfilter_mknode_existence($1.id);
		}
	}
	;

bytes_variable:		any_variable_type T_VAL_BYTE_RANGE
		{
			$$ = dfilter_mknode_bytes_variable($1.id, $2.offset, $2.length);
		}
	;

any_variable_type:	T_FT_UINT8 { $$ = $1; }
	|		T_FT_UINT16 { $$ = $1; }
	|		T_FT_UINT32 { $$ = $1; }
	|		T_FT_INT8 { $$ = $1; }
	|		T_FT_INT16 { $$ = $1; }
	|		T_FT_INT32 { $$ = $1; }
	|		T_FT_ETHER { $$ = $1; }
	|		T_FT_IPv4 { $$ = $1; }
	|		T_FT_IPv6 { $$ = $1; }
	|		T_FT_IPXNET { $$ = $1; }
	|		T_FT_NONE { $$ = $1; }
	|		T_FT_BYTES { $$ = $1; }
	|		T_FT_BOOLEAN { $$ = $1; }
	|		T_FT_STRING { $$ = $1; }
	;

numeric_relation:	TOK_EQ { $$ = TOK_EQ; }
	|		TOK_NE { $$ = TOK_NE; }
	|		TOK_GT { $$ = TOK_GT; }
	|		TOK_GE { $$ = TOK_GE; }
	|		TOK_LT { $$ = TOK_LT; }
	|		TOK_LE { $$ = TOK_LE; }
	;

equality_relation:	TOK_EQ { $$ = TOK_EQ; }
	|		TOK_NE { $$ = TOK_NE; }
	;

bytes_relation:		TOK_EQ { $$ = TOK_EQ; }
	|		TOK_NE { $$ = TOK_NE; }
	|		TOK_GT { $$ = TOK_GT; }
	|		TOK_LT { $$ = TOK_LT; }
	;

%%

static GNode*
dfilter_mknode_join(GNode *n1, enum node_type ntype, int operand, GNode *n2)
{
	dfilter_node	*node_root;
	GNode		*gnode_root;

	node_root = g_mem_chunk_alloc(global_df->node_memchunk);
	node_root->ntype = ntype;
	node_root->elem_size = 0;
	node_root->fill_array_func = NULL;
	node_root->check_relation_func = NULL;
	if (ntype == relation) {
		node_root->value.relation = operand;
	}
	else if (ntype == logical) {
		node_root->value.logical = operand;
	}
	else {
		g_assert_not_reached();
	}

	gnode_root = g_node_new(node_root);
	g_node_append(gnode_root, n1);
	g_node_append(gnode_root, n2);

	return gnode_root;
}

static GNode*
dfilter_mknode_unary(int operand, GNode *n2)
{
	dfilter_node	*node_root;
	GNode		*gnode_root;

	node_root = g_mem_chunk_alloc(global_df->node_memchunk);
	node_root->ntype = logical;
	node_root->value.logical = operand;
	node_root->elem_size = 0;
	node_root->fill_array_func = NULL;
	node_root->check_relation_func = NULL;

	gnode_root = g_node_new(node_root);
	g_node_append(gnode_root, n2);

	return gnode_root;
}


static GNode*
dfilter_mknode_numeric_variable(gint id)
{
	dfilter_node	*node;
	GNode		*gnode;

	node = g_mem_chunk_alloc(global_df->node_memchunk);
	node->ntype = variable;
	node->elem_size = sizeof(guint32);
	node->fill_array_func = fill_array_numeric_variable;
	node->check_relation_func = check_relation_numeric;
	node->value.variable = id;
	gnode = g_node_new(node);

	return gnode;
}

static GNode*
dfilter_mknode_ether_variable(gint id)
{
	dfilter_node	*node;
	GNode		*gnode;

	node = g_mem_chunk_alloc(global_df->node_memchunk);
	node->ntype = variable;
	node->elem_size = sizeof(guint8) * 6;
	node->fill_array_func = fill_array_ether_variable;
	node->check_relation_func = check_relation_ether;
	node->value.variable = id;
	gnode = g_node_new(node);

	return gnode;
}

static GNode*
dfilter_mknode_ipxnet_variable(gint id)
{
	dfilter_node	*node;
	GNode		*gnode;

	node = g_mem_chunk_alloc(global_df->node_memchunk);
	node->ntype = variable;
	node->elem_size = sizeof(guint8) * 4;
	node->fill_array_func = fill_array_numeric_variable; /* cheating ! */
	node->check_relation_func = check_relation_numeric; /* cheating ! */
	node->value.variable = id;
	gnode = g_node_new(node);

	return gnode;
}

static GNode*
dfilter_mknode_ipv4_variable(gint id)
{
	dfilter_node	*node;
	GNode		*gnode;

	node = g_mem_chunk_alloc(global_df->node_memchunk);
	node->ntype = variable;
	node->elem_size = sizeof(guint32);
	node->fill_array_func = fill_array_numeric_variable; /* cheating ! */
	node->check_relation_func = check_relation_numeric; /* cheating ! */
	node->value.variable = id;
	gnode = g_node_new(node);

	return gnode;
}

static GNode*
dfilter_mknode_ipv6_variable(gint id)
{
	dfilter_node	*node;
	GNode		*gnode;

	node = g_mem_chunk_alloc(global_df->node_memchunk);
	node->ntype = variable;
	node->elem_size = 16;
	node->fill_array_func = fill_array_ipv6_variable;
	node->check_relation_func = check_relation_ipv6; 
	node->value.variable = id;
	gnode = g_node_new(node);

	return gnode;
}

static GNode*
dfilter_mknode_bytes_variable(gint id, gint offset, guint length)
{
	dfilter_node	*node;
	GNode		*gnode;

	node = g_mem_chunk_alloc(global_df->node_memchunk);
	node->ntype = variable;
	node->elem_size = sizeof(GByteArray*);
	node->fill_array_func = fill_array_bytes_variable;
	node->check_relation_func = check_relation_bytes;
	node->value.variable = id;
	node->offset = offset;
	node->length = length;
	gnode = g_node_new(node);

	return gnode;
}

/* Gets length of variable represented by node from proto_register */
static gint
dfilter_get_bytes_variable_field_registered_length(GNode *gnode)
{
	dfilter_node	*node = gnode->data;

	/* Is this really a bytes_variable? */
	g_assert(node->fill_array_func = fill_array_bytes_variable);

	return proto_registrar_get_length(node->value.variable);
}

/* Sets the length of a bytes_variable node */
static void
dfilter_set_bytes_variable_length(GNode *gnode, guint length)
{
	dfilter_node	*node = gnode->data;

	/* Is this really a bytes_variable? */
	g_assert(node->fill_array_func = fill_array_bytes_variable);

	node->length = length;
}

/* Gets the length of a bytes_variable node */
static guint
dfilter_get_bytes_variable_length(GNode *gnode)
{
	dfilter_node	*node = gnode->data;

	/* Is this really a bytes_variable? */
	g_assert(node->fill_array_func = fill_array_bytes_variable);

	return node->length;
}

/* Gets the offset of a bytes_variable node */
static guint
dfilter_get_bytes_variable_offset(GNode *gnode)
{
	dfilter_node	*node = gnode->data;

	/* Is this really a bytes_variable? */
	g_assert(node->fill_array_func = fill_array_bytes_variable);

	return node->offset;
}

static char*
dfilter_get_variable_abbrev(GNode *gnode)
{
	dfilter_node	*node = gnode->data;

	return proto_registrar_get_abbrev(node->value.variable);
}

static GNode*
dfilter_mknode_numeric_value(guint32 val)
{
	dfilter_node	*node;
	GNode		*gnode;

	node = g_mem_chunk_alloc(global_df->node_memchunk);
	node->ntype = numeric;
	node->elem_size = sizeof(guint32);
	node->fill_array_func = fill_array_numeric_value;
	node->check_relation_func = check_relation_numeric;
	node->value.numeric = val;
	gnode = g_node_new(node);

	return gnode;
}

/* Returns NULL on bad parse of ETHER value */
static GNode*
dfilter_mknode_ether_value(gchar *byte_string)
{
	dfilter_node	*node;
	GNode		*gnode;

	node = g_mem_chunk_alloc(global_df->node_memchunk);
	node->ntype = ether;
	node->elem_size = sizeof(guint8) * 6;
	node->fill_array_func = fill_array_ether_value;
	node->check_relation_func = check_relation_ether;

	if (!ether_str_to_guint8_array(byte_string, &node->value.ether[0])) {
		/* Rather than free the mem_chunk allocation, let it
		 * stay. It will be cleaned up when "dfilter_compile()"
		 * calls "dfilter_destroy()". */
		dfilter_fail("\"%s\" is not a valid hardware address.",
		    byte_string);
		return NULL;
	}

	gnode = g_node_new(node);
	return gnode;
}

static GNode*
dfilter_mknode_ipxnet_value(guint32 ipx_net_val)
{
	dfilter_node	*node;
	GNode		*gnode;

	node = g_mem_chunk_alloc(global_df->node_memchunk);
	node->ntype = ipxnet;
	node->elem_size = sizeof(guint8) * 4;
	node->fill_array_func = fill_array_numeric_value; /* cheating ! */
	node->check_relation_func = check_relation_numeric; /* cheating ! */
	node->value.numeric = ipx_net_val;
	gnode = g_node_new(node);

	return gnode;
}

/* Returns NULL on bad parse of IP value */
static GNode*
dfilter_mknode_ipv4_value(char *host)
{
	dfilter_node	*node;
	GNode		*gnode;

	node = g_mem_chunk_alloc(global_df->node_memchunk);
	node->ntype = numeric;
	node->elem_size = sizeof(guint32);
	node->fill_array_func = fill_array_numeric_value; /* cheating ! */
	node->check_relation_func = check_relation_numeric; /* cheating ! */
	if (!get_host_ipaddr(host, &node->value.numeric)) {
		/* Rather than free the mem_chunk allocation, let it
		 * stay. It will be cleaned up when "dfilter_compile()"
		 * calls "dfilter_destroy()". */
		dfilter_fail("\"%s\" isn't a valid host name or IP address.",
		    host);
		return NULL;
	}
	node->value.numeric = htonl(node->value.numeric);

	gnode = g_node_new(node);
	return gnode;
}

/* Returns NULL on bad parse of IPv6 value */
static GNode*
dfilter_mknode_ipv6_value(char *host)
{
	dfilter_node	*node;
	GNode		*gnode;

	node = g_mem_chunk_alloc(global_df->node_memchunk);
	node->ntype = ipv6;
	node->elem_size = 16;
	node->fill_array_func = fill_array_ipv6_value;
	node->check_relation_func = check_relation_ipv6;

	if (!get_host_ipaddr6(host, (struct e_in6_addr*)&node->value.ipv6[0])) {
		/* Rather than free the mem_chunk allocation, let it
		 * stay. It will be cleaned up when "dfilter_compile()"
		 * calls "dfilter_destroy()". */
		dfilter_fail("\"%s\" isn't a valid IPv6 address.",
		    host);
		return NULL;
	}

	gnode = g_node_new(node);
	return gnode;
}

static GNode*
dfilter_mknode_bytes_value(GByteArray *barray)
{
	dfilter_node	*node;
	GNode		*gnode;

	node = g_mem_chunk_alloc(global_df->node_memchunk);
	node->ntype = bytes;
	node->elem_size = sizeof(GByteArray*);
	node->fill_array_func = fill_array_bytes_value;
	node->check_relation_func = check_relation_bytes;
	node->value.bytes = barray;
	node->offset = G_MAXINT;
	node->length = barray->len;
	gnode = g_node_new(node);

	return gnode;
}

/* Given a node representing a bytes_value, returns
 * the length of the byte array */
static guint
dfilter_get_bytes_value_length(GNode* gnode)
{
	dfilter_node	*node = gnode->data;

	g_assert(node->ntype == bytes);
	return node->length;
}

static guint32
string_to_value(char *s)
{
	char	*endptr;
	guint32	val;

	val = strtoul(s, &endptr, 0);
	if (endptr == s || *endptr != '\0') {
		/* This isn't a valid number. */
		dfilter_fail("\"%s\" is not a valid number.", s);
	}
	/* I should probably check errno here */

	return (guint32)val;
}
	
static GNode*
dfilter_mknode_existence(gint id)
{
	dfilter_node	*node;
	GNode		*gnode;

	node = g_mem_chunk_alloc(global_df->node_memchunk);
	node->ntype = existence;
	node->elem_size = sizeof(guint32);
	node->fill_array_func = NULL;
	node->check_relation_func = NULL;
	node->value.variable = id;
	gnode = g_node_new(node);

	return gnode;
}


/* converts a string representing an ether HW address
 * to a guint8 array.
 *
 * Returns 0 on failure, 1 on success.
 */
static int
ether_str_to_guint8_array(const char *s, guint8 *mac)
{
	char	ether_str[18]; /* 2+1+2+1+2+1+2+1+2+1+2 + 1 */
	char	*p, *str;
	int	i = 0;

	if (strlen(s) > 17) {
		return 0;
	}
	strcpy(ether_str, s); /* local copy of string */
	str = ether_str;
	while ((p = strtok(str, "-:."))) {
		/* catch short strings with too many hex bytes: 0.0.0.0.0.0.0 */
		if (i > 5) {
			return 0;
		}
		mac[i] = (guint8) strtoul(p, NULL, 16);
		i++;
		/* subsequent calls to strtok() require NULL as arg 1 */
		str = NULL;
	}
	if (i != 6)
		return 0;	/* failed to read 6 hex pairs */
	else
		return 1;	/* read exactly 6 hex pairs */
}


static int
check_bytes_variable_sanity(GNode *gnode)
{
	int a_off, a_len, reg_len, t_off;

	a_off = dfilter_get_bytes_variable_offset(gnode);
	a_len = dfilter_get_bytes_variable_length(gnode);
	reg_len = dfilter_get_bytes_variable_field_registered_length(gnode);

	if (reg_len > 0) {
		t_off = a_off >= 0 ? a_off : reg_len + a_off;
		if (t_off + a_len > reg_len) {
			dfilter_fail("The \"%s\" field is only %u byte%s wide, but "
				"%u byte%s %s supplied.",
				dfilter_get_variable_abbrev(gnode),
				reg_len, plurality(reg_len, "", "s"),
				a_len, plurality(a_len, "", "s"),
				       plurality(a_len, "was", "were"));
			return 0;
		}
	}
	return 1;
}
