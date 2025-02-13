/* ZAK180 Firmaware
 * Assert
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#include "lib/kprintf.h"
#include "lib/panic.h"
#include "hal/cpu.h"

void __assert(const char *msg, const char *file, int line)
{
	_DI;
	kprintf_use_irq(0);
	_kprintf("Assertion '%s' failed in file %s:%d.\r\n", msg, file, line);
	panic();
}
