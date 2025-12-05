TARGET = zesh

CC = $(shell if `which sdcc-sdcc > /dev/null 2>&1`; then echo "sdcc-sdcc"; else echo "sdcc"; fi)
AS = $(shell if `which sdcc-sdasz80 > /dev/null 2>&1`; then echo "sdcc-sdasz80"; else echo "sdasz80"; fi)

CPU = z180
CODE = 0x1020
DATA = 0x8000
CFLAGS = -m${CPU} --opt-code-size --max-allocs-per-node 10000 \
  -I ../zlibc/include
LDFLAGS = --code-loc ${CODE} --data-loc ${DATA} --no-std-crt0

SRC = main.c shell.c
OBJ = ../zlibc/crt0.rel $(SRC:.c=.rel)
LIB = "../zlibc/zlibc.lib"
TRASH = *.bin *.lk *.map *.mem *.lst *.rel *.rst *.sym *.asm *.ihx *.noi *.hex

.PHONY: clean

all: ${TARGET}.bin

${TARGET}.bin: ${TARGET}.hex
	@objcopy -Iihex -Obinary ${TARGET}.hex ${TARGET}.bin
	du -b ${TARGET}.bin

${TARGET}.hex: ${OBJ}
	$(CC) -o ${TARGET}.hex -l ${LIB} ${CFLAGS} ${LDFLAGS} ${OBJ}

%.rel: %.c
	$(CC) ${CFLAGS} -o "$@" -c "$<"

%.rel: %.s
	$(AS) -l -o -s "$<"

clean:
	@rm -f ${TRASH}
