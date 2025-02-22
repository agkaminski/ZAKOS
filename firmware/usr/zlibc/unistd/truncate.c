/* ZAK180 Zlibc
 * truncate.c
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

int truncate(const char *path, off_t size)
{
	int ret = __sys_truncate(path, size);
	return ret;
}
