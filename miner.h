#ifndef MINER_H
#define MINER_H

typedef struct {
    int id;
    // Add other miner-specific data as needed
} MinerThreadArgs;

void run_miner_process(int num_threads);

#endif