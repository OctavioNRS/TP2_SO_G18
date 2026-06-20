#include <pipe.h>
#include <circularBuffer.h>
#include <semaphore.h>
#include <memoryManager.h>
#include <process.h>   

typedef struct pipeCDT {
    CircularBuffer buffer;
    Semaphore      dataAvailable;
    Semaphore      freeSpaces;
    Semaphore      readMutex;
    Semaphore      writeMutex;
    int            refCount;
    int            writerCount;
} pipeCDT;

Pipe newPipe(void) {
    Pipe p = (Pipe) mem_alloc(sizeof(pipeCDT));
    if (!p) return 0;
    p->buffer        = newBuffer(PIPE_BUF_SIZE);
    p->dataAvailable = newSemaphore(0);
    p->freeSpaces    = newSemaphore(PIPE_BUF_SIZE);
    p->readMutex     = newSemaphore(1);
    p->writeMutex    = newSemaphore(1);
    if (!p->buffer || !p->dataAvailable || !p->freeSpaces ||
        !p->readMutex || !p->writeMutex) {
        freePipe(p);
        return 0;
    }
    p->refCount    = 0;
    p->writerCount = 0;
    return p;
}

void addPipeRef(Pipe p)    { if (p) p->refCount++; }
void addPipeWriter(Pipe p) { if (p) p->writerCount++; }

void removePipeRef(Pipe p) {
    if (!p) return;
    p->refCount--;
    if (p->refCount <= 0) freePipe(p);
}

void removePipeWriter(Pipe p) {
    if (!p) return;
    p->writerCount--;
    if (p->writerCount <= 0) {
        /* Último writer cerró: postear el semáforo de datos para que los
         * readers bloqueados se despierten y detecten el EOF. No sabemos cuántos
         * están bloqueados; posteamos MAX_PROCESSES veces. */
        forceReadersWakeUp(p, MAX_PROCESSES);
    }
}

int pipeHasData(Pipe p) { return p && bufferHasData(p->buffer); }
int pipeIsFull(Pipe p)  { return p && bufferIsFull(p->buffer); }

static int64_t drainBuffer(Pipe p, char *buf, uint64_t count) {
    int64_t n = 0;
    while ((uint64_t) n < count && pipeHasData(p)) {
        buf[n++] = bufferRead(p->buffer);
        semPost(p->freeSpaces);
    }
    return n;
}

static int64_t pipeReadBlocking(Pipe p, char *buf, uint64_t count) {
    if (!pipeHasData(p) && p->writerCount <= 0) return 0;

    while (1) {
        semWait(p->dataAvailable);
        semWait(p->readMutex);

        if (pipeHasData(p)) {
            int64_t n = drainBuffer(p, buf, count);
            if (pipeHasData(p)) semPost(p->dataAvailable);
            semPost(p->readMutex);
            return n;
        }

        if (p->writerCount <= 0) {
            semPost(p->readMutex);
            return 0;
        }

        semPost(p->readMutex);
    }
}

static int64_t pipeReadNonblocking(Pipe p, char *buf, uint64_t count) {
    if (!pipeHasData(p)) return p->writerCount > 0 ? PIPE_WOULD_BLOCK : 0;
    semWait(p->readMutex);
    if (!pipeHasData(p)) {
        int hasWriters = p->writerCount > 0;
        semPost(p->readMutex);
        return hasWriters ? PIPE_WOULD_BLOCK : 0;
    }
    int64_t n = drainBuffer(p, buf, count);
    semPost(p->readMutex);
    return n;
}

int64_t pipeRead(Pipe p, char *buf, uint64_t count, int nonblocking) {
    if (!p || count == 0) return 0;
    return nonblocking ? pipeReadNonblocking(p, buf, count)
                       : pipeReadBlocking(p, buf, count);
}

int64_t pipeWrite(Pipe p, const char *buf, uint64_t count, int nonblocking) {
    if (!p) return -1;
    semWait(p->writeMutex);

    int64_t n = 0;
    while ((uint64_t) n < count) {
        if (nonblocking && pipeIsFull(p)) {
            semPost(p->writeMutex);
            return n > 0 ? n : PIPE_WOULD_BLOCK;
        }
        semWait(p->freeSpaces);

        int wasEmpty = !pipeHasData(p);
        if (bufferWrite(p->buffer, buf[n])) {
            n++;
            if (wasEmpty) semPost(p->dataAvailable);
        }
    }

    semPost(p->writeMutex);
    return n;
}

void forceReadersWakeUp(Pipe p, int count) {
    if (!p) return;
    for (int i = 0; i < count; i++) semPost(p->dataAvailable);
}

void freePipe(Pipe p) {
    if (!p) return;
    if (p->buffer)        freeBuffer(p->buffer);
    if (p->dataAvailable) freeSemaphore(p->dataAvailable);
    if (p->freeSpaces)    freeSemaphore(p->freeSpaces);
    if (p->readMutex)     freeSemaphore(p->readMutex);
    if (p->writeMutex)    freeSemaphore(p->writeMutex);
    mem_free(p);
}
