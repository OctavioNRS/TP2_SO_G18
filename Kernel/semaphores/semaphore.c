/*
 * TAD Semaforo.
 *
 * Atomicidad por spinlock (test-and-set atómico con __atomic_exchange_n)
 */
#include <semaphore.h>
#include <memoryManager.h>
#include <queue.h>
#include <scheduler.h>
#include <stdint.h>

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

    ACQUIRE(&sem->lock);
    sem->value--;
    int mustBlock = (sem->value < 0);
    if (mustBlock) {
        int16_t currentPID = getCurrentPID();
        setCurrentBlocked();
        enqueue(sem->blockedQueue, (void*)(intptr_t)currentPID);
    }
    RELEASE(&sem->lock);

    if (mustBlock) forceSchedulerCall();   /* cede ya bloqueado */
    return 0;
}

int semPost(Semaphore sem) {
    if (!sem) return -1;

    ACQUIRE(&sem->lock);
    sem->value++;
    while (sem->value <= 0 && !isQueueEmpty(sem->blockedQueue)) {
        int16_t pid = (int16_t)(intptr_t) pollQueue(sem->blockedQueue);
        if (setReady(pid) == 0) break;
        sem->value++;
    }
    RELEASE(&sem->lock);
    return 0;
}

int semTryWait(Semaphore sem) {
    if (!sem) return -1;
    int got = -1;
    ACQUIRE(&sem->lock);
    if (sem->value > 0) {
        sem->value--;
        got = 0;
    }
    RELEASE(&sem->lock);
    return got;
}

void freeSemaphore(Semaphore sem) {
    if (!sem) return;
    ACQUIRE(&sem->lock);
    if (!isQueueEmpty(sem->blockedQueue)) {   /* no destruir con procesos esperando */
        RELEASE(&sem->lock);
        return;
    }
    freeQueue(sem->blockedQueue);
    /* no hace falta release: estamos liberando el objeto */
    mem_free(sem);
}
