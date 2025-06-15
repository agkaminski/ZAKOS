/* ZAK180 Zlibc
 * Syscalls prototypes
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#ifndef _ZLIBC_SYSCALL_H_
#define _ZLIBC_SYSCALL_H_

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>

int __sys_execv(const char *path, char *const argv[]) __sdcccall(0);
void __sys_exit(int exit) __sdcccall(0);
int __sys_fork(void) __sdcccall(0);
int __sys_putc(int c) __sdcccall(0);
int __sys_msleep(uint32_t mseconds) __sdcccall(0);
int __sys_waitpid(int16_t pid, int *status, int8_t options) __sdcccall(0);
int __sys_open(const char *path, uint8_t mode, uint8_t attr) __sdcccall(0);
int __sys_close(int8_t fd) __sdcccall(0);
int __sys_read(int8_t fd, void *buff, size_t bufflen) __sdcccall(0);
int __sys_write(int8_t fd, const void *buff, size_t bufflen) __sdcccall(0);
int __sys_truncate(const char *path, off_t size) __sdcccall(0);
int __sys_ftruncate(int8_t fd, off_t size) __sdcccall(0);
int __sys_readdir(int8_t dir, struct fs_dentry *dentry, uint16_t idx) __sdcccall(0);
int __sys_remove(const char *path) __sdcccall(0);
int __sys_dup2(int8_t oldfd, int8_t newfd) __sdcccall(0);

#endif
