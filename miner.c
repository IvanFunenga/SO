#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <fcntl.h>     
#include <unistd.h>
#include "miner.h"
#include "logging.h"
#include "common.h"
#include <semaphore.h>
#include <sys/mman.h>  // For mmap and shm_open
#include <sys/stat.h>   // For mode constants (0666)

static int num_miners;
static pthread_t* miner_threads = NULL;
static MinerThreadArgs* thread_args = NULL;
static volatile sig_atomic_t running = 1;

// Define FIFO path (should match in both miner and validator)
#define VALIDATOR_FIFO "/tmp/validator_fifo"
// Signal handler for SIGINT in the miner process
void handle_sigint_miner(int sig) {
    (void)sig;
    running = 0;
    log_message("INFO: SIGINT received by miner process, stopping mining...");
}

void send_block_to_validator(TransactionBlock* b) {
    int fd = open(VALIDATOR_FIFO, O_WRONLY);
    if (fd == -1) {
        log_message("MINER: Failed to open FIFO for writing");
        return;
    }

    ssize_t bytes_written = write(fd, b, sizeof(TransactionBlock));
    if (bytes_written == sizeof(TransactionBlock)) {
        log_message("MINER: Block sent to Validator (ID=%d)", b->txb_id);
    } else {
        log_message("ERROR: Incomplete block write to Validator FIFO");
    }

    close(fd);
}
void* miner_thread_func(void *arg) {
    MinerThreadArgs* args = (MinerThreadArgs*)arg;

    // Open semaphores (should already be created elsewhere in the system)
    sem_t *sem_mutex = sem_open("/sem_mutex", 0);  // Open the mutex semaphore
    sem_t *sem_empty = sem_open("/sem_empty", 0);  // Open the empty slots semaphore
    sem_t *sem_full = sem_open("/sem_full", 0);    // Open the full slots semaphore

    if (sem_mutex == SEM_FAILED || sem_empty == SEM_FAILED || sem_full == SEM_FAILED) {
        log_message("ERROR: Failed to open semaphores.");
        return NULL;
    }

    // Open the shared memory for the transaction pool
    int shm_fd = shm_open(TX_POOL_SHM, O_RDWR, 0666);  // Open shared memory object
    if (shm_fd == -1) {
        perror("shm_open failed");
        return NULL;
    }

    // Map the shared memory to access it
    TransactionPool *tx_pool = mmap(NULL, sizeof(TransactionPool) + sizeof(Transaction) * 50, 
                                    PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (tx_pool == MAP_FAILED) {
        perror("mmap failed");
        return NULL;
    }

    log_message("INFO: Miner %d is working...", args->id);

    int max_transactions_to_print = 5;  // Limit to 5 transactions
    TransactionBlock block;  // Define the transaction block to store transactions
    int stored_count = 0;    // Counter for stored transactions

    // Open FIFO for writing to send the block to the validator
    int fifo_fd = open(VALIDATOR_FIFO, O_WRONLY);
    if (fifo_fd == -1) {
        log_message("ERROR: Failed to open FIFO for writing");
        return NULL;
    }
    log_message("INFO: Miner %d FIFO opened for writing", args->id);

    // Keep checking for transactions and collecting them until the block is full
    while (running) {
        log_message("INFO: Miner %d is checking for transactions...", args->id);

        // Reset stored transactions for each iteration
        stored_count = 0;
        
        sem_wait(sem_full);    // Wait for a transaction to be available
        sem_wait(sem_mutex);   // Lock the pool for safe access
        // Extract all transactions available from the transaction pool (up to max limit)
        for (int i = 0; i < 50 && stored_count < max_transactions_to_print; i++) {
            // Check if the transaction is not empty
            if (!tx_pool->transactions_pending_set[i].empty) {
                // Store the transaction in the block's transactions array
                block.transactions[stored_count] = tx_pool->transactions_pending_set[i];
                stored_count++;

                log_message("Stored Transaction ID: %d, Reward: %d, From: %d, To: %d, Value: %d, Age: %d",
                            block.transactions[stored_count - 1].id,
                            block.transactions[stored_count - 1].reward,
                            block.transactions[stored_count - 1].sender_id,
                            block.transactions[stored_count - 1].receiver_id,
                            block.transactions[stored_count - 1].value,
                            block.transactions[stored_count - 1].age);
            }

        }
        sem_post(sem_mutex);   // Release the lock after accessing the pool
        sem_post(sem_empty);   // Notify that a slot in the pool is now free
        // If the block is full, send it to the validator and break the loop
        if (stored_count == max_transactions_to_print) {
            // Send the block to the validator via FIFO
            ssize_t bytes_written = write(fifo_fd, &block, sizeof(TransactionBlock));

            if (bytes_written == sizeof(TransactionBlock)) {
                log_message("INFO: Miner %d sent block to validator with %d transactions", args->id, stored_count);
            } else {
                log_message("ERROR: Failed to send complete block to validator. Only %zd bytes written.", bytes_written);
            }
            break;  // Exit loop since we have sent the block
        } else {
            log_message("INFO: Miner %d printed %d transactions, waiting for more...", args->id, stored_count);
            sleep(2);  // Wait for 2 seconds before trying again
        }
    }

    close(fifo_fd);  // Close the FIFO
    log_message("INFO: Miner thread %d stopping", args->id);
    return NULL;
}

// Initializes miner threads and arguments
void init_miner(int nm, int ps, int tpb) {
    // ps and tpb are not used for now
    num_miners = nm;
    (void)ps;
    (void)tpb;

    // Allocate memory for thread handles and arguments
    miner_threads = malloc(num_miners * sizeof(pthread_t));
    thread_args = malloc(num_miners * sizeof(MinerThreadArgs));

    if (!miner_threads || !thread_args) {
        log_message("ERROR: Failed to allocate memory for miner threads");
        exit(EXIT_FAILURE);
    }
}

// Starts all miner threads
void start_miner_threads() {
    for (int i = 0; i < num_miners; i++) {
        thread_args[i].id = i;
        if (pthread_create(&miner_threads[i], NULL, miner_thread_func, &thread_args[i]) != 0) {
            log_message("ERROR: Failed to create miner thread %d", i);
            exit(EXIT_FAILURE);
        }
        log_message("INFO: Successfully created miner thread %d", i);
    }
    log_message("INFO: All threads were created!");
}

// Waits for all miner threads to finish and frees resources
void stop_miner_threads() {
    for (int i = 0; i < num_miners; i++) {
        pthread_join(miner_threads[i], NULL);
    }

    free(miner_threads);
    free(thread_args);
    miner_threads = NULL;
    thread_args = NULL;

    log_message("INFO: Stopped all miner threads");
}

// Entry point for the miner process
void run_miner_process(int num_threads) {
    signal(SIGINT, handle_sigint_miner);

    init_miner(num_threads, 0, 0);
    start_miner_threads();

    while (running) {
        sleep(1); // Can be replaced with more useful logic
    }

    stop_miner_threads();
    log_message("INFO: Miner process exiting");
}
