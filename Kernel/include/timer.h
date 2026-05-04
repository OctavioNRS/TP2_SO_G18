#ifndef _TIMER_H_
#define _TIMER_H_
#include <stdint.h>

void timer_handler();
int ticks_elapsed();
int seconds_elapsed();
int milliseconds_elapsed();
uint64_t sleep(uint64_t millis);

#endif
