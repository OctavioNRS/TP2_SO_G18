#include <tests.h>
#include <syscalls.h>
#include <cstandard.h>

#define SEM_ID "sem"

int64_t global; // shared memory

void slowInc(int64_t *p, int64_t inc) {
  uint64_t aux = *p;
  if ((randomNext() % 100) < 30)
    sys_yield(); // This makes the race condition highly probable
  aux += inc;
  *p = aux;
}

static int my_process_inc(int argc, char **argv) {
  uint64_t n;
  int8_t inc;
  int8_t use_sem;

  if (argc != 4)
    return -1;

  if ((n = stringToInt(argv[1])) <= 0)
    return -1;
  if ((inc = stringToInt(argv[2])) == 0)
    return -1;
  if ((use_sem = stringToInt(argv[3])) < 0)
    return -1;

  if (use_sem)
    if (sys_sem_open(SEM_ID, 1) < 0) {
      printf("test_sync: ERROR opening semaphore\n");
      sys_exit();
      return -1;
    }

  uint64_t i;
  for (i = 0; i < n; i++) {
    if (use_sem)
      sys_sem_wait(SEM_ID);
    slowInc(&global, inc);
    if (use_sem)
      sys_sem_post(SEM_ID);
  }

  if (use_sem)
    sys_sem_close(SEM_ID);

  sys_exit();
  return 0;
}

int test_sync_command(int argc, char **argv) { //{test_sync, n_procesos, n_iter, use_sem}
  if (argc != 4)
    return -1;

  int64_t n_procesos = stringToInt(argv[1]);
  if (n_procesos <= 0)
    return -1;

  uint64_t pids[2 * n_procesos];

  char *argvDec[] = {"my_process_inc", argv[2], "-1", argv[3], NULL};
  char *argvInc[] = {"my_process_inc", argv[2], "1", argv[3], NULL};

  global = 0;

  uint64_t i;
  for (i = 0; i < (uint64_t)n_procesos; i++) {
    pids[i] = sys_create_process(my_process_inc, 4, argvDec, "my_process_inc");
    pids[i + n_procesos] = sys_create_process(my_process_inc, 4, argvInc, "my_process_inc");
  }

  for (i = 0; i < (uint64_t)n_procesos; i++) {
    sys_waitpid(pids[i]);
    sys_waitpid(pids[i + n_procesos]);
  }

  printf("Final value: %d\n", global);

  sys_exit();
  return 0;
}