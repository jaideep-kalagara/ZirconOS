#!/bin/bash

make clean

make CROSS_PREFIX=/home/jaideep/opt/cross/bin/i686-elf- -j"$(nproc)"

if [ $? -ne 0 ]; then
    echo "Build failed"
    exit 1
fi