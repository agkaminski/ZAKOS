#include <inttypes.h>

imaxdiv_t imaxdiv(intmax_t num, intmax_t den)
{
	imaxdiv_t ret = { num/den, num%den };
	return ret;
}
