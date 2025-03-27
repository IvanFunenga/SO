#ifndef MINER_H
#define MINER_H

typedef struct {
    int id;
    // Add other miner-specific data as needed
} MinerThreadArgs;

void init_miner(int num_miners, int pool_size, int transactions_per_block);
void start_miner_threads();
void stop_miner_threads();

#endif