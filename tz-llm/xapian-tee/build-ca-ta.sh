#!/bin/bash
set -e

echo "Building Xapian TA (for TEE)..."
cd /home/vectorxj/xapian-tee/xapian-ta
mkdir -p build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=/home/vectorxj/chcore/staros/build/toolchain.cmake ..
make -j$(nproc)

echo "Building Xapian CA (for Linux)..."
cd /home/vectorxj/xapian-tee/xapian-ca
aarch64-linux-gnu-g++ -O3 -Wall -Wextra -static main.cpp -o xapian-ca

echo "Copying outputs to checkpoints..."
mkdir -p /home/vectorxj/xapian-tee/checkpoints
cp /home/vectorxj/xapian-tee/xapian-ca/xapian-ca /home/vectorxj/xapian-tee/checkpoints/
cp /home/vectorxj/xapian-tee/xapian-ta/build/xapian-ta /home/vectorxj/xapian-tee/checkpoints/

echo "Done!"
