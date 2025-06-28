#!/bin/bash

set -e

JOBS=24
OP=
CLEAN=

if [ "$1" = "clean" ]; then CLEAN="make clean"; shift; fi
if [ "$1" = "all" ]; then OP="make -j$JOBS all"; shift; fi

(cd driver && $CLEAN && $OP)
(cd bootloader && $CLEAN && $OP)
(cd kernel && $CLEAN && $OP)
(cd usr/zlibc && $CLEAN && $OP)
(cd usr/init && $CLEAN && $OP)
(cd usr/hello && $CLEAN && $OP)
(cd usr/zesh && $CLEAN && $OP)
