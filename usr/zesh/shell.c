/* ZAK180 User Space App
 * Z shell
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <unistd.h>

#define LF  0x0a
#define CR  0x0d
#define ESC 0x1b
#define DEL 0x7f

static void process_line(char *line)
{
	printf("line: %s", line);
}

int8_t shell(int argc, char *argv[])
{
	char line[81];
	uint8_t lpos = 0;
	int8_t is_escape = 0;

	while (1) {
		char c, echo = 1;
		read(STDIN_FILENO, &c, 1);

		if (is_escape) {
			echo = 0;
		}
		else if (isprint(c)) {
			if (lpos < sizeof(line) - 1) {
				line[lpos] = c;
				++lpos;
			}
		}
		else if (c == CR) {
			char nl[] = "\r\n";
			write(STDOUT_FILENO, &nl, 2);
			echo = 0;

			line[lpos] = '\0';
			process_line(line);

			lpos = 0;
		}
		else if (c == LF) {
			/* Ignore */
			echo = 0;
		}
		else if (c == DEL) {
			if (lpos) {
				--lpos;
				char bs[] = "\b \b";
				write(STDOUT_FILENO, &bs, 3);
			}
			else {
				echo = 0;
			}
		}
		else if (c == ESC) {
			is_escape = 1;
			echo = 0;
		}

		if (echo) {
			write(STDOUT_FILENO, &c, 1);
		}
	}

	return 0;
}
