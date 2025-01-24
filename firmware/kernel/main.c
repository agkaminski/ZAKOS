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

#include "driver/uart.h"
#include "driver/vga.h"
#include "driver/critical.h"

#include "dev/floppy.h"
#include "fs/fs.h"
#include "fs/fat.h"

#include "lib/panic.h"

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

static void test_open(const char *path)
{
	struct fs_file *file;
	printf("test: open %s\r\n", path);
	int8_t ret = fs_open(path, &file, O_RDONLY, 0);
	printf("test: ret = %d, file = %p\r\n", ret, file);
	if (!ret) {
		printf("name = [%s]\r\n", file->name);
		printf("attr = 0x%02x\r\n", file->attr);
		printf("parent = %p\r\n", file->parent);
		printf("children = %p\r\n", file->children);
		printf("nrefs = %d\r\n", file->nrefs);
		printf("size = %lu\r\n", file->size);
	}
	printf("\r\n");
}

static void test_readdir(const char *path)
{
	struct fs_file *dir;
	printf("test: readdir %s\r\n", path);
	int8_t ret = fs_open(path, &dir, O_RDONLY, 0);
	printf("test: ret = %d, file = %p\r\n", ret, dir);
	if (!ret) {
		struct fs_dentry dentry;
		uint16_t idx = 0;

		while (1) {
			ret = fs_readdir(dir, &dentry, &idx);
			if (ret < 0) {
				break;
			}

			printf("idx = %u\r\n", idx);
			printf("name = [%s]\r\n", dentry.name);
			printf("size = %lu\r\n", dentry.size);
			printf("ctime = %llu\r\n", dentry.ctime);
			printf("atime = %llu\r\n", dentry.atime);
			printf("mtime = %llu\r\n", dentry.mtime);
			printf("\r\n");
		}
		printf("\r\n");
	}
}

static void test_readdump(const char *path)
{
	struct fs_file *file;
	printf("test: dump %s\r\n", path);
	int8_t ret = fs_open(path, &file, O_RDONLY, 0);
	printf("test: ret = %d, file = %p\r\n", ret, file);
	if (!ret) {
		char buff[33];
		off_t off = 0;

		while ((ret = fs_read(file, buff, sizeof(buff) - 1, off)) > 0) {
			buff[ret] = '\0';
			printf("%s", buff);
			off += ret;
		}
	}
	printf("\r\n");
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

	test_open("/foobar");
	test_open("/");
	test_open("/BOOT");
	test_open("/BOOT/KERNEL");
	test_open("/BOOT/KERNEL.IMG");
	test_open("/BOOT");

	test_readdir("/");
	test_readdir("/BOOT");

	test_readdump("/FANATYK.TXT");

	floppy_access(0);

	while (1) {
		thread_sleep_relative(1000);
		printf("alive\r\n");
	}
}

int main(void)
{
	uart_init();
	vga_init();

	printf("ZAK180 Operating System rev " VERSION " " DATE "\r\n");

	/* Start: 64 KB reserved for the kernel
	 * End: VGA starts at @0xFE000 */
	page_init(16, 238);
	timer_init();
	thread_init();
	fs_init();

	kalloc_init(kheap, sizeof(kheap));

	thread_create(&common.init, 4, init_thread, NULL);

	/* Enable interrupts and wait for reschedule */
	thread_start();
	common.schedule = 1;
	critical_enable();
	_EI;

	return 0;
}
