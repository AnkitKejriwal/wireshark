patable.p_check_col = check_col;
patable.p_col_clear = col_clear;
patable.p_col_add_fstr = col_add_fstr;
patable.p_col_append_fstr = col_append_fstr;
patable.p_col_prepend_fstr = col_prepend_fstr;
patable.p_col_add_str = col_add_str;
patable.p_col_append_str = col_append_str;
patable.p_col_set_str = col_set_str;
patable.p_register_init_routine = register_init_routine;
patable.p_register_postseq_cleanup_routine = register_postseq_cleanup_routine;
patable.p_match_strval = match_strval;
patable.p_val_to_str = val_to_str;
patable.p_conversation_new = conversation_new;
patable.p_find_conversation = find_conversation;
patable.p_conversation_set_dissector = conversation_set_dissector;
patable.p_proto_register_protocol = proto_register_protocol;
patable.p_proto_register_field_array = proto_register_field_array;
patable.p_proto_register_subtree_array = proto_register_subtree_array;
patable.p_dissector_add = dissector_add;
patable.p_dissector_delete = dissector_delete;
patable.p_dissector_add_handle = dissector_add_handle;
patable.p_heur_dissector_add = heur_dissector_add;
patable.p_register_dissector = register_dissector;
patable.p_find_dissector = find_dissector;
patable.p_create_dissector_handle = create_dissector_handle;
patable.p_call_dissector = call_dissector;
patable.p_tcp_dissect_pdus = tcp_dissect_pdus;
patable.p_proto_is_protocol_enabled = proto_is_protocol_enabled;
patable.p_proto_item_get_len = proto_item_get_len;
patable.p_proto_item_set_len = proto_item_set_len;
patable.p_proto_item_set_text = proto_item_set_text;
patable.p_proto_item_append_text = proto_item_append_text;
patable.p_proto_item_add_subtree = proto_item_add_subtree;
patable.p_proto_tree_add_item = proto_tree_add_item;
patable.p_proto_tree_add_item_hidden = proto_tree_add_item_hidden;
patable.p_proto_tree_add_protocol_format = proto_tree_add_protocol_format;
patable.p_proto_tree_add_bytes = proto_tree_add_bytes;
patable.p_proto_tree_add_bytes_hidden = proto_tree_add_bytes_hidden;
patable.p_proto_tree_add_bytes_format = proto_tree_add_bytes_format;
patable.p_proto_tree_add_time = proto_tree_add_time;
patable.p_proto_tree_add_time_hidden = proto_tree_add_time_hidden;
patable.p_proto_tree_add_time_format = proto_tree_add_time_format;
patable.p_proto_tree_add_ipxnet = proto_tree_add_ipxnet;
patable.p_proto_tree_add_ipxnet_hidden = proto_tree_add_ipxnet_hidden;
patable.p_proto_tree_add_ipxnet_format = proto_tree_add_ipxnet_format;
patable.p_proto_tree_add_ipv4 = proto_tree_add_ipv4;
patable.p_proto_tree_add_ipv4_hidden = proto_tree_add_ipv4_hidden;
patable.p_proto_tree_add_ipv4_format = proto_tree_add_ipv4_format;
patable.p_proto_tree_add_ipv6 = proto_tree_add_ipv6;
patable.p_proto_tree_add_ipv6_hidden = proto_tree_add_ipv6_hidden;
patable.p_proto_tree_add_ipv6_format = proto_tree_add_ipv6_format;
patable.p_proto_tree_add_ether = proto_tree_add_ether;
patable.p_proto_tree_add_ether_hidden = proto_tree_add_ether_hidden;
patable.p_proto_tree_add_ether_format = proto_tree_add_ether_format;
patable.p_proto_tree_add_string = proto_tree_add_string;
patable.p_proto_tree_add_string_hidden = proto_tree_add_string_hidden;
patable.p_proto_tree_add_string_format = proto_tree_add_string_format;
patable.p_proto_tree_add_boolean = proto_tree_add_boolean;
patable.p_proto_tree_add_boolean_hidden = proto_tree_add_boolean_hidden;
patable.p_proto_tree_add_boolean_format = proto_tree_add_boolean_format;
patable.p_proto_tree_add_double = proto_tree_add_double;
patable.p_proto_tree_add_double_hidden = proto_tree_add_double_hidden;
patable.p_proto_tree_add_double_format = proto_tree_add_double_format;
patable.p_proto_tree_add_uint = proto_tree_add_uint;
patable.p_proto_tree_add_uint_hidden = proto_tree_add_uint_hidden;
patable.p_proto_tree_add_uint_format = proto_tree_add_uint_format;
patable.p_proto_tree_add_int = proto_tree_add_int;
patable.p_proto_tree_add_int_hidden = proto_tree_add_int_hidden;
patable.p_proto_tree_add_int_format = proto_tree_add_int_format;
patable.p_proto_tree_add_text = proto_tree_add_text;
patable.p_tvb_new_subset = tvb_new_subset;
patable.p_tvb_set_free_cb = tvb_set_free_cb;
patable.p_tvb_set_child_real_data_tvbuff = tvb_set_child_real_data_tvbuff;
patable.p_tvb_new_real_data = tvb_new_real_data;
patable.p_tvb_length = tvb_length;
patable.p_tvb_length_remaining = tvb_length_remaining;
patable.p_tvb_bytes_exist = tvb_bytes_exist;
patable.p_tvb_offset_exists = tvb_offset_exists;
patable.p_tvb_reported_length = tvb_reported_length;
patable.p_tvb_reported_length_remaining = tvb_reported_length_remaining;
patable.p_tvb_get_guint8 = tvb_get_guint8;
patable.p_tvb_get_ntohs = tvb_get_ntohs;
patable.p_tvb_get_ntoh24 = tvb_get_ntoh24;
patable.p_tvb_get_ntohl = tvb_get_ntohl;
patable.p_tvb_get_letohs = tvb_get_letohs;
patable.p_tvb_get_letoh24 = tvb_get_letoh24;
patable.p_tvb_get_letohl = tvb_get_letohl;
patable.p_tvb_memcpy = tvb_memcpy;
patable.p_tvb_memdup = tvb_memdup;
patable.p_tvb_get_ptr = tvb_get_ptr;
patable.p_tvb_find_guint8 = tvb_find_guint8;
patable.p_tvb_pbrk_guint8 = tvb_pbrk_guint8;
patable.p_tvb_strnlen = tvb_strnlen;
patable.p_tvb_format_text = tvb_format_text;
patable.p_tvb_get_nstringz = tvb_get_nstringz;
patable.p_tvb_get_nstringz0 = tvb_get_nstringz0;
patable.p_tvb_find_line_end = tvb_find_line_end;
patable.p_tvb_find_line_end_unquoted = tvb_find_line_end_unquoted;
patable.p_tvb_strneql = tvb_strneql;
patable.p_tvb_strncaseeql = tvb_strncaseeql;
patable.p_tvb_bytes_to_str = tvb_bytes_to_str;
patable.p_prefs_register_protocol = prefs_register_protocol;
patable.p_prefs_register_uint_preference = prefs_register_uint_preference;
patable.p_prefs_register_bool_preference = prefs_register_bool_preference;
patable.p_prefs_register_enum_preference = prefs_register_enum_preference;
patable.p_prefs_register_string_preference = prefs_register_string_preference;
patable.p_register_giop_user = register_giop_user;
patable.p_is_big_endian = is_big_endian;
patable.p_get_CDR_encap_info = get_CDR_encap_info;
patable.p_get_CDR_any = get_CDR_any;
patable.p_get_CDR_boolean = get_CDR_boolean;
patable.p_get_CDR_char = get_CDR_char;
patable.p_get_CDR_double = get_CDR_double;
patable.p_get_CDR_enum = get_CDR_enum;
patable.p_get_CDR_fixed = get_CDR_fixed;
patable.p_get_CDR_float = get_CDR_float;
patable.p_get_CDR_interface = get_CDR_interface;
patable.p_get_CDR_long = get_CDR_long;
patable.p_get_CDR_object = get_CDR_object;
patable.p_get_CDR_octet = get_CDR_octet;
patable.p_get_CDR_octet_seq = get_CDR_octet_seq;
patable.p_get_CDR_short = get_CDR_short;
patable.p_get_CDR_string = get_CDR_string;
patable.p_get_CDR_typeCode = get_CDR_typeCode;
patable.p_get_CDR_ulong = get_CDR_ulong;
patable.p_get_CDR_ushort = get_CDR_ushort;
patable.p_get_CDR_wchar = get_CDR_wchar;
patable.p_get_CDR_wstring = get_CDR_wstring;
patable.p_is_tpkt = is_tpkt;
patable.p_dissect_tpkt_encap = dissect_tpkt_encap;
patable.p_set_actual_length = set_actual_length;
patable.p_decode_boolean_bitfield = decode_boolean_bitfield;
patable.p_decode_numeric_bitfield = decode_numeric_bitfield;
patable.p_decode_enumerated_bitfield = decode_enumerated_bitfield;
patable.p_register_dissector_table = register_dissector_table;
patable.p_except_throw = except_throw;
patable.p_dissector_try_port = dissector_try_port;
patable.p_conversation_add_proto_data = conversation_add_proto_data;
patable.p_conversation_get_proto_data = conversation_get_proto_data;
patable.p_conversation_delete_proto_data = conversation_delete_proto_data;
patable.p_p_add_proto_data = p_add_proto_data;
patable.p_p_get_proto_data = p_get_proto_data;
patable.p_ip_to_str = ip_to_str;
patable.p_ip6_to_str = ip6_to_str;
patable.p_time_secs_to_str = time_secs_to_str;
patable.p_time_msecs_to_str = time_msecs_to_str;
patable.p_abs_time_to_str = abs_time_to_str;
patable.p_proto_get_id_by_filter_name = proto_get_id_by_filter_name;
patable.p_proto_get_protocol_name = proto_get_protocol_name;
patable.p_proto_get_protocol_short_name = proto_get_protocol_short_name;
patable.p_proto_get_protocol_filter_name = proto_get_protocol_filter_name;
patable.p_prefs_register_obsolete_preference = prefs_register_obsolete_preference;
patable.p_add_new_data_source = add_new_data_source;
patable.p_fragment_table_init = fragment_table_init;
patable.p_reassembled_table_init = reassembled_table_init;
patable.p_fragment_add = fragment_add;
patable.p_fragment_add_seq = fragment_add_seq;
patable.p_fragment_add_seq_check = fragment_add_seq_check;
patable.p_fragment_add_seq_next = fragment_add_seq_next;
patable.p_fragment_get = fragment_get;
patable.p_fragment_set_tot_len = fragment_set_tot_len;
patable.p_fragment_get_tot_len = fragment_get_tot_len;
patable.p_fragment_set_partial_reassembly = fragment_set_partial_reassembly;
patable.p_fragment_delete = fragment_delete;
patable.p_show_fragment_tree = show_fragment_tree;
patable.p_show_fragment_seq_tree = show_fragment_seq_tree;
patable.p_register_tap = register_tap;
patable.p_tap_queue_packet = tap_queue_packet;
patable.p_asn1_open = asn1_open;
patable.p_asn1_close = asn1_close;
patable.p_asn1_octet_decode = asn1_octet_decode;
patable.p_asn1_tag_decode = asn1_tag_decode;
patable.p_asn1_id_decode = asn1_id_decode;
patable.p_asn1_length_decode = asn1_length_decode;
patable.p_asn1_header_decode = asn1_header_decode;
patable.p_asn1_eoc = asn1_eoc;
patable.p_asn1_eoc_decode = asn1_eoc_decode;
patable.p_asn1_null_decode = asn1_null_decode;
patable.p_asn1_bool_decode = asn1_bool_decode;
patable.p_asn1_int32_value_decode = asn1_int32_value_decode;
patable.p_asn1_int32_decode = asn1_int32_decode;
patable.p_asn1_uint32_value_decode = asn1_uint32_value_decode;
patable.p_asn1_uint32_decode = asn1_uint32_decode;
patable.p_asn1_bits_decode = asn1_bits_decode;
patable.p_asn1_string_value_decode = asn1_string_value_decode;
patable.p_asn1_string_decode = asn1_string_decode;
patable.p_asn1_octet_string_decode = asn1_octet_string_decode;
patable.p_asn1_subid_decode = asn1_subid_decode;
patable.p_asn1_oid_value_decode = asn1_oid_value_decode;
patable.p_asn1_oid_decode = asn1_oid_decode;
patable.p_asn1_sequence_decode = asn1_sequence_decode;
patable.p_asn1_err_to_str = asn1_err_to_str;
patable.p_proto_item_set_end = proto_item_set_end;
patable.p_proto_tree_add_none_format = proto_tree_add_none_format;
patable.p_except_init = except_init;
patable.p_except_deinit = except_deinit;
patable.p_except_rethrow = except_rethrow;
patable.p_except_throwd = except_throwd;
patable.p_except_throwf = except_throwf;
patable.p_except_unhandled_catcher = except_unhandled_catcher;
patable.p_except_take_data = except_take_data;
patable.p_except_set_allocator = except_set_allocator;
patable.p_except_alloc = except_alloc;
patable.p_except_free = except_free;
patable.p_except_pop = except_pop;
patable.p_except_setup_try = except_setup_try;
patable.p_col_set_fence = col_set_fence;
patable.p_tvb_get_string = tvb_get_string;
patable.p_tvb_get_stringz = tvb_get_stringz;
patable.p_find_dissector_table = find_dissector_table;
patable.p_dissector_get_port_handle = dissector_get_port_handle;
patable.p_dissector_handle_get_short_name = dissector_handle_get_short_name;
patable.p_dissector_handle_get_protocol_index = dissector_handle_get_protocol_index;
patable.p_new_register_dissector = new_register_dissector;
patable.p_new_create_dissector_handle = new_create_dissector_handle;
