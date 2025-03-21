TARGET = kernel

CPU = z180
CODE = 0x0240
DATA = 0x8B00
GHASH = "$(shell git describe --always --dirty)"
DATE = "$(shell date +"%X %x")"
CFLAGS = -m${CPU} -DVERSION='${GHASH}' -DDATE='${DATE}' \
  -L../driver -L../filesystem -I../ -I. -I../usr/zlibc/include/bits \
  --opt-code-size --max-allocs-per-node 10000
LDFLAGS = --code-loc ${CODE} --data-loc ${DATA} --no-std-crt0

SRC = main.c syscall.c
SRC += mem/page.c mem/kmalloc.c
SRC += proc/timer.c proc/thread.c proc/lock.c proc/cond.c proc/process.c proc/file.c
SRC += dev/floppy.c
SRC += fs/fs.c fs/fat.c fs/devfs.c
SRC += lib/list.c lib/bheap.c lib/strdup.c lib/id.c lib/panic.c lib/assert.c lib/kprintf.c
#SRC += test/kmalloc.c
#SRC += test/rand.c test/condwait.c
OBJ = hal/crt0.rel $(SRC:.c=.rel)
DRIVERS = driver.lib
FILESYSTEMS = fat12.lib
LIB = ${DRIVERS} ${FILESYSTEMS}
TRASH = *.bin *.lk *.map *.mem *.lst *.rel *.rst *.sym *.asm *.ihx *.noi *.hex

.PHONY: clean

all: ${TARGET}.bin

${TARGET}.bin: ${TARGET}.hex
	@objcopy -Iihex -Obinary ${TARGET}.hex ${TARGET}.bin
	du -b ${TARGET}.bin

${TARGET}.hex: ${OBJ}
	sdcc -o ${TARGET}.hex -l ${LIB} ${CFLAGS} ${LDFLAGS} ${OBJ}

%.rel: %.c
	sdcc ${CFLAGS} -o "$@" -c "$<"

%.rel: %.s
	sdasz80 -l -o -s "$<"

clean:
	@rm -f ${TRASH}
	@(cd proc && rm -f ${TRASH})
	@(cd lib && rm -f ${TRASH})
	@(cd fs && rm -f ${TRASH})
	@(cd dev && rm -f ${TRASH})
	@(cd test && rm -f ${TRASH})
	@(cd mem && rm -f ${TRASH})
	@(cd hal && rm -f ${TRASH})

