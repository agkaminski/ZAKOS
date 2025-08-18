#include <stdlib.h>

void srand48(long seed)
{
	unsigned short t[] = { 0x330e, seed, seed >> 16 };
	seed48(t);
}
