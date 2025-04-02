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
#include "lib/kprintf.h"
#include "lib/panic.h"

#include "driver/mmu.h"
#include "driver/dma.h"

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

static int8_t process_load(uint8_t mpage, const char *path)
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
	uint8_t page = mpage;
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

extern void _thread_jmp(uint8_t nstack, uint8_t ostack, void *sp);

static int8_t process_do_exec(struct process *process, uint8_t mmap, char *const argv[])
{
	/* Assume process is prepared for execution,
	 * i.e. it's created, we're executing its
	 * main thread, memory map is allocated and
	 * process has been loaded */

	struct thread *current = thread_current();
	uint8_t nstack = page_alloc(process, 1), ostack = current->stack_page;
	if (!nstack) {
		return -ENOMEM;
	}

	if (process->mpage != mmap) {
		uint8_t ompage = process->mpage;
		process->mpage = mmap;
		page_free(ompage, PROCESS_PAGES);
	}

	int argc = 0;
	char **s_argv = NULL;
	uint8_t *stack = mmu_map_scratch(nstack, NULL);
	stack += PAGE_SIZE;

	if (argv != NULL) {
		/* Count arguments */
		while (argv[argc] != NULL) ++argc;

		/* Allocated argv table on the stack */
		stack -= ((argc + 1) * sizeof(char *));
		s_argv = stack;

		for (int i = 0; i < argc; ++i) {
			const char *arg = argv[i];
			size_t len = strlen(arg) + 1;

			/* Put (relocated to 0xF000) argument on the stack */
			stack -= len;
			memcpy(stack, arg, len);
			s_argv[i] = stack + PAGE_SIZE;
		}

		/* argv termination */
		s_argv[argc] = NULL;

		/* Relocate the pointer to the stack space */
		s_argv = (uint8_t *)s_argv + PAGE_SIZE;
	}

	/* Put main() argument on the stack */
	stack -= sizeof(s_argv);
	memcpy(stack, &s_argv, sizeof(s_argv));
	stack -= sizeof(argc);
	memcpy(stack, &argc, sizeof(argc));

	/* Relocate the sp to the stack space */
	stack += PAGE_SIZE;

	_DI;
	mmu_map_user(mmap);
	current->stack_page = nstack;
	_thread_jmp(nstack, ostack, stack);

	/* Not reached */
	return 0;
}

int8_t process_execv(const char *path, char *const argv[])
{
	struct process *current = thread_current()->process;
	assert(current != NULL);

	uint8_t nmap = page_alloc(current, PROCESS_PAGES);
	if (!nmap) {
		return -ENOMEM;
	}

	int8_t err = process_load(nmap, path);
	if (err) {
		page_free(nmap, PROCESS_PAGES);
		return err;
	}

	return process_do_exec(current, nmap, argv);
}

static struct process *process_create(void)
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
	}
	return p;
}

struct fork_data {
	struct thread *queue;
	enum { fork_not_ready, fork_forking, fork_fail, fork_done } state;
	struct thread *tparent;
	uint8_t old_stack;
};

extern void _thread_longjmp(uint8_t stack, struct cpu_context *context);

static void process_fork_thread(void *arg)
{
	struct fork_data *fdata = (struct fork_data *)arg;
	struct thread *current = thread_current();

	/* Wait for parent to be ready */
	thread_critical_start();
	while (fdata->state == fork_not_ready) {
		_thread_wait(&fdata->queue, 0);
	}
	thread_critical_end();

	/* Alloc new stack page */
	uint8_t nstack = page_alloc(current->process, 1);

	thread_critical_start();
	if (!nstack) {
		fdata->state = fork_fail;
		_thread_signal(&fdata->queue);
		_thread_end(NULL);
	}

	/* Copy parent stack */
	dma_memcpy(nstack, 0, fdata->tparent->stack_page, 0, PAGE_SIZE);

	/* We got parent's stack, it's free to go now */
	fdata->state = fork_done;
	_thread_signal(&fdata->queue);

	/* Assign new stack */
	_DI;
	fdata->old_stack = current->stack_page;
	current->stack_page = nstack;

	/* Go to the parent suspension point */
	_thread_longjmp(nstack, fdata->tparent->context);
}

id_t process_fork(void)
{
	id_t pid;
	struct fork_data *fdata = kmalloc(sizeof(*fdata));
	if (fdata == NULL) {
		return -ENOMEM;
	}

	struct thread *tparent = thread_current();

	fdata->queue = NULL;
	fdata->state = fork_not_ready;
	fdata->tparent = tparent;
	fdata->old_stack = 0;

	struct process *parent = tparent->process;
	assert(parent != NULL);
	struct process *spawn = process_create();
	if (spawn == NULL) {
		return -ENOMEM;
	}

	/* Copy process */
	spawn->path = strdup(parent->path);
	if (spawn->path == NULL) {
		process_put(spawn);
		kfree(fdata);
		return -ENOMEM;
	}

	/* Copy file descriptor table */
	file_fdtable_copy(parent, spawn);

	/* Copy whole parent memory */
	dma_memcpy(spawn->mpage, 0, parent->mpage, 0, PROCESS_PAGES * PAGE_SIZE);

	/* Establish parent-child relation */
	thread_critical_start();
	spawn->parent = parent;
	LIST_ADD(&parent->children, spawn, struct process, next, prev);
	thread_critical_end();

	struct thread *thread = kmalloc(sizeof(*thread));
	if (thread == NULL) {
		file_close_all(spawn);
		process_put(spawn);
		kfree(fdata);
		return -ENOMEM;
	}

	/* Make this official */
	lock_lock(&common.plock);
	int8_t err = id_insert(&common.pid, &spawn->pid);
	if (err < 0) {
		lock_unlock(&common.plock);
		process_put(spawn);
		file_close_all(spawn);
		kfree(thread);
		kfree(fdata);
		return err;
	}
	pid = spawn->pid.id;
	lock_unlock(&common.plock);

	err = thread_create(thread, spawn->pid.id, THREAD_PRIORITY_DEFAULT, process_fork_thread, (void *)fdata);
	if (err != 0) {
		lock_lock(&common.plock);
		id_remove(&common.pid, &spawn->pid);
		lock_unlock(&common.plock);
		file_close_all(spawn);
		process_put(spawn);
		kfree(thread);
		kfree(fdata);
		return err;
	}

	int8_t ischild = 0;

	thread_critical_start();
	fdata->state = fork_forking;
	_thread_signal(&fdata->queue);
	while (fdata->state == fork_forking) {
		ischild = _thread_wait(&fdata->queue, 0);
	}
	thread_critical_end();

	if (ischild) {
		/* Free old stack */
		page_free(fdata->old_stack, 1);

		/* Wakeup parent */
		thread_critical_start();
		_thread_signal(&fdata->queue);
		thread_critical_end();

		return 0;
	}

	id_t result = (fdata->state == fork_done) ? pid : -ENOMEM;

	kfree(fdata);

	return result;
}

void process_start_thread(void *arg)
{
	struct process *process = thread_current()->process;
	assert(process != NULL);
	char *const *argv = arg;

	(void)process_do_exec(process, process->mpage, argv);
	panic();
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

	int8_t err = process_load(process->mpage, path);
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

	/* TODO prepare stack: argv, exit point etc */

	err = thread_create(thread, process->pid.id, THREAD_PRIORITY_DEFAULT, process_start_thread, argv);
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
	_thread_signal(&process->parent->wait);
}

void process_end(struct process *process, int exit)
{
	struct thread *curr = thread_current();

	if (process == NULL) {
		process = curr->process;
	}

	process->exit = exit;

	file_close_all(process);

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

id_t process_wait(id_t pid, int *status, int8_t options)
{
	int8_t err = 0, found = 0;
	int exit;
	struct process *curr = thread_current()->process, *zombie;

	thread_critical_start();

	if (curr->zombies == NULL && options) {
		thread_critical_end();
		return -EAGAIN;
	}

	do {
		while (curr->zombies == NULL && !err) {
			err = _thread_wait_relative(&curr->wait, 0);
		}

		if (err) {
			thread_critical_end();
			return err;
		}

		zombie = curr->zombies;
		do {
			if (pid < 0 || zombie->pid.id == pid) {
				found = 1;
				break;
			}
			zombie = zombie->next;
		} while (zombie != curr->zombies);
	} while (!found);

	LIST_REMOVE(&curr->zombies, zombie, struct process, next, prev);
	thread_critical_end();

	id_t zpid = zombie->pid.id;

	lock_lock(&common.plock);
	id_remove(&common.pid, &zombie->pid);
	lock_unlock(&common.plock);

	thread_join_all(zombie);

	exit = zombie->exit;
	process_put(zombie);

	if (status != NULL) {
		*status = exit;
	}

	return zpid;
}

void process_init(void)
{
	id_init(&common.pid);
	lock_init(&common.plock);
}

