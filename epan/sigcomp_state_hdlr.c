/* sigcomp_state_hdlr.c
 * Routines making up the State handler of the Univerasl Decompressor Virtual Machine (UDVM) 
 * used for Signaling Compression (SigComp) dissection.
 * Copyright 2004, Anders Broman <anders.broman@ericsson.com>
 *
 * $Id: udvm.c 11445 2004-07-20 19:04:48Z etxrab $
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * References:
 * The Session Initiation Protocol (SIP) and Session Description Protocol
 *    (SDP) Static Dictionary for Signaling Compression (SigComp)
 * http://www.ietf.org/rfc/rfc3485.txt?number=3485 
 *
 * http://www.ietf.org/rfc/rfc3320.txt?number=3320
 * http://www.ietf.org/rfc/rfc3321.txt?number=3321
 * Useful links :
 * http://www.ietf.org/internet-drafts/draft-ietf-rohc-sigcomp-impl-guide-03.txt
 * http://www.ietf.org/internet-drafts/draft-ietf-rohc-sigcomp-sip-01.txt
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <glib.h>

#ifdef NEED_SNPRINTF_H
# include "snprintf.h"
#endif

#include "packet.h"
#include "sigcomp_state_hdlr.h"
/*
 * Defenitions for:
 * The Session Initiation Protocol (SIP) and Session Description Protocol
 *    (SDP) Static Dictionary for Signaling Compression (SigComp)
 * http://www.ietf.org/rfc/rfc3485.txt?number=3485 
 */
guint16 sip_sdp_state_length = 0x12e4;

static const guint8 sip_sdp_state_identifier[20] =
{
   /* -0000, */  0xfb, 0xe5, 0x07, 0xdf, 0xe5, 0xe6, 0xaa, 0x5a, 0xf2, 0xab, 0xb9, 0x14, 0xce, 0xaa, 0x05, 0xf9,
   /* -0010, */  0x9c, 0xe6, 0x1b, 0xa5
};

static const guint8 sip_sdp_static_dictionaty_for_sigcomp[0x12e4] =
{
				 
   /* -0000, */  0x0d, 0x0a, 0x52, 0x65, 0x6a, 0x65, 0x63, 0x74, 0x2d, 0x43, 0x6f, 0x6e, 0x74, 0x61, 0x63, 0x74, 
   /* -0010, */  0x3a, 0x20, 0x0d, 0x0a, 0x45, 0x72, 0x72, 0x6f, 0x72, 0x2d, 0x49, 0x6e, 0x66, 0x6f, 0x3a, 0x20,        
   /* -0020, */  0x0d, 0x0a, 0x54, 0x69, 0x6d, 0x65, 0x73, 0x74, 0x61, 0x6d, 0x70, 0x3a, 0x20, 0x0d, 0x0a, 0x43, 
   /* -0030, */  0x61, 0x6c, 0x6c, 0x2d, 0x49, 0x6e, 0x66, 0x6f, 0x3a, 0x20, 0x0d, 0x0a, 0x52, 0x65, 0x70, 0x6c, 
   /* -0040, */  0x79, 0x2d, 0x54, 0x6f, 0x3a, 0x20, 0x0d, 0x0a, 0x57, 0x61, 0x72, 0x6e, 0x69, 0x6e, 0x67, 0x3a, 
   /* -0050, */  0x20, 0x0d, 0x0a, 0x53, 0x75, 0x62, 0x6a, 0x65, 0x63, 0x74, 0x3a, 0x20, 0x3b, 0x68, 0x61, 0x6e,
   /* -0060, */  0x64, 0x6c, 0x69, 0x6e, 0x67, 0x3d, 0x69, 0x6d, 0x61, 0x67, 0x65, 0x3b, 0x70, 0x75, 0x72, 0x70,        
   /* -0070, */  0x6f, 0x73, 0x65, 0x3d, 0x3b, 0x63, 0x61, 0x75, 0x73, 0x65, 0x3d, 0x3b, 0x74, 0x65, 0x78, 0x74, 
   /* -0080, */  0x3d, 0x63, 0x61, 0x72, 0x64, 0x33, 0x30, 0x30, 0x20, 0x4d, 0x75, 0x6c, 0x74, 0x69, 0x70, 0x6c, 
   /* -0090, */  0x65, 0x20, 0x43, 0x68, 0x6f, 0x69, 0x63, 0x65, 0x73, 0x6d, 0x69, 0x6d, 0x65, 0x73, 0x73, 0x61, 
   /* -00A0, */  0x67, 0x65, 0x2f, 0x73, 0x69, 0x70, 0x66, 0x72, 0x61, 0x67, 0x34, 0x30, 0x37, 0x20, 0x50, 0x72, 
   /* -00B0, */  0x6f, 0x78, 0x79, 0x20, 0x41, 0x75, 0x74, 0x68, 0x65, 0x6e, 0x74, 0x69, 0x63, 0x61, 0x74, 0x69, 
   /* -00C0, */  0x6f, 0x6e, 0x20, 0x52, 0x65, 0x71, 0x75, 0x69, 0x72, 0x65, 0x64, 0x69, 0x67, 0x65, 0x73, 0x74, 
   /* -00D0, */  0x2d, 0x69, 0x6e, 0x74, 0x65, 0x67, 0x72, 0x69, 0x74, 0x79, 0x34, 0x38, 0x34, 0x20, 0x41, 0x64, 
   /* -00E0, */  0x64, 0x72, 0x65, 0x73, 0x73, 0x20, 0x49, 0x6e, 0x63, 0x6f, 0x6d, 0x70, 0x6c, 0x65, 0x74, 0x65, 
   /* -00F0, */  0x6c, 0x65, 0x70, 0x68, 0x6f, 0x6e, 0x65, 0x2d, 0x65, 0x76, 0x65, 0x6e, 0x74, 0x73, 0x34, 0x39, 
   /* -0100, */  0x34, 0x20, 0x53, 0x65, 0x63, 0x75, 0x72, 0x69, 0x74, 0x79, 0x20, 0x41, 0x67, 0x72, 0x65, 0x65, 
   /* -0110, */  0x6d, 0x65, 0x6e, 0x74, 0x20, 0x52, 0x65, 0x71, 0x75, 0x69, 0x72, 0x65, 0x64, 0x65, 0x61, 0x63, 
   /* -0120, */  0x74, 0x69, 0x76, 0x61, 0x74, 0x65, 0x64, 0x34, 0x38, 0x31, 0x20, 0x43, 0x61, 0x6c, 0x6c, 0x2f, 
   /* -0130, */  0x54, 0x72, 0x61, 0x6e, 0x73, 0x61, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x44, 0x6f, 0x65, 0x73, 
   /* -0140, */  0x20, 0x4e, 0x6f, 0x74, 0x20, 0x45, 0x78, 0x69, 0x73, 0x74, 0x61, 0x6c, 0x65, 0x3d, 0x35, 0x30,       
   /* -0150, */  0x30, 0x20, 0x53, 0x65, 0x72, 0x76, 0x65, 0x72, 0x20, 0x49, 0x6e, 0x74, 0x65, 0x72, 0x6e, 0x61,
   /* -0160, */  0x6c, 0x20, 0x45, 0x72, 0x72, 0x6f, 0x72, 0x6f, 0x62, 0x75, 0x73, 0x74, 0x2d, 0x73, 0x6f, 0x72, 
   /* -0170, */  0x74, 0x69, 0x6e, 0x67, 0x3d, 0x34, 0x31, 0x36, 0x20, 0x55, 0x6e, 0x73, 0x75, 0x70, 0x70, 0x6f,
   /* -0180, */  0x72, 0x74, 0x65, 0x64, 0x20, 0x55, 0x52, 0x49, 0x20, 0x53, 0x63, 0x68, 0x65, 0x6d, 0x65, 0x72,
   /* -0190, */  0x67, 0x65, 0x6e, 0x63, 0x79, 0x34, 0x31, 0x35, 0x20, 0x55, 0x6e, 0x73, 0x75, 0x70, 0x70, 0x6f, 
   /* -01A0, */  0x72, 0x74, 0x65, 0x64, 0x20, 0x4d, 0x65, 0x64, 0x69, 0x61, 0x20, 0x54, 0x79, 0x70, 0x65, 0x6e, 
   /* -01B0, */  0x64, 0x69, 0x6e, 0x67, 0x34, 0x38, 0x38, 0x20, 0x4e, 0x6f, 0x74, 0x20, 0x41, 0x63, 0x63, 0x65, 
   /* -01C0, */  0x70, 0x74, 0x61, 0x62, 0x6c, 0x65, 0x20, 0x48, 0x65, 0x72, 0x65, 0x6a, 0x65, 0x63, 0x74, 0x65, 
   /* -01D0, */  0x64, 0x34, 0x32, 0x33, 0x20, 0x49, 0x6e, 0x74, 0x65, 0x72, 0x76, 0x61, 0x6c, 0x20, 0x54, 0x6f, 
   /* -01E0, */  0x6f, 0x20, 0x42, 0x72, 0x69, 0x65, 0x66, 0x72, 0x6f, 0x6d, 0x2d, 0x74, 0x61, 0x67, 0x51, 0x2e, 
   /* -01F0, */  0x38, 0x35, 0x30, 0x35, 0x20, 0x56, 0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e, 0x20, 0x4e, 0x6f, 0x74, 
   /* -0200, */  0x20, 0x53, 0x75, 0x70, 0x70, 0x6f, 0x72, 0x74, 0x65, 0x64, 0x34, 0x30, 0x33, 0x20, 0x46, 0x6f,
   /* -0210, */  0x72, 0x62, 0x69, 0x64, 0x64, 0x65, 0x6e, 0x6f, 0x6e, 0x2d, 0x75, 0x72, 0x67, 0x65, 0x6e, 0x74, 
   /* -0220, */  0x34, 0x32, 0x39, 0x20, 0x50, 0x72, 0x6f, 0x76, 0x69, 0x64, 0x65, 0x20, 0x52, 0x65, 0x66, 0x65, 
   /* -0230, */  0x72, 0x72, 0x6f, 0x72, 0x20, 0x49, 0x64, 0x65, 0x6e, 0x74, 0x69, 0x74, 0x79, 0x34, 0x32, 0x30,         
   /* -0240, */  0x20, 0x42, 0x61, 0x64, 0x20, 0x45, 0x78, 0x74, 0x65, 0x6e, 0x73, 0x69, 0x6f, 0x6e, 0x6f, 0x72,
   /* -0250, */  0x65, 0x73, 0x6f, 0x75, 0x72, 0x63, 0x65, 0x0d, 0x0a, 0x61, 0x3d, 0x6b, 0x65, 0x79, 0x2d, 0x6d, 
   /* -0260, */  0x67, 0x6d, 0x74, 0x3a, 0x6d, 0x69, 0x6b, 0x65, 0x79, 0x4f, 0x50, 0x54, 0x49, 0x4f, 0x4e, 0x53, 
   /* -0270, */  0x20, 0x4c, 0x61, 0x6e, 0x67, 0x75, 0x61, 0x67, 0x65, 0x3a, 0x20, 0x35, 0x30, 0x34, 0x20, 0x53,
   /* -0280, */  0x65, 0x72, 0x76, 0x65, 0x72, 0x20, 0x54, 0x69, 0x6d, 0x65, 0x2d, 0x6f, 0x75, 0x74, 0x6f, 0x2d, 
   /* -0290, */  0x74, 0x61, 0x67, 0x0d, 0x0a, 0x41, 0x75, 0x74, 0x68, 0x65, 0x6e, 0x74, 0x69, 0x63, 0x61, 0x74, 
   /* -02A0, */  0x69, 0x6f, 0x6e, 0x2d, 0x49, 0x6e, 0x66, 0x6f, 0x3a, 0x20, 0x44, 0x65, 0x63, 0x20, 0x33, 0x38, 
   /* -02B0, */  0x30, 0x20, 0x41, 0x6c, 0x74, 0x65, 0x72, 0x6e, 0x61, 0x74, 0x69, 0x76, 0x65, 0x20, 0x53, 0x65, 
   /* -02C0, */  0x72, 0x76, 0x69, 0x63, 0x65, 0x35, 0x30, 0x33, 0x20, 0x53, 0x65, 0x72, 0x76, 0x69, 0x63, 0x65, 
   /* -02D0, */  0x20, 0x55, 0x6e, 0x61, 0x76, 0x61, 0x69, 0x6c, 0x61, 0x62, 0x6c, 0x65, 0x34, 0x32, 0x31, 0x20,       
   /* -02E0, */  0x45, 0x78, 0x74, 0x65, 0x6e, 0x73, 0x69, 0x6f, 0x6e, 0x20, 0x52, 0x65, 0x71, 0x75, 0x69, 0x72, 
   /* -02F0, */  0x65, 0x64, 0x34, 0x30, 0x35, 0x20, 0x4d, 0x65, 0x74, 0x68, 0x6f, 0x64, 0x20, 0x4e, 0x6f, 0x74, 
   /* -0300, */  0x20, 0x41, 0x6c, 0x6c, 0x6f, 0x77, 0x65, 0x64, 0x34, 0x38, 0x37, 0x20, 0x52, 0x65, 0x71, 0x75,
   /* -0310, */  0x65, 0x73, 0x74, 0x20, 0x54, 0x65, 0x72, 0x6d, 0x69, 0x6e, 0x61, 0x74, 0x65, 0x64, 0x61, 0x75, 
   /* -0320, */  0x74, 0x68, 0x2d, 0x69, 0x6e, 0x74, 0x65, 0x72, 0x6c, 0x65, 0x61, 0x76, 0x69, 0x6e, 0x67, 0x3d, 
   /* -0330, */  0x0d, 0x0a, 0x6d, 0x3d, 0x61, 0x70, 0x70, 0x6c, 0x69, 0x63, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x20,        
   /* -0340, */  0x41, 0x75, 0x67, 0x20, 0x35, 0x31, 0x33, 0x20, 0x4d, 0x65, 0x73, 0x73, 0x61, 0x67, 0x65, 0x20,        
   /* -0350, */  0x54, 0x6f, 0x6f, 0x20, 0x4c, 0x61, 0x72, 0x67, 0x65, 0x36, 0x38, 0x37, 0x20, 0x44, 0x69, 0x61, 
   /* -0360, */  0x6c, 0x6f, 0x67, 0x20, 0x54, 0x65, 0x72, 0x6d, 0x69, 0x6e, 0x61, 0x74, 0x65, 0x64, 0x33, 0x30,         
   /* -0370, */  0x32, 0x20, 0x4d, 0x6f, 0x76, 0x65, 0x64, 0x20, 0x54, 0x65, 0x6d, 0x70, 0x6f, 0x72, 0x61, 0x72,
   /* -0380, */  0x69, 0x6c, 0x79, 0x33, 0x30, 0x31, 0x20, 0x4d, 0x6f, 0x76, 0x65, 0x64, 0x20, 0x50, 0x65, 0x72, 
   /* -0390, */  0x6d, 0x61, 0x6e, 0x65, 0x6e, 0x74, 0x6c, 0x79, 0x6d, 0x75, 0x6c, 0x74, 0x69, 0x70, 0x61, 0x72, 
   /* -03A0, */  0x74, 0x2f, 0x73, 0x69, 0x67, 0x6e, 0x65, 0x64, 0x0d, 0x0a, 0x52, 0x65, 0x74, 0x72, 0x79, 0x2d, 
   /* -03B0, */  0x41, 0x66, 0x74, 0x65, 0x72, 0x3a, 0x20, 0x47, 0x4d, 0x54, 0x68, 0x75, 0x2c, 0x20, 0x34, 0x30,        
   /* -03C0, */  0x32, 0x20, 0x50, 0x61, 0x79, 0x6d, 0x65, 0x6e, 0x74, 0x20, 0x52, 0x65, 0x71, 0x75, 0x69, 0x72, 
   /* -03D0, */  0x65, 0x64, 0x0d, 0x0a, 0x61, 0x3d, 0x6f, 0x72, 0x69, 0x65, 0x6e, 0x74, 0x3a, 0x6c, 0x61, 0x6e, 
   /* -03E0, */  0x64, 0x73, 0x63, 0x61, 0x70, 0x65, 0x34, 0x30, 0x30, 0x20, 0x42, 0x61, 0x64, 0x20, 0x52, 0x65, 
   /* -03F0, */  0x71, 0x75, 0x65, 0x73, 0x74, 0x72, 0x75, 0x65, 0x34, 0x39, 0x31, 0x20, 0x52, 0x65, 0x71, 0x75, 
   /* -0400, */  0x65, 0x73, 0x74, 0x20, 0x50, 0x65, 0x6e, 0x64, 0x69, 0x6e, 0x67, 0x35, 0x30, 0x31, 0x20, 0x4e, 
   /* -0410, */  0x6f, 0x74, 0x20, 0x49, 0x6d, 0x70, 0x6c, 0x65, 0x6d, 0x65, 0x6e, 0x74, 0x65, 0x64, 0x34, 0x30,        
   /* -0420, */  0x36, 0x20, 0x4e, 0x6f, 0x74, 0x20, 0x41, 0x63, 0x63, 0x65, 0x70, 0x74, 0x61, 0x62, 0x6c, 0x65, 
   /* -0430, */  0x36, 0x30, 0x36, 0x20, 0x4e, 0x6f, 0x74, 0x20, 0x41, 0x63, 0x63, 0x65, 0x70, 0x74, 0x61, 0x62, 
   /* -0440, */  0x6c, 0x65, 0x0d, 0x0a, 0x61, 0x3d, 0x74, 0x79, 0x70, 0x65, 0x3a, 0x62, 0x72, 0x6f, 0x61, 0x64, 
   /* -0450, */  0x63, 0x61, 0x73, 0x74, 0x6f, 0x6e, 0x65, 0x34, 0x39, 0x33, 0x20, 0x55, 0x6e, 0x64, 0x65, 0x63, 
   /* -0460, */  0x69, 0x70, 0x68, 0x65, 0x72, 0x61, 0x62, 0x6c, 0x65, 0x0d, 0x0a, 0x4d, 0x49, 0x4d, 0x45, 0x2d, 
   /* -0470, */  0x56, 0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e, 0x3a, 0x20, 0x4d, 0x61, 0x79, 0x20, 0x34, 0x38, 0x32, 
   /* -0480, */  0x20, 0x4c, 0x6f, 0x6f, 0x70, 0x20, 0x44, 0x65, 0x74, 0x65, 0x63, 0x74, 0x65, 0x64, 0x0d, 0x0a,
   /* -0490, */  0x4f, 0x72, 0x67, 0x61, 0x6e, 0x69, 0x7a, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x3a, 0x20, 0x4a, 0x75, 
   /* -04A0, */  0x6e, 0x20, 0x6d, 0x6f, 0x64, 0x65, 0x2d, 0x63, 0x68, 0x61, 0x6e, 0x67, 0x65, 0x2d, 0x6e, 0x65, 
   /* -04B0, */  0x69, 0x67, 0x68, 0x62, 0x6f, 0x72, 0x3d, 0x63, 0x72, 0x69, 0x74, 0x69, 0x63, 0x61, 0x6c, 0x65, 
   /* -04C0, */  0x72, 0x74, 0x63, 0x70, 0x2d, 0x66, 0x62, 0x34, 0x38, 0x39, 0x20, 0x42, 0x61, 0x64, 0x20, 0x45, 
   /* -04D0, */  0x76, 0x65, 0x6e, 0x74, 0x6c, 0x73, 0x0d, 0x0a, 0x55, 0x6e, 0x73, 0x75, 0x70, 0x70, 0x6f, 0x72, 
   /* -04E0, */  0x74, 0x65, 0x64, 0x3a, 0x20, 0x4a, 0x61, 0x6e, 0x20, 0x35, 0x30, 0x32, 0x20, 0x42, 0x61, 0x64, 
   /* -04F0, */  0x20, 0x47, 0x61, 0x74, 0x65, 0x77, 0x61, 0x79, 0x6d, 0x6f, 0x64, 0x65, 0x2d, 0x63, 0x68, 0x61, 
   /* -0500, */  0x6e, 0x67, 0x65, 0x2d, 0x70, 0x65, 0x72, 0x69, 0x6f, 0x64, 0x3d, 0x0d, 0x0a, 0x61, 0x3d, 0x6f, 
   /* -0510, */  0x72, 0x69, 0x65, 0x6e, 0x74, 0x3a, 0x73, 0x65, 0x61, 0x73, 0x63, 0x61, 0x70, 0x65, 0x0d, 0x0a, 
   /* -0520, */  0x61, 0x3d, 0x74, 0x79, 0x70, 0x65, 0x3a, 0x6d, 0x6f, 0x64, 0x65, 0x72, 0x61, 0x74, 0x65, 0x64, 
   /* -0530, */  0x34, 0x30, 0x34, 0x20, 0x4e, 0x6f, 0x74, 0x20, 0x46, 0x6f, 0x75, 0x6e, 0x64, 0x33, 0x30, 0x35, 
   /* -0540, */  0x20, 0x55, 0x73, 0x65, 0x20, 0x50, 0x72, 0x6f, 0x78, 0x79, 0x0d, 0x0a, 0x61, 0x3d, 0x74, 0x79,
   /* -0550, */  0x70, 0x65, 0x3a, 0x72, 0x65, 0x63, 0x76, 0x6f, 0x6e, 0x6c, 0x79, 0x0d, 0x0a, 0x61, 0x3d, 0x74, 
   /* -0560, */  0x79, 0x70, 0x65, 0x3a, 0x6d, 0x65, 0x65, 0x74, 0x69, 0x6e, 0x67, 0x0d, 0x0a, 0x6b, 0x3d, 0x70,       
   /* -0570, */  0x72, 0x6f, 0x6d, 0x70, 0x74, 0x3a, 0x0d, 0x0a, 0x52, 0x65, 0x66, 0x65, 0x72, 0x72, 0x65, 0x64, 
   /* -0580, */  0x2d, 0x42, 0x79, 0x3a, 0x20, 0x0d, 0x0a, 0x49, 0x6e, 0x2d, 0x52, 0x65, 0x70, 0x6c, 0x79, 0x2d, 
   /* -0590, */  0x54, 0x6f, 0x3a, 0x20, 0x54, 0x52, 0x55, 0x45, 0x6e, 0x63, 0x6f, 0x64, 0x69, 0x6e, 0x67, 0x3a, 
   /* -05A0, */  0x20, 0x31, 0x38, 0x32, 0x20, 0x51, 0x75, 0x65, 0x75, 0x65, 0x64, 0x41, 0x75, 0x74, 0x68, 0x65, 
   /* -05B0, */  0x6e, 0x74, 0x69, 0x63, 0x61, 0x74, 0x65, 0x3a, 0x20, 0x0d, 0x0a, 0x55, 0x73, 0x65, 0x72, 0x2d, 
   /* -05C0, */  0x41, 0x67, 0x65, 0x6e, 0x74, 0x3a, 0x20, 0x0d, 0x0a, 0x61, 0x3d, 0x66, 0x72, 0x61, 0x6d, 0x65, 
   /* -05D0, */  0x72, 0x61, 0x74, 0x65, 0x3a, 0x0d, 0x0a, 0x41, 0x6c, 0x65, 0x72, 0x74, 0x2d, 0x49, 0x6e, 0x66, 
   /* -05E0, */  0x6f, 0x3a, 0x20, 0x43, 0x41, 0x4e, 0x43, 0x45, 0x4c, 0x20, 0x0d, 0x0a, 0x61, 0x3d, 0x6d, 0x61, 
   /* -05F0, */  0x78, 0x70, 0x74, 0x69, 0x6d, 0x65, 0x3a, 0x3b, 0x72, 0x65, 0x74, 0x72, 0x79, 0x2d, 0x61, 0x66, 
   /* -0600, */  0x74, 0x65, 0x72, 0x3d, 0x75, 0x61, 0x63, 0x68, 0x61, 0x6e, 0x6e, 0x65, 0x6c, 0x73, 0x3d, 0x34, 
   /* -0610, */  0x31, 0x30, 0x20, 0x47, 0x6f, 0x6e, 0x65, 0x0d, 0x0a, 0x52, 0x65, 0x66, 0x65, 0x72, 0x2d, 0x54, 
   /* -0620, */  0x6f, 0x3a, 0x20, 0x0d, 0x0a, 0x50, 0x72, 0x69, 0x6f, 0x72, 0x69, 0x74, 0x79, 0x3a, 0x20, 0x0d, 
   /* -0630, */  0x0a, 0x6d, 0x3d, 0x63, 0x6f, 0x6e, 0x74, 0x72, 0x6f, 0x6c, 0x20, 0x0d, 0x0a, 0x61, 0x3d, 0x71, 
   /* -0640, */  0x75, 0x61, 0x6c, 0x69, 0x74, 0x79, 0x3a, 0x0d, 0x0a, 0x61, 0x3d, 0x73, 0x64, 0x70, 0x6c, 0x61, 
   /* -0650, */  0x6e, 0x67, 0x3a, 0x0d, 0x0a, 0x61, 0x3d, 0x63, 0x68, 0x61, 0x72, 0x73, 0x65, 0x74, 0x3a, 0x0d, 
   /* -0660, */  0x0a, 0x52, 0x65, 0x70, 0x6c, 0x61, 0x63, 0x65, 0x73, 0x3a, 0x20, 0x52, 0x45, 0x46, 0x45, 0x52, 
   /* -0670, */  0x20, 0x69, 0x70, 0x73, 0x65, 0x63, 0x2d, 0x69, 0x6b, 0x65, 0x3b, 0x74, 0x72, 0x61, 0x6e, 0x73,
   /* -0680, */  0x70, 0x6f, 0x72, 0x74, 0x3d, 0x0d, 0x0a, 0x61, 0x3d, 0x6b, 0x65, 0x79, 0x77, 0x64, 0x73, 0x3a, 
   /* -0690, */  0x0d, 0x0a, 0x6b, 0x3d, 0x62, 0x61, 0x73, 0x65, 0x36, 0x34, 0x3a, 0x3b, 0x72, 0x65, 0x66, 0x72, 
   /* -06A0, */  0x65, 0x73, 0x68, 0x65, 0x72, 0x3d, 0x0d, 0x0a, 0x61, 0x3d, 0x70, 0x74, 0x69, 0x6d, 0x65, 0x3a, 
   /* -06B0, */  0x0d, 0x0a, 0x6b, 0x3d, 0x63, 0x6c, 0x65, 0x61, 0x72, 0x3a, 0x3b, 0x72, 0x65, 0x63, 0x65, 0x69, 
   /* -06C0, */  0x76, 0x65, 0x64, 0x3d, 0x3b, 0x64, 0x75, 0x72, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x3d, 0x0d, 0x0a, 
   /* -06D0, */  0x41, 0x63, 0x63, 0x65, 0x70, 0x74, 0x3a, 0x20, 0x0d, 0x0a, 0x61, 0x3d, 0x67, 0x72, 0x6f, 0x75, 
   /* -06E0, */  0x70, 0x3a, 0x46, 0x41, 0x4c, 0x53, 0x45, 0x3a, 0x20, 0x49, 0x4e, 0x46, 0x4f, 0x20, 0x0d, 0x0a, 
   /* -06F0, */  0x41, 0x63, 0x63, 0x65, 0x70, 0x74, 0x2d, 0x0d, 0x0a, 0x61, 0x3d, 0x6c, 0x61, 0x6e, 0x67, 0x3a, 
   /* -0700, */  0x0d, 0x0a, 0x6d, 0x3d, 0x64, 0x61, 0x74, 0x61, 0x20, 0x6d, 0x6f, 0x64, 0x65, 0x2d, 0x73, 0x65, 
   /* -0710, */  0x74, 0x3d, 0x0d, 0x0a, 0x61, 0x3d, 0x74, 0x6f, 0x6f, 0x6c, 0x3a, 0x54, 0x4c, 0x53, 0x75, 0x6e, 
   /* -0720, */  0x2c, 0x20, 0x0d, 0x0a, 0x44, 0x61, 0x74, 0x65, 0x3a, 0x20, 0x0d, 0x0a, 0x61, 0x3d, 0x63, 0x61, 
   /* -0730, */  0x74, 0x3a, 0x0d, 0x0a, 0x6b, 0x3d, 0x75, 0x72, 0x69, 0x3a, 0x0d, 0x0a, 0x50, 0x72, 0x6f, 0x78, 
   /* -0740, */  0x79, 0x2d, 0x3b, 0x72, 0x65, 0x61, 0x73, 0x6f, 0x6e, 0x3d, 0x3b, 0x6d, 0x65, 0x74, 0x68, 0x6f, 
   /* -0750, */  0x64, 0x3d, 0x0d, 0x0a, 0x61, 0x3d, 0x6d, 0x69, 0x64, 0x3a, 0x3b, 0x6d, 0x61, 0x64, 0x64, 0x72, 
   /* -0760, */  0x3d, 0x6f, 0x70, 0x61, 0x71, 0x75, 0x65, 0x3d, 0x0d, 0x0a, 0x4d, 0x69, 0x6e, 0x2d, 0x3b, 0x61, 
   /* -0770, */  0x6c, 0x67, 0x3d, 0x4d, 0x6f, 0x6e, 0x2c, 0x20, 0x54, 0x75, 0x65, 0x2c, 0x20, 0x57, 0x65, 0x64, 
   /* -0780, */  0x2c, 0x20, 0x46, 0x72, 0x69, 0x2c, 0x20, 0x53, 0x61, 0x74, 0x2c, 0x20, 0x3b, 0x74, 0x74, 0x6c, 
   /* -0790, */  0x3d, 0x61, 0x75, 0x74, 0x73, 0x3d, 0x0d, 0x0a, 0x72, 0x3d, 0x0d, 0x0a, 0x7a, 0x3d, 0x0d, 0x0a, 
   /* -07A0, */  0x65, 0x3d, 0x3b, 0x69, 0x64, 0x3d, 0x0d, 0x0a, 0x69, 0x3d, 0x63, 0x72, 0x63, 0x3d, 0x0d, 0x0a, 
   /* -07B0, */  0x75, 0x3d, 0x3b, 0x71, 0x3d, 0x75, 0x61, 0x73, 0x34, 0x31, 0x34, 0x20, 0x52, 0x65, 0x71, 0x75, 
   /* -07C0, */  0x65, 0x73, 0x74, 0x2d, 0x55, 0x52, 0x49, 0x20, 0x54, 0x6f, 0x6f, 0x20, 0x4c, 0x6f, 0x6e, 0x67, 
   /* -07D0, */  0x69, 0x76, 0x65, 0x75, 0x70, 0x72, 0x69, 0x76, 0x61, 0x63, 0x79, 0x75, 0x64, 0x70, 0x72, 0x65, 
   /* -07E0, */  0x66, 0x65, 0x72, 0x36, 0x30, 0x30, 0x20, 0x42, 0x75, 0x73, 0x79, 0x20, 0x45, 0x76, 0x65, 0x72, 
   /* -07F0, */  0x79, 0x77, 0x68, 0x65, 0x72, 0x65, 0x71, 0x75, 0x69, 0x72, 0x65, 0x64, 0x34, 0x38, 0x30, 0x20,       
   /* -0800, */  0x54, 0x65, 0x6d, 0x70, 0x6f, 0x72, 0x61, 0x72, 0x69, 0x6c, 0x79, 0x20, 0x55, 0x6e, 0x61, 0x76, 
   /* -0810, */  0x61, 0x69, 0x6c, 0x61, 0x62, 0x6c, 0x65, 0x0d, 0x0a, 0x61, 0x3d, 0x74, 0x79, 0x70, 0x65, 0x3a, 
   /* -0820, */  0x48, 0x2e, 0x33, 0x33, 0x32, 0x30, 0x32, 0x20, 0x41, 0x63, 0x63, 0x65, 0x70, 0x74, 0x65, 0x64, 
   /* -0830, */  0x0d, 0x0a, 0x53, 0x65, 0x73, 0x73, 0x69, 0x6f, 0x6e, 0x2d, 0x45, 0x78, 0x70, 0x69, 0x72, 0x65, 
   /* -0840, */  0x73, 0x3a, 0x20, 0x0d, 0x0a, 0x53, 0x75, 0x62, 0x73, 0x63, 0x72, 0x69, 0x70, 0x74, 0x69, 0x6f, 
   /* -0850, */  0x6e, 0x2d, 0x53, 0x74, 0x61, 0x74, 0x65, 0x3a, 0x20, 0x4e, 0x6f, 0x76, 0x20, 0x0d, 0x0a, 0x53, 
   /* -0860, */  0x65, 0x72, 0x76, 0x69, 0x63, 0x65, 0x2d, 0x52, 0x6f, 0x75, 0x74, 0x65, 0x3a, 0x20, 0x53, 0x65, 
   /* -0870, */  0x70, 0x20, 0x0d, 0x0a, 0x41, 0x6c, 0x6c, 0x6f, 0x77, 0x2d, 0x45, 0x76, 0x65, 0x6e, 0x74, 0x73, 
   /* -0880, */  0x3a, 0x20, 0x46, 0x65, 0x62, 0x20, 0x0d, 0x0a, 0x61, 0x3d, 0x69, 0x6e, 0x61, 0x63, 0x74, 0x69, 
   /* -0890, */  0x76, 0x65, 0x52, 0x54, 0x50, 0x2f, 0x53, 0x41, 0x56, 0x50, 0x20, 0x52, 0x54, 0x50, 0x2f, 0x41, 
   /* -08A0, */  0x56, 0x50, 0x46, 0x20, 0x41, 0x6e, 0x6f, 0x6e, 0x79, 0x6d, 0x6f, 0x75, 0x73, 0x69, 0x70, 0x73, 
   /* -08B0, */  0x3a, 0x0d, 0x0a, 0x61, 0x3d, 0x74, 0x79, 0x70, 0x65, 0x3a, 0x74, 0x65, 0x73, 0x74, 0x65, 0x6c, 
   /* -08C0, */  0x3a, 0x4d, 0x45, 0x53, 0x53, 0x41, 0x47, 0x45, 0x20, 0x0d, 0x0a, 0x61, 0x3d, 0x72, 0x65, 0x63, 
   /* -08D0, */  0x76, 0x6f, 0x6e, 0x6c, 0x79, 0x0d, 0x0a, 0x61, 0x3d, 0x73, 0x65, 0x6e, 0x64, 0x6f, 0x6e, 0x6c, 
   /* -08E0, */  0x79, 0x0d, 0x0a, 0x63, 0x3d, 0x49, 0x4e, 0x20, 0x49, 0x50, 0x34, 0x20, 0x0d, 0x0a, 0x52, 0x65, 
   /* -08F0, */  0x61, 0x73, 0x6f, 0x6e, 0x3a, 0x20, 0x0d, 0x0a, 0x41, 0x6c, 0x6c, 0x6f, 0x77, 0x3a, 0x20, 0x0d, 
   /* -0900, */  0x0a, 0x45, 0x76, 0x65, 0x6e, 0x74, 0x3a, 0x20, 0x0d, 0x0a, 0x50, 0x61, 0x74, 0x68, 0x3a, 0x20,        
   /* -0910, */  0x3b, 0x75, 0x73, 0x65, 0x72, 0x3d, 0x0d, 0x0a, 0x62, 0x3d, 0x41, 0x53, 0x20, 0x43, 0x54, 0x20,       
   /* -0920, */  0x0d, 0x0a, 0x57, 0x57, 0x57, 0x2d, 0x41, 0x75, 0x74, 0x68, 0x65, 0x6e, 0x74, 0x69, 0x63, 0x61, 
   /* -0930, */  0x74, 0x65, 0x3a, 0x20, 0x44, 0x69, 0x67, 0x65, 0x73, 0x74, 0x20, 0x0d, 0x0a, 0x61, 0x3d, 0x73, 
   /* -0940, */  0x65, 0x6e, 0x64, 0x72, 0x65, 0x63, 0x76, 0x69, 0x64, 0x65, 0x6f, 0x63, 0x74, 0x65, 0x74, 0x2d, 
   /* -0950, */  0x61, 0x6c, 0x69, 0x67, 0x6e, 0x3d, 0x61, 0x70, 0x70, 0x6c, 0x69, 0x63, 0x61, 0x74, 0x69, 0x6f, 
   /* -0960, */  0x6e, 0x2f, 0x73, 0x64, 0x70, 0x61, 0x74, 0x68, 0x65, 0x61, 0x64, 0x65, 0x72, 0x73, 0x70, 0x61, 
   /* -0970, */  0x75, 0x74, 0x68, 0x3d, 0x0d, 0x0a, 0x61, 0x3d, 0x6f, 0x72, 0x69, 0x65, 0x6e, 0x74, 0x3a, 0x70,        
   /* -0980, */  0x6f, 0x72, 0x74, 0x72, 0x61, 0x69, 0x74, 0x69, 0x6d, 0x65, 0x6f, 0x75, 0x74, 0x74, 0x72, 0x2d, 
   /* -0990, */  0x69, 0x6e, 0x74, 0x69, 0x63, 0x6f, 0x6e, 0x63, 0x3d, 0x34, 0x38, 0x33, 0x20, 0x54, 0x6f, 0x6f, 
   /* -09A0, */  0x20, 0x4d, 0x61, 0x6e, 0x79, 0x20, 0x48, 0x6f, 0x70, 0x73, 0x6c, 0x69, 0x6e, 0x66, 0x6f, 0x70,         
   /* -09B0, */  0x74, 0x69, 0x6f, 0x6e, 0x61, 0x6c, 0x67, 0x6f, 0x72, 0x69, 0x74, 0x68, 0x6d, 0x3d, 0x36, 0x30,        
   /* -09C0, */  0x34, 0x20, 0x44, 0x6f, 0x65, 0x73, 0x20, 0x4e, 0x6f, 0x74, 0x20, 0x45, 0x78, 0x69, 0x73, 0x74, 
   /* -09D0, */  0x20, 0x41, 0x6e, 0x79, 0x77, 0x68, 0x65, 0x72, 0x65, 0x73, 0x70, 0x6f, 0x6e, 0x73, 0x65, 0x3d, 
   /* -09E0, */  0x0d, 0x0a, 0x0d, 0x0a, 0x52, 0x65, 0x71, 0x75, 0x65, 0x73, 0x74, 0x2d, 0x44, 0x69, 0x73, 0x70,       
   /* -09F0, */  0x6f, 0x73, 0x69, 0x74, 0x69, 0x6f, 0x6e, 0x3a, 0x20, 0x4d, 0x44, 0x35, 0x38, 0x30, 0x20, 0x50,         
   /* -0A00, */  0x72, 0x65, 0x63, 0x6f, 0x6e, 0x64, 0x69, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x46, 0x61, 0x69, 0x6c, 
   /* -0A10, */  0x75, 0x72, 0x65, 0x70, 0x6c, 0x61, 0x63, 0x65, 0x73, 0x34, 0x32, 0x32, 0x20, 0x53, 0x65, 0x73, 
   /* -0A20, */  0x73, 0x69, 0x6f, 0x6e, 0x20, 0x49, 0x6e, 0x74, 0x65, 0x72, 0x76, 0x61, 0x6c, 0x20, 0x54, 0x6f, 
   /* -0A30, */  0x6f, 0x20, 0x53, 0x6d, 0x61, 0x6c, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x31, 0x38, 0x31, 0x20, 0x43, 
   /* -0A40, */  0x61, 0x6c, 0x6c, 0x20, 0x49, 0x73, 0x20, 0x42, 0x65, 0x69, 0x6e, 0x67, 0x20, 0x46, 0x6f, 0x72, 
   /* -0A50, */  0x77, 0x61, 0x72, 0x64, 0x65, 0x64, 0x6f, 0x6d, 0x61, 0x69, 0x6e, 0x3d, 0x66, 0x61, 0x69, 0x6c, 
   /* -0A60, */  0x75, 0x72, 0x65, 0x6e, 0x64, 0x65, 0x72, 0x65, 0x61, 0x6c, 0x6d, 0x3d, 0x53, 0x55, 0x42, 0x53, 
   /* -0A70, */  0x43, 0x52, 0x49, 0x42, 0x45, 0x20, 0x70, 0x72, 0x65, 0x63, 0x6f, 0x6e, 0x64, 0x69, 0x74, 0x69, 
   /* -0A80, */  0x6f, 0x6e, 0x6f, 0x72, 0x6d, 0x61, 0x6c, 0x69, 0x70, 0x73, 0x65, 0x63, 0x2d, 0x6d, 0x61, 0x6e, 
   /* -0A90, */  0x64, 0x61, 0x74, 0x6f, 0x72, 0x79, 0x34, 0x31, 0x33, 0x20, 0x52, 0x65, 0x71, 0x75, 0x65, 0x73, 
   /* -0AA0, */  0x74, 0x20, 0x45, 0x6e, 0x74, 0x69, 0x74, 0x79, 0x20, 0x54, 0x6f, 0x6f, 0x20, 0x4c, 0x61, 0x72, 
   /* -0AB0, */  0x67, 0x65, 0x32, 0x65, 0x31, 0x38, 0x33, 0x20, 0x53, 0x65, 0x73, 0x73, 0x69, 0x6f, 0x6e, 0x20,        
   /* -0AC0, */  0x50, 0x72, 0x6f, 0x67, 0x72, 0x65, 0x73, 0x73, 0x63, 0x74, 0x70, 0x34, 0x38, 0x36, 0x20, 0x42, 
   /* -0AD0, */  0x75, 0x73, 0x79, 0x20, 0x48, 0x65, 0x72, 0x65, 0x6d, 0x6f, 0x74, 0x65, 0x72, 0x6d, 0x69, 0x6e, 
   /* -0AE0, */  0x61, 0x74, 0x65, 0x64, 0x41, 0x4b, 0x41, 0x76, 0x31, 0x2d, 0x4d, 0x44, 0x35, 0x2d, 0x73, 0x65, 
   /* -0AF0, */  0x73, 0x73, 0x69, 0x6f, 0x6e, 0x6f, 0x6e, 0x65, 0x0d, 0x0a, 0x41, 0x75, 0x74, 0x68, 0x6f, 0x72, 
   /* -0B00, */  0x69, 0x7a, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x3a, 0x20, 0x36, 0x30, 0x33, 0x20, 0x44, 0x65, 0x63, 
   /* -0B10, */  0x6c, 0x69, 0x6e, 0x65, 0x78, 0x74, 0x6e, 0x6f, 0x6e, 0x63, 0x65, 0x3d, 0x34, 0x38, 0x35, 0x20,         
   /* -0B20, */  0x41, 0x6d, 0x62, 0x69, 0x67, 0x75, 0x6f, 0x75, 0x73, 0x65, 0x72, 0x6e, 0x61, 0x6d, 0x65, 0x3d, 
   /* -0B30, */  0x61, 0x75, 0x64, 0x69, 0x6f, 0x0d, 0x0a, 0x43, 0x6f, 0x6e, 0x74, 0x65, 0x6e, 0x74, 0x2d, 0x54, 
   /* -0B40, */  0x79, 0x70, 0x65, 0x3a, 0x20, 0x4d, 0x61, 0x72, 0x20, 0x0d, 0x0a, 0x52, 0x65, 0x63, 0x6f, 0x72, 
   /* -0B50, */  0x64, 0x2d, 0x52, 0x6f, 0x75, 0x74, 0x65, 0x3a, 0x20, 0x4a, 0x75, 0x6c, 0x20, 0x34, 0x30, 0x31,
   /* -0B60, */  0x20, 0x55, 0x6e, 0x61, 0x75, 0x74, 0x68, 0x6f, 0x72, 0x69, 0x7a, 0x65, 0x64, 0x0d, 0x0a, 0x52,
   /* -0B70, */  0x65, 0x71, 0x75, 0x69, 0x72, 0x65, 0x3a, 0x20, 0x0d, 0x0a, 0x74, 0x3d, 0x30, 0x20, 0x30, 0x2e, 
   /* -0B80, */  0x30, 0x2e, 0x30, 0x2e, 0x30, 0x0d, 0x0a, 0x53, 0x65, 0x72, 0x76, 0x65, 0x72, 0x3a, 0x20, 0x52, 
   /* -0B90, */  0x45, 0x47, 0x49, 0x53, 0x54, 0x45, 0x52, 0x20, 0x0d, 0x0a, 0x63, 0x3d, 0x49, 0x4e, 0x20, 0x49, 
   /* -0BA0, */  0x50, 0x36, 0x20, 0x31, 0x38, 0x30, 0x20, 0x52, 0x69, 0x6e, 0x67, 0x69, 0x6e, 0x67, 0x31, 0x30,    
   /* -0BB0, */  0x30, 0x20, 0x54, 0x72, 0x79, 0x69, 0x6e, 0x67, 0x76, 0x3d, 0x30, 0x0d, 0x0a, 0x6f, 0x3d, 0x55, 
   /* -0BC0, */  0x50, 0x44, 0x41, 0x54, 0x45, 0x20, 0x4e, 0x4f, 0x54, 0x49, 0x46, 0x59, 0x20, 0x0d, 0x0a, 0x53, 
   /* -0BD0, */  0x75, 0x70, 0x70, 0x6f, 0x72, 0x74, 0x65, 0x64, 0x3a, 0x20, 0x75, 0x6e, 0x6b, 0x6e, 0x6f, 0x77, 
   /* -0BE0, */  0x6e, 0x41, 0x4d, 0x52, 0x54, 0x50, 0x2f, 0x41, 0x56, 0x50, 0x20, 0x0d, 0x0a, 0x50, 0x72, 0x69, 
   /* -0BF0, */  0x76, 0x61, 0x63, 0x79, 0x3a, 0x20, 0x0d, 0x0a, 0x53, 0x65, 0x63, 0x75, 0x72, 0x69, 0x74, 0x79, 
   /* -0C00, */  0x2d, 0x0d, 0x0a, 0x45, 0x78, 0x70, 0x69, 0x72, 0x65, 0x73, 0x3a, 0x20, 0x0d, 0x0a, 0x61, 0x3d, 
   /* -0C10, */  0x72, 0x74, 0x70, 0x6d, 0x61, 0x70, 0x3a, 0x0d, 0x0a, 0x6d, 0x3d, 0x76, 0x69, 0x64, 0x65, 0x6f, 
   /* -0C20, */  0x20, 0x0d, 0x0a, 0x6d, 0x3d, 0x61, 0x75, 0x64, 0x69, 0x6f, 0x20, 0x0d, 0x0a, 0x73, 0x3d, 0x20,         
   /* -0C30, */  0x66, 0x61, 0x6c, 0x73, 0x65, 0x0d, 0x0a, 0x61, 0x3d, 0x63, 0x6f, 0x6e, 0x66, 0x3a, 0x3b, 0x65, 
   /* -0C40, */  0x78, 0x70, 0x69, 0x72, 0x65, 0x73, 0x3d, 0x0d, 0x0a, 0x52, 0x6f, 0x75, 0x74, 0x65, 0x3a, 0x20,
   /* -0C50, */  0x0d, 0x0a, 0x61, 0x3d, 0x66, 0x6d, 0x74, 0x70, 0x3a, 0x0d, 0x0a, 0x61, 0x3d, 0x63, 0x75, 0x72, 
   /* -0C60, */  0x72, 0x3a, 0x43, 0x6c, 0x69, 0x65, 0x6e, 0x74, 0x3a, 0x20, 0x56, 0x65, 0x72, 0x69, 0x66, 0x79, 
   /* -0C70, */  0x3a, 0x20, 0x0d, 0x0a, 0x61, 0x3d, 0x64, 0x65, 0x73, 0x3a, 0x0d, 0x0a, 0x52, 0x41, 0x63, 0x6b, 
   /* -0C80, */  0x3a, 0x20, 0x0d, 0x0a, 0x52, 0x53, 0x65, 0x71, 0x3a, 0x20, 0x42, 0x59, 0x45, 0x20, 0x63, 0x6e, 
   /* -0C90, */  0x6f, 0x6e, 0x63, 0x65, 0x3d, 0x31, 0x30, 0x30, 0x72, 0x65, 0x6c, 0x75, 0x72, 0x69, 0x3d, 0x71, 
   /* -0CA0, */  0x6f, 0x70, 0x3d, 0x54, 0x43, 0x50, 0x55, 0x44, 0x50, 0x71, 0x6f, 0x73, 0x78, 0x6d, 0x6c, 0x3b, 
   /* -0CB0, */  0x6c, 0x72, 0x0d, 0x0a, 0x56, 0x69, 0x61, 0x3a, 0x20, 0x53, 0x49, 0x50, 0x2f, 0x32, 0x2e, 0x30,      
   /* -0CC0, */  0x2f, 0x54, 0x43, 0x50, 0x20, 0x34, 0x30, 0x38, 0x20, 0x52, 0x65, 0x71, 0x75, 0x65, 0x73, 0x74, 
   /* -0CD0, */  0x20, 0x54, 0x69, 0x6d, 0x65, 0x6f, 0x75, 0x74, 0x69, 0x6d, 0x65, 0x72, 0x70, 0x73, 0x69, 0x70,        
   /* -0CE0, */  0x3a, 0x0d, 0x0a, 0x43, 0x6f, 0x6e, 0x74, 0x65, 0x6e, 0x74, 0x2d, 0x4c, 0x65, 0x6e, 0x67, 0x74,
   /* -0CF0, */  0x68, 0x3a, 0x20, 0x4f, 0x63, 0x74, 0x20, 0x0d, 0x0a, 0x56, 0x69, 0x61, 0x3a, 0x20, 0x53, 0x49, 
   /* -0D00, */  0x50, 0x2f, 0x32, 0x2e, 0x30, 0x2f, 0x55, 0x44, 0x50, 0x20, 0x3b, 0x63, 0x6f, 0x6d, 0x70, 0x3d, 
   /* -0D10, */  0x73, 0x69, 0x67, 0x63, 0x6f, 0x6d, 0x70, 0x72, 0x6f, 0x62, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x61, 
   /* -0D20, */  0x63, 0x6b, 0x3b, 0x62, 0x72, 0x61, 0x6e, 0x63, 0x68, 0x3d, 0x7a, 0x39, 0x68, 0x47, 0x34, 0x62, 
   /* -0D30, */  0x4b, 0x0d, 0x0a, 0x4d, 0x61, 0x78, 0x2d, 0x46, 0x6f, 0x72, 0x77, 0x61, 0x72, 0x64, 0x73, 0x3a, 
   /* -0D40, */  0x20, 0x41, 0x70, 0x72, 0x20, 0x53, 0x43, 0x54, 0x50, 0x52, 0x41, 0x43, 0x4b, 0x20, 0x49, 0x4e, 
   /* -0D50, */  0x56, 0x49, 0x54, 0x45, 0x20, 0x0d, 0x0a, 0x43, 0x61, 0x6c, 0x6c, 0x2d, 0x49, 0x44, 0x3a, 0x20,         
   /* -0D60, */  0x0d, 0x0a, 0x43, 0x6f, 0x6e, 0x74, 0x61, 0x63, 0x74, 0x3a, 0x20, 0x32, 0x30, 0x30, 0x20, 0x4f, 
   /* -0D70, */  0x4b, 0x0d, 0x0a, 0x46, 0x72, 0x6f, 0x6d, 0x3a, 0x20, 0x0d, 0x0a, 0x43, 0x53, 0x65, 0x71, 0x3a, 
   /* -0D80, */  0x20, 0x0d, 0x0a, 0x54, 0x6f, 0x3a, 0x20, 0x3b, 0x74, 0x61, 0x67, 0x3d, 0x04, 0x10, 0xdd, 0x10,        
   /* -0D90, */  0x11, 0x31, 0x0d, 0x11, 0x0a, 0x07, 0x10, 0xb9, 0x0c, 0x10, 0xfe, 0x12, 0x10, 0xe1, 0x06, 0x11,
   /* -0DA0, */  0x4e, 0x07, 0x11, 0x4e, 0x03, 0x11, 0x4a, 0x04, 0x11, 0x4a, 0x07, 0x10, 0xb2, 0x08, 0x11, 0x79, 
   /* -0DB0, */  0x06, 0x11, 0x81, 0x0f, 0x11, 0x22, 0x0b, 0x11, 0x55, 0x06, 0x11, 0x6b, 0x0b, 0x11, 0x60, 0x13, 
   /* -0DC0, */  0x10, 0xb2, 0x08, 0x11, 0x71, 0x05, 0x11, 0x87, 0x13, 0x10, 0xf7, 0x09, 0x0e, 0x8d, 0x08, 0x0d, 
   /* -0DD0, */  0xae, 0x0c, 0x10, 0xb9, 0x07, 0x10, 0x8e, 0x03, 0x0d, 0x96, 0x03, 0x10, 0x8a, 0x04, 0x10, 0x8a, 
   /* -0DE0, */  0x09, 0x0d, 0xd7, 0x0a, 0x0f, 0x12, 0x08, 0x0f, 0x8f, 0x09, 0x0f, 0x8f, 0x08, 0x0d, 0x6c, 0x06, 
   /* -0DF0, */  0x0e, 0x66, 0x09, 0x0e, 0x6c, 0x0a, 0x0e, 0x6c, 0x06, 0x0f, 0xc6, 0x07, 0x0f, 0xc6, 0x05, 0x11, 
   /* -0E00, */  0x48, 0x06, 0x11, 0x48, 0x06, 0x0f, 0xbf, 0x07, 0x0f, 0xbf, 0x07, 0x0e, 0x55, 0x06, 0x0f, 0x16, 
   /* -0E10, */  0x04, 0x0e, 0xf4, 0x03, 0x0e, 0xb1, 0x03, 0x10, 0xa6, 0x09, 0x10, 0x50, 0x03, 0x10, 0xa3, 0x0a, 
   /* -0E20, */  0x0d, 0xb4, 0x05, 0x0e, 0x36, 0x06, 0x0e, 0xd6, 0x03, 0x0d, 0xf9, 0x11, 0x0e, 0xf8, 0x04, 0x0c,
   /* -0E30, */  0xd9, 0x08, 0x0e, 0xea, 0x04, 0x09, 0x53, 0x03, 0x0a, 0x4b, 0x04, 0x0e, 0xe4, 0x10, 0x0f, 0x35, 
   /* -0E40, */  0x09, 0x0e, 0xe4, 0x08, 0x0d, 0x3f, 0x03, 0x0f, 0xe1, 0x0b, 0x10, 0x01, 0x03, 0x10, 0xac, 0x06,
   /* -0E50, */  0x10, 0x95, 0x0c, 0x0e, 0x76, 0x0b, 0x0f, 0xeb, 0x0a, 0x0f, 0xae, 0x05, 0x10, 0x2b, 0x04, 0x10, 
   /* -0E60, */  0x2b, 0x08, 0x10, 0x7a, 0x10, 0x0f, 0x49, 0x07, 0x0f, 0xb8, 0x09, 0x10, 0x3e, 0x0b, 0x10, 0x0c, 
   /* -0E70, */  0x07, 0x0f, 0x78, 0x0b, 0x0f, 0x6d, 0x09, 0x10, 0x47, 0x08, 0x10, 0x82, 0x0b, 0x0f, 0xf6, 0x08, 
   /* -0E80, */  0x10, 0x62, 0x08, 0x0f, 0x87, 0x08, 0x10, 0x6a, 0x04, 0x0f, 0x78, 0x0d, 0x0f, 0xcd, 0x08, 0x0d, 
   /* -0E90, */  0xae, 0x10, 0x0f, 0x5d, 0x0b, 0x0f, 0x98, 0x14, 0x0d, 0x20, 0x1b, 0x0d, 0x20, 0x04, 0x0d, 0xe0,        
   /* -0EA0, */  0x14, 0x0e, 0xb4, 0x0b, 0x0f, 0xa3, 0x0b, 0x07, 0x34, 0x0f, 0x0d, 0x56, 0x04, 0x0e, 0xf4, 0x03, 
   /* -0EB0, */  0x10, 0xaf, 0x07, 0x0d, 0x34, 0x09, 0x0f, 0x27, 0x04, 0x10, 0x9b, 0x04, 0x10, 0x9f, 0x09, 0x10,       
   /* -0EC0, */  0x59, 0x08, 0x10, 0x72, 0x09, 0x10, 0x35, 0x0a, 0x10, 0x21, 0x0a, 0x10, 0x17, 0x08, 0x0f, 0xe3, 
   /* -0ED0, */  0x03, 0x10, 0xa9, 0x05, 0x0c, 0xac, 0x04, 0x0c, 0xbd, 0x07, 0x0c, 0xc1, 0x08, 0x0c, 0xc1, 0x09, 
   /* -0EE0, */  0x0c, 0xf6, 0x10, 0x0c, 0x72, 0x0c, 0x0c, 0x86, 0x04, 0x0d, 0x64, 0x0c, 0x0c, 0xd5, 0x09, 0x0c, 
   /* -0EF0, */  0xff, 0x1b, 0x0b, 0xfc, 0x11, 0x0c, 0x5d, 0x13, 0x0c, 0x30, 0x09, 0x0c, 0xa4, 0x0c, 0x0c, 0x24, 
   /* -0F00, */  0x0c, 0x0d, 0x3b, 0x03, 0x0d, 0x1a, 0x03, 0x0d, 0x1d, 0x16, 0x0c, 0x43, 0x09, 0x0c, 0x92, 0x09, 
   /* -0F10, */  0x0c, 0x9b, 0x0d, 0x0e, 0xcb, 0x04, 0x0d, 0x16, 0x06, 0x0d, 0x10, 0x05, 0x04, 0xf2, 0x0b, 0x0c, 
   /* -0F20, */  0xe1, 0x05, 0x0b, 0xde, 0x0a, 0x0c, 0xec, 0x13, 0x0b, 0xe3, 0x07, 0x0b, 0xd4, 0x08, 0x0d, 0x08, 
   /* -0F30, */  0x0c, 0x0c, 0xc9, 0x09, 0x0c, 0x3a, 0x04, 0x0a, 0xe5, 0x0c, 0x0a, 0x23, 0x08, 0x0b, 0x3a, 0x0e, 
   /* -0F40, */  0x09, 0xab, 0x0f, 0x0e, 0xfa, 0x09, 0x0f, 0x6f, 0x0c, 0x0a, 0x17, 0x0f, 0x09, 0x76, 0x0c, 0x0a, 
   /* -0F50, */  0x5f, 0x17, 0x0d, 0xe2, 0x0f, 0x07, 0xa8, 0x0a, 0x0f, 0x85, 0x0f, 0x08, 0xd6, 0x0e, 0x09, 0xb9, 
   /* -0F60, */  0x0b, 0x0a, 0x7a, 0x03, 0x0b, 0xdb, 0x03, 0x08, 0xc1, 0x04, 0x0e, 0xc7, 0x03, 0x08, 0xd3, 0x02, 
   /* -0F70, */  0x04, 0x8d, 0x08, 0x0b, 0x4a, 0x05, 0x0b, 0x8c, 0x07, 0x0b, 0x61, 0x06, 0x05, 0x48, 0x04, 0x07, 
   /* -0F80, */  0xf4, 0x05, 0x10, 0x30, 0x04, 0x07, 0x1e, 0x08, 0x07, 0x1e, 0x05, 0x0b, 0x91, 0x10, 0x04, 0xca, 
   /* -0F90, */  0x09, 0x0a, 0x71, 0x09, 0x0e, 0x87, 0x05, 0x04, 0x98, 0x05, 0x0b, 0x6e, 0x0b, 0x04, 0x9b, 0x0f, 
   /* -0FA0, */  0x04, 0x9b, 0x07, 0x04, 0x9b, 0x03, 0x04, 0xa3, 0x07, 0x04, 0xa3, 0x10, 0x07, 0x98, 0x09, 0x07, 
   /* -0FB0, */  0x98, 0x05, 0x0b, 0x73, 0x05, 0x0b, 0x78, 0x05, 0x0b, 0x7d, 0x05, 0x07, 0xb9, 0x05, 0x0b, 0x82, 
   /* -0FC0, */  0x05, 0x0b, 0x87, 0x05, 0x0b, 0x1d, 0x05, 0x08, 0xe4, 0x05, 0x0c, 0x81, 0x05, 0x0f, 0x44, 0x05, 
   /* -0FD0, */  0x11, 0x40, 0x05, 0x08, 0x78, 0x05, 0x08, 0x9d, 0x05, 0x0f, 0x58, 0x05, 0x07, 0x3f, 0x05, 0x0c, 
   /* -0FE0, */  0x6d, 0x05, 0x10, 0xf2, 0x05, 0x0c, 0x58, 0x05, 0x06, 0xa9, 0x04, 0x07, 0xb6, 0x09, 0x05, 0x8c, 
   /* -0FF0, */  0x06, 0x06, 0x1a, 0x06, 0x0e, 0x81, 0x0a, 0x06, 0x16, 0x0a, 0x0a, 0xc4, 0x07, 0x0b, 0x5a, 0x0a, 
   /* -1000, */  0x0a, 0xba, 0x03, 0x0b, 0x1b, 0x04, 0x11, 0x45, 0x06, 0x0c, 0x8c, 0x07, 0x05, 0xad, 0x0a, 0x0e, 
   /* -1010, */  0xda, 0x08, 0x0b, 0x42, 0x0d, 0x09, 0xf7, 0x0b, 0x05, 0x1c, 0x09, 0x11, 0x16, 0x08, 0x05, 0xc9, 
   /* -1020, */  0x07, 0x0d, 0x86, 0x06, 0x0b, 0xcf, 0x0a, 0x06, 0x4d, 0x04, 0x0b, 0xa2, 0x06, 0x06, 0x8d, 0x08, 
   /* -1030, */  0x05, 0xe6, 0x08, 0x0e, 0x11, 0x0b, 0x0a, 0x9b, 0x03, 0x0a, 0x04, 0x03, 0x0b, 0xb5, 0x05, 0x10,      
   /* -1040, */  0xd7, 0x04, 0x09, 0x94, 0x05, 0x0a, 0xe2, 0x03, 0x0b, 0xb2, 0x06, 0x0d, 0x67, 0x04, 0x0d, 0x11, 
   /* -1050, */  0x08, 0x08, 0xb7, 0x1b, 0x0e, 0x3b, 0x0a, 0x09, 0xa1, 0x14, 0x04, 0x85, 0x15, 0x07, 0x83, 0x15, 
   /* -1060, */  0x07, 0x6e, 0x0d, 0x09, 0x3d, 0x17, 0x06, 0xae, 0x0f, 0x07, 0xe6, 0x14, 0x07, 0xbe, 0x0d, 0x06, 
   /* -1070, */  0x0a, 0x0d, 0x09, 0x30, 0x16, 0x06, 0xf2, 0x12, 0x08, 0x1e, 0x21, 0x04, 0xaa, 0x13, 0x10, 0xc5, 
   /* -1080, */  0x08, 0x0a, 0x0f, 0x1c, 0x0e, 0x96, 0x18, 0x0b, 0xb8, 0x1a, 0x05, 0x95, 0x1a, 0x05, 0x75, 0x11, 
   /* -1090, */  0x06, 0x3d, 0x16, 0x06, 0xdc, 0x1e, 0x0e, 0x19, 0x16, 0x05, 0xd1, 0x1d, 0x06, 0x20, 0x23, 0x05, 
   /* -10A0, */  0x27, 0x11, 0x08, 0x7d, 0x11, 0x0d, 0x99, 0x16, 0x04, 0xda, 0x0d, 0x0f, 0x1c, 0x16, 0x07, 0x08, 
   /* -10B0, */  0x17, 0x05, 0xb4, 0x0d, 0x08, 0xc7, 0x13, 0x07, 0xf8, 0x12, 0x08, 0x57, 0x1f, 0x04, 0xfe, 0x19, 
   /* -10C0, */  0x05, 0x4e, 0x13, 0x08, 0x0b, 0x0f, 0x08, 0xe9, 0x17, 0x06, 0xc5, 0x13, 0x06, 0x7b, 0x19, 0x05, 
   /* -10D0, */  0xf1, 0x15, 0x07, 0x44, 0x18, 0x0d, 0xfb, 0x0b, 0x0f, 0x09, 0x1b, 0x0d, 0xbe, 0x12, 0x08, 0x30,        
   /* -10E0, */  0x15, 0x07, 0x59, 0x04, 0x0b, 0xa6, 0x04, 0x0b, 0xae, 0x04, 0x0b, 0x9e, 0x04, 0x0b, 0x96, 0x04, 
   /* -10F0, */  0x0b, 0x9a, 0x0a, 0x0a, 0xb0, 0x0b, 0x0a, 0x90, 0x08, 0x0b, 0x32, 0x0b, 0x09, 0x6b, 0x08, 0x0b, 
   /* -1100, */  0x2a, 0x0b, 0x0a, 0x85, 0x09, 0x0b, 0x12, 0x0a, 0x0a, 0xa6, 0x0d, 0x09, 0xea, 0x13, 0x0d, 0x74, 
   /* -1110, */  0x14, 0x07, 0xd2, 0x13, 0x09, 0x0b, 0x12, 0x08, 0x42, 0x10, 0x09, 0x5b, 0x12, 0x09, 0x1e, 0x0d, 
   /* -1120, */  0x0c, 0xb1, 0x0e, 0x0c, 0x17, 0x11, 0x09, 0x4a, 0x0c, 0x0a, 0x53, 0x0c, 0x0a, 0x47, 0x09, 0x0a, 
   /* -1130, */  0xf7, 0x0e, 0x09, 0xc7, 0x0c, 0x0a, 0x3b, 0x07, 0x06, 0x69, 0x08, 0x06, 0x69, 0x06, 0x09, 0xe3, 
   /* -1140, */  0x08, 0x0b, 0x52, 0x0a, 0x0a, 0xd8, 0x12, 0x06, 0x57, 0x0d, 0x06, 0x57, 0x07, 0x09, 0xe3, 0x04, 
   /* -1150, */  0x0a, 0xe9, 0x10, 0x07, 0x30, 0x09, 0x0b, 0x00, 0x0c, 0x0a, 0x2f, 0x05, 0x0a, 0xe9, 0x05, 0x0a, 
   /* -1160, */  0x6b, 0x06, 0x0a, 0x6b, 0x0a, 0x0a, 0xce, 0x09, 0x0a, 0xee, 0x03, 0x0b, 0xdb, 0x07, 0x0f, 0x7e, 
   /* -1170, */  0x0a, 0x09, 0x97, 0x0a, 0x06, 0x71, 0x0e, 0x09, 0xd5, 0x17, 0x06, 0x93, 0x07, 0x0e, 0x5c, 0x07, 
   /* -1180, */  0x0f, 0xda, 0x0a, 0x0f, 0x35, 0x0d, 0x0d, 0xec, 0x0a, 0x09, 0x97, 0x0a, 0x06, 0x71, 0x08, 0x0b, 
   /* -1190, */  0x22, 0x0f, 0x09, 0x85, 0x06, 0x0b, 0x68, 0x0c, 0x0d, 0x4a, 0x09, 0x0b, 0x09, 0x13, 0x08, 0xf8, 
   /* -11A0, */  0x15, 0x08, 0xa2, 0x04, 0x0b, 0xaa, 0x0f, 0x05, 0x66, 0x0d, 0x07, 0x23, 0x09, 0x0a, 0x06, 0x0b, 
   /* -11B0, */  0x0d, 0x4a, 0x0f, 0x04, 0xee, 0x06, 0x04, 0xf8, 0x04, 0x09, 0x2b, 0x04, 0x08, 0x53, 0x07, 0x08, 
   /* -11C0, */  0xc0, 0x03, 0x11, 0x1f, 0x04, 0x11, 0x1e, 0x07, 0x0d, 0x8c, 0x03, 0x07, 0x34, 0x04, 0x10, 0xdb, 
   /* -11D0, */  0x03, 0x07, 0x36, 0x03, 0x0d, 0xa9, 0x0d, 0x04, 0x20, 0x0b, 0x04, 0x51, 0x0c, 0x04, 0x3a, 0x04, 
   /* -11E0, */  0x0b, 0xb8, 0x04, 0x0c, 0x24, 0x04, 0x05, 0x95, 0x04, 0x04, 0x7c, 0x04, 0x05, 0x75, 0x04, 0x04, 
   /* -11F0, */  0x85, 0x04, 0x09, 0x6b, 0x04, 0x06, 0x3d, 0x06, 0x04, 0x7b, 0x04, 0x06, 0xdc, 0x04, 0x07, 0x83,
   /* -1200, */  0x04, 0x0e, 0x19, 0x12, 0x04, 0x00, 0x10, 0x08, 0x8e, 0x10, 0x08, 0x69, 0x0e, 0x04, 0x12, 0x0d,
   /* -1210, */  0x04, 0x2d, 0x03, 0x10, 0xb9, 0x04, 0x05, 0xd1, 0x04, 0x07, 0x6e, 0x04, 0x06, 0x20, 0x07, 0x04, 
   /* -1220, */  0x74, 0x04, 0x0b, 0xfc, 0x0a, 0x04, 0x5c, 0x04, 0x05, 0x27, 0x04, 0x09, 0x3d, 0x04, 0x08, 0x7d, 
   /* -1230, */  0x04, 0x0f, 0xae, 0x04, 0x0d, 0x99, 0x04, 0x06, 0xae, 0x04, 0x04, 0xda, 0x09, 0x04, 0x09, 0x08, 
   /* -1240, */  0x11, 0x22, 0x04, 0x0f, 0x1c, 0x04, 0x07, 0xe6, 0x04, 0x0e, 0xcb, 0x05, 0x08, 0xbd, 0x04, 0x07,
   /* -1250, */  0x08, 0x04, 0x0f, 0xa3, 0x04, 0x06, 0x57, 0x04, 0x05, 0xb4, 0x04, 0x0f, 0x5d, 0x04, 0x08, 0xc7,
   /* -1260, */  0x08, 0x0b, 0xf4, 0x04, 0x07, 0xf8, 0x04, 0x07, 0x30, 0x04, 0x07, 0xbe, 0x04, 0x08, 0x57, 0x05, 
   /* -1270, */  0x0d, 0x46, 0x04, 0x04, 0xfe, 0x04, 0x06, 0x0a, 0x04, 0x05, 0x4e, 0x04, 0x0e, 0x3b, 0x04, 0x08,
   /* -1280, */  0x0b, 0x04, 0x09, 0x30, 0x04, 0x08, 0xe9, 0x05, 0x05, 0xee, 0x04, 0x06, 0xc5, 0x04, 0x06, 0xf2, 
   /* -1290, */  0x04, 0x06, 0x7b, 0x04, 0x09, 0xa1, 0x04, 0x05, 0xf1, 0x04, 0x08, 0x1e, 0x04, 0x07, 0x44, 0x04, 
   /* -12A0, */  0x0b, 0xdd, 0x04, 0x0d, 0xfb, 0x04, 0x04, 0xaa, 0x04, 0x0b, 0xe3, 0x07, 0x0e, 0xee, 0x04, 0x0f, 
   /* -12B0, */  0x09, 0x04, 0x0e, 0xb4, 0x04, 0x0d, 0xbe, 0x04, 0x10, 0xc5, 0x04, 0x08, 0x30, 0x05, 0x0f, 0x30,     
   /* -12C0, */  0x04, 0x07, 0x59, 0x04, 0x0a, 0x0f, 0x06, 0x0e, 0x61, 0x04, 0x04, 0x81, 0x04, 0x0d, 0xab, 0x04, 
   /* -12D0, */  0x0d, 0x93, 0x04, 0x11, 0x6b, 0x04, 0x0e, 0x96, 0x05, 0x04, 0x66, 0x09, 0x04, 0x6b, 0x0b, 0x04,
   /* -12E0, */  0x46, 0x04, 0x0c, 0xe1

}; 
                          
int udvm_state_access(guint8 buff[],guint16 p_id_start, guint16 p_id_length, guint16 state_begin, guint16 state_length, 
					   guint16 state_address, guint16 state_instruction)
{
	int			result_code = 0;
	guint		n;
	guint16		k;
	guint16		byte_copy_right;
	guint16		byte_copy_left;

	/* 
	 * Perform initial checks on validity of data
	 * RFC 3320 :
	 * 9.4.5.  STATE-ACCESS
	 * :
	 * Decompression failure occurs if partial_identifier_length does not
	 * lie between 6 and 20 inclusive.  Decompression failure also occurs if
	 * no state item matching the partial state identifier can be found, if
	 * more than one state item matches the partial identifier, or if
	 * partial_identifier_length is less than the minimum_access_length of
	 * the matched state item. Otherwise, a state item is returned from the
	 * state handler.
	 */

	if (( p_id_length < 6 ) || ( p_id_length > 20 )){
		result_code = 1;
		return result_code;
	}
	/* 
	 * Is this a static library known to us ?
	 */
	n = 0;
	while (n < p_id_length ) {
		if ( buff[p_id_start + n] != sip_sdp_state_identifier[n] )
			goto not_static_dictionary;
		n++;
	}
	/* 
	 * sip_sdp_static_dictionaty
	 *
	 * 8.4.  Byte copying
	 * :
	 * The string of bytes is copied in ascending order of memory address,
	 * respecting the bounds set by byte_copy_left and byte_copy_right.
	 * More precisely, if a byte is copied from/to Address m then the next
	 * byte is copied from/to Address n where n is calculated as follows:
	 *
	 * Set k := m + 1 (modulo 2^16)
	 * If k = byte_copy_right then set n := byte_copy_left, else set n := k
	 *
	 */ 
	if ( ( state_begin + state_length ) > sip_sdp_state_length )
		return 3;

	n = state_begin;
	k = state_address; 
	byte_copy_right = buff[66] << 8;
	byte_copy_right = byte_copy_right ^ buff[67];
	byte_copy_left = buff[64] << 8;
	byte_copy_left = byte_copy_left ^ buff[65];
	/* debug 
	 *g_warning(" state_begin %u state_address %u",state_begin , state_address);
	 */
	while ( n < state_length ){
		buff[k] = sip_sdp_static_dictionaty_for_sigcomp[n];
		/*  debug 
		 *	g_warning(" Loading 0x%x at address %u",buff[k] , k);
		 */
		k = ( k + 1 ) & 0xffff;
		if ( k == byte_copy_right ){
			byte_copy_left = buff[64] << 8;
			byte_copy_left = byte_copy_left ^ buff[65];
			k = byte_copy_left;
		}
		n++;
	}

	return 0;
	/*
	 * End SIP
	 */
not_static_dictionary:
	return 255;
}

void udvm_state_create(guint8 state_buff[],guint8 state_identifier[]){
	/*
	 * Debug
	g_warning("Received items of state,state_length_buff[0]= %u, state_length_buff[1]= %u",
		state_length_buff[0],state_length_buff[1]);

	 */

}
