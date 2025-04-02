/* ZAK180 Zlibc
 * dup.c
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#include <sys/syscall.h>
#include <unistd.h>
#include <stdint.h>

int dup(int8_t oldfd)
{
	int ret = __sys_dup2(oldfd, -1);
	return ret;
}

int dup2(int8_t oldfd, int8_t newfd)
{
	int ret = __sys_dup2(oldfd, newfd);
	return ret;
}
