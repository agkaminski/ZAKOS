/* ZAK180 Zlibc
 * write.c
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#include <stddef.h>
#include <stdint.h>
#include "sys/syscall.h"

int write(int8_t fd, const void *buff, size_t bufflen)
{
	int ret = __sys_write(fd, buff, bufflen);
	return ret;
}
