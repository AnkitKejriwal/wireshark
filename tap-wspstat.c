/* tap-rpcstat.c
 * wspstat   2003 Jean-Michel FAYARD
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

/* This module provides WSP  statistics to tethereal.
 * It is only used by tethereal and not ethereal
 *
 */

/* With MSVC and a libethereal.dll this file needs to import some variables 
   in a special way. Therefore _NEED_VAR_IMPORT_ is defined. */   
#define _NEED_VAR_IMPORT_

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#include <string.h>
#include "epan/packet_info.h"
#include <epan/tap.h>
#include "register.h"
#include "epan/value_string.h"
#include <epan/dissectors/packet-wsp.h>

/* used to keep track of the stats for a specific PDU type*/
typedef struct _wsp_pdu_t {
	gchar 		*type;
	guint32		 packets;
} wsp_pdu_t;
/* used to keep track of RTT statistics */
typedef struct _wsp_status_code_t {
	gchar		*name;
	guint32		 packets;
} wsp_status_code_t;
/* used to keep track of the statictics for an entire program interface */
typedef struct _wsp_stats_t {
	char 		*filter;
	wsp_pdu_t 	*pdu_stats;
	guint32	num_pdus;
	GHashTable	*hash;
} wspstat_t;
	
static void
wsp_reset_hash(gchar *key _U_ , wsp_status_code_t *data, gpointer ptr _U_ ) 
{	
	data->packets = 0;
}
static void
wsp_print_statuscode(gint *key, wsp_status_code_t *data, char* format)
{
	if (data && (data->packets!=0))
		printf(format, *key, data->packets ,data->name);
}
static void
wsp_free_hash_table( gpointer key, gpointer value, gpointer user_data _U_ )
{
	g_free(key);
	g_free(value);
}
static void
wspstat_reset(void *psp)
{
	wspstat_t *sp=psp;
	guint32 i;

	for(i=1;i<=sp->num_pdus;i++)
	{
		sp->pdu_stats[i].packets=0;
	}
	g_hash_table_foreach( sp->hash, (GHFunc)wsp_reset_hash, NULL);
}


/* This callback is invoked whenever the tap system has seen a packet
 * we might be interested in.
 * The function is to be used to only update internal state information
 * in the *tapdata structure, and if there were state changes which requires
 * the window to be redrawn, return 1 and (*draw) will be called sometime
 * later.
 *
 * We didnt apply a filter when we registered so we will be called for
 * ALL packets and not just the ones we are collecting stats for.
 *
 */
static gint 
pdut2index(gint pdut)
{
	if (pdut<=0x09)		return pdut;
	if (pdut>=0x40){
		if (pdut <= 0x44){
			return pdut-54;
		} else if (pdut==0x60||pdut==0x61){
			return pdut-81;
		}
	}
	return 0;
}
static gint
index2pdut(gint pdut)
{
	if (pdut<=0x09)
		return pdut;
	if (pdut<=14)
		return pdut+54;
	if (pdut<=16)
		return pdut+81;
	return 0;
}
static int
wspstat_packet(void *psp, packet_info *pinfo _U_, epan_dissect_t *edt _U_, void *pri)
{
	wspstat_t *sp=psp;
	wsp_info_value_t *value=pri;
	gint index = pdut2index(value->pdut);
	int retour=0;

	if (value->status_code != 0) {
		gint *key=g_malloc( sizeof(gint) );
		wsp_status_code_t *sc;
		*key=value->status_code ;
		sc = g_hash_table_lookup( 
				sp->hash, 
				key);
		if (!sc) {
			sc = g_malloc( sizeof(wsp_status_code_t) );
			sc -> packets = 1;
			sc -> name = NULL;
			g_hash_table_insert(
				sp->hash,
				key,
				sc);
		} else {
			sc->packets++;
		}
		retour=1;
	}

		

	if (index!=0) {
		sp->pdu_stats[ index ].packets++;
		retour = 1;
	}
	return retour;
}


/* This callback is used when tethereal wants us to draw/update our
 * data to the output device. Since this is tethereal only output is
 * stdout.
 * Tethereal will only call this callback once, which is when tethereal has
 * finished reading all packets and exists.
 * If used with ethereal this may be called any time, perhaps once every 3 
 * seconds or so.
 * This function may even be called in parallell with (*reset) or (*draw)
 * so make sure there are no races. The data in the rpcstat_t can thus change
 * beneath us. Beware.
 */
static void
wspstat_draw(void *psp)
{
	wspstat_t *sp=psp;
	guint32 i;

	printf("\n");
	printf("===================================================================\n");
	printf("WSP Statistics:\n");
	printf("%-23s %9s || %-23s %9s\n","PDU Type", "Packets", "PDU Type", "Packets");
	for(i=1; i<= ((sp->num_pdus+1)/2) ; i++)
	{
		guint32 ii=i+sp->num_pdus/2;
		printf("%-23s %9d", sp->pdu_stats[i ].type, sp->pdu_stats[i ].packets);
		printf(" || ");
		if (ii< (sp->num_pdus) )
			printf("%-23s %9d\n", sp->pdu_stats[ii].type, sp->pdu_stats[ii].packets);
		else 
			printf("\n");
		}
	printf("\nStatus code in reply packets\n");
	printf(		"Status Code    Packets  Description\n");
	g_hash_table_foreach( sp->hash, (GHFunc) wsp_print_statuscode, 
			"       0x%02X  %9d  %s\n" ) ;
	printf("===================================================================\n");
}

/* When called, this function will create a new instance of wspstat.
 * program and version are whick onc-rpc program/version we want to
 * collect statistics for.
 * This function is called from tethereal when it parses the -z wsp, arguments
 * and it creates a new instance to store statistics in and registers this
 * new instance for the wsp tap.
 */
static void
wspstat_init(char *optarg)
{
	wspstat_t *sp;
	char *filter=NULL;
	guint32 i;
	GString	*error_string;
	wsp_status_code_t *sc;
	
	if (!strncmp (optarg, "wsp,stat," , 9)){
		filter=optarg+9;
	} else {
		filter=NULL;
	}
	
		
	sp = g_malloc( sizeof(wspstat_t) );
	sp->hash = g_hash_table_new( g_int_hash, g_int_equal);
	for (i=0 ; vals_status[i].strptr ; i++ )
	{
		gint *key;
		sc=g_malloc( sizeof(wsp_status_code_t) );
		key=g_malloc( sizeof(gint) );
		sc->packets=0;
		sc->name=vals_status[i].strptr;
		*key=vals_status[i].value;
		g_hash_table_insert(
				sp->hash,
				key,
				sc);
	}
	sp->num_pdus = 16;
	sp->pdu_stats=g_malloc( (sp->num_pdus+1) * sizeof( wsp_pdu_t) );
	if(filter){
		sp->filter=g_malloc(strlen(filter)+1);
		strcpy(sp->filter,filter);
	} else {
		sp->filter=NULL;
	}
	for (i=0;i<sp->num_pdus; i++)
	{
		sp->pdu_stats[i].packets=0;
		sp->pdu_stats[i].type = match_strval( index2pdut( i ), vals_pdu_type) ;
	}

	error_string = register_tap_listener( 
			"wsp",
			sp,
			filter,
			wspstat_reset,
			wspstat_packet,
			wspstat_draw);
	if (error_string){
		/* error, we failed to attach to the tap. clean up */
		g_free(sp->pdu_stats);
		g_free(sp->filter);
		g_free(sp);
		g_hash_table_foreach( sp->hash, (GHFunc) wsp_free_hash_table, NULL ) ;
		g_hash_table_destroy( sp->hash );
		fprintf(stderr, "tethereal: Couldn't register wsp,stat tap: %s\n",
				error_string->str);
		g_string_free(error_string, TRUE);
		exit(1);
	}
}
void 
register_tap_listener_wspstat(void)
{
	register_ethereal_tap("wsp,stat,", wspstat_init);
}
