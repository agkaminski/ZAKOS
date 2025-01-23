/* ZAK180 Firmaware
 * Kernel conditional variable
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#ifndef KERNEL_COND_H_
#define KERNEL_COND_H_

#include <stdint.h>
#include "thread.h"
#include "lock.h"
#include "timer.h"

int8_t cond_wait(struct thread **queue, struct lock *lock, ktime_t timeout);

int8_t cond_signal(struct thread **queue);

int8_t cond_broadcast(struct thread **queue);

#endif
