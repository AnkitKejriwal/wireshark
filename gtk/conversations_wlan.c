/* conversations_wlan.c 2004 Giles Scott
 * modified from conversations_eth.c   2003 Ronnie Sahlberg
 *
 * $Id$
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
#include <epan/tap.h>
#include "../register.h"
#include "conversations_table.h"
#include <epan/dissectors/packet-ieee80211.h>


static int
wlan_conversation_packet(void *pct, packet_info *pinfo, epan_dissect_t *edt _U_, void *vip)
{
	wlan_hdr *whdr=vip;

	add_conversation_table_data((conversations_table *)pct, &whdr->src, &whdr->dst, 0, 0, 1, pinfo->fd->pkt_len, SAT_ETHER, PT_NONE);

	return 1;
}



static void
wlan_conversation_init(char *optarg)
{
	char *filter=NULL;

	if(!strncmp(optarg,"conv,wlan,",10)){
		filter=optarg+10;
	} else {
		filter=NULL;
	}

	init_conversation_table(TRUE, "WLAN", "wlan", filter, (void *)wlan_conversation_packet);

}


static void
wlan_endpoints_cb(GtkWidget *w _U_, gpointer d _U_)
{
	wlan_conversation_init("conv,wlan");
}


void
register_tap_listener_wlan_conversation(void)
{
	register_ethereal_tap("conv,wlan", wlan_conversation_init);

	register_tap_menu_item("WLAN", REGISTER_TAP_GROUP_CONVERSATION_LIST,
	    wlan_endpoints_cb, NULL, NULL, NULL);

	register_conversation_table(TRUE, "WLAN", "wlan", NULL /*filter*/, (void *)wlan_conversation_packet);
}

