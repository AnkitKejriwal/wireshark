/* osi-utils.c
 * Routines for ISO/OSI network and transport protocol packet disassembly
 * Main entrance point and common functions
 *
 * $Id$
 * Laurent Deniel <laurent.deniel@free.fr>
 * Ralf Schneider <Ralf.Schneider@t-online.de>
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

#include <stdio.h>
#include <string.h>
#include <glib.h>

#include "osi-utils.h"

gchar *
print_nsap_net( const guint8 *ad, int length )
{
  static gchar  str[MAX_NSAP_LEN * 3 + 50]; /* reserve space for nice layout */
  gchar *cur;

  cur = str;
  print_nsap_net_buf( ad, length, cur );
  return( cur );
}

void
print_nsap_net_buf( const guint8 *ad, int length, gchar *buf )
{
  gchar *cur;

  /* to do : NSAP / NET decoding */

  if ( (length <= 0 ) || ( length > MAX_NSAP_LEN ) ) {
    sprintf( buf, "<Invalid length of NSAP>");
    return;
  }
  cur = buf;
  if ( ( length == RFC1237_NSAP_LEN ) || ( length == RFC1237_NSAP_LEN + 1 ) ) {
    print_area_buf( ad, RFC1237_FULLAREA_LEN, cur );
    cur += strlen( cur );
    print_system_id_buf( ad + RFC1237_FULLAREA_LEN, RFC1237_SYSTEMID_LEN, cur );
    cur += strlen( cur );
    cur += sprintf( cur, "[%02x]",
                    ad[ RFC1237_FULLAREA_LEN + RFC1237_SYSTEMID_LEN ] );
    if ( length == RFC1237_NSAP_LEN + 1 ) {
      cur += sprintf( cur, "-%02x", ad[ length -1 ] );
    }
  }
  else {    /* probably format as standard */
    print_area_buf( ad, length, buf );
  }
} /* print_nsap */

gchar *
print_system_id( const guint8 *ad, int length )
{
  static gchar  str[MAX_SYSTEMID_LEN * 3 + 5]; /* Don't trust exact matching */
  gchar        *cur;

  cur = str;
  print_system_id_buf(ad, length, cur );
  return( cur );
}

void
print_system_id_buf( const guint8 *ad, int length, gchar *buf )
{
  gchar        *cur;
  int           tmp;

  if ( ( length <= 0 ) || ( length > MAX_SYSTEMID_LEN ) ) {
    sprintf( buf, "<Invalid length of SYSTEM ID>");
    return;
  }

  cur = buf;
  if ( ( 6 == length ) || /* System-ID */
       ( 7 == length ) || /* LAN-ID */
       ( 8 == length )) { /* LSP-ID */
    cur += sprintf(cur, "%02x%02x.%02x%02x.%02x%02x", ad[0], ad[1],
                    ad[2], ad[3], ad[4], ad[5] );
    if ( ( 7 == length ) ||
         ( 8 == length )) {
        cur += sprintf( cur, ".%02x", ad[6] );
    }
    if ( 8 == length ) {
        cur += sprintf( cur, "-%02x", ad[7] );
    }
  }
  else {
    tmp = 0;
    while ( tmp < length / 4 ) { /* 16 / 4 == 4 > four Octets left to print */
      cur += sprintf( cur, "%02x", ad[tmp++] );
      cur += sprintf( cur, "%02x", ad[tmp++] );
      cur += sprintf( cur, "%02x", ad[tmp++] );
      cur += sprintf( cur, "%02x.", ad[tmp++] );
    }
    if ( 1 == tmp ) {   /* Special case for Designated IS */
      sprintf( --cur, ".%02x", ad[tmp] );
    }
    else {
      for ( ; tmp < length; ) {  /* print the rest without dot */
        cur += sprintf( cur, "%02x", ad[tmp++] );
      }
    }
  }
}

gchar *
print_area(const guint8 *ad, int length)
{
  static gchar  str[MAX_AREA_LEN * 3 + 20]; /* reserve space for nice layout */
  gchar *cur;

  cur = str;
  print_area_buf( ad, length, cur );
  return cur;
}

void
print_area_buf(const guint8 *ad, int length, gchar *buf)
{
  gchar *cur;
  int  tmp  = 0;

  /* to do : all real area decoding now: NET is assumed if id len is 1 more byte
   * and take away all these stupid resource consuming local statics
   */
  if (length <= 0 || length > MAX_AREA_LEN) {
    sprintf( buf, "<Invalid length of AREA>");
    return;
  }

  cur = buf;
  if ( (  ( NSAP_IDI_ISODCC          == *ad )
       || ( NSAP_IDI_GOSIP2          == *ad )
       )
       &&
       (  ( RFC1237_FULLAREA_LEN     ==  length )
       || ( RFC1237_FULLAREA_LEN + 1 ==  length )
       )
     ) {    /* AFI is good and length is long enough  */

    if ( length > RFC1237_FULLAREA_LEN + 1 ) {  /* Special Case Designated IS */
      sprintf( buf, "<Invalid length of AREA for DCC / GOSIP AFI>");
      return;
    }

    cur += sprintf( cur, "[%02x|%02x:%02x][%02x|%02x:%02x:%02x|%02x:%02x]",
                    ad[0], ad[1], ad[2], ad[3], ad[4],
                    ad[5], ad[6], ad[7], ad[8] );
    cur += sprintf( cur, "[%02x:%02x|%02x:%02x]",
                    ad[9], ad[10],  ad[11], ad[12] );
    if ( RFC1237_FULLAREA_LEN + 1 == length )
      sprintf( cur, "-[%02x]", ad[20] );
  }
  else { /* print standard format */
    if ( length == RFC1237_AREA_LEN ) {
      sprintf( buf, "%02x.%02x%02x", ad[0], ad[1], ad[2] );
      return;
    }
    if ( 4 < length ) {
      while ( tmp < length / 4 ) {      /* 16/4==4 > four Octets left to print */
        cur += sprintf( cur, "%02x", ad[tmp++] );
        cur += sprintf( cur, "%02x", ad[tmp++] );
        cur += sprintf( cur, "%02x", ad[tmp++] );
        cur += sprintf( cur, "%02x.", ad[tmp++] );
      }
      if ( 1 == tmp ) {                     /* Special case for Designated IS */
        sprintf( --cur, "-%02x", ad[tmp] );
      }
      else {
        for ( ; tmp < length; ) {  /* print the rest without dot */
          cur += sprintf( cur, "%02x", ad[tmp++] );
        }
      }
    }
  }
} /* print_area_buf */

