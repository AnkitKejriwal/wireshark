/* packet-tn3270.c
 * Routines for tn3270.packet dissection
 *
 * Reference:
 *  3270 Information Display System: Data Stream Programmer's Reference
 *  GA23-0059-07
 *  (http://www-01.ibm.com/support/docview.wss?uid=pub1ga23005907)
 *
 * Copyright 2009, Robert Hogan <robert@roberthogan.net>
 *
 * $Id$
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
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
 *
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <string.h>
#include <glib.h>
#include <epan/address.h>
#include <epan/conversation.h>
#include <epan/packet.h>
#include <epan/strutil.h>

#include "packet-tn3270.h"

static int proto_tn3270=-1;

static int hf_tn3270_fa_display=-1;
static int hf_tn3270_fa_graphic_convert1=-1;
static int hf_tn3270_fa_graphic_convert2=-1;
static int hf_tn3270_fa_modified=-1;
static int hf_tn3270_fa_numeric=-1;
static int hf_tn3270_fa_protected=-1;
static int hf_tn3270_fa_reserved=-1;
static int hf_tn3270_field_attribute=-1;
static int hf_tn3270_aid=-1;
static int hf_tn3270_all_character_attributes=-1;
static int hf_tn3270_attribute_type=-1;
static int hf_tn3270_begin_end_flags1=-1;
static int hf_tn3270_begin_end_flags2=-1;
static int hf_tn3270_bsc=-1;
static int hf_tn3270_buffer_x=-1;
static int hf_tn3270_buffer_y=-1;
static int hf_tn3270_c_cav=-1;
static int hf_tn3270_cc=-1;
static int hf_tn3270_character_code=-1;
static int hf_tn3270_character_set=-1;
static int hf_tn3270_charset=-1;
static int hf_tn3270_checkpoint=-1;
static int hf_tn3270_c_ci=-1;
static int hf_tn3270_c_offset=-1;
static int hf_tn3270_color=-1;
static int hf_tn3270_color_command=-1;
static int hf_tn3270_color_flags=-1;
static int hf_tn3270_command_code=-1;
static int hf_tn3270_cro=-1;
static int hf_tn3270_c_scsoff=-1;
static int hf_tn3270_c_seqoff=-1;
static int hf_tn3270_c_sequence=-1;
static int hf_tn3270_cursor_x=-1;
static int hf_tn3270_cursor_y=-1;
static int hf_tn3270_cw=-1;
static int hf_tn3270_data_chain_bitmask=-1;
static int hf_tn3270_destination_or_origin_bitmask=-1;
static int hf_tn3270_double_byte_sf_id=-1;
static int hf_tn3270_erase_flags=-1;
static int hf_tn3270_exception_or_status_flags=-1;
static int hf_tn3270_extended_highlighting=-1;
static int hf_tn3270_extended_ps_color=-1;
static int hf_tn3270_extended_ps_echar=-1;
static int hf_tn3270_extended_ps_flags=-1;
static int hf_tn3270_extended_ps_length=-1;
static int hf_tn3270_extended_ps_lw=-1;
static int hf_tn3270_extended_ps_lh=-1;
static int hf_tn3270_extended_ps_nh=-1;
static int hf_tn3270_extended_ps_nw=-1;
static int hf_tn3270_extended_ps_res=-1;
static int hf_tn3270_extended_ps_stsubs=-1;
static int hf_tn3270_extended_ps_subsn=-1;
static int hf_tn3270_featl=-1;
static int hf_tn3270_feats=-1;
static int hf_tn3270_field_data=-1;
static int hf_tn3270_field_outlining=-1;
static int hf_tn3270_field_validation_mandatory_entry=-1;
static int hf_tn3270_field_validation_mandatory_fill=-1;
static int hf_tn3270_field_validation_trigger=-1;
static int hf_tn3270_format_group=-1;
static int hf_tn3270_format_name=-1;
static int hf_tn3270_fov=-1;
static int hf_tn3270_fpc=-1;
static int hf_tn3270_hilite=-1;
static int hf_tn3270_h_length=-1;
static int hf_tn3270_h_offset=-1;
static int hf_tn3270_horizon=-1;
static int hf_tn3270_h_sequence=-1;
static int hf_tn3270_hw=-1;
static int hf_tn3270_interval=-1;
static int hf_tn3270_limin=-1;
static int hf_tn3270_limout=-1;
static int hf_tn3270_lines=-1;
static int hf_tn3270_load_color_command=-1;
static int hf_tn3270_load_format_storage_flags1=-1;
static int hf_tn3270_load_format_storage_flags2=-1;
static int hf_tn3270_load_format_storage_format_data=-1;
static int hf_tn3270_load_format_storage_localname=-1;
static int hf_tn3270_load_format_storage_operand=-1;
static int hf_tn3270_load_line_type_command=-1;
static int hf_tn3270_lvl=-1;
static int hf_tn3270_mode=-1;
static int hf_tn3270_msr_ind_mask=-1;
static int hf_tn3270_msr_ind_value=-1;
static int hf_tn3270_msr_state_mask=-1;
static int hf_tn3270_msr_state_value=-1;
static int hf_tn3270_msr_type=-1;
static int hf_tn3270_ap_na=-1;
static int hf_tn3270_ap_m=-1;
static int hf_tn3270_ap_vertical_scrolling=-1;
static int hf_tn3270_ap_horizontal_scrolling=-1;
static int hf_tn3270_ap_apres1=-1;
static int hf_tn3270_ap_apa=-1;
static int hf_tn3270_ap_pp=-1;
static int hf_tn3270_ap_lc=-1;
static int hf_tn3270_ap_mp=-1;
static int hf_tn3270_ap_apres2=-1;
static int hf_tn3270_c_np=-1;
static int hf_tn3270_number_of_attributes=-1;
static int hf_tn3270_object_control_flags=-1;
static int hf_tn3270_object_type=-1;
static int hf_tn3270_operation_type=-1;
static int hf_tn3270_order_code=-1;
static int hf_tn3270_outbound_text_header_hdr=-1;
static int hf_tn3270_outbound_text_header_lhdr=-1;
static int hf_tn3270_pages=-1;
static int hf_tn3270_partition_command=-1;
static int hf_tn3270_partition_cv=-1;
static int hf_tn3270_partition_cw=-1;
static int hf_tn3270_partition_flags=-1;
static int hf_tn3270_partition_height=-1;
static int hf_tn3270_partition_hv=-1;
static int hf_tn3270_partition_id=-1;
static int hf_tn3270_partition_ph=-1;
static int hf_tn3270_partition_pw=-1;
static int hf_tn3270_partition_res=-1;
static int hf_tn3270_partition_rs=-1;
static int hf_tn3270_partition_rv=-1;
static int hf_tn3270_partition_rw=-1;
static int hf_tn3270_partition_uom=-1;
static int hf_tn3270_partition_width=-1;
static int hf_tn3270_partition_wv=-1;
static int hf_tn3270_prime=-1;
static int hf_tn3270_printer_flags=-1;
static int hf_tn3270_ps_char=-1;
static int hf_tn3270_ps_flags=-1;
static int hf_tn3270_ps_lcid=-1;
static int hf_tn3270_ps_rws=-1;
static int hf_tn3270_query_reply_alphanumeric_flags=-1;
static int hf_tn3270_recovery_data_flags=-1;
static int hf_tn3270_reply_mode_attr_list=-1;
static int hf_tn3270_reqtyp=-1;
static int hf_tn3270_resbyte=-1;
static int hf_tn3270_resbytes=-1;
static int hf_tn3270_res_twobytes=-1;
static int hf_tn3270_rw=-1;
static int hf_tn3270_save_or_restore_format_flags=-1;
static int hf_tn3270_scs_data=-1;
static int hf_tn3270_sf_outbound_id=-1;
static int hf_tn3270_sf_inbound_id=-1;
static int hf_tn3270_sf_inbound_outbound_id=-1;
static int hf_tn3270_sf_length=-1;
static int hf_tn3270_sf_query_replies=-1;
static int hf_tn3270_sld=-1;
static int hf_tn3270_spd=-1;
static int hf_tn3270_start_line=-1;
static int hf_tn3270_start_page=-1;
static int hf_tn3270_stop_address=-1;
static int hf_tn3270_transparency=-1;
static int hf_tn3270_type_1_text_outbound_data=-1;
static int hf_tn3270_vertical=-1;
static int hf_tn3270_v_length=-1;
static int hf_tn3270_v_offset=-1;
static int hf_tn3270_v_sequence=-1;
static int hf_tn3270_wcc=-1;
static int hf_tn3270_wcc_nop=-1;
static int hf_tn3270_wcc_reset=-1;
static int hf_tn3270_wcc_printer1=-1;
static int hf_tn3270_wcc_printer2=-1;
static int hf_tn3270_wcc_start_printer=-1;
static int hf_tn3270_wcc_sound_alarm=-1;
static int hf_tn3270_wcc_keyboard_restore=-1;
static int hf_tn3270_wcc_reset_mdt=-1;
static int hf_tn3270_ww=-1;
static int hf_tn3270_tn3270e_data_type=-1;
static int hf_tn3270_tn3270e_request_flag=-1;
static int hf_tn3270_tn3270e_response_flag_3270_SCS=-1;
static int hf_tn3270_tn3270e_response_flag_response=-1;
static int hf_tn3270_tn3270e_seq_number=-1;
static int hf_tn3270_tn3270e_header_data=-1;
static int hf_tn3270_ua_cell_units=-1;
static int hf_tn3270_ua_characters=-1;
static int hf_tn3270_ua_hard_copy=-1;
static int hf_tn3270_ua_page_printer=-1;
static int hf_tn3270_ua_reserved1=-1;
static int hf_tn3270_ua_reserved2=-1;
static int hf_tn3270_ua_variable_cells=-1;
static int hf_tn3270_usable_area_flags1=-1;
static int hf_tn3270_usable_area_flags2=-1;
static int hf_tn3270_ua_addressing=-1;
static int hf_tn3270_ua_width_cells_pels=-1;
static int hf_tn3270_ua_height_cells_pels=-1;
static int hf_tn3270_ua_uom_cells_pels=-1;
static int hf_tn3270_ua_xr=-1;
static int hf_tn3270_ua_yr=-1;
static int hf_tn3270_ua_aw=-1;
static int hf_tn3270_ua_ah=-1;
static int hf_tn3270_ua_buffsz=-1;
static int hf_tn3270_ua_xmin=-1;
static int hf_tn3270_ua_ymin=-1;
static int hf_tn3270_ua_xmax=-1;
static int hf_tn3270_ua_ymax=-1;
static int hf_tn3270_cs_ge=-1;
static int hf_tn3270_cs_mi=-1;
static int hf_tn3270_cs_lps=-1;
static int hf_tn3270_cs_lpse=-1;
static int hf_tn3270_cs_ms=-1;
static int hf_tn3270_cs_ch2=-1;
static int hf_tn3270_cs_gf=-1;
static int hf_tn3270_cs_res=-1;
static int hf_tn3270_cs_res2=-1;
static int hf_tn3270_cs_pscs=-1;
static int hf_tn3270_cs_res3=-1;
static int hf_tn3270_cs_cf=-1;
static int hf_tn3270_cs_form_type1=-1;
static int hf_tn3270_cs_form_type2=-1;
static int hf_tn3270_cs_form_type3=-1;
static int hf_tn3270_cs_form_type4=-1;
static int hf_tn3270_cs_form_type5=-1;
static int hf_tn3270_cs_form_type6=-1;
static int hf_tn3270_cs_form_type8=-1;
static int hf_tn3270_cs_ds_load=-1;
static int hf_tn3270_cs_ds_triple=-1;
static int hf_tn3270_cs_ds_char=-1;
static int hf_tn3270_cs_ds_cb=-1;
static int hf_tn3270_character_sets_flags1=-1;
static int hf_tn3270_character_sets_flags2=-1;
static int hf_tn3270_sdw=-1;
static int hf_tn3270_sdh=-1;
static int hf_tn3270_form=-1;
static int hf_tn3270_formres=-1;
static int hf_tn3270_formres2=-1;
static int hf_tn3270_cs_dl=-1;
static int hf_tn3270_cs_descriptor_set=-1;
static int hf_tn3270_cs_descriptor_flags=-1;
static int hf_tn3270_lcid=-1;
static int hf_tn3270_sw=-1;
static int hf_tn3270_sh=-1;
static int hf_tn3270_ssubsn=-1;
static int hf_tn3270_esubsn=-1;
static int hf_tn3270_ccsgid=-1;
static int hf_tn3270_ccsid=-1;
static int hf_tn3270_c_prtblk=-1;
static int hf_tn3270_h_np=-1;
static int hf_tn3270_h_vi=-1;
static int hf_tn3270_h_ai=-1;
static int hf_tn3270_ddm_flags=-1;
static int hf_tn3270_ddm_limin=-1;
static int hf_tn3270_ddm_limout=-1;
static int hf_tn3270_ddm_nss=-1;
static int hf_tn3270_ddm_ddmss=-1;
static int hf_tn3270_rpq_device=-1;
static int hf_tn3270_rpq_mid=-1;
static int hf_tn3270_rpq_rpql=-1;
static int hf_tn3270_rpq_name=-1;
static int hf_tn3270_ip_flags=-1;
static int hf_tn3270_ipdd_length=-1;
static int hf_tn3270_ip_id=-1;
static int hf_tn3270_ipdd_wd=-1;
static int hf_tn3270_ipdd_hd=-1;
static int hf_tn3270_ipdd_wa=-1;
static int hf_tn3270_ipdd_ha=-1;
static int hf_tn3270_ippd_dpbs=-1;
static int hf_tn3270_ippd_apbs=-1;
static int hf_tn3270_ipccd_wcd=-1;
static int hf_tn3270_ipccd_hcd=-1;
static int hf_tn3270_ipccd_wca=-1;
static int hf_tn3270_ipccd_hca=-1;
static int hf_tn3270_dc_dir_flags=-1;
static int hf_tn3270_dc_both=-1;
static int hf_tn3270_dc_from_device=-1;
static int hf_tn3270_dc_to_device=-1;
static int hf_tn3270_oem_dsref=-1;
static int hf_tn3270_oem_dtype=-1;
static int hf_tn3270_oem_uname=-1;
static int hf_tn3270_sdp_daid=-1;
static int hf_tn3270_oem_sdp_ll_limin=-1;
static int hf_tn3270_oem_sdp_ll_limout=-1;
static int hf_tn3270_oem_sdp_pclk_vers=-1;
static int hf_tn3270_null=-1;
static int hf_tn3270_unknown_data=-1;
static int hf_tn3270_ds_default_sfid=-1;
static int hf_tn3270_ds_sfid=-1;
static int hf_tn3270_asia_sdp_sosi_soset=-1;
static int hf_tn3270_asia_sdp_ic_func=-1;
static int hf_tn3270_ccc=-1;
static int hf_tn3270_ccc_coding=-1;
static int hf_tn3270_ccc_printout=-1;
static int hf_tn3270_ccc_start_print=-1;
static int hf_tn3270_ccc_sound_alarm=-1;
static int hf_tn3270_ccc_copytype=-1;
static int hf_tn3270_msr_user=-1;
static int hf_tn3270_msr_locked=-1;
static int hf_tn3270_msr_auto=-1;
static int hf_tn3270_msr_ind1=-1;
static int hf_tn3270_msr_ind2=-1;
static int hf_tn3270_spc_sdp_ot=-1;
static int hf_tn3270_spc_sdp_ob=-1;
static int hf_tn3270_spc_sdp_ol=-1;
static int hf_tn3270_spc_sdp_or=-1;
static int hf_tn3270_spc_sdp_eucflags=-1;
static int hf_tn3270_spc_sdp_srepc=-1;
static int hf_tn3270_srf_fpcb=-1;
static int hf_tn3270_sdp_statcode=-1;
static int hf_tn3270_sdp_excode=-1;
static int hf_tn3270_sdp_ngl=-1;
static int hf_tn3270_sdp_nml=-1;
static int hf_tn3270_sdp_nlml=-1;
static int hf_tn3270_sdp_stor=-1;
static int hf_tn3270_ap_cm=-1;
static int hf_tn3270_ap_ro=-1;
static int hf_tn3270_ap_co=-1;
static int hf_tn3270_ap_fo=-1;
static int hf_tn3270_sdp_ln=-1;
static int hf_tn3270_sdp_id=-1;
static int hf_tn3270_db_cavdef=-1;
static int hf_tn3270_db_cidef=-1;
static int hf_tn3270_dia_flags=-1;
static int hf_tn3270_dia_limin=-1;
static int hf_tn3270_dia_limout=-1;
static int hf_tn3270_dia_nfs=-1;
static int hf_tn3270_dia_diafs=-1;
static int hf_tn3270_dia_diafn=-1;
static int hf_tn3270_fo_flags=-1;
static int hf_tn3270_fo_vpos=-1;
static int hf_tn3270_fo_hpos=-1;
static int hf_tn3270_fo_hpos0=-1;
static int hf_tn3270_fo_hpos1=-1;
static int hf_tn3270_fsad_flags=-1;
static int hf_tn3270_fsad_limin=-1;
static int hf_tn3270_fsad_limout=-1;
static int hf_tn3270_fsad_size=-1;
static int hf_tn3270_ibm_flags=-1;
static int hf_tn3270_ibm_limin=-1;
static int hf_tn3270_ibm_limout=-1;
static int hf_tn3270_ibm_type=-1;
static int hf_tn3270_msr_nd=-1;
static int hf_tn3270_pft_flags=-1;
static int hf_tn3270_pft_tmo=-1;
static int hf_tn3270_pft_bmo=-1;
static int hf_tn3270_ioca_limin=-1;
static int hf_tn3270_ioca_limout=-1;
static int hf_tn3270_ioca_type=-1;
static int hf_tn3270_pc_vo_thickness=-1;
static int hf_tn3270_pdds_ssid=-1;
static int hf_tn3270_pdds_refid=-1;
static int hf_tn3270_srf_fpcbl=-1;
static int hf_tn3270_spc_epc_flags=-1;
static int hf_tn3270_sp_spid=-1;
static int hf_tn3270_sp_size=-1;
static int hf_tn3270_sp_space=-1;
static int hf_tn3270_sp_objlist=-1;
static int hf_tn3270_tp_nt=-1;
static int hf_tn3270_tp_m=-1;
static int hf_tn3270_tp_flags=-1;
static int hf_tn3270_tp_ntt=-1;
static int hf_tn3270_tp_tlist=-1;
static int hf_tn3270_t_np=-1;
static int hf_tn3270_t_vi=-1;
static int hf_tn3270_t_ai=-1;
static int hf_tn3270_3270_tranlim=-1;

static gint ett_tn3270 =-1;
static gint ett_sf =-1;
static gint ett_tn3270_field_attribute =-1;
static gint ett_tn3270_field_validation =-1;
static gint ett_tn3270_wcc =-1;
static gint ett_tn3270_usable_area_flags1 =-1;
static gint ett_tn3270_usable_area_flags2 =-1;
static gint ett_tn3270_query_reply_alphanumeric_flags=-1;
static gint ett_tn3270_character_sets_flags1=-1;
static gint ett_tn3270_character_sets_flags2=-1;
static gint ett_tn3270_character_sets_form=-1;
static gint ett_tn3270_cs_descriptor_flags=-1;
static gint ett_tn3270_color_flags=-1;
static gint ett_tn3270_dc_dir_flags=-1;
static gint ett_tn3270_ccc=-1;
static gint ett_tn3270_msr_state_mask=-1;

tn3270_conv_info_t *tn3270_info_items;

gint dissect_orders_and_data(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset);

typedef struct hf_items {
  int hf;
  gint bitmask_ett;
  int length;
  const int **bitmask;
} hf_items;

/* Utility Functions */

static gint
tn3270_add_hf_items(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                             hf_items *fields)
{
  int start=offset;
  int i;

  for (i = 0; fields[i].hf; ++i) {
    if (fields[i].bitmask == 0) {
      proto_tree_add_item(tn3270_tree,
                            fields[i].hf,
                            tvb, offset,
                            fields[i].length,
                            FALSE);
    } else {
      proto_tree_add_bitmask(tn3270_tree, tvb, offset, fields[i].hf,
          fields[i].bitmask_ett, fields[i].bitmask, FALSE);
    }
    offset+=fields[i].length;
  }
  return (offset - start);
}

static gint
dissect_unknown_data(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset, gint start,
                                                gint sf_length)
{
  int len_left;

  len_left = (sf_length - 4) - (offset - start);

  if (len_left > 0) {
    proto_tree_add_item(tn3270_tree, hf_tn3270_unknown_data,
                            tvb, offset, len_left,
                            FALSE);
    return len_left;
  }
  return 0;
}

static gint
add_data_until_next_order_code(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset)
{
  int datalen = 0;
  gint order_code = 0;
  int done = 0;

  while (tvb_offset_exists(tvb, (offset + datalen)) && !done) {
    order_code = tvb_get_guint8(tvb, (offset + datalen));
    switch (order_code) {
          case SF:
            /*dummy*/
          case SFE:
          case SA:
          case MF:
          case IC:
          case PT:
          case RA:
          case EUA:
          case GE:
          case SBA:
            done = 1;
            break;
          default:
            datalen++;
            break;
    }
  }

  if (datalen) {
    proto_tree_add_item(tn3270_tree, hf_tn3270_field_data, tvb, offset,
                        datalen, FALSE);
  }
  return datalen;
}

static gint
dissect_query_reply_resbytes(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                                 gint sf_length)
{
  int start=offset;


  hf_items fields[] = {
      { hf_tn3270_res_twobytes, 0, 2, 0 },
      { 0, 0, 0, 0 },
  };


  offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                fields);

  offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, sf_length);

  return (offset - start);
}

static int
dissect_wcc(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset)
{

  static const int *wcc_fields[] = {
      &hf_tn3270_wcc_nop,
      &hf_tn3270_wcc_reset,
      &hf_tn3270_wcc_printer1,
      &hf_tn3270_wcc_printer2,
      &hf_tn3270_wcc_start_printer,
      &hf_tn3270_wcc_sound_alarm,
      &hf_tn3270_wcc_keyboard_restore,
      &hf_tn3270_wcc_reset_mdt,
      NULL
  };

  /* Qualifier and DeviceType */
  proto_tree_add_bitmask_text(tn3270_tree, tvb, offset, 1, "Write Control Character: ", "None",
          ett_tn3270_wcc, wcc_fields, TRUE, 0);
  return 1;

}

static gint
dissect_3270_field_validation(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset)
{
  int start = offset;

  static const int *byte[] = {
    &hf_tn3270_field_validation_mandatory_fill,
    &hf_tn3270_field_validation_mandatory_entry,
    &hf_tn3270_field_validation_trigger,
    NULL
    };

  proto_tree_add_bitmask_text(tn3270_tree, tvb, 1, 1, "Field Validation: ",
                              "None", ett_tn3270_field_validation, byte, TRUE, 0);

  offset++;
  return (offset - start);
}


static gint
dissect_3270_field_attribute(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset)
{
  int start = offset;

  static const int *byte[] = {
    &hf_tn3270_fa_display,
    &hf_tn3270_fa_graphic_convert1,
    &hf_tn3270_fa_graphic_convert2,
    &hf_tn3270_fa_modified,
    &hf_tn3270_fa_numeric,
    &hf_tn3270_fa_protected,
    &hf_tn3270_fa_reserved,
    NULL
    };


  proto_tree_add_bitmask(tn3270_tree, tvb, offset, hf_tn3270_field_attribute,
      ett_tn3270_field_attribute, byte, FALSE);

  offset++;
  return (offset - start);
}

/* 8.7 - Copy Control Code */
static gint
dissect_ccc(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset)
{
  int start = offset;

  static const int *byte[] = {
    &hf_tn3270_ccc_coding,
    &hf_tn3270_ccc_printout,
    &hf_tn3270_ccc_start_print,
    &hf_tn3270_ccc_sound_alarm,
    &hf_tn3270_ccc_copytype,
    NULL
    };



  proto_tree_add_bitmask(tn3270_tree, tvb, offset, hf_tn3270_ccc,
      ett_tn3270_ccc, byte, FALSE);

  offset++;
  return (offset - start);
}

/* End - Utility Functions */

/* Start: Handle Structured Fields */

/* --------------------------------------------------- */
/* 5.0 Outbound/Inbound and Outbound Structured Fields */
/* --------------------------------------------------- */

/* 5.5 Activate Partition - Search for ACTIVATE_PARTITION */
/* 5.6 Begin/End of File - Search for BEGIN_OR_END_OF_FILE */
/* 5.7 Create Partition */
static gint
dissect_create_partition(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset, gint sf_length)
{
  int start=offset;

  hf_items fields[] = {
    { hf_tn3270_partition_id, 0, 1, 0 },
    { hf_tn3270_partition_uom, 0, 1, 0 },
    { hf_tn3270_partition_flags, 0, 1, 0 },
    { hf_tn3270_partition_height, 0, 2, 0 },
    { hf_tn3270_partition_width, 0, 2, 0 },
    { hf_tn3270_partition_rv, 0, 2, 0 },
    { hf_tn3270_partition_cv, 0, 2, 0 },
    { hf_tn3270_partition_hv, 0, 2, 0 },
    { hf_tn3270_partition_wv, 0, 2, 0 },
    { hf_tn3270_partition_rw, 0, 2, 0 },
    { hf_tn3270_partition_cw, 0, 2, 0 },
    { hf_tn3270_partition_rs, 0, 2, 0 },
    { hf_tn3270_partition_res, 0, 2, 0 },
    { hf_tn3270_partition_pw, 0, 2, 0 },
    { hf_tn3270_partition_ph, 0, 2, 0 },
    { 0, 0, 0, 0 },
  };


  offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                fields);

  offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, sf_length);

  return (offset - start);
}

/* 5.7 Create Partition - Search for CREATE_PARTITION */
/* 5.8 Destroy Partition - Search for DESTROY_PARTITION */
/* 5.9 Erase/Reset - Search for ERASE_OR_RESET */
/* 5.10 Load Color Table - Search for LOAD_COLOR_TABLE */

/* 5.11 Load Format Storage */
static gint
dissect_load_format_storage(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset, gint sf_length)
{
  int start=offset;
  int operand;

  hf_items fields[] = {
    { hf_tn3270_load_format_storage_flags1, 0, 1, 0 },
    { hf_tn3270_load_format_storage_flags2, 0, 1, 0 },
    { hf_tn3270_load_format_storage_operand, 0, 1, 0 },
    { hf_tn3270_load_format_storage_localname, 0, 8, 0 },
    { hf_tn3270_format_group, 0, 6, 0 },
    { hf_tn3270_format_name, 0, 16, 0 },
    { 0, 0, 0, 0 },
  };

  operand = tvb_get_guint8(tvb, offset+2);

  offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                fields);

  if (operand == ADD) {
    int fmtln = ((sf_length - 4) - (offset - start));
    proto_tree_add_item(tn3270_tree, hf_tn3270_load_format_storage_format_data,
                        tvb, offset, fmtln, FALSE);
    offset+=fmtln;
  }

  offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, sf_length);

  return (offset - start);
}

/* 5.12 Load Line Type - Search for LOAD_LINE_TYPE */

/* 5.13 Load Programmed Symbols (Load PS) */
static gint
dissect_load_programmed_symbols(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset, gint sf_length)
{
  int start=offset, i;
  gint8 flags;
  gint8 extended_ps_length;
  hf_items ps_fields[] = {
      { hf_tn3270_ps_flags, 0, 1, 0 },
      { hf_tn3270_ps_lcid, 0, 1, 0 },
      { hf_tn3270_ps_char, 0, 1, 0 },
      { hf_tn3270_ps_rws, 0, 1, 0 },
      { 0, 0, 0, 0 },
  };

  hf_items extended_ps_fields[] = {
      { hf_tn3270_extended_ps_lw, 0, 1, 0 },
      { hf_tn3270_extended_ps_lh, 0, 1, 0 },
      { hf_tn3270_extended_ps_subsn, 0, 1, 0 },
      { hf_tn3270_extended_ps_color, 0, 1, 0 },
      { hf_tn3270_extended_ps_stsubs, 0, 1, 0 },
      { hf_tn3270_extended_ps_echar, 0, 1, 0 },
      { hf_tn3270_extended_ps_nw, 0, 1, 0 },
      { hf_tn3270_extended_ps_nh, 0, 1, 0 },
      { hf_tn3270_extended_ps_res, 0, 1, 0 },
      { 0, 0, 0, 0 },
  };

  flags = tvb_get_guint8(tvb, offset);
  offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                ps_fields);

  /*If extended flag not set return */
  if (!(flags & 0x80)) {
    return (offset - start);
  }

  extended_ps_length = tvb_get_guint8(tvb, offset);
  proto_tree_add_item(tn3270_tree,  hf_tn3270_extended_ps_length,
                          tvb, offset,  1, FALSE);
  offset++;
  proto_tree_add_item(tn3270_tree, hf_tn3270_extended_ps_flags,
                          tvb, offset, 1, FALSE);
  offset++;

  for (i = 0; i < extended_ps_length; ++i) {
      proto_tree_add_item(tn3270_tree, extended_ps_fields[i].hf,
                          tvb, offset, extended_ps_fields[i].length, FALSE);
      offset+=extended_ps_fields[i].length;
  }


  offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, sf_length);

  return (offset - start);
}

/* 5.14 Modify Partition) */
static gint
dissect_modify_partition(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset, gint sf_length)
{
  int start=offset;

  hf_items fields[] = {
    { hf_tn3270_resbyte, 0, 1, 0 },
    { hf_tn3270_partition_id, 0, 1, 0 },
    { hf_tn3270_resbyte, 0, 1, 0 },
    { hf_tn3270_partition_flags, 0, 1, 0 },
    { hf_tn3270_resbyte, 0, 1, 0 },
    { hf_tn3270_resbytes, 0, 2, 0 },
    { hf_tn3270_partition_rv, 0, 2, 0 },
    { hf_tn3270_partition_cv, 0, 2, 0 },
    { hf_tn3270_partition_hv, 0, 2, 0 },
    { hf_tn3270_partition_wv, 0, 2, 0 },
    { hf_tn3270_partition_rw, 0, 2, 0 },
    { hf_tn3270_partition_cw, 0, 2, 0 },
    { hf_tn3270_partition_rs, 0, 2, 0 },
    { hf_tn3270_partition_res, 0, 2, 0 },
    { hf_tn3270_partition_pw, 0, 2, 0 },
    { hf_tn3270_partition_ph, 0, 2, 0 },
    { 0, 0, 0, 0 },
  };


  offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                fields);

  offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, sf_length);

  return (offset - start);
}

/* 5.15 Outbound Text Header */
static gint
dissect_outbound_text_header(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                             gint sf_length)
{
  int start=offset;
  gint16 hdr_length;

  hf_items outbound_text_header_fields1[] = {
      { hf_tn3270_partition_id, 0, 1, 0 },
      { hf_tn3270_operation_type, 0, 1, 0 },
      { 0, 0, 0, 0 },
  };

  hf_items outbound_text_header_fields2[] = {
      { hf_tn3270_resbyte, 0, 1, 0 },
      { hf_tn3270_resbyte, 0, 1, 0 },
      { hf_tn3270_lvl, 0, 1, 0 },
      { hf_tn3270_cro, 0, 2, 0 },
      { hf_tn3270_cc, 0, 2, 0 },
      { 0, 0, 0, 0 },
  };

  offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                outbound_text_header_fields1);
  offset += dissect_wcc(tn3270_tree, tvb, offset);
  offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                outbound_text_header_fields2);

  hdr_length = tvb_get_ntohs(tvb, offset);

  proto_tree_add_item(tn3270_tree, hf_tn3270_outbound_text_header_lhdr,
                      tvb, offset, 2, FALSE);
  offset+=2;

  proto_tree_add_item(tn3270_tree, hf_tn3270_outbound_text_header_hdr,
                      tvb, offset, hdr_length, FALSE);
  offset+=hdr_length;

  offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, sf_length);

  return (offset - start);
}

/* 5.16 Outbound 3270DS */
static gint
dissect_outbound_3270ds(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                             gint sf_length)
{
  int start=offset; 
  int cmd;

  proto_tree_add_item(tn3270_tree,
                          hf_tn3270_partition_id,
                          tvb, offset,
                          1,
                          FALSE);
  offset++;

  cmd = tvb_get_guint8(tvb, offset);
  proto_tree_add_item(tn3270_tree,
                          hf_tn3270_partition_command,
                          tvb, offset,
                          1,
                          FALSE);
  offset++;

  /* FIXME: the spec is ambiguous at best about what to expect here,
            need a live sample to validate. */
  switch (cmd) {
    case SNA_BSC:
      offset += dissect_ccc(tn3270_tree, tvb, offset);
      proto_tree_add_item(tn3270_tree,
                              hf_tn3270_bsc,
                              tvb, offset,
                              2,
                              FALSE);
      offset+=2;
      break;
    case W:
    case EW:
    case EWA:
    case EAU:
    case SNA_W:
    case SNA_EW:
    case SNA_EWA:
    case SNA_EAU:
      /* WCC */
      if ((offset - start) < (sf_length - 3))
        offset += dissect_wcc(tn3270_tree, tvb, offset);
      if ((offset - start) < (sf_length - 3))
        offset += dissect_orders_and_data(tn3270_tree, tvb, offset);
      break;
    default:
      break;
  }

  offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, sf_length);

  return (offset - start);
}

/* 5.17 Present Absolute Format */
static gint
dissect_present_absolute_format(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                                gint sf_length)
{
  int start=offset;

  proto_tree_add_item(tn3270_tree,
                          hf_tn3270_partition_id,
                          tvb, offset,
                          1,
                          FALSE);
  offset++;

  proto_tree_add_item(tn3270_tree,
                          hf_tn3270_fpc,
                          tvb, offset,
                          1,
                          FALSE);
  offset++;

  offset+=dissect_wcc(tn3270_tree, tvb, offset);

  proto_tree_add_item(tn3270_tree,
                          hf_tn3270_format_name,
                          tvb, offset,
                          (sf_length - 5),
                          FALSE);
  offset+=(sf_length - 5);
  return (offset - start);
}

/* 5.18 Present Relative Format */
static gint
dissect_present_relative_format(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                                gint sf_length)
{
  int start=offset;

  proto_tree_add_item(tn3270_tree,
                          hf_tn3270_partition_id,
                          tvb, offset,
                          1,
                          FALSE);
  offset++;

  proto_tree_add_item(tn3270_tree,
                          hf_tn3270_fov,
                          tvb, offset,
                          2,
                          FALSE);
  offset+=2;

  proto_tree_add_item(tn3270_tree,
                          hf_tn3270_fpc,
                          tvb, offset,
                          1,
                          FALSE);
  offset++;

  offset += dissect_wcc(tn3270_tree, tvb, offset);

  proto_tree_add_item(tn3270_tree,
                          hf_tn3270_format_name,
                          tvb, offset,
                          (sf_length - 7),
                          FALSE);
  offset+=(sf_length - 7);
  return (offset - start);
}

/* 5.19 Read Partition */
static gint
dissect_read_partition(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                       gint sf_length)
{
  int start=offset;
  int type;

  proto_tree_add_item(tn3270_tree,
                          hf_tn3270_partition_id,
                          tvb, offset,
                          1,
                          FALSE);
  offset++;

  type = tvb_get_guint8(tvb, offset);
  if (type == 0xFF) { /* Partition ID of 0xFF is escaped with another 0xFF */
    offset++;
    type = tvb_get_guint8(tvb, offset);
  }

  proto_tree_add_item(tn3270_tree,
                          hf_tn3270_operation_type,
                          tvb, offset,
                          1,
                          FALSE);
  offset++;

  if (type == 0x03) { /* 'Query List' */
    proto_tree_add_item(tn3270_tree,
                            hf_tn3270_reqtyp,
                            tvb, offset,
                            1,
                            FALSE);
    offset++;

    proto_tree_add_item(tn3270_tree,
                            hf_tn3270_sf_query_replies,
                            tvb, offset,
                            (sf_length - 6),
                            FALSE);
    offset+=(sf_length - 6);
  }
  return (offset - start);
}

/*5.20 Request Recovery Data - Search for REQUEST_RECOVERY_DATA*/
/*5.21 Reset Partition - Search for RESET_PARTITION */

/*5.22 Restart */
static gint
dissect_restart(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                       gint sf_length)
{
  int start=offset;

  proto_tree_add_item(tn3270_tree,
                          hf_tn3270_resbyte,
                          tvb, offset,
                          1,
                          FALSE);
  offset++;

  proto_tree_add_item(tn3270_tree,
                          hf_tn3270_start_page,
                          tvb, offset,
                          2,
                          FALSE);
  offset+=2;

  proto_tree_add_item(tn3270_tree,
                          hf_tn3270_start_line,
                          tvb, offset,
                          2,
                          FALSE);
  offset+=2;


  proto_tree_add_item(tn3270_tree,
                          hf_tn3270_scs_data,
                          tvb, offset,
                          (sf_length - 9),
                          FALSE);
  offset+=(sf_length - 9);
  
  return (offset - start);
}

/* 5.23 SCS Data - Search for SCS_DATA */
/* 5.24 Color Table - Search for COLOR_TABLE */
/* 5.25 Format Group - Search for FORMAT_GROUP */
/* 5.26 Set Checkpoint Interval - Search for CHECKPOINT_INTERVAL */

/* 5.27 Set MSR Control */
static gint
dissect_set_msr_control(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                        gint sf_length)
{
  int start=offset;

  static const int *byte[] = {
     &hf_tn3270_msr_user,
     &hf_tn3270_msr_locked,
     &hf_tn3270_msr_auto,
     &hf_tn3270_msr_ind1,
     &hf_tn3270_msr_ind2,
     NULL
  };

  hf_items outbound_text_header_fields[] = {
      { hf_tn3270_partition_id, 0, 1, 0 },
      { hf_tn3270_msr_type, 0, 1, 0 },
      { hf_tn3270_msr_state_mask, ett_tn3270_msr_state_mask, 1, byte },
      { hf_tn3270_msr_state_value, 1, 1, 0 },
      { hf_tn3270_msr_ind_mask, 1, 1, 0 },
      { hf_tn3270_msr_ind_value, 1, 1, 0 },
      { 0, 0, 0, 0 },
  };


  offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                outbound_text_header_fields);

  offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, sf_length);

  return (offset - start);
}

/* 5.28 Set Partition Characteristics */
static gint
dissect_set_partition_characteristics_sd_parms(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset)
{

  int start=offset;
  guint16 sdp;

  hf_items sdp1[] = {
    { hf_tn3270_sdp_ln, 0, 1, 0 },
    { hf_tn3270_sdp_id, 0, 1, 0 },
    { hf_tn3270_spc_sdp_ot, 0, 1, 0 },
    { hf_tn3270_spc_sdp_ob, 0, 1, 0 },
    { hf_tn3270_spc_sdp_ol, 0, 1, 0 },
    { hf_tn3270_spc_sdp_or, 0, 1, 0 },
    { 0, 0, 0, 0 },
  };

  hf_items sdp2[] = {
    { hf_tn3270_sdp_ln, 0, 1, 0 },
    { hf_tn3270_sdp_id, 0, 1, 0 },
    { hf_tn3270_spc_sdp_eucflags, 0, 1, 0 },
    { 0, 0, 0, 0 },
  };

  hf_items sdp3[] = {
    { hf_tn3270_sdp_ln, 0, 1, 0 },
    { hf_tn3270_sdp_id, 0, 1, 0 },
    { hf_tn3270_spc_sdp_eucflags, 0, 1, 0 },
    { hf_tn3270_spc_sdp_eucflags, 0, 1, 0 },
    { 0, 0, 0, 0 },
  };


  sdp = tvb_get_ntohs(tvb, offset);

  switch (sdp) {
    case 0x0601: /*View Outport*/
      offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                    sdp1);
      break;
    case 0x0304: /*Enable User Call Up*/
      offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                    sdp2);
      break;
    case 0x0405: /*Select Base Character Set*/
      offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                    sdp3);
      break;
    default:
      return 0;
  }

  return (offset - start);

}

static gint
dissect_set_partition_characteristics(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                                      gint sf_length)
{

  int start=offset;
  int i;

  hf_items fields[] = {
    { hf_tn3270_partition_id, 0, 1, 0 },
    { hf_tn3270_resbytes, 0, 2, 0 },
    { 0, 0, 0, 0 },
  };


  offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                fields);

  for (i = 0; i < 3; i++) {
    offset += dissect_set_partition_characteristics_sd_parms(tn3270_tree, tvb, offset);
    if (!tvb_length_remaining(tvb, offset))
      break;
  }

  offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, sf_length);

  return (offset - start);
}

/* 5.29 Set Printer Characteristics */
static gint
dissect_set_printer_characteristics_sd_parms(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset)
{

  int start=offset;
  guint16 sdp;

  hf_items sdp1[] = {
    { hf_tn3270_sdp_ln, 0, 1, 0 },
    { hf_tn3270_sdp_id, 0, 1, 0 },
    { hf_tn3270_spc_sdp_srepc, 0, 1, 0 },
    { 0, 0, 0, 0 },
  };

  sdp = tvb_get_ntohs(tvb, offset);

  switch (sdp) {
    case 0x0301: /*Early Print Complete*/
      offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                    sdp1);
      break;
    default:
      return 0;
  }

  return (offset - start);

}

static gint
dissect_set_printer_characteristics(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                                      gint sf_length)
{

  int start=offset;
  int i;

  hf_items fields[] = {
    { hf_tn3270_printer_flags, 0, 1, 0 },
    { hf_tn3270_resbyte, 0, 1, 0 },
    { 0, 0, 0, 0 },
  };

  offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                fields);

  for (i = 0; i < 3; i++) {
    offset += dissect_set_printer_characteristics_sd_parms(tn3270_tree, tvb, offset);
    if (!tvb_length_remaining(tvb, offset))
      break;
  }

  offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, sf_length);

  return (offset - start);
}


/* 5.30 Set Reply Mode */
static gint
dissect_set_reply_mode(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                       gint sf_length)
{
  int start=offset;
  int type;

  hf_items fields[] = {
    { hf_tn3270_partition_id, 0, 1, 0 },
    { hf_tn3270_mode, 0, 1, 0 },
    { 0, 0, 0, 0 },
  };

  type = tvb_get_guint8(tvb, offset+1);

  offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                fields);

  if (type == 0x02) { /* 'Query List' */
    proto_tree_add_item(tn3270_tree,
                            hf_tn3270_reply_mode_attr_list,
                            tvb, offset,
                            (sf_length - 5),
                            FALSE);
    offset+=(sf_length - 5);
  }
  return (offset - start);
}

/* 5.31 Set Window Origin - Search for SET_WINDOW_ORIGIN */
/* 6.6 Type 1 Text Inbound
   5.32 Type 1 Text Outbound */
static gint
dissect_type_1_text(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                             gint sf_length)
{
  int start=offset;

  hf_items fields[] = {
    { hf_tn3270_partition_id, 0, 1, 0 },
    { hf_tn3270_resbytes, 0, 2, 0 },
    { 0, 0, 0, 0 },
  };

  offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                fields);
  proto_tree_add_item(tn3270_tree,
                          hf_tn3270_field_data,
                          tvb, offset,
                          (sf_length - 7),
                          FALSE);
  offset+=(sf_length - 7);
  return (offset - start);
}

/* 5.34 Data Chain - Search for DATA_CHAIN*/
/* 5.35 Destination/Origin -  Search for DESTINATION_OR_ORIGIN*/

/* 5.36 Object Control */
static gint
dissect_object_control(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                       gint sf_length)
{
  int start=offset;

  hf_items fields[] = {
    { hf_tn3270_partition_id, 0, 1, 0 },
    { hf_tn3270_object_control_flags, 0, 1, 0 },
    { hf_tn3270_object_type, 0, 1, 0 },
    { 0, 0, 0, 0 },
  };

  offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                fields);

  proto_tree_add_item(tn3270_tree,
                          hf_tn3270_type_1_text_outbound_data,
                          tvb, offset,
                          (sf_length - 7),
                          FALSE);
  offset+=(sf_length - 7);
  return (offset - start);
}

/* 5.37 Object Data - Search for OBJECT_DATA*/
/* 5.38 Object Picture - Search for OBJECT_PICTURE */
/* 5.39 OEM Data - Search for OEM_DATA */

/* 5.40 Save/Restore Format */
static gint
dissect_save_or_restore_format(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                               gint sf_length)
{
  int start=offset;

  hf_items fields[] = {
    { hf_tn3270_save_or_restore_format_flags, 0, 1, 0 },
    { hf_tn3270_srf_fpcb, 0, (sf_length-5), 0 },
    { 0, 0, 0, 0 },
  };

  offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                fields);

  offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, sf_length);

  return (offset - start);
}

/* 5.41 Select Intelligent Printer Data Stream (IPDS) Mode -  Search for SELECT_IPDS_MODE*/

/* -----------------------------------------*/
/* 6.0 CHAPTER 6. INBOUND STRUCTURED FIELDS */
/* -----------------------------------------*/

/* 6.2 Exception/Status */
static gint
dissect_exception_or_status_sd_parms(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset)
{

  int start=offset;
  guint16 sdp;

  hf_items sdp1[] = {
    { hf_tn3270_sdp_ln, 0, 1, 0 },
    { hf_tn3270_sdp_id, 0, 1, 0 },
    { hf_tn3270_resbyte, 0, 1, 0 },
    { hf_tn3270_sdp_excode, 0, 2, 0 },
    { 0, 0, 0, 0 },
  };

  hf_items sdp2[] = {
    { hf_tn3270_sdp_ln, 0, 1, 0 },
    { hf_tn3270_sdp_id, 0, 1, 0 },
    { hf_tn3270_sdp_statcode, 0, 2, 0 },
    { 0, 0, 0, 0 },
  };

  hf_items sdp3[] = {
    { hf_tn3270_sdp_ln, 0, 1, 0 },
    { hf_tn3270_sdp_id, 0, 1, 0 },
    { hf_tn3270_format_group, 0, 16, 0 },
    { hf_tn3270_format_name, 0, 16, 0 },
    { 0, 0, 0, 0 },
  };

  hf_items sdp4[] = {
    { hf_tn3270_sdp_ln, 0, 1, 0 },
    { hf_tn3270_sdp_id, 0, 1, 0 },
    { hf_tn3270_sdp_ngl, 0, 2, 0 },
    { hf_tn3270_sdp_nml, 0, 2, 0 },
    { hf_tn3270_sdp_nlml, 0, 2, 0 },
    { hf_tn3270_sdp_stor, 0, 4, 0 },
    { 0, 0, 0, 0 },
  };

  hf_items sdp5[] = {
    { hf_tn3270_sdp_ln, 0, 1, 0 },
    { hf_tn3270_sdp_id, 0, 1, 0 },
    { hf_tn3270_format_group, 0, 16, 0 },
    { hf_tn3270_sdp_nml, 0, 2, 0 },
    { 0, 0, 0, 0 },
  };

  sdp = tvb_get_ntohs(tvb, offset);

  switch (sdp) {
    case 0x0601: /*Auxiliary Device Exception*/
      offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                    sdp1);
      break;
    case 0x0402: /*Auxiliary Device status*/
      offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                    sdp2);
      break;
    case 0x2203: /*Failing Format status*/
      offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                    sdp3);
      break;
    case 0x0C04: /*Format status*/
      offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                    sdp4);
    case 0x1405: /*Group status*/
      offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                    sdp5);
      break;
    default:
      return 0;
  }

  return (offset - start);

}

static gint
dissect_exception_or_status(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                               gint sf_length)
{
  int start=offset, i;

  hf_items fields[] = {
    { hf_tn3270_partition_id, 0, 1, 0 },
    { hf_tn3270_exception_or_status_flags, 0, 1, 0 },
    { hf_tn3270_resbyte, 0, 1, 0 },
    { 0, 0, 0, 0 },
  };

  offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                fields);

  for (i = 0; i < 5; i++) {
    offset += dissect_exception_or_status_sd_parms(tn3270_tree, tvb, offset);
    if (!tvb_length_remaining(tvb, offset))
      break;
  }

  offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, sf_length);

  return (offset - start);
}

/* 6.3 Inbound Text Header */
static gint
dissect_inbound_text_header(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                            gint sf_length)
{
  int start=offset;

  hf_items outbound_text_header_fields[] = {
      { hf_tn3270_partition_id, 0, 1, 0 },
      { hf_tn3270_aid, 0, 1, 0 },
      { hf_tn3270_resbyte, 0, 1, 0 },
      { hf_tn3270_resbyte, 0, 1, 0 },
      { hf_tn3270_resbyte, 0, 1, 0 },
      { hf_tn3270_lvl, 0, 1, 0 },
      { hf_tn3270_cro, 0, 2, 0 },
      { hf_tn3270_cc, 0, 2, 0 },
      { hf_tn3270_rw, 0, 2, 0 },
      { hf_tn3270_cw, 0, 2, 0 },
      { hf_tn3270_hw, 0, 2, 0 },
      { hf_tn3270_ww, 0, 2, 0 },
      { 0, 0, 0, 0 },
  };


  offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                outbound_text_header_fields);

  offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, sf_length);

  return (offset - start);
}

/* 6.4 Inbound 3270DS */
static gint
dissect_inbound_3270ds(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                               gint sf_length)
{
  int start=offset;

  hf_items fields[] = {
    { hf_tn3270_partition_id, 0, 1, 0 },
    { hf_tn3270_aid, 0, 1, 0 },
    { hf_tn3270_cursor_x, 0, 1, 0 },
    { hf_tn3270_cursor_y, 0, 1, 0 },
    { hf_tn3270_field_data, 0, (sf_length - 8), 0 },
    { 0, 0, 0, 0 },
  };

  offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                fields);

  return (offset - start);
}



/* 6.5 Recovery Data */
static gint
dissect_recovery_data(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                      gint sf_length)
{
  int start=offset;


  hf_items fields[] = {
      { hf_tn3270_resbyte, 0, 1, 0 },
      { hf_tn3270_recovery_data_flags, 1, 1, 0 },
      { hf_tn3270_sld, 0, 1, 0 },
      { hf_tn3270_charset, 0, 1, 0 },
      { hf_tn3270_vertical, 0, 2, 0 },
      { hf_tn3270_v_offset, 0, 2, 0 },
      { hf_tn3270_v_sequence, 0, 2, 0 },
      { hf_tn3270_v_length, 0, 2, 0 },
      { hf_tn3270_spd, 0, 2, 0 },
      { hf_tn3270_horizon, 0, 2, 0 },
      { hf_tn3270_h_offset, 0, 2, 0 },
      { hf_tn3270_h_sequence, 0, 2, 0 },
      { hf_tn3270_h_length, 0, 2, 0 },
      { hf_tn3270_color, 0, 1, 0 },
      { hf_tn3270_hilite, 0, 1, 0 },
      { hf_tn3270_pages, 0, 2, 0 },
      { hf_tn3270_lines, 0, 2, 0 },
      { hf_tn3270_checkpoint, 0, 2, 0 },
      { hf_tn3270_c_offset, 0, 2, 0 },
      { hf_tn3270_c_sequence, 0, 2, 0 },
      { hf_tn3270_c_seqoff, 0, 2, 0 },
      { hf_tn3270_c_scsoff, 0, 2, 0 },
      { hf_tn3270_prime, 0, 1, 0 },
      { 0, 0, 0, 0 },
  };


  offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                fields);

  offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, sf_length);

  return (offset - start);
}

/* 6.6 Query Reply (Type 1 Text Inbound) - See above*/
/* 6.7 and 6.8 Query Reply - Introductory Matter */

/* 6.9 Query Reply (Alphanumeric Partitions) */
static gint
dissect_query_reply_alphanumeric_sd_parms(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset)
{

  int start=offset;
  guint16 sdp;

  hf_items sdp1[] = {
    { hf_tn3270_sdp_ln, 0, 1, 0 },
    { hf_tn3270_sdp_id, 0, 1, 0 },
    { hf_tn3270_ap_cm, 0, 1, 0 },
    { hf_tn3270_ap_ro, 0, 1, 0 },
    { hf_tn3270_ap_co, 0, 1, 0 },
    { hf_tn3270_ap_fo, 0, 2, 0 },
    { 0, 0, 0, 0 },
  };


  sdp = tvb_get_ntohs(tvb, offset);

  switch (sdp) {
    case 0x0702: /*Buffer Allocation*/
      offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                    sdp1);
      break;
    default:
      return 0;
  }

  return (offset - start);

}

static gint
dissect_query_reply_alphanumeric(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                                 gint sf_length)
{
  int start=offset;

  static const int *byte[] = {
    &hf_tn3270_ap_vertical_scrolling,
    &hf_tn3270_ap_horizontal_scrolling,
    &hf_tn3270_ap_apres1,
    &hf_tn3270_ap_apa,
    &hf_tn3270_ap_pp,
    &hf_tn3270_ap_lc,
    &hf_tn3270_ap_mp,
    &hf_tn3270_ap_apres2,
    NULL
    };

  hf_items fields[] = {
      { hf_tn3270_ap_na, 0, 1, 0 },
      { hf_tn3270_ap_m, 0, 2, 0 },
      { hf_tn3270_query_reply_alphanumeric_flags, ett_tn3270_query_reply_alphanumeric_flags, 1, byte },
      { 0, 0, 0, 0 },
  };

  offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                fields);

  offset += dissect_query_reply_alphanumeric_sd_parms(tn3270_tree, tvb, offset);

  offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, sf_length);

  return (offset - start);
}

/* 6.10 Query Reply (Auxiliary Device) - Search for QUERY_REPLY_AUXILIARY_DEVICE */
/* 6.11 Query Reply (BEGIN/End of File ) - Search for QUERY_REPLY_BEGIN_OR_END_OF_FILE */

/* 6.12 Query Reply (Character Sets) */
static gint
dissect_query_reply_character_sets(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                                   gint sf_length)
{
  int start=offset;
  int flagbyte1, flagbyte2;

  static const int *byte1[] = {
    &hf_tn3270_cs_ge,
    &hf_tn3270_cs_mi,
    &hf_tn3270_cs_lps,
    &hf_tn3270_cs_lpse,
    &hf_tn3270_cs_ms,
    &hf_tn3270_cs_ch2,
    &hf_tn3270_cs_gf,
    &hf_tn3270_cs_res,
    NULL
    };

  static const int *byte2[] = {
    &hf_tn3270_cs_res2,
    &hf_tn3270_cs_pscs,
    &hf_tn3270_cs_res3,
    &hf_tn3270_cs_cf,
    NULL
    };

  static const int *byte3[] = {
    &hf_tn3270_cs_form_type1,
    &hf_tn3270_cs_form_type2,
    &hf_tn3270_cs_form_type3,
    &hf_tn3270_cs_form_type4,
    &hf_tn3270_cs_form_type5,
    &hf_tn3270_cs_form_type6,
    &hf_tn3270_cs_form_type8,
    NULL
    };

  static const int *byte4[] = {
    &hf_tn3270_cs_ds_load,
    &hf_tn3270_cs_ds_triple,
    &hf_tn3270_cs_ds_char,
    &hf_tn3270_cs_ds_cb,
    NULL
    };


  hf_items fields[] = {
      { hf_tn3270_character_sets_flags1, ett_tn3270_character_sets_flags1, 1, byte1 },
      { hf_tn3270_character_sets_flags2, ett_tn3270_character_sets_flags2, 1, byte2 },
      { hf_tn3270_sdw, 0, 1, 0 },
      { hf_tn3270_sdh, 0, 1, 0 },
      { hf_tn3270_form, ett_tn3270_character_sets_form, 1, byte3 },
      { hf_tn3270_formres, 0, 1, 0 },
      { hf_tn3270_formres, 0, 1, 0 },
      { hf_tn3270_formres, 0, 1, 0 },
      { hf_tn3270_cs_dl, 0, 1, 0 },
      { 0, 0, 0, 0 },
  };

  hf_items descriptors[] = {
      { hf_tn3270_cs_descriptor_set, 0, 1, 0 },
      { hf_tn3270_cs_descriptor_flags, ett_tn3270_cs_descriptor_flags, 1, byte4 },
      { hf_tn3270_lcid, 0, 1, 0 },
      { 0, 0, 0, 0 },
  };

  hf_items sw_sh[] = {
      { hf_tn3270_sw, 0, 1, 0 },
      { hf_tn3270_sh, 0, 1, 0 },
      { 0, 0, 0, 0 },
  };

  hf_items subsn[] = {
      { hf_tn3270_ssubsn, 0, 1, 0 },
      { hf_tn3270_esubsn, 0, 1, 0 },
      { 0, 0, 0, 0 },
  };

  hf_items gf[] = {
      { hf_tn3270_ccsgid, 0, 4, 0 },
      { 0, 0, 0, 0 },
  };

  hf_items cf[] = {
      { hf_tn3270_ccsid, 0, 2, 0 },
      { 0, 0, 0, 0 },
  };

  flagbyte1 = tvb_get_guint8(tvb, offset);
  flagbyte2 = tvb_get_guint8(tvb, offset+1);

  offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                fields);

  while ((offset - start) < (sf_length - 4)) {

    offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                descriptors);

    if (flagbyte1 & MS) {
      offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                  sw_sh);
    }

    if (flagbyte1 & CH2) {
      offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                  subsn);
    }

    if (flagbyte1 & GF) {
      offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                  gf);
    }

    if (flagbyte2 & CF) {
      offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                  cf);
    }
  }
  offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, sf_length);

  return (offset - start);
}

/* 6.13 Query Reply (Color) */
static gint
dissect_query_reply_color_sd_parms(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset)
{

  int start=offset;
  guint16 sdp;

  hf_items sdp1[] = {
    { hf_tn3270_sdp_ln, 0, 1, 0 },
    { hf_tn3270_sdp_id, 0, 1, 0 },
    { hf_tn3270_db_cavdef, 0, 1, 0 },
    { hf_tn3270_db_cidef, 0, 1, 0 },
    { 0, 0, 0, 0 },
  };


  sdp = tvb_get_ntohs(tvb, offset);

  switch (sdp) {
    case 0x0402: /*Default Background Color*/
      offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                    sdp1);
      break;
    default:
      return 0;
  }

  return (offset - start);

}

static gint
dissect_query_reply_color(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                          gint sf_length)
{
  int start=offset;
  int i;
  int np;

  static const int *byte[] = {
    &hf_tn3270_c_prtblk,
    NULL
    };

  hf_items fields[] = {
    { hf_tn3270_color_flags, ett_tn3270_color_flags, 1, byte },
    { hf_tn3270_c_np, 0, 1, 0 },
    { 0, 0, 0, 0 },
  };


  np = tvb_get_guint8(tvb, offset +1);
  offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                fields);

  for (i=0; i < np; i++) {
    if (tvb_get_guint8(tvb, offset) == 0xFF) {
        offset++;
    }
    proto_tree_add_item(tn3270_tree,
                            hf_tn3270_c_cav,
                            tvb, offset,
                            1,
                            FALSE);
    offset++;
    if (tvb_get_guint8(tvb, offset) == 0xFF) {
        offset++;
    }
    proto_tree_add_item(tn3270_tree,
                            hf_tn3270_c_ci,
                            tvb, offset,
                            1,
                            FALSE);
    offset++;
  }
  offset += dissect_query_reply_color_sd_parms(tn3270_tree, tvb, offset);

  offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, sf_length);

  return (offset - start);
}


/* 6.36 - Query Reply (OEM Auxiliary Device) Self-Defining Parameters */
static gint
dissect_daid_sd_parm(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset)
{

  int start=offset;

  hf_items sdp1[] = {
    { hf_tn3270_sdp_ln, 0, 1, 0 },
    { hf_tn3270_sdp_id, 0, 1, 0 },
    { hf_tn3270_sdp_daid, 0, 2, 0 },
    { 0, 0, 0, 0 },
  };

  offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                sdp1);
  return (offset - start);

}

static gint
dissect_pclk_sd_parm(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset)
{

  int start=offset;

  hf_items sdp1[] = {
    { hf_tn3270_sdp_ln, 0, 1, 0 },
    { hf_tn3270_sdp_id, 0, 1, 0 },
    { hf_tn3270_oem_sdp_pclk_vers, 0, 2, 0 },
    { 0, 0, 0, 0 },
  };

  offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                sdp1);
  return (offset - start);

}

static gint
dissect_query_reply_oem_auxiliary_device_sd_parms(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset)
{

  int start=offset;
  int sdp_len;
  int sdp;

  hf_items sdp1[] = {
    { hf_tn3270_sdp_ln, 0, 1, 0 },
    { hf_tn3270_sdp_id, 0, 1, 0 },
    { hf_tn3270_sdp_daid, 0, 2, 0 },
    { 0, 0, 0, 0 },
  };

  hf_items sdp2[] = {
    { hf_tn3270_sdp_ln, 0, 1, 0 },
    { hf_tn3270_sdp_id, 0, 1, 0 },
    { hf_tn3270_oem_sdp_ll_limin, 0, 2, 0 },
    { hf_tn3270_oem_sdp_ll_limout, 0, 2, 0 },

    { 0, 0, 0, 0 },
  };

  hf_items sdp3[] = {
    { hf_tn3270_sdp_ln, 0, 1, 0 },
    { hf_tn3270_sdp_id, 0, 1, 0 },
    { hf_tn3270_oem_sdp_pclk_vers, 0, 2, 0 },
    { 0, 0, 0, 0 },
  };


  sdp_len = tvb_get_guint8(tvb, offset);
  if ((sdp_len != 0x04) && (sdp_len != 0x06)) {
    return 0;
  }

  sdp = tvb_get_guint8(tvb, offset+1);

  switch (sdp) {
    case 0x01:
      offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                    sdp1);
      break;
    case 0x02:
      offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                    sdp2);
      break;
    case 0x03:
      offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                    sdp3);
      break;
    default:
      return 0;
  }

  return (offset - start);

}

/* 6.14 - Query Reply (Cooperative Processing Requestor) */
static gint
dissect_query_reply_cooperative(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                                gint sf_length)
{
  int start=offset;

  hf_items fields[] = {
      { hf_tn3270_res_twobytes, 0, 2, 0 },
      { hf_tn3270_limin, 0, 2, 0 },
      { hf_tn3270_limout, 0, 2, 0 },
      { hf_tn3270_featl, 0, 1, 0 },
      { hf_tn3270_feats, 0, 2, 0 },
      { 0, 0, 0, 0 },
  };


  offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                fields);

  /*FIXME: Need to see this in action to dissect in detail */
  proto_tree_add_item(tn3270_tree,
                          hf_tn3270_field_data,
                          tvb, offset,
                          (sf_length - 13),
                          FALSE);
  offset+=(sf_length - 13);

  /* Uses same Self-Defining Parm as OEM Auxiliary Device */
  offset += dissect_query_reply_oem_auxiliary_device_sd_parms(tn3270_tree, tvb, offset);

  offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, sf_length);

  return (offset - start);
}

/* 6.15 - Query Reply (Data Chaining) */
static gint
dissect_query_reply_data_chaining(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                                  gint sf_length)
{
  int start=offset;

  static const int *byte1[] = {
    &hf_tn3270_dc_both,
    &hf_tn3270_dc_from_device,
    &hf_tn3270_dc_to_device,
    NULL
    };

  hf_items fields[] = {
      { hf_tn3270_dc_dir_flags, ett_tn3270_dc_dir_flags, 1, byte1 },
      { hf_tn3270_resbyte, 0, 1, 0 },
      { 0, 0, 0, 0 },
  };

  offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                fields);

  offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, sf_length);

  return (offset - start);
}

/* 6.16 - Query Reply (Data Streams) */

static gint
dissect_query_reply_data_streams(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                                  gint sf_length)
{
  int start=offset;
  int i;

  proto_tree_add_item(tn3270_tree, hf_tn3270_ds_default_sfid, tvb, offset, 1,
                      FALSE);
  for (i=0; i < (sf_length - 4); i++) {
    proto_tree_add_item(tn3270_tree,
                            hf_tn3270_ds_sfid,
                            tvb, offset,
                            1,
                            FALSE);
    offset++;
  }
  offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, sf_length);
  return (offset - start);
}

/* 6.17 - Query Reply (DBCS Asia) */

static gint
dissect_query_reply_dbcs_asia_sd_parms(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset)
{

  int start=offset;
  int sdp_len;
  int sdp;

  hf_items sdp1[] = {
    { hf_tn3270_sdp_ln, 0, 1, 0 },
    { hf_tn3270_sdp_id, 0, 1, 0 },
    { hf_tn3270_asia_sdp_sosi_soset, 0, 1, 0 },
    { 0, 0, 0, 0 },
  };

  hf_items sdp2[] = {
    { hf_tn3270_sdp_ln, 0, 1, 0 },
    { hf_tn3270_sdp_id, 0, 1, 0 },
    { hf_tn3270_asia_sdp_ic_func, 0, 1, 0 },
    { 0, 0, 0, 0 },
  };

  sdp_len = tvb_get_guint8(tvb, offset);
  if (sdp_len != 0x03) {
    return 0;
  }

  sdp = tvb_get_guint8(tvb, offset+1);

  switch (sdp) {
    case 0x01: /*SO/SI*/
      offset += tn3270_add_hf_items(tn3270_tree, tvb, offset, sdp1);
      break;
    case 0x02: /*Input Control*/
      offset += tn3270_add_hf_items(tn3270_tree, tvb, offset, sdp2);
      break;
    default:
      return 0;
  }

  return (offset - start);

}

static gint
dissect_query_reply_dbcs_asia(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                                  gint sf_length)
{

  int start=offset;
  int i;

  hf_items fields[] = {
    { hf_tn3270_resbyte, 0, 1, 0 },
    { 0, 0, 0, 0 },
  };


  offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                fields);

  for (i = 0; i < 3; i++) {
    offset += dissect_query_reply_dbcs_asia_sd_parms(tn3270_tree, tvb, offset);
    if (!tvb_length_remaining(tvb, offset))
      break;
  }

  offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, sf_length);

  return (offset - start);
}

/* 6.18 - Query Reply (Device Characteristics) */
static gint
dissect_query_reply_device_characteristics(proto_tree *tn3270_tree, tvbuff_t *tvb,
                                           gint offset, gint sf_length)
{
  int start=offset;

  proto_tree_add_item(tn3270_tree,
                          hf_tn3270_sf_outbound_id,
                          tvb, offset,
                          1,
                          FALSE);
  offset++;

  /* TODO: dissect descriptors */
  offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, sf_length);

  return (offset - start);
}

/* 6.19 - Query Reply (Distributed Data Management) */
static gint
dissect_query_reply_distributed_data_management(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                                                gint sf_length)
{
  int start=offset, i;
  int sdp;
  int done = 0;

  hf_items fields[] = {
    { hf_tn3270_ddm_flags,0, 1, 0 },
    { hf_tn3270_ddm_flags,0, 1, 0 },
    { hf_tn3270_ddm_limin,0, 2, 0 },
    { hf_tn3270_ddm_limout,0, 2, 0 },
    { hf_tn3270_ddm_nss,0, 1, 0 },
    { hf_tn3270_ddm_ddmss,0, 1, 0 },
    { 0, 0, 0, 0 },
  };

  offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                fields);

  for (i = 0; i < 3; i++) {
    sdp = tvb_get_guint8(tvb, offset+1);
    switch (sdp) {
      case 0x02: /*DDM*/
        /*TODO: DDM */
        offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, tvb_get_guint8(tvb,offset));
        break;
      case 0x01: /*DAID*/
        offset += dissect_daid_sd_parm(tn3270_tree, tvb, offset);
        break;
      case 0x03: /*PCLK*/
        offset += dissect_pclk_sd_parm(tn3270_tree, tvb, offset);
        break;
      default:
        done = 1;
        break;
    }
    if (!tvb_length_remaining(tvb, offset) || done)
      break;
  }

  offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, sf_length);

  return (offset - start);
}

/* 6.20 - Query Reply (Document Interchange Architecture) */
static gint
dissect_query_reply_document_interchange_architecture(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                                                      gint sf_length)
{
  int start=offset, sdp, ln, i;

  hf_items fields[] = {
    { hf_tn3270_dia_flags, 0, 2, 0 },
    { hf_tn3270_dia_limin, 0, 2, 0 },
    { hf_tn3270_dia_limout, 0, 2, 0 },
    { 0, 0, 0, 0 },
  };

  offset += tn3270_add_hf_items(tn3270_tree, tvb, offset, fields);

  ln = tvb_get_guint8(tvb, offset);
  proto_tree_add_item(tn3270_tree, hf_tn3270_dia_nfs,tvb, offset, 1, FALSE);

  for (i=0; i < ln; i++) {
    proto_tree_add_item(tn3270_tree, hf_tn3270_dia_diafs, tvb, offset, 1,
                        FALSE);
    offset++;
    proto_tree_add_item(tn3270_tree, hf_tn3270_dia_diafn, tvb, offset, 2,
                        FALSE);
    offset++;
  }

  sdp = tvb_get_guint8(tvb, offset+1);
  if (sdp == 0x01) { /*DAID*/
     offset += dissect_daid_sd_parm(tn3270_tree, tvb, offset);
  }

  offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, sf_length);

  return (offset - start);
}

/* 6.21 - Query Reply (Extended Drawing Routine) */
static gint
dissect_query_reply_extended_drawing_routine(proto_tree *tn3270_tree, tvbuff_t *tvb,
                                             gint offset, gint sf_length)
{
  int start=offset;

  proto_tree_add_item(tn3270_tree, hf_tn3270_field_data ,tvb, offset,
                      (sf_length-4), FALSE);

  offset += (sf_length - 4);

  return (offset - start);
}

/* 6.22 - Query Reply (Field Outlining) */
static gint
dissect_query_reply_field_outlining(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                                    gint sf_length)
{
  int start=offset;

  hf_items fields[] = {
    { hf_tn3270_resbyte, 0, 1, 0 },
    { hf_tn3270_fo_flags, 0, 1, 0 },
    { hf_tn3270_fo_vpos, 0, 1, 0 },
    { hf_tn3270_fo_hpos, 0, 1, 0 },
    { hf_tn3270_fo_hpos0, 0, 1, 0 },
    { hf_tn3270_fo_hpos1, 0, 1, 0 },
    { 0, 0, 0, 0 },
  };

  offset += tn3270_add_hf_items(tn3270_tree, tvb, offset, fields);

  offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, sf_length);

  return (offset - start);
}

/* 6.23 - Query Reply (Field Validation) - Search for FIELD_VALIDATION*/
/* 6.24 - Query Reply (Format Presentation) - Search for FORMAT_PRESENTATION*/

/* 6.25 - Query Reply (Format Storage Auxiliary Device)*/
static gint
dissect_query_reply_format_storage_aux_device(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                                              gint sf_length)
{
  int start=offset, sdp;

  hf_items fields[] = {
    { hf_tn3270_fsad_flags, 0, 1, 0 },
    { hf_tn3270_resbyte, 0, 1, 0 },
    { hf_tn3270_fsad_limin, 0, 2, 0 },
    { hf_tn3270_fsad_limout, 0, 2, 0 },
    { 0, 0, 0, 0 },
  };

  offset += tn3270_add_hf_items(tn3270_tree, tvb, offset, fields);

  sdp = tvb_get_guint8(tvb, offset+1);
  if (sdp == 0x01) { /*DAID*/
     offset += dissect_daid_sd_parm(tn3270_tree, tvb, offset);
     proto_tree_add_item(tn3270_tree, hf_tn3270_fsad_size ,tvb, offset,
                         2, FALSE);
     offset+=2;
  }

  offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, sf_length);

  return (offset - start);
}

/* 6.26 - Query Reply (Graphic Color) - Search for GRAPHIC_COLOR*/
/* 6.27 - Query Reply (Graphic Symbol Sets) - Search for GRAPHIC_SYMBOL_SETS*/

/* 6.28 - Query Reply (Highlighting) */
static gint
dissect_query_reply_highlighting(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                          gint sf_length)
{
  int start=offset;
  int i;
  int np;

  hf_items fields[] = {
    { hf_tn3270_h_np, 0, 1, 0 },
    { 0, 0, 0, 0 },
  };


  np = tvb_get_guint8(tvb, offset);
  offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                fields);

  for (i=0; i < np; i++) {
    if (tvb_get_guint8(tvb, offset) == 0xFF) {
        offset++;
    }
    proto_tree_add_item(tn3270_tree,
                            hf_tn3270_h_vi,
                            tvb, offset,
                            1,
                            FALSE);
    offset++;
    if (tvb_get_guint8(tvb, offset) == 0xFF) {
        offset++;
    }
    proto_tree_add_item(tn3270_tree,
                            hf_tn3270_h_ai,
                            tvb, offset,
                            1,
                            FALSE);
    offset++;
  }

  offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, sf_length);

  return (offset - start);
}

/* 6.29 - Query Reply (IBM Auxiliary Device) */
static gint
dissect_query_reply_ibm_aux_device(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                                                gint sf_length)
{
  int start=offset, i, sdp;
  int done = 0;

  hf_items fields[] = {
    { hf_tn3270_ibm_flags, 0, 1, 0 },
    { hf_tn3270_resbyte, 0, 1, 0 },
    { hf_tn3270_ibm_limin, 0, 2, 0 },
    { hf_tn3270_ibm_limout, 0, 2, 0 },
    { hf_tn3270_ibm_type, 0, 1, 0 },
    { 0, 0, 0, 0 },
  };

  offset += tn3270_add_hf_items(tn3270_tree, tvb, offset, fields);

  for (i = 0; i < 3; i++) {
    sdp = tvb_get_guint8(tvb, offset+1);
    switch (sdp) {
      case 0x02: /*Printer Name*/
        /*TODO: Printer Name */
        offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, tvb_get_guint8(tvb,offset));
        break;
      case 0x01: /*DAID*/
        offset += dissect_daid_sd_parm(tn3270_tree, tvb, offset);
        break;
      case 0x03: /*PCLK*/
        offset += dissect_pclk_sd_parm(tn3270_tree, tvb, offset);
        break;
      default:
        done = 1;
        break;
    }
    if (!tvb_length_remaining(tvb, offset) || done)
      break;
  }

  offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, sf_length);

  return (offset - start);
}

/* 6.30 - Query Reply (Image) */

/* 6.31 - Query Reply (Implicit Partitions) */
static gint
dissect_query_reply_implicit_partitions_sd_parms(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset)
{

  int start=offset;
  int sdp_len;
  int sdp;

  hf_items sdp1[] = {
    { hf_tn3270_sdp_ln, 0, 1, 0 },
    { hf_tn3270_sdp_id, 0, 1, 0 },
    { hf_tn3270_ip_flags, 0, 1, 0 },
    { hf_tn3270_ipdd_wd, 0, 2, 0 },
    { hf_tn3270_ipdd_hd, 0, 2, 0 },
    { hf_tn3270_ipdd_wa, 0, 2, 0 },
    { hf_tn3270_ipdd_ha, 0, 2, 0 },
    { 0, 0, 0, 0 },
  };

  hf_items sdp2[] = {
    { hf_tn3270_sdp_ln, 0, 1, 0 },
    { hf_tn3270_sdp_id, 0, 1, 0 },
    { hf_tn3270_ip_flags, 0, 1, 0 },
    { hf_tn3270_ippd_dpbs, 0, 4, 0 },
    { hf_tn3270_ippd_apbs, 0, 4, 0 },
    { 0, 0, 0, 0 },
  };

  hf_items sdp3[] = {
    { hf_tn3270_sdp_ln, 0, 1, 0 },
    { hf_tn3270_sdp_id, 0, 1, 0 },
    { hf_tn3270_ip_flags, 0, 1, 0 },
    { hf_tn3270_ipccd_wcd, 0, 2, 0 },
    { hf_tn3270_ipccd_hcd, 0, 2, 0 },
    { hf_tn3270_ipccd_wca, 0, 2, 0 },
    { hf_tn3270_ipccd_hca, 0, 2, 0 },
    { 0, 0, 0, 0 },
  };

  sdp_len = tvb_get_guint8(tvb, offset);
  if (sdp_len != 0x0B) {
    return 0;
  }

  sdp = tvb_get_guint8(tvb, offset+1);

  switch (sdp) {
    case DISPLAY:
      offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                    sdp1);
      break;
    case PRINTER:
      offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                    sdp2);
      break;
    case CHARACTER:
      offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                    sdp3);
      break;
    default:
      return 0;
  }

  return (offset - start);

}

static gint
dissect_query_reply_implicit_partitions(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                                        gint sf_length)
{
  int start=offset;
  int i;

  hf_items fields[] = {
    { hf_tn3270_ip_flags, 0, 1, 0 },
    { hf_tn3270_ip_flags, 0, 1, 0 },
    { 0, 0, 0, 0 },
  };

  offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                fields);

  for (i = 0; i < 3; i++) {
    offset += dissect_query_reply_implicit_partitions_sd_parms(tn3270_tree, tvb, offset);
    if (!tvb_length_remaining(tvb, offset))
      break;
  }

  offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, sf_length);

  return (offset - start);
}

/* 6.32 - Query Reply (IOCA Auxiliary Device) */
static gint
dissect_query_reply_ioca_aux_device(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                                                gint sf_length)
{
  int start=offset;

  hf_items fields[] = {
    { hf_tn3270_resbyte, 0, 1, 0 },
    { hf_tn3270_resbyte, 0, 1, 0 },
    { hf_tn3270_ioca_limin, 0, 2, 0 },
    { hf_tn3270_ioca_limout, 0, 2, 0 },
    { hf_tn3270_ioca_type, 0, 1, 0 },
    { 0, 0, 0, 0 },
  };

  offset += tn3270_add_hf_items(tn3270_tree, tvb, offset, fields);

  offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, sf_length);

  return (offset - start);
}

/* 6.33 - Query Reply (Line Type) - Search for LINE_TYPE*/

/* 6.34 - Query Reply (MSR Control) */
static gint
dissect_query_reply_msr_control(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                                                gint sf_length)
{
  int start=offset;

  hf_items fields[] = {
    { hf_tn3270_resbyte, 0, 1, 0 },
    { hf_tn3270_msr_nd, 0, 1, 0 },
    { hf_tn3270_msr_type, 0, 1, 0 },
    { 0, 0, 0, 0 },
  };

  offset += tn3270_add_hf_items(tn3270_tree, tvb, offset, fields);
  offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, sf_length);

  return (offset - start);
}

/* 6.35 - Query Reply (Null) - Search for QUERY_REPLY_NULL */

/* 6.36 - Query Reply (OEM Auxiliary Device) */
static gint
dissect_query_reply_oem_auxiliary_device(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                                        gint sf_length)
{
  int start=offset;
  int i;

  hf_items fields[] = {
    { hf_tn3270_resbyte, 0, 1, 0 },
    { hf_tn3270_oem_dsref, 0, 1, 0 },
    { hf_tn3270_oem_dtype, 0, 8, 0 },
    { hf_tn3270_oem_uname, 0, 8, 0 },
    { 0, 0, 0, 0 },
  };

  offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                fields);

  for (i = 0; i < 3; i++) {
    offset += dissect_query_reply_oem_auxiliary_device_sd_parms(tn3270_tree, tvb, offset);
    if (!tvb_length_remaining(tvb, offset))
      break;
  }

  offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, sf_length);

  return (offset - start);
}

/* 6.37 - Query Reply (Paper Feed Techniques) */
static gint
dissect_query_reply_paper_feed_techniques(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                                          gint sf_length)
{
  int start=offset;

  hf_items fields[] = {
    { hf_tn3270_pft_flags, 0, 1, 0 },
    { hf_tn3270_pft_tmo, 0, 2, 0 },
    { hf_tn3270_pft_bmo, 0, 2, 0 },
    { 0, 0, 0, 0 },
  };

  offset += tn3270_add_hf_items(tn3270_tree, tvb, offset, fields);
  offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, sf_length);

  return (offset - start);
}

/* 6.38 - Query Reply (Partition Characteristics) */
static gint
dissect_query_reply_partition_characteristics(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                                              gint sf_length)
{
  int start=offset, i, sdp;
  int done = 0;

  hf_items fields[] = {
    { hf_tn3270_sdp_ln, 0, 1, 0 },
    { hf_tn3270_sdp_id, 0, 1, 0 },
    { 0, 0, 0, 0 },
  };

  for (i = 0; i < 2; i++) {
    sdp = tvb_get_guint8(tvb, offset+1);
    switch (sdp) {
      case 0x01: /*Viewport Outline*/
        offset += tn3270_add_hf_items(tn3270_tree, tvb, offset, fields);
        proto_tree_add_item(tn3270_tree, hf_tn3270_pc_vo_thickness,
                                tvb, offset, 1, FALSE);
        offset++;
        break;
      case 0x03: /*Enable User Call-Up*/
        offset += tn3270_add_hf_items(tn3270_tree, tvb, offset, fields);
        break;
      default:
        done = 1;
        break;
    }
    if (!tvb_length_remaining(tvb, offset) || done)
      break;
  }

  offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, sf_length);

  return (offset - start);
}

/* 6.39 - Query Reply (Port) - Search for QUERY_REPLY_PORT */
/* 6.40 - Query Reply (Procedure) - Search for QUERY_REPLY_PROCEDURE */

/* 6.41 - Query Reply ((Product Defined Data Stream) */
static gint
dissect_query_reply_product_defined_data_stream(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                                                gint sf_length)
{
  int start=offset, sdp;

  hf_items fields[] = {
    { hf_tn3270_resbytes, 0, 2, 0 },
    { hf_tn3270_pdds_refid, 0, 1, 0 },
    { hf_tn3270_pdds_ssid, 0, 1, 0 },
    { 0, 0, 0, 0 },
  };

  offset += tn3270_add_hf_items(tn3270_tree, tvb, offset, fields);

  sdp = tvb_get_guint8(tvb, offset+1);
  if (sdp == 0x01) { /*DAID*/
     offset += dissect_daid_sd_parm(tn3270_tree, tvb, offset);
  }

  offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, sf_length);

  return (offset - start);
}

/* 6.42 - Query Reply (Modes) */
static gint
dissect_query_reply_modes(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                          gint sf_length)
{
  int start=offset;
  int i;

  for (i=0; i < (sf_length - 4); i++) {
    proto_tree_add_item(tn3270_tree,
                            hf_tn3270_mode,
                            tvb, offset,
                            1,
                            FALSE);
    offset++;
  }
  offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, sf_length);

  return (offset - start);
}

/* 6.43 - Query Reply (RPQ Names) */
static gint
dissect_query_reply_rpq_names(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                                                gint sf_length)
{
  int start=offset;
  int rpql;

  hf_items fields[] = {
    { hf_tn3270_rpq_device,0, 4, 0 },
    { hf_tn3270_rpq_mid,0, 4, 0 },
    { 0, 0, 0, 0 },
  };

  offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                fields);

  rpql = tvb_get_guint8(tvb, offset);

  proto_tree_add_item(tn3270_tree,
                          hf_tn3270_rpq_rpql,
                          tvb, offset,
                          1,
                          FALSE);
  offset++;

  proto_tree_add_item(tn3270_tree,
                          hf_tn3270_rpq_name,
                          tvb, offset,
                          (rpql - 1),
                          FALSE);
  offset+=(rpql-1);

  offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, sf_length);

  return (offset - start);
}

/* 6.44 - Query Reply (Save/Restore Format) */
static gint
dissect_query_reply_save_or_restore_format(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                                                gint sf_length)
{
  int start=offset;

  hf_items fields[] = {
    { hf_tn3270_srf_fpcbl, 0, 1, 0 },
    { 0, 0, 0, 0 },
  };

  offset += tn3270_add_hf_items(tn3270_tree, tvb, offset, fields);

  offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, sf_length);

  return (offset - start);
}

/* 6.45 - Query Reply (Segment) - Search for QUERY_REPLY_SEGMENT */

/* 6.46 - Query Reply ((Settable Printer Characteristics) */
static gint
dissect_query_reply_settable_printer_characteristics(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                                                     gint sf_length)
{
  int start=offset, sdp;

  hf_items fields[] = {
    { hf_tn3270_resbytes, 0, 2, 0 },
    { 0, 0, 0, 0 },
  };

  hf_items fields2[] = {
    { hf_tn3270_sdp_ln, 0, 1, 0 },
    { hf_tn3270_sdp_id, 0, 1, 0 },
    { 0, 0, 0, 0 },
  };

  offset += tn3270_add_hf_items(tn3270_tree, tvb, offset, fields);

  sdp = tvb_get_guint8(tvb, offset+1);
  if (sdp == 0x01) { /*Early Print Complete*/
    offset += tn3270_add_hf_items(tn3270_tree, tvb, offset, fields2);
    proto_tree_add_item(tn3270_tree, hf_tn3270_spc_epc_flags, tvb, offset,
                        1, FALSE);
    offset++;
  }

  offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, sf_length);

  return (offset - start);
}

/* 6.47 - Query Reply (Storage Pools) */
static gint
dissect_query_reply_storage_pools(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                                  gint sf_length)
{
  int start=offset, sdp, i;

  hf_items fields2[] = {
    { hf_tn3270_sdp_ln, 0, 1, 0 },
    { hf_tn3270_sdp_id, 0, 1, 0 },
    { hf_tn3270_sp_spid, 0, 1, 0 },
    { hf_tn3270_sp_size, 0, 4, 0 },
    { hf_tn3270_sp_space, 0, 4, 0 },
    { 0, 0, 0, 0 },
  };

  sdp = tvb_get_guint8(tvb, offset+1);
  if (sdp == 0x01) { /*Storage Pool Characteristics*/
    offset += tn3270_add_hf_items(tn3270_tree, tvb, offset, fields2);
    for (i=0; i < (sf_length - 4); i+=2) {
      proto_tree_add_item(tn3270_tree, hf_tn3270_sp_objlist,
                              tvb, offset, 2, FALSE);
      offset+=2;
    }
  }

  offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, sf_length);

  return (offset - start);
}

/* 6.48 - Query Reply (Summary) */
static gint
dissect_query_reply_summary(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                                           gint sf_length)
{
  int start=offset;
  int i;

  for (i=0; i < (sf_length - 4); i++) {
    if (!tvb_offset_exists(tvb, offset)) {
      return (offset - start);
    }
    proto_tree_add_item(tn3270_tree,
                            hf_tn3270_sf_query_replies,
                            tvb, offset,
                            1,
                            FALSE);
    offset++;
  }
  offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, sf_length);
  return (offset - start);
}

/* 6.49 - Query Reply (Text Partitions) */
static gint
dissect_query_reply_text_partitions(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                                  gint sf_length)
{
  int start=offset, len, i;

  hf_items fields[] = {
    { hf_tn3270_tp_nt, 0, 1, 0 },
    { hf_tn3270_tp_m, 0, 2, 0 },
    { hf_tn3270_tp_flags, 0, 1, 0 },
    { 0, 0, 0, 0 },
  };

  offset += tn3270_add_hf_items(tn3270_tree, tvb, offset, fields);

  len = tvb_get_guint8(tvb, offset);
  proto_tree_add_item(tn3270_tree, hf_tn3270_tp_ntt, tvb, offset, 1, FALSE);
  offset++;

  for (i=0; i < len; i++) {
    proto_tree_add_item(tn3270_tree, hf_tn3270_tp_tlist,
                            tvb, offset, 1, FALSE);
    offset++;
  }

  offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, sf_length);

  return (offset - start);
}

/* 6.50 - Query Reply (Transparency) */
static gint
dissect_query_reply_transparency(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                                  gint sf_length)
{
  int start=offset, i, len;

  len = tvb_get_guint8(tvb, offset);
  proto_tree_add_item(tn3270_tree, hf_tn3270_t_np, tvb, offset, 1, FALSE);
  offset++;

  for (i=0; i < len; i+=2) {
    proto_tree_add_item(tn3270_tree, hf_tn3270_t_vi,
                            tvb, offset, 1, FALSE);
    offset++;
    proto_tree_add_item(tn3270_tree, hf_tn3270_t_ai,
                            tvb, offset, 1, FALSE);
    offset++;
  }

  offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, sf_length);

  return (offset - start);
}

/* 6.51 - Query Reply Usable Area */
static gint
dissect_query_reply_usable_area(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                                           gint sf_length)
{
  int start=offset;
  int vcp;

  static const int *byte1[] = {
    &hf_tn3270_ua_reserved1,
    &hf_tn3270_ua_page_printer,
    &hf_tn3270_ua_reserved2,
    &hf_tn3270_ua_hard_copy,
    &hf_tn3270_ua_addressing,
    NULL
  };

  static const int *byte2[] = {
    &hf_tn3270_ua_variable_cells,
    &hf_tn3270_ua_characters,
    &hf_tn3270_ua_cell_units,
    NULL
  };

  hf_items fields[] = {
    { hf_tn3270_usable_area_flags1, ett_tn3270_usable_area_flags1, 1, byte1 },
    { hf_tn3270_usable_area_flags2, ett_tn3270_usable_area_flags1, 1, byte2 },
    { hf_tn3270_ua_width_cells_pels, 0, 2, 0 },
    { hf_tn3270_ua_height_cells_pels, 0, 2, 0 },
    { hf_tn3270_ua_uom_cells_pels, 0, 1, 0 },
    { hf_tn3270_ua_xr, 0, 4, 0 },
    { hf_tn3270_ua_yr, 0, 4, 0 },
    { hf_tn3270_ua_aw, 0, 1, 0 },
    { hf_tn3270_ua_ah, 0, 1, 0 },
    { hf_tn3270_ua_buffsz, 0, 2, 0 },
    { 0, 0, 0, 0 },
  };

  hf_items fields2[] = {
    { hf_tn3270_ua_xmin, 0, 1, 0 },
    { hf_tn3270_ua_ymin, 0, 1, 0 },
    { hf_tn3270_ua_xmax, 0, 1, 0 },
    { hf_tn3270_ua_ymax, 0, 1, 0 },
    { 0, 0, 0, 0 },
  };

  vcp = tvb_get_guint8(tvb, offset+1);

  offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                fields);

  if (vcp == VARIABLE_CELLS) {
    offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                  fields2);
  }

  /*TODO: self defining parms */
  offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, sf_length);

  return (offset - start);
}

/* 6.52 - Query Reply 3270 IPDS */
static gint
dissect_query_reply_3270_ipds(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                                                gint sf_length)
{
  int start=offset;

  hf_items fields[] = {
    { hf_tn3270_resbytes, 0, 2, 0 },
    { hf_tn3270_3270_tranlim, 0, 2, 0 },
    { 0, 0, 0, 0 },
  };

  offset += tn3270_add_hf_items(tn3270_tree, tvb, offset, fields);
  offset += dissect_unknown_data(tn3270_tree, tvb, offset, start, sf_length);

  return (offset - start);
}

static gint
process_in_out_structured_fields(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset,
                                 gint sf_length, gint16 sfid)
{
  int start = offset;

  switch (sfid) {
    case DATA_CHAIN:
      proto_tree_add_bits_item(tn3270_tree,
                              hf_tn3270_data_chain_bitmask,
                              tvb, offset,
                              1,
                              FALSE);
      offset++;
      proto_tree_add_item(tn3270_tree,
                              hf_tn3270_resbyte,
                              tvb, offset,
                              1,
                              FALSE);
      offset++;
      break;
    case DESTINATION_OR_ORIGIN:
      proto_tree_add_bits_item(tn3270_tree,
                              hf_tn3270_destination_or_origin_bitmask,
                              tvb, offset,
                              1,
                              FALSE);
      offset++;
      proto_tree_add_item(tn3270_tree,
                              hf_tn3270_resbyte,
                              tvb, offset,
                              1,
                              FALSE);
      offset++;
      proto_tree_add_item(tn3270_tree,
                              hf_tn3270_sf_inbound_outbound_id,
                              tvb, offset,
                              2,
                              FALSE);
      offset+=2;
      break;
    case OBJECT_DATA:
    case OBJECT_CONTROL:
    case OBJECT_PICTURE:
    case OEM_DATA: /* FIXME: Not really but same layout */
      offset += dissect_object_control(tn3270_tree, tvb, offset, sf_length);
      break;
    case SAVE_OR_RESTORE_FORMAT:
      offset += dissect_save_or_restore_format(tn3270_tree, tvb, offset, sf_length);
      break;
    case SELECT_IPDS_MODE:
      proto_tree_add_item(tn3270_tree,
                              hf_tn3270_resbytes,
                              tvb, offset,
                              2,
                              FALSE);
      offset+=2;
      break;
    default:
      break;
  }

  return (offset - start);
}

static gint
process_double_byte_sf(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset, gint sf_length)
{
  gint16 full_sf_id;
  int start = offset;

  full_sf_id = tvb_get_guint8(tvb, offset-1);
  proto_tree_add_item(tn3270_tree,
                          hf_tn3270_double_byte_sf_id,
                          tvb, offset-1,
                          2,
                          FALSE);
  offset++;
  switch (full_sf_id) {
    case BEGIN_OR_END_OF_FILE:
      proto_tree_add_item(tn3270_tree,
                              hf_tn3270_partition_id,
                              tvb, offset,
                              1,
                              FALSE);
      offset++;
      /*TODO: use bits_text */
      proto_tree_add_bits_item(tn3270_tree,
                              hf_tn3270_begin_end_flags1,
                              tvb, offset,
                              1,
                              FALSE);
      offset++;
      proto_tree_add_bits_item(tn3270_tree,
                              hf_tn3270_begin_end_flags2,
                              tvb, offset,
                              1,
                              FALSE);
      offset++;
      break;
    case LOAD_COLOR_TABLE:
      /* Refer to related graphics docs !*/
      proto_tree_add_item(tn3270_tree,
                              hf_tn3270_load_color_command,
                              tvb, offset,
                              (sf_length - 4),
                              FALSE);
      offset+=(sf_length - 4);
      break;
    case LOAD_FORMAT_STORAGE:
      offset += dissect_load_format_storage(tn3270_tree, tvb, offset, sf_length);
      break;
    case LOAD_LINE_TYPE:
      /* Refer to related graphics docs !*/
      proto_tree_add_item(tn3270_tree,
                              hf_tn3270_load_line_type_command,
                              tvb, offset,
                              (sf_length - 4),
                              FALSE);
      offset+=(sf_length - 4);
      break;
    case MODIFY_PARTITION:
      offset += dissect_modify_partition(tn3270_tree, tvb, offset, sf_length);
      break;
    case OUTBOUND_TEXT_HEADER:
      offset += dissect_outbound_text_header(tn3270_tree, tvb, offset, sf_length);
      break;
    case REQUEST_RECOVERY_DATA:
      proto_tree_add_item(tn3270_tree,
                              hf_tn3270_resbyte,
                              tvb, offset,
                              1,
                              FALSE);
      offset++;
      break;
    case RESTART:
      offset += dissect_restart(tn3270_tree, tvb, offset, sf_length);
      break;
    case SELECT_COLOR_TABLE:
      proto_tree_add_item(tn3270_tree,
                              hf_tn3270_color_command,
                              tvb, offset,
                              2,
                              FALSE);
      offset+=2;
      break;
    case SET_CHECKPOINT_INTERVAL:
      proto_tree_add_item(tn3270_tree,
                              hf_tn3270_resbyte,
                              tvb, offset,
                              1,
                              FALSE);
      offset++;
      proto_tree_add_item(tn3270_tree,
                              hf_tn3270_interval,
                              tvb, offset,
                              2,
                              FALSE);
      offset+=2;
      break;
    case SET_MSR_CONTROL:
      offset += dissect_set_msr_control(tn3270_tree, tvb, offset, sf_length);
      break;
    case SET_PARTITION_CHARACTERISTICS:
      offset += dissect_set_partition_characteristics(tn3270_tree, tvb, offset, sf_length);
      break;
    case SET_PRINTER_CHARACTERISTICS:
      offset += dissect_set_printer_characteristics(tn3270_tree, tvb, offset, sf_length);
      break;
    case TYPE_1_TEXT_OUTBOUND:
      offset += dissect_type_1_text(tn3270_tree, tvb, offset, sf_length);
      break;
    default:
      offset += process_in_out_structured_fields(tn3270_tree, tvb, offset, sf_length,
                                                 full_sf_id);
      break;
  }

  return (offset - start);
}

static gint
dissect_outbound_structured_fields(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset)
{
  int start = offset;
  int sf_id;
  gint16 sf_length;
  proto_tree   *sf_tree;
  proto_item   *ti;

  while (tvb_offset_exists(tvb, offset)) {
    sf_length = tvb_get_ntohs(tvb, offset);
    proto_tree_add_item(tn3270_tree,
                            hf_tn3270_sf_length,
                            tvb, offset,
                            2,
                            FALSE);
    offset+=2;
    sf_id = tvb_get_guint8(tvb, offset);
    ti = proto_tree_add_item(tn3270_tree, hf_tn3270_sf_outbound_id, tvb, offset, 1, FALSE);
    offset++;
    sf_tree = proto_item_add_subtree(ti, ett_sf);
    switch (sf_id) {
          case 0x0F:
          case 0x10:
            process_double_byte_sf(sf_tree, tvb, offset, sf_length);
            break;
          case READ_PARTITION:
            offset += dissect_read_partition(sf_tree, tvb, offset, sf_length);
            break;
          case ACTIVATE_PARTITION:
          case DESTROY_PARTITION:
          case RESET_PARTITION:
            proto_tree_add_item(sf_tree,
                                    hf_tn3270_partition_id,
                                    tvb, offset,
                                    1,
                                    FALSE);
            offset++;
            break;
          case CREATE_PARTITION:
            offset += dissect_create_partition(sf_tree, tvb, offset, sf_length);
            break;
          case ERASE_OR_RESET:
            proto_tree_add_bits_item(sf_tree,
                                    hf_tn3270_erase_flags,
                                    tvb, offset,
                                    1,
                                    FALSE);
            offset++;
            break;
          case LOAD_PROGRAMMED_SYMBOLS:
            offset += dissect_load_programmed_symbols(sf_tree, tvb, offset, sf_length);
            break;
          case OUTBOUND_3270DS:
            offset += dissect_outbound_3270ds(sf_tree, tvb, offset, sf_length);
            break;
          case PRESENT_ABSOLUTE_FORMAT:
            offset += dissect_present_absolute_format(sf_tree, tvb, offset, sf_length);
            break;
          case PRESENT_RELATIVE_FORMAT:
            offset += dissect_present_relative_format(sf_tree, tvb, offset, sf_length);
            break;
          case SCS_DATA:
            proto_tree_add_item(sf_tree,
                                    hf_tn3270_partition_id,
                                    tvb, offset,
                                    1,
                                    FALSE);
            offset++;
            proto_tree_add_item(sf_tree,
                                    hf_tn3270_scs_data,
                                    tvb, offset,
                                    (sf_length - 4),
                                    FALSE);
            offset+=(sf_length - 4);
            break;
          case SET_REPLY_MODE:
            offset += dissect_set_reply_mode(sf_tree, tvb, offset, sf_length);
            break;
          case SELECT_FORMAT_GROUP:
            proto_tree_add_item(sf_tree,
                                    hf_tn3270_partition_id,
                                    tvb, offset,
                                    1,
                                    FALSE);
            offset++;
            proto_tree_add_item(sf_tree,
                                    hf_tn3270_format_group,
                                    tvb, offset,
                                    (sf_length - 4),
                                    FALSE);
            offset+=(sf_length - 4);
            break;
          case SET_WINDOW_ORIGIN:
            proto_tree_add_item(sf_tree,
                                    hf_tn3270_partition_id,
                                    tvb, offset,
                                    1,
                                    FALSE);
            offset++;
            proto_tree_add_item(sf_tree,
                                    hf_tn3270_partition_rw,
                                    tvb, offset,
                                    2,
                                    FALSE);
            offset+=2;
            proto_tree_add_item(sf_tree,
                                    hf_tn3270_partition_cw,
                                    tvb, offset,
                                    2,
                                    FALSE);
            offset+=2;
            break;
          default:
            break;
    }
  }
  return (offset - start);
}

static gint
dissect_inbound_structured_fields(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset)
{
  int start = offset;
  gint16 sf_length;
  guint16 sf_id;
  proto_tree   *sf_tree;
  proto_item   *ti;

  while (tvb_offset_exists(tvb, offset)) {
    /*Handle NULL bytes until we find a length value */
    sf_length = tvb_get_ntohs(tvb, offset);
    while (!sf_length){
      proto_tree_add_item(tn3270_tree,
                              hf_tn3270_null,
                              tvb, offset,
                              1,
                              FALSE);
      offset++;
      sf_length = tvb_get_ntohs(tvb, offset);
    }

    proto_tree_add_item(tn3270_tree,
                            hf_tn3270_sf_length,
                            tvb, offset,
                            2,
                            FALSE);
    offset+=2;

    sf_id = tvb_get_ntohs(tvb, offset);
    ti = proto_tree_add_item(tn3270_tree, hf_tn3270_sf_inbound_id, tvb, offset, 2, FALSE);
    offset+=2;
    sf_tree = proto_item_add_subtree(ti, ett_sf);
    switch (sf_id) {
      case EXCEPTION_OR_STATUS:
        offset += dissect_exception_or_status(sf_tree, tvb, offset, sf_length);
        break;
      case INBOUND_TEXT_HEADER:
        offset += dissect_inbound_text_header(sf_tree, tvb, offset, sf_length);
        break;
      case INBOUND_3270DS:
        offset += dissect_inbound_3270ds(sf_tree, tvb, offset, sf_length);
        break;
      case RECOVERY_DATA:
        offset += dissect_recovery_data(sf_tree, tvb, offset, sf_length);
        break;
      case TYPE_1_TEXT_INBOUND:
        offset += dissect_type_1_text(sf_tree, tvb, offset, sf_length);
        break;
      case QUERY_REPLY_ALPHANUMERIC_PARTITIONS:
        offset += dissect_query_reply_alphanumeric(sf_tree, tvb, offset, sf_length);
        break;
      case QUERY_REPLY_AUXILIARY_DEVICE:
      case QUERY_REPLY_BEGIN_OR_END_OF_FILE:
        offset += dissect_query_reply_resbytes(sf_tree, tvb, offset, sf_length);
        break;
      case QUERY_REPLY_CHARACTER_SETS:
        offset += dissect_query_reply_character_sets(sf_tree, tvb, offset, sf_length);
        break;
      case QUERY_REPLY_COLOR:
        offset += dissect_query_reply_color(sf_tree, tvb, offset, sf_length);
        break;
      case QUERY_REPLY_COOPERATIVE_PROCESSING_REQUESTOR:
        offset += dissect_query_reply_cooperative(sf_tree, tvb, offset, sf_length);
        break;
      case QUERY_REPLY_DATA_CHAINING:
        offset += dissect_query_reply_data_chaining(sf_tree, tvb, offset, sf_length);
        break;
      case QUERY_REPLY_DATA_STREAMS:
        offset += dissect_query_reply_data_streams(sf_tree, tvb, offset, sf_length);
        break;
      case QUERY_REPLY_DBCS_ASIA:
        offset += dissect_query_reply_dbcs_asia(sf_tree, tvb, offset, sf_length);
        break;
      case QUERY_REPLY_DEVICE_CHARACTERISTICS:
        /*TODO: implement this beast */
        offset += dissect_query_reply_device_characteristics(sf_tree, tvb, offset, sf_length);
        break;
      case QUERY_REPLY_SUMMARY:
        offset += dissect_query_reply_summary(sf_tree, tvb, offset, sf_length);
        break;
      case QUERY_REPLY_USABLE_AREA:
        offset += dissect_query_reply_usable_area(sf_tree, tvb, offset, sf_length);
        break;
      case QUERY_REPLY_HIGHLIGHTING:
        offset += dissect_query_reply_highlighting(sf_tree, tvb, offset, sf_length);
        break;
      case QUERY_REPLY_REPLY_MODES:
        offset += dissect_query_reply_modes(sf_tree, tvb, offset, sf_length);
        break;
      case QUERY_REPLY_DISTRIBUTED_DATA_MANAGEMENT:
        offset += dissect_query_reply_distributed_data_management(sf_tree, tvb, offset, sf_length);
        break;
      case QUERY_REPLY_RPQ_NAMES:
        offset += dissect_query_reply_rpq_names(sf_tree, tvb, offset, sf_length);
        break;
      case QUERY_REPLY_IMPLICIT_PARTITION:
        offset += dissect_query_reply_implicit_partitions(sf_tree, tvb, offset, sf_length);
        break;
      case QUERY_REPLY_OEM_AUXILIARY_DEVICE:
        offset += dissect_query_reply_oem_auxiliary_device(sf_tree, tvb, offset, sf_length);
        break;
      case QUERY_REPLY_DOCUMENT_INTERCHANGE_ARCHITECTURE:
        offset += dissect_query_reply_document_interchange_architecture(sf_tree, tvb, offset, sf_length);
        break;
      case QUERY_REPLY_EXTENDED_DRAWING_ROUTINE:
        offset += dissect_query_reply_extended_drawing_routine(sf_tree, tvb, offset, sf_length);
        break;
      case QUERY_REPLY_FIELD_OUTLINING:
        offset += dissect_query_reply_field_outlining(sf_tree, tvb, offset, sf_length);
        break;
      case QUERY_REPLY_FIELD_VALIDATION:
        offset += dissect_3270_field_validation(sf_tree, tvb, offset);
        break;
      case QUERY_REPLY_FORMAT_STORAGE_AUXILIARY_DEVICE:
        offset += dissect_query_reply_format_storage_aux_device(sf_tree, tvb, offset, sf_length);
        break;
      case QUERY_REPLY_GRAPHIC_COLOR:
      case QUERY_REPLY_GRAPHIC_SYMBOL_SETS:
      case QUERY_REPLY_IMAGE:
      case QUERY_REPLY_LINE_TYPE:
      case QUERY_REPLY_PROCEDURE:
      case QUERY_REPLY_SEGMENT:
        /* Not an error - just has a data field like 'extended drawing'*/
        offset += dissect_query_reply_extended_drawing_routine(sf_tree, tvb, offset, sf_length);
        break;
      case QUERY_REPLY_IBM_AUXILIARY_DEVICE:
        offset += dissect_query_reply_ibm_aux_device(sf_tree, tvb, offset, sf_length);
        break;
      case QUERY_REPLY_IOCA_AUXILIARY_DEVICE:
        offset += dissect_query_reply_ioca_aux_device(sf_tree, tvb, offset, sf_length);
        break;
      case QUERY_REPLY_MSR_CONTROL:
        offset += dissect_query_reply_msr_control(sf_tree, tvb, offset, sf_length);
        break;
      case QUERY_REPLY_FORMAT_PRESENTATION:
      case QUERY_REPLY_NULL:
      case QUERY_REPLY_PORT:
        /* This field is always empty */
        break;
      case QUERY_REPLY_PAPER_FEED_TECHNIQUES:
        offset += dissect_query_reply_paper_feed_techniques(sf_tree, tvb, offset, sf_length);
        break;
      case QUERY_REPLY_PARTITION_CHARACTERISTICS:
        offset += dissect_query_reply_partition_characteristics(sf_tree, tvb, offset, sf_length);
        break;
      case QUERY_REPLY_PRODUCT_DEFINED_DATA_STREAM:
        offset += dissect_query_reply_product_defined_data_stream(sf_tree, tvb, offset, sf_length);
        break;
      case QUERY_REPLY_SAVE_OR_RESTORE_FORMAT:
        offset += dissect_query_reply_save_or_restore_format(sf_tree, tvb, offset, sf_length);
        break;
      case QUERY_REPLY_SETTABLE_PRINTER_CHARACTERISTICS:
        offset += dissect_query_reply_settable_printer_characteristics(sf_tree, tvb, offset, sf_length);
        break;
      case QUERY_REPLY_STORAGE_POOLS:
        offset += dissect_query_reply_storage_pools(sf_tree, tvb, offset, sf_length);
        break;
      case QUERY_REPLY_TEXT_PARTITIONS:
        offset += dissect_query_reply_text_partitions(sf_tree, tvb, offset, sf_length);
        break;
      case QUERY_REPLY_TRANSPARENCY:
        offset += dissect_query_reply_transparency(sf_tree, tvb, offset, sf_length);
        break;
      case QUERY_REPLY_3270_IPDS:
        offset += dissect_query_reply_3270_ipds(sf_tree, tvb, offset, sf_length);
        break;
      default:
        break;
    }
  }

  return (offset - start);
}

/* Start: Handle WCC, Orders and Data */

static gint
dissect_stop_address(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset)
{
  int start = offset;
  int is_ge;

  proto_tree_add_item(tn3270_tree,
                          hf_tn3270_stop_address,
                          tvb, offset,
                          2,
                          FALSE);
  offset++;
  is_ge = tvb_get_guint8(tvb, offset);
  if (is_ge != GE) {
    proto_tree_add_item(tn3270_tree,
                            hf_tn3270_character_code,
                            tvb, offset,
                            1,
                            FALSE);
    offset++;
  }

  return (offset - start);
}

static gint
dissect_sba(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset)
{
  int start = offset;
  proto_tree_add_item(tn3270_tree,
                          hf_tn3270_buffer_x,
                          tvb, offset,
                          1,
                          FALSE);
  offset++;
  proto_tree_add_item(tn3270_tree,
                          hf_tn3270_buffer_y,
                          tvb, offset,
                          1,
                          FALSE);
  offset++;

  return (offset - start);
}

static gint
dissect_field_attribute_pair(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset)
{
  int start = offset;
  int attribute_type;

  attribute_type = tvb_get_guint8(tvb, offset);
  proto_tree_add_item(tn3270_tree,
                          hf_tn3270_attribute_type,
                          tvb, offset,
                          1,
                          FALSE);
  offset++;
  switch (attribute_type) {
    case ALL_CHARACTER_ATTRIBUTES:
      proto_tree_add_item(tn3270_tree,
                          hf_tn3270_all_character_attributes,
                          tvb, offset,
                          1,
                          FALSE);
      offset++;
      break;
    case T3270_FIELD_ATTRIBUTE:
      dissect_3270_field_attribute(tn3270_tree, tvb, offset);
      break;
    case EXTENDED_HIGHLIGHTING:
      proto_tree_add_item(tn3270_tree,
                          hf_tn3270_extended_highlighting,
                          tvb, offset,
                          1,
                          FALSE);
      offset++;
      break;
    case FOREGROUND_COLOR:
    case BACKGROUND_COLOR:
      proto_tree_add_item(tn3270_tree,
                          hf_tn3270_color,
                          tvb, offset,
                          1,
                          FALSE);
      offset++;
      break;
    case CHARACTER_SET:
      proto_tree_add_item(tn3270_tree,
                          hf_tn3270_character_set,
                          tvb, offset,
                          1,
                          FALSE);
      offset++;
      break;
    case FIELD_OUTLINING:
      proto_tree_add_item(tn3270_tree,
                          hf_tn3270_field_outlining,
                          tvb, offset,
                          1,
                          FALSE);
      offset++;
      break;
    case TRANSPARENCY:
      proto_tree_add_item(tn3270_tree,
                          hf_tn3270_transparency,
                          tvb, offset,
                          1,
                          FALSE);
      offset++;
      break;
    case FIELD_VALIDATION:
      dissect_3270_field_validation(tn3270_tree, tvb, offset);
      break;
  }

  return (offset - start);
}

static gint
dissect_field_attribute_pairs(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset)
{
  int start = offset;
  int no_of_pairs;
  int i;

  no_of_pairs = tvb_get_guint8(tvb, offset);
  proto_tree_add_item(tn3270_tree,
                          hf_tn3270_number_of_attributes,
                          tvb, offset,
                          1,
                          FALSE);
  offset++;

  for (i=0; i < no_of_pairs; i++) {
    offset += dissect_field_attribute_pair(tn3270_tree, tvb, offset);
  }
  return (offset - start);
}

gint
dissect_orders_and_data(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset)
{
  gint start = offset;
  gint order_code;

  /* Order Code */

  while (tvb_offset_exists(tvb, offset)) {
    order_code = tvb_get_guint8(tvb, offset);
    proto_tree_add_item(tn3270_tree,
                            hf_tn3270_order_code,
                            tvb, offset,
                            1,
                            FALSE);
    offset++;
    switch (order_code) {
          case SF:
            offset += dissect_3270_field_attribute(tn3270_tree, tvb, offset);
            break;
          case MF:
          case SFE:
            offset += dissect_field_attribute_pairs(tn3270_tree, tvb, offset);
            break;
          case SA:
            offset += dissect_field_attribute_pair(tn3270_tree, tvb, offset);
            break;
          case EUA:
          case RA:
            offset += dissect_stop_address(tn3270_tree, tvb, offset);
            break;
          case GE:
            proto_tree_add_item(tn3270_tree,
                                    hf_tn3270_character_code,
                                    tvb, offset,
                                    1,
                                    FALSE);
            offset++;
            break;
          case SBA:
            offset += dissect_sba(tn3270_tree, tvb, offset);
            break;
/*          case PT:*/
          case IC:
          case EW:
          case EWA:
            break;
/*            return (offset - start);*/
          default:
            proto_tree_add_text(tn3270_tree, tvb, offset, 1, "Bogus value: %u", order_code);
            offset ++;
            break;
    }
    offset += add_data_until_next_order_code(tn3270_tree, tvb, offset);
  }
  offset += add_data_until_next_order_code(tn3270_tree, tvb, offset);
  return (offset - start);
}

/* End: Handle WCC, Orders and Data */


static gint
dissect_tn3270e_header(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset)
{

  int start=offset;
  gint data_type;
  int len = 0;

  hf_items fields[] = {
      { hf_tn3270_tn3270e_data_type, 0, 1, 0 },
      { hf_tn3270_tn3270e_request_flag, 0, 1, 0 },
      { hf_tn3270_tn3270e_response_flag_3270_SCS, 0, 1, 0 },
      { hf_tn3270_tn3270e_seq_number, 0, 2, 0 },
      { 0, 0, 0, 0 },
  };

  data_type = tvb_get_guint8(tvb, offset);

  offset += tn3270_add_hf_items(tn3270_tree, tvb, offset,
                                fields);

  switch (data_type) {
    case TN3270E_BIND_IMAGE:
    case TN3270E_NVT_DATA:
    case TN3270E_REQUEST:
    case TN3270E_RESPONSE:
    case TN3270E_SCS_DATA:
    case TN3270E_SSCP_LU_DATA:
    case TN3270E_UNBIND:
      len = tvb_length_remaining(tvb, offset);
      proto_tree_add_item(tn3270_tree, hf_tn3270_tn3270e_header_data, tvb, offset, len, FALSE);
      offset += len;
      break;
    default:
      break;
  }

  return (offset - start);
}

/* Detect and Handle Direction of Stream */
static gint
dissect_outbound_stream(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset)
{
  gint command_code;
  gint start = offset;
  /* Command Code*/
  command_code = tvb_get_guint8(tvb, offset);

  switch (command_code) {
    case W:
    case EW:
    case EWA:
    case SNA_W:
    case SNA_EW:
    case SNA_EWA:
      proto_tree_add_item(tn3270_tree,
                              hf_tn3270_command_code,
                              tvb, offset,
                              1,
                              FALSE);
      offset++;
      /* WCC */
      offset += dissect_wcc(tn3270_tree, tvb, offset);
      offset += dissect_orders_and_data(tn3270_tree, tvb, offset);
      break;
    case WSF:
    case SNA_WSF:
      proto_tree_add_item(tn3270_tree,
                              hf_tn3270_command_code,
                              tvb, offset,
                              1,
                              FALSE);
      offset++;
      offset += dissect_outbound_structured_fields(tn3270_tree, tvb, offset);
      break;
    default:
      proto_tree_add_text(tn3270_tree, tvb, offset, 1, "Bogus value: %u", command_code);
      offset ++;
      break;
  }
  return (offset - start);

}

/* INBOUND DATA STREAM (DISPLAY -> MAINFRAME PROGRAM) */
static gint
dissect_inbound_stream(proto_tree *tn3270_tree, tvbuff_t *tvb, gint offset)
{
  gint start = offset;
  gint aid;

  /* Command Code*/
  aid = tvb_get_guint8(tvb, offset);
  proto_tree_add_item(tn3270_tree,
                          hf_tn3270_aid,
                          tvb, offset,
                          1,
                          FALSE);
  offset++;
  switch (aid) {
    case  STRUCTURED_FIELD:
      offset += dissect_inbound_structured_fields(tn3270_tree, tvb, offset);
      break;
    case  READ_PARTITION_AID:
    case  NO_AID_GENERATED:
    case  NO_AID_GENERATED_(PRINTER_ONLY):
    case  TRIGGER_ACTION:
    case  TEST_REQ_AND_SYS_REQ:
    case  PF1_KEY:
    case  PF2_KEY:
    case  PF3_KEY:
    case  PF4_KEY:
    case  PF5_KEY:
    case  PF6_KEY:
    case  PF7_KEY:
    case  PF8_KEY:
    case  PF9_KEY:
    case  PF10_KEY:
    case  PF11_KEY:
    case  PF12_KEY:
    case  PF13_KEY:
    case  PF14_KEY:
    case  PF15_KEY:
    case  PF16_KEY:
    case  PF17_KEY:
    case  PF18_KEY:
    case  PF19_KEY:
    case  PF20_KEY:
    case  PF21_KEY:
    case  PF22_KEY:
    case  PF23_KEY:
    case  PF24_KEY:
    case  PA1_KEY:
    case  PA2_KEY_(CNCL):
    case  PA3_KEY:
    case  CLEAR_KEY:
    case  CLEAR_PARTITION_KEY:
    case  ENTER_KEY:
    case  SELECTOR_PEN_ATTENTION:
    case  OPERATOR_ID_READER:
    case  MAG_READER_NUMBER:
      proto_tree_add_item(tn3270_tree,
                              hf_tn3270_cursor_x,
                              tvb, offset,
                              1,
                              FALSE);
      offset++;

      proto_tree_add_item(tn3270_tree,
                              hf_tn3270_cursor_y,
                              tvb, offset,
                              1,
                              FALSE);
      offset++;
      offset += dissect_orders_and_data(tn3270_tree, tvb, offset);
      break;
    default:
      proto_tree_add_text(tn3270_tree, tvb, offset, 1, "Bogus value: %u", aid);
      offset++;
      break;
  }
  return (offset - start);
}


static void
dissect_tn3270(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
  proto_tree   *tn3270_tree;
  proto_item   *ti;
  gint         offset = 0;
  gint         tn3270_cmd = 0;
  conversation_t *conversation;
  tn3270_conv_info_t *tn3270_info = NULL;

  col_set_str(pinfo->cinfo, COL_PROTOCOL, "TN3270");

  pinfo->fd->flags.encoding = CHAR_EBCDIC;

  /* Do we have a conversation for this connection? */
  conversation = find_conversation(pinfo->fd->num, &pinfo->src, &pinfo->dst,
                                  pinfo->ptype, pinfo->srcport,
                                  pinfo->destport, 0);
  if (conversation != NULL) {
      /* Do we already have a type and mechanism? */
      tn3270_info = conversation_get_proto_data(conversation, proto_tn3270);
  }

  if (!tn3270_info)
      return;

  if (tree) {
    ti = proto_tree_add_item(tree, proto_tn3270, tvb, offset, -1, FALSE);
    tn3270_tree = proto_item_add_subtree(ti, ett_tn3270);
    if (check_col(pinfo->cinfo, COL_INFO))
        col_clear(pinfo->cinfo, COL_INFO);

    if (tn3270_info->extended) {
        offset += dissect_tn3270e_header(tn3270_tree, tvb, offset);
    }
    while (tvb_offset_exists(tvb, offset)) {
      tn3270_cmd = tvb_get_guint8(tvb, offset);
      if (pinfo->srcport == tn3270_info->outbound_port) {
        if (check_col(pinfo->cinfo, COL_INFO))
            col_set_str(pinfo->cinfo, COL_INFO, "TN3270 Data from Mainframe");
        offset += dissect_outbound_stream(tn3270_tree, tvb, offset);
      }else{
        if (check_col(pinfo->cinfo, COL_INFO))
            col_set_str(pinfo->cinfo, COL_INFO, "TN3270 Data to Mainframe");
        offset += dissect_inbound_stream(tn3270_tree, tvb, offset);
      }
    }
  }

}

void
add_tn3270_conversation(packet_info *pinfo, int tn3270e)
{
  conversation_t *conversation;
  tn3270_conv_info_t *tn3270_info = NULL;

    /*
    * Do we have a conversation for this connection?
    */
    conversation = find_conversation(pinfo->fd->num, &pinfo->src, &pinfo->dst,
                                    pinfo->ptype, pinfo->srcport,
                                    pinfo->destport, 0);
    if (conversation == NULL) {
      /* We don't yet have a conversation, so create one. */
      conversation = conversation_new(pinfo->fd->num, &pinfo->src, &pinfo->dst,
                                      pinfo->ptype, pinfo->srcport,
                                      pinfo->destport, 0);
    }

    /*
    * Do we already have a type and mechanism?
    */
    tn3270_info = conversation_get_proto_data(conversation, proto_tn3270);
    if (tn3270_info == NULL) {
      /* No.  Attach that information to the conversation, and add
      * it to the list of information structures.
      */
      tn3270_info = se_alloc(sizeof(tn3270_conv_info_t));
      COPY_ADDRESS(&(tn3270_info->outbound_addr),&(pinfo->dst));
      tn3270_info->outbound_port = pinfo->destport;
      COPY_ADDRESS(&(tn3270_info->inbound_addr),&(pinfo->src));
      tn3270_info->inbound_port = pinfo->srcport;
      conversation_add_proto_data(conversation, proto_tn3270, tn3270_info);
      tn3270_info->next = tn3270_info_items;
      tn3270_info_items = tn3270_info;
    }

    tn3270_info->extended = tn3270e;

}

int
find_tn3270_conversation(packet_info *pinfo)
{
  conversation_t *conversation = NULL;
  tn3270_conv_info_t *tn3270_info = NULL;

  /*
  * Do we have a conversation for this connection?
  */
  conversation = find_conversation(pinfo->fd->num, &pinfo->src, &pinfo->dst,
                                  pinfo->ptype, pinfo->srcport,
                                  pinfo->destport, 0);
  if (conversation != NULL) {
      tn3270_info = conversation_get_proto_data(conversation, proto_tn3270);
      if (tn3270_info != NULL) {
          /*
          * Do we already have a type and mechanism?
          */
          return 1;
      }
  }
  return 0;
}

void
proto_register_tn3270(void)
{
  static hf_register_info hf[] = {
    { &hf_tn3270_command_code,
      { "Command Code",           "tn3270.command_code",
        FT_UINT8, BASE_HEX, VALS(vals_command_codes), 0x0,
        "Command Code", HFILL }},
    { &hf_tn3270_sf_length,
      { "Structured Field Length", "tn3270.sf_length",
        FT_UINT16, BASE_DEC, NULL, 0x0,
        "Structured Field Length", HFILL }},
    /* Write Control Characters */
    { &hf_tn3270_wcc,
      { "Write Control Character", "tn3270.wcc",
        FT_UINT8, BASE_HEX, NULL, 0,
        NULL, HFILL }},
    { &hf_tn3270_wcc_nop,
        { "WCC NOP",
            "tn3270.wcc.nop", FT_BOOLEAN, 8, NULL, 0x01, NULL, HFILL }},
    { &hf_tn3270_wcc_reset,
        { "WCC Reset",
            "tn3270.wcc.reset", FT_BOOLEAN, 8, NULL, 0x02, NULL, HFILL }},
    { &hf_tn3270_wcc_printer1,
        { "WCC Printer1",
            "tn3270.wcc.printer1", FT_BOOLEAN, 8, NULL, 0x04, NULL, HFILL }},
    { &hf_tn3270_wcc_printer2,
        { "WCC Printer2",
            "tn3270.wcc.printer2", FT_BOOLEAN, 8, NULL, 0x08, NULL, HFILL }},
    { &hf_tn3270_wcc_start_printer,
        { "WCC Start Printer",
            "tn3270.wcc.start_printer", FT_BOOLEAN, 8, NULL, 0x10, NULL, HFILL }},
    { &hf_tn3270_wcc_sound_alarm,
        { "WCC Sound Alarm",
            "tn3270.wcc.sound_alarm", FT_BOOLEAN, 8, NULL, 0x20, NULL, HFILL }},
    { &hf_tn3270_wcc_keyboard_restore,
        { "WCC Keyboard Restore",
            "tn3270.wcc.keyboard_restore", FT_BOOLEAN, 8, NULL, 0x40, NULL, HFILL }},
    { &hf_tn3270_wcc_reset_mdt,
        { "WCC Reset MDT",
            "tn3270.wcc.reset_mdt", FT_BOOLEAN, 8, NULL, 0x80, NULL, HFILL }},

    /* 8.7 Copy Control Codes (CCC) */
    { &hf_tn3270_ccc,
      { "Copy Control Code", "tn3270.ccc", FT_UINT8, BASE_HEX,
        NULL, 0, NULL, HFILL }},
    { &hf_tn3270_ccc_coding,
      { "Coding", "tn3270.ccc_coding", FT_UINT8, BASE_HEX,
        VALS(vals_coding), CODING_BITS, NULL, HFILL }},
    { &hf_tn3270_ccc_printout,
      { "Printout Format", "tn3270.ccc_printout", FT_UINT8, BASE_HEX,
        VALS(vals_printout_format), PRINT_BITS, NULL, HFILL }},
    { &hf_tn3270_ccc_start_print,
      { "The start-print bit",
          "tn3270.ccc_start_print", FT_BOOLEAN, 8, NULL, START_PRINT, NULL, HFILL }},
    { &hf_tn3270_ccc_sound_alarm,
      { "The sound-alarm bit",
          "tn3270.ccc_sound_alarm", FT_BOOLEAN, 8, NULL, SOUND_ALARM, NULL, HFILL }},
    { &hf_tn3270_ccc_copytype,
      { "Type of Data to be Copied", "tn3270.ccc_copytype", FT_UINT8, BASE_HEX,
        VALS(vals_copytype), ATTRIBUTE_BITS, NULL, HFILL }},

    /* 4.4.1 Field Attributes */
    { &hf_tn3270_field_attribute,
        { "3270 Field Attribute", "tn3270.field_attribute", FT_UINT8, BASE_HEX,
          NULL, 0, NULL, HFILL }},
    { &hf_tn3270_fa_graphic_convert1,
        { "Graphic Convert1",
            "tn3270.fa.graphic_convert1", FT_BOOLEAN, 8, NULL, GRAPHIC_CONVERT1, NULL, HFILL }},
    { &hf_tn3270_fa_graphic_convert2,
        { "Graphic Convert2",
            "tn3270.fa.graphic_convert2", FT_BOOLEAN, 8, NULL, GRAPHIC_CONVERT2, NULL, HFILL }},
    { &hf_tn3270_fa_protected,
        { "Protected",
            "tn3270.fa.protected", FT_BOOLEAN, 8, NULL, PROTECTED, NULL, HFILL }},
    { &hf_tn3270_fa_numeric,
        { "Numeric",
            "tn3270.fa.numeric", FT_BOOLEAN, 8, NULL, NUMERIC, NULL, HFILL }},
    { &hf_tn3270_fa_display,
        { "Display", "tn3270.fa.display", FT_UINT8, BASE_HEX,
          VALS(vals_fa_display), DISPLAY_BITS, NULL, HFILL }},
    { &hf_tn3270_fa_reserved,
        { "Reserved",
            "tn3270.fa.reserved", FT_BOOLEAN, 8, NULL, RESERVED, NULL, HFILL }},
    { &hf_tn3270_fa_modified,
        { "Modified",
            "tn3270.fa.modified", FT_BOOLEAN, 8, NULL, MODIFIED, NULL, HFILL }},

    /* Order Code */
    { &hf_tn3270_order_code,
      { "Order Code",           "tn3270.order_code",
        FT_UINT8, BASE_HEX, VALS(vals_order_codes), 0x0,
        NULL, HFILL }},
    { &hf_tn3270_character_code,
      { "Character Code",           "tn3270.character_code",
        FT_UINT8, BASE_HEX, NULL, 0x0,
        NULL, HFILL }},
    { &hf_tn3270_stop_address,
      { "Stop Address",           "tn3270.stop_address",
        FT_UINT16, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},
    { &hf_tn3270_attribute_type,
      { "Attribute Type",           "tn3270.attribute_type",
        FT_UINT8, BASE_HEX, VALS(vals_attribute_types), 0x0,
        NULL, HFILL }},
    { &hf_tn3270_extended_highlighting,
      { "Extended Highlighting",           "tn3270.extended_highlighting",
        FT_UINT8, BASE_HEX, VALS(vals_extended_highlighting), 0x0,
        NULL, HFILL }},
    { &hf_tn3270_color,
      { "Color",           "tn3270.color",
        FT_UINT8, BASE_HEX, VALS(vals_color_identifications), 0x0,
        NULL, HFILL }},
    { &hf_tn3270_character_set,
      { "Character Set",           "tn3270.color",
        FT_UINT8, BASE_HEX, RVALS(rvals_character_set), 0x0,
        NULL, HFILL }},
    { &hf_tn3270_field_outlining,
      { "Field Outlining",           "tn3270.field_outlining",
        FT_UINT8, BASE_HEX, VALS(vals_field_outlining), 0x0,
        NULL, HFILL }},
    { &hf_tn3270_transparency,
      { "Transparency",           "tn3270.transparency",
        FT_UINT8, BASE_HEX, VALS(vals_transparency), 0x0,
        NULL, HFILL }},
    { &hf_tn3270_field_validation_mandatory_fill,
      { "3270 Field validation_mandatory_fill", "tn3270.field_validation_mandatory_fill",
        FT_BOOLEAN, 8, TFS(&tn3270_field_validation_mandatory_fill),
        RESERVED, NULL, HFILL }},
    { &hf_tn3270_field_validation_trigger,
      { "3270 Field validation_mandatory_trigger", "tn3270.field_validation_mandatory_trigger",
        FT_BOOLEAN, 8, TFS(&tn3270_field_validation_trigger),
        RESERVED, NULL, HFILL }},
    { &hf_tn3270_field_validation_mandatory_entry,
      { "3270 Field validation_mandatory_entry", "tn3270.field_validation_mandatory_entry",
        FT_BOOLEAN, 8, TFS(&tn3270_field_validation_mandatory_entry),
        RESERVED, NULL, HFILL }},
    { &hf_tn3270_all_character_attributes,
      { "all_character_attributes",           "tn3270.all_character_attributes",
        FT_UINT8, BASE_HEX, NULL, 0x0,
        NULL, HFILL }},
    { &hf_tn3270_aid,
      { "Attention Identification", "hf_tn3270_aid",
        FT_UINT8, BASE_HEX, VALS(vals_attention_identification_bytes), 0x0,
        NULL, HFILL }},

    { &hf_tn3270_buffer_x,
      { "Buffer X", "tn3270.buffer_x",
        FT_UINT8, BASE_HEX, NULL, 0x0,
        NULL, HFILL }},
    { &hf_tn3270_buffer_y,
      { "Buffer Y", "tn3270.buffer_y",
        FT_UINT8, BASE_HEX, NULL, 0x0,
        NULL, HFILL }},

    /* Self Defining Parameters */
    { &hf_tn3270_sdp_ln,
        {  "Length of this Self-Defining Parameter", "tn3270.sdp_ln",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_sdp_id,
        {  "Self-Defining Parameter ID", "tn3270.sdp_id",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    /* Self Defining Parameters */

    /* 5.6 - Begin/End of File */
    { &hf_tn3270_begin_end_flags1,
        {  "Begin End Flags1", "tn3270.begin_end_flags1",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_begin_end_flags2,
        {  "Begin End Flags2", "tn3270.begin_end_flags2",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    /* END - 5.6 - Begin/End of File */

    /* 5.7 - Create Partition */
    { &hf_tn3270_partition_id,
        {  "Partition ID", "tn3270.partition_id",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_partition_uom,
        {  "The unit of measure and address mode", "tn3270.partition_uom",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_partition_flags,
        {  "Flags", "tn3270.partition_flags",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_partition_height,
        {  "The height of the presentation space", "tn3270.partition_height",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_partition_width,
        {  "The width of the presentation space", "tn3270.partition_width",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_partition_rv,
        {  "The y, or row, origin of the viewport relative to the top edge of the usable area", "hf_tn3270_partition_rv",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_partition_cv,
        {  "The x, or column, origin of the viewport relative to the left side of the usable area", "tn3270.partition_cv",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_partition_hv,
        {  "The height of the viewport", "tn3270.partition_hv",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_partition_wv,
        {  "The width of the viewport", "tn3270.partition_wv",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_partition_rw,
        {  "The y, or row, origin of the window relative to the top edge of the presentation space", "tn3270.partition_rw",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_partition_cw,
        {  "The x, or column, origin of the window relative to the left edge of the presentation  space", "tn3270.partition_cw",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_partition_rs,
        {  "The number of units to be scrolled in a vertical multiple scroll", "tn3270.partition_rs",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_partition_res,
        {  "Reserved", "tn3270.partition_res",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_partition_pw,
        {  "The number of points in the horizontal direction in a character cell in this presentation space", "tn3270.partition_pw",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_partition_ph,
        {  "The number of points in the vertical direction in a character cell in this presentation space", "tn3270.partition_ph",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

    { &hf_tn3270_partition_command,
        {  "Partition Command", "hf_tn3270_partition_command",
            FT_UINT8, BASE_HEX, VALS(vals_command_codes), 0x0,
            NULL, HFILL }},
    /* End - 5.7 - Create Partition */

    /* 5.9 - Erase/Reset */
    { &hf_tn3270_erase_flags,
        {  "tn3270.erase_flags", "tn3270.erase_flags",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            "tn3270.erase_flags", HFILL }},
    /* End - 5.9 - Erase/Reset */

    /* 5.10 - Load Color Table */
    { &hf_tn3270_load_color_command,
        {  "Command", "tn3270.load_color_command",
            FT_UINT16, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    /* End - 5.10 - Load Color Table */

    /* 5.11 - Load Format Storage */
    { &hf_tn3270_load_format_storage_flags1,
        {  "Flags", "tn3270.load_format_storage_flags1",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_load_format_storage_flags2,
        {  "Flags (Reserved)", "tn3270.load_format_storage_flags2",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_load_format_storage_operand,
        {  "Operand:", "tn3270.load_format_storage_operand",
            FT_UINT8, BASE_HEX, VALS(vals_operand), 0x0,
            NULL, HFILL }},
    { &hf_tn3270_load_format_storage_localname,
        {  "Local name for user selectable formats", "tn3270.load_format_storage_localname",
            FT_EBCDIC, BASE_NONE, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_format_group,
        {  "Format Group name", "tn3270.format_group_name",
            FT_EBCDIC, BASE_NONE, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_format_name,
        {  "Format name", "tn3270.format_name",
            FT_EBCDIC, BASE_NONE, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_load_format_storage_format_data,
        {  "Format data", "tn3270.load_format_storage_format_data",
            FT_EBCDIC, BASE_NONE, NULL, 0x0,
            NULL, HFILL }},
    /* END - 5.11 - Load Format Storage */

    /* 5.12 - Load Line Type */
    { &hf_tn3270_load_line_type_command, 
        {  "Line Type Command", "tn3270.load_line_type_command",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},

    /* 5.13 - Load Programmed Symbols */
    { &hf_tn3270_ps_flags,
        {  "Flags", "tn3270.ps_flags",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_ps_lcid,
        {  "Local character set ID", "tn3270.ps_lcid",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_ps_char,
        {  "Beginning code point X'41' through X'FE'", "tn3270.ps_char",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_ps_rws,
        {  "Loadable Character Set RWS Number", "tn3270.ps_rws",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_extended_ps_length,
        {  "Length of parameters for extended form, including the length parameter", "tn3270.extended_ps_length",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_extended_ps_flags,
        {  "Flags", "tn3270.extended_ps_flags",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_extended_ps_lw,
        {  "Number of X-units in character cell (width of character matrixes)", "tn3270.extended_ps_lw",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_extended_ps_lh,
        {  "Number of Y-units in character cell (depth ofcharacter matrixes)", "tn3270.extended_ps_lh",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_extended_ps_subsn,
        {  "Subsection ID", "tn3270.extended_ps_subsn",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_extended_ps_color,
        {  " Color planes", "tn3270.extended_ps_color",
            FT_UINT8, BASE_HEX, VALS(vals_color_identifications), 0x0,
            NULL, HFILL }},
    { &hf_tn3270_extended_ps_stsubs,
        {  "Starting Subsection Identifier", "tn3270.extended_ps_stsubs",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_extended_ps_echar,
        {  "Ending code point", "tn3270.extended_ps_echar",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_extended_ps_nw,
        {  "Number of width pairs", "tn3270.extended_ps_nw",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_extended_ps_nh,
        {  "Number of height pairs", "tn3270.extended_ps_nh",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_extended_ps_res,
        {  "Reserved", "tn3270.extended_ps_res",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    /* END - 5.13 - Load Programmed Symbols */

    /* 5.15 - Outbound Text Header */
    { &hf_tn3270_operation_type,
        {  "Operation Type", "tn3270.operation_type",
            FT_UINT8, BASE_HEX, VALS(vals_operation_types), 0x0,
            NULL, HFILL }},
    { &hf_tn3270_lvl,
        {  "Cursor level", "tn3270.lvl",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_cro,
        {  "Cursor row offset", "tn3270.cro",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_cc,
        {  "Cursor column offset", "tn3270.cc",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_outbound_text_header_lhdr,
        {  "Header length includes itself", "tn3270.outbound_text_header_lhdr",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_outbound_text_header_hdr,
        {  "Initial format controls", "tn3270.outbound_text_header_hdr",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    /* END - 5.15 - Outbound Text Header */

    /* 5.16 - Outbound 3270DS */
    { &hf_tn3270_bsc,
        {  "SNA BSC", "tn3270.bsc",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    /* END - 5.16 - Outbound 3270DS */

    /* 5.17 - Present Absolute Format */
    { &hf_tn3270_fpc,
        {  "Format Presentation Command", "tn3270.fpc",
            FT_UINT8, BASE_HEX, VALS(vals_command_codes), 0x0,
            NULL, HFILL }},
    /* END - 5.17 - Present Absolute Format */

    /* 5.18 - Present Relative Format */
    { &hf_tn3270_fov,
        {  "Format Offset Value", "tn3270.fov",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    /* End - 5.18 - Present Relative Format */

    /* 5.19 - Read Partition */
    { &hf_tn3270_reqtyp,
        {  "Request Type", "tn3270.reqtyp",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    /* End - 5.19 - Read Partition */

    /* 5.22 - Restart */
    { &hf_tn3270_start_page,
        {  "Number of pages to skip on restart", "tn3270.start_page",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_start_line,
        {  "Number of lines to skip on page for restart", "tn3270.start_line",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_scs_data,
        {  "SCS data (noncompressed and noncompacted) to set up for restart", "tn3270.scs_data",
            FT_BYTES, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    /* End - 5.22 - Restart */

    /* 5.24 - Select Color Table */
    { &hf_tn3270_color_command,
        {  "Color Command", "tn3270.color_command",
            FT_UINT16, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    /* 5.24 - Select Color Table */

    /* 5.26 - Set Checkpoint Interval */
    { &hf_tn3270_interval,
        {  "Checkpoint interval", "tn3270.interval",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            "Specifies the number of pages in the interval between terminal checkpoints", HFILL }},
    /* End - 5.26 - Set Checkpoint Interval */

    /* 5.27 - Set MSR Interval */
    { &hf_tn3270_msr_type,
        {  "MSR type", "tn3270.msr_type",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_msr_state_mask,
        {  "State Mask", "tn3270.msr_state_mask",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_msr_user,
        { "User Mode",
            "tn3270.msr.user", FT_BOOLEAN, 8, NULL, 0x80, NULL, HFILL }},
    { &hf_tn3270_msr_locked,
        { "Locked",
            "tn3270.msr.locked", FT_BOOLEAN, 8, NULL, 0x40, NULL, HFILL }},
    { &hf_tn3270_msr_auto,
        { "Auto Enter",
            "tn3270.msr.auto", FT_BOOLEAN, 8, NULL, 0x20, NULL, HFILL }},
    { &hf_tn3270_msr_ind1,
        { "Audible Ind 1 Suppress",
            "tn3270.msr.ind1", FT_BOOLEAN, 8, NULL, 0x10, NULL, HFILL }},
    { &hf_tn3270_msr_ind2,
        { "Audible Ind 2 Suppress",
            "tn3270.msr.ind2", FT_BOOLEAN, 8, NULL, 0x08, NULL, HFILL }},
    { &hf_tn3270_msr_state_value,
        {  "State Value", "tn3270.msr_state_value",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_msr_ind_mask,
        {  "Indicator Mask", "tn3270.msr_ind_mask",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_msr_ind_value,
        {  "Indicator Value", "tn3270.msr_ind_value",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    /* END -  5.27 - Set MSR Interval */

    /* 5.28 - Set Partition Characteristics */
    { &hf_tn3270_spc_sdp_ot,
        {  "Top edge outline thickness", "tn3270.spc_sdp_ot",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_spc_sdp_ob,
        {  "Bottom edge outline thickness", "tn3270.spc_sdp_ob",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_spc_sdp_ol,
        {  "Left edge outline thickness", "tn3270.spc_sdp_ol",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_spc_sdp_or,
        {  "Right edge outline thickness", "tn3270.spc_sdp_or",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_spc_sdp_eucflags,
        {  "Flags", "tn3270.spc_sdp_eucflags",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    /* END - 5.28 - Set Partition Characteristics */

    /* 5.29 - Set Printer Characteristics */
    { &hf_tn3270_printer_flags,
        {  "Flags", "tn3270.printer_flags",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_spc_sdp_srepc,
        {  "Set/Reset Early Print Complete", "tn3270.spc_sdp_srepc",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    /* END - 5.29 - Set Printer Characteristics */

    /* 5.30 - Set Reply Mode */
    { &hf_tn3270_mode,
        {  "Mode", "tn3270.mode",
            FT_UINT8, BASE_HEX, VALS(vals_modes), 0x0,
            NULL, HFILL }},
    { &hf_tn3270_reply_mode_attr_list,
        {  "Type codes for the attribute types", "tn3270.reply_mode_attr_list",
            FT_UINT8, BASE_HEX, VALS(vals_attribute_types), 0x0,
            NULL, HFILL }},
    /* END - 5.30 - Set Reply Mode */

    /* 5.34 - Data Chain */
    { &hf_tn3270_data_chain_bitmask,
        {  "Mask", "tn3270.data_chain_bitmask",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    /* END - 5.34 - Data Chain */

    /* 5.35 - Destination/Origin */
    { &hf_tn3270_destination_or_origin_bitmask,
        {  "Mask", "tn3270.destination_or_origin_bitmask",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    /* END - 5.35 - Destination/Origin */


    /* 5.36 - Object Control */
    { &hf_tn3270_object_control_flags,
        {  "Flags", "tn3270.object_control_flags",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_object_type,
        {  "Object Type", "tn3270.object_type",
            FT_UINT8, BASE_HEX, VALS(vals_oc_type), 0x0,
            NULL, HFILL }},
    /* END - 5.36 - Object Control */

    /* 5.40 - Save/Restore Format */
    { &hf_tn3270_save_or_restore_format_flags,
        {  "Flags", "tn3270.save_or_restore_format_flags",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
   { &hf_tn3270_srf_fpcb,
        {  "Contents of the FPCB that is to be saved or restored", "tn3270.srf_fpcb",
            FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }},
   /* 5.40 - Save/Restore Format */

   { &hf_tn3270_type_1_text_outbound_data,
        {  "tn3270.type_1_text_outbound_data", "tn3270.type_1_text_outbound_data",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},

    /* 6.2 - Exception/Status */
     { &hf_tn3270_exception_or_status_flags,
        {  "Flags", "tn3270.exception_or_status_flags",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_sdp_excode,
        {  "Exception Code", "tn3270.sdp_excode",
            FT_UINT16, BASE_DEC, VALS(vals_excode), 0x0,
            NULL, HFILL }},
    { &hf_tn3270_sdp_statcode,
        {  "Status Code", "tn3270.sdp_statcode",
            FT_UINT16, BASE_DEC, VALS(vals_statcode), 0x0,
            NULL, HFILL }},
    { &hf_tn3270_sdp_ngl,
        {  "Number of groups currently assigned", "tn3270.sdp_ngl",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_sdp_nml,
        {  "Number of formats currently loaded", "tn3270.sdp_nml",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_sdp_nlml,
        {  "Number of local names used", "tn3270.sdp_nlml",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_sdp_stor,
        {  "Amount of format storage space available (KB)", "tn3270.sdp_stor",
            FT_UINT32, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    /* 6.2 - Exception/Status */

    /* 6.3 - Inbound Text Header */
    { &hf_tn3270_hw,
        {  "Window height", "tn3270.hw",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_rw,
        {  "Row offset of window origin", "tn3270.rw",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_ww,
        {  "Window width", "tn3270.ww",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_cw,
        {  "Column Offset of Window Origin", "tn3270.cw",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    /* END - 6.3 - Inbound Text Header */

    /* 6.4 Inbound 3270DS */
    { &hf_tn3270_cursor_x,
        {  "Cursor X", "tn3270.cursor_x",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_cursor_y,
        {  "Cursor Y", "tn3270.cursor_y",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    /* END - 6.4 Inbound 3270DS */

    /* 6.5 - Recovery Data */
    { &hf_tn3270_recovery_data_flags,
        {  "Flags", "tn3270.recovery_data_flags",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_sld,
        {  "SLD -- Set line density parameter in effect at the checkpoint", "tn3270.sld",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_charset,
        {  "Character set parameter of Set Attribute control in effect at the checkpoint", "tn3270.charset",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_vertical,
        {  "Byte offset from Checkpoint Interval structured field to the Set Vertical Format control in effect for the checkpoint", "tn3270.vertical",
            FT_UINT32, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_v_offset,
        {  "Byte offset within the string control byte string or the SVF character", "tn3270.v_offset",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_v_sequence,
        {  "RU sequence number", "tn3270.v_sequence",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_v_length,
        {  "Length of the SVF character string required for restart", "tn3270.v_length",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_spd,
        {  "Set Primary Density parameter in effect at the checkpoint", "tn3270.spd",
            FT_UINT16, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_horizon,
        {  "Byte offset from Checkpoint Interval structured field to the Set Horizontal Format control in effect for the checkpoint", "tn3270.horizon",
            FT_UINT32, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_h_offset,
        {  "Byte offset from Checkpoint Interval structured field to the Set Horizontal Format control in effect for the checkpoint", "tn3270.h_offset",
            FT_UINT32, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_h_sequence,
        {  "RU sequence number", "tn3270.h_sequence",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_h_length,
        {  "Length of the SHF character string required for restart", "tn3270.h_length",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_hilite,
        {  "Highlighting", "tn3270.hilite",
            FT_UINT8, BASE_HEX, VALS(vals_extended_highlighting), 0x0,
            NULL, HFILL }},
    { &hf_tn3270_pages,
        {  "Number of pages printed since the checkpoint", "tn3270.pages",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_lines,
        {  "Number of lines printed since the checkpoint", "tn3270.lines",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_checkpoint,
        {  "Byte offset from Set Checkpoint Interval structured field to the first "
            "character afterhe code point or character that caused an eject to the "
            "checkpointed page", "tn3270.checkpoint",
            FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
    { &hf_tn3270_c_offset,
        {  "Byte offset within the String Control Byte string or structured field of the checkpointed character", "tn3270.c_offset",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_c_sequence,
        {  "RU sequence number of the RU containing the checkpoint character", "tn3270.c_sequence",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_c_seqoff,
        {  "Byte offset within the RU of the checkpointed character", "tn3270.c_seqoff",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_c_scsoff,
        {  "Byte offset within the parameterized SCS control code (for example, TRN) of the checkpointed character.", "tn3270.c_scsoff",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_prime,
        {  "Prime compression character", "tn3270.prime",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    /* END - 6.5 - Recovery Data */

    /* 6.9 - Query Reply (Alphanumeric Partitions) */
    { &hf_tn3270_ap_na,
        {  " Max number of alphanumeric partitions", "tn3270.ap_na",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_ap_m,
        {  "Total available partition storage", "tn3270.ap_m",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_query_reply_alphanumeric_flags,
        {  "Flags", "tn3270.ap_flags",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_ap_vertical_scrolling,
        { "Vertical Scrolling Supported",
            "tn3270.ap_vertical_scrolling", FT_BOOLEAN, 8, NULL, VERTWIN, NULL, HFILL }},
    { &hf_tn3270_ap_horizontal_scrolling,
        { "Horizontal Scrolling Supported",
            "tn3270.ap_horizontal_scrolling", FT_BOOLEAN, 8, NULL, HORWIN, NULL, HFILL }},
    { &hf_tn3270_ap_apres1,
        { "Reserved",
            "tn3270.ap_apres1", FT_BOOLEAN, 8, NULL, APRES1, NULL, HFILL }},
    { &hf_tn3270_ap_apa,
        { "All Points addressability supported",
            "tn3270.ap_apa", FT_BOOLEAN, 8, NULL, APA_FLG, NULL, HFILL }},
    { &hf_tn3270_ap_pp,
        { "Partition protection supported",
            "tn3270.ap_pp", FT_BOOLEAN, 8, NULL, PROT, NULL, HFILL }},
    { &hf_tn3270_ap_lc,
        { "Presentation space local copy supported",
            "tn3270.ap_lc", FT_BOOLEAN, 8, NULL, LCOPY, NULL, HFILL }},
    { &hf_tn3270_ap_mp,
        { "Modify Partition supported",
            "tn3270.ap_mp", FT_BOOLEAN, 8, NULL, MODPART, NULL, HFILL }},
    { &hf_tn3270_ap_apres2,
        { "Reserved",
            "tn3270.ap_apres2", FT_BOOLEAN, 8, NULL, APRES2, NULL, HFILL }},

    { &hf_tn3270_ap_cm,
        {  "Character multiplier", "tn3270.ap_cm",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_ap_ro,
        {  "Row overhead", "tn3270.ap_ro",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_ap_co,
        {  "Column overhead", "tn3270.ap_co",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_ap_fo,
        {  "Fixed overhead", "tn3270.ap_fo",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    /* END - 6.9 - Query Reply (Alphanumeric Partitions) */

    /* 6.12 - Query Reply (Character Sets) */
    { &hf_tn3270_character_sets_flags1,
        {  "Flags (1)", "tn3270.character_sets_flags1",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_cs_ge,
        { "Graphic Escape supported",
            "tn3270.cs_ge", FT_BOOLEAN, 8, NULL, ALT, NULL, HFILL }},
    { &hf_tn3270_cs_mi,
        { "Multiple LCIDs are supported",
            "tn3270.cs_mi", FT_BOOLEAN, 8, NULL, MULTID, NULL, HFILL }},
    { &hf_tn3270_cs_lps,
        { "Load PSSF is supported",
            "tn3270.cs_lps", FT_BOOLEAN, 8, NULL, LOADABLE, NULL, HFILL }},
    { &hf_tn3270_cs_lpse,
        { "Load PS EXTENDED is supported",
            "tn3270.cs_lpse", FT_BOOLEAN, 8, NULL, EXT, NULL, HFILL }},
    { &hf_tn3270_cs_ms,
        { "More than one size of character slot is supported",
            "tn3270.cs_ms", FT_BOOLEAN, 8, NULL, MS, NULL, HFILL }},
    { &hf_tn3270_cs_ch2,
        { "Two-byte coded character sets are supported",
            "tn3270.cs_ch2", FT_BOOLEAN, 8, NULL, CH2, NULL, HFILL }},
    { &hf_tn3270_cs_gf,
        { "CGCSGID is present",
            "tn3270.cs_gf", FT_BOOLEAN, 8, NULL, GF, NULL, HFILL }},
    { &hf_tn3270_cs_res,
        { "Reserved",
            "tn3270.cs_res", FT_BOOLEAN, 8, NULL, CSRES, NULL, HFILL }},

    { &hf_tn3270_character_sets_flags2,
        {  "Flags (2)", "tn3270.character_sets_flags2",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_cs_res2,
        { "Reserved",
            "tn3270.cs_res2", FT_BOOLEAN, 8, NULL, CSRES2, NULL, HFILL }},
    { &hf_tn3270_cs_pscs,
        { "Load PS slot size match not required",
            "tn3270.cs_pscs", FT_BOOLEAN, 8, NULL, PSCS, NULL, HFILL }},
    { &hf_tn3270_cs_res3,
        { "Reserved",
            "tn3270.cs_res3", FT_BOOLEAN, 8, NULL, CSRES3, NULL, HFILL }},
    { &hf_tn3270_cs_cf,
        { "CCSID present",
            "tn3270.cs_cf", FT_BOOLEAN, 8, NULL, CF, NULL, HFILL }},

    { &hf_tn3270_sdw,
        {  "Default character slot width", "tn3270.cs_sdw",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_sdh,
        {  "Default character slot height", "tn3270.cs_sdh",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_form,
        {  "Form Types", "tn3270.form",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_formres,
        {  "Form Types (Reserved)", "tn3270.formres",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_formres2,
        {  "Form Types (Reserved)", "tn3270.formres2",
            FT_UINT16, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_cs_form_type1,
        { "18-byte form; the first 2 bytes contain a 16-bit vertical slice, "
           "the following 16 bytes contain 8-bit horizontal slices. For a 9 "
           "x 12 character matrix the last 4 bytes contain binary zero.",
           "tn3270.cs_form_type1", FT_BOOLEAN, 8, NULL, 0x80, NULL, HFILL }},
    { &hf_tn3270_cs_form_type2,
        { "18-byte form; the first 2 bytes contain a 16-bit vertical slice, "
           "the following 16 bytes contain 8-bit horizontal slices. For a 9 "
           "x 12 character matrix the last 4 bytes contain binary zero. (COMPRESSED)",
           "tn3270.cs_form_type2", FT_BOOLEAN, 8, NULL, 0x40, NULL, HFILL }},
    { &hf_tn3270_cs_form_type3,
        { "Row loading (from top to bottom)",
            "tn3270.cs_form_type3", FT_BOOLEAN, 8, NULL, 0x20, NULL, HFILL }},
    { &hf_tn3270_cs_form_type4,
        { "Row loading (from top to bottom) (Compressed)",
            "tn3270.cs_form_type4", FT_BOOLEAN, 8, NULL, 0x10, NULL, HFILL }},
    { &hf_tn3270_cs_form_type5,
        { "Column loading (from left to right)",
            "tn3270.cs_form_type5", FT_BOOLEAN, 8, NULL, 0x08, NULL, HFILL }},
    { &hf_tn3270_cs_form_type6,
        { "Column loading (from left to right) (Compressed)",
            "tn3270.cs_form_type6", FT_BOOLEAN, 8, NULL, 0x04, NULL, HFILL }},
    { &hf_tn3270_cs_form_type8,
        { "Vector",
            "tn3270.cs_form_type8", FT_BOOLEAN, 8, NULL, 0x02, NULL, HFILL }},
    { &hf_tn3270_cs_dl,
        {  "Length of each descriptor", "tn3270.cs_dl",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

    { &hf_tn3270_cs_descriptor_set,
        {  "Device Specific Character Set ID (PS store No.)", "tn3270.cs_descriptor_set",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_cs_descriptor_flags,
        {  "Flags", "tn3270.cs_descriptor_flags",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_cs_ds_load,
        { "Loadable character set",
            "tn3270.cs_ds_load", FT_BOOLEAN, 8, NULL, 0x80, NULL, HFILL }},
    { &hf_tn3270_cs_ds_triple,
        { "Triple-plane character set",
            "tn3270.cs_ds_triple", FT_BOOLEAN, 8, NULL, 0x40, NULL, HFILL }},
    { &hf_tn3270_cs_ds_char,
        { "Double-Byte coded character set",
            "tn3270.cs_ds_char", FT_BOOLEAN, 8, NULL, 0x20, NULL, HFILL }},
    { &hf_tn3270_cs_ds_cb,
        { "No LCID compare",
            "tn3270.cs_ds_cb", FT_BOOLEAN, 8, NULL, 0x10, NULL, HFILL }},

    { &hf_tn3270_lcid,
        {  "Local character set ID (alias)", "tn3270.lcid",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_sw,
        {  "Width of the character slots in this characterset.", "tn3270.sw",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_sh,
        {  "Height of the character slots in this character set.", "tn3270.sh",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_ssubsn,
        {  "Starting subsection.", "tn3270.ssubsn",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_esubsn,
        {  "Ending subsection.", "tn3270.esubsn",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_ccsgid,
        {  "Coded Graphic Character Set Identifier.", "tn3270.ccsgid",
            FT_UINT64, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_ccsid,
        {  "Coded Character Set Identifier.", "tn3270.ccsid",
            FT_UINT64, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    /* END - 6.12 - Query Reply (Character Sets) */

    /* 6.13 - Query Reply (Color) */
    { &hf_tn3270_color_flags,
        {  "Flags", "tn3270.color_flags",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_c_prtblk,
        { "Printer only - black ribbon is loaded",
            "tn3270.cc_prtblk", FT_BOOLEAN, 8, NULL, 0x40, NULL, HFILL }},
    { &hf_tn3270_c_np,
        {  "Length of color attribute list", "tn3270.np",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_c_cav,
        {  "Color attribute value accepted by the device", "tn3270.c_cav",
            FT_UINT8, BASE_HEX, VALS(vals_color_identifications), 0x0,
            NULL, HFILL }},
    { &hf_tn3270_c_ci,
        {  "Color identifier", "tn3270.c_ci",
            FT_UINT8, BASE_HEX, VALS(vals_color_identifications), 0x0,
            NULL, HFILL }},
    { &hf_tn3270_db_cavdef,
        {  "Default color attribute value", "tn3270.db_cavdef",
            FT_UINT8, BASE_HEX, VALS(vals_color_identifications), 0x0,
            NULL, HFILL }},
    { &hf_tn3270_db_cidef,
        {  "Default background color identifier", "tn3270.db_cidef",
            FT_UINT8, BASE_HEX, VALS(vals_color_identifications), 0x0,
            NULL, HFILL }},
    /* END - 6.13 - Query Reply (Color) */

    /* 6.14 - Query Reply (Cooperative Processing Requestor) */
    { &hf_tn3270_limin,
        {  "Maximum CPR bytes/transmission allowed inbound", "tn3270.limin",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_limout,
        {  "Maximum CPR bytes/transmission allowed outbound", "tn3270.limout",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_featl,
        {  "Length (in bytes) of feature information that follows", "tn3270.featl",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_feats,
        {  "CPR length and feature flags", "tn3270.feats",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    /* END - 6.14 - Query Reply (Cooperative Processing Requestor) */

    /* 6.15 - Query Reply (Data Chaining) */
    { &hf_tn3270_dc_dir_flags,
        {  "Indicates which direction can use the Data Chain structured field.", "tn3270.dc_dir_flags",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_dc_both,
        { "Both",
            "tn3270.dc_both", FT_BOOLEAN, 8, NULL, 0x00, NULL, HFILL }},
    { &hf_tn3270_dc_from_device,
        { "From Device Only",
            "tn3270.dc_device", FT_BOOLEAN, 8, NULL, 0x10, NULL, HFILL }},
    { &hf_tn3270_dc_to_device,
        { "To Device Only",
            "tn3270.dc_to_device", FT_BOOLEAN, 8, NULL, 0x20, NULL, HFILL }},
    /* END - 6.15 - Query Reply (Data Chaining) */

    /* 6.16 - Query Reply (Data Streams) */
    { &hf_tn3270_ds_default_sfid,
        {  "Default Data Stream", "tn3270.ds_default_sfid",
            FT_UINT8, BASE_HEX, VALS(vals_data_streams), 0x0,
            NULL, HFILL }},
    { &hf_tn3270_ds_sfid,
        {  "Supported Data Stream", "tn3270.ds_sfid",
            FT_UINT8, BASE_HEX, VALS(vals_data_streams), 0x0,
            NULL, HFILL }},
    /* END - 6.16 - Query Reply (Data Streams) */

    /* 6.17 - Query Reply (DBCS Asia) */
    { &hf_tn3270_asia_sdp_sosi_soset,
        {  "Set ID of the Shift Out (SO) character set", "tn3270.asia_sdp_sosi_soset",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            "Set ID of the Shift Out (SO) character set", HFILL }},
    { &hf_tn3270_asia_sdp_ic_func,
        { "SO/SI Creation supported",
            "tn3270.asia_sdp_ic_func", FT_BOOLEAN, 8, NULL, 0x01, NULL, HFILL }},
    /* END - 6.17 - Query Reply (DBCS Asia) */

    /* 6.19 - Query Reply (Distributed Data Management) */
    { &hf_tn3270_ddm_flags,
        {  "Flags (Reserved)", "tn3270.ddm_flags",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_ddm_limin,
        {  "Maximum DDM bytes/transmission allowed inbound", "tn3270.ddm_limin",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_ddm_limout,
        {  "Maximum DDM bytes/transmission allowed outbound", "tn3270.ddm_limout",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_ddm_nss,
        {  "Number of subsets supported", "tn3270.ddm_nss",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_ddm_ddmss,
        {  " DDM subset identifier", "tn3270.ddm_ddmss",
            FT_UINT8, BASE_HEX, VALS(vals_ddm), 0x0,
            NULL, HFILL }},
    /* END - 6.19 - Query Reply (Distributed Data Management) */

    /* 6.20 - Query Reply (Document Interchange Architecture) */
    { &hf_tn3270_dia_flags,
        {  "Flags (Reserved)", "tn3270.dia_flags",
            FT_UINT16, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_dia_limin,
        {  "Maximum DIA bytes/transmission allowed inbound", "tn3270.dia_limin",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_dia_limout,
        {  "Maximum DIA bytes/transmission allowed outbound", "tn3270.dia_limout",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_dia_nfs,
        {  "Number of subsets supported", "tn3270.dia_nfs",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_dia_diafs,
        {  "DIA function set identifier", "tn3270.dia_diafs",
            FT_UINT8, BASE_HEX, VALS(vals_dia), 0x0,
            NULL, HFILL }},
    { &hf_tn3270_dia_diafn,
        {  "DIA function set number", "tn3270.dia_diafn",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    /* END - 6.20 - Query Reply (Document Interchange Architecture) */

    /* 6.22 - Query Reply (Field Outlining) */
    { &hf_tn3270_fo_flags,
        {  "Flags", "tn3270.fo_flags",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_fo_vpos,
        {  "Location of vertical line", "tn3270.fo_vpos",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_fo_hpos,
        {  "Location of overline/underline", "tn3270.fo_hpos",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_fo_hpos0,
        {  "Location of overline in case of separation", "tn3270.fo_hpos0",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_fo_hpos1,
        {  "Location of underline in case of separation", "tn3270.fo_hpos1",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    /* END - 6.22 - Query Reply (Field Outlining) */

    /* 6.25 - Query Reply (Format Storage Auxiliary Device) */
    { &hf_tn3270_fsad_flags,
        {  "Flags", "tn3270.fsad_flags",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_fsad_limin,
        {  "Reserved for LIMIN parameter. Must be set to zeros.", "tn3270.fsad_limin",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_fsad_limout,
        {  "Maximum bytes of format storage data per transmission allowed outbound.", "tn3270.fsad_limout",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_fsad_size,
        {  "Size of the format storage space", "tn3270.fsad_size",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    /* END - 6.25 - Query Reply (Format Storage Auxiliary Device) */

    /* 6.28 - Query Reply (Highlighting) */
    { &hf_tn3270_h_np,
        {  "Number of attribute-value/action pairs", "tn3270.h_np",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_h_vi,
        {  "Data stream attribute value accepted", "tn3270.h_vi",
            FT_UINT8, BASE_HEX, VALS(vals_extended_highlighting), 0x0,
            NULL, HFILL }},
    { &hf_tn3270_h_ai,
        {  "Data stream action", "tn3270.h_ai",
            FT_UINT8, BASE_HEX, VALS(vals_extended_highlighting), 0x0,
            NULL, HFILL }},
    /* END - Query Reply (Highlighting) */

    /* 6.29 - Query Reply (IBM Auxiliary Device) */
    { &hf_tn3270_ibm_flags,
        {  "Flags", "tn3270.ibm_flags",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_ibm_limin,
        {  "Inbound message size limit", "tn3270.ibm_limin",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_ibm_limout,
        {  "Outbound message size limit", "tn3270.ibm_limout",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_ibm_type,
        {  "Type of IBM Auxiliary Device", "tn3270.ibm_type",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    /* END - 6.29 - Query Reply (IBM Auxiliary Device) */

    /* 6.31 - Query Reply (Implicit Partitions) */
    { &hf_tn3270_ip_flags,
        {  "Flags (Reserved)", "tn3270.ip_flags",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_ipdd_length,
        {  "Length of this self-defining parameter", "tn3270.ipdd_length",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_ip_id,
        {  "Identifies this self-defining parameter as Implicit Partition Sizes", "tn3270.ip_id",
            FT_UINT8, BASE_HEX, VALS(vals_ip), 0x0,
            NULL, HFILL }},
    { &hf_tn3270_ipdd_wd,
        {  "Width of the Implicit Partition default screen siz (in character cells)", "tn3270.ipdd_wd",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_ipdd_hd,
        {  "Height of the Implicit Partition default screen size", "tn3270.ipdd_hd",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_ipdd_wa,
        {  "Width of the Implicit Partition alternate screen size", "tn3270.ipdd_wa",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_ipdd_ha,
        {  "Height of the Implicit Partition alternate screen size", "tn3270.ipdd_ha",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_ippd_dpbs,
        {  " Default printer buffer size (in character cells)", "tn3270.ippd_dpbs",
            FT_UINT64, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_ippd_apbs,
        {  " Default printer buffer size (in character cells)", "tn3270.ippd_apbs",
            FT_UINT64, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_ipccd_wcd,
        {  "Width of the character cell for the Implicit Partition default screen size", "tn3270.ipccd_wcd",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_ipccd_hcd,
        {  "Height of the character cell for the Implicit Partition default screen size", "tn3270.ipccd_hcd",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_ipccd_wca,
        {  "Width of the character cell for the Implicit Partition alternate screen size", "tn3270.ipccd_wca",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_ipccd_hca,
        {  "Height of the character cell for the Implicit Partition alternate screen size", "tn3270.ipccd_hca",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    /* END - Query Reply (Implicit Partitions) */

    /* 6.32 - Query Reply (IOCA Auxiliary Device) */
    { &hf_tn3270_ioca_limin,
        {  "Max IOCA bytes/inbound transmission", "tn3270.ioca_limin",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_ioca_limout,
        {  "Max IOCA bytes/outbound transmission", "tn3270.ioca_limout",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_ioca_type,
        {  "Type of IOCA Auxiliary Device", "tn3270.ioca_type",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    /* END - 6.32 - Query Reply (IOCA Auxiliary Device) */

    /* 6.34 - Query Reply (MSR Control) */
    { &hf_tn3270_msr_nd,
        {  "Number of MSR device types", "tn3270.msr_nd",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    /* END - 6.34 - Query Reply (MSR Control) */

    /* 6.36 - Query Reply (OEM Auxiliary Device) */
    { &hf_tn3270_oem_dsref,
        {  "Data stream reference identifier", "tn3270.oem_dsref",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_oem_dtype,
        {  "Device type", "tn3270.oem_dtype",
            FT_EBCDIC, BASE_NONE, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_oem_uname,
        {  "User assigned name", "tn3270.oem_uname",
            FT_EBCDIC, BASE_NONE, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_sdp_daid,
        {  "Destination/Origin ID", "tn3270.oem_sdp_daid_doid",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_oem_sdp_ll_limin,
        {  "Maximum OEM dsf bytes/transmission allowed inbound", "tn3270.oem_sdp_ll_limin",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_oem_sdp_ll_limout,
        {  "Maximum OEM dsf bytes/transmission allowed outbound", "tn3270.oem_sdp_ll_limout",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_oem_sdp_pclk_vers,
        {  "Protocol version", "tn3270.oem_sdp_pclk_vers",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    /* END - 6.36 - Query Reply (OEM Auxiliary Device) */

    /* 6.37 - Query Reply (Paper Feed Techniques) */
    { &hf_tn3270_pft_flags,
        {  "Flags", "tn3270.pft_flags",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_pft_tmo,
        {  "Top margin offset in 1/1440ths of an inch", "tn3270.pft_tmo",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_pft_bmo,
        {  "Bottom margin offset in 1/1440ths of an inch", "tn3270.pft_bmo",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    /* END - 6.37 - Query Reply (Paper Feed Techniques) */

    /* 6.38 - Query Reply (Partition Characteristics) */
    { &hf_tn3270_pc_vo_thickness,
        {  "Thickness", "tn3270.pc_vo_thickness",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    /* END- 6.38 - Query Reply (Partition Characteristics) */

    /* 6.41 - Query Reply (Product Defined Data Stream) */
    { &hf_tn3270_pdds_refid,
        {  "Reference identifier", "tn3270.pdds_refid",
            FT_UINT8, BASE_HEX, VALS(vals_pdds_refid), 0x0,
            NULL, HFILL }},
    { &hf_tn3270_pdds_ssid,
        {  "Subset identifier", "tn3270.pdds_ssid",
            FT_UINT8, BASE_HEX, VALS(vals_pdds_ssid), 0x0,
            NULL, HFILL }},
    /* END - 6.41 - Query Reply (Product Defined Data Stream) */

    /* 6.43 - Query Reply (RPQ Names) */
    { &hf_tn3270_rpq_device,
        {  "Device type identifier", "tn3270.rpq_device",
            FT_EBCDIC, BASE_NONE, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_rpq_mid,
        {  "Model type identifier", "tn3270.rpq_mid",
            FT_UINT64, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_rpq_rpql,
        {  "Length of RPQ name (including this byte)", "tn3270.rpq_rpql",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_rpq_name,
        {  "RPQ name", "tn3270.rpq_name",
            FT_EBCDIC, BASE_NONE, NULL, 0x0,
            NULL, HFILL }},
    /* END - Query Reply (Names) */

    /* 6.44 - Query Reply (Save or Restore Format) */
    { &hf_tn3270_srf_fpcbl,
        {  "Format parameter control block length", "tn3270.srf_fpcbl",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    /* END - 6.44 - Query Reply (Save or Restore Format) */

    /* 6.45 - Query Reply (Settable Printer Characteristics) */
    { &hf_tn3270_spc_epc_flags,
        {  "Flags", "tn3270.spc_epc_flags",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    /* END - 6.45 - Query Reply (Settable Printer Characteristics) */

    /* 6.47 - Query Reply (Storage Pools) */
    { &hf_tn3270_sp_spid,
        {  "Storage pool identity", "tn3270.sp_spid",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_sp_size,
        {  "Size of this storage pool when empty", "tn3270.sp_size",
            FT_UINT32, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_sp_space,
        {  "Space available in this storage pool", "tn3270.sp_space",
            FT_UINT32, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_sp_objlist,
        {  "Identifiers of objects housed in this storage pool", "tn3270.sp_objlist",
            FT_UINT16, BASE_HEX, VALS(vals_objlist), 0x0,
            NULL, HFILL }},
    /* END - 6.47 - Query Reply (Storage Pools) */

    /* 6.49 - Query Reply (Text Partitions) */
    { &hf_tn3270_tp_nt,
        {  "Maximum number of text partitions", "tn3270.tp_nt",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_tp_m,
        {  "Maximum partition size", "tn3270.tp_m",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_tp_flags,
        {  "Flags", "tn3270.tp_flags",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_tp_ntt,
        {  "Number of text types supported", "tn3270.tp_ntt",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_tp_tlist,
        {  "List of types supported", "tn3270.tp_tlist",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    /* END - 6.49 - Query Reply (Text Partitions) */

    /* 6.50 - Query Reply (Transparency) */
    { &hf_tn3270_t_np,
        {  " Number of pairs", "tn3270.t_np",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_t_vi,
        {  "Data stream attribute value accepted", "tn3270.t_vi",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_t_ai,
        {  "Associated action value", "tn3270.t_ai",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    /* END - 6.50 - Query Reply (Transparency) */

    /* 6.51 Query Reply Usable Area */
    { &hf_tn3270_usable_area_flags1,
      {"Usable Area Flags", "tn3270.query_reply_usable_area_flags1", FT_UINT8, BASE_HEX,
        NULL, 0, NULL, HFILL}},
    { &hf_tn3270_ua_reserved1,
        { "Reserved",
            "tn3270.reserved", FT_BOOLEAN, 8, NULL, UA_RESERVED1, NULL, HFILL }},
    { &hf_tn3270_ua_page_printer,
        { "Page Printer",
            "tn3270.ua_page_printer", FT_BOOLEAN, 8, NULL, PAGE_PRINTER, NULL, HFILL }},
    { &hf_tn3270_ua_reserved2,
        { "Reserved",
            "tn3270.reserved", FT_BOOLEAN, 8, NULL, UA_RESERVED2, NULL, HFILL }},
    { &hf_tn3270_ua_hard_copy,
        { "Hard Copy",
            "tn3270.ua_hard_copy", FT_BOOLEAN, 8, NULL, HARD_COPY, NULL, HFILL }},
    { &hf_tn3270_ua_addressing,
      {"Usable Area Addressing", "tn3270.ua_addressing", FT_UINT8, BASE_HEX,
        VALS (vals_usable_area_flags1), UNMAPPED, NULL, HFILL}},
    { &hf_tn3270_usable_area_flags2,
      {"Usable Area Flags", "tn3270.query_reply_usable_area_flags2", FT_UINT8, BASE_HEX,
        NULL, 0x30, NULL, HFILL}},
    { &hf_tn3270_ua_variable_cells,
        { "Variable Cells",
            "tn3270.ua_variable_cells", FT_BOOLEAN, 8, NULL, VARIABLE_CELLS, NULL, HFILL }},
    { &hf_tn3270_ua_characters,
        { "Variable Characters",
            "tn3270.ua_characters", FT_BOOLEAN, 8, NULL, CHARACTERS, NULL, HFILL }},
    { &hf_tn3270_ua_cell_units,
        { "Cell Units",
            "tn3270.ua_cell_units", FT_BOOLEAN, 8, NULL, CELL_UNITS, NULL, HFILL }},
    { &hf_tn3270_ua_width_cells_pels,
        {  "Width of usable area in cells/pels", "tn3270.ua_width_cells_pels",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_ua_height_cells_pels,
        {  "Height of usable area in cells/pels", "tn3270.ua_height_cells_pels",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_ua_uom_cells_pels,
        {  "Units of measure for cells/pels", "tn3270.ua_uom_cells_pels",
            FT_UINT8, BASE_HEX, VALS(vals_usable_area_uom), 0x0,
            NULL, HFILL }},
    { &hf_tn3270_ua_xr,
        {  "Distance between points in X direction as a fraction, measured in UNITS, with 2-byte "
           "numerator and 2-byte denominator", "tn3270.ua_xr",
            FT_UINT64, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_ua_yr,
        {  "Distance between points in Y direction as a fraction, measured in UNITS, with 2-byte "
           "numerator and 2-byte denominator", "tn3270.ua_xr",
            FT_UINT64, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_ua_aw,
        {  "Number of X units in default cell", "tn3270.ua_aw",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_ua_ah,
        {  "Number of Y units in default cell", "tn3270.ua_ah",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_ua_buffsz,
        {  "Character buffer size (bytes)", "tn3270.ua_buffsz",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            "Character buffer size (bytes)", HFILL }},
    { &hf_tn3270_ua_xmin,
        {  " Minimum number of X units in variable cell", "tn3270.ua_xmin",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_ua_ymin,
        {  " Minimum number of Y units in variable cell", "tn3270.ua_ymin",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_ua_xmax,
        {  " Maximum number of X units in variable cell", "tn3270.ua_xmax",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_ua_ymax,
        {  " Maximum number of Y units in variable cell", "tn3270.ua_ymax",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    /* End - 6.51 Query Reply Usable Area */

    /* 6.52 - Query Reply (3270 IPDS) */
    { &hf_tn3270_3270_tranlim,
        {  "Maximum transmission size allowed outbound", "tn3270.3270_tranlim",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    /* END - 6.52 - Query Reply (3270 IPDS) */

    /* Miscellaneous */
    { &hf_tn3270_double_byte_sf_id,
        {  "Structured Field", "tn3270.double_byte_sf_id",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_field_data,
        {  "Field Data", "tn3270.field_data",
            FT_EBCDIC, BASE_NONE, NULL, 0x0,
            "tn3270.field_data", HFILL }},
    { &hf_tn3270_number_of_attributes,
        {  "Number of Attributes", "tn3270.number_of_attributes",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_resbyte,
        {  "Flags (Reserved)", "tn3270.resbyte",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_resbytes,
        {  "Flags (Reserved)", "tn3270.resbytes",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_res_twobytes,
        {  "Flags (Reserved)", "tn3270.res_twobytes",
            FT_UINT16, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_sf_outbound_id,
        {  "Structured Field", "tn3270.sf_outbound_id",
            FT_UINT8, BASE_HEX, VALS(vals_outbound_structured_fields), 0x0,
            NULL, HFILL }},
    { &hf_tn3270_sf_inbound_id,
        {  "Structured Field", "tn3270.sf_inbound_id",
            FT_UINT8, BASE_HEX, VALS(vals_inbound_structured_fields), 0x0,
            NULL, HFILL }},
    { &hf_tn3270_sf_inbound_outbound_id,
        {  "Structured Field", "tn3270.sf_inbound_outbound_id",
            FT_UINT8, BASE_HEX, VALS(vals_outbound_inbound_structured_fields), 0x0,
            NULL, HFILL }},
    { &hf_tn3270_sf_query_replies,
        {  "Query Reply", "tn3270.sf_query_reply",
            FT_UINT8, BASE_HEX, VALS(vals_query_replies), 0x0,
            NULL, HFILL }},
    { &hf_tn3270_null,
        {  "Trailing Null (Possible Mainframe/Emulator Bug)", "tn3270.null",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_unknown_data,
        {  "Unknown Data (Possible Mainframe/Emulator Bug)", "tn3270.unknown_data",
            FT_EBCDIC, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},

    /*TN3270E - Header Fields */
    { &hf_tn3270_tn3270e_data_type,
        {  "TN3270E Data Type", "tn3270.tn3270e_data_type",
            FT_UINT8, BASE_HEX, VALS(vals_tn3270_header_data_types), 0x0,
            NULL, HFILL }},
    { &hf_tn3270_tn3270e_request_flag,
        {  "TN3270E Request Flag", "tn3270.tn3270e_request_flag",
            FT_UINT8, BASE_HEX, VALS(vals_tn3270_header_request_flags), 0x0,
            NULL, HFILL }},
    { &hf_tn3270_tn3270e_response_flag_response,
        {  "TN3270E Response Flag", "tn3270.tn3270e_response_flag",
            FT_UINT8, BASE_HEX, VALS(vals_tn3270_header_response_flags_response), 0x0,
            NULL, HFILL }},
    { &hf_tn3270_tn3270e_response_flag_3270_SCS,
        {  "TN3270E Response Flag", "tn3270.tn3270e_response_flag",
            FT_UINT8, BASE_HEX, VALS(vals_tn3270_header_response_flags_3270_SCS), 0x0,
            NULL, HFILL }},
    { &hf_tn3270_tn3270e_seq_number,
        {  "TN3270E Seq Number", "tn3270.tn3270e_seq_number",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},
    { &hf_tn3270_tn3270e_header_data,
        {  "TN3270E Header Data", "tn3270..tn3270e_header_data",
            FT_EBCDIC, BASE_NONE, NULL, 0x0,
            NULL, HFILL }}
  };

  static gint *ett[] = {
    &ett_tn3270,
    &ett_sf,
    &ett_tn3270_field_attribute,
    &ett_tn3270_field_validation,
    &ett_tn3270_usable_area_flags1,
    &ett_tn3270_usable_area_flags2,
    &ett_tn3270_query_reply_alphanumeric_flags,
    &ett_tn3270_character_sets_flags1,
    &ett_tn3270_character_sets_flags2,
    &ett_tn3270_character_sets_form,
    &ett_tn3270_cs_descriptor_flags,
    &ett_tn3270_color_flags,
    &ett_tn3270_dc_dir_flags,
    &ett_tn3270_wcc,
    &ett_tn3270_ccc,
    &ett_tn3270_msr_state_mask
  };

  proto_tn3270 = proto_register_protocol("TN3270 Protocol", "TN3270", "tn3270");
  register_dissector("tn3270", dissect_tn3270, proto_tn3270);
  proto_register_field_array(proto_tn3270, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));

}

void
proto_reg_handoff_tn3270(void)
{
  dissector_handle_t tn3270_handle;
  tn3270_handle = find_dissector("tn3270");
  dissector_add_handle("tcp.port", tn3270_handle); /* for "decode-as" */
}

