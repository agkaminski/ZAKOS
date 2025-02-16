; ZAK180 Firmaware
; Syscalls
; Copyright: Aleksander Kaminski, 2025
; See LICENSE.md

;.area _CODE

.globl ___sys_fork
___sys_fork:
			ld a, #0
			rst 0x38

.globl ___sys_waitpid
___sys_waitpid:
			ld a, #1
			rst 0x38

.globl ___sys_exit
___sys_exit:
			ld a, #2
			rst 0x38
			ret

.globl ___sys_msleep
___sys_msleep:
			ld a, #3
			rst 0x38

.globl ___sys_execv
___sys_execv:
			ld a, #4
			rst 0x38

.globl ___sys_write
___sys_write:
			ld a, #5
			rst 0x38
