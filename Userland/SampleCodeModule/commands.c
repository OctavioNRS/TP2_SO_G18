#include <shell.h>
#include <commands.h>
#include <cstandard.h>
#include <hexColors.h>
#include <music.h>
#include <tests.h>

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
    puts("ls - Display the files in the current directory");
    puts("registers - Print the last snapshot of the CPU registers (Take one with F6)");
    puts("music <song> - Plays one of the available songs in our library (r: Rickroll Intro c: Coffin Dance)");
    puts("ps                          - Lista de procesos");
    puts("mem                         - Estado del heap");
    puts("kill <pid>                  - Mata un proceso");
    puts("nice <pid> <priority>       - Cambia la prioridad de un proceso");
    puts("block <pid>                 - Bloquea/desbloquea un proceso (toggle)");

    puts("test_mm <max_memory_bytes>  - Stress del memory manager (tecla para terminar)");
    puts("test_proc <max_processes>   - Stress de procesos crear/bloquear/matar");
    puts("test_prio <max_value>       - Tres procesos con prioridades distintas");
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
    /* `volatile` evita el warning de constant division by zero; la #DE ocurre en runtime. */
    volatile int zero = 0;
    volatile int a = argCount / zero;
    (void)a;
}

static void testInvalidOpcode(char **args, int argCount) {
    test_invalid_opcode();
}

static void rickrollCommand(char **args, int argCount) {
    sys_write(STDERR, "Of course there is no file system\n", 34);
    printOutput();
    play_song(get_rickroll_song());
}

/* ---------- Comandos del scheduler/memoria (Parte 3) ---------- */

static const char *stateName(ProcState s) {
    switch (s) {
        case ST_READY:   return "READY";
        case ST_RUNNING: return "RUNNING";
        case ST_BLOCKED: return "BLOCKED";
        case ST_ZOMBIE:  return "ZOMBIE";
        default:         return "?";
    }
}

#define PS_MAX_PROCS 64

static void psCommand(char **args, int argCount) {
    ProcessInfo procs[PS_MAX_PROCS];
    int n = (int) sys_ps(procs, PS_MAX_PROCS);
    printf("PID  PPID  PRIO  STATE    NAME\n");
    for (int i = 0; i < n; i++) {
        printf("%d    %d     %d     %s",
               (int)procs[i].pid, (int)procs[i].ppid,
               (int)procs[i].priority, stateName(procs[i].state));
        /* Espacios para alinear el nombre con la columna STATE de ancho ~8 */
        const char *st = stateName(procs[i].state);
        int slen = 0; while (st[slen]) slen++;
        for (int j = slen; j < 8; j++) putChar(' ');
        printf(" %s\n", procs[i].name);
    }
    printOutput();
}

static void memCommand(char **args, int argCount) {
    MemoryInfo m;
    sys_mem_status(&m);
    printf("Memoria (bytes):\n");
    printf("  total    : %d\n", (int)m.total);
    printf("  ocupada  : %d\n", (int)m.occupied);
    printf("  libre    : %d\n", (int)m.free);
    printf("  fragments: %d\n", (int)m.fragments);
    printOutput();
}

static void killCommand(char **args, int argCount) {
    if (argCount < 2) {
        sys_write(STDERR, "Usage: kill <pid>\n", 18);
        return;
    }
    int64_t pid = stringToInt(args[1]);
    if (pid < 0) {
        sys_write(STDERR, "kill: pid invalido\n", 19);
        printOutput();
        return;
    }
    if (sys_kill((int16_t)pid) < 0) {
        printf("kill: no se pudo matar al pid %d\n", (int)pid);
        printOutput();
        return;
    }
    printf("kill: pid %d terminado\n", (int)pid);
    printOutput();
}

static void niceCommand(char **args, int argCount) {
    if (argCount < 3) {
        sys_write(STDERR, "Usage: nice <pid> <priority>\n", 29);
        return;
    }
    int64_t pid = stringToInt(args[1]);
    int64_t prio = stringToInt(args[2]);
    if (pid < 0 || prio < 1) {
        sys_write(STDERR, "nice: argumentos invalidos (priority >= 1)\n", 43);
        printOutput();
        return;
    }
    if (sys_nice((int16_t)pid, (uint8_t)prio) < 0) {
        printf("nice: no se pudo cambiar la prioridad del pid %d\n", (int)pid);
        printOutput();
        return;
    }
    printf("nice: pid %d -> prioridad %d\n", (int)pid, (int)prio);
    printOutput();
}

/* block: toggle. Intenta bloquear primero; si falla (probablemente porque el proceso ya
 * está BLOCKED), intenta desbloquearlo. Si ambas syscalls fallan, reporta error. */
static void blockCommand(char **args, int argCount) {
    if (argCount < 2) {
        sys_write(STDERR, "Usage: block <pid>\n", 19);
        return;
    }
    int64_t pid = stringToInt(args[1]);
    if (pid < 0) {
        sys_write(STDERR, "block: pid invalido\n", 20);
        printOutput();
        return;
    }
    if (sys_block((int16_t)pid) == 0) {
        printf("block: pid %d bloqueado\n", (int)pid);
    } else if (sys_unblock((int16_t)pid) == 0) {
        printf("block: pid %d desbloqueado\n", (int)pid);
    } else {
        printf("block: no se pudo (pid %d)\n", (int)pid);
    }
    printOutput();
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
    {"scale", scaleCommand},
    {"registers", registerCommand},
    {"zero", testDivideByZero},
    {"invalid", testInvalidOpcode},
    {"ls", rickrollCommand},
    {"music", musicCommand},
    /* Scheduler / memoria */
    {"ps",      psCommand},
    {"mem",     memCommand},
    {"kill",    killCommand},
    {"nice",    niceCommand},
    {"block",   blockCommand},
    /* Tests */
    {"test_mm",   test_mm_command},
    {"test_proc", test_proc_command},
    {"test_prio", test_prio_command}
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