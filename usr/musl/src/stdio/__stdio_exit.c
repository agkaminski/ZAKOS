#include "stdio_impl.h"

static FILE *volatile dummy_file = 0;

static void do_seek(FILE *f)
{
	f->seek(f, f->rpos-f->rend, SEEK_CUR);
}

static void close_file(FILE *f)
{
	if (!f) return;
	FFINALLOCK(f);
	if (f->wpos != f->wbase) f->write(f, 0, 0);
	if (f->rpos != f->rend) do_seek(f);
}

void __stdio_exit(void)
{
	FILE *f;
	for (f=*__ofl_lock(); f; f=f->next) close_file(f);
	close_file(__stdin_used);
	close_file(__stdout_used);
	close_file(__stderr_used);
}
