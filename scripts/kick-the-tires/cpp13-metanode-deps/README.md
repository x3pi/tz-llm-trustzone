# cpp13-metanode-deps

## ⭐ 2026-08-17 UPDATE: real, DEPLOYABLE `mvm_ta` build now works — read this first

Everything below this section describes the **superseded** approach (GCC
13.3.0, generic musl-cross-make musl runtime) — kept for its own value
(the 5 libs it cross-built are still used) but **NOT what actually
produces a deployable TA binary**. That approach's own artifacts (headers/
`libstdc++.so` built against GCC 13.3.0) turned out to be **compiler
-version-incompatible** with the real chcore build's actual compiler
(`musl-gcc`, which wraps `aarch64-linux-gnu-gcc-11`, i.e. **GCC 11.4.0**) —
confirmed via a real reproduction: `<chrono>` alone fails with `'_Float32'
was not declared in this scope` when GCC13-built libstdc++ headers are fed
to a GCC11 compiler frontend (`_Float32`/`_Float64`/`_Float128` as C++
*types*, not just the `__FLT32_DIG__` limit macro, were only added to
GCC's C++ frontend in GCC 13 — a GCC11 frontend predefines the macro but
doesn't recognize the type).

**The real fix**: build a *second* musl-cross-make toolchain pinned to
**`GCC_VER=11.5.0`** (closest available release to the Docker image's
actual 11.4.0; same major/minor, libstdc++ ABI/header-compatible) —
config lives at `/home/pi/musl-cross-build-scratch-gcc11/musl-cross-make/
config.mak` on this machine as of this writing, `make && make install`
into `/home/pi/musl-cross-build-scratch-gcc11/output/`. Stage its
`aarch64-linux-musleabi/include/c++/11.5.0/*` + `lib/libstdc++.so*`/
`libgcc_s.so*` the same flattened way as before (this staged form is
`cpp11-stage.tar.gz` here, gitignored — regenerate from that toolchain
output), **plus copy over the same `3rdparty/` dir this file's original
cpp13 stage already built** (GMP/MPFR/secp256k1/libuuid/BLST — pure C
libraries, ABI-stable across GCC versions, safe to reuse as-is) — except
**GMP and MPFR needed rebuilding with `--with-pic`** (the originals
weren't position-independent, and chcore's real link is a PIE binary —
surfaced as `relocation R_AARCH64_ADR_PREL_PG_HI21 ... can not be used
when making a shared object; recompile with -fPIC` at final-link time).

Real build command shape (see `mvm_toolchain_chcore_real.cmake` in this
dir for the actual CMake toolchain file used for `c_mvm`/`linker`):
compile `c_mvm` and `linker` via CMake with `CMAKE_C_COMPILER`/
`CMAKE_CXX_COMPILER` set to `/home/vectorxj/chcore/staros/build/
chcore-libc/bin/musl-gcc` (the *real* chcore compiler wrapper — correct
musl syscall/struct ABI) and `CMAKE_AR`/`CMAKE_RANLIB` set to
`/usr/bin/aarch64-linux-gnu-ar`/`-ranlib` (musl-gcc's own bundled
`musl-ar` wrapper exists but there's no matching `musl-ranlib` — use the
system aarch64 ranlib directly, it doesn't need musl-specific behavior).
`metanode/execution/pkg/mvm/ta/mvm_ta_main.cpp` compiles directly with the
same `musl-gcc`, needing `-I` on this repo's real
`tz-llm/tee_os_kernel/user/system-services/chcore-libc/musl-libc/install/
include` (the image's own baked-in `chcore-libc/include/chcore/` is an
**older snapshot missing `llm.h`/`TZASC_NR`** — mount/use the host repo's
copy instead). Final link is `musl-gcc` again, `-Wl,--start-group ...
--end-group` over `libmvm_linker.a`+`libmvm.a`+the 5 3rdparty libs+
Xapian+TBB+zlib, then `-L.../lib libstdc++.so libgcc_s.so` for the C++
runtime (mirrors exactly how this repo's own `.cpp/aarch64` gets linked
into `llama-cli` — same pattern, just GCC11.5.0-built libstdc++ instead
of GCC9.2.0's).

**Result, verified 2026-08-17**: `mvm_ta` — a real, position-independent
executable (`readelf -h`: `Type: DYN`, `Machine: AArch64`), 42.9MB,
`FINAL_LINK_EXIT=0`, zero undefined symbols, built entirely with real
production-shaped tooling (no mixed musl runtimes, no compile-only-verify
shortcuts). Needs `libstdc++.so.6.0.29`/`libgcc_s.so.1` alongside it at
runtime (same as `llama-cli` needs its own `.cpp/aarch64/lib/*.so` copies
in the ramdisk). **Still NOT flashed/tested on real hardware** — this is
"builds and links correctly", not "proven correct at runtime". See
`metanode/note/tee_dual_mode_execution_plan.md` §9.6 for the full,
dated writeup (Xapian/TBB/zlib/xapian's own `.a` files, incidentally,
*are* still the ones originally built with the GCC13.3.0-era toolchain —
they're pure C++ but didn't hit the `_Float32` issue since they don't
transitively include `<chrono>`'s C++20 format/stream integration; flagged
as an open libstdc++-ABI-across-GCC-versions risk worth double-checking
before this is trusted as fully correct, not just "links without error").

**⭐ 2026-08-17 UPDATE #2: MUST strip before baking into the ramdisk — the
FIT image has a hard 64MiB ceiling**

First real `rebuild.sh` run with `mvm_ta` baked alongside `llama-cli` failed
for real at the `boot_merger`/`fast_build_uboot.sh` step:
```
ERROR: pack uboot.img failed! fit/uboot.itb actual: 116255744 bytes, max limit: 67108864 bytes
```
`uboot.itb` (kernel+ATF+U-Boot+the OP-TEE blob, which itself contains the
TEE-OS ramdisk with `mvm_ta`+`llama-cli`+the model gguf) has a **hard 64MiB
cap** (from `RK3588MINIALL.ini`/`boot_merger`) — the real build came out to
~110.9MiB, nearly double. `rebuild.sh`'s `| tee` has no `set -o pipefail`,
so the reported exit code was misleadingly 0 despite this failure — don't
trust the exit code alone, always grep the log for `ERROR`.

**Root cause + fix**: the original build (`mvm_ta` 42.9MB,
`libstdc++.so.6.0.29` 17.6MB, `libgcc_s.so.1` 0.5MB — ~58.3MB total) was
never stripped of debug symbols. `aarch64-linux-gnu-strip --strip-all`
(removes only `.symtab`/`.debug_*`, leaves `.dynsym`/`.dynstr`/SONAME/NEEDED
untouched — confirmed via `readelf -d`/`--dyn-syms` unchanged before/after)
brought it down to:
- `mvm_ta`: 42.9MB → **5.1MB**
- `libstdc++.so.6.0.29`: 17.6MB → **2.1MB**
- `libgcc_s.so.1`: 0.5MB → **0.13MB**
- Total: ~58.3MB → **~7.04MB**, saving ~51.2MB (needed at least ~46.9MB to
  fit under 64MiB — leaves only ~4.4MB margin, fairly tight).

`mvm_ta_output/` (gitignored) now holds the STRIPPED build as the primary
copy; the original unstripped build is preserved at
`mvm_ta_output/unstripped-backup/` (useful for gdb/addr2line debugging
later — rebuild with debug info on demand, don't overwrite this backup).
**Any future rebuild of `mvm_ta` MUST strip before baking into
`oh_tee/apps`**, or this `boot_merger` failure will recur. Worth adding
`-s` to the `musl-gcc` final link flags (strip at link time) or a strip
step right after build in the pipeline, instead of doing it by hand like
this time.

---

A **separate, parallel** toolchain image for the `metanode` project's TZ
dual-mode-execution work (GĐ3 — see
`metanode/note/tee_dual_mode_execution_plan.md`, 2026-08-16 entries). This is
**not** part of this repo's own LLM TA build pipeline — it exists purely so
metanode's `mvm`+`linker` C++ code (and its 5 external deps: GMP, MPFR,
secp256k1, libuuid, BLST) can eventually be compiled against a real
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
  v0.2.0 (recovery module built), libuuid (from util-linux 2.39.3), BLST
  (BLS12-381, compiled directly from metanode's own vendored
  `pkg/bls/blst/src/server.c` + `build/assembly.S` — no separate upstream
  release/tarball, this *is* the exact source metanode itself ships):
  real headers + static `.a`. Not part of any existing chcore/staros
  convention — this is metanode-specific, added here for convenience.
  BLST's need was only discovered via a full-stack link test (all 4
  original libs + mvm + linker linked into one real executable) turning
  up 13 undefined `blst_*` symbols from `c_mvm/src/crypto/kzg.cpp`
  (KZG/EIP-4844 blob verification) — see the plan doc, 2026-08-16.

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
3. BLST built by compiling `metanode/execution/pkg/bls/blst/src/server.c`
   (includes all the other .c files) + `.../blst/build/assembly.S`
   directly with the toolchain's gcc (`-O2 -fno-builtin -fPIC`, no
   configure step needed — the vendored source has no build.sh, just
   compile+archive those 2 files), `ar`-archived together. See the plan
   doc's "Full-stack link test thật" entry, 2026-08-16.

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
