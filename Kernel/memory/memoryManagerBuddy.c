/*
 * memoryManagerBuddy.c — Administrador de memoria por BUDDY SYSTEM.
 *
 * Se compila cuando está definido USE_BUDDY (`make buddy`).
 *
 * Diseño: la región se administra como bloques de tamaño potencia de 2 (2^order).
 * Hay una free list por cada orden. `mem_alloc` toma el menor orden que satisface
 * el pedido; si no hay, parte un bloque de orden mayor en dos "buddies". `mem_free`
 * intenta fusionar el bloque con su buddy (la otra mitad del bloque de orden
 * superior, ubicado en offset XOR 2^order) mientras el buddy esté libre y del
 * mismo orden. Cada bloque lleva un encabezado con su orden, y los bloques libres
 * encadenan su free list reutilizando el espacio del payload.
 */
#ifdef USE_BUDDY

#include <memoryManager.h>

#define MIN_ORDER 6                 /* bloque mínimo: 2^6 = 64 bytes  */
#define MAX_ORDER 40                /* soporta hasta 2^40             */

typedef struct FreeNode {
    struct FreeNode *next;          /* siguiente bloque libre del mismo orden */
} FreeNode;

typedef struct {
    uint32_t order;                 /* tamaño del bloque = 2^order */
    uint32_t free;                  /* 1 = libre, 0 = ocupado      */
} Header;

static uint8_t  *heapBase;
static uint64_t  heapTotal;
static uint32_t  maxOrder;
static FreeNode *freeLists[MAX_ORDER + 1];
static uint64_t  freeBytes;

static uint32_t orderForSize(uint64_t size) {
    uint64_t need = size + sizeof(Header);
    uint32_t order = MIN_ORDER;
    while (((uint64_t)1 << order) < need) order++;
    return order;
}

static void pushFree(uint8_t *block, uint32_t order) {
    Header *h = (Header *) block;
    h->order = order;
    h->free = 1;
    FreeNode *node = (FreeNode *)(block + sizeof(Header));
    node->next = freeLists[order];
    freeLists[order] = node;
}

static uint8_t *popFree(uint32_t order) {
    FreeNode *node = freeLists[order];
    if (node == NULL) return NULL;
    freeLists[order] = node->next;
    return (uint8_t *) node - sizeof(Header);
}

static void removeFree(uint8_t *block, uint32_t order) {
    FreeNode *target = (FreeNode *)(block + sizeof(Header));
    FreeNode **pp = &freeLists[order];
    while (*pp != NULL) {
        if (*pp == target) { *pp = target->next; return; }
        pp = &(*pp)->next;
    }
}

void mem_init(void *start, uint64_t size) {
    heapBase = (uint8_t *) start;
    heapTotal = size;
    for (int i = 0; i <= MAX_ORDER; i++) freeLists[i] = NULL;

    maxOrder = MIN_ORDER;
    while ((maxOrder + 1) <= MAX_ORDER && (((uint64_t)1 << (maxOrder + 1)) <= size))
        maxOrder++;

    pushFree(heapBase, maxOrder);
    freeBytes = ((uint64_t)1 << maxOrder);
}

void *mem_alloc(uint64_t size) {
    if (size == 0 || heapBase == NULL) return NULL;
    uint32_t order = orderForSize(size);
    if (order > maxOrder) return NULL;

    /* Buscar el menor orden disponible >= order */
    uint32_t cur = order;
    while (cur <= maxOrder && freeLists[cur] == NULL) cur++;
    if (cur > maxOrder) return NULL;

    uint8_t *block = popFree(cur);

    /* Partir hacia abajo, dejando libres los buddies sobrantes */
    while (cur > order) {
        cur--;
        pushFree(block + ((uint64_t)1 << cur), cur);
    }

    Header *h = (Header *) block;
    h->order = order;
    h->free = 0;
    freeBytes -= ((uint64_t)1 << order);
    return block + sizeof(Header);
}

void mem_free(void *ptr) {
    if (ptr == NULL) return;
    uint8_t *block = (uint8_t *) ptr - sizeof(Header);
    uint32_t order = ((Header *) block)->order;
    freeBytes += ((uint64_t)1 << order);

    /* Fusionar con el buddy mientras se pueda */
    while (order < maxOrder) {
        uint64_t offset = (uint64_t)(block - heapBase);
        uint8_t *buddy = heapBase + (offset ^ ((uint64_t)1 << order));
        Header *bh = (Header *) buddy;
        if (!bh->free || bh->order != order) break;
        removeFree(buddy, order);
        if (buddy < block) block = buddy;   /* el bloque fusionado empieza en el menor */
        order++;
    }
    pushFree(block, order);
}

void mem_status(MemoryInfo *info) {
    if (info == NULL) return;
    info->total = ((uint64_t)1 << maxOrder);
    info->free = freeBytes;
    info->occupied = info->total - freeBytes;
    info->fragments = 0;
    for (uint32_t o = MIN_ORDER; o <= maxOrder; o++) {
        for (FreeNode *n = freeLists[o]; n != NULL; n = n->next)
            info->fragments++;
    }
}

#endif /* USE_BUDDY */
