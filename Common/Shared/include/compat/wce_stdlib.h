/*
 * wce_stdlib.h -- C standard-library shims for Windows CE.
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
 * Windows CE's C runtime is a subset of the Windows desktop CRT.  Several
 * names that the ANSI / C89 standard library guarantees to be present
 * are absent from WCE's headers, including (but not limited to):
 *
 *   abort()    -- normally in <stdlib.h>
 *   EINVAL     -- normally in <errno.h>
 *   errno_t    -- normally in <errno.h> (Microsoft-extended C89)
 *   intptr_t   -- normally in <stddef.h> (Microsoft-extended C89)
 *
 * This header provides minimal substitutes for those names so that the
 * AlphaCipher SDK and the SQLite amalgamation will compile on WCE.
 *
 * Despite the c99math.h sibling's prefix, the names addressed here are
 * NOT a C99 versioning issue; they are a Windows CE platform-shortfall
 * issue.  Hence the wce_ prefix rather than c99_.
 *
 * Usage
 * -----
 *
 * Two consumption patterns are supported, both already wired up in
 * this tree:
 *
 *   1) Force-include from the compiler command line for the WCE
 *      configurations of sqlite3CE.vcproj:
 *
 *          ForcedIncludeFiles="$(SQLITE_WCE_STDLIB_FORCE_INCLUDE)"
 *
 *      (the macro is defined in Common/Externals/sqlite3/sqlite3.vsprops)
 *
 *   2) Quoted #include from navajoPort.h, which used to carry an
 *      inline WCE-only block providing the same definitions.  That
 *      block has been replaced by an #include of this file so there is
 *      a single source of truth.
 *
 * The body is wrapped in a _WIN32_WCE guard so the file is inert on
 * every non-WCE target (including desktop Visual C++ 2008, where the
 * CRT provides all of these names).
 *
 * --------------------------------------------------------------------------
 */

#ifndef _COMPAT_WCE_STDLIB_H_
#define _COMPAT_WCE_STDLIB_H_

#if defined(_WIN32_WCE)

/*
 * NOTE: The shims below reference HANDLE and ExitProcess, both of
 *       which come from <windows.h>.  Pull it in here so this header
 *       is self-sufficient when force-included before any other
 *       header has had a chance to bring it in.
 */

#include "windows.h"

/*
 * NOTE: <errno.h> on WCE does not define EINVAL or errno_t.  The
 *       values and typedef below match the Microsoft CRT on the
 *       desktop, so code that branches on errno values remains
 *       portable across the two targets.
 */

#ifndef EINVAL
#define EINVAL (22)
#endif

#ifndef _ERRCODE_DEFINED
#define _ERRCODE_DEFINED
typedef int errno_t;
#endif

/*
 * NOTE: <stddef.h> on WCE does not define intptr_t.  The HANDLE-typed
 *       definition below preserves the historical behavior of the
 *       AlphaCipher SDK on WCE; do not change it to a signed-integer
 *       form without auditing every consumer first.
 */

#ifndef _INTPTR_T_DEFINED
#define _INTPTR_T_DEFINED
typedef HANDLE intptr_t;
#endif

/*
 * NOTE: <stdlib.h> on WCE does not declare abort().  Map it to a
 *       fatal process exit with a distinct exit code (3) so a crash
 *       in this path can be told apart from the normal exit paths.
 */

#ifndef abort
#define abort() ExitProcess(3)
#endif

#endif /* defined(_WIN32_WCE) */

#endif /* _COMPAT_WCE_STDLIB_H_ */

/* end of file */
