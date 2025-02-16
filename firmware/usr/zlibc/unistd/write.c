/* ZAK180 Zlibc
 * write.c
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#include "sys/syscall.h"

int write(int fs, void *buff, size_t bufflen)
{
	int ret = __sys_write(fs, buff, bufflen);
	return ret;
}
