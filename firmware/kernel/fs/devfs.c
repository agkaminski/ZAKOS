/* ZAK180 Firmaware
 * devfs
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#include <stdint.h>
#include <sys/types.h>
#include <stddef.h>
#include <string.h>

#include "fs.h"
#include "devfs.h"

#include "lib/errno.h"
#include "lib/kprintf.h"
#include "lib/assert.h"
#include "mem/kmalloc.h"

struct devfs_entry {
	struct devfs_entry *next;
	const struct dev_ops *ops;
	uint8_t minor;
	off_t size;
	char name[];
};

static int16_t devfs_read(struct fs_file *file, void *buff, size_t bufflen, uint32_t offs);
static int16_t devfs_write(struct fs_file *file, const void *buff, size_t bufflen, uint32_t offs);
static int8_t devfs_create(struct fs_file *dir, const char *name, uint8_t attr, uint16_t *idx);
static int8_t devfs_truncate(struct fs_file *file, uint32_t size);
static int8_t devfs_readdir(struct fs_file *dir, struct fs_dentry *dentry, union fs_file_internal *file, uint16_t idx);
static int8_t devfs_move(struct fs_file *file, struct fs_file *ndir, const char *name);
static int8_t devfs_remove(struct fs_file *file);
static int8_t devfs_ioctl(struct fs_file *file, int16_t op, va_list arg);
static int8_t devfs_mount(struct fs_ctx *ctx, struct fs_file *dir, struct fs_file *root);
static int8_t devfs_unmount(struct fs_ctx *ctx);

const struct fs_file_op devfs_ops = {
	.read = devfs_read,
	.write = devfs_write,
	.create = devfs_create,
	.truncate = devfs_truncate,
	.readdir = devfs_readdir,
	.move = devfs_move,
	.remove = devfs_remove,
	.ioctl = devfs_ioctl,
	.mount = devfs_mount,
	.unmount = devfs_unmount
};

static int16_t devfs_read(struct fs_file *file, void *buff, size_t bufflen, uint32_t offs)
{
	if (file->file.devfs.entry == NULL) {
		return -ENOSYS;
	}

	return file->file.devfs.entry->ops->read(file->file.devfs.entry->minor, buff, bufflen, offs);
}

static int16_t devfs_write(struct fs_file *file, const void *buff, size_t bufflen, uint32_t offs)
{
	if (file->file.devfs.entry == NULL) {
		return -ENOSYS;
	}

	return file->file.devfs.entry->ops->write(file->file.devfs.entry->minor, buff, bufflen, offs);
}

static int8_t devfs_create(struct fs_file *dir, const char *name, uint8_t attr, uint16_t *idx)
{
	/* Not available on this FS */
	(void)dir;
	(void)name;
	(void)attr;
	(void)idx;
	return -ENOSYS;
}

static int8_t devfs_truncate(struct fs_file *file, uint32_t size)
{
	/* Not available on this FS */
	(void)file;
	(void)size;
	return -ENOSYS;
}

static int8_t devfs_readdir(struct fs_file *dir, struct fs_dentry *dentry, union fs_file_internal *file, uint16_t idx)
{
	if (dir->file.devfs.entry != NULL) {
		return -EINVAL;
	}

	memset(dentry, 0, sizeof(*dentry));

	if (idx == 0) {
		strcpy(dentry->name, ".");
		memcpy(file, &dir->file, sizeof(union fs_file_internal));
	}
	else if (idx == 1) {
		strcpy(dentry->name, "..");
		assert(dir->parent != NULL);
		memcpy(file, &dir->parent->file, sizeof(union fs_file_internal));
	}
	else {
		idx -= 2;
		lock_lock(&dir->ctx->devfs.lock);
		struct devfs_entry *entry = dir->ctx->devfs.entries;
		while (idx && entry != NULL) {
			entry = entry->next;
			--idx;
		}

		if (entry == NULL) {
			lock_unlock(&dir->ctx->devfs.lock);
			return -ENOENT;
		}

		ksprintf(dentry->name, "%s%u", entry->name, entry->minor);
		file->devfs.entry = entry;
		lock_unlock(&dir->ctx->devfs.lock);
	}

	return 0;
}

static int8_t devfs_move(struct fs_file *file, struct fs_file *ndir, const char *name)
{
	/* Not available on this FS */
	(void)file;
	(void)ndir;
	(void)name;
	return -ENOSYS;
}

static int8_t devfs_remove(struct fs_file *file)
{
	/* Not available on this FS */
	(void)file;
	return -ENOSYS;
}

static int8_t devfs_ioctl(struct fs_file *file, int16_t op, va_list arg)
{
	if (file->file.devfs.entry->ops->ioctl == NULL) {
		return -ENOSYS;
	}

	return file->file.devfs.entry->ops->ioctl(file->file.devfs.entry->minor, op, arg);
}

static int8_t devfs_mount(struct fs_ctx *ctx, struct fs_file *dir, struct fs_file *root)
{
	ctx->devfs.entries = NULL;
	ctx->devfs.parent = dir;
	root->file.devfs.entry = NULL;
	return 0;
}

static int8_t devfs_unmount(struct fs_ctx *ctx)
{
	while (ctx->devfs.entries != NULL) {
		struct devfs_entry *victim = ctx->devfs.entries;
		ctx->devfs.entries = ctx->devfs.entries->next;
		kfree(victim);
	}

	return 0;
}

int8_t devfs_register(struct fs_ctx *ctx, const char *name, uint8_t *minor, struct dev_ops *ops, off_t size)
{
	lock_lock(&ctx->devfs.lock);
	*minor = 0;
	struct devfs_entry *it = ctx->devfs.entries, *last = NULL;
	while (it != NULL) {
		if (strcmp(name, it->name) == 0) {
			++(*minor);
		}
		last = it;
		it = it->next;
	}

	size_t nlen = strlen(name);
	struct devfs_entry *netry = kmalloc(sizeof(struct devfs_entry) + nlen + 1);
	if (netry == NULL) {
		return -ENOMEM;
	}

	netry->minor = *minor;
	netry->ops = ops;
	netry->size = size;
	netry->next = NULL;
	strcpy(netry->name, name);

	if (last == NULL) {
		ctx->devfs.entries = netry;
	}
	else {
		last->next = netry;
	}
	lock_unlock(&ctx->devfs.lock);

	return 0;
}
