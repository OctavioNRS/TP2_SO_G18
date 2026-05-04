#include <shell.h>
#include <point.h>
#include <hexColors.h>
#include <syscalls.h>
#include <commands.h>
#include <cstandard.h>
#include <music.h>

#define CHAR_WIDTH (8*textScale)
#define CHAR_HEIGHT (16*textScale)
#define CURSOR_HEIGHT (1*textScale)
#define CURSOR_INIT {8,16}

#define DEFAULT_BG BLACK
#define DEFAULT_FG WHITE
#define DEFAULT_HL -1

static point2D cursor = CURSOR_INIT;
static point2D cursorStart = CURSOR_INIT;
static point2D cursorCommandStart = CURSOR_INIT;

static uint32_t backgroundColor = DEFAULT_BG;
static uint32_t foregroundColor = DEFAULT_FG;
static uint32_t highlightColor = DEFAULT_HL;

static uint16_t screenWidth;
static uint16_t screenHeight;

static int textScale = 1;

static void setCursorStartPos() {
    cursorStart = (point2D){CHAR_WIDTH,CHAR_HEIGHT};
}

uint32_t getBackgroundColor() {
    return backgroundColor;
}

void setBackgroundColor(uint32_t color) {
    backgroundColor = color;
}

uint32_t getForegroundColor() {
    return foregroundColor;
}

void setForegroundColor(uint32_t color) {
    foregroundColor = color;
}

void setHighlightColor(uint32_t color) {
    highlightColor = color;
}

void resetBackgroundColor() {
    backgroundColor = DEFAULT_BG;
}
void resetForegroundColor() {
    foregroundColor = DEFAULT_FG;
}
void resetHighlightColor() {
    highlightColor = DEFAULT_HL;
}

static void getScreenSize() {
    screenWidth = sys_screen_width();
    screenHeight = sys_screen_height();
}

void setScale(int scale) {
    if (scale <= 0) return;
    textScale = scale;
    setCursorStartPos();
    getScreenSize();
    //floor(screen_width / char_width)*char_width = max_chars*char_width = max_x
    screenWidth = (screenWidth/CHAR_WIDTH)*CHAR_WIDTH;
    screenHeight = (screenHeight/CHAR_HEIGHT)*CHAR_HEIGHT;
}


static void cursorRect(uint32_t color) {
    sys_draw_rect(color, cursor.x, cursor.y - CURSOR_HEIGHT, CHAR_WIDTH, CURSOR_HEIGHT);
}

static void drawChar(unsigned char c, uint32_t hexColor)
{
    sys_draw_char(c, cursor.x, cursor.y, hexColor, -1, textScale);
}

static void drawCharBg(unsigned char c, uint32_t hexColor, uint32_t hexBgColor)
{
    if (hexBgColor == -1) {
        drawChar(c, hexColor);
        return;
    }
    // drawSymbol(c,cursor.x,cursor.y,hexColor,hexBgColor);
    sys_draw_char(c, cursor.x, cursor.y, hexColor, hexBgColor, textScale);
}

static void setCursor(point2D newCursor) {
    cursor = newCursor;
    cursorRect(foregroundColor);
}

static void resetCursor() {
    setCursor(cursorStart);
}

static void carriageReturn() {
    setCursor(point(cursorStart.x, cursor.y));
}

static void newLine()
{
    cursor.y += CHAR_HEIGHT;
    carriageReturn();
}

static void moveCursor() {
    cursor.x += CHAR_WIDTH;
    if(cursor.x >= screenWidth) {
        newLine();
    }
}

void clearScreen()
{
    //sys_draw_rect(backgroundColor, 0, 0, screenWidth, screenHeight);
    sys_background(backgroundColor);
    resetCursor();
}

static void checkScreenHeightEnd() {
    if(cursor.y > screenHeight) {
        clearScreen();
    }
}

static int specialCharacterCheck(unsigned char c) {
    if(c == '\n') {
        newLine();
        return 1;
    } else if(c == '\r') {
        carriageReturn();
        return 1;
    }
    return 0;
}

static void printCharBg(unsigned char c, uint32_t hexColor, uint32_t hexBgColor)
{
    cursorRect(backgroundColor);
    int special = specialCharacterCheck(c);
    checkScreenHeightEnd();
    if (special) return;
    drawCharBg(c, hexColor, hexBgColor);
    moveCursor();
    cursorRect(foregroundColor);
}

static void printChar(unsigned char c, uint32_t hexColor)
{
    printCharBg(c, hexColor, highlightColor);
}

static void cursorBack()
{
    if (cursor.x <= cursorCommandStart.x && cursor.y <= cursorCommandStart.y) return;
    cursor.x -= CHAR_WIDTH;
    if (cursor.x < cursorStart.x) {
        cursor.x = screenWidth-CHAR_WIDTH;
        cursor.y -= CHAR_HEIGHT;
    }
}

static void backspace()
{
    cursorRect(backgroundColor);
    cursorBack();
    drawCharBg(' ',backgroundColor,backgroundColor);
    cursorRect(foregroundColor);
}

static void printStrBg(char *str, uint32_t hexColor, uint32_t hexBgColor)
{
    while(*str){
        printCharBg(*str,hexColor,hexBgColor);
        str++;
    }
}

static void printStr(char *str, uint32_t hexColor)
{
    printStrBg(str, hexColor, highlightColor);
}

static void print(char *str) {
    printStr(str, foregroundColor);
}

// Command interpreter

// Segundo byte de los multibytes que puede leer
#define ARROW_UP 0x48
#define ARROW_LEFT 0x4B
#define ARROW_RIGHT 0x4D
#define ARROW_DOWN 0x50

#define MAX_SAVED_COMMANDS 16

static char *shellPrompt = "[ARQ-SHELL] > ";

static char commandHistory[MAX_SAVED_COMMANDS][MAX_COMMAND_LENGTH+1] = {0};
static int saveIndex = 0;
static int historyAccessIndex = 0;

static char *accessHistory(char key) {
    int shift;
    switch (key)
    {
    case ARROW_UP:
        shift=1;
        break;
    
    case ARROW_DOWN:
        shift=-1;
        break;
    
    default:
        return (char*)(-1);
    }
    int readIndex = clamp(historyAccessIndex+shift, -1, MAX_SAVED_COMMANDS-1);
    if (readIndex == -1) {
        historyAccessIndex = readIndex;
        return "";
    }
    if (commandHistory[readIndex][0] == '\0') {
        return (char*)(-1);
    }
    historyAccessIndex = readIndex;
    return commandHistory[readIndex];
}

static void addToHistory(char *command) {
    for (int i = MAX_SAVED_COMMANDS-1; i > 0; i--)
    {
        for (int j = 0; j < MAX_COMMAND_LENGTH+1; j++)
        {
            commandHistory[i][j] = commandHistory[i-1][j];
        }
    }
    int write = 0;
    while(command[write] != 0)
    {
        commandHistory[0][write] = command[write];
        write++;
    }
    while(commandHistory[0][write] != 0) {
        commandHistory[0][write++] = 0;
    }
    
    if (saveIndex < MAX_SAVED_COMMANDS-1) saveIndex++;
    historyAccessIndex = -1;
}

void printOutput() {
    char c_out, c_err;
    do {
        // Imprimir la salida estandard
        c_out = getc(STDOUT);
        if (c_out) {
            printChar(c_out, foregroundColor);
        }
        else {
            // Si no hay salida estandard, imprimir la salida de error
            c_err = getc(STDERR);
            if (c_err) {
                printChar(c_err, RED);
            }
        }
    } while(c_out || c_err);
}

static char specialKey(char c) {
    // Deshace el shift provocado por el driver para obtener
    // el verdadero segundo byte del scancode
    return c -= 0x7F;
}

// A pesar de que esta funcion llama todo "command", se ocupa de recibir input del usuario en cualquier momento.
static int readUserInput() {
    cursorCommandStart = cursor;
    int writingCommand = 1;
    char commandBuffer[MAX_COMMAND_LENGTH+1] = {0};
    int commandIndex = 0;

    while(writingCommand) {
        // Leer entrada del usuario
        char c = getc(STDIN);
        printOutput();

        if (!isPrintableAscii(c)) {
            c = specialKey(c);

            // Acceso al historial de comandos           
            if (c == ARROW_UP || c == ARROW_DOWN) {
                char *prevCommand = accessHistory(c);
                if (prevCommand != (char*)(-1)) {
                    while (commandIndex > 0) {
                        backspace();
                        commandBuffer[commandIndex--] = 0;
                    }
                    sys_write(STDIN, prevCommand, strlen(prevCommand));
                }
            }

            continue;
        }

        // Escritura del comando
        if (c == '\b') {
            backspace();
            if (commandIndex > 0) commandIndex--;
            commandBuffer[commandIndex] = 0;
        }
        else if (commandIndex < MAX_COMMAND_LENGTH) {
            printChar(c, foregroundColor);
            if (c == '\n') {
                writingCommand = 0;
            }
            else commandBuffer[commandIndex++] = c;
        }
        else {
            if (c == '\n') {
                writingCommand = 0;
                printChar(c, foregroundColor);
            }
        }
    }
    return sys_write(STDIN, commandBuffer, strlen(commandBuffer));
}

void inputMode(int acceptsEmpty) {
    if (acceptsEmpty) {
        readUserInput();
    }
    else {
        while (readUserInput() == -1);
    }
}

static void writeCommand() {
    inputMode(1);
    char command[MAX_COMMAND_LENGTH+1] = {0};
    scanf("%s",command);
    if (command[0] != '\0') addToHistory(command);
    interpretCommand(command);
}

static void shellLoop() {
    while(1) {
        printOutput();
        print(shellPrompt);
        writeCommand();
    }
}

static void drawTitle() {
    uint64_t textWidth = CHAR_WIDTH * 28 * 2;
    uint64_t bottomLeftX = screenWidth/2 - textWidth/2;
    sys_draw_string("Arquitectura de Computadoras",bottomLeftX,36,GREEN,-1,2);
}

void initializeShell() {
    textScale = 1;
    setCursorStartPos();
    getScreenSize();
    clearScreen();
    print("\n\n\n");
    drawTitle();
    init_music();
    shellLoop();
}