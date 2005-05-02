/* packet.c
 * Routines for packet disassembly
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

#include <glib.h>

#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif

#include <string.h>
#include <ctype.h>
#include <time.h>

#include "packet.h"
#include "timestamp.h"

#include "atalk-utils.h"
#include "ipv6-utils.h"
#include "sna-utils.h"
#include "osi-utils.h"
#include "to_str.h"

#include "addr_resolv.h"
#include "tvbuff.h"
#include "plugins.h"
#include "epan_dissect.h"

#include <epan/reassemble.h>

static gint proto_malformed = -1;
static dissector_handle_t frame_handle = NULL;
static dissector_handle_t data_handle = NULL;

const true_false_string flags_set_truth = {
  "Set",
  "Not set"
};

void
packet_init(void)
{
  frame_handle = find_dissector("frame");
  data_handle = find_dissector("data");
  proto_malformed = proto_get_id_by_filter_name("malformed");
}

void
packet_cleanup(void)
{
	/* nothing */
}

/*
 * Given a tvbuff, and a length from a packet header, adjust the length
 * of the tvbuff to reflect the specified length.
 */
void
set_actual_length(tvbuff_t *tvb, guint specified_len)
{
  if (specified_len < tvb_reported_length(tvb)) {
    /* Adjust the length of this tvbuff to include only the specified
       payload length.

       The dissector above the one calling us (the dissector above is
       probably us) may use that to determine how much of its packet
       was padding. */
    tvb_set_reported_length(tvb, specified_len);
  }
}

/* Allow protocols to register "init" routines, which are called before
   we make a pass through a capture file and dissect all its packets
   (e.g., when we read in a new capture file, or run a "filter packets"
   or "colorize packets" pass over the current capture file). */
static GSList *init_routines;

void
register_init_routine(void (*func)(void))
{
	init_routines = g_slist_append(init_routines, (gpointer)func);
}

typedef void (*void_func_t)(void);

/* Initialize all data structures used for dissection. */
static void
call_init_routine(gpointer routine, gpointer dummy _U_)
{
	void_func_t func = (void_func_t)routine;
	(*func)();
}

/*
 * XXX - for now, these are the same; the "init" routines free whatever
 * stuff is left over from any previous dissection, and then initialize
 * their tables.
 *
 * We should probably split that into "init" and "cleanup" routines, for
 * cleanliness' sake.
 */
void
init_dissection(void)
{
	/* Initialize the table of conversations. */
	epan_conversation_init();

	/* Initialize the table of circuits. */
	epan_circuit_init();

	/* Initialize protocol-specific variables. */
	g_slist_foreach(init_routines, &call_init_routine, NULL);

	/* Initialize the common data structures for fragment reassembly.
	   Must be done *after* calling init routines, as those routines
	   may free up space for fragments, which they find by using the
	   data structures that "reassemble_init()" frees. */
	reassemble_init();
}

void
cleanup_dissection(void)
{
	init_dissection();
}

/* Allow protocols to register a "cleanup" routine to be
 * run after the initial sequential run through the packets.
 * Note that the file can still be open after this; this is not
 * the final cleanup. */
static GSList *postseq_cleanup_routines;

void
register_postseq_cleanup_routine(void_func_t func)
{
	postseq_cleanup_routines = g_slist_append(postseq_cleanup_routines,
			(gpointer)func);
}

/* Call all the registered "postseq_cleanup" routines. */
static void
call_postseq_cleanup_routine(gpointer routine, gpointer dummy _U_)
{
	void_func_t func = (void_func_t)routine;
	(*func)();
}

void
postseq_cleanup_all_protocols(void)
{
	g_slist_foreach(postseq_cleanup_routines,
			&call_postseq_cleanup_routine, NULL);
}

/* Contains information about data sources. */
static GMemChunk *data_source_chunk = NULL;

/*
 * Add a new data source to the list of data sources for a frame, given
 * the tvbuff for the data source and its name.
 */
void
add_new_data_source(packet_info *pinfo, tvbuff_t *tvb, char *name)
{
	data_source *src;

	if (data_source_chunk == NULL) {
		data_source_chunk = g_mem_chunk_new("data_source_chunk",
		    sizeof (data_source), 10 * sizeof (data_source),
		    G_ALLOC_AND_FREE);
	}
	src = g_mem_chunk_alloc(data_source_chunk);
	src->tvb = tvb;
	/*
	 * XXX - if we require this argument to be a string constant,
	 * we don't need to allocate a buffer for a copy and make a
	 * copy, and wouldn't need to free the buffer, either.
	 */
	src->name = g_strdup_printf("%s (%u bytes)", name, tvb_length(tvb));
	pinfo->data_src = g_slist_append(pinfo->data_src, src);
}

/*
 * Free up a frame's list of data sources.
 */
void
free_data_sources(packet_info *pinfo)
{
	GSList *src_le;
	data_source *src;

	for (src_le = pinfo->data_src; src_le != NULL; src_le = src_le->next) {
	    	src = src_le->data;
	    	g_free(src->name);
	    	g_mem_chunk_free(data_source_chunk, src);
	}
	g_slist_free(pinfo->data_src);
	pinfo->data_src = NULL;
}

/* Allow dissectors to register a "final_registration" routine
 * that is run like the proto_register_XXX() routine, but the end
 * end of the epan_init() function; that is, *after* all other
 * subsystems, like dfilters, have finished initializing. This is
 * useful for dissector registration routines which need to compile
 * display filters. dfilters can't initialize itself until all protocols
 * have registereed themselves. */
static GSList *final_registration_routines;

void
register_final_registration_routine(void (*func)(void))
{
	final_registration_routines = g_slist_append(final_registration_routines,
			(gpointer)func);
}

/* Call all the registered "final_registration" routines. */
static void
call_final_registration_routine(gpointer routine, gpointer dummy _U_)
{
	void_func_t func = (void_func_t)routine;

	(*func)();
}

void
final_registration_all_protocols(void)
{
	g_slist_foreach(final_registration_routines,
			&call_final_registration_routine, NULL);
}


/* Creates the top-most tvbuff and calls dissect_frame() */
void
dissect_packet(epan_dissect_t *edt, union wtap_pseudo_header *pseudo_header,
	       const guchar *pd, frame_data *fd, column_info *cinfo)
{
	if (cinfo != NULL)
		col_init(cinfo);
	edt->pi.current_proto = "<Missing Protocol Name>";
	edt->pi.cinfo = cinfo;
	edt->pi.fd = fd;
	edt->pi.pseudo_header = pseudo_header;
	edt->pi.data_src = NULL;
	edt->pi.dl_src.type = AT_NONE;
	edt->pi.dl_src.len = 0;
	edt->pi.dl_src.data = NULL;
	edt->pi.dl_dst.type = AT_NONE;
	edt->pi.dl_dst.len = 0;
	edt->pi.dl_dst.data = NULL;
	edt->pi.net_src.type = AT_NONE;
	edt->pi.net_src.len = 0;
	edt->pi.net_src.data = NULL;
	edt->pi.net_dst.type = AT_NONE;
	edt->pi.net_dst.len = 0;
	edt->pi.net_dst.data = NULL;
	edt->pi.src.type = AT_NONE;
	edt->pi.src.len = 0;
	edt->pi.src.data = NULL;
	edt->pi.dst.type = AT_NONE;
	edt->pi.dst.len = 0;
	edt->pi.dst.data = NULL;
	edt->pi.ethertype = 0;
	edt->pi.ipproto  = 0;
	edt->pi.ipxptype = 0;
	edt->pi.ctype = CT_NONE;
	edt->pi.circuit_id = 0;
	edt->pi.noreassembly_reason = "";
	edt->pi.fragmented = FALSE;
	edt->pi.in_error_pkt = FALSE;
	edt->pi.ptype = PT_NONE;
	edt->pi.srcport  = 0;
	edt->pi.destport = 0;
	edt->pi.match_port = 0;
	edt->pi.match_string = NULL;
	edt->pi.can_desegment = 0;
	edt->pi.want_pdu_tracking = 0;
	edt->pi.p2p_dir = P2P_DIR_UNKNOWN;
	edt->pi.private_data = NULL;
	edt->pi.oxid = 0;
	edt->pi.rxid = 0;
	edt->pi.r_ctl = 0;
	edt->pi.src_idx = 0;
	edt->pi.dst_idx = 0;
	edt->pi.vsan = 0;
	edt->pi.dcectxid = 0;
	edt->pi.dcetransporttype = -1;
	edt->pi.decrypt_gssapi_tvb = 0;
	edt->pi.gssapi_wrap_tvb = NULL;
	edt->pi.gssapi_encrypted_tvb = NULL;
	edt->pi.gssapi_decrypted_tvb = NULL;
	edt->pi.layer_names = NULL;
	edt->pi.link_number = 0;
	edt->pi.annex_a_used = MTP2_ANNEX_A_USED_UNKNOWN;

	TRY {
		edt->tvb = tvb_new_real_data(pd, fd->cap_len, fd->pkt_len);
		/* Add this tvbuffer into the data_src list */
		add_new_data_source(&edt->pi, edt->tvb, "Frame");

		/* Even though dissect_frame() catches all the exceptions a
		 * sub-dissector can throw, dissect_frame() itself may throw
		 * a ReportedBoundsError in bizarre cases. Thus, we catch the exception
		 * in this function. */
		if(frame_handle != NULL)
		  call_dissector(frame_handle, edt->tvb, &edt->pi, edt->tree);

	}
	CATCH(BoundsError) {
		g_assert_not_reached();
	}
	CATCH(ReportedBoundsError) {
		if(proto_malformed != -1){
			proto_tree_add_protocol_format(edt->tree, proto_malformed, edt->tvb, 0, 0,
			                               "[Malformed Frame: Packet Length]" );
		} else {
			g_assert_not_reached();
		}
	}
	ENDTRY;

	fd->flags.visited = 1;
}

/*********************** code added for sub-dissector lookup *********************/

/*
 * An dissector handle.
 */
struct dissector_handle {
	const char	*name;		/* dissector name */
	gboolean	is_new;		/* TRUE if new-style dissector */
	union {
		dissector_t	old;
		new_dissector_t	new;
	} dissector;
	protocol_t	*protocol;
};

static int
call_dissector_through_handle(dissector_handle_t handle, tvbuff_t *tvb,
    packet_info *pinfo, proto_tree *tree)
{
	const char *saved_proto;
	int ret;

	saved_proto = pinfo->current_proto;

	if (handle->protocol != NULL) {
		pinfo->current_proto =
		    proto_get_protocol_short_name(handle->protocol);
	}

	if (handle->is_new)
		ret = (*handle->dissector.new)(tvb, pinfo, tree);
	else {
		(*handle->dissector.old)(tvb, pinfo, tree);
		ret = tvb_length(tvb);
		if (ret == 0) {
			/*
			 * XXX - a tvbuff can have 0 bytes of data in
			 * it, so we have to make sure we don't return
			 * 0.
			 */
			ret = 1;
		}
	}

	pinfo->current_proto = saved_proto;

	return ret;
}

/*
 * Call a dissector through a handle.
 * If the protocol for that handle isn't enabled, return 0 without
 * calling the dissector.
 * Otherwise, if the handle refers to a new-style dissector, call the
 * dissector and return its return value, otherwise call it and return
 * the length of the tvbuff pointed to by the argument.
 */
static int
call_dissector_work(dissector_handle_t handle, tvbuff_t *tvb,
    packet_info *pinfo_arg, proto_tree *tree)
{
	packet_info *volatile pinfo = pinfo_arg;
	const char *saved_proto;
	guint16 saved_can_desegment;
	volatile int ret;
	gboolean save_writable;
	volatile address save_dl_src;
	volatile address save_dl_dst;
	volatile address save_net_src;
	volatile address save_net_dst;
	volatile address save_src;
	volatile address save_dst;
	volatile gint saved_layer_names_len = 0;

	if (handle->protocol != NULL &&
	    !proto_is_protocol_enabled(handle->protocol)) {
		/*
		 * The protocol isn't enabled.
		 */
		return 0;
	}

	saved_proto = pinfo->current_proto;
	saved_can_desegment = pinfo->can_desegment;

	if (pinfo->layer_names != NULL)
		saved_layer_names_len = pinfo->layer_names->len;

	/*
	 * can_desegment is set to 2 by anyone which offers the
	 * desegmentation api/service.
	 * Then everytime a subdissector is called it is decremented
	 * by one.
	 * Thus only the subdissector immediately on top of whoever
	 * offers this service can use it.
	 * We save the current value of "can_desegment" for the
	 * benefit of TCP proxying dissectors such as SOCKS, so they
	 * can restore it and allow the dissectors they call to use
	 * the desegmentation service.
	 */
	pinfo->saved_can_desegment = saved_can_desegment;
	pinfo->can_desegment = saved_can_desegment-(saved_can_desegment>0);
	if (handle->protocol != NULL) {
		pinfo->current_proto =
		    proto_get_protocol_short_name(handle->protocol);

		/*
		 * Add the protocol name to the layers
		 */
		if (pinfo->layer_names) {
			if (pinfo->layer_names->len > 0)
				g_string_append(pinfo->layer_names, ":");
			g_string_append(pinfo->layer_names, 
			    proto_get_protocol_filter_name(proto_get_id(handle->protocol)));
		}
	}

	if (pinfo->in_error_pkt) {
		/*
		 * This isn't a packet being transported inside
		 * the protocol whose dissector is calling us,
		 * it's a copy of a packet that caused an error
		 * in some protocol included in a packet that
		 * reports the error (e.g., an ICMP Unreachable
		 * packet).
		 */

		/*
		 * Save the current state of the writability of
		 * the columns, and restore them after the
		 * dissector returns, so that the columns
		 * don't reflect the packet that got the error,
		 * they reflect the packet that reported the
		 * error.
		 */
		save_writable = col_get_writable(pinfo->cinfo);
		col_set_writable(pinfo->cinfo, FALSE);
	 	save_dl_src = pinfo->dl_src;
		save_dl_dst = pinfo->dl_dst;
		save_net_src = pinfo->net_src;
		save_net_dst = pinfo->net_dst;
		save_src = pinfo->src;
		save_dst = pinfo->dst;

		/* Dissect the contained packet. */
		TRY {
			ret = call_dissector_through_handle(handle, tvb,
			    pinfo, tree);
		}
		CATCH(BoundsError) {
			/*
			 * Restore the column writability and addresses.
			 */
			col_set_writable(pinfo->cinfo, save_writable);
			pinfo->dl_src = save_dl_src;
			pinfo->dl_dst = save_dl_dst;
			pinfo->net_src = save_net_src;
			pinfo->net_dst = save_net_dst;
			pinfo->src = save_src;
			pinfo->dst = save_dst;

			/*
			 * Restore the current protocol, so any
			 * "Short Frame" indication reflects that
			 * protocol, not the protocol for the
			 * packet that got the error.
			 */
			pinfo->current_proto = saved_proto;

			/*
			 * Restore the desegmentability state.
			 */
			pinfo->can_desegment = saved_can_desegment;

			/*
			 * Rethrow the exception, so this will be
			 * reported as a short frame.
			 */
			RETHROW;
		}
		CATCH(ReportedBoundsError) {
			/*
			 * "ret" wasn't set because an exception was thrown
			 * before "call_dissector_through_handle()" returned.
			 * As it called something, at least one dissector
			 * accepted the packet, and, as an exception was
			 * thrown, not only was all the tvbuff dissected,
			 * a dissector tried dissecting past the end of
			 * the data in some tvbuff, so we'll assume that
			 * the entire tvbuff was dissected.
			 */
			ret = tvb_length(tvb);
		}
		ENDTRY;

		col_set_writable(pinfo->cinfo, save_writable);
		pinfo->dl_src = save_dl_src;
		pinfo->dl_dst = save_dl_dst;
		pinfo->net_src = save_net_src;
		pinfo->net_dst = save_net_dst;
		pinfo->src = save_src;
		pinfo->dst = save_dst;
		pinfo->want_pdu_tracking = 0;
	} else {
		/*
		 * Just call the subdissector.
		 */
		ret = call_dissector_through_handle(handle, tvb, pinfo, tree);
	}

	if (ret == 0) {
		/*
		 * That dissector didn't accept the packet, so
		 * remove its protocol's name from the list
		 * of protocols.
		 */
		if (pinfo->layer_names != NULL) {
			g_string_truncate(pinfo->layer_names,
			    saved_layer_names_len);
		}
	}
	pinfo->current_proto = saved_proto;
	pinfo->can_desegment = saved_can_desegment;
	return ret;
}

/*
 * An entry in the hash table portion of a dissector table.
 */
struct dtbl_entry {
	dissector_handle_t initial;
	dissector_handle_t current;
};

/*
 * A dissector table.
 *
 * "hash_table" is a hash table, indexed by port number, supplying
 * a "struct dtbl_entry"; it records what dissector is assigned to
 * that port number in that table.
 *
 * "dissector_handles" is a list of all dissectors that *could* be
 * used in that table; not all of them are necessarily in the table,
 * as they may be for protocols that don't have a fixed port number.
 *
 * "ui_name" is the name the dissector table has in the user interface.
 *
 * "type" is a field type giving the width of the port number for that
 * dissector table.
 *
 * "base" is the base in which to display the port number for that
 * dissector table.
 */
struct dissector_table {
	GHashTable	*hash_table;
	GSList		*dissector_handles;
	char		*ui_name;
	ftenum_t	type;
	int		base;
};

static GHashTable *dissector_tables = NULL;

/* Finds a dissector table by table name. */
dissector_table_t
find_dissector_table(const char *name)
{
	g_assert(dissector_tables);
	return g_hash_table_lookup( dissector_tables, name );
}

/* Find an entry in a uint dissector table. */
static dtbl_entry_t *
find_uint_dtbl_entry(dissector_table_t sub_dissectors, guint32 pattern)
{
	switch (sub_dissectors->type) {

	case FT_UINT8:
	case FT_UINT16:
	case FT_UINT24:
	case FT_UINT32:
		/*
		 * You can do a port lookup in these tables.
		 */
		break;

	default:
		/*
		 * But you can't do a port lookup in any other types
		 * of tables.
		 */
		g_assert_not_reached();
	}

	/*
	 * Find the entry.
	 */
	return g_hash_table_lookup(sub_dissectors->hash_table,
	    GUINT_TO_POINTER(pattern));
}

/* Add an entry to a uint dissector table. */
void
dissector_add(const char *name, guint32 pattern, dissector_handle_t handle)
{
	dissector_table_t sub_dissectors = find_dissector_table( name);
	dtbl_entry_t *dtbl_entry;

/* sanity checks */
	g_assert( sub_dissectors);
	switch (sub_dissectors->type) {

	case FT_UINT8:
	case FT_UINT16:
	case FT_UINT24:
	case FT_UINT32:
		/*
		 * You can do a port lookup in these tables.
		 */
		break;

	default:
		/*
		 * But you can't do a port lookup in any other types
		 * of tables.
		 */
		g_assert_not_reached();
	}

	dtbl_entry = g_malloc(sizeof (dtbl_entry_t));
	dtbl_entry->current = handle;
	dtbl_entry->initial = dtbl_entry->current;

/* do the table insertion */
    	g_hash_table_insert( sub_dissectors->hash_table,
    	    GUINT_TO_POINTER( pattern), (gpointer)dtbl_entry);

	/*
	 * Now add it to the list of handles that could be used with this
	 * table, because it *is* being used with this table.
	 */
	dissector_add_handle(name, handle);
}

/* Delete the entry for a dissector in a uint dissector table
   with a particular pattern. */

/* NOTE: this doesn't use the dissector call variable. It is included to */
/*	be consistant with the dissector_add and more importantly to be used */
/*	if the technique of adding a temporary dissector is implemented.  */
/*	If temporary dissectors are deleted, then the original dissector must */
/*	be available. */
void
dissector_delete(const char *name, guint32 pattern,
	dissector_handle_t handle _U_)
{
	dissector_table_t sub_dissectors = find_dissector_table( name);
	dtbl_entry_t *dtbl_entry;

/* sanity check */
	g_assert( sub_dissectors);

	/*
	 * Find the entry.
	 */
	dtbl_entry = find_uint_dtbl_entry(sub_dissectors, pattern);

	if (dtbl_entry != NULL) {
		/*
		 * Found - remove it.
		 */
		g_hash_table_remove(sub_dissectors->hash_table,
		    GUINT_TO_POINTER(pattern));

		/*
		 * Now free up the entry.
		 */
		g_free(dtbl_entry);
	}
}

/* Change the entry for a dissector in a uint dissector table
   with a particular pattern to use a new dissector handle. */
void
dissector_change(const char *name, guint32 pattern, dissector_handle_t handle)
{
	dissector_table_t sub_dissectors = find_dissector_table( name);
	dtbl_entry_t *dtbl_entry;

/* sanity check */
	g_assert( sub_dissectors);

	/*
	 * See if the entry already exists. If so, reuse it.
	 */
	dtbl_entry = find_uint_dtbl_entry(sub_dissectors, pattern);
	if (dtbl_entry != NULL) {
	  dtbl_entry->current = handle;
	  return;
	}

	/*
	 * Don't create an entry if there is no dissector handle - I.E. the
	 * user said not to decode something that wasn't being decoded
	 * in the first place.
	 */
	if (handle == NULL)
	  return;

	dtbl_entry = g_malloc(sizeof (dtbl_entry_t));
	dtbl_entry->initial = NULL;
	dtbl_entry->current = handle;

/* do the table insertion */
    	g_hash_table_insert( sub_dissectors->hash_table,
    	    GUINT_TO_POINTER( pattern), (gpointer)dtbl_entry);
}

/* Reset an entry in a uint dissector table to its initial value. */
void
dissector_reset(const char *name, guint32 pattern)
{
	dissector_table_t sub_dissectors = find_dissector_table( name);
	dtbl_entry_t *dtbl_entry;

/* sanity check */
	g_assert( sub_dissectors);

	/*
	 * Find the entry.
	 */
	dtbl_entry = find_uint_dtbl_entry(sub_dissectors, pattern);

	if (dtbl_entry == NULL)
		return;

	/*
	 * Found - is there an initial value?
	 */
	if (dtbl_entry->initial != NULL) {
		dtbl_entry->current = dtbl_entry->initial;
	} else {
		g_hash_table_remove(sub_dissectors->hash_table,
		    GUINT_TO_POINTER(pattern));
		g_free(dtbl_entry);
	}
}

/* Look for a given value in a given uint dissector table and, if found,
   call the dissector with the arguments supplied, and return TRUE,
   otherwise return FALSE. */
gboolean
dissector_try_port(dissector_table_t sub_dissectors, guint32 port,
    tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	dtbl_entry_t *dtbl_entry;
	struct dissector_handle *handle;
	guint32 saved_match_port;
	int ret;

	dtbl_entry = find_uint_dtbl_entry(sub_dissectors, port);
	if (dtbl_entry != NULL) {
		/*
		 * Is there currently a dissector handle for this entry?
		 */
		handle = dtbl_entry->current;
		if (handle == NULL) {
			/*
			 * No - pretend this dissector didn't exist,
			 * so that other dissectors might have a chance
			 * to dissect this packet.
			 */
			return FALSE;
		}

		/*
		 * Save the current value of "pinfo->match_port",
		 * set it to the port that matched, call the
		 * dissector, and restore "pinfo->match_port".
		 */
		saved_match_port = pinfo->match_port;
		pinfo->match_port = port;
		ret = call_dissector_work(handle, tvb, pinfo, tree);
		pinfo->match_port = saved_match_port;

		/*
		 * If a new-style dissector returned 0, it means that
		 * it didn't think this tvbuff represented a packet for
		 * its protocol, and didn't dissect anything.
		 *
		 * Old-style dissectors can't reject the packet.
		 *
		 * 0 is also returned if the protocol wasn't enabled.
		 *
		 * If the packet was rejected, we return FALSE, so that
		 * other dissectors might have a chance to dissect this
		 * packet, otherwise we return TRUE.
		 */
		return ret != 0;
	}
	return FALSE;
}

/* Look for a given value in a given uint dissector table and, if found,
   return the dissector handle for that value. */
dissector_handle_t
dissector_get_port_handle(dissector_table_t sub_dissectors, guint32 port)
{
	dtbl_entry_t *dtbl_entry;

	dtbl_entry = find_uint_dtbl_entry(sub_dissectors, port);
	if (dtbl_entry != NULL)
		return dtbl_entry->current;
	else
		return NULL;
}

/* Find an entry in a string dissector table. */
static dtbl_entry_t *
find_string_dtbl_entry(dissector_table_t sub_dissectors, const gchar *pattern)
{
	switch (sub_dissectors->type) {

	case FT_STRING:
	case FT_STRINGZ:
		/*
		 * You can do a string lookup in these tables.
		 */
		break;

	default:
		/*
		 * But you can't do a string lookup in any other types
		 * of tables.
		 */
		g_assert_not_reached();
	}

	/*
	 * Find the entry.
	 */
	return g_hash_table_lookup(sub_dissectors->hash_table, pattern);
}

/* Add an entry to a string dissector table. */
void
dissector_add_string(const char *name, gchar *pattern,
    dissector_handle_t handle)
{
	dissector_table_t sub_dissectors = find_dissector_table( name);
	dtbl_entry_t *dtbl_entry;

/* sanity check */
	g_assert( sub_dissectors);

	switch (sub_dissectors->type) {

	case FT_STRING:
	case FT_STRINGZ:
		/*
		 * You can do a string lookup in these tables.
		 */
		break;

	default:
		/*
		 * But you can't do a string lookup in any other types
		 * of tables.
		 */
		g_assert_not_reached();
	}

	dtbl_entry = g_malloc(sizeof (dtbl_entry_t));
	dtbl_entry->current = handle;
	dtbl_entry->initial = dtbl_entry->current;

/* do the table insertion */
    	g_hash_table_insert( sub_dissectors->hash_table, pattern,
    	    (gpointer)dtbl_entry);

	/*
	 * Now add it to the list of handles that could be used with this
	 * table, because it *is* being used with this table.
	 */
	dissector_add_handle(name, handle);
}

/* Delete the entry for a dissector in a string dissector table
   with a particular pattern. */

/* NOTE: this doesn't use the dissector call variable. It is included to */
/*	be consistant with the dissector_add_string and more importantly to */
/*      be used if the technique of adding a temporary dissector is */
/*      implemented.  */
/*	If temporary dissectors are deleted, then the original dissector must */
/*	be available. */
void
dissector_delete_string(const char *name, const gchar *pattern,
	dissector_handle_t handle _U_)
{
	dissector_table_t sub_dissectors = find_dissector_table( name);
	dtbl_entry_t *dtbl_entry;

/* sanity check */
	g_assert( sub_dissectors);

	/*
	 * Find the entry.
	 */
	dtbl_entry = find_string_dtbl_entry(sub_dissectors, pattern);

	if (dtbl_entry != NULL) {
		/*
		 * Found - remove it.
		 */
		g_hash_table_remove(sub_dissectors->hash_table, pattern);

		/*
		 * Now free up the entry.
		 */
		g_free(dtbl_entry);
	}
}

/* Change the entry for a dissector in a string dissector table
   with a particular pattern to use a new dissector handle. */
void
dissector_change_string(const char *name, gchar *pattern,
    dissector_handle_t handle)
{
	dissector_table_t sub_dissectors = find_dissector_table( name);
	dtbl_entry_t *dtbl_entry;

/* sanity check */
	g_assert( sub_dissectors);

	/*
	 * See if the entry already exists. If so, reuse it.
	 */
	dtbl_entry = find_string_dtbl_entry(sub_dissectors, pattern);
	if (dtbl_entry != NULL) {
	  dtbl_entry->current = handle;
	  return;
	}

	/*
	 * Don't create an entry if there is no dissector handle - I.E. the
	 * user said not to decode something that wasn't being decoded
	 * in the first place.
	 */
	if (handle == NULL)
	  return;

	dtbl_entry = g_malloc(sizeof (dtbl_entry_t));
	dtbl_entry->initial = NULL;
	dtbl_entry->current = handle;

/* do the table insertion */
    	g_hash_table_insert( sub_dissectors->hash_table, pattern,
    	    (gpointer)dtbl_entry);
}

/* Reset an entry in a string sub-dissector table to its initial value. */
void
dissector_reset_string(const char *name, const gchar *pattern)
{
	dissector_table_t sub_dissectors = find_dissector_table( name);
	dtbl_entry_t *dtbl_entry;

/* sanity check */
	g_assert( sub_dissectors);

	/*
	 * Find the entry.
	 */
	dtbl_entry = find_string_dtbl_entry(sub_dissectors, pattern);

	if (dtbl_entry == NULL)
		return;

	/*
	 * Found - is there an initial value?
	 */
	if (dtbl_entry->initial != NULL) {
		dtbl_entry->current = dtbl_entry->initial;
	} else {
		g_hash_table_remove(sub_dissectors->hash_table, pattern);
		g_free(dtbl_entry);
	}
}

/* Look for a given string in a given dissector table and, if found, call
   the dissector with the arguments supplied, and return TRUE, otherwise
   return FALSE. */
gboolean
dissector_try_string(dissector_table_t sub_dissectors, const gchar *string,
    tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	dtbl_entry_t *dtbl_entry;
	struct dissector_handle *handle;
	int ret;
	const gchar *saved_match_string;

	dtbl_entry = find_string_dtbl_entry(sub_dissectors, string);
	if (dtbl_entry != NULL) {
		/*
		 * Is there currently a dissector handle for this entry?
		 */
		handle = dtbl_entry->current;
		if (handle == NULL) {
			/*
			 * No - pretend this dissector didn't exist,
			 * so that other dissectors might have a chance
			 * to dissect this packet.
			 */
			return FALSE;
		}

		/*
		 * Save the current value of "pinfo->match_string",
		 * set it to the string that matched, call the
		 * dissector, and restore "pinfo->match_string".
		 */
		saved_match_string = pinfo->match_string;
		pinfo->match_string = string;
		ret = call_dissector_work(handle, tvb, pinfo, tree);
		pinfo->match_string = saved_match_string;

		/*
		 * If a new-style dissector returned 0, it means that
		 * it didn't think this tvbuff represented a packet for
		 * its protocol, and didn't dissect anything.
		 *
		 * Old-style dissectors can't reject the packet.
		 *
		 * 0 is also returned if the protocol wasn't enabled.
		 *
		 * If the packet was rejected, we return FALSE, so that
		 * other dissectors might have a chance to dissect this
		 * packet, otherwise we return TRUE.
		 */
		return ret != 0;
	}
	return FALSE;
}

/* Look for a given value in a given string dissector table and, if found,
   return the dissector handle for that value. */
dissector_handle_t
dissector_get_string_handle(dissector_table_t sub_dissectors,
    const gchar *string)
{
	dtbl_entry_t *dtbl_entry;

	dtbl_entry = find_string_dtbl_entry(sub_dissectors, string);
	if (dtbl_entry != NULL)
		return dtbl_entry->current;
	else
		return NULL;
}

dissector_handle_t
dtbl_entry_get_handle (dtbl_entry_t *dtbl_entry)
{
	return dtbl_entry->current;
}

/* Add a handle to the list of handles that *could* be used with this
   table.  That list is used by code in the UI. */
void
dissector_add_handle(const char *name, dissector_handle_t handle)
{
	dissector_table_t sub_dissectors = find_dissector_table( name);
	GSList *entry;

	/* sanity check */
	g_assert(sub_dissectors != NULL);

	/* Is it already in this list? */
	entry = g_slist_find(sub_dissectors->dissector_handles, (gpointer)handle);
	if (entry != NULL) {
		/*
		 * Yes - don't insert it again.
		 */
		return;
	}

	/* Add it to the list. */
	sub_dissectors->dissector_handles =
	    g_slist_append(sub_dissectors->dissector_handles, (gpointer)handle);
}

dissector_handle_t
dtbl_entry_get_initial_handle (dtbl_entry_t *dtbl_entry)
{
	return dtbl_entry->initial;
}

/**************************************************/
/*                                                */
/*       Routines to walk dissector tables        */
/*                                                */
/**************************************************/

typedef struct dissector_foreach_info {
  gpointer     caller_data;
  DATFunc      caller_func;
  GHFunc       next_func;
  gchar       *table_name;
  ftenum_t     selector_type;
} dissector_foreach_info_t;

/*
 * Called for each entry in a dissector table.
 */
static void
dissector_table_foreach_func (gpointer key, gpointer value, gpointer user_data)
{
	dissector_foreach_info_t *info;
	dtbl_entry_t *dtbl_entry;

	g_assert(value);
	g_assert(user_data);

	dtbl_entry = value;
	if (dtbl_entry->current == NULL ||
	    dtbl_entry->current->protocol == NULL) {
		/*
		 * Either there is no dissector for this entry, or
		 * the dissector doesn't have a protocol associated
		 * with it.
		 *
		 * XXX - should the latter check be done?
		 */
		return;
	}

	info = user_data;
	info->caller_func(info->table_name, info->selector_type, key, value,
	    info->caller_data);
}

/*
 * Called for each entry in the table of all dissector tables.
 */
static void
dissector_all_tables_foreach_func (gpointer key, gpointer value, gpointer user_data)
{
	dissector_table_t sub_dissectors;
	dissector_foreach_info_t *info;

	g_assert(value);
	g_assert(user_data);

	sub_dissectors = value;
	info = user_data;
	info->table_name = (gchar*) key;
	info->selector_type = get_dissector_table_selector_type(info->table_name);
	g_hash_table_foreach(sub_dissectors->hash_table, info->next_func, info);
}

/*
 * Walk all dissector tables calling a user supplied function on each
 * entry.
 */
void
dissector_all_tables_foreach (DATFunc func,
			      gpointer user_data)
{
	dissector_foreach_info_t info;

	info.caller_data = user_data;
	info.caller_func = func;
	info.next_func = dissector_table_foreach_func;
	g_hash_table_foreach(dissector_tables, dissector_all_tables_foreach_func, &info);
}

/*
 * Walk one dissector table's hash table calling a user supplied function
 * on each entry.
 */
void
dissector_table_foreach (char *name,
			 DATFunc func,
			 gpointer user_data)
{
	dissector_foreach_info_t info;
	dissector_table_t sub_dissectors = find_dissector_table( name);

	info.table_name = name;
	info.selector_type = sub_dissectors->type;
	info.caller_func = func;
	info.caller_data = user_data;
	g_hash_table_foreach(sub_dissectors->hash_table, dissector_table_foreach_func, &info);
}

/*
 * Walk one dissector table's list of handles calling a user supplied
 * function on each entry.
 */
void
dissector_table_foreach_handle(char *name,
			       DATFunc_handle func,
			       gpointer user_data)
{
	dissector_table_t sub_dissectors = find_dissector_table( name);
	GSList *tmp;

	for (tmp = sub_dissectors->dissector_handles; tmp != NULL;
	    tmp = g_slist_next(tmp))
		func(name, tmp->data, user_data);
}

/*
 * Called for each entry in a dissector table.
 */
static void
dissector_table_foreach_changed_func (gpointer key, gpointer value, gpointer user_data)
{
	dtbl_entry_t *dtbl_entry;
	dissector_foreach_info_t *info;

	g_assert(value);
	g_assert(user_data);

	dtbl_entry = value;
	if (dtbl_entry->initial == dtbl_entry->current) {
		/*
		 * Entry hasn't changed - don't call the function.
		 */
		return;
	}

	info = user_data;
	info->caller_func(info->table_name, info->selector_type, key, value,
	    info->caller_data);
}

/*
 * Walk all dissector tables calling a user supplied function only on
 * any entry that has been changed from its original state.
 */
void
dissector_all_tables_foreach_changed (DATFunc func,
				      gpointer user_data)
{
	dissector_foreach_info_t info;

	info.caller_data = user_data;
	info.caller_func = func;
	info.next_func = dissector_table_foreach_changed_func;
	g_hash_table_foreach(dissector_tables, dissector_all_tables_foreach_func, &info);
}

/*
 * Walk one dissector table calling a user supplied function only on
 * any entry that has been changed from its original state.
 */
void
dissector_table_foreach_changed (char *name,
				 DATFunc func,
				 gpointer user_data)
{
	dissector_foreach_info_t info;
	dissector_table_t sub_dissectors = find_dissector_table( name);

	info.table_name = name;
	info.selector_type = sub_dissectors->type;
	info.caller_func = func;
	info.caller_data = user_data;
	g_hash_table_foreach(sub_dissectors->hash_table,
	    dissector_table_foreach_changed_func, &info);
}

typedef struct dissector_foreach_table_info {
  gpointer      caller_data;
  DATFunc_table caller_func;
} dissector_foreach_table_info_t;

/*
 * Called for each entry in the table of all dissector tables.
 */
static void
dissector_all_tables_foreach_table_func (gpointer key, gpointer value, gpointer user_data)
{
	dissector_table_t table;
	dissector_foreach_table_info_t *info;

	table = value;
	info = user_data;
	(*info->caller_func)((gchar*)key, table->ui_name, info->caller_data);
}

/*
 * Walk all dissector tables calling a user supplied function on each
 * table.
 */
void
dissector_all_tables_foreach_table (DATFunc_table func,
				    gpointer user_data)
{
	dissector_foreach_table_info_t info;

	info.caller_data = user_data;
	info.caller_func = func;
	g_hash_table_foreach(dissector_tables, dissector_all_tables_foreach_table_func, &info);
}

dissector_table_t
register_dissector_table(const char *name, char *ui_name, ftenum_t type,
    int base)
{
	dissector_table_t	sub_dissectors;

	/* Create our hash-of-hashes if it doesn't already exist */
	if (!dissector_tables) {
		dissector_tables = g_hash_table_new( g_str_hash, g_str_equal );
		g_assert(dissector_tables);
	}

	/* Make sure the registration is unique */
	g_assert(!g_hash_table_lookup( dissector_tables, name ));

	/* Create and register the dissector table for this name; returns */
	/* a pointer to the dissector table. */
	sub_dissectors = g_malloc(sizeof (struct dissector_table));
	switch (type) {

	case FT_UINT8:
	case FT_UINT16:
	case FT_UINT24:
	case FT_UINT32:
		/*
		 * XXX - there's no "g_uint_hash()" or "g_uint_equal()",
		 * so we use "g_direct_hash()" and "g_direct_equal()".
		 */
		sub_dissectors->hash_table = g_hash_table_new( g_direct_hash,
		    g_direct_equal );
		break;

	case FT_STRING:
	case FT_STRINGZ:
		sub_dissectors->hash_table = g_hash_table_new( g_str_hash,
		    g_str_equal );
		break;

	default:
		g_assert_not_reached();
	}
	sub_dissectors->dissector_handles = NULL;
	sub_dissectors->ui_name = ui_name;
	sub_dissectors->type = type;
	sub_dissectors->base = base;
	g_hash_table_insert( dissector_tables, (gpointer)name, (gpointer) sub_dissectors );
	return sub_dissectors;
}

char *
get_dissector_table_ui_name(const char *name)
{
	dissector_table_t sub_dissectors = find_dissector_table( name);

	return sub_dissectors->ui_name;
}

ftenum_t
get_dissector_table_selector_type(const char *name)
{
	dissector_table_t sub_dissectors = find_dissector_table( name);

	return sub_dissectors->type;
}

int
get_dissector_table_base(const char *name)
{
	dissector_table_t sub_dissectors = find_dissector_table( name);

	return sub_dissectors->base;
}

static GHashTable *heur_dissector_lists = NULL;

typedef struct {
	heur_dissector_t dissector;
	protocol_t *protocol;
} heur_dtbl_entry_t;

/* Finds a heuristic dissector table by field name. */
static heur_dissector_list_t *
find_heur_dissector_list(const char *name)
{
	g_assert(heur_dissector_lists != NULL);
	return g_hash_table_lookup(heur_dissector_lists, name);
}

void
heur_dissector_add(const char *name, heur_dissector_t dissector, int proto)
{
	heur_dissector_list_t *sub_dissectors = find_heur_dissector_list(name);
	heur_dtbl_entry_t *dtbl_entry;

	/* sanity check */
	g_assert(sub_dissectors != NULL);

	dtbl_entry = g_malloc(sizeof (heur_dtbl_entry_t));
	dtbl_entry->dissector = dissector;
	dtbl_entry->protocol = find_protocol_by_id(proto);

	/* do the table insertion */
	*sub_dissectors = g_slist_append(*sub_dissectors, (gpointer)dtbl_entry);
}

gboolean
dissector_try_heuristic(heur_dissector_list_t sub_dissectors,
    tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	gboolean status;
	const char *saved_proto;
	GSList *entry;
	heur_dtbl_entry_t *dtbl_entry;
	guint16 saved_can_desegment;
	gint saved_layer_names_len = 0;

	/* can_desegment is set to 2 by anyone which offers this api/service.
	   then everytime a subdissector is called it is decremented by one.
	   thus only the subdissector immediately ontop of whoever offers this
	   service can use it.
	   We save the current value of "can_desegment" for the
	   benefit of TCP proxying dissectors such as SOCKS, so they
	   can restore it and allow the dissectors they call to use
	   the desegmentation service.
	*/
	saved_can_desegment=pinfo->can_desegment;
	pinfo->saved_can_desegment = saved_can_desegment;
	pinfo->can_desegment = saved_can_desegment-(saved_can_desegment>0);

	status = FALSE;
	saved_proto = pinfo->current_proto;

	if (pinfo->layer_names != NULL)
		saved_layer_names_len = pinfo->layer_names->len;

	for (entry = sub_dissectors; entry != NULL; entry = g_slist_next(entry)) {
		/* XXX - why set this now and above? */
		pinfo->can_desegment = saved_can_desegment-(saved_can_desegment>0);
		dtbl_entry = (heur_dtbl_entry_t *)entry->data;

		if (dtbl_entry->protocol != NULL &&
		    !proto_is_protocol_enabled(dtbl_entry->protocol)) {
			/*
			 * No - don't try this dissector.
			 */
			continue;
		}

		if (dtbl_entry->protocol != NULL) {
			pinfo->current_proto =
			    proto_get_protocol_short_name(dtbl_entry->protocol);
		}

		/*
		 * Add the protocol name to the layers; we'll remove it
		 * if the dissector fails.
		 */
		if (pinfo->layer_names) {
			if (pinfo->layer_names->len > 0)
				g_string_append(pinfo->layer_names, ":");
			g_string_append(pinfo->layer_names,
			    proto_get_protocol_filter_name(proto_get_id(dtbl_entry->protocol)));
		}

		if ((*dtbl_entry->dissector)(tvb, pinfo, tree)) {
			status = TRUE;
			break;
		} else {
			/*
			 * That dissector didn't accept the packet, so
			 * remove its protocol's name from the list
			 * of protocols.
			 */
			if (pinfo->layer_names != NULL) {
				g_string_truncate(pinfo->layer_names,
				    saved_layer_names_len);
			}
		}
	}
	pinfo->current_proto = saved_proto;
	pinfo->can_desegment=saved_can_desegment;
	return status;
}

void
register_heur_dissector_list(const char *name, heur_dissector_list_t *sub_dissectors)
{
	/* Create our hash-of-lists if it doesn't already exist */
	if (heur_dissector_lists == NULL) {
		heur_dissector_lists = g_hash_table_new(g_str_hash, g_str_equal);
		g_assert(heur_dissector_lists != NULL);
	}

	/* Make sure the registration is unique */
	g_assert(g_hash_table_lookup(heur_dissector_lists, name) == NULL);

	*sub_dissectors = NULL;	/* initially empty */
	g_hash_table_insert(heur_dissector_lists, (gpointer)name,
	    (gpointer) sub_dissectors);
}

/*
 * Register dissectors by name; used if one dissector always calls a
 * particular dissector, or if it bases the decision of which dissector
 * to call on something other than a numerical value or on "try a bunch
 * of dissectors until one likes the packet".
 */

/*
 * List of registered dissectors.
 */
static GHashTable *registered_dissectors = NULL;

/* Get the short name of the protocol for a dissector handle, if it has
   a protocol. */
char *
dissector_handle_get_short_name(dissector_handle_t handle)
{
	if (handle->protocol == NULL) {
		/*
		 * No protocol (see, for example, the handle for
		 * dissecting the set of protocols where the first
		 * octet of the payload is an OSI network layer protocol
		 * ID).
		 */
		return NULL;
	}
	return proto_get_protocol_short_name(handle->protocol);
}

/* Get the index of the protocol for a dissector handle, if it has
   a protocol. */
int
dissector_handle_get_protocol_index(dissector_handle_t handle)
{
	if (handle->protocol == NULL) {
		/*
		 * No protocol (see, for example, the handle for
		 * dissecting the set of protocols where the first
		 * octet of the payload is an OSI network layer protocol
		 * ID).
		 */
		return -1;
	}
	return proto_get_id(handle->protocol);
}

/* Find a registered dissector by name. */
dissector_handle_t
find_dissector(const char *name)
{
	g_assert(registered_dissectors != NULL);
	return g_hash_table_lookup(registered_dissectors, name);
}

/* Create an anonymous handle for a dissector. */
dissector_handle_t
create_dissector_handle(dissector_t dissector, int proto)
{
	struct dissector_handle *handle;

	handle = g_malloc(sizeof (struct dissector_handle));
	handle->name = NULL;
	handle->is_new = FALSE;
	handle->dissector.old = dissector;
	handle->protocol = find_protocol_by_id(proto);

	return handle;
}

dissector_handle_t
new_create_dissector_handle(new_dissector_t dissector, int proto)
{
	struct dissector_handle *handle;

	handle = g_malloc(sizeof (struct dissector_handle));
	handle->name = NULL;
	handle->is_new = TRUE;
	handle->dissector.new = dissector;
	handle->protocol = find_protocol_by_id(proto);

	return handle;
}

/* Register a dissector by name. */
void
register_dissector(const char *name, dissector_t dissector, int proto)
{
	struct dissector_handle *handle;

	/* Create our hash table if it doesn't already exist */
	if (registered_dissectors == NULL) {
		registered_dissectors = g_hash_table_new(g_str_hash, g_str_equal);
		g_assert(registered_dissectors != NULL);
	}

	/* Make sure the registration is unique */
	g_assert(g_hash_table_lookup(registered_dissectors, name) == NULL);

	handle = g_malloc(sizeof (struct dissector_handle));
	handle->name = name;
	handle->is_new = FALSE;
	handle->dissector.old = dissector;
	handle->protocol = find_protocol_by_id(proto);

	g_hash_table_insert(registered_dissectors, (gpointer)name,
	    (gpointer) handle);
}

void
new_register_dissector(const char *name, new_dissector_t dissector, int proto)
{
	struct dissector_handle *handle;

	/* Create our hash table if it doesn't already exist */
	if (registered_dissectors == NULL) {
		registered_dissectors = g_hash_table_new(g_str_hash, g_str_equal);
		g_assert(registered_dissectors != NULL);
	}

	/* Make sure the registration is unique */
	g_assert(g_hash_table_lookup(registered_dissectors, name) == NULL);

	handle = g_malloc(sizeof (struct dissector_handle));
	handle->name = name;
	handle->is_new = TRUE;
	handle->dissector.new = dissector;
	handle->protocol = find_protocol_by_id(proto);

	g_hash_table_insert(registered_dissectors, (gpointer)name,
	    (gpointer) handle);
}

/* Call a dissector through a handle. */
int
call_dissector(dissector_handle_t handle, tvbuff_t *tvb,
    packet_info *pinfo, proto_tree *tree)
{
	int ret;

	ret = call_dissector_work(handle, tvb, pinfo, tree);
	if (ret == 0) {
		/*
		 * The protocol was disabled, or the dissector rejected
		 * it.  Just dissect this packet as data.
		 */
	        g_assert(data_handle != NULL);
		g_assert(data_handle->protocol != NULL);
		call_dissector(data_handle, tvb, pinfo, tree);
		return tvb_length(tvb);
	}
	return ret;
}

/*
 * Dumps the "layer type"/"decode as" associations to stdout, similar
 * to the proto_registrar_dump_*() routines.
 *
 * There is one record per line. The fields are tab-delimited.
 *
 * Field 1 = layer type, e.g. "tcp.port"
 * Field 2 = selector in decimal
 * Field 3 = "decode as" name, e.g. "http"
 */


static void
dissector_dump_decodes_display(gchar *table_name, ftenum_t selector_type _U_,
    gpointer key, gpointer value, gpointer user_data _U_)
{
	guint32 selector = (guint32) key;
	dissector_table_t sub_dissectors = find_dissector_table(table_name);
	dtbl_entry_t *dtbl_entry;
	dissector_handle_t handle;
	gint proto_id;
	gchar *decode_as;

	g_assert(sub_dissectors);
	switch (sub_dissectors->type) {

		case FT_UINT8:
		case FT_UINT16:
		case FT_UINT24:
		case FT_UINT32:
			dtbl_entry = value;
			g_assert(dtbl_entry);

			handle = dtbl_entry->current;
			g_assert(handle);

			proto_id = dissector_handle_get_protocol_index(handle);

			if (proto_id != -1) {
				decode_as = proto_get_protocol_filter_name(proto_id);
				g_assert(decode_as != NULL);
				printf("%s\t%d\t%s\n", table_name, selector, decode_as);
			}
			break;

	default:
		break;
	}
}

void
dissector_dump_decodes() {
	dissector_all_tables_foreach(dissector_dump_decodes_display, NULL);
}
