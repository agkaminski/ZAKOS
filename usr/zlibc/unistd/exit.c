/* ZAK180 Zlibc
 * exit.c
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#include <sys/syscall.h>
#include <unistd.h>

void _exit(int exit)
{
	__sys_exit(exit);
}
