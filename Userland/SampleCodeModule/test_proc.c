#include <tests.h>
#include <syscalls.h>
#include <cstandard.h>

enum State { RUNNING,
             BLOCKED,
             KILLED };

typedef struct P_rq {
    int32_t pid;
    enum State state;
} p_rq;

static int endless_loop(int argc, char **argv) {
    (void)argc; (void)argv;
    while (1) ;
    return 0;
}

int test_proc_command(int argc, char **argv) {
    if (argc != 2) {
        sys_write(STDERR, "Usage: test_proc <max_processes>\n", 33);
        sys_exit();
        return 0;
    }

    int64_t parsed = stringToInt(argv[1]);
    if (parsed <= 0) {
        sys_write(STDERR, "test_proc: invalido\n", 20);
        sys_exit();
        return 0;
    }
    uint64_t max_processes = (uint64_t)parsed;

    p_rq p_rqs[max_processes];
    uint8_t rq;
    uint8_t alive = 0;
    uint8_t action;

    while (1) {

        for (rq = 0; rq < max_processes; rq++) {
            p_rqs[rq].pid = (int32_t)sys_create_process(endless_loop, 0, 0, "endless_loop");

            if (p_rqs[rq].pid == -1) {
                printf("test_processes: ERROR creating process\n");
                sys_exit();
                return -1;
            } else {
                p_rqs[rq].state = RUNNING;
                alive++;
            }
        }

        while (alive > 0) {

            for (rq = 0; rq < max_processes; rq++) {
                action = (uint8_t)(randomNext() % 2);

                switch (action) {
                    case 0:
                        if (p_rqs[rq].state == RUNNING || p_rqs[rq].state == BLOCKED) {
                            if (sys_kill((int16_t)p_rqs[rq].pid) == -1) {
                                printf("test_processes: ERROR killing process\n");
                                sys_exit();
                                return -1;
                            }
                            p_rqs[rq].state = KILLED;
                            sys_waitpid((int16_t)p_rqs[rq].pid);
                            alive--;
                        }
                        break;

                    case 1:
                        if (p_rqs[rq].state == RUNNING) {
                            if (sys_block((int16_t)p_rqs[rq].pid) == -1) {
                                printf("test_processes: ERROR blocking process\n");
                                sys_exit();
                                return -1;
                            }
                            p_rqs[rq].state = BLOCKED;
                        }
                        break;
                }
            }

            for (rq = 0; rq < max_processes; rq++)
                if (p_rqs[rq].state == BLOCKED && (randomNext() % 2)) {
                    if (sys_unblock((int16_t)p_rqs[rq].pid) == -1) {
                        printf("test_processes: ERROR unblocking process\n");
                        sys_exit();
                        return -1;
                    }
                    p_rqs[rq].state = RUNNING;
                }
        }
    }
}
