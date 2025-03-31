/* ZAK180 Zlibc
 * fcntl
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#ifndef __ZLIBC_FCNTL_H_
#define __ZLIBC_FCNTL_H_

#include <stdint.h>
#include <bits/fcntl.h>

int open(const char *path, uint8_t mode, uint8_t attr);

#endif
