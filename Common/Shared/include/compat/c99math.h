/*
 * c99math.h -- Minimal C99 <math.h> additions for Microsoft Visual C++ 2008.
 *
 * Copyright (c) 2026 by Joe Mistachkin.  All rights reserved.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * written by: Joe Mistachkin
 *
 * RCS: @(#) $Id: $
 *
 * --------------------------------------------------------------------------
 *
 * Background
 * ----------
 *
 * The Visual C++ 2008 toolchain (cl.exe v9.00, _MSC_VER == 1500) ships a
 * usable <math.h> from its CRT, but that header predates the C99
 * additions to math.h: INFINITY, NAN, HUGE_VALF, HUGE_VALL, and the
 * isnan / isinf / isfinite macros are all absent.  Visual C++ 2010 and
 * later (_MSC_VER >= 1600) ship a C99-aware <math.h> in the platform
 * SDK, so this header is needed only for the VS2008 configuration; the
 * body is wrapped in an _MSC_VER < 1600 guard so it is empty for newer
 * toolchains and for non-MSVC compilers.
 *
 * Modern SQLite amalgamations include <math.h> and reference INFINITY;
 * without these additions, sqlite3.c fails to compile on VS2008.
 *
 * Usage
 * -----
 *
 * Wire this header in as a force-include in the VS2008 NavajoSdk
 * project (ACK_C99MATH_FORCE_INCLUDE in navajoSdk.vsprops, consumed by
 * ForcedIncludeFiles on the VCCLCompilerTool in NavajoSdk.vcproj):
 *
 *     #include <math.h>                  the source's first reference;
 *                                        resolves to the CRT header
 *                                        because the file you are
 *                                        reading is named c99math.h,
 *                                        not math.h, and so does not
 *                                        shadow <math.h>
 *
 *     <C99 additions from this file>     pre-defined by /FI; the
 *                                        #if defined(_MSC_VER) &&
 *                                        (_MSC_VER < 1600) guard keeps
 *                                        the body inert on VS2010+ and
 *                                        on non-MSVC compilers
 *
 * Explicit use from your own source is also fine:
 *
 *     #include <math.h>                  CRT
 *     #include "compat/c99math.h"        C99 additions
 *
 * Note that this header includes <float.h> to ensure _isnan() and
 * _finite() are declared even when reached before <math.h> proper.
 *
 * --------------------------------------------------------------------------
 */

#ifndef _COMPAT_C99MATH_H_
#define _COMPAT_C99MATH_H_

#if defined(_MSC_VER) && (_MSC_VER < 1600)

/*
 * NOTE: <float.h> provides the prototypes for _isnan() and _finite()
 *       even when this header is included before <math.h> proper.
 */

#include <float.h>

/* ---------------------------------------------------------------- */
/* C99 7.12  Mathematics <math.h> -- constants                      */
/* ---------------------------------------------------------------- */

/*
 * NOTE: The constant expression below intentionally overflows at
 *       translation time to produce +Inf in a float (C99 7.12 #4).
 *       Visual C++ may emit warning C4756 ("overflow in constant
 *       arithmetic") at each call site; suppress it project-wide
 *       (see ACK_DISABLE_WARNINGS in navajoSdk.vsprops) or wrap
 *       individual uses with __pragma(warning(suppress:4756)).
 */

#ifndef INFINITY
#define INFINITY  ((float)(1e+300 * 1e+300))
#endif

/*
 * NOTE: Multiplying +Inf by 0 yields NaN per IEEE 754.  This is the
 *       same form used by Visual C++ 2013 and later.
 */

#ifndef NAN
#define NAN       ((float)(INFINITY * 0.0F))
#endif

#ifndef HUGE_VALF
#define HUGE_VALF INFINITY
#endif

#ifndef HUGE_VALL
/*
 * NOTE: On the Microsoft toolchain 'long double' has the same width
 *       and representation as 'double', so casting INFINITY (float)
 *       widens to long double without losing information.
 */

#define HUGE_VALL ((long double)INFINITY)
#endif

/* ---------------------------------------------------------------- */
/* C99 7.12.3  Classification macros                                */
/* ---------------------------------------------------------------- */

/*
 * NOTE: The VS2008 CRT provides _isnan(double) and _finite(double) but
 *       not the C99-spelled macros.  Forward to the CRT names; the
 *       outer parentheses and the (double) cast match the C99 contract
 *       that these be macros that accept any floating type.
 */

#ifndef isnan
#define isnan(x)    (_isnan((double)(x)))
#endif

#ifndef isinf
#define isinf(x)    ((!_finite((double)(x))) && (!_isnan((double)(x))))
#endif

#ifndef isfinite
#define isfinite(x) (!!_finite((double)(x)))
#endif

#endif /* defined(_MSC_VER) && (_MSC_VER < 1600) */

#endif /* _COMPAT_C99MATH_H_ */

/* end of file */
