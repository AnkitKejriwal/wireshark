/* print.c
 * Routines for printing packet analysis trees.
 *
 * $Id: print.c,v 1.50 2002/06/22 00:21:38 guy Exp $
 *
 * Gilbert Ramirez <gram@alumni.rice.edu>
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

#include <stdio.h>
#include <string.h>

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#include <epan/epan.h>
#include <epan/epan_dissect.h>
#include <epan/tvbuff.h>
#include <epan/packet.h>

#include "print.h"
#include "ps.h"
#include "util.h"
#include "packet-data.h"

static void proto_tree_print_node_text(GNode *node, gpointer data);
static void proto_tree_print_node_ps(GNode *node, gpointer data);
static void ps_clean_string(unsigned char *out, const unsigned char *in,
			int outbuf_size);
static void print_hex_data_text(FILE *fh, register const u_char *cp,
		register u_int length, char_enc encoding);
static void print_hex_data_ps(FILE *fh, register const u_char *cp,
		register u_int length, char_enc encoding);

typedef struct {
	int		level;
	FILE		*fh;
	GSList		*src_list;
	gboolean	print_all_levels;
	gboolean	print_hex_for_data;
	char_enc	encoding;
} print_data;

FILE *open_print_dest(int to_file, const char *dest)
{
	FILE	*fh;

	/* Open the file or command for output */
	if (to_file)
		fh = fopen(dest, "w");
	else
		fh = popen(dest, "w");

	return fh;
}

void close_print_dest(int to_file, FILE *fh)
{
	/* Close the file or command */
	if (to_file)
		fclose(fh);
	else
		pclose(fh);
}

void print_preamble(FILE *fh, gint format)
{
	if (format == PR_FMT_PS)
		print_ps_preamble(fh);
}

void print_finale(FILE *fh, gint format)
{
	if (format == PR_FMT_PS)
		print_ps_finale(fh);
}

void proto_tree_print(print_args_t *print_args, epan_dissect_t *edt,
    FILE *fh)
{
	print_data data;

	/* Create the output */
	data.level = 0;
	data.fh = fh;
	data.src_list = edt->pi.data_src;
	data.encoding = edt->pi.fd->flags.encoding;
	data.print_all_levels = print_args->expand_all;
	data.print_hex_for_data = !print_args->print_hex;
	    /* If we're printing the entire packet in hex, don't
	       print uninterpreted data fields in hex as well. */

	if (print_args->format == PR_FMT_TEXT) {
		g_node_children_foreach((GNode*) edt->tree, G_TRAVERSE_ALL,
			proto_tree_print_node_text, &data);
	} else {
		g_node_children_foreach((GNode*) edt->tree, G_TRAVERSE_ALL,
			proto_tree_print_node_ps, &data);
	}
}

/*
 * Find the data source for a specified field, and return a pointer
 * to the data in it.
 */
static const guint8 *
get_field_data(GSList *src_list, field_info *fi)
{
	GSList *src_le;
	data_source *src;
	tvbuff_t *src_tvb;

	for (src_le = src_list; src_le != NULL; src_le = src_le->next) {
		src = src_le->data;
		src_tvb = src->tvb;
		if (fi->ds_tvb == src_tvb) {
			/*
			 * Found it.
			 */
			return tvb_get_ptr(src_tvb, fi->start, fi->length);
		}
	}
	g_assert_not_reached();
	return NULL;	/* not found */
}

#define MAX_INDENT	160

/* Print a tree's data, and any child nodes, in plain text */
static
void proto_tree_print_node_text(GNode *node, gpointer data)
{
	field_info	*fi = PITEM_FINFO(node);
	print_data	*pdata = (print_data*) data;
	int		i;
	int		num_spaces;
	char		space[MAX_INDENT+1];
	const guint8	*pd;
	gchar		label_str[ITEM_LABEL_LENGTH];
	gchar		*label_ptr;

	/* Don't print invisible entries. */
	if (!fi->visible)
		return;

	/* was a free format label produced? */
	if (fi->representation) {
		label_ptr = fi->representation;
	}
	else { /* no, make a generic label */
		label_ptr = label_str;
		proto_item_fill_label(fi, label_str);
	}
		
	/* Prepare the tabs for printing, depending on tree level */
	num_spaces = pdata->level * 4;
	if (num_spaces > MAX_INDENT) {
		num_spaces = MAX_INDENT;
	}
	for (i = 0; i < num_spaces; i++) {
		space[i] = ' ';
	}
	/* The string is NUL-terminated */
	space[num_spaces] = '\0';

	/* Print the text */
	fprintf(pdata->fh, "%s%s\n", space, label_ptr);

	/* If it's uninterpreted data, dump it (unless our caller will
	   be printing the entire packet in hex). */
	if (fi->hfinfo->id == proto_data && pdata->print_hex_for_data) {
		/*
		 * Find the data for this field.
		 */
		pd = get_field_data(pdata->src_list, fi);
		print_hex_data_text(pdata->fh, pd, fi->length, pdata->encoding);
	}

	/* If we're printing all levels, or if this node is one with a
	   subtree and its subtree is expanded, recurse into the subtree,
	   if it exists. */
	g_assert(fi->tree_type >= -1 && fi->tree_type < num_tree_types);
	if (pdata->print_all_levels ||
	    (fi->tree_type >= 0 && tree_is_expanded[fi->tree_type])) {
		if (g_node_n_children(node) > 0) {
			pdata->level++;
			g_node_children_foreach(node, G_TRAVERSE_ALL,
				proto_tree_print_node_text, pdata);
			pdata->level--;
		}
	}
}

void print_hex_data(FILE *fh, gint format, epan_dissect_t *edt)
{
	gboolean multiple_sources;
	GSList *src_le;
	data_source *src;
	tvbuff_t *tvb;
	char *name;
	char *line;
	const u_char *cp;
	guint length;

	/*
	 * Set "multiple_sources" iff this frame has more than one
	 * data source; if it does, we need to print the name of
	 * the data source before printing the data from the
	 * data source.
	 */
	multiple_sources = (edt->pi.data_src->next != NULL);

	for (src_le = edt->pi.data_src; src_le != NULL;
	    src_le = src_le->next) {
		src = src_le->data;
		tvb = src->tvb;
		if (multiple_sources) {
			name = src->name;
			print_line(fh, format, "\n");
			line = g_malloc(strlen(name) + 3);	/* <name>:\n\0 */
			strcpy(line, name);
			strcat(line, ":\n");
			print_line(fh, format, line);
			g_free(line);
		}
		length = tvb_length(tvb);
		cp = tvb_get_ptr(tvb, 0, length);
		if (format == PR_FMT_PS) {
			print_hex_data_ps(fh, cp, length,
			    edt->pi.fd->flags.encoding);
		} else {
			print_hex_data_text(fh, cp, length,
			    edt->pi.fd->flags.encoding);
		}
	}
}

/*
 * This routine is based on a routine created by Dan Lasley
 * <DLASLEY@PROMUS.com>.
 *
 * It was modified for Ethereal by Gilbert Ramirez and others.
 */
static
void print_hex_data_common(FILE *fh, register const u_char *cp,
		register u_int length, char_enc encoding,
		void (*print_hex_data_start)(FILE *),
		void (*print_hex_data_line)(FILE *, u_char *))
{
	register unsigned int ad, i, j, k;
	u_char c;
	u_char line[6+50+16+1];
	static u_char binhex[16] = {
		'0', '1', '2', '3', '4', '5', '6', '7',
		'8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

	if (print_hex_data_start != NULL)
		(*print_hex_data_start)(fh);
	(*print_hex_data_line)(fh, "");
	ad = 0;
	j = 0;
	k = 0;
	sprintf(line, "%04x  ", ad);
	memset(line+6, ' ', sizeof line-6);
	line[sizeof (line)-1] = '\0';
	for (i=0; i<length; i++) {
		c = *cp++;
		line[6+j++] = binhex[c>>4];
		line[6+j++] = binhex[c&0xf];
		j++;
		if (encoding == CHAR_EBCDIC) {
			c = EBCDIC_to_ASCII1(c);
		}
		line[6+50+k++] = c >= ' ' && c < 0x7f ? c : '.';
		if ((i & 15) == 15) {
			(*print_hex_data_line)(fh, line);
			ad += 16;
			j = 0;
			k = 0;
			sprintf(line, "%04x  ", ad);
			memset(line+6, ' ', sizeof line-6);
			line[sizeof (line)-1] = '\0';
		}
	}

	if (line[0] != ' ')
		(*print_hex_data_line)(fh, line);
}

static void
print_hex_data_line_text(FILE *fh, u_char *line)
{
	fprintf(fh, "%s\n", line);
}

static
void print_hex_data_text(FILE *fh, register const u_char *cp,
		register u_int length, char_enc encoding)
{
	print_hex_data_common(fh, cp, length, encoding, NULL,
	    print_hex_data_line_text);
}

#define MAX_LINE_LENGTH 256

/* Print a node's data, and any child nodes, in PostScript */
static
void proto_tree_print_node_ps(GNode *node, gpointer data)
{
	field_info	*fi = PITEM_FINFO(node);
	print_data	*pdata = (print_data*) data;
	gchar		label_str[ITEM_LABEL_LENGTH];
	gchar		*label_ptr;
	const guint8	*pd;
	char		psbuffer[MAX_LINE_LENGTH]; /* static sized buffer! */

	if (!fi->visible)
		return;

	/* was a free format label produced? */
	if (fi->representation) {
		label_ptr = fi->representation;
	}
	else { /* no, make a generic label */
		label_ptr = label_str;
		proto_item_fill_label(fi, label_str);
	}
		
	/* Print the text */
	ps_clean_string(psbuffer, label_ptr, MAX_LINE_LENGTH);
	fprintf(pdata->fh, "%d (%s) putline\n", pdata->level, psbuffer);

	/* If it's uninterpreted data, dump it (unless our caller will
	   be printing the entire packet in hex). */
	if (fi->hfinfo->id == proto_data && pdata->print_hex_for_data) {
		/*
		 * Find the data for this field.
		 */
		pd = get_field_data(pdata->src_list, fi);
		print_hex_data_ps(pdata->fh, pd, fi->length, pdata->encoding);
	}

	/* Recurse into the subtree, if it exists */
	if (g_node_n_children(node) > 0) {
		pdata->level++;
		g_node_children_foreach(node, G_TRAVERSE_ALL,
			proto_tree_print_node_ps, pdata);
		pdata->level--;
	}
}

static
void ps_clean_string(unsigned char *out, const unsigned char *in,
			int outbuf_size)
{
	int rd, wr;
	char c;

	for (rd = 0, wr = 0 ; wr < outbuf_size; rd++, wr++ ) {
		c = in[rd];
		switch (c) {
			case '(':
			case ')':
			case '\\':
				out[wr] = '\\';
				out[++wr] = c;
				break;

			default:
				out[wr] = c;
				break;
		}

		if (c == 0) {
			break;
		}
	}
}

static void
print_hex_data_line_ps(FILE *fh, u_char *line)
{
	u_char psline[MAX_LINE_LENGTH];

	ps_clean_string(psline, line, MAX_LINE_LENGTH);
	fprintf(fh, "(%s) hexdump\n", psline);
}

static
void print_hex_data_ps(FILE *fh, register const u_char *cp,
		register u_int length, char_enc encoding)
{
	print_hex_data_common(fh, cp, length, encoding,
	    print_ps_hex, print_hex_data_line_ps);
}

void print_line(FILE *fh, gint format, char *line)
{
	char		psbuffer[MAX_LINE_LENGTH]; /* static sized buffer! */

	if (format == PR_FMT_PS) {
		ps_clean_string(psbuffer, line, MAX_LINE_LENGTH);
		fprintf(fh, "(%s) hexdump\n", psbuffer);
	} else
		fputs(line, fh);
}
