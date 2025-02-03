/* ZAK180 Firmaware
 * Cyclic list
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#include <stddef.h>

#include "list.h"

void __list_add(void **queue, void *member, uint8_t next, uint8_t prev)
{
	if (*queue == NULL) {
		*queue = member;
		*(uintptr_t *)((uint8_t *)member + next) = (uintptr_t)member;
		*(uintptr_t *)((uint8_t *)member + prev) = (uintptr_t)member;
	}
	else {
		uintptr_t *qp = (uintptr_t *)(((uint8_t *)(*queue)) + prev);

		*(uintptr_t *)((uint8_t *)member + next) = (uintptr_t)*queue;
		*(uintptr_t *)((uint8_t *)member + prev) = *qp;
		*(uintptr_t *)((uint8_t *)*qp + next) = (uintptr_t)member;
		*qp = (uintptr_t)member;
	}
}

void __list_remove(void **queue, void *member, uint8_t next, uint8_t prev)
{
	uintptr_t *mn = (uintptr_t *)((uint8_t *)member + next);
	uintptr_t *mp = (uintptr_t *)((uint8_t *)member + prev);

	if (*mn == (uintptr_t)member) {
		*queue = NULL;
	}
	else {
		if (*queue == member) {
			*queue = (void *)*mn;
		}

		*(uintptr_t *)((uint8_t *)*mp + next) = (uintptr_t)*mn;
		*(uintptr_t *)((uint8_t *)*mn + prev) = (uintptr_t)*mp;
	}
	*mn = (uintptr_t)NULL;
	*mp = (uintptr_t)NULL;
}
