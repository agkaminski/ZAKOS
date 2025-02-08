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

	if (!(file->attr & S_IX) || (file->size >= (0xffffu - (2 * PAGE_SIZE)))) {
		return -ENOEXEC;
	}

	off_t off = 0;
	size_t done = 0;

	while (done < file->size) {

	}
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

id_t process_start(void (*entry)(void *arg), uint8_t priority)
{
	struct process *process = process_create();
	if (process == NULL) {
		return -ENOMEM;
	}

	lock_lock(&common.plock);
	id_t pid = id_insert(&common.pid, &process->pid);
	if (pid < 0) {
		_process_put(process);
		lock_unlock(&common.plock);
		return pid;
	}
	lock_unlock(&common.plock);

	struct thread *thread = kmalloc(sizeof(*thread));
	int8_t err = -ENOMEM;
	if ((thread == NULL) || ((err = thread_create(thread, pid, priority, entry, NULL)) < 0)) {
		lock_lock(&common.plock);
		id_remove(&common.pid, &process->pid);
		_process_put(process);
		lock_unlock(&common.plock);
		kfree(thread);
		return (id_t)err;
	}

	return pid;
}

void process_init(void)
{
	id_init(&common.pid);
	lock_init(&common.plock);
}

