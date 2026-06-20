/*
 * shell.h — Shell interactiva en userland.
 *
 * Lee del STDIN del proceso (cableado por el init al pipe del teclado) y produce salida
 * al STDOUT (cableado por el propio shell a un pipe que va al proceso Terminal). Los
 * comandos se ejecutan como procesos hijos que heredan los 3 fds.
 */
#ifndef SHELL_H
#define SHELL_H

#define MAX_COMMAND_LENGTH 128

/* Entry point del shell process. */
int shellProcess(int argc, char **argv);

/* Para uso de los comandos: limpia el terminal mandando el FF. */
void terminalClear(void);

#endif
