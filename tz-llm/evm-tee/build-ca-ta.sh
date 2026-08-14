#!/bin/bash
set -e

echo "Building EVM TA (for TEE)..."
cd /home/vectorxj/evm-tee/evm-ta
rm -rf build_tee
mkdir -p build_tee
cd build_tee
cmake -DCMAKE_TOOLCHAIN_FILE=/home/vectorxj/chcore/staros/build/toolchain.cmake ..
make -j$(nproc)

echo "Building EVM CA (for Linux)..."
cd /home/vectorxj/evm-tee/evm-ca
aarch64-linux-gnu-g++ -O3 -Wall -Wextra -static main.cpp -o evm-ca

echo "Copying outputs to checkpoints..."
mkdir -p /home/vectorxj/evm-tee/checkpoints
cp /home/vectorxj/evm-tee/evm-ca/evm-ca /home/vectorxj/evm-tee/checkpoints/
cp /home/vectorxj/evm-tee/evm-ta/build_tee/evm-ta /home/vectorxj/evm-tee/checkpoints/

echo "Done!"
