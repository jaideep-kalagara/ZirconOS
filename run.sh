#!/bin/bash

make clean

make CROSS_PREFIX=/home/jaideep/opt/cross/bin/i686-elf- -j"$(nproc)"

make run

if [ $? -ne 0 ]; then
    echo "Build failed"
    exit 1
fi

if [ $1 -eq 1 ]; then
    make clean
fi