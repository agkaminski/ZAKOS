/* ZAK180 Firmaware
 * Panic
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#include "hal/cpu.h"
#include "lib/kprintf.h"

void panic() __naked
{
	kprintf("Kernel panic! Halting forever.\r\n");
	_DI;
	while (1) _HALT;
}
