#ifndef LOCK_H
#define LOCK_H

void __lock(volatile int *);
void __unlock(volatile int *);
#define LOCK(x) __lock(x)
#define UNLOCK(x) __unlock(x)

#endif
