/* plugin_api_list.c
 * Used to generate various included files for plugin API
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

#include <epan/packet.h>
#include <epan/conversation.h>
#include <epan/filesystem.h>
#include <epan/report_err.h>
#include <epan/except.h>
#include <epan/prefs.h>
#include <epan/reassemble.h>
#include <epan/dissectors/packet-dcerpc.h>
#include <epan/dissectors/packet-giop.h>
#include <epan/dissectors/packet-per.h>
#include <epan/dissectors/packet-ber.h>
#include <epan/dissectors/packet-tpkt.h>
#include <epan/dissectors/packet-tcp.h>
#include <epan/dissectors/packet-rpc.h>
#include <epan/dissectors/packet-rtp.h>
#include <epan/dissectors/packet-rtcp.h>
#include <epan/tap.h>
#include <epan/asn1.h>
#include <epan/xdlc.h>
#include <epan/crc16.h>

gint check_col(column_info*, gint);
void col_clear(column_info*, gint);
void col_add_fstr(column_info*, gint, const gchar*, ...);
void col_append_fstr(column_info*, gint, const gchar*, ...);
void col_prepend_fstr(column_info*, gint, const gchar*, ...);
void col_add_str(column_info*, gint, const gchar*);
void col_append_str(column_info*, gint, const gchar*);
void col_set_str(column_info*, gint, const gchar*);

void register_init_routine(void (*func)(void));
void register_postseq_cleanup_routine(void (*func)(void));

gchar* match_strval(guint32, const value_string*);
gchar* val_to_str(guint32, const value_string *, const char *);

conversation_t *conversation_new(guint32, address *, address *,
    port_type, guint32, guint32, guint);
conversation_t *find_conversation(guint32, address *, address *,
    port_type, guint32, guint32, guint);
void conversation_set_dissector(conversation_t *,
    dissector_handle_t);

int proto_register_protocol(char*, char*, char*);
void proto_register_field_array(int, hf_register_info*, int);
void proto_register_subtree_array(int *const *, int);

void dissector_add(const char *, guint32, dissector_handle_t);
void dissector_delete(const char *, guint32,
    dissector_handle_t);
void dissector_add_handle(const char *,
    dissector_handle_t);

void heur_dissector_add(const char *, heur_dissector_t, int);

void register_dissector(const char *, dissector_t, int);
dissector_handle_t find_dissector(const char *);
dissector_handle_t create_dissector_handle(dissector_t dissector,
    int proto);
int call_dissector(dissector_handle_t, tvbuff_t *,
    packet_info *, proto_tree *);

void tcp_dissect_pdus(tvbuff_t *, packet_info *, proto_tree *,
    gboolean, guint, guint (*)(tvbuff_t *, int),
    void (*)(tvbuff_t *, packet_info *, proto_tree *));

gboolean proto_is_protocol_enabled(protocol_t *);

int proto_item_get_len(proto_item*);
void proto_item_set_len(proto_item*, gint);
void proto_item_set_text(proto_item*, const char*, ...);
void proto_item_append_text(proto_item*, const char*, ...);
proto_tree* proto_item_add_subtree(proto_item*, gint);
proto_item* proto_tree_add_item(proto_tree*, int, tvbuff_t*, gint, gint, gboolean);
proto_item* proto_tree_add_item_hidden(proto_tree*, int, tvbuff_t*, gint, gint, gboolean);
proto_item* proto_tree_add_protocol_format(proto_tree*, int, tvbuff_t*, gint, gint, const char*, ...);

proto_item* proto_tree_add_bytes(proto_tree*, int, tvbuff_t*, gint, gint, const guint8*);
proto_item* proto_tree_add_bytes_hidden(proto_tree*, int, tvbuff_t*, gint, gint, const guint8*);
proto_item* proto_tree_add_bytes_format(proto_tree*, int, tvbuff_t*, gint, gint, const guint8*, const char*, ...);

proto_item* proto_tree_add_time(proto_tree*, int, tvbuff_t*, gint, gint, nstime_t*);
proto_item* proto_tree_add_time_hidden(proto_tree*, int, tvbuff_t*, gint, gint, nstime_t*);
proto_item* proto_tree_add_time_format(proto_tree*, int, tvbuff_t*, gint, gint, nstime_t*, const char*, ...);

proto_item* proto_tree_add_ipxnet(proto_tree*, int, tvbuff_t*, gint, gint, guint32);
proto_item* proto_tree_add_ipxnet_hidden(proto_tree*, int, tvbuff_t*, gint, gint, guint32);
proto_item* proto_tree_add_ipxnet_format(proto_tree*, int, tvbuff_t*, gint, gint, guint32, const char*, ...);

proto_item* proto_tree_add_ipv4(proto_tree*, int, tvbuff_t*, gint, gint, guint32);
proto_item* proto_tree_add_ipv4_hidden(proto_tree*, int, tvbuff_t*, gint, gint, guint32);
proto_item* proto_tree_add_ipv4_format(proto_tree*, int, tvbuff_t*, gint, gint, guint32, const char*, ...);

proto_item* proto_tree_add_ipv6(proto_tree*, int, tvbuff_t*, gint, gint, const guint8*);
proto_item* proto_tree_add_ipv6_hidden(proto_tree*, int, tvbuff_t*, gint, gint, const guint8*);
proto_item* proto_tree_add_ipv6_format(proto_tree*, int, tvbuff_t*, gint, gint, const guint8*, const char*, ...);

proto_item* proto_tree_add_ether(proto_tree*, int, tvbuff_t*, gint, gint, const guint8*);
proto_item* proto_tree_add_ether_hidden(proto_tree*, int, tvbuff_t*, gint, gint, const guint8*);
proto_item* proto_tree_add_ether_format(proto_tree*, int, tvbuff_t*, gint, gint, const guint8*, const char*, ...);

proto_item* proto_tree_add_string(proto_tree*, int, tvbuff_t*, gint, gint, const char*);
proto_item* proto_tree_add_string_hidden(proto_tree*, int, tvbuff_t*, gint, gint, const char*);
proto_item* proto_tree_add_string_format(proto_tree*, int, tvbuff_t*, gint, gint, const char*, const char*, ...);

proto_item* proto_tree_add_boolean(proto_tree*, int, tvbuff_t*, gint, gint, guint32);
proto_item* proto_tree_add_boolean_hidden(proto_tree*, int, tvbuff_t*, gint, gint, guint32);
proto_item* proto_tree_add_boolean_format(proto_tree*, int, tvbuff_t*, gint, gint, guint32, const char*, ...);

proto_item* proto_tree_add_double(proto_tree*, int, tvbuff_t*, gint, gint, double);
proto_item* proto_tree_add_double_hidden(proto_tree*, int, tvbuff_t*, gint, gint, double);
proto_item* proto_tree_add_double_format(proto_tree*, int, tvbuff_t*, gint, gint, double, const char*, ...);

proto_item* proto_tree_add_uint(proto_tree*, int, tvbuff_t*, gint, gint, guint32);
proto_item* proto_tree_add_uint_hidden(proto_tree*, int, tvbuff_t*, gint, gint, guint32);
proto_item* proto_tree_add_uint_format(proto_tree*, int, tvbuff_t*, gint, gint, guint32, const char*, ...);

proto_item* proto_tree_add_int(proto_tree*, int, tvbuff_t*, gint, gint, gint32);
proto_item* proto_tree_add_int_hidden(proto_tree*, int, tvbuff_t*, gint, gint, gint32);
proto_item* proto_tree_add_int_format(proto_tree*, int, tvbuff_t*, gint, gint, gint32, const char*, ...);

proto_item* proto_tree_add_text(proto_tree*, tvbuff_t*, gint, gint, const char*, ...);

tvbuff_t* tvb_new_subset(tvbuff_t*, gint, gint, gint);

void tvb_set_free_cb(tvbuff_t*, tvbuff_free_cb_t);
void tvb_set_child_real_data_tvbuff(tvbuff_t*, tvbuff_t*);
tvbuff_t* tvb_new_real_data(const guint8*, guint, gint);

guint tvb_length(tvbuff_t*);
gint tvb_length_remaining(tvbuff_t*, gint);
gboolean tvb_bytes_exist(tvbuff_t*, gint, gint);
gboolean tvb_offset_exists(tvbuff_t*, gint);
guint tvb_reported_length(tvbuff_t*);
gint tvb_reported_length_remaining(tvbuff_t*, gint);

guint8 tvb_get_guint8(tvbuff_t*, gint);

guint16 tvb_get_ntohs(tvbuff_t*, gint);
guint32 tvb_get_ntoh24(tvbuff_t*, gint);
guint32 tvb_get_ntohl(tvbuff_t*, gint);

guint16 tvb_get_letohs(tvbuff_t*, gint);
guint32 tvb_get_letoh24(tvbuff_t*, gint);
guint32 tvb_get_letohl(tvbuff_t*, gint);

guint8* tvb_memcpy(tvbuff_t*, guint8* target, gint, gint);
guint8* tvb_memdup(tvbuff_t*, gint, gint);

const guint8* tvb_get_ptr(tvbuff_t*, gint, gint);

gint tvb_find_guint8(tvbuff_t*, gint, gint, guint8);
gint tvb_pbrk_guint8(tvbuff_t *, gint, gint, guint8 *);

gint tvb_strnlen(tvbuff_t*, gint, guint);

gchar * tvb_format_text(tvbuff_t*, gint, gint);

gint tvb_get_nstringz(tvbuff_t*, gint, guint, guint8*);
gint tvb_get_nstringz0(tvbuff_t*, gint, guint, guint8*);

gint tvb_find_line_end(tvbuff_t*, gint, int, gint *, gboolean);
gint tvb_find_line_end_unquoted(tvbuff_t*, gint, int, gint *);

gint tvb_strneql(tvbuff_t*, gint, const gchar *, gint);
gint tvb_strncaseeql(tvbuff_t*, gint, const gchar *, gint);

gchar *tvb_bytes_to_str(tvbuff_t*, gint, gint len);

struct pref_module *prefs_register_protocol(int,
    void (*)(void));
void prefs_register_uint_preference(struct pref_module *,
    const char *, const char *, const char *, guint, guint *);
void prefs_register_bool_preference(struct pref_module *,
    const char *, const char *, const char *, gboolean *);
void prefs_register_enum_preference(struct pref_module *,
    const char *, const char *, const char *, gint *, const enum_val_t *,
    gboolean);
void prefs_register_string_preference(struct pref_module *,
    const char *, const char *, const char *, char**);

void register_giop_user(giop_sub_dissector_t *, gchar *, int);
gboolean is_big_endian(MessageHeader *);
guint32 get_CDR_encap_info(tvbuff_t *, proto_tree *, gint *,
		gboolean, guint32, gboolean *, guint32 *);
void get_CDR_any(tvbuff_t *, proto_tree *, gint *,
		gboolean, int, MessageHeader *);
gboolean get_CDR_boolean(tvbuff_t *, int *);
guint8 get_CDR_char(tvbuff_t *, int *);
gdouble get_CDR_double(tvbuff_t *, int *, gboolean, int);
guint32 get_CDR_enum(tvbuff_t *, int *, gboolean, int);
void get_CDR_fixed(tvbuff_t *, gchar **, gint *, guint32,
		gint32);
gfloat get_CDR_float(tvbuff_t *, int *, gboolean, int);
void get_CDR_interface(tvbuff_t *, packet_info *, proto_tree *,
		int *, gboolean, int);
gint32 get_CDR_long(tvbuff_t *, int *, gboolean, int);
void get_CDR_object(tvbuff_t *, packet_info *, proto_tree *,
		int *, gboolean, int);
guint8 get_CDR_octet(tvbuff_t *, int *);
void get_CDR_octet_seq(tvbuff_t *, gchar **, int *, guint32);
gint16 get_CDR_short(tvbuff_t *, int *, gboolean, int);
guint32 get_CDR_string(tvbuff_t *, gchar **, int *, gboolean,
		int);
guint32 get_CDR_typeCode(tvbuff_t *, proto_tree *, gint *,
	gboolean, int, MessageHeader *);
guint32 get_CDR_ulong(tvbuff_t *, int *, gboolean, int);
guint16 get_CDR_ushort(tvbuff_t *, int *, gboolean, int);
gint get_CDR_wchar(tvbuff_t *, gchar **, int *,
		MessageHeader *);
guint32 get_CDR_wstring(tvbuff_t *, gchar **, int *, gboolean,
		int, MessageHeader *);

int is_tpkt(tvbuff_t *, int);
void dissect_tpkt_encap(tvbuff_t *, packet_info *,
    proto_tree *, gboolean, dissector_handle_t);

void set_actual_length(tvbuff_t *, guint);

const char *decode_boolean_bitfield(guint32, guint32, int,
    const char *, const char *);
const char *decode_numeric_bitfield(guint32, guint32, int,
    const char *);
const char *decode_enumerated_bitfield(guint32, guint32, int,
    const value_string *, const char *);

dissector_table_t register_dissector_table(const char *, char *, ftenum_t ,int );
void except_throw(long, long, const char *);
gboolean dissector_try_port(dissector_table_t, guint32, tvbuff_t *, packet_info *, proto_tree *);

void conversation_add_proto_data(conversation_t *, int, void *);
void *conversation_get_proto_data(conversation_t *, int);
void conversation_delete_proto_data(conversation_t *, int);
void p_add_proto_data(frame_data *, int, void *);
void *p_get_proto_data(frame_data *, int);

gchar*	ip_to_str(const guint8 *);
char*	ip6_to_str(const struct e_in6_addr *);
gchar*	time_secs_to_str(guint32);
gchar*	time_msecs_to_str(guint32);
gchar*	abs_time_to_str(nstime_t*);

int proto_get_id_by_filter_name(gchar* filter_name);
char *proto_get_protocol_name(int n);
char *proto_get_protocol_short_name(protocol_t *);
char *proto_get_protocol_filter_name(int proto_id);

void prefs_register_obsolete_preference(module_t *, const char *);

void add_new_data_source(packet_info *, tvbuff_t *, char *);

void fragment_table_init(GHashTable **);
void reassembled_table_init(GHashTable **);
fragment_data *fragment_add(tvbuff_t *, int, packet_info *, guint32,
			GHashTable *, guint32, guint32, gboolean);
fragment_data *fragment_add_seq(tvbuff_t *, int, packet_info *, guint32,
			GHashTable *, guint32, guint32, gboolean);
fragment_data *fragment_add_seq_check(tvbuff_t *, int, packet_info *, guint32 id,
			GHashTable *, GHashTable *, guint32, guint32, gboolean);
fragment_data *fragment_add_seq_next(tvbuff_t *, int, packet_info *, guint32,
			GHashTable *, GHashTable *, guint32, gboolean);
fragment_data *fragment_get(packet_info *, guint32, GHashTable *);
void fragment_set_tot_len(packet_info *, guint32, GHashTable *, guint32);
guint32 fragment_get_tot_len(packet_info *, guint32, GHashTable *);
void fragment_set_partial_reassembly(packet_info *, guint32, GHashTable *);
unsigned char *fragment_delete(packet_info *, guint32, GHashTable *);
gboolean show_fragment_tree(fragment_data *, const fragment_items *, proto_tree *, packet_info *, tvbuff_t *);
gboolean show_fragment_seq_tree(fragment_data *, const fragment_items *, proto_tree *, packet_info *, tvbuff_t *);

int register_tap(char *);
void tap_queue_packet(int, packet_info *, const void *);

void asn1_open(ASN1_SCK *, tvbuff_t *, int );
void asn1_close(ASN1_SCK *, int *);
int asn1_octet_decode(ASN1_SCK *, guchar *);
int asn1_tag_decode(ASN1_SCK *, guint *);
int asn1_id_decode(ASN1_SCK *, guint *, guint *, guint *);
int asn1_length_decode(ASN1_SCK *, gboolean *, guint *);
int asn1_header_decode(ASN1_SCK *, guint *, guint *, guint *,
			gboolean *, guint *);
int asn1_eoc(ASN1_SCK *, int );
int asn1_eoc_decode(ASN1_SCK *, int );
int asn1_null_decode(ASN1_SCK *, int );
int asn1_bool_decode(ASN1_SCK *, int , gboolean *);
int asn1_int32_value_decode(ASN1_SCK *, int , gint32 *);
int asn1_int32_decode(ASN1_SCK *, gint32 *, guint *);
int asn1_uint32_value_decode(ASN1_SCK *, int , guint *);
int asn1_uint32_decode(ASN1_SCK *, guint32 *, guint *);
int asn1_bits_decode(ASN1_SCK *, int , guchar **,
                             guint *, guchar *);
int asn1_string_value_decode(ASN1_SCK *, int ,
			guchar **);
int asn1_string_decode(ASN1_SCK *, guchar **, guint *,
			guint *, guint );
int asn1_octet_string_decode(ASN1_SCK *, guchar **, guint *,
			guint *);
int asn1_subid_decode(ASN1_SCK *, subid_t *);
int asn1_oid_value_decode(ASN1_SCK *, int , subid_t **,
			guint *);
int asn1_oid_decode( ASN1_SCK *, subid_t **, guint *, guint *);
int asn1_sequence_decode( ASN1_SCK *, guint *, guint *);

char *asn1_err_to_str(int );

void proto_item_set_end(proto_item*, tvbuff_t *, gint);

proto_item* proto_tree_add_none_format(proto_tree*, int, tvbuff_t*, gint, gint, const char*, ...);

int except_init(void);
void except_deinit(void);
void except_rethrow(except_t *);
void except_throwd(long, long, const char *, void *);
void except_throwf(long, long, const char *, ...);
void (*except_unhandled_catcher(void (*)(except_t *)))(except_t *);
void *except_take_data(except_t *);
void except_set_allocator(void *(*)(size_t), void (*)(void *));
void *except_alloc(size_t);
void except_free(void *);
struct except_stacknode *except_pop(void);
void except_setup_try(struct except_stacknode *, struct except_catch *, const except_id_t [], size_t);

void col_set_fence(column_info*, gint);

guint8 *tvb_get_string(tvbuff_t *, gint, gint);
guint8 *tvb_get_stringz(tvbuff_t *, gint, gint *);

dissector_table_t find_dissector_table(const char *);
dissector_handle_t dissector_get_port_handle(dissector_table_t, guint32);
char *dissector_handle_get_short_name(dissector_handle_t);
int dissector_handle_get_protocol_index(dissector_handle_t);
void new_register_dissector(const char *, new_dissector_t, int);
dissector_handle_t new_create_dissector_handle(new_dissector_t, int);

void register_giop_user_module(giop_sub_dissector_t *sub, gchar *name, gchar *module, int sub_proto);

guint32 dissect_per_GeneralString(tvbuff_t*, guint32, packet_info*, proto_tree*, int);
guint32 dissect_per_sequence_of(tvbuff_t*, guint32, packet_info*, proto_tree*, int, gint, int (*)(tvbuff_t*, int, packet_info*, proto_tree*));
guint32 dissect_per_IA5String(tvbuff_t*, guint32, packet_info*, proto_tree*, int, int, int);
guint32 dissect_per_NumericString(tvbuff_t*, guint32, packet_info*, proto_tree*, int, int, int);
guint32 dissect_per_PrintableString(tvbuff_t*, guint32, packet_info*, proto_tree*, int, int, int);
guint32 dissect_per_BMPString(tvbuff_t*, guint32, packet_info*, proto_tree*, int, int, int);
guint32 dissect_per_constrained_sequence_of(tvbuff_t*, guint32, packet_info*, proto_tree*, int, gint, int (*)(tvbuff_t*, int, packet_info*, proto_tree*), int, int);
guint32 dissect_per_constrained_set_of(tvbuff_t*, guint32, packet_info*, proto_tree *parent_tree, int, gint, int (*)(tvbuff_t*, int, packet_info*, proto_tree*), int, int);
guint32 dissect_per_set_of(tvbuff_t*, guint32, packet_info*, proto_tree *parent_tree, int, gint, int (*)(tvbuff_t*, int, packet_info*, proto_tree*));
guint32 dissect_per_object_identifier(tvbuff_t*, guint32, packet_info*, proto_tree*, int, char*);
guint32 dissect_per_boolean(tvbuff_t*, guint32, packet_info *pinfo, proto_tree*, int, gboolean*, proto_item**);
guint32 dissect_per_integer(tvbuff_t*, guint32, packet_info*, proto_tree*, int, gint32*, proto_item**);
guint32 dissect_per_constrained_integer(tvbuff_t*, guint32, packet_info*, proto_tree*, int, guint32, guint32, guint32*, proto_item**, gboolean);
guint32 dissect_per_choice(tvbuff_t*, guint32, packet_info*, proto_tree*, int, gint, const per_choice_t*, char*, guint32*);
guint32 dissect_per_sequence(tvbuff_t*, guint32, packet_info*, proto_tree *parent_tree, int, gint, const per_sequence_t*);
guint32 dissect_per_octet_string(tvbuff_t*, guint32, packet_info*, proto_tree*, int, int, int, guint32*, guint32*);
guint32 dissect_per_restricted_character_string(tvbuff_t*, guint32, packet_info*, proto_tree*, int, int, int, char*, int, char *, guint32);

void dissector_add_string(const char*, gchar*, dissector_handle_t);
void dissector_delete_string(const char*, const gchar*,	dissector_handle_t);
void dissector_change_string(const char*, gchar *, dissector_handle_t);
void dissector_reset_string(const char*, const gchar*);
gboolean dissector_try_string(dissector_table_t, const gchar*, tvbuff_t*, packet_info*, proto_tree*);
dissector_handle_t dissector_get_string_handle(dissector_table_t, const gchar*);

char *get_datafile_path(const char *filename);
char *get_tempfile_path(const char *filename);

void register_heur_dissector_list(const char *name,
    heur_dissector_list_t *list);
gboolean dissector_try_heuristic(heur_dissector_list_t sub_dissectors,
    tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);

int asn1_id_decode1(ASN1_SCK *asn1, guint *tag);

gboolean col_get_writable(column_info *);
void col_set_writable(column_info *, gboolean);

const char *decode_enumerated_bitfield_shifted(guint32, guint32, int,
    const value_string *, const char *);

int dissect_xdlc_control(tvbuff_t *tvb, int offset, packet_info *pinfo,
  proto_tree *xdlc_tree, int hf_xdlc_control, gint ett_xdlc_control,
  const xdlc_cf_items *cf_items_nonext, const xdlc_cf_items *cf_items_ext,
  const value_string *u_modifier_short_vals_cmd,
  const value_string *u_modifier_short_vals_resp, int is_response,
  int is_extended, int append_info);

protocol_t *find_protocol_by_id(int n);

guint tvb_strsize(tvbuff_t *tvb, gint offset);

void report_open_failure(const char *filename, int err, gboolean for_writing);

void report_read_failure(const char *filename, int err);

guint32 dissect_per_bit_string(tvbuff_t*, guint32, packet_info*, proto_tree*, int, int, int);

int dissect_ber_identifier(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset, gint8 *class, gboolean *pc, gint32 *tag);
int dissect_ber_length(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset, guint32 *length, gboolean *ind);
int dissect_ber_integer(gboolean implicit_tag, packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset, gint hf_id, guint32 *value);
int dissect_ber_boolean(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset, gint hf_id);
int dissect_ber_choice(packet_info *pinfo, proto_tree *parent_tree, tvbuff_t *tvb, int offset, const ber_choice_t *ch, gint hf_id, gint ett_id);
int dissect_ber_GeneralizedTime(gboolean implicit_tag, packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset, gint hf_id);
int dissect_ber_sequence(gboolean implicit_tag, packet_info *pinfo, proto_tree *parent_tree, tvbuff_t *tvb, int offset, const ber_sequence_t *seq, gint hf_id, gint ett_id);
int dissect_ber_sequence_of(gboolean implicit_tag, packet_info *pinfo, proto_tree *parent_tree, tvbuff_t *tvb, int offset, const ber_sequence_t *seq, gint hf_id, gint ett_id);
int dissect_ber_set_of(gboolean implicit_tag, packet_info *pinfo, proto_tree *parent_tree, tvbuff_t *tvb, int offset, const ber_sequence_t *seq, gint hf_id, gint ett_id);
int dissect_ber_octet_string(gboolean implicit_tag, packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset, gint hf_id, tvbuff_t **tvb_out);
int dissect_ber_bitstring(gboolean implicit_tag, packet_info *pinfo, proto_tree *parent_tree, tvbuff_t *tvb, int offset, asn_namedbit const *named_bits, gint hf_id, gint ett_id, tvbuff_t **tvb_out);
int dissect_ber_restricted_string(gboolean implicit_tag, gint32 type, packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset, gint hf_id, tvbuff_t **tvb_out);
int dissect_ber_object_identifier(gboolean implicit_tag, packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset, gint hf_id, char *value_string);
int get_ber_identifier(tvbuff_t *tvb, int offset, gint8 *class, gboolean *pc, gint32 *tag);
int get_ber_length(tvbuff_t *tvb, int offset, guint32 *length, gboolean *ind);
proto_tree* proto_item_get_subtree(proto_item *ti);
proto_item* proto_tree_get_parent(proto_tree *tree);
proto_item* proto_item_get_parent(proto_item *ti);
proto_item* proto_item_get_parent_nth(proto_item *ti, int gen);
proto_item *get_ber_last_created_item(void);

void report_failure(const char *msg_format, ...);

void rpc_init_proc_table(guint prog, guint vers, const vsff *proc_table,
    int procedure_hf);
void rpc_init_prog(int proto, guint32 prog, int ett);
char *rpc_prog_name(guint32 prog);
char *rpc_proc_name(guint32 prog, guint32 vers, guint32 proc);
int rpc_prog_hf(guint32 prog, guint32 vers);

unsigned int rpc_roundup(unsigned int a);
int dissect_rpc_bool(tvbuff_t *tvb,
 proto_tree *tree, int hfindex, int offset);
int dissect_rpc_string(tvbuff_t *tvb,
 proto_tree *tree, int hfindex, int offset, char **string_buffer_ret);
int dissect_rpc_opaque_data(tvbuff_t *tvb, int offset,
    proto_tree *tree,
    packet_info *pinfo,
    int hfindex,
    gboolean fixed_length, guint32 length,
    gboolean string_data, char **string_buffer_ret,
    dissect_function_t *dissect_it);
int dissect_rpc_data(tvbuff_t *tvb,
 proto_tree *tree, int hfindex, int offset);
int dissect_rpc_bytes(tvbuff_t *tvb,
 proto_tree *tree, int hfindex, int offset, guint32 length,
 gboolean string_data, char **string_buffer_ret);
int dissect_rpc_list(tvbuff_t *tvb, packet_info *pinfo,
 proto_tree *tree, int offset, dissect_function_t *rpc_list_dissector);
int dissect_rpc_array(tvbuff_t *tvb, packet_info *pinfo,
 proto_tree *tree, int offset, dissect_function_t *rpc_array_dissector,
 int hfindex);
int dissect_rpc_uint32(tvbuff_t *tvb,
 proto_tree *tree, int hfindex, int offset);
int dissect_rpc_uint64(tvbuff_t *tvb,
 proto_tree *tree, int hfindex, int offset);

int dissect_rpc_indir_call(tvbuff_t *tvb, packet_info *pinfo,
 proto_tree *tree, int offset, int args_id, guint32 prog, guint32 vers,
 guint32 proc);
int dissect_rpc_indir_reply(tvbuff_t *tvb, packet_info *pinfo,
 proto_tree *tree, int offset, int result_id, int prog_id, int vers_id,
 int proc_id);
guint16 crc16_ccitt_tvb(tvbuff_t *tvb, unsigned int len);

guint64 tvb_get_letoh64(tvbuff_t *tvb, gint offset);
guint64 tvb_get_ntoh64(tvbuff_t *tvb, gint offset);

proto_item* proto_tree_add_float(proto_tree*, int, tvbuff_t*, gint, gint, float);
proto_item* proto_tree_add_float_hidden(proto_tree*, int, tvbuff_t*, gint, gint, float);
proto_item* proto_tree_add_float_format(proto_tree*, int, tvbuff_t*, gint, gint, float, const char*, ...);

gfloat tvb_get_ntohieee_float(tvbuff_t*, gint offset);
gdouble tvb_get_ntohieee_double(tvbuff_t*, gint offset);
gfloat tvb_get_letohieee_float(tvbuff_t*, gint offset);
gdouble tvb_get_letohieee_double(tvbuff_t*, gint offset);

proto_item *proto_tree_add_debug_text(proto_tree *tree, const char *format, ...);

void rtp_add_address(packet_info *pinfo, address *addr, int port, int other_port, gchar *setup_method, guint32 setup_frame_number);
void rtcp_add_address(packet_info *pinfo, address *addr, int port, int other_port, gchar *setup_method, guint32 setup_frame_number);

GString *register_tap_listener(char *, void *, char *, tap_reset_cb, tap_packet_cb, tap_draw_cb);
const char *get_datafile_dir(void);
char* proto_registrar_get_abbrev(int n);
header_field_info* proto_registrar_get_byname(const char *field_name);
double fvalue_get_floating(fvalue_t *fv);
char *fvalue_to_string_repr(fvalue_t *fv, ftrepr_t rtype, char *buf);
guint32 fvalue_get_integer(fvalue_t *fv);

int dissect_dcerpc_uint8 (tvbuff_t *tvb, gint offset, packet_info *pinfo,
                          proto_tree *tree, guint8 *drep,
                          int hfindex, guint8 *pdata);
int dissect_dcerpc_uint16 (tvbuff_t *tvb, gint offset, packet_info *pinfo,
                           proto_tree *tree, guint8 *drep,
                           int hfindex, guint16 *pdata);
int dissect_dcerpc_uint32 (tvbuff_t *tvb, gint offset, packet_info *pinfo,
                           proto_tree *tree, guint8 *drep,
                           int hfindex, guint32 *pdata);
int dissect_dcerpc_uuid_t (tvbuff_t *tvb, gint offset, packet_info *pinfo,
                           proto_tree *tree, char *drep,
                           int hfindex, e_uuid_t *pdata);
void dcerpc_init_uuid (int proto, int ett, e_uuid_t *uuid, guint16 ver, dcerpc_sub_dissector *procs, int opnum_hf);
int dissect_ndr_uint32 (tvbuff_t *tvb, gint offset, packet_info *pinfo,
                        proto_tree *tree, guint8 *drep,
                        int hfindex, guint32 *pdata);
