#include <features.h>

extern volatile int *const __at_quick_exit_lockptr;
extern volatile int *const __atexit_lockptr;
extern volatile int *const __gettext_lockptr;
extern volatile int *const __locale_lockptr;
extern volatile int *const __random_lockptr;
extern volatile int *const __sem_open_lockptr;
extern volatile int *const __stdio_ofl_lockptr;
extern volatile int *const __syslog_lockptr;
extern volatile int *const __timezone_lockptr;

extern volatile int *const __bump_lockptr;

extern volatile int *const __vmlock_lockptr;

void __malloc_atfork(int);
void __ldso_atfork(int);
void __pthread_key_atfork(int);

void __post_Fork(int);
