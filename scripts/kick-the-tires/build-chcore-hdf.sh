#!/bin/bash
set -e
CURRENT_DIR=$(realpath $(dirname $0))
SHARE_DIR=$CURRENT_DIR/share_hdf
mkdir -p $SHARE_DIR
cp $CURRENT_DIR/build-chcore-docker.sh $SHARE_DIR/
$CURRENT_DIR/oh-builder-hdf.sh $SHARE_DIR bash -c ./build-chcore-docker.sh
