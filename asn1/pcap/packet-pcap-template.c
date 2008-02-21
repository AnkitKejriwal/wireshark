/* packet-pcap.c
 * Routines for UTRAN Iupc interface Positioning Calculation Application Part (PCAP) packet dissection
 *
 * Copyright 2008, Anders Broman <anders.broman@ericsson.com>
 *
 * $Id$
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
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
 *
 * Based on the RANAP dissector
 *
 * References: ETSI TS 125 453 V7.9.0 (2008-02)
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
#include <epan/emem.h>
#include <epan/strutil.h>
#include <epan/asn1.h>

#include "packet-ber.h"
#include "packet-per.h"
#include "packet-sccp.h"

#ifdef _MSC_VER
/* disable: "warning C4146: unary minus operator applied to unsigned type, result still unsigned" */
#pragma warning(disable:4146)
#endif

#define PNAME  "UTRAN Iupc interface Positioning Calculation Application Part (PCAP)"
#define PSNAME "PCAP"
#define PFNAME "pcap"

#define MAX_SSN 254
static range_t *global_ssn_range;
static range_t *ssn_range;

static dissector_table_t sccp_ssn_table;

#include "packet-pcap-val.h"

static dissector_handle_t data_handle;
static dissector_handle_t pcap_handle = NULL;

/* Initialize the protocol and registered fields */
static int proto_pcap = -1;

#include "packet-pcap-hf.c"

/* Initialize the subtree pointers */
static int ett_pcap = -1;

#include "packet-pcap-ett.c"

/* Global variables */
static guint32 ProcedureCode;
static guint32 ProtocolIE_ID;
static guint32 ProtocolExtensionID;

/* Dissector tables */
static dissector_table_t pcap_ies_dissector_table;
static dissector_table_t pcap_ies_p1_dissector_table;
static dissector_table_t pcap_ies_p2_dissector_table;
static dissector_table_t pcap_extension_dissector_table;
static dissector_table_t pcap_proc_imsg_dissector_table;
static dissector_table_t pcap_proc_sout_dissector_table;
static dissector_table_t pcap_proc_uout_dissector_table;
static dissector_table_t pcap_proc_out_dissector_table;

static int dissect_ProtocolIEFieldValue(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);
static int dissect_ProtocolIEFieldPairFirstValue(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);
static int dissect_ProtocolIEFieldPairSecondValue(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);
static int dissect_ProtocolExtensionFieldExtensionValue(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);
static int dissect_InitiatingMessageValue(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);
static int dissect_SuccessfulOutcomeValue(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);
static int dissect_UnsuccessfulOutcomeValue(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);
static int dissect_OutcomeValue(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);

#include "packet-pcap-fn.c"

static int dissect_ProtocolIEFieldValue(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
  return (dissector_try_port(pcap_ies_dissector_table, ProtocolIE_ID, tvb, pinfo, tree)) ? tvb_length(tvb) : 0;
}

static int dissect_ProtocolIEFieldPairFirstValue(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
  return (dissector_try_port(pcap_ies_p1_dissector_table, ProtocolIE_ID, tvb, pinfo, tree)) ? tvb_length(tvb) : 0;
}

static int dissect_ProtocolIEFieldPairSecondValue(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
  return (dissector_try_port(pcap_ies_p2_dissector_table, ProtocolIE_ID, tvb, pinfo, tree)) ? tvb_length(tvb) : 0;
}

static int dissect_ProtocolExtensionFieldExtensionValue(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
  return (dissector_try_port(pcap_extension_dissector_table, ProtocolExtensionID, tvb, pinfo, tree)) ? tvb_length(tvb) : 0;
}

static int dissect_InitiatingMessageValue(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
  return (dissector_try_port(pcap_proc_imsg_dissector_table, ProcedureCode, tvb, pinfo, tree)) ? tvb_length(tvb) : 0;
}

static int dissect_SuccessfulOutcomeValue(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
  return (dissector_try_port(pcap_proc_sout_dissector_table, ProcedureCode, tvb, pinfo, tree)) ? tvb_length(tvb) : 0;
}

static int dissect_UnsuccessfulOutcomeValue(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
  return (dissector_try_port(pcap_proc_uout_dissector_table, ProcedureCode, tvb, pinfo, tree)) ? tvb_length(tvb) : 0;
}

static int dissect_OutcomeValue(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
  return (dissector_try_port(pcap_proc_out_dissector_table, ProcedureCode, tvb, pinfo, tree)) ? tvb_length(tvb) : 0;
}

static void
dissect_pcap(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	proto_item	*pcap_item = NULL;
	proto_tree	*pcap_tree = NULL;

	/* make entry in the Protocol column on summary display */
	if (check_col(pinfo->cinfo, COL_PROTOCOL))
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "PCAP");

	/* create the pcap protocol tree */
	pcap_item = proto_tree_add_item(tree, proto_pcap, tvb, 0, -1, FALSE);
	pcap_tree = proto_item_add_subtree(pcap_item, ett_pcap);
	
	dissect_PCAP_PDU_PDU(tvb, pinfo, pcap_tree);
}


static void range_delete_callback(guint32 ssn)
{
    if ( ssn ) {
        dissector_delete("sccp.ssn", ssn, pcap_handle);
    }
}

static void range_add_callback(guint32 ssn)
{
    if (ssn) {
        dissector_add("sccp.ssn", ssn, pcap_handle);
    }
}


static void init_pcap(void) {
    if (ssn_range) {
        range_foreach(ssn_range, range_delete_callback);
        g_free(ssn_range);
    }

    ssn_range = range_copy(global_ssn_range);
    range_foreach(ssn_range, range_add_callback);
}


/*--- proto_reg_handoff_pcap ---------------------------------------*/
void
proto_reg_handoff_pcap(void)
{

    static gboolean prefs_initialized = FALSE;

    if (! prefs_initialized) {
        sccp_ssn_table = find_dissector_table("sccp.ssn");
        prefs_initialized = TRUE;
    }

    data_handle = find_dissector("data");

#include "packet-pcap-dis-tab.c"
}

/*--- proto_register_pcap -------------------------------------------*/
void proto_register_pcap(void) {

  /* List of fields */

  static hf_register_info hf[] = {

#include "packet-pcap-hfarr.c"
  };

  /* List of subtrees */
  static gint *ett[] = {
		  &ett_pcap,
#include "packet-pcap-ettarr.c"
  };

  module_t *pcap_module;

  /* Register protocol */
  proto_pcap = proto_register_protocol(PNAME, PSNAME, PFNAME);
  /* Register fields and subtrees */
  proto_register_field_array(proto_pcap, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));

  pcap_module = prefs_register_protocol(proto_pcap, proto_reg_handoff_pcap);
 
  /* Register dissector */
  register_dissector("pcap", dissect_pcap, proto_pcap);
  pcap_handle = find_dissector("pcap");

  /* Register dissector tables */
  pcap_ies_dissector_table = register_dissector_table("pcap.ies", "PCAP-PROTOCOL-IES", FT_UINT32, BASE_DEC);
  pcap_ies_p1_dissector_table = register_dissector_table("pcap.ies.pair.first", "PCAP-PROTOCOL-IES-PAIR FirstValue", FT_UINT32, BASE_DEC);
  pcap_ies_p2_dissector_table = register_dissector_table("pcap.ies.pair.second", "PCAP-PROTOCOL-IES-PAIR SecondValue", FT_UINT32, BASE_DEC);
  pcap_extension_dissector_table = register_dissector_table("pcap.extension", "PCAP-PROTOCOL-EXTENSION", FT_UINT32, BASE_DEC);
  pcap_proc_imsg_dissector_table = register_dissector_table("pcap.proc.imsg", "PCAP-ELEMENTARY-PROCEDURE InitiatingMessage", FT_UINT32, BASE_DEC);
  pcap_proc_sout_dissector_table = register_dissector_table("pcap.proc.sout", "PCAP-ELEMENTARY-PROCEDURE SuccessfulOutcome", FT_UINT32, BASE_DEC);
  pcap_proc_uout_dissector_table = register_dissector_table("pcap.proc.uout", "PCAP-ELEMENTARY-PROCEDURE UnsuccessfulOutcome", FT_UINT32, BASE_DEC);
  pcap_proc_out_dissector_table = register_dissector_table("pcap.proc.out", "PCAP-ELEMENTARY-PROCEDURE Outcome", FT_UINT32, BASE_DEC);


  /* Preferences */
    /* Set default SSNs */
  range_convert_str(&global_ssn_range, "", MAX_SSN);
  ssn_range = range_empty();

  prefs_register_range_preference(pcap_module, "ssn", "SCCP SSNs",
  "SCCP (and SUA) SSNs to decode as PCAP",
  &global_ssn_range, MAX_SSN);


  register_init_routine(&init_pcap);
}




