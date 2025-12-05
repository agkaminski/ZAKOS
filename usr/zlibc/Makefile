TARGET = zlibc

CC = $(shell if `which sdcc-sdcc > /dev/null 2>&1`; then echo "sdcc-sdcc"; else echo "sdcc"; fi)
AS = $(shell if `which sdcc-sdasz80 > /dev/null 2>&1`; then echo "sdcc-sdasz80"; else echo "sdasz80"; fi)
AR = $(shell if `which sdcc-sdar > /dev/null 2>&1`; then echo "sdcc-sdar"; else echo "sdar"; fi)

CFLAGS = -mz180 --opt-code-size --max-allocs-per-node 10000 \
  -I include

SRC =  unistd/exit.c unistd/fork.c unistd/msleep.c unistd/write.c unistd/execv.c
SRC += unistd/close.c unistd/ftruncate.c unistd/truncate.c unistd/read.c unistd/dup.c
SRC += fcntl/open.c
SRC += wait/waitpid.c
SRC += stdio/putchar.c

OBJ = syscall.rel $(SRC:.c=.rel)
TRASH = *.bin *.lk *.map *.mem *.lst *.rel *.rst *.sym *.asm *.ihx *.noi *.hex *.lib

.PHONY: clean

all: ${TARGET}.lib

${TARGET}.lib: ${OBJ}
	@$(AR) -q "$@" $^

%.rel: %.c
	$(CC) ${CFLAGS} -o "$@" -c "$<"

%.rel: %.s
	$(AS) -l -o -s "$<"

clean:
	@rm -f ${TRASH}
	@(cd fcntl && rm -f ${TRASH})
	@(cd stdio && rm -f ${TRASH})
	@(cd unistd && rm -f ${TRASH})
	@(cd wait && rm -f ${TRASH})
