/* ZAK180 Zlibc
 * close.c
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#include <sys/syscall.h>
#include <unistd.h>
#include <stdint.h>

int close(int8_t fd)
{
	int ret = __sys_close(fd);
	return ret;
}
