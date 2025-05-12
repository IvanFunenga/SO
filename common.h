#ifndef STRUCTS_H
#define STRUCTS_H

#include <time.h>
#include <stdio.h>   // fopen, fscanf, fclose
#include <stdlib.h>  // exit
#include "logging.h" // log_message(...

// SHM Names
#define TX_POOL_SHM "/tx_pool_shm"
#define BLOCKCHAIN_SHM "/blockchain_shm"
#define VALIDATOR_FIFO "/tmp/validator_fifo"

// Configuração global
typedef struct {
    int num_miners;
    int pool_size;
    int transactions_per_block;
    int blockchain_blocks;
} Config;

// Transação na transaction pool
typedef struct {
    int id;
    int reward;
    int sender_id;
    int receiver_id;
    int value;
    time_t timestamp;
    int age;
    int empty; // 1 = vazio, 0 = ocupado
} Transaction;

typedef struct {
    int id;
    int miner_id;
    int previous_block_id;
    int nonce;
    int num_transactions;
    Transaction transactions[]; // ou dinâmico se suportares
} Block;

typedef struct {
    void* ptr;
    int fd;
} SharedMemory;

typedef struct {
    int current_block_id;
    Transaction transactions_pending_set[]; // tamanho definido dinamicamente
} TransactionPool;

// Function declaration
void load_config(const char *filename, Config *config);

#endif
