/* ZAK180 Firmaware
 * Kernel printf
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#include <stdint.h>
#include <stdarg.h>

#include "driver/vga.h"
#include "driver/uart.h"
#include "proc/lock.h"

#include "kprintf.h"

/* Lightweight, non-compliant printf
 * Supported formats:
 * %c, %s, %d, %u, %x
 * and %l* when KPRINTF_LONG defined
 */

//#define KPRINTF_LONG

#ifdef KPRINTF_LONG
extern void __ultoa(long val, char *buffer, char base);
extern void __ltoa(long val, char *buffer, char base);
#else
extern void __uitoa(int val, char *buffer, char base);
extern void __lioa(int val, char *buffer, char base);
#endif

static int8_t use_irq;
static struct lock lock;

void kprintf_use_irq(int8_t use)
{
	use_irq = use;
}

static void feed(char **buff, char c)
{
	if (*buff != NULL) {
		*((*buff)++) = c;
	}
	else {
		if (use_irq) {
			uart1_write(&c, 1, 1);
		}
		else {
			uart1_write_poll(&c, 1);
		}

		vga_putchar(c);
	}
}

static void feeds(char **buff, const char *s)
{
	while (*s != '\0') {
		feed(buff, *(s++));
	}
}

static void format_string(char *buff, const char *fmt, va_list ap)
{
	char nbuff[16];

	while (*fmt != '\0') {
		if (*fmt != '%') {
			feed(&buff, *(fmt++));
			continue;
		}

		char format = *(++fmt);
		if (format == '\0') {
			break;
		}
		++fmt;

#ifdef KPRINTF_LONG
		uint8_t is_long = 0;
		if (format == 'l') {
			is_long = 1;
			format = *(fmt++);
		}
#endif

		if (format == 'c') {
			char c = (char)va_arg(ap, int);
			feed(&buff, c);
		}
		else if (format == 's') {
			const char *s = va_arg(ap, char *);
			feeds(&buff, s);
		}
		else if (format == '%') {
			feed(&buff, '%');
		}
		else if (format == 'd' || format == 'u' || format == 'x') {
#ifdef KPRINTF_LONG
			long num = is_long ? va_arg(ap, long) : va_arg(ap, int);
#else
			unsigned num = va_arg(ap, unsigned);
#endif

			uint8_t is_signed = (format == 'd') ? 1 : 0;
			uint8_t base = (format == 'x') ? 16 : 10;

#ifdef KPRINTF_LONG
			if (is_signed) {
				__ltoa(num, nbuff, base);
			}
			else {
				__ultoa(num, nbuff, base);
			}
#else
			if (is_signed) {
				__itoa(num, nbuff, base);
			}
			else {
				__uitoa(num, nbuff, base);
			}
#endif

			feeds(&buff, nbuff);
		}
	}

	if (buff != NULL) {
		feed(&buff, '\0');
	}
}

void kprintf(const char *fmt, ...)
{
	va_list arg;
	lock_lock(&lock);
	va_start(arg, fmt);
	format_string(NULL, fmt, arg);
	va_end(arg);
	lock_unlock(&lock);
}

void ksprintf(char *buff, const char *fmt, ...)
{
	va_list arg;
	va_start(arg, fmt);
	format_string(buff, fmt, arg);
	va_end(arg);
}

