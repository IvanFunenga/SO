#include "common.h"
#include "logging.h"
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

int tx_pool_fd = -1;           // Actual definition
TransactionPool* tx_pool_ptr = NULL;      

void load_config(const char *filename, Config *config) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        log_message("ERROR: Failed to open configuration file %s", filename);
        exit(EXIT_FAILURE);
    }

    if (fscanf(file, "%d", &config->num_miners) != 1 ||
        fscanf(file, "%d", &config->pool_size) != 1 ||
        fscanf(file, "%d", &config->transactions_per_block) != 1 ||
        fscanf(file, "%d", &config->blockchain_blocks) != 1) {
        
        log_message("ERROR: Incorrect format in configuration file");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    fclose(file);
    
    if (config->num_miners <= 0 || config->pool_size <= 0 || 
        config->transactions_per_block <= 0 || config->blockchain_blocks <= 0) {
        log_message("ERROR: Invalid configuration values (must be positive)");
        exit(EXIT_FAILURE);
    }

    log_message("CONFIG: NUM_MINERS = %d", config->num_miners);
    log_message("CONFIG: POOL_SIZE = %d", config->pool_size);
    log_message("CONFIG: TRANSACTIONS_PER_BLOCK = %d", config->transactions_per_block);
    log_message("CONFIG: BLOCKCHAIN_BLOCKS = %d", config->blockchain_blocks);
} 

// Função para abrir a memória compartilhada da tx_pool (sem criá-la)
void open_tx_pool_memory() {
    // Get the size from the config (no fstat needed)
    size_t total_size = sizeof(TransactionPool) + 
                        sizeof(Transaction) * global_config.pool_size;

    // Open existing shared memory (no creation)
    tx_pool_fd = shm_open(TX_POOL_SHM, O_RDWR, 0666);
    if (tx_pool_fd == -1) {
        log_message("ERROR: shm_open failed for %s", TX_POOL_SHM);
        exit(EXIT_FAILURE);
    }

    // Map the memory using the precomputed size
    tx_pool_ptr = mmap(NULL, total_size, PROT_READ | PROT_WRITE, 
                       MAP_SHARED, tx_pool_fd, 0);
    if (tx_pool_ptr == MAP_FAILED) {
        log_message("ERROR: mmap failed for %s", TX_POOL_SHM);
        exit(EXIT_FAILURE);
    }

    log_message("SHM: tx_pool opened and mapped (size based on config)");
}

static inline size_t get_transaction_block_size() {
  if (transactions_per_block == 0) {
    perror("Must set the 'transactions_per_block' variable before using!\n");
    exit(-1);
  }
  return sizeof(TransactionBlock) +
         transactions_per_block * sizeof(Transaction);
}