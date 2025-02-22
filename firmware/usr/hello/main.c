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

	char path[] = "/BIN/BYE.ZEX";
	char arg0[] = "bye";
	char arg1[] = "arg1";
	char arg2[] = "arg2";
	char arg3[] = "arg3";
	char arg4[] = "arg4";
	char arg5[] = "arg5";
	char *eargv[] = { arg0, arg1, arg2, arg3, arg4, arg5, NULL };

	ret = execv(path, eargv);

	printf("execv: %d\r\n", ret);

	return 69;
}
