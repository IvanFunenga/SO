#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "logging.h"
#include "controller.h"
#include "miner.h"

volatile sig_atomic_t shutdown_requested = 0;
static pid_t miner_pid = -1;

// Função para ler o arquivo de configuração
void load_config(const char *filename, Config *config) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        log_message("ERRO: Falha ao abrir arquivo de configuração %s", filename);
        exit(EXIT_FAILURE);
    }

    // Leitura dos valores com verificação de erro
    if (fscanf(file, "%d", &config->num_miners) != 1 ||
        fscanf(file, "%d", &config->pool_size) != 1 ||
        fscanf(file, "%d", &config->transactions_per_block) != 1 ||
        fscanf(file, "%d", &config->blockchain_blocks) != 1) {
        
        log_message("ERRO: Formato incorreto no arquivo de configuração");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    fclose(file);
    
    // Validação básica dos valores
    if (config->num_miners <= 0 || config->pool_size <= 0 || 
        config->transactions_per_block <= 0 || config->blockchain_blocks <= 0) {
        log_message("ERRO: Valores de configuração inválidos (devem ser positivos)");
        exit(EXIT_FAILURE);
    }

    // Log das configurações lidas
    log_message("CONFIG: NUM_MINERS = %d", config->num_miners);
    log_message("CONFIG: POOL_SIZE = %d", config->pool_size);
    log_message("CONFIG: TRANSACTIONS_PER_BLOCK = %d", config->transactions_per_block);
    log_message("CONFIG: BLOCKCHAIN_BLOCKS = %d", config->blockchain_blocks);
}


void start_miner_process(int num_threads){
    log_message("INFO: Miner process started (PID: %d)", getpid());
    init_miner(num_threads, 0, 0); // Transaction pool e transaction per block nulos de momento
    start_miner_threads();
    exit(EXIT_SUCCESS);
}

void stop_miner_process(){
    stop_miner_threads();
    if (miner_pid > 0){
        waitpid(miner_pid, NULL, 0);  // Espera o processo filho terminar
        log_message("INFO: Miner process stopped");
    } 
}

int main() {
    Config config;
    pid_t miner_pid;
    
    // Inicializa sistema de logging
    log_init("DEIChain_log.txt");
    load_config("config.cfg", &config);
    

    // Lógica para inicalizar o processo miner
    miner_pid = fork();

    if (miner_pid < 0){
        log_message("ERROR: Failed to create miner process");
        exit(EXIT_FAILURE);
    } else if (miner_pid == 0){
        start_miner_process(config.num_miners);
    }   

    sleep(10);
    stop_miner_process();


    log_message("INFO: System correctly shut down");
    log_close();
    
    return 0;
}