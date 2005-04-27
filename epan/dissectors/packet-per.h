/* packet-per.h
 * Routines for dissection of ASN.1 Aligned PER
 * 2003  Ronnie Sahlberg
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

#ifndef __PACKET_PER_H__
#define __PACKET_PER_H__

#define PER_NOT_DECODED_YET(x) \
proto_tree_add_text(tree, tvb, 0, 0, "something unknown here [%s]",x); \
fprintf(stderr,"Not decoded yet in packet : %d  [%s]\n", pinfo->fd->num,x); \
if (check_col(pinfo->cinfo, COL_INFO)){ \
	col_append_fstr(pinfo->cinfo, COL_INFO, "[UNKNOWN PER: %s]", x); \
} \
tvb_get_guint8(tvb, 9999);

/* in all functions here, offset is guint32 and is
   byteposition<<3 + bitposition
*/

/* values for extensions */
#define ASN1_NO_EXTENSIONS	0
#define ASN1_EXTENSION_ROOT	1
#define ASN1_NOT_EXTENSION_ROOT	2

/* value for optional */
#define ASN1_NOT_OPTIONAL	0
#define ASN1_OPTIONAL		1

typedef struct _per_choice_t {
	int value;
	char *name;
	int extension;
	int (*func)(tvbuff_t *, int, packet_info *, proto_tree *);
} per_choice_t;

typedef struct _per_sequence_t {
	char *name;
	int extension;
	int optional;
	int (*func)(tvbuff_t *, int, packet_info *, proto_tree *);
} per_sequence_t;


extern guint32 dissect_per_length_determinant(tvbuff_t *tvb, guint32 offset, packet_info *pinfo, proto_tree *tree, int hf_index, guint32 *length);

extern guint32 dissect_per_null(tvbuff_t *tvb, guint32 offset, packet_info *pinfo, proto_tree *tree, int hf_index);

extern guint32 dissect_per_GeneralString(tvbuff_t *tvb, guint32 offset, packet_info *pinfo, proto_tree *tree, int hf_index);

extern guint32 dissect_per_sequence_of(tvbuff_t *tvb, guint32 offset, packet_info *pinfo, proto_tree *parent_tree, int hf_index, gint ett_index, int (*func)(tvbuff_t *, int , packet_info *, proto_tree *));

extern guint32 dissect_per_IA5String(tvbuff_t *tvb, guint32 offset, packet_info *pinfo, proto_tree *tree, int hf_index, int min_len, int max_len);

extern guint32 dissect_per_NumericString(tvbuff_t *tvb, guint32 offset, packet_info *pinfo, proto_tree *tree, int hf_index, int min_len, int max_len);

extern guint32 dissect_per_PrintableString(tvbuff_t *tvb, guint32 offset, packet_info *pinfo, proto_tree *tree, int hf_index, int min_len, int max_len);

extern guint32 dissect_per_VisibleString(tvbuff_t *tvb, guint32 offset, packet_info *pinfo, proto_tree *tree, int hf_index, int min_len, int max_len);

extern guint32 dissect_per_BMPString(tvbuff_t *tvb, guint32 offset, packet_info *pinfo, proto_tree *tree, int hf_index, int min_len, int max_len);

extern guint32 dissect_per_constrained_sequence_of(tvbuff_t *tvb, guint32 offset, packet_info *pinfo, proto_tree *parent_tree, int hf_index, gint ett_index, int (*func)(tvbuff_t *, int , packet_info *, proto_tree *), int min_len, int max_len);

extern guint32 dissect_per_constrained_set_of(tvbuff_t *tvb, guint32 offset, packet_info *pinfo, proto_tree *parent_tree, int hf_index, gint ett_index, int (*func)(tvbuff_t *, int , packet_info *, proto_tree *), int min_len, int max_len);

extern guint32 dissect_per_set_of(tvbuff_t *tvb, guint32 offset, packet_info *pinfo, proto_tree *parent_tree, int hf_index, gint ett_index, int (*func)(tvbuff_t *, int , packet_info *, proto_tree *));

extern guint32 dissect_per_object_identifier(tvbuff_t *tvb, guint32 offset, packet_info *pinfo, proto_tree *tree, int hf_index, char *value_string);

extern guint32 dissect_per_boolean(tvbuff_t *tvb, guint32 offset, packet_info *pinfo, proto_tree *tree, int hf_index, gboolean *bool, proto_item **item);

extern guint32 dissect_per_integer(tvbuff_t *tvb, guint32 offset, packet_info *pinfo, proto_tree *tree, int hf_index, gint32 *value, proto_item **item);

extern guint32 dissect_per_constrained_integer(tvbuff_t *tvb, guint32 offset, packet_info *pinfo, proto_tree *tree, int hf_index, guint32 min, guint32 max, guint32 *value, proto_item **item, gboolean has_extension);

extern guint32 dissect_per_choice(tvbuff_t *tvb, guint32 offset, packet_info *pinfo, proto_tree *tree, int hf_index, gint ett_index, const per_choice_t *choice, char *name, guint32 *value);

extern guint32 dissect_per_sequence(tvbuff_t *tvb, guint32 offset, packet_info *pinfo, proto_tree *parent_tree, int hf_index, gint ett_index, const per_sequence_t *sequence);

extern guint32 dissect_per_octet_string(tvbuff_t *tvb, guint32 offset, packet_info *pinfo, proto_tree *tree, int hf_index, int min_len, int max_len, guint32 *value_offset, guint32 *value_len);

extern guint32 dissect_per_bit_string(tvbuff_t *tvb, guint32 offset, packet_info *pinfo, proto_tree *tree, int hf_index, int min_len, int max_len);

extern guint32 dissect_per_restricted_character_string(tvbuff_t *tvb, guint32 offset, packet_info *pinfo, proto_tree *tree, int hf_index, int min_len, int max_len, char *alphabet, int alphabet_length, char *info_str, guint32 info_str_len);

#endif  /* __PACKET_PER_H__ */
