#ifndef REGISTERS_H
#define REGISTERS_H
#include <stdint.h>

#define SNAPSHOT_SCANCODE 0x40

// Estructura de la pila de excepciones
typedef struct {
    // Registros pushState (en orden inverso)
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rsi, rdi, rbp, rdx, rcx, rbx, rax;
    // Valores del resto del stack
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} StackFrame;

void print_registers(StackFrame *frame);
void createRegisterSnapshot(StackFrame *frame);
void saveRegisterSnapshot();
int printSnapshot();

#endif