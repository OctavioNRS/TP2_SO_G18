/*
 * scheduler.c — Procesos, context switching y scheduling (Parte 3).
 *
 * Scheduling: Round Robin con prioridades. La prioridad determina el tamaño del
 * quantum (cantidad de ticks consecutivos que el proceso corre antes de ceder el
 * CPU): a mayor prioridad, más quantum y por lo tanto más tiempo de CPU.
 *
 * El cambio de contexto ocurre en el handler del timer (_irq00Handler): guarda el
 * RSP del proceso saliente y carga el del entrante (ver schedule()). Un proceso que
 * se bloquea/termina/cede llama a yield() (int 0x20), que dispara el mismo camino.
 */
#include <scheduler.h>
#include <process.h>
#include <registers.h>      /* StackFrame */
#include <memoryManager.h>
#include <interrupts.h>     /* _hlt, _forceTimerInt */
#include <timer.h>          /* timer_handler */

#define KERNEL_CS        0x08
#define KERNEL_SS        0x00
#define INITIAL_RFLAGS   0x202    /* IF=1 (interrupciones on) + bit 1 reservado */

static PCB      processes[MAX_PROCESSES];
static int      currentIndex = -1;   /* índice del proceso en ejecución; -1 = ninguno */
static int      idleIndex    = -1;
static uint32_t nextPid      = 0;
static int      schedulerEnabled = 0; 


static void copyName(char *dst, const char *src) {
    int i = 0;
    if (src != NULL)
        for (; src[i] != '\0' && i < PROCESS_NAME_MAX - 1; i++) dst[i] = src[i];
    dst[i] = '\0';
}

static int findFreeSlot(void) {
    for (int i = 0; i < MAX_PROCESSES; i++)
        if (processes[i].state == UNUSED) return i;
    return -1;
}

static int findByPid(uint32_t pid) {
    for (int i = 0; i < MAX_PROCESSES; i++)
        if (processes[i].state != UNUSED && processes[i].pid == pid) return i;
    return -1;
}

static uint8_t clampPriority(int priority) {
    if (priority < MIN_PRIORITY) return MIN_PRIORITY;
    if (priority > MAX_PRIORITY) return MAX_PRIORITY;
    return (uint8_t)priority;
}

/* Despierta a los procesos que esperaban (waitpid) la terminación de deadPid. */
static void wakeWaiters(uint32_t deadPid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i].state == BLOCKED && processes[i].waitingForPid == (int32_t)deadPid) {
            processes[i].waitingForPid = -1;
            processes[i].state = READY;
        }
    }
}

/* Envoltorio: corre la función del proceso y, al retornar, lo termina. */
static void processWrapper(ProcessEntry entry, int argc, char **argv) {
    int ret = entry(argc, argv);
    exitProcess(ret);
    while (1) _hlt();
}

/* Proceso idle: se ejecuta cuando no hay ningún otro listo. */
static int idleProcess(int argc, char **argv) {
    (void)argc; (void)argv;
    while (1) _hlt();
    return 0;
}

/* ---------- API: ciclo de vida ---------- */

void initScheduler(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) processes[i].state = UNUSED;
    currentIndex = -1;
    nextPid = 0;
    idleIndex = -1;
    schedulerEnabled = 0;
    createProcess(idleProcess, 0, 0, "idle");   /* queda en el slot 0 */
    idleIndex = 0;
}

void enableScheduler(void) {
    schedulerEnabled = 1;
}

int32_t createProcess(ProcessEntry entry, int argc, char **argv, const char *name) {
    int slot = findFreeSlot();
    if (slot < 0) return -1;

    void *stack = mem_alloc(PROCESS_STACK_SIZE);
    if (stack == NULL) return -1;

    PCB *p = &processes[slot];
    p->pid           = nextPid++;
    copyName(p->name, name);
    p->stackBase     = stack;
    p->priority      = DEFAULT_PRIORITY;
    p->quantumLeft   = DEFAULT_PRIORITY;
    p->parentPid     = (currentIndex >= 0) ? processes[currentIndex].pid : 0;
    p->waitingForPid = -1;
    p->retValue      = 0;

    /* Arma el stack inicial: simula un proceso interrumpido listo para iretq. */
    uint8_t    *stackTop = (uint8_t *)stack + PROCESS_STACK_SIZE;
    StackFrame *frame    = (StackFrame *)(stackTop - sizeof(StackFrame));

    uint64_t *raw = (uint64_t *)frame;
    for (uint64_t i = 0; i < sizeof(StackFrame) / sizeof(uint64_t); i++) raw[i] = 0;

    frame->rip    = (uint64_t)processWrapper;
    frame->cs     = KERNEL_CS;
    frame->rflags = INITIAL_RFLAGS;
    frame->rsp    = (uint64_t)stackTop;
    frame->ss     = KERNEL_SS;
    /* argumentos de processWrapper (convención System V: rdi, rsi, rdx) */
    frame->rdi    = (uint64_t)entry;
    frame->rsi    = (uint64_t)(uint32_t)argc;
    frame->rdx    = (uint64_t)argv;

    p->rsp   = frame;
    p->state = READY;
    return (int32_t)p->pid;
}

void exitProcess(int retValue) {
    if (currentIndex < 0) return;
    processes[currentIndex].retValue = retValue;
    processes[currentIndex].state = TERMINATED;
    wakeWaiters(processes[currentIndex].pid);
    yield();                 /* fuerza el cambio de contexto; no vuelve */
    while (1) _hlt();
}

uint32_t getpid(void) {
    if (currentIndex < 0) return 0;
    return processes[currentIndex].pid;
}

void yield(void) {
    _forceTimerInt();        /* int 0x20 -> _irq00Handler -> schedule */
}

/* ---------- API: control de procesos ---------- */

int32_t killProcess(uint32_t pid) {
    int idx = findByPid(pid);
    if (idx < 0 || idx == idleIndex) return -1;
    processes[idx].state = TERMINATED;
    wakeWaiters(processes[idx].pid);
    if (idx == currentIndex) yield();
    return 0;
}

int32_t blockProcess(uint32_t pid) {
    int idx = findByPid(pid);
    if (idx < 0 || idx == idleIndex) return -1;
    if (processes[idx].state == READY || processes[idx].state == RUNNING) {
        processes[idx].state = BLOCKED;
        if (idx == currentIndex) yield();
        return 0;
    }
    return -1;
}

int32_t unblockProcess(uint32_t pid) {
    int idx = findByPid(pid);
    if (idx < 0) return -1;
    if (processes[idx].state == BLOCKED) {
        processes[idx].state = READY;
        return 0;
    }
    return -1;
}

/* Marca un proceso como BLOCKED sin ceder el CPU (uso interno de los semáforos). */
void setBlocked(uint32_t pid) {
    int idx = findByPid(pid);
    if (idx >= 0 && (processes[idx].state == READY || processes[idx].state == RUNNING))
        processes[idx].state = BLOCKED;
}

int32_t toggleBlock(uint32_t pid) {
    int idx = findByPid(pid);
    if (idx < 0 || idx == idleIndex) return -1;
    if (processes[idx].state == BLOCKED) return unblockProcess(pid);
    return blockProcess(pid);
}

int32_t setPriority(uint32_t pid, uint8_t priority) {
    int idx = findByPid(pid);
    if (idx < 0) return -1;
    processes[idx].priority = clampPriority(priority);
    return 0;
}

int32_t waitpid(uint32_t pid) {
    int idx = findByPid(pid);
    if (idx < 0) return -1;                            /* no existe / ya fue reapeado */
    if (processes[idx].state == TERMINATED) return 0;  /* ya terminó                  */
    if (currentIndex < 0) return -1;
    processes[currentIndex].waitingForPid = (int32_t)pid;
    processes[currentIndex].state = BLOCKED;
    yield();                 /* se desbloquea cuando el hijo termina */
    return 0;
}

int listProcesses(ProcessInfo *buffer, int max) {
    int n = 0;
    for (int i = 0; i < MAX_PROCESSES && n < max; i++) {
        if (processes[i].state == UNUSED) continue;
        ProcessInfo *info = &buffer[n++];
        info->pid        = processes[i].pid;
        copyName(info->name, processes[i].name);
        info->state      = processes[i].state;
        info->priority   = processes[i].priority;
        info->rsp        = processes[i].rsp;
        info->stackBase  = processes[i].stackBase;
        info->parentPid  = processes[i].parentPid;
    }
    return n;
}

/* ---------- scheduling ---------- */

/* Libera los stacks de procesos terminados que ya no están en ejecución. */
static void reapTerminated(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i].state == TERMINATED && i != currentIndex) {
            mem_free(processes[i].stackBase);
            processes[i].state = UNUSED;
        }
    }
}

/* Round-robin: próximo proceso READY a partir del actual; idle si no hay otro. */
static int pickNext(void) {
    int start = (currentIndex + 1) % MAX_PROCESSES;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        int idx = (start + i) % MAX_PROCESSES;
        if (idx == idleIndex) continue;
        if (processes[idx].state == READY) return idx;
    }
    return idleIndex;
}

uint64_t schedule(uint64_t rsp) {
    timer_handler();         /* mantiene el conteo de ticks (sleep, tiempo) */
    if (!schedulerEnabled) return rsp;
    reapTerminated();

    if (currentIndex >= 0) {
        PCB *cur = &processes[currentIndex];
        /* Si sigue corriendo y le queda quantum, no conmutamos (prioridad = quantum). */
        if (cur->state == RUNNING && cur->quantumLeft > 0) {
            cur->quantumLeft--;
            return rsp;
        }
        /* Hay que conmutar: guardamos el contexto si el proceso puede reanudarse. */
        if (cur->state == RUNNING || cur->state == BLOCKED) {
            cur->rsp = (void *)rsp;
            if (cur->state == RUNNING) cur->state = READY;
        }
        /* TERMINATED: no se guarda (será reapeado). */
    }

    int next = pickNext();
    currentIndex = next;
    processes[next].state = RUNNING;
    processes[next].quantumLeft = processes[next].priority;
    return (uint64_t)processes[next].rsp;
}
