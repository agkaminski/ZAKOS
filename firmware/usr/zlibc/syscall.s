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

.globl ___sys_open
___sys_open:
			ld a, #5
			rst 0x38

.globl ___sys_close
___sys_close:
			ld a, #6
			rst 0x38

.globl ___sys_read
___sys_read:
			ld a, #7
			rst 0x38

.globl ___sys_write
___sys_write:
			ld a, #8
			rst 0x38

.globl ___sys_truncate
___sys_truncate:
			ld a, #9
			rst 0x38

.globl ___sys_ftruncate
___sys_ftruncate:
			ld a, #10
			rst 0x38

.globl ___sys_readdir
___sys_readdir:
			ld a, #11
			rst 0x38

.globl ___sys_remove
___sys_remove:
			ld a, #12
			rst 0x38
