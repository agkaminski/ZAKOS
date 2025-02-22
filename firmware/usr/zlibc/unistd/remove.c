/* ZAK180 Zlibc
 * remove.c
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#include <sys/syscall.h>
#include <unistd.h>

int remove(const char *path)
{
	int ret = __sys_remove(path);
	return ret;
}
