/* ZAK180 Firmaware
 * Kernel threads
 * Copyright: Aleksander Kaminski, 2024-2025
 * See LICENSE.md
 */

#include <stddef.h>

#include "proc/thread.h"

#include "mem/page.h"

#include "driver/critical.h"
#include "driver/mmu.h"
#include "driver/critical.h"

#include "lib/errno.h"
#include "lib/list.h"
#include "lib/assert.h"

static struct {
	struct thread *threads;
	struct thread *sleeping;
	struct thread *ready[THREAD_PRIORITY_NO];
	struct thread *current;

	uint16_t idcntr;

	struct thread idle;

	volatile int8_t schedule;
	volatile uint8_t lock_level;
} common;

static void thread_scheduler_lock(void)
{
	common.schedule = 0;
}

static void thread_scheduler_unlock(void)
{
	common.schedule = 1;
}

void thread_critical_start(void)
{
	critical_start();
	thread_scheduler_lock();
	++common.lock_level;
	critical_end();
}

void thread_critical_end(void)
{
	critical_start();
	if (--common.lock_level == 0) {
		thread_scheduler_unlock();
	}
	critical_end();
}

static void _thread_sleeping_enqueue(ktime_t wakeup)
{
	LIST_ADD(&common.sleeping, common.current, struct thread, snext, sprev);

	common.current->wakeup = wakeup;
	common.current->state = THREAD_STATE_SLEEP;
}

static void _threads_add_ready(struct thread *thread)
{
	LIST_ADD(&common.ready[thread->priority], thread, struct thread, qnext, qprev);

	thread->state = THREAD_STATE_READY;

	if (thread->wakeup && common.sleeping != NULL) {
		LIST_REMOVE(&common.sleeping, thread, struct thread, snext, sprev);

		thread->wakeup = 0;
	}
}

static void _thread_dequeue(struct thread *thread)
{
	assert(thread != NULL);

	LIST_REMOVE(thread->qwait, thread, struct thread, qnext, qprev);
	thread->qwait = NULL;
	_threads_add_ready(thread);
}

void _thread_schedule(struct cpu_context *context)
{
	struct thread *prev = common.current;

	/* Put current thread */
	if (common.current != NULL) {
		common.current->context = context;

		if (common.current->state == THREAD_STATE_ACTIVE) {
			_threads_add_ready(common.current);
		}
	}

	/* Select new thread */
	struct thread *selected = NULL;
	for (uint8_t priority = 0; priority < THREAD_PRIORITY_NO; ++priority) {
		if (common.ready[priority] != NULL) {
			selected = common.ready[priority];
			LIST_REMOVE(&common.ready[priority], selected, struct thread, qnext, qprev);
			break;
		}
	}

	assert(selected != NULL);

	common.current = selected;
	selected->state = THREAD_STATE_ACTIVE;

	if (selected != prev) {
		/* Map selected thread stack space into the scratch page */
		uint8_t *scratch = mmu_map_scratch(selected->stack_page, NULL);
		struct cpu_context *selctx = (void *)((uint8_t *)selected->context - PAGE_SIZE);

		/* Switch context */
		context->nsp = selctx->sp;
		context->nmmu = selctx->mmu;
		context->nlayout = selctx->layout;
	}
}

static void _thread_set_return(struct thread *thread, int value)
{
	assert(thread != NULL);

	uint8_t *scratch = mmu_map_scratch(thread->stack_page, NULL);
	struct cpu_context *tctx = (void *)((uint8_t *)thread->context - PAGE_SIZE);
	tctx->de = (uint16_t)value;
}

void _thread_on_tick(struct cpu_context *context)
{
	if (common.schedule) {
		/* Allow HW IRQ to preempt the scheduler */
		common.schedule = 0;
		_EI;

		ktime_t now = _timer_get();

		while (common.sleeping != NULL && common.sleeping->wakeup <= now) {
			_thread_set_return(common.sleeping, -ETIME);
			_threads_add_ready(common.sleeping);
		}

		_thread_schedule(context);

		_DI;
		common.schedule = 1;
	}
}

int8_t thread_sleep(ktime_t wakeup)
{
	thread_critical_start();
	_thread_sleeping_enqueue(wakeup);
	return thread_yield(&common.schedule);
}

int8_t thread_sleep_relative(ktime_t sleep)
{
	thread_critical_start();
	return thread_sleep(_timer_get() + sleep);
}

int8_t _thread_wait(struct thread **queue, ktime_t wakeup)
{
	assert(queue != NULL);

	LIST_ADD(queue, common.current, struct thread, qnext, qprev);

	common.current->wakeup = wakeup;
	common.current->state = THREAD_STATE_SLEEP;
	common.current->qwait = queue;

	if (wakeup) {
		_thread_sleeping_enqueue(wakeup);
	}

	int ret = thread_yield(&common.schedule);
	thread_critical_start();

	return ret;
}

int8_t _thread_signal(struct thread **queue)
{
	assert(queue != NULL);

	if (*queue != NULL) {
		_thread_dequeue(*queue);
		return 1;
	}

	return 0;
}

int8_t _thread_signal_yield(struct thread **queue)
{
	assert(queue != NULL);

	if (_thread_signal(queue)) {
		(void)thread_yield(&common.schedule);
		return 1;
	}
	else {
		thread_critical_end();
	}

	return 0;
}

int8_t _thread_broadcast(struct thread **queue)
{
	assert(queue != NULL);

	int ret = 0;

	while (*queue != NULL) {
		_thread_dequeue(*queue);
		ret = 1;
	}

	return ret;
}

int8_t _thread_broadcast_yield(struct thread **queue)
{
	assert(queue != NULL);

	if (_thread_broadcast(queue)) {
		(void)thread_yield(&common.schedule);
		return 1;
	}
	else {
		thread_critical_end();
	}

	return 0;
}

static void thread_context_create(struct thread *thread, uint16_t entry, void *arg)
{
	assert(thread != NULL);

	uint8_t *scratch = mmu_map_scratch(thread->stack_page, NULL);
	struct cpu_context *tctx = (void *)(scratch + PAGE_SIZE - sizeof(struct cpu_context));

	tctx->pc = entry;
	tctx->af = 0;
	tctx->bc = 0;
	tctx->de = 0;
	tctx->hl = (uint16_t)arg;
	tctx->ix = 0;
	tctx->iy = 0;

	/* TODO set mmu layout according to the process */
	tctx->layout = CONTEXT_LAYOUT_KERNEL;
	tctx->mmu = (uint16_t)(thread->stack_page - (tctx->layout >> 4)) << 8;

	thread->context = (void *)((uint8_t *)tctx + PAGE_SIZE);
	tctx->sp = (uint16_t)((uint8_t *)thread->context + 12);
}

static void thread_idle(void *arg)
{
	(void)arg;

	while (1) {
		_HALT;
	}
}

int8_t thread_create(struct thread *thread, uint8_t priority, void (*entry)(void * arg), void *arg)
{
	assert(thread != NULL);
	assert(entry != NULL);

	thread->snext = NULL;
	thread->sprev = NULL;
	thread->qnext = NULL;
	thread->qwait = NULL;
	thread->id = ++common.idcntr;
	thread->refs = 1;
	thread->priority = priority;
	thread->exit = 0;
	thread->wakeup = 0;

	thread->stack_page = page_alloc(NULL, 1);
	if (thread->stack_page == 0) {
		return -ENOMEM;
	}

	thread_context_create(thread, (uint16_t)entry, arg);

	thread_critical_start();
	_threads_add_ready(thread);
	thread_critical_end();

	return 0;
}

void thread_start(void)
{
	common.schedule = 1;
}

void thread_init(void)
{
	thread_create(&common.idle, THREAD_PRIORITY_NO - 1, thread_idle, NULL);
}
