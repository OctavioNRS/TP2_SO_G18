#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include <process.h>

void initScheduler(void);

void enableScheduler(void);

int32_t createProcess(ProcessEntry entry, int argc, char **argv, const char *name);

void exitProcess(int retValue);

uint32_t getpid(void);

void yield(void);

int32_t killProcess(uint32_t pid);

int32_t blockProcess(uint32_t pid);

int32_t unblockProcess(uint32_t pid);

int32_t toggleBlock(uint32_t pid);

void setBlocked(uint32_t pid);

int32_t setPriority(uint32_t pid, uint8_t priority);

int32_t waitpid(uint32_t pid);

int listProcesses(ProcessInfo *buffer, int max);

uint64_t schedule(uint64_t rsp);

#endif
