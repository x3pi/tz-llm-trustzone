#!/bin/bash
cd /home/vectorxj/chcore/opentrustee_llm/llama.cpp
mkdir -p build-chcore
cd build-chcore
rm -rf CMakeCache.txt CMakeFiles
cmake -DCMAKE_TOOLCHAIN_FILE=/home/vectorxj/chcore/staros/build/toolchain.cmake -DGGML_NATIVE=OFF -DGGML_OPENMP=OFF -DLLAMA_BUILD_TESTS=OFF -DGGML_RKNPU2=OFF -DGGML_RKNPURE=ON ..
make -j4
