#include <stdint.h>
#include <videoDriver.h>
#include <keyboardDriver.h>
#include <timer.h>
#include <soundDriver.h>
#include <dateTime.h>
#include <registers.h>

#define SYSCALL_ARGS uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6

/*
enum SyscallNumber {
    SYS_READ = 0,
    SYS_WRITE,
    SYS_DRAW_RECT,
    SYS_DRAW_CIRCLE,
    SYS_DRAW_CHAR,
    SYS_SCREEN_WIDTH,
    SYS_SCREEN_HEIGHT,
    SYS_BACKGROUND,
    SYS_VIDEO_AUX,
    SYS_RENDER_AUX,
    SYS_SLEEP,
    SYS_DRAW_STRING,
    SYS_GET_KEY,
    SYS_PLAY_SOUND,
    SYS_DATE,
    SYS_TIME,
    SYS_PRINT_REGISTERS
};
*/

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
    return 0;
}
uint64_t sysTime(SYSCALL_ARGS) {
    getCurrentTime((uint8_t*)arg1, (uint8_t*)arg2, (uint8_t*)arg3); // hours, minutes, seconds
    return 0;
}
uint64_t sysKernelTime(SYSCALL_ARGS) {
    return milliseconds_elapsed();
}
uint64_t sysRegisters(SYSCALL_ARGS) {
    return printSnapshot();
}

static syscallFunction syscalls[] = {sysRead,sysWrite,sysDrawRect,sysDrawCircle,sysDrawChar,sysScrWidth,sysScrHeight,sysBg,sysVideoAux,sysRender,sysSleep,sysDrawStr,sysGetKey,sysPlaySound,sysDate,sysTime,sysKernelTime,sysRegisters};

uint64_t syscallDispatcher(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6, uint64_t syscallNumber) {
    if (syscallNumber < 18) {
        return syscalls[syscallNumber](arg1,arg2,arg3,arg4,arg5,arg6);
    }
    return 0;
}