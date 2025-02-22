/* ZAK180 Zlibc
 * open.c
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#include <stdint.h>
#include "sys/syscall.h"

int open(const char *path, uint8_t mode, uint8_t attr)
{
	int ret = __sys_open(path, mode, attr);
	return ret;
}
