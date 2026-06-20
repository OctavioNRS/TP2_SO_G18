/*
 * _loader.c — Entry point del módulo de userland (símbolo `_start`).
 *
 * Pure64 carga el binario en 0x400000 y la primera dirección ejecutable es
 * `_start`. Limpia el BSS del módulo a cero y reenvía a `main(argc, argv)`.
 * Los argumentos los pasa el kernel en rdi/rsi vía la convención System V AMD64
 * cuando el scheduler arma la pila inicial del proceso.
 */
#include <stdint.h>

extern char bss;
extern char endOfBinary;

int main(int argc, char **argv);

void * memset(void * destiny, int32_t c, uint64_t length);

int _start(int argc, char **argv) {
	memset(&bss, 0, &endOfBinary - &bss);
	return main(argc, argv);
}


void * memset(void * destiation, int32_t c, uint64_t length) {
	uint8_t chr = (uint8_t)c;
	char * dst = (char*)destiation;

	while(length--)
		dst[length] = chr;

	return destiation;
}
