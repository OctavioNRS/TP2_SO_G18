#include <tests.h>
#include <syscalls.h>
#include <cstandard.h>
#include <shell.h>   /* printOutput() */

static void t_memset(void *dst, uint8_t v, uint64_t n) {
    uint8_t *p = (uint8_t *)dst;
    for (uint64_t i = 0; i < n; i++) p[i] = v;
}

/* Devuelve -1 si todos los bytes coinciden, o el offset del primer mismatch. */
static int64_t t_memcheck(const void *dst, uint8_t v, uint64_t n) {
    const uint8_t *p = (const uint8_t *)dst;
    for (uint64_t i = 0; i < n; i++)
        if (p[i] != v) return (int64_t)i;
    return -1;
}

#define MM_MAX_BLOCKS 128
typedef struct { void *addr; uint32_t size; } MmReq;

void test_mm_command(char **args, int argCount) {
    if (argCount != 2) {
        sys_write(STDERR, "Usage: test_mm <max_memory_bytes>\n", 34);
        return;
    }
    int64_t parsed = stringToInt(args[1]);
    if (parsed <= 0) { sys_write(STDERR, "test_mm: max_memory invalido\n", 29); return; }
    uint64_t maxMem = (uint64_t)parsed;
    setRngSeed((uint16_t)sys_kernel_time());

    MemoryInfo before;
    sys_mem_status(&before);
    printf("test_mm: inicio (max=%d bytes, heap libre=%d). Tecla para terminar.\n",
           (int)maxMem, (int)before.free);
    printOutput();

    MmReq    reqs[MM_MAX_BLOCKS];
    uint64_t iter = 0;

    while (getc(STDIN) == 0) {
        int      count = 0;
        uint64_t total = 0;

        /* Asignación con clamp defensivo: nunca pasamos maxMem. */
        while (count < MM_MAX_BLOCKS && total < maxMem) {
            uint64_t remaining = maxMem - total;
            uint64_t cap = remaining < 4096 ? remaining : 4096;
            if (cap == 0) break;
            uint32_t size = (uint32_t)randomRange(1, (int64_t)cap);
            if (size > cap) size = (uint32_t)cap;
            void *p = sys_malloc(size);
            if (!p) break;
            reqs[count].addr = p;
            reqs[count].size = size;
            total += size;
            count++;
        }

        for (int i = 0; i < count; i++)
            t_memset(reqs[i].addr, (uint8_t)(i & 0xFF), reqs[i].size);

        for (int i = 0; i < count; i++) {
            int64_t bad = t_memcheck(reqs[i].addr, (uint8_t)(i & 0xFF), reqs[i].size);
            if (bad >= 0) {
                printf("test_mm ERROR iter %d: bloque %d (size=%d) corrupto en offset %d\n",
                       (int)iter, i, (int)reqs[i].size, (int)bad);
                printOutput();
                for (int j = 0; j < count; j++) sys_free(reqs[j].addr);
                return;
            }
        }
        for (int i = 0; i < count; i++) sys_free(reqs[i].addr);

        printf("iter %d OK: %d bloques, %d bytes\n", (int)iter, count, (int)total);
        printOutput();
        iter++;
    }
    printf("test_mm: terminado (iteraciones OK: %d)\n", (int)iter);
    printOutput();
}

static int endlessLoop(int argc, char **argv) {
    (void)argc; (void)argv;
    while (1) ;
    return 0;
}

typedef enum { PR_RUNNING, PR_BLOCKED, PR_KILLED } PrSt;
typedef struct { int16_t pid; PrSt state; } PrReq;

void test_proc_command(char **args, int argCount) {
    if (argCount != 2) {
        sys_write(STDERR, "Usage: test_proc <max_processes>\n", 33);
        return;
    }
    int64_t parsed = stringToInt(args[1]);
    if (parsed <= 0) { sys_write(STDERR, "test_proc: invalido\n", 20); return; }
    int max = (int)parsed;
    if (max > 30) max = 30;   /* respeta cupo del kernel */

    PrReq *reqs = (PrReq *)sys_malloc(max * sizeof(PrReq));
    if (!reqs) { sys_write(STDERR, "test_proc: sin memoria\n", 23); return; }
    setRngSeed((uint16_t)sys_kernel_time());

    printf("test_proc: rondas de %d procesos. Tecla para terminar.\n", max);
    printOutput();

    uint64_t round = 0;
    while (getc(STDIN) == 0) {
        int alive = 0;

        for (int i = 0; i < max; i++) {
            int64_t pid = sys_create_process(endlessLoop, 0, 0, "endless_loop");
            if (pid < 0) {
                printf("test_proc ERROR: no se pudo crear (slot %d)\n", i);
                printOutput();
                sys_free(reqs);
                return;
            }
            reqs[i].pid = (int16_t)pid;
            reqs[i].state = PR_RUNNING;
            alive++;
        }

        while (alive > 0) {
            for (int i = 0; i < max; i++) {
                if (reqs[i].state == PR_KILLED) continue;
                int action = (int)(randomNext() % 3); /* 0=kill, 1=block, 2=nada */
                if (action == 0) {
                    if (sys_kill(reqs[i].pid) < 0) {
                        printf("test_proc ERROR: kill pid %d\n", (int)reqs[i].pid);
                        printOutput();
                        sys_free(reqs);
                        return;
                    }
                    sys_waitpid(reqs[i].pid);
                    reqs[i].state = PR_KILLED;
                    alive--;
                } else if (action == 1 && reqs[i].state == PR_RUNNING) {
                    if (sys_block(reqs[i].pid) < 0) {
                        printf("test_proc ERROR: block pid %d\n", (int)reqs[i].pid);
                        printOutput();
                        sys_free(reqs);
                        return;
                    }
                    reqs[i].state = PR_BLOCKED;
                }
            }
            for (int i = 0; i < max; i++) {
                if (reqs[i].state == PR_BLOCKED && (randomNext() & 1)) {
                    sys_unblock(reqs[i].pid);   /* desbloquea explícitamente */
                    reqs[i].state = PR_RUNNING;
                }
            }
        }
        printf("test_proc: ronda %d completada\n", (int)round);
        printOutput();
        round++;
    }
    printf("test_proc: terminado (rondas OK: %d)\n", (int)round);
    printOutput();
    sys_free(reqs);
}


#define TP_PROCS  3
#define PRIO_LOW  1
#define PRIO_MED  2
#define PRIO_HIGH 4

static volatile uint64_t prioMax = 0;

static int countUpToMax(int argc, char **argv) {
    (void)argc; (void)argv;
    uint64_t v = 0;
    while (v++ != prioMax) ;
    printf("  PID %d termino\n", (int)sys_getpid());
    printOutput();
    sys_exit();      
    return 0;
}

void test_prio_command(char **args, int argCount) {
    if (argCount != 2) {
        sys_write(STDERR, "Usage: test_prio <max_value>\n", 29);
        return;
    }
    int64_t parsed = stringToInt(args[1]);
    if (parsed <= 0) { sys_write(STDERR, "test_prio: invalido\n", 20); return; }
    prioMax = (uint64_t)parsed;

    int16_t  pids[TP_PROCS];
    uint8_t  prios[TP_PROCS] = { PRIO_LOW, PRIO_MED, PRIO_HIGH };

    printf("== Misma prioridad ==\n");
    printOutput();
    for (int i = 0; i < TP_PROCS; i++)
        pids[i] = (int16_t)sys_create_process(countUpToMax, 0, 0, "count");
    for (int i = 0; i < TP_PROCS; i++) sys_waitpid(pids[i]);
    printOutput();

    printf("== Prioridades distintas ==\n");
    for (int i = 0; i < TP_PROCS; i++) {
        pids[i] = (int16_t)sys_create_process(countUpToMax, 0, 0, "count");
        sys_nice(pids[i], prios[i]);
        printf("  pid %d -> prioridad %d\n", (int)pids[i], (int)prios[i]);
    }
    printOutput();
    for (int i = 0; i < TP_PROCS; i++) sys_waitpid(pids[i]);
    printOutput();

    printf("test_prio OK\n");
    printOutput();
}
