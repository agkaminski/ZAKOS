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

/* Memory space for each process
 * Whole memory minus stack (1 page at the end) and
 * IVT and kernel entry point (1 page at the beginning) */
#define PROCESS_PAGES (((64ul * 1024) / PAGE_SIZE) - 2)

struct thread;

struct process {
	struct id_storage threads;
	struct thread *ghosts;

	struct process *parent;
	struct process *children;

	char *path;

	uint8_t mpage;

	int8_t refs;
	struct id_linkage pid;

	int8_t exit;

	struct lock lock;
};

struct process *process_get(id_t pid);

void process_put(struct process *process);

int8_t process_execve(const char *path, const char *argv, const char *envp);

id_t process_vfork(void);

id_t process_start(const char *path, char *argv);

void process_init(void);

#endif
