/* tap-stats_tree.c
 * tethereal's tap implememntation of stats_tree
 * 2005, Luis E. G. Ontanon
 *
 * $Id: $
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
#include "config.h"
#endif

#include <string.h>
#include <stdio.h>
#include <glib.h>
#include <epan/stats_tree_priv.h>

/* actually unused */
struct _st_node_pres {
	void* dummy;
};

struct _tree_pres {
	void* dummy;
};

struct _tree_cfg_pres {
	guint8* init_string;	
};

static void draw_stats_tree(void *psp) {
	stats_tree *st = psp;
	GString* s;
	gchar* fmt;
	stat_node* child;
	
	s = g_string_new("\n===================================================================\n");
	fmt = g_strdup_printf(" %%s%%-%us%%12s\t%%12s\t%%12s\n",stats_branch_max_name_len(&st->root,0));
	g_string_sprintfa(s,fmt,"",st->cfg->name,"value","rate","percent");
	g_free(fmt);
	g_string_sprintfa(s,"-------------------------------------------------------------------\n");
	
	for (child = st->root.children; child; child = child->next ) {
		stat_branch_to_str(child,s,0);
	}
	
	s = g_string_append(s,"\n===================================================================\n");
	
	printf("%s",s->str);
	
}

static void  init_stats_tree(char *optarg) {
	guint8* abbr = get_st_abbr(optarg);
	GString	*error_string;
	stats_tree_cfg *cfg = NULL;
	stats_tree* st = NULL;
	
	if (abbr) {
		cfg = get_stats_tree_by_abbr(abbr);

		if (cfg != NULL) {
			if (strncmp (optarg, cfg->pr->init_string, strlen(cfg->pr->init_string)) == 0){
				st = new_stats_tree(cfg,NULL,((guint8*)optarg)+strlen(cfg->pr->init_string));
			} else {
				st->filter=NULL;
			}
		} else {
			g_error("no such stats_tree (%s) found in stats_tree registry",abbr);
		}
		
		g_free(abbr);
		
	} else {
		g_error("could not obtain stats_tree abbr (%s) from optarg '%s'",abbr,optarg);		
	}
	
	error_string = register_tap_listener( st->cfg->tapname,
										  st,
										  st->filter,
										  reset_stats_tree,
										  stats_tree_packet,
										  draw_stats_tree);
	
	if (error_string) {
		g_error("stats_tree for: %s failed to attach to the tap: %s",cfg->name,error_string->str);
	}

	if (cfg->init) cfg->init(st);

}

void register_stats_tree_tap (gpointer k _U_, gpointer v, gpointer p _U_) {
	stats_tree_cfg* cfg = v;
	
	cfg->pr = g_malloc(sizeof(tree_cfg_pres));
	cfg->pr->init_string = g_strdup_printf("%s,tree",cfg->abbr);

	register_ethereal_tap(cfg->pr->init_string, init_stats_tree);
	
}

static void free_tree_presentation(stats_tree* st) {
	g_free(st->pr);
}


void
register_tap_listener_stats_tree_stat(void)
{
	stats_tree_presentation(register_stats_tree_tap,
							NULL, NULL, NULL, NULL, NULL,
							free_tree_presentation, NULL, NULL, NULL);
}
