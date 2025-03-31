/* ZAK180 Firmaware
 * Kernel main
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#include <string.h>
#include <stdint.h>

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
#include "lib/kprintf.h"

static struct {
	struct thread init;
	struct dev_blk floopy;
	struct fs_ctx rootfs;
} common;

extern void floppy_access(uint8_t enable);

void init_thread(void *arg)
{
	(void)arg;

	int ret;

	/* Init floppy and mount rootfs */
	while ((ret = blk_floppy_init(&common.floopy)) < 0) {
		kprintf("floopy: Init failed (%d), retrying...\r\n", ret);
		thread_sleep_relative(1000);
	}

	kprintf("floopy: Init done, media size: %u KB\r\n", (unsigned)(common.floopy.size / 1024));
	kprintf("kernel: Mounting rootfs...\r\n");

	while ((ret = fs_mount(&common.rootfs, &fat_op, &common.floopy, NULL)) < 0) {
		kprintf("fat: Failed to mount rootfs (%d)\r\n", ret);
		thread_sleep_relative(1000);
	}

	kprintf("fat: rootfs has been mounted\r\n");

	/* Start init process */
	ret = process_start("/BIN/HELLO.ZEX", NULL);
	thread_sleep_relative(1000);

	floppy_access(0);

	while (1) {
		kprintf("alive %u\r\n", (unsigned)timer_get());
		thread_sleep_relative(10000);
	}
}

void main(void) __naked
{
	uart_init();
	vga_init();

	_kprintf("ZAK180 Operating System rev " VERSION " " DATE "\r\n");

	/* Start: 64 KB reserved for the kernel
	 * End: VGA starts at @0xFE000 */
	page_init(16, 238);
	timer_init();
	thread_init();
	process_init();
	fs_init();

	/* Calculate heap area and init kmalloc */
	extern uint16_t _bss_end(void);
	uint16_t bss_end = _bss_end();
	size_t heap_size = 0xe000 - bss_end;
	kalloc_init((void *)bss_end, heap_size);

	if (thread_create(&common.init, 0, 4, init_thread, NULL) < 0) {
		panic();
	}

	/* Enable interrupts and reschedule */
	critical_enable();
	kprintf_use_irq(1);
	_thread_yield();
}
