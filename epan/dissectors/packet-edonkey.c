/* packet-edonkey.c
 * Routines for edonkey dissection
 * Copyright 2003, Xuan Zhang <xz@aemail4u.com>
 * eDonkey dissector based on protocol descriptions from mldonkey:
 *  http://savannah.nongnu.org/download/mldonkey/docs/Edonkey-Overnet/edonkey-protocol.txt 
 *  http://savannah.nongnu.org/download/mldonkey/docs/Edonkey-Overnet/overnet-protocol.txt
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
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include <epan/packet.h>
#include "packet-edonkey.h"

static int proto_edonkey = -1;

static int hf_edonkey_message  = -1;
static int hf_edonkey_protocol = -1;
static int hf_edonkey_message_length = -1;
static int hf_edonkey_message_type = -1;
static int hf_edonkey_client_hash = -1;
static int hf_edonkey_server_hash = -1;
static int hf_edonkey_file_hash = -1;
static int hf_edonkey_client_id = -1;
static int hf_edonkey_metatag_namesize = -1;
static int hf_edonkey_metatag_type = -1;
static int hf_edonkey_metatag = -1;
static int hf_edonkey_metatag_name = -1;
static int hf_edonkey_metatag_id = -1;
static int hf_edonkey_search = -1;
static int hf_edonkey_ip = -1;
static int hf_edonkey_port = -1;
static int hf_edonkey_hash = -1;
static int hf_edonkey_directory = -1;
static int hf_edonkey_string = -1;
static int hf_edonkey_string_length = -1;
static int hf_edonkey_fileinfo = -1;
static int hf_edonkey_clientinfo = -1;
static int hf_edonkey_serverinfo = -1;
static int hf_overnet_peer = -1;

static gint ett_edonkey = -1;
static gint ett_edonkey_message = -1;
static gint ett_edonkey_metatag = -1;
static gint ett_edonkey_search = -1;
static gint ett_edonkey_fileinfo = -1;
static gint ett_edonkey_serverinfo = -1;
static gint ett_edonkey_clientinfo = -1;
static gint ett_overnet_peer = -1;

static const value_string edonkey_protocols[] = {
	{ EDONKEY_PROTO_EDONKEY,             "eDonkey"                  },
	{ EDONKEY_PROTO_EMULE_EXT,           "eMule Extensions"         },
	{ EDONKEY_PROTO_EMULE_COMP,          "eMule Compressed"         },
    { 0,                                 NULL                       }
};

static const value_string edonkey_tcp_msgs[] = {
	{ EDONKEY_MSG_HELLO,	             "Hello"                    },
	{ EDONKEY_MSG_BAD_PROTO,             "Bad Proto"                },
	{ EDONKEY_MSG_GET_SERVER_LIST,       "Get Server List"          },
	{ EDONKEY_MSG_OFFER_FILES,           "Offer Files"              },
	{ EDONKEY_MSG_SEARCH_FILES,          "Search Files"             },
	{ EDONKEY_MSG_DISCONNECT,            "Disconnect"               },
	{ EDONKEY_MSG_GET_SOURCES,           "Get Sources"              },
	{ EDONKEY_MSG_SEARCH_USER,           "Search User"              },
	{ EDONKEY_MSG_CLIENT_CB_REQ,         "Client Callback Request"  },
	{ EDONKEY_MSG_MORE_RESULTS,          "More Results"             },
	{ EDONKEY_MSG_SERVER_LIST,           "Server List"              },
	{ EDONKEY_MSG_SEARCH_FILE_RESULTS,   "Search File Results"      },
	{ EDONKEY_MSG_SERVER_STATUS,         "Server Status"            },
	{ EDONKEY_MSG_SERVER_CB_REQ,         "Server Callback Request"  },
	{ EDONKEY_MSG_CALLBACK_FAIL,         "Callback Fail"            },
	{ EDONKEY_MSG_SERVER_MESSAGE,        "Server Message"           },
	{ EDONKEY_MSG_ID_CHANGE,             "ID Change"                },
	{ EDONKEY_MSG_SERVER_INFO_DATA,      "Server Info Data"         },
	{ EDONKEY_MSG_FOUND_SOURCES,         "Found Sources"            },
	{ EDONKEY_MSG_SEARCH_USER_RESULTS,   "Search User Results"      },
	{ EDONKEY_MSG_SENDING_PART,          "Sending Part"             },
	{ EDONKEY_MSG_REQUEST_PARTS,         "Request Parts"            },
	{ EDONKEY_MSG_NO_SUCH_FILE,          "No Such File"             },
	{ EDONKEY_MSG_END_OF_DOWNLOAD,       "End of Download"          },
	{ EDONKEY_MSG_VIEW_FILES,            "View Files"               },
	{ EDONKEY_MSG_VIEW_FILES_ANSWER,     "View Files Answer"        },
	{ EDONKEY_MSG_HELLO_ANSWER,          "Hello Answer"             },
	{ EDONKEY_MSG_NEW_CLIENT_ID,         "New Client ID"            },
	{ EDONKEY_MSG_CLIENT_MESSAGE,        "Client Message"           },
	{ EDONKEY_MSG_FILE_STATUS_REQUEST,   "File Status Request"      },
	{ EDONKEY_MSG_FILE_STATUS,           "File Status"              },
	{ EDONKEY_MSG_HASHSET_REQUEST,       "Hashset Request"          },
	{ EDONKEY_MSG_HASHSET_ANSWER,        "Hashset Answer"           },
	{ EDONKEY_MSG_SLOT_REQUEST,          "Slot Request"             },
	{ EDONKEY_MSG_SLOT_GIVEN,            "Slot Given"               },
	{ EDONKEY_MSG_SLOT_RELEASE,          "Slot Release"             },
	{ EDONKEY_MSG_SLOT_TAKEN,            "Slot Taken"               },
	{ EDONKEY_MSG_FILE_REQUEST,          "File Request"             },
	{ EDONKEY_MSG_FILE_REQUEST_ANSWER,   "File Request Answer"      },
	{ EDONKEY_MSG_GET_SHARED_DIRS,       "Get Shared Directories"   },
	{ EDONKEY_MSG_GET_SHARED_FILES,      "Get Shared Files"         },
	{ EDONKEY_MSG_SHARED_DIRS,           "Shared Directores"        },
	{ EDONKEY_MSG_SHARED_FILES,          "Shared Files"             },
	{ EDONKEY_MSG_SHARED_DENIED,         "Shared Denied"            },
    { 0,                                 NULL                       }
};

static const value_string emule_tcp_msgs[] = {
	{ EMULE_MSG_HELLO,	                 "Hello"                    },
	{ EMULE_MSG_HELLO_ANSWER,            "Hello Answer"             },
	{ EMULE_MSG_DATA_COMPRESSED,         "Data Compressed"          },
	{ EMULE_MSG_QUEUE_RANKING,           "Queue Ranking"            },
	{ EMULE_MSG_SOURCES_REQUEST,         "Sources Request"          },
	{ EMULE_MSG_SOURCES_ANSWER,          "Sources Answer"           },
    { 0,                                 NULL                       }
};

static const value_string edonkey_udp_msgs[] = {
	{ EDONKEY_MSG_UDP_SERVER_STATUS_REQUEST,    "Server Status Request"    },
	{ EDONKEY_MSG_UDP_SERVER_STATUS,            "Server Status"            },
	{ EDONKEY_MSG_UDP_SEARCH_FILE,              "Search File"              },
	{ EDONKEY_MSG_UDP_SEARCH_FILE_RESULTS,      "Search File Results"      },
	{ EDONKEY_MSG_UDP_GET_SOURCES,              "Get Sources"              },
	{ EDONKEY_MSG_UDP_FOUND_SOURCES,            "Found Sources"            },
	{ EDONKEY_MSG_UDP_CALLBACK_REQUEST,         "Callback Request"         },
	{ EDONKEY_MSG_UDP_CALLBACK_FAIL,            "Callback Fail"            },
	{ EDONKEY_MSG_UDP_SERVER_LIST,              "Server List"              },
	{ EDONKEY_MSG_UDP_GET_SERVER_INFO,          "Get Server Info"          },
	{ EDONKEY_MSG_UDP_SERVER_INFO,              "Server Info"              },
	{ EDONKEY_MSG_UDP_GET_SERVER_LIST,          "Get Server List"          },

    /* eMule Extensions */
	{ EMULE_MSG_UDP_REASKFILEPING,              "Reask File Ping"          },
	{ EMULE_MSG_UDP_REASKACK,                   "Reask ACK"                },
	{ EMULE_MSG_UDP_FILE_NOT_FOUND,             "File not found"           },
	{ EMULE_MSG_UDP_QUEUE_FULL,                 "Queue Full"               },

    /* Overnet Extensions */
	{ OVERNET_MSG_UDP_CONNECT,                  "Connect"                  },
	{ OVERNET_MSG_UDP_CONNECT_REPLY,            "Connect Reply"            },
	{ OVERNET_MSG_UDP_PUBLICIZE,                "Publicize"                },
	{ OVERNET_MSG_UDP_PUBLICIZE_ACK,            "Publicize ACK"            },
	{ OVERNET_MSG_UDP_SEARCH,                   "Search"                   },
	{ OVERNET_MSG_UDP_SEARCH_NEXT,              "Search Next"              },
	{ OVERNET_MSG_UDP_SEARCH_INFO,              "Search Info"              },
	{ OVERNET_MSG_UDP_SEARCH_RESULT,            "Search Result"            },
	{ OVERNET_MSG_UDP_SEARCH_END,               "Search End"               },
	{ OVERNET_MSG_UDP_PUBLISH,                  "Publish"                  },
	{ OVERNET_MSG_UDP_PUBLISH_ACK,              "Publish ACK"              },
	{ OVERNET_MSG_UDP_IDENTIFY_REPLY,           "Identify Reply"           },
	{ OVERNET_MSG_UDP_IDENTIFY_ACK,             "Identify ACK"             },
	{ OVERNET_MSG_UDP_FIREWALL_CONNECTION,      "Firewall Connection"      },
	{ OVERNET_MSG_UDP_FIREWALL_CONNECTION_ACK,  "Firewall Connection ACK"  },
	{ OVERNET_MSG_UDP_FIREWALL_CONNECTION_NACK, "Firewall Connection NACK" },
	{ OVERNET_MSG_UDP_IP_QUERY,                 "IP Query"                 },
	{ OVERNET_MSG_UDP_IP_QUERY_ANSWER,          "IP Query Answer"          },
	{ OVERNET_MSG_UDP_IP_QUERY_END,             "IP Query End"             },
	{ OVERNET_MSG_UDP_IDENTIFY,                 "Identify"                 },
    { 0,                                        NULL                       }
};

static const value_string edonkey_special_tags[] = {
    { EDONKEY_STAG_NAME,                "Name"                      },
    { EDONKEY_STAG_SIZE,                "Size"                      },
    { EDONKEY_STAG_TYPE,                "Type"                      },
    { EDONKEY_STAG_FORMAT,              "Format"                    },
    { EDONKEY_STAG_COLLECTION,          "Collection"                },
    { EDONKEY_STAG_PART_PATH,           "Part Path"                 },
    { EDONKEY_STAG_PART_HASH,           "Part Hash"                 },
    { EDONKEY_STAG_COPIED,              "Copied"                    },
    { EDONKEY_STAG_GAP_START,           "Gap Start"                 },
    { EDONKEY_STAG_GAP_END,             "Gap End"                   },
    { EDONKEY_STAG_DESCRIPTION,         "Description"               },
    { EDONKEY_STAG_PING,                "Ping"                      },
    { EDONKEY_STAG_FAIL,                "Fail"                      },
    { EDONKEY_STAG_PREFERENCE,          "Preference"                },
    { EDONKEY_STAG_PORT,                "Port"                      },
    { EDONKEY_STAG_IP,                  "IP"                        },
    { EDONKEY_STAG_VERSION,             "Version"                   },
    { EDONKEY_STAG_TEMPFILE,            "Temporary File"            },
    { EDONKEY_STAG_PRIORITY,            "Priority"                  },
    { EDONKEY_STAG_STATUS,              "Status"                    },
    { EDONKEY_STAG_AVAILABILITY,        "Availability"              },
    { EDONKEY_STAG_QTIME,               "Queue Time"                },
    { EDONKEY_STAG_PARTS,               "Parts"                     },
    { EMULE_STAG_COMPRESSION,           "Compression"               },
    { EMULE_STAG_UDP_CLIENT_PORT,       "UDP Client Port"           },
    { EMULE_STAG_UDP_VERSION,           "UDP Version"               },
    { EMULE_STAG_SOURCE_EXCHANGE,       "Source Exchange"           },
    { EMULE_STAG_COMMENTS,              "Comments"                  },
    { EMULE_STAG_EXTENDED_REQUEST,      "Extended Request"          },
    { EMULE_STAG_COMPATIBLE_CLIENT,     "Compatible Client"         },
    { 0,                                NULL                        }
};

static const value_string edonkey_search_ops[] = {
    { EDONKEY_SEARCH_AND,               "AND"                       },
    { EDONKEY_SEARCH_OR,                "OR"                        },
    { EDONKEY_SEARCH_ANDNOT,            "AND NOT"                   },
    { 0,                                NULL                        }
};

static const value_string edonkey_search_conds[] = {
    { EDONKEY_SEARCH_MIN,               "MIN"                       },
    { EDONKEY_SEARCH_MAX,               "MAX"                       },
    { 0,                                NULL                        }
};

/* Dissects a generic eDonkey list */
static int dissect_edonkey_list(tvbuff_t *tvb, packet_info *pinfo _U_, 
                                int offset,  proto_tree *tree,
                                int listnum_length, char* listdesc,
                                int  (*item_dissector)(tvbuff_t  *, packet_info *, int, proto_tree *))
{
    /* <List> ::= <List Size> <Item>* */
    guint32 listnum, i;
    switch (listnum_length) {
        case 1: 
            listnum = tvb_get_guint8(tvb, offset);
            break;

        case 2: 
            listnum = tvb_get_letohs(tvb, offset);
            break;

        case 4: 
            listnum = tvb_get_letohl(tvb, offset);
            break;

        default:
            /* Not Supported */
            return offset;
    }

    proto_tree_add_text(tree, tvb, offset, listnum_length, "%s List Size: %u", listdesc, listnum);
    offset+=listnum_length;
    for (i=0; i<listnum; i++) 
    {
        offset = (*item_dissector)(tvb, pinfo, offset, tree);
    }
    return offset;
}

gint lookup_str_index(gchar* str, gint length, const value_string *vs) 
{
  gint i = 0;

  if (str == NULL) return -1;

  while (vs[i].strptr) {
      if (strncasecmp(str, vs[i].strptr, length) == 0)
          return i;
      i++;
  }

  return -1;
}

static guint8 edonkey_metatag_name_get_type(tvbuff_t *tvb, gint start, gint length, guint8 special_tagtype) 
{
	guchar	*tag_name;
    tag_name = match_strval(special_tagtype, edonkey_special_tags);
    if (tag_name == NULL) {
        gint index;
		tag_name = (guchar*) tvb_get_ptr(tvb, start, length);
        index = lookup_str_index(tag_name, length, edonkey_special_tags);
        if (index < 0) 
            return EDONKEY_STAG_UNKNOWN;
        else return edonkey_special_tags[index].value;
    }
    else return special_tagtype;

    return EDONKEY_STAG_UNKNOWN;
}

static proto_item* edonkey_tree_add_metatag_name(proto_tree *tree, tvbuff_t *tvb,
                                            gint start, gint length, guint8 special_tagtype)
{
	gchar	*tag_name;
    tag_name = match_strval(special_tagtype, edonkey_special_tags);
    if (tag_name == NULL) {
        return proto_tree_add_item(tree, hf_edonkey_metatag_name, tvb, start, length, FALSE);
    }
    else {
        return proto_tree_add_uint_format(tree, hf_edonkey_metatag_id, tvb, start, length, 
                                          special_tagtype, "Meta Tag Name: %s (0x%02x)", 
                                          tag_name, special_tagtype);
    }
}

/* Dissects the eDonkey meta tag */
static int dissect_edonkey_metatag(tvbuff_t *tvb, packet_info *pinfo _U_, 
                                   int offset, proto_tree *tree)
{
    /* <Meta Tag> ::= <Tag Type (guint8)> <Tag Name> <Tag> */
    /* <Tag Name> ::= <Tag Name Size (guint16)> <Special Tag> || <String> */
    proto_item *ti;
    proto_tree *metatag_tree;
    guint8 tag_type, special_tagtype;
    guint16 tag_name_size, string_length;
    guint32 tag_length, tag_value_guint32;
    int tag_offset;

    tag_type = tvb_get_guint8(tvb, offset);
    tag_name_size = tvb_get_letohs(tvb, offset+1);
    special_tagtype = tvb_get_guint8(tvb, offset+3);
    
    tag_length = 3 + tag_name_size;
    tag_offset = offset + tag_length;
    
    switch (tag_type)
    {        
        case EDONKEY_MTAG_HASH:
            /* <Tag> ::= HASH */
            tag_length += 16;
            ti = proto_tree_add_item(tree, hf_edonkey_metatag, tvb, offset, tag_length, FALSE);
            metatag_tree = proto_item_add_subtree(ti, ett_edonkey_metatag);
            proto_tree_add_uint(metatag_tree, hf_edonkey_metatag_type, tvb, offset, 1, tag_type);
            proto_tree_add_uint(metatag_tree, hf_edonkey_metatag_namesize, tvb, offset+1, 2, tag_name_size);
            edonkey_tree_add_metatag_name(metatag_tree, tvb, offset+3, tag_name_size, special_tagtype);
            proto_tree_add_item(metatag_tree, hf_edonkey_hash, tvb, tag_offset, 16, FALSE);
            break;
            
        case EDONKEY_MTAG_STRING:
            /* <Tag> ::= <String> */
            string_length = tvb_get_letohs(tvb, tag_offset);
            tag_length += 2+string_length;
            ti = proto_tree_add_item(tree, hf_edonkey_metatag, tvb, offset, tag_length, FALSE);
            metatag_tree = proto_item_add_subtree(ti, ett_edonkey_metatag);
            proto_tree_add_uint(metatag_tree, hf_edonkey_metatag_type, tvb, offset, 1, tag_type);
            proto_tree_add_uint(metatag_tree, hf_edonkey_metatag_namesize, tvb, offset+1, 2, tag_name_size);
            edonkey_tree_add_metatag_name(metatag_tree, tvb, offset+3, tag_name_size, special_tagtype);
            proto_tree_add_uint(metatag_tree, hf_edonkey_string_length, tvb, tag_offset, 2, string_length);
            proto_tree_add_item(metatag_tree, hf_edonkey_string, tvb, tag_offset+2, string_length, FALSE);
            break;

        case EDONKEY_MTAG_DWORD:
            /* <Tag> ::= guint32 */
            tag_length += 4;
            ti = proto_tree_add_item(tree, hf_edonkey_metatag, tvb, offset, tag_length, FALSE);
            metatag_tree = proto_item_add_subtree(ti, ett_edonkey_metatag);
            proto_tree_add_uint(metatag_tree, hf_edonkey_metatag_type, tvb, offset, 1, tag_type);
            proto_tree_add_uint(metatag_tree, hf_edonkey_metatag_namesize, tvb, offset+1, 2, tag_name_size);
            edonkey_tree_add_metatag_name(metatag_tree, tvb, offset+3, tag_name_size, special_tagtype);
            if (edonkey_metatag_name_get_type(tvb, offset+3, tag_name_size, special_tagtype) == EDONKEY_STAG_IP) {
                proto_tree_add_item(metatag_tree, hf_edonkey_ip, tvb, tag_offset, 4, FALSE);
            }
            else {
                tag_value_guint32 = tvb_get_letohl(tvb, tag_offset);
                proto_tree_add_text(metatag_tree, tvb, tag_offset, 4, "Meta Tag Value: %u", tag_value_guint32);
            }
            break;

        case EDONKEY_MTAG_FLOAT:
            /* <Tag> ::=  4 byte float */
            tag_length += 4;
            ti = proto_tree_add_item(tree, hf_edonkey_metatag, tvb, offset, tag_length, FALSE);
            metatag_tree = proto_item_add_subtree(ti, ett_edonkey_metatag);
            proto_tree_add_uint(metatag_tree, hf_edonkey_metatag_type, tvb, offset, 1, tag_type);
            proto_tree_add_uint(metatag_tree, hf_edonkey_metatag_namesize, tvb, offset+1, 2, tag_name_size);
            edonkey_tree_add_metatag_name(metatag_tree, tvb, offset+3, tag_name_size, special_tagtype);
            break;

        case EDONKEY_MTAG_BOOL:       /* <Tag> ::= Boolean ?? bytes*/
        case EDONKEY_MTAG_BOOL_ARRAY: /* <Tag> ::= ?? */
        case EDONKEY_MTAG_BLOB:       /* <Tag> ::= ?? */
        case EDONKEY_MTAG_UNKNOWN:
        default:
            /* Unknown tag type - actual tag length is also unknown */
            ti = proto_tree_add_item(tree, hf_edonkey_metatag, tvb, offset, tag_length, FALSE);
            metatag_tree = proto_item_add_subtree(ti, ett_edonkey_metatag);
            proto_tree_add_text(metatag_tree, tvb, offset, 1, "Unknown Meta Tag Type (0x%02x)", tag_type);
            proto_tree_add_uint(metatag_tree, hf_edonkey_metatag_namesize, tvb, offset+1, 2, tag_name_size);
            edonkey_tree_add_metatag_name(metatag_tree, tvb, offset+3, tag_name_size, special_tagtype);
            break;
        
    }
    
    return offset + tag_length;
}

/* Dissects the eDonkey address */
static int dissect_edonkey_address(tvbuff_t *tvb, packet_info *pinfo _U_, 
                                   int offset, proto_tree *tree)
{
    /* <Address> ::= <IP> <Port> */
/*    guint32 ip = tvb_get_letohl(tvb, offset);
      proto_tree_add_ipv4(tree, hf_edonkey_ip, tvb, offset, 4, ip); */
    proto_tree_add_item(tree, hf_edonkey_ip, tvb, offset, 4, FALSE);
    proto_tree_add_item(tree, hf_edonkey_port, tvb, offset+4, 2, TRUE);
    return offset+6;
}

/* Dissects the eDonkey address list */
static int dissect_edonkey_address_list(tvbuff_t *tvb, packet_info *pinfo _U_, 
                                        int offset,  proto_tree *tree)
{
    /* <Address List> ::= <List Size (guint8)> <Address>* */
    return dissect_edonkey_list(tvb, pinfo, offset, tree, 1, "Address", dissect_edonkey_address);
}

/* Dissects the eMule   address list */
static int dissect_emule_address_list(tvbuff_t *tvb, packet_info *pinfo _U_, 
                                        int offset,  proto_tree *tree)
{
    /* <Address List> ::= <List Size (guint16)> <Address>* */
    return dissect_edonkey_list(tvb, pinfo, offset, tree, 2, "Address", dissect_edonkey_address);
}

/* Dissects the eDonkey hash */
static int dissect_edonkey_hash(tvbuff_t *tvb, packet_info *pinfo _U_, 
                                       int offset, proto_tree *tree)
{
    /* <hash> ::= HASH (16 word MD4 digest) */
    proto_tree_add_item(tree, hf_edonkey_hash, tvb, offset, 16, FALSE);
    return offset+16;
}


/* Dissects the eDonkey hash list */
static int dissect_edonkey_hash_list(tvbuff_t *tvb, packet_info *pinfo _U_, 
                                        int offset,  proto_tree *tree)
{
    /* <Hash List> ::= <List Size (guint16)> <Hash>* */
    return dissect_edonkey_list(tvb, pinfo, offset, tree, 2, "Hash", dissect_edonkey_hash);
}

/* Dissects the eDonkey meta tag list */
static int dissect_edonkey_metatag_list(tvbuff_t *tvb, packet_info *pinfo _U_, 
                                        int offset, proto_tree *tree)
{
    /* <Meta Tag List> ::= <List Size (guint32)> <Meta tag>* */
    return dissect_edonkey_list(tvb, pinfo, offset, tree, 4, "Meta Tag", dissect_edonkey_metatag);
}

/* Dissects the eDonkey String */
static int dissect_edonkey_string(tvbuff_t *tvb, packet_info *pinfo _U_, 
                                  int offset, proto_tree *tree)
{
    /* <String> ::= <String length (guint16)> DATA */
    guint16 string_length = tvb_get_letohs(tvb, offset);
    proto_tree_add_uint(tree, hf_edonkey_string_length, tvb, offset, 2, string_length);
    proto_tree_add_item(tree, hf_edonkey_string, tvb, offset+2, string_length, FALSE);
    return offset+2+string_length;
}

/* Dissects the eDonkey Directory */
static int dissect_edonkey_directory(tvbuff_t *tvb, packet_info *pinfo _U_, 
                                     int offset, proto_tree *tree)
{
    /* <Directory> ::= <String> */
    guint16 string_length = tvb_get_letohs(tvb, offset);
    proto_tree_add_uint(tree, hf_edonkey_string_length, tvb, offset, 2, string_length);
    proto_tree_add_item(tree, hf_edonkey_directory, tvb, offset+2, string_length, FALSE);
    return offset+2+string_length;
}

/* Dissects the eDonkey Filename */
static int dissect_edonkey_file_name(tvbuff_t *tvb, packet_info *pinfo _U_, 
                                     int offset, proto_tree *tree)
{
    /* <Filename> ::= <String> */
    return dissect_edonkey_string(tvb, pinfo, offset, tree);
}

/* Dissects the eDonkey directory list */
static int dissect_edonkey_directory_list(tvbuff_t *tvb, packet_info *pinfo _U_, 
                                          int offset,  proto_tree *tree)
{
    /* <Directory List> ::= <List Size (guint32)> <Directory>* */
    return dissect_edonkey_list(tvb, pinfo, offset, tree, 4, "Directory", dissect_edonkey_directory);
}

/* Dissects the eDonkey file hash */
static int dissect_edonkey_file_hash(tvbuff_t *tvb, packet_info *pinfo _U_, 
                                     int offset, proto_tree *tree)
{
    /* <File hash> ::= HASH (16 word MD4 digest) */
    proto_tree_add_item(tree, hf_edonkey_file_hash, tvb, offset, 16, FALSE);
    return offset+16;
}

/* Dissects the eDonkey server hash */
static int dissect_edonkey_server_hash(tvbuff_t *tvb, packet_info *pinfo _U_, 
                                       int offset, proto_tree *tree)
{
    /* <Server hash> ::= HASH (16 word MD4 digest) */
    proto_tree_add_item(tree, hf_edonkey_server_hash, tvb, offset, 16, FALSE);
    return offset+16;
}

/* Dissects the eDonkey client hash */
static int dissect_edonkey_client_hash(tvbuff_t *tvb, packet_info *pinfo _U_, 
                                       int offset, proto_tree *tree)
{
    /* <Client hash> ::= HASH (16 word MD4 digest) */
    proto_tree_add_item(tree, hf_edonkey_client_hash, tvb, offset, 16, FALSE);
    return offset+16;
}

/* Dissects the eDonkey client ID */
static int dissect_edonkey_client_id(tvbuff_t *tvb, packet_info *pinfo _U_, 
                                     int offset, proto_tree *tree)
{
    /* <Client ID> ::= guint32 */
/*    guint32 ip = tvb_get_letohl(tvb, offset);
      proto_tree_add_ipv4(tree, hf_edonkey_client_id, tvb, offset, 4, ip); */
    proto_tree_add_item(tree, hf_edonkey_client_id, tvb, offset, 4, FALSE);
    return offset+4;
}

/* Dissects the eDonkey port */
static int dissect_edonkey_port(tvbuff_t *tvb, packet_info *pinfo _U_, 
                                int offset, proto_tree *tree)
{
    /* <Port> ::= guint16 */
    proto_tree_add_item(tree, hf_edonkey_port, tvb, offset, 2, TRUE);
    return offset+2;
}

/* Dissects the eDonkey start offset */
static int dissect_edonkey_start_offset(tvbuff_t *tvb, packet_info *pinfo _U_, 
                                        int offset, proto_tree *tree)
{
    /* <Start Offset> ::= guint32 */
    guint32 start = tvb_get_letohl(tvb, offset);
    proto_tree_add_text(tree, tvb, offset, 4, "Start Offset: %u", start);
    return offset+4;
}

/* Dissects the eDonkey end offset */
static int dissect_edonkey_end_offset(tvbuff_t *tvb, packet_info *pinfo _U_, 
                                      int offset, proto_tree *tree)
{
    /* <End Offset> ::= guint32 */
    guint32 end = tvb_get_letohl(tvb, offset);
    proto_tree_add_text(tree, tvb, offset, 4, "End Offset: %u", end);
    return offset+4;
}

/* Dissects the eDonkey client info */
static int dissect_edonkey_client_info(tvbuff_t *tvb, packet_info *pinfo _U_, 
                                       int offset,  proto_tree *tree)
{
    /* <Client info> ::= <Client hash> <Client ID> <Port> <Meta tag list> */
    proto_item *ti;
    proto_tree *clientinfo_tree;    
    /* Add subtree for client info */
    ti = proto_tree_add_item(tree, hf_edonkey_clientinfo, tvb, offset, 0, FALSE);
    clientinfo_tree = proto_item_add_subtree(ti, ett_edonkey_clientinfo);        
    offset = dissect_edonkey_client_hash(tvb, pinfo, offset, clientinfo_tree);
    offset = dissect_edonkey_client_id(tvb, pinfo, offset, clientinfo_tree);
    offset = dissect_edonkey_port(tvb, pinfo, offset, clientinfo_tree); 
    offset = dissect_edonkey_metatag_list(tvb, pinfo, offset, clientinfo_tree); 
    return offset;
}

/* Dissects the eDonkey client info list */
static int dissect_edonkey_client_info_list(tvbuff_t *tvb, packet_info *pinfo _U_, 
                                       int offset,  proto_tree *tree)
{
    /* <Client Info List> ::= <List Size (guint32)> <Client Info>* */
    return dissect_edonkey_list(tvb, pinfo, offset, tree, 4, "Client Info", dissect_edonkey_client_info);
}

/* Dissects the eDonkey server info */
static int dissect_edonkey_server_info(tvbuff_t *tvb, packet_info *pinfo _U_, 
                                       int offset,  proto_tree *tree)
{
    /* <Server info> ::= <Server hash> <Server Address> <Meta tag list> */
    proto_item *ti;
    proto_tree *serverinfo_tree;    
    /* Add subtree for server info */
    ti = proto_tree_add_item(tree, hf_edonkey_serverinfo, tvb, offset, 0, FALSE);
    serverinfo_tree = proto_item_add_subtree(ti, ett_edonkey_serverinfo);        
    offset = dissect_edonkey_server_hash(tvb, pinfo, offset, serverinfo_tree);
    offset = dissect_edonkey_address(tvb, pinfo, offset, serverinfo_tree);
    offset = dissect_edonkey_metatag_list(tvb, pinfo, offset, serverinfo_tree); 
    return offset;
}

/* Dissects the eDonkey file info */
static int dissect_edonkey_file_info(tvbuff_t *tvb, packet_info *pinfo _U_, 
                                     int offset,  proto_tree *tree)
{
    /* <File info> ::= <File hash> <Client ID> <Port> <Meta tag list> */
    proto_item *ti;
    proto_tree *fileinfo_tree;    
    /* Add subtree for file info */
    ti = proto_tree_add_item(tree, hf_edonkey_fileinfo, tvb, offset, 0, FALSE);
    fileinfo_tree = proto_item_add_subtree(ti, ett_edonkey_fileinfo);        
    offset = dissect_edonkey_file_hash(tvb, pinfo, offset, fileinfo_tree);
    offset = dissect_edonkey_client_id(tvb, pinfo, offset, fileinfo_tree);
    offset = dissect_edonkey_port(tvb, pinfo, offset, fileinfo_tree); 
    offset = dissect_edonkey_metatag_list(tvb, pinfo, offset, fileinfo_tree); 
    return offset;
}

/* Dissects the eDonkey file info list */
static int dissect_edonkey_file_info_list(tvbuff_t *tvb, packet_info *pinfo _U_, 
                                          int offset,  proto_tree *tree)
{
    /* <File Info List> ::= <List Size (guint32)> <File Info>* */
    return dissect_edonkey_list(tvb, pinfo, offset, tree, 4, "File Info", dissect_edonkey_file_info);
}

/* Dissects the Overnet peer type */
static int dissect_overnet_peertype(tvbuff_t *tvb, packet_info *pinfo _U_, 
                                    int offset, proto_tree *tree)
{
    /* <Peer type> ::= guint8 */
    guint8 peertype = tvb_get_guint8(tvb, offset);
    proto_tree_add_text(tree, tvb, offset, 1, "Peer Type: %u", peertype);
    return offset+1;
}

/* Dissects the Overnet peer */
static int dissect_overnet_peer(tvbuff_t *tvb, packet_info *pinfo _U_, 
                                int offset, proto_tree *tree)
{
    /* <Peer> ::= <Hash> <Address> <Peer type> */
    proto_item *ti;
    proto_tree *peer_tree;    
    ti = proto_tree_add_item(tree, hf_overnet_peer, tvb, offset, 16 + 6 + 1, FALSE);
    peer_tree = proto_item_add_subtree(ti, ett_overnet_peer);
    offset = dissect_edonkey_hash(tvb, pinfo, offset, peer_tree);
    offset = dissect_edonkey_address(tvb, pinfo, offset, peer_tree);
    offset = dissect_overnet_peertype(tvb, pinfo, offset, peer_tree);        
    return offset;
}

/* Dissects the eDonkey search query */
static int dissect_edonkey_search_query(tvbuff_t *tvb, packet_info *pinfo _U_, 
                                        int offset, proto_tree *tree)
{
    /* <Search Query> ::= <Search Type> <Search> */
    proto_item *ti;
    proto_tree *search_tree;
    guint8 search_type, operator, special_tagtype, limit_type;
    guint16 tag_name_size, string_length;
    guint32 search_length, limit;
    int string_offset, tag_name_offset;

    search_type = tvb_get_guint8(tvb, offset);
    search_length = 1;

    switch (search_type)
    {        
        case EDONKEY_SEARCH_BOOL: 
            /* <Search> ::=  <Operator> <Search Query> <Search Query> */
            search_length += 1;
            operator = tvb_get_guint8(tvb, offset+1);
            
            /* Add subtree for search entry */
            ti = proto_tree_add_item(tree, hf_edonkey_search, tvb, offset, search_length, FALSE);
            search_tree = proto_item_add_subtree(ti, ett_edonkey_search);

            /* Add query info */
            proto_tree_add_text(search_tree, tvb, offset, 2, "Boolean search (0x%02x): %s (0x%02x)",
                                search_type, val_to_str(operator, edonkey_search_ops, "Unknown"), operator);

            offset+=2;
            offset = dissect_edonkey_search_query(tvb, pinfo, offset, search_tree);
            offset = dissect_edonkey_search_query(tvb, pinfo, offset, search_tree);
            break;

        case EDONKEY_SEARCH_NAME: 
            /* <Search> ::=  <String> */
            string_offset = offset + search_length;
            string_length = tvb_get_letohs(tvb, string_offset);
            search_length += 2+string_length;

            /* Add subtree for search entry */
            ti = proto_tree_add_item(tree, hf_edonkey_search, tvb, offset, search_length, FALSE);
            search_tree = proto_item_add_subtree(ti, ett_edonkey_search);

            /* Add query info */
            proto_tree_add_text(search_tree, tvb, offset, 1, "Search by name (0x%02x)", search_type);
            proto_tree_add_uint(search_tree, hf_edonkey_string_length, tvb, string_offset, 2, string_length);
            proto_tree_add_item(search_tree, hf_edonkey_string, tvb, string_offset+2, string_length, FALSE);
            offset += search_length;
            break;

        case EDONKEY_SEARCH_META: 
            /* <Search> ::=  <String> <Meta tag Name> */
            string_offset = offset + search_length;
            string_length = tvb_get_letohs(tvb, offset+1);
            search_length += 2+string_length;

            tag_name_offset = offset + search_length;
            tag_name_size = tvb_get_letohs(tvb, tag_name_offset);
            special_tagtype = tvb_get_guint8(tvb, tag_name_offset+2);
            search_length += 2 + tag_name_size;

            /* Add subtree for search entry */
            ti = proto_tree_add_item(tree, hf_edonkey_search, tvb, offset, search_length, FALSE);
            search_tree = proto_item_add_subtree(ti, ett_edonkey_search);

            /* Add query info */
            proto_tree_add_text(search_tree, tvb, offset, 1, "Search by metadata (0x%02x)", search_type);
            proto_tree_add_uint(search_tree, hf_edonkey_string_length, tvb, string_offset, 2, string_length);
            proto_tree_add_item(search_tree, hf_edonkey_string, tvb, string_offset+2, string_length, FALSE);
            proto_tree_add_uint(search_tree, hf_edonkey_metatag_namesize, tvb, tag_name_offset, 2, tag_name_size);
            edonkey_tree_add_metatag_name(search_tree, tvb, tag_name_offset+2, tag_name_size, special_tagtype);
            offset += search_length;
            break;

        case EDONKEY_SEARCH_LIMIT:
            /* <Search> ::=  <Limit (guint32)> <Minmax> <Meta tag Name> */
            search_length += 5; /* 4 bytes for the limit, one for the minmax */
            limit = tvb_get_letohl(tvb, offset+1);
            limit_type = tvb_get_guint8(tvb, offset+5);

            tag_name_offset = offset + search_length;
            tag_name_size = tvb_get_letohs(tvb, tag_name_offset);
            special_tagtype = tvb_get_guint8(tvb, tag_name_offset+2);
            search_length += 2 + tag_name_size;

            /* Add subtree for search entry */
            ti = proto_tree_add_item(tree, hf_edonkey_search, tvb, offset, search_length, FALSE);
            search_tree = proto_item_add_subtree(ti, ett_edonkey_search);

            /* Add query info */
            proto_tree_add_text(search_tree, tvb, offset, 6, "Search by limit (0x%02x): %s %u", 
                                search_type, val_to_str(limit_type, edonkey_search_conds, "Unknown"), limit);
            proto_tree_add_uint(search_tree, hf_edonkey_metatag_namesize, tvb, tag_name_offset, 2, tag_name_size);
            edonkey_tree_add_metatag_name(search_tree, tvb, tag_name_offset+2, tag_name_size, special_tagtype);
            offset += search_length;
            break;

        default:
            /* Unknown search type - actual search length is also unknown */
            ti = proto_tree_add_item(tree, hf_edonkey_search, tvb, offset, search_length, FALSE);
            search_tree = proto_item_add_subtree(ti, ett_edonkey_search);
            proto_tree_add_text(search_tree, tvb, offset, search_length, "Unknown Search (0x%02x)", search_type);
            offset += search_length; 
            break;
    }

    return offset;    
}

static void dissect_edonkey_tcp_message(guint8 msg_type,
                                        tvbuff_t *tvb, packet_info *pinfo _U_, 
                                        int offset, int length, proto_tree *tree) 
{   
    int msg_start, msg_end, bytes_remaining;
    guint8  hello, more;
    guint32 nusers, nfiles;

    if (tree == NULL) return;
    
    bytes_remaining = tvb_reported_length_remaining(tvb, offset);
    if ((length < 0) || (length > bytes_remaining)) length = bytes_remaining;
    if (length <= 0) return;

    msg_start = offset;
    msg_end = offset + length;

    switch (msg_type) {
        case EDONKEY_MSG_HELLO:
            /* Client to Server: <Client Info> */
            /* Client to Client: 0x10 <Client Info> */
            hello = tvb_get_guint8(tvb, offset);
            if (hello == 0x10) /* Hello Client */
                offset += 1;
            offset = dissect_edonkey_client_info(tvb, pinfo, offset, tree);
            break;

        case EDONKEY_MSG_HELLO_ANSWER: /* Hello Answer: <Client Info> <Server address> */
            offset = dissect_edonkey_client_info(tvb, pinfo, offset, tree);
            offset = dissect_edonkey_address(tvb, pinfo, offset, tree);
            break;

        case EDONKEY_MSG_SERVER_CB_REQ: /* Server Callback Request: <Client address> */
            offset = dissect_edonkey_address(tvb, pinfo, offset, tree);
            break;

        case EDONKEY_MSG_SERVER_INFO_DATA: /* Server Info Data: <Server Info> */
            offset = dissect_edonkey_server_info(tvb, pinfo, offset, tree);
            break;

        case EDONKEY_MSG_SERVER_LIST: /* Server List: <Address List> */
            offset = dissect_edonkey_address_list(tvb, pinfo, offset, tree);
            break;

        case EDONKEY_MSG_OFFER_FILES: /* Offer Files: <File info List> */
        case EDONKEY_MSG_VIEW_FILES_ANSWER: /* View Files Answer: <File info list> */
            offset = dissect_edonkey_file_info_list(tvb, pinfo, offset, tree);
            break;

        case EDONKEY_MSG_SEARCH_FILE_RESULTS: /* Search File Results: <File Info list> <More> */
            offset = dissect_edonkey_file_info_list(tvb, pinfo, offset, tree);
            more = tvb_get_guint8(tvb, offset);
            if (more)
                proto_tree_add_text(tree, tvb, offset, 1, "More: TRUE (0x%02x)", more);
            else proto_tree_add_text(tree, tvb, offset, 1, "More: FALSE (0x%02x)", more);            
            break;

        case EDONKEY_MSG_SEARCH_FILES: /* Search File: <Search query> */
        case EDONKEY_MSG_SEARCH_USER:  /* Search User: <Search query> */
            offset = dissect_edonkey_search_query(tvb, pinfo, offset, tree);
            break;
       
        case EDONKEY_MSG_GET_SOURCES:         /* Get Sources: <File Hash> */
        case EDONKEY_MSG_NO_SUCH_FILE:        /* No Such File: <File Hash> */
        case EDONKEY_MSG_END_OF_DOWNLOAD:     /* End of Download: <File Hash> */
        case EDONKEY_MSG_FILE_STATUS_REQUEST: /* File Status Request: <File Hash> */
        case EDONKEY_MSG_HASHSET_REQUEST:     /* Hashset Request: <File Hash> */
        case EDONKEY_MSG_SLOT_REQUEST:        /* Slot Request: <File Hash> */
        case EDONKEY_MSG_FILE_REQUEST:        /* File Request: <File Hash> */
            offset = dissect_edonkey_file_hash(tvb, pinfo, offset, tree);
            break;

        case EDONKEY_MSG_FOUND_SOURCES: /* Found Sources: <File Hash> <Address List> */
            offset = dissect_edonkey_file_hash(tvb, pinfo, offset, tree);
            offset = dissect_edonkey_address_list(tvb, pinfo, offset, tree);
            break;

        case EDONKEY_MSG_CLIENT_CB_REQ:  /* Client Callback Request: <Client ID> */
        case EDONKEY_MSG_CALLBACK_FAIL:  /* Callback Fail:           <Client ID> */
        case EDONKEY_MSG_ID_CHANGE:      /* ID Change:               <Client ID> */
            offset = dissect_edonkey_client_id(tvb, pinfo, offset, tree);
            break;

        case EDONKEY_MSG_NEW_CLIENT_ID:  /* New Client ID: <Client ID> <Client ID> */
            offset = dissect_edonkey_client_id(tvb, pinfo, offset, tree);
            offset = dissect_edonkey_client_id(tvb, pinfo, offset, tree);
            break;

        case EDONKEY_MSG_SERVER_MESSAGE: /* Server Message: <String> */
        case EDONKEY_MSG_CLIENT_MESSAGE: /* Client Message: <String> */
            offset = dissect_edonkey_string(tvb, pinfo, offset, tree);
            break;

        case EDONKEY_MSG_SERVER_STATUS:  /* Server Status: <Nusers> <Nfiles> */
            nusers = tvb_get_letohl(tvb, offset); 
            nfiles = tvb_get_letohl(tvb, offset+4);
            proto_tree_add_text(tree, tvb, offset, 4, "Number of Users: %u", nusers);
            proto_tree_add_text(tree, tvb, offset+4, 4, "Number of Files: %u", nfiles);
            break;

        case EDONKEY_MSG_FILE_REQUEST_ANSWER: /* File Request Answer: <File hash> <File name> */
            offset = dissect_edonkey_file_hash(tvb, pinfo, offset, tree);
            offset = dissect_edonkey_file_name(tvb, pinfo, offset, tree);
            break;

        case EDONKEY_MSG_REQUEST_PARTS:  /* Request Parts: <File hash> <Start offset>(3) <End offset>(3) */
            offset = dissect_edonkey_file_hash(tvb, pinfo, offset, tree);
            offset = dissect_edonkey_start_offset(tvb, pinfo, offset, tree);
            offset = dissect_edonkey_start_offset(tvb, pinfo, offset, tree);
            offset = dissect_edonkey_start_offset(tvb, pinfo, offset, tree);
            offset = dissect_edonkey_end_offset(tvb, pinfo, offset, tree);
            offset = dissect_edonkey_end_offset(tvb, pinfo, offset, tree);
            offset = dissect_edonkey_end_offset(tvb, pinfo, offset, tree);
            break;

        case EDONKEY_MSG_SENDING_PART:  /* Sending Part: <File hash> <Start offset> <End offset> DATA */
            offset = dissect_edonkey_file_hash(tvb, pinfo, offset, tree);
            offset = dissect_edonkey_start_offset(tvb, pinfo, offset, tree);
            offset = dissect_edonkey_end_offset(tvb, pinfo, offset, tree);
            if (msg_end > offset) {
                bytes_remaining = msg_end - offset;
                proto_tree_add_text(tree, tvb, offset, bytes_remaining, "Message Data (%d bytes)", bytes_remaining);
            }
            break;


        case EDONKEY_MSG_SEARCH_USER_RESULTS: /* Search User Results: <Client info list> */                 
            offset = dissect_edonkey_client_info_list(tvb, pinfo, offset, tree);
            break;

        case EDONKEY_MSG_GET_SHARED_FILES:    /* Get Shared Files: <Directory> */
            offset = dissect_edonkey_directory(tvb, pinfo, offset, tree);
            break;

        case EDONKEY_MSG_SHARED_DIRS: /* Shared Dirs: <Directory List> */
            offset = dissect_edonkey_directory_list(tvb, pinfo, offset, tree);
            break;

        case EDONKEY_MSG_SHARED_FILES: /* Shared Files: <Directory> <File info list> */
            offset = dissect_edonkey_directory(tvb, pinfo, offset, tree);
            offset = dissect_edonkey_file_info_list(tvb, pinfo, offset, tree);
            break;

        case EDONKEY_MSG_HASHSET_ANSWER:      /* Hashset Answer: <Hash List>  */
            offset = dissect_edonkey_hash_list(tvb, pinfo, offset, tree);
            break;

        default:
            proto_tree_add_text(tree, tvb, offset, length, "Message Data (%d bytes)", length);
            break;
    }
	return;
}

static void dissect_emule_tcp_message(guint8 msg_type,
                                      tvbuff_t *tvb, packet_info *pinfo _U_, 
                                      int offset, int length, proto_tree *tree) 
{   
    int msg_start, msg_end, bytes_remaining;
    guint32 packed_length;
    guint16 version, rank;

    if (tree == NULL) return;

    bytes_remaining = tvb_reported_length_remaining(tvb, offset);
    if ((length < 0) || (length > bytes_remaining)) length = bytes_remaining;
    if (length <= 0) return;

    msg_start = offset;
    msg_end = offset + length;

    switch (msg_type) {
        case EMULE_MSG_HELLO:  /* eMule Hello: <eMule Version> <Meta tag list> */
        case EMULE_MSG_HELLO_ANSWER:  /* eMule Hello Answer: <eMule Version> <Meta tag list> */
            version = tvb_get_letohs(tvb, offset); 
            proto_tree_add_text(tree, tvb, offset, 2, "Version: %u", version);
            offset = dissect_edonkey_metatag_list(tvb, pinfo, offset+2, tree);
            break;

        case EMULE_MSG_QUEUE_RANKING: /* eMule Queue Ranking: <eMule Rank (guint16)> */
            rank = tvb_get_letohs(tvb, offset); 
            proto_tree_add_text(tree, tvb, offset, 2, "Queue Ranking: %u", rank);
            break;

        case EMULE_MSG_SOURCES_REQUEST: /* Sources Request: <File Hash> */
            offset = dissect_edonkey_file_hash(tvb, pinfo, offset, tree);
            break;

        case EMULE_MSG_SOURCES_ANSWER: /* Sources Answer: <File Hash> <Address List> */
            offset = dissect_edonkey_file_hash(tvb, pinfo, offset, tree);
            offset = dissect_emule_address_list(tvb, pinfo, offset, tree);
            break;

        case EMULE_MSG_DATA_COMPRESSED: /* Data Compressed: <File Hash> <Start Offset> <Length (guint32)> <DATA> */
            offset = dissect_edonkey_file_hash(tvb, pinfo, offset, tree);
            offset = dissect_edonkey_start_offset(tvb, pinfo, offset, tree);
            packed_length = tvb_get_letohl(tvb, offset);
            proto_tree_add_text(tree, tvb, offset, packed_length, "Packed Length: %u", packed_length);
            offset += 4;
            if (msg_end > offset) {
                bytes_remaining = msg_end - offset;
                proto_tree_add_text(tree, tvb, offset, bytes_remaining, 
                                    "Compressed Message Data (%d bytes)", bytes_remaining);
            }
            break;

        default:
            dissect_edonkey_tcp_message(msg_type, tvb, pinfo, offset, length, tree);
            break;
    }
	return;
}

static void dissect_edonkey_udp_message(guint8 msg_type,
                                        tvbuff_t *tvb, packet_info *pinfo _U_, 
                                        int offset, int length, proto_tree *tree) 
{   
    int msg_start, msg_end, bytes_remaining;
    guint8 type;
    guint16 min, max;
    guint32 nusers, nfiles;

    if (tree == NULL) return;

    bytes_remaining = tvb_reported_length_remaining(tvb, offset);
    if ((length < 0) || (length > bytes_remaining)) length = bytes_remaining;
    if (length <= 0) return;

    msg_start = offset;
    msg_end = offset + length;

    switch (msg_type) {
        /* EDonkey UDP Messages */
        case EDONKEY_MSG_UDP_CALLBACK_REQUEST: /* Callback Request: <Address> <Client ID> */
            offset = dissect_edonkey_address(tvb, pinfo, offset, tree);
            offset = dissect_edonkey_client_id(tvb, pinfo, offset, tree);
            break;

        case EDONKEY_MSG_UDP_CALLBACK_FAIL: /* Callback Fail: <Client ID> */
            offset = dissect_edonkey_client_id(tvb, pinfo, offset, tree);
            break;

        case EDONKEY_MSG_UDP_SERVER_INFO: /* Server Info: <String> <String>*/
            offset = dissect_edonkey_string(tvb, pinfo, offset, tree);
            offset = dissect_edonkey_string(tvb, pinfo, offset, tree);
            break;

        case EDONKEY_MSG_UDP_SERVER_LIST: /* Server List: <Address List> */
            offset = dissect_edonkey_address_list(tvb, pinfo, offset, tree);
            break;

        case EDONKEY_MSG_UDP_SEARCH_FILE_RESULTS: /* Search File Result: <File Info> */
            offset = dissect_edonkey_file_info(tvb, pinfo, offset, tree);
            break;

        case EDONKEY_MSG_UDP_SEARCH_FILE: /* Search File: <Search query> */
            offset = dissect_edonkey_search_query(tvb, pinfo, offset, tree);
            break;
       
        case EDONKEY_MSG_UDP_GET_SOURCES:     /* Get Sources: <File Hash> */
            offset = dissect_edonkey_file_hash(tvb, pinfo, offset, tree);
            break;

        case EDONKEY_MSG_UDP_FOUND_SOURCES: /* Found Sources: <File Hash> <Address List> */
            offset = dissect_edonkey_file_hash(tvb, pinfo, offset, tree);
            offset = dissect_edonkey_address_list(tvb, pinfo, offset, tree);
            break;

        case EDONKEY_MSG_UDP_SERVER_STATUS:  /* Server Status: <guint32> <Nusers> <Nfiles> <Nusersmax> */
            offset += 4;
            nusers = tvb_get_letohl(tvb, offset); 
            nfiles = tvb_get_letohl(tvb, offset+4);
            proto_tree_add_text(tree, tvb, offset, 4, "Number of Users: %u", nusers);
            proto_tree_add_text(tree, tvb, offset+4, 4, "Number of Files: %u", nfiles);
            offset += 8;
            if (offset < msg_end) {
                nusers = tvb_get_letohl(tvb, offset);
                proto_tree_add_text(tree, tvb, offset, 4, "Max number of Users: %u", nusers);
                offset += 4;
            }
            break;

        /* Overnet UDP Messages */
        case OVERNET_MSG_UDP_CONNECT:    /* Connect:   <Peer (sender) > */
        case OVERNET_MSG_UDP_PUBLICIZE:  /* Publicize: <Peer (sender) > */
            offset = dissect_overnet_peer(tvb, pinfo, offset, tree);
            break;

        case OVERNET_MSG_UDP_CONNECT_REPLY:    /* Connect Reply: <guint16 Peer List> */
            offset = dissect_edonkey_list(tvb, pinfo, offset, tree, 2, "Overnet Peer", dissect_overnet_peer);
            break;

        case OVERNET_MSG_UDP_SEARCH:    /* Search: <search type (guint8)> <Hash> */
            type = tvb_get_guint8(tvb, offset);
            proto_tree_add_text(tree, tvb, offset, 1, "Search Type: %u", type);
            offset = dissect_edonkey_hash(tvb, pinfo, offset+1, tree);
            break;

        case OVERNET_MSG_UDP_SEARCH_INFO: 
           /* Search Info: <Hash> <search type (guint8)> <min (guint16)> <max (guint16)>*/
            offset = dissect_edonkey_hash(tvb, pinfo, offset, tree);
            type = tvb_get_guint8(tvb, offset);
            min = tvb_get_letohs(tvb, offset+1);
            max = tvb_get_letohs(tvb, offset+3);
            proto_tree_add_text(tree, tvb, offset, 1, "Search Type: %u", type);
            proto_tree_add_text(tree, tvb, offset+1, 4, "Search Range: Min=%u Max=%u", min, max);
            break;

        case OVERNET_MSG_UDP_SEARCH_NEXT:    /* Search Next: <Hash> <guint8 Peer List> */
            offset = dissect_edonkey_hash(tvb, pinfo, offset, tree);
            offset = dissect_edonkey_list(tvb, pinfo, offset, tree, 1, "Overnet Peer", dissect_overnet_peer);
            break;

        case OVERNET_MSG_UDP_SEARCH_RESULT:  /* Search Result: <Hash> <Hash> <Meta tag List> */
        case OVERNET_MSG_UDP_PUBLISH:        /* Publish: <Hash> <Hash> <Meta tag List> */
            offset = dissect_edonkey_hash(tvb, pinfo, offset, tree);
            offset = dissect_edonkey_hash(tvb, pinfo, offset, tree);
            offset = dissect_edonkey_metatag_list(tvb, pinfo, offset, tree); 
            break;

        case OVERNET_MSG_UDP_SEARCH_END:  /* Search End: <Hash> */
            offset = dissect_edonkey_hash(tvb, pinfo, offset, tree);
            break;

        case OVERNET_MSG_UDP_PUBLISH_ACK:  /* Publish ACK: <File Hash> */
            offset = dissect_edonkey_file_hash(tvb, pinfo, offset, tree);
            break;

        case OVERNET_MSG_UDP_IP_QUERY:  /* IP Query: <TCP Port> */
            proto_tree_add_item(tree, hf_edonkey_port, tvb, offset, 2, TRUE);
            break;

        case OVERNET_MSG_UDP_IP_QUERY_ANSWER:  /* IP Query Answer: <IP> */
            offset = dissect_edonkey_client_id(tvb, pinfo, offset, tree);
            break;

        case OVERNET_MSG_UDP_IDENTIFY_REPLY:  /* Identify Reply: <Contact (sender)> */
            /* <Contact> ::= <Hash> <Address> */
            offset = dissect_edonkey_hash(tvb, pinfo, offset, tree);
            offset = dissect_edonkey_address(tvb, pinfo, offset, tree);
            break;

        case OVERNET_MSG_UDP_IDENTIFY_ACK:  /* Identify Reply: <TCP Port (sender)> */
            proto_tree_add_item(tree, hf_edonkey_port, tvb, offset, 2, TRUE);
            break;

        case OVERNET_MSG_UDP_FIREWALL_CONNECTION:      /* Firewall Connnection  Ack: <Hash> <TCP Port> */
            offset = dissect_edonkey_client_hash(tvb, pinfo, offset, tree);
            proto_tree_add_item(tree, hf_edonkey_port, tvb, offset, 2, TRUE);
            break;

        case OVERNET_MSG_UDP_FIREWALL_CONNECTION_ACK:  /* Firewall Connnection  Ack: <Hash> */
        case OVERNET_MSG_UDP_FIREWALL_CONNECTION_NACK: /* Firewall Connnection NAck: <Hash> */
            offset = dissect_edonkey_client_hash(tvb, pinfo, offset, tree);
            break;

        default:
            proto_tree_add_text(tree, tvb, offset, length, "Message Data (%d bytes)", length);
            break;
    }
	return;
}

static void dissect_emule_udp_message(guint8 msg_type,
                                      tvbuff_t *tvb, packet_info *pinfo _U_, 
                                      int offset, int length, proto_tree *tree) 
{   
    int msg_start, msg_end, bytes_remaining;
    guint16 rank;

    bytes_remaining = tvb_reported_length_remaining(tvb, offset);
    if ((length < 0) || (length > bytes_remaining)) length = bytes_remaining;
    if (length <= 0) return;

    msg_start = offset;
    msg_end = offset + length;

    switch (msg_type) {
        case EMULE_MSG_UDP_REASKFILEPING:     /* Reask File Ping: <File Hash> */
            offset = dissect_edonkey_file_hash(tvb, pinfo, offset, tree);
            break;

        case EMULE_MSG_UDP_REASKACK:          /* Reask ACK:     <eMule Rank>  */
            rank = tvb_get_letohs(tvb, offset); 
            proto_tree_add_text(tree, tvb, offset, 2, "Queue Ranking: %u", rank);
            break;

        default:
            dissect_edonkey_udp_message(msg_type, tvb, pinfo, offset, length,tree);
            break;
    }
	return;
}

static void dissect_edonkey_tcp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree) 
{
  	proto_item *ti;
	proto_tree *edonkey_tree = NULL, *edonkey_msg_tree = NULL;
	int offset, bytes, messages;
	guint8 protocol, msg_type;
    guint32 msg_len;
    gchar *protocol_name, *message_name;
    void  (*dissector)(guint8, tvbuff_t*, packet_info*, int, int, proto_tree*);

	if (check_col(pinfo->cinfo, COL_PROTOCOL))
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "eDonkey");

	if (check_col(pinfo->cinfo, COL_INFO))
		col_clear(pinfo->cinfo, COL_INFO);

	if (tree) {
        ti = proto_tree_add_item(tree, proto_edonkey, tvb, 0, -1, FALSE);
        edonkey_tree = proto_item_add_subtree(ti, ett_edonkey);
    }

    offset = 0;
    messages = 0;
    while (tvb_length_remaining(tvb, offset) >= EDONKEY_TCP_HEADER_LENGTH) {
        protocol = tvb_get_guint8(tvb, offset);
        msg_len = tvb_get_letohl(tvb, offset+1);
        
        protocol_name = match_strval(protocol, edonkey_protocols);
        if (protocol_name == NULL) {
            /* Not a recognized eDonkey protocol - probably a continuation */
            if (check_col(pinfo->cinfo, COL_INFO))
                col_add_str(pinfo->cinfo, COL_INFO, "eDonkey Continuation");
            if (edonkey_tree) {
                bytes = tvb_length_remaining(tvb, offset);
                proto_tree_add_text(edonkey_tree, tvb, 0, -1, "Continuation data (%d bytes)", bytes);
            }
            return;
        }

        /* Add edonkey message tree */
        if (edonkey_tree) {
            ti = proto_tree_add_item(edonkey_tree, hf_edonkey_message, tvb, 
                                     offset, EDONKEY_TCP_HEADER_LENGTH + msg_len, FALSE);
            edonkey_msg_tree = proto_item_add_subtree(ti, ett_edonkey_message);
            
            proto_tree_add_uint_format(edonkey_msg_tree, hf_edonkey_protocol, tvb, offset, 1, protocol,
                                       "Protocol: %s (0x%02x)", protocol_name, protocol);
            proto_tree_add_uint(edonkey_msg_tree, hf_edonkey_message_length, tvb, offset+1, 4, msg_len);
        }


        /* Skip past the EDONKEY Header */
        offset += EDONKEY_TCP_HEADER_LENGTH;
        
        if(tvb_reported_length_remaining(tvb, offset) <= 0) {
            /* There is not enough space for the msg_type - mark as fragment */
            if (check_col(pinfo->cinfo, COL_INFO)) {
                if (messages == 0)
                    col_append_fstr(pinfo->cinfo, COL_INFO, "%s TCP Message Fragment", protocol_name);
                else col_append_fstr(pinfo->cinfo, COL_INFO, "; %s TCP Message Fragment", protocol_name);
            }
            return;
        } 

        if (check_col(pinfo->cinfo, COL_INFO)) {
            if (messages == 0)
                col_append_fstr(pinfo->cinfo, COL_INFO, "%s TCP", protocol_name);
            else col_append_fstr(pinfo->cinfo, COL_INFO, "; %s TCP", protocol_name);
        }

        msg_type = tvb_get_guint8(tvb, offset);
        switch (protocol) {
            case EDONKEY_PROTO_EDONKEY:
                message_name =  val_to_str(msg_type, edonkey_tcp_msgs, "Unknown");
                dissector = dissect_edonkey_tcp_message;
                break;
            
            case EDONKEY_PROTO_EMULE_EXT:
                message_name = val_to_str(msg_type, emule_tcp_msgs,
                                          val_to_str(msg_type, edonkey_tcp_msgs, "Unknown"));
                dissector = dissect_emule_tcp_message;
                break;

            default:
                message_name = "Unknown";
                dissector = NULL;
                break;
        }

        if (check_col(pinfo->cinfo, COL_INFO)) {
            col_append_fstr(pinfo->cinfo, COL_INFO, ": %s", message_name);
        }


        if (edonkey_msg_tree) {
            proto_tree_add_uint_format(edonkey_msg_tree, hf_edonkey_message_type, tvb, offset, 1, msg_type,
                                       "Message Type: %s (0x%02x)", message_name, msg_type);
            if (dissector && (msg_len > 1)) 
                (*dissector)(msg_type, tvb, pinfo, offset+1, msg_len-1, edonkey_msg_tree);
        }

        offset += msg_len;
        messages++;
	}
}

static void dissect_edonkey_udp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree) 
{
  	proto_item *ti;
	proto_tree *edonkey_tree = NULL, *edonkey_msg_tree = NULL;
	int offset;
	guint8 protocol, msg_type;
    gchar *protocol_name, *message_name;

	if (check_col(pinfo->cinfo, COL_PROTOCOL))
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "eDonkey");

	if (check_col(pinfo->cinfo, COL_INFO))
		col_set_str(pinfo->cinfo, COL_INFO, "eDonkey UDP Message");

	if (tree) {
        ti = proto_tree_add_item(tree, proto_edonkey, tvb, 0, -1, FALSE);
        edonkey_tree = proto_item_add_subtree(ti, ett_edonkey);
    }

    offset = 0;
    /* eDonkey UDP message - Assume that there is one message per packet */
    if (tvb_length_remaining(tvb, offset) >= EDONKEY_UDP_HEADER_LENGTH) {
        protocol = tvb_get_guint8(tvb, offset);
        msg_type = tvb_get_guint8(tvb, offset+1);
        protocol_name = val_to_str(protocol, edonkey_protocols, "Unknown");
        message_name = val_to_str(msg_type, edonkey_udp_msgs, "Unknown");

        if (check_col(pinfo->cinfo, COL_INFO)) {
            col_add_fstr(pinfo->cinfo, COL_INFO, "%s UDP: %s", protocol_name, message_name);
        }

        if (edonkey_tree) {
            ti = proto_tree_add_item(edonkey_tree, hf_edonkey_message, tvb, offset, -1, FALSE);
            edonkey_msg_tree = proto_item_add_subtree(ti, ett_edonkey_message);
            
		    proto_tree_add_uint_format(edonkey_msg_tree, hf_edonkey_protocol, tvb, offset, 1, protocol,
                                       "Protocol: %s (0x%02x)", protocol_name, protocol);
		    proto_tree_add_uint_format(edonkey_msg_tree, hf_edonkey_message_type, tvb, offset+1, 1, msg_type,
                                       "Message Type: %s (0x%02x)", message_name, msg_type);
            
            offset += EDONKEY_UDP_HEADER_LENGTH;

            switch (protocol) {
                case EDONKEY_PROTO_EDONKEY:
                    dissect_edonkey_udp_message(msg_type, tvb, pinfo, offset, -1, edonkey_msg_tree);
                    break;
                            
                case EDONKEY_PROTO_EMULE_EXT:
                    dissect_emule_udp_message(msg_type, tvb, pinfo, offset, -1, edonkey_msg_tree); 
                    break;

                default:
                    break;
            }
        }
	}
}

void proto_register_edonkey(void) {

	static hf_register_info hf[] = {
		{ &hf_edonkey_message,  
          { "eDonkey Message", "edonkey.message",
            FT_NONE, BASE_NONE, NULL, 0, "eDonkey Message", HFILL } },
		{ &hf_edonkey_protocol, 
          { "Protocol", "edonkey.protocol",
            FT_UINT8, BASE_HEX, NULL, 0, "eDonkey Protocol", HFILL } },
		{ &hf_edonkey_message_length,   
          { "Message Length", "edonkey.message.length",
            FT_UINT32, BASE_DEC, NULL, 0, "eDonkey Message Length", HFILL } },
		{ &hf_edonkey_message_type,   
          { "Message Type", "edonkey.message.type",
            FT_UINT8, BASE_HEX, NULL, 0, "eDonkey Message Type", HFILL } },
        { &hf_edonkey_client_hash,
          { "Client Hash", "edonkey.client_hash",
            FT_BYTES, BASE_HEX, NULL, 0, "eDonkey Client Hash", HFILL } },
        { &hf_edonkey_server_hash,
          { "Server Hash", "edonkey.server_hash",
            FT_BYTES, BASE_HEX, NULL, 0, "eDonkey Server Hash", HFILL } },
        { &hf_edonkey_file_hash,
          { "File Hash", "edonkey.file_hash",
            FT_BYTES, BASE_HEX, NULL, 0, "eDonkey File Hash", HFILL } },
        { &hf_edonkey_client_id,
          { "Client ID", "edonkey.clientid",
            FT_IPv4, BASE_DEC, NULL, 0, "eDonkey Client ID", HFILL } },
        { &hf_edonkey_ip,
          { "IP", "edonkey.ip",
            FT_IPv4, BASE_DEC, NULL, 0, "eDonkey IP", HFILL } },
        { &hf_edonkey_port,
          { "Port", "edonkey.port",
            FT_UINT16, BASE_DEC, NULL, 0, "eDonkey Port", HFILL } },
		{ &hf_edonkey_metatag,  
          { "eDonkey Meta Tag", "edonkey.metatag",
            FT_NONE, BASE_NONE, NULL, 0, "eDonkey Meta Tag", HFILL } },
        { &hf_edonkey_metatag_type,
          { "Meta Tag Type", "edonkey.metatag.type",
            FT_UINT8, BASE_HEX, NULL, 0, "eDonkey Meta Tag Type", HFILL } },
        { &hf_edonkey_metatag_id,
          { "Meta Tag ID", "edonkey.metatag.id",
            FT_UINT8, BASE_HEX, NULL, 0, "eDonkey Meta Tag ID", HFILL } },
        { &hf_edonkey_metatag_name,
          { "Meta Tag Name", "edonkey.metatag.name",
            FT_STRING, BASE_NONE, NULL, 0, "eDonkey Meta Tag Name", HFILL } },
        { &hf_edonkey_metatag_namesize,
          { "Meta Tag Name Size", "edonkey.metatag.namesize",
            FT_UINT16, BASE_DEC, NULL, 0, "eDonkey Meta Tag Name Size", HFILL } },
		{ &hf_edonkey_search,  
          { "eDonkey Search", "edonkey.search",
            FT_NONE, BASE_NONE, NULL, 0, "eDonkey Search", HFILL } },
        { &hf_edonkey_hash,
          { "Hash", "edonkey.hash",
            FT_BYTES, BASE_HEX, NULL, 0, "eDonkey Hash", HFILL } },
        { &hf_edonkey_string,
          { "String", "edonkey.string",
            FT_STRING, BASE_NONE, NULL, 0, "eDonkey String", HFILL } },
        { &hf_edonkey_string_length,
          { "String Length", "edonkey.string_length",
            FT_UINT16, BASE_DEC, NULL, 0, "eDonkey String Length", HFILL } },
        { &hf_edonkey_directory,
          { "Directory", "edonkey.directory",
            FT_STRING, BASE_NONE, NULL, 0, "eDonkey Directory", HFILL } },
		{ &hf_edonkey_fileinfo,  
          { "eDonkey File Info", "edonkey.fileinfo",
            FT_NONE, BASE_NONE, NULL, 0, "eDonkey File Info", HFILL } },
		{ &hf_edonkey_serverinfo,  
          { "eDonkey Server Info", "edonkey.serverinfo",
            FT_NONE, BASE_NONE, NULL, 0, "eDonkey Server Info", HFILL } },
		{ &hf_edonkey_clientinfo,  
          { "eDonkey Client Info", "edonkey.clientinfo",
            FT_NONE, BASE_NONE, NULL, 0, "eDonkey Client Info", HFILL } },
		{ &hf_overnet_peer,  
          { "Overnet Peer", "overnet.peer",
            FT_NONE, BASE_NONE, NULL, 0, "Overnet Peer", HFILL } },
	};

	static gint *ett[] = {
		&ett_edonkey,
        &ett_edonkey_message,
        &ett_edonkey_metatag,
        &ett_edonkey_search,
        &ett_edonkey_fileinfo,
        &ett_edonkey_serverinfo,
        &ett_edonkey_clientinfo,
        &ett_overnet_peer
	};

	proto_edonkey = proto_register_protocol("eDonkey Protocol", "EDONKEY", "edonkey");

	proto_register_field_array(proto_edonkey, hf, array_length(hf));

	proto_register_subtree_array(ett, array_length(ett));
	register_dissector("edonkey.tcp", dissect_edonkey_tcp, proto_edonkey);
	register_dissector("edonkey.udp", dissect_edonkey_udp, proto_edonkey);
}

void proto_reg_handoff_edonkey(void) {
	dissector_handle_t edonkey_tcp_handle;
	dissector_handle_t edonkey_udp_handle;

	edonkey_tcp_handle = find_dissector("edonkey.tcp");
	edonkey_udp_handle = find_dissector("edonkey.udp");

	dissector_add("tcp.port", 4661, edonkey_tcp_handle);
	dissector_add("tcp.port", 4662, edonkey_tcp_handle);
	dissector_add("tcp.port", 4663, edonkey_tcp_handle);
	dissector_add("udp.port", 4665, edonkey_udp_handle);
	dissector_add("udp.port", 4672, edonkey_udp_handle);
}
