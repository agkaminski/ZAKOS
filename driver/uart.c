/* ZAK180 Firmaware
 * UART driver
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#include <stdint.h>
#include <z180/z180.h>
#include "uart.h"

int uart0_write_poll(const void *buff, size_t bufflen)
{
	for (size_t i = 0; i < bufflen; ++i) {
		while (!(STAT0 & (1 << 1)));
		TDR0 = ((unsigned char *)buff)[i];
	}
	return bufflen;
}

int uart1_write_poll(const void *buff, size_t bufflen)
{
	for (size_t i = 0; i < bufflen; ++i) {
		while (!(STAT1 & (1 << 1)));
		TDR1 = ((unsigned char *)buff)[i];
	}
	return bufflen;
}

void uart_init(void)
{
	/* Both UARTs @19200 baud 8n1 */
	CNTLB0 = 0x01;
	CNTLB1 = 0x01;
	CNTLA0 = 0x64;
	CNTLA1 = 0x64;
}
