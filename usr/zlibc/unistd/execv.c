/* ZAK180 Zlibc
 * execv.c
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#include <sys/syscall.h>
#include <stdint.h>
#include <unistd.h>

int8_t execv(const char *path, char *const argv[])
{
	int8_t ret = __sys_execv(path, argv);
	return ret;
}
