/* packet-fcswils
 * Routines for FC Inter-switch link services
 * Copyright 2001, Dinesh G Dutt <ddutt@cisco.com>
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
#include "packet-fcswils.h"

#define FC_SWILS_RPLY               0x0
#define FC_SWILS_REQ                0x1
#define FC_SWILS_RSCN_DEVENTRY_SIZE 20

/* Zone name has the structure:
 * name_len (1 byte), rsvd (3 bytes), name (m bytes), fill (n bytes)
 * name_len excludes the 4 initial bytes before the name
 */
#define ZONENAME_LEN(x, y)  (tvb_get_guint8(x, y)+4)

/* Initialize the protocol and registered fields */
static int proto_fcswils               = -1;
static int hf_swils_opcode             = -1;
static int hf_swils_elp_rev            = -1;
static int hf_swils_elp_flags          = -1;
static int hf_swils_elp_r_a_tov        = -1;
static int hf_swils_elp_e_d_tov        = -1;
static int hf_swils_elp_req_epn        = -1;
static int hf_swils_elp_req_esn        = -1;
static int hf_swils_elp_clsf_svcp      = -1;
static int hf_swils_elp_clsf_rcvsz     = -1;
static int hf_swils_elp_clsf_conseq    = -1;
static int hf_swils_elp_clsf_e2e       = -1;
static int hf_swils_elp_clsf_openseq   = -1;
static int hf_swils_elp_cls1_svcp      = -1;
static int hf_swils_elp_cls1_rcvsz     = -1;
static int hf_swils_elp_cls2_svcp      = -1;
static int hf_swils_elp_cls2_rcvsz     = -1;
static int hf_swils_elp_cls3_svcp      = -1;
static int hf_swils_elp_cls3_rcvsz     = -1;
static int hf_swils_elp_isl_fc_mode    = -1;
static int hf_swils_elp_fcplen         = -1;
static int hf_swils_elp_b2bcredit      = -1;
static int hf_swils_elp_compat1        = -1;
static int hf_swils_elp_compat2        = -1;
static int hf_swils_elp_compat3        = -1;
static int hf_swils_elp_compat4        = -1;
static int hf_swils_efp_rec_type       = -1;
static int hf_swils_efp_dom_id         = -1;
static int hf_swils_efp_switch_name    = -1;
static int hf_swils_efp_mcast_grpno    = -1;
static int hf_swils_efp_alias_token    = -1;
static int hf_swils_efp_payload_len    = -1;
static int hf_swils_efp_pswitch_pri    = -1;
static int hf_swils_efp_pswitch_name   = -1;
static int hf_swils_dia_switch_name    = -1;
static int hf_swils_rdi_payload_len    = -1;
static int hf_swils_rdi_req_sname      = -1;
static int hf_swils_fspfh_cmd          = -1;
static int hf_swils_fspfh_rev          = -1;
static int hf_swils_fspfh_ar_num       = -1;
static int hf_swils_fspfh_auth_type    = -1;
static int hf_swils_fspfh_dom_id       = -1;
static int hf_swils_fspfh_auth         = -1;
static int hf_swils_hlo_options        = -1;
static int hf_swils_hlo_hloint         = -1;
static int hf_swils_hlo_deadint        = -1;
static int hf_swils_hlo_rcv_domid      = -1;
static int hf_swils_hlo_orig_pidx      = -1;
static int hf_swils_ldrec_linkid       = -1;
static int hf_swils_ldrec_out_pidx     = -1;
static int hf_swils_ldrec_nbr_pidx     = -1;
static int hf_swils_ldrec_link_type    = -1;
static int hf_swils_ldrec_link_cost    = -1;
static int hf_swils_lsrh_lsr_type      = -1;
static int hf_swils_lsrh_lsid          = -1;
static int hf_swils_lsrh_adv_domid     = -1;
static int hf_swils_lsrh_ls_incid      = -1;
static int hf_swils_esc_pdesc_vendorid = -1;
static int hf_swils_esc_swvendorid     = -1;
static int hf_swils_esc_protocolid     = -1;
static int hf_swils_rscn_evtype        = -1;
static int hf_swils_rscn_addrfmt       = -1;
static int hf_swils_rscn_detectfn      = -1;
static int hf_swils_rscn_affectedport  = -1;
static int hf_swils_rscn_portstate     = -1;
static int hf_swils_rscn_portid        = -1;
static int hf_swils_rscn_pwwn          = -1;
static int hf_swils_rscn_nwwn          = -1;
static int hf_swils_zone_activezonenm  = -1;
static int hf_swils_zone_objname       = -1;
static int hf_swils_zone_objtype       = -1;
static int hf_swils_zone_mbrtype       = -1;
static int hf_swils_zone_protocol      = -1;
static int hf_swils_zone_mbrid         = -1;
static int hf_swils_zone_status        = -1;
static int hf_swils_zone_reason        = -1;
static int hf_swils_aca_domainid       = -1;
static int hf_swils_sfc_opcode         = -1;
static int hf_swils_sfc_zonenm         = -1;
static int hf_swils_rjt                = -1;
static int hf_swils_rjtdet             = -1;
static int hf_swils_rjtvendor          = -1;
static int hf_swils_zone_mbrid_lun     = -1;

/* Initialize the subtree pointers */
static gint ett_fcswils             = -1;
static gint ett_fcswils_swacc       = -1;
static gint ett_fcswils_swrjt       = -1;
static gint ett_fcswils_elp         = -1;
static gint ett_fcswils_efp         = -1;
static gint ett_fcswils_efplist     = -1;
static gint ett_fcswils_dia         = -1;
static gint ett_fcswils_rdi         = -1;
static gint ett_fcswils_fspfhdr     = -1;
static gint ett_fcswils_hlo         = -1;
static gint ett_fcswils_lsrec       = -1;
static gint ett_fcswils_lsrechdr    = -1;
static gint ett_fcswils_ldrec       = -1;
static gint ett_fcswils_lsu         = -1;
static gint ett_fcswils_lsa         = -1;
static gint ett_fcswils_bf          = -1;
static gint ett_fcswils_rcf         = -1;
static gint ett_fcswils_rscn        = -1;
static gint ett_fcswils_rscn_dev    = -1;
static gint ett_fcswils_drlir       = -1;
static gint ett_fcswils_mr          = -1;
static gint ett_fcswils_zoneobjlist = -1;
static gint ett_fcswils_zoneobj     = -1;
static gint ett_fcswils_zonembr     = -1;
static gint ett_fcswils_aca         = -1;
static gint ett_fcswils_rca         = -1;
static gint ett_fcswils_sfc         = -1;
static gint ett_fcswils_ufc         = -1;
static gint ett_fcswils_esc         = -1;
static gint ett_fcswils_esc_pdesc   = -1;

static const value_string fc_swils_opcode_key_val[] = {
    {FC_SWILS_SWRJT  , "SW_RJT"},
    {FC_SWILS_SWACC  , "SW_ACC"},
    {FC_SWILS_ELP    , "ELP"},
    {FC_SWILS_EFP    , "EFP"},
    {FC_SWILS_DIA    , "DIA"},
    {FC_SWILS_RDI    , "RDI"},
    {FC_SWILS_HLO    , "HLO"},
    {FC_SWILS_LSU    , "LSU"},
    {FC_SWILS_LSA    , "LSA"},
    {FC_SWILS_BF     , "BF"},
    {FC_SWILS_RCF    , "RCF"},
    {FC_SWILS_RSCN   , "SW_RSCN"},
    {FC_SWILS_DRLIR  , "DRLIR"},
    {FC_SWILS_DSCN   , "DSCN"},
    {FC_SWILS_LOOPD  , "LOOPD"},
    {FC_SWILS_MR     , "MR"},
    {FC_SWILS_ACA    , "ACA"},
    {FC_SWILS_RCA    , "RCA"},
    {FC_SWILS_SFC    , "SFC"},
    {FC_SWILS_UFC    , "UFC"},
    {FC_SWILS_ESC    , "ESC"},
    {FC_SWILS_AUTH_ILS, "AUTH_ILS"},
    {0, NULL},
};

static const value_string fc_swils_rjt_val [] = {
    {FC_SWILS_RJT_INVCODE   , "Invalid Cmd Code"},
    {FC_SWILS_RJT_INVVER    , "Invalid Revision"},
    {FC_SWILS_RJT_LOGERR    , "Logical Error"},
    {FC_SWILS_RJT_INVSIZE   , "Invalid Size"},
    {FC_SWILS_RJT_LOGBSY    , "Logical Busy"},
    {FC_SWILS_RJT_PROTERR   , "Protocol Error"},
    {FC_SWILS_RJT_GENFAIL   , "Unable to Perform"},
    {FC_SWILS_RJT_CMDNOTSUPP, "Unsupported Cmd"},
    {FC_SWILS_RJT_VENDUNIQ  , "Vendor Unique Err"},
    {0, NULL},
};

static const value_string fc_swils_deterr_val [] = {
    {FC_SWILS_RJT_NODET ,      "No Additional Details"},
    {FC_SWILS_RJT_CLSF_ERR ,   "Class F Svc Param Err"},
    {FC_SWILS_RJT_CLSN_ERR ,   "Class N Svc Param Err"},
    {FC_SWILS_RJT_INVFC_CODE , "Unknown Flow Ctrl Code"},
    {FC_SWILS_RJT_INVFC_PARM , "Invalid Flow Ctrl Parm"},
    {FC_SWILS_RJT_INV_PNAME ,  "Invalid Port Name"},
    {FC_SWILS_RJT_INV_SNAME ,  "Invalid Switch Name"},
    {FC_SWILS_RJT_TOV_MSMTCH , "R_A_/E_D_TOV Mismatch"},
    {FC_SWILS_RJT_INV_DIDLST,  "Invalid Domain ID List"},
    {FC_SWILS_RJT_CMD_INPROG , "Cmd Already in Progress"},
    {FC_SWILS_RJT_OORSRC ,     "Insufficient Resources"},
    {FC_SWILS_RJT_NO_DID ,     "Domain ID Unavailable"},
    {FC_SWILS_RJT_INV_DID,     "Invalid Domain ID"},
    {FC_SWILS_RJT_NO_REQ ,     "Request Not Supported"},
    {FC_SWILS_RJT_NOLNK_PARM , "Link Parm Not Estd."},
    {FC_SWILS_RJT_NO_REQDID ,  "Group of Domain IDs Unavail"},
    {FC_SWILS_RJT_EP_ISOL ,    "E_Port Isolated"},
    {0, NULL}
};

static const value_string fcswils_elp_fc_val[] = {
    {FC_SWILS_ELP_FC_VENDOR, "Vendor Unique"},
    {FC_SWILS_ELP_FC_RRDY,   "R_RDY Flow Ctrl"},
    {0, NULL},
};

static const value_string fcswils_rectype_val[] = {
    {FC_SWILS_LRECTYPE_DOMAIN, "Domain ID List Rec"},
    {FC_SWILS_LRECTYPE_MCAST, "Multicast ID List Rec"},
    {0, NULL},
};

static const value_string fc_swils_link_type_val[] = {
    {0x01, "P2P Link"},
    {0xF0, "Vendor Specific"},
    {0xF1, "Vendor Specific"},
    {0xF2, "Vendor Specific"},
    {0xF3, "Vendor Specific"},
    {0xF4, "Vendor Specific"},
    {0xF5, "Vendor Specific"},
    {0xF6, "Vendor Specific"},
    {0xF7, "Vendor Specific"},
    {0xF8, "Vendor Specific"},
    {0xF9, "Vendor Specific"},
    {0xFA, "Vendor Specific"},
    {0xFB, "Vendor Specific"},
    {0xFC, "Vendor Specific"},
    {0xFD, "Vendor Specific"},
    {0xFE, "Vendor Specific"},
    {0xFF, "Vendor Specific"},
    {0, NULL},
};

static const value_string fc_swils_fspf_linkrec_val[] = {
    {FC_SWILS_LSR_SLR, "Switch Link Record"},
    {FC_SWILS_LSR_ARS, "AR Summary Record"},
    {0, NULL},
};

static const value_string fc_swils_fspf_lsrflags_val[] = {
    {0x0, "LSR is for a Topology Update"},
    {0x1, "LSR is for Initial DB Sync | Not the last seq in DB sync"},
    {0x2, "Last Seq in DB Sync. LSU has no LSRs"},
    {0x3, "LSR is for Initial DB Sync | Last Seq in DB Sync"},
    {0, NULL},
};

static const value_string fc_swils_rscn_portstate_val[] = {
    {0, "No Additional Info"},
    {1, "Port is online"},
    {2, "Port is offline"},
    {0, NULL},
};

static const value_string fc_swils_rscn_addrfmt_val[] = {
    {0, "Port Addr Format"},
    {1, "Area Addr Format"},
    {2, "Domain Addr Format"},
    {3, "Fabric Addr Format"},
    {0, NULL},
};

static const value_string fc_swils_rscn_detectfn_val[] = {
    {1, "Fabric Detected"},
    {2, "N_Port Detected"},
    {0, NULL},
};

static const value_string fc_swils_esc_protocol_val[] = {
    {0, "Reserved"},
    {1, "FSPF-Backbone Protocol"},
    {2, "FSPF Protocol"},
    {0, NULL},
};

static const value_string fc_swils_zoneobj_type_val[] = {
    {0, "Reserved"},
    {FC_SWILS_ZONEOBJ_ZONESET  , "Zone Set"},
    {FC_SWILS_ZONEOBJ_ZONE     , "Zone"},
    {FC_SWILS_ZONEOBJ_ZONEALIAS, "Zone Alias"},
    {0, NULL},
};

const value_string fc_swils_zonembr_type_val[] = {
    {0, "Reserved"},
    {FC_SWILS_ZONEMBR_WWN, "WWN"},
    {FC_SWILS_ZONEMBR_DP, "Domain/Physical Port (0x00ddpppp)"},
    {FC_SWILS_ZONEMBR_FCID, "FC Address"},
    {FC_SWILS_ZONEMBR_ALIAS, "Zone Alias"},
    {FC_SWILS_ZONEMBR_WWN_LUN, "WWN+LUN"},
    {FC_SWILS_ZONEMBR_DP_LUN, "Domain/Physical Port+LUN"},
    {FC_SWILS_ZONEMBR_FCID_LUN, "FCID+LUN"},
    {0, NULL},
};

static const value_string fc_swils_mr_rsp_val[] = {
    {0, "Successful"},
    {1, "Fabric Busy"},
    {2, "Failed"},
    {0, NULL},
};

static const value_string fc_swils_mr_reason_val[] = {
    {0x0, "No Reason"},
    {0x1, "Invalid Data Length"},
    {0x2, "Unsupported Command"},
    {0x3, "Reserved"},
    {0x4, "Not Authorized"},
    {0x5, "Invalid Request"},
    {0x6, "Fabric Changing"},
    {0x7, "Update Not Staged"},
    {0x8, "Invalid Zone Set Format"},
    {0x9, "Invalid Data"},
    {0xA, "Cannot Merge"},
    {0, NULL},
};

static const value_string fc_swils_sfc_op_val[] = {
    {0, "Reserved"},
    {1, "Reserved"},
    {2, "Reserved"},
    {3, "Activate Zone Set"},
    {4, "Deactivate Zone Set"},
    {0, NULL},
};

typedef struct _zonename {
    guint32 namelen:8,
            rsvd:24;
    gchar *name;
    gchar *pad;
} zonename_t;

typedef struct _fcswils_conv_key {
    guint32 conv_idx;
} fcswils_conv_key_t;

typedef struct _fcswils_conv_data {
    guint32 opcode;
} fcswils_conv_data_t;

GHashTable *fcswils_req_hash = NULL;
GMemChunk *fcswils_req_keys = NULL;
GMemChunk *fcswils_req_vals = NULL;
guint32 fcswils_init_count = 25;

static dissector_handle_t data_handle, fcsp_handle;

static gint get_zoneobj_len (tvbuff_t *tvb, gint offset);

/*
 * Hash Functions
 */
static gint
fcswils_equal(gconstpointer v, gconstpointer w)
{
  fcswils_conv_key_t *v1 = (fcswils_conv_key_t *)v;
  fcswils_conv_key_t *v2 = (fcswils_conv_key_t *)w;

  return (v1->conv_idx == v2->conv_idx);
}

static guint
fcswils_hash (gconstpointer v)
{
	fcswils_conv_key_t *key = (fcswils_conv_key_t *)v;
	guint val;

	val = key->conv_idx;

	return val;
}

/*
 * Protocol initialization
 */
static void
fcswils_init_protocol(void)
{
	if (fcswils_req_keys)
            g_mem_chunk_destroy (fcswils_req_keys);
	if (fcswils_req_vals)
            g_mem_chunk_destroy (fcswils_req_vals);
	if (fcswils_req_hash)
            g_hash_table_destroy (fcswils_req_hash);

	fcswils_req_hash = g_hash_table_new(fcswils_hash, fcswils_equal);
	fcswils_req_keys = g_mem_chunk_new("fcswils_req_keys",
                                           sizeof(fcswils_conv_key_t),
                                           fcswils_init_count * sizeof(fcswils_conv_key_t),
                                           G_ALLOC_AND_FREE);
	fcswils_req_vals = g_mem_chunk_new("fcswils_req_vals",
                                           sizeof(fcswils_conv_data_t),
                                           fcswils_init_count * sizeof(fcswils_conv_data_t),
                                           G_ALLOC_AND_FREE);
}

static gchar *
zonenm_to_str (tvbuff_t *tvb, gint offset)
{
    int len = tvb_get_guint8 (tvb, offset);
    return ((gchar *)tvb_get_ptr (tvb, offset+4, len));
}

/* Offset points to the start of the zone object */
static gint
get_zoneobj_len (tvbuff_t *tvb, gint offset)
{
    gint numrec, numrec1;
    guint8 objtype;
    gint i, j, len;

    /* zone object structure is:
     * type (1 byte), protocol (1 byte), rsvd (2 bytes), obj name (x bytes),
     * num of zone mbrs (4 bytes ), list of zone members (each member if of
     * variable length).
     *
     * zone member structure is:
     * type (1 byte), rsvd (1 byte), flags (1 byte), id_len (1 byte),
     * id (id_len bytes)
     */
    objtype = tvb_get_guint8 (tvb, offset);
    len = 4 + ZONENAME_LEN (tvb, offset+4); /* length upto num_of_mbrs field */
    numrec = tvb_get_ntohl (tvb, offset+len); /* gets us num of zone mbrs */

    len += 4;                   /* + num_mbrs */
    for (i = 0; i < numrec; i++) {
        if (objtype == FC_SWILS_ZONEOBJ_ZONESET) {
            len += 4 + ZONENAME_LEN (tvb, offset+4+len); /* length upto num_of_mbrs field */
            numrec1 = tvb_get_ntohl (tvb, offset+len);

            len += 4;
            for (j = 0; j < numrec1; j++) {
                len += 4 + tvb_get_guint8 (tvb, offset+3+len);
            }
        }
        else if (objtype == FC_SWILS_ZONEOBJ_ZONE) {
            len += 4 + tvb_get_guint8 (tvb, offset+3+len);
        }
    }

    return len;
}

static void
dissect_swils_elp (tvbuff_t *tvb, proto_tree *elp_tree, guint8 isreq _U_)
{
    
    /* Set up structures needed to add the protocol subtree and manage it */
    int offset = 0,
        stroff = 0;
    gchar flags[40];
    fcswils_elp elp;

    /* Response i.e. SW_ACC for an ELP has the same format as the request */
    /* We skip the initial 4 bytes as we don't care about the opcode */
    tvb_memcpy (tvb, (guint8 *)&elp, 4, FC_SWILS_ELP_SIZE);
    
    elp.r_a_tov = ntohl (elp.r_a_tov);
    elp.e_d_tov = ntohl (elp.e_d_tov);
    elp.isl_flwctrl_mode = ntohs (elp.isl_flwctrl_mode);
    elp.flw_ctrl_parmlen = ntohs (elp.flw_ctrl_parmlen);
    
    if (elp_tree) {
        offset += 4;
        proto_tree_add_item (elp_tree, hf_swils_elp_rev, tvb, offset++, 1, 0);
        proto_tree_add_item (elp_tree, hf_swils_elp_flags, tvb, offset, 2, 0);
        offset += 3;
        proto_tree_add_uint_format (elp_tree, hf_swils_elp_r_a_tov, tvb, offset, 4,
                                    elp.r_a_tov, "R_A_TOV: %d msecs", elp.r_a_tov);
        offset += 4;
        proto_tree_add_uint_format (elp_tree, hf_swils_elp_e_d_tov, tvb, offset, 4,
                                    elp.e_d_tov, "E_D_TOV: %d msecs", elp.e_d_tov);
        offset += 4;
        proto_tree_add_string (elp_tree, hf_swils_elp_req_epn, tvb, offset, 8,
                               fcwwn_to_str (elp.req_epname));
        offset += 8;
        proto_tree_add_string (elp_tree, hf_swils_elp_req_esn, tvb, offset, 8,
                               fcwwn_to_str (elp.req_sname));
        offset += 8;

        flags[0] = '\0';
        if (elp.clsf_svcparm[0] & 0x80) {
            strcpy (flags, "Class F Valid");

            if (elp.clsf_svcparm[4] & 0x20) {
                strcpy (&flags[13], " | X_ID Interlock");
            }
            else {
                strcpy (&flags[13], " | No X_ID Interlk");
            }
        }
        else {
            strcpy (flags, "Class F Invld");
        }
        proto_tree_add_bytes_format (elp_tree, hf_swils_elp_clsf_svcp, tvb, offset, 6, 
                                     &elp.clsf_svcparm[0], "Class F Svc Parameters: (%s)", flags);
        offset += 6;

        proto_tree_add_item (elp_tree, hf_swils_elp_clsf_rcvsz, tvb, offset, 2, 0);
        offset += 2;
        proto_tree_add_item (elp_tree, hf_swils_elp_clsf_conseq, tvb, offset, 2, 0);
        offset += 2;
        proto_tree_add_item (elp_tree, hf_swils_elp_clsf_e2e, tvb, offset, 2, 0);
        offset += 2;
        proto_tree_add_item (elp_tree, hf_swils_elp_clsf_openseq, tvb, offset, 2, 0);
        offset += 4;

        flags[0] = '\0';
        stroff = 0;
        if (elp.cls1_svcparm[0] & 0x80) {
            strcpy (&flags[stroff], "Class 1 Valid");
            stroff += 13;
            if (elp.cls1_svcparm[0] & 0x40) {
                strcpy (&flags[stroff], " | IMX");
                stroff += 6;
            }
            if (elp.cls1_svcparm[0] & 0x20) {
                strcpy (&flags[stroff], " | XPS");
                stroff += 6;
            }
            if (elp.cls1_svcparm[0] & 0x10) {
                strcpy (&flags[stroff], " | LKS");
            }
        }
        else {
            strcpy (&flags[0], "Class 1 Invalid");
        }
        
        proto_tree_add_bytes_format (elp_tree, hf_swils_elp_cls1_svcp, tvb, offset, 2,
                                     tvb_get_ptr (tvb, offset, 2),
                                     "Class 1 Svc Parameters: (%s)", flags);
        offset += 2;
        if (elp.cls1_svcparm[0] & 0x80) {
            proto_tree_add_item (elp_tree, hf_swils_elp_cls1_rcvsz, tvb, offset, 2, 0);
        }
        offset += 2;

        flags[0] = '\0';
        if (elp.cls2_svcparm[0] & 0x80) {
            strcpy (flags, "Class 2 Valid");

            if (elp.cls2_svcparm[0] & 0x08) {
                strcpy (&flags[13], " | Seq Delivery");
            }
            else {
                strcpy (&flags[13], " | No Seq Delivery");
            }
        }
        else {
            strcpy (flags, "Class 2 Invld");
        }
        
        proto_tree_add_bytes_format (elp_tree, hf_swils_elp_cls2_svcp, tvb, offset, 2,
                                     &elp.cls2_svcparm[0],
                                     "Class 2 Svc Parameters: (%s)", flags);
        offset += 2;
        
        if (elp.cls2_svcparm[0] & 0x80) {
            proto_tree_add_item (elp_tree, hf_swils_elp_cls2_rcvsz, tvb, offset, 2, 0);
        }
        offset += 2;
        
        flags[0] = '\0';
        if (elp.cls3_svcparm[0] & 0x80) {
            strcpy (flags, "Class 3 Valid");

            if (elp.cls3_svcparm[0] & 0x08) {
                strcpy (&flags[13], " | Seq Delivery");
            }
            else {
                strcpy (&flags[13], " | No Seq Delivery");
            }
        }
        else {
            strcpy (flags, "Class 3 Invld");
        }
        proto_tree_add_bytes_format (elp_tree, hf_swils_elp_cls3_svcp, tvb, offset, 2,
                                     &elp.cls3_svcparm[0],
                                     "Class 3 Svc Parameters: (%s)", flags);
        offset += 2;

        if (elp.cls3_svcparm[0] & 0x80) {
            proto_tree_add_item (elp_tree, hf_swils_elp_cls3_rcvsz, tvb, offset, 2, 0);
        }
        offset += 22;

        proto_tree_add_string (elp_tree, hf_swils_elp_isl_fc_mode, tvb, offset, 2,
                               val_to_str (elp.isl_flwctrl_mode, fcswils_elp_fc_val, "Vendor Unique"));
        offset += 2;
        proto_tree_add_item (elp_tree, hf_swils_elp_fcplen, tvb, offset, 2, 0);
        offset += 2;
        proto_tree_add_item (elp_tree, hf_swils_elp_b2bcredit, tvb, offset, 4, 0);
        offset += 4;
        proto_tree_add_item (elp_tree, hf_swils_elp_compat1, tvb, offset, 4, 0);
        offset += 4;
        proto_tree_add_item (elp_tree, hf_swils_elp_compat2, tvb, offset, 4, 0);
        offset += 4;
        proto_tree_add_item (elp_tree, hf_swils_elp_compat3, tvb, offset, 4, 0);
        offset += 4;
        proto_tree_add_item (elp_tree, hf_swils_elp_compat4, tvb, offset, 4, 0);
    }

}

static void
dissect_swils_efp (tvbuff_t *tvb, proto_tree *efp_tree, guint8 isreq _U_)
{

/* Set up structures needed to add the protocol subtree and manage it */
    proto_item *subti;
    proto_tree *lrec_tree;
    int num_listrec = 0,
        offset = 0;
    fcswils_efp efp;
    fcswils_efp_listrec *lrec;
    
    tvb_memcpy (tvb, (guint8 *)&efp, offset, FC_SWILS_EFP_SIZE);
    efp.payload_len = ntohs (efp.payload_len);
    efp.listrec = (fcswils_efp_listrec *)tvb_get_ptr (tvb, FC_SWILS_EFP_SIZE,
                                                      efp.payload_len - FC_SWILS_EFP_SIZE);
    
    if (efp_tree) {
        offset += 2;
        proto_tree_add_item (efp_tree, hf_swils_efp_payload_len, tvb, offset, 2, 0);
        offset += 5;
        proto_tree_add_item (efp_tree, hf_swils_efp_pswitch_pri, tvb,
                             offset++, 1, 0);
        proto_tree_add_string (efp_tree, hf_swils_efp_pswitch_name, tvb, offset,
                               8, fcwwn_to_str (efp.pswitch_name));
        offset += 8;

        /* Add List Records now */
        if (efp.reclen) {
            num_listrec = (efp.payload_len - FC_SWILS_EFP_SIZE)/efp.reclen;
        }
        else {
            num_listrec = 0;
        }
        
        while (num_listrec--) {
            lrec =  (fcswils_efp_listrec *)tvb_get_ptr (tvb, offset, efp.reclen);
            if (lrec != NULL) {
                if (lrec->didrec.rec_type == FC_SWILS_LRECTYPE_DOMAIN) {
                    subti = proto_tree_add_text (efp_tree, tvb, offset,
                                                 (efp.payload_len - FC_SWILS_EFP_SIZE),
                                                 "Domain ID Record");
                    lrec_tree = proto_item_add_subtree (subti, ett_fcswils_efplist);
                    proto_tree_add_item (lrec_tree, hf_swils_efp_dom_id, tvb, offset+1, 1, 0); 
                    proto_tree_add_string (lrec_tree, hf_swils_efp_switch_name, tvb, offset+8, 8,
                                           fcwwn_to_str (lrec->didrec.sname));
                }
                else if (lrec->didrec.rec_type == FC_SWILS_LRECTYPE_MCAST) {
                    subti = proto_tree_add_text (efp_tree, tvb, offset,
                                                 (efp.payload_len - FC_SWILS_EFP_SIZE),
                                                 "Multicast ID Record");
                    lrec_tree = proto_item_add_subtree (subti, ett_fcswils_efplist);
                    proto_tree_add_item (lrec_tree, hf_swils_efp_mcast_grpno, tvb, offset+1, 1, 0);  
                }
                offset += efp.reclen;
            }
        }
    }
}

static void
dissect_swils_dia (tvbuff_t *tvb, proto_tree *dia_tree, guint8 isreq _U_)
{
    /* Set up structures needed to add the protocol subtree and manage it */
    int offset = 0;

    if (dia_tree) {
        proto_tree_add_string (dia_tree, hf_swils_dia_switch_name, tvb, offset+4,
                               8, fcwwn_to_str (tvb_get_ptr (tvb, offset+4, 8)));
    }
}

static void
dissect_swils_rdi (tvbuff_t *tvb, proto_tree *rdi_tree, guint8 isreq)
{
    /* Set up structures needed to add the protocol subtree and manage it */
    int offset = 0;
    int i, plen, numrec;
    
    if (rdi_tree) {
        plen = tvb_get_ntohs (tvb, offset+2);
        
        proto_tree_add_item (rdi_tree, hf_swils_rdi_payload_len, tvb, offset+2, 2, 0);
        proto_tree_add_string (rdi_tree, hf_swils_rdi_req_sname, tvb, offset+4,
                               8, fcwwn_to_str (tvb_get_ptr (tvb, offset+4, 8)));

        /* 12 is the length of the initial header and 4 is the size of each
         * domain request record.
         */
        numrec = (plen - 12)/4;
        offset = 12;
        for (i = 0; i < numrec; i++) {
            if (isreq) {
                proto_tree_add_text (rdi_tree, tvb, offset+3, 1,
                                     "Requested Domain ID: %d",
                                     tvb_get_guint8 (tvb, offset+3));
            }
            else {
                proto_tree_add_text (rdi_tree, tvb, offset+3, 1,
                                     "Granted Domain ID: %d",
                                     tvb_get_guint8 (tvb, offset+3));
            }
            offset += 4;
        }
    }
}

static void
dissect_swils_fspf_hdr (tvbuff_t *tvb, proto_tree *tree, int offset)
{
    proto_item *subti;
    proto_tree *fspfh_tree;
    
    if (tree) {
        /* 20 is the size of FSPF header */
        subti = proto_tree_add_text (tree, tvb, offset, 20, "FSPF Header");
        fspfh_tree = proto_item_add_subtree (subti, ett_fcswils_fspfhdr);

        proto_tree_add_item (fspfh_tree, hf_swils_fspfh_rev, tvb, offset+4,
                             1, 0);
        proto_tree_add_item (fspfh_tree, hf_swils_fspfh_ar_num, tvb,
                             offset+5, 1, 0);
        proto_tree_add_item (fspfh_tree, hf_swils_fspfh_auth_type, tvb,
                             offset+6, 1, 0);
        proto_tree_add_item (fspfh_tree, hf_swils_fspfh_dom_id, tvb, offset+11,
                             1, 0);
        proto_tree_add_item (fspfh_tree, hf_swils_fspfh_auth, tvb, offset+12,
                             8, 0);
    }
}

static void
dissect_swils_fspf_lsrechdr (tvbuff_t *tvb, proto_tree *tree, int offset)
{
    proto_tree_add_item (tree, hf_swils_lsrh_lsr_type, tvb, offset, 1, 0);
    proto_tree_add_text (tree, tvb, offset+2, 2, "LSR Age: %d secs",
                         tvb_get_ntohs (tvb, offset+2));
    proto_tree_add_text (tree, tvb, offset+4, 4, "Options : 0x%x",
                         tvb_get_ntohl (tvb, offset+4));
    proto_tree_add_item (tree, hf_swils_lsrh_lsid, tvb, offset+11, 1, 0);
    proto_tree_add_item (tree, hf_swils_lsrh_adv_domid, tvb, offset+15, 1, 0);
    proto_tree_add_item (tree, hf_swils_lsrh_ls_incid, tvb, offset+16, 4, 0);
    proto_tree_add_text (tree, tvb, offset+20, 2, "Checksum: 0x%x",
                         tvb_get_ntohs (tvb, offset+20));
    proto_tree_add_text (tree, tvb, offset+22, 2, "LSR Length: %d",
                         tvb_get_ntohs (tvb, offset+22));
}

static void
dissect_swils_fspf_ldrec (tvbuff_t *tvb, proto_tree *tree, int offset)
{
    proto_tree_add_string (tree, hf_swils_ldrec_linkid, tvb, offset, 4,
                           fc_to_str (tvb_get_ptr (tvb, offset+1, 3)));
    proto_tree_add_item (tree, hf_swils_ldrec_out_pidx, tvb, offset+5, 3, 0);
    proto_tree_add_item (tree, hf_swils_ldrec_nbr_pidx, tvb, offset+9, 3, 0);
    proto_tree_add_item (tree, hf_swils_ldrec_link_type, tvb, offset+12, 1, 0);
    proto_tree_add_item (tree, hf_swils_ldrec_link_cost, tvb, offset+14, 2, 0);
}

static void
dissect_swils_fspf_lsrec (tvbuff_t *tvb, proto_tree *tree, int offset,
                          int num_lsrec)
{
    int i, j, num_ldrec;
    proto_item *subti1, *subti;
    proto_tree *lsrec_tree, *ldrec_tree, *lsrechdr_tree;

    if (tree) {
        for (j = 0; j < num_lsrec; j++) {
            num_ldrec = tvb_get_ntohs (tvb, offset+26); 
            subti = proto_tree_add_text (tree, tvb, offset, (28+num_ldrec*16),
                                         "Link State Record %d (Domain %d)", j,
                                         tvb_get_guint8 (tvb, offset+15));
            lsrec_tree = proto_item_add_subtree (subti, ett_fcswils_lsrec);
        
            subti = proto_tree_add_text (lsrec_tree, tvb, offset, 24,
                                         "Link State Record Header");
            lsrechdr_tree = proto_item_add_subtree (subti,
                                                    ett_fcswils_lsrechdr); 
        
            dissect_swils_fspf_lsrechdr (tvb, lsrechdr_tree, offset);
            proto_tree_add_text (tree, tvb, offset+26, 2, "Number of Links: %d",
                                 num_ldrec);
            offset += 28;
        
            for (i = 0; i < num_ldrec; i++) {
                subti1 = proto_tree_add_text (lsrec_tree, tvb, offset, 16,
                                              "Link Descriptor %d "
                                              "(Neighbor domain %d)", i,
                                              tvb_get_guint8 (tvb, offset+3));
                ldrec_tree = proto_item_add_subtree (subti1, ett_fcswils_ldrec);
                dissect_swils_fspf_ldrec (tvb, ldrec_tree, offset);
                offset += 16;
            }
        }
    }
}

static void
dissect_swils_hello (tvbuff_t *tvb, proto_tree *hlo_tree, guint8 isreq _U_)
{
    /* Set up structures needed to add the protocol subtree and manage it */
    int offset = 0;

    if (hlo_tree) {
        dissect_swils_fspf_hdr (tvb, hlo_tree, offset);

        proto_tree_add_item (hlo_tree, hf_swils_hlo_options, tvb, offset+20, 4, 0);
        proto_tree_add_item (hlo_tree, hf_swils_hlo_hloint, tvb, offset+24, 4, 0);
        proto_tree_add_item (hlo_tree, hf_swils_hlo_deadint, tvb, offset+28, 4, 0);
        proto_tree_add_item (hlo_tree, hf_swils_hlo_rcv_domid, tvb, offset+35, 1, 0);
        proto_tree_add_item (hlo_tree, hf_swils_hlo_orig_pidx, tvb, offset+37, 3, 0);
    }
}

static void
dissect_swils_lsupdate (tvbuff_t *tvb, proto_tree *lsu_tree, guint8 isreq _U_)
{
    /* Set up structures needed to add the protocol subtree and manage it */
    int offset = 0;
    int num_lsrec;

    if (lsu_tree) {
        dissect_swils_fspf_hdr (tvb, lsu_tree, offset);

        proto_tree_add_text (lsu_tree, tvb, offset+23, 1, "Flags : %s",
                             val_to_str (tvb_get_guint8 (tvb, offset+23),
                                         fc_swils_fspf_lsrflags_val, "0x%x"));
        num_lsrec = tvb_get_ntohl (tvb, offset+24);

        proto_tree_add_text (lsu_tree, tvb, offset+24, 4, "Num of LSRs: %d",
                             num_lsrec);

        offset = 28;
        dissect_swils_fspf_lsrec (tvb, lsu_tree, offset, num_lsrec);
    }
}

static void
dissect_swils_lsack (tvbuff_t *tvb, proto_tree *lsa_tree, guint8 isreq _U_)
{
    /* Set up structures needed to add the protocol subtree and manage it */
    int offset = 0;
    int num_lsrechdr, i;
    proto_item *subti;
    proto_tree *lsrechdr_tree;

    if (lsa_tree) {
        dissect_swils_fspf_hdr (tvb, lsa_tree, offset);

        proto_tree_add_text (lsa_tree, tvb, offset+23, 1, "Flags : %s",
                             val_to_str (tvb_get_guint8 (tvb, offset+23),
                                         fc_swils_fspf_lsrflags_val, "0x%x"));
        num_lsrechdr = tvb_get_ntohl (tvb, offset+24);

        proto_tree_add_text (lsa_tree, tvb, offset+24, 4, "Num of LSR Headers: %d",
                             num_lsrechdr);

        offset = 28;

        for (i = 0; i < num_lsrechdr; i++) {
            subti = proto_tree_add_text (lsa_tree, tvb, offset, 24,
                                         "Link State Record Header (Domain %d)",
                                         tvb_get_guint8 (tvb, offset+15));
            lsrechdr_tree = proto_item_add_subtree (subti,
                                                    ett_fcswils_lsrechdr); 
            dissect_swils_fspf_lsrechdr (tvb, lsrechdr_tree, offset);
            offset += 24;
        }
    }
}

static void
dissect_swils_rscn (tvbuff_t *tvb, proto_tree *rscn_tree, guint8 isreq)
{
    /* Set up structures needed to add the protocol subtree and manage it */
    int offset = 0;
    proto_tree *dev_tree;
    int addrfmt, evtype;
    int numrec, i;
    proto_item *subti;
    
    if (rscn_tree) {
        if (!isreq)
            return;
        
        evtype = tvb_get_guint8 (tvb, offset+4);
        addrfmt = evtype & 0x0F;
        evtype = evtype >> 4;

        proto_tree_add_item (rscn_tree, hf_swils_rscn_evtype, tvb, offset+4,
                             1, 0);
        proto_tree_add_item (rscn_tree, hf_swils_rscn_addrfmt, tvb, offset+4,
                             1, 0);
        proto_tree_add_string (rscn_tree, hf_swils_rscn_affectedport, tvb,
                               offset+5, 3, fc_to_str (tvb_get_ptr (tvb,
                                                                    offset+5, 3)));
        proto_tree_add_item (rscn_tree, hf_swils_rscn_detectfn, tvb,
                             offset+8, 4, 0);
        numrec = tvb_get_ntohl (tvb, offset+12);
        
        if (!tvb_bytes_exist (tvb, offset+16, FC_SWILS_RSCN_DEVENTRY_SIZE*numrec)) {
            /* Some older devices do not include device entry information. */
            return;
        }
        
        proto_tree_add_text (rscn_tree, tvb, offset+12, 4, "Num Entries: %d",
                             numrec);

        offset = 16;
        for (i = 0; i < numrec; i++) {
            subti = proto_tree_add_text (rscn_tree, tvb, offset, 20,
                                         "Device Entry %d", i);
            dev_tree = proto_item_add_subtree (rscn_tree, ett_fcswils_rscn_dev);

            proto_tree_add_item (dev_tree, hf_swils_rscn_portstate, tvb, offset, 1, 0);
            proto_tree_add_string (dev_tree, hf_swils_rscn_portid, tvb, offset+1, 3,
                                   fc_to_str (tvb_get_ptr (tvb, offset+1, 3)));
            proto_tree_add_string (dev_tree, hf_swils_rscn_pwwn, tvb, offset+4, 8,
                                   fcwwn_to_str (tvb_get_ptr (tvb, offset+4, 8)));
            proto_tree_add_string (dev_tree, hf_swils_rscn_nwwn, tvb, offset+12, 8,
                                   fcwwn_to_str (tvb_get_ptr (tvb, offset+12, 8)));
            offset += 20;
        }
    }
}

/*
 * Merge Request contains zoning objects organized in the following format:
 *
 * Zone Set Object
 *      |
 *      +---------------- Zone Object
 *      |                      |
 *      +--                    +---------------- Zone Member
 *      |                      |                     |
 *      +--                    +----                 +-----
 *
 * So the decoding of the zone merge request is based on this structure
 */

static void
dissect_swils_zone_mbr (tvbuff_t *tvb, proto_tree *zmbr_tree, int offset)
{
    int mbrlen = 4 + tvb_get_guint8 (tvb, offset+3);

    proto_tree_add_item (zmbr_tree, hf_swils_zone_mbrtype, tvb,
                         offset, 1, 0);
    proto_tree_add_text (zmbr_tree, tvb, offset+2, 1, "Flags: 0x%x",
                         tvb_get_guint8 (tvb, offset+2));
    proto_tree_add_text (zmbr_tree, tvb, offset+3, 1,
                         "Identifier Length: %d",
                         tvb_get_guint8 (tvb, offset+3));
    switch (tvb_get_guint8 (tvb, offset)) {
    case FC_SWILS_ZONEMBR_WWN:
        proto_tree_add_string (zmbr_tree, hf_swils_zone_mbrid, tvb,
                               offset+4, 8,
                               fcwwn_to_str (tvb_get_ptr (tvb,
                                                          offset+4,
                                                          8)));
        break;
    case FC_SWILS_ZONEMBR_DP:
        proto_tree_add_string_format (zmbr_tree,
                                      hf_swils_zone_mbrid,
                                      tvb, offset+4, 4, " ",
                                      "0x%x",
                                      tvb_get_ntohl (tvb,
                                                     offset+4));
        break;
    case FC_SWILS_ZONEMBR_FCID:
        proto_tree_add_string (zmbr_tree, hf_swils_zone_mbrid, tvb,
                               offset+4, 4,
                               fc_to_str (tvb_get_ptr (tvb,
                                                       offset+5,
                                                       3)));
        break;
    case FC_SWILS_ZONEMBR_ALIAS:
        proto_tree_add_string (zmbr_tree, hf_swils_zone_mbrid, tvb,
                               offset+4,
                               tvb_get_guint8 (tvb, offset+3),
                               zonenm_to_str (tvb, offset+4));
        break;
    case FC_SWILS_ZONEMBR_WWN_LUN:
        proto_tree_add_string (zmbr_tree, hf_swils_zone_mbrid, tvb,
                               offset+4, 8,
                               fcwwn_to_str (tvb_get_ptr (tvb,
                                                          offset+4,
                                                          8)));
        proto_tree_add_item (zmbr_tree, hf_swils_zone_mbrid_lun, tvb,
                             offset+12, 8, 0);
        break;
    case FC_SWILS_ZONEMBR_DP_LUN:
        proto_tree_add_string_format (zmbr_tree,
                                      hf_swils_zone_mbrid,
                                      tvb, offset+4, 4, " ",
                                      "0x%x",
                                      tvb_get_ntohl (tvb,
                                                     offset+4));
        proto_tree_add_item (zmbr_tree, hf_swils_zone_mbrid_lun, tvb,
                             offset+8, 8, 0);
        break;
    case FC_SWILS_ZONEMBR_FCID_LUN:
        proto_tree_add_string (zmbr_tree, hf_swils_zone_mbrid, tvb,
                               offset+4, 4,
                               fc_to_str (tvb_get_ptr (tvb,
                                                       offset+5,
                                                       3)));
        proto_tree_add_item (zmbr_tree, hf_swils_zone_mbrid_lun, tvb,
                             offset+8, 8, 0);
        break;
    default:
        proto_tree_add_string (zmbr_tree, hf_swils_zone_mbrid, tvb,
                               offset+4, mbrlen,
                               "Unknown member type format");
            
    }
}

static void
dissect_swils_zone_obj (tvbuff_t *tvb, proto_tree *zobj_tree, int offset)
{
    proto_tree *zmbr_tree;
    int mbrlen, numrec, i, objtype;
    proto_item *subti;

    objtype = tvb_get_guint8 (tvb, offset);
    
    proto_tree_add_item (zobj_tree, hf_swils_zone_objtype, tvb, offset,
                         1, 0);
    proto_tree_add_item (zobj_tree, hf_swils_zone_protocol, tvb,
                         offset+1, 1, 0);
    proto_tree_add_string (zobj_tree, hf_swils_zone_objname, tvb,
                           offset+4, ZONENAME_LEN (tvb, offset+4),
                           zonenm_to_str (tvb, offset+4));

    numrec = tvb_get_ntohl (tvb, offset+4+ZONENAME_LEN (tvb, offset+4));
    proto_tree_add_text (zobj_tree, tvb,
                         offset+4+ZONENAME_LEN (tvb, offset+4), 4,
                         "Number of Zone Members: %d", numrec);

    offset += 8 + ZONENAME_LEN (tvb, offset+4);
    for (i = 0; i < numrec; i++) {
        if (objtype == FC_SWILS_ZONEOBJ_ZONESET) {
            dissect_swils_zone_obj (tvb, zobj_tree, offset);
            offset += get_zoneobj_len (tvb, offset);
        }
        else {
            mbrlen = 4 + tvb_get_guint8 (tvb, offset+3);
            subti = proto_tree_add_text (zobj_tree, tvb, offset, mbrlen,
                                         "Zone Member %d", i);
            zmbr_tree = proto_item_add_subtree (zobj_tree,
                                                ett_fcswils_zonembr);
            dissect_swils_zone_mbr (tvb, zmbr_tree, offset);
            offset += mbrlen;
        }
    }
}

static void
dissect_swils_mergereq (tvbuff_t *tvb, proto_tree *mr_tree, guint8 isreq)
{
    /* Set up structures needed to add the protocol subtree and manage it */
    int offset = 0;
    proto_tree *zobjlist_tree, *zobj_tree;
    int numrec, i, zonesetlen, objlistlen, objlen;
    proto_item *subti;

    if (mr_tree) {
        if (isreq) {
            /* zonesetlen is the size of the zoneset including the zone name */ 
            zonesetlen = tvb_get_ntohs (tvb, offset+2);
            proto_tree_add_text (mr_tree, tvb, offset+2, 2,
                                 "Active ZoneSet Length: %d", zonesetlen);

            if (zonesetlen) {
                proto_tree_add_string (mr_tree, hf_swils_zone_activezonenm, tvb,
                                       offset+4, ZONENAME_LEN (tvb, offset+4),
                                       zonenm_to_str (tvb, offset+4));
                
                /* objlistlen gives the size of the active zoneset object list */ 
                objlistlen = zonesetlen - ZONENAME_LEN (tvb, offset+4);
                /* Offset = start of the active zoneset zoning object list */
                offset = offset + (4 + ZONENAME_LEN (tvb, offset+4));
                numrec = tvb_get_ntohl (tvb, offset);
                
                subti = proto_tree_add_text (mr_tree, tvb, offset, objlistlen,
                                             "Active Zone Set");
                zobjlist_tree = proto_item_add_subtree (subti,
                                                        ett_fcswils_zoneobjlist);
                
                proto_tree_add_text (zobjlist_tree, tvb, offset, 4,
                                     "Number of zoning objects: %d", numrec);

                offset += 4;
                for (i = 0; i < numrec; i++) {
                    objlen = get_zoneobj_len (tvb, offset);
                    subti = proto_tree_add_text (zobjlist_tree, tvb, offset+4,
                                                 objlen, "Zone Object %d", i);
                    zobj_tree = proto_item_add_subtree (subti, ett_fcswils_zoneobj);
                    dissect_swils_zone_obj (tvb, zobj_tree, offset);
                    offset += objlen;
                }
            }
            else {
                offset += 4;
            }
            
            zonesetlen = tvb_get_ntohl (tvb, offset);
            proto_tree_add_text (mr_tree, tvb, offset, 4,
                                 "Full Zone Set Length: %d", zonesetlen);

            if (zonesetlen) {
                objlistlen = zonesetlen;
                /* Offset = start of the active zoneset zoning object list */
                offset += 4;
                numrec = tvb_get_ntohl (tvb, offset);
                
                subti = proto_tree_add_text (mr_tree, tvb, offset, objlistlen,
                                             "Full Zone Set");
                
                zobjlist_tree = proto_item_add_subtree (subti,
                                                        ett_fcswils_zoneobjlist);
                proto_tree_add_text (zobjlist_tree, tvb, offset, 4,
                                     "Number of zoning objects: %d", numrec);
                offset += 4;
                for (i = 0; i < numrec; i++) {
                    objlen = get_zoneobj_len (tvb, offset);
                    subti = proto_tree_add_text (zobjlist_tree, tvb, offset,
                                                 objlen, "Zone Object %d", i);
                    zobj_tree = proto_item_add_subtree (subti, ett_fcswils_zoneobj);
                    dissect_swils_zone_obj (tvb, zobj_tree, offset);
                    offset += objlen;
                }
            }
        }
        else {
            proto_tree_add_item (mr_tree, hf_swils_zone_status, tvb,
                                 offset+5, 1, 0);
            proto_tree_add_item (mr_tree, hf_swils_zone_reason, tvb,
                                 offset+6, 1, 0);
            proto_tree_add_text (mr_tree, tvb, offset+7, 1,
                                 "Vendor Unique: 0x%x",
                                 tvb_get_guint8 (tvb, offset+7));
        }
    }
}

static void
dissect_swils_aca (tvbuff_t *tvb, proto_tree *aca_tree, guint8 isreq)
{
    /* Set up structures needed to add the protocol subtree and manage it */
    int offset = 0;
    int numrec, plen, i;

    if (aca_tree) {
        if (isreq) {
            plen = tvb_get_ntohs (tvb, offset+2);
            proto_tree_add_text (aca_tree, tvb, offset+2, 2,
                                 "Domain ID List Length: %d", plen);
            numrec = plen/4;
            offset = 4;
        
            for (i = 0; i < numrec; i++) {
                proto_tree_add_uint_format (aca_tree, hf_swils_aca_domainid,
                                            tvb, offset+3, 1, 
                                            tvb_get_guint8 (tvb, offset+3),
                                            "Domain ID %d: %d", i,
                                            tvb_get_guint8 (tvb, offset+3));
                offset += 4;
            }
        }
        else {
            proto_tree_add_item (aca_tree, hf_swils_zone_status, tvb,
                                 offset+5, 1, 0);
            proto_tree_add_item (aca_tree, hf_swils_zone_reason, tvb,
                                 offset+6, 1, 0);
            proto_tree_add_text (aca_tree, tvb, offset+7, 1,
                                 "Vendor Unique: 0x%x",
                                 tvb_get_guint8 (tvb, offset+7));
        }
    }
}

static void
dissect_swils_rca (tvbuff_t *tvb, proto_tree *rca_tree, guint8 isreq)
{
    /* Set up structures needed to add the protocol subtree and manage it */
    int offset = 0;

    if (rca_tree) {
        if (!isreq) {
            proto_tree_add_item (rca_tree, hf_swils_zone_status, tvb,
                                 offset+5, 1, 0);
            proto_tree_add_item (rca_tree, hf_swils_zone_reason, tvb,
                                 offset+6, 1, 0);
            proto_tree_add_text (rca_tree, tvb, offset+7, 1,
                                 "Vendor Unique: 0x%x",
                                 tvb_get_guint8 (tvb, offset+7));
        }
    }
}

static void
dissect_swils_sfc (tvbuff_t *tvb, proto_tree *sfc_tree, guint8 isreq)
{
    /* Set up structures needed to add the protocol subtree and manage it */
    int offset = 0;
    proto_tree *zobjlist_tree, *zobj_tree;
    int numrec, i, zonesetlen, objlistlen, objlen;
    proto_item *subti;

    if (sfc_tree) {
        if (isreq) {
            proto_tree_add_item (sfc_tree, hf_swils_sfc_opcode, tvb, offset+1, 1, 0);

            zonesetlen = tvb_get_ntohs (tvb, offset+2);
            proto_tree_add_text (sfc_tree, tvb, offset+2, 2,
                                 "ZoneSet Length: %d", zonesetlen);

            if (zonesetlen) {
                proto_tree_add_string (sfc_tree, hf_swils_sfc_zonenm, tvb,
                                       offset+4, ZONENAME_LEN (tvb, offset+4),
                                       zonenm_to_str (tvb, offset+4));
                
                /* objlistlen gives the size of the active zoneset object list */ 
                objlistlen = zonesetlen - ZONENAME_LEN (tvb, offset+4);
                /* Offset = start of the active zoneset zoning object list */
                offset = offset + (4 + ZONENAME_LEN (tvb, offset+4));
                numrec = tvb_get_ntohl (tvb, offset);
                
                subti = proto_tree_add_text (sfc_tree, tvb, offset, objlistlen,
                                             "Zone Set");
                zobjlist_tree = proto_item_add_subtree (subti,
                                                        ett_fcswils_zoneobjlist);
                
                proto_tree_add_text (zobjlist_tree, tvb, offset, 4,
                                     "Number of zoning objects: %d", numrec);

                offset += 4;
                for (i = 0; i < numrec; i++) {
                    objlen = get_zoneobj_len (tvb, offset);
                    subti = proto_tree_add_text (zobjlist_tree, tvb, offset,
                                                 objlen, "Zone Object %d", i);
                    zobj_tree = proto_item_add_subtree (subti, ett_fcswils_zoneobj);
                    dissect_swils_zone_obj (tvb, zobj_tree, offset);
                    offset += objlen;
                }
            }
            else {
                offset += 4;
            }

            zonesetlen = tvb_get_ntohl (tvb, offset);
            proto_tree_add_text (sfc_tree, tvb, offset, 4,
                                 "Full Zone Set Length: %d", zonesetlen);

            if (zonesetlen) {
                objlistlen = zonesetlen;
                /* Offset = start of the active zoneset zoning object list */
                offset += 4;
                numrec = tvb_get_ntohl (tvb, offset);
                
                subti = proto_tree_add_text (sfc_tree, tvb, offset, objlistlen,
                                             "Full Zone Set");
                
                zobjlist_tree = proto_item_add_subtree (subti,
                                                        ett_fcswils_zoneobjlist);
                proto_tree_add_text (zobjlist_tree, tvb, offset, 4,
                                     "Number of zoning objects: %d", numrec);
                offset += 4;
                for (i = 0; i < numrec; i++) {
                    objlen = get_zoneobj_len (tvb, offset);
                    subti = proto_tree_add_text (zobjlist_tree, tvb, offset,
                                                 objlen, "Zone Object %d", i);
                    zobj_tree = proto_item_add_subtree (subti, ett_fcswils_zoneobj);
                    dissect_swils_zone_obj (tvb, zobj_tree, offset);
                    offset += objlen;
                }
            }
        }
        else {
            proto_tree_add_item (sfc_tree, hf_swils_zone_status, tvb,
                                 offset+5, 1, 0);
            proto_tree_add_item (sfc_tree, hf_swils_zone_reason, tvb,
                                 offset+6, 1, 0);
            proto_tree_add_text (sfc_tree, tvb, offset+7, 1,
                                 "Vendor Unique: 0x%x",
                                 tvb_get_guint8 (tvb, offset+7));
        }
    }
}

static void
dissect_swils_ufc (tvbuff_t *tvb, proto_tree *ufc_tree, guint8 isreq)
{
    /* Set up structures needed to add the protocol subtree and manage it */
    int offset = 0;

    if (ufc_tree) {
        if (!isreq) {
            proto_tree_add_item (ufc_tree, hf_swils_zone_status, tvb,
                                 offset+5, 1, 0);
            proto_tree_add_item (ufc_tree, hf_swils_zone_reason, tvb,
                                 offset+6, 1, 0);
            proto_tree_add_text (ufc_tree, tvb, offset+7, 1,
                                 "Vendor Unique: 0x%x",
                                 tvb_get_guint8 (tvb, offset+7));
        }
    }
}

static void
dissect_swils_esc (tvbuff_t *tvb, proto_tree *esc_tree, guint8 isreq)
{
    /* Set up structures needed to add the protocol subtree and manage it */
    int offset = 0;
    int i, numrec, plen;
    proto_tree *pdesc_tree;
    proto_item *subti;

    if (esc_tree) {
        if (isreq) {
            plen = tvb_get_ntohs (tvb, offset+2);
            proto_tree_add_text (esc_tree, tvb, offset+2, 2,
                                 "Payload Length: %d", plen);
            proto_tree_add_item (esc_tree, hf_swils_esc_swvendorid, tvb,
                                 offset+4, 8, 0);
            numrec = (plen - 12)/12;
            offset = 12;

            for (i = 0; i < numrec; i++) {
                subti = proto_tree_add_text (esc_tree, tvb, offset, 12,
                                             "Protocol Descriptor %d", i);
                pdesc_tree = proto_item_add_subtree (subti,
                                                     ett_fcswils_esc_pdesc);
                proto_tree_add_item (pdesc_tree, hf_swils_esc_pdesc_vendorid, tvb,
                                     offset, 8, 0);
                proto_tree_add_item (pdesc_tree, hf_swils_esc_protocolid,
                                     tvb, offset+10, 2, 0);
                offset += 12;
            }
        }
        else {
            proto_tree_add_item (esc_tree, hf_swils_esc_swvendorid, tvb,
                                 offset+4, 8, 0);
            subti = proto_tree_add_text (esc_tree, tvb, offset+12, 12,
                                         "Accepted Protocol Descriptor");
            pdesc_tree = proto_item_add_subtree (subti, ett_fcswils_esc_pdesc);

            proto_tree_add_item (pdesc_tree, hf_swils_esc_pdesc_vendorid, tvb,
                                 offset+12, 8, 0);
            proto_tree_add_item (pdesc_tree, hf_swils_esc_protocolid,
                                 tvb, offset+22, 2, 0);
        }
    }
}

static void
dissect_swils_drlir (tvbuff_t *tvb _U_, proto_tree *drlir_tree _U_,
                     guint8 isreq _U_)
{
    /* Set up structures needed to add the protocol subtree and manage it */
    return;
}

static void
dissect_swils_swrjt (tvbuff_t *tvb, proto_tree *swrjt_tree)
{
    /* Set up structures needed to add the protocol subtree and manage it */
    int offset = 0;

    if (swrjt_tree) {
        proto_tree_add_item (swrjt_tree, hf_swils_rjt, tvb, offset+5, 1, 0);
        proto_tree_add_item (swrjt_tree, hf_swils_rjtdet, tvb, offset+6, 1, 0);
        proto_tree_add_item (swrjt_tree, hf_swils_rjtvendor, tvb, offset+7,
                             1, 0);
    }
}

/* Code to actually dissect the packets */
static void
dissect_fcswils (tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
    proto_item *ti = NULL;
    guint8 opcode,
        failed_opcode = 0;
    int offset = 0;
    conversation_t *conversation;
    fcswils_conv_data_t *cdata;
    fcswils_conv_key_t ckey, *req_key;
    proto_tree *swils_tree = NULL;
    guint8 isreq = FC_SWILS_REQ;
    tvbuff_t *next_tvb;

    /* Make entries in Protocol column and Info column on summary display */
    if (check_col(pinfo->cinfo, COL_PROTOCOL)) 
        col_set_str(pinfo->cinfo, COL_PROTOCOL, "SW_ILS");

    /* decoding of this is done by each individual opcode handler */
    opcode = tvb_get_guint8 (tvb, 0);
    
    if (tree) {
        ti = proto_tree_add_protocol_format (tree, proto_fcswils, tvb, 0,
                                             tvb_length (tvb), "SW_ILS");
        swils_tree = proto_item_add_subtree (ti, ett_fcswils);
    }

    /* Register conversation if this is not a response */
    if ((opcode != FC_SWILS_SWACC) && (opcode != FC_SWILS_SWRJT)) {
        conversation = find_conversation (&pinfo->src, &pinfo->dst,
                                          pinfo->ptype, pinfo->oxid,
                                          pinfo->rxid, NO_PORT2);
        if (!conversation) {
            conversation = conversation_new (&pinfo->src, &pinfo->dst,
                                             pinfo->ptype, pinfo->oxid,
                                             pinfo->rxid, NO_PORT2);
        }
    
        ckey.conv_idx = conversation->index;
        
        cdata = (fcswils_conv_data_t *)g_hash_table_lookup (fcswils_req_hash,
                                                            &ckey);
        if (cdata) {
            /* Since we never free the memory used by an exchange, this maybe a
             * case of another request using the same exchange as a previous
             * req. 
             */
            cdata->opcode = opcode;
        }
        else {
            req_key = g_mem_chunk_alloc (fcswils_req_keys);
            req_key->conv_idx = conversation->index;
            
            cdata = g_mem_chunk_alloc (fcswils_req_vals);
            cdata->opcode = opcode;
            
            g_hash_table_insert (fcswils_req_hash, req_key, cdata);
        }
    }
    else {
        /* Opcode is ACC or RJT */
        conversation = find_conversation (&pinfo->src, &pinfo->dst,
                                          pinfo->ptype, pinfo->oxid,
                                          pinfo->rxid, NO_PORT2);
        isreq = FC_SWILS_RPLY;
        if (!conversation) {
            if (tree && (opcode == FC_SWILS_SWACC)) {
                /* No record of what this accept is for. Can't decode */
                proto_tree_add_text (swils_tree, tvb, 0, tvb_length (tvb),
                                     "No record of Exchg. Unable to decode SW_ACC");
                return;
            }
        }
        else {
            ckey.conv_idx = conversation->index;

            cdata = (fcswils_conv_data_t *)g_hash_table_lookup (fcswils_req_hash, &ckey);

            if (cdata != NULL) {
                if (opcode == FC_SWILS_SWACC)
                    opcode = cdata->opcode;
                else
                    failed_opcode = cdata->opcode;
            }
            
            if (tree) {
                if ((cdata == NULL) && (opcode != FC_SWILS_SWRJT)) {
                    /* No record of what this accept is for. Can't decode */
                    proto_tree_add_text (swils_tree, tvb, 0, tvb_length (tvb),
                                         "No record of SW_ILS Req. Unable to decode SW_ACC");
                    return;
                }
            }
        }
    }

    if (check_col (pinfo->cinfo, COL_INFO)) {
        if (isreq == FC_SWILS_REQ) {
            col_add_str (pinfo->cinfo, COL_INFO,
                         val_to_str (opcode, fc_swils_opcode_key_val, "0x%x"));
        }
        else if (opcode == FC_SWILS_SWRJT) {
            col_add_fstr (pinfo->cinfo, COL_INFO, "SW_RJT (%s)",
                          val_to_str (failed_opcode, fc_swils_opcode_key_val, "0x%x"));
        }
        else {
            col_add_fstr (pinfo->cinfo, COL_INFO, "SW_ACC (%s)",
                          val_to_str (opcode, fc_swils_opcode_key_val, "0x%x"));
        }
    }

    if (tree) {
        proto_tree_add_item (swils_tree, hf_swils_opcode, tvb, offset, 1, 0);
    }

    switch (opcode) {
    case FC_SWILS_SWRJT:
        dissect_swils_swrjt (tvb, swils_tree);
        break;
    case FC_SWILS_ELP:
        dissect_swils_elp (tvb, swils_tree, isreq);
        break;
    case FC_SWILS_EFP:
        dissect_swils_efp (tvb, swils_tree, isreq);
        break;
    case FC_SWILS_DIA:
        dissect_swils_dia (tvb, swils_tree, isreq);
        break;
    case FC_SWILS_RDI:
        dissect_swils_rdi (tvb, swils_tree, isreq);
        break;
    case FC_SWILS_HLO:
        dissect_swils_hello (tvb, swils_tree, isreq);
        break;
    case FC_SWILS_LSU:
        dissect_swils_lsupdate (tvb, swils_tree, isreq);
        break;
    case FC_SWILS_LSA:
        dissect_swils_lsack (tvb, swils_tree, isreq);
        break;
    case FC_SWILS_BF:
    case FC_SWILS_RCF:
        /* Nothing to be displayed for these two commands */
        break;
    case FC_SWILS_RSCN:
        dissect_swils_rscn (tvb, swils_tree, isreq);
        break;
    case FC_SWILS_DRLIR:
        dissect_swils_drlir (tvb, swils_tree, isreq);
        break;
    case FC_SWILS_MR:
        dissect_swils_mergereq (tvb, swils_tree, isreq);
        break;
    case FC_SWILS_ACA:
        dissect_swils_aca (tvb, swils_tree, isreq);
        break;
    case FC_SWILS_RCA:
        dissect_swils_rca (tvb, swils_tree, isreq);
        break;
    case FC_SWILS_SFC:
        dissect_swils_sfc (tvb, swils_tree, isreq);
        break;
    case FC_SWILS_UFC:
        dissect_swils_ufc (tvb, swils_tree, isreq);
        break;
    case FC_SWILS_ESC:
        dissect_swils_esc (tvb, swils_tree, isreq);
        break;
    case FC_SWILS_AUTH_ILS:
        if (isreq && fcsp_handle) 
            call_dissector (fcsp_handle, tvb, pinfo, swils_tree);
        break;
    default:
        next_tvb = tvb_new_subset (tvb, offset+4, -1, -1);
        call_dissector (data_handle, next_tvb, pinfo, tree);
    }

}

/* Register the protocol with Ethereal */

/* this format is require because a script is used to build the C function
   that calls all the protocol registration.
*/

void
proto_register_fcswils (void)
{
/* Setup list of header fields  See Section 1.6.1 for details*/
    static hf_register_info hf[] = {
        { &hf_swils_opcode,
          {"Cmd Code", "swils.opcode", FT_UINT8, BASE_HEX,
           VALS (fc_swils_opcode_key_val), 0x0, "", HFILL}},
        { &hf_swils_elp_rev,
          {"Revision", "swils.elp.rev", FT_UINT8, BASE_DEC, NULL,
           0x0, "", HFILL}},
        { &hf_swils_elp_flags,
          {"Flag", "swils.elp.flag", FT_BYTES, BASE_HEX, NULL, 0x0, "", HFILL}},
        { &hf_swils_elp_r_a_tov,
          {"R_A_TOV", "swils.elp.ratov", FT_UINT32, BASE_DEC, NULL,
           0x0, "", HFILL}},
        { &hf_swils_elp_e_d_tov,
          {"E_D_TOV", "swils.elp.edtov", FT_UINT32, BASE_DEC, NULL, 0x0, "",
           HFILL}},
        { &hf_swils_elp_req_epn,
          {"Req Eport Name", "swils.elp.reqepn", FT_STRING, BASE_HEX, NULL, 0x0,
           "", HFILL}},
        { &hf_swils_elp_req_esn,
          {"Req Switch Name", "swils.elp.reqesn", FT_STRING, BASE_HEX, NULL, 0x0,
           "", HFILL}},
        { &hf_swils_elp_clsf_svcp,
          {"Class F Svc Param", "swils.elp.clsfp", FT_BYTES, BASE_NONE, NULL, 0x0,
           "", HFILL}},
        { &hf_swils_elp_clsf_rcvsz,
          {"Max Class F Frame Size", "swils.elp.clsfrsz", FT_UINT16, BASE_DEC,
           NULL, 0x0, "", HFILL}},
        { &hf_swils_elp_clsf_conseq,
          {"Class F Max Concurrent Seq", "swils.elp.clsfcs", FT_UINT16, BASE_DEC,
           NULL, 0x0, "", HFILL}},
        { &hf_swils_elp_clsf_e2e,
          {"Class F E2E Credit", "swils.elp.cfe2e", FT_UINT16, BASE_DEC, NULL,
           0x0, "", HFILL}},
        { &hf_swils_elp_clsf_openseq,
          {"Class F Max Open Seq", "swils.elp.oseq", FT_UINT16, BASE_DEC, NULL,
           0x0, "", HFILL}},
        { &hf_swils_elp_cls1_svcp,
          {"Class 1 Svc Param", "swils.elp.cls1p", FT_BYTES, BASE_NONE, NULL, 0x0,
           "", HFILL}},
        { &hf_swils_elp_cls1_rcvsz,
          {"Class 1 Frame Size", "swils.elp.cls1rsz", FT_UINT16, BASE_DEC, NULL,
           0x0, "", HFILL}},
        { &hf_swils_elp_cls2_svcp,
          {"Class 2 Svc Param", "swils.elp.cls2p", FT_BYTES, BASE_HEX, NULL, 0x0,
           "", HFILL}},
        { &hf_swils_elp_cls2_rcvsz,
          {"Class 2 Frame Size", "swils.elp.cls1rsz", FT_UINT16, BASE_DEC, NULL,
           0x0, "", HFILL}},
        { &hf_swils_elp_cls3_svcp,
          {"Class 3 Svc Param", "swils.elp.cls3p", FT_BYTES, BASE_HEX, NULL, 0x0,
           "", HFILL}},
        { &hf_swils_elp_cls3_rcvsz,
          {"Class 3 Frame Size", "swils.elp.cls1rsz", FT_UINT16, BASE_DEC, NULL,
           0x0, "", HFILL}},
        { &hf_swils_elp_isl_fc_mode,
          {"ISL Flow Ctrl Mode", "swils.elp.fcmode", FT_STRING, BASE_HEX, NULL,
           0x0, "", HFILL}},
        { &hf_swils_elp_fcplen,
          {"Flow Ctrl Param Len", "swils.elp.fcplen", FT_UINT16, BASE_DEC, NULL,
           0x0, "", HFILL}},
        { &hf_swils_elp_b2bcredit,
          {"B2B Credit", "swils.elp.b2b", FT_UINT32, BASE_DEC, NULL, 0x0, "",
           HFILL}},
        { &hf_swils_elp_compat1,
          {"Compatability Param 1", "swils.elp.compat1", FT_UINT32, BASE_DEC, NULL,
           0x0, "", HFILL}},
        { &hf_swils_elp_compat2,
          {"Compatability Param 2", "swils.elp.compat2", FT_UINT32, BASE_DEC, NULL,
           0x0, "", HFILL}},
        { &hf_swils_elp_compat3,
          {"Compatability Param 3", "swils.elp.compat3", FT_UINT32, BASE_DEC, NULL,
           0x0, "", HFILL}},
        { &hf_swils_elp_compat4,
          {"Compatability Param 4", "swils.elp.compat4", FT_UINT32, BASE_DEC, NULL,
           0x0, "", HFILL}},
        { &hf_swils_efp_rec_type,
          {"Record Type", "swils.efp.rectype", FT_UINT8, BASE_HEX,
           VALS (fcswils_rectype_val), 0x0, "", HFILL}},
        { &hf_swils_efp_dom_id,
          {"Domain ID", "swils.efp.domid", FT_UINT8, BASE_HEX, NULL, 0x0,
           "", HFILL}},
        { &hf_swils_efp_switch_name,
          {"Switch Name", "swils.efp.sname", FT_STRING, BASE_HEX, NULL, 0x0,
           "", HFILL}},
        { &hf_swils_efp_mcast_grpno,
          {"Mcast Grp#", "swils.efp.mcastno", FT_UINT8, BASE_HEX, NULL, 0x0,
           "", HFILL}},
        { &hf_swils_efp_alias_token,
          {"Alias Token", "swils.efp.aliastok", FT_BYTES, BASE_HEX, NULL, 0x0,
           "", HFILL}},
        { &hf_swils_efp_payload_len,
          {"Payload Len", "swils.efp.payloadlen", FT_UINT16, BASE_DEC, NULL, 0x0,
           "", HFILL}},
        { &hf_swils_efp_pswitch_pri,
          {"Principal Switch Priority", "swils.efp.psprio", FT_UINT8, BASE_DEC,
           NULL, 0x0, "", HFILL}},
        { &hf_swils_efp_pswitch_name,
          {"Principal Switch Name", "swils.efp.psname", FT_STRING, BASE_HEX, NULL,
           0x0, "", HFILL}},
        { &hf_swils_dia_switch_name,
          {"Switch Name", "swils.dia.sname", FT_STRING, BASE_HEX, NULL, 0x0,
           "", HFILL}},
        { &hf_swils_rdi_payload_len,
          {"Payload Len", "swils.rdi.len", FT_UINT16, BASE_DEC, NULL, 0x0, "",
           HFILL}},
        { &hf_swils_rdi_req_sname,
          {"Req Switch Name", "swils.rdi.reqsn", FT_STRING, BASE_HEX, NULL, 0x0,
           "", HFILL}},
        { &hf_swils_fspfh_cmd,
          {"Command: ", "swils.fspf.cmd", FT_UINT8, BASE_HEX, NULL, 0x0,
           "", HFILL}},
        { &hf_swils_fspfh_rev,
          {"Version", "swils.fspf.ver", FT_UINT8, BASE_HEX, NULL, 0x0, "", HFILL}},
        { &hf_swils_fspfh_ar_num,
          {"AR Number", "swils.fspf.arnum", FT_UINT8, BASE_HEX, NULL, 0x0, "",
           HFILL}},
        { &hf_swils_fspfh_auth_type,
          {"Authentication Type", "swils.fspf.authtype", FT_UINT8, BASE_HEX, NULL,
           0x0, "", HFILL}},
        { &hf_swils_fspfh_dom_id,
          {"Originating Domain ID", "swils.fspf.origdomid", FT_UINT8, BASE_DEC,
           NULL, 0x0, "", HFILL}},
        { &hf_swils_fspfh_auth,
          {"Authentication", "swils.fspf.auth", FT_BYTES, BASE_HEX, NULL, 0x0,
           "", HFILL}},
        { &hf_swils_hlo_options,
          {"Options", "swils.hlo.options", FT_BYTES, BASE_HEX, NULL, 0x0,
           "", HFILL}},
        { &hf_swils_hlo_hloint,
          {"Hello Interval (secs)", "swils.hlo.hloint", FT_UINT32, BASE_DEC,
           NULL, 0x0, "", HFILL}},
        { &hf_swils_hlo_deadint,
          {"Dead Interval (secs)", "swils.hlo.deadint", FT_UINT32, BASE_DEC,
           NULL, 0x0, "", HFILL}},
        { &hf_swils_hlo_rcv_domid,
          {"Recipient Domain ID", "swils.hlo.rcvdomid", FT_UINT8, BASE_DEC,
           NULL, 0x0, "", HFILL}},
        { &hf_swils_hlo_orig_pidx,
          {"Originating Port Idx", "swils.hlo.origpidx", FT_UINT24, BASE_HEX,
           NULL, 0x0, "", HFILL}},
        { &hf_swils_lsrh_lsr_type,
          {"LSR Type", "swils.lsr.type", FT_UINT8, BASE_HEX,
           VALS (fc_swils_fspf_linkrec_val), 0x0, "", HFILL}},
        { &hf_swils_lsrh_lsid,
          {"Link State Id", "swils.ls.id", FT_UINT8, BASE_DEC, NULL, 0x0,
           "", HFILL}},
        { &hf_swils_lsrh_adv_domid,
          {"Advertising Domain Id", "swils.lsr.advdomid", FT_UINT8, BASE_DEC,
           NULL, 0x0, "", HFILL}},
        { &hf_swils_lsrh_ls_incid,
          {"LS Incarnation Number", "swils.lsr.incid", FT_UINT32, BASE_DEC,
           NULL, 0x0, "", HFILL}},
        { &hf_swils_ldrec_linkid,
          {"Link ID", "swils.ldr.linkid", FT_STRING, BASE_HEX, NULL, 0x0,
           "", HFILL}},
        { &hf_swils_ldrec_out_pidx,
          {"Output Port Idx", "swils.ldr.out_portidx", FT_UINT24, BASE_HEX, NULL,
           0x0, "", HFILL}},
        { &hf_swils_ldrec_nbr_pidx,
          {"Neighbor Port Idx", "swils.ldr.nbr_portidx", FT_UINT24, BASE_HEX,
           NULL, 0x0, "", HFILL}},
        { &hf_swils_ldrec_link_type,
          {"Link Type", "swils.ldr.linktype", FT_UINT8, BASE_HEX,
           VALS (fc_swils_link_type_val), 0x0, "", HFILL}},
        { &hf_swils_ldrec_link_cost,
          {"Link Cost", "swils.ldr.linkcost", FT_UINT16, BASE_DEC, NULL, 0x0,
           "", HFILL}},
        { &hf_swils_rscn_evtype,
          {"Event Type", "swils.rscn.evtype", FT_UINT8, BASE_DEC,
           VALS (fc_swils_rscn_portstate_val), 0xF0, "", HFILL}},
        { &hf_swils_rscn_addrfmt,
          {"Address Format", "swils.rscn.addrfmt", FT_UINT8, BASE_DEC, 
           VALS (fc_swils_rscn_addrfmt_val), 0x0F, "", HFILL}},
        { &hf_swils_rscn_affectedport,
          {"Affected Port ID", "swils.rscn.affectedport", FT_STRING, BASE_HEX,
           NULL, 0x0, "", HFILL}},
        { &hf_swils_rscn_detectfn,
          {"Detection Function", "swils.rscn.detectfn", FT_UINT32, BASE_HEX,
           VALS (fc_swils_rscn_detectfn_val), 0x0, "", HFILL}},
        { &hf_swils_rscn_portstate,
          {"Port State", "swils.rscn.portstate", FT_UINT8, BASE_HEX, NULL, 0x0,
           "", HFILL}},
        { &hf_swils_rscn_portid,
          {"Port Id", "swils.rscn.portid", FT_STRING, BASE_HEX, NULL, 0x0, "",
           HFILL}},
        { &hf_swils_rscn_pwwn,
          {"Port WWN", "swils.rscn.pwwn", FT_STRING, BASE_HEX, NULL, 0x0, "",
           HFILL}},
        { &hf_swils_rscn_nwwn,
          {"Node WWN", "swils.rscn.nwwn", FT_STRING, BASE_HEX, NULL, 0x0, "",
           HFILL}},
        { &hf_swils_esc_swvendorid,
          {"Switch Vendor ID", "swils.esc.swvendor", FT_STRING, BASE_HEX, NULL,
           0x0, "", HFILL}},
        { &hf_swils_esc_pdesc_vendorid,
          {"Vendor ID", "swils.esc.vendorid", FT_STRING, BASE_HEX, NULL, 0x0,
           "", HFILL}},
        { &hf_swils_esc_protocolid,
          {"Protocol ID", "swils.esc.protocol", FT_UINT16, BASE_HEX,
           VALS (fc_swils_esc_protocol_val), 0x0, "", HFILL}},
        { &hf_swils_zone_activezonenm,
          {"Active Zoneset Name", "swils.mr.activezonesetname", FT_STRING,
           BASE_HEX, NULL, 0x0, "", HFILL}},
        { &hf_swils_zone_objname,
          {"Zone Object Name", "swils.zone.zoneobjname", FT_STRING, BASE_HEX,
           NULL, 0x0, "", HFILL}},
        { &hf_swils_zone_objtype,
          {"Zone Object Type", "swils.zone.zoneobjtype", FT_UINT8, BASE_HEX,
           VALS (fc_swils_zoneobj_type_val), 0x0, "", HFILL}},
        { &hf_swils_zone_mbrtype,
          {"Zone Member Type", "swils.zone.mbrtype", FT_UINT8, BASE_HEX,
           VALS (fc_swils_zonembr_type_val), 0x0, "", HFILL}},
        { &hf_swils_zone_protocol,
          {"Zone Protocol", "swils.zone.protocol", FT_UINT8, BASE_HEX, NULL,
           0x0, "", HFILL}},
        { &hf_swils_zone_mbrid,
          {"Member Identifier", "swils.zone.mbrid", FT_STRING, BASE_HEX, NULL,
           0x0, "", HFILL}},
        { &hf_swils_zone_status,
          {"Zone Command Status", "swils.zone.status", FT_UINT8, BASE_HEX,
           VALS (fc_swils_mr_rsp_val), 0x0, "Applies to MR, ACA, RCA, SFC, UFC",
           HFILL}},
        { &hf_swils_zone_reason,
          {"Zone Command Reason Code", "swils.zone.reason", FT_UINT8, BASE_HEX,
           VALS (fc_swils_mr_reason_val), 0x0, "Applies to MR, ACA, RCA, SFC, UFC",
           HFILL}},
        { &hf_swils_aca_domainid,
          {"Known Domain ID", "swils.aca.domainid", FT_UINT8, BASE_HEX, NULL,
           0x0, "", HFILL}},
        { &hf_swils_sfc_opcode,
          {"Operation Request", "swils.sfc.opcode", FT_UINT8, BASE_HEX,
           VALS (fc_swils_sfc_op_val), 0x0, "", HFILL}},
        { &hf_swils_sfc_zonenm,
          {"Zone Set Name", "swils.sfc.zonename", FT_STRING, BASE_HEX, NULL,
           0x0, "", HFILL}},
        { &hf_swils_rjt,
          {"Reason Code", "swils.rjt.reason", FT_UINT8, BASE_HEX,
           VALS (fc_swils_rjt_val), 0x0, "", HFILL}},
        { &hf_swils_rjtdet,
          {"Reason Code Explanantion", "swils.rjt.reasonexpl", FT_UINT8,
           BASE_HEX, VALS (fc_swils_deterr_val), 0x0, "", HFILL}},
        { &hf_swils_rjtvendor,
          {"Vendor Unique Error Code", "swils.rjt.vendor", FT_UINT8, BASE_HEX,
           NULL, 0x0, "", HFILL}},
        { &hf_swils_zone_mbrid_lun,
          {"LUN", "swils.zone.lun", FT_BYTES, BASE_HEX, NULL, 0x0, "",
           HFILL}},
    };

    /* Setup protocol subtree array */
    static gint *ett[] = {
        &ett_fcswils,
        &ett_fcswils_swacc,
        &ett_fcswils_swrjt,
        &ett_fcswils_elp,
        &ett_fcswils_efp,
        &ett_fcswils_efplist,
        &ett_fcswils_dia,
        &ett_fcswils_rdi,
        &ett_fcswils_fspfhdr,
        &ett_fcswils_hlo,
        &ett_fcswils_lsrec,
        &ett_fcswils_lsrechdr,
        &ett_fcswils_ldrec,
        &ett_fcswils_lsu,
        &ett_fcswils_lsa,
        &ett_fcswils_bf,
        &ett_fcswils_rcf,
        &ett_fcswils_rscn,
        &ett_fcswils_rscn_dev,
        &ett_fcswils_drlir,
        &ett_fcswils_mr,
        &ett_fcswils_zoneobjlist,
        &ett_fcswils_zoneobj,
        &ett_fcswils_zonembr,
        &ett_fcswils_aca,
        &ett_fcswils_rca,
        &ett_fcswils_sfc,
        &ett_fcswils_ufc,
        &ett_fcswils_esc,
        &ett_fcswils_esc_pdesc,
    };

    /* Register the protocol name and description */
    proto_fcswils = proto_register_protocol("Fibre Channel SW_ILS", "FC-SWILS", "swils");

    /* Required function calls to register the header fields and subtrees used */
    proto_register_field_array(proto_fcswils, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));
    register_init_routine(&fcswils_init_protocol);
}

/* If this dissector uses sub-dissector registration add a registration routine.
   This format is required because a script is used to find these routines and
   create the code that calls these routines.
*/
void
proto_reg_handoff_fcswils (void)
{
    dissector_handle_t swils_handle;

    swils_handle = create_dissector_handle (dissect_fcswils, proto_fcswils);
    dissector_add("fc.ftype", FC_FTYPE_SWILS, swils_handle);

    data_handle = find_dissector ("data");
    fcsp_handle = find_dissector ("fcsp");
}


