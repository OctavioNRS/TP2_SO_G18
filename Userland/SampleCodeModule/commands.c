#include <shell.h>
#include <commands.h>
#include <cstandard.h>
#include <minigolf.h>
#include <hexColors.h>
#include <music.h>

// Definir mayor o igual que MAX_COMMAND_LENGTH!!
#define MAX_TOKEN_LENGTH MAX_COMMAND_LENGTH
#define MAX_TOKENS MAX_TOKEN_LENGTH/2
#define SEPARATOR ' '

typedef void (*commandFunction)(char **args, int argCount);

typedef struct
{
    char *name;
    commandFunction function;
} CommandEntry;

static void clearCommand(char **args, int argCount)
{
    clearScreen();
}

static void echoCommand(char **args, int argCount) {
    for (int i = 1; i < argCount; i++)
    {
        sys_write(STDOUT, args[i], strlen(args[i]));
        if (i < argCount-1) {
            putChar(' ');
        }
    }
    putChar('\n');
}

static void helpCommand(char **args, int argCount) {
    puts("\nAvailable commands:\n");
    puts("clear - Clears the screen");
    puts("echo <text> - Prints <text> to the screen");
    puts("color <b|f|h> <color> - Sets the (b)ackground, (f)oreground or (h)ighlight color");
    puts("scale <factor> - Sets the screen text size");
    puts("datetime - Displays current date and time");
    puts("minigolf - Starts a game of minigolf");
    puts("ls - Display the files in the current directory");
    puts("registers - Print the last snapshot of the CPU registers (Take one with F6)");
    puts("music <song> - Plays one of the available songs in our library.\nAvailable Songs:\n- r: Rickroll Intro\n- c: Coffin Dance");
    putChar('\n');
    puts("Exception tests (will crash the shell)\n");
    puts("zero - Divide by 0");
    puts("invalid - Invalid opcode");
    putChar('\n');
}

static void colorCommand(char **args, int argCount) {
    if (argCount < 2 || strlen(args[1]) != 1) {
        sys_write(STDERR, "Usage: color <b|f|h> <hexColor>\n", 30);
        return;
    }
    uint32_t color;
    int resetFlag = 0;
    if (argCount == 2) {
        resetFlag = 1;
    }
    else {
        color = hexStringToInt(args[2]);
        if (color == -1) {
            sys_write(STDERR, "Invalid color\n", 14);
            return;
        }
    }

    switch (args[1][0])
    {
        case 'b':
            color &= 0x00FFFFFF; // Para evitar setear alpha
            if (resetFlag) resetBackgroundColor();
            else setBackgroundColor(color);
            clearScreen();
            break;
        case 'f':
            if (resetFlag) resetForegroundColor();
            else setForegroundColor(color);
            break;
        case 'h':
            if (resetFlag) resetHighlightColor();
            else setHighlightColor(color);
            break;
        default:
            sys_write(STDERR, "Use b for background, f for foreground or h for highlight\n", 58);
            break;
    }
}

static char *weekdays[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

static void datetimeCommand(char **args, int argCount) {
    uint8_t hour,minute,second,weekday,day,month,year;
    sys_date(&weekday,&day,&month,&year);
    sys_time(&hour,&minute,&second);
    printf("%d:%d:%d %s %d/%d/%d\n",hour,minute,second,weekdays[weekday-1],day,month,year);
}

static void minigolfCommand(char **args, int argCount) {
    int mode = 0;
    if (argCount > 1) {
        if (strcmp(args[1],"ice") == 0) mode = 1;
    }
    initializeMinigolf(mode);
    clearScreen();
}

static void scaleCommand(char **args, int argCount) {
    int scale = stringToInt(args[1]);
    if (argCount < 2 || scale <= 0) {
        sys_write(STDERR,"Usage: scale <factor> | factor > 0\n",35);
        return;
    }
    setScale(scale);
    clearScreen();
}

static void registerCommand(char **args, int argCount) {
    if (!sys_print_registers()) {
        sys_write(STDERR, "No snapshot taken. Press F6 at any moment to take one.\n", 55);
    }
}

static void testDivideByZero(char **args, int argCount) {
    int a = argCount/0;
}

static void testInvalidOpcode(char **args, int argCount) {
    test_invalid_opcode();
}

static void rickrollCommand(char **args, int argCount) {
    sys_write(STDERR, "Of course there is no file system\n", 34);
    printOutput();
    play_song(get_rickroll_song());
}

static void musicCommand(char **args, int argCount) {
    if (argCount < 2) {
        sys_write(STDERR, "Usage: music <song>\n", 20);
        return;
    }
    if (args[1][0] == 'r') {
        printf("Now playing: Rickroll Intro\n");
    }
    else if (args[1][0] == 'c') {
        printf("Now playing: Coffin Dance\n");
    }
    printOutput();
    switch (args[1][0])
    {
    case 'r':
        play_song(get_rickroll_song());
        break;
    case 'c':
        play_song(get_coffin_intro());
        play_song(get_coffin_chorus());
        break;
    default:
        sys_write(STDERR, "Use r for Rickroll Intro, c for Coffin Dance\n", 45);
        break;
    }
}

static CommandEntry validCommands[] = {
    {"clear", clearCommand},
    {"echo", echoCommand},
    {"help", helpCommand},
    {"color", colorCommand},
    {"datetime", datetimeCommand},
    {"minigolf", minigolfCommand},
    {"scale", scaleCommand},
    {"registers", registerCommand},
    {"zero", testDivideByZero},
    {"invalid", testInvalidOpcode},
    {"ls", rickrollCommand},
    {"music", musicCommand}
};

static int getCommandIndex(char *commandName)
{
    if (commandName[0] == 0) return -2;
    for (int i = 0; i < sizeof(validCommands)/sizeof(CommandEntry); i++)
    {
        if (strcmp(commandName,validCommands[i].name) == 0) {
            return i;
        }
    }
    return -1; //Command not found
}

void interpretCommand(char *command)
{
    // Ignorar blancos iniciales
    while(*command == SEPARATOR){
        command++;
    }

    // Tokenize

    char tokenBuffer[MAX_TOKENS][MAX_TOKEN_LENGTH] = {0};
    int token = 0;
    int inTokenIndex = 0;

    for (int i = 0; command[i] != 0 && i < MAX_COMMAND_LENGTH; i++)
    {
        if (command[i] != SEPARATOR)
        {
            tokenBuffer[token][inTokenIndex++] = command[i];
        }
        else
        {
            tokenBuffer[token++][inTokenIndex] = 0;
            inTokenIndex = 0;
        }
    }
    tokenBuffer[token++][inTokenIndex] = 0;

    char *args[MAX_TOKENS];
    for(int i = 0; i < token; i++) {
        args[i] = tokenBuffer[i];
    }

    // args[0] = command name
    int commandIndex = getCommandIndex(args[0]);
    // Rerserved: -1 = not found, -2 = empty
    switch (commandIndex)
    {
    case -1:
        sys_write(STDERR, "Command not found: ", 19);
        sys_write(STDERR, args[0], strlen(args[0]));
        putc('\n', STDERR);
        return;
    case -2:
        return;
    }

    validCommands[commandIndex].function(args, token);
}