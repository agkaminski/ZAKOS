/* ZAK180 Firmaware
 * Kernel threads
 * Copyright: Aleksander Kaminski, 2024-2025
 * See LICENSE.md
 */

#include <stddef.h>

#include "proc/thread.h"
#include "proc/lock.h"
#include "proc/process.h"

#include "mem/page.h"

#include "driver/critical.h"
#include "driver/mmu.h"
#include "driver/critical.h"

#include "lib/errno.h"
#include "lib/list.h"
#include "lib/assert.h"
#include "lib/bheap.h"
#include "lib/id.h"

static struct {
	struct thread *ready[THREAD_PRIORITY_NO];
	struct thread *ghosts;
	struct thread *current;

	struct thread *sleeping_array[THREAD_COUNT_MAX];
	struct bheap sleeping;

	struct thread idle;

	volatile int8_t schedule;
} common;

void thread_critical_start(void)
{
	critical_start();
	common.schedule = 0;
	critical_end();
}

void _thread_critical_end(void)
{
	common.schedule = 1;
}

void thread_critical_end(void)
{
	critical_start();
	common.schedule = 1;
	critical_end();
}

struct thread *thread_current(void)
{
	return common.current;
}

static int8_t _thread_wakeup_compare(void *v1, void *v2)
{
	struct thread *t1 = v1;
	struct thread *t2 = v2;

	if (t1->wakeup > t2->wakeup) {
		return 1;
	}
	else if (t1->wakeup < t2->wakeup) {
		return -1;
	}

	return 0;
}

static void _thread_sleeping_enqueue(ktime_t wakeup)
{
	common.current->wakeup = wakeup;
	common.current->state = THREAD_STATE_SLEEP;
	bheap_insert(&common.sleeping, common.current);
}

static void _threads_add_ready(struct thread *thread)
{
	LIST_ADD(&common.ready[thread->priority], thread, struct thread, qnext, qprev);

	thread->state = THREAD_STATE_READY;

	if (thread->wakeup) {
		bheap_extract(&common.sleeping, thread);
		thread->wakeup = 0;
	}
}

static void _thread_dequeue(struct thread *thread)
{
	assert(thread != NULL);

	if (thread->qwait != NULL) {
		LIST_REMOVE(thread->qwait, thread, struct thread, qnext, qprev);
		thread->qwait = NULL;
	}
	_threads_add_ready(thread);
}

static void _thread_kill(struct thread *thread)
{
	struct process *process = thread->process;

	/* Kernel threads do not end! */
	assert(process != NULL);
	thread->state = THREAD_STATE_GHOST;
	LIST_ADD(&process->ghosts, thread, struct thread, qnext, qprev);
	if (--process->thread_no == 0) {
		_process_zombify(process);
	}
}

void thread_end(struct thread *thread)
{
	thread_critical_start();
	if (thread == NULL) {
		_thread_kill(common.current);
		_thread_yield();
	}
	else {
		thread->exit = 1;
		thread_critical_end();
	}
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
	for (uint8_t priority = 0; priority < THREAD_PRIORITY_NO; ++priority) {
		if (common.ready[priority] != NULL) {
			struct thread *selected = common.ready[priority];
			LIST_REMOVE(&common.ready[priority], selected, struct thread, qnext, qprev);

			/* Map selected thread stack space into the scratch page */
			/* Scratch page is one page before stack page */
			uint8_t prev;
			(void)mmu_map_scratch(selected->stack_page, &prev);
			struct cpu_context *selctx = (void *)((uint8_t *)selected->context - PAGE_SIZE);

			if ((selected->exit) && (selctx->layout != CONTEXT_LAYOUT_KERNEL)) {
				_thread_kill(selected);
				(void)mmu_map_scratch(prev, NULL);
			}
			else {
				common.current = selected;
				selected->state = THREAD_STATE_ACTIVE;

				/* Switch context */
				context->nsp = selctx->sp;
				context->nmmu = selctx->mmu;
				context->nlayout = selctx->layout;
				(void)mmu_map_scratch(prev, NULL);
				break;
			}
		}
	}

	_DI;
	common.schedule = 1;
}

static void _thread_set_return(struct thread *thread, int value)
{
	assert(thread != NULL);

	uint8_t prev;
	uint8_t *scratch = mmu_map_scratch(thread->stack_page, &prev);
	struct cpu_context *tctx = (void *)((uint8_t *)thread->context - PAGE_SIZE);
	tctx->de = (uint16_t)value;
	(void)mmu_map_scratch(prev, NULL);
}

int8_t _thread_reschedule(volatile uint8_t *scheduler_lock);

int8_t _thread_yield(void)
{
	return _thread_reschedule(&common.schedule);
}

void _thread_on_tick(struct cpu_context *context)
{
	if (common.schedule) {
		/* Allow HW IRQ to preempt the scheduler */
		common.schedule = 0;
		_EI;

		ktime_t now = _timer_get();
		struct thread *t;

		while (!bheap_peek(&common.sleeping, &t) && (t->wakeup <= now)) {
			_thread_set_return(t, -ETIME);
			_thread_dequeue(t);
		}

		_thread_schedule(context);
	}
}

int8_t thread_sleep(ktime_t wakeup)
{
	thread_critical_start();
	_thread_sleeping_enqueue(wakeup);
	return _thread_yield();
}

int8_t thread_sleep_relative(ktime_t sleep)
{
	thread_critical_start();
	_thread_sleeping_enqueue(_timer_get() + sleep);
	return _thread_yield();
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

	int ret = _thread_yield();
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
		(void)_thread_yield();
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
		(void)_thread_yield();
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

	uint8_t prev;
	uint8_t *scratch = mmu_map_scratch(thread->stack_page, &prev);
	struct cpu_context *tctx = (void *)(scratch + PAGE_SIZE - sizeof(struct cpu_context));

	tctx->pc = entry;
	tctx->af = 0;
	tctx->bc = 0;
	tctx->de = 0;
	tctx->hl = (uint16_t)arg;
	tctx->ix = 0;
	tctx->iy = 0;

	tctx->layout = (thread->process != NULL) ? CONTEXT_LAYOUT_USER : CONTEXT_LAYOUT_KERNEL;
	tctx->mmu = (uint16_t)(thread->stack_page - (tctx->layout >> 4)) << 8;

	if (thread->process != NULL) {
		tctx->mmu |= thread->process->mpage - (tctx->layout & 0x0f);
	}

	thread->context = (void *)((uint8_t *)tctx + PAGE_SIZE);
	tctx->sp = (uint16_t)((uint8_t *)thread->context + 12);

	(void)mmu_map_scratch(prev, NULL);
}

static void thread_idle(void *arg)
{
	(void)arg;

	while (1) {
		_HALT;
	}
}

int8_t thread_create(struct thread *thread, id_t pid, uint8_t priority, void (*entry)(void *arg), void *arg)
{
	assert(thread != NULL);
	assert(entry != NULL);

	thread->qnext = NULL;
	thread->qwait = NULL;
	thread->priority = priority;
	thread->wakeup = 0;

	thread->stack_page = page_alloc(NULL, 1);
	if (thread->stack_page == 0) {
		return -ENOMEM;
	}

	if (pid) {
		struct process *p = process_get(pid);
		if (p == NULL) {
			page_free(thread->stack_page, 1);
			return -EINVAL;
		}

		lock_lock(&p->lock);
		int8_t err = id_insert(&p->threads, &thread->id);
		if (err != 0) {
			lock_unlock(&p->lock);
			process_put(p);
			page_free(thread->stack_page, 1);
			return err;
		}
		++p->thread_no;
		thread->process = p;
		lock_unlock(&p->lock);
	}

	thread_context_create(thread, (uint16_t)entry, arg);

	thread_critical_start();
	_threads_add_ready(thread);
	thread_critical_end();

	return 0;
}

void thread_init(void)
{
	bheap_init(&common.sleeping, common.sleeping_array, THREAD_COUNT_MAX, _thread_wakeup_compare);
	thread_create(&common.idle, 0, THREAD_PRIORITY_NO - 1, thread_idle, NULL);
}
