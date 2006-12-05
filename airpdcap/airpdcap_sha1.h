#ifndef	_AIRPDCAP_SHA1
#define	_AIRPDCAP_SHA1

/******************************************************************************/
/*	File includes																					*/
/*																										*/
#include "airpdcap_interop.h"
/*																										*/
/*																										*/
/******************************************************************************/

/******************************************************************************/
/*	Definitions																						*/
/*																										*/
/* Maximum HMAC block length																	*/
#define HMAC_MAX_BLOCK_LEN	SHA2_512_HMAC_BLOCK_LEN
#define HMAC_IPAD_VAL	0x36
#define HMAC_OPAD_VAL	0x5C
/*																										*/
/******************************************************************************/

/******************************************************************************/
/*	External function prototypes declarations												*/
/*																										*/
void AirPDcapAlgHmacSha1(
								 const UCHAR *key_len,
								 const size_t keylen,
								 UCHAR *buffer,
								 const size_t digest_len,
								 UCHAR digest[20])
					;
/*																										*/
/******************************************************************************/

#endif