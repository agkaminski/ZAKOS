# ZAKOS

ZAK18x SBCs operating system.

Based on the custom UN*X-like monolithic kernel.

## Software features

ROM bootloader functions solely to load the kernel image from the 1.44 MB 3.5"
FDD that is stored in `/BOOT/KERNEL.IMG` file on FAT12 file system.
The kernel is a multi-tasking, preeptive, of monolithic architecture, with
UN\*X-like features (not even close to the full POSIX compability). Hopefully,
it will be possible to port some UN\*X applications (like vi), although limited
user process address space may present an obstacle (56 KB + 4 KB of stack).

## Boot

The boot process is somewhat complicated due to the limited nature of the Z180
MMU.

First the bootloader copies itself to the high RAM, configures the MMU to divide
the address space into 3 regions and continues execution from the Common 1 area.
This enables the bootloader to disable ROM and allow the computer to access the
low RAM, that was previously occupied by ROM - this is the place kernel will be
loaded into. The kernel image is then loaded from the floppy disk from a file
`/BOOT/KERNEL.IMG` that resides on a FAT12 filesystem. After that the kernel is
executed.

The kernel reconfigures the MMU again to create a separate stack area and a
"window" to access arbitrary physical memory. After initialization of all kernel
subsystems, the first application is executed - init process, located at
`/BOOT/INIT.ZEX'.

Init process executes a simple script located at `/BOOT/INIT.INI` that setups
the console (stdin, stdout, stderr) and executes next processes - for now only
a shell `zesh`.

## MMU memory layouts

### Bootloader

| Region   | Logical start | Logical end | Physical start | Physical end | Description        |
|----------|---------------|-------------|----------------|--------------|--------------------|
| Common 0 | 0x0000        | 0x5FFF      | 0x00000        | 0x05FFF      | Kernel entry point |
| Bank     | 0x6000        | 0x7FFF      | N/A            | N/A          | Memory access      |
| Common 1 | 0x8000        | 0xFFFF      | 0xE8000        | 0xEFFFF      | Code and data      |

### Kernel

| Region   | Logical start | Logical end | Physical start | Physical end | Description        |
|----------|---------------|-------------|----------------|--------------|--------------------|
| Common 0 | 0x0000        | 0xDFFF      | 0x00000        | 0x0DFFF      | Kernel code/data   |
| Bank     | 0xE000        | 0xEFFF      | N/A            | N/A          | Memory access      |
| Common 1 | 0xF000        | 0xFFFF      | N/A            | N/A          | Switchable stack   |

### User

| Region   | Logical start | Logical end | Physical start | Physical end | Description        |
|----------|---------------|-------------|----------------|--------------|--------------------|
| Common 0 | 0x0000        | 0x0FFF      | 0x00000        | 0x00FFF      | Kernel entry       |
| Bank     | 0x1000        | 0xEFFF      | N/A            | N/A          | User code/data     |
| Common 1 | 0xF000        | 0xFFFF      | N/A            | N/A          | Switchable stack   |

## Repository structure

### bootloader

Source code of a bootloader ROM that is located on the internal EPROM memory.

### driver

ZAK180 peripheral drivers, both internal of Z180 MPU and present on the SBC.
This includes:

- Z180 ASCI,
- Z180 MMU,
- AY-3-8912,
- 82077,
- Z80 PIO,
- VGA.

### kernel

Source code of the kernel of the computer operating system.

### rootfs

Root filesystem skeleton. Populated with static files (e.g. `INIT.INI`) and
files created during firmware compilation. Later used to prepare a floopy disk.

### test

Simple dead or alive SBC test. All it does is writing to the ASCI1 (USB-C) 
at 19200 bn1.

### usr

User space programs. This includes `init` process, `zesh` shell, utilities and 
miscellaneous applications. 

### build.sh

Script to build all components of the firmware.

### flash.sh

Builds the firmware (by invoking `build.sh`) and copies the root file system
structure to the floopy disk.

## License

[Attribution-NonCommercial 4.0](LICENSE)
