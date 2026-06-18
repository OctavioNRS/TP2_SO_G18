/*
 * TAD de Semáforo
 *
 * Bloqueo sin busy-wait: si semWait no puede tomar el recurso, encola al proceso actual
 * y cede el CPU. Atomicidad de la sección crítica por spinlock con instrucción atómica
 * de hardware (emitida por __atomic_exchange_n vía macros).
 */
#ifndef SEMAPHORE_H
#define SEMAPHORE_H

typedef struct semCDT *Semaphore;

/* Crea un semáforo con valor inicial `value`. Devuelve NULL si no hay memoria. */
Semaphore newSemaphore(int value);

/* Decrementa el valor; si queda negativo, bloquea al proceso actual
 * (lo encola en la cola interna del semáforo y cede el CPU). */
int semWait(Semaphore semaphore);

/* Incrementa el valor; si había procesos bloqueados, despierta al primero. */
int semPost(Semaphore semaphore);

/* Libera la memoria del semáforo. No hace nada si todavía hay procesos bloqueados. */
void freeSemaphore(Semaphore semaphore);

#endif
