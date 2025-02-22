/* ZAK180 Zlibc
 * ftruncate.c
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>

int ftruncate(int8_t fd, off_t size)
{
	int ret = __sys_ftruncate(fd, size);
	return ret;
}
