#include "common.h"
#include "controller.h"
#include "shared_memory.h"
#include "ipc_utils.h"
#include "logging.h"
#include "miner.h"
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

// Configuration variables
int NUM_MINERS;
int POOL_SIZE;
int TRANSACTIONS_PER_BLOCK;
int BLOCKCHAIN_BLOCKS;

// Shared memory IDs
SharedMemoryIDs shm_ids;

void handle_sigint(int sig) {
    log_message("INFO: SIGINT received. Shutting down system...");
    stop_miner_threads();  // Stop all miner threads
    
    // Clean up shared memory and IPC resources
    destroy_shared_memory(shm_ids.transaction_pool_id);
    destroy_shared_memory(shm_ids.blockchain_ledger_id);
    cleanup_ipc();
    
    log_message("INFO: System shutdown complete.");
    exit(EXIT_SUCCESS);
}

void parse_config(const char* config_file) {
    FILE* file = fopen(config_file, "r");
    if (!file) {
        perror("Failed to open configuration file");
        exit(EXIT_FAILURE);
    }

    // Read configuration values in order
    if (fscanf(file, "%d", &NUM_MINERS) != 1 ||
        fscanf(file, "%d", &POOL_SIZE) != 1 ||
        fscanf(file, "%d", &TRANSACTIONS_PER_BLOCK) != 1 ||
        fscanf(file, "%d", &BLOCKCHAIN_BLOCKS) != 1) {
        perror("Failed to parse configuration file");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    fclose(file);

    log_message("CONFIG: NUM_MINERS=%d", NUM_MINERS);
    log_message("CONFIG: POOL_SIZE=%d", POOL_SIZE);
    log_message("CONFIG: TRANSACTIONS_PER_BLOCK=%d", TRANSACTIONS_PER_BLOCK);
    log_message("CONFIG: BLOCKCHAIN_BLOCKS=%d", BLOCKCHAIN_BLOCKS);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <config_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    log_message("INFO: Controller process started.");

    // Parse configuration
    parse_config(argv[1]);

    // Initialize shared memory
    shm_ids.transaction_pool_id = create_shared_memory(sizeof(Transaction) * POOL_SIZE);
    shm_ids.blockchain_ledger_id = create_shared_memory(sizeof(Block) * BLOCKCHAIN_BLOCKS);

    // Initialize IPC
    initialize_ipc();

    // Initialize and start miner threads
    init_miner(NUM_MINERS, POOL_SIZE, TRANSACTIONS_PER_BLOCK);
    start_miner_threads();
    log_message("INFO: Started %d miner threads", NUM_MINERS);

    // Set up signal handler
    signal(SIGINT, handle_sigint);

    // Main controller loop
    while (1) {
        // Add periodic checks or monitoring here
        sleep(1);
    }

    return 0;
}