/* ZAK180 Firmaware
 * Unique ID allocator/storage
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#ifndef KERNEL_LIB_ID_H_
#define KERNEL_LIB_ID_H_

#include <stdint.h>
#include <stddef.h>

typedef int16_t id_t;
#define ID_MAX INT16_MAX

struct id_linkage {
	struct id_linkage *next;
	id_t id;
};

struct id_storage {
	struct id_linkage *root;
	id_t counter;
};

void *__id_get(struct id_storage *storage, id_t id, uint8_t offs);

#define id_get(storage, id, type, member) (type *)__id_get(storage, id, offsetof(type, member))

int8_t id_insert(struct id_storage *storage, struct id_linkage *linkage);

void id_remove(struct id_storage *storage, struct id_linkage *linkage);

void id_init(struct id_storage *storage);

#endif
