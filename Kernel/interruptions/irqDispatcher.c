/*
 * irqDispatcher.c — Despacha las IRQs del PIC al handler correspondiente.
 *
 * El stub en ensamblador (interrupts.asm) pushea el estado y llama acá con el
 * número de IRQ. Mapeo actual: IRQ0 → timer (scheduler tick), IRQ1 → driver
 * del teclado. El resto está sin asignar y se ignora.
 */
#include <timer.h>
#include <stdint.h>
#include <keyboardDriver.h>

#define IRQ_COUNT 16

typedef void (*irq_handler_t)(void);
static irq_handler_t irqHandlers[IRQ_COUNT] = {timer_handler, keyboardInterrupt};

void irqDispatcher(uint64_t irq) {
	irqHandlers[irq]();
	return;
}