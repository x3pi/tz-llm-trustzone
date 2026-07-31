# TODO: should not use this
if (WIN32)
    if (BUILD_SHARED_LIBS)
        set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    endif()
endif()

#
# libraries
#

# llama

list(APPEND LLAMA_SOURCE_FILES
    ../include/llama.h
    llama.cpp
    llama-vocab.cpp
    llama-grammar.cpp
    llama-sampling.cpp
    unicode.h
    unicode.cpp
    unicode-data.cpp
    prefetch.h
    prefetch.cpp
    io.h
    io-frontend.h
    io-frontend.cpp
    pipeline.h
    pipeline.cpp
    layer-sched.cpp
    io-stage.cpp
    decrypt-stage.cpp
)

if (LLAMA_CHCORE_API)
    list(APPEND LLAMA_SOURCE_FILES alloc-stage-chcore.cpp)
else()
    list(APPEND LLAMA_SOURCE_FILES alloc-stage.cpp io-backend.cpp)
endif()

add_library(llama ${LLAMA_SOURCE_FILES})

target_include_directories(llama PUBLIC . ../include)
target_compile_features   (llama PUBLIC cxx_std_17) # don't bump

target_link_libraries(llama PUBLIC ggml)

# find_package(OpenSSL REQUIRED)
# target_include_directories(llama PRIVATE ${OPENSSL_INCLUDE_DIR})
# target_link_libraries(llama PRIVATE OpenSSL::SSL OpenSSL::Crypto)
# target_link_libraries(llama PUBLIC aio)

list(APPEND LLAMA_INCLUDE_DIR
    /home/vectorxj/openharmony/third_party/openssl/include
    /home/vectorxj/chcore/opentrustee_llm/libaio-0.3.113/install/include
)
list(APPEND LLAMA_LINK_DIR
)

if (LLAMA_CHCORE_API)
list(APPEND LLAMA_INCLUDE_DIR
    /home/vectorxj/openharmony/base/tee/tee_os_kernel/user/chcore-libc/musl-libc/include
)
list(APPEND LLAMA_LINK_DIR
    /home/vectorxj/openssl/openssl-3.4.1/install/lib/libcrypto.so.3
    /home/vectorxj/openssl/openssl-3.4.1/install/lib/libssl.so.3
)
else()
list(APPEND LLAMA_INCLUDE_DIR
    /home/vectorxj/chcore/opentrustee_llm/rknn-toolkit2-2.3.0/rknpu2/runtime/Linux/librknn_api/include
    /home/vectorxj/chcore/opentrustee_llm/libaio-0.3.113/install-glibc/include
)
list(APPEND LLAMA_LINK_DIR
    /home/vectorxj/chcore/opentrustee_llm/rknn-toolkit2-2.3.0/rknpu2/runtime/Linux/librknn_api/aarch64/librknnrt.so
    /home/vectorxj/openharmony/third_party/openssl/install-glibc/lib/libcrypto.so.3
    /home/vectorxj/openharmony/third_party/openssl/install-glibc/lib/libssl.so.3
    /home/vectorxj/chcore/opentrustee_llm/libaio-0.3.113/install-glibc/lib/libaio.so.1.0.2
)
endif()

target_include_directories(llama PUBLIC ${LLAMA_INCLUDE_DIR})
target_link_libraries(llama PUBLIC ${LLAMA_LINK_DIR})

if (NOT LLAMA_CHCORE_API)
# BUG FIX: io-backend.cpp was compiled into BOTH this library and `llama`
# (see the `list(APPEND LLAMA_SOURCE_FILES alloc-stage.cpp io-backend.cpp)`
# above) -- since io_init()/io_step()/get_ctx()/put_ctx() are ordinary
# (non-static) functions, each shared library ended up exporting its own
# duplicate definition with default ELF visibility. With both libllama.so
# and libremoting_backend.so loaded into the same `fake` process, the
# dynamic linker's symbol interposition picks ONE winning definition for
# *every* call site process-wide, regardless of which library's code
# thinks it's calling its own copy -- an inconsistent, effectively
# undefined mix of which library's static state (fd/tzd_fd/mutexes,
# themselves correctly kept file-local via `static`) actually backs a
# given call. This is the exact same class of bug already found and
# documented for `is_strawman` (see prefetch.cpp's comment) -- confirmed
# here via `nm -D`, which shows `io_step` exported as a global defined
# symbol (`T`) in both .so's. Symptom on hardware: an early, otherwise
# inexplicable `glibc pthread_mutex_lock: Assertion 'mutex->__data.__owner
# == 0' failed` right as the 4 ca_thread pthreads start concurrently
# calling into this code for the first time. Fix: don't recompile
# io-backend.cpp a second time here. ca-backend.cpp's `extern void
# io_init/io_step(...)` are left genuinely undefined in this .so on
# purpose -- shared libraries allow undefined symbols by default (they're
# expected to be resolved at process-load time against whatever else is
# already loaded), and since `fake`/`llama-cli` already link `llama`
# directly, the single copy built there satisfies these at runtime with
# no explicit link-time dependency needed here. (Tried linking against
# `llama` directly first -- works, but also transitively pulls in
# llama's libcrypto.so.3/libssl.so.3 link deps as new hard DT_NEEDED
# entries on this .so, which aren't present on the board's
# /data/ssd/rknpu/ and were never needed before since llama's own copy
# apparently gets --as-needed-dropped. Leaving the symbols undefined
# here avoids introducing that dependency at all.)
add_library(
    remoting_backend
    interface.h
    io.h
    ca-backend.cpp
)
target_compile_features(remoting_backend PUBLIC cxx_std_17)
target_include_directories(
    remoting_backend
    PUBLIC
    /home/vectorxj/chcore/opentrustee_llm/libaio-0.3.113/install-glibc/include
    /home/vectorxj/chcore/opentrustee_llm/rknn-toolkit2-2.3.0/rknpu2/runtime/Linux/librknn_api/include
)
target_link_libraries(
    remoting_backend
    PUBLIC
    /home/vectorxj/chcore/opentrustee_llm/libaio-0.3.113/install-glibc/lib/libaio.so.1.0.2
)
endif()

if (BUILD_SHARED_LIBS)
    set_target_properties(llama PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_compile_definitions(llama PRIVATE LLAMA_SHARED LLAMA_BUILD)
endif()
