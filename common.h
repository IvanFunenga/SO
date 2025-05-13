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

#define TX_ID_LEN 64
#define TXB_ID_LEN 64
#define HASH_SIZE 65  // SHA256_DIGEST_LENGTH * 2 + 1
// Configuração global
extern size_t transactions_per_block;

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

// Transaction Block structure
typedef struct {
  char txb_id[TXB_ID_LEN];              // Unique block ID (e.g., ThreadID + #)
  char previous_block_hash[HASH_SIZE];  // Hash of the previous block
  time_t timestamp;                     // Time when block was created
  Transaction transactions[50];            // buffer for transactions
  unsigned int nonce;                   // PoW solution
} TransactionBlock;

typedef struct {
    void *ptr;
    int fd;
} SharedMemory;

typedef struct {
    int current_block_id;
    Transaction transactions_pending_set[]; // tamanho definido dinamicamente
} TransactionPool;

// Function declaration
void load_config(const char *filename, Config *config);

static inline size_t get_transaction_block_size() {
  if (transactions_per_block == 0) {
    perror("Must set the 'transactions_per_block' variable before using!\n");
    exit(-1);
  }
  return sizeof(TransactionBlock) +
         transactions_per_block * sizeof(Transaction);
}
#endif