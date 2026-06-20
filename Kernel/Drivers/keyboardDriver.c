/*
 * keyboardDriver.c — Driver del teclado PS/2 vía IRQ1.
 *
 * Traduce scancodes a ASCII (mapeo US QWERTY con/sin shift), maneja CapsLock y
 * teclas modificadoras (Shift/Ctrl), y deposita cada byte en el pipe del teclado
 * (FileAccess registrado por `initializeKeyboardDriver`). Captura Ctrl+C para
 * matar el foreground set, Ctrl+D para marcar EOF en los reads bloqueados, y F6
 * (Print Screen) para tomar un snapshot de los registros.
 */
#include <stdint.h>
#include <lib.h>
#include <keyboardDriver.h>
#include <registers.h>
#include <fileAccess.h>
#include <pipe.h>
#include <scheduler.h>

#define KEYBOARD_DATA_PORT   0x60
#define KEYBOARD_STATUS_PORT 0x64

#define MULTI_BYTE_SCANCODE 0xE0
#define KEYCODE_SHIFT       0x7F

#define LSHIFT   0x2A
#define RSHIFT   0x36
#define CAPSLOCK 0x3A
#define LCONTROL 0x1D

#define PRESS_C 0x2E
#define PRESS_D 0x20

#define END_OF_TEXT         0x03
#define END_OF_TRANSMISSION 0x04

static const char scancodeToAscii[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' ', 0
};

static const char shiftedScancodeToAscii[] = {
    0, 0, '!', '\"', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '@', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' ', 0
};

static char       keyboardStatus[256] = {0};
static int        multiByteFlag = 0;
static int        capsLockActive = 0;
static FileAccess keyboardBuffer = 0;

static int keyboardHasData(void) { return (readPort(KEYBOARD_STATUS_PORT) & 1); }

void initializeKeyboardDriver(FileAccess kbdWriteEnd) {
    if (!isWritable(kbdWriteEnd)) return;
    keyboardBuffer = kbdWriteEnd;
    addFileRef(keyboardBuffer);
}

Pipe getKeyboardPipe(void) { return getFilePipe(keyboardBuffer); }

int isPressed(uint8_t keycode) { return keyboardStatus[keycode]; }

static int isAlpha(uint8_t scancode) {
    if (scancode >= sizeof(scancodeToAscii)) return 0;
    uint8_t ascii = scancodeToAscii[scancode];
    return 'a' <= ascii && ascii <= 'z';
}

static int shouldShift(uint8_t scancode) {
    int pressingShift = (keyboardStatus[LSHIFT] || keyboardStatus[RSHIFT]);
    return pressingShift != (capsLockActive && isAlpha(scancode));
}

char getKeyPress(uint8_t scancode) {
    uint8_t releaseFlag    = (scancode & 0x80) >> 7;
    uint8_t normalizedCode =  scancode & 0x7F;

    if (multiByteFlag) {
        normalizedCode += KEYCODE_SHIFT;
        multiByteFlag = 0;
    }

    keyboardStatus[normalizedCode] = !releaseFlag;
    if (releaseFlag) return 0;

    if (normalizedCode == CAPSLOCK) {
        capsLockActive = !capsLockActive;
        return 0;
    }

    if (normalizedCode < sizeof(scancodeToAscii)) {
        const char *map = shouldShift(normalizedCode) ? shiftedScancodeToAscii
                                                       : scancodeToAscii;
        return map[normalizedCode];
    }
    if (normalizedCode >= KEYCODE_SHIFT) return normalizedCode;
    return 0;
}

static char processControl(uint8_t scancode) {
    if (!isPressed(LCONTROL)) return 0;
    switch (scancode) {
        case PRESS_C: return END_OF_TEXT;
        case PRESS_D: return END_OF_TRANSMISSION;
        default:      return 0;
    }
}

/* Sólo manda EOF a los procesos fg cuyo STDIN sea el pipe del teclado */
static void requestEofOnKbdFg(pid_t pid) {
    FileAccess stdinFa = getFdOnPID(pid, STDIN);
    if (!stdinFa || !isReadable(stdinFa)) return;
    if (getFilePipe(stdinFa) != getKeyboardPipe()) return;
    requestEofOnPID(pid, STDIN);
}

void keyboardInterrupt() {
    if (!keyboardHasData()) return;
    uint8_t scancode = readPort(KEYBOARD_DATA_PORT);

    if (scancode == MULTI_BYTE_SCANCODE) { multiByteFlag = 1; return; }

    if (scancode == SNAPSHOT_SCANCODE) saveRegisterSnapshot();

    char ctrl = processControl(scancode);
    char key  = ctrl ? ctrl : getKeyPress(scancode);

    switch (key) {
        case 0:
            return;
        case END_OF_TEXT:
            forEachInFG(killFromIsr);
            return;
        case END_OF_TRANSMISSION:
            forEachInFG(requestEofOnKbdFg);
            {
                char c = '\0';
                fileWrite(keyboardBuffer, &c, 1);   /* destraba reads bloqueados */
            }
            return;
        default:
            fileWrite(keyboardBuffer, &key, 1);
            return;
    }
}
