/*
 * timer.c — IRQ0 (Programmable Interval Timer).
 *
 * Cada tick incrementa el contador de milisegundos del kernel y delega al
 * scheduler para hacer el context switch si corresponde. Expone `sleep` (espera
 * activa basada en el contador) y `milliseconds_elapsed` para userland.
 */
#include <timer.h>
#include <videoDriver.h>
#include <interrupts.h>
#include <soundDriver.h>

#define PERIOD_MILLIS 55
#define FREQUENCY (1000/PERIOD_MILLIS)

static unsigned long ticks = 0;

void timer_handler() {
	ticks++;
	isSoundPlaying();
}

int ticks_elapsed() {
	return ticks;
}

int seconds_elapsed() {
	return ticks / FREQUENCY;
}

int milliseconds_elapsed() {
	return ticks * PERIOD_MILLIS;
}

uint64_t sleep(uint64_t millis) {
	uint64_t start = milliseconds_elapsed();
	uint64_t elapsed = 0;
	_sti();
	while (elapsed < millis)
	{
		elapsed = milliseconds_elapsed() - start;
	}
	_cli();
	return elapsed;
}
