/* ZAK180 Firmaware
 * Binary heap
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#include <stddef.h>

#include "bheap.h"
#include "errno.h"

static void bheap_swap(struct bheap *heap, bheap_idx a, bheap_idx b)
{
	void *t = heap->array[a];
	heap->array[a] = heap->array[b];
	heap->array[b] = t;
}

static int8_t bheap_compare(struct bheap *heap, bheap_idx a, bheap_idx b)
{
	return heap->compare(heap->array[a], heap->array[b]);
}

static void bheap_heapify(struct bheap *heap, bheap_idx idx)
{
	bheap_idx p;

	while (idx && (p = (idx - 1) / 2, bheap_compare(heap, p, idx) > 0)) {
		bheap_swap(heap, idx, p);
		idx = p;
	}
}

static void bheap_rev_heapify(struct bheap *heap, bheap_idx idx)
{
	while (1) {
		bheap_idx l = (idx << 1) + 1, r = l + 1, sel = idx;

		if (r < heap->size && (bheap_compare(heap, r, sel) < 0)) {
			sel = r;
		}
		if (l < heap->size && (bheap_compare(heap, l, sel) < 0)) {
			sel = l;
		}

		if (sel == idx) {
			break;
		}

		bheap_swap(heap, idx, sel);
		idx = sel;
	}
}

int8_t bheap_insert(struct bheap *heap, void *elem)
{
	if (heap->capacity == heap->size) {
		return -ENOMEM;
	}

	bheap_idx idx = heap->size++;
	heap->array[idx] = elem;

	bheap_heapify(heap, idx);

	return 0;
}

static void bheap_remove(struct bheap *heap, bheap_idx idx)
{
	--heap->size;
	int8_t up = bheap_compare(heap, idx, heap->size);

	bheap_swap(heap, idx, heap->size);

	if (up > 0) {
		bheap_heapify(heap, idx);
	}
	else {
		bheap_rev_heapify(heap, idx);
	}
}

int8_t bheap_pop(struct bheap *heap, void **elem)
{
	if (heap->size == 0) {
		return -ENOENT;
	}

	if (elem != NULL) {
		*elem = heap->array[0];
	}
	bheap_remove(heap, 0);

	return 0;
}

int8_t bheap_peek(struct bheap *heap, void **elem)
{
	if (heap->size == 0) {
		return -ENOENT;
	}

	*elem = heap->array[0];
	return 0;
}

int8_t bheap_extract(struct bheap *heap, void *elem)
{
	bheap_idx idx;

	for (idx = 0; idx < heap->size; ++idx) {
		if (heap->array[idx] == elem) {
			bheap_remove(heap, idx);
			return 0;
		}
	}

	return -ENOENT;
}

void bheap_init(struct bheap *heap, void **array, bheap_idx capacity, int8_t (*compare)(void *, void *))
{
	heap->array = array;
	heap->capacity = capacity;
	heap->size = 0;
	heap->compare = compare;
}
