/*
 * Queue genérica (FIFO)
 */
#include <queue.h>
#include <memoryManager.h>

typedef struct node {
    void        *data;
    struct node *tail;
} Node;

typedef Node *List;

typedef struct queueCDT {
    List first;
    List last;
    int  count;
} queueCDT;

static void removeNodeFromQueue(Queue queue, List nodeToRemove, List previousNode, List nextNode) {
    if (previousNode == 0)        queue->first = nextNode;
    else                          previousNode->tail = nextNode;
    if (nodeToRemove == queue->last) queue->last = previousNode;
    mem_free(nodeToRemove);
    queue->count--;
}

Queue newQueue(void) {
    Queue queue = mem_alloc(sizeof(queueCDT));
    if (!queue) return 0;
    queue->first = queue->last = 0;
    queue->count = 0;
    return queue;
}

int enqueue(Queue queue, void *elementToAdd) {
    List newNode = mem_alloc(sizeof(Node));
    if (!newNode) return 0;
    newNode->data = elementToAdd;
    newNode->tail = 0;
    if (isQueueEmpty(queue)) queue->first = queue->last = newNode;
    else { queue->last->tail = newNode; queue->last = newNode; }
    queue->count++;
    return 1;
}

void *pollQueue(Queue queue) {
    if (isQueueEmpty(queue)) return 0;
    void *removedElement = queue->first->data;
    List  nextNode       = queue->first->tail;
    mem_free(queue->first);
    queue->first = nextNode;
    queue->count--;
    if (queue->count == 0) queue->last = 0;
    return removedElement;
}

int getQueueSize(Queue queue) { return queue ? queue->count : 0; }
int isQueueEmpty(Queue queue) { return getQueueSize(queue) == 0; }

void freeQueue(Queue queue) {
    if (!queue) return;
    List currentNode = queue->first;
    while (currentNode) {
        List nextNode = currentNode->tail;
        mem_free(currentNode);
        currentNode = nextNode;
    }
    mem_free(queue);
}

int removeFromQueue(Queue queue, void *elementToRemove, QueueCompareFunction compare) {
    if (!queue || !compare) return 0;
    List previousNode = 0;
    List currentNode  = queue->first;
    int  removedCount = 0;
    while (currentNode) {
        List nextNode = currentNode->tail;
        if (compare(elementToRemove, currentNode->data) == 0) {
            removeNodeFromQueue(queue, currentNode, previousNode, nextNode);
            removedCount++;
        } else {
            previousNode = currentNode;
        }
        currentNode = nextNode;
    }
    if (isQueueEmpty(queue)) queue->first = queue->last = 0;
    return removedCount;
}
