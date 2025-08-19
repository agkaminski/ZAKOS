#include <sys/ioctl.h>
#include <stdarg.h>
#include "syscall.h"

#define W 1
#define R 2
#define WR 3

int ioctl(int fd, int req, ...)
{
	void *arg;
	va_list ap;
	va_start(ap, req);
	arg = va_arg(ap, void *);
	va_end(ap);
	return __syscall_ret(__syscall(SYS_ioctl, fd, req, arg));
}
