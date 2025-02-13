; ZAK180 Firmaware
; User space crt0.s
; Copyright: Aleksander Kaminski, 2025
; See LICENSE.md

; Watch out! Only 0x200 bytes for this code!

.module ctr0

.globl _main

.z180

; User space memory layout:
; Common 0: (kernel entry point)
; 4 KB, 0x0000 -> 0x0FFF (0x00000 -> 0x00FFF)
; Bank: (user code and data)
; 56 KB, 0x1000 -> 0xEFFF (allocated by the kernel)
; Common 1: (stack)
; 4 KB, 0xF000 -> 0xFFFF (allocated by the kernel)

.area _HEADER (ABS)
.org 0x1000

_entry:
			; TODO handle argv and envp

			; Init .bss and .data
			call bss_init
			call data_init

			call _main

			; _syscall_process_end
			ld a, #3
			rst 0x38

loop:		halt
			jr loop

.area _HOME
.area _CODE
.area _INITIALIZER
.area _GSINIT
.area _GSFINAL

.area _DATA
.area _INITIALIZED
.area _BSEG
.area _BSS
.area _HEAP

.area _GSINIT

.globl l__DATA
.globl s__DATA
.globl l__INITIALIZER
.globl s__INITIALIZER
.globl s__INITIALIZED

bss_init:
			ld bc, #l__DATA ; Length to zero-out
			ld a, b         ; Check if zero
			or a, c
			ret z

			ld hl, #s__DATA ; Start of the .bss
			ld (hl), #0     ; Zero-out first byte
			dec bc          ; One byte already done
			ld a, b         ; Check if zero
			or a, c
			ret z

			ld d, h
			ld e, l
			inc de
			ldir            ; Copy first zeroed byte

			ret

data_init:
			ld bc, #l__INITIALIZER
			ld a, b         ; Check if zero
			or a, c
			ret z

			ld	de, #s__INITIALIZED
			ld	hl, #s__INITIALIZER
			ldir

			ret
