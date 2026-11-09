# Compile Botan 3.5.0

The pinned source tree of **Botan 3.5.0** (randombit/botan @ `3.5.0`) is already
extracted at `/workspace/src`. All required build tools and dependencies are
preinstalled in this container. **There is no network access** — do not try to
download anything; everything you need is already here.

## Task

Compile the source at `/workspace/src` and produce the following artifacts:

- `libbotan-3.so`
- `libbotan-3.a`
- `botan`

Place them (or simply leave them where the build system puts them) anywhere
under `/workspace`. They must be **real compiled outputs** — ELF binaries /
shared objects, or `ar` archives for `.a` files. Empty or fake files with the
right names will not pass, and neither will unrelated binaries copied from
the system and renamed — the grader inspects the ELF/archive contents, so
only artifacts genuinely built from the vendored source tree count. Keep
the artifacts **unstripped** (do not run `strip`) — the grader reads their
symbol tables.

## Notes

- You have 4 CPUs; parallel builds (`make -j4`) are recommended.
- Building tests, docs, or running test suites is NOT required — only the
  artifacts listed above matter.
