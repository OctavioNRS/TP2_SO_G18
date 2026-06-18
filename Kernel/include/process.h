/*
 * TAD de proceso (Process Control Block).
 *
 * Cada proceso encapsula su contexto (rsp, rip, stack), su identificación (pid, name,
 * ppid), su estado, su prioridad/quantums, su jerarquía padre-hijo (children +
 * terminatedChildren) y dos semáforos para sincronización del ciclo de vida:
 *   - `terminated`: se postea al morir; el padre lo espera con wait_pid.
 *   - `childTerminated`: se postea al morir un hijo; init lo espera con wait_any.
 *
 */
#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include <queue.h>
#include <semaphore.h>

#define MAX_PROCESSES               64
#define MAX_PROCESS_NAME_LENGTH     32
#define DEFAULT_STACK_SIZE          0x8000   /* 32 KiB */

#define IDLE_PID                    0
#define INIT_PID                    1

typedef struct processCDT *Process;
typedef int16_t            pid_t;

/* Estados del proceso. */
typedef enum {
    READY,
    RUNNING,
    BLOCKED,
    ZOMBIE     /* terminado, pendiente de wait+reap del padre */
} State;

/* Punto de entrada de un proceso. Devuelve un código de salida. */
typedef int (*ProcessEntryPoint)(int argc, char **argv);

/* Crea un nuevo proceso. `pid` viene asignado por el scheduler. */
Process newProcess(ProcessEntryPoint entryPoint, char *name, uint64_t stackSize,
                   pid_t pid, uint8_t priorityLevel, int argc, char **argv);

/* Libera los recursos del proceso (queues, semáforos, stack). */
void freeProcess(Process process);

/* ---------- Getters/Setters de contexto ---------- */

uint64_t *getRSP(Process process);
void      setRSP(Process process, uint64_t *newRSP);

uint64_t  *getProcessStackBase(Process process);
uint64_t   getRIP(Process process);

int       getArgc(Process process);
char    **getArgv(Process process);

uint64_t  getStackMisalignment(Process process);

/* ---------- Identificación y estado ---------- */

pid_t     getPID(Process process);
pid_t     getPPID(Process process);
void      setPPID(Process process, pid_t ppid);

char     *getProcessName(Process process);

State     getState(Process process);
void      setState(Process process, State newState);

/* ---------- Prioridad y quantums ---------- */

uint8_t   getPriority(Process process);
void      setPriority(Process process, uint8_t priority);

int       hasQuantums(Process process);
void      useQuantum(Process process);
void      resetQuantums(Process process);

/* ---------- Jerarquía padre/hijo ---------- */

Queue     getChildren(Process process);
Queue     getTerminatedChildren(Process process);

int       addChild(Process parent, pid_t childPid);
int       removeChild(Process parent, pid_t childPid);

/* ---------- Semáforos del ciclo de vida ---------- */

Semaphore getTerminated(Process process);
Semaphore getChildTerminated(Process process);

/* Comparador para usar con Queue (comparePIDs). */
int       comparePIDs(void *element1, void *element2);

#endif
