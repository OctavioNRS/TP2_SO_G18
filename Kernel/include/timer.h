/*
 * timer.h — Contador de ticks/ms del PIT (IRQ0) y `sleep` por espera activa.
 */
#ifndef _TIMER_H_
#define _TIMER_H_
#include <stdint.h>

void timer_handler();
int ticks_elapsed();
int seconds_elapsed();
int milliseconds_elapsed();
uint64_t sleep(uint64_t millis);

#endif
