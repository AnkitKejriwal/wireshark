/* endpoint_talkers_tr.c
 * endpoint_talkers_tr   2003 Ronnie Sahlberg
 *
 * $Id: endpoint_talkers_tr.c,v 1.14 2003/09/04 23:11:03 sahlberg Exp $
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
# include "config.h"
#endif

#include <stdio.h>

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#include <gtk/gtk.h>
#include <string.h>
#include "menu.h"
#include "../epan/packet_info.h"
#include "../epan/filesystem.h"
#include "../tap.h"
#include "../register.h"
#include "compat_macros.h"
#include "../simple_dialog.h"
#include "../file.h"
#include "../globals.h"
#include "endpoint_talkers_table.h"
#include "packet-tr.h"


static int
tr_talkers_packet(void *pit, packet_info *pinfo, epan_dissect_t *edt _U_, void *vip)
{
	endpoints_table *talkers=(endpoints_table *)pit;
	tr_hdr *trhdr=vip;

	add_ett_table_data(talkers, &trhdr->src, &trhdr->dst, 0, 0, 1, pinfo->fd->pkt_len, SAT_TOKENRING, PT_NONE);

	return 1;
}



static void
gtk_tr_talkers_init(char *optarg)
{
	char *filter=NULL;

	if(!strncmp(optarg,"talkers,tr,",11)){
		filter=optarg+11;
	} else {
		filter=NULL;
	}

	init_ett_table(TRUE, "Token Ring", "tr", filter, (void *)tr_talkers_packet);

}


static void
gtk_tr_endpoints_cb(GtkWidget *w _U_, gpointer d _U_)
{
	gtk_tr_talkers_init("talkers,tr");
}


void
register_tap_menu_tr_talkers(void)
{
	register_tap_menu_item("Endpoint Talkers/Token Ring", gtk_tr_endpoints_cb);
}




void
register_tap_listener_tr_talkers(void)
{
	register_ethereal_tap("talkers,tr", gtk_tr_talkers_init);
}

