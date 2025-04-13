/* ZAK180 Firmaware
 * Kernel conditional variable
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#include <stdint.h>
#include "thread.h"
#include "lock.h"
#include "timer.h"
#include "cond.h"
#include "lib/assert.h"

int8_t cond_wait(struct thread **queue, struct lock *lock, time_t timeout)
{
	assert(queue != NULL);
	assert(lock != NULL);

	thread_critical_start();
	_lock_unlock(lock);
	if (timeout) {
		/* Convert relative to absolute */
		timeout += timer_get();
	};
	int8_t ret = _thread_wait(queue, timeout);
	_lock_lock(lock);
	thread_critical_end();
	return ret;
}

int8_t cond_signal(struct thread **queue)
{
	assert(queue != NULL);

	thread_critical_start();
	int8_t ret = _thread_signal(queue);
	thread_critical_end();
	return ret;
}

int8_t cond_broadcast(struct thread **queue)
{
	assert(queue != NULL);

	thread_critical_start();
	int8_t ret = _thread_broadcast(queue);
	thread_critical_end();
	return ret;
}
