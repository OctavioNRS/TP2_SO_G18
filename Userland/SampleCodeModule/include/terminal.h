/*
 * terminal.h — Proceso de userland que renderiza al framebuffer lo que llega por su
 * STDIN (un pipe que viene del shell). Mantiene un cursor (col,row), hace scroll por
 * línea cuando se llega al borde inferior y maneja los caracteres de control básicos
 * (\n, \r, \b, \t).
 */
#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdint.h>

int terminalProcess(int argc, char **argv);

#endif
