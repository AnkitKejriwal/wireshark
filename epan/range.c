/* range.c
 * Range routines
 *
 * $Id$
 *
 * Dick Gooris <gooris@lucent.com>
 * Ulf Lamping <ulf.lamping@web.de>
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

#include <string.h>
#include <ctype.h>

#include <glib.h>

#include <epan/frame_data.h>

#include <epan/range.h>
#include <stdio.h>

/* init the range struct */
void range_init(range_t *range) {

  range->nranges            = 0;
  range->ranges[range->nranges].low  = 0L;
  range->ranges[range->nranges].high = 0L;
}

/******************** Range Entry Parser *********************************/

/* Converts a range string to a fast comparable array of ranges.
 * The parameter 'es' points to the string to be converted.
 * The parameter 'max_value' specifies the maximum value in a
 * range.
 *
 * This function fills the array range->ranges containing low and high values
 * with the number of ranges being range->nranges. After having called this
 * function, the function value_is_in_range() determines whether a given
 * number is within the range or not. 
 *
 * In case of a single number, we make a range where low is equal to high. 
 * We take care on wrongly entered ranges; opposite order will be taken
 * care of.
 * 
 * The following syntax is accepted :
 *
 *   1-20,30-40     Range from 1 to 20, and packets 30 to 40
 *   -20,30         Range from 1 to 20, and packet 30
 *   20,30,40-      20, 30, and the range from 40 to the end
 *   20-10,30-25    Range from 10 to 20, and from 25 to 30
 *   -              All values
 */

convert_ret_t
range_convert_str(range_t *range, const gchar *es, guint32 max_value)
{
    const gchar   *p;
    char          *endp;
    gchar         c;
    guint         i;
    guint32       tmp;
    unsigned long val;

    /* Reset the number of ranges we are going to find */
    range->nranges = 0;
    range->ranges[range->nranges].low  = 0L;
    range->ranges[range->nranges].high = 0L;

    /* Process the ranges separately until we get a comma or end of string.
     *
     * We build a structure array called ranges of high and low values. After the
     * following loop, we have the nranges variable which tells how many ranges
     * were found. The number of individual ranges is limited to 'MaxRanges'
     */

    p = es;
    for (;;) {
       /* Skip white space. */
       while ((c = *p) == ' ' || c == '\t')
       	   p++;
       if (c == '\0')
           break;

       /* This must be a subrange. */
       if (range->nranges == MaxRange) {
       	   /* We've filled up the structure; no room for any more. */
           return CVT_TOO_MANY_SUBRANGES;
       }

       if (c == '-') {
           /* Subrange starts with 1. */
           range->ranges[range->nranges].low = 1;
       } else {
           /* Subrange starts with the specified number */
           val = strtol(p, &endp, 10);
           if (p == endp) {
               /* That wasn't a valid number. */
               return CVT_SYNTAX_ERROR;
           }
           p = endp;
           range->ranges[range->nranges].low = val;

           /* Skip white space. */
           while ((c = *p) == ' ' || c == '\t')
               p++;
       }

       if (c == '-') {
           /* There's a hyphen in the range.  Skip past it. */
           p++;

           /* Skip white space. */
           while ((c = *p) == ' ' || c == '\t')
               p++;

           if (c == ',' || c == '\0') {
               /*
                * End of subrange string; that means the subrange ends
                * with max_value.
                */
               range->ranges[range->nranges].high = max_value;
           } else {
               /* Subrange ends with the specified number. */
               val = strtol(p, &endp, 10);
               if (p == endp) {
                   /* That wasn't a valid number. */
                   return CVT_SYNTAX_ERROR;
               }
               p = endp;
               range->ranges[range->nranges].high = val;

               /* Skip white space. */
               while ((c = *p) == ' ' || c == '\t')
                   p++;
           }
       } else if (c == ',' || c == '\0') {
           /*
            * End of subrange string; that means there's no hyphen
            * in the subrange, so the start and the end are the same.
            */
           range->ranges[range->nranges].high =
               range->ranges[range->nranges].low;
       } else {
          /* Invalid character. */ 
          return CVT_SYNTAX_ERROR;
       }
       range->nranges++;

       if (c == ',') {
       	   /* Subrange is followed by a comma; skip it. */
           p++;
       }
    }

    /*  Now we are going through the low and high values, and check
     *  whether they are in a proper order. Low should be equal or lower
     *  than high. So, go through the loop and swap if needed.
     */
    for (i=0; i < range->nranges; i++) {
       if (range->ranges[i].low > range->ranges[i].high) {
          tmp = range->ranges[i].low;
          range->ranges[i].low  = range->ranges[i].high;
          range->ranges[i].high = tmp;
       }
    }

    /* In case we want to know what the result ranges are :
     *
     * for (i=0; i < range->nranges; i++) {
     *  printf("Function : range_convert_str L=%u \t H=%u\n",range->ranges[i].low,range->ranges[i].high);
     * }
     *
     */
     return CVT_NO_ERROR;
} /* range_convert_str */

/* This function returns TRUE if a given value is within one of the ranges
 * stored in the ranges array.
 */
gboolean value_is_in_range(range_t *range, guint32 val)
{
   guint i;

   for (i=0; i < range->nranges; i++) {
      if (val >= range->ranges[i].low && val <= range->ranges[i].high)
         return TRUE;
   }
   return(FALSE);
}

/* This function returns TRUE if the two given range_t's are equal.
 */
gboolean ranges_are_equal(range_t *a, range_t *b)
{
   guint i;

   if (a->nranges != b->nranges)
      return FALSE;

   for (i=0; i < a->nranges; i++) {
      if (a->ranges[i].low != b->ranges[i].low)
	 return FALSE;

      if (a->ranges[i].high != b->ranges[i].high)
	 return FALSE;
   }

   return TRUE;

}

/* This function calls the provided callback function for each value in
 * in the range.
 */
void
range_foreach(range_t *range, void (*callback)(guint32 val))
{
   guint32 i, j;

   for (i=0; i < range->nranges; i++) {
      for (j = range->ranges[i].low; j <= range->ranges[i].high; j++)
         callback(j);
   }
}

/* This function converts a range_t to a (caller-provided) string.  */
char *
range_convert_range(range_t *range, char *string)
{
   guint32 i, k;

   k = 0;
   string[k] = '\0';

   for (i=0; i < range->nranges; i++) {
      if (i != 0)
	 string[k++] = ',';

      k += sprintf(&string[k], "%d-%d", range->ranges[i].low,
		   range->ranges[i].high);
   }

   return(string);

}

#if 0
/* This is a debug function to check the range functionality */
static void value_is_in_range_check(range_t *range, guint32 val)
{

  /* Print the result for a given value */
  printf("Function : value_is_in_range_check Number %u\t",val);

  if (value_is_in_range(range, val)) {
     printf("is in range\n");
  } else {
     printf("is not in range\n");
  }
}
#endif
