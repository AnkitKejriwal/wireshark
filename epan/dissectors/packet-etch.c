/* packet-etch.c
 *
 * Copyright (c) 2010, Holger Grandy, BMW Car IT GmbH (holger.grandy@bmw-carit.de)
 *
 * $Id$
 *
 * Apache Etch Protocol dissector
 * http://incubator.apache.org/etch/
 *
 * This dissector reads configuration files (generated by Etch IDL compiler).
 * Configuration file directory path is given in dissector options.
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
 */


/* Include files */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>

#include <stdio.h>
#include <string.h>

#include <wsutil/file_util.h>
#include <epan/packet.h>
#include <epan/prefs.h>
#include <epan/report_err.h>
#include <epan/dissectors/packet-tcp.h>

/*
 * maximum numbers for symbols from config files
 */
#define ETCH_MAX_SYMBOL_LENGTH "256"
#define ETCH_MAX_SYMBOLS 1000

/*
 * Magic Number for Etch
 */
static const guint8 etch_magic[] = { 0xde, 0xad, 0xbe, 0xef };

/*
 * Typecodes in the Etch protocol, representing the field types
 */
#define ETCH_TC_NULL            0x80
#define ETCH_TC_NONE            0x81
#define ETCH_TC_BOOLEAN_FALSE   0x82
#define ETCH_TC_BOOLEAN_TRUE    0x83
#define ETCH_TC_BYTE            0x84
#define ETCH_TC_SHORT           0x85
#define ETCH_TC_INT             0x86
#define ETCH_TC_LONG            0x87
#define ETCH_TC_FLOAT           0x88
#define ETCH_TC_DOUBLE          0x89
#define ETCH_TC_BYTES           0x8B
#define ETCH_TC_ARRAY           0x91
#define ETCH_TC_EMPTY_STRING    0x92
#define ETCH_TC_STRING          0x93
#define ETCH_TC_STRUCT          0x94
#define ETCH_TC_CUSTOM          0x95
#define ETCH_TC_ANY             0x96
#define ETCH_TC_MIN_TINY_INT    0xC0
#define ETCH_TC_MAX_TINY_INT    0x7F

/***************************************************************************/
/* Variables */

/*
 * String representation of all Type Codes
 */
static const value_string tc_lookup_table[] = {
  { ETCH_TC_NULL,          "Etch TypeCode: NULL"},
  { ETCH_TC_NONE,          "Etch TypeCode: NONE"},
  { ETCH_TC_BOOLEAN_FALSE, "Etch TypeCode: BOOLEAN_FALSE" },
  { ETCH_TC_BOOLEAN_TRUE,  "Etch TypeCode: BOOLEAN_TRUE"},
  { ETCH_TC_BYTE,          "Etch TypeCode: BYTE"},
  { ETCH_TC_SHORT,         "Etch TypeCode: SHORT"},
  { ETCH_TC_INT,           "Etch TypeCode: INT"},
  { ETCH_TC_LONG,          "Etch TypeCode: LONG"},
  { ETCH_TC_FLOAT,         "Etch TypeCode: FLOAT"},
  { ETCH_TC_DOUBLE,        "Etch TypeCode: DOUBLE"},
  { ETCH_TC_BYTES,         "Etch TypeCode: BYTES"},
  { ETCH_TC_ARRAY,         "Etch TypeCode: ARRAY"},
  { ETCH_TC_EMPTY_STRING,  "Etch TypeCode: EMPTY_STRING"},
  { ETCH_TC_STRING,        "Etch TypeCode: STRING"},
  { ETCH_TC_STRUCT,        "Etch TypeCode: STRUCT"},
  { ETCH_TC_CUSTOM,        "Etch TypeCode: CUSTOM"},
  { ETCH_TC_ANY,           "Etch TypeCode: ANY"},
  { 0,                     NULL}
};

/*
 * Wireshark internal fields
 */
static int proto_etch = -1;
static gint ett_etch = -1;
static gint ett_etch_struct = -1;
static gint ett_etch_keyvalue = -1;
static gint ett_etch_key = -1;
static gint ett_etch_value = -1;
static int hf_etch_sig = 0;
static int hf_etch_length = 0;
static int hf_etch_version = 0;
static int hf_etch_typecode = 0;
static int hf_etch_value = 0;
static int hf_etch_bytes = 0;
static int hf_etch_byte = 0;
static int hf_etch_short = 0;
static int hf_etch_int = 0;
static int hf_etch_long = 0;
static int hf_etch_float = 0;
static int hf_etch_double = 0;
static int hf_etch_key = 0;
static int hf_etch_valuename = 0;
static int hf_etch_keyname = 0;
static int hf_etch_string = 0;
static int hf_etch_keyvalue = 0;
static int hf_etch_struct = 0;
static int hf_etch_dim = 0;
static int hf_etch_symbol = 0;

/*
 * internal fields/defines for dissector
 */

static const char*       gbl_keytab_folder = "";
static guint             gbl_etch_port = 0;
static char*             gbl_current_keytab_folder = NULL;
static int               gbl_symbols_count = 0;
static value_string*     gbl_symbols = NULL;

static int               gbl_pdu_counter;
static guint32           gbl_old_frame_num;

static emem_strbuf_t*    gbl_symbol_buffer = NULL;
static gboolean          gbl_have_symbol = FALSE;

/***************************************************************************/
/* Methods */

/*
 * forward declared dissector methods
 */
static void read_key_value(unsigned int *offset, tvbuff_t * tvb,
                          proto_tree * etch_tree);
static void read_struct(unsigned int *offset, tvbuff_t * tvb,
                        proto_tree * etch_tree, int add_type_field);
static int read_value(unsigned int *offset, tvbuff_t * tvb, proto_tree * etch_tree,
                      int asWhat);
void proto_reg_handoff_etch(void);

/*********************************************************************************/
/* Aux Functions */

/*
 * get the length of a given typecode in bytes, -1 if to be derived from message
 */
static gint32 get_byte_length(guint8 typecode)
{
  switch (typecode) {
  case ETCH_TC_NULL:
    return  0;
    break;
  case ETCH_TC_NONE:
    return  0;
    break;
  case ETCH_TC_BOOLEAN_FALSE:
    return  0;
    break;
  case ETCH_TC_BOOLEAN_TRUE:
    return  0;
    break;
  case ETCH_TC_BYTE:
    return  1;
    break;
  case ETCH_TC_SHORT:
    return  2;
    break;
  case ETCH_TC_INT:
    return  4;
    break;
  case ETCH_TC_LONG:
    return  8;
    break;
  case ETCH_TC_FLOAT:
    return  4;
    break;
  case ETCH_TC_DOUBLE:
    return  8;
    break;
  case ETCH_TC_BYTES:
    return  -1;
    break;
  case ETCH_TC_ARRAY:
    return  -1;
    break;
  case ETCH_TC_EMPTY_STRING:
    return  0;
    break;
  case ETCH_TC_STRING:
    return  -1;
    break;
  case ETCH_TC_STRUCT:
    return  -1;
    break;
  case ETCH_TC_CUSTOM:
    return  -1;
    break;
  case ETCH_TC_ANY:
    return  -1;
    break;
  case ETCH_TC_MIN_TINY_INT:
    return  0;
    break;
  case ETCH_TC_MAX_TINY_INT:
    return  0;
    break;
  }
  return 0;
}

/*
 * add a etch symbol to our symbol cache
 */
static void add_symbol(int hash, const gchar *symbol)
{
  if (gbl_symbols_count < ETCH_MAX_SYMBOLS - 1) {
    gbl_symbols[gbl_symbols_count].value  = hash;
    gbl_symbols[gbl_symbols_count].strptr = g_strdup_printf("%." ETCH_MAX_SYMBOL_LENGTH "s", symbol);
    gbl_symbols_count++;
  }
}

/*
 * add all etch symbols from file to our symbol cache
 */
static void add_symbols_of_file(const char *filename)
{
  FILE *pFile;
  pFile = ws_fopen(filename, "r");

  if (pFile != NULL) {
    char line[256];
    while (fgets(line, sizeof line, pFile) != NULL) {
      int length, pos, hash;
      length = strlen(line);

      /* Must at least have a hash, else skip line */
      if (length < 10)
        continue;

      pos = length - 1;
      while (pos > 0 && (line[pos] == 0xD || line[pos] == 0xA)) {
        pos--;
      }
      line[pos + 1] = '\0';

     /* Parse the Hash */
      if (sscanf(&line[0], "%x", &hash) != 1)
        continue;  /* didn't find a valid hex value at the beginning of the line */

      /* And read the symbol */
      pos = strcspn(line, ",");
      if ((line[pos] != '\0') && (line[pos+1] !='\0')) /* require at least 1 char in symbol */
        add_symbol(hash, &line[pos+1]);
      }
    fclose(pFile);
  }
}

/*
 * add all etch symbol from directory to our symbol cache
 */
static void read_hashed_symbols_from_dir(const char *dirname)
{
  WS_DIR     *dir;
  WS_DIRENT  *file;
  const char *name;
  char       *filename;
  GError     *err_p = NULL;
  int         i;

  if(gbl_current_keytab_folder != NULL) {
    g_free(gbl_current_keytab_folder);
    gbl_current_keytab_folder = NULL;
  }

  for(i = 0; i < gbl_symbols_count; i++){
    value_string tmp = {0, NULL};
    g_free((gchar *)gbl_symbols[i].strptr);
    gbl_symbols[i] = tmp;
  }
  /* Reset all symbols */
  gbl_symbols_count = 0;

  if ((dirname == NULL) || (dirname[0] == '\0'))
    return;

  if ((dir = ws_dir_open(dirname, 0, &err_p)) != NULL) {
    gbl_current_keytab_folder = g_strdup(dirname);
    while ((file = ws_dir_read_name(dir)) != NULL) {
      name = ws_dir_get_name(file);

      if (g_str_has_suffix(file, ".ewh")) {
        filename =
          g_strdup_printf("%s" G_DIR_SEPARATOR_S "%s", dirname,
                          name);
        add_symbols_of_file(filename);
        g_free(filename);
      }
    }
    ws_dir_close(dir);
  }else{
    report_failure("%s", err_p->message);
    g_error_free(err_p);
  }
}

/***********************************************************************************/
/* Etch Protocol Functions*/

/*
 * read a type flag from tvb and add it to tree
 */
static guint8 read_type(unsigned int *offset, tvbuff_t * tvb, proto_tree * etch_tree)
{

  guint32 type_code;
  const gchar* type_as_string;

  type_code = tvb_get_guint8(tvb, *offset);
  type_as_string = val_to_str(type_code, tc_lookup_table, "Etch TypeCode: 0x%02x");
  proto_tree_add_text(etch_tree, tvb, *offset, 1, "%s", type_as_string);
  (*offset)++;
  return type_code;
}

/*
 * read a array type flag and add it to tree
 */
static void read_array_type(unsigned int *offset, tvbuff_t * tvb,
                           proto_tree * etch_tree)
{
  guint32 type_code;

  type_code = tvb_get_guint8(tvb, *offset);

  read_type(offset, tvb, etch_tree);
  if (type_code == (guint8) ETCH_TC_CUSTOM) {
    type_code = read_type(offset, tvb, etch_tree);
    proto_tree_add_item(etch_tree, hf_etch_value, tvb, *offset, 4,
                        ENC_BIG_ENDIAN);
    (*offset) += 4;
  }
}


/*
 * read the length of an array and add it to tree
 */
guint32 read_length(unsigned int *offset, tvbuff_t * tvb,
                proto_tree * etch_tree)
{
  guint32 length;
  int length_of_array_length_type;
  guint8 tiny;

  tiny = tvb_get_guint8(tvb, *offset);

  /*  Is this the value already?*/
  if (  tiny <= ETCH_TC_MAX_TINY_INT
        || tiny >= ETCH_TC_MIN_TINY_INT) {
    length = tiny;
    length_of_array_length_type = 1;
  } else {
    guint8 type_code;
    type_code = read_type(offset, tvb, etch_tree);
    length_of_array_length_type = get_byte_length(type_code);

    switch (length_of_array_length_type) {
    case 1:
      length = (guint32) tvb_get_guint8(tvb, *offset);
      break;
    case 2:
      length = (guint32) tvb_get_ntohs(tvb, *offset);
      break;
    case 4:
      length = (guint32) tvb_get_ntohl(tvb, *offset);
      break;
    default:
      return 0;             /* error!*/
    }
  }
  proto_tree_add_item(etch_tree, hf_etch_length, tvb, *offset,
                      length_of_array_length_type, ENC_BIG_ENDIAN);
  (*offset) += length_of_array_length_type;
  return length;
}


/*
 * read an array from tvb and add it to tree
 */
static void read_array(unsigned int *offset, tvbuff_t * tvb, proto_tree * etch_tree)
{
  int length;

  /*  array type*/
  read_type(offset, tvb, etch_tree);

  /*  Array of type:*/
  read_array_type(offset, tvb, etch_tree);

  /*  Array dim*/
  proto_tree_add_item(etch_tree, hf_etch_dim, tvb, *offset, 1, ENC_NA);
  (*offset)++;

  /*  Array length*/
  length = read_length(offset, tvb, etch_tree);

  for (; length > 0; length--) {
    read_value(offset, tvb, etch_tree, hf_etch_value);
  }
  /*  terminaton*/
  read_type(offset, tvb, etch_tree);
}


/*
 * read a sequence of bytes and add them to tree
 */
static void read_bytes(unsigned int *offset, tvbuff_t * tvb, proto_tree * etch_tree)
{
  int length;
  read_type(offset, tvb, etch_tree);
  length = read_length(offset, tvb, etch_tree);
  proto_tree_add_item(etch_tree, hf_etch_bytes, tvb, *offset, length,
                      ENC_NA);
  (*offset) += length;
}

/*
 * read a string and add it to tree
 */
static void read_string(unsigned int *offset, tvbuff_t * tvb,
                proto_tree * etch_tree)
{
  int byteLength;
  read_type(offset, tvb, etch_tree);

  byteLength = read_length(offset, tvb, etch_tree);

  proto_tree_add_item(etch_tree, hf_etch_string, tvb, *offset,
                      byteLength, ENC_NA);
  (*offset) += byteLength;
}

/*
 * read a number and add it to tree
 */
static void read_number(unsigned int *offset, tvbuff_t * tvb,
                       proto_tree * etch_tree, int asWhat, guint8 type_code)
{
  int byteLength;

  read_type(offset, tvb, etch_tree);
  byteLength = get_byte_length(type_code);
  if (byteLength > 0) {
    proto_item *ti;
    const gchar* symbol = NULL;
    guint32 hash = 0;

    gbl_symbol_buffer = ep_strbuf_new_label("");  /* no symbol found yet */
    if (byteLength == 4) {
      hash = tvb_get_ntohl(tvb, *offset);
      symbol = match_strval((const guint32)hash, gbl_symbols);
      if(symbol != NULL) {
        asWhat = hf_etch_symbol;
        gbl_have_symbol = TRUE;
        ep_strbuf_append_printf(gbl_symbol_buffer,"%s",symbol);
      }
    }
    ti = proto_tree_add_item(etch_tree, asWhat, tvb, *offset,
                             byteLength, ENC_BIG_ENDIAN);
    *offset += byteLength;
    if (symbol != NULL) {
      proto_item_append_text(ti, " (0x%08x) %s", hash, symbol);
    }
  }
}

/*
 * read a value and add it to tree
 */
static int read_value(unsigned int *offset, tvbuff_t * tvb, proto_tree * etch_tree,
                      int asWhat)
{
  guint8 type_code;

  type_code = tvb_get_guint8(tvb, *offset);
  if (type_code <= ETCH_TC_MAX_TINY_INT ||
      type_code >= ETCH_TC_MIN_TINY_INT) {
    /* this is the value already */
    proto_tree_add_item(etch_tree, asWhat, tvb, *offset, 1, ENC_BIG_ENDIAN);
    (*offset)++;
    return type_code;
  } else if (type_code == ETCH_TC_CUSTOM) {
    read_struct(offset, tvb, etch_tree, 1);
    return 0;
  } else if (type_code == ETCH_TC_ARRAY) {
    read_array(offset, tvb, etch_tree);
    return 0;
  } else if (type_code == ETCH_TC_STRING) {
    read_string(offset, tvb, etch_tree);
    return 0;
  } else if (type_code == ETCH_TC_FLOAT) {
    read_number(offset, tvb, etch_tree, hf_etch_float, type_code);
  } else if (type_code == ETCH_TC_DOUBLE) {
    read_number(offset, tvb, etch_tree, hf_etch_double, type_code);
  } else if (type_code == ETCH_TC_SHORT) {
    read_number(offset, tvb, etch_tree, hf_etch_short, type_code);
  } else if (type_code == ETCH_TC_INT) {
    read_number(offset, tvb, etch_tree, hf_etch_int, type_code);
  } else if (type_code == ETCH_TC_LONG) {
    read_number(offset, tvb, etch_tree, hf_etch_long, type_code);
  } else if (type_code == ETCH_TC_BYTE) {
    read_number(offset, tvb, etch_tree, hf_etch_byte, type_code);
  } else if (type_code == ETCH_TC_BYTES) {
    read_bytes(offset, tvb, etch_tree);
  } else {
    read_number(offset, tvb, etch_tree, asWhat, type_code);
  }
  return 0;
}

/*
 * read a struct and add it to tree
 */
static void read_struct(unsigned int *offset, tvbuff_t * tvb,
                        proto_tree * etch_tree, int add_type_field)
{
  proto_item *ti;
  proto_tree *new_tree;
  int length;
  int i;

  ti = proto_tree_add_item(etch_tree, hf_etch_struct, tvb, *offset,
                           tvb->length - *offset, ENC_NA);
  new_tree = proto_item_add_subtree(ti, ett_etch_struct);

  if (add_type_field) {
    read_type(offset, tvb, new_tree);
  }
  /* struct type as hash*/
  read_value(offset, tvb, new_tree, hf_etch_value);

  /*  struct length */
  length = read_value(offset, tvb, new_tree, hf_etch_length);

  for (i = 0; i < length; i++) {
    read_key_value(offset, tvb, new_tree);
  }

  /* termination */
  read_type(offset, tvb, new_tree);
}

/*
 * read a key value pair and add it to tree
 */
static void read_key_value(unsigned int *offset, tvbuff_t * tvb,
                          proto_tree * etch_tree)
{
  proto_tree *new_tree;
  proto_tree *new_tree_bck;
  proto_item *ti, *parent_ti;

  gbl_have_symbol = FALSE;

  parent_ti =
    proto_tree_add_item(etch_tree, hf_etch_keyvalue, tvb, *offset, 1,
                        ENC_NA);
  new_tree_bck = new_tree =
    proto_item_add_subtree(parent_ti, ett_etch_keyvalue);

  ti = proto_tree_add_item(new_tree, hf_etch_keyname, tvb, *offset, 0,
                           ENC_NA);
  new_tree = proto_item_add_subtree(ti, ett_etch_key);
  read_value(offset, tvb, new_tree, hf_etch_value);

  /* append the symbol of the key */
  if(gbl_have_symbol == TRUE){
    proto_item_append_text(parent_ti, " (");
    proto_item_append_text(parent_ti, "%s", gbl_symbol_buffer->str);
    proto_item_append_text(parent_ti, ")");
  }

  ti = proto_tree_add_item(new_tree_bck, hf_etch_valuename, tvb, *offset,
                           0, ENC_NA);
  new_tree = proto_item_add_subtree(ti, ett_etch_value);
  read_value(offset, tvb, new_tree, hf_etch_value);
}

/****************************************************************************************************/
/*
 * Preparse the message for the info column
 */
static emem_strbuf_t* get_column_info(tvbuff_t * tvb)
{
  int byte_length;
  guint8 type_code;
  emem_strbuf_t* result_buf;
  int my_offset = 0;

  /* We've a full PDU: 8 bytes + pdu_packetlen bytes  */
  result_buf = ep_strbuf_new_label("");

  my_offset += (4 + 4 + 1); /* skip Magic, Length, Version */

  type_code = tvb_get_guint8(tvb, my_offset);
  byte_length = get_byte_length(type_code);
  my_offset++;

  if (byte_length == 4) {
    const gchar* symbol;
    guint32 hash;
    hash   = tvb_get_ntohl(tvb, my_offset);
    symbol = val_to_str((const guint32) hash, gbl_symbols, "%x");
    if (symbol != NULL) {
      ep_strbuf_append_printf(result_buf, "%s()", symbol);
    }
  }

  return result_buf;
}


/****************************************************************************************************/
/*
 * main dissector function for an etch message
 */
static void dissect_etch_message(tvbuff_t * tvb, packet_info * pinfo,
                                 proto_tree * tree)
{
  /* We've a full PDU: 8 bytes + pdu_packetlen bytes  */
  emem_strbuf_t* colInfo = NULL;

  if (pinfo->cinfo || tree) {
    colInfo = get_column_info(tvb);    /* get current symbol */
  }

  if (pinfo->cinfo) {
    col_set_str(pinfo->cinfo, COL_PROTOCOL, "ETCH");
    gbl_pdu_counter++;

    /* Switch to another frame? => Clear column */
    if (pinfo->fd->num != gbl_old_frame_num) {
      col_clear(pinfo->cinfo, COL_INFO);
      gbl_pdu_counter = 0;
    }
    gbl_old_frame_num = pinfo->fd->num;

    col_set_writable(pinfo->cinfo, 1);
    col_append_fstr(pinfo->cinfo, COL_INFO, "%s ", colInfo->str);
  }

  if (tree) {
    /* we are being asked for details */
    unsigned int offset;
    proto_item *ti;
    proto_tree *etch_tree;

    ti = proto_tree_add_protocol_format(tree, proto_etch, tvb, 0, -1,
                                        "ETCH Protocol: %s", colInfo->str);

    offset = 9;
    etch_tree = proto_item_add_subtree(ti, ett_etch);
    proto_tree_add_item(etch_tree, hf_etch_sig, tvb, 0, 4, ENC_BIG_ENDIAN);
    proto_tree_add_item(etch_tree, hf_etch_length, tvb, 4, 4, ENC_BIG_ENDIAN);
    proto_tree_add_item(etch_tree, hf_etch_version, tvb, 8, 1, ENC_NA);
    read_struct(&offset, tvb, etch_tree, 0);
  }

}

/*
 * determine PDU length of protocol etch
 */
static guint get_etch_message_len(packet_info * pinfo _U_, tvbuff_t * tvb,
                                  int offset)
{
  /* length is at offset 4. we add magic bytes length + length size */
  return tvb_get_ntohl(tvb, offset + 4) + 8;
}


/*
 * main dissector function for the etch protocol
 */
static int dissect_etch(tvbuff_t * tvb, packet_info * pinfo,
                             proto_tree * tree)
{
  if (tvb_length(tvb) < 4) {
    /* Too small for an etch packet. */
    return 0;
  }

  if (tvb_memeql(tvb, 0, etch_magic, 4) == -1) {
    /* Not an etch packet. */
    return 0;
  }

  tcp_dissect_pdus(tvb, pinfo, tree, TRUE, 8, get_etch_message_len,
                   dissect_etch_message);

  if (gbl_pdu_counter > 0) {
    col_set_writable(pinfo->cinfo, 1);
    col_prepend_fstr(pinfo->cinfo, COL_INFO, "[%d] ", gbl_pdu_counter + 1);
  }

  return 1;
}

static void etch_dissector_init(void) {
  gbl_pdu_counter   = 0;
  gbl_old_frame_num = 0xFFFFFFFF;
}

void proto_register_etch(void)
{
  module_t *etch_module;
  int i = 0;

  static hf_register_info hf[] = {
    {&hf_etch_sig,
     {"Etch Signature", "etch.signature",
      FT_UINT32, BASE_HEX,
      NULL, 0x0,
      NULL, HFILL}
    },
    {&hf_etch_length,
     {"Etch Length", "etch.msglength",
      FT_UINT64, BASE_DEC,
      NULL, 0x0,
      NULL, HFILL}
    },
    {&hf_etch_dim,
     {"Etch Dim", "etch.dim",
      FT_UINT8, BASE_DEC,
      NULL, 0x0,
      NULL, HFILL}
    },
    {&hf_etch_version,
     {"Etch Version", "etch.version",
      FT_UINT8, BASE_DEC,
      NULL, 0x0,
      NULL, HFILL}
    },
    {&hf_etch_typecode,
     {"Etch TypeCode", "etch.typecode",
      FT_STRING, BASE_NONE,    /* FT_INT8*/
      NULL, 0x0,
      NULL, HFILL}
    },
    {&hf_etch_value,
     {"Etch Value", "etch.value",
      FT_UINT64, BASE_DEC,
      NULL, 0x0,
      NULL, HFILL}
    },
    {&hf_etch_bytes,
     {"Etch Bytes", "etch.bytes",
      FT_BYTES, BASE_NONE,
      NULL, 0x0,
      NULL, HFILL}
    },
    {&hf_etch_byte,
     {"Etch Byte", "etch.byte",
      FT_INT8, BASE_DEC,
      NULL, 0x0,
      NULL, HFILL}
    },
    {&hf_etch_short,
     {"Etch Short", "etch.short",
      FT_INT16, BASE_DEC,
      NULL, 0x0,
      NULL, HFILL}
    },
    {&hf_etch_int,
     {"Etch Int", "etch.int",
      FT_INT32, BASE_DEC,
      NULL, 0x0,
      NULL, HFILL}
    },
    {&hf_etch_long,
     {"Etch Long", "etch.long",
      FT_INT64, BASE_DEC,
      NULL, 0x0,
      NULL, HFILL}
    },
    {&hf_etch_float,
     {"Etch Float", "etch.float",
      FT_FLOAT, BASE_NONE,
      NULL, 0x0,
      NULL, HFILL}
    },
    {&hf_etch_double,
     {"Etch Double", "etch.double",
      FT_DOUBLE, BASE_NONE,
      NULL, 0x0,
      NULL, HFILL}
    },
    {&hf_etch_keyvalue,
     {"Etch keyValue", "etch.keyvalue",
      FT_NONE, BASE_NONE,
      NULL, 0x0,
      NULL, HFILL}
    },
    {&hf_etch_key,
     {"Etch key", "etch.key",
      FT_BYTES, BASE_NONE,
      NULL, 0x0,
      NULL, HFILL}
    },
    {&hf_etch_symbol,
     {"Etch symbol", "etch.symbol",
      FT_UINT32, BASE_HEX,
      NULL, 0x0,
      NULL, HFILL}
    },
    {&hf_etch_struct,
     {"Etch Struct", "etch.struct",
      FT_BYTES, BASE_NONE,
      NULL, 0x0,
      NULL, HFILL}
    },
    {&hf_etch_string,
     {"Etch String", "etch.string",
      FT_STRING, BASE_NONE,
      NULL, 0x0,
      NULL, HFILL}
    },
    {&hf_etch_keyname,
     {"Etch key", "etch.keyname",
      FT_NONE, BASE_NONE,
      NULL, 0x0,
      NULL, HFILL}
    },
    {&hf_etch_valuename,
     {"Etch value", "etch.valuename",
      FT_NONE, BASE_NONE,
      NULL, 0x0,
      NULL, HFILL}
    },
  };

  /* Setup protocol subtree array */
  static gint *ett[] = {
    &ett_etch,
    &ett_etch_struct,
    &ett_etch_keyvalue,
    &ett_etch_key,
    &ett_etch_value,
  };

  gbl_symbols = (value_string*) g_malloc(ETCH_MAX_SYMBOLS * sizeof(value_string));

  for(i = 0; i < ETCH_MAX_SYMBOLS; i++){
    value_string tmp = {0, NULL};
    gbl_symbols[i] = tmp;
  }

  proto_etch = proto_register_protocol("Apache Etch Protocol",    /* name       */
                                       "ETCH",    /* short name */
                                       "etch"    /* abbrev     */
                                       );

  proto_register_field_array(proto_etch, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));
  new_register_dissector("etch", dissect_etch, proto_etch);

  register_init_routine(&etch_dissector_init);

  etch_module = prefs_register_protocol(proto_etch,
                                        proto_reg_handoff_etch);

  prefs_register_string_preference(etch_module, "file",
                                 "Apache Etch symbol folder",
                                 "Place the hash/symbol files (generated by the Apache Etch compiler) ending with .ewh here",
                                 &gbl_keytab_folder);
  prefs_register_uint_preference(etch_module, "tcp.port",
                                 "etch TCP Port",
                                 "Etch TCP port",
                                 10,
                                 &gbl_etch_port);
}

void proto_reg_handoff_etch(void)
{
  static gboolean etch_prefs_initialized = FALSE;
  static dissector_handle_t etch_handle;
  static guint old_etch_port = 0;

  /* create dissector handle only once */
  if(!etch_prefs_initialized) {
    etch_handle = new_create_dissector_handle(dissect_etch, proto_etch);
    /* add heuristic dissector for tcp */
    heur_dissector_add("tcp", dissect_etch, proto_etch);
    etch_prefs_initialized = TRUE;
  }

  if(old_etch_port != 0 && old_etch_port != gbl_etch_port){
    dissector_delete("tcp.port", old_etch_port, etch_handle);
  }

  if(gbl_etch_port != 0 && old_etch_port != gbl_etch_port) {
    dissector_add("tcp.port", gbl_etch_port, etch_handle);
  }

  old_etch_port = gbl_etch_port;

  /* read config folder files, if filename has changed */
  if(g_strcmp0(gbl_keytab_folder, gbl_current_keytab_folder) != 0){
    read_hashed_symbols_from_dir(gbl_keytab_folder);
  }
}
