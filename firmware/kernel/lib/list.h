/* ZAK180 Firmaware
 * Cyclic list
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#ifndef KERNEL_LIB_LIST_H_
#define KERNEL_LIB_LIST_H_

#include <stdint.h>

#define LIST_OFFSETOFF(type, member) ((uint8_t))(&((type)0)->(member)))

void __list_add(void **queue, void *member, uint8_t next, uint8_t prev);
void __list_remove(void **queue, void *member, uint8_t next, uint8_t prev);

#define LIST_ADD(queue, member, type, next, prev) \
	__list_add(queue, member, offsetof(type, next), offsetof(type, prev))

#define LIST_REMOVE(queue, member, type, next, prev) \
	__list_remove(queue, member, offsetof(type, next), offsetof(type, prev))

#endif
