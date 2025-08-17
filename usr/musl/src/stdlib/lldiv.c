#include <stdlib.h>

lldiv_t lldiv(long long num, long long den)
{
	lldiv_t ret = { num/den, num%den };
	return ret;
}
