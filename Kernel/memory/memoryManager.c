/*
 * memoryManager.c — Administrador de memoria por FREE LIST (first-fit con coalescing).
 *
 * Implementación por defecto (se compila cuando NO está definido USE_BUDDY).
 *
 * Diseño: una lista doblemente enlazada de bloques contiguos en memoria, ordenada
 * por dirección física. Cada bloque tiene un encabezado (Block) seguido de su
 * payload. `mem_alloc` busca el primer bloque libre que alcance (first-fit) y lo
 * parte si sobra espacio; `mem_free` marca el bloque libre y lo fusiona
 * (coalescing) con sus vecinos físicos libres para combatir la fragmentación.
 */
#ifndef USE_BUDDY

#include <memoryManager.h>

typedef struct Block {
    uint64_t      size;   /* tamaño del payload (bytes), sin contar el encabezado */
    uint8_t       free;   /* 1 = libre, 0 = ocupado                               */
    struct Block *prev;   /* bloque físico anterior                               */
    struct Block *next;   /* bloque físico siguiente                              */
} Block;

#define ALIGNMENT   8
#define HEADER_SIZE (sizeof(Block))

static Block   *baseBlock = NULL;
static uint64_t totalBytes = 0;

static uint64_t alignUp(uint64_t n) {
    return (n + (ALIGNMENT - 1)) & ~((uint64_t)ALIGNMENT - 1);
}

void mem_init(void *start, uint64_t size) {
    baseBlock = (Block *) start;
    baseBlock->size = size - HEADER_SIZE;
    baseBlock->free = 1;
    baseBlock->prev = NULL;
    baseBlock->next = NULL;
    totalBytes = size;
}

void *mem_alloc(uint64_t size) {
    if (size == 0 || baseBlock == NULL) return NULL;
    size = alignUp(size);

    for (Block *b = baseBlock; b != NULL; b = b->next) {
        if (!b->free || b->size < size) continue;

        /* Partir el bloque si el remanente alcanza para otro encabezado + payload */
        if (b->size >= size + HEADER_SIZE + ALIGNMENT) {
            Block *split = (Block *)((uint8_t *)(b + 1) + size);
            split->size = b->size - size - HEADER_SIZE;
            split->free = 1;
            split->prev = b;
            split->next = b->next;
            if (b->next) b->next->prev = split;
            b->next = split;
            b->size = size;
        }
        b->free = 0;
        return (void *)(b + 1);
    }
    return NULL;   /* sin espacio */
}

void mem_free(void *ptr) {
    if (ptr == NULL) return;
    Block *b = (Block *) ptr - 1;
    b->free = 1;

    /* Fusionar con el bloque físico siguiente si está libre */
    if (b->next && b->next->free) {
        b->size += HEADER_SIZE + b->next->size;
        b->next = b->next->next;
        if (b->next) b->next->prev = b;
    }
    /* Fusionar con el bloque físico anterior si está libre */
    if (b->prev && b->prev->free) {
        b->prev->size += HEADER_SIZE + b->size;
        b->prev->next = b->next;
        if (b->next) b->next->prev = b->prev;
    }
}

void mem_status(MemoryInfo *info) {
    if (info == NULL) return;
    info->total = totalBytes;
    info->free = 0;
    info->fragments = 0;
    for (Block *b = baseBlock; b != NULL; b = b->next) {
        if (b->free) {
            info->free += b->size;
            info->fragments++;
        }
    }
    info->occupied = totalBytes - info->free;
}

#endif /* !USE_BUDDY */
