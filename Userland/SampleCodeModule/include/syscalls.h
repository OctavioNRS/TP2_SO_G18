#ifndef SYSCALLS_H
#define SYSCALLS_H
#include <stdint.h>

enum FileDescriptor {
    STDIN = 0,
    STDOUT = 1,
    STDERR = 2
};

uint64_t sys_read(int fd, char* buffer, uint64_t count);
uint64_t sys_write(int fd, char* buffer, uint64_t count);
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

void test_invalid_opcode();

#endif