/*
 * commands.h — Tabla de comandos disponibles en el shell.
 *
 * Cada comando es un proceso: su entry point recibe (argc, argv) como cualquier
 * ProcessEntry. La shell busca el comando por nombre y crea el proceso vía
 * sys_create_process_with_fds. El comando debe llamar sys_exit() antes de retornar.
 */
#ifndef COMMANDS_H
#define COMMANDS_H

#include <syscalls.h>

/* Devuelve el índice del comando en la tabla interna, o -1 si no existe. */
int getCommandIndex(const char *name);

/* Devuelve el entry point asociado al índice. */
ProcessEntry getCommandEntry(int idx);

/* Devuelve el nombre del comando (para uso de `help`). */
const char *getCommandName(int idx);

/* Cantidad total de comandos. */
int getCommandCount(void);

#endif
