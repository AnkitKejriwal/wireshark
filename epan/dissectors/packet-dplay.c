/* packet-dplay.c
 * This is a dissector for the DirectPlay protocol.
 * Copyright 2006 - 2008 by Kai Blin
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <epan/packet.h>
#include <epan/prefs.h>
#include <epan/emem.h>
#include <string.h>

/* function declarations */
void proto_reg_handoff_dplay(void);
static gboolean heur_dissect_dplay(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);
static void dissect_dplay(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);
static gint dissect_type1a_message(proto_tree *tree, tvbuff_t *tvb, gint offset);
static gint dissect_type2e_message(proto_tree *tree, tvbuff_t *tvb, gint offset);

static int proto_dplay = -1;
static dissector_handle_t dplay_handle;

/* Common data fields */
static int hf_dplay_size = -1;              /* Size of the whole data */
static int hf_dplay_token = -1;
static int hf_dplay_saddr_af = -1;          /* AF_INET, as this dissector does not handle IPX yet */
static int hf_dplay_saddr_port = -1;        /* port to use for the reply to this packet */
static int hf_dplay_saddr_ip = -1;          /* IP to use for the reply to this packet, or 0.0.0.0,
                                               then use the same IP as this packet used. */
static int hf_dplay_saddr_padding = -1;     /* null padding used in s_addr_in structures */
static int hf_dplay_play_str = -1;          /* always "play" without a null terminator */
static int hf_dplay_command = -1;           /* the dplay command this message contains*/
static int hf_dplay_proto_dialect = -1;     /* 0x0b00 for dplay7, 0x0e00 for dplay9 */
static int hf_dplay_play_str_2 = -1;        /* packet type 0x0015 encapsulates another packet */
static int hf_dplay_command_2 = -1;         /* that also has a "play" string, a command and a */
static int hf_dplay_proto_dialect_2 = -1;   /* protocol dialect, same as above */
static const int DPLAY_HEADER_OFFSET = 28;  /* The dplay header is 28 bytes in size */

/* The following fields are not part of the header, but hopefully have the same
 * meaning for all packets they show up in. */

static int hf_dplay_sess_desc_flags = -1; /* This is a 32bit field with some sort of a flag */
static int hf_dplay_flags_no_create_players = -1;
static int hf_dplay_flags_0002 = -1;
static int hf_dplay_flags_migrate_host = -1;
static int hf_dplay_flags_short_player_msg = -1;
static int hf_dplay_flags_ignored = -1;
static int hf_dplay_flags_can_join = -1;
static int hf_dplay_flags_use_ping = -1;
static int hf_dplay_flags_no_player_updates = -1;
static int hf_dplay_flags_use_auth = -1;
static int hf_dplay_flags_private_session = -1;
static int hf_dplay_flags_password_req = -1;
static int hf_dplay_flags_route = -1;
static int hf_dplay_flags_server_player_only = -1;
static int hf_dplay_flags_reliable = -1;
static int hf_dplay_flags_preserve_order = -1;
static int hf_dplay_flags_optimize_latency = -1;
static int hf_dplay_flags_acqire_voice = -1;
static int hf_dplay_flags_no_sess_desc_changes = -1;

#define DPLAY_FLAG_NO_CREATE_PLAYERS 0x0001
#define DPLAY_FLAG_0002 0x0002
#define DPLAY_FLAG_MIGRATE_HOST 0x0004
#define DPLAY_FLAG_SHORT_PLAYER_MSG 0x0008
#define DPLAY_FLAG_IGNORED 0x0010
#define DPLAY_FLAG_CAN_JOIN 0x0020
#define DPLAY_FLAG_USE_PING 0x0040
#define DPLAY_FLAG_NO_P_UPD 0x0080
#define DPLAY_FLAG_USE_AUTH 0x0100
#define DPLAY_FLAG_PRIV_SESS 0x0200
#define DPLAY_FLAG_PASS_REQ 0x0400
#define DPLAY_FLAG_ROUTE 0x0800
#define DPLAY_FLAG_SRV_ONLY 0x1000
#define DPLAY_FLAG_RELIABLE 0x2000
#define DPLAY_FLAG_ORDER 0x4000
#define DPLAY_FLAG_OPT_LAT 0x8000
#define DPLAY_FLAG_ACQ_VOICE 0x10000
#define DPLAY_FLAG_NO_SESS_DESC_CHANGES 0x20000

/* special fields, to be phased out in favour for more detailed information */
static int hf_dplay_data_type_0f = -1;
static int hf_dplay_data_type_1a = -1;
static int hf_dplay_data_type_29 = -1;

/* Session description structure fields */
static int hf_dplay_sess_desc_length = -1;
static int hf_dplay_game_guid = -1;
static int hf_dplay_instance_guid = -1;
static int hf_dplay_max_players = -1;
static int hf_dplay_curr_players = -1;
static int hf_dplay_sess_name_ptr = -1;
static int hf_dplay_passwd_ptr = -1;
static int hf_dplay_sess_desc_reserved_1 = -1;
static int hf_dplay_sess_desc_reserved_2 = -1;
static int hf_dplay_sess_desc_user_1 = -1;
static int hf_dplay_sess_desc_user_2 = -1;
static int hf_dplay_sess_desc_user_3 = -1;
static int hf_dplay_sess_desc_user_4 = -1;

/* Message Type 0x0001 data fields */
static int hf_dplay_type_01_name_offset = -1;
static int hf_dplay_type_01_game_name = -1;

/* Message Type 0x0002 data fields */
static int hf_dplay_type_02_game_guid = -1;
static int hf_dplay_type_02_ignored = -1;

/* Message Type 0x0005 data fields */
static int hf_dplay_type_05_request = -1;

/* Message Type 0x0007 data fields */
static int hf_dplay_type_07_dpid = -1;
static int hf_dplay_type_07_padding = -1;

/* Message Type 0x0008 data fields */
static int hf_dplay_type_08_padding_1 = -1;     /* 4 bytes */
static int hf_dplay_type_08_dpid_1 = -1;        /* 4 bytes */
static int hf_dplay_type_08_unknown_1 = -1;     /*20 bytes */
static int hf_dplay_type_08_dpid_2 = -1;        /* 4 bytes */
static int hf_dplay_type_08_string_1_len = -1;
static int hf_dplay_type_08_string_2_len = -1;
static int hf_dplay_type_08_unknown_2 = -1;     /*20 bytes */
static int hf_dplay_type_08_dpid_3 = -1;        /* 4 bytes */
static int hf_dplay_type_08_unknown_3 = -1;     /*12 bytes */
static int hf_dplay_type_08_string_1 = -1;
static int hf_dplay_type_08_string_2 = -1;
static int hf_dplay_type_08_saddr_af_1 = -1;    /* 2 bytes */
static int hf_dplay_type_08_saddr_port_1 = -1;  /* 2 bytes */
static int hf_dplay_type_08_saddr_ip_1 = -1;    /* 4 bytes */
static int hf_dplay_type_08_saddr_padd_1 = -1;  /* 8 bytes */
static int hf_dplay_type_08_saddr_af_2 = -1;    /* 2 bytes */
static int hf_dplay_type_08_saddr_port_2 = -1;  /* 2 bytes */
static int hf_dplay_type_08_saddr_ip_2 = -1;    /* 4 bytes */
static int hf_dplay_type_08_saddr_padd_2 = -1;  /* 8 bytes */
static int hf_dplay_type_08_padding_2 = -1;     /* 2 bytes */

/* Message Type 0x000b data fields */
static int hf_dplay_type_0b_padding_1 = -1;
static int hf_dplay_type_0b_dpid = -1;
static int hf_dplay_type_0b_padding_2 = -1;

/* Message Type 0x000d data fields */
static int hf_dplay_type_0d_padding_1 = -1;     /* 4 bytes */
static int hf_dplay_type_0d_dpid_1 = -1;        /* 4 bytes */
static int hf_dplay_type_0d_dpid_2 = -1;        /* 4 bytes */
static int hf_dplay_type_0d_padding_2 = -1;     /* 8 bytes */

/* Message Type 0x000e data fields */
static int hf_dplay_type_0e_padding_1 = -1;     /* 4 bytes */
static int hf_dplay_type_0e_dpid_1 = -1;        /* 4 bytes */
static int hf_dplay_type_0e_dpid_2 = -1;        /* 4 bytes */
static int hf_dplay_type_0e_padding_2 = -1;     /* 8 bytes */


/* Message Type 0x0013 data fields */
static int hf_dplay_type_13_padding_1 = -1;     /* 4 bytes */
static int hf_dplay_type_13_dpid_1 = -1;        /* 4 bytes */
static int hf_dplay_type_13_unknown_1 = -1;     /*20 bytes */
static int hf_dplay_type_13_dpid_2 = -1;        /* 4 bytes */
static int hf_dplay_type_13_unknown_2 = -1;     /*20 bytes */
static int hf_dplay_type_13_dpid_3 = -1;        /* 4 bytes */
static int hf_dplay_type_13_unknown_3 = -1;     /*12 bytes */
static int hf_dplay_type_13_saddr_af_1 = -1;    /* 2 bytes */
static int hf_dplay_type_13_saddr_port_1 = -1;  /* 2 bytes */
static int hf_dplay_type_13_saddr_ip_1 = -1;    /* 4 bytes */
static int hf_dplay_type_13_saddr_padd_1 = -1;  /* 8 bytes */
static int hf_dplay_type_13_saddr_af_2 = -1;    /* 2 bytes */
static int hf_dplay_type_13_saddr_port_2 = -1;  /* 2 bytes */
static int hf_dplay_type_13_saddr_ip_2 = -1;    /* 4 bytes */
static int hf_dplay_type_13_saddr_padd_2 = -1;  /* 8 bytes */
static int hf_dplay_type_13_padding_2 = -1;     /* 2 bytes */
static int hf_dplay_type_13_dpid_4 = -1;        /* 4 bytes */

/* Message Type 0x0015 data fields */
static int hf_dplay_container_guid = -1;
static int hf_dplay_type_15_padding_1 = -1;
static int hf_dplay_type_15_size_1 = -1;
static int hf_dplay_type_15_padding_2 = -1;
static int hf_dplay_type_15_unknown_1 = -1;
static int hf_dplay_type_15_size_2 = -1;
static int hf_dplay_type_15_padding_3 = -1;

/* Message Type 0x0016 data field */
static int hf_dplay_type_16_data = -1;

/* Message Type 0x0017 data field */
static int hf_dplay_type_17_data = -1;

/* Message Type 0x0029 data fields */
static int hf_dplay_type_29_unknown_uint32_01 = -1; /* seems to always be 3 */
static int hf_dplay_type_29_message_end_type = -1;  /* mostly 0, alternative packet ending on 1 */
static int hf_dplay_type_29_unknown_uint32_03 = -1;
static int hf_dplay_type_29_unknown_uint32_04 = -1;
static int hf_dplay_type_29_unknown_uint32_05 = -1;
static int hf_dplay_type_29_unknown_uint32_06 = -1;
static int hf_dplay_type_29_unknown_uint32_07 = -1;
static int hf_dplay_type_29_unknown_uint32_08 = -1;
static int hf_dplay_type_29_magic_16_bytes = -1;
static int hf_dplay_type_29_dpid_1 = -1;
static int hf_dplay_type_29_unknown_3 = -1;
static int hf_dplay_type_29_game_name = -1;
static int hf_dplay_type_29_unknown_uint32_10 = -1;
static int hf_dplay_type_29_unknown_uint32_11 = -1;
static int hf_dplay_type_29_dpid_2 = -1;
static int hf_dplay_type_29_unknown_uint32_12 = -1;
static int hf_dplay_type_29_unknown_uint32_13 = -1;
static int hf_dplay_type_29_saddr_field_len_1 = -1;
static int hf_dplay_type_29_saddr_af_1 = -1;
static int hf_dplay_type_29_saddr_port_1 = -1;
static int hf_dplay_type_29_saddr_ip_1 = -1;
static int hf_dplay_type_29_saddr_padd_1 = -1;
static int hf_dplay_type_29_saddr_af_2 = -1;
static int hf_dplay_type_29_saddr_port_2 = -1;
static int hf_dplay_type_29_saddr_ip_2 = -1;
static int hf_dplay_type_29_saddr_padd_2 = -1;
static int hf_dplay_type_29_unknown_uint32_14 = -1;
static int hf_dplay_type_29_unknown_uint32_15 = -1;
static int hf_dplay_type_29_dpid_3 = -1;
static int hf_dplay_type_29_unknown_uint32_16 = -1;
static int hf_dplay_type_29_unknown_uint32_17 = -1;
static int hf_dplay_type_29_saddr_field_len_2 = -1;
static int hf_dplay_type_29_saddr_af_3 = -1;
static int hf_dplay_type_29_saddr_port_3 = -1;
static int hf_dplay_type_29_saddr_ip_3 = -1;
static int hf_dplay_type_29_saddr_padd_3 = -1;
static int hf_dplay_type_29_saddr_af_4 = -1;
static int hf_dplay_type_29_saddr_port_4 = -1;
static int hf_dplay_type_29_saddr_ip_4 = -1;
static int hf_dplay_type_29_saddr_padd_4 = -1;
static int hf_dplay_type_29_unknown_uint32_18 = -1;
static int hf_dplay_type_29_unknown_uint32_19= -1;
static int hf_dplay_type_29_dpid_4 = -1;
static int hf_dplay_type_29_unknown_uint32_20 = -1;
static int hf_dplay_type_29_dpid_5 = -1;

/* Message Type 0x002e data fields */
static int hf_dplay_type_2e_padding_1 = -1;
static int hf_dplay_type_2e_dpid_1 = -1;
static int hf_dplay_type_2e_unknown_1 = -1;
static int hf_dplay_type_2e_dpid_2 = -1;
static int hf_dplay_type_2e_string_1_len = -1;
static int hf_dplay_type_2e_string_2_len = -1;
static int hf_dplay_type_2e_unknown_2 = -1;
static int hf_dplay_type_2e_dpid_3 = -1;
static int hf_dplay_type_2e_unknown_3 = -1;
static int hf_dplay_type_2e_string_1 = -1;
static int hf_dplay_type_2e_string_2 = -1;
static int hf_dplay_type_2e_saddr_af_1 = -1;
static int hf_dplay_type_2e_saddr_port_1 = -1;
static int hf_dplay_type_2e_saddr_ip_1 = -1;
static int hf_dplay_type_2e_saddr_padd_1 = -1;
static int hf_dplay_type_2e_saddr_af_2 = -1;
static int hf_dplay_type_2e_saddr_port_2 = -1;
static int hf_dplay_type_2e_saddr_ip_2 = -1;
static int hf_dplay_type_2e_saddr_padd_2 = -1;

/* Message Type 0x002f data fields */
static int hf_dplay_type_2f_dpid = -1;

/* Message Type 0x0038 data fields */
static int hf_dplay_type_38_padding_1 = -1;     /* 4 bytes */
static int hf_dplay_type_38_dpid_1 = -1;        /* 4 bytes */
static int hf_dplay_type_38_unknown_1 = -1;     /*20 bytes */
static int hf_dplay_type_38_dpid_2 = -1;        /* 4 bytes */
static int hf_dplay_type_38_string_1_len = -1;
static int hf_dplay_type_38_string_2_len = -1;
static int hf_dplay_type_38_unknown_2 = -1;     /*20 bytes */
static int hf_dplay_type_38_dpid_3 = -1;        /* 4 bytes */
static int hf_dplay_type_38_unknown_3 = -1;     /*12 bytes */
static int hf_dplay_type_38_string_1 = -1;
static int hf_dplay_type_38_string_2 = -1;
static int hf_dplay_type_38_saddr_af_1 = -1;    /* 2 bytes */
static int hf_dplay_type_38_saddr_port_1 = -1;  /* 2 bytes */
static int hf_dplay_type_38_saddr_ip_1 = -1;    /* 4 bytes */
static int hf_dplay_type_38_saddr_padd_1 = -1;  /* 8 bytes */
static int hf_dplay_type_38_saddr_af_2 = -1;    /* 2 bytes */
static int hf_dplay_type_38_saddr_port_2 = -1;  /* 2 bytes */
static int hf_dplay_type_38_saddr_ip_2 = -1;    /* 4 bytes */
static int hf_dplay_type_38_saddr_padd_2 = -1;  /* 8 bytes */

/* various */
static gint ett_dplay = -1;
static gint ett_dplay_header = -1;
static gint ett_dplay_sockaddr = -1;
static gint ett_dplay_data = -1;
static gint ett_dplay_enc_packet = -1;
static gint ett_dplay_flags = -1;
static gint ett_dplay_sess_desc_flags = -1;
static gint ett_dplay_type08_saddr1 = -1;
static gint ett_dplay_type08_saddr2 = -1;
static gint ett_dplay_type13_saddr1 = -1;
static gint ett_dplay_type13_saddr2 = -1;
static gint ett_dplay_type29_saddr1 = -1;
static gint ett_dplay_type29_saddr2 = -1;
static gint ett_dplay_type29_saddr3 = -1;
static gint ett_dplay_type29_saddr4 = -1;
static gint ett_dplay_type2e_saddr1 = -1;
static gint ett_dplay_type2e_saddr2 = -1;
static gint ett_dplay_type38_saddr1 = -1;
static gint ett_dplay_type38_saddr2 = -1;

static const value_string dplay_command_val[] = {
    { 0x0001, "Enum Sessions Reply" },
    { 0x0002, "Enum Sessions" },
    { 0x0003, "Enum Players Reply" },
    { 0x0004, "Enum Players" },
    { 0x0005, "Request Player ID" },
    { 0x0006, "Request Group ID" },
    { 0x0007, "Request Player Reply" },
    { 0x0008, "Create Player" },
    { 0x0009, "Create Group" },
    { 0x000a, "Player Message" },
    { 0x000b, "Delete Player" },
    { 0x000c, "Delete Group" },
    { 0x000d, "Add Player To Group" },
    { 0x000e, "Delete Player From Group" },
    { 0x000f, "Player Data Changed" },
    { 0x0010, "Player Name Changed" },
    { 0x0011, "Group Data Changed" },
    { 0x0012, "Group Name Changed" },
    { 0x0013, "Add Forward Request" },
    /* There is no command 0x0014 */
    { 0x0015, "Packet" },
    { 0x0016, "Ping" },
    { 0x0017, "Pong" },
    { 0x0018, "You Are Dead" },
    { 0x0019, "Player Wrapper" },
    { 0x001a, "Session Desc Changed" },
    { 0x001c, "Challenge" },
    { 0x001d, "Access Granted" },
    { 0x001e, "Logon Denied" },
    { 0x001f, "Auth Error" },
    { 0x0020, "Negotiate" },
    { 0x0021, "Challenge Response" },
    { 0x0022, "Signed"},
    /* There is no command 0x0023 */
    { 0x0024, "Add Forward Reply" },
    { 0x0025, "Ask For Multicast" },
    { 0x0026, "Ask For Multicast Guaranteed" },
    { 0x0027, "Add Shortcut To Group" },
    { 0x0028, "Delete Group From Group" },
    { 0x0029, "Super Enum Players Reply" },
    /* There is no command 0x002a */
    { 0x002b, "Key Exchange" },
    { 0x002c, "Key Exchange Reply" },
    { 0x002d, "Chat" },
    { 0x002e, "Add Forward" },
    { 0x002f, "Add Forward ACK" },
    { 0x0030, "Packet2 Data" },
    { 0x0031, "Packet2 ACK" },
    /* No commands 0x0032, 0x0033, 0x0034 */
    { 0x0035, "I Am Nameserver" },
    { 0x0036, "Voice" },
    { 0x0037, "Multicast Delivery" },
    { 0x0038, "Create Players Verify"},
    { 0     , NULL },
};

static const value_string dplay_af_val[] = {
    { 0x0002, "AF_INET" },
    { 0x0006, "AF_IPX" },
    { 0     , NULL},
};

static const value_string dplay_proto_dialect_val[] = {
    { 0x0009, "dplay 6" },
    { 0x000a, "dplay 6.1" },
    { 0x000b, "dplay 6.1a" },
    { 0x000c, "dplay 7.1" },
    { 0x000d, "dplay 8" },
    { 0x000e, "dplay 9"},
    { 0     , NULL},
};

static const value_string dplay_token_val[] = {
    { 0xfab, "Remote Message" },
    { 0xcab, "Forwarded Message" },
    { 0xbab, "Server Message" },
};

static const value_string dplay_type05_request[] = {
    { 0x00000008, "eight"},
    { 0x00000009, "nine"},
    { 0         , NULL},
};

static const value_string dplay_type29_end_type[] = {
    { 0x00000000, "sockaddr"},
    { 0x00000001, "DPID"},
    { 0         , NULL},
};

static const true_false_string tfs_dplay_flag = {
    "present",
    "absent"
};

/* borrowed from epan/dissectors/packets-smb-common.c */
static gint display_unicode_string(proto_tree *tree, gint hf_index, tvbuff_t *tvb, gint offset)
{
    char *str, *p;
    gint len;
    gint charoffset;
    guint16 character;

    /* display a unicode string from the tree and return new offset */

    /*
     * Get the length of the string.
     * XXX - is it a bug or a feature that this will throw an exception
     * if we don't find the '\0'?  I think it's a feature.
     */
    len = 0;
    while ((character = tvb_get_letohs(tvb, offset + len)) != '\0')
        len += 2;
    len += 2;   /* count the '\0' too */

    /*
     * Allocate a buffer for the string; "len" is the length in
     * bytes, not the length in characters.
     */
    str = ep_alloc(len/2);

    /*
     * XXX - this assumes the string is just ISO 8859-1; we need
     * to better handle multiple character sets in Wireshark,
     * including Unicode/ISO 10646, and multiple encodings of
     * that character set (UCS-2, UTF-8, etc.).
     */
    charoffset = offset;
    p = str;
    while ((character = tvb_get_letohs(tvb, charoffset)) != '\0') {
        *p++ = (char) character;
        charoffset += 2;
    }
    *p = '\0';

    proto_tree_add_string(tree, hf_index, tvb, offset, len, str);

    return  offset+len;
}

static gint dissect_sockaddr_in(proto_tree *tree, tvbuff_t *tvb, gint offset)
{
    proto_item *sa_item = NULL;
    proto_tree *sa_tree = NULL;

    sa_item = proto_tree_add_text(tree, tvb, offset, 16,
            "DirectPlay sockaddr_in structure");
    sa_tree = proto_item_add_subtree(sa_item, ett_dplay_sockaddr);
    proto_tree_add_item(sa_tree, hf_dplay_saddr_af, tvb, offset, 2, TRUE); offset += 2;
    proto_tree_add_item(sa_tree, hf_dplay_saddr_port, tvb, offset, 2, FALSE); offset += 2;
    proto_tree_add_item(sa_tree, hf_dplay_saddr_ip, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(sa_tree, hf_dplay_saddr_padding, tvb, offset, 8, FALSE); offset += 8;
    return offset;
}

static gint dissect_session_desc(proto_tree *tree, tvbuff_t *tvb, gint offset)
{
    gint data_len;
    guint32 flags;
    proto_item *flags_item = NULL;
    proto_tree *flags_tree = NULL;

    data_len = tvb_get_letohl(tvb, offset);
    flags = tvb_get_letohl(tvb, offset+4);

    proto_tree_add_item(tree, hf_dplay_sess_desc_length, tvb, offset, 4, TRUE); offset += 4;
    flags_item = proto_tree_add_item(tree, hf_dplay_sess_desc_flags, tvb, offset, 4, TRUE);
    flags_tree = proto_item_add_subtree(flags_item, ett_dplay_sess_desc_flags);
    proto_tree_add_boolean(flags_tree, hf_dplay_flags_no_sess_desc_changes, tvb, offset, 4, flags);
    proto_tree_add_boolean(flags_tree, hf_dplay_flags_acqire_voice, tvb, offset, 4, flags);
    proto_tree_add_boolean(flags_tree, hf_dplay_flags_optimize_latency, tvb, offset, 4, flags);
    proto_tree_add_boolean(flags_tree, hf_dplay_flags_preserve_order, tvb, offset, 4, flags);
    proto_tree_add_boolean(flags_tree, hf_dplay_flags_reliable, tvb, offset, 4, flags);
    proto_tree_add_boolean(flags_tree, hf_dplay_flags_server_player_only, tvb, offset, 4, flags);
    proto_tree_add_boolean(flags_tree, hf_dplay_flags_route, tvb, offset, 4, flags);
    proto_tree_add_boolean(flags_tree, hf_dplay_flags_password_req, tvb, offset, 4, flags);
    proto_tree_add_boolean(flags_tree, hf_dplay_flags_private_session, tvb, offset, 4, flags);
    proto_tree_add_boolean(flags_tree, hf_dplay_flags_use_auth, tvb, offset, 4, flags);
    proto_tree_add_boolean(flags_tree, hf_dplay_flags_no_player_updates, tvb, offset, 4, flags);
    proto_tree_add_boolean(flags_tree, hf_dplay_flags_use_ping, tvb, offset, 4, flags);
    proto_tree_add_boolean(flags_tree, hf_dplay_flags_can_join, tvb, offset, 4, flags);
    proto_tree_add_boolean(flags_tree, hf_dplay_flags_ignored, tvb, offset, 4, flags);
    proto_tree_add_boolean(flags_tree, hf_dplay_flags_short_player_msg, tvb, offset, 4, flags);
    proto_tree_add_boolean(flags_tree, hf_dplay_flags_migrate_host, tvb, offset, 4, flags);
    proto_tree_add_boolean(flags_tree, hf_dplay_flags_0002, tvb, offset, 4, flags);
    proto_tree_add_boolean(flags_tree, hf_dplay_flags_no_create_players, tvb, offset, 4, flags);
    offset += 4;

    proto_tree_add_item(tree, hf_dplay_instance_guid, tvb, offset, 16, FALSE); offset += 16;
    proto_tree_add_item(tree, hf_dplay_game_guid, tvb, offset, 16, FALSE); offset += 16;
    proto_tree_add_item(tree, hf_dplay_max_players, tvb, offset, 4, TRUE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_curr_players, tvb, offset, 4, TRUE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_sess_name_ptr, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_passwd_ptr, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_sess_desc_reserved_1, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_sess_desc_reserved_2, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_sess_desc_user_1, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_sess_desc_user_2, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_sess_desc_user_3, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_sess_desc_user_4, tvb, offset, 4, FALSE); offset += 4;

    return offset;
}

static gint dissect_dplay_header(proto_tree *tree, tvbuff_t *tvb, gint offset)
{
    guint32 mixed, size, token;

    mixed = tvb_get_letohl(tvb, offset);
    size = mixed & 0x000FFFFF;
    token = (mixed & 0xFFF00000) >> 20;

    proto_tree_add_uint(tree, hf_dplay_size, tvb, offset, 4, size);
    proto_tree_add_uint(tree, hf_dplay_token, tvb, offset, 4, token);
    offset += 4;
    offset = dissect_sockaddr_in(tree, tvb, offset);
    proto_tree_add_item(tree, hf_dplay_play_str, tvb, offset, 4, TRUE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_command, tvb, offset, 2, TRUE); offset += 2;
    proto_tree_add_item(tree, hf_dplay_proto_dialect, tvb, offset, 2, TRUE); offset += 2;
    return offset;
}

static gint dissect_type01_message(proto_tree *tree, tvbuff_t *tvb, gint offset)
{
    guint32 name_offset;

    offset = dissect_session_desc(tree, tvb, offset);
    name_offset = tvb_get_letohl(tvb, offset);
    proto_tree_add_item(tree, hf_dplay_type_01_name_offset, tvb, offset, 4, TRUE); offset += 4;

    if (name_offset != 0) {
        offset = display_unicode_string(tree, hf_dplay_type_01_game_name, tvb, offset);
    }
    return offset;
}

static gint dissect_type02_message(proto_tree *tree, tvbuff_t *tvb, gint offset)
{
    proto_tree_add_item(tree, hf_dplay_type_02_game_guid, tvb, offset, 16, FALSE); offset += 16;
    proto_tree_add_item(tree, hf_dplay_type_02_ignored, tvb, offset, 8, FALSE);
    return offset;
}

static gint dissect_type05_message(proto_tree *tree, tvbuff_t *tvb, gint offset)
{
    proto_tree_add_item(tree, hf_dplay_type_05_request, tvb, offset, 4, TRUE); offset += 4;
    return offset;
}

static gint dissect_type07_message(proto_tree *tree, tvbuff_t *tvb, gint offset)
{
    proto_tree_add_item(tree, hf_dplay_type_07_dpid, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_07_padding, tvb, offset, 36, FALSE); offset += 36;
    return offset;
}

static gint dissect_type08_message(proto_tree *tree, tvbuff_t *tvb, gint offset)
{
    proto_item *first_saddr_item = NULL, *second_saddr_item = NULL;
    proto_tree *first_saddr_tree = NULL, *second_saddr_tree = NULL;
    gint string_1_len, string_2_len;

    proto_tree_add_item(tree, hf_dplay_type_08_padding_1, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_08_dpid_1, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_08_unknown_1, tvb, offset, 20, FALSE); offset += 20;
    proto_tree_add_item(tree, hf_dplay_type_08_dpid_2, tvb, offset, 4, FALSE); offset += 4;
    string_1_len = tvb_get_letohl(tvb, offset);
    proto_tree_add_item(tree, hf_dplay_type_08_string_1_len, tvb, offset, 4, TRUE); offset += 4;
    string_2_len = tvb_get_letohl(tvb, offset);
    proto_tree_add_item(tree, hf_dplay_type_08_string_2_len, tvb, offset, 4, TRUE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_08_unknown_2, tvb, offset, 12, FALSE); offset += 12;
    proto_tree_add_item(tree, hf_dplay_type_08_dpid_3, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_08_unknown_3, tvb, offset, 12, FALSE); offset += 12;

    if(string_1_len > 0)
        offset = display_unicode_string(tree, hf_dplay_type_08_string_1, tvb, offset);
    if(string_2_len > 0)
        offset = display_unicode_string(tree, hf_dplay_type_08_string_2, tvb, offset);

    first_saddr_item = proto_tree_add_text(tree, tvb, offset, 16,
            "DirectPlay message type 0x0008 s_addr_in structure 1");
    first_saddr_tree = proto_item_add_subtree(first_saddr_item, ett_dplay_type08_saddr1);

    proto_tree_add_item(first_saddr_tree, hf_dplay_type_08_saddr_af_1, tvb, offset, 2, TRUE); offset += 2;
    proto_tree_add_item(first_saddr_tree, hf_dplay_type_08_saddr_port_1, tvb, offset, 2, FALSE); offset += 2;
    proto_tree_add_item(first_saddr_tree, hf_dplay_type_08_saddr_ip_1, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(first_saddr_tree, hf_dplay_type_08_saddr_padd_1, tvb, offset, 8, FALSE); offset += 8;

    second_saddr_item = proto_tree_add_text(tree, tvb, offset, 16,
            "DirectPlay message type 0x0008 s_addr_in structure 2");
    second_saddr_tree = proto_item_add_subtree(second_saddr_item, ett_dplay_type08_saddr2);

    proto_tree_add_item(second_saddr_tree, hf_dplay_type_08_saddr_af_2, tvb, offset, 2, TRUE); offset += 2;
    proto_tree_add_item(second_saddr_tree, hf_dplay_type_08_saddr_port_2, tvb, offset, 2, FALSE); offset += 2;
    proto_tree_add_item(second_saddr_tree, hf_dplay_type_08_saddr_ip_2, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(second_saddr_tree, hf_dplay_type_08_saddr_padd_2, tvb, offset, 8, FALSE); offset += 8;

    proto_tree_add_item(tree, hf_dplay_type_08_padding_2, tvb, offset, 6, FALSE); offset += 6;

    return offset;
}

static gint dissect_type0b_message(proto_tree *tree, tvbuff_t *tvb, gint offset)
{
    proto_tree_add_item(tree, hf_dplay_type_0b_padding_1, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_0b_dpid, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_0b_padding_2, tvb, offset, 12, FALSE); offset += 12;
    return offset;
}

static gint dissect_type0d_message(proto_tree *tree, tvbuff_t *tvb, gint offset)
{
    proto_tree_add_item(tree, hf_dplay_type_0d_padding_1, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_0d_dpid_1, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_0d_dpid_2, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_0d_padding_2, tvb, offset, 8, FALSE); offset += 8;
    return offset;
}

static gint dissect_type0e_message(proto_tree *tree, tvbuff_t *tvb, gint offset)
{
    proto_tree_add_item(tree, hf_dplay_type_0e_padding_1, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_0e_dpid_1, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_0e_dpid_2, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_0e_padding_2, tvb, offset, 8, FALSE); offset += 8;
    return offset;
}

static gint dissect_type0f_message(proto_tree *tree, tvbuff_t *tvb, gint offset)
{
    proto_tree_add_item(tree, hf_dplay_data_type_0f, tvb, offset, -1, FALSE);
    return offset;
}

static gint dissect_type13_message(proto_tree *tree, tvbuff_t *tvb, gint offset)
{
    proto_item *first_saddr_item = NULL, *second_saddr_item = NULL;
    proto_tree *first_saddr_tree = NULL, *second_saddr_tree = NULL;

    proto_tree_add_item(tree, hf_dplay_type_13_padding_1, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_13_dpid_1, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_13_unknown_1, tvb, offset, 20, FALSE); offset += 20;
    proto_tree_add_item(tree, hf_dplay_type_13_dpid_2, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_13_unknown_2, tvb, offset, 20, FALSE); offset += 20;
    proto_tree_add_item(tree, hf_dplay_type_13_dpid_3, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_13_unknown_3, tvb, offset, 12, FALSE); offset += 12;

    first_saddr_item = proto_tree_add_text(tree, tvb, offset, 16,
            "DirectPlay message type 0x0013 s_addr_in structure 1");
    first_saddr_tree = proto_item_add_subtree(first_saddr_item, ett_dplay_type13_saddr1);

    proto_tree_add_item(first_saddr_tree, hf_dplay_type_13_saddr_af_1, tvb, offset, 2, TRUE); offset += 2;
    proto_tree_add_item(first_saddr_tree, hf_dplay_type_13_saddr_port_1, tvb, offset, 2, FALSE); offset += 2;
    proto_tree_add_item(first_saddr_tree, hf_dplay_type_13_saddr_ip_1, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(first_saddr_tree, hf_dplay_type_13_saddr_padd_1, tvb, offset, 8, FALSE); offset += 8;

    second_saddr_item = proto_tree_add_text(tree, tvb, offset, 16,
            "DirectPlay message type 0x0013 s_addr_in structure 2");
    second_saddr_tree = proto_item_add_subtree(second_saddr_item, ett_dplay_type13_saddr2);

    proto_tree_add_item(second_saddr_tree, hf_dplay_type_13_saddr_af_2, tvb, offset, 2, TRUE); offset += 2;
    proto_tree_add_item(second_saddr_tree, hf_dplay_type_13_saddr_port_2, tvb, offset, 2, FALSE); offset += 2;
    proto_tree_add_item(second_saddr_tree, hf_dplay_type_13_saddr_ip_2, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(second_saddr_tree, hf_dplay_type_13_saddr_padd_2, tvb, offset, 8, FALSE); offset += 8;

    proto_tree_add_item(tree, hf_dplay_type_13_padding_2, tvb, offset, 2, FALSE); offset += 2;
    proto_tree_add_item(tree, hf_dplay_type_13_dpid_4, tvb, offset, 4, FALSE); offset += 4;

    return offset;
}

static gint dissect_type15_message(proto_tree *tree, tvbuff_t *tvb, gint offset)
{
    guint16 second_message_type;
    proto_item *enc_item = NULL;
    proto_tree *enc_tree = NULL;
    second_message_type = tvb_get_letohs(tvb, 72);

    proto_tree_add_item(tree, hf_dplay_container_guid, tvb, offset, 16, FALSE); offset += 16;
    proto_tree_add_item(tree, hf_dplay_type_15_padding_1, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_15_size_1, tvb, offset, 4, TRUE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_15_padding_2, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_15_unknown_1, tvb, offset, 4, TRUE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_15_size_2, tvb, offset, 4, TRUE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_15_padding_3, tvb, offset, 4, FALSE); offset += 4;

    enc_item = proto_tree_add_text(tree, tvb, offset, -1, "DirectPlay encapsulated packet");
    enc_tree = proto_item_add_subtree(enc_item, ett_dplay_enc_packet);

    proto_tree_add_item(enc_tree, hf_dplay_play_str_2, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(enc_tree, hf_dplay_command_2, tvb, offset, 2, TRUE); offset += 2;
    proto_tree_add_item(enc_tree, hf_dplay_proto_dialect_2, tvb, offset, 2, TRUE); offset += 2;

    switch(second_message_type)
    {
        case 0x0005:
            offset = dissect_type05_message(enc_tree, tvb, offset);
            break;
        case 0x0007:
            offset = dissect_type05_message(enc_tree, tvb, offset);
            break;
        case 0x0008:
            offset = dissect_type08_message(enc_tree, tvb, offset);
            break;
        case 0x000b:
            offset = dissect_type0b_message(enc_tree, tvb, offset);
            break;
        case 0x0013:
            offset = dissect_type13_message(enc_tree, tvb, offset);
            break;
        case 0x001a:
            offset = dissect_type1a_message(enc_tree, tvb, offset);
            break;
        case 0x002e:
            offset = dissect_type2e_message(enc_tree, tvb, offset);
            break;
    }

    return offset;
}

static gint dissect_type16_message(proto_tree *tree, tvbuff_t *tvb, gint offset)
{
    proto_tree_add_item(tree, hf_dplay_type_16_data, tvb, offset, -1, FALSE);
    return offset;
}

static gint dissect_type17_message(proto_tree *tree, tvbuff_t *tvb, gint offset)
{
    proto_tree_add_item(tree, hf_dplay_type_17_data, tvb, offset, -1, FALSE);
    return offset;
}

static gint dissect_type1a_message(proto_tree *tree, tvbuff_t *tvb, gint offset)
{
    proto_tree_add_item(tree, hf_dplay_data_type_1a, tvb, offset, -1, FALSE);
    return offset;
}

static gint dissect_type29_message(proto_tree *tree, tvbuff_t *tvb, gint offset)
{
    proto_item *first_saddr_item = NULL,
	       *second_saddr_item = NULL, *third_saddr_item = NULL,
	       *fourth_saddr_item = NULL;
    proto_tree *first_saddr_tree = NULL,
	       *second_saddr_tree = NULL, *third_saddr_tree = NULL,
	       *fourth_saddr_tree = NULL;

    proto_tree_add_item(tree, hf_dplay_type_29_unknown_uint32_01, tvb, offset, 4, TRUE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_29_message_end_type, tvb, offset, 4, TRUE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_29_unknown_uint32_03, tvb, offset, 4, TRUE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_29_unknown_uint32_04, tvb, offset, 4, TRUE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_29_unknown_uint32_05, tvb, offset, 4, TRUE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_29_unknown_uint32_06, tvb, offset, 4, TRUE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_29_unknown_uint32_07, tvb, offset, 4, TRUE); offset += 4;
    offset =  dissect_session_desc(tree, tvb, offset);

    offset = display_unicode_string(tree, hf_dplay_type_29_game_name, tvb, offset);
    proto_tree_add_item(tree, hf_dplay_type_29_unknown_uint32_10, tvb, offset, 4, TRUE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_29_unknown_uint32_11, tvb, offset, 4, TRUE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_29_dpid_2, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_29_unknown_uint32_12, tvb, offset, 4, TRUE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_29_unknown_uint32_13, tvb, offset, 4, TRUE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_29_saddr_field_len_1, tvb, offset, 1, FALSE); offset += 1;

    first_saddr_item = proto_tree_add_text(tree, tvb, offset, 16,
            "DirectPlay message type 0x0029 s_addr_in structure 1");
    first_saddr_tree = proto_item_add_subtree(first_saddr_item, ett_dplay_type29_saddr1);

    proto_tree_add_item(first_saddr_tree, hf_dplay_type_29_saddr_af_1, tvb, offset, 2, TRUE); offset += 2;
    proto_tree_add_item(first_saddr_tree, hf_dplay_type_29_saddr_port_1, tvb, offset, 2, FALSE); offset += 2;
    proto_tree_add_item(first_saddr_tree, hf_dplay_type_29_saddr_ip_1, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(first_saddr_tree, hf_dplay_type_29_saddr_padd_1, tvb, offset, 8, FALSE); offset += 8;

    second_saddr_item = proto_tree_add_text(tree, tvb, offset, 16,
            "DirectPlay message type 0x0029 s_addr_in structure 2");
    second_saddr_tree = proto_item_add_subtree(second_saddr_item, ett_dplay_type29_saddr2);

    proto_tree_add_item(second_saddr_tree, hf_dplay_type_29_saddr_af_2, tvb, offset, 2, TRUE); offset += 2;
    proto_tree_add_item(second_saddr_tree, hf_dplay_type_29_saddr_port_2, tvb, offset, 2, FALSE); offset += 2;
    proto_tree_add_item(second_saddr_tree, hf_dplay_type_29_saddr_ip_2, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(second_saddr_tree, hf_dplay_type_29_saddr_padd_2, tvb, offset, 8, FALSE); offset += 8;

    proto_tree_add_item(tree, hf_dplay_type_29_unknown_uint32_14, tvb, offset, 4, TRUE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_29_unknown_uint32_15, tvb, offset, 4, TRUE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_29_dpid_3, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_29_unknown_uint32_16, tvb, offset, 4, TRUE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_29_unknown_uint32_17, tvb, offset, 4, TRUE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_29_saddr_field_len_2, tvb, offset, 1, FALSE); offset += 1;

    third_saddr_item = proto_tree_add_text(tree, tvb, offset, 16,
            "DirectPlay message type 0x0029 s_addr_in structure 3");
    third_saddr_tree = proto_item_add_subtree(third_saddr_item,
		    ett_dplay_type29_saddr3);

    proto_tree_add_item(third_saddr_tree, hf_dplay_type_29_saddr_af_3, tvb, offset, 2, TRUE); offset += 2;
    proto_tree_add_item(third_saddr_tree, hf_dplay_type_29_saddr_port_3, tvb, offset, 2, FALSE); offset += 2;
    proto_tree_add_item(third_saddr_tree, hf_dplay_type_29_saddr_ip_3, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(third_saddr_tree, hf_dplay_type_29_saddr_padd_3, tvb, offset, 8, FALSE); offset += 8;

    fourth_saddr_item = proto_tree_add_text(tree, tvb, offset, 16,
            "DirectPlay message type 0x0029 s_addr_in structure 4");
    fourth_saddr_tree = proto_item_add_subtree(fourth_saddr_item,
		    ett_dplay_type29_saddr4);

    proto_tree_add_item(fourth_saddr_tree, hf_dplay_type_29_saddr_af_4, tvb, offset, 2, TRUE); offset += 2;
    proto_tree_add_item(fourth_saddr_tree, hf_dplay_type_29_saddr_port_4, tvb, offset, 2, FALSE); offset += 2;
    proto_tree_add_item(fourth_saddr_tree, hf_dplay_type_29_saddr_ip_4, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(fourth_saddr_tree, hf_dplay_type_29_saddr_padd_4, tvb, offset, 8, FALSE); offset += 8;

    proto_tree_add_item(tree, hf_dplay_type_29_unknown_uint32_18, tvb, offset, 4, TRUE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_29_unknown_uint32_19, tvb, offset, 4, TRUE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_29_dpid_4, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_29_unknown_uint32_20, tvb, offset, 4, TRUE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_29_dpid_5, tvb, offset, 4, FALSE); offset += 4;

    /* still some parts missing here */
    proto_tree_add_item(tree, hf_dplay_data_type_29, tvb, offset, -1, FALSE);
    /* here we parse another saddr_field_len and two saddr structs */
    return offset;
}

static gint dissect_type2e_message(proto_tree *tree, tvbuff_t *tvb, gint offset)
{
    proto_item *first_saddr_item = NULL, *second_saddr_item = NULL;
    proto_tree *first_saddr_tree = NULL, *second_saddr_tree = NULL;
    gint string_1_len, string_2_len;

    proto_tree_add_item(tree, hf_dplay_type_2e_padding_1, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_2e_dpid_1, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_2e_unknown_1, tvb, offset, 20, FALSE); offset += 20;
    proto_tree_add_item(tree, hf_dplay_type_2e_dpid_2, tvb, offset, 4, FALSE); offset += 4;
    string_1_len = tvb_get_letohl(tvb, offset);
    proto_tree_add_item(tree, hf_dplay_type_2e_string_1_len, tvb, offset, 4, TRUE); offset += 4;
    string_2_len = tvb_get_letohl(tvb, offset);
    proto_tree_add_item(tree, hf_dplay_type_2e_string_2_len, tvb, offset, 4, TRUE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_2e_unknown_2, tvb, offset, 12, FALSE); offset += 12;
    proto_tree_add_item(tree, hf_dplay_type_2e_dpid_3, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_2e_unknown_3, tvb, offset, 12, FALSE); offset += 12;

    if(string_1_len > 0)
        offset = display_unicode_string(tree, hf_dplay_type_2e_string_1, tvb, offset);
    if(string_2_len > 0)
        offset = display_unicode_string(tree, hf_dplay_type_2e_string_2, tvb, offset);

    first_saddr_item = proto_tree_add_text(tree, tvb, offset, 16,
            "DirectPlay message type 0x002e s_addr_in structure 1");
    first_saddr_tree = proto_item_add_subtree(first_saddr_item, ett_dplay_type2e_saddr1);

    proto_tree_add_item(first_saddr_tree, hf_dplay_type_2e_saddr_af_1, tvb, offset, 2, TRUE); offset += 2;
    proto_tree_add_item(first_saddr_tree, hf_dplay_type_2e_saddr_port_1, tvb, offset, 2, FALSE); offset += 2;
    proto_tree_add_item(first_saddr_tree, hf_dplay_type_2e_saddr_ip_1, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(first_saddr_tree, hf_dplay_type_2e_saddr_padd_1, tvb, offset, 8, FALSE); offset += 8;

    second_saddr_item = proto_tree_add_text(tree, tvb, offset, 16,
            "DirectPlay message type 0x002e s_addr_in structure 2");
    second_saddr_tree = proto_item_add_subtree(second_saddr_item, ett_dplay_type2e_saddr2);

    proto_tree_add_item(second_saddr_tree, hf_dplay_type_2e_saddr_af_2, tvb, offset, 2, TRUE); offset += 2;
    proto_tree_add_item(second_saddr_tree, hf_dplay_type_2e_saddr_port_2, tvb, offset, 2, FALSE); offset += 2;
    proto_tree_add_item(second_saddr_tree, hf_dplay_type_2e_saddr_ip_2, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(second_saddr_tree, hf_dplay_type_2e_saddr_padd_2, tvb, offset, 8, FALSE); offset += 8;

    return offset;
}

static gint dissect_type2f_message(proto_tree *tree, tvbuff_t *tvb, gint offset)
{
    proto_tree_add_item(tree, hf_dplay_type_2f_dpid, tvb, offset, 4, FALSE); offset += 4;
    return offset;
}

static gint dissect_type38_message(proto_tree *tree, tvbuff_t *tvb, gint offset)
{
    proto_item *first_saddr_item = NULL, *second_saddr_item = NULL;
    proto_tree *first_saddr_tree = NULL, *second_saddr_tree = NULL;
    gint string_1_len, string_2_len;

    proto_tree_add_item(tree, hf_dplay_type_38_padding_1, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_38_dpid_1, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_38_unknown_1, tvb, offset, 20, FALSE); offset += 20;
    proto_tree_add_item(tree, hf_dplay_type_38_dpid_2, tvb, offset, 4, FALSE); offset += 4;
    string_1_len = tvb_get_letohl(tvb, offset);
    proto_tree_add_item(tree, hf_dplay_type_38_string_1_len, tvb, offset, 4, TRUE); offset += 4;
    string_2_len = tvb_get_letohl(tvb, offset);
    proto_tree_add_item(tree, hf_dplay_type_38_string_2_len, tvb, offset, 4, TRUE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_38_unknown_2, tvb, offset, 12, FALSE); offset += 12;
    proto_tree_add_item(tree, hf_dplay_type_38_dpid_3, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(tree, hf_dplay_type_38_unknown_3, tvb, offset, 12, FALSE); offset += 12;

    if(string_1_len > 0)
        offset = display_unicode_string(tree, hf_dplay_type_38_string_1, tvb, offset);
    if(string_2_len > 0)
        offset = display_unicode_string(tree, hf_dplay_type_38_string_2, tvb, offset);

    first_saddr_item = proto_tree_add_text(tree, tvb, offset, 16,
            "DirectPlay message type 0x0038 s_addr_in structure 1");
    first_saddr_tree = proto_item_add_subtree(first_saddr_item, ett_dplay_type38_saddr1);

    proto_tree_add_item(first_saddr_tree, hf_dplay_type_38_saddr_af_1, tvb, offset, 2, TRUE); offset += 2;
    proto_tree_add_item(first_saddr_tree, hf_dplay_type_38_saddr_port_1, tvb, offset, 2, FALSE); offset += 2;
    proto_tree_add_item(first_saddr_tree, hf_dplay_type_38_saddr_ip_1, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(first_saddr_tree, hf_dplay_type_38_saddr_padd_1, tvb, offset, 8, FALSE); offset += 8;

    second_saddr_item = proto_tree_add_text(tree, tvb, offset, 16,
            "DirectPlay message type 0x0038 s_addr_in structure 2");
    second_saddr_tree = proto_item_add_subtree(second_saddr_item, ett_dplay_type38_saddr2);

    proto_tree_add_item(second_saddr_tree, hf_dplay_type_38_saddr_af_2, tvb, offset, 2, TRUE); offset += 2;
    proto_tree_add_item(second_saddr_tree, hf_dplay_type_38_saddr_port_2, tvb, offset, 2, FALSE); offset += 2;
    proto_tree_add_item(second_saddr_tree, hf_dplay_type_38_saddr_ip_2, tvb, offset, 4, FALSE); offset += 4;
    proto_tree_add_item(second_saddr_tree, hf_dplay_type_38_saddr_padd_2, tvb, offset, 8, FALSE); offset += 8;

    return offset;
}

static void dissect_dplay(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
    guint16 message_type;
    guint16 second_message_type = -1;
    guint16 proto_version;
    guint16 packet_size;
    guint32 dplay_id;
    guint8 play_id[] = {'p','l','a','y'};

    packet_size = tvb_get_letohs(tvb, 0);
    dplay_id = tvb_get_letohl(tvb, 20);
    message_type = tvb_get_letohs(tvb, 24);
    proto_version = tvb_get_letohs(tvb, 26);

    if(memcmp(play_id, (guint8 *)&dplay_id, 4) != 0)
    {
        if(check_col(pinfo->cinfo, COL_PROTOCOL))
            col_set_str(pinfo->cinfo, COL_PROTOCOL, "DPLAY");
        /* Clear out stuff in the info column */
        if(check_col(pinfo->cinfo,COL_INFO))
            col_clear(pinfo->cinfo,COL_INFO);
        if(check_col(pinfo->cinfo,COL_INFO))
            col_add_fstr(pinfo->cinfo,COL_INFO, "DPlay data packet");
        return;
    }

    if(message_type == 0x0015)
    {
        second_message_type = tvb_get_letohs(tvb, 72);
    }

    if(check_col(pinfo->cinfo, COL_PROTOCOL))
        col_set_str(pinfo->cinfo, COL_PROTOCOL, "DPLAY");
    /* Clear out stuff in the info column */
    if(check_col(pinfo->cinfo,COL_INFO))
    {
        col_clear(pinfo->cinfo,COL_INFO);
    }

    if(check_col(pinfo->cinfo,COL_INFO))
    {
        if(message_type == 0x0015)
            col_add_fstr(pinfo->cinfo,COL_INFO, "%s: %s, holding a %s",
                val_to_str(proto_version, dplay_proto_dialect_val, "Unknown (0x%04x)"),
                val_to_str(message_type, dplay_command_val, "Unknown (0x%04x)"),
                val_to_str(second_message_type, dplay_command_val, "Unknown (0x%04x)"));
        else
            col_add_fstr(pinfo->cinfo,COL_INFO, "%s: %s",
                val_to_str(proto_version, dplay_proto_dialect_val, "Unknown (0x%04x)"),
                val_to_str(message_type, dplay_command_val, "Unknown (0x%04x)"));
    }

    if(tree)
    {
        proto_item *dplay_item = NULL;
        proto_item *header_item = NULL;
        proto_item *data_item = NULL;
        proto_tree *dplay_tree = NULL;
        proto_tree *dplay_header = NULL;
        proto_tree *dplay_data = NULL;
        gint offset = 0;

        dplay_item = proto_tree_add_item(tree, proto_dplay, tvb, 0, -1, FALSE);
        dplay_tree = proto_item_add_subtree(dplay_item, ett_dplay);
        header_item = proto_tree_add_text(dplay_tree, tvb, offset, DPLAY_HEADER_OFFSET, "DirectPlay header");
        dplay_header = proto_item_add_subtree(header_item, ett_dplay_header);

        offset = dissect_dplay_header(dplay_header, tvb, offset);

        /* Special handling for empty type 0x0004 packets */
        if(message_type == 0x0004)
            return;

        data_item = proto_tree_add_text(dplay_tree, tvb, offset, -1, "DirectPlay data");
        dplay_data = proto_item_add_subtree(data_item, ett_dplay_data);

        switch(message_type)
        {
            case 0x0001:
                offset = dissect_type01_message(dplay_data, tvb, offset);
                break;
            case 0x0002:
                offset = dissect_type02_message(dplay_data, tvb, offset);
                break;
            case 0x0004:
                /* We should not get here. */
                break;
            case 0x0005:
                offset = dissect_type05_message(dplay_data, tvb, offset);
                break;
            case 0x0007:
                offset = dissect_type07_message(dplay_data, tvb, offset);
                break;
            case 0x0008:
                offset = dissect_type08_message(dplay_data, tvb, offset);
                break;
            case 0x000b:
                offset = dissect_type0b_message(dplay_data, tvb, offset);
                break;
            case 0x000d:
                offset = dissect_type0d_message(dplay_data, tvb, offset);
                break;
            case 0x000e:
                offset = dissect_type0e_message(dplay_data, tvb, offset);
                break;
            case 0x000f:
                offset = dissect_type0f_message(dplay_data, tvb, offset);
                break;
            case 0x0013:
                offset = dissect_type13_message(dplay_data, tvb, offset);
                break;
            case 0x0015:
                offset = dissect_type15_message(dplay_data, tvb, offset);
                break;
            case 0x0016:
                offset = dissect_type16_message(dplay_data, tvb, offset);
                break;
            case 0x0017:
                offset = dissect_type17_message(dplay_data, tvb, offset);
                break;
            case 0x001a:
                offset = dissect_type1a_message(dplay_data, tvb, offset);
                break;
            case 0x0029:
                offset = dissect_type29_message(dplay_data, tvb, offset);
                break;
            case 0x002e:
                offset = dissect_type2e_message(dplay_data, tvb, offset);
                break;
            case 0x002f:
                offset = dissect_type2f_message(dplay_data, tvb, offset);
                break;
            case 0x0038:
                offset = dissect_type38_message(dplay_data, tvb, offset);
                break;
        }
    }

}

static gboolean heur_dissect_dplay(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
    guint8 signature[] = {'p','l','a','y'};
    guint32 dplay_id;

    if(!tvb_bytes_exist(tvb, 0, 24))
        return FALSE;

    dplay_id = tvb_get_letohl(tvb, 20);
    if( memcmp(signature, (guint8 *)&dplay_id, 4) != 0)
        return FALSE;

    dissect_dplay(tvb, pinfo, tree);
    return TRUE;
}

void proto_register_dplay()
{
    static hf_register_info hf [] = {
    /* Common data fields */
    { &hf_dplay_size,
        { "DirectPlay package size", "dplay.size", FT_UINT32, BASE_DEC,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_token,
        { "DirectPlay token", "dplay.token", FT_UINT32, BASE_HEX,
        VALS(dplay_token_val), 0x0, "", HFILL}},
    { &hf_dplay_saddr_af,
        { "DirectPlay s_addr_in address family", "dplay.saddr.af", FT_UINT16, BASE_HEX,
        VALS(dplay_af_val), 0x0, "", HFILL}},
    { &hf_dplay_saddr_port,
        { "DirectPlay s_addr_in port", "dplay.saddr.port", FT_UINT16, BASE_DEC,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_saddr_ip,
        { "DirectPlay s_addr_in ip address", "dplay.saddr.ip", FT_IPv4, BASE_NONE,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_saddr_padding,
        { "DirectPlay s_addr_in null padding", "dplay.saddr.padding", FT_BYTES, BASE_NONE,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_play_str,
        { "DirectPlay action string", "dplay.dplay_str", FT_STRING, BASE_NONE,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_command,
        { "DirectPlay command", "dplay.command", FT_UINT16, BASE_HEX,
        VALS(dplay_command_val), 0x0, "", HFILL}},
    { &hf_dplay_proto_dialect,
        { "DirectPlay dialect version", "dplay.dialect.version", FT_UINT16, BASE_HEX,
        VALS(dplay_proto_dialect_val), 0x0, "", HFILL}},
    { &hf_dplay_play_str_2,
        { "DirectPlay second action string", "dplay.dplay_str_2", FT_STRING, BASE_NONE,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_command_2,
        { "DirectPlay second command", "dplay.type_2", FT_UINT16, BASE_HEX,
        VALS(dplay_command_val), 0x0, "", HFILL}},
    { &hf_dplay_proto_dialect_2,
        { "DirectPlay second dialect version", "dplay.dialect.version_2", FT_UINT16, BASE_HEX,
        VALS(dplay_proto_dialect_val), 0x0, "", HFILL}},

    /* special fields, to be phased out*/
    { &hf_dplay_data_type_0f,
        { "DirectPlay data for type 0f messages", "dplay.data.type_0f", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_data_type_1a,
        { "DirectPlay data for type 1a messages", "dplay.data.type_1a", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_data_type_29,
        { "DirectPlay data for type 29 messages", "dplay.data.type_29", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},

    /* Session Desc structure fields */
    { &hf_dplay_sess_desc_flags,
        { "DirectPlay session desc flags", "dplay.flags", FT_UINT32, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_flags_no_create_players,
        { "no create players flag", "dplay.flags.no_create_players", FT_BOOLEAN, 32,
        TFS(&tfs_dplay_flag), DPLAY_FLAG_NO_CREATE_PLAYERS, "No Create Players", HFILL}},
    { &hf_dplay_flags_0002,
        { "unused", "dplay.flags.unused", FT_BOOLEAN, 32,
        TFS(&tfs_dplay_flag), DPLAY_FLAG_0002, "Unused", HFILL}},
    { &hf_dplay_flags_migrate_host,
        { "migrate host flag", "dplay.flags.migrate_host", FT_BOOLEAN, 32,
        TFS(&tfs_dplay_flag), DPLAY_FLAG_MIGRATE_HOST, "Migrate Host", HFILL}},
    { &hf_dplay_flags_short_player_msg,
        { "short player message", "dplay.flags.short_player_msg", FT_BOOLEAN, 32,
        TFS(&tfs_dplay_flag), DPLAY_FLAG_SHORT_PLAYER_MSG, "Short Player Msg", HFILL}},
    { &hf_dplay_flags_ignored,
        { "ignored", "dplay.flags.ignored", FT_BOOLEAN, 32,
        TFS(&tfs_dplay_flag), DPLAY_FLAG_IGNORED, "Ignored", HFILL}},
    { &hf_dplay_flags_can_join,
        { "can join", "dplay.flags.can_join", FT_BOOLEAN, 32,
        TFS(&tfs_dplay_flag), DPLAY_FLAG_CAN_JOIN, "Can Join", HFILL}},
    { &hf_dplay_flags_use_ping,
        { "use ping", "dplay.flags.use_ping", FT_BOOLEAN, 32,
        TFS(&tfs_dplay_flag), DPLAY_FLAG_USE_PING, "Use Ping", HFILL}},
    { &hf_dplay_flags_no_player_updates,
        { "no player updates", "dplay.flags.no_player_updates", FT_BOOLEAN, 32,
        TFS(&tfs_dplay_flag), DPLAY_FLAG_NO_P_UPD, "No Player Updates", HFILL}},
    { &hf_dplay_flags_use_auth,
        { "use authentication", "dplay.flags.use_auth", FT_BOOLEAN, 32,
        TFS(&tfs_dplay_flag), DPLAY_FLAG_USE_AUTH, "Use Auth", HFILL}},
    { &hf_dplay_flags_private_session,
        { "private session", "dplay.flags.priv_sess", FT_BOOLEAN, 32,
        TFS(&tfs_dplay_flag), DPLAY_FLAG_PRIV_SESS, "Priv Session", HFILL}},
    { &hf_dplay_flags_password_req,
        { "password required", "dplay.flags.pass_req", FT_BOOLEAN, 32,
        TFS(&tfs_dplay_flag), DPLAY_FLAG_PASS_REQ, "Pass Req", HFILL}},
    { &hf_dplay_flags_route,
        { "route via game host", "dplay.flags.route", FT_BOOLEAN, 32,
        TFS(&tfs_dplay_flag), DPLAY_FLAG_ROUTE, "Route", HFILL}},
    { &hf_dplay_flags_server_player_only,
        { "get server player only", "dplay.flags.srv_p_only", FT_BOOLEAN, 32,
        TFS(&tfs_dplay_flag), DPLAY_FLAG_SRV_ONLY, "Svr Player Only", HFILL}},
    { &hf_dplay_flags_reliable,
        { "use reliable protocol", "dplay.flags.reliable", FT_BOOLEAN, 32,
        TFS(&tfs_dplay_flag), DPLAY_FLAG_RELIABLE, "Reliable", HFILL}},
    { &hf_dplay_flags_preserve_order,
        { "preserve order", "dplay.flags.order", FT_BOOLEAN, 32,
        TFS(&tfs_dplay_flag), DPLAY_FLAG_ORDER, "Order", HFILL}},
    { &hf_dplay_flags_optimize_latency,
        { "optimize for latency", "dplay.flags.opt_latency", FT_BOOLEAN, 32,
        TFS(&tfs_dplay_flag), DPLAY_FLAG_OPT_LAT, "Opt Latency", HFILL}},
    { &hf_dplay_flags_acqire_voice,
        { "acquire voice", "dplay.flags.acq_voice", FT_BOOLEAN, 32,
        TFS(&tfs_dplay_flag), DPLAY_FLAG_ACQ_VOICE, "Acq Voice", HFILL}},
    { &hf_dplay_flags_no_sess_desc_changes,
        { "no session desc changes", "dplay.flags.no_sess_desc", FT_BOOLEAN, 32,
        TFS(&tfs_dplay_flag), DPLAY_FLAG_NO_SESS_DESC_CHANGES, "No Sess Desc Changes", HFILL}},
    { &hf_dplay_instance_guid,
        { "DirectPlay instance guid", "dplay.instance.guid", FT_GUID, BASE_NONE,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_game_guid,
        { "DirectPlay game GUID", "dplay.game.guid", FT_GUID, BASE_NONE,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_sess_desc_length,
        { "DirectPlay session desc length", "dplay.sess_desc.length", FT_UINT32, BASE_DEC,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_max_players,
        { "DirectPlay max players ", "dplay.sess_desc.max_players", FT_UINT32, BASE_DEC,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_curr_players,
        { "DirectPlay current players", "dplay.sess_desc.curr_players", FT_UINT32, BASE_DEC,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_sess_name_ptr,
        { "Session description name pointer placeholder", "dplay.sess_desc.name_ptr", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_passwd_ptr,
        { "Session description password pointer placeholder", "dplay.sess_desc.pw_ptr", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_sess_desc_reserved_1,
        { "Session description reserved 1", "dplay.sess_desc.res_1", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_sess_desc_reserved_2,
        { "Session description reserved 2", "dplay.sess_desc.res_2", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_sess_desc_user_1,
        { "Session description user defined 1", "dplay.sess_desc.user_1", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_sess_desc_user_2,
        { "Session description user defined 2", "dplay.sess_desc.user_2", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_sess_desc_user_3,
        { "Session description user defined 3", "dplay.sess_desc.user_3", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_sess_desc_user_4,
        { "Session description user defined 4", "dplay.sess_desc.user_4", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},

    /* Data fields for message type 0x0001 */
    { &hf_dplay_type_01_name_offset,
        { "Enum Session Reply name offset", "dplay.type_01.name_offs", FT_UINT32, BASE_DEC,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_01_game_name,
        { "Enum Session Reply game name", "dplay.type_01.game_name", FT_STRING, BASE_NONE,
        NULL, 0x0, "", HFILL}},

    /* Data fields for message type 0x0002 */
    { &hf_dplay_type_02_game_guid,
        { "DirectPlay game GUID", "dplay.type02.game.guid", FT_GUID, BASE_NONE,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_02_ignored,
        { "DirectPlay message type 0x0002 ignored", "dplay.type02.ignored", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},

    /* Data fields for message type 0x0005 */
    { &hf_dplay_type_05_request,
        { "DirectPlay ID request", "dplay.type_05.request", FT_UINT32, BASE_HEX,
        VALS(dplay_type05_request), 0x0, "", HFILL}},

    /* Data fields for message type 0x0007 */
    { &hf_dplay_type_07_dpid,
        { "DirectPlay ID", "dplay.type_07.dpid", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_07_padding,
        { "DirectPlay message type 0x0007 padding", "dplay.type_07.padding", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},

    /* Data fields for message type 0x0008 */
    { &hf_dplay_type_08_padding_1,
        { "DirectPlay message type 0x0008 padding 1", "dplay.type_08.padding_1", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_08_dpid_1,
        { "DirectPlay message type 0x0008 client DP ID", "dplay.type_08.dpid_1", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_08_unknown_1,
        { "DirectPlay message type 0x0008 unknown 1", "dplay.type_08.unknown_1", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_08_dpid_2,
        { "DirectPlay message type 0x0008 client DP ID", "dplay.type_08.dpid_2", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_08_string_1_len,
        { "DirectPlay message type 0x0008 string 1 length", "dplay.type_08.string_1.length", FT_UINT32,
        BASE_DEC, NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_08_string_2_len,
        { "DirectPlay message type 0x0008 string 2 length", "dplay.type_08.string_2.length", FT_UINT32,
        BASE_DEC, NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_08_unknown_2,
        { "DirectPlay message type 0x0008 unknown 2", "dplay.type_08.unknown_2", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_08_dpid_3,
        { "DirectPlay message type 0x0008 client DP ID", "dplay.type_08.dpid_3", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_08_unknown_3,
        { "DirectPlay message type 0x0008 unknown 3", "dplay.type_08.unknown_3", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_08_string_1,
        { "DirectPlay message type 0x0008 string 1", "dplay.type_08.string_1", FT_STRING, BASE_NONE,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_08_string_2,
        { "DirectPlay message type 0x0008 string 2", "dplay.type_08.string_2", FT_STRING, BASE_NONE,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_08_saddr_af_1,
        { "DirectPlay message type 0x0008 s_addr_in address family 1", "dplay.type_08.saddr.af_1", FT_UINT16,
            BASE_DEC, VALS(dplay_af_val), 0x0, "", HFILL}},
    { &hf_dplay_type_08_saddr_port_1,
        { "DirectPlay message type 0x0008 s_addr_in port 1", "dplay.type_08.saddr.port_1", FT_UINT16, BASE_DEC,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_08_saddr_ip_1,
        { "DirectPlay message type 0x0008 s_addr_in ip 1", "dplay.type_08.saddr.ip_1", FT_IPv4, BASE_NONE,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_08_saddr_padd_1,
        { "DirectPlay message type 0x0008 s_addr_in padding 1", "dplay.type_08.saddr.padd_1", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_08_saddr_af_2,
        { "DirectPlay message type 0x0008 s_addr_in address family 2", "dplay.type_08.saddr.af_2", FT_UINT16,
            BASE_DEC, VALS(dplay_af_val), 0x0, "", HFILL}},
    { &hf_dplay_type_08_saddr_port_2,
        { "DirectPlay message type 0x0008 s_addr_in port 2", "dplay.type_08.saddr.port_2", FT_UINT16, BASE_DEC,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_08_saddr_ip_2,
        { "DirectPlay message type 0x0008 s_addr_in ip 2", "dplay.type_08.saddr.ip_2", FT_IPv4, BASE_NONE,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_08_saddr_padd_2,
        { "DirectPlay message type 0x0008 s_addr_in padding 2", "dplay.type_08.saddr.padd_2", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_08_padding_2,
        { "DirectPlay message type 0x0008 padding 2", "dplay.type_08.padding_2", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},

    /* Data fields for message type 0x000b */
    { &hf_dplay_type_0b_padding_1,
        { "DirectPlay message type 0x000b padding 1", "dplay.type_0b.padding_1", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_0b_dpid,
        { "DirectPlay message type 0x000b DP ID", "dplay.type_0b.dpid", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_0b_padding_2,
        { "DirectPlay message type 0x000b padding 2", "dplay.type_0b.padding_2", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},

    /* Data fields for message type 0x000d */
    { &hf_dplay_type_0d_padding_1,
        { "DirectPlay message type 0x000d padding 1", "dplay.type_0d.padding_1", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_0d_dpid_1,
        { "DirectPlay message type 0x000d DP ID 1", "dplay.type_0d.dpid_1", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_0d_dpid_2,
        { "DirectPlay message type 0x000d DP ID 2", "dplay.type_0d.dpid_2", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_0d_padding_2,
        { "DirectPlay message type 0x000d padding 2", "dplay.type_0d.padding_2", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},

    /* Data fields for message type 0x000e */
    { &hf_dplay_type_0e_padding_1,
        { "DirectPlay message type 0x000e padding 1", "dplay.type_0e.padding_1", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_0e_dpid_1,
        { "DirectPlay message type 0x000e DP ID 1", "dplay.type_0e.dpid_1", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_0e_dpid_2,
        { "DirectPlay message type 0x000e DP ID 2", "dplay.type_0e.dpid_2", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_0e_padding_2,
        { "DirectPlay message type 0x000e padding 2", "dplay.type_0e.padding_2", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},

    /* Data fields for message type 0x0013 */
    { &hf_dplay_type_13_padding_1,
        { "DirectPlay message type 0x0013 padding 1", "dplay.type_13.padding_1", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_13_dpid_1,
        { "DirectPlay message type 0x0013 client DP ID", "dplay.type_13.dpid_1", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_13_unknown_1,
        { "DirectPlay message type 0x0013 unknown 1", "dplay.type_13.unknown_1", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_13_dpid_2,
        { "DirectPlay message type 0x0013 client DP ID", "dplay.type_13.dpid_2", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_13_unknown_2,
        { "DirectPlay message type 0x0013 unknown 2", "dplay.type_13.unknown_2", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_13_dpid_3,
        { "DirectPlay message type 0x0013 client DP ID", "dplay.type_13.dpid_3", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_13_unknown_3,
        { "DirectPlay message type 0x0013 unknown 3", "dplay.type_13.unknown_3", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_13_saddr_af_1,
        { "DirectPlay message type 0x0013 s_addr_in address family 1", "dplay.type_13.saddr.af_1", FT_UINT16,
            BASE_DEC, VALS(dplay_af_val), 0x0, "", HFILL}},
    { &hf_dplay_type_13_saddr_port_1,
        { "DirectPlay message type 0x0013 s_addr_in port 1", "dplay.type_13.saddr.port_1", FT_UINT16, BASE_DEC,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_13_saddr_ip_1,
        { "DirectPlay message type 0x0013 s_addr_in ip 1", "dplay.type_13.saddr.ip_1", FT_IPv4, BASE_NONE,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_13_saddr_padd_1,
        { "DirectPlay message type 0x0013 s_addr_in padding 1", "dplay.type_13.saddr.padd_1", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_13_saddr_af_2,
        { "DirectPlay message type 0x0013 s_addr_in address family 2", "dplay.type_13.saddr.af_2", FT_UINT16,
            BASE_DEC, VALS(dplay_af_val), 0x0, "", HFILL}},
    { &hf_dplay_type_13_saddr_port_2,
        { "DirectPlay message type 0x0013 s_addr_in port 2", "dplay.type_13.saddr.port_2", FT_UINT16, BASE_DEC,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_13_saddr_ip_2,
        { "DirectPlay message type 0x0013 s_addr_in ip 2", "dplay.type_13.saddr.ip_2", FT_IPv4, BASE_NONE,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_13_saddr_padd_2,
        { "DirectPlay message type 0x0013 s_addr_in padding 2", "dplay.type_13.saddr.padd_2", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_13_padding_2,
        { "DirectPlay message type 0x0013 padding 2", "dplay.type_13.padding_2", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_13_dpid_4,
        { "DirectPlay message type 0x0013 server DP ID", "dplay.type_13.dpid_4", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},

    /* Data fields for message type 0x0015 */
    { &hf_dplay_container_guid,
        { "DirectPlay container GUID", "dplay.container.guid", FT_GUID, BASE_NONE,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_15_padding_1,
        { "DirectPlay message type 0x0015 padding 1", "dplay.type_15.padding_1", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_15_size_1,
        { "DirectPlay encapsulated packet size 1", "dplay.type_15.encap_size_1", FT_UINT32, BASE_DEC,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_15_padding_2,
        { "DirectPlay message type 0x0015 padding 2", "dplay.type_15.padding_2", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_15_unknown_1,
        { "DirectPlay message type 0x0015 unknown", "dplay.type_15.unknown_1", FT_UINT32, BASE_DEC,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_15_size_2,
        { "DirectPlay encapsulated packet size 2", "dplay.type_15.encap_size_2", FT_UINT32, BASE_DEC,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_15_padding_3,
        { "DirectPlay message type 0x0015 padding 3", "dplay.type_15.padding_3", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},

    /* Data field for message type 0x0016 */
    { &hf_dplay_type_16_data,
        { "DirectPlay type 0x0016 message data", "dplay.data.type_16", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},

    /* Data field for message type 0x0017 */
    { &hf_dplay_type_17_data,
        { "DirectPlay type 0x0017 message data", "dplay.data.type_17", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},

    /* Data fields for message type 0x0029 */
    { &hf_dplay_type_29_unknown_uint32_01,
        { "DirectPlay message type 0x0029 unknown uint32 1 (3)", "dplay.type_29.unknown_uint32_01", FT_UINT32,
        BASE_DEC, NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_29_message_end_type,
        { "DirectPlay message type 0x0029 message end type", "dplay.type_29.msg_end_type", FT_UINT32,
        BASE_DEC, VALS(dplay_type29_end_type), 0x0, "", HFILL}},
    { &hf_dplay_type_29_unknown_uint32_03,
        { "DirectPlay message type 0x0029 unknown uint32 3", "dplay.type_29.unknown_uint32_03", FT_UINT32,
        BASE_DEC, NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_29_unknown_uint32_04,
        { "DirectPlay message type 0x0029 unknown uint32 4 (0)", "dplay.type_29.unknown_uint32_04", FT_UINT32,
        BASE_DEC, NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_29_unknown_uint32_05,
        { "DirectPlay message type 0x0029 unknown uint32 5 (36)", "dplay.type_29.unknown_uint32_05", FT_UINT32,
        BASE_DEC, NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_29_unknown_uint32_06,
        { "DirectPlay message type 0x0029 unknown uint32 6 (116)", "dplay.type_29.unknown_uint32_06", FT_UINT32,
        BASE_DEC, NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_29_unknown_uint32_07,
        { "DirectPlay message type 0x0029 unknown uint32 7 (0)", "dplay.type_29.unknown_uint32_07", FT_UINT32,
        BASE_DEC, NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_29_unknown_uint32_08,
        { "DirectPlay message type 0x0029 unknown uint32 8 (80)", "dplay.type_29.unknown_uint32_08", FT_UINT32,
        BASE_DEC, NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_29_magic_16_bytes,
        { "DirectPlay message type 0x0029 magic 16 bytes", "dplay.type_29.magic_16_bytes", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_29_dpid_1,
        { "DirectPlay message type 0x0029 DPID", "dplay.type_29.dpid", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_29_unknown_3,
        { "DirectPlay message type 0x0029 unknown 3", "dplay.type_29.unknown_3", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_29_game_name,
        { "DirectPlay message type 0x0029 game name", "dplay.type_29.game_name", FT_STRING, BASE_NONE,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_29_unknown_uint32_10,
        { "DirectPlay message type 0x0029 unknown uint32 10", "dplay.type_29.unknown_uint32_10", FT_UINT32,
        BASE_DEC, NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_29_unknown_uint32_11,
        { "DirectPlay message type 0x0029 unknown uint32 11", "dplay.type_29.unknown_uint32_11", FT_UINT32,
        BASE_DEC, NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_29_dpid_2,
        { "DirectPlay message type 0x0029 DPID", "dplay.type_29.dpid", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_29_unknown_uint32_12,
        { "DirectPlay message type 0x0029 unknown uint32 12 (4)", "dplay.type_29.unknown_uint32_12", FT_UINT32,
        BASE_DEC, NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_29_unknown_uint32_13,
        { "DirectPlay message type 0x0029 unknown uint32 13 (14)", "dplay.type_29.unknown_uint32_13", FT_UINT32,
        BASE_DEC, NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_29_saddr_field_len_1,
        { "DirectPlay message type 0x0029 saddr field len 1", "dplay.type_29.saddr_field_len_1", FT_UINT8,
        BASE_DEC, NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_29_saddr_af_1,
        { "DirectPlay message type 0x0029 s_addr_in address family 1", "dplay.type_29.saddr.af_1", FT_UINT16,
            BASE_DEC, VALS(dplay_af_val), 0x0, "", HFILL}},
    { &hf_dplay_type_29_saddr_port_1,
        { "DirectPlay message type 0x0029 s_addr_in port 1", "dplay.type_29.saddr.port_1", FT_UINT16, BASE_DEC,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_29_saddr_ip_1,
        { "DirectPlay message type 0x0029 s_addr_in ip 1", "dplay.type_29.saddr.ip_1", FT_IPv4, BASE_NONE,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_29_saddr_padd_1,
        { "DirectPlay message type 0x0029 s_addr_in padding 1", "dplay.type_29.saddr.padd_1", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_29_saddr_af_2,
        { "DirectPlay message type 0x0029 s_addr_in address family 2", "dplay.type_29.saddr.af_2", FT_UINT16,
            BASE_DEC, VALS(dplay_af_val), 0x0, "", HFILL}},
    { &hf_dplay_type_29_saddr_port_2,
        { "DirectPlay message type 0x0029 s_addr_in port 2", "dplay.type_29.saddr.port_2", FT_UINT16, BASE_DEC,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_29_saddr_ip_2,
        { "DirectPlay message type 0x0029 s_addr_in ip 2", "dplay.type_29.saddr.ip_2", FT_IPv4, BASE_NONE,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_29_saddr_padd_2,
        { "DirectPlay message type 0x0029 s_addr_in padding 2", "dplay.type_29.saddr.padd_2", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_29_unknown_uint32_14,
        { "DirectPlay message type 0x0029 unknown uint32 14 (16)", "dplay.type_29.unknown_uint32_14", FT_UINT32,
        BASE_DEC, NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_29_unknown_uint32_15,
        { "DirectPlay message type 0x0029 unknown uint32 15 (15)", "dplay.type_29.unknown_uint32_15", FT_UINT32,
        BASE_DEC, NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_29_dpid_3,
        { "DirectPlay message type 0x0029 DPID", "dplay.type_29.dpid", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_29_unknown_uint32_16,
        { "DirectPlay message type 0x0029 unknown uint32 16 (4)", "dplay.type_29.unknown_uint32_16", FT_UINT32,
        BASE_DEC, NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_29_unknown_uint32_17,
        { "DirectPlay message type 0x0029 unknown uint32 17 (14)", "dplay.type_29.unknown_uint32_17", FT_UINT32,
        BASE_DEC, NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_29_saddr_field_len_2,
        { "DirectPlay message type 0x0029 saddr field len 2", "dplay.type_29.saddr_field_len_2", FT_UINT8,
        BASE_DEC, NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_29_saddr_af_3,
        { "DirectPlay message type 0x0029 s_addr_in address family 3", "dplay.type_29.saddr.af_3", FT_UINT16,
            BASE_DEC, VALS(dplay_af_val), 0x0, "", HFILL}},
    { &hf_dplay_type_29_saddr_port_3,
        { "DirectPlay message type 0x0029 s_addr_in port 3", "dplay.type_29.saddr.port_3", FT_UINT16, BASE_DEC,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_29_saddr_ip_3,
        { "DirectPlay message type 0x0029 s_addr_in ip 3", "dplay.type_29.saddr.ip_3", FT_IPv4, BASE_NONE,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_29_saddr_padd_3,
        { "DirectPlay message type 0x0029 s_addr_in padding 3", "dplay.type_29.saddr.padd_3", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_29_saddr_af_4,
        { "DirectPlay message type 0x0029 s_addr_in address family 4", "dplay.type_29.saddr.af_4", FT_UINT16,
            BASE_DEC, VALS(dplay_af_val), 0x0, "", HFILL}},
    { &hf_dplay_type_29_saddr_port_4,
        { "DirectPlay message type 0x0029 s_addr_in port 4", "dplay.type_29.saddr.port_4", FT_UINT16, BASE_DEC,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_29_saddr_ip_4,
        { "DirectPlay message type 0x0029 s_addr_in ip 4", "dplay.type_29.saddr.ip42", FT_IPv4, BASE_NONE,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_29_saddr_padd_4,
        { "DirectPlay message type 0x0029 s_addr_in padding 4", "dplay.type_29.saddr.padd_4", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_29_unknown_uint32_18,
        { "DirectPlay message type 0x0029 unknown uint32 18 (16)", "dplay.type_29.unknown_uint32_18", FT_UINT32,
        BASE_DEC, NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_29_unknown_uint32_19,
        { "DirectPlay message type 0x0029 unknown uint32 19 (15)", "dplay.type_29.unknown_uint32_19", FT_UINT32,
        BASE_DEC, NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_29_dpid_4,
        { "DirectPlay message type 0x0029 DPID", "dplay.type_29.dpid_4", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_29_unknown_uint32_20,
        { "DirectPlay message type 0x0029 unknown uint32 20", "dplay.type_29.unknown_uint32_20", FT_UINT32,
        BASE_DEC, NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_29_dpid_5,
        { "DirectPlay message type 0x0029 DPID", "dplay.type_29.dpid_5", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},

    /* Data fields for message type 0x002e */
    { &hf_dplay_type_2e_padding_1,
        { "DirectPlay message type 0x002e padding 1", "dplay.type_2e.padding_1", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_2e_dpid_1,
        { "DirectPlay message type 0x002e client DP ID", "dplay.type_2e.dpid_1", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_2e_unknown_1,
        { "DirectPlay message type 0x002e unknown 1", "dplay.type_2e.unknown_1", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_2e_dpid_2,
        { "DirectPlay message type 0x002e client DP ID", "dplay.type_2e.dpid_2", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_2e_string_1_len,
        { "DirectPlay message type 0x002e string 1 length", "dplay.type_2e.string_1.length", FT_UINT32,
        BASE_DEC, NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_2e_string_2_len,
        { "DirectPlay message type 0x002e string 2 length", "dplay.type_2e.string_2.length", FT_UINT32,
        BASE_DEC, NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_2e_unknown_2,
        { "DirectPlay message type 0x002e unknown 2", "dplay.type_2e.unknown_2", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_2e_dpid_3,
        { "DirectPlay message type 0x002e client DP ID", "dplay.type_2e.dpid_3", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_2e_unknown_3,
        { "DirectPlay message type 0x002e unknown 3", "dplay.type_2e.unknown_3", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_2e_string_1,
        { "DirectPlay message type 0x002e string 1", "dplay.type_2e.string_1", FT_STRING, BASE_NONE,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_2e_string_2,
        { "DirectPlay message type 0x002e string 2", "dplay.type_2e.string_2", FT_STRING, BASE_NONE,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_2e_saddr_af_1,
        { "DirectPlay message type 0x002e s_addr_in address family 1", "dplay.type_2e.saddr.af_1", FT_UINT16,
            BASE_DEC, VALS(dplay_af_val), 0x0, "", HFILL}},
    { &hf_dplay_type_2e_saddr_port_1,
        { "DirectPlay message type 0x002e s_addr_in port 1", "dplay.type_2e.saddr.port_1", FT_UINT16, BASE_DEC,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_2e_saddr_ip_1,
        { "DirectPlay message type 0x002e s_addr_in ip 1", "dplay.type_2e.saddr.ip_1", FT_IPv4, BASE_NONE,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_2e_saddr_padd_1,
        { "DirectPlay message type 0x002e s_addr_in padding 1", "dplay.type_2e.saddr.padd_1", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_2e_saddr_af_2,
        { "DirectPlay message type 0x002e s_addr_in address family 2", "dplay.type_2e.saddr.af_2", FT_UINT16,
            BASE_DEC, VALS(dplay_af_val), 0x0, "", HFILL}},
    { &hf_dplay_type_2e_saddr_port_2,
        { "DirectPlay message type 0x002e s_addr_in port 2", "dplay.type_2e.saddr.port_2", FT_UINT16, BASE_DEC,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_2e_saddr_ip_2,
        { "DirectPlay message type 0x002e s_addr_in ip 2", "dplay.type_2e.saddr.ip_2", FT_IPv4, BASE_NONE,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_2e_saddr_padd_2,
        { "DirectPlay message type 0x002e s_addr_in padding 2", "dplay.type_2e.saddr.padd_2", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},

    /* Data fields for message type 0x002f */
    { &hf_dplay_type_2f_dpid,
        { "DirectPlay message type 0x002f DP ID", "dplay.type_29.dpid", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},

    /* Data fields for message type 0x0038 */
    { &hf_dplay_type_38_padding_1,
        { "DirectPlay message type 0x0038 padding 1", "dplay.type_38.padding_1", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_38_dpid_1,
        { "DirectPlay message type 0x0038 client DP ID", "dplay.type_38.dpid_1", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_38_unknown_1,
        { "DirectPlay message type 0x0038 unknown 1", "dplay.type_38.unknown_1", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_38_dpid_2,
        { "DirectPlay message type 0x0038 client DP ID", "dplay.type_38.dpid_2", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_38_string_1_len,
        { "DirectPlay message type 0x0038 string 1 length", "dplay.type_38.string_1.length", FT_UINT32,
        BASE_DEC, NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_38_string_2_len,
        { "DirectPlay message type 0x0038 string 2 length", "dplay.type_38.string_2.length", FT_UINT32,
        BASE_DEC, NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_38_unknown_2,
        { "DirectPlay message type 0x0038 unknown 2", "dplay.type_38.unknown_2", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_38_dpid_3,
        { "DirectPlay message type 0x0038 client DP ID", "dplay.type_38.dpid_3", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_38_unknown_3,
        { "DirectPlay message type 0x0038 unknown 3", "dplay.type_38.unknown_3", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_38_string_1,
        { "DirectPlay message type 0x0038 string 1", "dplay.type_38.string_1", FT_STRING, BASE_NONE,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_38_string_2,
        { "DirectPlay message type 0x0038 string 2", "dplay.type_38.string_2", FT_STRING, BASE_NONE,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_38_saddr_af_1,
        { "DirectPlay message type 0x0038 s_addr_in address family 1", "dplay.type_38.saddr.af_1", FT_UINT16,
            BASE_DEC, VALS(dplay_af_val), 0x0, "", HFILL}},
    { &hf_dplay_type_38_saddr_port_1,
        { "DirectPlay message type 0x0038 s_addr_in port 1", "dplay.type_38.saddr.port_1", FT_UINT16, BASE_DEC,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_38_saddr_ip_1,
        { "DirectPlay message type 0x0038 s_addr_in ip 1", "dplay.type_38.saddr.ip_1", FT_IPv4, BASE_NONE,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_38_saddr_padd_1,
        { "DirectPlay message type 0x0038 s_addr_in padding 1", "dplay.type_38.saddr.padd_1", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_38_saddr_af_2,
        { "DirectPlay message type 0x0038 s_addr_in address family 2", "dplay.type_38.saddr.af_2", FT_UINT16,
            BASE_DEC, VALS(dplay_af_val), 0x0, "", HFILL}},
    { &hf_dplay_type_38_saddr_port_2,
        { "DirectPlay message type 0x0038 s_addr_in port 2", "dplay.type_38.saddr.port_2", FT_UINT16, BASE_DEC,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_38_saddr_ip_2,
        { "DirectPlay message type 0x0038 s_addr_in ip 2", "dplay.type_38.saddr.ip_2", FT_IPv4, BASE_NONE,
        NULL, 0x0, "", HFILL}},
    { &hf_dplay_type_38_saddr_padd_2,
        { "DirectPlay message type 0x0038 s_addr_in padding 2", "dplay.type_38.saddr.padd_2", FT_BYTES, BASE_HEX,
        NULL, 0x0, "", HFILL}},
    };

    static gint *ett[] = {
        &ett_dplay,
        &ett_dplay_header,
        &ett_dplay_sockaddr,
        &ett_dplay_data,
        &ett_dplay_flags,
        &ett_dplay_enc_packet,
        &ett_dplay_sess_desc_flags,
        &ett_dplay_type08_saddr1,
        &ett_dplay_type08_saddr2,
        &ett_dplay_type13_saddr1,
        &ett_dplay_type13_saddr2,
        &ett_dplay_type29_saddr1,
        &ett_dplay_type29_saddr2,
        &ett_dplay_type29_saddr3,
        &ett_dplay_type29_saddr4,
        &ett_dplay_type2e_saddr1,
        &ett_dplay_type2e_saddr2,
        &ett_dplay_type38_saddr1,
        &ett_dplay_type38_saddr2,
    };
    module_t *dplay_module;

    if(proto_dplay == -1)
    {
        proto_dplay = proto_register_protocol (
                "DirectPlay Protocol",
                "DPLAY",
                "dplay"
                );
        proto_register_field_array(proto_dplay, hf, array_length(hf));
        proto_register_subtree_array(ett, array_length(ett));
        dplay_module = prefs_register_protocol(proto_dplay, proto_reg_handoff_dplay);
    }
}

void proto_reg_handoff_dplay(void)
{
    static int initialized = FALSE;

    if(!initialized)
    {
        initialized = TRUE;
        heur_dissector_add("udp", heur_dissect_dplay, proto_dplay);
        heur_dissector_add("tcp", heur_dissect_dplay, proto_dplay);
        dplay_handle = create_dissector_handle(dissect_dplay, proto_dplay);
    }
}

