/* epan.h
 *
 * $Id: epan.c,v 1.21 2002/09/09 21:04:06 guy Exp $
 *
 * Ethereal Protocol Analyzer Library
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include "epan.h"
#include "epan_dissect.h"

#include "conversation.h"
#include "except.h"
#include "packet.h"
#include "column-utils.h"
#include "../tap.h"

/*
 * XXX - this takes the plugin directory as an argument, because
 * libethereal now has its own configure script and "config.h" file,
 * which is what code in the "epan" directory includes, but we need
 * to define PLUGIN_DIR in the top-level directory, as it's used by,
 * for example, the Makefile for the Gryphon plugin, so it knows
 * where to install the plugin.
 *
 * Eventually, we should probably have an "epan-configure" script
 * (or "libethereal-configure", or whatever), along the lines of what
 * GTK+ and GLib have, that can print, among other things, the directory
 * into which plugins should be installed.  That way, only libethereal
 * need know what directory that is; programs using it won't, *and*
 * Makefiles for plugins can just use "epan-configure" to figure out
 * where to install the plugins.
 *
 * (Would that *more* libraries had configure scripts like that, so
 * that configure scripts didn't have to go through various contortions
 * to figure out where the header files and libraries for various
 * libraries are located.)
 */
void
epan_init(const char *plugin_dir, void (register_all_protocols)(void),
	  void (register_all_handoffs)(void))
{
	except_init();
	tvbuff_init();
	frame_data_init();
	tap_init();
	proto_init(plugin_dir,register_all_protocols,register_all_handoffs);
	packet_init();
	dfilter_init();
	final_registration_all_protocols();
}

void
epan_cleanup(void)
{
	dfilter_cleanup();
	proto_cleanup();
	packet_cleanup();
	frame_data_cleanup();
	tvbuff_cleanup();
	except_deinit();
}


void
epan_conversation_init(void)
{
	conversation_init();
}



epan_dissect_t*
epan_dissect_new(gboolean create_proto_tree, gboolean proto_tree_visible)
{
	epan_dissect_t	*edt;

	edt = g_new(epan_dissect_t, 1);

	if (create_proto_tree) {
		edt->tree = proto_tree_create_root();
		proto_tree_set_visible(edt->tree, proto_tree_visible);
	}
	else {
		edt->tree = NULL;
	}

	return edt;
}

void
epan_dissect_run(epan_dissect_t *edt, void* pseudo_header,
        const guint8* data, frame_data *fd, column_info *cinfo)
{
	dissect_packet(edt, pseudo_header, data, fd, cinfo);
}


void
epan_dissect_free(epan_dissect_t* edt)
{
	/* Free the data sources list. */
	free_data_sources(&edt->pi);

	/* Free all tvb's created from this tvb, unless dissector
	 * wanted to store the pointer (in which case, the dissector
	 * would have incremented the usage count on that tvbuff_t*) */
	tvb_free_chain(edt->tvb);

	if (edt->tree) {
		proto_tree_free(edt->tree);
	}

	g_free(edt);
}

void
epan_dissect_prime_dfilter(epan_dissect_t *edt, const dfilter_t* dfcode)
{
	dfilter_prime_proto_tree(dfcode, edt->tree);
}

void
epan_dissect_fill_in_columns(epan_dissect_t *edt)
{
    fill_in_columns(&edt->pi);
}
