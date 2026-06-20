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