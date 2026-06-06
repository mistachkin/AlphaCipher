# AlphaCipher

AlphaCipher is a one-time pad (OTP) cryptographic engine written in
portable ANSI C.  Plaintext is XOR-encrypted against unique key
material drawn from a SQLite-backed key store; once a region of the
pad has been consumed, the engine advances the recorded offset so the
same material is never reused.  The repository ships:

- the **Navajo 4.0 SDK** — the public C API, the engine implementation,
  the SQLite schema, and the platform abstraction layer;
- a **smoke test** + the legacy `sdkTest` harness;
- two embedded application reference designs (`tinCan` audio terminal
  and `phone` Windows Mobile integration);
- the build infrastructure for GNU make on Linux / OpenBSD / MinGW and
  for Visual Studio 2008 on Windows desktop and Windows CE.

The plugin layers that consume this SDK (the AlphaCipher Eagle plugin
and the KeyPair generator UI) live in separate repositories.

## Why a one-time pad?

The one-time pad is the only widely understood cipher whose security
does not depend on a computational hardness assumption — and therefore
the only one **provably immune to advances in quantum computing**.

Modern ciphers — RSA, AES, ECDSA, Diffie-Hellman, and the
post-quantum lattice-based candidates — are all *computationally*
secure.  Their security rests on the claim that some particular
problem (integer factoring, discrete logarithms, learning-with-errors,
and so on) is infeasible to solve given the resources an attacker is
expected to have.  Quantum computing changes those resources:

- **Shor's algorithm** factors integers and solves discrete
  logarithms in polynomial time on a sufficiently large quantum
  computer, collapsing every currently-deployed RSA, DSA, and
  elliptic-curve scheme.
- **Grover's algorithm** gives a quadratic speed-up on unstructured
  search, effectively halving the bit-strength of symmetric ciphers
  (AES-256 retains roughly 128-bit security; AES-128 drops to roughly
  64).
- The **post-quantum candidates** resist those two specific
  algorithms, but they still rest on hardness assumptions that future
  cryptanalysis — quantum or classical — could weaken.

The one-time pad rests on a *theorem*, not an assumption.  Shannon
proved in 1949 that XOR'ing a plaintext against a truly-random pad
of equal length, used exactly once, produces a ciphertext that is
**statistically independent** of the plaintext.  Every possible
plaintext of length *N* is equally consistent with a given ciphertext
of length *N*.  There is no plaintext-bearing structure for an
adversary to recover — quantum, classical, or otherwise.  A quantum
computer of any size, running any algorithm, extracts the same amount
of information from a one-time-pad ciphertext as a classical
brute-force search does: **none**.  This is not "quantum-resistant for
the foreseeable future" or "secure assuming X remains hard"; it is a
proof.

The strength is conditional on four operational requirements:

1. The pad bytes must be **truly random** (not pseudorandom).
2. The pad must be **at least as long** as the message.
3. The pad must be **used exactly once**.
4. The pad must be **kept secret** and **distributed in advance** to
   both communicating parties.

Meeting all four is what makes one-time pads operationally hard
rather than impossible.  This SDK addresses requirements 2, 3, and a
significant part of 4: it manages pad material on disk, tracks the
consumed offset across encryptions so the same region of pad cannot
be reused even across crashes or process restarts, and intentionally
offers no PRNG-based fallback that might trade information-theoretic
security for convenience.  The first requirement — producing
genuinely random bytes — is the responsibility of the generator
stage, which lives in a separate plugin.

## Status

Version **4.0** is a release candidate.  The May–June 2026 work added
real multi-chunk encrypt / decrypt support, fixed a latent
`HRESULT`-on-LP64 bug that made `SUCCEEDED()` / `FAILED()` unreliable
on 64-bit Linux, and resolved a class of latent C bugs surfaced by a
full-tree audit.  See [CHANGELOG.md](CHANGELOG.md) for the full list.

## Platform support

The actively supported and tested platforms are:

- **Linux** (GCC) — primary development target; covered by CI on every
  push and pull request.
- **OpenBSD** (GCC) — best-effort, build supported through the Makefile.
- **Windows desktop** (Visual Studio 2008; also buildable with later
  Visual Studio versions through the same project files) — supported.

The following are **provided primarily for historical interest and
reference**.  They are not actively maintained, are not part of the
project's regular CI matrix, and are not covered by release
validation:

- **Windows CE / Windows Mobile** — including the engine
  (`NavajoCE.vcproj`), the SQLite storage backend
  (`sqlite3CE.vcproj`), and the `phone` and `tinCan` embedded
  reference applications under `Navajo/4.0/`.

The recent compatibility work (the `compat/wce_stdlib.h` shim and the
matching project wiring) is intentionally kept buildable so that the
code remains a useful reference.  It does **not** imply ongoing
support.  Regressions on these targets will not block a release.

High-quality contributions to the WCE / embedded paths are welcome
and will be reviewed on the same standards as desktop changes — the
maintainers simply do not commit to authoring fixes for them on any
particular timeline.  See [`CONTRIBUTING.md`](CONTRIBUTING.md) for
the review bar.

## License

Use of the software is governed by [`license.terms`](license.terms),
a Tcl-style permissive license with two project-specific additions:

- a **commercial-use transparency** clause reserving the right to
  publicly identify parties that derive substantial commercial benefit
  without entering a licensing arrangement;
- a **U.S. Government Restricted Rights** clause.

Read [`license.terms`](license.terms) for the canonical text.  Because
the additions are non-standard, this software is **not** OSI-approved
"open source" in the strictest sense — but redistribution, modification,
and use are unconditionally permitted under the terms stated there.

## Repository layout

```
.
├── Common/
│   ├── Externals/
│   │   └── sqlite3/              SQLite amalgamation + VS / WCE projects
│   └── Shared/
│       └── include/
│           ├── compat/           VS2008 / WCE compatibility shims
│           │   ├── stdint.h      C99 typedefs (VS2008 lacks one)
│           │   ├── c99math.h     INFINITY, NAN, etc. (VS2008 lacks them)
│           │   └── wce_stdlib.h  abort(), EINVAL, etc. (WCE-only)
│           ├── defs.h            Cross-platform compiler / Win32 macros
│           └── hresult.h         HRESULT, SUCCEEDED/FAILED, standard E_* codes
├── Navajo/4.0/
│   ├── include/                  Public + private API headers
│   ├── src/                      Engine implementation + Makefile + VS projects
│   │   └── os/                   Platform abstraction (ANSI/GCC/MSVC/POSIX/WIN32/WINCE)
│   ├── db/                       Reference SQLite schemas
│   ├── doc/                      SDK reference + audit reports
│   ├── test/                     sdkTest, integration tests
│   ├── images/                   Branding / icon art assets
│   ├── phone/                    Windows Mobile reference app
│   ├── tinCan/                   Windows CE audio terminal reference app
│   └── keys/                     Sample key material for tests
└── Tools/                        Release / signing / version-stamp scripts
```

## Build

### Linux / OpenBSD

```sh
cd Navajo/4.0/src
make           # builds libackernel4sdk.so and sdkTest in ./obj
make test      # runs sdkTest (requires fixture setup -- see test/README)
make clean
```

Requires GCC, GNU make, and pthreads.  All build output is placed in
`Navajo/4.0/src/obj/`; the source tree is never written to.

### Windows desktop (Visual Studio 2008)

1. Update the SQLite amalgamation from upstream (drop the
   latest `sqlite3.c` / `sqlite3.h` into
   `Common/Externals/sqlite3/` and adjust the project
   `SQLITE_DEFINES` if upstream has added new compile-time
   feature flags).
2. Open `Navajo/4.0/src/NavajoSdk.sln` in VS2008.
3. Batch-build the **ReleaseDll** configuration of the **sqlite3** and
   **NavajoSdk** projects for both Win32 and x64 platforms.

The VS2008 build automatically picks up the C99 `<stdint.h>` and
`<math.h>` additions from `Common/Shared/include/compat/`; the matching
header for newer Visual Studio versions is empty (a `_MSC_VER < 1600`
guard).

### Windows CE (historical / reference; unsupported)

> Provided primarily for historical interest and reference.  Not part
> of the project's CI matrix; no support timeline.  See *Platform
> support* above for the full statement.

Use `NavajoCE.vcproj` (engine) and `sqlite3CE.vcproj` (storage backend).
The CE build picks up the `wce_stdlib.h` shim automatically through
both the SDK source (via `navajoPort.h`) and the SQLite force-include
chain.

## Verifying the build

A clean build produces `obj/libackernel4sdk.so` (the engine shared
library) and `obj/sdkTest` (the test harness) with **zero warnings**
under `-Wall`.  Continuous integration enforces both conditions on
every push and pull request.

End-to-end verification is one command:

```sh
cd Navajo/4.0/src
make test
```

The `test` target stages a fresh copy of the keyset fixture under
`obj/test_keys/`, points `sdkTest` at it via the `ACK_TEST_DATABASE`
environment variable, and runs the full encrypt / decrypt
round-trip — 1000 iterations across multiple plaintext sizes.  A
passing run exits 0 with no comparison-failure lines; CI runs the
same target on every push and pull request.

To exercise `sdkTest` with a different keyset, set `ACK_TEST_DATABASE`
to the absolute path of an alternative `layout.pef` and run the
binary directly:

```sh
ACK_TEST_DATABASE=/path/to/your/layout.pef obj/sdkTest
```

## Documentation

| Document                                                      | Audience                                                 |
|---------------------------------------------------------------|----------------------------------------------------------|
| [`Navajo/4.0/doc/public_api.md`](Navajo/4.0/doc/public_api.md) | Engine integrators consuming `ACK_*` from C / C++         |
| [`Navajo/4.0/doc/latent_bug_audit.md`](Navajo/4.0/doc/latent_bug_audit.md) | Maintainers; record of the May–June 2026 audit + fixes    |
| [`CHANGELOG.md`](CHANGELOG.md)                                 | All consumers; version history                            |
| [`CONTRIBUTING.md`](CONTRIBUTING.md)                           | Anyone proposing source changes                           |
| [`SECURITY.md`](SECURITY.md)                                   | Anyone reporting a vulnerability                          |
| [`CODE_OF_CONDUCT.md`](CODE_OF_CONDUCT.md)                     | Community participants                                    |
| [`Navajo/4.0/src/README`](Navajo/4.0/src/README)               | Historical operator notes for the official binary SDK    |

## Reporting issues

- **Bug reports / feature requests:** open an issue using one of the
  templates under the *Issues* tab.
- **Security vulnerabilities:** do **not** open a public issue; use
  GitHub's *Report a vulnerability* button under the repository's
  *Security* tab.  Full procedure in [`SECURITY.md`](SECURITY.md).
