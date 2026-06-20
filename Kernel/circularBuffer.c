/* circularBuffer.c — Implementación del FIFO circular usado como cuerpo de pipe. */
#include <circularBuffer.h>
#include <memoryManager.h>

typedef struct circularBufferCDT {
    char    *data;
    uint64_t maxSize;
    uint64_t readIdx;
    uint64_t writeIdx;
    uint64_t count;
} circularBufferCDT;

CircularBuffer newBuffer(uint64_t maxSize) {
    if (maxSize == 0) return 0;
    CircularBuffer cb = (CircularBuffer) mem_alloc(sizeof(circularBufferCDT));
    
    if (!cb) return 0;
    
    cb->data = (char *) mem_alloc(maxSize);
    if (!cb->data){
        mem_free(cb); return 0;
    }
    
    cb->maxSize  = maxSize;
    cb->readIdx  = 0;
    cb->writeIdx = 0;
    cb->count    = 0;
    return cb;
}

int bufferIsFull(CircularBuffer cb)  { return cb && cb->count == cb->maxSize; }
int bufferHasData(CircularBuffer cb) { return cb && cb->count > 0; }

int bufferWrite(CircularBuffer cb, char byteToWrite) {
    if (!cb || bufferIsFull(cb)) 
        return 0;

    cb->data[cb->writeIdx] = byteToWrite;
    cb->writeIdx = (cb->writeIdx + 1) % cb->maxSize;
    cb->count++;
    return 1;
}

char bufferRead(CircularBuffer cb) {
    if (!cb || !bufferHasData(cb)) 
        return EOF;

    char readByte = cb->data[cb->readIdx];
    cb->readIdx = (cb->readIdx + 1) % cb->maxSize;
    cb->count--;
    return readByte;
}

void freeBuffer(CircularBuffer cb) {
    if (!cb) return;
    mem_free(cb->data);
    mem_free(cb);
}
