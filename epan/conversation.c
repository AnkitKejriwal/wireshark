/* conversation.c
 * Routines for building lists of packets that are part of a "conversation"
 *
 * $Id: conversation.c,v 1.2 2000/10/21 05:52:28 guy Exp $
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
# include "config.h"
#endif

#include <stdio.h>

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif

#include <string.h>
#include <glib.h>
#include "packet.h"
#include "conversation.h"

static GHashTable *conversation_hashtable = NULL;
static GMemChunk *conversation_key_chunk = NULL;
static GMemChunk *conversation_chunk = NULL;

#ifdef __NOT_USED__ 
typedef struct conversation_key {
	struct conversation_key *next;
	address	src;
	address	dst;
	port_type ptype;
	guint32	port_src;
	guint32	port_dst;
	guint	options;
} conversation_key;
#endif
/*
 * Linked list of conversation keys, so we can, before freeing them all,
 * free the address data allocations associated with them.
 */
static conversation_key *conversation_keys;

static guint32 new_index;

static int conversation_init_count = 200;

/*
 * Compare two conversation keys.
 */
static gint
conversation_equal(gconstpointer v, gconstpointer w)
{
	conversation_key *v1 = (conversation_key *)v;
	conversation_key *v2 = (conversation_key *)w;

	if (v1->ptype != v2->ptype)
		return 0;	/* different types of port */

	/*
	 * Are the first and second source ports the same, the first and
	 * second destination ports the same or not used (NO_DST_PORT),
	 * the first and second source addresses the same, and the first
	 * and second destination addresses the same or not used (NO_DST_ADDR)?
	 */
	if (v1->port_src == v2->port_src &&
            (((v1->options & NO_DST_PORT) && (v2->options & NO_DST_PORT)) ||
		 v1->port_dst == v2->port_dst) &&
	    v1->src.type == v2->src.type &&
	    v1->src.len == v2->src.len &&
	    memcmp(v1->src.data, v2->src.data, v1->src.len) == 0 &&
 	    (((v1->ptype & NO_DST_ADDR) && (v2->ptype & NO_DST_ADDR)) ||
	       (v1->dst.type == v2->dst.type &&
	       v1->dst.type == v2->dst.type &&
	       v1->dst.len == v2->dst.len &&
	       memcmp(v1->dst.data, v2->dst.data, v1->dst.len) == 0))) {
		/*
		 * Yes.  It's the same conversation, and the two
		 * address/port pairs are going in the same direction.
		 */
		return 1;
	}

	/*
	 * Is the first destination port the same as the second source
	 * port, the first source port the same as the second destination
	 * port or not used (NO_DEST_PORT), the first destination address
	 * the same as the second source address, and the first source
	 * address the same as the second destination address or not used
	 * (NO_DEST_ADDR).
	 */
	if (v1->port_dst == v2->port_src &&
	    (((v1->options & NO_DST_PORT) &&(v2->options & NO_DST_PORT)) ||
 	       v1->port_src == v2->port_dst) &&
	    v1->dst.type == v2->src.type &&
	    v1->dst.len == v2->src.len &&
	    memcmp(v1->dst.data, v2->src.data, v1->dst.len) == 0 &&
	    (((v1->options & NO_DST_ADDR) && (v2->options & NO_DST_ADDR)) || 
		(v1->src.type == v2->dst.type &&
	    	v1->src.len == v2->dst.len &&
	    	memcmp(v1->src.data, v2->dst.data, v1->src.len) == 0))) {
		/*
		 * Yes.  It's the same conversation, and the two
		 * address/port pairs are going in opposite directions.
		 */
		return 1;
	}

	/*
	 * The addresses or the ports don't match.
	 */	
	return 0;
}

/*
 * Compute the hash value for a given set of source and destination
 * addresses and ports.
 */
static guint 
conversation_hash(gconstpointer v)
{

	conversation_key *key = (conversation_key *)v;
	guint hash_val;
	int i;

	hash_val = 0;
	for (i = 0; i < key->src.len; i++)
		hash_val += key->src.data[i];

	hash_val += key->port_src;

/* Only hash the destination information if the value is needed. */
	if ( ! ( key->options & NO_DST_ADDR)) 
		for (i = 0; i < key->dst.len; i++)
			hash_val += key->dst.data[i];

	if ( ! (key->options & NO_DST_PORT))
		hash_val += key->port_dst;

	return hash_val;
}

/*
 * Initialize some variables every time a file is loaded or re-loaded.
 * Destroy all existing conversations, and create a new hash table
 * for the conversations in the new file.
 */
void
conversation_init(void)
{
	conversation_key *key;

	/*
	 * Free the addresses associated with the conversation keys.
	 */
	for (key = conversation_keys; key != NULL; key = key->next) {
		/*
		 * Grr.  I guess the theory here is that freeing
		 * something sure as heck modifies it, so you
		 * want to ban attempts to free it, but, alas,
		 * if we make the "data" field of an "address"
		 * structure not a "const", the compiler whines if
		 * we try to make it point into the data for a packet,
		 * as that's a "const" array (and should be, as dissectors
		 * shouldn't trash it).
		 *
		 * So we cast the complaint into oblivion, and rely on
		 * the fact that these addresses are known to have had
		 * their data mallocated, i.e. they don't point into,
		 * say, the middle of the data for a packet.
		 */
		g_free((gpointer)key->src.data);
		g_free((gpointer)key->dst.data);
	}
	conversation_keys = NULL;
	if (conversation_hashtable != NULL)
		g_hash_table_destroy(conversation_hashtable);
	if (conversation_key_chunk != NULL)
		g_mem_chunk_destroy(conversation_key_chunk);
	if (conversation_chunk != NULL)
		g_mem_chunk_destroy(conversation_chunk);

	conversation_hashtable = g_hash_table_new(conversation_hash,
	    conversation_equal);
	conversation_key_chunk = g_mem_chunk_new("conversation_key_chunk",
	    sizeof(conversation_key),
	    conversation_init_count * sizeof(struct conversation_key),
	    G_ALLOC_AND_FREE);
	conversation_chunk = g_mem_chunk_new("conversation_chunk",
	    sizeof(conversation_t),
	    conversation_init_count * sizeof(conversation_t),
	    G_ALLOC_AND_FREE);

	/*
	 * Start the conversation indices over at 0.
	 */
	new_index = 0;
}

/*
 * Copy an address, allocating a new buffer for the address data.
 */
static void
copy_address(address *to, address *from)
{
	guint8 *data;

	to->type = from->type;
	to->len = from->len;
	data = g_malloc(from->len);
	memcpy(data, from->data, from->len);
	to->data = data;
}

/*
 * Given source and destination addresses and ports for a packet,
 * create a new conversation to contain packets between those address/port
 * pairs. The options field is used to flag the destination address/port 
 * are not given and any value is acceptable.

 */
conversation_t *
conversation_new(address *src, address *dst, port_type ptype,
    guint32 src_port, guint32 dst_port, void *data, guint options)
{
	conversation_t *conversation;
	conversation_key *new_key;

	new_key = g_mem_chunk_alloc(conversation_key_chunk);
	new_key->next = conversation_keys;
	conversation_keys = new_key;
	copy_address(&new_key->src, src);
	copy_address(&new_key->dst, dst);
	new_key->ptype = ptype;
	new_key->port_src = src_port;
	new_key->port_dst = dst_port;
	new_key->options = options;

	conversation = g_mem_chunk_alloc(conversation_chunk);
	conversation->index = new_index;
	conversation->data = data;

/* clear dissector pointer */
	conversation->dissector.new_d = NULL;

/* set the key pointer */
	conversation->key_ptr = new_key;

	new_index++;

	g_hash_table_insert(conversation_hashtable, new_key, conversation);
	return conversation;
}

/* Set the destination port in a key.  Remove the original from table,
   update the options and port values, insert the updated key.
*/
void conversation_set_port( conversation_t *conv, guint32 port){

	g_hash_table_remove(conversation_hashtable, conv->key_ptr);
	conv->key_ptr->options &= ~NO_DST_PORT;
	conv->key_ptr->port_dst  = port;
	g_hash_table_insert(conversation_hashtable, conv->key_ptr, conv);
	
} 

/* Set the destination address in a key.  Remove the original from
   table, update the options and port values, insert the updated key.
*/
void conversation_set_addr( conversation_t *conv, address *addr){


	g_hash_table_remove(conversation_hashtable, conv->key_ptr);
	conv->key_ptr->options &= ~NO_DST_ADDR;
	copy_address(&conv->key_ptr->dst, addr);
	g_hash_table_insert(conversation_hashtable, conv->key_ptr, conv);
}
 
/*
 * Given source and destination addresses and ports for a packet,
 * search for a conversation containing packets between those address/port
 * pairs.  Returns NULL if not found.  If the NO_DEST_ADDR and/or NO_DEST_PORT
 * flags are set in the conversation options field, that value will not
 * be used.
 */
conversation_t *
find_conversation(address *src, address *dst, port_type ptype,
    guint32 src_port, guint32 dst_port, uint options)
{
	conversation_key key;

	/*
	 * We don't make a copy of the address data, we just copy the
	 * pointer to it, as "key" disappears when we return.
	 */
	key.src = *src;
	key.dst = *dst;
	key.ptype = ptype;
	key.options = options;
	key.port_src = src_port;
	key.port_dst = dst_port;
	return g_hash_table_lookup(conversation_hashtable, &key);
}

/*
 * Set the dissector for a conversation.
 */
void
old_conversation_set_dissector(conversation_t *conversation,
    old_dissector_t dissector)
{
	conversation->is_old_dissector = TRUE;
	conversation->dissector.old_d = dissector;
}

void
conversation_set_dissector(conversation_t *conversation,
    dissector_t dissector)
{
	conversation->is_old_dissector = FALSE;
	conversation->dissector.new_d = dissector;
}

/*
 * Given source and destination addresses and ports for a packet,
 * search for a conversational dissector.
 * If found, call it and return TRUE, otherwise return FALSE.
 *
 * Will search for a exact match (src & dst), then search for wild
 * card matches: try to match any port on the destination address first,
 * then try to match any address on the port, then try to match any 
 * address and any port. 
 * 
 */

gboolean
old_try_conversation_dissector(address *src, address *dst, port_type ptype,
    guint32 src_port, guint32 dst_port, const u_char *pd, int offset,
    frame_data *fd, proto_tree *tree)
{
	conversation_t *conversation;
	tvbuff_t *tvb;

	conversation = find_conversation(src, dst, ptype, src_port, dst_port, 0);

	if (conversation == NULL)
		conversation = find_conversation(src, dst, ptype, src_port, dst_port, NO_DST_ADDR);

	if (conversation == NULL)
		conversation = find_conversation(src, dst, ptype, src_port, dst_port, NO_DST_PORT);

	if (conversation == NULL)
		conversation = find_conversation(src, dst, ptype, src_port, dst_port,
		    NO_DST_PORT | NO_DST_ADDR);

	if (conversation != NULL) {
		if (conversation->is_old_dissector) {
			if (conversation->dissector.old_d == NULL)
				return FALSE;
			(*conversation->dissector.old_d)(pd, offset, fd, tree);
		} else {
			if (conversation->dissector.new_d == NULL)
				return FALSE;

			/*
			 * Old dissector calling new dissector; use
			 * "tvb_create_from_top()" to remap.
			 *
			 * XXX - what about the "pd" argument?  Do
			 * any dissectors not just pass that along and
			 * let the "offset" argument handle stepping
			 * through the packet?
			 */
			tvb = tvb_create_from_top(offset);
			(*conversation->dissector.new_d)(tvb, &pi, tree);
		}
		return TRUE;
	}
	return FALSE;
}

/*
 * Given source and destination addresses and ports for a packet,
 * search for a conversational dissector.
 * If found, call it and return TRUE, otherwise return FALSE.
 *
 * Will search for a exact match (src & dst), then search for wild
 * card matches: try to match any port on the destination address first,
 * then try to match any address on the port, then try to match any 
 * address and any port. 
*/

gboolean
try_conversation_dissector(address *src, address *dst, port_type ptype,
    guint32 src_port, guint32 dst_port, tvbuff_t *tvb, packet_info *pinfo,
    proto_tree *tree)
{
	conversation_t *conversation;
	const guint8 *pd;
	int offset;

	conversation = find_conversation(src, dst, ptype, src_port, dst_port, 0);

	if (conversation == NULL)
		conversation = find_conversation(src, dst, ptype, src_port, dst_port, NO_DST_ADDR);

	if (conversation == NULL)
		conversation = find_conversation(src, dst, ptype, src_port, dst_port, NO_DST_PORT);

	if (conversation == NULL)
		conversation = find_conversation(src, dst, ptype, src_port, dst_port,
		    NO_DST_PORT | NO_DST_ADDR);

	if (conversation != NULL) {
		if (conversation->is_old_dissector) {
			/*
			 * New dissector calling old dissector; use
			 * "tvb_compat()" to remap.
			 */
			tvb_compat(tvb, &pd, &offset);
			(*conversation->dissector.old_d)(pd, offset, pinfo->fd,
			    tree);
		} else
			(*conversation->dissector.new_d)(tvb, pinfo, tree);
		return TRUE;
	}
	return FALSE;
}
