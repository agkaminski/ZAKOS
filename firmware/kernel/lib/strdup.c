/* ZAK180 Firmaware
 * strdup()
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#include <string.h>

#include "strdup.h"

#include "mem/kmalloc.h"

char *strdup(const char *str)
{
	size_t len = strlen(str) + 1;
	char *nstr = kmalloc(len);
	if (nstr != NULL) {
		memcpy(nstr, str, len);
	}
	return nstr;
}
