#!/bin/bash
set -e

INSTALL_DIR=/home/vectorxj/xapian-tee/install
mkdir -p $INSTALL_DIR

CC=/home/vectorxj/chcore/staros/build/chcore-libc/bin/musl-gcc
CXX=/home/vectorxj/chcore/staros/build/chcore-libc/bin/musl-gcc
CFLAGS="-O3 -fPIC -fexceptions"

CPP_PREFIX="/home/vectorxj/chcore/staros/.cpp/aarch64"
CXXFLAGS="-O3 -fPIC -fexceptions -I$CPP_PREFIX/include/aarch64-linux-musleabi -I$CPP_PREFIX/include"
CPPFLAGS="-I$INSTALL_DIR/include"
LDFLAGS="-L$INSTALL_DIR/lib -lz -L$CPP_PREFIX/lib $CPP_PREFIX/lib/libstdc++.so $CPP_PREFIX/lib/libgcc_s.so"

echo "Building Xapian Core..."
cd /home/vectorxj/xapian-tee/src/xapian-core-1.4.25

# Configure Xapian for cross-compilation targeting TEE
CC=$CC CXX=$CXX CFLAGS=$CFLAGS CXXFLAGS=$CXXFLAGS CPPFLAGS=$CPPFLAGS LDFLAGS=$LDFLAGS \
./configure \
    --prefix=$INSTALL_DIR \
    --build=x86_64-linux-gnu \
    --host=aarch64-linux-gnu \
    --disable-shared \
    --enable-static \
    --disable-backend-remote \
    --disable-documentation \
    --without-libiconv \
    --with-zlib

make -j$(nproc)
make install
