# cpp13-metanode-deps

A **separate, parallel** toolchain image for the `metanode` project's TZ
dual-mode-execution work (GĐ3 — see
`metanode/note/tee_dual_mode_execution_plan.md`, 2026-08-16 entries). This is
**not** part of this repo's own LLM TA build pipeline — it exists purely so
metanode's `mvm`+`linker` C++ code (and its 4 external deps: GMP, MPFR,
secp256k1, libuuid) can eventually be compiled against a real
`aarch64-linux-musleabi` toolchain from inside a container that also has
this repo's chcore/musl-libc headers available for reference, without ever
touching the production `.cpp/aarch64` (GCC 9.2.0) toolchain that this
repo's actual flashed-on-hardware LLM TA depends on.

## What's in the image (and what's deliberately NOT)

`docker build` here layers one thing on top of
`vectorxj0553/tz-llm-llama-builder:latest`: a tarball extracted to
`/home/vectorxj/chcore/staros/.cpp13/aarch64/`.

**The GCC 13.3.0 compiler binary itself is NOT in this image** — confirmed
(2026-08-16) it cannot run inside this container: `aarch64-linux-musleabi-g++
--version` fails with `GLIBC_2.36'/`GLIBC_2.38' not found` against this
image's base OS glibc. This is the exact same host/Docker glibc mismatch
already hit building metanode's `mvm`+`linker` earlier this session — the
image's base OS predates the glibc the new toolchain's own binaries were
linked against. **Compile with the toolchain on the HOST instead**, then use
this container (or just these files directly, Docker not required) only for
the resulting headers/libs:

```
/home/pi/musl-cross-build-scratch/output/bin/aarch64-linux-musleabi-g++ ...
```

(path as of 2026-08-16; rebuild via musl-cross-make if it's gone). Everything
actually baked into this image is *data* (headers, a prebuilt runtime `.so`,
static `.a` archives) — those run/link fine regardless of host glibc, only
the compiler executable itself was affected.

- `include/`, `lib/` — the GCC13 toolchain's C++ headers + `libstdc++.so`/
  `libgcc_s.so`, flattened the same way `buildup_cpp.sh` lays out this
  repo's own `.cpp/aarch64/{include,lib}`, so the existing
  `toolchain.cmake` `_cpp_install_prefix` convention still works if you
  point it at `.cpp13` instead of `.cpp`.
- `3rdparty/include/`, `3rdparty/lib/` — GMP 6.3.0, MPFR 4.2.1, secp256k1
  v0.2.0 (recovery module built), libuuid (from util-linux 2.39.3): real
  headers + static `.a`. Not part of any existing chcore/staros convention
  — this is metanode-specific, added here for convenience.

## Known caveats (carried over from the plan doc, still open)

- None of this was built against chcore-libc's *actual* musl-libc source
  (`tz-llm/tee_os_kernel/user/system-services/chcore-libc/musl-libc/`) —
  same as this repo's own production C++ toolchain. TBB's futex fallback
  and libuuid's `getrandom()` codepath were only exercised against generic
  musl-cross-make musl, which *does* define `SYS_futex`/`SYS_getrandom` —
  this repo's chcore does **not** have a `getrandom()` syscall at all
  (confirmed by reading `kernel/syscall/syscall_num.h`). Don't assume
  either behaves identically on real chcore without a real chcore-libc
  build to check against.
- This produces a *compile-only* feasibility image — nothing here has run
  on the actual board. Getting from "compiles inside this container" to
  "runs as a real TA" still needs GĐ3's remaining work: wiring this
  toolchain into an actual TA CMake target, baking it into this repo's
  `oh_tee/apps` via `rebuild.sh`/`repack.sh`, and going through the full
  8-step build/flash/test checklist in `CLAUDE.md`.

## Regenerating `cpp13-metanode-deps.tar.gz`

The tarball itself is **not committed to git** (360MB, build artifact) —
build it fresh from:
1. The GCC 13.3.0 `aarch64-linux-musleabi` toolchain (built via
   musl-cross-make on the host, see `metanode/note/tee_dual_mode_execution_plan.md`
   for the exact steps that produced it).
2. GMP/MPFR/secp256k1/libuuid cross-built with that toolchain — exact
   `configure`/`cmake` flags and source versions are logged in the same
   plan doc's 2026-08-16 "CROSS-BUILD THẬT ĐÃ XONG" entry.

Layout expected inside the tarball (`tar czf cpp13-metanode-deps.tar.gz aarch64`
from a staging dir): `aarch64/{include,lib,3rdparty/{include,lib}}` — no
`toolchain/` (see above: doesn't run in-container, not worth the ~800MB).

## Building and using this image

```bash
# from this directory, with cpp13-metanode-deps.tar.gz present:
docker build -t vectorxj0553/tz-llm-llama-builder:cpp13-metanode-deps .

# compile on the HOST with the real toolchain, e.g.:
NEWTC=/home/pi/musl-cross-build-scratch/output
"$NEWTC/bin/aarch64-linux-musleabi-g++" -static \
    -I.../3rdparty/include -L.../3rdparty/lib -lmpfr -lgmp ...

# use this image only if/when a build step needs these headers/libs
# available inside a container (e.g. alongside this repo's own
# chcore-libc-mounted build steps):
docker run --rm -it vectorxj0553/tz-llm-llama-builder:cpp13-metanode-deps bash
#   headers/libs: /home/vectorxj/chcore/staros/.cpp13/aarch64/{include,lib,3rdparty}
```
