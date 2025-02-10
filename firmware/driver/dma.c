/* ZAK180 Firmaware
 * Z180 DMA
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#include <z180/z180.h>

#include "dma.h"
#include "critical.h"
#include "vga.h"

void _dma_memcpy(uint8_t dpage, uint16_t doffs, uint8_t spage, uint16_t soffs, size_t len)
{
	/* Source */
	SAR0L = soffs;
	SAR0H = (spage << 4) + (soffs >> 8);
	SAR0B = spage >> 4;

	/* Destination */
	DAR0L = doffs;
	DAR0H = (dpage << 4) + (doffs >> 8);
	DAR0B = dpage >> 4;

	/* Mode - memory to memory with increment, burst mode */
	DMODE = 0x02;

	/* Length, 80 bytes */
	BCR0L = len;
	BCR0H = len >> 8;

	/* Start DMA, no interrupt */
	DSTAT = 0x60;
}

void dma_memcpy(uint8_t dpage, uint16_t doffs, uint8_t spage, uint16_t soffs, size_t len)
{
	/* Need to sync with VGA */
	critical_start();
	_dma_memcpy(dpage, doffs, spage, soffs, len);
	_vga_late_irq();
	critical_end();
}
