# Real chcore-libc toolchain (matches how tz-llm-trustzone's own llama-cli
# is built for the LLM TA): musl-gcc (chcore-libc's real specs-wrapper
# around aarch64-linux-gnu-gcc-11, correct musl syscall/struct ABI) for the
# C runtime, with C++20 STL support supplied by a SEPARATE musl-cross-make
# build (GCC 13.3.0) since chcore-libc's own .cpp/aarch64 (GCC 9.2.0) lacks
# <concepts>/<compare>/<ranges> that intx (c_mvm's bigint dependency)
# genuinely needs -- confirmed via a real -fsyntax-only compile this
# session, not assumed. This EXACT "real musl C runtime + separate newer
# libstdc++" combination is how llama-cli itself already works on real
# hardware (.cpp/aarch64's own libstdc++.so, dynamically linked in via
# CXX_FLAGS) -- only the libstdc++ VERSION differs here, not the pattern.
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(MUSL_GCC /home/vectorxj/chcore/staros/build/chcore-libc/bin/musl-gcc)
set(CMAKE_C_COMPILER ${MUSL_GCC})
set(CMAKE_CXX_COMPILER ${MUSL_GCC})
set(CMAKE_AR /home/vectorxj/chcore/staros/build/chcore-libc/bin/musl-ar)
set(CMAKE_RANLIB /usr/bin/aarch64-linux-gnu-ranlib)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# 2026-08-21: gates c_mvm/include/mvm/safe_throw.h's setjmp/longjmp-based
# throw/catch replacement (C++ exceptions confirmed broken in this exact
# musl/chcore TA build -- see tz-llm-trustzone/DEPLOYED_STATE.md). This is
# the ONLY toolchain file that defines MVM_TA_BUILD -- the x86/cgo
# production build and the aarch64-linux-gnu glibc board cross-compile
# (execution/pkg/mvm/cmake/aarch64-linux-gnu.cmake) both leave it undefined
# and keep real throw/catch, byte-for-byte unchanged.
add_definitions(-DMVM_TA_BUILD=1)

if(NOT DEFINED CPP13_ROOT)
  set(CPP13_ROOT /tmp/cpp13/aarch64)
endif()

# TLS-free <uuid/uuid.h> shim (metanode/execution/pkg/mvm/ta/uuid_shim) --
# MUST resolve before the real libuuid header from MVM_3RDPARTY_ROOT/include
# (added later via linker/CMakeLists.txt's target_include_directories), or
# xapian_manager.cpp picks up the real libuuid's declarations with nothing
# to link against (libuuid.a is deliberately NOT linked into mvm_ta -- see
# uuid_shim/uuid/uuid.h's own doc comment for why: chcore's secure-world
# loader rejects any ELF with a PT_TLS segment, and real libuuid's
# uuid_generate_random() uses thread-local state internally). Confirmed via
# a real failed build (2026-08-17) that relying on CMAKE_C_FLAGS/CXX_FLAGS
# ordering alone does NOT reliably win this race -- include_directories()
# calls from a target's own CMakeLists.txt can still resolve first depending
# on generator. BEFORE here is the actual fix, not just a preference.
if(DEFINED UUID_SHIM_DIR)
  include_directories(BEFORE SYSTEM ${UUID_SHIM_DIR})
endif()

include_directories(SYSTEM
  ${CPP13_ROOT}/include/aarch64-linux-musleabi
  ${CPP13_ROOT}/include
)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${CPP13_ROOT}/lib ${CPP13_ROOT}/lib/libstdc++.so ${CPP13_ROOT}/lib/libgcc_s.so")

if(DEFINED XAPIAN_INCLUDE_DIR)
  include_directories(${XAPIAN_INCLUDE_DIR})
endif()

if(DEFINED TBB_INCLUDE_DIR)
  include_directories(${TBB_INCLUDE_DIR})
endif()
