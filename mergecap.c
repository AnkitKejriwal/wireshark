/* Combine two dump files, either by appending or by merging by timestamp
 *
 * $Id$
 *
 * Written by Scott Renfro <scott@renfro.org> based on
 * editcap by Richard Sharpe and Guy Harris
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <glib.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include <string.h>
#include "wtap.h"

#ifdef NEED_GETOPT_H
#include "getopt.h"
#endif

#include "svnversion.h"
#include "merge.h"

#ifdef HAVE_IO_H
# include <io.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

/* Win32 needs the O_BINARY flag for open() */
#ifndef O_BINARY
#define O_BINARY 	0
#endif

static int
get_natural_int(const char *string, const char *name)
{
  long number;
  char *p;

  number = strtol(string, &p, 10);
  if (p == string || *p != '\0') {
    fprintf(stderr, "mergecap: The specified %s \"%s\" is not a decimal number\n",
	    name, string);
    exit(1);
  }
  if (number < 0) {
    fprintf(stderr, "mergecap: The specified %s is a negative number\n",
	    name);
    exit(1);
  }
  if (number > INT_MAX) {
    fprintf(stderr, "mergecap: The specified %s is too large (greater than %d)\n",
	    name, INT_MAX);
    exit(1);
  }
  return number;
}

static int
get_positive_int(const char *string, const char *name)
{
  long number;

  number = get_natural_int(string, name);

  if (number == 0) {
    fprintf(stderr, "mergecap: The specified %s is zero\n",
	    name);
    exit(1);
  }

  return number;
}

/*
 * Show the usage
 */
static void
usage(void)
{
  int i;
  const char *string;

  printf("Usage: mergecap [-hva] [-s <snaplen>] [-T <encap type>]\n");
  printf("          [-F <capture type>] -w <outfile> <infile> [...]\n\n");
  printf("  where\t-h produces this help listing.\n");
  printf("       \t-v verbose operation, default is silent\n");
  printf("       \t-a files should be concatenated, not merged\n");
  printf("       \t     Default merges based on frame timestamps\n");
  printf("       \t-s <snaplen>: truncate packets to <snaplen> bytes of data\n");
  printf("       \t-w <outfile>: sets output filename to <outfile>\n");
  printf("       \t-T <encap type> encapsulation type to use:\n");
  for (i = 0; i < WTAP_NUM_ENCAP_TYPES; i++) {
      string = wtap_encap_short_string(i);
      if (string != NULL)
        printf("       \t     %s - %s\n",
          string, wtap_encap_string(i));
  }
  printf("       \t     default is the same as the first input file\n");
  printf("       \t-F <capture type> capture file type to write:\n");
  for (i = 0; i < WTAP_NUM_FILE_TYPES; i++) {
    if (wtap_dump_can_open(i))
      printf("       \t     %s - %s\n",
        wtap_file_type_short_string(i), wtap_file_type_string(i));
  }
  printf("       \t     default is libpcap\n");
}

int
main(int argc, char *argv[])
{
  extern char *optarg;
  extern int   optind;
  int          opt;
  gboolean     do_append     = FALSE;
  gboolean     verbose       = FALSE;
  int          in_file_count = 0;
  int          snaplen = 0;
  int          file_type = WTAP_FILE_PCAP;	/* default to libpcap format */
  int          frame_type = -2;
  int          out_fd;
  merge_in_file_t   *in_files      = NULL;
  int          i;
  merge_out_file_t   out_file;
  int          err, close_err;
  gchar       *err_info;
  int          err_fileno;
  char        *out_filename = NULL;
  gboolean     ret;

  /* Process the options first */
  while ((opt = getopt(argc, argv, "hvas:T:F:w:")) != -1) {

    switch (opt) {
    case 'w':
      out_filename = optarg;
      break;

    case 'a':
      do_append = !do_append;
      break;

    case 'T':
      frame_type = wtap_short_string_to_encap(optarg);
      if (frame_type < 0) {
      	fprintf(stderr, "mergecap: \"%s\" is not a valid encapsulation type\n",
      	    optarg);
      	exit(1);
      }
      break;

    case 'F':
      file_type = wtap_short_string_to_file_type(optarg);
      if (file_type < 0) {
      	fprintf(stderr, "mergecap: \"%s\" is not a valid capture file type\n",
      	    optarg);
      	exit(1);
      }
      break;

    case 'v':
      verbose = TRUE;
      break;

    case 's':
      snaplen = get_positive_int(optarg, "snapshot length");
      break;

    case 'h':
      printf("mergecap version %s"
#ifdef SVNVERSION
	  " (" SVNVERSION ")"
#endif
	  "\n", VERSION);
      usage();
      exit(0);
      break;

    case '?':              /* Bad options if GNU getopt */
      usage();
      exit(1);
      break;

    }

  }

  /* check for proper args; at a minimum, must have an output
   * filename and one input file
   */
  in_file_count = argc - optind;
  if (!out_filename) {
    fprintf(stderr, "mergecap: an output filename must be set with -w\n");
    fprintf(stderr, "          run with -h for help\n");
    exit(1);
  }
  if (in_file_count < 1) {
    fprintf(stderr, "mergecap: No input files were specified\n");
    exit(1);
  }

  /* open the input files */
  if (!merge_open_in_files(in_file_count, &argv[optind], &in_files,
                           &err, &err_info, &err_fileno)) {
    fprintf(stderr, "mergecap: Can't open %s: %s\n", argv[optind + err_fileno],
        wtap_strerror(err));
    switch (err) {

    case WTAP_ERR_UNSUPPORTED:
    case WTAP_ERR_UNSUPPORTED_ENCAP:
    case WTAP_ERR_BAD_RECORD:
      fprintf(stderr, "(%s)\n", err_info);
      g_free(err_info);
      break;
    }
    exit(1);
  }

  if (verbose) {
    for (i = 0; i < in_file_count; i++)
      fprintf(stderr, "mergecap: %s is type %s.\n", argv[optind + i],
              wtap_file_type_string(wtap_file_type(in_files[i].wth)));
  }

  if (snaplen == 0) {
    /*
     * Snapshot length not specified - default to the maximum of the
     * snapshot lengths of the input files.
     */
    snaplen = merge_max_snapshot_length(in_file_count, in_files);
  }

  /* set the outfile frame type */
  if (frame_type == -2) {
    /*
     * Default to the appropriate frame type for the input files.
     */
    frame_type = merge_select_frame_type(in_file_count, in_files);
    if (verbose) {
      if (frame_type == WTAP_ENCAP_PER_PACKET) {
        /*
         * Find out why we had to choose WTAP_ENCAP_PER_PACKET.
         */
        int first_frame_type, this_frame_type;

        first_frame_type = wtap_file_encap(in_files[0].wth);
        for (i = 1; i < in_file_count; i++) {
          this_frame_type = wtap_file_encap(in_files[i].wth);
          if (first_frame_type != this_frame_type) {
            fprintf(stderr, "mergecap: multiple frame encapsulation types detected\n");
            fprintf(stderr, "          defaulting to WTAP_ENCAP_PER_PACKET\n");
            fprintf(stderr, "          %s had type %s (%s)\n",
                    in_files[0].filename,
                    wtap_encap_string(first_frame_type),
                    wtap_encap_short_string(first_frame_type));
            fprintf(stderr, "          %s had type %s (%s)\n",
                    in_files[i].filename,
                    wtap_encap_string(this_frame_type),
                    wtap_encap_short_string(this_frame_type));
            break;
          }
        }
      }
      fprintf(stderr, "mergecap: selected frame_type %s (%s)\n",
              wtap_encap_string(frame_type),
              wtap_encap_short_string(frame_type));
    }
  }

  /* open the outfile */
  if (strncmp(out_filename, "-", 2) == 0) {  
    /* use stdout as the outfile */
    out_fd = 1 /*stdout*/;
  } else {
    /* open the outfile */
    out_fd = open(out_filename, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0644);
    if (out_fd == -1) {
      fprintf(stderr, "mergecap: Couldn't open output file %s: %s\n",
              out_filename, strerror(errno));
      exit(1);
    }
  }  
    
  /* prepare the outfile */
  if (!merge_open_outfile(&out_file, out_fd, file_type, frame_type, snaplen,
                          &err)) {
    merge_close_in_files(in_file_count, in_files);
    free(in_files);
    fprintf(stderr, "mergecap: Can't open or create %s: %s\n", out_filename,
            wtap_strerror(err));
    exit(1);
  }

  /* do the merge (or append) */
  if (do_append)
    ret = merge_append_files(in_file_count, in_files, &out_file, &err);
  else
    ret = merge_files(in_file_count, in_files, &out_file, &err);

  merge_close_in_files(in_file_count, in_files);
  if (ret)
    ret = merge_close_outfile(&out_file, &err);
  else
    merge_close_outfile(&out_file, &close_err);
  if (!ret) {
    fprintf(stderr, "mergecap: Error writing to outfile: %s\n",
            wtap_strerror(err));
  }

  free(in_files);

  return ret ? 0 : 2;
}
