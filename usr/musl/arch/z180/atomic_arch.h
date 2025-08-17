#define a_cas a_cas
static inline int a_cas(volatile int *p, int t, int s)
{
	__asm di __endasm;
	if (*p == t)
		*p = s;
	else
		t = *p;
	__asm ei __endasm;

	return t;
}
