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

	char path[] = "/DEV/UART1";
	int fd = open(path, O_RDWR, 0);
	if (fd < 0) {
		printf("failed to open /DEV/UART1 (%d)\r\n", fd);
		for (;;);
	}

	char c;
	while (1) {
		int ret = read(fd, &c, 1);
		if (ret < 0) {
			printf("read fail: %d\r\n", ret);
		}
		else {
			int ret = write(fd, &c, 1);
			if (ret < 0) {
				printf("write fail: %d\r\n", ret);
			}
		}
	}

	return 0;
}
