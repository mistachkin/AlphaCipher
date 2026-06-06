# Contributing to AlphaCipher

Thanks for considering a contribution.  This document covers the
mechanics of getting a change reviewed and merged.  Conduct
expectations are in [`CODE_OF_CONDUCT.md`](CODE_OF_CONDUCT.md); the
security disclosure path is in [`SECURITY.md`](SECURITY.md) and is
**not** GitHub Issues / PRs.

## Setting up to build

### Linux / WSL / OpenBSD

```sh
sudo apt-get install build-essential   # or your distro equivalent
cd Navajo/4.0/src
make
```

Build outputs land in `Navajo/4.0/src/obj/`; the source tree itself is
not written to.  A clean tree means a clean run of:

```sh
cd Navajo/4.0/src
make clean
make            # zero warnings, zero errors
LD_LIBRARY_PATH=obj obj/sdkTest   # smoke (see test/README for fixture setup)
```

The CI workflow at `.github/workflows/build.yml` runs the same
commands.

### Windows desktop (Visual Studio 2008)

Open `Navajo/4.0/src/NavajoSdk.sln`.  Batch-build the
**ReleaseDll** configuration of the **sqlite3** and **NavajoSdk**
projects for both Win32 and x64.  The VS2008 build picks up the
C99 `<stdint.h>` and `<math.h>` additions from
`Common/Shared/include/compat/` automatically — see the headers'
comments for the wiring.

Newer Visual Studio releases (2010+) consume the same project files,
but the compat headers become empty (`_MSC_VER < 1600` guard).

### Windows CE

`NavajoCE.vcproj` (engine) + `sqlite3CE.vcproj` (storage backend).
WCE picks up `compat/wce_stdlib.h` through both the SDK source (via
`navajoPort.h`) and the SQLite force-include chain.

## Proposing a change

1. **Fork and branch** from `trunk` (the integration branch — this
   project follows the Fossil naming convention).
2. **Match the existing house style** (see below).  Style-only
   churn is not accepted in functional PRs; submit it as a separate
   PR.
3. **Add or update tests** appropriate to the change.  Bug fixes
   should add a regression test where feasible.
4. **Update `CHANGELOG.md`** under `[Unreleased]` with one line per
   user-visible change.  Stick to the
   `Added` / `Changed` / `Fixed` / `Removed` / `Security` sections.
5. **Build clean** locally before opening the PR.
6. **Open the PR** using the PR template.  Describe the change, link
   any tracking issue, list the test commands you ran.

## House style

The code base predates this document; the conventions below describe
what is *already* there and what every new file is expected to follow.

### File header

Every C / C++ file begins with the canonical 11-line block:

```c
/*
 * <filename> -- <one-line description>
 *
 * Copyright (c) <years> by Joe Mistachkin.  All rights reserved.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * written by: Joe Mistachkin
 *
 * RCS: @(#) $Id: $
 */
```

New contributor copyright lines are welcome; preserve any existing
ones already present and add yours alongside, not in place.

### Function header

Return type on its own line; function name plus `/* PUBLIC */` or
`/* PRIVATE */` on the next; one parameter per line; opening brace on
its own line.  See `getChunkIdAndOffset` in `Navajo/4.0/src/navajoInt.c`
as the canonical exemplar.

### Code layout

- K&R brace style; `else` cuddles with `}`.
- 4-space indentation (no tabs in `Navajo/4.0/src/*.c`; the older
  `phone/` and `tinCan/` trees use tabs — match the file you're editing).
- Pointer-style `LPSTR p`, never `LPSTR* p`.  Space before the
  pointer name.
- Comments are `/* ... */` only.
- Every `.c` file ends with `/* end of file */`.

### Naming

Windows-style "Hungarian" naming is used throughout.  Don't try to
modernise it locally:

- `LPSTR` / `LPCSTR` / `LPBYTE` / `LPVOID` for pointer typedefs.
- `ACK_LPSESSIONINFO`, `pSessionInfo`, `pCachedKey` for local pointers.
- `kResult` for `ACK_RESULT` / `HRESULT` locals; `rc` for SQLite
  return codes.

### Errors

- All SDK-internal functions return `ACK_RESULT` (= `HRESULT`).
- Test outcomes via `SUCCEEDED(...)` / `FAILED(...)`, never by
  comparing to specific codes (the codes do not all share the same
  sign-bit semantics on legacy platforms; `SUCCEEDED` is portable).
- The `goto cleanup` + `goto done` two-label pattern is the project
  convention for resource-bearing functions.

### Asserts

`ACK_ASSERT(condition && "tag")` is the project idiom for tagging an
assertion with a context string.  Use a string literal for the tag,
not a function pointer.

### SQL

- All SQL templates live in `include/navajoSql.h`.
- Hand-wrap to keep line width below ~80 columns and align
  continuation strings to start at the same column.

### Build outputs

Never check in `.o`, `.so`, `.dll`, `.pdb`, `.lib`, `.exe`, or the VS
intermediate dirs (`Debug/`, `Release/`, `*.tlog/`, `.vs/`).  The
`.gitignore` covers the common cases; if your toolchain creates
something new, extend `.gitignore` in the same PR.

## Commit messages

One-paragraph descriptions are fine.  Imperative mood ("Fix the
chunk-plan overflow on …", not "Fixed").  Reference an issue with
`Fixes #N` when applicable.  Squashing is encouraged when the
intermediate commits aren't independently useful, but a thoughtful
multi-commit series is also welcome.

## Adding tests

The Linux build's CI runs `make` and `obj/sdkTest`.  When you add a
new code path:

- prefer extending the standalone smoke test (a self-contained C
  program that creates its own fixture databases under `/tmp`) over
  adding hidden setup dependencies;
- if your test needs a fixture file, place it under
  `Navajo/4.0/test/data/` (create it if needed) and reference it
  with a relative path from the test source.

## Pull-request review

PR reviewers will look for:

- a clean build with no new warnings on Linux GCC and (where
  applicable) VS2008;
- an entry in `CHANGELOG.md` under `[Unreleased]`;
- tests covering the changed behavior;
- a `Co-Authored-By` line if multiple humans contributed.

If a change touches the cryptographic core (anything under
`Navajo/4.0/src/navajo*.c` or `Navajo/4.0/src/protocol.c`), please
flag it explicitly in the PR description; those changes get extra
scrutiny.

Welcome aboard.  Thanks for the careful work.
