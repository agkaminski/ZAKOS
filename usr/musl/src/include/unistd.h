#ifndef UNISTD_H
#define UNISTD_H

#include "../../include/unistd.h"

extern char **__environ;

int __dup3(int, int, int);
int __mkostemps(char *, int, int);
int __execvpe(const char *, char *const *, char *const *);
off_t __lseek(int, off_t, int);

#endif
