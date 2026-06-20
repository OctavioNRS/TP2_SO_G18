/*
 * Scheduler.
 *
 * Estructuras:
 *   - processTable[pid] -> Process (puntero opaco); slot vacío = NULL.
 *   - readyQueue: cola FIFO de PIDs en estado READY.
 *   - foregroundSet[pid]: 1 si el proceso está en foreground (lo lanzó el shell sin '&').
 *
 * Procesos especiales:
 *   - IDLE_PID (0): proceso while(1)_hlt; corre cuando readyQueue está vacía.
 *   - INIT_PID (1): proceso que llama a wait_any en loop, reapeando huérfanos.
 */
#include <scheduler.h>
#include <process.h>
#include <fileAccess.h>
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
static uint8_t foregroundSet[MAX_PROCESSES] = {0};
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

    uint64_t mis = getStackMisalignment(process);
    sp = (uint64_t *)((uint64_t)sp - mis);

    *(--sp) = (uint64_t) KERNEL_SS;
    *(--sp) = stackPointerValue;
    *(--sp) = (uint64_t) INITIAL_RFLAGS;
    *(--sp) = (uint64_t) KERNEL_CS;
    *(--sp) = (uint64_t) getRIP(process);

    for (int i = 0; i < 5; i++) *(--sp) = 0;

    *(--sp) = (uint64_t)(uint32_t) getArgc(process);     /* rdi */
    *(--sp) = (uint64_t) getArgv(process);               /* rsi */

    for (int i = 0; i < 8; i++) *(--sp) = 0;

    setRSP(process, sp);
}

static pid_t findFreePid(void) {
    for (pid_t i = 0; i < MAX_PROCESSES; i++)
        if (processTable[i] == 0) return i;
    return -1;
}

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
        copyFdTable(process, currentProcess);   /* hereda fds + addFileRef */
    } else {
        setPPID(process, INIT_PID);
    }
    prepareStack(process);
    setState(process, READY);
    processTable[pid] = process;
    enqueue(readyQueue, (void *)(intptr_t)pid);
    return pid;
}

void initializeScheduler(ProcessEntryPoint initProcessEntry) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        processTable[i] = 0;
        foregroundSet[i] = 0;
    }
    readyQueue = newQueue();
    currentProcess = 0;
    schedulerEnabled = 0;

    startNewProcessWithPid(idleProcessEntry, "idle", 1, IDLE_PID, 0, 0);
    startNewProcessWithPid(initProcessEntry, "init", 1, INIT_PID, 0, 0);
}

void enableScheduler(void) { schedulerEnabled = 1; }

pid_t startNewProcess(ProcessEntryPoint entryPoint, char *name,
                      uint8_t priorityLevel, int argc, char **argv) {
    pid_t pid = findFreePid();
    if (pid < 0) return -1;
    return startNewProcessWithPid(entryPoint, name, priorityLevel, pid, argc, argv);
}

pid_t startNewProcessWithFds(ProcessEntryPoint entryPoint, char *name,
                             uint8_t priorityLevel, int argc, char **argv,
                             fd_override_t *overrides, int overrideCount) {
    pid_t pid = startNewProcess(entryPoint, name, priorityLevel, argc, argv);
    if (pid < 0 || !overrides) return pid;
    Process child = processTable[pid];
    if (!child) return pid;
    for (int i = 0; i < overrideCount; i++) {
        fd_override_t o = overrides[i];
        if (o.targetFd == -1) {
            closeFd(child, o.affectedFd);
        } else {
            dupFd(child, o.targetFd, o.affectedFd);
        }
    }
    return pid;
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
        buffer[count].pid          = getPID(p);
        buffer[count].ppid         = getPPID(p);
        buffer[count].state        = getState(p);
        buffer[count].name         = getProcessName(p);
        buffer[count].priority     = getPriority(p);
        buffer[count].inForeground = foregroundSet[i];
        buffer[count].rsp          = getRSP(p);
        buffer[count].stackBase    = getProcessStackBase(p);
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

    if (getState(p) != BLOCKED) return -1;
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

static void reapProcess(pid_t pid) {
    if (pid < 0 || pid >= MAX_PROCESSES) return;
    if (!processTable[pid]) return;
    freeProcess(processTable[pid]);
    processTable[pid] = 0;
    foregroundSet[pid] = 0;
}

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

    closeFdsOf(pid);
    foregroundSet[pid] = 0;
    notifyTermination(process, parent, pid);
    transferOrphanChildren(process);

    if (currentProcess && getPID(currentProcess) == pid) forceSchedulerCall();
    return 0;
}

void killFromIsr(pid_t pid) {
    if (pid < 0 || pid >= MAX_PROCESSES) return;
    if (isUnkillable(pid)) return;
    Process process = processTable[pid];
    if (!process) return;
    pid_t   ppid   = getPPID(process);
    Process parent = (ppid >= 0 && ppid < MAX_PROCESSES) ? processTable[ppid] : 0;
    if (!parent) return;

    closeFdsOf(pid);
    foregroundSet[pid] = 0;
    notifyTermination(process, parent, pid);
    transferOrphanChildren(process);
}

void wait_pid(pid_t pid) {
    if (pid < 0 || pid >= MAX_PROCESSES) return;
    Process process = processTable[pid];
    if (!process) return;
    pid_t   ppid   = getPPID(process);
    Process parent = (ppid >= 0 && ppid < MAX_PROCESSES) ? processTable[ppid] : 0;

    semWait(getTerminated(process));

    if (parent) {
        semTryWait(getChildTerminated(parent));
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

void reap_zombies(void) {
    if (!currentProcess) return;

    while (semTryWait(getChildTerminated(currentProcess)) == 0) {
        pid_t pid = (pid_t)(intptr_t) pollQueue(getTerminatedChildren(currentProcess));
        if (pid < 0 || pid >= MAX_PROCESSES) continue;
        removeChild(currentProcess, pid);
        reapProcess(pid);
    }
}

FileAccess getCurrentFd(int fd) {
    if (!currentProcess) return 0;
    return getFd(currentProcess, fd);
}

int addCurrentFd(FileAccess fa) {
    if (!currentProcess) return -1;
    return addFd(currentProcess, fa);
}

int openCurrentFd(FileAccess fa, int fd) {
    if (!currentProcess) return -1;
    return openFd(currentProcess, fa, fd);
}

int closeCurrentFd(int fd) {
    if (!currentProcess) return -1;
    return closeFd(currentProcess, fd);
}

int dupCurrentFd(int oldFd, int newFd) {
    if (!currentProcess) return -1;
    return dupFd(currentProcess, oldFd, newFd);
}

int reopenCurrentFd(int fd) {
    if (!currentProcess) return -1;
    return reopenFd(currentProcess, fd);
}

int64_t readFdOnPID(pid_t pid, int fd, char *buf, uint64_t count) {
    if (pid < 0 || pid >= MAX_PROCESSES) return -1;
    return readFd(processTable[pid], fd, buf, count);
}

int64_t writeFdOnPID(pid_t pid, int fd, const char *buf, uint64_t count) {
    if (pid < 0 || pid >= MAX_PROCESSES) return -1;
    return writeFd(processTable[pid], fd, buf, count);
}

int requestEofOnPID(pid_t pid, int fd) {
    if (pid < 0 || pid >= MAX_PROCESSES) return -1;
    return requestEofOf(processTable[pid], fd);
}

FileAccess getFdOnPID(pid_t pid, int fd) {
    if (pid < 0 || pid >= MAX_PROCESSES) return 0;
    return getFd(processTable[pid], fd);
}

int addFdToPID(pid_t pid, FileAccess fa) {
    if (pid < 0 || pid >= MAX_PROCESSES) return -1;
    return addFd(processTable[pid], fa);
}

int closeFdOnPID(pid_t pid, int fd) {
    if (pid < 0 || pid >= MAX_PROCESSES) return -1;
    return closeFd(processTable[pid], fd);
}

int dupFdOnPID(pid_t pid, int oldFd, int newFd) {
    if (pid < 0 || pid >= MAX_PROCESSES) return -1;
    return dupFd(processTable[pid], oldFd, newFd);
}

void closeFdsOf(pid_t pid) {
    if (pid < 0 || pid >= MAX_PROCESSES) return;
    closeAllFDs(processTable[pid]);
}

int64_t read(int fd, char *buf, uint64_t count) {
    pid_t pid = getCurrentPID();
    if (pid < 0) return -1;
    return readFdOnPID(pid, fd, buf, count);
}

int64_t write(int fd, const char *buf, uint64_t count) {
    pid_t pid = getCurrentPID();
    if (pid < 0) return -1;
    return writeFdOnPID(pid, fd, buf, count);
}


int addToFG(pid_t pid) {
    if (pid < 0 || pid >= MAX_PROCESSES) return -1;
    if (!processTable[pid] || foregroundSet[pid]) return -1;
    foregroundSet[pid] = 1;
    return 0;
}

int removeFromFG(pid_t pid) {
    if (pid < 0 || pid >= MAX_PROCESSES) return -1;
    if (!foregroundSet[pid]) return -1;
    foregroundSet[pid] = 0;
    return 0;
}

int isInFG(pid_t pid) {
    if (pid < 0 || pid >= MAX_PROCESSES) return 0;
    return foregroundSet[pid];
}

void forEachInFG(ActionOnPID action) {
    if (!action) return;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (foregroundSet[i]) action((pid_t) i);
    }
}


static pid_t getNextPID(void) {
    if (isQueueEmpty(readyQueue)) return IDLE_PID;
    return (pid_t)(intptr_t) pollQueue(readyQueue);
}

uint64_t schedule(uint64_t prevRSP) {
    timer_handler();
    if (!schedulerEnabled) return prevRSP;
    if (!currentProcess) {
        pid_t nextPid = getNextPID();
        currentProcess = processTable[nextPid];
        setState(currentProcess, RUNNING);
        resetQuantums(currentProcess);
        useQuantum(currentProcess);
        return (uint64_t) getRSP(currentProcess);
    }

    setRSP(currentProcess, (uint64_t *) prevRSP);

    if (getState(currentProcess) == RUNNING) {
        if (hasQuantums(currentProcess)) {
            useQuantum(currentProcess);
            return prevRSP;
        }
        setState(currentProcess, READY);
        enqueue(readyQueue, (void *)(intptr_t) getPID(currentProcess));
    }

    pid_t nextPid = getNextPID();
    currentProcess = processTable[nextPid];
    setState(currentProcess, RUNNING);
    resetQuantums(currentProcess);
    useQuantum(currentProcess);
    return (uint64_t) getRSP(currentProcess);
}
