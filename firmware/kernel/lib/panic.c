/* ZAK180 Firmaware
 * Panic
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#include <stdio.h>
#include "hal/cpu.h"

void panic() __naked
{
	printf("Kernel panic! Halting forever.\r\n");
	_DI;
	while (1) _HALT;
}
