---
name: Bug report
about: A defect, regression, or unexpected behavior in the AlphaCipher SDK
title: ''
labels: bug
assignees: ''
---

## Summary

One or two sentences describing what went wrong.

## Affected component

- [ ] Engine core (`Navajo/4.0/src/navajo*.c`, `protocol.c`, etc.)
- [ ] Storage backend (SQL schema, chunk attach/detach)
- [ ] OS abstraction (`Navajo/4.0/src/os/`)
- [ ] Embedded reference apps (`Navajo/4.0/phone/`, `Navajo/4.0/tinCan/`)
- [ ] Tests / smoke harness
- [ ] Build system (Makefile, `.vcproj`, `.vsprops`)
- [ ] Documentation
- [ ] Other (describe)

## Environment

- AlphaCipher version / commit:
- Operating system + version:
- Compiler + version:
- Build configuration: <!-- Linux Makefile / VS2008 Debug|DebugDll|Release|ReleaseDll, Win32|x64, WCE -->

## Steps to reproduce

1. ...
2. ...
3. ...

## Expected behavior

What you expected to happen.

## Actual behavior

What actually happened.  Include exact error messages, return codes,
and (if available) a stack trace.

## Minimal reproducer

If you can produce a self-contained C source or shell snippet that
demonstrates the issue, paste it here:

```c
/* ... */
```

## Additional context

Anything else that may help triage — related issues, recent changes
to your local tree, etc.

> **Reminder:** if this report concerns a security-relevant defect,
> please **stop** and follow [`SECURITY.md`](../../SECURITY.md)
> instead.  Do not paste exploit details into a public issue.
