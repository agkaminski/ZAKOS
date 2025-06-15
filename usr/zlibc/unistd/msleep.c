/* ZAK180 Zlibc
 * usleep.c
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>

int8_t msleep(mseconds_t time)
{
	int8_t ret = __sys_msleep(time);
	return ret;
}
