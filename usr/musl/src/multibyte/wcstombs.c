#include <stdlib.h>
#include <wchar.h>

size_t wcstombs(char *restrict s, const wchar_t *restrict ws, size_t n)
{
	const wchar_t *t = ws;
	return wcsrtombs(s, &t, n, 0);
}
