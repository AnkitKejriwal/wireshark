/* print.c
 * Routines for printing packet analysis trees.
 *
 * $Id: print.c,v 1.18 1999/09/09 02:42:25 gram Exp $
 *
 * Gilbert Ramirez <gram@verdict.uthscsa.edu>
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@zing.org>
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#include "packet.h"
#include "prefs.h"
#include "print.h"
#include "ps.h"

static void proto_tree_print_node_text(GNode *node, gpointer data);
static void dumpit (FILE *fh, register const u_char *cp, register u_int length);
static void proto_tree_print_node_ps(GNode *node, gpointer data);
static void ps_clean_string(unsigned char *out, const unsigned char *in,
			int outbuf_size);
static void dumpit_ps (FILE *fh, register const u_char *cp, register u_int length);

extern int proto_data; /* in packet-data.c */


typedef struct {
	int		level;
	FILE		*fh;
	const guint8	*pd;
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

void print_preamble(FILE *fh)
{
	if (prefs.pr_format == PR_FMT_PS)
		print_ps_preamble(fh);
}

void print_finale(FILE *fh)
{
	if (prefs.pr_format == PR_FMT_PS)
		print_ps_finale(fh);
}

void proto_tree_print(int frame_num, GNode *protocol_tree,
    const u_char *pd, frame_data *fd, FILE *fh)
{
	print_data data;

	/* Create the output */
	data.level = 0;
	data.fh = fh;
	data.pd = pd;

	/* XXX - printing multiple frames in PostScript looks as if it's
	   tricky - you have to deal with page boundaries, I think -
	   and I'll have to spend some time learning enough about
	   PostScript to figure it out, so, for now, we only print
	   multiple frames as text. */
	if (prefs.pr_format == PR_FMT_TEXT || frame_num != -1) {
		if (frame_num != -1)
			fprintf(fh, "Frame %d:\n\n", frame_num);
		g_node_children_foreach((GNode*) protocol_tree, G_TRAVERSE_ALL,
			proto_tree_print_node_text, &data);
		if (frame_num != -1)
			fprintf(fh, "\n");
	} else {
		if (frame_num != -1) {
			fprintf(fh, "0 (Frame %d:) putline\n", frame_num);
			fprintf(fh, "0 () putline\n");
		}
		g_node_children_foreach((GNode*) protocol_tree, G_TRAVERSE_ALL,
			proto_tree_print_node_ps, &data);
		if (frame_num != -1)
			fprintf(fh, "0 () putline\n");
	}
}

/* Print a tree's data, and any child nodes, in plain text */
static
void proto_tree_print_node_text(GNode *node, gpointer data)
{
	field_info	*fi = (field_info*) (node->data);
	print_data	*pdata = (print_data*) data;
	int		i;
	int		 num_spaces;
	char		space[41];
	gchar		label_str[ITEM_LABEL_LENGTH];
	gchar		*label_ptr;

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
	if (num_spaces > 40) {
		num_spaces = 40;
	}
	for (i = 0; i < num_spaces; i++) {
		space[i] = ' ';
	}
	/* The string is NUL-terminated */
	space[num_spaces] = 0;

	/* Print the text */
	fprintf(pdata->fh, "%s%s\n", space, label_ptr);

	/* If it's uninterpreted data, dump it. */
	if (fi->hfinfo->id == proto_data)
		dumpit(pdata->fh, &pdata->pd[fi->start], fi->length);

	/* Recurse into the subtree, if it exists */
	if (g_node_n_children(node) > 0) {
		pdata->level++;
		g_node_children_foreach(node, G_TRAVERSE_ALL,
			proto_tree_print_node_text, pdata);
		pdata->level--;
	}
}

/* This routine was created by Dan Lasley <DLASLEY@PROMUS.com>, and
only slightly modified for ethereal by Gilbert Ramirez. */
static
void dumpit (FILE *fh, register const u_char *cp, register u_int length)
{
        register int ad, i, j, k;
        u_char c;
        u_char line[60];
		static u_char binhex[16] = {
			'0', '1', '2', '3', '4', '5', '6', '7',
			'8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

        memset (line, ' ', sizeof line);
        line[sizeof (line)-1] = 0;
        for (ad=i=j=k=0; i<length; i++) {
                c = *cp++;
                line[j++] = binhex[c>>4];
                line[j++] = binhex[c&0xf];
                if (i&1) j++;
                line[42+k++] = c >= ' ' && c < 0x7f ? c : '.';
                if ((i & 15) == 15) {
                        fprintf (fh, "\n%4x  %s", ad, line);
                        /*if (i==15) printf (" %d", length);*/
                        memset (line, ' ', sizeof line);
                        line[sizeof (line)-1] = j = k = 0;
                        ad += 16;
                }
        }

        if (line[0] != ' ') fprintf (fh, "\n%4x  %s", ad, line);
        fprintf(fh, "\n");
        return;

}

#define MAX_LINE_LENGTH 256

/* Print a node's data, and any child nodes, in PostScript */
static
void proto_tree_print_node_ps(GNode *node, gpointer data)
{
	field_info	*fi = (field_info*) (node->data);
	print_data	*pdata = (print_data*) data;
	gchar		label_str[ITEM_LABEL_LENGTH];
	gchar		*label_ptr;
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

	/* If it's uninterpreted data, dump it. */
	if (fi->hfinfo->id == proto_data) {
		print_ps_hex(pdata->fh);
		dumpit_ps(pdata->fh, &pdata->pd[fi->start], fi->length);
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

static
void dumpit_ps (FILE *fh, register const u_char *cp, register u_int length)
{
        register int ad, i, j, k;
        u_char c;
        u_char line[60];
		static u_char binhex[16] = {
			'0', '1', '2', '3', '4', '5', '6', '7',
			'8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
		u_char psline[MAX_LINE_LENGTH];

        memset (line, ' ', sizeof line);
        line[sizeof (line)-1] = 0;
        for (ad=i=j=k=0; i<length; i++) {
                c = *cp++;
                line[j++] = binhex[c>>4];
                line[j++] = binhex[c&0xf];
                if (i&1) j++;
                line[42+k++] = c >= ' ' && c < 0x7f ? c : '.';
                if ((i & 15) == 15) {
						ps_clean_string(psline, line, MAX_LINE_LENGTH);
                        fprintf (fh, "(%4x  %s) hexdump\n", ad, psline);
                        memset (line, ' ', sizeof line);
                        line[sizeof (line)-1] = j = k = 0;
                        ad += 16;
                }
        }

        if (line[0] != ' ') {
			ps_clean_string(psline, line, MAX_LINE_LENGTH);
			fprintf (fh, "(%4x  %s) hexdump\n", ad, psline);
		}
        return;

}
