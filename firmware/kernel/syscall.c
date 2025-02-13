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
#include "proc/process.h"
#include "proc/thread.h"

static void *syscall_user_access(void *ptr, size_t *offs, uint8_t *prev)
{
	assert((uintptr_t)ptr >= PROCESS_MEM_START);

	struct process *curr = thread_current()->process;
	uint8_t page = curr->mpage + ((uintptr_t)ptr - PROCESS_MEM_START) / PAGE_SIZE;
	*offs = (uintptr_t)ptr % PAGE_SIZE;

	return mmu_map_scratch(page, prev);
}

static void syscall_get_from_user(void *dst, const void *src, size_t len)
{
	uint8_t prev;
	size_t offs;

	while (len) {
		uint8_t *buff = syscall_user_access(src, &offs, &prev);
		size_t chunk = len;
		if (chunk > (PAGE_SIZE - offs)) {
			chunk = PAGE_SIZE - offs;
		}
		memcpy(dst, buff + offs, chunk);

		len -= chunk;
		src = (const uint8_t *)src + chunk;
 	}
	mmu_map_scratch(prev, NULL);
}

static void syscall_set_to_user(void *dst, const void *src, size_t len)
{
	uint8_t prev;
	size_t offs;

	while (len) {
		uint8_t *buff = syscall_user_access(dst, &offs, &prev);
		size_t chunk = len;
		if (chunk > (PAGE_SIZE - offs)) {
			chunk = PAGE_SIZE - offs;
		}
		memcpy(buff + offs, src, chunk);

		len -= chunk;
		dst = (const uint8_t *)dst + chunk;
 	}
	mmu_map_scratch(prev, NULL);
}

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
	kprintf("%c", c);
	return 1;
}

int syscall_fork(uintptr_t raddr) __sdcccall(0)
{
	int ret = process_fork();
	return ret;
}

int syscall_waitpid(id_t pid, int8_t *status, ktime_t timeout) __sdcccall(0)
{
	int8_t kstatus;

	int ret = process_wait(pid, &kstatus, timeout);
	if (status != NULL) {
		syscall_set_to_user(status, &kstatus, sizeof(*status));
	}
	return ret;
}

void syscall_write(uintptr_t raddr, int fd, void *buff, size_t bufflen) __sdcccall(0)
{
	(void)raddr;
}
