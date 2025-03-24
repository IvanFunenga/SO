#ifndef MINER_H
#define MINER_H

#include "common.h"

typedef struct {
    int id;
    // Add other miner-specific data as needed
} MinerThreadArgs;

// Function to initialize miner module
void init_miner(int num_miners, int pool_size, int transactions_per_block);

// Function to start all miner threads
void start_miner_threads();

// Function to stop all miner threads
void stop_miner_threads();

#endif