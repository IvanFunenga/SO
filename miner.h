#ifndef MINER_H
#define MINER_H

typedef struct {
    int id;        // ID da thread
    int fifo_fd;   // Descritor de arquivo do FIFO
} MinerThreadArgs;
void run_miner_process(int num_threads);

#endif