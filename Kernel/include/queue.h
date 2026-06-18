/*
 * Queue genérica (cola FIFO).
 *
 * Implementación basada en lista enlazada. La usan el scheduler, los semáforos y los procesos.
 */
#ifndef QUEUE_H
#define QUEUE_H

typedef struct queueCDT *Queue;
typedef int (*QueueCompareFunction)(void *, void *);

Queue newQueue(void);
int   enqueue(Queue queue, void *elementToAdd);
void *pollQueue(Queue queue);
int   getQueueSize(Queue queue);
int   isQueueEmpty(Queue queue);
int   removeFromQueue(Queue queue, void *elementToRemove, QueueCompareFunction compare);
void  freeQueue(Queue queue);

#endif
