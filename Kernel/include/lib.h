#ifndef LIB_H
#define LIB_H
#include <stdint.h>

void * memset(void * destination, int32_t character, uint64_t length);
void * memcpy(void * destination, const void * source, uint64_t length);
int strcmp(const char *p1, const char *p2);
char *cpuVendor(char *result);
void getTime(uint8_t *timeBuffer);
char *strcpy(char *dest, const char *src);
void uint64ToHex(uint64_t value, char *buffer);
int strlen(const char *str);
void concat_strings(char *dest, const char *str1, const char *str2);

// char *strcat(char *dest, const char *src);

uint8_t readPort(uint16_t port);
void writePort(uint16_t port, uint8_t value);

#endif