#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <semaphore.h>
#include "logging.h"
#include "common.h"
#include "miner.h"

#define TX_POOL_SHM "/tx_pool_shm"
#define BLOCKCHAIN_SHM "/blockchain_shm"
#define NUM_SEMAPHORES 3

int blockchain_fd = -1;
void* blockchain_ptr = NULL;

volatile sig_atomic_t shutdown_requested = 0;
static pid_t miner_pid = -1;
static pid_t validator_pid = -1;
static pid_t statistics_pid = -1;

typedef struct {
    sem_t* handle;
    const char* name;
} NamedSemaphore;

NamedSemaphore semaphores[NUM_SEMAPHORES];
int semaphore_count = 0;

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

int create_named_semaphore(const char* name, unsigned int initial_value){
    sem_t* sem = sem_open(name, O_CREAT, 0666, initial_value);
    if (sem == SEM_FAILED) {
        perror("Erro ao criar semáforo");
        return -1;
    }

    semaphores[semaphore_count].handle = sem;
    semaphores[semaphore_count].name = name;
    semaphore_count++;

    return 0;
}

void cleanup_named_semaphores() {
    for (int i = 0; i < NUM_SEMAPHORES; i++) {
        sem_close(semaphores[i].handle);
        sem_unlink(semaphores[i].name);
    }
    semaphore_count = 0;
}

// Funções auxiliares
static void safe_munmap(void* addr, size_t size, const char* name) {
    if (addr && munmap(addr, size) == 0) {
        log_message("SHM: %s unmapped successfully", name);
    } else {
        log_message("ERROR: Failed to unmap %s", name);
    }
}

static void safe_close(int fd, const char* name) {
    if (fd != -1 && close(fd) == 0) {
        log_message("SHM: %s descriptor closed successfully", name);
    } else {
        log_message("ERROR: Failed to close descriptor for %s", name);
    }
}

static void safe_unlink(const char* name) {
    if (shm_unlink(name) == 0) {
        log_message("SHM: %s unlinked successfully", name);
    } else {
        log_message("ERROR: Failed to unlink %s", name);
    }
}

// Inicialização da blockchain
void init_blockchain_memory(const Config* config) {
    // Calcular o tamanho de um bloco (baseado no número de transações por bloco)
    size_t block_size = sizeof(TransactionBlock) + sizeof(Transaction) * config->transactions_per_block;

    // Calcular o tamanho total da blockchain
    size_t blockchain_size = block_size * config->blockchain_blocks;

    // Criar a memória compartilhada para a blockchain
    SharedMemory shm = create_shared_memory(BLOCKCHAIN_SHM, blockchain_size);
    blockchain_ptr = shm.ptr;
    blockchain_fd = shm.fd;

    // Inicializar blocos na memória
    for (int i = 0; i < config->blockchain_blocks; i++) {
        TransactionBlock* block = (TransactionBlock*)((char*)blockchain_ptr + i * block_size);

        // Inicializar o nonce (inicialmente 0)
        block->nonce = 0;
    }

    log_message("SHM: Blockchain created and mapped successfully with %zu blocks", config->blockchain_blocks);
}

// Unmap and unlink shared memory
void cleanup_shared_memory() {
    // Desfazer mappings
    size_t tx_pool_size = sizeof(TransactionPool) + sizeof(Transaction) * global_config.pool_size;
    size_t blockchain_size = sizeof(TransactionBlock) * global_config.blockchain_blocks;

    safe_munmap(tx_pool_ptr, tx_pool_size, "tx_pool");
    safe_munmap(blockchain_ptr, blockchain_size, "blockchain");

    // Fechar descritores
    safe_close(tx_pool_fd, "tx_pool");
    safe_close(blockchain_fd, "blockchain");

    // Remover objetos de memória
    safe_unlink(TX_POOL_SHM);
    safe_unlink(BLOCKCHAIN_SHM);
}

void create_named_pipe() {
        if (mkfifo(VALIDATOR_FIFO, O_CREAT|O_EXCL|0600)<0){
            if (errno != EEXIST) {
                log_message("ERROR: Failed to create FIFO %s: %s", VALIDATOR_FIFO, strerror(errno));
                exit(0);
            } else {
                log_message("INFO: FIFO %s already exists", VALIDATOR_FIFO);
            }
        } else {
            log_message("INFO: FIFO %s created successfully", VALIDATOR_FIFO);
        }
}

void cleanup_named_pipe(){
    if(unlink(VALIDATOR_FIFO) == 0){
        log_message("INFO: FIFO %s removed successfully", VALIDATOR_FIFO);
    } else {
        if (errno == ENOENT){
            log_message("INFO: FIFO %s already removed or not found", VALIDATOR_FIFO);
        } else {
            log_message("ERROR: Failed to unlink FIFO %s: %s", VALIDATOR_FIFO, strerror(errno));
        }
    }
}

void print_tx_pool(TransactionPool* pool, int pool_size) {
    printf("\n=== Conteúdo da Transaction Pool ===\n");
    printf("Current Block ID: %s\n", pool->current_block_hash);
    for (int i = 0; i < pool_size; i++) {
        Transaction* t = &pool->transactions_pending_set[i];
        if (t->empty) {
            printf("[Slot %d] VAZIO\n", i);
        } else {
            printf("[Slot %d] ID=%d | From=%d | To=%d | Value=%d | Reward=%d | Aging=%d\n",
                   i,
                   t->id,
                   t->sender_id,
                   t->receiver_id,
                   t->value,
                   t->reward,
                   t->age);
        }
    }
    printf("=====================================\n");
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
    
    create_tx_pool_memory(&global_config);
    init_blockchain_memory(&global_config);
    create_named_semaphore("/sem_mutex", 1);
    create_named_semaphore("/sem_empty", global_config.pool_size);
    create_named_semaphore("/sem_full", 0);
    create_named_pipe();

    miner_pid = create_process("Miner", run_miner_process_wrapper, &global_config.num_miners);
    validator_pid = create_process("Validator", NULL, NULL);
    statistics_pid = create_process("Statistics", NULL, NULL);

    // Main loop: wait for SIGINT
    while (!shutdown_requested) {
        print_tx_pool((TransactionPool*)tx_pool_ptr, global_config.pool_size);
        sleep(10);
        //pause(); // Can be replaced by useful logic
    }

    waitpid(miner_pid, NULL, 0);
    waitpid(validator_pid, NULL, 0);
    waitpid(statistics_pid, NULL, 0);

    cleanup_named_semaphores();
    cleanup_shared_memory();
    cleanup_named_pipe();
    log_message("INFO: System shut down successfully");
    log_close();

    return 0;
}