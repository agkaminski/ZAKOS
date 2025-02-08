/* ZAK180 Firmaware
 * Kernel threads
 * Copyright: Aleksander Kaminski, 2024-2025
 * See LICENSE.md
 */

#ifndef KERNEL_THREDS_H_
#define KERNEL_THREDS_H_

#include <stdint.h>

#include "timer.h"
#include "hal/cpu.h"

#define THREAD_PRIORITY_NO 8

#define THREAD_COUNT_MAX 32

#define THREAD_STATE_ACTIVE 0
#define THREAD_STATE_READY  1
#define THREAD_STATE_SLEEP  2
#define THREAD_STATE_GHOST  3

#define CONTEXT_LAYOUT_KERNEL 0xFE
#define CONTEXT_LAYOUT_USER   0xF1

struct thread {
	/* Wait queue or ready list */
	struct thread *qnext;
	struct thread *qprev;

	struct thread **qwait;

	uint8_t refs : 5;
	uint8_t state : 3;
	uint8_t priority : 4;
	uint8_t exit : 4;

	ktime_t wakeup;

	struct cpu_context *context;
	uint8_t stack_page;
};

void thread_critical_start(void);

void thread_critical_end(void);

struct thread *thread_current(void);

int8_t _thread_yield(void);

int8_t thread_sleep(ktime_t wakeup);

int8_t _thread_wait(struct thread **queue, ktime_t wakeup);

int8_t _thread_signal(struct thread **queue);

int8_t _thread_signal_yield(struct thread **queue);

int8_t _thread_broadcast(struct thread **queue);

int8_t _thread_broadcast_yield(struct thread **queue);

int8_t thread_sleep_relative(ktime_t sleep);

void _thread_on_tick(struct cpu_context *context);

int8_t thread_create(struct thread *thread, uint8_t priority, void (*entry)(void * arg), void *arg);

void thread_init(void);

#endif
