#include <tests.h>
#include <syscalls.h>
#include <cstandard.h>

#define MAX_BLOCKS 128

typedef struct {
    void *address;
    uint32_t size;
} mm_rq;

static void mm_memset(void *dst, uint8_t v, uint64_t n) {
    uint8_t *p = (uint8_t *)dst;
    for (uint64_t i = 0; i < n; i++) p[i] = v;
}

static int64_t mm_memcheck(const void *dst, uint8_t v, uint64_t n) {
    const uint8_t *p = (const uint8_t *)dst;
    for (uint64_t i = 0; i < n; i++)
        if (p[i] != v) return (int64_t)i;
    return -1;
}

int test_mm_command(int argc, char **argv) {
    if (argc != 2) {
        sys_write(STDERR, "Usage: test_mm <max_memory_bytes>\n", 34);
        sys_exit();
        return 0;
    }

    int64_t parsed = stringToInt(argv[1]);
    if (parsed <= 0) {
        sys_write(STDERR, "test_mm: max_memory invalido\n", 29);
        sys_exit();
        return 0;
    }
    uint64_t max_memory = (uint64_t)parsed;

    mm_rq mm_rqs[MAX_BLOCKS];

    while (1) {
        uint8_t rq = 0;
        uint32_t total = 0;

        while (rq < MAX_BLOCKS && total < max_memory) {
            mm_rqs[rq].size = (uint32_t)randomRange(1, (int64_t)(max_memory - total));
            mm_rqs[rq].address = sys_malloc(mm_rqs[rq].size);

            if (mm_rqs[rq].address) {
                total += mm_rqs[rq].size;
                rq++;
            }
        }

        uint32_t i;
        for (i = 0; i < rq; i++)
            if (mm_rqs[i].address)
                mm_memset(mm_rqs[i].address, (uint8_t)i, mm_rqs[i].size);

        for (i = 0; i < rq; i++)
            if (mm_rqs[i].address)
                if (mm_memcheck(mm_rqs[i].address, (uint8_t)i, mm_rqs[i].size) >= 0) {
                    printf("test_mm ERROR\n");
                    for (uint32_t j = 0; j < rq; j++)
                        if (mm_rqs[j].address)
                            sys_free(mm_rqs[j].address);
                    sys_exit();
                    return -1;
                }

        for (i = 0; i < rq; i++)
            if (mm_rqs[i].address)
                sys_free(mm_rqs[i].address);
    }
}
