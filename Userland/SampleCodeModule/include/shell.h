#ifndef CONSOLE_H
#define CONSOLE_H
#include <stdint.h>

#define MAX_COMMAND_LENGTH 128

void initializeShell();
void inputMode(int acceptsEmpty);
void clearScreen();

uint32_t getBackgroundColor();
void setBackgroundColor(uint32_t color);
uint32_t getForegroundColor();
void setForegroundColor(uint32_t color);
void setHighlightColor(uint32_t color);
void setScale(int scale);
void printOutput();

void resetBackgroundColor();
void resetForegroundColor();
void resetHighlightColor();

#endif