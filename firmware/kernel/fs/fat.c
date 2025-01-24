/* ZAK180 Firmaware
 * FAT12
 * Copyright: Aleksander Kaminski, 2024-2025
 * See LICENSE.md
 */

#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include "fs/fat.h"
#include "fs/fs.h"
#include "lib/errno.h"
#include "lib/assert.h"
#include "mem/page.h"
#include "driver/mmu.h"
#include "include/fcntl.h"

#define FAT12_FAT_SIZE    9
#define FAT12_FAT_COPIES  2
#define FAT12_ROOT_SIZE   14
#define FAT12_DATA_START  (1 + (FAT12_FAT_COPIES * FAT12_FAT_SIZE) + FAT12_ROOT_SIZE)

#define CLUSTER2SECTOR(c) ((c) + FAT12_DATA_START - 2)
#define SECTOR2CLUSTER(s) ((x) + 2 - FAT12_DATA_START)

#define CLUSTER_FREE     0
#define CLUSTER_RESERVED 0xFF0
#define CLUSTER_END      0xFFF

static int8_t fat_op_create(struct fs_file *dir, const char name, uint8_t attr, union fs_file_internal *file, uint16_t *idx);
static int16_t fat_op_read(struct fs_file *file, void *buff, size_t bufflen, uint32_t offs);
static int8_t fat_op_truncate(struct fs_file *file, uint32_t size);
static int16_t fat_op_write(struct fs_file *file, const void *buff, size_t bufflen, uint32_t offs);
static int8_t fat_op_readdir(struct fs_file *dir, struct fs_dentry *dentry, union fs_file_internal *file, uint16_t *idx);
static int8_t fat_op_move(struct fs_file *file, struct fs_file *ndir, const char *name);
static int8_t fat_op_remove(struct fs_file *file);
static int8_t fat_op_set_attr(struct fs_file *file, uint8_t attr, uint8_t mask);
static int8_t fat_op_mount(struct fs_ctx *ctx, struct fs_file *dir, struct fs_file *root);
static int8_t fat_op_unmount(struct fs_ctx *ctx);

const struct fs_file_op fat_op = {
	.create = fat_op_create,
	.read = fat_op_read,
	.write = fat_op_write,
	.truncate = fat_op_truncate,
	.readdir = fat_op_readdir,
	.move = fat_op_move,
	.remove = fat_op_remove,
	.set_attr = fat_op_set_attr,
	.ioctl = NULL,
	.mount = fat_op_mount,
	.unmount = fat_op_unmount
};

static int8_t fat_fat_get(struct fs_ctx *ctx, uint16_t n, uint16_t *cluster)
{
	if (CLUSTER2SECTOR(n) >= (ctx->cb->size / FAT12_SECTOR_SIZE)) {
		return -EIO;
	}

	uint16_t idx = (3 * n) / 2;
	uint8_t page = (idx < PAGE_SIZE) ? 0 : 1;
	idx %= PAGE_SIZE;
	uint8_t page_prev;
	const uint8_t *fat = mmu_map_scratch(ctx->fat.fat_page[page], &page_prev);

	*cluster = fat[idx];
	if (n & 1) {
		*cluster >>= 4;
	}

	if (++idx == PAGE_SIZE) {
		fat = mmu_map_scratch(ctx->fat.fat_page[1], NULL);
	}

	if (n & 1) {
		*cluster |= (uint16_t)fat[idx] << 4;
	}
	else {
		*cluster |= (uint16_t)(fat[idx] & 0x0F) << 8;
	}

	(void)mmu_map_scratch(page_prev, NULL);

	if (*cluster >= 0xFF8) {
		*cluster = CLUSTER_END;
	}
	else if (*cluster >= 0xFF0) {
		*cluster = CLUSTER_RESERVED;
	}

	return 0;
}

static int8_t fat_fat_set(struct fs_ctx *ctx,  uint16_t n, uint16_t cluster)
{
	if ((CLUSTER2SECTOR(n) >= (ctx->cb->size / FAT12_SECTOR_SIZE)) || (cluster & 0xF000)) {
		return -1;
	}

	uint16_t idx = (3 * n) / 2;
	uint8_t page = (idx > PAGE_SIZE) ? 1 : 0;
	idx %= PAGE_SIZE;
	uint8_t page_prev;
	uint8_t *fat = mmu_map_scratch(ctx->fat.fat_page[page], &page_prev);
	uint8_t low = cluster & 0x0F, high = cluster >> 8;
	uint16_t buff;

	if (n & 1) {
		buff = (fat[idx] & 0xF0) | ((low & 0x0F) << 4);
	}
	else {
		buff = low;
	}

	fat[idx] = buff & 0xFF;

	if (++idx == PAGE_SIZE) {
		fat = mmu_map_scratch(ctx->fat.fat_page[1], NULL);
	}

	if (n & 1) {
		buff |= ((uint16_t)high << 12) | ((uint16_t)(low & 0xF0) << 4);
	}
	else {
		buff |= ((uint16_t)high << 8) | (((uint16_t)fat[idx] & 0xF0) << 8);
	}

	fat[idx] = buff >> 8;
	(void)mmu_map_scratch(page_prev, NULL);

	idx = (3 * n) / 2;

	int ret = ctx->cb->write(FAT12_SECTOR_SIZE + idx, &buff, sizeof(buff));
	if (ret < 0) {
		return ret;
	}

	/* Need to update FAT copy too */
	ret = ctx->cb->write(FAT12_SECTOR_SIZE + (FAT12_FAT_SIZE * FAT12_SECTOR_SIZE) + idx, &buff, sizeof(buff));
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int8_t fat_fat_next(struct fs_ctx *ctx, uint16_t *cluster)
{
	if (fat_fat_get(ctx, *cluster, cluster) < 0) {
		return -EIO;
	}

	if (*cluster == CLUSTER_FREE || *cluster == CLUSTER_RESERVED) {
		return -EIO;
	}

	if (*cluster == CLUSTER_END) {
		return 1; /* EOF */
	}

	return 0;
}

static int8_t fat_fat_seek(struct fs_ctx *ctx, struct fat_file *file, uint16_t *cluster, uint32_t offs)
{
	*cluster = file->cluster;

	while (offs >= FAT12_SECTOR_SIZE) {
		int8_t ret = fat_fat_next(ctx, cluster);
		if (ret != 0) {
			return ret;
		}
		offs -= FAT12_SECTOR_SIZE;
	}

	return 0;
}

static int8_t fat_file_dir_access(struct fs_ctx *ctx, struct fat_file *dir, uint16_t idx, uint16_t *sector, uint16_t *offset)
{
	size_t len = 0;
	uint32_t offs = (uint32_t)idx * sizeof(struct fat_dentry);
	int err;

	/* Root dir has to be processed separatelly, as it has negative cluster index */
	if (dir->cluster == 0xffff) {
		*sector = 1 + (FAT12_FAT_COPIES * FAT12_FAT_SIZE) + (offs / FAT12_SECTOR_SIZE);
	}
	else {
		uint16_t cluster;
		err = fat_fat_seek(ctx, dir, &cluster, offs);
		if (err < 0) {
			return err;
		}

		if (err > 0) {
			/* EOF */
			return -ENOENT;
		}

		*sector = CLUSTER2SECTOR(cluster);
	}

	*offset = offs % FAT12_SECTOR_SIZE;

	return 0;
}

static int8_t fat_file_dir_read(struct fs_ctx *ctx, struct fat_file *dir, struct fat_dentry *entry, uint16_t idx)
{
	uint16_t sector, offset;
	int8_t err = fat_file_dir_access(ctx, dir, idx, &sector, &offset);
	if (err < 0) {
		return err;
	}

	err = ctx->cb->read(FAT12_SECTOR_SIZE * (off_t)sector + offset, entry, sizeof(*entry));
	if (err < 0) {
		return err;
	}

	return 0;
}

static int8_t fat_file_dir_write(struct fs_ctx *ctx, struct fat_file *dir, const struct fat_dentry *entry, uint16_t idx)
{
	uint16_t sector, offset;
	int8_t err = fat_file_dir_access(ctx, dir, idx, &sector, &offset);
	if (err < 0) {
		return err;
	}

	err = ctx->cb->write(FAT12_SECTOR_SIZE * (off_t)sector + offset, entry, sizeof(*entry));
	if (err < 0) {
		return err;
	}

	return 0;
}

static uint8_t fat_file_namelen(const char *str, uint8_t max)
{
	uint8_t pos = max - 1;

	while (pos != 0 && str[pos] == ' ') {
		--pos;
	}

	if (str[pos] == ' ') {
		return 0;
	}

	return pos + 1;
}

static int8_t fat_file_name_cmp(const struct fat_dentry *entry, const char *path)
{
	uint8_t i, pos;
	char c;
	uint8_t len = fat_file_namelen(entry->fname, sizeof(entry->fname));

	for (i = 0; i < len; ++i) {
		c = toupper(path[i]);
		if (c == '/' || c == '\0') {
			return 1;
		}

		if (entry->fname[i] != c) {
			return 1;
		}
	}

	len = fat_file_namelen(entry->extension, sizeof(entry->extension));

	pos = i;
	if (path[i] == '.') {
		++pos;
	}
	else {
		return ((path[i] != '/' && path[i] != '\0') || len != 0) ? 1 : 0;
	}

	for (i = 0; i < len; ++i) {
		c = toupper(path[pos + i]);
		if (c == '/' || c == '\0') {
			return 1;
		}

		if (entry->extension[i] != c) {
			return 1;
		}
	}

	return (path[pos + i] != '/' && path[pos + i] != '\0') ? 1 : 0;
}

static uint8_t fat_attr2fat(uint8_t attr)
{
	uint8_t fattr = 0;

	/* Read-only */
	if (!(attr & S_IW)) {
		fattr |= (1 << 0);
	}

	/* Subdirectory */
	if (attr & S_IFDIR) {
		fattr |= (1 << 4);
	}

	/* Other flags does not make sense for the usecase */
	return fattr;
}

static uint8_t fat_fat2attr(uint8_t fattr)
{
	/* By default allow execution */
	uint8_t attr = S_IX | S_IR;

	/* Read-only */
	if (!(fattr & (1 << 0))) {
		attr |= S_IW;
	}

	/* Subdirectory */
	if (fattr & (1 << 4)) {
		attr |= S_IFDIR;
	}
	else {
		attr |= S_IFREG;
	}

	/* No special files on FAT (TODO unused fields?) */
	return attr;
}

static int8_t fat_op_create(struct fs_file *dir, const char name, uint8_t attr, union fs_file_internal *file, uint16_t *idx)
{

}

static int16_t fat_op_read(struct fs_file *file, void *buff, size_t bufflen, uint32_t offs)
{
	uint16_t cluster;
	size_t len = 0;

	if (offs >= file->size) {
		return 0;
	}
	else if (offs + (uint32_t)bufflen > file->size) {
		bufflen = file->size - offs;
	}
	if (bufflen > 0x7FFF) {
		/* Reval has to fit in int (int16_t) */
		bufflen = 0x7FFF;
	}

	int8_t ret = fat_fat_seek(file->ctx, &file->file.fat, &cluster, offs);
	if (ret != 0) {
		return -EIO; /* EOF is not acceptable - we've checked the size */
	}

	while (1) {
		uint16_t sector = CLUSTER2SECTOR(cluster);
		uint16_t pos = offs % FAT12_SECTOR_SIZE;
		uint16_t chunk = FAT12_SECTOR_SIZE - pos;

		if (chunk > bufflen - len) {
			chunk = bufflen - len;
		}

		int ret = file->ctx->cb->read(FAT12_SECTOR_SIZE * (off_t)sector + pos, (uint8_t *)buff + len, chunk);
		if (ret < 0) {
			return ret;
		}

		len += chunk;
		offs += chunk;

		if (len < bufflen) {
			ret = fat_fat_next(file->ctx, &cluster);
			if (ret > 0) {
				/* EOF */
				break;
			}
			else if (ret < 0) {
				return -EIO;
			}
		}
		else {
			break;
		}
	}

	return (int16_t)len;
}

static int8_t fat_op_truncate(struct fs_file *file, uint32_t size)
{
	uint32_t old_size = file->size;
	int err;
	static const uint8_t zeroes[FAT12_SECTOR_SIZE] = { 0 };
	uint16_t cluster;

	if (file->size == size) {
		return 0;
	}

	if ((file->size / (uint32_t)FAT12_SECTOR_SIZE) == (size / (uint32_t)FAT12_SECTOR_SIZE)) {
		/* No need to add/remove clusters, just init the space */
		if (file->size < size) {
			err = fat_fat_seek(file->ctx, &file->file.fat, &cluster, file->size - 1);
			if (err != 0) {
				return -EIO;
			}

			uint16_t chunk = size - file->size;
			uint16_t offs = file->size % FAT12_SECTOR_SIZE;

			err = file->ctx->cb->write(CLUSTER2SECTOR(cluster) * (off_t)FAT12_SECTOR_SIZE + offs, zeroes, chunk);
			if (err < 0) {
				return err;
			}
		}
	}
	else if (file->size < size) {
		err = fat_fat_seek(file->ctx, &file->file.fat, &cluster, file->size - 1);
		if (err != 0) {
			return -EIO;
		}

		/* Try to find new cluster after the current last */
		while (file->size < size) {
			uint16_t ncluster, ccluster = cluster + 1;
			uint8_t retry = 0;

			for (ccluster = cluster+ 1; ; ++ccluster) {
				if (fat_fat_get(file->ctx, ccluster, &ncluster) < 0) {
					if (retry) {
						/* No space? Restore previous file and FAT state */
						(void)fat_op_truncate(file, old_size);
						return -ENOSPC;
					}
					else {
						/* Try again from the FAT start */
						retry = 1;
						ccluster = 0;
					}
				}

				if (retry && (ccluster == cluster)) {
					return -ENOSPC;
				}

				if (ncluster == CLUSTER_FREE) {
					break;
				}
			}

			/* Add a new cluster to the chain */
			if (fat_fat_set(file->ctx, cluster, ccluster) < 0) {
				(void)fat_op_truncate(file, old_size);
				return -EIO;
			}

			if (fat_fat_set(file->ctx, ccluster, CLUSTER_END) < 0) {
				(void)fat_op_truncate(file, old_size);
				return -EIO;
			}

			/* Zero-out new cluster */
			err = file->ctx->cb->write(CLUSTER2SECTOR(ccluster) * (off_t)FAT12_SECTOR_SIZE, zeroes, FAT12_SECTOR_SIZE);
			if (err < 0) {
				(void)fat_op_truncate(file, old_size);
				return -EIO;
			}

			file->size += FAT12_SECTOR_SIZE;
			cluster = ccluster;
		}
	}
	else {
		uint16_t ccluster = file->file.fat.cluster;
		uint16_t prev = ccluster;
		for (uint32_t csize = 0; csize < size; csize += FAT12_SECTOR_SIZE) {
			prev = ccluster;
			if (fat_fat_get(file->ctx, prev, &ccluster) < 0) {
				return -EIO;
			}

			if (ccluster == CLUSTER_FREE || ccluster == CLUSTER_RESERVED) {
				return -EIO;
			}
		}

		if (ccluster != CLUSTER_END) {
			/* ccluster have the rest of the chain, prev is new end */
			fat_fat_set(file->ctx, prev, CLUSTER_END);

			while (ccluster != CLUSTER_END) {
				prev = ccluster;
				if (fat_fat_get(file->ctx, prev, &ccluster) < 0) {
					/* We have leaked clusters and can't do anything about it */
					return -EIO;
				}
				if (fat_fat_set(file->ctx, prev, CLUSTER_FREE) < 0) {
					return -EIO;
				}
			}
		}
	}

	file->size = size;

	/* TODO update dentry */
	//return fat_file_dir_write(file->ctx, &file->parent->file.fat, file->file.fat.dentry, file->file.fat.idx);
}

static int16_t fat_op_write(struct fs_file *file, const void *buff, size_t bufflen, uint32_t offs)
{
	size_t len = 0;
	int8_t err;
	uint16_t cluster;

	if (file->size < offs + (uint32_t)bufflen) {
		err = fat_op_truncate(file, offs + (uint32_t)bufflen);
		if (err < 0) {
			return err;
		}
	}

	int8_t ret = fat_fat_seek(file->ctx, &file->file.fat, &cluster, offs);
	if (ret != 0) {
		return -EIO; /* EOF is not acceptable - we've checked the size */
	}

	while (1) {
		uint16_t missalign = offs % (uint32_t)FAT12_SECTOR_SIZE;
		size_t chunk = FAT12_SECTOR_SIZE - missalign;
		if (chunk > bufflen - len) {
			chunk = bufflen - len;
		}

		err = file->ctx->cb->write(CLUSTER2SECTOR(cluster) * (off_t)FAT12_SECTOR_SIZE + missalign, (uint8_t *)buff + len, chunk);
		if (err < 0) {
			return err;
		}

		offs += chunk;
		len += chunk;

		if (len < bufflen) {
			if (fat_fat_next(file->ctx, &cluster) != 0) {
				return -EIO;
			}
		}
		else {
			break;
		}
	}

	return len;
}

static int32_t fat_attr2epoch(uint16_t date, uint16_t time)
{
	/* TODO */
	(void)date;
	(void)time;
	return 0;
}

static int8_t fat_op_readdir(struct fs_file *dir, struct fs_dentry *dentry, union fs_file_internal *file, uint16_t *idx)
{
	int err;
	struct fat_dentry fentry;

	while (1) {
		err = fat_file_dir_read(dir->ctx, &dir->file.fat, &fentry, *idx);
		if (err) {
			return err;
		}

		if (fentry.fname[0] == 0x00) {
			/* Last record reached */
			err = -ENOENT;
			break;
		}

		++(*idx);

		if (fentry.fname[0] != 0xE5) {
			break;
		}

		/* Deleted entry */
	}

	dentry->atime = fat_attr2epoch(fentry.adate, 0);
	dentry->ctime = fat_attr2epoch(fentry.cdate, fentry.ctime);
	dentry->mtime = fat_attr2epoch(fentry.mdate, fentry.mtime);

	memcpy(dentry->name, fentry.fname, sizeof(fentry.fname));
	uint8_t pos = sizeof(fentry.fname) - 1;
	while (dentry->name[pos] == ' ') {
		if (!pos) {
			return -EIO;
		}
		--pos;
	}

	uint8_t ppos = ++pos;
	dentry->name[pos] = ' ';
	memcpy(dentry->name + pos + 1, fentry.extension, sizeof(fentry.extension));
	pos += sizeof(fentry.extension);
	while (dentry->name[pos] == ' ') {
		--pos;
	}

	if (ppos != pos) {
		dentry->name[ppos] = '.';
	}

	dentry->name[pos + 1] = '\0';
	dentry->attr = fat_fat2attr(fentry.attr);
	dentry->size = fentry.size;

	if (file != NULL) {
		file->fat.cluster = fentry.cluster;
		file->fat.idx = *idx - 1;
	}

	return err;
}

static int8_t fat_op_move(struct fs_file *file, struct fs_file *ndir, const char *name)
{
	/* TODO */
	(void)file;
	(void)ndir;
	(void)name;
	return -ENOSYS;
}

static int8_t fat_op_remove(struct fs_file *file)
{
	/* TODO */
	(void)file;
	return -ENOSYS;
}

static int8_t fat_op_set_attr(struct fs_file *file, uint8_t attr, uint8_t mask)
{
	/* TODO */
	(void)file;
	(void)attr;
	(void)mask;
	return -ENOSYS;
}

static int8_t fat_op_mount(struct fs_ctx *ctx, struct fs_file *dir, struct fs_file *root)
{
	int err;
	struct {
		uint16_t bytes_per_sector;
		uint8_t sectors_per_cluster;
		uint16_t reserved_sectors;
		uint8_t number_of_fats;
		uint16_t rootdir_size;
		uint16_t total_sector_count;
		uint8_t media_descriptor;
		uint16_t sectors_per_fat;
		uint16_t sectors_per_track;
		uint16_t number_of_heads;
	} bpb;

	/* Read and parse BIOS parameter block */
	err = ctx->cb->read(0x0B, &bpb, sizeof(bpb));
	if (err < 0) {
		return err;
	}

	/* Check if the filesystem is present and
	 * what we expect it to be. FAT12 is LE,
	 * Z180 is LE too, ignore endianess. */

	if ((bpb.bytes_per_sector != FAT12_SECTOR_SIZE) ||
		(bpb.sectors_per_cluster != 1) ||
		(bpb.reserved_sectors != 1) ||
		(bpb.number_of_fats != 2) ||
		(bpb.rootdir_size != 224) ||
		(bpb.sectors_per_fat != 9) ||
		(bpb.sectors_per_track != 18) ||
		(bpb.number_of_heads != 2) ||
		(bpb.total_sector_count != (2 * 18 * 80))) {
		return -EIO;
	}

	/* Alloc FAT cache */
	ctx->fat.fat_page[0] = page_alloc(NULL, 1);
	if (ctx->fat.fat_page[0] == 0) {
		return -ENOMEM;
	}
	ctx->fat.fat_page[1] = page_alloc(NULL, 1);
	if (ctx->fat.fat_page[1] == 0) {
		page_free(ctx->fat.fat_page[0], 1);
		return -ENOMEM;
	}

	/* Populate FAT cache */
	assert(PAGE_SIZE < (FAT12_SECTOR_SIZE * FAT12_FAT_SIZE));
	uint8_t page_prev;
	void *fat_buff = mmu_map_scratch(ctx->fat.fat_page[0], &page_prev);
	err = ctx->cb->read(FAT12_SECTOR_SIZE, fat_buff, PAGE_SIZE);
	if (err != PAGE_SIZE) {
		(void)mmu_map_scratch(page_prev, NULL);
		page_free(ctx->fat.fat_page[0], 1);
		page_free(ctx->fat.fat_page[1], 1);
		return -EIO;
	}
	fat_buff = mmu_map_scratch(ctx->fat.fat_page[1], NULL);
	err = ctx->cb->read(FAT12_SECTOR_SIZE + PAGE_SIZE, fat_buff, FAT12_SECTOR_SIZE * FAT12_FAT_SIZE - PAGE_SIZE);
	(void)mmu_map_scratch(page_prev, NULL);
	if (err != (FAT12_SECTOR_SIZE * FAT12_FAT_SIZE - PAGE_SIZE)) {
		page_free(ctx->fat.fat_page[0], 1);
		page_free(ctx->fat.fat_page[1], 1);
		return -EIO;
	}

	root->file.fat.idx = 0;
	root->file.fat.cluster = 0xFFFF; /* Special rootdir marker */

	/* FAT12 does not have physical . and .. entries in rootdir */
	(void)dir;

	ctx->type = FS_TYPE_FAT;

	return 0;
}

static int8_t fat_op_unmount(struct fs_ctx *ctx)
{
	ctx->cb->sync(0, 0);
	return 0;
}
