#include <stdio.h>
#include "putc.h"

int putc(int c, FILE *f)
{
	return do_putc(c, f);
}

