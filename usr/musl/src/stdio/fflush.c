#include "stdio_impl.h"

/* stdout.c will override this if linked */
static FILE *volatile dummy = 0;

/* This monstrosity needed because of SDCC 4.5.0 derping out:
 * error 9: FATAL Compiler Internal Error in file 'z80/gen.c'
 * line number '6470' : code generator internal error
 */
static void _do_seek(FILE *f)
{
	f->seek(f, f->rpos-f->rend, SEEK_CUR);
}

int fflush(FILE *f)
{
	if (!f) {
		int r = 0;
		if (__stdout_used) r |= fflush(__stdout_used);
		if (__stderr_used) r |= fflush(__stderr_used);

		for (f=*__ofl_lock(); f; f=f->next) {
			FLOCK(f);
			if (f->wpos != f->wbase) r |= fflush(f);
			FUNLOCK(f);
		}
		__ofl_unlock();

		return r;
	}

	FLOCK(f);

	/* If writing, flush output */
	if (f->wpos != f->wbase) {
		f->write(f, 0, 0);
		if (!f->wpos) {
			FUNLOCK(f);
			return EOF;
		}
	}

	/* If reading, sync position, per POSIX */
	if (f->rpos != f->rend) _do_seek(f);

	/* Clear read and write modes */
	f->wpos = f->wbase = f->wend = 0;
	f->rpos = f->rend = 0;

	FUNLOCK(f);
	return 0;
}
