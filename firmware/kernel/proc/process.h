/* ZAK180 Firmaware
 * Processes
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#ifndef KERNEL_PROCESS_H_
#define KERNEL_PROCESS_H_

#include "mem/page.h"
#include "lib/id.h"
#include "lock.h"
#include "file.h"

/* Memory space for each process
 * Whole memory minus stack (1 page at the end) and
 * IVT and kernel entry point (1 page at the beginning) */
#define PROCESS_PAGES     (((64ul * 1024) / PAGE_SIZE) - 2)
#define PROCESS_MEM_START 0x1000

struct thread;
struct file_descriptor;

struct process {
	/* List linkage (children/zombies) */
	struct process *next;
	struct process *prev;

	/* Thread management */
	struct id_storage threads;
	struct thread *ghosts;
	struct thread *reaper;
	uint8_t thread_no;

	/* Process management */
	struct process *parent;
	struct process *children;
	struct process *zombies;
	struct thread *wait;
	int exit;

	char *path;

	uint8_t mpage;

	/* Resources */
	struct file_descriptor *fdtable[16];

	/* PID */
	int8_t refs;
	struct id_linkage pid;

	struct lock lock;
};

struct process *process_get(id_t pid);

void process_put(struct process *process);

int8_t process_execv(const char *path, char *const argv[]);

id_t process_fork(void);

id_t process_start(const char *path, char *argv);

void _process_zombify(struct process *process);

void process_end(struct process *process, int exit);

id_t process_wait(id_t pid, int *status, int8_t options);

void process_init(void);

#endif
