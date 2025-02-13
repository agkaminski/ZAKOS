/* ZAK180 User Space App
 * Hello World
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#include <stdio.h>
#include <stdint.h>

int data = 5;
int wefwe;

static int sys_putc(int c) __sdcccall(0) __naked
{
	(void)c;

	__asm
		ld a, #0
		rst 0x38
	__endasm;
}

static int sys_fork(void) __sdcccall(0) __naked
{
	__asm
		ld a, #1
		rst 0x38
	__endasm;
}

static int sys_waitpid(int8_t pid, int8_t *status, int64_t timeout) __sdcccall(0) __naked
{
	__asm
		ld a, #2
		rst 0x38
	__endasm;
}

static int sys_usleep(uint32_t useconds) __sdcccall(0) __naked
{
	__asm
		ld a, #4
		rst 0x38
	__endasm;
}

int putchar(int c)
{
	int ret = sys_putc(c); /* workaround buggy sdcc */
	return ret;
}

int main(int argc, char *argv[])
{
	printf("Hello World!\r\n");
	int8_t ret = sys_fork();

	if (ret < 0) {
		printf("Fork failed %d\r\n", ret);
	}
	else if (ret > 0) {
		printf("forked\r\n");

		int8_t status = 0;
		ret = sys_waitpid(ret, &status, 0);

		printf("waitpid: %d %d\r\n", ret, status);

		for (;;)
			__asm halt __endasm;
	}

	sys_usleep(1000);

	printf("child is exiting\r\n");

	return 0;
}
