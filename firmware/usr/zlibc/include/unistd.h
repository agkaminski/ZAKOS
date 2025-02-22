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

int close(int8_t fd);
int8_t execv(const char *path, char *const argv[]);
void _exit(int exit);
pid_t fork(void);
int8_t msleep(mseconds_t time);
int read(int8_t fd, void *buff, size_t bufflen);
int write(int8_t fd, const void *buff, size_t bufflen);
int truncate(const char *path, off_t size);
int ftruncate(int8_t fd, off_t size);
int remove(const char *path);

#endif
