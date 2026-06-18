/*
 * TAD de proceso.
 *
 */
#include <process.h>
#include <memoryManager.h>
#include <queue.h>
#include <semaphore.h>
#include <stdint.h>

typedef struct processCDT {
    pid_t     pid;
    uint64_t *stackBase;
    uint64_t  stackSize;
    uint64_t *rsp;
    uint64_t  rip;
    State     state;
    char      name[MAX_PROCESS_NAME_LENGTH];

    uint8_t   priorityLevel;
    uint8_t   remainingQuantums;

    pid_t     ppid;
    Queue     children;
    Queue     terminatedChildren;
    Semaphore terminated;
    Semaphore childTerminated;

    int       argc;
    char    **argv;
} processCDT;

int comparePIDs(void *e1, void *e2) {
    pid_t pid1 = (pid_t)(intptr_t)e1;
    pid_t pid2 = (pid_t)(intptr_t)e2;
    return (int)pid1 - (int)pid2;
}

static void copyProcessName(Process process, const char *name) {
    int i = 0;
    if (name != 0)
        while (i + 1 < MAX_PROCESS_NAME_LENGTH && name[i]) {
            process->name[i] = name[i]; i++;
        }
    process->name[i] = '\0';
}

Process newProcess(ProcessEntryPoint entryPoint, char *name, uint64_t stackSize,
                   pid_t pid, uint8_t priorityLevel, int argc, char **argv) {
    Process process = (Process) mem_alloc(sizeof(processCDT));
    if (!process) return 0;
    process->stackBase = (uint64_t *) mem_alloc(stackSize);
    if (!process->stackBase) { mem_free(process); return 0; }

    process->stackSize = stackSize;
    process->rsp       = (uint64_t *)((uint64_t)process->stackBase + stackSize);
    process->pid       = pid;
    process->ppid      = INIT_PID;       /* el scheduler lo sobreescribe si corresponde */
    process->rip       = (uint64_t)entryPoint;
    process->state     = READY;

    process->priorityLevel    = priorityLevel;
    process->remainingQuantums = priorityLevel;

    process->children            = newQueue();
    process->terminatedChildren  = newQueue();
    process->terminated          = newSemaphore(0);
    process->childTerminated     = newSemaphore(0);

    copyProcessName(process, name);

    process->argc = argc;
    process->argv = argv;

    return process;
}

void freeProcess(Process process) {
    if (!process) return;
    mem_free(process->stackBase);
    freeQueue(process->children);
    freeQueue(process->terminatedChildren);
    freeSemaphore(process->terminated);
    freeSemaphore(process->childTerminated);
    mem_free(process);
}

uint64_t *getRSP(Process process)            { return process->rsp; }
void      setRSP(Process process, uint64_t *newRSP) { process->rsp = newRSP; }

uint64_t *getProcessStackBase(Process process) { return process->stackBase; }
uint64_t  getRIP(Process process)              { return process->rip; }

int       getArgc(Process process) { return process->argc; }
char    **getArgv(Process process) { return process->argv; }

uint64_t getStackMisalignment(Process process) {
    if (!process || !process->stackBase || !process->rsp) return 0;
    uint64_t offset = (uint64_t)process->rsp - (uint64_t)process->stackBase;
    uint64_t mis = offset % sizeof(uint64_t);
    return mis ? sizeof(uint64_t) - mis : 0;
}

pid_t getPID(Process process)              { return process->pid; }
pid_t getPPID(Process process)             { return process->ppid; }
void  setPPID(Process process, pid_t ppid) { process->ppid = ppid; }

char *getProcessName(Process process) { return process->name; }

State getState(Process process)             { return process->state; }
void  setState(Process process, State s)    { process->state = s; }

uint8_t getPriority(Process process)        { return process->priorityLevel; }
void    setPriority(Process process, uint8_t priority) {
    process->priorityLevel = priority;
    process->remainingQuantums = priority;
}

int  hasQuantums(Process process)    { return process->remainingQuantums > 0; }
void useQuantum(Process process)     { if (process->remainingQuantums > 0) process->remainingQuantums--; }
void resetQuantums(Process process)  { process->remainingQuantums = process->priorityLevel; }

Queue getChildren(Process process)            { return process->children; }
Queue getTerminatedChildren(Process process)  { return process->terminatedChildren; }

int addChild(Process parent, pid_t childPid) {
    return enqueue(getChildren(parent), (void*)(intptr_t)childPid);
}

int removeChild(Process parent, pid_t childPid) {
    return removeFromQueue(getChildren(parent), (void*)(intptr_t)childPid, comparePIDs);
}

Semaphore getTerminated(Process process)        { return process->terminated; }
Semaphore getChildTerminated(Process process)   { return process->childTerminated; }
