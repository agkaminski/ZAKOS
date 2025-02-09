/* ZAK180 Firmaware
 * Processes
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#include <stdint.h>
#include <string.h>

#include "process.h"
#include "lock.h"
#include "thread.h"

#include "fs/fs.h"
#include "mem/page.h"
#include "mem/kmalloc.h"
#include "lib/errno.h"
#include "lib/assert.h"
#include "lib/strdup.h"
#include "lib/list.h"
#include "driver/mmu.h"

#define PROCESS_ENTRY_POINT ((void (*)(void *))0x1000)

static struct {
	struct id_storage pid;
	struct lock plock;
} common;

struct process *_process_get(id_t pid)
{
	struct process *p;

	assert(pid >= 0);

	p = id_get(&common.pid, pid, struct process, pid);
	if (p != NULL) {
		assert(p->refs < INT8_MAX);
		assert(p->refs >= 0);
		++p->refs;
	}
	return p;
}

void _process_put(struct process *process)
{
	assert(process->refs > 0);

	if (--process->refs <= 0) {
		kfree(process->path);
		page_free(process->mpage, PROCESS_PAGES);
		kfree(process);
	}
}

struct process *process_get(id_t pid)
{
	lock_lock(&common.plock);
	struct process *p = _process_get(pid);
	lock_unlock(&common.plock);
	return p;
}

void process_put(struct process *process)
{
	lock_lock(&common.plock);
	_process_put(process);
	lock_unlock(&common.plock);
}

static int8_t process_load(struct process *process, const char *path)
{
	struct fs_file *file;
	int8_t err = fs_open(path, &file, O_RDONLY, 0);
	if (err != 0) {
		return err;
	}

	if (!(file->attr & S_IX) || (file->size > (PROCESS_PAGES * PAGE_SIZE))) {
		return -ENOEXEC;
	}

	off_t off = 0;
	uint8_t page = process->mpage;
	uint8_t prev_page;
	uint8_t *dest = mmu_map_scratch(page, &prev_page);

	while (1) {
		size_t chunk = PAGE_SIZE;
		while (chunk) {
			int len = fs_read(file, dest, PAGE_SIZE, off);
			if (len <= 0) { /* We do not expect EOF here */
				err = len;
				break;
			}
			off += len;
			chunk -= len;
		}

		if (off == file->size || err) {
			break;
		}

		dest = mmu_map_scratch(++page, NULL);
	}

	(void)fs_close(file);
	(void)mmu_map_scratch(prev_page, NULL);

	return err;
}

int8_t process_execve(const char *path, const char *argv, const char *envp)
{
	/* TODO */
	return -ENOSYS;
}

struct process *process_create(void)
{
	struct process *p = kmalloc(sizeof(*p));
	if (p != NULL) {
		memset(p, 0, sizeof(*p));
		p->mpage = page_alloc(p, PROCESS_PAGES);
		if (!p->mpage) {
			kfree(p);
			return NULL;
		}

		lock_init(&p->lock);

		p->refs = 1;

		/* TODO fd table etc */
	}
	return p;
}

id_t process_vfork(void)
{
	/* TODO */
	return -ENOSYS;
}

id_t process_start(const char *path, char *argv)
{
	struct process *process = process_create();
	if (process == NULL) {
		return -ENOMEM;
	}

	process->path = strdup(path);
	if (process->path == NULL) {
		process_put(process);
		return -ENOMEM;
	}

	int8_t err = process_load(process, path);
	if (err < 0) {
		process_put(process);
		return err;
	}

	struct thread *thread = kmalloc(sizeof(*thread));
	if (thread == NULL) {
		process_put(process);
		return -ENOMEM;
	}

	lock_lock(&common.plock);
	err = id_insert(&common.pid, &process->pid);
	if (err < 0) {
		_process_put(process);
		lock_unlock(&common.plock);
		kfree(thread);
		return err;
	}
	lock_unlock(&common.plock);

	assert((process->pid.id >= ID_MIN) && (process->pid.id <= ID_MAX));

	/* TODO prepare stack: argv, exit point etc */

	err = thread_create(thread, process->pid.id, THREAD_PRIORITY_DEFAULT, PROCESS_ENTRY_POINT, NULL);
	if (err != 0) {
		lock_lock(&common.plock);
		id_remove(&common.pid, &process->pid);
		_process_put(process);
		lock_unlock(&common.plock);
		kfree(thread);
		return err;
	}

	return process->pid.id;
}

void _process_zombify(struct process *process)
{
	/* Init can't die! */
	assert(process->parent != NULL);
	LIST_REMOVE(&process->parent->children, process, struct process, next, prev);
	LIST_ADD(&process->parent->zombies, process, struct process, next, prev);
	_thread_signal(&process->wait);
}

void process_end(struct process *process)
{
	struct thread *curr = thread_current();

	if (process == NULL) {
		process = curr->process;
	}

	lock_lock(&process->lock);
	struct thread *it = id_get_first(&process->threads, struct thread, id);
	while (it != NULL) {
		struct thread *next = id_get_next(it, struct thread, id);
		if (curr != it) {
			thread_end(it);
		}
		it = next;
	}
	lock_unlock(&process->lock);

	if (curr->process == process) {
		thread_end(NULL);
	}
}

void process_init(void)
{
	id_init(&common.pid);
	lock_init(&common.plock);
}

