/*
 * process.c — Implementación del TAD de proceso (PCB).
 */
#include <process.h>
#include <memoryManager.h>
#include <queue.h>
#include <semaphore.h>
#include <lib.h>
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

    FileAccess fdTable[MAX_FDS];
    uint8_t    eofRequests[MAX_FDS];   /* 1 si Ctrl+D pidió EOF en ese fd */
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

static char **deepCopyArgv(int argc, char **argv) {
    if (argc <= 0 || argv == 0) return 0;
    char **out = (char **) mem_alloc(sizeof(char *) * (uint64_t) argc);
    if (!out) return 0;
    for (int i = 0; i < argc; i++) out[i] = 0;
    for (int i = 0; i < argc; i++) {
        if (!argv[i]) continue;
        uint64_t len = strlen(argv[i]);
        out[i] = (char *) mem_alloc(len + 1);
        if (!out[i]) {
            for (int j = 0; j < i; j++) if (out[j]) mem_free(out[j]);
            mem_free(out);
            return 0;
        }
        for (uint64_t k = 0; k <= len; k++) out[i][k] = argv[i][k];
    }
    return out;
}

static void freeArgv(int argc, char **argv) {
    if (!argv) return;
    for (int i = 0; i < argc; i++) {
        if (argv[i]) mem_free(argv[i]);
    }
    mem_free(argv);
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
    process->ppid      = INIT_PID;
    process->rip       = (uint64_t)entryPoint;
    process->state     = READY;

    process->priorityLevel     = priorityLevel;
    process->remainingQuantums = priorityLevel;

    process->children           = newQueue();
    process->terminatedChildren = newQueue();
    process->terminated         = newSemaphore(0);
    process->childTerminated    = newSemaphore(0);

    copyProcessName(process, name);

    process->argc = (argc > 0 && argv != 0) ? argc : 0;
    process->argv = (argc > 0 && argv != 0) ? deepCopyArgv(argc, argv) : 0;
    if (process->argc > 0 && !process->argv) {
        /* Falló la copia de argv. Liberamos lo ya creado. */
        freeQueue(process->children);
        freeQueue(process->terminatedChildren);
        freeSemaphore(process->terminated);
        freeSemaphore(process->childTerminated);
        mem_free(process->stackBase);
        mem_free(process);
        return 0;
    }

    for (int i = 0; i < MAX_FDS; i++) {
        process->fdTable[i] = 0;
        process->eofRequests[i] = 0;
    }

    return process;
}

void freeProcess(Process process) {
    if (!process) return;
    freeArgv(process->argc, process->argv);
    mem_free(process->stackBase);
    freeQueue(process->children);
    freeQueue(process->terminatedChildren);
    freeSemaphore(process->terminated);
    freeSemaphore(process->childTerminated);
    mem_free(process);
}

uint64_t *getRSP(Process process)                   { return process->rsp; }
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

State getState(Process process)          { return process->state; }
void  setState(Process process, State s) { process->state = s; }

uint8_t getPriority(Process process) { return process->priorityLevel; }
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

static int isValidFd(int fd) { return fd >= 0 && fd < MAX_FDS; }
static int isOpenFd(Process p, int fd) { return isValidFd(fd) && p->fdTable[fd] != 0; }

FileAccess getFd(Process p, int fd) {
    if (!p || !isValidFd(fd)) return 0;
    return p->fdTable[fd];
}

int openFd(Process p, FileAccess fa, int fd) {
    if (!p || !fa || !isValidFd(fd) || isOpenFd(p, fd)) return -1;
    p->fdTable[fd] = fa;
    p->eofRequests[fd] = 0;
    addFileRef(fa);
    return fd;
}

int addFd(Process p, FileAccess fa) {
    if (!p || !fa) return -1;
    for (int i = 0; i < MAX_FDS; i++) {
        if (p->fdTable[i] == 0) return openFd(p, fa, i);
    }
    return -1;
}

int closeFd(Process p, int fd) {
    if (!p || !isOpenFd(p, fd)) return -1;
    FileAccess fa = p->fdTable[fd];
    p->fdTable[fd] = 0;
    p->eofRequests[fd] = 0;
    removeFileRef(fa);
    return 0;
}

int dupFd(Process p, int oldFd, int newFd) {
    if (!p || !isValidFd(newFd)) return -1;
    if (oldFd == newFd) return newFd;

    if (!isOpenFd(p, oldFd)) {
        closeFd(p, newFd);
        return newFd;
    }
    FileAccess fa = p->fdTable[oldFd];
    if (isOpenFd(p, newFd)) closeFd(p, newFd);
    p->fdTable[newFd] = fa;
    p->eofRequests[newFd] = 0;
    addFileRef(fa);
    return newFd;
}

int reopenFd(Process p, int fd) {
    if (!p || !isOpenFd(p, fd)) return -1;
    FileAccess fa = cloneFile(p->fdTable[fd]);
    if (!fa) return -1;
    int nfd = addFd(p, fa);
    if (nfd < 0) removeFileRef(fa);   
    return nfd;
}

int64_t readFd(Process p, int fd, char *buf, uint64_t count) {
    if (!p || !isOpenFd(p, fd)) return -1;

    if (p->eofRequests[fd]) {
        p->eofRequests[fd] = 0;
        return 0;
    }

    while (1) {
        int64_t n = fileRead(p->fdTable[fd], buf, count);
        if (n <= 0) return n;
        int64_t out = 0;
        for (int64_t i = 0; i < n; i++)
            if (buf[i] != '\0') buf[out++] = buf[i];
        if (out > 0) return out;
        if (p->eofRequests[fd]) {
            p->eofRequests[fd] = 0;
            return 0;
        }
    }
}

int64_t writeFd(Process p, int fd, const char *buf, uint64_t count) {
    if (!p || !isOpenFd(p, fd)) return -1;
    return fileWrite(p->fdTable[fd], buf, count);
}

int requestEofOf(Process p, int fd) {
    if (!p || !isOpenFd(p, fd)) return -1;
    p->eofRequests[fd] = 1;
    return 0;
}

void closeAllFDs(Process p) {
    if (!p) return;
    for (int i = 0; i < MAX_FDS; i++)
        if (p->fdTable[i]) closeFd(p, i);
}

void copyFdTable(Process target, Process source) {
    if (!target || !source) return;
    for (int i = 0; i < MAX_FDS; i++) {
        if (target->fdTable[i]) closeFd(target, i);
        if (source->fdTable[i]) openFd(target, source->fdTable[i], i);
    }
}
