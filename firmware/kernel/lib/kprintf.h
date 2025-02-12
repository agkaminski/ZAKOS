/* ZAK180 Firmaware
 * Kernel printf
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#ifndef KERNEL_LIB_KPRINTF_H_
#define KERNEL_LIB_KPRINTF_H_

#include <stdio.h>

#define kprintf(fmt, ...) printf(fmt, ##__VA_ARGS__)

#endif
