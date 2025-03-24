#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include "common.h"

int create_shared_memory(size_t size);
void* attach_shared_memory(int shm_id);
void detach_shared_memory(void* shm_addr);
void destroy_shared_memory(int shm_id);

#endif // SHARED_MEMORY_H