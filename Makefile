#    make          -> memory manager por defecto (free list)
#    make buddy    -> Buddy system (-DUSE_BUDDY)

all:    toolchain bootloader kernel userland image

buddy:  toolchain bootloader kernel-buddy userland image-buddy

toolchain:
	cd Toolchain; make all

bootloader:
	cd Bootloader; make all

kernel:
	cd Kernel; make all

kernel-buddy:
	cd Kernel; make buddy

userland:
	cd Userland; make all

image: kernel bootloader userland
	cd Image; make all

image-buddy: kernel-buddy bootloader userland
	cd Image; make all

clean:
	cd Toolchain; make clean
	cd Bootloader; make clean
	cd Image; make clean
	cd Kernel; make clean
	cd Userland; make clean

.PHONY: toolchain bootloader kernel kernel-buddy userland image image-buddy all buddy clean
