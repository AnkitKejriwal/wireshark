/* proto_draw.c
 * Routines for GTK+ packet display
 *
 * $Id: proto_draw.c,v 1.28 2001/03/07 19:33:24 gram Exp $
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

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#include <ctype.h>
#include <stdarg.h>

#include <gtk/gtk.h>

#ifdef NEED_SNPRINTF_H
# include "snprintf.h"
#endif

#include <stdio.h>
#include "main.h"
#include "packet.h"
#include "util.h"
#include "menu.h"
#include "keys.h"

#include "colors.h"
#include "prefs.h"
#include "proto_draw.h"
#include "packet_win.h"
#include "gtkglobals.h"

#define BYTE_VIEW_WIDTH    16
#define BYTE_VIEW_SEP      8

static void
proto_tree_draw_node(GNode *node, gpointer data);

/* Redraw a given byte view window. */
void
redraw_hex_dump(GtkWidget *bv, guint8 *pd, frame_data *fd, field_info *finfo)
{
  packet_hex_print(GTK_TEXT(bv), pd, fd, finfo);
}

void
redraw_hex_dump_all(void)
{
  if (cfile.current_frame != NULL)
    redraw_hex_dump(byte_view, cfile.pd, cfile.current_frame, finfo_selected);
  redraw_hex_dump_packet_wins();
}

	
static void
expand_tree(GtkCTree *ctree, GtkCTreeNode *node, gpointer user_data)
{
	field_info	*finfo;
	gboolean	*val;

	finfo = gtk_ctree_node_get_row_data( ctree, node );
	g_assert(finfo);

	val = &tree_is_expanded[finfo->tree_type];
	*val = TRUE;
}

static void
collapse_tree(GtkCTree *ctree, GtkCTreeNode *node, gpointer user_data)
{
	field_info	*finfo;
	gboolean	*val;

	finfo = gtk_ctree_node_get_row_data( ctree, node );
	g_assert(finfo);

	val = &tree_is_expanded[finfo->tree_type];
	*val = FALSE;
}

/* Which byte the offset is referring to. Associates
 * whitespace with the preceding digits. */
static int
byte_num(int offset, int start_point)
{
	return (offset - start_point) / 3;
}


/* If the user selected a certain byte in the byte view, try to find
 * the item in the GUI proto_tree that corresponds to that byte, and
 * select it. */
static gint
byte_view_select(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	GtkCTree	*ctree = GTK_CTREE(tree_view);
	GtkCTreeNode	*node, *parent;
	field_info	*finfo;
	GtkText		*bv = GTK_TEXT(widget);
	int		row, column;
	int		byte;

	/* The column of the first hex digit in the first half */
	const int	digits_start_1 = 6;
	/* The column of the last hex digit in the first half. */
	const int	digits_end_1 = 28;

	/* The column of the first hex digit in the second half */
	const int	digits_start_2 = 31;
	/* The column of the last hex digit in the second half. */
	const int	digits_end_2 = 53;

	/* The column of the first "text dump" character in first half. */
	const int	text_start_1 = 57;
	/* The column of the last "text dump" character in first half. */
	const int	text_end_1 = 64;

	/* The column of the first "text dump" character in second half. */
	const int	text_start_2 = 66;
	/* The column of the last "text dump" character in second half. */
	const int	text_end_2 = 73;

	/* Given the mouse (x,y) and the current GtkText (h,v)
	 * adjustments, and the size of the font, figure out
	 * which text column/row the user selected. This could be off
	 * if the bold version of the font is bigger than the
	 * regular version of the font. */
	column = (bv->hadj->value + event->x) / m_font_width;
	row = (bv->vadj->value + event->y) / m_font_height;

	/* Given the column and row, determine which byte offset
	 * the user clicked on. */
	if (column >= digits_start_1 && column <= digits_end_1) {
		byte = byte_num(column, digits_start_1);
		if (byte == -1) {
			return FALSE;
		}
	}
	else if (column >= digits_start_2 && column <= digits_end_2) {
		byte = byte_num(column, digits_start_2);
		if (byte == -1) {
			return FALSE;
		}
		byte += 8;
	}
	else if (column >= text_start_1 && column <= text_end_1) {
		byte = column - text_start_1;
	}
	else if (column >= text_start_2 && column <= text_end_2) {
		byte = 8 + column - text_start_2;
	}
	else {
		/* The user didn't select a hex digit or
		 * text-dump character. */
		return FALSE;
	}

	/* Add the number of bytes from the previous rows. */
	byte += row * 16;


	/* Find the finfo that corresponds to our byte. */
	finfo = proto_find_field_from_offset(cfile.protocol_tree, byte);

	if (!finfo) {
		return FALSE;
	}

	node = gtk_ctree_find_by_row_data(ctree, NULL, finfo);
	g_assert(node);

	/* Expand and select our field's row */
	gtk_ctree_expand(ctree, node);
	gtk_ctree_select(ctree, node);
	expand_tree(ctree, node, NULL);

	/* ... and its parents */
	parent = GTK_CTREE_ROW(node)->parent;
	while (parent) {
		gtk_ctree_expand(ctree, parent);
		expand_tree(ctree, parent, NULL);
		parent = GTK_CTREE_ROW(parent)->parent;
	}

	/* And position the window so the selection is visible.
	 * Position the selection in the middle of the viewable
	 * pane. */
	gtk_ctree_node_moveto(ctree, node, 0, .5, 0);

	return FALSE;
}

/* Calls functions for different mouse-button presses. */
static gint
byte_view_button_press_cb(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	GdkEventButton *event_button = NULL;

	if(widget == NULL || event == NULL || data == NULL) {
		return FALSE;
	}
	
	if(event->type == GDK_BUTTON_PRESS) {
		event_button = (GdkEventButton *) event;

		switch(event_button->button) {

		case 1:
			return byte_view_select(widget, event_button, data);
		case 3:
			return popup_menu_handler(widget, event, data);
		default:
			return FALSE;
		}
	}

	return FALSE;
}
void
create_byte_view(gint bv_size, GtkWidget *pane, GtkWidget **byte_view_p,
		GtkWidget **bv_scrollw_p, int pos)
{
  GtkWidget *byte_view, *byte_scrollw;

  /* Byte view.  Create a scrolled window for the text. */
  byte_scrollw = gtk_scrolled_window_new(NULL, NULL);
  gtk_paned_pack2(GTK_PANED(pane), byte_scrollw, FALSE, FALSE);
  gtk_widget_set_usize(byte_scrollw, -1, bv_size);

  /* The horizontal scrollbar of the scroll-window doesn't seem
   * to affect the GtkText widget at all, even when line wrapping
   * is turned off in the GtkText widget and there is indeed more
   * horizontal data. */
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(byte_scrollw),
			/* Horizontal */GTK_POLICY_NEVER,
			/* Vertical*/	GTK_POLICY_ALWAYS);
  set_scrollbar_placement_scrollw(byte_scrollw, pos);
  remember_scrolled_window(byte_scrollw);
  gtk_widget_show(byte_scrollw);

  byte_view = gtk_text_new(NULL, NULL);
  gtk_text_set_editable(GTK_TEXT(byte_view), FALSE);
  gtk_text_set_word_wrap(GTK_TEXT(byte_view), FALSE);
  gtk_text_set_line_wrap(GTK_TEXT(byte_view), FALSE);
  gtk_container_add(GTK_CONTAINER(byte_scrollw), byte_view);
  gtk_widget_show(byte_view);
  gtk_signal_connect(GTK_OBJECT(byte_view), "button_press_event",
		     GTK_SIGNAL_FUNC(byte_view_button_press_cb),
		     gtk_object_get_data(GTK_OBJECT(popup_menu_object), PM_HEXDUMP_KEY));


  *byte_view_p = byte_view;
  *bv_scrollw_p = byte_scrollw;
}

void
packet_hex_print(GtkText *bv, guint8 *pd, frame_data *fd, field_info *finfo)
{
  gint     i = 0, j, k, cur;
  guchar   line[128], hexchars[] = "0123456789abcdef", c = '\0';
  GdkFont *cur_font, *new_font;
  gint     bstart, blen;
  gint	   bend = -1;
  GdkColor *fg, *bg;
  gboolean reverse, newreverse;

  if (finfo != NULL) {
    bstart = finfo->start;
    blen = finfo->length;
  } else {
    bstart = -1;
    blen = -1;
  }

  /* Freeze the text for faster display */
  gtk_text_freeze(bv);

  /* Clear out the text */
  gtk_text_set_point(bv, 0);
  /* Keep GTK+ 1.2.3 through 1.2.6 from dumping core - see 
     http://www.ethereal.com/lists/ethereal-dev/199912/msg00312.html and
     http://www.gnome.org/mailing-lists/archives/gtk-devel-list/1999-October/0051.shtml
     for more information */
  gtk_adjustment_set_value(bv->vadj, 0.0);
  gtk_text_forward_delete(bv, gtk_text_get_length(bv));

  if (bstart >= 0 && blen >= 0) {
	  bend = bstart + blen;
  }

  while (i < fd->cap_len) {
    /* Print the line number */
    sprintf(line, "%04x  ", i);
    
    /* Display with inverse video ? */
    if (prefs.gui_hex_dump_highlight_style) {
      gtk_text_insert(bv, m_r_font, &BLACK, &WHITE, line, -1);
      /* Do we start in reverse? */
      reverse = i >= bstart && i < bend;
      fg = reverse ? &WHITE : &BLACK;
      bg = reverse ? &BLACK : &WHITE;
      j   = i;
      k   = i + BYTE_VIEW_WIDTH;
      cur = 0;
      /* Print the hex bit */
      while (i < k) {
	if (i < fd->cap_len) {
	  line[cur++] = hexchars[(pd[i] & 0xf0) >> 4];
	  line[cur++] = hexchars[pd[i] & 0x0f];
	} else {
	  line[cur++] = ' '; line[cur++] = ' ';
	}
	i++;
	newreverse = i >= bstart && i < bend;
	/* Have we gone from reverse to plain? */
	if (reverse && (reverse != newreverse)) {
	  gtk_text_insert(bv, m_r_font, fg, bg, line, cur);
	  fg = &BLACK;
	  bg = &WHITE;
	  cur = 0;
	}
	/* Inter byte space if not at end of line */
	if (i < k) {
	  line[cur++] = ' ';
	  /* insert a space every BYTE_VIEW_SEP bytes */
	  if( ( i % BYTE_VIEW_SEP ) == 0 ) {
	    line[cur++] = ' ';
	  }
	}
	/* Have we gone from plain to reversed? */
	if (!reverse && (reverse != newreverse)) {
	  gtk_text_insert(bv, m_r_font, fg, bg, line, cur);
	  fg = &WHITE;
	  bg = &BLACK;
	  cur = 0;
	}
	reverse = newreverse;
      }
      /* Print remaining part of line */
      gtk_text_insert(bv, m_r_font, fg, bg, line, cur);
      cur = 0;
      /* Print some space at the end of the line */
      line[cur++] = ' '; line[cur++] = ' '; line[cur++] = ' ';
      gtk_text_insert(bv, m_r_font, &BLACK, &WHITE, line, cur);
      cur = 0;

      /* Print the ASCII bit */
      i = j;
      /* Do we start in reverse? */
      reverse = i >= bstart && i < bend;
      fg = reverse ? &WHITE : &BLACK;
      bg = reverse ? &BLACK : &WHITE;
      while (i < k) {
	if (i < fd->cap_len) {
	  if (fd->flags.encoding == CHAR_ASCII) {
	    c = pd[i];
	  }
	  else if (fd->flags.encoding == CHAR_EBCDIC) {
	    c = EBCDIC_to_ASCII1(pd[i]);
	  }
	  else {
		  g_assert_not_reached();
	  }
	  line[cur++] = (isprint(c)) ? c : '.';
	} else {
	  line[cur++] = ' ';
	}
	i++;
	newreverse = i >= bstart && i < bend;
	/* Have we gone from reverse to plain? */
	if (reverse && (reverse != newreverse)) {
	  gtk_text_insert(bv, m_r_font, fg, bg, line, cur);
	  fg = &BLACK;
	  bg = &WHITE;
	  cur = 0;
	}
	if (i < k) {
	  /* insert a space every BYTE_VIEW_SEP bytes */
	  if( ( i % BYTE_VIEW_SEP ) == 0 ) {
	    line[cur++] = ' ';
	  }
	}
	/* Have we gone from plain to reversed? */
	if (!reverse && (reverse != newreverse)) {
	  gtk_text_insert(bv, m_r_font, fg, bg, line, cur);
	  fg = &WHITE;
	  bg = &BLACK;
	  cur = 0;
	}
	reverse = newreverse;
      }
      /* Print remaining part of line */
      gtk_text_insert(bv, m_r_font, fg, bg, line, cur);
      cur = 0;
      line[cur++] = '\n';
      line[cur]   = '\0';
      gtk_text_insert(bv, m_r_font, &BLACK, &WHITE, line, -1);
    }
    else {
      gtk_text_insert(bv, m_r_font, NULL, NULL, line, -1);
      /* Do we start in bold? */
      cur_font = (i >= bstart && i < bend) ? m_b_font : m_r_font;
      j   = i;
      k   = i + BYTE_VIEW_WIDTH;
      cur = 0;
      /* Print the hex bit */
      while (i < k) {
	if (i < fd->cap_len) {
	  line[cur++] = hexchars[(pd[i] & 0xf0) >> 4];
	  line[cur++] = hexchars[pd[i] & 0x0f];
	} else {
	  line[cur++] = ' '; line[cur++] = ' ';
	}
	line[cur++] = ' ';
	i++;
	/* insert a space every BYTE_VIEW_SEP bytes */
	if( ( i % BYTE_VIEW_SEP ) == 0 ) line[cur++] = ' ';
	/* Did we cross a bold/plain boundary? */
	new_font = (i >= bstart && i < bend) ? m_b_font : m_r_font;
	if (cur_font != new_font) {
	  gtk_text_insert(bv, cur_font, NULL, NULL, line, cur);
	  cur_font = new_font;
	  cur = 0;
	}
      }
      line[cur++] = ' ';
      gtk_text_insert(bv, cur_font, NULL, NULL, line, cur);

      cur = 0;
      i = j;
      /* Print the ASCII bit */
      cur_font = (i >= bstart && i < bend) ? m_b_font : m_r_font;
      while (i < k) {
	if (i < fd->cap_len) {
	  if (fd->flags.encoding == CHAR_ASCII) {
	    c = pd[i];
	  }
	  else if (fd->flags.encoding == CHAR_EBCDIC) {
	    c = EBCDIC_to_ASCII1(pd[i]);
	  }
	  else {
		  g_assert_not_reached();
	  }
	  line[cur++] = (isprint(c)) ? c : '.';
	} else {
	  line[cur++] = ' ';
	}
	i++;
	/* insert a space every BYTE_VIEW_SEP bytes */
	if( ( i % BYTE_VIEW_SEP ) == 0 ) line[cur++] = ' ';
	/* Did we cross a bold/plain boundary? */
	new_font = (i >= bstart && i < bend) ? m_b_font : m_r_font;
	if (cur_font != new_font) {
	  gtk_text_insert(bv, cur_font, NULL, NULL, line, cur);
	  cur_font = new_font;
	  cur = 0;
	}
      }
      line[cur++] = '\n';
      line[cur]   = '\0';
      gtk_text_insert(bv, cur_font, NULL, NULL, line, -1);
    }
  }

  /* scroll text into position */
  gtk_text_thaw(bv); /* must thaw before adjusting scroll bars */
  if ( bstart > 0 ) {
    int linenum;
    float scrollval;

    linenum = bstart / BYTE_VIEW_WIDTH;
    scrollval = MIN(linenum * m_font_height,
		    bv->vadj->upper - bv->vadj->page_size);

    gtk_adjustment_set_value(bv->vadj, scrollval);
  }
}

/* List of all protocol tree widgets, so we can globally set the selection
   mode, line style, expander style, and font of all of them. */
static GList *ptree_widgets;

/* Add a protocol tree widget to the list of protocol tree widgets. */
static void forget_ptree_widget(GtkWidget *ptreew, gpointer data);

static void
remember_ptree_widget(GtkWidget *ptreew)
{
  ptree_widgets = g_list_append(ptree_widgets, ptreew);

  /* Catch the "destroy" event on the widget, so that we remove it from
     the list when it's destroyed. */
  gtk_signal_connect(GTK_OBJECT(ptreew), "destroy",
		     GTK_SIGNAL_FUNC(forget_ptree_widget), NULL);
}

/* Remove a protocol tree widget from the list of protocol tree widgets. */
static void
forget_ptree_widget(GtkWidget *ptreew, gpointer data)
{
  ptree_widgets = g_list_remove(ptree_widgets, ptreew);
}

/* Set the selection mode of a given packet tree window. */
static void
set_ptree_sel_browse(GtkWidget *ptreew, gboolean val)
{
	/* Yeah, GTK uses "browse" in the case where we do not, but oh well.
	   I think "browse" in Ethereal makes more sense than "SINGLE" in
	   GTK+ */
	if (val) {
		gtk_clist_set_selection_mode(GTK_CLIST(ptreew),
		    GTK_SELECTION_SINGLE);
	}
	else {
		gtk_clist_set_selection_mode(GTK_CLIST(ptreew),
		    GTK_SELECTION_BROWSE);
	}
}

static void
set_ptree_sel_browse_cb(gpointer data, gpointer user_data)
{
	set_ptree_sel_browse((GtkWidget *)data, *(gboolean *)user_data);
}

/* Set the selection mode of all packet tree windows. */
void
set_ptree_sel_browse_all(gboolean val)
{
	g_list_foreach(ptree_widgets, set_ptree_sel_browse_cb, &val);
}

/* Set the line style of a given packet tree window. */
static void
set_ptree_line_style(GtkWidget *ptreew, gint style)
{
	/* I'm using an assert here since the preferences code limits
	 * the user input, both in the GUI and when reading the preferences file.
	 * If the value is incorrect, it's a program error, not a user-initiated error.
	 */
	g_assert(style >= GTK_CTREE_LINES_NONE && style <= GTK_CTREE_LINES_TABBED);
	gtk_ctree_set_line_style(GTK_CTREE(ptreew), style);
}

static void
set_ptree_line_style_cb(gpointer data, gpointer user_data)
{
	set_ptree_line_style((GtkWidget *)data, *(gint *)user_data);
}

/* Set the line style of all packet tree window. */
void
set_ptree_line_style_all(gint style)
{
	g_list_foreach(ptree_widgets, set_ptree_line_style_cb, &style);
}

/* Set the expander style of a given packet tree window. */
static void
set_ptree_expander_style(GtkWidget *ptreew, gint style)
{
	/* I'm using an assert here since the preferences code limits
	 * the user input, both in the GUI and when reading the preferences file.
	 * If the value is incorrect, it's a program error, not a user-initiated error.
	 */
	g_assert(style >= GTK_CTREE_EXPANDER_NONE && style <= GTK_CTREE_EXPANDER_CIRCULAR);
	gtk_ctree_set_expander_style(GTK_CTREE(ptreew), style);
}

static void
set_ptree_expander_style_cb(gpointer data, gpointer user_data)
{
	set_ptree_expander_style((GtkWidget *)data, *(gint *)user_data);
}
	
void
set_ptree_expander_style_all(gint style)
{
	g_list_foreach(ptree_widgets, set_ptree_expander_style_cb, &style);
}

static void
set_ptree_style_cb(gpointer data, gpointer user_data)
{
	gtk_widget_set_style((GtkWidget *)data, (GtkStyle *)user_data);
}
	
void
set_ptree_font_all(GdkFont *font)
{
	GtkStyle *style;

	style = gtk_style_new();
	gdk_font_unref(style->font);
	style->font = font;
	gdk_font_ref(font);

	g_list_foreach(ptree_widgets, set_ptree_style_cb, style);

	/* Now nuke the old style and replace it with the new one. */
	gtk_style_unref(item_style);
	item_style = style;
}

void
create_tree_view(gint tv_size, e_prefs *prefs, GtkWidget *pane,
		GtkWidget **tv_scrollw_p, GtkWidget **tree_view_p, int pos)
{
  GtkWidget *tv_scrollw, *tree_view;

  /* Tree view */
  tv_scrollw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(tv_scrollw),
    GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
  set_scrollbar_placement_scrollw(tv_scrollw, pos);
  remember_scrolled_window(tv_scrollw);
  gtk_paned_pack1(GTK_PANED(pane), tv_scrollw, TRUE, TRUE);
  gtk_widget_set_usize(tv_scrollw, -1, tv_size);
  gtk_widget_show(tv_scrollw);
  
  tree_view = gtk_ctree_new(1, 0);
  /* I need this next line to make the widget work correctly with hidden
   * column titles and GTK_SELECTION_BROWSE */
  gtk_clist_set_column_auto_resize( GTK_CLIST(tree_view), 0, TRUE );
  gtk_container_add( GTK_CONTAINER(tv_scrollw), tree_view );
  set_ptree_sel_browse(tree_view, prefs->gui_ptree_sel_browse);
  set_ptree_line_style(tree_view, prefs->gui_ptree_line_style);
  set_ptree_expander_style(tree_view, prefs->gui_ptree_expander_style);
  gtk_widget_set_style(tree_view, item_style);
  remember_ptree_widget(tree_view);

  *tree_view_p = tree_view;
  *tv_scrollw_p = tv_scrollw;
}

void expand_all_tree(proto_tree *protocol_tree, GtkWidget *tree_view) {
  int i;
  for(i=0; i < num_tree_types; i++) {
    tree_is_expanded[i] = TRUE;
  }
  gtk_clist_clear ( GTK_CLIST(tree_view) );
  proto_tree_draw(protocol_tree, tree_view);
  gtk_ctree_expand_recursive(GTK_CTREE(tree_view), NULL);
}

void collapse_all_tree(proto_tree *protocol_tree, GtkWidget *tree_view) {
  int i;
  for(i=0; i < num_tree_types; i++) {
    tree_is_expanded[i] = FALSE;
  }
  gtk_clist_clear ( GTK_CLIST(tree_view) );
  proto_tree_draw(protocol_tree, tree_view);
}


struct proto_tree_draw_info {
	GtkCTree	*ctree;
	GtkCTreeNode	*ctree_node;
};

void
proto_tree_draw(proto_tree *protocol_tree, GtkWidget *tree_view)
{
	struct proto_tree_draw_info	info;

	info.ctree = GTK_CTREE(tree_view);
	info.ctree_node = NULL;

	gtk_clist_freeze ( GTK_CLIST(tree_view) );

	g_node_children_foreach((GNode*) protocol_tree, G_TRAVERSE_ALL,
		proto_tree_draw_node, &info);

	gtk_signal_connect( GTK_OBJECT(info.ctree), "tree-expand",
		(GtkSignalFunc) expand_tree, NULL );
	gtk_signal_connect( GTK_OBJECT(info.ctree), "tree-collapse",
		(GtkSignalFunc) collapse_tree, NULL );

	gtk_clist_thaw ( GTK_CLIST(tree_view) );
}

static void
proto_tree_draw_node(GNode *node, gpointer data)
{
	struct proto_tree_draw_info	info;
	struct proto_tree_draw_info	*parent_info = (struct proto_tree_draw_info*) data;

	field_info	*fi = (field_info*) (node->data);
	gchar		label_str[ITEM_LABEL_LENGTH];
	gchar		*label_ptr;
	GtkCTreeNode	*parent;
	gboolean	is_leaf, is_expanded;

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

	if (g_node_n_children(node) > 0) {
		is_leaf = FALSE;
		if (tree_is_expanded[fi->tree_type]) {
			is_expanded = TRUE;
		}
		else {
			is_expanded = FALSE;
		}
	}
	else {
		is_leaf = TRUE;
		is_expanded = FALSE;
	}
	
	info.ctree = parent_info->ctree;
	parent = gtk_ctree_insert_node ( info.ctree, parent_info->ctree_node, NULL,
			&label_ptr, 5, NULL, NULL, NULL, NULL,
			is_leaf, is_expanded );

	gtk_ctree_node_set_row_data( GTK_CTREE(info.ctree), parent, fi );

	if (!is_leaf) {
		info.ctree_node = parent;
		g_node_children_foreach(node, G_TRAVERSE_ALL,
			proto_tree_draw_node, &info);
	}
}
