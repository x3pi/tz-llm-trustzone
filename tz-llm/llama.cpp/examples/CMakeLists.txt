# dependencies

find_package(Threads REQUIRED)

# third-party

# ...

# examples

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

if (EMSCRIPTEN)
else()
    add_subdirectory(cvector-generator)
    add_subdirectory(baby-llama)
    add_subdirectory(batched-bench)
    add_subdirectory(batched)
    add_subdirectory(benchmark)
    add_subdirectory(convert-llama2c-to-ggml)
    add_subdirectory(embedding)
    add_subdirectory(eval-callback)
    add_subdirectory(export-lora)
    add_subdirectory(gbnf-validator)
    # llama-gguf and llama-gguf-hash are the only two examples that link
    # `ggml` alone, without `llama` (see their own CMakeLists.txt) -- every
    # other target here links `llama`, which transitively pulls in
    # `libllama.so` and resolves it. This matters now because `libggml.so`
    # (ggml-rknpu-re.cpp's real-NPU-weight-buffer tzasc_cma path, under
    # GGML_USE_CHCORE) references push_pages()/commit_tzasc()/
    # tzasc_cma_meta_arr/cma_mtx, which are defined in llama.cpp/src
    # (alloc-stage-chcore.cpp/decrypt-stage.cpp), i.e. `libllama.so`, not
    # `libggml.so` itself -- a real cross-library reference that only
    # resolves for executables linking both libs together (`fake`,
    # `llama-cli`, everything else here). These two never did and are not
    # used anywhere in the TZ-LLM CA/TA pipeline (pure upstream GGUF
    # inspection utilities) -- skip them for the ChCore/TA build rather
    # than duplicating tzasc_cma glue into ggml/src just for their sake.
    if (NOT GGML_CHCORE)
        add_subdirectory(gguf-hash)
        add_subdirectory(gguf)
    endif()
    add_subdirectory(gguf-split)
    add_subdirectory(gritlm)
    add_subdirectory(imatrix)
    add_subdirectory(infill)
    add_subdirectory(llama-bench)
    add_subdirectory(llava)
    add_subdirectory(lookahead)
    add_subdirectory(lookup)
    add_subdirectory(main)
    add_subdirectory(parallel)
    add_subdirectory(passkey)
    add_subdirectory(perplexity)
    add_subdirectory(quantize-stats)
    add_subdirectory(quantize)
    add_subdirectory(retrieval)
    if (GGML_RPC)
        add_subdirectory(rpc)
    endif()
    if (LLAMA_BUILD_SERVER)
    add_subdirectory(server)
    endif()
    if (GGML_SYCL)
        add_subdirectory(sycl)
    endif()
    add_subdirectory(save-load-state)
    add_subdirectory(simple)
    add_subdirectory(speculative)
    add_subdirectory(tokenize)
endif()
