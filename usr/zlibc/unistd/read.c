/* ZAK180 Zlibc
 * read.c
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#include <stddef.h>
#include <stdint.h>
#include "sys/syscall.h"

int read(int8_t fd, void *buff, size_t bufflen)
{
	int ret = __sys_read(fd, buff, bufflen);
	return ret;
}
