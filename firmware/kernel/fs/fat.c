/* ZAK180 Firmaware
 * FAT12
 * Copyright: Aleksander Kaminski, 2024-2025
 * See LICENSE.md
 */

#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>

#include "fs/fat.h"
#include "fs/fs.h"
#include "lib/errno.h"
#include "lib/assert.h"
#include "mem/page.h"
#include "driver/mmu.h"

#define FAT12_FAT_SIZE    9
#define FAT12_FAT_COPIES  2
#define FAT12_ROOT_SIZE   14
#define FAT12_DATA_START  (1 + (FAT12_FAT_COPIES * FAT12_FAT_SIZE) + FAT12_ROOT_SIZE)

#define CLUSTER2SECTOR(c) ((c) + FAT12_DATA_START - 2)
#define SECTOR2CLUSTER(s) ((x) + 2 - FAT12_DATA_START)

#define CLUSTER_FREE     0
#define CLUSTER_RESERVED 0xFF0
#define CLUSTER_END      0xFFF

static int8_t fat_op_create(struct fs_file *dir, const char *name, uint8_t attr, uint16_t *idx);
static int16_t fat_op_read(struct fs_file *file, void *buff, size_t bufflen, uint32_t offs);
static int8_t fat_op_truncate(struct fs_file *file, uint32_t size);
static int16_t fat_op_write(struct fs_file *file, const void *buff, size_t bufflen, uint32_t offs);
static int8_t fat_op_readdir(struct fs_file *dir, struct fs_dentry *dentry, union fs_file_internal *file, uint16_t idx);
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

static uint16_t fat_sector_round_down(off_t offs)
{
	return offs / FAT12_SECTOR_SIZE;
}

static uint16_t fat_sector_round_up(off_t offs)
{
	return (offs + FAT12_SECTOR_SIZE - 1) / FAT12_SECTOR_SIZE;
}

static off_t fat_sector_offset(uint16_t sector)
{
	return (off_t)sector * FAT12_SECTOR_SIZE;
}

static int8_t fat_fat_get(struct fs_ctx *ctx, uint16_t n, uint16_t *cluster)
{
	if (CLUSTER2SECTOR(n) >= fat_sector_round_down(ctx->cb->size)) {
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
		idx = 0;
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

static int8_t fat_fat_set(struct fs_ctx *ctx, uint16_t n, uint16_t cluster)
{
	if ((CLUSTER2SECTOR(n) >= fat_sector_round_down(ctx->cb->size)) || (cluster & 0xF000)) {
		return -1;
	}

	uint16_t idx = (3 * n) / 2;
	uint8_t page = (idx > PAGE_SIZE) ? 1 : 0;
	idx %= PAGE_SIZE;
	uint8_t page_prev;
	uint8_t *fat = mmu_map_scratch(ctx->fat.fat_page[page], &page_prev);
	uint16_t buff = cluster;

	if (n & 1) {
		buff = (buff << 4) | (fat[idx] & 0x0F);
	}

	fat[idx] = buff & 0xFF;
	ctx->fat.fat_dirty |= (1 << ((page << 3) + (idx >> 9)));

	if (++idx == PAGE_SIZE) {
		fat = mmu_map_scratch(ctx->fat.fat_page[1], NULL);
		idx = 0;
		ctx->fat.fat_dirty |= (1 << 8);
	}

	if (!(n & 1)) {
		buff |= (uint16_t)(fat[idx] & 0xF0) << 8;
	}

	fat[idx] = buff >> 8;
	(void)mmu_map_scratch(page_prev, NULL);

	idx = (3 * n) / 2;

	return 0;
}

static int8_t fat_fat_sync(struct fs_ctx *ctx)
{
	uint8_t page_prev;
	uint8_t *fat = mmu_map_scratch(ctx->fat.fat_page[0], &page_prev);
	int ret = 0;

	for (uint8_t i = 0; i < FAT12_FAT_SIZE; ++i) {
		if (!ctx->fat.fat_dirty) {
			break;
		}

		if (i == fat_sector_round_down(PAGE_SIZE)) {
			mmu_map_scratch(ctx->fat.fat_page[1], NULL);
		}

		if (ctx->fat.fat_dirty & (1u << i)) {
			uint16_t offset = fat_sector_offset(i % (PAGE_SIZE / FAT12_SECTOR_SIZE));
			ret = ctx->cb->write(fat_sector_offset(i + 1), fat + offset, FAT12_SECTOR_SIZE);
			if (ret < 0) {
				break;
			}

			ret = ctx->cb->write(fat_sector_offset(i + FAT12_FAT_SIZE + 1), fat + offset, FAT12_SECTOR_SIZE);
			if (ret < 0) {
				break;
			}
		}
	}

	(void)mmu_map_scratch(page_prev, NULL);

	if (ret < 0) {
		return ret;
	}

	ctx->fat.fat_dirty = 0;
	return 0;
}

static int8_t fat_fat_fetch(struct fs_ctx *ctx)
{
	uint8_t page_prev;
	void *fat_buff = mmu_map_scratch(ctx->fat.fat_page[0], &page_prev);
	int err = ctx->cb->read(FAT12_SECTOR_SIZE, fat_buff, PAGE_SIZE);
	if (err != PAGE_SIZE) {
		(void)mmu_map_scratch(page_prev, NULL);
		return -EIO;
	}
	fat_buff = mmu_map_scratch(ctx->fat.fat_page[1], NULL);
	err = ctx->cb->read(FAT12_SECTOR_SIZE + PAGE_SIZE, fat_buff, FAT12_SECTOR_SIZE * FAT12_FAT_SIZE - PAGE_SIZE);
	(void)mmu_map_scratch(page_prev, NULL);
	if (err != (FAT12_SECTOR_SIZE * FAT12_FAT_SIZE - PAGE_SIZE)) {
		return -EIO;
	}
	ctx->fat.fat_dirty = 0;
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

static int8_t fat_fat_allocate_cluster(struct fs_ctx *ctx, uint16_t start, uint16_t *new)
{
	uint8_t retry = 0;

	for (*new = start + 1; ; ++(*new)) {
		uint16_t ncluster;
		if (*new == start) {
			/* No space */
			return -ENOSPC;
		}

		if (fat_fat_get(ctx, *new, &ncluster) < 0) {
			if (retry) {
				/* Fail caused by something other than running out of clusters */
				return -EIO;
			}
			/* Try from the FAT start instead */
			retry = 1;
			*new = 0;
		}

		if (ncluster == CLUSTER_FREE) {
			break;
		}
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
		*sector = 1 + (FAT12_FAT_COPIES * FAT12_FAT_SIZE) + fat_sector_round_down(offs);
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

	err = ctx->cb->read(fat_sector_offset(sector) + offset, entry, sizeof(*entry));
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

	err = ctx->cb->write(fat_sector_offset(sector) + offset, entry, sizeof(*entry));
	if (err < 0) {
		return err;
	}

	return 0;
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

static int8_t fat_cluster_clear(struct fs_ctx *ctx, uint16_t cluster, size_t offs, size_t len)
{
	/* Make sure this is on stack, we cannot afford to waste that much of static memory */
	uint8_t zeroes[FAT12_SECTOR_SIZE];
	memset(zeroes, 0, sizeof(zeroes));
	return ctx->cb->write(fat_sector_offset(CLUSTER2SECTOR(cluster)) + offs, zeroes, len) == len ? 0 : -EIO;
}

static int8_t fat_file_trim_chain(struct fs_file *file, struct fat_dentry *dentry, uint16_t length)
{
	uint16_t start = file->file.fat.cluster;
	uint16_t curr = start, last = start;
	uint16_t pos = 0;
	int8_t err;

	if (!start && !length) {
		return 0;
	}

	if (start) {
		for (pos = 1; pos <= length; ++pos) {
			curr = last;
			if (fat_fat_get(file->ctx, curr, &last) < 0) {
				return -EIO;
			}
			if (last == CLUSTER_FREE || last == CLUSTER_RESERVED) {
				return -EIO;
			}
			if (last == CLUSTER_END) {
				break;
			}
		}

		if ((pos == length) && (last == CLUSTER_END)) {
			return 0;
		}

		last = curr;
	}

	if (pos < length) {
		for (; pos < length; ++pos) {
			err = fat_fat_allocate_cluster(file->ctx, last, &curr);
			if (err < 0) {
				return err;
			}

			if (!dentry->cluster) {
				if (dentry != NULL) {
					dentry->cluster = curr;
				}
				file->file.fat.cluster = curr;
			}
			else {
				if (fat_fat_set(file->ctx, last, curr) < 0) {
					return -EIO;
				}
			}

			if (fat_cluster_clear(file->ctx, curr, 0, FAT12_SECTOR_SIZE) < 0) {
				return -EIO;
			}

			last = curr;
		}

		if (fat_fat_set(file->ctx, last, CLUSTER_END) < 0) {
			return -EIO;
		}
	}
	else {
		if (!length) {
			if (dentry != NULL) {
				dentry->cluster = 0;
			}
			file->file.fat.cluster = 0;
		}
		else {
			if (fat_fat_get(file->ctx, last, &curr) < 0) {
				return -EIO;
			}

			if (curr == CLUSTER_FREE || curr == CLUSTER_RESERVED) {
				return -EIO;
			}

			if (fat_fat_set(file->ctx, last, CLUSTER_END) < 0) {
				return -EIO;
			}
		}

		while (curr != CLUSTER_END) {
			if (fat_fat_get(file->ctx, curr, &last) < 0) {
				return -EIO;
			}

			if (last == CLUSTER_FREE || last == CLUSTER_RESERVED) {
				return -EIO;
			}

			if (fat_fat_set(file->ctx, curr, CLUSTER_FREE) < 0) {
				return -EIO;
			}

			curr = last;
		}
	}

	return fat_fat_sync(file->ctx);
}

static int8_t fat_op_create(struct fs_file *dir, const char *name, uint8_t attr, uint16_t *idx)
{
	if (!S_ISREG(attr)) {
		/* Only regular files and directories are supported */
		return -EINVAL;
	}

	struct fat_dentry dentry;

	memset(&dentry, 0, sizeof(dentry));
	memset(dentry.fname, ' ', sizeof(dentry.fname));
	memset(dentry.extension, ' ', sizeof(dentry.extension));

	uint8_t pos = 0;

	for (uint8_t i = 0; i < 8; ++i, ++pos) {
		if (name[pos] == '\0' || name[pos] == '.') {
			break;
		}

		dentry.fname[i] = toupper(name[pos]);
	}

	if (name[pos] == '.') {
		++pos;
		for (uint8_t i = 0; i < 3; ++i, ++pos) {
			if (name[pos] == '\0') {
				break;
			}

			dentry.extension[i] = toupper(name[pos]);
		}
	}

	if (name[pos] != '\0') {
		return -ENAMETOOLONG;
	}

	dentry.attr = fat_attr2fat(attr);

	/* TODO times */

	/* Find removed or free entry */
	uint16_t fidx;
	for (fidx = 0;; ++fidx) {
		struct fat_dentry tentry;
		int8_t err = fat_file_dir_read(dir->ctx, &dir->file.fat, &tentry, fidx);
		if (err == -ENOENT) {
			/* We've run out of dir clusters, extend it */

			if (dir->file.fat.cluster == 0xFFFF) {
				/* Rootdir, it cannot be extended */
				return -ENOSPC;
			}

			err = fat_file_trim_chain(dir, NULL, fat_sector_round_up((fidx + 1) * sizeof(struct fat_dentry)));
			if (err < 0) {
				return err;
			}

			err = fat_file_dir_read(dir->ctx, &dir->file.fat, &tentry, fidx);
		}

		if (err) {
			return err;
		}

		if (tentry.fname[0] == 0x00 || tentry.fname[0] == 0xE5) {
			/* Found valid place */
			break;
		}
	}

	if (idx != NULL) {
		*idx = fidx;
	}

	return fat_file_dir_write(dir->ctx, &dir->file.fat, &dentry, fidx);
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

	if (fat_fat_seek(file->ctx, &file->file.fat, &cluster, offs) != 0) {
		return -EIO; /* EOF is not acceptable - we've checked the size */
	}

	while (1) {
		uint16_t sector = CLUSTER2SECTOR(cluster);
		uint16_t pos = offs % FAT12_SECTOR_SIZE;
		uint16_t chunk = FAT12_SECTOR_SIZE - pos;

		if (chunk > bufflen - len) {
			chunk = bufflen - len;
		}

		int ret = file->ctx->cb->read(fat_sector_offset(sector) + pos, (uint8_t *)buff + len, chunk);
		if (ret < 0) {
			return ret;
		}

		len += chunk;
		offs += chunk;

		if (len >= bufflen) {
			break;
		}

		ret = fat_fat_next(file->ctx, &cluster);
		if (ret > 0) {
			/* EOF */
			break;
		}
		else if (ret < 0) {
			return -EIO;
		}
	}

	return (int16_t)len;
}

static int8_t fat_op_truncate(struct fs_file *file, uint32_t size)
{
	int8_t err;

	if (file->parent == NULL || !S_ISREG(file->attr)) {
		return -EIO;
	}

	if (file->size == size) {
		return 0;
	}

	/* Fetch dentry, we need to modify it afterwards */
	struct fat_dentry dentry;
	if (fat_file_dir_read(file->parent->ctx, &file->parent->file.fat, &dentry, file->file.fat.idx)) {
		return -EIO;
	}

	if (file->size && (file->size < size)) {
		/* Resize up to the current last cluster */
		uint32_t mod = file->size % FAT12_SECTOR_SIZE;
		if (mod) {
			uint32_t cluster_end = FAT12_SECTOR_SIZE - mod;
			if (file->size + cluster_end > size) {
				cluster_end = size - file->size;
			}

			uint16_t cluster;
			err = fat_fat_seek(file->ctx, &file->file.fat, &cluster, file->size - 1);
			if (err != 0) {
				return -EIO;
			}

			if (fat_cluster_clear(file->ctx, cluster, mod, cluster_end) < 0) {
				return -EIO;
			}
		}
	}

	uint16_t clusters_old = fat_sector_round_up(file->size);
	uint16_t clusters_new = fat_sector_round_up(size);

	if (clusters_new != clusters_old) {
		err = fat_file_trim_chain(file, &dentry, clusters_new);
		if (err < 0) {
			fat_fat_fetch(file->ctx);
			return err;
		}
	}

	file->size = size;
	dentry.size = size;

	return fat_file_dir_write(file->parent->ctx, &file->parent->file.fat, &dentry, file->file.fat.idx);
}

static int16_t fat_op_write(struct fs_file *file, const void *buff, size_t bufflen, uint32_t offs)
{
	size_t len = 0;
	int err;
	uint16_t cluster;

	if (file->size < offs + (uint32_t)bufflen) {
		err = fat_op_truncate(file, offs + (uint32_t)bufflen);
		if (err < 0) {
			return err;
		}
	}

	if (fat_fat_seek(file->ctx, &file->file.fat, &cluster, offs) != 0) {
		return -EIO; /* EOF is not acceptable - we've checked the size */
	}

	while (1) {
		uint16_t missalign = offs % (uint32_t)FAT12_SECTOR_SIZE;
		size_t chunk = FAT12_SECTOR_SIZE - missalign;
		if (chunk > bufflen - len) {
			chunk = bufflen - len;
		}

		err = file->ctx->cb->write(fat_sector_offset(CLUSTER2SECTOR(cluster)) + missalign, (uint8_t *)buff + len, chunk);
		if (err < 0) {
			return err;
		}

		offs += chunk;
		len += chunk;

		if (len >= bufflen) {
			break;
		}

		if (fat_fat_next(file->ctx, &cluster) != 0) {
			return -EIO;
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

static int8_t fat_op_readdir(struct fs_file *dir, struct fs_dentry *dentry, union fs_file_internal *file, uint16_t idx)
{
	int err;
	struct fat_dentry fentry;

	err = fat_file_dir_read(dir->ctx, &dir->file.fat, &fentry, idx);
	if (err) {
		return err;
	}

	if (fentry.fname[0] == 0x00) {
		/* Last record reached */
		return -ENOENT;
	}

	if (fentry.fname[0] == 0xE5) {
		/* Deleted entry. Increment idx and try again */
		return -EAGAIN;
	}

	/* Found valid entry */

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
		file->fat.idx = idx;
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
	struct fat_dentry dentry;
	int8_t err = fat_file_dir_read(file->ctx, &file->parent->file.fat, &dentry, file->file.fat.idx);
	if (err) {
		return err;
	}

	/* No directory shrinking (trimming the fat chain)!
	 * Linux vfat does not care, so neither do I (for now)
	 * Just mark as deleted */
	dentry.fname[0] = 0xE5;

	/* Trim file to zero - free all clusters */
	err = fat_file_trim_chain(file, &dentry, 0);
	if (err) {
		return err;
	}

	return fat_file_dir_write(file->ctx, &file->parent->file.fat, &dentry, file->file.fat.idx);
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
	if (fat_fat_fetch(ctx) < 0) {
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
