/* packet-ses.c
*
* Routine to dissect ISO 8327-1 OSI Session Protocol packets
*
* $Id: packet-ses.c,v 1.1 2003/11/11 08:29:33 guy Exp $
*
* Yuriy Sidelnikov <YSidelnikov@hotmail.com>
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

#include <glib.h>
#include <epan/packet.h>

#include <stdio.h>
#include <string.h>

#include "packet-ses.h"
#include "packet-frame.h"
#include "prefs.h"

#include <epan/strutil.h>

/* ses header fields             */
static int proto_ses          = -1;
static int hf_ses_type        = -1;
static int hf_ses_length      = -1;
static int hf_ses_version     = -1;
static int hf_ses_reserved    = -1;

/* ses fields defining a sub tree */
static gint ett_ses           = -1;
static gint ett_ses_param     = -1;

/* 
----------------------------------------------------------------------------------------------------------*/
static dissector_handle_t pres_handle = NULL;
/* 
----------------------------------------------------------------------------------------------------------*/


/* flags */
static int hf_connect_protocol_options_flags = -1;
static int hf_version_number_options_flags = -1;
static int hf_enclosure_item_options_flags = -1;
static int hf_token_item_options_flags = -1;

static gint ett_connect_protocol_options_flags = -1;
static gint ett_protocol_version_flags = -1;
static gint ett_enclosure_item_flags = -1;
static gint ett_token_item_flags = -1;
static gint ett_ses_req_options_flags = -1;

/* called SS user reference */
static int hf_called_ss_user_reference = -1;

/* calling SS user reference */
static int hf_calling_ss_user_reference = -1;

/* common reference */
static int hf_common_reference = -1;

/* additional reference information */
static int hf_additional_reference_information = -1;

/* token item */
static int hf_release_token = -1;
static int hf_major_activity_token = -1;
static int hf_synchronize_minor_token = -1;
static int hf_data_token = -1;

/* protocol options */
static int hf_able_to_receive_extended_concatenated_SPDU = -1;

/* session requirement */
static int hf_session_user_req_flags = -1;
static int hf_session_exception_report= -1;
static int hf_data_separation_function_unit= -1;
static int hf_symmetric_synchronize_function_unit= -1;
static int hf_typed_data_function_unit= -1;
static int hf_exception_function_unit= -1;
static int hf_capability_function_unit=-1;
static int hf_negotiated_release_function_unit= -1;
static int hf_activity_management_function_unit= -1;
static int hf_resynchronize_function_unit= -1;
static int hf_major_resynchronize_function_unit= -1;
static int hf_minor_resynchronize_function_unit= -1;
static int hf_expedited_data_resynchronize_function_unit= -1;
static int hf_duplex_function_unit= -1;
static int hf_half_duplex_function_unit = -1;

/* TSDU maximum size */
static int hf_proposed_tsdu_maximum_size_i2r = -1;
static int hf_proposed_tsdu_maximum_size_r2i = -1;

/* protocol version */
static int hf_protocol_version_1 = -1;
static int hf_protocol_version_2 = -1;

/* initial serial number */
static int hf_initial_serial_number = -1;

/* enclosure item */
static int hf_beginning_of_SSDU = -1;
static int hf_end_of_SSDU = -1;

/* token setting item */

static const value_string token_setting_vals[] = {
	{ 0x00, "initiator's side" },
	{ 0x01, "responder's side" },
	{ 0x02, "called SS user's choice" },
	{ 0x03, "reserved" },
	{ 0, NULL }
};
	
static int hf_release_token_setting = -1;
static int hf_major_activity_token_setting = -1;
static int hf_synchronize_minor_token_setting = -1;
static int hf_data_token_setting = -1;

/* calling session selector */
static int hf_calling_session_selector = -1;

/* called session selector */
static int hf_called_session_selector = -1;

/* serial number */
static int hf_serial_number = -1;

/* second serial number */
static int hf_second_serial_number = -1;

/* second initial serial number */
static int hf_second_initial_serial_number = -1;

/* large initial serial number */
static int hf_large_initial_serial_number = -1;

/* large second initial serial number */
static int hf_large_second_initial_serial_number = -1;

static const value_string ses_vals[] =
{
  {SES_CONNECTION_REQUEST,  "Connection request PDU" },
  {SES_CONNECTION_ACCEPT,    "Connection accept PDU"   },
  {SES_EXCEPTION_REPORT,    "Exception report PDU"   },
  {SES_DATA_TRANSFER,  "Data transfer PDU" },
  {SES_PLEASE_TOKENS,    "Please tokens PDU"   },
  {SES_EXPEDITED,    "Expedited PDU"   },
  {SES_PREPARE,    "Prepare PDU"   },
  {SES_NOT_FINISHED,    "Not finished PDU"   },
  {SES_FINISH,    "Finish PDU"   },
  {SES_DISCONNECT,    "Disconnect PDU"   },
  {SES_REFUSE,    "Refuse PDU"   },
  {SES_CONNECTION_DATA_OVERFLOW,    "Data overflow PDU"   },
  {SES_OVERFLOW_ACCEPT,    "Overflow accept PDU"   },
  {SES_GIVE_TOKENS_CONFIRM,    "Tokens confirm PDU"   },
  {SES_GIVE_TOKENS_ACK,    "Give tokens ACK PDU"   },
  {SES_ABORT,    "Abort PDU"   },
  {SES_ABORT_ACCEPT,    "Abort accept PDU"   },
  {SES_ACTIVITY_RESUME,    "Activity resume PDU"   },
  {SES_TYPED_DATA,    "Typed data PDU"   },
  {SES_RESYNCHRONIZE_ACK,    "Resynchronize ACK PDU"   },
  {SES_MAJOR_SYNC_POINT,    "Session major sync point PDU"   },
  {SES_MAJOR_SYNC_ACK,    "Session major sync ACK PDU"   },
  {SES_ACTIVITY_START,    "Activity start PDU"   },
  {SES_EXCEPTION_DATA,    "Exception data PDU"   },
  {SES_MINOR_SYNC_POINT,    "Minor sync point PDU"   },
  {SES_MINOR_SYNC_ACK,    "Minor sync ACK PDU"   },
  {SES_RESYNCHRONIZE,    "Resynchronize PDU"   },
  {SES_ACTIVITY_DISCARD,    "Activity discard PDU"   },
  {SES_ACTIVITY_DISCARD_ACK,    "Activity discard ACK PDU"   },
  {SES_CAPABILITY,    "Capability PDU"   },
  {SES_CAPABILITY_DATA_ACK,    "Capability data ACK PDU"   },
  {0,             NULL           }
};

static const value_string param_vals[] =
{
  {Connection_Identifier, "Connection Identifier"},
  {Connect_Accept_Item, "Connect Accept Item"},
  {Called_SS_user_Reference, "Called SS user Reference"},
  {Calling_SS_user_Reference, "Calling SS user Reference"},
  {Common_Reference, "Common Reference"},
  {Sync_Type_Item, "Sync Type Item"},
  {Token_Item, "Token Item"},
  {Transport_Disconnect, "Transport_Disconnect"},
  {Additional_Reference_Information, "Additional Reference Information"},
  {Protocol_Options, "Protocol Options"},
  {TSDU_Maximum_Size, "TSDU Maximum Size"},
  {Version_Number, "Version Number"},
  {Initial_Serial_Number, "Initial Serial Number"},
  {Prepare_Type, "Prepare Type"},
  {EnclosureItem, "Enclosure Item"},
  {Token_Setting_Item, "Token Setting Item"},
  {Resync_Type, "Resync Type"},
  {Serial_Number, "Serial Number"},
  {Linking_Information, "Linking Information"},
  {Reflect_Parameter, "Reflect Parameter"},
  {Reason_Code, "Reason Code"},
  {Calling_Session_Selector, "Calling Session Selector"},
  {Called_Session_Selector, "Called Session Selector"},
  {Second_Resync_Type, "Second Resync Type"},
  {Second_Serial_Number, "Second Serial Number"},
  {Second_Initial_Serial_Number, "Second Initial Serial Number"},
  {Upper_Limit_Serial_Number, "Upper Limit Serial Number"},
  {Large_Initial_Serial_Number, "Large Initial Serial Number"},
  {Large_Second_Initial_Serial_Number, "Large Second Initial Serial Number"},
  {Data_Overflow, "Data Overflow"},
  {Session_Requirement, "Session Requirement"},
  {User_Data, "Session user data"},
  {Extended_User_Data, "Session extended user data"},
  {0, NULL}
};

static const value_string reason_vals[] =
{
  {reason_not_specified,  "Rejection by called SS-user; reason not specified" },
  {temporary_congestion,    "Rejection by called SS-user due to temporary congestion"   },
  {Subsequent,    "Rejection by called SS-user."   },
  {Session_Selector_unknown,  "Session Selector unknown" },
  {SS_user_not_attached_to_SSAP,    "SS-user not attached to SSAP"   },
  {SPM_congestion_at_connect_time,    "SPM congestion at connect time"   },
  {versions_not_supported,    "Proposed protocol versions not supported"   },
  {SPM_reason_not_specified,    "Rejection by the SPM; reason not specified"   },
  {SPM_implementation_restriction,    "Finish PDU"   },
  {SES_DISCONNECT,    "Rejection by the SPM; implementation restriction stated in the PICS"   },
  {0,             NULL           }
};

/* desegmentation of OSI over ses  */
/*static gboolean ses_desegment = TRUE;*/


/* find the dissector for data */
static dissector_handle_t data_handle;

static void
call_pres_dissector(tvbuff_t *tvb, int offset, guint16 param_len,
    packet_info *pinfo, proto_tree *tree, proto_tree *param_tree)
{
	/* do we have OSI presentation packet dissector ? */
	if(!pres_handle)
	{
		/* No - display as data */
		if (tree)
		{
			proto_tree_add_text(param_tree, tvb, offset, param_len,
			    "User data");
		}
		else
		{
			/* Yes - call presentation dissector */
			tvbuff_t *next_tvb;

			next_tvb = tvb_new_subset(tvb, offset, param_len,
			    param_len);
			TRY
			{
				call_dissector(pres_handle, next_tvb, pinfo,
				    tree);
			}
			CATCH_ALL
			{
				show_exception(tvb, pinfo, tree, EXCEPT_CODE);
			}
			ENDTRY;
		}
	}
}

/* this routine returns length of parameter field, parameter group,
   or parameter */
static int
get_item_len(tvbuff_t *tvb, int offset, int *len_len)
{
	guint16 len;

	len = tvb_get_guint8(tvb, offset);
	if(len == TWO_BYTE_LEN)
	{
		len = tvb_get_ntohs(tvb, offset+1);
		*len_len = 3;
	}
	else
		*len_len = 1;
	return len;
}

static void
dissect_parameter(tvbuff_t *tvb, int offset, proto_tree *tree,
    proto_tree *param_tree, packet_info *pinfo, guint8 param_type,
    guint16 param_len)
{
	switch (param_type)
	{
	case Called_SS_user_Reference:
		if (tree)
		{
			proto_tree_add_item(param_tree,
			    hf_called_ss_user_reference,
			    tvb, offset, param_len, FALSE);
		}
		break;

	case Calling_SS_user_Reference:
		if (tree)
		{
			proto_tree_add_item(param_tree,
			    hf_calling_ss_user_reference,
			    tvb, offset, param_len, FALSE);
		}
		break;

	case Common_Reference:
		if (tree)
		{
			proto_tree_add_item(param_tree,
			    hf_common_reference,
			    tvb, offset, param_len, FALSE);
		}
		break;

	case Additional_Reference_Information:
		if (tree)
		{
			proto_tree_add_item(param_tree,
			    hf_additional_reference_information,
			    tvb, offset, param_len, FALSE);
		}
		break;

	case Token_Item:
		if (tree)
		{
			guint8       flags;
			proto_item   *tf;
			proto_tree   *flags_tree;

			flags = tvb_get_guint8(tvb, offset);
			tf = proto_tree_add_uint(param_tree,
			    hf_token_item_options_flags, tvb, offset, 1,
			    flags);
			flags_tree = proto_item_add_subtree(tf,
			    ett_token_item_flags);
			proto_tree_add_boolean(flags_tree, hf_release_token,
			    tvb, offset, 1, flags);
			proto_tree_add_boolean(flags_tree,
			    hf_major_activity_token, tvb, offset, 1, flags);
			proto_tree_add_boolean(flags_tree,
			    hf_synchronize_minor_token, tvb, offset, 1, flags);
			proto_tree_add_boolean(flags_tree, hf_data_token, tvb,
			    offset, 1, flags);
		}
		break;

	case Transport_Disconnect:
		if (tree)
		{
			guint8       flags;

			flags = tvb_get_guint8(tvb, offset);
			if(flags & transport_connection_is_released )
			{
				proto_tree_add_text(param_tree, tvb, offset, 1,
				    "transport connection is released");
			}
			else
			{
				proto_tree_add_text(param_tree, tvb, offset, 1,
				    "transport connection is kept");
			}

			if(flags & user_abort )
			{
				proto_tree_add_text(param_tree, tvb, offset, 1,
				    "user abort");
			}

			if(flags & protocol_error )
			{
				proto_tree_add_text(param_tree, tvb, offset, 1,
				    "protocol error");
			}

			if(flags & no_reason )
			{
				proto_tree_add_text(param_tree, tvb, offset, 1,
				    "no reason");
			}

			if(flags & implementation_restriction )
			{
				proto_tree_add_text(param_tree, tvb, offset, 1,
				    "implementation restriction");
			}
		}
		break;

	case Protocol_Options:
		if (tree)
		{
			guint8       flags;
			proto_item   *tf;
			proto_tree   *flags_tree;

			flags = tvb_get_guint8(tvb, offset);
			tf = proto_tree_add_uint(param_tree,
			    hf_connect_protocol_options_flags, tvb, offset, 1,
			    flags);
			flags_tree = proto_item_add_subtree(tf, 
			    ett_connect_protocol_options_flags);
			proto_tree_add_boolean(flags_tree, 
			    hf_able_to_receive_extended_concatenated_SPDU,
			    tvb, offset, 1, flags);
		}
		break;

	case Session_Requirement:
		if (tree)
		{
			guint16       flags;
			proto_item   *tf;
			proto_tree   *flags_tree;

			flags = tvb_get_ntohs(tvb, offset);
			tf = proto_tree_add_uint(param_tree,
			    hf_session_user_req_flags, tvb, offset, 2,
			    flags);
			flags_tree = proto_item_add_subtree(tf,
			    ett_ses_req_options_flags);
			proto_tree_add_boolean(flags_tree,
			    hf_session_exception_report, tvb, offset, 2, flags);
			proto_tree_add_boolean(flags_tree,
			    hf_data_separation_function_unit, tvb, offset, 2,
			    flags);
			proto_tree_add_boolean(flags_tree,
			    hf_symmetric_synchronize_function_unit, 
			    tvb, offset, 2, flags);
			proto_tree_add_boolean(flags_tree,
			    hf_typed_data_function_unit, tvb, offset, 2, flags);
			proto_tree_add_boolean(flags_tree,
			    hf_exception_function_unit, tvb, offset, 2, flags);
			proto_tree_add_boolean(flags_tree,
			    hf_capability_function_unit, tvb, offset, 2, flags);
			proto_tree_add_boolean(flags_tree,
			    hf_negotiated_release_function_unit, 
			    tvb, offset, 2, flags);
			proto_tree_add_boolean(flags_tree,
			    hf_activity_management_function_unit, 
			    tvb, offset, 2, flags);
			proto_tree_add_boolean(flags_tree,
			    hf_resynchronize_function_unit, tvb, offset, 2,
			    flags);
			proto_tree_add_boolean(flags_tree,
			    hf_major_resynchronize_function_unit, 
			    tvb, offset, 2, flags);
			proto_tree_add_boolean(flags_tree,
			    hf_minor_resynchronize_function_unit, 
			    tvb, offset, 2, flags);
			proto_tree_add_boolean(flags_tree, 
			    hf_expedited_data_resynchronize_function_unit,
			    tvb, offset, 2, flags);
			proto_tree_add_boolean(flags_tree,
			    hf_duplex_function_unit, tvb, offset, 2, flags);
			proto_tree_add_boolean(flags_tree,
			    hf_half_duplex_function_unit,
			    tvb, offset, 2, flags);
		}
		break;

	case TSDU_Maximum_Size:
		if (tree)
		{
			proto_tree_add_item(param_tree,
			    hf_proposed_tsdu_maximum_size_i2r,
			    tvb, offset, 2, FALSE);
			proto_tree_add_item(param_tree,
			    hf_proposed_tsdu_maximum_size_r2i,
			    tvb, offset+2, 2, FALSE);
		}
		break;

	case Version_Number:
		if (tree)
		{
			guint8       flags;
			proto_item   *tf;
			proto_tree   *flags_tree;

			flags = tvb_get_guint8(tvb, offset);
			tf = proto_tree_add_uint(param_tree,
			    hf_version_number_options_flags, tvb, offset, 1,
			    flags);
			flags_tree = proto_item_add_subtree(tf,
			    ett_protocol_version_flags);
			proto_tree_add_boolean(flags_tree,
			    hf_protocol_version_2, tvb, offset, 1, flags);
			proto_tree_add_boolean(flags_tree,
			    hf_protocol_version_1, tvb, offset, 1, flags);
		}
		break;

	case Initial_Serial_Number:
		if (tree)
		{
			proto_tree_add_item(param_tree,
			    hf_initial_serial_number,
			    tvb, offset, param_len, FALSE);
		}
		break;

	case EnclosureItem:
		if (tree)
		{
			guint8       flags;
			proto_item   *tf;
			proto_tree   *flags_tree;

			flags = tvb_get_guint8(tvb, offset);
			tf = proto_tree_add_uint(param_tree,
			    hf_enclosure_item_options_flags, tvb, offset, 1,
			    flags);
			flags_tree = proto_item_add_subtree(tf,
			    ett_enclosure_item_flags);
			proto_tree_add_boolean(flags_tree, hf_end_of_SSDU,
			    tvb, offset, 1, flags);
			proto_tree_add_boolean(flags_tree, hf_beginning_of_SSDU,
			    tvb, offset, 1, flags);
		}
		break;

	case Token_Setting_Item:
		if (tree)
		{
			proto_tree_add_item(param_tree,
			    hf_release_token_setting,
			    tvb, offset, 1, FALSE);
			proto_tree_add_item(param_tree,
			    hf_major_activity_token_setting,
			    tvb, offset, 1, FALSE);
			proto_tree_add_item(param_tree,
			    hf_synchronize_minor_token_setting,
			    tvb, offset, 1, FALSE);
			proto_tree_add_item(param_tree,
			    hf_data_token_setting,
			    tvb, offset, 1, FALSE);
		}
		break;

	case Reason_Code:
/*
	0:	Rejection by called SS-user; reason not specified.
	1:	Rejection by called SS-user due to temporary congestion.
	2:	Rejection by called SS-user. Subsequent octets may be used for user data 
up to a length of 512 octets if Protocol Version 1 has been selected, and up 
to a length such that the total length (including SI and LI)  of the SPDU 
does not exceed 65 539 octets if Protocol Version 2 has been selected.
	128 + 1:	Session Selector unknown.
	128 + 2:	SS-user not attached to SSAP.
	128 + 3:	SPM congestion at connect time.
	128 + 4:	Proposed protocol versions not supported.
	128 + 5:	Rejection by the SPM; reason not specified.
	128 + 6:	Rejection by the SPM; implementation restriction stated in the 
PICS.    */
		if (tree)
		{
			guint8      reason_code;

			reason_code = tvb_get_guint8(tvb, offset);
			proto_tree_add_text(param_tree, tvb, offset, 1,
			    "Reason Code: %s",
			    val_to_str(reason_code, reason_vals, "Unknown (%u)"));
		}
		offset++;
		param_len--;
		if (param_len != 0)
		{
			call_pres_dissector(tvb, offset, param_len,
			    pinfo, tree, param_tree);
		}
		break;

	case Calling_Session_Selector:
		if (tree)
		{
			proto_tree_add_item(param_tree,
			    hf_calling_session_selector,
			    tvb, offset, param_len, FALSE);
		}
		break;

	case Called_Session_Selector:
		if (tree)
		{
			proto_tree_add_item(param_tree,
			    hf_called_session_selector,
			    tvb, offset, param_len, FALSE);
		}
		break;

	case Second_Serial_Number:
		if (tree)
		{
			proto_tree_add_item(param_tree,
			    hf_second_serial_number,
			    tvb, offset, param_len, FALSE);
		}
		break;

	case Second_Initial_Serial_Number:
		if (tree)
		{
			proto_tree_add_item(param_tree,
			    hf_second_initial_serial_number,
			    tvb, offset, param_len, FALSE);
		}
		break;

	case Large_Initial_Serial_Number:
		if (tree)
		{
			proto_tree_add_item(param_tree,
			    hf_large_initial_serial_number,
			    tvb, offset, param_len, FALSE);
		}
		break;

	case Large_Second_Initial_Serial_Number:
		if (tree)
		{
			proto_tree_add_item(param_tree,
			    hf_large_second_initial_serial_number,
			    tvb, offset, param_len, FALSE);
		}
		break;

	default:
		break;
	}
}

static void
dissect_parameter_group(tvbuff_t *tvb, int offset, proto_tree *tree,
    proto_tree *pg_tree, packet_info *pinfo, guint16 pg_len)
{
	proto_item *ti;
	proto_tree *param_tree;
	guint8 param_type;
	char *param_str;
	int len_len;
	guint16 param_len;

	while(pg_len != 0)
	{
		param_type = tvb_get_guint8(tvb, offset);
		ti = proto_tree_add_text(pg_tree, tvb, offset, -1,
		    val_to_str(param_type, param_vals,
		      "Unknown parameter type (0x%02x)"));
		param_tree = proto_item_add_subtree(ti, ett_ses_param);
		param_str = match_strval(param_type, param_vals);
		proto_tree_add_text(param_tree, tvb, offset, 1,
		    "Parameter type: %s",
		    param_str != NULL ? param_str : "Unknown");
		offset++;
		pg_len--;
		param_len = get_item_len(tvb, offset, &len_len);
		if (len_len > pg_len) {
			proto_item_set_len(ti, pg_len + 1);
			proto_tree_add_text(param_tree, tvb, offset, pg_len,
			    "Parameter length doesn't fit in parameter");
			return;
		}
		pg_len -= len_len;
		if (param_len > pg_len) {
			proto_item_set_len(ti, pg_len + 1 + len_len);
			proto_tree_add_text(param_tree, tvb, offset, pg_len,
			    "Parameter length: %u, should be <= %u",
			    param_len, pg_len);
			return;
		}
		proto_item_set_len(ti, 1 + len_len + param_len);
		proto_tree_add_text(param_tree, tvb, offset, len_len,
		    "Parameter length: %u", param_len);
		offset += len_len;
		    
		if (param_str != NULL)
		{
			switch(param_type)
			{
			/* PG's in PG's are invalid, presumably */
			case Extended_User_Data:
			case User_Data:
			case Connect_Accept_Item:
			case Connection_Identifier:
			case Linking_Information:
				proto_tree_add_text(param_tree, tvb, offset,
				    param_len,
				    "Parameter group inside parameter group");
				break;

			default:
				dissect_parameter(tvb, offset, tree,
				    param_tree, pinfo, param_type, param_len);
				break;
			}
		}
		offset += param_len;
		pg_len -= param_len;
	}
}

static void
dissect_parameters(tvbuff_t *tvb, int offset, guint16 len, proto_tree *tree,
    proto_tree *ses_tree, packet_info *pinfo)
{
	proto_item *ti;
	proto_tree *param_tree;
	guint8 param_type;
	char *param_str;
	int len_len;
	guint16 param_len;

	while(len != 0)
	{
		param_type = tvb_get_guint8(tvb, offset);
		ti = proto_tree_add_text(ses_tree, tvb, offset, -1,
		    val_to_str(param_type, param_vals,
		      "Unknown parameter type (0x%02x)"));
		param_tree = proto_item_add_subtree(ti, ett_ses_param);
		param_str = match_strval(param_type, param_vals);
		proto_tree_add_text(param_tree, tvb, offset, 1,
		    "Parameter type: %s",
		    param_str != NULL ? param_str : "Unknown");
		offset++;
		len--;
		param_len = get_item_len(tvb, offset, &len_len);
		if (len_len > len) {
			proto_item_set_len(ti, len + 1 );
			proto_tree_add_text(param_tree, tvb, offset, len,
			    "Parameter length doesn't fit in parameter");
			return;
		}
		len -= len_len;
		if (param_len > len) {
			proto_item_set_len(ti, len + 1 + len_len);
			proto_tree_add_text(param_tree, tvb, offset, len,
			    "Parameter length: %u, should be <= %u",
			    param_len, len);
			return;
		}
		proto_item_set_len(ti, 1 + len_len + param_len);
		proto_tree_add_text(param_tree, tvb, offset, len_len,
		    "Parameter length: %u", param_len);
		offset += len_len;
		    
		if (param_str != NULL)
		{
			switch(param_type)
			{
			case Extended_User_Data:
				break;

			case User_Data:
				call_pres_dissector(tvb, offset, param_len,
				    pinfo, tree, param_tree);
				break;

			/* handle PGI's  */
			case Connect_Accept_Item:
			case Connection_Identifier:
			case Linking_Information:
				/* Yes. */
				dissect_parameter_group(tvb, offset, tree,
				    param_tree, pinfo, param_len);
				break;

			/* everything else is a PI */
			default:
				dissect_parameter(tvb, offset, tree,
				    param_tree, pinfo, param_type, param_len);
				break;
			}
		}
		offset += param_len;
		len -= param_len;
	}
}

/*
* Dissect ses-encapsulated data in a SES stream.
*/

static void
dissect_ses(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	int offset = 0;
	guint8 type;
	proto_item *ti = NULL;
	proto_tree *ses_tree = NULL;
	int len_len;
	guint16 parameters_len;
	tvbuff_t *next_tvb;

	if (check_col(pinfo->cinfo, COL_PROTOCOL))
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "SES");
  	if (check_col(pinfo->cinfo, COL_INFO))
  		col_clear(pinfo->cinfo, COL_INFO);

/*   get SPDU type */
	type = tvb_get_guint8(tvb, offset);
  	if (check_col(pinfo->cinfo, COL_INFO))
		col_add_str(pinfo->cinfo, COL_INFO,
		    val_to_str(type, ses_vals, "Unknown SPDU type (0x%02x)"));

	if (tree)
	{
		ti = proto_tree_add_item(tree, proto_ses, tvb, offset, -1,
		    FALSE);
		ses_tree = proto_item_add_subtree(ti, ett_ses);
		proto_tree_add_uint(ses_tree, hf_ses_type, tvb, offset, 1, type);
	}
	offset++;

	/* get length of SPDU parameter field */
	parameters_len = get_item_len(tvb, offset, &len_len);
	if (tree)
		proto_tree_add_uint(ses_tree, hf_ses_length, tvb, offset,
		    len_len, parameters_len);
	offset += len_len;

	/* Dissect parameters. */
	dissect_parameters(tvb, offset, parameters_len, tree, ses_tree, pinfo);
	offset += parameters_len;
	
	/* Dissect user information */
	if (tvb_reported_length_remaining(tvb, offset) > 0) {
		proto_item_set_end(ti, tvb, offset);
		next_tvb = tvb_new_subset(tvb, offset, -1, -1);

		/* do we have OSI presentation packet dissector ? */
		if(!pres_handle)
		{
			call_dissector(data_handle, next_tvb, pinfo, tree);
		}
		else
		{
			call_dissector(pres_handle, next_tvb, pinfo, tree);
		}
	}
}

void
proto_register_ses(void)
{
	static hf_register_info hf[] =
	{
		{
			&hf_ses_type,
			{
				"SPDU Type",
				"ses.type",
				FT_UINT8,
				BASE_DEC,
				VALS(ses_vals),
				0x0,
				"", HFILL
			}
		},
		{
			&hf_ses_length,
			{
				"Length",
				"ses.length",
				FT_UINT16,
				BASE_DEC,
				NULL,
				0x0,
				"", HFILL
			}
		},

		{
			&hf_ses_version,
			{
				"Version",
				"ses.version",
				FT_UINT8,
				BASE_DEC,
				NULL,
				0x0,
				"", HFILL
			}
		},
		{
			&hf_ses_reserved,
			{
				"Reserved",
				"ses.reserved",
				FT_UINT8,
				BASE_DEC,
				NULL,
				0x0,
				"", HFILL
			}
		},
		{
			&hf_called_ss_user_reference,
			{
				"Called SS User Reference",
				"ses.called_ss_user_reference",
				FT_BYTES, BASE_NONE,
				NULL,
				0x0,
				"Called SS User Reference",
				HFILL
			}
		},
		{
			&hf_calling_ss_user_reference,
			{
				"Calling SS User Reference",
				"ses.calling_ss_user_reference",
				FT_BYTES, BASE_NONE,
				NULL,
				0x0,
				"Calling SS User Reference",
				HFILL
			}
		},
		{
			&hf_common_reference,
			{
				"Common Reference",
				"ses.common_reference",
				FT_BYTES, BASE_NONE,
				NULL,
				0x0,
				"Common Reference",
				HFILL
			}
		},
		{
			&hf_additional_reference_information,
			{
				"Additional Reference Information",
				"ses.additional_reference_information",
				FT_BYTES, BASE_NONE,
				NULL,
				0x0,
				"Additional Reference Information",
				HFILL
			}
		},
		{
			&hf_release_token,
			{
				"release token",
				"ses.release_token",
				FT_BOOLEAN, 8,
				NULL,
				RELEASE_TOKEN,
				"release token",
				HFILL
			}
		},
		{
			&hf_major_activity_token,
			{
				"major/activity token",
				"ses.major.token",
				FT_BOOLEAN, 8,
				NULL,
				MAJOR_ACTIVITY_TOKEN,
				"major/activity token",
				HFILL
			}
		},
		{
			&hf_synchronize_minor_token,
			{
				"synchronize minor token",
				"ses.synchronize_token",
				FT_BOOLEAN, 8,
				NULL,
				SYNCHRONIZE_MINOR_TOKEN,
				"synchronize minor token",
				HFILL
			}
		},
		{
			&hf_data_token,
			{
				"data token",
				"ses.data_token",
				FT_BOOLEAN, 8,
				NULL,
				DATA_TOKEN,
				"data  token",
				HFILL
			}
		},
		{
			&hf_able_to_receive_extended_concatenated_SPDU,
			{
				"Able to receive extended concatenated SPDU",
				"ses.connect.f1",
				FT_BOOLEAN, 8,
				NULL,
				SES_EXT_CONT,
				"Able to receive extended concatenated SPDU",
				HFILL
			}
		},
		{
			&hf_session_user_req_flags,
			{
				"Flags",
				"ses.req.flags",
				FT_UINT16,
				BASE_HEX,
				NULL,
				0x0,
				"",
				HFILL
			}
		},
		{
			&hf_session_exception_report,
			{
				"Session exception report",
				"ses.exception_report.",
				FT_BOOLEAN, 16,
				NULL,
				SES_EXCEPTION_REPORT,
				"Session exception report",
				HFILL
			}
		},
		{
			&hf_data_separation_function_unit,
			{
				"Data separation function unit",
				"ses.data_sep",
				FT_BOOLEAN, 16,
				NULL,
				DATA_SEPARATION_FUNCTION_UNIT,
				"Data separation function unit",
				HFILL
			}
		},
		{
			&hf_symmetric_synchronize_function_unit,
			{
				"Symmetric synchronize function unit",
				"ses.symm_sync",
				FT_BOOLEAN, 16,
				NULL,
				SYMMETRIC_SYNCHRONIZE_FUNCTION_UNIT,
				"Symmetric synchronize function unit",
				HFILL
			}
		},
		{
			&hf_typed_data_function_unit,
			{
				"Typed data function unit",
				"ses.typed_data",
				FT_BOOLEAN, 16,
				NULL,
				TYPED_DATA_FUNCTION_UNIT,
				"Typed data function unit",
				HFILL
			}
		},
		{
			&hf_exception_function_unit,
			{
				"Exception function unit",
				"ses.exception_data",
				FT_BOOLEAN, 16,
				NULL,
				EXCEPTION_FUNCTION_UNIT,
				"Exception function unit",
				HFILL
			}
		},
		{
			&hf_capability_function_unit,
			{
				"Capability function unit",
				"ses.capability_data",
				FT_BOOLEAN, 16,
				NULL,
				CAPABILITY_DATA_FUNCTION_UNIT,
				"Capability function unit",
				HFILL
			}
		},
		{
			&hf_negotiated_release_function_unit,
			{
				"Negotiated release function unit",
				"ses.negotiated_release",
				FT_BOOLEAN, 16,
				NULL,
				NEGOTIATED_RELEASE_FUNCTION_UNIT,
				"Negotiated release function unit",
				HFILL
			}
		},
		{
			&hf_activity_management_function_unit,
			{
				"Activity management function unit",
				"ses.activity_management",
				FT_BOOLEAN, 16,
				NULL,
				ACTIVITY_MANAGEMENT_FUNCTION_UNIT,
				"Activity management function unit",
				HFILL
			}
		},
		{
			&hf_resynchronize_function_unit,
			{
				"Resynchronize function unit",
				"ses.resynchronize",
				FT_BOOLEAN, 16,
				NULL,
				RESYNCHRONIZE_FUNCTION_UNIT,
				"Resynchronize function unit",
				HFILL
			}
		},
		{
			&hf_major_resynchronize_function_unit,
			{
				"Major resynchronize function unit",
				"ses.major_resynchronize",
				FT_BOOLEAN, 16,
				NULL,
				MAJOR_SYNCHRONIZE_FUNCTION_UNIT,
				"Major resynchronize function unit",
				HFILL
			}
		},
		{
			&hf_minor_resynchronize_function_unit,
			{
				"Minor resynchronize function unit",
				"ses.minor_resynchronize",
				FT_BOOLEAN, 16,
				NULL,
				MINOR_SYNCHRONIZE_FUNCTION_UNIT,
				"Minor resynchronize function unit",
				HFILL
			}
		},
		{
			&hf_expedited_data_resynchronize_function_unit,
			{
				"Expedited data function unit",
				"ses.expedited_data",
				FT_BOOLEAN, 16,
				NULL,
				EXPEDITED_DATA_FUNCTION_UNIT,
				"Expedited data function unit",
				HFILL
			}
		},
		{
			&hf_duplex_function_unit,
			{
				"Duplex functional unit",
				"ses.duplex",
				FT_BOOLEAN, 16,
				NULL,
				DUPLEX_FUNCTION_UNIT,
				"Duplex functional unit",
				HFILL
			}
		},
		{
			&hf_half_duplex_function_unit,
			{
				"Half-duplex functional unit",
				"ses.half_duplex",
				FT_BOOLEAN, 16,
				NULL,
				HALF_DUPLEX_FUNCTION_UNIT,
				"Half-duplex functional unit",
				HFILL
			}
		},
		{
			&hf_proposed_tsdu_maximum_size_i2r,
			{
				"Proposed TSDU Maximum Size, Initiator to Responder",
				"ses.proposed_tsdu_maximum_size_i2r",
				FT_UINT16,
				BASE_DEC,
				NULL,
				0x0,
				"Proposed TSDU Maximum Size, Initiator to Responder",
				HFILL
			}
		},
		{
			&hf_proposed_tsdu_maximum_size_r2i,
			{
				"Proposed TSDU Maximum Size, Responder to Initiator",
				"ses.proposed_tsdu_maximum_size_r2i",
				FT_UINT16,
				BASE_DEC,
				NULL,
				0x0,
				"Proposed TSDU Maximum Size, Responder to Initiator",
				HFILL
			}
		},
		{
			&hf_protocol_version_1,
			{
				"Protocol Version 1",
				"ses.protocol_version1",
				FT_BOOLEAN, 8,
				NULL,
				PROTOCOL_VERSION_1,
				"Protocol Version 1",
				HFILL
			}
		},
		{
			&hf_protocol_version_2,
			{
				"Protocol Version 2",
				"ses.protocol_version2",
				FT_BOOLEAN, 8,
				NULL,
				PROTOCOL_VERSION_2,
				"Protocol Version 2",
				HFILL
			}
		},
		{
			&hf_initial_serial_number,
			{
				"Initial Serial Number",
				"ses.initial_serial_number",
				FT_STRING, BASE_NONE,
				NULL,
				0x0,
				"Initial Serial Number",
				HFILL
			}
		},
		{
			&hf_beginning_of_SSDU,
			{
				"beginning of SSDU",
				"ses.begininng_of_SSDU",
				FT_BOOLEAN, 8,
				NULL,
				BEGINNING_SPDU,
				"beginning of SSDU",
				HFILL
			}
		},
		{
			&hf_end_of_SSDU,
			{
				"end of SSDU",
				"ses.end_of_SSDU",
				FT_BOOLEAN, 8,
				NULL,
				END_SPDU,
				"end of SSDU",
				HFILL
			}
		},
		{
			&hf_release_token_setting,
			{
				"release token setting",
				"ses.release_token_setting",
				FT_UINT8, BASE_HEX,
				VALS(token_setting_vals),
				0xC0,
				"release token setting",
				HFILL
			}
		},
		{
			&hf_major_activity_token_setting,
			{
				"major/activity setting",
				"ses.major_activity_token_setting",
				FT_UINT8, BASE_HEX,
				VALS(token_setting_vals),
				0x30,
				"major/activity token setting",
				HFILL
			}
		},
		{
			&hf_synchronize_minor_token_setting,
			{
				"synchronize-minor token setting",
				"ses.synchronize_minor_token_setting",
				FT_UINT8, BASE_HEX,
				VALS(token_setting_vals),
				0x0C,
				"synchronize-minor token setting",
				HFILL
			}
		},
		{
			&hf_data_token_setting,
			{
				"data token setting",
				"ses.data_token_setting",
				FT_UINT8, BASE_HEX,
				VALS(token_setting_vals),
				0x03,
				"data token setting",
				HFILL
			}
		},
		{
			&hf_serial_number,
			{
				"Serial Number",
				"ses.serial_number",
				FT_STRING, BASE_NONE,
				NULL,
				0x0,
				"Serial Number",
				HFILL
			}
		},
		{
			&hf_calling_session_selector,
			{
				"Calling Session Selector",
				"ses.calling_session_selector",
				FT_BYTES, BASE_NONE,
				NULL,
				0x0,
				"Calling Session Selector",
				HFILL
			}
		},
		{
			&hf_called_session_selector,
			{
				"Called Session Selector",
				"ses.called_session_selector",
				FT_BYTES, BASE_NONE,
				NULL,
				0x0,
				"Called Session Selector",
				HFILL
			}
		},
		{
			&hf_second_serial_number,
			{
				"Second Serial Number",
				"ses.second_serial_number",
				FT_STRING, BASE_NONE,
				NULL,
				0x0,
				"Second Serial Number",
				HFILL
			}
		},
		{
			&hf_second_initial_serial_number,
			{
				"Second Initial Serial Number",
				"ses.second_initial_serial_number",
				FT_STRING, BASE_NONE,
				NULL,
				0x0,
				"Second Initial Serial Number",
				HFILL
			}
		},
		{
			&hf_large_initial_serial_number,
			{
				"Large Initial Serial Number",
				"ses.large_initial_serial_number",
				FT_STRING, BASE_NONE,
				NULL,
				0x0,
				"Large Initial Serial Number",
				HFILL
			}
		},
		{
			&hf_large_second_initial_serial_number,
			{
				"Large Second Initial Serial Number",
				"ses.large_second_initial_serial_number",
				FT_STRING, BASE_NONE,
				NULL,
				0x0,
				"Large Second Initial Serial Number",
				HFILL
			}
		},
		{
			&hf_connect_protocol_options_flags,
			{
				"Flags",
				"ses.connect.flags",
				FT_UINT8,
				BASE_HEX,
				NULL,
				0x0,
				"",
				HFILL
			}
		},
		{
			&hf_version_number_options_flags,

			{
				"Flags",
				"ses.version.flags",
				FT_UINT8,
				BASE_HEX,
				NULL,
				0x0,
				"",
				HFILL
			}
		},

		{
			&hf_token_item_options_flags,

			{
				"Flags",
				"ses.tken_item.flags",
				FT_UINT8,
				BASE_HEX,
				NULL,
				0x0,
				"",
				HFILL
			}
		},

		{
			&hf_enclosure_item_options_flags,

			{
				"Flags",
				"ses.enclosure.flags",
				FT_UINT8,
				BASE_HEX,
				NULL,
				0x0,
				"",
				HFILL
			}
		},
	};

	static gint *ett[] =
	{
		&ett_ses,
		&ett_ses_param,
		&ett_connect_protocol_options_flags,
		&ett_protocol_version_flags,
		&ett_enclosure_item_flags,
		&ett_token_item_flags,
		&ett_ses_req_options_flags,
	};
	module_t *ses_module;


	proto_ses = proto_register_protocol(PROTO_STRING_SES, "SES", "ses");
	proto_register_field_array(proto_ses, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));

	ses_module = prefs_register_protocol(proto_ses, NULL);
/*
	prefs_register_bool_preference(ses_module, "desegment",
	    "Desegment all session packets ",
	    "Whether the ses dissector should desegment all messages spanning 
multiple SES segments",
	    &ses_desegment);  */

	/*
	 * Register the dissector by name, so other dissectors can
	 * grab it by name rather than just referring to it directly
	 * (you can't refer to it directly from a plugin dissector
	 * on Windows without stuffing it into the Big Transfer Vector).
	 */
	register_dissector("ses", dissect_ses, proto_ses);
}

static gboolean
dissect_ses_heur(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree)
{
	/* must check that this really is a ses packet */
	int offset = 0;
	guint8 type;
	int len_len;
	guint16 len;

	/* first, check do we have at least 4 bytes (type+length) */
	if (!tvb_bytes_exist(tvb, 0, 4))
		return FALSE;	/* no */

	/* can we regognize session PDU ? Return FALSE if  not */
	/*   get SPDU type */
	type = tvb_get_guint8(tvb, offset);
	/* check SPDU type */
	if (match_strval(type, ses_vals) == NULL)
	{
		return FALSE;  /* no, it isn't a session PDU */
	}

	/*  OK,let's check SPDU length  */
	/*  get length of SPDU */
	len = get_item_len(tvb, offset, &len_len);
	/* do we have enough bytes ? */
	if (!tvb_bytes_exist(tvb, 0, len))
		return FALSE;	/* no */

	dissect_ses(tvb, pinfo, parent_tree);
	return TRUE;
}

void
proto_reg_handoff_ses(void)
{
	/*   find data dissector  */
	data_handle = find_dissector("data");

	/* define sub dissector */
	pres_handle = find_dissector("pres");

	/* add our session dissector to cotp dissector list */
	heur_dissector_add("cotp", dissect_ses_heur, proto_ses);
}
