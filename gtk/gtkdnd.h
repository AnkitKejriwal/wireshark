/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-1999.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#ifndef __GTK_DND_H__
#define __GTK_DND_H__

#include <gdk/gdk.h>
#include <gtk/gtkenums.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtkselection.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {
  GTK_DEST_DEFAULT_MOTION     = 1 << 0, /* respond to "drag_motion" */
  GTK_DEST_DEFAULT_HIGHLIGHT  = 1 << 1, /* auto-highlight */
  GTK_DEST_DEFAULT_DROP       = 1 << 2, /* respond to "drag_drop" */
  GTK_DEST_DEFAULT_ALL        = 0x07
} GtkDestDefaults;

/* Flags for the GtkTargetEntry on the destination side 
 */
typedef enum {
  GTK_TARGET_SAME_APP = 1 << 0,    /*< nick=same-app >*/
  GTK_TARGET_SAME_WIDGET = 1 << 1  /*< nick=same-widget >*/
} GtkTargetFlags;

/* Destination side */

void gtk_drag_get_data (GtkWidget      *widget,
			GdkDragContext *context,
			GdkAtom         target,
			guint32         time);
void gtk_drag_finish   (GdkDragContext *context,
			gboolean        success,
			gboolean        del,
			guint32         time);

GtkWidget *gtk_drag_get_source_widget (GdkDragContext *context);

void gtk_drag_highlight   (GtkWidget  *widget);
void gtk_drag_unhighlight (GtkWidget  *widget);

void gtk_drag_dest_set   (GtkWidget            *widget,
			  GtkDestDefaults       flags,
  		          const GtkTargetEntry *targets,
			  gint                  n_targets,
			  GdkDragAction         actions);

void gtk_drag_dest_set_proxy (GtkWidget      *widget,
			      GdkWindow      *proxy_window,
			      GdkDragProtocol protocol,
			      gboolean        use_coordinates);

/* There probably should be functions for setting the targets
 * as a GtkTargetList
 */

void gtk_drag_dest_unset (GtkWidget          *widget);

/* Source side */

void gtk_drag_source_set  (GtkWidget            *widget,
			   GdkModifierType       start_button_mask,
			   const GtkTargetEntry *targets,
			   gint                  n_targets,
			   GdkDragAction         actions);

void gtk_drag_source_unset (GtkWidget        *widget);

void gtk_drag_source_set_icon (GtkWidget     *widget,
			       GdkColormap   *colormap,
			       GdkPixmap     *pixmap,
			       GdkBitmap     *mask);

/* There probably should be functions for setting the targets
 * as a GtkTargetList
 */

GdkDragContext *gtk_drag_begin (GtkWidget         *widget,
				GtkTargetList     *targets,
				GdkDragAction      actions,
				gint               button,
				GdkEvent          *event);

/* Set the image being dragged around
 */
void gtk_drag_set_icon_widget  (GdkDragContext    *context,
				GtkWidget         *widget,
				gint               hot_x,
				gint               hot_y);

void gtk_drag_set_icon_pixmap  (GdkDragContext    *context,
				GdkColormap       *colormap,
				GdkPixmap         *pixmap,
				GdkBitmap         *mask,
				gint               hot_x,
				gint               hot_y);

void gtk_drag_set_icon_default (GdkDragContext    *context);

void gtk_drag_set_default_icon (GdkColormap   *colormap,
				GdkPixmap     *pixmap,
				GdkBitmap     *mask,
			        gint           hot_x,
			        gint           hot_y);


/* Internal functions */
void gtk_drag_source_handle_event (GtkWidget *widget,
				   GdkEvent  *event);
void gtk_drag_dest_handle_event (GtkWidget *toplevel,
				 GdkEvent  *event);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GTK_DND_H__ */



