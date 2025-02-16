/* ZAK180 Zlibc
 * unistd
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#ifndef __ZLIBC_UNISTD_H_
#define __ZLIBC_UNISTD_H_

#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#include <stddef.h>
#include <sys/types.h>

int8_t execv(const char *path, char *const argv[]);
void _exit(int exit);
pid_t fork(void);
int8_t msleep(mseconds_t time);
int write(int fd, void *buff, size_t bufflen);

#endif
