/* ZAK180 Firmaware
 * File system
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#ifndef __ZLIBC_BITS_SYS_FS_H_
#define __ZLIBC_BITS_SYS_FS_H_

#define __FS_NAME_LENGTH_MAX 32

#include <stdint.h>
#include "types.h"
#include "../time.h"

struct fs_dentry {
	uint8_t attr;
	time_t ctime;
	time_t atime;
	time_t mtime;
	off_t size;
	char name[__FS_NAME_LENGTH_MAX];
};
#endif
