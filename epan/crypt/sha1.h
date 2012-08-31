/*
 *  FIPS-180-1 compliant SHA-1 implementation
 *
 *  $Id$
 *
 *  Copyright (C) 2001-2003  Christophe Devine
 *  Copyright (C) 2012       Chris Elston, Katalix Systems Ltd <celston@katalix.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 *  Changed to use guint instead of uint 2004 by Anders Broman
 *	Original code found at http://www.cr0.net:8040/code/crypto/sha1/
 *  References: http://www.ietf.org/rfc/rfc3174.txt?number=3174
 *  
 *  2012-08-21 - C Elston - Split sha1_hmac function to allow incremental usage.
 */

#ifndef _SHA1_H
#define _SHA1_H

typedef struct
{
    guint32 total[2];
    guint32 state[5];
    guint8 buffer[64];
}
sha1_context;

void sha1_starts( sha1_context *ctx );
void sha1_update( sha1_context *ctx, const guint8 *input, guint32 length );
void sha1_finish( sha1_context *ctx, guint8 digest[20] );

typedef struct {
    sha1_context ctx;
    guint8 k_opad[64];
}
sha1_hmac_context;

void sha1_hmac_starts( sha1_hmac_context *hctx, const guint8 *key, guint32 keylen );
void sha1_hmac_update( sha1_hmac_context *hctx, const guint8 *buf, guint32 buflen );
void sha1_hmac_finish( sha1_hmac_context *hctx, guint8 digest[20] );
void sha1_hmac( const guint8 *key, guint32 keylen, const guint8 *buf, guint32 buflen,
                guint8 digest[20] );

#endif /* sha1.h */
