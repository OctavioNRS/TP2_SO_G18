include Makefile.inc

KERNEL=kernel.bin
SOURCES=$(wildcard *.c) $(wildcard memory/*.c)
SOURCES_ASM=$(wildcard asm/*.asm)
OBJECTS=$(SOURCES:.c=.o)
# El loader debe ser el PRIMER objeto del link: el output es OUTPUT_FORMAT("binary")
# y Pure64 salta al primer byte del binario (0x100000), por eso .text de loader.o
# tiene que ir primero. El resto se ordena por wildcard.
LOADEROBJECT=asm/loader.o
OBJECTS_ASM=$(filter-out $(LOADEROBJECT),$(SOURCES_ASM:.asm=.o))
STATICLIBS=

all: $(KERNEL)

# `make buddy` (desde el Makefile raíz) recompila el kernel con el Buddy system.
# Limpia primero para forzar la recompilación de los .o con -DUSE_BUDDY.
buddy: clean
	$(MAKE) all MM=BUDDY

$(KERNEL): $(LOADEROBJECT) $(OBJECTS) $(OBJECTS_ASM) $(STATICLIBS)
	$(LD) $(LDFLAGS) -T kernel.ld -o $(KERNEL) $(LOADEROBJECT) $(OBJECTS) $(OBJECTS_ASM) $(STATICLIBS)

%.o: %.c
	$(GCC) $(GCCFLAGS) -I./include -c $< -o $@

%.o : %.asm
	$(ASM) $(ASMFLAGS) $< -o $@

clean:
	rm -rf asm/*.o memory/*.o *.o *.bin

.PHONY: all buddy clean
