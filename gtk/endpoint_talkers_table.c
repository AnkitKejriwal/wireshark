/* mem leak   should free the column_arrows when the table is destroyed */

/* endpoint_talkers_table.c
 * endpoint_talkers_table   2003 Ronnie Sahlberg
 * Helper routines common to all endpoint talkers tap.
 *
 * $Id: endpoint_talkers_table.c,v 1.38 2004/05/03 22:15:21 ulfl Exp $
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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <gtk/gtk.h>

#include "compat_macros.h"
#include "epan/packet_info.h"
#include "epan/to_str.h"
#include "epan/resolv.h"
#include "endpoint_talkers_table.h"
#include "image/clist_ascend.xpm"
#include "image/clist_descend.xpm"
#include "simple_dialog.h"
#include "globals.h"
#include "tap.h"
#include "gtk/find_dlg.h"
#include "color.h"
#include "gtk/color_dlg.h"
#include "gtkglobals.h"
#include "main.h"
#include "ui_util.h"
#include "dlg_utils.h"

extern GtkWidget   *main_display_filter_widget;


#define GTK_MENU_FUNC(a) ((GtkItemFactoryCallback)(a))

#define NUM_COLS 10


/* convert a port number into a string */
static char *
ett_port_to_str(int port_type, guint32 port)
{
	static int i=0;
	static gchar *strp, str[4][12];

	i++;
	if(i>=4){
		i=0;
	}
	strp=str[i];

	switch(port_type){
	case PT_TCP:
	case PT_UDP:
		g_snprintf(strp, 11, "%d", port);
		return strp;
	}
	return NULL;
}


#define FN_SRC_ADDRESS		0
#define FN_DST_ADDRESS		1
#define FN_ANY_ADDRESS		2
#define FN_SRC_PORT		3
#define FN_DST_PORT		4
#define FN_ANY_PORT		5
/* given an address (to distinguis between ipv4 and ipv6 for tcp/udp
   a port_type and a name_type (FN_...)
   return a string for the filter name

   some addresses, like AT_ETHER may actually be any of multiple types
   of protocols,   either ethernet, tokenring, fddi etc so we must be more 
   specific there  thats why we need specific_addr_type
*/
static char *
ett_get_filter_name(address *addr, int specific_addr_type, int port_type, int name_type)
{
	switch(name_type){
	case FN_SRC_ADDRESS:
		switch(addr->type){
		case AT_ETHER:
			switch(specific_addr_type){
			case SAT_ETHER:
				return "eth.src";
			case SAT_FDDI:
				return "fddi.src";
			case SAT_TOKENRING:
				return "tr.src";
			}
		case AT_IPv4:
			return "ip.src";
		case AT_IPv6:
			return "ipv6.src";
		case AT_IPX:
			return "ipx.src";
		case AT_FC:
			return "fc.s_id";
		default:
			;
		}
	case FN_DST_ADDRESS:
		switch(addr->type){
		case AT_ETHER:
			switch(specific_addr_type){
			case SAT_ETHER:
				return "eth.dst";
			case SAT_FDDI:
				return "fddi.dst";
			case SAT_TOKENRING:
				return "tr.dst";
			}
		case AT_IPv4:
			return "ip.dst";
		case AT_IPv6:
			return "ipv6.dst";
		case AT_IPX:
			return "ipx.dst";
		case AT_FC:
			return "fc.d_id";
		default:
			;
		}
	case FN_ANY_ADDRESS:
		switch(addr->type){
		case AT_ETHER:
			switch(specific_addr_type){
			case SAT_ETHER:
				return "eth.addr";
			case SAT_FDDI:
				return "fddi.addr";
			case SAT_TOKENRING:
				return "tr.addr";
			}
		case AT_IPv4:
			return "ip.addr";
		case AT_IPv6:
			return "ipv6.addr";
		case AT_IPX:
			return "ipx.addr";
		case AT_FC:
			return "fc.id";
		default:
			;
		}
	case FN_SRC_PORT:
		switch(port_type){
		case PT_TCP:
			return "tcp.srcport";
		case PT_UDP:
			return "udp.srcport";
		}
		break;
	case FN_DST_PORT:
		switch(port_type){
		case PT_TCP:
			return "tcp.dstport";
		case PT_UDP:
			return "udp.dstport";
		}
		break;
	case FN_ANY_PORT:
		switch(port_type){
		case PT_TCP:
			return "tcp.port";
		case PT_UDP:
			return "udp.port";
		}
		break;
	}

	g_assert_not_reached();
	return NULL;
}


typedef struct column_arrows {
	GtkWidget *table;
	GtkWidget *ascend_pm;
	GtkWidget *descend_pm;
} column_arrows;



static void
reset_ett_table_data(endpoints_table *et)
{
	guint32 i;
	char title[256];

    if(et->page_lb) {
	    g_snprintf(title, 255, "Conversations: %s", cf_get_display_name(&cfile));
		gtk_window_set_title(GTK_WINDOW(et->win), title);
	    g_snprintf(title, 255, "%s", et->name);
		gtk_label_set_text(GTK_LABEL(et->page_lb), title);
        gtk_widget_set_sensitive(et->page_lb, FALSE);
    } else {
	    g_snprintf(title, 255, "%s Conversations: %s", et->name, cf_get_display_name(&cfile));
		gtk_window_set_title(GTK_WINDOW(et->win), title);
    }

	/* remove all entries from the clist */
	for(i=0;i<et->num_endpoints;i++){
		gtk_clist_remove(et->table, et->num_endpoints-i-1);
	}

	/* delete all endpoints */
	for(i=0;i<et->num_endpoints;i++){
		g_free((gpointer)et->endpoints[i].src_address.data);
		g_free((gpointer)et->endpoints[i].dst_address.data);
	}
	g_free(et->endpoints);
	et->endpoints=NULL;
	et->num_endpoints=0;
}


void protect_thread_critical_region(void);
void unprotect_thread_critical_region(void);
static void
ett_win_destroy_cb(GtkWindow *win _U_, gpointer data)
{
	endpoints_table *talkers=(endpoints_table *)data;

	protect_thread_critical_region();
	remove_tap_listener(talkers);
	unprotect_thread_critical_region();

	reset_ett_table_data(talkers);
	g_free(talkers);
}



static gint
ett_sort_column(GtkCList *clist, gconstpointer ptr1, gconstpointer ptr2)
{
	char *text1 = NULL;
	char *text2 = NULL;
	int i1, i2;

	GtkCListRow *row1 = (GtkCListRow *) ptr1;
	GtkCListRow *row2 = (GtkCListRow *) ptr2;

	text1 = GTK_CELL_TEXT (row1->cell[clist->sort_column])->text;
	text2 = GTK_CELL_TEXT (row2->cell[clist->sort_column])->text;

	switch(clist->sort_column){
	case 0:
	case 2:
		return strcmp (text1, text2);
	case 1:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
		i1=atoi(text1);
		i2=atoi(text2);
		return i1-i2;
	}
	g_assert_not_reached();
	return 0;
}


static void
ett_click_column_cb(GtkCList *clist, gint column, gpointer data)
{
	column_arrows *col_arrows = (column_arrows *) data;
	int i;

	gtk_clist_freeze(clist);

	for (i = 0; i < NUM_COLS; i++) {
		gtk_widget_hide(col_arrows[i].ascend_pm);
		gtk_widget_hide(col_arrows[i].descend_pm);
	}

	if (column == clist->sort_column) {
		if (clist->sort_type == GTK_SORT_ASCENDING) {
			clist->sort_type = GTK_SORT_DESCENDING;
			gtk_widget_show(col_arrows[column].descend_pm);
		} else {
			clist->sort_type = GTK_SORT_ASCENDING;
			gtk_widget_show(col_arrows[column].ascend_pm);
		}
	} else {
		clist->sort_type = GTK_SORT_DESCENDING;
		gtk_widget_show(col_arrows[column].descend_pm);
		gtk_clist_set_sort_column(clist, column);
	}
	gtk_clist_thaw(clist);

	gtk_clist_sort(clist);
}


/* action is encoded as 
   filter_action*65536+filter_type*256+filter_direction

   filter_action:
	0: Match
	1: Prepare
	2: Find Frame
	3:   Find Next
	4:   Find Previous
	5: Colorize Conversation
   filter_type:
	0: Selected
	1: Not Selected
	2: And Selected
	3: Or Selected
	4: And Not Selected
	5: Or Not Selected
   filter_direction:
	0: EP1 To/From EP2
	1: EP1 To EP2
	2: EP1 From EP2
	3: EP1 To/From ANY
	4: EP1 To ANY
	5: EP1 From ANY
	6: EP1 To/From ANY
	7: EP2 To ANY
	8: EP2 From ANY
*/
static void
ett_select_filter_cb(GtkWidget *widget _U_, gpointer callback_data, guint callback_action)
{
	int action, type, direction;
	int selection;
	endpoints_table *et = (endpoints_table *)callback_data;
	char dirstr[128];
	char str[256];
	const char *current_filter;
	char *sport, *dport;

	action=(callback_action>>16)&0xff;
	type=(callback_action>>8)&0xff;
	direction=callback_action&0xff;


	selection=GPOINTER_TO_INT(g_list_nth_data(GTK_CLIST(et->table)->selection, 0));
	if(selection>=(int)et->num_endpoints){
		simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK, "No conversation selected");
		return;
	}
	/* translate it back from row index to index in enndpoint array */
	selection=GPOINTER_TO_INT(gtk_clist_get_row_data(et->table, selection));

	sport=ett_port_to_str(et->endpoints[selection].port_type, et->endpoints[selection].src_port);
	dport=ett_port_to_str(et->endpoints[selection].port_type, et->endpoints[selection].dst_port);

	switch(direction){
	case 0:
		/* EP1 <-> EP2 */
		g_snprintf(dirstr, 127, "%s==%s %s%s%s%s && %s==%s %s%s%s%s",
			ett_get_filter_name(&et->endpoints[selection].src_address, et->endpoints[selection].sat, et->endpoints[selection].port_type,  FN_ANY_ADDRESS),
			address_to_str(&et->endpoints[selection].src_address),
			sport?" && ":"",
			sport?ett_get_filter_name(&et->endpoints[selection].src_address, et->endpoints[selection].sat, et->endpoints[selection].port_type,  FN_ANY_PORT):"",
			sport?"==":"",
			sport?sport:"",
			ett_get_filter_name(&et->endpoints[selection].dst_address, et->endpoints[selection].sat, et->endpoints[selection].port_type,  FN_ANY_ADDRESS),
			address_to_str(&et->endpoints[selection].dst_address),
			dport?" && ":"",
			dport?ett_get_filter_name(&et->endpoints[selection].dst_address, et->endpoints[selection].sat, et->endpoints[selection].port_type,  FN_ANY_PORT):"",
			dport?"==":"",
			dport?dport:""
		);
		break;
	case 1:
		/* EP1 --> EP2 */
		g_snprintf(dirstr, 127, "%s==%s %s%s%s%s && %s==%s %s%s%s%s",
			ett_get_filter_name(&et->endpoints[selection].src_address, et->endpoints[selection].sat, et->endpoints[selection].port_type,  FN_SRC_ADDRESS),
			address_to_str(&et->endpoints[selection].src_address),
			sport?" && ":"",
			sport?ett_get_filter_name(&et->endpoints[selection].src_address, et->endpoints[selection].sat, et->endpoints[selection].port_type,  FN_SRC_PORT):"",
			sport?"==":"",
			sport?sport:"",
			ett_get_filter_name(&et->endpoints[selection].dst_address, et->endpoints[selection].sat, et->endpoints[selection].port_type,  FN_DST_ADDRESS),
			address_to_str(&et->endpoints[selection].dst_address),
			dport?" && ":"",
			dport?ett_get_filter_name(&et->endpoints[selection].dst_address, et->endpoints[selection].sat, et->endpoints[selection].port_type,  FN_DST_PORT):"",
			dport?"==":"",
			dport?dport:""
		);
		break;
	case 2:
		/* EP1 <-- EP2 */
		g_snprintf(dirstr, 127, "%s==%s %s%s%s%s && %s==%s %s%s%s%s",
			ett_get_filter_name(&et->endpoints[selection].src_address, et->endpoints[selection].sat, et->endpoints[selection].port_type,  FN_DST_ADDRESS),
			address_to_str(&et->endpoints[selection].src_address),
			sport?" && ":"",
			sport?ett_get_filter_name(&et->endpoints[selection].src_address, et->endpoints[selection].sat, et->endpoints[selection].port_type,  FN_DST_PORT):"",
			sport?"==":"",
			sport?sport:"",
			ett_get_filter_name(&et->endpoints[selection].dst_address, et->endpoints[selection].sat, et->endpoints[selection].port_type,  FN_SRC_ADDRESS),
			address_to_str(&et->endpoints[selection].dst_address),
			dport?" && ":"",
			dport?ett_get_filter_name(&et->endpoints[selection].dst_address, et->endpoints[selection].sat, et->endpoints[selection].port_type,  FN_SRC_PORT):"",
			dport?"==":"",
			dport?dport:""
		);
		break;
	case 3:
		/* EP1 <-> ANY */
		g_snprintf(dirstr, 127, "%s==%s %s%s%s%s",
			ett_get_filter_name(&et->endpoints[selection].src_address, et->endpoints[selection].sat, et->endpoints[selection].port_type,  FN_ANY_ADDRESS),
			address_to_str(&et->endpoints[selection].src_address),
			sport?" && ":"",
			sport?ett_get_filter_name(&et->endpoints[selection].src_address, et->endpoints[selection].sat, et->endpoints[selection].port_type,  FN_ANY_PORT):"",
			sport?"==":"",
			sport?sport:""
		);
		break;
	case 4:
		/* EP1 --> ANY */
		g_snprintf(dirstr, 127, "%s==%s %s%s%s%s",
			ett_get_filter_name(&et->endpoints[selection].src_address, et->endpoints[selection].sat, et->endpoints[selection].port_type,  FN_SRC_ADDRESS),
			address_to_str(&et->endpoints[selection].src_address),
			sport?" && ":"",
			sport?ett_get_filter_name(&et->endpoints[selection].src_address, et->endpoints[selection].sat, et->endpoints[selection].port_type,  FN_SRC_PORT):"",
			sport?"==":"",
			sport?sport:""
		);
		break;
	case 5:
		/* EP1 <-- ANY */
		g_snprintf(dirstr, 127, "%s==%s %s%s%s%s",
			ett_get_filter_name(&et->endpoints[selection].src_address, et->endpoints[selection].sat, et->endpoints[selection].port_type,  FN_DST_ADDRESS),
			address_to_str(&et->endpoints[selection].src_address),
			sport?" && ":"",
			sport?ett_get_filter_name(&et->endpoints[selection].src_address, et->endpoints[selection].sat, et->endpoints[selection].port_type,  FN_DST_PORT):"",
			sport?"==":"",
			sport?sport:""
		);
		break;
	case 6:
		/* EP2 <-> ANY */
		g_snprintf(dirstr, 127, "%s==%s %s%s%s%s",
			ett_get_filter_name(&et->endpoints[selection].dst_address, et->endpoints[selection].sat, et->endpoints[selection].port_type,  FN_ANY_ADDRESS),
			address_to_str(&et->endpoints[selection].dst_address),
			dport?" && ":"",
			dport?ett_get_filter_name(&et->endpoints[selection].dst_address, et->endpoints[selection].sat, et->endpoints[selection].port_type,  FN_ANY_PORT):"",
			dport?"==":"",
			dport?dport:""
		);
		break;
	case 7:
		/* EP2 --> ANY */
		g_snprintf(dirstr, 127, "%s==%s %s%s%s%s",
			ett_get_filter_name(&et->endpoints[selection].dst_address, et->endpoints[selection].sat, et->endpoints[selection].port_type,  FN_SRC_ADDRESS),
			address_to_str(&et->endpoints[selection].dst_address),
			dport?" && ":"",
			dport?ett_get_filter_name(&et->endpoints[selection].dst_address, et->endpoints[selection].sat, et->endpoints[selection].port_type,  FN_SRC_PORT):"",
			dport?"==":"",
			dport?dport:""
		);
		break;
	case 8:
		/* EP2 <-- ANY */
		g_snprintf(dirstr, 127, "%s==%s %s%s%s%s",
			ett_get_filter_name(&et->endpoints[selection].dst_address, et->endpoints[selection].sat, et->endpoints[selection].port_type,  FN_DST_ADDRESS),
			address_to_str(&et->endpoints[selection].dst_address),
			dport?" && ":"",
			dport?ett_get_filter_name(&et->endpoints[selection].dst_address, et->endpoints[selection].sat, et->endpoints[selection].port_type,  FN_DST_PORT):"",
			dport?"==":"",
			dport?dport:""
		);
		break;
	}

	current_filter=gtk_entry_get_text(GTK_ENTRY(main_display_filter_widget));
	switch(type){
	case 0:
		/* selected */
		g_snprintf(str, 255, "%s", dirstr);
		break;
	case 1:
		/* not selected */
		g_snprintf(str, 255, "!(%s)", dirstr);
		break;
	case 2:
		/* and selected */
		g_snprintf(str, 255, "(%s) && (%s)", current_filter, dirstr);
		break;
	case 3:
		/* or selected */
		g_snprintf(str, 255, "(%s) || (%s)", current_filter, dirstr);
		break;
	case 4:
		/* and not selected */
		g_snprintf(str, 255, "(%s) && !(%s)", current_filter, dirstr);
		break;
	case 5:
		/* or not selected */
		g_snprintf(str, 255, "(%s) || !(%s)", current_filter, dirstr);
		break;
	}

	switch(action){
	case 0:
		/* match */
		gtk_entry_set_text(GTK_ENTRY(main_display_filter_widget), str);
		main_filter_packets(&cfile, str, FALSE);
		gdk_window_raise(top_level->window);
		break;
	case 1:
		/* prepare */
		gtk_entry_set_text(GTK_ENTRY(main_display_filter_widget), str);
		break;
	case 2:
		/* find frame */
		find_frame_with_filter(str);
		break;
	case 3:
		/* find next */
		find_previous_next_frame_with_filter(str, FALSE);
		break;
	case 4:
		/* find previous */
		find_previous_next_frame_with_filter(str, TRUE);
		break;
	case 5:
		/* colorize conversation */
		color_display_with_filter(str);
		break;
	}

}

static gint
ett_show_popup_menu_cb(void *widg _U_, GdkEvent *event, endpoints_table *et)
{
	GdkEventButton *bevent = (GdkEventButton *)event;

	if(event->type==GDK_BUTTON_PRESS && bevent->button==3){
		gtk_menu_popup(GTK_MENU(et->menu), NULL, NULL, NULL, NULL, 
			bevent->button, bevent->time);
	}

	return FALSE;
}

static GtkItemFactoryEntry ett_list_menu_items[] =
{
	/* Match */
	ITEM_FACTORY_ENTRY("/Match Display Filter", NULL, NULL, 0, "<Branch>", NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/Selected", NULL, NULL, 0, "<Branch>", NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/Selected/EP1 <-> EP2", NULL,
		ett_select_filter_cb, 0*65536+0*256+0, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/Selected/EP1 --> EP2", NULL,
		ett_select_filter_cb, 0*65536+0*256+1, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/Selected/EP1 <-- EP2", NULL,
		ett_select_filter_cb, 0*65536+0*256+2, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/Selected/EP1 <-> ANY", NULL,
		ett_select_filter_cb, 0*65536+0*256+3, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/Selected/EP1 --> ANY", NULL,
		ett_select_filter_cb, 0*65536+0*256+4, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/Selected/EP1 <-- ANY", NULL,
		ett_select_filter_cb, 0*65536+0*256+5, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/Selected/ANY <-> EP2", NULL,
		ett_select_filter_cb, 0*65536+0*256+6, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/Selected/ANY <-- EP2", NULL,
		ett_select_filter_cb, 0*65536+0*256+7, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/Selected/ANY --> EP2", NULL,
		ett_select_filter_cb, 0*65536+0*256+8, NULL, NULL),

	ITEM_FACTORY_ENTRY("/Match Display Filter/Not Selected", NULL, NULL, 0, "<Branch>", NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/Not Selected/EP1 <-> EP2", NULL,
		ett_select_filter_cb, 0*65536+1*256+0, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/Not Selected/EP1 --> EP2", NULL,
		ett_select_filter_cb, 0*65536+1*256+1, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/Not Selected/EP1 <-- EP2", NULL,
		ett_select_filter_cb, 0*65536+1*256+2, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/Not Selected/EP1 --> ANY", NULL,
		ett_select_filter_cb, 0*65536+1*256+3, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/Not Selected/EP1 <-> ANY", NULL,
		ett_select_filter_cb, 0*65536+1*256+4, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/Not Selected/EP1 <-- ANY", NULL,
		ett_select_filter_cb, 0*65536+1*256+5, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/Not Selected/ANY <-> EP2", NULL,
		ett_select_filter_cb, 0*65536+1*256+6, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/Not Selected/ANY <-- EP2", NULL,
		ett_select_filter_cb, 0*65536+1*256+7, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/Not Selected/ANY --> EP2", NULL,
		ett_select_filter_cb, 0*65536+1*256+8, NULL, NULL),


	ITEM_FACTORY_ENTRY("/Match Display Filter/And Selected", NULL, NULL, 0, "<Branch>", NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/And Selected/EP1 <-> EP2", NULL,
		ett_select_filter_cb, 0*65536+2*256+0, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/And Selected/EP1 --> EP2", NULL,
		ett_select_filter_cb, 0*65536+2*256+1, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/And Selected/EP1 <-- EP2", NULL,
		ett_select_filter_cb, 0*65536+2*256+2, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/And Selected/EP1 <-> ANY", NULL,
		ett_select_filter_cb, 0*65536+2*256+3, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/And Selected/EP1 --> ANY", NULL,
		ett_select_filter_cb, 0*65536+2*256+4, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/And Selected/EP1 <-- ANY", NULL,
		ett_select_filter_cb, 0*65536+2*256+5, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/And Selected/ANY <-> EP2", NULL,
		ett_select_filter_cb, 0*65536+2*256+6, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/And Selected/ANY <-- EP2", NULL,
		ett_select_filter_cb, 0*65536+2*256+7, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/And Selected/ANY --> EP2", NULL,
		ett_select_filter_cb, 0*65536+2*256+8, NULL, NULL),

	ITEM_FACTORY_ENTRY("/Match Display Filter/Or Selected", NULL, NULL, 0, "<Branch>", NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/Or Selected/EP1 <-> EP2", NULL,
		ett_select_filter_cb, 0*65536+3*256+0, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/Or Selected/EP1 --> EP2", NULL,
		ett_select_filter_cb, 0*65536+3*256+1, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/Or Selected/EP1 <-- EP2", NULL,
		ett_select_filter_cb, 0*65536+3*256+2, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/Or Selected/EP1 <-> ANY", NULL,
		ett_select_filter_cb, 0*65536+3*256+3, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/Or Selected/EP1 --> ANY", NULL,
		ett_select_filter_cb, 0*65536+3*256+4, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/Or Selected/EP1 <-- ANY", NULL,
		ett_select_filter_cb, 0*65536+3*256+5, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/Or Selected/ANY <-> EP2", NULL,
		ett_select_filter_cb, 0*65536+3*256+6, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/Or Selected/ANY <-- EP2", NULL,
		ett_select_filter_cb, 0*65536+3*256+7, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/Or Selected/ANY --> EP2", NULL,
		ett_select_filter_cb, 0*65536+3*256+8, NULL, NULL),

	ITEM_FACTORY_ENTRY("/Match Display Filter/And Not Selected", NULL, NULL, 0, "<Branch>", NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/And Not Selected/EP1 <-> EP2", NULL,
		ett_select_filter_cb, 0*65536+4*256+0, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/And Not Selected/EP1 --> EP2", NULL,
		ett_select_filter_cb, 0*65536+4*256+1, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/And Not Selected/EP1 <-- EP2", NULL,
		ett_select_filter_cb, 0*65536+4*256+2, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/And Not Selected/EP1 <-> ANY", NULL,
		ett_select_filter_cb, 0*65536+4*256+3, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/And Not Selected/EP1 --> ANY", NULL,
		ett_select_filter_cb, 0*65536+4*256+4, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/And Not Selected/EP1 <-- ANY", NULL,
		ett_select_filter_cb, 0*65536+4*256+5, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/And Not Selected/ANY <-> EP2", NULL,
		ett_select_filter_cb, 0*65536+4*256+6, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/And Not Selected/ANY <-- EP2", NULL,
		ett_select_filter_cb, 0*65536+4*256+7, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/And Not Selected/ANY --> EP2", NULL,
		ett_select_filter_cb, 0*65536+4*256+8, NULL, NULL),

	ITEM_FACTORY_ENTRY("/Match Display Filter/Or Not Selected", NULL, NULL, 0, "<Branch>", NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/Or Not Selected/EP1 <-> EP2", NULL,
		ett_select_filter_cb, 0*65536+5*256+0, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/Or Not Selected/EP1 --> EP2", NULL,
		ett_select_filter_cb, 0*65536+5*256+1, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/Or Not Selected/EP1 <-- EP2", NULL,
		ett_select_filter_cb, 0*65536+5*256+2, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/Or Not Selected/EP1 <-> ANY", NULL,
		ett_select_filter_cb, 0*65536+5*256+3, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/Or Not Selected/EP1 --> ANY", NULL,
		ett_select_filter_cb, 0*65536+5*256+4, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/Or Not Selected/EP1 <-- ANY", NULL,
		ett_select_filter_cb, 0*65536+5*256+5, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/Or Not Selected/ANY <-> EP2", NULL,
		ett_select_filter_cb, 0*65536+5*256+6, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/Or Not Selected/ANY <-- EP2", NULL,
		ett_select_filter_cb, 0*65536+5*256+7, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Match Display Filter/Or Not Selected/ANY --> EP2", NULL,
		ett_select_filter_cb, 0*65536+5*256+8, NULL, NULL),

	/* Prepare */
	ITEM_FACTORY_ENTRY("/Prepare Display Filter", NULL, NULL, 0, "<Branch>", NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/Selected", NULL, NULL, 0, "<Branch>", NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/Selected/EP1 <-> EP2", NULL,
		ett_select_filter_cb, 1*65536+0*256+0, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/Selected/EP1 --> EP2", NULL,
		ett_select_filter_cb, 1*65536+0*256+1, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/Selected/EP1 <-- EP2", NULL,
		ett_select_filter_cb, 1*65536+0*256+2, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/Selected/EP1 <-> ANY", NULL,
		ett_select_filter_cb, 1*65536+0*256+3, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/Selected/EP1 --> ANY", NULL,
		ett_select_filter_cb, 1*65536+0*256+4, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/Selected/EP1 <-- ANY", NULL,
		ett_select_filter_cb, 1*65536+0*256+5, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/Selected/ANY <-> EP2", NULL,
		ett_select_filter_cb, 1*65536+0*256+6, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/Selected/ANY <-- EP2", NULL,
		ett_select_filter_cb, 1*65536+0*256+7, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/Selected/ANY --> EP2", NULL,
		ett_select_filter_cb, 1*65536+0*256+8, NULL, NULL),

	ITEM_FACTORY_ENTRY("/Prepare Display Filter/Not Selected", NULL, NULL, 0, "<Branch>", NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/Not Selected/EP1 <-> EP2", NULL,
		ett_select_filter_cb, 1*65536+1*256+0, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/Not Selected/EP1 --> EP2", NULL,
		ett_select_filter_cb, 1*65536+1*256+1, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/Not Selected/EP1 <-- EP2", NULL,
		ett_select_filter_cb, 1*65536+1*256+2, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/Not Selected/EP1 <-> ANY", NULL,
		ett_select_filter_cb, 1*65536+1*256+3, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/Not Selected/EP1 --> ANY", NULL,
		ett_select_filter_cb, 1*65536+1*256+4, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/Not Selected/EP1 <-- ANY", NULL,
		ett_select_filter_cb, 1*65536+1*256+5, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/Not Selected/ANY <-> EP2", NULL,
		ett_select_filter_cb, 1*65536+1*256+6, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/Not Selected/ANY <-- EP2", NULL,
		ett_select_filter_cb, 1*65536+1*256+7, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/Not Selected/ANY --> EP2", NULL,
		ett_select_filter_cb, 1*65536+1*256+8, NULL, NULL),

	ITEM_FACTORY_ENTRY("/Prepare Display Filter/And Selected", NULL, NULL, 0, "<Branch>", NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/And Selected/EP1 <-> EP2", NULL,
		ett_select_filter_cb, 1*65536+2*256+0, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/And Selected/EP1 --> EP2", NULL,
		ett_select_filter_cb, 1*65536+2*256+1, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/And Selected/EP1 <-- EP2", NULL,
		ett_select_filter_cb, 1*65536+2*256+2, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/And Selected/EP1 <-> ANY", NULL,
		ett_select_filter_cb, 1*65536+2*256+3, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/And Selected/EP1 --> ANY", NULL,
		ett_select_filter_cb, 1*65536+2*256+4, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/And Selected/EP1 <-- ANY", NULL,
		ett_select_filter_cb, 1*65536+2*256+5, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/And Selected/ANY <-> EP2", NULL,
		ett_select_filter_cb, 1*65536+2*256+6, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/And Selected/ANY <-- EP2", NULL,
		ett_select_filter_cb, 1*65536+2*256+7, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/And Selected/ANY --> EP2", NULL,
		ett_select_filter_cb, 1*65536+2*256+8, NULL, NULL),

	ITEM_FACTORY_ENTRY("/Prepare Display Filter/Or Selected", NULL, NULL, 0, "<Branch>", NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/Or Selected/EP1 <-> EP2", NULL,
		ett_select_filter_cb, 1*65536+3*256+0, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/Or Selected/EP1 --> EP2", NULL,
		ett_select_filter_cb, 1*65536+3*256+1, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/Or Selected/EP1 <-- EP2", NULL,
		ett_select_filter_cb, 1*65536+3*256+2, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/Or Selected/EP1 <-> ANY", NULL,
		ett_select_filter_cb, 1*65536+3*256+3, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/Or Selected/EP1 --> ANY", NULL,
		ett_select_filter_cb, 1*65536+3*256+4, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/Or Selected/EP1 <-- ANY", NULL,
		ett_select_filter_cb, 1*65536+3*256+5, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/Or Selected/ANY <-> EP2", NULL,
		ett_select_filter_cb, 1*65536+3*256+6, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/Or Selected/ANY <-- EP2", NULL,
		ett_select_filter_cb, 1*65536+3*256+7, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/Or Selected/ANY --> EP2", NULL,
		ett_select_filter_cb, 1*65536+3*256+8, NULL, NULL),

	ITEM_FACTORY_ENTRY("/Prepare Display Filter/And Not Selected", NULL, NULL, 0, "<Branch>", NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/And Not Selected/EP1 <-> EP2", NULL,
		ett_select_filter_cb, 1*65536+4*256+0, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/And Not Selected/EP1 --> EP2", NULL,
		ett_select_filter_cb, 1*65536+4*256+1, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/And Not Selected/EP1 <-- EP2", NULL,
		ett_select_filter_cb, 1*65536+4*256+2, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/And Not Selected/EP1 <-> ANY", NULL,
		ett_select_filter_cb, 1*65536+4*256+3, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/And Not Selected/EP1 --> ANY", NULL,
		ett_select_filter_cb, 1*65536+4*256+4, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/And Not Selected/EP1 <-- ANY", NULL,
		ett_select_filter_cb, 1*65536+4*256+5, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/And Not Selected/ANY <-> EP2", NULL,
		ett_select_filter_cb, 1*65536+4*256+6, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/And Not Selected/ANY <-- EP2", NULL,
		ett_select_filter_cb, 1*65536+4*256+7, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/And Not Selected/ANY --> EP2", NULL,
		ett_select_filter_cb, 1*65536+4*256+8, NULL, NULL),

	ITEM_FACTORY_ENTRY("/Prepare Display Filter/Or Not Selected", NULL, NULL, 0, "<Branch>", NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/Or Not Selected/EP1 <-> EP2", NULL,
		ett_select_filter_cb, 1*65536+5*256+0, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/Or Not Selected/EP1 --> EP2", NULL,
		ett_select_filter_cb, 1*65536+5*256+1, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/Or Not Selected/EP1 <-- EP2", NULL,
		ett_select_filter_cb, 1*65536+5*256+2, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/Or Not Selected/EP1 <-> ANY", NULL,
		ett_select_filter_cb, 1*65536+5*256+3, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/Or Not Selected/EP1 --> ANY", NULL,
		ett_select_filter_cb, 1*65536+5*256+4, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/Or Not Selected/EP1 <-- ANY", NULL,
		ett_select_filter_cb, 1*65536+5*256+5, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/Or Not Selected/ANY <-> EP2", NULL,
		ett_select_filter_cb, 1*65536+5*256+6, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/Or Not Selected/ANY <-- EP2", NULL,
		ett_select_filter_cb, 1*65536+5*256+7, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Prepare Display Filter/Or Not Selected/ANY --> EP2", NULL,
		ett_select_filter_cb, 1*65536+5*256+8, NULL, NULL),

	/* Find Frame */
	ITEM_FACTORY_ENTRY("/Find Frame", NULL, NULL, 0, "<Branch>", NULL),
	ITEM_FACTORY_ENTRY("/Find Frame/Find Frame", NULL, NULL, 0, "<Branch>", NULL),
	ITEM_FACTORY_ENTRY("/Find Frame/Find Frame/EP1 <-> EP2", NULL,
		ett_select_filter_cb, 2*65536+0*256+0, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Find Frame/Find Frame/EP1 --> EP2", NULL,
		ett_select_filter_cb, 2*65536+0*256+1, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Find Frame/Find Frame/EP1 <-- EP2", NULL,
		ett_select_filter_cb, 2*65536+0*256+2, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Find Frame/Find Frame/EP1 <-> ANY", NULL,
		ett_select_filter_cb, 2*65536+0*256+3, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Find Frame/Find Frame/EP1 --> ANY", NULL,
		ett_select_filter_cb, 2*65536+0*256+4, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Find Frame/Find Frame/EP1 <-- ANY", NULL,
		ett_select_filter_cb, 2*65536+0*256+5, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Find Frame/Find Frame/ANY <-> EP2", NULL,
		ett_select_filter_cb, 2*65536+0*256+6, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Find Frame/Find Frame/ANY <-- EP2", NULL,
		ett_select_filter_cb, 2*65536+0*256+7, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Find Frame/Find Frame/ANY --> EP2", NULL,
		ett_select_filter_cb, 2*65536+0*256+8, NULL, NULL),
	/* Find Next */
	ITEM_FACTORY_ENTRY("/Find Frame/Find Next", NULL, NULL, 0, "<Branch>", NULL),
	ITEM_FACTORY_ENTRY("/Find Frame/Find Next/EP1 <-> EP2", NULL,
		ett_select_filter_cb, 3*65536+0*256+0, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Find Frame/Find Next/EP1 --> EP2", NULL,
		ett_select_filter_cb, 3*65536+0*256+1, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Find Frame/Find Next/EP1 <-- EP2", NULL,
		ett_select_filter_cb, 3*65536+0*256+2, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Find Frame/Find Next/EP1 <-> ANY", NULL,
		ett_select_filter_cb, 3*65536+0*256+3, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Find Frame/Find Next/EP1 --> ANY", NULL,
		ett_select_filter_cb, 3*65536+0*256+4, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Find Frame/Find Next/EP1 <-- ANY", NULL,
		ett_select_filter_cb, 3*65536+0*256+5, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Find Frame/Find Next/ANY <-> EP2", NULL,
		ett_select_filter_cb, 3*65536+0*256+6, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Find Frame/Find Next/ANY <-- EP2", NULL,
		ett_select_filter_cb, 3*65536+0*256+7, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Find Frame/Find Next/ANY --> EP2", NULL,
		ett_select_filter_cb, 3*65536+0*256+8, NULL, NULL),
	/* Find Previous */
	ITEM_FACTORY_ENTRY("/Find Frame/Find Previous", NULL, NULL, 0, "<Branch>", NULL),
	ITEM_FACTORY_ENTRY("/Find Frame/Find Previous/EP1 <-> EP2", NULL,
		ett_select_filter_cb, 4*65536+0*256+0, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Find Frame/Find Previous/EP1 --> EP2", NULL,
		ett_select_filter_cb, 4*65536+0*256+1, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Find Frame/Find Previous/EP1 <-- EP2", NULL,
		ett_select_filter_cb, 4*65536+0*256+2, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Find Frame/Find Previous/EP1 <-> ANY", NULL,
		ett_select_filter_cb, 4*65536+0*256+3, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Find Frame/Find Previous/EP1 --> ANY", NULL,
		ett_select_filter_cb, 4*65536+0*256+4, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Find Frame/Find Previous/EP1 <-- ANY", NULL,
		ett_select_filter_cb, 4*65536+0*256+5, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Find Frame/Find Previous/ANY <-> EP2", NULL,
		ett_select_filter_cb, 4*65536+0*256+6, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Find Frame/Find Previous/ANY <-- EP2", NULL,
		ett_select_filter_cb, 4*65536+0*256+7, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Find Frame/Find Previous/ANY --> EP2", NULL,
		ett_select_filter_cb, 4*65536+0*256+8, NULL, NULL),
	/* Colorize Conversation */
	ITEM_FACTORY_ENTRY("/Colorize Conversation", NULL, NULL, 0, "<Branch>", NULL),
	ITEM_FACTORY_ENTRY("/Colorize Conversation/EP1 <-> EP2", NULL,
		ett_select_filter_cb, 5*65536+0*256+0, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Colorize Conversation/EP1 --> EP2", NULL,
		ett_select_filter_cb, 5*65536+0*256+1, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Colorize Conversation/EP1 <-- EP2", NULL,
		ett_select_filter_cb, 5*65536+0*256+2, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Colorize Conversation/EP1 <-> ANY", NULL,
		ett_select_filter_cb, 5*65536+0*256+3, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Colorize Conversation/EP1 --> ANY", NULL,
		ett_select_filter_cb, 5*65536+0*256+4, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Colorize Conversation/EP1 <-- ANY", NULL,
		ett_select_filter_cb, 5*65536+0*256+5, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Colorize Conversation/ANY <-> EP2", NULL,
		ett_select_filter_cb, 5*65536+0*256+6, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Colorize Conversation/ANY <-- EP2", NULL,
		ett_select_filter_cb, 5*65536+0*256+7, NULL, NULL),
	ITEM_FACTORY_ENTRY("/Colorize Conversation/ANY --> EP2", NULL,
		ett_select_filter_cb, 5*65536+0*256+8, NULL, NULL),


};

static void
ett_create_popup_menu(endpoints_table *et)
{
	et->item_factory = gtk_item_factory_new(GTK_TYPE_MENU, "<main>", NULL);

	gtk_item_factory_create_items_ac(et->item_factory, sizeof(ett_list_menu_items)/sizeof(ett_list_menu_items[0]), ett_list_menu_items, et, 2);

	et->menu = gtk_item_factory_get_widget(et->item_factory, "<main>");
	SIGNAL_CONNECT(et->table, "button_press_event", ett_show_popup_menu_cb, et);
}


/* XXX should freeze/thaw table here and in the srt thingy? */
static void 
draw_ett_table_addresses(endpoints_table *et)
{
	guint32 i;
	int j;


	for(i=0;i<et->num_endpoints;i++){
		char *entry;
		char *port;
        address_type  at;
        guint32 pt;

		j=gtk_clist_find_row_from_data(et->table, (gpointer)i);

        at = et->endpoints[i].src_address.type;
        if(!et->resolve_names) at = AT_NONE;
        switch(at) {
        case(AT_IPv4):
            entry=get_hostname((*(guint *)et->endpoints[i].src_address.data));
            break;
        case(AT_ETHER):
            entry=get_ether_name(et->endpoints[i].src_address.data);
            break;
        default:
            entry=address_to_str(&et->endpoints[i].src_address);
        }
		gtk_clist_set_text(et->table, j, 0, entry);

        pt = et->endpoints[i].port_type;
        if(!et->resolve_names) pt = PT_NONE;
        switch(pt) {
        case(PT_TCP):
            entry=get_tcp_port(et->endpoints[i].src_port);
            break;
        case(PT_UDP):
            entry=get_udp_port(et->endpoints[i].src_port);
            break;
        default:
		    port=ett_port_to_str(et->endpoints[i].port_type, et->endpoints[i].src_port);
		    entry=port?port:"";
        }
		gtk_clist_set_text(et->table, j, 1, entry);

        at = et->endpoints[i].dst_address.type;
        if(!et->resolve_names) at = AT_NONE;
        switch(at) {
        case(AT_IPv4):
            entry=get_hostname((*(guint *)et->endpoints[i].dst_address.data));
            break;
        case(AT_ETHER):
            entry=get_ether_name(et->endpoints[i].dst_address.data);
            break;
        default:
		    entry=address_to_str(&et->endpoints[i].dst_address);
        }
		gtk_clist_set_text(et->table, j, 2, entry);

        switch(pt) {
        case(PT_TCP):
            entry=get_tcp_port(et->endpoints[i].dst_port);
            break;
        case(PT_UDP):
            entry=get_udp_port(et->endpoints[i].dst_port);
            break;
        default:
    		port=ett_port_to_str(et->endpoints[i].port_type, et->endpoints[i].dst_port);
            entry=port?port:"";
        }
		gtk_clist_set_text(et->table, j, 3, entry);
	}
}



/* XXX should freeze/thaw table here and in the srt thingy? */
static void 
draw_ett_table_data(endpoints_table *et)
{
	guint32 i;
	int j;
	char title[256];

    if (et->page_lb) {
        if(et->num_endpoints) {
	        g_snprintf(title, 255, "%s: %u", et->name, et->num_endpoints);
        } else {
	        g_snprintf(title, 255, "%s", et->name);
        }
		gtk_label_set_text(GTK_LABEL(et->page_lb), title);
        gtk_widget_set_sensitive(et->page_lb, et->num_endpoints);
    }

	for(i=0;i<et->num_endpoints;i++){
		char str[16];

		j=gtk_clist_find_row_from_data(et->table, (gpointer)i);

		g_snprintf(str, 16, "%u", et->endpoints[i].tx_frames+et->endpoints[i].rx_frames);
		gtk_clist_set_text(et->table, j, 4, str);		
		g_snprintf(str, 16, "%u", et->endpoints[i].tx_bytes+et->endpoints[i].rx_bytes);
		gtk_clist_set_text(et->table, j, 5, str);		


		g_snprintf(str, 16, "%u", et->endpoints[i].tx_frames);
		gtk_clist_set_text(et->table, j, 6, str);	
		g_snprintf(str, 16, "%u", et->endpoints[i].tx_bytes);
		gtk_clist_set_text(et->table, j, 7, str);		


		g_snprintf(str, 16, "%u", et->endpoints[i].rx_frames);
		gtk_clist_set_text(et->table, j, 8, str);		
		g_snprintf(str, 16, "%u", et->endpoints[i].rx_bytes);
		gtk_clist_set_text(et->table, j, 9, str);		

	}
	gtk_clist_sort(et->table);

    /* update table, so resolved addresses will be shown now */
    draw_ett_table_addresses(et);
}


gboolean
init_ett_table_page(endpoints_table *talkers, GtkWidget *vbox, gboolean hide_ports, char *table_name, char *tap_name, char *filter, void *packet_func)
{
	int i;
	column_arrows *col_arrows;
	GtkStyle *win_style;
	GtkWidget *column_lb;
	GString *error_string;
	GtkWidget *label;
	char title[256];
	char *default_titles[] = { "EP1 Address", "Port", "EP2 Address", "Port", "Frames", "Bytes", "-> Frames", "-> Bytes", "<- Frames", "<- Bytes" };


	g_snprintf(title, 255, "%s Conversations", table_name);
	label=gtk_label_new(title);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);


	talkers->scrolled_window=scrolled_window_new(NULL, NULL);
	gtk_box_pack_start(GTK_BOX(vbox), talkers->scrolled_window, TRUE, TRUE, 0);

	talkers->table=(GtkCList *)gtk_clist_new(NUM_COLS);

	col_arrows = (column_arrows *) g_malloc(sizeof(column_arrows) * NUM_COLS);
	win_style = gtk_widget_get_style(talkers->scrolled_window);
	for (i = 0; i < NUM_COLS; i++) {
		col_arrows[i].table = gtk_table_new(2, 2, FALSE);
		gtk_table_set_col_spacings(GTK_TABLE(col_arrows[i].table), 5);
		column_lb = gtk_label_new(default_titles[i]);
		gtk_table_attach(GTK_TABLE(col_arrows[i].table), column_lb, 0, 1, 0, 2, GTK_SHRINK, GTK_SHRINK, 0, 0);
		gtk_widget_show(column_lb);

        col_arrows[i].ascend_pm = xpm_to_widget((const char **) clist_ascend_xpm);
		gtk_table_attach(GTK_TABLE(col_arrows[i].table), col_arrows[i].ascend_pm, 1, 2, 1, 2, GTK_SHRINK, GTK_SHRINK, 0, 0);
		col_arrows[i].descend_pm = xpm_to_widget((const char **) clist_descend_xpm);
		gtk_table_attach(GTK_TABLE(col_arrows[i].table), col_arrows[i].descend_pm, 1, 2, 0, 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
		/* make total frames be the default sort order */
		if (i == 4) {
			gtk_widget_show(col_arrows[i].descend_pm);
		}
		gtk_clist_set_column_widget(GTK_CLIST(talkers->table), i, col_arrows[i].table);
		gtk_widget_show(col_arrows[i].table);
	}
	gtk_clist_column_titles_show(GTK_CLIST(talkers->table));

	gtk_clist_set_compare_func(talkers->table, ett_sort_column);
	gtk_clist_set_sort_column(talkers->table, 4);
	gtk_clist_set_sort_type(talkers->table, GTK_SORT_DESCENDING);


    gtk_clist_set_column_auto_resize(talkers->table, 0, TRUE);
    gtk_clist_set_column_auto_resize(talkers->table, 1, TRUE);
    gtk_clist_set_column_auto_resize(talkers->table, 2, TRUE);
    gtk_clist_set_column_auto_resize(talkers->table, 3, TRUE);
    gtk_clist_set_column_auto_resize(talkers->table, 4, TRUE);
    gtk_clist_set_column_auto_resize(talkers->table, 5, TRUE);
    gtk_clist_set_column_auto_resize(talkers->table, 6, TRUE);
    gtk_clist_set_column_auto_resize(talkers->table, 7, TRUE);
    gtk_clist_set_column_auto_resize(talkers->table, 8, TRUE);
    gtk_clist_set_column_auto_resize(talkers->table, 9, TRUE);

#if 0
	/*XXX instead of this we should probably have some code to
		dynamically adjust the width of the columns */
	gtk_clist_set_column_width(talkers->table, 0, 100);
	gtk_clist_set_column_width(talkers->table, 1, 40);
	gtk_clist_set_column_width(talkers->table, 2, 100);
	gtk_clist_set_column_width(talkers->table, 3, 40);
	gtk_clist_set_column_width(talkers->table, 4, 70);
	gtk_clist_set_column_width(talkers->table, 5, 60);
	gtk_clist_set_column_width(talkers->table, 6, 70);
	gtk_clist_set_column_width(talkers->table, 7, 60);
	gtk_clist_set_column_width(talkers->table, 8, 70);
	gtk_clist_set_column_width(talkers->table, 9, 60);
#endif

	gtk_clist_set_shadow_type(talkers->table, GTK_SHADOW_IN);
	gtk_clist_column_titles_show(talkers->table);
	gtk_container_add(GTK_CONTAINER(talkers->scrolled_window), (GtkWidget *)talkers->table);

	SIGNAL_CONNECT(talkers->table, "click-column", ett_click_column_cb, col_arrows);

	talkers->num_endpoints=0;
	talkers->endpoints=NULL;

	/* hide srcport and dstport if we don't use ports */
	if(hide_ports){
		gtk_clist_set_column_visibility(talkers->table, 1, FALSE);
		gtk_clist_set_column_visibility(talkers->table, 3, FALSE);
	}

	/* create popup menu for this table */
	ett_create_popup_menu(talkers);


	/* register the tap and rerun the taps on the packet list */
	error_string=register_tap_listener(tap_name, talkers, filter, (void *)reset_ett_table_data, packet_func, (void *)draw_ett_table_data);
	if(error_string){
		simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK, error_string->str);
		g_string_free(error_string, TRUE);
		return FALSE;
	}

    return TRUE;
}


void
init_ett_table(gboolean hide_ports, char *table_name, char *tap_name, char *filter, void *packet_func)
{
	endpoints_table *talkers;
	char title[256];
    GtkWidget *vbox;
    GtkWidget *bbox;
    GtkWidget *close_bt;
    gboolean ret;

    
    talkers=g_malloc(sizeof(endpoints_table));

	talkers->name=table_name;
	g_snprintf(title, 255, "%s Conversations: %s", table_name, cf_get_display_name(&cfile));
	talkers->win=dlg_window_new(title);
	talkers->page_lb=NULL;
    talkers->resolve_names=TRUE;
    gtk_window_set_default_size(GTK_WINDOW(talkers->win), 750, 400);

	SIGNAL_CONNECT(talkers->win, "destroy", ett_win_destroy_cb, talkers);

	vbox=gtk_vbox_new(FALSE, 3);
	gtk_container_add(GTK_CONTAINER(talkers->win), vbox);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);

    ret = init_ett_table_page(talkers, vbox, hide_ports, table_name, tap_name, filter, packet_func);
    if(ret == FALSE) {
		g_free(talkers);
        return;
    }

	/* Button row. */
	bbox = dlg_button_row_new(GTK_STOCK_CLOSE, NULL);
	gtk_box_pack_end(GTK_BOX(vbox), bbox, FALSE, FALSE, 0);

	close_bt = OBJECT_GET_DATA(bbox, GTK_STOCK_CLOSE);
	SIGNAL_CONNECT_OBJECT(close_bt, "clicked", gtk_widget_destroy, talkers->win);
	gtk_widget_grab_default(close_bt);

	/* Catch the "key_press_event" signal in the window, so that we can 
	   catch the ESC key being pressed and act as if the "Close" button had
	   been selected. */
	dlg_set_cancel(talkers->win, close_bt);

	gtk_widget_show_all(talkers->win);
	retap_packets(&cfile);

    /* after retapping, redraw table */
    draw_ett_table_data(talkers);
}



static void
ett_win_destroy_notebook_cb(GtkWindow *win _U_, gpointer data)
{
    void ** pages = data;
    int page;

    /* first "page" contains the number of pages */
    for (page=1; page<=GPOINTER_TO_INT(pages[0]); page++) {
        ett_win_destroy_cb(NULL, pages[page]);
    }
}




endpoints_table *
init_ett_notebook_page_cb(gboolean hide_ports, char *table_name, char *tap_name, char *filter, void *packet_func)
{
    gboolean ret;
    GtkWidget *page_vbox;
	endpoints_table *talkers;

    talkers=g_malloc(sizeof(endpoints_table));
	talkers->name=table_name;
    talkers->resolve_names=TRUE;

    page_vbox=gtk_vbox_new(FALSE, 6);
    talkers->win = page_vbox;
	gtk_container_set_border_width(GTK_CONTAINER(page_vbox), 6);

    ret = init_ett_table_page(talkers, page_vbox, hide_ports, table_name, tap_name, filter, packet_func);
    if(ret == FALSE) {
		g_free(talkers);
        return NULL;
    }

    return talkers;
}


typedef struct {
    gboolean hide_ports;    /* hide TCP / UDP port columns */
    char *table_name;       /* GUI output name */
    char *tap_name;         /* internal name */
    char *filter;           /* display filter string (unused) */
    void *packet_func;      /* function to be called for new incoming packets */
} register_ett_t;


GSList *registered_ett_tables = NULL;

void
register_ett_table(gboolean hide_ports, char *table_name, char *tap_name, char *filter, void *packet_func)
{
    register_ett_t *table;

    table = g_malloc(sizeof(register_ett_t));

    table->hide_ports   = hide_ports;
    table->table_name   = table_name;
    table->tap_name     = tap_name;
    table->filter       = filter;
    table->packet_func  = packet_func;

    registered_ett_tables = g_slist_append(registered_ett_tables, table);
}


static void
ett_resolve_toggle_dest(GtkWidget *widget, gpointer data)
{
    int page;
    void ** pages = data;
    gboolean resolve_names;
	endpoints_table *talkers;


    resolve_names = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (widget));

    for (page=1; page<=GPOINTER_TO_INT(pages[0]); page++) {
        talkers = pages[page];
        talkers->resolve_names = resolve_names;

        draw_ett_table_addresses(talkers);
    }
}

void
init_ett_notebook_cb(GtkWidget *w _U_, gpointer d _U_)
{
	endpoints_table *talkers;
	char title[256];
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *bbox;
    GtkWidget *close_bt;
    GtkWidget *win;
    GtkWidget *resolv_cb;
    int page;
    void ** pages;
    GtkWidget *nb;
    GtkWidget *page_lb;
    GSList  *current_table;
    register_ett_t *registered;
    GtkTooltips *tooltips = gtk_tooltips_new();


    pages = g_malloc(sizeof(void *) * (g_slist_length(registered_ett_tables) + 1));

	g_snprintf(title, 255, "Conversations: %s", cf_get_display_name(&cfile));
	win=dlg_window_new(title);
	gtk_window_set_default_size(GTK_WINDOW(win), 750, 400);

	SIGNAL_CONNECT(win, "destroy", ett_win_destroy_notebook_cb, pages);

    vbox=gtk_vbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(win), vbox);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);

    nb = gtk_notebook_new();
	gtk_container_add(GTK_CONTAINER(vbox), nb);

    page = 0;

    current_table = registered_ett_tables;
    while(current_table) {
        registered = current_table->data;
        page_lb = gtk_label_new("");
        talkers = init_ett_notebook_page_cb(registered->hide_ports, registered->table_name, registered->tap_name, 
            registered->filter, registered->packet_func);
        gtk_notebook_append_page(GTK_NOTEBOOK(nb), talkers->win, page_lb);
        talkers->win = win;
	    talkers->page_lb = page_lb;
        pages[++page] = talkers;

        current_table = g_slist_next(current_table);
    }

    pages[0] = GINT_TO_POINTER(page);

    hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    resolv_cb = CHECK_BUTTON_NEW_WITH_MNEMONIC("Name resolution", NULL);
	gtk_container_add(GTK_CONTAINER(hbox), resolv_cb);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(resolv_cb), TRUE);
    gtk_tooltips_set_tip(tooltips, resolv_cb, "Show results of name resolutions rather than the \"raw\" values. "
        "Please note: The corresponding name resolution must be enabled.", NULL);

    SIGNAL_CONNECT(resolv_cb, "toggled", ett_resolve_toggle_dest, pages);

	/* Button row. */
	bbox = dlg_button_row_new(GTK_STOCK_CLOSE, NULL);
	gtk_box_pack_end(GTK_BOX(vbox), bbox, FALSE, FALSE, 0);

	close_bt = OBJECT_GET_DATA(bbox, GTK_STOCK_CLOSE);
	SIGNAL_CONNECT_OBJECT(close_bt, "clicked", gtk_widget_destroy, win);
	gtk_widget_grab_default(close_bt);

	/* Catch the "key_press_event" signal in the window, so that we can 
	   catch the ESC key being pressed and act as if the "Close" button had
	   been selected. */
	dlg_set_cancel(win, close_bt);

	gtk_widget_show_all(win);
	retap_packets(&cfile);

    /* after retapping, redraw table */
    for (page=1; page<=GPOINTER_TO_INT(pages[0]); page++) {
        draw_ett_table_data(pages[page]);
    }
}


void 
add_ett_table_data(endpoints_table *et, address *src, address *dst, guint32 src_port, guint32 dst_port, int num_frames, int num_bytes, int sat, int port_type)
{
	address *addr1, *addr2;
	guint32 port1, port2;
	endpoint_talker_t *talker=NULL;
	int talker_idx=0;
	gboolean new_talker;

	if(src_port>dst_port){
		addr1=src;
		addr2=dst;
		port1=src_port;
		port2=dst_port;
	} else if(src_port<dst_port){
		addr2=src;
		addr1=dst;
		port2=src_port;
		port1=dst_port;
	} else if(CMP_ADDRESS(src, dst)<0){
		addr1=src;
		addr2=dst;
		port1=src_port;
		port2=dst_port;
	} else {
		addr2=src;
		addr1=dst;
		port2=src_port;
		port1=dst_port;
	}


	new_talker=FALSE;
	/* XXX should be optimized to allocate n extra entries at a time
	   instead of just one */
	/* if we dont have any entries at all yet */
	if(et->endpoints==NULL){
		et->endpoints=g_malloc(sizeof(endpoint_talker_t));
		et->num_endpoints=1;
		talker=&et->endpoints[0];
		talker_idx=0;
		new_talker=TRUE;
	}

	/* try to find it among the existing known talkers */
	if(talker==NULL){
		guint32 i;
		for(i=0;i<et->num_endpoints;i++){
			if(  (!CMP_ADDRESS(&et->endpoints[i].src_address, addr1))&&(!CMP_ADDRESS(&et->endpoints[i].dst_address, addr2))&&(et->endpoints[i].src_port==port1)&&(et->endpoints[i].dst_port==port2) ){
				talker=&et->endpoints[i];
				talker_idx=i;
				break;
			}
			if( (!CMP_ADDRESS(&et->endpoints[i].src_address, addr2))&&(!CMP_ADDRESS(&et->endpoints[i].dst_address, addr1))&&(et->endpoints[i].src_port==port2)&&(et->endpoints[i].dst_port==port1) ){
				talker=&et->endpoints[i];
				talker_idx=i;
				break;
			}
		}
	}

	/* if we still dont know what talker this is it has to be a new one
	   and we have to allocate it and append it to the end of the list */
	if(talker==NULL){
		new_talker=TRUE;
		et->num_endpoints++;
		et->endpoints=g_realloc(et->endpoints, et->num_endpoints*sizeof(endpoint_talker_t));
		talker=&et->endpoints[et->num_endpoints-1];
		talker_idx=et->num_endpoints-1;
	}

	/* if this is a new talker we need to initialize the struct */
	if(new_talker){
		COPY_ADDRESS(&talker->src_address, addr1);
		COPY_ADDRESS(&talker->dst_address, addr2);
		talker->sat=sat;
		talker->port_type=port_type;
		talker->src_port=port1;
		talker->dst_port=port2;
		talker->rx_frames=0;
		talker->tx_frames=0;
		talker->rx_bytes=0;
		talker->tx_bytes=0;
	}

	/* update the talker struct */
	if( (!CMP_ADDRESS(src, addr1))&&(!CMP_ADDRESS(dst, addr2))&&(src_port==port1)&&(dst_port==port2) ){
		talker->tx_frames+=num_frames;
		talker->tx_bytes+=num_bytes;
	} else {
		talker->rx_frames+=num_frames;
		talker->rx_bytes+=num_bytes;
	}

	/* if this was a new talker we have to create a clist row for it */
	if(new_talker){
		char *entries[NUM_COLS];
		char frames[16],bytes[16],txframes[16],txbytes[16],rxframes[16],rxbytes[16];


        /* these values will be filled by call to draw_ett_table_addresses() below */
        entries[0] = "";
        entries[1] = "";
        entries[2] = "";
        entries[3] = "";

		g_snprintf(frames, 16, "%u", talker->tx_frames+talker->rx_frames);
		entries[4]=frames;
		g_snprintf(bytes, 16, "%u", talker->tx_bytes+talker->rx_bytes);
		entries[5]=bytes;

		g_snprintf(txframes, 16, "%u", talker->tx_frames);
		entries[6]=txframes;
		g_snprintf(txbytes, 16, "%u", talker->tx_bytes);
		entries[7]=txbytes;

		g_snprintf(rxframes, 16, "%u", talker->rx_frames);
		entries[8]=rxframes;
		g_snprintf(rxbytes, 16, "%u", talker->rx_bytes);
		entries[9]=rxbytes;

		gtk_clist_insert(et->table, talker_idx, entries);
		gtk_clist_set_row_data(et->table, talker_idx, (gpointer) talker_idx);

        draw_ett_table_addresses(et);
    }
}


