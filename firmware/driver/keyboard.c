/* ZAK180 Firmaware
 * Keyboard driver
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#include <z180/z180.h>
#include <stdint.h>

#include "keyboard.h"

__sfr __at(0x0080) COL0;
__sfr __at(0x0081) COL1;
__sfr __at(0x0082) COL2;
__sfr __at(0x0083) COL3;
__sfr __at(0x0084) COL4;
__sfr __at(0x0085) COL5;
__sfr __at(0x0086) COL6;
__sfr __at(0x0087) COL7;
__sfr __at(0x0088) COL8;
__sfr __at(0x0089) COL9;
__sfr __at(0x008A) COL10;
__sfr __at(0x008B) COL11;
__sfr __at(0x008C) COL12;
__sfr __at(0x008D) COL13;
__sfr __at(0x008E) COL14;
__sfr __at(0x008F) COL15;

