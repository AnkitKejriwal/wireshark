/* hostlist_fc.c   2004 Ian Schorr
 * modified from endpoint_talkers_fc.c   2003 Ronnie Sahlberg
 *
 * $Id: hostlist_fc.c,v 1.5 2004/02/23 20:28:30 ulfl Exp $
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

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#include <gtk/gtk.h>
#include <string.h>
#include "epan/packet.h"
#include "tap_menu.h"
#include "../tap.h"
#include "../register.h"
#include "hostlist_table.h"
#include "packet-fc.h"


static int
fc_hostlist_packet(void *pit, packet_info *pinfo, epan_dissect_t *edt _U_, void *vip)
{
	hostlist_table *hosts=(hostlist_table *)pit;
	fc_hdr *fchdr=vip;

	/* Take two "add" passes per packet, adding for each direction, ensures that all
	packets are counted properly (even if address is sending to itself) 
	XXX - this could probably be done more efficiently inside hostlist_table */
	add_hostlist_table_data(hosts, &fchdr->s_id, 0, TRUE, 1, pinfo->fd->pkt_len, SAT_NONE, PT_NONE);
	add_hostlist_table_data(hosts, &fchdr->d_id, 0, FALSE, 1, pinfo->fd->pkt_len, SAT_NONE, PT_NONE);

	return 1;
}



static void
gtk_fc_hostlist_init(char *optarg)
{
	char *filter=NULL;

	if(!strncmp(optarg,"hosts,fc,",9)){
		filter=optarg+9;
	} else {
		filter=NULL;
	}

	init_hostlist_table(TRUE, "Fibre Channel Hosts", "fc", filter, (void *)fc_hostlist_packet);

}


static void
gtk_fc_hostlist_cb(GtkWidget *w _U_, gpointer d _U_)
{
	gtk_fc_hostlist_init("hosts,fc");
}


void
register_tap_listener_fc_hostlist(void)
{
	register_ethereal_tap("hosts,fc", gtk_fc_hostlist_init);

	register_tap_menu_item("Fibre Channel", REGISTER_TAP_GROUP_ENDPOINT_LIST,
	    gtk_fc_hostlist_cb, NULL, NULL, NULL);
}

