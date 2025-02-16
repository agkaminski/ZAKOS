TARGET = zlibc

CFLAGS = -mz180 --opt-code-size --max-allocs-per-node 10000 \
  -I include

SRC =  unistd/exit.c unistd/fork.c unistd/msleep.c unistd/write.c
SRC += wait/waitpid.c
SRC += stdio/putchar.c

OBJ = syscall.rel $(SRC:.c=.rel)
TRASH = *.bin *.lk *.map *.mem *.lst *.rel *.rst *.sym *.asm *.ihx *.noi *.hex *.lib

.PHONY: clean

all: ${TARGET}.lib

${TARGET}.lib: ${OBJ}
	@sdar -q "$@" $^

%.rel: %.c
	sdcc ${CFLAGS} -o "$@" -c "$<"

%.rel: %.s
	sdasz80 -l -o -s "$<"

clean:
	@rm -f ${TRASH}
	@(cd unistd && rm -f ${TRASH})
	@(cd wait && rm -f ${TRASH})
