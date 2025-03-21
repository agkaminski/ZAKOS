/* ZAK180 Firmaware
 * devfs
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#ifndef FS_DEVFS_H_
#define FS_DEVFS_H_

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <sys/types.h>

#include "proc/lock.h"

struct fs_ctx;
struct fs_file_op;
struct fs_file;

struct dev_ops {
	int16_t (*read)(uint8_t minor, void *buff, size_t bufflen, uint32_t offs);
	int16_t (*write)(uint8_t minor, const void *buff, size_t bufflen, uint32_t offs);
	int8_t (*sync)(off_t offs, off_t len);
	int8_t (*ioctl)(uint8_t minor, int16_t op, va_list arg);
};

struct devfs_entry;

struct devfs_file {
	struct devfs_entry *entry;
};

struct devfs_ctx {
	struct devfs_entry *entries;
	struct fs_file *parent;
	struct lock lock;
};

extern const struct fs_file_op devfs_ops;

int8_t devfs_register(struct fs_ctx *ctx, const char *name, uint8_t *minor, struct dev_ops *ops, off_t size);

#endif
