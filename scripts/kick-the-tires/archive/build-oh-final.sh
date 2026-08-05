#!/bin/bash
set -e

CURRENT_DIR=$(realpath $(dirname $0))
ROOT_DIR=$(realpath $CURRENT_DIR/../..)
TZ_LLM_DIR=$ROOT_DIR/tz-llm
SHARE_DIR=$CURRENT_DIR/share_fixed_20260725_final

mkdir -p $SHARE_DIR
cp $CURRENT_DIR/build-oh-docker.sh $SHARE_DIR/

$CURRENT_DIR/oh-builder-hdf.sh $SHARE_DIR bash -c ./build-oh-docker.sh
