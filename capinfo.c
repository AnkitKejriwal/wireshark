/* capinfo.c
 * Reports capture file information including # of packets, duration, others
 *
 * Copyright 2004 Ian Schorr
 *
 * $Id: $
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
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <string.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include <string.h>
#include <epan/packet.h>
#include "wtap.h"

#ifdef NEED_GETOPT_H
#include "getopt.h"
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif


static gboolean cap_file_type = FALSE;      /* Do not report capture type     */
static gboolean cap_packet_count = FALSE;   /* Do not produce packet count    */
static gboolean cap_file_size = FALSE;      /* Do not report file size        */
static gboolean cap_data_size = FALSE;      /* Do not report packet byte size */
static gboolean cap_duration = FALSE;       /* Do not report capture duration */
static gboolean cap_start_time = FALSE;
static gboolean cap_end_time = FALSE;

static gboolean cap_data_rate_byte = FALSE;
static gboolean cap_data_rate_bit = FALSE;
static gboolean cap_packet_size = FALSE;


typedef struct _capture_info {
	const char		*filename;
	guint16			file_type;
	guint64			filesize;
	guint64			packet_bytes;
	double			start_time;
	double			stop_time;
	guint32			packet_count;
	gboolean		snap_set;
	guint32			snaplen;
	gboolean		drops_known;
	guint32			drop_count;
	
	double			duration;
	double			packet_rate;
	double			packet_size;
	double			data_rate;		/* in bytes */
} capture_info;

static double
secs_usecs(guint32 s, guint32 us)
{
  return (us / 1000000.0) + (double)s;
}

static void
print_stats(capture_info *cf_info)
{
  gchar			*file_type_string;
  time_t		start_time_t;
  struct tm		*start_time_tm;
  time_t		stop_time_t;
  struct tm		*stop_time_tm;

  /* Build printable strings for various stats */
  file_type_string = wtap_file_type_string(cf_info->file_type);
  start_time_t = (long)cf_info->start_time;
  stop_time_t = (long)cf_info->stop_time;
  start_time_tm = localtime (&start_time_t);
  stop_time_tm = localtime (&stop_time_t);

  if (cap_file_type) printf("File Type: %s\n", file_type_string);
  if (cap_packet_count) printf("Number of packets: %u \n", cf_info->packet_count);
  if (cap_file_size) printf("File Size: %" PRIu64 " bytes\n", cf_info->filesize);
  if (cap_data_size) printf("Data Size: %" PRIu64 " bytes\n", cf_info->packet_bytes);
  if (cap_duration) printf("Capture duration: %f seconds\n", cf_info->duration);
  if (cap_start_time) printf("Start time: %s", asctime (start_time_tm));
  if (cap_end_time) printf("End time: %s", asctime (stop_time_tm));
  if (cap_data_rate_byte) printf("Data rate: %.2f bytes/s\n", cf_info->data_rate);
  if (cap_data_rate_bit) printf("Data rate: %.2f bits/s\n", cf_info->data_rate*8);
  if (cap_packet_size) printf("Average packet size: %.2f bytes\n", cf_info->packet_size);

}

static int 
process_cap_file(wtap *wth)
{
  int			err;
  gchar			*err_info;
  struct stat   cf_stat;
  long  		data_offset;
  
  guint32		packet = 0;
  gint64		bytes = 0;
  const struct wtap_pkthdr *phdr;
  capture_info  cf_info;
  double		start_time = 0;
  double		stop_time = 0;
  double		cur_time = 0;
  
  /* Tally up data that we need to parse through the file to find */
  while (wtap_read(wth, &err, &err_info, &data_offset))  {
    phdr = wtap_phdr(wth);
	cur_time = secs_usecs(phdr->ts.tv_sec, phdr->ts.tv_usec);
	if(packet==0) {
	  start_time = cur_time;
	  stop_time = cur_time;
	}
	if (cur_time < start_time) {
	  start_time = cur_time;
	}
	if (cur_time > stop_time) {
	  stop_time = cur_time;
	}
	bytes+=phdr->len;
    packet++;
  }
  
  if (err != 0) {
    fprintf(stderr, "Error after reading %i packets\n", packet);
	exit(1);
  }

  /* File size */
  if (fstat(wtap_fd(wth), &cf_stat) < 0) {
    wtap_close(wth);
	return 1;
  }
  
  cf_info.filesize = cf_stat.st_size;
  
  /* File Type */
  cf_info.file_type = wtap_file_type(wth);
  
  /* # of packets */
  cf_info.packet_count = packet;
  
  /* File Times */
  cf_info.start_time = start_time;
  cf_info.stop_time = stop_time;
  cf_info.duration = stop_time-start_time;
	
  /* Number of packet bytes */
  cf_info.packet_bytes = bytes;
  
  /* Data rate per second */
  cf_info.data_rate = (double)bytes / (stop_time-start_time);
  
  /* Avg packet size */
  cf_info.packet_size = (double)bytes/packet;
  
  print_stats(&cf_info);

return 0;
}

static void usage(gboolean is_error)
{
  FILE *output;
  
  if (!is_error) {
    output = stdout;
	/* XXX - add capinfo header info here */
  }
  else {
    output = stderr;
  }


  fprintf(output, "Usage: capinfo [-t] [-c] [-s] [-d] [-u] [-a] [-e] [-y]\n");
  fprintf(output, "               [-i] [-z] [-h] <capfile>\n");
  fprintf(output, "  where\t-t display the capture type of <capfile>\n");
  fprintf(output, "       \t-c count the number of packets\n");
  fprintf(output, "       \t-s display the size of the file \n");
  fprintf(output, "       \t-d display the total length of all packets in the file\n");
  fprintf(output, "       \t   (in bytes)\n");
  fprintf(output, "       \t-u display the capture duration (in seconds) \n");
  fprintf(output, "       \t-a display the capture start time\n");
  fprintf(output, "       \t-e display the capture end time\n");
  fprintf(output, "       \t-y display average data rate (in bytes)\n");
  fprintf(output, "       \t-i display average data rate (in bits)\n");
  fprintf(output, "       \t-z display average packet size (in bytes)\n");
  fprintf(output, "       \t-h produces this help listing.\n");
  fprintf(output, "\n      \t    If no data flags are given, default is to display all statistics\n");
}

int main(int argc, char *argv[])
{
  wtap *wth;
  int err;
  gchar *err_info;
  extern char *optarg;
  extern int optind;
  int opt;
  int status = 0;

  /* Process the options first */

  while ((opt = getopt(argc, argv, "tcsduaeyizvh")) !=-1) {

    switch (opt) {

	case 't':
	  cap_file_type = TRUE;
	  break;

	case 'c':
	  cap_packet_count = TRUE;
	  break;

	case 's':
	  cap_file_size = TRUE;
	  break;

	case 'd':
	  cap_data_size = TRUE;
	  break;

	case 'u':
	  cap_duration = TRUE;
	  break;

	case 'a':
	  cap_start_time = TRUE;
	  break;

	case 'e':
	  cap_end_time = TRUE;
	  break;

	case 'y':
	  cap_data_rate_byte = TRUE;
	  break;

	case 'i':
	  cap_data_rate_bit = TRUE;
	  break;

	case 'z':
	  cap_packet_size = TRUE;
	  break;

    case 'h':
      usage(FALSE);
      exit(1);
      break;

    case '?':              /* Bad flag - print usage message */
      usage(TRUE);
      exit(1);
      break;

    }

  }

  if (optind < 2) {

    /* If no arguments were given, by default display all statistics */
    cap_file_type = TRUE;      
    cap_packet_count = TRUE;   
    cap_file_size = TRUE;      
    cap_data_size = TRUE;      
    cap_duration = TRUE;       
    cap_start_time = TRUE;
    cap_end_time = TRUE;

    cap_data_rate_byte = TRUE;
    cap_data_rate_bit = TRUE;
    cap_packet_size = TRUE;

  }
  
  if ((argc - optind) < 1) {
    usage(TRUE);
	exit(1);
  }
  
  wth = wtap_open_offline(argv[optind], &err, &err_info, FALSE);

  if (!wth) {
    fprintf(stderr, "editcap: Can't open %s: %s\n", argv[optind],
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

  status = process_cap_file(wth);
  
  wtap_close(wth);
  return status;
}

