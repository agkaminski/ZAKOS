#include <utime.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>

int utime(const char *path, const struct utimbuf *times)
{
	struct timespec ts[2] = { { .tv_sec = times->actime}, { .tv_sec = times->modtime } };
	return utimensat(AT_FDCWD, path, times ? ts : 0, 0);
}
