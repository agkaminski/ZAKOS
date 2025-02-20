/* ZAK180 Firmaware
 * File descriptor
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#include "proc/lock.h"
#include "proc/thread.h"
#include "proc/process.h"
#include "proc/file.h"
#include "mem/kmalloc.h"
#include "lib/assert.h"
#include "lib/errno.h"

#define NELEMS(x) (sizeof(x) / sizeof(*x))

static struct {
	struct lock lock;
} common;

static struct file_open *file_fd_resolve(int8_t fd, uint8_t *flags)
{
	struct process *process = thread_current()->process;
	assert(process != NULL);

	if (fd < 0 || fd > NELEMS(process->fdtable)) {
		return NULL;
	}

	lock_lock(&common.lock);
	lock_lock(&process->lock);
	struct file_descriptor *des = process->fdtable[fd];
	lock_unlock(&process->lock);

	if (des == NULL) {
		lock_unlock(&common.lock);
		return NULL;
	}

	struct file_open *ofile = des->ofile;
	++ofile->refs;
	*flags = des->flags;
	lock_unlock(&common.lock);

	return ofile;
}

static void _file_file_put(struct file_open *ofile)
{
	--ofile->refs;
	if (!ofile->refs) {
		fs_close(ofile->file);
		kfree(ofile);
	}
}

static void file_file_put(struct file_open *ofile)
{
	lock_lock(&common.lock);
	_file_file_put(ofile);
	lock_unlock(&common.lock);
}

int8_t file_open(const char *path, uint8_t mode, uint8_t attr)
{
	struct file_open *ofile = kmalloc(sizeof(struct file_open));
	if (ofile == NULL) {
		return -ENOMEM;
	}

	struct file_descriptor *desc = kmalloc(sizeof(struct file_descriptor));
	if (desc == NULL) {
		kfree(ofile);
		return -ENOMEM;
	}

	ofile->refs = 1;
	ofile->offset = 0;
	lock_init(&ofile->lock);

	int8_t err = fs_open(path, &ofile->file, mode, attr);
	if (err != 0) {
		kfree(ofile);
		return err;
	}

	if (mode & O_TRUNC) {
		/* Kinda oopsie - when we run out of fd table space
		 * we will truncate the file anyway - I choose to ignore this. */
		err = fs_truncate(ofile->file, 0);
		if (err) {
			fs_close(ofile->file);
			kfree(ofile);
			return err;
		}
	}

	if (mode & O_APPEND) {
		ofile->offset = ofile->file->size;
	}

	desc->ofile = ofile;
	desc->flags = mode;

	struct process *process = thread_current()->process;
	assert(process != NULL);

	lock_lock(&common.lock);
	lock_lock(&process->lock);

	for (int8_t fd = 0; fd < NELEMS(process->fdtable); ++fd) {
		if (process->fdtable[fd] == NULL) {
			process->fdtable[fd] = desc;
			lock_unlock(&process->lock);
			lock_unlock(&common.lock);
			return fd;
		}
	}

	lock_unlock(&process->lock);
	lock_unlock(&common.lock);

	fs_close(ofile->file);
	kfree(desc);
	kfree(ofile);

	return -ENFILE;
}

int8_t file_close(int8_t fd)
{
	struct process *process = thread_current()->process;
	assert(process != NULL);

	lock_lock(&common.lock);
	lock_lock(&process->lock);
	struct file_descriptor *desc = process->fdtable[fd];
	if (desc == NULL) {
		lock_unlock(&process->lock);
		lock_unlock(&common.lock);
		return -EBADF;
	}

	_file_file_put(desc->ofile);
	process->fdtable[fd] = NULL;
	kfree(desc);

	lock_unlock(&process->lock);
	lock_unlock(&common.lock);
	return 0;
}

int16_t file_read(int8_t fd, void *buff, size_t bufflen)
{
	uint8_t flags;
	struct file_open *ofile = file_fd_resolve(fd, &flags);
	if (ofile == NULL) {
		return -EBADF;
	}

	if ((flags & O_WRONLY) || S_ISDIR(ofile->file->attr)) {
		file_file_put(ofile);
		return -EINVAL;
	}

	lock_lock(&ofile->lock);
	int16_t ret = fs_read(ofile->file, buff, bufflen, ofile->offset);
	if (ret > 0) {
		ofile->offset += ret;
	}
	lock_unlock(&ofile->lock);

	file_file_put(ofile);

	return ret;
}

int16_t file_write(int8_t fd, const void *buff, size_t bufflen)
{
	uint8_t flags;
	struct file_open *ofile = file_fd_resolve(fd, &flags);
	if (ofile == NULL) {
		return -EBADF;
	}

	if ((flags & O_RDONLY) || S_ISDIR(ofile->file->attr)) {
		file_file_put(ofile);
		return -EINVAL;
	}

	lock_lock(&ofile->lock);
	int16_t ret = fs_write(ofile->file, buff, bufflen, ofile->offset);
	if (ret > 0) {
		ofile->offset += ret;
	}
	lock_unlock(&ofile->lock);

	file_file_put(ofile);

	return ret;
}

int8_t file_truncate(const char *path, off_t size)
{
	struct fs_file *file;
	int8_t ret = fs_open(path, &file, O_WRONLY, 0);
	if (ret < 0) {
		return ret;
	}

	ret = fs_truncate(file, size);
	fs_close(file);

	return ret;
}

int8_t file_ftruncate(int8_t fd, off_t size)
{
	uint8_t flags;
	struct file_open *ofile = file_fd_resolve(fd, &flags);
	if (ofile == NULL) {
		return -EBADF;
	}

	if ((flags & O_RDONLY) || !S_ISREG(ofile->file->attr)) {
		file_file_put(ofile);
		return -EINVAL;
	}

	lock_lock(&ofile->lock);
	int16_t ret = fs_truncate(ofile->file, size);
	if ((ret > 0) && (ofile->offset > size) ) {
		ofile->offset = size;
	}
	lock_unlock(&ofile->lock);

	file_file_put(ofile);

	return ret;
}

int8_t file_readdir(int8_t dir, struct fs_dentry *dentry, uint16_t idx)
{
	uint8_t flags;
	struct file_open *ofile = file_fd_resolve(dir, &flags);
	if (ofile == NULL) {
		return -EBADF;
	}

	if ((flags & O_WRONLY) || !S_ISDIR(ofile->file->attr)) {
		file_file_put(ofile);
		return -EINVAL;
	}

	lock_lock(&ofile->lock);
	int16_t ret = fs_readdir(ofile->file, dentry, idx);
	lock_unlock(&ofile->lock);

	file_file_put(ofile);

	return ret;
}

int8_t file_remove(const char *path)
{
	return fs_remove(path);
}
