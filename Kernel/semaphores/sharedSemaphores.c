/*
 * sharedSemaphores.c — Semáforos compartibles por nombre (capa de named-sems).
 *
 * Tabla interna `table[MAX_SHARED_SEMS]` indexada por slot. Cada entrada guarda el
 * nombre, el puntero al TAD `Semaphore` (creado con newSemaphore) y un refcount.
 *
 * Serialización por spinlock atómico (test-and-set con __atomic_exchange_n),
 * mismo patrón que el de `semaphore.c`. Usar cli/sti acá rompía IF en mitad de
 * la atención de algún ISR; el spinlock evita ese acoplamiento al costo de un
 * busy-wait insignificante en un solo CPU (la ventana crítica son pocas
 * instrucciones, salvo en sharedSemOpen donde se llama a newSemaphore — ahí la
 * sección es algo más larga, pero los ISR del sistema no entran a esta capa).
 */
#include <stdint.h>
#include <sharedSemaphores.h>
#include <semaphore.h>
#include <lib.h>

#define ACQUIRE(lock_ptr) \
    while (__atomic_exchange_n((lock_ptr), (uint8_t)1, __ATOMIC_ACQUIRE)) { }
#define RELEASE(lock_ptr) \
    __atomic_store_n((lock_ptr), (uint8_t)0, __ATOMIC_RELEASE)

typedef struct {
    uint8_t   used;
    char      name[SEM_NAME_MAX];
    Semaphore sem;
    int       refCount;
} SharedSem;

static SharedSem        table[MAX_SHARED_SEMS];
static volatile uint8_t tableLock = 0;

static int findByName(const char *name) {
    for (int i = 0; i < MAX_SHARED_SEMS; i++)
        if (table[i].used && strcmp(table[i].name, name) == 0) return i;
    return -1;
}

static int findFreeSlot(void) {
    for (int i = 0; i < MAX_SHARED_SEMS; i++)
        if (!table[i].used) return i;
    return -1;
}

int sharedSemOpen(const char *name, int initialValue) {
    if (!name || !name[0]) return -1;
    ACQUIRE(&tableLock);
    int idx = findByName(name);
    if (idx >= 0) {
        table[idx].refCount++;
        RELEASE(&tableLock);
        return 0;
    }
    idx = findFreeSlot();
    if (idx < 0) { RELEASE(&tableLock); return -1; }
    Semaphore s = newSemaphore(initialValue);
    if (!s) { RELEASE(&tableLock); return -1; }
    table[idx].used = 1;
    strcpy(table[idx].name, name);
    table[idx].sem = s;
    table[idx].refCount = 1;
    RELEASE(&tableLock);
    return 0;
}

int sharedSemClose(const char *name) {
    if (!name || !name[0]) return -1;
    ACQUIRE(&tableLock);
    int idx = findByName(name);
    if (idx < 0) { RELEASE(&tableLock); return -1; }
    if (--table[idx].refCount <= 0) {
        Semaphore s = table[idx].sem;
        table[idx].used = 0;
        table[idx].sem = 0;
        RELEASE(&tableLock);
        freeSemaphore(s);    /* fuera del tableLock: freeSemaphore agarra su propio lock interno */
        return 0;
    }
    RELEASE(&tableLock);
    return 0;
}

int sharedSemWait(const char *name) {
    if (!name || !name[0]) return -1;
    ACQUIRE(&tableLock);
    int idx = findByName(name);
    Semaphore s = (idx >= 0) ? table[idx].sem : 0;
    RELEASE(&tableLock);
    if (!s) return -1;
    return semWait(s);
}

int sharedSemPost(const char *name) {
    if (!name || !name[0]) return -1;
    ACQUIRE(&tableLock);
    int idx = findByName(name);
    Semaphore s = (idx >= 0) ? table[idx].sem : 0;
    RELEASE(&tableLock);
    if (!s) return -1;
    return semPost(s);
}
