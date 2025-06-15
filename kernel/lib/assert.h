/* ZAK180 Firmaware
 * Assert
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#ifndef LIB_ASSERT_H_
#define LIB_ASSERT_H_

#ifndef NDEBUG
void __assert(const char *msg, const char *file, int line);
#define assert(__cond) do { if (!(__cond)) __assert(#__cond, __FILE__, __LINE__); } while (0)
#else
#define assert(__cond) ((void)0)
#endif

#endif
