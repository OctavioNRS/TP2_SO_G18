/*
 * terminal.c
 */
#include <terminal.h>
#include <syscalls.h>
#include <cstandard.h>
#include <hexColors.h>

#define CHAR_W      8     /* ancho de glifo del font del kernel */
#define CHAR_H      16    /* alto de glifo */
#define BASELINE_Y  12    /* offset que usa drawSymbol para la línea base */
#define TAB_WIDTH   4

#define DEFAULT_FG  WHITE
#define DEFAULT_BG  BLACK

#define READ_BUF       64
#define ESC            0x1B
#define ESC_PAYLOAD_MAX 16

static int      cursorCol = 0;
static int      cursorRow = 0;
static int      cols = 0;
static int      rows = 0;
static int      screenW = 0;
static int      screenH = 0;
static uint32_t fgColor = DEFAULT_FG;
static uint32_t bgColor = DEFAULT_BG;

/* Estado del parser de escape sequences. */
typedef enum { ST_NORMAL, ST_ESC_CMD, ST_ESC_PAYLOAD } EscState;
static EscState escState = ST_NORMAL;
static char     escCmd = 0;
static char     escPayload[ESC_PAYLOAD_MAX];
static int      escPayloadLen = 0;

static void initLayout(void) {
    screenW = (int) sys_screen_width();
    screenH = (int) sys_screen_height();
    cols = screenW / CHAR_W;
    rows = screenH / CHAR_H;
}

static void clearScreen(void) {
    sys_background(bgColor);
    cursorCol = cursorRow = 0;
}

static void newline(void) {
    cursorCol = 0;
    cursorRow++;
    if (cursorRow >= rows) {
        sys_scroll_up(CHAR_H, bgColor);
        cursorRow = rows - 1;
    }
}

static void backspace(void) {
    if (cursorCol == 0) {
        if (cursorRow == 0) return;
        cursorRow--;
        cursorCol = cols - 1;
    } else {
        cursorCol--;
    }
    int x = cursorCol * CHAR_W;
    int y = cursorRow * CHAR_H + BASELINE_Y;
    sys_draw_rect(bgColor, x, y - BASELINE_Y, CHAR_W, CHAR_H);
}

/* Aplica el comando de la escape sequence con el payload ya null-terminated. */
static void applyEscape(char cmd, char *payload) {
    switch (cmd) {
        case 'c':
            clearScreen();
            break;
        case 'f':
            fgColor = (uint32_t) hexStringToInt(payload);
            break;
        case 'b':
            bgColor = (uint32_t) hexStringToInt(payload);
            clearScreen();
            break;
        case 'r':
            fgColor = DEFAULT_FG;
            bgColor = DEFAULT_BG;
            break;
        default:
            break;
    }
}

static void drawNormalChar(char c) {
    if (cursorCol >= cols) newline();
    int x = cursorCol * CHAR_W;
    int y = cursorRow * CHAR_H + BASELINE_Y;
    sys_draw_char((unsigned char) c, x, y, fgColor, bgColor, 1);
    cursorCol++;
}

static void termPutChar(char c) {
    if (escState == ST_ESC_CMD) {
        escCmd = c;
        escPayloadLen = 0;
        escState = ST_ESC_PAYLOAD;
        return;
    }
    if (escState == ST_ESC_PAYLOAD) {
        if (c == ';') {
            escPayload[escPayloadLen] = '\0';
            applyEscape(escCmd, escPayload);
            escState = ST_NORMAL;
        } else if (escPayloadLen < ESC_PAYLOAD_MAX - 1) {
            escPayload[escPayloadLen++] = c;
        }
        return;
    }

    if (c == ESC) { escState = ST_ESC_CMD; return; }

    switch (c) {
        case '\0': return;
        case '\n': newline(); return;
        case '\r': cursorCol = 0; return;
        case '\b': backspace(); return;
        case '\f': clearScreen(); return;
        case '\t':
            for (int i = 0; i < TAB_WIDTH; i++) drawNormalChar(' ');
            return;
        default:
            drawNormalChar(c);
            return;
    }
}

int terminalProcess(int argc, char **argv) {
    (void)argc; (void)argv;
    initLayout();
    clearScreen();

    char buf[READ_BUF];
    while (1) {
        int64_t n = (int64_t) sys_read(STDIN, buf, READ_BUF);
        if (n <= 0) {
            /* EOF (writer cerró). Salimos del loop y terminamos el proceso. */
            break;
        }
        for (int64_t i = 0; i < n; i++) termPutChar(buf[i]);
    }
    sys_exit();
    return 0;
}
