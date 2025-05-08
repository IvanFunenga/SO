#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include "logging.h"
#include "common.h"
#include "miner.h"

#define TX_POOL_SHM "/tx_pool_shm"
#define BLOCKCHAIN_SHM "/blockchain_shm"

Config global_config;

int tx_pool_fd = -1, blockchain_fd = -1;
void* tx_pool_ptr = NULL;
void* blockchain_ptr = NULL;

volatile sig_atomic_t shutdown_requested = 0;
static pid_t miner_pid = -1;
static pid_t validator_pid = -1;
static pid_t statistics_pid = -1;

typedef void (*ProcessFunctionWithArgs)(void *);

// Signal handler for SIGINT
void handle_sigint(int sig) {
    (void)sig;
    shutdown_requested = 1;
    log_message("INFO: SIGINT signal captured. Shutting down...");
    if (miner_pid > 0) {
        kill(miner_pid, SIGINT);
    }
}

void print_tx_pool(Transaction* pool, int pool_size) {
    printf("\n=== Conteúdo da Transaction Pool ===\n");
    for (int i = 0; i < pool_size; i++) {
        if (pool[i].empty) {
            printf("[Slot %d] VAZIO\n", i);
        } else {
            printf("[Slot %d] ID=%d | From=%d | To=%d | Value=%d | Reward=%d\n",
                   i,
                   pool[i].id,
                   pool[i].sender_id,
                   pool[i].receiver_id,
                   pool[i].value,
                   pool[i].reward);
        }
    }
    printf("=====================================\n");
}


// Create and map shared memory
void init_shared_memory(const Config* config) {
    size_t tx_pool_size = sizeof(Transaction) * config->pool_size;
    size_t blockchain_size = sizeof(Block) * config->blockchain_blocks;

    // TX Pool
    tx_pool_fd = shm_open(TX_POOL_SHM, O_CREAT | O_RDWR, 0666);
    if (tx_pool_fd == -1) {
        log_message("ERROR: Failed to create shared memory for tx_pool");
        exit(EXIT_FAILURE);
    }
    if (ftruncate(tx_pool_fd, tx_pool_size) == -1) {
        log_message("ERROR: ftruncate failed for tx_pool");
        exit(EXIT_FAILURE);
    }
    tx_pool_ptr = mmap(NULL, tx_pool_size, PROT_READ | PROT_WRITE, MAP_SHARED, tx_pool_fd, 0);
    if (tx_pool_ptr == MAP_FAILED) {
        log_message("ERROR: mmap failed for tx_pool");
        exit(EXIT_FAILURE);
    }
    log_message("SHM: tx_pool created and mapped successfully (%zu bytes)", tx_pool_size);

    Transaction* pool = (Transaction*)tx_pool_ptr;
    for (int i = 0; i < config->pool_size; i++) {
        pool[i].empty = 1;
    }
    log_message("SHM: tx_pool inicializada com todos os slots vazios");

    // Blockchain
    blockchain_fd = shm_open(BLOCKCHAIN_SHM, O_CREAT | O_RDWR, 0666);
    if (blockchain_fd == -1) {
        log_message("ERROR: Failed to create shared memory for blockchain");
        exit(EXIT_FAILURE);
    }
    if (ftruncate(blockchain_fd, blockchain_size) == -1) {
        log_message("ERROR: ftruncate failed for blockchain");
        exit(EXIT_FAILURE);
    }
    blockchain_ptr = mmap(NULL, blockchain_size, PROT_READ | PROT_WRITE, MAP_SHARED, blockchain_fd, 0);
    if (blockchain_ptr == MAP_FAILED) {
        log_message("ERROR: mmap failed for blockchain");
        exit(EXIT_FAILURE);
    }
    log_message("SHM: blockchain created and mapped successfully (%zu bytes)", blockchain_size);
}

// Unmap and unlink shared memory
void cleanup_shared_memory() {
    if (tx_pool_ptr) {
        if (munmap(tx_pool_ptr, sizeof(Transaction) * global_config.pool_size) == 0) {
            log_message("SHM: tx_pool unmapped successfully");
        } else {
            log_message("ERROR: Failed to unmap tx_pool");
        }
    }

    if (blockchain_ptr) {
        if (munmap(blockchain_ptr, sizeof(Block)) == 0) {
            log_message("SHM: blockchain unmapped successfully");
        } else {
            log_message("ERROR: Failed to unmap blockchain");
        }
    }

    if (tx_pool_fd != -1) {
        if (close(tx_pool_fd) == 0) {
            log_message("SHM: tx_pool descriptor closed successfully");
        } else {
            log_message("ERROR: Failed to close tx_pool descriptor");
        }
    }

    if (blockchain_fd != -1) {
        if (close(blockchain_fd) == 0) {
            log_message("SHM: blockchain descriptor closed successfully");
        } else {
            log_message("ERROR: Failed to close blockchain descriptor");
        }
    }

    if (shm_unlink(TX_POOL_SHM) == 0) {
        log_message("SHM: tx_pool unlinked successfully");
    } else {
        log_message("ERROR: Failed to unlink tx_pool with shm_unlink");
    }

    if (shm_unlink(BLOCKCHAIN_SHM) == 0) {
        log_message("SHM: blockchain unlinked successfully");
    } else {
        log_message("ERROR: Failed to unlink blockchain with shm_unlink");
    }
}

// Wrapper to call the miner function
void run_miner_process_wrapper(void *arg) {
    int num_threads = *((int*)arg);
    run_miner_process(num_threads);
}

// Create process and call the given function
pid_t create_process(const char *name, ProcessFunctionWithArgs func, void *args) {
    pid_t pid = fork();

    if (pid < 0) {
        log_message("ERROR: Failed to create %s process", name);
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process
        if (func != NULL) {
            func(args);
        } else {
            log_message("WARNING: No function defined for %s process", name);
        }
        exit(EXIT_SUCCESS);
    }

    log_message("INFO: %s process created (PID: %d)", name, pid);
    return pid;
}

int main() {

    // Register SIGINT handler
    signal(SIGINT, handle_sigint);

    log_init("DEIChain_log.txt");
    load_config("config.cfg", &global_config);
    init_shared_memory(&global_config);

    miner_pid = create_process("Miner", run_miner_process_wrapper, &global_config.num_miners);
    validator_pid = create_process("Validator", NULL, NULL);
    statistics_pid = create_process("Statistics", NULL, NULL);

    // Main loop: wait for SIGINT
    while (!shutdown_requested) {
        print_tx_pool((Transaction*)tx_pool_ptr, global_config.pool_size);
        sleep(10);
        //pause(); // Can be replaced by useful logic
    }

    waitpid(miner_pid, NULL, 0);
    waitpid(validator_pid, NULL, 0);
    waitpid(statistics_pid, NULL, 0);

    cleanup_shared_memory();
    log_message("INFO: System shut down successfully");
    log_close();

    return 0;
}