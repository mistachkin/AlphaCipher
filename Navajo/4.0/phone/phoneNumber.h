/*
 * phoneNumber.h --
 *
 * Copyright (c) 2009-2026 by Joe Mistachkin.  All rights reserved.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * written by: Joe Mistachkin
 *
 * RCS: @(#) $Id: $
 */

#if !defined(_WINDOWS_) && !defined(__WINDOWS__)
#error "The header file <windows.h> must be included prior to this file."
#endif

#if !defined(TAPI_H)
#error "The header file <tapi.h> must be included prior to this file."
#endif

#if !defined(_TSP_H_)
#error "The header file <tsp.h> must be included prior to this file."
#endif

#ifndef _PHONE_NUMBER_H_
#define _PHONE_NUMBER_H_

/*****************************************************************************/

HRESULT SHGetPhoneNumber(
    LPTSTR szNumber,
    UINT cchNumber,
    UINT nLineNumber
);

/*****************************************************************************/

#endif /* _PHONE_NUMBER_H_ */

/* end of file*/
