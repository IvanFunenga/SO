#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <unistd.h>
#include "miner.h"
#include "logging.h"

static int num_miners;
static pthread_t* miner_threads = NULL;
static MinerThreadArgs* thread_args = NULL;
static volatile sig_atomic_t running = 1;

void handle_sigint_miner(int sig) {
    (void)sig;
    running = 0;
    log_message("INFO: SIGINT received by miner process, stopping mining...");
}

void* miner_thread_func(void *arg){
    MinerThreadArgs* args = (MinerThreadArgs*)arg;
    while (running) {

        log_message("INFO: Miner %d is working...", args->id);
        sleep(1);  // Simula trabalho
    }

    log_message("INFO: Miner thread %d stopping", args->id);
    return NULL;
}

void init_miner(int nm, int ps, int tpb){
    // Não vão ser usadas de momemto
    num_miners = nm;
    (void)ps;
    (void)tpb;

    // alocação de memória 
    miner_threads = malloc(num_miners * sizeof(pthread_t));
    thread_args = malloc(num_miners * sizeof(MinerThreadArgs));

    if (!miner_threads || !thread_args){
        log_message("ERROR: Failed to allocate memory for miner threads");
        exit(EXIT_FAILURE);
    }
}

void start_miner_threads(){
    for (int i = 0; i < num_miners; i++){
        thread_args[i].id = i;
        if (pthread_create(&miner_threads[i], NULL, miner_thread_func, &thread_args[i]) != 0){
            log_message("ERROR: Failed to create miner thread %d", i);
            exit(EXIT_FAILURE);
        }
        log_message("INFO: Successfully created miner thread %d", i);
    }
    log_message("INFO: All threads were created!");
}

void stop_miner_threads() {
    for (int i = 0; i < num_miners; i++){
        pthread_join(miner_threads[i], NULL);
    }

    free(miner_threads);
    free(thread_args);
    miner_threads = NULL;
    thread_args = NULL;

    log_message("INFO: Stopped all miner threads");
}

void run_miner_process(int num_threads) {
    signal(SIGINT, handle_sigint_miner);
    log_message("INFO: Miner process started (PID %d)", getpid());

    init_miner(num_threads, 0, 0);
    start_miner_threads();

    while (running) {
        sleep(1); // pode ser substituído por lógica mais útil
    }

    stop_miner_threads();
    log_message("INFO: Miner process exiting");
}