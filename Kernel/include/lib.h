/*
 * lib.h — Utilidades estándar mínimas para el kernel (no hay libc).
 * Incluye mem*, str*, I/O de puertos x86 y wrappers de CPUID/RTC.
 */
#ifndef LIB_H
#define LIB_H
#include <stdint.h>

void * memset(void * destination, int32_t character, uint64_t length);
void * memcpy(void * destination, const void * source, uint64_t length);
int strcmp(const char *p1, const char *p2);
char *cpuVendor(char *result);
void getTime(uint8_t *hours, uint8_t *minutes, uint8_t *seconds);
void getDate(uint8_t *weekday, uint8_t *day, uint8_t *month, uint8_t *year);
char *strcpy(char *dest, const char *src);
void uint64ToHex(uint64_t value, char *buffer);
uint64_t strlen(const char *str);
void concat_strings(char *dest, const char *str1, const char *str2);

// char *strcat(char *dest, const char *src);

uint8_t readPort(uint16_t port);
void writePort(uint16_t port, uint8_t value);

#endif