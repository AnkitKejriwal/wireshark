/* capture.c
 * Routines for packet capture windows
 *
 * $Id: capture.c,v 1.253 2004/06/29 20:51:26 ulfl Exp $
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

/* With MSVC and a libethereal.dll this file needs to import some variables 
   in a special way. Therefore _NEED_VAR_IMPORT_ is defined. */
#define _NEED_VAR_IMPORT_

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_LIBPCAP

#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif

#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif

#ifndef _WIN32
/*
 * Define various POSIX macros (and, in the case of WCOREDUMP, non-POSIX
 * macros) on UNIX systems that don't have them.
 */
#ifndef WIFEXITED
# define WIFEXITED(status)	(((status) & 0177) == 0)
#endif
#ifndef WIFSTOPPED
# define WIFSTOPPED(status)	(((status) & 0177) == 0177)
#endif
#ifndef WIFSIGNALED
# define WIFSIGNALED(status)	(!WIFSTOPPED(status) && !WIFEXITED(status))
#endif
#ifndef WEXITSTATUS
# define WEXITSTATUS(status)	((status) >> 8)
#endif
#ifndef WTERMSIG
# define WTERMSIG(status)	((status) & 0177)
#endif
#ifndef WCOREDUMP
# define WCOREDUMP(status)	((status) & 0200)
#endif
#ifndef WSTOPSIG
# define WSTOPSIG(status)	((status) >> 8)
#endif
#endif /* _WIN32 */

#ifdef HAVE_IO_H
# include <io.h>
#endif

#include <pcap.h>

#include <glib.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <time.h>

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#include <signal.h>
#include <errno.h>

#ifdef NEED_SNPRINTF_H
# include "snprintf.h"
#endif

#ifdef _WIN32
#include <process.h>    /* For spawning child process */
#endif

/*
 * We don't want to do a "select()" on the pcap_t's file descriptor on
 * BSD (because "select()" doesn't work correctly on BPF devices on at
 * least some releases of some flavors of BSD), and we don't want to do
 * it on Windows (because "select()" is something for sockets, not for
 * arbitrary handles).  (Note that "Windows" here includes Cygwin;
 * even in its pretend-it's-UNIX environment, we're using WinPcap, not
 * a UNIX libpcap.)
 *
 * We *do* want to do it on other platforms, as, on other platforms (with
 * the possible exception of Ultrix and Digital UNIX), the read timeout
 * doesn't expire if no packets have arrived, so a "pcap_dispatch()" call
 * will block until packets arrive, causing the UI to hang.
 *
 * XXX - the various BSDs appear to define BSD in <sys/param.h>; we don't
 * want to include it if it's not present on this platform, however.
 */
#if !defined(__FreeBSD__) && !defined(__NetBSD__) && !defined(__OpenBSD__) && \
    !defined(__bsdi__) && !defined(__APPLE__) && !defined(_WIN32) && \
    !defined(__CYGWIN__)
# define MUST_DO_SELECT
#endif

#include <epan/packet.h>
#include <epan/dfilter/dfilter.h>
#include "file.h"
#include "capture.h"
#include "util.h"
#include "pcap-util.h"
#include "alert_box.h"
#include "simple_dialog.h"
#include "prefs.h"
#include "globals.h"
#include "conditions.h"
#include "capture_stop_conditions.h"
#include "ringbuffer.h"

#include "wiretap/libpcap.h"
#include "wiretap/wtap.h"
#include "wiretap/wtap-capture.h"

#include "packet-ap1394.h"
#include "packet-atalk.h"
#include "packet-atm.h"
#include "packet-clip.h"
#include "packet-eth.h"
#include "packet-fddi.h"
#include "packet-null.h"
#include "packet-ppp.h"
#include "packet-raw.h"
#include "packet-sll.h"
#include "packet-tr.h"
#include "packet-ieee80211.h"
#include "packet-chdlc.h"
#include "packet-prism.h"
#include "packet-ipfc.h"
#include "packet-arcnet.h"

#ifdef _WIN32
#include "capture-wpcap.h"
#endif
#include "ui_util.h"

/*
 * Capture options.
 */
capture_options capture_opts;
gboolean quit_after_cap = FALSE;/* Makes a "capture only mode". Implies -k */
gboolean capture_child;	        /* if this is the child for "-S" */

static int sync_pipe[2];        /* used to sync father */
enum PIPES { READ, WRITE };     /* Constants 0 and 1 for READ and WRITE */
static int fork_child = -1;	    /* If not -1, in parent, process ID of child */

/* Size of buffer to hold decimal representation of
   signed/unsigned 64-bit int */
#define SP_DECISIZE 20

/*
 * Indications sent out on the sync pipe.
 */
#define SP_CAPSTART	';'	    /* capture start message */
#define SP_PACKET_COUNT	'*'	/* followed by count of packets captured since last message */
#define SP_ERROR_MSG	'!'	/* followed by length of error message that follows */
#define SP_DROPS	'#'	    /* followed by count of packets dropped in capture */


typedef struct _loop_data {
  gboolean       go;           /* TRUE as long as we're supposed to keep capturing */
  gint           max;          /* Number of packets we're supposed to capture - 0 means infinite */
  int            err;          /* if non-zero, error seen while capturing */
  gint           linktype;
  gint           sync_packets;
  gboolean       pcap_err;     /* TRUE if error from pcap */
  gboolean       from_cap_pipe;/* TRUE if we are capturing data from a pipe */
  packet_counts  counts;
  wtap_dumper   *pdh;
#ifndef _WIN32
  gboolean       modified;     /* TRUE if data in the pipe uses modified pcap headers */
  gboolean       byte_swapped; /* TRUE if data in the pipe is byte swapped */
  unsigned int   bytes_to_read, bytes_read; /* Used by cap_pipe_dispatch */
  enum {
         STATE_EXPECT_REC_HDR, STATE_READ_REC_HDR,
         STATE_EXPECT_DATA,     STATE_READ_DATA
       } cap_pipe_state;

  enum { PIPOK, PIPEOF, PIPERR, PIPNEXIST } cap_pipe_err;
#endif
} loop_data;


static gboolean sync_pipe_do_capture(gboolean is_tempfile);
static gboolean sync_pipe_input_cb(gint source, gpointer user_data);
static void sync_pipe_wait_for_child(gboolean);
static void sync_pipe_errmsg_to_parent(const char *);
#ifndef _WIN32
static char *sync_pipe_signame(int);
#endif

static gboolean normal_do_capture(gboolean is_tempfile);
static void capture_pcap_cb(guchar *, const struct pcap_pkthdr *,
  const guchar *);
static void get_capture_file_io_error(char *, int, const char *, int, gboolean);
static void popup_errmsg(const char *);
static void stop_capture_signal_handler(int signo);

#ifndef _WIN32
static void cap_pipe_adjust_header(loop_data *, struct pcap_hdr *, struct pcaprec_hdr *);
static int cap_pipe_open_live(char *, struct pcap_hdr *, loop_data *, char *, int);
static int cap_pipe_dispatch(int, loop_data *, struct pcap_hdr *, \
		struct pcaprec_modified_hdr *, guchar *, char *, int);
#endif






/* Open a specified file, or create a temporary file, and start a capture
   to the file in question.  Returns TRUE if the capture starts
   successfully, FALSE otherwise. */
gboolean
do_capture(const char *save_file)
{
  char tmpname[128+1];
  gboolean is_tempfile;
  gchar *capfile_name;
  gboolean ret;

  if (save_file != NULL) {
    /* If the Sync option is set, we return to the caller while the capture
     * is in progress.  Therefore we need to take a copy of save_file in
     * case the caller destroys it after we return.
     */
    capfile_name = g_strdup(save_file);
    if (capture_opts.multi_files_on) {
      /* ringbuffer is enabled */
      cfile.save_file_fd = ringbuf_init(capfile_name,
          (capture_opts.has_ring_num_files) ? capture_opts.ring_num_files : 0);
    } else {
      /* Try to open/create the specified file for use as a capture buffer. */
      cfile.save_file_fd = open(capfile_name, O_RDWR|O_BINARY|O_TRUNC|O_CREAT,
				0600);
    }
    is_tempfile = FALSE;
  } else {
    /* Choose a random name for the capture buffer */
    cfile.save_file_fd = create_tempfile(tmpname, sizeof tmpname, "ether");
    capfile_name = g_strdup(tmpname);
    is_tempfile = TRUE;
  }
  if (cfile.save_file_fd == -1) {
    if (is_tempfile) {
      simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK,
	"The temporary file to which the capture would be saved (\"%s\")"
	"could not be opened: %s.", capfile_name, strerror(errno));
    } else {
      if (capture_opts.multi_files_on) {
        ringbuf_error_cleanup();
      }
      open_failure_alert_box(capfile_name, errno, TRUE);
    }
    g_free(capfile_name);
    return FALSE;
  }
  cf_close(&cfile);
  g_assert(cfile.save_file == NULL);
  cfile.save_file = capfile_name;
  /* cfile.save_file is "g_free"ed below, which is equivalent to
     "g_free(capfile_name)". */
  fork_child = -1;

  if (capture_opts.sync_mode) {	
    /* sync mode: do the capture in a child process */
    ret = sync_pipe_do_capture(is_tempfile);
    /* capture is still running */
    set_main_window_name("(Live Capture in Progress) - Ethereal");
  } else {
    /* normal mode: do the capture synchronously */
    set_main_window_name("(Live Capture in Progress) - Ethereal");
    ret = normal_do_capture(is_tempfile);
    /* capture is finished here */
  }

  return ret;
}



/* Add a string pointer to a NULL-terminated array of string pointers. */
static char **
sync_pipe_add_arg(char **args, int *argc, char *arg)
{
  /* Grow the array; "*argc" currently contains the number of string
     pointers, *not* counting the NULL pointer at the end, so we have
     to add 2 in order to get the new size of the array, including the
     new pointer and the terminating NULL pointer. */
  args = g_realloc(args, (*argc + 2) * sizeof (char *));

  /* Stuff the pointer into the penultimate element of the array, which
     is the one at the index specified by "*argc". */
  args[*argc] = arg;

  /* Now bump the count. */
  (*argc)++;

  /* We overwrite the NULL pointer; put it back right after the
     element we added. */
  args[*argc] = NULL;

  return args;
}

#ifdef _WIN32
/* Given a string, return a pointer to a quote-encapsulated version of
   the string, so we can pass it as an argument with "spawnvp" even
   if it contains blanks. */
char *
sync_pipe_quote_encapsulate(const char *string)
{
  char *encapsulated_string;

  encapsulated_string = g_new(char, strlen(string) + 3);
  sprintf(encapsulated_string, "\"%s\"", string);
  return encapsulated_string;
}
#endif



static gboolean
sync_pipe_do_capture(gboolean is_tempfile) {
    guint byte_count;
    int  i;
    guchar  c;
    char *msg;
    int  err;
    char ssnap[24];
    char scount[24];			/* need a constant for len of numbers */
    char sautostop_filesize[24];	/* need a constant for len of numbers */
    char sautostop_duration[24];	/* need a constant for len of numbers */
    char save_file_fd[24];
#ifndef _WIN32
    char errmsg[1024+1];
#endif
    int error;
    int argc;
    char **argv;
#ifdef _WIN32
    char sync_pipe_fd[24];
    char *fontstring;
    char *filterstring;
#endif

    /* Allocate the string pointer array with enough space for the
       terminating NULL pointer. */
    argc = 0;
    argv = g_malloc(sizeof (char *));
    *argv = NULL;

    /* Now add those arguments used on all platforms. */
    argv = sync_pipe_add_arg(argv, &argc, CHILD_NAME);

    argv = sync_pipe_add_arg(argv, &argc, "-i");
    argv = sync_pipe_add_arg(argv, &argc, cfile.iface);

    argv = sync_pipe_add_arg(argv, &argc, "-w");
    argv = sync_pipe_add_arg(argv, &argc, cfile.save_file);

    argv = sync_pipe_add_arg(argv, &argc, "-W");
    sprintf(save_file_fd,"%d",cfile.save_file_fd);	/* in lieu of itoa */
    argv = sync_pipe_add_arg(argv, &argc, save_file_fd);

    if (capture_opts.has_autostop_packets) {
      argv = sync_pipe_add_arg(argv, &argc, "-c");
      sprintf(scount,"%d",capture_opts.autostop_packets);
      argv = sync_pipe_add_arg(argv, &argc, scount);
    }

    if (capture_opts.has_snaplen) {
      argv = sync_pipe_add_arg(argv, &argc, "-s");
      sprintf(ssnap,"%d",capture_opts.snaplen);
      argv = sync_pipe_add_arg(argv, &argc, ssnap);
    }

    if (capture_opts.linktype != -1) {
      argv = sync_pipe_add_arg(argv, &argc, "-y");
      sprintf(ssnap,"%d",capture_opts.linktype);
      argv = sync_pipe_add_arg(argv, &argc, ssnap);
    }

    if (capture_opts.has_autostop_filesize) {
      argv = sync_pipe_add_arg(argv, &argc, "-a");
      sprintf(sautostop_filesize,"filesize:%d",capture_opts.autostop_filesize);
      argv = sync_pipe_add_arg(argv, &argc, sautostop_filesize);
    }

    if (capture_opts.has_autostop_duration) {
      argv = sync_pipe_add_arg(argv, &argc, "-a");
      sprintf(sautostop_duration,"duration:%d",capture_opts.autostop_duration);
      argv = sync_pipe_add_arg(argv, &argc, sautostop_duration);
    }

    if (!capture_opts.show_info) {
      argv = sync_pipe_add_arg(argv, &argc, "-H");
    }

    if (!capture_opts.promisc_mode)
      argv = sync_pipe_add_arg(argv, &argc, "-p");

#ifdef _WIN32
    /* Create a pipe for the child process */

    if(_pipe(sync_pipe, 512, O_BINARY) < 0) {
      /* Couldn't create the pipe between parent and child. */
      error = errno;
      unlink(cfile.save_file);
      g_free(cfile.save_file);
      cfile.save_file = NULL;
      simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK, "Couldn't create sync pipe: %s",
                        strerror(error));
      return FALSE;
    }

    /* Convert font name to a quote-encapsulated string and pass to child */
    argv = sync_pipe_add_arg(argv, &argc, "-m");
    fontstring = sync_pipe_quote_encapsulate(prefs.PREFS_GUI_FONT_NAME);
    argv = sync_pipe_add_arg(argv, &argc, fontstring);

    /* Convert pipe write handle to a string and pass to child */
    argv = sync_pipe_add_arg(argv, &argc, "-Z");
    itoa(sync_pipe[WRITE], sync_pipe_fd, 10);
    argv = sync_pipe_add_arg(argv, &argc, sync_pipe_fd);

    /* Convert filter string to a quote delimited string and pass to child */
    filterstring = NULL;
    if (cfile.cfilter != NULL && strlen(cfile.cfilter) != 0) {
      argv = sync_pipe_add_arg(argv, &argc, "-f");
      filterstring = sync_pipe_quote_encapsulate(cfile.cfilter);
      argv = sync_pipe_add_arg(argv, &argc, filterstring);
    }

    /* Spawn process */
    fork_child = spawnvp(_P_NOWAIT, ethereal_path, argv);
    g_free(fontstring);
    if (filterstring) {
      g_free(filterstring);
    }
#else
    if (pipe(sync_pipe) < 0) {
      /* Couldn't create the pipe between parent and child. */
      error = errno;
      unlink(cfile.save_file);
      g_free(cfile.save_file);
      cfile.save_file = NULL;
      simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK, "Couldn't create sync pipe: %s",
			strerror(error));
      return FALSE;
    }

    argv = sync_pipe_add_arg(argv, &argc, "-m");
    argv = sync_pipe_add_arg(argv, &argc, prefs.PREFS_GUI_FONT_NAME);

    if (cfile.cfilter != NULL && strlen(cfile.cfilter) != 0) {
      argv = sync_pipe_add_arg(argv, &argc, "-f");
      argv = sync_pipe_add_arg(argv, &argc, cfile.cfilter);
    }

    if ((fork_child = fork()) == 0) {
      /*
       * Child process - run Ethereal with the right arguments to make
       * it just pop up the live capture dialog box and capture with
       * the specified capture parameters, writing to the specified file.
       *
       * args: -i interface specification
       * -w file to write
       * -W file descriptor to write
       * -c count to capture
       * -s snaplen
       * -m / -b fonts
       * -f "filter expression"
       */
      close(1);
      dup(sync_pipe[WRITE]);
      close(sync_pipe[READ]);
      execvp(ethereal_path, argv);
      snprintf(errmsg, sizeof errmsg, "Couldn't run %s in child process: %s",
		ethereal_path, strerror(errno));
      sync_pipe_errmsg_to_parent(errmsg);

      /* Exit with "_exit()", so that we don't close the connection
         to the X server (and cause stuff buffered up by our parent but
	 not yet sent to be sent, as that stuff should only be sent by
	 our parent). */
      _exit(2);
    }
#endif

    /* Parent process - read messages from the child process over the
       sync pipe. */
    g_free(argv);	/* free up arg array */

    /* Close the write side of the pipe, so that only the child has it
       open, and thus it completely closes, and thus returns to us
       an EOF indication, if the child closes it (either deliberately
       or by exiting abnormally). */
    close(sync_pipe[WRITE]);

    /* Close the save file FD, as we won't be using it - we'll be opening
       it and reading the save file through Wiretap. */
    close(cfile.save_file_fd);

    if (fork_child == -1) {
      /* We couldn't even create the child process. */
      error = errno;
      close(sync_pipe[READ]);
      unlink(cfile.save_file);
      g_free(cfile.save_file);
      cfile.save_file = NULL;
      simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK,
			"Couldn't create child process: %s", strerror(error));
      return FALSE;
    }

    /* Read a byte count from "sync_pipe[READ]", terminated with a
       colon; if the count is 0, the child process created the
       capture file and we should start reading from it, otherwise
       the capture couldn't start and the count is a count of bytes
       of error message, and we should display the message. */
    byte_count = 0;
    for (;;) {
      i = read(sync_pipe[READ], &c, 1);
      if (i == 0) {
	/* EOF - the child process died.
	   Close the read side of the sync pipe, remove the capture file,
	   and report the failure. */
	close(sync_pipe[READ]);
	unlink(cfile.save_file);
	g_free(cfile.save_file);
	cfile.save_file = NULL;
	sync_pipe_wait_for_child(TRUE);
	return FALSE;
      }
      if (c == SP_CAPSTART || c == SP_ERROR_MSG)
	break;
      if (!isdigit(c)) {
	/* Child process handed us crap.
	   Close the read side of the sync pipe, remove the capture file,
	   and report the failure. */
	close(sync_pipe[READ]);
	unlink(cfile.save_file);
	g_free(cfile.save_file);
	cfile.save_file = NULL;
	simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK,
			"Capture child process sent us a bad message");
	return FALSE;
      }
      byte_count = byte_count*10 + c - '0';
    }
    if (c != SP_CAPSTART) {
      /* Failure - the child process sent us a message indicating
	 what the problem was. */
      if (byte_count == 0) {
	/* Zero-length message? */
	simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK,
		"Capture child process failed, but its error message was empty.");
      } else {
	msg = g_malloc(byte_count + 1);
	if (msg == NULL) {
	  simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK,
		"Capture child process failed, but its error message was too big.");
	} else {
	  i = read(sync_pipe[READ], msg, byte_count);
	  msg[byte_count] = '\0';
	  if (i < 0) {
	    simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK,
		  "Capture child process failed: Error %s reading its error message.",
		  strerror(errno));
	  } else if (i == 0) {
	    simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK,
		  "Capture child process failed: EOF reading its error message.");
	    sync_pipe_wait_for_child(FALSE);
	  } else
	    simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK, msg);
	  g_free(msg);
	}

	/* Close the sync pipe. */
	close(sync_pipe[READ]);

	/* Get rid of the save file - the capture never started. */
	unlink(cfile.save_file);
	g_free(cfile.save_file);
	cfile.save_file = NULL;
      }
      return FALSE;
    }

    /* The child process started a capture.
       Attempt to open the capture file and set up to read it. */
    err = cf_start_tail(cfile.save_file, is_tempfile, &cfile);
    if (err != 0) {
      /* We weren't able to open the capture file; user has been
	 alerted. Close the sync pipe. */

      close(sync_pipe[READ]);

      /* Don't unlink the save file - leave it around, for debugging
	 purposes. */
      g_free(cfile.save_file);
      cfile.save_file = NULL;
      return FALSE;
    }
    /* We were able to open and set up to read the capture file;
       arrange that our callback be called whenever it's possible
       to read from the sync pipe, so that it's called when
       the child process wants to tell us something. */
    pipe_input_set_handler(sync_pipe[READ], (gpointer) &cfile, &fork_child, sync_pipe_input_cb);

    return TRUE;
}


/* There's stuff to read from the sync pipe, meaning the child has sent
   us a message, or the sync pipe has closed, meaning the child has
   closed it (perhaps because it exited). */
static gboolean 
sync_pipe_input_cb(gint source, gpointer user_data)
{
  capture_file *cf = (capture_file *)user_data;
#define BUFSIZE	4096
  char buffer[BUFSIZE+1], *p = buffer, *q = buffer, *msg, *r;
  int  nread, msglen, chars_to_copy;
  int  to_read = 0;
  int  err;


  if ((nread = read(source, buffer, BUFSIZE)) <= 0) {
    /* The child has closed the sync pipe, meaning it's not going to be
       capturing any more packets.  Pick up its exit status, and
       complain if it did anything other than exit with status 0. */
    sync_pipe_wait_for_child(FALSE);

    /* Read what remains of the capture file, and finish the capture.
       XXX - do something if this fails? */
    switch (cf_finish_tail(cf, &err)) {

    case READ_SUCCESS:
        if(cf->count == 0) {
          simple_dialog(ESD_TYPE_INFO, ESD_BTN_OK, 
          "%sNo packets captured!%s\n\n"
          "As no data was captured, closing the %scapture file!",
          simple_dialog_primary_start(), simple_dialog_primary_end(),
          (cf->is_tempfile) ? "temporary " : "");
          cf_close(cf);
        }
        break;
    case READ_ERROR:
      /* Just because we got an error, that doesn't mean we were unable
         to read any of the file; we handle what we could get from the
         file. */
      break;

    case READ_ABORTED:
      /* Exit by leaving the main loop, so that any quit functions
         we registered get called. */
      main_window_quit();
      return FALSE;
    }

    /* We're not doing a capture any more, so we don't have a save
       file. */
    g_free(cf->save_file);
    cf->save_file = NULL;

    return FALSE;
  }

  buffer[nread] = '\0';

  while (nread != 0) {
    /* look for (possibly multiple) indications */
    switch (*q) {
    case SP_PACKET_COUNT :
      to_read += atoi(p);
      p = q + 1;
      q++;
      nread--;
      break;
    case SP_DROPS :
      cf->drops_known = TRUE;
      cf->drops = atoi(p);
      p = q + 1;
      q++;
      nread--;
      break;
    case SP_ERROR_MSG :
      msglen = atoi(p);
      p = q + 1;
      q++;
      nread--;

      /* Read the entire message.
         XXX - if the child hasn't sent it all yet, this could cause us
         to hang until they do. */
      msg = g_malloc(msglen + 1);
      r = msg;
      while (msglen != 0) {
      	if (nread == 0) {
      	  /* Read more. */
          if ((nread = read(source, buffer, BUFSIZE)) <= 0)
            break;
          p = buffer;
          q = buffer;
        }
      	chars_to_copy = MIN(msglen, nread);
        memcpy(r, q, chars_to_copy);
        r += chars_to_copy;
        q += chars_to_copy;
        nread -= chars_to_copy;
        msglen -= chars_to_copy;
      }
      *r = '\0';
      simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK, msg);
      g_free(msg);
      break;
    default :
      q++;
      nread--;
      break;
    }
  }

  /* Read from the capture file the number of records the child told us
     it added.
     XXX - do something if this fails? */
  switch (cf_continue_tail(cf, to_read, &err)) {

  case READ_SUCCESS:
  case READ_ERROR:
    /* Just because we got an error, that doesn't mean we were unable
       to read any of the file; we handle what we could get from the
       file.

       XXX - abort on a read error? */
    break;

  case READ_ABORTED:
    /* Kill the child capture process; the user wants to exit, and we
       shouldn't just leave it running. */
    kill_capture_child();
    break;
  }

  return TRUE;
}

static void
sync_pipe_wait_for_child(gboolean always_report)
{
  int  wstatus;

#ifdef _WIN32
  /* XXX - analyze the wait status and display more information
     in the dialog box?
     XXX - set "fork_child" to -1 if we find it exited? */
  if (_cwait(&wstatus, fork_child, _WAIT_CHILD) == -1) {
    simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK,
		"Child capture process stopped unexpectedly");
  }
#else
  if (wait(&wstatus) != -1) {
    if (WIFEXITED(wstatus)) {
      /* The child exited; display its exit status, if it's not zero,
         and even if it's zero if "always_report" is true. */
      if (always_report || WEXITSTATUS(wstatus) != 0) {
        simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK,
		      "Child capture process exited: exit status %d",
		      WEXITSTATUS(wstatus));
      }
    } else if (WIFSTOPPED(wstatus)) {
      /* It stopped, rather than exiting.  "Should not happen." */
      simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK,
		    "Child capture process stopped: %s",
		    sync_pipe_signame(WSTOPSIG(wstatus)));
    } else if (WIFSIGNALED(wstatus)) {
      /* It died with a signal. */
      simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK,
		    "Child capture process died: %s%s",
		    sync_pipe_signame(WTERMSIG(wstatus)),
		    WCOREDUMP(wstatus) ? " - core dumped" : "");
    } else {
      /* What?  It had to either have exited, or stopped, or died with
         a signal; what happened here? */
      simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK,
		    "Child capture process died: wait status %#o", wstatus);
    }
  }

  /* No more child process. */
  fork_child = -1;
#endif
}

static void
sync_pipe_errmsg_to_parent(const char *errmsg)
{
    int msglen = strlen(errmsg);
    char lenbuf[SP_DECISIZE+1+1];

    sprintf(lenbuf, "%u%c", msglen, SP_ERROR_MSG);
    write(1, lenbuf, strlen(lenbuf));
    write(1, errmsg, msglen);
}

static void
sync_pipe_drops_to_parent(int drops)
{
	char tmp[SP_DECISIZE+1+1];
	sprintf(tmp, "%d%c", drops, SP_DROPS);
	write(1, tmp, strlen(tmp));
}

static void
sync_pipe_packet_count_to_parent(int packet_count)
{
    char tmp[SP_DECISIZE+1+1];
    sprintf(tmp, "%d%c", packet_count, SP_PACKET_COUNT);
    write(1, tmp, strlen(tmp));
}

static void
sync_pipe_capstart_to_parent(void)
{
    static const char capstart_msg = SP_CAPSTART;

    write(1, &capstart_msg, 1);
}

#ifndef _WIN32
static char *
sync_pipe_signame(int sig)
{
  char *sigmsg;
  static char sigmsg_buf[6+1+3+1];

  switch (sig) {

  case SIGHUP:
    sigmsg = "Hangup";
    break;

  case SIGINT:
    sigmsg = "Interrupted";
    break;

  case SIGQUIT:
    sigmsg = "Quit";
    break;

  case SIGILL:
    sigmsg = "Illegal instruction";
    break;

  case SIGTRAP:
    sigmsg = "Trace trap";
    break;

  case SIGABRT:
    sigmsg = "Abort";
    break;

  case SIGFPE:
    sigmsg = "Arithmetic exception";
    break;

  case SIGKILL:
    sigmsg = "Killed";
    break;

  case SIGBUS:
    sigmsg = "Bus error";
    break;

  case SIGSEGV:
    sigmsg = "Segmentation violation";
    break;

  /* http://metalab.unc.edu/pub/Linux/docs/HOWTO/GCC-HOWTO
     Linux is POSIX compliant.  These are not POSIX-defined signals ---
     ISO/IEC 9945-1:1990 (IEEE Std 1003.1-1990), paragraph B.3.3.1.1 sez:

	``The signals SIGBUS, SIGEMT, SIGIOT, SIGTRAP, and SIGSYS
	were omitted from POSIX.1 because their behavior is
	implementation dependent and could not be adequately catego-
	rized.  Conforming implementations may deliver these sig-
	nals, but must document the circumstances under which they
	are delivered and note any restrictions concerning their
	delivery.''

     So we only check for SIGSYS on those systems that happen to
     implement them (a system can be POSIX-compliant and implement
     them, it's just that POSIX doesn't *require* a POSIX-compliant
     system to implement them).
   */

#ifdef SIGSYS
  case SIGSYS:
    sigmsg = "Bad system call";
    break;
#endif

  case SIGPIPE:
    sigmsg = "Broken pipe";
    break;

  case SIGALRM:
    sigmsg = "Alarm clock";
    break;

  case SIGTERM:
    sigmsg = "Terminated";
    break;

  default:
    sprintf(sigmsg_buf, "Signal %d", sig);
    sigmsg = sigmsg_buf;
    break;
  }
  return sigmsg;
}
#endif






static gboolean
normal_do_capture(gboolean is_tempfile)
{
    int capture_succeeded;
    gboolean stats_known;
    struct pcap_stat stats;
    int err;

    /* Not sync mode. */
    capture_succeeded = capture(&stats_known, &stats);
    if (quit_after_cap) {
      /* DON'T unlink the save file.  Presumably someone wants it. */
        main_window_exit();
    }
    if (!capture_succeeded) {
      /* We didn't succeed in doing the capture, so we don't have a save
	 file. */
      if (capture_opts.multi_files_on) {
	ringbuf_free();
      } else {
	g_free(cfile.save_file);
      }
      cfile.save_file = NULL;
      return FALSE;
    }
    /* Capture succeeded; attempt to read in the capture file. */
    if ((err = cf_open(cfile.save_file, is_tempfile, &cfile)) != 0) {
      /* We're not doing a capture any more, so we don't have a save
	 file. */
      if (capture_opts.multi_files_on) {
	ringbuf_free();
      } else {
	g_free(cfile.save_file);
      }
      cfile.save_file = NULL;
      return FALSE;
    }

    /* Set the read filter to NULL. */
    cfile.rfcode = NULL;

    /* Get the packet-drop statistics.

       XXX - there are currently no packet-drop statistics stored
       in libpcap captures, and that's what we're reading.

       At some point, we will add support in Wiretap to return
       packet-drop statistics for capture file formats that store it,
       and will make "cf_read()" get those statistics from Wiretap.
       We clear the statistics (marking them as "not known") in
       "cf_open()", and "cf_read()" will only fetch them and mark
       them as known if Wiretap supplies them, so if we get the
       statistics now, after calling "cf_open()" but before calling
       "cf_read()", the values we store will be used by "cf_read()".

       If a future libpcap capture file format stores the statistics,
       we'll put them into the capture file that we write, and will
       thus not have to set them here - "cf_read()" will get them from
       the file and use them. */
    if (stats_known) {
      cfile.drops_known = TRUE;

      /* XXX - on some systems, libpcap doesn't bother filling in
         "ps_ifdrop" - it doesn't even set it to zero - so we don't
         bother looking at it.

         Ideally, libpcap would have an interface that gave us
         several statistics - perhaps including various interface
         error statistics - and would tell us which of them it
         supplies, allowing us to display only the ones it does. */
      cfile.drops = stats.ps_drop;
    }
    switch (cf_read(&cfile)) {

    case READ_SUCCESS:
    case READ_ERROR:
      /* Just because we got an error, that doesn't mean we were unable
         to read any of the file; we handle what we could get from the
         file. */
      break;

    case READ_ABORTED:
      /* Exit by leaving the main loop, so that any quit functions
         we registered get called. */
      main_window_nested_quit();
      return FALSE;
    }

    /* We're not doing a capture any more, so we don't have a save
       file. */
    if (capture_opts.multi_files_on) {
      ringbuf_free();
    } else {
      g_free(cfile.save_file);
    }
    cfile.save_file = NULL;

    /* if we didn't captured even a single packet, close the file again */
    if(cfile.count == 0) {
      simple_dialog(ESD_TYPE_INFO, ESD_BTN_OK, 
      "%sNo packets captured!%s\n\n"
      "As no data was captured, closing the %scapture file!",
      simple_dialog_primary_start(), simple_dialog_primary_end(),
      (cfile.is_tempfile) ? "temporary " : "");
      cf_close(&cfile);
    }
  return TRUE;
}

/*
 * Timeout, in milliseconds, for reads from the stream of captured packets.
 */
#define	CAP_READ_TIMEOUT	250

#ifndef _WIN32
/* Take care of byte order in the libpcap headers read from pipes.
 * (function taken from wiretap/libpcap.c) */
static void
cap_pipe_adjust_header(loop_data *ld, struct pcap_hdr *hdr, struct pcaprec_hdr *rechdr)
{
  if (ld->byte_swapped) {
    /* Byte-swap the record header fields. */
    rechdr->ts_sec = BSWAP32(rechdr->ts_sec);
    rechdr->ts_usec = BSWAP32(rechdr->ts_usec);
    rechdr->incl_len = BSWAP32(rechdr->incl_len);
    rechdr->orig_len = BSWAP32(rechdr->orig_len);
  }

  /* In file format version 2.3, the "incl_len" and "orig_len" fields were
     swapped, in order to match the BPF header layout.

     Unfortunately, some files were, according to a comment in the "libpcap"
     source, written with version 2.3 in their headers but without the
     interchanged fields, so if "incl_len" is greater than "orig_len" - which
     would make no sense - we assume that we need to swap them.  */
  if (hdr->version_major == 2 &&
      (hdr->version_minor < 3 ||
       (hdr->version_minor == 3 && rechdr->incl_len > rechdr->orig_len))) {
    guint32 temp;

    temp = rechdr->orig_len;
    rechdr->orig_len = rechdr->incl_len;
    rechdr->incl_len = temp;
  }
}

/* Mimic pcap_open_live() for pipe captures
 * We check if "pipename" is "-" (stdin) or a FIFO, open it, and read the
 * header.
 * N.B. : we can't read the libpcap formats used in RedHat 6.1 or SuSE 6.3
 * because we can't seek on pipes (see wiretap/libpcap.c for details) */
static int
cap_pipe_open_live(char *pipename, struct pcap_hdr *hdr, loop_data *ld,
                 char *errmsg, int errmsgl)
{
  struct stat pipe_stat;
  int         fd;
  guint32     magic;
  int         b, sel_ret;
  unsigned int bytes_read;
  fd_set      rfds;
  struct timeval timeout;

  /*
   * XXX Ethereal blocks until we return
   */
  if (strcmp(pipename, "-") == 0)
    fd = 0; /* read from stdin */
  else {
    if (stat(pipename, &pipe_stat) < 0) {
      if (errno == ENOENT || errno == ENOTDIR)
        ld->cap_pipe_err = PIPNEXIST;
      else {
        snprintf(errmsg, errmsgl,
          "The capture session could not be initiated "
          "due to error on pipe: %s", strerror(errno));
        ld->cap_pipe_err = PIPERR;
      }
      return -1;
    }
    if (! S_ISFIFO(pipe_stat.st_mode)) {
      if (S_ISCHR(pipe_stat.st_mode)) {
        /*
         * Assume the user specified an interface on a system where
         * interfaces are in /dev.  Pretend we haven't seen it.
         */
         ld->cap_pipe_err = PIPNEXIST;
      } else {
        snprintf(errmsg, errmsgl,
            "The capture session could not be initiated because\n"
            "\"%s\" is neither an interface nor a pipe", pipename);
        ld->cap_pipe_err = PIPERR;
      }
      return -1;
    }
    fd = open(pipename, O_RDONLY | O_NONBLOCK);
    if (fd == -1) {
      snprintf(errmsg, errmsgl,
          "The capture session could not be initiated "
          "due to error on pipe open: %s", strerror(errno));
      ld->cap_pipe_err = PIPERR;
      return -1;
    }
  }

  ld->from_cap_pipe = TRUE;

  /* read the pcap header */
  FD_ZERO(&rfds);
  bytes_read = 0;
  while (bytes_read < sizeof magic) {
    FD_SET(fd, &rfds);
    timeout.tv_sec = 0;
    timeout.tv_usec = CAP_READ_TIMEOUT*1000;
    sel_ret = select(fd+1, &rfds, NULL, NULL, &timeout);
    if (sel_ret < 0) {
      snprintf(errmsg, errmsgl,
        "Unexpected error from select: %s", strerror(errno));
      goto error;
    } else if (sel_ret > 0) {
      b = read(fd, ((char *)&magic)+bytes_read, sizeof magic-bytes_read);
      if (b <= 0) {
        if (b == 0)
          snprintf(errmsg, errmsgl, "End of file on pipe during open");
        else
          snprintf(errmsg, errmsgl, "Error on pipe during open: %s",
            strerror(errno));
        goto error;
      }
      bytes_read += b;
    }
  }

  switch (magic) {
  case PCAP_MAGIC:
    /* Host that wrote it has our byte order, and was running
       a program using either standard or ss990417 libpcap. */
    ld->byte_swapped = FALSE;
    ld->modified = FALSE;
    break;
  case PCAP_MODIFIED_MAGIC:
    /* Host that wrote it has our byte order, but was running
       a program using either ss990915 or ss991029 libpcap. */
    ld->byte_swapped = FALSE;
    ld->modified = TRUE;
    break;
  case PCAP_SWAPPED_MAGIC:
    /* Host that wrote it has a byte order opposite to ours,
       and was running a program using either standard or
       ss990417 libpcap. */
    ld->byte_swapped = TRUE;
    ld->modified = FALSE;
    break;
  case PCAP_SWAPPED_MODIFIED_MAGIC:
    /* Host that wrote it out has a byte order opposite to
       ours, and was running a program using either ss990915
       or ss991029 libpcap. */
    ld->byte_swapped = TRUE;
    ld->modified = TRUE;
    break;
  default:
    /* Not a "libpcap" type we know about. */
    snprintf(errmsg, errmsgl, "Unrecognized libpcap format");
    goto error;
  }

  /* Read the rest of the header */
  bytes_read = 0;
  while (bytes_read < sizeof(struct pcap_hdr)) {
    FD_SET(fd, &rfds);
    timeout.tv_sec = 0;
    timeout.tv_usec = CAP_READ_TIMEOUT*1000;
    sel_ret = select(fd+1, &rfds, NULL, NULL, &timeout);
    if (sel_ret < 0) {
      snprintf(errmsg, errmsgl,
        "Unexpected error from select: %s", strerror(errno));
      goto error;
    } else if (sel_ret > 0) {
      b = read(fd, ((char *)hdr)+bytes_read,
            sizeof(struct pcap_hdr) - bytes_read);
      if (b <= 0) {
        if (b == 0)
          snprintf(errmsg, errmsgl, "End of file on pipe during open");
        else
          snprintf(errmsg, errmsgl, "Error on pipe during open: %s",
            strerror(errno));
        goto error;
      }
      bytes_read += b;
    }
  }

  if (ld->byte_swapped) {
    /* Byte-swap the header fields about which we care. */
    hdr->version_major = BSWAP16(hdr->version_major);
    hdr->version_minor = BSWAP16(hdr->version_minor);
    hdr->snaplen = BSWAP32(hdr->snaplen);
    hdr->network = BSWAP32(hdr->network);
  }

  if (hdr->version_major < 2) {
    snprintf(errmsg, errmsgl, "Unable to read old libpcap format");
    goto error;
  }

  ld->cap_pipe_state = STATE_EXPECT_REC_HDR;
  ld->cap_pipe_err = PIPOK;
  return fd;

error:
  ld->cap_pipe_err = PIPERR;
  close(fd);
  return -1;

}

/* We read one record from the pipe, take care of byte order in the record
 * header, write the record in the capture file, and update capture statistics. */

static int
cap_pipe_dispatch(int fd, loop_data *ld, struct pcap_hdr *hdr,
		struct pcaprec_modified_hdr *rechdr, guchar *data,
		char *errmsg, int errmsgl)
{
  struct pcap_pkthdr phdr;
  int b;
  enum { PD_REC_HDR_READ, PD_DATA_READ, PD_PIPE_EOF, PD_PIPE_ERR,
          PD_ERR } result;

  switch (ld->cap_pipe_state) {

  case STATE_EXPECT_REC_HDR:
    ld->bytes_to_read = ld->modified ?
      sizeof(struct pcaprec_modified_hdr) : sizeof(struct pcaprec_hdr);
    ld->bytes_read = 0;
    ld->cap_pipe_state = STATE_READ_REC_HDR;
    /* Fall through */

  case STATE_READ_REC_HDR:
    b = read(fd, ((char *)rechdr)+ld->bytes_read,
      ld->bytes_to_read - ld->bytes_read);
    if (b <= 0) {
      if (b == 0)
        result = PD_PIPE_EOF;
      else
        result = PD_PIPE_ERR;
      break;
    }
    if ((ld->bytes_read += b) < ld->bytes_to_read)
        return 0;
    result = PD_REC_HDR_READ;
    break;

  case STATE_EXPECT_DATA:
    ld->bytes_read = 0;
    ld->cap_pipe_state = STATE_READ_DATA;
    /* Fall through */

  case STATE_READ_DATA:
    b = read(fd, data+ld->bytes_read, rechdr->hdr.incl_len - ld->bytes_read);
    if (b <= 0) {
      if (b == 0)
        result = PD_PIPE_EOF;
      else
        result = PD_PIPE_ERR;
      break;
    }
    if ((ld->bytes_read += b) < rechdr->hdr.incl_len)
      return 0;
    result = PD_DATA_READ;
    break;

  default:
    snprintf(errmsg, errmsgl, "cap_pipe_dispatch: invalid state");
    result = PD_ERR;

  } /* switch (ld->cap_pipe_state) */

  /*
   * We've now read as much data as we were expecting, so process it.
   */
  switch (result) {

  case PD_REC_HDR_READ:
    /* We've read the header. Take care of byte order. */
    cap_pipe_adjust_header(ld, hdr, &rechdr->hdr);
    if (rechdr->hdr.incl_len > WTAP_MAX_PACKET_SIZE) {
      snprintf(errmsg, errmsgl, "Frame %u too long (%d bytes)",
        ld->counts.total+1, rechdr->hdr.incl_len);
      break;
    }
    ld->cap_pipe_state = STATE_EXPECT_DATA;
    return 0;

  case PD_DATA_READ:
    /* Fill in a "struct pcap_pkthdr", and process the packet. */
    phdr.ts.tv_sec = rechdr->hdr.ts_sec;
    phdr.ts.tv_usec = rechdr->hdr.ts_usec;
    phdr.caplen = rechdr->hdr.incl_len;
    phdr.len = rechdr->hdr.orig_len;

    capture_pcap_cb((guchar *)ld, &phdr, data);

    ld->cap_pipe_state = STATE_EXPECT_REC_HDR;
    return 1;

  case PD_PIPE_EOF:
    ld->cap_pipe_err = PIPEOF;
    return -1;

  case PD_PIPE_ERR:
    snprintf(errmsg, errmsgl, "Error reading from pipe: %s",
      strerror(errno));
    /* Fall through */
  case PD_ERR:
    break;
  }

  ld->cap_pipe_err = PIPERR;
  /* Return here rather than inside the switch to prevent GCC warning */
  return -1;
}
#endif /* not _WIN32 */

/*
 * This needs to be static, so that the SIGUSR1 handler can clear the "go"
 * flag.
 */
static loop_data   ld;

/* Do the low-level work of a capture.
   Returns TRUE if it succeeds, FALSE otherwise. */
int
capture(gboolean *stats_known, struct pcap_stat *stats)
{
  pcap_t     *pch;
  int         pcap_encap;
  int         file_snaplen;
  gchar       open_err_str[PCAP_ERRBUF_SIZE];
  gchar       lookup_net_err_str[PCAP_ERRBUF_SIZE];
  bpf_u_int32 netnum, netmask;
  struct bpf_program fcode;
  const char *set_linktype_err_str;
  time_t      upd_time, cur_time;
  time_t      start_time;
  int         err, inpkts;
  condition  *cnd_file_duration = NULL;
  condition  *cnd_autostop_files = NULL;
  condition  *cnd_autostop_size = NULL;
  condition  *cnd_autostop_duration = NULL;
  guint32     autostop_files = 0;
  char        errmsg[4096+1];
  gboolean    write_ok;
  gboolean    close_ok;
  capture_info   capture_ui;

#ifdef _WIN32
  WORD        wVersionRequested;
  WSADATA     wsaData;
#else
  fd_set      set1;
  struct timeval timeout;
  static const char ppamsg[] = "can't find PPA for ";
  char       *libpcap_warn;
  int         sel_ret;
  int         pipe_fd = -1;
  struct pcap_hdr hdr;
  struct pcaprec_modified_hdr rechdr;
  guchar pcap_data[WTAP_MAX_PACKET_SIZE];
#endif
#ifdef MUST_DO_SELECT
  int         pcap_fd = 0;
#endif
  gboolean    show_info = capture_opts.show_info || !capture_opts.sync_mode;

  /* Initialize Windows Socket if we are in a WIN32 OS
     This needs to be done before querying the interface for network/netmask */
#ifdef _WIN32
  /* XXX - do we really require 1.1 or earlier?
     Are there any versions that support only 2.0 or higher? */
  wVersionRequested = MAKEWORD(1, 1);
  err = WSAStartup(wVersionRequested, &wsaData);
  if (err != 0) {
    switch (err) {

    case WSASYSNOTREADY:
      snprintf(errmsg, sizeof errmsg,
        "Couldn't initialize Windows Sockets: Network system not ready for network communication");
      break;

    case WSAVERNOTSUPPORTED:
      snprintf(errmsg, sizeof errmsg,
        "Couldn't initialize Windows Sockets: Windows Sockets version %u.%u not supported",
        LOBYTE(wVersionRequested), HIBYTE(wVersionRequested));
      break;

    case WSAEINPROGRESS:
      snprintf(errmsg, sizeof errmsg,
        "Couldn't initialize Windows Sockets: Blocking operation is in progress");
      break;

    case WSAEPROCLIM:
      snprintf(errmsg, sizeof errmsg,
        "Couldn't initialize Windows Sockets: Limit on the number of tasks supported by this WinSock implementation has been reached");
      break;

    case WSAEFAULT:
      snprintf(errmsg, sizeof errmsg,
        "Couldn't initialize Windows Sockets: Bad pointer passed to WSAStartup");
      break;

    default:
      snprintf(errmsg, sizeof errmsg,
        "Couldn't initialize Windows Sockets: error %d", err);
      break;
    }
    pch = NULL;
    goto error;
  }
#endif

  ld.go             = TRUE;
  ld.counts.total   = 0;
  if (capture_opts.has_autostop_packets)
    ld.max          = capture_opts.autostop_packets;
  else
    ld.max          = 0;	/* no limit */
  ld.err            = 0;	/* no error seen yet */
  ld.linktype       = WTAP_ENCAP_UNKNOWN;
  ld.pcap_err       = FALSE;
  ld.from_cap_pipe  = FALSE;
  ld.sync_packets   = 0;
  ld.counts.sctp    = 0;
  ld.counts.tcp     = 0;
  ld.counts.udp     = 0;
  ld.counts.icmp    = 0;
  ld.counts.ospf    = 0;
  ld.counts.gre     = 0;
  ld.counts.ipx     = 0;
  ld.counts.netbios = 0;
  ld.counts.vines   = 0;
  ld.counts.other   = 0;
  ld.counts.arp     = 0;
  ld.pdh            = NULL;

  /* We haven't yet gotten the capture statistics. */
  *stats_known      = FALSE;

  /* Open the network interface to capture from it.
     Some versions of libpcap may put warnings into the error buffer
     if they succeed; to tell if that's happened, we have to clear
     the error buffer, and check if it's still a null string.  */
  open_err_str[0] = '\0';
  pch = pcap_open_live(cfile.iface,
		       capture_opts.has_snaplen ? capture_opts.snaplen :
						  WTAP_MAX_PACKET_SIZE,
		       capture_opts.promisc_mode, CAP_READ_TIMEOUT,
		       open_err_str);

  if (pch != NULL) {
#ifdef _WIN32
    /* try to set the capture buffer size */
    if (pcap_setbuff(pch, capture_opts.buffer_size * 1024 * 1024) != 0) {
        simple_dialog(ESD_TYPE_INFO, ESD_BTN_OK,
          "%sCouldn't set the capture buffer size!%s\n"
          "\n"
          "The capture buffer size of %luMB seems to be too high for your machine,\n"
          "the default of 1MB will be used.\n"
          "\n"
          "Nonetheless, the capture is started.\n",
          simple_dialog_primary_start(), simple_dialog_primary_end(), capture_opts.buffer_size);
    }
#endif

    /* setting the data link type only works on real interfaces */
    if (capture_opts.linktype != -1) {
      set_linktype_err_str = set_pcap_linktype(pch, cfile.iface,
	capture_opts.linktype);
      if (set_linktype_err_str != NULL) {
	snprintf(errmsg, sizeof errmsg, "Unable to set data link type (%s).",
	  set_linktype_err_str);
	goto error;
      }
    }
  } else {
    /* We couldn't open "cfile.iface" as a network device. */
#ifdef _WIN32
    /* On Windows, we don't support capturing on pipes, so we give up.
       If this is a child process that does the capturing in sync
       mode or fork mode, it shouldn't do any UI stuff until we pop up the
       capture-progress window, and, since we couldn't start the
       capture, we haven't popped it up. */
    if (!capture_child) {
      main_window_update();
    }

    /* On Win32 OSes, the capture devices are probably available to all
       users; don't warn about permissions problems.

       Do, however, warn that WAN devices aren't supported. */
    snprintf(errmsg, sizeof errmsg,
	"The capture session could not be initiated (%s).\n"
	"Please check that you have the proper interface specified.\n"
	"\n"
	"Note that the WinPcap 2.x version of the driver Ethereal uses for packet\n"
	"capture on Windows doesn't support capturing on PPP/WAN interfaces in\n"
	"Windows NT/2000/XP/2003 Server, and that the WinPcap 3.0 and later versions\n"
	"don't support capturing on PPP/WAN interfaces at all.",
	open_err_str);
    goto error;
#else
    /* try to open cfile.iface as a pipe */
    pipe_fd = cap_pipe_open_live(cfile.iface, &hdr, &ld, errmsg, sizeof errmsg);

    if (pipe_fd == -1) {

      /* If this is a child process that does the capturing in sync
       * mode or fork mode, it shouldn't do any UI stuff until we pop up the
       * capture-progress window, and, since we couldn't start the
       * capture, we haven't popped it up.
       */
      if (!capture_child) {
          main_window_update();
      }

      if (ld.cap_pipe_err == PIPNEXIST) {
	/* Pipe doesn't exist, so output message for interface */

	/* If we got a "can't find PPA for XXX" message, warn the user (who
	   is running Ethereal on HP-UX) that they don't have a version
	   of libpcap that properly handles HP-UX (libpcap 0.6.x and later
	   versions, which properly handle HP-UX, say "can't find /dev/dlpi
	   PPA for XXX" rather than "can't find PPA for XXX"). */
	if (strncmp(open_err_str, ppamsg, sizeof ppamsg - 1) == 0)
	  libpcap_warn =
	    "\n\n"
	    "You are running Ethereal with a version of the libpcap library\n"
	    "that doesn't handle HP-UX network devices well; this means that\n"
	    "Ethereal may not be able to capture packets.\n"
	    "\n"
	    "To fix this, you should install libpcap 0.6.2, or a later version\n"
	    "of libpcap, rather than libpcap 0.4 or 0.5.x.  It is available in\n"
	    "packaged binary form from the Software Porting And Archive Centre\n"
	    "for HP-UX; the Centre is at http://hpux.connect.org.uk/ - the page\n"
	    "at the URL lists a number of mirror sites.";
	else
	  libpcap_warn = "";
	snprintf(errmsg, sizeof errmsg,
	  "The capture session could not be initiated (%s).\n"
	  "Please check to make sure you have sufficient permissions, and that\n"
	  "you have the proper interface or pipe specified.%s", open_err_str,
	  libpcap_warn);
      }
      /*
       * Else pipe (or file) does exist and cap_pipe_open_live() has
       * filled in errmsg
       */
      goto error;
    } else
      /* cap_pipe_open_live() succeeded; don't want
         error message from pcap_open_live() */
      open_err_str[0] = '\0';
#endif
  }

  /* capture filters only work on real interfaces */
  if (cfile.cfilter && !ld.from_cap_pipe) {
    /* A capture filter was specified; set it up. */
    if (pcap_lookupnet(cfile.iface, &netnum, &netmask, lookup_net_err_str) < 0) {
      /*
       * Well, we can't get the netmask for this interface; it's used
       * only for filters that check for broadcast IP addresses, so
       * we just punt and use 0.  It might be nice to warn the user,
       * but that's a pain in a GUI application, as it'd involve popping
       * up a message box, and it's not clear how often this would make
       * a difference (only filters that check for IP broadcast addresses
       * use the netmask).
       */
      netmask = 0;
    }
    if (pcap_compile(pch, &fcode, cfile.cfilter, 1, netmask) < 0) {
      dfilter_t   *rfcode = NULL;
      /* filter string invalid, did the user tried a display filter? */
      if (dfilter_compile(cfile.cfilter, &rfcode) && rfcode != NULL) {
        snprintf(errmsg, sizeof errmsg,
          "%sInvalid capture filter: \"%s\"!%s\n"
          "\n"
          "That string looks like a valid display filter; however, it is not a valid\n"
          "capture filter (%s).\n"
          "\n"
          "Note that display filters and capture filters don't have the same syntax,\n"
          "so you can't use most display filter expressions as capture filters.\n"
          "\n"
          "See the help for a description of the capture filter syntax.",
          simple_dialog_primary_start(), cfile.cfilter, simple_dialog_primary_end(),
          pcap_geterr(pch));
	dfilter_free(rfcode);
      } else {
        snprintf(errmsg, sizeof errmsg,
          "%sInvalid capture filter: \"%s\"!%s\n"
          "\n"
          "That string is not a valid capture filter (%s).\n"
          "See the help for a description of the capture filter syntax.",
          simple_dialog_primary_start(), cfile.cfilter, simple_dialog_primary_end(),
          pcap_geterr(pch));
      }
      goto error;
    }
    if (pcap_setfilter(pch, &fcode) < 0) {
      snprintf(errmsg, sizeof errmsg, "Can't install filter (%s).",
	pcap_geterr(pch));
      goto error;
    }
  }

  /* Set up to write to the capture file. */
#ifndef _WIN32
  if (ld.from_cap_pipe) {
    pcap_encap = hdr.network;
    file_snaplen = hdr.snaplen;
  } else
#endif
  {
    pcap_encap = get_pcap_linktype(pch, cfile.iface);
    file_snaplen = pcap_snapshot(pch);
  }
  ld.linktype = wtap_pcap_encap_to_wtap_encap(pcap_encap);
  if (ld.linktype == WTAP_ENCAP_UNKNOWN) {
    snprintf(errmsg, sizeof errmsg,
	"The network you're capturing from is of a type"
	" that Ethereal doesn't support (data link type %d).", pcap_encap);
    goto error;
  }
  if (capture_opts.multi_files_on) {
    ld.pdh = ringbuf_init_wtap_dump_fdopen(WTAP_FILE_PCAP, ld.linktype,
      file_snaplen, &err);
  } else {
    ld.pdh = wtap_dump_fdopen(cfile.save_file_fd, WTAP_FILE_PCAP,
      ld.linktype, file_snaplen, &err);
  }

  if (ld.pdh == NULL) {
    /* We couldn't set up to write to the capture file. */
    switch (err) {

    case WTAP_ERR_CANT_OPEN:
      strcpy(errmsg, "The file to which the capture would be saved"
               " couldn't be created for some unknown reason.");
      break;

    case WTAP_ERR_SHORT_WRITE:
      strcpy(errmsg, "A full header couldn't be written to the file"
               " to which the capture would be saved.");
      break;

    default:
      if (err < 0) {
        snprintf(errmsg, sizeof(errmsg),
		     "The file to which the capture would be"
                     " saved (\"%s\") could not be opened: Error %d.",
 			cfile.save_file, err);
      } else {
        snprintf(errmsg, sizeof(errmsg),
		     "The file to which the capture would be"
                     " saved (\"%s\") could not be opened: %s.",
 			cfile.save_file, strerror(err));
      }
      break;
    }
    goto error;
  }

  /* Does "open_err_str" contain a non-empty string?  If so, "pcap_open_live()"
     returned a warning; print it, but keep capturing. */
  if (open_err_str[0] != '\0')
    g_warning("%s.", open_err_str);

  /* XXX - capture SIGTERM and close the capture, in case we're on a
     Linux 2.0[.x] system and you have to explicitly close the capture
     stream in order to turn promiscuous mode off?  We need to do that
     in other places as well - and I don't think that works all the
     time in any case, due to libpcap bugs. */

  if (capture_child) {
    /* Well, we should be able to start capturing.

       This is the child process for a sync mode capture, so sync out
       the capture file, so the header makes it to the file system,
       and send a "capture started successfully and capture file created"
       message to our parent so that they'll open the capture file and
       update its windows to indicate that we have a live capture in
       progress. */
    fflush(wtap_dump_file(ld.pdh));
    sync_pipe_capstart_to_parent();
  }

  /* start capture info dialog */
  if(show_info) {
      capture_ui.callback_data  = &ld;
      capture_ui.counts         = &ld.counts;
      capture_info_create(&capture_ui, cfile.iface);
  }

  start_time = time(NULL);
  upd_time = time(NULL);
#ifdef MUST_DO_SELECT
  if (!ld.from_cap_pipe) pcap_fd = pcap_fileno(pch);
#endif

#ifndef _WIN32
  /*
   * Catch SIGUSR1, so that we exit cleanly if the parent process
   * kills us with it due to the user selecting "Capture->Stop".
   */
  if (capture_child)
    signal(SIGUSR1, stop_capture_signal_handler);
#endif

  /* initialize capture stop conditions */
  init_capture_stop_conditions();
  /* create stop conditions */
  if (capture_opts.has_autostop_filesize)
    cnd_autostop_size =
        cnd_new(CND_CLASS_CAPTURESIZE,(long)capture_opts.autostop_filesize);
  if (capture_opts.has_autostop_duration)
    cnd_autostop_duration =
        cnd_new(CND_CLASS_TIMEOUT,(gint32)capture_opts.autostop_duration);

  if (capture_opts.multi_files_on) {
      if (capture_opts.has_file_duration)
        cnd_file_duration =
	    cnd_new(CND_CLASS_TIMEOUT, capture_opts.file_duration);

      if (capture_opts.has_autostop_files)
        cnd_autostop_files =
	    cnd_new(CND_CLASS_CAPTURESIZE, capture_opts.autostop_files);
  }

  /* WOW, everything is prepared! */
  /* please fasten your seat belts, we will enter now the actual capture loop */
  while (ld.go) {
    main_window_update();

#ifndef _WIN32
    if (ld.from_cap_pipe) {
      FD_ZERO(&set1);
      FD_SET(pipe_fd, &set1);
      timeout.tv_sec = 0;
      timeout.tv_usec = CAP_READ_TIMEOUT*1000;
      sel_ret = select(pipe_fd+1, &set1, NULL, NULL, &timeout);
      if (sel_ret <= 0) {
	inpkts = 0;
        if (sel_ret < 0 && errno != EINTR) {
          snprintf(errmsg, sizeof(errmsg),
            "Unexpected error from select: %s", strerror(errno));
          popup_errmsg(errmsg);
          ld.go = FALSE;
        }
      } else {
	/*
	 * "select()" says we can read from the pipe without blocking
	 */
	inpkts = cap_pipe_dispatch(pipe_fd, &ld, &hdr, &rechdr, pcap_data,
          errmsg, sizeof errmsg);
	if (inpkts < 0) {
	  ld.go = FALSE;
        }
      }
    }
    else
#endif /* _WIN32 */
    {
#ifdef MUST_DO_SELECT
      /*
       * Sigh.  The semantics of the read timeout argument to
       * "pcap_open_live()" aren't particularly well specified by
       * the "pcap" man page - at least with the BSD BPF code, the
       * intent appears to be, at least in part, a way of cutting
       * down the number of reads done on a capture, by blocking
       * until the buffer fills or a timer expires - and the Linux
       * libpcap doesn't actually support it, so we can't use it
       * to break out of the "pcap_dispatch()" every 1/4 of a second
       * or so.  Linux's libpcap is not the only libpcap that doesn't
       * support the read timeout.
       *
       * Furthermore, at least on Solaris, the bufmod STREAMS module's
       * read timeout won't go off if no data has arrived, i.e. it cannot
       * be used to guarantee that a read from a DLPI stream will return
       * within a specified amount of time regardless of whether any
       * data arrives or not.
       *
       * Thus, on all platforms other than BSD, we do a "select()" on the
       * file descriptor for the capture, with a timeout of CAP_READ_TIMEOUT
       * milliseconds, or CAP_READ_TIMEOUT*1000 microseconds.
       *
       * "select()", on BPF devices, doesn't work as you might expect;
       * at least on some versions of some flavors of BSD, the timer
       * doesn't start until a read is done, so it won't expire if
       * only a "select()" or "poll()" is posted.
       */
      FD_ZERO(&set1);
      FD_SET(pcap_fd, &set1);
      timeout.tv_sec = 0;
      timeout.tv_usec = CAP_READ_TIMEOUT*1000;
      sel_ret = select(pcap_fd+1, &set1, NULL, NULL, &timeout);
      if (sel_ret > 0) {
	/*
	 * "select()" says we can read from it without blocking; go for
	 * it.
	 */
	inpkts = pcap_dispatch(pch, 1, capture_pcap_cb, (guchar *) &ld);
	if (inpkts < 0) {
	  ld.pcap_err = TRUE;
	  ld.go = FALSE;
	}
      } else {
        inpkts = 0;
        if (sel_ret < 0 && errno != EINTR) {
          snprintf(errmsg, sizeof(errmsg),
            "Unexpected error from select: %s", strerror(errno));
          popup_errmsg(errmsg);
          ld.go = FALSE;
        }
      }
#else
      inpkts = pcap_dispatch(pch, 1, capture_pcap_cb, (guchar *) &ld);
      if (inpkts < 0) {
        ld.pcap_err = TRUE;
        ld.go = FALSE;
      }
#endif /* MUST_DO_SELECT */
    }

    if (inpkts > 0) {
      ld.sync_packets += inpkts;

      /* check capture size condition */
      if (cnd_autostop_size != NULL && cnd_eval(cnd_autostop_size,
                    (guint32)wtap_get_bytes_dumped(ld.pdh))){
        /* Capture size limit reached, do we have another file? */
        if (capture_opts.multi_files_on) {
          if (cnd_autostop_files != NULL && cnd_eval(cnd_autostop_files, ++autostop_files)) {
            /* no files left: stop here */
            ld.go = FALSE;
            continue;
          }

          /* Switch to the next ringbuffer file */
          if (ringbuf_switch_file(&cfile, &ld.pdh, &ld.err)) {
            /* File switch succeeded: reset the conditions */
            cnd_reset(cnd_autostop_size);
            if (cnd_file_duration) {
              cnd_reset(cnd_file_duration);
            }
          } else {
            /* File switch failed: stop here */
            ld.go = FALSE;
            continue;
          }
        } else {
          /* single file, stop now */
          ld.go = FALSE;
          continue;
        }
      } /* cnd_autostop_size */
    }

    /* Only update once a second so as not to overload slow displays */
    cur_time = time(NULL);
    if (cur_time > upd_time) {
      upd_time = cur_time;

      /*if (pcap_stats(pch, stats) >= 0) {
        *stats_known = TRUE;
      }*/

      	/* Let the parent process know. */
      /* calculate and display running time */
      if(show_info) {
          cur_time -= start_time;
          capture_ui.running_time   = cur_time;
          capture_ui.new_packets    = ld.sync_packets;
          capture_info_update(&capture_ui);
      }

      if (ld.sync_packets) {
        /* do sync here */
        fflush(wtap_dump_file(ld.pdh));

        if (capture_child) {
	  /* This is the child process for a sync mode capture, so send
	     our parent a message saying we've written out "ld.sync_packets"
	     packets to the capture file. */
        sync_pipe_packet_count_to_parent(ld.sync_packets);
        }

        ld.sync_packets = 0;
      }

      /* check capture duration condition */
      if (cnd_autostop_duration != NULL && cnd_eval(cnd_autostop_duration)) {
        /* The maximum capture time has elapsed; stop the capture. */
        ld.go = FALSE;
        continue;
      }
      
      /* check capture file duration condition */
      if (cnd_file_duration != NULL && cnd_eval(cnd_file_duration)) {
        /* duration limit reached, do we have another file? */
        if (capture_opts.multi_files_on) {
          if (cnd_autostop_files != NULL && cnd_eval(cnd_autostop_files, ++autostop_files)) {
            /* no files left: stop here */
            ld.go = FALSE;
            continue;
          }

          /* Switch to the next ringbuffer file */
          if (ringbuf_switch_file(&cfile, &ld.pdh, &ld.err)) {
            /* file switch succeeded: reset the conditions */
            cnd_reset(cnd_file_duration);
            if(cnd_autostop_size)
              cnd_reset(cnd_autostop_size);
          } else {
            /* File switch failed: stop here */
	        ld.go = FALSE;
            continue;
          }
        } else {
          /* single file, stop now */
          ld.go = FALSE;
          continue;
        }
      } /* cnd_file_duration */
    }

  } /* while (ld.go) */

  /* delete stop conditions */
  if (cnd_file_duration != NULL)
    cnd_delete(cnd_file_duration);
  if (cnd_autostop_files != NULL)
    cnd_delete(cnd_autostop_files);
  if (cnd_autostop_size != NULL)
    cnd_delete(cnd_autostop_size);
  if (cnd_autostop_duration != NULL)
    cnd_delete(cnd_autostop_duration);

  if (ld.pcap_err) {
    snprintf(errmsg, sizeof(errmsg), "Error while capturing packets: %s",
      pcap_geterr(pch));
    popup_errmsg(errmsg);
#ifdef _WIN32
  }
#else
  } else if (ld.from_cap_pipe && ld.cap_pipe_err == PIPERR)
      popup_errmsg(errmsg);
#endif

  if (ld.err == 0)
    write_ok = TRUE;
  else {
    get_capture_file_io_error(errmsg, sizeof(errmsg), cfile.save_file, ld.err,
			      FALSE);
    popup_errmsg(errmsg);
    write_ok = FALSE;
  }

  if (capture_opts.multi_files_on) {
    close_ok = ringbuf_wtap_dump_close(&cfile, &err);
  } else {
    close_ok = wtap_dump_close(ld.pdh, &err);
  }
  /* If we've displayed a message about a write error, there's no point
     in displaying another message about an error on close. */
  if (!close_ok && write_ok) {
    get_capture_file_io_error(errmsg, sizeof(errmsg), cfile.save_file, err,
		TRUE);
    popup_errmsg(errmsg);
  }
  /* Set write_ok to mean the write and the close were successful. */
  write_ok = (write_ok && close_ok);

#ifndef _WIN32
  /*
   * XXX We exhibit different behaviour between normal mode and sync mode
   * when the pipe is stdin and not already at EOF.  If we're a child, the
   * parent's stdin isn't closed, so if the user starts another capture,
   * cap_pipe_open_live() will very likely not see the expected magic bytes and
   * will say "Unrecognized libpcap format".  On the other hand, in normal
   * mode, cap_pipe_open_live() will say "End of file on pipe during open".
   */
  if (ld.from_cap_pipe && pipe_fd >= 0)
    close(pipe_fd);
  else
#endif
  {
    /* Get the capture statistics, so we know how many packets were
       dropped. */
    if (pcap_stats(pch, stats) >= 0) {
      *stats_known = TRUE;
      if (capture_child) {
      	/* Let the parent process know. */
        sync_pipe_drops_to_parent(stats->ps_drop);
      }
    } else {
      snprintf(errmsg, sizeof(errmsg),
		"Can't get packet-drop statistics: %s",
		pcap_geterr(pch));
      popup_errmsg(errmsg);
    }
    pcap_close(pch);
  }

#ifdef _WIN32
  /* Shut down windows sockets */
  WSACleanup();
#endif

  if(show_info) {
    capture_info_destroy(&capture_ui);
  }

  return write_ok;

error:
  if (capture_opts.multi_files_on) {
    /* cleanup ringbuffer */
    ringbuf_error_cleanup();
  } else {
    /* We can't use the save file, and we have no wtap_dump stream
       to close in order to close it, so close the FD directly. */
    close(cfile.save_file_fd);

    /* We couldn't even start the capture, so get rid of the capture
       file. */
    unlink(cfile.save_file); /* silently ignore error */
    g_free(cfile.save_file);
  }
  cfile.save_file = NULL;
  popup_errmsg(errmsg);

#ifndef _WIN32
  if (ld.from_cap_pipe) {
    if (pipe_fd >= 0)
      close(pipe_fd);
  } else
#endif
  {
    if (pch != NULL)
      pcap_close(pch);
  }

  return FALSE;
}

static void
get_capture_file_io_error(char *errmsg, int errmsglen, const char *fname,
			  int err, gboolean is_close)
{
  switch (err) {

  case ENOSPC:
    snprintf(errmsg, errmsglen,
		"Not all the packets could be written to the file"
		" to which the capture was being saved\n"
		"(\"%s\") because there is no space left on the file system\n"
		"on which that file resides.",
		fname);
    break;

#ifdef EDQUOT
  case EDQUOT:
    snprintf(errmsg, errmsglen,
		"Not all the packets could be written to the file"
		" to which the capture was being saved\n"
		"(\"%s\") because you are too close to, or over,"
		" your disk quota\n"
		"on the file system on which that file resides.",
		fname);
  break;
#endif

  case WTAP_ERR_CANT_CLOSE:
    snprintf(errmsg, errmsglen,
		"The file to which the capture was being saved"
		" couldn't be closed for some unknown reason.");
    break;

  case WTAP_ERR_SHORT_WRITE:
    snprintf(errmsg, errmsglen,
		"Not all the packets could be written to the file"
		" to which the capture was being saved\n"
		"(\"%s\").",
		fname);
    break;

  default:
    if (is_close) {
      snprintf(errmsg, errmsglen,
		"The file to which the capture was being saved\n"
		"(\"%s\") could not be closed: %s.",
		fname, wtap_strerror(err));
    } else {
      snprintf(errmsg, errmsglen,
		"An error occurred while writing to the file"
		" to which the capture was being saved\n"
		"(\"%s\"): %s.",
		fname, wtap_strerror(err));
    }
    break;
  }
}

static void
popup_errmsg(const char *errmsg)
{
  if (capture_child) {
    /* This is the child process for a sync mode capture.
       Send the error message to our parent, so they can display a
       dialog box containing it. */
    sync_pipe_errmsg_to_parent(errmsg);
  } else {
    /* Display the dialog box ourselves; there's no parent. */
    simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK, "%s", errmsg);
  }
}

static void
stop_capture_signal_handler(int signo _U_)
{
  ld.go = FALSE;
}


void
capture_stop(void)
{
  if (fork_child != -1) {
#ifndef _WIN32
      kill(fork_child, SIGUSR1);
#else
      /* XXX: this is not the preferred method of closing a process!
       * the clean way would be getting the process id of the child process,
       * then getting window handle hWnd of that process (using EnumChildWindows),
       * and then do a SendMessage(hWnd, WM_CLOSE, 0, 0) 
       *
       * Unfortunately, I don't know how to get the process id from the handle */
      /* Hint: OpenProcess will get an handle from the id, not vice versa :-(
       *
       * Hint: GenerateConsoleCtrlEvent() will only work, if both processes are 
       * running in the same console, I don't know if that is true for our case.
       * And this also will require to have the process id
       */
      TerminateProcess((HANDLE) fork_child, 0);
#endif
  } else {
      ld.go = FALSE;
  }
}

void
kill_capture_child(void)
{
  if (fork_child != -1)
#ifndef _WIN32
      kill(fork_child, SIGTERM);	/* SIGTERM so it can clean up if necessary */
#else
      /* XXX: this is not the preferred method of closing a process!
       * the clean way would be getting the process id of the child process,
       * then getting window handle hWnd of that process (using EnumChildWindows),
       * and then do a SendMessage(hWnd, WM_CLOSE, 0, 0) 
       *
       * Unfortunately, I don't know how to get the process id from the handle */
      /* Hint: OpenProcess will get an handle from the id, not vice versa :-(
       *
       * Hint: GenerateConsoleCtrlEvent() will only work, if both processes are 
       * running in the same console, I don't know if that is true for our case.
       * And this also will require to have the process id
       */
      TerminateProcess((HANDLE) fork_child, 0);
#endif
}

/* one packet was captured, process it */
static void
capture_pcap_cb(guchar *user, const struct pcap_pkthdr *phdr,
  const guchar *pd)
{
  struct wtap_pkthdr whdr;
  union wtap_pseudo_header pseudo_header;
  loop_data *ld = (loop_data *) user;
  int err;

  /* user told us to stop after x packets, do we have enough? */
  if ((++ld->counts.total >= ld->max) && (ld->max > 0))
  {
     ld->go = FALSE;
  }

  /* Convert from libpcap to Wiretap format.
     If that fails, set "ld->go" to FALSE, to stop the capture, and set
     "ld->err" to the error. */
  pd = wtap_process_pcap_packet(ld->linktype, phdr, pd, &pseudo_header,
				&whdr, &err);
  if (pd == NULL) {
    ld->go = FALSE;
    ld->err = err;
    return;
  }

  if (ld->pdh) {
    /* We're supposed to write the packet to a file; do so.
       If this fails, set "ld->go" to FALSE, to stop the capture, and set
       "ld->err" to the error. */
    if (!wtap_dump(ld->pdh, &whdr, &pseudo_header, pd, &err)) {
      ld->go = FALSE;
      ld->err = err;
    }
  }

  switch (ld->linktype) {
    case WTAP_ENCAP_ETHERNET:
      capture_eth(pd, 0, whdr.caplen, &ld->counts);
      break;
    case WTAP_ENCAP_FDDI:
    case WTAP_ENCAP_FDDI_BITSWAPPED:
      capture_fddi(pd, whdr.caplen, &ld->counts);
      break;
    case WTAP_ENCAP_PRISM_HEADER:
      capture_prism(pd, 0, whdr.caplen, &ld->counts);
      break;
    case WTAP_ENCAP_TOKEN_RING:
      capture_tr(pd, 0, whdr.caplen, &ld->counts);
      break;
    case WTAP_ENCAP_NULL:
      capture_null(pd, whdr.caplen, &ld->counts);
      break;
    case WTAP_ENCAP_PPP:
      capture_ppp_hdlc(pd, 0, whdr.caplen, &ld->counts);
      break;
    case WTAP_ENCAP_RAW_IP:
      capture_raw(pd, whdr.caplen, &ld->counts);
      break;
    case WTAP_ENCAP_SLL:
      capture_sll(pd, whdr.caplen, &ld->counts);
      break;
    case WTAP_ENCAP_LINUX_ATM_CLIP:
      capture_clip(pd, whdr.caplen, &ld->counts);
      break;
    case WTAP_ENCAP_IEEE_802_11:
    case WTAP_ENCAP_IEEE_802_11_WITH_RADIO:
      capture_ieee80211(pd, 0, whdr.caplen, &ld->counts);
      break;
    case WTAP_ENCAP_CHDLC:
      capture_chdlc(pd, 0, whdr.caplen, &ld->counts);
      break;
    case WTAP_ENCAP_LOCALTALK:
      capture_llap(&ld->counts);
      break;
    case WTAP_ENCAP_ATM_PDUS:
      capture_atm(&pseudo_header, pd, whdr.caplen, &ld->counts);
      break;
    case WTAP_ENCAP_IP_OVER_FC:
      capture_ipfc(pd, whdr.caplen, &ld->counts);
      break;
    case WTAP_ENCAP_ARCNET:
      capture_arcnet(pd, whdr.caplen, &ld->counts, FALSE, TRUE);
      break;
    case WTAP_ENCAP_ARCNET_LINUX:
      capture_arcnet(pd, whdr.caplen, &ld->counts, TRUE, FALSE);
      break;
    case WTAP_ENCAP_APPLE_IP_OVER_IEEE1394:
      capture_ap1394(pd, 0, whdr.caplen, &ld->counts);
      break;
    /* XXX - some ATM drivers on FreeBSD might prepend a 4-byte ATM
       pseudo-header to DLT_ATM_RFC1483, with LLC header following;
       we might have to implement that at some point. */
  }
}

#endif /* HAVE_LIBPCAP */
