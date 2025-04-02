/* ZAK180 Firmaware
 * Syscalls
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#include <stddef.h>
#include <string.h>
#include <stdint.h>

#include "driver/mmu.h"
#include "lib/assert.h"
#include "lib/kprintf.h"
#include "lib/errno.h"
#include "proc/process.h"
#include "proc/thread.h"
#include "proc/file.h"

/* Every syscall has to have uintptr_t as a first argument!
 * This is a placeholder for the user space return address. */

/* SDCC 4.2.0 bug: do not do return func() when func is __sdcccall(1)
 * from a function that is __sdcccall(0)! SDCC generates required
 * ABI translation, but sadly does jp _func instead of call _func,
 * so the code is never executed. */

int syscall_fork(uintptr_t raddr) __sdcccall(0)
{
	(void)raddr;
	int ret = process_fork();
	return ret;
}

int syscall_waitpid(uintptr_t raddr, id_t pid, int *status, int8_t options) __sdcccall(0)
{
	(void)raddr;
	id_t ret = process_wait(pid, status, options);
	return ret;
}

void syscall_process_end(uintptr_t raddr, int exit) __sdcccall(0)
{
	(void)raddr;
	struct process *curr = thread_current()->process;
	process_end(curr, exit);
}

int syscall_msleep(uintptr_t raddr, uint32_t mseconds) __sdcccall(0)
{
	(void)raddr;
	int ret = thread_sleep_relative(mseconds);
	return ret;
}

int syscall_execv(uintptr_t raddr, const char *path, char *const argv[]) __sdcccall(0)
{
	(void)raddr;
	int ret = process_execv(path, argv);
	return ret;
}

int syscall_open(uintptr_t raddr, const char *path, uint8_t mode, uint8_t attr) __sdcccall(0)
{
	(void)raddr;
	int ret = file_open(path, mode, attr);
	return ret;
}

int syscall_close(uintptr_t raddr, int8_t fd) __sdcccall(0)
{
	(void)raddr;
	int ret = file_close(fd);
	return ret;
}

int syscall_read(uintptr_t raddr, int8_t fd, void *buff, size_t bufflen) __sdcccall(0)
{
	(void)raddr;
	int ret = file_read(fd, buff, bufflen);
	return ret;
}

int syscall_write(uintptr_t raddr, int8_t fd, const void *buff, size_t bufflen) __sdcccall(0)
{
	(void)raddr;

	if (fd == 1) {
		/* TODO for now putc mock */
		kprintf("%c", *((const char *)buff));
		return 1;
	}

	int ret = file_write(fd, buff, bufflen);
	return ret;
}

int syscall_truncate(uintptr_t raddr, const char *path, off_t size) __sdcccall(0)
{
	(void)raddr;
	int ret = file_truncate(path, size);
	return ret;
}

int syscall_ftruncate(uintptr_t raddr, int8_t fd, off_t size) __sdcccall(0)
{
	(void)raddr;
	int ret = file_ftruncate(fd, size);
	return ret;
}

int syscall_readdir(uintptr_t raddr, int8_t dir, struct fs_dentry *dentry, uint16_t idx) __sdcccall(0)
{
	(void)raddr;
	int ret = file_readdir(dir, dentry, idx);
	return ret;
}

int syscall_remove(uintptr_t raddr, const char *path) __sdcccall(0)
{
	(void)raddr;
	int ret = file_remove(path);
	return ret;
}

int syscall_dup2(uintptr_t raddr, int8_t oldfd, int8_t newfd) __sdcccall(0)
{
	(void)raddr;
	int ret = file_dup2(oldfd, newfd);
	return ret;
}
