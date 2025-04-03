/* ZAK180 User Space App
 * Init process
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

static int8_t tty_open(const char *path)
{
	int8_t fd = open(path, O_RDONLY, 0);
	if (fd < 0) {
		return fd;
	}

	if (fd != STDIN_FILENO) {
		int8_t nfd = dup2(fd, STDIN_FILENO);
		if (nfd != STDIN_FILENO) {
			return -1;
		}
		close(fd);
	}

	fd = open(path, O_WRONLY, 0);
	if (fd < 0) {
		return fd;
	}

	if (fd != STDOUT_FILENO) {
		int8_t nfd = dup2(fd, STDOUT_FILENO);
		if (nfd != STDOUT_FILENO) {
			return -1;
		}
	}

	if (fd != STDERR_FILENO) {
		int8_t nfd = dup2(fd, STDERR_FILENO);
		if (nfd != STDERR_FILENO) {
			return -1;
		}
		if (fd != STDOUT_FILENO) {
			close(fd);
		}
	}

	return 0;
}

static int8_t token_copy(char *dst, const char *src, size_t bufflen)
{
	int8_t pos = 0;

	for (; *src != ' ' && *src != '\0'; ++pos) {
		dst[pos] = *src;
		++src;

		if (pos >= bufflen) {
			return -1;
		}
	}

	dst[pos] = '\0';

	return pos;
}

static int8_t execute(const char *cmd)
{
	char arg[14][32];
	char path[32];
	char *argv[16] = {
		path, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6],
		arg[7], arg[8], arg[9], arg[10], arg[11], arg[12], arg[13], NULL
	};

	int8_t pos = token_copy(path, cmd, sizeof(path));
	if (pos <= 0) {
		return -1;
	}

	cmd += pos;
	while (*cmd == ' ')
		++cmd;

	uint8_t i = 1;
	for (; (i < (sizeof(argv) / sizeof(*argv)) - 1) && (*cmd != '\0'); ++i) {
		pos = token_copy(arg[i], cmd, sizeof(*arg));
		if (pos < 0) {
			return -1;
		}

		cmd += pos;
		while (*cmd == ' ')
			++cmd;
	}

	argv[i] = NULL;

	int8_t pid = fork();
	if (pid < 0) {
		return pid;
	}

	if (!pid) {
		execv(path, argv);
		_exit(1);
	}

	return 0;
}

static int8_t parse_line(const char *line)
{
	switch (*line) {
		case 'X':
			return execute(line + 1);
		case 'T':
			return tty_open(line + 1);
		default:
			return -1;
	}
}

int main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	char script_path[] = "/BOOT/INIT.INI";

	int8_t fd = open(script_path, O_RDONLY, 0);
	if (fd < 0) {
		return 1;
	}

	char script[512];
	int16_t slen = 0;
	int16_t ret;
	while ((ret = read(fd, script + slen, sizeof(script) - slen)) != 0) {
		if (ret < 0) {
			return 1;
		}

		slen += ret;
		if (slen >= sizeof(script)) {
			return 1;
		}
	}

	char line[64];
	size_t lpos = 0;

	for (size_t pos = 0; pos < slen; ++pos) {
		if (script[pos] == '\n' || script[pos] == '\r') {
			if (lpos) {
				line[lpos] = '\0';
				lpos = 0;

				if (parse_line(line) < 0) {
					return 1;
				}
			}

			continue;
		}

		line[lpos++] = script[pos];

		if (lpos >= sizeof(line)) {
			return 1;
		}
	}

	(void)close(STDIN_FILENO);
	(void)close(STDOUT_FILENO);
	(void)close(STDERR_FILENO);

	while (waitpid(-1, NULL, 0) >= 0);

	return 0;
}
