/*
 * sampleCodeModule.c — Entry point del módulo de userland.
 *
 * Este módulo ES el shell. El kernel (init) lo lanza directamente vía startNewProcess
 * apuntando a esta dirección (0x400000). La main() acá ejecuta el loop del shell de
 * principio a fin; sys_exit cierra el proceso. No hay proceso intermedio.
 */
#include <stdint.h>
#include <syscalls.h>
#include <shell.h>

int main(int argc, char **argv) {
    return shellProcess(argc, argv);
}
