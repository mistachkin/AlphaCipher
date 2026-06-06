<!--
  Thanks for the PR.  Please fill in the sections below.  Delete the
  ones that genuinely do not apply (a one-line "n/a" is fine).
-->

## Summary

What does this PR change, and why?

## Linked issues

- Fixes #
- Refs #

## Type of change

- [ ] Bug fix (non-breaking)
- [ ] Behavior change (potentially breaking)
- [ ] New feature
- [ ] Documentation only
- [ ] Build / packaging / CI
- [ ] Test only

## Testing

Describe what you did locally:

- [ ] `make clean && make` on Linux — zero warnings, zero errors
- [ ] `obj/sdkTest` smoke test passes
- [ ] VS2008 build (Win32 + x64, ReleaseDll) — describe configurations exercised
- [ ] WCE build, if applicable
- [ ] New tests added / existing tests updated

If you exercised any non-standard configuration (different SQLite
version, alternative compiler, etc.), say so.

## Documentation

- [ ] `CHANGELOG.md` updated under `[Unreleased]`
- [ ] Public API change reflected in `Navajo/4.0/doc/public_api.md`
- [ ] Header comments updated for any file whose copyright year you
      bumped or whose semantics you changed

## House style

- [ ] Existing brace style, indentation, and naming conventions
      preserved (see `CONTRIBUTING.md` § "House style")
- [ ] No new `strcpy` / `sprintf` / unbounded `strcat`
- [ ] `ACK_ASSERT` calls use the `condition && "tag"` idiom
- [ ] File ends with `/* end of file */`

## Notes for reviewers

Anything reviewers should pay particular attention to — subtle
ordering, race windows, transaction state, etc.

<!--
  If the change touches the cryptographic core (anything under
  Navajo/4.0/src/navajo*.c or Navajo/4.0/src/protocol.c), please flag
  it explicitly above.  Those changes receive additional scrutiny.
-->
