/* ZAK180 Firmaware
 * Syscalls
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#include <stddef.h>
#include <stdint.h>

#include "proc/process.h"

/* Every syscall has to have uintptr_t as a first argument!
 * This is a placeholder for the user space return address. */

/* SDCC 4.2.0 bug: do not do return func() when func is __sdcccall(1)
 * from a function that is __sdcccall(0)! SDCC generates required
 * ABI translation, but sadly does jp _func instead of call _func,
 * so the code is never executed. */

extern int putchar(int c);
int syscall_putc(uintptr_t raddr, int c) __sdcccall(0)
{
	(void)raddr;
	int ret = putchar(c);
	return ret;
}


int syscall_fork(uintptr_t raddr) __sdcccall(0)
{
	int ret = process_fork();
	return ret;
}


void syscall_write(uintptr_t raddr, int fd, void *buff, size_t bufflen) __sdcccall(0)
{
	(void)raddr;
}
