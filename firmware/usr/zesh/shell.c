/* ZAK180 User Space App
 * Hello World
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

int8_t shell(int argc, char *argv[])
{
	while (1) {
		char c;
		read(STDIN_FILENO, &c, 1);
		printf("0x%02x\r\n", c);
	}

	return 0;
}
