/* ZAK180 Firmaware
 * Kernel main
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include "hal/cpu.h"

#include "mem/page.h"
#include "mem/kmalloc.h"
#include "proc/timer.h"
#include "proc/thread.h"
#include "proc/process.h"

#include "driver/uart.h"
#include "driver/vga.h"
#include "driver/critical.h"

#include "dev/floppy.h"
#include "fs/fs.h"
#include "fs/fat.h"

#include "lib/panic.h"
#include "lib/errno.h"

static struct {
	int8_t schedule;
	struct thread init;
	struct dev_blk floopy;
	struct fs_ctx rootfs;
} common;

/* kmalloc memory pool */
static uint8_t kheap[16 * 1024];

int putchar(int c)
{
	char t = c;

	if (common.schedule) {
		uart1_write(&t, 1, 1);
	}
	else {
		uart1_write_poll(&t, 1);
	}

	vga_putchar(t);

	return 1;
}

extern void floppy_access(uint8_t enable);

void init_thread(void *arg)
{
	(void)arg;

	int ret;

	/* Init floppy and mount rootfs */
	while ((ret = blk_floppy_init(&common.floopy)) < 0) {
		printf("floopy: Init failed (%d), retrying...\r\n", ret);
		thread_sleep_relative(1000);
	}

	printf("floopy: Init done, media size: %ld bytes\r\n", common.floopy.size);
	printf("kernel: Mounting rootfs...\r\n");

	while ((ret = fs_mount(&common.rootfs, &fat_op, &common.floopy, NULL)) < 0) {
		printf("fat: Failed to mount rootfs (%d)\r\n", ret);
		thread_sleep_relative(1000);
	}

	printf("fat: rootfs has been mounted\r\n");

	floppy_access(0);

	/* Start init process */
	/* TODO */

	while (1) {
		printf("alive\r\n");
		thread_sleep_relative(10000);
	}
}

void main(void) __naked
{
	uart_init();
	vga_init();

	printf("ZAK180 Operating System rev " VERSION " " DATE "\r\n");

	/* Start: 64 KB reserved for the kernel
	 * End: VGA starts at @0xFE000 */
	page_init(16, 238);
	timer_init();
	thread_init();
	process_init();
	fs_init();

	kalloc_init(kheap, sizeof(kheap));

	if (thread_create(&common.init, 0, 4, init_thread, NULL) < 0) {
		panic();
	}

	/* Enable interrupts and reschedule */
	common.schedule = 1;
	critical_enable();
	_thread_yield();
}
