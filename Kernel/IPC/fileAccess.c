/* fileAccess.c — Implementación del wrapper Pipe + flags + refcount. */
#include <fileAccess.h>
#include <memoryManager.h>

typedef struct fileAccessCDT {
    Pipe    pipe;
    uint8_t flags;
    int     refCount;
} fileAccessCDT;

FileAccess newFileAccess(Pipe pipe, uint8_t flags) {
    FileAccess fa = (FileAccess) mem_alloc(sizeof(fileAccessCDT));
    if (!fa) return 0;
    fa->pipe     = pipe;
    fa->flags    = flags;
    fa->refCount = 0;
    if (pipe) {
        addPipeRef(pipe);
        if (flags & FILE_WRITE) addPipeWriter(pipe);
    }
    return fa;
}

void addFileRef(FileAccess fa)    { if (fa) fa->refCount++; }

void removeFileRef(FileAccess fa) {
    if (!fa) return;
    fa->refCount--;
    if (fa->refCount <= 0) freeFileAccess(fa);
}

static int hasFlag(FileAccess fa, uint8_t f) { return fa && (fa->flags & f); }

int isReadable   (FileAccess fa) { return hasFlag(fa, FILE_READ); }
int isWritable   (FileAccess fa) { return hasFlag(fa, FILE_WRITE); }
int isNonblocking(FileAccess fa) { return hasFlag(fa, FILE_NONBLOCK); }

int64_t fileRead(FileAccess fa, char *buf, uint64_t count) {
    if (!isReadable(fa)) return -1;
    return pipeRead(fa->pipe, buf, count, isNonblocking(fa));
}

int64_t fileWrite(FileAccess fa, const char *buf, uint64_t count) {
    if (!isWritable(fa)) return -1;
    return pipeWrite(fa->pipe, buf, count, isNonblocking(fa));
}

void setFileFlags   (FileAccess fa, uint8_t flags) { if (fa) fa->flags  = flags; }
void addFileFlags   (FileAccess fa, uint8_t flags) { if (fa) fa->flags |= flags; }
void removeFileFlags(FileAccess fa, uint8_t flags) { if (fa) fa->flags &= ~flags; }

FileAccess cloneFile(FileAccess fa) {
    if (!fa) return 0;
    return newFileAccess(fa->pipe, fa->flags);
}

Pipe getFilePipe(FileAccess fa) { return fa ? fa->pipe : 0; }

void forceWakeOnPipe(FileAccess fa, int count) {
    if (fa) forceReadersWakeUp(fa->pipe, count);
}

void freeFileAccess(FileAccess fa) {
    if (!fa) return;
    if (fa->pipe) {
        if (fa->flags & FILE_WRITE) removePipeWriter(fa->pipe);
        removePipeRef(fa->pipe);
    }
    mem_free(fa);
}
