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

SharedMemory create_shared_memory(const char* name, size_t size) {
    SharedMemory shm;
    shm.fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    if (shm.fd == -1) {
        log_message("ERROR: shm_open failed for %s", name);
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm.fd, size) == -1) {
        log_message("ERROR: ftruncate failed for %s", name);
        exit(EXIT_FAILURE);
    }

    shm.ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm.fd, 0);
    if (shm.ptr == MAP_FAILED) {
        log_message("ERROR: mmap failed for %s", name);
        exit(EXIT_FAILURE);
    }

    log_message("SHM: %s created and mapped (%zu bytes)", name, size);
    return shm;
}

// Função para abrir a memória compartilhada da tx_pool (sem criá-la)
void open_tx_pool_memory() {
    // Usando o tamanho que você já conhece ou que foi configurado
    size_t size = sizeof(TransactionPool) + sizeof(Transaction) * global_config.pool_size;
    SharedMemory shm = create_shared_memory(TX_POOL_SHM, size);

    tx_pool_ptr = shm.ptr;
    tx_pool_fd = shm.fd;  // Inicializa as variáveis globais

    log_message("SHM: tx_pool aberta e mapeada com sucesso");
}

void create_tx_pool_memory(const Config* config) {
    size_t size = sizeof(TransactionPool) + sizeof(Transaction) * config->pool_size;
    SharedMemory shm = create_shared_memory(TX_POOL_SHM, size);

    tx_pool_ptr = shm.ptr;
    tx_pool_fd = shm.fd;  // Inicializa as variáveis globais

    // Inicializando o TransactionPool
    TransactionPool* pool = (TransactionPool*)tx_pool_ptr;
    
    // Inicializando o hash do bloco (exemplo: um valor de hash inicial)
    memset(pool->current_block_hash, '0', HASH_SIZE - 1);  // Um hash de inicialização (zeros) para o primeiro bloco
    pool->current_block_hash[HASH_SIZE - 1] = '\0';  // Certificando-se de que a string seja terminada corretamente

    // Inicializando as transações
    for (int i = 0; i < config->pool_size; i++) {
        pool->transactions_pending_set[i].empty = 1;
    }
    
    log_message("SHM: tx_pool inicializada com todos os slots vazios e hash de bloco inicializado");
}

static inline size_t get_transaction_block_size() {
  if (transactions_per_block == 0) {
    perror("Must set the 'transactions_per_block' variable before using!\n");
    exit(-1);
  }
  return sizeof(TransactionBlock) +
         transactions_per_block * sizeof(Transaction);
}