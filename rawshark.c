/* rawshark.c
 *
 * $Id$
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * Rawshark - Raw field extractor by Gerald Combs <gerald@wireshark.org>
 * and Loris Degioanni <loris.degioanni@cacetech.com>
 * Based on TShark, by Gilbert Ramirez <gram@alumni.rice.edu> and Guy Harris
 * <guy@alum.mit.edu>.
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

/*
 * Rawshark does the following:
 * - Opens a specified file or named pipe
 * - Applies a specfied DLT or "decode as" encapsulation
 * - Reads frames prepended with a libpcap packet header.
 * - Prints a status line, followed by  fields from a specified list.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <locale.h>
#include <limits.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <errno.h>

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <signal.h>

#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif

#ifdef NEED_STRERROR_H
#include "strerror.h"
#endif

#ifdef NEED_GETOPT_H
#include "getopt.h"
#endif

#include <glib.h>
#include <epan/epan.h>
#include <epan/filesystem.h>
#include <epan/privileges.h>
#include <wiretap/file_util.h>

#include "globals.h"
#include <epan/packet.h>
#include "file.h"
#include "disabled_protos.h"
#include <epan/prefs.h>
#include <epan/column.h>
#include "print.h"
#include <epan/addr_resolv.h>
#include "util.h"
#include "clopts_common.h"
#include "cmdarg_err.h"
#include "version_info.h"
#include <epan/conversation.h>
#include <epan/plugins.h>
#include "register.h"
#include "conditions.h"
#include "capture_stop_conditions.h"
#include "capture_ui_utils.h"
#include <epan/epan_dissect.h>
#include <epan/stat_cmd_args.h>
#include <epan/timestamp.h>
#include <epan/unicode-utils.h>
#include "epan/column-utils.h"
#include "epan/proto.h"
#include <epan/tap.h>

#include <wtap.h>
#include <libpcap.h>

#ifdef HAVE_LIBPCAP
#include <pcap.h>
#include <setjmp.h>
#include "capture-pcap-util.h"
#include "pcapio.h"
#ifdef _WIN32
#include "capture-wpcap.h"
#include "capture_errs.h"
#endif /* _WIN32 */
#endif /* HAVE_LIBPCAP */
#include "epan/emem.h"
#include "log.h"

/*
 * This is the template for the decode as option; it is shared between the
 * various functions that output the usage for this parameter.
 */
static const gchar decode_as_arg_template[] = "<layer_type>==<selector>,<decode_as_protocol>";

static nstime_t first_ts;
static nstime_t prev_dis_ts;
static nstime_t prev_cap_ts;
static GString *comp_info_str, *runtime_info_str;

/*
 * The way the packet decode is to be written.
 */
typedef enum {
	WRITE_TEXT,	/* summary or detail text */
	WRITE_XML	/* PDML or PSML */
	/* Add CSV and the like here */
} output_action_e;
static gboolean line_buffered;
static guint32 cum_bytes = 0;
static print_format_e print_format = PR_FMT_TEXT;

/*
 * Standard secondary message for unexpected errors.
 */
static const char please_report[] =
    "Please report this to the Wireshark developers";

cf_status_t raw_cf_open(capture_file *cf, const char *fname);
static int load_cap_file(capture_file *cf);
static gboolean process_packet(capture_file *cf, gint64 offset,
    const struct wtap_pkthdr *whdr, const guchar *pd);
static void show_print_file_io_error(int err);

static void open_failure_message(const char *filename, int err,
    gboolean for_writing);
static void failure_message(const char *msg_format, va_list ap);
static void read_failure_message(const char *filename, int err);
static void protocolinfo_init(char *field);
static gboolean parse_field_string_format(char *format);

typedef enum {
	SF_NONE,	/* No format (placeholder) */
	SF_NAME,	/* %D Field name / description */
	SF_NUMVAL,	/* %N Numeric value */
	SF_STRVAL	/* %S String value */
} string_fmt_e;

typedef struct string_fmt_s {
	gchar *plain;
	string_fmt_e format;	/* Valid if plain is NULL */
} string_fmt_t;

capture_file cfile;
int n_rfilters;
int n_rfcodes;
dfilter_t *rfcodes[64];
int n_rfieldfilters;
dfilter_t *rfieldfcodes[64];
int fd;
int encap;
GPtrArray *string_fmts;

static void
print_usage(gboolean print_ver)
{
  FILE *output;

  if (print_ver) {
    output = stdout;
    fprintf(output,
        "Rawshark " VERSION "%s\n"
        "Dump and analyze network traffic.\n"
        "See http://www.wireshark.org for more information.\n"
        "\n"
        "%s",
	wireshark_svnversion, get_copyright_info());
  } else {
    output = stderr;
  }
  fprintf(output, "\n");
  fprintf(output, "Usage: rawshark [options] ...\n");
  fprintf(output, "\n");

  fprintf(output, "Input file:\n");
  fprintf(output, "  -r <infile>              set the pipe or file name to read from\n");

  fprintf(output, "\n");
  fprintf(output, "Processing:\n");
  fprintf(output, "  -R <read filter>         packet filter in Wireshark display filter syntax\n");
  fprintf(output, "  -F <field>               field to display\n");
  fprintf(output, "  -n                       disable all name resolution (def: all enabled)\n");
  fprintf(output, "  -N <name resolve flags>  enable specific name resolution(s): \"mntC\"\n");
  fprintf(output, "  -d <encap:dlt>|<proto:protoname>\n");
  fprintf(output, "                           packet encapsulation or protocol\n");

  /*fprintf(output, "\n");*/
  fprintf(output, "Output:\n");
  fprintf(output, "  -S                       format string for fields (%%D - name, %%S - stringval, %%N numval\n");
  fprintf(output, "  -t ad|a|r|d|dd|e         output format of time stamps (def: r: rel. to first)\n");
  fprintf(output, "  -l                       flush output after each packet\n");

  fprintf(output, "\n");
  fprintf(output, "Miscellaneous:\n");
  fprintf(output, "  -h                       display this help and exit\n");
  fprintf(output, "  -v                       display version info and exit\n");
  fprintf(output, "  -o <name>:<value> ...    override preference setting\n");
}

static void
log_func_ignore (const gchar *log_domain _U_, GLogLevelFlags log_level _U_,
    const gchar *message _U_, gpointer user_data _U_)
{
}

/**
 * Open a pipe for raw input.  This is a stripped-down version of
 * pcap_loop.c:cap_pipe_open_live().
 * We check if "pipe_name" is "-" (stdin) or a FIFO, and open it.
 * @param pipe_name The name of the pipe or FIFO.
 * @return A POSIX file descriptor on success, or -1 on failure.
 */
static int
raw_pipe_open(const char *pipe_name)
{
#ifndef _WIN32
  struct stat pipe_stat;
#else
  char *pncopy, *pos;
  wchar_t *err_str;
  HANDLE hPipe = NULL;
#endif
  int          rfd;

  g_log(LOG_DOMAIN_CAPTURE_CHILD, G_LOG_LEVEL_DEBUG, "open_raw_pipe: %s", pipe_name);

  /*
   * XXX Rawshark blocks until we return
   */
  if (strcmp(pipe_name, "-") == 0) {
    rfd = 0; /* read from stdin */
#ifdef _WIN32
    /*
     * This is needed to set the stdin pipe into binary mode, otherwise
     * CR/LF are mangled...
     */
    _setmode(0, _O_BINARY);
#endif  /* _WIN32 */
  } else {
#ifndef _WIN32
    if (eth_stat(pipe_name, &pipe_stat) < 0) {
      g_log(LOG_DOMAIN_CAPTURE_CHILD, G_LOG_LEVEL_DEBUG,
        "The capture session could not be initiated "
        "due to error on pipe: %s", strerror(errno));
      return -1;
    }
    if (! S_ISFIFO(pipe_stat.st_mode)) {
      if (S_ISCHR(pipe_stat.st_mode)) {
        /*
         * Assume the user specified an interface on a system where
         * interfaces are in /dev.  Pretend we haven't seen it.
         */
      } else
      {
        g_log(LOG_DOMAIN_CAPTURE_CHILD, G_LOG_LEVEL_DEBUG,
            "The capture session could not be initiated because\n"
            "\"%s\" is neither an interface nor a pipe", pipe_name);
      }
      return -1;
    }
    rfd = eth_open(pipe_name, O_RDONLY | O_NONBLOCK, 0000 /* no creation so don't matter */);
    if (rfd == -1) {
        g_log(LOG_DOMAIN_CAPTURE_CHILD, G_LOG_LEVEL_DEBUG,
          "The capture session could not be initiated "
          "due to error on pipe open: %s", strerror(errno));
      return -1;
    }
#else /* _WIN32 */
#define PIPE_STR "\\pipe\\"
    /* Under Windows, named pipes _must_ have the form
     * "\\<server>\pipe\<pipe_name>".  <server> may be "." for localhost.
     */
    pncopy = g_strdup(pipe_name);
    if (strstr(pncopy, "\\\\") == pncopy) {
      pos = strchr(pncopy + 3, '\\');
      if (pos && g_ascii_strncasecmp(pos, PIPE_STR, strlen(PIPE_STR)) != 0)
        pos = NULL;
    }

    g_free(pncopy);

    if (!pos) {
      g_log(LOG_DOMAIN_CAPTURE_CHILD, G_LOG_LEVEL_DEBUG,
          "The capture session could not be initiated because\n"
          "\"%s\" is neither an interface nor a pipe", pipe_name);
      return -1;
    }

    /* Wait for the pipe to appear */
    while (1) {
      hPipe = CreateFile(utf_8to16(pipe_name), GENERIC_READ, 0, NULL,
          OPEN_EXISTING, 0, NULL);

      if (hPipe != INVALID_HANDLE_VALUE)
        break;

      if (GetLastError() != ERROR_PIPE_BUSY) {
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
          NULL, GetLastError(), 0, (LPTSTR) &err_str, 0, NULL);
          g_log(LOG_DOMAIN_CAPTURE_CHILD, G_LOG_LEVEL_DEBUG,
            "The capture session on \"%s\" could not be initiated "
            "due to error on pipe open: pipe busy: %s (error %d)",
	    pipe_name, utf_16to8(err_str), GetLastError());
        LocalFree(err_str);
        return -1;
      }

      if (!WaitNamedPipe(utf_8to16(pipe_name), 30 * 1000)) {
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
          NULL, GetLastError(), 0, (LPTSTR) &err_str, 0, NULL);
          g_log(LOG_DOMAIN_CAPTURE_CHILD, G_LOG_LEVEL_DEBUG,
            "The capture session could not be initiated "
            "due to error on pipe open: %s (error %d)",
	    utf_16to8(err_str), GetLastError());
        LocalFree(err_str);
        return -1;
      }
    }

    rfd = _open_osfhandle((long) hPipe, _O_RDONLY);
    if (rfd == -1) {
      g_log(LOG_DOMAIN_CAPTURE_CHILD, G_LOG_LEVEL_DEBUG,
          "The capture session could not be initiated "
          "due to error on pipe open: %s", strerror(errno));
      return -1;
    }
#endif /* _WIN32 */
  }

  return rfd;
}

/**
 * Parse a link-type argument of the form "encap:<pcap dlt>" or
 * "proto:<proto name>".  "Pcap dlt" must be a name conforming to
 * pcap_datalink_name_to_val() or an integer.  "Proto name" must be
 * a protocol name, e.g. "http".
 */
static gboolean
set_link_type(const char *lt_arg) {
  char *spec_ptr = strchr(lt_arg, ':');
  int dlt_val;
  dissector_handle_t dhandle;
  GString *pref_str;

  if (!spec_ptr)
    return FALSE;

  spec_ptr++;

  if (strncmp(lt_arg, "encap:", strlen("encap:")) == 0) {
#ifdef HAVE_PCAP_DATALINK_NAME_TO_VAL
    dlt_val = linktype_name_to_val(spec_ptr);
    if (dlt_val >= 0) {
      encap = dlt_val;
      return TRUE;
    }
#endif
    dlt_val = strtol(spec_ptr, NULL, 10);
    if (errno != EINVAL && dlt_val >= 0) {
      encap = wtap_pcap_encap_to_wtap_encap(dlt_val);
      if (encap == WTAP_ENCAP_UNKNOWN) {
        return FALSE;
      }
      return TRUE;
    }
  } else if (strncmp(lt_arg, "proto:", strlen("proto:")) == 0) {
    dhandle = find_dissector(spec_ptr);
    if (dhandle) {
      encap = WTAP_ENCAP_USER0;
      pref_str = g_string_new("uat:user_dlts:");
	  /* This must match the format used in the user_dlts file */
	  g_string_sprintfa(pref_str,
		"\"User 0 (DLT=147)\",\"%s\",\"0\",\"\",\"0\",\"\"", spec_ptr);
      if (prefs_set_pref(pref_str->str) != PREFS_SET_OK) {
		return FALSE;
	  }
      g_string_free(pref_str, TRUE);
      return TRUE;
    }
  }
  return FALSE;
}

int
main(int argc, char *argv[])
{
  char                *init_progfile_dir_error;
  int                  opt, i;
  extern char         *optarg;
  gboolean             arg_error = FALSE;

#ifdef _WIN32
  WSADATA		wsaData;
#endif	/* _WIN32 */

  char                *gpf_path, *pf_path;
  char                *gdp_path, *dp_path;
  int                  gpf_open_errno, gpf_read_errno;
  int                  pf_open_errno, pf_read_errno;
  int                  gdp_open_errno, gdp_read_errno;
  int                  dp_open_errno, dp_read_errno;
  int                  err;
  gchar               *pipe_name = NULL;
  gchar               *rfilters[64];
  e_prefs             *prefs;
  char                 badopt;
  GLogLevelFlags       log_flags;
  GPtrArray           *disp_fields = g_ptr_array_new();
  guint                fc;

  #define OPTSTRING_INIT "d:F:hlnN:o:r:R:S:t:v"

  static const char    optstring[] = OPTSTRING_INIT;

  /*
   * Clear the filters arrays
   */
  memset(rfilters, 0, sizeof(rfilters));
  memset(rfcodes, 0, sizeof(rfcodes));
  n_rfilters = 0;
  n_rfcodes = 0;

  /*
   * Initialize our string format
   */
  string_fmts = g_ptr_array_new();

  /*
   * Attempt to get the pathname of the executable file.
   */
  init_progfile_dir_error = init_progfile_dir(argv[0]);
  if (init_progfile_dir_error != NULL) {
    fprintf(stderr, "rawshark: Can't get pathname of rawshark program: %s.\n",
            init_progfile_dir_error);
  }

  /*
   * Get credential information for later use.
   */
  get_credential_info();

  /* nothing more than the standard GLib handler, but without a warning */
  log_flags =
		    G_LOG_LEVEL_WARNING |
		    G_LOG_LEVEL_MESSAGE |
		    G_LOG_LEVEL_INFO |
		    G_LOG_LEVEL_DEBUG;

  g_log_set_handler(NULL,
		    log_flags,
		    log_func_ignore, NULL /* user_data */);
  g_log_set_handler(LOG_DOMAIN_CAPTURE_CHILD,
		    log_flags,
		    log_func_ignore, NULL /* user_data */);

  timestamp_set_type(TS_RELATIVE);
  timestamp_set_precision(TS_PREC_AUTO);

  /* Register all dissectors; we must do this before checking for the
     "-G" flag, as the "-G" flag dumps information registered by the
     dissectors, and we must do it before we read the preferences, in
     case any dissectors register preferences. */
  epan_init(register_all_protocols, register_all_protocol_handoffs, NULL, NULL,
            failure_message, open_failure_message, read_failure_message);

  /* Now register the preferences for any non-dissector modules.
     We must do that before we read the preferences as well. */
  prefs_register_modules();

  /* Set the C-language locale to the native environment. */
  setlocale(LC_ALL, "");

  prefs = read_prefs(&gpf_open_errno, &gpf_read_errno, &gpf_path,
                     &pf_open_errno, &pf_read_errno, &pf_path);
  if (gpf_path != NULL) {
    if (gpf_open_errno != 0) {
      cmdarg_err("Can't open global preferences file \"%s\": %s.",
              pf_path, strerror(gpf_open_errno));
    }
    if (gpf_read_errno != 0) {
      cmdarg_err("I/O error reading global preferences file \"%s\": %s.",
              pf_path, strerror(gpf_read_errno));
    }
  }
  if (pf_path != NULL) {
    if (pf_open_errno != 0) {
      cmdarg_err("Can't open your preferences file \"%s\": %s.", pf_path,
              strerror(pf_open_errno));
    }
    if (pf_read_errno != 0) {
      cmdarg_err("I/O error reading your preferences file \"%s\": %s.",
              pf_path, strerror(pf_read_errno));
    }
    g_free(pf_path);
    pf_path = NULL;
  }

  /* Set the name resolution code's flags from the preferences. */
  g_resolv_flags = prefs->name_resolve;

  /* Read the disabled protocols file. */
  read_disabled_protos_list(&gdp_path, &gdp_open_errno, &gdp_read_errno,
                            &dp_path, &dp_open_errno, &dp_read_errno);
  if (gdp_path != NULL) {
    if (gdp_open_errno != 0) {
      cmdarg_err("Could not open global disabled protocols file\n\"%s\": %s.",
                 gdp_path, strerror(gdp_open_errno));
    }
    if (gdp_read_errno != 0) {
      cmdarg_err("I/O error reading global disabled protocols file\n\"%s\": %s.",
                 gdp_path, strerror(gdp_read_errno));
    }
    g_free(gdp_path);
  }
  if (dp_path != NULL) {
    if (dp_open_errno != 0) {
      cmdarg_err(
        "Could not open your disabled protocols file\n\"%s\": %s.", dp_path,
        strerror(dp_open_errno));
    }
    if (dp_read_errno != 0) {
      cmdarg_err(
        "I/O error reading your disabled protocols file\n\"%s\": %s.", dp_path,
        strerror(dp_read_errno));
    }
    g_free(dp_path);
  }

#ifdef _WIN32
  /* Load Wpcap, if possible */
  load_wpcap();
#endif

  init_cap_file(&cfile);

  /* Assemble the compile-time version information string */
  comp_info_str = g_string_new("Compiled ");
  get_compiled_version_info(comp_info_str, get_epan_compiled_version_info);

  /* Assemble the run-time version information string */
  runtime_info_str = g_string_new("Running ");
  get_runtime_version_info(runtime_info_str, NULL);

  /* Print format defaults to this. */
  print_format = PR_FMT_TEXT;

  /* Initialize our encapsulation type */
  encap = WTAP_ENCAP_UNKNOWN;

  /* Now get our args */
  /* XXX - We should probably have an option to dump libpcap link types */
  while ((opt = getopt(argc, argv, optstring)) != -1) {
    switch (opt) {
      case 'd':        /* Payload type */
        if (!set_link_type(optarg)) {
          cmdarg_err("Invalid link type or protocol \"%s\"", optarg);
          exit(1);
        }
        break;
      case 'F':        /* Read field to display */
        g_ptr_array_add(disp_fields, g_strdup(optarg));
        break;
      case 'h':        /* Print help and exit */
        print_usage(TRUE);
        exit(0);
        break;
      case 'l':        /* "Line-buffer" standard output */
        /* This isn't line-buffering, strictly speaking, it's just
           flushing the standard output after the information for
           each packet is printed; however, that should be good
           enough for all the purposes to which "-l" is put (and
           is probably actually better for "-V", as it does fewer
           writes).

           See the comment in "process_packet()" for an explanation of
           why we do that, and why we don't just use "setvbuf()" to
           make the standard output line-buffered (short version: in
           Windows, "line-buffered" is the same as "fully-buffered",
           and the output buffer is only flushed when it fills up). */
        line_buffered = TRUE;
        break;
      case 'n':        /* No name resolution */
        g_resolv_flags = RESOLV_NONE;
        break;
      case 'N':        /* Select what types of addresses/port #s to resolve */
        if (g_resolv_flags == RESOLV_ALL)
          g_resolv_flags = RESOLV_NONE;
        badopt = string_to_name_resolve(optarg, &g_resolv_flags);
        if (badopt != '\0') {
          cmdarg_err("-N specifies unknown resolving option '%c'; valid options are 'm', 'n', and 't'",
                     badopt);
          exit(1);
        }
        break;
      case 'o':        /* Override preference from command line */
        switch (prefs_set_pref(optarg)) {

        case PREFS_SET_OK:
          break;

        case PREFS_SET_SYNTAX_ERR:
          cmdarg_err("Invalid -o flag \"%s\"", optarg);
          exit(1);
          break;

        case PREFS_SET_NO_SUCH_PREF:
        case PREFS_SET_OBSOLETE:
          cmdarg_err("-o flag \"%s\" specifies unknown preference", optarg);
          exit(1);
          break;
        }
        break;
      case 'r':        /* Read capture file xxx */
        pipe_name = g_strdup(optarg);
        break;
      case 'R':        /* Read file filter */
        if(n_rfilters < (int) sizeof(rfilters) / (int) sizeof(rfilters[0])) {
          rfilters[n_rfilters++] = optarg;
        }
        else {
          cmdarg_err("Too many display filters");
          exit(1);
        }
        break;
      case 'S':        /* Print string representations */
        if (!parse_field_string_format(optarg)) {
          cmdarg_err("Invalid field string format");
          exit(1);
        }
        break;
      case 't':        /* Time stamp type */
        if (strcmp(optarg, "r") == 0)
          timestamp_set_type(TS_RELATIVE);
        else if (strcmp(optarg, "a") == 0)
          timestamp_set_type(TS_ABSOLUTE);
        else if (strcmp(optarg, "ad") == 0)
          timestamp_set_type(TS_ABSOLUTE_WITH_DATE);
        else if (strcmp(optarg, "d") == 0)
          timestamp_set_type(TS_DELTA);
        else if (strcmp(optarg, "dd") == 0)
          timestamp_set_type(TS_DELTA_DIS);
        else if (strcmp(optarg, "e") == 0)
          timestamp_set_type(TS_EPOCH);
        else {
          cmdarg_err("Invalid time stamp type \"%s\"",
            optarg);
          cmdarg_err_cont("It must be \"r\" for relative, \"a\" for absolute,");
          cmdarg_err_cont("\"ad\" for absolute with date, or \"d\" for delta.");
          exit(1);
        }
        break;
      case 'v':        /* Show version and exit */
        printf("Rawshark " VERSION "%s\n"
               "\n"
               "%s"
               "\n"
               "%s"
               "\n"
               "%s",
               wireshark_svnversion, get_copyright_info(), comp_info_str->str,
               runtime_info_str->str);
        exit(0);
        break;

      default:
      case '?':        /* Bad flag - print usage message */
        print_usage(TRUE);
        exit(1);
        break;
    }
  }

  /* Initialize our display fields */
  for (fc = 0; fc < disp_fields->len; fc++) {
	protocolinfo_init(g_ptr_array_index(disp_fields, fc));
  }
  g_ptr_array_free(disp_fields, TRUE);
  printf("\n");
  fflush(stdout);

  /* If no capture filter or read filter has been specified, and there are
     still command-line arguments, treat them as the tokens of a capture
     filter (if no "-r" flag was specified) or a read filter (if a "-r"
     flag was specified. */
  if (optind < argc) {
    if (pipe_name != NULL) {
      if (n_rfilters != 0) {
        cmdarg_err("Read filters were specified both with \"-R\" "
            "and with additional command-line arguments");
        exit(1);
      }
      rfilters[n_rfilters] = get_args_as_string(argc, argv, optind);
    }
  }

  /* Make sure we got a dissector handle for our payload. */
  if (encap == WTAP_ENCAP_UNKNOWN) {
    cmdarg_err("No valid payload dissector specified.");
    exit(1);
  }

  if (arg_error) {
    print_usage(FALSE);
    exit(1);
  }


#ifdef _WIN32
  /* Start windows sockets */
  WSAStartup( MAKEWORD( 1, 1 ), &wsaData );
#endif /* _WIN32 */

  /* Notify all registered modules that have had any of their preferences
     changed either from one of the preferences file or from the command
     line that their preferences have changed. */
  prefs_apply_all();

  /* At this point MATE will have registered its field array so we can
     have a tap filter with one of MATE's late-registered fields as part
     of the filter.  We can now process all the "-z" arguments. */
  start_requested_stats();

  /* disabled protocols as per configuration file */
  if (gdp_path == NULL && dp_path == NULL) {
    set_disabled_protos_list();
  }

  /* Build the column format array */
  col_setup(&cfile.cinfo, prefs->num_cols);
  for (i = 0; i < cfile.cinfo.num_cols; i++) {
    cfile.cinfo.col_fmt[i] = get_column_format(i);
    cfile.cinfo.col_title[i] = g_strdup(get_column_title(i));
    cfile.cinfo.fmt_matx[i] = (gboolean *) g_malloc0(sizeof(gboolean) *
      NUM_COL_FMTS);
    get_column_format_matches(cfile.cinfo.fmt_matx[i], cfile.cinfo.col_fmt[i]);
    cfile.cinfo.col_data[i] = NULL;
    if (cfile.cinfo.col_fmt[i] == COL_INFO)
      cfile.cinfo.col_buf[i] = (gchar *) g_malloc(sizeof(gchar) * COL_MAX_INFO_LEN);
    else
      cfile.cinfo.col_buf[i] = (gchar *) g_malloc(sizeof(gchar) * COL_MAX_LEN);
    cfile.cinfo.col_fence[i] = 0;
    cfile.cinfo.col_expr[i] = (gchar *) g_malloc(sizeof(gchar) * COL_MAX_LEN);
    cfile.cinfo.col_expr_val[i] = (gchar *) g_malloc(sizeof(gchar) * COL_MAX_LEN);
  }

  for (i = 0; i < cfile.cinfo.num_cols; i++) {
      int j;

      for (j = 0; j < NUM_COL_FMTS; j++) {
         if (!cfile.cinfo.fmt_matx[i][j])
             continue;

         if (cfile.cinfo.col_first[j] == -1)
             cfile.cinfo.col_first[j] = i;
         cfile.cinfo.col_last[j] = i;
      }
  }

  if (n_rfilters != 0) {
    for (i = 0; i < n_rfilters; i++) {
      if (!dfilter_compile(rfilters[i], &rfcodes[n_rfcodes])) {
        cmdarg_err("%s", dfilter_error_msg);
        epan_cleanup();
        exit(2);
      }
	  n_rfcodes++;
    }
  }

  if (pipe_name) {
    /*
     * We're reading a pipe (or capture file).
     */

    /*
     * Immediately relinquish any special privileges we have; we must not
     * be allowed to read any capture files the user running Rawshark
     * can't open.
     */
    relinquish_special_privs_perm();

    if (raw_cf_open(&cfile, pipe_name) != CF_OK) {
      epan_cleanup();
      exit(2);
    }

    /* Set timestamp precision; there should arguably be a command-line
       option to let the user set this. */
#if 0
    switch(wtap_file_tsprecision(cfile.wth)) {
    case(WTAP_FILE_TSPREC_SEC):
      timestamp_set_precision(TS_PREC_AUTO_SEC);
      break;
    case(WTAP_FILE_TSPREC_DSEC):
      timestamp_set_precision(TS_PREC_AUTO_DSEC);
      break;
    case(WTAP_FILE_TSPREC_CSEC):
      timestamp_set_precision(TS_PREC_AUTO_CSEC);
      break;
    case(WTAP_FILE_TSPREC_MSEC):
      timestamp_set_precision(TS_PREC_AUTO_MSEC);
      break;
    case(WTAP_FILE_TSPREC_USEC):
      timestamp_set_precision(TS_PREC_AUTO_USEC);
      break;
    case(WTAP_FILE_TSPREC_NSEC):
      timestamp_set_precision(TS_PREC_AUTO_NSEC);
      break;
    default:
      g_assert_not_reached();
    }
#else
    timestamp_set_precision(TS_PREC_AUTO_USEC);
#endif

    /* Process the packets in the file */
    err = load_cap_file(&cfile);

    if (err != 0) {
      epan_cleanup();
      exit(2);
    }
  } else {
    /* If you want to capture live packets, use TShark. */
    cmdarg_err("Input file or pipe name not specified.");
    exit(2);
  }

  epan_cleanup();

  return 0;
}

/**
 * Read data from a raw pipe.  The "raw" data consists of a libpcap
 * packet header followed by the payload.
 * @param fd [IN] A POSIX file descriptor.  Because that's _exactly_ the sort
 *           of thing you want to use in Windows.
 * @param phdr [OUT] Packet header information.
 * @param err [OUT] Error indicator.  Uses wiretap values.
 * @param err_info [OUT] Error message.
 * @param data_offset [OUT] data offset in the pipe.
 * @return TRUE on success, FALSE on failure.
 */
static gboolean
raw_pipe_read(struct wtap_pkthdr *phdr, guchar * pd, int *err, gchar **err_info, gint64 *data_offset) {
  struct pcap_pkthdr hdr;
  int bytes_read = 0;
  int bytes_needed = sizeof(struct pcap_pkthdr);
  gchar err_str[1024+1];

  /* Copied from capture_loop.c */
  while (bytes_needed > 0) {
    bytes_read = read(fd, &hdr, bytes_needed);
    if (bytes_read == 0) {
      *err = 0;
      return FALSE;
    } else if (bytes_read < 0) {
      *err = WTAP_ERR_CANT_READ;
      *err_info = "Error reading header from pipe";
      return FALSE;
    }
    bytes_needed -= bytes_read;
    *data_offset += bytes_read;
  }

  bytes_read = 0;
  phdr->ts.secs = hdr.ts.tv_sec;
  phdr->ts.nsecs = hdr.ts.tv_usec * 1000;
  phdr->caplen = bytes_needed = hdr.caplen;
  phdr->len = hdr.len;
  phdr->pkt_encap = encap;

#if 0
  printf("tv_sec: %d (%04x)\n", hdr.ts.tv_sec, hdr.ts.tv_sec);
  printf("tv_usec: %d (%04x)\n", hdr.ts.tv_usec, hdr.ts.tv_usec);
  printf("caplen: %d (%04x)\n", hdr.caplen, hdr.caplen);
  printf("len: %d (%04x)\n", hdr.len, hdr.len);
#endif
  if (bytes_needed > WTAP_MAX_PACKET_SIZE) {
    *err = WTAP_ERR_BAD_RECORD;
    g_snprintf(err_str, 1024, "Bad packet length: %d (%04x)", bytes_needed, bytes_needed);
    *err_info = err_str;
    return FALSE;
  }

  while (bytes_needed > 0) {
    bytes_read = read(fd, pd, bytes_needed);
    if (bytes_read == 0) {
      *err = WTAP_ERR_SHORT_READ;
      *err_info = "Got zero bytes reading data from pipe";
      return FALSE;
    } else if (bytes_read < 0) {
      *err = WTAP_ERR_CANT_READ;
      *err_info = "Error reading data from pipe";
      return FALSE;
    }
    bytes_needed -= bytes_read;
    *data_offset += bytes_read;
  }
  return TRUE;
}

static int
load_cap_file(capture_file *cf)
{
  int          err;
  gchar        *err_info;
  gint64       data_offset = 0;
  char         *save_file_string = NULL;
  struct wtap_pkthdr  phdr;
  guchar       pd[WTAP_MAX_PACKET_SIZE];

  while (raw_pipe_read(&phdr, pd, &err, &err_info, &data_offset)) {
    process_packet(cf, data_offset, &phdr, pd);
  }

  if (err != 0) {
    /* Print a message noting that the read failed somewhere along the line. */
    switch (err) {

    case WTAP_ERR_UNSUPPORTED_ENCAP:
      cmdarg_err("\"%s\" has a packet with a network type that Rawshark doesn't support.\n(%s)",
                 cf->filename, err_info);
      break;

    case WTAP_ERR_CANT_READ:
      cmdarg_err("An attempt to read from \"%s\" failed for some unknown reason.",
                 cf->filename);
      break;

    case WTAP_ERR_SHORT_READ:
      cmdarg_err("\"%s\" appears to have been cut short in the middle of a packet.",
                 cf->filename);
      break;

    case WTAP_ERR_BAD_RECORD:
      cmdarg_err("\"%s\" appears to be damaged or corrupt.\n(%s)",
                 cf->filename, err_info);
      break;

    default:
      cmdarg_err("An error occurred while reading \"%s\": %s.",
                 cf->filename, wtap_strerror(err));
      break;
    }
  }

  if (save_file_string != NULL)
    g_free(save_file_string);

  return err;
}

static void
fill_in_fdata(frame_data *fdata, capture_file *cf,
              const struct wtap_pkthdr *phdr, gint64 offset)
{
  fdata->next = NULL;
  fdata->prev = NULL;
  fdata->pfd = NULL;
  fdata->num = cf->count;
  fdata->pkt_len = phdr->len;
  cum_bytes += phdr->len;
  fdata->cum_bytes  = cum_bytes;
  fdata->cap_len = phdr->caplen;
  fdata->file_off = offset;
  fdata->lnk_t = phdr->pkt_encap;
  fdata->abs_ts.secs = phdr->ts.secs;
  fdata->abs_ts.nsecs = phdr->ts.nsecs;
  fdata->flags.passed_dfilter = 0;
  fdata->flags.encoding = CHAR_ASCII;
  fdata->flags.visited = 0;
  fdata->flags.marked = 0;
  fdata->flags.ref_time = 0;
  fdata->color_filter = NULL;

  /* If we don't have the time stamp of the first packet in the
     capture, it's because this is the first packet.  Save the time
     stamp of this packet as the time stamp of the first packet. */
  if (nstime_is_unset(&first_ts)) {
    first_ts = fdata->abs_ts;
  }

  /* If we don't have the time stamp of the previous captured packet,
     it's because this is the first packet.  Save the time
     stamp of this packet as the time stamp of the previous captured
     packet. */
  if (nstime_is_unset(&prev_cap_ts)) {
    prev_cap_ts = fdata->abs_ts;
  }

  /* Get the time elapsed between the first packet and this packet. */
  nstime_delta(&fdata->rel_ts, &fdata->abs_ts, &first_ts);

  /* If it's greater than the current elapsed time, set the elapsed time
     to it (we check for "greater than" so as not to be confused by
     time moving backwards). */
  if ((gint32)cf->elapsed_time.secs < fdata->rel_ts.secs
	|| ((gint32)cf->elapsed_time.secs == fdata->rel_ts.secs && (gint32)cf->elapsed_time.nsecs < fdata->rel_ts.nsecs)) {
    cf->elapsed_time = fdata->rel_ts;
  }

  /* Get the time elapsed between the previous displayed packet and
     this packet. */
  if (nstime_is_unset(&prev_dis_ts))
    nstime_set_zero(&fdata->del_dis_ts);
  else
    nstime_delta(&fdata->del_dis_ts, &fdata->abs_ts, &prev_dis_ts);

  /* Get the time elapsed between the previous captured packet and
     this packet. */
  nstime_delta(&fdata->del_cap_ts, &fdata->abs_ts, &prev_cap_ts);
  prev_cap_ts = fdata->abs_ts;
}

/* Free up all data attached to a "frame_data" structure. */
static void
clear_fdata(frame_data *fdata)
{
  if (fdata->pfd)
    g_slist_free(fdata->pfd);
}

static gboolean
process_packet(capture_file *cf, gint64 offset, const struct wtap_pkthdr *whdr,
               const guchar *pd)
{
  frame_data fdata;
  gboolean create_proto_tree;
  epan_dissect_t *edt;
  gboolean passed;
  union wtap_pseudo_header pseudo_header;
  int i;

  if(whdr->len == 0)
  {
	  /* The user sends an empty packet when he wants to get output from us even if we don't currently have
	     packets to process. We spit out a line with the timestamp and the text "void"
	  */
	  printf("%lu %lu %lu void -\n", (unsigned long int)cf->count,
		  (unsigned long int)whdr->ts.secs,
		  (unsigned long int)whdr->ts.nsecs);

	  fflush(stdout);

	  return FALSE;
  }

  memset(&pseudo_header, 0, sizeof(pseudo_header));

  /* Count this packet. */
  cf->count++;

  /* If we're going to print packet information, or we're going to
     run a read filter, or we're going to process taps, set up to
     do a dissection and do so. */
  fill_in_fdata(&fdata, cf, whdr, offset);

  passed = TRUE;
  create_proto_tree = TRUE;

  /* The protocol tree will be "visible", i.e., printed, only if we're
     printing packet details, which is true if we're in verbose mode ("verbose"
     is true). */
  edt = epan_dissect_new(create_proto_tree, FALSE);

  /* If we're running a read filter, prime the epan_dissect_t with that
     filter. */
  if (n_rfilters > 0) {
    for(i = 0; i < n_rfcodes; i++) {
      epan_dissect_prime_dfilter(edt, rfcodes[i]);
    }
  }

  tap_queue_init(edt);

  printf("%lu", (unsigned long int)cf->count);

  /* We only need the columns if we're printing packet info but we're
     *not* verbose; in verbose mode, we print the protocol tree, not
     the protocol summary. */
  epan_dissect_run(edt, &pseudo_header, pd, &fdata, &cf->cinfo);

  tap_push_tapped_queue(edt);

  for(i = 0; i < n_rfilters; i++) {
    /* Run the read filter if we have one. */
    if (rfcodes[i])
      passed = dfilter_apply_edt(rfcodes[i], edt);
    else
      passed = TRUE;

    /* Print a one-line summary */
    printf(" %u", passed ? 1 : 0);
  }

  printf(" -\n");

  /* The ANSI C standard does not appear to *require* that a line-buffered
     stream be flushed to the host environment whenever a newline is
     written, it just says that, on such a stream, characters "are
     intended to be transmitted to or from the host environment as a
     block when a new-line character is encountered".

     The Visual C++ 6.0 C implementation doesn't do what is intended;
     even if you set a stream to be line-buffered, it still doesn't
     flush the buffer at the end of every line.

     So, if the "-l" flag was specified, we flush the standard output
     at the end of a packet.  This will do the right thing if we're
     printing packet summary lines, and, as we print the entire protocol
     tree for a single packet without waiting for anything to happen,
     it should be as good as line-buffered mode if we're printing
     protocol trees.  (The whole reason for the "-l" flag in either
     tcpdump or Rawshark is to allow the output of a live capture to
     be piped to a program or script and to have that script see the
     information for the packet as soon as it's printed, rather than
     having to wait until a standard I/O buffer fills up. */
  if (line_buffered)
    fflush(stdout);

  if (ferror(stdout)) {
    show_print_file_io_error(errno);
    exit(2);
  }

  epan_dissect_free(edt);
  clear_fdata(&fdata);

  return passed;
}

/****************************************************************************************
 * FIELD EXTRACTION ROUTINES
 ****************************************************************************************/
typedef struct _pci_t {
	char *filter;
	int hf_index;
	int cmd_line_index;
} pci_t;

char* ftenum_to_string(header_field_info *hfi)
{
	if (!hfi) {
		return "n.a.";
	}

	if (string_fmts->len > 0 && hfi->strings) {
		return "FT_STRING";
	}

	switch(hfi->type) {
		case FT_NONE:
			return "FT_NONE";
		case FT_PROTOCOL:
			return "FT_PROTOCOL";
		case FT_BOOLEAN:
			return "FT_BOOLEAN";
		case FT_UINT8:
			return "FT_UINT8";
		case FT_UINT16:
			return "FT_UINT16";
		case FT_UINT24:
			return "FT_UINT24";
		case FT_UINT32:
			return "FT_UINT32";
		case FT_UINT64:
			return "FT_UINT64";
		case FT_INT8:
			return "FT_INT8";
		case FT_INT16:
			return "FT_INT16";
		case FT_INT24:
			return "FT_INT24";
		case FT_INT32:
			return "FT_INT32";
		case FT_INT64:
			return "FT_INT64";
		case FT_FLOAT:
			return "FT_FLOAT";
		case FT_DOUBLE:
			return "FT_DOUBLE";
		case FT_ABSOLUTE_TIME:
			return "FT_ABSOLUTE_TIME";
		case FT_RELATIVE_TIME:
			return "FT_RELATIVE_TIME";
		case FT_STRING:
			return "FT_STRING";
		case FT_STRINGZ:
			return "FT_STRINGZ";
		case FT_UINT_STRING:
			return "FT_UINT_STRING";
		case FT_ETHER:
			return "FT_ETHER";
		case FT_BYTES:
			return "FT_BYTES";
		case FT_UINT_BYTES:
			return "FT_UINT_BYTES";
		case FT_IPv4:
			return "FT_IPv4";
		case FT_IPv6:
			return "FT_IPv6";
		case FT_IPXNET:
			return "FT_IPXNET";
		case FT_FRAMENUM:
			return "FT_FRAMENUM";
		case FT_PCRE:
			return "FT_PCRE";
		case FT_GUID:
			return "FT_GUID";
		case FT_OID:
			return "FT_OID";
		case FT_NUM_TYPES:
			return "FT_NUM_TYPES";
		default:
			return "n.a.";
	};
}

char* base_display_e_to_string(base_display_e bd)
{
	switch(bd) {
	case BASE_NONE:
			return "BASE_NONE";
	case BASE_DEC:
			return "BASE_DEC";
	case BASE_HEX:
			return "BASE_HEX";
	case BASE_OCT:
			return "BASE_OCT";
	case BASE_DEC_HEX:
			return "BASE_DEC_HEX";
	case BASE_HEX_DEC:
			return "BASE_HEX_DEC";
	default:
			return "n.a.";
	}
}

/*
 * Copied from various parts of proto.c
 */
#define FIELD_STR_INIT_LEN 256
#define cVALS(x) (const value_string*)(x)
static gboolean print_field_value(field_info *finfo, int cmd_line_index)
{
	header_field_info	*hfinfo;
	static char			*fs_buf = NULL;
	char				*fs_ptr = fs_buf;
	static GString     *label_s = NULL;
	int					fs_buf_len = FIELD_STR_INIT_LEN, fs_len;
	guint              i;
	string_fmt_t       *sf;
	guint32            uvalue;
	gint32             svalue;
	const true_false_string *tfstring = &tfs_true_false;

	hfinfo = finfo->hfinfo;

	if (!fs_buf) {
		fs_buf = g_malloc(fs_buf_len + 1);
		fs_ptr = fs_buf;
	}

	if (!label_s) {
		label_s = g_string_new("");
	}

	if(finfo->value.ftype->val_to_string_repr)
	{
		/*
		 * this field has an associated value,
		 * e.g: ip.hdr_len
		 */
		fs_len = fvalue_string_repr_len(&finfo->value, FTREPR_DFILTER);
		while (fs_buf_len < fs_len) {
			fs_buf_len *= 2;
			fs_buf = g_realloc(fs_buf, fs_buf_len + 1);
			fs_ptr = fs_buf;
		}
		fvalue_to_string_repr(&finfo->value,
			FTREPR_DFILTER,
			fs_buf);

		/* String types are quoted. Remove them. */
		if ((finfo->value.ftype->ftype == FT_STRING || finfo->value.ftype->ftype == FT_STRINGZ) && fs_len > 2) {
			fs_buf[fs_len - 1] = '\0';
			fs_ptr++;
		}
	}

	if (string_fmts->len > 0 && finfo->hfinfo->strings) {
		g_string_truncate(label_s, 0);
		for (i = 0; i < string_fmts->len; i++) {
			sf = g_ptr_array_index(string_fmts, i);
			if (sf->plain) {
				g_string_append(label_s, sf->plain);
			} else {
				switch (sf->format) {
					case SF_NAME:
						g_string_append(label_s, hfinfo->name);
						break;
					case SF_NUMVAL:
						g_string_append(label_s, fs_ptr);
						break;
					case SF_STRVAL:
						switch(hfinfo->type) {
							case FT_BOOLEAN:
								uvalue = fvalue_get_uinteger(&finfo->value);
								tfstring = (const struct true_false_string*) hfinfo->strings;
								g_string_append(label_s, uvalue ? tfstring->true_string : tfstring->false_string);
								break;
							case FT_INT8:
							case FT_INT16:
							case FT_INT24:
							case FT_INT32:
								DISSECTOR_ASSERT(!hfinfo->bitmask);
								svalue = fvalue_get_sinteger(&finfo->value);
								if (hfinfo->display & BASE_RANGE_STRING) {
								  g_string_append(label_s, rval_to_str(svalue, hfinfo->strings, "Unknown"));
								} else {
								  g_string_append(label_s, val_to_str(svalue, cVALS(hfinfo->strings), "Unknown"));
								}
							case FT_UINT8:
							case FT_UINT16:
							case FT_UINT24:
							case FT_UINT32:
								uvalue = fvalue_get_uinteger(&finfo->value);
								if (!hfinfo->bitmask && hfinfo->display & BASE_RANGE_STRING) {
								  g_string_append(label_s, rval_to_str(uvalue, hfinfo->strings, "Unknown"));
								} else {
								  g_string_append(label_s, val_to_str(uvalue, cVALS(hfinfo->strings), "Unknown"));
								}
								break;
							default:
								break;
						}
						break;
					default:
						break;
				}
			}
		}
		printf(" %u=\"%s\"", cmd_line_index, label_s->str);
		return TRUE;
	}

	if(finfo->value.ftype->val_to_string_repr)
	{
		printf(" %u=\"%s\"", cmd_line_index, fs_ptr);
		return TRUE;
	}

	/*
	 * This field doesn't have an associated value,
	 * e.g. http
	 * We return n.a.
	 */
	printf(" %u=\"n.a.\"", cmd_line_index);
	return TRUE;
}

static int
protocolinfo_packet(void *prs, packet_info *pinfo _U_, epan_dissect_t *edt, const void *dummy _U_)
{
	pci_t *rs=prs;
	GPtrArray *gp;
	guint i;

	gp=proto_get_finfo_ptr_array(edt->tree, rs->hf_index);
	if(!gp){
		printf(" n.a.");
		return 0;
	}

	/*
	 * Print each occurrence of the field
	 */
	for (i = 0; i < gp->len; i++) {
		print_field_value(gp->pdata[i], rs->cmd_line_index);
	}

	return 0;
}

int g_cmd_line_index = 0;

/*
 * field must be persistent - we don't g_strdup() it below
 */
static void
protocolinfo_init(char *field)
{
	pci_t *rs;
	header_field_info *hfi;
	GString *error_string;

	hfi=proto_registrar_get_byname(field);
	if(!hfi){
		fprintf(stderr, "rawshark: Field \"%s\" doesn't exist.\n", field);
		exit(1);
	}

	printf("%u %s %s - ",
		g_cmd_line_index,
		ftenum_to_string(hfi),
		base_display_e_to_string(hfi->display));

	rs=g_malloc(sizeof(pci_t));
	rs->hf_index=hfi->id;
	rs->filter=field;
	rs->cmd_line_index = g_cmd_line_index++;

	error_string=register_tap_listener("frame", rs, rs->filter, NULL, protocolinfo_packet, NULL);
	if(error_string){
		/* error, we failed to attach to the tap. complain and clean up */
		fprintf(stderr, "rawshark: Couldn't register field extraction tap: %s\n",
		    error_string->str);
		g_string_free(error_string, TRUE);
		if(rs->filter){
			g_free(rs->filter);
		}
		g_free(rs);

		exit(1);
	}
}

/*
 * Given a format string, split it into a GPtrArray of string_fmt_t structs
 * and fill in string_fmt_parts.
 */

static void
add_string_fmt(string_fmt_e format, gchar *plain) {
	string_fmt_t *sf = g_malloc(sizeof(string_fmt_t));

	sf->format = format;
	sf->plain = g_strdup(plain);

	g_ptr_array_add(string_fmts, sf);
}

static gboolean
parse_field_string_format(gchar *format) {
	GString *plain_s = g_string_new("");
	int len;
	int pos = 0;

	if (!format) {
		return FALSE;
	}

	len = strlen(format);
	g_ptr_array_set_size(string_fmts, 0);

	while (pos < len) {
		if (format[pos] == '%') {
			if (pos >= len) { /* There should always be a following character */
				return FALSE;
			}
			pos++;
			if (plain_s->len > 0) {
				add_string_fmt(SF_NONE, plain_s->str);
				g_string_truncate(plain_s, 0);
			}
			switch (format[pos]) {
				case 'D':
					add_string_fmt(SF_NAME, NULL);
					break;
				case 'N':
					add_string_fmt(SF_NUMVAL, NULL);
					break;
				case 'S':
					add_string_fmt(SF_STRVAL, NULL);
					break;
				case '%':
					g_string_append_c(plain_s, '%');
					break;
				default: /* Invalid format */
					return FALSE;
			}
		} else {
			g_string_append_c(plain_s, format[pos]);
		}
		pos++;
	}

	if (plain_s->len > 0) {
		add_string_fmt(SF_NONE, plain_s->str);
	}
	g_string_free(plain_s, TRUE);

	return TRUE;
}
/****************************************************************************************
 * END OF FIELD EXTRACTION ROUTINES
 ****************************************************************************************/

static void
show_print_file_io_error(int err)
{
  switch (err) {

  case ENOSPC:
    cmdarg_err("Not all the packets could be printed because there is "
"no space left on the file system.");
    break;

#ifdef EDQUOT
  case EDQUOT:
    cmdarg_err("Not all the packets could be printed because you are "
"too close to, or over your disk quota.");
  break;
#endif

  default:
    cmdarg_err("An error occurred while printing packets: %s.",
      strerror(err));
    break;
  }
}

/*
 * Open/create errors are reported with an console message in Rawshark.
 */
static void
open_failure_message(const char *filename, int err, gboolean for_writing)
{
  fprintf(stderr, "rawshark: ");
  fprintf(stderr, file_open_error_message(err, for_writing), filename);
  fprintf(stderr, "\n");
}

cf_status_t
raw_cf_open(capture_file *cf, const char *fname)
{
  if ((fd = raw_pipe_open(fname)) < 0)
    return CF_ERROR;

  /* The open succeeded.  Fill in the information for this file. */

  /* Initialize all data structures used for dissection. */
  init_dissection();

  cf->wth = NULL;
  cf->f_datalen = 0; /* not used, but set it anyway */

  /* Set the file name because we need it to set the follow stream filter.
     XXX - is that still true?  We need it for other reasons, though,
     in any case. */
  cf->filename = g_strdup(fname);

  /* Indicate whether it's a permanent or temporary file. */
  cf->is_tempfile = FALSE;

  /* If it's a temporary capture buffer file, mark it as not saved. */
  cf->user_saved = FALSE;

  cf->cd_t      = WTAP_FILE_UNKNOWN;
  cf->count     = 0;
  cf->drops_known = FALSE;
  cf->drops     = 0;
  cf->has_snap = FALSE;
  cf->snap = WTAP_MAX_PACKET_SIZE;
  nstime_set_zero(&cf->elapsed_time);
  nstime_set_unset(&first_ts);
  nstime_set_unset(&prev_dis_ts);
  nstime_set_unset(&prev_cap_ts);

  return CF_OK;
}


/*
 * General errors are reported with an console message in Rawshark.
 */
static void
failure_message(const char *msg_format, va_list ap)
{
  fprintf(stderr, "rawshark: ");
  vfprintf(stderr, msg_format, ap);
  fprintf(stderr, "\n");
}

/*
 * Read errors are reported with an console message in Rawshark.
 */
static void
read_failure_message(const char *filename, int err)
{
  cmdarg_err("An error occurred while reading from the file \"%s\": %s.",
          filename, strerror(err));
}

/*
 * Report an error in command-line arguments.
 */
void
cmdarg_err(const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  fprintf(stderr, "rawshark: ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
}

/*
 * Report additional information for an error in command-line arguments.
 */
void
cmdarg_err_cont(const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
}


/****************************************************************************************************************/
/* other "dummies" */
void
cf_mark_frame(capture_file *cf _U_, frame_data *frame _U_)
{
    /* shouldn't happen */
    g_assert_not_reached();
}

/*
 * Editor modelines
 *
 * Local Variables:
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: tabs
 * End:
 *
 * ex: set shiftwidth=4 tabstop=4 noexpandtab
 * :indentSize=4:tabSize=4:noTabs=false:
 */
