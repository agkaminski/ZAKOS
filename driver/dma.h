/* ZAK180 Firmaware
 * Z180 DMA
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#ifndef DRIVER_DMA_H_
#define DRIVER_DMA_H_

#include <stdint.h>
#include <stddef.h>

void _dma_memcpy(uint8_t dpage, uint16_t doffs, uint8_t spage, uint16_t soffs, size_t len);

void dma_memcpy(uint8_t dpage, uint16_t doffs, uint8_t spage, uint16_t soffs, size_t len);

#endif
