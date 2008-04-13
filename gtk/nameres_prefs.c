/* nameres_prefs.c
 * Dialog box for name resolution preferences
 *
 * $Id$
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
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
#include "config.h"
#endif

#include <gtk/gtk.h>

#include <epan/addr_resolv.h>
#include <epan/prefs.h>
#include <epan/uat.h>

#include "../globals.h"

#include "gtk/nameres_prefs.h"
#include "gtk/gtkglobals.h"
#include "gtk/prefs_dlg.h"
#include "gtk/gui_utils.h"
#include "gtk/main.h"
#include "gtk/main_menu.h"


#define M_RESOLVE_KEY	"m_resolve"
#define N_RESOLVE_KEY	"n_resolve"
#define T_RESOLVE_KEY	"t_resolve"
#ifdef HAVE_GNU_ADNS
# define C_RESOLVE_KEY	"c_resolve"
# define RESOLVE_CONCURRENCY_KEY "resolve_concurrency"
#endif /* HAVE_GNU_ADNS */
#ifdef HAVE_LIBSMI
#define SP_RESOLVE_KEY	"sp_resolve"
#define SM_RESOLVE_KEY	"sm_resolve"
#endif

#ifdef HAVE_GNU_ADNS
# ifdef HAVE_LIBSMI
#  define RESOLV_TABLE_ROWS 7
# else
#  define RESOLV_TABLE_ROWS 5
# endif
#else
# ifdef HAVE_LIBSMI
#  define RESOLV_TABLE_ROWS 5
# else
#  define RESOLV_TABLE_ROWS 3
# endif
#endif /* HAVE_GNU_ADNS */
GtkWidget*
nameres_prefs_show(void)
{
	guint		table_row;
	GtkWidget	*main_tb, *main_vb;
	GtkWidget	*m_resolv_cb, *n_resolv_cb, *t_resolv_cb;
	GtkTooltips *tooltips = gtk_tooltips_new();
#ifdef HAVE_GNU_ADNS
	GtkWidget	*c_resolv_cb, *resolv_concurrency_te;
	char		concur_str[10+1];
#endif /* HAVE_GNU_ADNS */
#ifdef HAVE_LIBSMI
	GtkWidget	*sp_resolv_cb, *sm_resolv_cb;
	uat_t *smi_paths_uat;
	uat_t *smi_modules_uat;
#endif
	/*
	 * XXX - it would be nice if the current setting of the resolver
	 * flags could be different from the preference flags, so that
	 * the preference flags would represent what the user *typically*
	 * wants, but they could override them for particular captures
	 * without a subsequent editing of the preferences recording the
	 * temporary settings as permanent preferences.
	 */
	prefs.name_resolve = g_resolv_flags;

	/* Main vertical box */
	main_vb = gtk_vbox_new(FALSE, 7);
	gtk_container_border_width(GTK_CONTAINER(main_vb), 5);

	/* Main table */
	main_tb = gtk_table_new(RESOLV_TABLE_ROWS, 3, FALSE);
	gtk_box_pack_start(GTK_BOX(main_vb), main_tb, FALSE, FALSE, 0);
	gtk_table_set_row_spacings(GTK_TABLE(main_tb), 10);
	gtk_table_set_col_spacings(GTK_TABLE(main_tb), 15);
	gtk_widget_show(main_tb);
	g_object_set_data(G_OBJECT(main_tb), E_TOOLTIPS_KEY, tooltips);

	/* Resolve MAC addresses */
	table_row = 0;
	m_resolv_cb = create_preference_check_button(main_tb, table_row,
	    "Enable MAC name resolution:", "e.g. Ethernet address to manufacturer name",
	    prefs.name_resolve & RESOLV_MAC);
	g_object_set_data(G_OBJECT(main_vb), M_RESOLVE_KEY, m_resolv_cb);

	/* Resolve network addresses */
	table_row++;
	n_resolv_cb = create_preference_check_button(main_tb, table_row,
	    "Enable network name resolution:", "e.g. IP address to DNS name (hostname)",
	    prefs.name_resolve & RESOLV_NETWORK);
	g_object_set_data(G_OBJECT(main_vb), N_RESOLVE_KEY, n_resolv_cb);

	/* Resolve transport addresses */
	table_row++;
	t_resolv_cb = create_preference_check_button(main_tb, table_row,
	    "Enable transport name resolution:", "e.g. TCP/UDP port to service name",
	    prefs.name_resolve & RESOLV_TRANSPORT);
	g_object_set_data(G_OBJECT(main_vb), T_RESOLVE_KEY, t_resolv_cb);

#ifdef HAVE_GNU_ADNS
	/* Enable concurrent (asynchronous) DNS lookups */
	table_row++;
	c_resolv_cb = create_preference_check_button(main_tb, table_row,
	    "Enable concurrent DNS name resolution:", "be sure to enable network name resolution",
	    prefs.name_resolve & RESOLV_CONCURRENT);
	g_object_set_data(G_OBJECT(main_vb), C_RESOLVE_KEY, c_resolv_cb);

	/* Max concurrent requests */
	table_row++;
	g_snprintf(concur_str, 10+1, "%d", prefs.name_resolve_concurrency);
	resolv_concurrency_te = create_preference_entry(main_tb, table_row,
	    "Maximum concurrent requests:", "maximum parallel running DNS requests", concur_str);
	g_object_set_data(G_OBJECT(main_vb), RESOLVE_CONCURRENCY_KEY, resolv_concurrency_te);

#endif /* HAVE_GNU_ADNS */
#ifdef HAVE_LIBSMI
	/* SMI paths UAT */
	smi_paths_uat = uat_get_table_by_name("SMI Paths");
	if (smi_paths_uat) {
		table_row++;
		sp_resolv_cb = create_preference_uat(main_tb, table_row,
		    "SMI (MIB and PIB) paths",
                    "Search paths for SMI (MIB and PIB) modules. You must\n"
                    "restart Wireshark for these changes to take effect.",
                    smi_paths_uat);
		g_object_set_data(G_OBJECT(main_vb), SP_RESOLVE_KEY, sp_resolv_cb);
	}

	/* SMI modules UAT */
	smi_modules_uat = uat_get_table_by_name("SMI Modules");
	if (smi_modules_uat) {
		table_row++;
		sm_resolv_cb = create_preference_uat(main_tb, table_row,
		    "SMI (MIB and PIB) modules",
                    "List of enabled SMI (MIB and PIB) modules. You must\n"
                    "restart Wireshark for these changes to take effect.",
                    smi_modules_uat);
		g_object_set_data(G_OBJECT(main_vb), SM_RESOLVE_KEY, sm_resolv_cb);
	}
#endif

	/* Show 'em what we got */
	gtk_widget_show_all(main_vb);

	return(main_vb);
}

void
nameres_prefs_fetch(GtkWidget *w)
{
	GtkWidget *m_resolv_cb, *n_resolv_cb, *t_resolv_cb;
#ifdef HAVE_GNU_ADNS
	GtkWidget *c_resolv_cb, *resolv_concurrency_te;
#endif /* HAVE_GNU_ADNS */
#ifdef HAVE_LIBSMI
	GtkWidget *sp_resolv_cb, *sm_resolv_cb;
#endif

	m_resolv_cb = (GtkWidget *)g_object_get_data(G_OBJECT(w), M_RESOLVE_KEY);
	n_resolv_cb = (GtkWidget *)g_object_get_data(G_OBJECT(w), N_RESOLVE_KEY);
	t_resolv_cb = (GtkWidget *)g_object_get_data(G_OBJECT(w), T_RESOLVE_KEY);
#ifdef HAVE_GNU_ADNS
	c_resolv_cb = (GtkWidget *)g_object_get_data(G_OBJECT(w), C_RESOLVE_KEY);

	resolv_concurrency_te = (GtkWidget *)g_object_get_data(G_OBJECT(w), RESOLVE_CONCURRENCY_KEY);
#endif /* HAVE_GNU_ADNS */
#ifdef HAVE_LIBSMI
	sp_resolv_cb = (GtkWidget *)g_object_get_data(G_OBJECT(w), SP_RESOLVE_KEY);
	sm_resolv_cb = (GtkWidget *)g_object_get_data(G_OBJECT(w), SM_RESOLVE_KEY);
#endif

	prefs.name_resolve = RESOLV_NONE;
	prefs.name_resolve |= (GTK_TOGGLE_BUTTON (m_resolv_cb)->active ? RESOLV_MAC : RESOLV_NONE);
	prefs.name_resolve |= (GTK_TOGGLE_BUTTON (n_resolv_cb)->active ? RESOLV_NETWORK : RESOLV_NONE);
	prefs.name_resolve |= (GTK_TOGGLE_BUTTON (t_resolv_cb)->active ? RESOLV_TRANSPORT : RESOLV_NONE);
#ifdef HAVE_GNU_ADNS
	prefs.name_resolve |= (GTK_TOGGLE_BUTTON (c_resolv_cb)->active ? RESOLV_CONCURRENT : RESOLV_NONE);

	prefs.name_resolve_concurrency = strtol (gtk_entry_get_text(
		GTK_ENTRY(resolv_concurrency_te)), NULL, 10);
#endif /* HAVE_GNU_ADNS */
}

void
nameres_prefs_apply(GtkWidget *w _U_)
{
	/*
	 * XXX - force a regeneration of the protocol list if this has
	 * changed?
	 */
	g_resolv_flags = prefs.name_resolve;
	menu_name_resolution_changed();
}

void
nameres_prefs_destroy(GtkWidget *w _U_)
{
}
