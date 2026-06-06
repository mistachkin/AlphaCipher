/*
 * navajo.h -- Public Engine API
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

#if defined(_MSC_VER) && defined(WIN32) && !defined(_WIN32_WCE) && !defined(_INC_ERRNO)
#error "The header file <errno.h> must be included prior to this file."
#endif

#if defined(_MSC_VER) && defined(WIN32) && !defined(_WIN32_WCE) && !defined(_INC_STDDEF)
#error "The header file <stddef.h> must be included prior to this file."
#endif

#if !defined(_NAVAJO_PORT_H_)
#error "The header file \"navajoPort.h\" must be included prior to this file."
#endif

#ifndef _NAVAJO_H_
#define _NAVAJO_H_

/*****************************************************************************/

#ifndef FALSE
#define FALSE                                (0)
#endif

#ifndef TRUE
#define TRUE                                 (1)
#endif

#ifndef ENOERR
#define ENOERR                               (0)
#endif

/*****************************************************************************/

/*
 * NOTE: This comment is required for the Symbian build system to pickup the
 *       exports declared in this file.  PLEASE DO NOT REMOVE IT.
 */
/* EXPORT_C */

#ifndef _ACK_IMPORT_DEFINED
    #define _ACK_IMPORT_DEFINED
    #if defined(WIN32)
        #if defined(_MSC_VER)
            #ifdef __cplusplus
                #define ACK_IMPORT extern "C" __declspec(dllimport)
            #else
                #define ACK_IMPORT extern __declspec(dllimport)
            #endif
        #elif defined(__GNUC__) && !defined(NO_VIZ)
            #ifdef __cplusplus
                #define ACK_IMPORT extern "C" __attribute__((dllimport))
            #else
                #define ACK_IMPORT extern __attribute__((dllimport))
            #endif
        #else
            #define ACK_IMPORT
        #endif
    #else
        #define ACK_IMPORT
    #endif
#endif

/*****************************************************************************/

#ifndef _ACK_EXPORT_DEFINED
    #define _ACK_EXPORT_DEFINED
    #if defined(WIN32)
        #if defined(_MSC_VER)
            #ifdef __cplusplus
                #define ACK_EXPORT extern "C" __declspec(dllexport)
            #else
                #define ACK_EXPORT extern __declspec(dllexport)
            #endif
        #elif defined(__GNUC__) && !defined(NO_VIZ)
            #ifdef __cplusplus
                #define ACK_EXPORT extern "C" __attribute__((dllexport))
            #else
                #define ACK_EXPORT extern __attribute__((dllexport))
            #endif
        #else
            #define ACK_EXPORT
        #endif
    #elif defined(__GNUC__) && !defined(NO_VIZ)
        #ifdef __cplusplus
            #define ACK_EXPORT extern "C" __attribute__ ((visibility("default")))
        #else
            #define ACK_EXPORT extern __attribute__ ((visibility("default")))
        #endif
    #else
        #ifdef __cplusplus
            #define ACK_EXPORT extern "C"
        #else
            #define ACK_EXPORT extern
        #endif
    #endif
#endif

/*****************************************************************************/

#ifndef ACK_API
    /*
     * NOTE: Check and see if we are building the library itself.  This is used
     *       to determine if we are producing (export) or consuming (import)
     *       the library functions.  This define may not be set by third-party
     *       code.
     */

    #ifdef ACK_BUILD
        /*
         * NOTE: Check if we are producing a static library.  If so, there is
         *       no need to decorate the functions; otherwise, we must decorate
         *       them as exported library functions.
         */

        #ifdef ACK_STATIC_LIB
            #define ACK_API
        #else
            #define ACK_API ACK_EXPORT
        #endif
    #else
        /*
         * NOTE: Check if we are consuming a static library.  If so, there is
         *       no need to decorate the functions; otherwise, we must decorate
         *       them as imported library functions.
         */

        #ifdef ACK_STATIC_LIB
            #define ACK_API
        #else
            #define ACK_API ACK_IMPORT
        #endif
    #endif
#endif

/*****************************************************************************/

/*
 * NOTE: This is used to decorate functions that are not intended for public
 *       use (i.e. we do not intend to export them and they should not be
 *       called from outside the library).
 */

#ifndef ACK_PRIVATE
#define ACK_PRIVATE
#endif

/*****************************************************************************/

/*
 * NOTE: Severity and facility codes defined by this library.  These codes are
 *       semantically compatible with HRESULT.
 */

#ifndef SEVERITY_SUCCESS
#define SEVERITY_SUCCESS                       0
#endif

#ifndef SEVERITY_ERROR
#define SEVERITY_ERROR                         1
#endif

#ifndef FACILITY_ACK
#define FACILITY_ACK                          95
#endif

#ifndef FACILITY_ACP
#define FACILITY_ACP                          96
#endif

/*****************************************************************************/

#ifndef _HRESULT_DEFINED
#define _HRESULT_DEFINED
/*
 * NOTE: HRESULT must be exactly 32 bits wide so the high (sign) bit
 *       carries failure status as on Windows.  On 64-bit Unix targets
 *       'long' is 64 bits, which would silently inflate error codes
 *       to positive values and break SUCCEEDED()/FAILED().
 */
#if defined(WIN32) || defined(_WIN32_WCE)
typedef long HRESULT;
#else
typedef int HRESULT;
#endif
#endif

#ifndef MAKE_HRESULT
#define MAKE_HRESULT(severity,facility,code) \
    ((HRESULT) (((unsigned long)(severity) << 31) | \
                ((unsigned long)(facility) << 16) | \
                ((unsigned long)(code))))
#endif

#ifndef SUCCEEDED
#define SUCCEEDED(hr) ((HRESULT)(hr) >= 0)
#endif

#ifndef FAILED
#define FAILED(hr) ((HRESULT)(hr) < 0)
#endif

/*****************************************************************************/

/*
 * NOTE: Success and failure codes returned by this library.  These codes are
 *       semantically compatible with HRESULT.
 */

#ifndef ACK_S_OK
#define ACK_S_OK \
    MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_ACK, 0x0)
#endif

#ifndef ACK_S_FALSE
#define ACK_S_FALSE \
    MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_ACK, 0x1)
#endif

#ifndef ACK_E_MISMATCHED_SESSION
#define ACK_E_MISMATCHED_SESSION \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ACK, 0x1)
#endif

#ifndef ACK_E_WRONG_SESSION_TYPE
#define ACK_E_WRONG_SESSION_TYPE \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ACK, 0x2)
#endif

#ifndef ACK_E_KEY_FOR_SESSION
#define ACK_E_KEY_FOR_SESSION \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ACK, 0x3)
#endif

#ifndef ACK_E_NO_KEY_FOR_SESSION
#define ACK_E_NO_KEY_FOR_SESSION \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ACK, 0x4)
#endif

#ifndef ACK_E_NO_FILE_NAME_FOR_KEY
#define ACK_E_NO_FILE_NAME_FOR_KEY \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ACK, 0x5)
#endif

#ifndef ACK_E_NO_KEY_DATABASE_FOR_SESSION
#define ACK_E_NO_KEY_DATABASE_FOR_SESSION \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ACK, 0x6)
#endif

#ifndef ACK_E_MOUNTED_DATABASE
#define ACK_E_MOUNTED_DATABASE \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ACK, 0x7)
#endif

#ifndef ACK_E_NO_MOUNTED_DATABASE
#define ACK_E_NO_MOUNTED_DATABASE \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ACK, 0x8)
#endif

#ifndef ACK_E_UNSUPPORTED_VERSION
#define ACK_E_UNSUPPORTED_VERSION \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ACK, 0x9)
#endif

#ifndef ACK_E_KEY_IS_DECRYPT_ONLY
#define ACK_E_KEY_IS_DECRYPT_ONLY \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ACK, 0xA)
#endif

#ifndef ACK_E_KEY_IS_REVOKED
#define ACK_E_KEY_IS_REVOKED \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ACK, 0xB)
#endif

#ifndef ACK_E_KEY_IS_TOO_SMALL
#define ACK_E_KEY_IS_TOO_SMALL \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ACK, 0xC)
#endif

#ifndef ACK_E_INSUFFICIENT_KEY_BYTES_REMAIN
#define ACK_E_INSUFFICIENT_KEY_BYTES_REMAIN \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ACK, 0xD)
#endif

#ifndef ACK_E_OFFSET_PLUS_SIZE_EXCEEDS_TOTAL
#define ACK_E_OFFSET_PLUS_SIZE_EXCEEDS_TOTAL \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ACK, 0xE)
#endif

#ifndef ACK_E_MAXIMUM_KEY_SIZE_EXCEEDED
#define ACK_E_MAXIMUM_KEY_SIZE_EXCEEDED \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ACK, 0xF)
#endif

#ifndef ACK_E_KEY_OFFSET_OVERFLOW
#define ACK_E_KEY_OFFSET_OVERFLOW \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ACK, 0x10)
#endif

#ifndef ACK_E_INVALID_CHUNK_ID
#define ACK_E_INVALID_CHUNK_ID \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ACK, 0x11)
#endif

#ifndef ACK_E_SESSION_CHECK_FAILED
#define ACK_E_SESSION_CHECK_FAILED \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ACK, 0x12)
#endif

#ifndef ACK_E_IO_ERROR
#define ACK_E_IO_ERROR \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ACK, 0x13)
#endif

#ifndef ACK_E_NO_CACHE
#define ACK_E_NO_CACHE \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ACK, 0x14)
#endif

#ifndef ACK_E_NO_STORAGE
#define ACK_E_NO_STORAGE \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ACK, 0x15)
#endif

#ifndef ACK_E_CACHE_IS_TOO_SMALL
#define ACK_E_CACHE_IS_TOO_SMALL \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ACK, 0x16)
#endif

#ifndef ACK_E_TRANSACTION_PENDING
#define ACK_E_TRANSACTION_PENDING \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ACK, 0x17)
#endif

#ifndef ACK_E_ALREADY_DONE
#define ACK_E_ALREADY_DONE \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ACK, 0x18)
#endif

#ifndef ACK_E_MISMATCHED_CHUNK_ID
#define ACK_E_MISMATCHED_CHUNK_ID \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ACK, 0x19)
#endif

#ifndef ACK_E_CHUNK_OFFSET_OVERFLOW
#define ACK_E_CHUNK_OFFSET_OVERFLOW \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ACK, 0x1A)
#endif

#ifndef ACK_E_CHUNK_SIZE_OVERFLOW
#define ACK_E_CHUNK_SIZE_OVERFLOW \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ACK, 0x1B)
#endif

#ifndef ACK_E_KEY_OFFSET_AND_SIZE_OVERFLOW
#define ACK_E_KEY_OFFSET_AND_SIZE_OVERFLOW \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ACK, 0x1C)
#endif

#ifndef ACK_E_CHUNK_OFFSET_AND_SIZE_OVERFLOW
#define ACK_E_CHUNK_OFFSET_AND_SIZE_OVERFLOW \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ACK, 0x1D)
#endif

/*****************************************************************************/

/*
 * NOTE: These flag values are used with the ACK_Encrypt and/or ACK_Decrypt
 *       functions.
 */

#ifndef ACK_F_NONE
#define ACK_F_NONE                           0x0
#endif

/*****************************************************************************/

/*
 * NOTE: These types defintions are for use with this library; however, they
 *       are also somewhat generic.  They can be overridden by defining the
 *       appropriate preprocessor macro to bypass these definitions.
 */

#ifndef _VOID_DEFINED
#define _VOID_DEFINED
#define VOID void
#endif

#ifndef _LPVOID_DEFINED
#define _LPVOID_DEFINED
typedef VOID *LPVOID;
#endif

#ifndef _ERRNO_T_DEFINED
#define _ERRNO_T_DEFINED
#if defined(HAVE_ERRNO_T)
typedef errno_t ERRNO_T;
#else
typedef int ERRNO_T;
#endif
#endif

#if !defined(__SIZE_T_DEFINED) && !defined(_BASETSD_H_) && !defined(_BASETSD_H)
#define __SIZE_T_DEFINED
typedef size_t SIZE_T;
#endif

#ifndef _LPSIZE_T_DEFINED
#define _LPSIZE_T_DEFINED
typedef SIZE_T *LPSIZE_T;
#endif

#ifndef _BOOL_DEFINED
#define _BOOL_DEFINED
typedef int BOOL;
#endif

#ifndef _INT_DEFINED
#define _INT_DEFINED
typedef int INT;
#endif

#ifndef _LPINT_DEFINED
#define _LPINT_DEFINED
typedef INT *LPINT;
#endif

#ifndef _UINT_DEFINED
#define _UINT_DEFINED
typedef unsigned int UINT;
#endif

#ifndef _LPUINT_DEFINED
#define _LPUINT_DEFINED
typedef UINT *LPUINT;
#endif

#ifndef _INT32_DEFINED
#define _INT32_DEFINED
#if defined(HAVE_INT32_T)
typedef int32_t INT32;
#elif defined(HAVE_INT32)
typedef __int32 INT32;
#else
#error "Cannot figure out how to define INT32."
#endif
#endif

#ifndef _LPINT32_DEFINED
#define _LPINT32_DEFINED
typedef INT32 *LPINT32;
#endif

#ifndef _INT64_DEFINED
#define _INT64_DEFINED
#if defined(HAVE_INT64_T)
typedef int64_t INT64;
#elif defined(HAVE_INT64)
typedef __int64 INT64;
#else
#error "Cannot figure out how to define INT64."
#endif
#endif

#ifndef _LPINT64_DEFINED
#define _LPINT64_DEFINED
typedef INT64 *LPINT64;
#endif

#ifndef _UINT32_DEFINED
#define _UINT32_DEFINED
#if defined(HAVE_UINT32_T)
typedef uint32_t UINT32;
#elif defined(HAVE_INT32)
typedef unsigned __int32 UINT32;
#else
#error "Cannot figure out how to define UINT32."
#endif
#endif

#ifndef _LPUINT32_DEFINED
#define _LPUINT32_DEFINED
typedef UINT32 *LPUINT32;
#endif

#ifndef _UINT64_DEFINED
#define _UINT64_DEFINED
#if defined(HAVE_UINT64_T)
typedef uint64_t UINT64;
#elif defined(HAVE_INT64)
typedef unsigned __int64 UINT64;
#else
#error "Cannot figure out how to define UINT64."
#endif
#endif

#ifndef _LPUINT64_DEFINED
#define _LPUINT64_DEFINED
typedef UINT64 *LPUINT64;
#endif

#ifndef _BYTE_DEFINED
#define _BYTE_DEFINED
typedef unsigned char BYTE;
#endif

#ifndef _LPBYTE_DEFINED
#define _LPBYTE_DEFINED
typedef BYTE *LPBYTE;
#endif

#ifndef _LPCBYTE_DEFINED
typedef const BYTE *LPCBYTE;
#endif

#ifndef _CHAR_DEFINED
#define _CHAR_DEFINED
typedef char CHAR;
#endif

#ifndef _LPSTR_DEFINED
#define _LPSTR_DEFINED
typedef CHAR *LPSTR;
#endif

#ifndef _LPCSTR_DEFINED
#define _LPCSTR_DEFINED
typedef const CHAR *LPCSTR;
#endif

#ifndef GUID_DEFINED
#define GUID_DEFINED
typedef struct GUID_tag {
    unsigned long  Data1;    /* 32-bits */
    unsigned short Data2;    /* 16-bits */
    unsigned short Data3;    /* 16-bits */
    unsigned char  Data4[8]; /* 64-bits */
} GUID;
#endif

#ifndef __LPGUID_DEFINED__
#define __LPGUID_DEFINED__
typedef GUID *LPGUID;
#endif

#ifndef __LPCGUID_DEFINED__
#define __LPCGUID_DEFINED__
typedef const GUID *LPCGUID;
#endif

/*****************************************************************************/

/*
 * NOTE: These types defintions are expressly for use with this library.  They
 *       can be overridden by defining the appropriate preprocessor macro to
 *       bypass these definitions.
 */

#ifndef _ACK_RESULT_DEFINED
#define _ACK_RESULT_DEFINED
/*
 * NOTE: ACK_RESULT mirrors HRESULT and must be exactly 32 bits wide so
 *       the high (sign) bit carries failure status.  See the HRESULT
 *       typedef above for further detail.
 */
#if defined(WIN32) || defined(_WIN32_WCE)
typedef long ACK_RESULT;
#else
typedef int ACK_RESULT;
#endif
#endif

#ifndef _ACK_SESSION_DEFINED
#define _ACK_SESSION_DEFINED
typedef struct _ACK_SESSION _ACK_SESSION;
typedef _ACK_SESSION *ACK_SESSION;
#endif

#ifndef _ACK_LPSESSION_DEFINED
#define _ACK_LPSESSION_DEFINED
typedef ACK_SESSION *ACK_LPSESSION;
#endif

#ifndef _ACK_CLIENTDATA_DEFINED
#define _ACK_CLIENTDATA_DEFINED
typedef VOID *ACK_CLIENTDATA;
#endif

#ifndef _ACK_PARAMETER_DEFINED
#define _ACK_PARAMETER_DEFINED
typedef VOID *ACK_PARAMETER;
#endif

#ifndef _ACK_SESSIONTYPE_DEFINED
#define _ACK_SESSIONTYPE_DEFINED
typedef enum {
    /*
     * NOTE: Session types supported by this library.  These are used with the
     *       ACK_CreateSession function and the ACK_SESSIONINFO structure type.
     */

    ACKST_None = 0x0, /* NOTE: RESERVED, do not use. */
    ACKST_Reserved1 = 0x1, /* NOTE: RESERVED, do not use. */
    ACKST_ListKeys = 0x2,
    ACKST_ListKeyProperties = 0x4,
    ACKST_GetKeyProperty = 0x8,
    ACKST_SetKeyProperty = 0x10,
    ACKST_UnsetKeyProperty = 0x20,
    ACKST_PrepareKey = 0x40,
    ACKST_ImportKey = 0x80,
    ACKST_Encrypt = 0x100,
    ACKST_Decrypt = 0x200,
    ACKST_Stream = 0x400,
    ACKST_Block = 0x800,
    ACKST_File = 0x1000,
    ACKST_Managed = 0x2000,
    ACKST_LegacyChunkId = 0x4000,
    ACKST_Reserved2 = 0x80000000 /* NOTE: RESERVED, do not use. */
} ACK_SESSIONTYPE;
#endif

/*****************************************************************************/

#ifndef ACK_KEYINFO_MAXID
#define ACK_KEYINFO_MAXID                    (39)
#endif

#ifndef ACK_KEYINFO_MAXNAME
#define ACK_KEYINFO_MAXNAME                 (261)
#endif

#ifndef ACK_KEYINFO_MAXFILENAME
#define ACK_KEYINFO_MAXFILENAME             (261)
#endif

#ifndef ACK_KEYINFO_VERSION
#define ACK_KEYINFO_VERSION                   (1)
#endif

#ifndef _ACK_KEYINFO_DEFINED
#define _ACK_KEYINFO_DEFINED
typedef struct ACK_KEYINFO_tag {
    INT64 version; /* NOTE: Always has the value of ACK_KEYINFO_VERSION. */
    UINT64 bytesUsed;
    UINT64 bytesTotal;
    BOOL isEncrypt;
    BOOL isArchive;
    BOOL isRevoked;
    BOOL isErasable;
    LPSTR keyId;
    LPSTR keySetId;
    LPSTR keyGroupId;
    LPSTR keyName;
    CHAR aKeyId[ACK_KEYINFO_MAXID];
    CHAR aKeySetId[ACK_KEYINFO_MAXID];
    CHAR aKeyGroupId[ACK_KEYINFO_MAXID];
    CHAR aKeyName[ACK_KEYINFO_MAXNAME];
} ACK_KEYINFO, *ACK_LPKEYINFO;
#endif

/*****************************************************************************/

#ifndef ACK_KEYPROP_MAXTYPE
#define ACK_KEYPROP_MAXTYPE                 (51)
#endif

#ifndef ACK_KEYPROP_MAXNAME
#define ACK_KEYPROP_MAXNAME                 (51)
#endif

#ifndef ACK_KEYPROP_MAXVALUE
#define ACK_KEYPROP_MAXVALUE               (201)
#endif

#ifndef ACK_KEYPROP_VERSION
#define ACK_KEYPROP_VERSION                  (1)
#endif

#ifndef _ACK_KEYPROP_DEFINED
#define _ACK_KEYPROP_DEFINED
typedef struct ACK_KEYPROP_tag {
    INT64 version; /* NOTE: Always has the value of ACK_KEYPROP_VERSION. */
    UINT64 id;
    LPSTR type;
    LPSTR name;
    LPSTR value;
    CHAR aType[ACK_KEYPROP_MAXTYPE];
    CHAR aName[ACK_KEYPROP_MAXNAME];
    CHAR aValue[ACK_KEYPROP_MAXVALUE];
} ACK_KEYPROP, *ACK_LPKEYPROP;
#endif

/*****************************************************************************/

#ifndef _ACK_BUFFER_DEFINED
#define _ACK_BUFFER_DEFINED
typedef struct ACK_BUFFER_tag {
    SIZE_T size;  /* size of data, in bytes. */
    LPBYTE pData; /* pointer to the actual data. */
} ACK_BUFFER, *ACK_LPBUFFER;
#endif

/*****************************************************************************/

/*
 * NOTE: This function provides a user-readable version string to identify the
 *       library.
 */

ACK_API LPCSTR ACK_GetVersion(VOID);

/*****************************************************************************/

/*
 * NOTE: These are the functions that must be called to initialize and finalize
 *       the library, respectively.  The library must be initialized via the
 *       ACK_Initialize function before any other functions may be called;
 *       otherwise, undefined behavior may result.  When the application is
 *       finished using the library, it must be finalized by calling the
 *       ACK_Finalize function; otherwise, undefined behavior may result.
 */

ACK_API ACK_RESULT ACK_Initialize(VOID);

ACK_API ACK_RESULT ACK_Finalize(VOID);

/*****************************************************************************/

/*
 * NOTE: These are the functions that must be used to allocate and free any
 *       memory for use with the other functions provided by this library.
 *       The ACK_malloc function is guaranteed to return a block of memory
 *       of at least the requested size that is suitably aligned for the
 *       current operating system and platform.  Also, a valid pointer will
 *       be returned even if the size argument is zero.  The memory will be
 *       filled with zeros prior to being returned.  The ACK_msize function
 *       returns the size of the memory block containing the address supplied
 *       by the pBlock argument.  If the supplied pointer does not point to
 *       memory allocated by this library, the result is undefined.  If the
 *       size of the block cannot be determined on the current platform the
 *       result is zero.  If the pBlock argument is NULL the result is
 *       negative one.  The ACK_free function frees memory previously
 *       allocated with the ACK_malloc function.  It is legal to pass a NULL
 *       argument to this function.  In that case, no work is done.
 */

ACK_API LPVOID ACK_malloc(
    SIZE_T size /* in */
);

ACK_API LPVOID ACK_zalloc(
    SIZE_T size /* in */
);

ACK_API LPVOID ACK_realloc(
    LPVOID pBlock, /* in */
    SIZE_T size,   /* in */
    BOOL zero
);

ACK_API SIZE_T ACK_msize(
    LPVOID pBlock /* in */
);

ACK_API VOID ACK_free(
    LPVOID pBlock /* in */
);

/*****************************************************************************/

/*
 * NOTE: These are the functions responsible for managing the session and
 *       its state.  Sessions are created and destroyed using the
 *       ACK_CreateSession and ACK_CloseSession functions, respectively.
 *       Mounting a keyset is required prior to using the key management,
 *       encryption, or decryption functions provided by this library.
 *       Only one keyset per session may be mounted at a given time.  In
 *       order to successfully mount a keyset, the ACK_MountKeySet function
 *       must be called with a valid session and the fully qualified database
 *       path and file name containing the keyset to mount.  Any keysets
 *       mounted must be unmounted with the ACK_UnmountKeySet function.
 *       Certain operations require the engine to call into a user-supplied
 *       function.  These functions may be registered for the session using
 *       the ACK_AddCallback function.  The message argument indicates which
 *       message the user-supplied function is supposed to handle; however,
 *       currently, the message argument MUST have the value of zero, which
 *       means that the user-supplied function MUST handle all possible
 *       messages.  The encryption/decryption key for the session is set
 *       using the ACK_SetSessionKey function and may NOT be changed once it
 *       has been set successfully.  The ACK_SetSessionDirectory may be used
 *       to override the directory where the key files (not the layout file)
 *       are located.  This function, if used, must be called prior to calling
 *       the ACK_SetSessionKey function.  The ACK_IsSessionOk function is used
 *       to verify the internal state of the session.  The ACK_IsKeySetMounted
 *       function is used to check if a keyset has been mounted for this
 *       session via ACK_MountKeySet AND has not yet been unmounted via the
 *       ACK_UnmountKeySet function.
 */

ACK_API ACK_RESULT ACK_CreateSession(
    ACK_LPSESSION pSession, /* out */
    ACK_SESSIONTYPE type,   /* in */
    SIZE_T cacheSize        /* in */
);

ACK_API ACK_RESULT ACK_CloseSession(
    ACK_LPSESSION pSession /* in, out */
);

ACK_API ACK_RESULT ACK_IsSessionOk(
    ACK_LPSESSION pSession /* in */
);

ACK_API ACK_RESULT ACK_MountKeySet(
    ACK_SESSION session, /* in */
    LPSTR database       /* in */
);

ACK_API ACK_RESULT ACK_UnmountKeySet(
    ACK_SESSION session /* in */
);

ACK_API ACK_RESULT ACK_IsKeySetMounted(
    ACK_SESSION session /* in */
);

ACK_API ACK_RESULT ACK_SetSessionDirectory(
    ACK_SESSION session, /* in */
    LPSTR directory      /* in */
);

ACK_API ACK_RESULT ACK_SetSessionKey(
    ACK_SESSION session, /* in */
    LPSTR keyId          /* in */
);

/*****************************************************************************/

/*
 * NOTE: These are the functions responsible for managing the encryption and
 *       decryption keys.  The ACK_GetAllKeys function returns all the keys
 *       available.  Currently, the version argument must be set to 1 and the
 *       pReserved argument must be NULL.  The returned array of ACK_KEYINFO
 *       structs will be allocated by this function and the resulting pointer
 *       will be placed into the location specified by the ppKeyInfo argument.
 *       Later, this entire structure must be freed by passing the values
 *       pointed to by the pCount and ppKeyInfo arguments verbatim to the
 *       ACK_FreeAllKeys function.  The ACK_GetKeyProperty, ACK_SetKeyProperty,
 *       and ACK_UnsetKeyProperty functions are used to get, set, and unset
 *       named properties for a particular key, respectively.  The key property
 *       structure (pointed to by *ppKeyProp) returned by ACK_GetKeyProperty
 *       function must be freed by passing the value pointed to by the
 *       ppKeyProp argument verbatim to the ACK_FreeAllKeyProperties function.
 *       The ACK_GetAllKeyProperties function returns all the properties for a
 *       particular key.  Currently, the version argument must be set to 1 and
 *       the pReserved argument must be NULL.  The returned array of
 *       ACK_KEYPROP structs will be allocated by this function and the
 *       resulting pointer will be placed into the location specified by the
 *       ppKeyProp argument.  Later, this entire structure must be freed by
 *       passing the values pointed to by the pCount and ppKeyProp arguments
 *       verbatim to the ACK_FreeAllKeyProperties function.  The
 *       ACK_SizeOfKeyInfo and ACK_SizeOfKeyProp functions return the size (in
 *       bytes) of the ACK_KEYINFO and ACK_KEYPROP structures, respectively.
 */

ACK_API ACK_RESULT ACK_GetAllKeys(
    ACK_SESSION session,     /* in */
    INT64 version,           /* in */
    LPVOID *ppReserved,      /* in, out: RESERVED, must be NULL. */
    LPINT64 pCount,          /* out */
    ACK_LPKEYINFO *ppKeyInfo /* in, out */
);

ACK_API ACK_RESULT ACK_FreeAllKeys(
    ACK_SESSION session,     /* in */
    LPVOID *ppReserved,      /* in, out: RESERVED, must be NULL. */
    LPINT64 pCount,          /* in, out */
    ACK_LPKEYINFO *ppKeyInfo /* in, out */
);

ACK_API SIZE_T ACK_SizeOfKeyInfo(VOID);

ACK_API ACK_RESULT ACK_GetAllKeyProperties(
    ACK_SESSION session,     /* in */
    INT64 version,           /* in */
    LPVOID *ppReserved,      /* in, out: RESERVED, must be NULL. */
    LPSTR keyId,             /* in */
    LPINT64 pCount,          /* out */
    ACK_LPKEYPROP *ppKeyProp /* in, out */
);

ACK_API ACK_RESULT ACK_GetKeyProperty(
    ACK_SESSION session,     /* in */
    INT64 version,           /* in */
    LPVOID *ppReserved,      /* in, out: RESERVED, must be NULL. */
    LPSTR keyId,             /* in */
    LPSTR name,              /* in */
    ACK_LPKEYPROP *ppKeyProp /* in, out */
);

ACK_API ACK_RESULT ACK_SetKeyProperty(
    ACK_SESSION session, /* in */
    INT64 version,       /* in */
    LPVOID *ppReserved,  /* in, out: RESERVED, must be NULL. */
    LPSTR keyId,         /* in */
    LPSTR type,          /* in */
    LPSTR name,          /* in */
    LPSTR value          /* in */
);

ACK_API ACK_RESULT ACK_UnsetKeyProperty(
    ACK_SESSION session, /* in */
    INT64 version,       /* in */
    LPVOID *ppReserved,  /* in, out: RESERVED, must be NULL. */
    LPSTR keyId,         /* in */
    LPSTR name           /* in */
);

ACK_API ACK_RESULT ACK_FreeAllKeyProperties(
    ACK_SESSION session,     /* in */
    LPVOID pReserved,        /* in: RESERVED, must be NULL. */
    LPINT64 pCount,          /* in, out */
    ACK_LPKEYPROP *ppKeyProp /* in, out */
);

ACK_API SIZE_T ACK_SizeOfKeyProp(VOID);

/*****************************************************************************/

/*
 * NOTE: These are the functions responsible for the encryption and decryption
 *       of data.  For a stream or buffer session, the ACK_Encrypt function
 *       encrypts the data in the buffer specified by the pInput and inSize
 *       arguments and places the encrypted output into the buffer specified by
 *       the pOutput and pOutSize arguments.  For a stream or buffer session,
 *       the ACK_Decrypt function decrypts the encrypted data in the buffer
 *       specified by the pInput and inSize arguments and places the decrypted
 *       output into the buffer specified by the pOutput and pOutSize arguments.
 *       For a file session, the ACK_Encrypt and ACK_Decrypt functions operate
 *       in a similar fashion; however, the contents of the input and output
 *       buffers are interpreted as file names.
 */

ACK_API ACK_RESULT ACK_Encrypt(
    ACK_SESSION session, /* in */
    LPVOID *ppReserved,  /* in, out: RESERVED, must be NULL. */
    UINT64 flags,        /* in: RESERVED, must be zero. */
    LPUINT64 pOffset,    /* out */
    LPBYTE pInput,       /* in */
    SIZE_T inSize,       /* in */
    LPBYTE *ppOutput,    /* in, out */
    LPSIZE_T pOutSize    /* in, out */
);

ACK_API ACK_RESULT ACK_Decrypt(
    ACK_SESSION session, /* in */
    LPVOID *ppReserved,  /* in, out: RESERVED, must be NULL. */
    UINT64 flags,        /* in: RESERVED, must be zero. */
    UINT64 offset,       /* in */
    LPBYTE pInput,       /* in */
    SIZE_T inSize,       /* in */
    LPBYTE *ppOutput,    /* in, out */
    LPSIZE_T pOutSize    /* in, out */
);

/*****************************************************************************/

#endif /* _NAVAJO_H_ */

/* end of file */
