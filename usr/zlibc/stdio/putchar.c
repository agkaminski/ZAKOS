/* ZAK180 Zlibc
 * putchar.c
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#include "unistd.h"

int putchar(int c)
{
	char sc = (char)c; /* Make sure the pointer is to the stack */
	return write(STDOUT_FILENO, &sc, 1);
}
