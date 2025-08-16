#ifndef STDLIB_H
#define STDLIB_H

#include "../../include/stdlib.h"

int __putenv(char *, size_t, char *);
void __env_rm_add(char *, char *);
int __mkostemps(char *, int, int);
int __ptsname_r(int, char *, size_t);
char *__randname(char *);
void __qsort_r (void *, size_t, size_t, int (*)(const void *, const void *, void *), void *);

void *__libc_malloc(size_t);
void *__libc_malloc_impl(size_t);
void *__libc_calloc(size_t, size_t);
void *__libc_realloc(void *, size_t);
void __libc_free(void *);

#endif
