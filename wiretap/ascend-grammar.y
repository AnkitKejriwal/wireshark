%{
/* ascend-grammar.y
 *
 * $Id: ascend-grammar.y,v 1.25 2004/01/06 20:05:39 guy Exp $
 *
 * Wiretap Library
 * Copyright (c) 1998 by Gilbert Ramirez <gram@alumni.rice.edu>
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
 *
 */

/*
   Example 'wandsess' output data:
   
RECV-iguana:241:(task: B02614C0, time: 1975432.85) 49 octets @ 8003BD94
  [0000]: FF 03 00 3D C0 06 CA 22 2F 45 00 00 28 6A 3B 40 
  [0010]: 00 3F 03 D7 37 CE 41 62 12 CF 00 FB 08 20 27 00 
  [0020]: 50 E4 08 DD D7 7C 4C 71 92 50 10 7D 78 67 C8 00 
  [0030]: 00 
XMIT-iguana:241:(task: B04E12C0, time: 1975432.85) 53 octets @ 8009EB16
  [0000]: FF 03 00 3D C0 09 1E 31 21 45 00 00 2C 2D BD 40 
  [0010]: 00 7A 06 D8 B1 CF 00 FB 08 CE 41 62 12 00 50 20 
  [0020]: 29 7C 4C 71 9C 9A 6A 93 A4 60 12 22 38 3F 10 00 
  [0030]: 00 02 04 05 B4 

    Example 'wdd' output data:

Date: 01/12/1990.  Time: 12:22:33
Cause an attempt to place call to 14082750382
WD_DIALOUT_DISP: chunk 2515EE type IP.
(task: 251790, time: 994953.28) 44 octets @ 2782B8
  [0000]: 00 C0 7B 71 45 6C 00 60 08 16 AA 51 08 00 45 00
  [0010]: 00 2C 66 1C 40 00 80 06 53 F6 AC 14 00 18 CC 47
  [0020]: C8 45 0A 31 00 50 3B D9 5B 75 00 00

    The following output comes from a MAX with Software 7.2.3:

RECV-187:(task: B050B480, time: 18042248.03) 100 octets @ 800012C0
  [0000]: FF 03 00 21 45 00 00 60 E3 49 00 00 7F 11 FD 7B
  [0010]: C0 A8 F7 05 8A C8 18 51 00 89 00 89 00 4C C7 C1
  [0020]: CC 8E 40 00 00 01 00 00 00 00 00 01 20 45 4A 45
  [0030]: 42 45 43 45 48 43 4E 46 43 46 41 43 41 43 41 43
  [0040]: 41 43 41 43 41 43 41 43 41 43 41 42 4E 00 00 20
  [0050]: 00 01 C0 0C 00 20 00 01 00 04 93 E0 00 06 60 00
  [0060]: C0 A8 F7 05
XMIT-187:(task: B0292CA0, time: 18042248.04) 60 octets @ 800AD576
  [0000]: FF 03 00 21 45 00 00 38 D7 EE 00 00 0F 01 11 2B
  [0010]: 0A FF FF FE C0 A8 F7 05 03 0D 33 D3 00 00 00 00
  [0020]: 45 00 00 60 E3 49 00 00 7E 11 FE 7B C0 A8 F7 05
  [0030]: 8A C8 18 51 00 89 00 89 00 4C C7 C1
RECV-187:(task: B0292CA0, time: 18042251.92) 16 octets @ 800018E8
  [0000]: FF 03 C0 21 09 01 00 0C DE 61 96 4B 00 30 94 92

  In TAOS 8.0, Lucent slightly changed the format as follows:

    Example 'wandisp' output data (TAOS 8.0.3): (same format is used 
    for 'wanopen' and 'wannext' command)

RECV-14: (task "idle task" at 0xb05e6e00, time: 1279.01) 29 octets @ 0x8000e0fc
  [0000]: ff 03 c0 21 01 01 00 19  01 04 05 f4 11 04 05 f4    ...!.... ........
  [0010]: 13 09 03 00 c0 7b 9a 9f  2d 17 04 10 00             .....{.. -....
XMIT-14: (task "idle task" at 0xb05e6e00, time: 1279.02) 38 octets @ 0x8007fd56
  [0000]: ff 03 c0 21 01 01 00 22  00 04 00 00 01 04 05 f4    ...!..." ........
  [0010]: 03 05 c2 23 05 11 04 05  f4 13 09 03 00 c0 7b 80    ...#.... ......{.
  [0020]: 7c ef 17 04 0e 00                                   |.....
XMIT-14: (task "idle task" at 0xb05e6e00, time: 1279.02) 29 octets @ 0x8007fa36
  [0000]: ff 03 c0 21 02 01 00 19  01 04 05 f4 11 04 05 f4    ...!.... ........
  [0010]: 13 09 03 00 c0 7b 9a 9f  2d 17 04 10 00             .....{.. -....

    Example 'wandsess' output data (TAOS 8.0.3): 

RECV-Max7:20: (task "_brouterControlTask" at 0xb094ac20, time: 1481.50) 20 octets @ 0x8000d198
  [0000]: ff 03 00 3d c0 00 00 04  80 fd 02 01 00 0a 11 06    ...=.... ........
  [0010]: 00 01 01 03                                         ....
XMIT-Max7:20: (task "_brouterControlTask" at 0xb094ac20, time: 1481.51) 26 octets @ 0x800806b6
  [0000]: ff 03 00 3d c0 00 00 00  80 21 01 01 00 10 02 06    ...=.... .!......
  [0010]: 00 2d 0f 01 03 06 89 64  03 08                      .-.....d ..
XMIT-Max7:20: (task "_brouterControlTask" at 0xb094ac20, time: 1481.51) 20 octets @ 0x8007f716
  [0000]: ff 03 00 3d c0 00 00 01  80 fd 01 01 00 0a 11 06    ...=.... ........
  [0010]: 00 01 01 03                                         ....

  The changes since TAOS 7.X are:

    1) White space is added before "(task".
    2) Task has a name, indicated by a subsequent string surrounded by a
       double-quote.
    3) Address expressed in hex number has a preceeding "0x".
    4) Hex numbers are in lower case.
    5) There is a character display corresponding to hex data in each line.

 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>

#include "wtap-int.h"
#include "buffer.h"
#include "ascend.h"
#include "ascend-int.h"

#define NO_USER "<none>"

extern int at_eof;

int yyparse(void);
void yyerror(char *);

unsigned int bcur = 0, bcount;
guint32 start_time, secs, usecs, caplen, wirelen;
ascend_pkthdr *header;
struct ascend_phdr *pseudo_header;
guint8 *pkt_data;

%}
 
%union {
gchar  *s;
guint32 d;
guint8  b;
}

%token <s> STRING KEYWORD WDD_DATE WDD_CHUNK COUNTER
%token <d> WDS_PREFIX DECNUM HEXNUM
%token <b> HEXBYTE

%type <s> string dataln datagroup
%type <d> wds_prefix decnum hexnum
%type <b> byte bytegroup

%%

data_packet:
  | wds_hdr datagroup
  | wds8_hdr datagroup
  | wdp7_hdr datagroup
  | wdp8_hdr datagroup
  | wdd_date wdd_hdr datagroup
  | wdd_hdr datagroup
;

wds_prefix: WDS_PREFIX;

string: STRING;

decnum: DECNUM;

hexnum: HEXNUM;

/* RECV-iguana:241:(task: B02614C0, time: 1975432.85) 49 octets @ 8003BD94 */
/*            1        2      3      4       5      6       7      8      9      10     11 */
wds_hdr: wds_prefix string decnum KEYWORD hexnum KEYWORD decnum decnum decnum KEYWORD HEXNUM {
  wirelen = $9;
  caplen = ($9 < ASCEND_MAX_PKT_LEN) ? $9 : ASCEND_MAX_PKT_LEN;
  /* If we don't have as many bytes of data as the octet count in
     the header, make the capture length the number of bytes we
     actually have. */
  if (bcount > 0 && bcount <= caplen)
    caplen = bcount;
  secs = $7;
  usecs = $8;
  if (pseudo_header != NULL) {
    /* pseudo_header->user is set in ascend-scanner.l */
    pseudo_header->type = $1;
    pseudo_header->sess = $3;
    pseudo_header->call_num[0] = '\0';
    pseudo_header->chunk = 0;
    pseudo_header->task = $5;
  }
  
  bcur = 0;
}
;

/* RECV-Max7:20: (task "_brouterControlTask" at 0xb094ac20, time: 1481.50) 20 octets @ 0x8000d198 */
/*                1       2       3     4       5       6      7       8      9      10     11     12      13 */
wds8_hdr: wds_prefix string decnum KEYWORD string KEYWORD hexnum KEYWORD decnum decnum decnum KEYWORD HEXNUM {
  wirelen = $11;
  caplen = ($11 < ASCEND_MAX_PKT_LEN) ? $11 : ASCEND_MAX_PKT_LEN;
  /* If we don't have as many bytes of data as the octet count in
     the header, make the capture length the number of bytes we
     actually have. */
  if (bcount > 0 && bcount <= caplen)
    caplen = bcount;
  secs = $9;
  usecs = $10;
  if (pseudo_header != NULL) {
    /* pseudo_header->user is set in ascend-scanner.l */
    pseudo_header->type = $1;
    pseudo_header->sess = $3;
    pseudo_header->call_num[0] = '\0';
    pseudo_header->chunk = 0;
    pseudo_header->task = $7;
  }
  
  bcur = 0;
}
;

/* RECV-187:(task: B050B480, time: 18042248.03) 100 octets @ 800012C0 */
/*            1        2       3      4       5       6      7      8      9      10    */
wdp7_hdr: wds_prefix decnum KEYWORD hexnum KEYWORD decnum decnum decnum KEYWORD HEXNUM {
  wirelen = $8;
  caplen = ($8 < ASCEND_MAX_PKT_LEN) ? $8 : ASCEND_MAX_PKT_LEN;
  /* If we don't have as many bytes of data as the octet count in
     the header, make the capture length the number of bytes we
     actually have. */
  if (bcount > 0 && bcount <= caplen)
    caplen = bcount;
  secs = $6;
  usecs = $7;
  if (pseudo_header != NULL) {
    /* pseudo_header->user is set in ascend-scanner.l */
    pseudo_header->type = $1;
    pseudo_header->sess = $2;
    pseudo_header->call_num[0] = '\0';
    pseudo_header->chunk = 0;
    pseudo_header->task = $4;
  }
  
  bcur = 0;
}
;

/* XMIT-44: (task "freedm_task" at 0xe051fd10, time: 6258.66) 29 octets @ 0x606d1f00 */
/*              1        2       3      4       5      6      7       8      9      10     11      12 */
wdp8_hdr: wds_prefix decnum KEYWORD string KEYWORD hexnum KEYWORD decnum decnum decnum KEYWORD HEXNUM {
  wirelen = $10;
  caplen = ($10 < ASCEND_MAX_PKT_LEN) ? $10 : ASCEND_MAX_PKT_LEN;
  /* If we don't have as many bytes of data as the octet count in
     the header, make the capture length the number of bytes we
     actually have. */
  if (bcount > 0 && bcount <= caplen)
    caplen = bcount;
  secs = $8;
  usecs = $9;
  if (pseudo_header != NULL) {
    /* pseudo_header->user is set in ascend-scanner.l */
    pseudo_header->type = $1;
    pseudo_header->sess = $2;
    pseudo_header->call_num[0] = '\0';
    pseudo_header->chunk = 0;
    pseudo_header->task = $6;
  }
  
  bcur = 0;
}
;

/*
Date: 01/12/1990.  Time: 12:22:33
Cause an attempt to place call to 14082750382
*/
/*           1        2      3      4      5       6      7      8      9      10*/
wdd_date: WDD_DATE decnum decnum decnum KEYWORD decnum decnum decnum KEYWORD string {
  /*
   * Supply the date/time value to the code above us; it will use the
   * first date/time value supplied as the capture start date/time.
   */
  struct tm wddt;

  wddt.tm_sec  = $8;
  wddt.tm_min  = $7;
  wddt.tm_hour = $6;
  wddt.tm_mday = $3;
  wddt.tm_mon  = $2 - 1;
  wddt.tm_year = ($4 > 1970) ? $4 - 1900 : 70;
  wddt.tm_isdst = -1;
  
  start_time = mktime(&wddt);
}
;
 
/*
WD_DIALOUT_DISP: chunk 2515EE type IP.
(task: 251790, time: 994953.28) 44 octets @ 2782B8
*/
/*           1        2      3       4       5      6       7      8      9      10     11*/
wdd_hdr: WDD_CHUNK hexnum KEYWORD KEYWORD hexnum KEYWORD decnum decnum decnum KEYWORD HEXNUM {
  wirelen = $9;
  caplen = ($9 < ASCEND_MAX_PKT_LEN) ? $9 : ASCEND_MAX_PKT_LEN;
  /* If we don't have as many bytes of data as the octet count in
     the header, make the capture length the number of bytes we
     actually have. */
  if (bcount > 0 && bcount <= caplen)
    caplen = bcount;
  secs = $7;
  usecs = $8;
  if (pseudo_header != NULL) {
    /* pseudo_header->call_num is set in ascend-scanner.l */
    pseudo_header->type = ASCEND_PFX_WDD;
    pseudo_header->user[0] = '\0';
    pseudo_header->sess = 0;
    pseudo_header->chunk = $2;
    pseudo_header->task = $5;
  }
  
  bcur = 0;
}
;
 
byte: HEXBYTE {
  if (bcur < caplen) {
    pkt_data[bcur] = $1;
    bcur++;
  }

  if (bcur >= caplen) {
    if (header != NULL) {
      header->start_time = start_time;
      header->secs = secs;
      header->usecs = usecs;
      header->caplen = caplen;
      header->len = wirelen;
    }
    YYACCEPT;
  }
} 
;

/* XXX  There must be a better way to do this... */
bytegroup: byte
  | byte byte
  | byte byte byte
  | byte byte byte byte
  | byte byte byte byte byte
  | byte byte byte byte byte byte
  | byte byte byte byte byte byte byte
  | byte byte byte byte byte byte byte byte
  | byte byte byte byte byte byte byte byte byte
  | byte byte byte byte byte byte byte byte byte byte
  | byte byte byte byte byte byte byte byte byte byte byte
  | byte byte byte byte byte byte byte byte byte byte byte byte
  | byte byte byte byte byte byte byte byte byte byte byte byte byte
  | byte byte byte byte byte byte byte byte byte byte byte byte byte byte
  | byte byte byte byte byte byte byte byte byte byte byte byte byte byte byte
  | byte byte byte byte byte byte byte byte byte byte byte byte byte byte byte byte
;

dataln: COUNTER bytegroup;

datagroup: dataln
  | dataln dataln
  | dataln dataln dataln
  | dataln dataln dataln dataln
  | dataln dataln dataln dataln dataln
  | dataln dataln dataln dataln dataln dataln
  | dataln dataln dataln dataln dataln dataln dataln
  | dataln dataln dataln dataln dataln dataln dataln dataln
;

%%

void
init_parse_ascend()
{
  bcur = 0;
  at_eof = 0;
  start_time = 0;	/* we haven't see a date/time yet */
}

/* Parse the capture file.  Return the offset of the next packet, or zero
   if there is none. */
int
parse_ascend(FILE_T fh, guint8 *pd, struct ascend_phdr *phdr,
		ascend_pkthdr *hdr, int len)
{
  /* yydebug = 1; */
 
  ascend_init_lexer(fh);
  pkt_data = pd;
  pseudo_header = phdr;
  header = hdr;
  bcount = len;
  
  /*
   * Not all packets in a "wdd" dump necessarily have a "Cause an
   * attempt to place call to" header (I presume this can happen if
   * there was a call in progress when the packet was sent or
   * received), so we won't necessarily have the phone number for
   * the packet.
   *
   * XXX - we could assume, in the sequential pass, that it's the
   * phone number from the last call, and remember that for use
   * when doing random access.
   */
  pseudo_header->call_num[0] = '\0';

  if (yyparse())
    return 0;
  else
    return 1;
}

void
yyerror (char *s _U_)
{
}
