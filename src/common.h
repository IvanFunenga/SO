#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <semaphore.h>
#include <fcntl.h>

extern int TRANSACTIONS_PER_BLOCK;

// Transaction structure
typedef struct {
    int transaction_id;
    int reward;
    int sender_id;
    int receiver_id;
    int value;
    time_t timestamp;
} Transaction;

// Block structure
typedef struct {
    int block_id;
    Transaction* transactions;  // Pointer instead of array
    int nonce;
    int previous_block_id;
} Block;

// Shared memory IDs
typedef struct {
    int transaction_pool_id;
    int blockchain_ledger_id;
} SharedMemoryIDs;

#endif // COMMON_H