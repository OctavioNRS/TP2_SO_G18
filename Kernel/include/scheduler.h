/*
 * Scheduler y gestor de procesos.
 *
 * Mantiene una tabla `processTable[MAX_PROCESSES]` indexada por PID (los PIDs van de 0
 * a MAX_PROCESSES-1 y se reusan al liberarse). Cola FIFO de procesos READY para que
 * `pickNext` sea O(1). Idle (PID 0) corre cuando no hay nadie listo; init (PID 1)
 * reapea huérfanos vía wait_any.
 *
 */
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include <process.h>

/* Información de un proceso*/
typedef struct {
    pid_t     pid;
    pid_t     ppid;
    State     state;
    char     *name;
    uint8_t   priority;
    uint64_t *rsp;
    uint64_t *stackBase;
} ProcessInfo;

/* Inicializa la tabla de procesos y crea idle (PID 0) e init (PID 1). */
void initializeScheduler(ProcessEntryPoint initProcessEntry);

/* Habilita el cambio de contexto*/
void enableScheduler(void);

/* Crea un nuevo proceso. Devuelve su PID, o -1 si falla */
pid_t startNewProcess(ProcessEntryPoint entryPoint, char *name,
                      uint8_t priorityLevel, int argc, char **argv);

/* Núcleo del context switch: lo invoca _irq00Handler. Devuelve siempre un rsp válido
 * (el mismo si no hubo conmutación). */
uint64_t schedule(uint64_t prevRSP);

/* Fuerza una invocación del scheduler por software (yield). */
void forceSchedulerCall(void);

/* ---------- Información ---------- */

pid_t getCurrentPID(void);
int   getProcessInfo(ProcessInfo *buffer, int maxCount);

/* ---------- Control de procesos ---------- */

/* Marca el proceso actual como BLOCKED (sin ceder el CPU; combinar con
 * forceSchedulerCall para ceder después de soltar locks). */
void setCurrentBlocked(void);

/* Marca un proceso por pid. Devuelve 0/-1. setReady también lo agrega a la readyQueue. */
int  setBlocked(pid_t pid);
int  setReady(pid_t pid);

/* Cambia la prioridad de un proceso. */
int  setPriorityOnPID(pid_t pid, uint8_t priority);

/* Termina un proceso */
int  terminateProcess(pid_t pid);

/* Espera a que un proceso hijo termine. Bloquea al proceso actual hasta
 * que el sem `terminated` del hijo se postee*/
void wait_pid(pid_t pid);

/* Espera a que cualquier hijo del proceso actual termine y reapealo. La usa init. */
void wait_any(void);

#endif
