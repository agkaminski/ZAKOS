/* ZAK180 Firmaware
 * Kernel printf
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#ifndef KERNEL_LIB_KPRINTF_H_
#define KERNEL_LIB_KPRINTF_H_

#include <stdint.h>

void kprintf_use_irq(int8_t use);

void kprintf(const char *fmt, ...);

void ksprintf(char *buff, const char *fmt, ...);

#endif
