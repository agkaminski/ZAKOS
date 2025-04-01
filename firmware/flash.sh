#!/bin/bash
# arg - path to FDD mountpoint
set -e

./build.sh all
cp kernel/kernel.bin $1/BOOT/KERNEL.IMG
cp usr/hello/hello.bin $1/BIN/HELLO.ZEX
mkdir -p $1/DEV
sync
sudo umount $1
sync

echo "done"
