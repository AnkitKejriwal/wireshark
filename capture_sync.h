/* capture_sync.h
 * Synchronisation between Ethereal capture parent and child instances
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


/** @file
 *  
 *  Sync mode capture (internal interface).
 *
 *  Will start a new Ethereal instance which will do the actual capture work.
 *  This is only used, if the "Update list of packets in real time" option is 
 *  used.
 */

#ifndef __CAPTURE_SYNC_H__
#define __CAPTURE_SYNC_H__

/** 
 * Start a new synced capture session.
 *  Create a capture child which is doing the real capture work.
 *
 *  Most of the parameters are passed through the global capture_opts.
 *
 *  @param capture_opts the options (formerly global)
 *  @param is_tempfile  TRUE if the current cfile is a tempfile
 *  @return             TRUE if a capture could be started, FALSE if not
 */
extern gboolean 
sync_pipe_do_capture(capture_options *capture_opts, gboolean is_tempfile);

/** User wants to stop capturing, gracefully close the capture child */
extern void
sync_pipe_stop(capture_options *capture_opts);

/** We want to stop the program, just kill the child as soon as possible */
extern void
sync_pipe_kill(capture_options *capture_opts);


/** the child will immediately start capturing, notify the parent */
extern void
sync_pipe_capstart_to_parent(void);

/** the child captured some new packets, notify the parent */
extern void
sync_pipe_packet_count_to_parent(int packet_count);

/** the child stopped capturing, notify the parent */
extern void
sync_pipe_drops_to_parent(int drops);

/** the child has opened a new capture file, notify the parent */
extern void
sync_pipe_filename_to_parent(const char *filename);

/** the child encountered an error, notify the parent */
extern void 
sync_pipe_errmsg_to_parent(const char *errmsg);


#endif /* capture_sync.h */
