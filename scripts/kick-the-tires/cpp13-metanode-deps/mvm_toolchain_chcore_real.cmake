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

if(NOT DEFINED CPP13_ROOT)
  set(CPP13_ROOT /tmp/cpp13/aarch64)
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
