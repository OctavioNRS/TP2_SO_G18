/*
 * pipe.h — TAD Pipe (FIFO bloqueante).
 *
 * Sincronización por semáforos contadores siguiendo la referencia:
 *   - dataAvailable: cantidad de bytes legibles. Los readers semWait acá.
 *   - freeSpaces:    cantidad de bytes que entran. Los writers semWait acá.
 *   - readMutex / writeMutex: serializan readers entre sí y writers entre sí.
 *
 * Cada Pipe lleva dos contadores:
 *   - refCount:    cuántos FileAccess lo apuntan. Cuando llega a 0 se libera.
 *   - writerCount: cuántos accesos con FILE_WRITE están vivos. Cuando llega a 0
 *                  se despierta a los readers bloqueados para que detecten EOF.
 *
 * No interactúa con la tabla de FDs: eso lo hace FileAccess + scheduler/process.
 */
#ifndef PIPE_H
#define PIPE_H

#include <stdint.h>

#define PIPE_BUF_SIZE    1024
#define PIPE_WOULD_BLOCK (-2)

typedef struct pipeCDT *Pipe;

/* Crea un pipe vacío con buffer interno de PIPE_BUF_SIZE bytes. refCount y
 * writerCount arrancan en 0 (los maneja el caller vía addPipeRef/addPipeWriter). */
Pipe newPipe(void);

void addPipeRef(Pipe pipe);
void removePipeRef(Pipe pipe);     /* libera el pipe cuando refCount llega a 0 */

void addPipeWriter(Pipe pipe);
void removePipeWriter(Pipe pipe);  /* despierta readers si era el último writer */

int  pipeHasData(Pipe pipe);
int  pipeIsFull(Pipe pipe);

/* Lectura. En modo bloqueante espera hasta haber datos o EOF (último writer cerró).
 * En no-bloqueante devuelve PIPE_WOULD_BLOCK si no hay datos pero todavía hay
 * writers. Devuelve 0 = EOF, n > 0 = bytes leídos, -1 = error. */
int64_t pipeRead(Pipe pipe, char *buf, uint64_t count, int nonblocking);

/* Escritura. En bloqueante espera hasta haber espacio. En no-bloqueante puede
 * devolver PIPE_WOULD_BLOCK si está lleno. */
int64_t pipeWrite(Pipe pipe, const char *buf, uint64_t count, int nonblocking);

/* Despierta a los readers bloqueados sin agregar datos (usado para EOF o Ctrl+C).
 * `count` es la cantidad de posts a hacer en el semáforo de datos. */
void forceReadersWakeUp(Pipe pipe, int count);

void freePipe(Pipe pipe);

#endif
