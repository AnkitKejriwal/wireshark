/* packet-fcfzs.c
 * Routines for FC Fabric Zone Server
 * Copyright 2001, Dinesh G Dutt <ddutt@andiamo.com>
 *
 * $Id$
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@ethereal.com>
 * Copyright 1998 Gerald Combs
 *
 * Copied from WHATEVER_FILE_YOU_USED (where "WHATEVER_FILE_YOU_USED"
 * is a dissector file; if you just copied this from README.developer,
 * don't bother with the "Copied from" - you don't even need to put
 * in a "Copied from" if you copied an existing dissector, especially
 * if the bulk of the code in the new dissector is your code)
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

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif

#include <glib.h>

#ifdef NEED_SNPRINTF_H
# include "snprintf.h"
#endif

#include <epan/packet.h>
#include <epan/conversation.h>
#include "etypes.h"
#include "packet-fc.h"
#include "packet-fcct.h"
#include "packet-fcfzs.h"

/* Initialize the protocol and registered fields */
static int proto_fcfzs              = -1;
static int hf_fcfzs_opcode          = -1;
static int hf_fcfzs_gzc_flags       = -1;
static int hf_fcfzs_gzc_vendor      = -1;
static int hf_fcfzs_zone_state      = -1;
static int hf_fcfzs_gest_vendor     = -1;
static int hf_fcfzs_numzoneattrs    = -1;
static int hf_fcfzs_zonesetnmlen    = -1;
static int hf_fcfzs_zonesetname     = -1;
static int hf_fcfzs_numzones        = -1;
static int hf_fcfzs_numzonesetattrs = -1;
static int hf_fcfzs_zonenmlen       = -1;
static int hf_fcfzs_zonename        = -1;
static int hf_fcfzs_nummbrs         = -1;
static int hf_fcfzs_nummbrentries   = -1;
static int hf_fcfzs_mbrid           = -1;
static int hf_fcfzs_mbridlen        = -1;
static int hf_fcfzs_mbrtype         = -1;
static int hf_fcfzs_reason          = -1;
static int hf_fcfzs_rjtdetail       = -1;
static int hf_fcfzs_rjtvendor       = -1;
static int hf_fcfzs_maxres_size     = -1;
static int hf_fcfzs_mbrid_lun       = -1;


/* Initialize the subtree pointers */
static gint ett_fcfzs = -1;

typedef struct _fcfzs_conv_key {
    guint32 conv_idx;
} fcfzs_conv_key_t;

typedef struct _fcfzs_conv_data {
    guint32 opcode;
} fcfzs_conv_data_t;

GHashTable *fcfzs_req_hash = NULL;
GMemChunk *fcfzs_req_keys = NULL;
GMemChunk *fcfzs_req_vals = NULL;
guint32 fcfzs_init_count = 25;

static dissector_handle_t data_handle;

/*
 * Hash Functions
 */
static gint
fcfzs_equal(gconstpointer v, gconstpointer w)
{
  const fcfzs_conv_key_t *v1 = v;
  const fcfzs_conv_key_t *v2 = w;

  return (v1->conv_idx == v2->conv_idx);
}

static guint
fcfzs_hash (gconstpointer v)
{
	const fcfzs_conv_key_t *key = v;
	guint val;

	val = key->conv_idx;

	return val;
}

/*
 * Protocol initialization
 */
static void
fcfzs_init_protocol(void)
{
	if (fcfzs_req_keys)
            g_mem_chunk_destroy (fcfzs_req_keys);
	if (fcfzs_req_vals)
            g_mem_chunk_destroy (fcfzs_req_vals);
	if (fcfzs_req_hash)
            g_hash_table_destroy (fcfzs_req_hash);

	fcfzs_req_hash = g_hash_table_new (fcfzs_hash, fcfzs_equal);
	fcfzs_req_keys = g_mem_chunk_new ("fcfzs_req_keys",
                                          sizeof(fcfzs_conv_key_t),
                                          fcfzs_init_count *
                                          sizeof(fcfzs_conv_key_t),
                                          G_ALLOC_AND_FREE);
	fcfzs_req_vals = g_mem_chunk_new ("fcfzs_req_vals",
                                          sizeof(fcfzs_conv_data_t),
                                          fcfzs_init_count *
                                          sizeof(fcfzs_conv_data_t),
                                          G_ALLOC_AND_FREE);
}

/* Code to actually dissect the packets */
static void
dissect_fcfzs_zoneset (tvbuff_t *tvb, proto_tree *tree, int offset)
{
    int numzones, nummbrs, i, j, len;
    
    /* The zoneset structure has the following format */
    /* zoneset name (len[not including pad], name, pad),
     * number of zones,
     * for each zone,
     *     Zone name (len[not including pad], name, pad), num zone mbrs
     *     for each zone mbr,
     *         zone mbr id type, zone mbr id (len, name, pad)
     */
    if (tree) {

        /* Zoneset Name */   
        len = tvb_get_guint8 (tvb, offset);
        proto_tree_add_item (tree, hf_fcfzs_zonesetnmlen, tvb, offset,
                                1, 0);
        proto_tree_add_item (tree, hf_fcfzs_zonesetname, tvb, offset+4,
                                len, 0);
        offset += 4 + len + (4-(len % 4));

        
        /* Number of zones */   
        numzones = tvb_get_ntohl (tvb, offset);
        proto_tree_add_item (tree, hf_fcfzs_numzones, tvb, offset, 4, 0);
        offset += 4;
        
        /* For each zone... */   
        for (i = 0; i < numzones; i++) {
            len = tvb_get_guint8 (tvb, offset);
            proto_tree_add_item (tree, hf_fcfzs_zonenmlen, tvb, offset,
                                 1, 0);
            proto_tree_add_item (tree, hf_fcfzs_zonename, tvb, offset+4,
                                 len, 0);
            offset += 4 + len + (4-(len % 4));

            nummbrs = tvb_get_ntohl (tvb, offset);
            proto_tree_add_item (tree, hf_fcfzs_nummbrentries, tvb, offset,
                                 4, 0);

            offset += 4;
            for (j = 0; j < nummbrs; j++) {
                proto_tree_add_item (tree, hf_fcfzs_mbrtype, tvb, offset, 1, 0);
                
                switch (tvb_get_guint8 (tvb, offset)) {
                case FC_FZS_ZONEMBR_PWWN:
                case FC_FZS_ZONEMBR_NWWN:
                    proto_tree_add_string (tree, hf_fcfzs_mbrid, tvb,
                                           offset+4, 8,
                                           fcwwn_to_str (tvb_get_ptr (tvb,
                                                                      offset+4,
                                                                      8)));
                    break;
                case FC_FZS_ZONEMBR_DP:
                    proto_tree_add_string_format (tree,
                                                  hf_fcfzs_mbrid,
                                                  tvb, offset+4, 3, " ",
                                                  "0x%x",
                                                  tvb_get_ntoh24 (tvb,
                                                                  offset+4));
                    break;
                case FC_FZS_ZONEMBR_FCID:
                    proto_tree_add_string (tree, hf_fcfzs_mbrid, tvb,
                                           offset+4, 4,
                                           fc_to_str (tvb_get_ptr (tvb,
                                                                   offset+4,
                                                                   3)));
                    break;
                case FC_FZS_ZONEMBR_PWWN_LUN:
                    proto_tree_add_string (tree, hf_fcfzs_mbrid, tvb,
                                           offset+4, 8,
                                           fcwwn_to_str (tvb_get_ptr (tvb,
                                                                      offset+4,
                                                                      8)));
                    proto_tree_add_item (tree, hf_fcfzs_mbrid_lun, tvb,
                                         offset+8, 8, 0);
                    break;
                case FC_FZS_ZONEMBR_DP_LUN:
                    proto_tree_add_string_format (tree,
                                                  hf_fcfzs_mbrid,
                                                  tvb, offset+4, 3, " ",
                                                  "0x%x",
                                                  tvb_get_ntoh24 (tvb,
                                                                  offset+4));
                    proto_tree_add_item (tree, hf_fcfzs_mbrid_lun, tvb,
                                         offset+4, 8, 0);
                    break;
                case FC_FZS_ZONEMBR_FCID_LUN:
                    proto_tree_add_string (tree, hf_fcfzs_mbrid, tvb,
                                           offset+4, 4,
                                           fc_to_str (tvb_get_ptr (tvb,
                                                                   offset+4,
                                                                   3)));
                    proto_tree_add_item (tree, hf_fcfzs_mbrid_lun, tvb,
                                         offset+4, 8, 0);
                    break;
                default:
                    proto_tree_add_string (tree, hf_fcfzs_mbrid, tvb,
                                           offset+4, 8,
                                           "Unknown member type format");
                }
                offset += 12;
            }
        }
    }
}

static void
dissect_fcfzs_gzc (tvbuff_t *tvb, proto_tree *tree, guint8 isreq)
{
    guint8 flags;
    gchar str[128];
    int offset = 16;            /* past the fc_ct header */
    int stroff = 0;
    
    if (tree) {
        if (!isreq) {
            flags = tvb_get_guint8 (tvb, offset);

            /* Disect the flags field */
            str[0] = '\0';
            if (flags & 0x80) {
                strcpy (str, "Hard Zones, ");
                stroff += 12;
            }

            if (flags & 0x40) {
                strcpy (&str[stroff], "Soft Zones Supported, ");
                stroff += 22;
            }

            if (flags & 0x01) {
                strcpy (&str[stroff], "ZoneSet Database Available");
                stroff += 26;
            }

            proto_tree_add_uint_format (tree, hf_fcfzs_gzc_flags, tvb, offset,
                                        1, flags, "Capabilities: 0x%x (%s)",
                                        flags, str);
            proto_tree_add_item (tree, hf_fcfzs_gzc_vendor, tvb, offset+4, 4, 0);
        }
    }
}

static void
dissect_fcfzs_gest (tvbuff_t *tvb, proto_tree *tree, guint8 isreq)
{
    guint8 flags;
    gchar str[128];
    int offset = 16;            /* past the fc_ct header */
    int stroff = 0;

    if (tree) {
        if (!isreq) {
            flags = tvb_get_guint8 (tvb, offset);
            str[0] = '\0';

            /* dissect the flags field */
            if (flags & 0x80) {
                strcpy (str, "Soft Zone Set Enforced, ");
                stroff += 24;
            }

            if (flags & 0x40) {
                 strcpy (str, "Hard Zone Set Enforced");
                stroff += 24;
            }

            proto_tree_add_uint_format (tree, hf_fcfzs_zone_state, tvb, offset,
                                        1, flags, "Zone State: 0x%x (%s)",
                                        flags, str);
            proto_tree_add_item (tree, hf_fcfzs_gest_vendor, tvb, offset+4, 4, 0);
        }
    }
}

static void
dissect_fcfzs_gzsn (tvbuff_t *tvb, proto_tree *tree, guint8 isreq)
{
    int numrec, i, len;
    int offset = 16;            /* past the fc_ct header */
    
    if (tree) {
        if (!isreq) {
            numrec = tvb_get_ntohl (tvb, offset);

            proto_tree_add_item (tree, hf_fcfzs_numzonesetattrs, tvb, offset,
                                 4, 0);

            offset += 4;
            for (i = 0; i < numrec; i++) {
                len = tvb_get_guint8 (tvb, offset);
                proto_tree_add_item (tree, hf_fcfzs_zonesetnmlen, tvb, offset,
                                     1, 0);
                proto_tree_add_item (tree, hf_fcfzs_zonesetname, tvb, offset+1,
                                     len, 0);
                offset += len + 1 + (len % 4);
                proto_tree_add_item (tree, hf_fcfzs_numzones, tvb, offset,
                                     4, 0);
                offset += 4;
            }
        }
    }
}

static void
dissect_fcfzs_gzd (tvbuff_t *tvb, proto_tree *tree, guint8 isreq)
{
    int numrec, i, len;
    int offset = 16;            /* past the fc_ct header */
    
    if (tree) {
        if (isreq) {
            len = tvb_get_guint8 (tvb, offset);
            proto_tree_add_item (tree, hf_fcfzs_zonesetnmlen, tvb, offset,
                                 1, 0);
            proto_tree_add_item (tree, hf_fcfzs_zonesetname, tvb, offset+1,
                                 len, 0);
        }
        else {
            numrec = tvb_get_ntohl (tvb, offset);

            proto_tree_add_item (tree, hf_fcfzs_numzoneattrs, tvb, offset,
                                 4, 0);
            
            offset += 4;
            for (i = 0; i < numrec; i++) {
                len = tvb_get_guint8 (tvb, offset);
                proto_tree_add_item (tree, hf_fcfzs_zonenmlen, tvb, offset,
                                     1, 0);
                proto_tree_add_item (tree, hf_fcfzs_zonename, tvb, offset+1,
                                     len, 0);
                offset += len + 1 + (len % 4);
                proto_tree_add_item (tree, hf_fcfzs_nummbrs, tvb, offset,
                                     4, 0);
                offset += 4;
            }
        }
    }
}

static void
dissect_fcfzs_gzm (tvbuff_t *tvb, proto_tree *tree, guint8 isreq)
{
    int numrec, i, len;
    int offset = 16;            /* past the fc_ct header */
 
    if (tree) {
        if (isreq) {
            len = tvb_get_guint8 (tvb, offset);
            proto_tree_add_item (tree, hf_fcfzs_zonenmlen, tvb, offset,
                                 1, 0);
            proto_tree_add_item (tree, hf_fcfzs_zonename, tvb, offset+1,
                                 len, 0);
        }
        else {
            numrec = tvb_get_ntohl (tvb, offset);

            proto_tree_add_item (tree, hf_fcfzs_nummbrentries, tvb, offset,
                                 4, 0);
            offset += 4;
            for (i = 0; i < numrec; i++) {
                proto_tree_add_item (tree, hf_fcfzs_mbrtype, tvb, offset, 1, 0);
                switch (tvb_get_guint8 (tvb, offset)) {
                case FC_FZS_ZONEMBR_PWWN:
                case FC_FZS_ZONEMBR_NWWN:
                    proto_tree_add_string (tree, hf_fcfzs_mbrid, tvb,
                                           offset+4, 8,
                                           fcwwn_to_str (tvb_get_ptr (tvb,
                                                                      offset+4,
                                                                      8)));
                    break;
                case FC_FZS_ZONEMBR_DP:
                    proto_tree_add_string_format (tree,
                                                  hf_fcfzs_mbrid,
                                                  tvb, offset+4, 3, " ",
                                                  "0x%x",
                                                  tvb_get_ntoh24 (tvb,
                                                                  offset+4));
                    break;
                case FC_FZS_ZONEMBR_FCID:
                    proto_tree_add_string (tree, hf_fcfzs_mbrid, tvb,
                                           offset+4, 4,
                                           fc_to_str (tvb_get_ptr (tvb,
                                                                   offset+4,
                                                                   3)));
                    break;
                default:
                    proto_tree_add_string (tree, hf_fcfzs_mbrid, tvb,
                                           offset+4, 8,
                                           "Unknown member type format");
                }
                offset += 12;
            }
        }
    }
}

static void
dissect_fcfzs_gazs (tvbuff_t *tvb, proto_tree *tree, guint8 isreq)
{
    int offset = 16;            /* past the fc_ct header */
    
    if (tree) {
        if (!isreq) {
            dissect_fcfzs_zoneset (tvb, tree, offset);
        }
    }
}

static void
dissect_fcfzs_gzs (tvbuff_t *tvb, proto_tree *tree, guint8 isreq)
{
    int offset = 16;            /* past the fc_ct header */
    int len;
    
    if (tree) {
        if (isreq) {
            len = tvb_get_guint8 (tvb, offset);
            proto_tree_add_item (tree, hf_fcfzs_zonesetnmlen, tvb, offset,
                                 1, 0);
            proto_tree_add_item (tree, hf_fcfzs_zonesetname, tvb, offset+4,
                                 len, 0);
        }
        else {
            dissect_fcfzs_zoneset (tvb, tree, offset);
        }
    }
}

static void
dissect_fcfzs_adzs (tvbuff_t *tvb, proto_tree *tree, guint8 isreq)
{
    int offset = 16;            /* past the fc_ct header */
    
    if (tree) {
        if (isreq) {
            dissect_fcfzs_zoneset (tvb, tree, offset);
        }
    }
}

static void
dissect_fcfzs_azsd (tvbuff_t *tvb, proto_tree *tree, guint8 isreq)
{
    int offset = 16;            /* past the fc_ct header */
    
    if (tree) {
        if (isreq) {
            dissect_fcfzs_zoneset (tvb, tree, offset);
        }
    }
}

static void
dissect_fcfzs_arzs (tvbuff_t *tvb, proto_tree *tree, guint8 isreq)
{
    int offset = 16;            /* past the fc_ct header */
    int len;
    
    if (tree) {
        if (isreq) {
            len = tvb_get_guint8 (tvb, offset);
            proto_tree_add_item (tree, hf_fcfzs_zonesetnmlen, tvb, offset,
                                 1, 0);
            proto_tree_add_item (tree, hf_fcfzs_zonesetname, tvb, offset+4,
                                 len, 0);
        }
    }
}

static void
dissect_fcfzs_dzs (tvbuff_t *tvb _U_, proto_tree *tree _U_, guint8 isreq _U_)
{
    /* Both req & successful response contain just the FC_CT header */
    return;
}

static void
dissect_fcfzs_arzm (tvbuff_t *tvb, proto_tree *tree, guint8 isreq)
{
    int numrec, i, len, plen;
    int offset = 16;            /* past the fc_ct header */
 
    if (tree) {
        if (isreq) {
            len = tvb_get_guint8 (tvb, offset);
            proto_tree_add_item (tree, hf_fcfzs_zonenmlen, tvb, offset,
                                 1, 0);
            proto_tree_add_item (tree, hf_fcfzs_zonename, tvb, offset+1,
                                 len, 0);

            len += (len % 4);
            plen = tvb_length (tvb) - offset - len;

            numrec = plen/12;   /* each mbr rec is 12 bytes long */

            offset += len;
            for (i = 0; i < numrec; i++) {
                proto_tree_add_item (tree, hf_fcfzs_mbrtype, tvb, offset, 1, 0);
                switch (tvb_get_guint8 (tvb, offset)) {
                case FC_FZS_ZONEMBR_PWWN:
                case FC_FZS_ZONEMBR_NWWN:
                    proto_tree_add_string (tree, hf_fcfzs_mbrid, tvb,
                                           offset+4, 8,
                                           fcwwn_to_str (tvb_get_ptr (tvb,
                                                                      offset+4,
                                                                      8)));
                    break;
                case FC_FZS_ZONEMBR_DP:
                    proto_tree_add_string_format (tree,
                                                  hf_fcfzs_mbrid,
                                                  tvb, offset+4, 3, " ",
                                                  "0x%x",
                                                  tvb_get_ntoh24 (tvb,
                                                                  offset+4));
                    break;
                case FC_FZS_ZONEMBR_FCID:
                    proto_tree_add_string (tree, hf_fcfzs_mbrid, tvb,
                                           offset+4, 4,
                                           fc_to_str (tvb_get_ptr (tvb,
                                                                   offset+4,
                                                                   3)));
                    break;
                default:
                    proto_tree_add_string (tree, hf_fcfzs_mbrid, tvb,
                                           offset+4, 8,
                                           "Unknown member type format");
                }
                offset += 12;
            }
        }
    }
}

static void
dissect_fcfzs_arzd (tvbuff_t *tvb, proto_tree *tree, guint8 isreq)
{
    int offset = 16;            /* past the fc_ct header */
    int len;
    
    if (tree) {
        if (isreq) {
            len = tvb_get_guint8 (tvb, offset);
            proto_tree_add_item (tree, hf_fcfzs_zonesetnmlen, tvb, offset,
                                 1, 0);
            proto_tree_add_item (tree, hf_fcfzs_zonesetname, tvb, offset+4,
                                 len, 0);
            len += (len % 4);
            offset += len;
            
            len = tvb_get_guint8 (tvb, offset);
            proto_tree_add_item (tree, hf_fcfzs_zonenmlen, tvb, offset, 1, 0);
            proto_tree_add_item (tree, hf_fcfzs_zonename, tvb, offset+4,
                                 len, 0);
        }
    }
}

static void
dissect_fcfzs_rjt (tvbuff_t *tvb, proto_tree *tree)
{
    int offset = 0;

    if (tree) {
        proto_tree_add_item (tree, hf_fcfzs_reason, tvb, offset+13, 1, 0);
        proto_tree_add_item (tree, hf_fcfzs_rjtdetail, tvb, offset+14, 1, 0);
        proto_tree_add_item (tree, hf_fcfzs_rjtvendor, tvb, offset+15, 1, 0); 
    }
}

static void
dissect_fcfzs (tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{

/* Set up structures needed to add the protocol subtree and manage it */
    proto_item *ti;
    proto_tree *fcfzs_tree = NULL;
    int offset = 0;
    fc_ct_preamble cthdr;
    int opcode,
        failed_opcode = 0;
    conversation_t *conversation;
    fcfzs_conv_data_t *cdata;
    fcfzs_conv_key_t ckey, *req_key;
    guint8 isreq = 1;

    /* Make entries in Protocol column and Info column on summary display */
    if (check_col(pinfo->cinfo, COL_PROTOCOL)) 
        col_set_str(pinfo->cinfo, COL_PROTOCOL, "Zone Server");
    
    
    tvb_memcpy (tvb, (guint8 *)&cthdr, offset, FCCT_PRMBL_SIZE);
    cthdr.revision = tvb_get_guint8 (tvb, offset+1);
    cthdr.in_id = tvb_get_ntoh24 (tvb, offset);
    cthdr.opcode = ntohs (cthdr.opcode);
    opcode = cthdr.opcode;
    cthdr.maxres_size = ntohs (cthdr.maxres_size);

    if (tree) {
        ti = proto_tree_add_protocol_format (tree, proto_fcfzs, tvb, 0,
                                             tvb_length (tvb),
                                             "Zone Server");
        fcfzs_tree = proto_item_add_subtree (ti, ett_fcfzs);
        proto_tree_add_item (fcfzs_tree, hf_fcfzs_opcode, tvb, offset+8, 2, 0);
        proto_tree_add_item (fcfzs_tree, hf_fcfzs_maxres_size, tvb, offset+10,
                             2, 0);
    }
    
    if ((opcode != FCCT_MSG_ACC) && (opcode != FCCT_MSG_RJT)) {
        conversation = find_conversation (pinfo->fd->num, &pinfo->src, &pinfo->dst,
                                          pinfo->ptype, pinfo->oxid,
                                          pinfo->rxid, NO_PORT2);
        if (!conversation) {
            conversation = conversation_new (pinfo->fd->num, &pinfo->src, &pinfo->dst,
                                             pinfo->ptype, pinfo->oxid,
                                             pinfo->rxid, NO_PORT2);
        }
        
        ckey.conv_idx = conversation->index;
        
        cdata = (fcfzs_conv_data_t *)g_hash_table_lookup (fcfzs_req_hash,
                                                            &ckey);
        if (cdata) {
            /* Since we never free the memory used by an exchange, this maybe a
             * case of another request using the same exchange as a previous
             * req. 
             */
            cdata->opcode = opcode;
        }
        else {
            req_key = g_mem_chunk_alloc (fcfzs_req_keys);
            req_key->conv_idx = conversation->index;
            
            cdata = g_mem_chunk_alloc (fcfzs_req_vals);
            cdata->opcode = opcode;
            
            g_hash_table_insert (fcfzs_req_hash, req_key, cdata);
        }
        if (check_col (pinfo->cinfo, COL_INFO)) {
            col_set_str (pinfo->cinfo, COL_INFO, val_to_str (opcode, fc_fzs_opcode_val,
                                                          "0x%x"));
        }
    }
    else {
        /* Opcode is ACC or RJT */
        conversation = find_conversation (pinfo->fd->num, &pinfo->src, &pinfo->dst,
                                          pinfo->ptype, pinfo->oxid,
                                          pinfo->rxid, NO_PORT2);
        isreq = 0;
        if (!conversation) {
            if (tree && (opcode == FCCT_MSG_ACC)) {
                if (check_col (pinfo->cinfo, COL_INFO)) {
                    col_set_str (pinfo->cinfo, COL_INFO,
                                 val_to_str (opcode, fc_fzs_opcode_val,
                                             "0x%x"));
                }
                /* No record of what this accept is for. Can't decode */
                proto_tree_add_text (fcfzs_tree, tvb, 0, tvb_length (tvb),
                                     "No record of Exchg. Unable to decode MSG_ACC");
                return;
            }
        }
        else {
            ckey.conv_idx = conversation->index;

            cdata = (fcfzs_conv_data_t *)g_hash_table_lookup (fcfzs_req_hash, &ckey);

            if (cdata != NULL) {
                if (opcode == FCCT_MSG_ACC)
                    opcode = cdata->opcode;
                else
                    failed_opcode = cdata->opcode;
            }
            
            if (check_col (pinfo->cinfo, COL_INFO)) {
                if (opcode != FCCT_MSG_RJT) {
                    col_add_fstr (pinfo->cinfo, COL_INFO, "MSG_ACC (%s)",
                                  val_to_str (opcode,
                                              fc_fzs_opcode_val, "0x%x"));
                }
                else {
                    col_add_fstr (pinfo->cinfo, COL_INFO, "MSG_RJT (%s)",
                                  val_to_str (failed_opcode,
                                              fc_fzs_opcode_val, "0x%x"));
                }
            }
                
            if (tree) {
                if ((cdata == NULL) && (opcode != FCCT_MSG_RJT)) {
                    /* No record of what this accept is for. Can't decode */
                    proto_tree_add_text (fcfzs_tree, tvb, 0, tvb_length (tvb),
                                         "No record of Exchg. Unable to decode MSG_ACC/RJT");
                    return;
                }
            }
        }
    }

    switch (opcode) {
    case FCCT_MSG_RJT:
        dissect_fcfzs_rjt (tvb, fcfzs_tree);
        break;
    case FC_FZS_GZC:
        dissect_fcfzs_gzc (tvb, fcfzs_tree, isreq);
        break;
    case FC_FZS_GEST:
        dissect_fcfzs_gest (tvb, fcfzs_tree, isreq);
        break;
    case FC_FZS_GZSN:
        dissect_fcfzs_gzsn (tvb, fcfzs_tree, isreq);
        break;
    case FC_FZS_GZD:
        dissect_fcfzs_gzd (tvb, fcfzs_tree, isreq);
        break;
    case FC_FZS_GZM:
        dissect_fcfzs_gzm (tvb, fcfzs_tree, isreq);
        break;
    case FC_FZS_GAZS:
        dissect_fcfzs_gazs (tvb, fcfzs_tree, isreq);
        break;
    case FC_FZS_GZS:
        dissect_fcfzs_gzs (tvb, fcfzs_tree, isreq);
        break;
    case FC_FZS_ADZS:
        dissect_fcfzs_adzs (tvb, fcfzs_tree, isreq);
        break;
    case FC_FZS_AZSD:
        dissect_fcfzs_azsd (tvb, fcfzs_tree, isreq);
        break;
    case FC_FZS_AZS:
        dissect_fcfzs_arzs (tvb, fcfzs_tree, isreq);
        break;
    case FC_FZS_DZS:
        dissect_fcfzs_dzs (tvb, fcfzs_tree, isreq);
        break;
    case FC_FZS_AZM:
        dissect_fcfzs_arzm (tvb, fcfzs_tree, isreq);
        break;
    case FC_FZS_AZD:
        dissect_fcfzs_arzd (tvb, fcfzs_tree, isreq);
        break;
    case FC_FZS_RZM:
        dissect_fcfzs_arzm (tvb, fcfzs_tree, isreq);
        break;
    case FC_FZS_RZD:
        dissect_fcfzs_arzd (tvb, fcfzs_tree, isreq);
        break;
    case FC_FZS_RZS:
        dissect_fcfzs_arzs (tvb, fcfzs_tree, isreq);
        break;
    default:
        call_dissector (data_handle, tvb, pinfo, tree);
        break;
    }
}

/* Register the protocol with Ethereal */

/* this format is require because a script is used to build the C function
   that calls all the protocol registration.
*/

void
proto_register_fcfzs(void)
{                 

/* Setup list of header fields  See Section 1.6.1 for details*/
    static hf_register_info hf[] = {
        { &hf_fcfzs_opcode,
          {"Opcode", "fcfzs.opcode", FT_UINT16, BASE_HEX,
           VALS (fc_fzs_opcode_val), 0x0, "", HFILL}},
        { &hf_fcfzs_gzc_flags,
          {"Capabilities", "fcfzs.gzc.flags", FT_UINT8, BASE_HEX, NULL, 0x0,
           "", HFILL}},
        { &hf_fcfzs_gzc_vendor,
          {"Vendor Specific Flags", "fcfzs.gzc.vendor", FT_UINT32, BASE_HEX,
           NULL, 0x0, "", HFILL}},
        { &hf_fcfzs_zone_state,
          {"Zone State", "fcfzs.zone.state", FT_UINT8, BASE_HEX, NULL, 0x0,
           "", HFILL}},
        { &hf_fcfzs_gest_vendor,
          {"Vendor Specific State", "fcfzs.gest.vendor", FT_UINT32, BASE_HEX,
           NULL, 0x0, "", HFILL}},
        { &hf_fcfzs_numzoneattrs,
          {"Number of Zone Attribute Entries", "fcfzs.zone.numattrs",
           FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL}},
        { &hf_fcfzs_zonesetnmlen,
          {"Zone Set Name Length", "fcfzs.zoneset.namelen", FT_UINT8, BASE_DEC,
           NULL, 0x0, "", HFILL}},
        { &hf_fcfzs_zonesetname,
          {"Zone Set Name", "fcfzs.zoneset.name", FT_STRING, BASE_HEX, NULL,
           0x0, "", HFILL}},
        { &hf_fcfzs_numzones,
          {"Number of Zones", "fcfzs.zoneset.numzones", FT_UINT32, BASE_DEC,
           NULL, 0x0, "", HFILL}},
        { &hf_fcfzs_numzonesetattrs,
          {"Number of Zone Set Attribute Entries", "fcfzs.zoneset.numattrs",
           FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL}},
        { &hf_fcfzs_zonenmlen,
          {"Zone Name Length", "fcfzs.zone.namelen", FT_UINT8, BASE_DEC, NULL,
           0x0, "", HFILL}},
        { &hf_fcfzs_zonename,
          {"Zone Name", "fcfzs.zone.name", FT_STRING, BASE_HEX, NULL, 0x0, "",
           HFILL}},
        { &hf_fcfzs_nummbrs,
          {"Number of Zone Members", "fcfzs.zone.nummbrs", FT_UINT32, BASE_DEC,
           NULL, 0x0, "", HFILL}},
        { &hf_fcfzs_nummbrentries,
          {"Number of Zone Member Attribute Entries", "fcfzs.zonembr.numattrs",
           FT_UINT32, BASE_DEC, NULL, 0x0, "", HFILL}},
        { &hf_fcfzs_mbrtype,
          {"Zone Member Identifier Type", "fcfzs.zonembr.idtype", FT_UINT8,
           BASE_HEX, VALS (fc_fzs_zonembr_type_val), 0x0, "", HFILL}},
        { &hf_fcfzs_mbridlen,
          {"Zone Member Identifier Length", "fcfzs.zonembr.idlen", FT_UINT8,
           BASE_DEC, NULL, 0x0, "", HFILL}},
        { &hf_fcfzs_mbrid,
          {"Zone Member Identifier", "fcfzs.zone.mbrid", FT_STRING, BASE_HEX,
           NULL, 0x0, "", HFILL}},
        { &hf_fcfzs_reason,
          {"Reason Code", "fcfzs.reason", FT_UINT8, BASE_HEX,
           VALS (fc_ct_rjt_code_vals), 0x0, "", HFILL}},
        { &hf_fcfzs_rjtdetail,
          {"Reason Code Explanation", "fcfzs.rjtdetail", FT_UINT8, BASE_HEX,
           VALS (fc_fzs_rjt_code_val), 0x0, "", HFILL}},
        { &hf_fcfzs_rjtvendor,
          {"Vendor Specific Reason", "fcfzs.rjtvendor", FT_UINT8, BASE_HEX,
           NULL, 0x0, "", HFILL}},
        { &hf_fcfzs_maxres_size,
          {"Maximum/Residual Size", "fcfzs.maxres_size", FT_UINT16, BASE_DEC,
           NULL, 0x0, "", HFILL}},
        { &hf_fcfzs_mbrid_lun,
          {"LUN", "fcfzs.zone.lun", FT_BYTES, BASE_HEX, NULL, 0x0, "",
           HFILL}},
    };

    /* Setup protocol subtree array */
    static gint *ett[] = {
        &ett_fcfzs,
    };

    /* Register the protocol name and description */
    proto_fcfzs = proto_register_protocol("Fibre Channel Fabric Zone Server", "FC FZS", "fcfzs");

    /* Required function calls to register the header fields and subtrees used */
    proto_register_field_array(proto_fcfzs, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));
    register_init_routine (&fcfzs_init_protocol);

}

/* If this dissector uses sub-dissector registration add a registration routine.
   This format is required because a script is used to find these routines and
   create the code that calls these routines.
*/
void
proto_reg_handoff_fcfzs (void)
{
    dissector_handle_t fzs_handle;

    fzs_handle = create_dissector_handle (dissect_fcfzs, proto_fcfzs);
    dissector_add("fcct.server", FCCT_GSRVR_FZS, fzs_handle);

    data_handle = find_dissector ("data");
}


