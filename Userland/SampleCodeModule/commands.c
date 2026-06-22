/*
 * commands.c — Tabla de comandos del shell.
 *
 * Cada comando es un proceso (entry point con firma int (int argc, char **argv)).
 * El shell lo crea con sys_create_process_with_fds para conectar bien los pipes.
 * Todo comando debe terminar con sys_exit().
 */
#include <commands.h>
#include <syscalls.h>
#include <cstandard.h>
#include <hexColors.h>
#include <music.h>
#include <shell.h>
#include <tests.h>
#include <mvar.h>

#define SIZE_BUFFER 128

static int writeStr(int fd, const char *s) {
    return (int) sys_write(fd, (char *)s, strlen((char *)s));
}

static int clearCommand(int argc, char **argv) {
    (void)argc; (void)argv;
    terminalClear();
    sys_exit();
    return 0;
}

static int echoCommand(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        writeStr(STDOUT, argv[i]);
        if (i < argc - 1) sys_write(STDOUT, " ", 1);
    }
    sys_write(STDOUT, "\n", 1);
    sys_exit();
    return 0;
}

static char *weekdays[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

static int datetimeCommand(int argc, char **argv) {
    (void)argc; (void)argv;
    uint8_t h, m, s, wd, d, mo, y;
    sys_date(&wd, &d, &mo, &y);
    sys_time(&h, &m, &s);
    printf("%d:%d:%d %s %d/%d/%d\n", h, m, s, weekdays[wd - 1], d, mo, y);
    sys_exit();
    return 0;
}

static int registersCommand(int argc, char **argv) {
    (void)argc; (void)argv;
    if (!sys_print_registers()) {
        writeStr(STDERR, "No snapshot taken. Press F6 at any moment to take one.\n");
    }
    sys_exit();
    return 0;
}

static int zeroCommand(int argc, char **argv) {
    (void)argc; (void)argv;
    volatile int zero = 0;
    volatile int a = 1 / zero;
    (void)a;
    sys_exit();
    return 0;
}

static int invalidCommand(int argc, char **argv) {
    (void)argc; (void)argv;
    test_invalid_opcode();
    sys_exit();
    return 0;
}

static int lsCommand(int argc, char **argv) {
    (void)argc; (void)argv;
    writeStr(STDERR, "Of course there is no file system\n");
    play_song(get_rickroll_song());
    sys_exit();
    return 0;
}

static int musicCommand(int argc, char **argv) {
    if (argc < 2) {
        writeStr(STDERR, "Usage: music <r|c>\n");
        sys_exit();
        return 0;
    }
    switch (argv[1][0]) {
        case 'r':
            writeStr(STDOUT, "Now playing: Rickroll Intro\n");
            play_song(get_rickroll_song());
            break;
        case 'c':
            writeStr(STDOUT, "Now playing: Coffin Dance\n");
            play_song(get_coffin_intro());
            play_song(get_coffin_chorus());
            break;
        default:
            writeStr(STDERR, "Use r for Rickroll, c for Coffin Dance\n");
    }
    sys_exit();
    return 0;
}

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

static void writeHex64(int fd, uint64_t value) {
    static const char hexChars[] = "0123456789ABCDEF";
    char buf[19];
    buf[0] = '0';
    buf[1] = 'x';
    for (int i = 0; i < 16; i++) {
        buf[2 + i] = hexChars[(value >> ((15 - i) * 4)) & 0xF];
    }
    buf[18] = '\0';
    sys_write(fd, buf, 18);
}

static void padTo(int currentLen, int target) {
    for (int j = currentLen; j < target; j++) putChar(' ');
}

static int psCommand(int argc, char **argv) {
    (void)argc; (void)argv;
    ProcessInfo procs[PS_MAX_PROCS];
    int n = (int) sys_ps(procs, PS_MAX_PROCS);

    printf("PID  PPID PRIO FG  STATE   RSP                STACKBASE          NAME\n");
    for (int i = 0; i < n; i++) {
        ProcessInfo *p = &procs[i];
        char num[8];

        numToString(p->pid, num);      sys_write(STDOUT, num, strlen(num));
        padTo(strlen(num), 5);

        numToString(p->ppid, num);     sys_write(STDOUT, num, strlen(num));
        padTo(strlen(num), 5);

        numToString(p->priority, num); sys_write(STDOUT, num, strlen(num));
        padTo(strlen(num), 5);

        sys_write(STDOUT, p->inForeground ? "YES " : "NO  ", 4);

        const char *st = stateName(p->state);
        sys_write(STDOUT, (char *)st, strlen((char *)st));
        padTo(strlen((char *)st), 8);

        writeHex64(STDOUT, (uint64_t)p->rsp);       putChar(' ');
        writeHex64(STDOUT, (uint64_t)p->stackBase); putChar(' ');

        printf("%s\n", p->name);
    }
    sys_exit();
    return 0;
}

static int memCommand(int argc, char **argv) {
    (void)argc; (void)argv;
    MemoryInfo m;
    sys_mem_status(&m);
    printf("Memoria (bytes):\n");
    printf("  total    : %d\n", (int)m.total);
    printf("  ocupada  : %d\n", (int)m.occupied);
    printf("  libre    : %d\n", (int)m.free);
    printf("  fragments: %d\n", (int)m.fragments);
    sys_exit();
    return 0;
}

static int killCommand(int argc, char **argv) {
    if (argc < 2) {
        writeStr(STDERR, "Usage: kill <pid>\n");
        sys_exit();
        return 0;
    }
    int64_t pid = stringToInt(argv[1]);
    if (pid < 0 || sys_kill((int16_t)pid) < 0) {
        printf("kill: no se pudo matar al pid %d\n", (int)pid);
    } else {
        printf("kill: pid %d terminado\n", (int)pid);
    }
    sys_exit();
    return 0;
}

static int niceCommand(int argc, char **argv) {
    if (argc < 3) {
        writeStr(STDERR, "Usage: nice <pid> <priority>\n");
        sys_exit();
        return 0;
    }
    int64_t pid  = stringToInt(argv[1]);
    int64_t prio = stringToInt(argv[2]);
    if (pid < 0 || prio < 1 || sys_nice((int16_t)pid, (uint8_t)prio) < 0) {
        printf("nice: argumentos invalidos o pid %d no existe\n", (int)pid);
    } else {
        printf("nice: pid %d -> prioridad %d\n", (int)pid, (int)prio);
    }
    sys_exit();
    return 0;
}

static int loopCommand(int argc, char **argv) {
    int secs = 1;
    if (argc >= 2) {
        int64_t v = stringToInt(argv[1]);
        if (v > 0) secs = (int) v;
    }
    int16_t pid = sys_getpid();
    while (1) {
        printf("Hola del PID %d\n", (int) pid);
        uint64_t start = sys_kernel_time();
        uint64_t target = start + (uint64_t)(secs * 1000);
        while (sys_kernel_time() < target) { /* espera activa */ }
    }
    sys_exit();
    return 0;
}

static int blockCommand(int argc, char **argv) {
    if (argc < 2) {
        writeStr(STDERR, "Usage: block <pid>\n");
        sys_exit();
        return 0;
    }
    int64_t pid = stringToInt(argv[1]);
    if (pid < 0) {
        writeStr(STDERR, "block: pid invalido\n");
    } else if (sys_block((int16_t)pid) == 0) {
        printf("block: pid %d bloqueado\n", (int)pid);
    } else if (sys_unblock((int16_t)pid) == 0) {
        printf("block: pid %d desbloqueado\n", (int)pid);
    } else {
        printf("block: no se pudo (pid %d)\n", (int)pid);
    }
    sys_exit();
    return 0;
}

static int catCommand(int argc, char **argv) {
    (void)argc; (void)argv;
    int size = 64;
    char buf[size];
    while (1) {
        int n = (int) sys_read(STDIN, buf, size);
        if (n <= 0) break;
        sys_write(STDOUT, buf, n);
    }
    sys_exit();
    return 0;
}

static int wcCommand(int argc, char **argv) {
    (void)argv;
    if (argc > 1) {
        writeStr(STDERR, "wc does not allow parameters\n");
        sys_exit();
        return 0;
    }
    char buf[SIZE_BUFFER];
    int lines = 0, words = 0, chars = 0;
    int inWord = 0;
    while (1) {
        int n = (int) sys_read(STDIN, buf, sizeof(buf));
        if (n <= 0) break;
        // sys_write(STDOUT, buf, n); COMPORTAMIENTO NO DESEADO AL HACER PIPE CON WC, SE MUESTRA EL TEXTO LEIDO. 
        // Ahora no se muestra lo que escribe el usuario, pero funciona correctamente
        for (int i = 0; i < n; i++) {
            chars++;
            char c = buf[i];
            if (c == '\n') lines++;
            if (c == ' ' || c == '\n' || c == '\t') {
                inWord = 0;
            } else if (!inWord) {
                inWord = 1;
                words++;
            }
        }
    }
    printf("\n %d lineas, %d palabras, %d caracteres\n", lines, words, chars);
    sys_exit();
    return 0;
}

static int isVowel(char c) {
    c = toLower(c);
    return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u';
}

static int filterCommand(int argc, char **argv) {
     if (argc >  1) {
        writeStr(STDERR, "Filter does not allow parameter\n");
        sys_exit();
        return 0;
    }
    (void)argc; (void)argv;
    char buf[128];
    char out[128];
   
    while (1) {
        int n = (int) sys_read(STDIN, buf, sizeof(buf));
        if (n <= 0) break;
        int k = 0;
        for (int i = 0; i < n; i++) {
            if (!isVowel(buf[i])) out[k++] = buf[i];
        }
        if (k > 0) sys_write(STDOUT, out, k);
    }
    sys_exit();
    return 0;
}

typedef struct {
    const char  *name;
    ProcessEntry entry;
    const char  *description;
    uint8_t      isTest;        /* 1 si es un test de cátedra, 0 si es un comando normal */
} CommandEntry;

static int helpCommand(int argc, char **argv);

static CommandEntry validCommands[] = {
    {"help",      helpCommand,      "Lista los comandos disponibles", 0},
    {"clear",     clearCommand,     "Limpia la pantalla", 0},
    {"echo",      echoCommand,      "Imprime sus argumentos", 0},
    {"datetime",  datetimeCommand,  "Imprime la fecha y hora actuales", 0},
    {"registers", registersCommand, "Imprime el snapshot de registros (toma uno con F6)", 0},
    {"ls",        lsCommand,        "Lista los archivos en el directorio actual", 0},
    {"music",     musicCommand,     "Reproduce una cancion (r=Rickroll, c=Coffin)", 0},
    {"zero",      zeroCommand,      "Provoca una division por cero (excepcion)", 0}, 
    {"invalid",   invalidCommand,   "Provoca un opcode invalido (excepcion)", 0},
    {"ps",        psCommand,        "Lista los procesos del sistema", 0},
    {"mem",       memCommand,       "Estado del heap", 0},
    {"loop",      loopCommand,      "Imprime su PID cada N segundos (espera activa)", 0},
    {"kill",      killCommand,      "Mata un proceso por PID", 0},
    {"nice",      niceCommand,      "Cambia la prioridad de un proceso", 0},
    {"block",     blockCommand,     "Bloquea/desbloquea un proceso (toggle)", 0},
    {"cat",       catCommand,       "Lee de stdin y escribe a stdout", 0},
    {"wc",        wcCommand,        "Cuenta lineas, palabras y caracteres de stdin", 0},
    {"filter",    filterCommand,    "Filtra las vocales de stdin", 0},
    {"mvar",      mvarCommand,      "MVar de Haskell: lectores/escritores sobre variable compartida (Usage: mvar <writers> <readers>)", 0},
    {"test_mm",   test_mm_command,   "Stress del memory manager (Usage: test_mm <max_bytes>)",            1},
    {"test_proc", test_proc_command, "Stress del scheduler crea/bloquea/mata procesos (Usage: test_proc <max_procs>)", 1},
    {"test_prio", test_prio_command, "Tres procesos con prioridades distintas (Usage: test_prio <max_value>)",         1},
    {"test_sync", test_sync_command, "Race conditions con y sin semaforos (Usage: test_sync <n_procesos> <n_iter> <use_sem>)", 1}
};

#define COMMAND_COUNT (sizeof(validCommands) / sizeof(CommandEntry))

static int helpCommand(int argc, char **argv) {
    (void)argc; (void)argv;
    writeStr(STDOUT, "\n" COLOR_FG("00FFFF") "Available commands:" COLOR_RESET "\n");
    for (int i = 0; i < (int)COMMAND_COUNT; i++) {
        if (validCommands[i].isTest) continue;
        printf("  " COLOR_FG("80FF80") "%s" COLOR_RESET " - %s\n",
               validCommands[i].name, validCommands[i].description);
    }
    writeStr(STDOUT, "\n" COLOR_FG("FFFF60") "Tests provistos por la catedra:" COLOR_RESET "\n");
    for (int i = 0; i < (int)COMMAND_COUNT; i++) {
        if (!validCommands[i].isTest) continue;
        printf("  " COLOR_FG("FFFF60") "%s" COLOR_RESET " - %s\n",
               validCommands[i].name, validCommands[i].description);
    }
    sys_write(STDOUT, "\n", 1);
    sys_exit();
    return 0;
}

int getCommandIndex(const char *name) {
    if (!name || !*name) return -1;
    for (int i = 0; i < (int)COMMAND_COUNT; i++) {
        if (strcmp((char *)name, (char *)validCommands[i].name) == 0) return i;
    }
    return -1;
}

ProcessEntry getCommandEntry(int idx) {
    if (idx < 0 || idx >= (int)COMMAND_COUNT) return 0;
    return validCommands[idx].entry;
}

const char *getCommandName(int idx) {
    if (idx < 0 || idx >= (int)COMMAND_COUNT) return 0;
    return validCommands[idx].name;
}

int getCommandCount(void) {
    return (int)COMMAND_COUNT;
}