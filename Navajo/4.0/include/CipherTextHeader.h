/*
 * CipherTextHeader.h -- Implements VPH for CipherText Header
 *
 * Copyright (c) 2009-2026 by Joe Mistachkin.  All rights reserved.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) $Id: $
 */

#ifndef _CIPHERTEXTHEADER_H_
#define _CIPHERTEXTHEADER_H_

#ifndef FACILITY_ACKP
#define FACILITY_ACKP                         97
#endif

typedef struct CIPHER_TEXT_HEADER_tag {
	BOOL	bInitialized;							//This structure is initialized.
	BOOL	bFlagInternal;							//The cipher text header is forced into a 32 bit field.
	GUID	guidKernelIdentifier;					//KernelKey guid, quick check for correct Kernel.
	UINT32	u32WaldoOffset;							//The waldo offset into the kernel key the header was encrypted with.
	GUID	guidDecryptKeyIdentifier;				//The guid of the key the data was encrypted with.
	UINT64	u64DecryptKeyOffset;					//The offset into the key the data was encrypted with.
	UINT64	u64CipherTextLength;					//Length of the CipherText.
	BOOL	bMandatoryDecryptOnce;					//Force erasing of cipher key after decryption.
	BOOL	bVerifyDecryptKey;						//Verify decrypt key before decryption to ensure bytes are the same.
	UINT32	u32CipherAlgorithm;						//Cipher algorithm the data was encrypted with.
	BOOL	bCipherEndChain;						//Marker for end of cipher data chain.
	UINT32	u32Compression;							//The compression that was used before encryption occured.
	UINT32	u32CipherKeyChecksumType;				//Key checksum for determining erased state and/or verifying decrypt key.
	LPBYTE	pCipherKeyChecksum;					
	BOOL	bTrailer;								//Marks the last cipher text header (cipher text length must be zero).
	UINT32	u32CipherTextChecksumType;				//CipherText checksum.
	LPBYTE	pCipherTextChecksum;					
} CIPHER_TEXT_HEADER, *LPCIPHER_TEXT_HEADER;

ACK_RESULT VPH_CT_CollapseStructure(LPCIPHER_TEXT_HEADER pStruct,LPCIPHER_TEXT_HEADER pPreviousStruct,BOOL bSinglePacket);
ACK_RESULT VPH_CT_StructureToBuffer(LPCIPHER_TEXT_HEADER pStruct,LPBYTE pBuffer,SIZE_T* pSizeBuffer);
ACK_RESULT VPH_CT_BufferToStructure(LPBYTE pBuffer,SIZE_T* psizeBuffer,LPCIPHER_TEXT_HEADER pStruct);
ACK_RESULT VPH_CT_ExpandStructure(LPCIPHER_TEXT_HEADER pStruct,LPCIPHER_TEXT_HEADER pPreviousStruct,SIZE_T sizePacket);

#ifndef IsEqualGUID
#define IsEqualGUID(a,b) (memcmp((a), (b), sizeof(GUID)) == 0)
#endif /* IsEqualGUID */

#if !defined(WIN32) && !defined(_WIN32_WCE)
extern GUID GUID_NULL;
#endif

#endif /* _CIPHERTEXTHEADER_H_ */
