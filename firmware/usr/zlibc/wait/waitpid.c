/* ZAK180 Zlibc
 * wait.c
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>

pid_t waitpid(pid_t pid, int *status, int8_t options)
{
	pid_t ret = __sys_waitpid(pid, status, options);
	return ret;
}
