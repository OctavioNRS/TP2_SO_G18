/*
 * TAD Semaforo.
 */
#include <semaphore.h>
#include <memoryManager.h>
#include <queue.h>
#include <scheduler.h>
#include <interrupts.h>   /* _cli, _sti */
#include <stdint.h>

/* Spinlock atómico con builtins de GCC. Implementado como macros para garantizar
 * inlinación en -O0 (sin esto el compilador deja referencias externas no resueltas). */
#define ACQUIRE(lock_ptr) \
    while (__atomic_exchange_n((lock_ptr), (uint8_t)1, __ATOMIC_ACQUIRE)) { }
#define RELEASE(lock_ptr) \
    __atomic_store_n((lock_ptr), (uint8_t)0, __ATOMIC_RELEASE)

typedef struct semCDT {
    int              value;
    Queue            blockedQueue;
    volatile uint8_t lock;
} semCDT;

Semaphore newSemaphore(int value) {
    Semaphore sem = (Semaphore) mem_alloc(sizeof(semCDT));
    if (!sem) return 0;
    sem->value = value;
    sem->lock  = 0;
    sem->blockedQueue = newQueue();
    if (!sem->blockedQueue) { mem_free(sem); return 0; }
    return sem;
}

int semWait(Semaphore sem) {
    if (!sem) return -1;

    _cli();
    ACQUIRE(&sem->lock);
    sem->value--;
    int mustBlock = (sem->value < 0);
    if (mustBlock) {
        int16_t currentPID = getCurrentPID();
        setCurrentBlocked();
        enqueue(sem->blockedQueue, (void*)(intptr_t)currentPID);
    }
    RELEASE(&sem->lock);
    _sti();

    if (mustBlock) forceSchedulerCall();   /* cede ya bloqueado */
    return 0;
}

int semPost(Semaphore sem) {
    if (!sem) return -1;

    _cli();
    ACQUIRE(&sem->lock);
    sem->value++;
    if (sem->value <= 0) {
        int16_t pid = (int16_t)(intptr_t) pollQueue(sem->blockedQueue);
        setReady(pid);
    }
    RELEASE(&sem->lock);
    _sti();
    return 0;
}

void freeSemaphore(Semaphore sem) {
    if (!sem) return;
    _cli();
    ACQUIRE(&sem->lock);
    if (!isQueueEmpty(sem->blockedQueue)) {   /* no destruir con procesos esperando */
        RELEASE(&sem->lock);
        _sti();
        return;
    }
    freeQueue(sem->blockedQueue);
    /* no hace falta release: estamos liberando el objeto */
    _sti();
    mem_free(sem);
}
