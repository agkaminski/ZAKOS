#!/bin/bash
# arg - path to FDD mountpoint
set -e

ROOTFS_SKEL=rootfs

./build.sh all
cp kernel/kernel.bin $ROOTFS_SKEL/BOOT/KERNEL.IMG
cp usr/init/init.bin $ROOTFS_SKEL/BOOT/INIT.ZEX
cp usr/hello/hello.bin $ROOTFS_SKEL/BIN/HELLO.ZEX
cp usr/zesh/zesh.bin $ROOTFS_SKEL/BIN/ZESH.ZEX
(cd $ROOTFS_SKEL && rsync -av --exclude=".*" * $1)
sync
sudo umount $1
sync

echo "done"
