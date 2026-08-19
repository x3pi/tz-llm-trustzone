#!/bin/bash
# Full real cross-build of metanode's mvm_ta for chcore/musl, from source,
# inside vectorxj0553/tz-llm-llama-builder:latest (has musl-gcc baked in at
# the real chcore path). Re-derived 2026-08-17 after mvm_ta_main.cpp gained
# MVM_TZ_CMD_EXECUTE + storage_change/code_change/full_db_hash encoding —
# the previous session's c_mvm/linker build dirs were inside an ephemeral
# --rm container and did not survive. See README.md for full narrative.
set -e

MUSL_GCC=/home/vectorxj/chcore/staros/build/chcore-libc/bin/musl-gcc
MUSL_AR=/home/vectorxj/chcore/staros/build/chcore-libc/bin/musl-ar
MUSL_STRIP=/home/vectorxj/chcore/staros/build/chcore-libc/bin/musl-strip
TC=/home/vectorxj/mvm_toolchain.cmake
CPP11=/home/vectorxj/cpp11
PKG=/home/vectorxj/pkg
OUT=/home/vectorxj/out
C_MVM_BUILD=/home/vectorxj/c_mvm_build_aarch64

mkdir -p "$C_MVM_BUILD" "$OUT"

echo "=== [1/4] configuring+building c_mvm (libmvm.a) ==="
cmake -S "$PKG/mvm/c_mvm" -B "$C_MVM_BUILD" \
    -DCMAKE_TOOLCHAIN_FILE="$TC" \
    -DCPP13_ROOT="$CPP11" \
    -DUUID_SHIM_DIR="$PKG/mvm/ta/uuid_shim" \
    -DMVM_INSTALL_PREFIX="$C_MVM_BUILD" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTS=OFF
cmake --build "$C_MVM_BUILD" -j"$(nproc)"
cmake --install "$C_MVM_BUILD"
ls -la "$C_MVM_BUILD/libmvm.a"

echo "=== [2/4] configuring+building mvm_linker (libmvm_linker.a) ==="
LINKER_BUILD=/home/vectorxj/linker_build_aarch64
mkdir -p "$LINKER_BUILD"
cmake -S "$PKG/mvm/linker" -B "$LINKER_BUILD" \
    -DCMAKE_TOOLCHAIN_FILE="$TC" \
    -DCPP13_ROOT="$CPP11" \
    -DUUID_SHIM_DIR="$PKG/mvm/ta/uuid_shim" \
    -DMVM_C_MVM_BUILD_DIR="$C_MVM_BUILD" \
    -DMVM_3RDPARTY_ROOT="$CPP11/3rdparty" \
    -DXAPIAN_INCLUDE_DIR=/home/vectorxj/xapian_include \
    -DTBB_INCLUDE_DIR=/home/vectorxj/tbb_include \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTS=OFF
cmake --build "$LINKER_BUILD" -j"$(nproc)"
ls -la "$LINKER_BUILD/libmvm_linker.a"

echo "=== [3/4] compiling mvm_ta_main.cpp ==="
"$MUSL_GCC" -std=c++20 -O2 -c \
    -I"$PKG/mvm/tzproto" \
    -I"$PKG/mvm/linker/include" \
    -I/home/vectorxj/chcore_include \
    -I"$CPP11/include/aarch64-linux-musleabi" \
    -I"$CPP11/include" \
    "$PKG/mvm/ta/mvm_ta_main.cpp" \
    -o "$OUT/mvm_ta_main.o"
ls -la "$OUT/mvm_ta_main.o"

echo "=== [4/4] final link -> mvm_ta ==="
# -Wl,-z,text: FAIL LOUDLY at link time if any TEXTREL would be needed,
# naming the exact object/symbol responsible, instead of silently
# producing a DT_TEXTREL binary that chcore's secure-world loader then
# rejects at runtime with the opaque "Not a valid dynamic program"
# (root-caused 2026-08-17 via UART debug tracing of map_library() --
# see plan doc §9.10 -- llama-cli's own binary has NO TEXTREL, confirmed
# via readelf -d comparison). Diagnostic-only for this pass: if this
# fails, read the error to find the culprit object, don't just remove
# the flag to make the build "succeed" again.
"$MUSL_GCC" -o "$OUT/mvm_ta" \
    "$OUT/mvm_ta_main.o" \
    -Wl,--start-group \
    "$LINKER_BUILD/libmvm_linker.a" \
    "$C_MVM_BUILD/libmvm.a" \
    "$CPP11/3rdparty/lib/libmpfr.a" \
    "$CPP11/3rdparty/lib/libgmp.a" \
    "$CPP11/3rdparty/lib/libsecp256k1.a" \
    "$CPP11/3rdparty/lib/libblst.a" \
    /home/vectorxj/xapian_tbb_libs/libxapian.a \
    /home/vectorxj/xapian_tbb_libs/libtbb.a \
    /home/vectorxj/xapian_tbb_libs/libz.a \
    -Wl,--end-group \
    -L"$CPP11/lib" "$CPP11/lib/libstdc++.so" "$CPP11/lib/libgcc_s.so" \
    -lpthread \
    -Wl,-z,text 2>&1 | tee "$OUT/final_link_ztext.log"

echo "=== stripping (avoid the 64MiB FIT-image overflow hit 2026-08-17) ==="
"$MUSL_STRIP" --strip-all "$OUT/mvm_ta" 2>&1 || aarch64-linux-gnu-strip --strip-all "$OUT/mvm_ta"

cp "$CPP11/lib/libstdc++.so.6.0.29" "$OUT/"
cp "$CPP11/lib/libgcc_s.so.1" "$OUT/"
aarch64-linux-gnu-strip --strip-all "$OUT/libstdc++.so.6.0.29" || true
aarch64-linux-gnu-strip --strip-all "$OUT/libgcc_s.so.1" || true
ln -sf libstdc++.so.6.0.29 "$OUT/libstdc++.so.6"

echo "=== DONE ==="
ls -la "$OUT/"
file "$OUT/mvm_ta"
readelf -d "$OUT/mvm_ta" | grep -i needed
