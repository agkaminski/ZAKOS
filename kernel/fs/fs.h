/* ZAK180 Firmaware
 * Filesystem abstraction layer
 * Copyright: Aleksander Kaminski, 2024-2025
 * See LICENSE.md
 */

#ifndef KERNEL_FS_H_
#define KERNEL_FS_H_

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <time.h>
#include <fcntl.h>

#include "dev/blk.h"
#include "proc/lock.h"
#include "proc/timer.h"
#include "fs/fat.h"
#include "fs/devfs.h"

#include <sys/fs.h>

#define FS_TYPE_FAT   0x01
#define FS_TYPE_DEVFS 0x02

struct fs_file_op;

struct fs_ctx {
	/* FS characteristic context storage */
	union  {
		struct fat_ctx fat;
		struct devfs_ctx devfs;
	};

	struct dev_blk *cb;
	const struct fs_file_op *op;
	uint8_t type;
};

union fs_file_internal {
	struct fat_file fat;
	struct devfs_file devfs;
};

struct fs_file {
	/* File system characteristic data storage */
	union fs_file_internal file;

	/* Directory the file resides in */
	struct fs_file *parent;

	/* Directory mounted at this point, NULL for no mountpoint */
	struct fs_file *mountpoint;

	/* Synchronization */
	int8_t nrefs;
	struct lock lock;

	/* Children */
	struct fs_file *children;
	struct fs_file *chnext;
	struct fs_file *chprev;

	/* File type and permissions */
	uint8_t attr;

	off_t size;

	/* File system context */
	struct fs_ctx *ctx;

	char name[];
};

struct fs_file_op {
	int16_t (*read)(struct fs_file *file, void *buff, size_t bufflen, uint32_t offs);
	int16_t (*write)(struct fs_file *file, const void *buff, size_t bufflen, uint32_t offs);
	int8_t (*create)(struct fs_file *dir, const char *name, uint8_t attr, uint16_t *idx);
	int8_t (*truncate)(struct fs_file *file, uint32_t size);
	int8_t (*readdir)(struct fs_file *dir, struct fs_dentry *dentry, union fs_file_internal *file, uint16_t idx);
	int8_t (*move)(struct fs_file *file, struct fs_file *ndir, const char *name);
	int8_t (*remove)(struct fs_file *file);
	int8_t (*ioctl)(struct fs_file *file, int16_t op, va_list arg);
	int8_t (*mount)(struct fs_ctx *ctx, struct fs_file *dir, struct fs_file *root);
	int8_t (*unmount)(struct fs_ctx *ctx);
};

int8_t fs_open(const char *path, struct fs_file **file, uint8_t mode, uint8_t attr);
void fs_reopen(struct fs_file *file);
int8_t fs_close(struct fs_file *file);
int16_t fs_read(struct fs_file *file, void *buff, size_t bufflen, uint32_t offs);
int16_t fs_write(struct fs_file *file, const void *buff, size_t bufflen, uint32_t offs);
int8_t fs_truncate(struct fs_file *file, uint32_t size);
int8_t fs_readdir(struct fs_file *dir, struct fs_dentry *dentry, uint16_t idx);
int8_t fs_move(struct fs_file *file, struct fs_file *ndir, const char *name);
int8_t fs_remove(const char *path);
int8_t fs_ioctl(struct fs_file *file, int16_t op, ...);
int8_t fs_mount(struct fs_ctx *ctx, const struct fs_file_op *op, struct dev_blk *cb, struct fs_file *dir);
int8_t fs_unmount(struct fs_file *mountpoint);
void fs_init(void);

#endif
