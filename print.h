/* print.h
 * Definitions for printing packet analysis trees.
 *
 * $Id$
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

#ifndef __PRINT_H__
#define __PRINT_H__

#include <epan/packet.h>

/* print output format */
typedef enum {
  PR_FMT_TEXT,    /* plain text */
  PR_FMT_PS       /* postscript */
} print_format_e;

/* print_range, enum which frames should be printed */
typedef enum {
  print_range_selected_only,    /* selected frame(s) only (currently only one) */
  print_range_marked_only,      /* marked frames only */
  print_range_all_displayed,    /* all frames currently displayed */
  print_range_all_captured      /* all frames in capture */
} print_range_e;

/* print_dissections, enum how the dissections should be printed */
typedef enum {
  print_dissections_none,         /* no dissections at all */
  print_dissections_collapsed,    /* no dissection details */
  print_dissections_as_displayed, /* details as displayed */
  print_dissections_expanded      /* all dissection details */
} print_dissections_e;

typedef struct {
  print_format_e format;	/* plain text or PostScript */
  gboolean	to_file;	/* TRUE if we're printing to a file */
  char		*file;		/* file output pathname */
  char		*cmd;		/* print command string (not win32) */
  packet_range_t range;

  gboolean	print_summary;	/* TRUE if we should just print summary;
				   FALSE if we should print protocol tree. */
  print_dissections_e   print_dissections;
  gboolean	print_hex;	/* TRUE if we should also print hex data;
				   FALSE if we should print only if not dissected. */
  gboolean	print_formfeed; /* TRUE if a formfeed should be printed 
                   before each new packet */
} print_args_t;

/* Functions in print.h */

extern void write_pdml_preamble(FILE *fh);
extern void proto_tree_write_pdml(epan_dissect_t *edt, FILE *fh);
extern void write_pdml_finale(FILE *fh);

extern void write_psml_preamble(FILE *fh);
extern void proto_tree_write_psml(epan_dissect_t *edt, FILE *fh);
extern void write_psml_finale(FILE *fh);

struct print_stream;

typedef struct print_stream_ops {
	gboolean (*print_preamble)(struct print_stream *self, gchar *filename);
	gboolean (*print_line)(struct print_stream *self, int indent,
	    const char *line);
	gboolean (*print_bookmark)(struct print_stream *self,
	    const gchar *name, const gchar *title);
	gboolean (*new_page)(struct print_stream *self);
	gboolean (*print_finale)(struct print_stream *self);
	gboolean (*destroy)(struct print_stream *self);
} print_stream_ops_t;

typedef struct print_stream {
	const print_stream_ops_t *ops;
	void *data;
} print_stream_t;

extern gboolean proto_tree_print(print_args_t *print_args, epan_dissect_t *edt,
     print_stream_t *stream);
extern gboolean print_hex_data(print_stream_t *stream, epan_dissect_t *edt);

extern print_stream_t *print_stream_text_new(int to_file, const char *dest);
extern print_stream_t *print_stream_text_stdio_new(FILE *fh);
extern print_stream_t *print_stream_ps_new(int to_file, const char *dest);
extern print_stream_t *print_stream_ps_stdio_new(FILE *fh);

extern gboolean print_preamble(print_stream_t *self, gchar *filename);
extern gboolean print_line(print_stream_t *self, int indent, const char *line);
extern gboolean print_bookmark(print_stream_t *self, const gchar *name,
    const gchar *title);
extern gboolean new_page(print_stream_t *self);
extern gboolean print_finale(print_stream_t *self);
extern gboolean destroy_print_stream(print_stream_t *self);

#endif /* print.h */
