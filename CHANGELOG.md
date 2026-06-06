# Changelog

All notable changes to AlphaCipher are recorded in this file.

The format is loosely based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/);
versioning is per the `PRODUCT_VERSION` macro in `Navajo/4.0/src/buildnum.h`.

## [Unreleased]

(Add changes here as they land.)

## [4.0.0] — 2026-06-06

This entry covers the multi-chunk modernization, the latent-bug audit
work, the build-system cleanup, and the VS2008 / WCE compatibility
work that together prepare the project for its first public GitHub
release.

### Added

- **Multi-chunk encrypt / decrypt.** `getKeyBytes` now builds a
  per-request chunk plan, ATTACHes only the chunk databases needed for
  the request (outside any active SQLite transaction), reads each
  chunk in turn, advances the recorded `(ChunkId, Offset)` atomically,
  and DETACHes on the way out.  The new private helpers
  `planChunkRange`, `attachChunkPlan`, `detachChunkPlan`,
  `buildChunkFilePath`, `updateChunkOffset`, and `freeChunkPlan` live
  in `navajoInt.c`; the public `ACK_Encrypt` / `ACK_Decrypt` ABI is
  unchanged (`pOffset` is still a global key offset).
- **Per-call chunk plan accounting** structures
  (`ACK_CHUNKPLAN_ENTRY`, `ACK_CHUNKPLAN`) in
  `include/navajoIntTypes.h`.
- **New SQL statement** `ACK_UPDATE_CHUNKOFFSET_SQL` for the
  set-not-increment offset update required by spanning reads;
  `ACK_RESET_OFFSET_SQL` extended to also restore `ChunkId`.
- **`Navajo/4.0/doc/public_api.md`** — formal reference for the public
  C API surface.
- **`Navajo/4.0/doc/latent_bug_audit.md`** — record of the audit pass
  + fix plan.
- **VS2008 / WCE compatibility shims** under
  `Common/Shared/include/compat/`:
  - `stdint.h` — C99 fixed-width integer types and limits.
  - `c99math.h` — `INFINITY`, `NAN`, `HUGE_VALF`, `HUGE_VALL`, and the
    `isnan` / `isinf` / `isfinite` macros, force-included into the
    SQLite compile for VS2008.
  - `wce_stdlib.h` — `abort()`, `EINVAL`, `errno_t`, `intptr_t`
    substitutes for the WCE C runtime, consolidated from the inline
    block previously in `navajoPort.h` and force-included into the
    SQLite WCE compile.
- **Repository docs**: this `CHANGELOG.md`, plus `README.md`,
  `CONTRIBUTING.md`, `CODE_OF_CONDUCT.md`, `SECURITY.md`, `.gitignore`,
  `.gitattributes`, issue / PR templates, and a Linux GitHub Actions
  CI workflow.

### Changed

- **`HRESULT` and `ACK_RESULT`** are now `typedef int` on non-Windows
  targets (still `long` on Windows / WCE).  On 64-bit Linux the
  previous `typedef long` placed the sign bit outside the 32-bit
  HRESULT layout, so `SUCCEEDED(hr)` returned true for codes that
  carry the failure flag and `FAILED(hr)` returned false.  Every code
  path that branched on the result of an SDK call was silently
  miscategorising errors on LP64.
- **`ACK_Decrypt`** no longer rejects requests whose global offset
  exceeds `INT_MAX`.  Multi-chunk keys can legitimately have offsets
  past that boundary; per-chunk overflow is still caught inside
  `getKeyBytes`.
- **`ACK_SetSessionKey`** no longer eagerly ATTACHes the key's chunk
  database.  ATTACH / DETACH now happens per-call inside
  `getKeyBytes`, which is the only safe place to do it outside an
  active transaction.
- **Makefile** restructured so every build artifact lands in
  `Navajo/4.0/src/obj/` (object files, shared library, executable);
  the source tree is no longer written to.  Test-link command line
  re-ordered (object file before `-l` flags) to work with modern
  GNU `ld --as-needed`; `$ORIGIN`-based rpath embedded so `obj/sdkTest`
  runs without `LD_LIBRARY_PATH`.
- **`compat/stdint.h`** and **`compat/c99math.h`** moved into a
  dedicated `compat/` sub-directory (initially named `vs2008/`,
  renamed to reflect generality).
- **Assert idiom** standardised: every `ACK_ASSERT(condition && tag)`
  call site now passes `tag` as a string literal (`"funcName"`)
  instead of a function pointer.  The previous form triggered
  `-Waddress` on modern GCC and emitted opaque diagnostics on failure.

### Fixed

- **`rollbackTransaction`** in `navajoSql.c` was issuing `COMMIT`
  instead of `ROLLBACK`.  Any error path that relied on rollback
  semantics was instead committing partial state.
- **`navajoGen.c` assert typo** at the `fclose` site (`(rc == 0) &&
  fclose`).  The function-pointer side-clause was always truthy, so
  the assertion never noticed an `fclose` failure even under
  `_DEBUG`.
- **Test correctness** (`test/test.c`, `test/sdkTest.c`).
  Round-trip comparison mismatches in the encrypt / decrypt loop
  printed a diagnostic and then returned `exit 0`.  A broken cipher
  passed the test suite.  The HRESULT-on-LP64 bug above hid behind
  this for the entire pre-modernization period.
- **WAVEHDR cleanup** in `tinCan/TinCan.cpp` (lines 414 and 573).
  `waveInUnprepareHeader` / `waveOutUnprepareHeader` were called with
  `(LPWAVEHDR)pBuffer` instead of
  `(LPWAVEHDR)(pBuffer + BUFFER_HEADER_OFFSET)`, handing the wave
  subsystem a pointer that did not point at the registered WAVEHDR.
- **Flexible-array OOM check** in `phone/queue.c`.
  `RTN_IF_BADNEW(pElement->pBuffer)` tested the address of a flexible
  array member (always non-NULL) instead of the stored value (the
  `ACK_malloc` result), so heap exhaustion silently produced
  zero-filled queue entries.  Also patches a leak of the surrounding
  `ELEMENT` on the now-detectable failure path.
- **Dialer DoS** in `phone/transport.c`.  Stray non-Connect /
  non-Accept packets each incremented `iTriesLeft`, exactly cancelling
  the `while` loop's decrement.  An attacker spamming UDP could
  indefinitely extend a `DialRemote` or `AnswerRemote` call.  Now
  bounded by a separate `iStrayLeft` budget.
- **`LB_GETTEXT` heap overflow** in `phone/phone.cpp`
  (`GetSelectedListText`).  `LB_GETTEXTLEN` returns the character
  count without the NUL; the allocation lacked the `+1`.
- **`AnswerWifiHandshakeProc` stack overflow** in `phone/phone.cpp`.
  Unbounded `_tcscpy` / `_tcscat` of network-controlled inputs into
  `TCHAR szBuffer[MAX_PATH]`; now uses bounded `_sntprintf`.
- **`ACKP_ScanPathForKeys` buffer overflow** in `src/protocol.c`.
  `char szPath[NAME_MAX]` (only large enough for a single path
  component) was assembled with unbounded `ACK_strcat` calls; replaced
  with `PATH_MAX`-sized buffer and explicit overflow checks at each
  concatenation step.  `pathAddSeparator` now takes a buffer-size
  argument and refuses to overrun.
- **`GuidToHex` unbounded `sprintf`** in `src/protocol.c` switched to
  `snprintf(MAX_GUID_SPACE)`.
- **INT64 truncation** at the `ACKP_GetAllKeys` allocation site
  (`iCount = iCount + (int) p->i64KeyCount;`).  Sums above `INT_MAX`
  silently truncated and undersized the `ACK_malloc`.  Two loop
  counters typed as `int` against `i64KeyCount` (`INT64`) were also
  retyped.
- **`makeQualifiedFileName` `strcpy`** in `src/navajoUtil.c` replaced
  with a bounded `memcpy` of the already-computed length, eliminating
  the only remaining unchecked `strcpy` in the engine.
- **`os_posix.c` `posixIsdir`** snprintf-into-`NAME_MAX` truncation
  warning resolved by sizing the buffer to `PATH_MAX + 1` (with a
  fallback) and passing `sizeof(path)` to `snprintf`.
- **`INT64_FORMAT` / `UINT64_FORMAT`** in `navajoPort.h` now resolve
  to `"%" PRId64` / `"%" PRIu64` from `<inttypes.h>` on C99-capable
  non-MSVC compilers, matching the underlying type whether it's
  `long` (LP64) or `long long` (LLP64 / pre-LP64).  Pre-C99 keeps the
  `%lld` fallback.
- **Volatile cross-thread state** in `phone/transport.c`
  (`bXmitThreadRunning`, `bRcveThreadRunning`, `XmitEvent`) inhibits
  the compiler from caching reads across loop iterations.
- **WCE assertion-only abort** path in `tinCan/TinCan.cpp` clarified.
- **`db/makeDemo.eagle`** now validates that the placeholder
  `trnFile` and `OtpKeyDir` paths have been replaced and exist before
  attempting to import key material.

### Removed

- `attachKeyInfoDatabase` (formerly in `navajoInt.c`).  Replaced by
  per-call attach inside `getKeyBytes`.
- Inline `_WIN32_WCE` shim block in `navajoPort.h` (now obtained via
  `#include "compat/wce_stdlib.h"`).
- The `#if 0`-guarded dead `ACK_GETNEXTCHUNKID_SQL` definition in
  `include/navajoSql.h`.

### Security

- The `rollbackTransaction` and `HRESULT`-on-LP64 fixes correct
  cryptographic correctness bugs that would have allowed silent
  partial commits and silent error-swallowing on 64-bit Linux.
  Treat any production deployment built before the modernization as
  affected.

[Unreleased]: ./
[4.0.0]: ./
