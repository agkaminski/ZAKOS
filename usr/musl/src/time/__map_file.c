#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include "syscall.h"

const char unsigned *__map_file(const char *pathname, size_t *size)
{
	struct stat st;
	unsigned char *map = NULL;
	int fd = sys_open(pathname, O_RDONLY|O_CLOEXEC|O_NONBLOCK);
	if (fd < 0) return 0;
	if (!__fstat(fd, &st)) {
		*size = st.st_size;
		map = malloc(*size);
		if (map == NULL)
			return NULL;
		size_t done = 0;
		int ret;
		do {
			ret = read(fd, map + done, size - done);
			if (ret < 0 && errno != EINTR)
				return NULL;
			done += ret;
		} while (ret);
	}
	__syscall(SYS_close, fd);
	return map;
}
