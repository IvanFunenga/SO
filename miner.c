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
#include <sys/mman.h>  
#include <sys/stat.h>

static int num_miners;
static pthread_t* miner_threads = NULL;
static MinerThreadArgs* thread_args = NULL;
static volatile sig_atomic_t running_miner = 1;

sem_t *sem_mutex = NULL;
sem_t *sem_full = NULL;
sem_t *sem_empty = NULL;

void handle_sigint_miner(int sig) {
    (void)sig;
    running_miner = 0;  // Mudar a variável de controle apenas para o miner
    log_message("INFO: SIGINT received by miner process, stopping mining...");
}

void send_block_to_validator(int fifo_fd, TransactionBlock* b) {
    ssize_t bytes_written = write(fifo_fd, b, sizeof(TransactionBlock));
    if (bytes_written == sizeof(TransactionBlock)) {
        log_message("MINER: Block sent to Validator (ID=%d)", b->txb_id);
    } else {
        log_message("ERROR: Incomplete block write to Validator FIFO");
    }
}

// Function executed by each miner thread
void* miner_thread_func(void *arg) {
    MinerThreadArgs* args = (MinerThreadArgs*)arg;

    int fifo_fd = args->fifo_fd;

    TransactionBlock block;
    int stored_count = 0;

    while (running_miner) {
        log_message("INFO: Miner %d is checking for transactions...", args->id);

        stored_count = 0;

        sem_wait(sem_full);    // Wait for a transaction to be available
        sem_wait(sem_mutex);   // Lock the pool for safe access
        
        for (int i = 0; i < global_config.pool_size && stored_count < global_config.transactions_per_block; i++) {
            // Check if the transaction is not empty
            if (!tx_pool_ptr->transactions_pending_set[i].empty) {
                // Store the transaction in the block's transactions array
                block.transactions[stored_count] = tx_pool_ptr->transactions_pending_set[i];
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
        sem_post(sem_full); 
        
        if (stored_count == global_config.transactions_per_block) {
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

    close_fifo(fifo_fd, VALIDATOR_FIFO);
    log_message("INFO: Miner thread %d stopping", args->id);
    return NULL;
}

// Initializes miner threads and arguments
void init_miner(int num_threads) {
    num_miners = num_threads;

    // Inicializa a memória compartilhada da tx_pool
    open_tx_pool_memory(global_config.pool_size);
    log_message("MINER: TX_POOL corretamente aberta");

    // Criar semáforos apenas uma vez antes de iniciar as threads
    sem_t *sem_mutex = sem_open("/sem_mutex", O_CREAT, 0666, 1);  // Mutex para proteger o acesso à tx_pool
    sem_t *sem_full = sem_open("/sem_full", O_CREAT, 0666, 0);    // Contagem de transações no pool

    if (sem_mutex == SEM_FAILED || sem_full == SEM_FAILED) {
        log_message("ERROR: Failed to open semaphores.");
        exit(EXIT_FAILURE);
    } else {
        log_message("MINER: Semáforos inicializados corretamente");
    }

    // Alocar memória para threads e seus argumentos
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

    init_miner(num_threads);
    start_miner_threads();

    while (running_miner) {
        sleep(1); // Can be replaced with more useful logic
    }

    stop_miner_threads();
    log_message("INFO: Miner process exiting");
}