/* plugins.c
 * plugin routines
 *
 * $Id: plugins.c,v 1.72 2003/06/04 00:11:02 guy Exp $
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@ethereal.com>
 * Copyright 1999 Gerald Combs
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

#ifdef NEED_SNPRINTF_H
# include "snprintf.h"
#endif

#include "plugins.h"

#ifdef HAVE_PLUGINS

#include <time.h>

#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif

#ifdef HAVE_DIRECT_H
#include <direct.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <errno.h>

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "filesystem.h"

#ifdef PLUGINS_NEED_ADDRESS_TABLE
#include "conversation.h"
#include "reassemble.h"
#include "prefs.h"
#include "packet-giop.h"
#include "packet-tpkt.h"
#include "packet-tcp.h"
#include "tap.h"
#include "asn1.h"
#include "plugins/plugin_table.h"
static plugin_address_table_t	patable;
#endif

/* linked list of all plugins */
plugin *plugin_list;

static gchar *user_plug_dir = NULL;

#define PLUGINS_DIR_NAME	"plugins"

/*
 * add a new plugin to the list
 * returns :
 * - 0 : OK
 * - ENOMEM : memory allocation problem
 * - EEXIST : the same plugin (i.e. name/version) was already registered.
 */
static int
add_plugin(void *handle, gchar *name, gchar *version,
	   void (*reg_handoff)(void))
{
    plugin *new_plug, *pt_plug;

    pt_plug = plugin_list;
    if (!pt_plug) /* the list is empty */
    {
	new_plug = (plugin *)g_malloc(sizeof(plugin));
	if (new_plug == NULL) return ENOMEM;
        plugin_list = new_plug;
    }
    else
    {
	while (1)
	{
	    /* check if the same name/version is already registered */
	    if (!strcmp(pt_plug->name, name) &&
		!strcmp(pt_plug->version, version))
	    {
		return EEXIST;
	    }

	    /* we found the last plugin in the list */
	    if (pt_plug->next == NULL) break;

	    pt_plug = pt_plug->next;
	}
	new_plug = (plugin *)g_malloc(sizeof(plugin));
	if (new_plug == NULL) return ENOMEM;
	pt_plug->next = new_plug;
    }

    new_plug->handle = handle;
    new_plug->name = name;
    new_plug->version = version;
    new_plug->reg_handoff = reg_handoff;
    new_plug->next = NULL;
    return 0;
}

/*
 * XXX - when we remove support for old-style plugins (which we should
 * probably do eventually, as all plugins should be written as new-style
 * ones), we may want to have "init_plugins()" merely save a pointer
 * to the plugin's "init" routine, just as we save a pointer to its
 * "reg_handoff" routine, and have a "register_all_plugins()" routine
 * to go through the list of plugins and call all of them.
 *
 * Then we'd have "epan_init()", or perhaps even something higher up
 * in the call tree, call "init_plugins()", and have "proto_init()"
 * call "register_all_plugins()" right after calling "register_all_protocols()";
 * this might be a bit cleaner.
 */
static void
plugins_scan_dir(const char *dirname)
{
#define FILENAME_LEN	1024
#if GLIB_MAJOR_VERSION < 2
    gchar         *hack_path;       /* pathname used to construct lt_lib_ext */
    gchar         *lt_lib_ext;      /* extension for loadable modules */
    DIR           *dir;             /* scanned directory */
    struct dirent *file;            /* current file */
    gchar         *name;
#else /* GLIB 2 */
    GDir          *dir;             /* scanned directory */
    GError        **dummy;
    const gchar   *name;
#endif
    gchar          filename[FILENAME_LEN];   /* current file name */
    GModule       *handle;          /* handle returned by dlopen */
    gchar         *version;
    void         (*init)(void *);
    void         (*reg_handoff)(void);
    gchar         *dot;
    int            cr;

#if GLIB_MAJOR_VERSION < 2
    /*
     * We find the extension used on this platform for loadable modules
     * by the sneaky hack of calling "g_module_build_path" to build
     * the pathname for a module with an empty directory name and
     * empty module name, and then search for the last "." and use
     * everything from the last "." on.
     */
    hack_path = g_module_build_path("", "");
    lt_lib_ext = strrchr(hack_path, '.');
    if (lt_lib_ext == NULL)
    {
	/*
	 * Does this mean there *is* no extension?  Assume so.
	 *
	 * XXX - the code below assumes that all loadable modules have
	 * an extension....
	 */
	lt_lib_ext = "";
    }

    if ((dir = opendir(dirname)) != NULL)
    {
	while ((file = readdir(dir)) != NULL)
	{
	    /* don't try to open "." and ".." */
	    if (!(strcmp(file->d_name, "..") &&
		  strcmp(file->d_name, "."))) continue;

            /* skip anything but files with lt_lib_ext */
            dot = strrchr(file->d_name, '.');
            if (dot == NULL || strcmp(dot, lt_lib_ext) != 0) continue;

	    snprintf(filename, FILENAME_LEN, "%s" G_DIR_SEPARATOR_S "%s",
	        dirname, file->d_name);
	    name = (gchar *)file->d_name;
#else /* GLIB 2 */
    /*
     * GLib 2.x defines G_MODULE_SUFFIX as the extension used on this
     * platform for loadable modules.
     */
    dummy = g_malloc(sizeof(GError *));
    *dummy = NULL;
    if ((dir = g_dir_open(dirname, 0, dummy)) != NULL)
    {
    	while ((name = g_dir_read_name(dir)) != NULL)
	{
	    /* skip anything but files with G_MODULE_SUFFIX */
            dot = strrchr(name, '.');
            if (dot == NULL || strcmp(dot+1, G_MODULE_SUFFIX) != 0) continue;

	    snprintf(filename, FILENAME_LEN, "%s" G_DIR_SEPARATOR_S "%s",
	        dirname, name);
#endif
	    if ((handle = g_module_open(filename, 0)) == NULL)
	    {
		g_warning("Couldn't load module %s: %s", filename,
			  g_module_error());
		continue;
	    }
	    if (g_module_symbol(handle, "version", (gpointer*)&version) == FALSE)
	    {
	        g_warning("The plugin %s has no version symbol", name);
		g_module_close(handle);
		continue;
	    }

	    /*
	     * Old-style dissectors don't have a "plugin_reg_handoff()"
	     * routine; we no longer support them.
	     *
	     * New-style dissectors have one, because, otherwise, there's
	     * no way for them to arrange that they ever be called.
	     */
	    if (g_module_symbol(handle, "plugin_reg_handoff",
					 (gpointer*)&reg_handoff))
	    {
		/*
		 * We require it to have a "plugin_init()" routine.
		 */
		if (!g_module_symbol(handle, "plugin_init", (gpointer*)&init))
		{
		    g_warning("The plugin %s has a plugin_reg_handoff symbol but no plugin_init routine",
			      name);
		    g_module_close(handle);
		    continue;
		}

		/*
		 * We have a "plugin_reg_handoff()" routine, so we don't
		 * need the protocol, filter string, or dissector pointer.
		 */
		if ((cr = add_plugin(handle, g_strdup(name), version,
				     reg_handoff)))
		{
		    if (cr == EEXIST)
			fprintf(stderr, "The plugin %s, version %s\n"
			    "was found in multiple directories\n", name, version);
		    else
			fprintf(stderr, "Memory allocation problem\n"
			    "when processing plugin %s, version %s\n",
			    name, version);
		    g_module_close(handle);
		    continue;
		}

		/*
		 * Call its init routine.
		 */
#ifdef PLUGINS_NEED_ADDRESS_TABLE
		init(&patable);
#else
		init(NULL);
#endif
	    }
	    else
	    {
		/*
		 * This is an old-style dissector; warn that it won't
		 * be used, as those aren't supported.
		 */
		fprintf(stderr,
		    "The plugin %s, version %s is an old-style plugin;\n"
		    "Those are no longer supported.\n", name, version);
	    }
	}
#if GLIB_MAJOR_VERSION < 2
	closedir(dir);
    }
    g_free(hack_path);
#else /* GLIB 2 */
	g_dir_close(dir);
    }
    g_clear_error(dummy);
    g_free(dummy);
#endif
}

/*
 * init plugins
 */
void
init_plugins(const char *plugin_dir)
{
#ifdef WIN32
    const char *datafile_dir;
    char *install_plugin_dir;
#endif

    if (plugin_list == NULL)      /* ensure init_plugins is only run once */
    {
#ifdef PLUGINS_NEED_ADDRESS_TABLE
	/* Intialize address table */
	patable.p_check_col			= check_col;
	patable.p_col_clear			= col_clear;
	patable.p_col_add_fstr			= col_add_fstr;
	patable.p_col_append_fstr		= col_append_fstr;
	patable.p_col_prepend_fstr		= col_prepend_fstr;
	patable.p_col_add_str			= col_add_str;
	patable.p_col_append_str		= col_append_str;
	patable.p_col_set_str			= col_set_str;

	patable.p_register_init_routine		= register_init_routine;
	patable.p_register_postseq_cleanup_routine	= register_postseq_cleanup_routine;

	patable.p_match_strval			= match_strval;
	patable.p_val_to_str			= val_to_str;

	patable.p_conversation_new		= conversation_new;
	patable.p_find_conversation		= find_conversation;
	patable.p_conversation_set_dissector	= conversation_set_dissector;

	patable.p_proto_register_protocol	= proto_register_protocol;
	patable.p_proto_register_field_array	= proto_register_field_array;
	patable.p_proto_register_subtree_array	= proto_register_subtree_array;

	patable.p_dissector_add			= dissector_add;
	patable.p_dissector_delete		= dissector_delete;
	patable.p_dissector_add_handle		= dissector_add_handle;

	patable.p_heur_dissector_add		= heur_dissector_add;

	patable.p_register_dissector		= register_dissector;
	patable.p_find_dissector		= find_dissector;
	patable.p_create_dissector_handle	= create_dissector_handle;
	patable.p_call_dissector		= call_dissector;

	patable.p_tcp_dissect_pdus		= tcp_dissect_pdus;

	patable.p_proto_is_protocol_enabled	= proto_is_protocol_enabled;

	patable.p_proto_item_get_len		= proto_item_get_len;
	patable.p_proto_item_set_len		= proto_item_set_len;
	patable.p_proto_item_set_text		= proto_item_set_text;
	patable.p_proto_item_append_text	= proto_item_append_text;
	patable.p_proto_item_add_subtree	= proto_item_add_subtree;
	patable.p_proto_tree_add_item		= proto_tree_add_item;
	patable.p_proto_tree_add_item_hidden	= proto_tree_add_item_hidden;
	patable.p_proto_tree_add_protocol_format = proto_tree_add_protocol_format;
	patable.p_proto_tree_add_bytes		= proto_tree_add_bytes;
	patable.p_proto_tree_add_bytes_hidden	= proto_tree_add_bytes_hidden;
	patable.p_proto_tree_add_bytes_format	= proto_tree_add_bytes_format;
	patable.p_proto_tree_add_time		= proto_tree_add_time;
	patable.p_proto_tree_add_time_hidden	= proto_tree_add_time_hidden;
	patable.p_proto_tree_add_time_format	= proto_tree_add_time_format;
	patable.p_proto_tree_add_ipxnet		= proto_tree_add_ipxnet;
	patable.p_proto_tree_add_ipxnet_hidden	= proto_tree_add_ipxnet_hidden;
	patable.p_proto_tree_add_ipxnet_format	= proto_tree_add_ipxnet_format;
	patable.p_proto_tree_add_ipv4		= proto_tree_add_ipv4;
	patable.p_proto_tree_add_ipv4_hidden	= proto_tree_add_ipv4_hidden;
	patable.p_proto_tree_add_ipv4_format	= proto_tree_add_ipv4_format;
	patable.p_proto_tree_add_ipv6		= proto_tree_add_ipv6;
	patable.p_proto_tree_add_ipv6_hidden	= proto_tree_add_ipv6_hidden;
	patable.p_proto_tree_add_ipv6_format	= proto_tree_add_ipv6_format;
	patable.p_proto_tree_add_ether		= proto_tree_add_ether;
	patable.p_proto_tree_add_ether_hidden	= proto_tree_add_ether_hidden;
	patable.p_proto_tree_add_ether_format	= proto_tree_add_ether_format;
	patable.p_proto_tree_add_string		= proto_tree_add_string;
	patable.p_proto_tree_add_string_hidden	= proto_tree_add_string_hidden;
	patable.p_proto_tree_add_string_format	= proto_tree_add_string_format;
	patable.p_proto_tree_add_boolean	= proto_tree_add_boolean;
	patable.p_proto_tree_add_boolean_hidden	= proto_tree_add_boolean_hidden;
	patable.p_proto_tree_add_boolean_format	= proto_tree_add_boolean_format;
	patable.p_proto_tree_add_double		= proto_tree_add_double;
	patable.p_proto_tree_add_double_hidden	= proto_tree_add_double_hidden;
	patable.p_proto_tree_add_double_format	= proto_tree_add_double_format;
	patable.p_proto_tree_add_uint		= proto_tree_add_uint;
	patable.p_proto_tree_add_uint_hidden	= proto_tree_add_uint_hidden;
	patable.p_proto_tree_add_uint_format	= proto_tree_add_uint_format;
	patable.p_proto_tree_add_int		= proto_tree_add_int;
	patable.p_proto_tree_add_int_hidden	= proto_tree_add_int_hidden;
	patable.p_proto_tree_add_int_format	= proto_tree_add_int_format;
	patable.p_proto_tree_add_text		= proto_tree_add_text;

	patable.p_tvb_new_subset		= tvb_new_subset;
	patable.p_tvb_set_free_cb		= tvb_set_free_cb;
	patable.p_tvb_set_child_real_data_tvbuff = tvb_set_child_real_data_tvbuff;
	patable.p_tvb_new_real_data		= tvb_new_real_data;

	patable.p_tvb_length			= tvb_length;
	patable.p_tvb_length_remaining		= tvb_length_remaining;
	patable.p_tvb_bytes_exist		= tvb_bytes_exist;
	patable.p_tvb_offset_exists		= tvb_offset_exists;
	patable.p_tvb_reported_length		= tvb_reported_length;
	patable.p_tvb_reported_length_remaining	= tvb_reported_length_remaining;

	patable.p_tvb_get_guint8		= tvb_get_guint8;

	patable.p_tvb_get_ntohs			= tvb_get_ntohs;
	patable.p_tvb_get_ntoh24		= tvb_get_ntoh24;
	patable.p_tvb_get_ntohl			= tvb_get_ntohl;

	patable.p_tvb_get_letohs		= tvb_get_letohs;
	patable.p_tvb_get_letoh24		= tvb_get_letoh24;
	patable.p_tvb_get_letohl		= tvb_get_letohl;

	patable.p_tvb_memcpy			= tvb_memcpy;
	patable.p_tvb_memdup			= tvb_memdup;

	patable.p_tvb_get_ptr			= tvb_get_ptr;

	patable.p_tvb_find_guint8		= tvb_find_guint8;
	patable.p_tvb_pbrk_guint8		= tvb_pbrk_guint8;

	patable.p_tvb_strnlen			= tvb_strnlen;

	patable.p_tvb_format_text		= tvb_format_text;

	patable.p_tvb_get_nstringz		= tvb_get_nstringz;
	patable.p_tvb_get_nstringz0		= tvb_get_nstringz0;

	patable.p_tvb_find_line_end		= tvb_find_line_end;
	patable.p_tvb_find_line_end_unquoted	= tvb_find_line_end_unquoted;

	patable.p_tvb_strneql			= tvb_strneql;
	patable.p_tvb_strncaseeql		= tvb_strncaseeql;

	patable.p_tvb_bytes_to_str		= tvb_bytes_to_str;

	patable.p_prefs_register_protocol	= prefs_register_protocol;
	patable.p_prefs_register_uint_preference = prefs_register_uint_preference;
	patable.p_prefs_register_bool_preference = prefs_register_bool_preference;
	patable.p_prefs_register_enum_preference = prefs_register_enum_preference;
	patable.p_prefs_register_string_preference = prefs_register_string_preference;

	patable.p_register_giop_user		= register_giop_user;
	patable.p_is_big_endian			= is_big_endian;
	patable.p_get_CDR_encap_info		= get_CDR_encap_info;

	patable.p_get_CDR_any			= get_CDR_any;
	patable.p_get_CDR_boolean		= get_CDR_boolean;
	patable.p_get_CDR_char			= get_CDR_char;
	patable.p_get_CDR_double		= get_CDR_double;
	patable.p_get_CDR_enum			= get_CDR_enum;
	patable.p_get_CDR_fixed			= get_CDR_fixed;
	patable.p_get_CDR_float			= get_CDR_float;
	patable.p_get_CDR_interface		= get_CDR_interface;
	patable.p_get_CDR_long			= get_CDR_long;
	patable.p_get_CDR_object		= get_CDR_object;
	patable.p_get_CDR_octet			= get_CDR_octet;
	patable.p_get_CDR_octet_seq		= get_CDR_octet_seq;
	patable.p_get_CDR_short			= get_CDR_short;
	patable.p_get_CDR_string		= get_CDR_string;
	patable.p_get_CDR_typeCode		= get_CDR_typeCode;
	patable.p_get_CDR_ulong			= get_CDR_ulong;
	patable.p_get_CDR_ushort		= get_CDR_ushort;
	patable.p_get_CDR_wchar			= get_CDR_wchar;
	patable.p_get_CDR_wstring		= get_CDR_wstring;

	patable.p_is_tpkt			= is_tpkt;
	patable.p_dissect_tpkt_encap		= dissect_tpkt_encap;

	patable.p_set_actual_length		= set_actual_length;
	patable.p_decode_boolean_bitfield	= decode_boolean_bitfield;
	patable.p_decode_numeric_bitfield	= decode_numeric_bitfield;
	patable.p_decode_enumerated_bitfield	= decode_enumerated_bitfield;
	patable.p_register_dissector_table	= register_dissector_table;
	patable.p_except_throw			= except_throw;
	patable.p_dissector_try_port		= dissector_try_port;

	patable.p_conversation_add_proto_data	= conversation_add_proto_data;
	patable.p_conversation_get_proto_data	= conversation_get_proto_data;
	patable.p_conversation_delete_proto_data = conversation_delete_proto_data;
	patable.p_p_add_proto_data		= p_add_proto_data;
	patable.p_p_get_proto_data		= p_get_proto_data;

	patable.p_ip_to_str			= ip_to_str;
	patable.p_ip6_to_str			= ip6_to_str;
	patable.p_time_secs_to_str		= time_secs_to_str;
	patable.p_time_msecs_to_str		= time_msecs_to_str;
	patable.p_abs_time_to_str		= abs_time_to_str;

	patable.p_proto_get_id_by_filter_name	= proto_get_id_by_filter_name;
	patable.p_proto_get_protocol_name	= proto_get_protocol_name;
	patable.p_proto_get_protocol_short_name	= proto_get_protocol_short_name;
	patable.p_proto_get_protocol_filter_name	= proto_get_protocol_filter_name;

	patable.p_prefs_register_obsolete_preference	= prefs_register_obsolete_preference;

	patable.p_add_new_data_source		= add_new_data_source;

	patable.p_fragment_table_init		= fragment_table_init;
	patable.p_reassembled_table_init	= reassembled_table_init;
	patable.p_fragment_add			= fragment_add;
	patable.p_fragment_add_seq		= fragment_add_seq;
	patable.p_fragment_add_seq_check	= fragment_add_seq_check;
	patable.p_fragment_add_seq_next		= fragment_add_seq_next;
	patable.p_fragment_get			= fragment_get;
	patable.p_fragment_set_tot_len		= fragment_set_tot_len;
	patable.p_fragment_get_tot_len		= fragment_get_tot_len;
	patable.p_fragment_set_partial_reassembly	= fragment_set_partial_reassembly;
	patable.p_fragment_delete		= fragment_delete;
	patable.p_show_fragment_tree		= show_fragment_tree;
	patable.p_show_fragment_seq_tree	= show_fragment_seq_tree;

	patable.p_register_tap			= register_tap;
	patable.p_tap_queue_packet		= tap_queue_packet;

	patable.p_asn1_open			= asn1_open;
	patable.p_asn1_close			= asn1_close;
	patable.p_asn1_octet_decode		= asn1_octet_decode;
	patable.p_asn1_tag_decode		= asn1_tag_decode;
	patable.p_asn1_id_decode		= asn1_id_decode;
	patable.p_asn1_length_decode		= asn1_length_decode;
	patable.p_asn1_header_decode		= asn1_header_decode;
	patable.p_asn1_eoc			= asn1_eoc;
	patable.p_asn1_eoc_decode		= asn1_eoc_decode;
	patable.p_asn1_null_decode		= asn1_null_decode;
	patable.p_asn1_bool_decode		= asn1_bool_decode;
	patable.p_asn1_int32_value_decode	= asn1_int32_value_decode;
	patable.p_asn1_int32_decode		= asn1_int32_decode;
	patable.p_asn1_uint32_value_decode	= asn1_uint32_value_decode;
	patable.p_asn1_uint32_decode		= asn1_uint32_decode;
	patable.p_asn1_bits_decode		= asn1_bits_decode;
	patable.p_asn1_string_value_decode	= asn1_string_value_decode;
	patable.p_asn1_string_decode		= asn1_string_decode;
	patable.p_asn1_octet_string_decode	= asn1_octet_string_decode;
	patable.p_asn1_subid_decode		= asn1_subid_decode;
	patable.p_asn1_oid_value_decode		= asn1_oid_value_decode;
	patable.p_asn1_oid_decode		= asn1_oid_decode;
	patable.p_asn1_sequence_decode		= asn1_sequence_decode;
	patable.p_asn1_err_to_str		= asn1_err_to_str;

	patable.p_proto_item_set_end		= proto_item_set_end;
	patable.p_proto_tree_add_none_format	= proto_tree_add_none_format;

	patable.p_except_init			= except_init;
	patable.p_except_deinit			= except_deinit;
	patable.p_except_rethrow		= except_rethrow;
	patable.p_except_throwd			= except_throwd;
	patable.p_except_throwf			= except_throwf;
	patable.p_except_unhandled_catcher	= except_unhandled_catcher;
	patable.p_except_take_data	       	= except_take_data;
	patable.p_except_set_allocator		= except_set_allocator;
	patable.p_except_alloc			= except_alloc;
	patable.p_except_free			= except_free;
	patable.p_except_pop			= except_pop;
	patable.p_except_setup_try		= except_setup_try;

	patable.p_col_set_fence			= col_set_fence;
#endif

#ifdef WIN32
	/*
	 * On Windows, the data file directory is the installation
	 * directory; the plugins are stored under it.
	 *
	 * Assume we're running the installed version of Ethereal;
	 * on Windows, the data file directory is the directory
	 * in which the Ethereal binary resides.
	 */
	datafile_dir = get_datafile_dir();
	install_plugin_dir = g_malloc(strlen(datafile_dir) + strlen("plugins") +
	    strlen(VERSION) + 3);
	sprintf(install_plugin_dir, "%s\\plugins\\%s", datafile_dir, VERSION);

	/*
	 * Make sure that pathname refers to a directory.
	 */
	if (test_for_directory(install_plugin_dir) != EISDIR) {
		/*
		 * Either it doesn't refer to a directory or it
		 * refers to something that doesn't exist.
		 *
		 * Assume that means we're running, for example,
		 * a version of Ethereal we've built in a source
		 * directory, and fall back on the default
		 * installation directory, so you can put the plugins
		 * somewhere so they can be used with this version
		 * of Ethereal.
		 *
		 * XXX - should we, instead, have the Windows build
		 * procedure create a subdirectory of the "plugins"
		 * source directory, and copy the plugin DLLs there,
		 * so that you use the plugins from the build tree?
		 */
		install_plugin_dir =
		    g_strdup("C:\\Program Files\\Ethereal\\plugins\\" VERSION);
	}

	/*
	 * Scan that directory.
	 */
	plugins_scan_dir(install_plugin_dir);
	g_free(install_plugin_dir);
#else
	/*
	 * Scan the plugin directory.
	 */
	plugins_scan_dir(plugin_dir);
#endif
	if (!user_plug_dir)
	    user_plug_dir = get_persconffile_path(PLUGINS_DIR_NAME, FALSE);
	plugins_scan_dir(user_plug_dir);
    }
}

void
register_all_plugin_handoffs(void)
{
  plugin *pt_plug;

  /*
   * For all new-style plugins, call the register-handoff routine.
   * This is called from "proto_init()"; it must be called after
   * "register_all_protocols()" and "init_plugins()" are called,
   * in case one plugin registers itself either with a built-in
   * dissector or with another plugin; we must first register all
   * dissectors, whether built-in or plugin, so their dissector tables
   * are initialized, and only then register all handoffs.
   *
   * We treat those protocols as always being enabled; they should
   * use the standard mechanism for enabling/disabling protocols, not
   * the plugin-specific mechanism.
   */
  for (pt_plug = plugin_list; pt_plug != NULL; pt_plug = pt_plug->next)
    (pt_plug->reg_handoff)();
}
#endif
