/* summary.c
 * Routines for capture file summary info
 *
 * $Id: summary.c,v 1.15 1999/12/10 04:20:53 gram Exp $
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@zing.org>
 * Copyright 1998 Gerald Combs
 *
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

#include "packet.h"
#include "globals.h"
#include "summary.h"


static double
secs_usecs( guint32 s, guint32 us)
{
  return (us / 1000000.0) + (double)s;
}

static void
tally_frame_data(frame_data *cur_frame, summary_tally *sum_tally)
{
  double cur_time;

  cur_time = secs_usecs(cur_frame->abs_secs, cur_frame->abs_usecs);

    if (cur_time < sum_tally->start_time) {
      sum_tally->start_time = cur_time;
    }
    if (cur_time > sum_tally->stop_time){
      sum_tally->stop_time = cur_time;
    }
    sum_tally->bytes += cur_frame->pkt_len;
    if (cur_frame->passed_dfilter)
	  sum_tally->filtered_count++;
}

void
summary_fill_in(summary_tally *st)
{

  frame_data    *first_frame, *cur_frame;
  int 		i;
  frame_data    *cur_glist;

 /* initialize the tally */
  first_frame = cf.plist;
  st->start_time = secs_usecs(first_frame->abs_secs,first_frame->abs_usecs) ;
  st->stop_time = secs_usecs(first_frame->abs_secs,first_frame->abs_usecs) ;
  st->bytes = 0;
  st->filtered_count = 0;
  cur_glist = cf.plist;

  for (i = 0; i < cf.count; i++) {
    cur_frame = cur_glist;
    tally_frame_data(cur_frame, st);
    cur_glist = cur_glist->next;
  }

  st->filename = cf.filename;
  st->file_length = cf.f_len;
  st->encap_type = cf.cd_t;
  st->snap = cf.snap;
  st->elapsed_time = secs_usecs(cf.esec, cf.eusec);
  st->packet_count = cf.count;
  st->drops = cf.drops;
  st->iface = cf.iface;
  st->dfilter = cf.dfilter;

#ifdef HAVE_LIBPCAP
  st->cfilter = cf.cfilter;
#else
  st->cfilter = NULL;
#endif


}

