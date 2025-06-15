/* ZAK180 Zlibc
 * wait
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#ifndef _ZLIBC_WAIT_H_
#define _ZLIBC_WAIT_H_

#include <sys/types.h>

#define WNOHANG 1

pid_t waitpid(pid_t pid, int *status, int8_t options);

#endif
