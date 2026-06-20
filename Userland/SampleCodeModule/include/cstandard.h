/*
 * cstandard.h — Mini libc del userland: printf/scanf básicos, conversiones,
 * helpers de char y un PRNG corto. La libc del sistema NO está disponible.
 */
#ifndef CSTANDARD_H
#define CSTANDARD_H
#include <syscalls.h>

#ifndef NULL
#define NULL ((void*)0)
#endif

#define RANDOM_MAX UINT16_MAX

int printf(const char *format, ...);
int scanf(const char *format, ...);

int puts(char *string);
int putc(char c, int fd);
int putChar(char c);
int getc(int fd);
int getChar();

int strcmp(const char *p1, const char *p2);
int strlen(const char *str);
void numToString(long number, char *outBuffer);
int64_t stringToInt(char *str);
int32_t hexStringToInt(char *str);
int isNumericalString(char *str);

int isPrintableAscii(char c);
int isNumber(char c);
int isLower(char c);
int isUpper(char c);
int isLetter(char c);
int isAlnum(char c);
char toLower(char c);
char toUpper(char c);

int clamp(int num, int min, int max);
void setRngSeed(uint16_t seed);
uint16_t randomNext();
uint64_t randomRange(int64_t min, int64_t max);

void * memcpy(void * destination, const void * source, uint64_t length);

#endif