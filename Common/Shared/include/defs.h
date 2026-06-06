/*
 * defs.h --
 *
 * Copyright (c) 2008-2026 by Joe Mistachkin.  All rights reserved.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) $Id: $
 */

#ifndef _DEFS_H_
#define _DEFS_H_

/*
 * For C++ compilers, use extern "C"
 */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The following definitions set up the proper options for Windows compilers.
 * We use this method because there is no autoconf equivalent.
 */

#ifndef __WIN32__
#   if defined(_WIN32) || defined(WIN32) || defined(__MINGW32__) || \
       defined(__BORLANDC__) || (defined(__WATCOMC__) && defined(__WINDOWS_386__))
#	define __WIN32__
#	ifndef WIN32
#	    define WIN32
#	endif
#	ifndef _WIN32
#	    define _WIN32
#	endif
#   endif
#endif

/*
 * STRICT: See MSDN Article Q83456
 */

#ifdef __WIN32__
#   ifndef STRICT
#	define STRICT
#   endif
#endif /* __WIN32__ */

/*
 * Utility macros: STRINGIFY takes an argument and wraps it in "" (double
 * quotation marks), JOIN joins two arguments.
 */

#ifndef STRINGIFY
#  define STRINGIFY(x) STRINGIFY1(x)
#  define STRINGIFY1(x) #x
#endif
#ifndef JOIN
#  define JOIN(a,b) JOIN1(a,b)
#  define JOIN1(a,b) a##b
#endif

/*
 * Macros used to declare a function to be exported by a DLL. Used by Windows,
 * maps to no-op declarations on non-Windows systems. The default build on
 * windows is for a DLL, which causes the DLLIMPORT and DLLEXPORT macros to be
 * nonempty. To build a static library, the macro STATIC_BUILD should be
 * defined.
 *
 * Note: when building static but linking dynamically to MSVCRT we must still
 *       correctly decorate the C library imported function.  Use CRTIMPORT
 *       for this purpose.  _DLL is defined by the compiler when linking to
 *       MSVCRT.  
 */

#if (defined(__WIN32__) && (defined(_MSC_VER) || (__BORLANDC__ >= 0x0550) || \
     defined(__LCC__) || defined(__WATCOMC__) || (defined(__GNUC__) && defined(__declspec))))
#   define HAVE_DECLSPEC 1
#endif

#ifdef STATIC_BUILD
#   define DLLIMPORT
#   define DLLEXPORT
#   if HAVE_DECLSPEC && defined(_DLL)
#	define CRTIMPORT __declspec(dllimport)
#   else
#	define CRTIMPORT
#   endif
#else
#   if HAVE_DECLSPEC
#	define DLLIMPORT __declspec(dllimport)
#	define DLLEXPORT __declspec(dllexport)
#	define CRTIMPORT __declspec(dllimport)
#   else
#	define DLLIMPORT
#	define DLLEXPORT
#	define CRTIMPORT
#   endif
#endif

/*
 * These macros are used to control whether functions are being declared for
 * import or export. If a function is being declared while it is being built
 * to be included in a shared library, then it should have the DLLEXPORT
 * storage class. If is being declared for use by a module that is going to
 * link against the shared library, then it should have the DLLIMPORT storage
 * class. If the symbol is beind declared for a static build or for use from a
 * stub library, then the storage class should be empty.
 *
 * The convention is that a macro called BUILD_xxxx, where xxxx is the name of
 * a library we are building, is set on the compile line for sources that are
 * to be placed in the library. When this macro is set, the storage class will
 * be set to DLLEXPORT. At the end of the header file, the storage class will
 * be reset to DLLIMPORT.
 */

#undef TCL_STORAGE_CLASS
#ifdef BUILD_tcl
#   define TCL_STORAGE_CLASS DLLEXPORT
#else
#   ifdef USE_TCL_STUBS
#      define TCL_STORAGE_CLASS
#   else
#      define TCL_STORAGE_CLASS DLLIMPORT
#   endif
#endif

/*
 * Definitions that allow this header file to be used either with or without
 * ANSI C features like function prototypes.
 */

#undef _ANSI_ARGS_
#undef CONST
#ifndef INLINE
#   define INLINE
#endif

#ifndef NO_CONST
#   define CONST const
#else
#   define CONST
#endif

#ifndef NO_PROTOTYPES
#   define _ANSI_ARGS_(x)	x
#else
#   define _ANSI_ARGS_(x)	()
#endif

#ifdef USE_NON_CONST
#   ifdef USE_COMPAT_CONST
#      error define at most one of USE_NON_CONST and USE_COMPAT_CONST
#   endif
#   define CONST84
#   define CONST84_RETURN
#else
#   ifdef USE_COMPAT_CONST
#      define CONST84
#      define CONST84_RETURN CONST
#   else
#      define CONST84 CONST
#      define CONST84_RETURN CONST
#   endif
#endif

/*
 * Make sure EXTERN isn't defined elsewhere
 */

#ifdef EXTERN
#   undef EXTERN
#endif /* EXTERN */

#ifdef __cplusplus
#   define EXTERN extern "C" TCL_STORAGE_CLASS
#else
#   define EXTERN extern TCL_STORAGE_CLASS
#endif

/*
 * Used to tag functions that are only to be visible within the module being
 * built and not outside it (where this is supported by the linker).
 */

#ifndef MODULE_SCOPE
#   ifdef __cplusplus
#	define MODULE_SCOPE extern "C"
#   else
#	define MODULE_SCOPE extern
#   endif
#endif

/*
 * Macros used to cast between pointers and integers (e.g. when storing an int
 * in ClientData), on 64-bit architectures they avoid gcc warning about "cast
 * to/from pointer from/to integer of different size".
 */

#if !defined(INT2PTR) && !defined(PTR2INT)
#   if defined(HAVE_INTPTR_T) || defined(intptr_t)
#	define INT2PTR(p) ((void*)(intptr_t)(p))
#	define PTR2INT(p) ((int)(intptr_t)(p))
#   else
#	define INT2PTR(p) ((void*)(p))
#	define PTR2INT(p) ((int)(p))
#   endif
#endif
#if !defined(UINT2PTR) && !defined(PTR2UINT)
#   if defined(HAVE_UINTPTR_T) || defined(uintptr_t)
#	define UINT2PTR(p) ((void*)(uintptr_t)(p))
#	define PTR2UINT(p) ((unsigned int)(uintptr_t)(p))
#   else
#	define UINT2PTR(p) ((void*)(p))
#	define PTR2UINT(p) ((unsigned int)(p))
#   endif
#endif

/*
 * The following code is copied from winnt.h. If we don't replicate it here,
 * then <windows.h> can't be included after tcl.h, since tcl.h also defines
 * VOID. This block is skipped under Cygwin and Mingw.
 */

#if defined(__WIN32__) && !defined(HAVE_WINNT_IGNORE_VOID)
#ifndef VOID
#define VOID void
typedef char CHAR;
typedef short SHORT;
typedef long LONG;
#endif
#endif /* __WIN32__ && !HAVE_WINNT_IGNORE_VOID */

/*
 * Macro to use instead of "void" for arguments that must have type "void *"
 * in ANSI C; maps them to type "char *" in non-ANSI systems.
 */

#ifndef NO_VOID
#define VOID	void
#else
#define VOID	char
#endif

/*
 * Define an abstract data type for generic opaque handles.
 */

#ifndef NO_HANDLE
typedef void *HANDLE;
#endif

/*
 * Define some string types.
 */

#ifndef NO_CHAR
typedef char CHAR;
#endif

#ifndef NO_LPSTR
typedef CHAR *LPSTR;
#endif

#ifndef NO_LPCSTR
typedef const CHAR *LPCSTR;
#endif

#ifndef NO_WCHAR
typedef unsigned short WCHAR;
#endif

#ifndef NO_LPWSTR
typedef WCHAR *LPWSTR;
#endif

#ifndef NO_LPCWSTR
typedef const WCHAR *LPCWSTR;
#endif

/*
 * Make sure NULL is defined.
 */

#ifndef NULL
#define NULL ((void *)0)
#endif

/*
 * Macros to use instead of "0" and "1" for values that are logically boolean.
 */

#ifndef FALSE
#define FALSE	0
#endif

#ifndef TRUE
#define TRUE	1
#endif

/*
 * Miscellaneous declarations.
 */

#ifndef _CLIENTDATA
#   ifndef NO_VOID
	typedef void *ClientData;
#   else
	typedef int *ClientData;
#   endif
#   define _CLIENTDATA
#endif

#ifndef UNUSED
#define UNUSED(x) (x)
#endif

/*
 * end block for C++
 */

#ifdef __cplusplus
}
#endif

#endif /* _DEFS_H_ */

/* end of file */
