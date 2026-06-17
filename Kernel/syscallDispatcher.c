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

#define SYSCALL_ARGS uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6

typedef uint64_t (*syscallFunction)(uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t);

uint64_t sysRead(SYSCALL_ARGS) {
    return read(arg1, (char*)arg2, arg3); //fd, buffer, count
}
uint64_t sysWrite(SYSCALL_ARGS) {
    return write(arg1, (char*)arg2, arg3); //fd, buffer, count
}
uint64_t sysDrawRect(SYSCALL_ARGS) {
    drawRectangle(arg1, arg2, arg3, arg4, arg5); //color, x, y, width, height
    return 0;
}
uint64_t sysDrawCircle(SYSCALL_ARGS) {
    if (arg5) { // arg5 = fill flag
        drawCircle(arg1, arg2, arg3, arg4); //color, centerX, centerY, radius
    } else {
        drawCircumference(arg1, arg2, arg3, arg4); //color, centerX, centerY, radius
    }
    return 0;
}
uint64_t sysDrawChar(SYSCALL_ARGS) {
    //bgcolor = -1 -> Transparent
    drawSymbol((unsigned char)arg1, arg2, arg3, arg4, arg5, arg6); //char, x, y, fgcolor, bgcolor, scale
    return 0;
}
uint64_t sysScrWidth(SYSCALL_ARGS) {
    return getScreenWidth();
}
uint64_t sysScrHeight(SYSCALL_ARGS) {
    return getScreenHeight();
}
uint64_t sysBg(SYSCALL_ARGS) {
    setBackground((uint32_t)arg1); //color
    return 0;
}
uint64_t sysVideoAux(SYSCALL_ARGS) {
    // Setear en 0 para dibujar al framebuffer directamente
    setAuxFramebuffer((uint8_t*)arg1, arg2, arg3); //buffer, width, height
    return 0;
}
uint64_t sysRender(SYSCALL_ARGS) {
    renderAux();
    return 0;
}
uint64_t sysSleep(SYSCALL_ARGS) {
    return sleep(arg1); //ms
}
uint64_t sysDrawStr(SYSCALL_ARGS) {
    //bgcolor = -1 -> Transparent
    drawString((unsigned char*)arg1, arg2, arg3, arg4, arg5, arg6); //string, x, y, fgcolor, bgcolor, scale
    return 0;
}
uint64_t sysGetKey(SYSCALL_ARGS) {
    return isPressed((uint8_t)arg1);
}
uint64_t sysPlaySound(SYSCALL_ARGS) {
    playSound(arg1, arg2); //frequency, duration_ms
    return 0;
}
uint64_t sysDate(SYSCALL_ARGS) {
    getCurrentDate((uint8_t*)arg1, (uint8_t*)arg2, (uint8_t*)arg3, (uint8_t*)arg4); // weekday, day, month, year
    getCurrentDate((uint8_t*)arg1, (uint8_t*)arg2, (uint8_t*)arg3, (uint8_t*)arg4);
    return 0;
}
uint64_t sysTime(SYSCALL_ARGS) {
    getCurrentTime((uint8_t*)arg1, (uint8_t*)arg2, (uint8_t*)arg3);
    return 0;
}
uint64_t sysKernelTime(SYSCALL_ARGS) {
    return milliseconds_elapsed();
}
uint64_t sysRegisters(SYSCALL_ARGS) {
    return printSnapshot();
}

uint64_t sysMalloc(SYSCALL_ARGS) {
    return (uint64_t) mem_alloc(arg1);
}
uint64_t sysFree(SYSCALL_ARGS) {
    mem_free((void *)arg1);
    return 0;
}
uint64_t sysMemStatus(SYSCALL_ARGS) {
    mem_status((MemoryInfo *)arg1);
    return 0;
}

uint64_t sysGetpid(SYSCALL_ARGS) {
    return getpid();
}
uint64_t sysCreateProcess(SYSCALL_ARGS) {
    return (uint64_t)(int64_t) createProcess((ProcessEntry)arg1, (int)arg2, (char **)arg3, (const char *)arg4);
}
uint64_t sysKill(SYSCALL_ARGS) {
    return (uint64_t)(int64_t) killProcess((uint32_t)arg1);
}
uint64_t sysNice(SYSCALL_ARGS) {
    return (uint64_t)(int64_t) setPriority((uint32_t)arg1, (uint8_t)arg2);
}
uint64_t sysBlock(SYSCALL_ARGS) {
    return (uint64_t)(int64_t) toggleBlock((uint32_t)arg1);
}
uint64_t sysYield(SYSCALL_ARGS) {
    yield();
    return 0;
}
uint64_t sysWaitpid(SYSCALL_ARGS) {
    return (uint64_t)(int64_t) waitpid((uint32_t)arg1);
}
uint64_t sysPs(SYSCALL_ARGS) {
    return (uint64_t) listProcesses((ProcessInfo *)arg1, (int)arg2);
}

static syscallFunction syscalls[] = {
    sysRead, sysWrite, sysDrawRect, sysDrawCircle, sysDrawChar, sysScrWidth, sysScrHeight,
    sysBg, sysVideoAux, sysRender, sysSleep, sysDrawStr, sysGetKey, sysPlaySound, sysDate,
    sysTime, sysKernelTime, sysRegisters,
    sysMalloc, sysFree, sysMemStatus,
    sysGetpid, sysCreateProcess, sysKill, sysNice, sysBlock, sysYield, sysWaitpid, sysPs
};

#define SYSCALL_COUNT (sizeof(syscalls) / sizeof(syscalls[0]))

uint64_t syscallDispatcher(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6, uint64_t syscallNumber) {
    if (syscallNumber < SYSCALL_COUNT) {
        return syscalls[syscallNumber](arg1,arg2,arg3,arg4,arg5,arg6);
    }
    return 0;
}
