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
#include "controller.h"
#include "miner.h"

#define TX_POOL_SHM "/tx_pool_shm"
#define BLOCKCHAIN_SHM "/blockchain_shm"


int tx_pool_fd = -1, blockchain_fd = -1;
void* tx_pool_ptr = NULL;
void* blockchain_ptr = NULL;

volatile sig_atomic_t shutdown_requested = 0;
static pid_t miner_pid = -1;
static pid_t validator_pid = -1;
static pid_t statistics_pid = -1;

typedef struct {
    int teste;
} Block;

typedef void (*ProcessFunctionWithArgs)(void *);

// Função para tratar o sinal SIGINT
void handle_sigint(int sig) {
    (void)sig;
    shutdown_requested = 1;
    log_message("INFO: Sinal SIGINT capturado. Encerrando...");
    if (miner_pid > 0) {
        kill(miner_pid, SIGINT);
    }
}

// Função para ler o arquivo de configuração
void load_config(const char *filename, Config *config) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        log_message("ERRO: Falha ao abrir arquivo de configuração %s", filename);
        exit(EXIT_FAILURE);
    }

    if (fscanf(file, "%d", &config->num_miners) != 1 ||
        fscanf(file, "%d", &config->pool_size) != 1 ||
        fscanf(file, "%d", &config->transactions_per_block) != 1 ||
        fscanf(file, "%d", &config->blockchain_blocks) != 1) {
        
        log_message("ERRO: Formato incorreto no arquivo de configuração");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    fclose(file);
    
    if (config->num_miners <= 0 || config->pool_size <= 0 || 
        config->transactions_per_block <= 0 || config->blockchain_blocks <= 0) {
        log_message("ERRO: Valores de configuração inválidos (devem ser positivos)");
        exit(EXIT_FAILURE);
    }

    log_message("CONFIG: NUM_MINERS = %d", config->num_miners);
    log_message("CONFIG: POOL_SIZE = %d", config->pool_size);
    log_message("CONFIG: TRANSACTIONS_PER_BLOCK = %d", config->transactions_per_block);
    log_message("CONFIG: BLOCKCHAIN_BLOCKS = %d", config->blockchain_blocks);
}

// Criação e verificação de memória partilhada
void init_shared_memory(const Config* config) {
    size_t tx_pool_size = sizeof(Transaction) * config->pool_size;
    size_t blockchain_size = sizeof(Block) * config->blockchain_blocks;

    // TX Pool
    tx_pool_fd = shm_open(TX_POOL_SHM, O_CREAT | O_RDWR, 0666);
    if (tx_pool_fd == -1) {
        log_message("ERRO: Falha ao criar shared memory para tx_pool");
        exit(EXIT_FAILURE);
    }
    if (ftruncate(tx_pool_fd, tx_pool_size) == -1) {
        log_message("ERRO: ftruncate falhou em tx_pool");
        exit(EXIT_FAILURE);
    }
    tx_pool_ptr = mmap(NULL, tx_pool_size, PROT_READ | PROT_WRITE, MAP_SHARED, tx_pool_fd, 0);
    if (tx_pool_ptr == MAP_FAILED) {
        log_message("ERRO: mmap falhou em tx_pool");
        exit(EXIT_FAILURE);
    }
    log_message("SHM: tx_pool criada e mapeada com sucesso (%zu bytes)", tx_pool_size);

    // Blockchain
    blockchain_fd = shm_open(BLOCKCHAIN_SHM, O_CREAT | O_RDWR, 0666);
    if (blockchain_fd == -1) {
        log_message("ERRO: Falha ao criar shared memory para blockchain");
        exit(EXIT_FAILURE);
    }
    if (ftruncate(blockchain_fd, blockchain_size) == -1) {
        log_message("ERRO: ftruncate falhou em blockchain");
        exit(EXIT_FAILURE);
    }
    blockchain_ptr = mmap(NULL, blockchain_size, PROT_READ | PROT_WRITE, MAP_SHARED, blockchain_fd, 0);
    if (blockchain_ptr == MAP_FAILED) {
        log_message("ERRO: mmap falhou em blockchain");
        exit(EXIT_FAILURE);
    }
    log_message("SHM: blockchain criada e mapeada com sucesso (%zu bytes)", blockchain_size);
}

void cleanup_shared_memory() {
    // Desmapear memória partilhada
    if (tx_pool_ptr) {
        if (munmap(tx_pool_ptr, sizeof(Transaction)) == 0) {
            log_message("SHM: tx_pool desmapeada com sucesso");
        } else {
            log_message("ERRO: Falha ao desmapear tx_pool");
        }
    }

    if (blockchain_ptr) {
        if (munmap(blockchain_ptr, sizeof(Block)) == 0) {
            log_message("SHM: blockchain desmapeada com sucesso");
        } else {
            log_message("ERRO: Falha ao desmapear blockchain");
        }
    }

    // Fechar descritores
    if (tx_pool_fd != -1) {
        if (close(tx_pool_fd) == 0) {
            log_message("SHM: tx_pool descriptor fechado com sucesso");
        } else {
            log_message("ERRO: Falha ao fechar descriptor de tx_pool");
        }
    }

    if (blockchain_fd != -1) {
        if (close(blockchain_fd) == 0) {
            log_message("SHM: blockchain descriptor fechado com sucesso");
        } else {
            log_message("ERRO: Falha ao fechar descriptor de blockchain");
        }
    }

    // Unlink (remover do sistema)
    if (shm_unlink(TX_POOL_SHM) == 0) {
        log_message("SHM: tx_pool removida com sucesso");
    } else {
        log_message("ERRO: Falha ao remover tx_pool com shm_unlink");
    }

    if (shm_unlink(BLOCKCHAIN_SHM) == 0) {
        log_message("SHM: blockchain removida com sucesso");
    } else {
        log_message("ERRO: Falha ao remover blockchain com shm_unlink");
    }
}

void run_miner_process_wrapper(void *arg) {
    int num_threads = *((int*)arg);
    run_miner_process(num_threads);
}
pid_t create_process(const char *name, ProcessFunctionWithArgs func, void *args) {
    pid_t pid = fork();

    if (pid < 0) {
        log_message("ERROR: Failed to create %s process", name);
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Processo filho
        if (func != NULL) {
            func(args);
        } else {
            log_message("WARNING: No function defined for %s process", name);
        }
        exit(EXIT_SUCCESS);
    }

    log_message("INFO: %s process created with (PID: %d)", name, pid);
    return pid;
}


int main() {
    Config config;
    
    // Registar o handler para SIGINT
    signal(SIGINT, handle_sigint);

    log_init("DEIChain_log.txt");
    load_config("config.cfg", &config);

    init_shared_memory(&config);

    miner_pid = create_process("Miner", run_miner_process_wrapper, &config.num_miners);
    validator_pid = create_process("Validator", NULL, NULL);
    statistics_pid = create_process("Statistics", NULL, NULL);

    // Loop principal: espera até que SIGINT seja capturado
    while (!shutdown_requested) {
        pause(); // Pode ser substituído por lógica útil
    }

    waitpid(miner_pid, NULL, 0);
    waitpid(validator_pid, NULL, 0);
    waitpid(statistics_pid, NULL, 0);
    // Fechar ferramentas de sincronização
    log_message("INFO: System correctly shut down");
    log_close();

    return 0;
}

