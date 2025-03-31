/* ZAK180 Firmaware
 * UART driver
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#ifndef DEV_UART_H_
#define DEV_UART_H_

#include <stdint.h>

struct fs_ctx;

int8_t dev_uart_init(struct fs_ctx *devfs, uint8_t uart, uint16_t baud, uint8_t parity, uint8_t stop);

#endif
