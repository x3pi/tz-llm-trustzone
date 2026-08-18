#!/bin/bash
set -e

CURRENT_DIR=$(realpath $(dirname $0))
ROOT_DIR=$(realpath $CURRENT_DIR/../..)
TZ_LLM_DIR=$ROOT_DIR/tz-llm

echo "CMD: $@"

docker run --rm \
    -v $TZ_LLM_DIR/tee_os_kernel:/home/vectorxj/openharmony/base/tee/tee_os_kernel \
    -v $TZ_LLM_DIR/evm-tee:/home/vectorxj/evm-tee \
    -v $TZ_LLM_DIR/xapian-tee:/home/vectorxj/xapian-tee \
    -v /home/abc/nhat/con-chain-v2:/home/abc/nhat/con-chain-v2 \
    -w /home/vectorxj/evm-tee \
    vectorxj0553/tz-llm-llama-builder:latest \
    $@
