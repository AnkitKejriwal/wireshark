/* packet-ncp-nmas.c
 * Routines for Novell Modular Authentication Service
 * Greg Morris <gmorris@novell.com>
 * Copyright (c) Novell, Inc. 2002-2004
 *
 * $Id: packet-ncp-nmas.c,v 1.1 2004/02/29 08:01:22 guy Exp $
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

#include <string.h>
#include <glib.h>
#include <epan/packet.h>
#include "prefs.h"
#include "packet-ncp-int.h"
#include "packet-ncp-nmas.h"

static gint ett_nmas = -1;

static int proto_nmas = -1;
static int hf_func = -1;
static int hf_subfunc = -1;
static int hf_ping_version = -1;
static int hf_ping_flags = -1;
static int hf_frag_handle = -1;
static int hf_length = -1;
static int hf_subverb = -1;
static int hf_tree = -1;
static int hf_user = -1;
static int hf_nmas_version = -1;
static int hf_msg_version = -1;
static int hf_session_ident = -1;
static int hf_msg_verb = -1;
static int hf_attribute = -1;
static int hf_clearence = -1;
static int hf_login_sequence = -1;
static int hf_opaque = -1;
static int hf_data = -1;
static int hf_return_code = -1;
static int hf_lsm_verb = -1;
static int hf_squeue_bytes = -1;
static int hf_cqueue_bytes = -1;
static int hf_num_creds = -1;
static int hf_cred_type = -1;
static int hf_login_state = -1;
static int hf_enc_cred = -1;
static int hf_enc_data = -1;

static const value_string nmas_func_enum[] = {
    { 0x00000001, "Ping" },
    { 0x00000002, "Fragment" },
    { 0x00000003, "Abort" },
    { 0,          NULL }
};

static const value_string nmas_subverb_enum[] = {
    { 0, "Fragmented Ping" },
    { 2, "Client Put Data" },
    { 4, "Client Get Data" },
    { 6, "Client Get User NDS Credentials" },
    { 8, "Login Store Management" },
    { 10, "Writable Object Check" },
    { 1242, "Message Handler" },
    { 0,          NULL }
};

static const value_string nmas_msgverb_enum[] = {
    { 1, "Echo Data" },
    { 3, "Start Session" },
    { 5, "Client Write Data" },
    { 7, "Client Read Data" },
    { 9, "End Session" },
    { 0,          NULL }
};

static const value_string nmas_attribute_enum[] = {
    { 1, "User Name" },
    { 2, "Tree Name" },
    { 4, "Clearence" },
    { 11, "Login Sequence" },
    { 0,          NULL }
};

static const value_string nmas_lsmverb_enum[] = {
    { 1, "Put Login Configuration" },
    { 2, "Get Login Configuration" },
    { 4, "Delete Login Configuration" },
    { 5, "Put Login Secret" },
    { 6, "Delete Login Secret" },
    { 0,          NULL }
};

static const value_string nmas_errors_enum[] = {
    { 0xFFFFF9A1, "(-1631) FRAGMENT FAILURE" },
    { 0xFFFFF9A0, "(-1632) BAD REQUEST SYNTAX" },
    { 0xFFFFF99F, "(-1633) BUFFER OVERFLOW" },
    { 0xFFFFF99E, "(-1634) SYSTEM RESOURCES" },
    { 0xFFFFF99D, "(-1635) INSUFFICIENT MEMORY" },
    { 0xFFFFF99C, "(-1636) NOT SUPPORTED" },
    { 0xFFFFF99B, "(-1637) BUFFER UNDERFLOW" },
    { 0xFFFFF99A, "(-1638) NOT FOUND" },
    { 0xFFFFF999, "(-1639) INVALID OPERATION" },
    { 0xFFFFF998, "(-1640) ASN1 DECODE" },
    { 0xFFFFF997, "(-1641) ASN1 ENCODE" },
    { 0xFFFFF996, "(-1642) LOGIN FAILED" },
    { 0xFFFFF995, "(-1643) INVALID PARAMETER" },
    { 0xFFFFF994, "(-1644) TIMED OUT RECOVERABLE" },
    { 0xFFFFF993, "(-1645) TIMED OUT NOT RECOVERABLE" },
    { 0xFFFFF992, "(-1646) TIMED OUT UNKNOWN" },
    { 0xFFFFF991, "(-1647) AUTHORIZATION FAILURE" },
    { 0xFFFFF990, "(-1648) INVALID DISTINGUSHED NAME" },
    { 0xFFFFF98F, "(-1649) CANNOT RESOLVE DISTINGUISHED NAME" },
    { 0xFFFFF98E, "(-1650) CANNOT RESOLVE CONNECTION" },
    { 0xFFFFF98D, "(-1651) NO CRYPTOGRAPHY" },
    { 0xFFFFF98C, "(-1652) INVALID VERSION" },
    { 0xFFFFF98B, "(-1653) SYNC NEEDED" },
    { 0xFFFFF98A, "(-1654) PROTOCOL STATE" },
    { 0xFFFFF989, "(-1655) INVALID HANDLE" },
    { 0xFFFFF988, "(-1656) INVALID METHOD" },
    { 0xFFFFF987, "(-1657) DEVELOPMENT VERSION" },
    { 0xFFFFF986, "(-1658) MISSING KEY" },
    { 0xFFFFF985, "(-1659) ACCESS NOT ALLOWED" },
    { 0xFFFFF984, "(-1660) SEQUENCE NOT FOUND" },
    { 0xFFFFF983, "(-1661) CLEARANCE NOT FOUND" },
    { 0xFFFFF982, "(-1662) LOGIN SERVER METHOD NOT FOUND" },
    { 0xFFFFF981, "(-1663) LOGIN CLIENT METHOD NOT FOUND" },
    { 0xFFFFF980, "(-1664) SERVER NOT FOUND" },
    { 0xFFFFF97F, "(-1665) LOGIN ATTRIBUTE NOT FOUND" },
    { 0xFFFFF97E, "(-1666) LEGACY INVALID PASSWORD" },
    { 0xFFFFF97D, "(-1667) ACCOUNT DISABLED" },
    { 0xFFFFF97C, "(-1668) ACCOUNT LOCKED" },
    { 0xFFFFF97B, "(-1669) ADDRESS RESTRICTION" },
    { 0xFFFFF97A, "(-1670) CONNECTION CLEARED" },
    { 0xFFFFF979, "(-1671) TIME RESTRICTION" },
    { 0xFFFFF978, "(-1672) SHORT TERM SECRET" },
    { 0xFFFFF977, "(-1673) NO NMAS ON TREE" },
    { 0xFFFFF976, "(-1674) NO NMAS ON SERVER" },
    { 0xFFFFF975, "(-1675) REQUEST CHALLENGED" },
    { 0xFFFFF974, "(-1676) LOGIN CANCELED" },
    { 0xFFFFF973, "(-1677) LOCAL CREDENTIAL STORE" },
    { 0xFFFFF972, "(-1678) REMOTE CREDENTIAL STORE" },
    { 0xFFFFF971, "(-1679) SMC NICM" },
    { 0xFFFFF970, "(-1680) SEQUENCE NOT AUTHORIZED" },
    { 0xFFFFF96F, "(-1681) TRANSPORT" },
    { 0xFFFFF96E, "(-1682) CRYPTO FAILED INIT" },
    { 0xFFFFF96D, "(-1683) DOUBLEBYTE FAILED INIT" },
    { 0xFFFFF96C, "(-1684) CODEPAGE FAILED INIT" },
    { 0xFFFFF96B, "(-1685) UNICODE FAILED INIT" },
    { 0xFFFFF96A, "(-1686) DLL FAILED LOADING" },
    { 0xFFFFF969, "(-1687) EVALUATION VERSION WARNING" },
    { 0xFFFFF968, "(-1688) CONCURRENT LOGIN" },
    { 0xFFFFF969, "(-1689) THREAD CREATE" },
    { 0xFFFFF96A, "(-1690) SECURE CHANNEL REQUIRED" },
    { 0xFFFFF96B, "(-1691) NO DEFAULT USER SEQUENCE" },
    { 0xFFFFF96C, "(-1692) NO TREENAME" },
    { 0xFFFFF96D, "(-1693) MECHANISM NOT FOUND" },
    { 0,          NULL }
};

static int
align_4(tvbuff_t *tvb, int aoffset)
{
       if(tvb_length_remaining(tvb, aoffset) > 4 )
       {
                return (aoffset%4);
       }
       return 0;
}

static int
nmas_string(tvbuff_t* tvb, int hfinfo, proto_tree *nmas_tree, int offset, gboolean little)
{
        int     foffset = offset;
        guint32 str_length;
        char    buffer[1024];
        guint32 i;
        guint16 c_char;
        guint32 length_remaining = 0;
        
        if (little) {
            str_length = tvb_get_letohl(tvb, foffset);
        }
        else
        {
            str_length = tvb_get_ntohl(tvb, foffset);
        }
        foffset += 4;
        length_remaining = tvb_length_remaining(tvb, foffset);
        if(str_length > (guint)length_remaining || str_length > 1024)
        {
                proto_tree_add_string(nmas_tree, hfinfo, tvb, foffset,
                    length_remaining + 4, "<String too long to process>");
                foffset += length_remaining;
                return foffset;
        }
        if(str_length == 0)
        {
       	    proto_tree_add_string(nmas_tree, hfinfo, tvb, offset,
                4, "<Not Specified>");
            return foffset;
        }
        for ( i = 0; i < str_length; i++ )
        {
                c_char = tvb_get_guint8(tvb, foffset );
                if (c_char<0x20 || c_char>0x7e)
                {
                        if (c_char != 0x00)
                        { 
                                c_char = 0x2e;
                                buffer[i] = c_char & 0xff;
                        }
                        else
                        {
                                i--;
                                str_length--;
                        }
                }
                else
                {
                        buffer[i] = c_char & 0xff;
                }
                foffset++;
                length_remaining--;
                
                if(length_remaining==1)
                {
                	i++;
                	break;
                }        
        }
        buffer[i] = '\0';
        
        if (little) {
            str_length = tvb_get_letohl(tvb, offset);
        }
        else
        {
            str_length = tvb_get_ntohl(tvb, offset);
        }
        proto_tree_add_string(nmas_tree, hfinfo, tvb, offset+4,
                str_length, buffer);
        foffset += align_4(tvb, foffset);
        return foffset;
}

void
dissect_nmas_request(tvbuff_t *tvb, packet_info *pinfo, proto_tree *ncp_tree, ncp_req_hash_value *request_value)
{
	guint8			    func, subfunc = 0;
    guint32             msg_length=0;
    guint32             foffset;
    guint32             subverb=0;
    guint32             attribute=0;
    guint8              msgverb=0;
    proto_tree          *atree;
    proto_item          *aitem;
    
    foffset = 6;
    func = tvb_get_guint8(tvb, foffset);
    foffset += 1;
    subfunc = tvb_get_guint8(tvb, foffset);
    foffset += 1;
    
	/* Fill in the INFO column. */
	if (check_col(pinfo->cinfo, COL_INFO)) {
       col_set_str(pinfo->cinfo, COL_PROTOCOL, "NMAS");
       col_add_fstr(pinfo->cinfo, COL_INFO, "C NMAS - %s", match_strval(subfunc, nmas_func_enum));
    }
    aitem = proto_tree_add_text(ncp_tree, tvb, foffset, tvb_length_remaining(tvb, foffset), "Packet Type: %s", match_strval(subfunc, nmas_func_enum));
    atree = proto_item_add_subtree(aitem, ett_nmas);
    switch (subfunc) {
    case 1:
        proto_tree_add_item(atree, hf_ping_version, tvb, foffset, 4, TRUE);
        foffset += 4;
        proto_tree_add_item(atree, hf_ping_flags, tvb, foffset, 4, TRUE);
        foffset += 4;
        break;
    case 2:
        proto_tree_add_item(atree, hf_frag_handle, tvb, foffset, 4, TRUE);
        foffset += 4;
        foffset += 4; /* Dont know what this is */
        proto_tree_add_item(atree, hf_length, tvb, foffset, 4, TRUE);
        msg_length = tvb_get_letohl(tvb, foffset);
        foffset += 4;
        foffset += 12;
        msg_length -= 16;
        proto_tree_add_item(atree, hf_subverb, tvb, foffset, 4, TRUE);
        subverb = tvb_get_letohl(tvb, foffset);
        if (request_value) {
            request_value->req_nds_flags=subverb;
        }
        foffset += 4;
        msg_length -= 4;
        if (check_col(pinfo->cinfo, COL_INFO)) {
            col_append_fstr(pinfo->cinfo, COL_INFO, ", %s", match_strval(subverb, nmas_subverb_enum));
        }
        switch (subverb) {
        case 0:             /* Fragmented Ping */
            proto_tree_add_item(atree, hf_ping_version, tvb, foffset, 4, TRUE);
            foffset += 4;
            proto_tree_add_item(atree, hf_ping_flags, tvb, foffset, 4, TRUE);
            foffset += 4;
            break;
        case 2:             /* Client Put Data */
            proto_tree_add_item(atree, hf_opaque, tvb, foffset, msg_length, FALSE);
            foffset += msg_length;
            break;
        case 4:             /* Client Get Data */
        case 6:             /* Client Get User NDS Credentials */
            /* No Op */
            break;
        case 8:             /* Login Store Management */
            msgverb = tvb_get_guint8(tvb, foffset); 
            if (request_value) {
                request_value->nds_request_verb=msgverb; /* Use nds_request_verb for passed subverb */
            }
            proto_tree_add_item(atree, hf_lsm_verb, tvb, foffset, 1, TRUE);
            foffset += 4;
            if (check_col(pinfo->cinfo, COL_INFO)) {
                col_append_fstr(pinfo->cinfo, COL_INFO, ", %s", match_strval(msgverb, nmas_lsmverb_enum));
            }
            switch (msgverb)
            {
            case 1:
                break;
            case 2:
                break;
            case 4:
                break;
            case 5:
                break;
            case 6:
                break;
            default:
                break;
            }
            break;
        case 10:            /* Writable Object Check */
            /* The first GUINT32 value is the len of the header? */
            foffset += 4; 
            /* The next two GUINT32 values are reserved and always 0 */
            foffset += 8;
            foffset = nmas_string(tvb, hf_tree, atree, foffset, TRUE);
            foffset = nmas_string(tvb, hf_user, atree, foffset, TRUE);
            break;
        case 1242:          /* Message Handler */
            foffset += 4;
            proto_tree_add_item(atree, hf_msg_version, tvb, foffset, 4, FALSE);
            foffset += 4;
            proto_tree_add_item(atree, hf_session_ident, tvb, foffset, 4, FALSE);
            foffset += 4;
            foffset += 3;
            msgverb = tvb_get_guint8(tvb, foffset);
            if (request_value) {
                request_value->nds_request_verb=msgverb; /* Use nds_request_verb for passed subverb */
            }
            proto_tree_add_item(atree, hf_msg_verb, tvb, foffset, 1, FALSE);
            foffset += 1;
            msg_length -= 12;
            if (check_col(pinfo->cinfo, COL_INFO)) {
                col_append_fstr(pinfo->cinfo, COL_INFO, ", %s", match_strval(msgverb, nmas_msgverb_enum));
            }
            switch(msgverb)
            {
            case 1:
                msg_length = tvb_get_ntohl(tvb, foffset);
                proto_tree_add_item(atree, hf_length, tvb, foffset, 4, FALSE);
                foffset += 4;
                proto_tree_add_item(atree, hf_data, tvb, foffset, msg_length, FALSE);
                foffset += msg_length;
                break;
            case 3:
                msg_length = tvb_get_ntohl(tvb, foffset);
                msg_length -= 4;
                proto_tree_add_item(atree, hf_length, tvb, foffset, 4, FALSE);
                foffset += 4;
                while (msg_length > 0)
                {
                    attribute = tvb_get_ntohl(tvb, foffset);
                    foffset += 4;
                    switch (attribute) {
                    case 1:
                        foffset = nmas_string(tvb, hf_user, atree, foffset, FALSE);
                        break;
                    case 2:
                        foffset = nmas_string(tvb, hf_tree, atree, foffset, FALSE);
                        break;
                    case 4:
                        foffset = nmas_string(tvb, hf_clearence, atree, foffset, FALSE);
                        break;
                    case 11:
                        foffset = nmas_string(tvb, hf_login_sequence, atree, foffset, FALSE);
                        break;
                    default:
                        break;
                    }
                    msg_length -= foffset;
                    if (tvb_get_ntohl(tvb, foffset)==0)
                    {
                        break;
                    }
                }
                break;
            case 5:
                proto_tree_add_item(atree, hf_opaque, tvb, foffset, msg_length, FALSE);
                foffset += msg_length;
                break;
            case 7:
            case 9:
                /* No Op */
                break;
            default:
                break;
            }
            break;
        default:
            break;
        }
        break;
    case 3:
        /* No Op */
        break;
    default:
        break;
    }
}

void
dissect_nmas_reply(tvbuff_t *tvb, packet_info *pinfo, proto_tree *ncp_tree, guint8 func _U_, guint8 subfunc, ncp_req_hash_value	*request_value)
{
    guint32             foffset=0;
    guint32             subverb=0;
    guint8              msgverb=0;
    guint32             msg_length=0;
    guint32             return_code=0;
    proto_tree          *atree;
    proto_item          *aitem;
    
    foffset = 8;
    if (request_value) {
        subverb = request_value->req_nds_flags;
        msgverb = request_value->nds_request_verb;
    }
	if (check_col(pinfo->cinfo, COL_INFO)) {
       col_set_str(pinfo->cinfo, COL_PROTOCOL, "NMAS");
    }
    if (tvb_length_remaining(tvb, foffset)<4) {
        return;
    }
        
    aitem = proto_tree_add_text(ncp_tree, tvb, foffset, tvb_length_remaining(tvb, foffset), "Packet Type: %s", match_strval(subfunc, nmas_func_enum));
    atree = proto_item_add_subtree(aitem, ett_nmas);
    switch (subfunc) {
    case 1:
        proto_tree_add_item(atree, hf_ping_flags, tvb, foffset, 4, TRUE);
        foffset += 4;
        proto_tree_add_item(atree, hf_nmas_version, tvb, foffset, 4, TRUE);
        foffset += 4;
        break;
    case 2:
        proto_tree_add_item(atree, hf_length, tvb, foffset, 4, TRUE);
        msg_length = tvb_get_letohl(tvb, foffset);
        return_code = tvb_get_ntohl(tvb, foffset+msg_length);

        foffset += 4;
        proto_tree_add_item(atree, hf_frag_handle, tvb, foffset, 4, TRUE);
        foffset += 4;
        msg_length -= 4;
        proto_tree_add_text(atree, tvb, foffset, tvb_length_remaining(tvb, foffset), "Verb: %s", match_strval(subverb, nmas_subverb_enum));
        switch (subverb) {
        case 0:             /* Fragmented Ping */
            proto_tree_add_item(atree, hf_ping_flags, tvb, foffset, 4, TRUE);
            foffset += 4;
            proto_tree_add_item(atree, hf_nmas_version, tvb, foffset, 4, TRUE);
            foffset += 4;
            break;
        case 2:             /* Client Put Data */
            proto_tree_add_item(atree, hf_squeue_bytes, tvb, foffset, 4, TRUE);
            foffset += 4;
            proto_tree_add_item(atree, hf_cqueue_bytes, tvb, foffset, 4, TRUE);
            foffset += 4;
            break;
        case 4:             /* Client Get Data */
            proto_tree_add_item(atree, hf_opaque, tvb, foffset, msg_length, TRUE);
            foffset += msg_length;
            break;
        case 6:             /* Client Get User NDS Credentials */
            proto_tree_add_item(atree, hf_num_creds, tvb, foffset, 4, TRUE);
            foffset += 4;
            proto_tree_add_item(atree, hf_cred_type, tvb, foffset, 4, TRUE);
            foffset += 4;
            proto_tree_add_item(atree, hf_login_state, tvb, foffset, 4, TRUE);
            foffset += 4;
            msg_length -= 12;
            proto_tree_add_item(atree, hf_enc_cred, tvb, foffset, msg_length, TRUE);
            foffset += msg_length;
            break;
        case 8:             /* Login Store Management */
            proto_tree_add_text(atree, tvb, foffset, tvb_length_remaining(tvb, foffset), "Subverb: %s", match_strval(msgverb, nmas_msgverb_enum));
            switch(msgverb)
            {
                /* The data within these structures is all encrypted. */
            case 1:
            case 3:
            case 5:
            case 7:
            case 9:
                proto_tree_add_item(atree, hf_enc_data, tvb, foffset, msg_length, TRUE);
                foffset += msg_length;
                break;
            default:
                break;
            }
            break;
        case 10:            /* Writable Object Check */
            if (tvb_length_remaining(tvb, foffset) < 8) 
            {
                return_code = tvb_get_letohl(tvb, foffset);
                if (match_strval(return_code, nmas_errors_enum)!=NULL) 
                {
                    proto_tree_add_item(atree, hf_return_code, tvb, foffset, 4, FALSE);
                    if (check_col(pinfo->cinfo, COL_INFO)) {
                       col_add_fstr(pinfo->cinfo, COL_INFO, "R Error - %s", match_strval(return_code, nmas_errors_enum));
                    }
                }
                return;
            }
            proto_tree_add_item(atree, hf_ping_flags, tvb, foffset, 4, TRUE);
            foffset += 4;
            proto_tree_add_item(atree, hf_nmas_version, tvb, foffset, 4, TRUE);
            foffset += 4;
            break;
        case 1242:          /* Message Handler */
            proto_tree_add_text(atree, tvb, foffset, tvb_length_remaining(tvb, foffset), "Subverb: %s", match_strval(msgverb, nmas_msgverb_enum));
            switch(msgverb)
            {
            case 1:
                msg_length = tvb_get_ntohl(tvb, foffset);
                proto_tree_add_item(atree, hf_length, tvb, foffset, 4, FALSE);
                foffset += 4;
                proto_tree_add_item(atree, hf_data, tvb, foffset, msg_length, FALSE);
                foffset += msg_length;
                break;
            case 3:
                proto_tree_add_item(atree, hf_session_ident, tvb, foffset, 4, FALSE);
                foffset += 4;
                break;
            case 5:
                /* No Op */
                break;
            case 7:
                proto_tree_add_item(atree, hf_opaque, tvb, foffset, msg_length, FALSE);
                foffset += msg_length;
                break;
            case 9:
                /* No Op */
                break;
            default:
                break;
            }
            break;
        default:
            break;
        }
        if (match_strval(return_code, nmas_errors_enum)!=NULL) 
        {
            proto_tree_add_item(atree, hf_return_code, tvb, foffset-4, 4, FALSE);
            if (check_col(pinfo->cinfo, COL_INFO)) {
               col_add_fstr(pinfo->cinfo, COL_INFO, "R Error - %s", match_strval(return_code, nmas_errors_enum));
            }
        }
        if (return_code == 0 && msgverb!=7) {
            proto_tree_add_text(atree, tvb, foffset, 4, "Return Code: Success (0x00000000)");
        }
        break;
    case 3:
        break;
    default:
        break;
    }
}

void
proto_register_nmas(void)
{
	static hf_register_info hf_nmas[] = {
		{ &hf_func,
		{ "Function",		"nmas.func", FT_UINT8, BASE_HEX, NULL, 0x0,
			"Function", HFILL }},

		{ &hf_subfunc,
		{ "Subfunction",		"nmas.subfunc", FT_UINT8, BASE_HEX, NULL, 0x0,
			"Subfunction", HFILL }},

		{ &hf_ping_version,
		{ "Ping Version",		"nmas.ping_version", FT_UINT32, BASE_HEX, NULL, 0x0,
			"Ping Version", HFILL }},

		{ &hf_ping_flags,
		{ "Flags",		"nmas.ping_flags", FT_UINT32, BASE_HEX, NULL, 0x0,
			"Flags", HFILL }},

		{ &hf_frag_handle,
		{ "Fragment Handle",		"nmas.frag_handle", FT_UINT32, BASE_HEX, NULL, 0x0,
			"Fragment Handle", HFILL }},

		{ &hf_length,
		{ "Length",		"nmas.length", FT_UINT32, BASE_DEC, NULL, 0x0,
			"Length", HFILL }},

        { &hf_subverb,
        { "Sub Verb",    "nmas.subverb",
          FT_UINT32,    BASE_HEX,   VALS(nmas_subverb_enum),   0x0,
          "Sub Verb", HFILL }},

        { &hf_tree,
        { "Tree",    "nmas.tree",
          FT_STRING,    BASE_NONE,   NULL,   0x0,
          "Tree", HFILL }},

        { &hf_user,
        { "User",    "nmas.user",
          FT_STRING,    BASE_NONE,   NULL,   0x0,
          "User", HFILL }},

		{ &hf_nmas_version,
		{ "NMAS Protocol Version",		"nmas.version", FT_UINT32, BASE_HEX, NULL, 0x0,
			"NMAS Protocol Version", HFILL }},

		{ &hf_msg_version,
		{ "Message Version",		"nmas.msg_version", FT_UINT32, BASE_HEX, NULL, 0x0,
			"Message Version", HFILL }},

		{ &hf_session_ident,
		{ "Session Identifier",		"nmas.session_ident", FT_UINT32, BASE_HEX, NULL, 0x0,
			"Session Identifier", HFILL }},

		{ &hf_msg_verb,
		{ "Message Verb",		"nmas.msg_verb", FT_UINT8, BASE_HEX, VALS(nmas_msgverb_enum), 0x0,
			"Message Verb", HFILL }},
		
        { &hf_attribute,
		{ "Attribute Type",		"nmas.attribute", FT_UINT32, BASE_DEC, VALS(nmas_attribute_enum), 0x0,
			"Attribute Type", HFILL }},

        { &hf_clearence,
        { "Requested Clearence",    "nmas.clearence",
          FT_STRING,    BASE_NONE,   NULL,   0x0,
          "Requested Clearence", HFILL }},

        { &hf_login_sequence,
        { "Requested Login Sequence",    "nmas.login_seq",
          FT_STRING,    BASE_NONE,   NULL,   0x0,
          "Requested Login Sequence", HFILL }},

        { &hf_opaque,
        { "Opaque Data",    "nmas.opaque",
          FT_BYTES,    BASE_NONE,   NULL,   0x0,
          "Opaque Data", HFILL }},

        { &hf_data,
        { "Data",    "nmas.data",
          FT_BYTES,    BASE_NONE,   NULL,   0x0,
          "Data", HFILL }},
        
        { &hf_return_code,
		{ "Return Code",		"nmas.return_code", FT_UINT32, BASE_HEX, VALS(nmas_errors_enum), 0x0,
			"Return Code", HFILL }},
       
		{ &hf_lsm_verb,
		{ "Login Store Message Verb",		"nmas.lsm_verb", FT_UINT8, BASE_HEX, VALS(nmas_lsmverb_enum), 0x0,
			"Login Store Message Verb", HFILL }},

        { &hf_squeue_bytes,
		{ "Server Queue Number of Bytes",		"nmas.squeue_bytes", FT_UINT32, BASE_DEC, NULL, 0x0,
			"Server Queue Number of Bytes", HFILL }},

        { &hf_cqueue_bytes,
		{ "Client Queue Number of Bytes",		"nmas.cqueue_bytes", FT_UINT32, BASE_DEC, NULL, 0x0,
			"Client Queue Number of Bytes", HFILL }},

        { &hf_num_creds,
		{ "Number of Credentials",		"nmas.num_creds", FT_UINT32, BASE_DEC, NULL, 0x0,
			"Number of Credentials", HFILL }},

        { &hf_cred_type,
		{ "Credential Type",		"nmas.cred_type", FT_UINT32, BASE_DEC, NULL, 0x0,
			"Credential Type", HFILL }},
        
        { &hf_login_state,
		{ "Login State",		"nmas.login_state", FT_UINT32, BASE_DEC, NULL, 0x0,
			"Login State", HFILL }},

        { &hf_enc_cred,
        { "Encrypted Credential",    "nmas.enc_cred",
          FT_BYTES,    BASE_NONE,   NULL,   0x0,
          "Encrypted Credential", HFILL }},

        { &hf_enc_data,
        { "Encrypted Data",    "nmas.enc_data",
          FT_BYTES,    BASE_NONE,   NULL,   0x0,
          "Encrypted Data", HFILL }},
    
    
    };

	static gint *ett[] = {
		&ett_nmas,
	};
	
	proto_nmas = proto_register_protocol("Novell Modular Authentication Service", "NMAS", "nmas");
	proto_register_field_array(proto_nmas, hf_nmas, array_length(hf_nmas));
	proto_register_subtree_array(ett, array_length(ett));
}
