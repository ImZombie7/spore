# Userland Cleanup Migration

## Phase 0 Decision

Spore will use one static musl binary per tool, not a busybox-style multiplexer.

Reason: separate binaries are independently confineable. A future policy can give
`cat` a different manifest than `ls`, which matches Spore's capability model. The
tradeoff is image size because each binary carries static musl; shrinking can be a
later dynamic-linking or multiplexer goal. The current filesystem has no symlinks,
so a multiplexer would also need hardlink-style dispatch or shell support.

## Final Binary Disposition

Real tools, target `/bin`:

- `spsh`
- `ls`
- `cat`
- `echo`
- `mkdir`
- `rm`
- `touch`
- `pwd`
- `true`
- `false`
- `hello`

Confinement fixtures, target `/demos`:

- `spinner`
- `peeker`
- `writer`
- `memhog`
- `escalate`

Integration-only checks, moved to the explicit `run-tests` image:

- `spore_demo`
- `exec_child`

Duplication to resolve:

- Keep canonical `/bin/hello`.
- Drop `/hello` and `/boot/hello` from the baked image.

## Result

- `/bin` contains only real tools and `spsh`.
- Confinement fixtures are baked under `/demos`.
- The interactive image is generated from `userland/image.manifest`.
- The regression gauntlet is generated from `userland/tests/integration/image.manifest`
  and runs with `meson compile -C build run-tests`.
- The separate-static-binary layout grows the interactive ISO to about 110 MB and
  the test ISO to about 71 MB. Shrinking is a later dynamic-linking or
  multiplexer goal.

## Known Test Debt

The v2 regression log currently shows `stdin demo: blocking read resume: SKIP`
unless driven by the special stdin harness. Closing that gap is explicitly outside
this cleanup goal; preserve the note while moving the regression binary.
