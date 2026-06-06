/*
 * protocol.c -- Public Protocol API
 *
 * Copyright (c) 2009-2026 by Joe Mistachkin.  All rights reserved.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) $Id: $
 */

#if !defined(FEATURE_KERNEL_ONLY)

#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "navajoPort.h"
#include "navajo.h"
#include "navajoProtocol.h"
#include "navajoSqlite.h"
#include "navajoIntTypes.h"
#include "navajoInt.h"
#include "navajoUtil.h"
#include "hresult.h"
#include "puke.h"
#include "CipherTextHeader.h"

#ifndef ACKP_PATH_MAX
/*
 * NOTE: PATH_MAX is the POSIX upper bound on a complete pathname.  It is
 *       not guaranteed to be defined on every platform we target, so a
 *       reasonable fallback is provided.
 */
#ifdef PATH_MAX
#define ACKP_PATH_MAX (PATH_MAX)
#else
#define ACKP_PATH_MAX (4096)
#endif
#endif

#ifndef _ACKP_PROVIDERINFO_DEFINED
#define _ACKP_PROVIDERINFO_DEFINED
typedef struct ACKP_KEYCHAIN_LINK_tag {
	struct ACKP_KEYCHAIN_LINK_tag* pNextKeyList;	//Link to the next item in the list of key lists.
	ACK_SESSION ackSession;
	LPSTR lpszDBPath;
	INT64 i64KeyCount;
	ACK_LPKEYINFO packKeyInfo;
} ACKP_KEYCHAIN_LINK, *LPACKP_KEYCHAIN_LINK;

typedef struct ACKP_PROVIDERINFO_tag {
	int iRefCount;							//Reference count on the provider.
	LPACKP_KEYCHAIN_LINK pKeyListHeader;	//The header to a key chain.
} ACKP_PROVIDERINFO, *LPACKP_PROVIDERINFO;
#endif

LPSTR pathAddSeparator(LPSTR szPath, SIZE_T sizeBuffer)
{
	char chSeparator = '\\';
	SIZE_T length = 0;

	if (szPath == NULL)
		return NULL;

	length = strlen(szPath);

	//Check for existance of separator.
	if ((length > 0) && (szPath[length - 1] == chSeparator))
		return szPath;

	//Refuse to append when there is no room for the separator + NUL.
	if (length + 2 > sizeBuffer)
		return NULL;

	szPath[length] = chSeparator;
	szPath[length + 1] = '\0';
	return szPath;
}

ACK_RESULT ACKP_CreateProvider(ACKP_LPPROVIDER phProvider)
{
	USES_RTN;
	static ACKP_PROVIDERINFO theProviderInfo = {0};

	RTN_IF_BADPTR(phProvider);
	RTN_IF_NOT_NULL(*phProvider);

	//XXXXXX:EnterCriticalSection(ProviderSection)
	if (theProviderInfo.iRefCount == 0)
		RTN_IF_NOT_NULL(theProviderInfo.pKeyListHeader);
	theProviderInfo.iRefCount++;
	*phProvider = (ACKP_PROVIDER) &theProviderInfo;
	//XXXXXX:LeaveCriticalSection(ProviderSection)

	BEGIN_RTN_CLEAN_UP;
	END_RTN_CLEAN_UP;
}

ACK_RESULT ACKP_CloseProvider(ACKP_PROVIDER hProvider)
{
	USES_RTN;
	LPACKP_PROVIDERINFO pProviderInfo = (LPACKP_PROVIDERINFO) hProvider;

	RTN_IF_BADHANDLE(pProviderInfo);

	//XXXXXX:EnterCriticalSection(ProviderSection)
	pProviderInfo->iRefCount--;
	if (pProviderInfo->iRefCount == 0)
	{
		//Clear out the key chain.
		while (pProviderInfo->pKeyListHeader != NULL)
		{
			LPACKP_KEYCHAIN_LINK linkToFree = pProviderInfo->pKeyListHeader;

			RTN_IF_FAILED(ACK_FreeAllKeys(linkToFree->ackSession,NULL,&linkToFree->i64KeyCount,&linkToFree->packKeyInfo));
			if (ACK_IsKeySetMounted(linkToFree->ackSession) == S_OK)
				RTN_IF_FAILED(ACK_UnmountKeySet(linkToFree->ackSession));
			RTN_IF_FAILED(ACK_CloseSession(&linkToFree->ackSession));

			//Maintain the key chain.
			pProviderInfo->pKeyListHeader = linkToFree->pNextKeyList;

			//Free allocated memory.
			ACK_free(linkToFree->lpszDBPath);
			ACK_free(linkToFree);
		}
		pProviderInfo->pKeyListHeader = NULL;
	}
	//XXXXXX:LeaveCriticalSection(ProviderSection)

	BEGIN_RTN_CLEAN_UP;
	END_RTN_CLEAN_UP;
}

ACK_RESULT ACKP_ScanPathForKeys(ACKP_PROVIDER hProvider,LPSTR lpszPath)
{
	USES_RTN;
	LPACKP_PROVIDERINFO pProviderInfo = (LPACKP_PROVIDERINFO) hProvider;
	LPACKP_KEYCHAIN_LINK pLink = NULL;
	char szPath[ACKP_PATH_MAX + 1] = {0};
	char szSearchName[] = ".pef";
	LPDIR pDir = NULL;
	LPDIRENT pDirEntry = NULL;

	RTN_IF_BADHANDLE(pProviderInfo);
	RTN_IF_BADPTR(lpszPath);

	pDir = ACK_opendir(lpszPath);
	RTN_IF_BADPTR(pDir);

	//XXXXXX:Need to filter out sqlite databases no longer accessible.
	//XXXXXX:Need to add new sqlite databases only when not already in the list.
	pDirEntry = ACK_readdir(pDir);
	while (pDirEntry != NULL)
	{
		LPSTR szExt = strstr(&pDirEntry->d_name[0],&szSearchName[0]);
		printf("#%d = \"%s\"\n",
			(unsigned int)pDirEntry->d_ino,
			pDirEntry->d_name);

		//Check to see that we truly end with the search string.
		if (szExt != NULL)
		{
			if (szExt[strlen(szSearchName)] != 0)
				szExt = NULL;
		}
		if (szExt != NULL)
		{
			pLink = (LPACKP_KEYCHAIN_LINK) ACK_zalloc(sizeof(ACKP_KEYCHAIN_LINK));
			RTN_IF_BADPTR(pLink);

			RTN_IF_FAILED(ACK_CreateSession(&pLink->ackSession,ACKST_ListKeys,0));

			//Compose the full path with bounded snprintf to prevent overflow.
			szPath[0] = '\0';
			if ((int)snprintf(szPath, sizeof(szPath), "%s", lpszPath) >=
					(int)sizeof(szPath))
				RTN_IF_FAILED(E_OVERFLOW);
			if (pathAddSeparator(szPath, sizeof(szPath)) == NULL)
				RTN_IF_FAILED(E_OVERFLOW);
			if (strlen(szPath) + strlen(pDirEntry->d_name) + 1 >
					sizeof(szPath))
				RTN_IF_FAILED(E_OVERFLOW);
			ACK_strcat(&szPath[0],pDirEntry->d_name);
			RTN_IF_FAILED(ACK_MountKeySet(pLink->ackSession,szPath));
			pLink->lpszDBPath = ACK_strdup(szPath);
			RTN_IF_BADPTR(pLink->lpszDBPath);

			RTN_IF_FAILED(ACK_GetAllKeys(pLink->ackSession,ACK_KEYINFO_VERSION,NULL,&pLink->i64KeyCount,&pLink->packKeyInfo));

			pLink->pNextKeyList = pProviderInfo->pKeyListHeader;
			pProviderInfo->pKeyListHeader = pLink;
			pLink = NULL;
		}
		pDirEntry = ACK_readdir(pDir);
	}

	BEGIN_RTN_CLEAN_UP
		if (pLink)
		{
			ACK_FreeAllKeys(pLink->ackSession,NULL,&pLink->i64KeyCount,&pLink->packKeyInfo);
			if (ACK_IsKeySetMounted(pLink->ackSession) == S_OK)
				ACK_UnmountKeySet(pLink->ackSession);
			ACK_CloseSession(&pLink->ackSession);
			ACK_free(pLink);
		}
		ACK_closedir(pDir);
	END_RTN_CLEAN_UP;
}

ACK_RESULT ACKP_GetAllKeys(ACKP_PROVIDER hProvider,LPINT pCount,ACK_LPKEYINFO** pKeyInfo)
{
	USES_RTN;
	LPACKP_PROVIDERINFO pProviderInfo = (LPACKP_PROVIDERINFO) hProvider;
	INT64 i64Count = 0;
	int iCount = 0;
	ACK_LPKEYINFO* pAllKeyInfo = NULL;
	int iIndex = 0;
	LPACKP_KEYCHAIN_LINK p = NULL;

	RTN_IF_BADHANDLE(pProviderInfo);
	RTN_IF_BADPTR(pCount);
	RTN_IF_BADPTR(pKeyInfo);

	*pCount = 0;

	//Sum as INT64 to detect overflow before truncating to int for ACK_malloc.
	p = pProviderInfo->pKeyListHeader;
	while (p != NULL)
	{
		LPACKP_KEYCHAIN_LINK next = p->pNextKeyList;
		if (p->i64KeyCount < 0 || p->i64KeyCount > (INT64)INT_MAX - i64Count)
			RTN_IF_FAILED(E_OVERFLOW);
		i64Count += p->i64KeyCount;
		p = next;
	}

	iCount = (int) i64Count;
	pAllKeyInfo = (ACK_LPKEYINFO*) ACK_malloc(sizeof(ACK_LPKEYINFO) * iCount);
	RTN_IF_BADNEW(pAllKeyInfo);

	p = pProviderInfo->pKeyListHeader;
	while (p != NULL)
	{
		LPACKP_KEYCHAIN_LINK next = p->pNextKeyList;
		INT64 index;
		for (index = 0;index < p->i64KeyCount;index++)
		{
			pAllKeyInfo[iIndex] = &p->packKeyInfo[index];
			iIndex++;
		}
		p = next;
	}

	ACK_free(*pKeyInfo);
	*pKeyInfo = pAllKeyInfo;
	pAllKeyInfo = NULL;
	*pCount = iCount;

	BEGIN_RTN_CLEAN_UP
		ACK_free(pAllKeyInfo);
	END_RTN_CLEAN_UP;
}

#ifndef _ACP_SESSIONINFO_DEFINED
#define _ACP_SESSIONINFO_DEFINED
typedef struct ACP_SESSIONINFO_tag {
	ACKP_PROVIDER		hProvider;
	ACK_SESSION			hKernelSession;
	ACK_SESSIONTYPE		type;
	GUID				guidSessionKey;
	LPBYTE				pBodyBuffer;			//Keep allocated buffer for next cipher round.
	SIZE_T				sizeBodyBuffer;
	CIPHER_TEXT_HEADER	ctPreviousHeader;
} ACP_SESSIONINFO, *LPACP_SESSIONINFO;
#endif

/******************************************************************************
** Convert GUID into a HEX string.
******************************************************************************/
void GuidToHex(const GUID guid,LPSTR lpszGuid)
{
	/*
	 * NOTE: The caller must provide a buffer of at least MAX_GUID_SPACE
	 *       bytes (currently 38).  snprintf bounds the write so a smaller
	 *       buffer yields a truncated GUID rather than a buffer overflow.
	 */
	snprintf(lpszGuid,MAX_GUID_SPACE,"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
						guid.Data1,
						guid.Data2,
						guid.Data3,
						guid.Data4[0],
						guid.Data4[1],
						guid.Data4[2],
						guid.Data4[3],
						guid.Data4[4],
						guid.Data4[5],
						guid.Data4[6],
						guid.Data4[7]);
}

/******************************************************************************
** Convert HEX string into a GUID.
******************************************************************************/
BOOL HexToGuid(LPCSTR lpszGuid,GUID* pGuid)
{
	UINT32 dw1, dw2, dw3, dw4, dw5, dw6, dw7, dw8, dw9, dw10, dw11;
	if (11 == sscanf(lpszGuid,"%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
									&dw1, &dw2, &dw3, &dw4, &dw5, &dw6, &dw7, &dw8, &dw9, &dw10, &dw11))
	{
		pGuid->Data1 = dw1;
		pGuid->Data2 = (unsigned short) dw2;
		pGuid->Data3 = (unsigned short) dw3;
		pGuid->Data4[0] = (BYTE) dw4;
		pGuid->Data4[1] = (BYTE) dw5;
		pGuid->Data4[2] = (BYTE) dw6;
		pGuid->Data4[3] = (BYTE) dw7;
		pGuid->Data4[4] = (BYTE) dw8;
		pGuid->Data4[5] = (BYTE) dw9;
		pGuid->Data4[6] = (BYTE) dw10;
		pGuid->Data4[7] = (BYTE) dw11;
		return TRUE;
	}
	else
		return FALSE;
}

ACK_RESULT ACP_CreateSession(ACP_LPSESSION phSession,ACK_SESSIONTYPE type,ACKP_PROVIDER hProvider)
{
	USES_RTN;
	LPACP_SESSIONINFO pSessionInfo = NULL;

	RTN_IF_BADPTR(phSession);
	RTN_IF_NOT_NULL(*phSession);
	RTN_IF_BADHANDLE(hProvider);

	switch (type & (ACKST_Stream | ACKST_Block | ACKST_File))
	{
		case ACKST_Stream:
		{
			pSessionInfo = (LPACP_SESSIONINFO) ACK_zalloc(sizeof(ACP_SESSIONINFO));
			RTN_IF_BADNEW(pSessionInfo);

			pSessionInfo->hProvider = hProvider;
			RTN_IF_FAILED(ACK_CreateSession(&pSessionInfo->hKernelSession,type,0));
			pSessionInfo->type = type;

			*phSession = (ACP_SESSION) pSessionInfo;	//XXXXXX:AddRef the provider
			pSessionInfo = NULL;
			break;
		}
		case ACKST_Block:
		case ACKST_File:
			//These session types will be supported one day.
			return ACP_E_SESSION_TYPE_NOT_SUPPORTED;
		default:
			//The other remaining session types will never be supported.
			return ACP_E_SESSION_TYPE_NOT_SUPPORTED;
	}

	BEGIN_RTN_CLEAN_UP
		if (pSessionInfo != NULL)
		{
			if (pSessionInfo->hKernelSession != NULL)
				ACK_CloseSession(&pSessionInfo->hKernelSession);
			ACK_free(pSessionInfo);
		}
	END_RTN_CLEAN_UP;
}

ACK_RESULT ACP_CloseSession(ACP_SESSION hSession)
{
	USES_RTN;
	LPACP_SESSIONINFO pSessionInfo = (LPACP_SESSIONINFO) hSession;

	RTN_IF_BADHANDLE(pSessionInfo);

	if (ACK_IsKeySetMounted(pSessionInfo->hKernelSession) == S_OK)
		RTN_IF_FAILED(ACK_UnmountKeySet(pSessionInfo->hKernelSession));
	RTN_IF_FAILED(ACK_CloseSession(&pSessionInfo->hKernelSession));
	ACK_free(pSessionInfo->pBodyBuffer);
	ACK_free(pSessionInfo);

	BEGIN_RTN_CLEAN_UP;
	END_RTN_CLEAN_UP;
}

ACK_RESULT ACP_SetSessionKey(ACP_SESSION hSession,LPSTR lpStrGuid)
{
	USES_RTN;
	LPACP_SESSIONINFO pSessionInfo = (LPACP_SESSIONINFO) hSession;
	LPACKP_KEYCHAIN_LINK p = NULL;
	LPSTR lpszDBPath = NULL;

	RTN_IF_BADHANDLE(pSessionInfo);
	RTN_IF_BADPTR(lpStrGuid);

	//XXXXXX:ACKP_VerifyKeySetAvailability(pSessionInfo->hProvider);

	p = ((LPACKP_PROVIDERINFO) pSessionInfo->hProvider)->pKeyListHeader;
	while (p != NULL)
	{
		INT64 index = 0;
		while (index < p->i64KeyCount)
		{
			if (strcmp(lpStrGuid,p->packKeyInfo[index].keyId) == 0)
			{
				lpszDBPath = p->lpszDBPath;
				break;
			}
			index++;
		}
		p = (lpszDBPath == NULL) ? p->pNextKeyList : NULL;
	}

	if (lpszDBPath == NULL)
	{
		RTN_IF_FAILED(ACP_E_KEY_NOT_FOUND);
	}
	else
	{
		RTN_IF_FAILED(ACK_MountKeySet(pSessionInfo->hKernelSession,lpszDBPath));
		RTN_IF_FAILED(ACK_SetSessionKey(pSessionInfo->hKernelSession,lpStrGuid));
		HexToGuid(lpStrGuid,&pSessionInfo->guidSessionKey);
	}

	BEGIN_RTN_CLEAN_UP;
	END_RTN_CLEAN_UP;
}

ACK_RESULT ACP_Encrypt(ACP_SESSION hSession,UINT32 flags,LPBYTE pInput,SIZE_T inSize,LPBYTE* ppOutput,LPSIZE_T pOutSize)
{
	USES_RTN;
	LPACP_SESSIONINFO pSessionInfo = (LPACP_SESSIONINFO) hSession;
	CIPHER_TEXT_HEADER ctHeader = {0};
	CIPHER_TEXT_HEADER ctPreviousHeader;
	//The size of CIPHER_TEXT_HEADER is way more space than we need
	//for the header but it will ensure that we won't have an overflow.
	SIZE_T sizeHeader = sizeof(CIPHER_TEXT_HEADER);
	SIZE_T sizeCompressed = 0;
	SIZE_T sizeCipherText;
	LPBYTE pCipherText = NULL;

	UNUSED_ARGUMENT(flags);

	RTN_IF_BADHANDLE(pSessionInfo);
	RTN_IF_BADPTR(pInput);
	RTN_IF_BADPTR(ppOutput);
	RTN_IF_BADPTR(pOutSize);

	//NOTE: Compression would be performed here.
	sizeCompressed = inSize;
	sizeCipherText = *pOutSize;

	//Make sure we have an allocated and large enough space for the CipherText.
	if ((*ppOutput == NULL) || (sizeCipherText < (sizeHeader + sizeCompressed)))
	{
		//We need to allocate enough space for the CT header and compressed body.
		sizeCipherText = sizeHeader + sizeCompressed;
		pCipherText = ACK_malloc(sizeCipherText);
		RTN_IF_BADPTR(pCipherText);
	}
	else
	{
		//Use the user allocated buffer that was passed in.
		pCipherText = *ppOutput;
	}

	//Reuse already allocated buffer and size (if exist) for the output; call the cipher.
	RTN_IF_FAILED(ACK_Encrypt(pSessionInfo->hKernelSession,NULL,0,&ctHeader.u64DecryptKeyOffset,
								pInput,sizeCompressed,&pSessionInfo->pBodyBuffer,&pSessionInfo->sizeBodyBuffer));

	//Fill in CT header data structure.
	ctHeader.bInitialized = TRUE;
	ctHeader.guidDecryptKeyIdentifier = pSessionInfo->guidSessionKey;
	ctHeader.u64CipherTextLength = pSessionInfo->sizeBodyBuffer;

	//Save the previous CT header to see if the current CT header can be collapsed.
	ctPreviousHeader = pSessionInfo->ctPreviousHeader;
	//Current CT header becomes previous CT header for next iteration.
	pSessionInfo->ctPreviousHeader = ctHeader;
//XXXXXX:Disable the header collapsing feature due to UTP packet losses.
//	RTN_IF_FAILED(VPH_CT_CollapseStructure(&ctHeader,&ctPreviousHeader,(pSessionInfo->type & ACKST_Stream)));

	//On return, sizeHeader will contain the actual size of the CT header, not just the MAX size.
	RTN_IF_FAILED(VPH_CT_StructureToBuffer(&ctHeader,pCipherText,&sizeHeader));

	if ((sizeCipherText < (sizeHeader + pSessionInfo->sizeBodyBuffer)))
	{
		//The ciphertext size is larger than expected, so we need to increase our buffer.
		LPBYTE p = ACK_malloc(sizeHeader + pSessionInfo->sizeBodyBuffer);
		RTN_IF_BADPTR(p);
		memcpy(p,pCipherText,sizeHeader);
		ACK_free(pCipherText);
		pCipherText = p;
		sizeCipherText = sizeHeader + pSessionInfo->sizeBodyBuffer;
	}
	//Append the body.
	memcpy(&pCipherText[sizeHeader],pSessionInfo->pBodyBuffer,pSessionInfo->sizeBodyBuffer);

	//If the passed in buffer was reallocated, release the passed in buffer.
	if (pCipherText != *ppOutput)
	{
		ACK_free(*ppOutput);
		*ppOutput = pCipherText;
		pCipherText = NULL;
	}
	else
		pCipherText = NULL;		//Buffer was not reallocated.
	//Return the actual size of the ciphertext buffer.
	*pOutSize = sizeHeader + pSessionInfo->sizeBodyBuffer;

	BEGIN_RTN_CLEAN_UP
		ACK_free(pCipherText);
	END_RTN_CLEAN_UP;
}

ACK_RESULT ACP_Decrypt(ACP_SESSION hSession,UINT32 flags,LPBYTE pInput,SIZE_T inSize,LPBYTE* ppOutput,LPSIZE_T pOutSize)
{
	USES_RTN;
	LPACP_SESSIONINFO pSessionInfo = (LPACP_SESSIONINFO) hSession;
	CIPHER_TEXT_HEADER ctHeader;
	SIZE_T sizeHeader = inSize;
	SIZE_T sizeBody = 0;
	SIZE_T sizeCT = 0;

	UNUSED_ARGUMENT(flags);

	RTN_IF_BADHANDLE(pSessionInfo);
	RTN_IF_BADPTR(pInput);
	RTN_IF_BADPTR(ppOutput);
	RTN_IF_BADPTR(pOutSize);

	RTN_IF_FAILED(VPH_CT_BufferToStructure(pInput,&sizeHeader,&ctHeader));
//XXXXXX:Disable the header collapsing feature due to UTP packet losses.
//	RTN_IF_FAILED(VPH_CT_ExpandStructure(&ctHeader,&pSessionInfo->ctPreviousHeader,inSize));
	//Current CT header becomes previous CT header for next iteration.
	pSessionInfo->ctPreviousHeader = ctHeader;

	if (!IsEqualGUID(&ctHeader.guidDecryptKeyIdentifier,&GUID_NULL) &&
		!IsEqualGUID(&ctHeader.guidDecryptKeyIdentifier,&pSessionInfo->guidSessionKey))
	{
		char szGuid[ACK_KEYINFO_MAXID];
		GuidToHex(ctHeader.guidDecryptKeyIdentifier,&szGuid[0]);
		RTN_IF_FAILED(ACP_SetSessionKey(hSession,&szGuid[0]));
	}

	sizeCT = (SIZE_T) ctHeader.u64CipherTextLength;
	sizeBody = *pOutSize;
	//Reuse already allocated buffer and size (if exist) for the output; call the cipher.
	RTN_IF_FAILED(ACK_Decrypt(pSessionInfo->hKernelSession,NULL,0,ctHeader.u64DecryptKeyOffset,
								&pInput[sizeHeader],sizeCT,ppOutput,&sizeBody));

	*pOutSize = sizeBody;

	BEGIN_RTN_CLEAN_UP;
	END_RTN_CLEAN_UP;
}
#endif

/* end of file */
