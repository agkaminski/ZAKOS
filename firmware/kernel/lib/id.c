/* ZAK180 Firmaware
 * Unique ID allocator/storage
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#include <stddef.h>

#include "id.h"
#include "errno.h"

/* Crappy, but small and simple allocator:
 * get: O(n)
 * insert: O(n)
 * remove: O(n) too
 */

void *__id_get(struct id_storage *storage, id_t id, uint8_t offs)
{
	if (storage->root != NULL) {
		struct id_linkage *curr = storage->root;

		while (curr != NULL) {
			if (curr->id == id) {
				return (void *)((char *)curr - offs);
			}
			curr = curr->next;
		}
	}

	return NULL;
}

void *__id_get_first(struct id_storage *storage, uint8_t offs)
{
	if (storage->root != NULL) {
		return (void *)((char *)storage->root - offs);
	}
	return NULL;
}

void *__if_get_next(void *it, uint8_t offs)
{
	struct id_linkage *curr = (void *)((char *)it + offs);

	if (curr->next == NULL) {
		return NULL;
	}

	return (void *)((char *)curr->next - offs);
}

static int8_t __id_insert(struct id_storage *storage, struct id_linkage *linkage, int8_t depth)
{
	struct id_linkage *curr = storage->root;
	id_t id = storage->counter;

	if ((curr != NULL) && (curr->id <= id)) {
		while (curr->next != NULL) {
			if (curr->id < id && curr->next->id > id) {
				break;
			}
			if (curr->id == id) {
				++id;
			}
			curr = curr->next;
		}

		if (curr->next == NULL) {
			if (curr->id >= id) {
				if (curr->id == ID_MAX) {
					if (depth) {
						return -ENOMEM;
					}
					/* Try again from zero */
					storage->counter = ID_MIN;
					return __id_insert(storage, linkage, 1);
				}

				id = curr->id + 1;
			}
		}
		linkage->next = curr->next;
		curr->next = linkage;
	}
	else {
		linkage->next = storage->root;
		storage->root = linkage;
	}

	linkage->id = id;

	if (id == ID_MAX) {
		id = ID_MIN;
	}
	else {
		++id;
	}

	storage->counter = id;

	return 0;
}

int8_t id_insert(struct id_storage *storage, struct id_linkage *linkage)
{
	return __id_insert(storage, linkage, 0);
}

void id_remove(struct id_storage *storage, struct id_linkage *linkage)
{
	struct id_linkage *curr = storage->root, *prev = NULL;

	while (curr != NULL) {
		if (curr == linkage) {
			if (prev == NULL) {
				storage->root = curr->next;
			}
			else {
				prev->next = curr->next;
			}
			break;
		}
		prev = curr;
		curr = curr->next;
	}
}

void id_init(struct id_storage *storage)
{
	storage->root = NULL;
	storage->counter = ID_MIN;
}
