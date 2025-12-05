.PHONY: clean

CC = $(shell if `which sdcc-sdcc > /dev/null 2>&1`; then echo "sdcc-sdcc"; else echo "sdcc"; fi)
AR = $(shell if `which sdcc-sdar > /dev/null 2>&1`; then echo "sdcc-sdar"; else echo "sdar"; fi)

OBJ := ay38912.rel critical.rel dma.rel floppy_cmd.rel floppy.rel \
  keyboard.rel mmu.rel pio.rel prt.rel uart.rel vga.rel

all: driver.lib

driver.lib: ${OBJ}
	@$(AR) -q "$@" $^

%.rel: %.c
	$(CC) -mz180 --opt-code-size --max-allocs-per-node 10000 -c "$<"

clean:
	@rm -f *.lib *.lk *.map *.mem *.lst *.rel *.rst *.sym *.asm *.ihx *.noi *.hex
