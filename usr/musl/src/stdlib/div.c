#include <stdlib.h>

div_t div(int num, int den)
{
	div_t ret = { num/den, num%den };
	return ret;
}
