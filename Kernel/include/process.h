/*
 * TAD de proceso (Process Control Block).
 *
 * Cada proceso encapsula su contexto (rsp, rip, stack), su identificación (pid, name,
 * ppid), su estado, su prioridad/quantums, su jerarquía padre-hijo y dos semáforos
 * para sincronización del ciclo de vida (terminated, childTerminated).
 *
 * Además mantiene una tabla de file descriptors. Cada slot apunta a un `FileAccess`,
 * que a su vez referencia un Pipe con permisos R/W/NONBLOCK. La tabla maneja
 * refcounts vía addFileRef/removeFileRef.
 */
#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include <queue.h>
#include <semaphore.h>
#include <fileAccess.h>

#define MAX_PROCESSES               64
#define MAX_PROCESS_NAME_LENGTH     32
#define DEFAULT_STACK_SIZE          0x8000   /* 32 KiB */
#define MAX_FDS                     16

#define IDLE_PID                    0
#define INIT_PID                    1

typedef struct processCDT *Process;
typedef int16_t            pid_t;

/* Estados del proceso. */
typedef enum {
    READY,
    RUNNING,
    BLOCKED,
    ZOMBIE
} State;

/* Punto de entrada de un proceso. Devuelve un código de salida. */
typedef int (*ProcessEntryPoint)(int argc, char **argv);

Process newProcess(ProcessEntryPoint entryPoint, char *name, uint64_t stackSize,
                   pid_t pid, uint8_t priorityLevel, int argc, char **argv);

void freeProcess(Process process);

uint64_t *getRSP(Process process);

void      setRSP(Process process, uint64_t *newRSP);

uint64_t  *getProcessStackBase(Process process);

uint64_t   getRIP(Process process);

int       getArgc(Process process);

char    **getArgv(Process process);

uint64_t  getStackMisalignment(Process process);

pid_t     getPID(Process process);
pid_t     getPPID(Process process);
void      setPPID(Process process, pid_t ppid);

char     *getProcessName(Process process);

State     getState(Process process);
void      setState(Process process, State newState);


uint8_t   getPriority(Process process);
void      setPriority(Process process, uint8_t priority);

int       hasQuantums(Process process);
void      useQuantum(Process process);
void      resetQuantums(Process process);

Queue     getChildren(Process process);
Queue     getTerminatedChildren(Process process);

int       addChild(Process parent, pid_t childPid);
int       removeChild(Process parent, pid_t childPid);

Semaphore getTerminated(Process process);
Semaphore getChildTerminated(Process process);

/* Devuelve el FileAccess en `fd` o NULL si está cerrado / fd inválido. */
FileAccess getFd(Process process, int fd);

/* Pone `fa` en el slot `fd` (debe estar libre). Incrementa la refcount. */
int        openFd(Process process, FileAccess fa, int fd);

/* Pone `fa` en el primer slot libre. Devuelve el fd asignado o -1. */
int        addFd(Process process, FileAccess fa);

/* Libera el slot fd (removeFileRef). */
int        closeFd(Process process, int fd);

/* Clona el FileAccess en oldFd y lo coloca en newFd (cierra newFd si estaba abierto). */
int        dupFd(Process process, int oldFd, int newFd);

/* Crea un nuevo FileAccess clon del que está en `fd` y lo abre en el primer slot
 * libre. Devuelve el nuevo fd o -1. */
int        reopenFd(Process process, int fd);

int64_t    readFd (Process process, int fd, char *buf, uint64_t count);
int64_t    writeFd(Process process, int fd, const char *buf, uint64_t count);

/* Marca EOF pendiente en `fd`: el próximo read devolverá 0 sin tocar el pipe. */
int        requestEofOf(Process process, int fd);

/* Cierra todos los fds del proceso. La llama terminateProcess. */
void       closeAllFDs(Process process);

/* Copia la tabla de fds de `source` a `target` (incrementa refcounts). Usado para
 * la herencia al crear un proceso hijo. */
void       copyFdTable(Process target, Process source);

/* Comparador para usar con Queue (comparePIDs). */
int       comparePIDs(void *element1, void *element2);

#endif
