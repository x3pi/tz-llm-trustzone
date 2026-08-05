#!/bin/bash
set -e

SHARE_DIR=/home/vectorxj/share

rknpure_upload() {
    mkdir -p $SHARE_DIR/build-rknpure
    BUILD_RKNPURE_DIR=$SHARE_DIR/build-rknpure
    cp ggml/src/libggml.so $BUILD_RKNPURE_DIR/
    cp src/libllama.so $BUILD_RKNPURE_DIR/
    cp src/libremoting_backend.so $BUILD_RKNPURE_DIR/
    cp bin/llama-cli $BUILD_RKNPURE_DIR/
    cp bin/fake $BUILD_RKNPURE_DIR/
}

cd /home/vectorxj/chcore/opentrustee_llm/llama.cpp

rm -rf build-rknpure
mkdir -p build-rknpure
cmake -DCMAKE_C_COMPILER=/usr/bin/aarch64-linux-gnu-gcc -DCMAKE_CXX_COMPILER=/usr/bin/aarch64-linux-gnu-g++ -DGGML_NATIVE=OFF -DGGML_OPENMP=OFF -DLLAMA_BUILD_TESTS=OFF -DLLAMA_CHCORE_API=OFF -DGGML_CHCORE=OFF -B build-rknpure

pushd build-rknpure
make -j$(nproc)
rknpure_upload
popd
