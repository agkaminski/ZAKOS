/* ZAK180 Firmaware
 * Kernel unit tests - kmalloc
 * Adapted from github.com/agkaminski/ualloc
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#include <string.h>
#include <stdlib.h>

#include "mem/kmalloc.h"
#include "test/rand.h"
#include "hal/cpu.h"
#include "proc/thread.h"
#include "lib/kprintf.h"

#define NELEMS(x) (sizeof(x) / sizeof(x[0]))

static char heap[16 * 1024];

static char *ptr[128];
static size_t size[128];
static struct thread thread;

static int check(void)
{
	for (int i = 0; i < NELEMS(ptr); ++i) {
		if (ptr[i] != NULL) {
			if (ptr[i][0] != (char)i || memcmp(&ptr[i][0], &ptr[i][1], size[i] - 1)) {
				kprintf("Error ptr[%d]\r\n", i);
				return -1;
			}
		}
	}

	return 0;
}

static void heap_show(void *heap)
{
	struct {
		size_t size;
		void *next;
	} *header;

	for (header = heap; header != NULL; header = header->next) {
		kprintf("\t0x%x: [%u,", header, header->size & 0xfffeUL);
		if (header->size & 1) {
			kprintf(" USED]\r\n");
		}
		else {
			kprintf(" FREE]\r\n");
		}
	}
}

static void test(void *arg)
{
	(void)arg;

	kalloc_init(heap, sizeof(heap));
	test_srand(420);

	for (size_t i = 0; i < NELEMS(ptr); ++i) {
		ptr[i] = NULL;
		size[i] = 0;
	}

	for (size_t i = 0; i < 10000; ++i) {
		size_t pos = test_rand16() % NELEMS(ptr);

		if (ptr[pos] != NULL) {
			kfree(ptr[pos]);
		}

		size[pos] = test_rand16() & 0x1ff;
		ptr[pos] = kmalloc(size[pos]);

		kprintf("%d: Alocated %u bytes, got 0x%x\r\n", pos, size[pos], ptr[pos]);

		if (ptr[pos] == NULL) {
			continue;
		}

		memset(ptr[pos], pos, size[pos]);

		if (check() != 0) {
			break;
		}

		heap_show(heap);
	}

	kprintf("Special cases\r\n");
	kprintf("Free all\r\n");
	for (size_t i = 0; i < NELEMS(ptr); ++i) {
		kfree(ptr[i]);
		ptr[i] = NULL;
		size[i] = 0;
	}

	heap_show(heap);

	kprintf("Biggest alloc\r\n");

	ptr[0] = kmalloc(sizeof(heap) - 16);

	heap_show(heap);

	kfree(ptr[0]);

	heap_show(heap);

	kprintf("Too big\r\n");

	ptr[0] = kmalloc(sizeof(heap));

	heap_show(heap);

	kprintf("Test krealloc\r\n");

	for (size_t i = 0; i < 10000; ++i) {
		size_t pos = test_rand16() % NELEMS(ptr);

		kprintf("%d: Is 0x%x (%u bytes), ", pos, ptr[pos], size[pos]);

		size_t prev = size[pos];

		size_t tsz = test_rand16() & 0x3ff;
		void *tptr = krealloc(ptr[pos], tsz);

		kprintf("alocated %u bytes, got 0x%x\r\n", tsz, tptr);

		if (tptr == NULL && tsz != 0 && ptr[pos] != NULL && prev >= tsz) {
			kprintf("krealloc failed (prev >= curr && NULL)\r\n");
			break;
		}

		if (tsz == 0) {
			ptr[pos] = NULL;
			size[pos] = 0;
		}

		if (tptr == NULL) {
			continue;
		}

		size[pos] = tsz;
		ptr[pos] = tptr;

		memset(ptr[pos], pos, size[pos]);

		heap_show(heap);

		if (check() != 0) {
			break;
		}
	}

	for (;;) {
		_HALT;
	}
}

void test_kmalloc(void)
{
	thread_create(&thread, 4, test, NULL);
}
