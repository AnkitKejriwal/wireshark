/* packet-tcp.c
 * Routines for TCP packet disassembly
 *
 * $Id: packet-tcp.c,v 1.183 2003/03/03 03:16:36 sharpe Exp $
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
#include "in_cksum.h"

#include <epan/resolv.h>
#include "ipproto.h"
#include "follow.h"
#include "prefs.h"
#include "packet-tcp.h"
#include "packet-ip.h"
#include "packet-frame.h"
#include <epan/conversation.h>
#include <epan/strutil.h>
#include "reassemble.h"
#include "tap.h"

static int tcp_tap = -1;

/* Place TCP summary in proto tree */
static gboolean tcp_summary_in_tree = TRUE;

/*
 * Flag to control whether to check the TCP checksum.
 *
 * In at least some Solaris network traces, there are packets with bad
 * TCP checksums, but the traffic appears to indicate that the packets
 * *were* received; the packets were probably sent by the host on which
 * the capture was being done, on a network interface to which
 * checksumming was offloaded, so that DLPI supplied an un-checksummed
 * packet to the capture program but a checksummed packet got put onto
 * the wire.
 */
static gboolean tcp_check_checksum = TRUE;

extern FILE* data_out_file;

static int proto_tcp = -1;
static int hf_tcp_srcport = -1;
static int hf_tcp_dstport = -1;
static int hf_tcp_port = -1;
static int hf_tcp_seq = -1;
static int hf_tcp_nxtseq = -1;
static int hf_tcp_ack = -1;
static int hf_tcp_hdr_len = -1;
static int hf_tcp_flags = -1;
static int hf_tcp_flags_cwr = -1;
static int hf_tcp_flags_ecn = -1;
static int hf_tcp_flags_urg = -1;
static int hf_tcp_flags_ack = -1;
static int hf_tcp_flags_push = -1;
static int hf_tcp_flags_reset = -1;
static int hf_tcp_flags_syn = -1;
static int hf_tcp_flags_fin = -1;
static int hf_tcp_window_size = -1;
static int hf_tcp_checksum = -1;
static int hf_tcp_checksum_bad = -1;
static int hf_tcp_len = -1;
static int hf_tcp_urgent_pointer = -1;
static int hf_tcp_analysis_flags = -1;
static int hf_tcp_analysis_acks_frame = -1;
static int hf_tcp_analysis_ack_rtt = -1;
static int hf_tcp_analysis_retransmission = -1;
static int hf_tcp_analysis_lost_packet = -1;
static int hf_tcp_analysis_ack_lost_packet = -1;
static int hf_tcp_analysis_keep_alive = -1;
static int hf_tcp_analysis_duplicate_ack = -1;
static int hf_tcp_analysis_zero_window = -1;
static int hf_tcp_analysis_zero_window_probe = -1;
static int hf_tcp_analysis_zero_window_violation = -1;
static int hf_tcp_segments = -1;
static int hf_tcp_segment = -1;
static int hf_tcp_segment_overlap = -1;
static int hf_tcp_segment_overlap_conflict = -1;
static int hf_tcp_segment_multiple_tails = -1;
static int hf_tcp_segment_too_long_fragment = -1;
static int hf_tcp_segment_error = -1;
static int hf_tcp_option_mss = -1;
static int hf_tcp_option_mss_val = -1;
static int hf_tcp_option_wscale = -1;
static int hf_tcp_option_wscale_val = -1;
static int hf_tcp_option_sack_perm = -1;
static int hf_tcp_option_sack = -1;
static int hf_tcp_option_sack_sle = -1;
static int hf_tcp_option_sack_sre = -1;
static int hf_tcp_option_echo = -1;
static int hf_tcp_option_echo_reply = -1;
static int hf_tcp_option_time_stamp = -1;
static int hf_tcp_option_cc = -1;
static int hf_tcp_option_ccnew = -1;
static int hf_tcp_option_ccecho = -1;
static int hf_tcp_option_md5 = -1;

static gint ett_tcp = -1;
static gint ett_tcp_flags = -1;
static gint ett_tcp_options = -1;
static gint ett_tcp_option_sack = -1;
static gint ett_tcp_analysis = -1;
static gint ett_tcp_analysis_faults = -1;
static gint ett_tcp_segments = -1;
static gint ett_tcp_segment  = -1;


/* not all of the hf_fields below make sense for TCP but we have to provide 
   them anyways to comply with the api (which was aimed for ip fragment 
   reassembly) */
static const fragment_items tcp_segment_items = {
	&ett_tcp_segment,
	&ett_tcp_segments,
	&hf_tcp_segments,
	&hf_tcp_segment,
	&hf_tcp_segment_overlap,
	&hf_tcp_segment_overlap_conflict,
	&hf_tcp_segment_multiple_tails,
	&hf_tcp_segment_too_long_fragment,
	&hf_tcp_segment_error,
	"Segments"
};

static dissector_table_t subdissector_table;
static heur_dissector_list_t heur_subdissector_list;
static dissector_handle_t data_handle;

/* TCP structs and definitions */


/* **************************************************************************
 * stuff to analyze TCP sequencenumbers for retransmissions, missing segments,
 * RTT and reltive sequence numbers.
 * **************************************************************************/
static gboolean tcp_analyze_seq = FALSE;
static gboolean tcp_relative_seq = FALSE;

static GMemChunk *tcp_unacked_chunk = NULL;
static int tcp_unacked_count = 500;	/* one for each packet until it is acked*/
struct tcp_unacked {
	struct tcp_unacked *next;
	guint32 frame;
	guint32	seq;
	guint32 nextseq;
	nstime_t ts;

	/* these are used for detection of duplicate acks and nothing else */
	guint32 ack_frame;
	guint32 ack;
	guint32 num_acks;

	/* this is to keep track of zero window and zero window probe */
	guint32 window;
};

/* Idea for gt: either x > y, or y is much bigger (assume wrap) */
#define GT_SEQ(x, y) ((gint32)((y) - (x)) < 0)
#define LT_SEQ(x, y) ((gint32)((x) - (y)) < 0)
#define GE_SEQ(x, y) ((gint32)((y) - (x)) <= 0)
#define LE_SEQ(x, y) ((gint32)((x) - (y)) <= 0)
#define EQ_SEQ(x, y) ((x) == (y))

static GMemChunk *tcp_acked_chunk = NULL;
static int tcp_acked_count = 5000;	/* one for almost every other segment in the capture */
#define TCP_A_RETRANSMISSION		0x01
#define TCP_A_LOST_PACKET		0x02
#define TCP_A_ACK_LOST_PACKET		0x04
#define TCP_A_KEEP_ALIVE		0x08
#define TCP_A_DUPLICATE_ACK		0x10
#define TCP_A_ZERO_WINDOW		0x20
#define TCP_A_ZERO_WINDOW_PROBE		0x40
#define TCP_A_ZERO_WINDOW_VIOLATION	0x80
struct tcp_acked {
	guint32 frame_acked;
	nstime_t ts;
	guint8 flags;
};
static GHashTable *tcp_analyze_acked_table = NULL;

static GMemChunk *tcp_rel_seq_chunk = NULL;
static int tcp_rel_seq_count = 10000; /* one for each segment in the capture */
struct tcp_rel_seq {
	guint32 seq_base;
	guint32 ack_base;
};
static GHashTable *tcp_rel_seq_table = NULL;

static GMemChunk *tcp_analysis_chunk = NULL;
static int tcp_analysis_count = 20;	/* one for each conversation */
struct tcp_analysis {
	/* These two structs are managed based on comparing the source
	 * and destination addresses and, if they're equal, comparing
	 * the source and destination ports.
	 *
	 * If the source is greater than the destination, then stuff
	 * sent from src is in ual1.
	 *
	 * If the source is less than the destination, then stuff
	 * sent from src is in ual2.
	 *
	 * XXX - if the addresses and ports are equal, we don't guarantee
	 * the behavior.
	 */
	struct tcp_unacked *ual1;	/* UnAcked List 1*/
	guint32 base_seq1;
	struct tcp_unacked *ual2;	/* UnAcked List 2*/
	guint32 base_seq2;
};

static void
tcp_get_relative_seq_ack(guint32 frame, guint32 *seq, guint32 *ack)
{
	struct tcp_rel_seq *trs;

	trs=g_hash_table_lookup(tcp_rel_seq_table, (void *)frame);
	if(!trs){
		return;
	}

	(*seq) -= trs->seq_base;
	(*ack) -= trs->ack_base;
}

static struct tcp_acked *
tcp_analyze_get_acked_struct(guint32 frame, gboolean createflag)
{
	struct tcp_acked *ta;

	ta=g_hash_table_lookup(tcp_analyze_acked_table, (void *)frame);
	if((!ta) && createflag){
		ta=g_mem_chunk_alloc(tcp_acked_chunk);
		ta->frame_acked=0;
		ta->ts.secs=0;
		ta->ts.nsecs=0;
		ta->flags=0;
		g_hash_table_insert(tcp_analyze_acked_table, (void *)frame, ta);
	}
	return ta;
}

static void
tcp_analyze_sequence_number(packet_info *pinfo, guint32 seq, guint32 ack, guint32 seglen, guint8 flags, guint16 window)
{
	conversation_t *conv=NULL;
	struct tcp_analysis *tcpd=NULL;
	int direction;
	struct tcp_unacked *ual1=NULL;
	struct tcp_unacked *ual2=NULL;
	struct tcp_unacked *ual=NULL;
	guint32 base_seq;
	guint32 base_ack;

	/* Have we seen this conversation before? */
	if( (conv=find_conversation(&pinfo->src, &pinfo->dst, pinfo->ptype, pinfo->srcport, pinfo->destport, 0)) == NULL){
		/* No this is a new conversation. */
		conv=conversation_new(&pinfo->src, &pinfo->dst, pinfo->ptype, pinfo->srcport, pinfo->destport, 0);
	}

	/* check if we have any data for this conversation */
	tcpd=conversation_get_proto_data(conv, proto_tcp);
	if(!tcpd){
		/* No no such data yet. Allocate and init it */
		tcpd=g_mem_chunk_alloc(tcp_analysis_chunk);
		tcpd->ual1=NULL;
		tcpd->base_seq1=0;
		tcpd->ual2=NULL;
		tcpd->base_seq2=0;
		conversation_add_proto_data(conv, proto_tcp, tcpd);
	}

	/* check direction and get ua lists */
	direction=CMP_ADDRESS(&pinfo->src, &pinfo->dst);
	/* if the addresses are equal, match the ports instead */
	if(direction==0) {
		direction= (pinfo->srcport > pinfo->destport)*2-1;
	}
	if(direction>=0){
		ual1=tcpd->ual1;
		ual2=tcpd->ual2;
		base_seq=tcpd->base_seq1;
		base_ack=tcpd->base_seq2;
	} else {
		ual1=tcpd->ual2;
		ual2=tcpd->ual1;
		base_seq=tcpd->base_seq2;
		base_ack=tcpd->base_seq1;
	}

	if(base_seq==0){
		base_seq=seq;
	}
	if(base_ack==0){
		base_ack=ack;
	}

	/* handle the sequence numbers */
	/* if this was a SYN packet, then remove existing list and
	 * put SEQ+1 first the list */
	if(flags&TH_SYN){
		for(ual=ual1;ual1;ual1=ual){
			ual=ual1->next;
			g_mem_chunk_free(tcp_unacked_chunk, ual1);
		}
		ual1=g_mem_chunk_alloc(tcp_unacked_chunk);
		ual1->next=NULL;
		ual1->frame=pinfo->fd->num;
		ual1->ack_frame=0;
		ual1->ack=0;
		ual1->num_acks=0;
		ual1->seq=seq+1;
		ual1->nextseq=seq+1;
		ual1->ts.secs=pinfo->fd->abs_secs;
		ual1->ts.nsecs=pinfo->fd->abs_usecs*1000;
		ual1->window=window;
		base_seq=seq;
		base_ack=ack;
		goto seq_finished;
	}

	/* if this is the first segment we see then just add it */
	if( !ual1 ){
		ual1=g_mem_chunk_alloc(tcp_unacked_chunk);
		ual1->next=NULL;
		ual1->frame=pinfo->fd->num;
		ual1->ack_frame=0;
		ual1->ack=0;
		ual1->num_acks=0;
		ual1->seq=seq;
		ual1->nextseq=seq+seglen;
		ual1->ts.secs=pinfo->fd->abs_secs;
		ual1->ts.nsecs=pinfo->fd->abs_usecs*1000;
		ual1->window=window;
		base_seq=seq;
		goto seq_finished;
	}

	/* if we get past here we know that ual1 points to a segment */

	/* To handle FIN, just pretend they have a length of 1.
	   else the ACK following the FIN-ACK will look like it was
	   outside the window. */
	if( (!seglen) && (flags&TH_FIN) ){
		seglen=1;
	}

	/* if seq is beyond ual1->nextseq we have lost a segment */
	if (GT_SEQ(seq, ual1->nextseq)) {
		struct tcp_acked *ta;

		ta=tcp_analyze_get_acked_struct(pinfo->fd->num, TRUE);
		ta->flags|=TCP_A_LOST_PACKET;

		/* just add the segment to the beginning of the list */
		ual=g_mem_chunk_alloc(tcp_unacked_chunk);
		ual->next=ual1;
		ual->frame=pinfo->fd->num;
		ual->ack_frame=0;
		ual->ack=0;
		ual->num_acks=0;
		ual->seq=seq;
		ual->nextseq=seq+seglen;
		ual->ts.secs=pinfo->fd->abs_secs;
		ual->ts.nsecs=pinfo->fd->abs_usecs*1000;
		ual->window=window;
		ual1=ual;
		goto seq_finished;
	}

	/* keep-alives are empty semgents with a sequence number -1 of what
	 * we would expect.
	 */
	if( (!seglen) && EQ_SEQ(seq, (ual1->nextseq-1)) ){
		struct tcp_acked *ta;

		ta=tcp_analyze_get_acked_struct(pinfo->fd->num, TRUE);
		ta->flags|=TCP_A_KEEP_ALIVE;
		goto seq_finished;
	}


	/* if this is an empty segment, just skip it all */
	if( !seglen ){
		goto seq_finished;
	}

	/* check if the sequence number is lower than expected, i.e. retransmission */
	if( LT_SEQ(seq, ual1->nextseq )){
		struct tcp_acked *ta;

		ta=tcp_analyze_get_acked_struct(pinfo->fd->num, TRUE);
		ta->flags|=TCP_A_RETRANSMISSION;

		/* did this segment contain any more data we havent seen yet?
		 * if so we can just increase nextseq
		 */
		if(GT_SEQ((seq+seglen), ual1->nextseq)){
			ual1->nextseq=seq+seglen;
			ual1->frame=pinfo->fd->num;
			ual1->ts.secs=pinfo->fd->abs_secs;
			ual1->ts.nsecs=pinfo->fd->abs_usecs*1000;
		}
		goto seq_finished;
	}

	/* just add the segment to the beginning of the list */
	ual=g_mem_chunk_alloc(tcp_unacked_chunk);
	ual->next=ual1;
	ual->frame=pinfo->fd->num;
	ual->ack_frame=0;
	ual->ack=0;
	ual->num_acks=0;
	ual->seq=seq;
	ual->nextseq=seq+seglen;
	ual->ts.secs=pinfo->fd->abs_secs;
	ual->ts.nsecs=pinfo->fd->abs_usecs*1000;
	ual->window=window;
	ual1=ual;

seq_finished:


	/* handle the ack numbers */

	/* if we dont have the ack flag its not much we can do */
	if( !(flags&TH_ACK)){
		goto ack_finished;
	}

	/* if we havent seen anything yet in the other direction we dont
	 * know what this one acks */
	if( !ual2 ){
		goto ack_finished;
	}

	/* if we dont have any real segments in the other direction not
	 * acked yet (as we see from the magic frame==0 entry)
	 * then there is no point in continuing
	 */
	if( !ual2->frame ){
		goto ack_finished;
	}

	/* if we get here we know ual2 is valid */

	/* if we are acking beyong what we have seen in the other direction
	 * we must have lost packets. Not much point in keeping the segments
	 * in the other direction either.
	 */
	if( GT_SEQ(ack, ual2->nextseq )){
		struct tcp_acked *ta;

		ta=tcp_analyze_get_acked_struct(pinfo->fd->num, TRUE);
		ta->flags|=TCP_A_ACK_LOST_PACKET;
		for(ual=ual2;ual2;ual2=ual){
			ual=ual2->next;
			g_mem_chunk_free(tcp_unacked_chunk, ual2);
		}
		goto ack_finished;
	}


	/* does this ACK ack all semgents we have seen in the other direction?*/
	if( EQ_SEQ(ack, ual2->nextseq )){
		struct tcp_acked *ta;

		ta=tcp_analyze_get_acked_struct(pinfo->fd->num, TRUE);
		ta->frame_acked=ual2->frame;
		ta->ts.secs=pinfo->fd->abs_secs-ual2->ts.secs;
		ta->ts.nsecs=pinfo->fd->abs_usecs*1000-ual2->ts.nsecs;
		if(ta->ts.nsecs<0){
			ta->ts.nsecs+=1000000000;
			ta->ts.secs--;
		}

		/* its all been ACKed so we dont need to keep them anymore */
		for(ual=ual2;ual2;ual2=ual){
			ual=ual2->next;
			g_mem_chunk_free(tcp_unacked_chunk, ual2);
		}
		goto ack_finished;
	}

	/* ok it only ACKs part of what we have seen. Find out how much
	 * update and remove the ACKed segments
	 */
	for(ual=ual2;ual->next;ual=ual->next){
		if( GE_SEQ(ack, ual->next->nextseq)){
			break;
		}
	}
	if(ual->next){
		struct tcp_unacked *tmpual=NULL;
		struct tcp_unacked *ackedual=NULL;
		struct tcp_acked *ta;

		/* XXX normal ACK*/
		ackedual=ual->next;

		ta=tcp_analyze_get_acked_struct(pinfo->fd->num, TRUE);
		ta->frame_acked=ackedual->frame;
		ta->ts.secs=pinfo->fd->abs_secs-ackedual->ts.secs;
		ta->ts.nsecs=pinfo->fd->abs_usecs*1000-ackedual->ts.nsecs;
		if(ta->ts.nsecs<0){
			ta->ts.nsecs+=1000000000;
			ta->ts.secs--;
		}

		/* just delete all ACKed segments */
		tmpual=ual->next;
		ual->next=NULL;
		for(ual=tmpual;ual;ual=tmpual){
			tmpual=ual->next;
			g_mem_chunk_free(tcp_unacked_chunk, ual);
		}

	}

ack_finished:
	/* we might have deleted the entire ual2 list, if this is an ACK,
	   make sure ual2 at least has a dummy entry for the current ACK */
	if( (!ual2) && (flags&TH_ACK) ){
		ual2=g_mem_chunk_alloc(tcp_unacked_chunk);
		ual2->next=NULL;
		ual2->frame=0;
		ual2->ack_frame=0;
		ual2->ack=0;
		ual2->num_acks=0;
		ual2->seq=ack;
		ual2->nextseq=ack;
		ual2->ts.secs=0;
		ual2->ts.nsecs=0;
		ual2->window=window;
	}

	/* update the ACK counter and check for
	   duplicate ACKs*/
	/* go to the oldest segment in the list of segments 
	   in the other direction */
	/* XXX we should guarantee ual2 to always be non NULL here
	   so we can skip the ual/ual2 tests */
	for(ual=ual2;ual&&ual->next;ual=ual->next)
		;
	if(ual2){
		/* we only consider this being a potential duplicate ack
		   if the segment length is 0 (ack only segment)
		   and if it acks something previous to oldest segment
		   in the other direction */
		if((!seglen)&&LE_SEQ(ack,ual->seq)){
			/* if this is the first ack to keep track of, it is not
			   a duplicate */
			if(ual->num_acks==0){
				ual->ack=ack;
				ual->ack_frame=pinfo->fd->num;
				ual->num_acks=1;
			/* if this ack is different, store this one 
			   instead and forget the previous one(s) */
			} else if(ual->ack!=ack){
				ual->ack=ack;
				ual->ack_frame=pinfo->fd->num;
				ual->num_acks=1;
			/* this has to be a duplicate ack */
			} else {
				ual->num_acks++;
			}	
			
			/* ok we have found a potential duplicate ack */
			if(ual->num_acks>1){
				struct tcp_acked *ta;
				ta=tcp_analyze_get_acked_struct(pinfo->fd->num, TRUE);
				ta->flags|=TCP_A_DUPLICATE_ACK;
			}
		}		

	}


	/* check for zero window probes 
	   a zero window probe is when a TCP tries to write 1 byte segments
	   where the remote side has advertised a window of 0 bytes.
	   We only do this check if we actually have seen anything from the
	   other side of this connection.

	   We also assume ual still points to the last entry in the ual2
	   list from the section above.

	   At the same time, check for violations, i.e. attempts to write >1
	   byte to a zero-window.
	*/
	/* XXX we should not need to do the ual->frame check here?
	   might be a bug somewhere. look for it later .
	*/
	if(ual2&&(ual->frame)){
		if((seglen==1)&&(ual->window==0)){
			struct tcp_acked *ta;
			ta=tcp_analyze_get_acked_struct(pinfo->fd->num, TRUE);
			ta->flags|=TCP_A_ZERO_WINDOW_PROBE;
		}
		if((seglen>1)&&(ual->window==0)){
			struct tcp_acked *ta;
			ta=tcp_analyze_get_acked_struct(pinfo->fd->num, TRUE);
			ta->flags|=TCP_A_ZERO_WINDOW_VIOLATION;
		}
	}

	/* check for zero window */
	if(!window){
		struct tcp_acked *ta;
		ta=tcp_analyze_get_acked_struct(pinfo->fd->num, TRUE);
		ta->flags|=TCP_A_ZERO_WINDOW;
	}


	/* store the lists back in our struct */
	if(direction>=0){
		/*
		 * XXX - if direction == 0, that'll be true for packets
		 * from both sides of the connection, so this won't
		 * work.
		 *
		 * That'd be a connection from a given port on a machine
		 * to that same port on the same machine; does that ever
		 * happen?
		 */
		tcpd->ual1=ual1;
		tcpd->ual2=ual2;
		tcpd->base_seq1=base_seq;
	} else {
		tcpd->ual1=ual2;
		tcpd->ual2=ual1;
		tcpd->base_seq2=base_seq;
	}


	if(tcp_relative_seq){
		struct tcp_rel_seq *trs;
		/* remember relative seq/ack number base for this packet */
		trs=g_mem_chunk_alloc(tcp_rel_seq_chunk);
		trs->seq_base=base_seq;
		trs->ack_base=base_ack;
		g_hash_table_insert(tcp_rel_seq_table, (void *)pinfo->fd->num, trs);
	}
}

static void
tcp_print_sequence_number_analysis(packet_info *pinfo, tvbuff_t *tvb, proto_tree *parent_tree)
{
	struct tcp_acked *ta;
	proto_item *item;
	proto_tree *tree;

	ta=tcp_analyze_get_acked_struct(pinfo->fd->num, FALSE);
	if(!ta){
		return;
	}

	item=proto_tree_add_text(parent_tree, tvb, 0, 0, "SEQ/ACK analysis");
	tree=proto_item_add_subtree(item, ett_tcp_analysis);

	/* encapsulate all proto_tree_add_xxx in ifs so we only print what
	   data we actually have */
	if(ta->frame_acked){
		proto_tree_add_uint(tree, hf_tcp_analysis_acks_frame,
			tvb, 0, 0, ta->frame_acked);
	}
	if( ta->ts.secs || ta->ts.nsecs ){
		proto_tree_add_time(tree, hf_tcp_analysis_ack_rtt,
		tvb, 0, 0, &ta->ts);
	}

	if(ta->flags){
		proto_item *flags_item=NULL;
		proto_tree *flags_tree=NULL;

		flags_item = proto_tree_add_item(tree, hf_tcp_analysis_flags, tvb, 0, -1, FALSE);
		flags_tree=proto_item_add_subtree(flags_item, ett_tcp_analysis);
		if( ta->flags&TCP_A_RETRANSMISSION ){
			proto_tree_add_none_format(flags_tree, hf_tcp_analysis_retransmission, tvb, 0, 0, "This frame is a (suspected) retransmission");
			if(check_col(pinfo->cinfo, COL_INFO)){
				col_prepend_fstr(pinfo->cinfo, COL_INFO, "[TCP Retransmission] ");
			}
		}
		if( ta->flags&TCP_A_LOST_PACKET ){
			proto_tree_add_none_format(flags_tree, hf_tcp_analysis_lost_packet, tvb, 0, 0, "A segment before this frame was lost");
			if(check_col(pinfo->cinfo, COL_INFO)){
				col_prepend_fstr(pinfo->cinfo, COL_INFO, "[TCP Previous segment lost] ");
			}
		}
		if( ta->flags&TCP_A_ACK_LOST_PACKET ){
			proto_tree_add_none_format(flags_tree, hf_tcp_analysis_ack_lost_packet, tvb, 0, 0, "This frame ACKs a segment we have not seen (lost?)");
			if(check_col(pinfo->cinfo, COL_INFO)){
				col_prepend_fstr(pinfo->cinfo, COL_INFO, "[TCP ACKed lost segment] ");
			}
		}
		if( ta->flags&TCP_A_KEEP_ALIVE ){
			proto_tree_add_none_format(flags_tree, hf_tcp_analysis_keep_alive, tvb, 0, 0, "This is a TCP keep-alive segment");
			if(check_col(pinfo->cinfo, COL_INFO)){
				col_prepend_fstr(pinfo->cinfo, COL_INFO, "[TCP Keep-Alive] ");
			}
		}
		if( ta->flags&TCP_A_DUPLICATE_ACK ){
			proto_tree_add_none_format(flags_tree, hf_tcp_analysis_duplicate_ack, tvb, 0, 0, "This is a TCP duplicate ack");
			if(check_col(pinfo->cinfo, COL_INFO)){
				col_prepend_fstr(pinfo->cinfo, COL_INFO, "[TCP Duplicate ACK] ");
			}
		}
		if( ta->flags&TCP_A_ZERO_WINDOW_PROBE ){
			proto_tree_add_none_format(flags_tree, hf_tcp_analysis_zero_window_probe, tvb, 0, 0, "This is a TCP zero-window-probe");
			if(check_col(pinfo->cinfo, COL_INFO)){
				col_prepend_fstr(pinfo->cinfo, COL_INFO, "[TCP ZeroWindowProbe] ");
			}
		}
		if( ta->flags&TCP_A_ZERO_WINDOW ){
			proto_tree_add_none_format(flags_tree, hf_tcp_analysis_zero_window, tvb, 0, 0, "This is a ZeroWindow segment");
			if(check_col(pinfo->cinfo, COL_INFO)){
				col_prepend_fstr(pinfo->cinfo, COL_INFO, "[TCP ZeroWindow] ");
			}
		}
		if( ta->flags&TCP_A_ZERO_WINDOW_VIOLATION ){
			proto_tree_add_none_format(flags_tree, hf_tcp_analysis_zero_window_violation, tvb, 0, 0, "This is a ZeroWindow violation, attempts to write >1 byte of data to a zero-window");
			if(check_col(pinfo->cinfo, COL_INFO)){
				col_prepend_fstr(pinfo->cinfo, COL_INFO, "[TCP ZeroWindowViolation] ");
			}
		}
	}

}


/* Do we still need to do this ...remove_all() even though we dont need
 * to do anything special?  The glib docs are not clear on this and
 * its better safe than sorry
 */
static gboolean
free_all_acked(gpointer key_arg _U_, gpointer value _U_, gpointer user_data _U_)
{
	return TRUE;
}

static guint
tcp_acked_hash(gconstpointer k)
{
	guint32 frame = (guint32)k;

	return frame;
}
static gint
tcp_acked_equal(gconstpointer k1, gconstpointer k2)
{
	guint32 frame1 = (guint32)k1;
	guint32 frame2 = (guint32)k2;

	return frame1==frame2;
}

static void
tcp_analyze_seq_init(void)
{
	/* first destroy the tables */
	if( tcp_analyze_acked_table ){
		g_hash_table_foreach_remove(tcp_analyze_acked_table,
			free_all_acked, NULL);
		g_hash_table_destroy(tcp_analyze_acked_table);
		tcp_analyze_acked_table = NULL;
	}
	if( tcp_rel_seq_table ){
		g_hash_table_foreach_remove(tcp_rel_seq_table,
			free_all_acked, NULL);
		g_hash_table_destroy(tcp_rel_seq_table);
		tcp_rel_seq_table = NULL;
	}

	/*
	 * Now destroy the chunk from which the conversation table
	 * structures were allocated.
	 */
	if (tcp_analysis_chunk) {
		g_mem_chunk_destroy(tcp_analysis_chunk);
		tcp_analysis_chunk = NULL;
	}
	if (tcp_unacked_chunk) {
		g_mem_chunk_destroy(tcp_unacked_chunk);
		tcp_unacked_chunk = NULL;
	}
	if (tcp_acked_chunk) {
		g_mem_chunk_destroy(tcp_acked_chunk);
		tcp_acked_chunk = NULL;
	}
	if (tcp_rel_seq_chunk) {
		g_mem_chunk_destroy(tcp_rel_seq_chunk);
		tcp_rel_seq_chunk = NULL;
	}

	if(tcp_analyze_seq){
		tcp_analyze_acked_table = g_hash_table_new(tcp_acked_hash,
			tcp_acked_equal);
		tcp_rel_seq_table = g_hash_table_new(tcp_acked_hash,
			tcp_acked_equal);
		tcp_analysis_chunk = g_mem_chunk_new("tcp_analysis_chunk",
			sizeof(struct tcp_analysis),
			tcp_analysis_count * sizeof(struct tcp_analysis),
			G_ALLOC_ONLY);
		tcp_unacked_chunk = g_mem_chunk_new("tcp_unacked_chunk",
			sizeof(struct tcp_unacked),
			tcp_unacked_count * sizeof(struct tcp_unacked),
			G_ALLOC_ONLY);
		tcp_acked_chunk = g_mem_chunk_new("tcp_acked_chunk",
			sizeof(struct tcp_acked),
			tcp_acked_count * sizeof(struct tcp_acked),
			G_ALLOC_ONLY);
		if(tcp_relative_seq){
			tcp_rel_seq_chunk = g_mem_chunk_new("tcp_rel_seq_chunk",
				sizeof(struct tcp_rel_seq),
				tcp_rel_seq_count * sizeof(struct tcp_rel_seq),
				G_ALLOC_ONLY);
		}
	}

}

/* **************************************************************************
 * End of tcp sequence number analysis
 * **************************************************************************/




/* Minimum TCP header length. */
#define	TCPH_MIN_LEN	20

/*
 *	TCP option
 */

#define TCPOPT_NOP		1	/* Padding */
#define TCPOPT_EOL		0	/* End of options */
#define TCPOPT_MSS		2	/* Segment size negotiating */
#define TCPOPT_WINDOW		3	/* Window scaling */
#define TCPOPT_SACK_PERM        4       /* SACK Permitted */
#define TCPOPT_SACK             5       /* SACK Block */
#define TCPOPT_ECHO             6
#define TCPOPT_ECHOREPLY        7
#define TCPOPT_TIMESTAMP	8	/* Better RTT estimations/PAWS */
#define TCPOPT_CC               11
#define TCPOPT_CCNEW            12
#define TCPOPT_CCECHO           13
#define TCPOPT_MD5              19      /* RFC2385 */

/*
 *     TCP option lengths
 */

#define TCPOLEN_MSS            4
#define TCPOLEN_WINDOW         3
#define TCPOLEN_SACK_PERM      2
#define TCPOLEN_SACK_MIN       2
#define TCPOLEN_ECHO           6
#define TCPOLEN_ECHOREPLY      6
#define TCPOLEN_TIMESTAMP      10
#define TCPOLEN_CC             6
#define TCPOLEN_CCNEW          6
#define TCPOLEN_CCECHO         6
#define TCPOLEN_MD5            18



/* Desegmentation of TCP streams */
/* table to hold defragmented TCP streams */
static GHashTable *tcp_fragment_table = NULL;
static void
tcp_fragment_init(void)
{
	fragment_table_init(&tcp_fragment_table);
}

/* functions to trace tcp segments */
/* Enable desegmenting of TCP streams */
static gboolean tcp_desegment = FALSE;

static GHashTable *tcp_segment_table = NULL;
static GMemChunk *tcp_segment_key_chunk = NULL;
static int tcp_segment_init_count = 200;
static GMemChunk *tcp_segment_address_chunk = NULL;
static int tcp_segment_address_init_count = 500;

typedef struct _tcp_segment_key {
	/* for own bookkeeping inside packet-tcp.c */
	address *src;
	address *dst;
	guint32 seq;
	/* xxx */
	guint16 sport;
	guint16 dport;
	guint32 start_seq;
	guint32 tot_len;
	guint32 first_frame;
} tcp_segment_key;

static gboolean
free_all_segments(gpointer key_arg, gpointer value _U_, gpointer user_data _U_)
{
	tcp_segment_key *key = key_arg;

	if((key->src)&&(key->src->data)){
		g_free((gpointer)key->src->data);
		key->src->data=NULL;
	}

	if((key->dst)&&(key->dst->data)){
		g_free((gpointer)key->dst->data);
		key->dst->data=NULL;
	}

	return TRUE;
}

static guint
tcp_segment_hash(gconstpointer k)
{
	const tcp_segment_key *key = (const tcp_segment_key *)k;

	return key->seq+key->sport;
}

static gint
tcp_segment_equal(gconstpointer k1, gconstpointer k2)
{
	const tcp_segment_key *key1 = (const tcp_segment_key *)k1;
	const tcp_segment_key *key2 = (const tcp_segment_key *)k2;

	return ( ( (key1->seq==key2->seq)
		 &&(ADDRESSES_EQUAL(key1->src, key2->src))
		 &&(ADDRESSES_EQUAL(key1->dst, key2->dst))
		 &&(key1->sport==key2->sport)
		 &&(key1->dport==key2->dport)
		 ) ? TRUE:FALSE);
}

static void
tcp_desegment_init(void)
{
	/*
	 * Free this before freeing any memory chunks; those
	 * chunks contain data we'll look at in "free_all_segments()".
	 */
	if(tcp_segment_table){
		g_hash_table_foreach_remove(tcp_segment_table,
			free_all_segments, NULL);
		g_hash_table_destroy(tcp_segment_table);
		tcp_segment_table = NULL;
	}

	if(tcp_segment_key_chunk){
		g_mem_chunk_destroy(tcp_segment_key_chunk);
		tcp_segment_key_chunk = NULL;
	}
	if(tcp_segment_address_chunk){
		g_mem_chunk_destroy(tcp_segment_address_chunk);
		tcp_segment_address_chunk = NULL;
	}

	/* dont allocate any hash table or memory chunks unless the user
	   really uses this option
	*/
	if(!tcp_desegment){
		return;
	}

	tcp_segment_table = g_hash_table_new(tcp_segment_hash,
		tcp_segment_equal);

	tcp_segment_key_chunk = g_mem_chunk_new("tcp_segment_key_chunk",
		sizeof(tcp_segment_key),
		tcp_segment_init_count*sizeof(tcp_segment_key),
		G_ALLOC_ONLY);

	tcp_segment_address_chunk = g_mem_chunk_new("tcp_segment_address_chunk",
		sizeof(address),
		tcp_segment_address_init_count*sizeof(address),
		G_ALLOC_ONLY);
}

static void
desegment_tcp(tvbuff_t *tvb, packet_info *pinfo, int offset,
		guint32 seq, guint32 nxtseq,
		guint32 sport, guint32 dport,
		proto_tree *tree, proto_tree *tcp_tree)
{
	struct tcpinfo *tcpinfo = pinfo->private_data;
	fragment_data *ipfd_head=NULL;
	tcp_segment_key old_tsk, *tsk;
	gboolean must_desegment = FALSE;
	gboolean called_dissector = FALSE;
	int deseg_offset;
	guint32 deseg_seq;
	gint nbytes;

	/*
	 * Initialize these to assume no desegmentation.
	 * If that's not the case, these will be set appropriately
	 * by the subdissector.
	 */
	pinfo->desegment_offset = 0;
	pinfo->desegment_len = 0;

	/*
	 * Initialize this to assume that this segment will just be
	 * added to the middle of a desegmented chunk of data, so
	 * that we should show it all as data.
	 * If that's not the case, it will be set appropriately.
	 */
	deseg_offset = offset;

	/* First we must check if this TCP segment should be desegmented.
	   This is only to check if we should desegment this packet,
	   so we dont spend time doing COPY_ADDRESS/g_free.
	   We just "borrow" some address structures from pinfo instead. Cheaper.
	*/
	old_tsk.src = &pinfo->src;
	old_tsk.dst = &pinfo->dst;
	old_tsk.sport = sport;
	old_tsk.dport = dport;
	old_tsk.seq = seq;
	tsk = g_hash_table_lookup(tcp_segment_table, &old_tsk);

	if(tsk){
		/* OK, this segment was found, which means it continues
		   a higher-level PDU. This means we must desegment it.
		   Add it to the defragmentation lists.
		*/
		ipfd_head = fragment_add(tvb, offset, pinfo, tsk->first_frame,
			tcp_fragment_table,
			seq - tsk->start_seq,
			nxtseq - seq,
			(LT_SEQ (nxtseq,tsk->start_seq + tsk->tot_len)) );

		if(!ipfd_head){
			/* fragment_add() returned NULL, This means that
			   desegmentation is not completed yet.
			   (its like defragmentation but we know we will
			    always add the segments in order).
			   XXX - no, we don't; there is no guarantee that
			   TCP segments are in order on the wire.

			   we must add next segment to our table so we will
			   find it later.
			*/
			tcp_segment_key *new_tsk;

			new_tsk = g_mem_chunk_alloc(tcp_segment_key_chunk);
			memcpy(new_tsk, tsk, sizeof(tcp_segment_key));
			new_tsk->seq=nxtseq;
			g_hash_table_insert(tcp_segment_table,new_tsk,new_tsk);
		}
	} else {
		/* This segment was not found in our table, so it doesn't
		   contain a continuation of a higher-level PDU.
		   Call the normal subdissector.
		*/
		decode_tcp_ports(tvb, offset, pinfo, tree,
				sport, dport);
		called_dissector = TRUE;

		/* Did the subdissector ask us to desegment some more data
		   before it could handle the packet?
		   If so we have to create some structures in our table but
		   this is something we only do the first time we see this
		   packet.
		*/
		if(pinfo->desegment_len) {
			if (!pinfo->fd->flags.visited)
				must_desegment = TRUE;

			/*
			 * Set "deseg_offset" to the offset in "tvb"
			 * of the first byte of data that the
			 * subdissector didn't process.
			 */
			deseg_offset = offset + pinfo->desegment_offset;
		}

		/* Either no desegmentation is necessary, or this is
		   segment contains the beginning but not the end of
		   a higher-level PDU and thus isn't completely
		   desegmented.
		*/
		ipfd_head = NULL;
	}

	/* is it completely desegmented? */
	if(ipfd_head){
		fragment_data *ipfd;

		/*
		 * Yes, we think it is.
		 * We only call subdissector for the last segment.
		 * Note that the last segment may include more than what
		 * we needed.
		 */
		if(GE_SEQ(nxtseq, tsk->start_seq + tsk->tot_len)){
			/*
			 * OK, this is the last segment.
			 * Let's call the subdissector with the desegmented
			 * data.
			 */
			tvbuff_t *next_tvb;
			int old_len;

			/* create a new TVB structure for desegmented data */
			next_tvb = tvb_new_real_data(ipfd_head->data,
					ipfd_head->datalen, ipfd_head->datalen);

			/* add this tvb as a child to the original one */
			tvb_set_child_real_data_tvbuff(tvb, next_tvb);

			/* add desegmented data to the data source list */
			add_new_data_source(pinfo, next_tvb, "Desegmented");

			/*
			 * Supply the sequence number of the first of the
			 * reassembled bytes.
			 */
			tcpinfo->seq = tsk->start_seq;

			/* indicate that this is reassembled data */
			tcpinfo->is_reassembled = TRUE;

			/* call subdissector */
			decode_tcp_ports(next_tvb, 0, pinfo, tree,
				sport, dport);
			called_dissector = TRUE;

			/*
			 * OK, did the subdissector think it was completely
			 * desegmented, or does it think we need even more
			 * data?
			 */
			old_len=(int)(tvb_reported_length(next_tvb)-tvb_reported_length_remaining(tvb, offset));
			if(pinfo->desegment_len &&
			    pinfo->desegment_offset<=old_len){
				tcp_segment_key *new_tsk;

				/*
				 * "desegment_len" isn't 0, so it needs more
				 * data for something - and "desegment_offset"
				 * is before "old_len", so it needs more data
				 * to dissect the stuff we thought was
				 * completely desegmented (as opposed to the
				 * stuff at the beginning being completely
				 * desegmented, but the stuff at the end
				 * being a new higher-level PDU that also
				 * needs desegmentation).
				 */
				fragment_set_partial_reassembly(pinfo,tsk->first_frame,tcp_fragment_table);
				tsk->tot_len = tvb_reported_length(next_tvb) + pinfo->desegment_len;

				/*
				 * Update tsk structure.
				 * Can ask ->next->next because at least there's a hdr and one
				 * entry in fragment_add()
				 */
				for(ipfd=ipfd_head->next; ipfd->next; ipfd=ipfd->next){
					old_tsk.seq = tsk->start_seq + ipfd->offset;
					new_tsk = g_hash_table_lookup(tcp_segment_table, &old_tsk);
					new_tsk->tot_len = tsk->tot_len;
				}

				/* this is the next segment in the sequence we want */
				new_tsk = g_mem_chunk_alloc(tcp_segment_key_chunk);
				memcpy(new_tsk, tsk, sizeof(tcp_segment_key));
				new_tsk->seq = nxtseq;
				g_hash_table_insert(tcp_segment_table,new_tsk,new_tsk);
			} else {
				/*
				 * Show the stuff in this TCP segment as
				 * just raw TCP segment data.
				 */
				nbytes =
				    tvb_reported_length_remaining(tvb, offset);
				proto_tree_add_text(tcp_tree, tvb, offset, -1,
				    "TCP segment data (%u byte%s)", nbytes,
				    plurality(nbytes, "", "s"));

				/*
				 * The subdissector thought it was completely
				 * desegmented (although the stuff at the
				 * end may, in turn, require desegmentation),
				 * so we show a tree with all segments.
				 */
				show_fragment_tree(ipfd_head, &tcp_segment_items,
					tcp_tree, pinfo, next_tvb);

				/* Did the subdissector ask us to desegment
				   some more data?  This means that the data
				   at the beginning of this segment completed
				   a higher-level PDU, but the data at the
				   end of this segment started a higher-level
				   PDU but didn't complete it.

				   If so, we have to create some structures
				   in our table, but this is something we
				   only do the first time we see this packet.
				*/
				if(pinfo->desegment_len) {
					if (!pinfo->fd->flags.visited)
						must_desegment = TRUE;

					/* The stuff we couldn't dissect
					   must have come from this segment,
					   so it's all in "tvb".

				 	   "pinfo->desegment_offset" is
				 	   relative to the beginning of
				 	   "next_tvb"; we want an offset
				 	   relative to the beginning of "tvb".

				 	   First, compute the offset relative
				 	   to the *end* of "next_tvb" - i.e.,
				 	   the number of bytes before the end
				 	   of "next_tvb" at which the
				 	   subdissector stopped.  That's the
				 	   length of "next_tvb" minus the
				 	   offset, relative to the beginning
				 	   of "next_tvb, at which the
				 	   subdissector stopped.
				 	*/
					deseg_offset =
					    ipfd_head->datalen - pinfo->desegment_offset;

					/* "tvb" and "next_tvb" end at the
					   same byte of data, so the offset
					   relative to the end of "next_tvb"
					   of the byte at which we stopped
					   is also the offset relative to
					   the end of "tvb" of the byte at
					   which we stopped.

					   Convert that back into an offset
					   relative to the beginninng of
					   "tvb", by taking the length of
					   "tvb" and subtracting the offset
					   relative to the end.
					*/
					deseg_offset=tvb_reported_length(tvb) - deseg_offset;
				}
			}
		}
	}

	if (must_desegment) {
	    tcp_segment_key *tsk, *new_tsk;

	    /*
	     * The sequence number at which the stuff to be desegmented
	     * starts is the sequence number of the byte at an offset
	     * of "deseg_offset" into "tvb".
	     *
	     * The sequence number of the byte at an offset of "offset"
	     * is "seq", i.e. the starting sequence number of this
	     * segment, so the sequence number of the byte at
	     * "deseg_offset" is "seq + (deseg_offset - offset)".
	     */
	    deseg_seq = seq + (deseg_offset - offset);

	    /*
	     * XXX - how do we detect out-of-order transmissions?
	     * We can't just check for "nxtseq" being greater than
	     * "tsk->start_seq"; for now, we check for the difference
	     * being less than a megabyte, but this is a really
	     * gross hack - we really need to handle out-of-order
	     * transmissions correctly.
	     */
	    if ((nxtseq - deseg_seq) <= 1024*1024) {
		/* OK, subdissector wants us to desegment
		   some data before it can process it. Add
		   what remains of this packet and set
		   up next packet/sequence number as well.

		   We must remember this segment
		*/
		tsk = g_mem_chunk_alloc(tcp_segment_key_chunk);
		tsk->src = g_mem_chunk_alloc(tcp_segment_address_chunk);
		COPY_ADDRESS(tsk->src, &pinfo->src);
		tsk->dst = g_mem_chunk_alloc(tcp_segment_address_chunk);
		COPY_ADDRESS(tsk->dst, &pinfo->dst);
		tsk->seq = deseg_seq;
		tsk->start_seq = tsk->seq;
		tsk->tot_len = nxtseq - tsk->start_seq + pinfo->desegment_len;
		tsk->first_frame = pinfo->fd->num;
		tsk->sport=sport;
		tsk->dport=dport;
		g_hash_table_insert(tcp_segment_table, tsk, tsk);

		/* Add portion of segment unprocessed by the subdissector
		   to defragmentation lists */
		fragment_add(tvb, deseg_offset, pinfo, tsk->first_frame,
		    tcp_fragment_table,
		    tsk->seq - tsk->start_seq,
		    nxtseq - tsk->start_seq,
		    LT_SEQ (nxtseq, tsk->start_seq + tsk->tot_len));

		/* this is the next segment in the sequence we want */
		new_tsk = g_mem_chunk_alloc(tcp_segment_key_chunk);
		memcpy(new_tsk, tsk, sizeof(tcp_segment_key));
		new_tsk->seq = nxtseq;
		g_hash_table_insert(tcp_segment_table,new_tsk,new_tsk);
	    }
	}

	if (!called_dissector || pinfo->desegment_len != 0) {
		/*
		 * Either we didn't call the subdissector at all (i.e.,
		 * this is a segment that contains the middle of a
		 * higher-level PDU, but contains neither the beginning
		 * nor the end), or the subdissector couldn't dissect it
		 * all, as some data was missing (i.e., it set
		 * "pinfo->desegment_len" to the amount of additional
		 * data it needs).
		 */
		if (pinfo->desegment_offset == 0) {
			/*
			 * It couldn't, in fact, dissect any of it (the
			 * first byte it couldn't dissect is at an offset
			 * of "pinfo->desegment_offset" from the beginning
			 * of the payload, and that's 0).
			 * Just mark this as TCP.
			 */
			if (check_col(pinfo->cinfo, COL_PROTOCOL)){
				col_set_str(pinfo->cinfo, COL_PROTOCOL, "TCP");
			}
			if (check_col(pinfo->cinfo, COL_INFO)){
				col_set_str(pinfo->cinfo, COL_INFO, "[Desegmented TCP]");
			}
		}

		/*
		 * Show what's left in the packet as just raw TCP segment
		 * data.
		 * XXX - remember what protocol the last subdissector
		 * was, and report it as a continuation of that, instead?
		 */
		nbytes = tvb_reported_length_remaining(tvb, deseg_offset);
		proto_tree_add_text(tcp_tree, tvb, deseg_offset, -1,
		    "TCP segment data (%u byte%s)", nbytes,
		    plurality(nbytes, "", "s"));
	}
	pinfo->can_desegment=0;
	pinfo->desegment_offset = 0;
	pinfo->desegment_len = 0;
}

/*
 * Loop for dissecting PDUs within a TCP stream; assumes that a PDU
 * consists of a fixed-length chunk of data that contains enough information
 * to determine the length of the PDU, followed by rest of the PDU.
 *
 * The first three arguments are the arguments passed to the dissector
 * that calls this routine.
 *
 * "proto_desegment" is the dissector's flag controlling whether it should
 * desegment PDUs that cross TCP segment boundaries.
 *
 * "fixed_len" is the length of the fixed-length part of the PDU.
 *
 * "get_pdu_len()" is a routine called to get the length of the PDU from
 * the fixed-length part of the PDU; it's passed "tvb" and "offset".
 *
 * "dissect_pdu()" is the routine to dissect a PDU.
 */
void
tcp_dissect_pdus(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
		 gboolean proto_desegment, guint fixed_len,
		 guint (*get_pdu_len)(tvbuff_t *, int),
		 void (*dissect_pdu)(tvbuff_t *, packet_info *, proto_tree *))
{
  volatile int offset = 0;
  int offset_before;
  guint length_remaining;
  guint plen;
  guint length;
  tvbuff_t *next_tvb;

  while (tvb_reported_length_remaining(tvb, offset) != 0) {
    /*
     * We use "tvb_ensure_length_remaining()" to make sure there actually
     * *is* data remaining.  The protocol we're handling could conceivably
     * consists of a sequence of fixed-length PDUs, and therefore the
     * "get_pdu_len" routine might not actually fetch anything from
     * the tvbuff, and thus might not cause an exception to be thrown if
     * we've run past the end of the tvbuff.
     *
     * This means we're guaranteed that "length_remaining" is positive.
     */
    length_remaining = tvb_ensure_length_remaining(tvb, offset);

    /*
     * Can we do reassembly?
     */
    if (proto_desegment && pinfo->can_desegment) {
      /*
       * Yes - is the fixed-length part of the PDU split across segment
       * boundaries?
       */
      if (length_remaining < fixed_len) {
	/*
	 * Yes.  Tell the TCP dissector where the data for this message
	 * starts in the data it handed us, and how many more bytes we
	 * need, and return.
	 */
	pinfo->desegment_offset = offset;
	pinfo->desegment_len = fixed_len - length_remaining;
	return;
      }
    }

    /*
     * Get the length of the PDU.
     */
    plen = (*get_pdu_len)(tvb, offset);
    if (plen < fixed_len) {
      /*
       * The PDU length from the fixed-length portion probably didn't
       * include the fixed-length portion's length, and was probably so
       * large that the total length overflowed.
       *
       * Report this as an error.
       */
      show_reported_bounds_error(tvb, pinfo, tree);
      return;
    }

    /*
     * Can we do reassembly?
     */
    if (proto_desegment && pinfo->can_desegment) {
      /*
       * Yes - is the PDU split across segment boundaries?
       */
      if (length_remaining < plen) {
	/*
	 * Yes.  Tell the TCP dissector where the data for this message
	 * starts in the data it handed us, and how many more bytes we
	 * need, and return.
	 */
	pinfo->desegment_offset = offset;
	pinfo->desegment_len = plen - length_remaining;
	return;
      }
    }

    /*
     * Construct a tvbuff containing the amount of the payload we have
     * available.  Make its reported length the amount of data in the PDU.
     *
     * XXX - if reassembly isn't enabled. the subdissector will throw a
     * BoundsError exception, rather than a ReportedBoundsError exception.
     * We really want a tvbuff where the length is "length", the reported
     * length is "plen", and the "if the snapshot length were infinite"
     * length is the minimum of the reported length of the tvbuff handed
     * to us and "plen", with a new type of exception thrown if the offset
     * is within the reported length but beyond that third length, with
     * that exception getting the "Unreassembled Packet" error.
     */
    length = length_remaining;
    if (length > plen)
	length = plen;
    next_tvb = tvb_new_subset(tvb, offset, length, plen);

    /*
     * Dissect the PDU.
     *
     * Catch the ReportedBoundsError exception; if this particular message
     * happens to get a ReportedBoundsError exception, that doesn't mean
     * that we should stop dissecting PDUs within this frame or chunk of
     * reassembled data.
     *
     * If it gets a BoundsError, we can stop, as there's nothing more to
     * see, so we just re-throw it.
     */
    TRY {
      (*dissect_pdu)(next_tvb, pinfo, tree);
    }
    CATCH(BoundsError) {
      RETHROW;
    }
    CATCH(ReportedBoundsError) {
      show_reported_bounds_error(tvb, pinfo, tree);
    }
    ENDTRY;

    /*
     * Step to the next PDU.
     * Make sure we don't overflow.
     */
    offset_before = offset;
    offset += plen;
    if (offset <= offset_before)
      break;
  }
}

static void
tcp_info_append_uint(packet_info *pinfo, const char *abbrev, guint32 val)
{
  if (check_col(pinfo->cinfo, COL_INFO))
    col_append_fstr(pinfo->cinfo, COL_INFO, " %s=%u", abbrev, val);
}

static void
dissect_tcpopt_maxseg(const ip_tcp_opt *optp, tvbuff_t *tvb,
    int offset, guint optlen, packet_info *pinfo, proto_tree *opt_tree)
{
  guint16 mss;

  mss = tvb_get_ntohs(tvb, offset + 2);
  proto_tree_add_boolean_hidden(opt_tree, hf_tcp_option_mss, tvb, offset,
				optlen, TRUE);
  proto_tree_add_uint_format(opt_tree, hf_tcp_option_mss_val, tvb, offset,
			     optlen, mss, "%s: %u bytes", optp->name, mss);
  tcp_info_append_uint(pinfo, "MSS", mss);
}

static void
dissect_tcpopt_wscale(const ip_tcp_opt *optp, tvbuff_t *tvb,
    int offset, guint optlen, packet_info *pinfo, proto_tree *opt_tree)
{
  guint8 ws;

  ws = tvb_get_guint8(tvb, offset + 2);
  proto_tree_add_boolean_hidden(opt_tree, hf_tcp_option_wscale, tvb, 
				offset, optlen, TRUE);
  proto_tree_add_uint_format(opt_tree, hf_tcp_option_wscale_val, tvb,
			     offset, optlen, ws, "%s: %u (multiply by %u)", 
			     optp->name, ws, 1 << ws);
  tcp_info_append_uint(pinfo, "WS", ws);
}

static void
dissect_tcpopt_sack(const ip_tcp_opt *optp, tvbuff_t *tvb,
    int offset, guint optlen, packet_info *pinfo, proto_tree *opt_tree)
{
  proto_tree *field_tree = NULL;
  proto_item *tf;
  guint leftedge, rightedge;

  tf = proto_tree_add_text(opt_tree, tvb, offset,      optlen, "%s:", optp->name);
  offset += 2;	/* skip past type and length */
  optlen -= 2;	/* subtract size of type and length */
  while (optlen > 0) {
    if (field_tree == NULL) {
      /* Haven't yet made a subtree out of this option.  Do so. */
      field_tree = proto_item_add_subtree(tf, *optp->subtree_index);
    }
    if (optlen < 4) {
      proto_tree_add_text(field_tree, tvb, offset,      optlen,
        "(suboption would go past end of option)");
      break;
    }
    leftedge = tvb_get_ntohl(tvb, offset);
    optlen -= 4;
    if (optlen < 4) {
      proto_tree_add_text(field_tree, tvb, offset,      optlen,
        "(suboption would go past end of option)");
      break;
    }
    /* XXX - check whether it goes past end of packet */
    rightedge = tvb_get_ntohl(tvb, offset + 4);
    optlen -= 4;
    proto_tree_add_boolean_hidden(field_tree, hf_tcp_option_sack, tvb, 
				  offset, 8, TRUE);
    proto_tree_add_uint_format(field_tree, hf_tcp_option_sack_sle, tvb, 
			       offset, 8, leftedge, 
			       "left edge = %u", leftedge);
    proto_tree_add_uint_format(field_tree, hf_tcp_option_sack_sre, tvb, 
			       offset, 8, rightedge, 
			       "right edge = %u", rightedge);
    tcp_info_append_uint(pinfo, "SLE", leftedge);
    tcp_info_append_uint(pinfo, "SRE", rightedge);
    offset += 8;
  }
}

static void
dissect_tcpopt_echo(const ip_tcp_opt *optp, tvbuff_t *tvb,
    int offset, guint optlen, packet_info *pinfo, proto_tree *opt_tree)
{
  guint32 echo;

  echo = tvb_get_ntohl(tvb, offset + 2);
  proto_tree_add_boolean_hidden(opt_tree, hf_tcp_option_echo, tvb, offset,
				optlen, TRUE);
  proto_tree_add_text(opt_tree, tvb, offset,      optlen,
			"%s: %u", optp->name, echo);
  tcp_info_append_uint(pinfo, "ECHO", echo);
}

static void
dissect_tcpopt_timestamp(const ip_tcp_opt *optp, tvbuff_t *tvb,
    int offset, guint optlen, packet_info *pinfo, proto_tree *opt_tree)
{
  guint32 tsv, tser;

  tsv = tvb_get_ntohl(tvb, offset + 2);
  tser = tvb_get_ntohl(tvb, offset + 6);
  proto_tree_add_boolean_hidden(opt_tree, hf_tcp_option_time_stamp, tvb, 
				offset, optlen, TRUE);
  proto_tree_add_text(opt_tree, tvb, offset,      optlen,
    "%s: tsval %u, tsecr %u", optp->name, tsv, tser);
  tcp_info_append_uint(pinfo, "TSV", tsv);
  tcp_info_append_uint(pinfo, "TSER", tser);
}

static void
dissect_tcpopt_cc(const ip_tcp_opt *optp, tvbuff_t *tvb,
    int offset, guint optlen, packet_info *pinfo, proto_tree *opt_tree)
{
  guint32 cc;

  cc = tvb_get_ntohl(tvb, offset + 2);
  proto_tree_add_boolean_hidden(opt_tree, hf_tcp_option_cc, tvb, offset,
				optlen, TRUE);
  proto_tree_add_text(opt_tree, tvb, offset,      optlen,
			"%s: %u", optp->name, cc);
  tcp_info_append_uint(pinfo, "CC", cc);
}

static const ip_tcp_opt tcpopts[] = {
  {
    TCPOPT_EOL,
    "EOL",
    NULL,
    NO_LENGTH,
    0,
    NULL,
  },
  {
    TCPOPT_NOP,
    "NOP",
    NULL,
    NO_LENGTH,
    0,
    NULL,
  },
  {
    TCPOPT_MSS,
    "Maximum segment size",
    NULL,
    FIXED_LENGTH,
    TCPOLEN_MSS,
    dissect_tcpopt_maxseg
  },
  {
    TCPOPT_WINDOW,
    "Window scale",
    NULL,
    FIXED_LENGTH,
    TCPOLEN_WINDOW,
    dissect_tcpopt_wscale
  },
  {
    TCPOPT_SACK_PERM,
    "SACK permitted",
    NULL,
    FIXED_LENGTH,
    TCPOLEN_SACK_PERM,
    NULL,
  },
  {
    TCPOPT_SACK,
    "SACK",
    &ett_tcp_option_sack,
    VARIABLE_LENGTH,
    TCPOLEN_SACK_MIN,
    dissect_tcpopt_sack
  },
  {
    TCPOPT_ECHO,
    "Echo",
    NULL,
    FIXED_LENGTH,
    TCPOLEN_ECHO,
    dissect_tcpopt_echo
  },
  {
    TCPOPT_ECHOREPLY,
    "Echo reply",
    NULL,
    FIXED_LENGTH,
    TCPOLEN_ECHOREPLY,
    dissect_tcpopt_echo
  },
  {
    TCPOPT_TIMESTAMP,
    "Time stamp",
    NULL,
    FIXED_LENGTH,
    TCPOLEN_TIMESTAMP,
    dissect_tcpopt_timestamp
  },
  {
    TCPOPT_CC,
    "CC",
    NULL,
    FIXED_LENGTH,
    TCPOLEN_CC,
    dissect_tcpopt_cc
  },
  {
    TCPOPT_CCNEW,
    "CC.NEW",
    NULL,
    FIXED_LENGTH,
    TCPOLEN_CCNEW,
    dissect_tcpopt_cc
  },
  {
    TCPOPT_CCECHO,
    "CC.ECHO",
    NULL,
    FIXED_LENGTH,
    TCPOLEN_CCECHO,
    dissect_tcpopt_cc
  },
  {
    TCPOPT_MD5,
    "TCP MD5 signature",
    NULL,
    FIXED_LENGTH,
    TCPOLEN_MD5,
    NULL
  }
};

#define N_TCP_OPTS	(sizeof tcpopts / sizeof tcpopts[0])

/* Determine if there is a sub-dissector and call it.  This has been */
/* separated into a stand alone routine to other protocol dissectors */
/* can call to it, ie. socks	*/

void
decode_tcp_ports(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree, int src_port, int dst_port)
{
  tvbuff_t *next_tvb;
  int low_port, high_port;

  next_tvb = tvb_new_subset(tvb, offset, -1, -1);

/* determine if this packet is part of a conversation and call dissector */
/* for the conversation if available */

  if (try_conversation_dissector(&pinfo->src, &pinfo->dst, PT_TCP,
		src_port, dst_port, next_tvb, pinfo, tree))
    return;

  /* Do lookups with the subdissector table.
     We try the port number with the lower value first, followed by the
     port number with the higher value.  This means that, for packets
     where a dissector is registered for *both* port numbers:

	1) we pick the same dissector for traffic going in both directions;

	2) we prefer the port number that's more likely to be the right
	   one (as that prefers well-known ports to reserved ports);

     although there is, of course, no guarantee that any such strategy
     will always pick the right port number.

     XXX - we ignore port numbers of 0, as some dissectors use a port
     number of 0 to disable the port. */
  if (src_port > dst_port) {
    low_port = dst_port;
    high_port = src_port;
  } else {
    low_port = src_port;
    high_port = dst_port;
  }
  if (low_port != 0 &&
      dissector_try_port(subdissector_table, low_port, next_tvb, pinfo, tree))
    return;
  if (high_port != 0 &&
      dissector_try_port(subdissector_table, high_port, next_tvb, pinfo, tree))
    return;

  /* do lookup with the heuristic subdissector table */
  if (dissector_try_heuristic(heur_subdissector_list, next_tvb, pinfo, tree))
    return;

  /* Oh, well, we don't know this; dissect it as data. */
  call_dissector(data_handle,next_tvb, pinfo, tree);
}


static void
dissect_tcp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
  guint8  th_off_x2; /* combines th_off and th_x2 */
  guint16 th_sum;
  guint16 th_urp;
  proto_tree *tcp_tree = NULL, *field_tree = NULL;
  proto_item *ti = NULL, *tf;
  int        offset = 0;
  gchar      flags[64] = "<None>";
  gchar     *fstr[] = {"FIN", "SYN", "RST", "PSH", "ACK", "URG", "ECN", "CWR" };
  gint       fpos = 0, i;
  guint      bpos;
  guint      optlen;
  guint32    nxtseq;
  guint      len;
  guint      reported_len;
  vec_t      cksum_vec[4];
  guint32    phdr[2];
  guint16    computed_cksum;
  guint      length_remaining;
  gboolean   desegment_ok;
  struct tcpinfo tcpinfo;
  gboolean   save_fragmented;
  static struct tcpheader tcphstruct[4], *tcph;
  static int tcph_count=0;

  tcph_count++;
  if(tcph_count>=4){
     tcph_count=0;
  }
  tcph=&tcphstruct[tcph_count];


  if (check_col(pinfo->cinfo, COL_PROTOCOL))
    col_set_str(pinfo->cinfo, COL_PROTOCOL, "TCP");

  /* Clear out the Info column. */
  if (check_col(pinfo->cinfo, COL_INFO))
    col_clear(pinfo->cinfo, COL_INFO);

  tcph->th_sport = tvb_get_ntohs(tvb, offset);
  tcph->th_dport = tvb_get_ntohs(tvb, offset + 2);
  if (check_col(pinfo->cinfo, COL_INFO)) {
    col_append_fstr(pinfo->cinfo, COL_INFO, "%s > %s",
      get_tcp_port(tcph->th_sport), get_tcp_port(tcph->th_dport));
  }
  if (tree) {
    if (tcp_summary_in_tree) {
	    ti = proto_tree_add_protocol_format(tree, proto_tcp, tvb, 0, -1,
		"Transmission Control Protocol, Src Port: %s (%u), Dst Port: %s (%u)",
		get_tcp_port(tcph->th_sport), tcph->th_sport,
		get_tcp_port(tcph->th_dport), tcph->th_dport);
    }
    else {
	    ti = proto_tree_add_item(tree, proto_tcp, tvb, 0, -1, FALSE);
    }
    tcp_tree = proto_item_add_subtree(ti, ett_tcp);
    proto_tree_add_uint_format(tcp_tree, hf_tcp_srcport, tvb, offset, 2, tcph->th_sport,
	"Source port: %s (%u)", get_tcp_port(tcph->th_sport), tcph->th_sport);
    proto_tree_add_uint_format(tcp_tree, hf_tcp_dstport, tvb, offset + 2, 2, tcph->th_dport,
	"Destination port: %s (%u)", get_tcp_port(tcph->th_dport), tcph->th_dport);
    proto_tree_add_uint_hidden(tcp_tree, hf_tcp_port, tvb, offset, 2, tcph->th_sport);
    proto_tree_add_uint_hidden(tcp_tree, hf_tcp_port, tvb, offset + 2, 2, tcph->th_dport);
  }

  /* Set the source and destination port numbers as soon as we get them,
     so that they're available to the "Follow TCP Stream" code even if
     we throw an exception dissecting the rest of the TCP header. */
  pinfo->ptype = PT_TCP;
  pinfo->srcport = tcph->th_sport;
  pinfo->destport = tcph->th_dport;

  tcph->th_seq = tvb_get_ntohl(tvb, offset + 4);
  tcph->th_ack = tvb_get_ntohl(tvb, offset + 8);
  th_off_x2 = tvb_get_guint8(tvb, offset + 12);
  tcph->th_flags = tvb_get_guint8(tvb, offset + 13);
  tcph->th_win = tvb_get_ntohs(tvb, offset + 14);
  tcph->th_hlen = hi_nibble(th_off_x2) * 4;  /* TCP header length, in bytes */

  reported_len = tvb_reported_length(tvb);
  len = tvb_length(tvb);

  /* Compute the length of data in this segment. */
  tcph->th_seglen = reported_len - tcph->th_hlen;

  if (tree) { /* Add the seglen as an invisible field */

    proto_tree_add_uint_hidden(ti, hf_tcp_len, tvb, offset, 4, tcph->th_seglen);

  }

  /* handle TCP seq# analysis parse all new segments we see */
  if(tcp_analyze_seq){
      if(!(pinfo->fd->flags.visited)){
          tcp_analyze_sequence_number(pinfo, tcph->th_seq, tcph->th_ack, tcph->th_seglen, tcph->th_flags, tcph->th_win);
      }
      if(tcp_relative_seq){
          tcp_get_relative_seq_ack(pinfo->fd->num, &(tcph->th_seq), &(tcph->th_ack));
      }
  }


  /* Compute the sequence number of next octet after this segment. */
  nxtseq = tcph->th_seq + tcph->th_seglen;

  if (check_col(pinfo->cinfo, COL_INFO) || tree) {
    for (i = 0; i < 8; i++) {
      bpos = 1 << i;
      if (tcph->th_flags & bpos) {
        if (fpos) {
          strcpy(&flags[fpos], ", ");
          fpos += 2;
        }
        strcpy(&flags[fpos], fstr[i]);
        fpos += 3;
      }
    }
    flags[fpos] = '\0';
  }

  if (check_col(pinfo->cinfo, COL_INFO)) {
    col_append_fstr(pinfo->cinfo, COL_INFO, " [%s] Seq=%u Ack=%u Win=%u",
      flags, tcph->th_seq, tcph->th_ack, tcph->th_win);
  }

  if (tree) {
    if (tcp_summary_in_tree)
      proto_item_append_text(ti, ", Seq: %u", tcph->th_seq);
    proto_tree_add_uint(tcp_tree, hf_tcp_seq, tvb, offset + 4, 4, tcph->th_seq);
  }

  if (tcph->th_hlen < TCPH_MIN_LEN) {
    /* Give up at this point; we put the source and destination port in
       the tree, before fetching the header length, so that they'll
       show up if this is in the failing packet in an ICMP error packet,
       but it's now time to give up if the header length is bogus. */
    if (check_col(pinfo->cinfo, COL_INFO))
      col_append_fstr(pinfo->cinfo, COL_INFO, ", bogus TCP header length (%u, must be at least %u)",
        tcph->th_hlen, TCPH_MIN_LEN);
    if (tree) {
      proto_tree_add_uint_format(tcp_tree, hf_tcp_hdr_len, tvb, offset + 12, 1, tcph->th_hlen,
       "Header length: %u bytes (bogus, must be at least %u)", tcph->th_hlen,
       TCPH_MIN_LEN);
    }
    return;
  }

  if (tree) {
    if (tcp_summary_in_tree)
      proto_item_append_text(ti, ", Ack: %u, Len: %u", tcph->th_ack, tcph->th_seglen);
    proto_item_set_len(ti, tcph->th_hlen);
    if (nxtseq != tcph->th_seq)
      proto_tree_add_uint(tcp_tree, hf_tcp_nxtseq, tvb, offset, 0, nxtseq);
    if (tcph->th_flags & TH_ACK)
      proto_tree_add_uint(tcp_tree, hf_tcp_ack, tvb, offset + 8, 4, tcph->th_ack);
    proto_tree_add_uint_format(tcp_tree, hf_tcp_hdr_len, tvb, offset + 12, 1, tcph->th_hlen,
	"Header length: %u bytes", tcph->th_hlen);
    tf = proto_tree_add_uint_format(tcp_tree, hf_tcp_flags, tvb, offset + 13, 1,
	tcph->th_flags, "Flags: 0x%04x (%s)", tcph->th_flags, flags);
    field_tree = proto_item_add_subtree(tf, ett_tcp_flags);
    proto_tree_add_boolean(field_tree, hf_tcp_flags_cwr, tvb, offset + 13, 1, tcph->th_flags);
    proto_tree_add_boolean(field_tree, hf_tcp_flags_ecn, tvb, offset + 13, 1, tcph->th_flags);
    proto_tree_add_boolean(field_tree, hf_tcp_flags_urg, tvb, offset + 13, 1, tcph->th_flags);
    proto_tree_add_boolean(field_tree, hf_tcp_flags_ack, tvb, offset + 13, 1, tcph->th_flags);
    proto_tree_add_boolean(field_tree, hf_tcp_flags_push, tvb, offset + 13, 1, tcph->th_flags);
    proto_tree_add_boolean(field_tree, hf_tcp_flags_reset, tvb, offset + 13, 1, tcph->th_flags);
    proto_tree_add_boolean(field_tree, hf_tcp_flags_syn, tvb, offset + 13, 1, tcph->th_flags);
    proto_tree_add_boolean(field_tree, hf_tcp_flags_fin, tvb, offset + 13, 1, tcph->th_flags);
    proto_tree_add_uint(tcp_tree, hf_tcp_window_size, tvb, offset + 14, 2, tcph->th_win);
  }

  /* Supply the sequence number of the first byte. */
  tcpinfo.seq = tcph->th_seq;

  /* Assume we'll pass un-reassembled data to subdissectors. */
  tcpinfo.is_reassembled = FALSE;

  pinfo->private_data = &tcpinfo;

  /*
   * Assume, initially, that we can't desegment.
   */
  pinfo->can_desegment = 0;
  th_sum = tvb_get_ntohs(tvb, offset + 16);
  if (!pinfo->fragmented && len >= reported_len) {
    /* The packet isn't part of an un-reassembled fragmented datagram
       and isn't truncated.  This means we have all the data, and thus
       can checksum it and, unless it's being returned in an error
       packet, are willing to allow subdissectors to request reassembly
       on it. */

    if (tcp_check_checksum) {
      /* We haven't turned checksum checking off; checksum it. */

      /* Set up the fields of the pseudo-header. */
      cksum_vec[0].ptr = pinfo->src.data;
      cksum_vec[0].len = pinfo->src.len;
      cksum_vec[1].ptr = pinfo->dst.data;
      cksum_vec[1].len = pinfo->dst.len;
      cksum_vec[2].ptr = (const guint8 *)&phdr;
      switch (pinfo->src.type) {

      case AT_IPv4:
        phdr[0] = g_htonl((IP_PROTO_TCP<<16) + reported_len);
        cksum_vec[2].len = 4;
        break;

      case AT_IPv6:
        phdr[0] = g_htonl(reported_len);
        phdr[1] = g_htonl(IP_PROTO_TCP);
        cksum_vec[2].len = 8;
        break;

      default:
        /* TCP runs only atop IPv4 and IPv6.... */
        g_assert_not_reached();
        break;
      }
      cksum_vec[3].ptr = tvb_get_ptr(tvb, offset, len);
      cksum_vec[3].len = reported_len;
      computed_cksum = in_cksum(&cksum_vec[0], 4);
      if (computed_cksum == 0) {
        proto_tree_add_uint_format(tcp_tree, hf_tcp_checksum, tvb,
          offset + 16, 2, th_sum, "Checksum: 0x%04x (correct)", th_sum);

        /* Checksum is valid, so we're willing to desegment it. */
        desegment_ok = TRUE;
      } else {
        proto_tree_add_boolean_hidden(tcp_tree, hf_tcp_checksum_bad, tvb,
	   offset + 16, 2, TRUE);
        proto_tree_add_uint_format(tcp_tree, hf_tcp_checksum, tvb,
           offset + 16, 2, th_sum,
	   "Checksum: 0x%04x (incorrect, should be 0x%04x)", th_sum,
	   in_cksum_shouldbe(th_sum, computed_cksum));

        /* Checksum is invalid, so we're not willing to desegment it. */
        desegment_ok = FALSE;
        pinfo->noreassembly_reason = " (incorrect TCP checksum)";
      }
    } else {
      proto_tree_add_uint_format(tcp_tree, hf_tcp_checksum, tvb,
         offset + 16, 2, th_sum, "Checksum: 0x%04x", th_sum);

      /* We didn't check the checksum, and don't care if it's valid,
         so we're willing to desegment it. */
      desegment_ok = TRUE;
    }
  } else {
    /* We don't have all the packet data, so we can't checksum it... */
    proto_tree_add_uint_format(tcp_tree, hf_tcp_checksum, tvb,
       offset + 16, 2, th_sum, "Checksum: 0x%04x", th_sum);

    /* ...and aren't willing to desegment it. */
    desegment_ok = FALSE;
  }

  if (desegment_ok) {
    /* We're willing to desegment this.  Is desegmentation enabled? */
    if (tcp_desegment) {
      /* Yes - is this segment being returned in an error packet? */
      if (!pinfo->in_error_pkt) {
	/* No - indicate that we will desegment.
	   We do NOT want to desegment segments returned in error
	   packets, as they're not part of a TCP connection. */
	pinfo->can_desegment = 2;
      }
    }
  }

  if (tcph->th_flags & TH_URG) {
    th_urp = tvb_get_ntohs(tvb, offset + 18);
    /* Export the urgent pointer, for the benefit of protocols such as
       rlogin. */
    tcpinfo.urgent = TRUE;
    tcpinfo.urgent_pointer = th_urp;
    if (check_col(pinfo->cinfo, COL_INFO))
      col_append_fstr(pinfo->cinfo, COL_INFO, " Urg=%u", th_urp);
    if (tcp_tree != NULL)
      proto_tree_add_uint(tcp_tree, hf_tcp_urgent_pointer, tvb, offset + 18, 2, th_urp);
  } else
    tcpinfo.urgent = FALSE;

  if (check_col(pinfo->cinfo, COL_INFO))
    col_append_fstr(pinfo->cinfo, COL_INFO, " Len=%u", tcph->th_seglen);

  /* Decode TCP options, if any. */
  if (tree && tcph->th_hlen > TCPH_MIN_LEN) {
    /* There's more than just the fixed-length header.  Decode the
       options. */
    optlen = tcph->th_hlen - TCPH_MIN_LEN; /* length of options, in bytes */
    tf = proto_tree_add_text(tcp_tree, tvb, offset +  20, optlen,
      "Options: (%u bytes)", optlen);
    field_tree = proto_item_add_subtree(tf, ett_tcp_options);
    dissect_ip_tcp_options(tvb, offset + 20, optlen,
      tcpopts, N_TCP_OPTS, TCPOPT_EOL, pinfo, field_tree);
  }

  /* Skip over header + options */
  offset += tcph->th_hlen;

  /* Check the packet length to see if there's more data
     (it could be an ACK-only packet) */
  length_remaining = tvb_length_remaining(tvb, offset);

  if( data_out_file ) {
    reassemble_tcp( tcph->th_seq,		/* sequence number */
        tcph->th_seglen,			/* data length */
        tvb_get_ptr(tvb, offset, length_remaining),	/* data */
        length_remaining,		/* captured data length */
        ( tcph->th_flags & TH_SYN ),		/* is syn set? */
        &pinfo->net_src,
	&pinfo->net_dst,
	pinfo->srcport,
	pinfo->destport);
  }

  if (length_remaining != 0) {
    if (tcph->th_flags & TH_RST) {
      /*
       * RFC1122 says:
       *
       *	4.2.2.12  RST Segment: RFC-793 Section 3.4
       *
       *	  A TCP SHOULD allow a received RST segment to include data.
       *
       *	  DISCUSSION
       * 	       It has been suggested that a RST segment could contain
       * 	       ASCII text that encoded and explained the cause of the
       *	       RST.  No standard has yet been established for such
       *	       data.
       *
       * so for segments with RST we just display the data as text.
       */
      proto_tree_add_text(tcp_tree, tvb, offset, length_remaining,
			    "Reset cause: %s",
			    tvb_format_text(tvb, offset, length_remaining));
    } else {
      /* Can we desegment this segment? */
      if (pinfo->can_desegment) {
        /* Yes. */
        desegment_tcp(tvb, pinfo, offset, tcph->th_seq, nxtseq, tcph->th_sport, tcph->th_dport, tree, tcp_tree);
      } else {
        /* No - just call the subdissector.
           Mark this as fragmented, so if somebody throws an exception,
           we don't report it as a malformed frame. */
        save_fragmented = pinfo->fragmented;
        pinfo->fragmented = TRUE;
        decode_tcp_ports(tvb, offset, pinfo, tree, tcph->th_sport, tcph->th_dport);
        pinfo->fragmented = save_fragmented;
      }
    }
  }

  /* handle TCP seq# analysis, print any extra SEQ/ACK data for this segment*/
  if(tcp_analyze_seq){
      tcp_print_sequence_number_analysis(pinfo, tvb, tcp_tree);
  }
  tap_queue_packet(tcp_tap, pinfo, tcph);
}

void
proto_register_tcp(void)
{
	static hf_register_info hf[] = {

		{ &hf_tcp_srcport,
		{ "Source Port",		"tcp.srcport", FT_UINT16, BASE_DEC, NULL, 0x0,
			"", HFILL }},

		{ &hf_tcp_dstport,
		{ "Destination Port",		"tcp.dstport", FT_UINT16, BASE_DEC, NULL, 0x0,
			"", HFILL }},

		{ &hf_tcp_port,
		{ "Source or Destination Port",	"tcp.port", FT_UINT16, BASE_DEC, NULL, 0x0,
			"", HFILL }},

		{ &hf_tcp_seq,
		{ "Sequence number",		"tcp.seq", FT_UINT32, BASE_DEC, NULL, 0x0,
			"", HFILL }},

		{ &hf_tcp_nxtseq,
		{ "Next sequence number",	"tcp.nxtseq", FT_UINT32, BASE_DEC, NULL, 0x0,
			"", HFILL }},

		{ &hf_tcp_ack,
		{ "Acknowledgement number",	"tcp.ack", FT_UINT32, BASE_DEC, NULL, 0x0,
			"", HFILL }},

		{ &hf_tcp_hdr_len,
		{ "Header Length",		"tcp.hdr_len", FT_UINT8, BASE_DEC, NULL, 0x0,
			"", HFILL }},

		{ &hf_tcp_flags,
		{ "Flags",			"tcp.flags", FT_UINT8, BASE_HEX, NULL, 0x0,
			"", HFILL }},

		{ &hf_tcp_flags_cwr,
		{ "Congestion Window Reduced (CWR)",			"tcp.flags.cwr", FT_BOOLEAN, 8, TFS(&flags_set_truth), TH_CWR,
			"", HFILL }},

		{ &hf_tcp_flags_ecn,
		{ "ECN-Echo",			"tcp.flags.ecn", FT_BOOLEAN, 8, TFS(&flags_set_truth), TH_ECN,
			"", HFILL }},

		{ &hf_tcp_flags_urg,
		{ "Urgent",			"tcp.flags.urg", FT_BOOLEAN, 8, TFS(&flags_set_truth), TH_URG,
			"", HFILL }},

		{ &hf_tcp_flags_ack,
		{ "Acknowledgment",		"tcp.flags.ack", FT_BOOLEAN, 8, TFS(&flags_set_truth), TH_ACK,
			"", HFILL }},

		{ &hf_tcp_flags_push,
		{ "Push",			"tcp.flags.push", FT_BOOLEAN, 8, TFS(&flags_set_truth), TH_PUSH,
			"", HFILL }},

		{ &hf_tcp_flags_reset,
		{ "Reset",			"tcp.flags.reset", FT_BOOLEAN, 8, TFS(&flags_set_truth), TH_RST,
			"", HFILL }},

		{ &hf_tcp_flags_syn,
		{ "Syn",			"tcp.flags.syn", FT_BOOLEAN, 8, TFS(&flags_set_truth), TH_SYN,
			"", HFILL }},

		{ &hf_tcp_flags_fin,
		{ "Fin",			"tcp.flags.fin", FT_BOOLEAN, 8, TFS(&flags_set_truth), TH_FIN,
			"", HFILL }},

		{ &hf_tcp_window_size,
		{ "Window size",		"tcp.window_size", FT_UINT16, BASE_DEC, NULL, 0x0,
			"", HFILL }},

		{ &hf_tcp_checksum,
		{ "Checksum",			"tcp.checksum", FT_UINT16, BASE_HEX, NULL, 0x0,
			"", HFILL }},

		{ &hf_tcp_checksum_bad,
		{ "Bad Checksum",		"tcp.checksum_bad", FT_BOOLEAN, BASE_NONE, NULL, 0x0,
			"", HFILL }},

		{ &hf_tcp_analysis_flags,
		{ "TCP Analysis Flags",		"tcp.analysis.flags", FT_NONE, BASE_NONE, NULL, 0x0,
			"This frame has some of the TCP analysis flags set", HFILL }},

		{ &hf_tcp_analysis_retransmission,
		{ "Retransmission",		"tcp.analysis.retransmission", FT_NONE, BASE_NONE, NULL, 0x0,
			"This frame is a suspected TCP retransmission", HFILL }},

		{ &hf_tcp_analysis_lost_packet,
		{ "Previous Segment Lost",		"tcp.analysis.lost_segment", FT_NONE, BASE_NONE, NULL, 0x0,
			"A segment before this one was lost from the capture", HFILL }},

		{ &hf_tcp_analysis_ack_lost_packet,
		{ "ACKed Lost Packet",		"tcp.analysis.ack_lost_segment", FT_NONE, BASE_NONE, NULL, 0x0,
			"This frame ACKs a lost segment", HFILL }},

		{ &hf_tcp_analysis_keep_alive,
		{ "Keep Alive",		"tcp.analysis.keep_alive", FT_NONE, BASE_NONE, NULL, 0x0,
			"This is a keep-alive segment", HFILL }},

		{ &hf_tcp_analysis_duplicate_ack,
		{ "Duplicate ACK",		"tcp.analysis.duplicate_ack", FT_NONE, BASE_NONE, NULL, 0x0,
			"This is a duplicate ACK", HFILL }},

		{ &hf_tcp_analysis_zero_window_violation,
		{ "Zero Window Violation",		"tcp.analysis.zero_window_violation", FT_NONE, BASE_NONE, NULL, 0x0,
			"This is a zero-window violation, an attempt to write >1 byte to a zero-window", HFILL }},

		{ &hf_tcp_analysis_zero_window_probe,
		{ "Zero Window Probe",		"tcp.analysis.zero_window_probe", FT_NONE, BASE_NONE, NULL, 0x0,
			"This is a zero-window-probe", HFILL }},

		{ &hf_tcp_analysis_zero_window,
		{ "Zero Window",		"tcp.analysis.zero_window", FT_NONE, BASE_NONE, NULL, 0x0,
			"This is a Zero-Window", HFILL }},

		{ &hf_tcp_len,
		  { "TCP Segment Len",            "tcp.len", FT_UINT32, BASE_DEC, NULL, 0x0,
		    "", HFILL}},

		{ &hf_tcp_analysis_acks_frame,
		  { "This is an ACK to the segment in frame",            "tcp.analysis.acks_frame", FT_UINT32, BASE_DEC, NULL, 0x0,
		    "Which previous segment is this an ACK for", HFILL}},

		{ &hf_tcp_analysis_ack_rtt,
		  { "The RTT to ACK the segment was",            "tcp.analysis.ack_rtt", FT_RELATIVE_TIME, BASE_NONE, NULL, 0x0,
		    "How long time it took to ACK the segment (RTT)", HFILL}},

		{ &hf_tcp_urgent_pointer,
		{ "Urgent pointer",		"tcp.urgent_pointer", FT_UINT16, BASE_DEC, NULL, 0x0,
			"", HFILL }},

		{ &hf_tcp_segment_overlap,
		{ "Segment overlap",	"tcp.segment.overlap", FT_BOOLEAN, BASE_NONE, NULL, 0x0,
			"Segment overlaps with other segments", HFILL }},

		{ &hf_tcp_segment_overlap_conflict,
		{ "Conflicting data in segment overlap",	"tcp.segment.overlap.conflict", FT_BOOLEAN, BASE_NONE, NULL, 0x0,
			"Overlapping segments contained conflicting data", HFILL }},

		{ &hf_tcp_segment_multiple_tails,
		{ "Multiple tail segments found",	"tcp.segment.multipletails", FT_BOOLEAN, BASE_NONE, NULL, 0x0,
			"Several tails were found when desegmenting the pdu", HFILL }},

		{ &hf_tcp_segment_too_long_fragment,
		{ "Segment too long",	"tcp.segment.toolongfragment", FT_BOOLEAN, BASE_NONE, NULL, 0x0,
			"Segment contained data past end of the pdu", HFILL }},

		{ &hf_tcp_segment_error,
		{ "Desegmentation error", "tcp.segment.error", FT_FRAMENUM, BASE_NONE, NULL, 0x0,
			"Desegmentation error due to illegal segments", HFILL }},

		{ &hf_tcp_segment,
		{ "TCP Segment", "tcp.segment", FT_FRAMENUM, BASE_NONE, NULL, 0x0,
			"TCP Segment", HFILL }},

		{ &hf_tcp_segments,
		{ "TCP Segments", "tcp.segments", FT_NONE, BASE_NONE, NULL, 0x0,
			"TCP Segments", HFILL }},
		{ &hf_tcp_option_mss,
		  { "TCP MSS Option", "tcp.options.mss", FT_BOOLEAN, 
		    BASE_NONE, NULL, 0x0, "TCP MSS Option", HFILL }},
		{ &hf_tcp_option_mss_val,
		  { "TCP MSS Option Value", "tcp.options.mss_val", FT_UINT16,
		    BASE_DEC, NULL, 0x0, "TCP MSS Option Value", HFILL}},
		{ &hf_tcp_option_wscale,
		  { "TCP Window Scale Option", "tcp.options.wscale", 
		    FT_BOOLEAN, 
		    BASE_NONE, NULL, 0x0, "TCP Window Option", HFILL}},
		{ &hf_tcp_option_wscale_val,
		  { "TCP Windows Scale Option Value", "tcp.options.wscale_val",
		    FT_UINT8, BASE_DEC, NULL, 0x0, "TCP Window Scale Value",
		    HFILL}},
		{ &hf_tcp_option_sack_perm, 
		  { "TCP Sack Perm Option", "tcp.options.sack_perm", 
		    FT_BOOLEAN,
		    BASE_NONE, NULL, 0x0, "TCP Sack Perm Option", HFILL}},
		{ &hf_tcp_option_sack,
		  { "TCP Sack Option", "tcp.options.sack", FT_BOOLEAN, 
		    BASE_NONE, NULL, 0x0, "TCP Sack Option", HFILL}},
		{ &hf_tcp_option_sack_sle,
		  {"TCP Sack Left Edge", "tcp.options.sack_le", FT_UINT32,
		   BASE_DEC, NULL, 0x0, "TCP Sack Left Edge", HFILL}},
		{ &hf_tcp_option_sack_sre,
		  {"TCP Sack Right Edge", "tcp.options.sack_re", FT_UINT32,
		   BASE_DEC, NULL, 0x0, "TCP Sack Right Edge", HFILL}},
		{ &hf_tcp_option_echo,
		  { "TCP Echo Option", "tcp.options.echo", FT_BOOLEAN, 
		    BASE_NONE, NULL, 0x0, "TCP Sack Echo", HFILL}},
		{ &hf_tcp_option_echo_reply,
		  { "TCP Echo Reply Option", "tcp.options.echo_reply", 
		    FT_BOOLEAN,
		    BASE_NONE, NULL, 0x0, "TCP Echo Reply Option", HFILL}},
		{ &hf_tcp_option_time_stamp,
		  { "TCP Time Stamp Option", "tcp.options.time_stamp", 
		    FT_BOOLEAN,
		    BASE_NONE, NULL, 0x0, "TCP Time Stamp Option", HFILL}},
		{ &hf_tcp_option_cc,
		  { "TCP CC Option", "tcp.options.cc", FT_BOOLEAN, BASE_NONE,
		    NULL, 0x0, "TCP CC Option", HFILL}},
		{ &hf_tcp_option_ccnew,
		  { "TCP CC New Option", "tcp.options.ccnew", FT_BOOLEAN, 
		    BASE_NONE, NULL, 0x0, "TCP CC New Option", HFILL}},
		{ &hf_tcp_option_ccecho,
		  { "TCP CC Echo Option", "tcp.options.ccecho", FT_BOOLEAN,
		    BASE_NONE, NULL, 0x0, "TCP CC Echo Option", HFILL}},
		{ &hf_tcp_option_md5,
		  { "TCP MD5 Option", "tcp.options.md5", FT_BOOLEAN, BASE_NONE,
		    NULL, 0x0, "TCP MD5 Option", HFILL}},
	};
	static gint *ett[] = {
		&ett_tcp,
		&ett_tcp_flags,
		&ett_tcp_options,
		&ett_tcp_option_sack,
		&ett_tcp_analysis_faults,
		&ett_tcp_analysis,
		&ett_tcp_segments,
		&ett_tcp_segment
	};
	module_t *tcp_module;

	proto_tcp = proto_register_protocol("Transmission Control Protocol",
	    "TCP", "tcp");
	proto_register_field_array(proto_tcp, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));

	/* subdissector code */
	subdissector_table = register_dissector_table("tcp.port",
	    "TCP port", FT_UINT16, BASE_DEC);
	register_heur_dissector_list("tcp", &heur_subdissector_list);

	/* Register configuration preferences */
	tcp_module = prefs_register_protocol(proto_tcp, NULL);
	prefs_register_bool_preference(tcp_module, "summary_in_tree",
	    "Show TCP summary in protocol tree",
"Whether the TCP summary line should be shown in the protocol tree",
	    &tcp_summary_in_tree);
	prefs_register_bool_preference(tcp_module, "check_checksum",
	    "Check the validity of the TCP checksum when possible",
"Whether to check the validity of the TCP checksum",
	    &tcp_check_checksum);
	prefs_register_bool_preference(tcp_module, "desegment_tcp_streams",
	    "Allow subdissector to desegment TCP streams",
"Whether subdissector can request TCP streams to be desegmented",
	    &tcp_desegment);
	prefs_register_bool_preference(tcp_module, "analyze_sequence_numbers",
	    "Analyze TCP sequence numbers",
	    "Make the TCP dissector analyze TCP sequence numbers to find and flag segment retransmissions, missing segments and RTT",
	    &tcp_analyze_seq);
	prefs_register_bool_preference(tcp_module, "relative_sequence_numbers",
	    "Use relative sequence numbers",
	    "Make the TCP dissector use relative sequence numbers instead of absolute ones. To use this option you must also enable \"Analyze TCP sequence numbers\".",
	    &tcp_relative_seq);

	register_init_routine(tcp_analyze_seq_init);
	register_init_routine(tcp_desegment_init);
	register_init_routine(tcp_fragment_init);
}

void
proto_reg_handoff_tcp(void)
{
	dissector_handle_t tcp_handle;

	tcp_handle = create_dissector_handle(dissect_tcp, proto_tcp);
	dissector_add("ip.proto", IP_PROTO_TCP, tcp_handle);
	data_handle = find_dissector("data");
	tcp_tap = register_tap("tcp");
}
