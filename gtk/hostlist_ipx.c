/* hostlist_ipx.c   2004 Ian Schorr
 * modified from endpoint_talkers_ipx.c   2003 Ronnie Sahlberg
 *
 * $Id: hostlist_ipx.c,v 1.5 2004/02/23 20:28:30 ulfl Exp $
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
#include "packet-ipx.h"


static int
ipx_hostlist_packet(void *pit, packet_info *pinfo, epan_dissect_t *edt _U_, void *vip)
{
	hostlist_table *hosts=(hostlist_table *)pit;
	ipxhdr_t *ipxh=vip;

	/* Take two "add" passes per packet, adding for each direction, ensures that all
	packets are counted properly (even if address is sending to itself) 
	XXX - this could probably be done more efficiently inside hostlist_table */
	add_hostlist_table_data(hosts, &ipxh->ipx_src, 0, TRUE, 1, pinfo->fd->pkt_len, SAT_NONE, PT_NONE);
	add_hostlist_table_data(hosts, &ipxh->ipx_dst, 0, FALSE, 1, pinfo->fd->pkt_len, SAT_NONE, PT_NONE);

	return 1;
}



static void
gtk_ipx_hostlist_init(char *optarg)
{
	char *filter=NULL;

	if(!strncmp(optarg,"hosts,ipx,",10)){
		filter=optarg+10;
	} else {
		filter=NULL;
	}

	init_hostlist_table(TRUE, "IPX Hosts", "ipx", filter, (void *)ipx_hostlist_packet);

}


static void
gtk_ipx_hostlist_cb(GtkWidget *w _U_, gpointer d _U_)
{
	gtk_ipx_hostlist_init("hosts,ipx");
}


void
register_tap_listener_ipx_hostlist(void)
{
	register_ethereal_tap("hosts,ipx", gtk_ipx_hostlist_init);

	register_tap_menu_item("IPX", REGISTER_TAP_GROUP_ENDPOINT_LIST,
	    gtk_ipx_hostlist_cb, NULL, NULL, NULL);
}

