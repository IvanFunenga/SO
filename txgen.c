#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include "logging.h"

volatile sig_atomic_t stop_requested = 0;

typedef struct {
    int id;
    int reward;
    int sender_id;
    int receiver_id;
    int value;
    time_t timestamp;
    int aging;
} Transaction;

void handle_sigint(int sig) {
    (void)sig;
    stop_requested = 1;
}

int main(int argc, char *argv[]) {
    log_init("DEIChain_log.txt");

    if (argc != 3) {
        log_message("ERRO: Uso incorreto. Sintaxe: %s <reward 1-3> <sleep_time_ms 200-3000>", argv[0]);
        log_close();
        return EXIT_FAILURE;
    }

    int reward = atoi(argv[1]);
    int sleep_time = atoi(argv[2]);

    if (reward < 1 || reward > 3) {
        log_message("ERRO: reward deve estar entre 1 e 3. Valor recebido: %d", reward);
        log_close();
        return EXIT_FAILURE;
    }

    if (sleep_time < 200 || sleep_time > 3000) {
        log_message("ERRO: sleep_time deve estar entre 200 e 3000 ms. Valor recebido: %d", sleep_time);
        log_close();
        return EXIT_FAILURE;
    }

    srand(time(NULL) ^ getpid());
    signal(SIGINT, handle_sigint);

    log_message("TxGen iniciado com reward = %d e sleep_time = %d ms", reward, sleep_time);

    int counter = 0;

    while (!stop_requested) {
        Transaction t;
        t.id = getpid() * 10000 + counter++;
        t.reward = reward;
        t.sender_id = getpid();
        t.receiver_id = rand() % 1000 + 1;
        t.value = rand() % 100 + 1;
        t.aging = 0;

        log_message("TRANSACTION | ID=%d | Reward=%d | From=%d | To=%d | Value=%d",
            t.id, t.reward, t.sender_id, t.receiver_id, t.value);

        usleep(sleep_time * 1000); // Espera entre transações
    }

    log_message("TxGen encerrado com SIGINT (PID=%d)", getpid());
    log_close();
    return EXIT_SUCCESS;
}
