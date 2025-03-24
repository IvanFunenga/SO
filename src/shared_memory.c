#include "shared_memory.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>

int create_shared_memory(size_t size) {
    int shm_id = shmget(IPC_PRIVATE, size, 0666 | IPC_CREAT);
    if (shm_id == -1) {
        perror("shmget failed");
        exit(EXIT_FAILURE);
    }
    return shm_id;
}

void* attach_shared_memory(int shm_id) {
    void* shm_addr = shmat(shm_id, NULL, 0);
    if (shm_addr == (void*)-1) {
        perror("shmat failed");
        exit(EXIT_FAILURE);
    }
    return shm_addr;
}

void detach_shared_memory(void* shm_addr) {
    if (shmdt(shm_addr) == -1) {
        perror("shmdt failed");
        exit(EXIT_FAILURE);
    }
}

void destroy_shared_memory(int shm_id) {
    if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
        perror("shmctl failed");
        exit(EXIT_FAILURE);
    }
}