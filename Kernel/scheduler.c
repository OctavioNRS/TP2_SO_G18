/*
 * Scheduler 
 *
 * Estructuras:
 *   - processTable[pid] -> Process (puntero opaco); slot vacío = NULL.
 *   - readyQueue: cola FIFO de PIDs en estado READY
 *
 * Procesos especiales:
 *   - IDLE_PID (0): proceso while(1)_hlt; corre cuando readyQueue está vacía.
 *   - INIT_PID (1): proceso que llama a wait_any en loop, reapeando huérfanos.
 *
 */
#include <scheduler.h>
#include <process.h>
#include <queue.h>
#include <semaphore.h>
#include <memoryManager.h>
#include <interrupts.h>     
#include <timer.h>         
#include <stdint.h>

#define KERNEL_CS       0x8
#define KERNEL_SS       0x0
#define INITIAL_RFLAGS  0x202

static Process processTable[MAX_PROCESSES] = {0};
static Process currentProcess  = 0;
static Queue   readyQueue      = 0;
static int     schedulerEnabled = 0;

/* Idle: hlt en loop hasta que llega la próxima interrupción. */
static int idleProcessEntry(int argc, char **argv) {
    (void)argc; (void)argv;
    while (1) _hlt();
    return 0;
}

static void prepareStack(Process process) {
    uint64_t *sp = getRSP(process);
    uint64_t stackPointerValue = (uint64_t) sp;

    /* Alineación si hace falta. */
    uint64_t mis = getStackMisalignment(process);
    sp = (uint64_t *)((uint64_t)sp - mis);

    /* Marco iretq. */
    *(--sp) = (uint64_t) KERNEL_SS;
    *(--sp) = stackPointerValue;            
    *(--sp) = (uint64_t) INITIAL_RFLAGS;
    *(--sp) = (uint64_t) KERNEL_CS;
    *(--sp) = (uint64_t) getRIP(process);    

    /* rax,rbx,rcx,rdx,rbp */
    for (int i = 0; i < 5; i++) *(--sp) = 0;

    *(--sp) = (uint64_t)(uint32_t) getArgc(process);     /* rdi */
    *(--sp) = (uint64_t) getArgv(process);               /* rsi */
    
    /* r8..r15 */
    for (int i = 0; i < 8; i++) *(--sp) = 0;

    setRSP(process, sp);
}

/* Encuentra el primer PID libre. Devuelve -1 si la tabla está llena. */
static pid_t findFreePid(void) {
    for (pid_t i = 0; i < MAX_PROCESSES; i++)
        if (processTable[i] == 0) return i;
    return -1;
}

/* Crea un proceso con un PID específico (uso interno: idle/init). */
static pid_t startNewProcessWithPid(ProcessEntryPoint entryPoint, char *name,
                                    uint8_t priorityLevel, pid_t pid,
                                    int argc, char **argv) {
    if (pid < 0 || pid >= MAX_PROCESSES) return -1;
    if (processTable[pid] != 0) return -1;
    Process process = newProcess(entryPoint, name, DEFAULT_STACK_SIZE, pid, priorityLevel,
                                 argc, argv);
    if (!process) return -1;
    if (currentProcess && getPID(currentProcess) != IDLE_PID) {
        setPPID(process, getPID(currentProcess));
        addChild(currentProcess, pid);
    } else {
        setPPID(process, INIT_PID);    /* idle/init/primer proceso: padre = init */
    }
    prepareStack(process);
    setState(process, READY);
    processTable[pid] = process;
    enqueue(readyQueue, (void *)(intptr_t)pid);
    return pid;
}

void initializeScheduler(ProcessEntryPoint initProcessEntry) {
    for (int i = 0; i < MAX_PROCESSES; i++) processTable[i] = 0;
    readyQueue = newQueue();
    currentProcess = 0;
    schedulerEnabled = 0;

    /* idle: PID 0. */
    startNewProcessWithPid(idleProcessEntry, "idle", 1, IDLE_PID, 0, 0);
    /* init: PID 1, llama a wait_any en loop reapeando huérfanos. */
    startNewProcessWithPid(initProcessEntry, "init", 1, INIT_PID, 0, 0);
}

void enableScheduler(void) { schedulerEnabled = 1; }

pid_t startNewProcess(ProcessEntryPoint entryPoint, char *name,
                      uint8_t priorityLevel, int argc, char **argv) {
    pid_t pid = findFreePid();
    if (pid < 0) return -1;
    return startNewProcessWithPid(entryPoint, name, priorityLevel, pid, argc, argv);
}

void forceSchedulerCall(void) { _forceTimerInt(); }

pid_t getCurrentPID(void) {
    return currentProcess ? getPID(currentProcess) : -1;
}

int getProcessInfo(ProcessInfo *buffer, int maxCount) {
    int count = 0;
    for (int i = 0; i < MAX_PROCESSES && count < maxCount; i++) {
        Process p = processTable[i];
        if (p == 0) continue;
        buffer[count].pid       = getPID(p);
        buffer[count].ppid      = getPPID(p);
        buffer[count].state     = getState(p);
        buffer[count].name      = getProcessName(p);
        buffer[count].priority  = getPriority(p);
        buffer[count].rsp       = getRSP(p);
        buffer[count].stackBase = getProcessStackBase(p);
        count++;
    }
    return count;
}

void setCurrentBlocked(void) {
    if (currentProcess) setState(currentProcess, BLOCKED);
}

static int isUnkillable(pid_t pid) { return pid == IDLE_PID || pid == INIT_PID; }

int setBlocked(pid_t pid) {
    if (pid < 0 || pid >= MAX_PROCESSES) return -1;
    Process p = processTable[pid];
    if (!p) return -1;
    if (isUnkillable(pid)) return -1;
    State s = getState(p);
    if (s != RUNNING && s != READY) return -1;
    if (s == READY) removeFromQueue(readyQueue, (void *)(intptr_t)pid, comparePIDs);
    setState(p, BLOCKED);
    return 0;
}

int setReady(pid_t pid) {
    if (pid < 0 || pid >= MAX_PROCESSES) return -1;
    Process p = processTable[pid];
    if (!p) return -1;
    State s = getState(p);
    if (s != RUNNING && s != BLOCKED) return -1;
    enqueue(readyQueue, (void *)(intptr_t)pid);
    setState(p, READY);
    return 0;
}

int setPriorityOnPID(pid_t pid, uint8_t priority) {
    if (pid < 0 || pid >= MAX_PROCESSES) return -1;
    if (!processTable[pid] || priority == 0) return -1;
    setPriority(processTable[pid], priority);
    return 0;
}

/* Libera el PCB de un proceso ZOMBIE y libera su PID. */
static void reapProcess(pid_t pid) {
    if (pid < 0 || pid >= MAX_PROCESSES) return;
    if (!processTable[pid]) return;
    freeProcess(processTable[pid]);
    processTable[pid] = 0;
}

/* Reparenta todos los hijos vivos de `process` a init; los que ya son ZOMBIE quedan
 * encolados en terminatedChildren de init para que wait_any los reapee. */
static void transferOrphanChildren(Process process) {
    Queue   children = getChildren(process);
    Process initProc = processTable[INIT_PID];
    if (!initProc) return;
    while (!isQueueEmpty(children)) {
        pid_t childPid = (pid_t)(intptr_t) pollQueue(children);
        if (childPid < 0 || childPid >= MAX_PROCESSES) continue;
        Process child = processTable[childPid];
        if (!child) continue;
        addChild(initProc, childPid);
        setPPID(child, INIT_PID);
        if (getState(child) == ZOMBIE) {
            enqueue(getTerminatedChildren(initProc), (void *)(intptr_t)childPid);
            semPost(getChildTerminated(initProc));
        }
    }
}

/* Notifica al padre que el proceso terminó (postea sus semáforos y encola en su lista). */
static void notifyTermination(Process process, Process parent, pid_t pid) {
    setState(process, ZOMBIE);
    removeFromQueue(readyQueue, (void *)(intptr_t)pid, comparePIDs);
    semPost(getTerminated(process));
    semPost(getChildTerminated(parent));
    enqueue(getTerminatedChildren(parent), (void *)(intptr_t)pid);
}

int terminateProcess(pid_t pid) {
    if (pid < 0 || pid >= MAX_PROCESSES) return -1;
    if (isUnkillable(pid)) return -1;
    Process process = processTable[pid];
    if (!process) return -1;

    pid_t   ppid   = getPPID(process);
    Process parent = (ppid >= 0 && ppid < MAX_PROCESSES) ? processTable[ppid] : 0;
    if (!parent) return -1;

    notifyTermination(process, parent, pid);
    transferOrphanChildren(process);

    /* Si el que terminó es el actual, ceder el CPU (no vuelve nunca a este punto). */
    if (currentProcess && getPID(currentProcess) == pid) forceSchedulerCall();
    return 0;
}

void wait_pid(pid_t pid) {
    if (pid < 0 || pid >= MAX_PROCESSES) return;
    Process process = processTable[pid];
    if (!process) return;
    pid_t   ppid   = getPPID(process);
    Process parent = (ppid >= 0 && ppid < MAX_PROCESSES) ? processTable[ppid] : 0;

    semWait(getTerminated(process));   /* despierta cuando process termine */

    if (parent) {
        removeFromQueue(getTerminatedChildren(parent), (void *)(intptr_t)pid, comparePIDs);
        removeChild(parent, pid);
    }
    reapProcess(pid);
}

void wait_any(void) {
    if (!currentProcess) return;
    semWait(getChildTerminated(currentProcess));
    pid_t pid = (pid_t)(intptr_t) pollQueue(getTerminatedChildren(currentProcess));
    if (pid < 0 || pid >= MAX_PROCESSES) return;
    removeChild(currentProcess, pid);
    reapProcess(pid);
}

/* ---------- Scheduling ---------- */

/* Saca el próximo PID READY de la cola; si está vacía, devuelve idle. */
static pid_t getNextPID(void) {
    if (isQueueEmpty(readyQueue)) return IDLE_PID;
    return (pid_t)(intptr_t) pollQueue(readyQueue);
}

uint64_t schedule(uint64_t prevRSP) {
    timer_handler();
    if (!schedulerEnabled) return prevRSP;
    if (!currentProcess) {
        /* Primer arranque: tomar el primer READY. */
        pid_t nextPid = getNextPID();
        currentProcess = processTable[nextPid];
        setState(currentProcess, RUNNING);
        resetQuantums(currentProcess);
        useQuantum(currentProcess);
        return (uint64_t) getRSP(currentProcess);
    }

    setRSP(currentProcess, (uint64_t *) prevRSP);

    /* Si sigue RUNNING y le queda quantum, no hacemos context switch. */
    if (getState(currentProcess) == RUNNING) {
        if (hasQuantums(currentProcess)) {
            useQuantum(currentProcess);
            return prevRSP;
        }
        setState(currentProcess, READY);
        enqueue(readyQueue, (void *)(intptr_t) getPID(currentProcess));
    }
    /* Si está BLOCKED, ZOMBIE o ya está fuera de la cola: context switch. */

    pid_t nextPid = getNextPID();
    currentProcess = processTable[nextPid];
    setState(currentProcess, RUNNING);
    resetQuantums(currentProcess);
    useQuantum(currentProcess);
    return (uint64_t) getRSP(currentProcess);
}
