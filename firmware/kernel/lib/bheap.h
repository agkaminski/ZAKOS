/* ZAK180 Firmaware
 * Binary heap
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#ifndef KERNEL_LIB_BHEAP_H_
#define KERNEL_LIB_BHEAP_H_

#include <stdint.h>

typedef uint8_t bheap_idx;

struct bheap {
	void **array;
	bheap_idx capacity;
	bheap_idx size;
	int8_t (*compare)(void *, void *);
};

int8_t bheap_insert(struct bheap *heap, void *elem);

int8_t bheap_pop(struct bheap *heap, void **elem);

int8_t bheap_peek(struct bheap *heap, void **elem);

int8_t bheap_extract(struct bheap *heap, void *elem);

void bheap_init(struct bheap *heap, void **array, bheap_idx capacity, int8_t (*compare)(void *, void *));

#endif
