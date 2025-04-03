/* ZAK180 User Space App
 * Hello World
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
	printf("Hello World!\r\n");

	while (1) {
		char c;
		read(STDIN_FILENO, &c, 1);
		write(STDOUT_FILENO, &c, 1);
	}

	return 0;
}
