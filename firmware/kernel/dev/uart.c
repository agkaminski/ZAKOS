/* ZAK180 Firmaware
 * UART driver
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <sys/types.h>
#include <z180/z180.h>
#include "fs/devfs.h"
#include "driver/critical.h"
#include "hal/cpu.h"
#include "lib/errno.h"
#include "proc/thread.h"

#define FIFO_SIZE 64

struct fifo {
	uint8_t buff[FIFO_SIZE];
	uint8_t rd, wr;
};

static int8_t _fifo_push(struct fifo *fifo, uint8_t c, uint8_t overwrite)
{
	uint8_t nwr = (fifo->wr + 1) % sizeof(fifo->buff);

	if (nwr == fifo->rd) {
		if (overwrite) {
			fifo->rd = (fifo->rd + 1) % sizeof(fifo->buff);
		}
		else {
			return -1;
		}
	}

	fifo->buff[fifo->wr] = c;
	fifo->wr = nwr;

	return 0;
}

static int8_t fifo_push(struct fifo *fifo, uint8_t c)
{
	critical_start();
	int8_t ret = _fifo_push(fifo, c, 0);
	critical_end();

	return ret;
}

static int8_t _fifo_pop(struct fifo *fifo, uint8_t *c)
{
	if (fifo->wr == fifo->rd) {
		return -1;
	}

	*c = fifo->buff[fifo->rd];
	fifo->rd = (fifo->rd + 1) % sizeof(fifo->buff);

	return 0;
}

static int8_t fifo_pop(struct fifo *fifo, uint8_t *c)
{
	critical_start();
	int8_t ret = _fifo_pop(fifo, c);
	critical_end();

	return ret;
}

struct uart_ctx {
	struct fifo tx;
	struct fifo rx;
	uint8_t minor;
	struct thread *rxqueue;
	struct thread *txqueue;
};

static struct uart_ctx uartctx[2];

static uint8_t dev_uart_minor_to_uart(uint8_t minor)
{
	if (uartctx[0].minor == minor) {
		return 0;
	}
	else {
		return 1;
	}
}

static void dev_uart_data_send(uint8_t uart, uint8_t data)
{
	if (!uart)
		TDR0 = data;
	else
		TDR1 = data;
}

static uint8_t dev_uart_data_recv(uint8_t uart)
{
	if (!uart)
		return RDR0;
	else
		return RDR1;
}

static void dev_uart_error_clear(uint8_t uart)
{
	if (!uart)
		CNTLA0 &= ~(1 << 3);
	else
		CNTLA1 &= ~(1 << 3);
}

static uint8_t dev_uart_txready(uint8_t uart)
{
	if (!uart)
		return STAT0 & (1 << 1);
	else
		return STAT1 & (1 << 1);
}

static uint8_t dev_uart_rxready(uint8_t uart)
{
	if (!uart)
		return STAT0 & (1 << 7);
	else
		return STAT1 & (1 << 7);
}

static void dev_uart_txirq_set(uint8_t uart, uint8_t state)
{
	/* rx interrupt stays always enabled */
	uint8_t val = 0x08;
	if (state)
		val = 0x09;

	if (!uart)
		STAT0 = val;
	else
		STAT1 = val;
}

void dev_uart_irq_handler(uint8_t uart)
{
	uint8_t rx = 0, tx = 0;

	while (dev_uart_rxready(uart)) {
		if (!_fifo_push(&uartctx[uart].rx, dev_uart_data_recv(uart), 1)) {
			rx = 1;
		}
	}

	dev_uart_error_clear(uart);

	if (dev_uart_txready(uart)) {
		uint8_t c;
		if (_fifo_pop(&uartctx[uart].tx, &c) != 0) {
			dev_uart_txirq_set(uart, 0);
		}
		else {
			dev_uart_data_send(uart, c);
			tx = 1;
		}
	}

	if (rx)
		_thread_signal_irq(&uartctx[uart].rxqueue);
	if (tx)
		_thread_signal_irq(&uartctx[uart].txqueue);
}

static int16_t dev_uart_read(uint8_t minor, void *buff, size_t bufflen, off_t offs)
{
	(void)offs;

	uint8_t uart = dev_uart_minor_to_uart(minor);
	size_t cnt;

	thread_critical_start();
	for (cnt = 0; cnt < bufflen; ++cnt) {
		while (fifo_pop(&uartctx[uart].rx, (uint8_t *)buff + cnt)) {
			_thread_wait(&uartctx[uart].rxqueue, 0);
		}
	}
	thread_critical_end();

	return cnt;
}

static int16_t dev_uart_write(uint8_t minor, const void *buff, size_t bufflen, off_t offs)
{
	(void)offs;

	uint8_t uart = dev_uart_minor_to_uart(minor);
	size_t cnt;

	thread_critical_start();
	for (cnt = 0; cnt < bufflen; ++cnt) {
		while (fifo_push(&uartctx[uart].tx, ((const uint8_t *)buff)[cnt])) {
			_thread_wait(&uartctx[uart].txqueue, 0);
		}
		dev_uart_txirq_set(uart, 1);
	}
	thread_critical_end();

	return cnt;
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

	/*
	 * Baudrates @CPU clk = 6.144 MHz (12.288 MHz xtal)
	 * DR = /16
	 * /1 38400
	 * /2 19200
	 * /4 9600
	 * /8 4800
	 * /16 2400
	 * /32 1200
	 * /64 600
	 *
	 * DR = /64
	 * /1 9600
	 * /2 4800
	 * /4 2400
	 * /8 1200
	 * /16 600
	 * /32 300
	 * /64 150
	 */

	uint8_t cntlb;
	switch (baud) {
		/* DR = 0 (CLK/16) */
		case 38400: cntlb = 0x00; break;
		case 19200: cntlb = 0x01; break;
		case 9600:  cntlb = 0x02; break;
		case 4800:  cntlb = 0x03; break;
		case 2400:  cntlb = 0x04; break;
		case 1200:  cntlb = 0x05; break;
		case 600:   cntlb = 0x06; break;

		/* DR = 1 (CLK/64) */
		case 300:   cntlb = 0x0D; break;
		case 150:   cntlb = 0x0E; break;

		default: return -EINVAL;
	}

	uint8_t cntla = (1 << 6) | (1 << 5) | (1 << 2); /* TX and RX enable, 8 bits */

	if (parity) {
		cntla |= (1 << 1);

		if (parity & 1) {
			cntlb |= (1 << 4); /* Odd parity, otherwise even */
		}
	}

	if (stop) {
		cntla |= 1;
	}

	if (!uart) {
		CNTLB0 = cntlb;
		CNTLA0 = cntla;

		/* Enable RX interrupt */
		STAT0 = 0x08;
	}
	else {
		CNTLB1 = cntlb;
		CNTLA1 = cntla;

		/* Enable RX interrupt */
		STAT1 = 0x08;
	}

	uint8_t minor;
	int8_t ret;
	if ((ret = devfs_register(devfs, "UART", &minor, &ops, 0)) < 0) {
		return ret;
	}

	uartctx[uart].minor = minor;

	return 0;
}
