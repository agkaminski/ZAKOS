#include "stdio_impl.h"
#include "aio_impl.h"

static int dummy(int fd)
{
	return fd;
}

int __stdio_close(FILE *f)
{
	return syscall(SYS_close, __aio_close(f->fd));
}
