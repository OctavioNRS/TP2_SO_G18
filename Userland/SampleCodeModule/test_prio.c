#include <tests.h>
#include <syscalls.h>
#include <cstandard.h>

#define TOTAL_PROCESSES 3

#define LOWEST 1
#define MEDIUM 2
#define HIGHEST 4

static int64_t prio[TOTAL_PROCESSES] = {LOWEST, MEDIUM, HIGHEST};

static volatile uint64_t max_value = 0;

static int zero_to_max(int argc, char **argv) {
    (void)argc; (void)argv;
    uint64_t value = 0;

    while (value++ != max_value);

    printf("PROCESS %d DONE!\n", (int)sys_getpid());
    sys_exit();
    return 0;
}

int test_prio_command(int argc, char **argv) {
    if (argc != 2) {
        sys_write(STDERR, "Usage: test_prio <max_value>\n", 29);
        sys_exit();
        return 0;
    }

    int64_t parsed = stringToInt(argv[1]);
    if (parsed <= 0) {
        sys_write(STDERR, "test_prio: invalido\n", 20);
        sys_exit();
        return 0;
    }
    max_value = (uint64_t)parsed;

    int16_t pids[TOTAL_PROCESSES];
    uint64_t i;

    printf("SAME PRIORITY...\n");

    for (i = 0; i < TOTAL_PROCESSES; i++)
        pids[i] = (int16_t)sys_create_process(zero_to_max, 0, 0, "zero_to_max");

    for (i = 0; i < TOTAL_PROCESSES; i++)
        sys_waitpid(pids[i]);

    printf("SAME PRIORITY, THEN CHANGE IT...\n");

    for (i = 0; i < TOTAL_PROCESSES; i++) {
        pids[i] = (int16_t)sys_create_process(zero_to_max, 0, 0, "zero_to_max");
        sys_nice(pids[i], (uint8_t)prio[i]);
        printf("  PROCESS %d NEW PRIORITY: %d\n", (int)pids[i], (int)prio[i]);
    }

    for (i = 0; i < TOTAL_PROCESSES; i++)
        sys_waitpid(pids[i]);

    printf("SAME PRIORITY, THEN CHANGE IT WHILE BLOCKED...\n");

    for (i = 0; i < TOTAL_PROCESSES; i++) {
        pids[i] = (int16_t)sys_create_process(zero_to_max, 0, 0, "zero_to_max");
        sys_block(pids[i]);
        sys_nice(pids[i], (uint8_t)prio[i]);
        printf("  PROCESS %d NEW PRIORITY: %d\n", (int)pids[i], (int)prio[i]);
    }

    for (i = 0; i < TOTAL_PROCESSES; i++)
        sys_unblock(pids[i]);

    for (i = 0; i < TOTAL_PROCESSES; i++)
        sys_waitpid(pids[i]);

    printf("test_prio OK\n");
    sys_exit();
    return 0;
}
