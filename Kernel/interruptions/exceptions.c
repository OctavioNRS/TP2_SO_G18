/*
 * exceptions.c — Handlers de excepciones de CPU (división por cero y opcode inválido).
 */
#include <videoDriver.h>
#include <timer.h>
#include <registers.h>
#include <lib.h>

enum ExceptionId {
	ZERO_EXCEPTION_ID = 0,
	INVALID_OPCODE_EXCEPTION_ID = 6
};

static const char messages[7][100] = {
	"Error: Zero division.",
	"",
	"",
	"",
	"",
	"",
	"Error: Invalid opcode."
};

static void handle_exception(int exception, StackFrame *frame) {
    setAuxFramebuffer(0,0,0);
	// Imprimo todos los registros a STDOUT
	print_registers(frame);
	
	uint32_t bgColor = 0x000082;
	setBackground(bgColor);
	
	char message_buffer[150] = {0};
	
	// Construyo el mensaje
	concat_strings(message_buffer, messages[exception], " Restarting in 3...");
	
	// Encuentro la posición donde está el número
	int number_index = strlen(messages[exception]) + 15; // 15 es la longitud de " Restarting in "
	
	// Countdown de 3 a 1
	for (int counter = 3; counter > 0; counter--) {
		// Cambio el dígito del contador
		message_buffer[number_index] = counter + '0';
		
		drawString((unsigned char *)message_buffer, 2*8, getScreenHeight()/2, 0xFFFFFF, bgColor, 2);
		sleep(1000);
	}
}

void exceptionDispatcher(int exception, StackFrame *frame) {
	handle_exception(exception, frame);
}