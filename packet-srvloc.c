/* packet-srvloc.c
 * Routines for SRVLOC (Service Location Protocol) packet dissection
 * Copyright 1999, James Coe <jammer@cin.net>
 *
 * NOTE: This is Alpha software not all features have been verified yet.
 *       In particular I have not had an opportunity to see how it 
 *       responds to SRVLOC over TCP.
 *
 * $Id: packet-srvloc.c,v 1.5 2000/01/07 22:05:40 guy Exp $
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@zing.org>
 * Copyright 1998 Gerald Combs
 *
 * Service Location Protocol is RFC 2165
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
#include <stdlib.h>

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif

#ifdef NEED_SNPRINTF_H
# ifdef HAVE_STDARG_H
#  include <stdarg.h>
# else
#  include <varargs.h>
# endif
# include "snprintf.h"
#endif

#include <string.h>
#include <time.h>
#include <glib.h>
#include "packet.h"
#include "packet-ipv6.h"

int proto_srvloc = -1;
int hf_srvloc_version = -1;
int hf_srvloc_function = -1;
int hf_srvloc_flags = -1;
int hf_srvloc_error = -1;

static gint ett_srvloc = -1;
gint ett_srvloc_flags = -1;

/* Define function types */

#define SRVREQ		1
#define SRVRPLY		2
#define SRVREG		3
#define SRVDEREG	4
#define SRVACK		5
#define ATTRRQST	6
#define ATTRRPLY	7
#define DAADVERT	8
#define SRVTYPERQST	9
#define	SRVTYPERPLY	10

/* Create protocol header structure */

struct srvloc_hdr {
    guint8	version;
    guint8	function;
    guint16	length;
    guint8	flags;
    guint8	dialect;
    u_char	language[2];
    guint16	encoding;
    guint16	xid;
};

/* List to resolve function numbers to names */

static const value_string srvloc_functions[] = {
    { SRVREQ, "Service Request" }, 
    { SRVRPLY, "Service Reply" }, 
    { SRVREG, "Service Registration" }, 
    { SRVDEREG, "Service Deregister" }, 
    { SRVACK, "Service Acknowledge" }, 
    { ATTRRQST, "Attribute Request" }, 
    { ATTRRPLY, "Attribute Reply" }, 
    { DAADVERT, "DA Advertisement" }, 
    { SRVTYPERQST, "Service Type Request" }, 
    { SRVTYPERPLY, "Service Type Reply" }, 
};

/* List to resolve flag values to names */


/* Define flag masks */

#define FLAG_O		0x80
#define FLAG_M		0x40
#define FLAG_U		0x20
#define FLAG_A		0x10
#define FLAG_F		0x08

/* Define Error Codes */

#define SUCCESS		0
#define LANG_NOT_SPTD	1
#define PROT_PARSE_ERR	2
#define INVLD_REG	3
#define SCOPE_NOT_SPTD	4
#define CHRSET_NOT_UND	5
#define AUTH_ABSENT	6
#define AUTH_FAILED	7

/* List to resolve error codes to names */

static const value_string srvloc_errs[] = {
    { SUCCESS, "No Error" },
    { LANG_NOT_SPTD, "Language not supported" },
    { PROT_PARSE_ERR, "Protocol parse error" },
    { INVLD_REG, "Invalid registration" },
    { SCOPE_NOT_SPTD, "Scope not supported" },
    { CHRSET_NOT_UND, "Character set not understood" },
    { AUTH_ABSENT, "Authentication absent" },
    { AUTH_FAILED, "Authentication failed" },
};

void
dissect_authblk(const u_char *pd, int offset, frame_data *fd, proto_tree *tree)
{
    struct tm *stamp;
    time_t seconds;
    double floatsec;
    guint16 length;
    
    seconds = pntohl(&pd[offset]) - 2208988800ul;
    stamp = gmtime(&seconds);
    floatsec = stamp->tm_sec + pntohl(&pd[offset + 4]) / 4294967296.0;
    proto_tree_add_text(tree, offset, 8,
			"Timestamp: %04d-%02d-%02d %02d:%02d:%07.4f UTC",
			stamp->tm_year + 1900, stamp->tm_mon, stamp->tm_mday,
			stamp->tm_hour, stamp->tm_min, floatsec);
    proto_tree_add_text(tree, offset + 8, 2, "Block Structure Desciptor: %u",
			pntohs(&pd[offset + 8]));
    length = pntohs(&pd[offset + 10]);
    proto_tree_add_text(tree, offset + 10, 2, "Authenticator length: %u",
			length);
    offset += 12;
    proto_tree_add_text(tree, offset, length, "Authentication block: %s",
			format_text(&pd[offset], length));
    offset += length;
};

/* Packet dissection routine called by tcp & udp when port 427 detected */

void
dissect_srvloc(const u_char *pd, int offset, frame_data *fd, proto_tree *tree)
{
    proto_item *ti, *tf;
    proto_tree *srvloc_tree, *srvloc_flags;
    struct srvloc_hdr srvloc_hdr;
    int count;
    int length;
    
    if (check_col(fd, COL_PROTOCOL))
        col_add_str(fd, COL_PROTOCOL, "SRVLOC");
    
    if (check_col(fd, COL_INFO))
        col_add_str(fd, COL_INFO, val_to_str(pd[offset + 1], srvloc_functions, "Unknown Function (%d)"));
        
    if (tree) {
        ti = proto_tree_add_item(tree, proto_srvloc, offset, END_OF_FRAME, NULL);
        srvloc_tree = proto_item_add_subtree(ti, ett_srvloc);
    
        if ( END_OF_FRAME > sizeof(srvloc_hdr) ) {
            memcpy( &srvloc_hdr, &pd[offset], sizeof(srvloc_hdr) );
            srvloc_hdr.length = pntohs(&srvloc_hdr.length);
            srvloc_hdr.encoding = pntohs(&srvloc_hdr.encoding);
            srvloc_hdr.xid = pntohs(&srvloc_hdr.xid);
            proto_tree_add_item(srvloc_tree, hf_srvloc_version, offset, 1, srvloc_hdr.version);
            proto_tree_add_item(srvloc_tree, hf_srvloc_function, offset + 1, 1, srvloc_hdr.function);
            proto_tree_add_text(srvloc_tree, offset + 2, 2, "Length: %d",srvloc_hdr.length);
            tf = proto_tree_add_item(srvloc_tree, hf_srvloc_flags, offset + 4, 1, srvloc_hdr.flags);
            srvloc_flags = proto_item_add_subtree(tf, ett_srvloc_flags);
            proto_tree_add_text(srvloc_flags, offset + 4, 0, "Overflow                          %d... .xxx", (srvloc_hdr.flags & FLAG_O) >> 7 );
            proto_tree_add_text(srvloc_flags, offset + 4, 0, "Monolingual                       .%d.. .xxx", (srvloc_hdr.flags & FLAG_M) >> 6 ); 
            proto_tree_add_text(srvloc_flags, offset + 4, 0, "URL Authentication Present        ..%d. .xxx", (srvloc_hdr.flags & FLAG_U) >> 5 );
            proto_tree_add_text(srvloc_flags, offset + 4, 0, "Attribute Authentication Present  ...%d .xxx", (srvloc_hdr.flags & FLAG_A) >> 4 );
            proto_tree_add_text(srvloc_flags, offset + 4, 0, "Fresh Service Entry               .... %dxxx", (srvloc_hdr.flags & FLAG_F) >> 3 );
            proto_tree_add_text(srvloc_tree, offset + 5, 1, "Dialect: %d",srvloc_hdr.dialect); 
            proto_tree_add_text(srvloc_tree, offset + 6, 2, "Language: %s", format_text(srvloc_hdr.language,2));
            proto_tree_add_text(srvloc_tree, offset + 8, 2, "Encoding: %d", srvloc_hdr.encoding);
            proto_tree_add_text(srvloc_tree, offset + 10, 2, "Transaction ID: %d", srvloc_hdr.xid);
            offset += 12;
        } else {
        proto_tree_add_text(srvloc_tree, offset, END_OF_FRAME, "Invalid Packet: Length less than header.");
        };
        
        if (( srvloc_hdr.length - 12 ) == END_OF_FRAME ) {
            switch (srvloc_hdr.function) {
                case SRVREQ:
                    proto_tree_add_text(srvloc_tree, offset, 0, "Service Request");
                    length = pntohs(&pd[offset]);
                    proto_tree_add_text(srvloc_tree, offset, 2, "Previous Response List Length: %d", length);
                    offset += 2;
                    proto_tree_add_text(srvloc_tree, offset, length, "Previous Response List: %s", format_text(&pd[offset], length)); 
                    offset += length;
                    length = pntohs(&pd[offset]);
                    proto_tree_add_text(srvloc_tree, offset, 2, "Predicate length: %d", length);
                    offset += 2;
                    proto_tree_add_text(srvloc_tree, offset, length, "Predicate: %s", format_text(&pd[offset], length));
                    offset += length;
                break;
            
                case SRVRPLY:
                    proto_tree_add_text(srvloc_tree, offset, 0, "Service Reply");
                    proto_tree_add_item(srvloc_tree, hf_srvloc_error, offset, 2, pd[offset]);;
                    offset += 2;
                    proto_tree_add_text(srvloc_tree, offset, 2, "URL Count: %d", pntohs(&pd[offset]));
                    offset += 2;
                    for (count = pntohs(&pd[offset]) + 1; count > 0; count--, offset++) {
                        proto_tree_add_text(srvloc_tree, offset, 2, "URL lifetime: %d", pntohs(&pd[offset]));
                        offset += 2;
                        length = pntohs(&pd[offset]);
                        proto_tree_add_text(srvloc_tree, offset, 2, "URL length: %d", length);
                        offset += 2;
                        proto_tree_add_text(srvloc_tree, offset, length, "Service URL: %s", format_text(&pd[offset], length));
                        offset += length;
                        if ( (srvloc_hdr.flags & FLAG_U) == FLAG_U ) 
                            dissect_authblk(pd, offset, fd, srvloc_tree);
                    };
                break;

                case SRVREG:
                    proto_tree_add_text(srvloc_tree, offset, 0, "Service Registration");
                    proto_tree_add_text(srvloc_tree, offset, 2, "URL lifetime: %d", pntohs(&pd[offset]));
                    offset += 2;
                    length = pntohs(&pd[offset]);
                    proto_tree_add_text(srvloc_tree, offset, 2, "URL length: %d", length);
                    offset += 2;
                    proto_tree_add_text(srvloc_tree, offset, length, "Service URL: %s", format_text(&pd[offset], length));
                    offset += length;
                    if ( (srvloc_hdr.flags & FLAG_U) == FLAG_U ) 
                        dissect_authblk(pd, offset, fd, srvloc_tree);
                    length = pntohs(&pd[offset]);
                    proto_tree_add_text(srvloc_tree, offset, 2, "Attribute List length: %d", length);
                    offset += 2;
                    proto_tree_add_text(srvloc_tree, offset, length, "Attribute List: %s", format_text(&pd[offset], length));
                    offset += length;
                    if ( (srvloc_hdr.flags & FLAG_A) == FLAG_A ) 
                        dissect_authblk(pd, offset, fd, srvloc_tree);
                break;

                case SRVDEREG:
                    proto_tree_add_text(srvloc_tree, offset, 0, "Service Deregister");
                    length = pntohs(&pd[offset]);
                    proto_tree_add_text(srvloc_tree, offset, 2, "URL length: %d", length);
                    offset += 2;
                    proto_tree_add_text(srvloc_tree, offset, length, "Service URL: %s", format_text(&pd[offset], length));
                    offset += length;
                    if ( (srvloc_hdr.flags & FLAG_U) == FLAG_U ) 
                        dissect_authblk(pd, offset, fd, srvloc_tree);
                    length = pntohs(&pd[offset]);
                    proto_tree_add_text(srvloc_tree, offset, 2, "Attribute List length: %d", length);
                    offset += 2;
                    proto_tree_add_text(srvloc_tree, offset, length, "Attribute List: %s", format_text(&pd[offset], length));
                    offset += length;
                    if ( (srvloc_hdr.flags & FLAG_A) == FLAG_A ) 
                        dissect_authblk(pd, offset, fd, srvloc_tree);
                break;
            
                case SRVACK:
                    proto_tree_add_text(srvloc_tree, offset, 0, "Service Acknowledge");
                    proto_tree_add_item(srvloc_tree, hf_srvloc_error, offset, 2, pd[offset]);;
                    offset += 2;
                break;

                case ATTRRQST:
                    proto_tree_add_text(srvloc_tree, offset, 0, "Attribute Request");
                    length = pntohs(&pd[offset]);
                    proto_tree_add_text(srvloc_tree, offset, 2, "Previous Response List Length: %d", length);
                    offset += 2;
                    proto_tree_add_text(srvloc_tree, offset, length, "Previous Response List: %s", format_text(&pd[offset], length)); 
                    offset += length;
                    length = pntohs(&pd[offset]);
                    proto_tree_add_text(srvloc_tree, offset, 2, "URL length: %d", length);
                    offset += 2;
                    proto_tree_add_text(srvloc_tree, offset, length, "Service URL: %s", format_text(&pd[offset], length));
                    offset += length;
                    length = pntohs(&pd[offset]);
                    proto_tree_add_text(srvloc_tree, offset, 2, "Scope List Length: %d", length);
                    offset += 2;
                    proto_tree_add_text(srvloc_tree, offset, length, "Scope Response List: %s", format_text(&pd[offset], length)); 
                    offset += length;
                    length = pntohs(&pd[offset]);
                    proto_tree_add_text(srvloc_tree, offset, 2, "Attribute List length: %d", length);
                    offset += 2;
                    proto_tree_add_text(srvloc_tree, offset, length, "Attribute List: %s", format_text(&pd[offset], length));
                    offset += length;
                break;
            
                case ATTRRPLY:
                    proto_tree_add_text(srvloc_tree, offset, 0, "Attribute Reply");
                    proto_tree_add_item(srvloc_tree, hf_srvloc_error, offset, 2, pd[offset]);;
                    offset += 2;
                    length = pntohs(&pd[offset]);
                    proto_tree_add_text(srvloc_tree, offset, 2, "Attribute List length: %d", length);
                    offset += 2;
                    proto_tree_add_text(srvloc_tree, offset, length, "Attribute List: %s", format_text(&pd[offset], length));
                    offset += length;
                    if ( (srvloc_hdr.flags & FLAG_A) == FLAG_A ) 
                        dissect_authblk(pd, offset, fd, srvloc_tree);
                break;
            
                case DAADVERT:
                    proto_tree_add_text(srvloc_tree, offset, 0, "DA Advertisement");
                    proto_tree_add_item(srvloc_tree, hf_srvloc_error, offset, 2, pd[offset]);;
                    offset += 2;
                    length = pntohs(&pd[offset]);
                    proto_tree_add_text(srvloc_tree, offset, 2, "URL length: %d", length);
                    offset += 2;
                    proto_tree_add_text(srvloc_tree, offset, length, "Service URL: %s", format_text(&pd[offset], length));
                    offset += length;
                    length = pntohs(&pd[offset]);
                    proto_tree_add_text(srvloc_tree, offset, 2, "Scope List Length: %d", length);
                    offset += 2;
                    proto_tree_add_text(srvloc_tree, offset, length, "Scope Response List: %s", format_text(&pd[offset], length)); 
                    offset += length;
                break;

                case SRVTYPERQST:
                    proto_tree_add_text(srvloc_tree, offset, 0, "Service Type Request");
                    length = pntohs(&pd[offset]);
                    proto_tree_add_text(srvloc_tree, offset, 2, "Previous Response List Length: %d", length);
                    offset += 2;
                    proto_tree_add_text(srvloc_tree, offset, length, "Previous Response List: %s", format_text(&pd[offset], length)); 
                    offset += length;
                    length = pntohs(&pd[offset]);
                    proto_tree_add_text(srvloc_tree, offset, 2, "Naming Authority List length: %d", length);
                    offset += 2;
                    proto_tree_add_text(srvloc_tree, offset, length, "Naming Authority List: %s", format_text(&pd[offset], length)); 
                    offset += length;
                    length = pntohs(&pd[offset]);
                    proto_tree_add_text(srvloc_tree, offset, 2, "Scope List Length: %d", length);
                    offset += 2;
                    proto_tree_add_text(srvloc_tree, offset, length, "Scope Response List: %s", format_text(&pd[offset], length)); 
                    offset += length;
                break;

                case SRVTYPERPLY:
                    proto_tree_add_text(srvloc_tree, offset, 0, "Service Type Reply");
                    proto_tree_add_item(srvloc_tree, hf_srvloc_error, offset, 2, pd[offset]);;
                    offset += 2;
                    proto_tree_add_text(srvloc_tree, offset, 2, "Service Type Count: %d", pntohs(&pd[offset]));
                    offset += 2;
                    for (count = pntohs(&pd[offset]) + 1; count > 0; count--, offset++) {
                        length = pntohs(&pd[offset]);
                        proto_tree_add_text(srvloc_tree, offset, 2, "Service Type List length: %d", length);
                        offset += 2;
                        proto_tree_add_text(srvloc_tree, offset, length, "Service Type List: %s", format_text(&pd[offset], length));
                        offset += length;
                    };
                break;

                default:
                    proto_tree_add_text(srvloc_tree, offset, END_OF_FRAME, "Unknown Function Type");
            };
        } else { proto_tree_add_text(srvloc_tree, offset, END_OF_FRAME,"Invalid packet: Bad length value");
        };        
    };
};

/* Register protocol with Ethereal. */

void
proto_register_srvloc(void)
{
    static hf_register_info hf[] = {
        { &hf_srvloc_version,
            { "Version",           "srvloc.version",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            "" }
        },
      
        {&hf_srvloc_function,
            {"Function", "srvloc.function", 
            FT_UINT8, BASE_DEC, VALS(srvloc_functions), 0x0, 
            ""}
        },

        {&hf_srvloc_flags,
            {"Flags", "srvloc.flags", 
            FT_UINT8, BASE_HEX, NULL, 0x0, 
            ""}
        },
        
        {&hf_srvloc_error,
            {"Error Code", "srvloc.err",
            FT_UINT8, BASE_DEC, VALS(srvloc_errs), 0x0,
            ""}
        },
   };
                  
   static gint *ett[] = {
      &ett_srvloc,
      &ett_srvloc_flags,
   };

    proto_srvloc = proto_register_protocol("Service Location Protocol", "srvloc");
    proto_register_field_array(proto_srvloc, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));
};
