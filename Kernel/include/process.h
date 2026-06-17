#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

#define MAX_PROCESSES      64
#define PROCESS_NAME_MAX   32
#define PROCESS_STACK_SIZE 0x8000   /* 32 KiB de stack por proceso (margen para userland) */

#define MIN_PRIORITY       0
#define MAX_PRIORITY       4
#define DEFAULT_PRIORITY   1

typedef enum {
    UNUSED = 0,   /* slot libre en la tabla            */
    READY,        /* listo para ejecutar               */
    RUNNING,      /* en ejecución                      */
    BLOCKED,      /* bloqueado                         */
    TERMINATED    /* terminó, pendiente de liberación  */
} ProcessState;

typedef int (*ProcessEntry)(int argc, char **argv);

typedef struct {
    uint32_t      pid;
    char          name[PROCESS_NAME_MAX];
    ProcessState  state;
    void         *rsp;            /* stack pointer guardado (contexto del proceso) */
    void         *stackBase;      /* base del stack alocado, para liberarlo        */
    uint8_t       priority;       
    int32_t       quantumLeft;    
    uint32_t      parentPid;      
    int32_t       waitingForPid;  
    int           retValue;       
} PCB;

typedef struct {
    uint32_t      pid;
    char          name[PROCESS_NAME_MAX];
    ProcessState  state;
    uint8_t       priority;
    void         *rsp;
    void         *stackBase;
    uint32_t      parentPid;
} ProcessInfo;

#endif
