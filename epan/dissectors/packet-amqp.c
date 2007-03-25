/* packet-amqp.c
 *
 * AMQP Wireshark dissector
 *
 * Author: Martin Sustrik <sustrik@imatix.com>
 *
 * Copyright (c) 1996-2007 iMatix Corporation
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
 */

#ifdef HAVE_CONFIG_H
#    include "config.h"
#endif

#include <gmodule.h>
#include <glib.h>
#include <epan/packet.h>
#include <epan/emem.h>
#include <epan/dissectors/packet-tcp.h>
#include <epan/prefs.h>

/*  Generic data  */

static int amqp_port = 5672;

/*  Generic defines  */

#define AMQP_INCREMENT(offset, addend, bound) {\
    int tmp;\
    tmp = offset;\
    offset += (addend);\
    DISSECTOR_ASSERT(offset >= tmp && offset <= bound);\
}

#define AMQP_FRAME_TYPE_METHOD                                    1
#define AMQP_FRAME_TYPE_CONTENT_HEADER                            2
#define AMQP_FRAME_TYPE_CONTENT_BODY                              3
#define AMQP_FRAME_TYPE_OOB_METHOD                                4
#define AMQP_FRAME_TYPE_OOB_CONTENT_HEADER                        5
#define AMQP_FRAME_TYPE_OOB_CONTENT_BODY                          6
#define AMQP_FRAME_TYPE_TRACE                                     7
#define AMQP_FRAME_TYPE_HEARTBEAT                                 8

#define AMQP_CLASS_CONNECTION                                     10
#define AMQP_CLASS_CHANNEL                                        20
#define AMQP_CLASS_ACCESS                                         30
#define AMQP_CLASS_EXCHANGE                                       40
#define AMQP_CLASS_QUEUE                                          50
#define AMQP_CLASS_BASIC                                          60
#define AMQP_CLASS_FILE                                           70
#define AMQP_CLASS_STREAM                                         80
#define AMQP_CLASS_TX                                             90
#define AMQP_CLASS_DTX                                            100
#define AMQP_CLASS_TUNNEL                                         110
#define AMQP_CLASS_TEST                                           120

#define AMQP_METHOD_CONNECTION_START                              10
#define AMQP_METHOD_CONNECTION_START_OK                           11
#define AMQP_METHOD_CONNECTION_SECURE                             20
#define AMQP_METHOD_CONNECTION_SECURE_OK                          21
#define AMQP_METHOD_CONNECTION_TUNE                               30
#define AMQP_METHOD_CONNECTION_TUNE_OK                            31
#define AMQP_METHOD_CONNECTION_OPEN                               40
#define AMQP_METHOD_CONNECTION_OPEN_OK                            41
#define AMQP_METHOD_CONNECTION_REDIRECT                           50
#define AMQP_METHOD_CONNECTION_CLOSE                              60
#define AMQP_METHOD_CONNECTION_CLOSE_OK                           61

#define AMQP_METHOD_CHANNEL_OPEN                                  10
#define AMQP_METHOD_CHANNEL_OPEN_OK                               11
#define AMQP_METHOD_CHANNEL_FLOW                                  20
#define AMQP_METHOD_CHANNEL_FLOW_OK                               21
#define AMQP_METHOD_CHANNEL_ALERT                                 30
#define AMQP_METHOD_CHANNEL_CLOSE                                 40
#define AMQP_METHOD_CHANNEL_CLOSE_OK                              41

#define AMQP_METHOD_ACCESS_REQUEST                                10
#define AMQP_METHOD_ACCESS_REQUEST_OK                             11

#define AMQP_METHOD_EXCHANGE_DECLARE                              10
#define AMQP_METHOD_EXCHANGE_DECLARE_OK                           11
#define AMQP_METHOD_EXCHANGE_DELETE                               20
#define AMQP_METHOD_EXCHANGE_DELETE_OK                            21

#define AMQP_METHOD_QUEUE_DECLARE                                 10
#define AMQP_METHOD_QUEUE_DECLARE_OK                              11
#define AMQP_METHOD_QUEUE_BIND                                    20
#define AMQP_METHOD_QUEUE_BIND_OK                                 21
#define AMQP_METHOD_QUEUE_PURGE                                   30
#define AMQP_METHOD_QUEUE_PURGE_OK                                31
#define AMQP_METHOD_QUEUE_DELETE                                  40
#define AMQP_METHOD_QUEUE_DELETE_OK                               41

#define AMQP_METHOD_BASIC_QOS                                     10
#define AMQP_METHOD_BASIC_QOS_OK                                  11
#define AMQP_METHOD_BASIC_CONSUME                                 20
#define AMQP_METHOD_BASIC_CONSUME_OK                              21
#define AMQP_METHOD_BASIC_CANCEL                                  30
#define AMQP_METHOD_BASIC_CANCEL_OK                               31
#define AMQP_METHOD_BASIC_PUBLISH                                 40
#define AMQP_METHOD_BASIC_RETURN                                  50
#define AMQP_METHOD_BASIC_DELIVER                                 60
#define AMQP_METHOD_BASIC_GET                                     70
#define AMQP_METHOD_BASIC_GET_OK                                  71
#define AMQP_METHOD_BASIC_GET_EMPTY                               72
#define AMQP_METHOD_BASIC_ACK                                     80
#define AMQP_METHOD_BASIC_REJECT                                  90
#define AMQP_METHOD_BASIC_RECOVER                                 100

#define AMQP_METHOD_FILE_QOS                                      10
#define AMQP_METHOD_FILE_QOS_OK                                   11
#define AMQP_METHOD_FILE_CONSUME                                  20
#define AMQP_METHOD_FILE_CONSUME_OK                               21
#define AMQP_METHOD_FILE_CANCEL                                   30
#define AMQP_METHOD_FILE_CANCEL_OK                                31
#define AMQP_METHOD_FILE_OPEN                                     40
#define AMQP_METHOD_FILE_OPEN_OK                                  41
#define AMQP_METHOD_FILE_STAGE                                    50
#define AMQP_METHOD_FILE_PUBLISH                                  60
#define AMQP_METHOD_FILE_RETURN                                   70
#define AMQP_METHOD_FILE_DELIVER                                  80
#define AMQP_METHOD_FILE_ACK                                      90
#define AMQP_METHOD_FILE_REJECT                                   100

#define AMQP_METHOD_STREAM_QOS                                    10
#define AMQP_METHOD_STREAM_QOS_OK                                 11
#define AMQP_METHOD_STREAM_CONSUME                                20
#define AMQP_METHOD_STREAM_CONSUME_OK                             21
#define AMQP_METHOD_STREAM_CANCEL                                 30
#define AMQP_METHOD_STREAM_CANCEL_OK                              31
#define AMQP_METHOD_STREAM_PUBLISH                                40
#define AMQP_METHOD_STREAM_RETURN                                 50
#define AMQP_METHOD_STREAM_DELIVER                                60

#define AMQP_METHOD_TX_SELECT                                     10
#define AMQP_METHOD_TX_SELECT_OK                                  11
#define AMQP_METHOD_TX_COMMIT                                     20
#define AMQP_METHOD_TX_COMMIT_OK                                  21
#define AMQP_METHOD_TX_ROLLBACK                                   30
#define AMQP_METHOD_TX_ROLLBACK_OK                                31

#define AMQP_METHOD_DTX_SELECT                                    10
#define AMQP_METHOD_DTX_SELECT_OK                                 11
#define AMQP_METHOD_DTX_START                                     20
#define AMQP_METHOD_DTX_START_OK                                  21

#define AMQP_METHOD_TUNNEL_REQUEST                                10

#define AMQP_METHOD_TEST_INTEGER                                  10
#define AMQP_METHOD_TEST_INTEGER_OK                               11
#define AMQP_METHOD_TEST_STRING                                   20
#define AMQP_METHOD_TEST_STRING_OK                                21
#define AMQP_METHOD_TEST_TABLE                                    30
#define AMQP_METHOD_TEST_TABLE_OK                                 31
#define AMQP_METHOD_TEST_CONTENT                                  40
#define AMQP_METHOD_TEST_CONTENT_OK                               41

/*  Registration functions for the dissector  */

void
proto_register_amqp(void);

void
proto_reg_handoff_amqp(void);

/*  Private functions  */

static void
dissect_amqp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);

static guint
get_amqp_message_len(packet_info *pinfo, tvbuff_t *tvb, int offset);

static void
dissect_amqp_field_table(tvbuff_t *tvb, int offset, int bound, size_t length, proto_item *item);

static void
dissect_amqp_frame(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);

static size_t
dissect_amqp_method_connection_start(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_connection_start_ok(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_connection_secure(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_connection_secure_ok(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_connection_tune(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_connection_tune_ok(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_connection_open(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_connection_open_ok(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_connection_redirect(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_connection_close(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_connection_close_ok(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_channel_open(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_channel_open_ok(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_channel_flow(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_channel_flow_ok(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_channel_alert(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_channel_close(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_channel_close_ok(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_access_request(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_access_request_ok(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_exchange_declare(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_exchange_declare_ok(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_exchange_delete(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_exchange_delete_ok(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_queue_declare(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_queue_declare_ok(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_queue_bind(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_queue_bind_ok(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_queue_purge(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_queue_purge_ok(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_queue_delete(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_queue_delete_ok(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_basic_qos(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_basic_qos_ok(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_basic_consume(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_basic_consume_ok(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_basic_cancel(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_basic_cancel_ok(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_basic_publish(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_basic_return(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_basic_deliver(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_basic_get(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_basic_get_ok(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_basic_get_empty(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_basic_ack(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_basic_reject(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_basic_recover(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_file_qos(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_file_qos_ok(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_file_consume(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_file_consume_ok(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_file_cancel(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_file_cancel_ok(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_file_open(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_file_open_ok(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_file_stage(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_file_publish(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_file_return(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_file_deliver(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_file_ack(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_file_reject(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_stream_qos(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_stream_qos_ok(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_stream_consume(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_stream_consume_ok(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_stream_cancel(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_stream_cancel_ok(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_stream_publish(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_stream_return(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_stream_deliver(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_tx_select(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_tx_select_ok(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_tx_commit(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_tx_commit_ok(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_tx_rollback(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_tx_rollback_ok(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_dtx_select(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_dtx_select_ok(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_dtx_start(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_dtx_start_ok(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_tunnel_request(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_test_integer(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_test_integer_ok(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_test_string(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_test_string_ok(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_test_table(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_test_table_ok(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_test_content(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_method_test_content_ok(tvbuff_t *tvb,
    int offset, int bound, proto_tree *args_tree);

static size_t
dissect_amqp_content_header_basic(tvbuff_t *tvb,
    int offset, int bound, proto_tree *prop_tree);

static size_t
dissect_amqp_content_header_file(tvbuff_t *tvb,
    int offset, int bound, proto_tree *prop_tree);

static size_t
dissect_amqp_content_header_stream(tvbuff_t *tvb,
    int offset, int bound, proto_tree *prop_tree);

static size_t
dissect_amqp_content_header_tunnel(tvbuff_t *tvb,
    int offset, int bound, proto_tree *prop_tree);

/*  Various handles  */

static int proto_amqp = -1;

static int hf_amqp_type = -1;
static int hf_amqp_channel = -1;
static int hf_amqp_length = -1;
static int hf_amqp_method_class_id = -1;
static int hf_amqp_method_connection_method_id = -1;
static int hf_amqp_method_channel_method_id = -1;
static int hf_amqp_method_access_method_id = -1;
static int hf_amqp_method_exchange_method_id = -1;
static int hf_amqp_method_queue_method_id = -1;
static int hf_amqp_method_basic_method_id = -1;
static int hf_amqp_method_file_method_id = -1;
static int hf_amqp_method_stream_method_id = -1;
static int hf_amqp_method_tx_method_id = -1;
static int hf_amqp_method_dtx_method_id = -1;
static int hf_amqp_method_tunnel_method_id = -1;
static int hf_amqp_method_test_method_id = -1;
static int hf_amqp_method_arguments = -1;
static int hf_amqp_method_connection_start_version_major = -1;
static int hf_amqp_method_connection_start_version_minor = -1;
static int hf_amqp_method_connection_start_server_properties = -1;
static int hf_amqp_method_connection_start_mechanisms = -1;
static int hf_amqp_method_connection_start_locales = -1;
static int hf_amqp_method_connection_start_ok_client_properties = -1;
static int hf_amqp_method_connection_start_ok_mechanism = -1;
static int hf_amqp_method_connection_start_ok_response = -1;
static int hf_amqp_method_connection_start_ok_locale = -1;
static int hf_amqp_method_connection_secure_challenge = -1;
static int hf_amqp_method_connection_secure_ok_response = -1;
static int hf_amqp_method_connection_tune_channel_max = -1;
static int hf_amqp_method_connection_tune_frame_max = -1;
static int hf_amqp_method_connection_tune_heartbeat = -1;
static int hf_amqp_method_connection_tune_ok_channel_max = -1;
static int hf_amqp_method_connection_tune_ok_frame_max = -1;
static int hf_amqp_method_connection_tune_ok_heartbeat = -1;
static int hf_amqp_method_connection_open_virtual_host = -1;
static int hf_amqp_method_connection_open_capabilities = -1;
static int hf_amqp_method_connection_open_insist = -1;
static int hf_amqp_method_connection_open_ok_known_hosts = -1;
static int hf_amqp_method_connection_redirect_host = -1;
static int hf_amqp_method_connection_redirect_known_hosts = -1;
static int hf_amqp_method_connection_close_reply_code = -1;
static int hf_amqp_method_connection_close_reply_text = -1;
static int hf_amqp_method_connection_close_class_id = -1;
static int hf_amqp_method_connection_close_method_id = -1;
static int hf_amqp_method_channel_open_out_of_band = -1;
static int hf_amqp_method_channel_flow_active = -1;
static int hf_amqp_method_channel_flow_ok_active = -1;
static int hf_amqp_method_channel_alert_reply_code = -1;
static int hf_amqp_method_channel_alert_reply_text = -1;
static int hf_amqp_method_channel_alert_details = -1;
static int hf_amqp_method_channel_close_reply_code = -1;
static int hf_amqp_method_channel_close_reply_text = -1;
static int hf_amqp_method_channel_close_class_id = -1;
static int hf_amqp_method_channel_close_method_id = -1;
static int hf_amqp_method_access_request_realm = -1;
static int hf_amqp_method_access_request_exclusive = -1;
static int hf_amqp_method_access_request_passive = -1;
static int hf_amqp_method_access_request_active = -1;
static int hf_amqp_method_access_request_write = -1;
static int hf_amqp_method_access_request_read = -1;
static int hf_amqp_method_access_request_ok_ticket = -1;
static int hf_amqp_method_exchange_declare_ticket = -1;
static int hf_amqp_method_exchange_declare_exchange = -1;
static int hf_amqp_method_exchange_declare_type = -1;
static int hf_amqp_method_exchange_declare_passive = -1;
static int hf_amqp_method_exchange_declare_durable = -1;
static int hf_amqp_method_exchange_declare_auto_delete = -1;
static int hf_amqp_method_exchange_declare_internal = -1;
static int hf_amqp_method_exchange_declare_nowait = -1;
static int hf_amqp_method_exchange_declare_arguments = -1;
static int hf_amqp_method_exchange_delete_ticket = -1;
static int hf_amqp_method_exchange_delete_exchange = -1;
static int hf_amqp_method_exchange_delete_if_unused = -1;
static int hf_amqp_method_exchange_delete_nowait = -1;
static int hf_amqp_method_queue_declare_ticket = -1;
static int hf_amqp_method_queue_declare_queue = -1;
static int hf_amqp_method_queue_declare_passive = -1;
static int hf_amqp_method_queue_declare_durable = -1;
static int hf_amqp_method_queue_declare_exclusive = -1;
static int hf_amqp_method_queue_declare_auto_delete = -1;
static int hf_amqp_method_queue_declare_nowait = -1;
static int hf_amqp_method_queue_declare_arguments = -1;
static int hf_amqp_method_queue_declare_ok_queue = -1;
static int hf_amqp_method_queue_declare_ok_message_count = -1;
static int hf_amqp_method_queue_declare_ok_consumer_count = -1;
static int hf_amqp_method_queue_bind_ticket = -1;
static int hf_amqp_method_queue_bind_queue = -1;
static int hf_amqp_method_queue_bind_exchange = -1;
static int hf_amqp_method_queue_bind_routing_key = -1;
static int hf_amqp_method_queue_bind_nowait = -1;
static int hf_amqp_method_queue_bind_arguments = -1;
static int hf_amqp_method_queue_purge_ticket = -1;
static int hf_amqp_method_queue_purge_queue = -1;
static int hf_amqp_method_queue_purge_nowait = -1;
static int hf_amqp_method_queue_purge_ok_message_count = -1;
static int hf_amqp_method_queue_delete_ticket = -1;
static int hf_amqp_method_queue_delete_queue = -1;
static int hf_amqp_method_queue_delete_if_unused = -1;
static int hf_amqp_method_queue_delete_if_empty = -1;
static int hf_amqp_method_queue_delete_nowait = -1;
static int hf_amqp_method_queue_delete_ok_message_count = -1;
static int hf_amqp_method_basic_qos_prefetch_size = -1;
static int hf_amqp_method_basic_qos_prefetch_count = -1;
static int hf_amqp_method_basic_qos_global = -1;
static int hf_amqp_method_basic_consume_ticket = -1;
static int hf_amqp_method_basic_consume_queue = -1;
static int hf_amqp_method_basic_consume_consumer_tag = -1;
static int hf_amqp_method_basic_consume_no_local = -1;
static int hf_amqp_method_basic_consume_no_ack = -1;
static int hf_amqp_method_basic_consume_exclusive = -1;
static int hf_amqp_method_basic_consume_nowait = -1;
static int hf_amqp_method_basic_consume_ok_consumer_tag = -1;
static int hf_amqp_method_basic_cancel_consumer_tag = -1;
static int hf_amqp_method_basic_cancel_nowait = -1;
static int hf_amqp_method_basic_cancel_ok_consumer_tag = -1;
static int hf_amqp_method_basic_publish_ticket = -1;
static int hf_amqp_method_basic_publish_exchange = -1;
static int hf_amqp_method_basic_publish_routing_key = -1;
static int hf_amqp_method_basic_publish_mandatory = -1;
static int hf_amqp_method_basic_publish_immediate = -1;
static int hf_amqp_method_basic_return_reply_code = -1;
static int hf_amqp_method_basic_return_reply_text = -1;
static int hf_amqp_method_basic_return_exchange = -1;
static int hf_amqp_method_basic_return_routing_key = -1;
static int hf_amqp_method_basic_deliver_consumer_tag = -1;
static int hf_amqp_method_basic_deliver_delivery_tag = -1;
static int hf_amqp_method_basic_deliver_redelivered = -1;
static int hf_amqp_method_basic_deliver_exchange = -1;
static int hf_amqp_method_basic_deliver_routing_key = -1;
static int hf_amqp_method_basic_get_ticket = -1;
static int hf_amqp_method_basic_get_queue = -1;
static int hf_amqp_method_basic_get_no_ack = -1;
static int hf_amqp_method_basic_get_ok_delivery_tag = -1;
static int hf_amqp_method_basic_get_ok_redelivered = -1;
static int hf_amqp_method_basic_get_ok_exchange = -1;
static int hf_amqp_method_basic_get_ok_routing_key = -1;
static int hf_amqp_method_basic_get_ok_message_count = -1;
static int hf_amqp_method_basic_get_empty_cluster_id = -1;
static int hf_amqp_method_basic_ack_delivery_tag = -1;
static int hf_amqp_method_basic_ack_multiple = -1;
static int hf_amqp_method_basic_reject_delivery_tag = -1;
static int hf_amqp_method_basic_reject_requeue = -1;
static int hf_amqp_method_basic_recover_requeue = -1;
static int hf_amqp_method_file_qos_prefetch_size = -1;
static int hf_amqp_method_file_qos_prefetch_count = -1;
static int hf_amqp_method_file_qos_global = -1;
static int hf_amqp_method_file_consume_ticket = -1;
static int hf_amqp_method_file_consume_queue = -1;
static int hf_amqp_method_file_consume_consumer_tag = -1;
static int hf_amqp_method_file_consume_no_local = -1;
static int hf_amqp_method_file_consume_no_ack = -1;
static int hf_amqp_method_file_consume_exclusive = -1;
static int hf_amqp_method_file_consume_nowait = -1;
static int hf_amqp_method_file_consume_ok_consumer_tag = -1;
static int hf_amqp_method_file_cancel_consumer_tag = -1;
static int hf_amqp_method_file_cancel_nowait = -1;
static int hf_amqp_method_file_cancel_ok_consumer_tag = -1;
static int hf_amqp_method_file_open_identifier = -1;
static int hf_amqp_method_file_open_content_size = -1;
static int hf_amqp_method_file_open_ok_staged_size = -1;
static int hf_amqp_method_file_publish_ticket = -1;
static int hf_amqp_method_file_publish_exchange = -1;
static int hf_amqp_method_file_publish_routing_key = -1;
static int hf_amqp_method_file_publish_mandatory = -1;
static int hf_amqp_method_file_publish_immediate = -1;
static int hf_amqp_method_file_publish_identifier = -1;
static int hf_amqp_method_file_return_reply_code = -1;
static int hf_amqp_method_file_return_reply_text = -1;
static int hf_amqp_method_file_return_exchange = -1;
static int hf_amqp_method_file_return_routing_key = -1;
static int hf_amqp_method_file_deliver_consumer_tag = -1;
static int hf_amqp_method_file_deliver_delivery_tag = -1;
static int hf_amqp_method_file_deliver_redelivered = -1;
static int hf_amqp_method_file_deliver_exchange = -1;
static int hf_amqp_method_file_deliver_routing_key = -1;
static int hf_amqp_method_file_deliver_identifier = -1;
static int hf_amqp_method_file_ack_delivery_tag = -1;
static int hf_amqp_method_file_ack_multiple = -1;
static int hf_amqp_method_file_reject_delivery_tag = -1;
static int hf_amqp_method_file_reject_requeue = -1;
static int hf_amqp_method_stream_qos_prefetch_size = -1;
static int hf_amqp_method_stream_qos_prefetch_count = -1;
static int hf_amqp_method_stream_qos_consume_rate = -1;
static int hf_amqp_method_stream_qos_global = -1;
static int hf_amqp_method_stream_consume_ticket = -1;
static int hf_amqp_method_stream_consume_queue = -1;
static int hf_amqp_method_stream_consume_consumer_tag = -1;
static int hf_amqp_method_stream_consume_no_local = -1;
static int hf_amqp_method_stream_consume_exclusive = -1;
static int hf_amqp_method_stream_consume_nowait = -1;
static int hf_amqp_method_stream_consume_ok_consumer_tag = -1;
static int hf_amqp_method_stream_cancel_consumer_tag = -1;
static int hf_amqp_method_stream_cancel_nowait = -1;
static int hf_amqp_method_stream_cancel_ok_consumer_tag = -1;
static int hf_amqp_method_stream_publish_ticket = -1;
static int hf_amqp_method_stream_publish_exchange = -1;
static int hf_amqp_method_stream_publish_routing_key = -1;
static int hf_amqp_method_stream_publish_mandatory = -1;
static int hf_amqp_method_stream_publish_immediate = -1;
static int hf_amqp_method_stream_return_reply_code = -1;
static int hf_amqp_method_stream_return_reply_text = -1;
static int hf_amqp_method_stream_return_exchange = -1;
static int hf_amqp_method_stream_return_routing_key = -1;
static int hf_amqp_method_stream_deliver_consumer_tag = -1;
static int hf_amqp_method_stream_deliver_delivery_tag = -1;
static int hf_amqp_method_stream_deliver_exchange = -1;
static int hf_amqp_method_stream_deliver_queue = -1;
static int hf_amqp_method_dtx_start_dtx_identifier = -1;
static int hf_amqp_method_tunnel_request_meta_data = -1;
static int hf_amqp_method_test_integer_integer_1 = -1;
static int hf_amqp_method_test_integer_integer_2 = -1;
static int hf_amqp_method_test_integer_integer_3 = -1;
static int hf_amqp_method_test_integer_integer_4 = -1;
static int hf_amqp_method_test_integer_operation = -1;
static int hf_amqp_method_test_integer_ok_result = -1;
static int hf_amqp_method_test_string_string_1 = -1;
static int hf_amqp_method_test_string_string_2 = -1;
static int hf_amqp_method_test_string_operation = -1;
static int hf_amqp_method_test_string_ok_result = -1;
static int hf_amqp_method_test_table_table = -1;
static int hf_amqp_method_test_table_integer_op = -1;
static int hf_amqp_method_test_table_string_op = -1;
static int hf_amqp_method_test_table_ok_integer_result = -1;
static int hf_amqp_method_test_table_ok_string_result = -1;
static int hf_amqp_method_test_content_ok_content_checksum = -1;
static int hf_amqp_field = -1;
static int hf_amqp_header_class_id = -1;
static int hf_amqp_header_weight = -1;
static int hf_amqp_header_body_size = -1;
static int hf_amqp_header_property_flags = -1;
static int hf_amqp_header_properties = -1;
static int hf_amqp_header_basic_content_type = -1;
static int hf_amqp_header_basic_content_encoding = -1;
static int hf_amqp_header_basic_headers = -1;
static int hf_amqp_header_basic_delivery_mode = -1;
static int hf_amqp_header_basic_priority = -1;
static int hf_amqp_header_basic_correlation_id = -1;
static int hf_amqp_header_basic_reply_to = -1;
static int hf_amqp_header_basic_expiration = -1;
static int hf_amqp_header_basic_message_id = -1;
static int hf_amqp_header_basic_timestamp = -1;
static int hf_amqp_header_basic_type = -1;
static int hf_amqp_header_basic_user_id = -1;
static int hf_amqp_header_basic_app_id = -1;
static int hf_amqp_header_basic_cluster_id = -1;
static int hf_amqp_header_file_content_type = -1;
static int hf_amqp_header_file_content_encoding = -1;
static int hf_amqp_header_file_headers = -1;
static int hf_amqp_header_file_priority = -1;
static int hf_amqp_header_file_reply_to = -1;
static int hf_amqp_header_file_message_id = -1;
static int hf_amqp_header_file_filename = -1;
static int hf_amqp_header_file_timestamp = -1;
static int hf_amqp_header_file_cluster_id = -1;
static int hf_amqp_header_stream_content_type = -1;
static int hf_amqp_header_stream_content_encoding = -1;
static int hf_amqp_header_stream_headers = -1;
static int hf_amqp_header_stream_priority = -1;
static int hf_amqp_header_stream_timestamp = -1;
static int hf_amqp_header_tunnel_headers = -1;
static int hf_amqp_header_tunnel_proxy_name = -1;
static int hf_amqp_header_tunnel_data_name = -1;
static int hf_amqp_header_tunnel_durable = -1;
static int hf_amqp_header_tunnel_broadcast = -1;
static int hf_amqp_payload = -1;

static gint ett_amqp = -1;
static gint ett_args = -1;
static gint ett_props = -1;
static gint ett_field_table = -1;

/*  Various enumerations  */

static const value_string amqp_frame_types [] = {
    {AMQP_FRAME_TYPE_METHOD,             "Method"},
    {AMQP_FRAME_TYPE_CONTENT_HEADER,     "Content header"},
    {AMQP_FRAME_TYPE_CONTENT_BODY,       "Content body"},
    {AMQP_FRAME_TYPE_OOB_METHOD,         "OOB Method"},
    {AMQP_FRAME_TYPE_OOB_CONTENT_HEADER, "OOB Content header"},
    {AMQP_FRAME_TYPE_OOB_CONTENT_BODY,   "OOB Content body"},
    {AMQP_FRAME_TYPE_TRACE ,             "Trace"},
    {AMQP_FRAME_TYPE_HEARTBEAT,          "Heartbeat"},
    {0, NULL}
};

static const value_string amqp_method_classes [] = {
    {10, "Connection"},
    {20, "Channel"},
    {30, "Access"},
    {40, "Exchange"},
    {50, "Queue"},
    {60, "Basic"},
    {70, "File"},
    {80, "Stream"},
    {90, "Tx"},
    {100, "Dtx"},
    {110, "Tunnel"},
    {120, "Test"},
    {0, NULL}
};

static const value_string amqp_method_connection_methods [] = {
    {10, "Start"},
    {11, "Start-Ok"},
    {20, "Secure"},
    {21, "Secure-Ok"},
    {30, "Tune"},
    {31, "Tune-Ok"},
    {40, "Open"},
    {41, "Open-Ok"},
    {50, "Redirect"},
    {60, "Close"},
    {61, "Close-Ok"},
    {0, NULL}
};

static const value_string amqp_method_channel_methods [] = {
    {10, "Open"},
    {11, "Open-Ok"},
    {20, "Flow"},
    {21, "Flow-Ok"},
    {30, "Alert"},
    {40, "Close"},
    {41, "Close-Ok"},
    {0, NULL}
};

static const value_string amqp_method_access_methods [] = {
    {10, "Request"},
    {11, "Request-Ok"},
    {0, NULL}
};

static const value_string amqp_method_exchange_methods [] = {
    {10, "Declare"},
    {11, "Declare-Ok"},
    {20, "Delete"},
    {21, "Delete-Ok"},
    {0, NULL}
};

static const value_string amqp_method_queue_methods [] = {
    {10, "Declare"},
    {11, "Declare-Ok"},
    {20, "Bind"},
    {21, "Bind-Ok"},
    {30, "Purge"},
    {31, "Purge-Ok"},
    {40, "Delete"},
    {41, "Delete-Ok"},
    {0, NULL}
};

static const value_string amqp_method_basic_methods [] = {
    {10, "Qos"},
    {11, "Qos-Ok"},
    {20, "Consume"},
    {21, "Consume-Ok"},
    {30, "Cancel"},
    {31, "Cancel-Ok"},
    {40, "Publish"},
    {50, "Return"},
    {60, "Deliver"},
    {70, "Get"},
    {71, "Get-Ok"},
    {72, "Get-Empty"},
    {80, "Ack"},
    {90, "Reject"},
    {100, "Recover"},
    {0, NULL}
};

static const value_string amqp_method_file_methods [] = {
    {10, "Qos"},
    {11, "Qos-Ok"},
    {20, "Consume"},
    {21, "Consume-Ok"},
    {30, "Cancel"},
    {31, "Cancel-Ok"},
    {40, "Open"},
    {41, "Open-Ok"},
    {50, "Stage"},
    {60, "Publish"},
    {70, "Return"},
    {80, "Deliver"},
    {90, "Ack"},
    {100, "Reject"},
    {0, NULL}
};

static const value_string amqp_method_stream_methods [] = {
    {10, "Qos"},
    {11, "Qos-Ok"},
    {20, "Consume"},
    {21, "Consume-Ok"},
    {30, "Cancel"},
    {31, "Cancel-Ok"},
    {40, "Publish"},
    {50, "Return"},
    {60, "Deliver"},
    {0, NULL}
};

static const value_string amqp_method_tx_methods [] = {
    {10, "Select"},
    {11, "Select-Ok"},
    {20, "Commit"},
    {21, "Commit-Ok"},
    {30, "Rollback"},
    {31, "Rollback-Ok"},
    {0, NULL}
};

static const value_string amqp_method_dtx_methods [] = {
    {10, "Select"},
    {11, "Select-Ok"},
    {20, "Start"},
    {21, "Start-Ok"},
    {0, NULL}
};

static const value_string amqp_method_tunnel_methods [] = {
    {10, "Request"},
    {0, NULL}
};

static const value_string amqp_method_test_methods [] = {
    {10, "Integer"},
    {11, "Integer-Ok"},
    {20, "String"},
    {21, "String-Ok"},
    {30, "Table"},
    {31, "Table-Ok"},
    {40, "Content"},
    {41, "Content-Ok"},
    {0, NULL}
};

/*  Main dissection routine  */

static void
dissect_amqp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
    tcp_dissect_pdus(tvb, pinfo, tree, TRUE, 7,
        get_amqp_message_len, dissect_amqp_frame);
}

static guint
get_amqp_message_len(packet_info *pinfo _U_, tvbuff_t *tvb, int offset)
{
    return (guint) tvb_get_ntohl(tvb, offset + 3) + 8;
}

/*  Dissection routine for AMQP field tables  */

static void
dissect_amqp_field_table(tvbuff_t *tvb, int offset, int bound, size_t length, proto_item *item)
{
    proto_item *field_table_tree;
    char *buff;
    size_t namelen, vallen;
    guint8 type;
    const char *name;
    const char *typename;
    const char *value;
    size_t field_start;

    buff = ep_alloc(64);

    field_table_tree = proto_item_add_subtree(item, ett_amqp);

    while (length > 0) {
        field_start = offset;
        namelen = tvb_get_guint8(tvb, offset);
        AMQP_INCREMENT(offset, 1, bound);
        length -= 1;
        name = (char*) tvb_get_ephemeral_string(tvb, offset, namelen);
        AMQP_INCREMENT(offset, namelen, bound);
        length -= namelen;
        type = tvb_get_guint8(tvb, offset);
        AMQP_INCREMENT(offset, 1, bound);
        length -= 1;
        switch (type) {
        case 'S':
            typename = "string";
            vallen = tvb_get_ntohl(tvb, offset);
            AMQP_INCREMENT(offset, 4, bound);
            length -= 4;
            value = (char*) tvb_get_ephemeral_string(tvb, offset, vallen);
            AMQP_INCREMENT(offset, vallen, bound);
            length -= vallen;
            break;
        case 'I':
            typename = "integer";
            g_snprintf(buff, 64, "%ld", (long) tvb_get_ntohl(tvb, offset));
            value = buff;
            AMQP_INCREMENT(offset, 4, bound);
            length -= 4;  
            break;
        case 'D':
            typename = "decimal";
            value = "...";
            AMQP_INCREMENT(offset, 5, bound);
            length -= 5; 
            break;
        case 'T':
            typename =  "timestamp";
            value = "...";
            AMQP_INCREMENT(offset, 8, bound);
            length -= 8; 
            break;
        case 'F':
            /*  TODO: make it recursive here  */
            typename =  "field table";
            vallen = tvb_get_ntohl(tvb, offset);
            AMQP_INCREMENT(offset, 4, bound);
            length -= 4;
            value = "...";
            AMQP_INCREMENT(offset, vallen, bound);
            length -= vallen;
            break;
        case 'V':
            typename = "void";
            value = "";
        default:
            typename = "";
            value = "";
            DISSECTOR_ASSERT(FALSE);
        }

        proto_tree_add_none_format(field_table_tree, hf_amqp_field, tvb,
            field_start, offset - field_start, "%s (%s): %s", name, typename,
            value);
    }
}

/*  Dissection routine for AMQP frames  */

static void
dissect_amqp_frame(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
    proto_item *ti;
    proto_item *amqp_tree;
    proto_item *args_tree;
    proto_item *prop_tree;
    size_t length;
    int offset;

    if (check_col(pinfo->cinfo, COL_PROTOCOL))
        col_set_str(pinfo->cinfo, COL_PROTOCOL, "AMQP");
    if (check_col(pinfo->cinfo, COL_INFO)) {
        col_clear(pinfo->cinfo, COL_INFO);
    }

    if (tree) {
        
        ti = proto_tree_add_item(tree, proto_amqp, tvb, 0, -1, FALSE);
        amqp_tree = proto_item_add_subtree(ti, ett_amqp);
	proto_tree_add_item(amqp_tree, hf_amqp_type, tvb, 0, 1, FALSE);
	proto_tree_add_item(amqp_tree, hf_amqp_channel, tvb, 1, 2, FALSE);
	proto_tree_add_item(amqp_tree, hf_amqp_length, tvb, 3, 4, FALSE);
        length = tvb_get_ntohl(tvb, 3);
        switch (tvb_get_guint8(tvb, 0)) {
        case AMQP_FRAME_TYPE_METHOD:
            proto_tree_add_item(amqp_tree, hf_amqp_method_class_id,
                tvb, 7, 2, FALSE);
            switch (tvb_get_ntohs(tvb, 7)) {
            case AMQP_CLASS_CONNECTION:
                proto_tree_add_item(amqp_tree, hf_amqp_method_connection_method_id,
                    tvb, 9, 2, FALSE);              
                ti = proto_tree_add_item(amqp_tree, hf_amqp_method_arguments,
                    tvb, 11, length - 4, FALSE);
                args_tree = proto_item_add_subtree(ti, ett_args);
                switch (tvb_get_ntohs(tvb, 9)) {
                case AMQP_METHOD_CONNECTION_START:
                    offset = dissect_amqp_method_connection_start(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_CONNECTION_START_OK:
                    offset = dissect_amqp_method_connection_start_ok(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_CONNECTION_SECURE:
                    offset = dissect_amqp_method_connection_secure(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_CONNECTION_SECURE_OK:
                    offset = dissect_amqp_method_connection_secure_ok(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_CONNECTION_TUNE:
                    offset = dissect_amqp_method_connection_tune(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_CONNECTION_TUNE_OK:
                    offset = dissect_amqp_method_connection_tune_ok(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_CONNECTION_OPEN:
                    offset = dissect_amqp_method_connection_open(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_CONNECTION_OPEN_OK:
                    offset = dissect_amqp_method_connection_open_ok(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_CONNECTION_REDIRECT:
                    offset = dissect_amqp_method_connection_redirect(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_CONNECTION_CLOSE:
                    offset = dissect_amqp_method_connection_close(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_CONNECTION_CLOSE_OK:
                    offset = dissect_amqp_method_connection_close_ok(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                default:
                    DISSECTOR_ASSERT(FALSE);
                }
                break;
            case AMQP_CLASS_CHANNEL:
                proto_tree_add_item(amqp_tree, hf_amqp_method_channel_method_id,
                    tvb, 9, 2, FALSE);              
                ti = proto_tree_add_item(amqp_tree, hf_amqp_method_arguments,
                    tvb, 11, length - 4, FALSE);
                args_tree = proto_item_add_subtree(ti, ett_args);
                switch (tvb_get_ntohs(tvb, 9)) {
                case AMQP_METHOD_CHANNEL_OPEN:
                    offset = dissect_amqp_method_channel_open(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_CHANNEL_OPEN_OK:
                    offset = dissect_amqp_method_channel_open_ok(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_CHANNEL_FLOW:
                    offset = dissect_amqp_method_channel_flow(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_CHANNEL_FLOW_OK:
                    offset = dissect_amqp_method_channel_flow_ok(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_CHANNEL_ALERT:
                    offset = dissect_amqp_method_channel_alert(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_CHANNEL_CLOSE:
                    offset = dissect_amqp_method_channel_close(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_CHANNEL_CLOSE_OK:
                    offset = dissect_amqp_method_channel_close_ok(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                default:
                    DISSECTOR_ASSERT(FALSE);
                }
                break;
            case AMQP_CLASS_ACCESS:
                proto_tree_add_item(amqp_tree, hf_amqp_method_access_method_id,
                    tvb, 9, 2, FALSE);              
                ti = proto_tree_add_item(amqp_tree, hf_amqp_method_arguments,
                    tvb, 11, length - 4, FALSE);
                args_tree = proto_item_add_subtree(ti, ett_args);
                switch (tvb_get_ntohs(tvb, 9)) {
                case AMQP_METHOD_ACCESS_REQUEST:
                    offset = dissect_amqp_method_access_request(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_ACCESS_REQUEST_OK:
                    offset = dissect_amqp_method_access_request_ok(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                default:
                    DISSECTOR_ASSERT(FALSE);
                }
                break;
            case AMQP_CLASS_EXCHANGE:
                proto_tree_add_item(amqp_tree, hf_amqp_method_exchange_method_id,
                    tvb, 9, 2, FALSE);              
                ti = proto_tree_add_item(amqp_tree, hf_amqp_method_arguments,
                    tvb, 11, length - 4, FALSE);
                args_tree = proto_item_add_subtree(ti, ett_args);
                switch (tvb_get_ntohs(tvb, 9)) {
                case AMQP_METHOD_EXCHANGE_DECLARE:
                    offset = dissect_amqp_method_exchange_declare(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_EXCHANGE_DECLARE_OK:
                    offset = dissect_amqp_method_exchange_declare_ok(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_EXCHANGE_DELETE:
                    offset = dissect_amqp_method_exchange_delete(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_EXCHANGE_DELETE_OK:
                    offset = dissect_amqp_method_exchange_delete_ok(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                default:
                    DISSECTOR_ASSERT(FALSE);
                }
                break;
            case AMQP_CLASS_QUEUE:
                proto_tree_add_item(amqp_tree, hf_amqp_method_queue_method_id,
                    tvb, 9, 2, FALSE);              
                ti = proto_tree_add_item(amqp_tree, hf_amqp_method_arguments,
                    tvb, 11, length - 4, FALSE);
                args_tree = proto_item_add_subtree(ti, ett_args);
                switch (tvb_get_ntohs(tvb, 9)) {
                case AMQP_METHOD_QUEUE_DECLARE:
                    offset = dissect_amqp_method_queue_declare(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_QUEUE_DECLARE_OK:
                    offset = dissect_amqp_method_queue_declare_ok(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_QUEUE_BIND:
                    offset = dissect_amqp_method_queue_bind(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_QUEUE_BIND_OK:
                    offset = dissect_amqp_method_queue_bind_ok(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_QUEUE_PURGE:
                    offset = dissect_amqp_method_queue_purge(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_QUEUE_PURGE_OK:
                    offset = dissect_amqp_method_queue_purge_ok(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_QUEUE_DELETE:
                    offset = dissect_amqp_method_queue_delete(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_QUEUE_DELETE_OK:
                    offset = dissect_amqp_method_queue_delete_ok(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                default:
                    DISSECTOR_ASSERT(FALSE);
                }
                break;
            case AMQP_CLASS_BASIC:
                proto_tree_add_item(amqp_tree, hf_amqp_method_basic_method_id,
                    tvb, 9, 2, FALSE);              
                ti = proto_tree_add_item(amqp_tree, hf_amqp_method_arguments,
                    tvb, 11, length - 4, FALSE);
                args_tree = proto_item_add_subtree(ti, ett_args);
                switch (tvb_get_ntohs(tvb, 9)) {
                case AMQP_METHOD_BASIC_QOS:
                    offset = dissect_amqp_method_basic_qos(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_BASIC_QOS_OK:
                    offset = dissect_amqp_method_basic_qos_ok(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_BASIC_CONSUME:
                    offset = dissect_amqp_method_basic_consume(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_BASIC_CONSUME_OK:
                    offset = dissect_amqp_method_basic_consume_ok(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_BASIC_CANCEL:
                    offset = dissect_amqp_method_basic_cancel(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_BASIC_CANCEL_OK:
                    offset = dissect_amqp_method_basic_cancel_ok(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_BASIC_PUBLISH:
                    offset = dissect_amqp_method_basic_publish(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_BASIC_RETURN:
                    offset = dissect_amqp_method_basic_return(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_BASIC_DELIVER:
                    offset = dissect_amqp_method_basic_deliver(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_BASIC_GET:
                    offset = dissect_amqp_method_basic_get(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_BASIC_GET_OK:
                    offset = dissect_amqp_method_basic_get_ok(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_BASIC_GET_EMPTY:
                    offset = dissect_amqp_method_basic_get_empty(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_BASIC_ACK:
                    offset = dissect_amqp_method_basic_ack(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_BASIC_REJECT:
                    offset = dissect_amqp_method_basic_reject(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_BASIC_RECOVER:
                    offset = dissect_amqp_method_basic_recover(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                default:
                    DISSECTOR_ASSERT(FALSE);
                }
                break;
            case AMQP_CLASS_FILE:
                proto_tree_add_item(amqp_tree, hf_amqp_method_file_method_id,
                    tvb, 9, 2, FALSE);              
                ti = proto_tree_add_item(amqp_tree, hf_amqp_method_arguments,
                    tvb, 11, length - 4, FALSE);
                args_tree = proto_item_add_subtree(ti, ett_args);
                switch (tvb_get_ntohs(tvb, 9)) {
                case AMQP_METHOD_FILE_QOS:
                    offset = dissect_amqp_method_file_qos(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_FILE_QOS_OK:
                    offset = dissect_amqp_method_file_qos_ok(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_FILE_CONSUME:
                    offset = dissect_amqp_method_file_consume(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_FILE_CONSUME_OK:
                    offset = dissect_amqp_method_file_consume_ok(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_FILE_CANCEL:
                    offset = dissect_amqp_method_file_cancel(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_FILE_CANCEL_OK:
                    offset = dissect_amqp_method_file_cancel_ok(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_FILE_OPEN:
                    offset = dissect_amqp_method_file_open(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_FILE_OPEN_OK:
                    offset = dissect_amqp_method_file_open_ok(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_FILE_STAGE:
                    offset = dissect_amqp_method_file_stage(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_FILE_PUBLISH:
                    offset = dissect_amqp_method_file_publish(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_FILE_RETURN:
                    offset = dissect_amqp_method_file_return(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_FILE_DELIVER:
                    offset = dissect_amqp_method_file_deliver(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_FILE_ACK:
                    offset = dissect_amqp_method_file_ack(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_FILE_REJECT:
                    offset = dissect_amqp_method_file_reject(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                default:
                    DISSECTOR_ASSERT(FALSE);
                }
                break;
            case AMQP_CLASS_STREAM:
                proto_tree_add_item(amqp_tree, hf_amqp_method_stream_method_id,
                    tvb, 9, 2, FALSE);              
                ti = proto_tree_add_item(amqp_tree, hf_amqp_method_arguments,
                    tvb, 11, length - 4, FALSE);
                args_tree = proto_item_add_subtree(ti, ett_args);
                switch (tvb_get_ntohs(tvb, 9)) {
                case AMQP_METHOD_STREAM_QOS:
                    offset = dissect_amqp_method_stream_qos(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_STREAM_QOS_OK:
                    offset = dissect_amqp_method_stream_qos_ok(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_STREAM_CONSUME:
                    offset = dissect_amqp_method_stream_consume(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_STREAM_CONSUME_OK:
                    offset = dissect_amqp_method_stream_consume_ok(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_STREAM_CANCEL:
                    offset = dissect_amqp_method_stream_cancel(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_STREAM_CANCEL_OK:
                    offset = dissect_amqp_method_stream_cancel_ok(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_STREAM_PUBLISH:
                    offset = dissect_amqp_method_stream_publish(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_STREAM_RETURN:
                    offset = dissect_amqp_method_stream_return(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_STREAM_DELIVER:
                    offset = dissect_amqp_method_stream_deliver(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                default:
                    DISSECTOR_ASSERT(FALSE);
                }
                break;
            case AMQP_CLASS_TX:
                proto_tree_add_item(amqp_tree, hf_amqp_method_tx_method_id,
                    tvb, 9, 2, FALSE);              
                ti = proto_tree_add_item(amqp_tree, hf_amqp_method_arguments,
                    tvb, 11, length - 4, FALSE);
                args_tree = proto_item_add_subtree(ti, ett_args);
                switch (tvb_get_ntohs(tvb, 9)) {
                case AMQP_METHOD_TX_SELECT:
                    offset = dissect_amqp_method_tx_select(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_TX_SELECT_OK:
                    offset = dissect_amqp_method_tx_select_ok(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_TX_COMMIT:
                    offset = dissect_amqp_method_tx_commit(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_TX_COMMIT_OK:
                    offset = dissect_amqp_method_tx_commit_ok(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_TX_ROLLBACK:
                    offset = dissect_amqp_method_tx_rollback(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_TX_ROLLBACK_OK:
                    offset = dissect_amqp_method_tx_rollback_ok(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                default:
                    DISSECTOR_ASSERT(FALSE);
                }
                break;
            case AMQP_CLASS_DTX:
                proto_tree_add_item(amqp_tree, hf_amqp_method_dtx_method_id,
                    tvb, 9, 2, FALSE);              
                ti = proto_tree_add_item(amqp_tree, hf_amqp_method_arguments,
                    tvb, 11, length - 4, FALSE);
                args_tree = proto_item_add_subtree(ti, ett_args);
                switch (tvb_get_ntohs(tvb, 9)) {
                case AMQP_METHOD_DTX_SELECT:
                    offset = dissect_amqp_method_dtx_select(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_DTX_SELECT_OK:
                    offset = dissect_amqp_method_dtx_select_ok(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_DTX_START:
                    offset = dissect_amqp_method_dtx_start(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_DTX_START_OK:
                    offset = dissect_amqp_method_dtx_start_ok(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                default:
                    DISSECTOR_ASSERT(FALSE);
                }
                break;
            case AMQP_CLASS_TUNNEL:
                proto_tree_add_item(amqp_tree, hf_amqp_method_tunnel_method_id,
                    tvb, 9, 2, FALSE);              
                ti = proto_tree_add_item(amqp_tree, hf_amqp_method_arguments,
                    tvb, 11, length - 4, FALSE);
                args_tree = proto_item_add_subtree(ti, ett_args);
                switch (tvb_get_ntohs(tvb, 9)) {
                case AMQP_METHOD_TUNNEL_REQUEST:
                    offset = dissect_amqp_method_tunnel_request(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                default:
                    DISSECTOR_ASSERT(FALSE);
                }
                break;
            case AMQP_CLASS_TEST:
                proto_tree_add_item(amqp_tree, hf_amqp_method_test_method_id,
                    tvb, 9, 2, FALSE);              
                ti = proto_tree_add_item(amqp_tree, hf_amqp_method_arguments,
                    tvb, 11, length - 4, FALSE);
                args_tree = proto_item_add_subtree(ti, ett_args);
                switch (tvb_get_ntohs(tvb, 9)) {
                case AMQP_METHOD_TEST_INTEGER:
                    offset = dissect_amqp_method_test_integer(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_TEST_INTEGER_OK:
                    offset = dissect_amqp_method_test_integer_ok(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_TEST_STRING:
                    offset = dissect_amqp_method_test_string(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_TEST_STRING_OK:
                    offset = dissect_amqp_method_test_string_ok(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_TEST_TABLE:
                    offset = dissect_amqp_method_test_table(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_TEST_TABLE_OK:
                    offset = dissect_amqp_method_test_table_ok(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_TEST_CONTENT:
                    offset = dissect_amqp_method_test_content(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                case AMQP_METHOD_TEST_CONTENT_OK:
                    offset = dissect_amqp_method_test_content_ok(tvb,
                        11, tvb_length (tvb), args_tree);
                    break;
                default:
                    DISSECTOR_ASSERT(FALSE);
                }
                break;
            default:
                DISSECTOR_ASSERT(FALSE);
            }
            break;
        case AMQP_FRAME_TYPE_CONTENT_HEADER:
            proto_tree_add_item(amqp_tree, hf_amqp_header_class_id,
                tvb, 7, 2, FALSE);
            proto_tree_add_item(amqp_tree, hf_amqp_header_weight,
                tvb, 9, 2, FALSE);
            proto_tree_add_item(amqp_tree, hf_amqp_header_body_size,
                tvb, 11, 8, FALSE);
            proto_tree_add_item(amqp_tree, hf_amqp_header_property_flags,
                tvb, 19, 2, FALSE);
            ti = proto_tree_add_item(amqp_tree, hf_amqp_header_properties,
                tvb, 21, length - 14, FALSE);
            prop_tree = proto_item_add_subtree(ti, ett_props);
            offset = 21;
            switch (tvb_get_ntohs(tvb, 7)) {
            case AMQP_CLASS_BASIC:
                offset = dissect_amqp_content_header_basic(tvb,
                    offset, tvb_length (tvb), prop_tree);
                break;
            case AMQP_CLASS_FILE:
                offset = dissect_amqp_content_header_file(tvb,
                    offset, tvb_length (tvb), prop_tree);
                break;
            case AMQP_CLASS_STREAM:
                offset = dissect_amqp_content_header_stream(tvb,
                    offset, tvb_length (tvb), prop_tree);
                break;
            case AMQP_CLASS_TUNNEL:
                offset = dissect_amqp_content_header_tunnel(tvb,
                    offset, tvb_length (tvb), prop_tree);
                break;
            default:
                DISSECTOR_ASSERT(FALSE);
            }
            break;
        case AMQP_FRAME_TYPE_CONTENT_BODY:
            proto_tree_add_item(amqp_tree, hf_amqp_payload,
                tvb, 7, length, FALSE);
            break;
        default:
            DISSECTOR_ASSERT(FALSE);
        }
    }
}

/*  Dissection routine for method Connection.Start                        */

static size_t
dissect_amqp_method_connection_start(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    proto_item *ti;
    /*  version major (octet)    */
    proto_tree_add_item(args_tree, hf_amqp_method_connection_start_version_major,
        tvb, offset, 1, FALSE);
    AMQP_INCREMENT(offset, 1, bound);

    /*  version minor (octet)    */
    proto_tree_add_item(args_tree, hf_amqp_method_connection_start_version_minor,
        tvb, offset, 1, FALSE);
    AMQP_INCREMENT(offset, 1, bound);

    /*  server properties (table)  */
    ti = proto_tree_add_item(
        args_tree, hf_amqp_method_connection_start_server_properties,
        tvb, offset + 4, tvb_get_ntohl(tvb, offset), FALSE);
    dissect_amqp_field_table (tvb, offset + 4,
        offset + 4 + tvb_get_ntohl(tvb, offset), tvb_get_ntohl(tvb, offset), ti);
    AMQP_INCREMENT(offset, 4 + tvb_get_ntohl(tvb, offset), bound);

    /*  mechanisms (longstr)     */
    proto_tree_add_item(args_tree, hf_amqp_method_connection_start_mechanisms,
        tvb, offset + 4, tvb_get_ntohl(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 4 + tvb_get_ntohl(tvb, offset), bound);

    /*  locales (longstr)        */
    proto_tree_add_item(args_tree, hf_amqp_method_connection_start_locales,
        tvb, offset + 4, tvb_get_ntohl(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 4 + tvb_get_ntohl(tvb, offset), bound);

    return offset;
}

/*  Dissection routine for method Connection.Start-Ok                     */

static size_t
dissect_amqp_method_connection_start_ok(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    proto_item *ti;
    /*  client properties (table)  */
    ti = proto_tree_add_item(
        args_tree, hf_amqp_method_connection_start_ok_client_properties,
        tvb, offset + 4, tvb_get_ntohl(tvb, offset), FALSE);
    dissect_amqp_field_table (tvb, offset + 4,
        offset + 4 + tvb_get_ntohl(tvb, offset), tvb_get_ntohl(tvb, offset), ti);
    AMQP_INCREMENT(offset, 4 + tvb_get_ntohl(tvb, offset), bound);

    /*  mechanism (shortstr)     */
    proto_tree_add_item(args_tree, hf_amqp_method_connection_start_ok_mechanism,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  response (longstr)       */
    proto_tree_add_item(args_tree, hf_amqp_method_connection_start_ok_response,
        tvb, offset + 4, tvb_get_ntohl(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 4 + tvb_get_ntohl(tvb, offset), bound);

    /*  locale (shortstr)        */
    proto_tree_add_item(args_tree, hf_amqp_method_connection_start_ok_locale,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    return offset;
}

/*  Dissection routine for method Connection.Secure                       */

static size_t
dissect_amqp_method_connection_secure(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  challenge (longstr)      */
    proto_tree_add_item(args_tree, hf_amqp_method_connection_secure_challenge,
        tvb, offset + 4, tvb_get_ntohl(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 4 + tvb_get_ntohl(tvb, offset), bound);

    return offset;
}

/*  Dissection routine for method Connection.Secure-Ok                    */

static size_t
dissect_amqp_method_connection_secure_ok(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  response (longstr)       */
    proto_tree_add_item(args_tree, hf_amqp_method_connection_secure_ok_response,
        tvb, offset + 4, tvb_get_ntohl(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 4 + tvb_get_ntohl(tvb, offset), bound);

    return offset;
}

/*  Dissection routine for method Connection.Tune                         */

static size_t
dissect_amqp_method_connection_tune(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  channel max (short)      */
    proto_tree_add_item(args_tree, hf_amqp_method_connection_tune_channel_max,
        tvb, offset, 2, FALSE);
    AMQP_INCREMENT(offset, 2, bound);

    /*  frame max (long)         */
    proto_tree_add_item(args_tree, hf_amqp_method_connection_tune_frame_max,
        tvb, offset, 4, FALSE);
    AMQP_INCREMENT(offset, 4, bound);

    /*  heartbeat (short)        */
    proto_tree_add_item(args_tree, hf_amqp_method_connection_tune_heartbeat,
        tvb, offset, 2, FALSE);
    AMQP_INCREMENT(offset, 2, bound);

    return offset;
}

/*  Dissection routine for method Connection.Tune-Ok                      */

static size_t
dissect_amqp_method_connection_tune_ok(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  channel max (short)      */
    proto_tree_add_item(args_tree, hf_amqp_method_connection_tune_ok_channel_max,
        tvb, offset, 2, FALSE);
    AMQP_INCREMENT(offset, 2, bound);

    /*  frame max (long)         */
    proto_tree_add_item(args_tree, hf_amqp_method_connection_tune_ok_frame_max,
        tvb, offset, 4, FALSE);
    AMQP_INCREMENT(offset, 4, bound);

    /*  heartbeat (short)        */
    proto_tree_add_item(args_tree, hf_amqp_method_connection_tune_ok_heartbeat,
        tvb, offset, 2, FALSE);
    AMQP_INCREMENT(offset, 2, bound);

    return offset;
}

/*  Dissection routine for method Connection.Open                         */

static size_t
dissect_amqp_method_connection_open(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  virtual host (shortstr)  */
    proto_tree_add_item(args_tree, hf_amqp_method_connection_open_virtual_host,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  capabilities (shortstr)  */
    proto_tree_add_item(args_tree, hf_amqp_method_connection_open_capabilities,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  insist (bit)             */
    proto_tree_add_item(args_tree, hf_amqp_method_connection_open_insist,
        tvb, offset, 1, FALSE);

    return offset;
}

/*  Dissection routine for method Connection.Open-Ok                      */

static size_t
dissect_amqp_method_connection_open_ok(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  known hosts (shortstr)   */
    proto_tree_add_item(args_tree, hf_amqp_method_connection_open_ok_known_hosts,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    return offset;
}

/*  Dissection routine for method Connection.Redirect                     */

static size_t
dissect_amqp_method_connection_redirect(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  host (shortstr)          */
    proto_tree_add_item(args_tree, hf_amqp_method_connection_redirect_host,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  known hosts (shortstr)   */
    proto_tree_add_item(args_tree, hf_amqp_method_connection_redirect_known_hosts,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    return offset;
}

/*  Dissection routine for method Connection.Close                        */

static size_t
dissect_amqp_method_connection_close(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  reply code (short)       */
    proto_tree_add_item(args_tree, hf_amqp_method_connection_close_reply_code,
        tvb, offset, 2, FALSE);
    AMQP_INCREMENT(offset, 2, bound);

    /*  reply text (shortstr)    */
    proto_tree_add_item(args_tree, hf_amqp_method_connection_close_reply_text,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  class id (short)         */
    proto_tree_add_item(args_tree, hf_amqp_method_connection_close_class_id,
        tvb, offset, 2, FALSE);
    AMQP_INCREMENT(offset, 2, bound);

    /*  method id (short)        */
    proto_tree_add_item(args_tree, hf_amqp_method_connection_close_method_id,
        tvb, offset, 2, FALSE);
    AMQP_INCREMENT(offset, 2, bound);

    return offset;
}

/*  Dissection routine for method Connection.Close-Ok                     */

static size_t
dissect_amqp_method_connection_close_ok(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    return offset;
}

/*  Dissection routine for method Channel.Open                            */

static size_t
dissect_amqp_method_channel_open(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  out of band (shortstr)   */
    proto_tree_add_item(args_tree, hf_amqp_method_channel_open_out_of_band,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    return offset;
}

/*  Dissection routine for method Channel.Open-Ok                         */

static size_t
dissect_amqp_method_channel_open_ok(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    return offset;
}

/*  Dissection routine for method Channel.Flow                            */

static size_t
dissect_amqp_method_channel_flow(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  active (bit)             */
    proto_tree_add_item(args_tree, hf_amqp_method_channel_flow_active,
        tvb, offset, 1, FALSE);

    return offset;
}

/*  Dissection routine for method Channel.Flow-Ok                         */

static size_t
dissect_amqp_method_channel_flow_ok(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  active (bit)             */
    proto_tree_add_item(args_tree, hf_amqp_method_channel_flow_ok_active,
        tvb, offset, 1, FALSE);

    return offset;
}

/*  Dissection routine for method Channel.Alert                           */

static size_t
dissect_amqp_method_channel_alert(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    proto_item *ti;
    /*  reply code (short)       */
    proto_tree_add_item(args_tree, hf_amqp_method_channel_alert_reply_code,
        tvb, offset, 2, FALSE);
    AMQP_INCREMENT(offset, 2, bound);

    /*  reply text (shortstr)    */
    proto_tree_add_item(args_tree, hf_amqp_method_channel_alert_reply_text,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  details (table)          */
    ti = proto_tree_add_item(
        args_tree, hf_amqp_method_channel_alert_details,
        tvb, offset + 4, tvb_get_ntohl(tvb, offset), FALSE);
    dissect_amqp_field_table (tvb, offset + 4,
        offset + 4 + tvb_get_ntohl(tvb, offset), tvb_get_ntohl(tvb, offset), ti);
    AMQP_INCREMENT(offset, 4 + tvb_get_ntohl(tvb, offset), bound);

    return offset;
}

/*  Dissection routine for method Channel.Close                           */

static size_t
dissect_amqp_method_channel_close(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  reply code (short)       */
    proto_tree_add_item(args_tree, hf_amqp_method_channel_close_reply_code,
        tvb, offset, 2, FALSE);
    AMQP_INCREMENT(offset, 2, bound);

    /*  reply text (shortstr)    */
    proto_tree_add_item(args_tree, hf_amqp_method_channel_close_reply_text,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  class id (short)         */
    proto_tree_add_item(args_tree, hf_amqp_method_channel_close_class_id,
        tvb, offset, 2, FALSE);
    AMQP_INCREMENT(offset, 2, bound);

    /*  method id (short)        */
    proto_tree_add_item(args_tree, hf_amqp_method_channel_close_method_id,
        tvb, offset, 2, FALSE);
    AMQP_INCREMENT(offset, 2, bound);

    return offset;
}

/*  Dissection routine for method Channel.Close-Ok                        */

static size_t
dissect_amqp_method_channel_close_ok(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    return offset;
}

/*  Dissection routine for method Access.Request                          */

static size_t
dissect_amqp_method_access_request(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  realm (shortstr)         */
    proto_tree_add_item(args_tree, hf_amqp_method_access_request_realm,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  exclusive (bit)          */
    proto_tree_add_item(args_tree, hf_amqp_method_access_request_exclusive,
        tvb, offset, 1, FALSE);

    /*  passive (bit)            */
    proto_tree_add_item(args_tree, hf_amqp_method_access_request_passive,
        tvb, offset, 1, FALSE);

    /*  active (bit)             */
    proto_tree_add_item(args_tree, hf_amqp_method_access_request_active,
        tvb, offset, 1, FALSE);

    /*  write (bit)              */
    proto_tree_add_item(args_tree, hf_amqp_method_access_request_write,
        tvb, offset, 1, FALSE);

    /*  read (bit)               */
    proto_tree_add_item(args_tree, hf_amqp_method_access_request_read,
        tvb, offset, 1, FALSE);

    return offset;
}

/*  Dissection routine for method Access.Request-Ok                       */

static size_t
dissect_amqp_method_access_request_ok(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  ticket (short)           */
    proto_tree_add_item(args_tree, hf_amqp_method_access_request_ok_ticket,
        tvb, offset, 2, FALSE);
    AMQP_INCREMENT(offset, 2, bound);

    return offset;
}

/*  Dissection routine for method Exchange.Declare                        */

static size_t
dissect_amqp_method_exchange_declare(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    proto_item *ti;
    /*  ticket (short)           */
    proto_tree_add_item(args_tree, hf_amqp_method_exchange_declare_ticket,
        tvb, offset, 2, FALSE);
    AMQP_INCREMENT(offset, 2, bound);

    /*  exchange (shortstr)      */
    proto_tree_add_item(args_tree, hf_amqp_method_exchange_declare_exchange,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  type (shortstr)          */
    proto_tree_add_item(args_tree, hf_amqp_method_exchange_declare_type,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  passive (bit)            */
    proto_tree_add_item(args_tree, hf_amqp_method_exchange_declare_passive,
        tvb, offset, 1, FALSE);

    /*  durable (bit)            */
    proto_tree_add_item(args_tree, hf_amqp_method_exchange_declare_durable,
        tvb, offset, 1, FALSE);

    /*  auto delete (bit)        */
    proto_tree_add_item(args_tree, hf_amqp_method_exchange_declare_auto_delete,
        tvb, offset, 1, FALSE);

    /*  internal (bit)           */
    proto_tree_add_item(args_tree, hf_amqp_method_exchange_declare_internal,
        tvb, offset, 1, FALSE);

    /*  nowait (bit)             */
    proto_tree_add_item(args_tree, hf_amqp_method_exchange_declare_nowait,
        tvb, offset, 1, FALSE);

    AMQP_INCREMENT(offset, 1, bound);
    /*  arguments (table)        */
    ti = proto_tree_add_item(
        args_tree, hf_amqp_method_exchange_declare_arguments,
        tvb, offset + 4, tvb_get_ntohl(tvb, offset), FALSE);
    dissect_amqp_field_table (tvb, offset + 4,
        offset + 4 + tvb_get_ntohl(tvb, offset), tvb_get_ntohl(tvb, offset), ti);
    AMQP_INCREMENT(offset, 4 + tvb_get_ntohl(tvb, offset), bound);

    return offset;
}

/*  Dissection routine for method Exchange.Declare-Ok                     */

static size_t
dissect_amqp_method_exchange_declare_ok(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    return offset;
}

/*  Dissection routine for method Exchange.Delete                         */

static size_t
dissect_amqp_method_exchange_delete(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  ticket (short)           */
    proto_tree_add_item(args_tree, hf_amqp_method_exchange_delete_ticket,
        tvb, offset, 2, FALSE);
    AMQP_INCREMENT(offset, 2, bound);

    /*  exchange (shortstr)      */
    proto_tree_add_item(args_tree, hf_amqp_method_exchange_delete_exchange,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  if unused (bit)          */
    proto_tree_add_item(args_tree, hf_amqp_method_exchange_delete_if_unused,
        tvb, offset, 1, FALSE);

    /*  nowait (bit)             */
    proto_tree_add_item(args_tree, hf_amqp_method_exchange_delete_nowait,
        tvb, offset, 1, FALSE);

    return offset;
}

/*  Dissection routine for method Exchange.Delete-Ok                      */

static size_t
dissect_amqp_method_exchange_delete_ok(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    return offset;
}

/*  Dissection routine for method Queue.Declare                           */

static size_t
dissect_amqp_method_queue_declare(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    proto_item *ti;
    /*  ticket (short)           */
    proto_tree_add_item(args_tree, hf_amqp_method_queue_declare_ticket,
        tvb, offset, 2, FALSE);
    AMQP_INCREMENT(offset, 2, bound);

    /*  queue (shortstr)         */
    proto_tree_add_item(args_tree, hf_amqp_method_queue_declare_queue,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  passive (bit)            */
    proto_tree_add_item(args_tree, hf_amqp_method_queue_declare_passive,
        tvb, offset, 1, FALSE);

    /*  durable (bit)            */
    proto_tree_add_item(args_tree, hf_amqp_method_queue_declare_durable,
        tvb, offset, 1, FALSE);

    /*  exclusive (bit)          */
    proto_tree_add_item(args_tree, hf_amqp_method_queue_declare_exclusive,
        tvb, offset, 1, FALSE);

    /*  auto delete (bit)        */
    proto_tree_add_item(args_tree, hf_amqp_method_queue_declare_auto_delete,
        tvb, offset, 1, FALSE);

    /*  nowait (bit)             */
    proto_tree_add_item(args_tree, hf_amqp_method_queue_declare_nowait,
        tvb, offset, 1, FALSE);

    AMQP_INCREMENT(offset, 1, bound);
    /*  arguments (table)        */
    ti = proto_tree_add_item(
        args_tree, hf_amqp_method_queue_declare_arguments,
        tvb, offset + 4, tvb_get_ntohl(tvb, offset), FALSE);
    dissect_amqp_field_table (tvb, offset + 4,
        offset + 4 + tvb_get_ntohl(tvb, offset), tvb_get_ntohl(tvb, offset), ti);
    AMQP_INCREMENT(offset, 4 + tvb_get_ntohl(tvb, offset), bound);

    return offset;
}

/*  Dissection routine for method Queue.Declare-Ok                        */

static size_t
dissect_amqp_method_queue_declare_ok(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  queue (shortstr)         */
    proto_tree_add_item(args_tree, hf_amqp_method_queue_declare_ok_queue,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  message count (long)     */
    proto_tree_add_item(args_tree, hf_amqp_method_queue_declare_ok_message_count,
        tvb, offset, 4, FALSE);
    AMQP_INCREMENT(offset, 4, bound);

    /*  consumer count (long)    */
    proto_tree_add_item(args_tree, hf_amqp_method_queue_declare_ok_consumer_count,
        tvb, offset, 4, FALSE);
    AMQP_INCREMENT(offset, 4, bound);

    return offset;
}

/*  Dissection routine for method Queue.Bind                              */

static size_t
dissect_amqp_method_queue_bind(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    proto_item *ti;
    /*  ticket (short)           */
    proto_tree_add_item(args_tree, hf_amqp_method_queue_bind_ticket,
        tvb, offset, 2, FALSE);
    AMQP_INCREMENT(offset, 2, bound);

    /*  queue (shortstr)         */
    proto_tree_add_item(args_tree, hf_amqp_method_queue_bind_queue,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  exchange (shortstr)      */
    proto_tree_add_item(args_tree, hf_amqp_method_queue_bind_exchange,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  routing key (shortstr)   */
    proto_tree_add_item(args_tree, hf_amqp_method_queue_bind_routing_key,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  nowait (bit)             */
    proto_tree_add_item(args_tree, hf_amqp_method_queue_bind_nowait,
        tvb, offset, 1, FALSE);

    AMQP_INCREMENT(offset, 1, bound);
    /*  arguments (table)        */
    ti = proto_tree_add_item(
        args_tree, hf_amqp_method_queue_bind_arguments,
        tvb, offset + 4, tvb_get_ntohl(tvb, offset), FALSE);
    dissect_amqp_field_table (tvb, offset + 4,
        offset + 4 + tvb_get_ntohl(tvb, offset), tvb_get_ntohl(tvb, offset), ti);
    AMQP_INCREMENT(offset, 4 + tvb_get_ntohl(tvb, offset), bound);

    return offset;
}

/*  Dissection routine for method Queue.Bind-Ok                           */

static size_t
dissect_amqp_method_queue_bind_ok(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    return offset;
}

/*  Dissection routine for method Queue.Purge                             */

static size_t
dissect_amqp_method_queue_purge(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  ticket (short)           */
    proto_tree_add_item(args_tree, hf_amqp_method_queue_purge_ticket,
        tvb, offset, 2, FALSE);
    AMQP_INCREMENT(offset, 2, bound);

    /*  queue (shortstr)         */
    proto_tree_add_item(args_tree, hf_amqp_method_queue_purge_queue,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  nowait (bit)             */
    proto_tree_add_item(args_tree, hf_amqp_method_queue_purge_nowait,
        tvb, offset, 1, FALSE);

    return offset;
}

/*  Dissection routine for method Queue.Purge-Ok                          */

static size_t
dissect_amqp_method_queue_purge_ok(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  message count (long)     */
    proto_tree_add_item(args_tree, hf_amqp_method_queue_purge_ok_message_count,
        tvb, offset, 4, FALSE);
    AMQP_INCREMENT(offset, 4, bound);

    return offset;
}

/*  Dissection routine for method Queue.Delete                            */

static size_t
dissect_amqp_method_queue_delete(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  ticket (short)           */
    proto_tree_add_item(args_tree, hf_amqp_method_queue_delete_ticket,
        tvb, offset, 2, FALSE);
    AMQP_INCREMENT(offset, 2, bound);

    /*  queue (shortstr)         */
    proto_tree_add_item(args_tree, hf_amqp_method_queue_delete_queue,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  if unused (bit)          */
    proto_tree_add_item(args_tree, hf_amqp_method_queue_delete_if_unused,
        tvb, offset, 1, FALSE);

    /*  if empty (bit)           */
    proto_tree_add_item(args_tree, hf_amqp_method_queue_delete_if_empty,
        tvb, offset, 1, FALSE);

    /*  nowait (bit)             */
    proto_tree_add_item(args_tree, hf_amqp_method_queue_delete_nowait,
        tvb, offset, 1, FALSE);

    return offset;
}

/*  Dissection routine for method Queue.Delete-Ok                         */

static size_t
dissect_amqp_method_queue_delete_ok(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  message count (long)     */
    proto_tree_add_item(args_tree, hf_amqp_method_queue_delete_ok_message_count,
        tvb, offset, 4, FALSE);
    AMQP_INCREMENT(offset, 4, bound);

    return offset;
}

/*  Dissection routine for method Basic.Qos                               */

static size_t
dissect_amqp_method_basic_qos(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  prefetch size (long)     */
    proto_tree_add_item(args_tree, hf_amqp_method_basic_qos_prefetch_size,
        tvb, offset, 4, FALSE);
    AMQP_INCREMENT(offset, 4, bound);

    /*  prefetch count (short)   */
    proto_tree_add_item(args_tree, hf_amqp_method_basic_qos_prefetch_count,
        tvb, offset, 2, FALSE);
    AMQP_INCREMENT(offset, 2, bound);

    /*  global (bit)             */
    proto_tree_add_item(args_tree, hf_amqp_method_basic_qos_global,
        tvb, offset, 1, FALSE);

    return offset;
}

/*  Dissection routine for method Basic.Qos-Ok                            */

static size_t
dissect_amqp_method_basic_qos_ok(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    return offset;
}

/*  Dissection routine for method Basic.Consume                           */

static size_t
dissect_amqp_method_basic_consume(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  ticket (short)           */
    proto_tree_add_item(args_tree, hf_amqp_method_basic_consume_ticket,
        tvb, offset, 2, FALSE);
    AMQP_INCREMENT(offset, 2, bound);

    /*  queue (shortstr)         */
    proto_tree_add_item(args_tree, hf_amqp_method_basic_consume_queue,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  consumer tag (shortstr)  */
    proto_tree_add_item(args_tree, hf_amqp_method_basic_consume_consumer_tag,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  no local (bit)           */
    proto_tree_add_item(args_tree, hf_amqp_method_basic_consume_no_local,
        tvb, offset, 1, FALSE);

    /*  no ack (bit)             */
    proto_tree_add_item(args_tree, hf_amqp_method_basic_consume_no_ack,
        tvb, offset, 1, FALSE);

    /*  exclusive (bit)          */
    proto_tree_add_item(args_tree, hf_amqp_method_basic_consume_exclusive,
        tvb, offset, 1, FALSE);

    /*  nowait (bit)             */
    proto_tree_add_item(args_tree, hf_amqp_method_basic_consume_nowait,
        tvb, offset, 1, FALSE);

    return offset;
}

/*  Dissection routine for method Basic.Consume-Ok                        */

static size_t
dissect_amqp_method_basic_consume_ok(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  consumer tag (shortstr)  */
    proto_tree_add_item(args_tree, hf_amqp_method_basic_consume_ok_consumer_tag,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    return offset;
}

/*  Dissection routine for method Basic.Cancel                            */

static size_t
dissect_amqp_method_basic_cancel(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  consumer tag (shortstr)  */
    proto_tree_add_item(args_tree, hf_amqp_method_basic_cancel_consumer_tag,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  nowait (bit)             */
    proto_tree_add_item(args_tree, hf_amqp_method_basic_cancel_nowait,
        tvb, offset, 1, FALSE);

    return offset;
}

/*  Dissection routine for method Basic.Cancel-Ok                         */

static size_t
dissect_amqp_method_basic_cancel_ok(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  consumer tag (shortstr)  */
    proto_tree_add_item(args_tree, hf_amqp_method_basic_cancel_ok_consumer_tag,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    return offset;
}

/*  Dissection routine for method Basic.Publish                           */

static size_t
dissect_amqp_method_basic_publish(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  ticket (short)           */
    proto_tree_add_item(args_tree, hf_amqp_method_basic_publish_ticket,
        tvb, offset, 2, FALSE);
    AMQP_INCREMENT(offset, 2, bound);

    /*  exchange (shortstr)      */
    proto_tree_add_item(args_tree, hf_amqp_method_basic_publish_exchange,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  routing key (shortstr)   */
    proto_tree_add_item(args_tree, hf_amqp_method_basic_publish_routing_key,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  mandatory (bit)          */
    proto_tree_add_item(args_tree, hf_amqp_method_basic_publish_mandatory,
        tvb, offset, 1, FALSE);

    /*  immediate (bit)          */
    proto_tree_add_item(args_tree, hf_amqp_method_basic_publish_immediate,
        tvb, offset, 1, FALSE);

    return offset;
}

/*  Dissection routine for method Basic.Return                            */

static size_t
dissect_amqp_method_basic_return(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  reply code (short)       */
    proto_tree_add_item(args_tree, hf_amqp_method_basic_return_reply_code,
        tvb, offset, 2, FALSE);
    AMQP_INCREMENT(offset, 2, bound);

    /*  reply text (shortstr)    */
    proto_tree_add_item(args_tree, hf_amqp_method_basic_return_reply_text,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  exchange (shortstr)      */
    proto_tree_add_item(args_tree, hf_amqp_method_basic_return_exchange,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  routing key (shortstr)   */
    proto_tree_add_item(args_tree, hf_amqp_method_basic_return_routing_key,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    return offset;
}

/*  Dissection routine for method Basic.Deliver                           */

static size_t
dissect_amqp_method_basic_deliver(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  consumer tag (shortstr)  */
    proto_tree_add_item(args_tree, hf_amqp_method_basic_deliver_consumer_tag,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  delivery tag (longlong)  */
    proto_tree_add_item(args_tree, hf_amqp_method_basic_deliver_delivery_tag,
        tvb, offset, 8, FALSE);
    AMQP_INCREMENT(offset, 8, bound);

    /*  redelivered (bit)        */
    proto_tree_add_item(args_tree, hf_amqp_method_basic_deliver_redelivered,
        tvb, offset, 1, FALSE);

    AMQP_INCREMENT(offset, 1, bound);
    /*  exchange (shortstr)      */
    proto_tree_add_item(args_tree, hf_amqp_method_basic_deliver_exchange,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  routing key (shortstr)   */
    proto_tree_add_item(args_tree, hf_amqp_method_basic_deliver_routing_key,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    return offset;
}

/*  Dissection routine for method Basic.Get                               */

static size_t
dissect_amqp_method_basic_get(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  ticket (short)           */
    proto_tree_add_item(args_tree, hf_amqp_method_basic_get_ticket,
        tvb, offset, 2, FALSE);
    AMQP_INCREMENT(offset, 2, bound);

    /*  queue (shortstr)         */
    proto_tree_add_item(args_tree, hf_amqp_method_basic_get_queue,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  no ack (bit)             */
    proto_tree_add_item(args_tree, hf_amqp_method_basic_get_no_ack,
        tvb, offset, 1, FALSE);

    return offset;
}

/*  Dissection routine for method Basic.Get-Ok                            */

static size_t
dissect_amqp_method_basic_get_ok(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  delivery tag (longlong)  */
    proto_tree_add_item(args_tree, hf_amqp_method_basic_get_ok_delivery_tag,
        tvb, offset, 8, FALSE);
    AMQP_INCREMENT(offset, 8, bound);

    /*  redelivered (bit)        */
    proto_tree_add_item(args_tree, hf_amqp_method_basic_get_ok_redelivered,
        tvb, offset, 1, FALSE);

    AMQP_INCREMENT(offset, 1, bound);
    /*  exchange (shortstr)      */
    proto_tree_add_item(args_tree, hf_amqp_method_basic_get_ok_exchange,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  routing key (shortstr)   */
    proto_tree_add_item(args_tree, hf_amqp_method_basic_get_ok_routing_key,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  message count (long)     */
    proto_tree_add_item(args_tree, hf_amqp_method_basic_get_ok_message_count,
        tvb, offset, 4, FALSE);
    AMQP_INCREMENT(offset, 4, bound);

    return offset;
}

/*  Dissection routine for method Basic.Get-Empty                         */

static size_t
dissect_amqp_method_basic_get_empty(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  cluster id (shortstr)    */
    proto_tree_add_item(args_tree, hf_amqp_method_basic_get_empty_cluster_id,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    return offset;
}

/*  Dissection routine for method Basic.Ack                               */

static size_t
dissect_amqp_method_basic_ack(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  delivery tag (longlong)  */
    proto_tree_add_item(args_tree, hf_amqp_method_basic_ack_delivery_tag,
        tvb, offset, 8, FALSE);
    AMQP_INCREMENT(offset, 8, bound);

    /*  multiple (bit)           */
    proto_tree_add_item(args_tree, hf_amqp_method_basic_ack_multiple,
        tvb, offset, 1, FALSE);

    return offset;
}

/*  Dissection routine for method Basic.Reject                            */

static size_t
dissect_amqp_method_basic_reject(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  delivery tag (longlong)  */
    proto_tree_add_item(args_tree, hf_amqp_method_basic_reject_delivery_tag,
        tvb, offset, 8, FALSE);
    AMQP_INCREMENT(offset, 8, bound);

    /*  requeue (bit)            */
    proto_tree_add_item(args_tree, hf_amqp_method_basic_reject_requeue,
        tvb, offset, 1, FALSE);

    return offset;
}

/*  Dissection routine for method Basic.Recover                           */

static size_t
dissect_amqp_method_basic_recover(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  requeue (bit)            */
    proto_tree_add_item(args_tree, hf_amqp_method_basic_recover_requeue,
        tvb, offset, 1, FALSE);

    return offset;
}

/*  Dissection routine for method File.Qos                                */

static size_t
dissect_amqp_method_file_qos(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  prefetch size (long)     */
    proto_tree_add_item(args_tree, hf_amqp_method_file_qos_prefetch_size,
        tvb, offset, 4, FALSE);
    AMQP_INCREMENT(offset, 4, bound);

    /*  prefetch count (short)   */
    proto_tree_add_item(args_tree, hf_amqp_method_file_qos_prefetch_count,
        tvb, offset, 2, FALSE);
    AMQP_INCREMENT(offset, 2, bound);

    /*  global (bit)             */
    proto_tree_add_item(args_tree, hf_amqp_method_file_qos_global,
        tvb, offset, 1, FALSE);

    return offset;
}

/*  Dissection routine for method File.Qos-Ok                             */

static size_t
dissect_amqp_method_file_qos_ok(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    return offset;
}

/*  Dissection routine for method File.Consume                            */

static size_t
dissect_amqp_method_file_consume(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  ticket (short)           */
    proto_tree_add_item(args_tree, hf_amqp_method_file_consume_ticket,
        tvb, offset, 2, FALSE);
    AMQP_INCREMENT(offset, 2, bound);

    /*  queue (shortstr)         */
    proto_tree_add_item(args_tree, hf_amqp_method_file_consume_queue,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  consumer tag (shortstr)  */
    proto_tree_add_item(args_tree, hf_amqp_method_file_consume_consumer_tag,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  no local (bit)           */
    proto_tree_add_item(args_tree, hf_amqp_method_file_consume_no_local,
        tvb, offset, 1, FALSE);

    /*  no ack (bit)             */
    proto_tree_add_item(args_tree, hf_amqp_method_file_consume_no_ack,
        tvb, offset, 1, FALSE);

    /*  exclusive (bit)          */
    proto_tree_add_item(args_tree, hf_amqp_method_file_consume_exclusive,
        tvb, offset, 1, FALSE);

    /*  nowait (bit)             */
    proto_tree_add_item(args_tree, hf_amqp_method_file_consume_nowait,
        tvb, offset, 1, FALSE);

    return offset;
}

/*  Dissection routine for method File.Consume-Ok                         */

static size_t
dissect_amqp_method_file_consume_ok(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  consumer tag (shortstr)  */
    proto_tree_add_item(args_tree, hf_amqp_method_file_consume_ok_consumer_tag,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    return offset;
}

/*  Dissection routine for method File.Cancel                             */

static size_t
dissect_amqp_method_file_cancel(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  consumer tag (shortstr)  */
    proto_tree_add_item(args_tree, hf_amqp_method_file_cancel_consumer_tag,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  nowait (bit)             */
    proto_tree_add_item(args_tree, hf_amqp_method_file_cancel_nowait,
        tvb, offset, 1, FALSE);

    return offset;
}

/*  Dissection routine for method File.Cancel-Ok                          */

static size_t
dissect_amqp_method_file_cancel_ok(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  consumer tag (shortstr)  */
    proto_tree_add_item(args_tree, hf_amqp_method_file_cancel_ok_consumer_tag,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    return offset;
}

/*  Dissection routine for method File.Open                               */

static size_t
dissect_amqp_method_file_open(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  identifier (shortstr)    */
    proto_tree_add_item(args_tree, hf_amqp_method_file_open_identifier,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  content size (longlong)  */
    proto_tree_add_item(args_tree, hf_amqp_method_file_open_content_size,
        tvb, offset, 8, FALSE);
    AMQP_INCREMENT(offset, 8, bound);

    return offset;
}

/*  Dissection routine for method File.Open-Ok                            */

static size_t
dissect_amqp_method_file_open_ok(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  staged size (longlong)   */
    proto_tree_add_item(args_tree, hf_amqp_method_file_open_ok_staged_size,
        tvb, offset, 8, FALSE);
    AMQP_INCREMENT(offset, 8, bound);

    return offset;
}

/*  Dissection routine for method File.Stage                              */

static size_t
dissect_amqp_method_file_stage(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    return offset;
}

/*  Dissection routine for method File.Publish                            */

static size_t
dissect_amqp_method_file_publish(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  ticket (short)           */
    proto_tree_add_item(args_tree, hf_amqp_method_file_publish_ticket,
        tvb, offset, 2, FALSE);
    AMQP_INCREMENT(offset, 2, bound);

    /*  exchange (shortstr)      */
    proto_tree_add_item(args_tree, hf_amqp_method_file_publish_exchange,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  routing key (shortstr)   */
    proto_tree_add_item(args_tree, hf_amqp_method_file_publish_routing_key,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  mandatory (bit)          */
    proto_tree_add_item(args_tree, hf_amqp_method_file_publish_mandatory,
        tvb, offset, 1, FALSE);

    /*  immediate (bit)          */
    proto_tree_add_item(args_tree, hf_amqp_method_file_publish_immediate,
        tvb, offset, 1, FALSE);

    AMQP_INCREMENT(offset, 1, bound);
    /*  identifier (shortstr)    */
    proto_tree_add_item(args_tree, hf_amqp_method_file_publish_identifier,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    return offset;
}

/*  Dissection routine for method File.Return                             */

static size_t
dissect_amqp_method_file_return(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  reply code (short)       */
    proto_tree_add_item(args_tree, hf_amqp_method_file_return_reply_code,
        tvb, offset, 2, FALSE);
    AMQP_INCREMENT(offset, 2, bound);

    /*  reply text (shortstr)    */
    proto_tree_add_item(args_tree, hf_amqp_method_file_return_reply_text,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  exchange (shortstr)      */
    proto_tree_add_item(args_tree, hf_amqp_method_file_return_exchange,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  routing key (shortstr)   */
    proto_tree_add_item(args_tree, hf_amqp_method_file_return_routing_key,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    return offset;
}

/*  Dissection routine for method File.Deliver                            */

static size_t
dissect_amqp_method_file_deliver(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  consumer tag (shortstr)  */
    proto_tree_add_item(args_tree, hf_amqp_method_file_deliver_consumer_tag,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  delivery tag (longlong)  */
    proto_tree_add_item(args_tree, hf_amqp_method_file_deliver_delivery_tag,
        tvb, offset, 8, FALSE);
    AMQP_INCREMENT(offset, 8, bound);

    /*  redelivered (bit)        */
    proto_tree_add_item(args_tree, hf_amqp_method_file_deliver_redelivered,
        tvb, offset, 1, FALSE);

    AMQP_INCREMENT(offset, 1, bound);
    /*  exchange (shortstr)      */
    proto_tree_add_item(args_tree, hf_amqp_method_file_deliver_exchange,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  routing key (shortstr)   */
    proto_tree_add_item(args_tree, hf_amqp_method_file_deliver_routing_key,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  identifier (shortstr)    */
    proto_tree_add_item(args_tree, hf_amqp_method_file_deliver_identifier,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    return offset;
}

/*  Dissection routine for method File.Ack                                */

static size_t
dissect_amqp_method_file_ack(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  delivery tag (longlong)  */
    proto_tree_add_item(args_tree, hf_amqp_method_file_ack_delivery_tag,
        tvb, offset, 8, FALSE);
    AMQP_INCREMENT(offset, 8, bound);

    /*  multiple (bit)           */
    proto_tree_add_item(args_tree, hf_amqp_method_file_ack_multiple,
        tvb, offset, 1, FALSE);

    return offset;
}

/*  Dissection routine for method File.Reject                             */

static size_t
dissect_amqp_method_file_reject(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  delivery tag (longlong)  */
    proto_tree_add_item(args_tree, hf_amqp_method_file_reject_delivery_tag,
        tvb, offset, 8, FALSE);
    AMQP_INCREMENT(offset, 8, bound);

    /*  requeue (bit)            */
    proto_tree_add_item(args_tree, hf_amqp_method_file_reject_requeue,
        tvb, offset, 1, FALSE);

    return offset;
}

/*  Dissection routine for method Stream.Qos                              */

static size_t
dissect_amqp_method_stream_qos(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  prefetch size (long)     */
    proto_tree_add_item(args_tree, hf_amqp_method_stream_qos_prefetch_size,
        tvb, offset, 4, FALSE);
    AMQP_INCREMENT(offset, 4, bound);

    /*  prefetch count (short)   */
    proto_tree_add_item(args_tree, hf_amqp_method_stream_qos_prefetch_count,
        tvb, offset, 2, FALSE);
    AMQP_INCREMENT(offset, 2, bound);

    /*  consume rate (long)      */
    proto_tree_add_item(args_tree, hf_amqp_method_stream_qos_consume_rate,
        tvb, offset, 4, FALSE);
    AMQP_INCREMENT(offset, 4, bound);

    /*  global (bit)             */
    proto_tree_add_item(args_tree, hf_amqp_method_stream_qos_global,
        tvb, offset, 1, FALSE);

    return offset;
}

/*  Dissection routine for method Stream.Qos-Ok                           */

static size_t
dissect_amqp_method_stream_qos_ok(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    return offset;
}

/*  Dissection routine for method Stream.Consume                          */

static size_t
dissect_amqp_method_stream_consume(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  ticket (short)           */
    proto_tree_add_item(args_tree, hf_amqp_method_stream_consume_ticket,
        tvb, offset, 2, FALSE);
    AMQP_INCREMENT(offset, 2, bound);

    /*  queue (shortstr)         */
    proto_tree_add_item(args_tree, hf_amqp_method_stream_consume_queue,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  consumer tag (shortstr)  */
    proto_tree_add_item(args_tree, hf_amqp_method_stream_consume_consumer_tag,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  no local (bit)           */
    proto_tree_add_item(args_tree, hf_amqp_method_stream_consume_no_local,
        tvb, offset, 1, FALSE);

    /*  exclusive (bit)          */
    proto_tree_add_item(args_tree, hf_amqp_method_stream_consume_exclusive,
        tvb, offset, 1, FALSE);

    /*  nowait (bit)             */
    proto_tree_add_item(args_tree, hf_amqp_method_stream_consume_nowait,
        tvb, offset, 1, FALSE);

    return offset;
}

/*  Dissection routine for method Stream.Consume-Ok                       */

static size_t
dissect_amqp_method_stream_consume_ok(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  consumer tag (shortstr)  */
    proto_tree_add_item(args_tree, hf_amqp_method_stream_consume_ok_consumer_tag,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    return offset;
}

/*  Dissection routine for method Stream.Cancel                           */

static size_t
dissect_amqp_method_stream_cancel(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  consumer tag (shortstr)  */
    proto_tree_add_item(args_tree, hf_amqp_method_stream_cancel_consumer_tag,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  nowait (bit)             */
    proto_tree_add_item(args_tree, hf_amqp_method_stream_cancel_nowait,
        tvb, offset, 1, FALSE);

    return offset;
}

/*  Dissection routine for method Stream.Cancel-Ok                        */

static size_t
dissect_amqp_method_stream_cancel_ok(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  consumer tag (shortstr)  */
    proto_tree_add_item(args_tree, hf_amqp_method_stream_cancel_ok_consumer_tag,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    return offset;
}

/*  Dissection routine for method Stream.Publish                          */

static size_t
dissect_amqp_method_stream_publish(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  ticket (short)           */
    proto_tree_add_item(args_tree, hf_amqp_method_stream_publish_ticket,
        tvb, offset, 2, FALSE);
    AMQP_INCREMENT(offset, 2, bound);

    /*  exchange (shortstr)      */
    proto_tree_add_item(args_tree, hf_amqp_method_stream_publish_exchange,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  routing key (shortstr)   */
    proto_tree_add_item(args_tree, hf_amqp_method_stream_publish_routing_key,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  mandatory (bit)          */
    proto_tree_add_item(args_tree, hf_amqp_method_stream_publish_mandatory,
        tvb, offset, 1, FALSE);

    /*  immediate (bit)          */
    proto_tree_add_item(args_tree, hf_amqp_method_stream_publish_immediate,
        tvb, offset, 1, FALSE);

    return offset;
}

/*  Dissection routine for method Stream.Return                           */

static size_t
dissect_amqp_method_stream_return(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  reply code (short)       */
    proto_tree_add_item(args_tree, hf_amqp_method_stream_return_reply_code,
        tvb, offset, 2, FALSE);
    AMQP_INCREMENT(offset, 2, bound);

    /*  reply text (shortstr)    */
    proto_tree_add_item(args_tree, hf_amqp_method_stream_return_reply_text,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  exchange (shortstr)      */
    proto_tree_add_item(args_tree, hf_amqp_method_stream_return_exchange,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  routing key (shortstr)   */
    proto_tree_add_item(args_tree, hf_amqp_method_stream_return_routing_key,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    return offset;
}

/*  Dissection routine for method Stream.Deliver                          */

static size_t
dissect_amqp_method_stream_deliver(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  consumer tag (shortstr)  */
    proto_tree_add_item(args_tree, hf_amqp_method_stream_deliver_consumer_tag,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  delivery tag (longlong)  */
    proto_tree_add_item(args_tree, hf_amqp_method_stream_deliver_delivery_tag,
        tvb, offset, 8, FALSE);
    AMQP_INCREMENT(offset, 8, bound);

    /*  exchange (shortstr)      */
    proto_tree_add_item(args_tree, hf_amqp_method_stream_deliver_exchange,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  queue (shortstr)         */
    proto_tree_add_item(args_tree, hf_amqp_method_stream_deliver_queue,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    return offset;
}

/*  Dissection routine for method Tx.Select                               */

static size_t
dissect_amqp_method_tx_select(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    return offset;
}

/*  Dissection routine for method Tx.Select-Ok                            */

static size_t
dissect_amqp_method_tx_select_ok(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    return offset;
}

/*  Dissection routine for method Tx.Commit                               */

static size_t
dissect_amqp_method_tx_commit(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    return offset;
}

/*  Dissection routine for method Tx.Commit-Ok                            */

static size_t
dissect_amqp_method_tx_commit_ok(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    return offset;
}

/*  Dissection routine for method Tx.Rollback                             */

static size_t
dissect_amqp_method_tx_rollback(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    return offset;
}

/*  Dissection routine for method Tx.Rollback-Ok                          */

static size_t
dissect_amqp_method_tx_rollback_ok(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    return offset;
}

/*  Dissection routine for method Dtx.Select                              */

static size_t
dissect_amqp_method_dtx_select(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    return offset;
}

/*  Dissection routine for method Dtx.Select-Ok                           */

static size_t
dissect_amqp_method_dtx_select_ok(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    return offset;
}

/*  Dissection routine for method Dtx.Start                               */

static size_t
dissect_amqp_method_dtx_start(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  dtx identifier (shortstr)  */
    proto_tree_add_item(args_tree, hf_amqp_method_dtx_start_dtx_identifier,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    return offset;
}

/*  Dissection routine for method Dtx.Start-Ok                            */

static size_t
dissect_amqp_method_dtx_start_ok(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    return offset;
}

/*  Dissection routine for method Tunnel.Request                          */

static size_t
dissect_amqp_method_tunnel_request(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    proto_item *ti;
    /*  meta data (table)        */
    ti = proto_tree_add_item(
        args_tree, hf_amqp_method_tunnel_request_meta_data,
        tvb, offset + 4, tvb_get_ntohl(tvb, offset), FALSE);
    dissect_amqp_field_table (tvb, offset + 4,
        offset + 4 + tvb_get_ntohl(tvb, offset), tvb_get_ntohl(tvb, offset), ti);
    AMQP_INCREMENT(offset, 4 + tvb_get_ntohl(tvb, offset), bound);

    return offset;
}

/*  Dissection routine for method Test.Integer                            */

static size_t
dissect_amqp_method_test_integer(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  integer 1 (octet)        */
    proto_tree_add_item(args_tree, hf_amqp_method_test_integer_integer_1,
        tvb, offset, 1, FALSE);
    AMQP_INCREMENT(offset, 1, bound);

    /*  integer 2 (short)        */
    proto_tree_add_item(args_tree, hf_amqp_method_test_integer_integer_2,
        tvb, offset, 2, FALSE);
    AMQP_INCREMENT(offset, 2, bound);

    /*  integer 3 (long)         */
    proto_tree_add_item(args_tree, hf_amqp_method_test_integer_integer_3,
        tvb, offset, 4, FALSE);
    AMQP_INCREMENT(offset, 4, bound);

    /*  integer 4 (longlong)     */
    proto_tree_add_item(args_tree, hf_amqp_method_test_integer_integer_4,
        tvb, offset, 8, FALSE);
    AMQP_INCREMENT(offset, 8, bound);

    /*  operation (octet)        */
    proto_tree_add_item(args_tree, hf_amqp_method_test_integer_operation,
        tvb, offset, 1, FALSE);
    AMQP_INCREMENT(offset, 1, bound);

    return offset;
}

/*  Dissection routine for method Test.Integer-Ok                         */

static size_t
dissect_amqp_method_test_integer_ok(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  result (longlong)        */
    proto_tree_add_item(args_tree, hf_amqp_method_test_integer_ok_result,
        tvb, offset, 8, FALSE);
    AMQP_INCREMENT(offset, 8, bound);

    return offset;
}

/*  Dissection routine for method Test.String                             */

static size_t
dissect_amqp_method_test_string(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  string 1 (shortstr)      */
    proto_tree_add_item(args_tree, hf_amqp_method_test_string_string_1,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    /*  string 2 (longstr)       */
    proto_tree_add_item(args_tree, hf_amqp_method_test_string_string_2,
        tvb, offset + 4, tvb_get_ntohl(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 4 + tvb_get_ntohl(tvb, offset), bound);

    /*  operation (octet)        */
    proto_tree_add_item(args_tree, hf_amqp_method_test_string_operation,
        tvb, offset, 1, FALSE);
    AMQP_INCREMENT(offset, 1, bound);

    return offset;
}

/*  Dissection routine for method Test.String-Ok                          */

static size_t
dissect_amqp_method_test_string_ok(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  result (longstr)         */
    proto_tree_add_item(args_tree, hf_amqp_method_test_string_ok_result,
        tvb, offset + 4, tvb_get_ntohl(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 4 + tvb_get_ntohl(tvb, offset), bound);

    return offset;
}

/*  Dissection routine for method Test.Table                              */

static size_t
dissect_amqp_method_test_table(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    proto_item *ti;
    /*  table (table)            */
    ti = proto_tree_add_item(
        args_tree, hf_amqp_method_test_table_table,
        tvb, offset + 4, tvb_get_ntohl(tvb, offset), FALSE);
    dissect_amqp_field_table (tvb, offset + 4,
        offset + 4 + tvb_get_ntohl(tvb, offset), tvb_get_ntohl(tvb, offset), ti);
    AMQP_INCREMENT(offset, 4 + tvb_get_ntohl(tvb, offset), bound);

    /*  integer op (octet)       */
    proto_tree_add_item(args_tree, hf_amqp_method_test_table_integer_op,
        tvb, offset, 1, FALSE);
    AMQP_INCREMENT(offset, 1, bound);

    /*  string op (octet)        */
    proto_tree_add_item(args_tree, hf_amqp_method_test_table_string_op,
        tvb, offset, 1, FALSE);
    AMQP_INCREMENT(offset, 1, bound);

    return offset;
}

/*  Dissection routine for method Test.Table-Ok                           */

static size_t
dissect_amqp_method_test_table_ok(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  integer result (longlong)  */
    proto_tree_add_item(args_tree, hf_amqp_method_test_table_ok_integer_result,
        tvb, offset, 8, FALSE);
    AMQP_INCREMENT(offset, 8, bound);

    /*  string result (longstr)  */
    proto_tree_add_item(args_tree, hf_amqp_method_test_table_ok_string_result,
        tvb, offset + 4, tvb_get_ntohl(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 4 + tvb_get_ntohl(tvb, offset), bound);

    return offset;
}

/*  Dissection routine for method Test.Content                            */

static size_t
dissect_amqp_method_test_content(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    return offset;
}

/*  Dissection routine for method Test.Content-Ok                         */

static size_t
dissect_amqp_method_test_content_ok(tvbuff_t *tvb _U_,
    int offset _U_, int bound _U_, proto_tree *args_tree _U_)
{
    /*  content checksum (long)  */
    proto_tree_add_item(args_tree, hf_amqp_method_test_content_ok_content_checksum,
        tvb, offset, 4, FALSE);
    AMQP_INCREMENT(offset, 4, bound);

    return offset;
}


/*  Dissection routine for content headers of class basic          */

static size_t
dissect_amqp_content_header_basic(tvbuff_t *tvb,
    int offset, int bound, proto_tree *prop_tree)
{
    proto_item *ti;
    guint16 prop_flags;

    prop_flags = tvb_get_ntohs(tvb, 19);
    if (prop_flags & 0x8000) {
    /*  content type (shortstr)  */
    proto_tree_add_item(prop_tree, hf_amqp_header_basic_content_type,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    }
    prop_flags <<= 1;
    if (prop_flags & 0x8000) {
    /*  content encoding (shortstr)  */
    proto_tree_add_item(prop_tree, hf_amqp_header_basic_content_encoding,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    }
    prop_flags <<= 1;
    if (prop_flags & 0x8000) {
    /*  headers (table)          */
    ti = proto_tree_add_item(
        prop_tree, hf_amqp_header_basic_headers,
        tvb, offset + 4, tvb_get_ntohl(tvb, offset), FALSE);
    dissect_amqp_field_table (tvb, offset + 4,
        offset + 4 + tvb_get_ntohl(tvb, offset), tvb_get_ntohl(tvb, offset), ti);
    AMQP_INCREMENT(offset, 4 + tvb_get_ntohl(tvb, offset), bound);

    }
    prop_flags <<= 1;
    if (prop_flags & 0x8000) {
    /*  delivery mode (octet)    */
    proto_tree_add_item(prop_tree, hf_amqp_header_basic_delivery_mode,
        tvb, offset, 1, FALSE);
    AMQP_INCREMENT(offset, 1, bound);

    }
    prop_flags <<= 1;
    if (prop_flags & 0x8000) {
    /*  priority (octet)         */
    proto_tree_add_item(prop_tree, hf_amqp_header_basic_priority,
        tvb, offset, 1, FALSE);
    AMQP_INCREMENT(offset, 1, bound);

    }
    prop_flags <<= 1;
    if (prop_flags & 0x8000) {
    /*  correlation id (shortstr)  */
    proto_tree_add_item(prop_tree, hf_amqp_header_basic_correlation_id,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    }
    prop_flags <<= 1;
    if (prop_flags & 0x8000) {
    /*  reply to (shortstr)      */
    proto_tree_add_item(prop_tree, hf_amqp_header_basic_reply_to,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    }
    prop_flags <<= 1;
    if (prop_flags & 0x8000) {
    /*  expiration (shortstr)    */
    proto_tree_add_item(prop_tree, hf_amqp_header_basic_expiration,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    }
    prop_flags <<= 1;
    if (prop_flags & 0x8000) {
    /*  message id (shortstr)    */
    proto_tree_add_item(prop_tree, hf_amqp_header_basic_message_id,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    }
    prop_flags <<= 1;
    if (prop_flags & 0x8000) {
    /*  timestamp (timestamp)    */
    proto_tree_add_item(prop_tree, hf_amqp_header_basic_timestamp,
        tvb, offset, 8, FALSE);
    AMQP_INCREMENT(offset, 8, bound);

    }
    prop_flags <<= 1;
    if (prop_flags & 0x8000) {
    /*  type (shortstr)          */
    proto_tree_add_item(prop_tree, hf_amqp_header_basic_type,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    }
    prop_flags <<= 1;
    if (prop_flags & 0x8000) {
    /*  user id (shortstr)       */
    proto_tree_add_item(prop_tree, hf_amqp_header_basic_user_id,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    }
    prop_flags <<= 1;
    if (prop_flags & 0x8000) {
    /*  app id (shortstr)        */
    proto_tree_add_item(prop_tree, hf_amqp_header_basic_app_id,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    }
    prop_flags <<= 1;
    if (prop_flags & 0x8000) {
    /*  cluster id (shortstr)    */
    proto_tree_add_item(prop_tree, hf_amqp_header_basic_cluster_id,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    }
    prop_flags <<= 1;

    return offset;
}
/*  Dissection routine for content headers of class file           */

static size_t
dissect_amqp_content_header_file(tvbuff_t *tvb,
    int offset, int bound, proto_tree *prop_tree)
{
    proto_item *ti;
    guint16 prop_flags;

    prop_flags = tvb_get_ntohs(tvb, 19);
    if (prop_flags & 0x8000) {
    /*  content type (shortstr)  */
    proto_tree_add_item(prop_tree, hf_amqp_header_file_content_type,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    }
    prop_flags <<= 1;
    if (prop_flags & 0x8000) {
    /*  content encoding (shortstr)  */
    proto_tree_add_item(prop_tree, hf_amqp_header_file_content_encoding,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    }
    prop_flags <<= 1;
    if (prop_flags & 0x8000) {
    /*  headers (table)          */
    ti = proto_tree_add_item(
        prop_tree, hf_amqp_header_file_headers,
        tvb, offset + 4, tvb_get_ntohl(tvb, offset), FALSE);
    dissect_amqp_field_table (tvb, offset + 4,
        offset + 4 + tvb_get_ntohl(tvb, offset), tvb_get_ntohl(tvb, offset), ti);
    AMQP_INCREMENT(offset, 4 + tvb_get_ntohl(tvb, offset), bound);

    }
    prop_flags <<= 1;
    if (prop_flags & 0x8000) {
    /*  priority (octet)         */
    proto_tree_add_item(prop_tree, hf_amqp_header_file_priority,
        tvb, offset, 1, FALSE);
    AMQP_INCREMENT(offset, 1, bound);

    }
    prop_flags <<= 1;
    if (prop_flags & 0x8000) {
    /*  reply to (shortstr)      */
    proto_tree_add_item(prop_tree, hf_amqp_header_file_reply_to,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    }
    prop_flags <<= 1;
    if (prop_flags & 0x8000) {
    /*  message id (shortstr)    */
    proto_tree_add_item(prop_tree, hf_amqp_header_file_message_id,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    }
    prop_flags <<= 1;
    if (prop_flags & 0x8000) {
    /*  filename (shortstr)      */
    proto_tree_add_item(prop_tree, hf_amqp_header_file_filename,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    }
    prop_flags <<= 1;
    if (prop_flags & 0x8000) {
    /*  timestamp (timestamp)    */
    proto_tree_add_item(prop_tree, hf_amqp_header_file_timestamp,
        tvb, offset, 8, FALSE);
    AMQP_INCREMENT(offset, 8, bound);

    }
    prop_flags <<= 1;
    if (prop_flags & 0x8000) {
    /*  cluster id (shortstr)    */
    proto_tree_add_item(prop_tree, hf_amqp_header_file_cluster_id,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    }
    prop_flags <<= 1;

    return offset;
}
/*  Dissection routine for content headers of class stream         */

static size_t
dissect_amqp_content_header_stream(tvbuff_t *tvb,
    int offset, int bound, proto_tree *prop_tree)
{
    proto_item *ti;
    guint16 prop_flags;

    prop_flags = tvb_get_ntohs(tvb, 19);
    if (prop_flags & 0x8000) {
    /*  content type (shortstr)  */
    proto_tree_add_item(prop_tree, hf_amqp_header_stream_content_type,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    }
    prop_flags <<= 1;
    if (prop_flags & 0x8000) {
    /*  content encoding (shortstr)  */
    proto_tree_add_item(prop_tree, hf_amqp_header_stream_content_encoding,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    }
    prop_flags <<= 1;
    if (prop_flags & 0x8000) {
    /*  headers (table)          */
    ti = proto_tree_add_item(
        prop_tree, hf_amqp_header_stream_headers,
        tvb, offset + 4, tvb_get_ntohl(tvb, offset), FALSE);
    dissect_amqp_field_table (tvb, offset + 4,
        offset + 4 + tvb_get_ntohl(tvb, offset), tvb_get_ntohl(tvb, offset), ti);
    AMQP_INCREMENT(offset, 4 + tvb_get_ntohl(tvb, offset), bound);

    }
    prop_flags <<= 1;
    if (prop_flags & 0x8000) {
    /*  priority (octet)         */
    proto_tree_add_item(prop_tree, hf_amqp_header_stream_priority,
        tvb, offset, 1, FALSE);
    AMQP_INCREMENT(offset, 1, bound);

    }
    prop_flags <<= 1;
    if (prop_flags & 0x8000) {
    /*  timestamp (timestamp)    */
    proto_tree_add_item(prop_tree, hf_amqp_header_stream_timestamp,
        tvb, offset, 8, FALSE);
    AMQP_INCREMENT(offset, 8, bound);

    }
    prop_flags <<= 1;

    return offset;
}
/*  Dissection routine for content headers of class tunnel         */

static size_t
dissect_amqp_content_header_tunnel(tvbuff_t *tvb,
    int offset, int bound, proto_tree *prop_tree)
{
    proto_item *ti;
    guint16 prop_flags;

    prop_flags = tvb_get_ntohs(tvb, 19);
    if (prop_flags & 0x8000) {
    /*  headers (table)          */
    ti = proto_tree_add_item(
        prop_tree, hf_amqp_header_tunnel_headers,
        tvb, offset + 4, tvb_get_ntohl(tvb, offset), FALSE);
    dissect_amqp_field_table (tvb, offset + 4,
        offset + 4 + tvb_get_ntohl(tvb, offset), tvb_get_ntohl(tvb, offset), ti);
    AMQP_INCREMENT(offset, 4 + tvb_get_ntohl(tvb, offset), bound);

    }
    prop_flags <<= 1;
    if (prop_flags & 0x8000) {
    /*  proxy name (shortstr)    */
    proto_tree_add_item(prop_tree, hf_amqp_header_tunnel_proxy_name,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    }
    prop_flags <<= 1;
    if (prop_flags & 0x8000) {
    /*  data name (shortstr)     */
    proto_tree_add_item(prop_tree, hf_amqp_header_tunnel_data_name,
        tvb, offset + 1, tvb_get_guint8(tvb, offset), FALSE);
    AMQP_INCREMENT(offset, 1 + tvb_get_guint8(tvb, offset), bound);

    }
    prop_flags <<= 1;
    if (prop_flags & 0x8000) {
    /*  durable (octet)          */
    proto_tree_add_item(prop_tree, hf_amqp_header_tunnel_durable,
        tvb, offset, 1, FALSE);
    AMQP_INCREMENT(offset, 1, bound);

    }
    prop_flags <<= 1;
    if (prop_flags & 0x8000) {
    /*  broadcast (octet)        */
    proto_tree_add_item(prop_tree, hf_amqp_header_tunnel_broadcast,
        tvb, offset, 1, FALSE);
    AMQP_INCREMENT(offset, 1, bound);

    }
    prop_flags <<= 1;

    return offset;
}

/*  Setup of field format array  */

static hf_register_info hf[] = {
    {&hf_amqp_type, {
        "Type", "amqp.type",
        FT_UINT8, BASE_DEC, VALS(amqp_frame_types), 0x0,
        "Frame type", HFILL}},
    {&hf_amqp_channel,{
        "Channel", "amqp.channel",
        FT_UINT16, BASE_DEC, NULL, 0x0,
        "Channel ID", HFILL}},
    {&hf_amqp_length, {
        "Length", "amqp.length",
        FT_UINT32, BASE_DEC, NULL, 0x0,
        "Length of the frame", HFILL}},
    {&hf_amqp_method_class_id, {
        "Class", "amqp.method.class",
        FT_UINT16, BASE_DEC, VALS(amqp_method_classes), 0x0,
        "Class ID", HFILL}},
    {&hf_amqp_method_connection_method_id, {
        "Method", "amqp.method.method",
        FT_UINT16, BASE_DEC, VALS(amqp_method_connection_methods), 0x0,
        "Method ID", HFILL}},
    {&hf_amqp_method_channel_method_id, {
        "Method", "amqp.method.method",
        FT_UINT16, BASE_DEC, VALS(amqp_method_channel_methods), 0x0,
        "Method ID", HFILL}},
    {&hf_amqp_method_access_method_id, {
        "Method", "amqp.method.method",
        FT_UINT16, BASE_DEC, VALS(amqp_method_access_methods), 0x0,
        "Method ID", HFILL}},
    {&hf_amqp_method_exchange_method_id, {
        "Method", "amqp.method.method",
        FT_UINT16, BASE_DEC, VALS(amqp_method_exchange_methods), 0x0,
        "Method ID", HFILL}},
    {&hf_amqp_method_queue_method_id, {
        "Method", "amqp.method.method",
        FT_UINT16, BASE_DEC, VALS(amqp_method_queue_methods), 0x0,
        "Method ID", HFILL}},
    {&hf_amqp_method_basic_method_id, {
        "Method", "amqp.method.method",
        FT_UINT16, BASE_DEC, VALS(amqp_method_basic_methods), 0x0,
        "Method ID", HFILL}},
    {&hf_amqp_method_file_method_id, {
        "Method", "amqp.method.method",
        FT_UINT16, BASE_DEC, VALS(amqp_method_file_methods), 0x0,
        "Method ID", HFILL}},
    {&hf_amqp_method_stream_method_id, {
        "Method", "amqp.method.method",
        FT_UINT16, BASE_DEC, VALS(amqp_method_stream_methods), 0x0,
        "Method ID", HFILL}},
    {&hf_amqp_method_tx_method_id, {
        "Method", "amqp.method.method",
        FT_UINT16, BASE_DEC, VALS(amqp_method_tx_methods), 0x0,
        "Method ID", HFILL}},
    {&hf_amqp_method_dtx_method_id, {
        "Method", "amqp.method.method",
        FT_UINT16, BASE_DEC, VALS(amqp_method_dtx_methods), 0x0,
        "Method ID", HFILL}},
    {&hf_amqp_method_tunnel_method_id, {
        "Method", "amqp.method.method",
        FT_UINT16, BASE_DEC, VALS(amqp_method_tunnel_methods), 0x0,
        "Method ID", HFILL}},
    {&hf_amqp_method_test_method_id, {
        "Method", "amqp.method.method",
        FT_UINT16, BASE_DEC, VALS(amqp_method_test_methods), 0x0,
        "Method ID", HFILL}},
    {&hf_amqp_method_arguments, {
        "Arguments", "amqp.method.arguments",
        FT_NONE, BASE_NONE, NULL, 0x0,
        "Method arguments", HFILL}},
    {&hf_amqp_method_connection_start_version_major, {
        "Version Major", "amqp.method.arguments.version_major",
        FT_UINT8, BASE_DEC, NULL, 0,
        "version major", HFILL}},
    {&hf_amqp_method_connection_start_version_minor, {
        "Version Minor", "amqp.method.arguments.version_minor",
        FT_UINT8, BASE_DEC, NULL, 0,
        "version minor", HFILL}},
    {&hf_amqp_method_connection_start_server_properties, {
        "Server Properties", "amqp.method.arguments.server_properties",
        FT_NONE, BASE_NONE, NULL, 0,
        "server properties", HFILL}},
    {&hf_amqp_method_connection_start_mechanisms, {
        "Mechanisms", "amqp.method.arguments.mechanisms",
        FT_BYTES, BASE_NONE, NULL, 0,
        "mechanisms", HFILL}},
    {&hf_amqp_method_connection_start_locales, {
        "Locales", "amqp.method.arguments.locales",
        FT_BYTES, BASE_NONE, NULL, 0,
        "locales", HFILL}},
    {&hf_amqp_method_connection_start_ok_client_properties, {
        "Client Properties", "amqp.method.arguments.client_properties",
        FT_NONE, BASE_NONE, NULL, 0,
        "client properties", HFILL}},
    {&hf_amqp_method_connection_start_ok_mechanism, {
        "Mechanism", "amqp.method.arguments.mechanism",
        FT_STRING, BASE_NONE, NULL, 0,
        "mechanism", HFILL}},
    {&hf_amqp_method_connection_start_ok_response, {
        "Response", "amqp.method.arguments.response",
        FT_BYTES, BASE_NONE, NULL, 0,
        "response", HFILL}},
    {&hf_amqp_method_connection_start_ok_locale, {
        "Locale", "amqp.method.arguments.locale",
        FT_STRING, BASE_NONE, NULL, 0,
        "locale", HFILL}},
    {&hf_amqp_method_connection_secure_challenge, {
        "Challenge", "amqp.method.arguments.challenge",
        FT_BYTES, BASE_NONE, NULL, 0,
        "challenge", HFILL}},
    {&hf_amqp_method_connection_secure_ok_response, {
        "Response", "amqp.method.arguments.response",
        FT_BYTES, BASE_NONE, NULL, 0,
        "response", HFILL}},
    {&hf_amqp_method_connection_tune_channel_max, {
        "Channel Max", "amqp.method.arguments.channel_max",
         FT_UINT16, BASE_DEC, NULL, 0,
        "channel max", HFILL}},
    {&hf_amqp_method_connection_tune_frame_max, {
        "Frame Max", "amqp.method.arguments.frame_max",
        FT_UINT32, BASE_DEC, NULL, 0,
        "frame max", HFILL}},
    {&hf_amqp_method_connection_tune_heartbeat, {
        "Heartbeat", "amqp.method.arguments.heartbeat",
         FT_UINT16, BASE_DEC, NULL, 0,
        "heartbeat", HFILL}},
    {&hf_amqp_method_connection_tune_ok_channel_max, {
        "Channel Max", "amqp.method.arguments.channel_max",
         FT_UINT16, BASE_DEC, NULL, 0,
        "channel max", HFILL}},
    {&hf_amqp_method_connection_tune_ok_frame_max, {
        "Frame Max", "amqp.method.arguments.frame_max",
        FT_UINT32, BASE_DEC, NULL, 0,
        "frame max", HFILL}},
    {&hf_amqp_method_connection_tune_ok_heartbeat, {
        "Heartbeat", "amqp.method.arguments.heartbeat",
         FT_UINT16, BASE_DEC, NULL, 0,
        "heartbeat", HFILL}},
    {&hf_amqp_method_connection_open_virtual_host, {
        "Virtual Host", "amqp.method.arguments.virtual_host",
        FT_STRING, BASE_NONE, NULL, 0,
        "virtual host", HFILL}},
    {&hf_amqp_method_connection_open_capabilities, {
        "Capabilities", "amqp.method.arguments.capabilities",
        FT_STRING, BASE_NONE, NULL, 0,
        "capabilities", HFILL}},
    {&hf_amqp_method_connection_open_insist, {
        "Insist", "amqp.method.arguments.insist",
        FT_BOOLEAN, 8, NULL, 0x01,
        "insist", HFILL}},
    {&hf_amqp_method_connection_open_ok_known_hosts, {
        "Known Hosts", "amqp.method.arguments.known_hosts",
        FT_STRING, BASE_NONE, NULL, 0,
        "known hosts", HFILL}},
    {&hf_amqp_method_connection_redirect_host, {
        "Host", "amqp.method.arguments.host",
        FT_STRING, BASE_NONE, NULL, 0,
        "host", HFILL}},
    {&hf_amqp_method_connection_redirect_known_hosts, {
        "Known Hosts", "amqp.method.arguments.known_hosts",
        FT_STRING, BASE_NONE, NULL, 0,
        "known hosts", HFILL}},
    {&hf_amqp_method_connection_close_reply_code, {
        "Reply Code", "amqp.method.arguments.reply_code",
         FT_UINT16, BASE_DEC, NULL, 0,
        "reply code", HFILL}},
    {&hf_amqp_method_connection_close_reply_text, {
        "Reply Text", "amqp.method.arguments.reply_text",
        FT_STRING, BASE_NONE, NULL, 0,
        "reply text", HFILL}},
    {&hf_amqp_method_connection_close_class_id, {
        "Class Id", "amqp.method.arguments.class_id",
         FT_UINT16, BASE_DEC, NULL, 0,
        "class id", HFILL}},
    {&hf_amqp_method_connection_close_method_id, {
        "Method Id", "amqp.method.arguments.method_id",
         FT_UINT16, BASE_DEC, NULL, 0,
        "method id", HFILL}},
    {&hf_amqp_method_channel_open_out_of_band, {
        "Out Of Band", "amqp.method.arguments.out_of_band",
        FT_STRING, BASE_NONE, NULL, 0,
        "out of band", HFILL}},
    {&hf_amqp_method_channel_flow_active, {
        "Active", "amqp.method.arguments.active",
        FT_BOOLEAN, 8, NULL, 0x01,
        "active", HFILL}},
    {&hf_amqp_method_channel_flow_ok_active, {
        "Active", "amqp.method.arguments.active",
        FT_BOOLEAN, 8, NULL, 0x01,
        "active", HFILL}},
    {&hf_amqp_method_channel_alert_reply_code, {
        "Reply Code", "amqp.method.arguments.reply_code",
         FT_UINT16, BASE_DEC, NULL, 0,
        "reply code", HFILL}},
    {&hf_amqp_method_channel_alert_reply_text, {
        "Reply Text", "amqp.method.arguments.reply_text",
        FT_STRING, BASE_NONE, NULL, 0,
        "reply text", HFILL}},
    {&hf_amqp_method_channel_alert_details, {
        "Details", "amqp.method.arguments.details",
        FT_NONE, BASE_NONE, NULL, 0,
        "details", HFILL}},
    {&hf_amqp_method_channel_close_reply_code, {
        "Reply Code", "amqp.method.arguments.reply_code",
         FT_UINT16, BASE_DEC, NULL, 0,
        "reply code", HFILL}},
    {&hf_amqp_method_channel_close_reply_text, {
        "Reply Text", "amqp.method.arguments.reply_text",
        FT_STRING, BASE_NONE, NULL, 0,
        "reply text", HFILL}},
    {&hf_amqp_method_channel_close_class_id, {
        "Class Id", "amqp.method.arguments.class_id",
         FT_UINT16, BASE_DEC, NULL, 0,
        "class id", HFILL}},
    {&hf_amqp_method_channel_close_method_id, {
        "Method Id", "amqp.method.arguments.method_id",
         FT_UINT16, BASE_DEC, NULL, 0,
        "method id", HFILL}},
    {&hf_amqp_method_access_request_realm, {
        "Realm", "amqp.method.arguments.realm",
        FT_STRING, BASE_NONE, NULL, 0,
        "realm", HFILL}},
    {&hf_amqp_method_access_request_exclusive, {
        "Exclusive", "amqp.method.arguments.exclusive",
        FT_BOOLEAN, 8, NULL, 0x01,
        "exclusive", HFILL}},
    {&hf_amqp_method_access_request_passive, {
        "Passive", "amqp.method.arguments.passive",
        FT_BOOLEAN, 8, NULL, 0x02,
        "passive", HFILL}},
    {&hf_amqp_method_access_request_active, {
        "Active", "amqp.method.arguments.active",
        FT_BOOLEAN, 8, NULL, 0x04,
        "active", HFILL}},
    {&hf_amqp_method_access_request_write, {
        "Write", "amqp.method.arguments.write",
        FT_BOOLEAN, 8, NULL, 0x08,
        "write", HFILL}},
    {&hf_amqp_method_access_request_read, {
        "Read", "amqp.method.arguments.read",
        FT_BOOLEAN, 8, NULL, 0x10,
        "read", HFILL}},
    {&hf_amqp_method_access_request_ok_ticket, {
        "Ticket", "amqp.method.arguments.ticket",
         FT_UINT16, BASE_DEC, NULL, 0,
        "ticket", HFILL}},
    {&hf_amqp_method_exchange_declare_ticket, {
        "Ticket", "amqp.method.arguments.ticket",
         FT_UINT16, BASE_DEC, NULL, 0,
        "ticket", HFILL}},
    {&hf_amqp_method_exchange_declare_exchange, {
        "Exchange", "amqp.method.arguments.exchange",
        FT_STRING, BASE_NONE, NULL, 0,
        "exchange", HFILL}},
    {&hf_amqp_method_exchange_declare_type, {
        "Type", "amqp.method.arguments.type",
        FT_STRING, BASE_NONE, NULL, 0,
        "type", HFILL}},
    {&hf_amqp_method_exchange_declare_passive, {
        "Passive", "amqp.method.arguments.passive",
        FT_BOOLEAN, 8, NULL, 0x01,
        "passive", HFILL}},
    {&hf_amqp_method_exchange_declare_durable, {
        "Durable", "amqp.method.arguments.durable",
        FT_BOOLEAN, 8, NULL, 0x02,
        "durable", HFILL}},
    {&hf_amqp_method_exchange_declare_auto_delete, {
        "Auto Delete", "amqp.method.arguments.auto_delete",
        FT_BOOLEAN, 8, NULL, 0x04,
        "auto delete", HFILL}},
    {&hf_amqp_method_exchange_declare_internal, {
        "Internal", "amqp.method.arguments.internal",
        FT_BOOLEAN, 8, NULL, 0x08,
        "internal", HFILL}},
    {&hf_amqp_method_exchange_declare_nowait, {
        "Nowait", "amqp.method.arguments.nowait",
        FT_BOOLEAN, 8, NULL, 0x10,
        "nowait", HFILL}},
    {&hf_amqp_method_exchange_declare_arguments, {
        "Arguments", "amqp.method.arguments.arguments",
        FT_NONE, BASE_NONE, NULL, 0,
        "arguments", HFILL}},
    {&hf_amqp_method_exchange_delete_ticket, {
        "Ticket", "amqp.method.arguments.ticket",
         FT_UINT16, BASE_DEC, NULL, 0,
        "ticket", HFILL}},
    {&hf_amqp_method_exchange_delete_exchange, {
        "Exchange", "amqp.method.arguments.exchange",
        FT_STRING, BASE_NONE, NULL, 0,
        "exchange", HFILL}},
    {&hf_amqp_method_exchange_delete_if_unused, {
        "If Unused", "amqp.method.arguments.if_unused",
        FT_BOOLEAN, 8, NULL, 0x01,
        "if unused", HFILL}},
    {&hf_amqp_method_exchange_delete_nowait, {
        "Nowait", "amqp.method.arguments.nowait",
        FT_BOOLEAN, 8, NULL, 0x02,
        "nowait", HFILL}},
    {&hf_amqp_method_queue_declare_ticket, {
        "Ticket", "amqp.method.arguments.ticket",
         FT_UINT16, BASE_DEC, NULL, 0,
        "ticket", HFILL}},
    {&hf_amqp_method_queue_declare_queue, {
        "Queue", "amqp.method.arguments.queue",
        FT_STRING, BASE_NONE, NULL, 0,
        "queue", HFILL}},
    {&hf_amqp_method_queue_declare_passive, {
        "Passive", "amqp.method.arguments.passive",
        FT_BOOLEAN, 8, NULL, 0x01,
        "passive", HFILL}},
    {&hf_amqp_method_queue_declare_durable, {
        "Durable", "amqp.method.arguments.durable",
        FT_BOOLEAN, 8, NULL, 0x02,
        "durable", HFILL}},
    {&hf_amqp_method_queue_declare_exclusive, {
        "Exclusive", "amqp.method.arguments.exclusive",
        FT_BOOLEAN, 8, NULL, 0x04,
        "exclusive", HFILL}},
    {&hf_amqp_method_queue_declare_auto_delete, {
        "Auto Delete", "amqp.method.arguments.auto_delete",
        FT_BOOLEAN, 8, NULL, 0x08,
        "auto delete", HFILL}},
    {&hf_amqp_method_queue_declare_nowait, {
        "Nowait", "amqp.method.arguments.nowait",
        FT_BOOLEAN, 8, NULL, 0x10,
        "nowait", HFILL}},
    {&hf_amqp_method_queue_declare_arguments, {
        "Arguments", "amqp.method.arguments.arguments",
        FT_NONE, BASE_NONE, NULL, 0,
        "arguments", HFILL}},
    {&hf_amqp_method_queue_declare_ok_queue, {
        "Queue", "amqp.method.arguments.queue",
        FT_STRING, BASE_NONE, NULL, 0,
        "queue", HFILL}},
    {&hf_amqp_method_queue_declare_ok_message_count, {
        "Message Count", "amqp.method.arguments.message_count",
        FT_UINT32, BASE_DEC, NULL, 0,
        "message count", HFILL}},
    {&hf_amqp_method_queue_declare_ok_consumer_count, {
        "Consumer Count", "amqp.method.arguments.consumer_count",
        FT_UINT32, BASE_DEC, NULL, 0,
        "consumer count", HFILL}},
    {&hf_amqp_method_queue_bind_ticket, {
        "Ticket", "amqp.method.arguments.ticket",
         FT_UINT16, BASE_DEC, NULL, 0,
        "ticket", HFILL}},
    {&hf_amqp_method_queue_bind_queue, {
        "Queue", "amqp.method.arguments.queue",
        FT_STRING, BASE_NONE, NULL, 0,
        "queue", HFILL}},
    {&hf_amqp_method_queue_bind_exchange, {
        "Exchange", "amqp.method.arguments.exchange",
        FT_STRING, BASE_NONE, NULL, 0,
        "exchange", HFILL}},
    {&hf_amqp_method_queue_bind_routing_key, {
        "Routing Key", "amqp.method.arguments.routing_key",
        FT_STRING, BASE_NONE, NULL, 0,
        "routing key", HFILL}},
    {&hf_amqp_method_queue_bind_nowait, {
        "Nowait", "amqp.method.arguments.nowait",
        FT_BOOLEAN, 8, NULL, 0x01,
        "nowait", HFILL}},
    {&hf_amqp_method_queue_bind_arguments, {
        "Arguments", "amqp.method.arguments.arguments",
        FT_NONE, BASE_NONE, NULL, 0,
        "arguments", HFILL}},
    {&hf_amqp_method_queue_purge_ticket, {
        "Ticket", "amqp.method.arguments.ticket",
         FT_UINT16, BASE_DEC, NULL, 0,
        "ticket", HFILL}},
    {&hf_amqp_method_queue_purge_queue, {
        "Queue", "amqp.method.arguments.queue",
        FT_STRING, BASE_NONE, NULL, 0,
        "queue", HFILL}},
    {&hf_amqp_method_queue_purge_nowait, {
        "Nowait", "amqp.method.arguments.nowait",
        FT_BOOLEAN, 8, NULL, 0x01,
        "nowait", HFILL}},
    {&hf_amqp_method_queue_purge_ok_message_count, {
        "Message Count", "amqp.method.arguments.message_count",
        FT_UINT32, BASE_DEC, NULL, 0,
        "message count", HFILL}},
    {&hf_amqp_method_queue_delete_ticket, {
        "Ticket", "amqp.method.arguments.ticket",
         FT_UINT16, BASE_DEC, NULL, 0,
        "ticket", HFILL}},
    {&hf_amqp_method_queue_delete_queue, {
        "Queue", "amqp.method.arguments.queue",
        FT_STRING, BASE_NONE, NULL, 0,
        "queue", HFILL}},
    {&hf_amqp_method_queue_delete_if_unused, {
        "If Unused", "amqp.method.arguments.if_unused",
        FT_BOOLEAN, 8, NULL, 0x01,
        "if unused", HFILL}},
    {&hf_amqp_method_queue_delete_if_empty, {
        "If Empty", "amqp.method.arguments.if_empty",
        FT_BOOLEAN, 8, NULL, 0x02,
        "if empty", HFILL}},
    {&hf_amqp_method_queue_delete_nowait, {
        "Nowait", "amqp.method.arguments.nowait",
        FT_BOOLEAN, 8, NULL, 0x04,
        "nowait", HFILL}},
    {&hf_amqp_method_queue_delete_ok_message_count, {
        "Message Count", "amqp.method.arguments.message_count",
        FT_UINT32, BASE_DEC, NULL, 0,
        "message count", HFILL}},
    {&hf_amqp_method_basic_qos_prefetch_size, {
        "Prefetch Size", "amqp.method.arguments.prefetch_size",
        FT_UINT32, BASE_DEC, NULL, 0,
        "prefetch size", HFILL}},
    {&hf_amqp_method_basic_qos_prefetch_count, {
        "Prefetch Count", "amqp.method.arguments.prefetch_count",
         FT_UINT16, BASE_DEC, NULL, 0,
        "prefetch count", HFILL}},
    {&hf_amqp_method_basic_qos_global, {
        "Global", "amqp.method.arguments.global",
        FT_BOOLEAN, 8, NULL, 0x01,
        "global", HFILL}},
    {&hf_amqp_method_basic_consume_ticket, {
        "Ticket", "amqp.method.arguments.ticket",
         FT_UINT16, BASE_DEC, NULL, 0,
        "ticket", HFILL}},
    {&hf_amqp_method_basic_consume_queue, {
        "Queue", "amqp.method.arguments.queue",
        FT_STRING, BASE_NONE, NULL, 0,
        "queue", HFILL}},
    {&hf_amqp_method_basic_consume_consumer_tag, {
        "Consumer Tag", "amqp.method.arguments.consumer_tag",
        FT_STRING, BASE_NONE, NULL, 0,
        "consumer tag", HFILL}},
    {&hf_amqp_method_basic_consume_no_local, {
        "No Local", "amqp.method.arguments.no_local",
        FT_BOOLEAN, 8, NULL, 0x01,
        "no local", HFILL}},
    {&hf_amqp_method_basic_consume_no_ack, {
        "No Ack", "amqp.method.arguments.no_ack",
        FT_BOOLEAN, 8, NULL, 0x02,
        "no ack", HFILL}},
    {&hf_amqp_method_basic_consume_exclusive, {
        "Exclusive", "amqp.method.arguments.exclusive",
        FT_BOOLEAN, 8, NULL, 0x04,
        "exclusive", HFILL}},
    {&hf_amqp_method_basic_consume_nowait, {
        "Nowait", "amqp.method.arguments.nowait",
        FT_BOOLEAN, 8, NULL, 0x08,
        "nowait", HFILL}},
    {&hf_amqp_method_basic_consume_ok_consumer_tag, {
        "Consumer Tag", "amqp.method.arguments.consumer_tag",
        FT_STRING, BASE_NONE, NULL, 0,
        "consumer tag", HFILL}},
    {&hf_amqp_method_basic_cancel_consumer_tag, {
        "Consumer Tag", "amqp.method.arguments.consumer_tag",
        FT_STRING, BASE_NONE, NULL, 0,
        "consumer tag", HFILL}},
    {&hf_amqp_method_basic_cancel_nowait, {
        "Nowait", "amqp.method.arguments.nowait",
        FT_BOOLEAN, 8, NULL, 0x01,
        "nowait", HFILL}},
    {&hf_amqp_method_basic_cancel_ok_consumer_tag, {
        "Consumer Tag", "amqp.method.arguments.consumer_tag",
        FT_STRING, BASE_NONE, NULL, 0,
        "consumer tag", HFILL}},
    {&hf_amqp_method_basic_publish_ticket, {
        "Ticket", "amqp.method.arguments.ticket",
         FT_UINT16, BASE_DEC, NULL, 0,
        "ticket", HFILL}},
    {&hf_amqp_method_basic_publish_exchange, {
        "Exchange", "amqp.method.arguments.exchange",
        FT_STRING, BASE_NONE, NULL, 0,
        "exchange", HFILL}},
    {&hf_amqp_method_basic_publish_routing_key, {
        "Routing Key", "amqp.method.arguments.routing_key",
        FT_STRING, BASE_NONE, NULL, 0,
        "routing key", HFILL}},
    {&hf_amqp_method_basic_publish_mandatory, {
        "Mandatory", "amqp.method.arguments.mandatory",
        FT_BOOLEAN, 8, NULL, 0x01,
        "mandatory", HFILL}},
    {&hf_amqp_method_basic_publish_immediate, {
        "Immediate", "amqp.method.arguments.immediate",
        FT_BOOLEAN, 8, NULL, 0x02,
        "immediate", HFILL}},
    {&hf_amqp_method_basic_return_reply_code, {
        "Reply Code", "amqp.method.arguments.reply_code",
         FT_UINT16, BASE_DEC, NULL, 0,
        "reply code", HFILL}},
    {&hf_amqp_method_basic_return_reply_text, {
        "Reply Text", "amqp.method.arguments.reply_text",
        FT_STRING, BASE_NONE, NULL, 0,
        "reply text", HFILL}},
    {&hf_amqp_method_basic_return_exchange, {
        "Exchange", "amqp.method.arguments.exchange",
        FT_STRING, BASE_NONE, NULL, 0,
        "exchange", HFILL}},
    {&hf_amqp_method_basic_return_routing_key, {
        "Routing Key", "amqp.method.arguments.routing_key",
        FT_STRING, BASE_NONE, NULL, 0,
        "routing key", HFILL}},
    {&hf_amqp_method_basic_deliver_consumer_tag, {
        "Consumer Tag", "amqp.method.arguments.consumer_tag",
        FT_STRING, BASE_NONE, NULL, 0,
        "consumer tag", HFILL}},
    {&hf_amqp_method_basic_deliver_delivery_tag, {
        "Delivery Tag", "amqp.method.arguments.delivery_tag",
        FT_UINT64, BASE_DEC, NULL, 0,
        "delivery tag", HFILL}},
    {&hf_amqp_method_basic_deliver_redelivered, {
        "Redelivered", "amqp.method.arguments.redelivered",
        FT_BOOLEAN, 8, NULL, 0x01,
        "redelivered", HFILL}},
    {&hf_amqp_method_basic_deliver_exchange, {
        "Exchange", "amqp.method.arguments.exchange",
        FT_STRING, BASE_NONE, NULL, 0,
        "exchange", HFILL}},
    {&hf_amqp_method_basic_deliver_routing_key, {
        "Routing Key", "amqp.method.arguments.routing_key",
        FT_STRING, BASE_NONE, NULL, 0,
        "routing key", HFILL}},
    {&hf_amqp_method_basic_get_ticket, {
        "Ticket", "amqp.method.arguments.ticket",
         FT_UINT16, BASE_DEC, NULL, 0,
        "ticket", HFILL}},
    {&hf_amqp_method_basic_get_queue, {
        "Queue", "amqp.method.arguments.queue",
        FT_STRING, BASE_NONE, NULL, 0,
        "queue", HFILL}},
    {&hf_amqp_method_basic_get_no_ack, {
        "No Ack", "amqp.method.arguments.no_ack",
        FT_BOOLEAN, 8, NULL, 0x01,
        "no ack", HFILL}},
    {&hf_amqp_method_basic_get_ok_delivery_tag, {
        "Delivery Tag", "amqp.method.arguments.delivery_tag",
        FT_UINT64, BASE_DEC, NULL, 0,
        "delivery tag", HFILL}},
    {&hf_amqp_method_basic_get_ok_redelivered, {
        "Redelivered", "amqp.method.arguments.redelivered",
        FT_BOOLEAN, 8, NULL, 0x01,
        "redelivered", HFILL}},
    {&hf_amqp_method_basic_get_ok_exchange, {
        "Exchange", "amqp.method.arguments.exchange",
        FT_STRING, BASE_NONE, NULL, 0,
        "exchange", HFILL}},
    {&hf_amqp_method_basic_get_ok_routing_key, {
        "Routing Key", "amqp.method.arguments.routing_key",
        FT_STRING, BASE_NONE, NULL, 0,
        "routing key", HFILL}},
    {&hf_amqp_method_basic_get_ok_message_count, {
        "Message Count", "amqp.method.arguments.message_count",
        FT_UINT32, BASE_DEC, NULL, 0,
        "message count", HFILL}},
    {&hf_amqp_method_basic_get_empty_cluster_id, {
        "Cluster Id", "amqp.method.arguments.cluster_id",
        FT_STRING, BASE_NONE, NULL, 0,
        "cluster id", HFILL}},
    {&hf_amqp_method_basic_ack_delivery_tag, {
        "Delivery Tag", "amqp.method.arguments.delivery_tag",
        FT_UINT64, BASE_DEC, NULL, 0,
        "delivery tag", HFILL}},
    {&hf_amqp_method_basic_ack_multiple, {
        "Multiple", "amqp.method.arguments.multiple",
        FT_BOOLEAN, 8, NULL, 0x01,
        "multiple", HFILL}},
    {&hf_amqp_method_basic_reject_delivery_tag, {
        "Delivery Tag", "amqp.method.arguments.delivery_tag",
        FT_UINT64, BASE_DEC, NULL, 0,
        "delivery tag", HFILL}},
    {&hf_amqp_method_basic_reject_requeue, {
        "Requeue", "amqp.method.arguments.requeue",
        FT_BOOLEAN, 8, NULL, 0x01,
        "requeue", HFILL}},
    {&hf_amqp_method_basic_recover_requeue, {
        "Requeue", "amqp.method.arguments.requeue",
        FT_BOOLEAN, 8, NULL, 0x01,
        "requeue", HFILL}},
    {&hf_amqp_method_file_qos_prefetch_size, {
        "Prefetch Size", "amqp.method.arguments.prefetch_size",
        FT_UINT32, BASE_DEC, NULL, 0,
        "prefetch size", HFILL}},
    {&hf_amqp_method_file_qos_prefetch_count, {
        "Prefetch Count", "amqp.method.arguments.prefetch_count",
         FT_UINT16, BASE_DEC, NULL, 0,
        "prefetch count", HFILL}},
    {&hf_amqp_method_file_qos_global, {
        "Global", "amqp.method.arguments.global",
        FT_BOOLEAN, 8, NULL, 0x01,
        "global", HFILL}},
    {&hf_amqp_method_file_consume_ticket, {
        "Ticket", "amqp.method.arguments.ticket",
         FT_UINT16, BASE_DEC, NULL, 0,
        "ticket", HFILL}},
    {&hf_amqp_method_file_consume_queue, {
        "Queue", "amqp.method.arguments.queue",
        FT_STRING, BASE_NONE, NULL, 0,
        "queue", HFILL}},
    {&hf_amqp_method_file_consume_consumer_tag, {
        "Consumer Tag", "amqp.method.arguments.consumer_tag",
        FT_STRING, BASE_NONE, NULL, 0,
        "consumer tag", HFILL}},
    {&hf_amqp_method_file_consume_no_local, {
        "No Local", "amqp.method.arguments.no_local",
        FT_BOOLEAN, 8, NULL, 0x01,
        "no local", HFILL}},
    {&hf_amqp_method_file_consume_no_ack, {
        "No Ack", "amqp.method.arguments.no_ack",
        FT_BOOLEAN, 8, NULL, 0x02,
        "no ack", HFILL}},
    {&hf_amqp_method_file_consume_exclusive, {
        "Exclusive", "amqp.method.arguments.exclusive",
        FT_BOOLEAN, 8, NULL, 0x04,
        "exclusive", HFILL}},
    {&hf_amqp_method_file_consume_nowait, {
        "Nowait", "amqp.method.arguments.nowait",
        FT_BOOLEAN, 8, NULL, 0x08,
        "nowait", HFILL}},
    {&hf_amqp_method_file_consume_ok_consumer_tag, {
        "Consumer Tag", "amqp.method.arguments.consumer_tag",
        FT_STRING, BASE_NONE, NULL, 0,
        "consumer tag", HFILL}},
    {&hf_amqp_method_file_cancel_consumer_tag, {
        "Consumer Tag", "amqp.method.arguments.consumer_tag",
        FT_STRING, BASE_NONE, NULL, 0,
        "consumer tag", HFILL}},
    {&hf_amqp_method_file_cancel_nowait, {
        "Nowait", "amqp.method.arguments.nowait",
        FT_BOOLEAN, 8, NULL, 0x01,
        "nowait", HFILL}},
    {&hf_amqp_method_file_cancel_ok_consumer_tag, {
        "Consumer Tag", "amqp.method.arguments.consumer_tag",
        FT_STRING, BASE_NONE, NULL, 0,
        "consumer tag", HFILL}},
    {&hf_amqp_method_file_open_identifier, {
        "Identifier", "amqp.method.arguments.identifier",
        FT_STRING, BASE_NONE, NULL, 0,
        "identifier", HFILL}},
    {&hf_amqp_method_file_open_content_size, {
        "Content Size", "amqp.method.arguments.content_size",
        FT_UINT64, BASE_DEC, NULL, 0,
        "content size", HFILL}},
    {&hf_amqp_method_file_open_ok_staged_size, {
        "Staged Size", "amqp.method.arguments.staged_size",
        FT_UINT64, BASE_DEC, NULL, 0,
        "staged size", HFILL}},
    {&hf_amqp_method_file_publish_ticket, {
        "Ticket", "amqp.method.arguments.ticket",
         FT_UINT16, BASE_DEC, NULL, 0,
        "ticket", HFILL}},
    {&hf_amqp_method_file_publish_exchange, {
        "Exchange", "amqp.method.arguments.exchange",
        FT_STRING, BASE_NONE, NULL, 0,
        "exchange", HFILL}},
    {&hf_amqp_method_file_publish_routing_key, {
        "Routing Key", "amqp.method.arguments.routing_key",
        FT_STRING, BASE_NONE, NULL, 0,
        "routing key", HFILL}},
    {&hf_amqp_method_file_publish_mandatory, {
        "Mandatory", "amqp.method.arguments.mandatory",
        FT_BOOLEAN, 8, NULL, 0x01,
        "mandatory", HFILL}},
    {&hf_amqp_method_file_publish_immediate, {
        "Immediate", "amqp.method.arguments.immediate",
        FT_BOOLEAN, 8, NULL, 0x02,
        "immediate", HFILL}},
    {&hf_amqp_method_file_publish_identifier, {
        "Identifier", "amqp.method.arguments.identifier",
        FT_STRING, BASE_NONE, NULL, 0,
        "identifier", HFILL}},
    {&hf_amqp_method_file_return_reply_code, {
        "Reply Code", "amqp.method.arguments.reply_code",
         FT_UINT16, BASE_DEC, NULL, 0,
        "reply code", HFILL}},
    {&hf_amqp_method_file_return_reply_text, {
        "Reply Text", "amqp.method.arguments.reply_text",
        FT_STRING, BASE_NONE, NULL, 0,
        "reply text", HFILL}},
    {&hf_amqp_method_file_return_exchange, {
        "Exchange", "amqp.method.arguments.exchange",
        FT_STRING, BASE_NONE, NULL, 0,
        "exchange", HFILL}},
    {&hf_amqp_method_file_return_routing_key, {
        "Routing Key", "amqp.method.arguments.routing_key",
        FT_STRING, BASE_NONE, NULL, 0,
        "routing key", HFILL}},
    {&hf_amqp_method_file_deliver_consumer_tag, {
        "Consumer Tag", "amqp.method.arguments.consumer_tag",
        FT_STRING, BASE_NONE, NULL, 0,
        "consumer tag", HFILL}},
    {&hf_amqp_method_file_deliver_delivery_tag, {
        "Delivery Tag", "amqp.method.arguments.delivery_tag",
        FT_UINT64, BASE_DEC, NULL, 0,
        "delivery tag", HFILL}},
    {&hf_amqp_method_file_deliver_redelivered, {
        "Redelivered", "amqp.method.arguments.redelivered",
        FT_BOOLEAN, 8, NULL, 0x01,
        "redelivered", HFILL}},
    {&hf_amqp_method_file_deliver_exchange, {
        "Exchange", "amqp.method.arguments.exchange",
        FT_STRING, BASE_NONE, NULL, 0,
        "exchange", HFILL}},
    {&hf_amqp_method_file_deliver_routing_key, {
        "Routing Key", "amqp.method.arguments.routing_key",
        FT_STRING, BASE_NONE, NULL, 0,
        "routing key", HFILL}},
    {&hf_amqp_method_file_deliver_identifier, {
        "Identifier", "amqp.method.arguments.identifier",
        FT_STRING, BASE_NONE, NULL, 0,
        "identifier", HFILL}},
    {&hf_amqp_method_file_ack_delivery_tag, {
        "Delivery Tag", "amqp.method.arguments.delivery_tag",
        FT_UINT64, BASE_DEC, NULL, 0,
        "delivery tag", HFILL}},
    {&hf_amqp_method_file_ack_multiple, {
        "Multiple", "amqp.method.arguments.multiple",
        FT_BOOLEAN, 8, NULL, 0x01,
        "multiple", HFILL}},
    {&hf_amqp_method_file_reject_delivery_tag, {
        "Delivery Tag", "amqp.method.arguments.delivery_tag",
        FT_UINT64, BASE_DEC, NULL, 0,
        "delivery tag", HFILL}},
    {&hf_amqp_method_file_reject_requeue, {
        "Requeue", "amqp.method.arguments.requeue",
        FT_BOOLEAN, 8, NULL, 0x01,
        "requeue", HFILL}},
    {&hf_amqp_method_stream_qos_prefetch_size, {
        "Prefetch Size", "amqp.method.arguments.prefetch_size",
        FT_UINT32, BASE_DEC, NULL, 0,
        "prefetch size", HFILL}},
    {&hf_amqp_method_stream_qos_prefetch_count, {
        "Prefetch Count", "amqp.method.arguments.prefetch_count",
         FT_UINT16, BASE_DEC, NULL, 0,
        "prefetch count", HFILL}},
    {&hf_amqp_method_stream_qos_consume_rate, {
        "Consume Rate", "amqp.method.arguments.consume_rate",
        FT_UINT32, BASE_DEC, NULL, 0,
        "consume rate", HFILL}},
    {&hf_amqp_method_stream_qos_global, {
        "Global", "amqp.method.arguments.global",
        FT_BOOLEAN, 8, NULL, 0x01,
        "global", HFILL}},
    {&hf_amqp_method_stream_consume_ticket, {
        "Ticket", "amqp.method.arguments.ticket",
         FT_UINT16, BASE_DEC, NULL, 0,
        "ticket", HFILL}},
    {&hf_amqp_method_stream_consume_queue, {
        "Queue", "amqp.method.arguments.queue",
        FT_STRING, BASE_NONE, NULL, 0,
        "queue", HFILL}},
    {&hf_amqp_method_stream_consume_consumer_tag, {
        "Consumer Tag", "amqp.method.arguments.consumer_tag",
        FT_STRING, BASE_NONE, NULL, 0,
        "consumer tag", HFILL}},
    {&hf_amqp_method_stream_consume_no_local, {
        "No Local", "amqp.method.arguments.no_local",
        FT_BOOLEAN, 8, NULL, 0x01,
        "no local", HFILL}},
    {&hf_amqp_method_stream_consume_exclusive, {
        "Exclusive", "amqp.method.arguments.exclusive",
        FT_BOOLEAN, 8, NULL, 0x02,
        "exclusive", HFILL}},
    {&hf_amqp_method_stream_consume_nowait, {
        "Nowait", "amqp.method.arguments.nowait",
        FT_BOOLEAN, 8, NULL, 0x04,
        "nowait", HFILL}},
    {&hf_amqp_method_stream_consume_ok_consumer_tag, {
        "Consumer Tag", "amqp.method.arguments.consumer_tag",
        FT_STRING, BASE_NONE, NULL, 0,
        "consumer tag", HFILL}},
    {&hf_amqp_method_stream_cancel_consumer_tag, {
        "Consumer Tag", "amqp.method.arguments.consumer_tag",
        FT_STRING, BASE_NONE, NULL, 0,
        "consumer tag", HFILL}},
    {&hf_amqp_method_stream_cancel_nowait, {
        "Nowait", "amqp.method.arguments.nowait",
        FT_BOOLEAN, 8, NULL, 0x01,
        "nowait", HFILL}},
    {&hf_amqp_method_stream_cancel_ok_consumer_tag, {
        "Consumer Tag", "amqp.method.arguments.consumer_tag",
        FT_STRING, BASE_NONE, NULL, 0,
        "consumer tag", HFILL}},
    {&hf_amqp_method_stream_publish_ticket, {
        "Ticket", "amqp.method.arguments.ticket",
         FT_UINT16, BASE_DEC, NULL, 0,
        "ticket", HFILL}},
    {&hf_amqp_method_stream_publish_exchange, {
        "Exchange", "amqp.method.arguments.exchange",
        FT_STRING, BASE_NONE, NULL, 0,
        "exchange", HFILL}},
    {&hf_amqp_method_stream_publish_routing_key, {
        "Routing Key", "amqp.method.arguments.routing_key",
        FT_STRING, BASE_NONE, NULL, 0,
        "routing key", HFILL}},
    {&hf_amqp_method_stream_publish_mandatory, {
        "Mandatory", "amqp.method.arguments.mandatory",
        FT_BOOLEAN, 8, NULL, 0x01,
        "mandatory", HFILL}},
    {&hf_amqp_method_stream_publish_immediate, {
        "Immediate", "amqp.method.arguments.immediate",
        FT_BOOLEAN, 8, NULL, 0x02,
        "immediate", HFILL}},
    {&hf_amqp_method_stream_return_reply_code, {
        "Reply Code", "amqp.method.arguments.reply_code",
         FT_UINT16, BASE_DEC, NULL, 0,
        "reply code", HFILL}},
    {&hf_amqp_method_stream_return_reply_text, {
        "Reply Text", "amqp.method.arguments.reply_text",
        FT_STRING, BASE_NONE, NULL, 0,
        "reply text", HFILL}},
    {&hf_amqp_method_stream_return_exchange, {
        "Exchange", "amqp.method.arguments.exchange",
        FT_STRING, BASE_NONE, NULL, 0,
        "exchange", HFILL}},
    {&hf_amqp_method_stream_return_routing_key, {
        "Routing Key", "amqp.method.arguments.routing_key",
        FT_STRING, BASE_NONE, NULL, 0,
        "routing key", HFILL}},
    {&hf_amqp_method_stream_deliver_consumer_tag, {
        "Consumer Tag", "amqp.method.arguments.consumer_tag",
        FT_STRING, BASE_NONE, NULL, 0,
        "consumer tag", HFILL}},
    {&hf_amqp_method_stream_deliver_delivery_tag, {
        "Delivery Tag", "amqp.method.arguments.delivery_tag",
        FT_UINT64, BASE_DEC, NULL, 0,
        "delivery tag", HFILL}},
    {&hf_amqp_method_stream_deliver_exchange, {
        "Exchange", "amqp.method.arguments.exchange",
        FT_STRING, BASE_NONE, NULL, 0,
        "exchange", HFILL}},
    {&hf_amqp_method_stream_deliver_queue, {
        "Queue", "amqp.method.arguments.queue",
        FT_STRING, BASE_NONE, NULL, 0,
        "queue", HFILL}},
    {&hf_amqp_method_dtx_start_dtx_identifier, {
        "Dtx Identifier", "amqp.method.arguments.dtx_identifier",
        FT_STRING, BASE_NONE, NULL, 0,
        "dtx identifier", HFILL}},
    {&hf_amqp_method_tunnel_request_meta_data, {
        "Meta Data", "amqp.method.arguments.meta_data",
        FT_NONE, BASE_NONE, NULL, 0,
        "meta data", HFILL}},
    {&hf_amqp_method_test_integer_integer_1, {
        "Integer 1", "amqp.method.arguments.integer_1",
        FT_UINT8, BASE_DEC, NULL, 0,
        "integer 1", HFILL}},
    {&hf_amqp_method_test_integer_integer_2, {
        "Integer 2", "amqp.method.arguments.integer_2",
         FT_UINT16, BASE_DEC, NULL, 0,
        "integer 2", HFILL}},
    {&hf_amqp_method_test_integer_integer_3, {
        "Integer 3", "amqp.method.arguments.integer_3",
        FT_UINT32, BASE_DEC, NULL, 0,
        "integer 3", HFILL}},
    {&hf_amqp_method_test_integer_integer_4, {
        "Integer 4", "amqp.method.arguments.integer_4",
        FT_UINT64, BASE_DEC, NULL, 0,
        "integer 4", HFILL}},
    {&hf_amqp_method_test_integer_operation, {
        "Operation", "amqp.method.arguments.operation",
        FT_UINT8, BASE_DEC, NULL, 0,
        "operation", HFILL}},
    {&hf_amqp_method_test_integer_ok_result, {
        "Result", "amqp.method.arguments.result",
        FT_UINT64, BASE_DEC, NULL, 0,
        "result", HFILL}},
    {&hf_amqp_method_test_string_string_1, {
        "String 1", "amqp.method.arguments.string_1",
        FT_STRING, BASE_NONE, NULL, 0,
        "string 1", HFILL}},
    {&hf_amqp_method_test_string_string_2, {
        "String 2", "amqp.method.arguments.string_2",
        FT_BYTES, BASE_NONE, NULL, 0,
        "string 2", HFILL}},
    {&hf_amqp_method_test_string_operation, {
        "Operation", "amqp.method.arguments.operation",
        FT_UINT8, BASE_DEC, NULL, 0,
        "operation", HFILL}},
    {&hf_amqp_method_test_string_ok_result, {
        "Result", "amqp.method.arguments.result",
        FT_BYTES, BASE_NONE, NULL, 0,
        "result", HFILL}},
    {&hf_amqp_method_test_table_table, {
        "Table", "amqp.method.arguments.table",
        FT_NONE, BASE_NONE, NULL, 0,
        "table", HFILL}},
    {&hf_amqp_method_test_table_integer_op, {
        "Integer Op", "amqp.method.arguments.integer_op",
        FT_UINT8, BASE_DEC, NULL, 0,
        "integer op", HFILL}},
    {&hf_amqp_method_test_table_string_op, {
        "String Op", "amqp.method.arguments.string_op",
        FT_UINT8, BASE_DEC, NULL, 0,
        "string op", HFILL}},
    {&hf_amqp_method_test_table_ok_integer_result, {
        "Integer Result", "amqp.method.arguments.integer_result",
        FT_UINT64, BASE_DEC, NULL, 0,
        "integer result", HFILL}},
    {&hf_amqp_method_test_table_ok_string_result, {
        "String Result", "amqp.method.arguments.string_result",
        FT_BYTES, BASE_NONE, NULL, 0,
        "string result", HFILL}},
    {&hf_amqp_method_test_content_ok_content_checksum, {
        "Content Checksum", "amqp.method.arguments.content_checksum",
        FT_UINT32, BASE_DEC, NULL, 0,
        "content checksum", HFILL}},
    {&hf_amqp_field, {
        "AMQP", "amqp",
        FT_NONE, BASE_NONE, NULL, 0,
        "AMQP", HFILL}},
    {&hf_amqp_header_class_id, {
        "Class ID", "amqp.header.class",
        FT_UINT16, BASE_DEC, VALS(amqp_method_classes), 0,
        "Class ID", HFILL}},
    {&hf_amqp_header_weight, {
        "Weight", "amqp.header.weight",
        FT_UINT16, BASE_DEC, NULL, 0,
        "Weight", HFILL}},
    {&hf_amqp_header_body_size, {
        "Body size", "amqp.header.body-size",
        FT_UINT64, BASE_DEC, NULL, 0,
        "Body size", HFILL}},
    {&hf_amqp_header_property_flags, {
        "Property flags", "amqp.header.property-flags",
        FT_UINT16, BASE_HEX, NULL, 0,
        "Property flags", HFILL}},
    {&hf_amqp_header_properties, {
        "Properties", "amqp.header.properties",
        FT_NONE, BASE_NONE, NULL, 0x0,
        "Message properties", HFILL}},
    {&hf_amqp_header_basic_content_type, {
        "Content Type", "amqp.method.properties.content_type",
        FT_STRING, BASE_NONE, NULL, 0,
        "content type", HFILL}},
    {&hf_amqp_header_basic_content_encoding, {
        "Content Encoding", "amqp.method.properties.content_encoding",
        FT_STRING, BASE_NONE, NULL, 0,
        "content encoding", HFILL}},
    {&hf_amqp_header_basic_headers, {
        "Headers", "amqp.method.properties.headers",
        FT_NONE, BASE_NONE, NULL, 0,
        "headers", HFILL}},
    {&hf_amqp_header_basic_delivery_mode, {
        "Delivery Mode", "amqp.method.properties.delivery_mode",
        FT_UINT8, BASE_DEC, NULL, 0,
        "delivery mode", HFILL}},
    {&hf_amqp_header_basic_priority, {
        "Priority", "amqp.method.properties.priority",
        FT_UINT8, BASE_DEC, NULL, 0,
        "priority", HFILL}},
    {&hf_amqp_header_basic_correlation_id, {
        "Correlation Id", "amqp.method.properties.correlation_id",
        FT_STRING, BASE_NONE, NULL, 0,
        "correlation id", HFILL}},
    {&hf_amqp_header_basic_reply_to, {
        "Reply To", "amqp.method.properties.reply_to",
        FT_STRING, BASE_NONE, NULL, 0,
        "reply to", HFILL}},
    {&hf_amqp_header_basic_expiration, {
        "Expiration", "amqp.method.properties.expiration",
        FT_STRING, BASE_NONE, NULL, 0,
        "expiration", HFILL}},
    {&hf_amqp_header_basic_message_id, {
        "Message Id", "amqp.method.properties.message_id",
        FT_STRING, BASE_NONE, NULL, 0,
        "message id", HFILL}},
    {&hf_amqp_header_basic_timestamp, {
        "Timestamp", "amqp.method.properties.timestamp",
        FT_UINT64, BASE_DEC, NULL, 0,
        "timestamp", HFILL}},
    {&hf_amqp_header_basic_type, {
        "Type", "amqp.method.properties.type",
        FT_STRING, BASE_NONE, NULL, 0,
        "type", HFILL}},
    {&hf_amqp_header_basic_user_id, {
        "User Id", "amqp.method.properties.user_id",
        FT_STRING, BASE_NONE, NULL, 0,
        "user id", HFILL}},
    {&hf_amqp_header_basic_app_id, {
        "App Id", "amqp.method.properties.app_id",
        FT_STRING, BASE_NONE, NULL, 0,
        "app id", HFILL}},
    {&hf_amqp_header_basic_cluster_id, {
        "Cluster Id", "amqp.method.properties.cluster_id",
        FT_STRING, BASE_NONE, NULL, 0,
        "cluster id", HFILL}},
    {&hf_amqp_header_file_content_type, {
        "Content Type", "amqp.method.properties.content_type",
        FT_STRING, BASE_NONE, NULL, 0,
        "content type", HFILL}},
    {&hf_amqp_header_file_content_encoding, {
        "Content Encoding", "amqp.method.properties.content_encoding",
        FT_STRING, BASE_NONE, NULL, 0,
        "content encoding", HFILL}},
    {&hf_amqp_header_file_headers, {
        "Headers", "amqp.method.properties.headers",
        FT_NONE, BASE_NONE, NULL, 0,
        "headers", HFILL}},
    {&hf_amqp_header_file_priority, {
        "Priority", "amqp.method.properties.priority",
        FT_UINT8, BASE_DEC, NULL, 0,
        "priority", HFILL}},
    {&hf_amqp_header_file_reply_to, {
        "Reply To", "amqp.method.properties.reply_to",
        FT_STRING, BASE_NONE, NULL, 0,
        "reply to", HFILL}},
    {&hf_amqp_header_file_message_id, {
        "Message Id", "amqp.method.properties.message_id",
        FT_STRING, BASE_NONE, NULL, 0,
        "message id", HFILL}},
    {&hf_amqp_header_file_filename, {
        "Filename", "amqp.method.properties.filename",
        FT_STRING, BASE_NONE, NULL, 0,
        "filename", HFILL}},
    {&hf_amqp_header_file_timestamp, {
        "Timestamp", "amqp.method.properties.timestamp",
        FT_UINT64, BASE_DEC, NULL, 0,
        "timestamp", HFILL}},
    {&hf_amqp_header_file_cluster_id, {
        "Cluster Id", "amqp.method.properties.cluster_id",
        FT_STRING, BASE_NONE, NULL, 0,
        "cluster id", HFILL}},
    {&hf_amqp_header_stream_content_type, {
        "Content Type", "amqp.method.properties.content_type",
        FT_STRING, BASE_NONE, NULL, 0,
        "content type", HFILL}},
    {&hf_amqp_header_stream_content_encoding, {
        "Content Encoding", "amqp.method.properties.content_encoding",
        FT_STRING, BASE_NONE, NULL, 0,
        "content encoding", HFILL}},
    {&hf_amqp_header_stream_headers, {
        "Headers", "amqp.method.properties.headers",
        FT_NONE, BASE_NONE, NULL, 0,
        "headers", HFILL}},
    {&hf_amqp_header_stream_priority, {
        "Priority", "amqp.method.properties.priority",
        FT_UINT8, BASE_DEC, NULL, 0,
        "priority", HFILL}},
    {&hf_amqp_header_stream_timestamp, {
        "Timestamp", "amqp.method.properties.timestamp",
        FT_UINT64, BASE_DEC, NULL, 0,
        "timestamp", HFILL}},
    {&hf_amqp_header_tunnel_headers, {
        "Headers", "amqp.method.properties.headers",
        FT_NONE, BASE_NONE, NULL, 0,
        "headers", HFILL}},
    {&hf_amqp_header_tunnel_proxy_name, {
        "Proxy Name", "amqp.method.properties.proxy_name",
        FT_STRING, BASE_NONE, NULL, 0,
        "proxy name", HFILL}},
    {&hf_amqp_header_tunnel_data_name, {
        "Data Name", "amqp.method.properties.data_name",
        FT_STRING, BASE_NONE, NULL, 0,
        "data name", HFILL}},
    {&hf_amqp_header_tunnel_durable, {
        "Durable", "amqp.method.properties.durable",
        FT_UINT8, BASE_DEC, NULL, 0,
        "durable", HFILL}},
    {&hf_amqp_header_tunnel_broadcast, {
        "Broadcast", "amqp.method.properties.broadcast",
        FT_UINT8, BASE_DEC, NULL, 0,
        "broadcast", HFILL}},
    {&hf_amqp_payload, {
        "Payload", "amqp.payload",
        FT_BYTES, BASE_NONE, NULL, 0,
        "Message payload", HFILL}},
};

/*  Setup of protocol subtree array  */

static gint *ett [] = {
     &ett_amqp,
     &ett_args,
     &ett_props,
     &ett_field_table
};

/*  Basic registration functions  */

void
proto_register_amqp(void)
{
    proto_amqp = proto_register_protocol(
        "Advanced Message Queueing Protocol", "AMQP", "amqp");
    proto_register_field_array(proto_amqp, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));
    prefs_register_protocol(proto_amqp, proto_reg_handoff_amqp);
}

void
proto_reg_handoff_amqp(void)
{
    dissector_add("tcp.port", amqp_port,
        create_dissector_handle(dissect_amqp, proto_amqp));
}
