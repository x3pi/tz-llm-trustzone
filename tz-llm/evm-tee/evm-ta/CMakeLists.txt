cmake_minimum_required(VERSION 3.10)
project(EvmTA CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-fexceptions -Wno-narrowing -Wno-return-type)
string(REPLACE "-fno-exceptions" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")

# Include ChCore syscall headers
include_directories(/home/vectorxj/chcore/staros/.cpp/aarch64/include)
include_directories(/home/vectorxj/chcore/staros/.cpp/aarch64/include/aarch64-linux-musleabi)
include_directories(/home/vectorxj/chcore/staros/build/chcore-libc/install/include)

# EVM variables
set(MVM_ROOT ${CMAKE_CURRENT_LIST_DIR}/c_mvm)

include_directories(
    ${MVM_ROOT}/include
    ${MVM_ROOT}/3rdparty
    ${MVM_ROOT}/3rdparty/intx/include
    ${MVM_ROOT}/3rdparty/keccak
    ${MVM_ROOT}/3rdparty/fmt/include
    ${MVM_ROOT}/3rdparty/nlohmann
)

# EVM Sources
set(MVM_SRCS
    ${MVM_ROOT}/src/disassembler.cpp
    ${MVM_ROOT}/src/processor.cpp
    ${MVM_ROOT}/src/stack.cpp
    ${MVM_ROOT}/src/transaction.cpp
    ${MVM_ROOT}/src/log.cpp
    ${MVM_ROOT}/src/util.cpp
    ${MVM_ROOT}/src/gas.cpp
    ${MVM_ROOT}/src/crypto/sha256.cpp
    ${MVM_ROOT}/src/crypto/ripemd160.cpp
    ${MVM_ROOT}/src/crypto/bn254.cpp
    ${MVM_ROOT}/src/crypto/pairing/bn254/pairing.cpp
    ${MVM_ROOT}/src/crypto/blake2b.cpp
)

# Keccak Sources
file(GLOB KECCAK_SRCS ${MVM_ROOT}/3rdparty/keccak/*.c)

# Add linker sources
set(LINKER_ROOT ${CMAKE_CURRENT_LIST_DIR}/linker)
set(LINKER_SRCS
    ${LINKER_ROOT}/src/my_global_state.cpp
    ${LINKER_ROOT}/src/my_account.cpp
    ${LINKER_ROOT}/src/my_storage.cpp
    ${LINKER_ROOT}/src/state.cpp
    ${LINKER_ROOT}/src/my_extension/my_extension.cpp
    ${LINKER_ROOT}/src/my_extension/crypto_handlers.cpp
    ${LINKER_ROOT}/src/my_extension/xapian_handlers.cpp
    ${LINKER_ROOT}/src/xapian/xapian_manager.cpp
    ${LINKER_ROOT}/src/xapian/xapian_search.cpp
    ${LINKER_ROOT}/src/xapian/xapian_registry.cpp
    ${LINKER_ROOT}/src/xapian/xapian_log.cpp
    ${LINKER_ROOT}/src/xapian/xapian_manager_resolve.cpp
    ${LINKER_ROOT}/src/xapian/xapian_crypto.cpp
    ${LINKER_ROOT}/src/my_logger.cpp
    ${LINKER_ROOT}/src/abi/encoding_utils.cpp
    ${LINKER_ROOT}/src/my_extension/utils.cpp
    ${LINKER_ROOT}/src/my_extension/constants.cpp
    ${CMAKE_CURRENT_LIST_DIR}/go_mocks.cpp
)

add_executable(evm-ta main.cpp ${MVM_SRCS} ${KECCAK_SRCS} ${LINKER_SRCS})

target_include_directories(evm-ta PRIVATE
    ${MVM_ROOT}/include
    ${MVM_ROOT}/3rdparty/keccak
    ${MVM_ROOT}/src
    ${LINKER_ROOT}/include
    ${LINKER_ROOT}/include/xapian
    /home/vectorxj/xapian-tee/install/include
)

target_compile_definitions(evm-ta PRIVATE MVM_LINKER_BUILD)

target_link_libraries(evm-ta
    /home/vectorxj/xapian-tee/install/lib/libxapian.a
    /home/vectorxj/xapian-tee/install/lib/libz.a
)

# We do not link libsolidity.a for now, if it complains we can mock missing symbols.
