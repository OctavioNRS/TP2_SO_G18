/*
 * syscalls.h — Declaraciones de las syscalls del userland.
 *
 * Cada `sys_*` mapea a su número de syscall en `asm/syscalls.asm`, que las
 * dispara con `int 0x80` y las despacha en `syscallDispatcher.c` del kernel.
 * También define los fds estándar (STDIN/STDOUT/STDERR), los structs públicos
 * (ProcessInfo, MemoryInfo, fd_override_t) y los enums de estado de proceso.
 */
#ifndef SYSCALLS_H
#define SYSCALLS_H
#include <stdint.h>

enum FileDescriptor {
    STDIN  = 0,
    STDOUT = 1,
    STDERR = 2
};

typedef enum { ST_READY = 0, ST_RUNNING, ST_BLOCKED, ST_ZOMBIE } ProcState;

typedef int (*ProcessEntry)(int argc, char **argv);

typedef struct {
    int16_t   pid;
    int16_t   ppid;
    ProcState state;
    char     *name;
    uint8_t   priority;
    uint8_t   inForeground;
    uint64_t *rsp;
    uint64_t *stackBase;
} ProcessInfo;

typedef struct {
    uint64_t total;
    uint64_t occupied;
    uint64_t free;
    uint64_t fragments;
} MemoryInfo;

typedef struct {
    int affectedFd;
    int targetFd;
} fd_override_t;

uint64_t sys_read(int fd, char* buffer, uint64_t count);
uint64_t sys_write(int fd, const char* buffer, uint64_t count);
uint64_t sys_draw_rect(uint64_t color, uint64_t x, uint64_t y, uint64_t width, uint64_t height);
uint64_t sys_draw_circle(uint64_t color, uint64_t centerX, uint64_t centerY, uint64_t radius, uint64_t fill);
uint64_t sys_draw_char(unsigned char c, uint64_t x, uint64_t y, uint64_t fgColor, uint64_t bgColor, int scale);
uint64_t sys_screen_width();
uint64_t sys_screen_height();
uint64_t sys_background(uint32_t color);
uint64_t sys_video_aux(uint8_t *auxBuffer, uint32_t width, uint32_t height);
uint64_t sys_render_aux();
uint64_t sys_sleep(uint64_t millis);
uint64_t sys_draw_string(char *string, uint64_t x, uint64_t y, uint64_t fgcolor, uint64_t bgcolor, int scale);
uint64_t sys_get_key(unsigned char keycode);
uint64_t sys_play_sound(uint64_t frequency, uint64_t duration_ms);
uint64_t sys_date(uint8_t *weekday, uint8_t *day, uint8_t *month, uint8_t *year);
uint64_t sys_time(uint8_t *hours, uint8_t *minutes, uint8_t *seconds);
uint64_t sys_kernel_time();
uint64_t sys_print_registers();

void*    sys_malloc(uint64_t size);
uint64_t sys_free(void *ptr);
uint64_t sys_mem_status(MemoryInfo *info);

int16_t  sys_getpid();
int64_t  sys_create_process(ProcessEntry entry, int argc, char **argv, char *name);
int64_t  sys_kill(int16_t pid);
int64_t  sys_nice(int16_t pid, uint8_t priority);
int64_t  sys_block(int16_t pid);
int64_t  sys_unblock(int16_t pid);
int64_t  sys_yield();
int64_t  sys_waitpid(int16_t pid);
uint64_t sys_ps(ProcessInfo *buffer, int max);
void     sys_exit();

int64_t  sys_sem_open(char *name, int initialValue);
int64_t  sys_sem_close(char *name);
int64_t  sys_sem_wait(char *name);
int64_t  sys_sem_post(char *name);

int64_t  sys_pipe(int fds[2]);
int64_t  sys_close(int fd);
int64_t  sys_dup(int oldFd, int newFd);
int64_t  sys_create_process_with_fds(ProcessEntry entry, int argc, char **argv, char *name,
                                     fd_override_t *overrides, int overrideCount);
int64_t  sys_foreground(int16_t pid);
int64_t  sys_reopen(int fd);
uint64_t sys_scroll_up(uint64_t pixelRows, uint32_t fillColor);

void     sys_reap_zombies(void);

void test_invalid_opcode();

#endif
