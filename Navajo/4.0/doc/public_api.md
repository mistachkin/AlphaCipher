# AlphaCipher Navajo 4.0 — Public C API Reference

This document describes the public C API surface exported by the
AlphaCipher Kernel ("Navajo") version 4.0 SDK.  Every `ACK_*` symbol
declared in `Navajo/4.0/include/navajo.h` and
`Navajo/4.0/include/navajoGen.h` is covered.

The canonical regression harness for this engine is the
**AlphaCipher Eagle plugin**, which is published in a separate
repository.  C code that links against `libackernel4sdk.so` (or
`ACKernel4Sdk.dll` on Windows) sees exactly the surface described
here.

---

## Conventions

* Every function returns an `ACK_RESULT`.  Use `SUCCEEDED(r)` and
  `FAILED(r)` (from `hresult.h`) to test the outcome; do not compare
  against `ACK_S_OK` directly except where noted.
* Output parameters are written **only on success** unless documented
  otherwise.  All-or-nothing semantics are the rule: on failure the
  caller's buffers/handles are left untouched.
* `LPSTR` / `LPCSTR` / `LPBYTE` / `LPVOID` / `LPUINT64` etc. are the
  Windows-style pointer typedefs declared in `navajo.h`.
* The library uses its own allocator (`ACK_malloc` / `ACK_zalloc` /
  `ACK_free`).  Buffers allocated by the library must be released with
  `ACK_free`; do not call libc `free`.
* All public functions are thread-safe to call concurrently from
  different sessions.  A single `ACK_SESSION` must not be used from
  multiple threads simultaneously.

---

## Section 1 — Lifecycle

The library must be initialised exactly once per process before any
other function is called, and finalised once when the process is
finished using the engine.

### `ACK_Initialize`

```c
ACK_RESULT ACK_Initialize(VOID);
```

Initialises the engine's OS-abstraction layer and global state.  Must
be called prior to any other `ACK_*` function.

* **Returns:** `ACK_S_OK` on success; an `E_*` or `ACK_E_*` failure
  code on error.

### `ACK_Finalize`

```c
ACK_RESULT ACK_Finalize(VOID);
```

Releases the resources held by the engine's OS layer.  Pairs with
`ACK_Initialize`.

* **Returns:** `ACK_S_OK` on success.

### `ACK_GetVersion`

```c
LPCSTR ACK_GetVersion(VOID);
```

Returns a pointer to a NUL-terminated string identifying the engine
build (product name, version, configuration, platform).  The returned
pointer references library-owned static storage; do not free it.

---

## Section 2 — Session lifecycle

A *session* groups together the mounted keyset, the active encryption
or decryption key, and (optionally) an in-memory key cache.

### `ACK_CreateSession`

```c
ACK_RESULT ACK_CreateSession(
    ACK_LPSESSION pSession,  /* out */
    ACK_SESSIONTYPE type,    /* in */
    SIZE_T cacheSize         /* in */
);
```

Allocates a new session.  `*pSession` receives an opaque handle that
must later be released via `ACK_CloseSession`.

* `type` is a bitmask of `ACKST_*` values declaring which operations
  this session may perform (e.g. `ACKST_Stream | ACKST_Encrypt`).
* `cacheSize` is the per-key cache size (bytes); pass `0` to disable
  the in-memory cache.  When non-zero the engine will pre-fetch up to
  this many bytes from the active key on every cache miss.
* **Preconditions:** `ACK_Initialize` has been called.
* **Returns:** `ACK_S_OK` or `E_POINTER`, `E_MUSTBENULL`,
  `E_OUTOFMEMORY`.

### `ACK_CloseSession`

```c
ACK_RESULT ACK_CloseSession(
    ACK_LPSESSION pSession   /* in, out */
);
```

Releases the session.  If a keyset is still mounted it is unmounted
first.  `*pSession` is set to `NULL` on success.

### `ACK_IsSessionOk`

```c
ACK_RESULT ACK_IsSessionOk(
    ACK_LPSESSION pSession   /* in */
);
```

Verifies internal session invariants.  Returns `ACK_S_OK` if the
session is consistent, `ACK_E_SESSION_CHECK_FAILED` otherwise.

### `ACK_MountKeySet`

```c
ACK_RESULT ACK_MountKeySet(
    ACK_SESSION session,  /* in */
    LPSTR database        /* in */
);
```

Opens the SQLite *master* keyset database referenced by `database`
(the "layout" file).  Only one keyset may be mounted per session.

* **Preconditions:** session is valid; no keyset already mounted.
* **Returns:** `ACK_S_OK`, `ACK_E_MOUNTED_DATABASE`, or a SQLite
  failure converted via `HRESULT_FROM_SQLITE`.

### `ACK_UnmountKeySet`

```c
ACK_RESULT ACK_UnmountKeySet(
    ACK_SESSION session   /* in */
);
```

Releases the in-memory key cache (if any), closes the SQLite handle,
and clears the active key state.

### `ACK_IsKeySetMounted`

```c
ACK_RESULT ACK_IsKeySetMounted(
    ACK_SESSION session   /* in */
);
```

Returns `S_OK` if a keyset is currently mounted, `S_FALSE` otherwise.

### `ACK_SetSessionDirectory`

```c
ACK_RESULT ACK_SetSessionDirectory(
    ACK_SESSION session,  /* in */
    LPSTR directory       /* in */
);
```

Overrides the directory where per-key chunk database files are
located.  When this override is unset (the default), chunk files are
resolved relative to the mounted keyset path.  Pass `NULL` to clear a
previously set override.

* **Preconditions:** must be called *before* `ACK_SetSessionKey`; the
  engine returns `ACK_E_KEY_FOR_SESSION` if a key is already active.

### `ACK_SetSessionKey`

```c
ACK_RESULT ACK_SetSessionKey(
    ACK_SESSION session,  /* in */
    LPSTR keyId           /* in */
);
```

Selects the active encryption/decryption key by its keyset Id (a GUID
string).  Pass `NULL` to clear the previously selected key.

The engine does **not** eagerly ATTACH the key's chunk database(s);
chunk attach/detach happens per request inside `ACK_Encrypt` and
`ACK_Decrypt`.

* **Returns:** `ACK_S_OK`, `ACK_E_NO_MOUNTED_DATABASE`,
  `ACK_E_NO_FILE_NAME_FOR_KEY`, etc.

---

## Section 3 — Key & key-property enumeration

### `ACK_GetAllKeys`

```c
ACK_RESULT ACK_GetAllKeys(
    ACK_SESSION session,
    INT64 version,
    LPVOID *ppReserved,      /* must be NULL */
    LPINT64 pCount,          /* out */
    ACK_LPKEYINFO *ppKeyInfo /* in, out: must point to NULL */
);
```

Fetches metadata for every key in the mounted keyset.

* `version` must be `ACK_KEYINFO_VERSION`.
* On success `*pCount` receives the number of entries and
  `*ppKeyInfo` receives a library-allocated array of `ACK_KEYINFO`.
* The caller releases the array by passing the same `*pCount` and
  `*ppKeyInfo` to `ACK_FreeAllKeys`.

### `ACK_FreeAllKeys`

```c
ACK_RESULT ACK_FreeAllKeys(
    ACK_SESSION session,
    LPVOID *ppReserved,      /* must be NULL */
    LPINT64 pCount,          /* in, out */
    ACK_LPKEYINFO *ppKeyInfo /* in, out */
);
```

Releases the array allocated by `ACK_GetAllKeys`.  On success
`*pCount = 0` and `*ppKeyInfo = NULL`.

### `ACK_SizeOfKeyInfo`

```c
SIZE_T ACK_SizeOfKeyInfo(VOID);
```

Returns `sizeof(ACK_KEYINFO)`.  Useful when the consumer must
allocate arrays of `ACK_KEYINFO` without including the SDK headers.

### `ACK_GetAllKeyProperties`

```c
ACK_RESULT ACK_GetAllKeyProperties(
    ACK_SESSION session,
    INT64 version,
    LPVOID *ppReserved,      /* must be NULL */
    LPSTR keyId,
    LPINT64 pCount,          /* out */
    ACK_LPKEYPROP *ppKeyProp /* in, out: must point to NULL */
);
```

Returns every property attached to the named key.  `version` must be
`ACK_KEYPROP_VERSION`.

### `ACK_GetKeyProperty`

```c
ACK_RESULT ACK_GetKeyProperty(
    ACK_SESSION session,
    INT64 version,
    LPVOID *ppReserved,      /* must be NULL */
    LPSTR keyId,
    LPSTR name,
    ACK_LPKEYPROP *ppKeyProp /* in, out: must point to NULL */
);
```

Returns a single named property of the named key.

### `ACK_SetKeyProperty`

```c
ACK_RESULT ACK_SetKeyProperty(
    ACK_SESSION session,
    INT64 version,
    LPVOID *ppReserved,      /* must be NULL */
    LPSTR keyId,
    LPSTR type,
    LPSTR name,
    LPSTR value
);
```

Inserts or replaces a named property on the named key.  Requires
session type `ACKST_SetKeyProperty`.

### `ACK_UnsetKeyProperty`

```c
ACK_RESULT ACK_UnsetKeyProperty(
    ACK_SESSION session,
    INT64 version,
    LPVOID *ppReserved,      /* must be NULL */
    LPSTR keyId,
    LPSTR name
);
```

Removes a named property from the named key.  Requires session type
`ACKST_UnsetKeyProperty`.

### `ACK_FreeAllKeyProperties`

```c
ACK_RESULT ACK_FreeAllKeyProperties(
    ACK_SESSION session,
    LPVOID pReserved,        /* must be NULL */
    LPINT64 pCount,          /* in, out */
    ACK_LPKEYPROP *ppKeyProp /* in, out */
);
```

Releases the array allocated by `ACK_GetAllKeyProperties` or the
single-element block allocated by `ACK_GetKeyProperty`.

### `ACK_SizeOfKeyProp`

```c
SIZE_T ACK_SizeOfKeyProp(VOID);
```

Returns `sizeof(ACK_KEYPROP)`.

---

## Section 4 — Encryption & decryption

### `ACK_Encrypt`

```c
ACK_RESULT ACK_Encrypt(
    ACK_SESSION session,
    LPVOID *ppReserved,  /* must be NULL */
    UINT64 flags,        /* must be 0 */
    LPUINT64 pOffset,    /* out: global key offset used */
    LPBYTE pInput,
    SIZE_T inSize,
    LPBYTE *ppOutput,    /* in, out */
    LPSIZE_T pOutSize    /* in, out */
);
```

XOR-encrypts `inSize` bytes from `*pInput` into `**ppOutput`.

* On entry `*pOffset` **must be zero** (reserved for future use).
* On success `*pOffset` receives the **global** key offset at which
  the consumed key bytes started.  The caller must record this value
  (typically inside `CIPHER_TEXT_HEADER.u64DecryptKeyOffset`) so the
  matching `ACK_Decrypt` can locate the same bytes.
* `**ppOutput` may be `NULL` or too small on entry; in that case the
  engine allocates a fresh `*pOutSize` = `inSize` buffer with
  `ACK_zalloc` and stores the pointer.  Otherwise the caller-provided
  buffer is used in place.
* The engine performs ATTACH / read / UPDATE / COMMIT / DETACH around
  the chunk(s) that hold the consumed key bytes.  See *Chunking*
  below.
* `inSize` must not exceed `MAX_CHUNK_SIZE` (900 MB) per call.
* **Returns:** `ACK_S_OK`, or one of `E_HANDLE`, `E_POINTER`,
  `E_MUSTBENULL`, `E_MUSTBEZERO`, `E_OVERFLOW`,
  `ACK_E_NO_KEY_FOR_SESSION`, `ACK_E_NO_MOUNTED_DATABASE`,
  `ACK_E_WRONG_SESSION_TYPE`, `ACK_E_KEY_IS_DECRYPT_ONLY`,
  `ACK_E_KEY_IS_REVOKED`, `ACK_E_INSUFFICIENT_KEY_BYTES_REMAIN`,
  `ACK_E_TRANSACTION_PENDING`, `ACK_E_MAXIMUM_KEY_SIZE_EXCEEDED`, or a
  SQLite failure converted via `HRESULT_FROM_SQLITE`.

### `ACK_Decrypt`

```c
ACK_RESULT ACK_Decrypt(
    ACK_SESSION session,
    LPVOID *ppReserved,  /* must be NULL */
    UINT64 flags,        /* must be 0 */
    UINT64 offset,       /* in: same value returned by ACK_Encrypt */
    LPBYTE pInput,
    SIZE_T inSize,
    LPBYTE *ppOutput,    /* in, out */
    LPSIZE_T pOutSize    /* in, out */
);
```

Inverse of `ACK_Encrypt`.  `offset` is the **global** key offset
returned by the original encryption call.  Buffer rules for
`**ppOutput` / `*pOutSize` are identical to `ACK_Encrypt`.

* Decrypt is read-only; the engine attaches the chunks, reads,
  detaches, and applies the XOR.  No transaction is opened.
* **Returns:** as for `ACK_Encrypt`, plus
  `ACK_E_OFFSET_PLUS_SIZE_EXCEEDS_TOTAL` and
  `ACK_E_KEY_OFFSET_OVERFLOW`.

---

## Section 5 — Key generation & import (FEATURE_IMPORT)

These functions are only compiled when the SDK is built with
`-DFEATURE_IMPORT`.  They populate a key database one chunk at a time
from a raw key-material file.

### `ACK_PrepareChunk`

```c
ACK_RESULT ACK_PrepareChunk(
    ACK_SESSION session,
    INT64 version,         /* must be ACK_CHUNK_VERSION */
    LPVOID *ppReserved,    /* must be NULL */
    UINT64 chunkId,        /* must be > 0 */
    UINT64 size            /* 1..MAX_CHUNK_SIZE */
);
```

Opens the active key's database for read/write and allocates a
`zeroblob(size)` row in the `Chunks` table with primary key `chunkId`
(or grows an existing row if it is too small).  Requires session type
`ACKST_PrepareKey`.

### `ACK_ImportChunk`

```c
ACK_RESULT ACK_ImportChunk(
    ACK_SESSION session,
    INT64 version,         /* must be ACK_CHUNK_VERSION */
    LPVOID *ppReserved,    /* must be NULL */
    UINT64 chunkId,        /* must be > 0 */
    UINT64 offset,         /* in-chunk byte offset to write at */
    LPSTR fileName,        /* path to raw key-material file */
    UINT64 size            /* bytes to import */
);
```

Reads `size` bytes from the tail of `fileName`, truncates the file to
remove the consumed bytes, and writes them into the active key's
chunk `chunkId` at byte offset `offset`.  Truncating the source file
on success makes the import operation single-use, preventing
accidental reuse of one-time pad material.  Requires session type
`ACKST_ImportKey`.

---

## Section 6 — Memory helpers

The library allocates all caller-visible buffers with these helpers,
so callers must use them (not libc `malloc`/`free`) when freeing
library-allocated buffers or when allocating buffers they will hand
back to the library.

### `ACK_malloc`

```c
LPVOID ACK_malloc(SIZE_T size);
```

Allocates `size` bytes, suitably aligned for the platform.  A
non-`NULL` pointer is returned even when `size == 0`.

### `ACK_zalloc`

```c
LPVOID ACK_zalloc(SIZE_T size);
```

Same as `ACK_malloc` but zero-fills the returned region.

### `ACK_realloc`

```c
LPVOID ACK_realloc(LPVOID pBlock, SIZE_T size, BOOL zero);
```

Reallocates an existing block.  If `zero` is `TRUE` any newly added
tail bytes are zero-filled.

### `ACK_msize`

```c
SIZE_T ACK_msize(LPVOID pBlock);
```

Returns the allocator's record of `pBlock`'s size.  Returns `0` when
the platform allocator cannot report this; returns `(SIZE_T)-1` when
`pBlock` is `NULL`.

### `ACK_free`

```c
VOID ACK_free(LPVOID pBlock);
```

Releases a block previously returned by `ACK_malloc` / `ACK_zalloc` /
`ACK_realloc`, or by any library function whose documentation says
"the caller must release with `ACK_free`".  Passing `NULL` is a
no-op.

---

## Chunking

A one-time pad key in this engine is conceptually a single linear
byte stream identified by a GUID.  The pad is physically stored
across one or more SQLite chunk database files.  For a key whose
metadata records `ChunkSize > 0` and `TotalBytes` total, the pad is
split into `ceil(TotalBytes / ChunkSize)` chunks numbered from `0`.
Each chunk lives in its own SQLite file:

* Chunk 0 → `<base>` (no suffix)
* Chunk N → `<base>_N`

where `<base>` is the `Keys.FileName` column.  Inside each chunk file
the `Chunks` table holds a single row whose primary-key `Id` equals
the chunk index, with the pad bytes in the `Data` BLOB column.

When `ChunkSize == 0` the key is treated as a single legacy chunk;
the recorded `KeyOffsets.ChunkId` is the BLOB rowid and the file name
is the unsuffixed `<base>`.

### Caller perspective

**Callers do not need to know about chunks.**  The `pOffset` argument
to `ACK_Encrypt` / `ACK_Decrypt` is always a **global** byte offset
into the linear pad — exactly as recorded in the
`CIPHER_TEXT_HEADER.u64DecryptKeyOffset` field of the cipher-text
header.  The engine translates between global offsets and
`(chunkId, intraChunkOffset)` internally, ATTACHes whichever chunk
databases are needed (one or many) before opening any transaction,
performs the read, advances the recorded offset on encrypt, and
DETACHes the chunks before returning.

### Limits

* `MAX_CHUNK_SIZE` = 900,000,000 bytes (≈ 900 MB) per chunk.
* `MAX_CHUNK_COUNT` = 500 chunks per key.
* `MAX_KEY_SIZE` ≈ 450 GB per key.

A single `ACK_Encrypt` or `ACK_Decrypt` call may not request more
than `MAX_CHUNK_SIZE` bytes, but the requested range may straddle
chunk boundaries; the engine reads from every covered chunk in order
and concatenates the results.

---

## Error-code summary

All values come from `Navajo/4.0/include/navajo.h` (severity `1`,
facility `FACILITY_ACK = 95`) unless noted.  The high (sign) bit of
the 32-bit `HRESULT` distinguishes success (0) from failure (1).

| Code                                       | Meaning                                                   |
|--------------------------------------------|-----------------------------------------------------------|
| `ACK_S_OK`                                 | success                                                   |
| `ACK_S_FALSE`                              | success, boolean "no"                                     |
| `ACK_E_MISMATCHED_SESSION`                 | session handle does not match the recorded session        |
| `ACK_E_WRONG_SESSION_TYPE`                 | requested op not enabled by this session's type bitmask   |
| `ACK_E_KEY_FOR_SESSION`                    | a key is already selected; operation requires none        |
| `ACK_E_NO_KEY_FOR_SESSION`                 | no key selected; call `ACK_SetSessionKey` first           |
| `ACK_E_NO_FILE_NAME_FOR_KEY`               | key metadata lacks a `FileName`                           |
| `ACK_E_NO_KEY_DATABASE_FOR_SESSION`        | key DB path not resolved                                  |
| `ACK_E_MOUNTED_DATABASE`                   | a keyset is already mounted                               |
| `ACK_E_NO_MOUNTED_DATABASE`                | no keyset is mounted; call `ACK_MountKeySet` first        |
| `ACK_E_UNSUPPORTED_VERSION`                | version argument does not match the SDK version           |
| `ACK_E_KEY_IS_DECRYPT_ONLY`                | encrypt attempted with a decrypt-only key                 |
| `ACK_E_KEY_IS_REVOKED`                     | the key has been revoked                                  |
| `ACK_E_KEY_IS_TOO_SMALL`                   | request larger than the key's total bytes                 |
| `ACK_E_INSUFFICIENT_KEY_BYTES_REMAIN`      | not enough unused key bytes left                          |
| `ACK_E_OFFSET_PLUS_SIZE_EXCEEDS_TOTAL`     | decrypt offset+size past end of key                       |
| `ACK_E_MAXIMUM_KEY_SIZE_EXCEEDED`          | single-call size > `MAX_CHUNK_SIZE`                       |
| `ACK_E_KEY_OFFSET_OVERFLOW`                | per-chunk offset exceeds `INT_MAX`                        |
| `ACK_E_INVALID_CHUNK_ID`                   | chunk Id `0` supplied where positive expected             |
| `ACK_E_SESSION_CHECK_FAILED`               | session integrity check failed                            |
| `ACK_E_IO_ERROR`                           | OS I/O error                                              |
| `ACK_E_NO_CACHE`                           | cache requested but disabled                              |
| `ACK_E_NO_STORAGE`                         | no free cache slot                                        |
| `ACK_E_CACHE_IS_TOO_SMALL`                 | request larger than configured cache size                 |
| `ACK_E_TRANSACTION_PENDING`                | re-entrant call while a transaction is active             |
| `ACK_E_ALREADY_DONE`                       | operation already complete                                |
| `ACK_E_MISMATCHED_CHUNK_ID`                | chunk Id does not match expectations                      |
| `ACK_E_CHUNK_OFFSET_OVERFLOW`              | intra-chunk offset overflow                               |
| `ACK_E_CHUNK_SIZE_OVERFLOW`                | computed plan exceeds `MAX_CHUNK_COUNT`                   |
| `ACK_E_KEY_OFFSET_AND_SIZE_OVERFLOW`       | global offset + size overflow                             |
| `ACK_E_CHUNK_OFFSET_AND_SIZE_OVERFLOW`     | intra-chunk offset + size overflow                        |
| `E_HANDLE`, `E_POINTER`, `E_OUTOFMEMORY`, `E_INVALIDARG`, `E_OVERFLOW`, `E_MUSTBENULL`, `E_MUSTBEZERO`, `E_FAIL`, `E_NOINTERFACE`     | standard `hresult.h` codes |
