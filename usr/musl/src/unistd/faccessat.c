#include <unistd.h>
#include <fcntl.h>
#include "syscall.h"

int faccessat(int fd, const char *filename, int amode, int flag)
{
	if (flag) {
		int ret = __syscall(SYS_faccessat2, fd, filename, amode, flag);
		if (ret != -ENOSYS) return __syscall_ret(ret);
	}

	if (flag & ~AT_EACCESS)
		return __syscall_ret(-EINVAL);

	return syscall(SYS_faccessat, fd, filename, amode);
}
