# Real, self-contained GCC 11.5.0 `aarch64-linux-musleabi` cross-compiler
# (built via musl-cross-make, output at
# /home/pi/musl-cross-build-scratch-gcc11/output/ -- see
# cpp13-metanode-deps/README.md's "2026-08-17 UPDATE" section for how it was
# built) -- used ONLY to compile c_mvm/linker to OBJECT FILES (static
# libraries, no final executable link happens here), replacing
# mvm_toolchain_chcore_real.cmake's `musl-gcc` (chcore-libc's specs-wrapper
# around Ubuntu's aarch64-linux-gnu-gcc-11, GCC 11.4.0) for this step.
#
# WHY (2026-08-21, tz-llm-trustzone DEPLOYED_STATE.md "root-caused SEND_NATIVE
# hang" entry): confirmed via real UART tracing that a plain `throw
# std::runtime_error(...)` inside sendNative() (mvm_linker.cpp, compiled by
# musl-gcc/GCC 11.4.0-Ubuntu) never reaches its own `catch` -- the TA hangs
# silently. Direct .comment-section inspection (not assumption) showed the
# C++ exception RUNTIME actually linked into mvm_ta at final-link time
# (libstdc++.so.6.0.29 + libgcc_s.so.1, from $CPP11/lib/, used by every
# build to date) is GCC 11.5.0 from THIS EXACT toolchain -- a real,
# self-consistent musl-cross-make build, NOT the same lineage as musl-gcc's
# Ubuntu-patched GCC 11.4.0. Project history (chưa từng có 1 lần exception=1
# nào trên bất kỳ command nào của mvm_ta) means this mismatch was never
# exercised before today. This toolchain file makes the COMPILER match the
# RUNTIME it's actually linked against, for the specific object code that
# needs correct exception unwinding (c_mvm, linker, mvm_ta_main.cpp, and
# libxapian.a/libz.a -- see mvm_toolchain_musleabi_gcc11_xapian_build.sh).
#
# Deliberately NOT used for the final mvm_ta executable link (that step
# stays on `musl-gcc`, unchanged) -- musl-gcc's specs file transparently
# supplies chcore's own patched musl crt0/libc (confirmed correct syscall/
# struct ABI per mvm_toolchain_chcore_real.cmake's own doc comment); this
# GCC-11.5.0 musleabi toolchain is a GENERIC musl-cross-make build with no
# chcore-specific patches, so linking a full executable with it directly
# would risk wrong syscall numbers/struct layouts at runtime (a much worse,
# harder-to-diagnose failure mode than the exception-ABI issue being fixed
# here). Since only `-c` (compile-to-object) is ever invoked through this
# toolchain file (c_mvm/linker are CMake STATIC libraries -- `ar`, not
# `ld`, packages them; mvm_ta_main.cpp is compiled with `-c` directly in
# build_mvm_ta.sh), no crt/libc linkage from this toolchain is ever
# actually pulled in -- confirmed safe by construction, not just assumed.
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(MUSLEABI_GCC11_ROOT /home/pi/musl-cross-build-scratch-gcc11/output)
set(CMAKE_C_COMPILER ${MUSLEABI_GCC11_ROOT}/bin/aarch64-linux-musleabi-gcc)
set(CMAKE_CXX_COMPILER ${MUSLEABI_GCC11_ROOT}/bin/aarch64-linux-musleabi-g++)
set(CMAKE_AR ${MUSLEABI_GCC11_ROOT}/bin/aarch64-linux-musleabi-ar)
set(CMAKE_RANLIB ${MUSLEABI_GCC11_ROOT}/bin/aarch64-linux-musleabi-ranlib)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# This toolchain's own bundled C++20 stdlib headers (real GCC 11.5.0,
# matches the linked runtime exactly) -- no $CPP13_ROOT override needed
# here, unlike mvm_toolchain_chcore_real.cmake (that override existed
# specifically to paper over chcore-libc's own too-old GCC 9.2.0 headers;
# this toolchain has no such gap).

if(NOT DEFINED CPP13_ROOT)
  set(CPP13_ROOT /tmp/cpp13/aarch64)
endif()

if(DEFINED UUID_SHIM_DIR)
  include_directories(BEFORE SYSTEM ${UUID_SHIM_DIR})
endif()

# 3rdparty (gmp/mpfr/secp256k1/blst) headers -- still the GCC13-built ones
# from CPP13_ROOT/3rdparty; these are pure C, ABI-stable across GCC
# versions (see README.md's own note), so no need to rebuild them here.
include_directories(SYSTEM
  ${CPP13_ROOT}/3rdparty/include
)

if(DEFINED XAPIAN_INCLUDE_DIR)
  include_directories(${XAPIAN_INCLUDE_DIR})
endif()

if(DEFINED TBB_INCLUDE_DIR)
  include_directories(${TBB_INCLUDE_DIR})
endif()
