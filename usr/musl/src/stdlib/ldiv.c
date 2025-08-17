#include <stdlib.h>

ldiv_t ldiv(long num, long den)
{
	ldiv_t ret = { num/den, num%den };
	return ret;
}
