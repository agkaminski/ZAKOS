#ifndef AIO_IMPL_H
#define AIO_IMPL_H

extern volatile int __aio_fut;

extern int __aio_close(int);
extern void __aio_atfork(int);

#endif
