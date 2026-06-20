/*
 * memoryManagerBuddy.c — Administrador de memoria por BUDDY SYSTEM con árbol.
 *
 * Se compila cuando está definido USE_BUDDY (`make buddy`).
 *
 * Diseño: árbol binario lazy con pool estático de nodos. Cada nodo representa un
 * bloque potencia-de-2 del pool; los nodos sólo se materializan cuando se hace
 * split. Al fusionarse dos buddies, sus nodos vuelven a una free-list de slots
 * (lista enlazada simple usando `leftChild` como puntero "next" y `parent = -1`
 * como marca de inválido), para evitar agotar el pool con allocs/frees repetidos.
 *
 *   nodes[i].order      → tamaño = 2^order
 *   nodes[i].offset     → offset desde minAddress
 *   nodes[i].isFree     → 1 si todo el subárbol está libre
 *   nodes[i].{left,right}Child  → hijos al hacer split (-1 si es hoja)
 *   nodes[i].parent     → padre (-1 si raíz o si está reciclado)
 *
 * Interfaz: este archivo expone la misma API que el resto del kernel espera
 * (mem_init/mem_alloc/mem_free/mem_status + MemoryInfo, ver memoryManager.h).
 */
#ifdef USE_BUDDY

#include <memoryManager.h>
#include <stdint.h>

#define MAX_ORDER 12   /* Orden máximo lógico para el helper de tamaño     */
#define MIN_ORDER 4    /* Orden mínimo: 2^4 = 16 bytes (alocación más chica) */
#define MAX_NODES 8192 /* Cantidad máxima de nodos vivos del árbol           */

typedef struct BuddyNode {
    uint8_t  isFree;     /* 1 = subárbol completamente libre, 0 = ocupado o parcial */
    uint8_t  order;      /* orden del bloque (tamaño = 2^order)               */
    uint64_t offset;     /* desplazamiento desde minAddress                   */
    int      leftChild;  /* índice del hijo izquierdo (-1 si no tiene)        */
    int      rightChild; /* índice del hijo derecho (-1 si no tiene)          */
    int      parent;     /* índice del padre (-1 si es raíz o si está libre)  */
} BuddyNode;

static uint8_t  *minAddress;
static uint64_t  totalSize;
static uint64_t  maxOrder;

static BuddyNode nodes[MAX_NODES];
static int       nodeCount;
static int       rootIndex;
static int       firstFreeNode;  /* cabeza de la free-list de slots reciclables */

/* log2 entero (piso): la lib del kernel no expone una, así que la metemos local. */
static int log2Int(uint64_t n) {
    int r = 0;
    while (n > 1) { n >>= 1; r++; }
    return r;
}

/* Toma un slot del pool: reusa uno reciclado si hay, o avanza nodeCount. */
static int createNode(uint8_t order, uint64_t offset, int parentIdx) {
    int idx;

    if (firstFreeNode != -1) {
        idx = firstFreeNode;
        firstFreeNode = nodes[idx].leftChild;
    } else {
        if (nodeCount >= MAX_NODES) return -1;
        idx = nodeCount++;
    }

    nodes[idx].isFree     = 1;
    nodes[idx].order      = order;
    nodes[idx].offset     = offset;
    nodes[idx].leftChild  = -1;
    nodes[idx].rightChild = -1;
    nodes[idx].parent     = parentIdx;
    return idx;
}

/* Divide un nodo en dos buddies (cada uno con orden = order-1). */
static int splitNode(int nodeIdx) {
    if (nodeIdx < 0 || nodeIdx >= nodeCount) return -1;
    if (nodes[nodeIdx].order <= MIN_ORDER)   return -1;
    if (nodes[nodeIdx].leftChild != -1)      return 0;   /* ya dividido */

    uint8_t  childOrder = nodes[nodeIdx].order - 1;
    uint64_t childSize  = 1ULL << childOrder;
    uint64_t offset     = nodes[nodeIdx].offset;

    int leftIdx = createNode(childOrder, offset, nodeIdx);
    if (leftIdx < 0) return -1;

    int rightIdx = createNode(childOrder, offset + childSize, nodeIdx);
    if (rightIdx < 0) {
        /* Reciclar el hijo izquierdo si falla el derecho (evita leak de slot). */
        nodes[leftIdx].parent    = -1;
        nodes[leftIdx].leftChild = firstFreeNode;
        firstFreeNode            = leftIdx;
        return -1;
    }

    nodes[nodeIdx].leftChild  = leftIdx;
    nodes[nodeIdx].rightChild = rightIdx;
    return 0;
}

void mem_init(void *startInclusive, uint64_t size) {
    minAddress    = (uint8_t *) startInclusive;
    totalSize     = size;
    nodeCount     = 0;
    firstFreeNode = -1;

    int capacity = log2Int(size);

    /* Mayor orden cuyo bloque entero entra en `size`. */
    maxOrder = MIN_ORDER;
    while ((1ULL << (maxOrder + 1)) <= size && maxOrder < (uint64_t) capacity)
        maxOrder++;

    rootIndex = createNode((uint8_t) maxOrder, 0, -1);
}

/* Orden mínimo que cubre `size` bytes. */
static uint64_t getOrder(uint64_t size) {
    uint64_t order = MIN_ORDER;
    while ((1ULL << order) < size && order < maxOrder) order++;
    return order;
}

/* Recalcula `isFree` del padre en función de sus hijos. */
static void updateNodeStatus(int nodeIdx) {
    if (nodeIdx < 0 || nodeIdx >= nodeCount) return;
    if (nodes[nodeIdx].leftChild == -1)      return;

    int leftIdx  = nodes[nodeIdx].leftChild;
    int rightIdx = nodes[nodeIdx].rightChild;
    nodes[nodeIdx].isFree = nodes[leftIdx].isFree && nodes[rightIdx].isFree;
}

/* Caso "tamaño justo": el nodo es del orden buscado y es hoja libre → ocuparlo. */
static int tryAllocateExactSize(int nodeIdx, uint8_t requiredOrder) {
    BuddyNode *node = &nodes[nodeIdx];
    if (node->order == requiredOrder && node->leftChild == -1) {
        if (!node->isFree) return -1;
        node->isFree = 0;
        return nodeIdx;
    }
    return -1;
}

static int findAndAllocate(int nodeIdx, uint8_t requiredOrder);

/* Caso "necesito partir": el nodo es hoja pero más grande de lo necesario. */
static int trySplitAndAllocate(int nodeIdx, uint8_t requiredOrder) {
    BuddyNode *node = &nodes[nodeIdx];
    if (!node->isFree)             return -1;
    if (splitNode(nodeIdx) < 0)    return -1;

    int result = findAndAllocate(node->leftChild, requiredOrder);
    if (result != -1) { updateNodeStatus(nodeIdx); return result; }

    result = findAndAllocate(node->rightChild, requiredOrder);
    if (result != -1) updateNodeStatus(nodeIdx);
    return result;
}

/* Busca recursivamente un bloque libre del orden pedido en el subárbol de nodeIdx. */
static int findAndAllocate(int nodeIdx, uint8_t requiredOrder) {
    if (nodeIdx < 0 || nodeIdx >= nodeCount) return -1;

    BuddyNode *node = &nodes[nodeIdx];

    if (node->order < requiredOrder) return -1;

    int exactResult = tryAllocateExactSize(nodeIdx, requiredOrder);
    if (exactResult != -1) return exactResult;

    if (node->leftChild == -1)
        return trySplitAndAllocate(nodeIdx, requiredOrder);

    int result = findAndAllocate(node->leftChild, requiredOrder);
    if (result != -1) { updateNodeStatus(nodeIdx); return result; }

    result = findAndAllocate(node->rightChild, requiredOrder);
    if (result != -1) updateNodeStatus(nodeIdx);
    return result;
}

/* Localiza el nodo asignado en `address`: scan lineal por offset coincidente. */
static int findNodeByAddress(uint8_t *address) {
    if (address < minAddress || address >= minAddress + totalSize) return -1;
    uint64_t offset = (uint64_t) (address - minAddress);

    for (int i = 0; i < nodeCount; i++) {
        if (nodes[i].offset == offset && !nodes[i].isFree && nodes[i].leftChild == -1)
            return i;
    }
    return -1;
}

/* Devuelve dos slots de nodos al pool reusable. */
static void recycleChildNodes(int leftIdx, int rightIdx) {
    nodes[leftIdx].parent     = -1;
    nodes[leftIdx].leftChild  = firstFreeNode;
    firstFreeNode             = leftIdx;

    nodes[rightIdx].parent    = -1;
    nodes[rightIdx].leftChild = firstFreeNode;
    firstFreeNode             = rightIdx;
}

static void tryMerge(int nodeIdx);

/* Fusión si ambos buddies están libres y son hojas (sin subdivisiones). */
static void mergeBuddies(int parentIdx, int leftIdx, int rightIdx) {
    if (nodes[leftIdx].isFree && nodes[rightIdx].isFree &&
        nodes[leftIdx].leftChild == -1 && nodes[rightIdx].leftChild == -1) {

        BuddyNode *parent = &nodes[parentIdx];
        parent->isFree = 1;
        recycleChildNodes(leftIdx, rightIdx);
        parent->leftChild  = -1;
        parent->rightChild = -1;

        tryMerge(parentIdx);   /* propagar fusión hacia arriba */
    }
}

static void tryMerge(int nodeIdx) {
    if (nodeIdx < 0 || nodeIdx >= nodeCount) return;

    int parentIdx = nodes[nodeIdx].parent;
    if (parentIdx < 0) return;   /* raíz o nodo reciclado */

    BuddyNode *parent = &nodes[parentIdx];
    if (parent->leftChild >= 0 && parent->rightChild >= 0)
        mergeBuddies(parentIdx, parent->leftChild, parent->rightChild);
}

void *mem_alloc(uint64_t bytes) {
    if (bytes == 0) return (void *) 0;

    uint8_t requiredOrder = (uint8_t) getOrder(bytes);

    int nodeIdx = findAndAllocate(rootIndex, requiredOrder);
    if (nodeIdx < 0) return (void *) 0;

    return (void *) (minAddress + nodes[nodeIdx].offset);
}

void mem_free(void *chunkStart) {
    if (chunkStart == (void *) 0) return;

    int nodeIdx = findNodeByAddress((uint8_t *) chunkStart);
    if (nodeIdx < 0) return;

    nodes[nodeIdx].isFree = 1;

    int parentIdx = nodes[nodeIdx].parent;
    if (parentIdx >= 0) updateNodeStatus(parentIdx);

    tryMerge(nodeIdx);
}

/* Cuenta nodos hoja (bloques asignados y libres) recorriendo el árbol. */
static void countLeafNodes(int nodeIdx,
                           uint64_t *allocatedCount, uint64_t *allocatedMemory,
                           uint64_t *freeCount,      uint64_t *freeMemory) {
    if (nodeIdx < 0 || nodeIdx >= nodeCount) return;

    BuddyNode *node = &nodes[nodeIdx];

    /* Saltar nodos huérfanos de fusiones previas (siguen viviendo en el array). */
    if (node->parent == -1 && nodeIdx != rootIndex) return;

    if (node->leftChild == -1) {
        uint64_t blockSize = 1ULL << node->order;
        if (node->isFree) { (*freeCount)++;      (*freeMemory)      += blockSize; }
        else              { (*allocatedCount)++; (*allocatedMemory) += blockSize; }
        return;
    }

    countLeafNodes(node->leftChild,  allocatedCount, allocatedMemory, freeCount, freeMemory);
    countLeafNodes(node->rightChild, allocatedCount, allocatedMemory, freeCount, freeMemory);
}

void mem_status(MemoryInfo *info) {
    if (info == 0) return;

    info->total = totalSize;

    uint64_t allocatedBlocks = 0;
    uint64_t allocatedMemory = 0;
    uint64_t freeBlocks      = 0;
    uint64_t freeMemory      = 0;
    countLeafNodes(rootIndex, &allocatedBlocks, &allocatedMemory, &freeBlocks, &freeMemory);

    info->occupied  = allocatedMemory;
    info->free      = freeMemory;
    info->fragments = freeBlocks;
}

#endif /* USE_BUDDY */
