/* plugin_api.h
 * Routines for Ethereal plugins.
 *
 * $Id: plugin_api.h,v 1.8 2000/11/12 11:08:46 guy Exp $
 *
 * Ethereal - Network traffic analyzer
 * Copyright 2000 by Gilbert Ramirez <gram@xiexie.org>
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
 *
 */

#ifdef PLUGINS_NEED_ADDRESS_TABLE

#ifdef _WIN32
#define DLLEXPORT    __declspec(dllexport)
#endif

/* Some OSes (Win32) have DLLs that cannot reference symbols in the parent
   executable, so the executable needs to provide a collection of pointers
   to those functions for the DLL plugin to use. */

/* #defines for those functions that call through pointers.
   #defined in this fashion so that the declaration of the functions,
   from the system header files, turn into declarations of pointers
   to functions, and the calls to it in plugins, in the plugins, turn
   into calls through the pointers. */
#define	check_col			(*p_check_col)
#define	col_add_fstr			(*p_col_add_fstr)
#define	col_append_fstr			(*p_col_append_fstr)
#define	col_add_str			(*p_col_add_str)
#define	col_append_str			(*p_col_append_str)

#define	dfilter_init			(*p_dfilter_init)
#define	dfilter_cleanup			(*p_dfilter_cleanup)

#define	proto_register_protocol		(*p_proto_register_protocol)
#define	proto_register_field_array	(*p_proto_register_field_array)
#define	proto_register_subtree_array	(*p_proto_register_subtree_array)

#define	dissector_add			(*p_dissector_add)
#define old_dissector_add		(*p_old_dissector_add)

#define	heur_dissector_add		(*p_heur_dissector_add)

#define old_dissect_data		(*p_old_dissect_data)

#define proto_is_protocol_enabled	(*p_proto_is_protocol_enabled)

#define	proto_item_add_subtree		(*p_proto_item_add_subtree)
#define	proto_tree_add_item		(*p_proto_tree_add_item)
#define	proto_tree_add_item_hidden	(*p_proto_tree_add_item_hidden)
#define	proto_tree_add_protocol_format	(*p_proto_tree_add_protocol_format)
#define	proto_tree_add_bytes		(*p_proto_tree_add_bytes)
#define	proto_tree_add_bytes_hidden	(*p_proto_tree_add_bytes_hidden)
#define	proto_tree_add_bytes_format	(*p_proto_tree_add_bytes_format)
#define	proto_tree_add_time		(*p_proto_tree_add_time)
#define	proto_tree_add_time_hidden	(*p_proto_tree_add_time_hidden)
#define	proto_tree_add_time_format	(*p_proto_tree_add_time_format)
#define	proto_tree_add_ipxnet		(*p_proto_tree_add_ipxnet)
#define	proto_tree_add_ipxnet_hidden	(*p_proto_tree_add_ipxnet_hidden)
#define	proto_tree_add_ipxnet_format	(*p_proto_tree_add_ipxnet_format)
#define	proto_tree_add_ipv4		(*p_proto_tree_add_ipv4)
#define	proto_tree_add_ipv4_hidden	(*p_proto_tree_add_ipv4_hidden)
#define	proto_tree_add_ipv4_format	(*p_proto_tree_add_ipv4_format)
#define	proto_tree_add_ipv6		(*p_proto_tree_add_ipv6)
#define	proto_tree_add_ipv6_hidden	(*p_proto_tree_add_ipv6_hidden)
#define	proto_tree_add_ipv6_format	(*p_proto_tree_add_ipv6_format)
#define	proto_tree_add_ether		(*p_proto_tree_add_ether)
#define	proto_tree_add_ether_hidden	(*p_proto_tree_add_ether_hidden)
#define	proto_tree_add_ether_format	(*p_proto_tree_add_ether_format)
#define	proto_tree_add_string		(*p_proto_tree_add_string)
#define	proto_tree_add_string_hidden	(*p_proto_tree_add_string_hidden)
#define	proto_tree_add_string_format	(*p_proto_tree_add_string_format)
#define	proto_tree_add_boolean		(*p_proto_tree_add_boolean)
#define	proto_tree_add_boolean_hidden	(*p_proto_tree_add_boolean_hidden)
#define	proto_tree_add_boolean_format	(*p_proto_tree_add_boolean_format)
#define	proto_tree_add_double		(*p_proto_tree_add_double)
#define	proto_tree_add_double_hidden	(*p_proto_tree_add_double_hidden)
#define	proto_tree_add_double_format	(*p_proto_tree_add_double_format)
#define	proto_tree_add_uint		(*p_proto_tree_add_uint)
#define	proto_tree_add_uint_hidden	(*p_proto_tree_add_uint_hidden)
#define	proto_tree_add_uint_format	(*p_proto_tree_add_uint_format)
#define	proto_tree_add_int		(*p_proto_tree_add_int)
#define	proto_tree_add_int_hidden	(*p_proto_tree_add_int_hidden)
#define	proto_tree_add_int_format	(*p_proto_tree_add_int_format)
#define	proto_tree_add_text		(*p_proto_tree_add_text)
#define	proto_tree_add_notext		(*p_proto_tree_add_notext)

#define pi	(*p_pi)

#else

/* ! PLUGINS_NEED_ADDRESS_TABLE */
#define DLLEXPORT 

#endif

#include "packet.h"
#include "dfilter.h"

#include "plugin_table.h"

#ifdef PLUGINS_NEED_ADDRESS_TABLE
/* The parent executable will send us the pointer to a filled in
   plugin_address_table_t struct, and we copy the pointers from
   that table so that we can use functions from the parent executable. */
void plugin_address_table_init(plugin_address_table_t*);
#else
#define plugin_address_table_init(x)    ;
#endif
