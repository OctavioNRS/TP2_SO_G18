/*
 * shell.c — Shell interactiva en userland.
 */
#include <shell.h>
#include <commands.h>
#include <terminal.h>
#include <syscalls.h>
#include <cstandard.h>
#include <hexColors.h>

#define MAX_TOKENS    16
#define MAX_PIPELINE  2       /* máx etapas p1 | p2 (un solo `|`) */

void terminalClear(void) {
    sys_write(STDOUT, "\f", 1);
}


static void launchTerminal(void) {

    sys_dup(STDIN, STDOUT);
    sys_dup(STDIN, STDERR);

    int pipeFds[2];
    if (sys_pipe(pipeFds) < 0) return;


    sys_dup(pipeFds[1], STDOUT);
    sys_dup(pipeFds[1], STDERR);
    sys_close(pipeFds[1]);


    fd_override_t overrides[] = {
        {STDIN,     pipeFds[0]},
        {STDOUT,    -1},
        {STDERR,    -1},
        {pipeFds[0], -1}
    };
    sys_create_process_with_fds(terminalProcess, 0, 0, "terminal", overrides, 4);

    sys_close(pipeFds[0]);
}


static int readLine(char *line, int maxLen) {
    int idx = 0;
    while (1) {
        char c;
        int n = (int) sys_read(STDIN, &c, 1);
        if (n <= 0) return -1;     /* EOF (Ctrl+D) o error */
        if (c == '\0') continue;   /* despertador de Ctrl+D, ignorar */

        if (c == '\n') {
            putChar('\n');
            line[idx] = '\0';
            return idx;
        }
        if (c == '\b') {
            if (idx > 0) {
                idx--;
                line[idx] = '\0';
                putChar('\b');
            }
            continue;
        }
        if (idx < maxLen - 1 && isPrintableAscii(c)) {
            line[idx++] = c;
            putChar(c);
        }
    }
}

static int tokenize(char *str, char **tokens, int maxTokens) {
    int n = 0;
    while (*str && n < maxTokens) {
        while (*str == ' ' || *str == '\t') str++;
        if (!*str) break;
        tokens[n++] = str;
        while (*str && *str != ' ' && *str != '\t') str++;
        if (*str) { *str = '\0'; str++; }
    }
    return n;
}

/* Encuentra y separa los `|` en la cadena (in-place). Devuelve cantidad de etapas, con
 * sus punteros en `stages[]`. Devuelve -1 si aparece un `|` adicional que excede
 * `maxStages` (la consigna pide soportar a lo sumo 2 etapas = 1 sólo `|`). */
static int splitPipeline(char *line, char **stages, int maxStages) {
    int n = 0;
    char *p = line;
    stages[n++] = p;
    while (*p) {
        if (*p == '|') {
            if (n >= maxStages) return -1;
            *p = '\0';
            p++;
            while (*p == ' ') p++;
            stages[n++] = p;
        } else {
            p++;
        }
    }
    return n;
}

/* Verifica si la línea termina con `&` (y la trunca). */
static int hasBackground(char *line) {
    int len = strlen(line);
    while (len > 0 && (line[len-1] == ' ' || line[len-1] == '\t')) {
        line[--len] = '\0';
    }
    if (len > 0 && line[len-1] == '&') {
        line[--len] = '\0';
        while (len > 0 && (line[len-1] == ' ' || line[len-1] == '\t')) {
            line[--len] = '\0';
        }
        return 1;
    }
    return 0;
}

static int runStage(char *stageStr, int stdinFd, int stdoutFd, int background,
                    int *closeFds, int nCloseFds) {
    char *tokens[MAX_TOKENS];
    int nTokens = tokenize(stageStr, tokens, MAX_TOKENS);
    if (nTokens == 0) return -1;

    int cmdIdx = getCommandIndex(tokens[0]);
    if (cmdIdx < 0) {
        const char *prefix = COLOR_FG("FF4040") "Command not found: ";
        const char *suffix = COLOR_RESET "\n";
        sys_write(STDERR, (char *)prefix, strlen(prefix));
        sys_write(STDERR, tokens[0], strlen(tokens[0]));
        sys_write(STDERR, (char *)suffix, strlen(suffix));
        return -1;
    }

    fd_override_t overrides[3 + MAX_PIPELINE * 2];
    int nOverrides = 0;
    if (stdinFd == -1) {
        overrides[nOverrides++] = (fd_override_t){STDIN, -1};
    } else if (stdinFd != STDIN) {
        overrides[nOverrides++] = (fd_override_t){STDIN, stdinFd};
    }
    if (stdoutFd != STDOUT) overrides[nOverrides++] = (fd_override_t){STDOUT, stdoutFd};
    for (int i = 0; i < nCloseFds; i++) {
        overrides[nOverrides++] = (fd_override_t){closeFds[i], -1};
    }

    int pid = (int) sys_create_process_with_fds(
        getCommandEntry(cmdIdx), nTokens, tokens, tokens[0],
        nOverrides > 0 ? overrides : 0, nOverrides);

    if (pid < 0) {
        sys_write(STDERR, "shell: cannot spawn\n", 20);
        return -1;
    }

    if (!background) sys_foreground((int16_t) pid);
    return pid;
}

static void runLine(char *line) {
    int background = hasBackground(line);

    char *stages[MAX_PIPELINE];
    int nStages = splitPipeline(line, stages, MAX_PIPELINE);
    if (nStages < 0 ) {
        sys_write(STDERR, "shell: solo se admite un `|` por linea\n", 39);
        return;
    }
    if (nStages == 0) return;

    int childStdin = (int)(int64_t) sys_reopen(STDIN);
    int firstStdin;
    if (childStdin < 0) {

        firstStdin = background ? -1 : STDIN;
    } else {
        firstStdin = background ? -1 : childStdin;
    }

    /* Caso simple: una sola etapa. */
    if (nStages == 1) {
        int closeOnChild[1];
        int nClose = 0;
        if (childStdin >= 0) closeOnChild[nClose++] = childStdin;
        int pid = runStage(stages[0], firstStdin, STDOUT, background,
                           nClose > 0 ? closeOnChild : 0, nClose);
        if (pid >= 0 && !background) {
            sys_waitpid((int16_t) pid);
            putChar('\n');
        }
        if (childStdin >= 0) sys_close(childStdin);
        return;
    }

    // Caso con nStages == 2 (no puede ser mayor, lo chequea splitPipeline)
    int pipeFds[2];
    if (sys_pipe(pipeFds) < 0) {
        sys_write(STDERR, "shell: cannot create pipe\n", 26);
        if (childStdin >= 0) sys_close(childStdin);
        return;
    }

    int closeOnFirst[2];
    int nCloseFirst = 0;
    closeOnFirst[nCloseFirst++] = pipeFds[0];
    if (childStdin >= 0 && childStdin != firstStdin)
        closeOnFirst[nCloseFirst++] = childStdin;
    int pid1 = runStage(stages[0], firstStdin, pipeFds[1], background,
                        closeOnFirst, nCloseFirst);

    if (pid1 < 0) {
        sys_close(pipeFds[0]);
        sys_close(pipeFds[1]);
        if (childStdin >= 0) sys_close(childStdin);
        return;
    }

    int closeOnSecond[2];
    int nCloseSecond = 0;
    closeOnSecond[nCloseSecond++] = pipeFds[1];
    if (childStdin >= 0)
        closeOnSecond[nCloseSecond++] = childStdin;
    int pid2 = runStage(stages[1], pipeFds[0], STDOUT, background,
                        closeOnSecond, nCloseSecond);

    sys_close(pipeFds[0]);
    sys_close(pipeFds[1]);

    if (pid2 < 0) {
        sys_kill((int16_t) pid1);
        if (!background) sys_waitpid((int16_t) pid1);
        if (childStdin >= 0) sys_close(childStdin);
        return;
    }

    if (!background) {
        sys_waitpid((int16_t) pid1);
        sys_waitpid((int16_t) pid2);
        putChar('\n');
    }

    if (childStdin >= 0) sys_close(childStdin);
}


#define PROMPT  COLOR_FG("00FFFF") "[SHELL]" COLOR_RESET " > "
#define WELCOME COLOR_FG("00FF80") "Welcome to TP2 SO G18 shell." COLOR_RESET " Type `help`.\n"

static void shellLoop(void) {
    char line[MAX_COMMAND_LENGTH];
    while (1) {
        sys_reap_zombies();
        sys_write(STDOUT, PROMPT, strlen(PROMPT));
        int n = readLine(line, MAX_COMMAND_LENGTH);
        if (n < 0) return;            /* EOF → fin del shell */
        if (n == 0) continue;
        runLine(line);
    }
}

int shellProcess(int argc, char **argv) {
    (void)argc; (void)argv;
    launchTerminal();
    sys_yield();
    terminalClear();
    sys_write(STDOUT, WELCOME, strlen(WELCOME));
    shellLoop();
    sys_exit();
    return 0;
}
