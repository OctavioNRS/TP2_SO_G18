/*
 * Scheduler y gestor de procesos.
 *
 * Tabla `processTable[MAX_PROCESSES]` indexada por PID (los PIDs van de 0 a
 * MAX_PROCESSES-1 y se reusan al liberarse). Cola FIFO de procesos READY para que
 * `pickNext` sea O(1). Idle (PID 0) corre cuando no hay nadie listo; init (PID 1)
 * reapea huérfanos vía wait_any.
 *
 * Foreground set: el shell agrega/saca PIDs según lanza/espera comandos. Ctrl+C
 * (manejado en el ISR del teclado) los mata a todos.
 */
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include <process.h>
#include <fileAccess.h>

typedef struct {
    pid_t     pid;
    pid_t     ppid;
    State     state;
    char     *name;
    uint8_t   priority;
    uint8_t   inForeground;
    uint64_t *rsp;
    uint64_t *stackBase;
} ProcessInfo;

typedef struct {
    int affectedFd;
    int targetFd;
} fd_override_t;

void initializeScheduler(ProcessEntryPoint initProcessEntry);
void enableScheduler(void);

int getProcessesCount(void);

pid_t startNewProcess(ProcessEntryPoint entryPoint, char *name,
                      uint8_t priorityLevel, int argc, char **argv);

pid_t startNewProcessWithFds(ProcessEntryPoint entryPoint, char *name,
                             uint8_t priorityLevel, int argc, char **argv,
                             fd_override_t *overrides, int overrideCount);

uint64_t schedule(uint64_t prevRSP);

void forceSchedulerCall(void);

pid_t getCurrentPID(void);
int   getProcessInfo(ProcessInfo *buffer, int maxCount);

void setCurrentBlocked(void);

int  setBlocked(pid_t pid);
int  setReady(pid_t pid);

int  setPriorityOnPID(pid_t pid, uint8_t priority);

int  terminateProcess(pid_t pid);

void wait_pid(pid_t pid);
void wait_any(void);

/* Reapea (sin bloquear) todos los hijos zombies del proceso actual. El shell lo llama
 * antes de cada prompt para no acumular procesos terminados en background. */
void reap_zombies(void);

/* Atajos sobre el current process. */
FileAccess getCurrentFd(int fd);
int        addCurrentFd(FileAccess fa);              /* primer slot libre */
int        openCurrentFd(FileAccess fa, int fd);     /* slot específico */
int        closeCurrentFd(int fd);
int        dupCurrentFd(int oldFd, int newFd);
int        reopenCurrentFd(int fd);

/* Operan sobre un pid arbitrario. */
int64_t    readFdOnPID(pid_t pid, int fd, char *buf, uint64_t count);
int64_t    writeFdOnPID(pid_t pid, int fd, const char *buf, uint64_t count);
int        requestEofOnPID(pid_t pid, int fd);
FileAccess getFdOnPID(pid_t pid, int fd);
int        addFdToPID(pid_t pid, FileAccess fa);
int        closeFdOnPID(pid_t pid, int fd);
int        dupFdOnPID(pid_t pid, int oldFd, int newFd);

/* Cierra todos los FDs del proceso (decrementa refcounts) */
void closeFdsOf(pid_t pid);

int64_t read(int fd, char *buf, uint64_t count);
int64_t write(int fd, const char *buf, uint64_t count);

int  addToFG(pid_t pid);
int  removeFromFG(pid_t pid);
int  isInFG(pid_t pid);

typedef void (*ActionOnPID)(pid_t pid);
void forEachInFG(ActionOnPID action);

/* Variante de terminateProcess segura desde un ISR: marca ZOMBIE y cierra fds, pero no
 * fuerza el scheduler ni habilita interrupciones (el ISR maneja su propio EOI/iretq). */
void killFromIsr(pid_t pid);

#endif
