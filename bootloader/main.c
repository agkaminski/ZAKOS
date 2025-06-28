/* ZAK180 Firmaware
 * Bootloader main
 * Copyright: Aleksander Kaminski 2024, 2025
 * See LICENSE.md
 */

#include <string.h>
#include <stdio.h>

#include "driver/uart.h"
#include "driver/vga.h"
#include "driver/floppy.h"
#include "driver/mmu.h"
#include "driver/critical.h"
#include "fat12ro.h"

#define SCRATCH_SIZE (8 * 1024)

__sfr __at(0x0080) COL0;
__sfr __at(0x0081) COL1;
__sfr __at(0x0082) COL2;
__sfr __at(0x0083) COL3;
__sfr __at(0x0084) COL4;
__sfr __at(0x0085) COL5;
__sfr __at(0x0086) COL6;
__sfr __at(0x0087) COL7;
__sfr __at(0x0088) COL8;
__sfr __at(0x0089) COL9;
__sfr __at(0x008A) COL10;
__sfr __at(0x008B) COL11;
__sfr __at(0x008C) COL12;
__sfr __at(0x008D) COL13;
__sfr __at(0x008E) COL14;
__sfr __at(0x008F) COL15;

int putchar(int c)
{
	char t = c;

	uart1_write_poll(&t, 1);
	vga_putchar(t);

	return 1;
}

static struct fat12_fs fs;
static const struct fat12_cb cb = {
	.read_sector = floppy_read_sector,
	.write_sector = floppy_write_sector
};

static void prompt(void)
{
	floppy_access(0);

	printf("Fatal error, press any key to retry\r\n");

	while (1) {
		if ((COL0 & 0x1F) != 0x1F) break;
		if ((COL1 & 0x1F) != 0x1F) break;
		if ((COL2 & 0x1F) != 0x1F) break;
		if ((COL3 & 0x1F) != 0x1F) break;
		if ((COL4 & 0x1F) != 0x1F) break;
		if ((COL5 & 0x1F) != 0x1F) break;
		if ((COL6 & 0x1F) != 0x1F) break;
		if ((COL7 & 0x1F) != 0x1F) break;
		if ((COL8 & 0x1F) != 0x1F) break;
		if ((COL9 & 0x1F) != 0x1F) break;
		if ((COL10 & 0x1F) != 0x1F) break;
		if ((COL11 & 0x1F) != 0x1F) break;
		if ((COL12 & 0x1F) != 0x1F) break;
		if ((COL13 & 0x1F) != 0x1F) break;
		if ((COL14 & 0x1F) != 0x1F) break;
		if ((COL15 & 0x1F) != 0x1F) break;
	}
}

static void kernel_jump(void)
{
	/* Give some time for vblank to come
	 * and refresh the screen. */
	for (volatile uint16_t i = 0; i < 6000; ++i);

	__asm
		di
		ld sp, #0x0000
		jp 0x0000
	__endasm;

	/* Never reached */
}

int main(void)
{
	critical_enable();
	uart_init();
	vga_init();

	printf("ZAK180 Bootloader rev " VERSION " " DATE "\r\n");

retry:

	printf("Floppy drive initialisation\r\n");
	int ret = floppy_init();
	if (ret < 0) {
		printf("Could not initialise media, please insert the system disk\r\n");
		prompt();
		goto retry;
	}

	printf("Mounting filesystem\r\n");
	ret = fat12_mount(&fs, &cb);
	if (ret < 0) {
		printf("No disk or inserted disk is not bootable\r\n");
		prompt();
		goto retry;
	}

	struct fat12_file file;
	ret = fat12_file_open(&fs, &file, "/BOOT/KERNEL.IMG");
	if (ret < 0) {
		printf("Could not find the kernel image.\r\nMake sure the kernel is present in /BOOT/KERNEL.IMG\r\n");
		prompt();
		goto retry;
	}

	printf("Loading the kernel image...\r\n");
	uint32_t total = 0;
	uint8_t page = 0;
	uint8_t done = 0;
	uint32_t offs = 0;
	do {
		uint8_t *dest = mmu_map_scratch(page, NULL);
		uint16_t left = SCRATCH_SIZE;
		uint16_t pos = 0;
		while (left) {
			int got = fat12_file_read(&fs, &file, dest + pos, SCRATCH_SIZE, offs);
			if (got < 0) {
				printf("File read error %d\r\n", got);
				prompt();
				goto retry;
			}
			if (!got) {
				done = 1;
				break;
			}

			left -= got;
			pos += got;
			offs += got;
			total += got;
		}
		printf("\rLoaded %llu bytes", total);
		page += SCRATCH_SIZE / PAGE_SIZE;
	} while (!done);

	floppy_access(0);

	printf("\r\nStarting the kernel...\r\n");

	kernel_jump();

	/* Never reached*/
	return 0;
}
