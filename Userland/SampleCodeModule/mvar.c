/*
 * mvar.c — Productor-consumidor con variable compartida de un slot.
 *
 * Un slot (char) protegido por 3 semáforos nombrados:
 *   - exclusion (init 1): mutex para acceso atómico al slot.
 *   - slotLibre (init 1): indica que el slot está vacío (writers esperan acá).
 *   - slotOcupado (init 0): indica que hay un dato (readers esperan acá).
 *
 * Protocolo writer: wait(slotLibre) → wait(exclusion) → escribir → post(exclusion) → post(slotOcupado)
 * Protocolo reader: wait(slotOcupado) → wait(exclusion) → leer → post(exclusion) → post(slotLibre)
 *
 * El proceso principal (mvarCommand) crea los hijos, los agrega al foreground
 * set, y se queda en un yield loop para que la shell bloquee en waitpid.
 * Ctrl+C mata al padre y a todos los hijos (todos están en foreground).
 */
#include <mvar.h>
#include <syscalls.h>
#include <cstandard.h>

#define SEM_EXCL       "mv_excl"
#define SEM_SLOT_LIBRE "mv_libre"
#define SEM_SLOT_LLENO "mv_lleno"

#define MAX_WORKERS 10

static const char *colorPalette[] = {
    "FF6060", "60FF60", "6060FF", "FFFF60", "FF60FF", "60FFFF",
    "FFA030", "30FFA0", "A030FF", "FF80B0"
};
#define PALETTE_LEN (sizeof(colorPalette) / sizeof(colorPalette[0]))

static volatile char slotValue = 0;

static void randomDelay(void) {
    int ms = (int)(randomNext() % 400u + 100u);
    uint64_t t0 = sys_kernel_time();
    while ((sys_kernel_time() - t0) < (uint64_t)ms) {}
}

static int producerEntry(int argc, char **argv) {
    (void)argc;
    char ch = argv[1][0];

    setRngSeed((uint16_t)(sys_kernel_time() + sys_getpid()));

    while (1) {
        randomDelay();
        sys_sem_wait(SEM_SLOT_LIBRE);
        sys_sem_wait(SEM_EXCL);
        slotValue = ch;
        sys_sem_post(SEM_EXCL);
        sys_sem_post(SEM_SLOT_LLENO);
    }
    sys_exit();
    return 0;
}

static int consumerEntry(int argc, char **argv) {
    (void)argc;
    int idx = (int)stringToInt(argv[1]);
    const char *color = colorPalette[idx % PALETTE_LEN];

    setRngSeed((uint16_t)(sys_kernel_time() + sys_getpid()));

    char out[16];
    while (1) {
        randomDelay();
        sys_sem_wait(SEM_SLOT_LLENO);
        sys_sem_wait(SEM_EXCL);
        char ch = slotValue;
        sys_sem_post(SEM_EXCL);
        sys_sem_post(SEM_SLOT_LIBRE);

        int n = 0;
        out[n++] = 0x1B; out[n++] = 'f';
        for (int i = 0; i < 6; i++) out[n++] = color[i];
        out[n++] = ';';
        out[n++] = ch;
        out[n++] = 0x1B; out[n++] = 'r'; out[n++] = ';';
        sys_write(STDOUT, out, n);
    }
    sys_exit();
    return 0;
}

static int writeErr(const char *s) {
    return (int)sys_write(STDERR, (char *)s, strlen((char *)s));
}

int mvarCommand(int argc, char **argv) {
    if (argc < 3) {
        writeErr("Usage: mvar <writers> <readers>\n");
        sys_exit();
        return 0;
    }

    int64_t nw = stringToInt(argv[1]);
    int64_t nr = stringToInt(argv[2]);
    if (nw <= 0 || nw + nr > MAX_WORKERS || nr <= 0 ) {
        writeErr("mvar: writers y readers deben ser entre 1 y 10\n");
        sys_exit();
        return 0;
    }

    slotValue = 0;

    sys_sem_close(SEM_EXCL);
    sys_sem_close(SEM_SLOT_LIBRE);
    sys_sem_close(SEM_SLOT_LLENO);

    sys_sem_open(SEM_EXCL, 1);
    sys_sem_open(SEM_SLOT_LIBRE, 1);
    sys_sem_open(SEM_SLOT_LLENO, 0);

    for (int i = 0; i < (int)nw; i++) {
        char letter[2] = {(char)('A' + (i % 26)), '\0'};
        char *args[2] = {"mvar_w", letter};
        int64_t pid = sys_create_process(producerEntry, 2, args, "mvar_w");
        if (pid >= 0) sys_foreground((int16_t)pid);
    }
    for (int i = 0; i < (int)nr; i++) {
        char idStr[8];
        numToString((long)i, idStr);
        char *args[2] = {"mvar_r", idStr};
        int64_t pid = sys_create_process(consumerEntry, 2, args, "mvar_r");
        if (pid >= 0) sys_foreground((int16_t)pid);
    }

    while (1)
        sys_yield();

    sys_exit();
    return 0;
}