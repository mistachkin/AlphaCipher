# Security

## Supported versions

| Version | Supported          |
|---------|--------------------|
| 4.0.x   | :white_check_mark: |
| < 4.0   | :x:                |

## Reporting a vulnerability

Please report suspected vulnerabilities **privately** using GitHub's
*Report a vulnerability* button under this repository's *Security*
tab.  Do not open a public GitHub issue or pull request for a
suspected vulnerability.

When you submit a report, please include:

- the affected file path and line number(s);
- a short proof-of-concept that demonstrates the issue (a minimal C
  program or shell snippet is ideal);
- the platform and toolchain you reproduced on (operating system,
  compiler version, build configuration);
- whether the issue is reachable from a remote attacker, a local
  user, the operating system itself, or another threat model you
  consider relevant.

You can expect:

- an acknowledgement of the report within five business days;
- a triage classification (out of scope / accepted / needs more
  information) within ten business days;
- regular status updates while a fix is being developed.

We will coordinate public disclosure with you once a fix is available.

## Scope

In scope:

- Anything under `Navajo/4.0/src/`, `Navajo/4.0/include/`,
  `Navajo/4.0/test/`, `Navajo/4.0/phone/`, `Navajo/4.0/tinCan/`, and
  `Common/Shared/`.
- The build / packaging scripts under `Tools/` insofar as they touch
  signing keys or other credentials.

Out of scope:

- The third-party SQLite amalgamation under
  `Common/Externals/sqlite3/`.  Report SQLite issues to upstream at
  https://www.sqlite.org/ .
- Issues that depend on running the SDK with attacker-controlled
  build configuration.

## Coordinated disclosure

We follow standard coordinated-disclosure practice.  We ask reporters
to refrain from publicising a vulnerability until a fix has been
released or a mutually agreed disclosure date has passed.
