#include <sys/statvfs.h>
#include <sys/statfs.h>
#include <string.h>
#include "syscall.h"

int statfs(const char *path, struct statfs *buf)
{
	memset(buf, 0, sizeof(*buf));
#ifdef SYS_statfs64
	return syscall(SYS_statfs64, path, sizeof *buf, buf);
#else
	return syscall(SYS_statfs, path, buf);
#endif
}

int fstatfs(int fd, struct statfs *buf)
{
	memset(buf, 0, sizeof(*buf));
#ifdef SYS_fstatfs64
	return syscall(SYS_fstatfs64, fd, sizeof *buf, buf);
#else
	return syscall(SYS_fstatfs, fd, buf);
#endif
}

static void fixup(struct statvfs *out, const struct statfs *in)
{
	memset(out, 0, sizeof(*out));
	out->f_bsize = in->f_bsize;
	out->f_frsize = in->f_frsize ? in->f_frsize : in->f_bsize;
	out->f_blocks = in->f_blocks;
	out->f_bfree = in->f_bfree;
	out->f_bavail = in->f_bavail;
	out->f_files = in->f_files;
	out->f_ffree = in->f_ffree;
	out->f_favail = in->f_ffree;
	out->f_fsid = in->f_fsid.__val[0];
	out->f_flag = in->f_flags;
	out->f_namemax = in->f_namelen;
	out->f_type = in->f_type;
}

int statvfs(const char *restrict path, struct statvfs *restrict buf)
{
	struct statfs kbuf;
	if (statfs(path, &kbuf)<0) return -1;
	fixup(buf, &kbuf);
	return 0;
}

int fstatvfs(int fd, struct statvfs *buf)
{
	struct statfs kbuf;
	if (fstatfs(fd, &kbuf)<0) return -1;
	fixup(buf, &kbuf);
	return 0;
}
