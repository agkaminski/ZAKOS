/* ZAK180 Firmaware
 * File descriptor
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#ifndef KERNEL_PROC_FILE_H_
#define KERNEL_PROC_FILE_H_

#include <stdint.h>
#include <sys/types.h>

#include "fs/fs.h"
#include "proc/lock.h"

struct file_open {
	struct fs_file *file;
	off_t offset;
	uint8_t refs;
	struct lock lock;
};

struct file_descriptor {
	struct file_open *ofile;
	uint8_t flags;
};

void file_fdtable_copy(struct process *parent, struct process *child);
int8_t file_dup2(int8_t oldfd, int8_t newfd);
int8_t file_open(const char *path, uint8_t mode, uint8_t attr);
void file_close_all(struct process *process);
int8_t file_close(int8_t fd);
int16_t file_read(int8_t fd, void *buff, size_t bufflen);
int16_t file_write(int8_t fd, const void *buff, size_t bufflen);
int8_t file_truncate(const char *path, off_t size);
int8_t file_ftruncate(int8_t fd, off_t size);
int8_t file_readdir(int8_t dir, struct fs_dentry *dentry, uint16_t idx);
int8_t file_remove(const char *path);

#endif
