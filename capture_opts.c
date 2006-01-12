/* capture_opts.c
 * Routines for capture options setting
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

#ifdef HAVE_LIBPCAP

#include <string.h>
#include <ctype.h>

#include <pcap.h>

#include <glib.h>

#include <epan/packet.h>

#include "capture.h"
#include "capture_opts.h"
#include "ringbuffer.h"
#include "clopts_common.h"
#include "cmdarg_err.h"

#include "capture-pcap-util.h"
#include "capture_ui_utils.h"
#include <wiretap/file_util.h>


static gboolean capture_opts_output_to_pipe(const char *save_file);


void
capture_opts_init(capture_options *capture_opts, void *cfile)
{
  capture_opts->cf                      = cfile;            
  capture_opts->cfilter		            = g_strdup("");     /* No capture filter string specified */
  capture_opts->iface                   = NULL;             /* Default is "pick the first interface" */
#ifdef _WIN32
  capture_opts->buffer_size             = 1;                /* 1 MB */
#endif
  capture_opts->has_snaplen             = FALSE;
  capture_opts->snaplen                 = WTAP_MAX_PACKET_SIZE; /* snapshot length - default is
                                                                    infinite, in effect */
  capture_opts->promisc_mode            = TRUE;             /* promiscuous mode is the default */
  capture_opts->linktype                = -1;               /* the default linktype */
  capture_opts->save_file               = NULL;
  capture_opts->real_time_mode          = TRUE;
  capture_opts->show_info               = TRUE;
  capture_opts->quit_after_cap          = FALSE;
  capture_opts->restart                 = FALSE;

  capture_opts->multi_files_on          = FALSE;
  capture_opts->has_file_duration       = FALSE;
  capture_opts->file_duration           = 60;               /* 1 min */
  capture_opts->has_ring_num_files      = FALSE;
  capture_opts->ring_num_files          = RINGBUFFER_MIN_NUM_FILES;

  capture_opts->has_autostop_files      = FALSE;
  capture_opts->autostop_files          = 1;
  capture_opts->has_autostop_packets    = FALSE;            
  capture_opts->autostop_packets        = 0;
  capture_opts->has_autostop_filesize   = FALSE;
  capture_opts->autostop_filesize       = 1024;             /* 1 MB */
  capture_opts->has_autostop_duration   = FALSE;
  capture_opts->autostop_duration       = 60;               /* 1 min */


  capture_opts->fork_child              = -1;               /* invalid process handle */
#ifdef _WIN32
  capture_opts->signal_pipe_fd          = -1;
#endif
  capture_opts->state                   = CAPTURE_STOPPED;
  capture_opts->output_to_pipe          = FALSE;
}


/* log content of capture_opts */
void
capture_opts_log(const char *log_domain, GLogLevelFlags log_level, capture_options *capture_opts) {
    g_log(log_domain, log_level, "CAPTURE OPTIONS    :");
    g_log(log_domain, log_level, "CFile              : 0x%p", capture_opts->cf);
    g_log(log_domain, log_level, "Filter             : %s", capture_opts->cfilter);
    g_log(log_domain, log_level, "Interface          : %s", capture_opts->iface);
#ifdef _WIN32
    g_log(log_domain, log_level, "BufferSize         : %u (MB)", capture_opts->buffer_size);
#endif
    g_log(log_domain, log_level, "SnapLen         (%u): %u", capture_opts->has_snaplen, capture_opts->snaplen);
    g_log(log_domain, log_level, "Promisc            : %u", capture_opts->promisc_mode);
    g_log(log_domain, log_level, "LinkType           : %d", capture_opts->linktype);
    g_log(log_domain, log_level, "SaveFile           : %s", (capture_opts->save_file) ? capture_opts->save_file : "");
    g_log(log_domain, log_level, "RealTimeMode       : %u", capture_opts->real_time_mode);
    g_log(log_domain, log_level, "ShowInfo           : %u", capture_opts->show_info);
    g_log(log_domain, log_level, "QuitAfterCap       : %u", capture_opts->quit_after_cap);

    g_log(log_domain, log_level, "MultiFilesOn       : %u", capture_opts->multi_files_on);
    g_log(log_domain, log_level, "FileDuration    (%u): %u", capture_opts->has_file_duration, capture_opts->file_duration);
    g_log(log_domain, log_level, "RingNumFiles    (%u): %u", capture_opts->has_ring_num_files, capture_opts->ring_num_files);

    g_log(log_domain, log_level, "AutostopFiles   (%u): %u", capture_opts->has_autostop_files, capture_opts->autostop_files);
    g_log(log_domain, log_level, "AutostopPackets (%u): %u", capture_opts->has_autostop_packets, capture_opts->autostop_packets);
    g_log(log_domain, log_level, "AutostopFilesize(%u): %u (KB)", capture_opts->has_autostop_filesize, capture_opts->autostop_filesize);
    g_log(log_domain, log_level, "AutostopDuration(%u): %u", capture_opts->has_autostop_duration, capture_opts->autostop_duration);

    g_log(log_domain, log_level, "ForkChild          : %d", capture_opts->fork_child);
#ifdef _WIN32
    g_log(log_domain, log_level, "SignalPipeFd       : %d", capture_opts->signal_pipe_fd);
#endif
}

/*
 * Given a string of the form "<autostop criterion>:<value>", as might appear
 * as an argument to a "-a" option, parse it and set the criterion in
 * question.  Return an indication of whether it succeeded or failed
 * in some fashion.
 */
static gboolean
set_autostop_criterion(capture_options *capture_opts, const char *autostoparg)
{
  gchar *p, *colonp;

  colonp = strchr(autostoparg, ':');
  if (colonp == NULL)
    return FALSE;

  p = colonp;
  *p++ = '\0';

  /*
   * Skip over any white space (there probably won't be any, but
   * as we allow it in the preferences file, we might as well
   * allow it here).
   */
  while (isspace((guchar)*p))
    p++;
  if (*p == '\0') {
    /*
     * Put the colon back, so if our caller uses, in an
     * error message, the string they passed us, the message
     * looks correct.
     */
    *colonp = ':';
    return FALSE;
  }
  if (strcmp(autostoparg,"duration") == 0) {
    capture_opts->has_autostop_duration = TRUE;
    capture_opts->autostop_duration = get_positive_int(p,"autostop duration");
  } else if (strcmp(autostoparg,"filesize") == 0) {
    capture_opts->has_autostop_filesize = TRUE;
    capture_opts->autostop_filesize = get_positive_int(p,"autostop filesize");
  } else if (strcmp(autostoparg,"files") == 0) {
    capture_opts->multi_files_on = TRUE;
    capture_opts->has_autostop_files = TRUE;
    capture_opts->autostop_files = get_positive_int(p,"autostop files");
  } else {
    return FALSE;
  }
  *colonp = ':'; /* put the colon back */
  return TRUE;
}

/*
 * Given a string of the form "<ring buffer file>:<duration>", as might appear
 * as an argument to a "-b" option, parse it and set the arguments in
 * question.  Return an indication of whether it succeeded or failed
 * in some fashion.
 */
static gboolean
get_ring_arguments(capture_options *capture_opts, const char *arg)
{
  gchar *p = NULL, *colonp;

  colonp = strchr(arg, ':');
  if (colonp == NULL)
    return TRUE;

  p = colonp;
  *p++ = '\0';

  /*
   * Skip over any white space (there probably won't be any, but
   * as we allow it in the preferences file, we might as well
   * allow it here).
   */
  while (isspace((guchar)*p))
    p++;
  if (*p == '\0') {
    /*
     * Put the colon back, so if our caller uses, in an
     * error message, the string they passed us, the message
     * looks correct.
     */
    *colonp = ':';
    return FALSE;
  }

  if (strcmp(arg,"files") == 0) {
    capture_opts->has_ring_num_files = TRUE;
    capture_opts->ring_num_files = get_natural_int(p, "number of ring buffer files");
  } else if (strcmp(arg,"filesize") == 0) {
    capture_opts->has_autostop_filesize = TRUE;
    capture_opts->autostop_filesize = get_positive_int(p, "ring buffer filesize");
  } else if (strcmp(arg,"duration") == 0) {
    capture_opts->has_file_duration = TRUE;
    capture_opts->file_duration = get_positive_int(p, "ring buffer duration");
  }

  *colonp = ':';	/* put the colon back */
  return TRUE;
}


#ifdef _WIN32
/*
 * Given a string of the form "<pipe name>:<file descriptor>", as might appear
 * as an argument to a "-Z" option, parse it and set the arguments in
 * question.  Return an indication of whether it succeeded or failed
 * in some fashion.
 */
static gboolean
get_pipe_arguments(capture_options *capture_opts, const char *arg)
{
  gchar *p = NULL, *colonp;
  int pipe_fd;


  colonp = strchr(arg, ':');
  if (colonp == NULL)
    return TRUE;

  p = colonp;
  *p++ = '\0';

  /*
   * Skip over any white space (there probably won't be any, but
   * as we allow it in the preferences file, we might as well
   * allow it here).
   */
  while (isspace((guchar)*p))
    p++;
  if (*p == '\0') {
    /*
     * Put the colon back, so if our caller uses, in an
     * error message, the string they passed us, the message
     * looks correct.
     */
    *colonp = ':';
    return FALSE;
  }

  if (strcmp(arg,"sync") == 0) {
    /* associate stdout with sync pipe */
    pipe_fd = get_natural_int(p, "sync pipe file descriptor");
    if (dup2(pipe_fd, 1) < 0) {
      cmdarg_err("Unable to dup sync pipe handle");
      return FALSE;
    }
  } else if (strcmp(arg,"signal") == 0) {
    /* associate stdin with signal pipe */
    pipe_fd = get_natural_int(p, "signal pipe file descriptor");
    if (dup2(pipe_fd, 0) < 0) {
      cmdarg_err("Unable to dup signal pipe handle");
      return FALSE;
    }
    capture_opts->signal_pipe_fd = pipe_fd;
  }

  *colonp = ':';	/* put the colon back */
  return TRUE;
}
#endif


void
capture_opts_add_iface_opt(capture_options *capture_opts, const char *optarg)
{
    long        adapter_index;
    char        *p;
    GList       *if_list;
    if_info_t   *if_info;
    int         err;
    gchar       err_str[PCAP_ERRBUF_SIZE];
    gchar       *cant_get_if_list_errstr;


    /*
     * If the argument is a number, treat it as an index into the list
     * of adapters, as printed by "tethereal -D".
     *
     * This should be OK on UNIX systems, as interfaces shouldn't have
     * names that begin with digits.  It can be useful on Windows, where
     * more than one interface can have the same name.
     */
    adapter_index = strtol(optarg, &p, 10);
    if (p != NULL && *p == '\0') {
      if (adapter_index < 0) {
        cmdarg_err("The specified adapter index is a negative number");
       exit(1);
      }
      if (adapter_index > INT_MAX) {
        cmdarg_err("The specified adapter index is too large (greater than %d)",
            INT_MAX);
        exit(1);
      }
      if (adapter_index == 0) {
        cmdarg_err("there is no interface with that adapter index");
        exit(1);
      }
      if_list = get_interface_list(&err, err_str);
      if (if_list == NULL) {
        switch (err) {

        case CANT_GET_INTERFACE_LIST:
            cant_get_if_list_errstr =
                cant_get_if_list_error_message(err_str);
            cmdarg_err("%s", cant_get_if_list_errstr);
            g_free(cant_get_if_list_errstr);
            break;

        case NO_INTERFACES_FOUND:
            cmdarg_err("There are no interfaces on which a capture can be done");
            break;
        }
        exit(2);
      }
      if_info = g_list_nth_data(if_list, adapter_index - 1);
      if (if_info == NULL) {
        cmdarg_err("there is no interface with that adapter index");
        exit(1);
      }
      capture_opts->iface = g_strdup(if_info->name);
      free_interface_list(if_list);
    } else {
      capture_opts->iface = g_strdup(optarg);
    }
}

void
capture_opts_add_opt(capture_options *capture_opts, int opt, const char *optarg, gboolean *start_capture)
{
    switch(opt) {
    case 'a':        /* autostop criteria */
        if (set_autostop_criterion(capture_opts, optarg) == FALSE) {
          cmdarg_err("Invalid or unknown -a flag \"%s\"", optarg);
          exit(1);
        }
        break;
    case 'b':        /* Ringbuffer option */
        capture_opts->multi_files_on = TRUE;
        if (get_ring_arguments(capture_opts, optarg) == FALSE) {
          cmdarg_err("Invalid or unknown -b arg \"%s\"", optarg);
          exit(1);
        }
        break;
#ifdef _WIN32
    case 'B':        /* Buffer size */
        capture_opts->buffer_size = get_positive_int(optarg, "buffer size");
        break;
#endif
    case 'c':        /* Capture xxx packets */
        capture_opts->has_autostop_packets = TRUE;
        capture_opts->autostop_packets = get_positive_int(optarg, "packet count");
        break;
    case 'f':        /* capture filter */
        if (capture_opts->cfilter)
            g_free(capture_opts->cfilter);
        capture_opts->cfilter = g_strdup(optarg);
        break;
    case 'H':        /* Hide capture info dialog box */
        capture_opts->show_info = FALSE;
        break;
    case 'i':        /* Use interface xxx */
        capture_opts_add_iface_opt(capture_opts, optarg);
        break;
    case 'k':        /* Start capture immediately */
        *start_capture = TRUE;
        break;
    /*case 'l':*/    /* Automatic scrolling in live capture mode */
    case 'p':        /* Don't capture in promiscuous mode */
        capture_opts->promisc_mode = FALSE;
        break;
    case 'Q':        /* Quit after capture (just capture to file) */
        capture_opts->quit_after_cap  = TRUE;
        *start_capture   = TRUE;  /*** -Q implies -k !! ***/
        break;
    case 's':        /* Set the snapshot (capture) length */
        capture_opts->has_snaplen = TRUE;
        capture_opts->snaplen = get_positive_int(optarg, "snapshot length");
        break;
    case 'S':        /* "Real-Time" mode: used for following file ala tail -f */
        capture_opts->real_time_mode = TRUE;
        break;
    case 'w':        /* Write to capture file xxx */
#if defined _WIN32 && (GLIB_MAJOR_VERSION > 2 || (GLIB_MAJOR_VERSION == 2 && GLIB_MINOR_VERSION >= 6))
        /* since GLib 2.6, we need to convert filenames to utf8 for Win32 */
        capture_opts->save_file = g_locale_to_utf8(optarg, -1, NULL, NULL, NULL);
#else
        capture_opts->save_file = g_strdup(optarg);
#endif
        capture_opts->output_to_pipe = capture_opts_output_to_pipe(capture_opts->save_file);
	    break;
    case 'y':        /* Set the pcap data link type */
#ifdef HAVE_PCAP_DATALINK_NAME_TO_VAL
        capture_opts->linktype = pcap_datalink_name_to_val(optarg);
        if (capture_opts->linktype == -1) {
          cmdarg_err("The specified data link type \"%s\" isn't valid",
                  optarg);
          exit(1);
        }
#else /* HAVE_PCAP_DATALINK_NAME_TO_VAL */
        /* XXX - just treat it as a number */
        capture_opts->linktype = get_natural_int(optarg, "data link type");
#endif /* HAVE_PCAP_DATALINK_NAME_TO_VAL */
        break;
#ifdef _WIN32
      /* Hidden option supporting Sync mode */
    case 'Z':        /* Write to pipe FD XXX */
       if (get_pipe_arguments(capture_opts, optarg) == FALSE) {
          cmdarg_err("Invalid or unknown -Z flag \"%s\"", optarg);
          exit(1);
        }
        break;
#endif /* _WIN32 */
    default:
        /* the caller is responsible to send us only the right opt's */
        g_assert_not_reached();
    }
}


void capture_opts_list_link_layer_types(capture_options *capture_opts)
{
    gchar err_str[PCAP_ERRBUF_SIZE];
    GList *lt_list, *lt_entry;
    data_link_info_t *data_link_info;

    /* Get the list of link-layer types for the capture device. */
    lt_list = get_pcap_linktype_list(capture_opts->iface, err_str);
    if (lt_list == NULL) {
      if (err_str[0] != '\0') {
	cmdarg_err("The list of data link types for the capture device could not be obtained (%s)."
	  "Please check to make sure you have sufficient permissions, and that\n"
	  "you have the proper interface or pipe specified.\n", err_str);
      } else
	cmdarg_err("The capture device has no data link types.");
      exit(2);
    }
    cmdarg_err_cont("Data link types (use option -y to set):");
    for (lt_entry = lt_list; lt_entry != NULL;
         lt_entry = g_list_next(lt_entry)) {
      data_link_info = lt_entry->data;
      cmdarg_err_cont("  %s", data_link_info->name);
      if (data_link_info->description != NULL)
	cmdarg_err_cont(" (%s)", data_link_info->description);
      else
	cmdarg_err_cont(" (not supported)");
      putchar('\n');
    }
    free_pcap_linktype_list(lt_list);
}


void capture_opts_list_interfaces()
{
    GList       *if_list;
    GList       *if_entry;
    if_info_t   *if_info;
    int         err;
    gchar       err_str[PCAP_ERRBUF_SIZE];
    gchar       *cant_get_if_list_errstr;
    int         i;


    if_list = get_interface_list(&err, err_str);
    if (if_list == NULL) {
        switch (err) {
        case CANT_GET_INTERFACE_LIST:
            cant_get_if_list_errstr = cant_get_if_list_error_message(err_str);
            cmdarg_err("%s", cant_get_if_list_errstr);
            g_free(cant_get_if_list_errstr);
        break;

        case NO_INTERFACES_FOUND:
            cmdarg_err("There are no interfaces on which a capture can be done");
        break;
        }
        exit(2);
    }

    i = 1;  /* Interface id number */
    for (if_entry = g_list_first(if_list); if_entry != NULL;
    if_entry = g_list_next(if_entry)) {
        if_info = if_entry->data;
        printf("%d. %s", i++, if_info->name);
        if (if_info->description != NULL)
            printf(" (%s)", if_info->description);
        printf("\n");
    }
    free_interface_list(if_list);
}


void capture_opts_trim_snaplen(capture_options *capture_opts, int snaplen_min)
{
  if (capture_opts->snaplen < 1)
    capture_opts->snaplen = WTAP_MAX_PACKET_SIZE;
  else if (capture_opts->snaplen < snaplen_min)
    capture_opts->snaplen = snaplen_min;
}


void capture_opts_trim_ring_num_files(capture_options *capture_opts)
{
  /* Check the value range of the ring_num_files parameter */
  if (capture_opts->ring_num_files > RINGBUFFER_MAX_NUM_FILES)
    capture_opts->ring_num_files = RINGBUFFER_MAX_NUM_FILES;
#if RINGBUFFER_MIN_NUM_FILES > 0
  else if (capture_opts->ring_num_files < RINGBUFFER_MIN_NUM_FILES)
    capture_opts->ring_num_files = RINGBUFFER_MIN_NUM_FILES;
#endif
}


gboolean capture_opts_trim_iface(capture_options *capture_opts, const char *capture_device)
{
    GList       *if_list;
    if_info_t   *if_info;
    int         err;
    gchar       err_str[PCAP_ERRBUF_SIZE];
    gchar       *cant_get_if_list_errstr;


    /* Did the user specify an interface to use? */
    if (capture_opts->iface == NULL) {
      /* No - is a default specified in the preferences file? */
      if (capture_device != NULL) {
          /* Yes - use it. */
          capture_opts->iface = g_strdup(capture_device);
      } else {
        /* No - pick the first one from the list of interfaces. */
        if_list = get_interface_list(&err, err_str);
        if (if_list == NULL) {
          switch (err) {

          case CANT_GET_INTERFACE_LIST:
              cant_get_if_list_errstr = cant_get_if_list_error_message(err_str);
              cmdarg_err("%s", cant_get_if_list_errstr);
              g_free(cant_get_if_list_errstr);
              break;

          case NO_INTERFACES_FOUND:
              cmdarg_err("There are no interfaces on which a capture can be done");
              break;
          }
          return FALSE;
        }
        if_info = if_list->data;	/* first interface */
        capture_opts->iface = g_strdup(if_info->name);
        free_interface_list(if_list);
      }
    }

    return TRUE;
}



#ifndef S_IFIFO
#define S_IFIFO	_S_IFIFO
#endif
#ifndef S_ISFIFO
#define S_ISFIFO(mode)  (((mode) & S_IFMT) == S_IFIFO)
#endif

/* copied from filesystem.c */
static int capture_opts_test_for_fifo(const char *path)
{
	struct stat statb;

	if (eth_stat(path, &statb) < 0)
		return errno;

	if (S_ISFIFO(statb.st_mode))
		return ESPIPE;
	else
		return 0;
}

static gboolean capture_opts_output_to_pipe(const char *save_file)
{
  int err;

  if (save_file != NULL) {
    /* We're writing to a capture file. */
    if (strcmp(save_file, "-") == 0) {
      /* Writing to stdout. */
      /* XXX - should we check whether it's a pipe?  It's arguably
         silly to do "-w - >output_file" rather than "-w output_file",
         but by not checking we might be violating the Principle Of
         Least Astonishment. */
      return TRUE;
    } else {
      /* not a capture file, test for a FIFO (aka named pipe) */
      err = capture_opts_test_for_fifo(save_file);
      switch (err) {

      case ENOENT:	/* it doesn't exist, so we'll be creating it,
      			   and it won't be a FIFO */
      case 0:		/* found it, but it's not a FIFO */
        break;

      case ESPIPE:	/* it is a FIFO */
        return TRUE;
        break;

      default:		/* couldn't stat it */
        cmdarg_err("Error testing whether capture file is a pipe: %s",
                strerror(errno));
        exit(2);
      }
    }
  }

  return FALSE;
}

#endif /* HAVE_LIBPCAP */
