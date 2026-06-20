/*
 * memoryManager.h — Interfaz del administrador de memoria.
 *
 * Esta API la implementan dos archivos intercambiables a build-time: el
 * free-list de `memory/memoryManager.c` (default) o el buddy de
 * `memory/memoryManagerBuddy.c` (cuando `USE_BUDDY` está definido, `make buddy`).
 */
#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <stdint.h>
#include <stddef.h>

#define MEM_HEAP_START 0x1000000UL
#define MEM_HEAP_SIZE  (256UL * 1024 * 1024)   /* 256 MiB (potencia de 2 para el buddy) */

typedef struct {
    uint64_t total;      /* bytes totales administrados             */
    uint64_t occupied;   /* bytes ocupados (incluye overhead)       */
    uint64_t free;       /* bytes libres                            */
    uint64_t fragments;  /* cantidad de bloques libres (fragmentos) */
} MemoryInfo;

void   mem_init(void *start, uint64_t size);
void  *mem_alloc(uint64_t size);
void   mem_free(void *ptr);
void   mem_status(MemoryInfo *info);

#endif
