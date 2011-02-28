/* packet-csn1.h
 * Declarations and types for CSN1 dissection in wireshark.
 * By Vincent Helfre
 * Copyright (c) 2011 ST-Ericsson
 *
 * $Id$
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
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

#ifndef _PACKET_CSN1_H_
#define _PACKET_CSN1_H_

/* Error codes */
#define  CSN_OK                               0
#define  CSN_ERROR_GENERAL                   -1
#define  CSN_ERROR_DATA_NOT_VALID            -2
#define  CSN_ERROR_IN_SCRIPT                 -3
#define  CSN_ERROR_INVALID_UNION_INDEX       -4
#define  CSN_ERROR_NEED_MORE_BITS_TO_UNPACK  -5
#define  CSN_ERROR_ILLEGAL_BIT_VALUE         -6
#define  CSN_ERROR_INTERNAL                  -7
#define  CSN_ERROR_STREAM_NOT_SUPPORTED      -8
#define  CSN_ERROR_MESSAGE_TOO_LONG          -9
#define  CSN_ERROR_                         -10

/* CallBack return status */
typedef gint16 CSN_CallBackStatus_t;

#define  CSNCBS_OK                      0
#define  CSNCBS_NOT_OK                -10
#define  CSNCBS_NOT_TO_US             -11
#define  CSNCBS_NOT_COMPLETE          -12

#define CSNCBS_REVISION_LIMIT_STOP   -20 /* Stop packing/unpacking - revision limit */
#define CSNCBS_NOT_SUPPORTED_IE      -21 /* Handling of the unpacked IE is not supported by MS-software */



#ifndef ElementsOf
#define ElementsOf(array) (sizeof(array) / sizeof(array[0]))
#endif

/* Context holding CSN1 parameters */
typedef struct
{
  gint  remaining_bits_len;  /* IN to an csn stream operation */
  gint  bit_offset;          /* IN/OUT to an csn stream operation */
} csnStream_t;

typedef gint16 (*StreamSerializeFcn_t)(proto_tree *tree, csnStream_t* ar, tvbuff_t *tvb, void* data, int ett_csn1);

enum
{
  CSN_END = 0,
  CSN_BIT,
  CSN_UINT,
  CSN_TYPE,
  CSN_CHOICE,
  CSN_UNION,
  CSN_UNION_LH,
  CSN_UINT_ARRAY,
  CSN_TYPE_ARRAY,
  CSN_BITMAP,                /* Bitmap with constant: <bitmap: bit(64)> */
  CSN_VARIABLE_BITMAP,       /* <N: bit (5)> <bitmap: bit(N + offset)> */
  CSN_VARIABLE_BITMAP_1,     /* <bitmap: bit**> i.e. to the end of message (R99) */
  CSN_LEFT_ALIGNED_VAR_BMP,  /* As variable bitmap but the result is left aligned (R99) */
  CSN_LEFT_ALIGNED_VAR_BMP_1,/* As above only size is to the end of message (R99) */
  CSN_VARIABLE_ARRAY,        /* Array with length specified in parameter: <N: bit(4)> <list: octet(N + offset)> */
  CSN_VARIABLE_TARRAY,       /* Type Array with length specified in parameter: <N: bit(x)> <Type>*N */
  CSN_VARIABLE_TARRAY_OFFSET,/* As above but with offset 1. Unpack: <N: bit(x)> <Type>*N - 1  Pack: <N: bit(x)> <Type>*N + 1 */
  CSN_RECURSIVE_ARRAY,       /* Recursive way to specify an array of uint:   <list> ::= {1 <number: bit(4) <list>|0}; */
  CSN_RECURSIVE_TARRAY,      /* Recursive way to specify an array of type:   <list> ::= {1 <type>} ** 0 ; */
  CSN_RECURSIVE_TARRAY_1,    /* same as above but first element always exist:<list> ::= <type> {1 <type>} ** 0 ; */
  CSN_RECURSIVE_TARRAY_2,    /* same as above but with reversed separators :<lists> ::= <type> { 0 <type> } ** 1 ; */
  CSN_EXIST,
  CSN_EXIST_LH,
  CSN_NEXT_EXIST,
  CSN_NEXT_EXIST_LH,
  CSN_NULL,
  CSN_FIXED,
  CSN_CALLBACK,
  CSN_UINT_OFFSET,        /* unpack will add offset, inverse pack will subtract offset */
  CSN_UINT_LH,            /* Low High extraction of int */
  CSN_SERIALIZE,
  CSN_TRAP_ERROR
};

/******************************************************************************************
 * CSN_DESCR structure:
 *
 * type:
 *       This is the CSN type. All existing types are specified in the section above.
 *
 * i:
 *       Depending on the contents of the type parameter,  the parameter "i" may have following meaning:
 *       - specifies the number of bits for the CSN_UINT type
 *       - the offset for an array size by which the size is incremented
 *          for the CSN_VAR_ARRAY type
 *       - the length of each element of an array for the CSN_REC_ARRAY type
 *       - the number of the elements in an array for the CSN_TYPE_ARRAY type
 *       - the offset to the variable keeping the number of elements of an array for in the CSN_VAR_TARRAY type
 *       - the number of different data types in a union for the CSN_UNION, CSN_UNION_LH, and  for the CSN_CHOICE types
 *       - the length in bits of the fixed number defined for  the CSN_FIXED type
 *       - the number of lines to skip in the CSN_DESCR type specified  for  the  CSN_NEXT_EXIST, CSN_NEXT_EXIST_LH,
 *          CSN_NEXT_EXIST_OR_NULL, and CSN_NEXT_EXIST_OR_NULL_LH types
 *       - the number of bits in a bitmap for the CSN_BITMAP type
 *       - the value by which the number of bits in a bitmap has to be incremented or decremented for the
 *          CSN_VAR_BITMAP, CSN_LEFT_VAR_BMP, and CSN_LEFT_BMP_1 types
 *       - the offset to param1 for the CSN_CALLBACK type
 *       - ERRORCODE  used by the CSN_ERROR type
 *
 * descr
 *       This parameter has different meaning depending on the value of the type parameter:
 *       - the offset for  the CSN_UINT_OFFSET type
 *       - the number of the elements in an array of the CSN_UINT_ARRAY type
 *       - the offset to the parameter where the size of the array has to be stored for the CSN_REC_ARRAY type
 *       - the address of the internal structure, describing the member type (by means of the CSN_DESCR type) in the
 *          CSN_TYPE_ARRAY, CSN_VAR_TARRAY, and CSN_TYPE types
 *       - the address of the variable of type CSN_ChoiceElement_t describing all elements in the CSN_CHOICE type union
 *       - the offset to the variable where the number of bits has to be or is stored for the CSN_VAR_BITMAP,
 *          CSN_LEFT_VAR_BMP, and CSN_LEFT_BMP_1 types
 *       - the function number (case number) for the CSN_CALLBACK and CSN_CALLBACK_NO_ARGS types
 *       - the free text used by the CSN_TRAP_ERROR
 *
 * offset
 *         This is an offset  to the _MEMBER parameter counting from the beginning of struct
 *         where the unpacked or packed value shall be stored or fetched. The meaning of the _MEMBER parameter
 *         varies depending on the type which is specified  and so is the meaning of the offset parameter.
 *         Some types (and corresponding macros) do not have the _MEMBER parameter and then the offset parameter
 *         is not used or is different from the offset to the _MEMBER.
 *         - the fixed value for the CSN_FIXED  type
 *         - an offset to the variable UnionType  for CSN_UNION and CSN_UNION_LH types
 *         - an offset to the variable Exist  for CSN_NEXT_EXIST and CSN_NEXT_EXIST_LH types
 *         - an offset to param2 in the CSN_CALLBACK  type
 *
 * sz
 *    - is the name of the parameter within the descr where their unpacked or packed value shall be stored or fetched.
 *      This paramater is pointed out by the offset parameter in the same CSN_DESCR variable as the sz.
 *    - the free text used by the CSN_TRAP_ERROR (the same as parameter "i")
 *
 * serialize
 *    - stores the size of _MEMBER type in case of the  M_TYPE_ARRAY and M_VAR_TARRAY,
 *    - the address of the function which is provided by the M_SERIALIZE type.
 ******************************************************************************************/


typedef struct
{
  gint16      type;
  gint16      i;
  union
  {
    void*     ptr;
    guint32   value;
  } descr;
  size_t      offset;
  const char* sz;
  union
  {
    StreamSerializeFcn_t  fcn;
    guint32               value;
  } serialize;
} CSN_DESCR;

typedef struct
{
  guint8     bits;
  guint8     value;
  CSN_DESCR descr;
} CSN_ChoiceElement_t;

void csnStreamInit(csnStream_t* ar,gint BitOffset,gint BitCount);

/******************************************************************************
* FUNCTION:  csnStreamDissector
* DESCRIPTION:
*            UnPacks data from bit stream. According to CSN description.
* ARGS:
*   ar        stream will hold the parameters to the pack function
*            ar->remaining_bits_len    [IN] Number of bits to unpack [OUT] number of bits left to unpack.
*            ar->bit_offset            [IN/OUT] is the current bit where to proceed with the next bit to unpack.

*   pDescr    CSN description.
*   tvb       buffer containing the bit stream to unpack.
*   data      unpacked data.
*   ett_csn1  tree
*
* RETURNS:  int  Number of bits left to be unpacked. Negative Error code if failed to unpack all bits
******************************************************************************/
gint16 csnStreamDissector(proto_tree *tree, csnStream_t* ar, const CSN_DESCR* pDescr, tvbuff_t *tvb, void* data, int ett_csn1);

/* CSN struct macro's */
#define  CSN_DESCR_BEGIN(_STRUCT)\
        CSN_DESCR CSNDESCR_##_STRUCT[] = {

#define  CSN_DESCR_END(_STRUCT)\
        {CSN_END, 0, {0}, 0, "", {(StreamSerializeFcn_t)0}} };

#define  CSN_ERROR(_STRUCT, _Text, _ERRCODE)\
        {CSN_TRAP_ERROR, _ERRCODE, {_Text}, 0, _Text, {(StreamSerializeFcn_t)0}}

#define M_BIT(_STRUCT, _MEMBER)\
        {CSN_BIT, 0, {0}, offsetof(_STRUCT, _MEMBER), #_MEMBER, {(StreamSerializeFcn_t)0}}

#define M_NEXT_EXIST(_STRUCT, _MEMBER, _NoOfExisting)\
        {CSN_NEXT_EXIST, _NoOfExisting, {0}, offsetof(_STRUCT, _MEMBER), #_MEMBER, {(StreamSerializeFcn_t)0}}

#define M_NEXT_EXIST_LH(_STRUCT, _MEMBER, _NoOfExisting)\
        {CSN_NEXT_EXIST_LH, _NoOfExisting, {0}, offsetof(_STRUCT, _MEMBER), #_MEMBER, {(StreamSerializeFcn_t)0}}

/* Covers the case of { null | 0 | 1 < IE > }.
 * Same as M_NEXT_EXIST with exception of (void*)1 instead of 0.
 */
#define M_NEXT_EXIST_OR_NULL(_STRUCT, _MEMBER, _NoOfExisting)\
        {CSN_NEXT_EXIST, _NoOfExisting, {(void*)1}, offsetof(_STRUCT, _MEMBER), #_MEMBER, {(StreamSerializeFcn_t)0}}

/* Covers the case of { null | L | H < IE > }
 * Same as M_NEXT_EXIST_LH with exception of (void*)1 instead of 0.
 */
#define M_NEXT_EXIST_OR_NULL_LH(_STRUCT, _MEMBER, _NoOfExisting)\
        {CSN_NEXT_EXIST_LH, _NoOfExisting, {(void*)1}, offsetof(_STRUCT, _MEMBER), #_MEMBER, {(StreamSerializeFcn_t)0}}

#define M_UINT(_STRUCT, _MEMBER, _BITS)\
        {CSN_UINT, _BITS, {(void*)1}, offsetof(_STRUCT, _MEMBER), #_MEMBER, {(StreamSerializeFcn_t)0}}

#define M_UINT_LH(_STRUCT, _MEMBER, _BITS)\
        {CSN_UINT_LH, _BITS, {(void*)1}, offsetof(_STRUCT, _MEMBER), #_MEMBER, {(StreamSerializeFcn_t)0}}

#define M_UINT_OFFSET(_STRUCT, _MEMBER, _BITS, _OFFSET)\
        {CSN_UINT_OFFSET, _BITS, {(void*)_OFFSET}, offsetof(_STRUCT, _MEMBER), #_MEMBER, {(StreamSerializeFcn_t)0}}

/* target is an array of integers where
 * _BITS         => number of bits in bitstream to decode for each element
 * _ElementCount => target array length supplied by value
 * _MEMBER       => reference to the first element of target array
 *
 * The last parameter ((0) in structure instantiation marks target array length as a value
 */
#define M_UINT_ARRAY(_STRUCT, _MEMBER, _BITS, _ElementCount)\
        {CSN_UINT_ARRAY, _BITS, {(void*)_ElementCount}, offsetof(_STRUCT, _MEMBER), #_MEMBER, {(StreamSerializeFcn_t)0}}

/* same as above but
 * _ElementCountField => target array length supplied by reference to structure member holding length value
 *
 * The last parameter (1) in structure instantiation marks target array length as a reference to value
 */
#define M_VAR_UINT_ARRAY(_STRUCT, _MEMBER, _BITS, _ElementCountField)\
        {CSN_UINT_ARRAY, _BITS, {(void*)offsetof(_STRUCT, _ElementCountField)}, offsetof(_STRUCT, _MEMBER), #_MEMBER, {(StreamSerializeFcn_t)1}}

#define M_VAR_ARRAY(_STRUCT, _MEMBER, _ElementCountField, _OFFSET)\
        {CSN_VARIABLE_ARRAY, _OFFSET, {(void*)offsetof(_STRUCT, _ElementCountField)}, offsetof(_STRUCT, _MEMBER), #_MEMBER, {(StreamSerializeFcn_t)0}}

#define M_VAR_TARRAY(_STRUCT, _MEMBER, _MEMBER_TYPE, _ElementCountField)\
        {CSN_VARIABLE_TARRAY, offsetof(_STRUCT, _ElementCountField), {(void*)CSNDESCR_##_MEMBER_TYPE}, offsetof(_STRUCT, _MEMBER), #_MEMBER, {(StreamSerializeFcn_t)sizeof(_MEMBER_TYPE)}}

#define M_VAR_TARRAY_OFFSET(_STRUCT, _MEMBER, _MEMBER_TYPE, _ElementCountField)\
        {CSN_VARIABLE_TARRAY_OFFSET, offsetof(_STRUCT, _ElementCountField), {(void*)CSNDESCR_##_MEMBER_TYPE}, offsetof(_STRUCT, _MEMBER), #_MEMBER, {(StreamSerializeFcn_t)sizeof(_MEMBER_TYPE)}}

#define M_REC_ARRAY(_STRUCT, _MEMBER, _ElementCountField, _BITS)\
        {CSN_RECURSIVE_ARRAY, _BITS, {(void*)offsetof(_STRUCT, _ElementCountField)}, offsetof(_STRUCT, _MEMBER), #_MEMBER, {(StreamSerializeFcn_t)0}}

#define M_TYPE_ARRAY(_STRUCT, _MEMBER, _MEMBER_TYPE, _ElementCount)\
        {CSN_TYPE_ARRAY, _ElementCount, {(void*)CSNDESCR_##_MEMBER_TYPE}, offsetof(_STRUCT, _MEMBER), #_MEMBER, {(StreamSerializeFcn_t)sizeof(_MEMBER_TYPE)}}

#define M_REC_TARRAY(_STRUCT, _MEMBER, _MEMBER_TYPE, _ElementCountField)\
        {CSN_RECURSIVE_TARRAY, offsetof(_STRUCT, _ElementCountField), {(void*)CSNDESCR_##_MEMBER_TYPE}, offsetof(_STRUCT, _MEMBER), #_MEMBER, {(StreamSerializeFcn_t)sizeof(_MEMBER_TYPE)}}

#define M_REC_TARRAY_1(_STRUCT, _MEMBER, _MEMBER_TYPE, _ElementCountField)\
        {CSN_RECURSIVE_TARRAY_1, offsetof(_STRUCT, _ElementCountField), {(void*)CSNDESCR_##_MEMBER_TYPE}, offsetof(_STRUCT, _MEMBER), #_MEMBER, {(StreamSerializeFcn_t)sizeof(_MEMBER_TYPE)}}

#define M_REC_TARRAY_2(_STRUCT, _MEMBER, _MEMBER_TYPE, _ElementCountField)\
        {CSN_RECURSIVE_TARRAY_2, offsetof(_STRUCT, _ElementCountField), {(void*)CSNDESCR_##_MEMBER_TYPE}, offsetof(_STRUCT, _MEMBER), #_MEMBER, {(StreamSerializeFcn_t)sizeof(_MEMBER_TYPE)}}

#define M_TYPE(_STRUCT, _MEMBER, _MEMBER_TYPE)\
        {CSN_TYPE, 0, {(void*)CSNDESCR_##_MEMBER_TYPE}, offsetof(_STRUCT, _MEMBER), #_MEMBER, {(StreamSerializeFcn_t)0}}

#define M_UNION(_STRUCT, _COUNT)\
        {CSN_UNION, _COUNT, {0}, offsetof(_STRUCT, UnionType), "UnionType", {(StreamSerializeFcn_t)0}}

#define M_UNION_LH(_STRUCT, _COUNT)\
        {CSN_UNION_LH, _COUNT, {0}, offsetof(_STRUCT, UnionType), "UnionType", {(StreamSerializeFcn_t)0}}

#define M_CHOICE(_STRUCT, _MEMBER, _CHOICE, _ElementCount)\
        {CSN_CHOICE, _ElementCount, {(void*)_CHOICE}, offsetof(_STRUCT, _MEMBER), #_CHOICE, {(StreamSerializeFcn_t)0}}

#define M_FIXED(_STRUCT, _BITS, _BITVALUE)\
        {CSN_FIXED, _BITS, {0}, _BITVALUE, #_BITVALUE, {(StreamSerializeFcn_t)0}}

#define M_SERIALIZE(_STRUCT, _MEMBER, _SERIALIZEFCN)\
        {CSN_SERIALIZE, 1, {0}, offsetof(_STRUCT, _MEMBER), #_MEMBER, {_SERIALIZEFCN}}

#define M_CALLBACK(_STRUCT, _CSNCALLBACKFCN, _PARAM1, _PARAM2)\
        {CSN_CALLBACK, offsetof(_STRUCT, _PARAM1), {_CSNCALLBACKFCN}, offsetof(_STRUCT, _PARAM2), "CallBack_"#_CSNCALLBACKFCN, {(StreamSerializeFcn_t)0}}
#define M_CALLBACK_NO_ARGS(_STRUCT, _CSNCALLBACKFCN)\
        {CSN_CALLBACK, 0, {_CSNCALLBACKFCN}, 0, "CallBack_"#_CSNCALLBACKFCN, {(StreamSerializeFcn_t)0}}

#define M_CALLBACK_THIS(_STRUCT, _SERIALIZEFCN)\
        {CSN_SERIALIZE, 1, {0}, 0, #_SERIALIZEFCN, {_SERIALIZEFCN}}

#define M_BITMAP(_STRUCT, _MEMBER, _BITS)\
        {CSN_BITMAP, _BITS, {0}, offsetof(_STRUCT, _MEMBER), #_MEMBER, {(StreamSerializeFcn_t)0}}

/* variable length, right aligned bitmap i.e. _ElementCountField = 11 => 00000111 11111111 */
#define M_VAR_BITMAP(_STRUCT, _MEMBER, _ElementCountField, _OFFSET)\
        {CSN_VARIABLE_BITMAP, _OFFSET, {(void*)offsetof(_STRUCT, _ElementCountField)}, offsetof(_STRUCT, _MEMBER), #_MEMBER, {(StreamSerializeFcn_t)0}}

/* variable length, right aligned bitmap filling the rest of message
 * - when unpacking the _ElementCountField will be set in runtime
 * - when packing _ElementCountField contains the size of bitmap
 */
#define M_VAR_BITMAP_1(_STRUCT, _MEMBER, _ElementCountField, _OFFSET)\
        {CSN_VARIABLE_BITMAP_1, _OFFSET, {(void*)offsetof(_STRUCT, _ElementCountField)}, offsetof(_STRUCT, _MEMBER), #_MEMBER, {(StreamSerializeFcn_t)0}}

/* variable length, left aligned bitmap i.e. _ElementCountField = 11 => 11111111 11100000 */
#define M_LEFT_VAR_BMP(_STRUCT, _MEMBER, _ElementCountField, _OFFSET)\
        {CSN_LEFT_ALIGNED_VAR_BMP, _OFFSET, {(void*)offsetof(_STRUCT, _ElementCountField)}, offsetof(_STRUCT, _MEMBER), #_MEMBER, {(StreamSerializeFcn_t)0}}

/* variable length, left aligned bitmap filling the rest of message
 *- when unpacking the _ElementCountField will be set in runtime
 * - when packing _ElementCountField contains the size of bitmap
 */
#define M_LEFT_VAR_BMP_1(_STRUCT, _MEMBER, _ElementCountField, _OFFSET)\
        {CSN_LEFT_ALIGNED_VAR_BMP_1, _OFFSET, {(void*)offsetof(_STRUCT, _ElementCountField)}, offsetof(_STRUCT, _MEMBER), #_MEMBER, {(StreamSerializeFcn_t)0}}

#define M_NULL(_STRUCT, _MEMBER)\
        {CSN_NULL, 0, {0}, offsetof(_STRUCT, _MEMBER), #_MEMBER, {(StreamSerializeFcn_t)0}}

#define M_THIS_EXIST(_STRUCT)\
        {CSN_EXIST, 0, {0}, offsetof(_STRUCT, Exist), "Exist", {(StreamSerializeFcn_t)0}}

#define M_THIS_EXIST_LH(_STRUCT)\
        {CSN_EXIST_LH, 0, {0}, offsetof(_STRUCT, Exist), "Exist", {(StreamSerializeFcn_t)0}}

/* return value 0 if ok else discontionue the unpacking */
typedef gint16 (*CsnCallBackFcn_t)(void* pv ,...);

#define CSNDESCR(_FuncType) CSNDESCR_##_FuncType

#endif /*_PACKET_CSN1_H_*/
