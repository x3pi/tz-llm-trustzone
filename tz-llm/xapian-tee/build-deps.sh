#!/bin/bash
set -e

INSTALL_DIR=/home/vectorxj/xapian-tee/install
mkdir -p $INSTALL_DIR

CC=/home/vectorxj/chcore/staros/build/chcore-libc/bin/musl-gcc
CXX=/home/vectorxj/chcore/staros/build/chcore-libc/bin/musl-g++
CFLAGS="-O3 -fPIC -fexceptions"

echo "Building zlib..."
cd /home/vectorxj/xapian-tee/src/zlib-1.3.1
CC=$CC CFLAGS=$CFLAGS ./configure --prefix=$INSTALL_DIR --static
make -j$(nproc)
make install

# Skipped libuuid as util-linux configure hangs on cross-compiling.
# Xapian has a built-in fallback for UUID generation if libuuid is missing.
