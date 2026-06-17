#include <stdint.h>
#include <lib.h>
#include <keyboardDriver.h>
#include <registers.h>

#define BUFFER_SIZE 4096

// Puertos del teclado
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64

#define MULTI_BYTE_SCANCODE 0xE0
#define KEYCODE_SHIFT 0x7F

// Teclas especiales
#define LSHIFT 0x2A
#define RSHIFT 0x36
#define CAPSLOCK 0x3A

typedef struct {
    uint32_t read;
    uint32_t write;
    uint8_t data[BUFFER_SIZE];
} Buffer;

// Mapeo de scancode a ASCII para el layout de teclado US
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

// [error, escape, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 'backspace',
//  'tab', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 'enter',
//  'L ctrl', 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', ''', '`',
//  'L shift', '\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 'R shift',
//  '*', 'L alt', 'space', 'caps']

static char keyboardStatus[256] = {0};  // Array para rastrear estados de teclas (0 = liberada, 1 = presionada)

static int keyboardHasData() {
    return (readPort(KEYBOARD_STATUS_PORT) & 1);
}

// static Buffer keyboardBuffer = {0, 0, {0}};    // Acá llega todo lo que tocas
static Buffer stdin = {0, 0, {0}};          // Entrada de teclado, incluyendo teclas no imprimibles
static Buffer stdout = {0, 0, {0}};         // Salida imprimible
static Buffer stderr = {0, 0, {0}};         // Salida de error, imprimible

static Buffer *buffers[] = {&stdin, &stdout, &stderr};

static void bufferPush(Buffer *buff, char c) {
    if (buff->write >= BUFFER_SIZE) {
        // Buffer overflow, ignore input
        return;
    }
    buff->data[buff->write++] = c;
}

static void resetBufferIndex(Buffer *buff) {
    buff->read = 0;
    buff->write = 0;
}

static uint8_t readBuffer(Buffer *buff) {
    if (buff->read >= buff->write) {
        return 0;
    }
    
    uint8_t c = buff->data[buff->read++];
    if (buff->read >= buff->write) {
        resetBufferIndex(buff);
    }
    return c;
}

static int multiByteFlag = 0;

void keyboardInterrupt() {
    if (!keyboardHasData()) return;
    uint8_t scancode = readPort(KEYBOARD_DATA_PORT);

    if (scancode == MULTI_BYTE_SCANCODE) {
        multiByteFlag = 1;
        return;
    }

    if (scancode == SNAPSHOT_SCANCODE) {
        saveRegisterSnapshot();
    }

    char key = getKeyPress(scancode);
    if (key != 0) {
        bufferPush(&stdin, key);
    }
}

// Scancodes normales: keycode = scancode.
// Scancodes de dos bytes: keycode = scancodeSecondByte + 7F
int isPressed(uint8_t keycode) {
    return keyboardStatus[keycode];
}

static int capsLockActive = 0;

int isAlpha(uint8_t scancode) {
    if (scancode >= sizeof(scancodeToAscii)) return 0;
    uint8_t ascii = scancodeToAscii[scancode];
    return 'a' <= ascii && ascii <= 'z';
}

int shift(uint8_t scancode) {
    int pressingShiftKey = (keyboardStatus[LSHIFT] || keyboardStatus[RSHIFT]);
    return pressingShiftKey != (capsLockActive && isAlpha(scancode));
}

char getKeyPress(uint8_t scancode) {

    uint8_t releaseFlag = (scancode & 0x80) >> 7;
    uint8_t normalizedCode = scancode & 0x7F;

    if (multiByteFlag) {
        // Con esto me aseguro que los scancodes de multiples bytes mappean fuera de la tabla ascii
        normalizedCode += KEYCODE_SHIFT;
        multiByteFlag = 0;
    }

    keyboardStatus[normalizedCode] = !releaseFlag;
    
    // Si la tecla está siendo soltada, ignorar
    if (releaseFlag) {
        return 0;
    }

    if (normalizedCode == CAPSLOCK) {
        capsLockActive = !capsLockActive;
    }
    
    // Convierto el scancode a ASCII
    if (normalizedCode < sizeof(scancodeToAscii)) {
        const char *asciiMap = shift(normalizedCode) ? shiftedScancodeToAscii : scancodeToAscii;
        return asciiMap[normalizedCode];
    }
    if (normalizedCode >= KEYCODE_SHIFT) {
        return normalizedCode;
    }
    
    return 0;
}

uint64_t read(int fd, char *buf, uint64_t count) {
    if (count <= 0) return -1;
    
    int bytesRead = 0;
    while (bytesRead < count) {
        char c = readBuffer(buffers[fd]);
        if (c == 0) break;  // No hay más datos
        buf[bytesRead++] = c;
        if (c == '\n') break;  // Fin de línea
    }
    
    return bytesRead;
}

uint64_t write(int fd, const char *buf, uint64_t count) {
    if (count <= 0) return -1;

    int bytesWritten = 0;
    while (bytesWritten < count) {
        bufferPush(buffers[fd], buf[bytesWritten]);
        bytesWritten++;
    }

    return bytesWritten;
}