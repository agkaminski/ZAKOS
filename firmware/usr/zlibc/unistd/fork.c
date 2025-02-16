/* ZAK180 Zlibc
 * fork.c
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

pid_t fork(void)
{
	pid_t ret = __sys_fork();
	return ret;
}
