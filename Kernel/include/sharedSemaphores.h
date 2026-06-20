/*
 * sharedSemaphores.h — Semáforos compartibles por nombre.
 *
 * Construye una capa de "named semaphores" arriba del TAD `Semaphore`. Procesos
 * no relacionados acuerdan un identificador (string). El kernel cuenta referencias: cuando todos los procesos lo cerraron, se
 * libera la memoria.
 */
#ifndef SHARED_SEMAPHORES_H
#define SHARED_SEMAPHORES_H

#define MAX_SHARED_SEMS 64
#define SEM_NAME_MAX    32

/* Crea o abre un semáforo con ese nombre. Si ya existe, incrementa su refcount y NO
 * cambia su valor inicial. Devuelve 0 ok / -1 si no hay slot libre o el nombre es inválido. */
int sharedSemOpen(const char *name, int initialValue);

/* Decrementa el refcount; si llega a 0, libera el semáforo y el slot. */
int sharedSemClose(const char *name);

/* Operaciones sobre el semáforo identificado por `name`. -1 si no existe. */
int sharedSemWait(const char *name);
int sharedSemPost(const char *name);

#endif
