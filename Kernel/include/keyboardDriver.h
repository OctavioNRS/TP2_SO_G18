/*
 * keyboardDriver.h — API del driver de teclado y constantes de fds estándar.
 */
#ifndef _KEYBOARD_DRIVER_H
#define _KEYBOARD_DRIVER_H
#include <stdint.h>
#include <fileAccess.h>

#define STDIN  0
#define STDOUT 1
#define STDERR 2

/* Devuelve el caracter ASCII para `scancode`. Cero si no aplica (release, tecla muerta). */
char getKeyPress(uint8_t scancode);

/* Estado on/off de una tecla por keycode. */
int  isPressed(uint8_t keycode);

/* Handler invocado desde el IRQ1: lee scancode, traduce, despacha. */
void keyboardInterrupt();

/* Conecta al teclado el lado de escritura del pipe stdin del sistema. Cada tecla
 * viaja por ese FileAccess; cualquier proceso que tenga su STDIN apuntando al
 * mismo pipe puede leerla con `read(STDIN, ...)`. Llamada una sola vez desde el
 * bootstrap. El driver mantiene su propia referencia al FileAccess (addFileRef). */
void initializeKeyboardDriver(FileAccess kbdWriteEnd);

/* Devuelve el Pipe subyacente al FileAccess de escritura del teclado, para
 * que otros módulos puedan chequear si un FD apunta al pipe del teclado. */
Pipe getKeyboardPipe(void);

#endif
