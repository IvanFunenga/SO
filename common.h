#ifndef STRUCTS_H
#define STRUCTS_H

#include <time.h>
#include <stdio.h>   // fopen, fscanf, fclose
#include <stdlib.h>  // exit
#include <string.h>
#include "logging.h" // log_message(...

// SHM Names
#define TX_POOL_SHM "/tx_pool_shm"
#define BLOCKCHAIN_SHM "/blockchain_shm"
#define VALIDATOR_FIFO "/tmp/validator_fifo"

#define TX_ID_LEN 64
#define TXB_ID_LEN 64
#define HASH_SIZE 65  // SHA256_DIGEST_LENGTH * 2 + 1
 
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
  char txb_id[TXB_ID_LEN];              // Unique block ID (e.g., ThreadID + #)
  char previous_block_hash[HASH_SIZE];  // Hash of the previous block
  time_t timestamp;                     // Time when block was created
  Transaction* transactions;  // Array de transações
  unsigned int nonce;                   // PoW solution
} TransactionBlock;


typedef struct {
    void *ptr;
    int fd;
} SharedMemory;

typedef struct {
    char current_block_hash[HASH_SIZE];
    int pool_size; 
    Transaction transactions_pending_set[]; // Flexible array 
} TransactionPool;

Config global_config;
extern size_t transactions_per_block;
extern int tx_pool_fd;         // Declare as extern
extern TransactionPool* tx_pool_ptr;      // Declare as extern

// Function declaration
void load_config(const char *filename, Config *config);
int open_fifo(const char* fifo_path, int mode);
void close_fifo(int fifo_fd, const char* fifo_path);
void open_tx_pool_memory();
//static inline size_t get_transaction_block_size();


#endif