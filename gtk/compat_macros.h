/* compat_macros.h
 * GTK-related Global defines, etc.
 *
 * $Id: compat_macros.h,v 1.1 2002/11/09 20:00:35 oabad Exp $
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

#ifndef __COMPAT_MACROS_H__
#define __COMPAT_MACROS_H__

/*
 * helper macros fro gtk1.2/gtk2 compatibility :
 * in gtk2, gtk_signal_xxx is deprecated in favor of g_signal_xxx
 *          gtk_object_xxx is deprecated in favor of g_object_xxx
 *          gtk_widget_set_usize is deprecated in favor of
 *              gtk_widget_set_size_request
 */
#if GTK_MAJOR_VERSION < 2

#define SIGNAL_CONNECT(widget, name, callback, arg) \
gtk_signal_connect(GTK_OBJECT(widget), name, GTK_SIGNAL_FUNC(callback), \
                   (gpointer)(arg));

#define SIGNAL_CONNECT_OBJECT(widget, name, callback, arg) \
gtk_signal_connect_object(GTK_OBJECT(widget), name, GTK_SIGNAL_FUNC(callback), \
                          (gpointer)(arg));

#define SIGNAL_DISCONNECT_BY_FUNC(object, func, data) \
gtk_signal_disconnect_by_func(GTK_OBJECT(object), func, data);

#define OBJECT_SET_DATA(widget, key, data) \
gtk_object_set_data(GTK_OBJECT(widget), key, data);

#define OBJECT_SET_DATA_FULL(widget, key, data, destroy) \
gtk_object_set_data_full(GTK_OBJECT(widget), key, (gpointer)(data), \
                         (GtkDestroyNotify)(destroy));

#define OBJECT_GET_DATA(widget, key) \
gtk_object_get_data(GTK_OBJECT(widget), key)

#define WIDGET_SET_SIZE(widget, width, height) \
gtk_widget_set_usize(GTK_WIDGET(widget), width, height);

#else

#define SIGNAL_CONNECT(widget, name, callback, arg) \
g_signal_connect(G_OBJECT(widget), name, G_CALLBACK(callback), \
                 (gpointer)(arg));

#define SIGNAL_CONNECT_OBJECT(widget, name, callback, arg) \
g_signal_connect_swapped(G_OBJECT(widget), name, G_CALLBACK(callback), \
                         (gpointer)(arg));

#define SIGNAL_DISCONNECT_BY_FUNC(object, func, data) \
g_signal_handlers_disconnect_by_func(G_OBJECT(object), func, data);

#define OBJECT_SET_DATA(widget, key, data) \
g_object_set_data(G_OBJECT(widget), key, data);

#define OBJECT_SET_DATA_FULL(widget, key, data, destroy) \
g_object_set_data_full(G_OBJECT(widget), key, (gpointer)(data), \
                       (GDestroyNotify)(destroy));

#define OBJECT_GET_DATA(widget, key) \
g_object_get_data(G_OBJECT(widget), key)

#define WIDGET_SET_SIZE(widget, width, height) \
gtk_widget_set_size_request(GTK_WIDGET(widget), width, height);

#endif

#endif
