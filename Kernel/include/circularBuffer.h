/*
 * circularBuffer.h — TAD de FIFO de bytes para el cuerpo del pipe.
 *
 * Sin sincronización: la exclusión mutua la hace el pipe que lo encapsula. Esto
 * permite reusar el mismo buffer en otros contextos donde la sync sea diferente.
 */
#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <stdint.h>
#define EOF 0

typedef struct circularBufferCDT *CircularBuffer;

/* Crea un buffer circular de hasta `maxSize` bytes. Devuelve NULL si no hay memoria. */
CircularBuffer newBuffer(uint64_t maxSize);

int  bufferIsFull(CircularBuffer cb);
int  bufferHasData(CircularBuffer cb);

/* Escribe un byte. Devuelve 1 si lo escribió, 0 si el buffer estaba lleno. */
int  bufferWrite(CircularBuffer cb, char byteToWrite);

/* Lee un byte y avanza la cabeza. Devuelve 0 si estaba vacío (caller debería
 * chequear con bufferHasData antes). */
char bufferRead(CircularBuffer cb);

void freeBuffer(CircularBuffer cb);

#endif
