#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <fcntl.h>
#include <string.h>
#include "logging.h"
#include "common.h"  // Inclui Transaction, Config, TX_POOL_SHM

volatile sig_atomic_t stop_requested = 0;

// Signal handler for SIGINT
void handle_sigint(int sig) {
    (void)sig;
    stop_requested = 1;
}

sem_t* init_semaphore(const char* name) {
    sem_t* sem = sem_open(name, 0);
    if (sem == SEM_FAILED) {
        log_message("ERROR: sem_open falhou para %s", name);
        exit(EXIT_FAILURE);
    }
    log_message("INFO: Semáforo %s aberto com sucesso", name);
    return sem;
}

int main(int argc, char *argv[]) {
    log_init("DEIChain_log.txt");
    load_config("config.cfg", &global_config);

    if (argc != 3) {
        log_message("ERROR: Incorrect usage. Syntax: %s <reward 1-3> <sleep_time_ms 200-3000>", argv[0]);
        log_close();
        return EXIT_FAILURE;
    }

    int reward = atoi(argv[1]);
    int sleep_time = atoi(argv[2]);

    if (reward < 1 || reward > 3) {
        log_message("ERROR: reward must be between 1 and 3. Received: %d", reward);
        log_close();
        return EXIT_FAILURE;
    }

    if (sleep_time < 200 || sleep_time > 3000) {
        log_message("ERROR: sleep_time must be between 200 and 3000 ms. Received: %d", sleep_time);
        log_close();
        return EXIT_FAILURE;
    }

    srand(time(NULL) ^ getpid());
    signal(SIGINT, handle_sigint);

    log_message("TxGen started with reward = %d and sleep_time = %d ms", reward, sleep_time);

    // Liga à memória partilhada da transaction pool
    open_tx_pool_memory(global_config.pool_size);
    sem_t* sem_mutex = init_semaphore("/sem_mutex");
    sem_t* sem_empty = init_semaphore("/sem_empty");
    sem_t* sem_full  = init_semaphore("/sem_full");

    int counter = 0;

    while (!stop_requested) {
        Transaction t;
        t.id = getpid() * 10000 + counter++;
        t.reward = reward;
        t.sender_id = getpid();
        t.receiver_id = rand() % 1000 + 1;
        t.value = rand() % 100 + 1;
        t.age = 0;

        log_message("TRANSACTION | ID=%d | Reward=%d | From=%d | To=%d | Value=%d",
            t.id, t.reward, t.sender_id, t.receiver_id, t.value);

        sem_wait(sem_empty); // Enquanto houver espaçoes livres
        sem_wait(sem_mutex); // Lock para acesso à região crítica
        // Publica transação na primeira posição livre da pool
        for (int i = 0; i < global_config.pool_size; i++) {
            if (tx_pool_ptr->transactions_pending_set[i].empty) {
                tx_pool_ptr->transactions_pending_set[i] = t;
                tx_pool_ptr->transactions_pending_set[i].empty = 0;

                log_message("TxGen: Inserida transação %d no slot %d", t.id, i);
                break;
            }
        }

        sem_post(sem_mutex); // Liberta o acesso à região crítica
        sem_post(sem_full); // Informa que há uma nova transação disponível

        usleep(sleep_time * 1000); // Espera antes de gerar a próxima
    }

    sem_close(sem_mutex);
    sem_close(sem_empty);
    sem_close(sem_full);

    log_message("TxGen terminated by SIGINT (PID=%d)", getpid());
    log_close();
    return EXIT_SUCCESS;
}
