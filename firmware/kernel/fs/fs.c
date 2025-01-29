/* ZAK180 Firmaware
 * Filesystem abstraction layer
 * Copyright: Aleksander Kaminski, 2024-2025
 * See LICENSE.md
 */

#include <stdarg.h>
#include <string.h>

#include "fs/fs.h"
#include "proc/lock.h"
#include "lib/errno.h"
#include "lib/assert.h"
#include "lib/list.h"
#include "mem/kmalloc.h"

static struct {
	struct lock lock;
	struct fs_file *root;
} common;

static void fs_file_get(struct fs_file *file)
{
	++file->nrefs;
}

static int8_t fs_file_put(struct fs_file *file)
{
	int8_t ret = 0;

	--file->nrefs;
	assert(file->nrefs >= 0);

	if (!file->nrefs) {
		assert(file->mountpoint == NULL);
		assert(file->children == NULL);

		if (file->parent != NULL) {
			LIST_REMOVE(&file->parent->children, file, struct fs_file, chnext, chprev);
			(void)fs_file_put(file->parent);
		}

		kfree(file);
	}

	return ret;
}

static struct fs_file *fs_file_spawn(const char *name, uint8_t attr)
{
	struct fs_file *file = kmalloc(sizeof(struct fs_file) + strlen(name) + 1);
	if (file != NULL) {
		memset(file, 0, sizeof(*file));
		file->attr = attr;
		file->nrefs = 0;
		strcpy(file->name, name);
		lock_init(&file->lock);
	}
	return file;
}

static int8_t fs_namecmp(const char *path, const char *fname)
{
	for (size_t i = 0; ; ++i) {
		if (path[i] == '/' || path[i] == '\0') {
			if (fname[i] == '\0') {
				return 0;
			}
		}

		if (path[i] != fname[i]) {
			return -1;
		}
	}
}

static int8_t fs_is_tail(const char *path)
{
	for (size_t pos = 0; path[pos] != '\0'; ++pos) {
		if (path[pos] == '/') {
			return 0;
		}
	}

	return 1;
}

static int8_t _fs_open_from_dir(struct fs_file *dir, const char *path, struct fs_file **fnew, uint16_t sidx)
{
	struct fs_dentry dentry;
	union fs_file_internal internal;
	struct fs_file *f;

	for (;; ++sidx) {
		int8_t err = dir->ctx->op->readdir(dir, &dentry, &internal, sidx);
		if (err == -EAGAIN) {
			continue;
		}
		else if (err) {
			return err;
		}

		if (!fs_namecmp(path, dentry.name)) {
			f = fs_file_spawn(dentry.name, dentry.attr);
			if (f == NULL) {
				return -ENOMEM;
			}

			f->parent = dir;
			f->ctx = dir->ctx;
			f->size = dentry.size;
			memcpy(&f->file, &internal, sizeof(internal));

			LIST_ADD(&dir->children, f, struct fs_file, chnext, chprev);
			fs_file_get(dir);

			*fnew = f;

			return 0;
		}
	}
}

static void fs_path_next(const char **path)
{
	while (**path != '/' && **path != '\0') {
		++(*path);
	}
	while (**path == '/') {
		++(*path);
	}
}

static int8_t _fs_lookup(const char **path, struct fs_file **file, struct fs_file **dir)
{
	if (**path != '/') {
		return -EINVAL;
	}

	*dir = common.root;
	*file = NULL;
	if (dir == NULL) {
		return -ENOENT;
	}

	while (**path == '/') {
		++(*path);
	}

	if (**path == '\0') {
		*file = *dir;
		return 0;
	}

	while (*path != '\0') {
		if (!S_ISDIR((*dir)->attr)) {
			return -ENOTDIR;
		}

		if ((*dir)->mountpoint != NULL) {
			(*dir) = (*dir)->mountpoint;
		}

		uint8_t found = 0;

		if ((*dir)->children != NULL) {
			*file = (*dir)->children;

			do {
				if (!fs_namecmp(*path, (*file)->name)) {
					found = 1;
					break;
				}
				*file = (*file)->chnext;
			} while (*file != (*dir)->children);
		}

		if (!found) {
			*file = NULL;
			return -ENOENT;
		}

		*dir = *file;

		fs_path_next(path);
	}

	if (*file != NULL) {
		*dir = (*file)->parent;
	}

	return 0;
}

static int8_t _fs_open(const char *path, struct fs_file **file, uint8_t mode, uint8_t attr)
{
	struct fs_file *dir;
	int8_t err = _fs_lookup(&path, file, &dir);
	if (err != 0) {
		if (err == -EINVAL || dir == NULL) {
			return err;
		}

		err = 0;
		while (!err && *path != '\0') {
			if (!S_ISDIR(dir->attr)) {
				err = -ENOTDIR;
				break;
			}

			if (dir->mountpoint != NULL) {
				dir = dir->mountpoint;
			}

			lock_lock(&dir->lock);
			err = _fs_open_from_dir(dir, path, file, 0);
			if (err == -ENOENT && (mode & O_CREAT) && fs_is_tail(path)) {
				uint16_t idx;
				err = dir->ctx->op->create(dir, path, attr, &idx);
				if (!err) {
					err = _fs_open_from_dir(dir, path, file, idx);
				}
			}
			lock_unlock(&dir->lock);

			dir = *file;

			fs_path_next(&path);
		}
	}

	if (*file != NULL) {
		fs_file_get(*file);
	}
	else {
		err = -ENOENT;
	}

	if (err) {
		/* Clean a dead branch */
		if (*file != NULL) {
			fs_file_put(*file);
		}
	}

	return err;
}

int8_t fs_open(const char *path, struct fs_file **file, uint8_t mode, uint8_t attr)
{
	lock_lock(&common.lock);
	int8_t err = _fs_open(path, file, mode, attr);
	lock_unlock(&common.lock);

	return err;
}

int8_t fs_close(struct fs_file *file)
{
	lock_lock(&common.lock);
	int8_t ret = fs_file_put(file);
	lock_unlock(&common.lock);

	return ret;
}

int16_t fs_read(struct fs_file *file, void *buff, size_t bufflen, uint32_t offs)
{
	lock_lock(&file->lock);
	int16_t ret = file->ctx->op->read(file, buff, bufflen, offs);
	lock_unlock(&file->lock);

	return ret;
}

int16_t fs_write(struct fs_file *file, const void *buff, size_t bufflen, uint32_t offs)
{
	lock_lock(&file->lock);
	int16_t ret = file->ctx->op->write(file, buff, bufflen, offs);
	lock_unlock(&file->lock);

	return ret;
}

int8_t fs_truncate(struct fs_file *file, uint32_t size)
{
	lock_lock(&file->lock);
	int8_t ret = file->ctx->op->truncate(file, size);
	lock_unlock(&file->lock);

	return ret;
}

int8_t fs_readdir(struct fs_file *dir, struct fs_dentry *dentry, uint16_t idx)
{
	if (!S_ISDIR(dir->attr)) {
		return -ENOTDIR;
	}

	lock_lock(&dir->lock);
	int8_t ret = dir->ctx->op->readdir(dir, dentry, NULL, idx);
	lock_unlock(&dir->lock);

	return ret;
}

int8_t fs_move(struct fs_file *file, struct fs_file *ndir, const char *name)
{
}

int8_t fs_remove(const char *path)
{
	struct fs_file *file;

	lock_lock(&common.lock);
	int8_t err = _fs_open(path, &file, O_RDWR, 0);
	if (err < 0) {
		lock_unlock(&common.lock);
		return err;
	}

	if (file->nrefs != 1) {
		(void)fs_file_put(file);
		lock_unlock(&common.lock);
		return -EBUSY;
	}

	err = file->ctx->op->remove(file);
	(void)fs_file_put(file);

	lock_unlock(&common.lock);

	return err;
}

int8_t fs_set_attr(struct fs_file *file, uint8_t attr, uint8_t mask)
{
	lock_lock(&file->lock);
	int8_t ret = file->ctx->op->set_attr(file, attr, mask);

	if (!ret) {
		file->attr = (file->attr & ~mask) | (attr & mask);
	}
	lock_unlock(&file->lock);

	return ret;
}

int8_t fs_ioctl(struct fs_file *file, int16_t op, ...)
{
	if (file->ctx->op->ioctl == NULL) return -ENOSYS;

	va_list args;
	va_start(args, op);
	lock_lock(&file->lock);
	int8_t ret = file->ctx->op->ioctl(file, op, args);
	lock_unlock(&file->lock);
	va_end(args);
	return ret;
}

int8_t fs_mount(struct fs_ctx *ctx, struct fs_file_op *op, struct dev_blk *cb, struct fs_file *dir)
{
	lock_lock(&common.lock);

	if (dir == NULL && common.root != NULL) {
		lock_unlock(&common.lock);
		return -EINVAL;
	}

	struct fs_file *rootdir = fs_file_spawn("", S_IFDIR | S_IR | S_IW);
	if (rootdir == NULL) {
		lock_unlock(&common.lock);
		return -ENOMEM;
	}

	ctx->cb = cb;
	ctx->op = op;

	if (dir != NULL) {
		if (dir->mountpoint != NULL) {
			lock_unlock(&common.lock);
			kfree(rootdir);
			return -EINVAL;
		}
	}

	int8_t ret = ctx->op->mount(ctx, dir, rootdir);
	if (ret < 0) {
		lock_unlock(&common.lock);
		kfree(rootdir);
		return ret;
	}

	rootdir->ctx = ctx;

	if (dir != NULL) {
		dir->mountpoint = rootdir;
		fs_file_get(dir);
	}
	else {
		common.root = rootdir;
	}

	fs_file_get(rootdir);

	lock_unlock(&common.lock);

	return 0;
}

int8_t fs_unmount(struct fs_file *mountpoint)
{
	if (mountpoint->mountpoint == NULL) return -EINVAL;


}

void fs_init(void)
{
	lock_init(&common.lock);
}
