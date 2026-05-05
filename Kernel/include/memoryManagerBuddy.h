

void initializeMemory(uint8_t* startAddress, uint64_t totalMemory);
void* allocateMemory(uint64_t size);
void freeMemory(void* address);
