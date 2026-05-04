#ifndef _KEYBOARD_DRIVER_H
#define _KEYBOARD_DRIVER_H
#include <stdint.h>

enum FileDescriptor {
    STDIN = 0,
    STDOUT = 1,
    STDERR = 2
};

// Function to get a key press and return the ASCII character
char getKeyPress(uint8_t scancode);

int isPressed(uint8_t keycode);

void keyboardInterrupt();
uint64_t read(int fd, char *buf, uint64_t count);
uint64_t write(int fd, const char *buf, uint64_t count);

#endif 