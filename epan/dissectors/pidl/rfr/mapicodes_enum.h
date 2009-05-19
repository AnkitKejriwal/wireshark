/*
   MAPI Implementation

   OpenChange Project

   Copyright (C) Julien Kerihuel 2006

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/* parser auto-generated by mparse */

typedef [public, v1_enum, flag(NDR_PAHEX)] enum {
	MAPI_E_SUCCESS                                      = 0x00000000,
	MAPI_E_NO_SUPPORT                                   = 0x80040102,
	MAPI_E_BAD_CHARWIDTH                                = 0x80040103,
	MAPI_E_STRING_TOO_LONG                              = 0x80040105,
	MAPI_E_UNKNOWN_FLAGS                                = 0x80040106,
	MAPI_E_INVALID_ENTRYID                              = 0x80040107,
	MAPI_E_INVALID_OBJECT                               = 0x80040108,
	MAPI_E_OBJECT_CHANGED                               = 0x80040109,
	MAPI_E_OBJECT_DELETED                               = 0x8004010A,
	MAPI_E_BUSY                                         = 0x8004010B,
	MAPI_E_NOT_ENOUGH_DISK                              = 0x8004010D,
	MAPI_E_NOT_ENOUGH_RESOURCES                         = 0x8004010E,
	MAPI_E_NOT_FOUND                                    = 0x8004010F,
	MAPI_E_VERSION                                      = 0x80040110,
	MAPI_E_LOGON_FAILED                                 = 0x80040111,
	MAPI_E_SESSION_LIMIT                                = 0x80040112,
	MAPI_E_USER_CANCEL                                  = 0x80040113,
	MAPI_E_UNABLE_TO_ABORT                              = 0x80040114,
	MAPI_E_NETWORK_ERROR                                = 0x80040115,
	MAPI_E_DISK_ERROR                                   = 0x80040116,
	MAPI_E_TOO_COMPLEX                                  = 0x80040117,
	MAPI_E_BAD_COLUMN                                   = 0x80040118,
	MAPI_E_EXTENDED_ERROR                               = 0x80040119,
	MAPI_E_COMPUTED                                     = 0x8004011A,
	MAPI_E_CORRUPT_DATA                                 = 0x8004011B,
	MAPI_E_UNCONFIGURED                                 = 0x8004011C,
	MAPI_E_FAILONEPROVIDER                              = 0x8004011D,
	MAPI_E_UNKNOWN_CPID                                 = 0x8004011E,
	MAPI_E_UNKNOWN_LCID                                 = 0x8004011F,
	MAPI_E_PASSWORD_CHANGE_REQUIRED                     = 0x80040120,
	MAPI_E_PASSWORD_EXPIRED                             = 0x80040121,
	MAPI_E_INVALID_WORKSTATION_ACCOUNT                  = 0x80040122,
	MAPI_E_INVALID_ACCESS_TIME                          = 0x80040123,
	MAPI_E_ACCOUNT_DISABLED                             = 0x80040124,
	MAPI_E_END_OF_SESSION                               = 0x80040200,
	MAPI_E_UNKNOWN_ENTRYID                              = 0x80040201,
	MAPI_E_MISSING_REQUIRED_COLUMN                      = 0x80040202,
	MAPI_W_NO_SERVICE                                   = 0x80040203,
	MAPI_E_BAD_VALUE                                    = 0x80040301,
	MAPI_E_INVALID_TYPE                                 = 0x80040302,
	MAPI_E_TYPE_NO_SUPPORT                              = 0x80040303,
	MAPI_E_UNEXPECTED_TYPE                              = 0x80040304,
	MAPI_E_TOO_BIG                                      = 0x80040305,
	MAPI_E_DECLINE_COPY                                 = 0x80040306,
	MAPI_E_UNEXPECTED_ID                                = 0x80040307,
	MAPI_W_ERRORS_RETURNED                              = 0x80040380,
	MAPI_E_UNABLE_TO_COMPLETE                           = 0x80040400,
	MAPI_E_TIMEOUT                                      = 0x80040401,
	MAPI_E_TABLE_EMPTY                                  = 0x80040402,
	MAPI_E_TABLE_TOO_BIG                                = 0x80040403,
	MAPI_E_INVALID_BOOKMARK                             = 0x80040405,
	MAPI_W_POSITION_CHANGED                             = 0x80040481,
	MAPI_W_APPROX_COUNT                                 = 0x80040482,
	MAPI_E_WAIT                                         = 0x80040500,
	MAPI_E_CANCEL                                       = 0x80040501,
	MAPI_E_NOT_ME                                       = 0x80040502,
	MAPI_W_CANCEL_MESSAGE                               = 0x80040580,
	MAPI_E_CORRUPT_STORE                                = 0x80040600,
	MAPI_E_NOT_IN_QUEUE                                 = 0x80040601,
	MAPI_E_NO_SUPPRESS                                  = 0x80040602,
	MAPI_E_COLLISION                                    = 0x80040604,
	MAPI_E_NOT_INITIALIZED                              = 0x80040605,
	MAPI_E_NON_STANDARD                                 = 0x80040606,
	MAPI_E_NO_RECIPIENTS                                = 0x80040607,
	MAPI_E_SUBMITTED                                    = 0x80040608,
	MAPI_E_HAS_FOLDERS                                  = 0x80040609,
	MAPI_E_HAS_MESAGES                                  = 0x8004060A,
	MAPI_E_FOLDER_CYCLE                                 = 0x8004060B,
	MAPI_W_PARTIAL_COMPLETION                           = 0x80040680,
	MAPI_E_AMBIGUOUS_RECIP                              = 0x80040700,
	MAPI_E_RESERVED                                     = 0xFFFFFFFF
} MAPISTATUS;

