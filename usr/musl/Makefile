TARGET = musl

# SDCC 4.4.0 and 4.5.0 craps out with internal error if --max-allocs-per-node is >= 500 :/
CFLAGS = -mz180 --opt-code-size --max-allocs-per-node 400 \
  -I arch/z180 -I arch/generic -I src/internal -I src/include -I include \

SRC =  $(wildcard src/internal/*.c)
SRC += $(wildcard src/termios/*.c)
SRC += $(wildcard src/time/*.c)
SRC += $(wildcard src/unistd/*.c)

OBJ = $(SRC:.c=.rel)
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
	@(cd src/internal && rm -f ${TRASH})
	@(cd src/termios && rm -f ${TRASH})
	@(cd src/time && rm -f ${TRASH})
	@(cd src/unistd && rm -f ${TRASH})
