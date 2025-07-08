/* ZAK180 User Space App
 * Z shell
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

int main(int argc, char *argv[])
{
	printf("argc: %d, argv[0]: %s\r\n", argc, argv[0]);

	/* Remove extension if present */
	for (uint8_t i = 0; argv[0][i] != '\0'; ++i) {
		if (argv[0][i] == '.') {
			argv[0][i] = '\0';
			break;
		}
	}

	int8_t ret = 0;

	if (strcmp(argv[0], "ZESH") == 0) {
		extern int8_t shell(int argc, char *argv[]);
		ret = shell(argc, argv);
	}
	else {
		/* TODO applets */
	}

	return ret;
}
