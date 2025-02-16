/* ZAK180 User Space App
 * Hello World
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
	printf("Hello World!\r\n");
	int8_t ret = fork();

	if (ret < 0) {
		printf("Fork failed %d\r\n", ret);
	}
	else if (ret > 0) {
		printf("forked\r\n");

		int status = 0;
		ret = waitpid(ret, &status, 0);

		printf("waitpid: %d %d\r\n", ret, status);

		for (;;)
			__asm halt __endasm;
	}

	msleep(1000);

	printf("child is exiting\r\n");

	return 69;
}
