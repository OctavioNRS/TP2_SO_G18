include Makefile.inc

KERNEL=kernel.bin
SOURCES=$(wildcard *.c) \
        $(wildcard memory/*.c) \
        $(wildcard Drivers/*.c) \
        $(wildcard IPC/*.c) \
        $(wildcard interruptions/*.c) \
        $(wildcard process_contextswitch/*.c) \
        $(wildcard semaphores/*.c)
SOURCES_ASM=$(wildcard asm/*.asm)
OBJECTS=$(SOURCES:.c=.o)

LOADEROBJECT=asm/loader.o
OBJECTS_ASM=$(filter-out $(LOADEROBJECT),$(SOURCES_ASM:.asm=.o))
STATICLIBS=

all: $(KERNEL)

buddy: clean
	$(MAKE) all MM=BUDDY

$(KERNEL): $(LOADEROBJECT) $(OBJECTS) $(OBJECTS_ASM) $(STATICLIBS)
	$(LD) $(LDFLAGS) -T kernel.ld -o $(KERNEL) $(LOADEROBJECT) $(OBJECTS) $(OBJECTS_ASM) $(STATICLIBS)

%.o: %.c
	$(GCC) $(GCCFLAGS) -I./include -c $< -o $@

%.o : %.asm
	$(ASM) $(ASMFLAGS) $< -o $@

clean:
	rm -rf asm/*.o memory/*.o Drivers/*.o IPC/*.o interruptions/*.o \
	       process_contextswitch/*.o semaphores/*.o *.o *.bin

.PHONY: all buddy clean
