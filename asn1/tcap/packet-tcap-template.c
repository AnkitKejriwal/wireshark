/* packet-tcap-template.c
 * Routines for ITU TCAP
 * Copyright 2004, Tim Endean <endeant@hotmail.com>
 * Built from the gsm-map dissector Copyright 2004, Anders Broman <anders.broman@ericsson.com>
 *
 * $Id$
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
 * References: ETSI 300 374
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glib.h>
#include <epan/packet.h>
#include <epan/prefs.h>
#include <epan/conversation.h>

#include <stdio.h>
#include <string.h>
#include <epan/asn1.h>
#include "packet-ber.h"
#include "packet-tcap.h"

#define PNAME  "tcap"
#define PSNAME "tcap"
#define PFNAME "tcap"

/* Initialize the protocol and registered fields */
int proto_tcap = -1;
static int hf_tcap_tag = -1;         
static int hf_tcap_length = -1;         
static int hf_tcap_data = -1;
#include "packet-tcap-hf.c"
static guint tcap_itu_ssn = 106;

static guint global_tcap_itu_ssn = 1;

/* Initialize the subtree pointers */
static gint ett_tcap = -1;
static gint ett_param = -1;

#include "packet-tcap-ett.c"

#define MAX_SSN 254
static range_t *global_ssn_range;
static range_t *ssn_range;

static packet_info *g_pinfo = NULL;
static proto_tree *g_tcap_tree = NULL;
static gboolean g_tcap_ends_def_len = FALSE;


dissector_handle_t	tcap_handle;
static dissector_table_t ber_oid_dissector_table=NULL;
static char * cur_oid;
static char * tcapext_oid;
static dissector_handle_t data_handle;
static dissector_table_t tcap_itu_ssn_dissector_table; /* map use ssn in sccp */
static dissector_table_t tcap_ansi_ssn_dissector_table; /* map use ssn in sccp */

static int dissect_tcap_param(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset);
static int dissect_tcap_UserInformation(gboolean implicit_tag _U_, tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree, int hf_index _U_);


#include "packet-tcap-fn.c"



const value_string tcap_component_type_str[] = {
    { TCAP_COMP_INVOKE,		"Invoke" },
    { TCAP_COMP_RRL,		"Return Result(L)" },
    { TCAP_COMP_RE,		"Return Error" },
    { TCAP_COMP_REJECT,		"Reject" },
    { TCAP_COMP_RRN,		"Return Result(NL)" },
    { 0,			NULL } };


static void
dissect_tcap(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree)
{
    proto_item		*item=NULL;
    proto_tree		*tree=NULL;


    if (check_col(pinfo->cinfo, COL_PROTOCOL))
    {
	col_set_str(pinfo->cinfo, COL_PROTOCOL, "tcap");
    }

    /* create display subtree for the protocol */
    if(parent_tree){
       item = proto_tree_add_item(parent_tree, proto_tcap, tvb, 0, -1, FALSE);
       tree = proto_item_add_subtree(item, ett_tcap);
    }
	cur_oid = NULL;
	tcapext_oid = NULL;
	pinfo->private_data = NULL;
    dissect_tcap_MessageType(FALSE, tvb, 0, pinfo, tree, -1);


}



/* Register the protocol with Ethereal */

void proto_reg_handoff_tcap(void);

/* this format is require because a script is used to build the C function
   that calls all the protocol registration.
*/
void
proto_register_tcap(void)
{

/* Setup list of header fields  See Section 1.6.1 for details*/
    static hf_register_info hf[] = {
	{ &hf_tcap_tag,
		{ "Tag",           "tcap.msgtype",
		FT_UINT8, BASE_HEX, NULL, 0,
		"", HFILL }
	},
	{ &hf_tcap_length,
		{ "Length", "tcap.len",
		FT_UINT8, BASE_HEX, NULL, 0,
		"", HFILL }
	},
	{ &hf_tcap_data,
		{ "Data", "tcap.data",
		FT_BYTES, BASE_HEX, NULL, 0,
		"", HFILL }
	},
#include "packet-tcap-hfarr.c"	
    };

/* Setup protocol subtree array */
    static gint *ett[] = {
	&ett_tcap,
	&ett_param,
	#include "packet-tcap-ettarr.c"
    };

    /*static enum_val_t tcap_options[] = {
	{ "itu", "ITU",  ITU_TCAP_STANDARD },
	{ "ansi", "ANSI", ANSI_TCAP_STANDARD },
	{ NULL, NULL, 0 }
    };*/

    module_t *tcap_module;

/* Register the protocol name and description */
    proto_tcap = proto_register_protocol("ITU Transaction Capabilities Application Part",
	"tcap", "tcap");

/* Required function calls to register the header fields and subtrees used */
    proto_register_field_array(proto_tcap, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));

    tcap_module = prefs_register_protocol(proto_tcap, proto_reg_handoff_tcap);

    /*prefs_register_enum_preference(tcap_module, "standard", "ITU TCAP standard",
	"The SS7 standard used in ITU TCAP packets",
	&tcap_standard, tcap_options, FALSE);*/

/*    prefs_register_bool_preference(tcap_module, "lock_info_col", "Lock Info column",
	"Always show TCAP in Info column",
	&lock_info_col);
*/
    /* Set default SSNs */
    range_convert_str(&global_ssn_range, "1-255", MAX_SSN);
    ssn_range = range_empty();

    prefs_register_range_preference(tcap_module, "ssn", "SCCP SSNs",
	"SCCP (and SUA) SSNs to decode as ITU TCAP",
	&global_ssn_range, MAX_SSN);

    /* we will fake a ssn subfield which has the same value obtained from sccp */
    tcap_itu_ssn_dissector_table = register_dissector_table("tcap.itu_ssn", "ITU TCAP SSN", FT_UINT8, BASE_DEC);
    tcap_ansi_ssn_dissector_table = register_dissector_table("tcap.ansi_ssn", "ANSI TCAP SSN", FT_UINT8, BASE_DEC);

	
}




/* If this dissector uses sub-dissector registration add a registration routine.
   This format is required because a script is used to find these routines and
   create the code that calls these routines.
*/

static void range_delete_callback(guint32 ssn)
{
    if (ssn) {
	dissector_delete("sccp.ssn", ssn, tcap_handle);
	dissector_delete("sua.ssn", ssn, tcap_handle);
    }
}

static void range_add_callback(guint32 ssn)
{
    if (ssn) {
	dissector_add("sccp.ssn", ssn, tcap_handle);
	dissector_add("sua.ssn", ssn, tcap_handle);
    }
}

void
proto_reg_handoff_tcap(void)
{
    static gboolean prefs_initialized = FALSE;

    if (!prefs_initialized) {

	tcap_handle = create_dissector_handle(dissect_tcap, proto_tcap);
	
	prefs_initialized = TRUE;

    } else {

	range_foreach(ssn_range, range_delete_callback);
    }

    g_free(ssn_range);
    ssn_range = range_copy(global_ssn_range);

    range_foreach(ssn_range, range_add_callback);
	register_ber_oid_name("0.0.17.773.1.1.1",
		"itu-t(0) recommendation(0) q(17) 773 as(1) dialogue-as(1) version1(1)");

}




/* some functions from packet_tcap which are needed by some other dissectors */

#define TC_DS_OK 1
#define TC_DS_FAIL 0


int
tcap_find_eoc(ASN1_SCK *asn1)
{
    guint	saved_offset;
    gint        prev_offset;
    guint	tag;
    guint	len;
    gboolean	def_len;

    saved_offset = asn1->offset;

    while (!asn1_eoc(asn1, -1))
    {
	prev_offset = asn1->offset;
	asn1_id_decode1(asn1, &tag);
	asn1_length_decode(asn1, &def_len, &len);

	if (def_len)
	{
	    asn1->offset += len;
	}
	else
	{
	    asn1->offset += tcap_find_eoc(asn1);
	    asn1_eoc_decode(asn1, -1);
	}
	if (prev_offset >= asn1->offset)
	    THROW(ReportedBoundsError);
    }

    len = asn1->offset - saved_offset;
    asn1->offset = saved_offset;

    return(len);
}

gboolean
tcap_check_tag(ASN1_SCK *asn1, guint tag)
{
    guint saved_offset, real_tag;

    if (tvb_length_remaining(asn1->tvb, asn1->offset) <= 0)
    {
	return (FALSE);
    }

    saved_offset = asn1->offset;
    asn1_id_decode1(asn1, &real_tag);
    asn1->offset = saved_offset;
    return (tag == real_tag);
}


static int
dissect_tcap_param(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset)
{
    gint orig_offset, tag_offset, saved_offset, len_offset;
    tvbuff_t	*next_tvb;
    proto_tree *subtree;
	proto_item *pi;
	gint32 tlen;
	guint8 class;
	gboolean pc;
	guint32 tag;
	guint32 len;
	guint32 ind_field;
	guint32 soffset = offset;
	
orig_offset = offset;
tlen = tvb_length_remaining(tvb, offset);      
	while (tlen > (offset-orig_offset))
    {
    saved_offset = offset;
	
    offset = get_ber_identifier(tvb, offset, &class, &pc, &tag);
    tag_offset = offset;
	offset = get_ber_length(tree, tvb, offset, &len, &ind_field);
	len_offset = offset;
	

	
	if (pc)
	{
	    pi =
		proto_tree_add_text(tree, tvb, saved_offset, -1, "CONSTRUCTOR");
	    subtree = proto_item_add_subtree(pi, ett_param);
	    proto_tree_add_uint_format(subtree, hf_tcap_tag, tvb,
		saved_offset, tag_offset-saved_offset, tag, "CONSTRUCTOR Tag");
	    proto_tree_add_uint(subtree, hf_tcap_tag, tvb,
		saved_offset, tag_offset-saved_offset, class);

		proto_tree_add_uint(subtree, hf_tcap_length, tvb,
		    tag_offset, len_offset-tag_offset, len);
	/* need to handle indefinite length */
		next_tvb = tvb_new_subset(tvb, offset, len, len);		
	     dissect_tcap_param(pinfo, subtree,next_tvb,0);
	     offset += len;
	}
	else
	{


	pi =
	    proto_tree_add_text(tree, tvb,
		saved_offset, len + (len_offset - saved_offset), "Parameter");

	subtree = proto_item_add_subtree(pi, ett_param);

	proto_tree_add_uint(subtree, hf_tcap_tag, tvb,
	    saved_offset, 1, tag);

	proto_tree_add_uint(subtree, hf_tcap_length, tvb,
	    saved_offset+1, 1, len);
	next_tvb = tvb_new_subset(tvb, offset, len, len);		
	dissect_ber_octet_string(TRUE, pinfo, tree, next_tvb, 0, hf_tcap_data,
                                    NULL);
	     offset += len;
	}
    }
return offset;
}