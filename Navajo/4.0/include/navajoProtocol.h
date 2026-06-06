/*
 * navajoProtocol.h -- Public Protocol API
 *
 * Copyright (c) 2008-2026 by Joe Mistachkin.  All rights reserved.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * written by: Joe Mistachkin
 *
 * RCS: @(#) $Id: $
 */

#if !defined(_NAVAJO_PORT_H_)
#error "The header file \"navajoPort.h\" must be included prior to this file."
#endif

#if !defined(_NAVAJO_H_)
#error "The header file \"navajo.h\" must be included prior to this file."
#endif

#ifndef _NAVAJO_PROTOCOL_H_
#define _NAVAJO_PROTOCOL_H_

/*****************************************************************************/

#ifndef _ACKP_PROVIDER_DEFINED
#define _ACKP_PROVIDER_DEFINED
typedef struct _ACKP_PROVIDER _ACKP_PROVIDER;
typedef _ACKP_PROVIDER *ACKP_PROVIDER;
#endif

#ifndef _ACKP_LPPROVIDER_DEFINED
#define _ACKP_LPPROVIDER_DEFINED
typedef ACKP_PROVIDER *ACKP_LPPROVIDER;
#endif

/*****************************************************************************/

/*
** These functions are responsible for managing the key provider.  The key
** provider has access to all the keys available to it, which can then be
** passed to the protocol layer for use in the encryption/decryption.  The
** provider is created and destroyed using the ACKP_CreateProvider and
** ACKP_CloseProvider functions, respectively.  ACKP_ScanPathForKeys is used
** to scan for KeySets on a given path.  ACKP_GetAllKeys will return all the
** keys that are available to the provider after it has scaned a path.
*/

ACK_EXPORT ACK_RESULT ACKP_CreateProvider(
    ACKP_LPPROVIDER pProvider
);

ACK_EXPORT ACK_RESULT ACKP_CloseProvider(
    ACKP_PROVIDER provider
);

ACK_EXPORT ACK_RESULT ACKP_ScanPathForKeys(
    ACKP_PROVIDER provider,
    LPSTR path
);

ACK_EXPORT ACK_RESULT ACKP_GetAllKeys(
    ACKP_PROVIDER provider,
    LPINT pCount,
    ACK_LPKEYINFO **pppKeyInfo
);

/*****************************************************************************/

#ifndef _ACP_SESSION_DEFINED
#define _ACP_SESSION_DEFINED
typedef struct _ACP_SESSION _ACP_SESSION;
typedef _ACP_SESSION *ACP_SESSION;
#endif

#ifndef _ACP_LPSESSION_DEFINED
#define _ACP_LPSESSION_DEFINED
typedef ACP_SESSION *ACP_LPSESSION;
#endif

/*****************************************************************************/

#ifndef ACP_E_SESSION_TYPE_NOT_SUPPORTED
#define ACP_E_SESSION_TYPE_NOT_SUPPORTED \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ACP, 0x1)
#endif

#ifndef ACP_E_KEY_NOT_FOUND
#define ACP_E_KEY_NOT_FOUND\
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ACP, 0x2)
#endif

/*****************************************************************************/

/*
** These functions are responsible for the protocol layer, which encapsulates
** the kernel layer into a more user friendly operation.  The protocol layer
** is created and destroyed using the ACP_CreateSession and ACP_CloseSession
** functions, respectively.  ACP_CreateSession requires a previously created
** provider for key access.  ACP_SetSessionKey is used to set the session key
** to use for encryption, while decryption does not need to make this call.
** ACP_Encrypt is used for encryption and ACP_Decrypt is used for decryption.
*/

ACK_EXPORT ACK_RESULT ACP_CreateSession(
    ACP_LPSESSION pSession,
    ACK_SESSIONTYPE type,
    ACKP_PROVIDER provider
);

ACK_EXPORT ACK_RESULT ACP_CloseSession(
    ACP_SESSION session
);

ACK_EXPORT ACK_RESULT ACP_SetSessionKey(
    ACP_SESSION session,
    LPSTR keyId
);

ACK_EXPORT ACK_RESULT ACP_Encrypt(
    ACP_SESSION session,
    UINT32 flags,
    LPBYTE pInput,
    SIZE_T inSize,
    LPBYTE *ppOutput,
    LPSIZE_T pOutSize
);

ACK_EXPORT ACK_RESULT ACP_Decrypt(
    ACP_SESSION session,
    UINT32 flags,
    LPBYTE pInput,
    SIZE_T inSize,
    LPBYTE *ppOutput,
    LPSIZE_T pOutSize
);

/*****************************************************************************/

#endif /* _NAVAJO_PROTOCOL_H_ */

/* end of file */
