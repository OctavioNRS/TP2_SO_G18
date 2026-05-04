/* sampleCodeModule.c */
#include <stdint.h>
#include <syscalls.h>
#include <shell.h>

int main() {
	initializeShell();

	return 0xBEBECAFE;
}