/* ZAK180 Zlibc
 * Syscalls prototypes
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#ifndef _ZLIBC_SYSCALL_H_
#define _ZLIBC_SYSCALL_H_

#include <stdint.h>
#include <stddef.h>

int __sys_execv(const char *path, char *const argv[]) __sdcccall(0);
void __sys_exit(int exit) __sdcccall(0);
int __sys_fork(void) __sdcccall(0);
int __sys_putc(int c) __sdcccall(0);
int __sys_msleep(uint32_t mseconds) __sdcccall(0);
int __sys_waitpid(int16_t pid, int *status, int8_t options) __sdcccall(0);
int __sys_write(int fd, void *buff, size_t bufflen) __sdcccall(0);

#endif
