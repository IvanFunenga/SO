#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <fcntl.h>     
#include <unistd.h>
#include "common.h"
#include "miner.h"
#include "logging.h"

static int num_miners;
static pthread_t* miner_threads = NULL;
static MinerThreadArgs* thread_args = NULL;
static volatile sig_atomic_t running = 1;

// Signal handler for SIGINT in the miner process
void handle_sigint_miner(int sig) {
    (void)sig;
    running = 0;
    log_message("INFO: SIGINT received by miner process, stopping mining...");
}

void send_block_to_validator(Block* b) {
    int fd = open(VALIDATOR_FIFO, O_WRONLY);
    if (fd == -1) {
        log_message("MINER: Failed to open FIFO for writing");
        return;
    }

    ssize_t bytes_written = write(fd, b, sizeof(Block));
    if (bytes_written == sizeof(Block)) {
        log_message("MINER: Block sent to Validator (ID=%d)", b->id);
    } else {
        log_message("ERROR: Incomplete block write to Validator FIFO");
    }

    close(fd);
}

// Function executed by each miner thread
void* miner_thread_func(void *arg) {
    MinerThreadArgs* args = (MinerThreadArgs*)arg;
    while (running) {
        log_message("INFO: Miner %d is working...", args->id);
        sleep(1);  // Simulates work
    }

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
