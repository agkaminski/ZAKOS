/* ZAK180 Firmaware
 * UART driver
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <sys/types.h>
#include "fs/devfs.h"
#include "driver/uart.h"
#include "hal/cpu.h"
#include "lib/errno.h"
#include "uart.h"

static struct {
	uint8_t minors[2];
} common;

static uint8_t dev_uart_minor_to_uart(uint8_t minor)
{
	if (common.minors[0] == minor) {
		return 0;
	}
	else {
		return 1;
	}
}

static int16_t dev_uart_read(uint8_t minor, void *buff, size_t bufflen, off_t offs)
{
	(void)offs;

	switch (dev_uart_minor_to_uart(minor)) {
		case 0:  return uart0_read(buff, bufflen, 1);
		case 1:  return uart1_read(buff, bufflen, 1);
		default: return -EINVAL;
	}
}

static int16_t dev_uart_write(uint8_t minor, const void *buff, size_t bufflen, off_t offs)
{
	(void)offs;

	switch (dev_uart_minor_to_uart(minor)) {
		case 0:  return uart0_write(buff, bufflen, 1);
		case 1:  return uart1_write(buff, bufflen, 1);
		default: return -EINVAL;
	}
}

static int8_t dev_uart_sync(uint8_t minor, off_t offs, off_t len)
{
	(void)minor;
	(void)offs;
	(void)len;
	return 0;
}

static int8_t dev_uart_ioctl(uint8_t minor, int16_t op, va_list arg)
{
	/* TODO */
	(void)minor;
	(void)op;
	(void)arg;
	return -ENOSYS;
}

int8_t dev_uart_init(struct fs_ctx *devfs, uint8_t uart, uint16_t baud, uint8_t parity, uint8_t stop)
{
	static const struct dev_ops ops = {
		.read = dev_uart_read,
		.write = dev_uart_write,
		.sync = dev_uart_sync,
		.ioctl = dev_uart_ioctl
	};

	/* TODO baud config */
	(void)baud;
	(void)parity;
	(void)stop;

	uint8_t minor;
	int8_t ret;
	if ((ret = devfs_register(devfs, "UART", &minor, &ops, 0)) < 0) {
		return ret;
	}

	common.minors[uart] = minor;

	return 0;
}
