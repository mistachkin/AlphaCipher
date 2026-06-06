/*
 * puke.h -- Error handling macros
 *
 * Copyright (c) 2008-2026 by Joe Mistachkin.  All rights reserved.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) $Id: $
 */

#include "hresult.h"

#ifndef USES_RTN
#define USES_RTN ACK_RESULT hrRes = S_OK
#endif

#ifndef RTN_IF_FAILED
#define RTN_IF_FAILED(expr) { hrRes = (expr); if (FAILED(hrRes)) goto cleanup; }
#endif

#ifndef RTN_IF_BADNEW
#define RTN_IF_BADNEW(expr) if ((expr) == NULL) { hrRes = E_OUTOFMEMORY; goto cleanup; }
#endif

#ifndef RTN_IF_BADPTR
#define RTN_IF_BADPTR(expr) if ((expr) == NULL) { hrRes = E_POINTER; goto cleanup; }
#endif

#ifndef RTN_IF_BADHANDLE
#define RTN_IF_BADHANDLE(expr) if ((expr) == NULL) { hrRes = E_HANDLE; goto cleanup; }
#endif

#ifndef RTN_IF_NOT_NULL
#define RTN_IF_NOT_NULL(expr) if ((expr)) { hrRes = E_MUSTBENULL; goto cleanup; }
#endif

#ifndef RTN_IF_BADMAGIC
#define RTN_IF_BADMAGIC(expr,cookie) if ((expr) != (cookie)) { hrRes = E_BADMAGIC; goto cleanup; }
#endif

#ifndef RTN_IF_INVALID_SOCKET
#define RTN_IF_INVALID_SOCKET(expr) { if ((expr) == INVALID_SOCKET) { hrRes = WSAGetLastError(); hrRes = HRESULT_FROM_WIN32(hrRes); goto cleanup; } }
#endif

#ifndef RTN_IF_SOCKET_ERROR
#define RTN_IF_SOCKET_ERROR(expr) { if ((expr) == SOCKET_ERROR) { hrRes = WSAGetLastError(); hrRes = HRESULT_FROM_WIN32(hrRes); goto cleanup; } }
#endif

#ifndef BEGIN_RTN_CLEAN_UP
#define BEGIN_RTN_CLEAN_UP cleanup:
#endif

#ifndef END_RTN_CLEAN_UP
#define END_RTN_CLEAN_UP return hrRes
#endif
