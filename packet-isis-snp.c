/* packet-isis-snp.c
 * Routines for decoding isis complete & partial SNP and their payload
 *
 * $Id: packet-isis-snp.c,v 1.21 2003/03/31 07:44:09 guy Exp $
 * Stuart Stanley <stuarts@mxmail.net>
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
#include "packet-osi.h"
#include "packet-isis.h"
#include "packet-isis-clv.h"
#include "packet-isis-lsp.h"
#include "packet-isis-snp.h"

/* csnp packets */
static int hf_isis_csnp_pdu_length = -1;
static gint ett_isis_csnp = -1;
static gint ett_isis_csnp_lsp_entries = -1;
static gint ett_isis_csnp_lsp_entry = -1;
static gint ett_isis_csnp_authentication = -1;
static gint ett_isis_csnp_clv_unknown = -1;

/* psnp packets */
static int hf_isis_psnp_pdu_length = -1;
static gint ett_isis_psnp = -1;
static gint ett_isis_psnp_lsp_entries = -1;
static gint ett_isis_psnp_lsp_entry = -1;
static gint ett_isis_psnp_authentication = -1;
static gint ett_isis_psnp_clv_unknown = -1;

static void dissect_l1_snp_authentication_clv(tvbuff_t *tvb,
	proto_tree *tree, int offset, int id_length, int length);
static void dissect_l2_snp_authentication_clv(tvbuff_t *tvb,
	proto_tree *tree, int offset, int id_length, int length);
static void dissect_csnp_lsp_entries(tvbuff_t *tvb,
	proto_tree *tree, int offset, int id_length, int length);
static void dissect_psnp_lsp_entries(tvbuff_t *tvb,
	proto_tree *tree, int offset, int id_length, int length);

static const isis_clv_handle_t clv_l1_csnp_opts[] = {
	{
		ISIS_CLV_L1_CSNP_LSP_ENTRIES,
		"LSP entries",
		&ett_isis_csnp_lsp_entries,
		dissect_csnp_lsp_entries
	},
	{
		ISIS_CLV_L1_CSNP_AUTHENTICATION_NS,
		"Authentication(non spec)",
		&ett_isis_csnp_authentication,
		dissect_l1_snp_authentication_clv
	},
	{
		ISIS_CLV_L1_CSNP_AUTHENTICATION,
		"Authentication",
		&ett_isis_csnp_authentication,
		dissect_l1_snp_authentication_clv
	},
	{
		0, "", NULL, NULL
	}
};

static const isis_clv_handle_t clv_l2_csnp_opts[] = {
	{
		ISIS_CLV_L2_CSNP_LSP_ENTRIES,
		"LSP entries",
		&ett_isis_csnp_lsp_entries,
		dissect_csnp_lsp_entries
	},
	{
		ISIS_CLV_L2_CSNP_AUTHENTICATION_NS,
		"Authentication(non spec)",
		&ett_isis_csnp_authentication,
		dissect_l2_snp_authentication_clv
	},
	{
		ISIS_CLV_L2_CSNP_AUTHENTICATION,
		"Authentication",
		&ett_isis_csnp_authentication,
		dissect_l2_snp_authentication_clv
	},
	{
		0, "", NULL, NULL
	}
};

static const isis_clv_handle_t clv_l1_psnp_opts[] = {
	{
		ISIS_CLV_L1_PSNP_LSP_ENTRIES,
		"LSP entries",
		&ett_isis_psnp_lsp_entries,
		dissect_psnp_lsp_entries
	},
	{
		ISIS_CLV_L1_PSNP_AUTHENTICATION_NS,
		"Authentication(non spec)",
		&ett_isis_psnp_authentication,
		dissect_l1_snp_authentication_clv
	},
	{
		ISIS_CLV_L1_PSNP_AUTHENTICATION,
		"Authentication",
		&ett_isis_psnp_authentication,
		dissect_l1_snp_authentication_clv
	},
	{
		0, "", NULL, NULL
	}
};

static const isis_clv_handle_t clv_l2_psnp_opts[] = {
	{
		ISIS_CLV_L2_PSNP_LSP_ENTRIES,
		"LSP entries",
		&ett_isis_psnp_lsp_entries,
		dissect_psnp_lsp_entries
	},
	{
		ISIS_CLV_L2_PSNP_AUTHENTICATION,
		"Authentication",
		&ett_isis_psnp_authentication,
		dissect_l2_snp_authentication_clv
	},
	{
		ISIS_CLV_L2_PSNP_AUTHENTICATION_NS,
		"Authentication(non spec)",
		&ett_isis_psnp_authentication,
		dissect_l2_snp_authentication_clv
	},
	{
		0, "", NULL, NULL
	}
};

/*
 * Name: dissect_snp_lsp_entries()
 *
 * Description:
 *	All the snp packets use a common payload format.  We have up
 *	to n entries (based on length), which are made of:
 *		2         : remaining life time
 *		id_length : lsp id
 *		4         : sequence number
 *		2         : checksum
 *
 * Input:
 *	tvbuff_t * : tvbuffer for packet data
 *	proto_tree * : protocol display tree to fill out.  May be NULL
 *	int : offset into packet data where we are.
 *	int : length of IDs in packet.
 *	int : length of payload to decode.
 *
 * Output:
 *      void, but we will add to proto tree if !NULL.
 */
static void
dissect_csnp_lsp_entries(tvbuff_t *tvb, proto_tree *tree, int offset,
	int id_length, int length)
{
        proto_tree *subtree,*ti;

	while ( length > 0 ) {
		if ( length < 2+id_length+2+4+2 ) {
			isis_dissect_unknown(tvb, tree, offset,
				"Short CSNP header entry (%d vs %d)", length,
				2+id_length+2+4+2 );
			return;
		}

	        ti = proto_tree_add_text(tree, tvb, offset, 2+id_length+2+4+2,
                                    "LSP-ID: %s, Sequence: 0x%08x, Lifetime: %5us, Checksum: 0x%04x",
                                           print_system_id( tvb_get_ptr(tvb, offset+2, id_length+2), id_length+2 ),
                                           tvb_get_ntohl(tvb, offset+2+id_length+2),
                                           tvb_get_ntohs(tvb, offset),
                                           tvb_get_ntohs(tvb, offset+2+id_length+2+4));

                subtree = proto_item_add_subtree(ti,ett_isis_csnp_lsp_entry);

		proto_tree_add_text(subtree, tvb, offset+2, 8,
			"LSP-ID:             : %s",
			print_system_id( tvb_get_ptr(tvb, offset+2, id_length+2), id_length+2 ));

		proto_tree_add_text(subtree, tvb, offset+2+id_length+2, 4,
			"LSP Sequence Number : 0x%08x",
			tvb_get_ntohl(tvb, offset+2+id_length+2));

		proto_tree_add_text(subtree, tvb, offset, 2,
			"Remaining Lifetime  : %us",
			tvb_get_ntohs(tvb, offset));

		proto_tree_add_text(subtree, tvb, offset+2+id_length+2+4, 2,
			"LSP checksum        : 0x%04x",
			tvb_get_ntohs(tvb, offset+2+id_length+2+4));

		length -= 2+id_length+2+4+2;
		offset += 2+id_length+2+4+2;
	}

}

static void
dissect_psnp_lsp_entries(tvbuff_t *tvb, proto_tree *tree, int offset,
	int id_length, int length)
{
        proto_tree *subtree,*ti;

	while ( length > 0 ) {
		if ( length < 2+id_length+2+4+2 ) {
			isis_dissect_unknown(tvb, tree, offset,
				"Short PSNP header entry (%d vs %d)", length,
				2+id_length+2+4+2 );
			return;
		}

	        ti = proto_tree_add_text(tree, tvb, offset, 2+id_length+2+4+2,
                                    "LSP-ID: %s, Sequence: 0x%08x, Lifetime: %5us, Checksum: 0x%04x",
                                           print_system_id( tvb_get_ptr(tvb, offset+2, id_length+2), id_length+2 ),
                                           tvb_get_ntohl(tvb, offset+2+id_length+2),
                                           tvb_get_ntohs(tvb, offset),
                                           tvb_get_ntohs(tvb, offset+2+id_length+2+4));

                subtree = proto_item_add_subtree(ti,ett_isis_psnp_lsp_entry);

		proto_tree_add_text(subtree, tvb, offset+2, 8,
			"LSP-ID:             : %s",
			print_system_id( tvb_get_ptr(tvb, offset+2, id_length+2), id_length+2 ));

		proto_tree_add_text(subtree, tvb, offset+2+id_length+2, 4,
			"LSP Sequence Number : 0x%08x",
			tvb_get_ntohl(tvb, offset+2+id_length+2));

		proto_tree_add_text(subtree, tvb, offset, 2,
			"Remaining Lifetime  : %us",
			tvb_get_ntohs(tvb, offset));

		proto_tree_add_text(subtree, tvb, offset+2+id_length+2+4, 2,
			"LSP checksum        : 0x%04x",
			tvb_get_ntohs(tvb, offset+2+id_length+2+4));

		length -= 2+id_length+2+4+2;
		offset += 2+id_length+2+4+2;
	}

}

/*
 * Name: isis_dissect_isis_csnp()
 *
 * Description:
 *	Tear apart a L1 or L2 CSNP header and then call into payload dissect
 *	to pull apart the lsp id payload.
 *
 * Input:
 *	tvbuff_t * : tvbuffer for packet data
 *	proto_tree * : protocol display tree to add to.  May be NULL.
 *	int offset : our offset into packet data.
 *	int : type (l1 csnp, l2 csnp)
 *	int : header length of packet.
 *	int : length of IDs in packet.
 *
 * Output:
 *      void, but we will add to proto tree if !NULL.
 */
void
isis_dissect_isis_csnp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset,
	int type, int header_length, int id_length)
{
	proto_item	*ti;
	proto_tree	*csnp_tree = NULL;
	guint16		pdu_length;
	int 		len;

	if (tree) {
		ti = proto_tree_add_text(tree, tvb, offset, -1,
		    PROTO_STRING_CSNP);
		csnp_tree = proto_item_add_subtree(ti, ett_isis_csnp);
	}

	pdu_length = tvb_get_ntohs(tvb, offset);
	if (tree) {
		proto_tree_add_uint(csnp_tree, hf_isis_csnp_pdu_length, tvb,
			offset, 2, pdu_length);
	}
	offset += 2;

	if (tree) {
		proto_tree_add_text(csnp_tree, tvb, offset, id_length + 1,
			"Source-ID:    %s",
				print_system_id( tvb_get_ptr(tvb, offset, id_length+1), id_length+1 ) );
	}
	if (check_col(pinfo->cinfo, COL_INFO)) {
		col_append_fstr(pinfo->cinfo, COL_INFO, ", Source-ID: %s",
			print_system_id( tvb_get_ptr(tvb, offset, id_length+1), id_length+1 ) );
	}
	offset += id_length + 1;

	if (tree) {
		proto_tree_add_text(csnp_tree, tvb, offset, id_length + 2,
			"Start LSP-ID: %s",
                                    print_system_id( tvb_get_ptr(tvb, offset, id_length+2), id_length+2 ) );                
	}
	if (check_col(pinfo->cinfo, COL_INFO)) {
		col_append_fstr(pinfo->cinfo, COL_INFO, ", Start LSP-ID: %s",
			print_system_id( tvb_get_ptr(tvb, offset, id_length+2), id_length+2 ) );
	}
	offset += id_length + 2;

	if (tree) {
		proto_tree_add_text(csnp_tree, tvb, offset, id_length + 2,
			"End LSP-ID: %s",
                                    print_system_id( tvb_get_ptr(tvb, offset, id_length+2), id_length+2 ) );  
	}
	if (check_col(pinfo->cinfo, COL_INFO)) {
		col_append_fstr(pinfo->cinfo, COL_INFO, ", End LSP-ID: %s",
			print_system_id( tvb_get_ptr(tvb, offset, id_length+2), id_length+2 ) );
	}
	offset += id_length + 2;

	len = pdu_length - header_length;
	if (len < 0) {
		return;
	}
	/* Call into payload dissector */
	if (type == ISIS_TYPE_L1_CSNP ) {
		isis_dissect_clvs(tvb, csnp_tree, offset,
			clv_l1_csnp_opts, len, id_length,
			ett_isis_csnp_clv_unknown );
	} else {
		isis_dissect_clvs(tvb, csnp_tree, offset,
			clv_l2_csnp_opts, len, id_length,
			ett_isis_csnp_clv_unknown );
	}
}

/*
 * Name: isis_dissect_isis_psnp()
 *
 * Description:
 *	Tear apart a L1 or L2 PSNP header and then call into payload dissect
 *	to pull apart the lsp id payload.
 *
 * Input:
 *	tvbuff_t * : tvbuffer for packet data
 *	proto_tree * : protocol display tree to add to.  May be NULL.
 *	int : our offset into packet data
 *	int : type (l1 psnp, l2 psnp)
 *	int : header length of packet.
 *	int : length of IDs in packet.
 *
 * Output:
 *      void, but we will add to proto tree if !NULL.
 */
void
isis_dissect_isis_psnp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset,
	int type, int header_length, int id_length)
{
	proto_item	*ti;
	proto_tree	*psnp_tree = NULL;
	guint16		pdu_length;
	int 		len;

	if (tree) {
		ti = proto_tree_add_text(tree, tvb, offset, -1,
		    PROTO_STRING_PSNP);
		psnp_tree = proto_item_add_subtree(ti, ett_isis_psnp);
	}

	pdu_length = tvb_get_ntohs(tvb, offset);
	if (tree) {
		proto_tree_add_uint(psnp_tree, hf_isis_psnp_pdu_length, tvb,
			offset, 2, pdu_length);
	}
	offset += 2;

	if (tree) {
		proto_tree_add_text(psnp_tree, tvb, offset, id_length + 1,
			"Source-ID: %s",
			print_system_id( tvb_get_ptr(tvb, offset, id_length+1), id_length + 1 ) );
	}
	if (check_col(pinfo->cinfo, COL_INFO)) {
		col_append_fstr(pinfo->cinfo, COL_INFO, ", Source-ID: %s",
			print_system_id( tvb_get_ptr(tvb, offset, id_length+1), id_length+1 ) );
	}
	offset += id_length + 1;

	len = pdu_length - header_length;
	if (len < 0) {
		isis_dissect_unknown(tvb, tree, offset,
			"packet header length %d went beyond packet",
			header_length );
		return;
	}
	/* Call into payload dissector */
	if (type == ISIS_TYPE_L1_CSNP ) {
		isis_dissect_clvs(tvb, psnp_tree, offset,
			clv_l1_csnp_opts, len, id_length,
			ett_isis_psnp_clv_unknown );
	} else {
		isis_dissect_clvs(tvb, psnp_tree, offset,
			clv_l2_csnp_opts, len, id_length,
			ett_isis_psnp_clv_unknown );
	}
}

/*
 * Name: dissect_L1_snp_authentication_clv()
 *
 * Description:
 *	Decode for a lsp packets authenticaion clv.  Calls into the
 *	clv common one.  An auth inside a L1 SNP is a per area password
 *
 * Input:
 *	tvbuff_t * : tvbuffer for packet data
 *	proto_tree * : proto tree to build on (may be null)
 *	int : current offset into packet data
 *	int : length of IDs in packet.
 *	int : length of this clv
 *
 * Output:
 *	void, will modify proto_tree if not null.
 */
static void
dissect_l1_snp_authentication_clv(tvbuff_t *tvb, proto_tree *tree, int offset,
	int id_length _U_, int length)
{
	isis_dissect_authentication_clv(tvb, tree, offset, length,
		"Per area authentication" );
}

/*
 * Name: dissect_l2_authentication_clv()
 *
 * Description:
 *	Decode for a lsp packets authenticaion clv.  Calls into the
 *	clv common one.  An auth inside a L2 LSP is a per domain password
 *
 * Input:
 *	tvbuff_t * : tvbuffer for packet data
 *	proto_tree * : proto tree to build on (may be null)
 *	int : current offset into packet data
 *	int : length of IDs in packet.
 *	int : length of this clv
 *
 * Output:
 *	void, will modify proto_tree if not null.
 */
static void
dissect_l2_snp_authentication_clv(tvbuff_t *tvb, proto_tree *tree, int offset,
	int id_length _U_, int length)
{
	isis_dissect_authentication_clv(tvb, tree, offset, length,
		"Per domain authentication" );
}

/*
 * Name: isis_register_csnp()
 *
 * Description:
 *	Register our protocol sub-sets with protocol manager.
 *
 * Input:
 *	int : protocol index for the ISIS protocol
 *
 * Output:
 *	void
 */
void
isis_register_csnp(int proto_isis) {
	static hf_register_info hf[] = {
		{ &hf_isis_csnp_pdu_length,
		{ "PDU length",		"isis.csnp.pdu_length", FT_UINT16,
		  BASE_DEC, NULL, 0x0, "", HFILL }},
	};
	static gint *ett[] = {
		&ett_isis_csnp,
		&ett_isis_csnp_lsp_entries,
		&ett_isis_csnp_lsp_entry,
		&ett_isis_csnp_authentication,
		&ett_isis_csnp_clv_unknown,
	};

	proto_register_field_array(proto_isis, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));
}


/*
 * Name: isis_register_psnp()
 *
 * Description:
 *	Register our protocol sub-sets with protocol manager.
 *
 * Input:
 *	int : protocol index for the ISIS protocol
 *
 * Output:
 *	void
 */
void
isis_register_psnp(int proto_isis) {
	static hf_register_info hf[] = {
		{ &hf_isis_psnp_pdu_length,
		{ "PDU length",		"isis.psnp.pdu_length", FT_UINT16,
		  BASE_DEC, NULL, 0x0, "", HFILL }},
	};
	static gint *ett[] = {
		&ett_isis_psnp,
		&ett_isis_psnp_lsp_entries,
		&ett_isis_psnp_lsp_entry,
		&ett_isis_psnp_authentication,
		&ett_isis_psnp_clv_unknown,
	};

	proto_register_field_array(proto_isis, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));
}
