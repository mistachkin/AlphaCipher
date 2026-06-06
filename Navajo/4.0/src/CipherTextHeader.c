/*
 * CipherTextHeader.c -- Implements VPH for CipherText Header
 *
 * Copyright (c) 2009-2026 by Joe Mistachkin.  All rights reserved.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) $Id: $
 */

#if !defined(FEATURE_KERNEL_ONLY)

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "navajoPort.h"
#include "navajo.h"
#include "navajoProtocol.h"
#include "navajoSqlite.h"
#include "navajoSql.h"
#include "navajoIntTypes.h"
#include "navajoInt.h"
#include "navajoUtil.h"

#include "CipherTextHeader.h"
#include "hresult.h"
#include "puke.h"

#pragma code_seg(".vph")
#if !defined(WIN32) && !defined(_WIN32_WCE)
GUID GUID_NULL = {0};
#endif

/******************************************************************************
** Rule #1: The first header, known as the leading header, must at least
** contain the CT_FLAG_MASK_LEADING_MUST_HAVE.
** Rule #2: All other non-leading headers may be of any size, all the way from
** four bytes long to unknown size that depends on the header flags.
** Rule #3: All headers must contain a checksum as filtered by
** CT_FLAG_MASK_CT_CHECKSUM.
*******************************************************************************
** VPH header flag format for a cipher text header.
** The first DWORD in the header is a set of flags that determine the size
** and contents of the header.  The highest bit (CT_ENFORCE_DIGITAL_CT) of
** these flags determines whether the CT is alphabetic or digital.  (All alpha
** characters belong in the range of A to Z; so if the high bit of the DWORD
** is found to be set, it can never be an alphabetic cipher text.)  On the
** other hand, if the CT gets mimed, CT_ENFORCE_DIGITAL_CT bit will be
** mimed away, meaning the CT is alphabetic.
**
** The CT_FLAG_INTERNAL field may be set for a header to limit the length
** of the header as much as possible.  When it is set, nibbles 3 through 6
** of the flags is actually a 16 bit offset and nibbles 7 and 8 is actually
** a CRC8 checksum.  If not set, nibles 3 through 8 are used as normal flags
** as explained elsewhere.  CT_FLAG_MASK_INTERNAL_FLAGS can be used to mask
** out flags that are only valid when the CT_FLAG_INTERNAL is set (currently
** unused).  The idea is to have a short header with a 16 bit offset and a
** small checksum and a few flags.  So if a header can not be collapsed into
** a DWORD, CT_FLAG_INTERNAL will not be used.
**
** The CT_KERNEL_IDENTIFIER tells us whether the header has information
** regarding the kernel to be used for decryption.
**
** The CT_DECRYPT_KEY_IDENTIFIER tells us the guid of the key the CT is to
** be decrypted with.
**
** The CT_64BIT_OFFSET, CT_32BIT_OFFSET and CT_16BIT_OFFSET specify the length
** of the CT offset, which can be 64, 32, 16 or no bits (no flags set).
**
** The CT_64BIT_CT_LENGTH, CT_32BIT_CT_LENGTH and CT_16BIT_CT_LENGTH specify
** the length of the CT size, which can be 64, 32, 16, or no bits (no flags
** set).
**
** The CT_FLAG_MASK_CRYPTO_ALGORYTHM isolates the algorythm that was used
** to encrypt the plain text.  This can be OTP, AES, etc.
**
** The compression used on the plain text before encryption ranges from
** CT_COMPRESSION_UNDEF_HIGHEST to CT_COMPRESSION_ZIP.  CT_COMPRESSION_NONE
** specifyes no compression being used.  Currently implementaion includes
** the zipper or no comprssion.
**
** The CT_FLAG_MASK_KEY_CHECKSUM can be used in the leading header to obtain
** the checksum to be used to determine if key to decrypt the chunk is still
** available (in other words, key bits have not been erased).  In the leading
** header, CT_KEY_USES_NONE is not valid.
**
** The CT_TRAILER tells us that the header is really a trailer, which may
** contain such items as a checksum of the previous headers.
**
** Upto 7 different checksums may be used on the cipher text itself.  (Use
** CT_FLAG_MASK_CT_CHECKSUM to isolate the checksum that is used in the CT.)
** The currently available checksums are MD5, CRC32 (TODOTODO: and CRC8).
**
** The CT_EXTENDED_FLAGS bit of any non-internal DWORD flag determines if
** there are more DWORD flags following the current one (for future
** extensibility).
*******************************************************************************
** To maintain backward compatibility, do not change the values of these flags.
** X = used flags: XXXX XXXX XXXX 0000 0000 XXXX XXXX XXXX
******************************************************************************/
//VPH flags for a CipherText header.
#define CT_ENFORCE_DIGITAL_CT				0x80000000
#define CT_FLAG_INTERNAL					0x40000000
//BEGIN CT_FLAG_INTERNAL is set
#define CT_INTERNAL_EXTENDED_FLAGS			0x01000000		//Another DWORD for more flags.
//Nibbles 3 to 6 are a 16bit number
//Nibbles 7 and 8 are an 8bit CRC checksum.
//ELSE CT_FLAG_INTERNAL is not set
#define CT_KERNEL_IDENTIFIER				0x20000000
#define CT_DECRYPT_KEY_IDENTIFIER			0x10000000
#define CT_64BIT_OFFSET						0x0C000000
#define CT_32BIT_OFFSET						0x08000000
#define CT_16BIT_OFFSET						0x04000000
//#define CT_NO_OFFSET						0x00000000
#define CT_64BIT_CT_LENGTH					0x03000000
#define CT_32BIT_CT_LENGTH					0x02000000
#define CT_16BIT_CT_LENGTH					0x01000000
//#define CT_FIXED_CT_LENGTH				0x00000000
#define CT_MANDATORY_DECRYPT_ONCE			0x00800000		//Decrypt users will be able to decrypt CT only once.
#define CT_VERIFY_DECRYPT_KEY				0x00400000		//Verify decrypt user's key to see if it matches our encrypt key.
#define CT_CIPHER_ALGORITHM					0x00200000		//Indicates the algorithm specifier is present.
#define CT_CIPHER_ENDCHAIN					0x00100000		//Indicates end of cipher chain for CBC ciphers.
#define CT_COMPRESSION_UNDEF_HIGHEST		0x00000F00		//Highest possible value for compression.
#define CT_COMPRESSION_ZIP					0x00000100		//Compression by zip
//#define CT_COMPRESSION_NONE				0x00000000		//No compression
//CT_FLAG_MASK_KEY_CHECKSUM allows us a range of 7 possible options (excluding none used).
#define CT_KEY_USES_UNDEF_HIGHEST			0x000000E0		//Highest possible value for key checksum.
#define CT_KEY_USES_MD5						0x00000080		//Check for erased key via MD5 checksum.
#define CT_KEY_USES_CRC32					0x00000040		//Check for erased key via CRC32 checksum.
#define CT_KEY_USES_UNDEF_LOWEST			0x00000020		//Lowest possible value for key checksum.
//#define CT_KEY_USES_NONE					0x00000000		//No erased key check is needed (not valid in leading header).
#define CT_TRAILER							0x00000010
//CT_FLAG_MASK_CT_CHECKSUM allows us a range of 7 possible options (excluding none used).
#define CT_CT_USES_UNDEF_HIGHEST			0x0000000E		//Highest possible value for CipherText checksum.
#define CT_CT_USES_MD5						0x00000006		//CipherText uses MD5 checksum.
#define CT_CT_USES_CRC32					0x00000004		//CipherText uses CRC32 checksum.
#define CT_CT_USES_CRC8						0x00000002		//CipherText uses CRC8 checksum.
#define CT_EXTENDED_FLAGS					0x00000001		//Another DWORD for more flags.
//END CT_FLAG_INTERNAL

//VPH flag masks for CipherText header.
//BEGIN CT_FLAG_INTERNAL is set
#define CT_FLAG_MASK_INTERNAL_FLAGS			0xFF000000		//Valid flags when CT_FLAG_INTERNAL is set.
//ELSE CT_FLAG_INTERNAL is not set
#define CT_FLAG_MASK_CT_OFFSET				0x0C000000		//Length (in bits) of the CT offset.
#define CT_FLAG_MASK_CT_LENGTH				0x03000000		//Length (in bits) of the CT size.
#define CT_FLAG_MASK_COMPRESSION			0x00000F00		//Compression to use.
#define CT_FLAG_MASK_KEY_CHECKSUM			0x000000E0		//Ways to check for erased keys.
#define CT_FLAG_MASK_CT_CHECKSUM			0x0000000E		//CipherText checksum to use.
															//CT_DECRYPT_KEY_IDENTIFIER flags.
//END CT_FLAG_INTERNAL

/******************************************************************************
** Attempt to make this header as small as possible by removing information
** that is the same when compared to the previous header.  This is an attempt
** in reducing the size of a non-leading header to be as small as possible.
** One such reduction would be when the length of the CipherText is constant,
** we can mark non-leading headers to have a fixed CT size (dwFIXED_CT_LENGTH).
******************************************************************************/
ACK_RESULT VPH_CT_CollapseStructure(LPCIPHER_TEXT_HEADER pStruct,LPCIPHER_TEXT_HEADER pPreviousStruct,BOOL bSinglePacket)
{
	USES_RTN;
	RTN_IF_BADPTR(pStruct);
	RTN_IF_BADPTR(pPreviousStruct);

//XXXXXX:	if (!pStruct->bInitialized)
//		return HR_HEADER_NOT_INITIALIZED;
	if (pPreviousStruct->bInitialized)
	{
		//Clear the decrypt key guid field if same in previous header.
		if (IsEqualGUID(&pStruct->guidDecryptKeyIdentifier,&pPreviousStruct->guidDecryptKeyIdentifier))
			pStruct->guidDecryptKeyIdentifier = GUID_NULL;
		//Clear the size field if same length as previous CT chunk.
		if (pStruct->u64CipherTextLength == pPreviousStruct->u64CipherTextLength)
			pStruct->u64CipherTextLength = 0;
		//Clear the cipher algorithm if same in previous header.
		if (pStruct->u32CipherAlgorithm == pPreviousStruct->u32CipherAlgorithm)
			pStruct->u32CipherAlgorithm = 0;

		if (bSinglePacket)
		{
			//Determine if this header could be made an internal header.
			//If it can be made an internal header than make it so.

			//Check high values of offset for no change.
			BOOL bInternal = ((pStruct->u64DecryptKeyOffset & 0xFFFFFFFFFFFF0000) == (pPreviousStruct->u64DecryptKeyOffset & 0xFFFFFFFFFFFF0000));
			//Check compression method for no change.
			bInternal = bInternal & (pStruct->u32Compression == pPreviousStruct->u32Compression);
			//Verification of key material just bloats the packet.  There are other ways to deal with bad key material,
			//including use of a sync frame where bloating is not a concern.
			//XXXXXX:Note that currently we are not using this field but this value should be checked against CRC32.
			bInternal = bInternal & (pStruct->u32CipherKeyChecksumType == 0);
			//Check cipher text checksum method for no change.
			//XXXXXX:Note that currently we are not using this field but this value should be checked against CRC8.
			bInternal = bInternal & (pStruct->u32CipherTextChecksumType == 0);

			//Can we collapse to an internal header?
			if (bInternal)
			{
				pStruct->bFlagInternal = TRUE;
				//Since it will be a single packet, the size of the CT will be size of the packet minus the 4 byte header.
				pStruct->u64CipherTextLength = 0;
				pStruct->u32Compression = 0;
				pStruct->u32CipherKeyChecksumType = 0;		//XXXXXX:Should be checksum type none.
				pStruct->u32CipherTextChecksumType = 0;		//XXXXXX:Should be checksum type none.
			}
		}
	}

	BEGIN_RTN_CLEAN_UP;
	END_RTN_CLEAN_UP;
}

ACK_RESULT WriteToNetBuffer(LPBYTE pBuffer,SIZE_T sizeBuffer,UINT* puOffset,LPBYTE pValue,SIZE_T sizeValue)
{
	USES_RTN;
	RTN_IF_BADPTR(pBuffer);
	RTN_IF_BADPTR(puOffset);
	RTN_IF_BADPTR(pValue);

	if ((*puOffset + sizeValue) > sizeBuffer)
		return E_OVERFLOW;

	//HOST2NETWORK(local);
	memcpy(&pBuffer[*puOffset],pValue,sizeValue);
	*puOffset = *puOffset + sizeValue;

	BEGIN_RTN_CLEAN_UP;
	END_RTN_CLEAN_UP;
}

ACK_RESULT VPH_CT_StructureToBuffer(LPCIPHER_TEXT_HEADER pStruct,LPBYTE pBuffer,SIZE_T* psizeBuffer)
{
	USES_RTN;
	UINT32 uHeaderFlags = 0;
	UINT uOffset = 0;

	RTN_IF_BADPTR(pStruct);
	RTN_IF_BADPTR(pBuffer);
	RTN_IF_BADPTR(psizeBuffer);

	//Always have this flag set.
	uHeaderFlags = uHeaderFlags | CT_ENFORCE_DIGITAL_CT;

	if (pStruct->bFlagInternal)
	{
		uHeaderFlags = uHeaderFlags | CT_FLAG_INTERNAL;
/*For future expansion
		//Add other internal flags here
*/
		//Add internal 16bit offset *before* possible conversion.
		uHeaderFlags = uHeaderFlags | ((pStruct->u64DecryptKeyOffset & 0xFFFF) << 8);

//	UINT32 uCipherTextChecksumType;				//CipherText checksum.
//	LPBYTE pCipherTextChecksum;					

		RTN_IF_FAILED(WriteToNetBuffer(pBuffer,*psizeBuffer,&uOffset,(LPBYTE) &uHeaderFlags,sizeof(uHeaderFlags)));
/*For future expansion.
		if ((uHeaderFlags & dwCT_INTERNAL_EXTENDED_FLAGS) == dwCT_INTERNAL_EXTENDED_FLAGS)
			RTN_IF_FAILED(WriteToNetBuffer(lpBuffer,dwBufferSize,bConcealed,&dwOffset,uHeaderFlagsXX));
*/

		//Actual size of header that was written.
		*psizeBuffer = uOffset;
	}
	else
	{
		//Start writing to the buffer after the flags area.
		uOffset = sizeof(uHeaderFlags);

		if (!IsEqualGUID(&pStruct->guidKernelIdentifier,&GUID_NULL))
		{
			uHeaderFlags = uHeaderFlags | CT_KERNEL_IDENTIFIER;
			RTN_IF_FAILED(WriteToNetBuffer(pBuffer,*psizeBuffer,&uOffset,(LPBYTE) &pStruct->guidKernelIdentifier,sizeof(pStruct->guidKernelIdentifier)));
			//At this point m_dwWaldoOffset should always be zero.
			//XXXXXX:ASSERT(m_dwWaldoOffset == 0);
			RTN_IF_FAILED(WriteToNetBuffer(pBuffer,*psizeBuffer,&uOffset,(LPBYTE) &pStruct->u32WaldoOffset,sizeof(pStruct->u32WaldoOffset)));
		}

		if (!IsEqualGUID(&pStruct->guidDecryptKeyIdentifier,&GUID_NULL))
		{
			uHeaderFlags = uHeaderFlags | CT_DECRYPT_KEY_IDENTIFIER;
			RTN_IF_FAILED(WriteToNetBuffer(pBuffer,*psizeBuffer,&uOffset,(LPBYTE) &pStruct->guidDecryptKeyIdentifier,sizeof(pStruct->guidDecryptKeyIdentifier)));
		}

		//XXXXXX:Signal no compression right here by putting zeroes into where waldo would be.

		if (pStruct->u64DecryptKeyOffset)
		{
			if ((pStruct->u64DecryptKeyOffset & 0xFFFFFFFFFFFF0000) == 0)
			{
				unsigned short w = (unsigned short) pStruct->u64DecryptKeyOffset;	//Just the lower WORD portion is needed.
				uHeaderFlags = uHeaderFlags | CT_16BIT_OFFSET;
				RTN_IF_FAILED(WriteToNetBuffer(pBuffer,*psizeBuffer,&uOffset,(LPBYTE) &w,sizeof(w)));
			}
			else if ((pStruct->u64DecryptKeyOffset & 0xFFFFFFFF00000000) == 0)
			{
				UINT32 dw = (UINT32) pStruct->u64DecryptKeyOffset;					//Just the lower DWORD portion is needed.
				uHeaderFlags = uHeaderFlags | CT_32BIT_OFFSET;
				RTN_IF_FAILED(WriteToNetBuffer(pBuffer,*psizeBuffer,&uOffset,(LPBYTE) &dw,sizeof(dw)));
			}
			else
			{
				uHeaderFlags = uHeaderFlags | CT_64BIT_OFFSET;
				RTN_IF_FAILED(WriteToNetBuffer(pBuffer,*psizeBuffer,&uOffset,(LPBYTE) &pStruct->u64DecryptKeyOffset,sizeof(pStruct->u64DecryptKeyOffset)));
			}
		}

		if (pStruct->u64CipherTextLength)
		{
			if ((pStruct->u64CipherTextLength & 0xFFFFFFFFFFFF0000) == 0)
			{
				unsigned short w = (unsigned short) pStruct->u64CipherTextLength;	//Just the lower WORD portion is needed.
				uHeaderFlags = uHeaderFlags | CT_16BIT_CT_LENGTH;
				RTN_IF_FAILED(WriteToNetBuffer(pBuffer,*psizeBuffer,&uOffset,(LPBYTE) &w,sizeof(w)));
			}
			else if ((pStruct->u64CipherTextLength & 0xFFFFFFFF00000000) == 0)
			{
				UINT32 dw = (UINT32) pStruct->u64CipherTextLength;					//Just the lower DWORD portion is needed.
				uHeaderFlags = uHeaderFlags | CT_32BIT_CT_LENGTH;
				RTN_IF_FAILED(WriteToNetBuffer(pBuffer,*psizeBuffer,&uOffset,(LPBYTE) &dw,sizeof(dw)));
			}
			else
			{
				uHeaderFlags = uHeaderFlags | CT_64BIT_CT_LENGTH;
				RTN_IF_FAILED(WriteToNetBuffer(pBuffer,*psizeBuffer,&uOffset,(LPBYTE) &pStruct->u64CipherTextLength,sizeof(pStruct->u64CipherTextLength)));
			}
		}

		if (pStruct->bMandatoryDecryptOnce)
			uHeaderFlags = uHeaderFlags | CT_MANDATORY_DECRYPT_ONCE;
		if (pStruct->bVerifyDecryptKey)
			uHeaderFlags = uHeaderFlags | CT_VERIFY_DECRYPT_KEY;

		if (pStruct->u32CipherAlgorithm)
		{
			//ASSERT(((DWORD)pStruct->u32CipherAlgorithm & 0xFFFFFF00) == 0);
			BYTE by = (BYTE) pStruct->u32CipherAlgorithm;
			uHeaderFlags = uHeaderFlags | CT_CIPHER_ALGORITHM;
			RTN_IF_FAILED(WriteToNetBuffer(pBuffer,*psizeBuffer,&uOffset,(LPBYTE) &by,sizeof(by)));
		}
		if (pStruct->bCipherEndChain)
			uHeaderFlags = uHeaderFlags | CT_CIPHER_ENDCHAIN;

//	UINT32 uCompression;						//The compression that was used before encryption occured.
//	UINT32 uCipherKeyChecksumType;				//Key checksum for determining erased state and/or verifying decrypt key.
//	LPBYTE pCipherKeyChecksum;					

		if (pStruct->bTrailer)
			uHeaderFlags = uHeaderFlags | CT_TRAILER;

//	UINT32 uCipherTextChecksumType;				//CipherText checksum.
//	LPBYTE pCipherTextChecksum;					

/*For future expansion.
		if ((uHeaderFlags & dwCT_EXTENDED_FLAGS) == dwCT_EXTENDED_FLAGS)
			RTN_IF_FAILED(WriteToNetBuffer(lpBuffer,dwBufferSize,bConcealed,&dwOffset,uHeaderFlagsXX));
*/

		//Actual size of header that was written.
		*psizeBuffer = uOffset;
		//Write out flags.
		uOffset = 0;
		RTN_IF_FAILED(WriteToNetBuffer(pBuffer,*psizeBuffer,&uOffset,(LPBYTE) &uHeaderFlags,sizeof(uHeaderFlags)));
	}

	BEGIN_RTN_CLEAN_UP;
	END_RTN_CLEAN_UP;
}

ACK_RESULT ReadFromNetBuffer(LPBYTE pBuffer,SIZE_T size,UINT* puOffset,LPBYTE pValue,SIZE_T sizeValue)
{
	USES_RTN;
	RTN_IF_BADPTR(pBuffer);
	RTN_IF_BADPTR(puOffset);
	RTN_IF_BADPTR(pValue);

	if ((*puOffset + sizeValue) > size)
		return E_OVERFLOW;

	//HOST2NETWORK(local);
	memcpy(pValue,&(pBuffer[*puOffset]),sizeValue);
	*puOffset = *puOffset + sizeValue;

	BEGIN_RTN_CLEAN_UP;
	END_RTN_CLEAN_UP;
}

ACK_RESULT VPH_CT_BufferToStructure(LPBYTE pBuffer,SIZE_T* psizeBuffer,LPCIPHER_TEXT_HEADER pStruct)
{
	USES_RTN;
	UINT uOffset = 0;
	UINT32 uHeaderFlags = 0;

	RTN_IF_BADPTR(pBuffer);
	RTN_IF_BADPTR(psizeBuffer);
	RTN_IF_BADPTR(pStruct);
	memset(pStruct,0,sizeof(CIPHER_TEXT_HEADER ));

	RTN_IF_FAILED(ReadFromNetBuffer(pBuffer,*psizeBuffer,&uOffset,(LPBYTE) &uHeaderFlags,sizeof(uHeaderFlags)));

	if ((uHeaderFlags & CT_FLAG_INTERNAL) == CT_FLAG_INTERNAL)
	{
		pStruct->bFlagInternal = TRUE;
//		if (m_bLeadingHeader)
//			return HR_VPH_IS_INVALID;				//The leading header can not have an internal flag.

		//Strip out the 16bit offset.
		pStruct->u64DecryptKeyOffset = ((uHeaderFlags >> 8) & 0xFFFF);
/*		//Strip out the 8bit checksum.
		BYTE byCheckSum = (BYTE) uHeaderFlags & 0xFF;

		//Are there any new flags we don't understand?
		if ((uHeaderFlags & dwCT_FLAG_MASK_INTERNAL_COMPREHEND) != uHeaderFlags)
			return HR_VPH_IS_NEWER;

		uHeaderFlags = uHeaderFlags | dwCT_CT_USES_CRC8;	//We are also restoring the checksum from the internal header.
		RTN_HR_IF_FAILED(CObject<CSecureMemory>::CreateInstance(m_spCTCheckSum.GetPtr()));
		RTN_HR_IF_FAILED(m_spCTCheckSum->RequestStorage(ms_dwSizeOfCRC8));
		*(m_spCTCheckSum->GetBuffer()) = byCheckSum;*/

/*For future expansion.
		if ((uHeaderFlags & dwCT_INTERNAL_EXTENDED_FLAGS) == dwCT_INTERNAL_EXTENDED_FLAGS)
			RTN_HR_IF_FAILED(ReadFromNetBuffer(lpBuffer,dwBufferSize,bConcealed,&dwOffset,&uHeaderFlagsXX));
*/
	}
	else
	{
		if ((uHeaderFlags & CT_KERNEL_IDENTIFIER) == CT_KERNEL_IDENTIFIER)
		{
			RTN_IF_FAILED(ReadFromNetBuffer(pBuffer,*psizeBuffer,&uOffset,(LPBYTE) &pStruct->guidKernelIdentifier,sizeof(pStruct->guidKernelIdentifier)));
			//At this point m_dwWaldoOffset should always be zero.
			//XXXXXX:ASSERT(m_dwWaldoOffset == 0);
			RTN_IF_FAILED(ReadFromNetBuffer(pBuffer,*psizeBuffer,&uOffset,(LPBYTE) &pStruct->u32WaldoOffset,sizeof(pStruct->u32WaldoOffset)));
		}

		if ((uHeaderFlags & CT_DECRYPT_KEY_IDENTIFIER) == CT_DECRYPT_KEY_IDENTIFIER)
			RTN_IF_FAILED(ReadFromNetBuffer(pBuffer,*psizeBuffer,&uOffset,(LPBYTE) &pStruct->guidDecryptKeyIdentifier,sizeof(pStruct->guidDecryptKeyIdentifier)));

		//XXXXXX:Signal no compression right here by putting zeroes into where waldo would be.

		switch (uHeaderFlags & CT_FLAG_MASK_CT_OFFSET)
		{
			case CT_16BIT_OFFSET:
			{
				unsigned short w;		//Restore the lower WORD portion.
				RTN_IF_FAILED(ReadFromNetBuffer(pBuffer,*psizeBuffer,&uOffset,(LPBYTE) &w,sizeof(w)));
				pStruct->u64DecryptKeyOffset = w;
				break;
			}
			case CT_32BIT_OFFSET:
			{
				UINT32 dw;			//Restore the lower DWORD portion.
				RTN_IF_FAILED(ReadFromNetBuffer(pBuffer,*psizeBuffer,&uOffset,(LPBYTE) &dw,sizeof(dw)));
				pStruct->u64DecryptKeyOffset = dw;
				break;
			}
			case CT_64BIT_OFFSET:
				RTN_IF_FAILED(ReadFromNetBuffer(pBuffer,*psizeBuffer,&uOffset,(LPBYTE) &pStruct->u64DecryptKeyOffset,sizeof(pStruct->u64DecryptKeyOffset)));
				break;
			default:
				//ASSERT(false);
				break;
		}

		switch (uHeaderFlags & CT_FLAG_MASK_CT_LENGTH)
		{
			case CT_16BIT_CT_LENGTH:
			{
				unsigned short w;		//Restore the lower WORD portion.
				RTN_IF_FAILED(ReadFromNetBuffer(pBuffer,*psizeBuffer,&uOffset,(LPBYTE) &w,sizeof(w)));
				pStruct->u64CipherTextLength = w;
				break;
			}
			case CT_32BIT_CT_LENGTH:
			{
				UINT32 dw;			//Restore the lower DWORD portion.
				RTN_IF_FAILED(ReadFromNetBuffer(pBuffer,*psizeBuffer,&uOffset,(LPBYTE) &dw,sizeof(dw)));
				pStruct->u64CipherTextLength = dw;
				break;
			}
			case CT_64BIT_CT_LENGTH:
				RTN_IF_FAILED(ReadFromNetBuffer(pBuffer,*psizeBuffer,&uOffset,(LPBYTE) &pStruct->u64CipherTextLength,sizeof(pStruct->u64CipherTextLength)));
				break;
			default:
				//ASSERT(false);
				break;
		}

		if ((uHeaderFlags & CT_MANDATORY_DECRYPT_ONCE) == CT_MANDATORY_DECRYPT_ONCE)
			pStruct->bMandatoryDecryptOnce = TRUE;
		if ((uHeaderFlags & CT_VERIFY_DECRYPT_KEY) == CT_VERIFY_DECRYPT_KEY)
			pStruct->bVerifyDecryptKey = TRUE;

		if ((uHeaderFlags & CT_CIPHER_ALGORITHM) == CT_CIPHER_ALGORITHM)
		{
			//ASSERT(((DWORD)pStruct->u32CipherAlgorithm & 0xFFFFFF00) == 0);
			BYTE by;
			RTN_IF_FAILED(ReadFromNetBuffer(pBuffer,*psizeBuffer,&uOffset,(LPBYTE) &by,sizeof(by)));
			pStruct->u32CipherAlgorithm = by;
		}
		if ((uHeaderFlags & CT_CIPHER_ENDCHAIN) == CT_CIPHER_ENDCHAIN)
			pStruct->bCipherEndChain = TRUE;

//	UINT32 uCompression;						//The compression that was used before encryption occured.
//	UINT32 uCipherKeyChecksumType;				//Key checksum for determining erased state and/or verifying decrypt key.
//	LPBYTE pCipherKeyChecksum;					

		if ((uHeaderFlags & CT_TRAILER) == CT_TRAILER)
			pStruct->bTrailer = TRUE;

//	UINT32 uCipherTextChecksumType;				//CipherText checksum.
//	LPBYTE pCipherTextChecksum;					

/*For future expansion.
		if ((uHeaderFlags & dwCT_EXTENDED_FLAGS) == dwCT_EXTENDED_FLAGS)
			RTN_IF_FAILED(ReadFromNetBuffer(lpBuffer,dwBufferSize,bConcealed,&dwOffset,uHeaderFlagsXX));
*/
	}
	*psizeBuffer = uOffset;
	pStruct->bInitialized = TRUE;

	BEGIN_RTN_CLEAN_UP;
	END_RTN_CLEAN_UP;
}

/******************************************************************************
** Fill in the missing pieces of this header by referencing the previous
** header.
******************************************************************************/
ACK_RESULT VPH_CT_ExpandStructure(LPCIPHER_TEXT_HEADER pStruct,LPCIPHER_TEXT_HEADER pPreviousStruct,SIZE_T sizePacket)
{
	USES_RTN;
	RTN_IF_BADPTR(pStruct);
	RTN_IF_BADPTR(pPreviousStruct);

//XXXXXX:	if (!pStruct->bInitialized)
//		return HR_HEADER_NOT_INITIALIZED;
	if (pPreviousStruct->bInitialized)
	{
		//Expand from an internal header.
		if (pStruct->bFlagInternal)
		{
			//Expand compression method.
			pStruct->u32Compression = pPreviousStruct->u32Compression;
			//Verify cipher text checksum method.
			//XXXXXX:ASSERT(pStruct->u32CipherTextChecksumType == CT_CT_USES_CRC8);

			//Expand offset.
			pStruct->u64DecryptKeyOffset = pStruct->u64DecryptKeyOffset | (pPreviousStruct->u64DecryptKeyOffset & 0xFFFFFFFFFFFF0000);
			//Since it is a single packet, the size of the CT is the size of the packet minus the 4 byte header.
			pStruct->u64CipherTextLength = sizePacket - sizeof(UINT32);
		}

		//Use previous header's decrypt key GUID when this header doesn't have one.
		if (IsEqualGUID(&pStruct->guidDecryptKeyIdentifier,&GUID_NULL))
			pStruct->guidDecryptKeyIdentifier = pPreviousStruct->guidDecryptKeyIdentifier;
		//Use previous header's CipherText size when this header doesn't have one.
		if (pStruct->u64CipherTextLength == 0)
			pStruct->u64CipherTextLength = pPreviousStruct->u64CipherTextLength;
		if (pStruct->u32Compression == 0)
			pStruct->u32Compression = pPreviousStruct->u32Compression;
		else
		{
			//Verify that there is no change in compression method.
			//XXXXXX:if (pStruct->u32Compression != pPreviousStruct->u32Compression)
			//	return HR_FILE_AUTH_FAILED;
		}

		if (pStruct->u32CipherAlgorithm == 0)
			pStruct->u32CipherAlgorithm = pPreviousStruct->u32CipherAlgorithm;
		else
		{
			//Verify that there is no change in cipher algorithm.
			//XXXXXX:if (pStruct->u32CipherAlgorithm != pPreviousStruct->u32CipherAlgorithm)
			//	return HR_FILE_AUTH_FAILED;
		}

		//Calculate the offset to use for AK consumption based on the
		//previous header's offset and content size while doing non-OTP
		//decryption.
/*		if ((m_cipherAlgorithm != CIPHERAlgorithmOTP) &&
			((m_dwFlags & dwCT_FLAG_MASK_CT_OFFSET) == dwCT_NO_OFFSET))
		{
			m_dw64Offset = pPreviousStruct->m_dw64Offset + pPreviousStruct->m_dw64Length;
		}
*/
	}

	BEGIN_RTN_CLEAN_UP;
	END_RTN_CLEAN_UP;
}
#pragma code_seg()

#endif

/* end of file */
