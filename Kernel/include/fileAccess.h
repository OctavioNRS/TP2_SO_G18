/*
 * fileAccess.h — Capa entre el FD del proceso y el Pipe físico.
 *
 * Un FileAccess es un "lado" de un pipe: lleva el puntero al pipe y un set de
 * flags (R, W, NONBLOCK). Es lo que vive en la tabla de FDs del proceso. Al
 * heredar fds entre padre e hijo se duplica la referencia (mismo FileAccess,
 * refCount+1); con dup también.
 */
#ifndef FILE_ACCESS_H
#define FILE_ACCESS_H

#include <pipe.h>

#define FILE_READ      0x01
#define FILE_WRITE     0x02
#define FILE_NONBLOCK  0x04

typedef struct fileAccessCDT *FileAccess;

/* Crea un FileAccess apuntando al pipe con los permisos indicados. Si tiene
 * FILE_WRITE, registra un writer en el pipe. Arranca con refCount 0: el caller
 * tiene que llamar addFileRef cuando lo guarde en un slot. */
FileAccess newFileAccess(Pipe pipe, uint8_t flags);

void addFileRef(FileAccess fa);
void removeFileRef(FileAccess fa);   /* libera el FileAccess (y el pipe vía refCount) si era el último */

int  isReadable(FileAccess fa);
int  isWritable(FileAccess fa);
int  isNonblocking(FileAccess fa);

int64_t fileRead (FileAccess fa, char *buf, uint64_t count);
int64_t fileWrite(FileAccess fa, const char *buf, uint64_t count);

void setFileFlags   (FileAccess fa, uint8_t flags);
void addFileFlags   (FileAccess fa, uint8_t flags);
void removeFileFlags(FileAccess fa, uint8_t flags);

/* Devuelve un nuevo FileAccess que apunta al mismo pipe con las mismas flags.
 * Útil para dup (y para reopen en el shell). */
FileAccess cloneFile(FileAccess fa);

/* Devuelve el pipe subyacente. Útil para comparar (¿el STDIN de un proceso es el
 * pipe del teclado?). NULL si el FileAccess es NULL. */
Pipe getFilePipe(FileAccess fa);

void forceWakeOnPipe(FileAccess fa, int count);

void freeFileAccess(FileAccess fa);

#endif
