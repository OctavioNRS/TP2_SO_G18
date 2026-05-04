#include <registers.h>
#include <keyboardDriver.h>
#include <lib.h>

static StackFrame snapshot;
static StackFrame savedSnapshot;
static int snapshotTaken = 0;

// Función helper para agregar un registro al output buffer
static void add_register_to_buffer(char *output_buffer, int *pos, const char *reg_name, uint64_t value) {
    char hex_buffer[19];
    
    // Agrego el nombre
    for (int i = 0; reg_name[i]; i++) {
        output_buffer[(*pos)++] = reg_name[i];
    }
    
    // Agrego el hex
    uint64ToHex(value, hex_buffer);
    for (int i = 0; hex_buffer[i]; i++) {
        output_buffer[(*pos)++] = hex_buffer[i];
    }
    
    output_buffer[(*pos)++] = '\n';
}

// Función para imprimir todos los registros a STDOUT
void print_registers(StackFrame *frame) {
    char output_buffer[600];
    int pos = 0;
    
    // Header
    const char *header = "\n=== REGISTER DUMP ===\n";
    for (int i = 0; header[i]; i++) {
        output_buffer[pos++] = header[i];
    }
    
    // Agrego todos los registros
    add_register_to_buffer(output_buffer, &pos, "RAX: ", frame->rax);
    add_register_to_buffer(output_buffer, &pos, "RBX: ", frame->rbx);
    add_register_to_buffer(output_buffer, &pos, "RCX: ", frame->rcx);
    add_register_to_buffer(output_buffer, &pos, "RDX: ", frame->rdx);
    add_register_to_buffer(output_buffer, &pos, "RSI: ", frame->rsi);
    add_register_to_buffer(output_buffer, &pos, "RDI: ", frame->rdi);
    add_register_to_buffer(output_buffer, &pos, "RBP: ", frame->rbp);
    add_register_to_buffer(output_buffer, &pos, "RSP: ", frame->rsp);
    add_register_to_buffer(output_buffer, &pos, "R8:  ", frame->r8);
    add_register_to_buffer(output_buffer, &pos, "R9:  ", frame->r9);
    add_register_to_buffer(output_buffer, &pos, "R10: ", frame->r10);
    add_register_to_buffer(output_buffer, &pos, "R11: ", frame->r11);
    add_register_to_buffer(output_buffer, &pos, "R12: ", frame->r12);
    add_register_to_buffer(output_buffer, &pos, "R13: ", frame->r13);
    add_register_to_buffer(output_buffer, &pos, "R14: ", frame->r14);
    add_register_to_buffer(output_buffer, &pos, "R15: ", frame->r15);
    add_register_to_buffer(output_buffer, &pos, "RIP: ", frame->rip);
    add_register_to_buffer(output_buffer, &pos, "RFLAGS: ", frame->rflags);
    add_register_to_buffer(output_buffer, &pos, "CS:  ", frame->cs);
    add_register_to_buffer(output_buffer, &pos, "SS:  ", frame->ss);
    
    // Footer
    const char *footer = "==================\n\n";
    for (int i = 0; footer[i]; i++) {
        output_buffer[pos++] = footer[i];
    }
    
    output_buffer[pos] = '\0';
    
    write(STDOUT, output_buffer, pos);
}

void createRegisterSnapshot(StackFrame* frame){
    snapshot = *frame;
}

void saveRegisterSnapshot() {
    snapshotTaken = 1;
    savedSnapshot = snapshot;
}

int printSnapshot() {
    if (!snapshotTaken) return 0;
    print_registers(&savedSnapshot);
    return 1;
}