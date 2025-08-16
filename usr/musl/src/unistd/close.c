#include <unistd.h>
#include <errno.h>
#include "aio_impl.h"
#include "syscall.h"

int close(int fd)
{
	int r = __syscall_cp(SYS_close, fd);
	if (r == -EINTR) r = 0;
	return __syscall_ret(r);
}
