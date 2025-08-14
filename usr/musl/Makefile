TARGET = musl

CFLAGS = -mz180 --opt-code-size --max-allocs-per-node 10000 \
  -I include

SRC =  # TODO add sources

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
