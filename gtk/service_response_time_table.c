/* service_response_time_table.c
 * service_response_time_table   2003 Ronnie Sahlberg
 * Helper routines common to all service response time statistics
 * tap.
 *
 * $Id: service_response_time_table.c,v 1.3 2003/06/21 05:57:34 sahlberg Exp $
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
#include "service_response_time_table.h"
#include "image/clist_ascend.xpm"
#include "image/clist_descend.xpm"


typedef struct column_arrows {
	GtkWidget *table;
	GtkWidget *ascend_pm;
	GtkWidget *descend_pm;
} column_arrows;


static void
srt_click_column_cb(GtkCList *clist, gint column, gpointer data)
{
	column_arrows *col_arrows = (column_arrows *) data;
	int i;

	gtk_clist_freeze(clist);

	for (i = 0; i < 6; i++) {
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
		/* Columns 2-5   Count, Min, Max, Avg are sorted in descending
			order by default.
		   Columns 0 and 1 sort by ascending order by default 
		*/
		if(column>=2){
			clist->sort_type = GTK_SORT_DESCENDING;
			gtk_widget_show(col_arrows[column].descend_pm);
		} else {
			clist->sort_type = GTK_SORT_ASCENDING;
			gtk_widget_show(col_arrows[column].ascend_pm);
		}
		gtk_clist_set_sort_column(clist, column);
	}
	gtk_clist_thaw(clist);

	gtk_clist_sort(clist);
}

static gint
srt_sort_column(GtkCList *clist, gconstpointer ptr1, gconstpointer ptr2)
{
	char *text1 = NULL;
	char *text2 = NULL;
	int i1, i2;
	float f1,f2;

	GtkCListRow *row1 = (GtkCListRow *) ptr1;
	GtkCListRow *row2 = (GtkCListRow *) ptr2;

	text1 = GTK_CELL_TEXT (row1->cell[clist->sort_column])->text;
	text2 = GTK_CELL_TEXT (row2->cell[clist->sort_column])->text;

	switch(clist->sort_column){
	case 1:
		return strcmp (text1, text2);
	case 0:
	case 2:
		i1=atoi(text1);	
		i2=atoi(text2);
		return i1-i2;
	case 3:
	case 4:
	case 5:
		sscanf(text1,"%f",&f1);
		sscanf(text2,"%f",&f2);
		if(fabs(f1-f2)<0.000005)
			return 0;
		if(f1>f2)
			return 1;
		return -1;
	}
	g_assert_not_reached();
	return 0;	
}

void
init_srt_table(srt_stat_table *rst, int num_procs, GtkWidget *vbox)
{
	int i, j;
	column_arrows *col_arrows;
	GdkBitmap *ascend_bm, *descend_bm;
	GdkPixmap *ascend_pm, *descend_pm;
	GtkStyle *win_style;
	GtkWidget *column_lb;
	char *default_titles[] = { "Index", "Procedure", "Calls", "Min SRT", "Max SRT", "Avg SRT" };


	rst->scrolled_window=gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(rst->scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	gtk_box_pack_start(GTK_BOX(vbox), rst->scrolled_window, TRUE, TRUE, 0);


	rst->table=(GtkCList *)gtk_clist_new(6);

	col_arrows = (column_arrows *) g_malloc(sizeof(column_arrows) * 6);
	win_style = gtk_widget_get_style(rst->scrolled_window);
	ascend_pm = gdk_pixmap_create_from_xpm_d(rst->scrolled_window->window,
			&ascend_bm,
			&win_style->bg[GTK_STATE_NORMAL],
			(gchar **)clist_ascend_xpm);
	descend_pm = gdk_pixmap_create_from_xpm_d(rst->scrolled_window->window,
			&descend_bm,
			&win_style->bg[GTK_STATE_NORMAL],
			(gchar **)clist_descend_xpm);
	for (i = 0; i < 6; i++) {
		col_arrows[i].table = gtk_table_new(2, 2, FALSE);
		gtk_table_set_col_spacings(GTK_TABLE(col_arrows[i].table), 5);
		column_lb = gtk_label_new(default_titles[i]);
		gtk_table_attach(GTK_TABLE(col_arrows[i].table), column_lb, 0, 1, 0, 2, GTK_SHRINK, GTK_SHRINK, 0, 0);
		gtk_widget_show(column_lb);

		col_arrows[i].ascend_pm = gtk_pixmap_new(ascend_pm, ascend_bm);
		gtk_table_attach(GTK_TABLE(col_arrows[i].table), col_arrows[i].ascend_pm, 1, 2, 1, 2, GTK_SHRINK, GTK_SHRINK, 0, 0);
		if (i == 0) {
			gtk_widget_show(col_arrows[i].ascend_pm);
		}
		col_arrows[i].descend_pm = gtk_pixmap_new(descend_pm, descend_bm);
		gtk_table_attach(GTK_TABLE(col_arrows[i].table), col_arrows[i].descend_pm, 1, 2, 0, 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
		gtk_clist_set_column_widget(GTK_CLIST(rst->table), i, col_arrows[i].table);
		gtk_widget_show(col_arrows[i].table);
	}
	gtk_clist_column_titles_show(GTK_CLIST(rst->table));

	gtk_clist_set_compare_func(rst->table, srt_sort_column);
	gtk_clist_set_sort_column(rst->table, 2);
	gtk_clist_set_sort_type(rst->table, GTK_SORT_DESCENDING);


	/*XXX instead of this we should probably have some code to
		dynamically adjust the width of the columns */
	gtk_clist_set_column_width(rst->table, 0, 32);
	gtk_clist_set_column_width(rst->table, 1, 160);
	gtk_clist_set_column_width(rst->table, 2, 50);
	gtk_clist_set_column_width(rst->table, 3, 60);
	gtk_clist_set_column_width(rst->table, 4, 60);
	gtk_clist_set_column_width(rst->table, 5, 60);

	gtk_clist_set_shadow_type(rst->table, GTK_SHADOW_IN);
	gtk_clist_column_titles_show(rst->table);
	gtk_container_add(GTK_CONTAINER(rst->scrolled_window), (GtkWidget *)rst->table);

	SIGNAL_CONNECT(rst->table, "click-column", srt_click_column_cb, col_arrows);

	gtk_widget_show((GtkWidget *)rst->table);
	gtk_widget_show(rst->scrolled_window);


	rst->num_procs=num_procs;
	rst->procedures=g_malloc(sizeof(srt_procedure_t)*num_procs);
	for(i=0;i<num_procs;i++){
		rst->procedures[i].num=0;
		rst->procedures[i].min.secs=0;
		rst->procedures[i].min.nsecs=0;
		rst->procedures[i].max.secs=0;
		rst->procedures[i].max.nsecs=0;
		rst->procedures[i].tot.secs=0;
		rst->procedures[i].tot.nsecs=0;
		for(j=0;j<6;j++){
			rst->procedures[i].entries[j]=NULL;
		}
	}
}

void
init_srt_table_row(srt_stat_table *rst, int index, char *procedure)
{
	char str[10];


	sprintf(str,"%d",index);
	rst->procedures[index].entries[0]=g_strdup(str);

	rst->procedures[index].entries[1]=g_strdup(procedure);

	rst->procedures[index].entries[2]=g_strdup("0");
	rst->procedures[index].entries[3]=g_strdup("0");
	rst->procedures[index].entries[4]=g_strdup("0");
	rst->procedures[index].entries[5]=g_strdup("0");

	gtk_clist_insert(rst->table, index, rst->procedures[index].entries);
	gtk_clist_set_row_data(rst->table, index, (gpointer) index);
}

void
add_srt_table_data(srt_stat_table *rst, int index, nstime_t *req_time, packet_info *pinfo)
{
	srt_procedure_t *rp;
	nstime_t delta;

	rp=&rst->procedures[index];
	
	/* calculate time delta between request and reply */
	delta.secs=pinfo->fd->abs_secs-req_time->secs;
	delta.nsecs=pinfo->fd->abs_usecs*1000-req_time->nsecs;
	if(delta.nsecs<0){
		delta.nsecs+=1000000000;
		delta.secs--;
	}

	if((rp->max.secs==0)
	&& (rp->max.nsecs==0) ){
		rp->max.secs=delta.secs;
		rp->max.nsecs=delta.nsecs;
	}

	if((rp->min.secs==0)
	&& (rp->min.nsecs==0) ){
		rp->min.secs=delta.secs;
		rp->min.nsecs=delta.nsecs;
	}

	if( (delta.secs<rp->min.secs)
	||( (delta.secs==rp->min.secs)
	  &&(delta.nsecs<rp->min.nsecs) ) ){
		rp->min.secs=delta.secs;
		rp->min.nsecs=delta.nsecs;
	}

	if( (delta.secs>rp->max.secs)
	||( (delta.secs==rp->max.secs)
	  &&(delta.nsecs>rp->max.nsecs) ) ){
		rp->max.secs=delta.secs;
		rp->max.nsecs=delta.nsecs;
	}

	rp->tot.secs += delta.secs;
	rp->tot.nsecs += delta.nsecs;
	if(rp->tot.nsecs>1000000000){
		rp->tot.nsecs-=1000000000;
		rp->tot.secs++;
	}
	rp->num++;
}

void
draw_srt_table_data(srt_stat_table *rst)
{
	int i,j;
#ifdef G_HAVE_UINT64
	guint64 td;
#else
	guint32 td;
#endif
	char str[256], *strp;

	for(i=0;i<rst->num_procs;i++){
		/* scale it to units of 10us.*/
		/* for long captures with a large tot time, this can overflow on 32bit */
		td=(int)rst->procedures[i].tot.secs;
		td=td*100000+(int)rst->procedures[i].tot.nsecs/10000;
		if(rst->procedures[i].num){
			td/=rst->procedures[i].num;
		} else {
			td=0;
		}

		j=gtk_clist_find_row_from_data(rst->table, (gpointer)i);

		sprintf(str,"%d", rst->procedures[i].num);
		strp=g_strdup(str);
		gtk_clist_set_text(rst->table, j, 2, strp);
		g_free(rst->procedures[i].entries[2]);
		rst->procedures[i].entries[2]=strp;


		sprintf(str,"%3d.%05d", (int)rst->procedures[i].min.secs,rst->procedures[i].min.nsecs/10000);
		strp=g_strdup(str);
		gtk_clist_set_text(rst->table, j, 3, strp);
		g_free(rst->procedures[i].entries[3]);
		rst->procedures[i].entries[3]=strp;


		sprintf(str,"%3d.%05d", (int)rst->procedures[i].max.secs,rst->procedures[i].max.nsecs/10000);
		strp=g_strdup(str);
		gtk_clist_set_text(rst->table, j, 4, strp);
		g_free(rst->procedures[i].entries[4]);
		rst->procedures[i].entries[4]=strp;

		sprintf(str,"%3d.%05d", td/100000, td%100000);
		strp=g_strdup(str);
		gtk_clist_set_text(rst->table, j, 5, strp);
		g_free(rst->procedures[i].entries[5]);
		rst->procedures[i].entries[5]=strp;
	}

	gtk_clist_sort(rst->table);
}


void
reset_srt_table_data(srt_stat_table *rst)
{
	int i;

	for(i=0;i<rst->num_procs;i++){
		rst->procedures[i].num=0;	
		rst->procedures[i].min.secs=0;
		rst->procedures[i].min.nsecs=0;
		rst->procedures[i].max.secs=0;
		rst->procedures[i].max.nsecs=0;
		rst->procedures[i].tot.secs=0;
		rst->procedures[i].tot.nsecs=0;
	}
}

void
free_srt_table_data(srt_stat_table *rst)
{
	int i,j;

	for(i=0;i<rst->num_procs;i++){
		for(j=0;j<6;j++){
			if(rst->procedures[i].entries[j]){
				g_free(rst->procedures[i].entries[j]);
				rst->procedures[i].entries[j]=NULL;
			}
		}
	}
	g_free(rst->procedures);
	rst->procedures=NULL;
	rst->num_procs=0;
}

