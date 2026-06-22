/*
 * syscallDispatcher.c — Tabla de system calls + wrappers individuales.
 *
 * El stub `_syscall` (interrupts.asm) entra desde `int 0x80`, pone el número
 * de syscall en rax y los argumentos en rdi/rsi/rdx/r10/r8/r9, y llama a
 * `syscallDispatcher`. Acá se indexa la tabla `syscalls[]` y se invoca el
 * wrapper concreto, que reenvía al subsistema correspondiente (video, fs,
 * scheduler, ipc, etc.).
 */
#include <stdint.h>
#include <videoDriver.h>
#include <keyboardDriver.h>
#include <timer.h>
#include <soundDriver.h>
#include <dateTime.h>
#include <registers.h>
#include <memoryManager.h>
#include <scheduler.h>
#include <process.h>
#include <sharedSemaphores.h>
#include <pipe.h>
#include <fileAccess.h>

#define SYSCALL_ARGS uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6

typedef uint64_t (*syscallFunction)(uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t);

uint64_t sysRead(SYSCALL_ARGS) {
    return (uint64_t) readFdOnPID(getCurrentPID(), (int)arg1, (char*)arg2, arg3);
}
uint64_t sysWrite(SYSCALL_ARGS) {
    return (uint64_t) writeFdOnPID(getCurrentPID(), (int)arg1, (const char*)arg2, arg3);
}
uint64_t sysDrawRect(SYSCALL_ARGS) {
    drawRectangle(arg1, arg2, arg3, arg4, arg5);
    return 0;
}
uint64_t sysDrawCircle(SYSCALL_ARGS) {
    if (arg5) drawCircle(arg1, arg2, arg3, arg4);
    else      drawCircumference(arg1, arg2, arg3, arg4);
    return 0;
}
uint64_t sysDrawChar(SYSCALL_ARGS) {
    drawSymbol((unsigned char)arg1, arg2, arg3, arg4, arg5, arg6);
    return 0;
}
uint64_t sysScrWidth(SYSCALL_ARGS) { return getScreenWidth(); }
uint64_t sysScrHeight(SYSCALL_ARGS) { return getScreenHeight(); }
uint64_t sysBg(SYSCALL_ARGS) { setBackground((uint32_t)arg1); return 0; }
uint64_t sysVideoAux(SYSCALL_ARGS) {
    setAuxFramebuffer((uint8_t*)arg1, arg2, arg3);
    return 0;
}
uint64_t sysRender(SYSCALL_ARGS) { renderAux(); return 0; }
uint64_t sysSleep(SYSCALL_ARGS) { return sleep(arg1); }
uint64_t sysDrawStr(SYSCALL_ARGS) {
    drawString((unsigned char*)arg1, arg2, arg3, arg4, arg5, arg6);
    return 0;
}
uint64_t sysGetKey(SYSCALL_ARGS) { return isPressed((uint8_t)arg1); }
uint64_t sysPlaySound(SYSCALL_ARGS) {
    playSound(arg1, arg2);
    return 0;
}
uint64_t sysDate(SYSCALL_ARGS) {
    getCurrentDate((uint8_t*)arg1, (uint8_t*)arg2, (uint8_t*)arg3, (uint8_t*)arg4);
    return 0;
}
uint64_t sysTime(SYSCALL_ARGS) {
    getCurrentTime((uint8_t*)arg1, (uint8_t*)arg2, (uint8_t*)arg3);
    return 0;
}
uint64_t sysKernelTime(SYSCALL_ARGS) { return milliseconds_elapsed(); }
uint64_t sysRegisters(SYSCALL_ARGS) { return printSnapshot(); }

uint64_t sysMalloc(SYSCALL_ARGS) { return (uint64_t) mem_alloc(arg1); }
uint64_t sysFree(SYSCALL_ARGS) { mem_free((void *)arg1); return 0; }
uint64_t sysMemStatus(SYSCALL_ARGS) {
    mem_status((MemoryInfo *)arg1);
    return 0;
}

uint64_t sysGetpid(SYSCALL_ARGS) { return getCurrentPID(); }
uint64_t sysCreateProcess(SYSCALL_ARGS) {
    return startNewProcess((ProcessEntryPoint)arg1, (char *)arg4, 1,
                            (int)arg2, (char **)arg3);
}
uint64_t sysKill(SYSCALL_ARGS) { return terminateProcess((pid_t)arg1); }
uint64_t sysNice(SYSCALL_ARGS) {
    return setPriorityOnPID((pid_t)arg1, (uint8_t)arg2);
}
uint64_t sysBlock(SYSCALL_ARGS) {
    return (uint64_t)(int64_t) setBlocked((pid_t)arg1);
}
uint64_t sysUnblock(SYSCALL_ARGS) {
    pid_t pid = (pid_t)arg1;
    if (pid == IDLE_PID || pid == INIT_PID) return (uint64_t)(int64_t)-1;
    return (uint64_t)(int64_t) setReady(pid);
}
uint64_t sysYield(SYSCALL_ARGS) { forceSchedulerCall(); return 0; }
uint64_t sysWaitpid(SYSCALL_ARGS) { wait_pid((pid_t)arg1); return 0; }
uint64_t sysPs(SYSCALL_ARGS) {
    return (uint64_t) getProcessInfo((ProcessInfo *)arg1, (int)arg2);
}
uint64_t sysExit(SYSCALL_ARGS) {
    terminateProcess(getCurrentPID());
    while (1) ;
    return 0;
}

/* --- Semáforos compartidos por nombre --- */
uint64_t sysSemOpen(SYSCALL_ARGS) {
    return (uint64_t)(int64_t) sharedSemOpen((const char *)arg1, (int)arg2);
}
uint64_t sysSemClose(SYSCALL_ARGS) {
    return (uint64_t)(int64_t) sharedSemClose((const char *)arg1);
}
uint64_t sysSemWait(SYSCALL_ARGS) {
    return (uint64_t)(int64_t) sharedSemWait((const char *)arg1);
}
uint64_t sysSemPost(SYSCALL_ARGS) {
    return (uint64_t)(int64_t) sharedSemPost((const char *)arg1);
}

uint64_t sysPipe(SYSCALL_ARGS) {
    int *userFds = (int *) arg1;
    if (!userFds) return (uint64_t)(int64_t) -1;

    Pipe pipe = newPipe();
    if (!pipe) return (uint64_t)(int64_t) -1;

    FileAccess readEnd  = newFileAccess(pipe, FILE_READ);
    FileAccess writeEnd = newFileAccess(pipe, FILE_WRITE);
    if (!readEnd || !writeEnd) {
        if (readEnd)  freeFileAccess(readEnd);
        if (writeEnd) freeFileAccess(writeEnd);
        /* Si ninguna FA se creó, el pipe quedó huérfano (refCount = 0). */
        if (!readEnd && !writeEnd) freePipe(pipe);
        return (uint64_t)(int64_t) -1;
    }

    int readFD = addCurrentFd(readEnd);
    if (readFD < 0) {
        freeFileAccess(readEnd);
        freeFileAccess(writeEnd);
        return (uint64_t)(int64_t) -1;
    }
    int writeFD = addCurrentFd(writeEnd);
    if (writeFD < 0) {
        closeCurrentFd(readFD);
        freeFileAccess(writeEnd);
        return (uint64_t)(int64_t) -1;
    }

    userFds[0] = readFD;
    userFds[1] = writeFD;
    return 0;
}

uint64_t sysClose(SYSCALL_ARGS) {
    return (uint64_t)(int64_t) closeCurrentFd((int) arg1);
}

uint64_t sysDup(SYSCALL_ARGS) {
    return (uint64_t)(int64_t) dupCurrentFd((int) arg1, (int) arg2);
}

uint64_t sysReopen(SYSCALL_ARGS) {
    return (uint64_t)(int64_t) reopenCurrentFd((int) arg1);
}

/* sys_create_process_with_fds(entry, argc, argv, name, overrides, overrideCount) */
uint64_t sysCreateProcessFds(SYSCALL_ARGS) {
    return (uint64_t)(int64_t) startNewProcessWithFds(
        (ProcessEntryPoint)arg1, (char *)arg4, 1, (int)arg2, (char **)arg3,
        (fd_override_t *)arg5, (int)arg6);
}

uint64_t sysForeground(SYSCALL_ARGS) {
    return (uint64_t)(int64_t) addToFG((pid_t)arg1);
}

uint64_t sysScrollUp(SYSCALL_ARGS) {
    scrollUp(arg1, (uint32_t)arg2);
    return 0;
}

uint64_t sysReapZombies(SYSCALL_ARGS) {
    (void)arg1; (void)arg2; (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    reap_zombies();
    return 0;
}

uint64_t sysGetMaxProcesses(SYSCALL_ARGS) {
    (void)arg1; (void)arg2; (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return (uint64_t)getMaxProcesses();
}

uint64_t sysGetProcessesCount(SYSCALL_ARGS) {
    (void)arg1; (void)arg2; (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return (uint64_t)getProcessesCount();
}

static syscallFunction syscalls[] = {
    sysRead, sysWrite, sysDrawRect, sysDrawCircle, sysDrawChar, sysScrWidth, sysScrHeight,
    sysBg, sysVideoAux, sysRender, sysSleep, sysDrawStr, sysGetKey, sysPlaySound, sysDate,
    sysTime, sysKernelTime, sysRegisters,
    sysMalloc, sysFree, sysMemStatus,
    sysGetpid, sysCreateProcess, sysKill, sysNice, sysBlock, sysYield, sysWaitpid, sysPs,
    sysExit, sysUnblock,
    sysSemOpen, sysSemClose, sysSemWait, sysSemPost,
    sysPipe, sysClose, sysDup, sysCreateProcessFds, sysForeground, sysReopen, sysScrollUp,
    sysReapZombies, sysGetMaxProcesses, sysGetProcessesCount
};

#define SYSCALL_COUNT (sizeof(syscalls) / sizeof(syscalls[0]))

uint64_t syscallDispatcher(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6, uint64_t syscallNumber) {
    if (syscallNumber < SYSCALL_COUNT) {
        return syscalls[syscallNumber](arg1,arg2,arg3,arg4,arg5,arg6);
    }
    return 0;
}