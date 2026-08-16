# Adds a SEPARATE, PARALLEL toolchain image for metanode's dual-mode-execution
# TA work (GĐ3, see metanode/note/tee_dual_mode_execution_plan.md) — NOT part
# of this repo's own LLM TA build pipeline.
#
# Contents baked in at /home/vectorxj/chcore/staros/.cpp13/aarch64/:
#   - NOT the GCC 13.3.0 compiler binary itself — confirmed (2026-08-16) it
#     cannot run inside this image: `GLIBC_2.36'/`GLIBC_2.38' not found
#     against this image's base OS glibc (same known mismatch already hit
#     building metanode's mvm/linker this session). The compiler must be
#     invoked on the HOST instead
#     (/home/pi/musl-cross-build-scratch/output/bin/aarch64-linux-musleabi-*
#     as of this writing) — this image only carries its *output* (headers,
#     runtime .so, static libs), which are just data and run fine anywhere.
#   - Its C++ headers + libstdc++.so/libgcc_s.so runtime, laid out the same
#     flattened way as this repo's own .cpp/aarch64/{include,lib} (built by
#     the in-image scripts/build/buildup_cpp.sh) so the existing
#     toolchain.cmake convention (${_cpp_install_prefix}/${arch}/include,
#     /lib) still applies if pointed at .cpp13 instead of .cpp.
#   - 5 real cross-built libs under .../3rdparty/{include,lib}: GMP 6.3.0,
#     MPFR 4.2.1, secp256k1 v0.2.0 (recovery module), libuuid (util-linux
#     2.39.3), BLST (BLS12-381, from metanode's own vendored
#     pkg/bls/blst source — compiled directly from src/server.c +
#     build/assembly.S, no separate upstream release) — headers + static
#     .a, matching real API usage in metanode's mvm/linker C++ (see the
#     plan doc's 2026-08-16 entries for exact configure flags / build log
#     evidence for each, including the full-stack link test that's how
#     BLST's need was discovered in the first place).
#
# Deliberately does NOT touch /home/vectorxj/chcore/staros/.cpp/aarch64/
# (GCC 9.2.0) — that is this repo's own production C++ toolchain for the
# LLM TA, already flashed + verified on real hardware. Do not repoint
# build-llama-docker.sh at this image; it is for metanode-side experiments
# only, run manually via a separate `docker run` (see this dir's README.md).
FROM vectorxj0553/tz-llm-llama-builder:latest

COPY cpp13-metanode-deps.tar.gz /tmp/cpp13-metanode-deps.tar.gz
RUN mkdir -p /home/vectorxj/chcore/staros/.cpp13 && \
    tar -xzf /tmp/cpp13-metanode-deps.tar.gz -C /home/vectorxj/chcore/staros/.cpp13 && \
    rm /tmp/cpp13-metanode-deps.tar.gz
