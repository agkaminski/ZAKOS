/* ZAK180 User Space App
 * Hello World
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#include <stdio.h>

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

int putchar(int c)
{
	int ret = sys_putc(c); /* workaround buggy sdcc */
	return ret;
}

int main(int argc, char *argv[])
{
	printf("Hello World!\r\n");
	int ret = sys_fork();
	printf("ret = %d\r\n", ret);
	return 0;
}
