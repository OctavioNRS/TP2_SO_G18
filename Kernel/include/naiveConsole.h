#ifndef NAIVE_CONSOLE_H
#define NAIVE_CONSOLE_H

#include <stdint.h>

void ncPrint(const char * string);
void ncPrintChar(char character);
void ncNewline();
void ncPrintDec(uint64_t value);
void ncPrintHex(uint64_t value);
void ncPrintBin(uint64_t value);
void ncPrintBase(uint64_t value, uint32_t base);
void ncClear();
void ncColorPrintChar(char character, char colors);
void ncColorPrint(const char* string, char colors);
void colorPrint(const char* string, const char* bgColor, const char* fgColor);
void displayCurrentTime();

#endif